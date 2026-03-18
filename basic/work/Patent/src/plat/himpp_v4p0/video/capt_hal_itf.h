/*******************************************************************************
* capt_hal_itf.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2017年09月08日 Create
*
* Description : 硬件平台采集模块
* Modification:
*******************************************************************************/
#ifndef _CAPT_HAL_ITF_H_
#define _CAPT_HAL_ITF_H_


#ifdef __cplusplus
extern "C"
{
#endif

#include "sal.h"


/*****************************************************************************
                            结构体定义
*****************************************************************************/

/* 模块数据传输结构体 */
typedef struct tagCaptHalFrameInfo
{
    PhysAddr uiDataAddr;  /* 平台相关数据结构体地址 uiDataAddr所指向结构体封装与平台相关的数据类型 */
    UINT32 uiDataLen;   /* uiDataAddr 所指向结构体信息 */
    PhysAddr uiAppData;   /* 模块数据定义 */
} CAPT_HAL_FRAME_INFO;


/* 模块数据管理信息 */
typedef struct tagCaptHalBufferInfo
{
    /* 每一帧的信息 */
    CAPT_HAL_FRAME_INFO stCaptHalFrameInfo[4];
    /* 帧的管理信息 */
    UINT32 uiUsedIdx;
} CAPT_HAL_BUFFER_INFO;


#ifdef __cplusplus
}
#endif


#endif /* _CAPT_HAL_ITF_H_ */
