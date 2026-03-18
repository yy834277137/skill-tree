/**
 * @file   ive_hal.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief
 * @author
 * @date    2021.6.17
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
#include "platform_sdk.h"
#include "ive_hal_inter.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          全局变量定义区                                    */
/* ========================================================================== */

static IVE_PLAT_OPS_ST g_stIvePlatOps;


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function    ive_hisi_CSC
 * @brief
 * @param[in]
 *
 * @param[out]
 *
 * @return
 */
INT32 ive_hisi_CSC(IVE_HAL_IMAGE *pstSrc, IVE_HAL_IMAGE *pstDst, IVE_HAL_MODE_CTRL *pstIveCscCtrl)
{
    INT32 ret = HI_SUCCESS;

    IVE_SRC_IMAGE_S stSrc = {0};
    IVE_DST_IMAGE_S stDst = {0};
    IVE_CSC_CTRL_S stCscCtrl = {0};
    IVE_HANDLE hIveHandle;
    HI_BOOL bBlock = HI_TRUE;
    HI_BOOL bFinish = 0;

    if (pstSrc == NULL || pstDst == NULL || pstIveCscCtrl == NULL)
    {
        SAL_ERROR("ive error prm \n");
        return SAL_FALSE;
    }

    stSrc.au64VirAddr[0] = pstSrc->u64VirAddr[0];
    stSrc.au64VirAddr[1] = pstSrc->u64VirAddr[1];
    stSrc.au64VirAddr[2] = pstSrc->u64VirAddr[2];
    stSrc.au64PhyAddr[0] = pstSrc->u64PhyAddr[0];
    stSrc.au64PhyAddr[1] = pstSrc->u64PhyAddr[1];
    stSrc.au64PhyAddr[2] = pstSrc->u64PhyAddr[2];
    stSrc.au32Stride[0] = pstSrc->u32Stride[0];
    stSrc.au32Stride[1] = pstSrc->u32Stride[1];
    stSrc.au32Stride[2] = pstSrc->u32Stride[2];
    stSrc.u32Width = pstSrc->u32Width;
    stSrc.u32Height = pstSrc->u32Height;

    stDst.au64VirAddr[0] = pstDst->u64VirAddr[0];
    stDst.au64VirAddr[1] = pstDst->u64VirAddr[1];
    stDst.au64VirAddr[2] = pstDst->u64VirAddr[2];
    stDst.au64PhyAddr[0] = pstDst->u64PhyAddr[0];
    stDst.au64PhyAddr[1] = pstDst->u64PhyAddr[1];
    stDst.au64PhyAddr[2] = pstDst->u64PhyAddr[2];
    stDst.au32Stride[0] = pstDst->u32Stride[0];
    stDst.au32Stride[1] = pstDst->u32Stride[1];
    stDst.au32Stride[2] = pstDst->u32Stride[2];
    stDst.u32Width = pstDst->u32Width;
    stDst.u32Height = pstDst->u32Height;

    switch (pstSrc->enColorType)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        {
            stSrc.enType = IVE_IMAGE_TYPE_YUV420SP;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420P:
        {
            stSrc.enType = IVE_IMAGE_TYPE_YUV420P;
            break;
        }
        case SAL_VIDEO_DATFMT_RGB24_888:
        case SAL_VIDEO_DATFMT_ARGB16_1555:
        case SAL_VIDEO_DATFMT_RGBA_8888:
        {
            break;
        }
        default:
        {
            XPACK_LOGE("HI_MPI_IVE_CSC err!\n");
            break;
        }

    }

    switch (pstDst->enColorType)
    {
        case SAL_VIDEO_DATFMT_RGB24_888:
        {
            /* IVE的图像格式 BGR顺序存储          存储在一个平面内 */
            stDst.enType = IVE_IMAGE_TYPE_U8C3_PACKAGE;
            break;
        }
        case SAL_VIDEO_DATFMT_BGR:
        {
            /* IVE的图像格式 BGR分开存储          存储在三个平面内 */
            stDst.enType = IVE_IMAGE_TYPE_U8C3_PLANAR;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        case SAL_VIDEO_DATFMT_ARGB16_1555:
        case SAL_VIDEO_DATFMT_RGBA_8888:
        {
            break;
        }
        default:
        {
            XPACK_LOGE("HI_MPI_IVE_CSC err!\n");
            break;
        }

    }

    switch (pstIveCscCtrl->u32enMode)
    {
        case IVE_CSC_PIC_BT709_YUV2RGB:
        {
            stCscCtrl.enMode = IVE_CSC_MODE_PIC_BT709_YUV2RGB;
            break;
        }
        case IVE_CSC_PIC_BT601_YUV2RGB:
        {
            stCscCtrl.enMode = IVE_CSC_MODE_PIC_BT601_YUV2RGB;
            break;
        }
        default:
        {
            break;
        }
    }

    /* 将yuv转成rgb */
    ret = HI_MPI_IVE_CSC(&hIveHandle, &stSrc, &stDst, &stCscCtrl, HI_TRUE);
    if (HI_SUCCESS != ret)
    {
        XPACK_LOGE("HI_MPI_IVE_CSC Failed! s32Ret:0x%x\n", ret);
        return ret;
    }

    ret = HI_MPI_IVE_Query(hIveHandle, &bFinish, bBlock);
    if (ret == HI_SUCCESS)
    {
        XPACK_LOGD("ive bFinish %d \n", bFinish);
    }

    return ret;
}

/**
 * @function    ive_hisi_DmaQuickCopy
 * @brief
 * @param[in]
 *
 * @param[out]
 *
 * @return
 */
INT32 ive_hisi_DmaQuickCopy(IVE_HAL_DATA *pstSrcData, IVE_HAL_DATA *pstDstData, UINT32 bCached)
{
    INT32 ret = HI_SUCCESS;

    IVE_HANDLE ive_handle;

    HI_BOOL bInstant = HI_TRUE;
    HI_BOOL bFinish = HI_FALSE;
    HI_BOOL bBlock = HI_TRUE;

    UINT32 uiQueryCnt = 0;

    IVE_DMA_CTRL_S stDmaCtrlPrm = {0};
    IVE_DATA_S stSrcData = {0};
    IVE_DATA_S stDstData = {0};

    if (NULL == pstSrcData || NULL == pstDstData)
    {
        SAL_LOGE("ptr == NULL! %p, %p \n", pstSrcData, pstDstData);
        return SAL_FAIL;
    }

    stDmaCtrlPrm.enMode = IVE_DMA_MODE_DIRECT_COPY;

    stSrcData.u64PhyAddr = pstSrcData->u64PhyAddr;
    stSrcData.u64VirAddr = pstSrcData->u64VirAddr;
    stSrcData.u32Stride = pstSrcData->u32Stride;
    stSrcData.u32Height = pstSrcData->u32Height;
    stSrcData.u32Width = pstSrcData->u32Width;

    stDstData.u64PhyAddr = pstDstData->u64PhyAddr;
    stDstData.u64VirAddr = pstDstData->u64VirAddr;
    stDstData.u32Stride = pstDstData->u32Stride;
    stDstData.u32Height = pstDstData->u32Height;
    stDstData.u32Width = pstDstData->u32Width;

    if (bCached)
    {
        ret = HI_MPI_SYS_MmzFlushCache(0, (void *)1, 0);
        if (HI_SUCCESS != ret)
        {
            SAL_LOGE("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", ret);
            return ret;
        }
    }

    ret = HI_MPI_IVE_DMA(&ive_handle, &stSrcData, &stDstData, &stDmaCtrlPrm, bInstant);
    if (HI_SUCCESS != ret)
    {
        SAL_LOGE("HI_MPI_IVE_DMA failed, error code :%#x \n", ret);
        return ret;
    }

    ret = HI_MPI_IVE_Query(ive_handle, &bFinish, bBlock);
    while ((uiQueryCnt != 10) && (HI_ERR_IVE_QUERY_TIMEOUT == ret))
    {
        usleep(100);
        ret = HI_MPI_IVE_Query(ive_handle, &bFinish, bBlock);
        uiQueryCnt++;
    }

    if (HI_SUCCESS != ret)
    {
        SAL_LOGE("Error(%#x),HI_MPI_IVE_Query failed!\n", ret);
        return ret;
    }

    if (bCached)
    {
        ret = HI_MPI_SYS_MmzFlushCache(0, (void *)1, 0);
        if (HI_SUCCESS != ret)
        {
            SAL_LOGE("Error(%#x),HI_MPI_IVE_Query failed!\n", ret);
            return ret;
        }
    }

    return ret;
}

/**
 * @function    ive_hal_QuickCopy
 * @brief       ive快速拷贝
 * @param[in]
 * @param[out]
 * @return
 */
INT32 ive_hisi_QuickCopy(SYSTEM_FRAME_INFO *pSrcSysFrame, SYSTEM_FRAME_INFO *pDstSysFrame, IVE_HAL_RECT *pstSrcRect, IVE_HAL_RECT *pstDstRect, UINT32 bCached)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiStride = 0;

    VIDEO_FRAME_INFO_S *pstSrcFrameInfo = NULL;
    VIDEO_FRAME_INFO_S *pstDstFrameInfo = NULL;

    void *pVirY = NULL;
    void *pPhyY = NULL;

    void *pVirUv = NULL;
    void *pPhyUv = NULL;

    IVE_HAL_DATA stSrcData = {0};
    IVE_HAL_DATA stDstData = {0};

    if (NULL == pSrcSysFrame
        || NULL == pDstSysFrame
        || NULL == pstSrcRect
        || NULL == pstDstRect)
    {
        SAL_LOGE("ptr == NULL! %p, %p, %p, %p \n", pSrcSysFrame, pDstSysFrame, pstSrcRect, pstDstRect);
        return SAL_FAIL;
    }

    pstSrcFrameInfo = (VIDEO_FRAME_INFO_S *)pSrcSysFrame->uiAppData;
    pstDstFrameInfo = (VIDEO_FRAME_INFO_S *)pDstSysFrame->uiAppData;

    /* Copy Y Data */
    pVirY = (void *)pstSrcFrameInfo->stVFrame.u64VirAddr[0];
    pPhyY = (void *)pstSrcFrameInfo->stVFrame.u64PhyAddr[0];

    uiStride = ALIGN_UP(pstSrcFrameInfo->stVFrame.u32Stride[0], 16);

    stSrcData.u64VirAddr = (UINT64)((INT8 *)pVirY + pstSrcRect->s32Ypos * uiStride + pstSrcRect->s32Xpos);
    stSrcData.u64PhyAddr = (UINT64)(pPhyY + pstSrcRect->s32Ypos * uiStride + pstSrcRect->s32Xpos);
    stSrcData.u32Width = pstSrcRect->u32Width;
    stSrcData.u32Height = pstSrcRect->u32Height;
    stSrcData.u32Stride = uiStride;

    pVirY = (void *)pstDstFrameInfo->stVFrame.u64VirAddr[0];
    pPhyY = (void *)pstDstFrameInfo->stVFrame.u64PhyAddr[0];

    uiStride = ALIGN_UP(pstDstFrameInfo->stVFrame.u32Stride[0], 16);

    stDstData.u64VirAddr = (UINT64)((INT8 *)pVirY + pstDstRect->s32Ypos * uiStride + pstDstRect->s32Xpos);
    stDstData.u64PhyAddr = (UINT64)(pPhyY + pstDstRect->s32Ypos * uiStride + pstDstRect->s32Xpos);
    stDstData.u32Width = pstDstRect->u32Width;
    stDstData.u32Height = pstDstRect->u32Height;
    stDstData.u32Stride = uiStride;

    s32Ret = ive_hisi_DmaQuickCopy(&stSrcData, &stDstData, bCached);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Ive Dma Quick Copy Failed! \n");
        return s32Ret;
    }

    /* Copy UV Data */
    pVirUv = (void *)pstSrcFrameInfo->stVFrame.u64VirAddr[0]    \
             + pstSrcFrameInfo->stVFrame.u32Stride[0] * pstSrcFrameInfo->stVFrame.u32Height;
    pPhyUv = (void *)pstSrcFrameInfo->stVFrame.u64PhyAddr[0]    \
             + pstSrcFrameInfo->stVFrame.u32Stride[0] * pstSrcFrameInfo->stVFrame.u32Height;

    uiStride = ALIGN_UP(pstSrcFrameInfo->stVFrame.u32Stride[0], 16);

    stSrcData.u64VirAddr = (UINT64)((INT8 *)pVirUv + pstSrcRect->s32Ypos / 2 * uiStride + pstSrcRect->s32Xpos);
    stSrcData.u64PhyAddr = (UINT64)(pPhyUv + pstSrcRect->s32Ypos / 2 * uiStride + pstSrcRect->s32Xpos);
    stSrcData.u32Width = pstSrcRect->u32Width;
    stSrcData.u32Height = pstSrcRect->u32Height / 2;
    stSrcData.u32Stride = uiStride;

    pVirUv = (void *)pstDstFrameInfo->stVFrame.u64VirAddr[0]    \
             + pstDstFrameInfo->stVFrame.u32Stride[0] * pstDstFrameInfo->stVFrame.u32Height;
    pPhyUv = (void *)pstDstFrameInfo->stVFrame.u64PhyAddr[0]    \
             + pstDstFrameInfo->stVFrame.u32Stride[0] * pstDstFrameInfo->stVFrame.u32Height;

    uiStride = ALIGN_UP(pstDstFrameInfo->stVFrame.u32Stride[0], 16);

    stDstData.u64VirAddr = (UINT64)((INT8 *)pVirUv + pstDstRect->s32Ypos / 2 * uiStride + pstDstRect->s32Xpos);
    stDstData.u64PhyAddr = (UINT64)(pPhyUv + pstDstRect->s32Ypos / 2 * uiStride + pstDstRect->s32Xpos);
    stDstData.u32Width = pstDstRect->u32Width;
    stDstData.u32Height = pstDstRect->u32Height / 2;
    stDstData.u32Stride = uiStride;

    s32Ret = ive_hisi_DmaQuickCopy(&stSrcData, &stDstData, bCached);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Ive Dma Quick Copy Failed! \n");
        return s32Ret;
    }

    return s32Ret;
}

/**
 * @function    ive_hisi_Map
 * @brief       ive map
 * @param[in]
 * @param[out]
 * @return
 */

INT32 ive_hisi_Map(IVE_HAL_IMAGE *pstSrc, IVE_HAL_IMAGE *pstDst, IVE_HAL_MEM_INFO *pstMap, IVE_HAL_MODE_CTRL *pstIveMapCtrl)
{
    INT32 ret = 0;
    HI_BOOL bBlock = HI_TRUE;
    HI_BOOL bFinish = 0;

    /* ive硬件初始化参数        */
    IVE_SRC_IMAGE_S stHiSrcImage = {0};
    IVE_DST_IMAGE_S stHiDstImage = {0};
    IVE_MAP_CTRL_S stHiMapCtrl = {0};
    IVE_SRC_MEM_INFO_S stHiMap = {0};
    IVE_HANDLE hIveHandle = 0;

    if (pstSrc->u64PhyAddr[0] == 0 || pstDst->u64PhyAddr[0] || pstMap->u64PhyAddr)
    {
        SAL_LOGE(" MAP error u64PhyAddr [ %llu  %llu  %llu]\n", pstSrc->u64PhyAddr[0], pstDst->u64PhyAddr[0], pstMap->u64PhyAddr);
        return SAL_FAIL;
    }

    if (pstSrc->u32Width % 16 != 0 || pstDst->u32Width % 16 != 0 || pstSrc->u32Height % 16 != 0 || pstDst->u32Height % 16 != 0)
    {
        SAL_LOGE("ive input not align 16 u32Width [%d %d] u32Height [%d %d]\n", pstSrc->u32Width, pstDst->u32Width, pstSrc->u32Height, pstDst->u32Height);
        return SAL_FAIL;
    }

    switch (pstIveMapCtrl->u32enMode)
    {
        case IVE_HAL_MAP_MODE_U8:
        {
            stHiMapCtrl.enMode = IVE_MAP_MODE_U8;
            break;
        }
        case IVE_HAL_MAP_MODE_U16:
        {
            stHiMapCtrl.enMode = IVE_MAP_MODE_U16;
            break;
        }
        case IVE_HAL_MAP_MODE_S16:
        {
            stHiMapCtrl.enMode = IVE_MAP_MODE_S16;
            break;
        }
        default:
        {
            SAL_LOGE(" MAP error MODE %d\n", pstIveMapCtrl->u32enMode);
            return SAL_FAIL;
        }
    }

    stHiMap.u64VirAddr = pstMap->u64VirAddr;
    stHiMap.u64PhyAddr = pstMap->u64PhyAddr;
    stHiMap.u32Size = pstMap->u32Size;

    switch (pstSrc->enColorType)
    {
        case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
        {
            stHiSrcImage.enType = stHiDstImage.enType = IVE_IMAGE_TYPE_U8C1;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        {
            stHiSrcImage.enType = stHiDstImage.enType = IVE_IMAGE_TYPE_YUV420SP;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420P:
        {
            stHiSrcImage.enType = stHiDstImage.enType = IVE_IMAGE_TYPE_YUV420P;
            break;
        }
        default:
        {
            SAL_LOGE(" MAP error tpye %d\n", pstSrc->enColorType);
            return SAL_FAIL;
        }
    }

    stHiSrcImage.au64VirAddr[0] = (UINT64)pstSrc->u64VirAddr[0];
    stHiSrcImage.au64PhyAddr[0] = (UINT64)pstSrc->u64PhyAddr[0];
    stHiSrcImage.au32Stride[0] = pstSrc->u32Stride[0];
    stHiSrcImage.u32Width = pstSrc->u32Width;
    stHiSrcImage.u32Height = pstSrc->u32Height;

    stHiDstImage.au64VirAddr[0] = (UINT64)pstDst->u64VirAddr[0];
    stHiDstImage.au64PhyAddr[0] = (UINT64)pstDst->u64PhyAddr[0];
    stHiDstImage.au32Stride[0] = pstDst->u32Stride[0];
    stHiDstImage.u32Width = pstDst->u32Width;
    stHiDstImage.u32Height = pstDst->u32Height;

    ret = HI_MPI_IVE_Map(&hIveHandle, &stHiSrcImage, &stHiMap, &stHiDstImage, &stHiMapCtrl, SAL_TRUE);
    if (ret != HI_SUCCESS)
    {
        XPACK_LOGE("ive failed %d \n", ret);
        return SAL_SOK;
    }

    ret = HI_MPI_IVE_Query(hIveHandle, &bFinish, bBlock);
    if (ret == HI_SUCCESS)
    {
        XPACK_LOGD("ive bFinish %d \n", bFinish);
    }

    return SAL_SOK;

}

/**
 * @function	ive_hal_register
 * @brief		对平台相关的内存申请函数进行注册
 * @param[in]
 * @param[out]
 * @return
 */
IVE_PLAT_OPS_ST *ive_hal_register(void)
{
    g_stIvePlatOps.pIveCSC = ive_hisi_CSC;
    g_stIvePlatOps.pIveQuickCopy = ive_hisi_QuickCopy;
    g_stIvePlatOps.pIveMap = ive_hisi_Map;

    return &g_stIvePlatOps;
}

