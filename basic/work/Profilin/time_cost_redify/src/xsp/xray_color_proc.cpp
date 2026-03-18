#include "xray_image_process.hpp"

/**@fn      RotateRgb
* @brief    rgb旋转
* @param1   matRgbIn           [I]     - 输入agb数据
* @param2   colorImg           [I/O]   - 输出agb数据
* @param3   enRotate           [I]     - 旋转参数
* @param4   enMirror           [I]     - 镜像参数
* @param5   nStrideH           [I]     - 输出rgb实际内存高度（时间方向）
* @param6   nStrideW           [I]     - 输出rgb实际内存宽度（探测器方向）
* @return   错误码
* @note     输出rgb内存按nStrideH方向连续
*/
static XRAY_LIB_HRESULT RotateRgb(XMat& matRgbIn, XRAYLIB_IMAGE& colorImg,
	                              XRAYLIB_IMG_ROTATE enRotate, 
	                              XRAYLIB_IMG_MIRROR enMirror,
	                              int32_t nStrideH, int32_t nStrideW)
{
	XSP_CHECK(matRgbIn.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(colorImg.pData[0], XRAY_LIB_NULLPTR, "XRAYLIB_IMAGE pData is null.");

	XRAY_LIB_HRESULT hr;
	XMat matRgbOut;
	hr = matRgbOut.Init(nStrideW, nStrideH, XSP_8UC3, (uint8_t*)colorImg.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	if (XRAYLIB_MOVE_RIGHT == enRotate && XRAYLIB_MIRROR_NONE == enMirror)
	{
		hr = MoveRightRgb(matRgbIn, matRgbOut);
	}
	else if (XRAYLIB_MOVE_LEFT == enRotate && XRAYLIB_MIRROR_NONE == enMirror)
	{
		hr = MoveLeftRgb(matRgbIn, matRgbOut);
	}
	else if (XRAYLIB_MOVE_RIGHT == enRotate && XRAYLIB_MIRROR_UPDOWN == enMirror)
	{
		hr = MoveRightMirrorUpDownRgb(matRgbIn, matRgbOut);
	}
	else if (XRAYLIB_MOVE_LEFT == enRotate && XRAYLIB_MIRROR_UPDOWN == enMirror)
	{
		hr = MoveLeftMirrorUpDownRgb(matRgbIn, matRgbOut);
	}
	else
	{
		log_error("UnSupport Rotate Mode: Rotate: %d; Mirror: %d\n", enRotate, enMirror);
		return XRAY_LIB_INVALID_PARAM;
	}
	colorImg.height = matRgbIn.wid;
	colorImg.width = matRgbIn.hei;
	return hr;
}

/**@fn      RotateYuv
* @brief    argb旋转
* @param1   matYIn             [I]     - 输入Y分量数据
* @param2   matUVIn            [I]     - 输入UV分量数据
* @param3   colorImg           [I/O]   - 输出yuv数据
* @param4   enRotate           [I]     - 旋转参数
* @param5   enMirror           [I]     - 镜像参数
* @param6   nStrideH           [I]     - 输出rgb实际内存高度（时间方向）
* @param7   nStrideW           [I]     - 输出rgb实际内存宽度（探测器方向）
* @return   错误码
* @note     输出yuv内存按nStrideH方向连续
*/
static XRAY_LIB_HRESULT RotateYuv(XMat& matYIn, XMat& matUVIn, XRAYLIB_IMAGE& colorImg,
	                              XRAYLIB_IMG_ROTATE enRotate, XRAYLIB_IMG_MIRROR enMirror, 
	                              int32_t nStrideH, int32_t nStrideW)
{
	XSP_CHECK(matYIn.IsValid() && matUVIn.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(colorImg.pData[0], XRAY_LIB_NULLPTR, "XRAYLIB_IMAGE pData is null.");

	XRAY_LIB_HRESULT hr;
	XMat matYOut, matUVOut;
	hr = matYOut.Init(nStrideW, nStrideH, XSP_8UC1, (uint8_t*)colorImg.pData[0]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Init matYOut err.");
	hr = matUVOut.Init(nStrideW >> 1, nStrideH >> 1, XSP_8UC2, (uint8_t*)colorImg.pData[1]);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "Init matUVOut err.");

	if (XRAYLIB_MOVE_RIGHT == enRotate && XRAYLIB_MIRROR_NONE == enMirror)
	{
		hr = MoveRightYuv(matYIn, matUVIn, matYOut, matUVOut);
	}
	else if (XRAYLIB_MOVE_LEFT == enRotate && XRAYLIB_MIRROR_NONE == enMirror)
	{
		hr = MoveLeftYuv(matYIn, matUVIn, matYOut, matUVOut);
	}
	else if (XRAYLIB_MOVE_RIGHT == enRotate && XRAYLIB_MIRROR_UPDOWN == enMirror)
	{
		hr = MoveRightMirrorUpDownYuv(matYIn, matUVIn, matYOut, matUVOut);
	}
	else if (XRAYLIB_MOVE_LEFT == enRotate && XRAYLIB_MIRROR_UPDOWN == enMirror)
	{
		hr = MoveLeftMirrorUpDownYuv(matYIn, matUVIn, matYOut, matUVOut);
	}
	else
	{
		log_error("UnSupport Rotate Mode: Rotate: %d; Mirror: %d\n", enRotate, enMirror);
		return XRAY_LIB_INVALID_PARAM;
	}
	colorImg.height = matYIn.wid;
	colorImg.width = matYIn.hei;
	return hr;
}

XRAY_LIB_HRESULT XRayImageProcess::GetColorImageBS(XMat& merge, XRAYLIB_IMAGE& colorImg, int32_t nColorWidthNeed, int32_t nStrideH, int32_t nStrideW)
{
	XRAY_LIB_HRESULT hr;

	if (nColorWidthNeed <= 0)
	{
		return XRAY_LIB_IMAGESIZE_ZERO;
	}

	XSP_CHECK(merge.IsValid(), XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	/***************
	*     RGB
	***************/
	hr = m_matEntireColorTemp.Reshape(nColorWidthNeed, merge.wid, XSP_8UC3);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	int32_t nWidth = merge.wid;

	/* 窗宽窗位选择 */
	float fGrayZoom = 65535.0f / (m_modDual.m_nGrayMax - m_modDual.m_nGrayMin);
	float fGrayAfterZoom;

	int32_t nh, nw, nGrayLevel = 0;
	int32_t nGrayBSTemp, nBSGray;

	uint16_t* pMerge = merge.PadPtr<uint16_t>();
	uint8_t* pArgbTemp = m_matEntireColorTemp.Ptr<uint8_t>();
	uint8_t nBSBgColor = (m_modDual.m_enInverseMode == XRAYLIB_INVERSE_NONE) ? 0 : 255;

	for (nh = 0; nh < nColorWidthNeed; nh++)
	{
		for (nw = 0; nw < merge.wid; nw++)
		{
			nBSGray = pMerge[nh * nWidth + nw];

			if (nBSGray < m_sharedPara.m_nBackGroundGray)
				nBSGray += m_modDual.m_nBrightnessAdjust;
			
			nBSGray = Clamp(nBSGray, m_modDual.m_nGrayMin, m_modDual.m_nGrayMax);

			nGrayBSTemp = (m_modDual.m_enInverseMode == XRAYLIB_INVERSE_NONE) ? nBSGray : (m_modDual.m_nGrayMax - nBSGray + m_modDual.m_nGrayMin);

			fGrayAfterZoom = (nGrayBSTemp - m_modDual.m_nGrayMin) * fGrayZoom;
			nGrayLevel = (ushort)(fGrayAfterZoom / 65535.0f * 255.0f);

			pArgbTemp[3 * nh * nWidth + 3 * nw] = nGrayLevel;
			pArgbTemp[3 * nh * nWidth + 3 * nw + 1] = nGrayLevel;
			pArgbTemp[3 * nh * nWidth + 3 * nw + 2] = nGrayLevel;

			if (nw < m_modDual.m_nWhiteUpBs || nw > merge.wid - m_modDual.m_nWhiteDownBs)
			{
				pArgbTemp[3 * nh * nWidth + 3 * nw] = nBSBgColor;
				pArgbTemp[3 * nh * nWidth + 3 * nw + 1] = nBSBgColor;
				pArgbTemp[3 * nh * nWidth + 3 * nw + 2] = nBSBgColor;
			}

		}
	}

	hr = RotateRgb(m_matEntireColorTemp, colorImg, m_enRotate, m_enMirror, nStrideH, nStrideW);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "RotateRgb err");
	
	return XRAY_LIB_OK;
}

// int gColorCount = 0;
XRAY_LIB_HRESULT XRayImageProcess::GetColorImage(XRAYLIB_IMAGE& imgOut, XMat& gray, XMat& z, XMat& wt, 
                                                 std::vector<XRAYLIB_RECT>& procWins, int32_t nStrideH, int32_t nStrideW)
{
	XSP_CHECK(XRAYLIB_IMG_RGB_ARGB_C3 == imgOut.format, XRAY_LIB_IMAGETYPE_ERROR, "color format unsupported");
	XSP_CHECK(MatSizeEq(gray, z) && MatSizeEq(gray, wt), XRAY_LIB_XMAT_SIZE_ERR, "XMat no equal");

	XRAY_LIB_HRESULT hr;
    hr = m_matEntireColorTemp.Reshape(gray.hei - gray.tpad - gray.bpad, gray.wid, XSP_8UC3);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);
	
	hr = m_modDual.GetColorImageRgb(m_matEntireColorTemp, gray, z, wt, procWins);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "GetColorImageRgb err");

	hr = RotateRgb(m_matEntireColorTemp, imgOut, m_enRotate, m_enMirror, nStrideH, nStrideW);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "RotateRgb err");

	return hr;
}


XRAY_LIB_HRESULT XRayImageProcess::GetAiColorImage(XMat& merge, XMat& zData,
	                                               XRAYLIB_IMAGE& colorImg,
	                                               int32_t nColorWidthNeed,
	                                               int32_t nStrideH, int32_t nStrideW)
{
	XRAY_LIB_HRESULT hr;

	/***************
	*     YUV
	***************/
	hr = m_matEntireColorTemp.Reshape(nColorWidthNeed * 2, merge.wid, XSP_8UC1);
	XSP_CHECK(XRAY_LIB_OK == hr, hr);

	XMat matYTemp, matUVTemp;
	matYTemp.Init(nColorWidthNeed, merge.wid, XSP_8UC1, m_matEntireColorTemp.Ptr());
	matUVTemp.Init(nColorWidthNeed >> 1, merge.wid >> 1, XSP_8UC2, m_matEntireColorTemp.Ptr(nColorWidthNeed));

	hr = m_modDual.GetColorImageAiYuv(merge, zData, matYTemp, matUVTemp);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "GetColorImageYuv err");

	hr = RotateYuv(matYTemp, matUVTemp, colorImg, m_enRotate, m_enMirror, nStrideH, nStrideW);
	XSP_CHECK(XRAY_LIB_OK == hr, hr, "RotateYuv err");

	return XRAY_LIB_OK;
}