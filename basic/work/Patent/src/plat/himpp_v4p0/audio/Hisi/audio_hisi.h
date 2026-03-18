/**
 * @file   audio_hisi.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---plat层音频接口封装
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

#ifndef _AUDIO_HISI_H_
#define _AUDIO_HISI_H_

#include <sal.h>
#include <platform_hal.h>
#include "hal_inc_inter/audio_hal_inter.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/**
 * @function   audio_hisi_aoCreate
 * @brief    音频输出初始化
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aoCreate(HAL_AIO_ATTR_S *pstAIOAttr);
/**
 * @function   audio_hisi_aoDestroy
 * @brief    销毁音频输出
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
VOID audio_hisi_aoDestroy(HAL_AIO_ATTR_S *pstAIOAttr);
/**
 * @function   audio_hisi_aoSendFrame
 * @brief    输出音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aoSendFrame(HAL_AIO_FRAME_S *pstAudioFrame);
/**
 * @function   audio_hisi_aiCreate
 * @brief    音频输入初始化
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiCreate(HAL_AIO_ATTR_S *pstAIOAttr);
/**
 * @function   audio_hisi_aiDestroy
 * @brief    音频输入销毁
 * @param[in] HAL_AIO_ATTR_S *pstAIOAttr 音频输入输出结构描述指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
VOID audio_hisi_aiDestroy(HAL_AIO_ATTR_S *pstAIOAttr);
/**
 * @function   audio_hisi_aiGetFrame
 * @brief    获取音频数据帧
 * @param[in] HAL_AIO_FRAME_S *pstAudioFrame 音频帧信息描述结构体指针
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiGetFrame(HAL_AIO_FRAME_S *pstAudioFrame);
/**
 * @function   audio_hisi_aiSetVolume
 * @brief    设置音频输入音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiSetVolume(UINT32 u32Channel, UINT32 u32Volume);
/**
 * @function   audio_hisi_aoSetVolume
 * @brief    设置音频输出音量
 * @param[in]  UINT32 u32Channel 通道号
 * @param[in]  UINT32 u32Volume 音量
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aoSetVolume(UINT32 u32Channel, UINT32 u32Volume);
/**
 * @function   audio_hisi_aiEnableVqe
 * @brief    开启VQE处理(声音质量增强)
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiEnableVqe(UINT32 u32AudioDevIdx);
/**
 * @function   audio_hisi_aiDisableVqe
 * @brief    关闭VQE处理
 * @param[in]  UINT32 u32AudioDevIdx 通道号
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hisi_aiDisableVqe(UINT32 u32AudioDevIdx);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*_AUDIO_HISI_H_*/

