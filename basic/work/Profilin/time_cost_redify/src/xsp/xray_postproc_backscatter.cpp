#include "xray_image_process.hpp"
#include "xray_image_process.hpp"

// #define XRAY_LIB_BS_PROC_FILTER_RADIUS  3     ///背散射增强滤波核半径大小

XRAY_LIB_HRESULT XRayImageProcess::DualDenoiseToEnhance(XRAYLIB_IMAGE& calilhz, // 输入参数，归一化高低能+原子序数
                                               XRAYLIB_IMAGE& merge, // 输出参数，channel为HIGHLOW时，默认增强后的融合灰度图
	                                           int32_t ntop, int32_t nbotm,	// 上下邻域 
											   int32_t bDescendOrder) // 是否和预览方向相反
{
	/* DSP转存通道merge与calilhz尺寸不保证相同，检查merge内存足够使用即可 */
	XSP_CHECK(calilhz.height * calilhz.width <= merge.height * merge.width,
		      XRAY_LIB_XMAT_SIZE_ERR, "caliData hei %d, wid %d, merge hei %d, wid %d", 
		      calilhz.height, calilhz.width, merge.height, merge.width);
    
    // XSP_CHECK(ntop >= XRAY_LIB_BS_PROC_FILTER_RADIUS || nbotm >= XRAY_LIB_BS_PROC_FILTER_RADIUS,
    //             XRAY_LIB_INVALID_PARAM, "TopNeighbor %d, BottomNeighbor %d", ntop, nbotm);

	XRAY_LIB_HRESULT hr;
	XMat matNLMeans, matAiXsp, matEnhance;

	hr = matNLMeans.Init(calilhz.height, calilhz.width, ntop, nbotm, XSP_16UC1, (uint8_t*)calilhz.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	hr = matAiXsp.Init(calilhz.height, calilhz.width, ntop, nbotm, XSP_16UC1, (uint8_t*)calilhz.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

    hr = matEnhance.Init(calilhz.height, calilhz.width, ntop, nbotm, XSP_16UC1, (uint8_t*)merge.pData[0]);
    XSP_CHECK(XRAY_LIB_OK == hr, hr);

    hr = m_modImgproc.EdgeEnhance_Sobel(matAiXsp, matNLMeans, matEnhance, bDescendOrder);
    XSP_CHECK(XRAY_LIB_OK == hr, hr);

    // 所有处理成功后，更新输出的融合灰度图参数
    merge.height = calilhz.height;
	merge.width = calilhz.width;

	return XRAY_LIB_OK;
}