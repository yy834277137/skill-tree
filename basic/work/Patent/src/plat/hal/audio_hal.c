/**
 * @file   audio_hal.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---hal层接口封装
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#include "platform_hal.h"
#include "hal_inc_inter/audio_hal_inter.h"

#line __LINE__ "audio_hal.c"


/*****************************************************************************
                               宏定义
*****************************************************************************/

/*****************************************************************************
                            全局结构体
*****************************************************************************/

typedef struct
{
    /* 上行的音频信息*/
    SAL_AudioFrameBuf stUpLineHalInfo;

    /* 下行的音频信息*/
    SAL_AudioFrameBuf stDnLineHalInfo;

} AIO_HAL_INFO;

/*****************************************************************************
                               全局变量
*****************************************************************************/
static AIO_HAL_INFO g_stAudioHalPrm = {0};

static AUDIO_PLAT_OPS_S g_stAudioHalOps = {0};


/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   audio_initPram
 * @brief    音频模块参数初始化
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
void audio_initPram(void)
{
    SAL_AudioFrameBuf *pstAudFrameParam = &g_stAudioHalPrm.stUpLineHalInfo;

    pstAudFrameParam->frameParam.bitWidth = SAL_AUDIO_BIT_WIDTH_16;
    #ifdef DSP_ISM
    pstAudFrameParam->frameParam.sampleRate = SAL_AUDIO_SAMPLE_RATE_8000;/*ISM使用8K采样率的PCM文件，设置ISM输出采样率8K*/
    #else
    pstAudFrameParam->frameParam.sampleRate = SAL_AUDIO_SAMPLE_RATE_16000;/*HIK tts只可以输出16K采样率数据，设置ISA输出采样率16K*/
    #endif

    pstAudFrameParam->frameParam.soundMode = SAL_AUDIO_SOUND_MODE_MONO;
    pstAudFrameParam->frameParam.frameRate = SAL_AUDIO_FRAME_RATE_25;

    pstAudFrameParam = &g_stAudioHalPrm.stDnLineHalInfo;

    pstAudFrameParam->frameParam.bitWidth = SAL_AUDIO_BIT_WIDTH_16;
    #ifdef DSP_ISM
    pstAudFrameParam->frameParam.sampleRate = SAL_AUDIO_SAMPLE_RATE_8000;/*ISM使用8K采样率的PCM文件，设置ISM输出采样率8K*/
    #else
    pstAudFrameParam->frameParam.sampleRate = SAL_AUDIO_SAMPLE_RATE_16000;/*HIK tts只可以输出16K采样率数据，设置ISA输出采样率16K*/
    #endif
    pstAudFrameParam->frameParam.soundMode = SAL_AUDIO_SOUND_MODE_MONO;
    pstAudFrameParam->frameParam.frameRate = SAL_AUDIO_FRAME_RATE_25;

    return;
}

/**
 * @function   audio_hal_getUpLinePram
 * @brief    获取音频上行模块参数
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
SAL_AudioFrameBuf *audio_hal_getUpLinePram(void)
{
    return &g_stAudioHalPrm.stUpLineHalInfo;
}

/**
 * @function   audio_hal_getDnLinePram
 * @brief    获取音频下行模块参数
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
SAL_AudioFrameBuf *audio_hal_getDnLinePram(void)
{
    return &g_stAudioHalPrm.stDnLineHalInfo;
}


/**
 * @function   audio_initAiBuf
 * @brief    初始化音频输入参数
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_initAiBuf(void)
{
    SAL_AudioFrameBuf *pstHalUpLine = &g_stAudioHalPrm.stUpLineHalInfo;
    SAL_AudioFrameParam *pstAudFrameParam = &g_stAudioHalPrm.stUpLineHalInfo.frameParam;


    pstHalUpLine->bufLen = pstAudFrameParam->sampleRate * pstAudFrameParam->bitWidth / pstAudFrameParam->frameRate / 8;
    pstHalUpLine->virAddr[0] = (PhysAddr)SAL_memMalloc(pstHalUpLine->bufLen, "audio", "hal");

    if (0 == pstHalUpLine->virAddr[0])
    {
        AUD_LOGE("pstHalUpLine->pRawBuf malloc failed !!!!!!!!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   audio_hal_aiGetFrame
 * @brief    读取音频采集数据
 * @param[in]  SAL_AudioFrameBuf **ppstAudFrmBuf 音频帧结构指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_aiGetFrame(SAL_AudioFrameBuf **ppstAudFrmBuf)
{
    INT32 s32Ret = SAL_FAIL;
    SAL_AudioFrameBuf *pstHalUpLine = &g_stAudioHalPrm.stUpLineHalInfo;
    HAL_AIO_FRAME_S stAudioFrame;
    CAPB_AUDIO *pstAudioCapb = NULL;
	UINT32 u32AiChn = 0;
    if (NULL == ppstAudFrmBuf)
    {
        AUD_LOGE("null prm\n");
        return SAL_FAIL;
    }

    pstAudioCapb = capb_get_audio();
    if (NULL == pstAudioCapb)
    {
        AUD_LOGE("null err\n");
        return SAL_FAIL;
    }
	for(u32AiChn = 0; u32AiChn < 1; u32AiChn++)
	{
		 //stAudioFrame.AudioDevIdx = pstAudioCapb->audiodevidx;
		 stAudioFrame.AudioDevIdx =0;
		 stAudioFrame.Channel = u32AiChn;
		 stAudioFrame.pBufferAddr = (UINT8 *)pstHalUpLine->virAddr[0];
		 if (NULL == stAudioFrame.pBufferAddr)
		 {
		     AUD_LOGE("buf err !\n");
		     return SAL_FAIL;
		 }

	    if (NULL != g_stAudioHalOps.aiGetFrame)
	    {
	        s32Ret = g_stAudioHalOps.aiGetFrame(&stAudioFrame); /* get actual audio data. use audio component. */
	        if (SAL_SOK != s32Ret)
	        {
	            AUD_LOGE("HAL_AIO_AiGetFrame(%d, %d), failed with %#x!\n", stAudioFrame.AudioDevIdx, \
	                      stAudioFrame.Channel, s32Ret);
	            return SAL_FAIL;
	        }

	        pstHalUpLine->pts = SAL_getTimeOfJiffies();
	        *ppstAudFrmBuf = pstHalUpLine;
	    }
	}

		
    return SAL_SOK;
}

/**
 * @function   audio_hal_aoSendFrame
 * @brief    音频输出
 * @param[in]  HAL_AIO_FRAME_S * pstAoFrame 输出帧
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_aoSendFrame(HAL_AIO_FRAME_S *pstAoFrame)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL == pstAoFrame)
    {
        AUD_LOGE("null prm\n");
        return SAL_FAIL;
    }

    if (NULL == g_stAudioHalOps.aoSendFrame)
    {
        AUD_LOGE("null err\n");
        return SAL_SOK;
    }

    s32Ret = g_stAudioHalOps.aoSendFrame(pstAoFrame);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE(" !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_hal_setVqe
 * @brief    VQE开关操作
 * @param[in]  UINT32 u32AudioChan 音频通道
 * @param[in]  BOOL bEanble 开关标志
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_setVqe(UINT32 u32AudioChan, BOOL bEanble)
{
    INT32 s32Ret = SAL_FAIL;

    if (bEanble)
    {
        if (NULL == g_stAudioHalOps.aiEnableVqe)
        {
            AUD_LOGW(" unsupport Vqe\n");
            return SAL_SOK;
        }

        s32Ret = g_stAudioHalOps.aiEnableVqe(u32AudioChan);
        if (SAL_SOK != s32Ret)
        {
            AUD_LOGE(" !!!\n");
            return SAL_FAIL;
        }
    }
    else
    {
        if (NULL == g_stAudioHalOps.aiDisableVqe)
        {
            AUD_LOGE(" unsupport\n");
            return SAL_SOK;
        }

        s32Ret = g_stAudioHalOps.aiDisableVqe(u32AudioChan);
        if (SAL_SOK != s32Ret)
        {
            AUD_LOGE(" !!!\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   audio_hal_setAiVolume
 * @brief    设置音频输入音量
 * @param[in]  UINT32 u32AudioChan 音频通道
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_setAiVolume(UINT32 u32AudioChan, UINT32 u32Volume)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL == g_stAudioHalOps.aiSetVolume)
    {
        AUD_LOGE("null err\n");
        return SAL_SOK;
    }

    s32Ret = g_stAudioHalOps.aiSetVolume(u32AudioChan, u32Volume);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE(" !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_hal_setAoVolume
 * @brief    设置音频输出音量
 * @param[in]  UINT32 u32AudioChan 音频通道
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_setAoVolume(UINT32 u32AudioChan, UINT32 u32Volume)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL == g_stAudioHalOps.aoSetVolume)
    {
        AUD_LOGE("null err\n");
        return SAL_SOK;
    }

    s32Ret = g_stAudioHalOps.aoSetVolume(u32AudioChan, u32Volume);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE(" !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_hal_create
 * @brief    音频模块创建
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_create(void)
{
    UINT32 i = 0;
    HAL_AIO_ATTR_S stAIOAttr = {0};
    const UINT32 u32AOChanNum = 1;
    const UINT32 u32AIChanNum = 1;
    CAPB_AUDIO *pstCapbAudio = NULL;

    pstCapbAudio = capb_get_audio();
    if (NULL == pstCapbAudio)
    {
        AUD_LOGE("null err\n");
        return SAL_FAIL;
    }

    memset(&g_stAudioHalPrm, 0x00, sizeof(AIO_HAL_INFO));

    audio_initPram();

    audio_initAiBuf();

    AUD_LOGI("========start audio init======\n");

    if (NULL != g_stAudioHalOps.aiCreate)
    {
        /* AI 设备通道初始化 */
        memset(&stAIOAttr, 0, sizeof(HAL_AIO_ATTR_S));
        //stAIOAttr.u32AudioDevIdx = pstCapbAudio->audiodevidx;
		stAIOAttr.u32AudioDevIdx =0;
        stAIOAttr.u32Chan = u32AIChanNum;
        stAIOAttr.u32SampleRate = g_stAudioHalPrm.stUpLineHalInfo.frameParam.sampleRate;
        stAIOAttr.u32BitWidth = g_stAudioHalPrm.stUpLineHalInfo.frameParam.bitWidth;
        stAIOAttr.u32SoundMode = g_stAudioHalPrm.stUpLineHalInfo.frameParam.soundMode;
        stAIOAttr.u32FrameRate = g_stAudioHalPrm.stUpLineHalInfo.frameParam.frameRate;
        if (SAL_SOK != g_stAudioHalOps.aiCreate(&stAIOAttr))
        {
            AUD_LOGE(" !!!\n");
            return SAL_FAIL;
        }
    }

    if (NULL != g_stAudioHalOps.aoCreate)
    {
        /* AO 设备通道初始化 */
        memset(&stAIOAttr, 0, sizeof(stAIOAttr));
        //stAIOAttr.u32AudioDevIdx = pstCapbAudio->audiodevidx;
		stAIOAttr.u32AudioDevIdx =0;
        stAIOAttr.u32Chan = u32AOChanNum;
        stAIOAttr.u32SampleRate = g_stAudioHalPrm.stDnLineHalInfo.frameParam.sampleRate;
        stAIOAttr.u32BitWidth = g_stAudioHalPrm.stDnLineHalInfo.frameParam.bitWidth;
        stAIOAttr.u32SoundMode = g_stAudioHalPrm.stDnLineHalInfo.frameParam.soundMode;
        stAIOAttr.u32FrameRate = g_stAudioHalPrm.stDnLineHalInfo.frameParam.frameRate;

        if (SAL_SOK != g_stAudioHalOps.aoCreate(&stAIOAttr))
        {
            AUD_LOGE(" !!!\n");
            return SAL_FAIL;
        }
    }

    /* 配置默认声音大小 */
    if (NULL != g_stAudioHalOps.aiSetVolume)
    {
        for (i = 0; i < u32AIChanNum; i++)
        {
            if (SAL_SOK != g_stAudioHalOps.aiSetVolume(i, 7))
            {
                AUD_LOGE(" !!!\n");
                return SAL_FAIL;
            }
        }
    }

    if (NULL != g_stAudioHalOps.aoSetVolume)
    {
        for (i = 0; i < u32AOChanNum; i++)
        {
            if (SAL_SOK != g_stAudioHalOps.aoSetVolume(i, 7))
            {
                AUD_LOGE(" !!!\n");
                return SAL_FAIL;
            }
        }
    }

    return SAL_SOK;
}

/**
 * @function   audio_hal_ttsGetSampleRte
 * @brief      获取TTS输出采样率
 * @param[in]  void
 * @param[out] None
 * @return     TTS输出采样率
 */
UINT32 audio_hal_ttsGetSampleRte(void)
{
    if (NULL == g_stAudioHalOps.ttsGetSampleRate)
    {
        AUD_LOGE("null err\n");
        return 8000; /*默认8k采样*/
    }

    return g_stAudioHalOps.ttsGetSampleRate();
}

/**
 * @function   audio_hal_ttsStart
 * @brief      tts语义转换开启
 * @param[in]  void **ppHandle
 * @param[in]  UINT32 u32VoiceType
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_ttsStart(void **ppHandle, UINT32 u32VoiceType)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL == g_stAudioHalOps.ttsStart)
    {
        AUD_LOGE("null err\n");
        return SAL_SOK;
    }

    s32Ret = g_stAudioHalOps.ttsStart(ppHandle, u32VoiceType);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE(" !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_hal_ttsStop
 * @brief      tts语义转换关闭
 * @param[in]  void** ppHandle
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_ttsStop(void **ppHandle)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL == g_stAudioHalOps.ttsStop)
    {
        AUD_LOGE("null err\n");
        return SAL_SOK;
    }

    s32Ret = g_stAudioHalOps.ttsStop(ppHandle);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE(" !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_hal_ttsProc
 * @brief      合成语音处理,hik库单次处理不能超过196字节
 * @param[in]  void* pHandle 输入句柄
 * @param[in]  AUTO_TTS_PARAM_S *pstPrm 输入数据
 * @param[out] PUINT8 pDataOut
 * @param[out] UINT32 *pDataSize
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_ttsProc(void *pHandle, AUTO_TTS_PARAM_S *pstPrm, PUINT8 pDataOut, UINT32 *pDataSize)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL == g_stAudioHalOps.ttsProc)
    {
        AUD_LOGE("null err\n");
        return SAL_SOK;
    }

    if (NULL == pstPrm)
    {
        AUD_LOGE("null err\n");
        return SAL_FAIL;
    }

    s32Ret = g_stAudioHalOps.ttsProc(pHandle, (PUINT8)pstPrm->pIndata, pstPrm->u32DataSize, pDataOut, pDataSize);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE(" !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#if 0

/**
 * @function   audio_hal_Register
 * @brief    弱函数当有平台不支持audio时保证编译通过
 * @param[in]  None
 * @param[out] AUDIO_PLAT_OPS_S *pstAudioHalOps 回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */

__attribute__((weak) /**
                      * @function    audio_hal_register
                      * @brief
                      * @param[in]
                      * @param[out]
                      * @return
                      */
              ) INT32 audio_hal_register(AUDIO_PLAT_OPS_S *pstAudioHalOps)
{
    return SAL_FAIL;
}

#endif

/**
 * @function   audio_hal_init
 * @brief    初始化音频模块
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_init(void)
{

    memset(&g_stAudioHalOps, 0x00, sizeof(AUDIO_PLAT_OPS_S));

    if (SAL_FAIL == audio_hal_register(&g_stAudioHalOps))
    {
        AUD_LOGE("plat reigister fail\n");
        return SAL_SOK;
    }

    return SAL_SOK;

}

