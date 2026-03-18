/**
 * @file   tde_hal.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   tde模块接口 主要包裹。
 * @author  wanglei100
 * @date    20210602
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */

#ifndef _TDE_HAL_H_
#define _TDE_HAL_H_

#include "sal.h"

/*****************************************************************************
                            结构体定义
*****************************************************************************/
/* tde支持旋转的角度 */
typedef enum tagTdeHalRotate {
	TDE_HAL_ROTATION_0		  = 0,
	TDE_HAL_ROTATION_90 	  = 1,
	TDE_HAL_ROTATION_180	  = 2,
	TDE_HAL_ROTATION_270	  = 3,
	TDE_HAL_ROTATION_BUTT
} TDE_HAL_ROTATION_E;

/* tde支持镜像的角度 */
typedef enum tagTdeHalMirror {
    TDE_HAL_MIRROR_NONE       = 0,
    TDE_HAL_MIRROR_HORIZONTAL = 1,  /* RW; MIRROR */
    TDE_HAL_MIRROR_VERTICAL   = 2,  /* RW; FLIP */
    TDE_HAL_MIRROR_BOTH       = 3,  /* RW; MIRROR and FLIP */
    TDE_HAL_MIRROR_BUTT
} TDE_HAL_MIRROR_E;

/* tde操作区域 */
typedef struct tagTdeHalRectSt
{
    INT32 s32Xpos;
    INT32 s32Ypos;
    UINT32 u32Width;               /*进行操作的区域宽 */
    UINT32 u32Height;              /*进行操作的区域高 */
} TDE_HAL_RECT;

/* tde输入参数 */
typedef struct tagTdeHalSerfaceSt
{
    UINT32 u32TdeHandle;             /* 缓存句柄, todo: unused */
    PhysAddr PhyAddr;                /* 物理地址 */
    VOID *virAddr;                   /* TDE拷贝时上层下发的是源mb拷贝区域rect的起始虚拟地址和src X/Y为0，RK根据此和mb的起始虚拟地址算出X/Y坐标;(上层最好改为下发mb的起始地址和src X/Y，但改动地方较多所以只能在RK plat里转换)*/
	SAL_VideoDataFormat enColorFmt;  /* 颜色格式 目前先用宏定义,建议后期在dspcommon.h添加统一的枚举*/
    UINT32 u32Width;                 /* 图像宽度 */
    UINT32 u32Height;                /* 图像高度 */
    UINT32 u32Stride;                /* 图像跨距 */
    UINT64 vbBlk;                    /* RK等平台的BLK信息，硬核需要用到 */
} TDE_HAL_SURFACE;


/**
 * @function    tde_hal_QuickResize
 * @brief       光栅位图缩放
 * @param[in]
 * @param[out]
 * @return
 */
INT32 tde_hal_QuickResize(TDE_HAL_SURFACE *pstSrc,TDE_HAL_RECT  *pstSrcRect,TDE_HAL_SURFACE *pstDst,TDE_HAL_RECT *pstDstRect);
/**
 * @function	tde_hal_QuickCopyYuv
 * @brief       快速拷贝yuv数据
 * @param[in]
 * @param[out]
 * @return
 */
INT32 tde_hal_QuickCopyYuv(TDE_HAL_SURFACE *pstSrc,
                           TDE_HAL_RECT  *pstSrcRect,
                           TDE_HAL_SURFACE *pstDst,
                           TDE_HAL_RECT *pstDstRect,
                           UINT32 u32SkipLen,
						   UINT32 bCached);

/**
 * @function	tde_hal_QuickCopy
 * @brief		快速拷贝yuv数据
 * @param[in]
 * @param[out]
 * @return
 */
INT32 tde_hal_QuickCopy(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect, UINT32 bCached);

/**
 * @function    tde_hal_QuickRotate
 * @brief       tde旋转90/180/270
 * @param[in]
 * @param[out]
 * @return
 */
INT32 tde_hal_QuickRotate(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect, TDE_HAL_ROTATION_E enRotateType);


/**
 * @function    tde_hal_DmaCopy
 * @brief       dma拷贝
 * @param[in]
 * @param[out]
 * @return
 */
INT32 tde_hal_DmaCopy(TDE_HAL_SURFACE *pstSrc,TDE_HAL_RECT  *pstSrcRect,TDE_HAL_SURFACE *pstDst,TDE_HAL_RECT *pstDstRect);


/**
 * @function	  tde_hal_Init
 * @brief     tde模块初始化
 * @param[in]
 * @param[out]
 * @return
 */
INT32 tde_hal_Init();



#endif


