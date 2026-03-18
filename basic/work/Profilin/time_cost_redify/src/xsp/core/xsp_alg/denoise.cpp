#include "xsp_alg.hpp"
#include "core_def.hpp"
#include <math.h>
#include <vector>
#include <memory>

XRAY_LIB_HRESULT Padarray(unsigned short* ptrImageIn, unsigned short* ptrImageOut, int nHeight, int nWidth, int padnum) 
{
	int nh, nw;
	int nwidthout = nWidth + padnum * 2;
	for (nh = padnum; nh<(padnum + nHeight); nh++) {
		for (nw = 0; nw<padnum; nw++) {
			ptrImageOut[nh*nwidthout + nw] = ptrImageIn[(nh - padnum)*nWidth + padnum - nw - 1];
		}
		for (nw = padnum; nw<(padnum + nWidth); nw++) {
			ptrImageOut[nh*nwidthout + nw] = ptrImageIn[(nh - padnum)*nWidth + nw - padnum];
		}
		for (nw = padnum + nWidth; nw<(padnum * 2 + nWidth); nw++) {
			ptrImageOut[nh*nwidthout + nw] = ptrImageIn[(nh - padnum)*nWidth + nWidth - 1 - (nw - padnum - nWidth)];
		}
	}
	for (nw = 0; nw<(padnum * 2 + nWidth); nw++) {
		for (nh = 0; nh<padnum; nh++) {
			ptrImageOut[nh*nwidthout + nw] = ptrImageOut[(padnum * 2 - nh - 1)*nwidthout + nw];
		}
		for (nh = (padnum + nHeight); nh<(padnum * 2 + nHeight); nh++) {
			ptrImageOut[nh*nwidthout + nw] = ptrImageOut[(nh - (nh - padnum - nHeight) * 2 - 1)*nwidthout + nw];
		}
	}

	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT IntegralImgSqDiff(unsigned short* ptrImageIn, unsigned int* ptrImageSd, unsigned short* ptrImageV2, unsigned short* ptrImageV2v,
												 int nHeight, int nWidth, int t1, int t2, int m1, int n1, int ds, int ks) 
{
	float dist2 = 0;
	for (int nh = 0; nh<m1; nh++) {
		for (int nw = 0; nw<n1; nw++) {
			dist2 = ptrImageIn[(nh + ds)*nWidth + nw + ds] - ptrImageIn[(nh + ds + t1)*nWidth + nw + ds + t2];
			dist2 = dist2 * dist2;
			if (nh == 0 && nw == 0) {
				ptrImageSd[nh*n1 + nw] = (ushort)dist2;
				// printf("if:ptrImageSd %d\n", ptrImageSd[nh * n1 + nw]);
			}
			else if (nh == 0 && nw != 0) {
				ptrImageSd[nh*n1 + nw] = ptrImageSd[nh*n1 + nw - 1] + (ushort)dist2;
				// printf("elif0:ptrImageSd %d ptrImageSd[nw-1] %d\n", ptrImageSd[nh * n1 + nw], ptrImageSd[nh*n1 + nw - 1]);
			}
			else if (nh != 0 && nw == 0) {
				ptrImageSd[nh*n1 + nw] = ptrImageSd[(nh - 1)*n1 + nw] + (ushort)dist2;
				// printf("elif1:ptrImageSd %d ptrImageSd[nh-1] %d\n", ptrImageSd[nh * n1 + nw], ptrImageSd[(nh - 1)*n1 + nw]);
			}
			else {
				ptrImageSd[nh*n1 + nw] = ptrImageSd[nh*n1 + nw - 1] + ptrImageSd[(nh - 1)*n1 + nw] - ptrImageSd[(nh - 1)*n1 + nw - 1] + (ushort)dist2;
				// printf("elif2:ptrImageSd %d ptrImageSd[nw-1] %d ptrImageSd[nh-1] %d ptrImageSd[nhw-1] %d\n", 
				// 		ptrImageSd[nh * n1 + nw], ptrImageSd[nh*n1 + nw - 1], ptrImageSd[(nh - 1)*n1 + nw], ptrImageSd[(nh - 1)*n1 + nw - 1]);
			}
		}
	}

	// 计算扩边数组
	for (int nh = 0; nh<(nHeight - (ds + ks + 1) * 2); nh++) {
		for (int nw = 0; nw<(nWidth - (ds + ks + 1) * 2); nw++) {
			ptrImageV2v[nh*(nWidth - (ds + ks + 1) * 2) + nw] = ptrImageV2[(nh + ds + t1)*(nWidth - (ks + 1) * 2) + nw + ds + t2];
		}
	}

	return XRAY_LIB_OK;
}

// 背散的校正模板更新、后处理两个地方调用, matsProc分别在ImgProc和Cali模块中申请
XRAY_LIB_HRESULT NLMeans(XMat& matIn, XMat& matOut, std::vector<XMat>& matsProc)
{
    XRAY_LIB_HRESULT hr;

    int ds = 5, ks = 2;
    float sigmaI = 40.0f;
    int nHeight = matIn.hei;
    int nWidth = matIn.wid; 

    // 最大扩边数组的尺寸
    int padnum = ds + ks + 1;   // 8
    int nwidthout = nWidth + padnum * 2;        // nwidth + 16
    int nheightout = nHeight + padnum * 2;      // nheight + 16

    // 积分图的尺寸
    int m1 = nheightout - ds * 2;   // nheight + 6
    int n1 = nwidthout - ds * 2;    // nwidth + 6

    // 初始化指针
    ushort* ptrImageIn = matIn.Ptr<ushort>();
    ushort* ptrImageOut = matOut.Ptr<ushort>();

    // 计算扩边数组
	matsProc[0].Reshape(nheightout, nwidthout, XSP_16UC1);
    ushort* ptrImageV1 = matsProc[0].Ptr<ushort>();
    memset(ptrImageV1, 0, sizeof(ushort) * nheightout * nwidthout);
    hr = Padarray(ptrImageIn, ptrImageV1, nHeight, nWidth, ds + ks + 1);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Padarray err.");

	matsProc[1].Reshape(nHeight + ds * 2, nWidth + ds * 2, XSP_16UC1);
    ushort* ptrImageV2 = matsProc[1].Ptr<ushort>();
    memset(ptrImageV2, 0, sizeof(ushort) * (nHeight + ds * 2) * (nWidth + ds * 2));
    hr = Padarray(ptrImageIn, ptrImageV2, nHeight, nWidth, ds);
    XSP_CHECK(XRAY_LIB_OK == hr, hr, "Padarray err.");

	matsProc[2].Reshape(nHeight, nWidth, XSP_16UC1);
    ushort* ptrImageV2v = matsProc[2].Ptr<ushort>();
    memset(ptrImageV2v, 0, sizeof(ushort) * nHeight * nWidth);

	matsProc[3].Reshape(m1, n1, XSP_32UC1);
    uint* ptrImageSd = matsProc[3].Ptr<uint>();
    memset(ptrImageSd, 0, sizeof(uint) * m1 * n1);

	matsProc[4].Reshape(nHeight, nWidth, XSP_32FC1);
    float* ptrfAverage = matsProc[4].Ptr<float>();
    memset(ptrfAverage, 0, sizeof(float) * nHeight * nWidth);

	matsProc[5].Reshape(nHeight, nWidth, XSP_32FC1);
    float* ptrfWeight = matsProc[5].Ptr<float>();
    memset(ptrfWeight, 0, sizeof(float) * nHeight * nWidth);

	matsProc[6].Reshape(nHeight, nWidth, XSP_32FC1);
    float* ptrfWMax = matsProc[6].Ptr<float>();
    memset(ptrfWMax, 0, sizeof(float) * nHeight * nWidth);

    float wei = 0.0f;
    float h2 = sigmaI * sigmaI;
    float d2 = (2 * ks + 1) * (2 * ks + 1);
    for (int t1 = -ds; t1 <= ds; t1++) {
		for (int t2 = -ds; t2 <= ds; t2++) {
			if (t1 == 0 && t2 == 0) {
				continue;
			}
			hr = IntegralImgSqDiff(ptrImageV1, ptrImageSd, ptrImageV2, ptrImageV2v, nheightout, nwidthout, t1, t2, m1, n1, ds, ks);
            XSP_CHECK(XRAY_LIB_OK == hr, hr, "IntegralImgSqDiff err.");
            
			for (int nh = 0; nh<nHeight; nh++) {
				for (int nw = 0; nw<nWidth; nw++) {
					int i1 = nh + ks + 1;
					int j1 = nw + ks + 1;
					float Dist22 = (float) (ptrImageSd[(i1 + ks)*n1 + j1 + ks] + ptrImageSd[(i1 - ks - 1)*n1 + j1 - ks - 1] -
								  ptrImageSd[(i1 + ks)*n1 + j1 - ks - 1] - ptrImageSd[(i1 - ks - 1)*n1 + j1 + ks]);
					Dist22 /= d2;
					wei = exp(-Dist22 / h2);
					ptrfWeight[nh*nWidth + nw] += wei;
					ptrfAverage[nh*nWidth + nw] += wei * ptrImageV2v[nh*nWidth + nw];
					ptrfWMax[nh*nWidth + nw] = (ptrfWMax[nh*nWidth + nw]>wei) ? ptrfWMax[nh*nWidth + nw] : wei;
				}
			}
		}
	}

	for (int nh = 0; nh<nHeight; nh++) {
		for (int nw = 0; nw<nWidth; nw++) {
			ptrfWeight[nh*nWidth + nw] += ptrfWMax[nh*nWidth + nw];
			ptrfAverage[nh*nWidth + nw] += ptrfWMax[nh*nWidth + nw] * ptrImageIn[nh*nWidth + nw];
			ptrImageOut[nh*nWidth + nw] = (unsigned short)(ptrfAverage[nh*nWidth + nw] / ptrfWeight[nh*nWidth + nw]);
		}
	}
	
	return XRAY_LIB_OK;
}

XRAY_LIB_HRESULT MirrorUpDownBS_u16(XMat& matData, int nWidthInBs)
{
	XSP_CHECK(matData.IsValid(), XRAY_LIB_XMAT_INVALID);
	int nHeight = matData.hei;
	int nWidth = matData.wid;

	std::unique_ptr<ushort[]> tempBuffer(new ushort[nWidth * nHeight]);
	memcpy(tempBuffer.get(), matData.Ptr<ushort>(0), sizeof(ushort) * matData.wid * matData.hei);
	ushort *pu16OrgData = matData.Ptr<ushort>(0);

	for (int nh = 0; nh < nHeight; nh++)
	{
		for (int nw = 0; nw < nWidthInBs; nw++)
		{
			pu16OrgData[nWidth * nh + nWidthInBs - nw - 1] = tempBuffer[nWidth * nh + nw];
		}
	}

	return XRAY_LIB_OK;
}