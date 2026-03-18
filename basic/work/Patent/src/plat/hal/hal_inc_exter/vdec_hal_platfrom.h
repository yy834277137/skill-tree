/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: vdec_hal_platfrom.h
   Description:
   Author:
   Date:
   Modification History:
******************************************************************************/
#ifndef _VDEC_HAL_PLATFORM_
#define _VDEC_HAL_PLATFORM_


#include "sal.h"
#include "sal_trace.h"
#include "sal_type.h"

#define MAX_VDEC_PLAT_CHAN 16
#define VDEC_WIDTH_DEF 1920
#define VDEC_HEIGH_DEF 1080
#define MIN_VDEC_WIDTH		114 /*解码通道最小宽度*/
#define MIN_VDEC_HEIGHT		114 /*解码通道最小高度*/
#define MAX_VDEC_WIDTH		1920 /*解码通道最大宽度*/
#define MAX_VDEC_HEIGHT		1088 /*解码通道最大高度*/
#define MAX_PPM_VDEC_WIDTH  2560 /*解码通道最大宽度*/
#define VDEC_BIND			1
#define VDEC_SUPPORT_JPEG	1



#define CHECK_PTR_NULL(ptr, ret) \
    if (!ptr) \
    { \
        VDEC_LOGE("null err\n"); \
        return ret; \
    }

#define CHECK_CHAN(u32VdecChn, value) \
    if (u32VdecChn >= MAX_VDEC_PLAT_CHAN) \
    { \
        VDEC_LOGE("Chan %d (Illegal parameters)\n", u32VdecChn); \
        return (value); \
    }
typedef enum
{
    MODE_SRC,
    MODE_DST,
} MODE_DATA_RES;

typedef struct tagVdecBindCtl
{
    BOOL needBind;                  /*是否需要绑定*/
    MODE_DATA_RES modeDataRes;      /*模块作为源还是接收者*/
} VDEC_BIND_CTL;

typedef struct _VDEC_PRM_
{
    UINT32 decWidth;            /*解码器宽*/
    UINT32 decHeight;           /*解码器高*/
    UINT32 encType;             /*视频编码类型*/
    UINT32 scaleLevel;          /*画面缩放*/
    VDEC_BIND_CTL vdecBindCtl;
} VDEC_PRM;

/*视频解码器属性*/
typedef struct _VDEC_HAL_PARAM_
{
    VDEC_PRM vdecPrm;           /*解码参数*/
    void *decPlatHandle;        /*解码器句柄*/
    UINT32 res[4];
} VDEC_HAL_PRM;

typedef enum
{
    VDEC_DATA_ADDR_COPY,
    VDEC_DATA_COPY,
    VDEC_COPY_DEFAULT
} VDEC_DATA_COPY_METHOD;

typedef struct tagVdecPicCopyEn
{
    INT32 copyth;       /*拷贝环境 0 hard 1 soft */
    VDEC_DATA_COPY_METHOD copyMeth;
} VDEC_PIC_COPY_EN;

typedef struct tagPicFramePrm
{
    UINT32 width;
    UINT32 height;
    UINT32 stride;
    UINT32 videoFormat;
    union
    {
        UINT32 poolId;
    };
    void *addr[3];
    PhysAddr phy[3];
} PIC_FRAME_PRM;

/* 通道 输出通道 属性 */
typedef struct tagHalMemPrm
{
    PhysAddr phyAddr;                /* 物理地址 */
    void *virAddr;                   /* 虚拟地址 */
    UINT32 memSize;                  /* 地址空间总大小 */
    UINT32 poolId;
    PhysAddr vbBlk;
} HAL_MEM_PRM;



#endif

