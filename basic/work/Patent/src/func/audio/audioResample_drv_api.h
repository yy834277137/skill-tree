/**
 * @file   audioResample_drv_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---重采样接口
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取重采样接口
 */
 
#ifndef _AUDIO_RESAMPLE_DRV_API_H_
#define _AUDIO_RESAMPLE_DRV_API_H_

/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/*****************************************************************************
                            结构
*****************************************************************************/
typedef struct tagResampleInfoSt
{   
    UINT32 u32DesRate;
    UINT32 u32SrcRate;

    void *pResampleHandle;

    UINT8 *pInputBuf;               /* 外部申请 */
    UINT32 u32InputBufLen;

    UINT8 *pOutputBuf;              /* 外部申请 */
    UINT32 u32OutputBufLen;
} RESAMPLE_INFO_S;  /* 解码重采样 */    

/*****************************************************************************
                            函数
*****************************************************************************/
/**
 * @function   audioResample_drv_create
 * @brief    创建重采样句柄
 * @param[in]  RESAMPLE_INFO_S *pstResampleInfo 重采样初始化参数
 * @param[out] void **ppHandle 重采样句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioResample_drv_create(void **ppHandle, RESAMPLE_INFO_S *pstInPrm);
/**
 * @function   audioResample_drv_Process
 * @brief    重采样处理
 * @param[in]  void * pHandle 重采样句柄
 * @param[in] AUDIO_INTERFACE_INFO_S *pstInPrm 重采样参数
 * @param[out] AUDIO_INTERFACE_INFO_S *pstInPrm 重采样参数
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioResample_drv_Process(void * pHandle, AUDIO_INTERFACE_INFO_S *pstInPrm);
/**
 * @function   audioResample_drv_destroy
 * @brief    销毁重采样句柄
 * @param[in]  void **ppHandle 重采样句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioResample_drv_destroy(void **ppHandle);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_RESAMPLE_DRV_H_ */



