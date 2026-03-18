/**
 *      @file xsp_mod_imgproc.hpp
 *	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
 *
 *      @brief XSP图像处理模块
 *
 *      @note
 */

#ifndef _XSP_MOD_IMGPROC_HPP_
#define _XSP_MOD_IMGPROC_HPP_

#include "xsp_interface.hpp"
#include "xray_shared_para.hpp"
#include "core_def.hpp"
#include "xmat.hpp"
#include "xsp_alg.hpp"
#include "isl_pipe.hpp"
#include "isl_fbg.hpp"
#include "isl_filter.hpp"

class ImgProcModule : public BaseModule
{
public:
	ImgProcModule();
	~ImgProcModule();
	/**@fn      Release
	 * @brief    释放接口
	 * @return   错误码
	 * @note
	 */
	virtual XRAY_LIB_HRESULT Release();

	/**@fn      GetMemSize
	 * @brief    获取内存表所需内存大小(字节单位)
	 * @param1   MemTab             [O]     - 内存表
	 * @param2   nDetectorWidth     [I]     - 探测器宽度
	 * @return   错误码
	 * @note     算法库所需内存由外部申请, 需提前计算所需内存大小
	 */
	virtual XRAY_LIB_HRESULT GetMemSize(XRAY_LIB_MEM_TAB &MemTab, XRAY_LIB_ABILITY &ability);

	/**@fn      Init
	 * @brief    图像处理模块初始化
	 * @param1   pPara                   [I]     - 公共参数
	 * @return   错误码
	 * @note
	 */
	XRAY_LIB_HRESULT Init(SharedPara *pPara, XRAY_LIB_ABILITY &ability);

public:
    /**
     * @fn      ImgMerging
     * @brief   低能+高能灰度图融合为单一灰度图
     * 
     * @param   [OUT] matMerge 融合灰度图
     * @param   [IN] matLow 归一化后的低能图
     * @param   [IN] matHigh 归一化后的高能图
     * @param   [IN] matWt 处理权重图
     * 
     * @return  XRAY_LIB_HRESULT 错误码
     */
	XRAY_LIB_HRESULT ImgMerging(XMat& matMerge, XMat& matLow, XMat& matHigh, XMat& matWt);

	/**@fn      AI路图像融合
	 * @brief    AI路图像融合方式
	 * @param1   matMergeIn             [I]       - 图像输入
	 * @param2   matMergeOut            [I]       - 图像输出
	 * @return   错误码
	 * @note     该方式与Disp路区分，保证AI检测路图像不变
	 */
	XRAY_LIB_HRESULT ImgMergingAI(XMat &matHighCaliData, XMat &matLowCaliData, XMat &matMergeOut);

	/**@fn      图像去噪
	 * @brief    包含多种图像去噪方式
	 * @param1   matDenoiseIn           [I]       - 图像输入
	 * @param2   matDenoiseOut          [I]       - 图像输出
	 * @return   错误码
	 * @note
	 */
	XRAY_LIB_HRESULT ImgDenoisingLow(XMat &matDenoiseIn, XMat &matDenoiseOut);
	XRAY_LIB_HRESULT ImgDenoisingHigh(XMat &matDenoiseIn, XMat &matDenoiseOut);

	/**@fn      图像默认增强
	 * @brief    包含多种图像增强方式
	 * @param1   matEnhanceIn           [I]       - 图像输入
	 * @param2   matEnhanceOut          [I]       - 图像输出
	 * @return   错误码
	 * @note
	 */
	XRAY_LIB_HRESULT ImgEnhancing(XMat &matEnhanceOut, XMat &matEnhanceIn, int32_t enhanceDir);

	/**@fn      图像特殊增强
	 * @brief    包含边缘增强、局部增强、超级增强
	 * @param1   matEnhanceIn           [I]       - 图像输入
	 * @param2   matEnhanceOut          [I]       - 图像输出
	 * @return   错误码
	 * @note
	 */
	XRAY_LIB_HRESULT ImgSpecialEnhance(XMat &matEnhanceIn, XMat &matHighCaliData, XMat &matLowCaliData, XMat &matEnhanceOut, int32_t nHStart, int32_t nHEnd);

	/**@fn      AirDetec
	 * @brief    包裹检测，特殊增强时不对空气条带操作
	 * @param1   matEnhanceIn           [I/O]     - 图像输入/输出
	 * @return   错误码
	 * @note
	 */
	bool AirDetec(XMat &matEnhanceIn);

	XRAY_LIB_HRESULT EdgeEnhance_Sobel(XMat &matEnhanceIn0, XMat &matEnhanceIn1, XMat &matEnhanceOut, uint8_t uDirection);

	void timeCostItemAdd(const std::string &functionName, const int32_t s32Width, const int32_t s32Height);

	void timeCostItemUpdate(const std::string &functionName, const int32_t idx);

	void timeCostShow() const;

	/**************************************************************************
	 *                               外部使用参数
	 ***************************************************************************/
	// 默认增强参数
	XRAYLIB_DEFAULT_ENHANCE m_enDefaultEnhance;
	XRAYLIB_DENOISING_INTENSITY m_enDenoisingIntensity;
	XRAYLIB_FUSION_MODE m_enFusionMode;

	uint16_t m_nEnhanceGrayUp;   // 增强灰度上限
	uint16_t m_nEnhanceGrayDown; // 增强灰度下限

	float32_t m_fDefaultEnhanceIntensity; // 默认增强系数
	float32_t m_fEdgeEnhanceIntensity;	  // 边增系数
	float32_t m_fEdgeSuperIntensity;	  // 超增系数
	int32_t m_nMergeBaseLine;			  // 融合图灰度偏移值
	int32_t m_nLumIntensity;			  // ISL模块亮度调整系数
	int32_t m_nContrastRatio;			  // ISL模块对比度调整系数
	int32_t m_nSharpnessRatio;			  // ISL模块锐化系数
	int32_t m_nLowLumCompensation;		  // ISL模块难穿区补偿系数
	int32_t m_nHighLumSensity;			  // ISL模块易穿区敏感度

	// 局部增强参数
	int32_t m_nLocalEnhanceThresholdUP;	// 局部增强阈值上限
	int32_t m_nLocalEnhanceThresholdDOWN; // 局部增强阈值下限
	int32_t m_nCaliLowGrayThreshold;		// 局增时对小于该值做低灰度不一致性校正，整体归一化到m_nCaliLowGrayThreshold值上
	int32_t m_nWindowCenter;				// 局增窗宽中心
	bool m_isLowGrayContiue;			// 低灰度连续增强标志位
	int32_t m_LastStripeMaxValue;			// 上一个条带的局增区域灰度最大值
	float32_t m_LastStripeMeanValue;		// 上一个条带的局增区域灰度均值

	isl::PipeGray          m_islPipeGray;
    isl::Fbg               m_islFbg;
	isl::Filter  		   m_islFilter;
	XRAYLIB_ENHANCE_MODE   m_enEnhanceMode;

	XRAYLIB_DENOISING_MODE m_enDenoisingMode;
	XRAYLIB_GAMMA_MODE m_enGammaMode;
	XRAY_TESTMODE m_enTestMode; // 测试体类型，用于区分局增、超增使用算法
	float32_t m_fTest5Ratio;		// test5 增强比例

	// 低穿透区域不一致校正表实时更新比例
	float32_t m_fRTUpdateLowGrayRatio;

	XRAY_LIB_ONOFF m_enTestAutoLE; // 测试体自动局增开关

	// 背散射边缘增强算法的权重系数
	float32_t m_edgeWeight_AI;
	float32_t m_edgeWeight_NLM;

protected:

	/**************************************************************************
	 *                               图像去噪
	 ***************************************************************************/
	void BEEPSProgressive(float32_t *data, int32_t startIndex, int32_t length, float32_t spatialContraDecay, float32_t photometricStandardDeviation);
	void BEEPSGain(float32_t *data, int32_t startIndex, int32_t length, float32_t spatialContraDecay);
	void BEEPSRegressive(float32_t *data, int32_t startIndex, int32_t length, float32_t spatialContraDecay, float32_t photometricStandardDeviation);
	void BEEPSVerticalHorizontal(float32_t *data, int32_t width, int32_t height, float32_t photometricStandardDeviation, float32_t spatialDecay);
	void BEEPSHorizontalVertical(float32_t *data, int32_t width, int32_t height, float32_t photometricStandardDeviation, float32_t spatialDecay);

	/**************************************************************************
	 *                               图像特殊增强
	 ***************************************************************************/
	XRAY_LIB_HRESULT EdgeEnhance(XMat &matEnhanceIn, XMat &matEnhanceOut, int32_t nHProcStart, int32_t nHProcEnd);
	XRAY_LIB_HRESULT SuperEnhance(XMat &matEnhanceIn, XMat &matCaliLowIn, XMat &matEnhanceOut, int32_t nHProcStart, int32_t nHProcEnd);
	XRAY_LIB_HRESULT LocalEnhance(XMat &matEnhanceIn, XMat &matHighData, XMat &matEnhanceOut, int32_t nHProcStart, int32_t nHProcEnd);
	XRAY_LIB_HRESULT ClaheEnhance(XMat &matEnhanceIn);
	XRAY_LIB_HRESULT MakeLut(uint16_t *pLUT, int32_t nMin, int32_t nMax);
	XRAY_LIB_HRESULT MakeHistogram(uint16_t *pImage, int32_t nWidth, int32_t nSubWidth, int32_t nSubHeight, uint16_t *Hist);
	XRAY_LIB_HRESULT ClipHistogram(uint16_t *pHist, int32_t nClipLimit);
	XRAY_LIB_HRESULT MapHistogram(uint16_t *pHist, int32_t nPixelMin, int32_t nPixelMax, int32_t nNrOfPixel);
	XRAY_LIB_HRESULT Interpolate(uint16_t *pImage, int32_t nXRes, uint16_t *pMapLU, uint16_t *pMapRU, uint16_t *pMapLB, uint16_t *pMapRB,
								 int32_t nXSize, int32_t nYSize);

private:
	/**************************************************************************
	 *                               共享参数
	 ***************************************************************************/
	SharedPara *m_pSharedPara;

	/**************************************************************************
	 *                               临时内存
	 ***************************************************************************/
	// 去噪
	XMat m_matDuplicateImage;
	XMat m_matDataImage;
	XMat m_matGImage1;
	XMat m_matPImage1;
	XMat m_matRImage1;
	XMat m_matGImage2;
	XMat m_matPImage2;
	XMat m_matRImage2;

	// Sobel边缘增强使用
	XMat m_matImgTemph2Den;
	XMat m_matImgTempw2Den;
	XMat m_matImgTempw2Tmp;

	// 局部增强
	XMat m_matWeightTemp;

	// 超级增强
	XMat m_matImageTemp1;
	XMat m_matImageTemp2;
	XMat m_matImageTemp3;
	uint16_t *m_pnALUT;

	// 对比度增强
	XMat m_matCompositiveTemp;
	XMat m_matGammaMem;
	XMat m_matGammaMask;
	XMat m_matGammaMaskSmooth;
	XMat m_matRtCache; // 实时条带Continued模式下用于缓存输出的融合灰度图

	/**************************************************************************
	 *                               配置参数
	 ***************************************************************************/
	// 默认增强参数
	int32_t m_nRoundPos[25];

	// CLAHE默认参数
	int32_t m_nNrBins;
	int32_t m_nKernelSizeX;
	int32_t m_nKernelSizeY;
	float32_t m_fCliplimit;

	int32_t m_nTimeCostCallTime;
	typedef struct
	{
		std::pair<std::string, int64_t> TimeCostStamps[12];
	} ENTIRE_IMAGE_TIME_COST_ITEM;

	typedef struct
	{
		int32_t s32IndexNum;
		int32_t s32Width;
		int32_t s32Height;
		ENTIRE_IMAGE_TIME_COST_ITEM TimeCostItems;
	} ENTIRE_IMAGE_TIME_COST_LIST;
	std::deque<ENTIRE_IMAGE_TIME_COST_LIST> m_dq_tc_stats;

	// 探测器信号，用于区分民航数据，民航走SuperEnhance_CA  5030走SpecialEnhance_CA
	XRAY_DETECTORNAME m_enDetectorName;
}; // ImgProcModule

#endif // _XSP_MOD_IMGPROC_HPP_
