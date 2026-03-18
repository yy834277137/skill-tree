/**
 * @file   audio_rk.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---plat层音频接口封装
 * @author sunzelin
 * @date   2022年3月11日 Create
 * @note
 * @note \n History
   1.date        : 2022年3月11日
     Author      : sunzelin
     Modification: 新建文件
 */
 
#include <pthread.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "./tts/hik_tts_hal.h"

#include "sal.h"
#include "platform_sdk.h"
#include "audio_rk.h"
//#include "../tts/kdxf_tts_hal.h"
#include "../ExternalCodec/ExternalCodec.h"
#include "platform_hal.h"

#define ACODEC_FILE "/dev/acodec"

/*****************************************************************************
                               全局变量
*****************************************************************************/
#define AUDIO_AI_MAX_VOLUME (10)
#define AUDIO_AO_MAX_VOLUME (10)
#define AUDIO_MBBLK_BUFSIZE (640)       /* 音频播放送PCM缓存大小 */

//static INT32 gAudioAIVolume[10] = {-40, -25, -10, 0, 10, 25, 40, 55, 70, 86};
//static INT32 gAudioAOVolume[10] = {-56, -30, -10, 0, 1, 2, 3, 4, 5, 6};

typedef struct{
    UINT32 r;
    UINT32 w;
    UINT32 u32DataBufLen;
    UINT8 *pData;
}AUDIO_DATABUF_S;

static UINT32  gGetFrameLen = 0;
static AUDIO_DATABUF_S gstDataBuff = {0};


static pthread_mutex_t SendFrame_lock;
static pthread_mutex_t GetFrame_lock;

static AUDIO_FRAME_S g_stSendAoFrame = {0};

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
    AUDIO_SAMPLE_RATE_E enRkSampleRate = 0;

    switch (u32SampleRate)
    {
        case 8000:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_8000;
            break;
        }
        case 12000:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_12000;
            break;
        }
        case 11025:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_11025;
            break;
        }
        case 16000:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_16000;
            break;
        }
        case 22050:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_22050;
            break;
        }
        case 24000:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_24000;
            break;
        }
        case 32000:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_32000;
            break;
        }
        case 44100:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_44100;
            break;
        }
        case 48000:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_48000;
            break;
        }
        case 64000:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_64000;
            break;
        }
        case 96000:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_96000;
            break;
        }
        default:
        {
            enRkSampleRate = AUDIO_SAMPLE_RATE_8000;
            break;
        }
    }

    return enRkSampleRate;
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
    AUDIO_BIT_WIDTH_E enRkBiteWidth = 0;

    switch (u32BiteWidth)
    {
        case 8:
        {
            enRkBiteWidth = AUDIO_BIT_WIDTH_8;
            break;
        }
        case 16:
        {
            enRkBiteWidth = AUDIO_BIT_WIDTH_16;
            break;
        }
        case 24:
        {
            enRkBiteWidth = AUDIO_BIT_WIDTH_24;
            break;
        }
        default:
        {
            enRkBiteWidth = AUDIO_BIT_WIDTH_16;
            break;
        }
    }

    return enRkBiteWidth;

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
    AUDIO_SOUND_MODE_E enRkSoundMode = 0;

    switch (u32SoundMode)
    {
        case 1:
        {
            enRkSoundMode = AUDIO_SOUND_MODE_MONO;
            break;
        }
        case 2:
        {
            enRkSoundMode = AUDIO_SOUND_MODE_STEREO;
            break;
        }
        default:
        {
            enRkSoundMode = AUDIO_SOUND_MODE_MONO;
            break;
        }
    }

    return enRkSoundMode;

}

/**
 * @function    audio_setSampleRate
 * @brief       设置音频外设采样率
 * @param[in]   UINT32 U32Rate 外设采样率
 * @param[out]  None
 * @return      INT32 音频复合模式
 */
static INT32 audio_setSampleRate(UINT32 U32Rate)
{
    INT32 ret = SAL_SOK; 
    INT32 fd = -1;

    DDM_soundUsrCmd    soundp;
    

    fd = open(DDM_DEV_SOUND_NAME, O_RDWR);
    if (fd < 0)
    {
        SAL_LOGE("open error fd %d errno %d\n",fd,errno);
        return SAL_FAIL;
    }

    memset(&soundp, 0x0, sizeof(DDM_soundUsrCmd));
    
    soundp.args.rateFormat.rate = U32Rate;
    soundp.args.rateFormat.format = SND_SOC_PCM_FORMAT_S16;
    
    ret = ioctl(fd, DDM_IOC_SOUND_SET_FORMAT, &soundp);
    if(ret < 0)
    {
        SAL_LOGE("DDM_IOC_SOUND_SET_ADC_VOLUME failed\n");
        
        close(fd);
        fd = -1;
        return SAL_FAIL;
    }
    
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
    
    return SAL_SOK;
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
    INT32 s32Ret = SAL_FAIL;

    UINT32 u32AudioDevIdx = 0;
    UINT32 u32AoChn = 0;

    AUDIO_SAMPLE_RATE_E enRkSampleRate = 8000;
    AUDIO_BIT_WIDTH_E enRkBiteWidth = AUDIO_BIT_WIDTH_16;
    AUDIO_SOUND_MODE_E enRkSoundMode = 0;

    AIO_ATTR_S stAoAttr = {0};

    if (NULL == pstAIOAttr
		|| pstAIOAttr->u32AudioDevIdx >= AO_DEV_MAX_NUM-1
		|| pstAIOAttr->u32Chan > AO_MAX_CHN_NUM)
    {
        SAL_LOGE("AoCreate input ptr is error! %p, dev %d, chn_cnt %d \n",
			      pstAIOAttr, (NULL != pstAIOAttr) ? pstAIOAttr->u32AudioDevIdx : 0, (NULL != pstAIOAttr) ? pstAIOAttr->u32Chan : 0);
        return SAL_FAIL;
    }

    u32AudioDevIdx = 0;
        
    enRkSampleRate = audio_sampleRateParse(pstAIOAttr->u32SampleRate);
    enRkBiteWidth = audio_biteWidthParse(pstAIOAttr->u32BitWidth);
    enRkSoundMode = audio_soundModeParse(pstAIOAttr->u32SoundMode);
    
    audio_setSampleRate(pstAIOAttr->u32SampleRate);
    
    memset(&stAoAttr, 0, sizeof(AIO_ATTR_S));

	/* fixme: 此处参考sdk demo配置的默认声卡配置 */
	stAoAttr.soundCard.bitWidth = AUDIO_BIT_WIDTH_16;
	stAoAttr.soundCard.channels = 2;
	stAoAttr.soundCard.sampleRate = pstAIOAttr->u32SampleRate;

    stAoAttr.u32ChnCnt = pstAIOAttr->u32Chan;
    stAoAttr.enBitwidth = enRkBiteWidth;
    stAoAttr.enSamplerate = enRkSampleRate;
    stAoAttr.u32PtNumPerFrm = enRkSampleRate / pstAIOAttr->u32FrameRate;
    stAoAttr.enSoundmode = enRkSoundMode;
    stAoAttr.u32EXFlag = 0;
    stAoAttr.u32FrmNum = 4;         /* 30 */
	strcpy((RK_CHAR*)stAoAttr.u8CardName,"hw:0,0"); 
    s32Ret = RK_MPI_AO_SetPubAttr(u32AudioDevIdx, &stAoAttr);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_AO_SetPubAttr %d attr err: %#x\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_AO_Enable(u32AudioDevIdx);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_AO_Enable %d err: %#x\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

    for (u32AoChn = 0; u32AoChn < stAoAttr.u32ChnCnt; u32AoChn++)
    {
        s32Ret = RK_MPI_AO_EnableChn(u32AudioDevIdx, u32AoChn);
        if (RK_SUCCESS != s32Ret)
        {
            SAL_LOGE("RK_MPI_AO_EnableChn %d:%d attr err: %#x\n", u32AudioDevIdx, u32AoChn, s32Ret);
            RK_MPI_AO_Disable(u32AudioDevIdx);
            return SAL_FAIL;
        }

        s32Ret = RK_MPI_AO_EnableReSmp(u32AudioDevIdx, u32AoChn,
                                  (AUDIO_SAMPLE_RATE_E)stAoAttr.enSamplerate);
        if (RK_SUCCESS != s32Ret)
        {
            SAL_LOGE("RK_MPI_AO_EnableReSmp %d:%d attr err: %#x\n", u32AudioDevIdx, u32AoChn, s32Ret);
            RK_MPI_AO_Disable(u32AudioDevIdx);
            return SAL_FAIL;
        }
    }
    
    SAL_LOGI("dev_%d ao set SampleRate %d per get frame len %d\n", u32AudioDevIdx, enRkSampleRate, gGetFrameLen);

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

    AUDIO_SAMPLE_RATE_E enRkSampleRate = 0;
    AUDIO_BIT_WIDTH_E enRkBiteWidth = 0;
    AUDIO_SOUND_MODE_E enRkSoundMode = 0;

    if (NULL == pstAIOAttr
        || pstAIOAttr->u32AudioDevIdx >= AI_DEV_MAX_NUM
        || pstAIOAttr->u32Chan > AI_MAX_CHN_NUM)
    {
        SAL_LOGE("invalid input! %p, dev %d, chn_cnt %d \n", 
             pstAIOAttr, (NULL != pstAIOAttr) ? pstAIOAttr->u32AudioDevIdx : 0, (NULL != pstAIOAttr) ? pstAIOAttr->u32Chan : 0);
        return SAL_FAIL;
    }

    enRkSampleRate = audio_sampleRateParse(pstAIOAttr->u32SampleRate);
    enRkBiteWidth = audio_biteWidthParse(pstAIOAttr->u32BitWidth);
    enRkSoundMode = audio_soundModeParse(pstAIOAttr->u32SoundMode);
    
    audio_setSampleRate(pstAIOAttr->u32SampleRate);

    memset(&stAiAttr, 0, sizeof(AIO_ATTR_S));
    u32AudioDevIdx = pstAIOAttr->u32AudioDevIdx;

    gGetFrameLen = pstAIOAttr->u32SampleRate * pstAIOAttr->u32BitWidth / pstAIOAttr->u32FrameRate / 8;

    gstDataBuff.u32DataBufLen = 10 * 1024;
    if (gstDataBuff.pData == NULL)
    {
        gstDataBuff.pData = SAL_memMalloc(gstDataBuff.u32DataBufLen, "audio_rk", "ai");
        if (NULL == gstDataBuff.pData)
        {
            SAL_LOGE("alloc pData buff len %d fail \n",gstDataBuff.u32DataBufLen);
            return SAL_FAIL;
        }
    }

    memset(gstDataBuff.pData , 0x00, gstDataBuff.u32DataBufLen);
    gstDataBuff.r = 0;
    gstDataBuff.w = 0;
    
    //stAiAttr.u32ChnCnt = pstAIOAttr->u32Chan;
    stAiAttr.u32ChnCnt = 1;
    stAiAttr.enBitwidth = enRkBiteWidth;
    stAiAttr.enSamplerate = enRkSampleRate;
    
    stAiAttr.u32PtNumPerFrm = enRkSampleRate / 25;
    stAiAttr.enSoundmode = enRkSoundMode;
    stAiAttr.u32EXFlag = 0;
    stAiAttr.u32FrmNum = 4;         /* 4 */

	strcpy((RK_CHAR*)stAiAttr.u8CardName,"hw:0,0"); 
	stAiAttr.soundCard.channels = 2;
	stAiAttr.soundCard.sampleRate = pstAIOAttr->u32SampleRate;
	stAiAttr.soundCard.bitWidth = AUDIO_BIT_WIDTH_16;
    
    SAL_LOGI("dev_%d ai set SampleRate %d per get frame len %d\n", u32AudioDevIdx, enRkSampleRate, gGetFrameLen);

	/* 閰嶇疆鍏ㄥ眬鍙傛暟锛屽Audio Input鐨勯?氶亾鏁? 浣嶅 閲囨牱鐜囩瓑鍙傛暟 */
    s32Ret = RK_MPI_AI_SetPubAttr(u32AudioDevIdx, &stAiAttr);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_AI_SetPubAttr dev_%d u32ChnCnt_%d attr err: %#x\n", u32AudioDevIdx, stAiAttr.u32ChnCnt, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_AI_Enable(u32AudioDevIdx);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_AI_Enable %d err: %#x\n", u32AudioDevIdx, s32Ret);
        return SAL_FAIL;
    }

	/* 寮?鍚敤鎴峰眰涓嬪彂鐨刟i閫氶亾缁? */
    for (u32AiChn = 0; u32AiChn < stAiAttr.u32ChnCnt; u32AiChn++)
    {
        s32Ret = RK_MPI_AI_EnableChn(u32AudioDevIdx, u32AiChn);
        if (RK_SUCCESS != s32Ret)
        {
            SAL_LOGE("RK_MPI_AI_EnableChn %d:%d attr err: %#x\n", u32AudioDevIdx, u32AiChn, s32Ret);
            RK_MPI_AI_Disable(u32AudioDevIdx);
            return SAL_FAIL;
        }
		s32Ret = RK_MPI_AI_EnableReSmp(u32AudioDevIdx, u32AiChn,(AUDIO_SAMPLE_RATE_E)stAiAttr.enSamplerate);
		if (s32Ret != 0)
		{
			SAL_LOGE("RK_MPI_AI_EnableReSmp %d:%d attr err: %#x\n", u32AudioDevIdx, u32AiChn, s32Ret);
        
        	return SAL_FAIL;
		}
    }

    s32Ret = RK_MPI_AI_GetChnParam(u32AudioDevIdx, 0, &stAiChnPara);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_AI_GetChnParam failed, ret: 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    stAiChnPara.u32UsrFrmDepth = 6;   /* 閰嶇疆闊抽杈撳叆缂撳瓨娣卞害涓?6 */

    s32Ret = RK_MPI_AI_SetChnParam(u32AudioDevIdx, 0, &stAiChnPara);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_AI_SetChnParam failed, ret: 0x%x \n", s32Ret);
        return SAL_FAIL;
    }
	
    return SAL_SOK;
}


#if 0
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
#endif

/**
 * @function   audio_init_AoFrameInfo
 * @brief    音频输出MB_BLK初始化
 * @param[in] None
 * @param[out] None
 * @return     int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_init_AoFrameInfo(void)
{
    INT32 s32Ret = SAL_FAIL;
    g_stSendAoFrame.u64TimeStamp = 0;
    s32Ret = RK_MPI_SYS_MmzAlloc(&g_stSendAoFrame.pMbBlk, NULL, NULL, AUDIO_MBBLK_BUFSIZE);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_MPI_SYS_MmzAlloc failed! ret: 0x%x \n", s32Ret);
        return SAL_FAIL;
    }
#if 0
	/* 帧内存静态参数 */
    g_stSendAoFrame.u32Len = 640;
    g_stSendAoFrame.enBitWidth = AUDIO_BIT_WIDTH_16;
    g_stSendAoFrame.enSoundMode = AUDIO_SOUND_MODE_MONO;
#endif
    return SAL_SOK;
}

/**
 * @function   audio_rk_aoCreate
 * @brief    音频输出初始化
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_rk_aoCreate(HAL_AIO_ATTR_S *pstAIOAttr)
{
    INT32 s32Ret = SAL_FAIL;

#if 0  /* 平台不支持 */
    /* 配置内部codec */
    if (SAL_SOK != audio_i2sInit(pstAIOAttr->u32SampleRate))
    {
        SAL_LOGE("Hal_acodecInit !!!\n");
        return SAL_FAIL;
    }
#endif
    s32Ret = SND_userDrvInit();
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Auido user device init error!\n");
        return SAL_FAIL;
    }

    s32Ret = audio_aoCreate(pstAIOAttr);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("AO Create error!\n");
        return SAL_FAIL;
    }

	/* 初始化送ao的帧内存，全局变量g_stSendAoFrame */
	s32Ret = audio_init_AoFrameInfo();
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("init ao frame info failed! \n");
		return SAL_FAIL;
	}

    pthread_mutex_init(&SendFrame_lock, NULL);

    return SAL_SOK;
}

/**
 * @function   audio_rk_aoDestroy
 * @brief    销毁音频输出
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
VOID audio_rk_aoDestroy(HAL_AIO_ATTR_S *pstAIOAttr)
{
    INT32 s32Ret = RK_FAILURE;
    UINT32 u32AoChn = 0;

    if (NULL == pstAIOAttr)
    {
        SAL_LOGE("AoDestroy input ptr is error!\n");
        return;
    }

    if (pstAIOAttr->u32AudioDevIdx >= AO_DEV_MAX_NUM)
    {
        SAL_LOGW("ao: dev id %d > max %d, use max. \n", pstAIOAttr->u32AudioDevIdx, AO_DEV_MAX_NUM - 1);

        pstAIOAttr->u32AudioDevIdx = (AO_DEV_MAX_NUM - 1);
    }

    if (pstAIOAttr->u32Chan > AO_MAX_CHN_NUM)
    {
        SAL_LOGW("ao: chn cnt %d > max %d, use max. \n", pstAIOAttr->u32Chan, AO_MAX_CHN_NUM);

        pstAIOAttr->u32Chan = AO_MAX_CHN_NUM;
    }

    /* 准备销毁音频输出，先销毁锁，阻止送音频帧使用设备 */
    pthread_mutex_destroy(&SendFrame_lock);

    for (u32AoChn = 0; u32AoChn < pstAIOAttr->u32Chan; u32AoChn++)
    {
        s32Ret = RK_MPI_AO_DisableChn(pstAIOAttr->u32AudioDevIdx, u32AoChn);
        if (s32Ret != RK_SUCCESS)
        {
            SAL_LOGE("ao: disable Dev %d chan %d failed, s32Ret 0x%0x", pstAIOAttr->u32AudioDevIdx, u32AoChn, s32Ret);
            return;
        }
    }

    s32Ret = RK_MPI_AO_Disable(pstAIOAttr->u32AudioDevIdx);
    if (s32Ret != RK_SUCCESS)
    {
        SAL_LOGE("ao: disable Dev %d failed, s32Ret 0x%0x", pstAIOAttr->u32AudioDevIdx, s32Ret);
        return;
    }

    /* 用户态驱动反初始化 */
    // SND_userDrvDeinit();

    /* 释放音频输出MB_BLK */
    s32Ret = RK_MPI_SYS_MmzFree(g_stSendAoFrame.pMbBlk);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_SYS_MmzAlloc failed! ret: 0x%x \n", s32Ret);
        return;
    }

    return;
}

/**
 * @function   audio_rk_aoSendFrame
 * @brief    输出音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_rk_aoSendFrame(HAL_AIO_FRAME_S *pstAudioFrame)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32UsedLen = 0;
    UINT32 u32FrameRsetLen = 0;
    void *pData = NULL;

    if ((NULL == pstAudioFrame) || (NULL == pstAudioFrame->pBufferAddr) || (0 >= pstAudioFrame->FrameLen))
    {
        SAL_LOGE("Parameter is wrong! %p, %p FrameLen:%u\n", pstAudioFrame, (NULL == pstAudioFrame) ? NULL : pstAudioFrame->pBufferAddr, (NULL == pstAudioFrame) ? 0 : pstAudioFrame->FrameLen);
        goto exit;
    }

    pthread_mutex_lock(&SendFrame_lock);

    u32FrameRsetLen = pstAudioFrame->FrameLen;
    g_stSendAoFrame.bBypassMbBlk = SAL_FALSE;           /* 需要将外部MB_BLK的信息拷贝到内部 */
    g_stSendAoFrame.enBitWidth = AUDIO_BIT_WIDTH_16;
    g_stSendAoFrame.enSoundMode = AUDIO_SOUND_MODE_MONO;
    g_stSendAoFrame.u32Seq = 0;                         /* fixme: 暂未填充 */

    pData = RK_MPI_MB_Handle2VirAddr(g_stSendAoFrame.pMbBlk);
    if (!pData)
    {
        SAL_LOGE("get vir from mb blk failed! \n");
        goto unlock;
    }

    while (u32FrameRsetLen > 0)
    {
        g_stSendAoFrame.u64TimeStamp = g_stSendAoFrame.u64TimeStamp + 1; /* fixme: 暂未填充 */
        g_stSendAoFrame.u32Len = u32FrameRsetLen > AUDIO_MBBLK_BUFSIZE ? AUDIO_MBBLK_BUFSIZE : u32FrameRsetLen;
        memcpy(pData, pstAudioFrame->pBufferAddr + u32UsedLen, g_stSendAoFrame.u32Len);
        u32UsedLen += g_stSendAoFrame.u32Len;
        u32FrameRsetLen -= g_stSendAoFrame.u32Len;

        s32Ret = RK_MPI_AO_SendFrame(pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel, &g_stSendAoFrame, 500);
        if (RK_SUCCESS != s32Ret)
        {
            SAL_LOGE("RK_MPI_AO_SendFrame(%d,%d) err %#x\n", pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel, s32Ret);
            goto unlock;
        }
    }

unlock:
    pthread_mutex_unlock(&SendFrame_lock);

exit:
    return s32Ret;
}

/**
 * @function   audio_hisi_aiCreate
 * @brief    音频输入初始化
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_rk_aiCreate(HAL_AIO_ATTR_S *pstAIOAttr)
{
    INT32 s32Ret = SAL_FAIL;

    if (NULL == pstAIOAttr)
    {
        SAL_LOGE("Input parameter pstAIOAttr is NULL!!!\n");
        return SAL_FAIL;
    }

    s32Ret = audio_aiCreate(pstAIOAttr);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("Ai Create error!\n");
        return SAL_FAIL;
    }

    pthread_mutex_init(&GetFrame_lock, NULL);

    return SAL_SOK;
}

/**
 * @function   audio_hisi_aiSetVolume
 * @brief    设置音频输入音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_rk_aiSetVolume(UINT32 u32Channel, UINT32 u32Volume)
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
 * @function   audio_rk_aoSetVolume
 * @brief    设置音频输出音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_rk_aoSetVolume(UINT32 u32Channel, UINT32 u32Volume)
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
 * @function   audio_rk_aiDestroy
 * @brief    音频输入销毁
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
VOID audio_rk_aiDestroy(HAL_AIO_ATTR_S *pstAIOAttr)
{
    INT32 s32Ret = RK_FAILURE;
    UINT32 u32AiChn = 0;

    if (NULL == pstAIOAttr)
    {
        SAL_LOGE("AiDestroy input ptr is error!\n");
        return;
    }

    if (pstAIOAttr->u32AudioDevIdx >= AI_DEV_MAX_NUM)
    {
        SAL_LOGW("ai: dev id %d > max %d, use max. \n", pstAIOAttr->u32AudioDevIdx, AI_DEV_MAX_NUM - 1);

        pstAIOAttr->u32AudioDevIdx = (AI_DEV_MAX_NUM - 1);
    }

    if (pstAIOAttr->u32Chan > AI_MAX_CHN_NUM)
    {
        SAL_LOGW("ai: chn cnt %d > max %d, use max. \n", pstAIOAttr->u32Chan, AI_MAX_CHN_NUM);

        pstAIOAttr->u32Chan = AI_MAX_CHN_NUM;
    }

    /* 准备销毁音频输入，先销毁锁，阻止获取音频帧使用设备 */
    pthread_mutex_destroy(&GetFrame_lock);

    if (gstDataBuff.pData != NULL)
    {
        SAL_memfree(gstDataBuff.pData, "audio_rk", "ai");
        gstDataBuff.pData = NULL;
    }

    for (u32AiChn = 0; u32AiChn < pstAIOAttr->u32Chan; u32AiChn++)
    {
        s32Ret = RK_MPI_AI_DisableChn(pstAIOAttr->u32AudioDevIdx, u32AiChn);
        if (s32Ret != RK_SUCCESS)
        {
            SAL_LOGE("ai: disable Dev %d chan %d failed, s32Ret 0x%0x", pstAIOAttr->u32AudioDevIdx, u32AiChn, s32Ret);
            return;
        }
    }

    s32Ret = RK_MPI_AO_Disable(pstAIOAttr->u32AudioDevIdx);
    if (s32Ret != RK_SUCCESS)
    {
        SAL_LOGE("ai: disable Dev %d, s32Ret 0x%0x", pstAIOAttr->u32AudioDevIdx, s32Ret);
        return;
    }

    return;
}

/**
 * @function   audio_rk_aiGetFrame
 * @brief    获取音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_rk_aiGetFrame(HAL_AIO_FRAME_S *pstAudioFrame)
{
    RK_S32 s32Ret = RK_FAILURE;
    AUDIO_FRAME_S stFrame = {0};
    UINT32 u32AudioDevIdx = 0;
    void *pData = NULL;
    UINT32 u32DataLen = 0;
    UINT32 u32DataBufLen = 0;
    UINT32 u32Part0 = 0;
    UINT32 u32Part1 = 0;
    UINT32 u32ValDataLen = 0;

    if (NULL == pstAudioFrame
        || pstAudioFrame->AudioDevIdx >= AI_DEV_MAX_NUM
        || pstAudioFrame->Channel >= AI_MAX_CHN_NUM)
    {
        SAL_LOGE("Parameter is wrong! %p, dev %d, chn %d \n", pstAudioFrame, 
            (NULL != pstAudioFrame) ? pstAudioFrame->AudioDevIdx : 0, (NULL != pstAudioFrame) ? pstAudioFrame->Channel : 0);
        return SAL_FAIL;
    }
        
    u32DataBufLen = gstDataBuff.u32DataBufLen;

    pthread_mutex_lock(&GetFrame_lock);
    
    u32ValDataLen = (gstDataBuff.w >= gstDataBuff.r) ? (gstDataBuff.w - gstDataBuff.r) \
                                : (gstDataBuff.u32DataBufLen + gstDataBuff.w - gstDataBuff.r);

    while (u32ValDataLen < gGetFrameLen)
    {
        s32Ret = RK_MPI_AI_GetFrame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame,NULL, -1);
        if (RK_SUCCESS != s32Ret)
        {
            SAL_LOGE("RK_MPI_AI_GetFrame(%d, %d), failed with 0x%x!\n", \
            u32AudioDevIdx, pstAudioFrame->Channel, s32Ret);
            
            pthread_mutex_unlock(&GetFrame_lock);
            return SAL_FAIL;
        }

        pData = RK_MPI_MB_Handle2VirAddr(stFrame.pMbBlk);
        if (!pData)
        {
            SAL_LOGE("RK_MPI_MB_Handle2VirAddr failed! \n");
            
            s32Ret = RK_MPI_AI_ReleaseFrame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame, NULL);
            if (RK_SUCCESS != s32Ret)
            {
                SAL_LOGE("MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n", \
                u32AudioDevIdx, pstAudioFrame->Channel, s32Ret);
                pthread_mutex_unlock(&GetFrame_lock);
                return SAL_FAIL;
            }
            
            pthread_mutex_unlock(&GetFrame_lock);
            return SAL_FAIL;
        }
        
        u32DataLen = stFrame.u32Len;

        u32Part0 = u32DataBufLen - gstDataBuff.w;
        if (u32DataLen > u32Part0)
        {
            memcpy(gstDataBuff.pData + gstDataBuff.w , (void *)pData, u32Part0);
            u32Part1 = u32DataLen - u32Part0;
            
            memcpy(gstDataBuff.pData , (void *)pData + u32Part0, u32Part1);
        }
        else
        {
            memcpy(gstDataBuff.pData + gstDataBuff.w , (void *)pData, u32DataLen);
        }
        
        gstDataBuff.w = (gstDataBuff.w + u32DataLen) % u32DataBufLen;
        
        s32Ret = RK_MPI_AI_ReleaseFrame(u32AudioDevIdx, pstAudioFrame->Channel, &stFrame, NULL);
        if (RK_SUCCESS != s32Ret)
        {
            SAL_LOGE("MPI_AI_ReleaseFrame(%d, %d), failed with %#x!\n", \
            u32AudioDevIdx, pstAudioFrame->Channel, s32Ret);
            
            pthread_mutex_unlock(&GetFrame_lock);
            return SAL_FAIL;
        }
        
        u32ValDataLen = (gstDataBuff.w >= gstDataBuff.r) ? (gstDataBuff.w - gstDataBuff.r) \
                                    : (gstDataBuff.u32DataBufLen + gstDataBuff.w - gstDataBuff.r);

    }


    u32Part0 = 0;
    u32Part1 = 0;

    if (gstDataBuff.r > gstDataBuff.w)
    {
        u32Part0 = u32DataBufLen - gstDataBuff.r;
        
        if (u32Part0 < gGetFrameLen)
        {
            memcpy(pstAudioFrame->pBufferAddr, (void *)gstDataBuff.pData + gstDataBuff.r, u32Part0);
            
            u32Part1 = gGetFrameLen - u32Part0;
            memcpy(pstAudioFrame->pBufferAddr + u32Part0, (void *)gstDataBuff.pData, u32Part1);
        }
        else
        {
            memcpy(pstAudioFrame->pBufferAddr, (void *)gstDataBuff.pData + gstDataBuff.r, gGetFrameLen);
        }

    }
    else
    {
        memcpy(pstAudioFrame->pBufferAddr, (void *)gstDataBuff.pData + gstDataBuff.r, gGetFrameLen);
    }
    
    gstDataBuff.r = ( gstDataBuff.r + gGetFrameLen) % u32DataBufLen;
    
    pstAudioFrame->FrameLen = gGetFrameLen;
    
    pthread_mutex_unlock(&GetFrame_lock);

    return SAL_SOK;
}


#if 0  /* RK平台不支持音量配置 */
/**
 * @function   audio_rk_aiSetVolume
 * @brief    设置音频输入音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_rk_aiSetVolume(UINT32 u32Channel, UINT32 u32Volume)
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
 * @function   audio_rk_aoSetVolume
 * @brief    设置音频输出音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_rk_aoSetVolume(UINT32 u32Channel, UINT32 u32Volume)
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
 * @function   audio_rk_aiEnableVqe
 * @brief    开启VQE处理(声音质量增强)
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_rk_aiEnableVqe(UINT32 u32AudioDevIdx)
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
 * @function   audio_rk_aiDisableVqe
 * @brief    关闭VQE处理
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_rk_aiDisableVqe(UINT32 u32AudioDevIdx)
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
    
    pstAudioPlatOps->aoCreate = audio_rk_aoCreate;
    pstAudioPlatOps->aoDestroy = audio_rk_aoDestroy;
    pstAudioPlatOps->aoSendFrame = audio_rk_aoSendFrame;
    pstAudioPlatOps->aoSetVolume = audio_rk_aoSetVolume;  //设置音频输出音量

    pstAudioPlatOps->aiCreate = audio_rk_aiCreate;
    pstAudioPlatOps->aiDestroy = audio_rk_aiDestroy;
    pstAudioPlatOps->aiGetFrame = audio_rk_aiGetFrame;
    pstAudioPlatOps->aiSetVolume = audio_rk_aiSetVolume;//audio_rk_aiSetVolume; //sdk不支持
    pstAudioPlatOps->aiEnableVqe = NULL;//audio_rk_aiEnableVqe; //sdk不支持
    pstAudioPlatOps->aiDisableVqe = NULL;//audio_rk_aiDisableVqe; //sdk不支持
    pstAudioPlatOps->aiSetLoopBack = NULL;

    /* todo: tts, 联系研究院 梁文杰5 获取支持RK3588的tts库 */
#if 1
    pstAudioPlatOps->ttsGetSampleRate = hik_tts_getSampleRate;
    pstAudioPlatOps->ttsStart = hik_tts_start;
    pstAudioPlatOps->ttsProc = hik_tts_proc;
    pstAudioPlatOps->ttsStop = hik_tts_stop;
#endif

    return SAL_SOK;
}


