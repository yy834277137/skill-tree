/*******************************************************************************
* recode_tsk.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wutao <wutao19@hikvision.com.cn>
* Version: V1.0.0  2019年6月28日 Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef _RECODE_TSK_H_
#define _RECODE_TSK_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sal.h"

#include "dspcommon.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define RECODE_MAX_CHN (MAX_VIPC_CHAN)
/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
typedef struct ivs_hdr_s
{
    unsigned short data_type;
    unsigned char marker_bit_0 : 1;
    unsigned char def_version : 7;
    unsigned short frame_num_high;
    unsigned char marker_bit_1 : 1;
    unsigned char reserved : 7;
    unsigned short frame_num_low;
} __attribute__((packed, aligned(1)))ivs_hdr_t;


/*此路待解码码流的属性*/
typedef struct tagRecodeChnPrm
{
    INT32 packType;       /* 原始封装方式, RTP  PS  TS*/
    INT32 encType;        /* 编码方式， H264 , MJPEG, MPEG4 */
    INT32 maxWidth;       /* 码流最大宽*/
    INT32 maxHeight;      /* 码流最大高*/
    INT32 dstPackType;    /* 封装方式, RTP  PS  TS*/
    INT32 audPackPs;        /* 音频转ps*/
    INT32 audPackRtp;       /* 音频转rtp*/
    UINT32 audioChannels;      /*音频通道数,0 mono,1 stero*/
    UINT32 audioBitsPerSample; /*8/16*/
    UINT32 audioSamplesPerSec; /*sample rate */
    UINT8 *ipcBufAddr;
    UINT32 ipcBufLen;
} RECODE_CHN_PRM;

typedef struct tagRecodePakcInfost
{
    UINT32 UsePs;
    UINT32 UseRtp;
    UINT32 PsHandle;
    UINT32 RtpHandle;
    UINT32 packBufSize;
    UINT8 *packOutputBuf;
    UINT32 packTmpBufSize;
    UINT8 *packTmpBuf;
} RECODE_PACK_INFO;

typedef struct
{
    UINT32 bFirstVideoPts;
    UINT32 lastVRtpTime;
    UINT32 lastTmpVRtpTime;
    UINT32 lastVTime;
    UINT32 compTime;
    UINT32 timeDist;
    UINT32 timeOffset;
    UINT32 timeStamp;
    UINT64 u64Vpts;
    UINT64 u64Pts_v;
} RECODE_PACK_TIME;


typedef struct _RECODE_MOD_CTRL_
{
    /*解码模块起停控制*/
    UINT8 enable;
    VOID *mutexHdl;

    /*解包模块控制句柄*/
    INT32 rtpHandle;          /*rtp解包handle*/
    INT32 psHandle;           /*ps解包handle*/
    INT32 chnHandle;          /*当前handle*/
} RECODE_MOD_CTRL;

/* capt Chn 属性 */
typedef struct tagRecodeObjPrm
{
    UINT32 uiChn;
    SAL_ThrHndl stThrHandl;         /* 线程句柄 */
    RECODE_CHN_PRM recodeChn;
    RECODE_MOD_CTRL recodeModCtrl;
    RECODE_PACK_INFO recodePack;
    RECODE_PACK_TIME recodeTime;
    RECODE_PACK_TIME recodeAudTime;
} RECODE_OBJ_PRM;

/* Capt obj 管理结构体 */
typedef struct tagRecodeObjCOMMON
{
    UINT32 uiObjCnt;                                         /* 已创建个数 */
    RECODE_OBJ_PRM recodeStObj[RECODE_MAX_CHN];
} RECODE_OBJ_COMMON;

/*******************************************************************************
* 函数名  : Host_DecStart
* 描  述  :
* 输  入  : - chan  :
*         : - pParam:
*         : - pBuf  :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxStart(UINT32 chan, RECODE_PRM *pRecodePrm);

/*******************************************************************************
* 函数名  : Host_DecStop
* 描  述  :
* 输  入  : - chan  :
*         : - pParam:
*         : - pBuf  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxStop(UINT32 chan);


/*******************************************************************************
* 函数名  : recode_tskDemuxSet
* 描  述  : 设置解包参数
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxSet(UINT32 chan, RECODE_PRM *pRecodePrm);


/*******************************************************************************
* 函数名  : Vdec_moduleCreate
* 描  述  :
* 输  入  : - pCreate:
*         : - pHandle:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskModuleCreate(void);

/*******************************************************************************
* 函数名  : recode_tskDemuxAudStart
* 描  述  : 开启音频转封装
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskDemuxAudStart(UINT32 chan, RECODE_PRM *pRecodePrm);

/*******************************************************************************
* 函数名  : recode_tskStart
* 描  述  :
* 输  入  : - chan  :
*         : - pParam:
*         : - pBuf  :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 recode_tskStart(UINT32 chan, UINT32 *pParam, UINT32 *pBuf);



/*******************************************************************************
* 函数名  : recode_tskStop
* 描  述  :
* 输  入  : - chan  :
*         : - pParam:
*         : - pBuf  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 recode_tskStop(UINT32 chan, UINT32 *pParam, UINT32 *pBuf);



/*******************************************************************************
* 函数名  : recode_tskSetPrm
* 描  述  :
* 输  入  : - chan  :
*         : - pParam:
*         : - pBuf  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 recode_tskSetPrm(UINT32 chan, UINT32 *pParam, UINT32 *pBuf);

/*******************************************************************************
* 函数名  : recode_tskSetAudPack
* 描  述  : 设置音频转包开始、关闭
* 输  入  : - chan  :
*         : - pParam:
*         : - pBuf  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 recode_tskSetAudPack(UINT32 chan, UINT32 *pParam, UINT32 *pBuf);

#ifdef __cplusplus
}
#endif

#endif /* _RECODE_DEV_H_ */

