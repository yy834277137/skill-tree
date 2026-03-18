/**
 * @file   audioTTS_drv.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---合成语音组件
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取合成语音接口
 */


/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal_mem_new.h"
#include "platform_hal.h"
#include "audioResample_drv_api.h"
#include "audio_drv_api.h"
#include "audioTTS_drv_api.h"

/*****************************************************************************
                         结构定义
*****************************************************************************/
#define AUDIO_TTS_BUF_LEN		(640 * 2 * 1024)
#define DEF_AUDIO_TTS_SAMPLRATE (8000)
#define DEF_AUDIO_TTS_FRMlEN (DEF_AUDIO_TTS_SAMPLRATE *2/25)

typedef struct
{
    UINT32 chn;

    BOOL bUsed;                      /* tts正在使用中，使用中:1 */
    UINT32 isEnable;                 /* 文字文字转语音使能 */
    UINT32 u32SampleRate;
    AUTO_TTS_HANDLE_PARAM stTtsOutParam; /*tts 输出 buf*/

    /*hik tts*/
    void *pTtsHdl;
    RESAMPLE_INFO_S stResampleProc;
} AUDIO_TTS_DRV_S;


/*****************************************************************************
                            函数
*****************************************************************************/

/**
 * @function   audioTTS_drv_start
 * @brief      创建合成语音句柄
 * @param[in]  UINT32 u32Chan 通道号
 * @param[in]  void *pPrm 合成语音初始化参数(AUTO_TTS_START_PARAM)
 * @param[out] void **ppHandle 合成语音句柄
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioTTS_drv_start(UINT32 u32Chan, void **ppHandle, void *pPrm)
{
    INT32 s32Ret = SAL_SOK;
    AUTO_TTS_START_PARAM *pstTtsStartPrm = NULL;
    AUDIO_TTS_DRV_S *pstTtsCtrl = NULL;

    if ((NULL == pPrm) || (NULL == ppHandle))
    {
        AUD_LOGE("pPrm NULL\n");
        return SAL_FAIL;
    }

    if (*ppHandle)
    {
        AUD_LOGW("tts handle exist!!\n");
        return SAL_SOK;
    }

    pstTtsStartPrm = (AUTO_TTS_START_PARAM *)pPrm;
    if (SAL_FALSE == pstTtsStartPrm->enable)
    {
        AUD_LOGE("voiceType %d, enable %d err\n", pstTtsStartPrm->voiceType, pstTtsStartPrm->enable);
        return SAL_FAIL;
    }

    pstTtsCtrl = SAL_memZalloc(sizeof(AUDIO_TTS_DRV_S), "audio", "tts");
    if (NULL == pstTtsCtrl)
    {
        AUD_LOGE("alloc mem fail\n");
        return SAL_FAIL;
    }

    pstTtsCtrl->stTtsOutParam.txtBuf = SAL_memZalloc(AUDIO_TTS_BUF_LEN, "audio", "tts");
    if (NULL == pstTtsCtrl->stTtsOutParam.txtBuf)
    {
        AUD_LOGE("alloc mem fail\n");
        goto err;
    }

    pstTtsCtrl->u32SampleRate = audio_hal_ttsGetSampleRte();
    if (DEF_AUDIO_TTS_SAMPLRATE != pstTtsCtrl->u32SampleRate)
    {
        pstTtsCtrl->stResampleProc.u32SrcRate = pstTtsCtrl->u32SampleRate;
        pstTtsCtrl->stResampleProc.u32DesRate = 8000;
        pstTtsCtrl->stResampleProc.u32InputBufLen = AUDIO_TTS_BUF_LEN;
        pstTtsCtrl->stResampleProc.u32OutputBufLen = AUDIO_TTS_BUF_LEN;
        pstTtsCtrl->stResampleProc.pInputBuf = SAL_memZalloc(AUDIO_TTS_BUF_LEN, "audio", "tts");
        if (NULL == pstTtsCtrl->stResampleProc.pInputBuf)
        {
            AUD_LOGE("alloc mem fail\n");
            goto err;
        }

        pstTtsCtrl->stResampleProc.pOutputBuf = SAL_memZalloc(AUDIO_TTS_BUF_LEN, "audio", "tts");
        if (NULL == pstTtsCtrl->stResampleProc.pOutputBuf)
        {
            AUD_LOGE("alloc mem fail\n");
            goto err;
        }

        s32Ret = audioResample_drv_create(&pstTtsCtrl->stResampleProc.pResampleHandle, &pstTtsCtrl->stResampleProc);
        if (SAL_SOK != s32Ret)
        {
            AUD_LOGE("audioResample_drv_create fail\n");
            goto err;
        }
    }
    else
    {
        pstTtsCtrl->stResampleProc.pResampleHandle = NULL;
    }

    s32Ret = audio_hal_ttsStart(&pstTtsCtrl->pTtsHdl, pstTtsStartPrm->voiceType);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE("audioResample_drv_create fail\n");
        goto err;
    }

    pstTtsCtrl->chn = u32Chan;
    pstTtsCtrl->isEnable = pstTtsStartPrm->enable;
    pstTtsCtrl->bUsed = SAL_FALSE;

    *ppHandle = (void *)pstTtsCtrl;

    AUD_LOGI("chan %d tts init ok\n", u32Chan);

    return SAL_SOK;

err:
    if (pstTtsCtrl->stTtsOutParam.txtBuf)
    {
        SAL_memfree(pstTtsCtrl->stTtsOutParam.txtBuf, "audio", "tts");
    }

    if (pstTtsCtrl->stResampleProc.pResampleHandle)
    {
        audioResample_drv_destroy(&pstTtsCtrl->stResampleProc.pResampleHandle);
    }

    if (pstTtsCtrl->stResampleProc.pInputBuf)
    {
        SAL_memfree(pstTtsCtrl->stResampleProc.pInputBuf, "audio", "tts");
    }

    if (pstTtsCtrl->stResampleProc.pOutputBuf)
    {
        SAL_memfree(pstTtsCtrl->stResampleProc.pOutputBuf, "audio", "tts");
    }

    SAL_memfree(pstTtsCtrl, "audio", "tts");

    return SAL_FAIL;
}

/**
 * @function   audioTTS_drv_process
 * @brief      合成语音处理,单次处理不能超过196字节
 * @param[in]  void* pHandle 输入句柄
 * @param[in]  void *pPrm 合成语音处理参数(AUTO_TTS_HANDLE_PARAM)
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioTTS_drv_process(void *pHandle, VOID *pPrm, PUINT8 *pDataOut, UINT32 *pDataSize)
{
    AUTO_TTS_HANDLE_PARAM *pstTtsInPrm = NULL;
    AUTO_TTS_HANDLE_PARAM *pstTtsOutPrm = NULL;
    AUDIO_TTS_DRV_S *pstTtsCtrl = NULL;
    AUTO_TTS_PARAM_S stAudoTtsPrm = {0};

    INT32 s32Ret = SAL_SOK;
    UINT32 u32Time0 = 0;
    UINT32 u32Time1 = 0;
    AUDIO_INTERFACE_INFO_S stTmpInterFace = {0};


    if ((NULL == pPrm) || (NULL == pHandle)
        || (NULL == pDataOut) || (NULL == pDataSize))
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    pstTtsCtrl = (AUDIO_TTS_DRV_S *)pHandle;
    if (NULL == pstTtsCtrl->stTtsOutParam.txtBuf)
    {
        AUD_LOGE("txtBuf NULL\n");
        return SAL_FAIL;
    }

    if (pstTtsCtrl->bUsed == SAL_TRUE)
    {
        AUD_LOGE("tts is handling\n");
        return SAL_FAIL;
    }

    pstTtsOutPrm = &pstTtsCtrl->stTtsOutParam;
    memset(pstTtsOutPrm->txtBuf, 0x00, AUDIO_TTS_BUF_LEN);

    pstTtsInPrm = (AUTO_TTS_HANDLE_PARAM *)pPrm;
    if (pstTtsInPrm->txtBuf == NULL)
    {
        AUD_LOGE("txtBuf  NULL\n");
        return SAL_FAIL;
    }

    u32Time0 = SAL_getCurMs();

    if (pstTtsCtrl->isEnable)
    {
        pstTtsCtrl->bUsed = SAL_TRUE;
        stAudoTtsPrm.pIndata = pstTtsInPrm->txtBuf;
        stAudoTtsPrm.u32DataSize = pstTtsInPrm->txtDataLen;

        s32Ret = audio_hal_ttsProc(pstTtsCtrl->pTtsHdl, &stAudoTtsPrm, (PUINT8)pstTtsOutPrm->txtBuf, &pstTtsOutPrm->txtDataLen);
        if (SAL_SOK != s32Ret)
        {
            AUD_LOGE("audio_hal_ttsProc err\n");
            return SAL_FAIL;
        }

        u32Time1 = SAL_getCurMs();
        AUD_LOGI("tts use time %d\n", u32Time1 - u32Time0);

        if (DEF_AUDIO_TTS_SAMPLRATE != pstTtsCtrl->u32SampleRate)
        {
            stTmpInterFace.pInputBuf = (UINT8 *)pstTtsOutPrm->txtBuf;         /*input buf == out put buf*/
            stTmpInterFace.pOutputBuf = (UINT8 *)pstTtsOutPrm->txtBuf;
            stTmpInterFace.uiInputLen = pstTtsOutPrm->txtDataLen;

            s32Ret = audioResample_drv_Process(pstTtsCtrl->stResampleProc.pResampleHandle, &stTmpInterFace);
            if (SAL_SOK != s32Ret)
            {
                AUD_LOGE("resample proc err\n");
            }

            memset(pstTtsOutPrm->txtBuf + stTmpInterFace.uiOutputLen, 0, AUDIO_TTS_BUF_LEN - stTmpInterFace.uiOutputLen);
            pstTtsOutPrm->txtDataLen = (stTmpInterFace.uiOutputLen + (DEF_AUDIO_TTS_FRMlEN - 1)) / DEF_AUDIO_TTS_FRMlEN * DEF_AUDIO_TTS_FRMlEN;
            
            u32Time0 = SAL_getCurMs();
            AUD_LOGI("tts resample use time %d\n", u32Time0 - u32Time1);
        }
        else
        {
            /*不需要重采样*/
        }

        AUD_LOGI("to app, audio pcm len: %d-%d.\n", pstTtsOutPrm->txtDataLen, stTmpInterFace.uiOutputLen);

        *pDataOut = (PUINT8)pstTtsOutPrm;
        *pDataSize = sizeof(AUTO_TTS_HANDLE_PARAM);

        pstTtsCtrl->bUsed = SAL_FALSE;

#if 0
        FILE *pFile = fopen("./ai/tts.pcm", "a+");
        if (pFile)
        {
            fwrite(pstTtsOutPrm->txtBuf, 1, pstTtsOutPrm->txtDataLen, pFile);
            fflush(pFile);
            fclose(pFile);
        }

#endif

/*test:tts->vo*/
#if 0
        AUDIO_BUF_PRM_S stAudioBuf = {0};
        stAudioBuf.u32BufSize = pstTtsOutPrm->txtDataLen;
        stAudioBuf.pDataBuf = SAL_memZalloc(stAudioBuf.u32BufSize, "audio", "tts");
        if (stAudioBuf.pDataBuf)
        {
            audio_drv_aoSendFrame((UINT8 *)pstTtsOutPrm->txtBuf, pstTtsOutPrm->txtDataLen, &stAudioBuf);
            SAL_memfree(stAudioBuf.pDataBuf, "audio", "tts");
        }

#endif
        return SAL_SOK;
    }
    else
    {
        AUD_LOGE("chan %d tts disable\n", pstTtsCtrl->chn);
        return SAL_FAIL;
    }
}

/**
 * @function   audioTTS_drv_stop
 * @brief      关闭合成语音处理
 * @param[in]  void** ppHandle 输入句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioTTS_drv_stop(void **ppHandle)
{
    INT32 s32Ret = SAL_SOK;
    AUDIO_TTS_DRV_S *pstTtsCtrl = NULL;

    if (NULL == ppHandle)
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    pstTtsCtrl = (AUDIO_TTS_DRV_S *)(*ppHandle);
    if (NULL == pstTtsCtrl)
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    s32Ret = audio_hal_ttsStop(&pstTtsCtrl->pTtsHdl);
    if (SAL_SOK != s32Ret)
    {
        AUD_LOGE("audio_hal_ttsStop err\n");
    }

    if (pstTtsCtrl->stTtsOutParam.txtBuf)
    {
        SAL_memfree(pstTtsCtrl->stTtsOutParam.txtBuf, "audio", "tts");
    }

    if (pstTtsCtrl->stResampleProc.pResampleHandle)
    {
        audioResample_drv_destroy(&pstTtsCtrl->stResampleProc.pResampleHandle);

        if (pstTtsCtrl->stResampleProc.pInputBuf)
        {
            SAL_memfree(pstTtsCtrl->stResampleProc.pInputBuf, "audio", "tts");
        }

        if (pstTtsCtrl->stResampleProc.pOutputBuf)
        {
            SAL_memfree(pstTtsCtrl->stResampleProc.pOutputBuf, "audio", "tts");
        }
    }

    SAL_memfree(pstTtsCtrl, "audio", "tts");
    *ppHandle = NULL;
    AUD_LOGI("hik tts stop\n");

    return SAL_SOK;
}

