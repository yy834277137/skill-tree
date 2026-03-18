/**
 * @file   system_prm.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  DSP 模块 系统参数管理模块
 * @author dsp
 * @date   2022/5/7
 * @note
 * @note \n History
   1.日    期: 2022/5/7
     作    者: dsp
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "system_prm_api.h"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
/* DSP交互全局结构体 */
static DSPINITPARA g_DspInitPara;

/* 全局结构体指针，统一使用指针操作全局结构体，方便单进程、多进程模式兼容 */
static DSPINITPARA *g_pDspInitPara = NULL;

/* 回调函数指针*/
static DATACALLBACK pDataCallBackFunc = NULL;

/* 系统参数*/
static SYS_PRM_CTRL gstSysPrmCtrl;

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/**
 * @function   InitParam
 * @brief      初始化共享结构体
 * @param[in]  DSPINITPARA *pstDspInitPara
 * @param[out] None
 * @return     static INT32 HIK_SOK  : 成功
 *                          HIK_FAIL : 失败
 */
static INT32 InitParam(DSPINITPARA *pstDspInitPara)
{
    /* int i = 0; */

    pstDspInitPara->viType = 0;      /* 定义视频模式 ，不同的设备此值不相同 */
    pstDspInitPara->aiType = 0;      /* 定义音频模式 ，不同的设备此值不相同 */

    pstDspInitPara->machineType = 0; /* 定义设备类型，一般不使用 ，赋值 0 */
    pstDspInitPara->boardType = ISD_SC5030S_1CVS1;   /* 设备类型   */
    pstDspInitPara->sn = 0;          /* 设备序列号 */

    /***********************************************************************/
    /*                    DSP模块通道数定义信息由应用程序同事设置          */
    /***********************************************************************/
    pstDspInitPara->rtpPackLen = 1400; /* 最大RTP打包包长*/

    /***********************************************************************/
    /*                  应用下发信息，用于实现对应功能                     */
    /***********************************************************************/
    /* 采集无视频信号时，所使用的无视频信号图像信息 */

    /* 最多支持两张图片，用于采集无信号的时候显示用，根据图片的填入真实的宽高大小，
       以及按大小来分配内存，分配的内存用于保存图片的数据，具体的图片格式此处没有
       体现，需要用dsp协商 无此功能，可先不用填
     */

    /* OSD功能所使用字库的信息 ，读入字库的信息，传给DSP ，目前的大小为 4096与267616 */

    /* 码流编码与封包信息 ，按实现需要填写 */
    pstDspInitPara->stStreamInfo.stVideoParam.videoEncType = S264;
    pstDspInitPara->stStreamInfo.stVideoParam.videoSubEncType = S264;
    pstDspInitPara->stStreamInfo.stVideoParam.packetType = RTP_STREAM | PS_STREAM;
    pstDspInitPara->stStreamInfo.stVideoParam.subPacketType = RTP_STREAM | PS_STREAM;
    pstDspInitPara->stStreamInfo.stVideoParam.ThirdPacketType = PS_STREAM;
    pstDspInitPara->stStreamInfo.stVideoParam.bWaterMark = 0;

    pstDspInitPara->stStreamInfo.stAudioParam.encType = AAC;               /* 音频压缩算法 */
    pstDspInitPara->stStreamInfo.stAudioParam.samplesPerSecond = 8000;         /* 采样率 */
    pstDspInitPara->stStreamInfo.stAudioParam.samplesPerFrame = 0;             /* 每帧样点数 */
    pstDspInitPara->stStreamInfo.stAudioParam.boardSamplesPerSecond = 0;       /* 板子原始音频采样率 */
    pstDspInitPara->stStreamInfo.stAudioParam.encChans = 0;                    /* 算法通道数，通常为单声道 */
    pstDspInitPara->stStreamInfo.stAudioParam.bitRate = 0;                     /* 输出码率 */

    pstDspInitPara->stStreamInfo.stTalkBackParam.talkBackType = AAC;             /* 语音对讲压缩算法 */
    pstDspInitPara->stStreamInfo.stTalkBackParam.talkBackSamplesPerSecond = 16000;    /* 语音对讲采样率 */
    pstDspInitPara->stStreamInfo.stTalkBackParam.talkBackSamplesPerFrame = 0;        /* 语音对讲每帧样点数 */
    pstDspInitPara->stStreamInfo.stTalkBackParam.boardOutSamplesPerSecond = 0;       /* 板子原始输出音频采样率 */
    pstDspInitPara->stStreamInfo.stTalkBackParam.talkBackBitRate = 0;                /* 语音对讲输出码率 */
    pstDspInitPara->stStreamInfo.stTalkBackParam.talkBackChans = 0;                  /* 语音对讲算法通道数，通常为单声道 */
    pstDspInitPara->stStreamInfo.stTalkBackParam.boardOutChans = 0;                  /* 板子输出通道数，davinci为立体声 */
    pstDspInitPara->stStreamInfo.stTalkBackParam.audOutVolume = 0;

    /* 人脸相关参数阈值 按实际值填写 ，没有该功能，则此处不用分配内存，直接置为NULL即可 */
    pstDspInitPara->face_threshold.stat = 1;             /* 1:登记 2:比对    */
    pstDspInitPara->face_threshold.movementVal_rec = 0;  /* 登记移动侦测阈值 */
    pstDspInitPara->face_threshold.movementVal_cmp = 0;  /* 比对移动侦测阈值 */
    pstDspInitPara->face_threshold.movementCtrl = 0;     /* 移动侦测开关控制 */
    pstDspInitPara->face_threshold.frontal_conf = 0;     /* 正面评分阈值     */
    pstDspInitPara->face_threshold.clearity = 0;         /* 清晰度           */
    pstDspInitPara->face_threshold.pitch = 0;            /* ptich阈值        */
    pstDspInitPara->face_threshold.yaw = 0;              /* yaw阈值          */
    pstDspInitPara->face_threshold.roll = 0;             /* roll阈值         */

    /* 水印功能信息 按实际值填写 ，没有该功能，则此处不用分配内存，直接置为NULL即可 */
    memcpy(&pstDspInitPara->stWaterMaskInfo.macAddr[0], "abcde", 6); /* MAC地址  */
    pstDspInitPara->stWaterMaskInfo.device_type = 0;                  /* 型号     */
    pstDspInitPara->stWaterMaskInfo.device_info = 0;                 /* 附加信息 */

    return SAL_SOK;
}

/**
 * @function   SystemPrm_getNoSignalInfo
 * @brief      获取无视频信号信息
 * @param[in]  CAPT_NOSIGNAL_INFO_ST *pstNoSignalInfo
 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 SystemPrm_getNoSignalInfo(CAPT_NOSIGNAL_INFO_ST *pstNoSignalInfo)
{
    if (NULL == pstNoSignalInfo)
    {
        return SAL_FAIL;
    }

    memcpy(pstNoSignalInfo, &g_pDspInitPara->stCaptNoSignalInfo, sizeof(CAPT_NOSIGNAL_INFO_ST));

    return SAL_SOK;
}

/**
 * @function   SystemPrm_getSysVideoFormat
 * @brief      获取系统视频制式
 * @param[in]  void
 * @param[out] None
 * @return     INT32 SAL_SOK  表示P制
                     SAL_FAIL 表示N制
 */
INT32 SystemPrm_getSysVideoFormat(void)
{
    UINT32 uiPalFlag = 0;
    VI_INIT_INFO_ST *pstViInitInfo = &g_pDspInitPara->stViInitInfoSt;
    INT32 i = 0;

    for (i = 0; i < pstViInitInfo->uiViChn; i++)
    {
        if (VS_STD_PAL != pstViInitInfo->stViInitPrm[i].eViStandard)
        {
            uiPalFlag = 1;
        }
    }

    if (0 != uiPalFlag)
    {
        /* 不是 PAL 制 返回错误 */
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   SystemPrm_cbFunProc
 * @brief      回调处理
 * @param[in]  STREAM_ELEMENT *pEle  信息头
 * @param[in]  unsigned char *buf    缓冲
 * @param[in]  unsigned int bufLen   缓冲长度
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_cbFunProc(STREAM_ELEMENT *pEle, unsigned char *buf, unsigned int bufLen)
{
    pEle->magic = STREAM_ELEMENT_MAGIC;
    pEle->dataLen = bufLen;

    pDataCallBackFunc(pEle, buf, bufLen);

    return SAL_SOK;
}

/**
 * @function   SystemPrm_readTalkBackPool
 * @brief      读取语音对讲数据
 * @param[in]  UINT32 uiChn     通道
 * @param[in]  PUINT8 pucAddr   地址
 * @param[in]  UINT32 *puiSize  大小
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_readTalkBackPool(UINT32 uiChn, PUINT8 pucAddr, UINT32 *puiSize)
{
    AUDIO_TB_BUF_INFO_ST *pAudioTalkBack = &g_pDspInitPara->stAudioTBBufInfo;
    INT32 dist = 0;

    /* 读写指针距离超过 32 才更新 */
    dist = DIST(pAudioTalkBack->hostWriteIdx, pAudioTalkBack->dspReadIdx, TALKBACK_FRM_CNT);
    pAudioTalkBack->dspReadIdx = ((dist > 32) ? pAudioTalkBack->hostWriteIdx : pAudioTalkBack->dspReadIdx);

    dist = DIST(pAudioTalkBack->hostWriteIdx, pAudioTalkBack->dspReadIdx, TALKBACK_FRM_CNT);
    if (dist < 2)
    {
        return SAL_FAIL;
    }

    memcpy(pucAddr, &pAudioTalkBack->audTalkbackHost[pAudioTalkBack->dspReadIdx * TALKBACK_FRM_LEN], TALKBACK_FRM_LEN);
    GO_FORWARD(pAudioTalkBack->dspReadIdx, TALKBACK_FRM_CNT);
    memcpy(pucAddr + TALKBACK_FRM_LEN, &pAudioTalkBack->audTalkbackHost[pAudioTalkBack->dspReadIdx * TALKBACK_FRM_LEN], TALKBACK_FRM_LEN);
    GO_FORWARD(pAudioTalkBack->dspReadIdx, TALKBACK_FRM_CNT);

    *puiSize = TALKBACK_FRM_LEN * 2;

    return SAL_SOK;
}

/**
 * @function   SystemPrm_writeTalkBackPool
 * @brief      写入语音对讲数据
 * @param[in]  UINT32 uiChn    通道
 * @param[in]  PUINT8 pucAddr  地址
 * @param[in]  UINT32 uiSize   大小
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeTalkBackPool(UINT32 uiChn, PUINT8 pucAddr, UINT32 uiSize)
{
    UINT32 u32Offset = 0;
    UINT32 u32len = uiSize;
    AUDIO_TB_BUF_INFO_ST *pAudioTalkBack = &g_pDspInitPara->stAudioTBBufInfo;


    while(u32len)
    {
        memcpy(&pAudioTalkBack->audTalkbackDsp[pAudioTalkBack->dspWriteIdx * TALKBACK_FRM_LEN],
                pucAddr + TALKBACK_FRM_LEN * u32Offset ,
                u32len >= TALKBACK_FRM_LEN ? TALKBACK_FRM_LEN : u32len);
        GO_FORWARD(pAudioTalkBack->dspWriteIdx, TALKBACK_FRM_CNT);

        u32len = u32len >= TALKBACK_FRM_LEN ? u32len - TALKBACK_FRM_LEN : 0;
        u32Offset++;
    }

    return SAL_SOK;
}

/**
 * @function   SystemPrm_readDecSharedPool
 * @brief      读取待解码数据
 * @param[in]  UINT32 uiChn     通道
 * @param[in]  UINT8 **pucAddr  地址
 * @param[in]  UINT32 *puiSize  大小
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_readDecSharedPool(UINT32 uiChn, UINT8 **pucAddr, UINT32 *puiSize)
{
    if (NULL == puiSize)
    {
        return SAL_SOK;
    }

    UINT32 u32DatLen = 0;
    DEC_SHARE_BUF *pstDecShare = &g_pDspInitPara->decShareBuf[uiChn];

    u32DatLen = pstDecShare->writeIdx >= pstDecShare->readIdx
                ? pstDecShare->writeIdx - pstDecShare->readIdx
                : pstDecShare->bufLen + pstDecShare->writeIdx - pstDecShare->readIdx;
    if (u32DatLen >= 336)
    {
        *pucAddr = pstDecShare->pVirtAddr + pstDecShare->readIdx;
        *puiSize = u32DatLen;
        pstDecShare->readIdx = (pstDecShare->readIdx + u32DatLen) % pstDecShare->bufLen;
        return SAL_SOK;
    }

    *puiSize = 0;
    return SAL_FAIL;
}

/**
 * @function   SystemPrm_getDecShareBuf
 * @brief      获取解码缓冲控制结构体
 * @param[in]  UINT32 uiChn               通道
 * @param[in]  DEC_SHARE_BUF **pDecShare  返回的解码缓冲控制信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_getDecShareBuf(UINT32 uiChn, DEC_SHARE_BUF **pDecShare)
{
    *pDecShare = &g_pDspInitPara->decShareBuf[uiChn];
    return SAL_SOK;
}

/**
 * @function   SystemPrm_getStreamType
 * @brief      获取码流类型
 * @param[in]  UINT32 uiChn  通道
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_getStreamType(UINT32 uiChn)
{
    return MPEG2MUX_STREAM_TYPE_RTP;
}

/**
 * @function   SystemPrm_writeDecSharedPool
 * @brief      写解码缓冲
 * @param[in]  UINT32 uiChn    通道
 * @param[in]  UINT8 *pucAddr  地址
 * @param[in]  UINT32 uiSize   大小
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeDecSharedPool(UINT32 uiChn, UINT8 *pucAddr, UINT32 uiSize)
{
    DEC_SHARE_BUF *pstDecShare = &g_pDspInitPara->decShareBuf[uiChn];
    INT32 w, totalLen, part1, part2;
    UINT8 *p = NULL;

    /* INT32           i; */
    w = pstDecShare->writeIdx;
    p = pstDecShare->pVirtAddr;

    totalLen = pstDecShare->bufLen;

    part1 = totalLen - w;

    if (uiSize > part1)
    {
        memcpy((PUINT8)(p + w), pucAddr, part1);
        part2 = uiSize - part1;
        memcpy((PUINT8)p, pucAddr + part1, part2);
    }
    else
    {
        memcpy((PUINT8)(p + w), pucAddr, uiSize);
    }

    pstDecShare->writeIdx = (pstDecShare->writeIdx + uiSize) % pstDecShare->bufLen;
    return SAL_SOK;
}

/**
 * @function   SystemPrm_writeRecPool
 * @brief      写入录像内存
 * @param[in]  UINT32 chan                        通道
 * @param[in]  INT32 streamId                     流ID
 * @param[in]  REC_STREAM_INFO_ST *pstStreamInfo  流信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeRecPool(UINT32 chan, INT32 streamId, REC_STREAM_INFO_ST *pstStreamInfo)
{
    INT32 w = 0;
    INT32 r = 0;
    UINT32 bIFrameStart = 0;
    UINT32 IFrameLen = 0;
    INT32 spareLen = 0;
    INT32 part1 = 0;
    INT32 part2 = 0;
    UINT32 totalLen = 0;
    UINT8 *p = NULL;

    IFRAME_INFO_ARRAY *ifInfo = NULL;
    REC_POOL_INFO *pstRecPoolInfo = NULL;
    STREAM_IFRAME_INFO *pstInfo = NULL;

    SYS_PRM_DEV_INFO *pstSysPrmDevInfo = NULL;
    SYS_PRM_CHN_INFO *pstSysPrmChnInfo = NULL;

    pstSysPrmDevInfo = &gstSysPrmCtrl.stSysPrmDevInfo[chan];
    pstSysPrmChnInfo = &pstSysPrmDevInfo->stSysPrmChnInfo[streamId];
    if (pstSysPrmDevInfo == NULL || pstSysPrmChnInfo == NULL)
    {
        SYS_LOGE("pstSysPrmDevInfo == NULL || pstSysPrmChnInfo == NULL \n");
        return SAL_SOK;
    }

    if (NULL == pstStreamInfo)
    {
        SYS_LOGE("pstStreamInfo =  NULL\n");
        return SAL_SOK;
    }

    bIFrameStart = (pstStreamInfo->IFrameInfo & 0x1);
    IFrameLen = ((pstStreamInfo->IFrameInfo >> 1) & 0x7FFFFFFF);

    if (NULL == pstStreamInfo->pucAddr)
    {
        SYS_LOGE("pucAddr is null\n");
        return SAL_SOK;
    }

    if (chan >= g_pDspInitPara->encChanCnt)
    {
        SYS_LOGE("chan %d,encChanCnt %d\n", chan, g_pDspInitPara->encChanCnt);
        return SAL_FAIL;
    }

    if (0 == streamId)
    {
        /* 主码流 */
        pstRecPoolInfo = &g_DspInitPara.RecPoolMain[chan];
    }
    else if (1 == streamId)
    {
        /* 子码流 */
        pstRecPoolInfo = &g_DspInitPara.RecPoolSub[chan];
    }
    else
    {
        pstRecPoolInfo = &g_DspInitPara.RecPoolThird[chan];
    }

    if (pstRecPoolInfo->streamType == STREAM_VIDEO && !pstStreamInfo->uiType)
    {
        SYS_LOGE("chan %d, streamId %d\n", chan, streamId);
        return SAL_SOK;
    }

    SAL_mutexLock(pstSysPrmChnInfo->MutexHandle);
    ifInfo = &pstRecPoolInfo->ifInfo;
    w = pstRecPoolInfo->wOffset;
    r = pstRecPoolInfo->rOffset;
    totalLen = pstRecPoolInfo->totalLen;
    p = pstRecPoolInfo->vAddr;

    if (pstStreamInfo->uiType && (pstStreamInfo->IFrameInfo & 0x1))
    {
        pstRecPoolInfo->vFrmCounter++;
    }

    spareLen = (r + totalLen - w - 1) % totalLen;


    /*预留4k的空闲*/
    if ((pstStreamInfo->uiSize + (4 * 1024)) > spareLen)
    {
        /* getBufOverFlowTime(chan, pts); */
        /* SYS_LOGW("get stream too slow\n"); */
        SAL_mutexUnlock(pstSysPrmChnInfo->MutexHandle);
        return SAL_SOK;
    }

    pstRecPoolInfo->wErrTime = 0;
    part1 = totalLen - w;

    if (pstStreamInfo->uiSize > part1)
    {

        memcpy((PUINT8)(p + w), (PUINT8)pstStreamInfo->pucAddr, part1);

        part2 = pstStreamInfo->uiSize - part1;

        memcpy((PUINT8)p, (PUINT8)pstStreamInfo->pucAddr + part1, part2);

    }
    else
    {
        memcpy((PUINT8)(p + w), (PUINT8)pstStreamInfo->pucAddr, pstStreamInfo->uiSize);
    }

    /* I帧 */
    if (1 == pstStreamInfo->streamType && bIFrameStart)
    {
        pstInfo = &ifInfo->ifInfo[ifInfo->wIdx];
        pstInfo->len = IFrameLen;
        pstInfo->stdTime = pstStreamInfo->stdTime;
        pstInfo->uiAddr = w;

        memcpy((char *)(&pstInfo->dspAbsTime), (char *)(&pstStreamInfo->absTime), sizeof(DATE_TIME));

        GO_FORWARD(ifInfo->wIdx, 200);
    }

    /* 视频数据 */
    if (pstStreamInfo->uiType && bIFrameStart)
    {
        pstRecPoolInfo->lastFrameStdTime = pstStreamInfo->stdTime;
        memcpy((char *)(&pstRecPoolInfo->lastFrameAbsTime), (char *)(&pstStreamInfo->absTime), sizeof(DATE_TIME));
    }

    pstRecPoolInfo->wOffset = (pstRecPoolInfo->wOffset + pstStreamInfo->uiSize) % totalLen;
    SAL_mutexUnlock(pstSysPrmChnInfo->MutexHandle);
    return SAL_SOK;
}

/**
 * @function   SystemPrm_writeRecPoolByRecode
 * @brief      写入录像内存（转存）
 * @param[in]  UINT32 chan                        通道
 * @param[in]  REC_STREAM_INFO_ST *pstStreamInfo  流信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeRecPoolByRecode(UINT32 chan, REC_STREAM_INFO_ST *pstStreamInfo)
{
    INT32 w = 0;
    INT32 r = 0;
    UINT32 bIFrameStart = 0;
    UINT32 IFrameLen = 0;
    INT32 spareLen = 0;
    INT32 part1 = 0;
    INT32 part2 = 0;
    UINT32 totalLen = 0;
    UINT8 *p = NULL;

    IFRAME_INFO_ARRAY *ifInfo = NULL;
    REC_POOL_INFO *pstRecPoolInfo = NULL;
    STREAM_IFRAME_INFO *pstInfo = NULL;


    if (NULL == pstStreamInfo)
    {
        SYS_LOGE("pstStreamInfo =  NULL\n");
        return SAL_SOK;
    }

    bIFrameStart = (pstStreamInfo->IFrameInfo & 0x1);
    IFrameLen = ((pstStreamInfo->IFrameInfo >> 1) & 0x7FFFFFFF);

    if (NULL == pstStreamInfo->pucAddr)
    {
        SYS_LOGE("pucAddr is null\n");
        return SAL_SOK;
    }

    if (chan >= g_pDspInitPara->ipcChanCnt)
    {
        SYS_LOGE("chan %d,encChanCnt %d\n", chan, g_pDspInitPara->encChanCnt);
        return SAL_FAIL;
    }

    pstRecPoolInfo = &g_DspInitPara.RecPoolRecode[chan];


    if (pstRecPoolInfo->streamType == STREAM_VIDEO && !pstStreamInfo->uiType)
    {
        SYS_LOGE("\n");
        return SAL_SOK;
    }

    ifInfo = &pstRecPoolInfo->ifInfo;
    w = pstRecPoolInfo->wOffset;
    r = pstRecPoolInfo->rOffset;
    totalLen = pstRecPoolInfo->totalLen;
    p = pstRecPoolInfo->vAddr;

    if (pstStreamInfo->uiType && (pstStreamInfo->IFrameInfo & 0x1))
    {
        pstRecPoolInfo->vFrmCounter++;
    }

    spareLen = (r + totalLen - w - 1) % totalLen;


    /*预留4k的空闲*/
    if ((pstStreamInfo->uiSize + (4 * 1024)) > spareLen)
    {
        /* getBufOverFlowTime(chan, pts); */
        /* SYS_LOGW("get stream too slow\n"); */
        return SAL_SOK;
    }

    pstRecPoolInfo->wErrTime = 0;
    part1 = totalLen - w;

    if (pstStreamInfo->uiSize > part1)
    {

        memcpy((PUINT8)(p + w), (PUINT8)pstStreamInfo->pucAddr, part1);

        part2 = pstStreamInfo->uiSize - part1;

        memcpy((PUINT8)p, (PUINT8)pstStreamInfo->pucAddr + part1, part2);

    }
    else
    {
        memcpy((PUINT8)(p + w), (PUINT8)pstStreamInfo->pucAddr, pstStreamInfo->uiSize);
    }

    /* I帧 */
    if (1 == pstStreamInfo->streamType && bIFrameStart)
    {
        pstInfo = &ifInfo->ifInfo[ifInfo->wIdx];
        pstInfo->len = IFrameLen;
        pstInfo->stdTime = pstStreamInfo->stdTime;
        pstInfo->uiAddr = w;

        memcpy((char *)(&pstInfo->dspAbsTime), (char *)(&pstStreamInfo->absTime), sizeof(DATE_TIME));

        GO_FORWARD(ifInfo->wIdx, 200);
    }

    /* 视频数据 */
    if (pstStreamInfo->uiType && bIFrameStart)
    {
        pstRecPoolInfo->lastFrameStdTime = pstStreamInfo->stdTime;
        memcpy((char *)(&pstRecPoolInfo->lastFrameAbsTime), (char *)(&pstStreamInfo->absTime), sizeof(DATE_TIME));
    }

    pstRecPoolInfo->wOffset = (pstRecPoolInfo->wOffset + pstStreamInfo->uiSize) % totalLen;
    return SAL_SOK;
}

/**
 * @function   SystemPrm_resetRecPool
 * @brief      复位录像缓存池，主要是将读写指针置0
 * @param[in]  UINT32 chan                        通道
 * @param[in]  INT32 streamId                     流ID
 * @param[in]  REC_STREAM_INFO_ST *pstStreamInfo  流信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_resetRecPool(UINT32 chan, INT32 streamId, REC_STREAM_INFO_ST *pstStreamInfo)
{
    REC_POOL_INFO *pstRecPoolInfo = NULL;
    SYS_PRM_DEV_INFO *pstSysPrmDevInfo = NULL;
    SYS_PRM_CHN_INFO *pstSysPrmChnInfo = NULL;

    pstSysPrmDevInfo = &gstSysPrmCtrl.stSysPrmDevInfo[chan];
    pstSysPrmChnInfo = &pstSysPrmDevInfo->stSysPrmChnInfo[streamId];

    if (NULL == pstStreamInfo)
    {
        SYS_LOGE("\n");
        return SAL_SOK;
    }

    if (NULL == pstStreamInfo->pucAddr)
    {
        SYS_LOGE("pucAddr is null\n");
        return SAL_SOK;
    }

    if (chan >= g_pDspInitPara->encChanCnt)
    {
        SYS_LOGE("chan %d,encChanCnt %d\n", chan, g_pDspInitPara->encChanCnt);
        return SAL_FAIL;
    }

    if (0 == streamId)
    {
        /* 主码流 */
        pstRecPoolInfo = &g_DspInitPara.RecPoolMain[chan];
    }
    else if (1 == streamId)
    {
        /* 子码流 */
        pstRecPoolInfo = &g_DspInitPara.RecPoolSub[chan];
    }
    else
    {
        pstRecPoolInfo = &g_DspInitPara.RecPoolThird[chan];
    }

    if (pstRecPoolInfo->streamType == STREAM_VIDEO && !pstStreamInfo->uiType)
    {
        SYS_LOGE("\n");
        return SAL_SOK;
    }

    SAL_mutexLock(pstSysPrmChnInfo->MutexHandle);
    pstRecPoolInfo->rOffset = 0;
    pstRecPoolInfo->wOffset = 0;
    memset(&pstRecPoolInfo->ifInfo, 0x0, sizeof(IFRAME_INFO_ARRAY));
    SAL_mutexUnlock(pstSysPrmChnInfo->MutexHandle);
    return SAL_SOK;
}

/**
 * @function   SystemPrm_writeToNetPool
 * @brief      写入网传内存
 * @param[in]  UINT32 chan                        通道
 * @param[in]  INT32 streamId                     流ID
 * @param[in]  NET_STREAM_INFO_ST *pstStreamInfo  流信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeToNetPool(UINT32 chan, INT32 streamId, NET_STREAM_INFO_ST *pstStreamInfo)
{
    INT32 w = 0;
    INT32 part1 = 0;
    INT32 part2 = 0;
    UINT32 totalLen = 0;

    char *p = NULL;
    NET_POOL_INFO *netPool = NULL;

    SYS_PRM_DEV_INFO *pstSysPrmDevInfo = NULL;
    SYS_PRM_CHN_INFO *pstSysPrmChnInfo = NULL;

    pstSysPrmDevInfo = &gstSysPrmCtrl.stSysPrmDevInfo[chan];
    pstSysPrmChnInfo = &pstSysPrmDevInfo->stSysPrmChnInfo[streamId];

    if (NULL == pstStreamInfo)
    {
        return SAL_FAIL;
    }

    if (NULL == pstStreamInfo->pucAddr)
    {
        return SAL_FAIL;
    }

    if (streamId > 3)
    {
        SYS_LOGE("\n");
        return SAL_FAIL;
    }

    if (chan >= g_pDspInitPara->encChanCnt)
    {
        return SAL_FAIL;
    }

    if (0 == streamId)
    {
        /* 主码流 */
        netPool = &g_DspInitPara.NetPoolMain[chan];
    }
    else if (1 == streamId)
    {
        /* 子码流 */
        netPool = &g_DspInitPara.NetPoolSub[chan];
    }
    else
    {
        SYS_LOGE("chan %d streamId %d not sunpport Netpool \n", chan, streamId);
        return SAL_FAIL;
    }

    SAL_mutexLock(pstSysPrmChnInfo->MutexHandle);
    if (1 == pstStreamInfo->uiType)
    {
        netPool->vFrmCounter++;
    }

    w = netPool->wIdx;
    totalLen = netPool->totalLen;
    p = (char *)netPool->vAddr;


    if (pstStreamInfo->uiSize >= totalLen)
    {
        SAL_mutexUnlock(pstSysPrmChnInfo->MutexHandle);
        return SAL_FAIL;
    }

    part1 = totalLen - w;

    if (pstStreamInfo->uiSize > part1)
    {
        memcpy((PUINT8)(p + w), (PUINT8)pstStreamInfo->pucAddr, part1);
        part2 = pstStreamInfo->uiSize - part1;
        memcpy((PUINT8)p, (PUINT8)pstStreamInfo->pucAddr + part1, part2);
    }
    else
    {
        memcpy((PUINT8)(p + w), (PUINT8)pstStreamInfo->pucAddr, pstStreamInfo->uiSize);
    }

    netPool->wIdx = (netPool->wIdx + pstStreamInfo->uiSize) % totalLen;
    netPool->totalWLen += pstStreamInfo->uiSize;
    if (netPool->totalWLen > (0xff000000 + netPool->totalLen))
    {
        netPool->totalWLen -= 0xff000000;
    }

    SAL_mutexUnlock(pstSysPrmChnInfo->MutexHandle);

    /*static int cnt_debug_enc = 0;
       if (chan == 0&& streamId == 0 && cnt_debug_enc >= 240 && 1 == pstStreamInfo->uiType)
       {
        cnt_debug_enc = 0;
        VENC_LOGI("wIdx %d totalWLen %d vAddr %p\n",netPool->wIdx,netPool->totalWLen, netPool->vAddr);
       }
       cnt_debug_enc++;*/
    return SAL_SOK;
}

/**
 * @function   SystemPrm_writeToNetPoolByRecode
 * @brief      写入网传内存
 * @param[in]  UINT32 chan                        通道
 * @param[in]  NET_STREAM_INFO_ST *pstStreamInfo  流信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeToNetPoolByRecode(UINT32 chan, NET_STREAM_INFO_ST *pstStreamInfo)
{
    INT32 w = 0;
    INT32 part1 = 0;
    INT32 part2 = 0;
    UINT32 totalLen = 0;

    char *p = NULL;
    NET_POOL_INFO *netPool = NULL;


    if (chan >= g_pDspInitPara->ipcChanCnt)
    {
        SYS_LOGE("chan is %d error\n", chan);
        return SAL_FAIL;
    }

    netPool = &g_DspInitPara.NetPoolRecode[chan];

    if (1 == pstStreamInfo->uiType)
    {
        netPool->vFrmCounter++;
    }

    w = netPool->wIdx;
    totalLen = netPool->totalLen;
    p = (char *)netPool->vAddr;


    if (pstStreamInfo->uiSize >= totalLen)
    {
        return SAL_FAIL;
    }

    part1 = totalLen - w;

    if (pstStreamInfo->uiSize > part1)
    {
        memcpy((PUINT8)(p + w), (PUINT8)pstStreamInfo->pucAddr, part1);
        part2 = pstStreamInfo->uiSize - part1;
        memcpy((PUINT8)p, (PUINT8)pstStreamInfo->pucAddr + part1, part2);
    }
    else
    {
        memcpy((PUINT8)(p + w), (PUINT8)pstStreamInfo->pucAddr, pstStreamInfo->uiSize);
    }

    netPool->wIdx = (netPool->wIdx + pstStreamInfo->uiSize) % totalLen;
    netPool->totalWLen += pstStreamInfo->uiSize;
    if (netPool->totalWLen > (0xff000000 + netPool->totalLen))
    {
        netPool->totalWLen -= 0xff000000;
    }

    return SAL_SOK;
}

/**
 * @function   SystemPrm_Init
 * @brief      系统参数初始化
 * @param[in]  void
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_Init(void)
{
    UINT32 uiDev = 0;
    UINT32 uiChn = 0;
    SYS_PRM_DEV_INFO *pstSysPrmDevInfo = NULL;
    SYS_PRM_CHN_INFO *pstSysPrmChnInfo = NULL;

    memset(&gstSysPrmCtrl, 0, sizeof(SYS_PRM_CTRL));

    gstSysPrmCtrl.DevNum = g_pDspInitPara->encChanCnt;

    for (uiDev = 0; uiDev < gstSysPrmCtrl.DevNum; uiDev++)
    {
        pstSysPrmDevInfo = &gstSysPrmCtrl.stSysPrmDevInfo[uiDev];
        for (uiChn = 0; uiChn < VENC_CHN_MAX_NUM; uiChn++)
        {
            pstSysPrmChnInfo = &pstSysPrmDevInfo->stSysPrmChnInfo[uiChn];
            SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstSysPrmChnInfo->MutexHandle);
        }
    }

    return SAL_SOK;

}

/**
 * @function   SystemPrm_getDspInitPara
 * @brief      获取全局结构体指针
 * @param[in]  void
 * @param[out] None
 * @return     DSPINITPARA *
 */
DSPINITPARA *SystemPrm_getDspInitPara(void)
{
    return g_pDspInitPara;
}

/**
 * @function   InitDspPara
 * @brief      初始化共享信息
 * @param[in]  DSPINITPARA **ppDspInitPara  全局指针地址
 * @param[in]  DATACALLBACK pFunc           回调函数
 * @param[out] None
 * @return     INT32
 */
INT32 InitDspPara(DSPINITPARA **ppDspInitPara, DATACALLBACK pFunc)
{
    memset(&g_DspInitPara, 0, sizeof(DSPINITPARA));
    InitParam(&g_DspInitPara);
    *ppDspInitPara = &g_DspInitPara;
    pDataCallBackFunc = pFunc;

    g_pDspInitPara = &g_DspInitPara;

    return SAL_SOK;
}

