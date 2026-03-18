/**
 * @file   venc_ssv5.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  编码组件---plat层接口封装
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        : xx年xx月xx日 Create
     Author      : unknow
     Modification: 新建文件
   2.Date        : 2021/08/09
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#include <sal.h>
#include <dspcommon.h>
#include <platform_sdk.h>
#include <platform_hal.h>
#include "venc_ssv5.h"
#include "stream_bits_info_def.h"

/*****************************************************************************
                            宏定义
*****************************************************************************/

#define SSV5_SDK_STREAM_MAX_CNT (64)
#define CROP_VPSS_GROUP_WIDTH	(1200)
#define CROP_VPSS_GROUP_HEIGHT	(1920)
#define MAX_VENC_FPS_P_STANDARD (50)
#define MAX_VENC_FPS_N_STANDARD (60)

#define VENC_HAL_CHECK_PRM(ptr, value) {if (!ptr) {VENC_LOGE("Ptr (The address is empty or Value is 0 )\n"); return (value); }}

/*****************************************************************************
                            全局结构体
*****************************************************************************/

/* 编码器通道属性 */
typedef struct tagVencHalChnInfoHiv5St
{
    ot_venc_chn uiChn;                           /* 编码器通道号    */
    UINT32 uibStart;                        /* 是否是start状态 */
    UINT32 uiState;                        /* 编码状态，0 正常状态，1等待状态 */
    UINT32 isCreate;                        /* 编码器被创建好 */
    ot_venc_chn_attr stVencChnAttr;          /* 编码器通道状态属性 */
    ot_venc_stream stStream;
    /* 固定申请4K足够的混存空间存放抓图时的 VENC_PACK_S信息 */
    UINT32 packBufLen;
    ot_venc_pack *pstVirtPack;

    UINT32 uiFd;                            /* 通道所对应的fd     */
    UINT32 vencFps;                         /* 编码帧率 */
} VENC_HAL_CHN_INFO_S;

/* 编码器模块编码全局属性与通道管理 */
typedef struct tagVencHalInfoSt
{
    void *MutexHandle;
    /* 编码器通道管理 通道号对应数组下标,支持 VENC_SSV5_CHN_NUM 个 */
    VENC_HAL_CHN_INFO_S stVencHalChnPrm[VENC_SSV5_CHN_NUM];
} VENC_HAL_INFO_S;

/*****************************************************************************
                            全局结构体
*****************************************************************************/

static VENC_HAL_INFO_S g_stVencInfo = {0};

/* 记录 venc group 使用情况 */
static UINT32 isVencGroupUsed = 0;
static UINT64 streamPTS[VENC_SSV5_CHN_NUM] = {0};

/* #define VENC_HAL_SAVE_BITS_DATA */

#ifdef VENC_HAL_SAVE_BITS_DATA
/* 测试用 */
FILE *file[VENC_SSV5_CHN_NUM];
#endif

/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   venc_getEncType
 * @brief      获取平台层编码类型
 * @param[in]  UINT32 u32InType 上层传入编码类型
 * @param[out]  None
 * @return      ot_payload_type hisi层编码类型
 */
static ot_payload_type venc_getEncType(UINT32 u32InType)
{
    switch (u32InType)
    {
        case MJPEG:
            return OT_PT_JPEG;
        case H265:
            return OT_PT_H265;
        default:
            return OT_PT_H264;
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

    if (VENC_SSV5_CHN_NUM > 32)
    {
        VENC_LOGE("HAL venc pchn overflow %d\n", VENC_SSV5_CHN_NUM);
        return SAL_FAIL;
    }

    SAL_mutexLock(g_stVencInfo.MutexHandle);
    for (i = 0; i < VENC_SSV5_CHN_NUM; i++)
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
    if (i == VENC_SSV5_CHN_NUM)
    {
        *pchn = 0xff;
        SAL_mutexUnlock(g_stVencInfo.MutexHandle);
        return SAL_FAIL;
    }

    SAL_mutexUnlock(g_stVencInfo.MutexHandle);
    return SAL_SOK;
}

/**
 * @function   venc_putGroup
 * @brief    释放的 Venc 模块
 * @param[in]  UINT32 u32Chan 通道号
 * @param[out] None
 * @return     void
 */
static void venc_putGroup(UINT32 u32Chan)
{
    SAL_mutexLock(g_stVencInfo.MutexHandle);
    /* 编码器消耗后回收通道 */
    if (isVencGroupUsed & (1 << u32Chan))
    {
        isVencGroupUsed = isVencGroupUsed & (~(1 << u32Chan));
    }

    SAL_mutexUnlock(g_stVencInfo.MutexHandle);
}

/**
 * @function   venc_saveH264Stream
 * @brief    调试接口，保存H264编码码流
 * @param[in]  VENC_STREAM_S *pstStream 码流信息
 * @param[out] FILE *fpH264File 目标文件
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_saveH264Stream(FILE *fpH264File, ot_venc_stream *pstStream)
{
    td_s32 i = 0;

    for (i = 0; i < pstStream->pack_cnt; i++)
    {
        fwrite(pstStream->pack[i].addr + pstStream->pack[i].offset,
               pstStream->pack[i].len - pstStream->pack[i].offset, 1, fpH264File);

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
INT32 venc_saveJpegStream(FILE *fpMJpegFile, ot_venc_stream *pstStream)
{
    ot_venc_pack *pstData = NULL;
    td_u32 i = 0;

    for (i = 0; i < pstStream->pack_cnt; i++)
    {
        pstData = &pstStream->pack[i];
        fwrite(pstData->addr + pstData->offset, pstData->len - pstData->offset, 1, fpMJpegFile);
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
INT32 venc_saveH265Stream(FILE *fpH265File, ot_venc_stream *pstStream)
{
    td_s32 i = 0;

    for (i = 0; i < pstStream->pack_cnt; i++)
    {
        fwrite(pstStream->pack[i].addr + pstStream->pack[i].offset,
               pstStream->pack[i].len - pstStream->pack[i].offset, 1, fpH265File);

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
    td_s32 s32Ret = TD_SUCCESS;

    ot_venc_chn_attr stAttr = {0};
    ot_venc_chn_param stChnParam = {0};
    ot_venc_jpeg_param stJpegParam = {0};
    UINT32 u32JpgQuality = 0;
    UINT32 u32W = 0;
    UINT32 u32H = 0;

    memset(&stAttr, 0, sizeof(ot_venc_chn_attr));
    /* memset(&stFrameRate, 0, sizeof(VENC_FRAME_RATE_S)); */

    if (TD_SUCCESS != (s32Ret = ss_mpi_venc_get_chn_attr(u32Chn, &stAttr)))
    {
        VENC_LOGE("Venc Chn %d Get Chn Attr Failed, %x !!!\n", u32Chn, s32Ret);
        return SAL_FAIL;
    }

    if (OT_PT_JPEG == stAttr.venc_attr.type)
    {
        memset(&stJpegParam, 0, sizeof(ot_venc_jpeg_param));
        if (TD_SUCCESS != (s32Ret = ss_mpi_venc_get_jpeg_param(u32Chn, &stJpegParam)))
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

        stJpegParam.qfactor = u32JpgQuality;
        if (TD_SUCCESS != (s32Ret = ss_mpi_venc_set_jpeg_param(u32Chn, &stJpegParam)))
        {
            VENC_LOGE("JPEG Chn %d Set Chn Param Failed, %x !!!\n", u32Chn, s32Ret);
            return SAL_FAIL;
        }

        venc_checkResPrm(pstVencHalChnPrm->width,
                         pstVencHalChnPrm->height,
                         &u32W, &u32H);

        stAttr.venc_attr.pic_width = u32W;
        stAttr.venc_attr.pic_height = u32H;
    }
    else
    {
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, Enc Type Failed , %d  !!!\n", u32Chn, stAttr.venc_attr.type);
        return SAL_FAIL;
    }

    if (TD_SUCCESS != (s32Ret = ss_mpi_venc_set_chn_attr(u32Chn, &stAttr)))
    {
        VENC_LOGE("u32PicWidth %d u32PicHeight %d, FrmRate %d !!!\n",
                  stAttr.venc_attr.pic_width, stAttr.venc_attr.pic_height, stChnParam.frame_rate.dst_frame_rate);
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, %x !!!\n", u32Chn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_ssV5_requestIDR
 * @brief    强制I帧编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_requestIDR(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if (SAL_SOK != ss_mpi_venc_request_idr(pstVencHalChnPrm->uiChn, TD_TRUE))
    {
        VENC_LOGE("\n");

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_ssV5_putFrame
 * @brief    释放指定通道的码流缓存，和VencHal_getFrame 成对出现
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_putFrame(void *pHandle)
{
    td_s32 s32Ret = TD_SUCCESS;
    ot_venc_stream *pstStream = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstStream = &pstVencHalChnPrm->stStream;
    if ((NULL == pstStream) || (NULL == pstStream->pack))
    {
        VENC_LOGE("Stream is NULL !!!\n");
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_venc_release_stream(pstVencHalChnPrm->uiChn, pstStream);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("s32Ret %x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_ssV5_getFrame
 * @brief    从指定的编码通道获取最新的一帧编码码流，保证和VencHal_putFrame成对调用
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] void *pInPrm 获取编码码流
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_getFrame(void *pHandle, void *pInPrm)
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
    ot_venc_stream *pstStream = NULL;
    ot_venc_chn_status stStat = {0};

    if ((NULL == pHandle) || (NULL == pInPrm))
    {
        VENC_LOGE("Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstFrameInfo = (STREAM_FRAME_INFO_ST *)pInPrm;
    pstStream = &pstVencHalChnPrm->stStream;
    if (NULL == pstStream->pack)
    {
        VENC_LOGE("Memory err!!!\n");
        return SAL_FAIL;
    }

    u32Fd = pstVencHalChnPrm->uiFd;
    /* 获取码流前先清0标志位 */
    pstFrameInfo->uiValid = 0;
    pstFrameInfo->uiNaluNum = 0;
    memset(&stStat, 0, sizeof(ot_venc_chn_status));
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
                s32Ret = ss_mpi_venc_query_status(pstVencHalChnPrm->uiChn, &stStat);
                if (TD_SUCCESS != s32Ret)
                {
                    VENC_LOGE("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", pstVencHalChnPrm->uiChn, s32Ret);
                    return SAL_FAIL;
                }

                if (0 == stStat.cur_packs || 0 == stStat.left_stream_frames)
                {
                    if (0 != pstVencHalChnPrm->uiState)
                    {
                        VENC_LOGE("HI_MPI_VENC_Query chn[%d][%d,%d]\n", pstVencHalChnPrm->uiChn, stStat.cur_packs, stStat.left_stream_frames);
                        return SAL_FAIL;
                    }

                    continue;
                }

                pstStream->pack_cnt = stStat.cur_packs;
                s32Ret = ss_mpi_venc_get_stream(pstVencHalChnPrm->uiChn, pstStream, 1000);
                if (TD_SUCCESS != s32Ret)
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
                for (i = 0; i < pstStream->pack_cnt; i++)
                {
                    pstFrameInfo->stNaluInfo[i].pucNaluPtr = pstStream->pack[i].addr + pstStream->pack[i].offset;
                    pstFrameInfo->stNaluInfo[i].uiNaluLen = pstStream->pack[i].len - pstStream->pack[i].offset;
                    pstFrameInfo->stNaluInfo[i].u64PTS = pstStream->pack[i].pts / 1000;
                    #if 0
                    UINT64 curPts = 0;
                    if (TD_SUCCESS != hi_mpi_sys_get_cur_pts(&curPts))
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
                            ss_mpi_sys_get_cur_pts(&p64CurPts);
                            VENC_LOGI("p64CurPts is %llu。。。old Pts %llu,cur Pts %llu\n", p64CurPts / 1000, streamPTS[pstVencHalChnPrm->uiChn], pstFrameInfo->stNaluInfo[i].u64PTS);
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

                pstFrameInfo->uiFrameWidth = pstVencHalChnPrm->stVencChnAttr.venc_attr.pic_width;
                pstFrameInfo->uiFrameHeight = pstVencHalChnPrm->stVencChnAttr.venc_attr.pic_height;

                u32RcType = pstVencHalChnPrm->stVencChnAttr.rc_attr.rc_mode;
                if (OT_VENC_RC_MODE_H264_CBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.rc_attr.h264_cbr.dst_frame_rate;
                    pstFrameInfo->eFrameType = (pstStream->pack_cnt >= 3) ? (STREAM_TYPE_H264_IFRAME) : (STREAM_TYPE_H264_PFRAME);
                }
                else if (OT_VENC_RC_MODE_H264_VBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.rc_attr.h264_vbr.dst_frame_rate;
                    pstFrameInfo->eFrameType = (pstStream->pack_cnt >= 3) ? (STREAM_TYPE_H264_IFRAME) : (STREAM_TYPE_H264_PFRAME);
                }
                else if (OT_VENC_RC_MODE_H265_CBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.rc_attr.h265_cbr.dst_frame_rate;
                    pstFrameInfo->eFrameType = (pstStream->pack_cnt >= 4) ? (STREAM_TYPE_H265_IFRAME) : (STREAM_TYPE_H265_PFRAME);
                }
                else if (OT_VENC_RC_MODE_H265_VBR == u32RcType)
                {
                    pstFrameInfo->uiFps = pstVencHalChnPrm->stVencChnAttr.rc_attr.h265_vbr.dst_frame_rate;
                    pstFrameInfo->eFrameType = (pstStream->pack_cnt >= 4) ? (STREAM_TYPE_H265_IFRAME) : (STREAM_TYPE_H265_PFRAME);
                }
                else
                {
                    pstFrameInfo->uiFps = 25;
                    pstFrameInfo->eFrameType = (pstStream->pack_cnt >= 3) ? (STREAM_TYPE_H264_IFRAME) : (STREAM_TYPE_H264_PFRAME);
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
    td_s32 s32Ret = TD_SUCCESS;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    ot_venc_chn_status stStat = {0};

    if ((NULL == pFrameCnt) || (VENC_SSV5_CHN_NUM <= u32Chn))
    {
        VENC_LOGE("Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = &g_stVencInfo.stVencHalChnPrm[u32Chn];
    *pFrameCnt = 0;

    memset(&stStat, 0, sizeof(ot_venc_chn_status));

    s32Ret = ss_mpi_venc_query_status(pstVencHalChnPrm->uiChn, &stStat);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_Query chn[%d] failed with %#x!\n", u32Chn, s32Ret);
        return SAL_FAIL;
    }

    if (0 == stStat.cur_packs || 0 == stStat.left_stream_frames)
    {
        VENC_LOGE("NOTE: Current chn %d:%d frame is NULL!\n", u32Chn, pstVencHalChnPrm->uiChn);
        return SAL_FAIL;
    }

    VENC_LOGI("u32CurPacks %d !!!\n", stStat.cur_packs);
    *pFrameCnt = stStat.cur_packs;

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

    pstVencHalChnPrm = &g_stVencInfo.stVencHalChnPrm[u32Chan];
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
 * @function   venc_ssV5_stop
 * @brief    停止编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_stop(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    td_s32 s32Ret = TD_SUCCESS;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if (SAL_TRUE == pstVencHalChnPrm->uibStart)
    {
        s32Ret = ss_mpi_venc_stop_chn(pstVencHalChnPrm->uiChn);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_ERROR("hi_mpi_venc_stop_chn Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        pstVencHalChnPrm->uibStart = 0;
    }

    return SAL_SOK;
}

/**
 * @function   venc_ssV5_start
 * @brief    开始编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_start(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    ot_venc_start_param stRecvParam = {0};
    td_s32 s32Ret = TD_SUCCESS;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if (SAL_FALSE == pstVencHalChnPrm->uibStart)
    {
        s32Ret = ss_mpi_venc_reset_chn(pstVencHalChnPrm->uiChn);
        if (TD_SUCCESS != s32Ret)
        {
            VENC_LOGE("Venc HI_MPI_VENC_ResetChn Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            /* return SAL_FAIL; */
        }

        stRecvParam.recv_pic_num = -1;

        s32Ret = ss_mpi_venc_start_chn(pstVencHalChnPrm->uiChn, &stRecvParam);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_ERROR("hi_mpi_venc_start_chn Chn %d faild with %#x! \n",
                      pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        pstVencHalChnPrm->uibStart = 1;

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
    td_s32 s32Ret = TD_SUCCESS;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    ot_venc_stream *pstStream = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    /* 关闭文件句柄 */
    s32Ret = ss_mpi_venc_close_fd(pstVencHalChnPrm->uiChn);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_CloseFd [%d] faild with %#x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_venc_destroy_chn(pstVencHalChnPrm->uiChn);
    if (s32Ret != TD_SUCCESS)
    {
        VENC_LOGE("DestroyChn chan=%d!!!\n", pstVencHalChnPrm->uiChn);
        return SAL_FAIL;
    }

    if (OT_PT_JPEG == pstVencHalChnPrm->stVencChnAttr.venc_attr.type)
    {
        if (NULL != pstVencHalChnPrm->pstVirtPack)
        {
            SAL_memfree(pstVencHalChnPrm->pstVirtPack, "venc", "hisi_enc");
            pstVencHalChnPrm->pstVirtPack = NULL;
        }
    }
    else
    {
        /* 释放底层编码器所需内存 */
        pstStream = &pstVencHalChnPrm->stStream;
        if (NULL != pstStream->pack)
        {
            SAL_memfree(pstStream->pack, "venc", "hisi_enc");
            pstStream->pack = NULL;
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

    ot_venc_chn_attr *pstAttr = NULL;
    ot_frame_rate_ctrl stFrameRate = {0};
    ot_venc_h264_vui stVui = {0};
    ot_venc_rc_param stRcParam = {0};
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
    if (TD_SUCCESS != (s32Ret = ss_mpi_venc_get_chn_attr((ot_venc_chn)pstVencHalChnPrm->uiChn, pstAttr)))
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

    /* 配置帧率 */
    stFrameRate.src_frame_rate = (0 == u32ViFps) ? 60 : u32ViFps; /* (0 == pstVencChnInfo->standard)?(MAX_VENC_FPS_P_STANDARD):(MAX_VENC_FPS_N_STANDARD); */
    stFrameRate.dst_frame_rate = u32EncFps;
    pstVencHalChnPrm->vencFps = u32EncFps;
    pstAttr->venc_attr.pic_width = u32ActualWid;
    pstAttr->venc_attr.pic_height = u32ActualHei;

    if (OT_PT_H265 == venc_getEncType(pstVencChnInfo->encodeType))
    {
        pstAttr->rc_attr.rc_mode = (pstVencChnInfo->bpsType == 0) ? OT_VENC_RC_MODE_H265_VBR : OT_VENC_RC_MODE_H265_CBR;
        pstAttr->venc_attr.type = OT_PT_H265;
        if (OT_VENC_RC_MODE_H265_VBR == pstAttr->rc_attr.rc_mode)
        {
            pstAttr->rc_attr.h265_vbr.src_frame_rate = stFrameRate.src_frame_rate;
            pstAttr->rc_attr.h265_vbr.dst_frame_rate = stFrameRate.dst_frame_rate;
            pstAttr->rc_attr.h265_vbr.gop = stFrameRate.dst_frame_rate * 2;
            pstAttr->rc_attr.h265_vbr.max_bit_rate = pstVencChnInfo->bps;
            pstAttr->rc_attr.h265_vbr.stats_time = 4;
        }
        else if (OT_VENC_RC_MODE_H265_CBR == pstAttr->rc_attr.rc_mode)
        {
            pstAttr->rc_attr.h265_cbr.src_frame_rate = stFrameRate.src_frame_rate;
            pstAttr->rc_attr.h265_cbr.dst_frame_rate = stFrameRate.dst_frame_rate;
            pstAttr->rc_attr.h265_cbr.gop = stFrameRate.dst_frame_rate * 2;
            pstAttr->rc_attr.h265_cbr.bit_rate = pstVencChnInfo->bps;
            pstAttr->rc_attr.h265_cbr.stats_time = 4;

        }

        pstAttr->gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
        pstAttr->gop_attr.normal_p.ip_qp_delta = -1;    /* 使用推荐值 */

        if (TD_SUCCESS != (s32Ret = ss_mpi_venc_set_chn_attr((ot_venc_chn)pstVencHalChnPrm->uiChn, pstAttr)))
        {
            VENC_LOGE("Venc Chn %d:%d Set Chn Attr Failed, %x !!!\n", pstVencHalChnPrm->uiChn, pstVencHalChnPrm->uiChn, s32Ret);
            return SAL_FAIL;
        }

        return SAL_SOK;
    }
    else
    {
        /* 目前只有h265和h264 */
        pstAttr->venc_attr.type = OT_PT_H264;
    }

    pstAttr->rc_attr.rc_mode = (pstVencChnInfo->bpsType == 0) ? OT_VENC_RC_MODE_H264_VBR : OT_VENC_RC_MODE_H264_CBR;

    if (OT_VENC_RC_MODE_H264_VBR == pstAttr->rc_attr.rc_mode)
    {
        VENC_LOGW("========= VBR RC control frame rate, Maxbitrate %d =========\n", pstVencChnInfo->bps);

        pstAttr->rc_attr.h264_vbr.max_bit_rate = pstVencChnInfo->bps;
        pstAttr->rc_attr.h264_vbr.stats_time = 6;  /* 10秒统计一次码率 */
        pstAttr->rc_attr.h264_vbr.dst_frame_rate = stFrameRate.dst_frame_rate;
        pstAttr->rc_attr.h264_vbr.src_frame_rate = stFrameRate.src_frame_rate;
        pstAttr->rc_attr.h264_vbr.gop = stFrameRate.dst_frame_rate * 2;
    }
    else if (OT_VENC_RC_MODE_H264_CBR == pstAttr->rc_attr.rc_mode)
    {
        VENC_LOGW("========== CBR RC control frame rate, bitrate %d =========\n", pstVencChnInfo->bps);

        pstAttr->rc_attr.h264_cbr.bit_rate = pstVencChnInfo->bps;
        pstAttr->rc_attr.h264_cbr.stats_time = 4;     /* 10秒统计一次码率 */
        pstAttr->rc_attr.h264_cbr.dst_frame_rate = stFrameRate.dst_frame_rate;
        pstAttr->rc_attr.h264_cbr.src_frame_rate = stFrameRate.src_frame_rate;
        pstAttr->rc_attr.h264_cbr.gop = stFrameRate.dst_frame_rate * 2;
    }

    VENC_LOGI("HAL SrcFps %d DstFps %d !!!\n", stFrameRate.src_frame_rate, stFrameRate.dst_frame_rate);

    pstAttr->gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
    pstAttr->gop_attr.normal_p.ip_qp_delta = -1;   /* 使用推荐值 */

    if (TD_SUCCESS != (s32Ret = ss_mpi_venc_set_chn_attr((ot_venc_chn)pstVencHalChnPrm->uiChn, pstAttr)))
    {
        VENC_LOGE("Venc Chn %d Set Chn Attr Failed, %x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    SAL_clear(&stRcParam);
    s32Ret = ss_mpi_venc_get_rc_param((ot_venc_chn)pstVencHalChnPrm->uiChn, &stRcParam);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_SetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    if (OT_VENC_RC_MODE_H264_VBR == pstAttr->rc_attr.rc_mode)
    {
        stRcParam.h264_vbr_param.min_qp = 10;
        stRcParam.h264_vbr_param.max_qp = 50;
        stRcParam.h264_vbr_param.min_i_qp = 10;
        stRcParam.h264_vbr_param.max_i_qp = 50;
        stRcParam.h264_vbr_param.min_i_proportion = 1;
        stRcParam.h264_vbr_param.max_i_proportion = 100;
        VENC_LOGI("vbr pQp[%d,%d] IQp[%d,%d],Iprop [%d,%d] !!!\n", stRcParam.h264_vbr_param.min_qp, stRcParam.h264_vbr_param.max_qp,
                  stRcParam.h264_vbr_param.min_i_qp, stRcParam.h264_vbr_param.max_i_qp,
                  stRcParam.h264_vbr_param.min_i_proportion, stRcParam.h264_vbr_param.max_i_proportion);
    }

    s32Ret = ss_mpi_venc_set_rc_param((ot_venc_chn)pstVencHalChnPrm->uiChn, &stRcParam);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_SetRcParam [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_venc_get_h264_vui((ot_venc_chn)pstVencHalChnPrm->uiChn, &stVui);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_GetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    stVui.vui_time_info.timing_info_present_flag = 1;
    stVui.vui_time_info.time_scale = 60000;
    stVui.vui_time_info.num_units_in_tick = 30000 / stFrameRate.dst_frame_rate;
    s32Ret = ss_mpi_venc_set_h264_vui((ot_venc_chn)pstVencHalChnPrm->uiChn, &stVui);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_SetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_sendFrame
 * @brief    向 venc 通道发送 数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  void *pStr 视频帧信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_sendFrame(void *pHandle, void *pStr)
{
    ot_video_frame_info *pstFrame = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    td_s32 s32Ret = TD_SUCCESS;

    if ((NULL == pStr) || (NULL == pHandle))
    {
        VENC_LOGE("Input ptr is null!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstFrame = (ot_video_frame_info *)pStr;

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

    if (OT_PT_JPEG == pstVencHalChnPrm->stVencChnAttr.venc_attr.type)
    {
        s32Ret = ss_mpi_venc_send_frame((ot_venc_chn)pstVencHalChnPrm->uiChn, pstFrame, -1);
    }
    else
    {
        s32Ret = ss_mpi_venc_send_frame((ot_venc_chn)pstVencHalChnPrm->uiChn, pstFrame, 500);
    }

    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("hi_mpi_venc_send_frame fail!, s32Ret = %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_ssV5_sendFrm
 * @brief    向 venc 通道发送 数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  void *pStr 视频帧信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_sendFrm(void *pHandle, void *pStr)
{
    ot_video_frame_info *pstFrame = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    SYSTEM_FRAME_INFO *pstSrc = NULL;
    td_s32 s32Ret = TD_SUCCESS;

    if ((NULL == pStr) || (NULL == pHandle))
    {
        VENC_LOGE("Input ptr is null!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstSrc = (SYSTEM_FRAME_INFO *)pStr;

    pstFrame = (ot_video_frame_info *)pstSrc->uiAppData;
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

    if (OT_PT_JPEG != pstVencHalChnPrm->stVencChnAttr.venc_attr.type)
    {
        s32Ret = ss_mpi_venc_send_frame((ot_venc_chn)pstVencHalChnPrm->uiChn, pstFrame, 500);
        if (TD_SUCCESS != s32Ret)
        {
            VENC_LOGE("hi_mpi_venc_send_frame fail!, s32Ret = %#x\n", s32Ret);
            return SAL_FAIL;
        }
    }
    else
    {
        VENC_LOGE("enc type err, %#x\n", pstVencHalChnPrm->stVencChnAttr.venc_attr.type);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_ssV5_setStatus
 * @brief    设置编码状态，方便drv层管理编码、取流流程
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  void *pStr 状态信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_setStatus(void *pHandle, void *pStr)
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
    td_s32 s32Ret = TD_SUCCESS;
    UINT32 u32Width = 0;
    UINT32 u32Height = 0;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    ot_venc_chn_attr *pstVencChnAttr = NULL;
    ot_venc_stream *pstStream = NULL;
    SAL_VideoFrameParam *pstHalPrm = NULL;
    ot_venc_rc_param stRcParam = {0};
    ot_venc_h264_vui stVui = {0};
    UINT32 u32Chan = 0;
    UINT32 u32EncFps = 0;
    UINT32 u32ViFps = 0;
    ot_payload_type enEncType = OT_PT_H264; /*def prm*/


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

    pstVencHalChnPrm = &g_stVencInfo.stVencHalChnPrm[u32Chan];
    pstVencHalChnPrm->uiChn = u32Chan;

    u32Width = pstHalPrm->width;
    u32Height = pstHalPrm->height;
    enEncType = venc_getEncType(pstHalPrm->encodeType);

    pstVencChnAttr = &pstVencHalChnPrm->stVencChnAttr;
    memset(pstVencChnAttr, 0, sizeof(ot_venc_chn_attr));

    pstVencChnAttr->venc_attr.type = enEncType;
    pstVencChnAttr->venc_attr.max_pic_width = u32Width;
    pstVencChnAttr->venc_attr.max_pic_height = u32Height;
    pstVencChnAttr->venc_attr.pic_width = u32Width;
    pstVencChnAttr->venc_attr.pic_height = u32Height;
    pstVencChnAttr->venc_attr.buf_size = SAL_align(u32Width * u32Height * 3 / 2, 64);
    pstVencChnAttr->venc_attr.profile = 0; /*base line*/
    pstVencChnAttr->venc_attr.is_by_frame = TD_TRUE;

    /*不用区分264还是265，数据结构是一样的*/
    pstVencChnAttr->venc_attr.h264_attr.frame_buf_ratio = 100;
    pstVencChnAttr->venc_attr.h264_attr.rcn_ref_share_buf_en = 0;

    VENC_LOGI("chan %d venc_attr:type %d res %dx%d->%dx%d buf size %#x profile %d by frm %d ratio %d %d\n", u32Chan,
              pstVencChnAttr->venc_attr.type,
              pstVencChnAttr->venc_attr.max_pic_width,
              pstVencChnAttr->venc_attr.max_pic_height,
              pstVencChnAttr->venc_attr.pic_width,
              pstVencChnAttr->venc_attr.pic_height,
              pstVencChnAttr->venc_attr.buf_size,
              pstVencChnAttr->venc_attr.profile,
              pstVencChnAttr->venc_attr.is_by_frame,
              pstVencChnAttr->venc_attr.h264_attr.frame_buf_ratio,
              pstVencChnAttr->venc_attr.h264_attr.rcn_ref_share_buf_en);

    u32ViFps = (pstHalPrm->fps >> 16); /*高16位源fps，低16位编码fps*/
    u32EncFps = (pstHalPrm->fps & 0xffff);

    if (0 == pstHalPrm->bpsType)
    {
        /* 默认使用变码率 */
        if (OT_PT_H264 == enEncType)
        {
            pstVencChnAttr->rc_attr.rc_mode = OT_VENC_RC_MODE_H264_VBR;
            pstVencChnAttr->rc_attr.h264_vbr.max_bit_rate = pstHalPrm->bps;
            pstVencChnAttr->rc_attr.h264_vbr.stats_time = 6;    /* 10秒统计一次码率 */

            pstVencChnAttr->rc_attr.h264_vbr.src_frame_rate = (0 == u32ViFps) ? 60 : u32ViFps;
            pstVencChnAttr->rc_attr.h264_vbr.dst_frame_rate = u32EncFps > 0 ? u32EncFps : 30; /* 30;//(0 == pstHalPrm->uiVStandard)?(MAX_FPS_P_STANDARD):(MAX_FPS_N_STANDARD); */
            pstVencChnAttr->rc_attr.h264_vbr.gop = pstVencChnAttr->rc_attr.h264_vbr.dst_frame_rate * 2;
            VENC_LOGI("create H264 VBR chn %d bps %d, stats_time %d, fps is %d->%d, gop %d\n", u32Chan,
                      pstVencChnAttr->rc_attr.h264_vbr.max_bit_rate,
                      pstVencChnAttr->rc_attr.h264_vbr.stats_time,
                      pstVencChnAttr->rc_attr.h264_vbr.src_frame_rate,
                      pstVencChnAttr->rc_attr.h264_vbr.dst_frame_rate,
                      pstVencChnAttr->rc_attr.h264_vbr.gop);
        }
        else if (OT_PT_H265 == enEncType)
        {
            pstVencChnAttr->rc_attr.rc_mode = OT_VENC_RC_MODE_H265_VBR;
            pstVencChnAttr->rc_attr.h265_vbr.max_bit_rate = pstHalPrm->bps;
            pstVencChnAttr->rc_attr.h265_vbr.stats_time = 10;  /* 10秒统计一次码率 */

            pstVencChnAttr->rc_attr.h265_vbr.src_frame_rate = (0 == u32ViFps) ? 60 : u32ViFps; /* (0 == pstHalPrm->standard)?(MAX_VENC_FPS_P_STANDARD):(MAX_VENC_FPS_N_STANDARD); */
            pstVencChnAttr->rc_attr.h265_vbr.dst_frame_rate = u32EncFps > 0 ? u32EncFps : 30; /* 30;//(0 == pstHalPrm->uiVStandard)?(MAX_FPS_P_STANDARD):(MAX_FPS_N_STANDARD); */
            pstVencChnAttr->rc_attr.h265_vbr.gop = pstVencChnAttr->rc_attr.h264_vbr.dst_frame_rate * 2;
            VENC_LOGI("create H265 VBR chn %d bps %d, stats_time %d, fps is %d->%d, gop %d\n", u32Chan,
                      pstVencChnAttr->rc_attr.h265_vbr.max_bit_rate,
                      pstVencChnAttr->rc_attr.h265_vbr.stats_time,
                      pstVencChnAttr->rc_attr.h265_vbr.src_frame_rate,
                      pstVencChnAttr->rc_attr.h265_vbr.dst_frame_rate,
                      pstVencChnAttr->rc_attr.h265_vbr.gop);
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
        if (OT_PT_H264 == enEncType)
        {
            pstVencChnAttr->rc_attr.rc_mode = OT_VENC_RC_MODE_H264_CBR;
            pstVencChnAttr->rc_attr.h264_cbr.bit_rate = pstHalPrm->bps;
            pstVencChnAttr->rc_attr.h264_cbr.stats_time = 2;
            pstVencChnAttr->rc_attr.h264_cbr.src_frame_rate = (0 == u32ViFps) ? 60 : u32ViFps; /* (0 == pstHalPrm->standard)?(MAX_VENC_FPS_P_STANDARD):(MAX_VENC_FPS_N_STANDARD); */
            pstVencChnAttr->rc_attr.h264_cbr.dst_frame_rate = u32EncFps > 0 ? u32EncFps : 30; /* 30;//(0 == pstHalPrm->uiVStandard)?(MAX_FPS_P_STANDARD):(MAX_FPS_N_STANDARD); */
            pstVencChnAttr->rc_attr.h264_cbr.gop = pstVencChnAttr->rc_attr.h264_cbr.dst_frame_rate * 2;
            VENC_LOGI("create H264 CBR chn %d bps %d, stats_time %d, fps is %d->%d, gop %d\n", u32Chan,
                      pstVencChnAttr->rc_attr.h264_cbr.bit_rate,
                      pstVencChnAttr->rc_attr.h264_cbr.stats_time,
                      pstVencChnAttr->rc_attr.h264_cbr.src_frame_rate,
                      pstVencChnAttr->rc_attr.h264_cbr.dst_frame_rate,
                      pstVencChnAttr->rc_attr.h264_cbr.gop);
        }
        else if (OT_PT_H265 == enEncType)
        {
            pstVencChnAttr->rc_attr.rc_mode = OT_VENC_RC_MODE_H265_CBR;
            pstVencChnAttr->rc_attr.h265_cbr.bit_rate = pstHalPrm->bps;
            pstVencChnAttr->rc_attr.h265_cbr.stats_time = 2;
            pstVencChnAttr->rc_attr.h265_cbr.src_frame_rate = (0 == u32ViFps) ? 60 : u32ViFps; /* (0 == pstHalPrm->standard)?(MAX_VENC_FPS_P_STANDARD):(MAX_VENC_FPS_N_STANDARD); */
            pstVencChnAttr->rc_attr.h265_cbr.dst_frame_rate = u32EncFps > 0 ? u32EncFps : 30; /* 30;//(0 == pstHalPrm->uiVStandard)?(MAX_FPS_P_STANDARD):(MAX_FPS_N_STANDARD); */
            pstVencChnAttr->rc_attr.h265_cbr.gop = pstVencChnAttr->rc_attr.h265_cbr.dst_frame_rate * 2;

            VENC_LOGI("create H265 CBR chn %d bps %d, stats_time %d, fps is %d->%d, gop %d\n", u32Chan,
                      pstVencChnAttr->rc_attr.h265_cbr.bit_rate,
                      pstVencChnAttr->rc_attr.h265_cbr.stats_time,
                      pstVencChnAttr->rc_attr.h265_cbr.src_frame_rate,
                      pstVencChnAttr->rc_attr.h265_cbr.dst_frame_rate,
                      pstVencChnAttr->rc_attr.h265_cbr.gop);

        }
        else
        {
            VENC_LOGE(" encType %d(%d) error only support h264/h265....\n", enEncType, pstHalPrm->encodeType);
            goto err;
        }
    }

    pstVencChnAttr->gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
    pstVencChnAttr->gop_attr.normal_p.ip_qp_delta = 0; /*I帧相对P帧的QP差值，-10-30*/

    s32Ret = ss_mpi_venc_create_chn((ot_venc_chn)pstVencHalChnPrm->uiChn, pstVencChnAttr);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("VENC Module %d chan %d Create faild with %#x! ===\n", u32Chan, pstVencHalChnPrm->uiChn, s32Ret);
        goto err;
    }

    SAL_clear(&stRcParam);
    s32Ret = ss_mpi_venc_get_rc_param((ot_venc_chn)pstVencHalChnPrm->uiChn, &stRcParam);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_GetRcParam [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        goto err;
    }

    if (0 == pstHalPrm->bpsType)
    {
        if (OT_PT_H264 == enEncType)
        {
            stRcParam.h264_vbr_param.min_qp = 10;
            stRcParam.h264_vbr_param.max_qp = 50;
            stRcParam.h264_vbr_param.min_i_qp = 10;
            stRcParam.h264_vbr_param.max_i_qp = 50;
            stRcParam.h264_vbr_param.min_i_proportion = 1;
            stRcParam.h264_vbr_param.max_i_proportion = 100;
            VENC_LOGI("vbr pQp[%d,%d] IQp[%d,%d],Iprop [%d,%d] !!!\n", stRcParam.h264_vbr_param.min_qp, stRcParam.h264_vbr_param.max_qp,
                      stRcParam.h264_vbr_param.min_i_qp, stRcParam.h264_vbr_param.max_i_qp,
                      stRcParam.h264_vbr_param.min_i_proportion, stRcParam.h264_vbr_param.max_i_proportion);
        }
        else if (OT_PT_H265 == enEncType)
        {
            stRcParam.h265_vbr_param.min_qp = 10;
            stRcParam.h265_vbr_param.min_i_qp = 10;
            stRcParam.h265_vbr_param.max_qp = 40;
        }
        else
        {
            VENC_LOGE(" encType %d(%d) error only support h264/h265....\n", enEncType, pstHalPrm->encodeType);
            goto err;
        }
    }

    s32Ret = ss_mpi_venc_set_rc_param((ot_venc_chn)pstVencHalChnPrm->uiChn, &stRcParam);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("HI_MPI_VENC_SetRcParam [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
        goto err;
    }

    if (OT_PT_H264 == enEncType)
    {
        s32Ret = ss_mpi_venc_get_h264_vui((ot_venc_chn)pstVencHalChnPrm->uiChn, &stVui);
        if (TD_SUCCESS != s32Ret)
        {
            VENC_LOGE("HI_MPI_VENC_GetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
            goto err;
        }

        /* 这里设置时间戳的单位 60K时钟  80K时钟  90K时钟，具体值需要和海思确认过 */
        stVui.vui_time_info.timing_info_present_flag = 1;
        stVui.vui_time_info.time_scale = 60000;
        stVui.vui_time_info.num_units_in_tick = 30000 / (pstVencChnAttr->rc_attr.h264_cbr.dst_frame_rate);
        s32Ret = ss_mpi_venc_set_h264_vui((ot_venc_chn)pstVencHalChnPrm->uiChn, &stVui);
        if (TD_SUCCESS != s32Ret)
        {
            VENC_LOGE("HI_MPI_VENC_SetH264Vui [%d] faild with %#x! ===\n", pstVencHalChnPrm->uiChn, s32Ret);
            goto err;
        }
    }

    pstVencHalChnPrm->uiFd = ss_mpi_venc_get_fd((ot_venc_chn)pstVencHalChnPrm->uiChn);

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
    pstStream->pack = (ot_venc_pack *)SAL_memMalloc(sizeof(ot_venc_pack) * FRAME_MAX_NALU_NUM, "venc", "hisi_enc");
    if (NULL == pstStream->pack)
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
 * @function   venc_ssV5_delete
 * @brief    销毁编码器
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_delete(void *pHandle)
{
    INT32 s32Ret = SAL_SOK;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    s32Ret = venc_ssV5_stop(pHandle);
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
 * @function   venc_ssV5_getChan
 * @brief    获取plat层通道venc_hal_getVencHalChan
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] UINT32 *poutChan plat层通道号指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */

INT32 venc_ssV5_getChan(void *pHandle, UINT32 *pOutChan)
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

void sample_yuv_8bit_dump(ot_video_frame *pstVBuf, FILE *pfd)
{
    UINT32 u32W = 0;
    UINT32 u32H = 0;
    char *pVBufVirtY = NULL;
    char *pVBufVirtC = NULL;
    char *pMemContent = NULL;
    unsigned char *TmpBuff = NULL; /* If this value is too small and the image is big, this memory may not be enough */
    HI_U64 u64PhyAddr;
    ot_pixel_format enPixelFormat = pstVBuf->pixel_format;
    HI_U32 u32UvHeight = 0; /*When the storage format is a planar format, this variable is used to keep the height of the UV component */
    HI_U32 u32Size = 0;
    HI_CHAR *pUserPageAddr[2] = {HI_NULL, HI_NULL};

    if (TmpBuff == NULL)
    {
        TmpBuff = malloc(1920 * 1080 * 2);
    }

    if ((OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420 == enPixelFormat)
        || (OT_PIXEL_FORMAT_YUV_SEMIPLANAR_420 == enPixelFormat))
    {
        u32Size = (pstVBuf->stride[0]) * (pstVBuf->height) * 3 / 2;
        u32UvHeight = pstVBuf->height / 2;
    }
    else if ((OT_PIXEL_FORMAT_YVU_SEMIPLANAR_422 == enPixelFormat)
             || (OT_PIXEL_FORMAT_YUV_SEMIPLANAR_422 == enPixelFormat))
    {
        u32Size = (pstVBuf->stride[0]) * (pstVBuf->height) * 2;
        u32UvHeight = pstVBuf->height;
    }
    else if (OT_PIXEL_FORMAT_YUV_400 == enPixelFormat)
    {
        u32Size = (pstVBuf->stride[0]) * (pstVBuf->height);
        u32UvHeight = pstVBuf->height;
    }

    u64PhyAddr = pstVBuf->virt_addr[0];

    pUserPageAddr[0] = (HI_CHAR *)u64PhyAddr; /* /(HI_CHAR*) HI_MPI_SYS_Mmap(u64PhyAddr, u32Size); */

    if (HI_NULL == pUserPageAddr[0])
    {
        return;
    }

    pVBufVirtY = pUserPageAddr[0];
    pVBufVirtC = pVBufVirtY + (pstVBuf->stride[0]) * (pstVBuf->height);

    /* save Y ----------------------------------------------------------------*/
    printf("saving......Y......wxh[%d,%d],s %d,pF %d\n", pstVBuf->width, pstVBuf->height, pstVBuf->stride[0], enPixelFormat);
    fflush(stdout);

    for (u32H = 0; u32H < pstVBuf->height; u32H++)
    {
        pMemContent = pVBufVirtY + u32H * pstVBuf->stride[0];
        fwrite(pMemContent, pstVBuf->width, 1, pfd);
    }

    if (OT_PIXEL_FORMAT_YUV_400 != enPixelFormat)
    {
        fflush(pfd);
        /* save v ----------------------------------------------------------------*/
        printf("v.....u32Stride %d.", pstVBuf->stride[1]);
        fflush(stdout);

        for (u32H = 0; u32H < u32UvHeight; u32H++)
        {
            pMemContent = pVBufVirtC + u32H * pstVBuf->stride[1];

            pMemContent += 1;

            for (u32W = 0; u32W < pstVBuf->width / 2; u32W++)
            {
                TmpBuff[u32W] = *pMemContent;
                pMemContent += 2;
            }

            fwrite(TmpBuff, pstVBuf->width / 2, 1, pfd);
        }

        fflush(pfd);

        /* save U ----------------------------------------------------------------*/
        fprintf(stderr, "u......");
        fflush(stderr);

        for (u32H = 0; u32H < u32UvHeight; u32H++)
        {
            pMemContent = pVBufVirtC + u32H * pstVBuf->stride[1];

            for (u32W = 0; u32W < pstVBuf->width / 2; u32W++)
            {
                TmpBuff[u32W] = *pMemContent;
                pMemContent += 2;
            }

            fwrite(TmpBuff, pstVBuf->width / 2, 1, pfd);
        }
    }

    fflush(pfd);

    fprintf(stderr, "done %d!\n", pstVBuf->time_ref);
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
 * @param[in]  hi_crop_info *pstCropInfo crop信息
 * @param[out]  UINT32 *pJpegSize 编码码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_setJpegCropPrm(void *pHandle, ot_crop_info *pstCropInfo)
{
    td_s32 s32Ret = TD_SUCCESS;
    ot_venc_chn_param stChnParam = {0};
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if (NULL == pstCropInfo || SAL_TRUE != pstCropInfo->enable)
    {
        return SAL_SOK;
    }

    s32Ret = ss_mpi_venc_get_chn_param((ot_venc_chn)pstVencHalChnPrm->uiChn, &stChnParam);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("Venc Chn %d Get Chn Attr Failed, ret: 0x%x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    stChnParam.crop_info.enable = pstCropInfo->enable;
    stChnParam.crop_info.rect.x = OT_ALIGN_UP(pstCropInfo->rect.x, 16);
    stChnParam.crop_info.rect.y = OT_ALIGN_UP(pstCropInfo->rect.y, 16);
    stChnParam.crop_info.rect.width = OT_ALIGN_UP(pstCropInfo->rect.width, 4);
    stChnParam.crop_info.rect.height = OT_ALIGN_UP(pstCropInfo->rect.height, 4);

    s32Ret = ss_mpi_venc_set_chn_param((ot_venc_chn)pstVencHalChnPrm->uiChn, &stChnParam);
    if (TD_SUCCESS != s32Ret)
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
    td_s32 s32Ret = TD_SUCCESS;
    INT32 i = 0;
    UINT32 u32Size = 0;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    ot_venc_stream stStream = {0};
    ot_venc_chn_status stStat = {0};
    ot_venc_pack *pstData = NULL;;

    UINT32 QueryCount = 0;

    if ((NULL == pHandle) || (NULL == pcJpeg) || (NULL == pJpegSize))
    {
        VENC_LOGE("inv ptr\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    memset(&stStat, 0, sizeof(ot_venc_chn_status));

    while (0 == stStat.cur_packs || 0 == stStat.left_stream_frames)
    {
        s32Ret = ss_mpi_venc_query_status((ot_venc_chn)pstVencHalChnPrm->uiChn, &stStat);
        if (TD_SUCCESS != s32Ret)
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

    stStream.pack = pstVencHalChnPrm->pstVirtPack;
    if (NULL == stStream.pack)
    {
        VENC_LOGE("malloc stream pack failed!\n");
        return SAL_FAIL;
    }

    stStream.pack_cnt = stStat.cur_packs;

    s32Ret = ss_mpi_venc_get_stream((ot_venc_chn)pstVencHalChnPrm->uiChn, &stStream, -1);
    if (TD_SUCCESS != s32Ret)
    {
        stStream.pack = NULL;
        VENC_LOGE("chn %d HI_MPI_VENC_GetStream failed with %#x!\n", pstVencHalChnPrm->uiChn, s32Ret);
        return SAL_FAIL;
    }

    u32Size = 0;
    for (i = 0; i < stStream.pack_cnt; i++)
    {
        pstData = &stStream.pack[i];

        if (pstData->len > 0 && pstData->len > pstData->offset)
        {
            memcpy(pcJpeg + u32Size,
                   pstData->addr + pstData->offset,
                   pstData->len - pstData->offset);

            u32Size += pstData->len - pstData->offset;
        }
    }

    *pJpegSize = u32Size;
    s32Ret = ss_mpi_venc_release_stream((ot_venc_chn)pstVencHalChnPrm->uiChn, &stStream);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("VENC release fail!\n");
        stStream.pack = NULL;
        return SAL_FAIL;
    }

    memset(pstVencHalChnPrm->pstVirtPack, 0x00, pstVencHalChnPrm->packBufLen);
    stStream.pack = NULL;

    return SAL_SOK;
}

/**
 * @function   venc_ssV5_encJpeg
 * @brief    获取plat层通道venc_hal_getVencHalChan
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 帧信息
 * @param[in]  CROP_S *pstCropInfo 编码crop信息
 * @param[in]  BOOL bSetPrm 编码参数更新，用于重配编码器
 * @param[out] INT8 *pJpeg 编码输出JPEG码流
 * @param[out] INT8 *pJpegSize 编码输出JPEG码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_encJpeg(void *pHandle, SYSTEM_FRAME_INFO *pstFrame, INT8 *pJpeg, UINT32 *pJpegSize, CROP_S *pstCropInfo, BOOL bSetPrm)
{
    VENC_HAL_CHECK_PRM(pstFrame, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pJpeg, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pJpegSize, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pHandle, SAL_FAIL);

    ot_video_frame_info *pstFrameInfo = (ot_video_frame_info *)pstFrame->uiAppData;
    VENC_HAL_CHECK_PRM(pstFrameInfo, SAL_FAIL);

    SAL_VideoFrameParam stJpegPicInfo = {0};
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    ot_crop_info stCropInfo = {0};

    if ((SAL_TRUE == bSetPrm) || (NULL != pstCropInfo)) /*抠图模式必然要设置参数*/
    {
        stJpegPicInfo.quality = 99; /* 80; */

        if (NULL != pstCropInfo)
        {
            stCropInfo.enable = pstCropInfo->u32CropEnable;
            stCropInfo.rect.height = (td_u32)pstCropInfo->u32H;
            stCropInfo.rect.width = (td_u32)pstCropInfo->u32W;
            stCropInfo.rect.x = (td_s32)pstCropInfo->u32X;
            stCropInfo.rect.y = (td_s32)pstCropInfo->u32Y;

            stJpegPicInfo.height = stCropInfo.rect.height;
            stJpegPicInfo.width = stCropInfo.rect.width;
        }
        else
        {
            stCropInfo.enable = SAL_FALSE;
            stCropInfo.rect.height = (td_u32)pstFrameInfo->video_frame.height;
            stCropInfo.rect.width = (td_u32)pstFrameInfo->video_frame.width;
            stCropInfo.rect.x = (td_s32)0;
            stCropInfo.rect.y = (td_s32)0;
            stJpegPicInfo.height = pstFrameInfo->video_frame.height;
            stJpegPicInfo.width = pstFrameInfo->video_frame.width;
        }

        VENC_LOGI("crop %d res %d %d %d %d!\n", stCropInfo.enable,
                  stCropInfo.rect.x, stCropInfo.rect.y, stCropInfo.rect.width, stCropInfo.rect.height);

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

    if (SAL_SOK != venc_ssV5_start(pHandle))
    {
        VENC_LOGE("VencHal_drvStart error !\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_sendFrame(pHandle, pstFrameInfo))
    {
        VENC_LOGE("venc_sendFrame error !\n");
        venc_ssV5_stop(pHandle);
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_getJpegFrame(pHandle, pJpeg, pJpegSize))
    {
        VENC_LOGE("venc_getJpegFrame error !\n");
        venc_ssV5_stop(pHandle);
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_ssV5_stop(pHandle))
    {
        VENC_LOGE("venc_ssV5_stop error !\n");
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
    ot_venc_chn_attr *pstVencChnAttr = NULL;

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

    pstVencHalChnPrm = &g_stVencInfo.stVencHalChnPrm[u32HalChan];
    pstVencHalChnPrm->uiChn = u32HalChan;

    pstVencChnAttr = &pstVencHalChnPrm->stVencChnAttr;
    memset(pstVencChnAttr, 0, sizeof(ot_venc_chn_attr));

    /* 配置通道属性 */
    pstVencChnAttr->venc_attr.type = OT_PT_JPEG;
    pstVencChnAttr->venc_attr.pic_width = pstInPrm->width;
    pstVencChnAttr->venc_attr.pic_height = pstInPrm->height;
    pstVencChnAttr->venc_attr.max_pic_width = VENC_JPEG_MAX_WIDTH; 
    pstVencChnAttr->venc_attr.max_pic_height = VENC_JPEG_MAX_HEIGHT;

    /* 需要 64 对齐 */
    pstVencChnAttr->venc_attr.buf_size = SAL_align((pstVencChnAttr->venc_attr.max_pic_width * pstVencChnAttr->venc_attr.max_pic_height * 3 / 2), 64);
    pstVencChnAttr->venc_attr.profile = 0;
    pstVencChnAttr->venc_attr.is_by_frame = TD_TRUE;
    pstVencChnAttr->venc_attr.jpeg_attr.dcf_en = TD_FALSE;
    pstVencChnAttr->venc_attr.jpeg_attr.mpf_cfg.large_thumbnail_num = 0;
    pstVencChnAttr->venc_attr.jpeg_attr.recv_mode = OT_VENC_PIC_RECV_SINGLE;
    pstVencChnAttr->gop_attr.gop_mode = OT_VENC_GOP_MODE_NORMAL_P;
    pstVencChnAttr->gop_attr.normal_p.ip_qp_delta = 0;

    VENC_LOGI("JPEG Chn enc_%d  W : %d H : %d !!!\n", pstVencHalChnPrm->uiChn, pstInPrm->width, pstInPrm->height);
    s32Ret = ss_mpi_venc_create_chn((ot_venc_chn)pstVencHalChnPrm->uiChn, pstVencChnAttr);
    if (TD_SUCCESS != s32Ret)
    {
        VENC_LOGE("Hal Create Jpeg [%d] faild with %#x !!!\n", pstVencHalChnPrm->uiChn, s32Ret);
        venc_putGroup(u32HalChan);
        return SAL_FAIL;
    }

    /* HI_MPI_VENC_SetMaxStreamCnt(pstVencHalChnPrm->uiChn, 1); */

    pstVencHalChnPrm->uiFd = ss_mpi_venc_get_fd((ot_venc_chn)pstVencHalChnPrm->uiChn);
    pstVencHalChnPrm->uibStart = SAL_FALSE;
    pstVencHalChnPrm->packBufLen = SAL_KB * 4;
    pstVencHalChnPrm->pstVirtPack = (ot_venc_pack *)SAL_memMalloc(pstVencHalChnPrm->packBufLen, "jpeg", "hisi_enc");
    if (NULL == pstVencHalChnPrm->pstVirtPack)
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
 * @function   venc_ssV5_setEncPrm
 * @brief    设置编码参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]   SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_setEncPrm(void *ppHandle, SAL_VideoFrameParam *pstInPrm)
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

    if (OT_PT_JPEG == pstVencHalChnPrm->stVencChnAttr.venc_attr.type)
    {
        memset(&stVencFrmPrm, 0, sizeof(SAL_VideoFrameParam));

        stVencFrmPrm.width = pstInPrm->width;
        stVencFrmPrm.height = pstInPrm->height;
        stVencFrmPrm.quality = pstInPrm->quality;

        if (SAL_SOK != venc_ssV5_stop(ppHandle))
        {
            VENC_LOGE("!!!\n");
            return SAL_FAIL;
        }

        if (SAL_SOK != venc_setJpegPrm(pstVencHalChnPrm->uiChn, &stVencFrmPrm))
        {
            VENC_LOGE("!!!\n");
            return SAL_FAIL;
        }

        if (SAL_SOK != venc_ssV5_start(ppHandle))
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
 * @function   venc_ssV5_create
 * @brief    创建视频硬件编码器
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] void **ppHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_ssV5_create(void **ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_FAIL;

    if ((NULL == pstInPrm) || (ppHandle == NULL))
    {
        VENC_LOGE("Prm is NULL, Create Failed !!!\n");
        return SAL_FAIL;
    }

    if (OT_PT_JPEG == venc_getEncType(pstInPrm->encodeType))
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
INT32 venc_ssV5_init(void)
{
    /* 初始化通道管理锁 */
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &g_stVencInfo.MutexHandle);
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
    pstVencPlatOps->Init = venc_ssV5_init;
    pstVencPlatOps->Create = venc_ssV5_create;
    pstVencPlatOps->Delete = venc_ssV5_delete;
    pstVencPlatOps->Start = venc_ssV5_start;
    pstVencPlatOps->Stop = venc_ssV5_stop;
    pstVencPlatOps->SetParam = venc_ssV5_setEncPrm;
    pstVencPlatOps->ForceIFrame = venc_ssV5_requestIDR;
    pstVencPlatOps->GetFrame = venc_ssV5_getFrame;
    pstVencPlatOps->PutFrame = venc_ssV5_putFrame;
    pstVencPlatOps->GetHalChn = venc_ssV5_getChan;
    pstVencPlatOps->SetVencStatues = venc_ssV5_setStatus;
    pstVencPlatOps->EncJpegProcess = venc_ssV5_encJpeg;
    pstVencPlatOps->EncSendFrm = venc_ssV5_sendFrm;

    return SAL_SOK;
}

