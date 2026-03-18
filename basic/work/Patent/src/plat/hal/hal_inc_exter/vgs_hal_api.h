/*******************************************************************************
* vgs_hal_api.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2018年03月13日 Create
*
* Description : 硬件平台 vgs 模块
* Modification:
*******************************************************************************/
#ifndef _VGS_HAL_H_
#define _VGS_HAL_H_

/*****************************************************************************
                                头文件
*****************************************************************************/
#include <sys/ioctl.h>
#include "sal.h"
#include "capbility.h"


/*****************************************************************************
                                宏定义
*****************************************************************************/

#define MAX_VGS_LINE_NUM	(128 * 4)
#define MAX_VGS_OSD_NUM		(128)
#define MAX_VGS_RECT_NUM    (128)
#define MAX_VGS_COVER_NUM	(MAX_VGS_OSD_NUM)
#define MAX_VGS_MOSAIC_NUM	(MAX_VGS_OSD_NUM)


/*****************************************************************************
                                结构体定义
*****************************************************************************/

typedef struct tagVgsLinePrmSt
{
    UINT32 uiStartX;       /* 起始点 x */
    UINT32 uiStartY;       /* 起始点 y */
    UINT32 uiEndX;         /* 结束点 x */
    UINT32 uiEndY;         /* 结束点 y */

    UINT32 uiLineWidth;    /* 线宽度   */
    UINT32 uiColor;        /* 线的颜色 */
} VGS_LINE_PRM;

typedef struct tagDispArticleLineType
{
    ARTICLE_TYPE article_type;
    DISPLINE_PRM ailine;          /* 智能画线参数 */
    DISPLINE_PRM orgnaicline;         /* TIP画线参数 */
    DISPLINE_PRM tipline;     /* 有机物画线参数 */
    DISPLINE_PRM unpenline;         /* 难穿透画线参数 */
} VGS_ARTICLE_LINE_TYPE;

typedef struct tagVgsRect
{
    INT32 s32X;
    INT32 s32Y;
    UINT32 u32Width;
    UINT32 u32Height;
} VGS_RECT_S;

typedef struct tagVgsDrawRect
{
    INT32 s32X;
    INT32 s32Y;
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Thick;
    UINT32 u32Color;
    FLOAT32 f32StartXFract;
    FLOAT32 f32EndXFract;
    DISPLINE_PRM stLinePrm;
} VGS_DRAW_RECT_S;

typedef struct tagVgsRectArray
{
    UINT32 u32RectNum;
    RECT_EXCEED_TYPE_E enExceedType;
    VGS_DRAW_RECT_S astRect[MAX_VGS_RECT_NUM];
} VGS_RECT_ARRAY_S;

typedef struct tagPOINT_S {
    INT32 s32X;
    INT32 s32Y;
} VGS_POINT;

typedef struct tagVGS_DRAW_LINE_S {
    VGS_POINT stStartPoint; /* Line start point */
    VGS_POINT stEndPoint;   /* Line end point */
    UINT32 u32Thick;      /* RW; Width of line */
    UINT32 u32Color;      /* RW; Range: [0,0xFFFFFF]; Color of line */
} VGS_DRAW_PRM_S;

typedef struct tagVgsDrawLinePrmSt
{
    UINT32 uiLineNum;                              /* 总的线数 */
    FLOAT32 af32XFract[MAX_VGS_LINE_NUM];
    DISPLINE_PRM linetype[MAX_VGS_LINE_NUM];
    VGS_DRAW_PRM_S stVgsLinePrm[MAX_VGS_LINE_NUM]; /* 每个通道的信息 */
} VGS_DRAW_LINE_PRM;

/* vgs 画osd  参数*/
typedef struct tagVgsOsdPrmSt
{
    INT32 s32X;                                    /* osd水平起始点 */
    INT32 s32Y;                                    /* osd 垂直起始点 */
    INT32 u32Width;                                /* osd  宽*/
    INT32 u32Height;                               /* osd  高*/
    UINT32 u32BgColor;                              /* osd背景颜色 */
    SAL_VideoDataFormat enPixelFmt;                 /* osd像素格式*/
    UINT64 u64PhyAddr;                              /* osd物理地址 */
    UINT8 *pVirAddr;
    UINT32 u32Stride;                               /* osd stride */
    UINT32 u32BgAlpha;                              /* osd背景透明值 */
    UINT32 u32FgAlpha;                              /* osd字体透明值 */
    UINT32 bOsdRevert;                              /* 是否使能osd反色 */
} VGS_OSD_PRM;

typedef struct tagVgsAddOsdPrmSt
{
    UINT32 uiOsdNum;                              /* osd 总的个数 */
    VGS_OSD_PRM stVgsOsdPrm[MAX_VGS_OSD_NUM * 2]; /* 每个osd 的信息 */
} VGS_ADD_OSD_PRM;

typedef enum tagVgsHalCoverTypeE 
{
    VGS_HAL_COVER_RECT = 0,             /* Retangle cover */
    VGS_HAL_COVER_QUAD_RANGLE,          /* Quadrangle cover */
    VGS_HAL_COVER_BUTT
} VGS_HAL_COVER_TYPE_E;

typedef struct tagVgsQuadrangleCoverSt 
{
    BOOL              bSolid;            /* Solid or hollow cover */
    UINT32            u32Thick;          /* RW; Range: [2,8]; Thick of the hollow quadrangle */
    VGS_POINT         stPoint[4];        /* Four points of the quadrangle */
} VGS_QUAD_COVER_S;

typedef struct tagVgsRectSt 
{
    INT32 s32X;
    INT32 s32Y;
    UINT32 u32Width;
    UINT32 u32Height;
} VGS_HAL_RECT_S;

typedef struct tagVgsCoverPrmSt 
{
    VGS_HAL_COVER_TYPE_E            enCoverType;    /* Cover type */
	
    union 
	{
        VGS_HAL_RECT_S              stDstRect;      /* The rectangle area */
        VGS_QUAD_COVER_S            stQuadRangle;   /* The quadrangle area */
    };

    UINT32                          u32Color;       /* RW; Range: [0,0xFFFFFF]; Color of cover */
} VGS_COVER_PRM_S;

typedef struct tagVgsAddCoverPrmSt
{
	UINT32 u32CoverNum;
	VGS_COVER_PRM_S astCoverPrm[MAX_VGS_COVER_NUM];
} VGS_ADD_COVER_PRM_S;

typedef enum tagVgsMosaicTypeE
{
    VGS_HAL_MOSAIC_BLK_SIZE_8  = 8, /* block size 8*8 of MOSAIC */
    VGS_HAL_MOSAIC_BLK_SIZE_16 = 16, /* block size 16*16 of MOSAIC */
    VGS_HAL_MOSAIC_BLK_SIZE_32 = 32, /* block size 32*32 of MOSAIC */
    VGS_HAL_MOSAIC_BLK_SIZE_64 = 64, /* block size 64*64 of MOSAIC */
    VGS_HAL_MOSAIC_BLK_SIZE_BUT
} VGS_HAL_MOSAIC_BLK_SIZE_E;

typedef struct tagVgsMosaicPrmSt 
{
    VGS_HAL_MOSAIC_BLK_SIZE_E enBlkSize;
    VGS_HAL_RECT_S stDstRect;
} VGS_MOSAIC_PRM_S;

typedef struct tagVgsAddMosaicPrmSt
{
	UINT32 u32MosaicNum;
	VGS_MOSAIC_PRM_S astMosaicPrm[MAX_VGS_MOSAIC_NUM];
} VGS_ADD_MOSAIC_PRM_S;

typedef enum tagVgsCropCoorTypeE 
{
    VGS_HAL_CROP_RATIO_COOR = 0,   /* Ratio coordinate. [0, 999], ratio_x * 1000 */
    VGS_HAL_CROP_ABS_COOR          /* Absolute coordinate. */
} VGS_HAL_CROP_COORDINATE_E;

typedef struct tagVgsCropPrmSt 
{
    VGS_HAL_CROP_COORDINATE_E   enCropCoordinate;   /* RW; Range: [0, 1]; Coordinate mode of the crop start point. */
    VGS_HAL_RECT_S              stCropRect;         /* CROP rectangular. */
} VGS_HAL_CROP_PRM_S;

typedef enum tagVgsRotateTypeE 
{
    VGS_HAL_ROTATION_0        = 0,
    VGS_HAL_ROTATION_90       = 1,
    VGS_HAL_ROTATION_180      = 2,
    VGS_HAL_ROTATION_270      = 3,
    VGS_HAL_ROTATION_BUTT
} VGS_HAL_ROTATE_TYPE_E;

typedef struct tagDispDrawAiPrm
{
    VGS_DRAW_LINE_PRM VgsDrawVLine;
    VGS_RECT_ARRAY_S VgsDrawRect;
} VGS_DRAW_AI_PRM;


/* vgs 功能模块 */
typedef enum TAG_VGS_MODE_E
{
    VGS_MODE_NONE = 0,        /* Not revert */
    VGS_MODE_DRAWLINE,        /* vgs画线模块 */
    VGS_MODE_OSD,             /* vgs osd模块*/
    VGS_MODE_BUTT
} TAG_VGS_MODE;

/* vgs 的参数类型 */
typedef enum TAG_VGS_ALIGN_TYPE_E
{
    VGS_ALIGN_NONE = 0,       /* Not revert */
    VGS_ALIGN_X,              /* vgs x 对齐*/
    VGS_ALIGN_Y,              /* vgs y 对齐*/
    VGS_ALIGN_W,              /* vgs 宽对齐  */
    VGS_ALIGN_H,              /* vgs 高对齐*/
    VGS_ALIGN_STRIED,         /* vgs 跨距对齐*/
    VGS_ALIGN_ADDR,           /* vgs 地址对齐*/
    VGS_ALIGN_BUTT
} VGS_ALIGN_TYPE;

/* 叠加一直变化的OSD信息 */
typedef struct tagVgsVarOsdInfo
{
    UINT32 u32Len;              /* 字符长度 */
    VOID *pvLattticeAddr;
    VOID *pvFontAddr;
    UINT64 u64LattticePhyAddr;
    UINT64 u64FontPhyAddr;
} VGS_VAR_OSD_INFO_S;

typedef struct tagVgsVarOsdArray
{
    UINT32 u32Num;
    VGS_VAR_OSD_INFO_S *pstVarInfo;
} VGS_VAR_OSD_ARRAY_S;

typedef struct tagVgsVarOsdChnInfo
{
	BOOL bUse;
    VGS_VAR_OSD_ARRAY_S stVarOsdArray;
} VGS_VAR_OSD_CHN_INFO;


/*****************************************************************************
                            函数声明
*****************************************************************************/

/*****************************************************************************
   函 数 名  : vgs_hal_drawLineArray
   功能描述  : 使用 VGS 画多条线
   输入参数  : stTask vgs画线任务参数
                             pstLinePrm 画线的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hal_drawLineArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_DRAW_LINE_PRM *pstLinePrm);

/*****************************************************************************
   函 数 名  : vgs_hal_drawOSDArray
   功能描述  : 使用 VGS 画osd
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hal_drawOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm);

/*****************************************************************************
   函 数 名  : vgs_hal_drawOsdSingle
   功能描述  : 使用 VGS 画osd
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hal_drawOsdSingle(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm);


/*****************************************************************************
   函 数 名  : vgs_hal_drawLineOSDArray
   功能描述  : 使用 VGS 画osd和框
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
                             pstLinePrm画框参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hal_drawLineOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm, VGS_DRAW_LINE_PRM *pstLinePrm);

/*****************************************************************************
   函 数 名  : vgs_hal_scaleFrame
   功能描述  : 使用 VGS 缩放
   输入参数  : stTask vgs缩放任务参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hal_scaleFrame(SYSTEM_FRAME_INFO *pDstSystemFrame, SYSTEM_FRAME_INFO *pSrcSystemFrame, UINT32 dstW, UINT32 dstH);

/*******************************************************************************
* 函数名  : vgs_hal_init
* 描  述  : 初始化vgs模块
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 vgs_hal_init(void);


#endif /* _VGS_HAL_H_*/

