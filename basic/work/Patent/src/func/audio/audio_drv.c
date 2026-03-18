/**
 * @file   audio_drv.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---hal层接口调用
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建audio_drv.c文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#include "platform_hal.h"
#include "audio_drv_api.h"


/*****************************************************************************
                            宏定义
*****************************************************************************/

/*****************************************************************************
                            全局结构体
*****************************************************************************/


/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   audio_drv_setAudioTalkBack
 * @brief      开始/结束 SDK 对讲
 * @param[in]  UINT32 u32AudioChan 音频通道
 * @param[in]  void *pBuf 对讲使能参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_setAudioTalkBack(UINT32 u32AudioChan, void *pBuf)
{
    DSP_AUDIO_PARAM *pstAudioPrm = NULL;
    INT32 bEnable = SAL_FALSE;
    INT32 s32Ret = SAL_FAIL;

    if (NULL == pBuf)
    {
        AUD_LOGE("buf Null\n");
        return SAL_FAIL;
    }

    pstAudioPrm = (DSP_AUDIO_PARAM *)pBuf;
    bEnable = pstAudioPrm->bEnable;

    s32Ret = audio_hal_setVqe(u32AudioChan, bEnable);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE("HAL_AIO_AiEnableVqe error\n");
    }

    return SAL_SOK;
}

/**
 * @function   audio_drv_setAiVolume
 * @brief      配置输入音量等级
 * @param[in]  UINT32 u32AudioChan 音频通道，当前无效
 * @param[in]  void *pstAudioVol 音频信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_setAiVolume(UINT32 u32AudioChan, void *pstAudioVol)
{
    INT32 s32Ret = 0;
    UINT32 u32AiChn = 0;
    UINT32 u32AiVolume = 0;
    AUDIO_VOL_PRM_S *pstVol = NULL;

    /* 校验参数是否为空 */
    if (NULL == pstAudioVol)
    {
        AUD_LOGE("pu32Volume is Null\n");
        return SAL_FAIL;
    }

    pstVol = (AUDIO_VOL_PRM_S *)pstAudioVol;
    u32AiChn = pstVol->uiChn;
    u32AiVolume = pstVol->uiVol;

    s32Ret = audio_hal_setAiVolume(u32AiChn, u32AiVolume);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE("Audio_halSetAiVolume err %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_drv_setAoVolume
 * @brief      配置输出音量等级
 * @param[in]  UINT32 u32AudioChan 音频通道，当前无效
 * @param[in]  void *pstAudioVol 音频信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_setAoVolume(UINT32 u32AudioChan, void *pstAudioVol)
{
    INT32 s32Ret = 0;
    UINT32 u32AoChn = 0;
    UINT32 u32AoVolume = 0;
    AUDIO_VOL_PRM_S *pstVol = NULL;

    /* 校验参数是否为空 */
    if (NULL == pstAudioVol)
    {
        AUD_LOGE("pu32Volume is Null\n");
        return SAL_FAIL;
    }

    pstVol = (AUDIO_VOL_PRM_S *)pstAudioVol;
    u32AoChn = pstVol->uiChn;
    u32AoVolume = pstVol->uiVol;

    s32Ret = audio_hal_setAoVolume(u32AoChn, u32AoVolume);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE("Audio_halSetAoVolume err %#x!\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_drv_aiGetFrame
 * @brief      获取一帧音频数据
 * @param[in]  SAL_AudioFrameBuf **pstAudFrmBuf 音频采集到的PCM数据
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_aiGetFrame(SAL_AudioFrameBuf **pstAudFrmBuf)
{
    INT32 s32Ret = 0;

    if (NULL == pstAudFrmBuf)
    {
        AUD_LOGE("pstInPrm is NULL!\n");
        return SAL_FAIL;
    }

    s32Ret = audio_hal_aiGetFrame(pstAudFrmBuf);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE("========== Get audio frame failed =======\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   audio_drv_aoSendFrame
 * @brief      播放一帧音频数据
 * @param[in]  UINT8 *pu8AudioDat 待输出音频数据
 * @param[in]  UINT32 u32len 音频数据长度
 * @param[in]  AUDIO_BUF_PRM_S *pstAoBufPrm 音频输出中转buf
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_aoSendFrame(UINT8 *pu8AudioDat, UINT32 u32len, AUDIO_BUF_PRM_S *pstAoBufPrm)
{
    INT32 s32Ret = 0;
    UINT32 u32InLen = u32len;
    HAL_AIO_FRAME_S stAudioFrame = {0};
    CAPB_AUDIO *pstCapbAudio = NULL;

    if ((NULL == pu8AudioDat) || (NULL == pstAoBufPrm))
    {
        AUD_LOGE("pstInPrm is NULL!\n");
        return SAL_FAIL;
    }
    if ((NULL == pstAoBufPrm->pDataBuf))
    {
        AUD_LOGE("Need Audio Init!\n");
        return SAL_FAIL;
    }
    if (0 == u32InLen)
    {
        AUD_LOGE("==== length of data is 0 ======\n");
        return SAL_FAIL;
    }

    pstCapbAudio = capb_get_audio();
    if (pstCapbAudio == NULL)
    {
        AUD_LOGE("null err\n");
        return SAL_FAIL;
    }

    memset(&stAudioFrame, 0, sizeof(HAL_AIO_FRAME_S));
    stAudioFrame.AudioDevIdx = 0;
    stAudioFrame.Channel = 0;
    stAudioFrame.FrameLen = u32InLen;
    memcpy(pstAoBufPrm->pDataBuf, pu8AudioDat, u32InLen);
    stAudioFrame.pBufferAddr = pstAoBufPrm->pDataBuf;
    s32Ret = audio_hal_aoSendFrame(&stAudioFrame);
    if (SAL_SOK != s32Ret)
    {
    	AUD_LOGE("HAL_AIO_AoSendFrame AO(%d.%d) fail %#x\n", stAudioFrame.AudioDevIdx, stAudioFrame.Channel, s32Ret);
    }
    return SAL_SOK;
}
/**
 * @function   audio_drv_deInit
 * @brief      audio drv层反初始化资源
 * @param[in]  void
 * @param[out] None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_deInit(void)
{
    return SAL_SOK;
}


/**
 * @function   audio_drv_init
 * @brief      audio drv层初始化资源
 * @param[in]  void
 * @param[out] None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_init(void)
{
    /* 音频 HAL 层初始化 */
    if (SAL_SOK != audio_hal_create())
    {
        AUD_LOGE("audio_hal_create !!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

