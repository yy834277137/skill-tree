/**
 * @file   venc_nt9833x.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  编码组件---plat层接口封装
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        : 2021/12/28 create
     Author      : yeyanzhong
     Modification: nt98336 venc 平台层接口开发
 */

#include <sal.h>
#include <dspcommon.h>
#include <platform_sdk.h>
#include <platform_hal.h>
#include "venc_nt9833x.h"
#include "stream_bits_info_def.h"

#line __LINE__ "venc_nt9833x.c"

/*****************************************************************************
                            宏定义
*****************************************************************************/

#define HISI_SDK_STREAM_MAX_CNT (64)
#define CROP_VPSS_GROUP_WIDTH   (1200)
#define CROP_VPSS_GROUP_HEIGHT  (1920)
#define MAX_VENC_FPS_P_STANDARD (50)
#define MAX_VENC_FPS_N_STANDARD (60)

#define VENC_HAL_CHECK_PRM(ptr, value) {if (!ptr) {VENC_LOGE("Ptr (The address is empty or Value is 0 )\n"); return (value); }}

/*****************************************************************************
                            全局结构体
*****************************************************************************/

/*绑定方式属性，获取帧后续要释放内存*/
typedef struct tagVencHalChnInfoBindSt
{
    NT_VENC_SENDFRAME_MODE s32SendMode;       /*送帧模式0、绑定模式 1、送帧模式*/
    INT32 s32BufSize;
    void  *pFrameVirAddr;
    HD_VIDEOENC_BS stOutFrame;

}VENC_HAL_CHN_INFO_BINT_S;


/* 编码器通道属性 */
typedef struct tagVencHalChnInfoSt
{
    UINT32 uiChn;                           /* 编码器通道号    */
    UINT32 uibStart;                        /* 是否是start状态 */
    UINT32 uiState;                        /* 编码状态，0 正常状态，1等待状态 */
    UINT32 isCreate;                        /* 编码器被创建好 */

    HD_PATH_ID u32PathId;                   /* NT9833x使用 */
    HD_VIDEOENC_BS stPushInBs;              /* NT9833x jpeg抓图时使用 */
    CHAR *pStreamBuf;                      /* 用于存放venc硬核编码得到的码流，即供VENC底层库使用 */
    UINTPTR u64StreamBufPa;                 /* 缓冲区buf的物理地址 */
    UINT32 u32StreamBufSize;               /* streamBuff*/
    UINT32 u32frmW;                         /* NT98336里获取不到编码帧的宽高信息，所以创建VENC时进行保存 */
    UINT32 u32frmH;
    UINT32 vencFps;                         /* 编码帧率，NT98336里获取不到编码帧的帧率信息，所以创建VENC时进行保存 */
    ALLOC_VB_INFO_S stVencVbInfo;
    VENC_HAL_CHN_INFO_BINT_S stVencInfoBind;
} VENC_HAL_CHN_INFO_S;

/* 编码器模块编码全局属性与通道管理 */
typedef struct tagVencHalInfoSt
{
    void *MutexHandle;
    /* 编码器通道管理 通道号对应数组下标,支持 VENC_NT9833X_CHN_NUM 个 */
    VENC_HAL_CHN_INFO_S stVencHalChnPrm[VENC_NT9833X_CHN_NUM];
} VENC_HAL_INFO_S;

/*****************************************************************************
                            全局结构体
*****************************************************************************/

static VENC_HAL_INFO_S gstVencInfo = {0};

/* 记录 venc group 使用情况 */
static UINT32 isVencGroupUsed = 0;
static UINT64 streamPTS[VENC_NT9833X_CHN_NUM] = {0};

/* #define VENC_HAL_SAVE_BITS_DATA */

#ifdef VENC_HAL_SAVE_BITS_DATA
/* 测试用 */
FILE *file[VENC_NT9833X_CHN_NUM];
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
static HD_VIDEO_CODEC venc_getEncType(UINT32 u32InType)
{
    switch (u32InType)
    {
        case MJPEG:
            return HD_CODEC_TYPE_JPEG;
        case H265:
            return HD_CODEC_TYPE_H265;
        default:
            return HD_CODEC_TYPE_H264;
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

    if (VENC_NT9833X_CHN_NUM > 32)
    {
        VENC_LOGE("HAL venc pchn overflow %d\n", VENC_NT9833X_CHN_NUM);
        return SAL_FAIL;
    }

    SAL_mutexLock(gstVencInfo.MutexHandle);
    for (i = 0; i < VENC_NT9833X_CHN_NUM; i++)
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
    if (i == VENC_NT9833X_CHN_NUM)
    {
        *pchn = 0xff;
        SAL_mutexUnlock(gstVencInfo.MutexHandle);
        return SAL_FAIL;
    }

    SAL_mutexUnlock(gstVencInfo.MutexHandle);
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
    SAL_mutexLock(gstVencInfo.MutexHandle);
    /* 编码器消耗后回收通道 */
    if (isVencGroupUsed & (1 << u32Chan))
    {
        isVencGroupUsed = isVencGroupUsed & (~(1 << u32Chan));
    }

    SAL_mutexUnlock(gstVencInfo.MutexHandle);
}

#if 0
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
#endif
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
 * @function   venc_nt9833x_requestIDR
 * @brief    强制I帧编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_nt9833x_requestIDR(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    HD_H26XENC_REQUEST_IFRAME stReqIfme = {0};
    HD_RESULT enRet;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;   
    stReqIfme.enable = 1;
    enRet = hd_videoenc_set(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &stReqIfme);
    if (enRet != HD_OK) {
        printf("set HD_VIDEOENC_PARAM_IN fail\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_nt9833x_putFrame
 * @brief    释放指定通道的码流缓存，和VencHal_getFrame 成对出现
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_nt9833x_putFrame(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    INT32 s32Ret = SAL_SOK;

    if ((NULL == pHandle))
    {
        VENC_LOGE("Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if(pstVencHalChnPrm->stVencInfoBind.s32SendMode == NT_VENC_FRAME_PUT)
    {
        return SAL_SOK;
    }
    
    if(pstVencHalChnPrm->stVencInfoBind.pFrameVirAddr != NULL)
    {
        s32Ret = hd_common_mem_munmap(pstVencHalChnPrm->stVencInfoBind.pFrameVirAddr, pstVencHalChnPrm->stVencInfoBind.s32BufSize);
        if (s32Ret != HD_OK) 
        {
            VENC_LOGE("hd_common_mem_munmap fail, va=0x%p, size=%u\r\n", pstVencHalChnPrm->stVencInfoBind.pFrameVirAddr, pstVencHalChnPrm->stVencInfoBind.s32BufSize);
            return SAL_FAIL;
        }
        else
        {
            pstVencHalChnPrm->stVencInfoBind.pFrameVirAddr = NULL;
        }
    }

    s32Ret= hd_videoenc_release_out_buf(pstVencHalChnPrm->u32PathId, &pstVencHalChnPrm->stVencInfoBind.stOutFrame);
    if (s32Ret != HD_OK)
    {
        VENC_LOGE("hd_videoproc_release_out_buf fail\n");
        return SAL_FAIL;
    }
       
    return SAL_SOK;
}

/**
 * @function   venc_nt9833x_getFrame
 * @brief    从指定的编码通道获取最新的一帧编码码流，保证和VencHal_putFrame成对调用
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] void *pInPrm 获取编码码流
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
 #if 0
INT32 venc_nt9833x_getFrame(void *pHandle, void *pInPrm)
{
    INT32 i = 0;  

    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    STREAM_FRAME_INFO_ST *pstFrameInfo = NULL;
    HD_VIDEOENC_POLL_LIST poll_list[1];
    HD_VIDEOENC_RECV_LIST recv_list[1];
    HD_RESULT enRet = 0;

    if ((NULL == pHandle) || (NULL == pInPrm))
    {
        VENC_LOGE("Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstFrameInfo = (STREAM_FRAME_INFO_ST *)pInPrm;

    /* 获取码流前先清0标志位 */
    pstFrameInfo->uiValid = 0;
    pstFrameInfo->uiNaluNum = 0;


    memset(poll_list, 0, sizeof(poll_list));
    poll_list[0].path_id = pstVencHalChnPrm->u32PathId;
    do
    {
        enRet = hd_videoenc_poll_list(poll_list, 1, 1000);
        if (enRet == HD_ERR_TIMEDOUT) {
            VENC_LOGI("Poll timeout!!");
            continue;
        }

        if (poll_list[0].revent.event != TRUE) {
            continue;
        }
        if (poll_list[0].revent.bs_size > pstVencHalChnPrm->u32StreamBufSize) {
            printf("bitstream buffer bs_size is not enough! %u, %d\n",
                   poll_list[i].revent.bs_size, pstVencHalChnPrm->u32StreamBufSize);
            continue;
        }
        
        memset(recv_list, 0, sizeof(recv_list));
        recv_list[0].path_id = poll_list[0].path_id;
        recv_list[0].user_bs.p_user_buf = pstVencHalChnPrm->pStreamBuf;
        recv_list[0].user_bs.user_buf_size = pstVencHalChnPrm->u32StreamBufSize;

        if ((enRet = hd_videoenc_recv_list(recv_list, 1)) < 0) {
            printf("Error return value %d\n", enRet);
        } 
        else 
        {
            if ((recv_list[0].retval < 0) && recv_list[0].path_id) 
            {
                printf("path_id(0x%x): Error to receive bitstream. ret=%d\n",
                       recv_list[0].path_id, recv_list[0].retval);
            } 
            else if (recv_list[0].retval >= 0) 
            {

                for (i = 0; i < recv_list[0].user_bs.pack_num; i++) 
                {
                    //bs_size += recv_list[0].user_bs.video_pack[u32PackNum].size;
                    pstFrameInfo->stNaluInfo[i].pucNaluPtr = (UINT8 *)recv_list[0].user_bs.video_pack[i].user_buf_addr;
                    pstFrameInfo->stNaluInfo[i].uiNaluLen = recv_list[0].user_bs.video_pack[i].size;
                    pstFrameInfo->stNaluInfo[i].u64PTS = recv_list[0].user_bs.timestamp / 1000;

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
                        if (streamPTS[pstVencHalChnPrm->uiChn] > pstFrameInfo->stNaluInfo[i].u64PTS)
                        {
                            UINT64 p64CurPts = 0;
                            //HI_MPI_SYS_GetCurPTS(&p64CurPts);
                            VENC_LOGI("p64CurPts is %llu old Pts %llu,cur Pts %llu\n", p64CurPts / 1000, streamPTS[pstVencHalChnPrm->uiChn], pstFrameInfo->stNaluInfo[i].u64PTS);
                            pstFrameInfo->stNaluInfo[i].u64PTS += 2;
                        }

                        streamPTS[pstVencHalChnPrm->uiChn] = pstFrameInfo->stNaluInfo[i].u64PTS;
                    }
                }
                
                /* NT98336从编码器里获取不到分辨率、帧率信息，所以把创建时的信息返回给上层 */
                pstFrameInfo->uiFrameWidth = pstVencHalChnPrm->u32frmH;
                pstFrameInfo->uiFrameHeight = pstVencHalChnPrm->u32frmW;
                pstFrameInfo->uiFps = pstVencHalChnPrm->vencFps;

                if (HD_CODEC_TYPE_H264 == recv_list[0].user_bs.vcodec_format)
                {
                    if ((HD_FRAME_TYPE_IDR == recv_list[0].user_bs.frame_type) || (HD_FRAME_TYPE_I == recv_list[0].user_bs.frame_type))
                    {
                        pstFrameInfo->eFrameType = STREAM_TYPE_H264_IFRAME;
                    }
                    else
                    {
                        pstFrameInfo->eFrameType = STREAM_TYPE_H264_PFRAME;
                    }
                }
                else if (HD_CODEC_TYPE_H265 == recv_list[0].user_bs.vcodec_format)
                {
                    if ((HD_FRAME_TYPE_IDR == recv_list[0].user_bs.frame_type) || (HD_FRAME_TYPE_I == recv_list[0].user_bs.frame_type))
                    {
                        pstFrameInfo->eFrameType = STREAM_TYPE_H265_IFRAME;
                    }
                    else
                    {
                        pstFrameInfo->eFrameType = STREAM_TYPE_H265_PFRAME;
                    }
                }
                else
                {
                    if ((HD_FRAME_TYPE_IDR == recv_list[0].user_bs.frame_type) || (HD_FRAME_TYPE_I == recv_list[0].user_bs.frame_type))
                    {
                        pstFrameInfo->eFrameType = STREAM_TYPE_H264_IFRAME;
                    }
                    else
                    {
                        pstFrameInfo->eFrameType = STREAM_TYPE_H264_PFRAME;
                    }

                    VENC_LOGW("vcodec_format error is %d\n", recv_list[0].user_bs.vcodec_format);
                }

                pstFrameInfo->uiValid = 1;
                return SAL_SOK;
            }
            else
            {
                VENC_LOGE("venc getFrame     error:   recv   retval %d, path_Id %d \n ", recv_list[0].retval, recv_list[0].path_id);
            }
        
        }

    }
    while (1);

    return SAL_SOK;
}
#endif


INT32 venc_nt9833x_getFrame(void *pHandle, void *pInPrm)
{
    INT32 i = 0;  
    INT32 stBufSize = 0;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    STREAM_FRAME_INFO_ST *pstFrameInfo = NULL;
    HD_VIDEOENC_BS *pVencBs = NULL;
    HD_RESULT enRet = 0;
    void *pVirAddr = NULL;

    if ((NULL == pHandle) || (NULL == pInPrm))
    {
        VENC_LOGE("Prm is NULL !!!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstFrameInfo = (STREAM_FRAME_INFO_ST *)pInPrm;
    pVencBs = &pstVencHalChnPrm->stVencInfoBind.stOutFrame;

   // memset(pVencBs, 0 , sizeof(HD_VIDEOENC_BS));
    /* 获取码流前先清0标志位 */
    pstFrameInfo->uiValid = 0;
    pstFrameInfo->uiNaluNum = 0;

    do
    {
        enRet = hd_videoenc_pull_out_buf(pstVencHalChnPrm->u32PathId, pVencBs, 1000);
        if (enRet < 0) 
        {
            printf("Error return value %d\n", enRet);
            return SAL_FAIL;
        } 
        else 
        {
            if(pstVencHalChnPrm->stVencInfoBind.s32SendMode == NT_VENC_FRAME_PUT)
            {
                for (i = 0; i < pVencBs->pack_num; i++) 
                {
                    pstFrameInfo->stNaluInfo[i].pucNaluPtr = (UINT8 *)(((UINT64)pVencBs->video_pack[i].phy_addr - pstVencHalChnPrm->u64StreamBufPa) + (UINT64)pstVencHalChnPrm->pStreamBuf);
                    pstFrameInfo->stNaluInfo[i].uiNaluLen = pVencBs->video_pack[i].size;
                    // pVencBs->video_pack[i].timestamp恒为0，所以使用SAL_getCurUs
                    pstFrameInfo->stNaluInfo[i].u64PTS = SAL_getCurUs() / 1000;
                    pstFrameInfo->uiNaluNum++;
    
                    if (i == 0)
                    {
                        if (streamPTS[pstVencHalChnPrm->uiChn] > pstFrameInfo->stNaluInfo[i].u64PTS)
                        {
                            VENC_LOGI("old Pts %llu,cur Pts %llu\n", streamPTS[pstVencHalChnPrm->uiChn], pstFrameInfo->stNaluInfo[i].u64PTS);
                            pstFrameInfo->stNaluInfo[i].u64PTS += 2;
                        }
    
                        streamPTS[pstVencHalChnPrm->uiChn] = pstFrameInfo->stNaluInfo[i].u64PTS;
                    }
                }
            }
            else
            {
                for (i = 0; i < pVencBs->pack_num; i++) 
                {
                    stBufSize += pVencBs->video_pack[i].size;
                  
                }

                /*多包情况合称一包*/
                i = 0;  
                pVirAddr = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, pVencBs->video_pack[i].phy_addr, stBufSize);
                if(pVirAddr == NULL)
                {
                     SAL_ERROR("phy_addr  %lx\n", pVencBs->video_pack[0].phy_addr);
                     return SAL_FAIL;
                }

                pstFrameInfo->stNaluInfo[i].pucNaluPtr = pVirAddr;
                pstVencHalChnPrm->stVencInfoBind.pFrameVirAddr = pstFrameInfo->stNaluInfo[i].pucNaluPtr;
                pstVencHalChnPrm->stVencInfoBind.s32BufSize = stBufSize;
                pstFrameInfo->stNaluInfo[i].uiNaluLen = stBufSize;
                pstFrameInfo->stNaluInfo[i].u64PTS = SAL_getCurUs() / 1000;
                pstFrameInfo->uiNaluNum++;

                if (streamPTS[pstVencHalChnPrm->uiChn] > pstFrameInfo->stNaluInfo[i].u64PTS)
                {
                    VENC_LOGI("old Pts %llu,cur Pts %llu\n", streamPTS[pstVencHalChnPrm->uiChn], pstFrameInfo->stNaluInfo[i].u64PTS);
                    pstFrameInfo->stNaluInfo[i].u64PTS += 2;
                }
                streamPTS[pstVencHalChnPrm->uiChn] = pstFrameInfo->stNaluInfo[i].u64PTS;
                
                
            }

            /* NT98336从编码器里获取不到分辨率、帧率信息，所以把创建时的信息返回给上层 */
           
            pstFrameInfo->uiFrameWidth =pstVencHalChnPrm->u32frmW; 
            pstFrameInfo->uiFrameHeight = pstVencHalChnPrm->u32frmH;
            pstFrameInfo->uiFps = pstVencHalChnPrm->vencFps;


            if (HD_CODEC_TYPE_H264 == pVencBs->vcodec_format)
            {
                if ((HD_FRAME_TYPE_IDR == pVencBs->frame_type) || (HD_FRAME_TYPE_I == pVencBs->frame_type))
                {
                    pstFrameInfo->eFrameType = STREAM_TYPE_H264_IFRAME;
                }
                else
                {
                    pstFrameInfo->eFrameType = STREAM_TYPE_H264_PFRAME;
                }
            }
            else if (HD_CODEC_TYPE_H265 == pVencBs->vcodec_format)
            {
                if ((HD_FRAME_TYPE_IDR == pVencBs->frame_type) || (HD_FRAME_TYPE_I == pVencBs->frame_type))
                {
                    pstFrameInfo->eFrameType = STREAM_TYPE_H265_IFRAME;
                }
                else
                {
                    pstFrameInfo->eFrameType = STREAM_TYPE_H265_PFRAME;
                }
            }
            else
            {
                if ((HD_FRAME_TYPE_IDR == pVencBs->frame_type) || (HD_FRAME_TYPE_I == pVencBs->frame_type))
                {
                    pstFrameInfo->eFrameType = STREAM_TYPE_H264_IFRAME;
                }
                else
                {
                    pstFrameInfo->eFrameType = STREAM_TYPE_H264_PFRAME;
                }

                VENC_LOGW("vcodec_format error is %d\n", pVencBs->vcodec_format);
            }

            pstFrameInfo->uiValid = 1;
            return SAL_SOK;
        }

    }
    while (1);

    return SAL_SOK;
}


/**
 * @function   venc_nt9833x_stop
 * @brief    停止编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_nt9833x_stop(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    HD_RESULT ret;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    if (1 == pstVencHalChnPrm->uibStart)
    {
        if ((ret = hd_videoenc_stop_list(&pstVencHalChnPrm->u32PathId, 1)) != HD_OK)
        {
            printf("start videoenc fail\n");
            return SAL_FAIL;
        }
        pstVencHalChnPrm->uibStart = 0;
    }

    return SAL_SOK;
}

/**
 * @function   venc_nt9833x_start
 * @brief    开始编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_nt9833x_start(void *pHandle)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    HD_RESULT ret;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;


    if (SAL_FALSE == pstVencHalChnPrm->uibStart)
    {
        if ((ret = hd_videoenc_start_list(&pstVencHalChnPrm->u32PathId, 1)) != HD_OK)
        {
            VENC_LOGE("hd_videoenc_start_list fail, ret:0x%x\n", ret);
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
    INT32 s32Ret = SAL_SOK;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    HD_RESULT ret = HD_OK;
    
    UINT32 u32VbBufSize;
    UINT64 u64VbBlk;
    UINT32 u32PoolId;
    
    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    
    u32VbBufSize = pstVencHalChnPrm->stVencVbInfo.u32Size;
    u64VbBlk     = pstVencHalChnPrm->stVencVbInfo.u64VbBlk;
    u32PoolId    = pstVencHalChnPrm->stVencVbInfo.u32PoolId;

    s32Ret = mem_hal_vbFree(pstVencHalChnPrm->pStreamBuf, "venc_nt9833x", "streamBuf", u32VbBufSize, u64VbBlk, u32PoolId);
    if (SAL_FAIL == s32Ret)
    {
        VENC_LOGE("mem_hal_vbFree failed size %u !\n", u32VbBufSize);
        return SAL_FAIL;
    }
    
    ret = hd_videoenc_close(pstVencHalChnPrm->u32PathId);
    if (ret != HD_OK) {
        VENC_LOGE("hd_videoenc_close fail\n");
        //return SAL_FAIL; /* 前面内存已经释放，即使close失败也把该编码通道状态置为uncreated */
    }

    /* 释放编码器通道 */
    venc_putGroup(pstVencHalChnPrm->uiChn);

    pstVencHalChnPrm->uibStart = SAL_FALSE;
    pstVencHalChnPrm->isCreate = SAL_FALSE;

    VENC_LOGI("VENC Chn HalChn %d Destroy success.\n", pstVencHalChnPrm->uiChn);
    return SAL_SOK;
}

/**
 * @function   venc_setPrm
 * @brief    配置编码参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_setPrm(void *pHandle, SAL_VideoFrameParam *pstInPrm)
{
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    SAL_VideoFrameParam *pstVencChnInfo = NULL;

    HD_VIDEOENC_IN  encode_in_param;
    HD_VIDEOENC_OUT encode_out_param;
    HD_H26XENC_RATE_CONTROL h26x_rc_param;
    HD_VIDEO_CODEC enEncType;
    HD_RESULT ret = HD_OK;

    UINT32 u32ActualWid = 0;
    UINT32 u32ActualHei = 0;
    UINT32 u32EncFps = 0;
    UINT32 u32ViFps = 0;
    UINT32 u32JpgQuality = 0;

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

    ret = hd_videoenc_get(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_IN, &encode_in_param);
    if (ret != HD_OK) {
        printf("set HD_VIDEOENC_PARAM_IN fail\n");
        return SAL_FAIL;
    }
    venc_checkResPrm(pstVencChnInfo->width,
                     pstVencChnInfo->height,
                     &u32ActualWid,
                     &u32ActualHei);

    memset(&encode_in_param, 0, sizeof(encode_in_param));
    encode_in_param.dim.w = u32ActualWid;
    encode_in_param.dim.h = u32ActualHei;
    encode_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
    encode_in_param.dir = HD_VIDEO_DIR_NONE;
    /* fixme 配置PARAM_IN时需要stop -> set -> start */
    ret = hd_videoenc_set(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_IN, &encode_in_param);
    if (ret != HD_OK) {
        printf("set HD_VIDEOENC_PARAM_IN fail\n");
        return SAL_FAIL;
    }

    u32ViFps = (pstVencChnInfo->fps >> 16); /*高16位源fps，低16位编码fps*/
    u32EncFps = (pstVencChnInfo->fps & 0xffff);
    VENC_LOGD("src rate %d dst rate %d\n", u32ViFps, u32EncFps);
    pstVencHalChnPrm->u32frmW = u32ActualWid;
    pstVencHalChnPrm->u32frmH = u32ActualHei;
    pstVencHalChnPrm->vencFps =  u32EncFps > 0 ? u32EncFps : 30;

    if (pstVencChnInfo->quality < 1)
    {
        u32JpgQuality = 1;
    }
    else if (pstVencChnInfo->quality > 99)
    {
        u32JpgQuality = 99;
    }
    else
    {
        u32JpgQuality = pstVencChnInfo->quality;
    }

    ret = hd_videoenc_get(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &encode_out_param);
     if (ret != HD_OK) {
         printf("get HD_VIDEOENC_PARAM_OUT_ENC_PARAM fail\n");
         return SAL_FAIL;
     }
    enEncType = venc_getEncType(pstVencChnInfo->encodeType);
     /* set new videoenc output parameters */
     switch (enEncType) {
     case HD_CODEC_TYPE_H264:
         encode_out_param.codec_type = HD_CODEC_TYPE_H264;
         encode_out_param.h26x.gop_num = pstVencHalChnPrm->vencFps * 2;
         break;
    
     case HD_CODEC_TYPE_H265:
         encode_out_param.codec_type = HD_CODEC_TYPE_H265;
         encode_out_param.h26x.gop_num = pstVencHalChnPrm->vencFps * 2;
         encode_out_param.h26x.svc_layer = HD_SVC_DISABLE;
         encode_out_param.h26x.profile = HD_H265E_MAIN_PROFILE;
         encode_out_param.h26x.level_idc = HD_H265E_LEVEL_4_1;
         encode_out_param.h26x.entropy_mode = HD_H265E_CABAC_CODING;
         break;
    
     case HD_CODEC_TYPE_JPEG:
         encode_out_param.codec_type = HD_CODEC_TYPE_JPEG;
         encode_out_param.jpeg.image_quality = u32JpgQuality;
         encode_out_param.jpeg.retstart_interval = 0;
         break;

     default:
        VENC_LOGE("enc type %d not support \n", enEncType);        
        return SAL_FAIL;
     }

     /* fixme 配置OUT_ENC_PARAM时需要stop -> set -> start */
     ret = hd_videoenc_set(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &encode_out_param);
     if (ret != HD_OK) {
         printf("set HD_VIDEOENC_PARAM_OUT_ENC_PARAM fail\n");
         return SAL_FAIL;
     }


    /* 配置帧率 */
   if (HD_CODEC_TYPE_JPEG == encode_out_param.codec_type)
    {
#if JPEG_RATE_CONTROL
        /* get the current jpeg rate control parameters of output stream for videoenc */
        ret = vendor_videoenc_get(pstVencHalChnPrm->u32PathId, VENDOR_JPEGENC_PARAM_RATE_CONTROL, &jpeg_rc_param);
        if (ret != HD_OK) {
            printf("vendor_videoenc_get(VENDOR_JPEGENC_PARAM_RATE_CONTROL) fail\n");
            return SAL_FAIL;
        }

        /* set the new jpeg rate control parameters of output stream for videoenc */
        jpeg_rc_param.vbr_mode = VENDOR_JPEG_RC_MODE_CBR;
        jpeg_rc_param.frame_rate_base = 30;   /* fps = frame_rate_base/frame_rate_incr */
        jpeg_rc_param.frame_rate_incr = 1;
        jpeg_rc_param.base_qp = 40;
        jpeg_rc_param.min_quality = 1;
        jpeg_rc_param.max_quality = 70;
        jpeg_rc_param.bitrate = 32 * 1024 * 1024;
        ret = vendor_videoenc_set(pstVencHalChnPrm->u32PathId, VENDOR_JPEGENC_PARAM_RATE_CONTROL, &jpeg_rc_param);
        if (ret != HD_OK) {
            printf("vendor_videoenc_set(VENDOR_JPEGENC_PARAM_RATE_CONTROL) fail\n");
            return SAL_FAIL;
        }
#endif
    } 
    else
    {
        /* get the current h26x rate control parameters of output stream for videoenc */
        ret = hd_videoenc_get(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &h26x_rc_param);
        if (ret != HD_OK) {
            printf("hd_videoenc_get(HD_VIDEOENC_PARAM_OUT_RATE_CONTROL) fail\n");
            return SAL_FAIL;
        }


        /* set the new h26x rate control parameters of output stream for videoenc */
        if (0 == pstVencChnInfo->bpsType)
        {
            h26x_rc_param.rc_mode = HD_RC_MODE_VBR;
            h26x_rc_param.vbr.frame_rate_base =  pstVencHalChnPrm->vencFps;   /* fps = frame_rate_base/frame_rate_incr */
            h26x_rc_param.vbr.frame_rate_incr = 1;
            h26x_rc_param.vbr.bitrate = pstVencChnInfo->bps * 1024;
            h26x_rc_param.vbr.static_time = 10;
        }
        else
        {
            h26x_rc_param.rc_mode = HD_RC_MODE_CBR;
            h26x_rc_param.cbr.frame_rate_base = pstVencHalChnPrm->vencFps;   /* fps = frame_rate_base/frame_rate_incr */
            h26x_rc_param.cbr.frame_rate_incr = 1;
            h26x_rc_param.cbr.bitrate = pstVencChnInfo->bps * 1024;
            h26x_rc_param.cbr.static_time = 2;            
        }
        /* fixme 配置PARAM_OUT_RATE_CONTROL时需要set -> start */
        ret = hd_videoenc_set(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &h26x_rc_param);
        if (ret != HD_OK) {
            printf("hd_videoenc_set(HD_VIDEOENC_PARAM_OUT_RATE_CONTROL) fail\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   venc_nt9833x_sendFrame
 * @brief    向 venc 通道发送 数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  VOID *pStr 视频帧信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 venc_sendFrame(VOID *pHandle, VOID *pStr)
{
    HD_VIDEO_FRAME *pstFrame = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    HD_RESULT ret = HD_OK;
    
    if ((NULL == pStr) || (NULL == pHandle))
    {
        VENC_LOGE("Input ptr is null!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstFrame = (HD_VIDEO_FRAME *)pStr;

    memset(&(pstVencHalChnPrm->stPushInBs), 0, sizeof(pstVencHalChnPrm->stPushInBs));
    pstVencHalChnPrm->stPushInBs.sign = MAKEFOURCC('V', 'S', 'T', 'M');
    pstVencHalChnPrm->stPushInBs.video_pack[0].phy_addr = pstVencHalChnPrm->u64StreamBufPa;
    pstVencHalChnPrm->stPushInBs.video_pack[0].size = pstVencHalChnPrm->u32StreamBufSize;
    pstVencHalChnPrm->stPushInBs.ddr_id = (pstVencHalChnPrm->u64StreamBufPa < DDR1_ADDR_START)? DDR_ID0 : DDR_ID1;

    
    ret = hd_videoenc_push_in_buf(pstVencHalChnPrm->u32PathId, pstFrame, &(pstVencHalChnPrm->stPushInBs), 1000);
    if (ret != HD_OK) {
        printf("hd_videoenc_push_in_buf fail, ret(%d)\n", ret);
        return SAL_FAIL;
    }


    return SAL_SOK;
}

/**
 * @function   venc_nt9833x_sendFrm
 * @brief    向 venc 通道发送 数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  void *pStr 视频帧信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_nt9833x_sendFrm(void *pHandle, void *pStr)
{
    void *pstFrame = NULL;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;
    SYSTEM_FRAME_INFO *pstSrc = NULL;

    if ((NULL == pStr) || (NULL == pHandle))
    {
        VENC_LOGE("Input ptr is null!\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pstSrc = (SYSTEM_FRAME_INFO *)pStr;

    pstFrame = (void *)pstSrc->uiAppData;
    VENC_HAL_CHECK_PRM(pstFrame, SAL_FAIL);
    pstVencHalChnPrm->stVencInfoBind.s32SendMode = NT_VENC_FRAME_PUT;    /*送帧方式*/

    return venc_sendFrame(pstVencHalChnPrm, pstFrame);
}

/**
 * @function   venc_nt9833x_setStatus
 * @brief    设置编码状态，方便drv层管理编码、取流流程
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  VOID *pStr 状态信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_nt9833x_setStatus(void *pHandle, VOID *pStr)
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
INT32 venc_create(void **ppHandle, SAL_VideoFrameParam *pstInPrm)
{

    HD_VIDEOENC_PATH_CONFIG encode_config = {0};
    HD_VIDEOENC_IN  encode_in_param;
    HD_VIDEOENC_OUT encode_out_param;
    HD_H26XENC_RATE_CONTROL h26x_rc_param;
//    VENDOR_JPEGENC_RATE_CONTROL jpeg_rc_param;
    HD_RESULT ret = HD_OK;

    UINT32 u32Width = 0;
    UINT32 u32Height = 0;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    SAL_VideoFrameParam *pstHalPrm = NULL;

    UINT32 u32Chan = 0;
    UINT32 u32EncFps = 0;
    UINT32 u32ViFps = 0;
    HD_VIDEO_CODEC enEncType = HD_CODEC_TYPE_H264; /*def prm*/
    INT32 s32Ret;
    HD_COMMON_MEM_DDR_ID enc_out_ddrid;
    ALLOC_VB_INFO_S stVbInfo;

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
    
    pstVencHalChnPrm = &gstVencInfo.stVencHalChnPrm[u32Chan];
    pstVencHalChnPrm->uiChn = u32Chan;

    if ((ret = hd_videoenc_open(HD_VIDEOENC_IN(0, u32Chan), HD_VIDEOENC_OUT(0, u32Chan), &pstVencHalChnPrm->u32PathId)) != HD_OK)
    {
        goto err0;
    }
        
    /* set videoenc path and pool config */
    ret = vendor_common_get_ddrid(HD_COMMON_MEM_ENC_OUT_POOL, 0, &enc_out_ddrid);
    if ( ret != HD_OK ) {
        printf("vendor_common_get_ddrid pool_type(%d) fail,please dts config\n", HD_COMMON_MEM_ENC_OUT_POOL);
        goto err0;
    }

    encode_config.data_pool[0].mode = HD_VIDEOENC_POOL_ENABLE;
    encode_config.data_pool[0].ddr_id = enc_out_ddrid;
    encode_config.data_pool[0].counts = HD_VIDEOENC_SET_COUNT(4, 0);
    encode_config.data_pool[1].mode = HD_VIDEOENC_POOL_DISABLE;
    encode_config.data_pool[2].mode = HD_VIDEOENC_POOL_DISABLE;
    encode_config.data_pool[3].mode = HD_VIDEOENC_POOL_DISABLE;

    ret = hd_videoenc_set(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_PATH_CONFIG, &encode_config);
    if (ret != HD_OK) {
        printf("hd_videoenc_set for main stream fail\n");
        goto err1;
    }

    u32Width = pstHalPrm->width;
    u32Height = pstHalPrm->height;
    enEncType = venc_getEncType(pstHalPrm->encodeType);
    u32ViFps = (pstHalPrm->fps >> 16); /*高16位源fps，低16位编码fps*/
    u32EncFps = (pstHalPrm->fps & 0xffff);
    VENC_LOGI("src rate %d dst rate %d\n", u32ViFps, u32EncFps);    

    pstVencHalChnPrm->u32frmW = u32Width;
    pstVencHalChnPrm->u32frmH = u32Height;
    pstVencHalChnPrm->vencFps =  u32EncFps > 0 ? u32EncFps : 30;

    ret = hd_videoenc_get(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_IN, &encode_in_param);
    if (ret != HD_OK) {
        printf("set HD_VIDEOENC_PARAM_IN fail\n");
        goto err1;
    }
    
    /* set videoenc input parameters */
    encode_in_param.dim.w = u32Width;
    encode_in_param.dim.h = u32Height;
    encode_in_param.pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
    encode_in_param.dir = HD_VIDEO_DIR_NONE;
    ret = hd_videoenc_set(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_IN, &encode_in_param);
    if (ret != HD_OK) {
        printf("set HD_VIDEOENC_PARAM_IN fail\n");
        goto err1;
    } 

    /* get current videoenc output parameters */
    ret = hd_videoenc_get(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &encode_out_param);
    if (ret != HD_OK) {
        printf("get HD_VIDEOENC_PARAM_OUT_ENC_PARAM fail\n");
        goto err1;
    }

    /* set new videoenc output parameters */
    switch (enEncType)
    {
        case HD_CODEC_TYPE_H264:
            encode_out_param.codec_type = HD_CODEC_TYPE_H264;
            encode_out_param.h26x.gop_num = pstVencHalChnPrm->vencFps * 2;
            break;

        case HD_CODEC_TYPE_H265:
            encode_out_param.codec_type = HD_CODEC_TYPE_H265;
            encode_out_param.h26x.gop_num = pstVencHalChnPrm->vencFps * 2;
            encode_out_param.h26x.svc_layer = HD_SVC_DISABLE;
            encode_out_param.h26x.profile = HD_H265E_MAIN_PROFILE;
            encode_out_param.h26x.level_idc = HD_H265E_LEVEL_4_1;
            encode_out_param.h26x.entropy_mode = HD_H265E_CABAC_CODING;
            break;

        case HD_CODEC_TYPE_JPEG:
            encode_out_param.codec_type = HD_CODEC_TYPE_JPEG;
            encode_out_param.jpeg.image_quality = 90;
            encode_out_param.jpeg.retstart_interval = 0;
            break;

        default:
            VENC_LOGE("enc type %d not support \n", enEncType);        
            return SAL_FAIL;
    }

    ret = hd_videoenc_set(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &encode_out_param);
    if (ret != HD_OK) {
        printf("set HD_VIDEOENC_PARAM_OUT_ENC_PARAM fail\n");
        goto err1;
    }

    if (HD_CODEC_TYPE_JPEG == encode_out_param.codec_type)
    {
#if JPEG_RATE_CONTROL
        /* get the current jpeg rate control parameters of output stream for videoenc */
        ret = vendor_videoenc_get(pstVencHalChnPrm->u32PathId, VENDOR_JPEGENC_PARAM_RATE_CONTROL, &jpeg_rc_param);
        if (ret != HD_OK) {
            printf("vendor_videoenc_get(VENDOR_JPEGENC_PARAM_RATE_CONTROL) fail\n");
            goto err1;
        }

        /* set the new jpeg rate control parameters of output stream for videoenc */
        jpeg_rc_param.vbr_mode = VENDOR_JPEG_RC_MODE_CBR;
        jpeg_rc_param.frame_rate_base = 30;   /* fps = frame_rate_base/frame_rate_incr */
        jpeg_rc_param.frame_rate_incr = 1;
        jpeg_rc_param.base_qp = 40;
        jpeg_rc_param.min_quality = 1;
        jpeg_rc_param.max_quality = 70;
        jpeg_rc_param.bitrate = 32 * 1024 * 1024;
        ret = vendor_videoenc_set(pstVencHalChnPrm->u32PathId, VENDOR_JPEGENC_PARAM_RATE_CONTROL, &jpeg_rc_param);
        if (ret != HD_OK) {
            printf("vendor_videoenc_set(VENDOR_JPEGENC_PARAM_RATE_CONTROL) fail\n");
            goto err1;
        }
#endif
    } 
    else
    {
        /* get the current h26x rate control parameters of output stream for videoenc */
        ret = hd_videoenc_get(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &h26x_rc_param);
        if (ret != HD_OK) {
            printf("hd_videoenc_get(HD_VIDEOENC_PARAM_OUT_RATE_CONTROL) fail\n");
            goto err1;
        }


        /* set the new h26x rate control parameters of output stream for videoenc */
        if (0 == pstHalPrm->bpsType)
        {
            h26x_rc_param.rc_mode = HD_RC_MODE_VBR;
            h26x_rc_param.vbr.frame_rate_base = pstVencHalChnPrm->vencFps;   /* fps = frame_rate_base/frame_rate_incr */
            h26x_rc_param.vbr.frame_rate_incr = 1;
            h26x_rc_param.vbr.bitrate = pstHalPrm->bps * 1024;
            h26x_rc_param.vbr.static_time = 10;
        }
        else
        {
            h26x_rc_param.rc_mode = HD_RC_MODE_CBR;
            h26x_rc_param.cbr.frame_rate_base =  pstVencHalChnPrm->vencFps;   /* fps = frame_rate_base/frame_rate_incr */
            h26x_rc_param.cbr.frame_rate_incr = 1;
            h26x_rc_param.cbr.bitrate = pstHalPrm->bps * 1024;
            //h26x_rc_param.cbr.static_time = 2;            
        }
        ret = hd_videoenc_set(pstVencHalChnPrm->u32PathId, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &h26x_rc_param);
        if (ret != HD_OK) {
            printf("hd_videoenc_set(HD_VIDEOENC_PARAM_OUT_RATE_CONTROL) fail\n");
            goto err1;
        }
    }

    //pstVencHalChnPrm->uiFd = HI_MPI_VENC_GetFd(pstVencHalChnPrm->uiChn);

#ifdef VENC_HAL_SAVE_BITS_DATA

    char aszFileName[64];
    snprintf(aszFileName, 64, "./DSP_TEST_RES/Stream/ES/stream_chn%d.es.h264", u32Chan);

    file[uiChn] = fopen(aszFileName, "wb");

    if (HI_NULL == file[u32Chan])
    {
        VENC_LOGE("open file failed:%s!\n", strerror(errno));
    }

#endif

    *ppHandle = pstVencHalChnPrm;

    pstVencHalChnPrm->uibStart = SAL_FALSE;
    pstVencHalChnPrm->isCreate = SAL_TRUE;
    VENC_LOGI("VENC Chn %d HalChn %d Create success.\n", u32Chan, pstVencHalChnPrm->uiChn);

    pstVencHalChnPrm->u32StreamBufSize = 1920*1080*2;
    
    s32Ret = mem_hal_vbAlloc(pstVencHalChnPrm->u32StreamBufSize, "venc_nt9833x", "streamBuf", NULL, SAL_FALSE, &stVbInfo);
    if (SAL_FAIL == s32Ret)
    {
        VENC_LOGE("mem_hal_vbAlloc failed size %u !\n", pstVencHalChnPrm->u32StreamBufSize);
        goto err1;
    }
    pstVencHalChnPrm->pStreamBuf = (CHAR *)stVbInfo.pVirAddr;
    pstVencHalChnPrm->u64StreamBufPa = stVbInfo.u64PhysAddr;
    
    pstVencHalChnPrm->stVencVbInfo.u32PoolId = stVbInfo.u32PoolId;
    pstVencHalChnPrm->stVencVbInfo.u64VbBlk  = stVbInfo.u64VbBlk;
    pstVencHalChnPrm->stVencVbInfo.u32Size   = stVbInfo.u32Size;
        
    return SAL_SOK;

err1:
    hd_videoenc_close(pstVencHalChnPrm->u32PathId);
err0:
    venc_putGroup(u32Chan);

    return SAL_FAIL;
}

/**
 * @function   venc_nt9833x_delete
 * @brief    销毁编码器
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_nt9833x_delete(void *pHandle)
{
    INT32 s32Ret = SAL_SOK;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    if (NULL == pHandle)
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;

    s32Ret = venc_nt9833x_stop(pHandle);
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
 * @function   venc_nt9833x_getChan
 * @brief    获取plat层通道venc_hal_getVencHalChan
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] UINT32 *poutChan plat层通道号指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */

INT32 venc_nt9833x_getChan(void *pHandle, UINT32 *pOutChan)
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
 * @function   venc_getJpegFrame
 * @brief    Jpeg编码通道获取一帧数据
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out]  INT8 *pcJpeg 编码码流
 * @param[out]  UINT32 *pJpegSize 编码码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_getJpegFrame(void *pHandle, INT8 *pcJpeg, UINT32 *pJpegSize)
{
    HD_RESULT enRet = HD_OK;
    VENC_HAL_CHN_INFO_S *pstVencHalChnPrm = NULL;

    HD_VIDEOENC_BS *pPushInBs = NULL;
    UINT32 u32BsSize = 0;
    UINT32 u32PackNum, u32PaOffset;


    if ((NULL == pHandle) || (NULL == pcJpeg) || (NULL == pJpegSize))
    {
        VENC_LOGE("inv ptr\n");
        return SAL_FAIL;
    }

    pstVencHalChnPrm = (VENC_HAL_CHN_INFO_S *)pHandle;
    pPushInBs = &pstVencHalChnPrm->stPushInBs;


    enRet = hd_videoenc_pull_out_buf(pstVencHalChnPrm->u32PathId, pPushInBs, 2000);
    if (enRet == HD_ERR_TIMEDOUT)
    {
        printf("hd_videoenc_pull_out_buf timeout, retry...\n");
        return SAL_FAIL;
    }
    else if (enRet != HD_OK)
    {
        printf("hd_videoenc_pull_out_buf fail\n");
        return SAL_FAIL;
    }
    else
    {
        for (u32PackNum = 0; u32PackNum < pPushInBs->pack_num; u32PackNum++)
        {
            u32PaOffset = pPushInBs->video_pack[u32PackNum].phy_addr - pstVencHalChnPrm->u64StreamBufPa;
            memcpy(pcJpeg + u32BsSize, pstVencHalChnPrm->pStreamBuf + u32PaOffset, pPushInBs->video_pack[u32PackNum].size);
            u32BsSize += pPushInBs->video_pack[u32PackNum].size;
        }
        *pJpegSize = u32BsSize;
        VENC_LOGD(" jpeg Encode output size: %d\n", u32BsSize);
    }

    return SAL_SOK;
}

/**
 * @function   venc_nt9833x_encJpeg
 * @brief    获取plat层通道venc_hal_getVencHalChan
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 帧信息
 * @param[in]  CROP_S *pstCropInfo 编码crop信息
 * @param[in]  BOOL bSetPrm 编码参数更新，用于重配编码器
 * @param[out] INT8 *pJpeg 编码输出JPEG码流
 * @param[out] INT8 *pJpegSize 编码输出JPEG码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_nt9833x_encJpeg(void *pHandle, SYSTEM_FRAME_INFO *pstFrame, INT8 *pJpeg, UINT32 *pJpegSize, CROP_S *pstCropInfo, BOOL bSetPrm)
{
    VENC_HAL_CHECK_PRM(pstFrame, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pJpeg, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pJpegSize, SAL_FAIL);
    VENC_HAL_CHECK_PRM(pHandle, SAL_FAIL);

    HD_VIDEO_FRAME *pstFrameInfo = (HD_VIDEO_FRAME *)pstFrame->uiAppData;
    VENC_HAL_CHECK_PRM(pstFrameInfo, SAL_FAIL);

    SAL_VideoFrameParam stJpegPicInfo = {0};

    if ((SAL_TRUE == bSetPrm) || (NULL != pstCropInfo)) /*抠图模式必然要设置参数*/
    {
        stJpegPicInfo.encodeType = MJPEG;
        stJpegPicInfo.quality = 80; /* 80; */
        stJpegPicInfo.width = pstFrameInfo->dim.w;
        stJpegPicInfo.height = pstFrameInfo->dim.h;

        /* jpeg 裁剪 抓图 还未实现 */
        if (NULL != pstCropInfo)
        {
            VENC_LOGE("nt9833x jpeg crop has not been developed \n");
        }

        if (SAL_SOK != venc_setPrm(pHandle, &stJpegPicInfo))
        {
            VENC_LOGE("venc_setJpegPrm fail!\n");
            return SAL_FAIL;
        }

    }

    if (SAL_SOK != venc_nt9833x_start(pHandle))
    {
        VENC_LOGE("VencHal_drvStart error !\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_sendFrame(pHandle, pstFrameInfo))
    {
        VENC_LOGE("venc_nt9833x_sendFrame error !\n");
        venc_nt9833x_stop(pHandle);
        return SAL_FAIL;
    }

    usleep(50 * 1000);

    if (SAL_SOK != venc_getJpegFrame(pHandle, pJpeg, pJpegSize))
    {
        VENC_LOGE("venc_getJpegFrame error !\n");
        venc_nt9833x_stop(pHandle);
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_nt9833x_stop(pHandle))
    {
        VENC_LOGE("venc_nt9833x_stop error !\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   venc_nt9833x_setEncPrm
 * @brief    设置编码参数
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]   SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_nt9833x_setEncPrm(void *ppHandle, SAL_VideoFrameParam *pstInPrm)
{   
    if ((NULL == ppHandle) || (NULL == pstInPrm))
    {
        VENC_LOGE("inv hdl\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_nt9833x_stop(ppHandle))
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_setPrm(ppHandle, pstInPrm))
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != venc_nt9833x_start(ppHandle))
    {
        VENC_LOGE("!!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   venc_nt9833x_create
 * @brief    创建视频硬件编码器
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] void **ppHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_nt9833x_create(void **ppHandle, SAL_VideoFrameParam *pstInPrm)
{
    INT32 s32Ret = SAL_FAIL;

    if ((NULL == pstInPrm) || (ppHandle == NULL))
    {
        VENC_LOGE("Prm is NULL, Create Failed !!!\n");
        return SAL_FAIL;
    }

    s32Ret = venc_create(ppHandle, pstInPrm);
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
INT32 venc_nt9833x_init(void)
{
    HD_RESULT ret;
    
    /* 初始化通道管理锁 */
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &gstVencInfo.MutexHandle);

    if ((ret = hd_videoenc_init()) != HD_OK) {
        return SAL_FAIL;
    }
    
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
    pstVencPlatOps->Init = venc_nt9833x_init;
    pstVencPlatOps->Create = venc_nt9833x_create;
    pstVencPlatOps->Delete = venc_nt9833x_delete;
    pstVencPlatOps->Start = venc_nt9833x_start;
    pstVencPlatOps->Stop = venc_nt9833x_stop;
    pstVencPlatOps->SetParam = venc_nt9833x_setEncPrm;
    pstVencPlatOps->ForceIFrame = venc_nt9833x_requestIDR;
    pstVencPlatOps->GetFrame = venc_nt9833x_getFrame;
    pstVencPlatOps->PutFrame = venc_nt9833x_putFrame;
    pstVencPlatOps->GetHalChn = venc_nt9833x_getChan;
    pstVencPlatOps->SetVencStatues = venc_nt9833x_setStatus;
    pstVencPlatOps->EncJpegProcess = venc_nt9833x_encJpeg;
    pstVencPlatOps->EncSendFrm = venc_nt9833x_sendFrm;

    return SAL_SOK;
}

