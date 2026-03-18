/**
 * @file   audio_hal_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---hal层接口封装
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */
 
#ifndef _AUDIO_HAL_API_H_
#define _AUDIO_HAL_API_H_

#include "sal.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/*****************************************************************************
                            宏定义
*****************************************************************************/
/* 最大支持数 */
#define AUDIO_AI_MAX_CHN	(2)
#define AUDIO_AO_MAX_CHN	(2)

#define AUDIO_MAX_VOLUME (10)

/*****************************************************************************
                            结构体定义
*****************************************************************************/
/* 单/双声道 */
typedef enum
{
    AUDIO_MODE_MONO = 0,  /*mono*/
    AUDIO_MODE_STEREO = 1, /*stereo*/
    AUDIO_MODE_BUTT,

} AUDIO_MODE_E;

typedef struct
{
    UINT8 *pBufferAddr;   /*audio frame buffer addr*/
    UINT32 FrameLen;     /*audio frame length*/
    UINT32 Channel;      /*audio get or send channel mode*/
    UINT32 AudioDevIdx;
}HAL_AIO_FRAME_S;

typedef struct
{
    char *pIndata;               /* 文字数据缓存 */
    UINT32 u32DataSize;    /* 文字数据长度 */
} AUTO_TTS_PARAM_S;

/*****************************************************************************
                            函数声明
*****************************************************************************/
/**
 * @function   audio_hal_create
 * @brief    音频模块创建
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_create();
/**
 * @function   audio_hal_aoSendFrame
 * @brief    音频输出
 * @param[in]  HAL_AIO_FRAME_S * pstAoFrame 输出帧
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_aoSendFrame(HAL_AIO_FRAME_S * pstAoFrame);

/**
 * @function   audio_hal_setAoVolume
 * @brief    设置音频输出音量
 * @param[in]  UINT32 u32AudioChan 音频通道
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_setAoVolume(UINT32 u32AudioChan, UINT32 u32Volume);
/**
 * @function   audio_hal_setAiVolume
 * @brief    设置音频输入音量
 * @param[in]  UINT32 u32AudioChan 音频通道
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
 INT32 audio_hal_setAiVolume(UINT32 u32AudioChan, UINT32 u32Volume);
/**
 * @function   audio_hal_setVqe
 * @brief    VQE开关操作
 * @param[in]  UINT32 u32AudioChan 音频通道
 * @param[in]  BOOL bEanble 开关标志
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_setVqe(UINT32 u32AudioChan, BOOL bEanble);
/**
 * @function   audio_hal_aiGetFrame
 * @brief    读取音频采集数据
 * @param[in]  SAL_AudioFrameBuf **ppstAudFrmBuf 音频帧结构指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_aiGetFrame(SAL_AudioFrameBuf **ppstAudFrmBuf);

/**
 * @function   audio_hal_init
 * @brief    初始化音频模块
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_init(void);

/**
 * @function   audio_hal_ttsGetSampleRte
 * @brief      获取TTS输出采样率
 * @param[in]  void 
 * @param[out] None
 * @return     TTS输出采样率
 */
UINT32 audio_hal_ttsGetSampleRte(void);

/**
 * @function   audio_hal_ttsProc
 * @brief      合成语音处理,hik库单次处理不能超过196字节
 * @param[in]  void* pHandle 输入句柄
 * @param[in]  AUTO_TTS_PARAM_S *pstPrm 输入数据
 * @param[out] PUINT8 pDataOut
 * @param[out] UINT32 *pDataSize
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_ttsProc(void *pHandle, AUTO_TTS_PARAM_S *pstPrm, PUINT8 pDataOut, UINT32 *pDataSize);

/**
 * @function   audio_hal_ttsStop
 * @brief      tts语义转换关闭
 * @param[in]  void** ppHandle
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_ttsStop(void** ppHandle);

/**
 * @function   audio_hal_ttsStart
 * @brief      tts语义转换开启
 * @param[in]  void **ppHandle
 * @param[in]  UINT32 u32VoiceType
 * @param[in]  void *pPrm
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_ttsStart(void **ppHandle, UINT32 u32VoiceType);

/**
 * @function   audio_hal_getUpLinePram
 * @brief    获取音频上行模块参数
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
SAL_AudioFrameBuf *audio_hal_getUpLinePram(void);

/**
 * @function   audio_hal_getDnLinePram
 * @brief    获取音频下行模块参数
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
SAL_AudioFrameBuf *audio_hal_getDnLinePram(void);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_HAL_H_ */

