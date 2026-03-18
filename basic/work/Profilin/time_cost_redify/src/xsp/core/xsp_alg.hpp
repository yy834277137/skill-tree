/**
*      @file xsp_alg.hpp
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*
*      @brief XSP通用图像处理或其他算法集合，放在该模块实现的所有算法
*             不需要使用临时内存来支持实现
*
*      @author wangtianshu
*      @date 2023/4/10
*
*      @note
*/

#ifndef _XSP_ALG_HPP_
#define _XSP_ALG_HPP_

#include "xmat.hpp"
#include "xsp_def.hpp"
#include "xsp_check.hpp"
#include <math.h>
#include <vector>

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

/***********************************************
*                 最大最小截取
************************************************/
template<typename Dtype> inline
Dtype Clamp(Dtype val, Dtype min, Dtype max)
{
	return MIN(MAX(val, min), max);
}

/***********************************************
*                   图像缩放
************************************************/
XRAY_LIB_HRESULT ResizeBilinear(XMat& matIn, XMat& matOut);

XRAY_LIB_HRESULT ResizeNearst(XMat& matIn, XMat& matOut);

// @brief out = clamp(resize(in) * scale, fMin, fMax)
XRAY_LIB_HRESULT ResizeBilinearScale(XMat& matIn, XMat& matOut, float fScale, float fMin, float fMax);

// @brief out = clamp(resize(in) * scale, fMin, fMax)
XRAY_LIB_HRESULT ResizeNearstScale(XMat& matIn, XMat& matOut, float fScale, float fMin, float fMax);

// 图像格式转换
template<typename DtypeIn, typename DtypeOut>
XRAY_LIB_HRESULT ImgConvertType(DtypeIn* src, DtypeOut* dst, int32_t pixsize, float fScale, float fMin, float fMax)
{
	DtypeOut* pDst = dst;
	DtypeIn* pSrc = src;
	
	for (int32_t idx = 0; idx < pixsize; idx++)
	{
		float val = float(*(pSrc++)) * fScale;
		val = Clamp(val, fMin, fMax);
		*(pDst++) = (DtypeOut)(val);
	}

	return XRAY_LIB_OK;
}

/***********************************************
*               ARGB彩色图像旋转
************************************************/
// @brief ARGB向左滚屏（ARGB数据左转90度）
XRAY_LIB_HRESULT MoveLeftArgb(XMat& matArgbIn, XMat& matArgbOut);

// @brief ARGB向右滚屏（ARGB数据右转90度）
XRAY_LIB_HRESULT MoveRightArgb(XMat& matArgbIn, XMat& matArgbOut);

// @brief ARGB向左滚屏加上下镜像
XRAY_LIB_HRESULT MoveLeftMirrorUpDownArgb(XMat& matArgbIn, XMat& matArgbOut);

// @brief ARGB向右滚屏加上下镜像
XRAY_LIB_HRESULT MoveRightMirrorUpDownArgb(XMat& matArgbIn, XMat& matArgbOut);

/***********************************************
*               RGB彩色图像旋转
************************************************/
// @brief RGB向左滚屏（RGB数据左转90度）
XRAY_LIB_HRESULT MoveLeftRgb(XMat& matRgbIn, XMat& matRgbOut);

// @brief RGB向右滚屏（RGB数据右转90度）
XRAY_LIB_HRESULT MoveRightRgb(XMat& matRgbIn, XMat& matRgbOut);

// @brief RGB向左滚屏加上下镜像
XRAY_LIB_HRESULT MoveLeftMirrorUpDownRgb(XMat& matRgbIn, XMat& matRgbOut);

// @brief RGB向右滚屏加上下镜像
XRAY_LIB_HRESULT MoveRightMirrorUpDownRgb(XMat& matRgbIn, XMat& matRgbOut);

/***********************************************
*               YUV彩色图像旋转
************************************************/
// @brief YUV向左滚屏（YUV数据左转90度）
XRAY_LIB_HRESULT MoveLeftYuv(XMat& matYIn, XMat& matUVIn,
	                         XMat& matYOut, XMat& matUVOut);

// @brief YUV向右滚屏（YUV数据右转90度）
XRAY_LIB_HRESULT MoveRightYuv(XMat& matYIn, XMat& matUVIn,
	                          XMat& matYOut, XMat& matUVOut);

// @brief YUV向左滚屏加上下镜像
XRAY_LIB_HRESULT MoveLeftMirrorUpDownYuv(XMat& matYIn, XMat& matUVIn,
	                                     XMat& matYOut, XMat& matUVOut);

// @brief YUV向右滚屏加上下镜像
XRAY_LIB_HRESULT MoveRightMirrorUpDownYuv(XMat& matYIn, XMat& matUVIn,
	                                      XMat& matYOut, XMat& matUVOut);


/***********************************************
*                 降噪
************************************************/

// 目前背散使用
XRAY_LIB_HRESULT NLMeans(XMat& matIn, XMat& matOut, std::vector<XMat>& matsProc);

// 目前背散使用
XRAY_LIB_HRESULT MirrorUpDownBS_u16(XMat& matData, int nWidthInBs);

/***********************************************
*                 浮点型等于判断
************************************************/
inline
bool FpEqual(float val1, float val2)
{
	return fabs(val1 - val2) < 1e-6;
}

#endif // _XSP_ALG_HPP_
