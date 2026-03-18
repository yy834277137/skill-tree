/**
 * @file   audio_hisi.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---plat层音频接口封装
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/25
     Author      : yindongping
     Modification: 组件开发，整理接口
 */
 
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "sal.h"
#include "platform_sdk.h"
#include "audio_hisi.h"
#include "../tts/kdxf_tts_hal.h"

#define ACODEC_FILE "/dev/acodec"

/*****************************************************************************
                               全局变量
*****************************************************************************/
#define AUDIO_AI_MAX_VOLUME (10)
#define AUDIO_AO_MAX_VOLUME (10)

static INT32 gAudioAIVolume[10] = {-40, -25, -10, 0, 10, 25, 40, 55, 70, 86};
static INT32 gAudioAOVolume[10] = {-56, -30, -10, 0, 1, 2, 3, 4, 5, 6};

static pthread_mutex_t SendFrame_lock;
static pthread_mutex_t GetFrame_lock;

/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   audio_sampleRateParse
 * @brief    计算采样率
 * @param[in] UINT32 u32SampleRate 采样率
 * @param[out] None
 * @return      INT32 采样率
 */
static INT32 audio_sampleRateParse(UINT32 u32SampleRate)
{
    AUDIO_SAMPLE_RATE_E enHisiSampleRate = 0;

    switch (u32SampleRate)
    {
        case 8000:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_8000;
            break;
        }
        case 12000:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_12000;
            break;
        }
        case 11025:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_11025;
            break;
        }
        case 16000:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_16000;
            break;
        }
        case 22050:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_22050;
            break;
        }
        case 24000:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_24000;
            break;
        }
        case 32000:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_32000;
            break;
        }
        case 44100:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_44100;
            break;
        }
        case 48000:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_48000;
            break;
        }
        case 64000:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_64000;
            break;
        }
        case 96000:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_96000;
            break;
        }
        default:
        {
            enHisiSampleRate = AUDIO_SAMPLE_RATE_8000;
            break;
        }
    }

    return enHisiSampleRate;
}
/**
 * @function   audio_biteWidthParse
 * @brief    计算每个采样点的位宽
 * @param[in] UINT32 u32BiteWidth 位宽信息
 * @param[out] None
 * @return      INT32 位宽
 */
static INT32 audio_biteWidthParse(UINT32 u32BiteWidth)
{
    AUDIO_BIT_WIDTH_E enHisiBiteWidth = 0;

    switch (u32BiteWidth)
    {
        case 8:
        {
            enHisiBiteWidth = AUDIO_BIT_WIDTH_8;
            break;
        }
        case 16:
        {
            enHisiBiteWidth = AUDIO_BIT_WIDTH_16;
            break;
        }
        case 24:
        {
            enHisiBiteWidth = AUDIO_BIT_WIDTH_24;
            break;
        }
        default:
        {
            enHisiBiteWidth = AUDIO_BIT_WIDTH_16;
            break;
        }
    }

    return enHisiBiteWidth;

}
/**
 * @function   audio_soundModeParse
 * @brief    获取音频复合模式单，双，静音
 * @param[in] UINT32 SoundMode 模式信息
 * @param[out] None
 * @return      INT32 音频复合模式
 */
static INT32 audio_soundModeParse(UINT32 u32SoundMode)
{
    AUDIO_SOUND_MODE_E enHisiSoundMode = 0;

    switch (u32SoundMode)
    {
        case 1:
        {
            enHisiSoundMode = AUDIO_SOUND_MODE_MONO;
            break;
        }
        case 2:
        {
            enHisiSoundMode = AUDIO_SOUND_MODE_STEREO;
            break;
        }
        default:
        {
            enHisiSoundMode = AUDIO_SOUND_MODE_MONO;
            break;
        }
    }

    return enHisiSoundMode;

}
/**
 * @function   audio_aoCreate
 * @brief    配置音频输出参数
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_aoCreate(HAL_AIO_ATTR_S *pstAIOAttr)
{
    AIO_ATTR_S stAoAttr = {0};
    UINT32 u32AudioDevIdx = 0;
    UINT32 u32AoChn = 0;
    INT32 s32Ret = SAL_FAIL;

    AUDIO_SAMPLE_RATE_E enHisiSampleRate = 0;
    AUDIO_BIT_WIDTH_E enHisiBiteWidth = 0;
    AUDIO_SOUND_MODE_E enHisiSoundMode = 0;
    AO_VQE_CONFIG_S stAoVqeAttr = {0};

    if (NULL == pstAIOAttr)
    {
        SAL_LOGE("AoCreate input ptr is error!\n");
        return SAL_FAIL;
    }

    enHisiSampleRate = audio_sampleRateParse(pstAIOAttr->u32SampleRate);
    enHisiBiteWidth = audio_biteWidthParse(pstAIOAttr->u32BitWidth);
    enHisiSoundMode = audio_soundModeParse(pstAIOAttr->u32SoundMode);

    memset(&stAoAttr, 0, sizeof(AIO_ATTR_S));
    u32AudioDevIdx = pstAIOAttr->u32AudioDevIdx;

    stAoAttr.enWorkmode = AIO_MODE_I2S_MASTER;       /* AIO_MODE_I2S_SLAVE; */
    stAoAttr.u32ChnCnt = pstAIOAttr->u32Chan;
    stAoAttr.enBitwidth = enHisiBiteWidth;
    stAoAttr.enSamplerate = enHisiSampleRate;
    stAoAttr.u32PtNumPerFrm = enHisiSampleRate / pstAIOAttr->u32FrameRate;
    stAoAttr.enSoundmode = enHisiSoundMode;
    stAoAttr.u32EXFlag = 0;
    stAoAttr.u32FrmNum = 6;         /* 30 */
    stAoAttr.u32ClkSel = 0;

    s32Ret = HI_MPI_AO_SetPubAttr(u32AudioDevIdx, &stAoAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AO_SetPubAttr %d attr err: %#x\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_AO_Enable(u32AudioDevIdx);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AO_Enable %d err: %#x\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    for (u32AoChn = 0; u32AoChn < stAoAttr.u32ChnCnt; u32AoChn++)
    {
        s32Ret = HI_MPI_AO_EnableChn(u32AudioDevIdx, u32AoChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("HI_MPI_AO_EnableChn %d:%d attr err: %#x\n", u32AudioDevIdx, u32AoChn, s32Ret);
            HI_MPI_AO_Disable(u32AudioDevIdx);
            return SAL_FAIL;
        }

        memset(&stAoVqeAttr, 0, sizeof(AO_VQE_CONFIG_S));
        stAoVqeAttr.s32WorkSampleRate = AUDIO_SAMPLE_RATE_8000;
        stAoVqeAttr.s32FrameSample = AUDIO_SAMPLE_RATE_8000 / 25;
        stAoVqeAttr.enWorkstate = VQE_WORKSTATE_COMMON;
        stAoVqeAttr.stAnrCfg.bUsrMode = HI_FALSE;
        stAoVqeAttr.stAgcCfg.bUsrMode = HI_FALSE;
        stAoVqeAttr.stHpfCfg.bUsrMode = HI_TRUE;
        stAoVqeAttr.stHpfCfg.enHpfFreq = AUDIO_HPF_FREQ_80;

        stAoVqeAttr.u32OpenMask = AO_VQE_MASK_HPF;

        s32Ret = HI_MPI_AO_SetVqeAttr(u32AudioDevIdx, u32AoChn, &stAoVqeAttr);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("HI_MPI_AO_SetVqeAttr %d:%d attr err: %#x\n", u32AudioDevIdx, u32AoChn, s32Ret);
            return SAL_FAIL;
        }

        s32Ret = HI_MPI_AO_EnableVqe(u32AudioDevIdx, u32AoChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("HI_MPI_AO_EnableVqe %d:%d attr err: %#x\n", u32AudioDevIdx, u32AoChn, s32Ret);
            return SAL_FAIL;
        }
    }

    /* printf(" Hisi Ao init finished !!!!!!\n"); */
    return SAL_SOK;
}
/**
 * @function   audio_aoCreate
 * @brief    配置音频输入参数
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_aiCreate(HAL_AIO_ATTR_S *pstAIOAttr)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32AiChn = 0;
    UINT32 u32AudioDevIdx = 0;
    AIO_ATTR_S stAiAttr = {0};
    AI_CHN_PARAM_S stAiChnPara = {0};

    AUDIO_SAMPLE_RATE_E enHisiSampleRate = 0;
    AUDIO_BIT_WIDTH_E enHisiBiteWidth = 0;
    AUDIO_SOUND_MODE_E enHisiSoundMode = 0;

    if (NULL == pstAIOAttr)
    {
        SAL_LOGE("ptr is NULL!\n");
        return SAL_FAIL;
    }

    AUDIO_MOD_PARAM_S stModParam = {0};
    s32Ret = HI_MPI_AUDIO_GetModParam(&stModParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AUDIO_GetModParam err: %#x\n", s32Ret);
        return SAL_FAIL;
    }

    SAL_LOGI("enClkSel = %d\n", stModParam.enClkSel);

    stModParam.enClkSel = AUDIO_CLKSEL_SPARE;

    s32Ret = HI_MPI_AUDIO_SetModParam(&stModParam);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AUDIO_SetModParam err: %#x\n", s32Ret);
        return SAL_FAIL;
    }

    enHisiSampleRate = audio_sampleRateParse(pstAIOAttr->u32SampleRate);
    enHisiBiteWidth = audio_biteWidthParse(pstAIOAttr->u32BitWidth);
    enHisiSoundMode = audio_soundModeParse(pstAIOAttr->u32SoundMode);

    memset(&stAiAttr, 0, sizeof(AIO_ATTR_S));
    u32AudioDevIdx = pstAIOAttr->u32AudioDevIdx;

    stAiAttr.enWorkmode = AIO_MODE_I2S_MASTER;       /* AIO_MODE_I2S_SLAVE; */
    stAiAttr.u32ChnCnt = pstAIOAttr->u32Chan;
    stAiAttr.enBitwidth = enHisiBiteWidth;
    stAiAttr.enSamplerate = enHisiSampleRate;
    stAiAttr.u32PtNumPerFrm = enHisiSampleRate / 25;
    stAiAttr.enSoundmode = enHisiSoundMode;
    stAiAttr.u32EXFlag = 0;
    stAiAttr.u32FrmNum = 6;         /* 4 */
    stAiAttr.u32ClkSel = 0;

    s32Ret = HI_MPI_AI_SetPubAttr(u32AudioDevIdx, &stAiAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AI_SetPubAttr dev_%d u32ChnCnt_%d attr err: %#x\n", u32AudioDevIdx, stAiAttr.u32ChnCnt, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_AI_Enable(u32AudioDevIdx);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AI_Enable %d err: %#x\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    if (stAiAttr.u32ChnCnt > 1)
    {
        for (u32AiChn = 0; u32AiChn < stAiAttr.u32ChnCnt; u32AiChn++)
        {
            s32Ret = HI_MPI_AI_EnableChn(u32AudioDevIdx, u32AiChn);
            if (HI_SUCCESS != s32Ret)
            {
                SAL_LOGE("HI_MPI_AI_EnableChn %d:%d attr err: %#x\n", u32AudioDevIdx, u32AiChn, s32Ret);
                HI_MPI_AI_Disable(u32AudioDevIdx);
                return SAL_FAIL;
            }
        }
    }
    else
    {
        u32AiChn = 0;
        s32Ret = HI_MPI_AI_EnableChn(u32AudioDevIdx, u32AiChn);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("HI_MPI_AI_EnableChn %d:%d attr err: %#x\n", u32AudioDevIdx, u32AiChn, s32Ret);
            HI_MPI_AI_Disable(u32AudioDevIdx);
            return SAL_FAIL;
        }
    }

    s32Ret = HI_MPI_AI_GetChnParam(u32AudioDevIdx, 0, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("Get ai chn param failed\n");
        return SAL_FAIL;
    }

    stAiChnPara.u32UsrFrmDepth = 6;

    s32Ret = HI_MPI_AI_SetChnParam(u32AudioDevIdx, 0, &stAiChnPara);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("Set ai chn param failed\n");
        return SAL_FAIL;
    }

    AI_TALKVQE_CONFIG_S stAiVqeTalkAttr;
    memset(&stAiVqeTalkAttr, 0, sizeof(AI_TALKVQE_CONFIG_S));
    stAiVqeTalkAttr.s32WorkSampleRate = AUDIO_SAMPLE_RATE_8000;
    stAiVqeTalkAttr.s32FrameSample = AUDIO_SAMPLE_RATE_8000 / 25;
    stAiVqeTalkAttr.enWorkstate = VQE_WORKSTATE_MUSIC;
    stAiVqeTalkAttr.stAecCfg.bUsrMode = HI_FALSE;
    /* stAiVqeTalkAttr.stAecCfg.s8CngMode   = 0; */
    stAiVqeTalkAttr.stAgcCfg.bUsrMode = HI_FALSE;
    stAiVqeTalkAttr.stAnrCfg.bUsrMode = HI_FALSE;
    stAiVqeTalkAttr.stHpfCfg.bUsrMode = HI_TRUE;
    stAiVqeTalkAttr.stHpfCfg.enHpfFreq = AUDIO_HPF_FREQ_80;
    /* stAiVqeTalkAttr.stHdrCfg.bUsrMode    = HI_FALSE; */

    stAiVqeTalkAttr.u32OpenMask = AI_TALKVQE_MASK_AEC | AI_TALKVQE_MASK_HPF | AI_TALKVQE_MASK_ANR;
    /* stAiVqeTalkAttr.u32OpenMask = AI_TALKVQE_MASK_HPF; */

    s32Ret = HI_MPI_AI_SetTalkVqeAttr(u32AudioDevIdx, 0, u32AudioDevIdx, 0, (AI_TALKVQE_CONFIG_S *)(&stAiVqeTalkAttr));
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("================= HI_MPI_AI_SetTalkVqeAttr: %#x ======================\n", s32Ret);
        return SAL_FAIL;
    }

#if 1
    s32Ret = HI_MPI_AI_EnableVqe(u32AudioDevIdx, 0);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("==========u32AudioDevIdx %d======= HI_MPI_AI_EnableVqe: %#x ======================\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

#endif
    return SAL_SOK;
}
/**
 * @function   audio_i2sInit
 * @brief    音频i2s协议初始化
 * @param[in] AUDIO_SAMPLE_RATE_E enSample 采样率
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_i2sInit(AUDIO_SAMPLE_RATE_E enSample)
{
    INT32 s32FdAcodec = -1;
    INT32 s32Ret = SAL_SOK;

    ACODEC_FS_E enI2sFs = 0;
    /* INT32            iAcodecInputVol = 0; */
    ACODEC_MIXER_E enInputMode = 0;

    s32FdAcodec = open(ACODEC_FILE, O_RDWR);
    if (s32FdAcodec < 0)
    {
        SAL_LOGE("Can't open Acodec,%s\n", ACODEC_FILE);
        return SAL_FAIL;
    }

    if (ioctl(s32FdAcodec, ACODEC_SOFT_RESET_CTRL))
    {
        SAL_LOGE("Reset audio codec error\n");
    }

    switch (enSample)
    {
        case AUDIO_SAMPLE_RATE_8000:
            enI2sFs = ACODEC_FS_8000;
            break;

        case AUDIO_SAMPLE_RATE_16000:
            enI2sFs = ACODEC_FS_16000;
            break;

        case AUDIO_SAMPLE_RATE_32000:
            enI2sFs = ACODEC_FS_32000;
            break;

        case AUDIO_SAMPLE_RATE_11025:
            enI2sFs = ACODEC_FS_11025;
            break;

        case AUDIO_SAMPLE_RATE_22050:
            enI2sFs = ACODEC_FS_22050;
            break;

        case AUDIO_SAMPLE_RATE_44100:
            enI2sFs = ACODEC_FS_44100;
            break;

        case AUDIO_SAMPLE_RATE_12000:
            enI2sFs = ACODEC_FS_12000;
            break;

        case AUDIO_SAMPLE_RATE_24000:
            enI2sFs = ACODEC_FS_24000;
            break;

        case AUDIO_SAMPLE_RATE_48000:
            enI2sFs = ACODEC_FS_48000;
            break;

        case AUDIO_SAMPLE_RATE_64000:
            enI2sFs = ACODEC_FS_64000;
            break;

        case AUDIO_SAMPLE_RATE_96000:
            enI2sFs = ACODEC_FS_96000;
            break;

        default:
            SAL_LOGE("Not support enSample:%d\n", enSample);
            s32Ret = HI_FAILURE;
            break;
    }

    if (ioctl(s32FdAcodec, ACODEC_SET_I2S1_FS, &enI2sFs))
    {
        SAL_LOGE("Set acodec sample rate failed\n");
        s32Ret = SAL_FAIL;
    }

    enInputMode = ACODEC_MIXER_IN0;  /* 内置codec硬件使用IN0 */
    if (ioctl(s32FdAcodec, ACODEC_SET_MIXER_MIC, &enInputMode))
    {
        SAL_LOGE("Select acodec enInputMode failed\n");
        s32Ret = SAL_FAIL;
    }

    close(s32FdAcodec);

    audio_hisi_aiSetVolume(0, 10);
    audio_hisi_aoSetVolume(0, 10);

    /* SAL_LOGI("ok\n"); */
    return s32Ret;
}

/**
 * @function   audio_hisi_aoCreate
 * @brief    音频输出初始化
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aoCreate(HAL_AIO_ATTR_S *pstAIOAttr)
{
    INT32 s32Ret = SAL_FAIL;

#ifdef USE_IOCTL
    s32Ret = SND_userDrvInit();
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Auido user device init error!\n");
        return SAL_FAIL;
    }

#else
    /* 配置内部codec */
    if (SAL_SOK != audio_i2sInit(pstAIOAttr->u32SampleRate))
    {
        SAL_LOGE("Hal_acodecInit !!!\n");
        return SAL_FAIL;
    }

#endif

    s32Ret = audio_aoCreate(pstAIOAttr);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("AO Create error!\n");
#ifdef USE_IOCTL
        SND_userDrvDeinit();
#endif
        return SAL_FAIL;
    }

    pthread_mutex_init(&SendFrame_lock, NULL);

    return SAL_SOK;
}
/**
 * @function   audio_hisi_aoDestroy
 * @brief    销毁音频输出
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
VOID audio_hisi_aoDestroy(HAL_AIO_ATTR_S *pstAIOAttr)
{
    UINT32 u32AoChn = 0;

    if (NULL == pstAIOAttr)
    {
        SAL_LOGE("AoDestroy input ptr is error!\n");
        return;
    }

#ifdef USE_IOCTL
    SND_userDrvDeinit();
#endif

    if (pstAIOAttr->u32Chan > 1)
    {
        for (u32AoChn = 0; u32AoChn < pstAIOAttr->u32Chan; u32AoChn++)
        {
            HI_MPI_AO_DisableChn(pstAIOAttr->u32AudioDevIdx, u32AoChn);
        }
    }
    else
    {
        u32AoChn = 0;
        HI_MPI_AO_DisableChn(pstAIOAttr->u32AudioDevIdx, u32AoChn);
    }

    HI_MPI_AO_Disable(pstAIOAttr->u32AudioDevIdx);

    pthread_mutex_destroy(&SendFrame_lock);
}
/**
 * @function   audio_hisi_aoSendFrame
 * @brief    输出音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aoSendFrame(HAL_AIO_FRAME_S *pstAudioFrame)
{
    AUDIO_FRAME_S stFrame = {0};
    INT32 s32Ret = SAL_FAIL;

    pthread_mutex_lock(&SendFrame_lock);

    if ((NULL == pstAudioFrame) || (NULL == pstAudioFrame->pBufferAddr))
    {
        SAL_LOGE("Parameter is wrong!\n");
        pthread_mutex_unlock(&SendFrame_lock);
        return SAL_FAIL;
    }

    memset(&stFrame, 0, sizeof(AUDIO_FRAME_S));
    stFrame.u32Len = pstAudioFrame->FrameLen;
    stFrame.enBitwidth = AUDIO_BIT_WIDTH_16;
    stFrame.enSoundmode = AUDIO_SOUND_MODE_MONO;
    stFrame.u64VirAddr[0] = pstAudioFrame->pBufferAddr;
    s32Ret = HI_MPI_AO_SendFrame(pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel, &stFrame, 500);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AO_SendFrame(%d,%d) err %#x\n", pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel, s32Ret);
        pthread_mutex_unlock(&SendFrame_lock);
        return SAL_FAIL;
    }

    pthread_mutex_unlock(&SendFrame_lock);

    return SAL_SOK;
}
/**
 * @function   audio_hisi_aiCreate
 * @brief    音频输入初始化
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiCreate(HAL_AIO_ATTR_S *pstAIOAttr)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL == pstAIOAttr)
    {
        SAL_LOGE("Input parameter pstAIOAttr is NULL!!!\n");
        return SAL_FAIL;
    }

#ifdef USE_IOCTL
    s32Ret = SND_userDrvInit();
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Auido user device init error!\n");
        return SAL_FAIL;
    }

#endif

    s32Ret = audio_aiCreate(pstAIOAttr);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Ai Create error!\n");

#ifdef USE_IOCTL
        SND_userDrvDeinit();
#endif
        return SAL_FAIL;
    }

    pthread_mutex_init(&GetFrame_lock, NULL);

    return SAL_SOK;
}
/**
 * @function   audio_hisi_aiDestroy
 * @brief    音频输入销毁
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
VOID audio_hisi_aiDestroy(HAL_AIO_ATTR_S *pstAIOAttr)
{
    UINT32 u32AiChn = 0;

    if (NULL == pstAIOAttr)
    {
        SAL_LOGE("AiDestroy input ptr is error!\n");
        return;
    }

#ifdef USE_IOCTL
    SND_userDrvDeinit();
#endif

    if (pstAIOAttr->u32Chan > 1)
    {
        for (u32AiChn = 0; u32AiChn < pstAIOAttr->u32Chan; u32AiChn++)
        {
            HI_MPI_AI_DisableChn(pstAIOAttr->u32AudioDevIdx, u32AiChn);
        }
    }
    else
    {
        u32AiChn = 0;
        HI_MPI_AI_DisableChn(pstAIOAttr->u32AudioDevIdx, u32AiChn);
    }

    HI_MPI_AO_Disable(pstAIOAttr->u32AudioDevIdx);

    pthread_mutex_destroy(&GetFrame_lock);
}
/**
 * @function   audio_hisi_aiGetFrame
 * @brief    获取音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiGetFrame(HAL_AIO_FRAME_S *pstAudioFrame)
{
    HI_S32 s32Ret = HI_SUCCESS;
    AEC_FRAME_S stAecFrm = {0};
    AUDIO_FRAME_S stFrame = {0};

    UINT32 u32AudioDevIdx = 0;

    pthread_mutex_lock(&GetFrame_lock);

    if (NULL == pstAudioFrame)
    {
        SAL_LOGE("Parameter is wrong!\n");
        pthread_mutex_unlock(&GetFrame_lock);
        return SAL_FAIL;
    }

    u32AudioDevIdx = pstAudioFrame->AudioDevIdx;

    memset(&stFrame, 0, sizeof(AUDIO_FRAME_S));
    memset(&stAecFrm, 0, sizeof(AEC_FRAME_S));

    /* s32Ret = HI_MPI_AI_GetFrame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame, &stAecFrm, 30); */
    /* s32Ret = HI_MPI_AI_GetFrame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame, &stAecFrm, 0); */
    s32Ret = HI_MPI_AI_GetFrame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame, &stAecFrm, 2000);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AI_GetFrame(%d, %d), failed with %#x!\n", \
                  u32AudioDevIdx, pstAudioFrame->Channel, s32Ret);
        pthread_mutex_unlock(&GetFrame_lock);
        return SAL_FAIL;
    }

    memcpy(pstAudioFrame->pBufferAddr, stFrame.u64VirAddr[pstAudioFrame->Channel], stFrame.u32Len);
    pstAudioFrame->FrameLen = stFrame.u32Len;

    s32Ret = HI_MPI_AI_ReleaseFrame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame, &stAecFrm);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n", \
                  u32AudioDevIdx, pstAudioFrame->Channel, s32Ret);
        pthread_mutex_unlock(&GetFrame_lock);
        return SAL_FAIL;
    }

    pthread_mutex_unlock(&GetFrame_lock);

    return SAL_SOK;
}
#ifndef USE_IOCTL

/**
 * @function   audio_hisi_aiSetVolume
 * @brief    设置音频输入音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiSetVolume(UINT32 u32Channel, UINT32 u32Volume)
{
    INT32 s32AcodecInputVol = 0;
    INT32 s32FdAcodec = -1;

    if (u32Volume > AUDIO_AI_MAX_VOLUME)
    {
        SAL_LOGW("u32Volume=%d > %d,use MaxVolume\n", u32Volume, AUDIO_MAX_VOLUME);
        u32Volume = AUDIO_MAX_VOLUME;
    }

    /* printf("=========HISI_SetAiVolume:%d=========\n",u32Volume); */

    /* HISI_SetAiVolume(u32Volume); */
    s32FdAcodec = open(ACODEC_FILE, O_RDWR);
    if (s32FdAcodec < 0)
    {
        SAL_LOGE("can't open Acodec,%s\n", ACODEC_FILE);
        return HI_FAILURE;
    }

    s32AcodecInputVol = (u32Volume == 0) ? -77 : gAudioAIVolume[u32Volume - 1]; /* ((INT32)(u32Volume*16) - 77) > 86 ? 86 : (u32Volume*16 - 77); */
    SAL_LOGI("InputVol = %d\n", s32AcodecInputVol);
    if (ioctl(s32FdAcodec, ACODEC_SET_INPUT_VOL, &s32AcodecInputVol))
    {
        SAL_LOGE("set acodec micin volume failed\n");
        close(s32FdAcodec);
        return HI_FAILURE;
    }

    close(s32FdAcodec);

    return SAL_SOK;
}

/**
 * @function   audio_hisi_aoSetVolume
 * @brief    设置音频输出音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aoSetVolume(UINT32 u32Channel, UINT32 u32Volume)
{
    INT32 s32AcodecInputVol = 0;
    INT32 s32FdAcodec = -1;

    if (u32Volume > AUDIO_AO_MAX_VOLUME)
    {
        SAL_LOGW("u32Volume=%d > %d,use MaxVolume\n", u32Volume, AUDIO_MAX_VOLUME);
        u32Volume = AUDIO_MAX_VOLUME;
    }

    SAL_LOGI("=========HISI_SetAoVolume:%d=========\n", u32Volume);
    /* HISI_SetAoVolume(u32Volume); */
    s32FdAcodec = open(ACODEC_FILE, O_RDWR);
    if (s32FdAcodec < 0)
    {
        SAL_LOGE("can't open Acodec,%s\n", ACODEC_FILE);
        return HI_FAILURE;
    }

    s32AcodecInputVol = (u32Volume == 0) ? -121 : gAudioAOVolume[u32Volume - 1]; /* ((INT32)(u32Volume*13) - 121) > 6 ? 6 : (u32Volume*13 - 121); */
    SAL_LOGI("Output Vol = %d\n", s32AcodecInputVol);
    if (ioctl(s32FdAcodec, ACODEC_SET_OUTPUT_VOL, &s32AcodecInputVol))
    {
        SAL_LOGE("set acodec micin volume failed\n");
        close(s32FdAcodec);
        return HI_FAILURE;
    }

    close(s32FdAcodec);

    return SAL_SOK;
}
/**
 * @function   audio_hisi_aiEnableVqe
 * @brief    开启VQE处理(声音质量增强)
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiEnableVqe(UINT32 u32AudioDevIdx)
{
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = HI_MPI_AI_EnableVqe(u32AudioDevIdx, 0);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("==========AudioDevIdx %d======= HI_MPI_AI_EnableVqe: %#x ======================\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}
/**
 * @function   audio_hisi_aiDisableVqe
 * @brief    关闭VQE处理
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiDisableVqe(UINT32 u32AudioDevIdx)
{
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = HI_MPI_AI_DisableVqe(u32AudioDevIdx, 0);
    if (HI_SUCCESS != s32Ret)
    {
        SAL_LOGE("==========AudioDevIdx %d======= HI_MPI_AI_EnableVqe: %#x ======================\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#else
/**
 * @function   audio_hisi_aiSetVolume
 * @brief    设置音频输入音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiSetVolume(UINT32 u32Channel, UINT32 u32Volume)
{
    INT32 s32Ret = SAL_FAIL;

    if (u32Volume > AUDIO_MAX_VOLUME)
    {
        SAL_LOGW("u32Volume=%d > %d,use MaxVolume\n", u32Volume, AUDIO_MAX_VOLUME);
        u32Volume = AUDIO_MAX_VOLUME;
    }

    if (u32Channel > 2)
    {
        SAL_LOGE("u32Channel ID is wrong!\n");
        return SAL_FAIL;
    }

    s32Ret = SND_setAdcVolume(u32Channel, u32Volume);

    return s32Ret;
}
/**
 * @function   audio_hisi_aoSetVolume
 * @brief    设置音频输出音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aoSetVolume(UINT32 u32Channel, UINT32 u32Volume)
{
    INT32 s32Ret = SAL_FAIL;

    SAL_LOGI("go to here, u32Channel = %d, u32Volume = %d\n", u32Channel, u32Volume);
    if (u32Channel > 2)
    {
        SAL_LOGE("u32Channel ID is wrong!\n");
        return SAL_FAIL;
    }

    if (u32Volume > AUDIO_MAX_VOLUME)
    {
        SAL_LOGW("u32Volume=%d > %d,use MaxVolume\n", u32Volume, AUDIO_MAX_VOLUME);
        u32Volume = AUDIO_MAX_VOLUME;
    }

    s32Ret = SND_setDacVolume(u32Channel, u32Volume);

    return s32Ret;
}
/**
 * @function   audio_hisi_aiEnableVqe
 * @brief    开启VQE处理
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiEnableVqe(UINT32 u32AudioDevIdx)
{
    return SAL_SOK;
}
/**
 * @function   audio_hisi_aiDisableVqe
 * @brief    关闭VQE处理
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiDisableVqe(UINT32 u32AudioDevIdx)
{
    return SAL_SOK;
}

#endif
/**
 * @function   audio_hal_register
 * @brief      注册hal层回调函数
 * @param[in]  None
 * @param[out] AUDIO_PLAT_OPS_S * 回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_register(AUDIO_PLAT_OPS_S * pstAudioPlatOps)
{
    if(NULL == pstAudioPlatOps)
    {
        return SAL_FAIL;
    }
    
    pstAudioPlatOps->aoCreate = audio_hisi_aoCreate;
    pstAudioPlatOps->aoDestroy = audio_hisi_aoDestroy;
    pstAudioPlatOps->aoSendFrame = audio_hisi_aoSendFrame;
    pstAudioPlatOps->aoSetVolume = audio_hisi_aoSetVolume;

    pstAudioPlatOps->aiCreate = audio_hisi_aiCreate;
    pstAudioPlatOps->aiDestroy = audio_hisi_aiDestroy;
    pstAudioPlatOps->aiGetFrame = audio_hisi_aiGetFrame;
    pstAudioPlatOps->aiSetVolume = audio_hisi_aiSetVolume;
    pstAudioPlatOps->aiEnableVqe = audio_hisi_aiEnableVqe;
    pstAudioPlatOps->aiDisableVqe = audio_hisi_aiDisableVqe;
    pstAudioPlatOps->aiSetLoopBack = NULL;

    /*tts*/
    pstAudioPlatOps->ttsGetSampleRate = kdxf_tts_getSampleRate;
    pstAudioPlatOps->ttsStart = kdxf_tts_start;
    pstAudioPlatOps->ttsProc = kdxf_tts_proc;
    pstAudioPlatOps->ttsStop = kdxf_tts_stop;

    return SAL_SOK;
}


