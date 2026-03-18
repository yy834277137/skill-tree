/**
 * @file   hik_tts_hal.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---音量调节组件
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2022/01/07
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取合成语音接口
 */


/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal_mem_new.h"
#include "hik_alc_lib.h"
#include "hik_volume_hal.h"

/*****************************************************************************
                         宏定义
*****************************************************************************/
#define MAX_VOLUME_LEVEL			(10)
#define DEF_VOLUME					(50)
#define VOLUME_PROC_MAX_FRAME_LEN	(1280)

/*****************************************************************************
                         结构定义
*****************************************************************************/

typedef struct
{
    UINT32 u32ChnType;              /*0 ai 非0 ao*/
    UINT32 u32VolumeLevel;          /*音量等级*/
    UINT32 u32Volume;               /*音量值*/
    PUINT8 pDataProcBuf;            /*音量处理临时缓冲*/

    void *pHandle;                  /* 声明handle指针变量 */
    APA_MEM_TAB mem_tab[1];         /* 声明地址表变量 */
    APA_AUDIO_INFO audio_header;    /* 声明音频头信息变量 */
} HIK_VOLUME_CHN_S;

/*****************************************************************************
                         全局变量定义
*****************************************************************************/
static UINT32 u32AoVolume[MAX_VOLUME_LEVEL] = {10, 20, 25, 30, 35, 40, 45, 50, 60, 70}; /* 取值[1,100],1为-20dB，100为20dB，50为0dB */
static UINT32 u32AiVolume[MAX_VOLUME_LEVEL] = {10, 20, 25, 30, 35, 40, 45, 50, 60, 70}; /* 取值[1,100],1为-20dB，100为20dB，50为0dB */

/**
 * @function   hik_volume_setAi
 * @brief      创建音量调节通道
 * @param[in]  void **ppHandle
 * @param[in]  UINT32 u32ChnType 0 ai 非0 ao
 * @param[in]  UINT32 u32VolumeLevel
 * @param[out] void **ppHandle 合成语音句柄
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_volume_create(void **ppHandle, UINT32 u32ChnType, UINT32 u32VolumeLevel)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32Volume = 0;
    APA_KEY_PARAM key_param = {0};        /* 声明配置参数结构体变量 */
    HIK_VOLUME_CHN_S *pstHikVolumeCtrl = NULL;

    if ((NULL == ppHandle) || (MAX_VOLUME_LEVEL <= u32VolumeLevel))
    {
        AUD_LOGE("pPrm %p volume level %d\n", ppHandle, u32VolumeLevel);
        return SAL_FAIL;
    }

    if (NULL != *ppHandle)
    {
        AUD_LOGW("volume handle exist!!\n");
        return SAL_SOK;
    }

    pstHikVolumeCtrl = SAL_memAlign(32, sizeof(HIK_VOLUME_CHN_S), "audio", "volume");
    if (NULL == pstHikVolumeCtrl)
    {
        AUD_LOGE("alloc mem fail\n");
        return SAL_FAIL;
    }

    pstHikVolumeCtrl->pDataProcBuf = SAL_memAlign(32, VOLUME_PROC_MAX_FRAME_LEN, "audio", "volume");
    if (NULL == pstHikVolumeCtrl)
    {
        AUD_LOGE("alloc mem fail\n");
        return SAL_FAIL;
    }

    /* 配置音频信息 */
    pstHikVolumeCtrl->audio_header.sample_rate = 8000;      /* 采样率,支持的采样率查看.h文件 */
    pstHikVolumeCtrl->audio_header.channel_num = 1;         /* 通道数 */
    pstHikVolumeCtrl->audio_header.bits_per_sample = 16;     /* 每个采样点的bit数，公司现有的音频数据都是16bit */
    pstHikVolumeCtrl->audio_header.data_len = 320;          /* 帧长，采样点数是short型，注意！支持的帧长查看.h文件 */

    /* 调用接口，获取所需内存大小 */
    s32Ret = HIK_ALC_GetMemSize(&pstHikVolumeCtrl->audio_header, pstHikVolumeCtrl->mem_tab);
    /* 错误码判断，当返回HIK_APA_LIB_S_OK时，调用成功，其他错误码则调用错误，需根据apa_common.h和lib.h查找对应错误码注释信息 */
    if (s32Ret != HIK_APA_LIB_S_OK)
    {
        AUD_LOGE("%x \n", s32Ret);
        goto err;
    }

    /* 采用首地址对齐的方式分配内存空间 */
    pstHikVolumeCtrl->mem_tab[0].base = (void *)SAL_memAlign(pstHikVolumeCtrl->mem_tab[0].alignment,
                                                             pstHikVolumeCtrl->mem_tab[0].size, "audio", "volume");
    if (NULL == pstHikVolumeCtrl->mem_tab[0].base)
    {
        AUD_LOGE("memory alloc failed!\n");
        goto err;
    }

    /* 创建handle句柄，进行初始化操作 */
    s32Ret = HIK_ALC_Create(&pstHikVolumeCtrl->audio_header, pstHikVolumeCtrl->mem_tab, &pstHikVolumeCtrl->pHandle);
    /* 错误码判断，当返回HIK_APA_LIB_S_OK时，调用成功，其他错误码则调用错误，需根据apa_common.h和lib.h查找对应错误码注释信息 */
    if (s32Ret != HIK_APA_LIB_S_OK)
    {
        AUD_LOGE("HIK_ALC_Create %x \n", s32Ret);
        goto err1;
    }

    /* ///////////////////////////////////高级参数配置，可以根据产品场景配置//////////////////////////////// */
    /* 设置初始音量，每次创建算法库只可设置一次 */
    key_param.index = ALC_CFG_PARAM_VOLUME_SET;     /* 对应取值.h文件中ALC_CFG_ENUM_KEY_PARAM对应的变量，来确定当前配置的是哪个参数 */
    key_param.value = DEF_VOLUME;     /* 取值[1,100],1为-20dB，100为20dB，50为0dB，默认50 */

    s32Ret = HIK_ALC_SetConfig(pstHikVolumeCtrl->pHandle,
                               APA_SET_CFG_SINGLE_PARAM,
                               &key_param,
                               sizeof(APA_KEY_PARAM));
    /* 错误码判断，当返回HIK_APA_LIB_S_OK时，调用成功，其他错误码则调用错误，需根据apa_common.h和lib.h查找对应错误码注释信息 */
    if (s32Ret != HIK_APA_LIB_S_OK)
    {
        AUD_LOGE("HIK_ALC_SetConfig %x \n", s32Ret);
        goto err1;
    }

    u32Volume = u32ChnType ? u32AoVolume[u32VolumeLevel] : u32AiVolume[u32VolumeLevel]; /* 设置目标等级 */
    /* 设置音量调整参数，可在算法运行中随时变动 */
    key_param.index = ALC_CFG_PARAM_VOLUME_ADJUST;
    key_param.value = u32Volume; /* 取值[1,100],1为-20dB，100为20dB，50为0dB，默认50 */

    s32Ret = HIK_ALC_SetConfig(pstHikVolumeCtrl->pHandle,
                               APA_SET_CFG_SINGLE_PARAM,
                               &key_param,
                               sizeof(APA_KEY_PARAM));
    if (s32Ret != HIK_APA_LIB_S_OK)
    {
        AUD_LOGE("HIK_ALC_SetConfig %x \n", s32Ret);
        goto err1;
    }

    /* 获取算法库内部目前配置音量调整参数 */
    key_param.index = ALC_CFG_PARAM_VOLUME_ADJUST;

    s32Ret = HIK_ALC_GetConfig(pstHikVolumeCtrl->pHandle,
                               APA_SET_CFG_SINGLE_PARAM,
                               &key_param,
                               sizeof(APA_KEY_PARAM));
    if (s32Ret != HIK_APA_LIB_S_OK)
    {
        AUD_LOGE("HIK_ALC_GetConfig %x \n", s32Ret);
    }
    else
    {
        AUD_LOGI("HIK_ALC_GetConfig volume %d \n", key_param.value);
    }

    pstHikVolumeCtrl->u32ChnType = u32ChnType;
    pstHikVolumeCtrl->u32VolumeLevel = u32VolumeLevel;
    pstHikVolumeCtrl->u32Volume = u32Volume;
    *ppHandle = pstHikVolumeCtrl;

    return SAL_SOK;

err1:
    if (pstHikVolumeCtrl->mem_tab[0].base)
    {
        SAL_memfree(pstHikVolumeCtrl->mem_tab[0].base, "audio", "volume");
    }

err:
    if (pstHikVolumeCtrl)
    {
        SAL_memfree(pstHikVolumeCtrl, "audio", "volume");
        *ppHandle = NULL;
    }

    return SAL_FAIL;
}

/**
 * @function   hik_volume_set
 * @brief      设置音量
 * @param[in]  void *ppHandle
 * @param[in]  UINT32 u32VolumeLevel
 * @param[out] void **ppHandle 合成语音句柄
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_volume_set(void *pHandle, UINT32 u32VolumeLevel)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32Volume = 0;
    APA_KEY_PARAM key_param = {0};        /* 声明配置参数结构体变量 */
    HIK_VOLUME_CHN_S *pstHikVolumeCtrl = NULL;

    if ((NULL == pHandle) || (MAX_VOLUME_LEVEL <= u32VolumeLevel))
    {
        AUD_LOGE("pPrm %p volume level %d\n", pHandle, u32VolumeLevel);
        return SAL_FAIL;
    }

    pstHikVolumeCtrl = (HIK_VOLUME_CHN_S *)pHandle;
    if (u32VolumeLevel == pstHikVolumeCtrl->u32VolumeLevel)
    {
        AUD_LOGW("same volume lev %d\n", u32VolumeLevel);
        return SAL_SOK;
    }

    u32Volume = pstHikVolumeCtrl->u32ChnType ? u32AoVolume[u32VolumeLevel] : u32AiVolume[u32VolumeLevel]; /* 设置目标等级 */
    /* 设置音量调整参数，可在算法运行中随时变动 */
    key_param.index = ALC_CFG_PARAM_VOLUME_ADJUST;
    key_param.value = u32Volume; /* 取值[1,100],1为-20dB，100为20dB，50为0dB，默认50 */

    s32Ret = HIK_ALC_SetConfig(pstHikVolumeCtrl->pHandle,
                               APA_SET_CFG_SINGLE_PARAM,
                               &key_param,
                               sizeof(APA_KEY_PARAM));
    if (s32Ret != HIK_APA_LIB_S_OK)
    {
        AUD_LOGE("HIK_ALC_SetConfig %x \n", s32Ret);
        return SAL_FAIL;
    }

    /* 获取算法库内部目前配置音量调整参数 */
    key_param.index = ALC_CFG_PARAM_VOLUME_ADJUST;

    s32Ret = HIK_ALC_GetConfig(pstHikVolumeCtrl->pHandle,
                               APA_SET_CFG_SINGLE_PARAM,
                               &key_param,
                               sizeof(APA_KEY_PARAM));
    if (s32Ret != HIK_APA_LIB_S_OK)
    {
        AUD_LOGE("HIK_ALC_GetConfig %x \n", s32Ret);
    }
    else
    {
        AUD_LOGI("HIK_ALC_GetConfig volume lev %d val %d \n", u32VolumeLevel, key_param.value);
    }

    pstHikVolumeCtrl->u32VolumeLevel = u32VolumeLevel;
    pstHikVolumeCtrl->u32Volume = u32Volume;

    return SAL_SOK;
}

/**
 * @function   hik_volume_proc
 * @brief      音量调节处理
 * @param[in]  void* pHandle 输入句柄
 * @param[in]  PUINT8 pDataIn 输入数据
 * @param[in]  UINT32 u32InDataSize 输入数据大小
 * @param[out] PUINT8 pDataOut
 * @param[out] UINT32 *pDataSize
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_volume_proc(void *pHandle, PUINT8 pDataIn, UINT32 u32InDataSize, PUINT8 pDataOut)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32FrmLen = 0;
    UINT32 u32ProcDataLen = 0;
    HIK_VOLUME_CHN_S *pstHikVolumeCtrl = NULL;

    if ((NULL == pDataIn) || (NULL == pHandle)
        || (NULL == pDataOut) || (0 == u32InDataSize))
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    pstHikVolumeCtrl = (HIK_VOLUME_CHN_S *)pHandle;
    if (DEF_VOLUME == pstHikVolumeCtrl->u32Volume) /*不做调节*/
    {
        if (pDataIn != pDataOut)
        {
            memcpy(pDataOut, pDataIn, u32InDataSize);
        }

        return SAL_SOK;
    }

    if (VOLUME_PROC_MAX_FRAME_LEN < u32FrmLen)
    {
        AUD_LOGE("buf over flow %d\n", u32FrmLen);
        return SAL_FAIL;
    }

    /* SAL_INFO("insize %d frm len %d\n", u32InDataSize, pstHikVolumeCtrl->audio_header.data_len << 1); */

    u32FrmLen = pstHikVolumeCtrl->audio_header.data_len << 1;
    while (u32InDataSize >= u32FrmLen)
    {
        memset(pstHikVolumeCtrl->pDataProcBuf, 0, u32FrmLen);
        s32Ret = HIK_ALC_Process(pstHikVolumeCtrl->pHandle, pDataIn + u32ProcDataLen, u32FrmLen, pstHikVolumeCtrl->pDataProcBuf, u32FrmLen);        /* 调用主处理函数 */
        /* 错误码判断，当返回HIK_APA_LIB_S_OK时，调用成功，其他错误码则调用错误，需根据apa_common.h和lib.h查找对应错误码注释信息 */
        if (s32Ret != HIK_APA_LIB_S_OK)
        {
            AUD_LOGE("HIK_ALC_Process %x \n", s32Ret);
            return SAL_FAIL;
        }

        memcpy(pDataOut + u32ProcDataLen, pstHikVolumeCtrl->pDataProcBuf, u32FrmLen);

#if 0
        FILE *pOutFile = fopen("/home/config/audio_volume_out.pcm", "a+");
        if (pOutFile)
        {
            fwrite(pDataOut + u32ProcDataLen, 1, u32FrmLen, pOutFile);
            fflush(pOutFile);
            fclose(pOutFile);
        }

#endif
        u32InDataSize -= u32FrmLen;
        u32ProcDataLen += u32FrmLen;
    }

    return SAL_SOK;
}

/**
 * @function   hik_volume_destroy
 * @brief      销毁音量调节通道
 * @param[in]  void** ppHandle 输入句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_volume_destroy(void **ppHandle)
{
    HIK_VOLUME_CHN_S *pstHikVolumeCtrl = NULL;

    if (NULL == ppHandle)
    {
        AUD_LOGE("prm NULL\n");
        return SAL_FAIL;
    }

    pstHikVolumeCtrl = (HIK_VOLUME_CHN_S *)(*ppHandle);
    if (NULL == pstHikVolumeCtrl)
    {
        AUD_LOGE("prm NULL\n");
        return SAL_SOK;
    }

    if (pstHikVolumeCtrl->mem_tab[0].base)
    {
        SAL_memfree(pstHikVolumeCtrl->mem_tab[0].base, "audio", "volume");
    }

    if (pstHikVolumeCtrl->pDataProcBuf)
    {
        SAL_memfree(pstHikVolumeCtrl->pDataProcBuf, "audio", "volume");
    }

    SAL_memfree(pstHikVolumeCtrl, "audio", "volume");
    *ppHandle = NULL;
    return SAL_SOK;
}

