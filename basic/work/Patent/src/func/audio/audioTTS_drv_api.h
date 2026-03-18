/**
 * @file   audioTTS_drv_api.h
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
 
#ifndef _AUDIO_TTS_DRV_API_H_
#define _AUDIO_TTS_DRV_API_H_

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
 * @function   audioTTS_drv_start
 * @brief      创建合成语音句柄
 * @param[in]  UINT32 u32Chan 通道号
 * @param[in]  void *pPrm 合成语音初始化参数(AUTO_TTS_START_PARAM)
 * @param[out] void **ppHandle 合成语音句柄
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioTTS_drv_start(UINT32 u32Chan, void **ppHandle, void *pPrm);
/**
 * @function   audioTTS_drv_process
 * @brief      合成语音处理
 * @param[in]  void* pHandle 输入句柄
 * @param[in]  void *pPrm 合成语音处理参数(AUTO_TTS_HANDLE_PARAM)
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioTTS_drv_process(void *pHandle, VOID *pPrm, PUINT8 *pDataOut, UINT32 *pDataSize);
/**
 * @function   audioTTS_drv_stop
 * @brief      关闭合成语音处理
 * @param[in]  void** ppHandle 输入句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioTTS_drv_stop(void** ppHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_TTS_DRV_H_ */


