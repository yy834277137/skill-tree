#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include "Pcm_Tinyalsa/asoundlib.h"
#include "Pcm_Audio.h"
//#include <ExternalCodec.h>
#include "sal_trace.h"


/***********************************define value********************************/
#define SOUND_CARD              (0)
#define SOUND_DEVICE            (0)
#define SOUND_PERIOD_SIZE       (80)
#define SOUND_PERIOD_COUNT      (4)

#define SOUND_AO_CONTROLID      (0)
#define SOUND_AI_CONTROLID      (5)


/***********************************global definination********************************/

static struct pcm *pAoPcm = NULL;
static struct pcm *pAiPcm = NULL;
static pthread_mutex_t SendFrame_lock;
static pthread_mutex_t GetFrame_lock;

/*******************************************************************************
* Function      : OneChannelToTwoChannel
* Description   : PCM分支下，当播放音频为单通道时，需要转换为双通道输出
* Input         : - InBuf      : 播放的原始音频数据
*               : - Length     : 原始数据一帧长度
* Output        : - OutBuf     : 转换为双通道后的音频数据
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
static INT32 OneChannelToTwoChannel(UINT8 *InBuf, UINT32 Length, UINT8 *OutBuf)
{
    UINT32 i, cnt;
    UINT16 *pIn = NULL;
    UINT32 *pOut;
    UINT32 temp = 0;

    cnt  = Length / 2;
    pIn  = (UINT16 *)InBuf;
    pOut = (UINT32 *)OutBuf;

    for(i = 0; i < cnt; i++)
    {
        temp = *(pIn + i);
        *(pOut+i) = ((temp & 0xffff) | ((temp & 0xffff) << 16));
    }
    
    return SAL_SOK;
}

/*******************************************************************************
* Function      : TwoChannelToOneChannel
* Description   : PCM分支下，将录制后音频转换为单通道
* Input         : - InBuf      : 播放的原始音频数据
*               : - Length     : 原始数据一帧长度
* Output        : - OutBuf     : 转换为单通道后的音频数据
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/

static INT32 TwoChannelToOneChannel(UINT8 *InBuf, UINT32 Length, UINT8 *OutBuf)
{
    UINT32 i, cnt;
    UINT32 *pIn = NULL;
    UINT16 *pOut;
    UINT32 temp = 0;

    cnt  = Length / 4;
    pIn  = (UINT32 *)InBuf;
    pOut = (UINT16 *)OutBuf;

    for(i = 0; i < cnt; i++)
    {
        temp = *(pIn + i);
        *(pOut+i) = (temp & 0xffff);
    }

    return SAL_SOK;
}

static INT32 check_param(struct pcm_params *params, UINT32 checkparam, UINT32 value,
                 CHAR *param_name, CHAR *param_unit)
{
    UINT32 min;
    UINT32 max;
    INT32 is_within_bounds = 1;

    min = pcm_params_get_min(params, checkparam);

    if (value < min)
    {
        fprintf(stderr, "%s is %u%s, device only supports >= %u%s\n", param_name, value,
                param_unit, min, param_unit);
        is_within_bounds = 0;
    }

    max = pcm_params_get_max(params, checkparam);
    if (value > max)
    {
        fprintf(stderr, "%s is %u%s, device only supports <= %u%s\n", param_name, value,
                param_unit, max, param_unit);
        is_within_bounds = 0;
    }

    return is_within_bounds;
}

static INT32 sample_is_playable(struct pcm_config *pConfig, INT32 bits, INT32 device, INT32 card)
{
    struct pcm_params *params;
    INT32 can_play;

    params = pcm_params_get(card, device, PCM_OUT);
    if (params == NULL)
    {
        fprintf(stderr, "Unable to open PCM device %u. %s, %d\n", device, __func__, __LINE__);
        return 0;
    }

    can_play = check_param(params, PCM_PARAM_RATE, pConfig->rate, "Sample rate", "Hz");
    can_play &= check_param(params, PCM_PARAM_CHANNELS, pConfig->channels, "Sample", " channels");
    can_play &= check_param(params, PCM_PARAM_SAMPLE_BITS, bits, "Bitrate", " bits");
    can_play &= check_param(params, PCM_PARAM_PERIOD_SIZE, pConfig->period_size, "Period size", "Hz");
    can_play &= check_param(params, PCM_PARAM_PERIODS, pConfig->period_count, "Period count", "Hz");

    pcm_params_free(params);

    return can_play;
}

/*******************************************************************************
* Function      : HAL_PCM_AoCreate
* Description   : 创建PCM分支音频输出
* Input         : - pAIOAttr   : 音频输入输出结构描述指针
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
INT32 HAL_PCM_AoCreate(HAL_AIO_ATTR_S *pAIOAttr)
{
    struct pcm_config config;
    INT32 bits = 16;
    
    if (NULL == pAIOAttr)
    {
        printf("Audio Attr is null!\n");
        return SAL_FAIL;
    }

    if(pAoPcm)
    {
        printf("PCM device %u is open\n", SOUND_DEVICE);
        return SAL_SOK;
    }

    memset(&config, 0, sizeof(config));
    config.channels = pAIOAttr->u32Chan;
    config.rate = pAIOAttr->u32SampleRate;
    config.period_size = SOUND_PERIOD_SIZE;
    config.period_count = SOUND_PERIOD_COUNT;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;
    config.format = PCM_FORMAT_S16_LE;

    if (!sample_is_playable(&config, bits, SOUND_DEVICE, SOUND_CARD))
    {
        return SAL_FAIL;
    }

    pAoPcm = pcm_open(SOUND_CARD, SOUND_DEVICE, PCM_OUT, &config);
    if (!pAoPcm || !pcm_is_ready(pAoPcm))
    {
        fprintf(stderr, "Unable to open PCM device %u (%s), %s, %d\n", SOUND_DEVICE, pcm_get_error(pAoPcm), __func__, __LINE__);
        return SAL_FAIL;
    }

    
#ifdef USE_IOTCL
    SND_userDrvInit();
    SND_setRateFormat(pAIOAttr->u32SampleRate);
#endif

    pthread_mutex_init(&SendFrame_lock, NULL);

    return SAL_SOK;
}
/*******************************************************************************
* Function      : HAL_PCM_AoDestroy
* Description   : 销毁PCM分支音频输出
* Input         : - pAIOAttr   : 此处未使用，与其他分支保持一致
* Output        : NONE
* Return        : NONE
*******************************************************************************/

VOID HAL_PCM_AoDestroy(HAL_AIO_ATTR_S *pAIOAttr)
{
    if (NULL != pAoPcm)
    {
        pcm_close(pAoPcm);
        pAoPcm = NULL;
        pthread_mutex_destroy(&SendFrame_lock);
    }

#ifdef USE_IOTCL
    SND_userDrvDeinit();
#endif
}

/*******************************************************************************
* Function      : HAL_PCM_AoSendFrame
* Description   : PCM分支音频输出
* Input         : - pAudioFrame   : 音频帧信息描述结构体指针
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
INT32 HAL_PCM_AoSendFrame(HAL_AIO_FRAME_S *pAudioFrame)
{
    UINT8 *pOutBuf = NULL;
    UINT8 *pWritePtr = NULL;
    UINT32 FrameLen = 0;

    pthread_mutex_lock(&SendFrame_lock);

    if ((NULL == pAudioFrame) || (NULL == pAudioFrame->pBufferAddr))
    {
        printf("Parameter is wrong!\n");
        pthread_mutex_unlock(&SendFrame_lock);
        return SAL_FAIL;
    }

    if (pAudioFrame->Channel > 2)
    {
        printf("Audio channel must not exceed 2!\n");
        pthread_mutex_unlock(&SendFrame_lock);
        return SAL_FAIL;
    }

    pOutBuf = (UINT8 *)malloc(sizeof(UINT8) * pAudioFrame->FrameLen * 2);
	if (pOutBuf == NULL)
	{
		SAL_ERROR("Out Buf malloc fail\n");
		pthread_mutex_unlock(&SendFrame_lock);
		return SAL_FAIL;
	}
    if (pAudioFrame->Channel < 2)
    {
        OneChannelToTwoChannel(pAudioFrame->pBufferAddr, pAudioFrame->FrameLen, pOutBuf);
        pWritePtr = pOutBuf;
        FrameLen = pAudioFrame->FrameLen * 2;
    }
    else
    {
        pWritePtr = pAudioFrame->pBufferAddr;
        FrameLen = pAudioFrame->FrameLen;
    }

	if (NULL == pWritePtr)
	{
	    free(pOutBuf);
        pthread_mutex_unlock(&SendFrame_lock);
        return SAL_FAIL;
	}

    if (pcm_write(pAoPcm, pWritePtr, FrameLen))
    {
        free(pOutBuf);
        pthread_mutex_unlock(&SendFrame_lock);
        return SAL_FAIL;
    }

    free(pOutBuf);
    pthread_mutex_unlock(&SendFrame_lock);

    return SAL_SOK;
}

/*******************************************************************************
* Function      : HAL_PCM_AiCreate
* Description   : 创建PCM分支音频输入
* Input         : - pAIOAttr   : 音频输入输出结构描述指针
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
INT32 HAL_PCM_AiCreate(HAL_AIO_ATTR_S *pAIOAttr)
{
    struct pcm_config config;

    if (NULL == pAIOAttr)
    {
        printf("Audio Attr is null!\n");
        return SAL_FAIL;
    }

    if(pAiPcm)
    {
        printf("PCM device %u is open\n", SOUND_DEVICE);
        return SAL_SOK;
    }

    memset(&config, 0, sizeof(config));
    config.channels = pAIOAttr->u32Chan;
    config.rate = pAIOAttr->u32SampleRate;
    config.period_size = SOUND_PERIOD_SIZE;
    config.period_count = SOUND_PERIOD_COUNT;
    config.format = PCM_FORMAT_S16_LE;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;

    pAiPcm = pcm_open(SOUND_CARD, SOUND_DEVICE, PCM_IN, &config);
    if (!pAiPcm || !pcm_is_ready(pAiPcm))
    {
        fprintf(stderr, "Unable to open PCM device (%s), %s, %d\n", pcm_get_error(pAiPcm), __func__, __LINE__);
        return SAL_FAIL;
    }

#ifdef USE_IOTCL
    SND_userDrvInit();
    SND_setRateFormat(pAIOAttr->u32SampleRate);
#endif

    pthread_mutex_init(&GetFrame_lock, NULL);

    return SAL_SOK;
}

/*******************************************************************************
* Function      : HAL_PCM_AiDestroy
* Description   : 销毁PCM分支音频输入
* Input         : - pAIOAttr   : 此处未使用，与其他分支保持一致
* Output        : NONE
* Return        : NONE
*******************************************************************************/

VOID HAL_PCM_AiDestroy(HAL_AIO_ATTR_S *pAIOAttr)
{
    if (NULL != pAiPcm)
    {
        pcm_close(pAiPcm);
        pAiPcm = NULL;
        pthread_mutex_destroy(&GetFrame_lock);
    }

#ifdef USE_IOTCL 
    SND_userDrvDeinit();
#endif
}

/*******************************************************************************
* Function      : HAL_PCM_AiGetFrame
* Description   : PCM分支音频输入
* Input         : - pAudioFrame   : 音频帧信息描述结构体指针
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
INT32 HAL_PCM_AiGetFrame(HAL_AIO_FRAME_S *pAudioFrame)//input mult
{
    UINT8 *pOutBuf = NULL;
    UINT8 *pReadPtr = NULL;
    UINT32 FrameLen = 0;

    pthread_mutex_lock(&GetFrame_lock);

    if ((NULL == pAudioFrame) || (NULL == pAudioFrame->pBufferAddr))
    {
        printf("Parameter is wrong!\n");
        pthread_mutex_unlock(&GetFrame_lock);
        return SAL_FAIL;
    }

    if (pAudioFrame->Channel > 2)
    {
        printf("Audio channel must not exceed 2!\n");
        pthread_mutex_unlock(&GetFrame_lock);
        return SAL_FAIL;
    }

    pOutBuf = (UINT8 *)malloc(sizeof(UINT8) * pAudioFrame->FrameLen * 2);

    if (pAudioFrame->Channel < 2)
    {
        pReadPtr = pOutBuf;
        FrameLen = pAudioFrame->FrameLen * 2;
    }
    else
    {
        pReadPtr = pAudioFrame->pBufferAddr;
        FrameLen = pAudioFrame->FrameLen;
    }

    if (pcm_read(pAiPcm, pReadPtr, FrameLen))
    {
        free(pOutBuf);
        pthread_mutex_unlock(&GetFrame_lock);
        return SAL_FAIL;
    }

    if (pAudioFrame->Channel < 2)
    {
        TwoChannelToOneChannel(pReadPtr, FrameLen, pAudioFrame->pBufferAddr);
    }

    free(pOutBuf);
    pthread_mutex_unlock(&GetFrame_lock);

    return SAL_SOK;
}

/*******************************************************************************
* Function      : HAL_PCM_AiSetVolume
* Description   : PCM分支设置录制音量
* Input         : - Channel  : 通道号
*               : - Volume   : 音量
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
#ifndef USE_IOCTL
INT32 HAL_PCM_AiSetVolume(UINT32 Channel, UINT32 Volume)
{
    struct mixer *mixer = NULL;
    struct mixer_ctl *ctl;
    unsigned int num_ctl_values;
    unsigned int i;

    if (Channel >= 2)
    {
        printf("Channel ID is wrong! %s, %d\n", __func__, __LINE__);
        return SAL_FAIL;
    }

    mixer = mixer_open(SOUND_CARD);
    if (!mixer)
    {
        printf("Can't open the mixer! %s, %d\n", __func__, __LINE__);
        return SAL_FAIL;
    }

    ctl = mixer_get_ctl(mixer, SOUND_AI_CONTROLID);
    if (!ctl)
    {
        fprintf(stderr, "Invalid AO mixer control\n");
        mixer_close(mixer);
        return SAL_FAIL;
    }

    num_ctl_values = mixer_ctl_get_num_values(ctl);
    /* Set all values the same */
    for (i = 0; i < num_ctl_values; i++)
    {
        if (mixer_ctl_set_value(ctl, i, Volume))
        {
            fprintf(stderr, "Error: invalid AO value\n");
            mixer_close(mixer);
            return SAL_FAIL;
        }
    }

    mixer_close(mixer);

    return SAL_SOK;
}

/*******************************************************************************
* Function      : HAL_PCM_AiSetVolume
* Description   : PCM分支设置播放音量
* Input         : - Channel  : 通道号
*               : - Volume   : 音量
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/
INT32 HAL_PCM_AoSetVolume(UINT32 Channel, UINT32 Volume)
{
    struct mixer *mixer = NULL;
    struct mixer_ctl *ctl;
    unsigned int num_ctl_values;
    unsigned int i;

    if (Channel >= 2)
    {
        printf("Channel ID is wrong! %s, %d\n", __func__, __LINE__);
        return SAL_FAIL;
    }

    mixer = mixer_open(SOUND_CARD);
    if (!mixer)
    {
        printf("Can't open the mixer! %s, %d\n", __func__, __LINE__);
        return SAL_FAIL;
    }

    ctl = mixer_get_ctl(mixer, SOUND_AO_CONTROLID);
    if (!ctl)
    {
        fprintf(stderr, "Invalid AO mixer control\n");
        mixer_close(mixer);
        return SAL_FAIL;
    }

    num_ctl_values = mixer_ctl_get_num_values(ctl);
    /* Set all values the same */
    for (i = 0; i < num_ctl_values; i++)
    {
        if (mixer_ctl_set_value(ctl, i, Volume))
        {
            fprintf(stderr, "Error: invalid AO value\n");
            mixer_close(mixer);
            return SAL_FAIL;
        }
    }

    mixer_close(mixer);

    return SAL_SOK;
}

INT32 HAL_PCM_AoSetLoopBack(UINT32 isLoop)
{
    return SAL_SOK;
}

#else
INT32 HAL_PCM_AiSetVolume(UINT32 Channel, UINT32 Volume)
{
    INT32 ret;

    if (Channel > 2)
    {
        printf("Channel ID is wrong! %s, %d\n", __func__, __LINE__);
        return SAL_FAIL;
    }

    ret = SND_setAdcVolume(Channel, Volume);

    return ret;
}

INT32 HAL_PCM_AoSetVolume(UINT32 Channel, UINT32 Volume)
{
    INT32 ret;

    if (Channel > 2)
    {
        printf("Channel ID is wrong! %s, %d\n", __func__, __LINE__);
        return SAL_FAIL;
    }

    ret = SND_setDacVolume(Channel, Volume);

    return ret;
}

/*******************************************************************************
* Function      : HAL_PCM_AoSetLoopBack
* Description   : PCM分支设置回环播放
* Input         : - isLoop
* Output        : NONE
* Return        : SAL_SOK  : Success
*                 SAL_FAIL : Fail
*******************************************************************************/

INT32 HAL_PCM_AoSetLoopBack(UINT32 isLoop)
{
    INT32 ret;

    ret = SND_setLoopBack(isLoop);

    return ret;
}


#endif



