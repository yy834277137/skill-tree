/**
 * @file   sys_rk.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  sys注册
 * @author liuxianying
 * @date   2022.03.08
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */

#include "platform_sdk.h"
#include "sys_hal_inter.h"
#include "fmtConvert_rk.h"


static SYS_PLAT_OPS g_stSysPlatOps;

/*****************************************************************************
                            宏定义
*****************************************************************************/


/**
 * @function   sys_rk_deInit
 * @brief     去初始化sys
 * @param[in]  void
 * @param[out] None
 * @return    INT32
 */
INT32 sys_rk_deInit(void)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = RK_MPI_SYS_Exit();
    if (s32Ret != SAL_SOK)
    {
        SYS_LOGE("sys_rk_deInit fail\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   sys_rk_init
 * @brief     初始化sys
 * @param[in]  UINT32 u32ViChnNum 输入编号
 * @param[out] None
 * @return    INT32
 */
static INT32 sys_rk_init(UINT32 u32ViChnNum)
{
    INT32 sRet = SAL_SOK;
    RK_U64 p64CurPts = 0;
    RK_CHAR cModName[16] = "cmpi";
    LOG_LEVEL_CONF_S stLogLevel;

    sRet = RK_MPI_SYS_Init();   /*先使用sdk提供设备树方式进行分配内存*/
    if (SAL_SOK != sRet)
    {
        SYS_LOGE("sys_rk_memInit fail\n");
        return SAL_FAIL;
    }

    p64CurPts = SAL_getCurUs();

    sRet = RK_MPI_SYS_InitPTSBase(p64CurPts);   /* 初始化rk mpi时间戳 */
    if (SAL_SOK != sRet)
    {
        SYS_LOGE("sys_rk_InitPTSBase fail, ret %#x\n", sRet);
        return SAL_FAIL;
    }

    stLogLevel.enModId = RK_ID_CMPI; /* 所有模块 */
    #ifdef _DBG_MODE_
    stLogLevel.s32Level = 0; /* 打印影响较大，debug下，也先关闭rockit库打印*/
    #else
    stLogLevel.s32Level = 0;
    #endif
    //stLogLevel.s32Level = 0;
    memcpy(stLogLevel.cModName, cModName, sizeof(cModName));
    RK_MPI_LOG_SetLevelConf(&stLogLevel);

    /* SYS_LOGI("common init ok\n"); */

    return sRet;
}

/**
 * @function   sys_rk_init
 * @brief     获取当前帧时间戳
 * @param[in]  UINT64 pCurPts 时间戳
 * @param[out] None
 * @return    INT32
 */
INT32 sys_rk_getPts(UINT64 *pCurPts)
{
    INT32 s32Ret = SAL_SOK;
    RK_U64 p64CurPts = 0;

    if (NULL == pCurPts)
    {
        SYS_LOGE("error\n");
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_SYS_GetCurPTS(&p64CurPts);
    if (s32Ret != SAL_SOK)
    {
        VDEC_LOGE("ss_mpi_sys_get_cur_pts error 0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    *pCurPts = (UINT64)p64CurPts;

    return SAL_SOK;
}

/**
 * @function   sys_rk_bind
 * @brief     数据源到数据接收者绑定的接口
 * @param[in]  SYSTEM_MOD_ID_E uiSrcModId 源模块ID
 *             UINT32 uiSrcDevId  数据源设备号
 *             UINT32 uiSrcChn    数据源通道号
 *             SYSTEM_MOD_ID_E uiDstModId 接收模块ID
 *             UINT32 uiDstDevId  数据接收设备号
 *             UINT32 uiDstChn    数据接收通道号
 * @param[out] None
 * @return    INT32
 */
INT32 sys_rk_bind(SYSTEM_MOD_ID_E uiSrcModId, UINT32 uiSrcDevId, UINT32 uiSrcChn, SYSTEM_MOD_ID_E uiDstModId, UINT32 uiDstDevId, UINT32 uiDstChn, UINT32 uiBind)
{
    INT32 s32Ret = SAL_SOK;
    MPP_CHN_S stSrcChn;
    MPP_CHN_S stDestChn;

    memset(&stSrcChn, 0, sizeof(MPP_CHN_S));
    memset(&stDestChn, 0, sizeof(MPP_CHN_S));

    if (SYSTEM_MOD_ID_DUP == uiSrcModId)
    {
        stSrcChn.enModId = RK_ID_VPSS;
        stSrcChn.s32DevId = uiSrcDevId;
        stSrcChn.s32ChnId = uiSrcChn;
    }
    else if (SYSTEM_MOD_ID_CAPT == uiSrcModId)
    {
        stSrcChn.enModId = RK_ID_VI;
        stSrcChn.s32DevId = uiSrcDevId;
        stSrcChn.s32ChnId = 0;

    }
    else if (SYSTEM_MOD_ID_VDEC == uiSrcModId)
    {
        stSrcChn.enModId = RK_ID_VDEC;
        stSrcChn.s32DevId = 0;
        stSrcChn.s32ChnId = uiSrcChn;

    }
    else
    {
        SYS_LOGE("uiSrcModId %d not support \n", uiSrcModId);
        return SAL_FAIL;
    }

    if (SYSTEM_MOD_ID_DUP == uiDstModId)
    {
        stDestChn.enModId = RK_ID_VPSS;
        stDestChn.s32DevId = uiDstDevId;
        stDestChn.s32ChnId = uiDstChn;
    }
    else if (SYSTEM_MOD_ID_DISP == uiDstModId)
    {
        stDestChn.enModId = RK_ID_VO;
        stDestChn.s32DevId = uiDstDevId;
        stDestChn.s32ChnId = uiDstChn;

    }
    else if (SYSTEM_MOD_ID_VENC == uiDstModId)
    {
        stDestChn.enModId = RK_ID_VENC;
        stDestChn.s32DevId = 0;
        stDestChn.s32ChnId = uiDstChn;

    }
    else
    {
        SYS_LOGE("uiDstModId %d not support \n", uiDstModId);
        return SAL_FAIL;
    }

    if (1 == uiBind)
    {
        s32Ret = RK_MPI_SYS_Bind(&stSrcChn, &stDestChn);
        if (s32Ret != SAL_SOK)
        {
            SYS_LOGE("SRC M-%d_D-%d_C-%d Bind to  M-%d_D-%d_C-%d failed with %#x!\n",
                     stSrcChn.enModId, stSrcChn.s32DevId, stSrcChn.s32ChnId, stDestChn.enModId, stDestChn.s32DevId, stDestChn.s32ChnId, s32Ret);
            return SAL_FAIL;
        }

        SYS_LOGD("SRC M-%d_D-%d_C-%d Bind to  M-%d_D-%d_C-%d failed with %#x!\n",
                 stSrcChn.enModId, stSrcChn.s32DevId, stSrcChn.s32ChnId, stDestChn.enModId, stDestChn.s32DevId, stDestChn.s32ChnId, s32Ret);
    }
    else
    {

        s32Ret = RK_MPI_SYS_UnBind(&stSrcChn, &stDestChn);
        if (s32Ret != SAL_SOK)
        {
            SYS_LOGE("SRC M-%d_D-%d_C-%d unBind from Venc M-%d_D-%d_C-%d failed with %#x!\n",
                     stSrcChn.enModId, stSrcChn.s32DevId, stSrcChn.s32ChnId, stDestChn.enModId, stDestChn.s32DevId, stDestChn.s32ChnId, s32Ret);
            return SAL_FAIL;
        }

        SYS_LOGD("SRC M-%d_D-%d_C-%d unBind from Venc M-%d_D-%d_C-%d failed with %#x!\n",
                 stSrcChn.enModId, stSrcChn.s32DevId, stSrcChn.s32ChnId, stDestChn.enModId, stDestChn.s32DevId, stDestChn.s32ChnId, s32Ret);
    }

    return SAL_SOK;
}

/**
 * @function   sys_rk_allocVideoFrameInfoSt
 * @brief     申请平台帧信息
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrameInfo 帧信息
 * @param[out] None
 * @return    INT32
 */
INT32 sys_rk_allocVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    VIDEO_FRAME_INFO_S *pOutFrm = NULL;

    pOutFrm = (VIDEO_FRAME_INFO_S *)SAL_memMalloc(sizeof(VIDEO_FRAME_INFO_S), "sys_rk", "video_frame");
    if (!pOutFrm)
    {
        SYS_LOGE("alloc mem[mod:sys_rk, name:video_frame] fail \n");
        return SAL_FAIL;
    }

    memset(pOutFrm, 0x00, sizeof(VIDEO_FRAME_INFO_S));
    pstSystemFrameInfo->uiAppData = (PhysAddr)pOutFrm;
    /* pstSystemFrameInfo->uiDataAddr = (PhysAddr)pOutFrm->stVFrame.u64VirAddr[0]; */

    return SAL_SOK;
}

/**
 * @function   sys_rk_releaseVideoFrameInfoSt
 * @brief     释放平台帧信息
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrameInfo 帧信息
 * @param[out] None
 * @return    INT32
 */
INT32 sys_rk_releaseVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    VIDEO_FRAME_INFO_S *pOutFrm = (VIDEO_FRAME_INFO_S *)pstSystemFrameInfo->uiAppData;

    SAL_memfree(pOutFrm, "sys_rk", "video_frame");
    pstSystemFrameInfo->uiAppData = 0;

    return SAL_SOK;
}

/**
 * @function   sys_rk_buildVideoFrame
 * @brief     构建平台帧信息
 * @param[in]  SAL_VideoFrameBuf *pVideoFrame 帧信息
 *             SYSTEM_FRAME_INFO *pstSystemFrameInfo 帧信息
 * @param[out] None
 * @return    INT32
 */
INT32 sys_rk_buildVideoFrame(SAL_VideoFrameBuf *pVideoFrame, SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    PIXEL_FORMAT_E enPixelFormat = RK_FMT_YUV420SP;
    VIDEO_FRAME_INFO_S *pVideoFrameInfo = (VIDEO_FRAME_INFO_S *)pstSystemFrameInfo->uiAppData;

    if (!pVideoFrameInfo)
    {
        SYS_LOGE("pVideoFrameInfo is NULL \n");
        return SAL_FAIL;
    }

    fmtConvert_rk_sal2rk(pVideoFrame->frameParam.dataFormat, &enPixelFormat);

    pstSystemFrameInfo->u32LeftOffset = pVideoFrame->frameParam.crop.left;
    pstSystemFrameInfo->u32TopOffset = pVideoFrame->frameParam.crop.top;
    pVideoFrameInfo->stVFrame.u32Width = pVideoFrame->frameParam.width;
    pVideoFrameInfo->stVFrame.u32Height = pVideoFrame->frameParam.height;

    pVideoFrameInfo->stVFrame.enField = VIDEO_FIELD_FRAME;
    pVideoFrameInfo->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    pVideoFrameInfo->stVFrame.enPixelFormat = enPixelFormat;

    pVideoFrameInfo->stVFrame.pVirAddr[0] = (RK_VOID *)pVideoFrame->virAddr[0];
    pVideoFrameInfo->stVFrame.pVirAddr[1] = (RK_VOID *)pVideoFrame->virAddr[1];

    pVideoFrameInfo->stVFrame.u32VirWidth = pVideoFrame->stride[0]; /* RK里指像素的跨距，不是内存的虚宽，所以跟ARGB还是YUV像素格式没有关系 */
    /*xpack里把大的缓存的高度作为虚高构建图像帧，其他模块vertStride[0]可赋为0即把实际图像高度作为虚高 */
    pVideoFrameInfo->stVFrame.u32VirHeight = (pVideoFrame->vertStride[0] > 0) ? pVideoFrame->vertStride[0] : pVideoFrame->frameParam.height;
    pVideoFrameInfo->stVFrame.u32TimeRef = (RK_U32)pVideoFrame->frameNum;
    pVideoFrameInfo->stVFrame.u64PTS = (RK_U64)pVideoFrame->pts;

    if (0 == pVideoFrame->vbBlk)
    {
        /* SYS_LOGE("pVideoFrame->vbBlk  is NULL \n"); */
        pVideoFrameInfo->stVFrame.pMbBlk = RK_MPI_MB_VirAddr2Handle((RK_VOID *)pVideoFrame->virAddr[0]);
    }
    else
    {
        pVideoFrameInfo->stVFrame.pMbBlk = (MB_BLK)pVideoFrame->vbBlk;  /* pMbBlk */
    }

    pVideoFrameInfo->stVFrame.u64PrivateData = (PhysAddr)pVideoFrame->privateDate;

    pstSystemFrameInfo->uiDataAddr = pVideoFrame->virAddr[0];
    pstSystemFrameInfo->uiDataWidth = pVideoFrameInfo->stVFrame.u32Width;
    pstSystemFrameInfo->uiDataHeight = pVideoFrameInfo->stVFrame.u32Height;


    return SAL_SOK;
}

/**
 * @function   sys_rk_getVideoFrameInfo
 * @brief     获取平台帧信息
 * @param[in]  SAL_VideoFrameBuf *pVideoFrame 帧信息
 *             SYSTEM_FRAME_INFO *pstSystemFrameInfo 帧信息
 * @param[out] None
 * @return    INT32
 */
static INT32 sys_rk_getVideoFrameInfo(SYSTEM_FRAME_INFO *pstSystemFrameInfo, SAL_VideoFrameBuf *pVideoFrame)
{
    VIDEO_FRAME_INFO_S *pVideoFrameInfo = (VIDEO_FRAME_INFO_S *)pstSystemFrameInfo->uiAppData;
    SAL_VideoDataFormat enSalPixFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;

    memset(pVideoFrame, 0x0, sizeof(SAL_VideoFrameBuf));

    fmtConvert_rk_rk2sal(pVideoFrameInfo->stVFrame.enPixelFormat, &enSalPixFmt);

    pVideoFrame->frameParam.width = pVideoFrameInfo->stVFrame.u32Width;
    pVideoFrame->frameParam.height = pVideoFrameInfo->stVFrame.u32Height;
    pVideoFrame->frameParam.dataFormat = enSalPixFmt;
/* VideoFrame->phyAddr[0] = (PhysAddr)pVideoFrameInfo->stVFrame.phys_addr[0]; */
/* VideoFrame->phyAddr[1] = (PhysAddr)pVideoFrameInfo->stVFrame.phys_addr[1]; */
/* VideoFrame->phyAddr[2] = pVideoFrame->phyAddr[1]; */

    if ((PhysAddr)pVideoFrameInfo->stVFrame.pVirAddr[0] != 0x00)
    {
        pVideoFrame->virAddr[0] = (PhysAddr)pVideoFrameInfo->stVFrame.pVirAddr[0];
        pVideoFrame->virAddr[1] = (PhysAddr)pVideoFrameInfo->stVFrame.pVirAddr[1];
        pVideoFrame->virAddr[2] = pVideoFrame->virAddr[1];
    }
    else if (pstSystemFrameInfo->uiDataAddr != 0x00)
    {
        pVideoFrame->virAddr[0] = pstSystemFrameInfo->uiDataAddr;
        pVideoFrame->virAddr[1] = pVideoFrame->virAddr[0] + pVideoFrameInfo->stVFrame.u32Width * pVideoFrameInfo->stVFrame.u32Height;
        pVideoFrame->virAddr[2] = pVideoFrame->virAddr[1];
    }
    else
    {
        SAL_LOGE("uiDataAddr NULL GetVirAddr err!!!\n");
    }

    pVideoFrame->stride[0] = pVideoFrameInfo->stVFrame.u32VirWidth;
    pVideoFrame->stride[1] = pVideoFrameInfo->stVFrame.u32VirWidth;
    pVideoFrame->vertStride[0] = pVideoFrameInfo->stVFrame.u32VirHeight;
    pVideoFrame->vertStride[1] = pVideoFrameInfo->stVFrame.u32VirHeight;

    pVideoFrame->frameNum = (UINT32)pVideoFrameInfo->stVFrame.u32TimeRef;
    pVideoFrame->pts = (UINT64)pVideoFrameInfo->stVFrame.u64PTS;
    pVideoFrame->privateDate = (PhysAddr)pVideoFrameInfo->stVFrame.u64PrivateData;
    pVideoFrame->vbBlk = (UINT64)pVideoFrameInfo->stVFrame.pMbBlk;  /* pMbBlk */

    pVideoFrame->frameParam.crop.left = pstSystemFrameInfo->u32LeftOffset;
    pVideoFrame->frameParam.crop.top = pstSystemFrameInfo->u32TopOffset;


    return SAL_SOK;
}

/**
 * @function   sys_rk_MB_VirAddr
 * @brief     获取虚拟地址的mb
 * @param[in]  UINT64 pCurPts 时间戳
 * @param[out] None
 * @return    INT32
 */
UINT64 sys_rk_MB_VirAddr(RK_VOID *pstVirAddr)
{

    MB_BLK mb = RK_MPI_MB_VirAddr2Handle(pstVirAddr);
    return (UINT64)mb;
}

/**
 * @function    sys_rk_setVideoTimeIfno
 * @brief       获取帧信息和时间戳
 * @param[in]    SYSTEM_FRAME_INFO *pstSystemFrameInfo
 *               UINT32 u32RefTime 帧时间戳
 *               UINT64 u64Pts     帧序号
 * @param[out]
 * @return
 */
INT32 sys_rk_setVideoTimeIfno(SYSTEM_FRAME_INFO *pstSystemFrameInfo, UINT32 u32RefTime, UINT64 u64Pts)
{
    VIDEO_FRAME_INFO_S *pstFrame = NULL;

    if (NULL == pstSystemFrameInfo)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    pstFrame = (VIDEO_FRAME_INFO_S *)pstSystemFrameInfo->uiAppData;
    if (NULL == pstFrame)
    {
        SAL_LOGE("prm null\n");
        return SAL_FAIL;
    }

    pstFrame->stVFrame.u32TimeRef = u32RefTime;
    pstFrame->stVFrame.u64PTS = u64Pts;
    return SAL_SOK;
}

/**
 * @function    sys_rk_setVideoTimeIfno
 * @brief       注册sys函数
 * @param[in]    void
 * @param[out]  SYS_PLAT_OPS* sys函数指针
 * @return
 */
const SYS_PLAT_OPS *sys_plat_register(void)
{
    INT32 s32Ret = SAL_SOK;

    g_stSysPlatOps.init = NULL;
    g_stSysPlatOps.deInit = sys_rk_deInit;
    g_stSysPlatOps.bind = sys_rk_bind;
    g_stSysPlatOps.getPts = sys_rk_getPts;
    g_stSysPlatOps.getVideoFrameInfo = sys_rk_getVideoFrameInfo;
    g_stSysPlatOps.getMBbyVirAddr = sys_rk_MB_VirAddr;
    g_stSysPlatOps.buildVideoFrame = sys_rk_buildVideoFrame;
    g_stSysPlatOps.allocVideoFrameInfoSt = sys_rk_allocVideoFrameInfoSt;
    g_stSysPlatOps.releaseVideoFrameInfoSt = sys_rk_releaseVideoFrameInfoSt;
    g_stSysPlatOps.setVideoTimeInfo = sys_rk_setVideoTimeIfno;

    s32Ret = sys_rk_init(0);
    if (SAL_SOK != s32Ret)
    {
        SYS_LOGE("sys_rk_init init fail\n");
    }

    return &g_stSysPlatOps;
}

