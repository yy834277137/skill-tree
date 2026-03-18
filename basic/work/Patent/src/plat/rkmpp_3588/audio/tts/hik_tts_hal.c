/**
 * @file   hik_tts_hal.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---合成语音组件
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2022/01/04
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取合成语音接口
 */


/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal_mem_new.h"
#include "hikspeech-ltts-lib.h"
#include "hik_tts_hal.h"

/*****************************************************************************
                         结构定义
*****************************************************************************/
#define AUDIO_TTS_BUF_LEN	(640 * 2 * 1024)
#define AUDIO_TTS_TXT_LEN	(196)

typedef enum _GENDER_TYPE_
{
    FEMALE = 0,                          /* 女声 */
    MALE = 1,                            /* 男声 */
    GENDER_ERR = 2,                      /* 男声 */
} GENDER_TYPE_E;

typedef struct
{
    GENDER_TYPE_E u32VoiceType;             /* 0女声，1男声 */

    void *pTtsHdl;
    PUINT8 pOutBuf;
    HIKSPEECH_LTTS_INPUT stInPut;
    HIKSPEECH_LTTS_MEM_TAB stLttsMem;
    HIKSPEECH_LTTS_MEM_TAB stProcMem;
    HIKSPEECH_LTTS_MODEL_HANDLE stModHdl;
} HIK_TTS_DEV_S;


/*****************************************************************************
                            函数
*****************************************************************************/

/**
 * @function   hik_tts_getSampleRate
 * @brief      tts语义转换开启
 * @param[in]  void
 * @param[out] None
 * @return     TTS输出采样率
 */
UINT32 hik_tts_getSampleRate(void)
{
    return 8000;
}

/**
 * @function   hik_tts_start
 * @brief      创建合成语音句柄
 * @param[in]  void **ppHandle
 * @param[in]  UINT32 u32VoiceType
 * @param[out] void **ppHandle 合成语音句柄
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_tts_start(void **ppHandle, UINT32 u32VoiceType)
{
    HRESULT hrst = 0;
    HIKSPEECH_LTTS_MODEL_PARAM ltts_model_param = {0};
    HIK_TTS_DEV_S *pstTtsCtrl = NULL;

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

    pstTtsCtrl = SAL_memZalloc(sizeof(HIK_TTS_DEV_S), "audio", "tts");
    if (NULL == pstTtsCtrl)
    {
        AUD_LOGE("alloc mem fail\n");
        return SAL_FAIL;
    }

    pstTtsCtrl->pOutBuf = SAL_memZalloc(AUDIO_TTS_BUF_LEN, "audio", "tts");
    if (NULL == pstTtsCtrl->pOutBuf)
    {
        AUD_LOGE("alloc mem fail\n");
        return SAL_FAIL;
    }
#if 1/*与应用调试使用*/
    /* set model param */
    if (FEMALE == u32VoiceType)
    {
        strcpy(ltts_model_param.py_model_path, "/home/config/source/res/hik_tts/models/py_model.dat");
        strcpy(ltts_model_param.melgan_model_path, "/home/config/source/res/hik_tts/models/hik_hifigan_female.model");
        strcpy(ltts_model_param.fs2_model_path, "/home/config/source/res/hik_tts/models/fs2_model_female.bin");
        strcpy(ltts_model_param.fs2_py2ph_path, "/home/config/source/res/hik_tts/models/fs2_py2ph_female.txt");
        strcpy(ltts_model_param.fs2_vocab_path, "/home/config/source/res/hik_tts/models/fs2_symbols_female.txt");
    }
    else
    {
        strcpy(ltts_model_param.py_model_path, "/home/config/source/res/hik_tts/models/py_model.dat");
        strcpy(ltts_model_param.melgan_model_path, "/home/config/source/res/hik_tts/models/hik_hifigan_male.model");
        strcpy(ltts_model_param.fs2_model_path, "/home/config/source/res/hik_tts/models/fs2_model_male.bin");
        strcpy(ltts_model_param.fs2_py2ph_path, "/home/config/source/res/hik_tts/models/fs2_py2ph_male.txt");
        strcpy(ltts_model_param.fs2_vocab_path, "/home/config/source/res/hik_tts/models/fs2_symbols_male.txt");
    }
#endif

#if 0
    /* set model param */
    if (FEMALE == u32VoiceType)
    {
        strcpy(ltts_model_param.py_model_path, "./hik_tts/models/py_model.dat");
        strcpy(ltts_model_param.melgan_model_path, "./hik_tts/models/hik_hifigan_female.model");
        strcpy(ltts_model_param.fs2_model_path, "./hik_tts/models/fs2_model_female.bin");
        strcpy(ltts_model_param.fs2_py2ph_path, "./hik_tts/models/fs2_py2ph_female.txt");
        strcpy(ltts_model_param.fs2_vocab_path, "./hik_tts/models/fs2_symbols_female.txt");
    }
    else
    {
        strcpy(ltts_model_param.py_model_path, "./hik_tts/models/py_model.dat");
        strcpy(ltts_model_param.melgan_model_path, "./hik_tts/models/hik_hifigan_male.model");
        strcpy(ltts_model_param.fs2_model_path, "./hik_tts/models/fs2_model_male.bin");
        strcpy(ltts_model_param.fs2_py2ph_path, "./hik_tts/models/fs2_py2ph_male.txt");
        strcpy(ltts_model_param.fs2_vocab_path, "./hik_tts/models/fs2_symbols_male.txt");
    }
#endif
    hrst = HIKSPEECH_LTTS_LIB_GetModelMemSize(&ltts_model_param, &pstTtsCtrl->stLttsMem);
    if (HIKSPEECH_LIB_S_OK != hrst)
    {
        AUD_LOGE("Get Model Memory Size is Wrong! hrst: %#X\n", hrst);
        goto err;
    }
    //pstTtsCtrl->stLttsMem.pp_mem_tab.base = SAL_memAlign(pstTtsCtrl->stLttsMem.pp_mem_tab.alignment, pstTtsCtrl->stLttsMem.pp_mem_tab.size, "audio", "tts");
    pstTtsCtrl->stLttsMem.pinyin_mem_tab.base = SAL_memAlign(16, pstTtsCtrl->stLttsMem.pinyin_mem_tab.size, "audio", "tts");
    pstTtsCtrl->stLttsMem.fs2_mem_tab.base = SAL_memAlign(pstTtsCtrl->stLttsMem.fs2_mem_tab.alignment, pstTtsCtrl->stLttsMem.fs2_mem_tab.size, "audio", "tts");
    pstTtsCtrl->stLttsMem.melgan_mem_tab.base = SAL_memAlign(pstTtsCtrl->stLttsMem.melgan_mem_tab.alignment, pstTtsCtrl->stLttsMem.melgan_mem_tab.size, "audio", "tts");
    if ((NULL == pstTtsCtrl->stLttsMem.pinyin_mem_tab.base)
        || (NULL == pstTtsCtrl->stLttsMem.fs2_mem_tab.base)
        || (NULL == pstTtsCtrl->stLttsMem.melgan_mem_tab.base))
    {
        AUD_LOGE("alloc mem fail\n");
        goto err;
    }

    //AUD_LOGI("pp_mem_tab.base:%p .size: %f M\n", pstTtsCtrl->stLttsMem.pp_mem_tab.base, pstTtsCtrl->stLttsMem.pp_mem_tab.size / (1024.0 * 1024.0));
    AUD_LOGI("pinyin_mem_tab.base:%p size: %f M\n", pstTtsCtrl->stLttsMem.pinyin_mem_tab.base, pstTtsCtrl->stLttsMem.pinyin_mem_tab.size / (1024.0 * 1024.0));
    AUD_LOGI("fs2_mem_tab.base:%p size: %f M\n", pstTtsCtrl->stLttsMem.fs2_mem_tab.base, pstTtsCtrl->stLttsMem.fs2_mem_tab.size / (1024.0 * 1024.0));
    AUD_LOGI("melgan_mem_tab.base:%p size: %f M\n\n", pstTtsCtrl->stLttsMem.melgan_mem_tab.base, pstTtsCtrl->stLttsMem.melgan_mem_tab.size / (1024.0 * 1024.0));

    hrst = HIKSPEECH_LTTS_LIB_CreateModel((void *)&pstTtsCtrl->stModHdl, &ltts_model_param, &pstTtsCtrl->stLttsMem);
    if (HIKSPEECH_LIB_S_OK != hrst)
    {
        AUD_LOGE("Create Model is Wrong! hrst: %X\n", hrst);
        goto err2;
    }

    hrst = HIKSPEECH_LTTS_LIB_GetMemSize((void *)&pstTtsCtrl->stModHdl, &pstTtsCtrl->stProcMem);
    if (HIKSPEECH_LIB_S_OK != hrst)
    {
        AUD_LOGE("Get MemSize is Wrong! hrst: %X\n", hrst);
        goto err2;
    }

    //pstTtsCtrl->stProcMem.pp_mem_tab.base = SAL_memAlign(pstTtsCtrl->stProcMem.pp_mem_tab.alignment, pstTtsCtrl->stProcMem.pp_mem_tab.size, "audio", "tts");
    pstTtsCtrl->stProcMem.pinyin_mem_tab.base = SAL_memAlign(16, pstTtsCtrl->stProcMem.pinyin_mem_tab.size, "audio", "tts");/*研究院需要16字节对齐*/
    pstTtsCtrl->stProcMem.fs2_mem_tab.base = SAL_memAlign(pstTtsCtrl->stProcMem.fs2_mem_tab.alignment, pstTtsCtrl->stProcMem.fs2_mem_tab.size, "audio", "tts");
    pstTtsCtrl->stProcMem.melgan_mem_tab.base = SAL_memAlign(pstTtsCtrl->stProcMem.melgan_mem_tab.alignment, pstTtsCtrl->stProcMem.melgan_mem_tab.size, "audio", "tts");
    pstTtsCtrl->stProcMem.ltts_mem_tab.base = SAL_memAlign(pstTtsCtrl->stProcMem.ltts_mem_tab.alignment, pstTtsCtrl->stProcMem.ltts_mem_tab.size, "audio", "tts");
    if ( (NULL == pstTtsCtrl->stProcMem.pinyin_mem_tab.base)
        || (NULL == pstTtsCtrl->stProcMem.fs2_mem_tab.base)
        || (NULL == pstTtsCtrl->stProcMem.ltts_mem_tab.base)
        || (NULL == pstTtsCtrl->stProcMem.melgan_mem_tab.base))
    {
        AUD_LOGE("alloc mem fail\n");
        goto err1;
    }

    //AUD_LOGI("pstTtsCtrl->stProcMem->pp_mem_tab.base %p size: %f M\n", pstTtsCtrl->stProcMem.pp_mem_tab.base, pstTtsCtrl->stProcMem.pp_mem_tab.size / (1024.0 * 1024.0));
    AUD_LOGI("pstTtsCtrl->stProcMem->pinyin_mem_tab.base %p size: %f M\n", pstTtsCtrl->stProcMem.pinyin_mem_tab.base, pstTtsCtrl->stProcMem.pinyin_mem_tab.size / (1024.0 * 1024.0));
    AUD_LOGI("pstTtsCtrl->stProcMem->fs2_mem_tab.base %p size: %f M\n", pstTtsCtrl->stProcMem.fs2_mem_tab.base, pstTtsCtrl->stProcMem.fs2_mem_tab.size / (1024.0 * 1024.0));
    AUD_LOGI("pstTtsCtrl->stProcMem->melgan_mem_tab.base %p size: %f M\n", pstTtsCtrl->stProcMem.melgan_mem_tab.base, pstTtsCtrl->stProcMem.melgan_mem_tab.size / (1024.0 * 1024.0));
    AUD_LOGI("pstTtsCtrl->stProcMem->pstTtsCtrl->stLttsMem.base %p size: %f M\n\n", pstTtsCtrl->stProcMem.ltts_mem_tab.base, pstTtsCtrl->stProcMem.ltts_mem_tab.size / (1024.0 * 1024.0));

    hrst = HIKSPEECH_LTTS_LIB_Create((void *)&pstTtsCtrl->stModHdl, &pstTtsCtrl->stProcMem, &pstTtsCtrl->pTtsHdl);
    if (HIKSPEECH_LIB_S_OK != hrst)
    {
        AUD_LOGE("Create Model handle is Wrong! hrst: %X\n", hrst);
        goto err1;
    }

    pstTtsCtrl->u32VoiceType = u32VoiceType;

    *ppHandle = (void *)pstTtsCtrl;

    AUD_LOGI("hik tts init ok\n");

    return SAL_SOK;
err1:
 

    if (pstTtsCtrl->stProcMem.pinyin_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stProcMem.pinyin_mem_tab.base, "audio", "tts");
    }

    if (pstTtsCtrl->stProcMem.fs2_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stProcMem.fs2_mem_tab.base, "audio", "tts");
    }

    if (pstTtsCtrl->stProcMem.melgan_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stProcMem.melgan_mem_tab.base, "audio", "tts");
    }

    if (pstTtsCtrl->stProcMem.ltts_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stProcMem.ltts_mem_tab.base, "audio", "tts");
    }

err2:
  

    if (pstTtsCtrl->stLttsMem.pinyin_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stLttsMem.pinyin_mem_tab.base, "audio", "tts");
    }

    if (pstTtsCtrl->stLttsMem.fs2_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stLttsMem.fs2_mem_tab.base, "audio", "tts");
    }

    if (pstTtsCtrl->stLttsMem.melgan_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stLttsMem.melgan_mem_tab.base, "audio", "tts");
    }

err:
    SAL_memfree(pstTtsCtrl, "audio", "tts");

    return SAL_FAIL;
}

/**
 * @function   hik_tts_proc
 * @brief      合成语音处理,单次处理不能超过196字节
 * @param[in]  void* pHandle 输入句柄
 * @param[in]  PUINT8 pDataIn 输入数据
 * @param[in]  UINT32 u32InDataSize 输入数据大小
 * @param[out] PUINT8 pDataOut
 * @param[out] UINT32 *pDataSize
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_tts_proc(void *pHandle, PUINT8 pDataIn, UINT32 u32InDataSize, PUINT8 pDataOut, UINT32 *pDataSize)
{
    HRESULT hrst = HIKSPEECH_LIB_S_OK;
    UINT32 u32AudioTtsSize = 0;
    FLOAT32 f32WaveTime = 0.0;
    FLOAT32 f32Rtf = 0.0;
    UINT32 u32StartTime = SAL_getCurMs();
    UINT32 u32EndTime = 0;
    HIK_TTS_DEV_S *pstTtsCtrl = NULL;
    HIKSPEECH_LTTS_OUTPUT stOutPut = {0};

    if ((NULL == pDataIn) || (NULL == pHandle)
        || (NULL == pDataOut) || (NULL == pDataSize)
        || (0 == u32InDataSize))
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    pstTtsCtrl = (HIK_TTS_DEV_S *)pHandle;

    u32AudioTtsSize = u32InDataSize;
    if (u32AudioTtsSize > AUDIO_TTS_TXT_LEN)
    {
        u32AudioTtsSize = AUDIO_TTS_TXT_LEN;
        SAL_ERROR("tts text too long %d max %d\n", u32InDataSize, AUDIO_TTS_TXT_LEN);
    }

    AUD_LOGI("tts data %s size %d\n", pDataIn, u32InDataSize);

    memset(pstTtsCtrl->stInPut.ltts_data, 0, LTTS_MAX_SENT_TOKENS);
    memcpy(pstTtsCtrl->stInPut.ltts_data, pDataIn, u32AudioTtsSize);
	pstTtsCtrl->stInPut.pace = 1;                                         /*语速先设置为1，是否需要从上层设置*/
    stOutPut.output_wav = (short *)pstTtsCtrl->pOutBuf;
    stOutPut.eos_flag = 0;
    while (stOutPut.eos_flag != 1 && hrst == HIKSPEECH_LIB_S_OK)
    {
        hrst = HIKSPEECH_LTTS_LIB_Process(pstTtsCtrl->pTtsHdl, &pstTtsCtrl->stInPut, &stOutPut);
        if (HIKSPEECH_LIB_S_OK != hrst)
        {
            AUD_LOGE("Process is Wrong! hrst: %X\n", hrst);
        }
    }

    //*pDataSize = (stOutPut.output_wav_length + 1279) / 1280 * 1280;    /*16k采样*/
    *pDataSize = stOutPut.output_wav_length * sizeof(short);/*研究院算法输出数据地址为short类型*/
    memcpy(pDataOut, pstTtsCtrl->pOutBuf, *pDataSize);

    u32EndTime = SAL_getCurMs();

    //f32WaveTime = (stOutPut.output_wav_length) / 16000.0;
    f32WaveTime = (stOutPut.output_wav_length) / 16000.0;
    f32Rtf = (FLOAT32)((u32EndTime - u32StartTime) / (1000.0 * f32WaveTime));
    AUD_LOGI("tts ---, audio pcm len: %d-%d. rtf %f\n", stOutPut.output_wav_length, *pDataSize, f32Rtf);

#if 0
    FILE *pFile = fopen("./ai/tts_8k.pcm", "w+");
    if (pFile)
    {
        fwrite(pDataOut, 1, *pDataSize, pFile);
        fflush(pFile);
        fclose(pFile);
    }

#endif
    /* 一整句话完成后重置 */
    hrst = HIKSPEECH_LTTS_LIB_Reset(pstTtsCtrl->pTtsHdl, &stOutPut);
    if (HIKSPEECH_LIB_S_OK != hrst)
    {
        AUD_LOGE("reset is Wrong! hrst: %X\n", hrst);
    }

    return SAL_SOK;
}

/**
 * @function   hik_tts_stop
 * @brief      关闭合成语音处理
 * @param[in]  void** ppHandle 输入句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_tts_stop(void **ppHandle)
{
    HRESULT hrst = 0;
    HIK_TTS_DEV_S *pstTtsCtrl = NULL;

    if (NULL == ppHandle)
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    pstTtsCtrl = (HIK_TTS_DEV_S *)(*ppHandle);
    if (NULL == pstTtsCtrl)
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    hrst = HIKSPEECH_LTTS_LIB_Release((void *)&pstTtsCtrl->stModHdl, pstTtsCtrl->pTtsHdl);
    if (HIKSPEECH_LIB_S_OK != hrst)
    {
        AUD_LOGE("HIKSPEECH_LTTS_LIB_Release wrong! hrst: %X\n", hrst);
    }

    if (pstTtsCtrl->pOutBuf)
    {
        SAL_memfree(pstTtsCtrl->pOutBuf, "audio", "tts");
    }

    

    if (pstTtsCtrl->stProcMem.pinyin_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stProcMem.pinyin_mem_tab.base, "audio", "tts");
    }

    if (pstTtsCtrl->stProcMem.fs2_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stProcMem.fs2_mem_tab.base, "audio", "tts");
    }

    if (pstTtsCtrl->stProcMem.melgan_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stProcMem.melgan_mem_tab.base, "audio", "tts");
    }

    if (pstTtsCtrl->stProcMem.ltts_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stProcMem.ltts_mem_tab.base, "audio", "tts");
    }

    

    if (pstTtsCtrl->stLttsMem.pinyin_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stLttsMem.pinyin_mem_tab.base, "audio", "tts");
    }

    if (pstTtsCtrl->stLttsMem.fs2_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stLttsMem.fs2_mem_tab.base, "audio", "tts");
    }

    if (pstTtsCtrl->stLttsMem.melgan_mem_tab.base)
    {
        SAL_memfree(pstTtsCtrl->stLttsMem.melgan_mem_tab.base, "audio", "tts");
    }

    SAL_memfree(pstTtsCtrl, "audio", "tts");

    *ppHandle = NULL;
    AUD_LOGI("hik tts stop\n");

    return SAL_SOK;
}

