/*******************************************************************************
* svp_dsp_drv_api.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : ????<???@hikvision.com>
* Version: V1.0.0  
* Author : liuxianying<liuxianying@hikvision.com>
* Version: V1.0.0  2021年07月20日
*
* Description :
* Modification:
*******************************************************************************/


#ifndef __SVP_DSP_DRV_H_
#define __SVP_DSP_DRV_H_

//#include "platform_sdk.h"
#include "platform_hal.h"
#include "sal.h"

#define IS_USE_SVP_DSP (1)

#define SVP_DSP_TASK_NUM_MAX        (10)
#define SVP_DSP_TASK_BUFF_NUM       (5)

#define SVP_DSP_LAT_NUM_MAX         (2)

/* 任务完成，或者出错后释放资源类型 */
#define SVP_DSP_RLS_BUFF_FLAG       ((UINT32)0x01)      /* 释放已申请的缓存 */
#define SVP_DSP_RLS_CORE_FLAG       ((UINT32)0x02)      /* 释放正在执行任务的DSP核 */
#define SVP_DSP_CMD_LINE_OSD_USER   ((UINT32)0x103)     /* 对应hisi sdk SVP_DSP_CMD_E*/


/* 需要画线，叠框，OSD的最大个数 */
#define SVP_DSP_LINE_MAX          (128)
#define SVP_DSP_RECT_MAX          (128)
#define SVP_DSP_OSD_MAX           (128)

#pragma pack(8) 

/*************************** DSP核调度模块定义 ***********************/

/* 任务状态 */
typedef enum tagSvpDspTaskStatus
{
    SVP_DSP_TASK_STATUS_WAITTING,
    SVP_DSP_TASK_STATUS_READY,
    SVP_DSP_TASK_STATUS_RUN,
    SVP_DSP_TASK_STATUS_FINISHED,
}SVP_DSP_TASK_STATUS_E;

/* DSP核的状态 */
typedef enum tagSvpDspCoreStatus
{
    SVP_DSP_CORE_STATUS_FREE,
    SVP_DSP_CORE_STATUS_BUSY,
}SVP_DSP_CORE_STATUS_E;

/* 使用buff的模式 */
typedef enum tagSvpDspBuffMode
{
    SVP_DSP_BUFF_MODE_FOREVER,      /* 缓存申请后一直使用，使用完成后手动释放 */
    SVP_DSP_BUFF_MODE_ONCE,         /* 缓存使用一次，使用完成后自动释放 */
}SVP_DSP_BUFF_MODE_E;

typedef struct tagSvpDspTaskBuff
{
    UINT64 u64PhyAddr;
    VOID *pvVirAddr;
    UINT32 u32BuffLen;
    SVP_DSP_BUFF_MODE_E enBuffMode;
}SVP_DSP_TASK_BUFF_S;

typedef struct tagDspTask
{
    UINT32 u32CoreId;
    UINT32 enDspCmd;                      /*为hisisdk SVP_DSP_CMD_E 宏定义*/
    SVP_DSP_TASK_STATUS_E enTaskStatus;
    SVP_DSP_TASK_BUFF_S *pstTaskBuff;
}SVP_DSP_TASK_S;

typedef struct tagSvpDspVideoFrame
{
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 au32Stride[3];
    UINT64 au64PhyAddr[3];
    UINT64 au64VirAddr[3];
}SVP_DSP_VIDEO_FRAME_S;

typedef struct tagSvpDspTaskInitParam
{
    UINT32 enCmd;                           /*为hisisdk SVP_DSP_CMD_E 宏定义*/
    UINT32 u32ParamLen;
    DSA_QueHndl stFreeBuffQue;
    SVP_DSP_TASK_BUFF_S astTaskBuff[SVP_DSP_TASK_BUFF_NUM];
}SVP_DSP_TASK_INIT_PARAM_S;


/*************************** DSP核任务参数定义 ***********************/

/* 矩形定义 */
typedef struct tagSvpDspRect
{
    UINT32 u32X;
    UINT32 u32Y;
    UINT32 u32Width;
    UINT32 u32Height;
}SVP_DSP_RECT_S;

/* OSD叠加参数 */
typedef struct tagOsdParam
{
    UINT8 au8Lattice[8192];
    UINT32 u32LatPitch;
    SVP_DSP_RECT_S stOsdRect;
    UINT32 u32OsdColor;
    UINT32 u32BgColor;
    UINT32 u32BgEnable;
}SVP_DSP_OSD_S;

/* OSD叠加参数 */
typedef struct tagSvpDspOsdArrayParam
{
    UINT32 u32OsdNum;
    SVP_DSP_OSD_S astOsdParam[SVP_DSP_OSD_MAX];
}SVP_DSP_OSD_ARRAY_S;

/* 由ARM核传递给DSP核的参数 */
typedef struct tagSvpDspOsdParam
{
    SVP_DSP_VIDEO_FRAME_S stVideoFrame;
    SVP_DSP_OSD_ARRAY_S stOsdArray;
}SVP_DSP_OSD_PARAM_S;

typedef struct tagSvpDspLineType
{
    UINT32 u32FrameType;        /* 智能框画线类型 */
    UINT32 u32FullLength;       /* 实线长度*/
    UINT32 u32Gaps;             /* 缝隙长度*/
    UINT32 u32Node;             /* 缝隙中点的个数，点的总长度要小于缝隙长度*/
}SVP_DSP_LINE_TYPE_S;

typedef struct tagSvpDspLineArray
{
    UINT32 u32LineNum;
    SVP_DSP_LINE_TYPE_S astLineType[SVP_DSP_LINE_MAX];  /* 以下3个数组一一对应 */
    VGS_DRAW_PRM_S astVgsLinePrm[SVP_DSP_LINE_MAX];
    FLOAT32 af32XFract[SVP_DSP_LINE_MAX];
}SVP_DSP_LINE_ARRAY_S;

/* 由ARM核传递给DSP核的参数 */
typedef struct tagSvpDspDrawLineParam
{
    SVP_DSP_VIDEO_FRAME_S stVideoFrame;
    SVP_DSP_LINE_ARRAY_S stLineArray;
}SVP_DSP_DRAW_LINE_PRM_S;

typedef struct tagSvpDspDrawRectPrm
{
    SVP_DSP_LINE_TYPE_S stLineType;
    SVP_DSP_RECT_S stRectPrm;
    UINT32 u32Color;
    UINT32 u32Thick;
    FLOAT32 f32StartXFract;
    FLOAT32 f32EndXFract;
}SVP_DSP_DRAW_RECT_PRM_S;

typedef struct tagSvpDspRectArray
{
    UINT32 u32RectNum;
    UINT32 u32RectType;
    SVP_DSP_DRAW_RECT_PRM_S astRect[SVP_DSP_RECT_MAX];
}SVP_DSP_RECT_ARRAY_S;

/* 由ARM核传递给DSP核的参数 */
typedef struct tagSvpDspRectParam
{
    SVP_DSP_VIDEO_FRAME_S stVideoFrame;
    SVP_DSP_RECT_ARRAY_S stRectArray;
}SVP_DSP_RECT_PARAM_S;

/* 由ARM核传递给DSP核的参数 */
typedef struct tagSvpDspLineOsdParam
{
    SVP_DSP_VIDEO_FRAME_S stVideoFrame;
    SVP_DSP_LINE_ARRAY_S stLineArray;
    SVP_DSP_RECT_ARRAY_S stRectArray;
    SVP_DSP_OSD_ARRAY_S stOsdArray;
}SVP_DSP_LINE_OSD_PARAM_S;

INT32 svpdsp_func_init(UINT32 u32CoreNum, UINT32 *pu32Core);
INT32 SvpDsp_drvSetTimeTest(INT32 s32Test, INT32 s32Num);
INT32 svpdsp_func_getBuff(UINT32 enCmd, SVP_DSP_TASK_S *pstDspTask, SVP_DSP_BUFF_MODE_E enBuffMode);
INT32 svpdsp_func_releaseBuff(SVP_DSP_TASK_S *pstDspTask);
INT32 svpdsp_func_runTask(SVP_DSP_TASK_S *pstDspTask);
INT32 svpdsp_func_waitTaskFinish(SVP_DSP_TASK_S *pstDspTask);
VOID svpdsp_func_getVideoFrame(SAL_VideoFrameBuf *pstVideoFrame, SVP_DSP_VIDEO_FRAME_S *pstDspFrame);
VOID svpdsp_func_getLineType(DISPLINE_PRM *pstSrc, SVP_DSP_LINE_TYPE_S *pstDst);

#endif // __DSP_DRV_H_

