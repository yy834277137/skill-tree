/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: vdec_tsk.h
   Description:
   Author:
   Date:
   Modification History:
******************************************************************************/
#ifndef _VDEC_TSK_
#define _VDEC_TSK_

#include "sal.h"
#include "platform_hal.h"

/*
    解码器状态
 */
typedef enum
{
    VDEC_STATE_UNPREPARED,
    VDEC_STATE_PREPARING,
    VDEC_STATE_PREPARED,
    VDEC_STATE_RUNNING,
    VDEC_STATE_PAUSE,
    VDEC_STATE_STOPPED,
    VDEC_BUIT
} VDEC_STATUS;

/*获取解码帧用途*/
typedef enum
{
    VDEC_DISP_GET_FRAME,
    VDEC_JPEG_GET_FRAME,
    VDEC_I_TO_BMP_GET_FRAME,
    VDEC_PLAY_BACK_GET_FRAME,
    VDEC_BA_GET_FRAME,
    VDEC_FACE_GET_FRAME,
    VDEC_DEFAULT
} VDEC_GET_FRAME_PURPOSE;

typedef struct tagDecHalPicPrmSt
{
    UINT32 uiW;
    UINT32 uiH;
} VDEC_HAL_PIC_PRM;

typedef struct 
{
	BOOL bind;	/*是否已经绑定显示通道*/
	UINT32 enable[4];	/*通道使能情况*/
	RECT rect[4];		/*显示窗口大小以及位置*/
}DISP_WINDOW_PRM;

typedef struct tagVDecPicPrmSt
{
    UINT32 uiW;
    UINT32 uiH;
} VDEC_TSK_PIC_PRM;

/*jpg解码buff*/
typedef struct tagVDecJpgBuffST
{
    UINT32 uiUseFlag;    /* 该内存是否被占用 */
	UINT32 uiBufSize;    /* 缓存大小 */
	UINT64 PhyAddr;      /* 物理地址 */
	UINT64 VirAddr;      /* 虚拟地址 */
    UINT32 u32Pool;
    UINT64 vbBlk;        /* 平台层需要使用到的blk */
} VDEC_JPG_BUFF_ST;

/*jpg2BMP全局参数*/
typedef struct tagVDecTskJpgToBmpPrmST
{
    VDEC_JPG_BUFF_ST stJpgDecBuff;
    VDEC_JPG_BUFF_ST stJpg2YuvBuf;
    SYSTEM_FRAME_INFO stSystemFrmInfo;
} VDEC_TSK_JPGTOPMP_PRM;


/**
 * @function	vdec_tsk_ModuleCreate
 * @brief	  解码模块创建
 * @param[in]
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_ModuleCreate();

/**
 * @function	vdec_tsk_Start
 * @brief	  启动解码器
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_Start(UINT32 s32VdecChn, void *prm);

/**
 * @function	vdec_tsk_Stop
 * @brief	 停止解码
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_Stop(UINT32 s32VdecChn, void *prm);

/**
 * @function	vdec_tsk_Pause
 * @brief	 解码暂停
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_Pause(UINT32 s32VdecChn, void *prm);

/**
 * @function	vdec_tsk_SetPrm
 * @brief	 设置参数
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SetPrm(UINT32 s32VdecChn, void *prm);

/**
 * @function	vdec_tsk_SetSynPrm
 * @brief	 设置同步参数
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SetSynPrm(UINT32 s32VdecChn, void *prm);

/**
 * @function	vdec_tsk_ReSet
 * @brief	 解码复位
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_ReSet(UINT32 s32VdecChn);

/**
 * @function	vdec_tsk_AudioEnable
 * @brief	 启动音频解码
 * @param[in] UINT32 u32VdecChn 解码通道号
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_AudioEnable(UINT32 s32VdecChn, void *prm);

/**
 * @function	vdec_tsk_GetProc
 * @brief	 proc信息查看
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_GetProc(void *prm);

/**
 * @function   vdec_tsk_GetVdecChnStatus
 * @brief	   当前解码器状态
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  INT32 *pCreat 解码器是否被创建
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_tsk_GetVdecChnStatus(UINT32 u32VdecChn, UINT32 *pCreat);

/**
 * @function	vdec_tsk_GetProc
 * @brief	 proc信息查看
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_VdecDecframe(UINT32 u32VdecChn, SAL_VideoFrameBuf *pstSrcframe, SAL_VideoFrameBuf *pstDstframe);

/**
 * @function   vdec_tsk_GetVdecFrame
 * @brief     其他模块获取解码数据帧接口
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 voDev 通道号
 * @param[in]  UINT32 purpose 数据帧用途
 * @param[in]  UINT32 *needFree 是否需要释放
 * @param[in]  UINT32 *status 是否允许叠框
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_GetVdecFrame(UINT32 vdecChn, UINT32 voDev, UINT32 purpose, UINT32 *needFree, SYSTEM_FRAME_INFO *pstFrame, UINT32 *status);

/**
 * @function   vdec_tsk_PutVdecFrame
 * @brief     释放解码数据帧
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 chn 释放帧通道号
 * @param[in]  SYSTEM_FRAME_INFO *prm 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_PutVdecFrame(UINT32 u32VdecChn, UINT32 chn, SYSTEM_FRAME_INFO *prm);

/**
 * @function   vdec_tsk_PutVdecFrame
 * @brief     释放解码数据帧
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 voDev 输出通道号
 * @param[in]  SYSTEM_FRAME_INFO *prm 数据帧
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_PutVideoFrmaeMeth(UINT32 vdecChn, UINT32 voDev, SYSTEM_FRAME_INFO *pstFrame);

/**
 * @function   vdec_tsk_GetChnStatus
 * @brief     获取当前解码器通道状态
 * @param[in]  UINT32 vdecChn 解码通道
 * @param[in]  UINT32 *pStatus 状态值
 * @param[out] None
 * @return     static INT32
 */
INT32 vdec_tsk_GetChnStatus(UINT32 u32VdecChn, UINT32 *pStatus);

/**
 * @function	vdec_tsk_MemQuickCpoy
 * @brief	快速拷贝
 * @param[in] void *prm 入参
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_MemQuickCpoy(void *prm);

/**
 * @function	vdec_tsk_SaveYuv2Bmp
 * @brief	yuv转bmp
 * @param[in] BMP_RESULT_ST *date bmp 数据
 * @param[in] SYSTEM_FRAME_INFO *pVFrame 数据帧
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SaveYuv2Bmp(SYSTEM_FRAME_INFO *pVFrame, BMP_RESULT_ST *date);

/**
 * @function	vdec_tsk_Nv21ToRgb24P
 * @brief	nv21 转 rgb
 * @param[in] U08 *pSrc nv21数据
 * @param[in] U08* pRBuf 红色分量
 * @param[in] U08*pGBuf 绿色分量
 * @param[in] U08* pBBuf 蓝色分量
 * @param[in] UINT32 u32Width 宽
 * @param[in] UINT32 u32Height 高
 * @param[in] UINT32 u32Stride 跨距
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_Nv21ToRgb24P(U08 *pSrc, U08 *pRBuf, U08 *pGBuf, U08 *pBBuf, UINT32 u32Width, UINT32 u32Height, UINT32 u32Stride);

/**
 * @function	vdec_tsk_Bmp2Nv21
 * @brief	 bmp转nv21
 * @param[in] CHAR *pBmp bmp数据
 * @param[in] CHAR *pYuv yuv数据
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_Bmp2Nv21(CHAR *pBmp, CHAR *pYuv, void* prm);

/**
 * @function	vdec_tskGetJpegInfo
 * @brief	 获取jpeg参数
 * @param[in] void *prm 入参
 * @param[in] UINT32 *w 宽度
 * @param[in] UINT32 *h 高度
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tskGetJpegInfo(void *prm, UINT32 *w, UINT32 *h);

/**
 * @function	vdec_tsk_ScaleJpegChn
 * @brief	jpeg缩放
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] UINT32 chn 子通道
 * @param[in] UINT32 width 宽
 * @param[in] UINT32 height 高
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_ScaleJpegChn(UINT32 uiChn, UINT32 uiVpssChn, UINT32 width, UINT32 height);

/**
 * @function	vdec_tsk_SetOutChnResolution
 * @brief	配置解码输出通道宽高
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] UINT32 u32OutChn 子通道
 * @param[in] UINT32 width 宽
 * @param[in] UINT32 height 高
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SetOutChnResolution(UINT32 u32VdecChn, UINT32 u32OutChn, UINT32 width, UINT32 height);

/**
 * @function   vdec_tsk_SetOutChnDataFormat
 * @brief      更新解码器绑定的vpss输出通道的图像格式
 * @param[in]  UINT32 u32VdecChn                 
 * @param[in]  UINT32 u32OutChn                  
 * @param[in]  SAL_VideoDataFormat enDataFormat  
 * @param[out] None
 * @return     INT32
 */
INT32 vdec_tsk_SetOutChnDataFormat(UINT32 u32VdecChn, UINT32 u32OutChn, SAL_VideoDataFormat enDataFormat);

/**
 * @function	vdec_tsk_SendJpegFrame
 * @brief	解码jpeg图片
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] VOID *pBuf 数据
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SendJpegFrame(UINT32 u32VdecChn, VOID *pAddr, UINT32 u32StrLen);

/**
 * @function	vdec_tsk_GetJpegYUVFrame
 * @brief	解码jpeg后的yuv数据
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] UINT32 chn 子通道
 * @param[in] void *pBuf 数据
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_GetJpegYUVFrame(UINT32 uiChn, UINT32 uiVpssChn, void *pBuf);

/**
 * @function	vdec_tsk_SetDbLeave
 * @brief	设置log等级
 * @param[in] int level 等级
 * @param[out] None
 * @return  static INT32
 */
void vdec_tsk_SetDbLeave(int level);

/**
 * @function	vdec_tsk_SetDispPrm
 * @brief	解码jpeg后的yuv数据
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] DISP_WINDOW_PRM prm 显示参数
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_SetDispPrm(UINT32 u32VdecChn,DISP_WINDOW_PRM *prm);

/**
 * @function	vdec_tsk_CreateVdec
 * @brief	创建解码器
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] VDEC_PRM *pstVdecPrm 解码器创建参数
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_CreateVdec(UINT32 u32VdecChn, VDEC_PRM *pstVdecPrm);

/**
 * @function	vdec_tsk_DestroyVdec
 * @brief	销毁解码器
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[in] BOOL bVdecDupBindSts 当前解码器和dup绑定状态
 * @param[in] unsigned int u32FrontDupHdl  dup前级通道句柄
 * @param[in] unsigned int u32RearDupHdl   dup后级通道句柄
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_DestroyVdec(UINT32 u32VdecChn, BOOL bVdecDupBindSts, unsigned int u32FrontDupHdl, unsigned int u32RearDupHdl);

/**
 * @function	Vdec_task_vdecDupChnHandleCreate
 * @brief	创建人包解码器
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return  static INT32
 */
INT32 Vdec_task_PpmDupChnHandleCreate(UINT32 u32VdecChn, void* pFrontDupHandle);

/**
 * @function	vdec_tsk_PpmSetOutChnResolution
 * @brief	配置解码输出通道宽高
 * @param[in] UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return  static INT32
 */
INT32 vdec_tsk_PpmSetOutChnResolution(UINT32 u32VdecChn, UINT32 u32OutChn, UINT32 width, UINT32 height);

/**
 * @function	vdec_tsk_JpgToBmp
 * @brief	
 * @param[in] 
 * @param[out] None
 * @return  static INT32
 */

INT32 vdec_tsk_JpgToBmp(DECODER_JPGTOBMP_PRM *pstDecJpg2BmpPrm);

/**
 * @function	vdec_tsk_JpgDecBuffInit
 * @brief	
 * @param[in] 
 * @param[in] 
 * @param[out] None
 * @return  static INT32
 */

INT32 vdec_tsk_JpgDecBuffInit();



#endif

