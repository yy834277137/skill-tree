/**
 * @file   hik_volume_hal.h
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
 
#ifndef _HIK_VOLUME_HAL_H_
#define _HIK_VOLUME_HAL_H_

/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/**
 * @function   hik_volume_setAi
 * @brief      创建音量调节通道
 * @param[in]  void **ppHandle
 * @param[in]  UINT32 u32ChnType 0 ai 非0 ao
 * @param[in]  UINT32 u32VolumeLevel
 * @param[out] void **ppHandle 合成语音句柄
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_volume_create(void ** ppHandle, UINT32 u32ChnType, UINT32 u32VolumeLevel);

/**
 * @function   hik_volume_set
 * @brief      设置音量
 * @param[in]  void *ppHandle
 * @param[in]  UINT32 u32VolumeLevel
 * @param[out] void **ppHandle 合成语音句柄
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_volume_set(void *pHandle, UINT32 u32VolumeLevel);

/**
 * @function   hik_volume_proc
 * @brief      音量调节处理
 * @param[in]  void* pHandle 输入句柄
 * @param[in]  PUINT8 pDataIn 输入数据
 * @param[in]  UINT32 u32InDataSize 输入数据大小
 * @param[out] PUINT8 pDataOut
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_volume_proc(void * pHandle, PUINT8 pDataIn, UINT32 u32InDataSize, PUINT8 pDataOut);

/**
 * @function   hik_volume_destroy
 * @brief      销毁音量调节通道
 * @param[in]  void** ppHandle 输入句柄
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 hik_volume_destroy(void **ppHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _HIK_VOLUME_HAL_H_ */



