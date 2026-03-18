/**
 * @file   venc_hal_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  编码组件---hal层接口封装
 * @author 
 * @date   
 * @note
 * @note \n History
   1.Date        : 2018年12月1日 Create
     Author      : liuyun10
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#ifndef _VENC_HAL_API_H_
#define _VENC_HAL_API_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */



/*****************************************************************************
                            宏定义
*****************************************************************************/

/*****************************************************************************
                            结构体定义
*****************************************************************************/


/*****************************************************************************
                            函数声明
*****************************************************************************/

/**
 * @function   venc_drv_getfps
 * @brief  获取原编码帧率
 * @param[in] pstChnInfo
 * @param[out] None
 * @return      int  帧率
 */
INT32 venc_drv_getfps(void *pstChnInfo);

/**
 * @function   venc_hal_requestIDR
 * @brief    强制I帧编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_requestIDR(void *pHandle);

/**
 * @function   venc_hal_setEncPrm
 * @brief    设置编码参数
 * @param[in]  void *pHandle hal层编码器句柄 
 * @param[in]   SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_setEncPrm(void *pHandle, SAL_VideoFrameParam *pstInPrm);

/**
 * @function   venc_hal_stop
 * @brief    停止编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_stop(void *pHandle);

/**
 * @function   venc_hal_start
 * @brief    开始编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_start(void *pHandle);

/**
 * @function   venc_hal_getFrame
 * @brief    从指定的编码通道获取最新的一帧编码码流，保证和VencHal_putFrame成对调用
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] void *pInPrm 获取编码码流
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_getFrame(void *pHandle, void *pInPrm);

/**
 * @function   venc_hal_putFrame
 * @brief    释放指定通道的码流缓存，和VencHal_getFrame 成对出现
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_putFrame(void *pHandle);

/**
 * @function   venc_hal_getChn
 * @brief    获取plat层通道venc_hal_getVencHalChan
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] UINT32 *pOutChan plat层通道号指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_getChn(void *pHandle, UINT32 *pOutChan);

/**
 * @function   venc_hal_create
 * @brief    编码模块创建
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_init(void);

/**
 * @function   venc_hal_create
 * @brief    创建编码器
 * @param[in]  SAL_VideoFrameParam *pstInPrm 编码参数
 * @param[out] void **ppHandle hal层编码器句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_create(void **ppHandle, SAL_VideoFrameParam *pstInPrm);

/**
 * @function   venc_hal_delete
 * @brief    销毁编码器
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_delete(void *pHandle);

/**
 * @function   venc_hal_setStatus
 * @brief    设置编码状态，方便drv层管理编码、取流流程
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  void *pStr 状态信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_setStatus(void *pHandle, void *pStr);

/**
 * @function   venc_hal_encJpeg
 * @brief      JPEG编码
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 帧信息
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  CROP_S *pstCropInfo 编码crop信息
 * @param[in]  BOOL bSetPrm 编码参数更新，用于重配编码器
 * @param[out] INT8 *pJpeg 编码输出JPEG码流
 * @param[out] INT8 *pJpegSize 编码输出JPEG码流大小
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_encJpeg(void *pHandle, SYSTEM_FRAME_INFO *pstFrame, INT8 *pJpeg, UINT32 *pJpegSize, CROP_S *pstCropInfo, BOOL bSetPrm);

/**
 * @function   venc_hal_sendVideoFrm
 * @brief      往编码器送数据(非JPEG)
 * @param[in]  void *pHandle hal层编码器句柄
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 帧信息
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_sendVideoFrm(void *pHandle, SYSTEM_FRAME_INFO *pstFrame);

/**
 * @function   venc_hal_getFrameMem
 * @brief      申请帧内存
 * @param[in]   imgW
 * @param[in]   imgH
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_getFrameMem(UINT32 imgW, UINT32 imgH, SYSTEM_FRAME_INFO *pstSystemFrameInfo);

/**
 * @function   venc_hal_getFrameMem
 * @brief      释放帧内存
 * @param[in]   imgW
 * @param[in]   imgH
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_putFrameMem(SYSTEM_FRAME_INFO *pstSystemFrameInfo);

/**
 * @function   venc_hal_init
 * @brief    编码模块创建
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_hal_init(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*_VENC_HAL_H_*/

