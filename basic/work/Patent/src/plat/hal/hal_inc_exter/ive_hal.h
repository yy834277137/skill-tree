/**
 * @file   ive_hal.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   ive模块接口 。
 * @author  wanglei100
 * @date    20210610
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */

#ifndef _IVE_HAL_H_
#define _IVE_HAL_H_

#include "sal.h"



/*****************************************************************************
                            结构体定义
*****************************************************************************/

/* ive模式,可根据需求自行添加 */
typedef enum enIveHalMode_E
{
    IVE_CSC_PIC_BT601_YUV2RGB = 0x2,   /* CSC: YUV2RGB */
    IVE_CSC_PIC_BT709_YUV2RGB = 0x3,   /* CSC: YUV2RGB */

    IVE_CSC_PIC_BT601_RGB2YUV = 0xa,   /* CSC: RGB2YUV */
    IVE_CSC_PIC_BT709_RGB2YUV = 0xb,   /* CSC: RGB2YUV */

    IVE_HAL_MAP_MODE_U8 = 0x10,        /* MAP: U8 */
    IVE_HAL_MAP_MODE_S16 = 0x11,       /* MAP: S16 */
    IVE_HAL_MAP_MODE_U16 = 0x12,       /* MAP: U16 */
} IVE_HAL_MODE_E;


/* ive输入参数 */
typedef struct tagIveImageSt
{
    UINT64 u64PhyAddr[3];            /* 图像物理地址 */
    UINT64 u64VirAddr[3];            /* 图像虚拟地址 */
    UINT64 u32Stride[3];             /* 图像跨距 */
    UINT32 u32Width;                 /* 图像宽 */
    UINT32 u32Height;                /* 图像高 */
    SAL_VideoDataFormat enColorType;      /* 图像格式 */
} IVE_HAL_IMAGE;

/* ive CSC控制参数 */
typedef struct tagIveHalCtrlSt
{
    INT32 s32IveHandle;               /* 句柄 */
    IVE_HAL_MODE_E u32enMode;         /* 模式 */
} IVE_HAL_MODE_CTRL;

/* tde操作区域 */
typedef struct tagIveHalRectSt
{
    INT32 s32Xpos;
    INT32 s32Ypos;
    UINT32 u32Width;               /*进行操作的区域宽 */
    UINT32 u32Height;              /*进行操作的区域高 */
} IVE_HAL_RECT;

/* ide操作区域 */
typedef struct tagIveHalDataSt
{
    UINT64 u64PhyAddr;            /* 物理地址 */
    UINT64 u64VirAddr;            /* 虚拟地址 */

    UINT32 u32Stride;             /* 2D 数据跨距 */
    UINT32 u32Width;              /* 2D 数据宽 */
    UINT32 u32Height;             /* 2D 数据高 */
} IVE_HAL_DATA;

/* ide map表地址 */
typedef struct tagIveHalMemInfoSt
{
    UINT64 u64PhyAddr;            /* 物理地址 */
    UINT64 u64VirAddr;            /* 虚拟地址 */
    UINT32 u32Size;
} IVE_HAL_MEM_INFO;



/**
 * @function    ive_hal_CSC
 * @brief       光栅位图缩放
 * @param[in]
 * @param[out]
 * @return
 */
INT32 ive_hal_CSC(IVE_HAL_IMAGE *pstSrc, IVE_HAL_IMAGE *pstDst, IVE_HAL_MODE_CTRL *pstIveCscCtrl);

/**
 * @function    ive_hal_QuickCopy
 * @brief       ive快速拷贝
 * @param[in]
 * @param[out]
 * @return
 */
INT32 ive_hal_QuickCopy(SYSTEM_FRAME_INFO *pSrcSysFrame, SYSTEM_FRAME_INFO *pDstSysFrame, IVE_HAL_RECT *pstSrcRect, IVE_HAL_RECT *pstDstRect, UINT32 bCached);

/**
 * @function    ive_hal_Map
 * @brief       map
 * @param[in]
 * @param[out]
 * @return
 */
INT32 ive_hal_Map(IVE_HAL_IMAGE *pstSrc, IVE_HAL_IMAGE *pstDst, IVE_HAL_MEM_INFO *pstMap, IVE_HAL_MODE_CTRL *pstIveMapCtrl);

/**
 * @function    ive_hal_init
 * @brief       初始化ive模块
 * @param[in]
 * @param[out]
 * @return
 */
INT32 ive_hal_init(void);


#endif


