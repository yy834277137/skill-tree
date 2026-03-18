/**
 * @file   audio_tsk.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---音频业务模块
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建audio_drv.c文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取码流业务逻辑，保留主机通讯接口
 */

#include "dspcommon.h"
#include "type_dscrpt_common.h"
#include "libdemux.h"
#include "sal.h"
#include "system_prm_api.h"
#include "capbility.h"
#include "audio_tsk.h"
#include "audio_tsk_api.h"
#include "audio_drv_api.h"
#include "audioTTS_drv_api.h"
#include "audioResample_drv_api.h"
#include "audioCodec_drv_api.h"
#include "audioMux_drv_api.h"
#include "platform_hal.h"



/*****************************************************************************
                            宏定义
*****************************************************************************/

/*****************************************************************************
                         结构定义
*****************************************************************************/

/*  */
typedef enum
{
    MOUDLE_IDLE = 0x00,
    MOUDLE_PLAYBACK = 0x01,
    MOUDLE_SDKTB = 0x02,
    MOUDLE_AUDTB = 0x03,    /* 语音对讲(与室内机等) */
    MODULE_OTHER = 0x04,
} MOUDLE_STATE_E;

typedef struct tagCodecInfoSt
{
    /* Enc & Dec Handle , such as AAC, G722 ...*/
    CODEC_OPERATION_S stOpsList[ENC_TYPE_NUM];

    AUDIO_ENC_TYPE enCurType;        /* 当前音频编码类型*/
    /* UINT32          CurTalkBackType; */
    UINT32 bNeedHndl;
    void *Hndl;
} CODEC_PRM_S; /*编解码函数指针*/

typedef struct tagProcInfoSt
{
    CODEC_PRM_S stCodecPrm;  /*编解码操作*/
    AUDIO_INTERFACE_INFO_S stInterfaceInfo;   /* config this strcture when to call enc/dec  driver. */
    RESAMPLE_INFO_S stResamplePrm; /*重采样操作，解码需要*/
} PROCESS_INFO_S;

/*
 * 存放下行音频业务的数据源
 */

typedef struct tagPoolInfoSt
{
    unsigned char *vAddr;                   /* malloc when initializing*/
    /* unsigned char         *pAddr;           / * physic addr * / */
    volatile unsigned int wIdx;
    volatile unsigned int rIdx;
    /* volatile unsigned int  totalLen; */
    UINT32 u32BufLen;
    UINT32 u32CurDataLen;

    unsigned char *pTemp;

    pthread_mutex_t AudDemux;

} MODULE_POOL_INFO_S;

/*下行对讲通道，裸流处理，不涉及封装*/
typedef struct tagTalkBackDnCtrlInfoSt
{
    UINT32 u32Chn;
    UINT32 bEnable;                 /* 音频DataPool使能状态，不区分通道 */

    AUDIO_ENC_TYPE enType;          /* 音频解码类型 */

    UINT32 SrcSample;
    UINT32 bReSample;

    MODULE_POOL_INFO_S stDataPool;  /* 下行解码数据读写buf */
} TALK_BACK_DN_CTRL_INFO_S;

/*上行通道，编码的输入输出都由外部控制，需要封装*/
typedef struct tagTalkBackUpCtrlInfoSt
{
    UINT32 u32Chn;
    UINT32 bEnable;
    AUDIO_ENC_TYPE enType;                        /* 音频解码类型*/

    UINT32 bNeedPsPack;
    UINT32 bNeedRtpPack;
} TALK_BACK_UP_CTRL_INFO_S;

/*
    Audio Play 业务属性
 */
typedef struct tagPlayBackCtrlInfoSt
{
    UINT32 aoDev;                           /* AO 播放的音频设备号 */
    UINT32 aoChan;                          /* AO 播放的音频通道 */
    UINT32 dataLen;                         /* 有效音频数据的总长度 */
    UINT32 u32LoopCnt; /* 划分为个小帧进行播放 */                       /* */
    UINT32 frmNum;                          /* 帧数: 总长度除以每帧长度 , 每帧的长度根据格式而变化*/
    UINT32 RdFrameLen;                      /* The data len that sent to be decoded at once time */
    UINT32 enAudioPlayType;                 /* 本播放通道正在播放的铃音文件类型优先级，全数字播放通道1和半双数字播放通道0需要控制优先级 */
    UINT32 playTime;                        /* 设定的播放时间:单位ms，0-播放一遍 非0-播放固定时间，一遍结束后重复播放 */
    UINT64 startTime;                       /* 开始播放的系统时间 */
    INT32 stopHand;                          /* 立刻停止 */
    INT32 bEnable;                           /* 播放使能 */
    AUDIO_ENC_TYPE enType;                  /* 铃声的编码格式*/

    MODULE_POOL_INFO_S stDataPool;

    /* pthread_cond_t  pbcond; */
    pthread_mutex_t muOnPlayAudio;
} AUDIO_PLAY_CTRL_INFO_S;

/* 音频上行结构体 */
typedef struct tagAudUpLineInfo
{
    CHN_INFO_S stAudChnInfo[DEV_MAX_NUM];      /* ai chn num*/
    AUDIO_PACK_INFO_S stAudPackInfo[DEV_MAX_NUM];     /* 上行的音频打包信息 */

    TALK_BACK_UP_CTRL_INFO_S stSdkTbCtrl;       /* sdk  对讲控制  */
    TALK_BACK_UP_CTRL_INFO_S stAudTbCtrl;       /* 语音 对讲控制 */

    PROCESS_INFO_S stSdkTbProcInfo;          /* 上行可能存在sdk对讲与拉复合流同时存在的情况且两种情况的音频编码格式不同 */
    PROCESS_INFO_S stAudTbProcInfo;

    AUDIO_BUF_PRM_S stSdkTbDataBuf;  /*SDK对讲固定发送160字节，需要做备份*/
} AUD_UPLINE_INFO_S;

/* 音频下行结构体 */
typedef struct tagAudDnLineInfo
{
    MOUDLE_STATE_E enDnState;              /* 播放音频路线状态  */

    AUDIO_PLAY_CTRL_INFO_S stAudioPlayCtrl;    /* 播放铃音控制  */
    TALK_BACK_DN_CTRL_INFO_S stSdkTbCtrl;       /* sdk  对讲控制  */
    TALK_BACK_DN_CTRL_INFO_S stAudTbCtrl;       /* 语音 对讲控制 */

    PROCESS_INFO_S procInfo;          /* 铃声播放/sdk对讲/语音对讲，三者只存在其一 */
    AUDIO_BUF_PRM_S stAoBufPrm;
} AUD_DNLINE_INFO_S;

typedef struct
{
    void *pTtsHandle;  /*语音合成*/

    AUD_UPLINE_INFO_S stAudUpInfo;              /* 音频上行 */
    AUD_DNLINE_INFO_S stAudDnInfo;              /* 音频下行 */

    SAL_ThrHndl pidOut;
    SAL_ThrHndl pidIn;
} AUDIO_TSK_CTRL_S;


/*****************************************************************************
                            全局结构体
*****************************************************************************/
static AUDIO_TSK_CTRL_S gstAudioTskCtrl = {0};
static INT32 aacAdtsHeaderSamplingFrequency[16] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 0, 0, 0};
static UINT32 gu32AudioEnableChn = 0;       /* 按位表示16个IPC解码通道音频播放使能状态 */

/*****************************************************************************
                        函数定义
*****************************************************************************/

/**
 * @function   audio_initDnLine
 * @brief    音频下行播放所需内存的初始化
 * @param[in]  None
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_initDnLine(void)
{
    PROCESS_INFO_S *pstProcInfo = &gstAudioTskCtrl.stAudDnInfo.procInfo;               /* 编解码接口 */
    AUDIO_PLAY_CTRL_INFO_S *pstPlayBackCtrl = &gstAudioTskCtrl.stAudDnInfo.stAudioPlayCtrl;
    MODULE_POOL_INFO_S *pstPlayBackInPool = &gstAudioTskCtrl.stAudDnInfo.stAudioPlayCtrl.stDataPool;
    MODULE_POOL_INFO_S *pstSdkTbInPool = &gstAudioTskCtrl.stAudDnInfo.stSdkTbCtrl.stDataPool;
    MODULE_POOL_INFO_S *pstAudTbInPool = &gstAudioTskCtrl.stAudDnInfo.stAudTbCtrl.stDataPool;

    /* 初始化铃声播放所需要的锁 */
    pthread_mutex_init(&pstPlayBackCtrl->muOnPlayAudio, NULL);

    /* 铃声播放的InPool内存初始化 */
    pstPlayBackInPool->u32BufLen = AUDIO_PLAY_BACK_BUF_LEN;      /* */
    pstPlayBackInPool->u32CurDataLen = 0;
    pstPlayBackInPool->vAddr = SAL_memMalloc(pstPlayBackInPool->u32BufLen, "audio", "tsk");      /* AUDIO_PLAY_BACK_BUF_LEN */
    if (NULL == pstPlayBackInPool->vAddr)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    memset(pstPlayBackInPool->vAddr, 0x0, pstPlayBackInPool->u32BufLen);

    /* SDK对讲的InPool内存初始化 */
    pstSdkTbInPool->u32BufLen = SDK_TALK_BACK_BUF_LEN;      /* */
    pstSdkTbInPool->u32CurDataLen = 0;
    pstSdkTbInPool->vAddr = SAL_memMalloc(pstSdkTbInPool->u32BufLen, "audio", "tsk");      /* AUDIO_PLAY_BACK_BUF_LEN */
    if (NULL == pstSdkTbInPool->vAddr)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }


    memset(pstSdkTbInPool->vAddr, 0x0, pstSdkTbInPool->u32BufLen);

    /* 语音对讲的InPool内存初始化 */
    pthread_mutex_init(&pstAudTbInPool->AudDemux, NULL);
    pstAudTbInPool->u32BufLen = AUD_TALK_BACK_BUF_LEN;      /* */
    pstAudTbInPool->u32CurDataLen = 0;
    pstAudTbInPool->vAddr = SAL_memMalloc(pstAudTbInPool->u32BufLen * 2, "audio", "tsk");      /* AUDIO_PLAY_BACK_BUF_LEN */
    if (NULL == pstAudTbInPool->vAddr)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    pstAudTbInPool->pTemp = pstAudTbInPool->vAddr + pstAudTbInPool->u32BufLen;


    memset(pstAudTbInPool->vAddr, 0x0, pstAudTbInPool->u32BufLen * 2);

    pstProcInfo->stInterfaceInfo.uiOutputBufLen = AUD_OUTPUT_BUF_LEN;
    pstProcInfo->stInterfaceInfo.pOutputBuf = SAL_memMalloc(pstProcInfo->stInterfaceInfo.uiOutputBufLen, "audio", "tsk");
    pstProcInfo->stInterfaceInfo.uiOutputLen = 0;
    if (NULL == pstProcInfo->stInterfaceInfo.pOutputBuf)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    pstProcInfo->stInterfaceInfo.pInTmpBuf = SAL_memMalloc(AUD_ENC_TMP_BUF_LEN, "audio", "tsk");
    pstProcInfo->stInterfaceInfo.uiInTmpBufLen = AUD_ENC_TMP_BUF_LEN;
    pstProcInfo->stInterfaceInfo.uiInTmpLen = 0;
    if (NULL == pstProcInfo->stInterfaceInfo.pInTmpBuf)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    pstProcInfo->stResamplePrm.pInputBuf = SAL_memMalloc(AUDIO_AAC_RESAMPLE_FRM_LEN, "audio", "tsk");
    pstProcInfo->stResamplePrm.u32InputBufLen = AUDIO_AAC_RESAMPLE_FRM_LEN;
    if (NULL == pstProcInfo->stResamplePrm.pInputBuf)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    pstProcInfo->stResamplePrm.pOutputBuf = SAL_memMalloc(AUDIO_AAC_RESAMPLE_FRM_LEN, "audio", "tsk");
    pstProcInfo->stResamplePrm.u32OutputBufLen = AUDIO_AAC_RESAMPLE_FRM_LEN;
    if (NULL == pstProcInfo->stResamplePrm.pOutputBuf)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    gstAudioTskCtrl.stAudDnInfo.stAoBufPrm.u32BufSize = AUDIO_AAC_RESAMPLE_FRM_LEN;
    gstAudioTskCtrl.stAudDnInfo.stAoBufPrm.pDataBuf = SAL_memMalloc(AUDIO_AAC_RESAMPLE_FRM_LEN, "audio", "tsk");
    gstAudioTskCtrl.stAudDnInfo.stAoBufPrm.u32RestDataLen = 0;
    if (NULL == gstAudioTskCtrl.stAudDnInfo.stAoBufPrm.pDataBuf)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    /* 注册音频下行的解码函数 */
    audioCodec_drv_decRegister(ENC_TYPE_NUM, &pstProcInfo->stCodecPrm.stOpsList[0]);

    return SAL_SOK;
}

/**
 * @function   audio_initUpLine
 * @brief    音频上行播放所需内存的初始化
 * @param[in]  None
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_initUpLine(void)
{
    PROCESS_INFO_S *pstAudTbProcInfo = &gstAudioTskCtrl.stAudUpInfo.stAudTbProcInfo;
    PROCESS_INFO_S *pstSdkTbProcInfo = &gstAudioTskCtrl.stAudUpInfo.stSdkTbProcInfo;

    /* 初始化 RTP/PS 音频打包句柄 */
    if (SAL_SOK != audioMux_drv_init(&gstAudioTskCtrl.stAudUpInfo.stAudPackInfo[DEV_VISIBLE_LIGHT]))
    {
        AUD_LOGE("Audio_drvInitDnLineMem error !!!\n");
        return SAL_FAIL;
    }

    /* 注册语音对讲所需要的音频编码处理函数 */
    pstAudTbProcInfo->stInterfaceInfo.uiOutputBufLen = AUD_ENC_TMP_BUF_LEN;
    pstAudTbProcInfo->stInterfaceInfo.pOutputBuf = SAL_memMalloc(AUD_ENC_TMP_BUF_LEN, "audio", "tsk");
    pstAudTbProcInfo->stInterfaceInfo.uiOutputLen = 0;
    if (NULL == pstAudTbProcInfo->stInterfaceInfo.pOutputBuf)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    pstAudTbProcInfo->stInterfaceInfo.pInTmpBuf = SAL_memMalloc(AUD_ENC_TMP_BUF_LEN, "audio", "tsk");
    pstAudTbProcInfo->stInterfaceInfo.uiInTmpBufLen = AUD_ENC_TMP_BUF_LEN;
    pstAudTbProcInfo->stInterfaceInfo.uiInTmpLen = 0;
    if (NULL == pstAudTbProcInfo->stInterfaceInfo.pInTmpBuf)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    #if 0
    pstAudTbProcInfo->stResamplePrm.pInputBuf = SAL_memMalloc(AUDIO_AAC_RESAMPLE_FRM_LEN, "audio", NULL);
    pstAudTbProcInfo->stResamplePrm.u32InputBufLen = AUDIO_AAC_RESAMPLE_FRM_LEN;
    pstAudTbProcInfo->stResamplePrm.uiInputLen = 0;
    pstAudTbProcInfo->stResamplePrm.pOutputBuf = SAL_memMalloc(AUDIO_AAC_RESAMPLE_FRM_LEN);
    pstAudTbProcInfo->stResamplePrm.u32OutputBufLen = AUDIO_AAC_RESAMPLE_FRM_LEN;
    pstAudTbProcInfo->stResamplePrm.uiOutputLen = 0;
    #endif
    audioCodec_drv_encRegister(ENC_TYPE_NUM, &pstAudTbProcInfo->stCodecPrm.stOpsList[0]);

    /* 注册 SDK 对讲所需要的音频编码处理函数 */
    pstSdkTbProcInfo->stInterfaceInfo.uiOutputBufLen = AUD_ENC_TMP_BUF_LEN;
    pstSdkTbProcInfo->stInterfaceInfo.pOutputBuf = SAL_memMalloc(AUD_ENC_TMP_BUF_LEN, "audio", "tsk");
    pstSdkTbProcInfo->stInterfaceInfo.uiOutputLen = 0;
    if (NULL == pstSdkTbProcInfo->stInterfaceInfo.pOutputBuf)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    pstSdkTbProcInfo->stInterfaceInfo.pInTmpBuf = SAL_memMalloc(AUD_ENC_TMP_BUF_LEN, "audio", "tsk");
    pstSdkTbProcInfo->stInterfaceInfo.uiInTmpBufLen = AUD_ENC_TMP_BUF_LEN;
    pstSdkTbProcInfo->stInterfaceInfo.uiInTmpLen = 0;
    if (NULL == pstSdkTbProcInfo->stInterfaceInfo.pInTmpBuf)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    gstAudioTskCtrl.stAudUpInfo.stSdkTbDataBuf.pDataBuf = SAL_memMalloc(AUD_ENC_TMP_BUF_LEN, "audio", "tsk");
    gstAudioTskCtrl.stAudUpInfo.stSdkTbDataBuf.u32BufSize = AUD_ENC_TMP_BUF_LEN;
    gstAudioTskCtrl.stAudUpInfo.stSdkTbDataBuf.u32RestDataLen = 0;
    if (NULL == gstAudioTskCtrl.stAudUpInfo.stSdkTbDataBuf.pDataBuf)
    {
        AUD_LOGE("SAL_memMalloc err !\n");
        return SAL_FAIL;
    }

    #if 0
    pstSdkTbProcInfo->stResamplePrm.pInputBuf = SAL_memMalloc(AUDIO_AAC_RESAMPLE_FRM_LEN, "audio", NULL);
    pstSdkTbProcInfo->stResamplePrm.u32InputBufLen = AUDIO_AAC_RESAMPLE_FRM_LEN;
    pstSdkTbProcInfo->stResamplePrm.uiInputLen = 0;

    pstSdkTbProcInfo->stResamplePrm.pOutputBuf = SAL_memMalloc(AUDIO_AAC_RESAMPLE_FRM_LEN, "audio", NULL);
    pstSdkTbProcInfo->stResamplePrm.u32OutputBufLen = AUDIO_AAC_RESAMPLE_FRM_LEN;
    pstSdkTbProcInfo->stResamplePrm.uiOutputLen = 0;
    #endif
    audioCodec_drv_encRegister(ENC_TYPE_NUM, &pstSdkTbProcInfo->stCodecPrm.stOpsList[0]);

    return SAL_SOK;
}

/**
 * @function   audio_process
 * @brief    音频编解码处理通用流程
 * @param[in]  PROCESS_INFO_S *pstProcessInfo 待处理数据
 * @param[out]  PROCESS_INFO_S *pstProcessInfo 编解码输出数据
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_process(PROCESS_INFO_S *pstProcessInfo)
{
    int iRet = 0;
    AUDIO_ENC_TYPE enEncType = pstProcessInfo->stInterfaceInfo.enDstType;
    CODEC_PRM_S *pstCodecPrm = &pstProcessInfo->stCodecPrm;

    if (ENC_TYPE_NUM <= enEncType)
    {
        SAL_ERROR("unsupport enc type %d\n", enEncType);
        return SAL_FAIL;
    }

    if (SAL_TRUE == pstProcessInfo->stInterfaceInfo.bClrTmp)
    {
        pstCodecPrm->stOpsList[enEncType].PreProc(&pstProcessInfo->stInterfaceInfo);
        pstProcessInfo->stInterfaceInfo.bClrTmp = 0;
    }

    if (pstCodecPrm->enCurType != enEncType)
    {
        pstCodecPrm->enCurType = enEncType;
        pstCodecPrm->bNeedHndl = pstCodecPrm->stOpsList[enEncType].bNeedHndl;
        pstCodecPrm->Hndl = pstCodecPrm->stOpsList[enEncType].hndl;

        pstCodecPrm->stOpsList[enEncType].PreProc(&pstProcessInfo->stInterfaceInfo);
        /* clear data stored which is planed to deal with last enc/dec type. */
    }

    if (NULL == pstCodecPrm->Hndl && SAL_TRUE == pstCodecPrm->bNeedHndl) /* Need handle, but haven't created. */
    {
        iRet = pstCodecPrm->stOpsList[enEncType].Init(&pstCodecPrm->stOpsList[enEncType].hndl);
        if (SAL_SOK != iRet) /* Enc/Dec init function. */
        {
            AUD_LOGE("Enc Hndl Init failed, err:%x !!!!\n", iRet);
            return iRet;
        }

        pstCodecPrm->Hndl = pstCodecPrm->stOpsList[enEncType].hndl;
    }

    iRet = pstCodecPrm->stOpsList[enEncType].Proc(pstCodecPrm->Hndl, &pstProcessInfo->stInterfaceInfo);
    if (SAL_FAIL == iRet) /* Enc/Dec actual process function. */
    {
        AUD_LOGE(" Enc Procesee failed with err:%x !!!!\n", iRet);
        return iRet;
    }

    return iRet;
}

/**
 * @function   audio_getPackInfo
 * @brief    获取音频封装信息
 * @param[in]  None
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static void audio_getPackInfo(void)
{
    TALK_BACK_UP_CTRL_INFO_S *pstAudTbCtrl = &gstAudioTskCtrl.stAudUpInfo.stAudTbCtrl;
    CHN_INFO_S *pstVLChnInfo = &gstAudioTskCtrl.stAudUpInfo.stAudChnInfo[DEV_VISIBLE_LIGHT];             /* 可见光设备 */

    /* CHN_INFO_S            *pstILChnInfo    = &pstTalkCtrlInfo->AudChnInfo[DEV_INFRARED_LIGHT]; */

    pstAudTbCtrl->bEnable = (SAL_TRUE == pstVLChnInfo->stStreamPackInfo[MAIN_STREAM_CHN].bNeedAud    \
                             || SAL_TRUE == pstVLChnInfo->stStreamPackInfo[SUB_STREAM_CHN].bNeedAud    \
                             || SAL_TRUE == pstVLChnInfo->stStreamPackInfo[THIRD_STREAM_CHN].bNeedAud) ? SAL_TRUE : SAL_FALSE;

    /* AUD_LOGE("----- pstAudTbCtrl->bEnable:%d -----\n", pstAudTbCtrl->bEnable); */

    pstAudTbCtrl->bNeedRtpPack = (SAL_TRUE == pstVLChnInfo->stStreamPackInfo[MAIN_STREAM_CHN].bNeedRtpPack   \
                                  || SAL_TRUE == pstVLChnInfo->stStreamPackInfo[SUB_STREAM_CHN].bNeedRtpPack    \
                                  || SAL_TRUE == pstVLChnInfo->stStreamPackInfo[THIRD_STREAM_CHN].bNeedRtpPack) ? SAL_TRUE : SAL_FALSE;

    pstAudTbCtrl->bNeedPsPack = (SAL_TRUE == pstVLChnInfo->stStreamPackInfo[MAIN_STREAM_CHN].bNeedPsPack   \
                                 || SAL_TRUE == pstVLChnInfo->stStreamPackInfo[SUB_STREAM_CHN].bNeedPsPack    \
                                 || SAL_TRUE == pstVLChnInfo->stStreamPackInfo[THIRD_STREAM_CHN].bNeedPsPack) ? SAL_TRUE : SAL_FALSE;

    /* pstAudTbCtrl->bEnable = SAL_TRUE; */

    return;

}

/**
 * @function   audio_tsk_soundInput
 * @brief 		开启音频输入录音
 * @param[in]  UINT32 u32Chn 通道号，无效
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */

INT32 audio_tsk_soundInput(UINT32 u32Chn)
{
    
	audio_tsk_Save(NULL);

    return SAL_SOK;
}

/**
 * @function   audio_tsk_Save
 * @brief    音频数据保存
 * @param[in]  void *prm 无效
 * @param[out]  None
 * @return      void
 */
void *audio_tsk_Save(void *prm) /* upLine */
{
	INT32 s32Ret = SAL_FAIL;
    SAL_AudioFrameBuf *pstAudFrmBuf = NULL;
    SAL_SET_THR_NAME();
    audio_getPackInfo();
    s32Ret = audio_drv_aiGetFrame(&pstAudFrmBuf);
    if (SAL_SOK != s32Ret)
    {
    	AUD_LOGD("========== Get audio frame failed =======\n");
    }
    return NULL;
}

/**
 * @function   audio_tsk_playSave
 * @brief 播放保存好的音频文件
 * @param[in]  void *pPrm 无效参数
 * @param[out] None
 * @return     void
 */
void *audio_tsk_playSave(void *pPrm)
{
    UINT8 *pDstData = NULL;
    UINT32 u32DstLen = 0;
    INT32 s32Ret = SAL_SOK;
    INT32 size = 0;

    SAL_SET_THR_NAME();
    FILE *pFileDec = NULL;
    
    pFileDec = fopen("./ai/tts.pcm", "rb+");
    
    if (NULL == pFileDec)
    {
        AUD_LOGE("open ./ai/tts.pcm failed !!!\n");
        return NULL;
    }
    pDstData  = (UINT8 *)(SAL_memMalloc(640, "audio", "play-save"));
    while (1)
    {
        if (NULL == pDstData)
        {
            AUD_LOGE("malloc failed len(640) !!!\n");
            break;
        }
        memset(pDstData, 0, 640);
        size = fread(pDstData, 1, 640, pFileDec);
        if(size<=0)
        {
            break;
        }
        u32DstLen = size;

        s32Ret = audio_drv_aoSendFrame(pDstData, u32DstLen, &gstAudioTskCtrl.stAudDnInfo.stAoBufPrm); /* play actual audio data. */
        if (SAL_SOK != s32Ret)
        {
            AUD_LOGE("audio_drv_aoSendFrame error !!!\n");
            break;
        }
    }
    SAL_memfree(pDstData, "audio", "play-save");
    pDstData = NULL;
    fclose(pFileDec);
    pFileDec = NULL;
    return NULL;
}


/**
 * @function   audio_tsk_encThread
 * @brief    音频编码线程
                    =====> enc ===> rtp/ps pack ===> NetPool/RecPool
          Get audio ||
                    =====> enc ===> SDK buffer
 * @param[in]  void *prm 无效
 * @param[out]  None
 * @return      void
 */
static void *audio_tsk_encThread(void *prm) /* upLine */
{
    AUD_UPLINE_INFO_S *pstUplineInfo = &gstAudioTskCtrl.stAudUpInfo;
    TALK_BACK_UP_CTRL_INFO_S *pstAudTbCtrl = &gstAudioTskCtrl.stAudUpInfo.stAudTbCtrl;
    TALK_BACK_UP_CTRL_INFO_S *pstSdkTbCtrl = &gstAudioTskCtrl.stAudUpInfo.stSdkTbCtrl;

    PROCESS_INFO_S *pstAudTbProcInfo = &gstAudioTskCtrl.stAudUpInfo.stAudTbProcInfo;
    PROCESS_INFO_S *pstSdkTbProcInfo = &gstAudioTskCtrl.stAudUpInfo.stSdkTbProcInfo;
    AUDIO_PACK_INFO_S *pstAudPackInfo = &gstAudioTskCtrl.stAudUpInfo.stAudPackInfo[DEV_VISIBLE_LIGHT];
    AUDIO_BUF_PRM_S *pstSdkTbBuf = &gstAudioTskCtrl.stAudUpInfo.stSdkTbDataBuf;
    UINT32 i = 0;
    UINT32 u32FrameNum = 0;
    UINT8 *pOutData = NULL;
    UINT32 u32OutLen = 0;
    INT32 s32Ret = SAL_FAIL;
    UINT8 *pDstData = NULL;
    UINT32 u32DstLen = 0;

    AUD_BITS_INFO_S stAudBitsInfo = {0};
    NET_STREAM_INFO_ST stNetStreamInfo = {0};
    REC_STREAM_INFO_ST stRecStreamInfo = {0};

    /* HAL_AIO_FRAME_S *AudioFrame = NULL; */
    /* memset(&AudioFrame, 0x0, sizeof(HAL_AIO_FRAME_S)); */
    SAL_AudioFrameBuf *pstAudFrmBuf = NULL;

    SAL_SET_THR_NAME();

    while (1)
    {
        audio_getPackInfo();
        if (SAL_FALSE == pstSdkTbCtrl->bEnable && SAL_FALSE == pstAudTbCtrl->bEnable)
        {
            usleep(10 * 1000); /* consider to replace usleep with select . */
            continue;
        }

        /* pstAudFrmBuf = &g_stAudioHalPrm.stUpLineHalInfo; */
        s32Ret = audio_drv_aiGetFrame(&pstAudFrmBuf);
        if (SAL_SOK != s32Ret)
        {
            AUD_LOGD("========== Get audio frame failed =======\n");
            continue;
        }

#if 0
        FILE *pFileAi = fopen("/home/config/audio_ai.pcm", "a+");
        if (pFileAi)
        {
            fwrite((void *)pstAudFrmBuf->virAddr[0], 1, pstAudFrmBuf->bufLen, pFileAi);
            fclose(pFileAi);
        }

#endif

#if 0
        /*loop back*/
        s32Ret = audio_drv_aoSendFrame((UINT8 *)pstAudFrmBuf->virAddr[0], pstAudFrmBuf->bufLen, &gstAudioTskCtrl.stAudDnInfo.stAoBufPrm); /* play actual audio data. */
        if (SAL_SOK != s32Ret)
        {
            AUD_LOGE("audio_drv_aoSendFrame error !!!\n");
        }
#endif

        /* Sdk talk back module is enable. */
        if (SAL_TRUE == pstSdkTbCtrl->bEnable)
        {
            pstSdkTbProcInfo->stInterfaceInfo.enDstType = pstSdkTbCtrl->enType;
            pstSdkTbProcInfo->stInterfaceInfo.pInputBuf = (UINT8 *)pstAudFrmBuf->virAddr[0];
            pstSdkTbProcInfo->stInterfaceInfo.uiInputLen = pstAudFrmBuf->bufLen;
            s32Ret = audio_process(pstSdkTbProcInfo);
            if (AUDIO_CODEC_NEED_MORE_DATA == s32Ret)
            {
                AUD_LOGD("===== AUDIO_CODEC_NEED_MORE_DATA !!!! =====\n");
                continue;
            }
            else if (SAL_FAIL == s32Ret)
            {
                AUD_LOGE("==== DEC failed !!!! =====\n");
                usleep(10000); /* 10ms */
                continue;
            }

            /* aac~{Aw2;JG9L6(~}AUDIO_AAC_FRM_LEN~{PhR*4&@m~} */
            if (pstSdkTbCtrl->enType == AAC)
            {
                pOutData = pstSdkTbProcInfo->stInterfaceInfo.pOutputBuf;
                u32OutLen = pstSdkTbProcInfo->stInterfaceInfo.uiOutputLen;
                /* AUD_LOGI("u32OutLen %d,uiOutTmpLen %d\n",u32OutLen,pstSdkTbProcInfo->stInterfaceInfo.uiOutTmpLen); */
                if ((pstSdkTbBuf->u32RestDataLen > 0) && (u32OutLen + pstSdkTbBuf->u32RestDataLen >= AUDIO_AAC_FRM_LEN))
                {
                    /* ~{Sk@z4N2;9;~}AUDIO_AAC_FRM_LEN~{4sP!5DJ}>]Wi3IR;8v~}AUDIO_AAC_FRM_LEN~{4sP!5DV!7"KM3vH%~} */
                    memmove(pstSdkTbBuf->pDataBuf + pstSdkTbBuf->u32RestDataLen, pOutData, AUDIO_AAC_FRM_LEN - pstSdkTbBuf->u32RestDataLen);
                    pDstData = pstSdkTbBuf->pDataBuf;
                    u32DstLen = AUDIO_AAC_FRM_LEN;
                    SystemPrm_writeTalkBackPool(0, pDstData, u32DstLen);
                    pOutData += AUDIO_AAC_FRM_LEN - pstSdkTbBuf->u32RestDataLen;
                    u32OutLen -= AUDIO_AAC_FRM_LEN - pstSdkTbBuf->u32RestDataLen;
                    u32FrameNum = u32OutLen / AUDIO_AAC_FRM_LEN;
                    pstSdkTbBuf->u32RestDataLen = 0;
                }
                else
                {
                    u32FrameNum = u32OutLen / AUDIO_AAC_FRM_LEN;
                }

                if (u32FrameNum > 0)
                {
                    /* AUD_LOGI("u32OutLen %d,u32FrameNum %d\n",u32OutLen,u32FrameNum); */
                    for (i = 0; i < u32FrameNum; i++)
                    {
                        /* memmove(pstSdkTbProcInfo->stInterfaceInfo.pOutTmpBuf, pOutData, TALKBACK_FRM_LEN); */
                        pDstData = pOutData; /* pstSdkTbProcInfo->stInterfaceInfo.pOutTmpBuf; */
                        u32DstLen = AUDIO_AAC_FRM_LEN;
                        SystemPrm_writeTalkBackPool(0, pDstData, u32DstLen);
                        pOutData += AUDIO_AAC_FRM_LEN;
                    }
                }

                if (u32FrameNum * AUDIO_AAC_FRM_LEN < u32OutLen)
                {
                    memmove(pstSdkTbBuf->pDataBuf + pstSdkTbBuf->u32RestDataLen, pOutData, u32OutLen - u32FrameNum * AUDIO_AAC_FRM_LEN);
                    pstSdkTbBuf->u32RestDataLen += u32OutLen - u32FrameNum * AUDIO_AAC_FRM_LEN;
                }
            }
            else
            {
                pstSdkTbBuf->u32RestDataLen = 0;
                pDstData = pstSdkTbProcInfo->stInterfaceInfo.pOutputBuf;
                u32DstLen = pstSdkTbProcInfo->stInterfaceInfo.uiOutputLen;
                pstSdkTbProcInfo->stInterfaceInfo.uiOutputLen = 0;
                /* AUD_LOGI("u32DstLen %d\n",u32DstLen); */
                SystemPrm_writeTalkBackPool(0, pDstData, u32DstLen);
            }
        }

        if (SAL_TRUE == pstAudTbCtrl->bEnable) /* need audio */
        {
            pstAudTbProcInfo->stInterfaceInfo.enDstType = pstAudTbCtrl->enType;
            pstAudTbProcInfo->stInterfaceInfo.pInputBuf = (UINT8 *)pstAudFrmBuf->virAddr[0];
            pstAudTbProcInfo->stInterfaceInfo.uiInputLen = pstAudFrmBuf->bufLen;

            s32Ret = audio_process(pstAudTbProcInfo); /* encode */
            if (AUDIO_CODEC_NEED_MORE_DATA == s32Ret)
            {
                /* AUD_LOGI("===== AUDIO_CODEC_NEED_MORE_DATA !!!! =====\n"); */
                continue;
            }
            else if (SAL_FAIL == s32Ret)
            {
                AUD_LOGE("==== DEC failed !!!! =====\n");
                usleep(10000); /* 10ms */
                continue;
            }

#if 0

            SAL_INFO("enc new audio frm len %d\n", pstAudTbProcInfo->stInterfaceInfo.uiOutputLen);

            FILE *pFileEnc = fopen("/home/config/audio_enc.enc", "a+");
            if (pFileEnc)
            {
                fwrite(pDstData, 1, u32DstLen, pFileEnc);
                fclose(pFileEnc);
            }

#endif

            pDstData = pstAudTbProcInfo->stInterfaceInfo.pOutputBuf;
            u32DstLen = pstAudTbProcInfo->stInterfaceInfo.uiOutputLen;

            pstAudTbProcInfo->stInterfaceInfo.uiOutputLen = 0; /* clear */

        #if 1
            if (1 == pstAudTbCtrl->bNeedRtpPack)   /*  audio rtp pack */
            {
                stAudBitsInfo.pBitsDataAddr = pDstData;
                stAudBitsInfo.u32BitsDataLen = u32DstLen;
                stAudBitsInfo.u64TimeStamp = pstAudFrmBuf->pts;
                stAudBitsInfo.enAudioEncType = pstAudTbCtrl->enType;

                s32Ret = audioMux_drv_rtpProcess(pstAudPackInfo, &stAudBitsInfo);
                if (SAL_SOK == s32Ret)
                {
                    memset(&stNetStreamInfo, 0, sizeof(NET_STREAM_INFO_ST));
                    stNetStreamInfo.pucAddr = pstAudPackInfo->pPackOutputBuf;
                    stNetStreamInfo.uiSize = pstAudPackInfo->u32PackSize;
                    stNetStreamInfo.uiType = 0;

                    /* 将打包好后的音频码流下发到需要音频的NetPool中去 */

                    for (i = 0; i < DEV_MAX_NUM; i++)
                    {
                        if (SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[MAIN_STREAM_CHN].bNeedAud
                            && SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[MAIN_STREAM_CHN].bNeedRtpPack)
                        {
                            SystemPrm_writeToNetPool(i, MAIN_STREAM_CHN, &stNetStreamInfo);
                        }

                        if (SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[SUB_STREAM_CHN].bNeedAud
                            && SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[SUB_STREAM_CHN].bNeedRtpPack)
                        {
                            SystemPrm_writeToNetPool(i, SUB_STREAM_CHN, &stNetStreamInfo);
                        }

                        if (SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[THIRD_STREAM_CHN].bNeedAud
                            && SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[THIRD_STREAM_CHN].bNeedRtpPack)
                        {
                            SystemPrm_writeToNetPool(i, THIRD_STREAM_CHN, &stNetStreamInfo);
                        }
                    }
                }
            }

            if (1 == pstAudTbCtrl->bNeedPsPack)
            {
                stAudBitsInfo.pBitsDataAddr = pDstData;
                stAudBitsInfo.u32BitsDataLen = u32DstLen;
                stAudBitsInfo.u64TimeStamp = pstAudFrmBuf->pts;
                stAudBitsInfo.enAudioEncType = pstAudTbCtrl->enType;

                s32Ret = audioMux_drv_psProcess(pstAudPackInfo, &stAudBitsInfo);
                if (SAL_SOK == s32Ret)
                {
                    memset(&stRecStreamInfo, 0x0, sizeof(REC_STREAM_INFO_ST));
                    stRecStreamInfo.pucAddr = pstAudPackInfo->pPackOutputBuf;
                    stRecStreamInfo.uiSize = pstAudPackInfo->u32PackSize;
                    stRecStreamInfo.uiType = 0;
                    stRecStreamInfo.streamType = 0;

                    /* printf("====== handl:%d size:%d =====\n", muxProcInfo.muxHdl, stRecStreamInfo.uiSize); */

                    /* SystemPrm_writeRecPool(i, &stRecStreamInfo); */

                    for (i = 0; i < DEV_MAX_NUM; i++)
                    {
                        if (SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[MAIN_STREAM_CHN].bNeedAud
                            && SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[MAIN_STREAM_CHN].bNeedPsPack)
                        {
                            /* printf("----- audio ps pack ----\n"); */
                            SystemPrm_writeRecPool(i, MAIN_STREAM_CHN, &stRecStreamInfo);
                        }

                        if (SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[SUB_STREAM_CHN].bNeedAud
                            && SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[SUB_STREAM_CHN].bNeedPsPack)
                        {
                            SystemPrm_writeRecPool(i, SUB_STREAM_CHN, &stRecStreamInfo);
                        }

                        /* */
                        if (SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[THIRD_STREAM_CHN].bNeedAud
                            && SAL_TRUE == pstUplineInfo->stAudChnInfo[i].stStreamPackInfo[THIRD_STREAM_CHN].bNeedPsPack)
                        {
                            SystemPrm_writeRecPool(i, THIRD_STREAM_CHN, &stRecStreamInfo);
                        }
                    }
                }
            }

        #endif
        }
    }

    return NULL;
}

/**
 * @function   audio_checkRingDataEnd
 * @brief    判断是否播放结束, 用于判断文件单次播放结束 和规定时间播放结束
 * @param[in]  AUDIO_PLAY_CTRL_INFO_S *pstPlayWavCtrl 语音播报解码控制指针
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_checkRingDataEnd(AUDIO_PLAY_CTRL_INFO_S *pstPlayWavCtrl)
{
    UINT64 u64CurTime = 0;

    if (NULL == pstPlayWavCtrl)
    {
        AUD_LOGE("Null Point!\n");
        /* return SAL_FAIL; */
        return SAL_FALSE;
    }

    if (pstPlayWavCtrl->stopHand)
    {
        return SAL_TRUE;
    }

    /* 如果没有设定的播放时间，使用正常播放完有效数据即结束 */
    if (0 == pstPlayWavCtrl->playTime)
    {
        if (pstPlayWavCtrl->u32LoopCnt >= pstPlayWavCtrl->frmNum)
        {
            return SAL_TRUE;
        }
        else
        {
            return SAL_FALSE;
        }
    }
    else
    {
        /* 计算循环播放的时间 */
        u64CurTime = SAL_getTimeOfJiffies();
        u64CurTime = u64CurTime - pstPlayWavCtrl->startTime;

        if ((u64CurTime) >= pstPlayWavCtrl->playTime)
        {
            return SAL_TRUE;
        }
        else
        {
            return SAL_FALSE;
        }
    }

    return SAL_FALSE;
}

/**
 * @function   audio_checkTbDataUpdate
 * @brief    读取TB通道解码数据
 * @param[in]  MODULE_POOL_INFO_S *pstSrc 数据缓存结构指针
 * @param[out]  UINT8 **pVir 待解码数据
 * @param[out]  UINT32 *pInLen 待解码数据长度
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_checkTbDataUpdate(MODULE_POOL_INFO_S *pstSrc, UINT8 **pVir, UINT32 *pLen)
{
#if 1
    unsigned char *pvAddr = NULL;
    unsigned char *pTemp = NULL;
    UINT32 u32WIdx = 0;
    UINT32 u32RIdx = 0;
    UINT32 u32BufLen = 0;
    UINT32 u32DataLen = 0;
    UINT32 u32Part1 = 0;

    if (pstSrc->pTemp == NULL || pstSrc->vAddr == NULL)
    {
        AUD_LOGW("********** CurDataLen:%d *****u32BufLen %d***%p,%p**\n", pstSrc->u32CurDataLen, pstSrc->u32BufLen, pstSrc->pTemp, pstSrc->vAddr);
        return SAL_FAIL;
    }

    pvAddr = pstSrc->vAddr;
    pTemp = pstSrc->pTemp;

    u32WIdx = pstSrc->wIdx;
    u32RIdx = pstSrc->rIdx;
    /* u32BufLen   = pstSrc->totalLen; */
    u32BufLen = pstSrc->u32BufLen;

    /* printf(" rIdx:%d, wIdx:%d !!!!!!!\n", u32RIdx, u32WIdx); */

    if (u32WIdx == u32RIdx)
    {
        /* AUD_LOGI(" rIdx:%d, wIdx:%d, no data !!!!!!!\n", u32RIdx, u32WIdx); */
        /* SAL_msleep_o(10); */
        return SAL_FAIL;
    }

    u32DataLen = (u32RIdx < u32WIdx) ? (u32WIdx - u32RIdx) : (u32BufLen - u32RIdx + u32WIdx);

    if (u32RIdx < u32WIdx)
    {
        memcpy(pTemp, pvAddr + u32RIdx, u32DataLen);
    }
    else
    {
        u32Part1 = u32BufLen - u32RIdx;
        memcpy(pTemp, pvAddr + u32RIdx, u32Part1);
        memcpy(pTemp + u32Part1, pvAddr, u32WIdx);
    }

    pstSrc->rIdx = u32WIdx;

    *pVir = pTemp;
    *pLen = u32DataLen;

    /* AUD_LOGW("********** dataLen:%d **********\n", u32DataLen); */
#endif

    return SAL_SOK;
}

/**
 * @function   audio_getPlayState
 * @brief  获取音频解码状态：IPC解码，SDK对讲，振铃
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static UINT32 audio_getPlayState(void)
{
    AUD_DNLINE_INFO_S *pstAudDnInfo = &gstAudioTskCtrl.stAudDnInfo;
    AUDIO_PLAY_CTRL_INFO_S *pstPlayBackCtrl = &pstAudDnInfo->stAudioPlayCtrl;
    TALK_BACK_DN_CTRL_INFO_S *pstSdkTbCtrl = &pstAudDnInfo->stSdkTbCtrl;
    TALK_BACK_DN_CTRL_INFO_S *pstAudTbCtrl = &pstAudDnInfo->stAudTbCtrl;

    if (SAL_TRUE == pstSdkTbCtrl->bEnable)
    {
        pstAudDnInfo->enDnState = MOUDLE_SDKTB;
    }
    else if (SAL_TRUE == pstPlayBackCtrl->bEnable)
    {
        pstAudDnInfo->enDnState = MOUDLE_PLAYBACK;
    }
    else if (SAL_TRUE == pstAudTbCtrl->bEnable)
    {
        pstAudDnInfo->enDnState = MOUDLE_AUDTB;
    }
    else
    {
        pstAudDnInfo->enDnState = MOUDLE_IDLE;
    }

    return pstAudDnInfo->enDnState;
}

/**
 * @function   audio_getPlayData
 * @brief  获取铃声数据，并解码
 * @param[in]  UINT8 **ppAudFrm 解码输出帧地址
 * @param[in]  UINT32 *pInLen 解码输出帧长度
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_getPlayData(UINT8 **ppAudFrm, UINT32 *pInLen)
{
    UINT32 u32CurrentLen = 0;
    INT32 s32Ret = SAL_FAIL;
    AUDIO_PLAY_CTRL_INFO_S *pstPlayBackCtrl = &gstAudioTskCtrl.stAudDnInfo.stAudioPlayCtrl;
    PROCESS_INFO_S *pstProcInfo = &gstAudioTskCtrl.stAudDnInfo.procInfo;               /* 编解码接口 */
    MODULE_POOL_INFO_S *pstPoolInfo = &gstAudioTskCtrl.stAudDnInfo.stAudioPlayCtrl.stDataPool;
    UINT8 *pSrcData = NULL;
    DSPINITPARA *pstDspInitParm = SystemPrm_getDspInitPara();

    if (NULL == ppAudFrm || NULL == pInLen)
    {
        AUD_LOGE("!!!GetPlayData input error \n");
        return SAL_FAIL;
    }
    pthread_mutex_lock(&pstPlayBackCtrl->muOnPlayAudio);

    /* 计算已经播放的长度 */
    u32CurrentLen = pstPlayBackCtrl->u32LoopCnt * WAV_FRAME_LEN;
    /* u32CurrentLen = pstPlayBackCtrl->u32LoopCnt * 160;  //640 */
    pstPlayBackCtrl->u32LoopCnt++;

    if (SAL_TRUE == audio_checkRingDataEnd(pstPlayBackCtrl))
    {
        AUD_LOGW("playEnd u32LoopCnt=%d\n", pstPlayBackCtrl->u32LoopCnt);
        
        pstPlayBackCtrl->u32LoopCnt = 0;
        pstPlayBackCtrl->bEnable = SAL_FALSE;
        pstPlayBackCtrl->enAudioPlayType = AUDIO_PLAY_NON;
        pstDspInitParm->stAudioPlayBcakPrm.bAudIsPlaying = 0;
        pthread_mutex_unlock(&pstPlayBackCtrl->muOnPlayAudio);
        return SAL_FAIL;
    }

    pstDspInitParm->stAudioPlayBcakPrm.bAudIsPlaying = 1;

    /* ~{8|PBRQ>-2%7EJ}5DV!<FJ}#,~}loopCnt ~{H!S`<FJ}V'3VQ-;75]9i2%7ERtF5~} */
    pstPlayBackCtrl->u32LoopCnt = pstPlayBackCtrl->u32LoopCnt % pstPlayBackCtrl->frmNum;
    pthread_mutex_unlock(&pstPlayBackCtrl->muOnPlayAudio);
    /*******************当前只支持RAW格式************************/
    pSrcData = pstPoolInfo->vAddr + u32CurrentLen;
    if (RAW == pstPlayBackCtrl->enType) /* wav 这种无编码的裸数据 */
    {
        *ppAudFrm = pSrcData;
        *pInLen = WAV_FRAME_LEN;
        pstProcInfo->stInterfaceInfo.reSample = SAL_FALSE;
        return SAL_SOK;
    }
    else
    {
        pstProcInfo->stInterfaceInfo.enDstType = pstPlayBackCtrl->enType;
        pstProcInfo->stInterfaceInfo.pInputBuf = pSrcData;
        pstProcInfo->stInterfaceInfo.uiInputLen = WAV_FRAME_LEN; /* 160;//WAV_FRAME_LEN; */
        pstProcInfo->stInterfaceInfo.reSample = SAL_FALSE;

        s32Ret = audio_process(pstProcInfo);
        if (SAL_SOK == s32Ret)  /* only have 3 value returned , -1:failed; 0: successfully; 1: need more data. */
        {
            *ppAudFrm = pstProcInfo->stInterfaceInfo.pOutputBuf;
            *pInLen = pstProcInfo->stInterfaceInfo.uiOutputLen;
            pstProcInfo->stInterfaceInfo.uiOutputLen = 0;

            return SAL_SOK;
        }
        else
        {
            AUD_LOGE("==== DEC failed !!!! =====\n");
            return SAL_FAIL;
        }
    }
}

/**
 * @function   audio_getSDKData
 * @brief  获取SDK对讲应用下发的音频数据并解码
 * @param[in]  UINT8 **ppAudFrm 解码输出帧地址
 * @param[in]  UINT32 *pInLen 解码输出帧长度
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_getSDKData(UINT8 **ppAudFrm, UINT32 *pInLen)
{
    TALK_BACK_DN_CTRL_INFO_S *pstSdkTbCtrl = &gstAudioTskCtrl.stAudDnInfo.stSdkTbCtrl;
    PROCESS_INFO_S *pstProcInfo = &gstAudioTskCtrl.stAudDnInfo.procInfo;           /* 编解码接口 */
    MODULE_POOL_INFO_S *pstPoolInfo = &gstAudioTskCtrl.stAudDnInfo.stSdkTbCtrl.stDataPool;
    UINT8 *pSrcData = NULL;
    UINT32 u32SrcLen = 0;
    INT32 s32Ret = SAL_FAIL;

    if (NULL == ppAudFrm || NULL == pInLen)
    {
        AUD_LOGE("!!!GetPlayData input error \n");
        return SAL_FAIL;
    }

    pSrcData = pstPoolInfo->vAddr;

    SystemPrm_readTalkBackPool(0, pSrcData, &u32SrcLen);
    if (0 == u32SrcLen)
    {
        /* AUD_LOGE("read TalkBack Pool error\n"); */
        return SAL_FAIL; /* not data */
    }

#if 0
    FILE *pFile = fopen("/home/config/tb_audio_src.audio", "a+");
    if (pFile)
    {
        fwrite(pSrcData, 1, u32SrcLen, pFile);
        fclose(pFile);
    }

#endif

    pstProcInfo->stInterfaceInfo.enDstType = pstSdkTbCtrl->enType;
    pstProcInfo->stInterfaceInfo.pInputBuf = pSrcData;
    pstProcInfo->stInterfaceInfo.uiInputLen = u32SrcLen;
    pstProcInfo->stInterfaceInfo.reSample = pstSdkTbCtrl->bReSample;
    pstProcInfo->stInterfaceInfo.srcSampRate = pstSdkTbCtrl->SrcSample;
    s32Ret = audio_process(pstProcInfo); /*仅支持G711格式*/
    if (SAL_SOK == s32Ret)  /* only have 3 value returned , -1:failed; 0: successfully; 1: need more data. */
    {
        *ppAudFrm = pstProcInfo->stInterfaceInfo.pOutputBuf;
        *pInLen = pstProcInfo->stInterfaceInfo.uiOutputLen;
        pstProcInfo->stInterfaceInfo.uiOutputLen = 0;
        return SAL_SOK;
    }
    else
    {
        AUD_LOGE("==== DEC failed !!!! =====\n");
        return SAL_FAIL;
    }
}

/**
 * @function   audio_getTBData
 * @brief  获取ipc、回放解码的音频数据。
 * @param[in]  UINT8 **ppAudFrm 解码输出帧地址
 * @param[in]  UINT32 *pInLen 解码输出帧长度
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_getTBData(UINT8 **ppAudFrm, UINT32 *pInLen)
{
    TALK_BACK_DN_CTRL_INFO_S *pstAudTbCtrl = &gstAudioTskCtrl.stAudDnInfo.stAudTbCtrl;
    PROCESS_INFO_S *pstProcInfo = &gstAudioTskCtrl.stAudDnInfo.procInfo;             /* 编解码接口 */
    MODULE_POOL_INFO_S *pstPoolInfo = &gstAudioTskCtrl.stAudDnInfo.stAudTbCtrl.stDataPool;
    UINT8 *pSrcData = NULL;
    UINT32 u32SrcLen = 0;
    INT32 s32Ret = SAL_FAIL;

    if (NULL == ppAudFrm || NULL == pInLen)
    {
        AUD_LOGE("!!!GetPlayData input error \n");
        return SAL_FAIL;
    }

    pthread_mutex_lock(&pstPoolInfo->AudDemux);
    s32Ret = audio_checkTbDataUpdate(pstPoolInfo, &pSrcData, &u32SrcLen);
    if (SAL_SOK == s32Ret)
    {
        pthread_mutex_unlock(&pstPoolInfo->AudDemux);
    }
    else
    {
        pthread_mutex_unlock(&pstPoolInfo->AudDemux); /* 没有数据 */
        return SAL_FAIL;
    }

    pstProcInfo->stInterfaceInfo.enDstType = pstAudTbCtrl->enType;
    pstProcInfo->stInterfaceInfo.pInputBuf = pSrcData; /* pSrcData; */
    pstProcInfo->stInterfaceInfo.uiInputLen = u32SrcLen;
    pstProcInfo->stInterfaceInfo.reSample = pstAudTbCtrl->bReSample;
    pstProcInfo->stInterfaceInfo.srcSampRate = pstAudTbCtrl->SrcSample;

    s32Ret = audio_process(pstProcInfo);
    if (SAL_SOK == s32Ret)  /* only have 3 value returned , -1:failed; 0: successfully; 1: need more data. */
    {
        *ppAudFrm = pstProcInfo->stInterfaceInfo.pOutputBuf;
        *pInLen = pstProcInfo->stInterfaceInfo.uiOutputLen;
        pstProcInfo->stInterfaceInfo.uiOutputLen = 0;
        /*SAL_INFO("get audio dec data len %d %d\n", u32SrcLen, *pInLen);*/
        return SAL_SOK;
    }
    else if(s32Ret == AUDIO_CODEC_NEED_MORE_DATA)
    {
        AUD_LOGD("==== DEC failed: need more data!!!! =====\n");
        return SAL_FAIL;
    }
    {
        AUD_LOGE("==== DEC failed !!!! =====\n");
        return SAL_FAIL;
    }
}

/**
 * @function   audio_getAoData
 * @brief  获取解码后数据
 * @param[in]  UINT8 **ppAudFrm 解码输出帧地址
 * @param[in]  UINT32 *pInLen 解码输出帧长度
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_getAoData(UINT8 **ppAudFrm, UINT32 *pInLen)
{
    INT32 s32Ret = SAL_SOK;
    MOUDLE_STATE_E enState = MOUDLE_IDLE;

    if (NULL == ppAudFrm || NULL == pInLen)
    {
        AUD_LOGE("!!!GetPlayData input error \n");
        return SAL_FAIL;
    }

    enState = audio_getPlayState();
    /* AUD_LOGI("===== enState:%d =====\n",enState); */
    switch (enState)
    {
        case MOUDLE_IDLE:
        {
            /* do nothing */
            s32Ret = SAL_FAIL;    /* 返回SAL_FAIL，上层将会去播放静音文件。*/
            break;
        }
        case MOUDLE_PLAYBACK:
        {
            s32Ret = audio_getPlayData(ppAudFrm, pInLen);
            break;
        }
        case MOUDLE_SDKTB:/*对讲*/
        {
            s32Ret = audio_getSDKData(ppAudFrm, pInLen);
            break;
        }
        case MOUDLE_AUDTB: /*ipc、play*/
        {
            s32Ret = audio_getTBData(ppAudFrm, pInLen);
            break;
        }
        default:
        {
            s32Ret = SAL_FAIL;
            break;
        }
    }

    return s32Ret;
}

/**
 * @function   audio_tsk_playDec
 * @brief 播放编码后的音频
 * @param[in]  void *pPrm 无效参数
 * @param[out] None
 * @return     void
 */
static void *audio_tsk_playDec(void *pPrm)
{
    UINT8 *pDstData = NULL;
    UINT32 u32DstLen = 0;
    INT32 s32Ret = SAL_SOK;
    UINT8 mute[WAV_FRAME_LEN] = {0x0};
    u32DstLen = 0;
    AUDIO_PLAY_PARAM *pstAudioPlayParam = NULL;
    pstAudioPlayParam = (AUDIO_PLAY_PARAM *)pPrm;
	AUD_LOGE("audio_tsk data = %p len = %d\n",pDstData,u32DstLen);
while(pstAudioPlayParam->audioDataLen > 0)
{
    s32Ret = audio_getAoData(&pDstData, &u32DstLen);
    if (SAL_SOK != s32Ret && u32DstLen == 0)
    {
        pDstData = &mute[0];
        u32DstLen = WAV_FRAME_LEN;
        AUD_LOGE("audio_tsk_GetFrame error !!!\n");
		break;
    }
    s32Ret = audio_drv_aoSendFrame(pDstData , u32DstLen, &gstAudioTskCtrl.stAudDnInfo.stAoBufPrm); /* play actual audio data. */
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE("audio_drv_aoSendFrame error !!!\n");
    }
    pstAudioPlayParam->audioDataLen = pstAudioPlayParam->audioDataLen - u32DstLen;
}
    return NULL;
}


/**
 * @function   audio_tsk_playSound
 * @brief 只进行播放音频
 * @param[in]  void *pData 需要播放的数据参数 每次只播放640字节
 * @param[out] None
 * @return     INT32
 */
static INT32 audio_tsk_playSound(UINT32 u32Len, void *pData)
{
    UINT8 *pDstData = NULL;
    INT32 s32Ret = SAL_SOK;
    UINT32 u32playLen = 0;
    SAL_SET_THR_NAME();
    pDstData = pData;

    while(1)
    {
    s32Ret = audio_drv_aoSendFrame(pDstData, 640, &gstAudioTskCtrl.stAudDnInfo.stAoBufPrm); /* play actual audio data. */
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE("audio_drv_aoSendFrame error !!!\n");
    }
    u32playLen = u32playLen + 640;
    pDstData = pDstData + 640;
    if((u32Len - u32playLen)<640)
        {
            s32Ret = audio_drv_aoSendFrame(pDstData, (u32Len - u32playLen), &gstAudioTskCtrl.stAudDnInfo.stAoBufPrm); /* play actual audio data. */
            if (SAL_SOK != s32Ret)
            {
                AUD_LOGE("audio_drv_aoSendFrame error !!!\n");
            }
            break;
        }
    }
    return SAL_SOK;
}

/**
 * @function   audio_tsk_InputEnc
 * @brief 		开启音频输入录音并编码
 * @param[in]  UINT32 u32Chn 通道号，无效
 * @param[in]  void *pData 音频信号，输入
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_InputEnc(UINT32 u32Chn,  void *pData)
{
    UINT8 *pDstData = NULL;
    UINT32 u32DstLen = 0;
    INT32 s32Ret = SAL_FAIL;
	AUDIO_PLAY_PARAM *pstAudioPlayParam = NULL;
    TALK_BACK_UP_CTRL_INFO_S *pstAudTbCtrl = &gstAudioTskCtrl.stAudUpInfo.stAudTbCtrl;

    /* 校验参数是否为空 */
    if (NULL == pData)
    {
        AUD_LOGE("pData is Null\n");
        return SAL_FAIL;
    }

    pstAudioPlayParam = (AUDIO_PLAY_PARAM *)pData;

    if ((NULL == pstAudioPlayParam->audioBuf) || (0 == pstAudioPlayParam->audioDataLen))
    {
        AUD_LOGE("audioBuf = NULL or audioDataLen = 0\n");
        return SAL_FAIL;
    }


	pstAudTbCtrl->bEnable = SAL_FAIL;
	pstAudTbCtrl->enType = pstAudioPlayParam->res[0];
	pstAudTbCtrl->bEnable = SAL_TRUE;
	s32Ret = audio_tsk_encStart(&pDstData, &u32DstLen,pstAudioPlayParam);
	if(s32Ret == SAL_FAIL)
	{
		return SAL_FAIL;
	}
	FILE *pFileDec = fopen("./ai/enc_G711.pcm", "a+");

	if (pFileDec)
	{
		fwrite(pDstData, 1, u32DstLen, pFileDec);
		fclose(pFileDec);
	}
	 
    return SAL_SOK;
}

/**
 * @function   audio_tsk_decPreviewStart
 * @brief 开启音频解码播放
 * @param[in]  UINT32 u32VdecChn 解码通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_decPreviewStart(UINT32 u32VdecChn)
{
    TALK_BACK_DN_CTRL_INFO_S *pstAudTbCtrl = &gstAudioTskCtrl.stAudDnInfo.stAudTbCtrl;

    gu32AudioEnableChn |= 1 << u32VdecChn;
    pstAudTbCtrl->bEnable = SAL_TRUE;

    return SAL_SOK;
}
 /**
 * @function   audio_tsk_playThread
 * @brief 播放音频线程
 * @param[in]  void *pPrm 无效参数
 * @param[out] None
 * @return     void
 **/
static void *audio_tsk_playThread(void *pPrm)
{
    PROCESS_INFO_S *pstProcInfo = &gstAudioTskCtrl.stAudDnInfo.procInfo;  /* 编解码接口 */
    AUDIO_INTERFACE_INFO_S *pstInterfaceInfo = &pstProcInfo->stInterfaceInfo;
    RESAMPLE_INFO_S *pstResamplePrm = &pstProcInfo->stResamplePrm;
    AUDIO_INTERFACE_INFO_S stTmpInterFace = {0};
    UINT8 *pDstData = NULL;
    UINT32 u32DstLen = 0;
    INT32 u32LastSampleRate = 0;
    INT32 s32Ret = SAL_SOK;
    SAL_AudioFrameBuf *pstAudFrameParam = audio_hal_getDnLinePram();

    UINT8 mute[WAV_FRAME_LEN] = {0x0};

    SAL_SET_THR_NAME();
    while (1)
    {
    #if 1
        u32DstLen = 0;
        s32Ret = audio_getAoData(&pDstData, &u32DstLen);
        if (SAL_SOK != s32Ret || u32DstLen == 0)
        {
            /* usleep(5*1000); */
            pDstData = &mute[0];
            u32DstLen = WAV_FRAME_LEN;
            usleep(10 * 1000);
            continue;
        }

    #else
        SAL_AudioFrameBuf *pstAudFrmBuf = NULL;
        s32Ret = audio_hal_aiGetFrame(&pstAudFrmBuf);
        if (HI_SUCCESS != s32Ret)
        {
            AUD_LOGD("========== Get audio frame failed =======\n");
            continue;
        }

        pDstData = (UINT8 *)pstAudFrmBuf->virAddr[0];
        u32DstLen = pstAudFrmBuf->bufLen;
    #endif
	#if 0
        FILE *pFileDec = fopen("/home/config/dec_audio_raw.pcm", "a+");
        if (pFileDec)
        {
            fwrite(pDstData, 1, u32DstLen, pFileDec);
            fclose(pFileDec);
        }

	#endif

        if (pstInterfaceInfo->reSample)
        {
            u32LastSampleRate = pstInterfaceInfo->srcSampRate;

            if (u32LastSampleRate != pstResamplePrm->u32SrcRate && pstResamplePrm->pResampleHandle)
            {
                s32Ret = audioResample_drv_destroy(&pstResamplePrm->pResampleHandle);
                if (SAL_SOK != s32Ret)
                {
                    SAL_ERROR("audio resample destroy fail\n");
                    continue;
                }
            }

            if (pstResamplePrm->pResampleHandle == NULL)
            {
                pstResamplePrm->u32DesRate = pstAudFrameParam->frameParam.sampleRate;
                pstResamplePrm->u32SrcRate = u32LastSampleRate;
                s32Ret = audioResample_drv_create(&pstResamplePrm->pResampleHandle, pstResamplePrm);
                if (SAL_SOK != s32Ret)
                {
                    SAL_ERROR("audio resample create fail\n");
                    continue;
                }
            }

            stTmpInterFace = *pstInterfaceInfo;
            stTmpInterFace.pInputBuf = pDstData; /*input buf == out put buf*/
            stTmpInterFace.uiInputLen = u32DstLen;

            s32Ret = audioResample_drv_Process(pstResamplePrm->pResampleHandle, (void *)&stTmpInterFace);
            if (SAL_SOK == s32Ret)
            {
                pDstData = stTmpInterFace.pOutputBuf;
                u32DstLen = stTmpInterFace.uiOutputLen;

	#if 0
                FILE *pFileResample = fopen("/home/config/dec_audio_resample.pcm", "a+");
                if (pFileResample)
                {
                    fwrite(pDstData, 1, u32DstLen, pFileResample);
                    fclose(pFileResample);
                }

	#endif
            }
            else if (SAL_EBUSY == s32Ret) /* 重采样输入数据量小于一次重采样处理大小，需等待新数据 */
            {
                continue;
            }
            else
            {
                AUD_LOGE("===== audioResample_drv_Processerr with %#x ====\n", s32Ret);
                continue;
            }
        }

	#if 0
        FILE *pFileAo = fopen("/home/config/audio_ao.pcm", "a+");
        if (pFileAo)
        {
            fwrite(pDstData, 1, u32DstLen, pFileAo);
            fclose(pFileAo);
        }

	#endif
        s32Ret = audio_drv_aoSendFrame(pDstData, u32DstLen, &gstAudioTskCtrl.stAudDnInfo.stAoBufPrm); /* play actual audio data. */
        if (SAL_SOK != s32Ret)
        {
            AUD_LOGE("audio_drv_aoSendFrame error !!!\n");
        }
    }

    return NULL;
}

/**
 * @function   audio_tsk_soundStart
 * @brief 设置播放文件语音参数/开始播放
 * @param[in]  UINT32 u32Chn 通道号，无效
 * @param[in]  void *pBuf 语音播报参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */

INT32 audio_tsk_soundStart(UINT32 u32Chn)
{
    audio_tsk_playSave(NULL);
    return SAL_SOK;
}


/**
 * @function   audio_tsk_soundStartTTS
 * @brief 设置播放TTS
 * @param[in]  UINT32 u32Chn 通道号，无效
 * @param[in]  void *pBuf 语音播报参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */

INT32 audio_tsk_soundStartTTS(UINT32 u32Len, void *pData)
{
    
	audio_tsk_playSound(u32Len,pData);
 
    return SAL_SOK;
}

/**
 * @function   audio_tsk_encStart
 * @brief 设置开始编码
 * @param[in]  AUDIO_PLAY_PARAM *pstAudioPlayParam 需要编码的音频信息
 * @param[in]  UINT32 *pInLen 音频编码长度（640）
 * @param[out] UINT8 **ppAudFrm 编码后的输出数据
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_encStart(UINT8 **ppAudFrm, UINT32 *pInLen, AUDIO_PLAY_PARAM *pstAudioPlayParam)
{	
    TALK_BACK_UP_CTRL_INFO_S *pstAudTbCtrl = &gstAudioTskCtrl.stAudUpInfo.stAudTbCtrl; 
    PROCESS_INFO_S *pstAudTbProcInfo = &gstAudioTskCtrl.stAudUpInfo.stAudTbProcInfo;
	AUDIO_INTERFACE_INFO_S *pstInterfaceInfo = &pstAudTbProcInfo->stInterfaceInfo; 

    INT32 s32Ret = SAL_FAIL;  
	
    if (NULL == ppAudFrm || NULL == pInLen)
    {
        AUD_LOGE("!!!GetPlayData input error \n");
        return SAL_FAIL;
    }		

    /*******************当前只支持RAW格式************************/
    if (RAW == pstAudTbCtrl->enType) /* wav 这种无编码的裸数据 */
    {
        *ppAudFrm = (UINT8 *)pstAudioPlayParam->audioBuf;
        *pInLen = WAV_FRAME_LEN;
        pstAudTbProcInfo->stInterfaceInfo.reSample = SAL_FALSE;
        return SAL_SOK;
    }
    else
    {
        pstInterfaceInfo->enDstType = pstAudTbCtrl->enType;
        pstInterfaceInfo->pInputBuf = (UINT8 *)pstAudioPlayParam->audioBuf;
        pstInterfaceInfo->uiInputLen = WAV_FRAME_LEN; /* 160;//WAV_FRAME_LEN; */
        pstInterfaceInfo->reSample = SAL_FALSE;

        s32Ret = audio_process(pstAudTbProcInfo);
        if (SAL_SOK == s32Ret)  /* only have 3 value returned , -1:failed; 0: successfully; 1: need more data. */
        {
            *ppAudFrm = pstInterfaceInfo->pOutputBuf;
            *pInLen = pstInterfaceInfo->uiOutputLen;
            pstInterfaceInfo->uiOutputLen = 0;

            return SAL_SOK;
        }
        else
        {
            AUD_LOGE("==== DEC failed !!!! =====\n");
            return SAL_FAIL;
        }
    }
}


/**
 * @function   audio_tsk_decPreviewStop
 * @brief 停止音频解码播放
 * @param[in]  UINT32 u32VdecChn 解码通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_decPreviewStop(UINT32 u32VdecChn)
{
    TALK_BACK_DN_CTRL_INFO_S *pstAudTbCtrl = &gstAudioTskCtrl.stAudDnInfo.stAudTbCtrl;
    MODULE_POOL_INFO_S *pstPoolInfo = &pstAudTbCtrl->stDataPool;

    gu32AudioEnableChn &= ~(1 << u32VdecChn);
    
    if(gu32AudioEnableChn == 0)
    {
        pstAudTbCtrl->bEnable = SAL_FALSE;

        pthread_mutex_lock(&pstPoolInfo->AudDemux);
        pstPoolInfo->wIdx = 0;
        pstPoolInfo->rIdx = 0;
        pstPoolInfo->u32CurDataLen = 0;
        pthread_mutex_unlock(&pstPoolInfo->AudDemux);
    }

    return SAL_SOK;
}

/**
 * @function   audio_tsk_decPreviewWriteData
 * @brief IPC/回放 解析到的音频数据写入对应的datapool
 * @param[in]  UINT8 *pAudFrm 音频数据
 * @param[in]  UINT32 u32InLen 音频数据长度
 * @param[in]  UINT32 u32AudType 音频类型
 * @param[in]  UINT8 *pExtAudFrm 音频扩展头
 * @param[in]  UINT32 u32InExtLen 音频头长度
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_decPreviewWriteData(UINT8 *pAudFrm, UINT32 u32InLen, UINT32 u32AudType, UINT8 *pExtAudFrm, UINT32 u32InExtLen)
{
    TALK_BACK_DN_CTRL_INFO_S *pstAudTbCtrl = &gstAudioTskCtrl.stAudDnInfo.stAudTbCtrl;
    MODULE_POOL_INFO_S *pstPoolInfo = &pstAudTbCtrl->stDataPool;
    UINT32 u32Idx = 0;
    UINT32 u32SrcLen = 0;
    UINT32 u32DataFree = 0;
    UINT32 u32BuffOffset = 0;
    UINT32 u32Spare = 0;
    SAL_AudioFrameBuf *pstAudFrameParam = audio_hal_getDnLinePram();

    if (NULL == pAudFrm || 0 == u32InLen)
    {
        AUD_LOGE("!!!GetPlayData input error \n");
        return SAL_FAIL;
    }

    pthread_mutex_lock(&pstPoolInfo->AudDemux);

    if ((STREAM_TYPE_AUDIO_G711U == u32AudType) || (AUDIO_G711_U == u32AudType))
    {
        AUD_LOGD("G711_MU,len %d\n", pstPoolInfo->u32BufLen);
        pstAudTbCtrl->enType = G711_MU;
    }
    else if ((STREAM_TYPE_AUDIO_G711A == u32AudType) || (AUDIO_G711_A == u32AudType))
    {
        AUD_LOGD("G711_A\n");
        pstAudTbCtrl->enType = G711_A;
    }
    else
    {

        pstAudTbCtrl->enType = AAC;
        /* indx = ((unsigned int) pExtAudFrm[2] & 0x3c) >> 2; */
        AUD_LOGD("u32AudType %#x\n", u32AudType);
    }

    if (SAL_FALSE == pstAudTbCtrl->bEnable)
    {
        /*SAL_WARN("audio tb not open \n");*/
        pthread_mutex_unlock(&pstPoolInfo->AudDemux);
        return SAL_SOK;
    }

#if 0
    FILE *pFile = fopen("/home/config/dec_audio_src.audio", "a+");
    if (pFile)
    {
        fwrite(pAudFrm, 1, u32InLen, pFile);
        fclose(pFile);
    }

#endif

    if (u32InLen > pstPoolInfo->u32BufLen)
    {
        u32SrcLen = pstPoolInfo->u32BufLen;
        SAL_ERROR("u32InLen %d u32InExtLen %d, BufLen %d\n", u32InLen, u32InExtLen, pstPoolInfo->u32BufLen);
    }
    else
    {
        u32SrcLen = u32InLen;
    }

    u32Spare = pstPoolInfo->wIdx < pstPoolInfo->rIdx ? (pstPoolInfo->rIdx - pstPoolInfo->wIdx) \
               : (pstPoolInfo->u32BufLen - pstPoolInfo->rIdx + pstPoolInfo->wIdx);
    if (u32InLen >= u32Spare)
    {
        SAL_WARN("no spare %d %d, data overflow!!\n", u32InLen, u32Spare);
    }

    pstAudTbCtrl->bReSample = 0;
    pstAudTbCtrl->SrcSample = SAL_AUDIO_SAMPLE_RATE_8000;
    u32DataFree = u32SrcLen;
    if (0 < u32DataFree)
    {
        if (pstAudTbCtrl->enType == AAC)
        {
            if ((NULL != pExtAudFrm) && (0 < u32InExtLen))
            {
                if (pstPoolInfo->wIdx + u32InExtLen > pstPoolInfo->u32BufLen)
                {
                    memcpy(pstPoolInfo->vAddr + pstPoolInfo->wIdx, pExtAudFrm, pstPoolInfo->u32BufLen - pstPoolInfo->wIdx);
                    u32InExtLen = u32InExtLen - (pstPoolInfo->u32BufLen - pstPoolInfo->wIdx);
                    u32BuffOffset = pstPoolInfo->u32BufLen - pstPoolInfo->wIdx;
                    pstPoolInfo->wIdx = 0;
                }

                /*inExtLen必然非0*/
                memcpy(pstPoolInfo->vAddr + pstPoolInfo->wIdx, pExtAudFrm + u32BuffOffset, u32InExtLen);
                pstPoolInfo->wIdx += u32InExtLen;

                u32DataFree -= 4;
                u32BuffOffset = 0;
                if (pstPoolInfo->wIdx + u32DataFree > pstPoolInfo->u32BufLen)
                {
                    memcpy(pstPoolInfo->vAddr + pstPoolInfo->wIdx, pAudFrm + 4, pstPoolInfo->u32BufLen - pstPoolInfo->wIdx);
                    u32DataFree = u32DataFree - (pstPoolInfo->u32BufLen - pstPoolInfo->wIdx);
                    u32BuffOffset = pstPoolInfo->u32BufLen - pstPoolInfo->wIdx;
                    pstPoolInfo->wIdx = 0;
                }

                /*dataFree必然非0*/
                memcpy(pstPoolInfo->vAddr + pstPoolInfo->wIdx, pAudFrm + 4 + u32BuffOffset, u32DataFree);
                pstPoolInfo->wIdx += u32DataFree;

                u32Idx = ((unsigned int) pExtAudFrm[2] & 0x3c) >> 2;
            }
            else
            {
                if (pstPoolInfo->wIdx + u32DataFree > pstPoolInfo->u32BufLen)
                {
                    memcpy(pstPoolInfo->vAddr + pstPoolInfo->wIdx, pAudFrm, pstPoolInfo->u32BufLen - pstPoolInfo->wIdx);
                    u32DataFree = u32DataFree - (pstPoolInfo->u32BufLen - pstPoolInfo->wIdx);
                    u32BuffOffset = pstPoolInfo->u32BufLen - pstPoolInfo->wIdx;
                    pstPoolInfo->wIdx = 0;
                }

                memcpy(pstPoolInfo->vAddr + pstPoolInfo->wIdx, pAudFrm + u32BuffOffset, u32DataFree);
                pstPoolInfo->wIdx += u32DataFree;

                u32Idx = ((unsigned int) pAudFrm[2] & 0x3c) >> 2;
                if ((pAudFrm[0] != 0xff) || (((pAudFrm[1] >> 4) & 0xff) != 0xf))
                {
                    AUD_LOGE("aac sync fail %d %d\n", pAudFrm[0], pAudFrm[1]);
                }
            }

            if (u32Idx >= 16)
            {
                AUD_LOGE("u32Idx overflow %d, 0x%x\n", u32Idx, pAudFrm[2]);
            }
            else
            {
                if (u32Idx >= 13)
                {
                    AUD_LOGE("u32Idx inv %d, 0x%x\n", u32Idx, pAudFrm[2]);
                }

                if (pstAudFrameParam->frameParam.sampleRate != aacAdtsHeaderSamplingFrequency[u32Idx])
                {
                    pstAudTbCtrl->bReSample = 1;
                    pstAudTbCtrl->SrcSample = aacAdtsHeaderSamplingFrequency[u32Idx];
                    /* AUD_LOGI("Aud TalkBack bReSample is 1,SrcSample %d,u32Idx %d\n",pstAudTbCtrl->SrcSample,u32Idx); */
                }
            }
        }
        else
        {
            if (pstPoolInfo->wIdx + u32DataFree > pstPoolInfo->u32BufLen)
            {
                memcpy(pstPoolInfo->vAddr + pstPoolInfo->wIdx, pAudFrm, pstPoolInfo->u32BufLen - pstPoolInfo->wIdx);
                u32DataFree = u32DataFree - (pstPoolInfo->u32BufLen - pstPoolInfo->wIdx);
                u32BuffOffset = pstPoolInfo->u32BufLen - pstPoolInfo->wIdx;
                pstPoolInfo->wIdx = 0;
            }

            memcpy(pstPoolInfo->vAddr + pstPoolInfo->wIdx, pAudFrm, u32DataFree);
            pstPoolInfo->wIdx += u32DataFree;
            pstAudTbCtrl->SrcSample = SAL_AUDIO_SAMPLE_RATE_8000;

            /* 源采样率和目标采样率不一样，需要重采样 */
            if(pstAudFrameParam->frameParam.sampleRate != pstAudTbCtrl->SrcSample)
            {
                pstAudTbCtrl->bReSample = 1;
            }
        }
    }
    else
    {
        pthread_mutex_unlock(&pstPoolInfo->AudDemux); /* 没有数据 */
        SAL_ERROR("dec lost audio frm\n");
        return SAL_FAIL;
    }

    pstPoolInfo->u32CurDataLen = u32SrcLen;

    pthread_mutex_unlock(&pstPoolInfo->AudDemux);
    return SAL_SOK;
}
/**
 * @function   audio_tsk_playStartPCM
 * @brief 设置播放语音参数/开始播放只播放PCM格式文件
 * @param[in]  UINT32 u32Chn 通道号，无效
 * @param[in]  void *pBuf 语音播报参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_playStartPCM(UINT32 u32Chn, void *pBuf)
{
    AUDIO_PLAY_PARAM *pstAudioPlayParam = NULL;
    DSPINITPARA *pstDspInitParm = SystemPrm_getDspInitPara();
    AUDIO_PLAY_CTRL_INFO_S *pstPlayBackCtrl = &gstAudioTskCtrl.stAudDnInfo.stAudioPlayCtrl;

    /* 校验参数是否为空 */
    if (NULL == pBuf)
    {
        AUD_LOGE("pBuf is Null\n");

        return SAL_FAIL;
    }

    pstAudioPlayParam = (AUDIO_PLAY_PARAM *)pBuf;

    if ((NULL == pstAudioPlayParam->audioBuf) || (0 == pstAudioPlayParam->audioDataLen))
    {
        AUD_LOGE("audioBuf = NULL or audioDataLen = 0\n");

        return SAL_FAIL;
    }
    pthread_mutex_lock(&pstPlayBackCtrl->muOnPlayAudio);
    pstDspInitParm->stAudioPlayBcakPrm.bAudIsPlaying = 1;
    pthread_mutex_unlock(&pstPlayBackCtrl->muOnPlayAudio);
    audio_tsk_playSound(pstAudioPlayParam->audioDataLen,pstAudioPlayParam->audioBuf);
    return SAL_SOK;
}

/**
 * @function   audio_tsk_playStart
 * @brief 设置播放语音参数/开始播放
 * @param[in]  UINT32 u32Chn 通道号，无效
 * @param[in]  void *pBuf 语音播报参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_playStart(UINT32 u32Chn, void *pBuf)
{
    AUDIO_PLAY_PARAM *pstAudioPlayParam = NULL;
    AUDIO_PLAY_CTRL_INFO_S *pstPlayBackCtrl = &gstAudioTskCtrl.stAudDnInfo.stAudioPlayCtrl;
    MODULE_POOL_INFO_S *pstPoolInfo = &pstPlayBackCtrl->stDataPool;
    UINT32 u32DefValue = 0x00;
    DSPINITPARA *pstDspInitParm = SystemPrm_getDspInitPara();

    /* 校验参数是否为空 */
    if (NULL == pBuf)
    {
        AUD_LOGE("pBuf is Null\n");

        return SAL_FAIL;
    }

    pstAudioPlayParam = (AUDIO_PLAY_PARAM *)pBuf;

    if ((NULL == pstAudioPlayParam->audioBuf) || (0 == pstAudioPlayParam->audioDataLen))
    {
        AUD_LOGE("audioBuf = NULL or audioDataLen = 0\n");

        return SAL_FAIL;
    }
	AUD_LOGE("audioBuf = %p or audioDataLen = %d\n",pstAudioPlayParam->audioBuf,pstAudioPlayParam->audioDataLen);
/* 音频数据不能够大于DSP最大缓冲区, 大于缓存区需要多次拷贝 */
    while (pstAudioPlayParam->audioDataLen > pstPoolInfo->u32BufLen)
    {

        /* 播放繁忙, 铃音文件优先级控制，铃音级别低不可以打断正在播放的高级别铃音*/
        if (pstAudioPlayParam->stPlayType < pstPlayBackCtrl->enAudioPlayType)
        {
            AUD_LOGE("CHAN_UNREADY! Cur displayType %d, DSP %d \n",
                     pstAudioPlayParam->stPlayType,
                     pstPlayBackCtrl->enAudioPlayType);
            return SAL_SOK;
        }
            pthread_mutex_lock(&pstPlayBackCtrl->muOnPlayAudio);

            pstPlayBackCtrl->stopHand = SAL_TRUE;
            pstPlayBackCtrl->bEnable = SAL_FALSE;

            pstPlayBackCtrl->dataLen = pstPoolInfo->u32BufLen;
            pstPlayBackCtrl->startTime = SAL_getTimeOfJiffies();
            pstPlayBackCtrl->playTime = pstAudioPlayParam->uiPlayTime;
            pstPlayBackCtrl->u32LoopCnt = 0;

            pstPlayBackCtrl->frmNum = pstPlayBackCtrl->dataLen / WAV_FRAME_LEN; /*不管什么编码格式，每次都读取640个字节送去解码 */
            pstPlayBackCtrl->enType = pstAudioPlayParam->res[0];   /* 获取铃声的编码格式 */
            u32DefValue = (pstPlayBackCtrl->enType == G711_MU || pstPlayBackCtrl->enType == G711_A) ? 0xFF : 0x00;
            memset(pstPoolInfo->vAddr, u32DefValue, pstPoolInfo->u32BufLen);
            memcpy(pstPoolInfo->vAddr, pstAudioPlayParam->audioBuf, pstAudioPlayParam->audioDataLen);       /* 拷贝音频数据 */
            pstPlayBackCtrl->enAudioPlayType = pstAudioPlayParam->stPlayType;     
            AUD_LOGI("Play: loopCnt %d; displayType=%d,PlayAoChan=%d,dataLen=%d,frmNum=%d,playTime=%d,startTime=%lld\n",
                     pstPlayBackCtrl->u32LoopCnt,
                     pstPlayBackCtrl->enAudioPlayType,
                     pstPlayBackCtrl->aoChan,
                     pstPlayBackCtrl->dataLen,
                     pstPlayBackCtrl->frmNum,
                     pstPlayBackCtrl->playTime,
                     pstPlayBackCtrl->startTime);

            pstPlayBackCtrl->stopHand = SAL_FALSE;
            pstPlayBackCtrl->bEnable = SAL_TRUE;
            pstDspInitParm->stAudioPlayBcakPrm.bAudIsPlaying = 1;
            pstAudioPlayParam->audioDataLen= pstAudioPlayParam->audioDataLen - pstPoolInfo->u32BufLen;
            /* pthread_cond_signal(&pstPlayBack->pbcond); */
            pthread_mutex_unlock(&pstPlayBackCtrl->muOnPlayAudio);
            audio_tsk_playDec((void*)pstAudioPlayParam);
    }
        /* 播放繁忙, 铃音文件优先级控制，铃音级别低不可以打断正在播放的高级别铃音*/
        if (pstAudioPlayParam->stPlayType < pstPlayBackCtrl->enAudioPlayType)
        {
            AUD_LOGE("CHAN_UNREADY! Cur displayType %d, DSP %d \n",
            pstAudioPlayParam->stPlayType,
            pstPlayBackCtrl->enAudioPlayType);
            return SAL_SOK;
        }
             pthread_mutex_lock(&pstPlayBackCtrl->muOnPlayAudio);
        
            pstPlayBackCtrl->stopHand = SAL_TRUE;
            pstPlayBackCtrl->bEnable = SAL_FALSE;
        
            pstPlayBackCtrl->dataLen = pstPoolInfo->u32BufLen;
            pstPlayBackCtrl->startTime = SAL_getTimeOfJiffies();
            pstPlayBackCtrl->playTime = pstAudioPlayParam->uiPlayTime;
            pstPlayBackCtrl->u32LoopCnt = 0;
        
            /*不管什么编码格式，每次都读取640个字节送去解码 */
            pstPlayBackCtrl->frmNum = pstPlayBackCtrl->dataLen / WAV_FRAME_LEN;
            pstPlayBackCtrl->enType = pstAudioPlayParam->res[0];
            u32DefValue = (pstPlayBackCtrl->enType == G711_MU || pstPlayBackCtrl->enType == G711_A) ? 0xFF : 0x00;
            memset(pstPoolInfo->vAddr, u32DefValue, pstPoolInfo->u32BufLen);
            memcpy(pstPoolInfo->vAddr, pstAudioPlayParam->audioBuf, pstAudioPlayParam->audioDataLen);
            pstPlayBackCtrl->enAudioPlayType = pstAudioPlayParam->stPlayType;
            /* pstAencDecInfo->ProcessInfo[DEC].stInterfaceInfo.bClrTmp = 1; */
            AUD_LOGE("Play: loopCnt %d; displayType=%d,PlayAoChan=%d,dataLen=%d,frmNum=%d,playTime=%d,startTime=%lld\n",
                pstPlayBackCtrl->u32LoopCnt,
                pstPlayBackCtrl->enAudioPlayType,
                pstPlayBackCtrl->aoChan,
                pstPlayBackCtrl->dataLen,
                pstPlayBackCtrl->frmNum,
                pstPlayBackCtrl->playTime,
                pstPlayBackCtrl->startTime);
        
            pstPlayBackCtrl->stopHand = SAL_FALSE;
            pstPlayBackCtrl->bEnable = SAL_TRUE;
            pstDspInitParm->stAudioPlayBcakPrm.bAudIsPlaying = 1;
            pstAudioPlayParam->audioDataLen= 0;
            /* pthread_cond_signal(&pstPlayBack->pbcond); */
            pthread_mutex_unlock(&pstPlayBackCtrl->muOnPlayAudio);

        audio_tsk_playDec(pBuf);

    return SAL_SOK;
}


/**
 * @function   audio_tsk_playStop
 * @brief 停止语音播放
 * @param[in]  UINT32 u32Chn 通道号，无效
 * @param[in]  void *prm 无效
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_playStop(UINT32 u32Chn, void *prm)
{
    /* AUD_LOGW("=========================== Audio_drvStopPlayAudio =====================\n"); */
    AUDIO_PLAY_CTRL_INFO_S *pstPlayBackCtrl = &gstAudioTskCtrl.stAudDnInfo.stAudioPlayCtrl;

    pthread_mutex_lock(&pstPlayBackCtrl->muOnPlayAudio);
    pstPlayBackCtrl->stopHand = SAL_TRUE;
    pstPlayBackCtrl->bEnable = SAL_FALSE;
    pthread_mutex_unlock(&pstPlayBackCtrl->muOnPlayAudio);

    return SAL_SOK;
}

/**
 * @function   audio_tsk_setTalkback
 * @brief  开始/结束  SDK 对讲
 * @param[in]  UINT32 u32Chn 通道号
 * @param[in]  void *prm 对讲参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_setTalkback(UINT32 u32Chn, void *prm)
{
    DSPINITPARA *pstDspInitParm = SystemPrm_getDspInitPara();
    TALK_BACK_DN_CTRL_INFO_S *pstSdkTbDnCtrl = &gstAudioTskCtrl.stAudDnInfo.stSdkTbCtrl;      /* 下行的sdk对讲控制结构体 */
    TALK_BACK_UP_CTRL_INFO_S *pstSdkTbUpCtrl = &gstAudioTskCtrl.stAudUpInfo.stSdkTbCtrl;      /* 上行的sdk对讲控制结构体 */
    TALK_BACK_PARAM *pstTalkBackParam = &pstDspInitParm->stStreamInfo.stTalkBackParam;
    DSP_AUDIO_PARAM *pstAudioPrm = NULL;
    CAPB_AUDIO *pstCapbAudio = NULL;
    SAL_AudioFrameBuf *pstUpAudFrameParam = audio_hal_getUpLinePram();
    SAL_AudioFrameBuf *pstDnAudFrameParam = audio_hal_getDnLinePram();

    INT32 bEnable = SAL_FALSE;
    INT32 s32Ret = SAL_FALSE;

    /* 校验参数是否为空 */
    if (NULL == prm)
    {
        AUD_LOGE("pBuf is Null\n");
        return SAL_FAIL;
    }

    pstCapbAudio = capb_get_audio();
    if (pstCapbAudio == NULL)
    {
        AUD_LOGE("null err\n");
        return SAL_FAIL;
    }

    pstAudioPrm = (DSP_AUDIO_PARAM *)prm;
    bEnable = pstAudioPrm->bEnable;

    AUD_LOGI("SDK TalkBack bEnable: %d.talkBackType %d,talkBackSamplesPerSecond %d...\n", bEnable, pstTalkBackParam->talkBackType, pstTalkBackParam->talkBackSamplesPerSecond);

    pstSdkTbUpCtrl->bEnable = bEnable;                         /* up line enable. */
    pstSdkTbUpCtrl->enType = pstTalkBackParam->talkBackType;   /* sdk Tb enc type */
    pstSdkTbUpCtrl->bNeedPsPack = SAL_FALSE;
    pstSdkTbUpCtrl->bNeedRtpPack = SAL_FALSE;

    pstSdkTbDnCtrl->bEnable = bEnable;                         /* Dn line enable. */
    pstSdkTbDnCtrl->enType = pstTalkBackParam->talkBackType;   /* sdk Tb enc type */
    pstSdkTbDnCtrl->bReSample = 0;
    pstSdkTbDnCtrl->SrcSample = pstUpAudFrameParam->frameParam.sampleRate;

    if (pstTalkBackParam->talkBackSamplesPerSecond != pstDnAudFrameParam->frameParam.sampleRate)
    {
        pstSdkTbDnCtrl->bReSample = 1;
        pstSdkTbDnCtrl->SrcSample = pstTalkBackParam->talkBackSamplesPerSecond;
        AUD_LOGI("SDK TalkBack bReSample is 1\n");
    }

    s32Ret = audio_drv_setAudioTalkBack(pstCapbAudio->audiodevidx, prm);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("audio_drv_setAudioTalkBack err %d\n", s32Ret);
        return SAL_FAIL;
    }

    SAL_INFO("audio tb:type %d\n", pstSdkTbDnCtrl->enType);

    return SAL_SOK;
}

/**
 * @function   audio_tsk_setAiVolume
 * @brief   配置输入音量等级
 * @param[in]  UINT32 u32Chn 通道号
 * @param[in]  void *prm 音量参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_setAiVolume(UINT32 u32Chn, void *prm)
{
    INT32 s32Ret = 0;

    s32Ret = audio_drv_setAiVolume(u32Chn, prm);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("audio_drv_setAiVolume err %d\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_tsk_setAoVolume
 * @brief    配置输出音量等级
 * @param[in]  UINT32 u32Chn 通道号
 * @param[in]  void *prm 音量参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_setAoVolume(UINT32 u32Chn, void *prm)
{
    INT32 s32Ret = 0;

    s32Ret = audio_drv_setAoVolume(u32Chn, prm);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("Audio_drvSetAOVolume err %d\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_tsk_ttsStart
 * @brief    合成语音开始
 * @param[in]  UINT32 u32Chn 通道号
 * @param[in]  void *prm 初始化参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_ttsStart(UINT32 u32Chn, void *prm)
{
    if (SAL_SOK != audioTTS_drv_start(u32Chn, &gstAudioTskCtrl.pTtsHandle, prm))
    {
        SAL_ERROR("audioTTS_drv_start err !!!\n");
    }

    return SAL_SOK;
}

/**
 * @function   audio_tsk_ttsProcess
 * @brief    合成语音处理
 * @param[in]  UINT32 u32Chn 音频通道，无效
 * @param[in]  void *prm 输入数据
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_ttsProcess(UINT32 u32Chn, void *prm)
{
    STREAM_ELEMENT stStreamEle = {0};
    PUINT8 pTtsData = NULL;
    UINT32 u32DataSize = 0;

    if (SAL_SOK != audioTTS_drv_process(gstAudioTskCtrl.pTtsHandle, prm, &pTtsData, &u32DataSize))
    {
        SAL_ERROR("audioTTS_drv_process err !!!\n");
        return SAL_FAIL;
    }

    if ((NULL == pTtsData) || (0 == u32DataSize))
    {
        SAL_ERROR("audioTTS_drv_process err !!!\n");
    }

    memset(&stStreamEle, 0x00, sizeof(STREAM_ELEMENT));
    stStreamEle.type = STREAM_ELEMENT_TTS_HANDLE;
    stStreamEle.chan = 0;

    SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)pTtsData, u32DataSize);

    return SAL_SOK;
}

/**
 * @function   audio_tsk_ttsStop
 * @brief    合成语音关闭
 * @param[in]  UINT32 u32Chn 音频通道，无效
 * @param[in]  void *prm 输入参数，无效
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_ttsStop(UINT32 u32Chn, void *prm)
{
    if (SAL_SOK != audioTTS_drv_stop(&gstAudioTskCtrl.pTtsHandle))
    {
        SAL_ERROR("audioTTS_drv_stop err !!!\n");
    }

    return SAL_SOK;
}

/**
 * @function   audio_tsk_deInit
 * @brief    audio tsk层去初始化。
 * @param[in]  void
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_deInit()
{
    /* drv 层去初始化 */
    if (SAL_SOK != audio_drv_deInit())
    {
        SAL_ERROR(" !!!\n");

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_tsk_init
 * @brief    audio tsk层初始化资源。
 * @param[in]  void
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_init()
{
    memset(&gstAudioTskCtrl, 0, sizeof(gstAudioTskCtrl));

    /* DRV 层初始化 */
    if (SAL_SOK != audio_drv_init())
    {
        AUD_LOGE(" !!!\n");
        return SAL_FAIL;
    }

    /* 音频下行初始化 */
    if (SAL_SOK != audio_initDnLine())
    {
        AUD_LOGE("Audio_drvInitDnLineMem error !!!\n");
        return SAL_FAIL;
    }

    /* 音频上行初始化 */
    if (SAL_SOK != audio_initUpLine())
    {
        AUD_LOGE("audio_initUpLine !!!\n");
        return SAL_FAIL;
    }

    /* 创建语音播放的处理线程 */
    SAL_thrCreate(&gstAudioTskCtrl.pidOut, audio_tsk_playThread, SAL_THR_PRI_DEFAULT, 0, NULL);

    /* 创建语音采集的处理线程 */
    SAL_thrCreate(&gstAudioTskCtrl.pidIn, audio_tsk_encThread, SAL_THR_PRI_DEFAULT, 0, NULL);

    return SAL_SOK;
}

/**
 * @function   audio_tsk_setPrm
 * @brief    外部只需配置哪个设备(可见光还是红外光)的哪个通道(主码流还是子码流)需要声音数据。
 * @param[in]  UINT32 u32Dev 哪个设备(可见光还是红外光);
 * @param[in]  UINT32 u32Chn 主码流还是子码流.;
 * @param[in]  void *prm 编码参数;
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_setPrm(UINT32 u32Dev, UINT32 u32Chn, void *prm)
{
    ENCODER_PARAM *pstEncParam = (ENCODER_PARAM *)prm;
    DSPINITPARA *pstDspInitPara = SystemPrm_getDspInitPara();
    AUDIO_PARAM *pstAudioParam = &pstDspInitPara->stStreamInfo.stAudioParam;
    VIDEO_PARAM *pstVideoParam = &pstDspInitPara->stStreamInfo.stVideoParam;
    TALK_BACK_UP_CTRL_INFO_S *pstAudTbCtrl = &gstAudioTskCtrl.stAudUpInfo.stAudTbCtrl;
    CHN_INFO_S *pstChnInfo = &gstAudioTskCtrl.stAudUpInfo.stAudChnInfo[u32Dev];
    STREAM_PACKET_TYPE enPacketType = 0;

    /* pstTalkCtrlInfo->enType              = pstAudParam->encType; */

    AUD_LOGW("-------- u32Dev:%d, u32Chn:%d --------\n", u32Dev, u32Chn);

    /* ~{2bJTSC~} */
    pstAudioParam->encType = pstEncParam->audioType;
    if (MAIN_STREAM_CHN == u32Chn)
    {
        enPacketType = pstVideoParam->packetType;
    }
    else if (SUB_STREAM_CHN == u32Chn)
    {
        enPacketType = pstVideoParam->subPacketType;
    }
    else
    {
        enPacketType = pstVideoParam->ThirdPacketType;
    }

    pstAudTbCtrl->enType = pstAudioParam->encType;

    pstChnInfo->stStreamPackInfo[u32Chn].bNeedAud = (STREAM_AUDIO == pstEncParam->streamType    \
                                                     || STREAM_MULTI == pstEncParam->streamType) ? SAL_TRUE : SAL_FALSE;
    pstChnInfo->stStreamPackInfo[u32Chn].bNeedRtpPack = (enPacketType & RTP_STREAM) ? SAL_TRUE : SAL_FALSE;
    pstChnInfo->stStreamPackInfo[u32Chn].bNeedPsPack = (enPacketType & PS_STREAM) ? SAL_TRUE : SAL_FALSE;

    AUD_LOGI("=== pstChnInfo->stStreamPackInfo[%d].bNeedAud:%d audioType %d=====\n", u32Chn, pstChnInfo->stStreamPackInfo[u32Chn].bNeedAud, pstEncParam->audioType);

    return SAL_SOK;
}

