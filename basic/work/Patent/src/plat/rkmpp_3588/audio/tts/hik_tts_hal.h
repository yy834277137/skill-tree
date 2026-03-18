/**
 * @file   hik_tts_hal.h
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
 
#ifndef _HIK_TTS_HAL_H_
#define _HIK_TTS_HAL_H_

/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


/*****************************************************************************
                            函数
*****************************************************************************/
/**
 * @function   hik_tts_getSampleRate
 * @brief      获取TTS输出采样率
 * @param[in]  void 
 * @param[out] None
 * @return     TTS输出采样率
 */
UINT32 hik_tts_getSampleRate(void);

/**
 * @function   hik_tts_start
 * @brief      创建合成语音句柄
 * @param[in]  void **ppHandle
 * @param[in]  UINT32 u32VoiceType
 * @param[out] void **ppHandle 合成语音句柄
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_tts_start(void **ppHandle, UINT32 u32VoiceType);
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
INT32 hik_tts_proc(void *pHandle, PUINT8 pDataIn, UINT32 u32InDataSize, PUINT8 pDataOut, UINT32 *pDataSize);

/**
 * @function   hik_tts_stop
 * @brief      关闭合成语音处理
 * @param[in]  void** ppHandle 输入句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_tts_stop(void** ppHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HIK_TTS_HAL_H_ */


