/**
 * @file   venc_tsk_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  VENC 模块 tsk 层
 * @author liuyun10
 * @date   2018年12月16日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月16日 Create
     Author      : liuyun10
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#ifndef _VENC_TSK_API_H_
#define _VENC_TSK_API_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sal.h>
#include "stream_bits_info_def.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define VENC_DEV_NUM		(MAX_VENC_CHAN)           /* 最大编码设备数 */
#define VENC_DEV_CHN_NUM	(3)                       /* 每个设备道数 */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

typedef enum _STREAM_ID_
{
    VENC_STREAM_ID_MAIN = 0,    /* 主码流 */
    VENC_STREAM_ID_SUB = 1,     /* 子码流 */
    VENC_STREAM_ID_THRD = 2,    /* 第三码流 */
    VENC_STREAM_NUM
} VENC_STREAM_ID_E;




/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function   venc_tsk_init
 * @brief  初始模块初始化
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_init(void);

/**
 * @function   venc_tsk_init
 * @brief  初始模块初始化
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in] UINT32 u32StreamId 码流ID mian/sub/third
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_start(UINT32 u32Dev, UINT32 u32StreamId);

/**
 * @function   venc_tsk_stop
 * @brief  停止编码命令
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in] UINT32 u32StreamId 码流ID mian/sub/third
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_stop(UINT32 u32Dev, UINT32 u32StreamId);

/**
 * @function   venc_tsk_forceIFrame
 * @brief  强制I帧命令
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in] UINT32 u32StreamId 码流ID mian/sub/third
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_forceIFrame(UINT32 u32Dev, UINT32 u32StreamId);

/**
 * @function   venc_tsk_setPrm
 * @brief  设置编码参数命令，通道未创建时，仅设置编码参数，不执行其他操作
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in]  UINT32 u32StreamId 码流ID mian/sub/third
 * @param[in]  void *prm 编码参数信息
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_setPrm(UINT32 u32Dev, UINT32 u32StreamId, void *prm);

/**
 * @function   venc_tsk_getHalChan
 * @brief   获取hal通道ID
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in] UINT32 u32StreamId 码流ID mian/sub/third
 * @param[out] UINT32 *pChn 返回通道号
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_getHalChan(UINT32 uiDev, UINT32 u32StreamId, UINT32 *pChn);

/**
 * @function   venc_tsk_getResolution
 * @brief   获取编码分辨率
 * @param[in]  UINT32 u32Dev 通道号
 * @param[in] UINT32 u32StreamId 码流ID mian/sub/third
 * @param[out] UINT32 *pResolution 返回分辨率信息
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 venc_tsk_getResolution(UINT32 uiDev, UINT32 u32StreamId, UINT32 *pResolution);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _VENC_TSK_H_ */




