/**
 * @file   sys_hal.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  系统模块抽象层，包括公共vb的初始化，SDK绑定的封装，视频帧结构体的分配释放，视频帧的构建。
 * @author yeyanzhong
 * @date   2021.6.22
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#ifndef _SYS_HAL_H_

#define _SYS_HAL_H_

#include "sal.h"

/* 平台硬件模块ID,根据硬件平台可增加 */
typedef enum tagSystemModIDEn
{
    SYSTEM_MOD_ID_SYS = 0,     /* 系统模块 */
    SYSTEM_MOD_ID_CAPT = 1,    /* 采集模块 */
    SYSTEM_MOD_ID_VENC = 2,    /* 编码模块 */
    SYSTEM_MOD_ID_JPEG = 3,    /* 编码模块 */
    SYSTEM_MOD_ID_VDEC = 4,    /* 解码模块 */
    SYSTEM_MOD_ID_DISP = 5,    /* 显示模块 */
    SYSTEM_MOD_ID_AUDIO = 6,   /* 音频模块 */
    SYSTEM_MOD_ID_DUP = 7,     /* DUP模块  */
    SYSTEM_MOD_ID_NULL = 8,    /* NULL模块 */
    SYSTEM_MOD_ID_MD = 9,      /* MD  模块 */
    SYSTEM_MOD_ID_QR = 10,     /* 二维码  模块 */
    SYSTEM_MOD_ID_DFR = 11,    /* 智能  模块 */
    SYSTEM_MOD_ID_FRAME_OUT = 12,  /* 帧数据输出   */
    SYSTEM_MOD_ID_BITS_OUT = 13,   /* 码流数据输出 */
    SYSTEM_MOD_ID_FRAME_IN = 14,   /* 帧数据输入   */
    SYSTEM_MOD_ID_BITS_IN = 15,    /* 码流数据输入 */
    SYSTEM_MOD_ID_MAX
} SYSTEM_MOD_ID_E;

/* 模块数据传输结构体 */
typedef struct tagSystemFrameInfo
{
    PhysAddr uiDataAddr;    /* 存放访问图像帧的起始虚拟地址 */
    UINT32 u32LeftOffset;    /* 从大的缓存区构建一帧videoFrame图像帧时，实际图像横向偏移量 */
    UINT32 u32TopOffset;    /* 从大的缓存区构建一帧videoFrame图像帧时，实际图像纵向偏移量 */
    UINT32 uiDataWidth;
    UINT32 uiDataHeight;
    UINT32 uiDataLen;       /* uiDataAddr 所指向的图像帧长度 */
    PhysAddr uiAppData;     /* 平台图像帧信息结构体的地址 */
} SYSTEM_FRAME_INFO;

/**
 * @function    sys_hal_allocVideoFrameInfoSt
 * @brief       硬件平台视频帧信息结构体的分配
 * @param[in]   pstSystemFrameInfo，上层统一使用该平台无关的视频信息结构体
 * @param[out]
 * @return
 */
INT32 sys_hal_allocVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo);

/**
 * @function    sys_hal_rleaseVideoFrameInfoSt
 * @brief       硬件平台视频帧信息结构体的释放
 * @param[in]   pstSystemFrameInfo，上层统一使用该平台无关的视频信息结构体
 * @param[out]
 * @return
 */
INT32 sys_hal_rleaseVideoFrameInfoSt(SYSTEM_FRAME_INFO *pstSystemFrameInfo);

/**
 * @function    sys_hal_buildVideoFrame
 * @brief       构建硬件平台视频帧信息
 * @param[in]   pVideoFrame，用于构建视频帧信息的参数
 *               pstSystemFrameInfo，上层统一使用该平台无关的视频信息结构体
 * @param[out]
 * @return
 */
INT32 sys_hal_buildVideoFrame(SAL_VideoFrameBuf *pVideoFrame, SYSTEM_FRAME_INFO *pstSystemFrameInfo);

/**
 * @function    sys_hal_getVideoFrameInfo
 * @brief       获取视频帧信息
 * @param[in]    pstSystemFrameInfo，上层统一使用该平台无关的视频信息结构体
 * @param[out]   pVideoFrame，用于构建视频帧信息的参数
 * @return
 */
INT32 sys_hal_getVideoFrameInfo(SYSTEM_FRAME_INFO *pstSystemFrameInfo, SAL_VideoFrameBuf *pVideoFrame);

/**
 * @function    sys_hal_getMBbyVirAddr
 * @brief       获取虚拟地址的MB
 * @param[in]   *viraddr，需要获取MB的虚拟地址
 * @param[out]   NONE
 * @return
 */
UINT64 sys_hal_getMBbyVirAddr(void *viraddr);

/**
 * @function    sys_hal_SDKbind
 * @brief       芯片平台SDK支持的绑定接口的封装
 * @param[in]   uiSrcModId  源模块ID
 *              s32SrcDevId 源设备ID
 *              s32SrcChn   源通道ID
 *              uiDstModId  目的模块ID
 *              s32DstDevId 目的设备ID
 *              s32DstChn   目的通道ID
 * @param[out]
 * @return
 */
INT32 sys_hal_SDKbind(SYSTEM_MOD_ID_E uiSrcModId, UINT32 s32SrcDevId, UINT32 s32SrcChn, SYSTEM_MOD_ID_E uiDstModId, UINT32 s32DstDevId, UINT32 s32DstChn);

/**
 * @function    sys_hal_SDKunbind
 * @brief       芯片平台SDK支持的解绑接口的封装
 * @param[in]   uiSrcModId  源模块ID
 *              s32SrcDevId 源设备ID
 *              s32SrcChn   源通道ID
 *              uiDstModId  目的模块ID
 *              s32DstDevId 目的设备ID
 *              s32DstChn   目的通道ID
 * @param[out]
 * @return
 */
INT32 sys_hal_SDKunbind(SYSTEM_MOD_ID_E uiSrcModId, UINT32 s32SrcDevId, UINT32 s32SrcChn, SYSTEM_MOD_ID_E uiDstModId, UINT32 s32DstDevId, UINT32 s32DstChn);

/**
 * @function	sys_hal_GetSysCurPts
 * @brief		系统获取平台pts
 * @param[in]	pCurPts 获取pts参数
 * @param[out]
 * @return
 */
INT32 sys_hal_GetSysCurPts(UINT64 *pCurPts);

/**
 * @function	sys_hal_setVideoTimeInfo
 * @brief		设置视频帧时间信息
 * @param[in]	pstSystemFrameInfo 视频帧指针
 * @param[in]	u32TimeRef 参考时间参数
 * @param[in]	u64Pts pts参数
 * @param[out]
 * @return
 */
INT32 sys_hal_setVideoTimeInfo(SYSTEM_FRAME_INFO *pstSystemFrameInfo, UINT32 u32TimeRef, UINT64 u64Pts);

/**
 * @function    sys_hal_init
 * @brief       系统控制初始化，主要是公共vb初始化，平台MPP初始化
 * @param[in]   u32ViChnNum，VI输入的路数
 * @param[out]
 * @return
 */
INT32 sys_hal_initMpp(UINT32 u32ViChnNum);


/**
 * @function    sys_hal_deInit
 * @brief       系统控制去初始化。
 * @param[in]
 * @param[out]
 * @return
 */
INT32 sys_hal_deInit(void);

/**
 * @function    sys_hal_init
 * @brief       系统控制初始化
 * @param[in]   
 * @param[out]
 * @return
 */
INT32 sys_hal_init(void);


#endif /* _SYS_HAL_H_ */


