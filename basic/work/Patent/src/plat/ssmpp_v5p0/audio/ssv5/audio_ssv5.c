/**
 * @file   audio_ssv5.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---plat层音频接口封装
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/08/09
     Author      : yindongping
     Modification: 组件开发，整理接口
 */
 
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "sal.h"
#include "platform_sdk.h"
#include "audio_ssv5.h"

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
    ot_audio_sample_rate enSampleRate = 0;

    switch (u32SampleRate)
    {
        case 8000:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_8000;
            break;
        }
        case 12000:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_12000;
            break;
        }
        case 11025:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_11025;
            break;
        }
        case 16000:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_16000;
            break;
        }
        case 22050:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_22050;
            break;
        }
        case 24000:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_24000;
            break;
        }
        case 32000:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_32000;
            break;
        }
        case 44100:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_44100;
            break;
        }
        case 48000:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_48000;
            break;
        }
        case 64000:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_64000;
            break;
        }
        case 96000:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_96000;
            break;
        }
        default:
        {
            enSampleRate = OT_AUDIO_SAMPLE_RATE_8000;
            break;
        }
    }

    return enSampleRate;
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
    ot_audio_bit_width enBiteWidth = 0;

    switch (u32BiteWidth)
    {
        case 8:
        {
            enBiteWidth = OT_AUDIO_BIT_WIDTH_8;
            break;
        }
        case 16:
        {
            enBiteWidth = OT_AUDIO_BIT_WIDTH_16;
            break;
        }
        case 24:
        {
            enBiteWidth = OT_AUDIO_BIT_WIDTH_24;
            break;
        }
        default:
        {
            enBiteWidth = OT_AUDIO_BIT_WIDTH_16;
            break;
        }
    }

    return enBiteWidth;

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
    ot_audio_snd_mode enSoundMode = 0;

    switch (u32SoundMode)
    {
        case 1:
        {
            enSoundMode = OT_AUDIO_SOUND_MODE_MONO;
            break;
        }
        case 2:
        {
            enSoundMode = OT_AUDIO_SOUND_MODE_STEREO;
            break;
        }
        default:
        {
            enSoundMode = OT_AUDIO_SOUND_MODE_MONO;
            break;
        }
    }

    return enSoundMode;

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
    ot_aio_attr stAoAttr = {0};
    ot_audio_dev u32AudioDevIdx = 0;
    ot_ao_chn u32AoChn = 0;
    INT32 s32Ret = SAL_FAIL;

    ot_audio_sample_rate enSampleRate = 0;
    ot_audio_bit_width enBiteWidth = 0;
    ot_audio_snd_mode enSoundMode = 0;
    ot_ao_vqe_cfg stAoVqeAttr = {0};

    if (NULL == pstAIOAttr)
    {
        SAL_LOGE("AoCreate input ptr is error!\n");
        return SAL_FAIL;
    }

    enSampleRate = audio_sampleRateParse(pstAIOAttr->u32SampleRate);
    enBiteWidth = audio_biteWidthParse(pstAIOAttr->u32BitWidth);
    enSoundMode = audio_soundModeParse(pstAIOAttr->u32SoundMode);

    memset(&stAoAttr, 0, sizeof(ot_aio_attr));
    u32AudioDevIdx = pstAIOAttr->u32AudioDevIdx;

    stAoAttr.work_mode = OT_AIO_MODE_I2S_MASTER;       /* AIO_MODE_I2S_SLAVE; */
    stAoAttr.chn_cnt = pstAIOAttr->u32Chan + 1;/*左声道是0，右声道是1，硬件只接右声道*/
    stAoAttr.bit_width = enBiteWidth;
    stAoAttr.sample_rate = enSampleRate;
    stAoAttr.point_num_per_frame = enSampleRate / pstAIOAttr->u32FrameRate;
    stAoAttr.snd_mode = enSoundMode;
    stAoAttr.expand_flag = 0;
    stAoAttr.frame_num = 6;         /* 30 */
    stAoAttr.clk_share = 1;
    stAoAttr.i2s_type = OT_AIO_I2STYPE_INNERCODEC;

    s32Ret = ss_mpi_ao_set_pub_attr(u32AudioDevIdx, &stAoAttr);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AO_SetPubAttr %d attr err: %#x\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_ao_enable(u32AudioDevIdx);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("ss_mpi_ao_enable %d err: %#x\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    for (u32AoChn = 0; u32AoChn < stAoAttr.chn_cnt; u32AoChn++)
    {
        s32Ret = ss_mpi_ao_enable_chn(u32AudioDevIdx, u32AoChn);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_LOGE("ss_mpi_ao_enable_chn %d:%d attr err: %#x\n", u32AudioDevIdx, u32AoChn, s32Ret);
            ss_mpi_ao_disable(u32AudioDevIdx);
            return SAL_FAIL;
        }

        memset(&stAoVqeAttr, 0, sizeof(ot_ao_vqe_cfg));
        stAoVqeAttr.work_sample_rate = OT_AUDIO_SAMPLE_RATE_8000;
        stAoVqeAttr.frame_sample = OT_AUDIO_SAMPLE_RATE_8000 / 25;
        stAoVqeAttr.work_state = OT_VQE_WORK_STATE_COMMON;
        stAoVqeAttr.anr_cfg.usr_mode = TD_FALSE;
        stAoVqeAttr.agc_cfg.usr_mode = TD_FALSE;
        stAoVqeAttr.hpf_cfg.usr_mode = TD_TRUE;
        stAoVqeAttr.hpf_cfg.hpf_freq = OT_AUDIO_HPF_FREQ_80;

        stAoVqeAttr.open_mask = OT_AO_VQE_MASK_HPF;

        s32Ret = ss_mpi_ao_set_vqe_attr(u32AudioDevIdx, u32AoChn, &stAoVqeAttr);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_LOGE("HI_MPI_AO_SetVqeAttr %d:%d attr err: %#x\n", u32AudioDevIdx, u32AoChn, s32Ret);
            return SAL_FAIL;
        }

        s32Ret = ss_mpi_ao_enable_vqe(u32AudioDevIdx, u32AoChn);
        if (TD_SUCCESS != s32Ret)
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
    ot_audio_dev u32AudioDevIdx = 0;
    ot_aio_attr stAiAttr = {0};
    ot_ai_chn_param stAiChnPara = {0};

    ot_audio_sample_rate enSampleRate = 0;
    ot_audio_bit_width enBiteWidth = 0;
    ot_audio_snd_mode enSoundMode = 0;

    if (NULL == pstAIOAttr)
    {
        SAL_LOGE("ptr is NULL!\n");
        return SAL_FAIL;
    }

    ot_audio_mod_param stModParam = {0};
    s32Ret = ss_mpi_audio_get_mod_param(&stModParam);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AUDIO_GetModParam err: %#x\n", s32Ret);
        return SAL_FAIL;
    }

    SAL_LOGI("enClkSel = %d\n", stModParam.clk_select);

    stModParam.clk_select = OT_AUDIO_CLK_SELECT_SPARE;

    s32Ret = ss_mpi_audio_set_mod_param(&stModParam);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AUDIO_SetModParam err: %#x\n", s32Ret);
        return SAL_FAIL;
    }

    enSampleRate = audio_sampleRateParse(pstAIOAttr->u32SampleRate);
    enBiteWidth = audio_biteWidthParse(pstAIOAttr->u32BitWidth);
    enSoundMode = audio_soundModeParse(pstAIOAttr->u32SoundMode);

    memset(&stAiAttr, 0, sizeof(ot_aio_attr));
    u32AudioDevIdx = pstAIOAttr->u32AudioDevIdx;

    stAiAttr.work_mode = OT_AIO_MODE_I2S_MASTER;       /* AIO_MODE_I2S_SLAVE; */
    stAiAttr.chn_cnt = pstAIOAttr->u32Chan;
    stAiAttr.bit_width = enBiteWidth;
    stAiAttr.sample_rate = enSampleRate;
    stAiAttr.point_num_per_frame = enSampleRate / 25;
    stAiAttr.snd_mode = enSoundMode;
    stAiAttr.expand_flag = 0;
    stAiAttr.frame_num = 6;         /* 4 */
    stAiAttr.clk_share = 1;
    stAiAttr.i2s_type = OT_AIO_I2STYPE_INNERCODEC;

    s32Ret = ss_mpi_ai_set_pub_attr(u32AudioDevIdx, &stAiAttr);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AI_SetPubAttr dev_%d u32ChnCnt_%d attr err: %#x\n", u32AudioDevIdx, stAiAttr.chn_cnt, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_ai_enable(u32AudioDevIdx);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AI_Enable %d err: %#x\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    if (stAiAttr.chn_cnt > 1)
    {
        for (u32AiChn = 0; u32AiChn < stAiAttr.chn_cnt; u32AiChn++)
        {
            s32Ret = ss_mpi_ai_enable_chn(u32AudioDevIdx, u32AiChn);
            if (TD_SUCCESS != s32Ret)
            {
                SAL_LOGE("HI_MPI_AI_EnableChn %d:%d attr err: %#x\n", u32AudioDevIdx, u32AiChn, s32Ret);
                ss_mpi_ai_disable(u32AudioDevIdx);
                return SAL_FAIL;
            }
        }
    }
    else
    {
        u32AiChn = 0;
        s32Ret = ss_mpi_ai_enable_chn(u32AudioDevIdx, u32AiChn);
        if (TD_SUCCESS != s32Ret)
        {
            SAL_LOGE("HI_MPI_AI_EnableChn %d:%d attr err: %#x\n", u32AudioDevIdx, u32AiChn, s32Ret);
            ss_mpi_ai_disable(u32AudioDevIdx);
            return SAL_FAIL;
        }
    }

    s32Ret = ss_mpi_ai_get_chn_param(u32AudioDevIdx, 0, &stAiChnPara);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("Get ai chn param failed\n");
        return SAL_FAIL;
    }

    stAiChnPara.usr_frame_depth = 6;

    s32Ret = ss_mpi_ai_set_chn_param(u32AudioDevIdx, 0, &stAiChnPara);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("Set ai chn param failed\n");
        return SAL_FAIL;
    }

    ot_ai_talk_vqe_cfg stAiVqeTalkAttr;
    memset(&stAiVqeTalkAttr, 0, sizeof(ot_ai_talk_vqe_cfg));
    stAiVqeTalkAttr.work_sample_rate = OT_AUDIO_SAMPLE_RATE_8000;
    stAiVqeTalkAttr.frame_sample = OT_AUDIO_SAMPLE_RATE_8000 / 25;
    stAiVqeTalkAttr.work_state = OT_VQE_WORK_STATE_MUSIC;
    stAiVqeTalkAttr.aec_cfg.usr_mode = TD_FALSE;
    /* stAiVqeTalkAttr.aec_cfg.cozy_noisy_mode   = 0; */
    stAiVqeTalkAttr.agc_cfg.usr_mode = TD_FALSE;
    stAiVqeTalkAttr.anr_cfg.usr_mode = TD_FALSE;
    stAiVqeTalkAttr.hpf_cfg.usr_mode = TD_TRUE;
    stAiVqeTalkAttr.hpf_cfg.hpf_freq = OT_AUDIO_HPF_FREQ_80;
    /* stAiVqeTalkAttr.stHdrCfg.bUsrMode    = TD_FALSE; */

    stAiVqeTalkAttr.open_mask = OT_AI_TALKVQE_MASK_AEC | OT_AI_TALKVQE_MASK_HPF | OT_AI_TALKVQE_MASK_ANR;
    /* stAiVqeTalkAttr.u32OpenMask = AI_TALKVQE_MASK_HPF; */

    s32Ret = ss_mpi_ai_set_talk_vqe_attr(u32AudioDevIdx, 0, u32AudioDevIdx, 0, (ot_ai_talk_vqe_cfg *)(&stAiVqeTalkAttr));
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("================= HI_MPI_AI_SetTalkVqeAttr: %#x ======================\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_ai_enable_vqe(u32AudioDevIdx, 0);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("==========u32AudioDevIdx %d======= HI_MPI_AI_EnableVqe: %#x ======================\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}
/**
 * @function   audio_i2sInit
 * @brief    音频i2s协议初始化
 * @param[in] hi_audio_sample_rate enSample 采样率
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_i2sInit(ot_audio_sample_rate enSample)
{
    INT32 s32FdAcodec = -1;
    INT32 s32Ret = SAL_SOK;

    ot_acodec_fs enI2sFs = 0;
    /* INT32            iAcodecInputVol = 0; */
    ot_acodec_mixer enInputMode = 0;

    s32FdAcodec = open(ACODEC_FILE, O_RDWR);
    if (s32FdAcodec < 0)
    {
        SAL_LOGE("Can't open Acodec,%s\n", ACODEC_FILE);
        return SAL_FAIL;
    }

    if (ioctl(s32FdAcodec, OT_ACODEC_SOFT_RESET_CTRL))
    {
        SAL_LOGE("Reset audio codec error\n");
    }

    switch (enSample)
    {
        case OT_AUDIO_SAMPLE_RATE_8000:
            enI2sFs = OT_ACODEC_FS_8000;
            break;

        case OT_AUDIO_SAMPLE_RATE_16000:
            enI2sFs = OT_ACODEC_FS_16000;
            break;

        case OT_AUDIO_SAMPLE_RATE_32000:
            enI2sFs = OT_ACODEC_FS_32000;
            break;

        case OT_AUDIO_SAMPLE_RATE_11025:
            enI2sFs = OT_ACODEC_FS_11025;
            break;

        case OT_AUDIO_SAMPLE_RATE_22050:
            enI2sFs = OT_ACODEC_FS_22050;
            break;

        case OT_AUDIO_SAMPLE_RATE_44100:
            enI2sFs = OT_ACODEC_FS_44100;
            break;

        case OT_AUDIO_SAMPLE_RATE_12000:
            enI2sFs = OT_ACODEC_FS_12000;
            break;

        case OT_AUDIO_SAMPLE_RATE_24000:
            enI2sFs = OT_ACODEC_FS_24000;
            break;

        case OT_AUDIO_SAMPLE_RATE_48000:
            enI2sFs = OT_ACODEC_FS_48000;
            break;

        case OT_AUDIO_SAMPLE_RATE_64000:
            enI2sFs = OT_ACODEC_FS_64000;
            break;

        case OT_AUDIO_SAMPLE_RATE_96000:
            enI2sFs = OT_ACODEC_FS_96000;
            break;

        default:
            SAL_LOGE("Not support enSample:%d\n", enSample);
            s32Ret = TD_FAILURE;
            break;
    }

    if (ioctl(s32FdAcodec, OT_ACODEC_SET_I2S1_FS, &enI2sFs))
    {
        SAL_LOGE("Set acodec sample rate failed\n");
        s32Ret = SAL_FAIL;
    }

    enInputMode = OT_ACODEC_MIXER_IN1; /*取值范围[1, 2]，默认选择 IN1 单端输入，差分输入选择IN_D*/
    if (ioctl(s32FdAcodec, OT_ACODEC_SET_MIXER_MIC, &enInputMode))
    {
        SAL_LOGE("Select acodec enInputMode failed\n");
        s32Ret = SAL_FAIL;
    }

    close(s32FdAcodec);

    audio_ssV5_aiSetVolume(0, 10);
    audio_ssV5_aoSetVolume(0, 10);

    /* SAL_LOGI("ok\n"); */
    return s32Ret;
}

/**
 * @function   audio_ssV5_aoCreate
 * @brief    音频输出初始化
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aoCreate(HAL_AIO_ATTR_S *pstAIOAttr)
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
 * @function   audio_ssV5_aoDestroy
 * @brief    销毁音频输出
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
VOID audio_ssV5_aoDestroy(HAL_AIO_ATTR_S *pstAIOAttr)
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
            ss_mpi_ao_disable_chn(pstAIOAttr->u32AudioDevIdx, u32AoChn);
        }
    }
    else
    {
        u32AoChn = 0;
        ss_mpi_ao_disable_chn(pstAIOAttr->u32AudioDevIdx, u32AoChn);
    }

    ss_mpi_ao_disable(pstAIOAttr->u32AudioDevIdx);

    pthread_mutex_destroy(&SendFrame_lock);
}
/**
 * @function   audio_ssV5_aoSendFrame
 * @brief    输出音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aoSendFrame(HAL_AIO_FRAME_S *pstAudioFrame)
{
    ot_audio_frame stFrame = {0};
    INT32 s32Ret = SAL_FAIL;

    pthread_mutex_lock(&SendFrame_lock);

    if ((NULL == pstAudioFrame) || (NULL == pstAudioFrame->pBufferAddr))
    {
        SAL_LOGE("Parameter is wrong!\n");
        pthread_mutex_unlock(&SendFrame_lock);
        return SAL_FAIL;
    }

    memset(&stFrame, 0, sizeof(ot_audio_frame));
    stFrame.len = pstAudioFrame->FrameLen;
    stFrame.bit_width = OT_AUDIO_BIT_WIDTH_16;
    stFrame.snd_mode = OT_AUDIO_SOUND_MODE_MONO;
    stFrame.virt_addr[0] = pstAudioFrame->pBufferAddr;
    /*左声道是0，右声道是1，硬件只接右声道*/
    s32Ret = ss_mpi_ao_send_frame(pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel + 1, &stFrame, 500);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AO_SendFrame(%d,%d) err %#x\n", pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel, s32Ret);
        pthread_mutex_unlock(&SendFrame_lock);
        return SAL_FAIL;
    }

    pthread_mutex_unlock(&SendFrame_lock);

    return SAL_SOK;
}
/**
 * @function   audio_ssV5_aiCreate
 * @brief    音频输入初始化
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aiCreate(HAL_AIO_ATTR_S *pstAIOAttr)
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
 * @function   audio_ssV5_aiDestroy
 * @brief    音频输入销毁
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
VOID audio_ssV5_aiDestroy(HAL_AIO_ATTR_S *pstAIOAttr)
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
            ss_mpi_ai_disable_chn(pstAIOAttr->u32AudioDevIdx, u32AiChn);
        }
    }
    else
    {
        u32AiChn = 0;
        ss_mpi_ai_disable_chn(pstAIOAttr->u32AudioDevIdx, u32AiChn);
    }

    ss_mpi_ao_disable(pstAIOAttr->u32AudioDevIdx);

    pthread_mutex_destroy(&GetFrame_lock);
}
/**
 * @function   audio_ssV5_aiGetFrame
 * @brief    获取音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aiGetFrame(HAL_AIO_FRAME_S *pstAudioFrame)
{
    td_s32 s32Ret = TD_SUCCESS;
    ot_aec_frame stAecFrm = {0};
    ot_audio_frame stFrame = {0};

    UINT32 u32AudioDevIdx = 0;

    pthread_mutex_lock(&GetFrame_lock);

    if (NULL == pstAudioFrame)
    {
        SAL_LOGE("Parameter is wrong!\n");
        pthread_mutex_unlock(&GetFrame_lock);
        return SAL_FAIL;
    }

    u32AudioDevIdx = pstAudioFrame->AudioDevIdx;

    memset(&stFrame, 0, sizeof(ot_audio_frame));
    memset(&stAecFrm, 0, sizeof(ot_aec_frame));

    /* s32Ret = HI_MPI_AI_GetFrame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame, &stAecFrm, 30); */
    /* s32Ret = HI_MPI_AI_GetFrame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame, &stAecFrm, 0); */
    s32Ret = ss_mpi_ai_get_frame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame, &stAecFrm, 2000);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("HI_MPI_AI_GetFrame(%d, %d), failed with %#x!\n", \
                  u32AudioDevIdx, pstAudioFrame->Channel, s32Ret);
        pthread_mutex_unlock(&GetFrame_lock);
        return SAL_FAIL;
    }

    memcpy(pstAudioFrame->pBufferAddr, stFrame.virt_addr[pstAudioFrame->Channel], stFrame.len);
    pstAudioFrame->FrameLen = stFrame.len;

    s32Ret = ss_mpi_ai_release_frame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame, &stAecFrm);
    if (TD_SUCCESS != s32Ret)
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
 * @function   audio_ssV5_aiSetVolume
 * @brief    设置音频输入音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aiSetVolume(UINT32 u32Channel, UINT32 u32Volume)
{
    INT32 s32AcodecInputVol = 0;
    INT32 s32FdAcodec = -1;

    if (u32Volume > AUDIO_AI_MAX_VOLUME)
    {
        SAL_LOGW("u32Volume=%d > %d,use MaxVolume\n", u32Volume, AUDIO_MAX_VOLUME);
        u32Volume = AUDIO_MAX_VOLUME;
    }

    SAL_LOGI("=========HISI_SetAiVolume:%d=========\n", u32Volume);

    /* HISI_SetAiVolume(u32Volume); */
    s32FdAcodec = open(ACODEC_FILE, O_RDWR);
    if (s32FdAcodec < 0)
    {
        SAL_LOGE("can't open Acodec,%s\n", ACODEC_FILE);
        return TD_FAILURE;
    }

    s32AcodecInputVol = (u32Volume == 0) ? -77 : gAudioAIVolume[u32Volume - 1]; /* ((INT32)(u32Volume*16) - 77) > 86 ? 86 : (u32Volume*16 - 77); */
    SAL_LOGI("InputVol = %d\n", s32AcodecInputVol);
    if (ioctl(s32FdAcodec, OT_ACODEC_SET_INPUT_VOLUME, &s32AcodecInputVol))
    {
        SAL_LOGE("set acodec micin volume failed\n");
        close(s32FdAcodec);
        return TD_FAILURE;
    }

    close(s32FdAcodec);

    return SAL_SOK;
}

/**
 * @function   audio_ssV5_aoSetVolume
 * @brief    设置音频输出音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aoSetVolume(UINT32 u32Channel, UINT32 u32Volume)
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
        return TD_FAILURE;
    }

    s32AcodecInputVol = (u32Volume == 0) ? -121 : gAudioAOVolume[u32Volume - 1]; /* ((INT32)(u32Volume*13) - 121) > 6 ? 6 : (u32Volume*13 - 121); */
    SAL_LOGI("Output Vol = %d\n", s32AcodecInputVol);
    if (ioctl(s32FdAcodec, OT_ACODEC_SET_OUTPUT_VOLUME, &s32AcodecInputVol))
    {
        SAL_LOGE("set acodec micin volume failed\n");
        close(s32FdAcodec);
        return TD_FAILURE;
    }

    close(s32FdAcodec);

    return SAL_SOK;
}
/**
 * @function   audio_ssV5_aiEnableVqe
 * @brief    开启VQE处理(声音质量增强)
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aiEnableVqe(UINT32 u32AudioDevIdx)
{
    td_s32 s32Ret = TD_SUCCESS;

    s32Ret = ss_mpi_ai_enable_vqe(u32AudioDevIdx, 0);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("==========AudioDevIdx %d======= HI_MPI_AI_EnableVqe: %#x ======================\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}
/**
 * @function   audio_ssV5_aiDisableVqe
 * @brief    关闭VQE处理
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aiDisableVqe(UINT32 u32AudioDevIdx)
{
    td_s32 s32Ret = TD_SUCCESS;

    s32Ret = ss_mpi_ai_disable_vqe(u32AudioDevIdx, 0);
    if (TD_SUCCESS != s32Ret)
    {
        SAL_LOGE("==========AudioDevIdx %d======= HI_MPI_AI_EnableVqe: %#x ======================\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#else
/**
 * @function   audio_ssV5_aiSetVolume
 * @brief    设置音频输入音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aiSetVolume(UINT32 u32Channel, UINT32 u32Volume)
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
 * @function   audio_ssV5_aoSetVolume
 * @brief    设置音频输出音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aoSetVolume(UINT32 u32Channel, UINT32 u32Volume)
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
 * @function   audio_ssV5_aiEnableVqe
 * @brief    开启VQE处理
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aiEnableVqe(UINT32 u32AudioDevIdx)
{
    return SAL_SOK;
}
/**
 * @function   audio_ssV5_aiDisableVqe
 * @brief    关闭VQE处理
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ssV5_aiDisableVqe(UINT32 u32AudioDevIdx)
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
    
    pstAudioPlatOps->aoCreate = audio_ssV5_aoCreate;
    pstAudioPlatOps->aoDestroy = audio_ssV5_aoDestroy;
    pstAudioPlatOps->aoSendFrame = audio_ssV5_aoSendFrame;
    pstAudioPlatOps->aoSetVolume = audio_ssV5_aoSetVolume;

    pstAudioPlatOps->aiCreate = audio_ssV5_aiCreate;
    pstAudioPlatOps->aiDestroy = audio_ssV5_aiDestroy;
    pstAudioPlatOps->aiGetFrame = audio_ssV5_aiGetFrame;
    pstAudioPlatOps->aiSetVolume = audio_ssV5_aiSetVolume;
    pstAudioPlatOps->aiEnableVqe = audio_ssV5_aiEnableVqe;
    pstAudioPlatOps->aiDisableVqe = audio_ssV5_aiDisableVqe;
    pstAudioPlatOps->aiSetLoopBack = NULL;

    return SAL_SOK;
}


