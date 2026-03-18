#include "libXRay.h"
#include "xsp_interface.hpp"
#include "utils/aes.h"
#include "xray_image_process.hpp"
#include "utils/strptime.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <string>
#include <iostream>
#include <ctime>


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


// AES算法库校验模块的全局变量
int G_AesOk = false;

// Plaintext:  明文
#ifdef AES_DEMO
uint8_t G_Plaintext[] = { "0123456789abcdef0123456789abcdef" };
#else
uint8_t G_Plaintext[32] = {0};
#endif

/*******1，获取版本号***********/
XRAYLIB_XRY_VERSION libXRay_GetVersion(XRAY_LIB_VERSION_TYPE versionType)
{
	///////////////////各版本号值不超过8bit
	XRAYLIB_XRY_VERSION xRayVersion;
    // 版本号，格式为：主版本.子版本.编译版本，在CMakeLists.txt中修改PROJECT_VERSION_MAJOR与PROJECT_VERSION_MINOR的定义
#ifndef PROJECT_VERSION_MAJOR
#define PROJECT_VERSION_MAJOR (8)
#endif 
#ifndef PROJECT_VERSION_MINOR
#define PROJECT_VERSION_MINOR (8)
#endif 
#ifndef PROJECT_VERSION_PATCH
#define PROJECT_VERSION_PATCH (8)
#endif 

	// 版本号设置
	switch (versionType) {
		case XRAYLIB_VERSION_IMAGING_ALG:
			xRayVersion.major = PROJECT_VERSION_MAJOR;
			xRayVersion.minor = PROJECT_VERSION_MINOR;
			xRayVersion.revision = PROJECT_VERSION_PATCH;
			break;
		case XRAYLIB_VERSION_EXPLOSIVE_DETECT_ALG1:
			xRayVersion.major = 1;
			xRayVersion.minor = 3;
			xRayVersion.revision = 0;
			break;
		case XRAYLIB_VERSION_EXPLOSIVE_DETECT_ALG2:
			xRayVersion.major = 1;
			xRayVersion.minor = 3;
			xRayVersion.revision = 0;
			break;
		case XRAYLIB_VERSION_LOWPENETRATION_DETECT_ALG:
			xRayVersion.major = 1;
			xRayVersion.minor = 3;
			xRayVersion.revision = 0;
			break;
		default:
			xRayVersion.major = PROJECT_VERSION_MAJOR;
			xRayVersion.minor = PROJECT_VERSION_MINOR;
			xRayVersion.revision = PROJECT_VERSION_PATCH;
	}
	// 后续可以继续添加其他子版本...

	// 编译日期处理部分
	std::istringstream sDate(__DATE__); //MMM DDD YYY
	std::vector<std::string> vecDate;
	std::vector<std::string> vecMonth = { "Jan","Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	std::string word;
	while (sDate >> word)
	{
		vecDate.push_back(word);
	}
	for (int i = 0; i < static_cast<int>(vecMonth.size()); i++)
	{
		if (vecDate[0] == vecMonth[i])
		{
			xRayVersion.month = i + 1;
			break;
		}
	}
	xRayVersion.day = std::stoi(vecDate[1]);
	xRayVersion.year = std::stoi(vecDate[2]);

	return xRayVersion;
}

/*******2，获取开辟内存大小***********/
XRAY_LIB_HRESULT libXRay_GetMemSize(XRAY_LIB_ABILITY*	   ability,
                                    XRAY_LIB_MEM_TAB       mem_tab[XRAY_LIB_MTAB_NUM])
{
	//这里需要传入的高度和宽度必须保持为偶数
	if(ability->nDetectorWidth <=0)
		return XRAY_LIB_IMAGESIZE_ZERO;

	XRayImageProcess XspModule;
	XspModule.SetMemTabSize(mem_tab, ability);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT libXRay_Create(XRAY_LIB_ABILITY*	  	ability,
	                            XRAYLIB_AI_PARAM      	xraylib_AIPara, 
	                            const char*           PublicFileFolderName, 
	                            const char*           PrivateFileFolderName, 
	                            XRAY_LIB_MEM_TAB      	mem_tab[XRAY_LIB_MTAB_NUM], 
	                            VOID**                	handle, 
	                            XRAYLIB_PIPELINE_MODE 	xraylib_pipeline_mode, 
	                            XRAY_DEVICEINFO*      	xraylib_deviceInfo)
{
	IXRayImageProcess* pXspHandle = CreateXspObj(mem_tab[0].base);
	*handle = pXspHandle;

	XRAY_LIB_HRESULT hr;

	XRAYLIB_FILEPATH_MODE  filepathmode = XRAYLIB_FILEPATH_OPPOSITE;

	hr = pXspHandle->Init(xraylib_pipeline_mode, 
		                  filepathmode, 
		                  *xraylib_deviceInfo, 
		                  xraylib_AIPara,
		                  mem_tab, 
		                  PublicFileFolderName, 
		                  PrivateFileFolderName, 
		                  ability);

	return hr;
}

XRAY_LIB_HRESULT libXRay_Create_AbsolutePath(XRAY_LIB_ABILITY*	   ability,
	                                         XRAYLIB_AI_PARAM      xraylib_AIPara, 
	                                         const char*           PublicFileFolderName, 
	                                         const char*           PrivateFileFolderName, 
	                                         XRAY_LIB_MEM_TAB      mem_tab[XRAY_LIB_MTAB_NUM], 
	                                         VOID**                handle, 
	                                         XRAYLIB_PIPELINE_MODE xraylib_pipeline_mode, 
	                                         XRAY_DEVICEINFO*      xraylib_deviceInfo)
{
	IXRayImageProcess* pXspHandle = CreateXspObj(mem_tab[0].base);
	*handle = pXspHandle;

	XRAY_LIB_HRESULT hr;

	XRAYLIB_FILEPATH_MODE  filepathmode = XRAYLIB_FILEPATH_ABSOLUTE;

	hr = pXspHandle->Init(xraylib_pipeline_mode, 
		                  filepathmode, 
		                  *xraylib_deviceInfo, 
		                  xraylib_AIPara,
		                  mem_tab, 
		                  PublicFileFolderName, 
		                  PrivateFileFolderName, 
		                  ability);
	return hr;
}

/*******5，打开或关闭算法输出***********/
XRAY_LIB_HRESULT libXRay_EnablePrint(VOID                   *handle,
									XRAY_LIB_ONOFF			xarylibprint)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	if (XRAYLIB_OFF == xarylibprint)
		hr = pXspHandle->SetLibXRayPrint(false);
	else if (XRAYLIB_ON == xarylibprint)
		hr = pXspHandle->SetLibXRayPrint(true);
	else
		hr = XRAY_LIB_INVALID_PARAM;

	return hr;
}

XRAY_LIB_HRESULT libXRay_UpdateZeroAndFull(VOID                            *handle, 
	                                       XRAYLIB_IMAGE                   *p_xraylib_org_lh,
	                                       XRAYLIB_UPDATE_ZEROANDFULL_MODE updatemode)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->GenerateCaliTable(*p_xraylib_org_lh, updatemode);

	return hr;
}

XRAY_LIB_HRESULT libXRay_UpdateZeroAndFullMem(VOID                            *handle,
	                                          XRAYLIB_IMAGE                   *p_xraylib_org_lh,
	                                          XRAYLIB_UPDATE_ZEROANDFULL_MODE updatemode)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->UpdateCaliMem(*p_xraylib_org_lh, updatemode);

	return hr;
}

XRAY_LIB_HRESULT libXRay_SetTIPImage(VOID               *handle,
	                                 XRAYLIB_IMAGE      *p_xraylib_tip_img,
									 int packageHeight,
									 int randomPosition)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->SetTipImage(*p_xraylib_tip_img, packageHeight, randomPosition);
	return hr;
}

/*******8，强制停止当前正在进行的TIP操作***********/
XRAY_LIB_HRESULT libXRay_ForceStopTIP(VOID *handle)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->ForceStopTIP();
	return hr;
}

XRAY_LIB_HRESULT libXRay_GetImage(VOID                   *handle,
                                  XRAYLIB_IMAGE          *p_xraylib_calilhz,
								  XRAYLIB_IMAGE          *p_xraylib_merge,
								  XRAYLIB_IMAGE          *p_xraylib_color,
	                              XRAYLIB_DUMPPROC_TIP_PARAM *p_xraylib_tip,
							      XRAYLIB_IMG_CHANNEL    img_channel,
                                  int                    height_ntop, 
	                              int                    height_nbotm,
								  int					 bDescendOrder)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->CalilhzMergeToColor(*p_xraylib_calilhz, 
		                                 *p_xraylib_merge, 
		                                 *p_xraylib_color, 
		                                 *p_xraylib_tip,
		                                 img_channel, 
		                                 height_ntop, 
		                                 height_nbotm,
										 bDescendOrder);

	return hr;
}

XRAY_LIB_HRESULT libXRay_GetProcessMergeImage(VOID           *handle,
											  XRAYLIB_IMAGE  *p_xraylib_calilh,
											  XRAYLIB_IMAGE  *p_xraylib_merge,
											  int 			 height_ntop,
											  int 			 height_nbotm,
											  int 			 bDescendOrder)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->GetProcessMerge(*p_xraylib_calilh, *p_xraylib_merge, height_ntop, height_nbotm, bDescendOrder);
	return hr;
}

XRAY_LIB_HRESULT libXRay_ConcernedArea(VOID                   *handle, 
	                                   XRAYLIB_IMAGE          *p_xraylib_img_param,
	                                   XRAYLIB_CONCERED_AREA  *ConcerdMaterialM, 
	                                   XRAYLIB_CONCERED_AREA  *LowPenetration, 
	                                   XRAYLIB_CONCERED_AREA  *ExplosiveArea, 
	                                   XRAYLIB_CONCERED_AREA  *DrugArea)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->GetConcernedArea(*p_xraylib_img_param, 
		                              ConcerdMaterialM, 
		                              LowPenetration, 
		                              ExplosiveArea, 
		                              DrugArea);
	return hr;
}

XRAY_LIB_HRESULT libXRay_ConcernedAreaPiecewise(VOID                  *handle,
									            XRAYLIB_IMAGE         *p_xraylib_img_param,
									            XRAYLIB_CONCERED_AREA *ConcerdMaterialM,
									            XRAYLIB_CONCERED_AREA *LowPenetration,
									            XRAYLIB_CONCERED_AREA *ExplosiveArea,
	                                            XRAYLIB_CONCERED_AREA *DrugArea,
	                                            int                    nDetecFlag)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->GetConcernedAreaPiecewise(*p_xraylib_img_param,
		                                       ConcerdMaterialM, 
		                                       LowPenetration, 
		                                       ExplosiveArea, 
		                                       DrugArea,
		                                       nDetecFlag);
	return hr;
}

XRAY_LIB_HRESULT libXRay_SetConfig(VOID                      *handle, 
	                               VOID                      *configparam, 
	                               XRAYLIB_SETCONFIG_CHANNEL configchannel)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	if (configchannel == XRAYLIB_SETCONFIG_IMAGEORDER)
	{
		hr = pXspHandle->SetImage((XRAYLIB_CONFIG_IMAGE*)configparam);
	}
	else if (configchannel == XRAYLIB_SETCONFIG_PARAMCHANGE)
	{
		hr = pXspHandle->SetPara((XRAYLIB_CONFIG_PARAM*)configparam);
	}
	else
	{
		hr = XRAY_LIB_INVALID_PARAM;
	}

	return hr;
}

XRAY_LIB_HRESULT libXRay_GetConfig(VOID                      *handle, 
	                               VOID                      *configparam, 
	                               XRAYLIB_SETCONFIG_CHANNEL configchannel)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	if (configchannel == XRAYLIB_SETCONFIG_IMAGEORDER)
	{
		hr = pXspHandle->GetImage((XRAYLIB_CONFIG_IMAGE*)configparam);
	}
	else if (configchannel == XRAYLIB_SETCONFIG_PARAMCHANGE)
	{
		hr = pXspHandle->GetPara((XRAYLIB_CONFIG_PARAM*)configparam);
	}
	else
	{
		hr = XRAY_LIB_INVALID_PARAM;
	}

	return hr;
}

XRAY_LIB_HRESULT libXRay_GetCaliTable(VOID *handle, VOID *calitable)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	XRAYLIB_CALI_TABLE* pCaliTableStruct = (XRAYLIB_CALI_TABLE*)calitable;
	hr = pXspHandle->GetCaliTable(pCaliTableStruct);

	return hr;
}

XRAY_LIB_HRESULT libXRay_Release(VOID *handle)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	pXspHandle->Release();
	return XRAY_LIB_OK;
}



/*******18，提供随机数明文的密文***********/
XRAY_LIB_HRESULT libXRay_AESEncrypt(uint8_t *encryptedMessage)
{
	uint8_t mPlaintext[32] = { 0 };

	 // 明文数组
	char arr[100];

	// 生成一个随机数明文
	srand((unsigned int)time(NULL));//生成随机数
	for (int i = 0; i < 100; i++)
	{
	arr[i] = i;
	}

	for (int i = 0; i < 100; i++)//将字母数组打乱顺序
	{
	int p = rand() % 100;
	int q = rand() % 100;

	int temp = arr[p];
	arr[p] = arr[q];
	arr[q] = temp;
	}

	for (int i = 0; i < 32; i++)//生成结果
	{
		G_Plaintext[i] = arr[i];
		mPlaintext[i] = G_Plaintext[i];
	}

	uint8_t key[] = { "LibXRay248202108" };
	AES aes(key);

	aes.Cipher((void*)mPlaintext);
	for (int j = 0; j < 32; j++)
	{
		encryptedMessage[j] = (uint8_t)mPlaintext[j];
	}
	return XRAY_LIB_OK;
}

/*******19，实现明文对比***********/
XRAY_LIB_HRESULT libXRay_AESCompare(uint8_t *Input)
{
	// 明文为长度32的uchar类型字符数组
	if (Input == NULL)
		return XRAY_LIB_INITIALVERIFICATION_ERROR;

	for (int i = 0; i < 32; i++)
		if (Input[i] != G_Plaintext[i])
			return XRAY_LIB_INITIALVERIFICATION_ERROR;

	// 若每个字符均未报错，说明匹配正确，直接置成true
	G_AesOk = true;
	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT libXRay_PipelineProcess(VOID* handle, 
	                                     VOID* p_xraylib_pipeline, 
	                                     int   index)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->PipelineProcess(p_xraylib_pipeline, index);

	return hr;
}

XRAY_LIB_HRESULT libXRay_ReloadProfile(VOID            *handle,
	                                   XRAY_DEVICEINFO *xraylib_deviceInfo)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->ReloadProfile(xraylib_deviceInfo);

	return hr;
}

XRAY_LIB_HRESULT libXRay_GetRCurveName(VOID            *handle,
	                                   XRAY_DEVICEINFO *xraylib_deviceInfo,
	                                   XRAY_RCURVE_TYPE xraylib_rcurve_type,
                                       char* xraylib_rcurvename)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->GetRCurveName(xraylib_deviceInfo, xraylib_rcurve_type, xraylib_rcurvename);

	return hr;
}

XRAY_LIB_HRESULT libXRay_SetTestModeParams(VOID                     *handle,
	                                       XRAYLIB_MATERIAL_COLOR_PARAMS  *xraylib_testbparams)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->SetTestModeParams(xraylib_testbparams);

	return hr;
}

// 三色R曲线微调
XRAY_LIB_HRESULT libXRay_RCurveAdjust(VOID                     *handle,
	                                       XRAYLIB_RCURVE_ADJUST_PARAMS  *xraylib_rcurveparams)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->SetRCurveAdjustParams(*xraylib_rcurveparams);

	return hr;
}


XRAY_LIB_HRESULT libXRay_GetGeoOffsetLut(VOID                   *handle,
										int			p_geooffset_lut_width,
										 unsigned short         *p_geooffset_lut)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");
	XSP_CHECK(p_geooffset_lut, XRAY_LIB_NULLPTR, "p_geooffset_lut is null.");
	XSP_CHECK(p_geooffset_lut_width, XRAY_LIB_NULLPTR, "p_geooffset_lut_width is null.");

	XRAY_LIB_HRESULT hr;
	hr = pXspHandle->GetGeoOffsetLut(p_geooffset_lut, p_geooffset_lut_width);

	return hr;
}

XRAY_LIB_HRESULT libXRay_ProcessDataDump(VOID *handle, int dataPoint, int dumpCount, char* pDirName)
{
	IXRayImageProcess* pXspHandle = (IXRayImageProcess*)handle;
	XSP_CHECK(pXspHandle, XRAY_LIB_NULLPTR, "pXspHandle is null.");

	return pXspHandle->SetProcessDataDumpPrm(dataPoint, dumpCount, pDirName);
}