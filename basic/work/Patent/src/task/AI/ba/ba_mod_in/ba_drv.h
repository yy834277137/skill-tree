/*******************************************************************************
* ba_drv.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2019年9月5日 Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef _BA_DRV_H_
#define _BA_DRV_H_


/* ========================================================================== */
/*                          头文件区									   */
/* ========================================================================== */
#include "sal.h"
#include "system_common_api.h"
#include <dspcommon.h>
#include <platform_hal.h>
#include "jdec_soft.h"
#include "jpeg_drv.h"
#include "ba_hal.h"
#include <platform_hal.h>


/* ========================================================================== */
/*                          宏定义区									   */
/* ========================================================================== */
#define BA_MAX_PRM_BUF_NUM		(10)    /* 配置参数缓存个数 */
#define BA_MAX_YUV_BUF_NUM		(10) /* (5)     / * YUV队列缓存个数 * / */
#define BA_INPUT_DATA_BUF_NUM	(10)

#define BA_CHN_WIDTH	(1280) /* (1280) */
#define BA_CHN_HEIGHT	(720) /* (720) */

#define BA_INPUT_HEIGHT (1.0f)            /* 处理范围高度（设置来屏蔽水印、操作界面录像等） */
#define BA_INPUT_WIDTH	(1.0f)            /* 处理范围宽度（设置来屏蔽水印、操作界面录像等） */
#define BA_INPUT_Y		(0.0f)            /* 处理范围左上角Y坐标（设置来屏蔽水印、操作界面录像等） */
#define BA_INPUT_X		(0.0f)            /* 处理范围左上角X坐标（设置来屏蔽水印、操作界面录像等） */

#define BA_DEFAULT_ALARM_CNT (10)          /* 默认触发告警的异常行为出现次数 */

#define BA_VPSS_CHN_ID (3)                 /* 行为分析获取数据的VPSS通道号 */

#define BA_MAX_DEC_YUV_SIZE (BA_CHN_WIDTH * BA_CHN_HEIGHT * 3 / 2)
#define BA_JPEG_SIZE		(BA_CHN_WIDTH * BA_CHN_HEIGHT * 4)

/* ========================================================================== */
/*                          数据结构区									   */
/* ========================================================================== */
typedef enum tagBaQueType
{
    SINGLE_QUEUE_TYPE = 0,                  /* 单队列模型 */
    DOUBLE_QUEUE_TYPE,                      /* 双队列模型 */
    MAX_QUEUE_TYPE,
} BA_QUE_TYPE_E;

typedef enum tagBaDutyType
{
    BA_DUTY_LEAVE_STATE = 0,              /*行为分析离岗状态*/
    BA_DUTY_ARRIVE_STATE,                 /*行为分析在岗状态*/
    BA_DUTY_NUM,
} BA_DUTY_TYPE_E;

typedef enum _ABNOR_BEHA_NUM_E_
{
    ABNOR_BEHAV_LEAVE_TYPE = 0,          /* 离岗类型 */
    ABNOR_BEHAV_EXIST_TYPE,              /* 在岗类型 */
    ABNOR_BEHAV_COVER_TYPE,              /* 遮挡类型 */
    ABNOR_BEHAV_DHQ_TYPE,                /* 打哈欠 */
    ABNOR_BEHAV_NLF_TYPE,                /* 未直视前方 */
    ABNOR_BEHAV_CHAT_TYPE,               /* 侧头聊天 */
    ABNOR_BEHAV_TIRED_TYPE,              /* 眯眼疲劳 */
    ABNOR_BEHAV_SMOKE_TYPE,              /*抽烟报警*/
    ABNOR_BEHAV_PHONE_TYPE,              /*打电话报警*/
    ABNOR_BEHAV_TYPE_NUM,                /* 当前开放7种行为检测，离岗、在岗、遮挡、打哈欠、未直视前方、侧头聊天、眯眼疲劳 */
} ABNOR_BEHAV_NUM_E;

typedef enum  _ABNOR_BEHA_TYEP_E_
{
    BAHAV_EXIST = 0,                    /*在岗状态大类*/
    BAHAV_DISAPPERA,                    /*离岗状态大类*/
} ABNOR_BEHAAV_TYPE_E;

typedef struct _ABNOR_BEHAV_CNT_PRM_
{
    UINT32 uiUpFlag;                                   /* 更新标志 */
    UINT32 uiAbnorBehavCnt;                            /* 异常行为检测次数，超过即告警并清零重新计数 */
} BA_ABNOR_BEHAV_CNT_PRM;

/* 行为检测功能类参数*/
typedef struct tagBaProcessIn
{
    /* 算法需要每帧更新的参数 */
    UINT32 uiCapt;                                     /* 是否开启编图 */
    UINT32 uiSensity;                                  /* 灵敏度，对应检测持续时间 */
    BA_ABNOR_BEHAV_CNT_PRM stAbnorBehavCntPrm;         /* 异常行为报警计数 */
    SVA_RECT_F rect;                                   /* 检测框 */
} BA_PROCESS_IN;

typedef struct tagBaQuePrm
{
    BA_QUE_TYPE_E enType;             /* 队列模型 */

    UINT32 uiBufCnt;                  /* Buf个数 */
    UINT32 uiBufSize;                 /* 单个Buf大小 */
    DSA_QueHndl *pstFullQue;          /* 数据满队列 */
    DSA_QueHndl *pstEmptQue;          /* 数据空队列 */
} BA_QUEUE_PRM;

typedef struct tagBaResultSt
{
    SBAE_OUTPUT_T stSbaRsltInfo;           /* 行为分析引擎检测结果 */
    UINT32 uiReserved[4];
} BA_OUTPUT_RESULT_S;

/* jpeg编码队列数据格式 */
typedef struct tagBaJpegInfoSt
{
    SYSTEM_FRAME_INFO stFrame;      /* 帧数据 */
    BA_OUTPUT_RESULT_S stBaRslt;    /* 行为分析检测结果 */
    UINT32 uiReserved[4];           /* 保留位 */
} BA_FRAME_INFO_ST;

typedef struct tagBaYuvQuePrmSt
{
    BA_QUEUE_PRM *pstYuvQuePrm;
    BA_FRAME_INFO_ST *pstYuvFrame[BA_MAX_YUV_BUF_NUM];
} BA_YUV_QUE_PRM;

typedef struct tagBaCfgQuePrmSt
{
    BA_QUEUE_PRM *pstCfgQuePrm;
    BA_PROCESS_IN *pstIn[BA_MAX_PRM_BUF_NUM];
} BA_CFG_QUE_PRM;

/* jpeg编码信息 */
typedef struct tagBaJpegVencPrmSt
{
    INT8 *pPicJpeg;                   /* jpeg文件地址 */

    UINT8 bDecChnStatus;               /* 编码通道是否已创建 */

    UINT32 uiJpegEncChn;                /* 编码通道 */

    UINT32 uiPicJpegSize;             /* jpeg大小 */

    JPEG_DRV_CHN_INFO_S *pstJpegChnInfo;   /* jpeg通道信息 */
} BA_JPEG_VENC_PRM;

typedef struct tagBaInputPrivDataSt
{
    UINT32 uiChn;                            /* 通道号 */
    UINT32 uiInputIdx;                       /* 索引值 */
    UINT32 uiInputTime;                      /* 输入时间戳，单位ms */
    UINT32 uiRervered[2];
} BA_INPUT_PRVT_DATA_S;

typedef struct tagBaInputDataSt
{
    UINT32 uiUseFlag;                         /* 使用状态 */

    UINT32 uiChn;                             /* 通道号 */
    UINT32 uiInputId;                         /* buf索引ID */
    ICF_INPUT_DATA stIcfInputData;            /* 输入数据信息 */
    BA_INPUT_PRVT_DATA_S stIptPrvtData;       /* 输入私有信息 */
} BA_INPUT_DATA_S;

typedef struct tagBaInputPrmSt
{
    UINT32 uiCnt;                                            /* 初始化的input data个数，最大BA_INPUT_DATA_BUF_NUM */
    BA_INPUT_DATA_S *pstBaInputData[BA_INPUT_DATA_BUF_NUM];        /* 输入数据信息，动态申请内存 */
} BA_INPUT_PRM_S;

typedef struct tagBaLastSts
{
    UINT32 uiLastFrmNum[ABNOR_BEHAV_TYPE_NUM];   /* 上次异常行为的帧号 */
    UINT32 auiLastAltsts[ABNOR_BEHAV_TYPE_NUM];  /* 上次异常行为分析状态 */
} BA_LAST_STS_S;

/* 安检行为分析算法模块参数 */
typedef struct tagBAPrmSt
{
    UINT32 chan;

    /* UINT32            uiCapt;				 / * 是否开启编图，暂未启用 * / */

    UINT32 uiVdecChn;                            /* 解码通道 */

    UINT32 uiChnStatus;                          /* 通道创建状态 */

    UINT32 uiRlsFlag;                            /* 处理线程是否休眠，用于资源退出判断 */

    UINT32 uiFrameNum;                           /* 帧序号 */

    BA_LAST_STS_S stLastStsInfo;                 /* 用于保存上一次检测告警的相关参数，用于去重 */

    BA_INPUT_PRM_S stBaInputPrm;                 /* 行为分析输入数据缓存参数 */

    BA_JPEG_VENC_PRM stJpegVenc;                 /* 包裹帧编码成JPEG的信息 */

    BA_YUV_QUE_PRM stYuvQueInfo;                 /* 行为分析YUV存放队列信息 */

    BA_CFG_QUE_PRM stCfgQueInfo;                 /* 行为分析YUV存放队列信息 */

    BA_PROCESS_IN stIn;                          /* 算法需要的参数 */

    SYSTEM_FRAME_INFO stDecFrameInfo;            /* 用于拷贝解码数据 */

    SYSTEM_FRAME_INFO stRltFrameInfo;            /* 用于拷贝结果数据 */

/*    SAL_ThrHndl stRltThrHandl;                   / * 结果处理句柄 * / */

    SAL_ThrHndl stProcThrHandl;                  /* 算法处理句柄 */

    SAL_ThrHndl stFrmThrHandl;                   /* 帧获取句柄 */

    void *mChnMutexHdl;                          /* 通道信号量*/
} BA_DEV_PRM;

/* 安检算法行为分析模块全局参数 */
typedef struct tagBaCommonSt
{
    UINT32 uiPublic;                            /* 全局参数初始化状态 */
    UINT32 uiDevCnt;                            /* 算法通道个数 */
    BA_DEV_PRM stBa_dev[4];                     /* 算法通道，行为分析预留四个通道 */
} BA_COMMON;

/* ========================================================================== */
/*                          函数定义区									   */
/* ========================================================================== */

/**
 * @function:   Ba_DrvGetVersion
 * @brief:      获取版本号信息
 * @param[in]:  UINT32 chan
 * @param[out]:  CHAR *pcVerInfo
 * @return:     INT32
 */
INT32 Ba_DrvGetVersion(IN UINT32 chan, OUT CHAR *pcVerInfo);

/**
 * @function:   Ba_DrvSetDetectLevel
 * @brief:      设置异常行为检测级别
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvSetDetectLevel(UINT32 chan, void *prm);

/**
 * @function:   Ba_DrvSetDetectRegion
 * @brief:      设置行为分析检测区域
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvSetDetectRegion(UINT32 chan, void *prm);

/**
 * @function:   Ba_DrvCaptSwitch
 * @brief:      通道抓拍参数（暂未开启）
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 uiSwitch
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvCaptSwitch(UINT32 chan, UINT32 uiSwitch);

/**
 * @function:   Ba_DrvSetSensity
 * @brief:      设置行为分析检测灵敏度
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvSetSensity(UINT32 chan, void *prm);

/**
 * @function:   Ba_DrvModuleInit
 * @brief:      行为分析通道初始化
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvModuleInit(UINT32 chan, void *prm);

/**
 * @function:   Ba_DrvModuleDeInit
 * @brief:      行为分析通道去初始化
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvModuleDeInit(UINT32 chan);

/**
 * @function:   Ba_DrvInit
 * @brief:      行为分析驱动层初始化
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ba_DrvInit(void);

#endif

