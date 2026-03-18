/**
 * @file   vpss_rk3588.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  rk3588平台相关的vpss接口
 * @author yeyanzhong
 * @date   2022.3.8
 * @note
 * @note \n History
   1.Date        :2022.3.8
     Author      :yeyanzhong
     Modification:创建文件
 */

#include <sal.h>
#include <platform_sdk.h>
#include "capbility.h"
#include "fmtConvert_rk.h"
#include "dup_hal_inter.h"

#line __LINE__ "vpss_rk3588.c"


static DUP_PLAT_OPS g_stDupPlatOps;

#define DUP_CHECK_RET(ret, ret_success, str) \
    { \
        if (ret != ret_success) \
        { \
            DUP_LOGE("%s, ret: 0x%x \n", str, ret); \
            return SAL_FAIL; \
        } \
    }


/**
 * @function    setVpssChnSize
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 setVpssChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32W, UINT32 u32H)
{
    VPSS_CHN_ATTR_S stVpssChnAttr;
    RK_S32 Ret;

    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    Ret = RK_MPI_VPSS_GetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_GetChnAttr (%d,%d)failed with %#x \n", u32GrpId, u32Chn, Ret);
        return SAL_FAIL;
    }

    if (stVpssChnAttr.u32Width == u32W && stVpssChnAttr.u32Height == u32H)
    {
        return SAL_SOK;
    }

    stVpssChnAttr.u32Width = u32W;
    stVpssChnAttr.u32Height = u32H;

    Ret = RK_MPI_VPSS_SetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_SetChnAttr failed with %#x\n", Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    getVpssChnSize
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 getVpssChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 *pU32W, UINT32 *pU32H)
{
    VPSS_CHN_ATTR_S stVpssChnAttr;
    RK_S32 Ret;

    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    Ret = RK_MPI_VPSS_GetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_GetChnAttr (%d,%d)failed with %#x\n", u32GrpId, u32Chn, Ret);
        return SAL_FAIL;
    }

    *pU32W = stVpssChnAttr.u32Width;
    *pU32H = stVpssChnAttr.u32Height;

    return SAL_SOK;
}

/**
 * @function    setVpssExtChnSize
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 setVpssExtChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32W, UINT32 u32H)
{

    DUP_LOGE("not support extChnAttr set \n");

    return SAL_SOK;
}

/**
 * @function    getVpssExtChnSize
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 getVpssExtChnSize(UINT32 u32GrpId, UINT32 u32Chn, UINT32 *pU32W, UINT32 *pU32H)
{
    DUP_LOGE("not support extChnAttr get \n");

    return SAL_SOK;
}

/**
 * @function    setVpssChnMirror
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 setVpssChnMirror(UINT32 u32GrpId, UINT32 u32Chn, UINT32 u32Mirror, UINT32 u32Flip)
{
    INT32 s32Ret = 0;
    VPSS_CHN_ATTR_S pstChnAttr;

    /* 需要做此操作 */
    s32Ret = RK_MPI_VPSS_GetChnAttr(u32GrpId, u32Chn, &pstChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        DUP_LOGE("RK_MPI_VPSS_GetChnAttr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    pstChnAttr.bMirror = u32Mirror;
    pstChnAttr.bFlip = u32Flip;

    s32Ret = RK_MPI_VPSS_SetChnAttr(u32GrpId, u32Chn, &pstChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        DUP_LOGE("RK_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    setVpssChnMirror
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 setVpssChnPixFormat(UINT32 u32GrpId, UINT32 u32Chn, SAL_VideoDataFormat enFixFormat)
{
    INT32 s32Ret = 0;
    VPSS_CHN_ATTR_S pstChnAttr;
    PIXEL_FORMAT_E enRkPixFormat;

    /* 需要做此操作 */
    s32Ret = RK_MPI_VPSS_GetChnAttr(u32GrpId, u32Chn, &pstChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        DUP_LOGE("RK_MPI_VPSS_GetChnAttr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = fmtConvert_rk_sal2rk(enFixFormat, &enRkPixFormat);
    if (s32Ret != SAL_SOK)
    {
        enRkPixFormat = RK_FMT_BGRA8888;
    }
    pstChnAttr.enPixelFormat = enRkPixFormat;

    s32Ret = RK_MPI_VPSS_SetChnAttr(u32GrpId, u32Chn, &pstChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        DUP_LOGE("RK_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    setVpssChnFps
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 setVpssChnFps(UINT32 u32GrpId, UINT32 u32VpssChn, UINT32 u32Fps)
{
    INT32 s32Ret = 0;
    VPSS_CHN_ATTR_S pstChnAttr;
    UINT32 u32SrcFps = 0;
    UINT32 u32DstFps = 0;
    
    /* 需要做此操作 */
    s32Ret = RK_MPI_VPSS_GetChnAttr(u32GrpId, u32VpssChn, &pstChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        DUP_LOGE("RK_MPI_VPSS_GetChnAttr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    u32SrcFps = (u32Fps >> 16);   /*高16位源fps，低16位目的fps*/
    u32DstFps = (u32Fps & 0xffff);

    pstChnAttr.stFrameRate.s32SrcFrameRate = u32SrcFps;
    pstChnAttr.stFrameRate.s32DstFrameRate = u32DstFps;
    
    DUP_LOGW("s32SrcFrameRate = %d s32DstFrameRate = %d \n", pstChnAttr.stFrameRate.s32SrcFrameRate, pstChnAttr.stFrameRate.s32DstFrameRate);

    s32Ret = RK_MPI_VPSS_SetChnAttr(u32GrpId, u32VpssChn, &pstChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        DUP_LOGE("RK_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    setVpssChnRotate
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 setVpssChnRotate(UINT32 u32GrpId, UINT32 u32VpssChn, UINT32 u32Rotate)
{
    INT32 s32Ret = 0;

    s32Ret = RK_MPI_VPSS_SetChnRotation(u32GrpId, u32VpssChn, u32Rotate);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_SetChnRotation failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    setVpssChnCrop
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 setVpssChnCrop(UINT32 u32GrpId, UINT32 u32VpssChn, PARAM_INFO_S *pParamInfo)
{
    INT32 s32Ret = 0;

    VPSS_CROP_INFO_S stCropParam = {0};

    stCropParam.bEnable = pParamInfo->stCrop.u32CropEnable;
    stCropParam.enCropCoordinate = VPSS_CROP_ABS_COOR;
    stCropParam.stCropRect.s32X = SAL_align(pParamInfo->stCrop.u32X, 2);
    stCropParam.stCropRect.s32Y = SAL_align(pParamInfo->stCrop.u32Y, 2);
    stCropParam.stCropRect.u32Width = SAL_align(pParamInfo->stCrop.u32W, 2);
    stCropParam.stCropRect.u32Height = SAL_align(pParamInfo->stCrop.u32H, 2);

    DUP_LOGD("grp %d,chn %d,x %d y %d w %d h %d[%d,%d]\n", u32GrpId, u32VpssChn,
             stCropParam.stCropRect.s32X, stCropParam.stCropRect.s32Y,
             stCropParam.stCropRect.u32Width, stCropParam.stCropRect.u32Height,
             stCropParam.stCropRect.s32X + (stCropParam.stCropRect.u32Width / 2),
             stCropParam.stCropRect.s32Y + (stCropParam.stCropRect.u32Height / 2));

    s32Ret = RK_MPI_VPSS_SetChnCrop(u32GrpId, u32VpssChn, &stCropParam);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_SetChnCrop failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    setVpssGrpCrop
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 setVpssGrpCrop(UINT32 u32GrpId, UINT32 u32VpssChn, PARAM_INFO_S *pParamInfo)
{
    INT32 s32Ret = 0;
    VPSS_CROP_INFO_S stCropParam = {0};

    s32Ret = RK_MPI_VPSS_GetGrpCrop(u32GrpId, &stCropParam);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_GetGrpCrop failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    stCropParam.bEnable = pParamInfo->stCrop.u32CropEnable;
    stCropParam.enCropCoordinate = VPSS_CROP_ABS_COOR;
    stCropParam.stCropRect.s32X = pParamInfo->stCrop.u32X > 0 ? pParamInfo->stCrop.u32X : 0;
    stCropParam.stCropRect.s32Y = pParamInfo->stCrop.u32Y > 0 ? pParamInfo->stCrop.u32Y : 0;
    stCropParam.stCropRect.u32Width = pParamInfo->stCrop.u32W > 64 ? pParamInfo->stCrop.u32W : 64;
    stCropParam.stCropRect.u32Height = pParamInfo->stCrop.u32H > 64 ? pParamInfo->stCrop.u32H : 64;

    DUP_LOGD("grp %d,chn %d,x %d y %d w %d h %d[%d,%d]\n", u32GrpId, u32VpssChn,
             stCropParam.stCropRect.s32X, stCropParam.stCropRect.s32Y,
             stCropParam.stCropRect.u32Width, stCropParam.stCropRect.u32Height,
             stCropParam.stCropRect.s32X + (stCropParam.stCropRect.u32Width / 2),
             stCropParam.stCropRect.s32Y + (stCropParam.stCropRect.u32Height / 2));
    s32Ret = RK_MPI_VPSS_SetGrpCrop(u32GrpId, &stCropParam);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_SetGrpCrop failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    setVpssChnMode
 * @brief       修改vpss的模式为user\auto\passthrough
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 setVpssChnMode(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    VPSS_CHN_ATTR_S stVpssChnAttr;
    RK_S32 Ret;

    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    Ret = RK_MPI_VPSS_GetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_GetChnAttr (%d,%d)failed with %#x \n", u32GrpId, u32Chn, Ret);
        return SAL_FAIL;
    }

    switch (pParamInfo->enVpssMode)
    {
        case VPSS_MODE_USER:
            stVpssChnAttr.enChnMode = VPSS_CHN_MODE_USER;
            break;
        case VPSS_MODE_AUTO:
            stVpssChnAttr.enChnMode = VPSS_CHN_MODE_AUTO;
            break;
        case VPSS_MODE_PASSTHROUGH:
            stVpssChnAttr.enChnMode = VPSS_CHN_MODE_PASSTHROUGH;
            break;
        default:
            DUP_LOGE("set Vpss Mode GrpId %d u32Chn%d failed, set %d > %d \n", u32GrpId, u32Chn, \
                pParamInfo->enVpssMode, VPSS_MODE_BUTT);
            return SAL_FAIL;
    }

    Ret = RK_MPI_VPSS_SetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_SetChnAttr failed with %#x\n", Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    setVpssChnScalingAlgorithm
 * @brief       修改vpss的缩放算法（GPU时使用有效，RGA无效，比较耗GPU性能）
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 setVpssChnScalingAlgorithm(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    VPSS_CHN_ATTR_S stVpssChnAttr;
    RK_S32 Ret;

    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    Ret = RK_MPI_VPSS_GetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_GetChnAttr (%d,%d)failed with %#x \n", u32GrpId, u32Chn, Ret);
        return SAL_FAIL;
    }

    stVpssChnAttr.bDeflicker = pParamInfo->bNewScalingAlgorithm;

    Ret = RK_MPI_VPSS_SetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_SetChnAttr failed with %#x\n", Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    getVpssChnSize
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 getVpssChnMode(UINT32 u32GrpId, UINT32 u32Chn, VPSS_MODE_E *enChnMode)
{
    VPSS_CHN_ATTR_S stVpssChnAttr;
    RK_S32 Ret;

    /* 按输出通道的个数来初始化 vpss 的输出通道属性 */
    memset(&stVpssChnAttr, 0, sizeof(VPSS_CHN_ATTR_S));
    Ret = RK_MPI_VPSS_GetChnAttr(u32GrpId, u32Chn, &stVpssChnAttr);
    if (Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_GetChnAttr (%d,%d)failed with %#x\n", u32GrpId, u32Chn, Ret);
        return SAL_FAIL;
    }
    switch (stVpssChnAttr.enChnMode)
    {
        case VPSS_CHN_MODE_USER:
            *enChnMode = VPSS_MODE_USER;
            break;
        case VPSS_CHN_MODE_AUTO:
            *enChnMode = VPSS_MODE_AUTO;
            break;
        case VPSS_CHN_MODE_PASSTHROUGH:
            *enChnMode = VPSS_MODE_PASSTHROUGH;
            break;
        default:
            DUP_LOGE("get Vpss Mode GrpId %d u32Chn%d failed, set %d \n", u32GrpId, u32Chn, stVpssChnAttr.enChnMode);
            return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    vpss_rk3588_getPhyChnNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_getPhyChnNum()
{
    return VPSS_MAX_CHN_NUM;
}

/**
 * @function    vpss_rk3588_getExtChnNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_getExtChnNum()
{
    DUP_LOGI("rk3588 has no extChn \n");
    return 0;
}

/**
 * @function    vpss_rk3588_getFrame
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_getFrame(UINT32 u32GrpId, UINT32 u32Chn, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs)
{
    INT32 iRet = 0;
    /* UINT32 u32Size = 0; */
    VIDEO_FRAME_INFO_S *pstVideoFrame = NULL;

    if (!pstFrame)
    {
        return SAL_FAIL;
    }

    pstVideoFrame = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    if (NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

    iRet = RK_MPI_VPSS_GetChnFrame(u32GrpId, u32Chn, pstVideoFrame, u32TimeoutMs);
    if (RK_SUCCESS != iRet)
    {
        /*8倍率播放会导致单帧获取时间超过200ms*/
        DUP_LOGW("Vpss %d Get vpss Chn %d Frame Failed, %x, time %d !!!\n", u32GrpId, u32Chn, iRet, u32TimeoutMs);
        return SAL_FAIL;
    }

#if 0
    char szFileName[128] = {0};
    static UINT32 count  = 0;
    snprintf(szFileName, sizeof(szFileName), "%sgrp_%d_chn_%d_num%d.yuv", "./out/", u32GrpId, u32Chn, count);
    void *pu8Buff = NULL;
    pu8Buff = RK_MPI_MB_Handle2VirAddr(pstVideoFrame->stVFrame.pMbBlk);
    if (0 == (count++ % 100))
    {
        DUP_LOGI("grp %d, chn %d, mb2VirAddr %p, vframe.pvirAddr %p, w %d,   h %d \n", u32GrpId, u32Chn, pu8Buff, pstVideoFrame->stVFrame.pVirAddr[0], pstVideoFrame->stVFrame.u32Width, pstVideoFrame->stVFrame.u32Height);
        if (count < 2000)
        {
            iRet = SAL_WriteToFile(szFileName, 0, SEEK_SET, (CHAR *)pu8Buff, pstVideoFrame->stVFrame.u32Width * pstVideoFrame->stVFrame.u32Height*3/2);
        }
    }
#endif

    pstFrame->uiDataAddr = (PhysAddr)pstVideoFrame->stVFrame.pVirAddr[0];
    pstFrame->uiDataWidth = pstVideoFrame->stVFrame.u32Width;
    pstFrame->uiDataHeight = pstVideoFrame->stVFrame.u32Height;

    return SAL_SOK;

}

/**
 * @function    vpss_rk3588_releaseFrame
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_releaseFrame(UINT32 u32GrpId, UINT32 u32Chn, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 iRet = 0;
    VIDEO_FRAME_INFO_S *pstVideoFrame = NULL;

    if (!pstFrame)
    {
        return SAL_FAIL;
    }

    pstVideoFrame = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    if (NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

    iRet = RK_MPI_VPSS_ReleaseChnFrame(u32GrpId, u32Chn, pstVideoFrame);
    if (RK_SUCCESS != iRet)
    {
        DUP_LOGE("Vpss %d release vpss Chn %d Frame Failed, %x !!!\n", u32GrpId, u32Chn, iRet);
        return SAL_FAIL;
    }

    return SAL_SOK;

}

/**
 * @function    vpss_rk3588_sendFrame
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_sendFrame(UINT32 dupChan, SYSTEM_FRAME_INFO *pstFrame, UINT32 u32TimeoutMs)
{
    INT32 iRet = 0;
    UINT32 VpssGroupID = 0;
    VIDEO_FRAME_INFO_S *pstVideoFrame = NULL;
    UINT32 u32Offset = 0;

    if (NULL == pstFrame)
    {
        DUP_LOGE("Chn %d is null!!!\n", dupChan);
        return SAL_FAIL;
    }

    pstVideoFrame = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    if (NULL == pstVideoFrame)
    {
        DUP_LOGE("pstVideoFrame is NULL \n");
        return SAL_FAIL;
    }

    /* 配置新构建一帧的起始地址与原始mb基地址的偏移量 */
    //if ((pstFrame->u32LeftOffset > 0) || (pstFrame->u32TopOffset > 0)) /* 回拉时offset有可能为0，所以这个判断语句需要去掉 */
    {
        u32Offset = pstVideoFrame->stVFrame.u32VirWidth * pstFrame->u32TopOffset + pstFrame->u32LeftOffset;
        RK_MPI_MB_SetOffset(pstVideoFrame->stVFrame.pMbBlk, u32Offset); //offset指内存的偏移
        //DUP_LOGW("       u32Offset %d \n", u32Offset);
    }

    VpssGroupID = dupChan;
    iRet = RK_MPI_VPSS_SendFrame(VpssGroupID, 0, pstVideoFrame, u32TimeoutMs);
    if (RK_SUCCESS != iRet)
    {
        DUP_LOGE("grp %d, RK_MPI_VPSS_SendFrame failed, ret:0x%x, u32LeftOffset %d, u32TopOffset %d, stVFrame w %d, h %d, virW %d, virH %d, offset %d, pixelFmt 0x%x\n", 
                 VpssGroupID, iRet, pstFrame->u32LeftOffset, pstFrame->u32TopOffset, pstVideoFrame->stVFrame.u32Width, 
                 pstVideoFrame->stVFrame.u32Height, pstVideoFrame->stVFrame.u32VirWidth, pstVideoFrame->stVFrame.u32VirHeight,
                 pstVideoFrame->stVFrame.u32VirWidth * pstFrame->u32TopOffset + pstFrame->u32LeftOffset, pstVideoFrame->stVFrame.enPixelFormat);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    vpss_rk3588_setParam
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_setParam(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    CFG_TYPE_E enCfgType = 0;
    INT32 s32Ret = 0;

    if (NULL == pParamInfo)
    {
        DUP_LOGE("pParamInfo is null!!!\n");
        return SAL_FAIL;
    }

    enCfgType = pParamInfo->enType;
    if (enCfgType <= MIN_CFG || enCfgType >= MAX_CFG)
    {
        DUP_LOGE("s32CfgType is %d, illgal \n", enCfgType);
        return SAL_FAIL;
    }

    switch (enCfgType)
    {
        case IMAGE_SIZE_CFG:
            s32Ret = setVpssChnSize(u32GrpId, u32Chn, pParamInfo->stImgSize.u32Width, pParamInfo->stImgSize.u32Height);
            break;

        case IMAGE_SIZE_EXT_CFG:
            s32Ret = setVpssExtChnSize(u32GrpId, u32Chn, pParamInfo->stImgSize.u32Width, pParamInfo->stImgSize.u32Height);
            break;

        case PIX_FORMAT_CFG:
            s32Ret = setVpssChnPixFormat(u32GrpId, u32Chn, pParamInfo->enPixFormat);
            break;

        case MIRROR_CFG:
            s32Ret = setVpssChnMirror(u32GrpId, u32Chn, pParamInfo->stMirror.u32Mirror, pParamInfo->stMirror.u32Flip);
            break;

        case FPS_CFG:
            s32Ret = setVpssChnFps(u32GrpId, u32Chn, pParamInfo->u32Fps);
            break;

        case ROTATE_CFG:
            s32Ret = setVpssChnRotate(u32GrpId, u32Chn, pParamInfo->u32Rotate);
            break;

        case CHN_CROP_CFG:
            s32Ret = setVpssChnCrop(u32GrpId, u32Chn, pParamInfo);
            break;
        case GRP_CROP_CFG:
            s32Ret = setVpssGrpCrop(u32GrpId, u32Chn, pParamInfo);
            break;
        case VPSS_CHN_MODE:
            s32Ret = setVpssChnMode(u32GrpId, u32Chn, pParamInfo);
            break;
        case SCALING_ALGORITHM_CFG:
            s32Ret = setVpssChnScalingAlgorithm(u32GrpId, u32Chn, pParamInfo);
            break;
        default:
            DUP_LOGE("enCfgType is invalid:%d\n", enCfgType);
            return SAL_FAIL;


    }

    return s32Ret;

}

/**
 * @function    vpss_rk3588_getParam
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_getParam(UINT32 u32GrpId, UINT32 u32Chn, PARAM_INFO_S *pParamInfo)
{
    CFG_TYPE_E enCfgType = 0;
    INT32 s32Ret = 0;

    if (NULL == pParamInfo)
    {
        DUP_LOGE("pParamInfo is null!!!\n");
        return SAL_FAIL;
    }

    enCfgType = pParamInfo->enType;
    if (enCfgType <= MIN_CFG || enCfgType >= MAX_CFG)
    {
        DUP_LOGE("s32CfgType is %d, illgal \n", enCfgType);
        return SAL_FAIL;
    }

    switch (enCfgType)
    {
        case IMAGE_SIZE_CFG:
            s32Ret = getVpssChnSize(u32GrpId, u32Chn, &(pParamInfo->stImgSize.u32Width), &(pParamInfo->stImgSize.u32Height));
            break;
        case IMAGE_SIZE_EXT_CFG:
            s32Ret = getVpssExtChnSize(u32GrpId, u32Chn, &(pParamInfo->stImgSize.u32Width), &(pParamInfo->stImgSize.u32Height));
            break;
        case VPSS_CHN_MODE:
            s32Ret = getVpssChnMode(u32GrpId, u32Chn, &(pParamInfo->enVpssMode));
            break;

        default:
            DUP_LOGE("enCfgType is invalid:%d\n", enCfgType);
            return SAL_FAIL;


    }

    return s32Ret;

}

/*****************************************************************************
   函 数 名  : DfrHal_drvInitVpssGroup
   功能描述  : 初始化 Vpss Group 通道
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史	   :
   1.日	  期   : 2018年07月02日
    作	  者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
static INT32 vpss_rk3588_create(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm, DUP_HAL_CHN_INIT_ATTR *pstChnInitAttr)
{
    VPSS_GRP_ATTR_S stVpssGrpAttr = {0};
    VPSS_CHN_ATTR_S stVpssChnAttr = {0};
    VPSS_MOD_PARAM_S stVpssModParam = {0};
    RK_S32 s32Ret = 0;
    UINT32 u32VpssGroup = 0;
    INT32 i = 0;
    UINT32 u32Depth = 0;
    VB_INFO_S stVbInfo = {0};
    char *pcMmzName = NULL;
    char *pcMemName = NULL;
    PIXEL_FORMAT_E enPixelFmt = 0;

    u32VpssGroup = pstHalVpssGrpPrm->uiVpssChn;
    u32Depth = pstHalVpssGrpPrm->uiChnDepth;


    /*1080x1920编码出错，图像宽高先按16自己对齐*/
    stVbInfo.u32BlkWidth = SAL_align(pstHalVpssGrpPrm->uiGrpWidth, 16);
    stVbInfo.u32BlkHeight = SAL_align(pstHalVpssGrpPrm->uiGrpHeight, 16);
    stVbInfo.u32BlkCnt = pstHalVpssGrpPrm->u32BlkCnt;
    stVbInfo.enSalPixelFmt = pstHalVpssGrpPrm->enOutputSalPixelFmt;

    if (VPSS_DEV_RGA == pstHalVpssGrpPrm->enVpssDevType)
    {
        pcMemName = "RGA_vb";
    }
    else
    {
        pcMemName = "GPU_vb";
    }

    pcMmzName = NULL; //RK没有DSP核
    if (SAL_SOK != mem_hal_vbPoolAlloc(&stVbInfo, "vpss_rk", pcMemName, pcMmzName))
    {
        DUP_LOGW("alloc buff from dsp zone fail, try anonymous\n");
        //pcMmzName = NULL;
        if (SAL_SOK != mem_hal_vbPoolAlloc(&stVbInfo, "vpss_rk", pcMemName, pcMmzName))
        {
            DUP_LOGW("mem_hal_vbPoolAlloc fail\n");
        }
    }
    pstHalVpssGrpPrm->u32PoolID = stVbInfo.u32VbPoolID;
    pstHalVpssGrpPrm->u32PoolSize = stVbInfo.u32PoolSize;

    stVpssModParam.enVpssMBSource = MB_SOURCE_USER;
    s32Ret =RK_MPI_VPSS_SetModParam(&stVpssModParam);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_SetModParam failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    /* 创建 vpss group */
    stVpssGrpAttr.u32MaxW = pstHalVpssGrpPrm->uiGrpWidth;
    stVpssGrpAttr.u32MaxH = pstHalVpssGrpPrm->uiGrpHeight;
    s32Ret = fmtConvert_rk_sal2rk(pstHalVpssGrpPrm->enInputSalPixelFmt, &enPixelFmt);
    DUP_CHECK_RET(s32Ret, SAL_SOK, "pixFmtConvert fail");
    stVpssGrpAttr.enPixelFormat = enPixelFmt;
    stVpssGrpAttr.enDynamicRange = DYNAMIC_RANGE_SDR8;
    stVpssGrpAttr.stFrameRate.s32DstFrameRate = -1;
    stVpssGrpAttr.stFrameRate.s32SrcFrameRate = -1;

    s32Ret = RK_MPI_VPSS_CreateGrp(u32VpssGroup, &stVpssGrpAttr);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_CreateGrp failed with %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VPSS_StartGrp(u32VpssGroup);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_StartGrp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    RK_MPI_VPSS_ResetGrp(u32VpssGroup);

    /* 创建 vpss chn 4个物理通道 + 2个扩展通道 */
    for (i = 0; i < VPSS_MAX_CHN_NUM; i++)
    {
        if (ENABLED != pstChnInitAttr->uiOutChnEnable[i])
        {
            continue;
        }

        SAL_clear(&stVpssChnAttr);
        stVpssChnAttr.u32Width = pstChnInitAttr->stOutChnAttr[i].uiOutChnW;
        stVpssChnAttr.u32Height = pstChnInitAttr->stOutChnAttr[i].uiOutChnH;
        stVpssChnAttr.enChnMode = VPSS_CHN_MODE_PASSTHROUGH; /* 这里如果改成USER模式，能力集里mb blk个数不能为0 */
        stVpssChnAttr.enCompressMode = COMPRESS_MODE_NONE;              /* COMPRESS_MODE_SEG; */
        stVpssChnAttr.enDynamicRange = stVpssGrpAttr.enDynamicRange;
        s32Ret = fmtConvert_rk_sal2rk(pstHalVpssGrpPrm->enOutputSalPixelFmt, &enPixelFmt);
        DUP_CHECK_RET(s32Ret, SAL_SOK, "pixFmtConvert fail");
        stVpssChnAttr.enPixelFormat = enPixelFmt;
        stVpssChnAttr.stFrameRate.s32SrcFrameRate = -1;
        stVpssChnAttr.stFrameRate.s32DstFrameRate = -1;
        stVpssChnAttr.u32Depth = u32Depth;
        stVpssChnAttr.bMirror = RK_FALSE;
        stVpssChnAttr.bFlip = RK_FALSE;
        stVpssChnAttr.enVideoFormat = VIDEO_FORMAT_LINEAR;
        stVpssChnAttr.stAspectRatio.enMode = ASPECT_RATIO_NONE;
        stVpssChnAttr.u32FrameBufCnt = 0; /* MB_SOURCE_USER attach 方式时，frm_cnt不需要配置，内部默认值4也不会生效 */
        s32Ret = RK_MPI_VPSS_SetChnAttr(u32VpssGroup, i, &stVpssChnAttr);
        if (s32Ret != RK_SUCCESS)
        {
            DUP_LOGE("RK_MPI_VPSS_SetChnAttr failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }

        s32Ret = RK_MPI_VPSS_AttachMbPool(u32VpssGroup, i, pstHalVpssGrpPrm->u32PoolID);
        if (s32Ret != RK_SUCCESS)
        {
            DUP_LOGE("RK_MPI_VPSS_AttachVbPool failed with %#x (Grp:%d,chn:%d),u32PoolId %d\n", s32Ret, u32VpssGroup, i, pstHalVpssGrpPrm->u32PoolID);
            return SAL_FAIL;
        }
        else
        {
            DUP_LOGD("RK_MPI_VPSS_AttachVbPool succeed!!! (%d,%d),u32PoolId %d\n", u32VpssGroup, i, pstHalVpssGrpPrm->u32PoolID);
        }
    }

    if ( VPSS_DEV_GPU == pstHalVpssGrpPrm->enVpssDevType)
    {
        RK_MPI_VPSS_SetVProcDev(u32VpssGroup, VIDEO_PROC_DEV_GPU);
    }
    else if (VPSS_DEV_RGA == pstHalVpssGrpPrm->enVpssDevType)
    {
        RK_MPI_VPSS_SetVProcDev(u32VpssGroup, VIDEO_PROC_DEV_RGA);
    }
    else
    {
        RK_MPI_VPSS_SetVProcDev(u32VpssGroup, VIDEO_PROC_DEV_RGA);
    }

    /* 创建拓展通道 */
    if (pstHalVpssGrpPrm->uiExtChnNum > 0)
    {
        DUP_LOGE("rk3588 not support create vpss extChn, uiExtChnNum %d \n", pstHalVpssGrpPrm->uiExtChnNum);
    }

    DUP_LOGD("Create Vpss Group Chn %d SUC!!!poolId %d\n", u32VpssGroup, pstHalVpssGrpPrm->u32PoolID);

    pstHalVpssGrpPrm->uiVpssChn = u32VpssGroup;
    return SAL_SOK;
}

/**
 * @function    vpss_rk3588_start
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_start(UINT32 u32GrpId, UINT32 u32Chn)
{
    RK_S32 s32Ret = 0;

    s32Ret = RK_MPI_VPSS_EnableChn(u32GrpId, u32Chn);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_EnableChn failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    vpss_rk3588_stop
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_stop(UINT32 u32GrpId, UINT32 u32Chn)
{
    RK_S32 s32Ret = 0;

    s32Ret = RK_MPI_VPSS_DisableChn(u32GrpId, u32Chn);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_DisableChn failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    vpss_rk3588_destroy
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_destroy(HAL_VPSS_GRP_PRM *pstHalVpssGrpPrm)
{
    RK_S32 s32Ret = 0;
    char *pcMemName = NULL;
    UINT32 i = 0;
    VB_INFO_S stVbInfo = {0};

    for (i = 0; i < VPSS_MAX_CHN_NUM; i++)
    {
        if (ENABLED != pstHalVpssGrpPrm->uiChnEnable[i])
        {
            continue;
        }

        s32Ret = RK_MPI_VPSS_DisableChn(pstHalVpssGrpPrm->uiVpssChn, i);
        if (s32Ret != RK_SUCCESS)
        {
            DUP_LOGE("RK_MPI_VPSS_DisableChn failed with %#x\n", s32Ret);
            return SAL_FAIL;
        }

        s32Ret = RK_MPI_VPSS_DetachMbPool(pstHalVpssGrpPrm->uiVpssChn, i);
        if (s32Ret != RK_SUCCESS)
        {
            DUP_LOGE("RK_MPI_VPSS_DetachVbPool failed with %#x (Grp:%d,chn:%d)\n", s32Ret, pstHalVpssGrpPrm->uiVpssChn, i);
            return SAL_FAIL;
        }
        else
        {
            DUP_LOGD("RK_MPI_VPSS_DetachVbPool succeed!!! (%d,%d)\n", pstHalVpssGrpPrm->uiVpssChn, i);
        }
    }

    s32Ret = RK_MPI_VPSS_StopGrp(pstHalVpssGrpPrm->uiVpssChn);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_StopGrp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VPSS_ResetGrp(pstHalVpssGrpPrm->uiVpssChn);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("hi_mpi_vpss_reset_grp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VPSS_DestroyGrp(pstHalVpssGrpPrm->uiVpssChn);
    if (s32Ret != RK_SUCCESS)
    {
        DUP_LOGE("RK_MPI_VPSS_DestroyGrp failed with %#x\n", s32Ret);
        return SAL_FAIL;
    }
    
    if (VPSS_DEV_RGA == pstHalVpssGrpPrm->enVpssDevType)
    {
        pcMemName = "RGA_vb";
    }
    else
    {
        pcMemName = "GPU_vb";
    }

    stVbInfo.u32VbPoolID = pstHalVpssGrpPrm->u32PoolID;
    stVbInfo.u32PoolSize = pstHalVpssGrpPrm->u32PoolSize;
    mem_hal_vbPoolFree(&stVbInfo, "vpss_rk", pcMemName);

    return SAL_SOK;
}

/**
 * @function    vpss_rk3588_init
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_init()
{
    return SAL_SOK;
}

/**
 * @function    vpss_rk3588_deinit
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 vpss_rk3588_deinit()
{
    return SAL_SOK;
}

/**
 * @function    dup_hal_register
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
const DUP_PLAT_OPS *dup_hal_register()
{
    g_stDupPlatOps.init = vpss_rk3588_init;
    g_stDupPlatOps.deinit = vpss_rk3588_deinit;
    g_stDupPlatOps.create = vpss_rk3588_create;
    g_stDupPlatOps.start = vpss_rk3588_start;
    /* g_stDupPlatOps.enableChn = vpss_rk3588_enableChn; */
    g_stDupPlatOps.stop = vpss_rk3588_stop;
    g_stDupPlatOps.destroy = vpss_rk3588_destroy;

    g_stDupPlatOps.getFrame = vpss_rk3588_getFrame;
    g_stDupPlatOps.releaseFrame = vpss_rk3588_releaseFrame;
    g_stDupPlatOps.sendFrame = vpss_rk3588_sendFrame;
    g_stDupPlatOps.setParam = vpss_rk3588_setParam;
    g_stDupPlatOps.getParam = vpss_rk3588_getParam;

    g_stDupPlatOps.getPhyChnNum = vpss_rk3588_getPhyChnNum;
    g_stDupPlatOps.getExtChnNum = vpss_rk3588_getExtChnNum;

    if (g_stDupPlatOps.init)
    {
        g_stDupPlatOps.init();
    }

    return &g_stDupPlatOps;
}

