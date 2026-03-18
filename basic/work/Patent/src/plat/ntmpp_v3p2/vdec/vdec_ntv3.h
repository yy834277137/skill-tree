/**
 * @file   vdec_ntv3.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  NT平台解码模块接口
 * @author liuxianying
 * @date   2021/12/27
 * @note
 * @note \n History
   1.日    期: 2021/12/27
     作    者: liuxianying
     修改历史: 创建文件
 */

#ifndef _VDEC_ntv3_
#define _VDEC_ntv3_

#include <sal.h>
#include "vdec_hal_platfrom.h"


VOID *vdec_ntv3_MemAlloc(UINT32 u32Format, void *prm);
INT32 vdec_ntv3_MemFree(void *ptr, void *prm);

/**
 * @function   vdec_ntv3_VdecCreate
 * @brief	  解码器创建
 * @param[in]  UINT32 u32VdecChn 通道号
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecCreate(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm);

/**
 * @function   vdec_hisi_VdecStart
 * @brief	   启动解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecStart(void *handle);

/**
 * @function   vdec_ntv3_VdecStop
 * @brief	   停止解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecStop(void *handle);

/**
 * @function   vdec_ntv3_VdecDeinit
 * @brief	   释放解码器
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecDeinit(void *handle);

/**
 * @function   vdec_ntv3_VdecSetPrm
 * @brief	   设置解码器参数
 * @param[in]  UINT32 u32VdecChn 解码通道
 * @param[in]  VDEC_HAL_PRM *pVdecHalPrm 解码参数
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecSetPrm(UINT32 u32VdecChn, VDEC_HAL_PRM *pVdecHalPrm);

/**
 * @function   vdec_ntv3_VdecClear
 * @brief	   清除解码器数据
 * @param[in]  void *handle handle
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecClear(void *handle);

/**
 * @function   vdec_ntv3_VdecDecframe
 * @brief	   清除解码器数据
 * @param[in]  void *handle handle
 * @param[in]  SAL_VideoFrameBuf *pFrameIn 数据源
 * @param[in]  SAL_VideoFrameBuf *pFrameOut 输出数据
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecDecframe(void *handle, SAL_VideoFrameBuf *pFrameIn, SAL_VideoFrameBuf *pFrameOut);

/**
 * @function   vdec_ntv3_VdecMakeframe
 * @brief	   生成帧数据
 * @param[in]  PIC_FRAME_PRM *pPicFramePrm 数据内存
 * @param[in]  void *pstVideoFrame 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecMakeframe(PIC_FRAME_PRM *pPicFramePrm, void *pstframe);

/**
 * @function   vdec_ntv3_VdecCpyframe
 * @brief	   数据拷贝
 * @param[in]  VDEC_PIC_COPY_EN *copyEn 拷贝方式
 * @param[in]  void *pstSrcframe 数据源
 * @param[in]  void *pstDstframe 目的
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecCpyframe(VDEC_PIC_COPY_EN *copyEn, void *pstSrcframe, void *pstDstframe);

/**
 * @function   vdec_ntv3_VdecGetframe
 * @brief	   获取解码器数据帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecGetframe(void *handle, UINT32 dstChn, void *pstframe);

/**
 * @function   vdec_ntv3_VdecReleaseframe
 * @brief	   释放解码帧
 * @param[in]  void *handle handle
 * @param[in]  UINT32 dstChn 子通道
 * @param[in]  void *pstframe 数据帧
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecReleaseframe(void *handle, UINT32 dstChn, void *pstframe);

/**
 * @function   vdec_ntv3_GetYuvPoolBlockMem
 * @brief	   申请yuv平台相关内存
 * @param[in]  UINT32 imgW 宽
 * @param[in]  UINT32 imgH 高
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_GetYuvPoolBlockMem(UINT32 imgW, UINT32 imgH, HAL_MEM_PRM *pOutFrm);

/**
 * @function   vdec_ntv3_FreeYuvPoolBlockMem
 * @brief	   释放yuv内存
 * @param[in]  HAL_MEM_PRM *pOutFrm 内存保存
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_FreeYuvPoolBlockMem(HAL_MEM_PRM *addrPrm);

/**
 * @function   vdec_ntv3_VdecMmap
 * @brief	   帧映射
 * @param[in]  void *pVideoFrame 数据帧
 * @param[in]  PIC_FRAME_PRM *pPicprm  映射地址
 * @param[out] None
 * @return	  INT32
 */
INT32 vdec_ntv3_VdecMmap(void *pVideoFrame, PIC_FRAME_PRM *pPicprm);


#endif

