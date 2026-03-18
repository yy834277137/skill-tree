/**
 * @file   audio_ntv3.c
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
#include "audio_ntv3.h"
#include "../tts/hik_tts_hal.h"
#include "../volume/hik_volume_hal.h"

/*****************************************************************************
                               宏定义
*****************************************************************************/
/*环通测试**/
/* #define LIVE_SOUND */

#line __LINE__ "audio_ntv3.c"


#ifndef LIVE_SOUND
/*ai 必须绑定 enc*/
#define AUDIO_AI_BIND_CODEC

/*ao 可以不绑定dec，不绑定是非阻塞模式*/
#define AUDIO_AO_BIND_CODEC

#ifdef AUDIO_AI_BIND_CODEC
/*ai取帧方式有两种，pull和recv list，PULL map buf太频繁，不建议使用*/
/* #define AUDIO_ENC_PULL */
#endif
/*dev内核已经配置好*/
/* #define AUDIO_INIT_DEV */

/*AO的音量调节走SDK接口*/
/* #define AUDIO_AO_VOLUME_SOFT */

/*SDK不支持AI音量调节，用alc库*/
#define AUDIO_AI_VOLUME_SOFT

#endif

#define MAX_AUDIO_DEV_NUM	(1)
#define MAX_AUDIO_CHN_NUM	(HD_AUDIO_MAX_CH)
#define DEF_AUDIO_FRM_LEN	(640)
#define AUDIO_DDR_ID		(0)

/*****************************************************************************
                               数据结构
*****************************************************************************/
typedef struct
{
    UINT32 u32BufSize;
    HD_COMMON_MEM_DDR_ID eDdrId;
    HD_COMMON_MEM_VB_BLK hdBlk;
    UINTPTR phyAddr;
    char *pVirAddr;
} NTV3_BIT_STREAM_ST;

typedef struct _AUDIO_LIVESOUND_
{
    UINT32 u32Dev;
    UINT32 u32Chn;

    /*audio in/out*/
    UINT32 u32FrmRate;
    UINT32 u32FrmLen;
    HD_AUDIO_SR hdSampleRate;
    HD_AUDIO_BIT_WIDTH hdBitWidth;
    HD_AUDIO_SOUND_MODE hdSoundMode;

    HD_PATH_ID audioIoPath;
    /* audio encode/decode */
    HD_PATH_ID audioCodecPath;

    /*ai/o stream buf*/
    NTV3_BIT_STREAM_ST stBitStreamData;
    UINT64 u64FrmTime; /*ao输出控制*/

    void *pVolumeHdl; /*ai音量调节hdl*/
} AUDIO_LIVESOUND_ST;

typedef struct _tagAudioDev_
{
    UINT32 isInit;
    HD_PATH_ID audioDevPath;
    AUDIO_LIVESOUND_ST g_stAudioChan[MAX_AUDIO_CHN_NUM]; /*dev和chn的关联需要整理下*/
} NTV3_AUDIO_DEV_ST;

/*****************************************************************************
                               全局变量
*****************************************************************************/
#define AUDIO_AI_MAX_VOLUME_LEV (10)
#define AUDIO_AO_MAX_VOLUME_LEV (11)

static pthread_mutex_t SendFrame_lock;
static pthread_mutex_t GetFrame_lock;

static NTV3_AUDIO_DEV_ST g_stAudioIn[MAX_AUDIO_DEV_NUM] = {0};
static NTV3_AUDIO_DEV_ST g_stAudioOut[MAX_AUDIO_DEV_NUM] = {0};

#ifndef AUDIO_AO_VOLUME_SOFT
static UINT32 u32AoVolume[AUDIO_AO_MAX_VOLUME_LEV] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100}; /* 取值[10,100],10为-24dB，100为30dB，50为0dB */
#endif

#ifndef AUDIO_AI_VOLUME_SOFT
static UINT32 u32AiVolume[AUDIO_AI_MAX_VOLUME_LEV] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100}; /* sdk 不支持ai音量调节 */
#endif

/**
 * @function    audio_mapBuf
 * @brief       地址映射
 * @param[in]   NTV3_BIT_STREAM_ST *pstBuf
 * @param[out]
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_mapBuf(NTV3_BIT_STREAM_ST *pstBuf)
{
    if ((NULL == pstBuf) || (0 == pstBuf->u32BufSize))
    {
        SAL_LOGE("Parameter is wrong!\n");
        return SAL_FAIL;
    }

    pstBuf->hdBlk = hd_common_mem_get_block(HD_COMMON_MEM_USER_BLK, pstBuf->u32BufSize, pstBuf->eDdrId);
    if (HD_COMMON_MEM_VB_INVALID_BLK == pstBuf->hdBlk)
    {
        SAL_LOGE("hd_common_mem_get_block fail\r\n");
        goto exit;
    }

    pstBuf->phyAddr = hd_common_mem_blk2pa(pstBuf->hdBlk);
    if (0 == pstBuf->phyAddr)
    {
        SAL_LOGE("hd_common_mem_blk2pa fail, pcm_blk = %#lx\r\n", pstBuf->hdBlk);
        hd_common_mem_release_block(pstBuf->hdBlk);
        goto exit;
    }

    pstBuf->pVirAddr = (char *)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, pstBuf->phyAddr, pstBuf->u32BufSize);
    if (NULL == pstBuf->pVirAddr)
    {
        SAL_LOGE("hd_common_mem_mmap fail, pcm_blk = %#lx\r\n", pstBuf->hdBlk);
        hd_common_mem_release_block(pstBuf->hdBlk);
        goto exit;
    }

    memset(pstBuf->pVirAddr, 0, pstBuf->u32BufSize);
    return SAL_SOK;

exit:
    return SAL_FAIL;
}

/**
 * @function    audio_unMapBuf
 * @brief       ADDR UNMAP
 * @param[in]   NTV3_BIT_STREAM_ST *pstBuf
 * @param[out]
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_unMapBuf(NTV3_BIT_STREAM_ST *pstBuf)
{
    HD_RESULT hdRet = HD_OK;

    if ((NULL == pstBuf)
        || (NULL == pstBuf->pVirAddr)
        || (0 == pstBuf->u32BufSize))
    {
        SAL_LOGE("Parameter is wrong!\n");
        return SAL_FAIL;
    }

    hdRet = hd_common_mem_munmap(pstBuf->pVirAddr, pstBuf->u32BufSize);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_common_mem_munmap fail %d\n", hdRet);
        goto exit;
    }

    hdRet = hd_common_mem_release_block(pstBuf->hdBlk);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_common_mem_release_block fail %d\n", hdRet);
        goto exit;
    }

    return SAL_SOK;
exit:
    return SAL_FAIL;
}

/**
 * @function   audio_sampleRateParse
 * @brief    计算采样率
 * @param[in] UINT32 u32SampleRate 采样率
 * @param[out] None
 * @return      INT32 采样率
 */
static HD_AUDIO_SR audio_sampleRateParse(UINT32 u32SampleRate)
{
    HD_AUDIO_SR enSampleRate = 0;

    switch (u32SampleRate)
    {
        case 8000:
        {
            enSampleRate = HD_AUDIO_SR_8000;
            break;
        }
        case 12000:
        {
            enSampleRate = HD_AUDIO_SR_12000;
            break;
        }
        case 11025:
        {
            enSampleRate = HD_AUDIO_SR_11025;
            break;
        }
        case 16000:
        {
            enSampleRate = HD_AUDIO_SR_16000;
            break;
        }
        case 22050:
        {
            enSampleRate = HD_AUDIO_SR_22050;
            break;
        }
        case 24000:
        {
            enSampleRate = HD_AUDIO_SR_24000;
            break;
        }
        case 32000:
        {
            enSampleRate = HD_AUDIO_SR_32000;
            break;
        }
        case 44100:
        {
            enSampleRate = HD_AUDIO_SR_44100;
            break;
        }
        case 48000:
        {
            enSampleRate = HD_AUDIO_SR_48000;
            break;
        }
        default:
        {
            enSampleRate = HD_AUDIO_SR_8000;
            break;
        }
    }

    return enSampleRate;
}

/**
 * @function   audio_bitWidthParse
 * @brief    计算每个采样点的位宽
 * @param[in] UINT32 u32BiteWidth 位宽信息
 * @param[out] None
 * @return      INT32 位宽
 */
static HD_AUDIO_BIT_WIDTH audio_bitWidthParse(UINT32 u32BiteWidth)
{
    HD_AUDIO_BIT_WIDTH enBitWidth = 0;

    switch (u32BiteWidth)
    {
        case 8:
        {
            enBitWidth = HD_AUDIO_BIT_WIDTH_8;
            break;
        }
        case 16:
        {
            enBitWidth = HD_AUDIO_BIT_WIDTH_16;
            break;
        }
        default:
        {
            enBitWidth = HD_AUDIO_BIT_WIDTH_16;
            break;
        }
    }

    return enBitWidth;

}

/**
 * @function   audio_soundModeParse
 * @brief    获取音频复合模式单，双，静音
 * @param[in] UINT32 SoundMode 模式信息
 * @param[out] None
 * @return      HD_AUDIO_SOUND_MODE 音频复合模式
 */
static HD_AUDIO_SOUND_MODE audio_soundModeParse(UINT32 u32SoundMode)
{
    HD_AUDIO_SOUND_MODE enSoundMode = 0;

    switch (u32SoundMode)
    {
        case 1:
        {
            enSoundMode = HD_AUDIO_SOUND_MODE_MONO;
            break;
        }
        case 2:
        {
            enSoundMode = HD_AUDIO_SOUND_MODE_STEREO;
            break;
        }
        case 3:
        {
            enSoundMode = HD_AUDIO_SOUND_MODE_STEREO_PLANAR;
            break;
        }
        default:
        {
            enSoundMode = HD_AUDIO_SOUND_MODE_MONO;
            break;
        }
    }

    return enSoundMode;

}

/**
 * @function   audio_aoCreate
 * @brief    配置音频输出参数
 * @param[in] AUDIO_LIVESOUND_ST *pstLiveSound 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_aoCreate(AUDIO_LIVESOUND_ST *pstLiveSound)
{
    HD_RESULT hdRet = HD_OK;
    HD_AUDIOOUT_IN stAudioOutInPrm = {0};
    HD_AUDIOOUT_OUT stAudioOutPrm = {0};

#ifdef AUDIO_AO_BIND_CODEC
    HD_COMMON_MEM_DDR_ID eDdrId = AUDIO_DDR_ID;
    HD_AUDIODEC_PATH_CONFIG stAudioDecPathCfg = {0};
    HD_AUDIODEC_IN stAudioDecInPrm = {0};
#endif

    if (NULL == pstLiveSound)
    {
        SAL_LOGE("null prm\n");
        return SAL_FAIL;
    }

    hdRet = hd_audioout_open(HD_AUDIOOUT_IN(pstLiveSound->u32Dev, 0),
                             HD_AUDIOOUT_OUT(pstLiveSound->u32Dev, pstLiveSound->u32Chn),
                             &pstLiveSound->audioIoPath);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioout_open fail,%d dev %d chn %d\n", hdRet, pstLiveSound->u32Dev, pstLiveSound->u32Chn);
        return SAL_FAIL;
    }

    /*
     * Set data sample rate
     */
    stAudioOutInPrm.sample_rate = pstLiveSound->hdSampleRate;
    hdRet = hd_audioout_set(pstLiveSound->audioIoPath, HD_AUDIOOUT_PARAM_IN, &stAudioOutInPrm);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioout_set(HD_AUDIOOUT_PARAM_IN) fail %d\n", hdRet);
        return SAL_FAIL;
    }

    hdRet = hd_audioout_get(pstLiveSound->audioIoPath, HD_AUDIOOUT_PARAM_OUT, &stAudioOutPrm);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioout_get(HD_AUDIOOUT_PARAM_OUT) fail %d\n", hdRet);
        return SAL_FAIL;
    }

    stAudioOutPrm.sample_rate = pstLiveSound->hdSampleRate;
    stAudioOutPrm.sample_bit = pstLiveSound->hdBitWidth;
    stAudioOutPrm.mode = pstLiveSound->hdSoundMode;
    hdRet = hd_audioout_set(pstLiveSound->audioIoPath, HD_AUDIOOUT_PARAM_OUT, &stAudioOutPrm);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioout_set(HD_AUDIOOUT_PARAM_OUT) fail %d\n", hdRet);
        return SAL_FAIL;
    }

#ifdef AUDIO_AO_BIND_CODEC
#if 0
    hdRet = vendor_common_get_ddrid(HD_COMMON_MEM_AU_DEC_AU_RENDER_IN_POOL, COMMON_PCIE_CHIP_RC, &eDdrId);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("vendor_common_get_ddrid fail,%d dev %d chn %d\n", hdRet, pstLiveSound->u32Dev, pstLiveSound->u32Chn);
        return SAL_FAIL;
    }

#endif

    hdRet = hd_audiodec_open(HD_AUDIODEC_IN(0, pstLiveSound->u32Chn),
                             HD_AUDIODEC_OUT(0, pstLiveSound->u32Chn),
                             &pstLiveSound->audioCodecPath);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiodec_open fail,%d dev %d chn %d\n", hdRet, pstLiveSound->u32Dev, pstLiveSound->u32Chn);
        return SAL_FAIL;
    }

    stAudioDecPathCfg.data_pool[0].mode = HD_AUDIODEC_POOL_ENABLE;
    stAudioDecPathCfg.data_pool[0].ddr_id = eDdrId;
    stAudioDecPathCfg.data_pool[0].frame_sample_size = 320 * 2;
    stAudioDecPathCfg.data_pool[0].counts = HD_AUDIODEC_SET_COUNT(10, 0);
    hdRet = hd_audiodec_set(pstLiveSound->audioCodecPath, HD_AUDIODEC_PARAM_PATH_CONFIG, &stAudioDecPathCfg);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiodec_set(HD_AUDIODEC_PARAM_PATH_CONFIG) fail %d\n", hdRet);
        return SAL_FAIL;
    }

    /* set audiodec parameters */
    hdRet = hd_audiodec_get(pstLiveSound->audioCodecPath, HD_AUDIODEC_PARAM_IN, &stAudioDecInPrm);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiodec_get(HD_AUDIODEC_PARAM_IN) fail %d\n", hdRet);
        return SAL_FAIL;
    }

    stAudioDecInPrm.codec_type = HD_AUDIO_CODEC_PCM;
    stAudioDecInPrm.sample_rate = pstLiveSound->hdSampleRate;
    stAudioDecInPrm.sample_bit = pstLiveSound->hdBitWidth;
    stAudioDecInPrm.mode = pstLiveSound->hdSoundMode;
    hdRet = hd_audiodec_set(pstLiveSound->audioCodecPath, HD_AUDIODEC_PARAM_IN, &stAudioDecInPrm);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiodec_set(HD_AUDIODEC_PARAM_IN) fail %d\n", hdRet);
        return SAL_FAIL;
    }

    hdRet = hd_audiodec_bind(HD_AUDIODEC_OUT(0, pstLiveSound->u32Chn), HD_AUDIOOUT_IN(pstLiveSound->u32Dev, pstLiveSound->u32Chn));
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiodec_bind(HD_AUDIOENC_PARAM_OUT) fail %d\n", hdRet);
        return SAL_FAIL;
    }

    hdRet = hd_audiodec_start(pstLiveSound->audioCodecPath);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("start hd_audiodec_start fail%d\n", hdRet);
        return SAL_FAIL;
    }

#endif

#ifdef LIVE_SOUND /*line view test，no io bind*/
    hdRet = hd_audiocap_bind(HD_AUDIOCAP_OUT(0, 0), HD_AUDIOOUT_IN(0, 0));
    if (HD_OK != hdRet)
    {
        SAL_LOGE("start hd_audiodec_start fail%d\n", hdRet);
        return SAL_FAIL;
    }

#endif

    hdRet = hd_audioout_start(pstLiveSound->audioIoPath);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("start audioout fail %d\n", hdRet);
        return SAL_FAIL;
    }

    SAL_INFO("%p dev %d chn %d init finish path ao %#x adec %#x\n",
             pstLiveSound, pstLiveSound->u32Dev, pstLiveSound->u32Chn,
             pstLiveSound->audioIoPath, pstLiveSound->audioCodecPath);

    return SAL_SOK;
}

/**
 * @function   audio_aoDevInit
 * @brief      device cfg
 * @param[in]  NTV3_AUDIO_DEV_ST *pstDev
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_aoDevInit(NTV3_AUDIO_DEV_ST *pstDev)
{
    HD_RESULT hdRet = HD_OK;

#ifdef AUDIO_INIT_DEV
    HD_AUDIOOUT_DEV_CONFIG stAudioDevCfg = {0};
    HD_AUDIOOUT_DRV_CONFIG stAudioDrvCfg = {0};
#endif
    if (NULL == pstDev)
    {
        SAL_LOGE("null prm\n");
        return SAL_FAIL;
    }

    if (SAL_FALSE == pstDev->isInit)
    {
        hdRet = hd_audioout_init();
        if (HD_OK != hdRet)
        {
            SAL_LOGE("hd_audiocap_init fail,%d\n", hdRet);
            return SAL_FAIL;
        }

#ifdef AUDIO_AO_BIND_CODEC
        hdRet = hd_audiodec_init();
        if (HD_OK != hdRet)
        {
            SAL_LOGE("hd_audiodec_init fail,%d\n", hdRet);
            return SAL_FAIL;
        }

#endif

#ifdef AUDIO_INIT_DEV
        hdRet = hd_audioout_open(0, HD_AUDIOOUT_0_CTRL, &pstDev->audioDevPath); /* open this for device control */
        memset(&stAudioDevCfg, 0, sizeof(stAudioDevCfg));

        stAudioDevCfg.out_max.sample_rate = HD_AUDIO_SR_8000;
        stAudioDevCfg.out_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
        stAudioDevCfg.out_max.mode = HD_AUDIO_SOUND_MODE_MONO;
        stAudioDevCfg.frame_sample_max = 320;
        stAudioDevCfg.frame_num_max = 10;
        hdRet = hd_audioout_set(pstDev->audioDevPath, HD_AUDIOOUT_PARAM_DEV_CONFIG, &stAudioDevCfg);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("hd_audioout_set(HD_AUDIOOUT_PARAM_DEV_CONFIG) fail %d\n", hdRet);
            return SAL_FAIL;
        }

        stAudioDrvCfg.mono = HD_AUDIO_MONO_RIGHT;
        stAudioDrvCfg.output = HD_AUDIOOUT_OUTPUT_SPK;
        hdRet = hd_audioout_set(pstDev->audioDevPath, HD_AUDIOOUT_PARAM_DRV_CONFIG, &stAudioDrvCfg);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("hd_audioout_set(HD_AUDIOOUT_PARAM_DRV_CONFIG) fail %d\n", hdRet);
            return SAL_FAIL;
        }

#endif
        pstDev->isInit = SAL_TRUE;
    }
    else
    {
        SAL_LOGE("ao device already inited\n");
    }

    return SAL_SOK;
}

/**
 * @function   audio_aoCreate
 * @brief    配置音频输入参数
 * @param[in] AUDIO_LIVESOUND_ST *pstLiveSound 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_aiCreate(AUDIO_LIVESOUND_ST *pstLiveSound)
{
    HD_RESULT hdRet = HD_OK;
    INT32 s32Ret = SAL_SOK;
    HD_COMMON_MEM_DDR_ID eDdrId = AUDIO_DDR_ID;
    HD_AUDIOCAP_IN stAudioCapPrm = {0};
    HD_AUDIOCAP_DEV_CONFIG stAudioDevCfg = {0};

#ifdef AUDIO_AI_BIND_CODEC
    HD_AUDIOENC_OUT stAudioEncPrm = {0};
#endif
    /* HD_AUDIOCAP_BUFINFO stAudioCapbuf = {0}; */

    if (NULL == pstLiveSound)
    {
        SAL_LOGE("null prm\n");
        return SAL_FAIL;
    }

#if 0
    hdRet = vendor_common_get_ddrid(HD_COMMON_MEM_AU_ENC_AU_GRAB_OUT_POOL, COMMON_PCIE_CHIP_RC, &eDdrId);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("vendor_common_get_ddrid fail,%d dev %d chn %d\n", hdRet, pstLiveSound->u32Dev, pstLiveSound->u32Chn);
        return SAL_FAIL;
    }

#endif

    hdRet = hd_audiocap_open(HD_AUDIOCAP_IN(pstLiveSound->u32Dev, 0),
                             HD_AUDIOCAP_OUT(pstLiveSound->u32Dev, pstLiveSound->u32Chn),
                             &pstLiveSound->audioIoPath);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiocap_open fail,%d\n", hdRet);
        return SAL_FAIL;
    }

    memset(&stAudioDevCfg, 0, sizeof(stAudioDevCfg));

    stAudioDevCfg.data_pool[0].mode = HD_AUDIOCAP_POOL_ENABLE;
    stAudioDevCfg.data_pool[0].ddr_id = eDdrId;
    stAudioDevCfg.data_pool[0].frame_sample_size = pstLiveSound->u32FrmLen;
    stAudioDevCfg.data_pool[0].counts = HD_AUDIOCAP_SET_COUNT(20, 0);

    hdRet = hd_audiocap_set(pstLiveSound->audioIoPath, HD_AUDIOCAP_PARAM_DEV_CONFIG, &stAudioDevCfg);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiocap_set(HD_AUDIOCAP_PARAM_PATH_CONFIG) fail %d\n", hdRet);
        return SAL_FAIL;
    }

    /* set audiocap parameters */
    hdRet = hd_audiocap_get(pstLiveSound->audioIoPath, HD_AUDIOCAP_PARAM_IN, &stAudioCapPrm);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiocap_get(HD_AUDIOCAP_PARAM_IN) fail %d\n", hdRet);
        return SAL_FAIL;
    }

    stAudioCapPrm.sample_rate = pstLiveSound->hdSampleRate;
    stAudioCapPrm.sample_bit = pstLiveSound->hdBitWidth;
    stAudioCapPrm.mode = pstLiveSound->hdSoundMode;
    stAudioCapPrm.frame_sample = pstLiveSound->u32FrmLen / 2;  /* for 25fps: 8000/25=320 */
    hdRet = hd_audiocap_set(pstLiveSound->audioIoPath, HD_AUDIOCAP_PARAM_IN, &stAudioCapPrm);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiocap_set(HD_AUDIOCAP_PARAM_IN) fail %d\n", hdRet);
        return SAL_FAIL;
    }

#ifdef AUDIO_AI_BIND_CODEC
    hdRet = hd_audioenc_open(HD_AUDIOENC_IN(0, pstLiveSound->u32Chn),
                             HD_AUDIOENC_OUT(0, pstLiveSound->u32Chn),
                             &pstLiveSound->audioCodecPath);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioenc_open fail,%d dev %d chn %d\n", hdRet, pstLiveSound->u32Dev, pstLiveSound->u32Chn);
        return SAL_FAIL;
    }

    /* set audioenc parameters */
    hdRet = hd_audioenc_get(pstLiveSound->audioCodecPath, HD_AUDIOENC_PARAM_OUT, &stAudioEncPrm);
    if (hdRet != HD_OK)
    {
        SAL_LOGE("hd_audioenc_get(HD_AUDIOENC_PARAM_OUT) fail %d\n", hdRet);
        return SAL_FAIL;
    }

    stAudioEncPrm.codec_type = HD_AUDIO_CODEC_PCM;
    hdRet = hd_audioenc_set(pstLiveSound->audioCodecPath, HD_AUDIOENC_PARAM_OUT, &stAudioEncPrm);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioenc_set(HD_AUDIOENC_PARAM_OUT) fail %d\n", hdRet);
        return SAL_FAIL;
    }

    hdRet = hd_audiocap_bind(HD_AUDIOCAP_OUT(pstLiveSound->u32Dev, pstLiveSound->u32Chn), HD_AUDIOENC_IN(0, pstLiveSound->u32Chn));
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiocap_bind fail %d\n", hdRet);
        return SAL_FAIL;
    }

#endif

    hdRet = hd_audiocap_start(pstLiveSound->audioIoPath);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("start hd_audiocap_start fail%d\n", hdRet);
        return SAL_FAIL;
    }

#ifdef AUDIO_AI_BIND_CODEC
    hdRet = hd_audioenc_start(pstLiveSound->audioCodecPath);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("start audioenc fail%d\n", hdRet);
        return SAL_FAIL;
    }

#endif

    /* prepare buffer for receiving data */
    pstLiveSound->stBitStreamData.u32BufSize = pstLiveSound->u32FrmLen * 2;
    pstLiveSound->stBitStreamData.eDdrId = eDdrId;
    s32Ret = audio_mapBuf(&pstLiveSound->stBitStreamData);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("audio_mapBuf fail\n");
        return SAL_FAIL;
    }

    SAL_INFO("bit stream size %d addr %p\n", pstLiveSound->stBitStreamData.u32BufSize, pstLiveSound->stBitStreamData.pVirAddr);

    SAL_INFO("%p dev %d chn %d init finish path ai %#x aenc %#x\n",
             pstLiveSound, pstLiveSound->u32Dev, pstLiveSound->u32Chn,
             pstLiveSound->audioIoPath, pstLiveSound->audioCodecPath);

    return SAL_SOK;
}

/**
 * @function   audio_aiDevInit
 * @brief      device cfg
 * @param[in]  NTV3_AUDIO_DEV_ST *pstDev
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 audio_aiDevInit(NTV3_AUDIO_DEV_ST *pstDev)
{
    HD_RESULT hdRet = HD_OK;

#ifdef AUDIO_INIT_DEV
    HD_AUDIOCAP_DEV_CONFIG stAudioDevCfg = {0};
    HD_AUDIOCAP_DRV_CONFIG stAudioDrvCfg = {0};
#endif

    if (NULL == pstDev)
    {
        SAL_LOGE("null prm\n");
        return SAL_FAIL;
    }

    if (SAL_FALSE == pstDev->isInit)
    {
        hdRet = hd_audiocap_init();
        if (HD_OK != hdRet)
        {
            SAL_LOGE("hd_audiocap_init fail,%d\n", hdRet);
            return SAL_FAIL;
        }

#ifdef AUDIO_AI_BIND_CODEC
        hdRet = hd_audioenc_init();
        if (HD_OK != hdRet)
        {
            SAL_LOGE("hd_audiodec_init fail,%d\n", hdRet);
            return SAL_FAIL;
        }

#endif

#ifdef AUDIO_INIT_DEV
        hdRet = hd_audiocap_open(0, HD_AUDIOCAP_0_CTRL, &pstDev->audioDevPath); /* open this for device control */
        memset(&stAudioDevCfg, 0, sizeof(stAudioDevCfg));

        stAudioDevCfg.in_max.sample_rate = HD_AUDIO_SR_8000;
        stAudioDevCfg.in_max.sample_bit = HD_AUDIO_BIT_WIDTH_16;
        stAudioDevCfg.in_max.mode = HD_AUDIO_SOUND_MODE_MONO;
        stAudioDevCfg.in_max.frame_sample = 320;

        stAudioDevCfg.frame_num_max = 10;
        hdRet = hd_audiocap_set(pstDev->audioDevPath, HD_AUDIOCAP_PARAM_DEV_CONFIG, &stAudioDevCfg);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("hd_audiocap_set(HD_AUDIOCAP_PARAM_PATH_CONFIG) fail %d\n", hdRet);
            return SAL_FAIL;
        }

        stAudioDrvCfg.mono = HD_AUDIO_MONO_RIGHT;
        hdRet = hd_audiocap_set(pstDev->audioDevPath, HD_AUDIOCAP_PARAM_DRV_CONFIG, &stAudioDrvCfg);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("hd_audiocap_set(HD_AUDIOCAP_PARAM_PATH_CONFIG) fail %d\n", hdRet);
            return SAL_FAIL;
        }

#endif
        pstDev->isInit = SAL_TRUE;
    }
    else
    {
        SAL_LOGE("ai device already inited\n");
    }

    return SAL_SOK;
}

/**
 * @function   audio_ntv3_aoCreate
 * @brief    音频输出初始化
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ntv3_aoCreate(HAL_AIO_ATTR_S *pstAIOAttr)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 i = 0;
    AUDIO_LIVESOUND_ST *pstLiveSound = NULL;

    if ((NULL == pstAIOAttr)
        || (MAX_AUDIO_DEV_NUM <= pstAIOAttr->u32AudioDevIdx) /*dev id*/
        || (MAX_AUDIO_CHN_NUM < pstAIOAttr->u32Chan)) /*chan number*/
    {
        SAL_LOGE("Input parameter pstAIOAttr is NULL!!!\n");
        return SAL_FAIL;
    }

    if (SAL_TRUE != g_stAudioOut[pstAIOAttr->u32AudioDevIdx].isInit)
    {
        s32Ret = audio_aoDevInit(&g_stAudioOut[pstAIOAttr->u32AudioDevIdx]);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("audio_aoDevInit error! %d\n", pstAIOAttr->u32AudioDevIdx);
            return SAL_FAIL;
        }
    }

    for (i = 0; i < pstAIOAttr->u32Chan; i++)
    {
        pstLiveSound = &g_stAudioOut[pstAIOAttr->u32AudioDevIdx].g_stAudioChan[i];
        pstLiveSound->u32Dev = pstAIOAttr->u32AudioDevIdx;
        pstLiveSound->u32Chn = i;

        pstLiveSound->hdSampleRate = audio_sampleRateParse(pstAIOAttr->u32SampleRate);
        pstLiveSound->hdBitWidth = audio_bitWidthParse(pstAIOAttr->u32BitWidth);
        pstLiveSound->hdSoundMode = audio_soundModeParse(pstAIOAttr->u32SoundMode);
        pstLiveSound->u32FrmRate = pstAIOAttr->u32FrameRate ? pstAIOAttr->u32FrameRate : 25;
        pstLiveSound->u32FrmLen = pstAIOAttr->u32SampleRate * pstAIOAttr->u32BitWidth / pstLiveSound->u32FrmRate / 8;

        s32Ret = audio_aoCreate(pstLiveSound);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("AO Create error!\n");

            return SAL_FAIL;
        }

#ifdef AUDIO_AO_VOLUME_SOFT
        s32Ret = hik_volume_create(&pstLiveSound->pVolumeHdl, 1, 7);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Ao volume create error!\n");
            return SAL_FAIL;
        }

#endif
    }

    pthread_mutex_init(&SendFrame_lock, NULL);

    return SAL_SOK;
}

/**
 * @function   audio_ntv3_aoDestroy
 * @brief      销毁音频输出
 * @param[in]  HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return     void
 */
void audio_ntv3_aoDestroy(HAL_AIO_ATTR_S *pstAIOAttr)
{
#ifdef AUDIO_AO_VOLUME_SOFT
    INT32 s32Ret = SAL_FAIL;
#endif
    HD_RESULT hdRet = HD_OK;
    UINT32 i = 0;
    AUDIO_LIVESOUND_ST *pstLiveSound = NULL;

    if ((NULL == pstAIOAttr)
        || (MAX_AUDIO_DEV_NUM <= pstAIOAttr->u32AudioDevIdx)
        || (MAX_AUDIO_CHN_NUM < pstAIOAttr->u32Chan))
    {
        SAL_LOGE("Input parameter pstAIOAttr is NULL!!!\n");
        return;
    }

    if (SAL_TRUE != g_stAudioOut[pstAIOAttr->u32AudioDevIdx].isInit)
    {
        SAL_LOGE("dev not init!\n");
        return;
    }

    for (i = 0; i < pstAIOAttr->u32Chan; i++)
    {
        pstLiveSound = &g_stAudioOut[pstAIOAttr->u32AudioDevIdx].g_stAudioChan[i];
        hdRet = hd_audioout_stop(pstLiveSound->audioIoPath);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("stop audioout fail %d\n", hdRet);
        }

#ifdef AUDIO_AO_BIND_CODEC
        hdRet = hd_audiodec_stop(pstLiveSound->audioCodecPath);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("stop hd_audioenc_stop fail %d\n", hdRet);
        }

        hdRet = hd_audiodec_unbind(HD_AUDIODEC_OUT(0, pstLiveSound->u32Chn));
        if (HD_OK != hdRet)
        {
            SAL_LOGE("stop hd_audiocap_unbind fail %d\n", hdRet);
        }

        hdRet = hd_audiodec_close(pstLiveSound->audioCodecPath);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("stop hd_audiodec_close fail %d\n", hdRet);
        }

#endif

        hdRet = hd_audioout_close(pstLiveSound->audioIoPath);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("stop hd_audioout_close fail %d\n", hdRet);
        }

#ifdef AUDIO_AO_VOLUME_SOFT
        s32Ret = hik_volume_destroy(&pstLiveSound->pVolumeHdl);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("stop hik_volume_destroy fail\n");
        }

#endif
    }

#ifdef AUDIO_AO_BIND_CODEC
    hdRet = hd_audiodec_uninit();
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiodec_uninit fail,%d\n", hdRet);
        return;
    }

#endif

    hdRet = hd_audioout_uninit();
    if (HD_OK != hdRet)
    {
        SAL_LOGE("stop hd_audioout_uninit fail %d\n", hdRet);
    }

    g_stAudioOut[pstAIOAttr->u32AudioDevIdx].isInit = SAL_FALSE;

    pthread_mutex_destroy(&SendFrame_lock);
}

/**
 * @function   audio_ntv3_aoSendFrame
 * @brief    输出音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ntv3_aoSendFrame(HAL_AIO_FRAME_S *pstAudioFrame)
{
    HD_RESULT hdRet = HD_OK;
    INT32 s32Ret = SAL_SOK;
    AUDIO_LIVESOUND_ST *pstLiveSound = NULL;
    UINT64 u64AoStartTime = 0;
    UINT64 u64AoEndTime = 0;


#ifdef AUDIO_AO_BIND_CODEC
    HD_AUDIODEC_SEND_LIST stSendList = {0}; /*single frm*/
#else
    UINT64 u64SleepTime = 0; /*us*/
    UINT64 u64WaitTime = 0;
    HD_AUDIO_FRAME stAudioFrm = {0};
#endif

    if ((NULL == pstAudioFrame)
        || (MAX_AUDIO_DEV_NUM <= pstAudioFrame->AudioDevIdx)
        || (MAX_AUDIO_CHN_NUM <= pstAudioFrame->Channel))
    {
        SAL_LOGE("Parameter is wrong!\n");
        return SAL_FAIL;
    }

    if (SAL_TRUE != g_stAudioOut[pstAudioFrame->AudioDevIdx].isInit)
    {
        SAL_LOGE("dev not init!\n");
        return SAL_FAIL;
    }

    u64AoStartTime = SAL_getCurUs();
    pthread_mutex_lock(&SendFrame_lock);
    pstLiveSound = &g_stAudioOut[pstAudioFrame->AudioDevIdx].g_stAudioChan[pstAudioFrame->Channel];

#ifdef AUDIO_AO_VOLUME_SOFT
    /*考虑下要不要增加一级缓存*/
    s32Ret = hik_volume_proc(pstLiveSound->pVolumeHdl, pstAudioFrame->pBufferAddr,
                             pstAudioFrame->FrameLen, pstAudioFrame->pBufferAddr);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("hik_volume_proc fail\n");
        goto exit;
    }

#endif

    /* prepare buffer for sending data */
    pstLiveSound->stBitStreamData.u32BufSize = pstAudioFrame->FrameLen;
    pstLiveSound->stBitStreamData.eDdrId = AUDIO_DDR_ID;
    s32Ret = audio_mapBuf(&pstLiveSound->stBitStreamData);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("audio_mapBuf fail\n");
        goto exit;
    }

    memcpy((void *)pstLiveSound->stBitStreamData.pVirAddr, pstAudioFrame->pBufferAddr, pstAudioFrame->FrameLen);

#ifdef AUDIO_AO_BIND_CODEC
    /* fill the structure and send it to audiodec */
    stSendList.path_id = pstLiveSound->audioCodecPath;
    stSendList.user_bs.timestamp = u64AoStartTime;
    stSendList.user_bs.p_user_buf = pstLiveSound->stBitStreamData.pVirAddr;
    stSendList.user_bs.user_buf_size = pstAudioFrame->FrameLen;
    if ((hdRet = hd_audiodec_send_list(&stSendList, 1, 500)) < 0)
    {
        SAL_LOGE("<send bitstream fail(%d)!>\n", hdRet);
        goto exit1;
    }

#else /*非阻塞模式，需要外部控制输出速度*/
    stAudioFrm.sign = MAKEFOURCC('A', 'F', 'R', 'M');
    stAudioFrm.ddr_id = pstLiveSound->stBitStreamData.eDdrId;
    stAudioFrm.size = pstAudioFrame->FrameLen;
    stAudioFrm.phy_addr[0] = pstLiveSound->stBitStreamData.phyAddr; /*物理地址*/
    stAudioFrm.bit_width = pstLiveSound->hdBitWidth;
    stAudioFrm.sound_mode = pstLiveSound->hdSoundMode;
    stAudioFrm.sample_rate = pstLiveSound->hdSampleRate;
    stAudioFrm.timestamp = u64AoStartTime;
    stAudioFrm.count = 1;
    hdRet = hd_audioout_push_in_buf(pstLiveSound->audioIoPath, &stAudioFrm, 200);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("stop hd_audioout_push_in_buf fail %d\n", hdRet);
        goto exit1;
    }

#endif
    /* release buffer */
    s32Ret = audio_unMapBuf(&pstLiveSound->stBitStreamData);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("audio_mapBuf fail\n");
        goto exit;
    }

    pthread_mutex_unlock(&SendFrame_lock);

    u64AoEndTime = SAL_getCurUs();

#ifdef AUDIO_AO_BIND_CODEC
    /* SAL_LOGI("send frm start %lld end %lld\n", u64AoStartTime / 1000, u64AoEndTime / 1000); */
    pstLiveSound->u64FrmTime = u64AoEndTime - u64AoStartTime; /*仅用于消除编译错误*/
#else
    if (pstLiveSound->u32FrmRate)
    {
        u64SleepTime = 1000000 / pstLiveSound->u32FrmRate;
    }
    else
    {
        u64SleepTime = 40000; /*us*/
    }

    if ((pstLiveSound->u64FrmTime > u64AoStartTime)
        || (pstLiveSound->u64FrmTime + u64SleepTime * 2 < u64AoStartTime))
    {
        /*第一帧输出*/
        if (u64SleepTime + u64AoStartTime > u64AoEndTime)
        {
            u64WaitTime = u64SleepTime + u64AoStartTime - u64AoEndTime;
        }
    }
    else if (u64SleepTime + pstLiveSound->u64FrmTime > u64AoEndTime)
    {
        u64WaitTime = u64SleepTime + pstLiveSound->u64FrmTime - u64AoEndTime;
    }

    /* SAL_LOGI("send frm start %lld end %lld sleep %lld last %lld\n", u64AoStartTime, u64AoEndTime, u64WaitTime, pstLiveSound->u64FrmTime); */

    if (u64WaitTime)
    {
        usleep(u64WaitTime);
    }

    pstLiveSound->u64FrmTime = u64AoStartTime;
#endif
    return SAL_SOK;

exit1:
    /* release buffer */
    s32Ret = audio_unMapBuf(&pstLiveSound->stBitStreamData);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("audio_mapBuf fail\n");
    }

exit:
    pthread_mutex_unlock(&SendFrame_lock);
    return SAL_FAIL;
}

/**
 * @function   audio_ntv3_aiCreate
 * @brief    音频输入初始化
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ntv3_aiCreate(HAL_AIO_ATTR_S *pstAIOAttr)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 i = 0;
    AUDIO_LIVESOUND_ST *pstLiveSound = NULL;

    if ((NULL == pstAIOAttr)
        || (MAX_AUDIO_DEV_NUM <= pstAIOAttr->u32AudioDevIdx)
        || (MAX_AUDIO_CHN_NUM < pstAIOAttr->u32Chan))
    {
        SAL_LOGE("Input parameter pstAIOAttr is NULL!!!\n");
        return SAL_FAIL;
    }

    if (SAL_TRUE != g_stAudioIn[pstAIOAttr->u32AudioDevIdx].isInit)
    {
        s32Ret = audio_aiDevInit(&g_stAudioIn[pstAIOAttr->u32AudioDevIdx]);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("audio_aiDevInit error! %d\n", pstAIOAttr->u32AudioDevIdx);
            return SAL_FAIL;
        }
    }

    for (i = 0; i < pstAIOAttr->u32Chan; i++)
    {
        pstLiveSound = &g_stAudioIn[pstAIOAttr->u32AudioDevIdx].g_stAudioChan[i];

        pstLiveSound->u32Dev = pstAIOAttr->u32AudioDevIdx;
        pstLiveSound->u32Chn = i;

        pstLiveSound->hdSampleRate = audio_sampleRateParse(pstAIOAttr->u32SampleRate);
        pstLiveSound->hdBitWidth = audio_bitWidthParse(pstAIOAttr->u32BitWidth);
        pstLiveSound->hdSoundMode = audio_soundModeParse(pstAIOAttr->u32SoundMode);
        pstLiveSound->u32FrmRate = pstAIOAttr->u32FrameRate ? pstAIOAttr->u32FrameRate : 25;
        pstLiveSound->u32FrmLen = pstAIOAttr->u32SampleRate * pstAIOAttr->u32BitWidth / pstLiveSound->u32FrmRate / 8;
        if (DEF_AUDIO_FRM_LEN != pstLiveSound->u32FrmLen)
        {
            SAL_LOGE("ai len err %d!\n", pstLiveSound->u32FrmLen);
            pstLiveSound->u32FrmLen = DEF_AUDIO_FRM_LEN;
        }

        s32Ret = audio_aiCreate(pstLiveSound);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Ai Create error!\n");
            return SAL_FAIL;
        }

#ifdef AUDIO_AI_VOLUME_SOFT
        s32Ret = hik_volume_create(&pstLiveSound->pVolumeHdl, 0, 7);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("Ai volume create error!\n");
            return SAL_FAIL;
        }

#endif
    }

    pthread_mutex_init(&GetFrame_lock, NULL);

    return SAL_SOK;
}

/**
 * @function   audio_ntv3_aiDestroy
 * @brief      音频输入销毁
 * @param[in]  HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return     void
 */
void audio_ntv3_aiDestroy(HAL_AIO_ATTR_S *pstAIOAttr)
{
    HD_RESULT hdRet = HD_OK;
    INT32 s32Ret = SAL_FAIL;
    UINT32 i = 0;
    AUDIO_LIVESOUND_ST *pstLiveSound = NULL;

    if ((NULL == pstAIOAttr)
        || (MAX_AUDIO_DEV_NUM <= pstAIOAttr->u32AudioDevIdx)
        || (MAX_AUDIO_CHN_NUM < pstAIOAttr->u32Chan))
    {
        SAL_LOGE("Input parameter pstAIOAttr is NULL!!!\n");
        return;
    }

    if (SAL_TRUE != g_stAudioIn[pstAIOAttr->u32AudioDevIdx].isInit)
    {
        SAL_LOGE("dev not init!\n");
        return;
    }

    for (i = 0; i < pstAIOAttr->u32Chan; i++)
    {
        pstLiveSound = &g_stAudioIn[pstAIOAttr->u32AudioDevIdx].g_stAudioChan[i];

        hdRet = hd_audiocap_stop(pstLiveSound->audioIoPath);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("stop hd_audiocap_stop fail %d\n", hdRet);
        }

#ifdef AUDIO_AI_BIND_CODEC
        hdRet = hd_audioenc_stop(pstLiveSound->audioCodecPath);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("stop hd_audioenc_stop fail %d\n", hdRet);
        }

        hdRet = hd_audiocap_unbind(HD_AUDIOCAP_OUT(pstLiveSound->u32Dev, pstLiveSound->u32Chn));
        if (HD_OK != hdRet)
        {
            SAL_LOGE("stop hd_audiocap_unbind fail %d\n", hdRet);
        }

        hdRet = hd_audioenc_close(pstLiveSound->audioCodecPath);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("stop hd_audioenc_close fail %d\n", hdRet);
        }

#endif

        hdRet = hd_audiocap_close(pstLiveSound->audioIoPath);
        if (HD_OK != hdRet)
        {
            SAL_LOGE("stop hd_audiocap_close fail %d\n", hdRet);
        }

#ifdef AUDIO_AI_VOLUME_SOFT
        s32Ret = hik_volume_destroy(&pstLiveSound->pVolumeHdl);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("stop hik_volume_destroy fail\n");
        }

#endif
    }

    hdRet = hd_audiocap_uninit();
    if (HD_OK != hdRet)
    {
        SAL_LOGE("stop hd_audiocap_uninit fail %d\n", hdRet);
    }

#ifdef AUDIO_AI_BIND_CODEC
    hdRet = hd_audioenc_uninit();
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioenc_uninit fail,%d\n", hdRet);
        return;
    }

#endif
    s32Ret = audio_unMapBuf(&pstLiveSound->stBitStreamData);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("audio_unMapBuf fail\n");
        return;
    }

    g_stAudioIn[pstAIOAttr->u32AudioDevIdx].isInit = SAL_FALSE;

    pthread_mutex_destroy(&GetFrame_lock);
}

/**
 * @function   audio_ntv3_aiGetFrame
 * @brief    获取音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_aiGetEncFrame(AUDIO_LIVESOUND_ST *pstLiveSound, HAL_AIO_FRAME_S *pstAudioFrame)
{
    HD_RESULT hdRet = HD_OK;
    UINT32 u32FrmLen = 0;

#ifdef AUDIO_ENC_PULL
    HD_AUDIO_BS stAudioBs = {0};
    char *pVirAddr = NULL;
#else
    HD_AUDIOENC_RECV_LIST stRecvFrm = {0};
    HD_AUDIOENC_POLL_LIST stPollFrm = {0};
#endif

    if ((NULL == pstLiveSound) || (NULL == pstAudioFrame))
    {
        SAL_LOGE("Parameter is wrong!\n");
        return SAL_FAIL;
    }

    u32FrmLen = pstLiveSound->u32FrmLen;
    if (DEF_AUDIO_FRM_LEN != u32FrmLen)
    {
        SAL_LOGE("frm len err %d!!!\n", u32FrmLen);
        return SAL_FAIL;
    }

#ifdef AUDIO_ENC_PULL
    hdRet = hd_audioenc_pull_out_buf(pstLiveSound->audioCodecPath, &stAudioBs, 500);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioenc_pull_out_buf fail %d,dev %d chn %d\n", hdRet, pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel);
        goto exit;
    }

    pVirAddr = (char *)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE,
                                          stAudioBs.phy_addr,
                                          stAudioBs.size);
    if (NULL == pVirAddr)
    {
        SAL_LOGE("hd_common_mem_munmap fail %d\n", hdRet);
        goto exit1;
    }

    if (stAudioBs.size <= u32FrmLen)                                                                               /*这里不合理，pstAudioFrame的buf大小未知*/
    {
        memcpy(pstAudioFrame->pBufferAddr, (void *)pVirAddr, stAudioBs.size);
        pstAudioFrame->FrameLen = stAudioBs.size;
    }

    hdRet = hd_common_mem_munmap((void *)pVirAddr, stAudioBs.size);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_common_mem_munmap fail %d\n", hdRet);
        goto exit1;
    }

    hdRet = hd_audioenc_release_out_buf(pstLiveSound->audioCodecPath, &stAudioBs);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioenc_pull_out_buf fail %d,dev %d chn %d\n", hdRet, pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel);
        goto exit;
    }

#else
    stPollFrm.path_id = pstLiveSound->audioCodecPath;
    /* call poll to check the stream availability */
    hdRet = hd_audioenc_poll_list(&stPollFrm, 1, 1000);
    if (HD_ERR_TIMEDOUT == hdRet)
    {
        SAL_LOGE("Poll timeout %d path %#x!!\n", hdRet, stPollFrm.path_id);
        goto exit;
    }

    if (stPollFrm.revent.event != TRUE)     /*TRUE == 0*/
    {
        SAL_LOGE("ret %d stPollFrm.revent.event %d!!\n", hdRet, stPollFrm.revent.event);
        goto exit;
    }

    if (stPollFrm.revent.bs_size > u32FrmLen)
    {
        SAL_LOGE("buffer size is not enough! %d, %d\n", stPollFrm.revent.bs_size, u32FrmLen);
        goto exit;
    }

    stRecvFrm.path_id = pstLiveSound->audioCodecPath;
    stRecvFrm.user_bs.p_user_buf = pstLiveSound->stBitStreamData.pVirAddr;
    stRecvFrm.user_bs.user_buf_size = u32FrmLen;

    hdRet = hd_audioenc_recv_list(&stRecvFrm, 1);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioenc_recv_list err %d path %#x!!\n",
                 hdRet, stRecvFrm.path_id);
        goto exit;
    }

    if (stRecvFrm.retval < 0)
    {
        SAL_LOGE("get bitstreame error! ret = %d\n", stRecvFrm.retval);
        goto exit;
    }
    else if (stRecvFrm.retval >= 0)
    {
        /* write audio data */
        if (stRecvFrm.user_bs.size > 0)
        {
            memcpy(pstAudioFrame->pBufferAddr, (void *)stRecvFrm.user_bs.p_user_buf, stRecvFrm.user_bs.size);
            pstAudioFrame->FrameLen = stRecvFrm.user_bs.size;
        }
    }

#endif

    return SAL_SOK;
#ifdef AUDIO_ENC_PULL
exit1:
    hdRet = hd_audioenc_release_out_buf(pstLiveSound->audioCodecPath, &stAudioBs);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audioenc_pull_out_buf fail %d,dev %d chn %d\n", hdRet, pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel);
        goto exit;
    }

#endif
exit:
    return SAL_FAIL;
}

/**
 * @function   audio_ntv3_aiGetFrame
 * @brief    获取音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ntv3_aiGetFrame(HAL_AIO_FRAME_S *pstAudioFrame)
{
    HD_RESULT hdRet = HD_OK;
    INT32 s32Ret = SAL_SOK;
    UINT32 u32FrmLen = 0;
    AUDIO_LIVESOUND_ST *pstLiveSound = NULL;

#ifndef AUDIO_AI_BIND_CODEC
    char *pVirAddr = NULL;
    HD_AUDIO_FRAME stAudioFrm = {0};
#endif

    if ((NULL == pstAudioFrame)
        || (MAX_AUDIO_DEV_NUM <= pstAudioFrame->AudioDevIdx)
        || (MAX_AUDIO_CHN_NUM <= pstAudioFrame->Channel))
    {
        SAL_LOGE("Parameter is wrong!\n");
        return SAL_FAIL;
    }

    if (SAL_TRUE != g_stAudioIn[pstAudioFrame->AudioDevIdx].isInit)
    {
        SAL_LOGE("dev not init!\n");
        return SAL_FAIL;
    }

    pstLiveSound = &g_stAudioIn[pstAudioFrame->AudioDevIdx].g_stAudioChan[pstAudioFrame->Channel];
    u32FrmLen = pstLiveSound->u32FrmLen;
    if (DEF_AUDIO_FRM_LEN != u32FrmLen)
    {
        SAL_LOGE("frm len err %d!!!\n", u32FrmLen);
        return SAL_FAIL;
    }

    pthread_mutex_lock(&GetFrame_lock);
#ifdef AUDIO_AI_BIND_CODEC
    s32Ret = audio_aiGetEncFrame(pstLiveSound, pstAudioFrame);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("audio_aiGetEncFrame fail %d,dev %d chn %d\n", hdRet, pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel);
        goto exit;
    }

#else /*不支持*/

    hdRet = hd_audiocap_pull_out_buf(pstLiveSound->audioIoPath, &stAudioFrm, 500);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiocap_pull_out_buf fail %d,dev %d chn %d\n", hdRet, pstAudioFrame->AudioDevIdx, pstAudioFrame->Channel);
        goto exit;
    }

    pVirAddr = (char *) hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE,
                                           stAudioFrm.phy_addr[0],
                                           stAudioFrm.size);

    if (stAudioFrm.size <= pstLiveSound->u32FrmLen)        /*这里不合理，pstAudioFrame的buf大小未知*/
    {
        memcpy(pstAudioFrame->pBufferAddr, (void *)pVirAddr, stAudioFrm.size);
        pstAudioFrame->FrameLen = stAudioFrm.size;
    }

    hdRet = hd_audiocap_release_out_buf(pstLiveSound->audioIoPath, &stAudioFrm);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("stop hd_audiocap_release_out_buf fail %d\n", hdRet);
        goto exit;
    }

#endif

#if 0
    FILE *pInFile = fopen("/home/config/audio_cap.pcm", "a+");
    if (pInFile)
    {
        fwrite(pstAudioFrame->pBufferAddr, 1, pstAudioFrame->FrameLen, pInFile);
        fflush(pInFile);
        fclose(pInFile);
    }

#endif

#ifdef AUDIO_AI_VOLUME_SOFT
    /*考虑下要不要增加一级缓存*/
    s32Ret = hik_volume_proc(pstLiveSound->pVolumeHdl, pstAudioFrame->pBufferAddr,
                             pstAudioFrame->FrameLen, pstAudioFrame->pBufferAddr);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("hik_volume_proc fail\n");
        goto exit;
    }

#endif

    pthread_mutex_unlock(&GetFrame_lock);
    /* SAL_LOGI("get frm len %d time %d\n", pstAudioFrame->FrameLen, SAL_getCurMs()); */

    return SAL_SOK;

exit:

    pthread_mutex_unlock(&GetFrame_lock);
    return SAL_FAIL;
}

/**
 * @function   audio_ntv3_aiSetVolume
 * @brief    设置音频输入音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ntv3_aiSetVolume(UINT32 u32Channel, UINT32 u32VolumeLev)
{
#ifndef AUDIO_AI_VOLUME_SOFT
    HD_RESULT hdRet = HD_OK;
    HD_AUDIOCAP_VOLUME stAudioVolume = {0};
#endif
    AUDIO_LIVESOUND_ST *pstLiveSound = NULL;

    if ((MAX_AUDIO_CHN_NUM <= u32Channel) && (AUDIO_AI_MAX_VOLUME_LEV > u32VolumeLev))
    {
        SAL_LOGE("Parameter is wrong!\n");
        return SAL_FAIL;
    }

    pstLiveSound = &g_stAudioIn[0].g_stAudioChan[u32Channel];     /*暂时默认dev0*/
#ifdef AUDIO_AI_VOLUME_SOFT
    if (pstLiveSound->pVolumeHdl)
    {
        return hik_volume_set(pstLiveSound->pVolumeHdl, u32VolumeLev);
    }
    else
    {
        SAL_LOGE("%p chan %d null hdl\n", pstLiveSound, u32Channel);
        return SAL_FAIL;
    }

#else
    stAudioVolume.volume = u32AiVolume[u32VolumeLev];
    hdRet = hd_audiocap_set(pstLiveSound->audioIoPath, HD_AUDIOCAP_PARAM_VOLUME, &stAudioVolume);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiocap_open fail,%d\n", hdRet);
        return SAL_FAIL;
    }

    return SAL_SOK;
#endif
}

/**
 * @function   audio_ntv3_aoSetVolume
 * @brief    设置音频输出音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ntv3_aoSetVolume(UINT32 u32Channel, UINT32 u32VolumeLev)
{
#ifndef AUDIO_AO_VOLUME_SOFT
    HD_RESULT hdRet = HD_OK;
    HD_AUDIOOUT_VOLUME stAudioVolume = {0};
#endif
    AUDIO_LIVESOUND_ST *pstLiveSound = NULL;

    if ((MAX_AUDIO_CHN_NUM <= u32Channel) && (AUDIO_AO_MAX_VOLUME_LEV > u32VolumeLev))
    {
        SAL_LOGE("Parameter is wrong!\n");
        return SAL_FAIL;
    }

    pstLiveSound = &g_stAudioOut[0].g_stAudioChan[u32Channel];       /*暂时默认dev0*/
#ifdef AUDIO_AO_VOLUME_SOFT
    if (pstLiveSound->pVolumeHdl)
    {
        return hik_volume_set(pstLiveSound->pVolumeHdl, u32VolumeLev);
    }
    else
    {
        SAL_LOGE("%p chan %d null hdl\n", pstLiveSound, u32Channel);
        return SAL_FAIL;
    }

#else
    stAudioVolume.volume = u32AoVolume[u32VolumeLev];
    hdRet = hd_audioout_set(pstLiveSound->audioIoPath, HD_AUDIOOUT_PARAM_VOLUME, &stAudioVolume);
    if (HD_OK != hdRet)
    {
        SAL_LOGE("hd_audiocap_open fail,%d\n", hdRet);
        return SAL_FAIL;
    }

    SAL_INFO("ao new vol lev %d val %d\n", u32VolumeLev, u32AoVolume[u32VolumeLev]);

    return SAL_SOK;
#endif
}

/**
 * @function   audio_ntv3_aiEnableVqe
 * @brief    开启VQE处理(声音质量增强)
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ntv3_aiEnableVqe(UINT32 u32AudioDevIdx)
{
    return SAL_SOK;
}

/**
 * @function   audio_ntv3_aiDisableVqe
 * @brief    关闭VQE处理
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_ntv3_aiDisableVqe(UINT32 u32AudioDevIdx)
{
    return SAL_SOK;
}

/**
 * @function   audio_hal_register
 * @brief      注册hal层回调函数
 * @param[in]  None
 * @param[out] AUDIO_PLAT_OPS_S * 回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_register(AUDIO_PLAT_OPS_S *pstAudioPlatOps)
{
    if (NULL == pstAudioPlatOps)
    {
        return SAL_FAIL;
    }

    pstAudioPlatOps->aoCreate = audio_ntv3_aoCreate;
    pstAudioPlatOps->aoDestroy = audio_ntv3_aoDestroy;
    pstAudioPlatOps->aoSendFrame = audio_ntv3_aoSendFrame;
    pstAudioPlatOps->aoSetVolume = audio_ntv3_aoSetVolume;

    pstAudioPlatOps->aiCreate = audio_ntv3_aiCreate;
    pstAudioPlatOps->aiDestroy = audio_ntv3_aiDestroy;
    pstAudioPlatOps->aiGetFrame = audio_ntv3_aiGetFrame;
    pstAudioPlatOps->aiEnableVqe = audio_ntv3_aiEnableVqe;
    pstAudioPlatOps->aiDisableVqe = audio_ntv3_aiDisableVqe;
    pstAudioPlatOps->aiSetVolume = audio_ntv3_aiSetVolume;
    pstAudioPlatOps->aiSetLoopBack = NULL;
    /*tts*/
    pstAudioPlatOps->ttsGetSampleRate = hik_tts_getSampleRate;
    pstAudioPlatOps->ttsStart = hik_tts_start;
    pstAudioPlatOps->ttsProc = hik_tts_proc;
    pstAudioPlatOps->ttsStop = hik_tts_stop;

    return SAL_SOK;
}

