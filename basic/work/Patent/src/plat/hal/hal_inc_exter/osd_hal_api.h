/**
 * @file   osd_hal_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  hal层OSD处理接口封装
 * @author heshengyuan
 * @date   2014年12月31日 Create
 * @note
 * @note \n History
   1.Date        : 2014年12月31日 Create
     Author      : heshengyuan
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */


#ifndef _OSD_HAL_H_
#define _OSD_HAL_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <pthread.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


#include "sal.h"
#include "dspcommon.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/**
 * @function   osd_hal_getMemSize
 * @brief    获取osd hdl所需内存大小
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_getMemSize(void);

/**
 * @function   osd_hal_create
 * @brief    创建OSD句柄
 * @param[in]  UINT32 u32Idx rgn id
 * @param[in]  unsigned char *pInBuf 输入buf，外部申请
 * @param[out] void **ppHandle hal层OSD句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_create(UINT32 u32Start, UINT32 u32Idx, void **pHandle, unsigned char *pInBuf);
/**
 * @function   osd_hal_destroy
 * @brief      销毁OSD句柄
 * @param[in]  UINT32 chan venc hal层通道号
 * @param[in]  void *pHandle OSD句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_destroy(UINT32 chan,  void *pHandle);
/**
 * @function   osd_hal_process
 * @brief      osd叠加处理
 * @param[in]  UINT32 chan venc hal层通道号
 * @param[in]   void *pHandle hal层OSD句柄
 * @param[in]   PUINT8 pCharImgBuf 输入buf
 * @param[in]   UINT32 dstW, UINT32 dstH, UINT32 dstX, UINT32 dstY OSD区域信息
 * @param[in]   UINT32 color OSD颜色
 * @param[in]   UINT32 bTranslucent 是否半透明
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_process(UINT32 chan, void *pHandle, PUINT8 pCharImgBuf, UINT64 u64PhyAddr,
                            UINT32 dstW, UINT32 dstH, UINT32 dstX, UINT32 dstY, UINT32 color, UINT32 bTranslucent);
/**
 * @function   osd_hal_init
 * @brief    OSD模块创建
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*_OSD_HAL_H_*/

