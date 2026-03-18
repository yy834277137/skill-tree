#include "xsp_mod_cali.hpp"
#include "xray_shared_para.hpp"
#include "core_def.hpp"
#include "xsp_alg.hpp"
#include "utils/cjson.h"
#include <math.h>
#include <ai_xsp_interface.h>
#include <vector>

CaliModule::CaliModule()
{
	/* 环形缓冲区缓存条带参数 */
	m_stLPadCache.Init();
	m_stOriLPadCache.Init();
	m_stCaliLPadCache.Init();

	m_stHPadCache.Init();
	m_stOriHPadCache.Init();
	m_stCaliHPadCache.Init();

	m_stMaskCache.Init();
	m_stBSCache.Init(0);

	/*冷热源校正相关参数*/
	m_fRTUpdateAirRatio = 0.02f;
	m_fStretchRatio = 1.0f;
	m_fSpeed = 0.0f;
	m_nTimeInterval = 30000; //us
	m_TimeColdHotParaH.nColdHotTik = 0;
	m_TimeColdHotParaH.bTimeNodeStatus = false;
	m_TimeColdHotParaH.ldOneTime = 0;
	m_TimeColdHotParaL.nColdHotTik = 0;
	m_TimeColdHotParaL.bTimeNodeStatus = false;
	m_TimeColdHotParaL.ldOneTime = 0;
	GetTimeNow(&m_TimeColdHotParaL.TimeBegin);
	m_nCountThreshold = 100;
	m_nPreType = 0;
	m_nTopHeight = 0;
	m_nDownHeight = 0;
	m_fMaxAirViranceL = 300;

	m_TimeBegin = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(m_TimeBegin);
	auto value = now_ms.time_since_epoch();
	m_lBeginTime = value.count(); // 这里得到的是自1970年1月1日以来的毫秒数

	/*亚像素偏移、边界融合*/
	m_bDefusionCanUse = false;
	
	/*剂量波动校正*/
	m_enDoseCaliMertric = XRAYLIB_OFF;

	/*几何校正*/
	m_bGeoCaliCanUse = false;
	m_bGeoCaliTableExist = false;
	m_bGeometryFigExist = false;
	m_fGeomFactor = 0.4f;
	m_nDetecLocatiion = 20;
	m_enGeoMertric = XRAYLIB_OFF;

	/*平铺校正*/
	m_bFlatDetCaliCanUse = false;
	m_enFlatDetMertric = XRAYLIB_OFF;

	/* 探测器像素之间的不一致性校正 */
	m_bInconsistencyCaliCanUse = true;

	/*冷热源校正*/
	m_enColdAndHot = XRAYLIB_OFF;

	/*上下置白参数*/
	m_nWhitenUp = 0;
	m_nWhitenDown = 0;

	/* 预设去除皮带纹行数 */
	m_nBeltGrainLimit = 10;

	/* 皮带边缘初始化默认读不到json*/
	m_stBeltdge.nStart1 = -1;
	m_stBeltdge.nEnd1 = -1;
	m_stBeltdge.nStart2 = -1;
	m_stBeltdge.nEnd2 = -1;

	m_bMirrorBS = false;
}

XRAY_LIB_HRESULT CaliModule::Release()
{
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::Init(const char* szPublicFileFolderName, 
	                              const char* szPrivateFileFolderName, 
	                              const char* szGeoCaliTableName,
	                              const char* szFlatDetCaliTableName, 
	                              const char* szGeometryName,
	                              SharedPara* pPara, int32_t nDetectorLen)
{
	
	XRAY_LIB_HRESULT hr;

	/*初始化全局通用参数*/
	m_pSharedPara = pPara;

	/* 几何光路板卡数设置, 这里先按常规数量初始化，实际板卡数可能小于该值，空板卡位置采传补0处理 */
	m_stGeometry.SetDetNum(static_cast<uint32_t>(nDetectorLen / static_cast<int32_t>(m_pSharedPara->m_nPerDetectorWidth)));

	/* 初始化几何光路计算内存 */
	m_matGeomPixPhyLen.Reshape(1, int32_t(nDetectorLen * pPara->m_fResizeScale), XSP_32F);
	m_matGeomPixPhyPos.Reshape(1, int32_t(nDetectorLen * pPara->m_fResizeScale), XSP_32F);
	m_matGeomPixCorrPos.Reshape(1, int32_t(nDetectorLen * pPara->m_fResizeScale), XSP_32F);

	m_matGeoDetectionPixelOffsetLut.Reshape(1, nDetectorLen, XSP_16U);

	// 背散射
	if (XRAYLIB_ENERGY_SCATTER == pPara->m_enEnergyMode)
	{
		m_matCaliTableZeroBS.Reshape(1, nDetectorLen, XSP_16U);
		m_matCaliTableAirBS.Reshape(1, nDetectorLen, XSP_16U);
		m_matCaliTableAirBSSecCali_AI.Reshape(1, nDetectorLen, XSP_16U);

		/*初始化本底满载*/
		m_matHighZero.Reshape(1, nDetectorLen, XSP_16U);
		m_matHighFull.Reshape(1, nDetectorLen, XSP_16U);
		m_matLowZero.Reshape(1, nDetectorLen, XSP_16U);
		m_matLowFull.Reshape(1, nDetectorLen, XSP_16U);

		// 校正表路径赋值
		strcpy(m_szGeoCaliTablePath, szPublicFileFolderName);
		strcat(m_szGeoCaliTablePath, "/GeoCaliTable/");
		strcat(m_szGeoCaliTablePath, szGeoCaliTableName);

		hr = ReadGeoCaliTableBS(m_szGeoCaliTablePath, m_caliBSGeoTbe, nDetectorLen);
		if (XRAY_LIB_OK != hr)
		{
			/* 读取文件失败仅关闭校正功能 */
			log_warn("bs geometry cali func is turned off");
			m_bGeoCaliCanUse = false;
		}

		strcpy(m_szGeometryPath, szPublicFileFolderName);
		strcat(m_szGeometryPath, "/GeoCaliTable/");
		strcat(m_szGeometryPath, szGeometryName);

		hr = InitGeoMetricTbe(nDetectorLen);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "InitGeoMetricTbe Failed.");
	}
	else // 透射
	{
		/*初始化本底满载*/
		m_matHighZero.Reshape(1, nDetectorLen, XSP_16U);
		m_matHighFull.Reshape(1, nDetectorLen, XSP_16U);
		m_matLowZero.Reshape(1, nDetectorLen, XSP_16U);
		m_matLowFull.Reshape(1, nDetectorLen, XSP_16U);

		/*初始化冷热源临时内存*/
		m_stCaliHTemp.orginMean.Reshape(1, nDetectorLen, XSP_16U);
		m_stCaliLTemp.orginMean.Reshape(1, nDetectorLen, XSP_16U);

		/*获取校正表名*/
		hr = InitFilePath(szPublicFileFolderName, szPrivateFileFolderName, 
						szGeoCaliTableName, szFlatDetCaliTableName, szGeometryName);
		XSP_CHECK(XRAY_LIB_OK == hr, hr,"Get calitablename err.");

		/*读取校正表*/
		hr = InitTable(nDetectorLen);
		XSP_CHECK(XRAY_LIB_OK == hr, hr,"Read calitable err.");

		hr = InitGeoMetricTbe(nDetectorLen);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "InitGeoMetricTbe Failed.");
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::GetMemSize(XRAY_LIB_MEM_TAB &MemTab, XRAY_LIB_ABILITY &ability)
{
	MemTab.size = 0;

	int32_t nDetectorWidth = ability.nDetectorWidth;
	int32_t nMaxHeightRealTime = ability.nMaxHeightRealTime;
	float32_t fScale = ability.fResizeScale;

	int32_t nRTTBHeightMax = (nMaxHeightRealTime + XRAY_LIB_MAX_TCPROCESS_LENGTH * 2) * XRAY_LIB_MAX_RESIZE_FACTOR;
	int32_t nRTWidthMax = nDetectorWidth * XRAY_LIB_MAX_RESIZE_FACTOR;

	int32_t nMaxRtSliceHeight = XRAY_LIB_MAX_RT_SLICE_HEIGHT + XRAY_LIB_MAX_FILTER_KERNEL_LENGTH;
	int32_t nMaxNLMeansWidth = nDetectorWidth + XRAY_LIB_MAX_FILTER_KERNEL_LENGTH;

	// 背散射校正相关内存申请
	if (XRAYLIB_ENERGY_SCATTER == ability.nEnergyMode)
	{
		m_matCaliTableZeroBS.SetMem(nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_matCaliTableZeroBS.data, m_matCaliTableZeroBS.Size(), MemTab);

		m_matCaliTableAirBS.SetMem(nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_matCaliTableAirBS.data, m_matCaliTableAirBS.Size(), MemTab);

		m_matCaliTableAirBSSecCali_AI.SetMem(nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_matCaliTableAirBSSecCali_AI.data, m_matCaliTableAirBSSecCali_AI.Size(), MemTab);

		/* 扩边前的临时高低能数据, 背散当作临时内存使用 */
		m_stCaliHTemp.caliTemp0.SetMem(nDetectorWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliHTemp.caliTemp0.data, m_stCaliHTemp.caliTemp0.Size(), MemTab);

		m_stCaliLTemp.caliTemp0.SetMem(nDetectorWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliLTemp.caliTemp0.data, m_stCaliLTemp.caliTemp0.Size(), MemTab);

		m_stCaliHTemp.caliTemp1.SetMem(nDetectorWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliHTemp.caliTemp1.data, m_stCaliHTemp.caliTemp1.Size(), MemTab);

		m_stCaliLTemp.caliTemp1.SetMem(nDetectorWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliLTemp.caliTemp1.data, m_stCaliLTemp.caliTemp1.Size(), MemTab);

		m_caliBSGeoTbe.SetMem(nDetectorWidth);
		XspMalloc((void**)&m_caliBSGeoTbe.table, m_caliBSGeoTbe.Size(), MemTab);

		// NLMeans使用内存
		m_matCaliV1.SetMem(nMaxNLMeansWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_matCaliV1.data, m_matCaliV1.Size(), MemTab);

		m_matCaliV2.SetMem(nMaxNLMeansWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_matCaliV2.data, m_matCaliV2.Size(), MemTab);

		m_matCaliV2v.SetMem(nMaxNLMeansWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_matCaliV2v.data, m_matCaliV2v.Size(), MemTab);

		m_matCaliSd.SetMem(nMaxNLMeansWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_32U));
		XspMalloc((void**)&m_matCaliSd.data, m_matCaliSd.Size(), MemTab);

		m_matCalifAvg.SetMem(nMaxNLMeansWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_32F));
		XspMalloc((void**)&m_matCalifAvg.data, m_matCalifAvg.Size(), MemTab);

		m_matCalifWeight.SetMem(nMaxNLMeansWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_32F));
		XspMalloc((void**)&m_matCalifWeight.data, m_matCalifWeight.Size(), MemTab);

		m_matCalifWMax.SetMem(nMaxNLMeansWidth * nMaxRtSliceHeight * XSP_ELEM_SIZE(XSP_32F));
		XspMalloc((void**)&m_matCalifWMax.data, m_matCalifWMax.Size(), MemTab);

		m_stBSCache.mRingBuf.SetMem( 2 * (2 * nMaxHeightRealTime + XRAY_LIB_MAX_TCPROCESS_LENGTH) * 2 * nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stBSCache.mRingBuf.data, m_stBSCache.mRingBuf.Size(), MemTab);

	}
	else	// 透射相关内存申请
	{
		m_matCorrectTemp.SetMem(nDetectorWidth * XSP_MODCALI_CORRECT_HEIGHT *XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_matCorrectTemp.data, m_matCorrectTemp.Size(), MemTab);

		/* 扩边前的临时高低能数据 */
		m_stCaliHTemp.caliTemp0.SetMem(nDetectorWidth * nMaxHeightRealTime * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliHTemp.caliTemp0.data, m_stCaliHTemp.caliTemp0.Size(), MemTab);

		m_stCaliLTemp.caliTemp0.SetMem(nDetectorWidth * nMaxHeightRealTime * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliLTemp.caliTemp0.data, m_stCaliLTemp.caliTemp0.Size(), MemTab);

		m_stCaliHTemp.caliTemp1.SetMem(nDetectorWidth * nMaxHeightRealTime * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliHTemp.caliTemp1.data, m_stCaliHTemp.caliTemp1.Size(), MemTab);

		m_stCaliLTemp.caliTemp1.SetMem(nDetectorWidth * nMaxHeightRealTime * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliLTemp.caliTemp1.data, m_stCaliLTemp.caliTemp1.Size(), MemTab);

		/* 条带上下扩边环形缓冲池内存申请 */ 
		m_stHPadCache.mRingBuf.SetMem(nRTTBHeightMax * nRTWidthMax * XSP_ELEM_SIZE(XSP_16U));
		m_stLPadCache.mRingBuf.SetMem(nRTTBHeightMax * nRTWidthMax * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stHPadCache.mRingBuf.data, m_stHPadCache.mRingBuf.Size(), MemTab);
		XspMalloc((void**)&m_stLPadCache.mRingBuf.data, m_stLPadCache.mRingBuf.Size(), MemTab);

		m_stOriHPadCache.mRingBuf.SetMem((nMaxHeightRealTime + 2 * XRAY_LIB_MAX_TCPROCESS_LENGTH) * nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
		m_stOriLPadCache.mRingBuf.SetMem((nMaxHeightRealTime + 2 * XRAY_LIB_MAX_TCPROCESS_LENGTH) * nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stOriHPadCache.mRingBuf.data, m_stOriHPadCache.mRingBuf.Size(), MemTab);
		XspMalloc((void**)&m_stOriLPadCache.mRingBuf.data, m_stOriLPadCache.mRingBuf.Size(), MemTab);

		m_stCaliHPadCache.mRingBuf.SetMem((nMaxHeightRealTime + 2 * XRAY_LIB_MAX_TCPROCESS_LENGTH) * nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
		m_stCaliLPadCache.mRingBuf.SetMem((nMaxHeightRealTime + 2 * XRAY_LIB_MAX_TCPROCESS_LENGTH) * nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliHPadCache.mRingBuf.data, m_stCaliHPadCache.mRingBuf.Size(), MemTab);
		XspMalloc((void**)&m_stCaliLPadCache.mRingBuf.data, m_stCaliLPadCache.mRingBuf.Size(), MemTab);

		m_stMaskCache.mRingBuf.SetMem(nRTTBHeightMax * nRTWidthMax * XSP_ELEM_SIZE(XSP_8U));
		XspMalloc((void**)&m_stMaskCache.mRingBuf.data, m_stMaskCache.mRingBuf.Size(), MemTab);

		/*平铺校正表*/
		m_stCaliFlatTbe.SetMem(nDetectorWidth);
		XspMalloc((void**)&m_stCaliFlatTbe.table, m_stCaliFlatTbe.Size(), MemTab);
		
		/* 不一致性校正表 */ 
		m_stInconsistCaliTbe.SetMem(nDetectorWidth);
		XspMalloc((void**)&m_stInconsistCaliTbe.pTable, m_stInconsistCaliTbe.Size(), MemTab);
		
		/* 冷热源临时内存 */
		m_stCaliHTemp.orginMean.SetMem(nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliHTemp.orginMean.data, m_stCaliHTemp.orginMean.Size(), MemTab);

		m_stCaliLTemp.orginMean.SetMem(nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
		XspMalloc((void**)&m_stCaliLTemp.orginMean.data, m_stCaliLTemp.orginMean.Size(), MemTab);
	}

	/*几何校正表*/
	m_caliGeoTbe.SetMem(nDetectorWidth);
	XspMalloc((void**)&m_caliGeoTbe.table, m_caliGeoTbe.Size(), MemTab);

	/* 几何校正表计算内存 */
	m_matGeomPixPhyLen.SetMem(nDetectorWidth * fScale * XSP_ELEM_SIZE(XSP_32FC1));
	XspMalloc((void**)&m_matGeomPixPhyLen.data, m_matGeomPixPhyLen.Size(), MemTab);

	m_matGeomPixPhyPos.SetMem(nDetectorWidth * fScale * XSP_ELEM_SIZE(XSP_32FC1));
	XspMalloc((void**)&m_matGeomPixPhyPos.data, m_matGeomPixPhyPos.Size(), MemTab);

	m_matGeomPixCorrPos.SetMem(nDetectorWidth * fScale * XSP_ELEM_SIZE(XSP_32FC1));
	XspMalloc((void**)&m_matGeomPixCorrPos.data, m_matGeomPixCorrPos.Size(), MemTab);

	/*几何校正坐标查找表*/
	m_matGeoDetectionPixelOffsetLut.SetMem(nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
	XspMalloc((void**)&m_matGeoDetectionPixelOffsetLut.data, m_matGeoDetectionPixelOffsetLut.Size(), MemTab);

	/* 满载本底 */
	m_matHighZero.SetMem(nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
	XspMalloc((void**)&m_matHighZero.data, m_matHighZero.Size(), MemTab);

	m_matHighFull.SetMem(nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
	XspMalloc((void**)&m_matHighFull.data, m_matHighFull.Size(), MemTab);

	m_matLowZero.SetMem(nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
	XspMalloc((void**)&m_matLowZero.data, m_matLowZero.Size(), MemTab);

	m_matLowFull.SetMem(nDetectorWidth * XSP_ELEM_SIZE(XSP_16U));
	XspMalloc((void**)&m_matLowFull.data, m_matLowFull.Size(), MemTab);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::InitFilePath(const char* szPublicFileFolderName, 
	                                      const char* szPrivateFileFolderName,
	                                      const char* szGeoCaliTableName,
	                                      const char* szFlatDetCaliTableName, 
	                                      const char* szGeometryName)
{
	XSP_CHECK(szPublicFileFolderName, XRAY_LIB_NULLPTR, "Public path is null.");
	XSP_CHECK(szPrivateFileFolderName, XRAY_LIB_NULLPTR, "Private path is null.");
	XSP_CHECK(szGeoCaliTableName, XRAY_LIB_NULLPTR, "GeoCaliTable name is null.");
	XSP_CHECK(szFlatDetCaliTableName, XRAY_LIB_NULLPTR, "FlatDetCali name is null.");
	XSP_CHECK(szGeometryName, XRAY_LIB_NULLPTR, "Geometry name is null.");

	/******************************************************
	*                    路径初始化
	*******************************************************/
	int32_t nPathLength = XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME;

	memset(m_szInconsistCaliTablePath, 0, sizeof(int8_t) * nPathLength);
	memset(m_szGeoCaliTablePath, 0, sizeof(int8_t) * nPathLength);
	memset(m_szGeometryPath, 0, sizeof(int8_t) * nPathLength);
	memset(m_szFlatDetCaliTablePath, 0, sizeof(int8_t) * nPathLength);
	

	/******************************************************
	*                     路径赋值
	*******************************************************/
	/* 超低灰度不一致归一化校正表路径 */
	strcpy(m_szInconsistCaliTablePath, szPublicFileFolderName);
	strcat(m_szInconsistCaliTablePath, "/FactoryCaliTable/");
	strcat(m_szInconsistCaliTablePath, "InconsistCaliTable.tbe");

	/* GeoCaliTable路径 */
	strcpy(m_szGeoCaliTablePath, szPublicFileFolderName);
	strcat(m_szGeoCaliTablePath, "/GeoCaliTable/");
	strcat(m_szGeoCaliTablePath, szGeoCaliTableName);
	log_info("geoCaliTable path : %s", m_szGeoCaliTablePath);

	/* Geometry路径 */
	strcpy(m_szGeometryPath, szPublicFileFolderName);
	strcat(m_szGeometryPath, "/GeoCaliTable/");
	strcat(m_szGeometryPath, szGeometryName);
	log_info("geometry path : %s", m_szGeometryPath);
	
	/* FlatDetCali路径*/
	strcpy(m_szFlatDetCaliTablePath, szPublicFileFolderName);
	strcat(m_szFlatDetCaliTablePath, "/FlatDetCaliTable/");
	strcat(m_szFlatDetCaliTablePath, szFlatDetCaliTableName);
	log_info("FlatDetCaliTable path : %s", m_szFlatDetCaliTablePath);
	
	return XRAY_LIB_OK;
}

// 将生成的intpLut校正表，转换为GeoCaliTable形式
auto transIntpLut2CaliTable(GeoCaliTable& stTable, std::vector<isl::px_intp_t>& intpLut)
{
    // 保留校正表头的信息，从intpLut中重写校正表
    stTable.nIdxRange = intpLut.size();
    
    for (int32_t i = 0; i < stTable.nIdxRange; i++) 
    {
        int32_t idxnum = 0;
        // 计算intpLut[i]中的索引数量
        for (auto it = intpLut[i].begin(); it != intpLut[i].end(); it++) 
        {
            // 取出索引值和权重
            stTable.IdxPtr(i)[idxnum] = it->first;
            stTable.RatioPtr(i)[idxnum] = it->second;
			idxnum++;
        }
        stTable.SetIdxNum(i, idxnum);
    }
}

// 将现有的校正表转换为像素的位置
auto transCaliTable2Pos(GeoCaliTable& stTable, std::vector<double>& pxPos)
{
	for (int32_t i = 0; i < stTable.nIdxRange; i++) 
	{
		double center = 0.0;
		for (int32_t j = 0; j < stTable.IdxNum(i); j++) 
		{
			center += stTable.IdxPtr(i)[j] * stTable.RatioPtr(i)[j];
		}
		pxPos.push_back(center);
	}
}

XRAY_LIB_HRESULT CaliModule::InitGeoMetricTbe(int32_t nDetectorLen)
{
	XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
	/* note: 1. 民航几何光路可用时畸变在外部处理, 用于超分辨后畸变校正, 提升分辨率指标 
	 *		 2. 通用只在前处理做畸变校正, 因此只保留原尺寸畸变校正表, 不需要Resize畸变校正表
	 *          a. 几何光路可用时(.json文件存在时), 可设置畸变校正调整系数;
	 *          b. 几何光路不可用时(只存在.bin文件时), 设置畸变校正调整系数无效.
	 * 
	 * tips1: 畸变校正功能使用畸变校正表
	 *          .bin(m_caliGeoTbe的文件形式, 直接read, m_bGeometryFigExist )  ==== m_caliGeoTbe
	 *          .json(光路图, 根据物理坐标及对应映射关系生成畸变校正表, m_bGeometryFigExist) ====> m_caliGeoTbe
	 * 
	 * tips2:  m_bGeoCaliCanUse 表示有畸变校正表可用, 可直接使用畸变校正功能;
	 */
	/******************************************************
	*                 畸变校正表读取
	*******************************************************/
	// 优先读取几何光路图(.json), 可修改畸变校正比例因子;
	if (m_bGeometryFigExist)
	{
		hr = ReadGeometryFromJson(m_szGeometryPath, m_stGeometry);
		if (XRAY_LIB_OK != hr)
		{
			/* 读取文件失败仅关闭畸变功能 */
			log_warn("geometry figure read failed.");
			m_bGeometryFigExist = false;	// 读取失败, 相当于几何光路图不存在
			m_bGeoCaliCanUse = false;	// 未生成畸变校正表
			hr = XRAY_LIB_OK;	// 保证畸变矫正功能失效时可正常初始化
		}
		else
		{
			/* 常规几何校正表 */
			m_stGeometry.nDetPixels = static_cast<int32_t>(m_pSharedPara->m_nPerDetectorWidth);
			hr = ResetGeoCaliTable(m_stGeometry, m_caliGeoTbe, m_pSharedPara->m_nDetectorWidth);
			XSP_CHECK(XRAY_LIB_OK == hr, hr, "init GeoTbe failed.");

			m_bGeoCaliCanUse = true; // 读取成功, 生成畸变校正表
		}
	}

	if (m_bGeoCaliTableExist && !m_bGeoCaliCanUse)	// 几何光路图读取失败, 未生成畸变校正表, 且畸变校正文件存在(.bin), (不可修改比例因子)
	{
		hr = ReadGeoCaliTable(m_szGeoCaliTablePath, m_caliGeoTbe, nDetectorLen);
		if (XRAY_LIB_OK != hr)
		{
			/* 读取文件失败仅关闭畸变功能 */
			log_warn("geocali file read failed.");
			m_bGeoCaliCanUse = false;	// 读取失败, 未生成畸变校正表
			m_bGeoCaliTableExist = false; // 相当于.bin文件不存在
		}
		else
		{
			m_bGeoCaliCanUse = true;	// 读取成功, 生成畸变校正表
		}
	}

	return hr;
}

XRAY_LIB_HRESULT CaliModule::InitTable(int32_t nDetectorLen)
{
	XRAY_LIB_HRESULT hr;
	hr = InitGeoMetricTbe(nDetectorLen);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "InitGeoMetricTbe Failed.");

	// 没有畸变校正表也可以正常过包出图, 但是畸变校正功能无法开启, 初始化打印提示
	log_warn("geocalitbe generate %s, geo factor %s be redify.", m_bGeoCaliCanUse ? "successful" : "fail", 
				m_bGeometryFigExist ? "can" : "can't");

	/******************************************************
	*                 平铺校正表读取
	*******************************************************/
	if (m_bFlatDetCaliCanUse)
	{
		hr = ReadFlatDetCaliTable(m_szFlatDetCaliTablePath, m_stCaliFlatTbe, nDetectorLen);
		if (XRAY_LIB_OK != hr)
		{
			/* 读取文件失败仅关闭平铺功能 */
			log_warn("flat cali func is turned off");
			m_bFlatDetCaliCanUse = false;
		}
	}

	/******************************************************
	*               低灰度不一致性校正读取
	*******************************************************/
	hr = ReadInconsistencyCaliTable(m_szInconsistCaliTablePath, m_stInconsistCaliTbe, nDetectorLen);
	if (XRAY_LIB_OK != hr)
	{
		/* 读取文件失败仅关闭不一致性校正功能 */
		log_warn("Inconsistency cali func is turned off file path: %s", m_szInconsistCaliTablePath);
		m_bInconsistencyCaliCanUse = false;
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::ReadGeoCaliTableBS(const char* szFileName, GeoCaliTable &stTable, int32_t nDetectorWidth)
{
	XSP_CHECK(szFileName, XRAY_LIB_NULLPTR, "GeoCaliTable path is null.");
	XSP_CHECK(stTable.table, XRAY_LIB_NULLPTR, "GeoCaliTable data is null.");

	size_t ReadSize;
	FILE* file = LIBXRAY_NULL;

	XRAYLIB_GEOCALI_HEADER stHeader;
	file = fopen(szFileName, "rb");
	XSP_CHECK(file, XRAY_LIB_OPENFILE_FAIL, "Open GeoCaliTable failed.");

	/*文件总大小有效性判断*/
	ReadSize = fread(&stHeader, sizeof(XRAYLIB_GEOCALI_HEADER), 1, file);
	if (ReadSize != 1)
	{
		fclose(file);
		log_warn("read geometry file failed");
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	if (stHeader.usPixelNumber != nDetectorWidth)
	{
		fclose(file);
		log_warn("geom detec num : %d, input num %d", stHeader.usPixelNumber, nDetectorWidth);
		return XRAY_LIB_PARAM_WIDTH_ERROR;
	}
	memset(stTable.table, 0, sizeof(uint16_t)*stHeader.usPixelNumber * stHeader.usIndexPerPixel);

	ReadSize = fread((uint16_t*)stTable.table, sizeof(uint16_t), stHeader.usPixelNumber*stHeader.usIndexPerPixel, file);

	if (ReadSize != (size_t)stHeader.usPixelNumber * stHeader.usIndexPerPixel)
	{
		fclose(file);
		log_warn("read geometry file failed");
		return XRAY_LIB_PARAM_WIDTH_ERROR;
	}

	fclose(file);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::ReadGeoCaliTable(const char* szFileName, GeoCaliTable &stTable, int32_t nDetectorWidth)
{
	XSP_CHECK(szFileName, XRAY_LIB_NULLPTR, "GeoCaliTable path is null.");
	XSP_CHECK(stTable.table, XRAY_LIB_NULLPTR, "GeoCaliTable data is null.");

	size_t ReadSize;
	FILE* file = LIBXRAY_NULL;

	XRAYLIB_GEOCALI_HEADER stHeader;
	file = fopen(szFileName, "rb");
	XSP_CHECK(file, XRAY_LIB_OPENFILE_FAIL, "Open GeoCaliTable failed.");

	/*文件总大小有效性判断*/
	ReadSize = fread(&stHeader, sizeof(XRAYLIB_GEOCALI_HEADER), 1, file);
	if (ReadSize != 1)
	{
		fclose(file);
		log_warn("read geometry file failed");
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	if (stHeader.usPixelNumber != nDetectorWidth)
	{
		fclose(file);
		log_warn("geom detec num : %d, input num %d", stHeader.usPixelNumber, nDetectorWidth);
		return XRAY_LIB_PARAM_WIDTH_ERROR;
	}
	memset(stTable.table, 0, sizeof(double)*stHeader.usPixelNumber * stHeader.usIndexPerPixel);

	ReadSize = fread(stTable.table, sizeof(double), stHeader.usPixelNumber*stHeader.usIndexPerPixel, file);
	if (ReadSize != (size_t)stHeader.usPixelNumber * stHeader.usIndexPerPixel)
	{
		fclose(file);
		log_warn("read geometry file failed");
		return XRAY_LIB_PARAM_WIDTH_ERROR;
	}

	fclose(file);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::GetGeoOffsetLut()
{
	XSP_CHECK(m_matGeoDetectionPixelOffsetLut.Ptr(), XRAY_LIB_NULLPTR, "m_matGeoDetectionPixelOffsetLut is null.");
	XSP_CHECK(m_caliGeoTbe.table, XRAY_LIB_NULLPTR, "GeoCaliTable data is null.");

	int szIdx[m_caliGeoTbe.nIdxRange] = {0};
	double szIdxRatio[m_caliGeoTbe.nIdxRange] = {-10.0f};
	for (int i = 0; i < m_caliGeoTbe.nIdxRange; i++)
	{
		for (int idx = 0; idx < m_caliGeoTbe.IdxNum(i); idx++)
		{
			int nIdx = static_cast<int>(m_caliGeoTbe.IdxPtr(i)[idx]);
			if (szIdxRatio[nIdx] < m_caliGeoTbe.RatioPtr(i)[idx])
			{
				szIdx[nIdx] = i;
				szIdxRatio[nIdx] = m_caliGeoTbe.RatioPtr(i)[idx];
			}
		}
	}

	for (int i = 0; i < m_caliGeoTbe.nIdxRange; i++)
	{
		if (m_bGeoCaliInverse)
		{
			m_matGeoDetectionPixelOffsetLut.Ptr<uint16_t>()[m_caliGeoTbe.nIdxRange - i - 1] = szIdx[i];
		}
		else
		{
			m_matGeoDetectionPixelOffsetLut.Ptr<uint16_t>()[i] = szIdx[i];
		}
		
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::ReadInconsistencyCaliTable(const char* szFileName, InconsistCaliTable& stTable, int32_t nDetectorWidth)
{
	XSP_CHECK(szFileName, XRAY_LIB_NULLPTR, "FlatDetCaliTable path is null.");
	XSP_CHECK(stTable.pTable, XRAY_LIB_NULLPTR, "FlatDetCaliTable data is null.");

	size_t ReadSize;
	FILE* file = LIBXRAY_NULL;

	file = fopen(szFileName, "rb");
	if (!file)
	{
		return XRAY_LIB_OPENFILE_FAIL;
	}
	
	ReadSize = fread(&stTable.hdr, sizeof(InconsistCaliHeader), 1, file);
	if (1 != ReadSize)
	{
		fclose(file);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	if (nDetectorWidth != stTable.hdr.uPixelNumber)
	{
		fclose(file);
		return XRAY_LIB_PARAM_WIDTH_ERROR;
	}

	ReadSize = fread(stTable.pTable, sizeof(uint16_t), stTable.hdr.uPixelNumber * stTable.hdr.uRefLevelNum, file);
	if (ReadSize != (size_t)stTable.hdr.uPixelNumber * stTable.hdr.uRefLevelNum)
	{
		fclose(file);
		return XRAY_LIB_PARAM_WIDTH_ERROR;
	}

	log_info("InconsistencyCaliTable loading successful, table pixel index [%d, %d], ref level num %d, uPixelNumber %d",stTable.hdr.uCaliStartPixel, stTable.hdr.uCaliEndPixel, stTable.hdr.uRefLevelNum, stTable.hdr.uPixelNumber);

	fclose(file);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::ReadFlatDetCaliTable(const char* szFileName, FlatCaliTable& stTable, int32_t nDetectorWidth)
{
	XSP_CHECK(szFileName, XRAY_LIB_NULLPTR, "FlatDetCaliTable path is null.");
	XSP_CHECK(stTable.table, XRAY_LIB_NULLPTR, "FlatDetCaliTable data is null.");

	size_t ReadSize;
	FILE* file = LIBXRAY_NULL;

	XRAYLIB_GEOCALI_HEADER stHeader;
	file = fopen(szFileName, "rb");
	XSP_CHECK(file, XRAY_LIB_OPENFILE_FAIL, "Open FlatDetCaliTable failed.");

	ReadSize = fread(&stHeader, sizeof(XRAYLIB_GEOCALI_HEADER), 1, file);
	if (1 != ReadSize)
	{
		fclose(file);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	if (stHeader.usPixelNumber != nDetectorWidth)
	{
		fclose(file);
		return XRAY_LIB_PARAM_WIDTH_ERROR;
	}

	memset(stTable.table, 0, sizeof(double)*stHeader.usPixelNumber*stHeader.usIndexPerPixel);

	ReadSize = fread(stTable.table, sizeof(double), stHeader.usPixelNumber*stHeader.usIndexPerPixel, file);
	if (ReadSize != (size_t)stHeader.usPixelNumber*stHeader.usIndexPerPixel)
	{
		fclose(file);
		return XRAY_LIB_PARAM_WIDTH_ERROR;
	}

	fclose(file);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::ReadGeometryFromJson(const char* szFileName, Geometry& stGeom)
{
	XSP_CHECK(szFileName, XRAY_LIB_NULLPTR, "Geometry json file path is null.");
	size_t ReadSize;
	FILE* file = LIBXRAY_NULL;

	/* 打开json配置文件 */
	file = fopen(szFileName, "rb");
	XSP_CHECK(file, XRAY_LIB_OPENFILE_FAIL, "Open  file failed.");

	fseek(file, 0, SEEK_END);
	size_t FileSize = ftell(file);

	char* data = (char*)malloc(FileSize * sizeof(char));

	fseek(file, 0, SEEK_SET);

	ReadSize = fread(data, FileSize, 1, file);
	if (1 != ReadSize)
	{
		fclose(file);
		free(data);
		log_error("Read Geometry  File failed.");
		return XRAY_LIB_READFILESIZE_ERROR;
	}
	fclose(file);

	/*  解析 */
	xsp::cJSON *root = xsp::cJSON_Parse(data);
	if (!root)
	{
		free(data);
		log_error("get json root err");
		return XRAY_LIB_INVALID_PARAM;
	}

	/* 视角获取 */
	xsp::cJSON *view = cJSON_GetObjectItemCaseSensitive(root, "view");
	if (!view)
	{
		log_error("get json view err");
		cJSON_Delete(root);
		free(data);
		return XRAY_LIB_INVALID_PARAM;
	}

	if (0 == strcmp(view->valuestring, "main"))
	{
		stGeom.isMain = true;
	}
	else if (0 == strcmp(view->valuestring, "aux"))
	{
		stGeom.isMain = false;
	}
	else
	{
		log_error("json file view err %s", view->valuestring);
		cJSON_Delete(root);
		free(data);
		return XRAY_LIB_INVALID_PARAM;
	}

	/* 探测器坐标获取 */
	xsp::cJSON *dets = cJSON_GetObjectItemCaseSensitive(root, "det_coor");
	if (!dets)
	{
		log_error("get json det_coor err");
		cJSON_Delete(root);
		free(data);
		return XRAY_LIB_INVALID_PARAM;
	}

	int32_t nDetNum = cJSON_GetArraySize(dets);


	if (nDetNum != static_cast<int32_t>(stGeom.GetDetNum()))
	{
		/* 转存情况不读取json配置 */
		log_error("detector num err, json num %d, true num %d", nDetNum, stGeom.GetDetNum());
		cJSON_Delete(root);
		free(data);
		return XRAY_LIB_INVALID_PARAM;
	}

	for (int32_t i = 0; i < nDetNum; i++) 
	{
		xsp::cJSON *coors = xsp::cJSON_GetArrayItem(dets, i);
		if (!coors)
		{
			log_error("get json coors err");
			cJSON_Delete(root);
			free(data);
			return XRAY_LIB_INVALID_PARAM;
		}
		int32_t nCoorNum = cJSON_GetArraySize(coors);

		if (nCoorNum != 4)
		{
			log_error("detector nCoorNum err, nCoorNum %d", nCoorNum);
			cJSON_Delete(root);
			free(data);
			return XRAY_LIB_INVALID_PARAM;
		}

		Vec2 start(float32_t(cJSON_GetArrayItem(coors, 0)->valuedouble), float32_t(cJSON_GetArrayItem(coors, 1)->valuedouble));
		Vec2 end(float32_t(cJSON_GetArrayItem(coors, 2)->valuedouble), float32_t(cJSON_GetArrayItem(coors, 3)->valuedouble));
		stGeom.SetDetCoor(start, end, i);
	}

	log_debug("geometry info: view %d", stGeom.isMain);
	for (uint32_t n = 0; n < stGeom.GetDetNum(); n++)
	{
		DetCoor det = stGeom.GetDetCoor(n);
		log_debug("det[%d] : [%f, %f, %f, %f]", n , det.start.x, det.start.y, det.end.x, det.end.y);
	}

	xsp::cJSON *belt_edge = cJSON_GetObjectItemCaseSensitive(root, "belt_edge");
	if (!belt_edge)
	{
		log_info("get json belt_edge err");
		m_stBeltdge.nStart1 = -1;
		m_stBeltdge.nEnd1 = -1;
		m_stBeltdge.nStart2 = -1;
		m_stBeltdge.nEnd2 = -1;
		cJSON_Delete(root);
		free(data);
		return XRAY_LIB_OK;
	}

	int32_t nBeltEdgeNum = cJSON_GetArraySize(belt_edge);
	if (nBeltEdgeNum != 4)
	{
		log_error("BeltEdgeNum err, nBeltEdgeNum %d", nBeltEdgeNum);
		cJSON_Delete(root);
		free(data);
		return XRAY_LIB_INVALID_PARAM;
	}

	m_stBeltdge.nStart1 = cJSON_GetArrayItem(belt_edge, 0)->valueint;
	m_stBeltdge.nEnd1 = cJSON_GetArrayItem(belt_edge, 1)->valueint;
	m_stBeltdge.nStart2 = cJSON_GetArrayItem(belt_edge, 2)->valueint;
	m_stBeltdge.nEnd2 = cJSON_GetArrayItem(belt_edge, 3)->valueint;

	cJSON_Delete(root);
	free(data);
	return XRAY_LIB_OK;

}

XRAY_LIB_HRESULT CaliModule::ManualUpdateCorrect(XMat& matHigh, XMat& matLow,
	                                           XRAYLIB_UPDATE_ZEROANDFULL_MODE enUpdateMode)
{
	XSP_CHECK(enUpdateMode >= XRAYLIB_UPDATE_ZEROANDFULL_MIN &&
		      enUpdateMode <= XRAYLIB_UPDATE_ZEROANDFULL_MAX,
		      XRAY_LIB_INVALID_PARAM, "Update mode illegal");

	XRAY_LIB_HRESULT hr;
	int32_t nNum = 0;
	double dblTotal;
	
	if (XRAYLIB_ENERGY_SCATTER == m_pSharedPara->m_enEnergyMode)
	{
		XMat matBSTbe, matAirSecAI;

		if (enUpdateMode == XRAYLIB_UPDATEZERO) //更新本底
		{
			matBSTbe = m_matCaliTableZeroBS;
		}
		else if (enUpdateMode == XRAYLIB_UPDATEFULL)//更新空气
		{
			matBSTbe = m_matCaliTableAirBS;
			matAirSecAI = m_matCaliTableAirBSSecCali_AI;
			memset(matAirSecAI.Ptr<uint16_t>(0), 0, sizeof(uint16_t) * matAirSecAI.wid * matAirSecAI.hei);
		}
		else
		{
			return XRAY_LIB_UPDATEAIRANDZERO_MODE_ERROR;
		}

		XSP_CHECK(matLow.IsValid(), XRAY_LIB_XMAT_INVALID);

		if (m_bMirrorBS)
		{
			MirrorUpDownBS_u16(matLow, m_nImageWidthBS);
		}

		int32_t nWidth = matLow.wid;
		int32_t nHeight = matLow.hei;
		uint16_t* ptrLow = matLow.Ptr<uint16_t>(0);
		uint16_t* ptrTbe = matBSTbe.Ptr<uint16_t>(0);

		/* 
		 *	note: 时间轴方向两倍降采样, 探测器方向因为原始数据只有m_nImageWidthBS数据是有效的, 所以只需要处理m_nImageWidthBS即可
		 *  	  但实际的数据宽度仍是nWidth, 因此跳内存时要按照nWidth跳
		 * 
		 */
		for (int32_t nw = 0; nw < nWidth; nw++)
		{
			dblTotal = 0;
			nNum = 0;
			int32_t nImageHeightDownSize = (int32_t)(nHeight / m_fPulsBS);
			for (int32_t nh = 0; nh < nImageHeightDownSize; nh++)
			{
				for (int32_t nt = 0; nt < m_fPulsBS; nt++)
				{
					nNum++;
					int32_t index = (int32_t)((nh * m_fPulsBS + nt) * nWidth + nw);
					dblTotal += (double)ptrLow[index];
				}
			}

			nNum = nNum / m_fPulsBS;

			if (nNum == 0 || dblTotal <= 0)
			{
				ptrTbe[nw] = 0;
			}
			else
			{
				ptrTbe[nw] = (uint16_t)(dblTotal / (double)nNum);
			}
		}
		return hr; 
	}

	XMat matHighTbe, matLowTbe;
	if (enUpdateMode == XRAYLIB_UPDATEZERO) //更新本底
	{
		matHighTbe = m_matHighZero;
		matLowTbe = m_matLowZero;
	}
	else if (enUpdateMode == XRAYLIB_UPDATEFULL)//更新空气
	{
		matHighTbe = m_matHighFull;
		matLowTbe = m_matLowFull;
	}
	else
	{
		return XRAY_LIB_UPDATEAIRANDZERO_MODE_ERROR;
	}

	int32_t nHei, nWid;
	nHei = matHigh.hei;
	nWid = matHigh.wid;

	m_nPreType = 0;
	m_fMaxAirViranceL = 300;
	if (enUpdateMode == XRAYLIB_UPDATEFULL)
	{
		uint16_t* pImgLow = matLow.Ptr<uint16_t>();
		for (int32_t nw = 0; nw < nWid / 2; nw++)
		{
			if (pImgLow[nw] != 0 && (pImgLow[nw] >= 60000 || pImgLow[nw] < 6000))
			{
				m_nTopHeight = nw + 1;
			}
			else
			{
				break;
			}
		}
		for (int32_t nw = nWid - 1; nw > nWid / 2; nw--)
		{
			if (pImgLow[nw] != 0 && (pImgLow[nw] >= 60000 || pImgLow[nw] < 6000))
			{
				m_nDownHeight = nWid - nw;
			}
			else
			{
				break;
			}
		}
		if(m_nTopHeight == nWid / 2)
		{
			m_nTopHeight = 0;
		}
		if (m_nDownHeight == nWid / 2 - 1)
		{
			m_nDownHeight = 0;
		}
	}

	if (XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode ||
		XRAYLIB_ENERGY_HIGH == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(matHigh.IsValid(), XRAY_LIB_XMAT_INVALID);

		for (int32_t nw = 0; nw < nWid; nw++)
		{
			dblTotal = 0;
			nNum = 0;
			for (int32_t nh = 0; nh < nHei; nh++)
			{
				nNum++;
				dblTotal += *(matHigh.Ptr<uint16_t>(nh, nw));
			}

			if (nNum == 0 || dblTotal <= 0)
				matHighTbe.Ptr<uint16_t>()[nw] = 0;
			else
				matHighTbe.Ptr<uint16_t>()[nw] = (uint16_t)(dblTotal / (double)nNum);
		}
	}
	if (XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode ||
		XRAYLIB_ENERGY_LOW == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(matLow.IsValid(), XRAY_LIB_XMAT_INVALID);

		for (int32_t nw = 0; nw < nWid; nw++)
		{
			dblTotal = 0;
			nNum = 0;
			for (int32_t nh = 0; nh < nHei; nh++)
			{
				nNum++;
				dblTotal += *(matLow.Ptr<uint16_t>(nh, nw));
			}

			if (nNum == 0 || dblTotal <= 0)
				matLowTbe.Ptr<uint16_t>()[nw] = 0;
			else
				matLowTbe.Ptr<uint16_t>()[nw] = (uint16_t)(dblTotal / (double)nNum);
		}
	}

	return XRAY_LIB_OK;
	// return SaveCaliTable(nWid);
}


XRAY_LIB_HRESULT CaliModule::AutoUpdateCorrect(XMat& matHigh, XMat& matLow,
	XRAYLIB_UPDATE_ZEROANDFULL_MODE enUpdateMode)
{
	XSP_CHECK(enUpdateMode >= XRAYLIB_UPDATE_ZEROANDFULL_MIN &&
		enUpdateMode <= XRAYLIB_UPDATE_ZEROANDFULL_MAX,
		XRAY_LIB_INVALID_PARAM, "Update mode illegal");


	int32_t nNum = 0;
	double dblTotal;

	XMat matHighTbe, matLowTbe;
	if (enUpdateMode == XRAYLIB_UPDATEZERO) //更新本底
	{
		matHighTbe = m_matHighZero;
		matLowTbe = m_matLowZero;
	}
	else if (enUpdateMode == XRAYLIB_UPDATEFULL)//更新空气
	{
		matHighTbe = m_matHighFull;
		matLowTbe = m_matLowFull;
	}
	else
	{
		return XRAY_LIB_UPDATEAIRANDZERO_MODE_ERROR;
	}

	int32_t nHei, nWid;
	nHei = matHigh.hei;
	nWid = matHigh.wid;

	if (XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode ||
		XRAYLIB_ENERGY_HIGH == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(matHigh.IsValid(), XRAY_LIB_XMAT_INVALID);

		for (int32_t nw = 0; nw < nWid; nw++)
		{
			dblTotal = 0;
			nNum = 0;
			for (int32_t nh = 0; nh < nHei; nh++)
			{
				nNum++;
				dblTotal += *(matHigh.Ptr<uint16_t>(nh, nw));
			}

			if (nNum == 0 || dblTotal <= 0)
				matHighTbe.Ptr<uint16_t>()[nw] = 0;
			else
				matHighTbe.Ptr<uint16_t>()[nw] = (uint16_t)(dblTotal / (double)nNum);
		}
	}
	if (XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode ||
		XRAYLIB_ENERGY_LOW == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(matLow.IsValid(), XRAY_LIB_XMAT_INVALID);

		for (int32_t nw = 0; nw < nWid; nw++)
		{
			dblTotal = 0;
			nNum = 0;
			for (int32_t nh = 0; nh < nHei; nh++)
			{
				nNum++;
				dblTotal += *(matLow.Ptr<uint16_t>(nh, nw));
			}

			if (nNum == 0 || dblTotal <= 0)
				matLowTbe.Ptr<uint16_t>()[nw] = 0;
			else
				matLowTbe.Ptr<uint16_t>()[nw] = (uint16_t)(dblTotal / (double)nNum);
		}
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::BackgroundCali_BS(XMat& matIn, XMat& matOut, XMat& matCaliTbe, float32_t fAntiBS)
{
	XSP_CHECK(matIn.IsValid() && matCaliTbe.IsValid() && matOut.IsValid(), XRAY_LIB_XMAT_INVALID);
	XSP_CHECK(MatSizeEq(matIn, matOut), XRAY_LIB_XMAT_SIZE_ERR);
	XSP_CHECK(XSP_16UC1 == matIn.type && XSP_16UC1 == matCaliTbe.type, XRAY_LIB_XMAT_TYPE_ERR);

	int32_t nWidth = matIn.wid, nHeight = matIn.hei;
	float32_t f32ImgTemp;

	for (int32_t nh = 0; nh < nHeight; nh++)
	{
		uint16_t* ptrImageIn = matIn.Ptr<uint16_t>(nh);
		uint16_t* ptrImageOut = matOut.Ptr<uint16_t>(nh);
		uint16_t* ptrCaliTbe = matCaliTbe.Ptr<uint16_t>(0);
		for (int32_t nw = 0; nw < nWidth; nw++)
		{
			f32ImgTemp = (float32_t)((ptrImageIn[nw] - ptrCaliTbe[nw]) * fAntiBS);
			ptrImageOut[nw] = static_cast<uint16_t>(Clamp(f32ImgTemp, 0.0f, 65535.0f));
		}
	}

	return XRAY_LIB_OK;
}

// 条带状态判断
XRAY_LIB_HRESULT CaliModule::StripeTypeJudge(XMat & matLow)
{
	if (0 == m_nCountThreshold)
	{
		return XRAY_LIB_OK;
	}

	m_TimeEnd = std::chrono::system_clock::now();
	auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(m_TimeEnd);
	auto value = now_ms.time_since_epoch();
	m_lEndTime = value.count(); // 这里得到的是自1970年1月1日以来的毫秒数

	if ((m_lEndTime - m_lBeginTime) > 1500)
	{
		m_nPreType = 0;
		m_fMaxAirViranceL = 300;
	}
	m_lBeginTime = m_lEndTime;

	XSP_CHECK(matLow.IsValid() && m_matLowFull.IsValid() &&
			  m_matLowZero.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == matLow.type && XSP_16U == m_matLowFull.type &&
			  XSP_16U == m_matLowZero.type, XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(m_matLowFull.wid == matLow.wid, XRAY_LIB_XMAT_SIZE_ERR);

	int32_t nWidthIn = matLow.wid;
	int32_t nHeightIn = matLow.hei;
	uint16_t *pOriData, *pDataTemp1, *pDataTemp2;

	int32_t nMaxMeanRow = 0;
	int32_t nMinMeanRow = 0;
	double dfMaxMean = 0.0f;
	double dfMinMean = 65535.0f;
	double dfMean = 0.0f;

	//找到纵向均值最小和最大的两列
	for (int32_t nh = 0; nh < nHeightIn; nh++)
	{
		pOriData = matLow.Ptr<uint16_t>(nh, m_nTopHeight);
		dfMean = 0.0f;
		for (int32_t nw = m_nTopHeight; nw < nWidthIn - m_nDownHeight; nw++)
		{
			dfMean += (double)*pOriData;
			pOriData++;
		}
		dfMean /= (nWidthIn - m_nDownHeight - m_nTopHeight);
		if (dfMean > dfMaxMean)
		{
			dfMaxMean = dfMean;
			nMaxMeanRow = nh;
		}
		if (dfMean < dfMinMean)
		{
			dfMinMean = (int32_t)dfMean;
			nMinMeanRow = nh;
		}
	}

	if (nMaxMeanRow >= nMinMeanRow)
	{
		nMaxMeanRow = 0;
	}

	//特征1：行数据一致性，计算每行数据的标准差，得到标准差最大值dfMaxVariance，以及对应行数nMaxChangePos
	double dfVariance = 0.0f;
	double dfMaxVariance = -1.0f;
	int32_t nMaxChangePos = 0;
	pDataTemp1 = m_stCaliHTemp.orginMean.Ptr<uint16_t>() + m_nTopHeight;
	for (int32_t nw = m_nTopHeight; nw < nWidthIn - m_nDownHeight; nw++)
	{
		pOriData = matLow.Ptr<uint16_t>(0, nw);
		dfMean = 0.0f;
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			dfMean += (double)*pOriData;
			pOriData += nWidthIn;
		}
		dfMean /= nHeightIn;
		*pDataTemp1 = (uint16_t)dfMean;
		pDataTemp1++;
	}

	pDataTemp1 = m_stCaliHTemp.orginMean.Ptr<uint16_t>() + m_nTopHeight;
	for (int32_t nw = m_nTopHeight; nw < nWidthIn - m_nDownHeight; nw++)
	{
		pOriData = matLow.Ptr<uint16_t>(0, nw);
		dfVariance = 0.0f;
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			dfVariance += abs(*pDataTemp1 - *pOriData);
			pOriData += nWidthIn;
		}
		pDataTemp1++;
		dfVariance /= nHeightIn;
		if (dfVariance > dfMaxVariance)
		{
			dfMaxVariance = dfVariance;
			nMaxChangePos = nw;
		}
	}
	if (nMaxChangePos - 25 < m_nTopHeight)
	{
		nMaxChangePos = 25 + m_nTopHeight;
	}
	if (nMaxChangePos + 26 > nWidthIn - m_nDownHeight)
	{
		nMaxChangePos = nWidthIn - 26 - m_nDownHeight;
	}

	//特征2：行数据一致性，分别计算均值最小列左右两侧每行数据的标准差
	double dfMean1;
	uint16_t* pOriTemp1, *pOriTemp2;
	pDataTemp1 = m_stCaliHTemp.orginMean.Ptr<uint16_t>() + m_nTopHeight;
	pDataTemp2 = m_stCaliLTemp.orginMean.Ptr<uint16_t>() + m_nTopHeight;
	for (int32_t nw = m_nTopHeight; nw < nWidthIn - m_nDownHeight; nw++)
	{
		pOriTemp1 = matLow.Ptr<uint16_t>(0, nw);
		dfMean = 0.0f;
		dfMean1 = 0.0f;
		for (int32_t nh = 0; nh < nMinMeanRow - 1; nh++)
		{
			dfMean += (double)*pOriTemp1;
			pOriTemp1 += nWidthIn;
		}
		if (nMinMeanRow + 1 < nHeightIn)
		{
			pOriTemp2 = matLow.Ptr<uint16_t>(nMinMeanRow + 1, nw);
			for (int32_t nh = nMinMeanRow + 1; nh < nHeightIn; nh++)
			{
				dfMean1 += (double)*pOriTemp2;
				pOriTemp2 += nWidthIn;
			}
		}

		if (nMinMeanRow - 1 > 0)
		{
			dfMean /= (nMinMeanRow - 1);
		}
		if (nHeightIn - nMinMeanRow - 1 > 0)
		{
			dfMean1 /= (nHeightIn - nMinMeanRow - 1);
		}
		*pDataTemp1 = (uint16_t)dfMean;
		pDataTemp1++;
		*pDataTemp2 = (uint16_t)dfMean1;
		pDataTemp2++;
	}

	double dfVarianceL = 0.0f;
	double dfVarianceR = 0.0f;
	double dfMaxVarianceL = -1.0f;
	double dfMaxVarianceR = -1.0f;
	pDataTemp1 = m_stCaliHTemp.orginMean.Ptr<uint16_t>() + m_nTopHeight;
	pDataTemp2 = m_stCaliLTemp.orginMean.Ptr<uint16_t>() + m_nTopHeight;
	for (int32_t nw = m_nTopHeight; nw < nWidthIn - m_nDownHeight; nw++)
	{
		dfVarianceL = 0.0f;
		dfVarianceR = 0.0f;
		pOriTemp1 = matLow.Ptr<uint16_t>(0, nw);
		for (int32_t nh = 0; nh < nMinMeanRow - 1; nh++)
		{
			dfVarianceL += abs(*pDataTemp1 - *pOriTemp1);
			pOriTemp1 += nWidthIn;
		}
		if (nMinMeanRow + 1 < nHeightIn)
		{
			pOriTemp2 = matLow.Ptr<uint16_t>(nMinMeanRow + 1, nw);
			for (int32_t nh = nMinMeanRow + 1; nh < nHeightIn; nh++)
			{
				dfVarianceR += abs(*pDataTemp2 - *pOriTemp2);
				pOriTemp2 += nWidthIn;
			}
		}
		if (nMinMeanRow - 1 > 0)
		{
			dfVarianceL /= (nMinMeanRow - 1);
		}
		if (nHeightIn - nMinMeanRow - 1 > 0)
		{
			dfVarianceR /= (nHeightIn - nMinMeanRow - 1);
		}
		if (dfVarianceL > dfMaxVarianceL)
		{
			dfMaxVarianceL = dfVarianceL;
		}
		if (dfVarianceR > dfMaxVarianceR)
		{
			dfMaxVarianceR = dfVarianceR;
		}
		pDataTemp1++;
		pDataTemp2++;
	}


	float32_t fTotalTop, fRealAirTop, fRatioTotalTop1, fRatioTotalTop2;
	int32_t nNumTop;
	nNumTop = 0; fTotalTop = 0.0f; fRatioTotalTop1 = 0.0f; fRatioTotalTop2 = 0.0f; 

	uint16_t* pCaliTableAirL;
	uint16_t* pCaliTableBkL;

	//特征4：均值最大一列和均值最小一列变化最剧烈一行附近均值变化情况（打火逻辑）
	float32_t fRefChange1 = 0.0f;
	float32_t fRefChange2 = 0.0f;
	pDataTemp1 = matLow.Ptr<uint16_t>() + nWidthIn * nMaxMeanRow + nMaxChangePos - 10;
	if (0 == nMinMeanRow)
	{
		pDataTemp2 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1) + nMaxChangePos - 10;
	}
	else
	{
		pDataTemp2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow + nMaxChangePos - 10;
	}
	pCaliTableAirL = m_matLowFull.Ptr<uint16_t>() + nMaxChangePos - 10;
	pCaliTableBkL = m_matLowZero.Ptr<uint16_t>() + nMaxChangePos - 10;
	nNumTop = 0;
	fRatioTotalTop1 = 0.0f;
	fRatioTotalTop2 = 0.0f;
	for (int32_t nw = nMaxChangePos - 10; nw < nMaxChangePos + 10; nw++)
	{
		fRealAirTop = (*pCaliTableAirL - *pCaliTableBkL);
		fTotalTop = (float32_t)*pCaliTableAirL / *pCaliTableBkL;
		if (fRealAirTop == 0)
		{
			fRealAirTop = 1;
		}
		if (fTotalTop > 1.05)
		{
			nNumTop++;
			fRatioTotalTop1 += (float32_t)(*pDataTemp1 - *pCaliTableBkL) / fRealAirTop;
			fRatioTotalTop2 += (float32_t)(*pDataTemp2 - *pCaliTableBkL) / fRealAirTop;
		}
		pDataTemp1++;
		pDataTemp2++;
		pCaliTableAirL++;
		pCaliTableBkL++;
	}
	if (nNumTop == 0)
	{
		nNumTop = 1;
	}
	fRefChange1 = fRatioTotalTop1 / nNumTop;
	fRefChange2 = fRatioTotalTop2 / nNumTop;
	bool bChange = fRefChange1 > fRefChange2 + 0.015;

	//特征5  板卡之间横向均值之差
	float32_t fRealAir;
	float32_t fTemp;
	int32_t nValue;
	double dfGraySum = 0.0f;
	double dfGraySumPre = 0.0f;
	pCaliTableAirL = m_matLowFull.Ptr<uint16_t>() + 63;
	pCaliTableBkL = m_matLowZero.Ptr<uint16_t>() + 63;
	int32_t nChangeNum = 0;
	for (int32_t nw = 63; nw < nWidthIn - 64; nw += 64)
	{
		if (nw < m_nTopHeight || nw >= nWidthIn - m_nDownHeight - 1)
		{
			pCaliTableAirL += 64;
			pCaliTableBkL += 64;
			continue;
		}
		dfGraySum = 0.0f;
		dfGraySumPre = 0.0f;
		pDataTemp1 = matLow.Ptr<uint16_t>(0, nw);
		pDataTemp2 = matLow.Ptr<uint16_t>(0, nw + 1);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			fRealAir = (*pCaliTableAirL - *pCaliTableBkL);
			if (fRealAir == 0)
			{
				fRealAir = 1;
			}
			fTemp = (float32_t)(*pDataTemp1 - *pCaliTableBkL) / fRealAir;
			nValue = (int32_t)(fTemp*0.97f*65535.0f);
			if (nValue < 0)
			{
				nValue = 0;
			}
			if (nValue > 65535)
			{
				nValue = 65535;
			}
			dfGraySum += nValue;
			pDataTemp1 += nWidthIn;
			fRealAir = (*(pCaliTableAirL + 1) - *(pCaliTableBkL + 1));
			if (fRealAir == 0)
			{
				fRealAir = 1;
			}
			fTemp = (float32_t)(*pDataTemp2 - *(pCaliTableBkL + 1)) / fRealAir;
			nValue = (int32_t)(fTemp*0.97f*65535.0f);
			if (nValue < 0)
			{
				nValue = 0;
			}
			if (nValue > 65535)
			{
				nValue = 65535;
			}
			dfGraySumPre += nValue;
			pDataTemp2 += nWidthIn;
		}
		dfGraySumPre = dfGraySumPre / nHeightIn;
		dfGraySum = dfGraySum / nHeightIn;
		if (fabs(dfGraySum - dfGraySumPre) > 1500.0f)
		{
			nChangeNum++;
		}
		pCaliTableAirL += 64;
		pCaliTableBkL += 64;
	}

	//特征6 归一化后积分时间方向均值最小值与最大值比值
	float32_t fMaxAvgGray = 0.0f;
	float32_t fMinAvgGray = 65535.0f;
	pCaliTableAirL = m_matLowFull.Ptr<uint16_t>() + m_nTopHeight;
	pCaliTableBkL = m_matLowZero.Ptr<uint16_t>() + m_nTopHeight;
	for (int32_t nw = m_nTopHeight; nw < nWidthIn - m_nDownHeight; nw++)
	{
		dfGraySum = 0.0f;
		pOriData = matLow.Ptr<uint16_t>(0, nw);
		fRealAir = (*pCaliTableAirL - *pCaliTableBkL);
		if (fRealAir == 0)
		{
			fRealAir = 1;
		}
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			fTemp = (float32_t)(*pOriData - *pCaliTableBkL) / fRealAir;
			nValue = (int32_t)(fTemp*0.97f*65535.0f);
			if (nValue < 0)
			{
				nValue = 0;
			}
			if (nValue > 65535)
			{
				nValue = 65535;
			}
			dfGraySum += nValue;
			pOriData += nWidthIn;
		}
		dfGraySum = dfGraySum / nHeightIn;
		if (dfGraySum > fMaxAvgGray && fabs(dfGraySum - 65535) > 1E-6)
		{
			fMaxAvgGray = dfGraySum;
		}
		if (dfGraySum < fMinAvgGray && fabs(dfGraySum) > 1E-6 && fabs(dfGraySum - 65535) > 1E-6)
		{
			fMinAvgGray = dfGraySum;
		}
		pCaliTableAirL++;
		pCaliTableBkL++;
	}
	float32_t fRatio = fMinAvgGray / fMaxAvgGray;

	//指标7  连续三行以上小于阈值，用于判断薄包带
	pCaliTableAirL = m_matLowFull.Ptr<uint16_t>() + m_nTopHeight;
	pCaliTableBkL = m_matLowZero.Ptr<uint16_t>() + m_nTopHeight;
	pOriTemp1 = matLow.Ptr<uint16_t>(0, m_nTopHeight);
	pOriTemp2 = matLow.Ptr<uint16_t>(1, m_nTopHeight);
	pDataTemp1 = m_stCaliHTemp.orginMean.Ptr<uint16_t>() + m_nTopHeight;
	pDataTemp2 = m_stCaliLTemp.orginMean.Ptr<uint16_t>() + m_nTopHeight;
	uint16_t pix1, pix2, pix3, pix4, pix5;
	bool bPack = false;
	for (int32_t nw = m_nTopHeight; nw < nWidthIn - m_nDownHeight; nw++)
	{
		fRealAir = (*pCaliTableAirL - *pCaliTableBkL);
		if (fRealAir == 0)
		{
			fRealAir = 1;
		}
		fTemp = (float32_t)(*pOriTemp1 - *pCaliTableBkL) / fRealAir;
		nValue = (int32_t)(fTemp*0.97f*65535.0f);
		if (nValue < 0)
		{
			nValue = 0;
		}
		if (nValue > 65535)
		{
			nValue = 65535;
		}
		*pDataTemp1 = (uint16_t)nValue;
		fTemp = (float32_t)(*pOriTemp2 - *pCaliTableBkL) / fRealAir;
		nValue = (int32_t)(fTemp*0.97f*65535.0f);
		if (nValue < 0)
		{
			nValue = 0;
		}
		if (nValue > 65535)
		{
			nValue = 65535;
		}
		*pDataTemp2 = (uint16_t)nValue;
		pOriTemp1++;
		pOriTemp2++;
		pCaliTableAirL++;
		pCaliTableBkL++;
		pDataTemp1++;
		pDataTemp2++;

	}
	pDataTemp1 = m_stCaliHTemp.orginMean.Ptr<uint16_t>() + 2;
	pDataTemp2 = m_stCaliLTemp.orginMean.Ptr<uint16_t>() + 2;
	for (int32_t nw = m_nTopHeight + 2; nw < nWidthIn - m_nDownHeight - 2; nw++)
	{
		pix1 = *(pDataTemp1 - 2 + m_nTopHeight);
		pix2 = *(pDataTemp1 - 1 + m_nTopHeight);
		pix3 = *(pDataTemp1 + m_nTopHeight);
		pix4 = *(pDataTemp1 + 1 + m_nTopHeight);
		pix5 = *(pDataTemp1 + 2 + m_nTopHeight);
		/* 统计3个像素中小于背景灰度阈值的个数 */
		uint16_t columnFiveMask = (pix1 < (uint16_t)62000) +
			(pix2 < (uint16_t)62000) + (pix3 < (uint16_t)62000) +
			(pix4 < (uint16_t)62000) + (pix5 < (uint16_t)62000);
		if (columnFiveMask >= 3)
		{
			pix1 = *(pDataTemp2 - 2 + m_nTopHeight);
			pix2 = *(pDataTemp2 - 1 + m_nTopHeight);
			pix3 = *(pDataTemp2 + m_nTopHeight);
			pix4 = *(pDataTemp2 + 1 + m_nTopHeight);
			pix5 = *(pDataTemp2 + 2 + m_nTopHeight);
			/* 统计3个像素中小于背景灰度阈值的个数 */
			columnFiveMask = (pix1 < (uint16_t)62000) +
				(pix2 < (uint16_t)62000) + (pix3 < (uint16_t)62000) +
				(pix4 < (uint16_t)62000) + (pix5 < (uint16_t)62000);
			if (columnFiveMask >= 3)
			{
				bPack = true;
				break;
			}
		}
		pDataTemp1++;
		pDataTemp2++;
	}

	pDataTemp1 = m_stCaliHTemp.orginMean.Ptr<uint16_t>() + m_nTopHeight;
	int32_t nUpNum = 0;
	int32_t nDownNum = 0;
	for (int32_t nw = m_nTopHeight; nw < m_nTopHeight + 64 && nw < nWidthIn - m_nDownHeight; nw++)
	{
		if (*pDataTemp1 < 62000)
		{
			nUpNum++;
		}
		pDataTemp1++;
	}
	pDataTemp1 = m_stCaliHTemp.orginMean.Ptr<uint16_t>() + nWidthIn - m_nDownHeight - 64;
	for (int32_t nw = nWidthIn - m_nDownHeight - 64; nw > 0 && nw < nWidthIn - m_nDownHeight; nw++)
	{
		if (*pDataTemp1 < 62000)
		{
			nDownNum++;
		}
		pDataTemp1++;
	}
	if (nUpNum > 10 && nDownNum > 10 && bPack == true)
	{
		bPack = false;
	}

	int32_t nNumTemp = 40;
	int32_t nDetNum = nWidthIn / 64 * 2 / 3;

	if (0 == m_nPreType)   //0为空气  4为空气或薄包裹但是倾向空气
	{
		if (dfMaxVariance < m_fMaxAirViranceL)
		{
			int32_t nNum = 0;
			uint16_t* ptrImg1 = matLow.Ptr<uint16_t>() + nWidthIn * 3;
			uint16_t* ptrImg2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow;
			uint16_t* ptrImg3 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1);
			pacNumDec(ptrImg1, ptrImg2, ptrImg3, nWidthIn, 60000, nNum);
			if (nNum <= 24 || bPack == false)
			{
				if (dfMaxVariance < m_fMaxAirViranceL - nNumTemp && fabs(dfMaxVariance) > 1E-6 && dfMaxVariance > 60)
				{
					m_fMaxAirViranceL = dfMaxVariance + nNumTemp;
				}
				m_nPreType = 0;
				return XRAY_LIB_OK;
			}
			else if (false == bChange)
			{
				int32_t nNum1 = 0;
				int32_t nNum2 = 0;
				ptrImg1 = matLow.Ptr<uint16_t>() + nWidthIn * 3;
				ptrImg2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow;
				ptrImg3 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1);
				pacNumDec(ptrImg1, ptrImg2, ptrImg3, nWidthIn, 56000, 20000, nNum1, nNum2);
				if (nNum1 < 80 && nNum2 < 15)
				{
					if (dfMaxVariance < m_fMaxAirViranceL - nNumTemp && fabs(dfMaxVariance) > 1E-6 && dfMaxVariance > 60)
					{
						m_fMaxAirViranceL = dfMaxVariance + nNumTemp;
					}
					m_nPreType = 0;
					return XRAY_LIB_OK;
				}
				else if ((fRatio > 0.86 && nChangeNum > 2) || nChangeNum >= nDetNum)
				{
					m_nPreType = 2;
					return XRAY_LIB_OK;
				}
				else
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
			}
			else
			{
				m_nPreType = 1;
				return XRAY_LIB_OK;
			}
		}
		else
		{
			int32_t nNum = 0;
			uint16_t* ptrImg1 = matLow.Ptr<uint16_t>() + nWidthIn * 3;
			uint16_t* ptrImg2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow;
			uint16_t* ptrImg3 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1);
			pacNumDec(ptrImg1, ptrImg2, ptrImg3, nWidthIn, 57000, nNum);
			if ((fRatio > 0.86 && nChangeNum > 2) || nChangeNum >= 7)
			{
				m_nPreType = 2;
				return XRAY_LIB_OK;
			}
			if (nNum > 0 && true == bChange)
			{
				m_nPreType = 1;
				return XRAY_LIB_OK;
			}
			else
			{
				if (nNum >= 24)
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
				else
				{
					if (true == bChange || (bPack == 1 && fRatio < 0.89))
					{
						m_nPreType = 1;
						return XRAY_LIB_OK;
					}
					else
					{
						m_nPreType = 4;
						return XRAY_LIB_OK;
					}
				}
			}
		}
	}
	else if (4 == m_nPreType)   //0为空气  4为空气或薄包裹但是倾向空气
	{
		if (dfMaxVariance < m_fMaxAirViranceL)
		{
			int32_t nNum = 0;
			uint16_t* ptrImg1 = matLow.Ptr<uint16_t>() + nWidthIn * 3;
			uint16_t* ptrImg2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow;
			uint16_t* ptrImg3 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1);
			pacNumDec(ptrImg1, ptrImg2, ptrImg3, nWidthIn, 60000, nNum);

			if ((fRatio > 0.86 && nChangeNum > 2) || nChangeNum >= 7)
			{
				m_nPreType = 2;
				return XRAY_LIB_OK;
			}
			if (((nChangeNum <= 2) && (fRatio < 0.95) && (nNum > 10 || bPack == true)) || (nNum > 100))
			{
				m_nPreType = 4;
				return XRAY_LIB_OK;
			}
			else
			{
				m_nPreType = 0;
				return XRAY_LIB_OK;
			}
		}
		else
		{
			int32_t nNum = 0;
			uint16_t* ptrImg1 = matLow.Ptr<uint16_t>() + nWidthIn * 3;
			uint16_t* ptrImg2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow;
			uint16_t* ptrImg3 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1);
			pacNumDec(ptrImg1, ptrImg2, ptrImg3, nWidthIn, 61000, nNum);
			if (nNum > 5)
			{
				if ((fRatio > 0.86 && nChangeNum > 2) || nChangeNum >= 7)
				{
					m_nPreType = 2;
					return XRAY_LIB_OK;
				}
				else
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
			}
			else
			{
				m_nPreType = 0;
				return XRAY_LIB_OK;
			}
		}
	}
	else if (5 == m_nPreType)  //空气或薄包裹，但是倾向于包裹
	{
		if (dfMaxVariance < m_fMaxAirViranceL)
		{
			int32_t nNum = 0;
			uint16_t* ptrImg1 = matLow.Ptr<uint16_t>() + nWidthIn * 3;
			uint16_t* ptrImg2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow;
			uint16_t* ptrImg3 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1);
			pacNumDec(ptrImg1, ptrImg2, ptrImg3, nWidthIn, 60000, nNum);
			if (nNum >= 12)
			{
				if ((fRatio > 0.86 && nChangeNum > 2) || nChangeNum >= 7)
				{
					m_nPreType = 7;
					return XRAY_LIB_OK;
				}
				else
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
			}
			else
			{
				if (dfMaxVariance < m_fMaxAirViranceL - nNumTemp && fabs(dfMaxVariance) > 1E-6 && dfMaxVariance > 60)
				{
					m_fMaxAirViranceL = dfMaxVariance + nNumTemp;
				}
				m_nPreType = 0;
				return XRAY_LIB_OK;
			}
		}
		else
		{
			int32_t nNum = 0;
			uint16_t* ptrImg1 = matLow.Ptr<uint16_t>() + nWidthIn * 3;
			uint16_t* ptrImg2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow;
			uint16_t* ptrImg3 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1);
			pacNumDec(ptrImg1, ptrImg2, ptrImg3, nWidthIn, 60000, nNum);
			if (true == bChange && nNum != 0)
			{
				if ((fRatio > 0.86 && nChangeNum > 2) || nChangeNum >= 7)
				{
					m_nPreType = 7;
					return XRAY_LIB_OK;
				}
				else
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
			}
			else
			{
				if (nNum >= 5)
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
				else
				{
					if (true == bChange)
					{
						m_nPreType = 1;
						return XRAY_LIB_OK;
					}
					else
					{
						m_nPreType = 5;
						return XRAY_LIB_OK;
					}
				}
			}
		}
	}
	else if (1 == m_nPreType || 6 == m_nPreType)
	{
		if (dfMaxVariance < m_fMaxAirViranceL)
		{
			int32_t nNum = 0;
			uint16_t* ptrImg1 = matLow.Ptr<uint16_t>() + nWidthIn * 3;
			uint16_t* ptrImg2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow;
			uint16_t* ptrImg3 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1);
			pacNumDec(ptrImg1, ptrImg2, ptrImg3, nWidthIn, 61000, nNum);
			if (nNum >= 1)
			{
				if ((fRatio > 0.86 && nChangeNum > 2) || nChangeNum >= 7)
				{
					m_nPreType = 7;
					return XRAY_LIB_OK;
				}
				//特征5：计算归一化后积分时间方向均值最小值与最大值比值
				if (bPack == true && nNum < 30)
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
				else
				{
					m_nPreType = 4;
					return XRAY_LIB_OK;
				}
			}
			else
			{
				if (bPack == true)
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
				else
				{
					m_nPreType = 4;
					return XRAY_LIB_OK;
				}


			}
		}
		else
		{
			int32_t nNum = 0;
			uint16_t* ptrImg1 = matLow.Ptr<uint16_t>() + nWidthIn * 3;
			uint16_t* ptrImg2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow;
			uint16_t* ptrImg3 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1);
			pacNumDec(ptrImg1, ptrImg2, ptrImg3, nWidthIn, 60000, nNum);
			if (nNum >= 5)
			{
				m_nPreType = 1;
				return XRAY_LIB_OK;
			}
			else if (nNum >= 1)
			{
				if (fabs(dfMaxVarianceL) > 1E-6 && dfMaxVarianceL < m_fMaxAirViranceL && fabs(dfMaxVarianceR) > 1E-6 &&  dfMaxVarianceR < m_fMaxAirViranceL)
				{
					m_nPreType = 5;
					return XRAY_LIB_OK;
				}
				else
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
			}
			else
			{
				if (bPack == true)
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
				if (true == bChange)
				{
					m_nPreType = 7;
					return XRAY_LIB_OK;
				}
				if (fabs(dfMaxVarianceL) > 1E-6 && dfMaxVarianceL < m_fMaxAirViranceL && fabs(dfMaxVarianceR) > 1E-6 &&  dfMaxVarianceR < m_fMaxAirViranceL)
				{
					m_nPreType = 4;
					return XRAY_LIB_OK;
				}
				else
				{
					m_nPreType = 5;
					return XRAY_LIB_OK;
				}
			}
		}
	}
	else   //前一个条带为本底空气过渡处
	{
		if (dfMaxVariance < m_fMaxAirViranceL + 30.0f)
		{
			if (dfMaxVariance < m_fMaxAirViranceL - nNumTemp && fabs(dfMaxVariance) > 1E-6 && dfMaxVariance > 60)
			{
				m_fMaxAirViranceL = dfMaxVariance + nNumTemp;
			}
			m_nPreType = 0;
			return XRAY_LIB_OK;
		}
		else
		{
			int32_t nNum = 0;
			uint16_t* ptrImg1 = matLow.Ptr<uint16_t>() + nWidthIn * 3;
			uint16_t* ptrImg2 = matLow.Ptr<uint16_t>() + nWidthIn * nMinMeanRow;
			uint16_t* ptrImg3 = matLow.Ptr<uint16_t>() + nWidthIn * (nHeightIn - 1);
			pacNumDec(ptrImg1, ptrImg2, ptrImg3, nWidthIn, 58000, nNum);
			if (true == bChange)
			{
				if (fRatio > 0.86 && nChangeNum > 2)
				{
					m_nPreType = 2;
					return XRAY_LIB_OK;
				}
				else
				{
					m_nPreType = 1;
					return XRAY_LIB_OK;
				}
			}
			else
			{
				if ((nNum >= 20))
				{
					m_nPreType = 4;
					return XRAY_LIB_OK;
				}
				else
				{
					m_nPreType = 0;
					return XRAY_LIB_OK;
				}
			}
		}
	}

	return XRAY_LIB_OK;
}

/*********************************************************************
*                        校正功能接口
**********************************************************************/
/*高能接口*/
XRAY_LIB_HRESULT CaliModule::CaliHImage(XMat& highOrgData, XMat& highCaliData, XMat& highOriCaliData)
{
	XRAY_LIB_HRESULT hr;
	if (0 != m_nCountThreshold && 0 == m_nPreType)
	{
		hr = UpdateCaliTable(highOrgData, m_matHighFull);;
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "UpdateCaliTable is err.");
	}
	hr = CaliImage(highOrgData, highCaliData, highOriCaliData, m_matHighZero, m_matHighFull, m_stCaliHTemp, true);

	return hr;
}

/*低能接口*/
XRAY_LIB_HRESULT CaliModule::CaliLImage(XMat& lowOrgData, XMat& lowCaliData, XMat& lowOriCaliData)
{
	XRAY_LIB_HRESULT hr;
	if (0 != m_nCountThreshold && 0 == m_nPreType)
	{
		hr = UpdateCaliTable(lowOrgData, m_matLowFull);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "UpdateCaliTable is err.");
	}
	hr = CaliImage(lowOrgData, lowCaliData, lowOriCaliData, m_matLowZero, m_matLowFull, m_stCaliLTemp);

	return hr;
}

XRAY_LIB_HRESULT CaliModule::GetCaliTable(XRAYLIB_CALI_TABLE* pCaliTable)
{
	/* 目前仅支持单排 */
	XSP_CHECK(1 == pCaliTable->nCaliTableHeight, XRAY_LIB_INVALID_PARAM,
		     "calitable height : %d illegal.", pCaliTable->nCaliTableHeight);

	XSP_CHECK(m_matLowFull.wid == pCaliTable->nCaliWidth, XRAY_LIB_INVALID_PARAM, 
		      "CaliTable Width: %d don't match with libXRay ImageWidth：%d.", 
		      pCaliTable->nCaliWidth, m_matLowFull.wid);

	XSP_CHECK(pCaliTable->EnergyMode == m_pSharedPara->m_enEnergyMode, XRAY_LIB_ENERGYMODE_ERROR, 
		      "CaliTable EnergyMode: %d don't match with libXRay EnergyMode：%d.", 
		      pCaliTable->EnergyMode, m_pSharedPara->m_enEnergyMode);

	if (XRAYLIB_ENERGY_DUAL == pCaliTable->EnergyMode || XRAYLIB_ENERGY_HIGH == pCaliTable->EnergyMode)
	{
		if (XRAYLIB_UPDATEZERO == pCaliTable->UpdateMode)
		{
			XSP_CHECK(pCaliTable->pCaliTableBackgroundHigh, XRAY_LIB_NULLPTR);
			memcpy(pCaliTable->pCaliTableBackgroundHigh, m_matHighZero.Ptr(), m_matHighZero.wid * sizeof(uint16_t));
		}
		else if (XRAYLIB_UPDATEFULL == pCaliTable->UpdateMode)
		{
			XSP_CHECK(pCaliTable->pCaliTableAirHigh, XRAY_LIB_NULLPTR);
			memcpy(pCaliTable->pCaliTableAirHigh, m_matHighFull.Ptr(), m_matHighFull.wid * sizeof(uint16_t));
		}
		else
		{
			return XRAY_LIB_UPDATEAIRANDZERO_MODE_ERROR;
		}
	}

	if (XRAYLIB_ENERGY_DUAL == pCaliTable->EnergyMode || XRAYLIB_ENERGY_LOW == pCaliTable->EnergyMode)
	{
		if (XRAYLIB_UPDATEZERO == pCaliTable->UpdateMode)
		{
			XSP_CHECK(pCaliTable->pCaliTableBackgroundLow, XRAY_LIB_NULLPTR);
			memcpy(pCaliTable->pCaliTableBackgroundLow, m_matLowZero.Ptr(), m_matLowZero.wid * sizeof(uint16_t));
		}
		else if (XRAYLIB_UPDATEFULL == pCaliTable->UpdateMode)
		{
			XSP_CHECK(pCaliTable->pCaliTableAirLow, XRAY_LIB_NULLPTR);
			memcpy(pCaliTable->pCaliTableAirLow, m_matLowFull.Ptr(), m_matLowFull.wid * sizeof(uint16_t));
		}
		else
		{
			return XRAY_LIB_UPDATEAIRANDZERO_MODE_ERROR;
		}
	}

	if (XRAYLIB_ENERGY_SCATTER == pCaliTable->EnergyMode)
	{
		if (XRAYLIB_UPDATEZERO == pCaliTable->UpdateMode)
		{
			XSP_CHECK(pCaliTable->pCaliTableBackgroundLow, XRAY_LIB_NULLPTR);
			memcpy(pCaliTable->pCaliTableBackgroundLow, m_matCaliTableZeroBS.Ptr(), m_matCaliTableZeroBS.wid * sizeof(uint16_t));
		}
		else if (XRAYLIB_UPDATEFULL == pCaliTable->UpdateMode)
		{
			XSP_CHECK(pCaliTable->pCaliTableAirLow, XRAY_LIB_NULLPTR);
			memcpy(pCaliTable->pCaliTableAirLow, m_matCaliTableAirBS.Ptr(), m_matCaliTableAirBS.wid * sizeof(uint16_t));
		}
		else
		{
			return XRAY_LIB_UPDATEAIRANDZERO_MODE_ERROR;
		}
	}

	return XRAY_LIB_OK;
}

/*校正中间接口,用于拆分高低能对外接口*/
XRAY_LIB_HRESULT CaliModule::CaliImage(XMat& orgData, XMat& caliData, XMat& caliOriData,
	                                   XMat& tbeZero, XMat& tbeFull,  RTCaliTemp& caliTemp, bool bInconsistCali)
{
	XRAY_LIB_HRESULT hr;

	/* 临时内存尺寸初始化 */
	int32_t nWidthIn = orgData.wid;
	int32_t nHeightIn = orgData.hei;
	hr = caliTemp.caliTemp0.Reshape(nHeightIn, nWidthIn, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Init caliTemp0 is err.");
	hr = caliTemp.caliTemp1.Reshape(nHeightIn, nWidthIn, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Init caliTemp1 is err.");

	XMat matIn  = caliTemp.caliTemp0;
	XMat matOut = caliTemp.caliTemp1;

	/* 剂量修正后的归一化校正 */
	if (XRAYLIB_ON == m_enDoseCaliMertric)
	{
		hr = DoseCaliImageCPU(orgData, matOut, tbeZero, tbeFull);
		XSP_CHECK(XRAY_LIB_OK == hr, hr,"DoseCaliImageCPU is err.");
	}
	else
	{
		hr = CaliImageCPU(orgData, matOut, tbeZero, tbeFull);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "CaliImageCPU is err.");
	}
	/* 更新实时空气 */
	if (0 != m_nCountThreshold)
	{
		if (2 != m_nPreType)
		{
			m_bNeedRTCali = AirDetec(matOut);
			hr = UpdateCaliTable(orgData, tbeFull, m_bNeedRTCali, m_fRTUpdateAirRatio);
		}
	}
	else
	{
		m_bNeedRTCali = AirDetec(matOut);
		hr = UpdateCaliTable(orgData, tbeFull, m_bNeedRTCali, m_fRTUpdateAirRatio);
	}
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "UpdateCaliTable is err.");

	// OrgRaw输出
	caliOriData.Reshape(nHeightIn, nWidthIn, XSP_16UC1);
	memcpy(caliOriData.Ptr(), matOut.Ptr(), nHeightIn * nWidthIn * XSP_ELEM_SIZE(XSP_16UC1));

	/* 低灰度区域探测器不一致性校正 */ 
	if (true == m_bInconsistencyCaliCanUse && bInconsistCali)
	{
		hr = InconsistCaliImage(orgData, matOut, tbeZero, tbeFull);
		XSP_CHECK(XRAY_LIB_OK == hr, hr,"DoseCaliImageCPU is err.");
	}

	/* 平铺校正 */
	if (XRAYLIB_ON == m_enFlatDetMertric && m_bFlatDetCaliCanUse)
	{
		SwitchMat(matIn, matOut);
		hr = FlatDetCali(matIn, matOut, m_bFlatDetCaliInverse);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "FlatDetCali is err.");
	}

	/* 几何校正 */ 
	if (XRAYLIB_ON == m_enGeoMertric && m_bGeoCaliCanUse)
	{
		SwitchMat(matIn, matOut);
		hr = GeoCali(matIn, matOut, m_caliGeoTbe, m_bGeoCaliInverse);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "GeoCali is err.");
	}

	/* 皮带纹校正 */
	SwitchMat(matIn, matOut);
	hr = RemovalBeltGrain(matIn, matOut, m_pSharedPara->m_nBackGroundThreshold);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "RemovalBeltGrain is err.");

	/* 拉伸校正 */
	if (!FpEqual(m_fStretchRatio, 1.0f))
	{
		SwitchMat(matIn, matOut);
		hr = StretchImg(matIn, matOut);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "StretchImg is err.");
	}

	/* 数据拷贝输出 */
	caliData.Reshape(nHeightIn, nWidthIn, XSP_16UC1);
	memcpy(caliData.Ptr(), matOut.Ptr(), nHeightIn * nWidthIn * XSP_ELEM_SIZE(XSP_16UC1));

	return XRAY_LIB_OK;
}


/*********************************************************************
*                        不同特性校正内部接口
**********************************************************************/
/*归一化校正*/
XRAY_LIB_HRESULT CaliModule::CaliImageCPU(XMat &orgImg, XMat &caliImg, 
	                                      XMat &tbeZero, XMat &tbeFull)
{
	/* check para */
	XSP_CHECK(orgImg.IsValid() && caliImg.IsValid() &&
		      tbeZero.IsValid() && tbeFull.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == orgImg.type && XSP_16U == tbeZero.type && 
		      XSP_16U == caliImg.type && XSP_16U == tbeFull.type,
		      XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(orgImg, caliImg) && MatSizeEq(tbeZero, tbeFull),
		      XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XSP_CHECK(tbeFull.wid == orgImg.wid, XRAY_LIB_XMAT_SIZE_ERR);

	uint16_t *pOriData, *pCaliData, *pZero, *pFull;
	float32_t fRealAir, fValue;
	float32_t fAnti = 0.97f; //反溢出因子
	uint16_t nBaseLine = 0; //基线

	for (int32_t nh = 0; nh < orgImg.hei; nh++)
	{
		pZero = tbeZero.Ptr<uint16_t>();
		pFull = tbeFull.Ptr<uint16_t>();
		pOriData = orgImg.Ptr<uint16_t>(nh);
		pCaliData = caliImg.Ptr<uint16_t>(nh);
		for (int32_t nw = 0; nw < orgImg.wid; nw++)
		{
			fRealAir = float32_t(*pFull) - (*pZero);
			/* 预防不响应的坏点判断 */
			if (fRealAir - 0.0f < 1e-6)
			{
				fRealAir = 1.0f;
			}
			fValue = 65535.0f * (float32_t(*pOriData - *pZero) * fAnti / fRealAir) + nBaseLine;

			if (0 != m_nCountThreshold && 2 == m_nPreType)
			{
				fValue = 65535.0f;
			}

			*pCaliData = (uint16_t)Clamp(fValue, 0.0f, 65535.0f);

			if (nw<m_nWhitenUp || nw>orgImg.wid - m_nWhitenDown)
			{
				*pCaliData = (uint16_t)m_pSharedPara->m_nBackGroundGray;
			}

			pCaliData++;
			pOriData++;
			pZero++;
			pFull++;
		}
	}	
	return XRAY_LIB_OK;
}

/* 带低灰度区域校正的归一化校正 */
XRAY_LIB_HRESULT CaliModule::InconsistCaliImage(XMat &orgImg, XMat &caliImg, XMat &tbeZero, XMat &tbeFull)
{
	/* check para */
	XSP_CHECK(orgImg.IsValid() && caliImg.IsValid() &&
		      tbeZero.IsValid() && tbeFull.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == orgImg.type && XSP_16U == tbeZero.type && 
		      XSP_16U == caliImg.type && XSP_16U == tbeFull.type,
		      XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(orgImg, caliImg) && MatSizeEq(tbeZero, tbeFull),
		      XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XSP_CHECK(tbeFull.wid == orgImg.wid, XRAY_LIB_XMAT_SIZE_ERR);

	uint16_t *pOriData, *pCaliData, *pZero, *pFull;
	float32_t fRatio, fValue, fRealAir;
	const float32_t fAnti = 0.97f; // 反溢出因子
	uint16_t nRefLevel0, nRefLevel1, nRefLevel2, nRefLevel3; // 参考灰度值值
	uint16_t nTargetValue0, nTargetValue1, nTargetValue2, nTargetValue3;
	uint16_t InconsistencyWid = m_stInconsistCaliTbe.hdr.uCaliEndPixel - m_stInconsistCaliTbe.hdr.uCaliStartPixel;

	uint16_t bk_mean = tbeZero.Mean<uint16_t>(m_stInconsistCaliTbe.hdr.uCaliStartPixel, 0, InconsistencyWid, tbeZero.hei);
	uint16_t air_mean = tbeFull.Mean<uint16_t>(m_stInconsistCaliTbe.hdr.uCaliStartPixel, 0, InconsistencyWid, tbeFull.hei);

	// 计算校正范围内的参考灰度值均值
	float64_t dSum0 = 0, dSum1 = 0, dSum2 = 0, dSum3 = 0;
	for(int32_t i = m_stInconsistCaliTbe.hdr.uCaliStartPixel; i < m_stInconsistCaliTbe.hdr.uCaliEndPixel; i++)
	{
		dSum0 = dSum0 + (uint16_t)m_stInconsistCaliTbe.Ptr(0)[i];
		dSum1 = dSum1 + (uint16_t)m_stInconsistCaliTbe.Ptr(1)[i];
		dSum2 = dSum2 + (uint16_t)m_stInconsistCaliTbe.Ptr(2)[i];
		dSum3 = dSum3 + (uint16_t)m_stInconsistCaliTbe.Ptr(3)[i];
	}
	nTargetValue0 = dSum0 / InconsistencyWid;
	nTargetValue1 = dSum1 / InconsistencyWid;
	nTargetValue2 = dSum2 / InconsistencyWid;
	nTargetValue3 = dSum3 / InconsistencyWid;

	nTargetValue0 = 65535.0f * (nTargetValue0 - bk_mean) * fAnti / (air_mean - bk_mean);
	nTargetValue1 = 65535.0f * (nTargetValue1 - bk_mean) * fAnti / (air_mean - bk_mean);
	nTargetValue2 = 65535.0f * (nTargetValue2 - bk_mean) * fAnti / (air_mean - bk_mean);
	nTargetValue3 = 65535.0f * (nTargetValue3 - bk_mean) * fAnti / (air_mean - bk_mean);


	for (int32_t nh = 0; nh < orgImg.hei; nh++)
	{
		pZero = tbeZero.Ptr<uint16_t>(); // 这里tbeZero和tbeFull都只有一行数据
		pFull = tbeFull.Ptr<uint16_t>();
		pOriData = orgImg.Ptr<uint16_t>(nh);
		pCaliData = caliImg.Ptr<uint16_t>(nh);
		for (int32_t nw = 0; nw < orgImg.wid; nw++)
		{
			fRealAir = float32_t(*pFull) - (*pZero);
			/* 预防不响应的坏点判断 */
			if (fRealAir - 0.0f < 1e-6)
			{
				fRealAir = 1.0f;
			}

			if(nw < m_stInconsistCaliTbe.hdr.uCaliStartPixel || nw > m_stInconsistCaliTbe.hdr.uCaliEndPixel)
			{

				fValue = 65535.0f * (float32_t(*pOriData - *pZero) * fAnti / fRealAir);
			}
			else
			{
				nRefLevel0 = m_stInconsistCaliTbe.Ptr(0)[nw];
				nRefLevel1 = m_stInconsistCaliTbe.Ptr(1)[nw];
				nRefLevel2 = m_stInconsistCaliTbe.Ptr(2)[nw];
				nRefLevel3 = m_stInconsistCaliTbe.Ptr(3)[nw];
	
				if (*pOriData >= nRefLevel0)
				{
					if(*pFull - nRefLevel0 > 1e-10)
					{
						fRatio = float32_t(*pOriData - nRefLevel0) / (*pFull - nRefLevel0);
						fValue = fRatio * (65535.0f * fAnti - nTargetValue0) + nTargetValue0;
					}
					else
					{
						fValue = nTargetValue0;
					}

				}
				else if (*pOriData >= nRefLevel1)
				{
					if (nRefLevel0 - nRefLevel1 > 1e-10)
					{
						fRatio = float32_t(*pOriData - nRefLevel1) / (nRefLevel0 - nRefLevel1);
						fValue = fRatio * (nTargetValue0 - nTargetValue1) + nTargetValue1;
					}
					else
					{
						fValue = nTargetValue1;
					}
				}
				else if (*pOriData >= nRefLevel2)
				{
					if(nRefLevel1 - nRefLevel2 > 1e-10)
					{
						fRatio = float32_t(*pOriData - nRefLevel2) / (nRefLevel1 - nRefLevel2);
						fValue = fRatio * (nTargetValue1 - nTargetValue2) + nTargetValue2;
					}
					else
					{
						fValue = nTargetValue2;
					}
				}
				else if (*pOriData >= nRefLevel3)
				{
					if(nRefLevel2 - nRefLevel3 > 1e-10)
					{
						fRatio = float32_t(*pOriData - nRefLevel3) / (nRefLevel2 - nRefLevel3);
						fValue = fRatio * (nTargetValue2 - nTargetValue3) + nTargetValue3;
					}
					else
					{
						fValue = nTargetValue3;
					}
				}
				else
				{
					if(nRefLevel3 - *pZero > 1e-10)
					{
						fRatio = float32_t(*pOriData - *pZero) / (nRefLevel3 - *pZero);
						fValue = fRatio * nTargetValue3;
					}
					else
					{
						fValue = 0;
					}
					
				}
			}

			if (0 != m_nCountThreshold && 2 == m_nPreType)
			{
				fValue = 65535.0f;
			}

			*pCaliData = (uint16_t)Clamp(fValue, 0.0f, 65535.0f);

			// 上下置白处理
			if (nw < m_nWhitenUp || nw>orgImg.wid - m_nWhitenDown)
			{
				*pCaliData = (uint16_t)m_pSharedPara->m_nBackGroundGray;
			}

			pCaliData++;
			pOriData++;
			pZero++;
			pFull++;
		}
	}	
	return XRAY_LIB_OK;
}


VOID InterLinear(int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t x, float32_t &y)
{
	if (x0 == x1)
	{
		y = static_cast<float32_t>(y0);
	}
	else
	{
		const float32_t fx0 = static_cast<float32_t>(x0);
		const float32_t fy0 = static_cast<float32_t>(y0);
		const float32_t fx1 = static_cast<float32_t>(x1); 
		const float32_t fy1 = static_cast<float32_t>(y1);
		const float32_t fx = static_cast<float32_t>(x);
		y = fy0 + (fx - fx0) * (fy1 - fy0) / (fx1 - fx0);
	}
	return;
}

/*背散射几何配准*/
XRAY_LIB_HRESULT CaliModule::GeoCali_BS_InterLinear(XMat& imageIn, XMat& imageOut, bool bInverse)
{
	XSP_CHECK(imageIn.IsValid() && imageOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == imageIn.type && XSP_16U == imageOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR);

	int32_t nk, nh, nw, kindex1, kindex2;

	uint16_t* ptrImageIn = imageIn.Ptr<uint16_t>(0);
	uint16_t* ptrImageOut = imageOut.Ptr<uint16_t>(0);
	uint16_t* ptrGeoCaliFactor = (uint16_t*)m_caliBSGeoTbe.table;

	int32_t nHeight = imageIn.hei;
	int32_t nWidthOut = imageOut.wid;

	int32_t posDataBS = 0;
	float32_t f32ImgTemp = 0.0f, f32InterTemp = 0.0f;
	int32_t nxi0, nyi0, nxi1, nyi1;

	for (nk = 0; nk < m_nImageWidthBS; nk++)
	{
		kindex1 = ptrGeoCaliFactor[nWidthOut + nk] - 1;
		kindex2 = ptrGeoCaliFactor[nWidthOut + nk + 1] - 1 - 1;
		for (nh = 0; nh < nHeight; nh++)
		{
			if (kindex1 == kindex2)
			{
				nw = kindex1;
				f32ImgTemp = ptrImageIn[nh * m_nImageWidthBS + ptrGeoCaliFactor[nw] - 1 + posDataBS];
				f32ImgTemp = Clamp(f32ImgTemp, 0.0f, 65535.0f);

				if (bInverse)
				{
					ptrImageOut[nh * nWidthOut + nWidthOut - nw - 1] = (uint16_t)(f32ImgTemp);
				}
				else
				{
					ptrImageOut[nh * nWidthOut + nw] = (uint16_t)(f32ImgTemp);
				}
			}
			else // if kindex1 == kindex2
			{
				for (nw = kindex1; nw <= kindex2; nw++)
				{
					if (nk < (m_nImageWidthBS - 1))
					{
						nxi0 = kindex1;
						nxi1 = kindex2 + 1;
						nyi0 = ptrImageIn[nh * m_nImageWidthBS + nk + posDataBS];
						nyi1 = ptrImageIn[nh * m_nImageWidthBS + nk + 1 + posDataBS];
					}
					else
					{
						nxi0 = kindex1 - 1;
						nxi1 = kindex2;
						nyi0 = ptrImageIn[nh * m_nImageWidthBS + nk - 1 + posDataBS];
						nyi1 = ptrImageIn[nh * m_nImageWidthBS + nk + posDataBS];
					}

					InterLinear(nxi0, nyi0, nxi1, nyi1, nw, f32InterTemp);
					f32ImgTemp = Clamp(f32InterTemp, 0.0f, 65535.0f);
					if (bInverse)
					{
						ptrImageOut[nh * nWidthOut + nWidthOut - nw - 1] = (uint16_t)(f32ImgTemp);
					}
					else
					{
						ptrImageOut[nh * nWidthOut + nw] = (uint16_t)(f32ImgTemp);
					}
				}
			}
		}
	}

	return XRAY_LIB_OK;
}

/*背散射归一化校正*/
XRAY_LIB_HRESULT CaliModule::CaliBSImageCPU(XMat& orgImg, XMat& caliImg, float32_t fAntiBS)
{
	XSP_CHECK(orgImg.IsValid() && caliImg.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	uint16_t nBaseLine = 0;
	int32_t nHeightOut = orgImg.hei / m_fPulsBS; 	// 时间轴下采样后的长度
	int32_t nWidthOut = caliImg.wid;

	int32_t nh, nw, nt;
	uint16_t u32TempPuls = 0;
	float32_t fTemp = 0.0f, fValue = 0.0f, fRealAir = 0.0f;

	for (nh = 0; nh < nHeightOut; nh++)
	{
		uint16_t* ptrCali = caliImg.Ptr<uint16_t>(nh);
		uint16_t* ptrOrg = orgImg.Ptr<uint16_t>(nh);
		uint16_t* ptrAir = m_matCaliTableAirBS.Ptr<uint16_t>(0);
		uint16_t* ptrZero = m_matCaliTableZeroBS.Ptr<uint16_t>(0);

		// 只计算m_nImageWidthBS, 即背散数据探测器方向有效宽度
		for (nw = 0; nw < nWidthOut; nw++)
		{
			// 时间轴方向下采样 长度缩短为之前的 (1 / m_fPulsBs)
			for (nt = 0; nt < m_fPulsBS; nt++)
			{
				ptrOrg = orgImg.Ptr<uint16_t>(nh * m_fPulsBS + nt);
				u32TempPuls += ptrOrg[nw];
			}

			fRealAir = static_cast<float32_t>(ptrAir[nw] - ptrZero[nw]);
			if (fRealAir - 0.0f < 1e-6) 
			{ 
				fRealAir = 1.0f; 
			}

			fTemp = (float32_t)(u32TempPuls - ptrZero[nw]) / fRealAir - 1.0f;
			fValue = (float32_t)(fTemp * fAntiBS + nBaseLine);
			fValue = Clamp(fValue, 0.0f, 65535.0f);

			ptrCali[nw] = static_cast<uint16_t>(fValue);
			u32TempPuls = 0;
		}
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::DoseCaliImageCPU(XMat& orgImg, XMat& caliImg,
	                                          XMat& tbeZero, XMat& tbeFull)
{
	XSP_CHECK(orgImg.IsValid() && caliImg.IsValid() &&
		      tbeZero.IsValid() && tbeFull.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == orgImg.type && XSP_16U == tbeZero.type &&
		      XSP_16U == caliImg.type && XSP_16U == tbeFull.type,
		      XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(orgImg, caliImg) && MatSizeEq(tbeZero, tbeFull),
		      XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XSP_CHECK(tbeFull.wid == orgImg.wid, XRAY_LIB_XMAT_SIZE_ERR);

	uint16_t *pOriData, *pCaliData, *pZero, *pFull;

	float32_t fValue, fRatioDose, fRealAir;
	float32_t fDoseRefTop, fDoseRefDown; /*fDoseDifTop,fDoseDifDown,fDoseDif*/;
	float32_t fAnti = 0.97f; //反溢出因子
	uint16_t nBaseLine = 0; //基线

	for (int32_t nh = 0; nh < orgImg.hei; nh++)
	{
		pZero = tbeZero.Ptr<uint16_t>();
		pFull = tbeFull.Ptr<uint16_t>();
		pOriData = orgImg.Ptr<uint16_t>(nh);
		pCaliData = caliImg.Ptr<uint16_t>(nh);
		CaliDoseRef(pOriData, tbeZero, tbeFull, fDoseRefTop, fDoseRefDown, orgImg.wid);
		fRatioDose = 1.0f;
		if ((fDoseRefTop < 0.995 && fDoseRefDown < 0.995) || (fDoseRefTop > 1.005 && fDoseRefDown > 1.005))
		{
			fRatioDose = (fDoseRefTop + fDoseRefDown) / 2;
		}

		for (int32_t nw = 0; nw < orgImg.wid; nw++)
		{
			fRealAir = float32_t(*pFull) - (*pZero);
			/* 预防不响应的坏点判断 */
			if (fRealAir - 0.0f < 1e-6)
			{
				fRealAir = 1.0f;
			}
			if (fRatioDose < 0.2)
			{
				fValue = 65535 * 0.97f;
			}
			else
			{
				fValue = (float32_t)((*pOriData - *pZero) / fRealAir);
				if (fRatioDose > 0.995 && fRatioDose < 1.005)
				{
					fValue = (float32_t)(fValue*fAnti*65535.0f + nBaseLine);
				}
				else
				{
					fValue = fValue / fRatioDose;
					fValue = (float32_t)(fValue*fAnti*65535.0f + nBaseLine);
				}
			}

			if (0 != m_nCountThreshold && 2 == m_nPreType)
			{
				fValue = 65535.0f;
			}

			*pCaliData = (uint16_t)Clamp(fValue, 0.0f, 65535.0f);

			if (nw<m_nWhitenUp || nw>orgImg.wid - m_nWhitenDown)
			{
				*pCaliData = (uint16_t)m_pSharedPara->m_nBackGroundGray;
			}
			pCaliData++;
			pOriData++;
			pZero++;
			pFull++;
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::CaliDoseRef(uint16_t* orgImg, XMat &tbeZero, XMat &tbeFull,
	                                     float32_t& fRefTop, float32_t& fRefDown, int32_t nDetectorWidth)
{
	XSP_CHECK(tbeZero.IsValid() && tbeFull.IsValid() && orgImg, XRAY_LIB_XMAT_INVALID, "Invalid XMat.");
	fRefTop = 0.0f;
	fRefDown = 0.0f;

	//计算校正表左右两端的平均值，作为剂量对比标准值。为排除边缘点的错误，左右两端各排除10个点，即从10到32和nwith-32到nwith-10；
	float32_t fTotalTop, fTotalDown, fRealAirTop, fRealAirDown, fRatioTotalTop, fRatioTotalDown;
	int32_t nw, nNumTop, nNumDown;
	nNumTop = 0; nNumDown = 0; fTotalTop = 0; fTotalDown = 0; fRatioTotalTop = 0; fRatioTotalDown = 0;
	for (nw = 10; nw < 32; nw++)
	{
		fRealAirTop = (float32_t)(tbeFull.data.us[nw] - tbeZero.data.us[nw]);
		fTotalTop = (float32_t)tbeFull.data.us[nw] / tbeZero.data.us[nw];
		if (fRealAirTop == 0)
		{
			fRealAirTop = 1;
		}
		fRealAirDown = (float32_t)(tbeFull.data.us[nDetectorWidth - nw] - tbeZero.data.us[nDetectorWidth - nw]);
		fTotalDown = (float32_t)tbeFull.data.us[nDetectorWidth - nw] / tbeZero.data.us[nDetectorWidth - nw];
		if (fRealAirDown == 0)
		{
			fRealAirDown = 1;
		}
		if (fTotalTop > 1.05)
		{
			nNumTop++;
			fRatioTotalTop += (float32_t)(orgImg[nw] - tbeZero.data.us[nw]) / fRealAirTop;
		}
		if (fTotalDown > 1.05)
		{
			nNumDown++;
			fRatioTotalDown += (float32_t)(orgImg[nDetectorWidth - nw] - tbeZero.data.us[nDetectorWidth - nw]) / fRealAirDown;
		}
	}
	if (nNumTop == 0)
	{
		nNumTop = 1;
	}
	if (nNumDown == 0)
	{
		nNumDown = 1;
	}
	fRefTop = fRatioTotalTop / nNumTop;
	fRefDown = fRatioTotalDown / nNumDown;
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::ExecuteGeoCali(XMat& imageIn, XMat& imageOut)
{
	GeoCali(imageIn, imageOut, m_caliGeoTbe, m_bGeoCaliInverse);
	return XRAY_LIB_OK;
}

/*几何校正*/
XRAY_LIB_HRESULT CaliModule::GeoCali(XMat& imageIn, XMat& imageOut, GeoCaliTable GeomTbe, bool bInverse)
{
	XSP_CHECK(m_bGeoCaliCanUse, XRAY_LIB_GEOCALITABLE_ERROR, "LibXRay: Can not do GeoCali.");

	int32_t nh, nw, nNum, nTotal, nIndex;
	double dRatio, dTotalRatio;
	for (nh = 0; nh < imageIn.hei; nh++)
	{
		uint16_t* pIn = imageIn.Ptr<uint16_t>(nh);
		uint16_t* pOut = imageOut.Ptr<uint16_t>(nh);
		for (nw = 0; nw < imageIn.wid; nw++)
		{
			nTotal = 0;
			dTotalRatio = 0;
			double* pIdx = GeomTbe.IdxPtr(nw);
			double* pRatio = GeomTbe.RatioPtr(nw);
			for (nNum = 0; nNum < GeomTbe.IdxNum(nw); nNum++)
			{
				if (bInverse)
				{
					nIndex = imageIn.wid - 1 - (int32_t)(*pIdx);
				}
				else
				{
					nIndex = (int32_t)(*pIdx);
				}
				dRatio = *pRatio;
				/*第零列存的都是剂量值，因此不参与插值*/
				if (nIndex == 0)
				{
					nIndex = 1;
				}
				nTotal += (int32_t)(dRatio * (pIn[nIndex]));
				dTotalRatio += dRatio;
				pIdx++;
				pRatio++;
			}
			/*因为计算和结构设计误差，边缘点可能会产生黑线等情况*/
			if (dTotalRatio < 0.95)
			{
				nTotal = 65535;
			}
			nTotal = Clamp(nTotal, 0, 65535);
			if (bInverse)
			{
				*(pOut + imageIn.wid - nw - 1) = nTotal;
			}
			else
			{
				*(pOut + nw) = nTotal;
			}
		}
	}
	return XRAY_LIB_OK;
}

/*平铺校正*/
XRAY_LIB_HRESULT CaliModule::FlatDetCali(XMat& imageIn, XMat& imageOut, bool bInverse)
{
	int32_t nh, nw, nNum;
	double nTotal;

	for (nh = 0; nh < imageIn.hei; nh++)
	{
		uint16_t* pIn = imageIn.Ptr<uint16_t>(nh);
		uint16_t* pOut = imageOut.Ptr<uint16_t>(nh);
		// 前10个像素
		for (nw = 0; nw < 10; nw++)
		{
			nTotal = 0;
			double* pIdx = m_stCaliFlatTbe.Ptr(nw);
			if (bInverse)
			{
				for (nNum = 10 - nw; nNum < 21; nNum++)
				{
					nTotal += *pIdx * (double)(*(pIn + imageIn.wid - 1 - (nw - 10 + nNum)));
					pIdx++;
				}
				nTotal = (double)Clamp(nTotal, 0.0, 65535.0);
				*(pOut+ imageIn.wid - 1 - nw) = (uint16_t)round(nTotal);
			}
			else
			{
				for (nNum = 10 - nw; nNum < 21; nNum++)
				{
					nTotal += *pIdx * (double)(*(pIn + imageIn.wid - 1 - (nw - 10 + nNum)));
					pIdx++;
				}
				nTotal = (double)Clamp(nTotal, 0.0, 65535.0);
				*(pOut + nw) = (uint16_t)round(nTotal);
			}
		}
		// 第6到第nWidth-5个像素
		for (nw = 10; nw < imageIn.wid - 10; nw++)
		{
			nTotal = 0;
			double* pIdx = m_stCaliFlatTbe.Ptr(nw);
			if (bInverse)
			{
				for (nNum = 0; nNum < 21; nNum++)
				{
					nTotal += *pIdx * (double)(*(pIn + imageIn.wid - 1 - (nw - 10 + nNum)));
					pIdx++;
				}
				nTotal = (double)Clamp(nTotal, 0.0, 65535.0);
				*(pOut + imageIn.wid - 1 - nw) = (uint16_t)round(nTotal);
			}
			else
			{
				for (nNum = 0; nNum < 21; nNum++)
				{
					nTotal += *pIdx * (double)(*(pIn + imageIn.wid - 1 - (nw - 10 + nNum)));
					pIdx++;
				}
				nTotal = (double)Clamp(nTotal, 0.0, 65535.0);
				*(pOut + nw) = (uint16_t)round(nTotal);
			}
		}
		// 第nWidth-10到第nWidth个像素
		for (nw = imageIn.wid - 10; nw < imageIn.wid; nw++)
		{
			nTotal = 0;
			double* pIdx = m_stCaliFlatTbe.Ptr(nw);
			if (bInverse)
			{
				double* pIdx = m_stCaliFlatTbe.Ptr(nw);
				for (nNum = 0; nNum < imageIn.wid - nw + 10; nNum++)
				{
					nTotal += *pIdx * (double)(*(pIn + imageIn.wid - 1 - (nw - 10 + nNum)));
					pIdx++;
				}
				nTotal = (double)Clamp(nTotal, 0.0, 65535.0);
				*(pOut + imageIn.wid - 1 - nw) = (uint16_t)round(nTotal);
			}
			else
			{
				for (nNum = 0; nNum < imageIn.wid - nw + 10; nNum++)
				{
					nTotal += *pIdx * (double)(*(pIn + imageIn.wid - 1 - (nw - 10 + nNum)));
					pIdx++;
				}
				nTotal = (double)Clamp(nTotal, 0.0, 65535.0);
				*(pOut + nw) = (uint16_t)round(nTotal);
			}
		}
	}
	return XRAY_LIB_OK;
}

/*皮带纹校正*/
XRAY_LIB_HRESULT CaliModule::RemovalBeltGrain(XMat &orgImg, XMat &beltTemp, int32_t nBackGroundThreshold)
{
	
	/*计算皮带纹去除滤波核半径*/
	int32_t kernel_r = m_nBeltGrainLimit / 2 + 1;

	uint16_t *pOriData, *pTempData;
	for (int32_t nh = 0; nh < orgImg.hei; nh++)
	{
		pOriData = orgImg.Ptr<uint16_t>(nh);
		pTempData = beltTemp.Ptr<uint16_t>(nh);
		for (int32_t nw = 0; nw < orgImg.wid; nw++)
		{
			uint16_t tmpPixValue = *pOriData;
			/*皮带纹去除操作参数*/
			uint16_t pix2, pix3, pix4, pix5, pix6;
			if (nh < orgImg.tpad ||
				nh >= orgImg.hei - orgImg.tpad)
			{
				pix2 = nBackGroundThreshold;
				pix3 = nBackGroundThreshold;
				pix4 = nBackGroundThreshold;
				pix5 = nBackGroundThreshold;
				pix6 = nBackGroundThreshold;
			}
			else
			{
				pix2 = *(pOriData - kernel_r - 2);
				pix3 = *(pOriData - kernel_r);
				pix4 = *(pOriData);
				pix5 = *(pOriData + kernel_r);
				pix6 = *(pOriData + kernel_r + 2);
			}
			/* 统计3个像素中小于背景灰度阈值的个数 */
			uint16_t columnFiveMask = (pix3 < (uint16_t)nBackGroundThreshold) + 
				(pix4 < (uint16_t)nBackGroundThreshold) + (pix5 < (uint16_t)nBackGroundThreshold); // 整列像素
			*pTempData = tmpPixValue;
			/* 皮带纹路判断逻辑：
			* 1. 内部三像素小于背景阈值的数目不大于2
			* 2. 中心像素灰度值小于背景阈值
			* 3. 外部两像素灰度值不小于背景阈值
			*/
			if ((columnFiveMask <= 2) &&
				(pix4 < (uint16_t)nBackGroundThreshold) && 
				(pix2 >= (uint16_t)nBackGroundThreshold) &&
				(pix6 >= (uint16_t)nBackGroundThreshold))
			{
				/*当json几何光路有皮带边缘时，仅对皮带边缘置白，当几何光路无皮带边缘时，皮带边缘赋值为-1，直接对整图处理*/
				if ((nw > m_stBeltdge.nStart1 && nw < m_stBeltdge.nEnd1) ||
					(nw > m_stBeltdge.nStart2 && nw < m_stBeltdge.nEnd2) ||
					-1 == m_stBeltdge.nStart1)
				{
					/*如果去皮带纹上限为0，直接跳过*/
					if (m_nBeltGrainLimit != 0)
					{
						*pTempData = 63568;
					}
				}
			}
			pOriData++;
			pTempData++;
		}
	}
	return XRAY_LIB_OK;
}

/*探测器亚像素偏移*/
XRAY_LIB_HRESULT CaliModule::PixelWriggleCali(XMat& ptrImageIn, XMat& ptrImageOut, XMat& lambda, XMat& direction, XRAYLIB_ENERGY_MODE mode)
{
	//入参检查
	XSP_CHECK(ptrImageIn.IsValid() && ptrImageOut.IsValid() && 
		      lambda.IsValid() && direction.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(MatSizeEq(ptrImageIn, ptrImageOut), 
		      XRAY_LIB_XMAT_SIZE_ERR, "PixelWriggleCali XMat no equal");

	int32_t nw, nh, nIndex;
	int32_t nWidth = ptrImageIn.wid;
	int32_t nHeight = ptrImageIn.hei;
	float32_t fTemp, fLambda;
	float32_t fSpeed = m_fSpeed;

	memcpy(ptrImageOut.Ptr<uint16_t>(), ptrImageIn.Ptr<uint16_t>(), sizeof(uint16_t) * nWidth * nHeight);

	int8_t* pDirect = direction.Ptr<int8_t>();
	float32_t* pLambda = lambda.Ptr<float32_t>();
	for (nh = 1; nh < nHeight-1; nh++)
	{
		uint16_t* pIn = ptrImageIn.Ptr<uint16_t>(nh);
		uint16_t* pOut = ptrImageOut.Ptr<uint16_t>(nh);
		for (nw = 1; nw < nWidth - 1; nw++)
		{
			nIndex = pDirect[nw];
			if (mode == XRAYLIB_ENERGY_HIGH)
			{
				/**************************************************************
				* 依据速度对高能数据做整体的偏移，修正原子序数图，去除图像绿边
				* 依据140100高速机2m/s偏移-0.5及1m/s偏移0的经验值为基准，线性建模
				* 经探测版错位校正后插值系数lambda计算公式为：
				* lambda = lambda - max((speed - 1.0), 0) * 0.5
				***************************************************************/
				fLambda = nIndex * pLambda[nw] - MAX((fSpeed - 1.0f), 0.0f) * 0.5f;
				nIndex = fLambda < 0 ? -1 : 1;
				fLambda = abs(fLambda);
				fLambda = fLambda > 1.0f ? (fLambda - 1.0f) : fLambda;

				fTemp = (1 - fLambda) * pIn[nw] + fLambda * pIn[nw + nIndex * nWidth];
			}
			else 
			{
				pLambda[nw] = pLambda[nw] > 1.0f ? (pLambda[nw] - 1.0f) : pLambda[nw];
				fTemp = (1 - pLambda[nw]) * pIn[nw] + pLambda[nw] * pIn[nw + nIndex * nWidth];
			}
			pOut[nw] = (uint16_t)fTemp;
		}
	}
	return XRAY_LIB_OK;
}

/*拉伸校正*/
XRAY_LIB_HRESULT CaliModule::StretchImg(XMat& imageIn, XMat& imageOut)
{
	XSP_CHECK(imageIn.IsValid() && imageOut.IsValid(), 
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(MatSizeEq(imageIn, imageOut),
		      XRAY_LIB_XMAT_SIZE_ERR, "BoundaryFusion XMat no equal");

	float32_t fScaleWidth = m_fStretchRatio;
	int32_t nDstWidth = (int32_t)floor(imageIn.wid * m_fStretchRatio);
	int32_t nWidth = imageIn.wid;
	int32_t whiteRow = imageIn.wid - nDstWidth;
	int32_t whiteRowUp = whiteRow / 2;
	uint16_t* pIn = imageIn.Ptr<uint16_t>();
	uint16_t* pOut = imageOut.Ptr<uint16_t>();
	int32_t j = 0;
	for (int32_t i = 0; i < imageIn.hei; i++)
	{
		for (int32_t n = 0; n < imageIn.wid; n++)
		{
			if (n < whiteRowUp || n >= whiteRowUp + nDstWidth)
			{
				pOut[i * nWidth + n] = m_pSharedPara->m_nBackGroundGray;
				continue;
			}
			j = n - whiteRowUp;
			float32_t Temp_j = (float32_t)((j + 0.5) / fScaleWidth - 0.5);
			int32_t Round_j = (int32_t)floor(Temp_j);
			float32_t v = Temp_j - Round_j;
			if (Round_j < 0) Round_j = 0;
			if ((Round_j + 1) > nWidth - 1) Round_j = nWidth - 2;
			int32_t Index_A = i * nWidth + Round_j;
			int32_t Index_B = i * nWidth + Round_j + 1;
			int32_t Index = i * nWidth + n;
			float32_t TempZ = (1 - v) * pIn[Index_A] + v * pIn[Index_B];
			pOut[Index] = (uint16_t)(TempZ);
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::SetTBpad(int32_t stripeWid, int32_t stripeHei, int32_t stripeType, 
									  std::pair<int32_t, int32_t>& tbPadHei, XRAY_TBPAD_CACHE_PARA& tbPadCache, uint8_t uPadDefaltValue)
{
	XRAY_LIB_HRESULT hr = XRAY_LIB_OK;

	// 检查当前扩边参数是否与缓存中的参数一致
	bool isSameParameters = (tbPadHei.first == tbPadCache.nTpadHei) &&
							(tbPadHei.second == tbPadCache.nBpadHei) &&
							(stripeHei == (tbPadCache.nTBpadStripeHei - tbPadCache.nTpadHei - tbPadCache.nBpadHei));

	// 如果参数一致且不需要清除临时数据，则直接返回
	if (!isSameParameters || tbPadCache.bClearTemp)
	{
		tbPadCache.bClearTemp = false;

		// 初始化写入索引、当前填充数量以及缓存中条带和上下扩边的总高度
		int32_t nTpadNumHei = static_cast<int32_t>(std::ceil(static_cast<float32_t>(tbPadHei.first) / stripeHei)) * stripeHei;
		tbPadCache.nWriteIndex = nTpadNumHei + stripeHei; // 这里初始化写索引为上扩边+条带高度是因为默认上扩边就是填充完成的，防止缺少输出部分数据
		tbPadCache.nReadIndex = 0;
		tbPadCache.nTpadHei = tbPadHei.first;
		tbPadCache.nBpadHei = tbPadHei.second;
		tbPadCache.nTBpadStripeHei = tbPadHei.first + tbPadHei.second + stripeHei;

		// 计算环形缓冲区的宽度和高度
		int32_t nBufferWid = stripeWid;
		int32_t nBufferHei = stripeHei * (tbPadCache.mRingBuf.size / XSP_ELEM_SIZE(stripeType) / stripeWid / stripeHei);

		// 重新调整环形缓冲区的大小以适应新的参数
		hr = tbPadCache.mRingBuf.Reshape(nBufferHei, nBufferWid, stripeType);
		XSP_CHECK(hr == XRAY_LIB_OK, hr, "Reshape ring buffer failed nBufferHei:%d nBufferWid:%d buffsize:%ld", nBufferHei, nBufferWid, tbPadCache.mRingBuf.size);
		tbPadCache.mRingBuf.SetValue(uPadDefaltValue);
	}

	return hr;
}

XMat CaliModule::CacheTBpad(XMat& matIn, XRAY_TBPAD_CACHE_PARA& tbPadCache, bool* bReachedLimit)
{
	XMat matOut;

    int32_t nWidIn = matIn.wid;
    int32_t nHeiIn = matIn.hei - matIn.tpad - matIn.bpad;
	size_t nElemSize = XSP_ELEM_SIZE(matIn.type);

	// 计算扩边条带涉及到的缓存区的高度
	int32_t nTpadNumHei = static_cast<int32_t>(std::ceil(static_cast<float32_t>(tbPadCache.nTpadHei) / nHeiIn)) * nHeiIn;
	int32_t nBpadNumHei = static_cast<int32_t>(std::ceil(static_cast<float32_t>(tbPadCache.nBpadHei) / nHeiIn)) * nHeiIn;
	int32_t nTBpadCacheHei = nTpadNumHei + nBpadNumHei + nHeiIn;

	// 环形缓冲区高度和宽度
	int32_t nBufferWid = tbPadCache.mRingBuf.wid;
	int32_t nBufferHei = tbPadCache.mRingBuf.hei;

	// 当写索引为nBufferHei时，则需要移动环形缓冲区的数据，让其可以直接构造XMat
	if (tbPadCache.nWriteIndex == nBufferHei)
	{
		for(int32_t i = 0; i < nBufferHei - tbPadCache.nReadIndex; ++i)
		{
			memcpy(tbPadCache.mRingBuf.Ptr(i), tbPadCache.mRingBuf.Ptr(tbPadCache.nReadIndex + i), nWidIn * nElemSize);
		}
		tbPadCache.nWriteIndex = nBufferHei - tbPadCache.nReadIndex;
		tbPadCache.nReadIndex = 0;
	}
	
	// 拷贝数据至扩边条带缓存区
	for (int32_t i = 0; i < nHeiIn; ++i)
	{
		memcpy(tbPadCache.mRingBuf.Ptr(tbPadCache.nWriteIndex + i), matIn.Ptr(matIn.tpad + i), nWidIn * nElemSize);
	}

	// 更新写索引
	tbPadCache.nWriteIndex = tbPadCache.nWriteIndex + nHeiIn;

	// 当前已缓存条带高度
	int32_t nCurTBpadHei = tbPadCache.nWriteIndex - tbPadCache.nReadIndex;

	// 更新读索引
	tbPadCache.nReadIndex = (nCurTBpadHei >= nTBpadCacheHei + nHeiIn) ? (tbPadCache.nReadIndex + nHeiIn) : tbPadCache.nReadIndex;

	if (bReachedLimit != nullptr)
	{
		*bReachedLimit = (nCurTBpadHei >= nTBpadCacheHei + nHeiIn) ? true: false;
	}

	// 输出matOut
	int32_t nTpadGapHei = nTpadNumHei - tbPadCache.nTpadHei;
	matOut.Init(tbPadCache.nTBpadStripeHei, nBufferWid, tbPadCache.nTpadHei, tbPadCache.nBpadHei, matIn.type, tbPadCache.mRingBuf.Ptr(tbPadCache.nReadIndex + nTpadGapHei));

    return matOut;
}

XRAY_LIB_HRESULT CaliModule::ModTpadBuf(XMat& matIn, XRAY_TBPAD_CACHE_PARA& tbPadCache)
{
	XRAY_LIB_HRESULT hr = XRAY_LIB_OK;

    int32_t nWidIn = matIn.wid;
    int32_t nHeiIn = matIn.hei - matIn.tpad - matIn.bpad;
	size_t nElemSize = XSP_ELEM_SIZE(matIn.type);

	// 环形缓冲区高度和宽度
	int32_t nBufferHei = tbPadCache.mRingBuf.hei;

	// 计算环形缓冲区中需要被替换的index
	int32_t nCircleCacheReplaceIndex = tbPadCache.nWriteIndex - nHeiIn;

	if(nCircleCacheReplaceIndex >= 0)
	{
		for(int32_t i = 0; i < nHeiIn; ++i)
		{
			memcpy(tbPadCache.mRingBuf.Ptr(nCircleCacheReplaceIndex + i), matIn.Ptr(matIn.tpad + i), nWidIn * nElemSize);
		}
	}
	else
	{
		// 先替换头部数据
		int32_t nHeadHei = std::abs(nCircleCacheReplaceIndex);
		for (int32_t i = 0; i < nHeadHei; ++i)
		{
			memcpy(tbPadCache.mRingBuf.Ptr(nBufferHei - nHeadHei + i), matIn.Ptr(matIn.tpad + i), nWidIn * nElemSize);
		}

		// 再替换尾部数据
		int32_t nTailHei = nHeiIn - nHeadHei;
		for (int32_t i = 0; i < nTailHei; ++i)
		{
			memcpy(tbPadCache.mRingBuf.Ptr(i), matIn.Ptr(matIn.tpad + nHeadHei + i), nWidIn * nElemSize);
		}
	}

    return hr;
}

XRAY_LIB_HRESULT CaliModule::UpdateCaliTable(XMat & orgImg, XMat & tbeFull, bool bNeedRTCali, float32_t fRTCaliRatio)
{
	XSP_CHECK(fRTCaliRatio >= 0 && fRTCaliRatio <= 1,
		XRAY_LIB_RTCALIRATIO_ERROR, "Invalid RTCaliRtio.");

	uint16_t *pOriData, *pFull;
	if (true == bNeedRTCali)
	{
		for (int32_t nh = 0; nh < orgImg.hei; nh++)
		{
			pOriData = orgImg.Ptr<uint16_t>(nh);
			pFull = tbeFull.Ptr<uint16_t>();
			for (int32_t nw = 0; nw < orgImg.wid; nw++)
			{

				*pFull = (uint16_t)(*pFull * (1.0 - fRTCaliRatio) + *pOriData * fRTCaliRatio);
				pOriData++;
				pFull++;

			}
		}
	}
		/*
		疑问：此计数是否有用？
		m_nRTUpdateTimes += nHeight;
		if (m_nRTUpdateTimes >= 48)
			m_nDoseAdjust = 0;
		*/
	return XRAY_LIB_OK;
}

bool CaliModule::AirDetec(XMat& caliImg)
{
	/*计数+均值同时统计的方式，统计小于61000以下所有点，求其点数及均值。如下情况认为是空气，进行更新空气：
	1，均值较高（60000-61000）且点数较少20; 认为是校正失效引起的。
	2，均值很高 大于61000，且点数更少10；*/
	int32_t nh, nw, nNum, nNumR;
	double dTotal, dMeanH;
	/*先对nh方向做平均，减少噪声影响*/
	dTotal = 0; nNum = 0; nNumR = 0;
	for (nw = m_nTopHeight; nw < caliImg.wid - m_nDownHeight; nw++)
	{
		uint16_t* pIn = caliImg.Ptr<uint16_t>(0,nw);
		dMeanH = 0;
		for (nh = caliImg.tpad; nh < caliImg.hei - caliImg.tpad; nh++)
		{
			dMeanH += *pIn;
			pIn += caliImg.wid;
		}
		dMeanH /= nh;
		if (dMeanH > m_pSharedPara->m_nBackGroundThreshold || dMeanH == 0)  //等于0或者大于背景值的点不参与计算
		{
			continue;
		}
		if (dMeanH < m_pSharedPara->m_nPackageThresholdR)
		{
			nNumR++;
		}
		dTotal += dMeanH;
		nNum++;

	}
	bool bReturn = false;
	if (nNum == 0)
	{
		bReturn = true;//没有点比背景值更低，直接返回是空气
	}
	else
	{
		dTotal /= nNum;
		if (dTotal > 61000 && nNum<16 && nNumR<4)
			bReturn = true;
		else if (dTotal > 60000 && dTotal <= 61000 && nNum<10 && nNumR<3)
			bReturn = true;
	}
	return bReturn;
}

XRAY_LIB_HRESULT CaliModule::UpdateCaliTableSwitch(XMat& orgImg, XMat& tbeFull, XMat& orgImgMean)
{
	int32_t nh, nw, nCount;
	float32_t fMeanData, fRatioThreshold;
	int32_t nWidth = orgImg.wid;
	int32_t nHeight = orgImg.hei;
	uint16_t *pOriData, *pOriDataTemp;

	nCount = 0;
	fRatioThreshold = 0.03f;

	pOriDataTemp = orgImgMean.Ptr<uint16_t>();
	//按行计算均值
	for (nw = 0; nw < nWidth; nw++)
	{
		pOriData = orgImg.Ptr<uint16_t>(0, nw);
		fMeanData = 0.0f;
		for (nh = 0; nh < nHeight; nh++)
		{
			fMeanData += (float32_t)*pOriData;
			pOriData += nWidth;//待确定
		}
		fMeanData /= nh;

		//判断前条带是否包含包裹
		for (nh = 0; nh < nHeight; nh++)
		{
			if (abs(*pOriData - fMeanData) >= *pOriData * fRatioThreshold)
			{
				nCount += 1;
			}
			pOriData -= nWidth;
		}
		*pOriDataTemp = (uint16_t)(fMeanData);
		pOriDataTemp++;
	}
	//更新模板
	if (nCount < m_nCountThreshold)
	{
		memcpy(tbeFull.Ptr<uint16_t>(), orgImgMean.Ptr<uint16_t>(), sizeof(uint16_t)*nWidth);
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::UpdateCaliTableSwitch(XMat& orgImg, XMat& tbeFull, XMat& orgImgMean,
	                                               TIME_COLDHOT_PARA& timePara)
{
	if (!timePara.bTimeNodeStatus)
	{
		timePara.bTimeNodeStatus = true;
		GetTimeNow(&timePara.TimeBegin);
	}
	else
	{
		GetTimeNow(&timePara.TimeEnd);
		timePara.ldOneTime = GetTimeSpanUs(&timePara.TimeBegin, &timePara.TimeEnd);
		timePara.TimeBegin = timePara.TimeEnd;

		if ((timePara.ldOneTime / 1000) > m_nTimeInterval || (timePara.nColdHotTik >= 1 && timePara.nColdHotTik <= 4))
		{
			/* 时间间隔超过m_nTimeInterval(ms)后连续5个条带都进行更新判断 */
			UpdateCaliTableSwitch(orgImg, tbeFull, orgImgMean);
			timePara.nColdHotTik++;
			if (timePara.nColdHotTik == 5)
			{
				timePara.nColdHotTik = 0;
			}
		}
	}
	if (XRAYLIB_ON == m_enColdAndHot)
	{
		if (timePara.nColdHotTik >= 0 && timePara.nColdHotTik <= 4)
		{
			UpdateCaliTableSwitch(orgImg, tbeFull, orgImgMean);
			timePara.nColdHotTik++;
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::pacNumDec(uint16_t * ptrImg1, uint16_t * ptrImg2, uint16_t * ptrImg3, int32_t nWidth, int32_t nLimit, int32_t & nNum)
{
	float32_t fRealAir;
	float32_t fTemp;
	float32_t fValue;
	int32_t nValue;
	uint16_t* pCaliTableAirL = m_matLowFull.Ptr<uint16_t>() + m_nTopHeight;
	uint16_t* pCaliTableBkL = m_matLowZero.Ptr<uint16_t>() + m_nTopHeight;
	ptrImg1 += m_nTopHeight;
	ptrImg2 += m_nTopHeight;
	ptrImg3 += m_nTopHeight;
	for (int32_t nw = m_nTopHeight; nw < nWidth - m_nDownHeight; nw++)
	{
		fRealAir = (*pCaliTableAirL - *pCaliTableBkL);
		if (fRealAir == 0)
		{
			fRealAir = 1;
		}
		fTemp = (float32_t)(*ptrImg1 - *pCaliTableBkL) / fRealAir;
		fValue = (float32_t)(fTemp*0.97f*65535.0f);
		if (fValue < 0)
		{
			fValue = 0;
		}
		if (fValue > 65535)
		{
			fValue = 65535;
		}
		nValue = (uint16_t)fValue;
		if (0 < nValue && nValue < nLimit)
		{
			nNum++;
		}
		fTemp = (float32_t)(*ptrImg2 - *pCaliTableBkL) / fRealAir;
		fValue = (float32_t)(fTemp*0.97f*65535.0f);
		if (fValue < 0)
		{
			fValue = 0;
		}
		if (fValue > 65535)
		{
			fValue = 65535;
		}
		nValue = (uint16_t)fValue;
		if (0 < nValue && nValue < nLimit)
		{
			nNum++;
		}
		fTemp = (float32_t)(*ptrImg3 - *pCaliTableBkL) / fRealAir;
		fValue = (float32_t)(fTemp*0.97f*65535.0f);
		if (fValue < 0)
		{
			fValue = 0;
		}
		if (fValue > 65535)
		{
			fValue = 65535;
		}
		nValue = (uint16_t)fValue;
		if (0 < nValue && nValue < nLimit)
		{
			nNum++;
		}
		pCaliTableAirL++;
		pCaliTableBkL++;
		ptrImg1++;
		ptrImg2++;
		ptrImg3++;
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::pacNumDec(uint16_t * ptrImg1, uint16_t * ptrImg2, uint16_t * ptrImg3, int32_t nWidth, int32_t nLimit, int32_t nLimit1, int32_t & nNum, int32_t & nNum1)
{
	float32_t fRealAir;
	float32_t fTemp;
	float32_t fValue;
	int32_t nValue;
	uint16_t* pCaliTableAirL = m_matLowFull.Ptr<uint16_t>() + m_nTopHeight;
	uint16_t* pCaliTableBkL = m_matLowZero.Ptr<uint16_t>() + m_nTopHeight;
	ptrImg1 += m_nTopHeight;
	ptrImg2 += m_nTopHeight;
	ptrImg3 += m_nTopHeight;
	for (int32_t nw = m_nTopHeight; nw < nWidth - m_nDownHeight; nw++)
	{
		fRealAir = (*pCaliTableAirL - *pCaliTableBkL);
		if (fRealAir == 0)
		{
			fRealAir = 1;
		}
		fTemp = (float32_t)(*ptrImg1 - *pCaliTableBkL) / fRealAir;
		fValue = (float32_t)(fTemp*0.97f*65535.0f);
		if (fValue < 0)
		{
			fValue = 0;
		}
		if (fValue > 65535)
		{
			fValue = 65535;
		}
		nValue = (uint16_t)fValue;
		if (0 < nValue && nValue < nLimit)
		{
			nNum++;
		}
		if (0 < nValue && nValue < nLimit1)
		{
			nNum1++;
		}
		fTemp = (float32_t)(*ptrImg2 - *pCaliTableBkL) / fRealAir;
		fValue = (float32_t)(fTemp*0.97f*65535.0f);
		if (fValue < 0)
		{
			fValue = 0;
		}
		if (fValue > 65535)
		{
			fValue = 65535;
		}
		nValue = (uint16_t)fValue;
		if (0 < nValue && nValue < nLimit)
		{
			nNum++;
		}
		if (0 < nValue && nValue < nLimit1)
		{
			nNum1++;
		}
		fTemp = (float32_t)(*ptrImg3 - *pCaliTableBkL) / fRealAir;
		fValue = (float32_t)(fTemp*0.97f*65535.0f);
		if (fValue < 0)
		{
			fValue = 0;
		}
		if (fValue > 65535)
		{
			fValue = 65535;
		}
		nValue = (uint16_t)fValue;
		if (0 < nValue && nValue < nLimit)
		{
			nNum++;
		}
		if (0 < nValue && nValue < nLimit1)
		{
			nNum1++;
		}
		pCaliTableAirL++;
		pCaliTableBkL++;
		ptrImg1++;
		ptrImg2++;
		ptrImg3++;
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT CaliModule::UpdateCaliTable(XMat & orgImg, XMat & tbeFull)
{
	XSP_CHECK(orgImg.IsValid() && tbeFull.IsValid(),
		XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == orgImg.type &&  XSP_16U == tbeFull.type,
		XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(tbeFull.wid == orgImg.wid, XRAY_LIB_XMAT_SIZE_ERR);
	int32_t nh, nw;
	double dmean = 0.0f;
	int32_t nWidth = orgImg.wid;
	int32_t nHeight = orgImg.hei;
	uint16_t* pOriData = orgImg.Ptr<uint16_t>();
	uint16_t* pFull = tbeFull.Ptr<uint16_t>();
	uint16_t nMinValue;
	for (nw = 0; nw < nWidth; nw++)
	{
		dmean = 0.0f;
		nMinValue = 65535;
		pOriData = orgImg.Ptr<uint16_t>(0, nw);
		for (nh = 0; nh < (nHeight - 3); nh++)
		{
			if (nw < m_nTopHeight || (nWidth - nw) < m_nDownHeight)
			{
				if (*pOriData < nMinValue)
				{
					nMinValue = *pOriData;
				}
			}
			else
			{
				dmean += (double)*pOriData;
			}
			pOriData += nWidth;
		}
		dmean /= (nHeight - 3);
		if (nw < m_nTopHeight || (nWidth - nw) < m_nDownHeight)
		{
			if (nMinValue < 60000)
			{
				*pFull = nMinValue;
			}
		}
		else
		{
			if (dmean < 60000)
			{
				*pFull = (uint16_t)(dmean);
			}
		}
		pFull++;
	}
	return XRAY_LIB_OK;
}

/**@fn             GetPixPhyLen
*  @breif          获取探测器像素映射到成像平面的物理长度
*  @param1         stGeom                [I]         成像几何
*  @param2         matPixPhyLen          [O]         像素物理长度存储矩阵
*  @return         错误码
*  @note
*/
static XRAY_LIB_HRESULT GetPixPhyLen(Geometry& stGeom, XMat& matPixPhyLen, int32_t nDetec)
{
	XSP_CHECK(matPixPhyLen.IsValid(), XRAY_LIB_XMAT_INVALID);
	XSP_CHECK(XSP_32FC1 == matPixPhyLen.type, XRAY_LIB_XMAT_TYPE_ERR);
	XSP_CHECK(stGeom.GetDetNum() * stGeom.nDetPixels <= uint32_t(matPixPhyLen.wid), XRAY_LIB_IMAGELENGTH_OVERFLOW);

	for (uint32_t n = 0; n < stGeom.GetDetNum(); n++)
	{
		// 依次考虑每个像素的起始位置和结束位置
		DetCoor det_coor = stGeom.GetDetCoor(n);
		Vec2 det_vec;
		det_vec.x = (det_coor.end.x - det_coor.start.x) / stGeom.nDetPixels;
		det_vec.y = (det_coor.end.y - det_coor.start.y) / stGeom.nDetPixels;

		Vec2 pix_start, pix_end;
		float32_t alpha, beta;
		float32_t SOD = stGeom.SOD;
		float32_t pix_phy_len;

		for (uint32_t p = 0; p < stGeom.nDetPixels; p++)
		{
			pix_start.x = det_coor.start.x + p * det_vec.x;
			pix_start.y = det_coor.start.y + p * det_vec.y;

			pix_end.x = det_coor.start.x + (p + 1) * det_vec.x;
			pix_end.y = det_coor.start.y + (p + 1) * det_vec.y;

			alpha = atan(pix_start.y / pix_start.x);
			beta = atan(pix_end.y / pix_end.x);

			/* 主辅视角区分 */
			if (stGeom.isMain)
			{
				pix_phy_len = abs(SOD * (1 / tan(alpha) - 1 / tan(beta)));
			}
			else
			{
				pix_phy_len = abs(SOD * (tan(alpha) - tan(beta)));
			}

			matPixPhyLen.Ptr<float32_t>()[n * stGeom.nDetPixels + p] = pix_phy_len;

		}
	}

	/*特定区域内畸变比例调整，保持图像真实比例*/
	if (stGeom.isMain)
	{
		float32_t fMinLen = *std::min_element(matPixPhyLen.Ptr<float32_t>(), matPixPhyLen.Ptr<float32_t>() + stGeom.GetDetNum() * stGeom.nDetPixels);

		for (int32_t n = nDetec; n < static_cast<int32_t>(stGeom.GetDetNum()); n++)
		{
			for (uint32_t p = 0; p < stGeom.nDetPixels; p++)
			{
				matPixPhyLen.Ptr<float32_t>()[n * stGeom.nDetPixels + p] = fMinLen;
			}
		}
	}
	return XRAY_LIB_OK;
}


/**@fn             GetPixPhyPos
*  @breif          获取探测器像素映射到成像平面的物理坐标
*  @param1         stGeom                [I]         几何光路
*  @param2         matPixPhyLen          [I]         像素物理长度存储矩阵
*  @param3         matPixPhyPos          [O]         像素物理坐标存储矩阵
*  @param4         matPixCorrPos         [O]         像素映射坐标存储矩阵
*  @return         错误码
*  @note
*/
static XRAY_LIB_HRESULT GetPixPos(Geometry& stGeom, XMat& matPixPhyLen, 
	                              XMat& matPixPhyPos, XMat& matPixCorrPos, int32_t nWidth)
{
	XSP_CHECK(matPixPhyLen.IsValid() && matPixPhyPos.IsValid() && matPixCorrPos.IsValid(), XRAY_LIB_XMAT_INVALID);

	XSP_CHECK(XSP_32FC1 == matPixPhyLen.type && XSP_32FC1 == matPixPhyPos.type && XSP_32FC1 == matPixCorrPos.type, XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(matPixPhyLen, matPixPhyPos) && MatSizeEq(matPixPhyLen, matPixCorrPos), XRAY_LIB_XMAT_SIZE_ERR);

	XSP_CHECK(1 == matPixPhyLen.hei && 1 == matPixCorrPos.hei, XRAY_LIB_XMAT_SIZE_ERR);

	XSP_CHECK(stGeom.nDetPixels * stGeom.GetDetNum() <= uint32_t(matPixPhyLen.wid), XRAY_LIB_XMAT_SIZE_ERR);

	/* 这里使用的是实际的板卡数量，像素数可能小于实际条带宽度，多出的部分为无效像素，采传补0 */
	int32_t nWid = nWidth;
	float32_t* pPixPhyLen = matPixPhyLen.Ptr<float32_t>();
	float32_t* pPixPhyPos = matPixPhyPos.Ptr<float32_t>();
	float32_t* pPixCorrPos = matPixCorrPos.Ptr<float32_t>();
	float32_t fPos = 0.0f;
	for (int32_t nw = 0; nw < nWid; nw++)
	{
		pPixPhyPos[nw] = fPos + 0.5f * pPixPhyLen[nw];
		fPos += pPixPhyLen[nw];
	}

	float32_t fStep = (pPixPhyPos[nWid - 1] - pPixPhyPos[0]) / float32_t(nWid - 1);

	for (int32_t nw = 0; nw < nWid; nw++)
	{
		pPixCorrPos[nw] = pPixPhyPos[0] + nw * fStep;
	}

	return XRAY_LIB_OK;
}

/**@fn             AdjustPixPos
*  @breif          获取探测器像素映射到成像平面的物理坐标
*  @param1         matPixPhyPos          [I]         像素物理坐标存储矩阵
*  @param2         matPixCorrPos         [I/O]       像素映射坐标存储矩阵
*  @param3         k                     [I]         畸变调整系数
*  @return         错误码
*  @note           k=0完全畸变校正，k=1无畸变校正，k=0.7介于中间结果
*/
static XRAY_LIB_HRESULT AdjustPixPos(XMat& matPixPhyPos, XMat& matPixCorrPos, float32_t k)
{
	XSP_CHECK(matPixPhyPos.IsValid() && matPixCorrPos.IsValid(), XRAY_LIB_XMAT_INVALID);

	XSP_CHECK(XSP_32FC1 == matPixCorrPos.type && XSP_32FC1 == matPixPhyPos.type,
		      XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(matPixCorrPos, matPixPhyPos), XRAY_LIB_XMAT_SIZE_ERR);

	XSP_CHECK(1 == matPixPhyPos.hei, XRAY_LIB_XMAT_SIZE_ERR);

	XSP_CHECK(0.0f <= k && 1.0f >= k, XRAY_LIB_INVALID_PARAM);

	int32_t nWid = matPixPhyPos.wid;
	float32_t* pPixPhyPos = matPixPhyPos.Ptr<float32_t>();
	float32_t* pPixCorrPos = matPixCorrPos.Ptr<float32_t>();

	for (int32_t nw = 0; nw < nWid; nw++)
	{
		pPixCorrPos[nw] *= pow((pPixPhyPos[nw] / pPixCorrPos[nw]), k);
	}

	return XRAY_LIB_OK;
}

/**@fn             GetGeomTbe
*  @breif          生成畸变校正表
*  @param1         stGeom                [I]         几何光路
*  @param2         matPixPhyPos          [I]         像素物理坐标存储矩阵
*  @param3         matPixCorrPos         [I]         像素映射坐标存储矩阵
*  @param4         stTable               [O]         畸变表输出
*  @return         错误码
*  @note
*/
static XRAY_LIB_HRESULT GetGeomTbe(Geometry& stGeom, XMat& matPixPhyPos, 
	                               XMat& matPixCorrPos, GeoCaliTable& stTable, int32_t nWidth)
{
	XSP_CHECK(matPixPhyPos.IsValid() && matPixCorrPos.IsValid(), XRAY_LIB_XMAT_INVALID);
	XSP_CHECK(stGeom.nDetPixels * stGeom.GetDetNum() <= uint32_t(matPixPhyPos.wid), XRAY_LIB_XMAT_SIZE_ERR);
	XSP_CHECK(XSP_32FC1 == matPixCorrPos.type && XSP_32FC1 == matPixPhyPos.type, XRAY_LIB_XMAT_TYPE_ERR);
	XSP_CHECK(MatSizeEq(matPixPhyPos, matPixCorrPos), XRAY_LIB_XMAT_SIZE_ERR);

	int32_t nWid = nWidth;
	float32_t* pPixPhyPos = matPixPhyPos.Ptr<float32_t>();
	float32_t* pPixCorrPos = matPixCorrPos.Ptr<float32_t>();

	float32_t fPosDelta = 1e-3f * (pPixCorrPos[1] - pPixCorrPos[0]);
	int32_t nNearestIdx = 0;

	/****************************************** 
	*           有效板卡位置映射 
	*******************************************/
	for (int32_t nw = 0; nw < nWid; nw++)
	{
		float32_t fPixCorrPos = pPixCorrPos[nw];

		float32_t fErrAbsMin = FLT_MAX;
		float32_t fErr = 0.0f;

		/* 查找像素位置最接近的索引 */
		for (int32_t nIdx = nNearestIdx; nIdx < nWid; nIdx++)
		{
			float32_t fErrTmp = fPixCorrPos - pPixPhyPos[nIdx];
			if (abs(fErrTmp) < fErrAbsMin)
			{
				fErrAbsMin = abs(fErrTmp);
				fErr = fErrTmp;
				nNearestIdx = nIdx;
			}
			else
			{
				break;
			}
		}

		if (fErrAbsMin > fPosDelta && nNearestIdx > 0 && nNearestIdx < nWid - 1)
		{
			/* 需插值的坐标 */
			float32_t fErr1 = fPixCorrPos - pPixPhyPos[nNearestIdx - 1];
			float32_t fErr2 = fPixCorrPos - pPixPhyPos[nNearestIdx + 1];
			stTable.SetIdxNum(nw, 2);
			if (fErr > 0)
			{
				stTable.IdxPtr(nw)[0] = nNearestIdx;
				stTable.IdxPtr(nw)[1] = nNearestIdx + 1;
				stTable.RatioPtr(nw)[0] = abs(fErr2) / (abs(fErr) + abs(fErr2));
				stTable.RatioPtr(nw)[1] = abs(fErr) / (abs(fErr) + abs(fErr2));
			}
			else
			{
				stTable.IdxPtr(nw)[0] = nNearestIdx - 1;
				stTable.IdxPtr(nw)[1] = nNearestIdx;
				stTable.RatioPtr(nw)[0] = abs(fErr) / (abs(fErr) + abs(fErr1));
				stTable.RatioPtr(nw)[1] = abs(fErr1) / (abs(fErr) + abs(fErr1));
			}
		}
		else
		{
			/* 无需插值的坐标 */
			stTable.SetIdxNum(nw, 1);
			stTable.IdxPtr(nw)[0] = nNearestIdx;
			stTable.RatioPtr(nw)[0] = 1.0;
		}
	} // for nw

	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT CaliModule::ResetGeoCaliTable(Geometry& stGeom, GeoCaliTable& stTable, int32_t nWidth)
{
	XRAY_LIB_HRESULT hr;

	/* 计算单个像素映射在成像平面的物理长度 */
	hr = GetPixPhyLen(stGeom, m_matGeomPixPhyLen, m_nDetecLocatiion);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	/* 计算单个像素物理像素坐标和映射像素坐标 */
	hr = GetPixPos(stGeom, m_matGeomPixPhyLen, m_matGeomPixPhyPos, m_matGeomPixCorrPos, nWidth);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	/* 映射像素坐标微调 */
	hr = AdjustPixPos(m_matGeomPixPhyPos, m_matGeomPixCorrPos, m_fGeomFactor);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	/* 生成校正表 */
	hr = GetGeomTbe(stGeom, m_matGeomPixPhyPos, m_matGeomPixCorrPos, stTable, nWidth);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	/* 读取的几何校正表进行lanzos插值 */
	std::vector<double> pxPos;
	std::vector<isl::px_intp_t> intpLut;
	transCaliTable2Pos(m_caliGeoTbe, pxPos);
	isl::transPos2IntpLut(intpLut, m_caliGeoTbe.nRelatedRatioNum, pxPos, m_caliGeoTbe.nIdxRange);
	transIntpLut2CaliTable(m_caliGeoTbe, intpLut);

	/* 设置当前校正表的实际索引像素范围 */
	stTable.nIdxRange = nWidth;

	// 更新查找表坐标信息
	hr = GetGeoOffsetLut();
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "GetGeoOffsetLut failed.");

	return XRAY_LIB_OK;
}

/**@fn             SetGeoCaliTableFacotr()
*  @breif          设置畸变校正比例因子(比例因子保存在m_fGeomFactor)
*  @return         错误码
*  @note		   重新生成畸变校正表
*/
XRAY_LIB_HRESULT CaliModule::SetGeoCaliTableFacotr()
{
	XRAY_LIB_HRESULT hr;

	XSP_CHECK(m_bGeometryFigExist, XRAY_LIB_GEOCALITABLE_ERROR, 
				"No Geometry Figure Exist, Set Factor Failed.");

	m_stGeometry.nDetPixels = static_cast<int32_t>(m_pSharedPara->m_nPerDetectorWidth);
	hr = ResetGeoCaliTable(m_stGeometry, m_caliGeoTbe, m_pSharedPara->m_nDetectorWidth);
	if (XRAY_LIB_OK != hr)
	{
		m_bGeoCaliCanUse = false;
		log_error("Re Generate GeoCaliTable Failed.");
		return hr;
	}
	m_bGeoCaliCanUse = true;
	log_info("Re Generate GeoCaliTable Successful. Factor %.2f", m_fGeomFactor);

	return XRAY_LIB_OK;
}
/**@fn             GetCorrrctMatNoise
*  @breif          获取校正后的图像噪声
*  @param1         matIn                 [I]         输入图像
*  @param2         nUpdateMode           [I]         更新模式
*  @param3         nEnergyType           [I]         能量类型
*  @param4         pCorrctNoise          [O]         校正后的图像噪声
*  @return         错误码
*  @note           输入图像必须为16位灰度图像
*/
XRAY_LIB_HRESULT CaliModule::GetCorrrctMatNoise(XMat& matIn, XRAYLIB_UPDATE_ZEROANDFULL_MODE nUpdateMode, XRAYLIB_ENERGY_MODE nEnergyType, float32_t* pCorrctNoise)
{
	XRAY_LIB_HRESULT hr;
	XSP_CHECK(matIn.IsValid(), XRAY_LIB_XMAT_INVALID);
	int32_t nWid = matIn.wid;
	// 如果传入图像大于校正图像缓存，则进行裁剪
	int32_t nHei = matIn.hei > XSP_MODCALI_CORRECT_HEIGHT ? XSP_MODCALI_CORRECT_HEIGHT : matIn.hei;

	XMat FullMat = (nEnergyType == XRAYLIB_ENERGY_LOW ) ?  m_matLowFull : m_matHighFull;
	XMat ZeroMat = (nEnergyType == XRAYLIB_ENERGY_LOW ) ?  m_matLowZero : m_matHighZero;
	uint16_t *pOriData, *pCaliData, *pZero, *pFull;		
	float32_t fStd = 0.0f, fMean = 0.0f;

	hr = m_matCorrectTemp.Reshape(nHei , nWid, XSP_16U);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matCorrectTemp reshape is err.");

	// 当输入校正数据为满载数据时，计算标准差
	if(XRAYLIB_UPDATEFULL == nUpdateMode)
	{
		// matIn进行归一化处理
		for(int32_t nh = 0; nh < nHei; nh++)
		{
			pZero = ZeroMat.Ptr<uint16_t>();
			pFull = FullMat.Ptr<uint16_t>();
			pOriData = matIn.Ptr<uint16_t>(nh);
			pCaliData = m_matCorrectTemp.Ptr<uint16_t>(nh);

			for (int32_t nw = 0; nw < nWid; nw++)
			{
				float32_t fRealAir = (float32_t)(*pFull) - (*pZero);
				if(fRealAir - 0.0f < 1e-6f)
				{
					fRealAir = 1.0f;
				}
				float32_t fValue = 65535.0f * (float32_t(*pOriData - *pZero) * 0.97f / fRealAir);

				*pCaliData = (uint16_t)Clamp(fValue, 0.0f, 65535.0f);

				if(nw < m_nWhitenUp || nw > nWid - m_nWhitenDown)
				{
					*pCaliData = (uint16_t)m_pSharedPara->m_nBackGroundGray;
				}

				pCaliData++;
				pOriData++;
				pZero++;
				pFull++;
			}
		}

		for(int32_t i = 0; i < nHei; i++)
		{
			pCaliData = m_matCorrectTemp.Ptr<uint16_t>(i);
			for (int32_t j = m_nWhitenUp; j < nWid - m_nWhitenDown; j++)
			{
				fMean += pCaliData[j];
			}
		}
		fMean /= ((nWid - m_nWhitenUp - m_nWhitenDown)*nHei);

		// 计算matIn的标准差
		for(int32_t i = 0; i < nHei; i++)
		{
			pCaliData = m_matCorrectTemp.Ptr<uint16_t>(i);
			for (int32_t j = m_nWhitenUp; j < nWid - m_nWhitenDown; j++)
			{
				fStd += (pCaliData[j] - fMean)*(pCaliData[j] - fMean);
			}
		}
		fStd = sqrt(fStd / ((nWid - m_nWhitenUp - m_nWhitenDown)*nHei));

		if((fStd > FLT_EPSILON) && (fStd < 5000.0f))
		{
			*pCorrctNoise = fStd;
			return XRAY_LIB_OK;
		}

	}

	return XRAY_LIB_XMAT_INVALID;
}
