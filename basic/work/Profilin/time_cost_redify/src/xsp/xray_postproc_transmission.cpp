#include "xray_image_process.hpp"

/**
 * @fn      Cali2GrayEnhance
 * @brief   校正后的高低能图像转默认增强后的融合灰度图 
 * @note    如果有TIP，插入TIP的流程应在此之前
 * 
 * @param   [OUT] matGray 默认增强后的融合灰度图，输出到matGray指定的Buffer中
 * @param   [IN] matLow 校正后的低能数据
 * @param   [IN] matHigh 校正后的高能数据，单能时为nullptr
 * @param   [IN] matWt 处理权重图
 * @param   [IN] enhanceDir 默认增强方向，0-非法，即不做增强，1-从上往下顺序增强，2-从下往上逆序增强
 * 
 * @return  XRAY_LIB_HRESULT 
 */
XRAY_LIB_HRESULT XRayImageProcess::Cali2GrayEnhance(XMat& matGray, XMat& matLow, XMat* matHigh, [[maybe_unused]]XMat& matWt, int32_t enhanceDir)
{
	XRAY_LIB_HRESULT hr;

	m_modImgproc.timeCostItemUpdate("Raw2MergeIn", 1);

    /// 高低能融合，输出至临时内存m_matEntireMerge
    hr = m_matEntireMerge.Reshape(matLow.hei, matLow.wid, matLow.tpad, matLow.bpad, XSP_16UC1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr);

    hr = m_modImgproc.ImgMerging(m_matEntireMerge, matLow, *matHigh, matWt);
    XSP_CHECK(XRAY_LIB_OK == hr, hr);

    /// 默认增强，输出至matGray
    hr = m_modImgproc.ImgEnhancing(matGray, m_matEntireMerge, enhanceDir);
    XSP_CHECK(XRAY_LIB_OK == hr, hr);

	m_modImgproc.timeCostItemUpdate("Raw2MergeOut", 6);

	return XRAY_LIB_OK;
}


/**
 * @fn      GraySpecialEnhance
 * @brief   融合灰度图的特殊增强（边增、超增、局增）
 * 
 * @param   [IN] matLow 校正后的低能数据
 * @param   [IN] matHigh 校正后的高能数据，单能时为nullptr
 * @param   [IN] matGray 融合灰度图
 * 
 * @return  XMat 特殊增强后的融合灰度图，出现异常时会直接输出matGray，其他情况会返回一个全局对象
 */
XMat XRayImageProcess::GraySpecialEnhance(XMat& matLow, XMat* matHigh, XMat& matGray)
{
	XRAY_LIB_HRESULT hr;

    if (m_modImgproc.m_enEnhanceMode == XRAYLIB_ENHANCE_NORAML)
    {
        return matGray;
    }

	m_modImgproc.timeCostItemUpdate("Merge2ColorIn", 7);

    /******************** 计算实际处理的宽高与所需邻域 ********************/

	uint32_t u32TPad = matLow.tpad, u32BPad = matLow.bpad, u32Wid = matLow.wid, u32Hei = matLow.hei;

	hr = m_matEntireEnhance.Reshape(u32Hei, u32Wid, u32TPad, u32BPad, matLow.type);
	XSP_CHECK(XRAY_LIB_OK == hr, matGray);

	/* 特殊增强 */
	hr = m_modImgproc.ImgSpecialEnhance(matGray, *matHigh, matLow, m_matEntireEnhance, 0, m_matEntireEnhance.hei);
	XSP_CHECK(XRAY_LIB_OK == hr, matGray);

	m_modImgproc.timeCostItemUpdate("SpEnhanceOut", 8);

	return m_matEntireEnhance;
}