/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: vdec_hal.h
   Description:
   Author:
   Date:
   Modification History:
******************************************************************************/
#ifndef _VDEC_HAL_INTER_
#define _VDEC_HAL_INTER_

#include "sal.h"
//#include "sal_type.h"
#include "vdec_hal_platfrom.h"

typedef struct
{
    BOOL curChnDecoding; /*当前解码通道是否在使用中*/
    BOOL bcreate;       /*当前解码通道是否被创建*/
    BOOL bstart;       /*当前解码通道是否被开启*/
} VDEC_HAL_STATUS;



/**
 * @function   vdec_hal_Init
 * @brief	   hal层初始化
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[out] None
 * @return	INT32
 */
INT32 vdec_hal_Init(UINT32 u32VdecChn);

/**
 * @function   vdec_hal_MemAlloc
 * @brief	   申请内存
 * @param[in]  UINT32 u32Format 格式
 * @param[in]  void *prm 入参
 * @param[out] None
 * @return
 */
VOID *vdec_hal_MemAlloc(UINT32 u32Format, void *prm);

/**
 * @function   vdec_hal_MemFree
 * @brief	   申请释放
 * @param[in]  void *ptr 指针
 * @param[in]  void *prm 入参
 * @param[out] None
 * @return INT32
 */
INT32 vdec_hal_MemFree(void *ptr, void *prm);

/**
 * @function   vdec_hal_VdecCreate
 * @brief	   解码器创建
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_PRM *vdecPrm 参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecCreate(UINT32 u32VdecChn, VDEC_PRM *vdecPrm);

/**
 * @function   vdec_hal_VdecStart
 * @brief	   启动解码器
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecStart(UINT32 u32VdecChn);

/**
 * @function   vdec_hal_VdecStop
 * @brief	   停止解码器
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecStop(UINT32 u32VdecChn);

/**
 * @function   vdec_hal_VdecDeinit
 * @brief	   释放解码器
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecDeinit(UINT32 u32VdecChn);

/**
 * @function   vdec_hal_VdecSetPrm
 * @brief	   设置解码器参数
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_PRM *vdecPrm 设置解码器参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecSetPrm(UINT32 u32VdecChn, VDEC_PRM *vdecPrm);

/**
 * @function   vdec_hal_VdecReset
 * @brief	   复位解码器
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecReset(UINT32 u32VdecChn);

/**
 * @function   vdec_hal_VdecClear
 * @brief	   清除解码器内缓存
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecClear(UINT32 u32VdecChn);

/**
 * @function   vdec_hal_VdecMakeframe
 * @brief	   生成一帧数据
 * @param[in]  PIC_FRAME_PRM *pPicFramePrm 数据
 * @param[in] void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecMakeframe(PIC_FRAME_PRM *pPicFramePrm, void *pstframe);

/**
 * @function   vdec_hal_VdecCpyframe
 * @brief	   数据拷贝
 * @param[in]  VDEC_PIC_COPY_EN *copyEn 拷贝方式
 * @param[in]  void *pstSrcframe 数据源
 * @param[in]  void *pstDstframe 目的
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecCpyframe(VDEC_PIC_COPY_EN *copyEn, void *pstSrcframe, void *pstDstframe);

/**
 * @function   vdec_hal_VdecGetframe
 * @brief	   获取数据帧
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecGetframe(UINT32 u32VdecChn, UINT32 dstChn, void *pstframe, UINT32 u32TimeOut);

/**
 * @function   vdec_hal_VdecReleaseframe
 * @brief	   释放数据帧
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecReleaseframe(UINT32 u32VdecChn, UINT32 dstChn, void *pstframe);

/**
 * @function   vdec_hal_VdecDecframe
 * @brief	   解码一帧数据
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in] SAL_VideoFrameBuf *pFrameIn 数据源
 * @param[in] SAL_VideoFrameBuf *pFrameOut 解码后数据
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecDecframe(UINT32 u32VdecChn, SAL_VideoFrameBuf *pFrameIn, SAL_VideoFrameBuf *pFrameOut, UINT32 u32TimeOut);

/**
 * @function   vdec_hal_getFreeChan
 * @brief	   获取可用通道
 * @param[in]
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_getFreeChan();

/**
 * @function   vdec_hal_VdecJpegSupport
 * @brief	   是否支持jpeg解码
 * @param[in]
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecJpegSupport();

/**
 * @function   vdec_hal_VdecGetBind
 * @brief	   解码器是否需要与其他模块绑定
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_BIND_CTL *pVdecBindCtl 参数
 * @param[out] None
 * @return	 INT32
 */
INT32 vdec_hal_VdecGetBind(UINT32 u32VdecChn, VDEC_BIND_CTL *pVdecBindCtl);

/**
 * @function   vdec_hal_VdecChnStatus
 * @brief	   当前解码器状态
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  VDEC_HAL_STATUS *pvdecStatus 状态
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_VdecChnStatus(UINT32 u32VdecChn, VDEC_HAL_STATUS *pvdecStatus);

/*******************************************************************************
* 函数名  : vdec_hal_MallocYUVMem
* 描  述  : 申请yuv内存
* 输  入  : - u32width: 解码通道
*		  -u32height 解码宽
*		addrPrm 申请内存参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 vdec_hal_MallocYUVMem(UINT32 u32width, UINT32 u32height, HAL_MEM_PRM *addrPrm);

/**
 * @function   vdec_hal_FreeYUVMem
 * @brief	   申请yuv内存
 * @param[in]  UINT32 u32width 宽
 * @param[in]  UINT32 u32height 高
 * @param[in]  HAL_MEM_PRM *addrPrm 数据地址
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_FreeYUVMem(HAL_MEM_PRM *addrPrm);

/**
 * @function   vdec_hal_Mmap
 * @brief	   帧数据映射地址
 * @param[in]  HAL_MEM_PRM *addrPrm 数据地址
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hal_Mmap(void *pVideoframe, PIC_FRAME_PRM *pPicprm);


#endif

