/**
 * @file   audioMux_drv.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---音频封装接口
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取音频封装接口
 */ 

/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal.h"   
#include "audio_common.h"
#include "libmux.h"
#include "libdemux.h"
#include "PSDemuxLib.h"
#include "audioMux_drv_api.h"

/*****************************************************************************
                         结构定义
*****************************************************************************/


/*****************************************************************************
                        函数
*****************************************************************************/
/**
 * @function   audioMux_create
 * @brief      创建封装句柄
 * @param[in]  UINT32 u32PackTpye 封装类型
 * @param[out] UINT32 *pHandleId 封装句柄ID
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audioMux_create(UINT32 u32PackTpye, UINT32 *pHandleId)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32MuxType = 0;
    MUX_PARAM stMuxPrm = {0};

    if(NULL == pHandleId)
    {
        AUD_LOGE("ptr null!\n");
        return SAL_FAIL;
    }

    /**********动态创建一个新的打包通道***************/
    u32MuxType = u32PackTpye;
    memset(&stMuxPrm, 0, sizeof(stMuxPrm));

    s32Ret = MuxControl(MUX_GET_CHAN, &u32MuxType, &stMuxPrm);
    if (s32Ret != SAL_SOK)
    {
        AUD_LOGE("MuxControl idx %d err %x\n", MUX_GET_CHAN, s32Ret);
        return SAL_FAIL;
    }

    /**********创建打包模块***************/
    /* stMuxPrm.audPack = 1; */
    stMuxPrm.bufAddr = (PUINT8)SAL_memMalloc(stMuxPrm.bufLen, "audioMux_drv", "mux");
    if (stMuxPrm.bufAddr == NULL)
    {
        AUD_LOGE("stMuxPrm buffer malloc err!\n");
        return SAL_FAIL;
    }

    s32Ret = MuxControl(MUX_CREATE, &stMuxPrm, NULL);
    if (s32Ret != SAL_SOK)
    {
        AUD_LOGE("MuxControl idx %d err %x\n", MUX_CREATE, s32Ret);
        return SAL_FAIL;
    }

    /* 记录当前打包handle的通道号 */
    *pHandleId = stMuxPrm.muxHdl;

    return SAL_SOK;
}
/**
 * @function   audioMux_drv_init
 * @brief      音频封装组件初始化
 * @param[in]  AUDIO_PACK_INFO_S *pstAudPackInfo 封装参数指针
 * @param[out] AUDIO_PACK_INFO_S *pstAudPackInfo 封装句柄
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioMux_drv_init(AUDIO_PACK_INFO_S *pstAudPackInfo)
{
    INT32 s32Ret = SAL_FAIL;
    MUX_STREAM_INFO_S stStreamInfo = {0};

    if (NULL == pstAudPackInfo)
    {
        AUD_LOGE("!!!Venc packcreate input error \n");
        return SAL_FAIL;
    }

    memset(&stStreamInfo, 0, sizeof(stStreamInfo));
    stStreamInfo.muxType = MUX_AUD;

    s32Ret = MuxControl(MUX_GET_STREAMSIZE, &stStreamInfo, &pstAudPackInfo->u32PackBufSize);
    if (s32Ret != SAL_SOK)
    {
        AUD_LOGE("MuxControl idx %d err %x\n", MUX_GET_STREAMSIZE, s32Ret);
        return SAL_FAIL;
    }

    pstAudPackInfo->pPackOutputBuf = (UINT8 *)SAL_memMalloc(pstAudPackInfo->u32PackBufSize, "audioMux_drv", "mux"); /* 打包后的输出缓存 */
    if (NULL == pstAudPackInfo->pPackOutputBuf)
    {
        AUD_LOGE("packOutputBuf malloc err \n");
        return SAL_FAIL;
    }

    AUD_LOGI("Create Ps Handle!!!\n");
    pstAudPackInfo->isUsePs = SAL_TRUE;
    if (SAL_SOK != audioMux_create(MUX_PS, &pstAudPackInfo->u32PsHandle)) /* PsHandle 具体指的是通道号。 */
    {
        AUD_LOGE("Ps Pack Create Failed !!!\n");
        return SAL_FAIL;
    }

    AUD_LOGI("Create RTP Handle!!!\n");
    pstAudPackInfo->isUseRtp = SAL_TRUE;
    if (SAL_SOK != audioMux_create(MUX_RTP, &pstAudPackInfo->u32RtpHandle)) /* RtpHandle 具体指的是通道号。 */
    {
        AUD_LOGE("RTP Pack Create Failed !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}
/**
 * @function   audioMux_drv_rtpProcess
 * @brief      音频RTP封装
 * @param[in]  AUDIO_PACK_INFO_S *pstAudPackInfo 封装参数指针
 * @param[in]  AUD_BITS_INFO_S *pstBitDataInfo 输入码流
 * @param[out] AUDIO_PACK_INFO_S *pstAudPackInfo 封装后的码流
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioMux_drv_rtpProcess(AUDIO_PACK_INFO_S *pstAudPackInfo, AUD_BITS_INFO_S *pstBitDataInfo)
{
    INT32 s32Ret = SAL_FAIL;

    MUX_PROC_INFO_S stMuxProcInfo = {0};
    MUX_IN_BITS_INTO_S *pstMuxInInfo = NULL;
    MUX_OUT_BITS_INTO_S *pstMuxOutInfo = NULL;


    if( (NULL == pstAudPackInfo) || (NULL == pstBitDataInfo))
    {
        AUD_LOGE("!!!\n");
        return SAL_FAIL;
    }

    /* rtp pack */
    if (0xFF == pstAudPackInfo->u32RtpHandle)
    {
        AUD_LOGE("======= Rtp handle err ===========\n");
        return SAL_FAIL;
    }

    memset(&stMuxProcInfo, 0, sizeof(MUX_PROC_INFO_S));
    pstMuxInInfo = &stMuxProcInfo.muxData.stInBuffer;
    pstMuxOutInfo = &stMuxProcInfo.muxData.stOutBuffer;

    stMuxProcInfo.muxHdl = pstAudPackInfo->u32RtpHandle;        /* rtp打包创建的通道 */
    pstMuxInInfo->bVideo = 0;         /* 音频打包 */
    pstMuxInInfo->naluNum = 1;
    pstMuxInInfo->audExInfo = 1;
    pstMuxInInfo->bufferAddr[0] = pstBitDataInfo->pBitsDataAddr;
    pstMuxInInfo->bufferLen[0] = pstBitDataInfo->u32BitsDataLen;
    pstMuxInInfo->u64PTS = pstBitDataInfo->u64TimeStamp;

    pstMuxInInfo->frame_type = FRAME_TYPE_AUDIO_FRAME;
    if (pstBitDataInfo->enAudioEncType == G711_MU)
    {
        pstMuxInInfo->audEnctype = AUDIO_G711_U;

    }
    else if (pstBitDataInfo->enAudioEncType == G711_A)
    {
        pstMuxInInfo->audEnctype = AUDIO_G711_A;
    }
    else
    {
        pstMuxInInfo->audEnctype = AUDIO_AAC;
    }

    pstMuxOutInfo->bufferAddr = pstAudPackInfo->pPackOutputBuf;
    pstMuxOutInfo->bufferLen = pstAudPackInfo->u32PackBufSize;
    pstMuxOutInfo->streamLen = 0;

    if (SAL_SOK != MuxControl(MUX_PRCESS, &stMuxProcInfo, NULL))
    {
        AUD_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);
        return SAL_FAIL;
    }
    pstAudPackInfo->u32PackSize = pstMuxOutInfo->streamLen;

    return SAL_SOK;
}
/**
 * @function   audioMux_drv_psProcess
 * @brief      音频PS封装
 * @param[in]  AUDIO_PACK_INFO_S *pstAudPackInfo 封装参数指针
 * @param[in]  AUD_BITS_INFO_S *pstBitDataInfo 输入码流
 * @param[out] AUDIO_PACK_INFO_S *pstAudPackInfo 封装后的码流
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioMux_drv_psProcess(AUDIO_PACK_INFO_S *pstAudPackInfo, AUD_BITS_INFO_S *pstBitDataInfo)
{
    INT32 s32Ret = SAL_FAIL;

    MUX_PROC_INFO_S stMxProcInfo = {0};
    MUX_IN_BITS_INTO_S *pstMuxInInfo = NULL;
    MUX_OUT_BITS_INTO_S *pstMuxOutInfo = NULL;

    if( (NULL == pstAudPackInfo) || (NULL == pstBitDataInfo))
     {
         AUD_LOGE("inv prm!!!\n");
         return SAL_FAIL;
     }

    /* ps pack */
    if (0xFF == pstAudPackInfo->u32PsHandle)
    {
        AUD_LOGE("===== ps Handle err ======\n");
        return SAL_FAIL;
    }

    memset(&stMxProcInfo, 0, sizeof(MUX_PROC_INFO_S));
    pstMuxInInfo = &stMxProcInfo.muxData.stInBuffer;
    pstMuxOutInfo = &stMxProcInfo.muxData.stOutBuffer;

    stMxProcInfo.muxHdl = pstAudPackInfo->u32PsHandle;
    pstMuxInInfo->bVideo = 0;
    pstMuxInInfo->naluNum = 1;
    pstMuxInInfo->bufferAddr[0] = pstBitDataInfo->pBitsDataAddr;
    pstMuxInInfo->bufferLen[0] = pstBitDataInfo->u32BitsDataLen;
    pstMuxInInfo->u64PTS = pstBitDataInfo->u64TimeStamp;

    pstMuxInInfo->frame_type = FRAME_TYPE_AUDIO_FRAME;
    if (pstBitDataInfo->enAudioEncType == G711_MU)
    {
        pstMuxInInfo->audEnctype = AUDIO_G711_U;

    }
    else if (pstBitDataInfo->enAudioEncType == G711_A)
    {
        pstMuxInInfo->audEnctype = AUDIO_G711_A;
    }
    else
    {
        pstMuxInInfo->audEnctype = AUDIO_AAC;
    }

    pstMuxOutInfo->bufferAddr = pstAudPackInfo->pPackOutputBuf;
    pstMuxOutInfo->bufferLen = pstAudPackInfo->u32PackBufSize;
    pstMuxOutInfo->streamLen = 0;

    if (SAL_SOK != MuxControl(MUX_PRCESS, &stMxProcInfo, NULL))
    {
        AUD_LOGE("MuxControl idx %d err %x\n", MUX_PRCESS, s32Ret);
        return SAL_FAIL;
    }

    pstAudPackInfo->u32PackSize = pstMuxOutInfo->streamLen;

    return SAL_SOK;
}

