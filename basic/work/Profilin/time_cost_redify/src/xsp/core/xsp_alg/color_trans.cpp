#include "xsp_alg.hpp"
#include <math.h>

/***********************************************
*               ARGB²ÊÉ«Í¼ÏñÐý×ª
************************************************/
XRAY_LIB_HRESULT MoveLeftArgb(XMat& matArgbIn, XMat& matArgbOut)
{
	/* check para */
	XSP_CHECK(matArgbIn.IsValid() && matArgbOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC4 == matArgbIn.type && XSP_8UC4 == matArgbOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matArgbIn.wid == matArgbOut.hei, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn wid : %d, argbOut hei : %d.", matArgbIn.wid, matArgbOut.hei);

	XSP_CHECK(matArgbIn.hei <= matArgbOut.wid, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn hei : %d, argbOut wid : %d.", matArgbIn.hei, matArgbOut.wid);

	int32_t nWidthIn = matArgbIn.wid;
	int32_t nHeightIn = matArgbIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pArgbOut = matArgbOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pArgbIn = matArgbIn.Ptr<uint8_t>(nh, nw);
			pArgbOut[0] = pArgbIn[0];
			pArgbOut[1] = pArgbIn[1];
			pArgbOut[2] = pArgbIn[2];
			pArgbOut[3] = pArgbIn[3];
			pArgbOut += XSP_ELEM_SIZE(XSP_8UC4);
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT MoveRightArgb(XMat& matArgbIn, XMat& matArgbOut)
{
	/* check para */
	XSP_CHECK(matArgbIn.IsValid() && matArgbOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC4 == matArgbIn.type && XSP_8UC4 == matArgbOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matArgbIn.wid == matArgbOut.hei, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn wid : %d, argbOut hei : %d.", matArgbIn.wid, matArgbOut.hei);

	XSP_CHECK(matArgbIn.hei <= matArgbOut.wid, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn hei : %d, argbOut wid : %d.", matArgbIn.hei, matArgbOut.wid);

	int32_t nWidthIn = matArgbIn.wid;
	int32_t nHeightIn = matArgbIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pArgbOut = matArgbOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pArgbIn = matArgbIn.Ptr<uint8_t>(nHeightIn - nh - 1, nw);
			pArgbOut[0] = pArgbIn[0];
			pArgbOut[1] = pArgbIn[1];
			pArgbOut[2] = pArgbIn[2];
			pArgbOut[3] = pArgbIn[3];
			pArgbOut += XSP_ELEM_SIZE(XSP_8UC4);
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT MoveLeftMirrorUpDownArgb(XMat& matArgbIn, XMat& matArgbOut)
{
	/* check para */
	XSP_CHECK(matArgbIn.IsValid() && matArgbOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC4 == matArgbIn.type && XSP_8UC4 == matArgbOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matArgbIn.wid == matArgbOut.hei, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn wid : %d, argbOut hei : %d.", matArgbIn.wid, matArgbOut.hei);

	XSP_CHECK(matArgbIn.hei <= matArgbOut.wid, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn hei : %d, argbOut wid : %d.", matArgbIn.hei, matArgbOut.wid);

	int32_t nWidthIn = matArgbIn.wid;
	int32_t nHeightIn = matArgbIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pArgbOut = matArgbOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pArgbIn = matArgbIn.Ptr<uint8_t>(nh, nWidthIn - nw - 1);
			pArgbOut[0] = pArgbIn[0];
			pArgbOut[1] = pArgbIn[1];
			pArgbOut[2] = pArgbIn[2];
			pArgbOut[3] = pArgbIn[3];
			pArgbOut += XSP_ELEM_SIZE(XSP_8UC4);
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT MoveRightMirrorUpDownArgb(XMat& matArgbIn, XMat& matArgbOut)
{
	/* check para */
	XSP_CHECK(matArgbIn.IsValid() && matArgbOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC4 == matArgbIn.type && XSP_8UC4 == matArgbOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matArgbIn.wid == matArgbOut.hei, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn wid : %d, argbOut hei : %d.", matArgbIn.wid, matArgbOut.hei);

	XSP_CHECK(matArgbIn.hei <= matArgbOut.wid, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn hei : %d, argbOut wid : %d.", matArgbIn.hei, matArgbOut.wid);

	int32_t nWidthIn = matArgbIn.wid;
	int32_t nHeightIn = matArgbIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pArgbOut = matArgbOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pArgbIn = matArgbIn.Ptr<uint8_t>(nHeightIn - nh - 1, nWidthIn - nw - 1);
			pArgbOut[0] = pArgbIn[0];
			pArgbOut[1] = pArgbIn[1];
			pArgbOut[2] = pArgbIn[2];
			pArgbOut[3] = pArgbIn[3];
			pArgbOut += XSP_ELEM_SIZE(XSP_8UC4);
		}
	}
	return XRAY_LIB_OK;
}

/***********************************************
*               RGB²ÊÉ«Í¼ÏñÐý×ª
************************************************/
XRAY_LIB_HRESULT MoveLeftRgb(XMat& matRgbIn, XMat& matRgbOut)
{
	/* check para */
	XSP_CHECK(matRgbIn.IsValid() && matRgbOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC3 == matRgbIn.type && XSP_8UC3 == matRgbOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matRgbIn.wid == matRgbOut.hei, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn wid : %d, argbOut hei : %d.", matRgbIn.wid, matRgbOut.hei);

	XSP_CHECK(matRgbIn.hei <= matRgbOut.wid, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn hei : %d, argbOut wid : %d.", matRgbIn.hei, matRgbOut.wid);

	int32_t nWidthIn = matRgbIn.wid;
	int32_t nHeightIn = matRgbIn.hei;
	#pragma omp parallel for schedule (static, 8)
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pRgbOut = matRgbOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pRgbIn = matRgbIn.Ptr<uint8_t>(nh, nw);
			pRgbOut[0] = pRgbIn[0];
			pRgbOut[1] = pRgbIn[1];
			pRgbOut[2] = pRgbIn[2];
			pRgbOut += XSP_ELEM_SIZE(XSP_8UC3);
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT MoveRightRgb(XMat& matRgbIn, XMat& matRgbOut)
{
	/* check para */
	XSP_CHECK(matRgbIn.IsValid() && matRgbOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC3 == matRgbIn.type && XSP_8UC3 == matRgbOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matRgbIn.wid == matRgbOut.hei, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn wid : %d, argbOut hei : %d.", matRgbIn.wid, matRgbOut.hei);

	XSP_CHECK(matRgbIn.hei <= matRgbOut.wid, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn hei : %d, argbOut wid : %d.", matRgbIn.hei, matRgbOut.wid);

	int32_t nWidthIn = matRgbIn.wid;
	int32_t nHeightIn = matRgbIn.hei;
	#pragma omp parallel for schedule (static, 8)
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pRgbOut = matRgbOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pRgbIn = matRgbIn.Ptr<uint8_t>(nHeightIn - nh - 1, nw);
			pRgbOut[0] = pRgbIn[0];
			pRgbOut[1] = pRgbIn[1];
			pRgbOut[2] = pRgbIn[2];
			pRgbOut += XSP_ELEM_SIZE(XSP_8UC3);
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT MoveLeftMirrorUpDownRgb(XMat& matRgbIn, XMat& matRgbOut)
{
	/* check para */
	XSP_CHECK(matRgbIn.IsValid() && matRgbOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC3 == matRgbIn.type && XSP_8UC3 == matRgbOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matRgbIn.wid == matRgbOut.hei, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn wid : %d, argbOut hei : %d.", matRgbIn.wid, matRgbOut.hei);

	XSP_CHECK(matRgbIn.hei <= matRgbOut.wid, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn hei : %d, argbOut wid : %d.", matRgbIn.hei, matRgbOut.wid);

	int32_t nWidthIn = matRgbIn.wid;
	int32_t nHeightIn = matRgbIn.hei;
	#pragma omp parallel for schedule (static, 8)
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pRgbOut = matRgbOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pRgbIn = matRgbIn.Ptr<uint8_t>(nh, nWidthIn - nw - 1);
			pRgbOut[0] = pRgbIn[0];
			pRgbOut[1] = pRgbIn[1];
			pRgbOut[2] = pRgbIn[2];
			pRgbOut += XSP_ELEM_SIZE(XSP_8UC3);
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT MoveRightMirrorUpDownRgb(XMat& matRgbIn, XMat& matRgbOut)
{
	/* check para */
	XSP_CHECK(matRgbIn.IsValid() && matRgbOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC3 == matRgbIn.type && XSP_8UC3 == matRgbOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matRgbIn.wid == matRgbOut.hei, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn wid : %d, argbOut hei : %d.", matRgbIn.wid, matRgbOut.hei);

	XSP_CHECK(matRgbIn.hei <= matRgbOut.wid, XRAY_LIB_XMAT_SIZE_ERR, 
		      "argbIn hei : %d, argbOut wid : %d.", matRgbIn.hei, matRgbOut.wid);

	int32_t nWidthIn = matRgbIn.wid;
	int32_t nHeightIn = matRgbIn.hei;
	#pragma omp parallel for schedule (static, 8)
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pRgbOut = matRgbOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pRgbIn = matRgbIn.Ptr<uint8_t>(nHeightIn - nh - 1, nWidthIn - nw - 1);
			pRgbOut[0] = pRgbIn[0];
			pRgbOut[1] = pRgbIn[1];
			pRgbOut[2] = pRgbIn[2];
			pRgbOut += XSP_ELEM_SIZE(XSP_8UC3);
		}
	}
	return XRAY_LIB_OK;
}

/**************************************************************************
*                            YUV²ÊÉ«Í¼ÏñÐý×ª
***************************************************************************/
XRAY_LIB_HRESULT MoveLeftYuv(XMat& matYIn, XMat& matUVIn,
	                         XMat& matYOut, XMat& matUVOut)
{
	/* check para */
	XSP_CHECK(matYIn.IsValid() && matUVIn.IsValid() && 
		      matYOut.IsValid() && matUVOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC1 == matYIn.type && XSP_8UC2 == matUVIn.type && 
		      XSP_8UC1 == matYOut.type && XSP_8UC2 == matUVOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matYIn.wid == matUVIn.wid * 2 && 
		      matYOut.wid == matUVOut.wid * 2, 
		      XRAY_LIB_XMAT_SIZE_ERR,
		      "matYIn wid : %d, matUVIn wid : %d, matYOut wid : %d, matUVOut wid : %d.", 
		      matYIn.wid, matUVIn.wid, matYOut.wid, matUVOut.wid);

	XSP_CHECK(matYIn.wid == matYOut.hei, XRAY_LIB_XMAT_SIZE_ERR,
		      "argbIn wid : %d, argbOut hei : %d.", matYIn.wid, matYOut.hei);

	XSP_CHECK(matYIn.hei <= matYOut.wid, XRAY_LIB_XMAT_SIZE_ERR,
		      "argbIn hei : %d, argbOut wid : %d.", matYIn.hei, matYOut.wid);

	int32_t nWidthIn = matYIn.wid;
	int32_t nHeightIn = matYIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pYOut = matYOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pYIn = matYIn.Ptr<uint8_t>(nh, nw);
			pYOut[0] = pYIn[0];
			pYOut++;
		}
	}

	nWidthIn = matUVIn.wid;
	nHeightIn = matUVIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pUVOut = matUVOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pUVIn = matUVIn.Ptr<uint8_t>(nh, nw);
			pUVOut[0] = pUVIn[0];
			pUVOut[1] = pUVIn[1];
			pUVOut += XSP_ELEM_SIZE(XSP_8UC2);
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT MoveRightYuv(XMat& matYIn, XMat& matUVIn,
	                          XMat& matYOut, XMat& matUVOut)
{
	/* check para */
	XSP_CHECK(matYIn.IsValid() && matUVIn.IsValid() && 
		      matYOut.IsValid() && matUVOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC1 == matYIn.type && XSP_8UC2 == matUVIn.type && 
		      XSP_8UC1 == matYOut.type && XSP_8UC2 == matUVOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matYIn.wid == matUVIn.wid * 2 && 
		      matYOut.wid == matUVOut.wid * 2, 
		      XRAY_LIB_XMAT_SIZE_ERR,
		      "matYIn wid : %d, matUVIn wid : %d, matYOut wid : %d, matUVOut wid : %d.", 
		      matYIn.wid, matUVIn.wid, matYOut.wid, matUVOut.wid);

	XSP_CHECK(matYIn.wid == matYOut.hei, XRAY_LIB_XMAT_SIZE_ERR,
		      "argbIn wid : %d, argbOut hei : %d.", matYIn.wid, matYOut.hei);

	XSP_CHECK(matYIn.hei <= matYOut.wid, XRAY_LIB_XMAT_SIZE_ERR,
		      "argbIn hei : %d, argbOut wid : %d.", matYIn.hei, matYOut.wid);

	int32_t nWidthIn = matYIn.wid;
	int32_t nHeightIn = matYIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pYOut = matYOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pYIn = matYIn.Ptr<uint8_t>(nHeightIn - nh - 1, nw);
			pYOut[0] = pYIn[0];
			pYOut++;
		}
	}

	nWidthIn = matUVIn.wid;
	nHeightIn = matUVIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pUVOut = matUVOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pUVIn = matUVIn.Ptr<uint8_t>(nHeightIn - nh - 1, nw);
			pUVOut[0] = pUVIn[0];
			pUVOut[1] = pUVIn[1];
			pUVOut += XSP_ELEM_SIZE(XSP_8UC2);
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT MoveLeftMirrorUpDownYuv(XMat& matYIn, XMat& matUVIn,
	                                     XMat& matYOut, XMat& matUVOut)
{
	/* check para */
	XSP_CHECK(matYIn.IsValid() && matUVIn.IsValid() && 
		      matYOut.IsValid() && matUVOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC1 == matYIn.type && XSP_8UC2 == matUVIn.type && 
		      XSP_8UC1 == matYOut.type && XSP_8UC2 == matUVOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matYIn.wid == matUVIn.wid * 2 && 
		      matYOut.wid == matUVOut.wid * 2, 
		      XRAY_LIB_XMAT_SIZE_ERR,
		      "matYIn wid : %d, matUVIn wid : %d, matYOut wid : %d, matUVOut wid : %d.", 
		      matYIn.wid, matUVIn.wid, matYOut.wid, matUVOut.wid);

	XSP_CHECK(matYIn.wid == matYOut.hei, XRAY_LIB_XMAT_SIZE_ERR,
		      "argbIn wid : %d, argbOut hei : %d.", matYIn.wid, matYOut.hei);

	XSP_CHECK(matYIn.hei <= matYOut.wid, XRAY_LIB_XMAT_SIZE_ERR,
		      "argbIn hei : %d, argbOut wid : %d.", matYIn.hei, matYOut.wid);

	int32_t nWidthIn = matYIn.wid;
	int32_t nHeightIn = matYIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pYOut = matYOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pYIn = matYIn.Ptr<uint8_t>(nh, nWidthIn - nw - 1);
			pYOut[0] = pYIn[0];
			pYOut++;
		}
	}

	nWidthIn = matUVIn.wid;
	nHeightIn = matUVIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pUVOut = matUVOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pUVIn = matUVIn.Ptr<uint8_t>(nh, nWidthIn - nw - 1);
			pUVOut[0] = pUVIn[0];
			pUVOut[1] = pUVIn[1];
			pUVOut += XSP_ELEM_SIZE(XSP_8UC2);
		}
	}
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT MoveRightMirrorUpDownYuv(XMat& matYIn, XMat& matUVIn,
	                                      XMat& matYOut, XMat& matUVOut)
{
	/* check para */
	XSP_CHECK(matYIn.IsValid() && matUVIn.IsValid() && 
		      matYOut.IsValid() && matUVOut.IsValid(),
		      XRAY_LIB_XMAT_INVALID, "Invalid XMat.");

	XSP_CHECK(XSP_8UC1 == matYIn.type && XSP_8UC2 == matUVIn.type && 
		      XSP_8UC1 == matYOut.type && XSP_8UC2 == matUVOut.type,
		      XRAY_LIB_XMAT_TYPE_ERR, "XMat type err.");

	XSP_CHECK(matYIn.wid == matUVIn.wid * 2 && 
		      matYOut.wid == matUVOut.wid * 2, 
		      XRAY_LIB_XMAT_SIZE_ERR,
		      "matYIn wid : %d, matUVIn wid : %d, matYOut wid : %d, matUVOut wid : %d.", 
		      matYIn.wid, matUVIn.wid, matYOut.wid, matUVOut.wid);

	XSP_CHECK(matYIn.wid == matYOut.hei, XRAY_LIB_XMAT_SIZE_ERR,
		      "argbIn wid : %d, argbOut hei : %d.", matYIn.wid, matYOut.hei);

	XSP_CHECK(matYIn.hei <= matYOut.wid, XRAY_LIB_XMAT_SIZE_ERR,
		      "argbIn hei : %d, argbOut wid : %d.", matYIn.hei, matYOut.wid);

	int32_t nWidthIn = matYIn.wid;
	int32_t nHeightIn = matYIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pYOut = matYOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pYIn = matYIn.Ptr<uint8_t>(nHeightIn - nh - 1, nWidthIn - nw - 1);
			pYOut[0] = pYIn[0];
			pYOut++;
		}
	}

	nWidthIn = matUVIn.wid;
	nHeightIn = matUVIn.hei;
	for (int32_t nw = 0; nw < nWidthIn; nw++)
	{
		uint8_t* pUVOut = matUVOut.Ptr<uint8_t>(nw);
		for (int32_t nh = 0; nh < nHeightIn; nh++)
		{
			uint8_t* pUVIn = matUVIn.Ptr<uint8_t>(nHeightIn - nh - 1, nWidthIn - nw - 1);
			pUVOut[0] = pUVIn[0];
			pUVOut[1] = pUVIn[1];
			pUVOut += XSP_ELEM_SIZE(XSP_8UC2);
		}
	}
	return XRAY_LIB_OK;
}