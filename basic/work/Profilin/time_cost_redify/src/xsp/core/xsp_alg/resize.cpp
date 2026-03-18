#include "xsp_alg.hpp"
#include "core_def.hpp"
#include <math.h>

/***************************************************************************
*                               ResizeBilinear
****************************************************************************/

/**@fn      ResizeBilinear_uint16_c
* @brief    uint16双线性插值c版本
* @param1   src                [I]     - 输入数据
* @param2   dst                [O]     - 输出数据
* @param3   heiIn              [I]     - 输入高度
* @param4   widIn              [I]     - 输入宽度
* @param5   heiOut             [I]     - 输出高度
* @param6   widOut             [I]     - 输出宽度
* @return   void
* @note     
*/
static void ResizeBilinear_uint16_c(uint16_t* src, uint16_t* dst, int heiIn, int widIn, int heiOut, int widOut)
{
	float fScaleHei = float(heiOut) / float(heiIn);
	float fScaleWid = float(widOut) / float(widIn);

	float Temp_nh, Temp_nw;
	int Round_nh_l, Round_nw_l, Round_nh_r, Round_nw_r;
	int nSkip_l, nSkip_r;
	uint32_t nU, nV, nInvU, nInvV;
	int nResizeCoefBit = 8; // 浮点数的缩放尺度 uint16最大为2^8位
	int nResizeScale = 1 << nResizeCoefBit;

	for (int nh = 0; nh < heiOut; ++nh)
	{
		Temp_nh = (float)((nh + 0.5) / fScaleHei - 0.5);
		Temp_nh = Clamp(Temp_nh, 0.0f, heiIn - 1.0f);
		Round_nh_l = int(Temp_nh);
		Round_nh_r = int(ceil(Temp_nh));
		nU = uint32_t((Temp_nh - Round_nh_l) * nResizeScale);
		nInvU = nResizeScale - nU;

		nSkip_l = Round_nh_l * widIn;
		nSkip_r = Round_nh_r * widIn;

		uint16_t* dst_cur = dst + nh * widOut;

		for (int nw = 0; nw < widOut; ++nw)
		{
			Temp_nw = (float)((nw + 0.5) / fScaleWid - 0.5);
			Temp_nw = Clamp(Temp_nw, 0.0f, widIn - 1.0f);
			Round_nw_l = int(Temp_nw);
			Round_nw_r = int(ceil(Temp_nw));
			nV = uint32_t((Temp_nw - Round_nw_l) * nResizeScale);
			nInvV = nResizeScale - nV;

			uint32_t val0 = (uint32_t)src[nSkip_l + Round_nw_l] * nInvV + (uint32_t)src[nSkip_l + Round_nw_r] * nV;
			uint32_t val1 = (uint32_t)src[nSkip_r + Round_nw_l] * nInvV + (uint32_t)src[nSkip_r + Round_nw_r] * nV;

			*(dst_cur++) = (uint16_t)((val0 * nInvU + val1 * nU) >> 16);
		}
	}
	return;
}
#ifdef COMPILE_NENO
/**@fn      ResizeBilinear_uint16_2x2_neon
* @brief    uint16双线性插值2倍缩放特例neon加速版本
* @param1   src                [I]     - 输入数据
* @param2   dst                [O]     - 输出数据
* @param3   heiIn              [I]     - 输入高度
* @param4   widIn              [I]     - 输入宽度
* @param5   heiOut             [I]     - 输出高度
* @param6   widOut             [I]     - 输出宽度
* @return   void
* @note
*/
static void ResizeBilinear_uint16_2x2_neon(uint16_t* src, uint16_t* dst, int heiIn, int widIn, int heiOut, int widOut)
{
	float fScaleHei = float(heiOut) / float(heiIn);
	float fScaleWid = float(widOut) / float(widIn);

	float Temp_nh, Temp_nw;
	int Round_nh_l, Round_nw_l, Round_nh_r, Round_nw_r;
	int nSkip_l, nSkip_r;
	uint32_t nU, nV, nInvU, nInvV;
	int nResizeCoefBit = 8;
	int nResizeScale = 1 << nResizeCoefBit;

	uint32x4_t vec_u0, vec_u1, vec_v;
	uint32x4_t vec_val0, vec_val1;
	uint32x2_t vec_val_a, vec_val_b;
	uint32_t coef_u_even[4] = { 64, 64, 64, 64 };
	uint32_t coef_u_odd[4]  = { 192, 192, 192, 192 };
	uint32_t coef_v[4] = { 192, 64, 64, 192 };
	vec_v = vld1q_u32(coef_v);

	/* along height process  nw = 0 or nw = wid - 1 */
	for (int nh = 0; nh < heiOut; ++nh)
	{
		Temp_nh = (float)((nh + 0.5) / fScaleHei - 0.5);
		Temp_nh = Clamp(Temp_nh, 0.0f, heiIn - 1.0f);
		Round_nh_l = int(Temp_nh);
		Round_nh_r = int(ceil(Temp_nh));
		nU = uint32_t((Temp_nh - Round_nh_l) * nResizeScale);
		nInvU = nResizeScale - nU;

		nSkip_l = Round_nh_l * widIn;
		nSkip_r = Round_nh_r * widIn;

		uint16_t* dst_cur = dst + nh * widOut;

		uint32_t val0 = (uint32_t)src[nSkip_l] * nInvU + (uint32_t)src[nSkip_r] * nU;
		uint32_t val1 = (uint32_t)src[nSkip_l + widIn - 1] * nInvU + (uint32_t)src[nSkip_r + widIn - 1] * nU;

		*(dst_cur) = (uint16_t)(val0 >> 8);
		*(dst_cur + widOut - 1) = (uint16_t)(val1 >> 8);
	}

	/* along width process nh = 0 or nh = hei - 1 */
	uint16_t* dst1 = dst;
	uint16_t* dst2 = dst + widOut * (heiOut - 1);
	int nSkip = widIn * (heiIn - 1);
	for (int nw = 0; nw < widOut; ++nw)
	{
		Temp_nw = (float)((nw + 0.5) / fScaleWid - 0.5);
		Temp_nw = Clamp(Temp_nw, 0.0f, widIn - 1.0f);
		Round_nw_l = int(Temp_nw);
		Round_nw_r = int(ceil(Temp_nw));
		nV = uint32_t((Temp_nw - Round_nw_l) * nResizeScale);
		nInvV = nResizeScale - nV;

		uint32_t val0 = (uint32_t)src[Round_nw_l] * nInvV + (uint32_t)src[Round_nw_r] * nV;
		uint32_t val1 = (uint32_t)src[Round_nw_l + nSkip] * nInvV + (uint32_t)src[Round_nw_r + nSkip] * nV;

		dst1[nw] = (uint16_t)(val0 >> 8);
		dst2[nw] = (uint16_t)(val1 >> 8);
	}

	/* process neon */
	for (int nh = 1; nh < heiOut - 1; ++nh)
	{
		Temp_nh = (float)((nh + 0.5) / fScaleHei - 0.5);
		Round_nh_l = int(Temp_nh);
		Round_nh_r = Round_nh_l + 1;

		nSkip_l = Round_nh_l * widIn;
		nSkip_r = Round_nh_r * widIn;

		if (nh % 2 == 0)
		{
			vec_u0 = vld1q_u32(coef_u_even);
			vec_u1 = vld1q_u32(coef_u_odd);
		}
		else
		{
			vec_u0 = vld1q_u32(coef_u_odd);
			vec_u1 = vld1q_u32(coef_u_even);
		}

		uint16_t* dst_cur = dst + nh * widOut + 1;

		for (int nw = 1; nw < widOut - 1; nw+=2)
		{
			Temp_nw = (float)((nw + 0.5) / fScaleWid - 0.5);
			Round_nw_l = int(Temp_nw);
			Round_nw_r = Round_nw_l + 1;

			uint32_t val_0[4] = { src[nSkip_l + Round_nw_l], src[nSkip_l + Round_nw_l], src[nSkip_l + Round_nw_r], src[nSkip_l + Round_nw_r] };
			uint32_t val_1[4] = { src[nSkip_r + Round_nw_l], src[nSkip_r + Round_nw_l], src[nSkip_r + Round_nw_r], src[nSkip_r + Round_nw_r] };

			/*
			* uint32_t val0 = (uint32_t)src[nSkip_l + Round_nw_l] * nInvV + (uint32_t)src[nSkip_l + Round_nw_r] * nV;
			* uint32_t val1 = (uint32_t)src[nSkip_r + Round_nw_l] * nInvV + (uint32_t)src[nSkip_r + Round_nw_r] * nV;
			*
			* (dst_cur++) = (uint16_t)((val0 * nInvU + val1 * nU) >> 16);
			*/

			vec_val0 = vld1q_u32(val_0);
			vec_val1 = vld1q_u32(val_1);

			vec_val0 = vmulq_u32(vec_val0, vec_v);
			vec_val1 = vmulq_u32(vec_val1, vec_v);

			vec_val0 = vmulq_u32(vec_val0, vec_u0);
			vec_val1 = vmulq_u32(vec_val1, vec_u1);

			vec_val0 = vaddq_u32(vec_val0, vec_val1);

			vec_val_a = vget_high_u32(vec_val0);
			vec_val_b = vget_low_u32(vec_val0);

			vec_val_a = vadd_u32(vec_val_a, vec_val_b);

			*(dst_cur++) = (uint16_t)(vget_lane_u32(vec_val_a, 0) >> 16);
			*(dst_cur++) = (uint16_t)(vget_lane_u32(vec_val_a, 1) >> 16);
		}
	}

	return;
}
#endif

/**@fn      ResizeBilinear_uint16
* @brief    uint16双线性插值各平台加速版本对外接口
* @param1   src                [I]     - 输入数据
* @param2   dst                [O]     - 输出数据
* @param3   heiIn              [I]     - 输入高度
* @param4   widIn              [I]     - 输入宽度
* @param5   heiOut             [I]     - 输出高度
* @param6   widOut             [I]     - 输出宽度
* @return   void
* @note
*/
static void ResizeBilinear_uint16(uint16_t* src, uint16_t* dst, int heiIn, int widIn, int heiOut, int widOut)
{
	if (FpEqual(float(heiOut) / heiIn, 2.0f) && FpEqual(float(widOut) / widIn, 2.0f))
	{
		XSP_NENO_RUN(ResizeBilinear_uint16_2x2_neon(src, dst, heiIn, widIn, heiOut, widOut));
	}

	ResizeBilinear_uint16_c(src, dst, heiIn, widIn, heiOut, widOut);
	return;
}

/**@fn      ResizeBilinear_uint8_c
* @brief    uint8双线性插值c版本
* @param1   src                [I]     - 输入数据
* @param2   dst                [O]     - 输出数据
* @param3   heiIn              [I]     - 输入高度
* @param4   widIn              [I]     - 输入宽度
* @param5   heiOut             [I]     - 输出高度
* @param6   widOut             [I]     - 输出宽度
* @return   void
* @note
*/
static void ResizeBilinear_uint8_c(uint8_t* src, uint8_t* dst, int heiIn, int widIn, int heiOut, int widOut)
{
	float fScaleHei = float(heiOut) / float(heiIn);
	float fScaleWid = float(widOut) / float(widIn);

	float Temp_nh, Temp_nw;
	int Round_nh_l, Round_nw_l, Round_nh_r, Round_nw_r;
	int nSkip_l, nSkip_r;
	uint32_t nU, nV, nInvU, nInvV;
	int nResizeCoefBit = 8;
	int nResizeScale = 1 << nResizeCoefBit;

	uint8_t* dst_cur = dst;
	for (int nh = 0; nh < heiOut; ++nh)
	{
		Temp_nh = (float)((nh + 0.5) / fScaleHei - 0.5);
		Temp_nh = Clamp(Temp_nh, 0.0f, heiIn - 1.0f);
		Round_nh_l = int(Temp_nh);
		Round_nh_r = int(ceil(Temp_nh));
		nU = uint32_t((Temp_nh - Round_nh_l) * nResizeScale);
		nInvU = nResizeScale - nU;

		nSkip_l = Round_nh_l * widIn;
		nSkip_r = Round_nh_r * widIn;

		for (int nw = 0; nw < widOut; ++nw)
		{
			Temp_nw = (float)((nw + 0.5) / fScaleWid - 0.5);
			Temp_nw = Clamp(Temp_nw, 0.0f, widIn - 1.0f);
			Round_nw_l = int(Temp_nw);
			Round_nw_r = int(ceil(Temp_nw));
			nV = uint32_t((Temp_nw - Round_nw_l) * nResizeScale);
			nInvV = nResizeScale - nV;

			uint32_t val0 = (uint32_t)src[nSkip_l + Round_nw_l] * nInvV + (uint32_t)src[nSkip_l + Round_nw_r] * nV;
			uint32_t val1 = (uint32_t)src[nSkip_r + Round_nw_l] * nInvV + (uint32_t)src[nSkip_r + Round_nw_r] * nV;

			*(dst_cur++) = (uint8_t)((val0 * nInvU + val1 * nU) >> 16);
		}
	}
	return;
}

/**@fn      ResizeBilinear_uint8
* @brief    uint8双线性插值各平台加速版本对外接口
* @param1   src                [I]     - 输入数据
* @param2   dst                [O]     - 输出数据
* @param3   heiIn              [I]     - 输入高度
* @param4   widIn              [I]     - 输入宽度
* @param5   heiOut             [I]     - 输出高度
* @param6   widOut             [I]     - 输出宽度
* @return   void
* @note
*/
static void ResizeBilinear_uint8(uint8_t* src, uint8_t* dst, int heiIn, int widIn, int heiOut, int widOut)
{
	ResizeBilinear_uint8_c(src, dst, heiIn, widIn, heiOut, widOut);
	return;
}

/**@fn      ResizeBilinear
* @brief    双线性插值外部调用接口
* @param1   matIn              [I]     - 输入数据
* @param2   matOut             [O]     - 输出数据
* @return   错误码
* @note
*/
XRAY_LIB_HRESULT ResizeBilinear(XMat& matIn, XMat& matOut)
{
	XSP_CHECK(matIn.IsValid() && matOut.IsValid(), XRAY_LIB_XMAT_INVALID);
	XSP_CHECK(matIn.type == matOut.type, XRAY_LIB_XMAT_TYPE_ERR);

	int heiIn, widIn, heiOut, widOut;
	heiIn = matIn.hei;
	widIn = matIn.wid;
	heiOut = matOut.hei;
	widOut = matOut.wid;

	if (heiIn == heiOut && widIn == widOut)
	{
		memcpy(matOut.Ptr(), matIn.Ptr(), heiIn * widIn * XSP_ELEM_SIZE(matIn.type));
		return XRAY_LIB_OK;
	}

	switch (matIn.type)
	{
	case XSP_8UC1:
	{
		ResizeBilinear_uint8(matIn.Ptr<uint8_t>(), matOut.Ptr<uint8_t>(),
			                 heiIn, widIn, heiOut, widOut);
		return XRAY_LIB_OK;
	}
	case XSP_16UC1:
	{
		ResizeBilinear_uint16(matIn.Ptr<uint16_t>(), matOut.Ptr<uint16_t>(),
			                  heiIn, widIn, heiOut, widOut);
		return XRAY_LIB_OK;
	}
	default :
	{
		return XRAY_LIB_XMAT_TYPE_UNSUPPORTED;
	}
	}
}

/**@fn      ResizeBilinearScale
* @brief    uint16双线性插值c版本
* @param1   src                [I]     - 输入数据
* @param2   dst                [O]     - 输出数据
* @param3   heiIn              [I]     - 输入高度
* @param4   widIn              [I]     - 输入宽度
* @param5   heiOut             [I]     - 输出高度
* @param6   widOut             [I]     - 输出宽度
* @param7   fScale             [I]     - 数值缩放系数
* @param8   fMin               [I]     - 数值上界
* @param9   fMax               [I]     - 数值下界
* @return   void
* @note
*/
template<typename DtypeIn, typename DtypeOut>
static void ResizeBilinearScale(DtypeIn* src, DtypeOut* dst, int heiIn, int widIn, int heiOut, int widOut, float fScale, float fMin, float fMax)
{
	if (heiIn == heiOut && widIn == widOut)
	{
		DtypeOut* pDst = dst;
		DtypeIn* pSrc = src;
		for (int idx = 0; idx < heiIn * widIn; ++idx)
		{
			float val = float(*(pSrc++)) * fScale;
			val = Clamp(val, fMin, fMax);
			*(pDst++) = (DtypeOut)(val);
		}

		return;
	}

	float fScaleHei = float(heiOut) / float(heiIn);
	float fScaleWid = float(widOut) / float(widIn);

	float Temp_nh, Temp_nw;
	int Round_nh_l, Round_nw_l, Round_nh_r, Round_nw_r;
	int nSkip_l, nSkip_r;
	float fU, fV, fInvU, fInvV;


	for (int nh = 0; nh < heiOut; ++nh)
	{
		Temp_nh = (float)((nh + 0.5) / fScaleHei - 0.5);
		Temp_nh = Clamp(Temp_nh, 0.0f, heiIn - 1.0f);
		Round_nh_l = int(Temp_nh);
		Round_nh_r = int(ceil(Temp_nh));
		fU = Temp_nh - Round_nh_l;
		fInvU = 1.0f - fU;

		nSkip_l = Round_nh_l * widIn;
		nSkip_r = Round_nh_r * widIn;

		DtypeOut* dst_cur = dst + nh * widOut;

		for (int nw = 0; nw < widOut; ++nw)
		{
			Temp_nw = (float)((nw + 0.5) / fScaleWid - 0.5);
			Temp_nw = Clamp(Temp_nw, 0.0f, widIn - 1.0f);
			Round_nw_l = int(Temp_nw);
			Round_nw_r = int(ceil(Temp_nw));
			fV = Temp_nw - Round_nw_l;
			fInvV = 1.0f - fV;

			float val0 = src[nSkip_l + Round_nw_l] * fInvV + src[nSkip_l + Round_nw_r] * fV;
			float val1 = src[nSkip_r + Round_nw_l] * fInvV + src[nSkip_r + Round_nw_r] * fV;

			float val = (val0 * fInvU + val1 * fU) * fScale;
			val = Clamp(val, fMin, fMax);

			*(dst_cur++) = (DtypeOut)(val);
		}
	}
	return;
}

XRAY_LIB_HRESULT ResizeBilinearScale(XMat& matIn, XMat& matOut, float fScale, float fMin, float fMax)
{
	XSP_CHECK(matIn.IsValid() && matOut.IsValid(), XRAY_LIB_XMAT_INVALID);

	int heiIn, widIn, heiOut, widOut;
	heiIn = matIn.hei;
	widIn = matIn.wid;
	heiOut = matOut.hei;
	widOut = matOut.wid;

	if (XSP_16UC1 == matIn.type && XSP_32FC1 == matOut.type)
	{
		ResizeBilinearScale(matIn.Ptr<uint16_t>(), matOut.Ptr<float>(), heiIn, widIn, heiOut, widOut, fScale, fMin, fMax);
		return XRAY_LIB_OK;
	}
	if (XSP_32FC1 == matIn.type && XSP_16UC1 == matOut.type)
	{
		ResizeBilinearScale(matIn.Ptr<float>(), matOut.Ptr<uint16_t>(), heiIn, widIn, heiOut, widOut, fScale, fMin, fMax);
		return XRAY_LIB_OK;
	}
	if (XSP_16UC1 == matIn.type && XSP_16UC1 == matOut.type)
	{
		ResizeBilinearScale(matIn.Ptr<uint16_t>(), matOut.Ptr<uint16_t>(), heiIn, widIn, heiOut, widOut, fScale, fMin, fMax);
		return XRAY_LIB_OK;
	}
	else
	{
		return XRAY_LIB_XMAT_TYPE_UNSUPPORTED;
	}

}

/***************************************************************************
*                               ResizeNearst
****************************************************************************/
template<typename Dtype>
static void ResizeNearst(Dtype* src, Dtype* dst, int heiIn, int widIn, int heiOut, int widOut)
{
	float fScaleHei = float(heiOut) / float(heiIn);
	float fScaleWid = float(widOut) / float(widIn);

	for (int i = 0; i < heiOut; i++)
	{
		for (int j = 0; j < widOut; j++)
		{
			int index_x = MAX(0, MIN(heiIn - 1, (int)round(i / fScaleHei)));
			int index_y = MAX(0, MIN(widIn - 1, (int)round(j / fScaleWid)));
			int index = i * widOut + j;
			int index_new = index_x * widIn + index_y;
			dst[index] = src[index_new];
		}
	}
	return;
}

/**@fn      ResizeNearst
* @brief    最近邻插值外部调用接口
* @param1   matIn              [I]     - 输入数据
* @param2   matOut             [O]     - 输出数据
* @return   错误码
* @note
*/
XRAY_LIB_HRESULT ResizeNearst(XMat& matIn, XMat& matOut)
{
	XSP_CHECK(matIn.IsValid() && matOut.IsValid(), XRAY_LIB_XMAT_INVALID);
	XSP_CHECK(matIn.type == matOut.type, XRAY_LIB_XMAT_TYPE_ERR);

	int heiIn, widIn, heiOut, widOut;
	heiIn = matIn.hei;
	widIn = matIn.wid;
	heiOut = matOut.hei;
	widOut = matOut.wid;

	if (heiIn == heiOut && widIn == widOut)
	{
		memcpy(matOut.Ptr(), matIn.Ptr(), heiIn * widIn * XSP_ELEM_SIZE(matIn.type));
		return XRAY_LIB_OK;
	}

	switch (matIn.type)
	{
	case XSP_8UC1:
	{
		ResizeNearst(matIn.Ptr<uint8_t>(), matOut.Ptr<uint8_t>(),
			         heiIn, widIn, heiOut, widOut);
		return XRAY_LIB_OK;
	}
	case XSP_16UC1:
	{
		ResizeNearst(matIn.Ptr<uint16_t>(), matOut.Ptr<uint16_t>(),
			         heiIn, widIn, heiOut, widOut);
		return XRAY_LIB_OK;
	}
	case XSP_32UC1:
	{
		ResizeNearst(matIn.Ptr<uint32_t>(), matOut.Ptr<uint32_t>(),
			         heiIn, widIn, heiOut, widOut);
		return XRAY_LIB_OK;
	}
	case XSP_32FC1:
	{
		ResizeNearst(matIn.Ptr<float>(), matOut.Ptr<float>(),
			         heiIn, widIn, heiOut, widOut);
		return XRAY_LIB_OK;
	}
	default :
	{
		return XRAY_LIB_XMAT_TYPE_UNSUPPORTED;
	}
	}
}

template<typename DtypeIn, typename DtypeOut>
static void ResizeNearstScale(DtypeIn* src, DtypeOut* dst, int heiIn, int widIn, int heiOut, int widOut, float fScale, float fMin, float fMax)
{
	if (heiIn == heiOut && widIn == widOut)
	{
		DtypeOut* pDst = dst;
		DtypeIn* pSrc = src;
		for (int idx = 0; idx < heiIn * widIn; ++idx)
		{
			float val = float(*(pSrc++)) * fScale;
			val = Clamp(val, fMin, fMax);
			*(pDst++) = (DtypeOut)(val);
		}

		return;
	}

	float fScaleHei = float(heiOut) / float(heiIn);
	float fScaleWid = float(widOut) / float(widIn);

	for (int i = 0; i < heiOut; i++)
	{
		for (int j = 0; j < widOut; j++)
		{
			int index_x = MAX(0, MIN(heiIn - 1, (int)round(i / fScaleHei)));
			int index_y = MAX(0, MIN(widIn - 1, (int)round(j / fScaleWid)));
			int index = i * widOut + j;
			int index_new = index_x * widIn + index_y;

			float val = src[index_new] * fScale;
			val = Clamp(val, fMin, fMax);
			dst[index] = val;
		}
	}
	return;
}

/**@fn      ResizeNearst
* @brief    最近邻插值外部调用接口
* @param1   matIn              [I]     - 输入数据
* @param2   matOut             [O]     - 输出数据
* @return   错误码
* @note
*/
XRAY_LIB_HRESULT ResizeNearstScale(XMat& matIn, XMat& matOut, float fScale, float fMin, float fMax)
{
	XSP_CHECK(matIn.IsValid() && matOut.IsValid(), XRAY_LIB_XMAT_INVALID);

	int heiIn, widIn, heiOut, widOut;
	heiIn = matIn.hei;
	widIn = matIn.wid;
	heiOut = matOut.hei;
	widOut = matOut.wid;

	if (XSP_8UC1 == matIn.type && XSP_32FC1 == matOut.type)
	{
		ResizeNearstScale(matIn.Ptr<uint8_t>(), matOut.Ptr<float>(), heiIn, widIn, heiOut, widOut, fScale, fMin, fMax);
		return XRAY_LIB_OK;
	}
	if (XSP_32FC1 == matIn.type && XSP_8UC1 == matOut.type)
	{
		ResizeNearstScale(matIn.Ptr<float>(), matOut.Ptr<uint8_t>(), heiIn, widIn, heiOut, widOut, fScale, fMin, fMax);
		return XRAY_LIB_OK;
	}
	if (XSP_16UC1 == matIn.type && XSP_32FC1 == matOut.type)
	{
		ResizeNearstScale(matIn.Ptr<uint16_t>(), matOut.Ptr<float>(), heiIn, widIn, heiOut, widOut, fScale, fMin, fMax);
		return XRAY_LIB_OK;
	}
	else
	{
		return XRAY_LIB_XMAT_TYPE_UNSUPPORTED;
	}
}

