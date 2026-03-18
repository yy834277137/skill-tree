#include "xray_image_process.hpp"
#include "xsp_alg.hpp"
#include "utils/ini.h"
#include <string>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>

static int fd_cycles, fd_instructions, fd_cache_misses;
static int fd_stalled_frontend, fd_stalled_backend;
static struct perf_event_attr attr;

static int perf_event_open(struct perf_event_attr *attr, pid_t pid, 
                          int cpu, int group_fd, unsigned long flags) {
    return syscall(SYS_perf_event_open, attr, pid, cpu, group_fd, flags);
}

static void perf_init(void) {
    memset(&attr, 0, sizeof(attr));
    attr.size = sizeof(attr);
    attr.disabled = 1;
    attr.exclude_kernel = 1;
    attr.exclude_hv = 1;
    
    // 基础计数器
    attr.type = PERF_TYPE_HARDWARE;
    
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    fd_cycles = perf_event_open(&attr, 0, -1, -1, 0);
    
    attr.config = PERF_COUNT_HW_INSTRUCTIONS;
    fd_instructions = perf_event_open(&attr, 0, -1, -1, 0);
    
    attr.config = PERF_COUNT_HW_CACHE_MISSES;
    fd_cache_misses = perf_event_open(&attr, 0, -1, -1, 0);
    
    // 停滞周期计数器 - 关键诊断指标
    attr.config = PERF_COUNT_HW_STALLED_CYCLES_FRONTEND;
    fd_stalled_frontend = perf_event_open(&attr, 0, -1, -1, 0);
    
    attr.config = PERF_COUNT_HW_STALLED_CYCLES_BACKEND;
    fd_stalled_backend = perf_event_open(&attr, 0, -1, -1, 0);
}

static void perf_start(void) {
    ioctl(fd_cycles, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd_instructions, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd_cache_misses, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd_stalled_frontend, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd_stalled_backend, PERF_EVENT_IOC_RESET, 0);
    
    ioctl(fd_cycles, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(fd_instructions, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(fd_cache_misses, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(fd_stalled_frontend, PERF_EVENT_IOC_ENABLE, 0);
    ioctl(fd_stalled_backend, PERF_EVENT_IOC_ENABLE, 0);
}

static void perf_stop(void) {
    long long count_cycles, count_instructions, count_cache_misses;
    long long count_stalled_frontend, count_stalled_backend;
    
    ioctl(fd_cycles, PERF_EVENT_IOC_DISABLE, 0);
    ioctl(fd_instructions, PERF_EVENT_IOC_DISABLE, 0);
    ioctl(fd_cache_misses, PERF_EVENT_IOC_DISABLE, 0);
    ioctl(fd_stalled_frontend, PERF_EVENT_IOC_DISABLE, 0);
    ioctl(fd_stalled_backend, PERF_EVENT_IOC_DISABLE, 0);
    
    read(fd_cycles, &count_cycles, sizeof(count_cycles));
    read(fd_instructions, &count_instructions, sizeof(count_instructions));
    read(fd_cache_misses, &count_cache_misses, sizeof(count_cache_misses));
    read(fd_stalled_frontend, &count_stalled_frontend, sizeof(count_stalled_frontend));
    read(fd_stalled_backend, &count_stalled_backend, sizeof(count_stalled_backend));
    
    // 计算关键指标
    double cpi = (double)count_cycles / count_instructions;
    double stalled_frontend_ratio = (double)count_stalled_frontend / count_cycles * 100;
    double stalled_backend_ratio = (double)count_stalled_backend / count_cycles * 100;
    double total_stalled_ratio = stalled_frontend_ratio + stalled_backend_ratio;
    
    printf("=== Performance Analysis Results ===\n");
    printf("CPU Cycles: %lld\n", count_cycles);
    printf("Instructions: %lld\n", count_instructions);
    printf("Cache Misses: %lld\n", count_cache_misses);
    printf("CPI: %.2f\n", cpi);
    printf("Frontend Stalled Cycles: %lld (%.1f%%)\n", count_stalled_frontend, stalled_frontend_ratio);
    printf("Backend Stalled Cycles: %lld (%.1f%%)\n", count_stalled_backend, stalled_backend_ratio);
    printf("Total Stalled Ratio: %.1f%%\n", total_stalled_ratio);
    
    // 瓶颈诊断
    printf("=== Bottleneck Diagnosis ===\n");
    if (total_stalled_ratio > 30) {
        printf("RED Memory Bound: High stalled cycle ratio\n");
        if (stalled_backend_ratio > 20) {
            printf("   - Severe backend stalls -> Memory access bottleneck\n");
        }
        if (stalled_frontend_ratio > 10) {
            printf("   - Frontend stalls -> Instruction cache or branch prediction issues\n");
        }
    } else if (total_stalled_ratio < 15) {
        printf("GREEN Compute Bound: Low stalled cycle ratio\n");
        printf("   - CPU compute units are the bottleneck\n");
    } else {
        printf("YELLOW Balanced: Medium stalled cycle ratio\n");
    }
    
    // 关闭所有计数器
    close(fd_cycles);
    close(fd_instructions);
    close(fd_cache_misses);
    close(fd_stalled_frontend);
    close(fd_stalled_backend);
}

int32_t g_nCount = 0;
int32_t XRayImageProcess::m_SnTipHeightRand = -1;
bool XRayImageProcess::m_SbMainReady = false;
bool XRayImageProcess::m_SbPackReady = false;
int32_t XRayImageProcess::m_SbPackHeightRemian = 0;
XRAYLIB_TIP_STATUS XRayImageProcess::m_SeStatus = XRAYLIB_TIP_NONE;
#define XRAY_LIB_INVALID_WIDTH -1 

#if defined _MSC_VER
#ifndef WINDOWS
#define WINDOWS
#endif
#elif defined __GNUC__
#ifndef LINUX
#define LINUX
#endif
#endif

#if defined WINDOWS
#include <direct.h>
#elif defined LINUX
#include <unistd.h>
#endif

char const *deviceTypeArray[] =
{
	"",
	"XRAYLIB_DEVICETYPE_6550",
	"XRAYLIB_DEVICETYPE_5030",
	"XRAYLIB_DEVICETYPE_100100",
	"XRAYLIB_DEVICETYPE_4233",
	"XRAYLIB_DEVICETYPE_140100",
	"XRAYLIB_DEVICETYPE_6040",
};

char const *modeldeviceTypeArray[] =
{
	"",
	"AI_XSP_6550",
	"AI_XSP_5030",
	"AI_XSP_100100",
	"AI_XSP_4233",
	"AI_XSP_140100",
	"AI_XSP_6040",
};

char const *energyModeArray[] =
{
	"D", // 双能
	"H", // 高能
	"L", // 低能
	"BS", // 背散射
};

char const *visualNoArray[] =
{
	"_MAIN",
	"_AUX",
	"_SINGLE",
};

char const *valueTypeArray[] =
{
	"_SC",
	"_SG",
	"_CA",
};

char const *detectorTypeArray[] =
{
	"",
	"_DT",
	"_SUNFY",
	"_IRAY",
	"_HAMAMATSU",
	"_TYW",
	"_VIDETECT",
	"_RAYIN",
	"_DT_CA",
	"_RAYIN_QIPAN",
	"_RAYIN_GOS",
	"_RAYIN_DDC264",
	"_RAYIN_CSI",
	"_RAYIN_MCD_HS",
	"_RAYIN_DIGTAL32",
	"_RAYIN_BS_DSB",
};

/**
* @brief 获取源字符串
* 射线源枚举暂不按照字符串数组查询
*/
static const char* GetSourceString(XRAYLIB_SOURCE_TYPE_E type)
{
	switch (type)
	{
		/* 射线源厂商-机电院 */
	case XRAYLIB_SOURCE_JDY_T160:   return "_JDY_T160";
	case XRAYLIB_SOURCE_JDY_T140:   return "_JDY_T140";
	case XRAYLIB_SOURCE_JDY_T80:    return "_JDY_T80";
	case XRAYLIB_SOURCE_HIK_T120:   return "_HIK_T120";
	case XRAYLIB_SOURCE_JDY_T120:   return "_JDY_T120";
	case XRAYLIB_SOURCE_JDY_T160YT: return "_JDY_T160YT";
	case XRAYLIB_SOURCE_JDY_T2050:  return "_JDY_T2050";
	case XRAYLIB_SOURCE_JDY_T140RT: return "_JDY_T140RT";
	case XRAYLIB_SOURCE_JDY_TM80:   return "_JDY_TM80";
		/* 射线源厂商-凯威信达 */
	case XRAYLIB_SOURCE_KVXD_X160:  return "_KVXD_T160";
		/* 射线源厂商-超群 */
	case XRAYLIB_SOURCE_CQ_T200:    return "_CQ_T200";
		/* 射线源厂商-博思得 */
	case XRAYLIB_SOURCE_BSD_T160:   return "_BSD_T160";
	case XRAYLIB_SOURCE_BSD_T140:   return "_BSD_T140";
	case XRAYLIB_SOURCE_BSD_T80:    return "_BSD_T80";
		/* 射线源厂商-spellman */
	case XRAYLIB_SOURCE_SPM_S180:   return "_SPM_S180";
	case XRAYLIB_SOURCE_SPM_S180L:  return "_SPM_S180L";
		/* 射线源厂商-深圳力能时代 LIOENERGY */
	case XRAYLIB_SOURCE_LIOENERGY_LXB80:   return "_LIOENERGY_LXB80";
	/* 射线源厂商-睿影*/
	case XRAYLIB_SOURCE_RAYIN_XSD180:	return "_RAYIN_XSD180";
	case XRAYLIB_SOURCE_RAYIN_XSD160:	return "_RAYIN_XSD160";
	case XRAYLIB_SOURCE_RAYIN_XSD160L:	return "_RAYIN_XSD160L";

		/* 射线源厂商-通用型号-算法测试 */
	case XRAYLIB_SOURCE_NORMAL:     return "";
	}
	return "_UnknowSource";
}

// @brief 获取机型大类字符串
static const char* GetDeviceTypeString(XRAY_DEVICETYPE type)
{
	int32_t length = sizeof(deviceTypeArray) / sizeof(deviceTypeArray[0]);
	// 安检机机型大类，0~3位表示SG、SC子型号，第8~11位表示机型大类
	int32_t nDeviceType = type >> 8;
	if (nDeviceType > length || nDeviceType < 0)
	{
		return "UnknowDevice";
	}
	return deviceTypeArray[nDeviceType];
}
// @brief 获取机型子型号字符串
static const char* GetValueTypeString(XRAY_DEVICETYPE type)
{
	int32_t length = sizeof(valueTypeArray) / sizeof(valueTypeArray[0]);
	// 安检机机型大类，0~3位表示SG、SC子型号，第8~11位表示机型大类
	int32_t nValueType = type & 0x00F;
	if (nValueType > length || nValueType < 0)
	{
		return "_UnknowType";
	}
	return valueTypeArray[nValueType];
}


// @brief 获取AI机型大类字符串
static const char* GetAiDeviceTypeString(XRAY_DEVICETYPE type)
{
	int32_t length = sizeof(modeldeviceTypeArray) / sizeof(modeldeviceTypeArray[0]);
	// 安检机机型大类，0~3位表示SG、SC子型号，第8~11位表示机型大类
	int32_t nDeviceType = type >> 8;
	if (nDeviceType > length || nDeviceType < 0)
	{
		return "UnknowDevice";
	}
	return modeldeviceTypeArray[nDeviceType];
}


// @brief 获取探测器字符串
static const char* GetDetectorString(XRAY_DETECTORNAME type)
{
	int32_t length = sizeof(detectorTypeArray) / sizeof(detectorTypeArray[0]);
	int32_t nValueType = type;
	if (nValueType > length || nValueType < 0)
	{
		return "_UnknowDetector";
	}
	return detectorTypeArray[nValueType];
}

// @brief 获取视角字符串
static const char* GetVisualString(XRAY_LIB_VISUAL type)
{
	int32_t length = sizeof(visualNoArray) / sizeof(visualNoArray[0]);
	int32_t nValueType = type;
	if (nValueType > length || nValueType < 0)
	{
		return "_UnknowVisual";
	}
	return visualNoArray[nValueType];
}

// @brief 获取能量字符串
static const char* GetEnergyString(XRAYLIB_ENERGY_MODE type)
{
	int32_t length = sizeof(energyModeArray) / sizeof(energyModeArray[0]);
	int32_t nValueType = type;
	if (nValueType > length || nValueType < 0)
	{
		return "_UnknowEnergy";
	}
	return energyModeArray[nValueType];
}

size_t GetXspObjSize()
{
	return sizeof(XRayImageProcess);
}

IXRayImageProcess* CreateXspObj(void* ptr)
{
	return new (ptr)XRayImageProcess;
}

void DestroyXspObj(IXRayImageProcess* pExport)
{
	pExport->Release();
}

XRayImageProcess::XRayImageProcess() : m_islPipeComm({&m_modImgproc.m_islPipeGray, &m_modDual.m_islPipeChroma})
{
	m_nDetectorWidth = 0;

	m_fHeightResizeFactor = 1.0f;
	m_fWidthResizeFactor = 1.0f;
	m_nResizeHeight = 0;
	m_nResizeWidth = 0;
	m_nResizePadLen = XRAY_LIB_MAX_FILTER_KERNEL_LENGTH;
	m_nPadLen = XRAY_LIB_MAX_FILTER_KERNEL_LENGTH;

	m_fDenoiseSigma = 1.0f;
	m_fSigmaDn = 1.0f;
	m_fSigmaSr = 1.0f;
	m_AiEdgeFactor = 50;

	m_sharedPara.m_nBackGroundThreshold = 62000;
	m_sharedPara.m_nBackGroundGray = 65000;
	m_sharedPara.m_nPackageThresholdR = 59000;
	m_sharedPara.m_nDetectorWidth = 0;

	/* 彩图旋转方向 */
	m_enRotate = XRAYLIB_MOVE_RIGHT;
	m_enRotateRT = XRAYLIB_MOVE_RIGHT;
	m_enRotateEntire = XRAYLIB_MOVE_RIGHT;
	m_enMirror = XRAYLIB_MIRROR_NONE;

	/* TIP参数初始化 */
	m_nNeedTip = 0;
	m_nTipHeight = 0;
	m_nTipWidth = 0;
	m_nTipedHeight = 0;
	m_nTipWidthStart = -1;
	m_nTipStripeStart = -1;
	m_nPackageHeight = 0;
	m_nRandomPosition = 0;
	m_vecCoordinates.reserve(18);
	m_vecCoordinates = { 0,0,0,1,0,2,1,0,1,1,1,2,2,0,2,1,2,2 };

	m_enScanMode = XRAYLIB_SCAN_MODE_NORMAL;////强扫状态-强扫状态时包裹判断一律返回有包裹

	m_nMaxHeightRealTime = XRAY_LIB_MAX_RTPROCESS_LENGTH;
	m_fMaxResizeFactor = 2.0f;

	m_enDefaultEnhanceJudge = XRAYLIB_DEFAULTENHANCE_MODE3;

	/* 曲线状态 */
	m_bMixCurveValid = false;

	procWtCache.Init();

	m_AllowSyncOperation.store(true);

	/* 子模块载入 */
	m_vecModPtr.push_back(&m_modCali);
	m_vecModPtr.push_back(&m_modDual);
	m_vecModPtr.push_back(&m_modArea);
	m_vecModPtr.push_back(&m_modImgproc);
	m_vecModPtr.push_back(&m_modCnn);
	m_vecModPtr.push_back(&m_modTcproc);

    return;
}


XRAY_LIB_HRESULT XRayImageProcess::Init(XRAYLIB_PIPELINE_MODE xraylib_pipeline_mode,
	                                    XRAYLIB_FILEPATH_MODE enFilePathNameMode,
	                                    XRAY_DEVICEINFO       xraylib_deviceInfo,
	                                    XRAYLIB_AI_PARAM      xraylib_AIPara,
	                                    XRAY_LIB_MEM_TAB*     memTab,
	                                    const char*           PublicFileFolderName,
	                                    const char*           PrivateFileFolderName,
	                                    XRAY_LIB_ABILITY*     ability)
{
	/* 堆上申请的算法库对象内部指针需重新初始化内存表 */
	XSP_CHECK(m_vecModPtr.size() == XRAY_LIB_MTAB_NUM - 2, XRAY_LIB_MEM_ERROR);
	this->GetMemSize(memTab[1], *ability);
	this->InitMemTab(memTab[1]);
	/* 各模块内存初始化及分配 */
	for (int32_t i = 0; i < (int32_t)m_vecModPtr.size(); i++)
	{
		m_vecModPtr[i]->GetMemSize(memTab[i + 2], *ability);
		m_vecModPtr[i]->InitMemTab(memTab[i + 2]);
	}

	/***************************************** 
	*                路径初始化 
	******************************************/
	memset(m_szPath, 0, sizeof(char) * (XRAY_LIB_MAX_PATH));
	memset(m_szPublicFileFolderName, 0, sizeof(char) * (XRAY_LIB_MAX_PATH));
	memset(m_szPrivateFileFolderName, 0, sizeof(char) * (XRAY_LIB_MAX_PATH));

	memset(m_szCnnKeyName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
	memset(m_szCnnKeyNameSubType, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
	memset(m_szDeviceKeyName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
	memset(m_szZTableName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
	memset(m_szAutoZTableName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
	memset(m_szZ6TableName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
	memset(m_szMixZTableName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
	memset(m_szGeoCaliName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
	memset(m_szFlatDetCaliName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
	memset(m_szGeometryName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));

	memset(m_szDevicePath, 0, sizeof(char) * (XRAY_LIB_MAX_NAME + XRAY_LIB_MAX_NAME));

	if (XRAYLIB_FILEPATH_OPPOSITE == enFilePathNameMode)
	{
#if defined WINDOWS
		_getcwd(m_szPath, XRAY_LIB_MAX_PATH);
		strcat(m_szPath, "/");
#elif defined LINUX
		int32_t cnt = readlink("/proc/self/exe", m_szPath, XRAY_LIB_MAX_PATH);
		if (cnt < 0 || cnt >= XRAY_LIB_MAX_PATH)
			return XRAY_LIB_FINDEXEPATH_FAIL;
		int32_t i;
		for (i = cnt; i >= 0; --i)
		{
			if (m_szPath[i] == '/')
			{
				m_szPath[i + 1] = '\0';
				break;
			}
		}
#endif
		strcpy(m_szPublicFileFolderName, m_szPath);
		strcat(m_szPublicFileFolderName, PublicFileFolderName);

		strcpy(m_szPrivateFileFolderName, m_szPath);
		strcat(m_szPrivateFileFolderName, PrivateFileFolderName);
	}
	else if (XRAYLIB_FILEPATH_ABSOLUTE == enFilePathNameMode)
	{
		strcpy(m_szPublicFileFolderName, PublicFileFolderName);
		strcpy(m_szPrivateFileFolderName, PrivateFileFolderName);
	}

	strcpy(m_szDevicePath, m_szPublicFileFolderName);
	if ((xraylib_deviceInfo.xraylib_devicetype & 0xF00) == XRAY_DEVICETYPE_6550_SC)
	{
		strcat(m_szDevicePath, "/DeviceInfo_6550.ini");
	}
	else if ((xraylib_deviceInfo.xraylib_devicetype & 0xF00) == XRAY_DEVICETYPE_5030_SC)
	{
		strcat(m_szDevicePath, "/DeviceInfo_5030.ini");
	}
	else if ((xraylib_deviceInfo.xraylib_devicetype & 0xF00) == XRAY_DEVICETYPE_100100_SC)
	{
		strcat(m_szDevicePath, "/DeviceInfo_100100.ini");
	}
	else if ((xraylib_deviceInfo.xraylib_devicetype & 0xF00) == XRAY_DEVICETYPE_4233_SC)
	{
		strcat(m_szDevicePath, "/DeviceInfo_4233.ini");
	}
	else if ((xraylib_deviceInfo.xraylib_devicetype & 0xF00) == XRAY_DEVICETYPE_140100_SC)
	{
		strcat(m_szDevicePath, "/DeviceInfo_140100.ini");
	}
	else if ((xraylib_deviceInfo.xraylib_devicetype & 0xF00) == XRAY_DEVICETYPE_6040_SC)
	{
		strcat(m_szDevicePath, "/DeviceInfo_6040.ini");
	}
	else
	{
		return XRAY_LIB_DEVICEPARA_ERROR;
	}

	XRAY_LIB_HRESULT hr;
	hr = GetKeyNameByDeviceInfo(xraylib_deviceInfo, m_szCnnKeyName, m_szCnnKeyNameSubType, m_szDeviceKeyName);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	/* 算法库对外信息打印 */
	log_info("DeviceInfo path : %s\n", m_szDevicePath);
	log_info("szDeviceKey = %s\n", m_szDeviceKeyName);
	log_info("szCnnKey = %s\n", m_szCnnKeyName);
	log_info("szCnnKeySub = %s\n", m_szCnnKeyNameSubType);
#ifdef SVN_NUM
	log_info("xsp svn version : %d", SVN_NUM);
#endif

	/* 对外信息输出后，初始化日志等级 */
#ifdef XSP_DEBUG
	xlog::log_set_level(LOG_TRACE);
#else
	xlog::log_set_level(LOG_WARN);
#endif

	hr = (XRAYLIB_ENERGY_SCATTER == ability->nEnergyMode) ? InitBSDeviceParaByIni(m_szDevicePath, m_szDeviceKeyName) : 
	 														InitDeviceParaByIni(m_szDevicePath, m_szDeviceKeyName);
	XSP_CHECK(XRAY_LIB_OK == hr, XRAY_LIB_DEVICEPARA_ERROR);

	int32_t nImageWidth = ability->nDetectorWidth;

	/* 参数初始化 */
	m_enPipelineMode = xraylib_pipeline_mode;
	m_nDetectorWidth = nImageWidth;
	m_nMaxHeightRealTime = ability->nMaxHeightRealTime;
	m_fMaxResizeFactor = ability->fResizeScale;
	m_enDefaultEnhanceJudge = m_modImgproc.m_enDefaultEnhance;
	m_enVisual = xraylib_deviceInfo.xraylib_visualNo;
	
	if (XRAYLIB_VISUAL_MAIN == m_enVisual)
	{
		strcpy(m_szAutoZTableName, "cali_curve_main.tbe");
	}
	else if (XRAYLIB_VISUAL_AUX == m_enVisual)
	{
		strcpy(m_szAutoZTableName, "cali_curve_aux.tbe");
	}
	else if (XRAYLIB_VISUAL_SINGLE == m_enVisual)
	{
		strcpy(m_szAutoZTableName, "cali_curve_single.tbe");
	}
	else
	{
		return XRAY_LIB_VISUALVIEW_ERROR;
	}

	/* 共享参数初始化 */
	m_sharedPara.m_enEnergyMode = xraylib_deviceInfo.xraylib_energymode;
	m_sharedPara.m_enDeviceInfo = xraylib_deviceInfo;
	m_sharedPara.m_enVisual = xraylib_deviceInfo.xraylib_visualNo;
	m_sharedPara.m_nDetectorWidth = nImageWidth;
	m_sharedPara.m_fResizeScale = ability->fResizeScale;
	m_sharedPara.m_nMaxHeightEntire = ability->nMaxHeightEntire;
	m_sharedPara.m_nPerDetectorWidth = static_cast<XRAY_DETECTOR_PIXEL>(ability->nPerDetectorPixel);

	XSP_CHECK(m_sharedPara.m_nPerDetectorWidth >= XRAY_DETECTOR_PIXEL_MIN_VALUE &&
			  m_sharedPara.m_nPerDetectorWidth <= XRAY_DETECTOR_PIXEL_MAX_VALUE, 
			  XRAY_LIB_PARAM_WIDTH_ERROR);

	XSP_CHECK(m_sharedPara.m_enEnergyMode >= XRAYLIB_ENERGY_DUAL &&
			  m_sharedPara.m_enEnergyMode <= XRAYLIB_ENERGY_SCATTER,
			  XRAY_LIB_ENERGYMODE_ERROR);

	/*****************************************
	*              子模块初始化
	******************************************/

	/* 校正模块初始化 */
	hr = m_modCali.Init(m_szPublicFileFolderName, m_szPrivateFileFolderName, 
		                m_szGeoCaliName, m_szFlatDetCaliName, m_szGeometryName, 
		                &m_sharedPara, nImageWidth);
	m_modCali.m_AI_height_in = xraylib_AIPara.AI_height_in;
	m_modCali.m_AI_nChannel = xraylib_AIPara.nChannel;
	m_modCali.m_nUseAi = xraylib_AIPara.nUseAI;
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Cali mod init err");

	/* 双能模块初始化 */
	hr = m_modDual.Init(m_szPublicFileFolderName, m_szZTableName, m_szZ6TableName,&m_sharedPara, 
		                xraylib_deviceInfo.xraylib_visualNo, xraylib_deviceInfo.xraylib_detectortype, xraylib_deviceInfo.xraylib_devicetype);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Dual mod init err");

	/* 检测模块初始化 */
	hr = m_modArea.Init(m_szPublicFileFolderName,&m_sharedPara);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Area mod init err");

	/* 图像处理模块初始化 */
	hr = m_modImgproc.Init(&m_sharedPara, *ability);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Imgproc mod init err");
	
	/* CNN处理模块初始化,这里修改为根据要求的输出条带高度和宽度进行AI模型的选择 */
	hr = m_modCnn.Init(m_szPublicFileFolderName, xraylib_AIPara, &m_sharedPara);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Cnn mod init err");

	/*测试体指标增强模块初始化默认给6个条带*/
	hr = m_modTcproc.Init(&m_sharedPara, 6, ability->nUseTc);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Tcproc mod init err");

	/* 注册参数键值 */
	RegistParaSetMap();
	RegistParaGetMap();
	RegistImageSetMap();
	RegistImageGetMap();

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::ReloadProfile(XRAY_DEVICEINFO* xraylib_deviceInfo)
{
	XRAY_LIB_HRESULT hr;

	if (xraylib_deviceInfo)
	{
		/* 机型数据有效，根据机型重新加载曲线 */
		/*****************************************
		*                路径初始化
		******************************************/
		memset(m_szCnnKeyName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
		memset(m_szCnnKeyNameSubType, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
		memset(m_szDeviceKeyName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
		memset(m_szZTableName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
		memset(m_szZ6TableName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
		memset(m_szGeoCaliName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
		memset(m_szFlatDetCaliName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));

		
		hr = GetKeyNameByDeviceInfo(*xraylib_deviceInfo, m_szCnnKeyName, m_szCnnKeyNameSubType, m_szDeviceKeyName);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		hr = InitDeviceParaByIni(m_szDevicePath, m_szDeviceKeyName);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		hr = m_modDual.ReInit(m_szPublicFileFolderName, m_szZTableName, m_szZ6TableName, xraylib_deviceInfo->xraylib_visualNo);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Dual Reinit err");
	}
	else
	{
		/* 机型数据无效，直接重新加载曲线，用于液体识别标定曲线重新载入 */
		hr = m_modDual.ReInit(m_szPublicFileFolderName, m_szZTableName, m_szZ6TableName, m_enVisual);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Dual Reinit err");
	}
	
	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT XRayImageProcess::GetRCurveName(XRAY_DEVICEINFO* xraylib_deviceInfo, 
	                                             XRAY_RCURVE_TYPE xraylib_rcurve_type, 
	                                             char* xraylib_rcurvename)
{
	XSP_CHECK(xraylib_rcurve_type >= XRAYLIB_CURVE_TYPE_MIN_VALUE && 
		      xraylib_rcurve_type <= XRAYLIB_CURVE_TYPE_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM, "curve type error, type in [%d]", xraylib_rcurve_type);

	XRAY_LIB_HRESULT hr;

	if (xraylib_deviceInfo)
	{
		/* 机型数据有效，根据机型重新加载曲线 */
		/*****************************************
		*                路径初始化
		******************************************/
		char szGetDeviceKeyName[XRAY_LIB_MAX_NAME];               // 外部获取R曲线设备配置ini键值
		char szGetZTableName[XRAY_LIB_MAX_NAME];                  // 外部获取R曲线文件名
		char szGetMixZTableName[XRAY_LIB_MAX_NAME];               // 外部获取混合R曲线文件名
		memset(szGetDeviceKeyName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
		memset(szGetZTableName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
		memset(szGetMixZTableName, 0, sizeof(char) * (XRAY_LIB_MAX_NAME));
		bool bGetMixCurveValid = false;

		hr = GetKeyNameByDeviceInfo(*xraylib_deviceInfo, szGetDeviceKeyName);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		hr = GetRCurvePathByIni(m_szDevicePath, szGetDeviceKeyName, szGetZTableName, szGetMixZTableName, bGetMixCurveValid);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
		

		if (XRAYLIB_CURVE_NORMAL == xraylib_rcurve_type)
		{
			// if (m_modDual.m_enCurveState == XRAYLIB_ON)
			// {
			// 	strcpy(xraylib_rcurvename, m_szAutoZTableName);
			// }
			// else
			// {
				strcpy(xraylib_rcurvename, szGetZTableName);
			// }
		}
		else if (XRAYLIB_CURVE_MIXTURE == xraylib_rcurve_type)
		{
			if (bGetMixCurveValid)
			{
				strcpy(xraylib_rcurvename, szGetMixZTableName);
			}
			else
			{
				return XRAY_LIB_MIX_CURVE_INVALID;
			}
		}
		else
		{
			log_error("curve type error");
			return XRAY_LIB_INVALID_PARAM;
		}
	}
	else
	{
		if (XRAYLIB_CURVE_NORMAL == xraylib_rcurve_type)
		{
			// if (m_modDual.m_enCurveState == XRAYLIB_ON)
			// {
			// 	strcpy(xraylib_rcurvename, m_szAutoZTableName);
			// }
			// else
			// {
				strcpy(xraylib_rcurvename, m_szZTableName);
			// }
		}
		else if (XRAYLIB_CURVE_MIXTURE == xraylib_rcurve_type)
		{
			if (m_bMixCurveValid)
			{
				strcpy(xraylib_rcurvename, m_szMixZTableName);
			}
			else
			{
				return XRAY_LIB_MIX_CURVE_INVALID;
			}
		}
		else
		{
			log_error("curve type error");
			return XRAY_LIB_INVALID_PARAM;
		}
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetTestModeParams(XRAYLIB_MATERIAL_COLOR_PARAMS *xraylib_testbparams)
{
	/*测试区域5比例赋值*/
	m_modImgproc.m_fTest5Ratio = xraylib_testbparams->fTest5RatioCN;
	m_modDual.m_stTestbParams.nGrayTuneNum = xraylib_testbparams->nGrayTuneNum;
	m_modDual.m_stTestbParams.nRTuneNum = xraylib_testbparams->nRTuneNum;

	for (int32_t i = 0; i < xraylib_testbparams->nGrayTuneNum; i++)
	{
		/*灰度值微调参数*/
		m_modDual.m_stTestbParams.stGrayValue[i].nParamGrayValueLL = 
			xraylib_testbparams->stGrayValue[i].nParamGrayValueLL;
		m_modDual.m_stTestbParams.stGrayValue[i].nParamGrayValueUL =
			xraylib_testbparams->stGrayValue[i].nParamGrayValueUL;
		m_modDual.m_stTestbParams.stGrayValue[i].nParamRValueLL =
			xraylib_testbparams->stGrayValue[i].nParamRValueLL;
		m_modDual.m_stTestbParams.stGrayValue[i].nParamRValueUL =
			xraylib_testbparams->stGrayValue[i].nParamRValueUL;
		m_modDual.m_stTestbParams.stGrayValue[i].nParamOffset =
			xraylib_testbparams->stGrayValue[i].nParamOffset;
	}
	
	for (int32_t i = 0; i < xraylib_testbparams->nRTuneNum; i++)
	{
		/*原子序数微调参数*/
		m_modDual.m_stTestbParams.stRValue[i].nParamGrayValueLL =
			xraylib_testbparams->stRValue[i].nParamGrayValueLL;
		m_modDual.m_stTestbParams.stRValue[i].nParamGrayValueUL =
			xraylib_testbparams->stRValue[i].nParamGrayValueUL;
		m_modDual.m_stTestbParams.stRValue[i].nParamRValueLL =
			xraylib_testbparams->stRValue[i].nParamRValueLL;
		m_modDual.m_stTestbParams.stRValue[i].nParamRValueUL =
			xraylib_testbparams->stRValue[i].nParamRValueUL;
		m_modDual.m_stTestbParams.stRValue[i].nParamOffset =
			xraylib_testbparams->stRValue[i].nParamOffset;
	}

	return XRAY_LIB_OK;
}

// 内存中的Ztable数值重写，微调参数，目前仅支持三色曲线的调整
XRAY_LIB_HRESULT XRayImageProcess::SetRCurveAdjustParams(XRAYLIB_RCURVE_ADJUST_PARAMS &xraylib_rcurveparams)
{
	XRAY_LIB_HRESULT hr;
	hr = m_modDual.AdjustZTable(xraylib_rcurveparams);
	// 复制出调整参数
	memcpy(&m_modDual.m_stTestbParams.stRCurveAdjust, &xraylib_rcurveparams, sizeof(XRAYLIB_RCURVE_ADJUST_PARAMS));
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::GenerateCaliTable(XRAYLIB_IMAGE& xData,
	                                                 XRAYLIB_UPDATE_ZEROANDFULL_MODE nUpdateMode)
{
	XRAY_LIB_HRESULT hr;
	XSP_CHECK(xData.width == static_cast<uint>(m_nDetectorWidth), XRAY_LIB_PARAM_WIDTH_ERROR);

	XMat high, low;
	hr = low.InitNoCheck(xData.height, xData.width, XSP_16UC1, (uint8_t*)xData.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	hr = high.InitNoCheck(xData.height, xData.width, XSP_16UC1, (uint8_t*)xData.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr = m_modCali.ManualUpdateCorrect(high, low, nUpdateMode);
	/* 这里由于手动校正数据为外部异步线程调用，需等待数据处理结束后再清除条带，否则会导致条带数据混乱 */ 
	while(m_AllowSyncOperation.load() == false)
	{
		std::this_thread::yield();
	}

	m_modCali.m_stLPadCache.bClearTemp = true; 
	m_modCali.m_stHPadCache.bClearTemp = true;
	m_modCali.m_stOriLPadCache.bClearTemp = true;
	m_modCali.m_stOriHPadCache.bClearTemp = true;
	m_modCali.m_stCaliLPadCache.bClearTemp = true;
	m_modCali.m_stCaliHPadCache.bClearTemp = true;
	m_modCali.m_stMaskCache.bClearTemp = true;
    this->procWtCache.bClearTemp = true;

	if (XRAYLIB_ENERGY_SCATTER == m_sharedPara.m_enEnergyMode)
	{
		log_info("Current Energy is BackScatter. UpdateMode is %s , Return.\n", nUpdateMode == XRAYLIB_UPDATEZERO ? "Zero" : "Full");
		if (XRAYLIB_UPDATEFULL == nUpdateMode)
		{
			int nHeight = low.hei;
			int nProcessHeight = m_modCnn.AiXsp_GetHeiIn();
			int nWidth = low.wid;
			int nHeightDownSize = nHeight / m_modCali.m_fPulsBS;

			hr = m_matNeighborBs0.Reshape(nHeightDownSize, nWidth, XSP_16UC1);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = m_modCali.CaliBSImageCPU(low, m_matNeighborBs0, m_modCali.m_fAntiBS1_AI);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = m_matNeighborBs1.Reshape(nHeightDownSize, nWidth, XSP_16UC1);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = m_modCali.GeoCali_BS_InterLinear(m_matNeighborBs0, m_matNeighborBs1, m_modCali.m_bGeoCaliBSInverse);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			memset(m_matNeighborBs0.Ptr(), 0, sizeof(uint16_t) * nHeightDownSize * nWidth);

			hr = m_matNeighborBs0.Reshape(nProcessHeight, nWidth, XSP_16UC1);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = m_matNeighborBs1.Reshape(nProcessHeight, nWidth, XSP_16UC1);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			Calilhz caliTbeIn, caliTbeOut;
			caliTbeIn.low = m_matNeighborBs1;
			caliTbeOut.low = m_matNeighborBs0;

			hr = m_modCnn.AiXsp_SyncProcess(caliTbeIn, caliTbeOut);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			uint16_t* ptrAirDenoise = reinterpret_cast<uint16_t*>(m_matNeighborBs0.Ptr());
			uint16_t* ptrAirSecCali_Ai = reinterpret_cast<uint16_t*>(m_modCali.m_matCaliTableAirBSSecCali_AI.Ptr());

			for (int nw = 0; nw < nWidth; nw++)
			{
				double f64Total= 0;
				int nNum = 0;
				for (int nh = 0; nh < nProcessHeight; nh++)
				{
					nNum++;
					f64Total += ptrAirDenoise[nh * nWidth + nw];
				}

				if (0 == nNum || 0 >= f64Total)
				{
					ptrAirSecCali_Ai[nw] = 0;
				}
				else
				{
					ptrAirSecCali_Ai[nw] = static_cast<uint16_t>(f64Total / static_cast<double>(nNum));
				}
			}
		}

		return hr;
	}

	// 计算低能噪声
	float32_t fLowNoise = 0.0f;
	hr = m_modCali.GetCorrrctMatNoise(low, nUpdateMode, XRAYLIB_ENERGY_LOW, &fLowNoise);
	if (hr == XRAY_LIB_OK)
	{
		if((m_fSigmaDn > FLT_EPSILON) && (m_fSigmaDn < 1.0f))
		{
			m_fSigmaDn = Clamp( fLowNoise / AI_XSP_MODEL_STD_DEV, 0.0f, 1.0f);
		}
		log_info("Manual correction chn-%d Get low noise: %f m_fSigmaDn = %f", m_sharedPara.m_enDeviceInfo.xraylib_visualNo, fLowNoise, m_fSigmaDn);
		hr = m_modCnn.AiXsp_SwitchModel( m_nResizeHeight - 2 * m_nResizePadLen, m_nResizeWidth, m_nRTHeight, m_nDetectorWidth);
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::UpdateCaliMem(XRAYLIB_IMAGE& xData,
	                                             XRAYLIB_UPDATE_ZEROANDFULL_MODE nUpdateMode)
{
	XRAY_LIB_HRESULT hr;
	XSP_CHECK(xData.width == static_cast<uint>(m_nDetectorWidth), XRAY_LIB_PARAM_WIDTH_ERROR);

	XMat high, low;
	hr = low.InitNoCheck(xData.height, xData.width, XSP_16UC1, (uint8_t*)xData.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	hr = high.InitNoCheck(xData.height, xData.width, XSP_16UC1, (uint8_t*)xData.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr = m_modCali.AutoUpdateCorrect(high, low, nUpdateMode);
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::GetCaliTable(XRAYLIB_CALI_TABLE *ptrCaliTable)
{
	XRAY_LIB_HRESULT hr;
	hr = m_modCali.GetCaliTable(ptrCaliTable);
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::SetLibXRayPrint(bool bPrint)
{
	if (bPrint)
	{
		xlog::log_set_level(LOG_TRACE);
	}
	else
	{
		xlog::log_set_level(LOG_ERROR);
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetMemTabSize(XRAY_LIB_MEM_TAB *memTab, XRAY_LIB_ABILITY *ability)
{
	/* XRayImageProcess类大小 */
	XSP_CHECK(m_vecModPtr.size() == XRAY_LIB_MTAB_NUM - 2, XRAY_LIB_MEM_ERROR);
	memTab[0].size = sizeof(XRayImageProcess);
	this->GetMemSize(memTab[1], *ability);
	for (int32_t i = 0; i < (int32_t)m_vecModPtr.size(); i++)
	{
		m_vecModPtr[i]->GetMemSize(memTab[i + 2], *ability);
	}
	
	return XRAY_LIB_OK;;
}

XRAY_LIB_HRESULT XRayImageProcess::PipelineProcess(VOID* pipe, int32_t index)
{
	XSP_CHECK(pipe, XRAY_LIB_NULLPTR, "pipeline is nullptr.");
	XRAY_LIB_HRESULT hr;

	if (XRAYLIB_PIPELINE_MODE3A == m_enPipelineMode)
    {
		hr = PipelineProcess3A(*(XRAYLIB_PIPELINE_PARAM_MODE3A*)pipe, index);
    }
	else if (XRAYLIB_PIPELINE_MODE3B == m_enPipelineMode && XRAYLIB_ENERGY_SCATTER == m_sharedPara.m_enEnergyMode)
	{
		/* 背散三级流水 */
		hr = PipelineProcessBS3B(*(XRAYLIB_PIPELINE_PARAM_MODE3B*)pipe, index);
	}
	else
	{
		hr = XRAY_LIB_INVALID_PARAM;
	}
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::CaliBSToRGBImage(XRAYLIB_IMAGE& calilhz, XRAYLIB_IMAGE& colorImg, int32_t ntop, int32_t nbotm)
{
	XRAY_LIB_HRESULT hr = XRAY_LIB_OK;

	XMat matLow, matHigh;

	hr = matLow.Init(calilhz.height, calilhz.width, XSP_16UC1, (uint8_t*)(static_cast<uint8_t*>(calilhz.pData[0]) + ntop * calilhz.width * sizeof(uint16_t)));
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr = matHigh.Init(calilhz.height, calilhz.width, XSP_16UC1, (uint8_t*)(static_cast<uint8_t*>(calilhz.pData[1]) + ntop * calilhz.width * sizeof(uint16_t)));
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	XMat &matGetColored = (XRAYLIB_BSIMAGE_NORMAL == m_enumBSImageMode) ? matLow : matHigh; 

    hr = GetColorImageBS(matGetColored, colorImg, calilhz.height - ntop - nbotm, colorImg.stride[0], calilhz.width);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	return XRAY_LIB_OK;
}


// XRAY_LIB_HRESULT BgGrayEliminate(XMat& matMerge, XMat& matProcWt)
// {
// 	XSP_CHECK(matMerge.wid == matProcWt.wid && matMerge.hei == matProcWt.hei, XRAY_LIB_INVALID_PARAM, "Failed.");
	
// 	#pragma omp parallel for schedule(static, 8)
// 	for (int i = 0; i < matMerge.hei; i++)
// 	{
// 		uint8_t* pWt = matProcWt.Ptr<uint8_t>(i);
// 		uint16_t* pMerge = matMerge.Ptr<uint16_t>(i);
// 		for (int j = 0; j < matMerge.wid; j++)
// 		{
// 			uint16_t bgVal = 0xFFFF;
// 			uint16_t oriVal = *pMerge;
// 			if (0xFF == *pWt)
// 			{
// 				*pMerge = bgVal;
// 			}
// 			else if (0 == *pWt)
// 			{
// 				*pMerge = oriVal;
// 			}
// 			else
// 			{
// 				double dRatio = *pWt / 255.0; 
// 				*pMerge = static_cast<uint16_t>(std::clamp(dRatio * 65535 + (1 - dRatio) * (*pMerge), 0.0, 65535.0));
// 			}
// 			pMerge++;
// 			pWt++;
			
// 		}
// 	}

// 	return XRAY_LIB_OK;
// }

int bCount = 0;
XRAY_LIB_HRESULT BgGrayEliminate(XMat& matMerge, XMat& matProcWt)
{
    XSP_CHECK(matMerge.wid == matProcWt.wid && matMerge.hei == matProcWt.hei, 
              XRAY_LIB_INVALID_PARAM, "Failed.");
    
    // 使用线程局部变量避免数据竞争
    struct ThreadLocal {
        int maxWid = 0;
        int minWid = std::numeric_limits<int>::max();
        int maxHei = 0;
        int minHei = std::numeric_limits<int>::max();
        int bgFullWidthCount = 0; // 记录全行为背景的行数
    };
    
    std::vector<ThreadLocal> threadLocals(omp_get_max_threads());
    int globalMinWid = std::numeric_limits<int>::max();
    int globalMaxWid = 0;
    int globalMinHei = std::numeric_limits<int>::max();
    int globalMaxHei = 0;

    #pragma omp parallel for schedule(static, 8)
    for (int i = 0; i < matMerge.hei; i++)
    {
        int threadId = omp_get_thread_num();
        ThreadLocal& local = threadLocals[threadId];
        
        int sBgWidCount = 0;
        uint8_t* pWt = matProcWt.Ptr<uint8_t>(i);
        uint16_t* pMerge = matMerge.Ptr<uint16_t>(i);
        
        for (int j = 0; j < matMerge.wid; j++)
        {
            uint16_t bgVal = 0xFFFF;
            uint16_t oriVal = *pMerge;
            
            if (*pWt == 0xFF)  // 背景像素
            {
                *pMerge = bgVal;
                sBgWidCount++;
                
                // 更新线程局部的最值
                local.maxWid = std::max(local.maxWid, j);
                local.minWid = std::min(local.minWid, j);
            }
            else if (*pWt == 0)  // 前景像素
            {
                *pMerge = oriVal;
            }
            else  // 混合像素
            {
                double dRatio = *pWt / 255.0; 
                // 优化计算：避免重复计算和类型转换
                double result = dRatio * 65535.0 + (1.0 - dRatio) * oriVal;
                *pMerge = static_cast<uint16_t>(std::clamp(result, 0.0, 65535.0));
            }
            pMerge++;
            pWt++;
        }
        
        // 修正逻辑：判断整行是否都是背景
        if (sBgWidCount == matMerge.wid)  // 修正：应该是 == matMerge.wid
        {
            local.bgFullWidthCount++;
            local.maxHei = std::max(local.maxHei, i);
            local.minHei = std::min(local.minHei, i);
        }
    }

    // 归并各线程的结果
    for (const auto& local : threadLocals) 
    {
        if (local.minWid != std::numeric_limits<int>::max()) {
            globalMinWid = std::min(globalMinWid, local.minWid);
        }
        if (local.maxWid > 0) {
            globalMaxWid = std::max(globalMaxWid, local.maxWid);
        }
        if (local.minHei != std::numeric_limits<int>::max()) {
            globalMinHei = std::min(globalMinHei, local.minHei);
        }
        if (local.maxHei > 0) {
            globalMaxHei = std::max(globalMaxHei, local.maxHei);
        }
    }

    // 处理边界情况：如果没有背景像素
    if (globalMinWid == std::numeric_limits<int>::max()) {
        globalMinWid = 0;
    }
    if (globalMaxWid == 0) {
        globalMaxWid = matMerge.wid - 1;
    }
    if (globalMinHei == std::numeric_limits<int>::max()) {
        globalMinHei = 0;
    }
    if (globalMaxHei == 0) {
        globalMaxHei = matMerge.hei - 1;
    }

    // printf("\nNo. %d Current Region: wid %d, hei %d\n", bCount, matMerge.wid, matMerge.hei);
    // printf("No. %d ROI Wid From %d to %d, Hei From %d to %d\n", 
    //        bCount++, globalMinWid, globalMaxWid, globalMinHei, globalMaxHei);
    // printf("Background full-width rows: %d\n", 
    //        std::accumulate(threadLocals.begin(), threadLocals.end(), 0, 
    //                      [](int sum, const ThreadLocal& tl) { return sum + tl.bgFullWidthCount; }));

    return XRAY_LIB_OK;
}



int gCount = 0;
XRAY_LIB_HRESULT XRayImageProcess::CalilhzMergeToColor(XRAYLIB_IMAGE& calilhz,
	                                                   XRAYLIB_IMAGE& merge, // channel为HIGHLOW时，输出参数；为MERGE时，输入参数
	                                                   XRAYLIB_IMAGE& colorImg,
	                                                   XRAYLIB_DUMPPROC_TIP_PARAM& p_xraylib_tip,
	                                                   XRAYLIB_IMG_CHANNEL channel,
	                                                   int32_t ntop, int32_t nbotm, 
                                                       int32_t bDescendOrder)
{
	XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
	m_modImgproc.timeCostItemAdd("AlgIn", calilhz.width, calilhz.height);

	if (XRAYLIB_ENERGY_SCATTER == m_sharedPara.m_enEnergyMode) // 背散
	{
		// NLMeans与AIXSP降噪数据合成增强数据
		if (XRAYLIB_IMG_CHANNEL_HIGHLOW == channel)
		{
			hr = DualDenoiseToEnhance(calilhz, merge, ntop, nbotm, bDescendOrder);
		}
		// 不走高低能通道时, 直接使用merge数据
        XMat matGray;
		hr = matGray.Init(merge.height, merge.width, ntop, nbotm, XSP_16UC1, (uint8_t*)merge.pData[0]);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		hr = GetColorImageBS(matGray, colorImg, merge.height - ntop - nbotm, colorImg.stride[0], merge.width);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
	}
    else // 透射
	{
		auto begin_time = std::chrono::high_resolution_clock::now();
        XSP_CHECK(calilhz.format == XRAYLIB_IMG_RAW_LHZ8 || calilhz.format == XRAYLIB_IMG_RAW_L, XRAY_LIB_XMAT_TYPE_ERR);

        XMat matLow, matHigh, matMask, matZ, matGray;
		auto get_color_0 = std::chrono::high_resolution_clock::now();

		// 整图原子序数Z(不含包裹Mask)
        hr = m_matEntireZValue.Reshape(calilhz.height, calilhz.width, ntop, nbotm, XSP_8UC1);
        XSP_CHECK(XRAY_LIB_OK == hr, hr);
		m_matEntireZValue.SetValue(0xFF);

		// 权重图 (From包裹Mask平滑)
        hr = m_matEntireProcWt.Reshape(calilhz.height, calilhz.width, ntop, nbotm, XSP_8UC1);
        XSP_CHECK(XRAY_LIB_OK == hr, hr);
		m_matEntireProcWt.SetValue(0xFF);

		// 包裹Mask(二值图)
		hr = m_matEntireMask.Reshape(calilhz.height, calilhz.width, ntop, nbotm, XSP_8UC1);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
		m_matEntireMask.SetValue(0xFF);

		hr = matLow.Init(calilhz.height, calilhz.width, ntop, nbotm, XSP_16UC1, (uint8_t*)calilhz.pData[0]);
        XSP_CHECK(XRAY_LIB_OK == hr, hr);

		hr = matHigh.Init(calilhz.height, calilhz.width, ntop, nbotm, XSP_16UC1, (uint8_t*)calilhz.pData[1]);
		XSP_CHECK((XRAY_LIB_OK == hr) && MatSizeEq(matLow, matHigh), XRAY_LIB_INVALID_PARAM);

		hr = matZ.Init(calilhz.height, calilhz.width, ntop, nbotm, XSP_8UC1, (uint8_t*)calilhz.pData[2]);
		XSP_CHECK((XRAY_LIB_OK == hr) && MatSizeEq(matLow, matZ), XRAY_LIB_INVALID_PARAM);

		hr = matGray.Init(calilhz.height, calilhz.width, ntop, nbotm, XSP_16UC1, (uint8_t*)merge.pData[0]);
		XSP_CHECK(XRAY_LIB_OK == hr && MatSizeEq(matLow, matGray), XRAY_LIB_INVALID_PARAM);

		#pragma omp parallel for schedule (static, 8)
		for (int32_t y = 0; y < m_matEntireZValue.hei; ++y)
		{
			uint8_t *pSrc = matZ.Ptr(y);
			uint8_t *pDst = m_matEntireZValue.Ptr(y);
			uint8_t *pMask = m_matEntireMask.Ptr(y);
			for (int32_t x = 0; x < m_matEntireZValue.wid; ++x, ++pSrc, ++pDst, ++pMask)
			{
				*pDst = *pSrc & 0x7F; // 第0~6Bit为原子序数Z	拆出来正常上色的原子序数图
				*pMask = *pSrc & 0x80 ? 0xFF : 0;	// 拆出来Mask数据 背景为0xFF, 前景为0
			}
		}

        /// 插入TIP
        hr = DumpProcTipImage(matLow, matHigh, m_matEntireZValue, m_matEntireMask, p_xraylib_tip);
        XSP_CHECK(XRAY_LIB_OK == hr, hr);

		// 将TIP的Z值拷贝回输出
		#pragma omp parallel for schedule (static, 8)
		for (int32_t y = 0; y < m_matEntireZValue.hei; ++y)
		{
			uint8_t *pSrc = m_matEntireZValue.Ptr(y);
			uint8_t *pDst = matZ.Ptr(y);
			uint8_t *pMask = m_matEntireMask.Ptr(y);
			for (int32_t x = 0; x < m_matEntireZValue.wid; ++x, ++pSrc, ++pDst, ++pMask)
			{
				*pDst = (*pSrc | (*pMask & 0x80)); // 将原子序数和Mask合并返回
			}
		}

		isl::Imat<uint8_t> imgMask(m_matEntireMask);
        isl::Imat<uint8_t> imgWt(imgMask, (uint8_t*)m_matEntireProcWt.Ptr(), m_matEntireProcWt.size);

		isl::Imat<uint8_t> fTmp(imgMask, (uint8_t*)m_matEntireEnhance.Ptr(), m_matEntireEnhance.size);
		auto gauss_cost_0 = std::chrono::high_resolution_clock::now();

		auto duration_init = std::chrono::duration_cast<std::chrono::microseconds>(gauss_cost_0 - begin_time);
		printf("Step %d: wid %d, hei %d init cost: %ld us (%.3f ms)\n", gCount, matGray.wid, matGray.hei,
					duration_init.count(), duration_init.count() / 1000.0);

		perf_init();
		
		uint8_t u8fgVal = 0xFF;
		std::function<bool(const uint8_t*, int32_t)>skipFgValFunc = [u8fgVal](const uint8_t* u8Pixel, int32_t count) -> bool{
			for (int i = 0; i < count; ++i)
			{
				if (u8fgVal == u8Pixel[i])
				{
					return true;
				}
			}
			return false;
		};
		perf_start();
		isl::Filter::Gaussian(imgWt, imgMask, fTmp, 2, -1.0, skipFgValFunc);
		// isl::Filter::Gaussian(imgWt, imgMask, fTmp, 2, -1.0);
		perf_stop();
		m_matEntireEnhance.SetValue(0xFF);

        std::vector<XRAYLIB_RECT> procRect;	// 无用 后面高低删了他

		auto enhance_cost = std::chrono::high_resolution_clock::now();

		auto duration_gauss = std::chrono::duration_cast<std::chrono::microseconds>(enhance_cost - gauss_cost_0);
		printf("Step %d: wid %d, hei %d gauss_filter cost: %ld us (%.3f ms)\n", gCount, matGray.wid, matGray.hei,
					duration_gauss.count(), duration_gauss.count() / 1000.0);

		int32_t bMergeRegenerated = (XRAYLIB_IMG_CHANNEL_HIGHLOW == channel || XRAYLIB_DEFAULTENHANCE_CLOSE == m_enDefaultEnhanceJudge) ? true : false;
		
		if (bMergeRegenerated)
		{
			hr = Cali2GrayEnhance(matGray, matLow, &matHigh, m_matEntireProcWt, (bDescendOrder ? 2 : 1));
            XSP_CHECK(XRAY_LIB_OK == hr, hr);

            /// 输出Merge图像
            merge.format = XRAYLIB_IMG_RAW_MERGE;
            merge.height = calilhz.height;
            merge.width = calilhz.width;
        
			// 如果默认增强关闭, 使用中间输出的融合灰度图
			if (XRAYLIB_DEFAULTENHANCE_CLOSE == m_enDefaultEnhanceJudge)
			{
				matGray = m_matEntireMerge;
			}
		}

        /******************** 特殊增强 ********************/
		// 海外版本自动局增功能, 满足条件默认开启局增
		if ((XRAYLIB_TESTMODE_EUR == m_modImgproc.m_enTestMode || XRAYLIB_TESTMODE_USA == m_modImgproc.m_enTestMode) && 
            (XRAYLIB_ON == m_modImgproc.m_enTestAutoLE) && (m_modImgproc.m_enEnhanceMode == XRAYLIB_ENHANCE_NORAML))
        {
            m_modImgproc.m_enEnhanceMode = XRAYLIB_SPECIAL_LOCALENHANCE;
        }

        if (m_modImgproc.m_enEnhanceMode != XRAYLIB_ENHANCE_NORAML)
        {
            matGray = GraySpecialEnhance(matLow, &matHigh, matGray);
        }

		auto bg_eliminate0 = std::chrono::high_resolution_clock::now();
		auto duration_enhance = std::chrono::duration_cast<std::chrono::microseconds>(bg_eliminate0 - enhance_cost);
		printf("Step %d: wid %d, hei %d enhance cost: %ld us (%.3f ms)\n", gCount, matGray.wid, matGray.hei,
					duration_enhance.count(), duration_enhance.count() / 1000.0);
		
		/******************** 背景剔除 ********************/
		hr = BgGrayEliminate(matGray, m_matEntireProcWt);
		auto bg_eliminate1 = std::chrono::high_resolution_clock::now();
		auto duration_bg_eliminate0 = std::chrono::duration_cast<std::chrono::microseconds>(bg_eliminate1 - bg_eliminate0);
		printf("Step %d: wid %d, hei %d BgGrayEliminate cost: %ld us (%.3f ms)\n", gCount, matGray.wid, matGray.hei,
					duration_bg_eliminate0.count(), duration_bg_eliminate0.count() / 1000.0);

        /******************** 上色 ********************/
		
        hr = GetColorImage(colorImg, matGray, m_matEntireZValue, m_matEntireProcWt, procRect, colorImg.stride[0], calilhz.width);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
		auto get_color_1 = std::chrono::high_resolution_clock::now();

		auto duration_get_color_0 = std::chrono::duration_cast<std::chrono::microseconds>(get_color_1 - bg_eliminate1);
		printf("Step %d: wid %d, hei %d GetColorImageRgb cost: %ld us (%.3f ms)\n", gCount, m_matEntireColorTemp.wid, m_matEntireColorTemp.hei,
					duration_get_color_0.count(), duration_get_color_0.count() / 1000.0);

		auto duration_total = std::chrono::duration_cast<std::chrono::microseconds>(get_color_1 - begin_time);
		printf("Step %d: wid %d, hei %d GetColorImageRgb cost: %ld us (%.3f ms)\n", gCount++, m_matEntireColorTemp.wid, m_matEntireColorTemp.hei,
					duration_total.count(), duration_total.count() / 1000.0);
	}

    m_modImgproc.timeCostItemUpdate("AlgOut", 11);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetProcessMerge(XRAYLIB_IMAGE& calilh, XRAYLIB_IMAGE& merge, int32_t ntop, int32_t nbotm, int32_t bDescendOrder)
{
	XRAY_LIB_HRESULT hr = XRAY_LIB_OK;

	XSP_CHECK(calilh.format == XRAYLIB_IMG_RAW_LHZ8 || calilh.format == XRAYLIB_IMG_RAW_L, XRAY_LIB_XMAT_TYPE_ERR);

	m_modImgproc.timeCostItemAdd("AlgIn", calilh.width, calilh.height);

	XMat matLow, matHigh, matMask;

	hr = matLow.Init(calilh.height, calilh.width, ntop, nbotm, XSP_16UC1, (uint8_t*)calilh.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr = m_matEntireZValue.Reshape(calilh.height, calilh.width, ntop, nbotm, XSP_8UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr = m_matEntireProcWt.Reshape(calilh.height, calilh.width, ntop, nbotm, XSP_8UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	if (XRAYLIB_ENERGY_DUAL == m_sharedPara.m_enEnergyMode && calilh.format == XRAYLIB_IMG_RAW_LHZ8) // 双能
	{
		XMat matZ;
		hr = matHigh.Init(calilh.height, calilh.width, ntop, nbotm, XSP_16UC1, (uint8_t*)calilh.pData[1]);
		XSP_CHECK((XRAY_LIB_OK == hr) && MatSizeEq(matLow, matHigh), XRAY_LIB_INVALID_PARAM);

		hr = matZ.Init(calilh.height, calilh.width, ntop, nbotm, XSP_8UC1, (uint8_t*)calilh.pData[2]);
		XSP_CHECK((XRAY_LIB_OK == hr) && MatSizeEq(matLow, matZ), XRAY_LIB_INVALID_PARAM);

		matMask.Init(calilh.height, calilh.width, XSP_8UC1, (uint8_t*)calilh.pData[2]);

		for (int32_t y = 0; y < m_matEntireZValue.hei; ++y)
		{
			uint8_t *pSrc = matZ.Ptr(y);
			uint8_t *pDst = m_matEntireZValue.Ptr(y);
			for (int32_t x = 0; x < m_matEntireZValue.wid; ++x, ++pSrc, ++pDst)
			{
				*pDst = *pSrc & 0x7F; // 第0~6Bit为原子序数Z
			}
		}
	}
	else // 单能，生成虚拟Z值表与matMask
	{

	}

	isl::Imat<uint8_t> imgMask(matMask);
	isl::Imat<uint8_t> imgWt(imgMask, (uint8_t*)m_matEntireProcWt.Ptr(), m_matEntireProcWt.size);
	const int32_t smoothRadius = 3;
	std::vector<isl::Rect<int32_t>>&& procArea = isl::Fbg::SmoothWeight(imgWt, imgMask, smoothRadius);
	std::vector<XRAYLIB_RECT> procRect;
	std::transform(procArea.begin(), procArea.end(), std::back_inserter(procRect),
					[](const isl::Rect<int32_t>& r) -> XRAYLIB_RECT {
						return {static_cast<unsigned int>(r.x),
								static_cast<unsigned int>(r.y),
								static_cast<unsigned int>(r.width),
								static_cast<unsigned int>(r.height)};
					});

	hr = m_matEntireEnhance.Reshape(calilh.height, calilh.width, ntop, nbotm, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, XRAY_LIB_INVALID_PARAM, "XMat Reshape Error.");

	hr = Cali2GrayEnhance(m_matEntireEnhance, matLow, &matHigh, m_matEntireProcWt, (bDescendOrder ? 2 : 1));
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	memcpy(merge.pData[0], m_matEntireEnhance.PadPtr(),calilh.width * (calilh.height - ntop - nbotm) * XSP_ELEM_SIZE(XSP_16UC1));

	/// 输出Merge图像
	merge.format = XRAYLIB_IMG_RAW_MERGE;
	merge.height = calilh.height - ntop - nbotm;
	merge.width = calilh.width;

	m_modImgproc.timeCostItemUpdate("AlgOut", 7);

	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::GetConcernedArea(XRAYLIB_IMAGE&         xraylib_img,
													XRAYLIB_CONCERED_AREA* ConcerdMaterialM,
													XRAYLIB_CONCERED_AREA* LowPeneArea,
													XRAYLIB_CONCERED_AREA* ExplosiveArea,
													XRAYLIB_CONCERED_AREA* DrugArea)
{
	XRAY_LIB_HRESULT hr;
	hr = m_modArea.GetConcernedArea(xraylib_img, ConcerdMaterialM, LowPeneArea, ExplosiveArea, DrugArea);
	if (m_modCali.m_enGeoMertric == XRAYLIB_ON)
	{
		hr = m_modArea.GeoConcernedArea(m_modCali.m_matGeoDetectionPixelOffsetLut, ConcerdMaterialM, LowPeneArea, ExplosiveArea, DrugArea);
	}

	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::GetConcernedAreaPiecewise(XRAYLIB_IMAGE&         xraylib_img,
											                 XRAYLIB_CONCERED_AREA *ConcerdMaterialM, 
											                 XRAYLIB_CONCERED_AREA *LowPeneArea,
											                 XRAYLIB_CONCERED_AREA *ExplosiveArea,
											                 XRAYLIB_CONCERED_AREA *DrugArea,
		                                                     int32_t                    nDetecFlag)
{
	XRAY_LIB_HRESULT hr;
	hr = m_modArea.GetConcernedAreaPiecewise(xraylib_img, ConcerdMaterialM, LowPeneArea, ExplosiveArea, DrugArea, nDetecFlag);
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::GetGeoOffsetLut(uint16_t* p_geooffset_lut, int32_t p_geooffset_lut_width)
{
	XSP_CHECK(p_geooffset_lut, XRAY_LIB_NULLPTR);
	if(p_geooffset_lut_width == m_modCali.m_matGeoDetectionPixelOffsetLut.wid)
	{
		memcpy(p_geooffset_lut, m_modCali.m_matGeoDetectionPixelOffsetLut.Ptr(), 
			m_modCali.m_matGeoDetectionPixelOffsetLut.Size());
	}
	else
	{
		return XRAY_LIB_INVALID_WIDTH;
	}

	return XRAY_LIB_OK;

}

XRAY_LIB_HRESULT XRayImageProcess::SetImage(XRAYLIB_CONFIG_IMAGE* stConfigImage)
{
	XSP_CHECK( stConfigImage, XRAY_LIB_NULLPTR );
	XSP_CHECK( stConfigImage->key >= XRAYLIB_CONFIG_IMAGE_KEY_MIN_VALUE &&
		       stConfigImage->key <= XRAYLIB_CONFIG_IMAGE_KEY_MAX_VALUE,
		       XRAY_LIB_INVALID_PARAM );

	XRAYLIB_CONFIG_IMAGE_KEY enKey = XRAYLIB_CONFIG_IMAGE_KEY(stConfigImage->key);

	if (m_mapImageSet.count(enKey) <= 0)
	{
		log_error("Invalid key : %X", enKey);
		return XRAY_LIB_INVALID_PARAM;
	}
	ImageFunc pFunc = m_mapImageSet[enKey];
	XSP_CHECK( 0 != pFunc, XRAY_LIB_INVALID_PARAM);

	XRAY_LIB_HRESULT hr;
	hr = (this->*pFunc)(stConfigImage);
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::GetImage(XRAYLIB_CONFIG_IMAGE* stConfigImage)
{
	XSP_CHECK(stConfigImage, XRAY_LIB_NULLPTR);
	XSP_CHECK(stConfigImage->key >= XRAYLIB_CONFIG_IMAGE_KEY_MIN_VALUE &&
		      stConfigImage->key <= XRAYLIB_CONFIG_IMAGE_KEY_MAX_VALUE,
		      XRAY_LIB_INVALID_PARAM);

	XRAYLIB_CONFIG_IMAGE_KEY enKey = XRAYLIB_CONFIG_IMAGE_KEY(stConfigImage->key);

	if (m_mapImageGet.count(enKey) <= 0)
	{
		log_error("Invalid key : %X", enKey);
		return XRAY_LIB_INVALID_PARAM;
	}
	ImageFunc pFunc = m_mapImageGet[enKey];
	XSP_CHECK(0 != pFunc, XRAY_LIB_INVALID_PARAM);

	XRAY_LIB_HRESULT hr;
	hr = (this->*pFunc)(stConfigImage);
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::SetPara(XRAYLIB_CONFIG_PARAM* stConfigPara)
{
	XSP_CHECK(stConfigPara, XRAY_LIB_NULLPTR);

	XRAYLIB_CONFIG_PARAM_KEY enKey = XRAYLIB_CONFIG_PARAM_KEY(stConfigPara->key);

	if (m_mapParaSet.count(enKey) <= 0)
	{
		log_error("Invalid key : %X", enKey);
		return XRAY_LIB_INVALID_PARAM;
	}
	ParaFunc pFunc = m_mapParaSet[enKey];
	XSP_CHECK(0 != pFunc, XRAY_LIB_INVALID_PARAM); 
	XRAY_LIB_HRESULT hr;
	hr = (this->*pFunc)(stConfigPara);
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::GetPara(XRAYLIB_CONFIG_PARAM * stConfigPara)
{
	XSP_CHECK(stConfigPara, XRAY_LIB_NULLPTR);

	XRAYLIB_CONFIG_PARAM_KEY enKey = XRAYLIB_CONFIG_PARAM_KEY(stConfigPara->key);

	if (m_mapParaGet.count(enKey) <= 0)
	{
		log_error("Invalid key : %X", enKey);
		return XRAY_LIB_INVALID_PARAM;
	}
	ParaFunc pFunc = m_mapParaGet[enKey];
	XSP_CHECK(0 != pFunc, XRAY_LIB_INVALID_PARAM);
	XRAY_LIB_HRESULT hr;
	hr = (this->*pFunc)(stConfigPara);
	return hr;
}

void XRayImageProcess::Release()
{
	for (int32_t i = 0; i < (int32_t)m_vecModPtr.size(); i++)
	{
		m_vecModPtr[i]->Release();
	}

	return;
}

XRAY_LIB_HRESULT XRayImageProcess::GetMemSize(XRAY_LIB_MEM_TAB& MemTab, XRAY_LIB_ABILITY &ability)
{
	MemTab.size = 0;

	const int32_t processWidth = int32_t(ability.nDetectorWidth * ability.fResizeScale);
	const int32_t processHeightRt = int32_t((ability.nMaxHeightRealTime + 2 * XRAY_LIB_MAX_FILTER_KERNEL_LENGTH) * ability.fResizeScale);
	const int32_t processHeightEntrie = ability.nMaxHeightEntire + XRAY_LIB_MAX_FILTER_KERNEL_LENGTH * 2;

	// 背散射
	if (XRAYLIB_ENERGY_SCATTER == ability.nEnergyMode)
	{
		m_matBSTmp.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matBSTmp.data, m_matBSTmp.Size(), MemTab);

		m_matNLMeansTmp.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matNLMeansTmp.data, m_matNLMeansTmp.Size(), MemTab);

		m_matAiXspTmp.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matAiXspTmp.data, m_matAiXspTmp.Size(), MemTab);

		m_matNLMeansOut.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matNLMeansOut.data, m_matNLMeansOut.Size(), MemTab);

		m_matAiXspOut.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matAiXspOut.data, m_matAiXspOut.Size(), MemTab);

		m_matNLMeansResizeOut.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matNLMeansResizeOut.data, m_matNLMeansResizeOut.Size(), MemTab);

		m_matAiXspResizeOut.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matAiXspResizeOut.data, m_matAiXspResizeOut.Size(), MemTab);

		m_matNeighborBs0.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matNeighborBs0.data, m_matNeighborBs0.Size(), MemTab);

		m_matNeighborBs1.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matNeighborBs1.data, m_matNeighborBs1.Size(), MemTab);

		// NLMeans使用内存
		m_matV1.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_matV1.data, m_matV1.Size(), MemTab);

		m_matV2.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_matV2.data, m_matV2.Size(), MemTab);

		m_matV2v.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_matV2v.data, m_matV2v.Size(), MemTab);

		m_matSd.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_32U));
		XspMalloc((void**)&m_matSd.data, m_matSd.Size(), MemTab);

		m_matfAvg.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_32F));
		XspMalloc((void**)&m_matfAvg.data, m_matfAvg.Size(), MemTab);

		m_matfWeight.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_32F));
		XspMalloc((void**)&m_matfWeight.data, m_matfWeight.Size(), MemTab);

		m_matfWMax.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_32F));
		XspMalloc((void**)&m_matfWMax.data, m_matfWMax.Size(), MemTab);
	}
	else // 透射
	{
		/*注意测试体增强需要较大的内存缓存，所以扩边逻辑的内存统一按照这个大小申请，后续图像操作的内存大小按照 processHeightRt 申请*/
		const int32_t nTcEnhanceRTProcessHeight = int32_t(ability.nMaxHeightRealTime + 2 * XRAY_LIB_MAX_TCPROCESS_LENGTH);
		const int32_t nRTProcessHeightOri = int32_t(ability.nMaxHeightRealTime + 2 * XRAY_LIB_MAX_FILTER_KERNEL_LENGTH);
		const int32_t nRTProcessWidthOri = int32_t(ability.nDetectorWidth);
		const int32_t nRTProcessPixelNum = nRTProcessHeightOri * nRTProcessWidthOri;


		m_matHTemp.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * nTcEnhanceRTProcessHeight * XRAY_LIB_MAX_RESIZE_FACTOR * nRTProcessWidthOri * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matHTemp.data, m_matHTemp.Size(), MemTab);

		m_matLTemp.SetMem( XRAY_LIB_MAX_RESIZE_FACTOR * nTcEnhanceRTProcessHeight * XRAY_LIB_MAX_RESIZE_FACTOR * nRTProcessWidthOri * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matLTemp.data, m_matLTemp.Size(), MemTab);

		m_matOriHTemp.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * nTcEnhanceRTProcessHeight * XRAY_LIB_MAX_RESIZE_FACTOR * nRTProcessWidthOri * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matOriHTemp.data, m_matOriHTemp.Size(), MemTab);

		m_matOriLTemp.SetMem( XRAY_LIB_MAX_RESIZE_FACTOR * XRAY_LIB_MAX_RESIZE_FACTOR * nRTProcessPixelNum * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matOriLTemp.data, m_matOriLTemp.Size(), MemTab);

		m_matResizeLow.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * XRAY_LIB_MAX_RESIZE_FACTOR * nRTProcessPixelNum * XSP_ELEM_SIZE(XSP_32FC1));
		XspMalloc((void**)&m_matResizeLow.data, m_matResizeLow.Size(), MemTab);
		
		m_matResizeHigh.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * XRAY_LIB_MAX_RESIZE_FACTOR * nRTProcessPixelNum * XSP_ELEM_SIZE(XSP_32FC1));
		XspMalloc((void**)&m_matResizeHigh.data, m_matResizeHigh.Size(), MemTab);

		m_matResizeZValue.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * XRAY_LIB_MAX_RESIZE_FACTOR * nRTProcessPixelNum * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_matResizeZValue.data, m_matResizeZValue.Size(), MemTab);

		m_mCnnLanczosTemp.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * XRAY_LIB_MAX_RESIZE_FACTOR * nRTProcessPixelNum * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_mCnnLanczosTemp.data, m_mCnnLanczosTemp.Size(), MemTab);

		m_mPreLanczosTemp.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * XRAY_LIB_MAX_RESIZE_FACTOR * nRTProcessPixelNum * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_mPreLanczosTemp.data, m_mPreLanczosTemp.Size(), MemTab);

		m_mPostLanczosTemp.SetMem(XRAY_LIB_MAX_RESIZE_FACTOR * XRAY_LIB_MAX_RESIZE_FACTOR * nRTProcessPixelNum * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_mPostLanczosTemp.data, m_mPostLanczosTemp.Size(), MemTab);

		m_matMerge.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matMerge.data, m_matMerge.Size(), MemTab);

		m_matProcWt.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_matProcWt.data, m_matProcWt.Size(), MemTab);

		m_matSpecialEnhance.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matSpecialEnhance.data, m_matSpecialEnhance.Size(), MemTab);

		procWtCache.mRingBuf.SetMem(processHeightRt * 8 * processWidth * XSP_ELEM_SIZE(XSP_8U)); // RingBuf设置为8倍最大条带内存
		XspMalloc((void**)&procWtCache.mRingBuf.data, procWtCache.mRingBuf.Size(), MemTab);

		m_matResizeHighPad.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matResizeHighPad.data, m_matResizeHighPad.Size(), MemTab);

		m_matResizeLowPad.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matResizeLowPad.data, m_matResizeLowPad.Size(), MemTab);

		m_matResizeMergePad.SetMem(processHeightRt * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matResizeMergePad.data, m_matResizeMergePad.Size(), MemTab);

		m_matOriGeoHigh.SetMem(nRTProcessHeightOri * nRTProcessWidthOri * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matOriGeoHigh.data, m_matOriGeoHigh.Size(), MemTab);
	
		m_matOriGeomLow.SetMem(nRTProcessHeightOri * nRTProcessWidthOri * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matOriGeomLow.data, m_matOriGeomLow.Size(), MemTab);

		/***************************
		*         整图内存
		****************************/
		m_matEntireZValue.SetMem(processHeightEntrie * processWidth * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_matEntireZValue.data, m_matEntireZValue.Size(), MemTab);

		m_matEntireMerge.SetMem(processHeightEntrie * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matEntireMerge.data, m_matEntireMerge.Size(), MemTab);

		m_matEntireEnhance.SetMem(processHeightEntrie * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matEntireEnhance.data, m_matEntireEnhance.Size(), MemTab);

		m_matEntireProcWt.SetMem(processHeightEntrie * processWidth * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_matEntireProcWt.data, m_matEntireProcWt.Size(), MemTab);

		m_matEntireMask.SetMem(processHeightEntrie * processWidth * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_matEntireMask.data, m_matEntireMask.Size(), MemTab);

		/*************************** 
		*          TIP内存 
		****************************/
		int32_t nTipHei = XRAY_LIB_MAX_TIP_IMAGE_HEIGHT;
		int32_t nTipWid = XRAY_LIB_MAX_TIP_IMAGE_WIDTH;

		m_matTipHigh.SetMem(nTipHei * nTipWid * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matTipHigh.data, m_matTipHigh.Size(), MemTab);

		m_matTipLow.SetMem(nTipHei * nTipWid * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matTipLow.data, m_matTipLow.Size(), MemTab);

		m_matTipZValue.SetMem(nTipHei * nTipWid * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_matTipZValue.data, m_matTipZValue.Size(), MemTab);

		int32_t nTipTmpHei = int32_t(ability.nDetectorWidth * ability.fResizeScale + XRAY_LIB_MAX_FILTER_KERNEL_LENGTH * 2);
		int32_t nTipTmpWid = int32_t(XRAY_LIB_MAX_TIP_IMAGE_WIDTH * ability.fResizeScale);

		m_matTipedHigh.SetMem(nTipTmpHei * nTipTmpWid * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matTipedHigh.data, m_matTipedHigh.Size(), MemTab);

		m_matTipedLow.SetMem(nTipTmpHei * nTipTmpWid * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matTipedLow.data, m_matTipedLow.Size(), MemTab);

		m_matTipedZValue.SetMem(nTipTmpHei * nTipTmpWid * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_matTipedZValue.data, m_matTipedZValue.Size(), MemTab);

		m_matTipedMerge.SetMem(nTipTmpHei * nTipTmpWid * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matTipedMerge.data, m_matTipedMerge.Size(), MemTab);
	}

	/*************************** 
	*          通用内存 
	****************************/
	m_matEntireColorTemp.SetMem(processHeightEntrie * processWidth * XSP_ELEM_SIZE(XSP_8UC4));
	XspMalloc((void**)&m_matEntireColorTemp.data, m_matEntireColorTemp.Size(), MemTab);

	m_matDefaultEnhance.SetMem(processHeightEntrie * processWidth * XSP_ELEM_SIZE(XSP_16UC1));
	XspMalloc((void**)&m_matDefaultEnhance.data, m_matDefaultEnhance.Size(), MemTab);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetKeyNameByDeviceInfo(XRAY_DEVICEINFO& xraylib_deviceInfo,
	                                                      char* szCnnKey, char* szCnnKeyTmp, 
	                                                      char* szDeviceKey)
{
	if (xraylib_deviceInfo.xraylib_detectortype < XRAY_DETECTORTYPE_MIN_VALUE ||
		xraylib_deviceInfo.xraylib_detectortype > XRAY_DETECTORTYPE_MAX_VALUE)
	{
		return XRAY_LIB_DETECTORTYPE_ERROR;
	}
	if (xraylib_deviceInfo.xraylib_devicetype < XRAY_DEVICETYPE_MIN_VALUE ||
		xraylib_deviceInfo.xraylib_devicetype > XRAY_DEVICETYPE_MAX_VALUE)
	{
		return XRAY_LIB_DEVICETYPE_ERROR;
	}
	if (xraylib_deviceInfo.xraylib_energymode < XRAYLIB_ENERGY_MODE_MIN_VALUE ||
		xraylib_deviceInfo.xraylib_energymode >= XRAYLIB_ENERGY_MODE_MAX_VALUE)
	{
		return XRAY_LIB_ENERGYMODE_ERROR;
	}
	if (xraylib_deviceInfo.xraylib_sourcetype < XRAY_SOURCETYPE_MIN_VALUE ||
		xraylib_deviceInfo.xraylib_sourcetype > XRAY_SOURCETYPE_MAX_VALUE)
	{
		return XRAY_LIB_SOURCETYPE_ERROR;
	}
	if (xraylib_deviceInfo.xraylib_visualNo < XRAYLIB_VISUAL_MIN_VALUE ||
		xraylib_deviceInfo.xraylib_visualNo > XRAYLIB_VISUAL_MAX_VALUE)
	{
		return XRAY_LIB_VISUALVIEW_ERROR;
	}

	XSP_CHECK(szCnnKey && szDeviceKey, XRAY_LIB_NULLPTR);

	strcpy(szDeviceKey, GetDeviceTypeString(xraylib_deviceInfo.xraylib_devicetype));
	strcat(szDeviceKey, GetEnergyString(xraylib_deviceInfo.xraylib_energymode));
	strcat(szDeviceKey, GetVisualString(xraylib_deviceInfo.xraylib_visualNo));
	strcat(szDeviceKey, GetValueTypeString(xraylib_deviceInfo.xraylib_devicetype));
	strcat(szDeviceKey, GetDetectorString(xraylib_deviceInfo.xraylib_detectortype));
	strcat(szDeviceKey, GetSourceString(xraylib_deviceInfo.xraylib_sourcetype));

	strcpy(szCnnKey, GetAiDeviceTypeString(xraylib_deviceInfo.xraylib_devicetype));
	strcat(szCnnKey, GetEnergyString(xraylib_deviceInfo.xraylib_energymode));
	strcat(szCnnKey, GetDetectorString(xraylib_deviceInfo.xraylib_detectortype));

	strcpy(szCnnKeyTmp, GetAiDeviceTypeString(xraylib_deviceInfo.xraylib_devicetype));
	strcat(szCnnKeyTmp, GetEnergyString(xraylib_deviceInfo.xraylib_energymode));
	strcat(szCnnKeyTmp, GetValueTypeString(xraylib_deviceInfo.xraylib_devicetype));
	strcat(szCnnKeyTmp, GetDetectorString(xraylib_deviceInfo.xraylib_detectortype));

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetKeyNameByDeviceInfo(XRAY_DEVICEINFO & xraylib_deviceInfo, char * szDeviceKey)
{
	if (xraylib_deviceInfo.xraylib_detectortype < XRAY_DETECTORTYPE_MIN_VALUE ||
		xraylib_deviceInfo.xraylib_detectortype > XRAY_DETECTORTYPE_MAX_VALUE)
	{
		return XRAY_LIB_DETECTORTYPE_ERROR;
	}
	if (xraylib_deviceInfo.xraylib_devicetype < XRAY_DEVICETYPE_MIN_VALUE ||
		xraylib_deviceInfo.xraylib_devicetype > XRAY_DEVICETYPE_MAX_VALUE)
	{
		return XRAY_LIB_DEVICETYPE_ERROR;
	}
	if (xraylib_deviceInfo.xraylib_energymode < XRAYLIB_ENERGY_MODE_MIN_VALUE ||
		xraylib_deviceInfo.xraylib_energymode >= XRAYLIB_ENERGY_MODE_MAX_VALUE)
	{
		return XRAY_LIB_ENERGYMODE_ERROR;
	}
	if (xraylib_deviceInfo.xraylib_sourcetype < XRAY_SOURCETYPE_MIN_VALUE ||
		xraylib_deviceInfo.xraylib_sourcetype > XRAY_SOURCETYPE_MAX_VALUE)
	{
		return XRAY_LIB_SOURCETYPE_ERROR;
	}
	if (xraylib_deviceInfo.xraylib_visualNo < XRAYLIB_VISUAL_MIN_VALUE ||
		xraylib_deviceInfo.xraylib_visualNo > XRAYLIB_VISUAL_MAX_VALUE)
	{
		return XRAY_LIB_VISUALVIEW_ERROR;
	}

	strcpy(szDeviceKey, GetDeviceTypeString(xraylib_deviceInfo.xraylib_devicetype));
	strcat(szDeviceKey, GetEnergyString(xraylib_deviceInfo.xraylib_energymode));
	strcat(szDeviceKey, GetVisualString(xraylib_deviceInfo.xraylib_visualNo));
	strcat(szDeviceKey, GetValueTypeString(xraylib_deviceInfo.xraylib_devicetype));
	strcat(szDeviceKey, GetDetectorString(xraylib_deviceInfo.xraylib_detectortype));
	strcat(szDeviceKey, GetSourceString(xraylib_deviceInfo.xraylib_sourcetype));
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::InitBSDeviceParaByIni(const char* szDeviceIniPath, const char* szDeviceKey)
{
	XSP_CHECK(szDeviceIniPath && szDeviceKey, XRAY_LIB_NULLPTR);

	ini_t *config = ini_load(szDeviceIniPath);
	XSP_CHECK(config, XRAY_LIB_NULLPTR, "Load DeviceInfo.ini failed.");

	int32_t hr = 0;
	char* pPath = LIBXRAY_NULL;

	/*********************** 
	*    配置文件名称获取 
	************************/
	printf("szDeviceIniPath %s, szDeviceKey %s\n", szDeviceIniPath, szDeviceKey);
	/* 背散射几何配准表 */
	hr = ini_sget(config, szDeviceKey, "GeoCaliTableName", NULL, &pPath);
	if (hr != INI_SGET_FAILURE)
	{
		strcpy(m_szGeoCaliName, pPath);
		m_modCali.m_bGeoCaliTableExist = true;
	}
	XSP_CHECK(INI_SGET_SUCCESS == hr, XRAY_LIB_INI_ERROR);

	/* 透射几何光路图 */
	hr = ini_sget(config, szDeviceKey, "GeometryName", NULL, &pPath);
	if (hr != INI_SGET_FAILURE)
	{
		strcpy(m_szGeometryName, pPath);
		m_modCali.m_bGeometryFigExist = true;
	}

	/***********************
	*       参数获取
	************************/
	hr = ini_sget(config, szDeviceKey, "BSGeoCaliInverse", "%d", &m_modCali.m_bGeoCaliBSInverse);	// 背散射畸变矫正

	hr = ini_sget(config, szDeviceKey, "TSGeoCaliInverse", "%d", &m_modCali.m_bGeoCaliInverse);		//透射畸变矫正

	hr &= ini_sget(config, szDeviceKey, "nImageWidthBS", "%d", &m_modCali.m_nImageWidthBS);

	hr &= ini_sget(config, szDeviceKey, "fPulsBS", "%f", &m_modCali.m_fPulsBS);

	hr &= ini_sget(config, szDeviceKey, "nwSkipUpBS", "%d", &m_modCali.m_nwSkipUpBS);

	hr &= ini_sget(config, szDeviceKey, "nwSkipDownBS", "%d", &m_modCali.m_nwSkipDownBS);

	hr &= ini_sget(config, szDeviceKey, "fAntiBS1_AI", "%f", &m_modCali.m_fAntiBS1_AI);

	hr &= ini_sget(config, szDeviceKey, "fAntiBS2_AI", "%f", &m_modCali.m_fAntiBS2_AI);

	hr &= ini_sget(config, szDeviceKey, "fAntiBS1_NLM", "%f", &m_modCali.m_fAntiBS1_NLM);

	hr &= ini_sget(config, szDeviceKey, "fAntiBS2_NLM", "%f", &m_modCali.m_fAntiBS2_NLM);

	hr &= ini_sget(config, szDeviceKey, "eWeight_AI", "%f", &m_modImgproc.m_edgeWeight_AI);

	hr &= ini_sget(config, szDeviceKey, "eWeight_NLM", "%f", &m_modImgproc.m_edgeWeight_NLM);

	// hr &= ini_sget(config, szDeviceKey, "filterS", "%d", &m_modImgproc.sigmaI);	// NLMeans中作为局部变量使用

	hr &= ini_sget(config, szDeviceKey, "fCompress", "%f", &m_modCali.m_fCompress);

	// hr &= ini_sget(config, szDeviceKey, "pBSCali", "%d", &m_modCali.pBSCali);	// 二次校正使用, 目前不用

	XSP_CHECK(INI_SGET_SUCCESS == hr, XRAY_LIB_INI_ERROR, "Read DeviceInfo.ini failed.");

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::InitDeviceParaByIni(const char* szDeviceIniPath, const char* szDeviceKey)
{
	XSP_CHECK(szDeviceIniPath && szDeviceKey, XRAY_LIB_NULLPTR);

	ini_t *config = ini_load(szDeviceIniPath);
	XSP_CHECK(config, XRAY_LIB_NULLPTR, "Load DeviceInfo.ini failed.");

	int32_t hr = 0;
	char* pPath = LIBXRAY_NULL;

	/*********************** 
	*    配置文件名称获取 
	************************/
	/* Z值表 */
	hr = ini_sget(config, szDeviceKey, "ZTableName", NULL, &pPath);
	if (hr != INI_SGET_FAILURE)
	{
		strcpy(m_szZTableName, pPath);
	}
	XSP_CHECK(INI_SGET_SUCCESS == hr, XRAY_LIB_INI_ERROR);

	/*******************************************
	*      以下文件非必须的配置，可不存在
	********************************************/
	/* 6色Z值表 */
	hr = ini_sget(config, szDeviceKey, "Z6TableName", NULL, &pPath);
	if (hr != INI_SGET_FAILURE && strcmp(pPath, "nullptr") != 0)
	{
		strcpy(m_szZ6TableName, pPath);
		m_modDual.m_bZ6CanUse = true;
	}

	/* 混合Z值表 */
	hr = ini_sget(config, szDeviceKey, "MixTableName", NULL, &pPath);
	if (hr != INI_SGET_FAILURE)
	{
		strcpy(m_szMixZTableName, pPath);
		m_bMixCurveValid = true;
	}

	/* 畸变校正表 */
	hr = ini_sget(config, szDeviceKey, "GeoCaliTableName", NULL, &pPath);
	if (hr != INI_SGET_FAILURE)
	{
		strcpy(m_szGeoCaliName, pPath);
		m_modCali.m_bGeoCaliTableExist = true;
	}

	/* 几何光路图 */
	hr = ini_sget(config, szDeviceKey, "GeometryName", NULL, &pPath);
	if (hr != INI_SGET_FAILURE)
	{
		strcpy(m_szGeometryName, pPath);
		m_modCali.m_bGeometryFigExist = true;
	}

	/* 平铺校正表 */
	hr = ini_sget(config, szDeviceKey, "FlatDetCaliTableName", NULL, &pPath);
	if (hr != INI_SGET_FAILURE && strcmp(pPath, "nullptr") == 1)
	{
		strcpy(m_szFlatDetCaliName, pPath);
		m_modCali.m_bFlatDetCaliCanUse = true;
	}


	/***********************
	*       参数获取
	************************/
	hr = ini_sget(config, szDeviceKey, "CaliGeomFactor", "%f", &m_modCali.m_fGeomFactor);

	hr = ini_sget(config, szDeviceKey, "CaliDetecLocatiion", "%d", &m_modCali.m_nDetecLocatiion);
	/*******************************************
	*      以下参数为必须的配置，必须存在
	********************************************/
	hr = ini_sget(config, szDeviceKey, "FlatDetCaliInverse", "%d", &m_modCali.m_bFlatDetCaliInverse);

	hr &= ini_sget(config, szDeviceKey, "GeoCaliInverse", "%d", &m_modCali.m_bGeoCaliInverse);

	hr &= ini_sget(config, szDeviceKey, "CaliLowGrayThreshold", "%d", &m_modImgproc.m_nCaliLowGrayThreshold);

	hr &= ini_sget(config, szDeviceKey, "LowPenetrationThreshold", "%d", &m_modArea.m_nLowPenetrationThreshold);

	hr &= ini_sget(config, szDeviceKey, "BackGroundThreshold", "%d", &m_sharedPara.m_nBackGroundThreshold);

	hr &= ini_sget(config, szDeviceKey, "EnhanceGrayUp", "%d", &m_modImgproc.m_nEnhanceGrayUp);

	hr &= ini_sget(config, szDeviceKey, "EnhanceGrayDown", "%d", &m_modImgproc.m_nEnhanceGrayDown);
	
	hr &= ini_sget(config, szDeviceKey, "lowPenetrationThresholdL", "%d", &m_nLowPenesensParam.lowPeneMin);

	hr &= ini_sget(config, szDeviceKey, "caliLowGrayThresholdL", "%d", &m_nLowPenesensParam.LowGrayMin);

	hr &= ini_sget(config, szDeviceKey, "lowPenetrationThresholdM", "%d", &m_nLowPenesensParam.lowPeneMid);

	hr &= ini_sget(config, szDeviceKey, "caliLowGrayThresholdM", "%d", &m_nLowPenesensParam.LowGrayMid);

	hr &= ini_sget(config, szDeviceKey, "lowPenetrationThresholdH", "%d", &m_nLowPenesensParam.lowPeneMax);

	hr &= ini_sget(config, szDeviceKey, "caliLowGrayThresholdH", "%d", &m_nLowPenesensParam.LowGrayMax);

	XSP_CHECK(INI_SGET_SUCCESS == hr, XRAY_LIB_INI_ERROR, "Read DeviceInfo.ini failed.");

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::GetRCurvePathByIni(const char* szDeviceIniPath, 
													  const char* szDeviceKey,
													  char* szZTablePath,
													  char* szMixZTablePath,
													  bool& nGetMixCurveValid)
{
	XSP_CHECK(szDeviceIniPath && szDeviceKey, XRAY_LIB_NULLPTR);

	ini_t *config = ini_load(szDeviceIniPath);
	XSP_CHECK(config, XRAY_LIB_NULLPTR, "Load DeviceInfo.ini failed.");

	int32_t hr = 0;
	char* pPath = LIBXRAY_NULL;

	/***********************
	*    配置文件名称获取
	************************/
	/* Z值表 */
	hr = ini_sget(config, szDeviceKey, "ZTableName", NULL, &pPath);
	if (hr != INI_SGET_FAILURE)
	{
		strcpy(szZTablePath, pPath);
	}
	XSP_CHECK(INI_SGET_SUCCESS == hr, XRAY_LIB_INI_ERROR);

	/* 混合Z值表 */
	hr = ini_sget(config, szDeviceKey, "MixTableName", NULL, &pPath);
	if (hr != INI_SGET_FAILURE)
	{
		strcpy(szMixZTablePath, pPath);
		nGetMixCurveValid = true;
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::ZU16ToU8(XMat& Z16, XMat& Z8)
{
	/* 这里是与raw2yuv结果对齐的写法 代码不要改动!!!!!! */
	uint16_t* pZU16 = Z16.Ptr<uint16_t>();
	uint8_t* pZU8 = Z8.Ptr<uint8_t>();
	for (int32_t idx = 0; idx < Z16.wid * Z16.hei; idx++)
	{
		float32_t temp = ((float32_t)(pZU16[idx]) / 65535) * 43;
		int32_t nMaterial = (int32_t)(temp + 0.005);
		nMaterial = Clamp(nMaterial, 0, 34);
		pZU8[idx] = (uint8_t)(nMaterial);
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::MergeSingleAiOri(XMat& matAi, XMat& matOri, float32_t ratio)
{
	XSP_CHECK(matAi.IsValid() && matOri.IsValid(), XRAY_LIB_XMAT_INVALID);
	XSP_CHECK(XSP_16UC1 == matAi.type && XSP_16UC1 == matOri.type, XRAY_LIB_XMAT_TYPE_ERR);

	if (matAi.wid == matOri.wid)
	{
		uint16_t* ai = matAi.PadPtr<uint16_t>();
		uint16_t* ori = matOri.PadPtr<uint16_t>();
		for (int32_t i = 0; i < matAi.wid * (matAi.hei - matAi.tpad * 2); i++)
		{
			ai[0] = uint16_t(ratio * ai[0] + (1.0f - ratio) * ori[0]);
			ai++;
			ori++;
		}

		return XRAY_LIB_OK;
	}

	if (matAi.wid > matOri.wid)
	{
		int32_t heiOut = matAi.hei;
		int32_t heiIn = matOri.hei;
		int32_t widOut = matAi.wid;
		int32_t widIn = matOri.wid;

		float32_t fScaleHei = float32_t(heiOut) / float32_t(heiIn);
		float32_t fScaleWid = float32_t(widOut) / float32_t(widIn);

		int32_t Index_A, Index_B, Index_C, Index_D, Index;

		uint16_t* ai = matAi.Ptr<uint16_t>();
		uint16_t* ori = matOri.Ptr<uint16_t>();
		for (int32_t i = matAi.tpad; i < heiOut - matAi.tpad; i++)
		{
			for (int32_t j = 0; j < widOut; j++)
			{
				float32_t Temp_i = (float32_t)((i + 0.5) / fScaleHei - 0.5);
				float32_t Temp_j = (float32_t)((j + 0.5) / fScaleWid - 0.5);

				int32_t Round_i = (int32_t)(Temp_i);
				int32_t Round_j = (int32_t)(Temp_j);

				float32_t u = Temp_i - Round_i;
				float32_t v = Temp_j - Round_j;

				Round_i = Clamp(Round_i, 0, heiIn - 2);
				Round_j = Clamp(Round_j, 0, widIn - 2);

				Index_A = Round_i * widIn + Round_j;
				Index_B = Round_i * widIn + Round_j + 1;
				Index_C = (Round_i + 1) * widIn + Round_j;
				Index_D = (Round_i + 1) * widIn + Round_j + 1;

				float32_t TempZ = (1 - u) * (1 - v) * ori[Index_A] +
					          (1 - u) * v * ori[Index_B] +
					          u * (1 - v) * ori[Index_C] +
					          u * v * ori[Index_D];

				Index = i * widOut + j;
				ai[Index] = uint16_t(ratio * ai[Index] + (1.0f - ratio) * TempZ);
			}
		}

		return XRAY_LIB_OK;
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::MergeDualAiOri(XMat& matAiHigh, XMat& matAiLow,
	                                              XMat& matOriHigh, XMat& matOriLow, float32_t ratio)
{
	XSP_CHECK(matAiHigh.IsValid() && matOriHigh.IsValid() && 
		      matAiLow.IsValid() && matOriLow.IsValid(), 
		      XRAY_LIB_XMAT_INVALID);

	XSP_CHECK(XSP_16UC1 == matAiHigh.type && XSP_16UC1 == matOriHigh.type &&
		      XSP_16UC1 == matAiLow.type && XSP_16UC1 == matOriLow.type,
		      XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(matAiHigh, matAiLow) && MatSizeEq(matOriHigh, matOriLow),
		      XRAY_LIB_XMAT_SIZE_ERR);

	if (matAiHigh.wid == matOriHigh.wid)
	{
		uint16_t* ai_h = matAiHigh.PadPtr<uint16_t>();
		uint16_t* ori_h = matOriHigh.PadPtr<uint16_t>();
		uint16_t* ai_l = matAiLow.PadPtr<uint16_t>();
		uint16_t* ori_l = matOriLow.PadPtr<uint16_t>();
		for (int32_t i = 0; i < matAiHigh.wid * (matAiHigh.hei - matAiHigh.tpad * 2); i++)
		{
			ai_h[0] = uint16_t(ratio * ai_h[0] + (1.0f - ratio) * ori_h[0]);

			ai_l[0] = uint16_t(ratio * ai_l[0] + (1.0f - ratio) * ori_l[0]);

			ai_h++;
			ai_l++;
			ori_h++;
			ori_l++;
		}
		return XRAY_LIB_OK;
	}

	if (matAiHigh.wid > matOriHigh.wid)
	{
		int32_t heiOut = matAiHigh.hei;
		int32_t heiIn = matOriHigh.hei;
		int32_t widOut = matAiHigh.wid;
		int32_t widIn = matOriHigh.wid;

		float32_t fScaleHei = float32_t(heiOut) / float32_t(heiIn);
		float32_t fScaleWid = float32_t(widOut) / float32_t(widIn);

		int32_t Index_A, Index_B, Index_C, Index_D, Index;
		float32_t TempZ;

		uint16_t* ai_h = matAiHigh.Ptr<uint16_t>();
		uint16_t* ori_h = matOriHigh.Ptr<uint16_t>();
		uint16_t* ai_l = matAiLow.Ptr<uint16_t>();
		uint16_t* ori_l = matOriLow.Ptr<uint16_t>();
		for (int32_t i = matAiHigh.tpad; i < heiOut - matAiHigh.tpad; i++)
		{
			for (int32_t j = 0; j < widOut; j++)
			{
				float32_t Temp_i = (float32_t)((i + 0.5) / fScaleHei - 0.5);
				float32_t Temp_j = (float32_t)((j + 0.5) / fScaleWid - 0.5);

				int32_t Round_i = (int32_t)(Temp_i);
				int32_t Round_j = (int32_t)(Temp_j);

				float32_t u = Temp_i - Round_i;
				float32_t v = Temp_j - Round_j;

				Round_i = Clamp(Round_i, 0, heiIn - 2);
				Round_j = Clamp(Round_j, 0, widIn - 2);

				Index_A = Round_i * widIn + Round_j;
				Index_B = Round_i * widIn + Round_j + 1;
				Index_C = (Round_i + 1) * widIn + Round_j;
				Index_D = (Round_i + 1) * widIn + Round_j + 1;

				Index = i * widOut + j;

				TempZ = (1 - u) * (1 - v) * ori_h[Index_A] +
					    (1 - u) * v * ori_h[Index_B] +
					    u * (1 - v) * ori_h[Index_C] +
					    u * v * ori_h[Index_D];

				ai_h[Index] = uint16_t(ratio * ai_h[Index] + (1.0f - ratio) * TempZ);


				TempZ = (1 - u) * (1 - v) * ori_l[Index_A] +
					    (1 - u) * v * ori_l[Index_B] +
					    u * (1 - v) * ori_l[Index_C] +
					    u * v * ori_l[Index_D];

				ai_l[Index] = uint16_t(ratio * ai_l[Index] + (1.0f - ratio) * TempZ);
			}
		}

		return XRAY_LIB_OK;
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::MergeDualAiOriFp32ByMask(XMat& matAiHigh, XMat& matAiLow,
	                                                  XMat& matOriHigh, XMat& matOriLow, XMat& matFuseMask)
{

	// 参数合法性校验
	XSP_CHECK(matAiHigh.IsValid() && matOriHigh.IsValid() ,XRAY_LIB_XMAT_INVALID);

	// 检查mask图像尺寸是否正常,否则不做mask权重叠加
	if(!MatSizeEq(matFuseMask, matOriHigh))
	{
		return XRAY_LIB_OK;
	}

	XSP_CHECK(XSP_32FC1 == matAiHigh.type && XSP_32FC1 == matOriHigh.type && XSP_32FC1 == matAiLow.type && XSP_32FC1 == matOriLow.type, XRAY_LIB_PARAM_WIDTH_ERROR); 

	XSP_CHECK(MatSizeEq(matAiHigh, matAiLow) && MatSizeEq(matOriHigh, matOriLow) && MatSizeEq(matFuseMask, matAiHigh) && MatSizeEq(matAiHigh, matOriHigh),XRAY_LIB_PARAM_WIDTH_ERROR);

	auto scale = m_modCnn.AiXsp_GetScale();
	// TODO：超分模型临时处理，后面需要根据Mask实际用法进行优化
	if((scale.first > 1.0) || (scale.second > 1.0))
	{
		float32_t* pWeight = matFuseMask.Ptr<float32_t>();
		for (int32_t i = 0; i < matFuseMask.hei * matFuseMask.wid; i++)
		{
			*pWeight = m_fSigmaSr * 2;	
			*pWeight = Clamp(*pWeight, 0.0f, 1.0f);
			pWeight++;
		}
	}
	else
	{
		// 根据m_AiEffectFactor计算AIMASK区间,线性映射区间0~100
		std::pair<float32_t, float32_t> aiEffectRange{0.2, 0.9}; //m_AiEffectFactor为50时为默认区间0.2-0.9
		aiEffectRange.first = (m_AiEdgeFactor <= 50) ?                                      						\
			(aiEffectRange.first) : 														  						\
			(aiEffectRange.first + (m_AiEdgeFactor - 50) * (aiEffectRange.second - aiEffectRange.first) / 50.0f);

		aiEffectRange.second = (m_AiEdgeFactor <= 50) ? 									  						 \
			(aiEffectRange.second - (50 - m_AiEdgeFactor) * (aiEffectRange.second - aiEffectRange.first) / 50.0f): \
			(aiEffectRange.second);

		// oriAIWeight中低于fRange.first的权重设置为0，高于fRange.second的权重设置为1，然后将fRange区间内的像素值线性映射到[0,1]区间
		float32_t fRangeLen = aiEffectRange.second - aiEffectRange.first;
		float32_t* pWeight = matFuseMask.Ptr<float32_t>();
		int32_t ilevel =  m_fSigmaDn * 100;

		float32_t adjust = (ilevel <= 50) ? (-(50 - ilevel) * 0.02) : ((ilevel - 50) * 0.02);

		for (int32_t i = 0; i < matFuseMask.hei * matFuseMask.wid; i++)
		{
			if (*pWeight < aiEffectRange.first)
			{
				*pWeight = 0.0f;
			}
			else if (*pWeight > aiEffectRange.second)
			{
				*pWeight = 1.0f;
			}
			else
			{
				*pWeight = (*pWeight - aiEffectRange.first) / fRangeLen;
			}

			*pWeight += adjust;	
			*pWeight = Clamp(*pWeight, 0.0f, 1.0f);
			pWeight++;
		}
	}

	// 合并双层AI和原图
	float32_t* pFuseMask = matFuseMask.Ptr<float32_t>();
	float32_t* pAiHigh = matAiHigh.Ptr<float32_t>();
	float32_t* pOriHigh = matOriHigh.Ptr<float32_t>();
	float32_t* pAiLow = matAiLow.Ptr<float32_t>();
	float32_t* pOriLow = matOriLow.Ptr<float32_t>();
	for (int i = 0; i < matFuseMask.wid * matFuseMask.hei; i++)
	{
		float fuseWeight = *pFuseMask;
		float aiHighVal = *pAiHigh;
		float oriHighVal = *pOriHigh;
		float aiLowVal = *pAiLow;
		float oriLowVal = *pOriLow;

		if(aiHighVal < 2000 / 65535.0f)
		{
			*pAiHigh = oriHighVal;
			*pAiLow = oriLowVal;
		}
		else
		{
		*pAiHigh = fuseWeight * aiHighVal + (1.0f - fuseWeight) * oriHighVal;
		*pAiLow = fuseWeight * aiLowVal + (1.0f - fuseWeight) * oriLowVal;
		}
		
		pFuseMask++;
		pAiHigh++;
		pOriHigh++;
		pAiLow++;
		pOriLow++;
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::UpdateResizePara(int32_t nWidth, int32_t nHeight)
{
	/*宽度方向*/
	m_nResizeWidth = static_cast<int32_t>(nWidth * m_fWidthResizeFactor);
	int32_t nRemainder = m_nResizeWidth % XRAY_LIB_RESIZE_MULTIPLE;
	if (nRemainder != 0)
	{
		m_nResizeWidth += (XRAY_LIB_RESIZE_MULTIPLE - nRemainder);
	}

	/*高度方向*/
	int32_t nHeightResized = static_cast<int32_t>(nHeight * m_fHeightResizeFactor);
	nRemainder = nHeightResized % XRAY_LIB_RESIZE_MULTIPLE;
	if (nRemainder != 0)
	{
		nHeightResized += (XRAY_LIB_RESIZE_MULTIPLE - nRemainder);
	}

	/* 扩边默认为条带高度 */
	int32_t nPadLen = nHeight;
	float32_t fScaleH = float32_t(nHeightResized) / nHeight; // 使用对齐后的高度重新计算缩放系数
	m_nResizeHeight = (int32_t)floor((nHeight + 2 * nPadLen) * fScaleH); // 缩放后条带高度(带扩边)
	if ((m_nResizeHeight - nHeightResized) % 2 != 0)
	{
		m_nResizeHeight++;
	}
	m_nResizePadLen = (m_nResizeHeight - nHeightResized) / 2; // 缩放后扩边长度，供下级流水使用

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT XRayImageProcess::SetProcessDataDumpPrm(int32_t dataPoint, int32_t dumpCount, char* pDirName)
{
    std::string dumpDir = (nullptr != pDirName) ? pDirName : "/tmp/"; // 若为空则dump到/tmp目录
	m_modImgproc.m_islPipeGray.SetDump(dataPoint, dumpDir, dumpCount);
	m_modDual.m_islPipeChroma.SetDump(dataPoint, dumpDir, dumpCount);

	return XRAY_LIB_OK;
}