/**
 * @file   xray_hostcmd_proc.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  X ray数据下载解析预处理模块命令交互
 * @author liwenbin
 * @date   2019/10/19
 * @note :
 * @note \n History:
   1.Date        : 2019/10/19
     Author      : liwenbin
     Modification: Created file
 */


/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"
#include "xray_tsk.h"
#include "xray_hostcmd_proc.h"
#include "xsp_wrap.h"

#line __LINE__ "xray_hostcmd_proc.c"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
// 二维双线性插值权重
typedef struct
{
    UINT32 src00x;
    UINT32 src00y;
    UINT32 src11x;
    UINT32 src11y;
    FLOAT32 wgt00;
    FLOAT32 wgt01;
    FLOAT32 wgt10;
    FLOAT32 wgt11;
}WEIGHT_PARAM;


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */



/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

SAL_STATUS Host_XraySplitNraw(UINT32 chan, UINT32 *pParam, VOID *pBuf)
{
    UINT32 i = 0, u32DestHeight = 0; /* 截取的高度 */
    UINT32 u32LeLineSize = 0, u32HeLineSize = 0, u32ZLineSize = 0; /* 一行数据的大小，单位：字节 */
    UINT8 *pu8LeSrc = NULL, *pu8HeSrc = NULL, *pu8ZSrc = NULL; /* 源数据地址 */
    UINT8 *pu8LeDest = NULL, *pu8HeDest = NULL, *pu8ZDest = NULL; /* 目的数据地址 */
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    CAPB_XSP *pstCapXsp = capb_get_xsp();
    XRAY_NRAW_SPLIT_PARAM *pNrawSplit = (XRAY_NRAW_SPLIT_PARAM *)pBuf;

    /* 输入参数校验*/
    if (NULL == pBuf)
    {
        XSP_LOGE("the 'pBuf' is NULL\n");
        return SAL_FAIL;
    }

    if (NULL == pNrawSplit->pSrcBuf || NULL == pNrawSplit->pDestBuf)
    {
        XSP_LOGE("the pSrcBuf[%p] OR pDestBuf[%p] is NULL\n", pNrawSplit->pSrcBuf, pNrawSplit->pDestBuf);
        return SAL_FAIL;
    }

    if (pNrawSplit->u32StartLine == pNrawSplit->u32EndLine
        || pNrawSplit->u32StartLine >= pNrawSplit->u32SrcHeight
        || pNrawSplit->u32EndLine >= pNrawSplit->u32SrcHeight)
    {
        XSP_LOGE("the u32StartLine[%u] OR u32EndLine[%u] is invalid, range: [0, %u)\n",
                 pNrawSplit->u32StartLine, pNrawSplit->u32EndLine, pNrawSplit->u32SrcHeight);
        return SAL_FAIL;
    }

    if (pNrawSplit->u32SrcWidth > pstCapXsp->xraw_width_resized_max)
    {
        XSP_LOGW("the u32SrcWidth[%u] maybe invalid, but xraw_width_max is %u\n", pNrawSplit->u32SrcWidth, pstCapXsp->xraw_width_resized_max);
    }

    if (pNrawSplit->u32StartLine < pNrawSplit->u32EndLine) /* 正向拷贝 */
    {
        u32DestHeight = pNrawSplit->u32EndLine - pNrawSplit->u32StartLine + 1;

        /* 低能 */
        pu8LeSrc = XRAW_LE_OFFSET(pNrawSplit->pSrcBuf, pNrawSplit->u32SrcWidth, pNrawSplit->u32SrcHeight)
                   + XRAW_LE_SIZE(pNrawSplit->u32SrcWidth, pNrawSplit->u32StartLine);
        pu8LeDest = XRAW_LE_OFFSET(pNrawSplit->pDestBuf, pNrawSplit->u32SrcWidth, u32DestHeight);
        memcpy(pu8LeDest, pu8LeSrc, XRAW_LE_SIZE(pNrawSplit->u32SrcWidth, u32DestHeight));

        if (XRAY_ENERGY_DUAL == pstCapXrayIn->xray_energy_num)
        {
            /* 高能 */
            pu8HeSrc = XRAW_HE_OFFSET(pNrawSplit->pSrcBuf, pNrawSplit->u32SrcWidth, pNrawSplit->u32SrcHeight)
                       + XRAW_HE_SIZE(pNrawSplit->u32SrcWidth, pNrawSplit->u32StartLine);
            pu8HeDest = XRAW_HE_OFFSET(pNrawSplit->pDestBuf, pNrawSplit->u32SrcWidth, u32DestHeight);
            memcpy(pu8HeDest, pu8HeSrc, XRAW_HE_SIZE(pNrawSplit->u32SrcWidth, u32DestHeight));

            /* 原子序数 */
            pu8ZSrc = XRAW_Z_OFFSET(pNrawSplit->pSrcBuf, pNrawSplit->u32SrcWidth, pNrawSplit->u32SrcHeight)
                      + XRAW_Z_SIZE(pNrawSplit->u32SrcWidth, pNrawSplit->u32StartLine);
            pu8ZDest = XRAW_Z_OFFSET(pNrawSplit->pDestBuf, pNrawSplit->u32SrcWidth, u32DestHeight);
            memcpy(pu8ZDest, pu8ZSrc, XRAW_Z_SIZE(pNrawSplit->u32SrcWidth, u32DestHeight));
        }
    }
    else /* 反向拷贝 */
    {
        u32DestHeight = pNrawSplit->u32StartLine - pNrawSplit->u32EndLine + 1;

        /* 低能 */
        u32LeLineSize = XRAW_LE_SIZE(pNrawSplit->u32SrcWidth, 1);
        pu8LeSrc = XRAW_LE_OFFSET(pNrawSplit->pSrcBuf, pNrawSplit->u32SrcWidth, pNrawSplit->u32SrcHeight)
                   + XRAW_LE_SIZE(pNrawSplit->u32SrcWidth, pNrawSplit->u32StartLine);
        pu8LeDest = XRAW_LE_OFFSET(pNrawSplit->pDestBuf, pNrawSplit->u32SrcWidth, u32DestHeight);
        for (i = 0; i < u32DestHeight; i++)
        {
            memcpy(pu8LeDest, pu8LeSrc, u32LeLineSize);
            pu8LeSrc -= u32LeLineSize;
            pu8LeDest += u32LeLineSize;
        }

        if (XRAY_ENERGY_DUAL == pstCapXrayIn->xray_energy_num)
        {
            /* 高能 */
            u32HeLineSize = XRAW_HE_SIZE(pNrawSplit->u32SrcWidth, 1);
            pu8HeSrc = XRAW_HE_OFFSET(pNrawSplit->pSrcBuf, pNrawSplit->u32SrcWidth, pNrawSplit->u32SrcHeight)
                       + XRAW_HE_SIZE(pNrawSplit->u32SrcWidth, pNrawSplit->u32StartLine);
            pu8HeDest = XRAW_HE_OFFSET(pNrawSplit->pDestBuf, pNrawSplit->u32SrcWidth, u32DestHeight);
            for (i = 0; i < u32DestHeight; i++)
            {
                memcpy(pu8HeDest, pu8HeSrc, u32HeLineSize);
                pu8HeSrc -= u32HeLineSize;
                pu8HeDest += u32HeLineSize;
            }

            /* 原子序数 */
            u32ZLineSize = XRAW_Z_SIZE(pNrawSplit->u32SrcWidth, 1);
            pu8ZSrc = XRAW_Z_OFFSET(pNrawSplit->pSrcBuf, pNrawSplit->u32SrcWidth, pNrawSplit->u32SrcHeight)
                      + XRAW_Z_SIZE(pNrawSplit->u32SrcWidth, pNrawSplit->u32StartLine);
            pu8ZDest = XRAW_Z_OFFSET(pNrawSplit->pDestBuf, pNrawSplit->u32SrcWidth, u32DestHeight);
            for (i = 0; i < u32DestHeight; i++)
            {
                memcpy(pu8ZDest, pu8ZSrc, u32ZLineSize);
                pu8ZSrc -= u32ZLineSize;
                pu8ZDest += u32ZLineSize;
            }
        }
    }

    return SAL_SOK;
}

/**
 * @function    Host_XraySpliceNraw
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS Host_XraySpliceNraw(UINT32 chan, UINT32 *pParam, VOID *pBuf)
{
    UINT32 i = 0, u32SliceNum = 0; /* 条带数 */
    UINT32 u32SliceSize = 0; /* 条带数据的大小，单位：字节 */
    UINT32 u32SliceLeSize = 0, u32SliceHeSize = 0, u32SliceZSize = 0;
    UINT8 *pu8LeSrc = NULL, *pu8HeSrc = NULL, *pu8ZSrc = NULL; /* 源数据地址 */
    UINT8 *pu8LeDest = NULL, *pu8HeDest = NULL, *pu8ZDest = NULL; /* 目的数据地址 */
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
	CAPB_XSP *pstCapXsp = capb_get_xsp();
    XRAY_NRAW_SPLICE_PARAM *pNrawSplice = (XRAY_NRAW_SPLICE_PARAM *)pBuf;

    /* 输入参数校验*/
    if (NULL == pBuf)
    {
        XSP_LOGE("the 'pBuf' is NULL\n");
        return SAL_FAIL;
    }

    if (NULL == pNrawSplice->pSrcBuf || NULL == pNrawSplice->pDestBuf)
    {
        XSP_LOGE("the pSrcBuf[%p] OR pDestBuf[%p] is NULL\n", pNrawSplice->pSrcBuf, pNrawSplice->pDestBuf);
        return SAL_FAIL;
    }

    if (0 == pNrawSplice->u32SrcWidth || 0 == pNrawSplice->u32SrcHeightTotal || 0 == pNrawSplice->u32SliceHeight || 
        0 != pNrawSplice->u32SrcHeightTotal % pNrawSplice->u32SliceHeight || pNrawSplice->u32SrcHeightTotal < pNrawSplice->u32SliceHeight || 
        pNrawSplice->u32BufSize != pNrawSplice->u32SrcHeightTotal * pNrawSplice->u32SrcWidth * pstCapXsp->xsp_normalized_raw_bw)
    {
        XSP_LOGE("the u32SrcWidth[%u] OR u32SrcHeightTotal[%u] OR u32SliceHeight[%u] OR u32BufSize[%u] is invalid\n",
                 pNrawSplice->u32SrcWidth, pNrawSplice->u32SrcHeightTotal, pNrawSplice->u32SliceHeight, pNrawSplice->u32BufSize);
        return SAL_FAIL;
    }

    u32SliceNum = pNrawSplice->u32SrcHeightTotal / pNrawSplice->u32SliceHeight;
    u32SliceSize = pNrawSplice->u32SrcWidth * pNrawSplice->u32SliceHeight * pstCapXsp->xsp_normalized_raw_bw;

    if (pNrawSplice->u32SliceOrder == 0)  /* 正序拼接 */
    {
        /* 低能 */
        u32SliceLeSize = XRAW_LE_SIZE(pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
        pu8LeSrc = XRAW_LE_OFFSET(pNrawSplice->pSrcBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
        pu8LeDest = XRAW_LE_OFFSET(pNrawSplice->pDestBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SrcHeightTotal);
        for (i = 0; i < u32SliceNum; i++)
        {
            memcpy(pu8LeDest, pu8LeSrc, u32SliceLeSize);
            pu8LeSrc += u32SliceSize;
            pu8LeDest += u32SliceLeSize;
        }

        if (XRAY_ENERGY_DUAL == pstCapXrayIn->xray_energy_num)
        {
            /* 高能 */
            u32SliceHeSize = XRAW_HE_SIZE(pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
            pu8HeSrc = XRAW_HE_OFFSET(pNrawSplice->pSrcBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
            pu8HeDest = XRAW_HE_OFFSET(pNrawSplice->pDestBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SrcHeightTotal);
            for (i = 0; i < u32SliceNum; i++)
            {
                memcpy(pu8HeDest, pu8HeSrc, u32SliceHeSize);
                pu8HeSrc += u32SliceSize;
                pu8HeDest += u32SliceHeSize;
            }

            /* 原子序数 */
            u32SliceZSize = XRAW_Z_SIZE(pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
            pu8ZSrc = XRAW_Z_OFFSET(pNrawSplice->pSrcBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
            pu8ZDest = XRAW_Z_OFFSET(pNrawSplice->pDestBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SrcHeightTotal);
            for (i = 0; i < u32SliceNum; i++)
            {
                memcpy(pu8ZDest, pu8ZSrc, u32SliceZSize);
                pu8ZSrc += u32SliceSize;
                pu8ZDest += u32SliceZSize;
            }
        }
    }
    else if (pNrawSplice->u32SliceOrder == 1)  /* 逆序拼接 */
    {
        /* 低能 */

        u32SliceLeSize = XRAW_LE_SIZE(pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
        pu8LeSrc = XRAW_LE_OFFSET(pNrawSplice->pSrcBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
        pu8LeDest = XRAW_LE_OFFSET(pNrawSplice->pDestBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SrcHeightTotal);
		pu8LeDest += (u32SliceNum - 1) * u32SliceLeSize;
        for (i = 0; i < u32SliceNum; i++)
        {
            memcpy(pu8LeDest, pu8LeSrc, u32SliceLeSize);
            pu8LeSrc += u32SliceSize;
            pu8LeDest -= u32SliceLeSize;
        }

        if (XRAY_ENERGY_DUAL == pstCapXrayIn->xray_energy_num)
        {
            /* 高能 */
            u32SliceHeSize = XRAW_HE_SIZE(pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
            pu8HeSrc = XRAW_HE_OFFSET(pNrawSplice->pSrcBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
            pu8HeDest = XRAW_HE_OFFSET(pNrawSplice->pDestBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SrcHeightTotal);
			pu8HeDest += (u32SliceNum - 1) * u32SliceHeSize;
            for (i = 0; i < u32SliceNum; i++)
            {
                memcpy(pu8HeDest, pu8HeSrc, u32SliceHeSize);
                pu8HeSrc += u32SliceSize;
                pu8HeDest -= u32SliceHeSize;
            }

            /* 原子序数 */
            u32SliceZSize = XRAW_Z_SIZE(pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
            pu8ZSrc = XRAW_Z_OFFSET(pNrawSplice->pSrcBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SliceHeight);
            pu8ZDest = XRAW_Z_OFFSET(pNrawSplice->pDestBuf, pNrawSplice->u32SrcWidth, pNrawSplice->u32SrcHeightTotal);
			pu8ZDest += (u32SliceNum - 1) * u32SliceZSize;
            for (i = 0; i < u32SliceNum; i++)
            {
                memcpy(pu8ZDest, pu8ZSrc, u32SliceZSize);
                pu8ZSrc += u32SliceSize;
                pu8ZDest -= u32SliceZSize;
            }
        }
    }
    else
    {
        XSP_LOGE("u32SliceOrder is error %d\n", pNrawSplice->u32SliceOrder);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


static SAL_STATUS linear_resize(void *pDstImg, UINT32 u32DstW, UINT32 u32DstH, void *pSrcImg, UINT32 u32SrcW, UINT32 u32SrcH, WEIGHT_PARAM *weightTable, UINT32 u32ImgBw)
{
    UINT32 i = 0, j = 0;
    FLOAT32 f32Tmp = 0;
    UINT8 *pu8Line0 = NULL, *pu8Line1 = NULL;
    UINT8 *pu8DstImg = (UINT8 *)pDstImg, *pu8SrcImg = (UINT8 *)pSrcImg;
    UINT16 *pu16Line0 = NULL, *pu16Line1 = NULL;
    UINT16 *pu16DstImg = (UINT16 *)pDstImg, *pu16SrcImg = (UINT16 *)pSrcImg;

    if (1 == u32ImgBw)
    {
        for (j = 0; j < u32DstH; j++)
        {
            pu8Line0 = pu8SrcImg + weightTable[0].src00y * u32SrcW;
            pu8Line1 = pu8SrcImg + weightTable[0].src11y * u32SrcW;
            for (i = 0; i < u32DstW; i++)
            {
                f32Tmp = weightTable[i].wgt00 * pu8Line0[weightTable[i].src00x] + weightTable[i].wgt01 * pu8Line0[weightTable[i].src11x] +
                    weightTable[i].wgt10 * pu8Line1[weightTable[i].src00x] + weightTable[i].wgt11 * pu8Line1[weightTable[i].src11x];
                pu8DstImg[i] = (UINT8)(SAL_CLIP(f32Tmp, 0.0, 255.0));
            }
            pu8DstImg += u32DstW;
            weightTable += u32DstW;
        }
    }
    else if (2 == u32ImgBw)
    {
        for (j = 0; j < u32DstH; j++)
        {
            pu16Line0 = pu16SrcImg + weightTable[0].src00y * u32SrcW;
            pu16Line1 = pu16SrcImg + weightTable[0].src11y * u32SrcW;
            for (i = 0; i < u32DstW; i++)
            {
                f32Tmp = weightTable[i].wgt00 * pu16Line0[weightTable[i].src00x] + weightTable[i].wgt01 * pu16Line0[weightTable[i].src11x] +
                    weightTable[i].wgt10 * pu16Line1[weightTable[i].src00x] + weightTable[i].wgt11 * pu16Line1[weightTable[i].src11x];
                pu16DstImg[i] = (UINT16)(SAL_CLIP(f32Tmp, 0.0, 65535.0));
            }
            pu16DstImg += u32DstW;
            weightTable += u32DstW;
        }
    }
    else
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      xraw_resize_linear
 * @brief   包裹RAW数据（高低能+原子序数）缩放，线性方式 
 * @warning 暂仅支持纵向缩小，即u32DstH小于u32SrcH
 * 
 * @param   [OUT] pDstXraw 目的RAW数据Buffer
 * @param   [IN] u32DstH 目的RAW数据高
 * @param   [IN] pSrcXraw 源RAW数据Buffer
 * @param   [IN] u32SrcH 源RAW数据高
 * @param   [IN] u32Width 源RAW数据宽
 * 
 * @return  SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
static SAL_STATUS xraw_resize_linear(void *pDstXraw, unsigned int u32DstW, unsigned int u32DstH, void *pSrcXraw, unsigned int u32SrcW, unsigned int u32SrcH)
{
    UINT32 i = 0, j = 0;
    FLOAT32 f32WScale = ((FLOAT32)u32SrcW - 1.0) / ((FLOAT32)u32DstW - 1.0);
    FLOAT32 f32HScale = ((FLOAT32)u32SrcH - 1.0) / ((FLOAT32)u32DstH - 1.0);

    WEIGHT_PARAM *pWeightTable = (WEIGHT_PARAM *)SAL_memMalloc(u32DstH * u32DstW * sizeof(WEIGHT_PARAM), "xray_hostcmd", "xraw_resize");    
    WEIGHT_PARAM *pWeightLine = pWeightTable;
    if (NULL == pWeightTable)
    {
        printf("malloc for 'pWeightTable' failed, DstH: %u, DstW: %u, WEIGHT_PARAM: %zu\n", u32DstH, u32DstW, sizeof(WEIGHT_PARAM));
        return SAL_FAIL;
    }

    for (j = 0; j < u32DstH; j++)
    {
        for (i = 0; i < u32DstW; i++)
        {
            pWeightLine[i].src00x = (UINT32)(f32WScale * i);
            pWeightLine[i].src00y = (UINT32)(f32HScale * j);
            pWeightLine[i].src11x = SAL_MIN(pWeightLine[i].src00x+1, u32SrcW-1);
            pWeightLine[i].src11y = SAL_MIN(pWeightLine[i].src00y+1, u32SrcH-1);
            pWeightLine[i].wgt00 = (1.0 + (FLOAT32)pWeightLine[i].src00x - f32WScale * i) * (1.0 + (FLOAT32)pWeightLine[i].src00y - f32HScale * j);
            pWeightLine[i].wgt01 = (f32WScale * i - (FLOAT32)pWeightLine[i].src00x) * (1.0 + (FLOAT32)pWeightLine[i].src00y - f32HScale * j);
            pWeightLine[i].wgt10 = (1.0 + (FLOAT32)pWeightLine[i].src00x - f32WScale * i) * (f32HScale * j - (FLOAT32)pWeightLine[i].src00y);
            pWeightLine[i].wgt11 = 1.0 - pWeightLine[i].wgt00 - pWeightLine[i].wgt01 - pWeightLine[i].wgt10;
        }
        pWeightLine += u32DstW;
    }

    if (SAL_SOK != linear_resize(XRAW_LE_OFFSET(pDstXraw, u32DstW, u32DstH), u32DstW, u32DstH, 
                                 XRAW_LE_OFFSET(pSrcXraw, u32SrcW, u32SrcH), u32SrcW, u32SrcH, 
                                 pWeightTable, 2))
    {
        SAL_memfree(pWeightTable, "xray_hostcmd", "xraw_resize");
        return SAL_FAIL;
    }
    if (SAL_SOK != linear_resize(XRAW_HE_OFFSET(pDstXraw, u32DstW, u32DstH), u32DstW, u32DstH, 
                                 XRAW_HE_OFFSET(pSrcXraw, u32SrcW, u32SrcH), u32SrcW, u32SrcH, 
                                 pWeightTable, 2))
    {
        SAL_memfree(pWeightTable, "xray_hostcmd", "xraw_resize");
        return SAL_FAIL;
    }
    if (SAL_SOK != linear_resize(XRAW_Z_OFFSET(pDstXraw, u32DstW, u32DstH), u32DstW, u32DstH, 
                                 XRAW_Z_OFFSET(pSrcXraw, u32SrcW, u32SrcH), u32SrcW, u32SrcH, 
                                 pWeightTable, 1))
    {
        SAL_memfree(pWeightTable, "xray_hostcmd", "xraw_resize");
        return SAL_FAIL;
    }

    SAL_memfree(pWeightTable, "xray_hostcmd", "xraw_resize");
    return SAL_SOK;
}


/**
 * @fn      xsp_xraw_resize
 * @brief   包裹RAW数据缩放，当src大于dst时为缩小，当src小于dst时为放大
 * 
 * @param   [IN] enImgFmt 图像格式，当前仅支持DSP_IMG_DATFMT_LHZP
 * @param   [OUT] pDstXraw 目的RAW数据Buffer
 * @param   [IN] u32DstW 目的RAW数据宽
 * @param   [IN] u32DstH 目的RAW数据高
 * @param   [IN] pSrcXraw 源RAW数据Buffer
 * @param   [IN] u32SrcW 源RAW数据宽
 * @param   [IN] u32SrcH 源RAW数据高
 * 
 * @return  int 0-成功，-1-失败
 */
int xsp_xraw_resize(DSP_IMG_DATFMT enImgFmt, 
                    void *pDstXraw, unsigned int u32DstW, unsigned int u32DstH, 
                    void *pSrcXraw, unsigned int u32SrcW, unsigned int u32SrcH)
{
    if (DSP_IMG_DATFMT_LHZP != enImgFmt)
    {
        return -1;
    }
    if (NULL == pDstXraw || NULL == pSrcXraw)
    {
        return -1;
    }
    if (u32DstW < 2 || u32DstH < 2 || u32SrcW < 2 || u32SrcH < 2)
    {
        return -1;
    }

    if (u32SrcW == u32DstW && u32SrcH == u32DstH)
    {
        memcpy(pDstXraw, pSrcXraw, XRAW_DEZ_SIZE(u32SrcW, u32SrcH));
        return 0;
    }
    else // 使用双线性插值缩放
    {
        return xraw_resize_linear(pDstXraw, u32DstW, u32DstH, pSrcXraw, u32SrcW, u32SrcH);
    }
}


static SAL_STATUS Host_XrayResizeNraw(UINT32 chan, UINT32 *pParam, VOID *pBuf)
{
    XRAY_NRAW_RESIZE_PARAM *pstResizeNrawParam = (XRAY_NRAW_RESIZE_PARAM *)pBuf;

    CHECK_PTR_NULL(pstResizeNrawParam, SAL_FAIL);
    CHECK_PTR_NULL(pstResizeNrawParam->pDestBuf, SAL_FAIL);
    CHECK_PTR_NULL(pstResizeNrawParam->pSrcBuf, SAL_FAIL);

    XSP_LOGI("chan %u, src:buf %p, w %u h %u, dst:buf %p, w %u h %u.\n", 
             chan, pstResizeNrawParam->pSrcBuf, pstResizeNrawParam->u32SrcW, pstResizeNrawParam->u32SrcH, 
             pstResizeNrawParam->pDestBuf, pstResizeNrawParam->u32DstW, pstResizeNrawParam->u32DstH);

    if (0 != xsp_xraw_resize(DSP_IMG_DATFMT_LHZP, pstResizeNrawParam->pDestBuf, pstResizeNrawParam->u32DstW, 
                             pstResizeNrawParam->u32DstH, pstResizeNrawParam->pSrcBuf, pstResizeNrawParam->u32SrcW, pstResizeNrawParam->u32SrcH))
    {
        XSP_LOGE("xsp_xraw_resize failed, src:buf %p, w %u h %u, dst:buf %p, w %u h %u.\n", 
                 pstResizeNrawParam->pSrcBuf, pstResizeNrawParam->u32SrcW, pstResizeNrawParam->u32SrcH, 
                 pstResizeNrawParam->pDestBuf, pstResizeNrawParam->u32DstW, pstResizeNrawParam->u32DstH);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   CmdProc_XrayCmdProc
 * @brief:      X ray数据下载解析预处理模块命令处理
 * @param[in]:  HOST_CMD cmd  命令号
 * @param[in]:  UINT32 chan   通道
 * @param[in]:  VOID *pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     INT32         成功SAL_SOK，失败SAL_FALSE
 */
SAL_STATUS CmdProc_XrayCmdProc(HOST_CMD cmd, UINT32 chan, UINT32 *pParam, VOID *pBuf)
{
    SAL_STATUS ret_val = SAL_SOK;

    switch (cmd)
    {
        case HOST_CMD_XRAY_INPUT_START:
            SAL_LOGW("HOST_CMD_XRAY_INPUT_START, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XrayInputStart(chan, NULL, pBuf);
            break;

        case HOST_CMD_XRAY_INPUT_STOP:
            SAL_LOGW("HOST_CMD_XRAY_INPUT_STOP, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XrayInputStop(chan, NULL, pBuf);
            break;

        case HOST_CMD_XRAY_PLAYBACK_START:
            SAL_LOGW("HOST_CMD_XRAY_PLAKBACK_START, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XrayPlaybackStart(chan, NULL, pBuf);
            break;

        case HOST_CMD_XRAY_PLAYBACK_STOP:
            SAL_LOGW("HOST_CMD_XRAY_PLAKBACK_STOP, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XrayPlaybackStop(chan, NULL, pBuf);
            break;

        case HOST_CMD_XRAY_TRANS:
            SAL_LOGW("HOST_CMD_XRAY_TRANS, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XrayTransform(chan, NULL, pBuf);
            break;

        case HOST_CMD_XRAY_SET_PARAM:
            SAL_LOGW("HOST_CMD_XRAY_SET_PARAM, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XraySetParam(chan, NULL, pBuf);
            break;

        case HOST_CMD_XRAW_STORE:
            SAL_LOGW("HOST_CMD_XRAW_STORE, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XrayRawStoreEn(chan, pParam, NULL);
            break;

        case HOST_CMD_XRAY_CHANGE_SPEED:
            SAL_LOGW("HOST_CMD_XRAY_CHANGE_SPEED, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XrayChangeSpeed(chan, NULL, pBuf);
            break;

        case HOST_CMD_XRAY_GET_SLICE_NUM_AFTER_CLS:
            SAL_LOGI("HOST_CMD_XRAY_GET_SLICE_NUM_AFTER_CLS, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XrayGetSliceNumAfterCls(chan, pParam, NULL);
            break;

        case HOST_CMD_XRAY_SPLIT_NRAW:
            SAL_LOGT("HOST_CMD_XRAY_SPLIT_NRAW, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XraySplitNraw(chan, NULL, pBuf);
            break;

        case HOST_CMD_XRAY_SPLICE_NRAW:
            SAL_LOGD("HOST_CMD_XRAY_SPLICE_NRAW, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XraySpliceNraw(chan, NULL, pBuf);
            break;

        case HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE:
            SAL_LOGI("HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XrayGetTransBufferSize(chan, NULL, pBuf);
            break;

        case HOST_CMD_XRAY_GET_CORR_DATA:
            SAL_LOGD("HOST_CMD_XRAW_FULL_OR_ZERO_DATA, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = HOST_XrawFullOrZeroData(chan, pParam, pBuf);
            break;

        case HOST_CMD_XRAY_RESIZE_NRAW:
            SAL_LOGI("HOST_CMD_XRAY_RESIZE_NRAW, chan: %d, cmd: 0x%x !\n", chan, cmd);
            ret_val = Host_XrayResizeNraw(chan, pParam, pBuf);
            break;

        default:
            SAL_LOGE("HOST_CMD_MODULE_XRAY cmd: 0x%x, is unsupported\n", cmd);
            break;
    }

    if (SAL_FAIL == ret_val)
    {
        SAL_LOGE("HOST_CMD_MODULE_XRAY process failed, chan: %d, cmd: 0x%x\n", chan, cmd);
    }

    return ret_val;
}

