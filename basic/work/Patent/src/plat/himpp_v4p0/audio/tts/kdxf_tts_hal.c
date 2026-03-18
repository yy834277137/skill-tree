/**
 * @file   kdxf_tts_hal.c
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
#include "msp_cmn.h"
#include "msp_types.h"
#include "msp_errors.h"
#include "qtts.h"
#include "kdxf_tts_hal.h"

/*****************************************************************************
                         结构定义
*****************************************************************************/
#define AUDIO_TTS_BUF_LEN (640 * 2 * 1024)

typedef enum _GENDER_TYPE_
{
    FEMALE = 0,                          /* 女声 */
    MALE = 1,                            /* 男声 */
    GENDER_ERR = 2,                      /* 男声 */
} GENDER_TYPE_E;

typedef struct
{
    GENDER_TYPE_E u32VoiceType;             /* 0女声，1男声 */

    PUINT8 pInTxtBuf;        /* malloc when initializing*/
    UINT32 u32TxtBufLen;
} KDXF_TTS_DEV_S;


/*****************************************************************************
                            函数
*****************************************************************************/

/**
 * @function   kdxf_tts_getSampleRate
 * @brief      tts语义转换开启
 * @param[in]  void
 * @param[out] None
 * @return     TTS输出采样率
 */
UINT32 kdxf_tts_getSampleRate(void)
{
    return 8000;
}

/**
 * @function   kdxf_tts_start
 * @brief      创建合成语音句柄
 * @param[in]  void **ppHandle
 * @param[in]  UINT32 u32VoiceType
 * @param[out] void **ppHandle 合成语音句柄
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 kdxf_tts_start(void **ppHandle, UINT32 u32VoiceType)
{
    KDXF_TTS_DEV_S *pstTtsCtrl = NULL;
    INT32 s32Ret = SAL_FAIL;
    const char *pLoginPrm = "appid = 5c8b256f, work_dir =.";

    if ((NULL == ppHandle) || (GENDER_ERR <= u32VoiceType))
    {
        AUD_LOGE("pPrm %p voice type %d\n", ppHandle, u32VoiceType);
        return SAL_FAIL;
    }

    if (*ppHandle)
    {
        AUD_LOGW("tts handle exist!!\n");
        return SAL_SOK;
    }

    pstTtsCtrl = SAL_memZalloc(sizeof(KDXF_TTS_DEV_S), "audio", "tts");
    if (NULL == pstTtsCtrl)
    {
        AUD_LOGE("alloc mem fail\n");
        return SAL_FAIL;
    }

    pstTtsCtrl->u32TxtBufLen = AUDIO_TTS_BUF_LEN;
    pstTtsCtrl->pInTxtBuf = (PUINT8)SAL_memZalloc(pstTtsCtrl->u32TxtBufLen, "audio", "tts");
    if (NULL == pstTtsCtrl->pInTxtBuf)
    {
        AUD_LOGE("alloc mem fail\n");
        goto err;
    }

    s32Ret = MSPLogin(NULL, NULL, pLoginPrm);
    if (MSP_SUCCESS != s32Ret)
    {
        AUD_LOGE("MSPLogin failed, error code: %d.\n", s32Ret);
        goto err;
    }

    pstTtsCtrl->u32VoiceType = u32VoiceType;

    *ppHandle = (void *)pstTtsCtrl;

    AUD_LOGI("kdxf tts init ok\n");

    return SAL_SOK;
err:

    if (pstTtsCtrl->pInTxtBuf)
    {
        SAL_memfree(pstTtsCtrl->pInTxtBuf, "audio", "tts");
    }

    if (pstTtsCtrl)
    {
        SAL_memfree(pstTtsCtrl, "audio", "tts");
    }

    return SAL_FAIL;

}

/**
 * @function   kdxf_tts_proc
 * @brief      合成语音处理
 * @param[in]  void* pHandle 输入句柄
 * @param[in]  PUINT8 pDataIn 输入数据
 * @param[in]  UINT32 u32InDataSize 输入数据大小
 * @param[out] PUINT8 pDataOut
 * @param[out] UINT32 *pDataSize
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 kdxf_tts_proc(void *pHandle, PUINT8 pDataIn, UINT32 u32InDataSize, PUINT8 pDataOut, UINT32 *pDataSize)
{
    KDXF_TTS_DEV_S *pstTtsCtrl = NULL;
    UINT32 u32AudioTtsSize = 0;
    unsigned char *pTxtaddr = NULL;

    const void *pOutData = NULL;
    char sessionBeginParams[1024] = {0};
    INT32 s32Ret = SAL_FAIL;
    const char *pSessionID = NULL;
    UINT32 u32AudioLen = 0;
    UINT32 u32DataSize = 0;                              /* = ~{4?J}>]3$6H~} */
    INT32 s32SynthStatus = MSP_TTS_FLAG_STILL_HAVE_DATA;
    char curPath[128] = {0};

    if ((NULL == pDataIn) || (NULL == pHandle)
        || (NULL == pDataOut) || (NULL == pDataSize)
        || (0 == u32InDataSize))
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    pstTtsCtrl = (KDXF_TTS_DEV_S *)pHandle;

    u32AudioTtsSize = u32InDataSize;
    if (u32AudioTtsSize > pstTtsCtrl->u32TxtBufLen)
    {
        AUD_LOGW("txt data overflow %d->%d.\n", u32AudioTtsSize, pstTtsCtrl->u32TxtBufLen);
        u32AudioTtsSize = pstTtsCtrl->u32TxtBufLen;
    }

    pTxtaddr = pstTtsCtrl->pInTxtBuf;
    memset(pTxtaddr, 0x00, pstTtsCtrl->u32TxtBufLen);
    memcpy(pTxtaddr, pDataIn, u32AudioTtsSize);
    getcwd(curPath, 128);

    if (FEMALE == pstTtsCtrl->u32VoiceType)
    {
        snprintf(sessionBeginParams, 1024, "engine_type = local,voice_name=xiaoxue, text_encoding = UTF8, tts_res_path = fo|%s/tts/xiaoxue.jet;fo|%s/tts/common.jet, sample_rate = 8000, speed = 50, volume = 100, pitch = 50, rdn = 3", curPath, curPath);
    }
    else
    {
        snprintf(sessionBeginParams, 1024, "engine_type = local,voice_name=jiuxu, text_encoding = UTF8, tts_res_path = fo|%s/tts/jiuxu.jet;fo|%s/tts/common.jet, sample_rate = 8000, speed = 50, volume = 100, pitch = 50, rdn = 3", curPath, curPath);
    }

    pSessionID = QTTSSessionBegin(sessionBeginParams, &s32Ret);
    if ((NULL == pSessionID) || (MSP_SUCCESS != s32Ret))
    {
        AUD_LOGE("QTTSSessionBegin failed, error code: %d. pSessionID %p\n", s32Ret, pSessionID);
        return SAL_FAIL;
    }

    s32Ret = QTTSTextPut(pSessionID, (const char *)pTxtaddr, u32AudioTtsSize, NULL);
    if (MSP_SUCCESS != s32Ret)
    {
        AUD_LOGE("QTTSTextPut failed, error code: %d.\n", s32Ret);
        QTTSSessionEnd(pSessionID, "TextPutError");
        return SAL_FAIL;
    }

    AUD_LOGI("tts in string: %s.\n", pTxtaddr);

    while (1)
    {
        pOutData = QTTSAudioGet(pSessionID, &u32AudioLen, &s32SynthStatus, &s32Ret);
        if (MSP_SUCCESS != s32Ret)
        {
            break;
        }

        if ((NULL != pOutData) && (0 != u32AudioLen))
        {
            if ((u32DataSize + u32AudioLen) >= AUDIO_TTS_BUF_LEN)
            {
                AUD_LOGE("size is too big %d\n", (u32DataSize + u32AudioLen));
                break;
            }

            memcpy(pDataOut + u32DataSize, pOutData, u32AudioLen);
            u32DataSize += u32AudioLen;
        }

        if (MSP_TTS_FLAG_DATA_END == s32SynthStatus)
        {
            AUD_LOGI("tts proc finish\n");
            break;
        }
    }

    s32Ret = QTTSSessionEnd(pSessionID, "Normal");
    if (MSP_SUCCESS != s32Ret)
    {
        AUD_LOGE("QTTSSessionEnd failed, error code: %d.\n", s32Ret);
        return SAL_FAIL;
    }

    *pDataSize = (u32DataSize + 639) / 640 * 640;
    AUD_LOGI("to app, audio pcm len: %d-%d.\n", u32DataSize, *pDataSize);


#if 0
    FILE *pFile = fopen("/home/config/audio_tts.pcm", "wb");
    if (pFile)
    {
        fwrite(pDataOut, 1, *pDataSize, pFile);
        fclose(pFile);
    }

#endif

    AUD_LOGI("%s success\n", __func__);
    return SAL_SOK;

}

/**
 * @function   kdxf_tts_stop
 * @brief      关闭合成语音处理
 * @param[in]  void** ppHandle 输入句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 kdxf_tts_stop(void **ppHandle)
{
    KDXF_TTS_DEV_S *pstTtsCtrl = NULL;

    if (NULL == ppHandle)
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    pstTtsCtrl = (KDXF_TTS_DEV_S *)(*ppHandle);
    if (NULL == pstTtsCtrl)
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    MSPLogout();

    if (pstTtsCtrl->pInTxtBuf)
    {
        SAL_memfree(pstTtsCtrl->pInTxtBuf, "audio", "tts");
    }

    if (pstTtsCtrl->pInTxtBuf)
    {
        SAL_memfree(pstTtsCtrl, "audio", "tts");
    }

    *ppHandle = NULL;
    AUD_LOGI("kdxf tts stop\n");


    return SAL_SOK;
}

