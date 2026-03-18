#include "xsp_mod_dual.hpp"
#include "core_def.hpp"
#include "xsp_alg.hpp"
#include <math.h>


/* BGRA背景色设置 */
#define IMG_BG_A_VAL(x)			((x) >> 24 & 0xFF)    // A通道
#define IMG_BG_R_VAL(x)			((x) >> 16 & 0xFF)    // R通道
#define IMG_BG_G_VAL(x)			((x) >> 8 & 0xFF)     // G通道
#define IMG_BG_B_VAL(x)			((x) >> 0 & 0xFF)     // B通道
#define IMG_BG_BGRA_DEF_VAL		(0xFFFFFFFF)          // BGRA默认背景值

/* YUV背景色设置 */
#define IMG_BG_Y_VAL(x)			((x) >> 16 & 0xFF)    // Y通道
#define IMG_BG_U_VAL(x)			((x) >> 8 & 0xFF)     // U通道
#define IMG_BG_V_VAL(x)			((x) >> 0 & 0xFF)     // V通道
#define IMG_BG_YUV_DEF_VAL		(0xFFEB8080)          // YUV默认背景值
#define CSMO_NEIGHBOR			(2)					  // 色域平滑需要使用的邻域					


DualModule::DualModule()
{
	m_enColorMode = XRAYLIB_DUALCOLOR;
	m_enInverseMode = XRAYLIB_INVERSE_NONE;
	m_enSingleColorTableSel = XRAYLIB_SINGLECOLORTABLE_SEL1;
	m_enDualColorTableSel = XRAYLIB_DUALCOLORTABLE_SEL2;
	m_enColorImagingSel = XRAYLIB_COLORSIMAGING_3S;

	m_enPenetrationMode = XRAYLIB_PENETRATION_NORMAL;

	m_nWhiteUpBs = 0;
	m_nWhiteDownBs = 0;

	m_nGrayMin = 0;
	m_nGrayMax = 65535;
	m_nGrayLevel = 0;
	m_nBrightnessAdjust = 0;

	m_nDualDistinguishGrayUp = 62800;
	m_nDualDistinguishGrayDown = 200;
	m_nDualDistinguishNoneInorganic = 54000;  //双能分辨无机物上限
	m_nDualDistinguishNoneMixture = 400;      //双能分辨混合物下限

	m_fMaterialAdjust = 0;
    m_nMixtureRangeDownAdj = 0;
    m_nMixtureRangeUpAdj = 0;

	m_nBKColorARGB = IMG_BG_BGRA_DEF_VAL;
	m_nBKLuminance = 0xFF;

	m_bZ6CanUse = false;

	m_fpeneMergeParm = 0.5f;
	m_fHpeneRatio = 0.75f;
	m_fLpeneRatio = 0.25f;

	m_stTestbParams.fTest5RatioCN = 0.0f;
	m_stTestbParams.nRTuneNum = 0;
	m_stTestbParams.nGrayTuneNum = 0;

	// 颜色微调参数初始化
	m_stTestbParams.stRCurveAdjust.nAlGrayHighFactor = 50;
	m_stTestbParams.stRCurveAdjust.nAlGrayLowFactor = 50;
	m_stTestbParams.stRCurveAdjust.nCGrayHighFactor = 50;
	m_stTestbParams.stRCurveAdjust.nCGrayLowFactor = 50;
	m_stTestbParams.stRCurveAdjust.nFeGrayHighFactor = 50;
	m_stTestbParams.stRCurveAdjust.nFeGrayLowFactor = 50;

	m_enCurveState = XRAYLIB_OFF; // 默认曲线自动标定未开启
	m_enTestAutoLE = XRAYLIB_OFF;
	m_enZcorrparState = XRAYLIB_OFF;

	m_bUseTAVZTable = false;
}

XRAY_LIB_HRESULT DualModule::Init(const char* szPublicFileFolderName,
	                              const char* szZTableName,
	                              const char* szZ6TableName,
	                              SharedPara* pPara,
	                              XRAY_LIB_VISUAL visual,
	                              XRAY_DETECTORNAME detectorName,
	                              XRAY_DEVICETYPE devicetype)
{
	XRAY_LIB_HRESULT hr;

	m_pSharedPara = pPara;
	m_enDevicetype = devicetype;
	m_enDetectorName = detectorName;

	hr = ClearFilePath();
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	if (XRAYLIB_ENERGY_SCATTER == pPara->m_enEnergyMode)
	{
		return XRAY_LIB_OK;
	}
	else if (XRAYLIB_ENERGY_DUAL == pPara->m_enEnergyMode)
	{
		hr = InitZTableFilePath(szPublicFileFolderName, szZTableName, szZ6TableName, visual);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		hr = InitZTable();
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
	}

	/* 民航设备判断 */
	if (XRAY_XRAYDV_DTCA == detectorName || 0x002 == (m_pSharedPara->m_enDeviceInfo.xraylib_devicetype & 0x00F))
	{
		hr = InitColorTableCAFilePath(szPublicFileFolderName);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		InitColorTableCA();
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
	}
	else
	{
		hr = InitColorTableFilePath(szPublicFileFolderName);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		InitColorTable();
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
	}

	hr = ColorTbeSelected();
	XSP_CHECK(XRAY_LIB_OK == hr, hr);


	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::ReInit(const char* szPublicFileFolderName,
	                                const char* szZTableName,
	                                const char* szZ6TableName,
	                                XRAY_LIB_VISUAL visual)
{
	XRAY_LIB_HRESULT hr;
	hr = ClearFilePath();
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr = InitZTableFilePath(szPublicFileFolderName, szZTableName, szZ6TableName, visual);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr = InitZTable();
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	return XRAY_LIB_OK;
}


/* 原子序数计算流程
 *
 * 						   | 	     	|-----6color-----六色颜色表(Z8Identify6S)
 * 						   | 	     	|
 * 						   | 	     	|				 |-----hue128-----民航颜色表(Z8IdentifyCA)
 * 						   | 	     	| 				 |
 * 						   | 	     	|                |                |-----利用总衰减值计算(Z8IdentifyByTAV)              
 * 						   |-----8z-----|-----3color-----|                |
 * 						   | 	     	| 				 |-----hue44------|
 * 						   | 	     	|				 |                |
 * 						   | 	     	|				 |                |-----利用低能和R值表计算(Z8IdentifyByLow)
 * 						   | 	     	|				 |
 * 						   | 	     	|
 * 			ZIdentify -----|
 * 						   | 
 * 						   | 	     	|-----利用总衰减值计算(Z16IdentifyByTAV)
 * 						   | 	     	|
 * 						   |----16z-----|
 * 						   | 	     	| 
 * 						   | 	     	|-----利用低能和R值表计算(Z16IdentifyByLow)
 * 							
*/
XRAY_LIB_HRESULT DualModule::ZIdentify(XMat& highData, XMat& lowData, XMat& zData)
{
	XRAY_LIB_HRESULT hr;
	int32_t nPadLen = ((lowData.tpad >= 2) && (lowData.bpad >= 2)) ? 2 : 0;

	if (XSP_16UC1 == zData.type)
	{
		hr = m_bUseTAVZTable ? Z16IdentifyByTAV(highData, lowData, zData, nPadLen) : 
									Z16IdentifyByLow(highData, lowData, zData, nPadLen);
	}
	else if (XSP_8UC1 == zData.type)
	{
		if (XRAYLIB_COLORSIMAGING_6S == m_enColorImagingSel)
		{
			hr = Z8Identify6S(highData, lowData, zData, nPadLen);
		}
		else if (XRAYLIB_COLORSIMAGING_3S == m_enColorImagingSel)
		{
			if (128 == m_stColorTbe.hdr.hueNum)
			{
				hr = Z8IdentifyCA(highData, lowData, zData, nPadLen);
			}
			else if (44 == m_stColorTbe.hdr.hueNum)
			{
				hr = m_bUseTAVZTable ? Z8IdentifyByTAV(highData, lowData, zData, nPadLen):
										Z8IdentifyByLow(highData, lowData, zData, nPadLen);
			}
			else
			{
				log_error("color tbe hue num %d error.", m_stColorTbe.hdr.hueNum);
				return XRAY_LIB_INVALID_PARAM;
			}
		}
		else
		{
			log_error("colorsimaging type 0x%x error.", m_enColorImagingSel);
			return XRAY_LIB_INVALID_PARAM;
		}
	}
	else
	{
		log_error("zData type 0x%x error.", zData.type);
		return XRAY_LIB_XMAT_TYPE_ERR;
	}

	return hr;
}


/**
 * @fn      GetColorImageRgb
 * @brief   根据灰度图、原子序数图和当前选择的颜色模式，伪彩化为RGB颜色空间的图像
 * 
 * @param   [OUT] rgbOut RGB图像，格式为RGB888
 * @param   [IN] gray 融合灰度图
 * @param   [IN] z 原子序数图，尺寸需与gray相同
 * @param   [IN] wt 处理权重图，255为包裹区，0为背景区，其他为过渡区
 * @param   [IN] procWins 需处理的矩形区域集合
 * 
 * @return  XRAY_LIB_HRESULT 
 */
XRAY_LIB_HRESULT DualModule::GetColorImageRgb(XMat& rgbOut, XMat& gray, XMat& z, XMat& wt, std::vector<XRAYLIB_RECT>& procWins)
{
	/* check para */
	XSP_CHECK(gray.IsValid() && z.IsValid() && rgbOut.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");
	XSP_CHECK(m_stColorTbe.pColor, XRAY_LIB_NULLPTR, "Color table ptr is null.");
	XSP_CHECK(MatSizeEq(gray, z) && MatSizeEq(gray, wt), XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");
    XSP_CHECK(gray.hei - gray.tpad - gray.bpad == rgbOut.hei - rgbOut.tpad - rgbOut.bpad && gray.wid == rgbOut.wid, XRAY_LIB_XMAT_SIZE_ERR);
	XSP_CHECK(XSP_8UC3 == rgbOut.type, XRAY_LIB_XMAT_TYPE_ERR);
	XSP_CHECK(XSP_16U == gray.type && XSP_8U == z.type && XSP_8U == wt.type, XRAY_LIB_XMAT_TYPE_ERR);

	isl::PipeChroma::yuv4p_t islYuvOut(rgbOut.wid, rgbOut.hei, (void*)m_matIslYuvOut.data.ptr, {rgbOut.tpad, rgbOut.bpad});
    isl::PipeChroma::yuv4p_t islYuvTmp(gray.wid, gray.hei, (void*)m_matIslYuvTmp.data.ptr, {gray.tpad, gray.bpad});
    isl::PipeChroma::rgb8c_t islRgbOut(rgbOut);
    isl::Imat<uint16_t> grayIn(gray); // gray与z实际处理区域上下需有2行邻域
    isl::Imat<uint8_t> zIn(z);
    isl::Imat<uint8_t> wtIn(wt);
    std::vector<isl::Rect<int32_t>> packArea;

    m_islPipeChroma.PaintZColorImage(islRgbOut, islYuvOut, islYuvTmp, grayIn, zIn, wtIn, packArea, m_stColorTbe.szZMapUsed,
        (XRAYLIB_INVERSE_NONE == m_enInverseMode) ? std::make_pair(m_nGrayMin, m_nGrayMax) : std::make_pair(m_nGrayMax, m_nGrayMin));

	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT DualModule::GetColorImageAiYuv(XMat& merge, XMat& zData, XMat& yTemp, XMat& uvTemp)
{
	/* check para */
	XSP_CHECK(merge.IsValid() && zData.IsValid() && yTemp.IsValid() && uvTemp.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(m_stColorTbeAiYuv.pColor, XRAY_LIB_NULLPTR, "Color table ptr is null.");

	XSP_CHECK(MatSizeEq(merge, zData), XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XSP_CHECK((merge.hei - merge.tpad * 2) <= yTemp.hei && merge.wid == yTemp.wid,
		      XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XSP_CHECK(yTemp.hei == uvTemp.hei * 2 && yTemp.wid == uvTemp.wid * 2,
		      XRAY_LIB_XMAT_SIZE_ERR, "The size of Y and UV not match");

	XSP_CHECK(XSP_16U == merge.type && XSP_8U == zData.type &&
		      XSP_8UC1 == yTemp.type && XSP_8UC2 == uvTemp.type, 
		      XRAY_LIB_XMAT_TYPE_ERR, "merge.type: %d, zData.type: %d, yTemp.type: %d, uvTemp.type: %d", merge.type, zData.type, yTemp.type, uvTemp.type);

	/* 窗宽窗位选择 */
	int32_t nGrayTemp, nMergeGray, nGrayLevel;

	int32_t nMaxGrayLevel = m_stColorTbeAiYuv.hdr.yLevel;
	int32_t nScale = 65536 / nMaxGrayLevel;

	int32_t nBackGroundThreshold, nBackGroundGray;
	nBackGroundThreshold = 62000;
	nBackGroundGray = 65535;

	uint16_t* pMerge = merge.Ptr<uint16_t>(merge.tpad);
	uint8_t* pZData = zData.Ptr<uint8_t>(zData.tpad);
	uint8_t* pYTemp = yTemp.Ptr<uint8_t>();
	uint8_t* pUVTemp = uvTemp.Ptr<uint8_t>();
	int32_t nRawHei = merge.hei - merge.tpad * 2;
	for (int32_t nh = 0; nh < yTemp.hei; nh++)
	{
		for (int32_t nw = 0; nw < yTemp.wid; nw++)
		{
			/* 超过raw的高度，直接赋值255 */
			if (nh >= nRawHei)
			{
				pYTemp[0] = 235;
				if (nh % 2 == 0 && nw % 2 == 0)
				{
					pUVTemp[0] = 128;
					pUVTemp[1] = 128;
					pUVTemp += XSP_ELEM_SIZE(XSP_8UC2);
				}
				pYTemp++;
				continue;
			}
			/* 背景判断 */
			nMergeGray = (*pMerge) >(uint16_t)nBackGroundThreshold ? nBackGroundGray : (*pMerge);
			if (nw == 0) nMergeGray = (uint16_t)nBackGroundGray;
			nGrayTemp = nMergeGray;
			nGrayLevel = (int32_t)(nGrayTemp / nScale);

			/**********************************
			*             YUV赋值
			***********************************/
			uint8_t* pColor = m_stColorTbeAiYuv.Ptr(int32_t(*pZData), nGrayLevel);

			/* Y分量赋值 ,保证与工具一致，不设置最大最小值判断*/ 
			pYTemp[0] = pColor[0];

			/* UV分量赋值 */
			if (nh % 2 == 0 && nw % 2 == 0)
			{
				pUVTemp[0] = pColor[2];
				pUVTemp[1] = pColor[1];
				/* uv分量偏移 */
				pUVTemp += XSP_ELEM_SIZE(XSP_8UC2);
			}

			/* 指针偏移 */
			pMerge++;
			pZData++;
			pYTemp++;
		}
	}

	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT DualModule::ColorTbeSelected()
{
	XSP_CHECK(m_enColorImagingSel >= XRAYLIB_COLORSIMAGING_MIN_VALUE &&
		m_enColorImagingSel <= XRAYLIB_COLORSIMAGING_MAX_VALUE,
		XRAY_LIB_INVALID_PARAM, "ColorImageSel err");

	XSP_CHECK(m_enDualColorTableSel >= XRAYLIB_DUALCOLORTABLE_MIN_VALUE &&
		m_enDualColorTableSel <= XRAYLIB_DUALCOLORTABLE_MAX_VALUE,
		XRAY_LIB_INVALID_PARAM, "DualColorTableSel err");

	if (XRAYLIB_COLORSIMAGING_3S == m_enColorImagingSel)
	{
		/* 默认颜色表选取 */
		ColorTable& colortbe = m_stColorTbeRgb[int32_t(m_enDualColorTableSel)];
		m_islPipeChroma.SetZColorTable(colortbe.pColor, colortbe.hdr.yLevel, colortbe.hdr.hueNum);
		InitColorTableZMap();
		ColorTableCopy(m_stColorTbe, m_stColorTbeRgb[int32_t(m_enDualColorTableSel)]);
	}
	else if (XRAYLIB_COLORSIMAGING_6S == m_enColorImagingSel)
	{
		/* 6色双能颜色表选取 */
		ColorTable& colortbe = m_stColorTbeZ6Rgb;
		m_islPipeChroma.SetZColorTable(colortbe.pColor, colortbe.hdr.yLevel, colortbe.hdr.hueNum);
		InitColorTableZMap();
		ColorTableCopy(m_stColorTbe, m_stColorTbeZ6Rgb);
	}
	else
	{
		return XRAY_LIB_ENERGYMODE_ERROR;
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::Release()
{
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::GetMemSize(XRAY_LIB_MEM_TAB &MemTab, XRAY_LIB_ABILITY &ability)
{
	MemTab.size = 0;
	/******************************************************
	*                 颜色表及Z值表内存
	*******************************************************/
	if (XRAYLIB_ENERGY_SCATTER == ability.nEnergyMode)
	{
		return XRAY_LIB_OK;
	}
	else
	{
		/* 实际使用的颜色表内存，按民航颜色表内存大小申请 */
		m_stColorTbe.SetMem(44, 1024, 3);
		XspMalloc((void**)&m_stColorTbe.pColor, m_stColorTbe.Size(), MemTab);

		/* Ai路内存 */
		m_stColorTbeAiYuv.SetMem(45, 1024, 3);
		XspMalloc((void**)&m_stColorTbeAiYuv.pColor, m_stColorTbeAiYuv.Size(), MemTab);

		/* 正常颜色表内存 */
		for (int32_t i = 0; i < XRAYLIB_DUALCOLORTABLE_NUM; i++)
		{
			m_stColorTbeRgb[i].SetMem(44, 1024, 3);

			XspMalloc((void**)&m_stColorTbeRgb[i].pColor, m_stColorTbeRgb[i].Size(), MemTab);
		}

		/* 正常6色颜色表内存 */
		m_stColorTbeZ6Rgb.SetMem(44, 1024, 3);
		XspMalloc((void**)&m_stColorTbeZ6Rgb.pColor, m_stColorTbeZ6Rgb.Size(), MemTab);



		/* 单能虚拟Z值查找表内存 */
		m_stSingleZMapTbe.SetMem(44, 1024, 1);
		XspMalloc((void**)&m_stSingleZMapTbe.pZValue, m_stSingleZMapTbe.Size(), MemTab);

		/* 分类曲线内存 */
		m_stZTable.len = 65536;
		XspMalloc((void**)&m_stZTable.pC,  m_stZTable.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZTable.pH2O, m_stZTable.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZTable.pAl, m_stZTable.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZTable.pFe, m_stZTable.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZTable.pDiffThr, m_stZTable.len * sizeof(float64_t), MemTab);

		/* 分类曲线备份内存 */
		m_stZTable_bak.len = 65536;
		XspMalloc((void**)&m_stZTable_bak.pC,  m_stZTable_bak.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZTable_bak.pH2O, m_stZTable_bak.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZTable_bak.pAl, m_stZTable_bak.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZTable_bak.pFe, m_stZTable_bak.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZTable_bak.pDiffThr, m_stZTable_bak.len * sizeof(float64_t), MemTab);

		/* 6色分类曲线内存 */
		m_stZ6Table.len = 65536;
		XspMalloc((void**)&m_stZ6Table.pPE, m_stZ6Table.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZ6Table.pPOM, m_stZ6Table.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZ6Table.pPEAL, m_stZ6Table.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZ6Table.pFeAl, m_stZ6Table.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZ6Table.pAl, m_stZ6Table.len * sizeof(float64_t), MemTab);
		XspMalloc((void**)&m_stZ6Table.pFe, m_stZ6Table.len * sizeof(float64_t), MemTab);

		/******************************************************
		*                      整屏相关 
		*******************************************************/
		int32_t nEntrieProcessHeight = XRAY_LIB_MAX_IMGENHANCE_LENGTH + XRAY_LIB_MAX_FILTER_KERNEL_LENGTH * 2;
		int32_t nEntrieProcessWidth = int32_t(ability.nDetectorWidth * ability.fResizeScale);

		m_matEntireZDataSelected.SetMem(nEntrieProcessHeight * nEntrieProcessWidth * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_matEntireZDataSelected.data, m_matEntireZDataSelected.Size(), MemTab);

		m_matIslYuvOut.SetMem(nEntrieProcessHeight * nEntrieProcessWidth * XSP_ELEM_SIZE(XSP_8UC3));
		XspMalloc((void**)&m_matIslYuvOut.data, m_matIslYuvOut.Size(), MemTab);

		m_matIslYuvTmp.SetMem(nEntrieProcessHeight * nEntrieProcessWidth * XSP_ELEM_SIZE(XSP_8UC3));
		XspMalloc((void**)&m_matIslYuvTmp.data, m_matIslYuvTmp.Size(), MemTab);

		/*原子序数通道补偿曲线*/
		m_stCorrparTable.len = ability.nDetectorWidth;
		XspMalloc((void**)&m_stCorrparTable.pZcorrpar, m_stCorrparTable.len  * sizeof(float32_t), MemTab);
	}
	return XRAY_LIB_OK;
}


/**************************************************************************
*                            初始化相关
***************************************************************************/
XRAY_LIB_HRESULT DualModule::ClearFilePath()
{
	int32_t nPathLength = XRAY_LIB_MAX_PATH + XRAY_LIB_MAX_NAME;
	for (int32_t i = 0; i < XRAYLIB_DUALCOLORTABLE_NUM; i++)
	{
		memset(m_szDualColorPath[i], 0, sizeof(int8_t) * nPathLength);
	}
	
	memset(m_szDualZ6ColorPath, 0, sizeof(int8_t) * nPathLength);
	memset(m_szSingleZMapTbePath, 0, sizeof(int8_t) * nPathLength);
	memset(m_szDualColorAiPath, 0, sizeof(int8_t) * nPathLength);
	memset(m_szPresetZTablePath, 0, sizeof(int8_t) * nPathLength);
	memset(m_szAutoZTablePath, 0, sizeof(int8_t) * nPathLength);
	memset(m_szCorrparZTablePath, 0, sizeof(int8_t) * nPathLength);
	
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::InitZTableFilePath(const char* szPublicFileFolderName,
												const char* szZTableName,
												const char* szZ6TableName,
												XRAY_LIB_VISUAL visual)
{
	XSP_CHECK(szPublicFileFolderName, XRAY_LIB_NULLPTR, "Public path is null.");
	XSP_CHECK(szZTableName, XRAY_LIB_NULLPTR, "ZTable name is null.");
	/******************************************************
	*                  ZTable路径初始化
	*******************************************************/
	/* 预置的ZTable路径 */
	strcpy(m_szPresetZTablePath, szPublicFileFolderName);
	strcat(m_szPresetZTablePath, "/ZTable/");
	strcat(m_szPresetZTablePath, szZTableName);

	/* 预置的Z6Table路径 */
	strcpy(m_szPresetZ6TablePath, szPublicFileFolderName);
	strcat(m_szPresetZ6TablePath, "/Z6Table/");
	strcat(m_szPresetZ6TablePath, szZ6TableName);

	/* 自动补偿曲线路径 */
	strcpy(m_szCorrparZTablePath, szPublicFileFolderName);
	strcat(m_szCorrparZTablePath, "/ZTable/");
	if (XRAYLIB_VISUAL_MAIN == visual)
	{
		strcat(m_szCorrparZTablePath, "zcorrpar_main.tbe");
	}
	else if (XRAYLIB_VISUAL_AUX == visual)
	{
		strcat(m_szCorrparZTablePath, "zcorrpar_aux.tbe");
		
	}
	else if (XRAYLIB_VISUAL_SINGLE == visual)
	{
		strcat(m_szCorrparZTablePath, "zcorrpar_single.tbe");
	}
	else
	{
		return XRAY_LIB_VISUALVIEW_ERROR;
	}

	/* AutoZTable路径 */
	strcpy(m_szAutoZTablePath, szPublicFileFolderName);
	strcat(m_szAutoZTablePath, "/FactoryCaliTable/");
	if (XRAYLIB_VISUAL_MAIN == visual)
	{
		strcat(m_szAutoZTablePath, "cali_curve_main.tbe");
	}
	else if (XRAYLIB_VISUAL_AUX == visual)
	{
		strcat(m_szAutoZTablePath, "cali_curve_aux.tbe");
	}
	else if (XRAYLIB_VISUAL_SINGLE == visual)
	{
		strcat(m_szAutoZTablePath, "cali_curve_single.tbe");
	}
	else
	{
		return XRAY_LIB_VISUALVIEW_ERROR;
	}

	log_info("ZTablePath path : %s", m_szPresetZTablePath);
	log_info("Z6TablePath path : %s", m_szPresetZ6TablePath);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::InitColorTableFilePath(const char* szPublicFileFolderName)
{
	/******************************************************
	*                     路径赋值
	*******************************************************/
	/* 正常双能色表路径 */
	for (int32_t i = 0; i < XRAYLIB_DUALCOLORTABLE_NUM; i++)
	{
		strcpy(m_szDualColorPath[i], szPublicFileFolderName);
	}
	/* 常规 */
	strcat(m_szDualColorPath[0], "/DualColorTable/1/colorTableRGB.YUV");
	strcat(m_szDualColorPath[1], "/DualColorTable/2/colorTableRGB.YUV");
	strcat(m_szDualColorPath[2], "/DualColorTable/3/colorTableRGB.YUV");
	strcat(m_szDualColorPath[3], "/DualColorTable/4/colorTableRGB.YUV");
	strcat(m_szDualColorPath[4], "/DualColorTable/Pseudo/colorTableRGB.YUV");

	/* 6色双能颜色表路径 */
	strcpy(m_szDualZ6ColorPath, szPublicFileFolderName);
	/* 常规 */
	strcat(m_szDualZ6ColorPath, "/Dual6ColorTable/1/colorTable.YUV");

	/* Ai路双能色表路径 */
	strcpy(m_szDualColorAiPath, szPublicFileFolderName);
	if (XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{
		strcat(m_szDualColorAiPath, "/DualColorTable/AI/colorTableAI.YUV");
	}
	else
	{
		strcat(m_szDualColorAiPath, "/SingleColorTable/AI/SingleColorTable.YUV");
	}


	/* 单能Z值映射曲线路径 */
	strcpy(m_szSingleZMapTbePath, szPublicFileFolderName);
	strcat(m_szSingleZMapTbePath, "/SingleColorTable/MaterialMap/SingleZMapTbe.TBE");

	for (int32_t i = 0; i < XRAYLIB_DUALCOLORTABLE_NUM; i++)
	{
		log_debug("m_szDualColorPath [%d]: %s", i, m_szDualColorPath[i]);
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::InitColorTableCAFilePath(const char* szPublicFileFolderName)
{
	/******************************************************
	*                     路径赋值
	*******************************************************/
	/* 民航双能色表路径 */
	for (int32_t i = 0; i < XRAYLIB_DUALCOLORTABLE_NUM; i++)
	{
		strcpy(m_szDualColorPath[i], szPublicFileFolderName);
	}
	/* 常规 */
	strcat(m_szDualColorPath[0], "/DualCAColorTable/1/colorTableRGB.YUV");
	strcat(m_szDualColorPath[1], "/DualCAColorTable/2/colorTableRGB.YUV");
	strcat(m_szDualColorPath[2], "/DualCAColorTable/3/colorTableRGB.YUV");
	strcat(m_szDualColorPath[3], "/DualCAColorTable/4/colorTableRGB.YUV");

	/* Ai路双能色表路径 */
	strcpy(m_szDualColorAiPath, szPublicFileFolderName);
	strcat(m_szDualColorAiPath, "/DualCAColorTable/AI/colorTableAI.YUV");

	for (int32_t i = 0; i < XRAYLIB_DUALCOLORTABLE_NUM; i++)
	{
		log_debug("m_szDualColorPath [%d]: %s", i, m_szDualColorPath[i]);
	}

	return XRAY_LIB_OK;
}

// 初始化ZTable
XRAY_LIB_HRESULT DualModule::InitZTable()
{
	XRAY_LIB_HRESULT hr;

	/* 初始化ZTable内存 */ 
	m_stZTable.InitMem();
	m_stZTable_bak.InitMem();

	/* 读取物质分类表 */
	hr = ReadZTable(m_szPresetZTablePath, m_szAutoZTablePath);
	hr = ReadZcorrparTable(m_szCorrparZTablePath, m_stCorrparTable);

	/* ZTable曲线数据进行备份 */ 
	memcpy(m_stZTable_bak.pC, m_stZTable.pC, m_stZTable.len * sizeof(double));
	memcpy(m_stZTable_bak.pAl, m_stZTable.pAl, m_stZTable.len * sizeof(double));
	memcpy(m_stZTable_bak.pFe, m_stZTable.pFe, m_stZTable.len * sizeof(double));

	/* 读取6色R曲线 */
	if (m_bZ6CanUse)
	{
		hr = ReadZ6Table(m_szPresetZ6TablePath, m_stZ6Table);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Read z6table failed.");
	}

	return XRAY_LIB_OK;
}

// 微调ZTable，目前仅支持三色R曲线微调
XRAY_LIB_HRESULT DualModule::AdjustZTable(XRAYLIB_RCURVE_ADJUST_PARAMS xraylib_rcurveparams)
{
	log_debug("xraylib_rcurveparams.nAlGrayLowFactor = %d", xraylib_rcurveparams.nAlGrayLowFactor);
	log_debug("xraylib_rcurveparams.nAlGrayHighFactor = %d", xraylib_rcurveparams.nAlGrayHighFactor);
	log_debug("xraylib_rcurveparams.nFeGrayLowFactor = %d", xraylib_rcurveparams.nFeGrayLowFactor);
	log_debug("xraylib_rcurveparams.nFeGrayHighFactor = %d", xraylib_rcurveparams.nFeGrayHighFactor);

	// 最大调整区间,前一个值为调整区间中值，后一个值为调整区间范围,后面复用为调整区间，Al和C的调整区间相同
	int32_t nAlHighGraySection[2] = {50000,20000};
	int32_t nFeHighGraySection[2] = {40000,20000};
	int32_t nAlLowGraySection[2] = {500,5000};
	int32_t nFeLowGraySection[2] = {500,5000};
	int32_t nAlHighStep = nAlHighGraySection[1];
	int32_t nAlLowStep = nAlLowGraySection[1];
	int32_t nFeHighStep = nFeHighGraySection[1];
	int32_t nFeLowStep = nFeLowGraySection[1];

	// 将拉伸系数0-100转换为实际拉伸系数-1-1
	double fAlGrayLowFactor = (float32_t)(xraylib_rcurveparams.nAlGrayLowFactor - 50) / 50.0f;
	double fAlGrayHighFactor = (float32_t)(xraylib_rcurveparams.nAlGrayHighFactor - 50) / 50.0f;
	double fFeGrayLowFactor = (float32_t)(xraylib_rcurveparams.nFeGrayLowFactor - 50) / 50.0f;
	double fFeGrayHighFactor = (float32_t)(xraylib_rcurveparams.nFeGrayHighFactor - 50) / 50.0f;
	double fCGrayLowFactor = (float32_t)(xraylib_rcurveparams.nCGrayLowFactor - 50) / 50.0f;
	double fCGrayHighFactor = (float32_t)(xraylib_rcurveparams.nCGrayHighFactor - 50) / 50.0f;

	// 还原设备初始R曲线
	if(fAlGrayLowFactor == 0 && fAlGrayHighFactor == 0 && fFeGrayLowFactor == 0 && fFeGrayHighFactor == 0 && fCGrayLowFactor == 0 && fCGrayHighFactor == 0)
	{
		memcpy(m_stZTable.pC, m_stZTable_bak.pC, m_stZTable.len * sizeof(double));
		memcpy(m_stZTable.pAl, m_stZTable_bak.pAl, m_stZTable.len * sizeof(double));
		memcpy(m_stZTable.pFe, m_stZTable_bak.pFe, m_stZTable.len * sizeof(double));
		return XRAY_LIB_OK;
	}

	// 根据微调系数来自适应区间 
	nAlHighGraySection[1] = nAlHighGraySection[0] + nAlHighStep * abs(fAlGrayHighFactor);
	nAlHighGraySection[0] = nAlHighGraySection[0] - nAlHighStep * abs(fAlGrayHighFactor);
	nAlLowGraySection[1] = nAlLowGraySection[0] + nAlLowStep * abs(fAlGrayLowFactor);
	nAlLowGraySection[0] = nAlLowGraySection[0] - nAlLowStep * abs(fAlGrayLowFactor);
	nFeHighGraySection[1] = nFeHighGraySection[0] + nFeHighStep * abs(fFeGrayHighFactor);
	nFeHighGraySection[0] = nFeHighGraySection[0] - nFeHighStep * abs(fFeGrayHighFactor);
	nFeLowGraySection[1] = nFeLowGraySection[0] + nFeLowStep * abs(fFeGrayLowFactor);
	nFeLowGraySection[0] = nFeLowGraySection[0] - nFeLowStep * abs(fFeGrayLowFactor);

	// 暂存归一化值
	double normali = 0;
	// Al,C高灰度区间R值调整
	for(int32_t i = nAlHighGraySection[0]; i < nAlHighGraySection[1]; i++)
	{	
		if(i > 65535)
		{
			break;
		}
		normali = (i - nAlHighGraySection[0]) / (double)(nAlHighGraySection[1] - nAlHighGraySection[0]);
		m_stZTable.pAl[i] = m_stZTable_bak.pAl[i] * ( 1 + 0.25 * fAlGrayHighFactor - fAlGrayHighFactor * pow((normali - 0.5),2));
		m_stZTable.pC[i] = m_stZTable_bak.pC[i] * ( 1 + 0.25 * fCGrayHighFactor - fCGrayHighFactor * pow((normali - 0.5),2));
		// 下发参数校验
		if((m_stZTable.pC[i] >= 1.0) || (m_stZTable.pAl[i] > m_stZTable.pC[i]))
		{
			memcpy(m_stZTable.pC, m_stZTable_bak.pC, m_stZTable.len * sizeof(double));
			memcpy(m_stZTable.pAl, m_stZTable_bak.pAl, m_stZTable.len * sizeof(double));
			memcpy(m_stZTable.pFe, m_stZTable_bak.pFe, m_stZTable.len * sizeof(double));
			log_error("AdjustZTable: Adjusted C or Al is invalid");
		}
	}
	// Fe高灰度区间R值调整
	for(int32_t i = nFeHighGraySection[0]; i < nFeHighGraySection[1]; i++)
	{
		if(i > 65535)
		{
			break;
		}
		normali = (i - nFeHighGraySection[0]) / (double)(nFeHighGraySection[1] - nFeHighGraySection[0]);
		m_stZTable.pFe[i] = m_stZTable_bak.pFe[i] * ( 1 + 0.25 * fFeGrayHighFactor - fFeGrayHighFactor * pow((normali - 0.5),2));
		// 下发参数校验
		if((m_stZTable.pFe[i] >= m_stZTable.pAl[i]))
		{
			memcpy(m_stZTable.pC, m_stZTable_bak.pC, m_stZTable.len * sizeof(double));
			memcpy(m_stZTable.pAl, m_stZTable_bak.pAl, m_stZTable.len * sizeof(double));
			memcpy(m_stZTable.pFe, m_stZTable_bak.pFe, m_stZTable.len * sizeof(double));
			log_error("AdjustZTable: Adjusted Fe or Al is invalid");
		}
	}

	// Al,C低灰度区间R值调整
	for(int32_t i = nAlLowGraySection[0]; i < nAlLowGraySection[1]; i++)
	{
		if(i < 0)
		{
			continue;
		}
		normali = (i - nAlLowGraySection[0]) / (double)(nAlLowGraySection[1] - nAlLowGraySection[0]);
		m_stZTable.pAl[i] = m_stZTable_bak.pAl[i] * ( 1 + 0.25 * fAlGrayLowFactor - fAlGrayLowFactor * pow((normali - 0.5),2));
		m_stZTable.pC[i] = m_stZTable_bak.pC[i] * ( 1 + 0.25 * fCGrayLowFactor - fCGrayLowFactor * pow((normali - 0.5),2));
	}
	// Fe低灰度区间R值调整
	for(int32_t i = nFeLowGraySection[0]; i < nFeLowGraySection[1]; i++)
	{
		if(i < 0)
		{
			continue;
		}
		normali = (i - nFeLowGraySection[0]) / (double)(nFeLowGraySection[1] - nFeLowGraySection[0]);
		m_stZTable.pFe[i] = m_stZTable_bak.pFe[i] * ( 1 + 0.25 * fFeGrayLowFactor - fFeGrayLowFactor * pow((normali - 0.5),2));
	}

	// FILE* file = LIBXRAY_NULL;
	// file = fopen("/mnt/nfs/dump/ztable.tbe", "wb+");
	// fwrite(m_stZTable.pC, sizeof(double), m_stZTable.len, file);
	// fwrite(m_stZTable.pAl, sizeof(double), m_stZTable.len, file);	
	// fwrite(m_stZTable.pFe, sizeof(double), m_stZTable.len, file);
	// fclose(file);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::InitColorTable()
{
	/******************************************************
	*               颜色表及分类曲线初始化
	*******************************************************/
	XRAY_LIB_HRESULT hr;
	for (int32_t i = 0; i < XRAYLIB_DUALCOLORTABLE_NUM; i++)
	{
		/* 常规颜色表 */
		hr = ReadColorTable(m_szDualColorPath[i], m_stColorTbeRgb[i]);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Read color table [%d] failed.", i);
	}

	/* 六色颜色表 */
	hr = ReadColorTable(m_szDualZ6ColorPath, m_stColorTbeZ6Rgb);


	hr = ReadSingleZMapTbe(m_szSingleZMapTbePath, m_stSingleZMapTbe);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Read Single Z Map Tbe failed.");

	hr = ReadColorTable(m_szDualColorAiPath, m_stColorTbeAiYuv);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Read ai color table failed.");



	for (int32_t nGrayLevel = 937; nGrayLevel < 1024; nGrayLevel++)
	{
		m_stSingleZMapTbe.CurvePtr()[nGrayLevel] = 2;
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::InitColorTableCA()
{
	/******************************************************
	*               颜色表及分类曲线初始化
	*******************************************************/
	XRAY_LIB_HRESULT hr;
	/* 民航颜色表只有4个 */
	for (int32_t i = 0; i < XRAYLIB_DUALCOLORTABLE_NUM; i++)
	{
		/* 常规颜色表 */
		hr = ReadColorTable(m_szDualColorPath[i], m_stColorTbeRgb[i]);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "Read color table [%d] failed.", i);
	}

	hr = ReadColorTable(m_szDualColorAiPath, m_stColorTbeAiYuv);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Read ai color table failed.");

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::ReadZTableElement(FILE* file, void* buffer, size_t elementSize, size_t elementCount, const char* elementName)
{
    size_t ReadSize = fread(buffer, elementSize, elementCount, file);
    if (ReadSize != elementCount)
    {
        fclose(file);
        log_error("Read ZTable %s failed. Read size: %d.", elementName, (int32_t)ReadSize);
        return XRAY_LIB_READFILESIZE_ERROR;
    }
    return XRAY_LIB_OK;
}

/**************************************************************************
*                               配置文件读取
***************************************************************************/
XRAY_LIB_HRESULT DualModule::ReadZTable(const char* szPresetZTablePath, const char* szAutoCaliZTablePath)
{
	XSP_CHECK(szPresetZTablePath, XRAY_LIB_NULLPTR, "ZTable path is null.");
	XSP_CHECK(szAutoCaliZTablePath, XRAY_LIB_NULLPTR, "AutoZTable path is null.");

	XRAY_LIB_HRESULT hr;
	size_t ReadSize;

	/* 先读取自动标定的文件 */ 
	FILE* fAutoCali = LIBXRAY_NULL;
	fAutoCali = fopen(szAutoCaliZTablePath, "rb");
	if (fAutoCali) // 自动标定文件存在
	{
		fseek(fAutoCali, 0L, SEEK_END);
		ReadSize = ftell(fAutoCali);

		if (ReadSize == static_cast<size_t>((m_stZTable.len * 5 + 4) * sizeof(float64_t)))  // TAV-Zeff支持的标定表
		{
			fseek(fAutoCali, 0L, SEEK_SET);
			hr = ReadZTableElement(fAutoCali, m_stZTable.pC, sizeof(float64_t), m_stZTable.len, "TAV-C");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, m_stZTable.pH2O, sizeof(float64_t), m_stZTable.len, "TAV-H2O");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, m_stZTable.pAl, sizeof(float64_t), m_stZTable.len, "TAV-Al");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, m_stZTable.pFe, sizeof(float64_t), m_stZTable.len, "TAV-Fe");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, m_stZTable.pDiffThr, sizeof(float64_t), m_stZTable.len, "TAV-DiffThr");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, &m_stZTable.minTAV, sizeof(float64_t), 1, "TAV_minTAV");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, &m_stZTable.maxTAV, sizeof(float64_t), 1, "TAV-maxTAV");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, &m_stZTable.minResolvableTAV, sizeof(float64_t), 1, "TAV-minResolvableTAV");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, &m_stZTable.maxResolvableTAV, sizeof(float64_t), 1, "TAV-maxResolvableTAV");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			fclose(fAutoCali);
			log_info("Read AutoCali-TAV-4C ZTable Succ: %s", m_szAutoZTablePath);
			m_bUseTAVZTable = true;

		}
		else if (ReadSize == static_cast<size_t>(m_stZTable.len * 3 * sizeof(float64_t)))  // 旧有三曲线标定表
		{
			fseek(fAutoCali, 0L, SEEK_SET);
			hr = ReadZTableElement(fAutoCali, m_stZTable.pC, sizeof(float64_t), m_stZTable.len, "3C-C");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, m_stZTable.pAl, sizeof(float64_t), m_stZTable.len, "3C-Al");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, m_stZTable.pFe, sizeof(float64_t), m_stZTable.len, "3C-Fe");

			fclose(fAutoCali);
			log_info("Read AutoCali-3C ZTable Succ: %s", m_szAutoZTablePath);
		}
		else if (ReadSize == static_cast<size_t>(m_stZTable.len * 4 * sizeof(float64_t)))  // 旧有四曲线标定表
		{
			fseek(fAutoCali, 0L, SEEK_SET);
			hr = ReadZTableElement(fAutoCali, m_stZTable.pC, sizeof(float64_t), m_stZTable.len, "4C-C");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, m_stZTable.pH2O, sizeof(float64_t), m_stZTable.len, "4C-H2O");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, m_stZTable.pAl, sizeof(float64_t), m_stZTable.len, "4C-Al");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fAutoCali, m_stZTable.pFe, sizeof(float64_t), m_stZTable.len, "4C-Fe");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			fclose(fAutoCali);
			log_info("Read AutoCali-4C ZTable Succ: %s", m_szAutoZTablePath);
		}
		else
		{
			fclose(fAutoCali);
			log_error("Read AutoCali ZTable Err: %s", m_szAutoZTablePath);
			return XRAY_LIB_READFILESIZE_ERROR;
		}
	}
	else // 无自动标定表，从预置表读取, 预置表也分为TAV-4C表和4C表
	{
		FILE* fPresetCali = LIBXRAY_NULL;
		fPresetCali = fopen(szPresetZTablePath, "rb");

		fseek(fPresetCali, 0L, SEEK_END);
		ReadSize = ftell(fPresetCali);

		// 普通4C表
		if(ReadSize == static_cast<size_t>(m_stZTable.len * 4 * sizeof(float64_t)))
		{
			fseek(fPresetCali, 0L, SEEK_SET);
			hr = ReadZTableElement(fPresetCali, m_stZTable.pC, sizeof(float64_t), m_stZTable.len, "Preset-C");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);
	
			hr = ReadZTableElement(fPresetCali, m_stZTable.pH2O, sizeof(float64_t), m_stZTable.len, "Preset-H2O");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);
	
			hr = ReadZTableElement(fPresetCali, m_stZTable.pAl, sizeof(float64_t), m_stZTable.len, "Preset-Al");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);
	
			hr = ReadZTableElement(fPresetCali, m_stZTable.pFe, sizeof(float64_t), m_stZTable.len, "Preset-Fe");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);
			
			fclose(fPresetCali);
			log_info("Read Preset 4C-ZTable Succ: %s", szPresetZTablePath);
		}
		// TAV-Zeff支持的标定表
		else if (ReadSize == static_cast<size_t>((m_stZTable.len * 5 + 4) * sizeof(float64_t))) 
		{
			fseek(fPresetCali, 0L, SEEK_SET);
			hr = ReadZTableElement(fPresetCali, m_stZTable.pC, sizeof(float64_t), m_stZTable.len, "TAV-C");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fPresetCali, m_stZTable.pH2O, sizeof(float64_t), m_stZTable.len, "TAV-H2O");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fPresetCali, m_stZTable.pAl, sizeof(float64_t), m_stZTable.len, "TAV-Al");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fPresetCali, m_stZTable.pFe, sizeof(float64_t), m_stZTable.len, "TAV-Fe");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fPresetCali, m_stZTable.pDiffThr, sizeof(float64_t), m_stZTable.len, "TAV-DiffThr");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fPresetCali, &m_stZTable.minTAV, sizeof(float64_t), 1, "TAV_minTAV");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fPresetCali, &m_stZTable.maxTAV, sizeof(float64_t), 1, "TAV-maxTAV");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fPresetCali, &m_stZTable.minResolvableTAV, sizeof(float64_t), 1, "TAV-minResolvableTAV");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			hr = ReadZTableElement(fPresetCali, &m_stZTable.maxResolvableTAV, sizeof(float64_t), 1, "TAV-maxResolvableTAV");
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			fclose(fPresetCali);
			log_info("Read PreSet-TAV-4C ZTable Succ: %s", szPresetZTablePath);
			m_bUseTAVZTable = true;
		}
		else
		{
			fclose(fPresetCali);
			log_error("Read Preset ZTable Err: %s", szPresetZTablePath);
			return XRAY_LIB_READFILESIZE_ERROR;
		}

	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::ReadZcorrparTable(const char* ZcorrparName, CorrparTable& ZcorrparTbe)
{
	XSP_CHECK(ZcorrparName, XRAY_LIB_NULLPTR, "Zcorrpar path is null.");
	XSP_CHECK(ZcorrparTbe.pZcorrpar,
		XRAY_LIB_NULLPTR, "Zcorrpar data is null.");

	FILE* file = LIBXRAY_NULL;
	size_t ReadSize;
	file = fopen(ZcorrparName, "rb");
	if (file == NULL)
	{
		std::cout <<" Open Zcorrpar failed.path: "<< ZcorrparName << std::endl;
		return XRAY_LIB_OPENFILE_FAIL;
	}
	/*XSP_CHECK(file, XRAY_LIB_OPENFILE_FAIL, "Open Zcorrpar failed. path : %s", ZcorrparName);*/

	/* 文件总大小有效性判断 */
	fseek(file, 0L, SEEK_END);
	if (ftell(file) != static_cast<long>((ZcorrparTbe.len +4)* sizeof(float32_t)))
	{
		fclose(file);
		log_error("ZcorrparTable size err.");
		return XRAY_LIB_READFILESIZE_ERROR;
	}
	fseek(file, 16, SEEK_SET);

	/* 读取Z表内存 */
	ReadSize = fread(ZcorrparTbe.pZcorrpar, sizeof(float32_t), ZcorrparTbe.len, file);
	if (ReadSize != (size_t)ZcorrparTbe.len)
	{
		fclose(file);
		log_error("Read ZcorrparTable failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}
	fclose(file);
	log_info("Load ZcorrparTable Sucessful!");
	m_enZcorrparState = XRAYLIB_ON;


	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::ReadZ6Table(const char* szFileName, Z6Table& stZTbe)
{
	XSP_CHECK(szFileName, XRAY_LIB_NULLPTR, "ZTable path is null.");
	XSP_CHECK(stZTbe.pPE && stZTbe.pPOM && stZTbe.pPEAL &&
		      stZTbe.pFeAl && stZTbe.pAl && stZTbe.pFe,
		      XRAY_LIB_NULLPTR, "ZTable data is null.");

	FILE* file = LIBXRAY_NULL;
	size_t ReadSize;
	file = fopen(szFileName, "rb");
	XSP_CHECK(file, XRAY_LIB_OPENFILE_FAIL, "Open ZTable failed.");

	/* 文件总大小有效性判断 */
	fseek(file, 0L, SEEK_END);
	if (ftell(file) != static_cast<long>(stZTbe.len * 6 * sizeof(double)))
	{
		fclose(file);
		log_error("ZTable size err.");
		return XRAY_LIB_READFILESIZE_ERROR;
	}
	fseek(file, 0L, SEEK_SET);

	/* 读取Z表内存 */
	ReadSize = fread(stZTbe.pPE, sizeof(double), stZTbe.len, file);
	if (ReadSize != (size_t)stZTbe.len)
	{
		fclose(file);
		log_error("Read ZTable PE failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	ReadSize = fread(stZTbe.pPOM, sizeof(double), stZTbe.len, file);
	if (ReadSize != (size_t)stZTbe.len)
	{
		fclose(file);
		log_error("Read ZTable POM failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	ReadSize = fread(stZTbe.pPEAL, sizeof(double), stZTbe.len, file);
	if (ReadSize != (size_t)stZTbe.len)
	{
		fclose(file);
		log_error("Read ZTable PEAL failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	ReadSize = fread(stZTbe.pAl, sizeof(double), stZTbe.len, file);
	if (ReadSize != (size_t)stZTbe.len)
	{
		fclose(file);
		log_error("Read ZTable Al failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	ReadSize = fread(stZTbe.pFeAl, sizeof(double), stZTbe.len, file);
	if (ReadSize != (size_t)stZTbe.len)
	{
		fclose(file);
		log_error("Read ZTable FeAl failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	ReadSize = fread(stZTbe.pFe, sizeof(double), stZTbe.len, file);
	if (ReadSize != (size_t)stZTbe.len)
	{
		fclose(file);
		log_error("Read ZTable Fe failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	fclose(file);
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::ReadColorTable(const char* szFileName,
	                                        ColorTable& stCTbe)
{
	XSP_CHECK(szFileName, XRAY_LIB_NULLPTR, "ColorTable path is null.");
	XSP_CHECK(stCTbe.pColor, XRAY_LIB_NULLPTR, "ColorTable data is null.");

	size_t ReadSize;
	FILE* file = LIBXRAY_NULL;
	file = fopen(szFileName, "rb");
	XSP_CHECK(file, XRAY_LIB_OPENFILE_FAIL, "Open %s failed.", szFileName);

	COLOR_TBE_HEADER stHeader;
	ReadSize = fread(&stHeader, sizeof(COLOR_TBE_HEADER), 1, file);
	if (ReadSize != 1)
	{
		fclose(file);
		log_error("Read ColorTable header failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	if (stHeader.hueNum * stHeader.pixelSize * stHeader.yLevel > 
		stCTbe.hdr.hueNum * stCTbe.hdr.pixelSize * stCTbe.hdr.yLevel)
	{
		fclose(file);
		log_error("ColorTable header unmatched. file: hueNum %d, pixelSize %d, yLevel %d;",
			      stHeader.hueNum, stHeader.pixelSize, stHeader.yLevel);
		log_error("ColorTable header unmatched. max mem: hueNum %d, pixelSize %d, yLevel %d;",
			      stCTbe.hdr.hueNum, stCTbe.hdr.pixelSize, stCTbe.hdr.yLevel);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	stCTbe.hdr = stHeader;

	memset(stCTbe.pColor, 0, stCTbe.Size());
	size_t nTbeSize = stHeader.hueNum * stHeader.pixelSize * stHeader.yLevel;
	ReadSize = fread(stCTbe.pColor, sizeof(uint8_t), nTbeSize, file);
	if (ReadSize != nTbeSize)
	{
		fclose(file);
		log_error("Read colortable failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}
	fclose(file);

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::ReadSingleZMapTbe(const char* szFileName,
	                                        SingleZMapTbe& stZMapTbe)
{
	XSP_CHECK(szFileName, XRAY_LIB_NULLPTR, "Z Map Table path is null.");
	XSP_CHECK(stZMapTbe.pZValue, XRAY_LIB_NULLPTR, "Z Map Table data is null.");

	size_t ReadSize;
	FILE* file = LIBXRAY_NULL;
	file = fopen(szFileName, "rb");
	XSP_CHECK(file, XRAY_LIB_OPENFILE_FAIL, "Open Z Map Table failed.");

	SINGLE_Z_TBE_HEADER stHeader;
	ReadSize = fread(&stHeader, sizeof(SINGLE_Z_TBE_HEADER), 1, file);
	if (ReadSize != 1)
	{
		fclose(file);
		log_error("Read Z Map Table header failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	if (stHeader.zLevel * stHeader.perZSize * stHeader.yLevel > 
		stZMapTbe.hdr.zLevel * stZMapTbe.hdr.perZSize * stZMapTbe.hdr.yLevel)
	{
		fclose(file);
		log_error("Z Map Table header unmatched. file: zLevel %d, perZSize %d, yLevel %d;",
			      stHeader.zLevel, stHeader.perZSize, stHeader.yLevel);
		log_error("Z Map Table header unmatched. max mem: zLevel %d, perZSize %d, yLevel %d;",
			      stZMapTbe.hdr.zLevel, stZMapTbe.hdr.perZSize, stZMapTbe.hdr.yLevel);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	stZMapTbe.hdr = stHeader;

	memset(stZMapTbe.pZValue, 0, stZMapTbe.Size());
	size_t nTbeSize = stHeader.perZSize * stHeader.yLevel;
	ReadSize = fread(stZMapTbe.pZValue, sizeof(uint8_t), nTbeSize, file);
	if (ReadSize != nTbeSize)
	{
		fclose(file);
		log_error("Read Z Map Table failed. Read size: %d, nTbeSize %d.", (int32_t)ReadSize, (int32_t)nTbeSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}
	fclose(file);

	return XRAY_LIB_OK;
}

/**************************************************************************
*                               原子序数计算
***************************************************************************/
XRAY_LIB_HRESULT DualModule::Z8IdentifyByLow(XMat& highData, XMat& lowData,
	                                      XMat& zData, int32_t nPadLen)
{
	/* check para */
	XSP_CHECK(highData.IsValid() && lowData.IsValid() && zData.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == highData.type && XSP_16U == lowData.type &&
		      XSP_8U == zData.type, XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(highData, lowData) && MatSizeEq(highData, zData),
		      XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XSP_CHECK(nPadLen <= highData.tpad && nPadLen <= lowData.tpad && 
		      nPadLen <= zData.tpad, XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	float32_t fAnti = 0.97f; //反溢出因子
	uint16_t usBaseLine = 0; //基线
	double dAir = 65535.0f * fAnti; 

	double dRC, dRAl, dRFe;
	double dTransH, dTransL, dRPos, dAtomicNum;

	uint16_t usHighVal, usLowVal;

	int32_t nStart, nEnd;
	nStart = highData.tpad - nPadLen;
	nEnd = highData.hei - highData.tpad + nPadLen;
	for (int32_t nh = nStart; nh < nEnd; nh++)
	{
		uint16_t* pHigh = highData.Ptr<uint16_t>(nh);
		uint16_t* pLow = lowData.Ptr<uint16_t>(nh);
		uint8_t* pZData = zData.Ptr<uint8_t>(nh);
		for (int32_t nw = 0; nw < highData.wid; nw++)
		{
			if (pLow[nw] <= m_nDualDistinguishGrayDown) // 低于双能分辨下限，置灰
			{
				pZData[nw] = ColorTable::z44_3c_map[ColorTable::_3c_z0_gray].first;
			}
			else if (pLow[nw] >= m_nDualDistinguishGrayUp || pLow[nw] > pHigh[nw]) // 高于双能分辨上限，或低能大于高能，置灰
			{
				pZData[nw] = ColorTable::z44_3c_map[ColorTable::_3c_z0_gray].first;
			}
			else if (m_nDualDistinguishNoneInorganic < pLow[nw] && pLow[nw] < m_nDualDistinguishGrayUp) // 铁和铝的不可分上限，归为有机物
			{
				pZData[nw] = ColorTable::z44_3c_map[ColorTable::_3c_org_org].first;
			}
			else
			{
				usHighVal = pHigh[nw] - usBaseLine;
				usLowVal = pLow[nw] - usBaseLine;

				/* 分类曲线取值 */
				dRC = m_stZTable.CurveC(usLowVal);
				dRAl = m_stZTable.CurveAl(usLowVal);
				dRFe = m_stZTable.CurveFe(usLowVal);

				/* 计算该点R值 */
				dTransH = (double)usHighVal / dAir;
				dTransL = (double)usLowVal / dAir;
				dRPos = log(dTransH) / log(dTransL);

				/* R值到原子序数映射 */
				if (dRPos >= dRC)
				{
					dAtomicNum = 6.5 - (dRPos - dRC) / (1.0 - dRC) * 6.5;
				}
				else if (dRPos > dRAl && dRPos < dRC)
				{
					dAtomicNum = 13.0 - 6.5 * (dRPos - dRAl) / (dRC - dRAl);
				}
				else
				{
					dAtomicNum = 13.0 + 13.0 * (dRPos - dRAl) / (dRFe - dRAl);
				}

				/* 原子序数到颜色表的映射 */
                int32_t nZResult = 0;
				if (dAtomicNum >= 1.0 && dAtomicNum <= 10.0)
				{
					// [1.0, 10.0] -> [2, 11]
					nZResult = (int32_t)(dAtomicNum + m_fMaterialAdjust + 1);
				}
				else if (dAtomicNum > 10.0 && dAtomicNum < 16.0)
				{
					// (10.0, 16.0) -> [12, 22]
					nZResult = (int32_t)((dAtomicNum - 10.0) * 2.0 + m_fMaterialAdjust + 11);
				}
				else if (dAtomicNum >= 16.0)
				{
					// [16.0, ∞) -> [23, 41]
					nZResult = (int32_t)(dAtomicNum + m_fMaterialAdjust + 7);
				}
				pZData[nw] = std::clamp(nZResult, 
                                        ColorTable::z44_3c_map[ColorTable::_3c_org_org].first, 
                                        ColorTable::z44_3c_map[ColorTable::_3c_ino_blue].second);
			}
		} // for nw
	} // for nh

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::Z8IdentifyByTAV(XMat& highData, XMat& lowData, XMat& zData, int32_t nPadLen)
{
    // 参数检查
    XSP_CHECK(highData.IsValid() && lowData.IsValid() && zData.IsValid(),
              XRAY_LIB_XMAT_INVALID, "Invalid XMat.");
    XSP_CHECK(XSP_16U == highData.type && XSP_16U == lowData.type && XSP_8U == zData.type,
              XRAY_LIB_XMAT_TYPE_ERR);
    XSP_CHECK(MatSizeEq(highData, lowData) && MatSizeEq(highData, zData),
              XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");
    XSP_CHECK(nPadLen <= highData.tpad && nPadLen <= lowData.tpad && nPadLen <= zData.tpad,
              XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

    // 常量定义
    constexpr float64_t fAnti = 0.97f;
    constexpr float64_t dAir = 65535.0f * fAnti;
	const int32_t startH = highData.tpad - nPadLen;
	const int32_t endH = highData.hei - highData.bpad + nPadLen;
	const int32_t width = highData.wid;
	const int32_t heigth = highData.hei;
	const float64_t fMaxResolvableTAVAdjust = std::clamp((200.0 - m_nDualDistinguishGrayDown) / 200 * 2.0, -2.0, 2.0); // 复用双能分辨下限值为总衰减值算法上限调整值（0-200-400 -> -2.0-2.0）
	const float64_t fMinResolvableTAVAdjust = std::clamp((62800.0 - m_nDualDistinguishGrayUp) / 2736 * 2.0, -2.0, 2.0); // 复用双能分辨上限值为总衰减值算法下限调整值（60064-62800-65535 -> -2.0-2.0）
	float64_t fMaxResolvableTAV = std::min(m_stZTable.maxResolvableTAV + fMaxResolvableTAVAdjust, m_stZTable.maxTAV); //外部可调参数，用于控制双能分辨上下限的精准度，下同
	float64_t fMinResolvableTAV = std::max(m_stZTable.minResolvableTAV + fMinResolvableTAVAdjust, m_stZTable.minTAV);

	// 获取由高低能计算得到的R值图
	std::vector<float64_t> RvalueBuf(heigth * width, 0);
    for (int32_t nh = startH; nh < endH; nh++)
	{
		for(int32_t nw = 0; nw < width; nw++)
		{
			const uint16_t usHighVal = highData.Ptr<uint16_t>(nh)[nw];
			const uint16_t usLowVal = lowData.Ptr<uint16_t>(nh)[nw];
			float64_t* pTempData = RvalueBuf.data() + nh * width;

			// 对全图进行R值计算
			if (usLowVal > usHighVal || usHighVal == 0 || usLowVal == 0)
			{
				continue;
			}

			pTempData[nw] = log(dAir / usHighVal) / log(dAir / usLowVal);
		}
	}
	
    // 根据R值图进行原子序数平滑和计算处理
    for (int32_t nh = startH; nh < endH; nh++)
    {
        uint16_t* pHigh = highData.Ptr<uint16_t>(nh);
        uint16_t* pLow = lowData.Ptr<uint16_t>(nh);
        uint8_t* pZData = zData.Ptr<uint8_t>(nh);

        for (int32_t nw = 0; nw < width; nw++)
        {
			// 读取R值图
			float64_t dRPos = RvalueBuf[nh * width + nw];

			// R值异常处理
            if (dRPos >= 1.0 || dRPos <= 0.0)
            {
                pZData[nw] = ColorTable::z44_3c_map[ColorTable::_3c_z0_gray].first;
                continue;
            }

            // 计算总衰减值
			const uint16_t usHighVal = pHigh[nw];
			const uint16_t usLowVal = pLow[nw];
            const float64_t dSumAttenuation = log(dAir / usHighVal) + log(dAir / usLowVal);

            // 超出范围处理
            if (dSumAttenuation >= fMaxResolvableTAV || dSumAttenuation <= fMinResolvableTAV)
            {
                pZData[nw] = ColorTable::z44_3c_map[ColorTable::_3c_z0_gray].first;
                continue;
            }

			// R值平滑处理
			const float64_t dRPosThres = m_stZTable.DiffThr(dRPos) * dRPos;
			const int32_t xStart = std::max(0, nw - nPadLen);
			const int32_t xEnd = std::min(width, nw + nPadLen + 1);
			const int32_t yStart = std::max(0, nh - nPadLen);
			const int32_t yEnd = std::min(heigth, nh + nPadLen + 1);

			float64_t dTotalR = 0;
			int32_t nNum = 0;

			for (int32_t y = yStart; y < yEnd; y++)
			{
				for (int32_t x = xStart; x < xEnd; x++)
				{
					if (abs(RvalueBuf[y * width + x] - dRPos) <= dRPosThres)
					{
						dTotalR += RvalueBuf[y * width + x];
						nNum++;
					}
				}
			}

			dRPos = (nNum > 0) ? (dTotalR / nNum) : dRPos;

            // 计算索引和曲线值
            const float64_t index = (dSumAttenuation - m_stZTable.minTAV) / 
                                  (m_stZTable.maxTAV - m_stZTable.minTAV) * 65536.0;
            const float64_t dRC = m_stZTable.CurveC(index);
            const float64_t dRAl = m_stZTable.CurveAl(index);
            const float64_t dRFe = m_stZTable.CurveFe(index);

            // 原子序数计算
            float64_t dAtomicNum;
			if (dRPos >= dRC)
				dAtomicNum = 6.5 - (dRPos - dRC) / (1.0 - dRC) * 6.5;
			else if (dRPos > dRAl && dRPos < dRC)
				dAtomicNum = 13.0 - 6.5 * (dRPos - dRAl) / (dRC - dRAl);
			else
				dAtomicNum = 13.0 + 13.0 * (dRPos - dRAl) / (dRFe - dRAl);

            // 颜色映射
            int32_t nZResult;
            if (dAtomicNum <= 10.0)
                nZResult = static_cast<int32_t>(dAtomicNum + m_fMaterialAdjust + 1);
            else if (dAtomicNum < 22)
                nZResult = static_cast<int32_t>((dAtomicNum - 10.0) * 2.0 + m_fMaterialAdjust + 11);
            else
                nZResult = static_cast<int32_t>(dAtomicNum + m_fMaterialAdjust + 7);

            // 结果赋值
            pZData[nw] = std::clamp(nZResult,
                                   ColorTable::z44_3c_map[ColorTable::_3c_org_org].first,
                                   ColorTable::z44_3c_map[ColorTable::_3c_ino_blue].second);
        }
    }

    return XRAY_LIB_OK;
}

/**************************************************************************
*                              6色原子序数计算
***************************************************************************/
XRAY_LIB_HRESULT DualModule::Z8Identify6S(XMat& highData, XMat& lowData,
	                                        XMat& zData, int32_t nPadLen)
{
	/* check para */
	XSP_CHECK(highData.IsValid() && lowData.IsValid() && zData.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == highData.type && XSP_16U == lowData.type &&
		      XSP_8U == zData.type, XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(highData, lowData) && MatSizeEq(highData, zData),
		      XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XSP_CHECK(nPadLen <= highData.tpad && nPadLen <= lowData.tpad && 
		      nPadLen <= zData.tpad, XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	float32_t fAnti = 0.97f; //反溢出因子
	uint16_t usBaseLine = 0; //基线
	double dAir = 65535.0f * fAnti; 

	double RPE, RPOM, RPEAL, RAl, RFeAl, RFe;
	double dTransH, dTransL, dRPos, dAtomicNum;
	int32_t nZResult, nTemp;

	uint16_t usHighVal, usLowVal;

	int32_t nStart, nEnd;
	nStart = highData.tpad - nPadLen;
	nEnd = highData.hei - highData.tpad + nPadLen;
	for (int32_t nh = nStart; nh < nEnd; nh++)
	{
		uint16_t* pHigh = highData.Ptr<uint16_t>(nh);
		uint16_t* pLow = lowData.Ptr<uint16_t>(nh);
		uint8_t* pZData = zData.Ptr<uint8_t>(nh);
		for (int32_t nw = 0; nw < highData.wid; nw++)
		{
			if (pLow[nw] <= m_nDualDistinguishGrayDown || 
				pLow[nw] >= m_nDualDistinguishGrayUp ||
				pLow[nw] > pHigh[nw])
			{
				/* 双能分辨上下界外置0, 低能大于高能置0 */
				pZData[nw] = 0;
			}
			else if (pLow[nw] < m_nDualDistinguishGrayUp && 
				pLow[nw] > m_nDualDistinguishNoneInorganic)
			{
				/* 铁和铝的不可分上限，此灰度段内全部为有机物，暂全归为z = 2 */
				pZData[nw] = 2; 
			}
			else
			{
				usHighVal = pHigh[nw] - usBaseLine;
				usLowVal = pLow[nw] - usBaseLine;

				/* 分类曲线取值 */
				RPE = m_stZ6Table.CurvePE(usLowVal);
				RPOM = m_stZ6Table.CurvePOM(usLowVal);
				RPEAL = m_stZ6Table.CurvePEAL(usLowVal);
				RAl = m_stZ6Table.CurveAl(usLowVal);
				RFeAl = m_stZ6Table.CurveFeAl(usLowVal);
				RFe = m_stZ6Table.CurveFe(usLowVal);

				/* 计算该点R值 */
				dTransH = (double)usHighVal / dAir;
				dTransL = (double)usLowVal / dAir;
				dRPos = log(dTransH) / log(dTransL);

				/* R值到原子序数映射 */
				if (dRPos >= RPE) {
					dAtomicNum = 6.0 - (dRPos - RPE) / (1.0 - RPE)*6.0;
				}
				else if (dRPos > RPOM && dRPos < RPE) {
					dAtomicNum = 8.0 - 2.0*(dRPos - RPOM) / (RPE - RPOM);
				}
				else if (dRPos > RPEAL && dRPos < RPOM) {
					dAtomicNum = 10.0 - 2.0*(dRPos - RPEAL) / (RPOM - RPEAL);
				}
				else if (dRPos > RAl && dRPos < RPEAL) {
					dAtomicNum = 13.0 - 3.0*(dRPos - RAl) / (RPEAL - RAl);
				}
				else if (dRPos > RFeAl && dRPos < RAl) {
					dAtomicNum = 20.0 - 7.0*(dRPos - RFeAl) / (RAl - RFeAl);
				}
				else {
					dAtomicNum = 20.0 + 6.0*(dRPos - RFeAl) / (RFe - RFeAl);
				}

				/* 原子序数到颜色表映射 */
				if (dAtomicNum < 1)
				{
					nZResult = 2;
				}
				else if (dAtomicNum >= 1 && dAtomicNum < 6)
				{
					nZResult = (int32_t)(dAtomicNum + 1 + m_fMaterialAdjust);
				}
				else if (dAtomicNum >= 6 && dAtomicNum < 8)
				{
					nTemp = (int32_t)((dAtomicNum - 6) / 0.5);
					nZResult = (int32_t)(nTemp + 7 + m_fMaterialAdjust);
				}
				else if (dAtomicNum >= 8 && dAtomicNum < 20)
				{
					nZResult = (int32_t)(dAtomicNum + 3 + m_fMaterialAdjust);
				}
				else if (dAtomicNum >= 20 && dAtomicNum < 36)
				{
					nTemp = (int32_t)((dAtomicNum - 20) / 0.80);
					nZResult = (int32_t)(nTemp + 23 + m_fMaterialAdjust);
				}
				else
				{
					nZResult = 43;
				}

				/* 边界判断 */
				if (nZResult < 0)
				{
					nZResult = 2;
				}
				if (nZResult > 34)
				{
					nZResult = 34;
				}

				pZData[nw] = (uint8_t)nZResult;
			}
		} // for nw
	} // for nh

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::Z8IdentifyCA(XMat& highData, XMat& lowData,
	                                     XMat& zData, int32_t nPadLen)
{
	/* check para */
	XSP_CHECK(highData.IsValid() && lowData.IsValid() && zData.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == highData.type && XSP_16U == lowData.type &&
		      XSP_8U == zData.type, XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(highData, lowData) && MatSizeEq(highData, zData),
		      XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XSP_CHECK(nPadLen <= highData.tpad && nPadLen <= lowData.tpad && 
		      nPadLen <= zData.tpad, XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	float32_t fAnti = 0.97f; //反溢出因子
	uint16_t usBaseLine = 0; //基线
	double dAir = 65535.0f * fAnti; 

	double dRC, dRAl, dRFe;
	double dTransH, dTransL, dRPos, dAtomicNum;
	int32_t nZResult, nTemp;

	uint16_t usHighVal, usLowVal;

	int32_t nStart, nEnd;
	nStart = highData.tpad - nPadLen;
	nEnd = highData.hei - highData.tpad + nPadLen;
	for (int32_t nh = nStart; nh < nEnd; nh++)
	{
		uint16_t* pHigh = highData.Ptr<uint16_t>(nh);
		uint16_t* pLow = lowData.Ptr<uint16_t>(nh);
		uint8_t* pZData = zData.Ptr<uint8_t>(nh);
		for (int32_t nw = 0; nw < highData.wid; nw++)
		{
			if (pLow[nw] <= m_nDualDistinguishGrayDown)
			{
				pZData[nw] = 0;
			}
			else if(pLow[nw] >= m_nDualDistinguishGrayUp || pLow[nw] > pHigh[nw])
			{
				/* 双能分辨上下界外置0, 低能大于高能置0 */
				pZData[nw] = 0;
			}
			else if (pLow[nw] < m_nDualDistinguishGrayUp && 
				pLow[nw] > m_nDualDistinguishNoneInorganic)
			{
				/* 铁和铝的不可分上限，此灰度段内全部为有机物，暂全归为z = 2 */
				pZData[nw] = 6; 
			}
			else
			{
				usHighVal = pHigh[nw] - usBaseLine;
				usLowVal = pLow[nw] - usBaseLine;

				/* 分类曲线取值 */
				dRC = m_stZTable.CurveC(usLowVal);
				dRAl = m_stZTable.CurveAl(usLowVal);
				dRFe = m_stZTable.CurveFe(usLowVal);

				/* 计算该点R值 */
				dTransH = (double)usHighVal / dAir;
				dTransL = (double)usLowVal / dAir;
				dRPos = log(dTransH) / log(dTransL);

				/* R值到原子序数映射 */
				if (dRPos >= dRC)
				{
					dAtomicNum = 18.0 - (dRPos - dRC) / (1.0 - dRC) * 18.0;
				}
				else if (dRPos > dRAl && dRPos < dRC)
				{
					dAtomicNum = 39.0 - 21.0 * (dRPos - dRAl) / (dRC - dRAl);
				}
				else
				{
					dAtomicNum = 39.0 + 39.0 * (dRPos - dRAl) / (dRFe - dRAl);
				}

				/* 原子序数到颜色表映射 */
				if (dAtomicNum < 1)
				{
					nZResult = 2;
				}
				else if (dAtomicNum >= 1 && dAtomicNum <= 30)
				{
					/* 1 ~ 10 -> 2 ~ 11 */
					nZResult = (int32_t)(dAtomicNum + 1 + m_fMaterialAdjust);
				}
				else if (dAtomicNum > 30 && dAtomicNum < 48)
				{
					/* 10 ~ 16 -> 11 ~ 22 */
					nTemp = (int32_t)((dAtomicNum - 30) / 0.5);
					nZResult = (int32_t)(nTemp + 31 + m_fMaterialAdjust);
				}
				else if (dAtomicNum >= 48 && dAtomicNum < 108)
				{
					/* 16 ~ 36 -> 23 ~ 43 */
					nZResult = (int32_t)(dAtomicNum + 19 + m_fMaterialAdjust);
				}
				else
				{
					nZResult = 127;
				}

				/* 边界判断 */
				if (nZResult < 0)
				{
					nZResult = 2;
				}
				if (nZResult >= 125)
				{
					nZResult = 125;
				}

				pZData[nw] = (uint8_t)nZResult;
			}
		} // for nw
	} // for nh

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::Z16IdentifyByLow(XMat& highData, XMat& lowData,
	                                      XMat& zData, int32_t nPadLen)
{
	/* check para */
	XSP_CHECK(highData.IsValid() && lowData.IsValid() && zData.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == highData.type && XSP_16U == lowData.type &&
		      XSP_16U == zData.type, XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(highData, lowData) && MatSizeEq(highData, zData),
		      XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XSP_CHECK(nPadLen <= highData.tpad && nPadLen <= lowData.tpad && 
		      nPadLen <= zData.tpad, XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	float32_t fAnti = 0.97f; //反溢出因子
	uint16_t usBaseLine = 0; //基线
	double dAir = 65535.0f * fAnti; 

	double dRC, dRH2O, dRAl, dRFe;
	double dTransH, dTransL, dRPos, dAtomicNum;
	float32_t fZResult, fTemp;

	/* 平滑参数 */
	int32_t nKernelLength, nShift, nTotalH, nTotalL, nNum;
	int32_t nXStart, nXEnd, nYStart, nYEnd;
	int32_t nDualDistinguishGrayDown = 500, nDualDistinguishGrayUp = 62000;//Ai路双能分辨上下限固定
	float32_t fFilterRange;

	uint16_t usHighVal, usLowVal;

	int32_t nStart, nEnd, nHei, nWid;
	nStart = highData.tpad - nPadLen;
	nEnd = highData.hei - highData.tpad + nPadLen;
	nHei = highData.hei;
	nWid = highData.wid;
	uint16_t* pHighOri = highData.Ptr<uint16_t>();
	uint16_t* pLowOri = lowData.Ptr<uint16_t>();
	for (int32_t nh = nStart; nh < nEnd; nh++)
	{
		uint16_t* pHigh = highData.Ptr<uint16_t>(nh);
		uint16_t* pLow = lowData.Ptr<uint16_t>(nh);
		uint16_t* pZData = zData.Ptr<uint16_t>(nh);
		for (int32_t nw = 0; nw < highData.wid; nw++)
		{
			if (pLow[nw] <= nDualDistinguishGrayDown)
			{
				pZData[nw] = uint16_t(65535 * 34.0f / 43.0f);
			}
			else if ( pLow[nw] >= nDualDistinguishGrayUp ||
				pLow[nw] > pHigh[nw])
			{
				/* 双能分辨上下界外置0, 低能大于高能置0 */
				pZData[nw] = 0;
			}
			else if (pLow[nw] < nDualDistinguishGrayUp && 
				pLow[nw] > m_nDualDistinguishNoneInorganic)
			{
				/* 铁和铝的不可分上限，此灰度段内全部为有机物，暂全归为z = 2 */
				pZData[nw] = uint16_t(65535 * 2.0f / 43.0f); 
			}
			else
			{
				/*********************************************
				*               原子序数平滑
				**********************************************/

				if (pLow[nw] > 50000)
				{
					nKernelLength = 5; //5
					fFilterRange = 0.30f;
				}
				else if (pLow[nw] > 35000)
				{
					nKernelLength = 4;
					fFilterRange = 0.10f;
				}
				else if (pLow[nw] > 15000)
				{
					nKernelLength = 3;
					fFilterRange = 0.07f;
				}
				else if (pLow[nw] > 4000)
				{
					nKernelLength = 4;
					fFilterRange = 0.10f;
				}
				else
				{
					nKernelLength = 5; //5
					fFilterRange = 0.30f;
				}

				nXStart = nw < nKernelLength ? 0 : nw - nKernelLength;
				nXEnd = nw > (nWid - nKernelLength - 1) ? nWid : nw + nKernelLength + 1;
				nYStart = nh < nKernelLength ? 0 : nh - nKernelLength;
				nYEnd = nh > (nHei - nKernelLength - 1) ? nHei : nh + nKernelLength + 1;

				nNum = 0;
				nTotalH = 0;
				nTotalL = 0;

				for (int32_t y = nYStart; y < nYEnd; y++)
				{
					for (int32_t x = nXStart; x < nXEnd; x++)
					{
						nShift = y * nWid + x;
						if (pLowOri[nShift]<nDualDistinguishGrayDown || pLowOri[nShift]>nDualDistinguishGrayUp)
							continue;

						if (abs((float32_t)(pHighOri[nShift] - pHigh[nw])) < (float32_t)pHigh[nw] * fFilterRange &&
							abs((float32_t)(pLowOri[nShift] - pLow[nw])) < (float32_t)pLow[nw] * fFilterRange)
						{
							nTotalH += pHighOri[nShift];
							nTotalL += pLowOri[nShift];
							nNum++;
						}
					}
				}

				if (nNum != 0)
				{
					usHighVal = nTotalH / nNum - usBaseLine;
					usLowVal = nTotalL / nNum - usBaseLine;
				}
				else
				{
					usHighVal = pHigh[nw] - usBaseLine;
					usLowVal = pLow[nw] - usBaseLine;
				}

				/******************************
				*         原子序数计算
				*******************************/
				/* 分类曲线取值 */
				dRC = m_stZTable.CurveC(usLowVal);
				dRH2O = m_stZTable.CurveH2O(usLowVal);
				dRAl = m_stZTable.CurveAl(usLowVal);
				dRFe = m_stZTable.CurveFe(usLowVal);

				/* 计算该点R值 */
				dTransH = (double)usHighVal / dAir;
				dTransL = (double)usLowVal / dAir;
				dRPos = log(dTransH) / log(dTransL);

				/* R值到原子序数映射 */
				if (dRPos >= dRC)
				{
					dAtomicNum = 6.5 - (dRPos - dRC) / (1.0 - dRC) * 6.5;
				}
				else if (dRPos >= dRH2O  && dRPos < dRC)
				{
					dAtomicNum = 7.5 - (dRPos - dRH2O) / (dRC - dRH2O);
				}
				else if (dRPos > dRAl && dRPos < dRH2O)
				{
					dAtomicNum = 13.0 - 5.5 * (dRPos - dRAl) / (dRH2O - dRAl);
				}
				else
				{
					dAtomicNum = 13.0 + 13.0 * (dRPos - dRAl) / (dRFe - dRAl);
				}

				/*AI路原子序数位置修正,by lukaikai, 6550、5030机型*/
				if (m_enZcorrparState == XRAYLIB_ON)
				{					
					dAtomicNum = dAtomicNum + m_stCorrparTable.CurveZcorrpar(nw);
				}
				else
				{
					if (m_enDevicetype == XRAY_DEVICETYPE_6550_SC)
					{
						if (XRAY_DT == m_enDetectorName || XRAY_SUNFY == m_enDetectorName)
						{
							if (nw <= 110)
							{
								dAtomicNum = dAtomicNum + 0.33 + (110. - nw) / 100 * 0.1;
							}

							else if (110 < nw && nw <= 210)
							{
								dAtomicNum = dAtomicNum + 0.13 + (210. - nw) / 100 * 0.2;
							}

							else if (nw > 210 && nw <= 340)
							{
								dAtomicNum = dAtomicNum + (340. - nw) / 100 * 0.1;
							}
						}
						else if (XRAY_RAYIN_DIGITAL_WEIYING == m_enDetectorName)
						{
							if (nw <= 90)
							{
								dAtomicNum = dAtomicNum + 0.4 + (90. - nw) / 90 * 0.25;
							}
							else if (90 < nw && nw <= 230)
							{
								dAtomicNum = dAtomicNum + 0.25 + (230. - nw) / 140 * 0.15;
							}

							else if (nw > 230 && nw <= 320)
							{
								dAtomicNum = dAtomicNum + (320. - nw) / 90 * 0.25;
							}
						}
						else if (XRAY_RAYIN_QIPAN == m_enDetectorName)
						{
							if (nw <= 40)
							{
								dAtomicNum = dAtomicNum + 0.8 + (40. - nw) / 40 * 0.05;
							}
							else if (40 < nw&&nw <= 140)
							{
								dAtomicNum = dAtomicNum + 0.5 + (140. - nw) / 100 * 0.25;
							}
							else if (140 < nw&&nw <= 320)
							{
								dAtomicNum = dAtomicNum + 0.25 + (320. - nw) / 180 * 0.25;
							}
							else if (nw > 320 && nw <= 480)
							{
								dAtomicNum = dAtomicNum + (480. - nw) / 160. * 0.25;
							}
						}
					}
					if (m_enDevicetype == XRAY_DEVICETYPE_6550_SG)
					{
						if (XRAY_RAYIN_ANALOG_NEW == m_enDetectorName)
						{
							if (nw <= 270)
							{
								dAtomicNum = dAtomicNum + 0.35 + (270. - nw) / 270 * 0.2;
							}

							else if (270 < nw && nw <= 480)
							{
								dAtomicNum = dAtomicNum + 0.05 + (480. - nw) / 210 * 0.3;
							}

							else if (nw > 480 && nw <= 540)
							{
								dAtomicNum = dAtomicNum + (540. - nw) / 60 * 0.05;
							}
						}
						else if (XRAY_RAYIN_QIPAN == m_enDetectorName)
						{
							if (nw <= 270)
							{
								dAtomicNum = dAtomicNum + 0.35 + (270. - nw) / 270 * 0.2;
							}

							else if (270 < nw && nw <= 480)
							{
								dAtomicNum = dAtomicNum + 0.1 + (480. - nw) / 210 * 0.25;
							}

							else if (nw > 480 && nw <= 540)
							{
								dAtomicNum = dAtomicNum + (540. - nw) / 60 * 0.1;
							}

						}
					}
					else if (m_enDevicetype == XRAY_DEVICETYPE_5030_SC)
					{
						if (XRAY_SUNFY == m_enDetectorName)
						{
							if (nw <= 50)
							{
								dAtomicNum = dAtomicNum + 0.7 + (50. - nw) / 50 * 0.05;
							}

							else if (50 < nw && nw <= 150)
							{
								dAtomicNum = dAtomicNum + 0.25 + (150. - nw) / 100 * 0.45;
							}

							else if (nw > 150 && nw <= 230)
							{
								dAtomicNum = dAtomicNum + (230. - nw) / 80 * 0.25;
							}
						}
						if (XRAY_RAYIN_ANALOG_NEW == m_enDetectorName)
						{
							if (nw <= 160)
							{
								dAtomicNum = dAtomicNum + 0.35 + (160. - nw) / 160 * 0.2;
							}
							else if (nw > 160 && nw <= 240)
							{
								dAtomicNum = dAtomicNum + (240. - nw) / 80 * 0.35;
							}
						}
					}
					else if (m_enDevicetype == XRAY_DEVICETYPE_5030_SG)
					{
						if (XRAY_RAYIN_QIPAN == m_enDetectorName)
						{
							if (nw <= 160)
							{
								dAtomicNum = dAtomicNum + 0.45 + (160. - nw) / 160 * 0.5;
							}

							else if (160 < nw && nw <= 240)
							{
								dAtomicNum = dAtomicNum + 0.3 + (240. - nw) / 80 * 0.15;
							}

							else if (nw > 240 && nw <= 320)
							{
								dAtomicNum = dAtomicNum + (320. - nw) / 80 * 0.3;
							}
						}
						if (XRAY_RAYIN_ANALOG_NEW == m_enDetectorName)
						{
							if (nw <= 120)
							{
								dAtomicNum = dAtomicNum + 0.25 + (160. - nw) / 120 * 0.2;
							}

							else if (120 < nw && nw <= 240)
							{
								dAtomicNum = dAtomicNum + (240. - nw) / 120 * 0.25;
							}
						}
					}
				}

				/* 原子序数到颜色表映射 */
				if (dAtomicNum < 1)
				{
					fZResult = 2.0;
				}
				else if (dAtomicNum >= 1 && dAtomicNum <= 10)
				{
					/* 1 ~ 10 -> 2 ~ 11 */
					fZResult = (float32_t)(dAtomicNum + 1 + 0);//Ai路m_fMaterialAdjust固定为0
				}
				else if (dAtomicNum > 10 && dAtomicNum < 16)
				{
					/* 10 ~ 16 -> 11 ~ 22 */
					fTemp = (float32_t)((dAtomicNum - 10) / 0.5);
					fZResult = (float32_t)(fTemp + 11 + 0);
				}
				else if (dAtomicNum >= 16 && dAtomicNum < 36)
				{
					/* 16 ~ 36 -> 23 ~ 43 */
					fZResult = (float32_t)(dAtomicNum + 7 + 0);
				}
				else
				{
					fZResult = 43.0f;
				}

				/* 边界判断 */
				if (fZResult < 0.0f)
				{
					fZResult = 2.0f;
				}
				if (fZResult > 34.0f)
				{
					fZResult = 34.0f;
				}

				pZData[nw] = (uint16_t)(65535 * fZResult / 43.0);
			}
		} // for nw
	} // for nh

	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT DualModule::Z16IdentifyByTAV(XMat& highData, XMat& lowData,
	                                      XMat& zData, int32_t nPadLen)
{
	/* check para */
	XSP_CHECK(highData.IsValid() && lowData.IsValid() && zData.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == highData.type && XSP_16U == lowData.type &&
		      XSP_16U == zData.type, XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(highData, lowData) && MatSizeEq(highData, zData),
		      XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XSP_CHECK(nPadLen <= highData.tpad && nPadLen <= lowData.tpad && 
		      nPadLen <= zData.tpad, XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	constexpr float64_t fAnti = 0.97f;
	constexpr float64_t dAir = 65535.0f * fAnti;
	const int32_t nWid = highData.wid;
	const int32_t nHei = highData.hei;

	double dTransH, dTransL, dRPos, dAtomicNum;
	float32_t fZResult, fTemp;

	/* 平滑参数 */
	int32_t nKernelLength, nShift, nTotalH, nTotalL, nNum;
	int32_t nXStart, nXEnd, nYStart, nYEnd;
	int32_t nDualDistinguishGrayDown = 500, nDualDistinguishGrayUp = 62000;//Ai路双能分辨上下限固定
	float32_t fFilterRange;

	uint16_t usHighVal, usLowVal;

	int32_t nStart, nEnd;
	nStart = highData.tpad - nPadLen;
	nEnd = highData.hei - highData.tpad + nPadLen;
	uint16_t* pHighOri = highData.Ptr<uint16_t>();
	uint16_t* pLowOri = lowData.Ptr<uint16_t>();
	for (int32_t nh = nStart; nh < nEnd; nh++)
	{
		uint16_t* pHigh = highData.Ptr<uint16_t>(nh);
		uint16_t* pLow = lowData.Ptr<uint16_t>(nh);
		uint16_t* pZData = zData.Ptr<uint16_t>(nh);
		for (int32_t nw = 0; nw < highData.wid; nw++)
		{
			if (pLow[nw] <= nDualDistinguishGrayDown)
			{
				pZData[nw] = uint16_t(65535 * 34.0f / 43.0f);
			}
			else if ( pLow[nw] >= nDualDistinguishGrayUp ||
				pLow[nw] > pHigh[nw])
			{
				/* 双能分辨上下界外置0, 低能大于高能置0 */
				pZData[nw] = 0;
			}
			else if (pLow[nw] < nDualDistinguishGrayUp && 
				pLow[nw] > m_nDualDistinguishNoneInorganic)
			{
				/* 铁和铝的不可分上限，此灰度段内全部为有机物，暂全归为z = 2 */
				pZData[nw] = uint16_t(65535 * 2.0f / 43.0f); 
			}
			else
			{
				/*********************************************
				*               原子序数平滑
				**********************************************/

				if (pLow[nw] > 50000)
				{
					nKernelLength = 5; //5
					fFilterRange = 0.30f;
				}
				else if (pLow[nw] > 35000)
				{
					nKernelLength = 4;
					fFilterRange = 0.10f;
				}
				else if (pLow[nw] > 15000)
				{
					nKernelLength = 3;
					fFilterRange = 0.07f;
				}
				else if (pLow[nw] > 4000)
				{
					nKernelLength = 4;
					fFilterRange = 0.10f;
				}
				else
				{
					nKernelLength = 5; //5
					fFilterRange = 0.30f;
				}

				nXStart = nw < nKernelLength ? 0 : nw - nKernelLength;
				nXEnd = nw > (nWid - nKernelLength - 1) ? nWid : nw + nKernelLength + 1;
				nYStart = nh < nKernelLength ? 0 : nh - nKernelLength;
				nYEnd = nh > (nHei - nKernelLength - 1) ? nHei : nh + nKernelLength + 1;

				nNum = 0;
				nTotalH = 0;
				nTotalL = 0;

				for (int32_t y = nYStart; y < nYEnd; y++)
				{
					for (int32_t x = nXStart; x < nXEnd; x++)
					{
						nShift = y * nWid + x;
						if (abs((float32_t)(pHighOri[nShift] - pHigh[nw])) < (float32_t)pHigh[nw] * fFilterRange &&
							abs((float32_t)(pLowOri[nShift] - pLow[nw])) < (float32_t)pLow[nw] * fFilterRange)
						{
							nTotalH += pHighOri[nShift];
							nTotalL += pLowOri[nShift];
							nNum++;
						}
					}
				}

				if (nNum != 0)
				{
					usHighVal = nTotalH / nNum;
					usLowVal = nTotalL / nNum;
				}
				else
				{
					usHighVal = pHigh[nw];
					usLowVal = pLow[nw];
				}

				/******************************
				*         原子序数计算
				*******************************/

				// 计算总衰减值
				const float64_t dSumAttenuation = std::clamp(log(dAir/usHighVal) + log(dAir/usLowVal), m_stZTable.minTAV, m_stZTable.maxTAV);

				// 计算索引和曲线值
				const float64_t index = (dSumAttenuation - m_stZTable.minTAV) / (m_stZTable.maxTAV - m_stZTable.minTAV) * 65536.0;
				const float64_t dRC = m_stZTable.CurveC(index);
				const float64_t dRH2O = m_stZTable.CurveH2O(index);
				const float64_t dRAl = m_stZTable.CurveAl(index);
				const float64_t dRFe = m_stZTable.CurveFe(index);

				/* 计算该点R值 */
				dTransH = (double)usHighVal / dAir;
				dTransL = (double)usLowVal / dAir;
				dRPos = log(dTransH) / log(dTransL);

				/* R值到原子序数映射 */
				if (dRPos >= dRC)
				{
					dAtomicNum = 6.5 - (dRPos - dRC) / (1.0 - dRC) * 6.5;
				}
				else if (dRPos >= dRH2O  && dRPos < dRC)
				{
					dAtomicNum = 8.0 - (dRPos - dRH2O) / (dRC - dRH2O);
				}
				else if (dRPos > dRAl && dRPos < dRH2O)
				{
					dAtomicNum = 13.0 - 5.0 * (dRPos - dRAl) / (dRH2O - dRAl);
				}
				else
				{
					dAtomicNum = 13.0 + 13.0 * (dRPos - dRAl) / (dRFe - dRAl);
				}

				/*AI路原子序数位置修正,by lukaikai, 6550、5030机型*/
				if (m_enZcorrparState == XRAYLIB_ON)
				{					
					dAtomicNum = dAtomicNum + m_stCorrparTable.CurveZcorrpar(nw);
				}
				else
				{
					if (m_enDevicetype == XRAY_DEVICETYPE_6550_SC)
					{
						if (XRAY_DT == m_enDetectorName || XRAY_SUNFY == m_enDetectorName)
						{
							if (nw <= 110)
							{
								dAtomicNum = dAtomicNum + 0.33 + (110. - nw) / 100 * 0.1;
							}

							else if (110 < nw && nw <= 210)
							{
								dAtomicNum = dAtomicNum + 0.13 + (210. - nw) / 100 * 0.2;
							}

							else if (nw > 210 && nw <= 340)
							{
								dAtomicNum = dAtomicNum + (340. - nw) / 100 * 0.1;
							}
						}
						else if (XRAY_RAYIN_DIGITAL_WEIYING == m_enDetectorName)
						{
							if (nw <= 90)
							{
								dAtomicNum = dAtomicNum + 0.4 + (90. - nw) / 90 * 0.25;
							}
							else if (90 < nw && nw <= 230)
							{
								dAtomicNum = dAtomicNum + 0.25 + (230. - nw) / 140 * 0.15;
							}

							else if (nw > 230 && nw <= 320)
							{
								dAtomicNum = dAtomicNum + (320. - nw) / 90 * 0.25;
							}
						}
						else if (XRAY_RAYIN_QIPAN == m_enDetectorName)
						{
							if (nw <= 40)
							{
								dAtomicNum = dAtomicNum + 0.8 + (40. - nw) / 40 * 0.05;
							}
							else if (40 < nw&&nw <= 140)
							{
								dAtomicNum = dAtomicNum + 0.5 + (140. - nw) / 100 * 0.25;
							}
							else if (140 < nw&&nw <= 320)
							{
								dAtomicNum = dAtomicNum + 0.25 + (320. - nw) / 180 * 0.25;
							}
							else if (nw > 320 && nw <= 480)
							{
								dAtomicNum = dAtomicNum + (480. - nw) / 160. * 0.25;
							}
						}
					}
					if (m_enDevicetype == XRAY_DEVICETYPE_6550_SG)
					{
						if (XRAY_RAYIN_ANALOG_NEW == m_enDetectorName)
						{
							if (nw <= 270)
							{
								dAtomicNum = dAtomicNum + 0.35 + (270. - nw) / 270 * 0.2;
							}

							else if (270 < nw && nw <= 480)
							{
								dAtomicNum = dAtomicNum + 0.05 + (480. - nw) / 210 * 0.3;
							}

							else if (nw > 480 && nw <= 540)
							{
								dAtomicNum = dAtomicNum + (540. - nw) / 60 * 0.05;
							}
						}
						else if (XRAY_RAYIN_QIPAN == m_enDetectorName)
						{
							if (nw <= 270)
							{
								dAtomicNum = dAtomicNum + 0.35 + (270. - nw) / 270 * 0.2;
							}

							else if (270 < nw && nw <= 480)
							{
								dAtomicNum = dAtomicNum + 0.1 + (480. - nw) / 210 * 0.25;
							}

							else if (nw > 480 && nw <= 540)
							{
								dAtomicNum = dAtomicNum + (540. - nw) / 60 * 0.1;
							}

						}
					}
					else if (m_enDevicetype == XRAY_DEVICETYPE_5030_SC)
					{
						if (XRAY_SUNFY == m_enDetectorName)
						{
							if (nw <= 50)
							{
								dAtomicNum = dAtomicNum + 0.7 + (50. - nw) / 50 * 0.05;
							}

							else if (50 < nw && nw <= 150)
							{
								dAtomicNum = dAtomicNum + 0.25 + (150. - nw) / 100 * 0.45;
							}

							else if (nw > 150 && nw <= 230)
							{
								dAtomicNum = dAtomicNum + (230. - nw) / 80 * 0.25;
							}
						}
						if (XRAY_RAYIN_ANALOG_NEW == m_enDetectorName)
						{
							if (nw <= 160)
							{
								dAtomicNum = dAtomicNum + 0.35 + (160. - nw) / 160 * 0.2;
							}
							else if (nw > 160 && nw <= 240)
							{
								dAtomicNum = dAtomicNum + (240. - nw) / 80 * 0.35;
							}
						}
					}
					else if (m_enDevicetype == XRAY_DEVICETYPE_5030_SG)
					{
						if (XRAY_RAYIN_QIPAN == m_enDetectorName)
						{
							if (nw <= 160)
							{
								dAtomicNum = dAtomicNum + 0.45 + (160. - nw) / 160 * 0.5;
							}

							else if (160 < nw && nw <= 240)
							{
								dAtomicNum = dAtomicNum + 0.3 + (240. - nw) / 80 * 0.15;
							}

							else if (nw > 240 && nw <= 320)
							{
								dAtomicNum = dAtomicNum + (320. - nw) / 80 * 0.3;
							}
						}
						if (XRAY_RAYIN_ANALOG_NEW == m_enDetectorName)
						{
							if (nw <= 120)
							{
								dAtomicNum = dAtomicNum + 0.25 + (160. - nw) / 120 * 0.2;
							}

							else if (120 < nw && nw <= 240)
							{
								dAtomicNum = dAtomicNum + (240. - nw) / 120 * 0.25;
							}
						}
					}
				}

				/* 原子序数到颜色表映射 */
				if (dAtomicNum < 1)
				{
					fZResult = 2.0;
				}
				else if (dAtomicNum >= 1 && dAtomicNum <= 10)
				{
					/* 1 ~ 10 -> 2 ~ 11 */
					fZResult = (float32_t)(dAtomicNum + 1 + 0);//Ai路m_fMaterialAdjust固定为0
				}
				else if (dAtomicNum > 10 && dAtomicNum < 16)
				{
					/* 10 ~ 16 -> 11 ~ 22 */
					fTemp = (float32_t)((dAtomicNum - 10) / 0.5);
					fZResult = (float32_t)(fTemp + 11 + 0);
				}
				else if (dAtomicNum >= 16 && dAtomicNum < 36)
				{
					/* 16 ~ 36 -> 23 ~ 43 */
					fZResult = (float32_t)(dAtomicNum + 7 + 0);
				}
				else
				{
					fZResult = 43.0f;
				}

				/* 边界判断 */
				if (fZResult < 0.0f)
				{
					fZResult = 2.0f;
				}
				if (fZResult > 34.0f)
				{
					fZResult = 34.0f;
				}

				pZData[nw] = (uint16_t)(65535 * fZResult / 43.0);
			}
			
			
		} // for nw
	} // for nh

return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::SingleZMapSimulate(XMat& xData, XMat& zData, uint16_t bAi)
{
	/* check para */
	XSP_CHECK(xData.IsValid() && zData.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16UC1 == xData.type && XSP_8UC1 == zData.type, XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(xData, zData), XRAY_LIB_XMAT_SIZE_ERR);

	int32_t nWid, nHei, nPad;
	nWid = xData.wid;
	nHei = xData.hei;
	nPad = xData.tpad;

	int32_t nGrayLevel, nGrayStep;
	/* 根据颜色表灰阶做scale调整 */
	int32_t nMaxGrayLevel = m_stColorTbe.hdr.yLevel;
	nGrayStep = 65536 / nMaxGrayLevel;

	float32_t fMaterialAdjust = (1 == bAi) ? 0 : m_fMaterialAdjust;
	
	uint16_t* pData = xData.Ptr<uint16_t>();
	uint8_t* pSlmuCurve = m_stSingleZMapTbe.CurvePtr();
	int32_t nhStart = std::max(nPad - CSMO_NEIGHBOR, 0);
	int32_t nhEnd = std::min(nHei - nPad + CSMO_NEIGHBOR, nHei);
	for (int32_t nh = nhStart; nh < nhEnd; nh++)
	{
		uint8_t* pZValue = zData.Ptr<uint8_t>(nh);
		for (int32_t nw = 0; nw < nWid; nw++)
		{

			nGrayLevel = pData[nh * nWid + nw]/ nGrayStep;

			if (nGrayLevel < 0 || nGrayLevel > nMaxGrayLevel)
			{
				nGrayLevel = 0;
			}
				
			pZValue[nw] = Clamp((uint8_t)(pSlmuCurve[nGrayLevel] + fMaterialAdjust), (uint8_t)2, (uint8_t)34);
		}
	}
	return XRAY_LIB_OK;
}


/**
 * @fn      ColorModeSelected
 * @brief   通过重映射Z值，实现各种上色模式：灰彩、有机无机剔除、Z789、冷暖色调 
 * @note    颜色表的首行为灰度的映射专用，末行为Z789的映射专用
 *  
 * 
 * @return  XRAY_LIB_HRESULT 
 */
XRAY_LIB_HRESULT DualModule::ColorModeSelected()
{
	const std::array<std::pair<int32_t, int32_t>, ColorTable::_3c_cls_num> &_3c_map = ColorTable::z44_3c_map;	// 暂不支持民航128原子序数
	const std::array<int32_t, 44> zMapOri = m_stColorTbe.szZMapOriginal;
	std::array<int32_t, 44>& zMapUsed = m_stColorTbe.szZMapUsed;

    const int32_t orgTh = zMapOri.at(_3c_map[ColorTable::_3c_mix_green].first); // 有机物上限，不包含
    const int32_t inoTh = zMapOri.at(_3c_map[ColorTable::_3c_ino_blue].first); // 无机物上限，包含
    const int32_t z7 = zMapOri.at(7), z8 = zMapOri.at(8), z9 = zMapOri.at(9), z10 = zMapOri.at(10);
	const int32_t nMaxZNum = zMapOri.size();
	int32_t *pZMapUsed = zMapUsed.data();
	const int32_t *pZMapOri = zMapOri.data();
	int32_t nIdx = 0;

	switch (m_enColorMode)
	{
		case XRAYLIB_DUALCOLOR:	// 彩色模式
			memcpy(pZMapUsed, pZMapOri, sizeof(int32_t) * nMaxZNum);
			break;
		
		case XRAYLIB_GRAY:	// 黑白模式
			memset(pZMapUsed, 0, sizeof(int32_t) * nMaxZNum);
			break;

		case XRAYLIB_ORGANIC:	// 有机剔除，有机物置灰
			memset(pZMapUsed, 0, orgTh * sizeof(int32_t));
			break;

		case XRAYLIB_INORGANIC_PLUS:	// 非有机剔除，非有机物置灰
			memset(pZMapUsed + orgTh, 0, (nMaxZNum - orgTh) * sizeof(int32_t));
			break;

		case XRAYLIB_INORGANIC:	// 无机剔除，无机物置灰
			memset(pZMapUsed + inoTh, 0, (nMaxZNum - inoTh) * sizeof(int32_t));
			break;

		case XRAYLIB_ORGANIC_PLUS:	// 非无机剔除，非无机物置灰
			memset(pZMapUsed, 0, inoTh * sizeof(int32_t));
			break;

		case XRAYLIB_Z789:
			for (nIdx = z7; nIdx < z10; nIdx++)
			{
				pZMapUsed[nIdx] = _3c_map[ColorTable::_3c_z789_magenta].second;
			}
			break;

		case XRAYLIB_Z7:
			pZMapUsed[z7] = _3c_map[ColorTable::_3c_z789_magenta].second;
			break;
		
		case XRAYLIB_Z8:
			pZMapUsed[z8] = _3c_map[ColorTable::_3c_z789_magenta].second;
			break;
		
		case XRAYLIB_Z9:
			pZMapUsed[z9] = _3c_map[ColorTable::_3c_z789_magenta].second;
			break;
		
		default:
			memcpy(zMapUsed.data(), pZMapUsed, sizeof(int32_t) * nMaxZNum);
			log_error("UnKnown colorMode : %d\n", m_enColorMode);
			return XRAY_LIB_INVALID_PARAM;
	}

	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT DualModule::ColorTableCopy(ColorTable & colorTbeOut, ColorTable & colorTbeIn)
{
	colorTbeOut.hdr = colorTbeIn.hdr;
	memcpy(colorTbeOut.pColor, colorTbeIn.pColor,
		   colorTbeOut.hdr.hueNum * colorTbeOut.hdr.pixelSize * colorTbeOut.hdr.yLevel * sizeof(uint8_t));
	return XRAY_LIB_OK;
}

// 设置原子序数映射关系
XRAY_LIB_HRESULT DualModule::InitColorTableZMap()
{
	const std::array<std::pair<int32_t, int32_t>, ColorTable::_3c_cls_num> &_3c_map = // 区分H128颜色表和H44颜色表
        (128 == m_stColorTbe.hdr.hueNum) ? ColorTable::z128_3c_map : ColorTable::z44_3c_map;
    const int32_t _3c_mix_diff = _3c_map[ColorTable::_3c_mix_green].second - _3c_map[ColorTable::_3c_mix_green].first;
    if (m_nMixtureRangeDownAdj - m_nMixtureRangeUpAdj > _3c_mix_diff)
    {
        if (m_nMixtureRangeDownAdj > 0 && m_nMixtureRangeUpAdj >= 0)
        {
            m_nMixtureRangeDownAdj = _3c_mix_diff + m_nMixtureRangeUpAdj;
        }
        else if (m_nMixtureRangeDownAdj <= 0 && m_nMixtureRangeUpAdj < 0)
        {
            m_nMixtureRangeUpAdj = m_nMixtureRangeDownAdj - _3c_mix_diff;
        }
        else // m_nMixtureRangeDownAdj > 0 && m_nMixtureRangeUpAdj < 0
        {
            int32_t adjSum = m_nMixtureRangeDownAdj - m_nMixtureRangeUpAdj;
            m_nMixtureRangeDownAdj = (_3c_mix_diff * m_nMixtureRangeDownAdj + adjSum / 2) / adjSum;
            m_nMixtureRangeUpAdj = m_nMixtureRangeDownAdj - _3c_mix_diff;
        }
    }
    std::pair<int32_t, int32_t> hRangeLimit(_3c_map[ColorTable::_3c_org_org].first, _3c_map[ColorTable::_3c_ino_blue].second);
    std::pair<int32_t, int32_t> hMixRange = {
        std::clamp(_3c_map[ColorTable::_3c_mix_green].first-m_nMixtureRangeDownAdj, hRangeLimit.first+1, hRangeLimit.second-1),
        std::clamp(_3c_map[ColorTable::_3c_mix_green].second-m_nMixtureRangeUpAdj, hRangeLimit.first+1, hRangeLimit.second-1)
    };

	int32_t* pZMapOri = m_stColorTbe.szZMapOriginal.data();
	int32_t* pZMapUsed = m_stColorTbe.szZMapUsed.data();

    std::iota(m_stColorTbe.szZMapOriginal.begin(), m_stColorTbe.szZMapOriginal.end(), 0);
    if (hMixRange.first != _3c_map[ColorTable::_3c_mix_green].first) // 有机物区间
    {
        double kOrg = (double)((hMixRange.first - 1) - _3c_map[ColorTable::_3c_org_org].first) / 
            (_3c_map[ColorTable::_3c_org_org].second - _3c_map[ColorTable::_3c_org_org].first);
        for (int32_t i = _3c_map[ColorTable::_3c_org_org].first; i <= _3c_map[ColorTable::_3c_org_org].second; i++)
        {
            pZMapOri[i] = static_cast<int32_t>(std::round(kOrg * (i - _3c_map[ColorTable::_3c_org_org].first))) + _3c_map[ColorTable::_3c_org_org].first;
        }
    }
    if (hMixRange.second != _3c_map[ColorTable::_3c_mix_green].second) // 无机物区间
    {
        double kIno = (double)(_3c_map[ColorTable::_3c_ino_blue].second - (hMixRange.second + 1)) / 
            (_3c_map[ColorTable::_3c_ino_blue].second - _3c_map[ColorTable::_3c_ino_blue].first);
        for (int32_t i = _3c_map[ColorTable::_3c_ino_blue].first; i <= _3c_map[ColorTable::_3c_ino_blue].second; i++)
        {
            pZMapOri[i] = static_cast<int32_t>(std::round(kIno * (i - _3c_map[ColorTable::_3c_ino_blue].first))) + (hMixRange.second + 1);
        }
    }
    if (hMixRange.first != _3c_map[ColorTable::_3c_mix_green].first || hMixRange.second != _3c_map[ColorTable::_3c_mix_green].second) // 混合物区间
    {
        double kMix = (double)(hMixRange.second - hMixRange.first) / 
            (_3c_map[ColorTable::_3c_mix_green].second - _3c_map[ColorTable::_3c_mix_green].first);
        for (int32_t i = _3c_map[ColorTable::_3c_mix_green].first; i <= _3c_map[ColorTable::_3c_mix_green].second; i++)
        {
            pZMapOri[i] = static_cast<int32_t>(std::round(kMix * (i - _3c_map[ColorTable::_3c_mix_green].first))) + hMixRange.first;
        }
    }

	memcpy(pZMapUsed, pZMapOri, m_stColorTbe.szZMapOriginal.size() * sizeof(int32_t));

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT DualModule::GrayRValueTune(XMat& merge, XMat& zData,XMat& low)
{
	uint16_t* mergeData = merge.Ptr<uint16_t>();
	uint16_t* lowData = low.Ptr<uint16_t>();
	uint8_t*  zValue = zData.Ptr();

	/* 如果自动指标关闭，直接返回*/
	if ( XRAYLIB_OFF == m_enTestAutoLE)
	{
		return XRAY_LIB_OK;
	}

	/*跳过扩边区域*/
	int32_t nskipStart = merge.wid * merge.tpad;
	int32_t nskipEnd = merge.wid * (merge.hei - merge.tpad);
	/*最多支持三区域调调整*/
	// 融合灰度调整
	for (int32_t i = 0; i < m_stTestbParams.nGrayTuneNum; i++)
	{
		for (int32_t j = nskipStart; j < nskipEnd; j++)
		{
			/*依据低能灰度值和原子序数调整融合图灰度值*/
			if (m_stTestbParams.stGrayValue[i].nParamGrayValueLL <= mergeData[j] &&
				m_stTestbParams.stGrayValue[i].nParamGrayValueUL >= mergeData[j] &&
				m_stTestbParams.stGrayValue[i].nParamRValueLL <= zValue[j] &&
				m_stTestbParams.stGrayValue[i].nParamRValueUL >= zValue[j] )
			{
				mergeData[j] += m_stTestbParams.stGrayValue->nParamOffset;
			}
		}
	}

	// Z值调整
	for (int32_t i = 0; i < m_stTestbParams.nRTuneNum; i++)
	{
		for (int32_t j = nskipStart; j < nskipEnd; j++)
		{
			/*依据低能灰度值和原子序数调整原子序数*/
			if (m_stTestbParams.stRValue[i].nParamGrayValueLL <= lowData[j] &&
				m_stTestbParams.stRValue[i].nParamGrayValueUL >= lowData[j] &&
				m_stTestbParams.stRValue[i].nParamRValueLL <= zValue[j] &&
				m_stTestbParams.stRValue[i].nParamRValueUL >= zValue[j])
			{
				zValue[j] += m_stTestbParams.stRValue->nParamOffset;
			}
		}
	}
	return XRAY_LIB_OK;
}
