/**
 * @file   venc_hisi.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  编码组件---plat层接口封装
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        : xx年xx月xx日 Create
     Author      : unknow
     Modification: 新建文件
   2.Date        : 2021/06/25
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#include <sal.h>
#include <dspcommon.h>
#include <platform_sdk.h>
#include <platform_hal.h>
#include "venc_hisi.h"
#include "stream_bits_info_def.h"



/*****************************************************************************
                            宏定义
*****************************************************************************/

#define HISI_SDK_STREAM_MAX_CNT (64)
#define CROP_VPSS_GROUP_WIDTH	(1200)
#define CROP_VPSS_GROUP_HEIGHT	(1920)
#define MAX_VENC_FPS_P_STANDARD (50)
#define MAX_VENC_FPS_N_STANDARD (60)

#define VENC_HAL_CHECK_PRM(ptr, value) {if (!ptr) {VENC_LOGE("Ptr (The address is empty or Value is 0 )\n"); return (value); }}

/*****************************************************************************
                            全局结构体
*****************************************************************************/

/* 编码器通道属性 */
typedef struct tagVencHalChnInfoSt
{
    UINT32 uiChn;                           /* 编码器通道号    */
    UINT32 uibStart;                        /* 是否是start状态 */
    UINT32 uiState;                        /* 编码状态，0 正常状态，1等待状态 */
    UINT32 isCreate;                        /* 编码器被创建好 */
    VENC_CHN_ATTR_S stVencChnAttr;          /* 编码器通道状态属性 */
    VENC_STREAM_S stStream;
    /* 固定申请4K足够的混存空间存放抓图时的 VENC_PACK_S信息 */
    UINT32 packBufLen;
    VENC_PACK_S *pVirtPack;

    UINT32 uiFd;                            /* 通道所对应的fd     */
    UINT32 vencFps;                         /* 编码帧率 */
} VENC_HAL_CHN_INFO_S;

/* 编码器模块编码全局属性与通道管理 */
typedef struct tagVencHalInfoSt
{
    void *MutexHandle;
    /* 编码器通道管理 通道号对应数组下标,支持 VENC_HISI_CHN_NUM 个 */
    VENC_HAL_CHN_INFO_S stVencHalChnPrm[VENC_HISI_CHN_NUM];
} VENC_HAL_INFO_S;

/*****************************************************************************
                            全局结构体
*****************************************************************************/

static VENC_HAL_INFO_S gstVencHisiInfo = {0};

/* 记录 venc group 使用情况 */
static UINT32 isVencGroupUsed = 0;
static UINT64 streamPTS[VENC_HISI_CHN_NUM] = {0};

/* #define VENC_HAL_SAVE_BITS_DATA */

#ifdef VENC_HAL_SAVE_BITS_DATA
/* 测试用 */
FILE *file[VENC_HISI_CHN_NUM];
#endif

/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   venc_getEncType
 * @brief      获取平台层编码类型
 * @param[in]  UINT32 u32InType 上层传入编码类型
 * @param[out]  None
 * @return      PAYLOAD_TYPE_E hisi层编码类型
 */
static PAYLOAD_TYPE_E venc_getEncType(UINT32 u32InType)
{
    switch (u32InType)
    {
        case MJPEG:
            return PT_JPEG;
        case H265:
            return PT_H265;
        default:
            return PT_H264;
    }
}

/**
 * @function   venc_getGroup
 * @brief    获取可用的 Venc 模块
 * @param[in]  None
 * @param[out]  UINT32 *chan 通道指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_getGroup(UINT32 *pchn)
{
    int i = 0;

    if (VENC_HISI_CHN_NUM > 32)
    {
        VENC_LOGE("HAL venc pchn overflow %d\n", VENC_HISI_CHN_NUM);
        return SAL_FAIL;
    }

    SAL_mutexLock(gstVencHisiInfo.MutexHandle);
    for (i = 0; i < VENC_HISI_CHN_NUM; i++)
    {
        /* 标记为1了，表示已被使用 */
        if (isVencGroupUsed & (1 << i))
        {
            continue;
        }
        else
        {
            /* 发现未使用的，返回通道号，并标记为已用 */
            *pchn = i;
            isVencGroupUsed = isVencGroupUsed | (1 << i);
            break;
        }
    }

    /* DUP_CHN_NUM_MAX 个都被使用了，表示无可用的，返回错误，此情况就该扩大 DUP_CHN_NUM_MAX 的值 */
    if (i == VENC_HISI_CHN_NUM)
    {
        *pchn = 0xff;
        SAL_mutexUnlock(gstVencHisiInfo.MutexHandle);
        return SAL_FAIL;
    }

    SAL_mutexUnlock(gstVencHisiInfo.MutexHandle);
    return SAL_SOK;
}

/**
 * @function   venc_putGroup
 * @brief    释放的 Venc 模块
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static void venc_putGroup(UINT32 u32Chan)
{
    SAL_mutexLock(gstVencHisiInfo.MutexHandle);
    /* 编码器消耗后回收通道 */
    if (isVencGroupUsed & (1 << u32Chan))
    {
        isVencGroupUsed = isVencGroupUsed & (~(1 << u32Chan));
    }

    SAL_mutexUnlock(gstVencHisiInfo.MutexHandle);
}

/**
 * @function   venc_saveH264Stream
 * @brief    调试接口，保存H264编码码流
 * @param[in]  VENC_STREAM_S *pstStream 码流信息
 * @param[out] FILE *fpH264File 目标文件
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_saveH264Stream(FILE *fpH264File, VENC_STREAM_S *pstStream)
{
    HI_S32 i = 0;

    for (i = 0; i < pstStream->u32PackCount; i++)
    {
        fwrite(pstStream->pstPack[i].pu8Addr + pstStream->pstPack[i].u32Offset,
               pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset, 1, fpH264File);

        fflush(fpH264File);
    }

    return SAL_SOK;
}

/**
 * @function   venc_saveJpegStream
 * @brief    调试接口，保存Jpeg编码码流
 * @param[in]  VENC_STREAM_S *pstStream 码流信息
 * @param[out] FILE *fpH264File 目标文件
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_saveJpegStream(FILE *fpMJpegFile, VENC_STREAM_S *pstStream)
{
    VENC_PACK_S *pstData = NULL;
    HI_U32 i = 0;

    for (i = 0; i < pstStream->u32PackCount; i++)
    {
        pstData = &pstStream->pstPack[i];
        fwrite(pstData->pu8Addr + pstData->u32Offset, pstData->u32Len - pstData->u32Offset, 1, fpMJpegFile);
        fflush(fpMJpegFile);
    }

    return SAL_SOK;
}

/**
 * @function   venc_saveH265Stream
 * @brief    调试接口，保存H265编码码流
 * @param[in]  VENC_STREAM_S *pstStream 码流信息
 * @param[out] FILE *fpH264File 目标文件
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_saveH265Stream(FILE *fpH265File, VENC_STREAM_S *pstStream)
{
    HI_S32 i = 0;

    for (i = 0; i < pstStream->u32PackCount; i++)
    {
        fwrite(pstStream->pstPack[i].pu8Addr + pstStream->pstPack[i].u32Offset,
               pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset, 1, fpH265File);

        fflush(fpH265File);
    }

    return SAL_SOK;
}

/**
 * @function   venc_checkResPrm
 * @brief    检查分辨率
 * @param[in]  UINT32 u32UserSetW, UINT32 u32UserSetH 宽高信息
 * @param[out] UINT32 *pUiW, UINT32 *pUiH 宽高信息
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_checkResPrm(UINT32 u32UserSetW, UINT32 u32UserSetH, UINT32 *pUiW, UINT32 *pUiH)
{
    /*607项目中宽为1080，高为1920，在全局初始化的时候已经下发(syschain)*/
    UINT32 u32Width = 0;
    UINT32 u32Height = 0;

    /* 宽 */
    if (u32UserSetW <= VENC_MAX_WIDTH)
    {
        u32Width = u32UserSetW;
    }
    else
    {
        VENC_LOGW("Set Enc Prm Width(%d) is large than Max Width(%d), Please reset the resolution!!!\n", u32UserSetW, VENC_MAX_WIDTH);
        u32Width = VENC_MAX_WIDTH;
    }

    /* 高 */
    if (u32UserSetH <= VENC_MAX_HEIGHT)
    {
        u32Height = u32UserSetH;
    }
    else
    {
        VENC_LOGW("Set Enc Prm Height(%d) is large than Max Height(%d), Please reset the resolution !!!\n", u32UserSetH, VENC_MAX_HEIGHT);
        u32Height = VENC_MAX_HEIGHT;
    }

    /* 3516av200 平台上的Jpeg编码最小分辨率是 32 * 32 */
    if (u32Width < VENC_MIN_WIDTH)
    {
        VENC_LOGW("Set Enc Prm Width(%d) is Smaller than Min Width(%d), Please reset the resolution !!!\n", u32UserSetW, VENC_MIN_WIDTH);
        u32Width = VENC_MIN_WIDTH;
    }

    if (u32Height < VENC_MIN_HEIGHT)
    {
        VENC_LOGW("Set Enc Prm Height(%d) is Smaller than Min Height(%d), Please reset the resolution !!!\n", u32UserSetH, VENC_MIN_HEIGHT);
        u32Height = VENC_MIN_HEIGHT;
    }

    *pUiW = u32Width;
    *pUiH = u32Height;

    return SAL_SOK;
}

/**
 * @function   venc_setJpegPrm
 * @brief   配置编码参数
 * @param[in]  UINT32 u32Chn 编码通道
 * @param[in]  SAL_VideoFrameParam *pstVencHalChnPrm 编码参数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_setJpegPrm(UINT32 u32Chn, SAL_VideoFrameParam *pstVencHalChnPrm)
{
    HI_S32 s32Ret = HI_SUCCESS;

    VENC_CHN_ATTR_S stAttr = {0};
    VENC_CHN_PARAM_S stChnParam = {0};
    VENC_JPEG_PARAM_S stJpegParam = {0};
    UINT32 u32JpgQuality = 0;
    UINT32 u32W = 0;
    UINT32 u32H = 0;

    memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
    /* memset(&stFrameRate, 0, sizeof(VENC_FRAME_RATE_S)); */

    if (HI_SUCCESS != (s32Ret = HI_MPI_VENC_GetChnAttr(u32Chn, &stAttr)))
    {
        VENC_LOGE("Venc Chn %d Get Chn Attr Failed, %x !!!\n", u32Chn, s32Ret);
        return SAL_FAIL;
    }

    if (PT_JPEG == stAttr.stVencAttr.enType)
    {
        memset(&stJpegParam, 0, sizeof(VENC_JPEG_PARAM_S));
        if (HI_SUCCESS != (s32Ret = HI_MPI_VENC_GetJpegParam(u32Chn, &stJpegParam)))
        {
            VENC_LOGE("JPEG Chn %d get Chn Param Failed, %x !!!\n", u32Chn, s32Ret);
            return SAL_FAIL;
        }

        if (pstVencHalChnPrm->quality < 1)
        {
            u32JpgQuality = 1;
        }
        else if (pstVencHalChnPrm->quality > 99)
        {
            u32JpgQuality = 99;
        }
        else
        {
            u32JpgQuality = pstVencHalChnPrm->quality;
        }

        stJpegParam.u32Qfactor = u32JpgQuality;
        if (HI_SUCCESS != (s32Ret = HI_MPI_VENC_SetJpegParam(u32Chn, &stJpegParam)))
        {
            VENC_LOGE("JPEG Chn %d Set Chn Param Failed, %x !!!\n", u32Chn, s32Ret);
            return SAL_FAIL;
        }

        venc_checkResPrm(pstVencHalChnPrm->width,
                         pstVencHalChnPrm->height,
                         &u32W, &u32H);

        stAttr.stVencAttr.u32PicWidth = u32W;
        stAttr.stVencAttr.u32PicHeight = u32H;
    }
    else
    {
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, Enc Type Failed , %d  !!!\n", u32Chn, stAttr.stVencAttr.enType);
        return SAL_FAIL;
    }

    if (HI_SUCCESS != (s32Ret = HI_MPI_VENC_SetChnAttr(u32Chn, &stAttr)))
    {
        VENC_LOGE("u32PicWidth %d u32PicHeight %d, FrmRate %d !!!\n",
                  stAttr.stVencAttr.u32PicWidth, stAttr.stVencAttr.u32PicHeight, stChnParam.stFrameRate.s32DstFrmRate);
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, %x !!!\n", u32Chn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_hisi_requestIDR
 * @brief    强制I帧编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_requestIDR(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if (SAL_SOK != HI_MPI_VENC_RequestIDR(pstVencHalChnPrm->uiChn, SAL_TRUE))
    {
        VENC_LOGE("\n");

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_hisi_putFrame
 * @brief    释放指定通道的码流缓存，和VencHal_getFrame 成对出现
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_putFrame(void *pHandle)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VENC_STREAM_S *pstStream = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstStream = &pstVencHalChnPrm->stStream;
    if ((NULL == pstStream) || (NULL == pstStream->pstPack))
    {
        VENC_LOGE("Stream is NULL !!!\n");
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VENC_ReleaseStream(pstVencHalChnPrm->uiChn, pstStream);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("s32Ret %x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_hisi_getFrame
 * @brief    从指定的编码通道获取最新的一帧编码码流，保证和VencHal_putFrame成对调用
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] void *pInPrm 获取编码码流
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_getFrame(void *pHandle, void *pInPrm)
{
    INT32 s32Ret = 0;
    INT32 i = 0;
    UINT32 u32Fd = 0;
    UINT32 u32RcType = 0;
    fd_set fdRread;
    UINT32 u32TimeOut = 0;
    UINT32 u32Cnt = 0;

    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    STREAM_FRAME_INFO_ST *pstFrameInfo = NULL;
    VENC_STREAM_S *pstStream = NULL;
    VENC_CHN_STATUS_S stStat = {0};

    if ((NULL == pHandle) || (NULL == pInPrm))
    {
        VENC_LOGE("Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstFrameInfo = (STREAM_FRAME_INFO_ST *)pInPrm;
    pstStream = &pstVencHalChnPrm->stStream;
    if (NULL == pstStream->pstPack)
    {
        VENC_LOGE("Memory err!!!\n");
        return SAL_FAIL;
    }

    u32Fd = pstVencHalChnPrm->uiFd;
    /* 获取码流前先清0标志位 */
    pstFrameInfo->uiValid = 0;
    pstFrameInfo->uiNaluNum = 0;
    memset(&stStat, 0, sizeof(VENC_CHN_STATUS_S));
    u32TimeOut = 1000 / (pstVencHalChnPrm->vencFps > 0 ? pstVencHalChnPrm->vencFps : 1);

    do
    {
        FD_ZERO(&fdRread);
        FD_SET(u32Fd, &fdRread);
        s32Ret = SAL_mselect(u32Fd, 100);
        if (s32Ret < 0)
        {
            VENC_LOGE("Select Error \n");
            return SAL_FAIL;
        }
        else if (0 == s32Ret)
        {
            /* return SAL_FAIL; */
            if ((0 != pstVencHalChnPrm->uiState) || (u32Cnt * 100 > u32TimeOut))
            {
                return SAL_FAIL;
            }

            u32Cnt++;
            continue;
        }
        else
        {
            if (FD_ISSET(u32Fd, &fdRread))
            {
                s32Ret = HI_MPI_VENC_QueryStatus(pstVencHalChnPrm->uiChn, &stStat);
                if (HI_SUCCESS != s32Ret)
                {
                    VENC_LOGE("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", pstVencHalChnPrm->uiChn, s32Ret);
                    return SAL_FAIL;
                }

                if (0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
                {
                    if (0 != pstVencHalChnPrm->uiState)
                    {
                        VENC_LOGE("HI_MPI_VENC_Query chn[%d][%d,%d]\n", pstVencHalChnPrm->uiChn, stStat.u32CurPacks, stStat.u32LeftStreamFrames);
                        return SAL_FAIL;
                    }

                    continue;
                }

                pstStream->u32PackCount = stStat.u32CurPacks;
                s32Ret = HI_MPI_VENC_GetStream(pstVencHalChnPrm->uiChn, pstStream, 1000);
                if (HI_SUCCESS != s32Ret)
                {

                    VENC_LOGE("VENC chan %d HI_MPI_VENC_GetStream failed with %#x!\n", pstVencHalChnPrm->uiChn, s32Ret);
                    return SAL_FAIL;
                }

                #ifdef VENC_HAL_SAVE_BITS_DATA
                /* 保存码流到文件 */
                venc_saveH264Stream(file[pstVencHalChnPrm->uiChn], pstStream);
                #endif

                /* 提取信息 */
                /* pstFrameInfo->eFrameType = (pstStream->u32PackCount >= 3) ? (H264_TYPE_IFRAME): (H264_TYPE_PFRAME); */
                for (i = 0; i < pstStream->u32PackCount; i++)
                {
                    pstFrameInfo->stNaluInfo[i].pucNaluPtr = pstStream->pstPack[i].pu8Addr + pstStream->pstPack[i].u32Offset;
                    pstFrameInfo->stNaluInfo[i].uiNaluLen = pstStream->pstPack[i].u32Len - pstStream->pstPack[i].u32Offset;
                    pstFrameInfo->stNaluInfo[i].u64PTS = pstStream->pstPack[i].u64PTS / 1000;
                    #if 0
                    UINT64 curPts = 0;
                    if (HI_SUCCESS != HI_MPI_SYS_GetCurPTS(&curPts))
                    {
                        pstFrameInfo->stNaluInfo[i].u64PTS = streamPTS[uiChn] + 10;    /* pstStream->pstPack[i].u64PTS; */
                        VENC_LOGW("hisi get pts is error \n");
                        /* VENC_LOGI("old Pts %llu,cur Pts %llu\n",streamPTS[uiChn],pstStream->pstPack[i].u64PTS); */
                    }
                    else
                    {
                        pstFrameInfo->stNaluInfo[i].u64PTS = curPts / 1000;
                    }

                    #endif
                    pstFrameInfo->uiNaluNum++;
                    if (i == 0)
                    {
                        /* VENC_LOGI("uiChn %d,Nalu Pts %llu,stream Pts %llu\n",uiChn,pstFrameInfo->stNaluInfo[i].u64PTS ,pstStream->pstPack[i].u64PTS); */
                        if (streamPTS[pstVencHalChnPrm->uiChn] >= pstFrameInfo->stNaluInfo[i].u64PTS)
                        {
                            UINT64 p64CurPts = 0;
                            HI_MPI_SYS_GetCurPTS(&p64CurPts);
                            VENC_LOGI("p64CurPts is %llu old Pts %llu,cur Pts %llu\n", p64CurPts / 1000, streamPTS[pstVencHalChnPrm->uiChn], pstFrameInfo->stNaluInfo[i].u64PTS);
                            pstFrameInfo->stNaluInfo[i].u64PTS += 2;
                        }

                        streamPTS[pstVencHalChnPrm->uiChn] = pstFrameInfo->stNaluInfo[i].u64PTS;
                    }

                     #if 0 /* 开启此处条件判断和宏 TEST_H264_STREAM 便可以直接用 vlc 拉 H264 裸流 */
                    UnitTest_rtspDemoPutStreamData(pstFrameInfo->stNaluInfo[i].pucNaluPtr, \
                                                   pstFrameInfo->stNaluInfo[i].uiNaluLen, \
                                                   pstFrameInfo->stNaluInfo[i].u64PTS, \
                                                   pstStream->pstPack[i].DataType.enH264EType, \
                                                   pstStream->u32Seq);
                    #endif
                }

                pstFrameInfo->uiFrameWidth = pstVencHalChnPrm->stVencChnAttr.stVencAttr.u32PicWidth;
                pstFrameInfo->uiFrameHeight = pstVencHalChnPrm->stVencChnAttr.stVencAttr.u32PicHeight;

                u32RcType = pstVencHalChnPrm->stVencChnAttr.stRcAttr.enRcMode;
                if (VENC_RC_MODE_H264CBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRate;
                    pstFrameInfo->eFrameType = (pstStream->u32PackCount >= 3) ? (STREAM_TYPE_H264_IFRAME) : (STREAM_TYPE_H264_PFRAME);
                }
                else if (VENC_RC_MODE_H264VBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.stRcAttr.stH264Vbr.fr32DstFrameRate;
                    pstFrameInfo->eFrameType = (pstStream->u32PackCount >= 3) ? (STREAM_TYPE_H264_IFRAME) : (STREAM_TYPE_H264_PFRAME);
                }
                else if (VENC_RC_MODE_H265CBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRate;
                    pstFrameInfo->eFrameType = (pstStream->u32PackCount >= 4) ? (STREAM_TYPE_H265_IFRAME) : (STREAM_TYPE_H265_PFRAME);
                }
                else if (VENC_RC_MODE_H265VBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.stRcAttr.stH265Vbr.fr32DstFrameRate;
                    pstFrameInfo->eFrameType = (pstStream->u32PackCount >= 4) ? (STREAM_TYPE_H265_IFRAME) : (STREAM_TYPE_H265_PFRAME);
                }
                else
                {
                    pstFrameInfo->uiFps = 25;
                    pstFrameInfo->eFrameType = (pstStream->u32PackCount >= 3) ? (STREAM_TYPE_H264_IFRAME) : (STREAM_TYPE_H264_PFRAME);
                    VENC_LOGW("u32RcType error is %d\n", u32RcType);
                }

                pstFrameInfo->uiValid = 1;

                pstFrameInfo->uiValid = 1;
                return SAL_SOK;
            }
            else
            {
                return SAL_FAIL;
            }
        }
    }
    while (1);

    return SAL_SOK;
}

/**
 * @function   venc_query
 * @brief   Venc通道查询当前帧的码流包个数
 * @param[in]  UINT32 u32Chn 编码通道
 * @param[out]  UINT32 *pFrameCnt 码流包个数
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_query(UINT32 u32Chn, UINT32 *pFrameCnt)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    pstVencHalChnPrm = &gstVencHisiInfo.stVencHalChnPrm[u32Chn];
    VENC_CHN_STATUS_S stStat = {0};

    if (NULL == pFrameCnt)
    {
        VENC_LOGE("Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    *pFrameCnt = 0;

    memset(&stStat, 0, sizeof(VENC_CHN_STATUS_S));

    s32Ret = HI_MPI_VENC_QueryStatus(pstVencHalChnPrm->uiChn, &stStat);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", u32Chn, s32Ret);
        return SAL_FAIL;
    }

    if (0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
    {
        VENC_LOGE("NOTE: Current chn %d:%d frame is NULL!\n", u32Chn, pstVencHalChnPrm->uiChn);
        return SAL_FAIL;
    }

    VENC_LOGI("u32CurPacks %d !!!\n", stStat.u32CurPacks);
    *pFrameCnt = stStat.u32CurPacks;

    return SAL_SOK;
}

/**
 * @function   venc_select
 * @brief   Venc通道事件数据
 * @param[in]  UINT32 u32Chan 编码通道
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_select(UINT32 u32Chan)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    struct timeval stTimeoutVal = {0};

    UINT32 u32Fd = 0;
    fd_set fdRead;
    INT32 s32Ret = 0;

    pstVencHalChnPrm = &gstVencHisiInfo.stVencHalChnPrm[u32Chan];
    u32Fd = pstVencHalChnPrm->uiFd;

    if (0 == pstVencHalChnPrm->uibStart)
    {
        return SAL_FAIL;
    }

    FD_ZERO(&fdRead);

    FD_SET(u32Fd, &fdRead);

    stTimeoutVal.tv_sec = 2;
    stTimeoutVal.tv_usec = 0;

    s32Ret = select(u32Fd + 1, &fdRead, NULL, NULL, &stTimeoutVal);

    if (s32Ret < 0)
    {
        VENC_LOGE("select err\n");
        return SAL_FAIL;
    }
    else if (s32Ret == 0)
    {
        return SAL_FAIL;
    }
    else
    {
        return SAL_SOK;
    }

    return SAL_SOK;
}

/**
 * @function   venc_hisi_stop
 * @brief    停止编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_stop(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    HI_S32 s32Ret = HI_SUCCESS;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if (1 == pstVencHalChnPrm->uibStart)
    {
        s32Ret = HI_MPI_VENC_StopRecvFrame(pstVencHalChnPrm->uiChn);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc HI_MPI_VENC_StopRecvFrame Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        pstVencHalChnPrm->uibStart = 0;
    }

    return SAL_SOK;
}

/**
 * @function   venc_hisi_start
 * @brief    开始编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_start(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    VENC_RECV_PIC_PARAM_S stRecvParam = {0};
    HI_S32 s32Ret = HI_SUCCESS;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    stRecvParam.s32RecvPicNum = -1;
    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;


    if (SAL_FALSE == pstVencHalChnPrm->uibStart)
    {
        s32Ret = HI_MPI_VENC_StopRecvFrame(pstVencHalChnPrm->uiChn);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc HI_MPI_VENC_StopRecvFrame Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        s32Ret = HI_MPI_VENC_ResetChn(pstVencHalChnPrm->uiChn);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc HI_MPI_VENC_ResetChn Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            /* return SAL_FAIL; */
        }

        s32Ret = HI_MPI_VENC_StartRecvFrame(pstVencHalChnPrm->uiChn, &stRecvParam);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc HI_MPI_VENC_StartRecvPic Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        pstVencHalChnPrm->uibStart = SAL_TRUE;
    }

    return SAL_SOK;
}

/**
 * @function   venc_destroy
 * @brief   销毁编码器
 * @param[in]  void * pHandle 编码句柄
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_destroy(void *pHandle)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    VENC_STREAM_S *pstStream = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    /* 关闭文件句柄 */
    s32Ret = HI_MPI_VENC_CloseFd(pstVencHalChnPrm->uiChn);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_CloseFd [%d] faild with %#x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VENC_DestroyChn(pstVencHalChnPrm->uiChn);
    if (s32Ret != HI_SUCCESS)
    {
        VENC_LOGE("DestroyChn chan=%d!!!\n", pstVencHalChnPrm->uiChn);
        return SAL_FAIL;
    }

    if (PT_JPEG == pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType)
    {
        if (NULL != pstVencHalChnPrm->pVirtPack)
        {
            SAL_memfree(pstVencHalChnPrm->pVirtPack, "venc", "hisi_enc");
            pstVencHalChnPrm->pVirtPack = NULL;
        }
    }
    else
    {
        /* 释放底层编码器所需内存 */
        pstStream = &pstVencHalChnPrm->stStream;
        if (NULL != pstStream->pstPack)
        {
            SAL_memfree(pstStream->pstPack, "venc", "hisi_enc");
            pstStream->pstPack = NULL;
        }
    }

    /* 释放编码器通道 */
    venc_putGroup(pstVencHalChnPrm->uiChn);

    pstVencHalChnPrm->uibStart = SAL_FALSE;
    pstVencHalChnPrm->isCreate = SAL_FALSE;

    VENC_LOGI("VENC Chn HalChn %d Destroy success.\n", pstVencHalChnPrm->uiChn);
    return SAL_SOK;
}

/**
 * @function   venc_setH26xPrm
 * @brief    配置编码参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_setH26xPrm(void *pHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_SOK;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    SAL_VideoFrameParam *pstVencChnInfo = NULL;

    VENC_CHN_ATTR_S *pstAttr = NULL;
    VENC_FRAME_RATE_S stFrameRate = {0};
    VENC_H264_VUI_S stVui = {0};
    VENC_RC_PARAM_S stRcParam = {0};
    UINT32 u32ActualWid = 0;
    UINT32 u32ActualHei = 0;
    UINT32 u32EncFps = 0;
    UINT32 u32ViFps = 0;

    if ((NULL == pHandle) || (NULL == pstInPrm))
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencChnInfo = pstInPrm;
    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    if (SAL_FALSE == pstVencHalChnPrm->isCreate)
    {
        VENC_LOGE("VENC Hal chan %d no create.\n", pstVencHalChnPrm->uiChn);
        return SAL_SOK;
    }

    pstAttr = &pstVencHalChnPrm->stVencChnAttr;
    if (HI_SUCCESS != (s32Ret = HI_MPI_VENC_GetChnAttr(pstVencHalChnPrm->uiChn, pstAttr)))
    {
        VENC_LOGE("Venc Hal Chn %d:%d Get Chn Attr Failed, %x !!!\n", pstVencHalChnPrm->uiChn, pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    venc_checkResPrm(pstVencChnInfo->width,
                     pstVencChnInfo->height,
                     &u32ActualWid,
                     &u32ActualHei);

    u32ViFps = (pstVencChnInfo->fps >> 16); /*高16位源fps，低16位编码fps*/
    u32EncFps = (pstVencChnInfo->fps & 0xffff);
    VENC_LOGI("src rate %d dst rate %d\n", u32ViFps, u32EncFps);

    /* 配置帧率 */
    stFrameRate.s32SrcFrmRate = (0 == u32ViFps) ? 60 : u32ViFps; /* (0 == pstVencChnInfo->standard)?(MAX_VENC_FPS_P_STANDARD):(MAX_VENC_FPS_N_STANDARD); */
    stFrameRate.s32DstFrmRate = u32EncFps;
    pstVencHalChnPrm->vencFps = u32EncFps;
    pstAttr->stVencAttr.u32PicWidth = u32ActualWid;
    pstAttr->stVencAttr.u32PicHeight = u32ActualHei;

    if (PT_H265 == venc_getEncType(pstVencChnInfo->encodeType))
    {
        pstAttr->stRcAttr.enRcMode = (pstVencChnInfo->bpsType == 0) ? VENC_RC_MODE_H265VBR : VENC_RC_MODE_H265CBR;
        pstAttr->stVencAttr.enType = PT_H265;
        if (VENC_RC_MODE_H265VBR == pstAttr->stRcAttr.enRcMode)
        {
            pstAttr->stRcAttr.stH265Vbr.u32SrcFrameRate = stFrameRate.s32SrcFrmRate;
            pstAttr->stRcAttr.stH265Vbr.fr32DstFrameRate = stFrameRate.s32DstFrmRate;
            pstAttr->stRcAttr.stH265Vbr.u32Gop = stFrameRate.s32DstFrmRate * 2;
            pstAttr->stRcAttr.stH265Vbr.u32MaxBitRate = pstVencChnInfo->bps;
            pstAttr->stRcAttr.stH265Vbr.u32StatTime = 4;
        }
        else if (VENC_RC_MODE_H265CBR == pstAttr->stRcAttr.enRcMode)
        {
            pstAttr->stRcAttr.stH265Cbr.u32SrcFrameRate = stFrameRate.s32SrcFrmRate;
            pstAttr->stRcAttr.stH265Cbr.fr32DstFrameRate = stFrameRate.s32DstFrmRate;
            pstAttr->stRcAttr.stH265Cbr.u32Gop = stFrameRate.s32DstFrmRate * 2;
            pstAttr->stRcAttr.stH265Cbr.u32BitRate = pstVencChnInfo->bps;
            pstAttr->stRcAttr.stH265Cbr.u32StatTime = 4;

        }

        pstAttr->stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
        pstAttr->stGopAttr.stNormalP.s32IPQpDelta = -1;    /* 使用推荐值 */

        if (HI_SUCCESS != (s32Ret = HI_MPI_VENC_SetChnAttr(pstVencHalChnPrm->uiChn, pstAttr)))
        {
            VENC_LOGE("Venc Chn %d:%d Set Chn Attr Failed, %x !!!\n", pstVencHalChnPrm->uiChn, pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        return SAL_SOK;
    }
    else
    {
        /* 目前只有h265和h264 */
        pstAttr->stVencAttr.enType = PT_H264;
    }

    pstAttr->stRcAttr.enRcMode = (pstVencChnInfo->bpsType == 0) ? VENC_RC_MODE_H264VBR : VENC_RC_MODE_H264CBR;

    if (VENC_RC_MODE_H264VBR == pstAttr->stRcAttr.enRcMode)
    {
        VENC_LOGW("========= VBR RC control frame rate, Maxbitrate %d =========\n", pstVencChnInfo->bps);

        pstAttr->stRcAttr.stH264Vbr.u32MaxBitRate = pstVencChnInfo->bps;
        pstAttr->stRcAttr.stH264Vbr.u32StatTime = 6;  /* 10秒统计一次码率 */
        pstAttr->stRcAttr.stH264Vbr.u32SrcFrameRate = stFrameRate.s32SrcFrmRate;
        pstAttr->stRcAttr.stH264Vbr.fr32DstFrameRate = stFrameRate.s32DstFrmRate;
        pstAttr->stRcAttr.stH264Vbr.u32Gop = stFrameRate.s32DstFrmRate * 2;
    }
    else if (VENC_RC_MODE_H264CBR == pstAttr->stRcAttr.enRcMode)
    {
        VENC_LOGW("========== CBR RC control frame rate, bitrate %d =========\n", pstVencChnInfo->bps);

        pstAttr->stRcAttr.stH264Cbr.u32BitRate = pstVencChnInfo->bps;
        pstAttr->stRcAttr.stH264Cbr.u32StatTime = 4;     /* 10秒统计一次码率 */
        pstAttr->stRcAttr.stH264Cbr.u32SrcFrameRate = stFrameRate.s32SrcFrmRate;
        pstAttr->stRcAttr.stH264Cbr.fr32DstFrameRate = stFrameRate.s32DstFrmRate;
        pstAttr->stRcAttr.stH264Cbr.u32Gop = stFrameRate.s32DstFrmRate * 2;
    }

    VENC_LOGI("HAL SrcFps %d DstFps %d !!!\n", stFrameRate.s32SrcFrmRate, stFrameRate.s32DstFrmRate);

    pstAttr->stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pstAttr->stGopAttr.stNormalP.s32IPQpDelta = -1;   /* 使用推荐值 */

    if (HI_SUCCESS != (s32Ret = HI_MPI_VENC_SetChnAttr(pstVencHalChnPrm->uiChn, pstAttr)))
    {
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, %x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    SAL_clear(&stRcParam);
    s32Ret = HI_MPI_VENC_GetRcParam(pstVencHalChnPrm->uiChn, &stRcParam);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_SetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    if (VENC_RC_MODE_H264VBR == pstAttr->stRcAttr.enRcMode)
    {
        stRcParam.stParamH264Vbr.u32MinQp = 10;
        stRcParam.stParamH264Vbr.u32MaxQp = 50;
        stRcParam.stParamH264Vbr.u32MinIQp = 10;
        stRcParam.stParamH264Vbr.u32MaxIQp = 50;
        stRcParam.stParamH264Vbr.u32MinIprop = 1;
        stRcParam.stParamH264Vbr.u32MaxIprop = 100;
        VENC_LOGI("vbr pQp[%d,%d] IQp[%d,%d],Iprop [%d,%d] !!!\n", stRcParam.stParamH264Vbr.u32MinQp, stRcParam.stParamH264Vbr.u32MaxQp,
                  stRcParam.stParamH264Vbr.u32MinIQp, stRcParam.stParamH264Vbr.u32MaxIQp, stRcParam.stParamH264Vbr.u32MinIprop, stRcParam.stParamH264Vbr.u32MaxIprop);
    }

    s32Ret = HI_MPI_VENC_SetRcParam(pstVencHalChnPrm->uiChn, &stRcParam);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_SetRcParam [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VENC_GetH264Vui(pstVencHalChnPrm->uiChn, &stVui);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_GetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    stVui.stVuiTimeInfo.timing_info_present_flag = 1;
    stVui.stVuiTimeInfo.time_scale = 60000;
    stVui.stVuiTimeInfo.num_units_in_tick = 30000 / stFrameRate.s32DstFrmRate;
    s32Ret = HI_MPI_VENC_SetH264Vui(pstVencHalChnPrm->uiChn, &stVui);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_SetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_hisi_sendFrame
 * @brief    向 venc 通道发送 数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  VOID *pStr 视频帧信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_sendFrame(void *pHandle, VOID *pStr)
{
    VIDEO_FRAME_INFO_S *pstFrame = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    HI_S32 s32Ret = HI_SUCCESS;

    if ((NULL == pStr) || (NULL == pHandle))
    {
        VENC_LOGE("Input ptr is null!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstFrame = (VIDEO_FRAME_INFO_S *)pStr;
    if (PT_JPEG == pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType)
    {
        s32Ret = HI_MPI_VENC_SendFrame(pstVencHalChnPrm->uiChn, pstFrame, -1);
    }
    else
    {

        s32Ret = HI_MPI_VENC_SendFrame(pstVencHalChnPrm->uiChn, pstFrame, 500);
    }

    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("VENC_SendFrame fail!, s32Ret = %x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_hisi_sendFrm
 * @brief    向 venc 通道发送 数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  void *pStr 视频帧信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_sendFrm(void *pHandle, void *pStr)
{
    VIDEO_FRAME_INFO_S *pstFrame = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    SYSTEM_FRAME_INFO *pstSrc = NULL;
    HI_S32 s32Ret = HI_SUCCESS;

    if ((NULL == pStr) || (NULL == pHandle))
    {
        VENC_LOGE("Input ptr is null!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstSrc = (SYSTEM_FRAME_INFO *)pStr;

    pstFrame = (VIDEO_FRAME_INFO_S *)pstSrc->uiAppData;
    VENC_HAL_CHECK_PRM(pstFrame, SAL_FAIL);

#if 0
    VENC_LOGI("frm info:mod %#x pool %#x w %d h %d pixel_format %d v format %d compress %d dynamic %d color_gamut %d time_ref %d pts %d\n",
              pstFrame->mod_id, pstFrame->pool_id,
              pstFrame->video_frame.width, pstFrame->video_frame.height,
              pstFrame->video_frame.pixel_format, pstFrame->video_frame.video_format, pstFrame->video_frame.compress_mode,
              pstFrame->video_frame.dynamic_range, pstFrame->video_frame.color_gamut,
              pstFrame->video_frame.time_ref, pstFrame->video_frame.pts);

    VENC_LOGI("frm info:header_stride %d stride %d header_phys_addr %#x phys_addr %#x header_virt_addr %#x virt_addr %#x\n",
              pstFrame->video_frame.header_stride[0], pstFrame->video_frame.stride[0],
              pstFrame->video_frame.header_phys_addr[0], pstFrame->video_frame.phys_addr[0],
              pstFrame->video_frame.header_virt_addr[0], pstFrame->video_frame.virt_addr[0]);
#endif

    if (PT_JPEG != pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType)
    {
         s32Ret = HI_MPI_VENC_SendFrame((VENC_CHN)pstVencHalChnPrm->uiChn, pstFrame, 500);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("hi_mpi_venc_send_frame fail!, s32Ret = %#x time ref %d pts %llu\n", s32Ret, pstFrame->stVFrame.u32TimeRef, pstFrame->stVFrame.u64PTS);
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("enc type err, %#x\n", pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_hisi_setStatus
 * @brief    设置编码状态，方便drv层管理编码、取流流程
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  VOID *pStr 状态信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_setStatus(void *pHandle, VOID *pStr)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    UINT32 u32Status = 0;

    if ((NULL == pStr) || (NULL == pHandle))
    {
        VENC_LOGE("Input ptr is null!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    u32Status = *((UINT32 *)pStr);

    pstVencHalChnPrm->uiState = u32Status;

    return SAL_SOK;
}

/**
 * @function   venc_creatH26x
 * @brief    创建H26x硬件编码器
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] void **ppHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_creatH26x(void **ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    HI_S32 s32Ret = HI_SUCCESS;
    UINT32 u32Width = 0;
    UINT32 u32Height = 0;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    VENC_CHN_ATTR_S *pstVencChnAttr = NULL;
    VENC_STREAM_S *pstStream = NULL;
    SAL_VideoFrameParam *pstHalPrm = NULL;
    VENC_RC_PARAM_S stRcParam = {0};
    VENC_H264_VUI_S stVui = {0};
    UINT32 u32Chan = 0;
    UINT32 u32EncFps = 0;
    UINT32 u32ViFps = 0;
    PAYLOAD_TYPE_E enEncType = PT_H264; /*def prm*/


    if ((NULL == pstInPrm) || (ppHandle == NULL))
    {
        VENC_LOGE("Prm is NULL, Create Failed !!!\n");
        return SAL_FAIL;
    }

    pstHalPrm = pstInPrm;
    if (SAL_FAIL == venc_getGroup(&u32Chan))
    {
        VENC_LOGE("HAL Not Venc Chn to Used, Chn %d Create Failed !!!\n", u32Chan);
        return SAL_FAIL;
    }

    pstVencHalChnPrm = &gstVencHisiInfo.stVencHalChnPrm[u32Chan];
    pstVencHalChnPrm->uiChn = u32Chan;

    u32Width = pstHalPrm->width;
    u32Height = pstHalPrm->height;
    enEncType = venc_getEncType(pstHalPrm->encodeType);

    pstVencChnAttr = &pstVencHalChnPrm->stVencChnAttr;
    memset(pstVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));

    pstVencChnAttr->stVencAttr.enType = enEncType;
    pstVencChnAttr->stVencAttr.u32MaxPicWidth = 1920;
    pstVencChnAttr->stVencAttr.u32MaxPicHeight = 1920;
    pstVencChnAttr->stVencAttr.u32PicWidth = u32Width;
    pstVencChnAttr->stVencAttr.u32PicHeight = u32Height;
    pstVencChnAttr->stVencAttr.u32BufSize = SAL_align(1920 * 1920 * 3 / 2, 64);
    pstVencChnAttr->stVencAttr.u32Profile = 0;
    pstVencChnAttr->stVencAttr.bByFrame = HI_TRUE;

    u32ViFps = (pstHalPrm->fps >> 16); /*高16位源fps，低16位编码fps*/
    u32EncFps = (pstHalPrm->fps & 0xffff);
    VENC_LOGI("src rate %d dst rate %d\n", u32ViFps, u32EncFps);

    if (0 == pstHalPrm->bpsType)
    {
        /* 默认使用变码率 */
        if (PT_H264 == enEncType)
        {
            pstVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
            pstVencChnAttr->stRcAttr.stH264Vbr.u32MaxBitRate = pstHalPrm->bps;
            pstVencChnAttr->stRcAttr.stH264Vbr.u32StatTime = 6;    /* 10秒统计一次码率 */

            pstVencChnAttr->stRcAttr.stH264Vbr.u32SrcFrameRate = (0 == u32ViFps) ? 60 : u32ViFps;
            pstVencChnAttr->stRcAttr.stH264Vbr.fr32DstFrameRate = u32EncFps > 0 ? u32EncFps : 30; /* 30;//(0 == pstHalPrm->uiVStandard)?(MAX_FPS_P_STANDARD):(MAX_FPS_N_STANDARD); */
            pstVencChnAttr->stRcAttr.stH264Vbr.u32Gop = pstVencChnAttr->stRcAttr.stH264Vbr.fr32DstFrameRate * 2;
            VENC_LOGW("create H264 VBR chn %d w is %d, h is %d, fps is %d, bps is %d, RcMode is %d\n",
                      u32Chan, u32Width, u32Height, pstVencChnAttr->stRcAttr.stH264Vbr.fr32DstFrameRate, pstHalPrm->bps,
                      pstVencChnAttr->stRcAttr.enRcMode);
        }
        else if (PT_H265 == enEncType)
        {
            pstVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
            pstVencChnAttr->stRcAttr.stH265Vbr.u32MaxBitRate = pstHalPrm->bps;
            pstVencChnAttr->stRcAttr.stH265Vbr.u32StatTime = 10;  /* 10秒统计一次码率 */

            pstVencChnAttr->stRcAttr.stH265Vbr.u32SrcFrameRate = (0 == u32ViFps) ? 60 : u32ViFps; /* (0 == pstHalPrm->standard)?(MAX_VENC_FPS_P_STANDARD):(MAX_VENC_FPS_N_STANDARD); */
            pstVencChnAttr->stRcAttr.stH265Vbr.fr32DstFrameRate = u32EncFps > 0 ? u32EncFps : 30; /* 30;//(0 == pstHalPrm->uiVStandard)?(MAX_FPS_P_STANDARD):(MAX_FPS_N_STANDARD); */
            pstVencChnAttr->stRcAttr.stH265Vbr.u32Gop = pstVencChnAttr->stRcAttr.stH264Vbr.fr32DstFrameRate * 2;
            VENC_LOGW("create H265 VBR chn %d w is %d, h is %d, fps is %d, bps is %d, RcMode is %d\n",
                      u32Chan, u32Width, u32Height, pstVencChnAttr->stRcAttr.stH265Vbr.fr32DstFrameRate, pstHalPrm->bps,
                      pstVencChnAttr->stRcAttr.enRcMode);
        }
        else
        {
            VENC_LOGE(" encType (%d) error only support h264/h265....\n", pstHalPrm->encodeType);
            goto err;
        }
    }
    else
    {
        /* 默认使用定码率 */
        if (PT_H264 == enEncType)
        {
            pstVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
            pstVencChnAttr->stRcAttr.stH264Cbr.u32BitRate = pstHalPrm->bps;
            pstVencChnAttr->stRcAttr.stH264Cbr.u32StatTime = 2;
            pstVencChnAttr->stRcAttr.stH264Cbr.u32SrcFrameRate = (0 == u32ViFps) ? 60 : u32ViFps; /* (0 == pstHalPrm->standard)?(MAX_VENC_FPS_P_STANDARD):(MAX_VENC_FPS_N_STANDARD); */
            pstVencChnAttr->stRcAttr.stH264Cbr.fr32DstFrameRate = u32EncFps > 0 ? u32EncFps : 30; /* 30;//(0 == pstHalPrm->uiVStandard)?(MAX_FPS_P_STANDARD):(MAX_FPS_N_STANDARD); */
            pstVencChnAttr->stRcAttr.stH264Cbr.u32Gop = pstVencChnAttr->stRcAttr.stH264Cbr.fr32DstFrameRate * 2;

            VENC_LOGW("create H264 CBR chn %d w is %d, h is %d, fps is %d, bps is %d,RcMode is %d\n",
                      u32Chan, u32Width, u32Height, pstVencChnAttr->stRcAttr.stH264Cbr.fr32DstFrameRate, pstHalPrm->bps,
                      pstVencChnAttr->stRcAttr.enRcMode);
        }
        else if (PT_H265 == enEncType)
        {
            pstVencChnAttr->stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
            pstVencChnAttr->stRcAttr.stH265Cbr.u32BitRate = pstHalPrm->bps;
            pstVencChnAttr->stRcAttr.stH265Cbr.u32StatTime = 2;
            pstVencChnAttr->stRcAttr.stH265Cbr.u32SrcFrameRate = (0 == u32ViFps) ? 60 : u32ViFps; /* (0 == pstHalPrm->standard)?(MAX_VENC_FPS_P_STANDARD):(MAX_VENC_FPS_N_STANDARD); */
            pstVencChnAttr->stRcAttr.stH265Cbr.fr32DstFrameRate = u32EncFps > 0 ? u32EncFps : 30; /* 30;//(0 == pstHalPrm->uiVStandard)?(MAX_FPS_P_STANDARD):(MAX_FPS_N_STANDARD); */
            pstVencChnAttr->stRcAttr.stH265Cbr.u32Gop = pstVencChnAttr->stRcAttr.stH265Cbr.fr32DstFrameRate * 2;

            VENC_LOGW("create H265 CBR chn %d w is %d, h is %d, fps is %d, bps is %d, RcMode is %d\n",
                      u32Chan, u32Width, u32Height, pstVencChnAttr->stRcAttr.stH264Cbr.fr32DstFrameRate, pstHalPrm->bps,
                      pstVencChnAttr->stRcAttr.enRcMode);
        }
        else
        {
            VENC_LOGE(" encType %d(%d) error only support h264/h265....\n", enEncType, pstHalPrm->encodeType);
            goto err;
        }
    }

    pstVencChnAttr->stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pstVencChnAttr->stGopAttr.stNormalP.s32IPQpDelta = 0;

    s32Ret = HI_MPI_VENC_CreateChn(pstVencHalChnPrm->uiChn, pstVencChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("VENC Module %d chan %d Create faild with %#x! ===\n", u32Chan, pstVencHalChnPrm->uiChn, s32Ret);
        goto err;
    }

    SAL_clear(&stRcParam);
    s32Ret = HI_MPI_VENC_GetRcParam(pstVencHalChnPrm->uiChn, &stRcParam);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_GetRcParam [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        goto err;
    }

    if (0 == pstHalPrm->bpsType)
    {
        if (PT_H264 == enEncType)
        {
            stRcParam.stParamH264Vbr.u32MinQp = 10;
            stRcParam.stParamH264Vbr.u32MaxQp = 50;
            stRcParam.stParamH264Vbr.u32MinIQp = 10;
            stRcParam.stParamH264Vbr.u32MaxIQp = 50;
            stRcParam.stParamH264Vbr.u32MinIprop = 1;
            stRcParam.stParamH264Vbr.u32MaxIprop = 100;
            VENC_LOGI("vbr pQp[%d,%d] IQp[%d,%d],Iprop [%d,%d] !!!\n", stRcParam.stParamH264Vbr.u32MinQp, stRcParam.stParamH264Vbr.u32MaxQp,
                      stRcParam.stParamH264Vbr.u32MinIQp, stRcParam.stParamH264Vbr.u32MaxIQp, stRcParam.stParamH264Vbr.u32MinIprop, stRcParam.stParamH264Vbr.u32MaxIprop);
        }
        else if (PT_H265 == enEncType)
        {
            stRcParam.stParamH265Vbr.u32MinQp = 10;
            stRcParam.stParamH265Vbr.u32MinIQp = 10;
            stRcParam.stParamH265Vbr.u32MaxQp = 40;
        }
        else
        {
            VENC_LOGE(" encType %d(%d) error only support h264/h265....\n", enEncType, pstHalPrm->encodeType);
            goto err;
        }
    }

    s32Ret = HI_MPI_VENC_SetRcParam(pstVencHalChnPrm->uiChn, &stRcParam);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_SetRcParam [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        goto err;
    }

    if (PT_H264 == enEncType)
    {
        s32Ret = HI_MPI_VENC_GetH264Vui(pstVencHalChnPrm->uiChn, &stVui);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("HI_MPI_VENC_GetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
            goto err;
        }

        /* 这里设置时间戳的单位 60K时钟  80K时钟  90K时钟，具体值需要和海思确认过 */
        stVui.stVuiTimeInfo.timing_info_present_flag = 1;
        stVui.stVuiTimeInfo.time_scale = 60000;
        stVui.stVuiTimeInfo.num_units_in_tick = 30000 / (pstVencChnAttr->stRcAttr.stH264Cbr.fr32DstFrameRate);
        s32Ret = HI_MPI_VENC_SetH264Vui(pstVencHalChnPrm->uiChn, &stVui);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("HI_MPI_VENC_SetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
            goto err;
        }
    }

    pstVencHalChnPrm->uiFd = HI_MPI_VENC_GetFd(pstVencHalChnPrm->uiChn);

#ifdef VENC_HAL_SAVE_BITS_DATA

    char aszFileName[64];
    snprintf(aszFileName, 64, "./DSP_TEST_RES/Stream/ES/stream_chn%d.es.h264", u32Chan);

    file[uiChn] = fopen(aszFileName, "wb");

    if (HI_NULL == file[u32Chan])
    {
        VENC_LOGE("open file failed:%s!\n", strerror(errno));
    }

#endif
    /* 申请底层编码器所需内存 */
    pstStream = &pstVencHalChnPrm->stStream;
    pstStream->pstPack = (VENC_PACK_S *)SAL_memMalloc(sizeof(VENC_PACK_S) * FRAME_MAX_NALU_NUM, "venc", "hisi_enc");
    if (NULL == pstStream->pstPack)
    {
        VENC_LOGE("malloc stream pack failed!\n");
        goto err;
    }

    *ppHandle = pstVencHalChnPrm;

    pstVencHalChnPrm->uibStart = SAL_FALSE;
    pstVencHalChnPrm->isCreate = SAL_TRUE;
    VENC_LOGI("VENC Chn %d HalChn %d Create success.\n", u32Chan, pstVencHalChnPrm->uiChn);
    return SAL_SOK;

err:
    venc_putGroup(u32Chan);
    return SAL_FAIL;
}

/**
 * @function   venc_hisi_delete
 * @brief    销毁编码器
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_delete(void *pHandle)
{
    INT32 s32Ret = SAL_SOK;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    s32Ret = venc_hisi_stop(pHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("StopRecvPic chan=%d!!!\n", pstVencHalChnPrm->uiChn);
        return SAL_FAIL;
    }

    /* 销毁编码器通道 */
    s32Ret = venc_destroy(pHandle);
    if (SAL_isFail(s32Ret))
    {
        VENC_LOGE("DestroyChn chan=%d!!!\n", pstVencHalChnPrm->uiChn);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_hisi_getChan
 * @brief    获取plat层通道venc_hal_getVencHalChan
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] UINT32 *poutChan plat层通道号指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */

INT32 venc_hisi_getChan(void *pHandle, UINT32 *pOutChan)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if ((NULL == pHandle) || (NULL == pOutChan))
    {
        VENC_LOGE("inv prm\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    *pOutChan = pstVencHalChnPrm->uiChn;

    return SAL_SOK;
}

/* #define TEST_SAVE_JPEG_YUV */
#ifdef TEST_SAVE_JPEG_YUV
UINT32 gCnt = 0;
/* static HI_U32 u32Size = 0; */
/* static HI_CHAR* pUserPageAddr[2] = {HI_NULL, HI_NULL}; */

void sample_yuv_8bit_dump(VIDEO_FRAME_S *pstVBuf, FILE *pfd)
{
    UINT32 u32W = 0;
    UINT32 u32H = 0;
    char *pVBufVirtY = NULL;
    char *pVBufVirtC = NULL;
    char *pMemContent = NULL;
    unsigned char *TmpBuff = NULL; /* If this value is too small and the image is big, this memory may not be enough */
    HI_U64 u64PhyAddr;
    PIXEL_FORMAT_E enPixelFormat = pstVBuf->enPixelFormat;
    HI_U32 u32UvHeight = 0; /*When the storage format is a planar format, this variable is used to keep the height of the UV component */
    HI_U32 u32Size = 0;
    HI_CHAR *pUserPageAddr[2] = {HI_NULL, HI_NULL};

    if (TmpBuff == NULL)
    {
        TmpBuff = malloc(1920 * 1080 * 2);
    }

    if ((PIXEL_FORMAT_YVU_SEMIPLANAR_420 == enPixelFormat)
        || (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat))
    {
        u32Size = (pstVBuf->u32Stride[0]) * (pstVBuf->u32Height) * 3 / 2;
        u32UvHeight = pstVBuf->u32Height / 2;
    }
    else if ((PIXEL_FORMAT_YVU_SEMIPLANAR_422 == enPixelFormat)
             || (PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixelFormat))
    {
        u32Size = (pstVBuf->u32Stride[0]) * (pstVBuf->u32Height) * 2;
        u32UvHeight = pstVBuf->u32Height;
    }
    else if (PIXEL_FORMAT_YUV_400 == enPixelFormat)
    {
        u32Size = (pstVBuf->u32Stride[0]) * (pstVBuf->u32Height);
        u32UvHeight = pstVBuf->u32Height;
    }

    u64PhyAddr = pstVBuf->u64VirAddr[0];

    pUserPageAddr[0] = (HI_CHAR *)u64PhyAddr; /* /(HI_CHAR*) HI_MPI_SYS_Mmap(u64PhyAddr, u32Size); */

    if (HI_NULL == pUserPageAddr[0])
    {
        return;
    }

    pVBufVirtY = pUserPageAddr[0];
    pVBufVirtC = pVBufVirtY + (pstVBuf->u32Stride[0]) * (pstVBuf->u32Height);

    /* save Y ----------------------------------------------------------------*/
    printf("saving......Y......wxh[%d,%d],s %d,pF %d\n", pstVBuf->u32Width, pstVBuf->u32Height, pstVBuf->u32Stride[0], enPixelFormat);
    fflush(stdout);

    for (u32H = 0; u32H < pstVBuf->u32Height; u32H++)
    {
        pMemContent = pVBufVirtY + u32H * pstVBuf->u32Stride[0];
        fwrite(pMemContent, pstVBuf->u32Width, 1, pfd);
    }

    if (PIXEL_FORMAT_YUV_400 != enPixelFormat)
    {
        fflush(pfd);
        /* save v ----------------------------------------------------------------*/
        printf("v.....u32Stride %d.", pstVBuf->u32Stride[1]);
        fflush(stdout);

        for (u32H = 0; u32H < u32UvHeight; u32H++)
        {
            pMemContent = pVBufVirtC + u32H * pstVBuf->u32Stride[1];

            pMemContent += 1;

            for (u32W = 0; u32W < pstVBuf->u32Width / 2; u32W++)
            {
                TmpBuff[u32W] = *pMemContent;
                pMemContent += 2;
            }

            fwrite(TmpBuff, pstVBuf->u32Width / 2, 1, pfd);
        }

        fflush(pfd);

        /* save U ----------------------------------------------------------------*/
        fprintf(stderr, "u......");
        fflush(stderr);

        for (u32H = 0; u32H < u32UvHeight; u32H++)
        {
            pMemContent = pVBufVirtC + u32H * pstVBuf->u32Stride[1];

            for (u32W = 0; u32W < pstVBuf->u32Width / 2; u32W++)
            {
                TmpBuff[u32W] = *pMemContent;
                pMemContent += 2;
            }

            fwrite(TmpBuff, pstVBuf->u32Width / 2, 1, pfd);
        }
    }

    fflush(pfd);

    fprintf(stderr, "done %d!\n", pstVBuf->u32TimeRef);
    fflush(stderr);

    /* /HI_MPI_SYS_Munmap(pUserPageAddr[0], u32Size); */
    pUserPageAddr[0] = HI_NULL;
    free(TmpBuff);
    TmpBuff = NULL;
}

#endif

/**
 * @function   venc_setJpegCropPrm
 * @brief    配置编码通道裁剪参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  VENC_CROP_INFO_S *pstCropInfo crop信息
 * @param[out]  UINT32 *pJpegSize 编码码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_setJpegCropPrm(void *pHandle, VENC_CROP_INFO_S *pstCropInfo)
{
    HI_S32 s32Ret = HI_SUCCESS;
    VENC_CHN_PARAM_S stChnParam = {0};
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if (NULL == pstCropInfo || SAL_TRUE != pstCropInfo->bEnable)
    {
        return SAL_SOK;
    }

    s32Ret = HI_MPI_VENC_GetChnParam(pstVencHalChnPrm->uiChn, &stChnParam);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("Venc Chn %d Get Chn Attr Failed, ret: 0x%x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    stChnParam.stCropCfg.bEnable = pstCropInfo->bEnable;
    stChnParam.stCropCfg.stRect.s32X = ALIGN_DOWN(pstCropInfo->stRect.s32X + 1, 2);
    stChnParam.stCropCfg.stRect.s32Y = ALIGN_DOWN(pstCropInfo->stRect.s32Y + 1, 2);
    stChnParam.stCropCfg.stRect.u32Width = ALIGN_DOWN(pstCropInfo->stRect.u32Width + 2, 4);
    stChnParam.stCropCfg.stRect.u32Height = ALIGN_DOWN(pstCropInfo->stRect.u32Height + 2, 4);

    s32Ret = HI_MPI_VENC_SetChnParam(pstVencHalChnPrm->uiChn, &stChnParam);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, ret: 0x%x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_getJpegFrame
 * @brief    Jpeg编码通道获取一帧数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out]  INT8 *pcJpeg 编码码流
 * @param[out]  UINT32 *pJpegSize 编码码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_getJpegFrame(void *pHandle, INT8 *pcJpeg, UINT32 *pJpegSize)
{
    HI_S32 s32Ret = HI_SUCCESS;
    INT32 i = 0;
    UINT32 u32Size = 0;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    VENC_STREAM_S stStream = {0};
    VENC_CHN_STATUS_S stStat = {0};
    VENC_PACK_S *pstData = NULL;;

    UINT32 QueryCount = 0;

    if ((NULL == pHandle) || (NULL == pcJpeg) || (NULL == pJpegSize))
    {
        VENC_LOGE("inv ptr\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    memset(&stStat, 0, sizeof(VENC_CHN_STATUS_S));

    while (0 == stStat.u32CurPacks || 0 == stStat.u32LeftStreamFrames)
    {
        s32Ret = HI_MPI_VENC_QueryStatus(pstVencHalChnPrm->uiChn, &stStat);
        if (HI_SUCCESS != s32Ret)
        {
            VENC_LOGE("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        QueryCount++;
        if (QueryCount >= 500)
        {
            VENC_LOGE("Can not get jpeg frame!\n");
            return SAL_FAIL;
        }

        SAL_msleep(10);
    }

    stStream.pstPack = pstVencHalChnPrm->pVirtPack;
    if (NULL == stStream.pstPack)
    {
        VENC_LOGE("malloc stream pack failed!\n");
        return SAL_FAIL;
    }

    stStream.u32PackCount = stStat.u32CurPacks;

    s32Ret = HI_MPI_VENC_GetStream(pstVencHalChnPrm->uiChn, &stStream, -1);
    if (HI_SUCCESS != s32Ret)
    {
        stStream.pstPack = NULL;
        VENC_LOGE("chn %d HI_MPI_VENC_GetStream failed with %#x!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    u32Size = 0;
    for (i = 0; i < stStream.u32PackCount; i++)
    {
        pstData = &stStream.pstPack[i];

        if (pstData->u32Len > 0 && pstData->u32Len > pstData->u32Offset)
        {
            memcpy(pcJpeg + u32Size,
                   pstData->pu8Addr + pstData->u32Offset,
                   pstData->u32Len - pstData->u32Offset);

            u32Size += pstData->u32Len - pstData->u32Offset;
        }
    }

    *pJpegSize = u32Size;
    s32Ret = HI_MPI_VENC_ReleaseStream(pstVencHalChnPrm->uiChn, &stStream);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("VENC release fail!\n");
        stStream.pstPack = NULL;
        return SAL_FAIL;
    }

    memset(pstVencHalChnPrm->pVirtPack, 0x00, pstVencHalChnPrm->packBufLen);
    stStream.pstPack = NULL;

    return SAL_SOK;
}

/**
 * @function   venc_hisi_encJpeg
 * @brief    获取plat层通道venc_hal_getVencHalChan
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 帧信息
 * @param[in]  CROP_S *pstCropInfo 编码crop信息
 * @param[in]  BOOL bSetPrm 编码参数更新，用于重配编码器
 * @param[out] INT8 *pJpeg 编码输出JPEG码流
 * @param[out] INT8 *pJpegSize 编码输出JPEG码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_encJpeg(void *pHandle, SYSTEM_FRAME_INFO *pstFrame, INT8 *pJpeg, UINT32 *pJpegSize, CROP_S *pstCropInfo, BOOL bSetPrm)
{
    VENC_HAL_CHECK_PRM(pstFrame, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pJpeg, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pJpegSize, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pHandle, SAL_FAIL);

    VIDEO_FRAME_INFO_S *pstFrameInfo = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    VENC_HAL_CHECK_PRM(pstFrameInfo, SAL_FAIL);

    SAL_VideoFrameParam stJpegPicInfo = {0};
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    VENC_CROP_INFO_S stCropInfo = {0};

    if ((SAL_TRUE == bSetPrm) || (NULL != pstCropInfo)) /*抠图模式必然要设置参数*/
    {
        stJpegPicInfo.quality = 99; /* 80; */

        if (NULL != pstCropInfo)
        {
            stCropInfo.bEnable = pstCropInfo->u32CropEnable;
            stCropInfo.stRect.u32Height = (HI_U32)pstCropInfo->u32H;
            stCropInfo.stRect.u32Width = (HI_U32)pstCropInfo->u32W;
            stCropInfo.stRect.s32X = (HI_S32)pstCropInfo->u32X;
            stCropInfo.stRect.s32Y = (HI_S32)pstCropInfo->u32Y;

            stJpegPicInfo.height = stCropInfo.stRect.u32Height;
            stJpegPicInfo.width = stCropInfo.stRect.u32Width;
        }
        else
        {
            stCropInfo.bEnable = SAL_FALSE;
            stCropInfo.stRect.u32Height = (HI_U32)pstFrameInfo->stVFrame.u32Height;
            stCropInfo.stRect.u32Width = (HI_U32)pstFrameInfo->stVFrame.u32Width;
            stCropInfo.stRect.s32X = (HI_S32)0;
            stCropInfo.stRect.s32Y = (HI_S32)0;
            stJpegPicInfo.height = pstFrameInfo->stVFrame.u32Height;
            stJpegPicInfo.width = pstFrameInfo->stVFrame.u32Width;
        }

        if (SAL_SOK != venc_setJpegPrm(pstVencHalChnPrm->uiChn, &stJpegPicInfo))
        {
            VENC_LOGE("venc_setJpegPrm fail!\n");
            return SAL_FAIL;
        }

        if (SAL_SOK != venc_setJpegCropPrm(pHandle, &stCropInfo))
        {
            VENC_LOGE("venc_setJpegCropPrm error !\n");
            return SAL_FAIL;
        }
    }

    if (SAL_SOK != venc_hisi_start(pHandle))
    {
        VENC_LOGE("VencHal_drvStart error !\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_sendFrame(pHandle, pstFrameInfo))
    {
        VENC_LOGE("venc_hisi_sendFrame error !\n");
        venc_hisi_stop(pHandle);
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_getJpegFrame(pHandle, pJpeg, pJpegSize))
    {
        VENC_LOGE("venc_getJpegFrame error !\n");
        venc_hisi_stop(pHandle);
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_hisi_stop(pHandle))
    {
        VENC_LOGE("venc_hisi_stop error !\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_createJpeg
 * @brief    创建JPEG编码通道
 * @param[in]   SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out]  void *pHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_createJpeg(void **ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32HalChan = 0;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    VENC_CHN_ATTR_S *pstVencChnAttr = NULL;

    if ((NULL == pstInPrm) || (NULL == ppHandle))
    {
        VENC_LOGE("Prm is NULL, Create Failed !!!\n");
        return SAL_FAIL;
    }

    s32Ret = venc_getGroup(&u32HalChan);
    if (SAL_SOK != s32Ret)
    {
        VENC_LOGE("get chan fail, Chn %d Create Failed !!!\n", u32HalChan);
        return SAL_FAIL;
    }

    pstVencHalChnPrm = &gstVencHisiInfo.stVencHalChnPrm[u32HalChan];
    pstVencHalChnPrm->uiChn = u32HalChan;

    pstVencChnAttr = &pstVencHalChnPrm->stVencChnAttr;
    memset(pstVencChnAttr, 0, sizeof(VENC_CHN_ATTR_S));

    /* 配置通道属性 */
    pstVencChnAttr->stVencAttr.enType = PT_JPEG;
    pstVencChnAttr->stVencAttr.u32PicWidth = pstInPrm->width;
    pstVencChnAttr->stVencAttr.u32PicHeight = pstInPrm->height;
    pstVencChnAttr->stVencAttr.u32MaxPicWidth = VENC_JPEG_MAX_WIDTH; 
    pstVencChnAttr->stVencAttr.u32MaxPicHeight = VENC_JPEG_MAX_HEIGHT; 

    /* 需要 64 对齐 */
    pstVencChnAttr->stVencAttr.u32BufSize = SAL_align((pstVencChnAttr->stVencAttr.u32MaxPicWidth * pstVencChnAttr->stVencAttr.u32MaxPicHeight * 3 / 2), 64);
    pstVencChnAttr->stVencAttr.u32Profile = 0;
    pstVencChnAttr->stVencAttr.bByFrame = HI_TRUE;
    pstVencChnAttr->stVencAttr.stAttrJpege.bSupportDCF = HI_FALSE;
    pstVencChnAttr->stVencAttr.stAttrJpege.stMPFCfg.u8LargeThumbNailNum = 0;
    pstVencChnAttr->stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
    pstVencChnAttr->stGopAttr.stNormalP.s32IPQpDelta = 0;

    VENC_LOGI("JPEG Chn enc_%d  W : %d H : %d !!!\n", pstVencHalChnPrm->uiChn, pstInPrm->width, pstInPrm->height);
    s32Ret = HI_MPI_VENC_CreateChn(pstVencHalChnPrm->uiChn, pstVencChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        VENC_LOGE("Hal Create Jpeg [%d] faild with %#x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        venc_putGroup(u32HalChan);
        return SAL_FAIL;
    }

    /* HI_MPI_VENC_SetMaxStreamCnt(pstVencHalChnPrm->uiChn, 1); */

    pstVencHalChnPrm->uiFd = HI_MPI_VENC_GetFd(pstVencHalChnPrm->uiChn);
    pstVencHalChnPrm->uibStart = SAL_FALSE;
    pstVencHalChnPrm->packBufLen = SAL_KB * 4;
    pstVencHalChnPrm->pVirtPack = (VENC_PACK_S *)SAL_memMalloc(pstVencHalChnPrm->packBufLen, "jpeg", "hisi_enc");
    if (NULL == pstVencHalChnPrm->pVirtPack)
    {
        VENC_LOGE("!!!\n");
        venc_putGroup(u32HalChan);
        return SAL_FAIL;
    }

    *ppHandle = (void *)pstVencHalChnPrm;

    VENC_LOGI("Hal Create JPEG Chn %d Suc !!!\n", pstVencHalChnPrm->uiChn);

    return SAL_SOK;
}

/**
 * @function   venc_hisi_setEncPrm
 * @brief    设置编码参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]   SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_setEncPrm(void *ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_SOK;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    SAL_VideoFrameParam stVencFrmPrm = {0};

    if ((NULL == ppHandle) || (NULL == pstInPrm))
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)ppHandle;

    if (PT_JPEG == pstVencHalChnPrm->stVencChnAttr.stVencAttr.enType)
    {
        memset(&stVencFrmPrm, 0, sizeof(SAL_VideoFrameParam));

        stVencFrmPrm.width = pstInPrm->width;
        stVencFrmPrm.height = pstInPrm->height;
        stVencFrmPrm.quality = pstInPrm->quality;

        if (SAL_SOK != venc_hisi_stop(ppHandle))
        {
            VENC_LOGE("!!!\n");
            return SAL_FAIL;
        }

        if (SAL_SOK != venc_setJpegPrm(pstVencHalChnPrm->uiChn, &stVencFrmPrm))
        {
            VENC_LOGE("!!!\n");
            return SAL_FAIL;
        }

        if (SAL_SOK != venc_hisi_start(ppHandle))
        {
            VENC_LOGE("!!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        s32Ret = venc_setH26xPrm(ppHandle, pstInPrm);
        if (SAL_SOK != s32Ret)
        {
            VENC_LOGE("!!!\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   venc_hisi_create
 * @brief    创建视频硬件编码器
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] void **ppHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hisi_create(void **ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_FAIL;

    if ((NULL == pstInPrm) || (ppHandle == NULL))
    {
        VENC_LOGE("Prm is NULL, Create Failed !!!\n");
        return SAL_FAIL;
    }

    if (PT_JPEG == venc_getEncType(pstInPrm->encodeType))
    {
        s32Ret = venc_createJpeg(ppHandle, pstInPrm);
    }
    else
    {
        s32Ret = venc_creatH26x(ppHandle, pstInPrm);
    }

    if (SAL_SOK != s32Ret)
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
   Function:   vencHal_drv_init
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 venc_hisi_init(void)
{
    /* 初始化通道管理锁 */
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &gstVencHisiInfo.MutexHandle);
    return SAL_SOK;
}

/**
 * @function   venc_hal_register
 * @brief      注册hal层回调函数
 * @param[in]  None
 * @param[out] VENC_PLAT_OPS_S * 回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_register(VENC_PLAT_OPS_S *pstVencPlatOps)
{
    if (NULL == pstVencPlatOps)
    {
        return SAL_FAIL;
    }

    /* 注册hal层处理函数 */
    pstVencPlatOps->Init = venc_hisi_init;
    pstVencPlatOps->Create = venc_hisi_create;
    pstVencPlatOps->Delete = venc_hisi_delete;
    pstVencPlatOps->Start = venc_hisi_start;
    pstVencPlatOps->Stop = venc_hisi_stop;
    pstVencPlatOps->SetParam = venc_hisi_setEncPrm;
    pstVencPlatOps->ForceIFrame = venc_hisi_requestIDR;
    pstVencPlatOps->GetFrame = venc_hisi_getFrame;
    pstVencPlatOps->PutFrame = venc_hisi_putFrame;
    pstVencPlatOps->GetHalChn = venc_hisi_getChan;
    pstVencPlatOps->SetVencStatues = venc_hisi_setStatus;
    pstVencPlatOps->EncJpegProcess = venc_hisi_encJpeg;
    pstVencPlatOps->EncSendFrm = venc_hisi_sendFrm;

    return SAL_SOK;
}

