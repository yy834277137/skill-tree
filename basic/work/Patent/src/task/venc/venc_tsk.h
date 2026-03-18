/**
 * @file   venc_tsk.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  VENC 模块 tsk 层
 * @author liuyun10
 * @date   2018年12月16日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月16日 Create
     Author      : liuyun10
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#ifndef _VENC_TSK_H_
#define _VENC_TSK_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <sal.h>
#include "stream_bits_info_def.h"
#include "system_plat_common.h"
#include "venc_tsk_api.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#define VENC_MAX_BUFFER	(1)

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
typedef struct tagVencPakcInfost
{
    UINT32 isUsePs;
    UINT32 isUseRtp;
    UINT32 u32PsHandle;
    UINT32 u32RtpHandle;
    UINT32 u32PackBufSize;
    UINT8 *pPackOutputBuf;
} VENC_PACK_INFO_ST;

/* 视频编码通道信息 */
typedef struct tagVencChnInfost
{
    UINT32 isStart;      /* 通道是否已经开启 */
    UINT32 isAdd;        /* 该通道是否已经创建  */
    UINT32 uiStart;      /* 就否开始编码         */
    UINT32 taskRun;      /* 线程运行中 */
    UINT32 needWait;     /* 线程是否需要等待 */
    UINT32 sendFrmTaskRun;      /* 线程运行中 */
//    UINT32 needWait;     /* 线程是否需要等待 */

    
    UINT32 u32Dev;       /* 该通道对应的设备号*/
    UINT32 u32StreamId;  /* 该通道对应的码流id, 0:main,1:sub,2:third */
    UINT32 u32HalChan;   /*hal层通道号OSD叠加*/

    VS_STANDARD_E enVsStandard;          /* 视频制式 */
    STREAM_TYPE enStreamType;            /* 码流类型 */
    UINT32 u32PackType;                  /* 打包类型 */
    UINT32 resolution;
    UINT32 u32EncWidth;
    UINT32 u32EncHeight;
	UINT32 u32EncodeType;

    void *pVencDrvHandle;   /* 对应drv层编码器handle */

    void *pDupHandle;    /* Dup handle */
    void *mutexHandle;
    void *sendFrmMutexHandle;
    SAL_ThrHndl hndl;     /* 编码线程   */
    SAL_ThrHndl hndle2;   /* 送帧线程   */

    VENC_PACK_INFO_ST stVencPakcInfo;
    SYSTEM_BITS_DATA_INFO_ST stStreamFrameInfo[VENC_MAX_BUFFER];  /* 缓存区 */
} VENC_CHN_INFO_S;


/* 每个编码组的信息 */
typedef struct tagVencDevInfost
{
    UINT32 u32Group;                            /* 编码组号 */
    VENC_CHN_INFO_S stVencChnInfo[VENC_DEV_CHN_NUM];  /* 编码组通道信息，分主、子码流 */
} VENC_DEV_INFO_S;


/* Venc 模块信息 */
typedef struct
{
    UINT32 u32DevNum;
    UINT32 u32Standard;
    UINT32 u32MainPackType;
    UINT32 u32SubPackType;
    UINT32 u32ThirdPackType;
    VENC_DEV_INFO_S stVencDevInfo[VENC_DEV_NUM];   /* 设备参数，每组对应多个通道 */
} VENC_TSK_CTRL_S;


/* 初始化的默认参数 */
typedef enum tagVencDefaultPrmEn
{
    VENC_DEF_WIDTH = 1920,     /* 编码宽 */
    VENC_DEF_HEIGHT = 1080,    /* 编码宽 */
    VENC_DEF_QUALITY = 90,     /* 图像质量 */
    VENC_DEF_BPS = 4096,       /* 码率 */
    VENC_DEF_BPSTYPE = 1       /* 码率控制模式(1:CBR 0:VBR) */
} VENC_DEFAULT_PRM_E;


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _VENC_TSK_H_ */



