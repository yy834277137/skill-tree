/**
 * @file   jpeg_drv.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  jpeg 模块 drv 层内部接口
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

#ifndef _JPEG_DRV_H_
#define _JPEG_DRV_H_

#include <sal.h>
#include <platform_hal.h>
#include <jpeg_drv_api.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


/*****************************************************************************
                            宏定义
*****************************************************************************/

#define JPEG_MAX_CHN_NUM (MAX_VENC_CHAN + MAX_VDEC_CHAN + 2 + 2) /* 加2个智能抓图的通道+2个转存通道 */

/*****************************************************************************
                            结构定义
*****************************************************************************/

/* Jpeg Dev 信息 普通抓图的业务信息 */
typedef struct tagJpegVbAllocSt
{
    PhysAddr phyAddr;                     /* 物理地址 */
    void* virAddr;                        /* 虚拟地址 */
    UINT32 u32MemSize;                    /* 地址空间总大小 */
    UINT32 u32PoolId;                     
    UINT64 u64vbBlk;
}JPEG_VB_ALLOC_ST;

typedef struct tagJpegAddrSt
{
    SYSTEM_FRAME_INFO stYuvFrame;
    JPEG_VB_ALLOC_ST stYuvBackAddr;
}JPEG_ADDR;


typedef struct tagJpegDrvDevInfoSt
{
    UINT32 u32Chan;              /* 当前tsk通道号 */
    UINT32 isCreate;

    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Quality;

    JPEG_CTRL_PRM_S stCtrlPrm;  /* 当前抓图控制信息 */

    void *pMutexHandle;          /* 锁和信号量的handle */
    void *pVencHandle;           /* 对应hal层编码器handle */
} JPEG_DRV_CHN_INFO_S;

/* JPEG编码 模块状态结构体 */
typedef struct tagJpegDrvCtrlSt
{
    UINT32 isJpegChnUsed;
    void *pMutexHandle;
    JPEG_DRV_CHN_INFO_S stJpegChnInfo[JPEG_MAX_CHN_NUM];
} JPEG_DRV_CTRL_S;


/*****************************************************************************
                            函数声明
*****************************************************************************/


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _JPEG_DRV_H_ */

