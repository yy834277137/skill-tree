/**
 * @file   system_prm_api.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  DSP 模块 系统参数管理模块头文件
 * @author dsp
 * @date   2022/5/7
 * @note
 * @note \n History
   1.日    期: 2022/5/7
     作    者: dsp
     修改历史: 创建文件
 */

#ifndef _SYSTEM_PRM_H_
#define _SYSTEM_PRM_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"
#include "common_boardtype.h"


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

#define VENC_CHN_MAX_NUM (3)       /* 每路采集通道最大编码通道数 */


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* 录像流信息结构体 (ps) */
typedef struct tagRecStreamInfoSt
{
    PUINT8 pucAddr;        /* 码流的地址 */
    UINT32 uiSize;         /* 码流的大小 */
    UINT32 uiType;         /* 码流的类型，0:音频 1:视频 */
    UINT32 streamType;     /* 帧类型，指视频 1: I帧 0: P帧 */
    DATE_TIME absTime;     /* 绝对时标 */
    UINT32 stdTime;        /* 相对时标 */
    INT64L pts;            /* 微秒时标 */
    UINT32 IFrameInfo;
} REC_STREAM_INFO_ST;


/* 网传流信息结构体 (RTP) */
typedef struct tagNetStreamInfoSt
{
    PUINT8 pucAddr;     /* 码流的地址 */
    UINT32 uiSize;      /* 码流的大小 */
    UINT32 uiType;      /* 码流的类型，0:音频 1:视频 */
} NET_STREAM_INFO_ST;

typedef struct tagSysPrmChnInfost
{
    void *MutexHandle;
} SYS_PRM_CHN_INFO;

typedef struct tagSysPrmDevInfost
{
    SYS_PRM_CHN_INFO stSysPrmChnInfo[VENC_CHN_MAX_NUM];
} SYS_PRM_DEV_INFO;


typedef struct tagSysPrmCtrlst
{
    UINT32 DevNum;
    SYS_PRM_DEV_INFO stSysPrmDevInfo[MAX_VENC_CHAN];

} SYS_PRM_CTRL;

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/**
 * @function   SystemPrm_getNoSignalInfo
 * @brief      获取无视频信号信息
 * @param[in]  CAPT_NOSIGNAL_INFO_ST *pstNoSignalInfo
 * @param[out] None
 * @return     INT32 HIK_SOK  : 成功
 *                   HIK_FAIL : 失败
 */
INT32 SystemPrm_getNoSignalInfo(CAPT_NOSIGNAL_INFO_ST *pstNoSignalInfo);

/**
 * @function   SystemPrm_getSysVideoFormat
 * @brief      获取系统视频制式
 * @param[in]  void
 * @param[out] None
 * @return     INT32 SAL_SOK  表示P制
                     SAL_FAIL 表示N制
 */
INT32 SystemPrm_getSysVideoFormat(void);

/**
 * @function   SystemPrm_cbFunProc
 * @brief      回调处理
 * @param[in]  STREAM_ELEMENT *pEle  信息头
 * @param[in]  unsigned char *buf    缓冲
 * @param[in]  unsigned int bufLen   缓冲长度
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_cbFunProc(STREAM_ELEMENT *pEle, unsigned char *buf, unsigned int bufLen);

/**
 * @function   SystemPrm_readTalkBackPool
 * @brief      读取语音对讲数据
 * @param[in]  UINT32 uiChn     通道
 * @param[in]  PUINT8 pucAddr   地址
 * @param[in]  UINT32 *puiSize  大小
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_readTalkBackPool(UINT32 uiChn, PUINT8 pucAddr, UINT32 *puiSize);

/**
 * @function   SystemPrm_writeTalkBackPool
 * @brief      写入语音对讲数据
 * @param[in]  UINT32 uiChn    通道
 * @param[in]  PUINT8 pucAddr  地址
 * @param[in]  UINT32 uiSize   大小
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeTalkBackPool(UINT32 uiChn, PUINT8 pucAddr, UINT32 uiSize);

/**
 * @function   SystemPrm_readDecSharedPool
 * @brief      读取待解码数据
 * @param[in]  UINT32 uiChn     通道
 * @param[in]  UINT8 **pucAddr  地址
 * @param[in]  UINT32 *puiSize  大小
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_readDecSharedPool(UINT32 uiChn, UINT8 **pucAddr, UINT32 *puiSize);

/**
 * @function   SystemPrm_getDecShareBuf
 * @brief      获取解码缓冲控制结构体
 * @param[in]  UINT32 uiChn               通道
 * @param[in]  DEC_SHARE_BUF **pDecShare  返回的解码缓冲控制信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_getDecShareBuf(UINT32 uiChn, DEC_SHARE_BUF **pDecShare);

/**
 * @function   SystemPrm_getStreamType
 * @brief      获取码流类型
 * @param[in]  UINT32 uiChn  通道
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_getStreamType(UINT32 uiChn);

/**
 * @function   SystemPrm_writeDecSharedPool
 * @brief      写解码缓冲
 * @param[in]  UINT32 uiChn    通道
 * @param[in]  UINT8 *pucAddr  地址
 * @param[in]  UINT32 uiSize   大小
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeDecSharedPool(UINT32 uiChn, UINT8 *pucAddr, UINT32 uiSize);

/**
 * @function   SystemPrm_writeRecPool
 * @brief      写入录像内存
 * @param[in]  UINT32 chan                        通道
 * @param[in]  INT32 streamId                     流ID
 * @param[in]  REC_STREAM_INFO_ST *pstStreamInfo  流信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeRecPool(UINT32 chan, INT32 streamId, REC_STREAM_INFO_ST *pstStreamInfo);

/**
 * @function   SystemPrm_writeRecPoolByRecode
 * @brief      写入录像内存（转存）
 * @param[in]  UINT32 chan                        通道
 * @param[in]  REC_STREAM_INFO_ST *pstStreamInfo  流信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeRecPoolByRecode(UINT32 chan, REC_STREAM_INFO_ST *pstStreamInfo);

/**
 * @function   SystemPrm_resetRecPool
 * @brief      复位录像缓存池，主要是将读写指针置0
 * @param[in]  UINT32 chan                        通道
 * @param[in]  INT32 streamId                     流ID
 * @param[in]  REC_STREAM_INFO_ST *pstStreamInfo  流信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_resetRecPool(UINT32 chan, INT32 streamId, REC_STREAM_INFO_ST *pstStreamInfo);

/**
 * @function   SystemPrm_writeToNetPool
 * @brief      写入网传内存
 * @param[in]  UINT32 chan                        通道
 * @param[in]  INT32 streamId                     流ID
 * @param[in]  NET_STREAM_INFO_ST *pstStreamInfo  流信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeToNetPool(UINT32 chan, INT32 streamId, NET_STREAM_INFO_ST *pstStreamInfo);

/**
 * @function   SystemPrm_writeToNetPoolByRecode
 * @brief      写入网传内存
 * @param[in]  UINT32 chan                        通道
 * @param[in]  NET_STREAM_INFO_ST *pstStreamInfo  流信息
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_writeToNetPoolByRecode(UINT32 chan, NET_STREAM_INFO_ST *pstStreamInfo);

/**
 * @function   SystemPrm_Init
 * @brief      系统参数初始化
 * @param[in]  void
 * @param[out] None
 * @return     INT32
 */
INT32 SystemPrm_Init(void);

/**
 * @function   SystemPrm_getDspInitPara
 * @brief      获取全局结构体指针
 * @param[in]  void
 * @param[out] None
 * @return     DSPINITPARA *
 */
DSPINITPARA *SystemPrm_getDspInitPara(void);

/**
 * @function   InitDspPara
 * @brief      初始化共享信息
 * @param[in]  DSPINITPARA **ppDspInitPara  全局指针地址
 * @param[in]  DATACALLBACK pFunc           回调函数
 * @param[out] None
 * @return     INT32
 */
INT32 InitDspPara(DSPINITPARA **ppDspInitPara, DATACALLBACK pFunc);

#endif

