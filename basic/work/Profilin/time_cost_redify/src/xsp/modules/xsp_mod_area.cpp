#include "xsp_mod_area.hpp"
#include "core_def.hpp"
#include <vector>
#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <functional>
#include <queue>
#include <numeric>
#include <iostream>
#include <fstream>
#include <chrono> 
#include <string>

#define XSP_AREA_SEG_LENGTH  800 //分段检测数据长度

int32_t roundRegion[8][2] = { { 0,-1 },{ -1,-1 },{ -1,0 },{ -1,1 },{ 0,1 },{ 1,1 },{ 1,0 },{ 1,-1 } };
int32_t roundRegionDilate[4][2] = { { 0,-1 },{ -1,0 },{ 0,1 },{ 1,0 } };
AreaModule::AreaModule()
{
	//低穿识别参数
	m_enLowPeneSenstivity = XRAYLIB_LOWPENESENS_LOW;
	m_nLowPenetrationThreshold = 400;
	m_stLowPene.nGrayDown = 0;
	m_stLowPene.nGrayUp = m_nLowPenetrationThreshold;
	m_stLowPene.nZDown = 0;
	m_stLowPene.nZUp = 0;
	m_stLowPene.nAreaNumberMin = 1200;
	m_stLowPene.nRectWidthMin = 20;
	m_stLowPene.nRectHeightMin = 40;
	//可疑有机物识别参数
	m_enConcernedMSenstivity = XRAYLIB_CONCERNEDMSENS_MEDIUM;
	m_stConcernedM.nGrayDown = 10000;
	m_stConcernedM.nGrayUp = 35000;
	m_stConcernedM.nZDown = 7;
	m_stConcernedM.nZUp = 9;
	m_stConcernedM.nAreaNumberMin = 1200;
	m_stConcernedM.nRectWidthMin = 20;
	m_stConcernedM.nRectHeightMin = 20;
	//毒品识别参数
	m_enDrugSenstivity = XRAYLIB_CONCERNEDMSENS_MEDIUM;
	m_stDrug.nGrayDown = 48000;
	m_stDrug.nGrayUp = 49000;
	m_stDrug.nZDown = 6;
	m_stDrug.nZUp = 7;
	m_stDrug.nAreaNumberMin = 1200;
	m_stDrug.nRectWidthMin = 20;
	m_stDrug.nRectHeightMin = 40;
	//爆炸物识别参数
	m_enExplosiveSenstivity = XRAYLIB_CONCERNEDMSENS_MEDIUM;
	m_stExplosive.nGrayDown = 10000;
	m_stExplosive.nGrayUp = 35000;
	m_stExplosive.nZDown = 6;
	m_stExplosive.nZUp = 8;
	m_stExplosive.nAreaNumberMin = 1200;
	m_stExplosive.nRectWidthMin = 40;
	m_stExplosive.nRectHeightMin = 40;


	memset(m_nRegionXYPos_Concerned, 0, sizeof(int32_t) * 8);
	m_nConcernedRectExpendBorder = 30;
	m_vecRegionlist.reserve(1000);
	m_RectRegonList.reserve(1000);

	for (int32_t i = 1; i < 4; i++)
	{
		m_vecRectRegonSave[i].reserve(1000);
	}
   
	//开关
	m_enLowPenetrationPrompt = XRAYLIB_OFF;
	m_enConcernedMaterialPrompt = XRAYLIB_OFF;
	m_enExplosivePrompt = XRAYLIB_OFF;
	m_enDrugPrompt = XRAYLIB_OFF;

	m_enDetectionMode = XRAYLIB_EXPLOSIVEDETECTION_1;

	// 分段检测相关变量初始化
	m_nLoopHeight = 0;
	m_nDetecFlag = -1000;

}


XRAY_LIB_HRESULT AreaModule::GetMemSize(XRAY_LIB_MEM_TAB &MemTab, XRAY_LIB_ABILITY &ability)
{
	MemTab.size = 0;

	if (XRAYLIB_ENERGY_SCATTER == ability.nEnergyMode)
	{
		return XRAY_LIB_OK;
	}
	else
	{
		int32_t nRTProcessWidth = ability.nDetectorWidth * XRAY_LIB_MAX_RESIZE_FACTOR;
		int32_t nRTProcessHeight = ability.nMaxHeightEntire;
	
		/* 存储状态和每个像素所属编号临时内存申请，按最大允许传入的图像长度申请 */
		m_matRegionSegStatusTemp.SetMem(XRAY_LIB_MAX_IMGENHANCE_LENGTH * nRTProcessWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matRegionSegStatusTemp.data, m_matRegionSegStatusTemp.Size(), MemTab);

		/* 存储分割后每个区域的数量临时内存申请，按最大允许传入的图像长度申请 */
		m_matRegionSegNumTemp.SetMem(XRAY_LIB_MAX_SEGNUM * XSP_ELEM_SIZE(XSP_32SC1));
		XspMalloc((void**)&m_matRegionSegNumTemp.data, m_matRegionSegNumTemp.Size(), MemTab);

		m_matZDataU8.SetMem(XRAY_LIB_MAX_IMGENHANCE_LENGTH * nRTProcessWidth * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_matZDataU8.data, m_matZDataU8.Size(), MemTab);

		m_total_matZDataU8.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_8UC1));
		XspMalloc((void**)&m_total_matZDataU8.data, m_total_matZDataU8.Size(), MemTab);

		/*线衰减内存*/
		PmmaMu.len = 65536;
		XspMalloc((void**)&PmmaMu.pLowMu, PmmaMu.len * sizeof(double), MemTab);
		XspMalloc((void**)&PmmaMu.pHighMu, PmmaMu.len * sizeof(double), MemTab);

		AlMu.len = 65536;
		XspMalloc((void**)&AlMu.pLowMu, AlMu.len * sizeof(double), MemTab);
		XspMalloc((void**)&AlMu.pHighMu, AlMu.len * sizeof(double), MemTab);

		FeMu.len = 65536;
		XspMalloc((void**)&FeMu.pLowMu, FeMu.len * sizeof(double), MemTab);
		XspMalloc((void**)&FeMu.pHighMu, FeMu.len * sizeof(double), MemTab);

		return XRAY_LIB_OK;
	}
}



XRAY_LIB_HRESULT AreaModule::Init(const char* szPublicFileFolderName,SharedPara* pPara)
{
	XRAY_LIB_HRESULT hr;
	m_pSharedPara = pPara;

	if (XRAYLIB_ENERGY_SCATTER != m_pSharedPara->m_enEnergyMode)
	{
		hr = InitMuTable(szPublicFileFolderName);
		XSP_CHECK(XRAY_LIB_OK == hr, hr, "InitMuTable failed.");
	}
	
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT AreaModule::SetSensitivity(XRAYLIB_CONCERNEDM_SENSITIVITY sensitivity)
{
	m_enConcernedMSenstivity = sensitivity;
	m_enDrugSenstivity = sensitivity;
	m_enExplosiveSenstivity = sensitivity;
	if (XRAY_XRAYDV_DTCA == m_pSharedPara->m_enDeviceInfo.xraylib_detectortype ||
		0x002 == (m_pSharedPara->m_enDeviceInfo.xraylib_devicetype & 0x00F))
	{
		if (sensitivity == XRAYLIB_CONCERNEDMSENS_LOW)
		{
			m_nConcernedRectExpendBorder = 15;
			/* 毒品设置 */
			m_stDrug.nGrayDown = 48000;
			m_stDrug.nGrayUp = 50000;
			m_stDrug.nZDown = 6;
			m_stDrug.nZUp = 7;

			m_stDrug.nAreaNumberMin = 400;
			m_stDrug.nRectWidthMin = 10;
			m_stDrug.nRectHeightMin = 30;

			/* 爆炸物设置 */
			m_stExplosive.nGrayDown = 10000;
			m_stExplosive.nGrayUp = 35000;
			m_stExplosive.nZDown = 6;
			m_stExplosive.nZUp = 8;

			m_stExplosive.nAreaNumberMin = 1200;
			m_stExplosive.nRectWidthMin = 40;
			m_stExplosive.nRectHeightMin = 40;

			/* 有机物设置 */
			m_stConcernedM.nGrayDown = 45000;
			m_stConcernedM.nGrayUp = 50000;
			m_stConcernedM.nZDown = 6;
			m_stConcernedM.nZUp = 9;

			m_stConcernedM.nAreaNumberMin = 400;
			m_stConcernedM.nRectWidthMin = 10;
			m_stConcernedM.nRectHeightMin = 30;

			/* 爆炸物厚度检测灵敏度 */
			limitdata.nMinThick = 40;
		}
		else if (sensitivity == XRAYLIB_CONCERNEDMSENS_MEDIUM)
		{
			m_nConcernedRectExpendBorder = 15;
			/* 毒品设置 */
			m_stDrug.nGrayDown = 40000;
			m_stDrug.nGrayUp = 50000;
			m_stDrug.nZDown = 6;
			m_stDrug.nZUp = 7;

			m_stDrug.nAreaNumberMin = 400;
			m_stDrug.nRectWidthMin = 10;
			m_stDrug.nRectHeightMin = 30;

			/* 爆炸物设置 */
			m_stExplosive.nGrayDown = 10000;
			m_stExplosive.nGrayUp = 35000;
			m_stExplosive.nZDown = 6;
			m_stExplosive.nZUp = 8;

			m_stExplosive.nAreaNumberMin = 1200;
			m_stExplosive.nRectWidthMin = 40;
			m_stExplosive.nRectHeightMin = 40;

			/* 有机物设置 */
			m_stConcernedM.nGrayDown = 40000;
			m_stConcernedM.nGrayUp = 50000;
			m_stConcernedM.nZDown = 6;
			m_stConcernedM.nZUp = 9;

			m_stConcernedM.nAreaNumberMin = 400;
			m_stConcernedM.nRectWidthMin = 10;
			m_stConcernedM.nRectHeightMin = 30;

			/* 爆炸物厚度检测灵敏度 */
			limitdata.nMinThick = 30;
		}
		else if (sensitivity == XRAYLIB_CONCERNEDMSENS_HIGH)
		{
			m_nConcernedRectExpendBorder = 15;
			/* 毒品设置 */
			m_stDrug.nGrayDown = 20000;
			m_stDrug.nGrayUp = 55000;
			m_stDrug.nZDown = 6;
			m_stDrug.nZUp = 7;

			m_stDrug.nAreaNumberMin = 300;
			m_stDrug.nRectWidthMin = 10;
			m_stDrug.nRectHeightMin = 20;

			/* 爆炸物设置 */
			m_stExplosive.nGrayDown = 7000;
			m_stExplosive.nGrayUp = 40000;
			m_stExplosive.nZDown = 6;
			m_stExplosive.nZUp = 10;

			m_stExplosive.nAreaNumberMin = 1200;
			m_stExplosive.nRectWidthMin = 40;
			m_stExplosive.nRectHeightMin = 40;

			/* 有机物设置 */
			m_stConcernedM.nGrayDown = 20000;
			m_stConcernedM.nGrayUp = 50000;
			m_stConcernedM.nZDown = 6;
			m_stConcernedM.nZUp = 9;

			m_stConcernedM.nAreaNumberMin = 300;
			m_stConcernedM.nRectWidthMin = 10;
			m_stConcernedM.nRectHeightMin = 20;

			/* 爆炸物厚度检测灵敏度 */
			limitdata.nMinThick = 20;
		}
		else
		{
			m_nConcernedRectExpendBorder = 15;
			/* 毒品设置 */
			m_stDrug.nGrayDown = 50000;
			m_stDrug.nGrayUp = 60000;
			m_stDrug.nZDown = 6;
			m_stDrug.nZUp = 7;

			m_stDrug.nAreaNumberMin = 200;
			m_stDrug.nRectWidthMin = 10;
			m_stDrug.nRectHeightMin = 15;

			/* 爆炸物设置 */
			m_stExplosive.nGrayDown = 50000;
			m_stExplosive.nGrayUp = 60000;
			m_stExplosive.nZDown = 3;
			m_stExplosive.nZUp = 4;

			m_stExplosive.nAreaNumberMin = 200;
			m_stExplosive.nRectWidthMin = 10;
			m_stExplosive.nRectHeightMin = 15;

			/* 有机物设置 */
			m_stConcernedM.nGrayDown = 50000;
			m_stConcernedM.nGrayUp = 60000;
			m_stConcernedM.nZDown = 7;
			m_stConcernedM.nZUp = 9;

			m_stConcernedM.nAreaNumberMin = 200;
			m_stConcernedM.nRectWidthMin = 10;
			m_stConcernedM.nRectHeightMin = 15;

			/* 爆炸物厚度检测灵敏度 */
			limitdata.nMinThick = 30;
		}
	}
	else
	{
		if (sensitivity == XRAYLIB_CONCERNEDMSENS_LOW)
		{
			m_nConcernedRectExpendBorder = 15;
			/* 毒品设置 */
			m_stDrug.nGrayDown = 48000;
			m_stDrug.nGrayUp = 51000;
			m_stDrug.nZDown = 6;
			m_stDrug.nZUp = 7;

			m_stDrug.nAreaNumberMin = 400;
			m_stDrug.nRectWidthMin = 10;
			m_stDrug.nRectHeightMin = 30;

			/* 爆炸物设置 */
			m_stExplosive.nGrayDown = 50000;
			m_stExplosive.nGrayUp = 52000;
			m_stExplosive.nZDown = 3;
			m_stExplosive.nZUp = 4;

			m_stExplosive.nAreaNumberMin = 400;
			m_stExplosive.nRectWidthMin = 10;
			m_stExplosive.nRectHeightMin = 30;

			/* 有机物设置 */
			m_stConcernedM.nGrayDown = 10000;
			m_stConcernedM.nGrayUp = 35000;
			m_stConcernedM.nZDown = 7;
			m_stConcernedM.nZUp = 9;

			m_stConcernedM.nAreaNumberMin = 400;
			m_stConcernedM.nRectWidthMin = 10;
			m_stConcernedM.nRectHeightMin = 30;
		}
		else if (sensitivity == XRAYLIB_CONCERNEDMSENS_MEDIUM)
		{
			m_nConcernedRectExpendBorder = 15;
			/* 毒品设置 */
			m_stDrug.nGrayDown = 28000;
			m_stDrug.nGrayUp = 51000;
			m_stDrug.nZDown = 6;
			m_stDrug.nZUp = 7;

			m_stDrug.nAreaNumberMin = 200;
			m_stDrug.nRectWidthMin = 10;
			m_stDrug.nRectHeightMin = 15;

			/* 爆炸物设置 */
			m_stExplosive.nGrayDown = 50000;
			m_stExplosive.nGrayUp = 52000;
			m_stExplosive.nZDown = 3;
			m_stExplosive.nZUp = 4;

			m_stExplosive.nAreaNumberMin = 300;
			m_stExplosive.nRectWidthMin = 10;
			m_stExplosive.nRectHeightMin = 20;

			/* 有机物设置 */
			m_stConcernedM.nGrayDown = 10000;
			m_stConcernedM.nGrayUp = 35000;
			m_stConcernedM.nZDown = 7;
			m_stConcernedM.nZUp = 9;

			m_stConcernedM.nAreaNumberMin = 300;
			m_stConcernedM.nRectWidthMin = 10;
			m_stConcernedM.nRectHeightMin = 20;
		}
		else if (sensitivity == XRAYLIB_CONCERNEDMSENS_HIGH)
		{
			m_nConcernedRectExpendBorder = 15;
			/* 毒品设置 */
			m_stDrug.nGrayDown = 20000;
			m_stDrug.nGrayUp = 55000;
			m_stDrug.nZDown = 6;
			m_stDrug.nZUp = 7;

			m_stDrug.nAreaNumberMin = 200;
			m_stDrug.nRectWidthMin = 10;
			m_stDrug.nRectHeightMin = 15;

			/* 爆炸物设置 */
			m_stExplosive.nGrayDown = 20000;
			m_stExplosive.nGrayUp = 52000;
			m_stExplosive.nZDown = 3;
			m_stExplosive.nZUp = 7;

			m_stExplosive.nAreaNumberMin = 200;
			m_stExplosive.nRectWidthMin = 10;
			m_stExplosive.nRectHeightMin = 15;

			/* 有机物设置 */
			m_stConcernedM.nGrayDown = 8000;
			m_stConcernedM.nGrayUp = 40000;
			m_stConcernedM.nZDown = 7;
			m_stConcernedM.nZUp = 9;

			m_stConcernedM.nAreaNumberMin = 200;
			m_stConcernedM.nRectWidthMin = 10;
			m_stConcernedM.nRectHeightMin = 15;
		}
		else
		{
			m_nConcernedRectExpendBorder = 15;
			/* 毒品设置 */
			m_stDrug.nGrayDown = 48000;
			m_stDrug.nGrayUp = 51000;
			m_stDrug.nZDown = 6;
			m_stDrug.nZUp = 7;

			m_stDrug.nAreaNumberMin = 200;
			m_stDrug.nRectWidthMin = 10;
			m_stDrug.nRectHeightMin = 15;

			/* 爆炸物设置 */
			m_stExplosive.nGrayDown = 50000;
			m_stExplosive.nGrayUp = 52000;
			m_stExplosive.nZDown = 3;
			m_stExplosive.nZUp = 4;

			m_stExplosive.nAreaNumberMin = 200;
			m_stExplosive.nRectWidthMin = 10;
			m_stExplosive.nRectHeightMin = 15;

			/* 有机物设置 */
			m_stConcernedM.nGrayDown = 8000;
			m_stConcernedM.nGrayUp = 40000;
			m_stConcernedM.nZDown = 7;
			m_stConcernedM.nZUp = 9;

			m_stConcernedM.nAreaNumberMin = 200;
			m_stConcernedM.nRectWidthMin = 10;
			m_stConcernedM.nRectHeightMin = 15;
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT AreaModule::Release()
{
	std::vector<int32_t>().swap(m_vecRegionlist);
	std::vector<XRAYLIB_BOX>().swap(m_RectRegonList);
	
	return XRAY_LIB_OK;
}



XRAY_LIB_HRESULT AreaModule::GetConcernedArea(XRAYLIB_IMAGE&         xraylib_img,
											  XRAYLIB_CONCERED_AREA	*ConcerdMaterialM,
											  XRAYLIB_CONCERED_AREA	*LowPeneArea,
											  XRAYLIB_CONCERED_AREA	*ExplosiveArea,
											  XRAYLIB_CONCERED_AREA	*DrugArea)
{
	XRAY_LIB_HRESULT hr;
	XMat matGrayUse;
	int32_t nHeight = xraylib_img.height;
	int32_t nWidth = xraylib_img.width;

	ConcerdMaterialM->nNum = 0;
	LowPeneArea->nNum = 0;
	ExplosiveArea->nNum = 0;	//最终输出的锚框数量
	DrugArea->nNum = 0;

	/* 内存初始化 */
	hr = m_matRegionSegStatusTemp.Reshape(nHeight, nWidth, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr=m_matRegionSegNumTemp.Reshape(1, XRAY_LIB_MAX_SEGNUM, XSP_32SC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	/* calilhz */
	Calilhz caliData;
	hr = caliData.low.InitNoCheck(nHeight, nWidth, XSP_16UC1, (uint8_t*)xraylib_img.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	hr = caliData.high.InitNoCheck(nHeight, nWidth, XSP_16UC1, (uint8_t*)xraylib_img.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	
	/*16位原子序数转8位*/
	if (XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{
		if (XRAYLIB_IMG_RAW_LHZ16 == xraylib_img.format)
		{
			hr = m_matZDataU8.Reshape(nHeight, nWidth, XSP_8UC1);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			caliData.zData = m_matZDataU8;

			uint16_t* pZDataU16 = (uint16_t*)xraylib_img.pData[2];
			uint8_t* pZDataU8 = caliData.zData.Ptr();
			for (int32_t i = 0; i < nHeight * nWidth; i++)
			{
				pZDataU8[i] = (uint8_t)(43 * float32_t(pZDataU16[i]) / 65535.0 + 0.005);
			}
		}
		else
		{
			hr = caliData.zData.InitNoCheck(xraylib_img.height, xraylib_img.width, XSP_8UC1, (uint8_t*)xraylib_img.pData[2]);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);
		}
	}
	
	/*注：上面得到高低能与八位的原子序数*/
	/* start detect */
	
	if (XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode || XRAYLIB_ENERGY_LOW == m_pSharedPara->m_enEnergyMode) //优先使用低能灰度图作为判断依据
		matGrayUse = caliData.low;
	else if (XRAYLIB_ENERGY_HIGH == m_pSharedPara->m_enEnergyMode)
		matGrayUse = caliData.high;
	else
	{
		log_error("LibXRay: Unknown EnergyMode.");
		return XRAY_LIB_ENERGYMODE_ERROR;
	}
	//////////////////////难穿透(难穿透不需要Z值，放在第一位，防止单能数据不执行)
	if (XRAYLIB_ON == m_enLowPenetrationPrompt)
	{
		XSP_CHECK(matGrayUse.Ptr<uint16_t>() && LowPeneArea, XRAY_LIB_NULLPTR, "LibXRay: Null ptr.");
		m_stLowPene.nGrayUp = m_nLowPenetrationThreshold;//临时解决低穿灵敏度不生效问题
		RegionSeg(matGrayUse, caliData.zData, LowPeneArea, m_stLowPene,
			m_matRegionSegNumTemp, m_matRegionSegStatusTemp, &AreaModule::bFitPoint3);
	}	
	////////////////////// 毒品
	if (XRAYLIB_ON == m_enDrugPrompt && XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{		
		XSP_CHECK(matGrayUse.Ptr<uint16_t>() && caliData.zData.Ptr<uint8_t>() && DrugArea, XRAY_LIB_NULLPTR, "LibXRay: Null ptr.");
		RegionSeg(matGrayUse, caliData.zData, DrugArea, m_stDrug,
			m_matRegionSegNumTemp, m_matRegionSegStatusTemp, &AreaModule::bFitPoint2);
	}
	////////////////////// 爆炸物
	if (XRAYLIB_ON == m_enExplosivePrompt && XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(matGrayUse.Ptr<uint16_t>() && caliData.zData.Ptr<uint8_t>() && ExplosiveArea, XRAY_LIB_NULLPTR, "LibXRay: Null ptr.");

		if (XRAYLIB_EXPLOSIVEDETECTION_1 == m_enDetectionMode) { // 简单场景
			RegionSeg(matGrayUse, caliData.zData, ExplosiveArea, m_stExplosive,
				m_matRegionSegNumTemp, m_matRegionSegStatusTemp, &AreaModule::bFitPoint2);
		}
		else if (XRAYLIB_EXPLOSIVEDETECTION_2 == m_enDetectionMode) 
		{ // 复杂场景
			RegionExp(caliData.low.Ptr<uint16_t>(), caliData.high.Ptr<uint16_t>(), caliData.zData.Ptr<uint8_t>(), ExplosiveArea, nWidth, nHeight);
		}
		else { // 无效参数
			return XRAY_LIB_INVALID_PARAM;
		}
	}
	//////////////////////可疑有机物
	if (XRAYLIB_ON == m_enConcernedMaterialPrompt && XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(matGrayUse.Ptr<uint16_t>() && caliData.zData.Ptr<uint8_t>() && ConcerdMaterialM, XRAY_LIB_NULLPTR, "LibXRay: Null ptr.");

		RegionSeg(matGrayUse, caliData.zData, ConcerdMaterialM, m_stConcernedM,
			m_matRegionSegNumTemp, m_matRegionSegStatusTemp, &AreaModule::bFitPoint2);
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT AreaModule::MapAreaCoordinates(XMat& m_matGeoDetectionPixelOffsetLut,
												XRAYLIB_CONCERED_AREA* pArea)
{
	// 获取LUT的指针
	uint16_t* pOffsetLut = m_matGeoDetectionPixelOffsetLut.Ptr<uint16_t>();

	// 遍历区域中的每个矩形
	for (int32_t i = 0; i < pArea->nNum; ++i)
	{
		XRAYLIB_BOX* pRect = &pArea->CMRect[i];

		// 将坐标映射
		int32_t newLeft = pOffsetLut[pRect->nLeft];
		int32_t newRight = pOffsetLut[pRect->nRight];

		// 更新坐标
		pRect->nLeft = newLeft;
		pRect->nRight = newRight;
	}

	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT AreaModule::GeoConcernedArea(XMat& m_matGeoDetectionPixelOffsetLut,
											  XRAYLIB_CONCERED_AREA *ConcerdMaterialM,
											  XRAYLIB_CONCERED_AREA *LowPeneArea,
											  XRAYLIB_CONCERED_AREA *ExplosiveArea,
											  XRAYLIB_CONCERED_AREA *DrugArea)
{
	// 根据各区域的开关判断是否需要进行坐标映射

	// 处理低穿透区域
	if (XRAYLIB_ON == m_enLowPenetrationPrompt)
	{
		XSP_CHECK(LowPeneArea, XRAY_LIB_NULLPTR, "LibXRay: LowPeneArea ptr null.");
		MapAreaCoordinates(m_matGeoDetectionPixelOffsetLut, LowPeneArea);
	}

	// 处理毒品区域
	if (XRAYLIB_ON == m_enDrugPrompt && XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(DrugArea, XRAY_LIB_NULLPTR, "LibXRay: DrugArea ptr null.");
		MapAreaCoordinates(m_matGeoDetectionPixelOffsetLut, DrugArea);
	}

	// 处理爆炸物区域
	if (XRAYLIB_ON == m_enExplosivePrompt && XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(ExplosiveArea, XRAY_LIB_NULLPTR, "LibXRay: ExplosiveArea ptr null.");
		MapAreaCoordinates(m_matGeoDetectionPixelOffsetLut, ExplosiveArea);
	}

	// 处理可疑有机物区域
	if (XRAYLIB_ON == m_enConcernedMaterialPrompt && XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(ConcerdMaterialM, XRAY_LIB_NULLPTR, "LibXRay: ConcerdMaterialM ptr null.");
		MapAreaCoordinates(m_matGeoDetectionPixelOffsetLut, ConcerdMaterialM);
	}

	return XRAY_LIB_OK;
}

/**@fn      SaveBox
* @brief    存储area中rect到vector容器
* @param1   rect                           [I]     - 原始存储rect
* @param2   vecRect					       [O]     - 临时rect的vector向量
* @return   错误码
* @note
*/
void AreaModule::SaveBox(XRAYLIB_CONCERED_AREA* rect, std::vector<XRAYLIB_BOX>& vecRect)
{
	if (rect->nNum < 0)
	{
		return;
	}
	vecRect.clear();

	for (int32_t i = 0; i < rect->nNum; i++)
	{
		vecRect.push_back(rect->CMRect[i]);
	}
	return;
}

/**@fn      ResetBox
* @brief    存储area中rect到vector容器
* @param1   rect                           [I/O]     - 原始存储rect
* @param2   LowPeneArea					   [I]       - 临时rect的vector向量
* @return   错误码
* @note
*/
void AreaModule::ResetBox(XRAYLIB_CONCERED_AREA* rect, std::vector<XRAYLIB_BOX>& vecRect, int32_t nSkipHei)
{
	for (int32_t i = 0; i < rect->nNum; i++)
	{
		rect->CMRect[i].nBottom += nSkipHei;
		rect->CMRect[i].nTop += nSkipHei;
		vecRect.push_back(rect->CMRect[i]);
	}

	RemoveBox(vecRect);

	int32_t nBoxNum = 0;
	if (vecRect.size() > XRAY_LIB_MAX_CONCEREDNUMBER)
	{
		nBoxNum = XRAY_LIB_MAX_CONCEREDNUMBER;
	}
	else
	{
		nBoxNum = vecRect.size();
	}

	rect->nNum = nBoxNum;

	for (int32_t i = 0; i < nBoxNum; i++)
	{
		rect->CMRect[i] = vecRect[i];
	}
	
	return;
}


XRAY_LIB_HRESULT AreaModule::GetConcernedAreaPiecewise(XRAYLIB_IMAGE&            xraylib_img,
									                   XRAYLIB_CONCERED_AREA	*ConcerdMaterialM,
									                   XRAYLIB_CONCERED_AREA	*LowPeneArea,
									                   XRAYLIB_CONCERED_AREA	*ExplosiveArea,
									                   XRAYLIB_CONCERED_AREA	*DrugArea,
	                                                   int32_t                       nDetecFlag)
{
	if (m_nDetecFlag != nDetecFlag)
	{
		m_nDetecFlag = nDetecFlag;

		m_nLoopHeight = 0;

		m_vecRectRegonSave[0].clear();
		m_vecRectRegonSave[1].clear();
		m_vecRectRegonSave[2].clear();
		m_vecRectRegonSave[3].clear();
	}

	XRAY_LIB_HRESULT hr;
	int32_t nHei = xraylib_img.height - m_nLoopHeight;
	int32_t nWid = xraylib_img.width;
	XSP_CHECK(nHei > 0, XRAY_LIB_INVALID_PARAM);
	int32_t total_nHei = xraylib_img.height;

    size_t nSkip = m_nLoopHeight * nWid;
	int32_t nHeiSkip = m_nLoopHeight;
	m_nLoopHeight = xraylib_img.height;

	/* 内存初始化 */
	hr = m_matRegionSegStatusTemp.Reshape(nHei, nWid, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr = m_matRegionSegNumTemp.Reshape(1, XRAY_LIB_MAX_SEGNUM, XSP_32SC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	/********************************** 
	*           矩阵初始化
	***********************************/
	XMat matGrayUse, matZValue;
	XMat total_matZValue;

	if (XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode) 
	{
		XSP_CHECK(xraylib_img.pData[0] && xraylib_img.pData[2], XRAY_LIB_NULLPTR);

		/* 优先使用低能灰度图作为判断依据 */
		hr = matGrayUse.Init(nHei, nWid, XSP_16UC1, (uint8_t*)xraylib_img.pData[0] + nSkip * XSP_ELEM_SIZE(XSP_16UC1));
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		hr = matZValue.Init(nHei, nWid, XSP_8UC1, (uint8_t*)xraylib_img.pData[2] + nSkip * XSP_ELEM_SIZE(XSP_8UC1));
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		hr = total_matZValue.Init(total_nHei, nWid, XSP_8UC1, (uint8_t*)xraylib_img.pData[2]);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);

		/* Z16下原子序数转换 */
		if (XRAYLIB_IMG_RAW_LHZ16 == xraylib_img.format)
		{
			hr = m_matZDataU8.Reshape(nHei, nWid, XSP_8UC1);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			matZValue = m_matZDataU8;

			uint16_t* pZDataU16 = (uint16_t*)xraylib_img.pData[2] + nSkip;
			uint8_t* pZDataU8 = matZValue.Ptr();
			for (int32_t i = 0; i < nHei * nWid; i++)
			{
				pZDataU8[i] = (uint8_t)(43 * float32_t(pZDataU16[i]) / 65535.0 + 0.005);
			}

			hr = m_total_matZDataU8.Reshape(total_nHei, nWid, XSP_8UC1);
			XSP_CHECK(XRAY_LIB_OK == hr, hr);

			total_matZValue = m_total_matZDataU8;
			uint16_t* total_pZDataU16 = (uint16_t*)xraylib_img.pData[2];
			uint8_t* total_pZDataU8 = total_matZValue.Ptr();
			for (int32_t i = 0; i < total_nHei * nWid; i++)
			{
				total_pZDataU8[i] = (uint8_t)(43 * float32_t(total_pZDataU16[i]) / 65535.0 + 0.005);
			}
		}
	}
	else if (XRAYLIB_ENERGY_LOW == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(xraylib_img.pData[0], XRAY_LIB_NULLPTR);

		hr = matGrayUse.Init(nHei, nWid, XSP_16UC1, (uint8_t*)xraylib_img.pData[0] + nSkip * XSP_ELEM_SIZE(XSP_16UC1));
	}
	else if (XRAYLIB_ENERGY_HIGH == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(xraylib_img.pData[1], XRAY_LIB_NULLPTR);

		hr = matGrayUse.Init(nHei, nWid, XSP_16UC1, (uint8_t*)xraylib_img.pData[1] + nSkip * XSP_ELEM_SIZE(XSP_16UC1));
	}
	else
	{
		log_error("LibXRay: Unknown EnergyMode.");
		return XRAY_LIB_ENERGYMODE_ERROR;
	}

	/**********************************
	*            区域检测
	***********************************/
	if (XRAYLIB_ON == m_enLowPenetrationPrompt)
	{
		XSP_CHECK(LowPeneArea, XRAY_LIB_NULLPTR, "LibXRay: Null ptr.");

		m_stLowPene.nGrayUp = m_nLowPenetrationThreshold;//临时解决低穿灵敏度不生效问题
		RegionSeg(matGrayUse, matZValue, LowPeneArea, m_stLowPene, m_matRegionSegNumTemp, 
			      m_matRegionSegStatusTemp, &AreaModule::bFitPoint3);

		ResetBox(LowPeneArea, m_vecRectRegonSave[0], nHeiSkip);
	}
	////////////////////// 毒品
	if (XRAYLIB_ON == m_enDrugPrompt && XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(DrugArea, XRAY_LIB_NULLPTR, "LibXRay: Null ptr.");

		RegionSeg(matGrayUse, matZValue, DrugArea, m_stDrug, m_matRegionSegNumTemp, 
			      m_matRegionSegStatusTemp, &AreaModule::bFitPoint2);

		ResetBox(DrugArea, m_vecRectRegonSave[1], nHeiSkip);
	}
	////////////////////// 爆炸物
	if (XRAYLIB_ON == m_enExplosivePrompt && XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(ExplosiveArea, XRAY_LIB_NULLPTR, "LibXRay: Null ptr.");

		if (XRAYLIB_EXPLOSIVEDETECTION_1 == m_enDetectionMode)
		{ // 简单场景
			RegionSeg(matGrayUse, matZValue, ExplosiveArea, m_stExplosive,
				m_matRegionSegNumTemp, m_matRegionSegStatusTemp, &AreaModule::bFitPoint2);
			ResetBox(ExplosiveArea, m_vecRectRegonSave[2], nHeiSkip);
		}
		else if (XRAYLIB_EXPLOSIVEDETECTION_2 == m_enDetectionMode)
		{ // 复杂场景
			RegionExp((uint16_t*) xraylib_img.pData[0], (uint16_t*)xraylib_img.pData[1] , total_matZValue.Ptr<uint8_t>(), ExplosiveArea, nWid, total_nHei);
			/*for (int32_t j = 0; j < ExplosiveArea->nNum; j++)
			{
				std::cout << "**********************************************"
						  << m_pSharedPara->m_enVisual << " "
						  << ExplosiveArea->CMRect[j].nTop << " "
						  << ExplosiveArea->CMRect[j].nBottom << " "
						  << ExplosiveArea->CMRect[j].nLeft << " "
						  << ExplosiveArea->CMRect[j].nRight << std::endl;
			}*/
		}
		else 
		{ // 无效参数
			return XRAY_LIB_INVALID_PARAM;
		}
	}
	//////////////////////可疑有机物
	if (XRAYLIB_ON == m_enConcernedMaterialPrompt && XRAYLIB_ENERGY_DUAL == m_pSharedPara->m_enEnergyMode)
	{
		XSP_CHECK(ConcerdMaterialM, XRAY_LIB_NULLPTR, "LibXRay: Null ptr.");

		RegionSeg(matGrayUse, matZValue, ConcerdMaterialM, m_stConcernedM, 
			      m_matRegionSegNumTemp, m_matRegionSegStatusTemp, &AreaModule::bFitPoint2);

		ResetBox(ConcerdMaterialM, m_vecRectRegonSave[3], nHeiSkip);
	}

	return XRAY_LIB_OK;
}

//**********************************************************************************//
//								图像处理函数										//
//**********************************************************************************//

void AreaModule::BoxFilterESD(const float32_t* src,
	float32_t* dst,
	int32_t nWidth,
	int32_t nHeight,
	int32_t r)
{
	std::vector<float32_t> temp(nWidth * nHeight);

	// 水平方向滤波
	for (int32_t y = 0; y < nHeight; ++y)
	{
		float32_t sum = 0;
		int32_t count = 0;

		// 初始化窗口
		for (int32_t x = -r; x <= r; ++x)
		{
			int32_t x_clamped = std::max(0, std::min(nWidth - 1, x));
			sum += src[y * nWidth + x_clamped];
			count++;
		}
		temp[y * nWidth] = sum / count;

		// 滑动窗口
		for (int32_t x = 1; x < nWidth; ++x)
		{
			int32_t prev = std::max(0, x - 1 - r);
			int32_t next = std::min(nWidth - 1, x + r);
			sum += src[y * nWidth + next] - src[y * nWidth + prev];
			temp[y * nWidth + x] = sum / count;
		}
	}

	// 垂直方向滤波
	for (int32_t x = 0; x < nWidth; ++x)
	{
		float32_t sum = 0;
		int32_t count = 0;

		// 初始化窗口
		for (int32_t y = -r; y <= r; ++y)
		{
			int32_t y_clamped = std::max(0, std::min(nHeight - 1, y));
			sum += temp[y_clamped * nWidth + x];
			count++;
		}
		dst[x] = sum / count;
		// 滑动窗口
		for (int32_t y = 1; y < nHeight; ++y) {
			int32_t prev = std::max(0, y - 1 - r);
			int32_t next = std::min(nHeight - 1, y + r);
			sum += temp[next * nWidth + x] - temp[prev * nWidth + x];
			dst[y * nWidth + x] = sum / count;
		}
	}
};
/**************************************************************************
*                              new add
***************************************************************************/
XRAY_LIB_HRESULT AreaModule::InitMuTable(const char*szPublicFileFolderName)
{
	XSP_CHECK(szPublicFileFolderName, XRAY_LIB_NULLPTR, "Public path is null.");
	strcpy(PmmaMuTablePath, szPublicFileFolderName);
	strcat(PmmaMuTablePath, "/MuTable/Mu_PMMA.tbe");
	strcpy(AlMuTablePath, szPublicFileFolderName);
	strcat(AlMuTablePath, "/MuTable/Mu_Al.tbe");
	strcpy(FeMuTablePath, szPublicFileFolderName);
	strcat(FeMuTablePath, "/MuTable/Mu_Fe.tbe");
	log_info("Pmma MuTablePath path : %s", PmmaMuTablePath);
	log_info("Al MuTablePathh path : %s", AlMuTablePath);
	log_info("Fe MuTablePath path : %s", FeMuTablePath);

	XRAY_LIB_HRESULT hr;
	hr = ReadMuTable(PmmaMuTablePath, PmmaMu);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Read Pmma Mutable failed.");
	hr = ReadMuTable(AlMuTablePath, AlMu);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Read Al Mutable failed.");
	hr = ReadMuTable(FeMuTablePath, FeMu);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Read Fe Mutable failed.");
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT AreaModule::ReadMuTable(const char* szFileName, MuTable& stMuTbe)
{
	XSP_CHECK(szFileName, XRAY_LIB_NULLPTR, "MuTable path is null.");
	XSP_CHECK(stMuTbe.pLowMu && stMuTbe.pHighMu,XRAY_LIB_NULLPTR, "MuTable data is null."); 

	FILE* file = LIBXRAY_NULL;
	size_t ReadSize;
	file = fopen(szFileName, "rb");
	XSP_CHECK(file, XRAY_LIB_OPENFILE_FAIL, "Open MuTable failed. path : %s", szFileName);

	/* 文件总大小有效性判断 */
	fseek(file, 0L, SEEK_END);
	if (ftell(file) != static_cast<long int32_t>(stMuTbe.len * 2 * sizeof(double)))
	{
		fclose(file);
		log_error("MuTable size err.");
		return XRAY_LIB_READFILESIZE_ERROR;
	}
	fseek(file, 0L, SEEK_SET);

	/* 读取Z表内存 */
	ReadSize = fread(stMuTbe.pLowMu, sizeof(double), stMuTbe.len, file);
	if (ReadSize != (size_t)stMuTbe.len)
	{
		fclose(file);
		log_error("Read MuTable LowMu failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}

	ReadSize = fread(stMuTbe.pHighMu, sizeof(double), stMuTbe.len, file);
	if (ReadSize != (size_t)stMuTbe.len)
	{
		fclose(file);
		log_error("Read ZTable HighMu failed. Read size: %d.", (int32_t)ReadSize);
		return XRAY_LIB_READFILESIZE_ERROR;
	}
	fclose(file);
	return XRAY_LIB_OK;
}

void AreaModule::GuidedFilter(const float32_t* I,
	float32_t* fImgOut,
	int32_t nWidth,
	int32_t nHeight,
	int32_t nRadius,
	float32_t eps)
{
	// 参数检查
	if (nRadius <= 0 || eps <= 0 || !I || !fImgOut) {
		throw std::invalid_argument("Invalid parameters");
	}

	const int32_t size = nWidth * nHeight;
	std::vector<float32_t> mean_I(size);
	std::vector<float32_t> II(size);
	std::vector<float32_t> mean_II(size);
	std::vector<float32_t> a(size), b(size);
	std::vector<float32_t> mean_a(size), mean_b(size);

	// 第一步：计算均值
	BoxFilterESD(I, mean_I.data(), nWidth, nHeight, nRadius);

	// 计算I*p和I*I
	for (int32_t i = 0; i < size; ++i)
	{
		II[i] = I[i] * I[i];
	}

	// 计算Ip和II的均值
	BoxFilterESD(II.data(), mean_II.data(), nWidth, nHeight, nRadius);

	// 计算系数a和b
	for (int32_t i = 0; i < size; ++i) {
		float32_t var_I = mean_II[i] - mean_I[i] * mean_I[i];
		a[i] = var_I / (var_I + eps);
		b[i] = mean_I[i] - a[i] * mean_I[i];
	}

	// 对系数进行均值滤波
	BoxFilterESD(a.data(), mean_a.data(), nWidth, nHeight, nRadius);
	BoxFilterESD(b.data(), mean_b.data(), nWidth, nHeight, nRadius);

	// 生成最终结果
	for (int32_t i = 0; i < size; ++i) {
		fImgOut[i] = mean_a[i] * I[i] + mean_b[i];
		// 限制到0-65535范围
		fImgOut[i] = std::max(0.0f, std::min(65535.0f, fImgOut[i]));
	}
};

void AreaModule::CalcThick(const float32_t* pfImgL,
	const float32_t* pfImgH,
	float32_t* pfThickPMMA,
	float32_t* pfThickAl,
	float32_t* pfThickFe,
	float32_t* pfThickAl1,
	PARAM param)
{
	int32_t nHeight = param.nHeight;
	int32_t nWidth = param.nWidth;
	double Mul, Muh, fMuHAl, fMuLAl, fMuHPMMA, fMuLPMMA, pdMuHFe, pdMuLFe;
	float32_t dAir = 65536;

	for (int32_t i = 0; i < nWidth * nHeight; ++i)
	{
		if (pfImgL[i] > param.unMaxGray)
		{
			pfThickPMMA[i] = 0;
			pfThickAl[i] = 0;
			pfThickAl1[i] = 0;
			pfThickFe[i] = 0;
		}
		else if (pfImgL[i] < 1)
		{
			pfThickPMMA[i] = 0;
			pfThickAl[i] = 0;
			pfThickAl1[i] = 0;
			pfThickFe[i] = 0;
		}
		else
		{
			Mul = log((float32_t)pfImgL[i] / dAir);
			Muh = log((float32_t)pfImgH[i] / dAir);
			int32_t index = (int32_t)floor(pfImgL[i]);
			fMuHAl = std::abs(AlMu.CurveHighMu(index));
			// fMuHAl = std::abs(AlMu.CurveHighMu((int32_t)floor(pfImgL[i])));
			fMuLAl = std::abs(AlMu.CurveLowMu(index));
			fMuHPMMA = std::abs(PmmaMu.CurveHighMu(index));
			fMuLPMMA = std::abs(PmmaMu.CurveLowMu(index));
			pdMuHFe = std::abs(FeMu.CurveHighMu(index));
			pdMuLFe = std::abs(FeMu.CurveLowMu(index));

			// C-Al 分解
			pfThickPMMA[i] = (fMuHAl * Mul - fMuLAl * Muh) / (fMuLAl * fMuHPMMA - fMuHAl * fMuLPMMA);
			pfThickAl1[i] = (fMuHPMMA * Mul - fMuLPMMA * Muh) / (fMuLAl * fMuHPMMA - fMuHAl * fMuLPMMA);

			// Al-Fe 分解
			float32_t fTemp = pdMuLFe * fMuHAl - pdMuHFe * fMuLAl;
			pfThickAl[i] = (pdMuHFe * Mul - pdMuLFe * Muh) / fTemp;
			pfThickFe[i] = (fMuHAl * Mul - fMuLAl * Muh) / fTemp;
			pfThickAl[i] = std::max((float32_t)0, pfThickAl[i]);
		}
	}
};


void AreaModule::MinFilter(const float32_t* fImgIn,
	float32_t* fImgOut,
	float32_t* fThreshold,
	PARAM param)
{
	int32_t nHeight = param.nHeight;
	int32_t nWidth = param.nWidth;

	for (int32_t y = 0; y < nHeight; ++y)
	{
		// 计算当前行的y边界
		const int32_t y_min = std::max(y - param.nMinFilterSize, 0);
		const int32_t y_max = std::min(y + param.nMinFilterSize, nHeight - 1);

		for (int32_t x = 0; x < nWidth; ++x)
		{
			// 计算当前像素的x边界
			const int32_t x_min = std::max(x - param.nMinFilterSize, 0);
			const int32_t x_max = std::min(x + param.nMinFilterSize, nWidth - 1);

			float32_t min_val = 65535;

			// 在滤波窗口内搜索最小值
			for (int32_t ky = y_min; ky <= y_max; ++ky)
			{
				for (int32_t kx = x_min; kx <= x_max; ++kx)
				{
					const float32_t val = fImgIn[ky * nWidth + kx];
					if (val < min_val)
					{
						min_val = val;
					}
				}
			}
			int32_t index = y * nWidth + x;
			fThreshold[index] = min_val;

			float32_t residual = fImgIn[index] - fImgOut[index];
			if (residual > 50)
			{
				fImgOut[index] = fImgIn[index];
			}
			else if (residual > 30)
			{
				fImgOut[index] = (residual - 30) / 20 * fImgIn[index] + (50 - residual) / 20 * min_val;
			}
			else
			{
				fImgOut[index] = min_val;
			}

			if (param.nView == 1)
			{
				if (fImgOut[index] > 50)
				{
					fImgOut[index] = 100 / (1 + std::exp(-(fImgOut[index] - 50) / 50));
				}
			}
		}
	}
};

void AreaModule::MinFilter(const float32_t* fImgIn, float32_t* fImgOut, PARAM param)
{
	int32_t nHeight = param.nHeight;
	int32_t nWidth = param.nWidth;

	for (int32_t y = 0; y < nHeight; ++y)
	{
		// 计算当前行的y边界
		const int32_t y_min = std::max(y - param.nMinFilterSizeFe, 0);
		const int32_t y_max = std::min(y + param.nMinFilterSizeFe, nHeight - 1);

		for (int32_t x = 0; x < nWidth; ++x)
		{
			// 计算当前像素的x边界
			const int32_t x_min = std::max(x - param.nMinFilterSizeFe, 0);
			const int32_t x_max = std::min(x + param.nMinFilterSizeFe, nWidth - 1);

			float32_t min_val = 65535;

			// 在滤波窗口内搜索最小值
			for (int32_t ky = y_min; ky <= y_max; ++ky)
			{
				for (int32_t kx = x_min; kx <= x_max; ++kx)
				{
					const float32_t val = fImgIn[ky * nWidth + kx];
					if (val < min_val)
					{
						min_val = val;
					}
				}
			}
			int32_t index = y * nWidth + x;
			fImgOut[index] = min_val;
		}
	}
};

void AreaModule::FindSeed(float32_t* fThick,
	std::vector <SEEDPOINT>& SeedPointInfo,
	int32_t seedradius,
	int32_t nWidth,
	int32_t nHeight)
{
	// 遍历图像（排除边界）
	for (int32_t i = seedradius; i < nWidth - seedradius; ++i)
	{
		for (int32_t j = seedradius; j < nHeight - seedradius; ++j)
		{
			float32_t center_value = fThick[j* nWidth + i];
			if (center_value <= 0) continue;

			// 提取邻域并找最大值
			float32_t max_val = center_value;
			float32_t neighborhood_sum = 0.0;
			bool valid = true;
			float32_t min_val = center_value;

			for (int32_t di = -seedradius; di <= seedradius; ++di)
			{
				for (int32_t dj = -seedradius; dj <= seedradius; ++dj)
				{
					float32_t neighbor = fThick[(j + dj)* nWidth + di + i];
					if (neighbor <= 0)
					{
						valid = false;
						break;
					}
					max_val = std::max(max_val, neighbor);
					neighborhood_sum += neighbor;
					min_val = std::min(min_val, neighbor);
				}
				if (!valid) break;
			}

			// 检查条件
			if (valid && (center_value == max_val) && (center_value > 5))
			{
				SEEDPOINT seedpoint;
				seedpoint.nWidth = i;
				seedpoint.nHeight = j;
				SeedPointInfo.push_back(seedpoint);
			}
		}
	}
}

void AreaModule::ConnectedComponents(const int32_t* image_data,
	int32_t width,
	int32_t height,
	int32_t* labels_data,
	std::vector<int32_t>& areas)
{
	if (!image_data || !labels_data || width <= 0 || height <= 0) {
		areas.clear();
		return;
	}

	// 初始化标签矩阵为0
	std::fill_n(labels_data, width * height, 0);

	// 并查集数据结构
	std::vector<int32_t> parent(1, 0); // 索引0占位，标签从1开始
	std::vector<int32_t> rank(1, 0);   // 秩用于优化合并
	int32_t next_label = 1;       // 下一个可用的标签值

							  // 定义并查集操作（Lambda表达式）
	std::function<int32_t(int32_t)> find = [&](int32_t x) -> int32_t {
		if (x >= static_cast<int32_t>(parent.size()) || x == 0) return x;
		if (parent[x] != x) {
			parent[x] = find(parent[x]);
		}
		return parent[x];
	};

	auto unionSets = [&](int32_t a, int32_t b) {
		if (a == b || a == 0 || b == 0) return;
		int32_t rootA = find(a);
		int32_t rootB = find(b);
		if (rootA == rootB) return;

		int32_t max_index = std::max(rootA, rootB);
		if (max_index >= static_cast<int32_t>(parent.size())) {
			parent.resize(max_index + 1);
			rank.resize(max_index + 1, 0);
			for (int32_t i = static_cast<int32_t>(parent.size()) - 1; i <= max_index; i++) {
				if (i >= static_cast<int32_t>(parent.size())) break;
				parent[i] = i;
			}
		}

		if (rank[rootA] < rank[rootB]) {
			parent[rootA] = rootB;
		}
		else if (rank[rootA] > rank[rootB]) {
			parent[rootB] = rootA;
		}
		else {
			parent[rootB] = rootA;
			rank[rootA]++;
		}
	};

	// 第一遍扫描：标记临时标签并构建等价关系
	for (int32_t i = 0; i < height; ++i) {
		for (int32_t j = 0; j < width; ++j) {
			int32_t idx = i * width + j;
			if (image_data[idx] == 0) continue;

			std::vector<int32_t> neighbors;

			if (j > 0) {
				int32_t left_idx = idx - 1;
				if (labels_data[left_idx] != 0) {
					neighbors.push_back(labels_data[left_idx]);
				}
			}

			if (i > 0) {
				int32_t top_idx = (i - 1) * width + j;
				if (labels_data[top_idx] != 0) {
					neighbors.push_back(labels_data[top_idx]);
				}
			}

			if (neighbors.empty()) {
				if (next_label >= static_cast<int32_t>(parent.size())) {
					parent.resize(next_label + 1);
					rank.resize(next_label + 1, 0);
				}
				parent[next_label] = next_label;
				labels_data[idx] = next_label++;
			}
			else {
				int32_t min_label = *min_element(neighbors.begin(), neighbors.end());
				labels_data[idx] = min_label;

				for (int32_t nb : neighbors) {
					if (find(nb) != find(min_label)) {
						unionSets(nb, min_label);
					}
				}
			}
		}
	}

	// 第二遍扫描：统一标签并统计面积
	std::unordered_map<int32_t, int32_t> root_area;

	for (int32_t i = 0; i < height; ++i) {
		for (int32_t j = 0; j < width; ++j) {
			int32_t idx = i * width + j;
			if (labels_data[idx] == 0) continue;

			int32_t root = find(labels_data[idx]);
			root_area[root]++;
		}
	}

	// 第三遍扫描：按面积从小到大重新映射标签
	areas.clear();
	std::vector<std::pair<int32_t, int32_t>> area_pairs;

	for (const auto& p : root_area) {
		area_pairs.push_back(std::make_pair(p.first, p.second));
	}

	// 使用函数对象替代lambda表达式以确保C++11兼容性
	struct {
		bool operator()(const std::pair<int32_t, int32_t>& a, const std::pair<int32_t, int32_t>& b) const {
			if (a.second != b.second)
				return a.second < b.second;
			return a.first < b.first;
		}
	} area_compare;

	sort(area_pairs.begin(), area_pairs.end(), area_compare);

	std::unordered_map<int32_t, int32_t> root_to_label;
	for (size_t i = 0; i < area_pairs.size(); ++i) {
		int32_t root = area_pairs[i].first;
		int32_t area = area_pairs[i].second;
		root_to_label[root] = static_cast<int32_t>(i) + 1;
		areas.push_back(area);
	}

	for (int32_t i = 0; i < height * width; ++i) {
		if (labels_data[i] != 0) {
			labels_data[i] = root_to_label[find(labels_data[i])];
		}
	}
}

//**********************************************************************************//
//								处理流程函数										//
//**********************************************************************************//
void AreaModule::RegionSeg(XMat& ptrImageIn,
							XMat& ptrMaterialIn,
							XRAYLIB_CONCERED_AREA* pConceredM,
							CONCERNEDM stConcerned,
							XMat& nRegionSegNum,
							XMat& nRegionSegStatus,
							Func bFitpoint)
{
	int32_t nImageHeight = ptrImageIn.hei;
	int32_t nImageWidth = ptrImageIn.wid;
	int32_t nMaxImageHeight = m_pSharedPara->m_nMaxHeightEntire;
	if (nImageHeight > nMaxImageHeight)
	{
		nImageHeight = nMaxImageHeight;
	}
	memset(nRegionSegNum.Ptr<int32_t>(), 0, sizeof(int32_t) * XRAY_LIB_MAX_SEGNUM);
	memset(nRegionSegStatus.Ptr<uint16_t>(), 0, sizeof(uint16_t) * nImageWidth * nMaxImageHeight);

	//////对每个符合条件的点，给予区域编号（最大编号值XRAY_LIB_MAX_SEGNUM)
	//////联通的区域给予同一个编号
	//////状态内存，0代表未做处理；1，代表已处理但不符合条件；>1代表符合且其区域编号
	//////因为需要查找周围1的邻域，因此查找范围从 1 - w-1;计算输出坐标时需要默认扩边。

	int32_t nh, nw, nIndex, nTempIndex, nLoop, nColNum, nRowNum;
	bool bFit = false; //变量初始化
	int32_t nSegNum = 1; ///从1为起点

	// 重新计算8邻域索引,原始索引按探测器原始尺寸初始化，该函数的输入图像width可能会resize
	for (nIndex = 0; nIndex < 4; nIndex++)
	{
		m_nRegionXYPos_Concerned[nIndex] = roundRegionDilate[nIndex][0] * nImageWidth + roundRegionDilate[nIndex][1];
	}
	
	uint8_t* pZDataIn = LIBXRAY_NULL;
	if (ptrMaterialIn.IsValid())//单能模式下无原子序数
	{
		pZDataIn = ptrMaterialIn.Ptr<uint8_t>();
	}
	uint16_t* pImageIn = ptrImageIn.Ptr<uint16_t>();
	uint16_t* pRegionSegStatus = nRegionSegStatus.Ptr<uint16_t>();
	int32_t* pRegionSegNum = nRegionSegNum.Ptr<int32_t>();
	
	for (nh = 1; nh < nImageHeight - 1; nh+=2)
	{

		
		for (nw = 1; nw < nImageWidth - 1; nw+=2)
		{
			nIndex = nh*nImageWidth + nw;

			/////已做过处理
			if (pRegionSegStatus[nIndex] >= 1)
			{
				continue;
			}
			///判断是否符合可疑有机物
			if (ptrMaterialIn.IsValid())
			{
				bFit = (this->*bFitpoint)(pZDataIn[nIndex], pImageIn[nIndex], stConcerned.nGrayDown, 
					stConcerned.nGrayUp, stConcerned.nZDown, stConcerned.nZUp);
			}
			else
			{
				bFit = (this->*bFitpoint)(1, pImageIn[nIndex], stConcerned.nGrayDown, stConcerned.nGrayUp,
					stConcerned.nZDown, stConcerned.nZUp);
			}
			///如果为非符合点，标记为已处理，然后返回
			if (bFit == false)
			{
				pRegionSegStatus[nIndex] = 1;
				continue;
			}

			///如果是符合点，以此点为种子点，找出所有连接区域，并给予编号
			nSegNum++;
			if (nSegNum >= XRAY_LIB_MAX_SEGNUM)
			{
				break;
			}
			pRegionSegStatus[nIndex] = (uint16_t)nSegNum;
			pRegionSegNum[nSegNum]++; //记录该连通域面积

			m_vecRegionlist.clear();
			m_vecRegionlist.push_back(nIndex);
			while (!m_vecRegionlist.empty())
			{
				////取出第一个Index并删除第一个元素
				m_it = m_vecRegionlist.begin();
				nIndex = (*m_it);
				m_vecRegionlist.erase(m_it);

				////计算该点周围8个邻域是否符合条件，为真则加入到链表尾部。
				////在加入链表尾部之前，需要确认，该点不在图像边缘，否则当取出该点进行邻域判断时会出错！
				for (nLoop = 0; nLoop < 4; nLoop++)
				{
					nTempIndex = nIndex + m_nRegionXYPos_Concerned[nLoop];

					/////已做过处理
					if (pRegionSegStatus[nTempIndex] >= 1)
						continue;

					///判断是否符合条件
					if (ptrMaterialIn.IsValid())
					{
						bFit = (this->*bFitpoint)(pZDataIn[nTempIndex], pImageIn[nTempIndex], stConcerned.nGrayDown, 
							stConcerned.nGrayUp, stConcerned.nZDown, stConcerned.nZUp);
					}
					else
					{
						bFit = (this->*bFitpoint)(1, pImageIn[nTempIndex], stConcerned.nGrayDown, 
							stConcerned.nGrayUp, stConcerned.nZDown, stConcerned.nZUp);
					}
					
					///如果为非符合点，标记为已处理，然后返回
					if (bFit == false)
					{
						pRegionSegStatus[nTempIndex] = 1;
						continue;
					}

					///是符合点，状态内存更新，
					pRegionSegStatus[nTempIndex] = (uint16_t)nSegNum;
					pRegionSegNum[nSegNum]++;
					///根据是否在边缘，判断是否要加入到list中
					nColNum = nTempIndex % nImageWidth;
					nRowNum = nTempIndex / nImageWidth;

					if (nColNum != 0 && nColNum != nImageWidth - 1 && nRowNum != 0 && nRowNum != nImageHeight - 1)
					{
						m_vecRegionlist.push_back(nTempIndex);
					}
				}
			}
		}
	}
	m_vecRegionlist.clear();

	/////////////符合条件的区域统计（一定数量），给出坐标点；
	XRAYLIB_BOX rectTemp;
	m_RectRegonList.clear();



	for (nLoop = 0; nLoop <= nSegNum; nLoop++)
	{
		if (pRegionSegNum[nLoop] > stConcerned.nAreaNumberMin)
		{
			///////求矩形框的四边
			rectTemp.nLeft = nImageWidth;  rectTemp.nRight = 0;
			rectTemp.nBottom = nImageHeight; rectTemp.nTop = 0;

			for (nh = 0; nh < nImageHeight; nh+=2)
			{
				for (nw = 0; nw < nImageWidth; nw+=2)
				{
					nIndex = nh*nImageWidth + nw;

					if (nLoop == pRegionSegStatus[nIndex])
					{
						rectTemp.nLeft = std::min(rectTemp.nLeft, nw);
						rectTemp.nRight = std::max(rectTemp.nRight, nw);
						rectTemp.nBottom = std::min(rectTemp.nBottom, nh);
						rectTemp.nTop = std::max(rectTemp.nTop,nh);					
					}
				}
			}
			/////////////////获得的矩形框加入到listTemp中
			m_RectRegonList.push_back(rectTemp);
		}
	}
	/////////////矩形框合并,输出矩形框
	pConceredM->nNum = 0;
	// 去除重复候选框

	RemoveBox(m_RectRegonList);
	for (m_rectit = m_RectRegonList.begin(); m_rectit != m_RectRegonList.end(); ++m_rectit) 
	{
		rectTemp = *m_rectit;
		if (pConceredM->nNum >= XRAY_LIB_MAX_CONCEREDNUMBER || pConceredM->nNum < 0)
		{
			break;
		}
		if (rectTemp.nRight - rectTemp.nLeft >= stConcerned.nRectWidthMin &&
			rectTemp.nTop - rectTemp.nBottom >= stConcerned.nRectHeightMin)
		{
			if (rectTemp.nLeft == 1)
			{
				rectTemp.nLeft = 0;
			}
			if (nImageWidth - 1 == rectTemp.nRight)
			{
				rectTemp.nRight = nImageWidth;
			}
			pConceredM->CMRect[pConceredM->nNum].nLeft = rectTemp.nLeft;
			pConceredM->CMRect[pConceredM->nNum].nRight = rectTemp.nRight;
			pConceredM->CMRect[pConceredM->nNum].nBottom = rectTemp.nBottom;
			pConceredM->CMRect[pConceredM->nNum].nTop = rectTemp.nTop;
			pConceredM->nNum++;
		}
	}
}

bool AreaModule::bFitPoint2(unsigned char Material,
							unsigned short GrayValue,
							int32_t nConcernedGrayDown,
							int32_t nConcernedGrayUp,
							int32_t nConcernedZDown,
							int32_t nConcernedZUp)
{
	//如果灰度值不符合，直接返回false
	if (GrayValue >= nConcernedGrayUp || GrayValue <= nConcernedGrayDown)
	{
		return false;
	}
	if (Material >= nConcernedZDown && Material <= nConcernedZUp)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool AreaModule::bFitPoint3(unsigned char Material,
							unsigned short GrayValue,
							int32_t nConcernedGrayDown,
							int32_t nConcernedGrayUp,
							int32_t nConcernedZDown,
							int32_t nConcernedZUp)
{
	if (Material >= nConcernedZDown && Material <= nConcernedZUp)
	{
		
	}

	if (GrayValue >= nConcernedGrayDown && GrayValue <= nConcernedGrayUp) 
	{
		return true;
	}
	else
	{
		return false;
	}
}


void AreaModule::RemoveBox(std::vector<XRAYLIB_BOX> &vecRect)
{
	if (0 == vecRect.size()) 
	{
		return;
	}

	XRAYLIB_BOX recttemp, rectLoop, rectMerge;
	std::vector<XRAYLIB_BOX>::iterator rect_iter_begin;
	bool bMerge = false;
	int32_t begin_idx = 0;
	int32_t len = (int32_t)vecRect.size();
	while (begin_idx < len) 
	{
		if (bMerge) 
		{
			len = (int32_t)vecRect.size();
			begin_idx = 0; //新矩形加入，重新判断
			bMerge = false;
		}
		recttemp = vecRect[begin_idx];
		begin_idx++;

		for (int32_t idx = begin_idx; idx < len; ++idx) 
		{
			rectLoop = vecRect[idx];
			////判断是否在一定范围内
			if (recttemp.nRight > rectLoop.nLeft - m_nConcernedRectExpendBorder &&
				rectLoop.nRight + m_nConcernedRectExpendBorder > recttemp.nLeft &&
				recttemp.nTop > rectLoop.nBottom - m_nConcernedRectExpendBorder &&
				rectLoop.nTop + m_nConcernedRectExpendBorder > recttemp.nBottom)
			{
				bMerge = true;
				vecRect.erase(std::begin(vecRect) + idx);
				vecRect.erase(std::begin(vecRect) + begin_idx - 1);
				//合并为新矩形加入到list中
				rectMerge.nLeft = std::min(recttemp.nLeft, rectLoop.nLeft);
				rectMerge.nRight = std::max(recttemp.nRight, rectLoop.nRight);
				rectMerge.nBottom = std::min(recttemp.nBottom, rectLoop.nBottom);
				rectMerge.nTop = std::max(recttemp.nTop, rectLoop.nTop);
				vecRect.push_back(rectMerge);
				break;
			}
		}
	}
}

void AreaModule::Process_Rescale(unsigned short* psImgL,	/// 输入低能图像 (0-65535)
	unsigned short* psImgH, /// 输入高能图像 (0-65535)
	unsigned char* pcImgZ,	/// 输入原子序数图 (0-43)
	float32_t* pfImgL,			/// 输出低能图像 (0-65535)
	float32_t* pfImgH,			/// 输出高能图像 (0-65535)
	float32_t* pfImgZ,			/// 输出原子序数图
	PARAM& param)			/// 图像参数
{
	int32_t nWidth = param.nWidth;
	int32_t nHeight = param.nHeight;

	nWidth = int32_t((nWidth + param.nScale - 1) / param.nScale);
	nHeight = int32_t((nHeight + param.nScale - 1) / param.nScale);
	if (param.nView == 1)
	{
		for (int32_t nh = 0; nh < nHeight; nh++)
		{
			for (int32_t nw = 0; nw < nWidth; nw++)
			{
				int32_t nIndexNew = nh * nWidth + nw;
				int32_t nIndexRaw = nh * param.nScale * nWidth + nw * param.nScale;
				pfImgL[nIndexNew] = psImgL[nIndexRaw];
				pfImgH[nIndexNew] = psImgH[nIndexRaw];
				pfImgZ[nIndexNew] = pcImgZ[nIndexRaw];
				if (psImgL[nIndexRaw] == 0)
				{
					pfImgL[nIndexNew] = 65535;
					pfImgH[nIndexNew] = 65535;
					pfImgZ[nIndexNew] = 0;
				}
			}
		}
		param.nWidth = nWidth;
		param.nHeight = nHeight;
		param.nSize = param.nWidth * param.nHeight;
	}
	else if (param.nView == 2)
	{
		for (int32_t nh = 0; nh < nHeight; nh++)
		{
			for (int32_t nw = nWidth - 1; nw >= 0; nw--)
			{
				int32_t nIndexNew = nh * nWidth + nw;
				int32_t nIndexRaw = nh * param.nScale * nWidth + (nw - 100) * param.nScale;
				{
					if (nw >= 100)
					{
						pfImgL[nIndexNew] = psImgL[nIndexRaw];
						pfImgH[nIndexNew] = psImgH[nIndexRaw];
						pfImgZ[nIndexNew] = pcImgZ[nIndexRaw];
					}
					else
					{
						pfImgL[nIndexNew] = 65535;
						pfImgH[nIndexNew] = 65535;
						pfImgZ[nIndexNew] = 0;
					}
				}
			}
		}
		param.nWidth = nWidth;
		param.nHeight = nHeight;
		param.nSize = param.nWidth * param.nHeight;
	}
}

void AreaModule::Process_Filter(float32_t* pfImgL,			/// 输入低能图像
	float32_t* pfImgH,			/// 输入高能图像
	float32_t* pfImgLGuided,	/// 输出低能引导滤波图像
	float32_t* pfImgHGuided,	/// 输出高能引导滤波图像
	PARAM param)			/// 图像参数
{
	GuidedFilter(pfImgL, pfImgLGuided, param.nWidth, param.nHeight, param.nNeighborhood, param.fDegreeOfSmoothGray);
	GuidedFilter(pfImgH, pfImgHGuided, param.nWidth, param.nHeight, param.nNeighborhood, param.fDegreeOfSmoothGray);
}

void AreaModule::Process_Stripe(float32_t* pfImgLGuided,	/// 输入低能引导滤波图像
	float32_t* pfImgHGuided,	/// 输入高能引导滤波图像
	float32_t* fThickAl,		/// 输入Al-Fe分解 Al系数图
	PARAM param)			/// 图像参数
{
	int32_t nHeight = param.nHeight;
	int32_t nWidth = param.nWidth;
	if (param.nView == 1)
	{

	}
	else if (param.nView == 2)
	{
		std::vector <float32_t> fMean(nHeight, 0);
		for (int32_t nh = 0; nh < nHeight; nh++)
		{
			for (int32_t nw = 101; nw < 105; nw++)
			{
				fMean[nh] = fMean[nh] + fThickAl[nh * nWidth + nw];
			}
		}
		// 左端点
		auto first_it = std::find_if(
			fMean.begin(),
			fMean.end(),
			[](int32_t x) { return x > 500; }
		);
		int32_t first_index = static_cast<int32_t>(std::distance(fMean.begin(), first_it) - 1);
		// 右端点
		auto last_it = std::find_if(
			fMean.rbegin(),
			fMean.rend(),
			[](int32_t x) { return x > 500; }
		);
		int32_t last_index = nHeight - static_cast<int32_t>(std::distance(fMean.rbegin(), last_it) - 1);
		for (int32_t nh = 0; nh < nHeight; nh++)
		{
			if ((nh < first_index) || (nh > last_index))
			{
				for (int32_t nw = 0; nw < nWidth; nw++)
				{
					int32_t nIndex = nh * nWidth + nw;
					fThickAl[nIndex] = std::min(fThickAl[nh * nWidth + nw], 2.0f);
					pfImgHGuided[nIndex] = 65535;
					pfImgLGuided[nIndex] = 65535;
				}
			}
			else
			{
				for (int32_t nw = 100; nw < 107; nw++)
				{
					int32_t nIndex = nh * nWidth + nw;
					fThickAl[nIndex] = std::min(fThickAl[nh * nWidth + nw], 2.0f);
					pfImgHGuided[nIndex] = 65535;
					pfImgLGuided[nIndex] = 65535;
				}
			}
		}
	}
}

void AreaModule::Process_FindSeed(float32_t* fThickAlGuidedMinf,				/// 输入最小值滤波后的Al厚度值
	float32_t* fThickAlCorr,					/// 经过sigmoid函数后的Al厚度值
	float32_t* fThickFe,						/// C-Fe分解 Fe系数值
	float32_t* fThickInputInner,				/// 小范围均值滤波结果
	float32_t* fThickInputOuter,				///	大范围均值滤波结果
	float32_t* fThickAlDiff,					/// Al厚度差
	std::vector<SEEDPOINT>& SeedPointInfo,	/// 种子点位置
	PARAM param)							/// 图像参数
{
	int32_t nHeight = param.nHeight;
	int32_t nWidth = param.nWidth;
	BoxFilterESD(fThickAlGuidedMinf, fThickInputInner, nWidth, nHeight, param.nBoxInnerSize);
	BoxFilterESD(fThickAlGuidedMinf, fThickInputOuter, nWidth, nHeight, param.nBoxOuterSize);
	for (int32_t i = 0; i < nHeight * nWidth; i++)
	{
		fThickAlDiff[i] = fThickInputOuter[i] - fThickInputInner[i];
		if (param.nView == 1)
		{
			if (fThickAlGuidedMinf[i] < 10)
			{
				fThickAlCorr[i] = fThickAlGuidedMinf[i] / (1 + exp(2 * (fThickAlDiff[i] + 3)));
			}
			else
			{
				fThickAlCorr[i] = fThickAlGuidedMinf[i];
			}
		}
		if (param.nView == 2)
		{
			// fThickAlCorr[i] = fThickAlGuidedMinf[i];
			if (fThickAlGuidedMinf[i] < 20)
			{
				fThickAlCorr[i] = fThickAlGuidedMinf[i] / (1 + exp(1 * (fThickAlDiff[i] + 6)));
			}
			else
			{
				fThickAlCorr[i] = fThickAlGuidedMinf[i];
			}
		}
	}
	int32_t count = 0;
	for (int32_t nh = param.nFindSeedRadius; nh < nHeight - param.nFindSeedRadius; nh++)
	{
		for (int32_t nw = param.nFindSeedRadius; nw < nWidth - param.nFindSeedRadius; nw++)
		{
			int32_t nIndex = nh * nWidth + nw;
			// 对厚度做sigmoid，减少无关区域
			if (fThickAlDiff[nIndex] < -param.fThickAlDiffThres2)
			{
				SEEDPOINT seedpoint;
				seedpoint.nWidth = nw;
				seedpoint.nHeight = nh;
				seedpoint.fThickAlDiff = -fThickAlDiff[nIndex];
				SeedPointInfo.push_back(seedpoint);
				count++;
			}
			else if (fThickAlDiff[nIndex] < -param.fThickAlDiffThres)
			{
				float32_t minVal = fThickAlDiff[nIndex];
				for (int32_t i = -param.nFindSeedRadius; i < param.nFindSeedRadius; i++)
				{
					for (int32_t j = -param.nFindSeedRadius; j < param.nFindSeedRadius; j++)
					{
						minVal = std::min(minVal, fThickAlDiff[(nh + j) * nWidth + (nw + i)]);
					}
				}
				if ((fThickAlDiff[nIndex] == minVal) & (fThickFe[nIndex] > -5))
				{
					SEEDPOINT seedpoint;
					seedpoint.nWidth = nw;
					seedpoint.nHeight = nh;
					seedpoint.fThickAlDiff = -fThickAlDiff[nIndex];
					SeedPointInfo.push_back(seedpoint);
					count++;
				}
			}
		}
	}

	std::sort(SeedPointInfo.begin(), SeedPointInfo.end(), [](const SEEDPOINT& a, const SEEDPOINT& b) {
		return a.fThickAlDiff > b.fThickAlDiff;
	});
}

void AreaModule::Score(int32_t* nSegIndex,
	const float32_t* pfThick,
	const float32_t* pfThickAlDiff,
	float32_t* fThickPMMA,
	float32_t* fThickAl,
	float32_t* fThickFe,
	float32_t* fAirConv,
	int32_t* nSegSingle,
	REGIONINFO& attr,
	std::vector<int32_t>& connect,
	int32_t nHeightMin,
	int32_t nHeightMax,
	int32_t nWidthMin,
	int32_t nWidthMax,
	int32_t nArea,
	int32_t nSegNumber,
	float32_t fThickSum,
	PARAM param)
{
	int32_t nAreaInner = 0;
	int32_t nAreaOuter = 0;
	float32_t fThickInner = 0.0f;
	float32_t fThickOuter = 0.0f;
	float32_t fThickAlDiff = 0.0f;
	float32_t fThickAlDiff1 = 0.0f;
	float32_t fWeight = 0.0f;
	float32_t fScore = 0.0f;
	float32_t fThickAlDiffMin = 100.0f;
	float32_t fRatioAir = 0.0f;
	float32_t fRatioPMMA = 0.0f;
	float32_t fRatioAl = 0.0f;
	float32_t fRatioFe = 0.0f;

	int32_t nHeight = param.nHeight;
	int32_t nWidth = param.nWidth;
	if (nArea == 0)
	{
		REGIONINFO attr;
		attr.nNumber = nSegNumber;
		attr.nArea = 0;
		attr.nWidthMin = 0;
		attr.nWidthMax = 0;
		attr.nHeightMin = 0;
		attr.nHeightMax = 0;
		attr.fThick = 0;
	}

	int32_t nExpand = param.nExpandSize;
	for (int32_t nh = std::max(nHeightMin - nExpand, 0); nh < std::min(nHeightMax + nExpand, nHeight); nh++)
	{
		for (int32_t nw = std::max(nWidthMin - nExpand, 0); nw < std::min(nWidthMax + nExpand, nWidth); nw++)
		{
			int32_t nIndex = nh * nWidth + nw;
			if (nSegSingle[nIndex] == 1)
			{
				fThickInner = fThickInner + pfThick[nIndex];
				fThickAlDiffMin = std::min(fThickAlDiffMin, pfThick[nIndex]);
				if (fThickAlDiffMin > 99.9f) { fThickAlDiffMin = 0.0f; };
				fThickAlDiff1 = fThickAlDiff1 + pfThickAlDiff[nIndex];
				fRatioAir = fRatioAir + fAirConv[nIndex];
				fRatioPMMA = fRatioPMMA + fThickPMMA[nIndex];
				fRatioAl = fRatioAl + fThickAl[nIndex];
				fRatioFe = fRatioFe + fThickFe[nIndex];
				nAreaInner++;

				for (int32_t i = -5; i < 6; i++)
				{
					for (int32_t j = -5; j < 6; j++)
					{
						if (abs(i) + abs(j) < 6)
						{
							if ((i + nh > 0) & (i + nh < nHeight) & (j + nw > 0) & (j + nw < nWidth))
							{
								int32_t nIndexTemp = (nh + i) * nWidth + (nw + j);
								int32_t nOut = nSegIndex[nIndexTemp];
								if ((nOut > 3) & (nOut != nSegNumber))
								{
									if (std::find(connect.begin(), connect.end(), nOut) == connect.end())
									{
										connect.push_back(nOut);
									}
								}
							}
						}
					}
				}
			}
			else
			{
				fThickOuter = fThickOuter + pfThick[nIndex];
				nAreaOuter++;
			}
		}
	}
	nAreaInner = std::max(nAreaInner, 1);
	nAreaOuter = std::max(nAreaOuter, 1);
	fThickAlDiff1 = fThickAlDiff1 / (float32_t)nAreaInner;
	fThickAlDiff = fThickInner / (float32_t)nAreaInner - fThickOuter / (float32_t)nAreaOuter;
	fThickAlDiffMin = fThickAlDiffMin - fThickOuter / (float32_t)nAreaOuter;
	fRatioAir = fRatioAir / (float32_t)nAreaInner;
	fRatioPMMA = fRatioPMMA / (float32_t)nAreaInner;
	fRatioAl = fRatioAl / (float32_t)nAreaInner;
	fRatioFe = fRatioFe / (float32_t)nAreaInner;
	if (param.nView == 1)
	{
		fWeight = fThickAlDiffMin * nAreaInner;
	}
	else
	{
		fWeight = fThickAlDiff * nAreaInner;
	}

	if (std::isnan(fRatioPMMA)) { fRatioPMMA = 0; }
	if (std::isnan(fRatioAl)) { fRatioAl = 0; }
	if (std::isnan(fRatioFe)) { fRatioFe = 0; }
	fScore = 1 / (1 + std::exp(-0.2 * (fWeight / 1000 - param.fScoreWeight)));
	fScore = fScore * 1 / (1 + std::exp(-0.25 * (fThickAlDiff - param.fScoreDiff)));

	// 10. 检查区域面积是否满足限制条件

	float32_t fThick = fThickSum / nArea;

	attr.nNumber = nSegNumber;
	attr.nArea = nArea;
	attr.nWidthMin = nWidthMin;
	attr.nWidthMax = nWidthMax;
	attr.nHeightMin = nHeightMin;
	attr.nHeightMax = nHeightMax;
	attr.fThick = fThick;
	attr.fThickAlDiff = fThickAlDiff;
	attr.fThickAlDiffMin = fThickAlDiffMin;
	attr.fWeight = fWeight;
	attr.fRatioAir = fRatioAir;
	attr.fRatioPMMA = fRatioPMMA;
	attr.fRatioAl = fRatioAl;
	attr.fRatioFe = fRatioFe;
	attr.fScore = fScore;
	attr.fThickAlDiff1 = fThickAlDiff1;
}

void AreaModule::RegionSegment(int32_t* nSegIndex,
	const float32_t* pfThickCorr,
	const float32_t* pfThick,
	const float32_t* pfThickAlDiff,
	float32_t* fThickPMMA,
	float32_t* fThickAl,
	float32_t* fThickFe,
	float32_t* fAirConv,
	int32_t* nVisited,
	int32_t* nSegSingle,
	int32_t* nSegConv,
	std::vector<SEEDPOINT> SeedPointInfo,
	std::vector<REGIONINFO>& RegionInfo,
	std::vector<std::vector<int32_t>>& vConnectTotal,
	PARAM param)
{
	int32_t nConnect = param.nConnect;
	int32_t nWidth = param.nWidth;
	int32_t nHeight = param.nHeight;
	int32_t nFlagGrow = 0; // 0 为正常退出，1为过拟合
	int32_t nArea = 0;
	for (int32_t i = 0; i < param.nSize; i++)
	{
		if (nSegIndex[i] >= 1)
		{
			nVisited[i] = 1;
		}
		else
		{
			nVisited[i] = 0;
		}
		nSegSingle[i] = 0;
		nSegConv[i] = 0.0f;
	}
	// 1. 初始化标记矩阵（记录像素是否已被访问）

	// 2. 定义邻域偏移量（上、下、左、右、左上、右上、左下、右下）
	const int32_t dx[] = { -1, 1, 0, 0, -1, -1, 1, 1 };
	const int32_t dy[] = { 0, 0, -1, 1, -1, 1, -1, 1 };

	for (int32_t i = 0; i < 4; i++)
	{
		REGIONINFO attr;
		attr.nNumber = i;
		attr.nArea = 0;
		attr.nWidthMin = 0;
		attr.nWidthMax = 0;
		attr.nHeightMin = 0;
		attr.nHeightMax = 0;
		attr.fThick = 0;
		RegionInfo.push_back(attr);
		std::vector<int32_t> connect;
		vConnectTotal.push_back(connect);
	}
	if (param.BWSeg == 1)
	{
		for (int32_t nSeg = 0; nSeg < param.nSegNumber; nSeg++)
		{
			std::vector<int32_t> connect;
			nArea = 0;
			float32_t fThickSum = 0.0f;
			int32_t nWidthMax = 0;
			int32_t nWidthMin = nWidth;
			int32_t nHeightMax = 0;
			int32_t nHeightMin = nHeight;
			for (int32_t nh = 0; nh < nHeight; nh++)
			{
				for (int32_t nw = 0; nw < nWidth; nw++)
				{
					int32_t nIndex = nh * nWidth + nw;
					if (nSegIndex[nIndex] != nSeg)
					{
						nSegSingle[nIndex] = 0;
					}
					else
					{
						nSegSingle[nIndex] = 1;
						nVisited[nIndex] = 1;
						nWidthMax = std::max(nWidthMax, nw);
						nWidthMin = std::min(nWidthMin, nw);
						nHeightMax = std::max(nHeightMax, nh);
						nHeightMin = std::min(nHeightMin, nh);
						nArea++;
						fThickSum += pfThickCorr[nIndex];
					}
				}
			}
			REGIONINFO attr;
			Score(nSegIndex, pfThick, pfThickAlDiff, fThickPMMA, fThickAl, fThickFe, fAirConv,
				nSegSingle, attr, connect, nHeightMin, nHeightMax, nWidthMin, nWidthMax, nArea, nSeg, fThickSum, param);
			RegionInfo.push_back(attr);
			vConnectTotal.push_back(connect);
		}
	}
	else
	{
		// 3. 遍历所有种子点
		for (const auto& seed : SeedPointInfo)
		{
			nFlagGrow = 0;
			if (seed.nWidth < 0 || seed.nWidth >= nWidth || seed.nHeight < 0 || seed.nHeight >= nHeight)
			{
				continue;  // 跳过无效种子点
			}

			int32_t seedIdx = seed.nHeight * nWidth + seed.nWidth;
			if (((nVisited[seedIdx] == 1) & (-pfThickCorr[seedIdx] < 5)) || (nSegIndex[seedIdx] > 0))
			{
				continue;  // 跳过已处理的种子点
			}
			std::vector<int32_t> connect;
			for (int32_t i = 0; i < param.nSize; i++)
			{
				nSegSingle[i] = 0;
			}
			// 4. 初始化当前区域的统计量
			nArea = 0;
			float32_t fThickSum = 0.0f;
			int32_t nWidthMax = 0;
			int32_t nWidthMin = nWidth;
			int32_t nHeightMax = 0;
			int32_t nHeightMin = nHeight;

			// 5. 使用队列实现区域生长（BFS）
			std::queue<std::pair<int32_t, int32_t>> q;
			q.push({ seed.nWidth, seed.nHeight });
			nVisited[seedIdx] = 1;

			// 6. 获取种子点强度（作为生长参考）
			float32_t seedValue = pfThickCorr[seedIdx];
			float32_t fThreshold = std::min(-pfThickAlDiff[seedIdx], 20.0f) / param.fRegionCoef;
			if (seedValue > 50)
			{
				fThreshold = seedValue / 25 * fThreshold;
			}
			else if (seedValue > 20)
			{
				fThreshold = (seedValue + 10) / 30 * fThreshold;
			}

			while (!q.empty())
			{
				if (nFlagGrow == 1)
				{
					break;
				}
				auto current = q.front();
				q.pop();
				int32_t x = current.first;
				int32_t y = current.second;
				int32_t idx = y * nWidth + x;

				nSegIndex[idx] = param.nSegNumber;
				nSegSingle[idx] = 1;
				// 7. 更新区域统计量
				float32_t pixelValue = pfThickCorr[idx];

				nArea++;
				fThickSum += pixelValue;
				nHeightMax = std::max(nHeightMax, y);
				nHeightMin = std::min(nHeightMin, y);
				nWidthMax = std::max(nWidthMax, x);
				nWidthMin = std::min(nWidthMin, x);
				// 8. 检查4邻域
				for (int32_t i = 0; i < nConnect; ++i)
				{
					int32_t nx = x + dx[i];
					int32_t ny = y + dy[i];
					int32_t nIdx = ny * nWidth + nx;

					// 检查边界和访问状态
					if (nx < 0 || nx >= nWidth || ny < 0 || ny >= nHeight || nVisited[nIdx] == 1)
					{
						continue;
					}

					// 检查强度限制和生长条件（可根据需求修改）
					float32_t neighborValue = pfThickCorr[nIdx];
					// if ((std::abs(neighborValue - seedValue) < 1.5 * fThreshold) && (std::abs(neighborValue - fThickSum / area) < fThreshold))
					// if ((std::abs(neighborValue - fThickSum / area) < fThreshold) && (std::abs(neighborValue - seedValue) < 1.5 * fThreshold))
					float32_t fMean = fThickSum / nArea;
					bool bIndex = true;
					if (param.nView == 1)
					{
						bIndex = ((std::abs(neighborValue - fMean) < fThreshold) && (std::abs(neighborValue - seedValue) < 2.0 * fThreshold));
					}
					else if (param.nView == 2)
					{
						bIndex = (std::abs(neighborValue - fMean) < fThreshold);
					}
					if (bIndex)
					{
						if (std::abs(seedValue - fMean) > fThreshold)
						{
							nFlagGrow = 1;
						}
						nVisited[nIdx] = 1;
						q.push({ nx, ny });
						nSegSingle[idx] = 1;
						nSegIndex[idx] = param.nSegNumber;
					}
					else
					{
						// nVisited[nIdx] = 0;
					}
				}
			}
			REGIONINFO attr;
			Score(nSegIndex, pfThick, pfThickAlDiff, fThickPMMA, fThickAl, fThickFe, fAirConv,
				nSegSingle, attr, connect, nHeightMin, nHeightMax, nWidthMin, nWidthMax, nArea, param.nSegNumber, fThickSum, param);
			RegionInfo.push_back(attr);
			vConnectTotal.push_back(connect);
			param.nSegNumber += 1;
		}
	}
}

void AreaModule::Process_Segment(int32_t* nSegIndex,
	float32_t* pfImgL,
	float32_t* fThickPMMA,
	float32_t* fThickAl1,
	float32_t* fThickFe,
	const float32_t* fThickAlGuidedMinf,
	float32_t* fThickAlCorr,
	const float32_t* fThickAlDiff,
	float32_t* fAirMap,
	float32_t* fAirConv,
	int32_t* nVisited,
	int32_t* nSegSingle,
	int32_t* nSegConv,
	std::vector<SEEDPOINT> SeedPointInfo,
	std::vector<REGIONINFO>& RegionInfo,
	std::vector<std::vector<int32_t>>& vConnectTotal,
	PARAM param)
{
	// 预分割
	int32_t nHeight = param.nHeight;
	int32_t nWidth = param.nWidth;

	for (int32_t nh = 0; nh < nHeight; nh++)
	{
		for (int32_t nw = 0; nw < nWidth; nw++)
		{
			int32_t nIndex = nh * nWidth + nw;
			if (pfImgL[nIndex] > 61000)
			{
				nSegIndex[nIndex] = 1;
				fAirMap[nIndex] = 1.0f;
			}
			else if (pfImgL[nIndex] > 50000)
			{
				nSegIndex[nIndex] = 3;
			}
			else if (pfImgL[nIndex] < 500)
			{
				nSegIndex[nIndex] = 2;
			}

			if (param.nView == 2)
			{
				if (pfImgL[nIndex] < 1000)
				{
					nSegIndex[nIndex] = 2;
				}
			}
		}
	}

	BoxFilterESD(fAirMap, fAirConv, nWidth, nHeight, 50);
	RegionSegment(nSegIndex, fThickAlCorr, fThickAlGuidedMinf, fThickAlDiff, fThickPMMA, fThickAl1, fThickFe, fAirConv, nVisited, nSegSingle, nSegConv,
		SeedPointInfo, RegionInfo, vConnectTotal, param);
}

void AreaModule::Process_Merge(int32_t* nSegIndex,
	int32_t* nSegSingle,
	std::vector<REGIONINFO>& RegionInfo,
	std::vector<std::vector<int32_t>>& vConnectTotal,
	float32_t* pfThick,
	float32_t* pfThickAlDiff,
	float32_t* fThickPMMA,
	float32_t* fThickAl,
	float32_t* fThickFe,
	float32_t* fAirConv,
	PARAM param)
{
	int32_t nHeight = param.nHeight;
	int32_t nWidth = param.nWidth;

	for (int32_t i = RegionInfo.size() - 1; i >= 4; i--)
	{
		for (int32_t j = 0; j < static_cast<int32_t>(vConnectTotal[i].size()); j++)
		{
			int32_t seg_num_ext = vConnectTotal[i][j];
			std::vector<int32_t> connect;
			int32_t nHeightMin = std::max(std::min(RegionInfo[i].nHeightMin, RegionInfo[seg_num_ext].nHeightMin), 0);
			int32_t nHeightMax = std::min(std::max(RegionInfo[i].nHeightMax, RegionInfo[seg_num_ext].nHeightMax), nHeight);
			int32_t nWidthMin = std::max(std::min(RegionInfo[i].nWidthMin, RegionInfo[seg_num_ext].nWidthMin), 0);
			int32_t nWidthMax = std::min(std::max(RegionInfo[i].nWidthMax, RegionInfo[seg_num_ext].nWidthMax), nWidth);
			int32_t nArea = RegionInfo[i].nArea + RegionInfo[i].nArea;

			bool b2 = (RegionInfo[i].fThick > 10);
			bool b3 = (RegionInfo[seg_num_ext].fThick > 10);
			bool b6 = (RegionInfo[i].fScore > 0.1);
			bool b7 = (RegionInfo[seg_num_ext].fThickAlDiff > 5);
			if (b2 & b3 & b6 & b7)
			{
				for (int32_t j = 0; j < nHeight * nWidth; j++)
				{
					nSegSingle[j] = 0;
				}
				for (int32_t nh = nHeightMin; nh < nHeightMax; nh++)
				{
					for (int32_t nw = nWidthMin; nw < nWidthMax; nw++)
					{
						int32_t index = nh * nWidth + nw;
						if ((nSegIndex[index] == i) || (nSegIndex[index] == seg_num_ext))
						{
							nSegSingle[index] = 1;
						}
					}
				}
				REGIONINFO attr;
				std::vector<int32_t> connect;

				float32_t fThickSum = (RegionInfo[i].fThick * RegionInfo[i].nArea + RegionInfo[seg_num_ext].fThick * RegionInfo[seg_num_ext].nArea) / (RegionInfo[i].nArea + RegionInfo[seg_num_ext].nArea);
				Score(nSegIndex, pfThick, pfThickAlDiff, fThickPMMA, fThickAl, fThickFe, fAirConv,
					nSegSingle, attr, connect, nHeightMin, nHeightMax, nWidthMin, nWidthMax, nArea, seg_num_ext, fThickSum, param);
				
				bool b11 = (attr.fScore > RegionInfo[i].fScore) && (attr.fScore > RegionInfo[seg_num_ext].fScore);
				bool b12 = attr.fScore > 0.9;
				int32_t nArea0 = ((nHeightMax - nHeightMin) * (nWidthMax - nWidthMin));
				int32_t nArea1 = (RegionInfo[i].nHeightMax - RegionInfo[i].nHeightMax) * (RegionInfo[i].nWidthMax - RegionInfo[i].nWidthMin);
				int32_t nArea2 = (RegionInfo[seg_num_ext].nHeightMax - RegionInfo[seg_num_ext].nHeightMax) * (RegionInfo[seg_num_ext].nWidthMax - RegionInfo[seg_num_ext].nWidthMin);

				
				bool b13 = true;
				if (param.nView == 1)
				{
					b13 = std::max(nArea1, nArea2) / std::max(nArea0, 1) < param.fScoreThsMain;
				}
				else if (param.nView == 2)
				{
					b13 = std::max(nArea1, nArea2) / std::max(nArea0, 1) < param.fScoreThsAux;
				}
				if ((b12 & b13) || b11)
				{
					RegionInfo[seg_num_ext].fScore = attr.fScore;
					RegionInfo[seg_num_ext].fThickAlDiff = attr.fThickAlDiff;
					RegionInfo[seg_num_ext].fThickAlDiffMin = attr.fThickAlDiffMin;
					RegionInfo[seg_num_ext].fWeight = attr.fWeight;
					RegionInfo[seg_num_ext].nHeightMax = std::max(RegionInfo[i].nHeightMax, RegionInfo[seg_num_ext].nHeightMax);
					RegionInfo[seg_num_ext].nHeightMin = std::min(RegionInfo[i].nHeightMin, RegionInfo[seg_num_ext].nHeightMin);
					RegionInfo[seg_num_ext].nWidthMax = std::max(RegionInfo[i].nWidthMax, RegionInfo[seg_num_ext].nWidthMax);
					RegionInfo[seg_num_ext].nWidthMin = std::min(RegionInfo[i].nWidthMin, RegionInfo[seg_num_ext].nWidthMin);
					RegionInfo[seg_num_ext].nArea = RegionInfo[i].nArea + RegionInfo[seg_num_ext].nArea;
					RegionInfo[i].nArea = 0;
					RegionInfo[i].fScore = 0;
					RegionInfo[i].fThickAlDiff = 0;
					RegionInfo[i].fThickAlDiffMin = 0;
					RegionInfo[i].fWeight = 0;
					RegionInfo[i].fRatioAir = 0;
					RegionInfo[i].nHeightMax = 0;
					RegionInfo[i].nHeightMin = 0;
					RegionInfo[i].nWidthMax = 0;
					RegionInfo[i].nWidthMin = 0;
					for (int32_t nh = nHeightMin; nh < nHeightMax; nh++)
					{
						for (int32_t nw = nWidthMin; nw < nWidthMax; nw++)
						{
							int32_t index = nh * nWidth + nw;
							if (nSegIndex[index] == i)
							{
								nSegIndex[index] = seg_num_ext;
							}
						}
					}
				}
			}
		}
	}
}

void AreaModule::RegionExp(
	unsigned short* psImgL,             //输入低能
	unsigned short* psImgH,             //输入高能
	unsigned char* pcImgZ,
	XRAYLIB_CONCERED_AREA* ExplosiveArea,  //输出位置信息结构体
	int32_t nWidth,
	int32_t nHeight)
{
	//************************原输入图像环境和算法库一致**************************
	/* STEP 0: DECLARE VARS */
	auto start = std::chrono::high_resolution_clock::now(); // 计时函数
	int32_t size = nWidth * nHeight;
	std::vector<float32_t> pfImgL(size, 0.0f);
	std::vector<float32_t> pfImgH(size, 0.0f);
	std::vector<float32_t> pfImgZ(size, 0.0f);
	std::vector<float32_t> pfImgLGuided(size, 0.0f);
	std::vector<float32_t> pfImgHGuided(size, 0.0f);

	std::vector<float32_t> fThickPMMA(size, 0.0f);
	std::vector<float32_t> fThickFe(size, 0.0f);
	std::vector<float32_t> fThickFeMinf(size, 0.0f);
	std::vector<float32_t> fThickAl(size, 0.0f);
	std::vector<float32_t> fThickAl1(size, 0.0f); // PMMA+Al分解结果
	std::vector<float32_t> fThickAlCorr(size, 0.0f); // sigmoid映射
	std::vector<float32_t> fThickAlGuided(size, 0.0f);
	std::vector<float32_t> fThickAlGuidedMinf(size, 0.0f);
	std::vector<float32_t> fThickAlGuidedMinfThs(size, 0.0f);
	std::vector<float32_t> fThickInputInner(size, 0.0f);
	std::vector<float32_t> fThickInputOuter(size, 0.0f);
	std::vector<float32_t> fThickAlDiff(size, 0.0f);
	std::vector<SEEDPOINT> SeedPointInfo;
	std::vector<REGIONINFO> RegionInfo;  // 区域分割属性
	std::vector<int32_t> nSegIndex(size, 0); // 区域分割结果

	std::vector<int32_t> nSegSingle(size, 0); // 单次区域分割结果
	std::vector<int32_t> nSegConv(size, 0); // 单次区域分割结果
	std::vector<int32_t> nSegSingleFill(size, 0); // 单次区域分割结果
	std::vector<int32_t> nSegSingleDilate(size, 0);
	std::vector<int32_t> nSegSingleBw(size, 0);
	std::vector<int32_t> nSegSingleBwTemp(size, 0);
	std::vector<int32_t> nSegSingleErode(size, 0);
	std::vector<int32_t> nVisited(size, 0);
	std::vector<float32_t> fAirMap(size, 0.0f);
	std::vector<float32_t> fAirConv(size, 0.0f);
	std::vector<std::vector<int32_t>> vConnectTotal;

	std::vector<int32_t> nBWinput(size, 0);
	std::vector<int32_t> nBWlabel(size, 0);
	std::vector<int32_t> nSegIndexBW(size, 0); // 区域BW分割结果
	std::vector<std::vector<int32_t>> vConnectTotalBW;
	std::vector<REGIONINFO> RegionInfoBW;  // 区域分割属性
	std::vector<float32_t> fThickAlDiffBW(size, 0.0f); // 厚度差异
	std::vector<float32_t> fThickInputInnerBW(size, 0.0f);
	PARAM param;
	param.nHeight = nHeight;
	param.nWidth = nWidth;
	if (XRAYLIB_VISUAL_MAIN == m_pSharedPara->m_enVisual)
	{
		param.nView = 1;
		param.BWSeg = 0;
		param.fRegionCoef = 3.5;
		log_info("Processing MainView");
	}
	else if (XRAYLIB_VISUAL_AUX == m_pSharedPara->m_enVisual)
	{
		param.nView = 2;
		param.BWSeg = 0;
		param.nMinFilterSize = 1.0;
		param.fRegionCoef = 5.0;
		param.fFeThs = -20.0;
		param.fScoreWeight = 30.0;
		param.fScoreDiff = 15.0;
		log_info("Processing AuxView");
	}
	else
	{
		log_info("No such nView value, Default Using MainView");
		m_pSharedPara->m_enVisual = XRAYLIB_VISUAL_MAIN;
		param.nView = 1;
		param.BWSeg = 0;
		param.fRegionCoef = 3.5;
		log_info("Processing MainView");
	}
	param.nScale = 1;
	// 1. 初始化
	Process_Rescale(psImgL, psImgH, pcImgZ, pfImgL.data(), pfImgH.data(), pfImgZ.data(), param);
	pfImgL.resize(param.nSize);
	pfImgH.resize(param.nSize);
	pfImgZ.resize(param.nSize);

	//*********************************************************************************************************
	// 2.引导滤波
	Process_Filter(pfImgL.data(), pfImgH.data(), pfImgLGuided.data(), pfImgHGuided.data(), param);

	//*********************************************************************************************************
	// 3.生成基材料厚度
	CalcThick(pfImgLGuided.data(), pfImgHGuided.data(), fThickPMMA.data(), fThickAl.data(), fThickAl1.data(), fThickFe.data(), param);
	
	Process_Stripe(pfImgLGuided.data(), pfImgHGuided.data(), fThickAl.data(), param);
	GuidedFilter(fThickAl.data(), fThickAlGuided.data(), nWidth, nHeight, param.nNeighborhood, param.fDegreeOfSmoothMu);
	
	//*********************************************************************************************************
	// 4.定义种子点
	MinFilter(fThickAlGuided.data(), fThickAlGuidedMinf.data(), fThickAlGuidedMinfThs.data(), param);
	param.nMinFilterSizeFe = 2;
	MinFilter(fThickFe.data(), fThickFeMinf.data(), param);
	Process_FindSeed(fThickAlGuidedMinfThs.data(), fThickAlCorr.data(), fThickFeMinf.data(), fThickInputInner.data(), fThickInputOuter.data(), fThickAlDiff.data(), SeedPointInfo, param);
	
	//*********************************************************************************************************
	// 5.区域分割
	Process_Segment(nSegIndex.data(), pfImgLGuided.data(), fThickPMMA.data(), fThickAl.data(), fThickFeMinf.data(),
		fThickAlGuidedMinf.data(), fThickAlCorr.data(), fThickAlDiff.data(), fAirMap.data(), fAirConv.data(),
		nVisited.data(), nSegSingle.data(), nSegConv.data(), SeedPointInfo, RegionInfo, vConnectTotal, param);
	
	//*********************************************************************************************************
	// 6.评分模块
	Process_Merge(nSegIndex.data(), nSegSingle.data(), RegionInfo, vConnectTotal, fThickAl.data(), fThickAlDiff.data(),
		fThickPMMA.data(), fThickAl.data(), fThickFe.data(), fAirConv.data(), param);
	std::string s = "Time elapsed: Score";
	auto end = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	log_info("%s(%d): %d ms", s, param.nView, duration.count());

	for (int32_t i = 0; i < param.nSize; i++)
	{
		if (nSegIndex[i] == 0)
		{
			if (fThickFeMinf[i] < param.fFeThs)
			{
				nSegIndex[i] = 2;
			}
		}
	}
	// 薄片程序
	if (param.nView == 2)
	{
		BoxFilterESD(fThickAlGuided.data(), fThickInputInnerBW.data(), nWidth, nHeight, 5);
		for (int32_t i = 0; i < param.nSize; i++)
		{
			nBWinput[i] = 0;
			if ((fThickAlGuided[i] > param.fThickThsBW) & (fThickAlDiff[i] < -param.fThickThsBW))
			{
				fThickAlDiffBW[i] = fThickAlGuided[i] - fThickInputInnerBW[i];
				if (fThickAlDiffBW[i] > -1)
				{
					nBWinput[i] = 1;
				}
			}
		}
		std::vector<int32_t> nBWarea;
		ConnectedComponents(nBWinput.data(), param.nWidth, param.nHeight, nBWlabel.data(), nBWarea);

		auto first_to_remove = std::upper_bound(
			nBWarea.begin(),
			nBWarea.end(),
			param.fAreaThsBW
		);
		nBWarea.erase(first_to_remove, nBWarea.end());
		param.nSegNumber = param.nSegNumber + nBWarea.size();

		for (int32_t i = 0; i < param.nSize; i++)
		{
			if (nBWlabel[i] == 0)
			{
				if (pfImgLGuided[i] > param.unMaxGray)
				{
					nSegIndexBW[i] = 1;
				}
				else
				{
					nSegIndexBW[i] = 3;
				}
			}
			else if (nBWlabel[i] < static_cast<int32_t>(nBWarea.size()) + 1)
			{
				nSegIndexBW[i] = nBWlabel[i] + 3;
			}
		}

		param.BWSeg = 1;
		Process_Segment(nSegIndexBW.data(), pfImgLGuided.data(), fThickPMMA.data(), fThickAl.data(), fThickFeMinf.data(),
			fThickAlGuidedMinf.data(), fThickAlCorr.data(), fThickAlDiff.data(), fAirMap.data(), fAirConv.data(),
			nVisited.data(), nSegSingle.data(), nSegConv.data(), SeedPointInfo, RegionInfoBW, vConnectTotalBW, param);
	}

	// Anchor merge
	std::vector<REGIONINFO> RegionInfoEff;

	for (int32_t i = 0; i < static_cast<int32_t>(RegionInfo.size()); i++)
	{
		if (RegionInfo[i].fScore < 0.5)
		{
			break;
		}
		REGIONINFO attr;
		attr = RegionInfo[i];
		attr.nHeightMin = std::max(0, RegionInfo[i].nHeightMin - param.nMinFilterSize);
		attr.nHeightMax = std::min(param.nHeight, RegionInfo[i].nHeightMax + param.nMinFilterSize);
		attr.nWidthMin = std::max(0, RegionInfo[i].nWidthMin - param.nMinFilterSize);
		attr.nWidthMax = std::min(param.nWidth, RegionInfo[i].nWidthMax + param.nMinFilterSize);

		int32_t nHMax1 = RegionInfo[i].nHeightMax;
		int32_t nHMin1 = RegionInfo[i].nHeightMin;
		int32_t nWMax1 = RegionInfo[i].nWidthMax;
		int32_t nWMin1 = RegionInfo[i].nWidthMin;
		for (int32_t j = 0; j < static_cast<int32_t>(RegionInfo.size()); j++)
		{
			if (RegionInfo[j].fScore < 0.5)
			{
				break;
			}
			int32_t nHMax2 = RegionInfo[j].nHeightMax;
			int32_t nHMin2 = RegionInfo[j].nHeightMin;
			int32_t nWMax2 = RegionInfo[j].nWidthMax;
			int32_t nWMin2 = RegionInfo[j].nWidthMin;

			int32_t nHMaxMax = std::max(nHMax1, nHMax2);
			int32_t nHMinMin = std::min(nHMin1, nHMin2);
			int32_t nWMaxMax = std::max(nWMax1, nWMax2);
			int32_t nWMinMin = std::min(nWMin1, nWMin2);

			int32_t nAreaU = std::max((nHMaxMax - nHMinMin) * (nWMaxMax - nWMinMin), 1);
			int32_t nArea1 = (nHMax1 - nHMin1) * (nWMax1 - nWMin1);
			int32_t nArea2 = (nHMax2 - nHMin2) * (nWMax2 - nWMin2);

			if (((nArea1 / nAreaU) > 0.8) || (nArea2 / nAreaU) > 0.8)
			{
				attr.nHeightMin = std::max(0, nHMinMin - param.nMinFilterSize);
				attr.nHeightMax = std::min(param.nHeight, nHMaxMax + param.nMinFilterSize);
				attr.nWidthMin = std::max(0, nWMinMin - param.nMinFilterSize);
				attr.nWidthMax = std::min(param.nWidth, nWMaxMax + param.nMinFilterSize);
				RegionInfo.erase(RegionInfo.begin() + j);
				break;
			}
			RegionInfoEff.push_back(attr);
		}
	}

	ExplosiveArea->nNum = 0;

	if (RegionInfo.size() == 0)
	{
		ExplosiveArea->nNum = 0;
	}
	else if (RegionInfo.size() > 0)
	{
		for (int32_t i = 0; i < static_cast<int32_t>(RegionInfo.size()); i++)
		{
			if (param.nView == 1)
			{
				if ((RegionInfo[i].fScore > 0.8) && (RegionInfo[i].fThickAlDiff > 5) && ((RegionInfo[i].fRatioAl / RegionInfo[i].fRatioPMMA < 0.9)))
				{
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nLeft = std::max(RegionInfo[i].nWidthMin - param.nMinFilterSize, 0);
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nRight = std::min(RegionInfo[i].nWidthMax + param.nMinFilterSize, param.nWidth - 1);
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nBottom = std::max(RegionInfo[i].nHeightMin - param.nMinFilterSize, 0);
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nTop = std::min(RegionInfo[i].nHeightMax + param.nMinFilterSize, param.nHeight - 1);
					ExplosiveArea->nNum = ExplosiveArea->nNum + 1;
				}
			}
			else if (param.nView == 2)
			{
				if ((RegionInfo[i].fScore > 0.9) && (RegionInfo[i].fThickAlDiff > 5) && ((RegionInfo[i].fRatioAl / RegionInfo[i].fRatioPMMA < 0.9)))
				{
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nLeft = std::max(RegionInfo[i].nWidthMin - 100 - param.nMinFilterSize, 0);
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nRight = std::min(RegionInfo[i].nWidthMax - 100 + param.nMinFilterSize, param.nWidth - 1);
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nBottom = std::max(RegionInfo[i].nHeightMin - param.nMinFilterSize, 0);
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nTop = std::min(RegionInfo[i].nHeightMax + param.nMinFilterSize, param.nHeight - 1);
					/*
					std::cout << ExplosiveArea->CMRect[ExplosiveArea->nNum].nLeft << " "
								<< ExplosiveArea->CMRect[ExplosiveArea->nNum].nRight << " "
								<< ExplosiveArea->CMRect[ExplosiveArea->nNum].nTop << " "
								<< ExplosiveArea->CMRect[ExplosiveArea->nNum].nBottom << " " << std::endl;*/
					ExplosiveArea->nNum = ExplosiveArea->nNum + 1;
				}
			}
		}
		if ((param.nView == 2) && (ExplosiveArea->nNum == 0))
		{
			for (int32_t i = 0; i < static_cast<int32_t>(RegionInfoBW.size()); i++)
			{
				if ((RegionInfoBW[i].fScore > 0.9) && (RegionInfoBW[i].fThickAlDiff > 5) && ((RegionInfoBW[i].fRatioAl / RegionInfoBW[i].fRatioPMMA < 0.95)))
				{
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nLeft = std::max(RegionInfoBW[i].nWidthMin - 100 - param.nMinFilterSize, 0);
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nRight = std::min(RegionInfoBW[i].nWidthMax - 100 + param.nMinFilterSize, param.nWidth - 1);
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nBottom = std::max(RegionInfoBW[i].nHeightMin - param.nMinFilterSize, 0);
					ExplosiveArea->CMRect[ExplosiveArea->nNum].nTop = std::min(RegionInfoBW[i].nHeightMax + param.nMinFilterSize, param.nHeight - 1);
					ExplosiveArea->nNum = ExplosiveArea->nNum + 1;
				}
			}
		}
	}
}