/**
 * @file   ive_ssv5.c
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
INT32 ive_ssv5_CSC(IVE_HAL_IMAGE *pstSrc, IVE_HAL_IMAGE *pstDst, IVE_HAL_MODE_CTRL *pstIveCscCtrl)
{
    INT32 ret = TD_SUCCESS;
    td_bool is_instant = TD_TRUE;
    ot_svp_src_img stSrc = {0};
    ot_svp_dst_img stDst = {0};
    ot_ive_csc_ctrl stCscCtrl = {0};
    ot_ive_handle hIveHandle;

    if (pstSrc == NULL || pstDst == NULL || pstIveCscCtrl == NULL)
    {
        SAL_ERROR("ive error prm \n");
        return SAL_FALSE;
    }

    stSrc.phys_addr[0] = pstSrc->u64PhyAddr[0];
    stSrc.phys_addr[1] = pstSrc->u64PhyAddr[1];
    stSrc.phys_addr[2] = pstSrc->u64PhyAddr[2];
    stSrc.stride[0] = pstSrc->u32Stride[0];
    stSrc.stride[1] = pstSrc->u32Stride[1];
    stSrc.stride[2] = pstSrc->u32Stride[2];
    stSrc.width = pstSrc->u32Width;
    stSrc.height = pstSrc->u32Height;

    stDst.phys_addr[0] = pstDst->u64PhyAddr[0];
    stDst.phys_addr[1] = pstDst->u64PhyAddr[1];
    stDst.phys_addr[2] = pstDst->u64PhyAddr[2];
    stDst.stride[0] = pstDst->u32Stride[0];
    stDst.stride[1] = pstDst->u32Stride[1];
    stDst.stride[2] = pstDst->u32Stride[2];
    stDst.width = pstDst->u32Width;
    stDst.height = pstDst->u32Height;

    switch (pstSrc->enColorType)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        {
            stSrc.type = OT_SVP_IMG_TYPE_YUV420SP;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420P:
        {
            stSrc.type = OT_SVP_IMG_TYPE_YUV420P;
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
            break;
        }

    }

    switch (pstDst->enColorType)
    {
        case SAL_VIDEO_DATFMT_RGB24_888:
        {
            /* IVE的图像格式 BGR顺序存储          存储在一个平面内 */
            stSrc.type = OT_SVP_IMG_TYPE_U8C3_PACKAGE;
            break;
        }
        case SAL_VIDEO_DATFMT_BGR:
        {
            /* IVE的图像格式 BGR分开存储          存储在三个平面内 */
            stSrc.type = OT_SVP_IMG_TYPE_U8C3_PLANAR;
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
            break;
        }

    }

    switch (pstIveCscCtrl->u32enMode)
    {
        case IVE_CSC_PIC_BT709_YUV2RGB:
        {
            stCscCtrl.mode = OT_IVE_CSC_MODE_PIC_BT709_YUV_TO_RGB;
            break;
        }
        case IVE_CSC_PIC_BT601_YUV2RGB:
        {
            stCscCtrl.mode = OT_IVE_CSC_MODE_PIC_BT601_YUV_TO_RGB;
            break;
        }
        default:
        {
            break;
        }
    }

    /* 将yuv转成rgb */
    ret = ss_mpi_ive_csc(&hIveHandle, &stSrc, &stDst, &stCscCtrl,is_instant);
    if (TD_SUCCESS != ret)
    {
        SAL_LOGE("ss_mpi_ive_csc Failed! s32Ret:0x%x\n", ret);
        return ret;
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
INT32 ive_ssv5_DmaQuickCopy(IVE_HAL_DATA *pstSrcData, IVE_HAL_DATA *pstDstData, UINT32 bCached)
{
    INT32 ret = TD_SUCCESS;

    ot_ive_handle ive_handle;

    td_bool bInstant = TD_TRUE;
    td_bool bFinish = TD_FALSE;
    td_bool bBlock = TD_TRUE;

    UINT32 uiQueryCnt = 0;

    ot_ive_dma_ctrl stDmaCtrlPrm = {0};
    ot_svp_data stSrcData = {0};
    ot_svp_dst_data stDstData = {0};

    if (NULL == pstSrcData || NULL == pstDstData)
    {
        SAL_LOGE("ptr == NULL! %p, %p \n", pstSrcData, pstDstData);
        return SAL_FAIL;
    }

    stDmaCtrlPrm.mode = OT_IVE_DMA_MODE_DIRECT_COPY;

    stSrcData.phys_addr = pstSrcData->u64PhyAddr;
    stSrcData.virt_addr = pstSrcData->u64VirAddr;
    stSrcData.stride = pstSrcData->u32Stride;
    stSrcData.height = pstSrcData->u32Height;
    stSrcData.width = pstSrcData->u32Width;

    stDstData.phys_addr = pstDstData->u64PhyAddr;
    stDstData.virt_addr = pstDstData->u64VirAddr;
    stDstData.stride = pstDstData->u32Stride;
    stDstData.height = pstDstData->u32Height;
    stDstData.width = pstDstData->u32Width;

    ret = ss_mpi_ive_dma(&ive_handle,  &stSrcData, &stDstData, &stDmaCtrlPrm, bInstant);
    if (TD_SUCCESS != ret)
    {
        SAL_LOGE("HI_MPI_IVE_DMA failed, error code :%#x \n", ret);
        return ret;
    }
    ret = ss_mpi_ive_query(ive_handle, &bFinish, bBlock) ;
    while ((uiQueryCnt != 10) && (OT_ERR_IVE_QUERY_TIMEOUT == ret))
    {
        usleep(100);
        ret = ss_mpi_ive_query(ive_handle, &bFinish, bBlock);
        uiQueryCnt++;
    }

    if (TD_SUCCESS != ret)
    {
        SAL_LOGE("Error(%#x),HI_MPI_IVE_Query failed!\n", ret);
        return ret;
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
INT32 ive_ssv5_QuickCopy(SYSTEM_FRAME_INFO *pSrcSysFrame, SYSTEM_FRAME_INFO *pDstSysFrame, IVE_HAL_RECT *pstSrcRect, IVE_HAL_RECT *pstDstRect, UINT32 bCached)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiStride = 0;

    ot_video_frame_info *pstSrcFrameInfo = NULL;
    ot_video_frame_info *pstDstFrameInfo = NULL;

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

    pstSrcFrameInfo = (ot_video_frame_info *)pSrcSysFrame->uiAppData;
    pstDstFrameInfo = (ot_video_frame_info *)pDstSysFrame->uiAppData;

    /* Copy Y Data */
    pVirY = (void *)pstSrcFrameInfo->video_frame.virt_addr[0];
    pPhyY = (void *)pstSrcFrameInfo->video_frame.phys_addr[0];

	if (bCached)
    {
        s32Ret = ss_mpi_sys_flush_cache(pstSrcFrameInfo->video_frame.phys_addr[0], (void *)pstSrcFrameInfo->video_frame.virt_addr[0], pstSrcFrameInfo->video_frame.stride[0] * pstSrcFrameInfo->video_frame.height * 3/2);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_LOGE("Error(%#x), phy %p, vir %p, s %d, h %d, ss_mpi_sys_flush_cache failed!\n", s32Ret,
				      (VOID *)pstSrcFrameInfo->video_frame.phys_addr[0], (VOID *)pstSrcFrameInfo->video_frame.virt_addr[0],
				      pstSrcFrameInfo->video_frame.stride[0], pstSrcFrameInfo->video_frame.height);
            return s32Ret;
        }
    }

    uiStride = OT_ALIGN_UP(pstSrcFrameInfo->video_frame.stride[0], 16);

    stSrcData.u64VirAddr = (UINT64)((INT8 *)pVirY + pstSrcRect->s32Ypos * uiStride + pstSrcRect->s32Xpos);
    stSrcData.u64PhyAddr = (UINT64)(pPhyY + pstSrcRect->s32Ypos * uiStride + pstSrcRect->s32Xpos);
    stSrcData.u32Width = pstSrcRect->u32Width;
    stSrcData.u32Height = pstSrcRect->u32Height;
    stSrcData.u32Stride = uiStride;

    pVirY = (void *)pstDstFrameInfo->video_frame.virt_addr[0];
    pPhyY = (void *)pstDstFrameInfo->video_frame.phys_addr[0];

    uiStride = OT_ALIGN_UP(pstDstFrameInfo->video_frame.stride[0], 16);

    stDstData.u64VirAddr = (UINT64)((INT8 *)pVirY + pstDstRect->s32Ypos * uiStride + pstDstRect->s32Xpos);
    stDstData.u64PhyAddr = (UINT64)(pPhyY + pstDstRect->s32Ypos * uiStride + pstDstRect->s32Xpos);
    stDstData.u32Width = pstDstRect->u32Width;
    stDstData.u32Height = pstDstRect->u32Height;
    stDstData.u32Stride = uiStride;

    s32Ret = ive_ssv5_DmaQuickCopy(&stSrcData, &stDstData, bCached);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Ive Dma Quick Copy Failed! \n");
        return s32Ret;
    }

    /* Copy UV Data */
    pVirUv = (void *)pstSrcFrameInfo->video_frame.virt_addr[0]    \
             + pstSrcFrameInfo->video_frame.stride[0] * pstSrcFrameInfo->video_frame.height;
    pPhyUv = (void *)pstSrcFrameInfo->video_frame.phys_addr[0]    \
             + pstSrcFrameInfo->video_frame.stride[0] * pstSrcFrameInfo->video_frame.height;

    uiStride = OT_ALIGN_UP(pstSrcFrameInfo->video_frame.stride[0], 16);

    stSrcData.u64VirAddr = (UINT64)((INT8 *)pVirUv + pstSrcRect->s32Ypos / 2 * uiStride + pstSrcRect->s32Xpos);
    stSrcData.u64PhyAddr = (UINT64)(pPhyUv + pstSrcRect->s32Ypos / 2 * uiStride + pstSrcRect->s32Xpos);
    stSrcData.u32Width = pstSrcRect->u32Width;
    stSrcData.u32Height = pstSrcRect->u32Height / 2;
    stSrcData.u32Stride = uiStride;

    pVirUv = (void *)pstDstFrameInfo->video_frame.virt_addr[0]    \
             + pstDstFrameInfo->video_frame.stride[0] * pstDstFrameInfo->video_frame.height;
    pPhyUv = (void *)pstDstFrameInfo->video_frame.phys_addr[0]    \
             + pstDstFrameInfo->video_frame.stride[0] * pstDstFrameInfo->video_frame.height;

    uiStride = OT_ALIGN_UP(pstDstFrameInfo->video_frame.stride[0], 16);

    stDstData.u64VirAddr = (UINT64)((INT8 *)pVirUv + pstDstRect->s32Ypos / 2 * uiStride + pstDstRect->s32Xpos);
    stDstData.u64PhyAddr = (UINT64)(pPhyUv + pstDstRect->s32Ypos / 2 * uiStride + pstDstRect->s32Xpos);
    stDstData.u32Width = pstDstRect->u32Width;
    stDstData.u32Height = pstDstRect->u32Height / 2;
    stDstData.u32Stride = uiStride;

    s32Ret = ive_ssv5_DmaQuickCopy(&stSrcData, &stDstData, bCached);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Ive Dma Quick Copy Failed! \n");
        return s32Ret;
    }
	
	if (bCached)
    {
        s32Ret = ss_mpi_sys_flush_cache(pstDstFrameInfo->video_frame.phys_addr[0], (void *)pstDstFrameInfo->video_frame.virt_addr[0], pstDstFrameInfo->video_frame.stride[0] * pstDstFrameInfo->video_frame.height * 3/2);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_LOGE("Error(%#x),HI_MPI_IVE_Query failed!\n", s32Ret);
            return s32Ret;
        }
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
    td_bool bBlock = TD_TRUE;
    td_bool bFinish = 0;

    /* ive硬件初始化参数        */
    ot_svp_src_img stHiSrcImage = {0};
    ot_svp_dst_img stHiDstImage = {0};
    ot_ive_map_ctrl stHiMapCtrl = {0};
    ot_svp_src_mem_info stHiMap = {0};
    ot_ive_handle hIveHandle = 0;

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
            stHiMapCtrl.mode = OT_IVE_MAP_MODE_U8;
            break;
        }
        case IVE_HAL_MAP_MODE_U16:
        {
            stHiMapCtrl.mode = OT_IVE_MAP_MODE_U16;
            break;
        }
        case IVE_HAL_MAP_MODE_S16:
        {
            stHiMapCtrl.mode = OT_IVE_MAP_MODE_S16;
            break;
        }
        default:
        {
            SAL_LOGE(" MAP error MODE %d\n", pstIveMapCtrl->u32enMode);
            return SAL_FAIL;
        }
    }

    stHiMap.phys_addr = pstMap->u64VirAddr;  
    stHiMap.virt_addr = pstMap->u64PhyAddr;
    stHiMap.size = pstMap->u32Size;

    switch (pstSrc->enColorType)
    {
        case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
        {
            stHiSrcImage.type = stHiDstImage.type = OT_SVP_IMG_TYPE_U8C1;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        {
            stHiSrcImage.type = stHiDstImage.type = OT_SVP_IMG_TYPE_YUV420SP;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420P:
        {
            stHiSrcImage.type = stHiDstImage.type = OT_SVP_IMG_TYPE_YUV420P;
            break;
        }
        default:
        {
            SAL_LOGE(" MAP error tpye %d\n", pstSrc->enColorType);
            return SAL_FAIL;
        }
    }
    
    stHiSrcImage.phys_addr[0] = (UINT64)pstSrc->u64VirAddr[0];
    stHiSrcImage.virt_addr[0] = (UINT64)pstSrc->u64PhyAddr[0];
    stHiSrcImage.stride[0] = pstSrc->u32Stride[0];
    stHiSrcImage.width = pstSrc->u32Width;
    stHiSrcImage.height = pstSrc->u32Height;

    stHiDstImage.phys_addr[0] = (UINT64)pstDst->u64VirAddr[0];
    stHiDstImage.virt_addr[0] = (UINT64)pstDst->u64PhyAddr[0];
    stHiDstImage.stride[0] = pstDst->u32Stride[0];
    stHiDstImage.width = pstDst->u32Width;
    stHiDstImage.height = pstDst->u32Height;

    ret = ss_mpi_ive_map(&hIveHandle, &stHiSrcImage, &stHiMap, &stHiDstImage, &stHiMapCtrl, SAL_TRUE);
    if (ret != TD_SUCCESS)
    {
        XPACK_LOGE("ive failed %d \n", ret);
        return SAL_SOK;
    }

    ret = ss_mpi_ive_query(hIveHandle, &bFinish, bBlock);
    if (ret == TD_SUCCESS)
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
    g_stIvePlatOps.pIveCSC = ive_ssv5_CSC;
    g_stIvePlatOps.pIveQuickCopy = ive_ssv5_QuickCopy;

    return &g_stIvePlatOps;
}


