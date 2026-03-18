/**
 * @file
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  部分公用接口（从xpack剥离出）
 * @author
 * @date   2021/10/10
 * @note
 * @note \n History
   1.日    期: 2021/10/3
     作    者:
     修改历史: 创建文件
 */

#ifndef _PLAT_COM_INTER_HAL_H_
#define _PLAT_COM_INTER_HAL_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <platform_hal.h>
#include "dspcommon.h"
#include "../../task/ism/ximg_proc.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define SAL_COM_GREY_W_MAX	2304

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

typedef enum
{
    COPY_METHOD_HARD,
    COPY_METHOD_SOFT,
} COPY_METHOD;

typedef struct
{
    SAL_VideoDataFormat enPixFmt;
    UINT32 pixelDefault[4]; /*像素默认值*/
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Stride; /* 单位像素 */
    UINT32 u32Bpp; /* 每像素字节 */
    UINT32 u32BitDepth; /* 元素占用字节 单位字节 */
    UINT32 poolId;      /* 内存池id */
    UINT64 vbBlk;                    /* RK等平台的BLK信息，硬核需要用到 */
    VOID *VirAddr[4];
    UINT64 phyAddr[4];
} IMG;

/* xpack 拷贝输入参数 */
typedef struct tagComCopySerfaceSt
{
    IMG picPrm;
    BOOL bNeedGray;            /* 是否需要灰度 */
    UINT32 u32CopyYuvSkip;     /*拷贝不连续yuv的地址偏移量*/
} XPACK_IMG;

/** 
 * 构建System Frame的私有信息 
 * 注：仅用于透传一些私有信息，结构体中参数会复用，含义与其表面意思可能不符
 */
typedef struct
{
    UINT64 u64Pts;          // 时间戳
    UINT32 u32FrameNum;     // 帧序号
} SAL_SYSFRAME_PRIVATE;

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

INT32 SAL_halGetPixelBpp(SAL_VideoDataFormat format, UINT32 *bpp);
INT32 SAL_halGetPixelMemSize(UINT32 u32Width, UINT32 u32Height, SAL_VideoDataFormat format, UINT32 *u32Size);
INT32 SAL_halSetSrcFramePrm(SAL_VideoFrameBuf *pstVideoFrame, UINT32 u32TimeRef, UINT64 framePts);
INT32 SAL_halSoftCopyYuv(XPACK_IMG *pstSrc, XPACK_IMG *pstDst);
INT32 SAL_halSoftCopyRaw(XPACK_IMG *pstSrc, XPACK_IMG *pstDst);
INT32 SAL_halSoftCopyRgb(XPACK_IMG *pstSrc, XPACK_IMG *pstDst);
INT32 SAL_halDataCopy(XPACK_IMG *pstSrc, XPACK_IMG *pstDst);
INT32 SAL_halMakeFrame(void *virAddr, PhysAddr phyAddr, UINT32 pool, UINT32 w, UINT32 h, UINT32 s, INT32 pixelFormat, SAL_VideoFrameBuf *pstVideoFrame);

/**
 * @fn      Sal_halBuildSysFrame
 * @brief   构建System Frame信息
 * @param   [OUT] pstSysFrame       System Frame信息
 * @param   [OUT] pstVidFrame       Video Frame信息，可为NULL，NULL则不输出
 * @param   [OUT]      poolId       内存池Id
 * @param   [OUT]      vbBlk        帧缓存块地址
 * @param   [OUT]      pts          时间戳信息
 * @param   [OUT]      frameNum     帧序号信息
 * @param   [OUT]      virAddr      视频帧起始虚拟地址
 * @param   [OUT]      phyAddr      视频帧起始物理地址
 * @param   [OUT]      stride       视频帧内存跨距，单位：pixel
 * @param   [OUT]      vertStride   视频帧内存竖向跨距，不能小于抠图高+视频帧高，单位：pixel
 * @param   [OUT]      frameParam   视频帧尺寸信息
 * @param   [OUT]           width       视频帧宽度信息
 * @param   [OUT]           height      视频帧高度信息
 * @param   [OUT]           dataFormat  视频帧数据格式
 * @param   [OUT]           crop        视频帧内存抠取信息
 * @param   [OUT]               left        实际视频帧数据起点距离内存块起始的水平偏移，单位：byte
 * @param   [OUT]               top         实际视频帧数据起点距离内存块起始的竖直偏移，单位：byte
 * @param   [OUT]               width       抠取宽，等于实际视频帧宽，单位：pixel，不使用
 * @param   [OUT]               height      抠取高，等于实际视频帧高，单位：pixel，不使用
 * @param   [IN]  *pstImgData       实际视频帧数据
 * @param   [IN]        stMbAttr        视频帧使用的内存块信息
 * @param   [IN]  pstPriv           私有信息，可为NULL，当为NULL时即无私有信息 
 * @return  SAL_STATUS SAL_SOK：构建成功，SAL_FAIL：构建失败
 */
SAL_STATUS Sal_halBuildSysFrame(SYSTEM_FRAME_INFO *pstSysFrame, SAL_VideoFrameBuf *pstVidFrame, XIMAGE_DATA_ST *pstImgData, SAL_SYSFRAME_PRIVATE *pstPriv);


#endif

