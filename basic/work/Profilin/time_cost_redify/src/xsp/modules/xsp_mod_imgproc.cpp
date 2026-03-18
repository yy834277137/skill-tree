/**
*      @file xsp_mod_imgproc.cpp
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*
*      @brief XSP图像处理模块
*
*      @note
*/

#include "xsp_mod_imgproc.hpp"
#include <math.h>
#include <algorithm>
#include <string>
#include <chrono>
#include <iomanip>

/**************************************************************************
*                                滤波核
***************************************************************************/
int32_t roundXYGetDualImg[25][2] = { { 0,0 },{ 0,-1 },{ -1,-1 },{ -1,0 },{ -1,1 },{ 0,1 },{ 1,1 },{ 1,0 },{ 1,-1 },{ 1,-2 },{ 0,-2 },{ -1,-2 },
								 { -2,-2 },{ -2,-1 },{ -2,0 },{ -2,1 },{ -2,2 },{ -1,2 },{ 0,2 },{ 1,2 },{ 2,2 },{ 2,1 },{ 2,0 },{ 2,-1 },{ 2,-2 } };
int32_t kernel[3][3] = { { -1,-1,-1 },{ -1,9,-1 },{ -1,-1,-1 } };

#define XRAY_LIB_IMAGEPROC_FILTER_SIZE      2     ///超增、边增、默认增强等滤波核大小。
#define XRAY_LIB_PI                         3.14159265359
#define idx2(x, y, dim_x) ((x) + ((y) * (dim_x)) )

/**************************************************************************
*                                共享参数初始化
***************************************************************************/
XRAY_LIB_HRESULT ImgProcModule::Init(SharedPara* pPara, XRAY_LIB_ABILITY& ability)
{
	if (XRAYLIB_ENERGY_SCATTER == ability.nEnergyMode)
	{
		return XRAY_LIB_OK;
	}
	else
	{
		m_pSharedPara = pPara;
		XRAY_LIB_HRESULT hr;
		hr = MakeLut(m_pnALUT, 0, 65535);
		XSP_CHECK(XRAY_LIB_OK == hr, hr);
		m_islPipeGray.m_nPipeResizeWidth = pPara->m_nDetectorWidth * pPara->m_fResizeScale;
	}

	return XRAY_LIB_OK;
}

/**************************************************************************
*                                变量初始化
***************************************************************************/
ImgProcModule::ImgProcModule()
{
	// 局增、超增、边增、默认增强的灰度值范围
	m_nEnhanceGrayUp = 62000;
	m_nEnhanceGrayDown = 400;
	memset(m_nRoundPos, 0, sizeof(int32_t) * 25);

	// 默认增强参数
	m_fDefaultEnhanceIntensity = 2.0f;        // 默认增强强度
	m_fEdgeEnhanceIntensity = 2.0f;			 // 默认边缘增强强度
	m_nMergeBaseLine = 0;                     // 融合图拉伸基线值

	// ISL模块参数设置
	m_nLumIntensity = 50;					 //ISL模块亮度调整系数
	m_nContrastRatio = 50;					 //ISL模块对比度调整系数
	m_nSharpnessRatio = 50;					 // ISL模块锐化系数
	m_nLowLumCompensation = 50;				 // ISL模块难穿区补偿系数
	m_nHighLumSensity = 50;					 // ISL模块易穿区敏感度

	// 局部增强参数
	m_nLocalEnhanceThresholdUP = 600;	      // 局部拉伸阈值上限
	m_nLocalEnhanceThresholdDOWN = 0;       // 局部拉伸阈值下限
	m_nCaliLowGrayThreshold = 500;            // 低灰度阈值
	m_fRTUpdateLowGrayRatio = 0.02f;          // 低灰度区实时更新参数
	m_nWindowCenter = 100;					  // 局增窗位

	m_enEnhanceMode = XRAYLIB_ENHANCE_NORAML;
	m_enDefaultEnhance = XRAYLIB_DEFAULTENHANCE_MODE1;
	m_enGammaMode = XRAYLIB_GAMMA_CLOSE;
	m_enFusionMode = XRAYLIB_FUSION_DEFAULT;

	//CLAHE参数
	m_nNrBins = 1500;
	m_nKernelSizeX = 32;
	m_nKernelSizeY = 32;
	m_fCliplimit = 0.2f;

	m_enTestMode = XRAYLIB_TESTMODE_CN;
	m_fTest5Ratio = 1.0;

	m_enTestAutoLE = XRAYLIB_OFF;
}

ImgProcModule::~ImgProcModule()
{

}

/**************************************************************************
*                             内存申请与释放
***************************************************************************/
XRAY_LIB_HRESULT ImgProcModule::Release()
{
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT ImgProcModule::GetMemSize(XRAY_LIB_MEM_TAB &MemTab, XRAY_LIB_ABILITY &ability)
{
	MemTab.size = 0;
	/******************************************************
	*                 图像处理内存
	*******************************************************/
	int32_t nRTProcessHeight = int32_t((ability.nMaxHeightRealTime + 2 * XRAY_LIB_MAX_FILTER_KERNEL_LENGTH) * ability.fResizeScale);
	int32_t nRTProcessWidth = int32_t(ability.nDetectorWidth * ability.fResizeScale);

	int32_t nEntrieProcessHeight = XRAY_LIB_MAX_IMGENHANCE_LENGTH + XRAY_LIB_MAX_FILTER_KERNEL_LENGTH * 2;

	if (XRAYLIB_ENERGY_SCATTER == ability.nEnergyMode)
	{
		/* Sobel 增强临时内存 */
		m_matImgTemph2Den.SetMem(nEntrieProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matImgTemph2Den.data, m_matImgTemph2Den.Size(), MemTab);
		
		m_matImgTempw2Den.SetMem(nEntrieProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matImgTempw2Den.data, m_matImgTempw2Den.Size(), MemTab);
		
		m_matImgTempw2Tmp.SetMem(nEntrieProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matImgTempw2Tmp.data, m_matImgTempw2Tmp.Size(), MemTab);
	}
	else
	{
		if (((ability.nPipeLineMode & 0xF00) == 0x100) && (!ability.nUseAI))	// 2级流水且不开AI
		{
			/* 去噪 */
			m_matDuplicateImage.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matDuplicateImage.data, m_matDuplicateImage.Size(), MemTab);

			m_matDataImage.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matDataImage.data, m_matDataImage.Size(), MemTab);

			m_matGImage1.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matGImage1.data, m_matGImage1.Size(), MemTab);

			m_matPImage1.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matPImage1.data, m_matPImage1.Size(), MemTab);

			m_matRImage1.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matRImage1.data, m_matRImage1.Size(), MemTab);

			m_matGImage2.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matGImage2.data, m_matGImage2.Size(), MemTab);

			m_matPImage2.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matPImage2.data, m_matPImage2.Size(), MemTab);

			m_matRImage2.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
			XspMalloc((void**)&m_matRImage2.data, m_matRImage2.Size(), MemTab);
		}

		/* 局部增强的用于羽化的权重图 */ 
		m_matWeightTemp.SetMem(nEntrieProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_32FC1));
		XspMalloc((void**)&m_matWeightTemp.data, m_matWeightTemp.Size(), MemTab);

		/* 对比度增强 */
		int32_t nEntrieProcessHeight = XRAY_LIB_MAX_IMGENHANCE_LENGTH + XRAY_LIB_MAX_FILTER_KERNEL_LENGTH * 2;
		int32_t nEntrieProcessWidth = nRTProcessWidth;

		m_matCompositiveTemp.SetMem(nEntrieProcessHeight * nEntrieProcessWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matCompositiveTemp.data, m_matCompositiveTemp.Size(), MemTab);

		m_matGammaMem.SetMem(nEntrieProcessHeight * nEntrieProcessWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matGammaMem.data, m_matGammaMem.Size(), MemTab);

		m_matGammaMask.SetMem(nEntrieProcessHeight * nEntrieProcessWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matGammaMask.data, m_matGammaMask.Size(), MemTab);

		m_matGammaMaskSmooth.SetMem(nEntrieProcessHeight * nEntrieProcessWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matGammaMaskSmooth.data, m_matGammaMaskSmooth.Size(), MemTab);

		m_matRtCache.SetMem(nRTProcessHeight * nRTProcessWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matRtCache.data, m_matRtCache.Size(), MemTab);
		m_matRtCache.Reshape(nRTProcessHeight, nRTProcessWidth, XSP_16UC1);

		/* 超级增强2 */
		m_matImageTemp1.SetMem((nEntrieProcessHeight + m_nKernelSizeY * 2) * nRTProcessWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matImageTemp1.data, m_matImageTemp1.Size(), MemTab);

		m_matImageTemp2.SetMem((nEntrieProcessHeight + m_nKernelSizeY * 2) * nRTProcessWidth * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matImageTemp2.data, m_matImageTemp2.Size(), MemTab);

		m_matImageTemp3.SetMem(m_nKernelSizeX * m_nKernelSizeY * m_nNrBins * XSP_ELEM_SIZE(XSP_16UC1));
		XspMalloc((void**)&m_matImageTemp3.data, m_matImageTemp3.Size(), MemTab);

		XspMalloc((void**)&m_pnALUT, 65536 * sizeof(uint16_t), MemTab);
	}	

	return XRAY_LIB_OK;
}

/**************************************************************************
*                               图像融合
***************************************************************************/
// XRAY_LIB_HRESULT ImgProcModule::ImgMerging(XMat& matMerge, XMat& matLow, XMat& matHigh, XMat& matWt)
// {
// 	/* check para */
// 	XSP_CHECK(matHigh.IsValid() && matLow.IsValid() && matMerge.IsValid() && matWt.IsValid(), 
//               XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

//     XSP_CHECK(XSP_16U == matHigh.type && XSP_16U == matLow.type && XSP_16U == matMerge.type && XSP_8U == matWt.type,
//               XRAY_LIB_XMAT_TYPE_ERR);

// 	XSP_CHECK(MatSizeEq(matHigh, matMerge) && MatSizeEq(matLow, matMerge) && MatSizeEq(matWt, matMerge), 
//               XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

// 	/* init and define */
// 	uint16_t *pHighCaliData = matHigh.Ptr<uint16_t>();
// 	uint16_t *pLowCaliData = matLow.Ptr<uint16_t>();
// 	uint16_t *pMergeOut = matMerge.Ptr<uint16_t>();
//     uint8_t *pWt = matWt.Ptr<uint8_t>();

//     // TODO: 这里可以建立融合灰度图的缓存，避免重复条带的融合
// 	int32_t nLength = matHigh.hei * matHigh.wid;
// 	for (int32_t i = 0; i < nLength; i++)
// 	{
//         if (pWt[i])
//         {
//             float64_t fHLRatio = std::sqrt(static_cast<float64_t>(pHighCaliData[i]) / 65535.0);
//             float64_t fResult = fHLRatio * pLowCaliData[i] + (1.0 - fHLRatio) * pHighCaliData[i];
//             pMergeOut[i] = std::clamp(static_cast<int32_t>(std::round(fResult + m_nMergeBaseLine)), 0, 65535);
//         }
//         else
//         {
//             pMergeOut[i] = 65535;
//         }
// 	}

// 	return XRAY_LIB_OK;
// }

XRAY_LIB_HRESULT ImgProcModule::ImgMerging(XMat& matMerge, XMat& matLow, XMat& matHigh, [[maybe_unused]]XMat& matWt)
{
	/* check para */
	XSP_CHECK(matHigh.IsValid() && matLow.IsValid() && matMerge.IsValid(), 
              XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

    XSP_CHECK(XSP_16U == matHigh.type && XSP_16U == matLow.type && XSP_16U == matMerge.type,
              XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(matHigh, matMerge) && MatSizeEq(matLow, matMerge), 
              XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	/* init and define */
	uint16_t *pHighCaliData = matHigh.Ptr<uint16_t>();
	uint16_t *pLowCaliData = matLow.Ptr<uint16_t>();
	uint16_t *pMergeOut = matMerge.Ptr<uint16_t>();

    // TODO: 这里可以建立融合灰度图的缓存，避免重复条带的融合
	int32_t nLength = matHigh.hei * matHigh.wid;
	#pragma omp parallel for schedule(static, 8)
	for (int32_t i = 0; i < nLength; i++)
	{
        float64_t fHLRatio = std::sqrt(static_cast<float64_t>(pHighCaliData[i]) / 65535.0);
		float64_t fResult = fHLRatio * pLowCaliData[i] + (1.0 - fHLRatio) * pHighCaliData[i];
		pMergeOut[i] = std::clamp(static_cast<int32_t>(std::round(fResult + m_nMergeBaseLine)), 0, 65535);
	}

	return XRAY_LIB_OK;
}


XRAY_LIB_HRESULT ImgProcModule::ImgMergingAI(XMat& matHighCaliData, XMat& matLowCaliData, XMat& matMergeOut)
{
	/* check para */
	XSP_CHECK(matHighCaliData.IsValid() && matLowCaliData.IsValid() && matMergeOut.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_16U == matHighCaliData.type && XSP_16U == matLowCaliData.type && XSP_16U == matMergeOut.type, XRAY_LIB_XMAT_TYPE_ERR);

	XSP_CHECK(MatSizeEq(matHighCaliData, matMergeOut) && MatSizeEq(matLowCaliData, matMergeOut), XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	/* init and define  */
	uint16_t *pHighCaliData = matHighCaliData.Ptr<uint16_t>();
	uint16_t *pLowCaliData = matLowCaliData.Ptr<uint16_t>();
	uint16_t *pMergeOut = matMergeOut.Ptr<uint16_t>();

	int32_t nLength = matHighCaliData.hei * matHighCaliData.wid;
	float32_t fHLRatio;
	float32_t fResult;

	/* process */
	/* 这里是与raw2yuv对齐的写法 代码不要改动!!!! */
	for (int32_t i = 0; i < nLength; i++)
	{
		fHLRatio = (float32_t)pHighCaliData[i] / 65535.0;
		fHLRatio = sqrt(fHLRatio);
		fResult = pLowCaliData[i] * fHLRatio + pHighCaliData[i] * (1.0 - fHLRatio);
		if (fResult < 0)
			fResult = 0;
		if (fResult > 65535)
			fResult = 65535;
		fResult = fResult + 0.005;
		pMergeOut[i] = (uint16_t)(fResult);
	}
	return XRAY_LIB_OK;
}
/**************************************************************************
*                               图像去噪
***************************************************************************/
/**@fn      ImgDenoisingLow
* @brief    低能图像去噪（支持8位和16位）
* @param1   matDenoiseIn      [I]     - 输入融合图像
* @param2   matDenoiseOut     [O]     - 输出去噪后的图像
* @return   错误码
* @note
*/
XRAY_LIB_HRESULT ImgProcModule::ImgDenoisingLow(XMat& matDenoiseIn, XMat& matDenoiseOut)
{
	/* check para */
	XSP_CHECK(matDenoiseIn.IsValid() && matDenoiseOut.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");
	XSP_CHECK(XSP_16U == matDenoiseIn.type && XSP_16U == matDenoiseOut.type, XRAY_LIB_XMAT_TYPE_ERR);
	XSP_CHECK(MatSizeEq(matDenoiseIn, matDenoiseOut), XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	/* init and define */
	uint16_t *pDenoiseIn = matDenoiseIn.Ptr<uint16_t>();
	uint16_t *pDenoiseOut = matDenoiseOut.Ptr<uint16_t>();
	int32_t nHeight = matDenoiseIn.hei;
	int32_t nWidth = matDenoiseOut.wid;
	float32_t spatialDecay = 0.55f;
	float32_t photometricStandardDeviation = 10.0f;

	for (int32_t k = 0; k < nHeight*nWidth; k++)
	{
		m_matDuplicateImage.data.fl[k] = pDenoiseIn[k] / 255.0f;
	}

	BEEPSHorizontalVertical(m_matDuplicateImage.data.fl, nWidth, nHeight, photometricStandardDeviation, spatialDecay);

	for (int32_t k = 0; k < nWidth*nHeight; k++)
	{
		pDenoiseOut[k] = uint16_t(m_matDuplicateImage.data.fl[k] * 255.0f);
	}

	return XRAY_LIB_OK;
}

/**@fn      ImgDenoisingHigh
* @brief    高能图像去噪（支持8位和16位）
* @param1   matDenoiseIn      [I]     - 输入融合图像
* @param2   matDenoiseOut     [O]     - 输出去噪后的图像
* @return   错误码
* @note
*/
XRAY_LIB_HRESULT ImgProcModule::ImgDenoisingHigh(XMat& matDenoiseIn, XMat& matDenoiseOut)
{
	/* check para */
	XSP_CHECK(matDenoiseIn.IsValid() && matDenoiseOut.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");
	XSP_CHECK(XSP_16U == matDenoiseIn.type && XSP_16U == matDenoiseOut.type, XRAY_LIB_XMAT_TYPE_ERR);
	XSP_CHECK(MatSizeEq(matDenoiseIn, matDenoiseOut), XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	/* init and define  */
	uint16_t *pDenoiseIn = matDenoiseIn.Ptr<uint16_t>();
	uint16_t *pDenoiseOut = matDenoiseOut.Ptr<uint16_t>();
	int32_t nHeight = matDenoiseIn.hei;
	int32_t nWidth = matDenoiseOut.wid;
	float32_t spatialDecay = 0.55f;
	float32_t photometricStandardDeviation = 10.0f;

	for (int32_t k = 0; k < nHeight*nWidth; k++)
	{
		m_matDataImage.data.fl[k] = pDenoiseIn[k] / 255.0f;
	}

	BEEPSVerticalHorizontal(m_matDataImage.data.fl, nWidth, nHeight, photometricStandardDeviation, spatialDecay);

	for (int32_t k = 0; k < nWidth*nHeight; k++)
	{
		pDenoiseOut[k] = uint16_t(m_matDataImage.data.fl[k] * 255.0f);
	}

	return XRAY_LIB_OK;
}
// Progressive
void ImgProcModule::BEEPSProgressive(float32_t* data, int32_t startIndex, int32_t length, float32_t spatialContraDecay, float32_t photometricStandardDeviation)
{
	float32_t c;
	float32_t rho = 1.0f + spatialContraDecay;

	c = -0.5f / (photometricStandardDeviation * photometricStandardDeviation); // gauss

	float32_t mu = 0.0f;
	data[startIndex] /= rho;

	for (int32_t k = startIndex + 1, K = startIndex + length; (k < K); k++) // gauss
	{
		mu = data[k] - rho * data[k - 1];
		mu = spatialContraDecay * exp(c * mu * mu);
		data[k] = data[k - 1] * mu + data[k] * (1.0f - mu) / rho;
	}
}
// Gain
void ImgProcModule::BEEPSGain(float32_t* data, int32_t startIndex, int32_t length, float32_t spatialContraDecay)
{
	float32_t mu = (1.0f - spatialContraDecay) / (1.0f + spatialContraDecay);

	for (int32_t k = startIndex, K = startIndex + length; (k < K); k++)
	{
		data[k] *= mu;
	}
}
// Regressive
void ImgProcModule::BEEPSRegressive(float32_t* data, int32_t startIndex, int32_t length, float32_t spatialContraDecay, float32_t photometricStandardDeviation)
{
	float32_t c;
	float32_t rho = 1.0f + spatialContraDecay;

	c = -0.5f / (photometricStandardDeviation * photometricStandardDeviation); // gauss

	float32_t mu = 0.0f;
	data[startIndex + length - 1] /= rho;

	for (int32_t k = startIndex + length - 2; (startIndex <= k); k--) // gauss
	{
		mu = data[k] - rho * data[k + 1];
		mu = spatialContraDecay * exp(c * mu * mu);
		data[k] = data[k + 1] * mu + data[k] * (1.0f - mu) / rho;
	}
}
// Vertical + Horizontal
void ImgProcModule::BEEPSVerticalHorizontal(float32_t* data, int32_t width, int32_t height, float32_t photometricStandardDeviation, float32_t spatialDecay)
{
	int32_t m = 0;
	for (int32_t k2 = 0; (k2 < height); k2++)
	{
		int32_t n = k2;
		for (int32_t k1 = 0; (k1 < width); k1++)
		{
			m_matGImage1.data.fl[n] = data[m++];
			n += height;
		}
	}

	for (int32_t i = 0; i < height * width; i++)
	{
		m_matPImage1.data.fl[i] = m_matGImage1.data.fl[i];
		m_matRImage1.data.fl[i] = m_matGImage1.data.fl[i];
	}

	for (int32_t k1 = 0; (k1 < width); k1++)
	{
		BEEPSProgressive(m_matPImage1.data.fl, k1 * height, height, 1.0f - spatialDecay, photometricStandardDeviation);
		BEEPSGain(m_matGImage1.data.fl, k1 * height, height, 1.0f - spatialDecay);
		BEEPSRegressive(m_matRImage1.data.fl, k1 * height, height, 1.0f - spatialDecay, photometricStandardDeviation);
	}

	for (int32_t k = 0, K = height * width; (k < K); k++)
	{
		m_matRImage1.data.fl[k] += m_matPImage1.data.fl[k] - m_matGImage1.data.fl[k];
	}
	m = 0;
	for (int32_t k1 = 0; (k1 < width); k1++)
	{
		int32_t n = k1;
		for (int32_t k2 = 0; (k2 < height); k2++)
		{
			m_matGImage1.data.fl[n] = m_matRImage1.data.fl[m++];
			n += width;
		}
	}

	for (int32_t i = 0; i < height * width; i++)
	{
		m_matPImage1.data.fl[i] = m_matGImage1.data.fl[i];
		m_matRImage1.data.fl[i] = m_matGImage1.data.fl[i];
	}
	for (int32_t k2 = 0; (k2 < height); k2++)
	{
		BEEPSProgressive(m_matPImage1.data.fl, k2 * width, width, 1.0f - spatialDecay, photometricStandardDeviation);
		BEEPSGain(m_matGImage1.data.fl, k2 * width, width, 1.0f - spatialDecay);
		BEEPSRegressive(m_matRImage1.data.fl, k2 * width, width, 1.0f - spatialDecay, photometricStandardDeviation);
	}

	for (int32_t k = 0, K = width * height; (k < K); k++)
	{
		data[k] = m_matPImage1.data.fl[k] - m_matGImage1.data.fl[k] + m_matRImage1.data.fl[k];
	}
}
// Horizontal + Vertical
void ImgProcModule::BEEPSHorizontalVertical(float32_t* data, int32_t width, int32_t height, float32_t photometricStandardDeviation, float32_t spatialDecay)
{
	for (int32_t k = 0, K = width*height; (k < K); k++)
	{
		m_matGImage2.data.fl[k] = data[k];
	}

	for (int32_t i = 0; i < height * width; i++)
	{
		m_matPImage2.data.fl[i] = m_matGImage2.data.fl[i];
		m_matRImage2.data.fl[i] = m_matGImage2.data.fl[i];
	}

	for (int32_t k2 = 0; (k2 < height); k2++)
	{
		BEEPSProgressive(m_matPImage2.data.fl, k2 * width, width, 1.0f - spatialDecay, photometricStandardDeviation);
		BEEPSGain(m_matGImage2.data.fl, k2 * width, width, 1.0f - spatialDecay);
		BEEPSRegressive(m_matRImage2.data.fl, k2 * width, width, 1.0f - spatialDecay, photometricStandardDeviation);
	}

	for (int32_t k = 0, K = height*width; (k < K); k++)
	{
		m_matRImage2.data.fl[k] += m_matPImage2.data.fl[k] - m_matGImage2.data.fl[k];
	}
	int32_t m = 0;
	for (int32_t k2 = 0; (k2 < height); k2++)
	{
		int32_t n = k2;
		for (int32_t k1 = 0; (k1 < width); k1++)
		{
			m_matGImage2.data.fl[n] = m_matRImage2.data.fl[m++];
			n += height;
		}
	}

	for (int32_t i = 0; i < height * width; i++)
	{
		m_matPImage2.data.fl[i] = m_matGImage2.data.fl[i];
		m_matRImage2.data.fl[i] = m_matGImage2.data.fl[i];
	}
	for (int32_t k1 = 0; (k1 < width); k1++)
	{
		BEEPSProgressive(m_matPImage2.data.fl, k1 * height, height, 1.0f - spatialDecay, photometricStandardDeviation);
		BEEPSGain(m_matGImage2.data.fl, k1 * height, height, 1.0f - spatialDecay);
		BEEPSRegressive(m_matRImage2.data.fl, k1 * height, height, 1.0f - spatialDecay, photometricStandardDeviation);
	}

	for (int32_t k = 0, K = height*width; (k < K); k++)
	{
		m_matRImage2.data.fl[k] += m_matPImage2.data.fl[k] - m_matGImage2.data.fl[k];
	}
	m = 0;
	for (int32_t k1 = 0; (k1 < width); k1++)
	{
		int32_t n = k1;
		for (int32_t k2 = 0; (k2 < height); k2++)
		{
			data[n] = m_matRImage2.data.fl[m++];
			n += width;
		}
	}
}

/**
 * @fn      ImgEnhancing
 * @brief   图像默认增强
 * 
 * @param   [OUT] matEnhanceOut 默认增强后的融合灰度图，与matEnhanceIn内存不可共用
 * @param   [IN] matEnhanceIn 仅做高低能融合后的图像，注：其中的pad参数无效，使用入参中的HStart和HEnd 
 * @param   [IN] enhanceDir 默认增强方向，0-实时条带迭代增强，1-从上往下顺序增强，2-从下往上逆序增强
 * 
 * @return  XRAY_LIB_HRESULT 
 */
XRAY_LIB_HRESULT ImgProcModule::ImgEnhancing(XMat& matEnhanceOut, XMat& matEnhanceIn, int32_t enhanceDir)
{
	XRAY_LIB_HRESULT hr;
    auto _TimeCostPb = [&] (const std::string& functionName, const int32_t idx)
    {
        if (0 != enhanceDir) // 只记录回拉转存模式下的耗时
        {
            timeCostItemUpdate(functionName, idx);
        }
    };
    _TimeCostPb("EnhanceIn", 2);

    // 复用m_matCompositiveTemp、m_matGammaMask、m_matGammaMaskSmooth为临时内存
	hr = m_matCompositiveTemp.Reshape(matEnhanceIn.hei, matEnhanceIn.wid, matEnhanceIn.tpad, matEnhanceIn.bpad, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matCompositiveTemp reshape err.");
    hr = m_matGammaMask.Reshape(matEnhanceIn.hei, matEnhanceIn.wid, matEnhanceIn.tpad, matEnhanceIn.bpad, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matGammaMask reshape err.");
    hr = m_matGammaMaskSmooth.Reshape(matEnhanceIn.hei, matEnhanceIn.wid, matEnhanceIn.tpad, matEnhanceIn.bpad, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matGammaMaskSmooth reshape err.");
    if (0 != enhanceDir) // 回拉转存模式，cache与imgin属性保持一致
    {
        hr = m_matGammaMem.Reshape(matEnhanceIn.hei, matEnhanceIn.wid, matEnhanceIn.tpad, matEnhanceIn.bpad, XSP_16UC1);
        XSP_CHECK(XRAY_LIB_OK == hr, hr, "m_matGammaMem reshape err.");
    }

    isl::Imat<uint16_t> imgin(matEnhanceIn);
    isl::Imat<uint16_t> imgcache((0 == enhanceDir) ? m_matRtCache : m_matGammaMem);
    isl::Imat<uint16_t> imgtmp0(m_matCompositiveTemp), imgtmp1(m_matGammaMask), imgtmp2(m_matGammaMaskSmooth);
    isl::Imat<uint16_t> imgout(matEnhanceOut);
    std::array<isl::Imat<uint16_t>*, 3> imgtmp{&imgtmp0, &imgtmp1, &imgtmp2};

	hr = m_islPipeGray.AutoContraceEnhance(imgcache, imgin, imgtmp, static_cast<isl::PipeGray::pmode_t>(enhanceDir));
	XSP_CHECK(0 == hr, XRAY_LIB_XMAT_INVALID);
    _TimeCostPb("ACEOut", 3);

    hr = m_islPipeGray.AutoSharpnessEnhance(imgout, imgcache, imgtmp);
    XSP_CHECK(0 == hr, XRAY_LIB_XMAT_INVALID);
    _TimeCostPb("ASEOut", 4);

    if (0 == enhanceDir) // 实时条带模式，更新cache属性，下个条带会继续作为输入使用
	{
        hr = m_matRtCache.Reshape(imgcache.height(), imgcache.width(), imgcache.get_vpads().first, imgcache.get_vpads().second, XSP_16UC1);
        XSP_CHECK(XRAY_LIB_OK == hr, hr);
	}

    _TimeCostPb("EnhanceOut", 5);
    return XRAY_LIB_OK;
}


/**************************************************************************
*                               图像特殊增强
***************************************************************************/
XRAY_LIB_HRESULT ImgProcModule::ImgSpecialEnhance(XMat& matEnhanceIn, XMat& matHighCaliData, XMat& matLowCaliData, XMat& matEnhanceOut, int32_t nHStart, int32_t nHEnd)
{
	XRAY_LIB_HRESULT hr = XRAY_LIB_INVALID_PARAM;
	if (m_enEnhanceMode == XRAYLIB_ENHANCE_SUPER) //超级增强
	{
		hr = SuperEnhance(matEnhanceIn, matLowCaliData, matEnhanceOut, nHStart, nHEnd);
	}
	else if (m_enEnhanceMode == XRAYLIB_ENHANCE_EDGE1) //边缘增强
	{
		hr = EdgeEnhance(matEnhanceIn, matEnhanceOut, nHStart, nHEnd);
	}
	else if (m_enEnhanceMode == XRAYLIB_SPECIAL_LOCALENHANCE) //局部增强
	{
		hr = LocalEnhance(matEnhanceIn, matHighCaliData, matEnhanceOut, nHStart, nHEnd);
	}
	else
	{
		log_error("EnhanceMode %d Error.\n", m_enEnhanceMode);
	}

	return hr;
}

XRAY_LIB_HRESULT SmoothCPU2(float32_t* ptrImageIn, float32_t* ptrImageOut, int32_t nHeight, int32_t nWidth)
{
	int32_t h, w, nIndex, nNum, nTotal;
	int32_t nRoundPos[25] = {0};
	for (nIndex = 0; nIndex<25; nIndex++)
		nRoundPos[nIndex] = roundXYGetDualImg[nIndex][0] * nWidth + roundXYGetDualImg[nIndex][1];

	for (h = 2; h<nHeight - 2; h++)
	{
		for (w = 2; w<nWidth - 2; w++)
		{
			nTotal = 0;
			nNum = 0;

			for (nIndex = 0; nIndex < 25; nIndex++)
			{
				nTotal += ptrImageIn[h*nWidth + w + nRoundPos[nIndex]];
				nNum++;
			}

			if (nNum != 0)
			{
				ptrImageOut[h*nWidth + w] = nTotal / nNum;
			}
			else
			{
				ptrImageOut[h*nWidth + w] = 0;
			}

		}
	}

	return XRAY_LIB_OK;
}

/**@fn      EdgeEnhance_Sobel
* @brief    边缘增强 Sobel算子 BackScatter使用
* @param1   matEnhanceIn0      [I]     - 输入融合图像0
* @param2   matEnhanceIn1      [I]     - 输入融合图像1
* @param3   matEnhanceOut      [O]     - 输出增强后的图像
* @param4   uDirection		   [I]	   - 输入处理顺序(1==正序, 0==逆序)
* @return   错误码
* @note
*/
XRAY_LIB_HRESULT ImgProcModule::EdgeEnhance_Sobel(XMat &matEnhanceIn0, XMat &matEnhanceIn1, XMat &matEnhanceOut, uint8_t uDirection)
{
    /* check para */
    XSP_CHECK(matEnhanceIn0.IsValid() && matEnhanceIn1.IsValid() && matEnhanceOut.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");
    XSP_CHECK(XSP_16U == matEnhanceIn0.type && XSP_16U == matEnhanceIn1.type && XSP_16U == matEnhanceOut.type, XRAY_LIB_XMAT_TYPE_ERR);
    XSP_CHECK(MatSizeEq(matEnhanceIn0, matEnhanceOut), XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");
    XSP_CHECK(MatSizeEq(matEnhanceIn1, matEnhanceOut), XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

    uint16_t* originalptr = matEnhanceIn0.Ptr<uint16_t>();
    uint16_t* processedptr = matEnhanceOut.Ptr<uint16_t>();
    uint16_t* ptrImageIn2 = matEnhanceIn1.Ptr<uint16_t>();

    int32_t nHeight = matEnhanceIn0.hei;
    int32_t nWidth = matEnhanceIn0.wid;

    // 复制输入数据到输出，确保数据对应
    memcpy(processedptr, ptrImageIn2, sizeof(uint16_t) * nHeight * nWidth);

	if (0 == m_fDefaultEnhanceIntensity)
	{
		return XRAY_LIB_OK;
	}

    m_matImgTemph2Den.Reshape(nHeight, nWidth, XSP_32UC1);
    m_matImgTempw2Den.Reshape(nHeight, nWidth, XSP_32UC1);
    m_matImgTempw2Tmp.Reshape(nHeight, nWidth, XSP_32UC1);

    float32_t* m_prtImgTemph2Den = m_matImgTemph2Den.Ptr<float32_t>();
    float32_t* m_prtImgTempw2Den = m_matImgTempw2Den.Ptr<float32_t>();
    float32_t* m_prtImgTempw2Tmp = m_matImgTempw2Tmp.Ptr<float32_t>();

    // 初始化临时矩阵
    memset(m_prtImgTemph2Den, 0, sizeof(float32_t) * nHeight * nWidth);
    memset(m_prtImgTempw2Den, 0, sizeof(float32_t) * nHeight * nWidth);
    memset(m_prtImgTempw2Tmp, 0, sizeof(float32_t) * nHeight * nWidth);

    // Sobel卷积核
	// 正向Sobel算子
	const int32_t sobelh_forward[] = {-1, -2, -1, 0, 0, 0, 1, 2, 1};

	// 反向Sobel算子（直接置反）
	const int32_t sobelh_reverse[] = {1, 2, 1, 0, 0, 0, -1, -2, -1};
	

	int32_t sobelh[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    int32_t sobelw[] = {1, 0, -1, 2, 0, -2, 1, 0, -1};

	if (uDirection)
	{
		for (int32_t i = 0; i < 9; i++)
		{
			sobelh[i] = sobelh_forward[i];
		}
	}
	else
	{
		for (int32_t i = 0; i < 9; i++)
		{
			sobelh[i] = sobelh_reverse[i];
		}
	}

    float32_t prtImgTemph, prtImgTempw, prtImgTemph2, prtImgTempw2, prtImgTemp;

    // 对matEnhanceIn1进行Sobel边缘检测（用于NLM权重计算）
    for (int32_t nh = 1; nh < nHeight - 1; nh++) {
        for (int32_t nw = 1; nw < nWidth - 1; nw++) {
            // 正确的3x3卷积核索引计算
            int32_t idx = 0;
            float32_t h_sum = 0.0f;
            float32_t w_sum = 0.0f;
            
            for (int32_t i = -1; i <= 1; i++) {
                for (int32_t j = -1; j <= 1; j++) {
                    int32_t pixel_idx = (nh + i) * nWidth + (nw + j);
                    h_sum += sobelh[idx] * ptrImageIn2[pixel_idx];
                    w_sum += sobelw[idx] * ptrImageIn2[pixel_idx];
                    idx++;
                }
            }
            
            m_prtImgTemph2Den[nh * nWidth + nw] = h_sum;
            m_prtImgTempw2Den[nh * nWidth + nw] = w_sum;
        }
    }

    // 平滑处理
    memset(m_prtImgTempw2Tmp, 0, sizeof(float32_t) * nHeight * nWidth);
    SmoothCPU2(m_prtImgTemph2Den, m_prtImgTempw2Tmp, nHeight, nWidth);
    memcpy(m_prtImgTemph2Den, m_prtImgTempw2Tmp, sizeof(float32_t) * nWidth * nHeight);
    
    memset(m_prtImgTempw2Tmp, 0, sizeof(float32_t) * nHeight * nWidth);
    SmoothCPU2(m_prtImgTempw2Den, m_prtImgTempw2Tmp, nHeight, nWidth);
    memcpy(m_prtImgTempw2Den, m_prtImgTempw2Tmp, sizeof(float32_t) * nWidth * nHeight);

    // 对原始图像进行边缘增强处理
    for (int32_t nh = 1; nh < nHeight - 1; nh++) {
        for (int32_t nw = 1; nw < nWidth - 1; nw++) {
            // 对原始图像的Sobel边缘检测
            int32_t idx = 0;
            float32_t h_sum_orig = 0.0f;
            float32_t w_sum_orig = 0.0f;
            
            for (int32_t i = -1; i <= 1; i++) {
                for (int32_t j = -1; j <= 1; j++) {
                    int32_t pixel_idx = (nh + i) * nWidth + (nw + j);
                    h_sum_orig += sobelh[idx] * originalptr[pixel_idx];
                    w_sum_orig += sobelw[idx] * originalptr[pixel_idx];
                    idx++;
                }
            }
            
            prtImgTemph = h_sum_orig;
            prtImgTempw = w_sum_orig;
            prtImgTemph2 = m_prtImgTemph2Den[nh * nWidth + nw];
            prtImgTempw2 = m_prtImgTempw2Den[nh * nWidth + nw];
            
            // 边缘增强计算
            prtImgTemp = (float32_t)originalptr[nh * nWidth + nw] - 
                        (prtImgTemph + prtImgTempw) / m_edgeWeight_AI - 
                        (prtImgTemph2 + prtImgTempw2) / m_edgeWeight_NLM;
            
            prtImgTemp = Clamp(prtImgTemp, 0.0f, 65535.0f);
            
            // 确保输出位置与输入位置完全对应
            processedptr[nh * nWidth + nw] = (uint16_t)prtImgTemp;
		}
    }
    
    return XRAY_LIB_OK;
}

/**@fn      EdgeEnhance
* @brief    边缘增强
* @param1   matEnhanceIn      [I]     - 输入融合图像
* @param2   matEnhanceOut     [O]     - 输出增强后的图像
* @return   错误码
* @note
*/
XRAY_LIB_HRESULT ImgProcModule::EdgeEnhance(XMat& matEnhanceIn, XMat& matEnhanceOut, int32_t nHProcStart, int32_t nHProcEnd)
{
	/* check para */
	XSP_CHECK(matEnhanceIn.IsValid() && matEnhanceOut.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");
	XSP_CHECK(XSP_16U == matEnhanceIn.type && XSP_16U == matEnhanceOut.type, XRAY_LIB_XMAT_TYPE_ERR);
	XSP_CHECK(MatSizeEq(matEnhanceIn, matEnhanceOut), XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	/* init and define */
	int32_t nIndex, nh, nw;
	uint32_t minValue = 65535, maxValue = 0;
	float32_t expIndex, dif, curGray, fResultGray, medium = 0, nNum;
	int32_t nWidth = matEnhanceIn.wid;
	/* 默认增强强度系数 */
	float32_t fScale = m_fEdgeEnhanceIntensity / 10.0f + 2.0f;
	float32_t fExpIndexW = 1.5f * fScale;
	float32_t fExpIndexB = 2.0f * fScale;

	#pragma omp parallel for schedule(static, 8)
	for (nh = nHProcStart; nh < nHProcEnd; nh++)
	{
		uint16_t *src = matEnhanceIn.Ptr<uint16_t>(nh);
		uint16_t *dst = matEnhanceOut.Ptr<uint16_t>(nh);
		for (nw = 0; nw < nWidth; nw++)
		{
			if ((*src) < m_pSharedPara->m_nBackGroundThreshold)        //对背景像素不进行变换
			{
				medium = 0;
				minValue = 65535, maxValue = 0;

				nNum = 0;
				for (nIndex = 0; nIndex < 9; nIndex++)
				{
					int32_t posY = nh + roundXYGetDualImg[nIndex][0];
					int32_t posX = nw + roundXYGetDualImg[nIndex][1];
					medium += *(matEnhanceIn.NeighborPtr<uint16_t>(posY, posX));
					minValue = MIN(minValue, *(matEnhanceIn.NeighborPtr<uint16_t>(posY, posX)));
					maxValue = MAX(maxValue, *(matEnhanceIn.NeighborPtr<uint16_t>(posY, posX)));
				}
				nNum = 9;

				if (4000 >= maxValue - minValue)
				{
					for (nIndex = 9; nIndex < 25; nIndex++)
					{
						int32_t posY = nh + roundXYGetDualImg[nIndex][0];
						int32_t posX = nw + roundXYGetDualImg[nIndex][1];
						medium += *(matEnhanceIn.NeighborPtr<uint16_t>(posY, posX));
						nNum++;
					} 
					nNum = 25;
				}
				medium /= (float32_t)nNum;

				if ((*src) - medium >= 0)
					expIndex = fExpIndexW;            //亮细节增强程度
				else
					expIndex = fExpIndexB;           //暗细节增强程度

				if ((*src) > 40000)      //灰度阈值，灰度高于一定的阈值，则不进行细节增强，只稍微进行边缘增强
				{
					dif = (float32_t)(*src) - medium;
					dif = std::min(dif, 1.0f);
					fResultGray = (float32_t)((*src) + ((*src) - medium)*fScale);
				}
				else
				{
					curGray = (float32_t)(*src) / 65535.0f;
					dif = curGray - (float32_t)medium / 65535.0f;
					if (fabs(dif)*curGray*curGray*curGray < 0.002)
					{
						dif *= 2;
						dif = std::min(dif, 1.0f);
						if (dif >= 0)
							fResultGray = (*src) + expIndex * (float32_t)cos(XRAY_LIB_PI*0.5f*dif) * 65535 * 0.001f + ((*src) - medium) * fScale;
						else
							fResultGray = (*src) - expIndex * (float32_t)cos(XRAY_LIB_PI*0.5f*dif) * 65535 * 0.001f + ((*src) - medium) * fScale;
					}
					else
					{
						dif *= 2;
						dif = std::min(dif, 1.0f);
						if (dif >= 0)
							fResultGray = (*src) + expIndex * (float32_t)cos(XRAY_LIB_PI*0.5f*dif) * 65535 * 0.01f + ((*src) - medium) * fScale;
						else
							fResultGray = (*src) - expIndex * (float32_t)cos(XRAY_LIB_PI*0.5f*dif) * 65535 * 0.01f + ((*src) - medium) * fScale;
					}
				}
			}
			else
			{
				fResultGray = (float32_t)(*src);
			}
			fResultGray = Clamp(fResultGray, 0.0f, 65535.0f);
			*dst = (uint16_t)fResultGray;

			src++;
			dst++;
		}
	}

	return XRAY_LIB_OK;
}

/**@fn      SuperEnhance
* @brief    CLAHE+局部增强
* @param1   matEnhanceIn      [I]     - 输入融合图像
* @param2   matEnhanceOut     [O]     - 输出增强后的图像
* @return   错误码
* @note
*/
XRAY_LIB_HRESULT ImgProcModule::SuperEnhance(XMat & matEnhanceIn, XMat & matCaliLowIn, XMat & matEnhanceOut, int32_t nHProcStart, int32_t nHProcEnd)
{
	/* check para */
	XSP_CHECK(matEnhanceIn.IsValid() && matEnhanceOut.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");
	XSP_CHECK(XSP_16U == matEnhanceIn.type && XSP_16U == matEnhanceOut.type, XRAY_LIB_XMAT_TYPE_ERR);
	XSP_CHECK(MatSizeEq(matEnhanceIn, matEnhanceOut), XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	/* init and define */
	uint16_t *src = matEnhanceIn.Ptr<uint16_t>();
	uint16_t *srclow = matCaliLowIn.Ptr<uint16_t>();
	uint16_t *dst = matEnhanceOut.Ptr<uint16_t>();
	uint16_t nResult;
	int32_t nHeight = matEnhanceIn.hei;
	int32_t nWidth = matEnhanceIn.wid;
	int32_t nPad = matEnhanceIn.tpad;
	
	/*条带高度方向数据补充*/
	int32_t nHeightTemp = nHeight;
	if (nHeight < m_nKernelSizeY)
	{
		nHeightTemp = 2 * m_nKernelSizeY;
	}
	else if (nHeight % m_nKernelSizeY != 0 || nHeight == m_nKernelSizeY)
	{
		nHeightTemp = (nHeight / m_nKernelSizeY + 1) * m_nKernelSizeY;
	}

	int32_t nLeftPad = (nHeightTemp - nHeight) / 2;
	
	/*探测器方向数据补充，nCount区分奇偶，避免竖条纹*/
	int32_t nWidthTemp;
	int32_t nCount = nWidth / m_nKernelSizeX;	//	探测器方向必须是64的整数倍
	if (0 == nCount % 2 && 0 == nWidth % m_nKernelSizeX)
	{
		nWidthTemp = nWidth;
	}
	else
	{
		if (0 == nCount % 2)
		{
			nWidthTemp = (nCount + 2) * m_nKernelSizeX;
		}
		else
		{
			nWidthTemp = (nCount + 1) * m_nKernelSizeX;
		}
	}
	XRAY_LIB_HRESULT hr;

	hr = m_matImageTemp1.Reshape(nHeightTemp, nWidthTemp, nPad, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	// 初始化为65535
	uint16_t *pTemp1 = m_matImageTemp1.Ptr<uint16_t>();
	memset(pTemp1, 65535, sizeof(uint16_t) * nHeightTemp * nWidthTemp);	

	/* Width(Detector)
	 * ^		原数据时间轴方向左右镜像补邻域, 减弱边缘过渡不自然的现象
	 * |		--------- —————————————— ---------
	 * | 		.\\\\\\\\|//////////////|\\\\\\\\.
	 * | 		.\\\\\\\\|//////////////|\\\\\\\\.
	 * | 		.\\\\\\\\|//////////////|\\\\\\\\.
	 * | 		.\\\\\\\\|//////////////|\\\\\\\\.
	 * | 		.\Neighb\|/  True Data /|\Neighb\.
	 * | 		.\\\\\\\\|//////////////|\\\\\\\\.
	 * | 		.\\\\\\\\|//////////////|\\\\\\\\.
	 * | 		.\\\\\\\\|//////////////|\\\\\\\\.
	 * | 		.\\\\\\\\|//////////////|\\\\\\\\.
	 * | 		.\\\\\\\\|//////////////|\\\\\\\\.
	 * |		--------- —————————————— ---------
	 * ——————————————————————————————————————————>Height(time)
	*/
	
	// 先补时间轴方向左邻域
	for (int32_t i = 0; i < nLeftPad; i++)
	{
		memcpy(pTemp1 + i * nWidthTemp, matCaliLowIn.NeighborPtr<uint16_t>(i-nLeftPad-1, 0), XSP_ELEM_SIZE(XSP_16UC1) * nWidth);
	}

	// 拷贝原数据和右邻域
	for (int32_t i = 0; i < nHeightTemp - nLeftPad; i++)
	{
		memcpy(pTemp1 + (nLeftPad + i) * nWidthTemp, matCaliLowIn.NeighborPtr<uint16_t>(i, 0), XSP_ELEM_SIZE(XSP_16UC1) * nWidth);
	}

	/*超增*/
	hr = ClaheEnhance(m_matImageTemp1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	for (int32_t nh = nHProcStart; nh < nHProcEnd; nh++)
	{
		int32_t nIdx = nh*nWidth;
		int32_t nIdxTemp = (nh + nLeftPad)*nWidthTemp;	// 跳过左邻域
		for (int32_t nw = 0; nw < nWidth; nw++)
		{
			if (m_nCaliLowGrayThreshold < src[nIdx + nw])
			{
				nResult = (uint16_t)(0.65*srclow[nIdx + nw] + 0.35*pTemp1[nIdxTemp + nw]);
			}
			else
			{
				nResult = (uint16_t)srclow[nIdx + nw];
			}
			dst[nIdx + nw] = (uint16_t)Clamp(nResult, (uint16_t)0, (uint16_t)65535);//数据类型转换
		}
	}
	return XRAY_LIB_OK;
}

/**@fn      LocalEnhance
* @brief    局部增强
* @param1   matEnhanceIn      [I]     - 输入融合图像
* @param2   matEnhanceIn      [I]     - 输入高能图像
* @param3   matEnhanceOut     [O]     - 输出增强后的图像
* @return   错误码
* @note		局部增强的邻域为8方向各1 实际上为3*3矩阵
*			
*			a4	a5	a6
*			a3	a0	a7
*			a2	a1	a8
*			为保证a0数据正常, 需保证a1-a8为有效数据, 因此对应边界方式采用镜像取值的方式, 保证均值平滑正常即可
*/
XRAY_LIB_HRESULT ImgProcModule::LocalEnhance(
	XMat& matEnhanceIn, XMat& matHighData, XMat& matEnhanceOut,
    int32_t nHProcStart, int32_t nHProcEnd)
{
    XSP_CHECK(matEnhanceIn.IsValid() && matEnhanceOut.IsValid(), XRAY_LIB_XMAT_INVALID);
    XSP_CHECK(matEnhanceIn.type == XSP_16U && matEnhanceOut.type == XSP_16U, XRAY_LIB_XMAT_TYPE_ERR);
    XSP_CHECK(MatSizeEq(matEnhanceIn, matEnhanceOut), XRAY_LIB_XMAT_SIZE_ERR);

    const int32_t H = matEnhanceIn.hei;
    const int32_t W = matEnhanceIn.wid;

    XRAY_LIB_HRESULT hr = m_matWeightTemp.Reshape(H, W, matEnhanceIn.tpad, XSP_32FC1);  // 权重图，羽化关键 
    XSP_CHECK(hr == XRAY_LIB_OK, hr);

    const uint16_t TH_LOW  = m_nLocalEnhanceThresholdDOWN;
    const uint16_t TH_HIGH = m_nLocalEnhanceThresholdUP;

    // ==================== 统计几何均值和最大值 ====================
    float64_t logSum = 0.0;
    int64_t pixelCount = 0;
    uint16_t maxVal = 0;

    for (int32_t y = nHProcStart; y < nHProcEnd; ++y)
    {
        const uint16_t* pSrc = matHighData.Ptr<uint16_t>(y);

        for (int32_t x = 0; x < W; ++x)
        {
            if (pSrc[x] > TH_LOW && pSrc[x] < TH_HIGH)
            {
                uint16_t v = pSrc[x];
                logSum += log(v + 1.0);
                if (v > maxVal) maxVal = v;
                ++pixelCount;
            }
        }
    }

    if (pixelCount == 0)
    {
		memcpy(matEnhanceOut.Ptr<uint16_t>(), matEnhanceIn.Ptr<uint16_t>(), H * W * sizeof(uint16_t));
        return XRAY_LIB_OK;
    }
    float64_t geoMean = exp(logSum / pixelCount);

    // ==================== 生成羽化权重====================
	const int32_t R = 5;
	const int32_t limitVaildPixNum = 3;
	float32_t* pWeight = m_matWeightTemp.Ptr<float32_t>();
	
	for (int32_t y = nHProcStart; y < nHProcEnd; ++y)
	{
		float32_t* pw = pWeight + y * W;
		const uint16_t* pCenterRow = matHighData.Ptr<uint16_t>(y);
	
		for (int32_t x = 0; x < W; ++x)
		{
			if (pCenterRow[x] <= TH_LOW || pCenterRow[x] >= TH_HIGH)
			{
				pw[x] = 0.0f;
				continue;
			}
	
			// 计算到最近无效像素的距离
			float32_t minDistanceToInvalid = R * 2.0f;
			int32_t nVaildPixNum = 0;
			for (int32_t dy = -R; dy <= R; ++dy)
			{
				int32_t ny = y + dy;
				if (ny < 0 || ny >= H) continue;
				const uint16_t* row = matHighData.Ptr<uint16_t>(ny);
	
				for (int32_t dx = -R; dx <= R; ++dx)
				{
					int32_t nx = x + dx;
					if (nx < 0 || nx >= W) continue;
	
					// 这里只对大于阈值的像素进行统计，避免低灰度区域内部出现白斑
					if (row[nx] >= TH_HIGH && row[nx] <= TH_LOW)
					{
						nVaildPixNum++;
						float32_t distance = sqrt(dx*dx + dy*dy);
						if (distance < minDistanceToInvalid)
						{
							minDistanceToInvalid = distance;
						}
					}
				}
			}
	
			// 快速羽化过渡的权重计算
			if ((minDistanceToInvalid >= R * 2.0f) || (nVaildPixNum < limitVaildPixNum))
			{
				pw[x] = 1.0f;
			}
			else
			{
				float32_t normalizedDistance = minDistanceToInvalid / (R * 1.5f);
				normalizedDistance = std::max(0.0f, std::min(1.0f, normalizedDistance));
				
				pw[x] = normalizedDistance;  // 线性过渡
			}
		}
	}	

    // ==================== 增强 + 羽化融合 ====================
    const float32_t TARGET_MAX = 65535.0f * 0.7f; // 这个值需要根据颜色表的颜色分界线进行限定，当前策略是尽量保持原色相
    const float64_t denom = log((float64_t)maxVal / geoMean + 1.0);

    for (int32_t y = nHProcStart; y < nHProcEnd; ++y)
    {
        const uint16_t* pHighData = matHighData.Ptr<uint16_t>(y);
        const uint16_t* pIn = matEnhanceIn.Ptr<uint16_t>(y);
        const float32_t* pW  = m_matWeightTemp.Ptr<float32_t>(y);
		uint16_t* pOut = matEnhanceOut.Ptr<uint16_t>(y);
		
        for (int32_t x = 0; x < W; ++x)
        {
            float32_t w = pW[x];
            if (w > 0.01f)  // 轻微阈值避免浮点噪声
            {
                float64_t enhanced = log((float64_t)pHighData[x] / geoMean + 1.0) / denom * TARGET_MAX;
                enhanced = std::max(0.0, std::min(enhanced, 65535.0));
                pOut[x] = static_cast<uint16_t>(pIn[x] * (1.0f - w) + enhanced * w + 0.5f);
            }
            else
            {
                pOut[x] = pIn[x];
            }
        }
    }

    return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT ImgProcModule::ClaheEnhance(XMat & matEnhanceIn)
{
	XSP_CHECK(matEnhanceIn.wid > m_nKernelSizeX && matEnhanceIn.hei > m_nKernelSizeY, XRAY_LIB_INVALID_PARAM);
	XSP_CHECK(0 == (matEnhanceIn.wid % m_nKernelSizeX) && 0 == (matEnhanceIn.hei % m_nKernelSizeY)
		&& 2 < m_nKernelSizeX && 2 < m_nKernelSizeY, XRAY_LIB_INVALID_PARAM);
	XSP_CHECK(fabs(m_fCliplimit - 1.0f) > 1E-6, XRAY_LIB_INVALID_PARAM);

	if (0 == m_nNrBins)
	{
		m_nNrBins = 128;
	}

	int32_t nHeight = matEnhanceIn.hei;
	int32_t nWidth = matEnhanceIn.wid;
	int32_t nPad = matEnhanceIn.tpad;

	uint16_t* pImgIn = matEnhanceIn.Ptr<uint16_t>();

	XRAY_LIB_HRESULT hr;
	hr = m_matImageTemp2.Reshape(nHeight, nWidth, nPad, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	uint16_t* pImgTemp = m_matImageTemp2.Ptr<uint16_t>();
	memcpy(pImgTemp, pImgIn, nHeight * nWidth * sizeof(uint16_t));

	uint32_t nXSize, nYSize, nNrPixels, uClipLimit;
	nXSize = nWidth / m_nKernelSizeX;
	nYSize = nHeight / m_nKernelSizeY;
	nNrPixels = nXSize * nYSize;

	if (0.0f < m_fCliplimit)
	{
		uClipLimit = (uint32_t)(m_fCliplimit * nNrPixels);
		uClipLimit = MAX(uClipLimit, 1);
	}
	else
	{
		uClipLimit = 1 << 14;
	}

	uint16_t* pnHist;
	hr = m_matImageTemp3.Reshape(m_nKernelSizeX * m_nKernelSizeY * m_nNrBins, 1, XSP_16UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	uint16_t* pnMapArray = m_matImageTemp3.Ptr<uint16_t>();
	memset(pnMapArray, 0, m_nKernelSizeX * m_nKernelSizeY * m_nNrBins * sizeof(uint16_t));
	for (int32_t nh = 0; nh < m_nKernelSizeY; nh++)
	{
		for (int32_t nw = 0; nw < m_nKernelSizeX; nw++, pImgIn += nXSize)
		{
			pnHist = &pnMapArray[m_nNrBins * (nh * m_nKernelSizeX + nw)];
			MakeHistogram(pImgIn, nWidth, nXSize, nYSize, pnHist);
			ClipHistogram(pnHist, uClipLimit);
			MapHistogram(pnHist, 0, 65535, nNrPixels);
		}
		pImgIn += (nYSize - 1) * nWidth;
	}
	pImgIn = matEnhanceIn.Ptr<uint16_t>();
	uint32_t nSubX, nSubY, nXL, nXR, nYU, nYB;
	uint16_t* pnLU, *pnRU, *pnLB, *pnRB;
	for (int32_t nh = 0; nh <= m_nKernelSizeY; nh++)
	{
		if (0 == nh)
		{
			nSubY = nYSize >> 1;
			nYU = 0;
			nYB = 0;
		}
		else
		{
			if (nh == m_nKernelSizeY)
			{
				nSubY = nYSize >> 1;
				nYU = m_nKernelSizeY - 1;
				nYB = nYU;
			}
			else
			{
				nSubY = nYSize;
				nYU = nh - 1;
				nYB = nYU + 1;
			}
		}
		for (int32_t nw = 0; nw <= m_nKernelSizeX; nw++)
		{
			if (0 == nw)
			{
				nSubX = nXSize >> 1;
				nXL = 0;
				nXR = 0;
			}
			else
			{
				if (nw == m_nKernelSizeX)
				{
					nSubX = nXSize >> 1;
					nXL = m_nKernelSizeX - 1;
					nXR = nXL;
				}
				else
				{
					nSubX = nXSize;
					nXL = nw - 1;
					nXR = nXL + 1;
				}
			}

			pnLU = &pnMapArray[m_nNrBins * (nYU * m_nKernelSizeX + nXL)];
			pnRU = &pnMapArray[m_nNrBins * (nYU * m_nKernelSizeX + nXR)];
			pnLB = &pnMapArray[m_nNrBins * (nYB * m_nKernelSizeX + nXL)];
			pnRB = &pnMapArray[m_nNrBins * (nYB * m_nKernelSizeX + nXR)];
			Interpolate(pImgIn, nWidth, pnLU, pnRU, pnLB, pnRB, nSubX, nSubY);
			pImgIn += nSubX;
		}
		pImgIn += (nSubY - 1) * nWidth;
	}
	pImgIn = matEnhanceIn.Ptr<uint16_t>();
	for (int32_t i = 0; i < nHeight * nWidth; i++)
	{
		if (61500 < *pImgTemp)
		{
			*pImgIn = *pImgTemp;
		}
		if (XRAYLIB_TESTMODE_CN_CHECK == m_enTestMode)
		{
			*pImgIn = *pImgIn * m_fTest5Ratio + *pImgTemp * (1 - m_fTest5Ratio);
		}
		pImgIn++;
		pImgTemp++;
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT ImgProcModule::MakeLut(uint16_t * pLUT, int32_t nMin, int32_t nMax)
{
	const uint16_t nBinSize = (uint16_t)(1 + (nMax - nMin) / m_nNrBins);

	uint16_t* pIn = pLUT;
	for (int32_t i = nMin; i <= nMax; i++)
	{
		*pIn = (uint16_t)((i - nMin) / nBinSize);
		pIn++;
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT ImgProcModule::MakeHistogram(uint16_t * pImage, int32_t nWidth, int32_t nSubWidth, int32_t nSubHeight, uint16_t * Hist)
{
	uint16_t* pImagePointer;
	// uint16_t* pHistIn = Hist;
	//for (int32_t i = 0; i < m_nNrBins; i++)
	//{
	//	*pHistIn = 0;
	//	pHistIn++;
	//}

	for (int32_t nh = 0; nh < nSubHeight; nh++)
	{
		pImagePointer = pImage + nSubWidth;
		while (pImage < pImagePointer)
		{
			Hist[m_pnALUT[*pImage]]++;
			pImage++;
		}
		pImage += nWidth - nSubWidth;
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT ImgProcModule::ClipHistogram(uint16_t * pHist, int32_t nClipLimit)
{
	uint16_t* ptrHistIn = pHist;
	uint16_t* ptrHisto, *ptrEndPointer, *ptrBinPointer;
	int32_t nBinExcess, nBinIncr, nUpper, nOldNrExcess, nStepSize;
	int32_t nNrExcess = 0;

	for (int32_t i = 0; i < m_nNrBins; i++)
	{
		nBinExcess = (int32_t)(*ptrHistIn - nClipLimit);
		if (0 < nBinExcess)
		{
			nNrExcess += nBinExcess;
		}
		ptrHistIn++;
	}
	nBinIncr = nNrExcess / m_nNrBins;
	nUpper = nClipLimit - nBinIncr;

	ptrHistIn = pHist;

	for (int32_t i = 0; i < m_nNrBins; i++)
	{
		if (*ptrHistIn > nClipLimit)
		{
			*ptrHistIn = nClipLimit;
		}
		else
		{
			if (*ptrHistIn > nUpper)
			{
				nNrExcess -= nClipLimit - *ptrHistIn;
				*ptrHistIn = nClipLimit;
			}
			else
			{
				nNrExcess -= nBinIncr;
				*ptrHistIn += nBinIncr;
			}
		}
		ptrHistIn++;
	}

	do {
		ptrEndPointer = pHist + m_nNrBins;
		ptrHisto = pHist;

		nOldNrExcess = nNrExcess;

		while (nNrExcess && ptrHisto < ptrEndPointer)
		{
			nStepSize = m_nNrBins / nNrExcess;
			if (1 > nStepSize)
			{
				nStepSize = 1;
			}

			for (ptrBinPointer = ptrHisto; ptrBinPointer < ptrEndPointer && nNrExcess; ptrBinPointer += nStepSize)
			{
				if (*ptrBinPointer < nClipLimit)
				{
					(*ptrBinPointer)++;
					nNrExcess--;
				}
			}
			ptrHisto++;       /* restart redistributing on other bin location */
		}
	} while ((nNrExcess) && (nNrExcess < nOldNrExcess));
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT ImgProcModule::MapHistogram(uint16_t * pHist, int32_t nPixelMin, int32_t nPixelMax, int32_t nNrOfPixel)
{
	int32_t nSum = 0;
	float32_t fScale = ((float32_t)(nPixelMax - nPixelMin)) / nNrOfPixel;
	uint16_t* ptrHistIn = pHist;
	for (int32_t i = 0; i < m_nNrBins; i++)
	{
		nSum += *ptrHistIn;
		*ptrHistIn = (uint16_t)(nPixelMin + nSum * fScale);
		if (*ptrHistIn > nPixelMax)
		{
			*ptrHistIn = nPixelMax;
		}
		ptrHistIn++;
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT ImgProcModule::Interpolate(uint16_t * pImage, int32_t nXRes, uint16_t * pMapLU, uint16_t * pMapRU, uint16_t * pMapLB, uint16_t * pMapRB, int32_t nXSize, int32_t nYSize)
{
	int32_t nIncr = nXRes - nXSize;
	int32_t nNum = nXSize * nYSize;
	int32_t nGrayValue, nXCoef, nYCoef, nXInvCoef, nYInvCoef, nShift = 0;

	if (nNum & (nNum - 1))
	{
		for (nYCoef = 0, nYInvCoef = nYSize; nYCoef < nYSize; nYCoef++, nYInvCoef--, pImage += nIncr)
		{
			for (nXCoef = 0, nXInvCoef = nXSize; nXCoef < nXSize; nXCoef++, nXInvCoef--)
			{
				nGrayValue = m_pnALUT[*pImage];
				*pImage = (uint16_t)((nYInvCoef * (nXInvCoef * pMapLU[nGrayValue] + nXCoef * pMapRU[nGrayValue])
					+ nYCoef * (nXInvCoef * pMapLB[nGrayValue] + nXCoef * pMapRB[nGrayValue])) / nNum);
				pImage++;
			}
		}
	}
	else
	{
		while (nNum >>= 1)
		{
			nShift++;
		}
		for (nYCoef = 0, nYInvCoef = nYSize; nYCoef < nYSize; nYCoef++, nYInvCoef--, pImage += nIncr)
		{
			for (nXCoef = 0, nXInvCoef = nXSize; nXCoef < nXSize; nXCoef++, nXInvCoef--)
			{
				nGrayValue = m_pnALUT[*pImage];
				*pImage = (uint16_t)((nYInvCoef * (nXInvCoef * pMapLU[nGrayValue] + nXCoef * pMapRU[nGrayValue])
					+ nYCoef * (nXInvCoef * pMapLB[nGrayValue] + nXCoef * pMapRB[nGrayValue])) >> nShift);
				pImage++;
			}
		}
	}

	return XRAY_LIB_OK;
}


bool ImgProcModule::AirDetec(XMat& matEnhanceIn)
{
	int32_t nRealHei = matEnhanceIn.hei - 2 * matEnhanceIn.tpad;
	/**
	* nNum:小于背景阈值行计数
	* nNumR:小于包裹阈值行计数
	* nLowGrayHei:一行小于背景阈值像素数
	*/
	int32_t nNum, nNumR, nLowGrayHei;
	/**
	* dTotal 条带包裹灰度均值
	* dMeanH 条带行灰度均值
	* dSubMean 小于背景阈值像素灰度均值
	*/
	float64_t dTotal, dMeanH, dSubMean;

	int32_t nwSkip = 15;
	dTotal = 0; nNum = 0; nNumR = 0;
	dSubMean = 0;

	float64_t dTotalForce = 0;
	int32_t nNumForce = 0;

	/* 不考虑探测器上下边界的nwSkip行数据 */
	for (int32_t nw = nwSkip; nw < matEnhanceIn.wid - nwSkip; nw++)
	{
		dMeanH = 0;
		dSubMean = 0;
		nLowGrayHei = 0;
		uint16_t* pIn = matEnhanceIn.Ptr<uint16_t>(0, nw);
		/* 先对nh方向做平均，减少噪声影响.*/
		for (int32_t nh = matEnhanceIn.tpad; nh < matEnhanceIn.hei - matEnhanceIn.tpad; nh++)
		{
			dMeanH += *pIn;
			dTotalForce += *pIn;		
			nNumForce += 1;
			if (*pIn < m_pSharedPara->m_nBackGroundThreshold)
			{
				nLowGrayHei++;
				dSubMean += *pIn;
			}
			pIn += matEnhanceIn.wid;
		}
		/**
		* 检测到当前行小于阈值灰度比例大于0.1，
		* 认为该行是包裹，防止包裹边缘被切
		*/
		if (float32_t(nLowGrayHei) / nRealHei > 0.1)
		{
			dMeanH = dSubMean / nLowGrayHei;
		}
		else
		{
			dMeanH /= nRealHei;
		}
		/* 等于0或者大于背景值的点不参与计算 */
		if (dMeanH > m_pSharedPara->m_nBackGroundThreshold || dMeanH == 0)
		{
			continue;
		}
		else
		{
			dTotal += dMeanH;
			nNum++;
			/* 包裹第二阈值判断 */
			if (dMeanH < m_pSharedPara->m_nPackageThresholdR)
				nNumR++;
		}
		
	} // for nw

	//packpos->stForcedSeg.avgGrayLevel = (uint32_t)(dTotalForce / nNumForce);


	/* 条带均值计算 */
	if (nNum == 0)
		dTotal = 0;
	else
		dTotal /= nNum;

	/* 判断包裹 */
	bool bPackage = false;
	// bool bPackageR = false;

	///* 包裹第二阈值判断 */
	//if (nNumR > int32_t(10 * m_fWidthResizeFactor))
	//	bPackageR = true;

	/**
	* 计数+均值同时统计的方式，
	* 统计小于m_nBackGroundThreshold以下所有点，求其点数及均值。如下情况认为有物体：
	* 1,均值（60000-bkValue）点数>24;
	* 2,均值（55000-60000）点数>18;
	* 3,均值（50000-55000）点数>12;
	* 4,均值（30000-50000）点数>10;
	* 5,均值（0 - 30000）点数>5;
	*/
	if (dTotal < m_pSharedPara->m_nBackGroundThreshold && dTotal >= 60000)
	{
		if (nNum > 24)
			bPackage = true;
		else
			bPackage = false;
	}
	else if (dTotal < 60000 && dTotal >= 55000)
	{
		if (nNum > 18)
			bPackage = true;
		else
			bPackage = false;
	}
	else if (dTotal < 55000 && dTotal >= 50000)
	{
		if (nNum > 12)
			bPackage = true;
		else
			bPackage = false;
	}
	else if (dTotal < 50000 && dTotal >= 30000)
	{
		if (nNum > 10)
			bPackage = true;
		else
			bPackage = false;
	}
	else if (dTotal<30000)
	{
		if (nNum > 5)
			bPackage = true;
		else
			bPackage = false;
	}

	return bPackage;
}

void ImgProcModule::timeCostItemAdd(const std::string& functionName, const int32_t s32Width, const int32_t s32Height)
{
	while(m_dq_tc_stats.size() >= 20)
	{
		m_dq_tc_stats.pop_front();
	}

	ENTIRE_IMAGE_TIME_COST_LIST stImageTc;
	
	stImageTc.s32IndexNum = this->m_nTimeCostCallTime++;
	stImageTc.s32Width = s32Width;
	stImageTc.s32Height = s32Height;

	auto now = std::chrono::system_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

	int64_t timeStamps = duration.count();

	stImageTc.TimeCostItems.TimeCostStamps[0] = std::make_pair(functionName, timeStamps); 
	m_dq_tc_stats.push_back(stImageTc);
	return;
}

void ImgProcModule::timeCostItemUpdate(const std::string& functionName, const int32_t idx)
{
	ENTIRE_IMAGE_TIME_COST_LIST stImageTc = m_dq_tc_stats.back(); 

	auto now = std::chrono::system_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());

	int64_t timeStamps = duration.count();
	stImageTc.TimeCostItems.TimeCostStamps[idx] = std::make_pair(functionName, timeStamps);

	m_dq_tc_stats.pop_back();

	m_dq_tc_stats.emplace_back(stImageTc);
	return;
}

void ImgProcModule::timeCostShow() const
{
	if (m_dq_tc_stats.empty())
	{
		log_info("No Data Avaliable.");
		return;
	}

	std::cout << "Statement:\n"
          << "  R     = Raw Data\n"
          << "  M     = Merge (Gray Normalization)\n"
          << "  Enh   = Enhance (Standard Process)\n"
		  << "  ACE   = Contrast Enhance Process\n"
		  << "  ASE   = Edge Enhance Process\n"
          << "  Clr   = Colorization\n"
          << "  SpEnh = SpecialEnhance\n"
          << "  Alg   = Algorithm Module\n"
          << "  R2M   = RawToMerge Transformation\n"
          << "  M2Clr = MergeToColor Conversion"
          << std::endl;
	std::cout<<std::endl;

	std::cout << std::setw(10) << std::left << "Index" 
			  << std::setw(10) << std::left << "Width" 
			  << std::setw(10) << std::left << "Height" 
			  << std::setw(10) << std::left << "AlgIn" 
              << std::setw(10) << std::left << "R2MIn"
              << std::setw(10) << std::left << "EnhIn"
              << std::setw(10) << std::left << "ACEOut"
              << std::setw(10) << std::left << "ASEOut"
              << std::setw(10) << std::left << "EnhOut"
              << std::setw(10) << std::left << "R2MOut"
              << std::setw(10) << std::left << "M2ClrIn"
              << std::setw(10) << std::left << "SpEnhOut"
              << std::setw(10) << std::left << "ClrOut"
              << std::setw(10) << std::left << "M2ClrOut"
              << std::setw(10) << std::left << "AlgOut" << std::endl;

    // 输出数据
	for (const auto& entry : m_dq_tc_stats) 
	{
        std:: cout << std::setw(10) << std::left << entry.s32IndexNum 
				   << std::setw(10) << std::left << entry.s32Width 
				   << std::setw(10) << std::left << entry.s32Height;
		for (int32_t i = 0; i < 12; i++) 
		{
            std::cout << std::setw(10) << std::left << (entry.TimeCostItems.TimeCostStamps[i].second) % 10000000;
        }
		std::cout << std::endl;
    }

	return;
}

