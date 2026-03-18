/******************************************************************************
   Copyright 2009-2012 Hikvision Co.,Ltd
   FileName: vdec_hisi.h
   Description:
   Author:
   Date:
   Modification History:
******************************************************************************/
#ifndef _VDEC_HISI_
#define _VDEC_HISI_

#include <sal.h>
#include "vdec_hal_platfrom.h"

/**
 * @function   vdec_hisi_VdecCreate
 * @brief	   解码器创建
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecCreate(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm);

/**
 * @function   vdec_hisi_VdecStart
 * @brief	   启动解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecStart(void *handle);

/**
 * @function   vdec_hisi_VdecStop
 * @brief	   停止解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecStop(void *handle);

/**
 * @function   vdec_hisi_VdecDeinit
 * @brief	   释放解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecDeinit(void *handle);

/**
 * @function   vdec_hisi_VdecSetPrm
 * @brief	   设置解码器参数
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 解码参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecSetPrm(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm);

/**
 * @function   vdec_hisi_VdecClear
 * @brief	   清除解码器数据
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecClear(void *handle);

/**
 * @function   vdec_hisi_VdecDecframe
 * @brief	   清除解码器数据
 * @param[in]  void *handle handle
 * @param[in]  SAL_VideoFrameBuf *pFrameIn 数据源
 * @param[in]  SAL_VideoFrameBuf *pFrameOut 输出数据
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecDecframe(void *handle, SAL_VideoFrameBuf *pFrameIn, SAL_VideoFrameBuf *pFrameOut);

/**
 * @function   vdec_hisi_VdecDecframe
 * @brief	   清除解码器数据
 * @param[in]  PIC_FRAME_PRM *pPicFramePrm 数据内存
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecMakeframe(PIC_FRAME_PRM *pPicFramePrm, void *pstframe);

/**
 * @function   vdec_hisi_VdecCpyframe
 * @brief	   数据拷贝
 * @param[in]  VDEC_PIC_COPY_EN *copyEn 拷贝方式
 * @param[in]  void *pstSrcframe 数据源
 * @param[in]  void *pstDstframe 目的
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecCpyframe(VDEC_PIC_COPY_EN *copyEn, void *pstSrcframe, void *pstDstframe);

/**
 * @function   vdec_hisi_VdecGetframe
 * @brief	   获取解码器数据帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecGetframe(void *handle, UINT32 dstChn, void *pstframe);


/**
 * @function   vdec_hisi_VdecReleaseframe
 * @brief	   释放解码帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecReleaseframe(void *handle, UINT32 dstChn, void *pstframe);

/**
 * @function   vdec_hisi_GetYuvPoolBlockMem
 * @brief	   申请yuv平台相关内存
 * @param[in]  UINT32 imgW 宽
 * @param[in]  UINT32 imgH 高
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_GetYuvPoolBlockMem(UINT32 imgW, UINT32 imgH, HAL_MEM_PRM *pOutFrm);

/**
 * @function   vdec_hisi_FreeYuvPoolBlockMem
 * @brief	   释放yuv内存
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_FreeYuvPoolBlockMem(HAL_MEM_PRM *addrPrm);

/**
 * @function   vdec_hisi_VdecMmap
 * @brief	   帧映射
 * @param[in]  void *pVideoFrame 数据帧
 * @param[in]  PIC_FRAME_PRM *pPicprm  映射地址
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_hisi_VdecMmap(void *pVideoFrame, PIC_FRAME_PRM *pPicprm);


#endif

