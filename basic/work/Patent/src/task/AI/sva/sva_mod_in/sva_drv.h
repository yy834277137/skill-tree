/*******************************************************************************
* sva_drv.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : huangshuxin <huangshuxin@hikvision.com>
* Version: V1.0.0  2019年2月14日 Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef __SVA_DRV_H__
#define __SVA_DRV_H__


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "sal.h"

#include <dspcommon.h>
#include <platform_hal.h>
#include "jdec_soft.h"
#include "jpeg_drv_api.h"
#include "sva_hal.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#define SVA_DEV_MAX (2)

#define SVA_MAX_PRM_BUFFER (100)          /* 智能参数使用的队列缓存 */

#define SVA_MAX_JPEG_FRM_BUF (10)         /* 智能抓图使用的YUV队列缓存 */

#define SVA_MAX_SLAVE_BUFFER_NUM (12)     /* 临时将接收主片发来的帧数据队列深度，设置为12，若不够进行增加 */

#define SVA_MAX_OUT_BUFFER (15)

#define SVA_JPEG_SIZE (SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2)

#define SVA_BMP_SIZE (SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 4)

#define SVA_MAX_DEC_YUV_SIZE (HIK_DIC_MAX_WIDTH * HIK_DIC_MAX_WIDTH * 3 / 2)

#define SVA_RESULT_NO_UPDATE (0)

#define SVA_RESULT_UPDATE (1)

/* #define SVA_OFFSET_BUF_LEN          (12) */

#define SVA_DRV_SYNC_FLG (0)                  /* TODO: use variable from app and distinguish */

#define SVA_DISP_LEFT_TO_RIGHT (1)

#define SVA_DISP_RIGHT_TO_LEFT (3)

#define SVA_DISP_LEFT_TO_RIGHT_MIRROR (2)

#define SVA_DISP_RIGHT_TO_LEFT_MIRROR (0)

#define SVA_DEFAULT_CONF_THRESH (0)                /* 默认的置信度阈值 */

#define SVA_DEFAULT_ALERT_TARGET_CNT (1)              /* 危险品默认告警个数阈值 */

#define SVA_ALG_PROC_AVE_MS (17)              /* 调试使用；算法处理平均耗时，单位ms */

#define SVA_MAX_MATCH_BUF_NUM (12)

#define SVA_MATCH_OFFSET_TAB_CNT (7)          /* 用于匹配失败计算偏移量的数据表个数 */

#define SVA_MAX_POST_PROC_QUE_LEN (6)            /* 智能抓图使用的YUV队列缓存 */

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
#if 0
typedef struct tagSvaOffsetPrm
{
    UINT32 uiTimeRef;                 /* 帧序号 */
    float fOffset;                    /* 帧间隔 */
    float fStdValue;                  /* 当前的偏移经验值，统计得出 */
} SVA_OFFSET_PRM_S;

typedef struct tagSvaOffsetBufTab
{
    UINT32 uiIdx;                                        /* index of current buf table */
    UINT32 uiMaxLen;                                     /* max length of buf table */
    SVA_OFFSET_PRM_S stOffsetPrm[SVA_OFFSET_BUF_LEN];    /* offset prm of buf table */
} SVA_OFFSET_BUF_TAB_S;

#endif
typedef enum _SVA_SYNC_TYPE_
{
    SVA_SYNC_TYPE_DISABLE = 0,                   /* 线程同步状态--关闭 */
    SVA_SYNC_TYPE_ENABLE,                        /* 线程同步状态--开启 */
    SVA_SYNC_TYPE_NUM,
} SVA_SYNC_TYPE_E;

typedef struct _SVA_SYNC_PRM_
{
    SVA_SYNC_TYPE_E enType;                         /* 同步类型 */

    INT32 iTimeOut;                                  /* 超时次数，>0 进行超时计算，-1表示阻塞等待 */
    UINT32 uiSingleWaitMs;                           /* 单次等待休眠时间，ms */
    UINT32 *puiMonitSts;                             /* 待监控的同步标志位 */
    VOID *pSyncMutex;                                /* 同步信号锁 */
} SVA_SYNC_PRM_S;

typedef struct tagGapData
{
    float gap;                            /* gap value */
    UINT32 cnt;                           /* cnt of value gap appears */
    UINT32 max_cnt;
} GAP_DATA_S;

typedef struct tagSvaUnifSpeedPrm
{
    UINT32 uiFlag;                        /* 判断是否已经成功获取标准值 */
    UINT32 uiCount;                       /* 当前Data Table中数据个数 */
    UINT32 uiLastTimeRef;                 /* 上一帧的帧参考序号 */
    float fStdValue;                      /* 匀速过包模式的偏移量标准值 */
    GAP_DATA_S stGapData[10];             /* 包含已出现的帧间隔数据: specific value & existing times */
} SVA_OFFSET_UNIFORM_S;

typedef struct tagSvaOffsetBuf
{
    UINT32 uiBufIdx;                      /* 循环Buf当前的索引值 */
    float fGapBuf[3];                     /* 缓存包括当前帧和之前两帧的Gap数据，用于包裹停止状态的间隔处理 */
} SVA_OFFSET_BUF_S;

typedef struct tagSvaOffsetStopModePrm
{
    float fStdValue;                      /* 若触发停止状态，则使用该标准值用来计算偏移量 */
    SVA_OFFSET_BUF_S stBufPrm;            /* 循环Buf相关参数 */
} SVA_OFFSET_STOP_S;

typedef struct tagSvaOffsetInfo
{
    SVA_OFFSET_UNIFORM_S stUniformPrm;    /* 匀速过包模式相关全局参数 */
} SVA_OFFSET_PRM;

/* jpeg编码队列数据格式 */
typedef struct tagSvaJpegInfoSt
{
    SYSTEM_FRAME_INFO stFrame;        /* 帧数据 */
    SVA_PROCESS_IN stIn;              /* 外部配置算法参数 */
    SVA_PROCESS_OUT szJpegOut;        /* 算法结果 */
} SVA_JPEG_INFO_ST;

/* 行为分析帧数据队列信息 */
typedef struct tagSvaYuvQuePrmSt
{
    DSA_QueHndl pstFullQue;            /* 数据满队列 */

    DSA_QueHndl pstEmptQue;            /* 数据空队列 */

    SVA_JPEG_INFO_ST stJpegFrame[SVA_MAX_JPEG_FRM_BUF];
} SVA_YUV_QUE_PRM;

/* jpeg编码信息 */
typedef struct tagSvaJpegVencPrmSt
{
    INT8 *pPicJpeg[SVA_MAX_CALLBACK_JPEG_NUM];                  /* jpeg文件地址 */

    INT8 *pPicBmp[SVA_MAX_CALLBACK_BMP_NUM];                    /* bmp文件地址 */

    UINT8 bDecChnStatus;                                        /* 编码通道是否已创建 */

    UINT32 uiJpegEncChn;                                        /* 编码通道 */

    UINT32 uiPicJpegSize[SVA_MAX_CALLBACK_JPEG_NUM];            /* jpeg大小 */

    UINT32 uiPicBmpSize[SVA_MAX_CALLBACK_BMP_NUM];              /* bmp大小 */

    SYSTEM_FRAME_INFO stTmpFrameBuf;                            /* tmp Buf, 使用cached mmz提高处理速度 */

    SYSTEM_FRAME_INFO stTmpJpegFrameBuf;                        /* tmp Buf, 使用cached mmz提高处理速度 */

    SYSTEM_FRAME_INFO stTmpOsdJpegFrameBuf;                     /* osd Buf, 用于存放需要叠osd的帧数据 */

    SYSTEM_FRAME_INFO stTmpOsdJpegFrameBufTmp;                     /* osd Buf, 用于存放需要叠osd的帧数据 */

    SYSTEM_FRAME_INFO stMirFrameBuf;                            /* Mirror Buf, 用于镜像翻转 */

    void *pstJpegChnInfo;                          /* jpeg通道信息 */

    DSA_QueHndl pstFullQue;                                     /* 数据满队列 */

    DSA_QueHndl pstEmptQue;                                     /* 数据空队列 */

    SVA_JPEG_INFO_ST stJpegFrame[SVA_MAX_JPEG_FRM_BUF];
} SVA_JPEG_VENC_PRM;

/* 数据分发模块信息 */
typedef struct tagSvaDupPrmSt
{
    UINT32 bMirror;

    VOID *pHandle;                   /* 数据分发句柄 */

    VOID *pVpHandle;                 /* 视频处理通道handle，当前用来进行镜像处理 */

    VOID *pVpScaleHandle;            /* 视频处理通道handle，当前用来自动化工具帧数据缩小 */
} SVA_DUP_PRM;

typedef struct tagSvaMatchBufData
{
    BOOL bDataValid;                            /* 标记是否存在有效帧数据缓存 */
    UINT32 uiFrameNum;                          /* 帧序号 */
    SYSTEM_FRAME_INFO stSysFrameInfo;           /* 帧数据相关参数 */
    void *pReserved;                            /* 预留 */
} SVA_MATCH_BUF_DATA;

typedef struct tagSvaMatchBufInfo
{
    UINT32 uiCnt;                                                /* 帧数据匹配缓存个数 */
    UINT32 uiW_Idx;                                              /* 写索引 */
    Handle pMutex;                                               /* 读写互斥锁 */
    SVA_MATCH_BUF_DATA stMatchBufData[SVA_MAX_MATCH_BUF_NUM];    /* 匹配缓存数据 */

    UINT32 uiOffsetW_Idx;                                        /* 当前偏移数据表的写索引 */
    float fPastOffset[SVA_MATCH_OFFSET_TAB_CNT];                 /* 记录过去7帧的offset用于匹配失败时进行偏移 */
} SVA_MATCH_BUF_INFO;

typedef struct tagSvaDbgPrm
{
    CHAR cPath[64];                                              /* dump文件路径 */
    FILE *pFile;                                                 /* dump文件描述符 */

    UINT32 uiFlag;                                               /* 参数初始化标志 */
    UINT32 uiDbgCnt;                                             /* dump的数据索引 */
    UINT32 uiDbgDumpNum;                                         /* dump的目标帧个数 */
} SVA_DBG_PRM;

typedef struct _SVA_PROC_INTER_PRM_
{
    UINT32 uiProcFlag;                           /* 送帧线程开启标志，V2使用 */

    UINT32 uiSyncFlag;                           /* 同步标志位 */

    void *mChnMutexHd1;                          /* 通道信号量 */
} SVA_PROC_INTER_PRM;

typedef struct tagSvaXtransPrm
{
    UINT32 uiToChipId;                          /* 跨芯片传输数据对端的chip id */

    XTRANS_BUSSINESS_DEV_S *pstBusiDevSendFrm;  /* 帧数据发送的business设备 */
    sem_t *pSem;                                /* 用于帧数据发送和接收线程，xtrans库内部注册通知业务层发送已结束 */

    XTRANS_BUSSINESS_DEV_S *pstBusiDevSendRslt; /* 智能结果发送的business设备 */
	
	/* 帧结果传输 */
	sem_t *pRsltSem;
} SVA_XTRANS_PRM_S;

typedef struct tagSvaPostProcQue
{
    UINT32 uiQueLen;
    DSA_QueHndl stFullQue;                      /* 数据满队列 */
    DSA_QueHndl stEmptQue;                      /* 数据空队列 */

    SVA_SYNC_RESULT_S *pstAlgOutRslt[SVA_MAX_POST_PROC_QUE_LEN];    /* 智能结果 */
} SVA_POST_PROC_QUE_S;

typedef enum _SVA_VALID_PKG_TYPE_
{
    SVA_VALID_PKG_TYPE_BUTT = 0,               /* 异常类型 */
    SVA_VALID_PKG_TYPE_SINGLE = 1,             /* 单个包裹类型 */
    SVA_VALID_PKG_TYPE_COMBINE = 2,            /* 融合包裹类型 */
} SVA_VALID_PKG_TYPE_E;

typedef struct _SVA_PKG_VALID_INFO_
{
    BOOL bDataValid;                            /* 数据是否有效 */
    SVA_VALID_PKG_TYPE_E enPkgValidType;        /* 包裹类型，目前有单个包裹和融合包裹两种 */
    UINT32 uiPkgLogicId;                        /* 融合包裹中有分包结果的最大id */
    UINT32 uiMaxSubPkgId;                       /* 融合包裹中最大子包裹id，包括未分包结束的 */
    float fPkgLogicFwd;                         /* 包裹逻辑前沿 */

	UINT32 uiR_Idx;                             /* 读索引个数 */
	UINT32 uiW_Idx;                             /* 写索引个数 */
    UINT32 uiPkgLogicCnt;                       /* 融合包裹源个数 */
    UINT32 auiPkgID[SVA_PACKAGE_MAX_NUM];       /* 包裹融合的包裹链ID */
    BOOL abOldPkg[SVA_PACKAGE_MAX_NUM];         /* 是否为历史包裹 */
    BOOL abPkgExist[SVA_PACKAGE_MAX_NUM];       /* 包裹是否已经消失 */
} SVA_PKG_VALID_INFO_S;

typedef struct _SVA_LAST_VALID_PKG_PRM_
{
    UINT32 uiMaxSubPkgId;                       /* 最大子包裹id */

    UINT32 uiPkgLogicCnt;                       /* 上一次valid使能的融合包裹中的源个数 */
    UINT32 auiPkgID[SVA_PACKAGE_MAX_NUM];       /* 上一次valid使能的包裹融合的包裹链ID */
} SVA_LAST_VALID_PKG_PRM_S;

typedef struct _SVA_PKG_COMB_PRM_
{
    UINT32 u32CurMaxDetId;                      /* 当前最大的det包裹id，用于规避融合包裹已经det valid，之后子包裹再出现det valid的问题 */

    UINT32 uiRIdx;
    UINT32 uiWIdx;
    SVA_LAST_VALID_PKG_PRM_S stLastValidPkgPrm[SVA_PACKAGE_MAX_NUM];    /* 画面中缓存的valid融合包裹(历史) */

    SVA_PROCESS_OUT stTmpOut;                                           /* 中间变量，用于将包裹融合信息同步到全局 */
    SVA_PKG_VALID_INFO_S stPkgValidInfo[SVA_PACKAGE_MAX_NUM];           /* 当前包裹融合的包裹信息 */
} SVA_PKG_COMB_PRM_S;

typedef struct _SVA_SPLIT_SLICE_BUF_PRM_
{
	/* 分片组的全局处理信息 */
	SAV_INS_ENC_PRM_S stInsEncPrm;
} SVA_SPLIT_SLICE_BUF_PRM_S;

typedef struct _SVA_POST_PROC_PKG_INFO_
{
	/* 是否有效 */
	BOOL bValid;
	/* 包裹是否已经完成检测 */
	BOOL bPkgDetValid;
    /* 新包裹出现标志位 */
    UINT32 u32PkgId;
	/* 分片组参数，集中判图使用 */
	SVA_SPLIT_SLICE_BUF_PRM_S stSliceBufPrm;
	/* 包裹新出现时的信息，包括前沿和绝对时间 */
    SVA_PERSION_PKG_MATCH_PRM_S stPkgStartInfo;
	/* 包裹分包完成时的信息，包括前沿和绝对时间 */
    SVA_PERSION_PKG_MATCH_PRM_S stPkgEndInfo;
} SVA_POST_PROC_PKG_INFO_S;

/* 安检算法模块通道参数 */
typedef struct tagSvaDevPrmSt
{
    UINT32 chan;

    UINT32 uiInitStatus;                        /* 算法参数初始化状态，如果为0，则表示失败，后面init算法通道返回错误 */

    UINT32 uiChnStatus;                         /* 通道创建状态 */

    UINT32 uiChnMirrorSync;                     /* 设置通道镜像触发同步标志 */

    SVA_MACHINE_TYPE enChnSize;                 /* 安检机通道大小，用来换算算法因子 */

    UINT32 uiSwitch;                            /* 应用层管理总开关，0-不回调智能结果，1-回调智能结果 */

	BOOL bSplitSerialStart;                     /* 分片序列是否开启的标志 */
	
    SVA_PKG_TAR_CNT_S stPkgTarCntInfo;          /* 包裹目标个数统计信息 */

    SVA_MATCH_BUF_INFO *pstMatchBufInfo;        /* 智能帧数据匹配缓存，用于双芯片主片设备上，其他型号无需关注 */

    SYSTEM_FRAME_INFO stSysFrame;               /* 帧数据，临时存放智能回调帧数据 */

    SYSTEM_FRAME_INFO stAtSysFrame[3];          /* 帧数据, 算法指标自动化测试使用 */

    SYSTEM_FRAME_INFO stGetPackSysFrame[SVA_DEV_MAX]; /*帧数据，获取其他模块输入的包裹帧数据*/

    SYSTEM_FRAME_INFO stPackCpyFrame[SVA_DEV_MAX]; /*帧数据，用于安检包裹数据cpy*/

    SVA_POST_PROC_QUE_S stPostProcQue;          /* 主片: 数据后处理队列参数 */

    SVA_JPEG_VENC_PRM stJpegVenc;               /* 包裹帧编码成JPEG的信息 */

    SVA_DUP_PRM DupHandle;                      /* 检测视频的通道handle */

    SVA_DBG_PRM stDbgPrm;                       /* 调试参数 */

	float f32PkgCombThr;                        /* 判断包裹融合的像素阈值，[-50,50]/1280 */

	SVA_FORCE_SPLIT_PRM_S stForceSplitPrm;      /* 强制分包参数 */

	UINT32 u32PkgCombFlag;                      /* 包裹融合开关 */
	
    SVA_PKG_COMB_PRM_S stPkgCombPrm;            /* 包裹融合全局参数 */

    SVA_PROCESS_IN stIn;                        /* 算法需要的参数，单个报警物开关也在里面 */

#if 1 /* 集中判图使用 */
    SAV_INS_ENC_PRM_S stInsEncPrm;              /* 即刻编图相关参数 */
	/* 分片包含目标个数 */
	UINT32 u32SliceTarNum;                        
    /* 分片包含目标信息，编图线程使用中间变量，从out中剥离出来，减少智能结果传输数据量 */
	SVA_TARGET stSliceTarget[SVA_XSI_MAX_ALARM_NUM];
#endif

	/* 包裹后处理缓存 */
	SVA_POST_PROC_PKG_INFO_S astPostProcPkgInfo[SVA_PACKAGE_MAX_NUM];

    SVA_PROCESS_OUT stFilterOut;                /* 逻辑链过滤中间结果 */

	SVA_TARGET stRealtarget[SVA_XSI_MAX_ALARM_NUM];   /* 局部大堆栈，编图线程使用 */

    SVA_PROCESS_OUT stTmpOut;                   /* 局部申请结构体太大，coverity会报错，所以搞一个全局的，6 不 6，耶 不 耶 */

    SVA_PROCESS_OUT stXpackRlst;                /* 保存获取到的最新算法结果给Xpack模块 */

    DSA_QueHndl stPrmFullQue;                   /* 设置参数数据满队列 */

    DSA_QueHndl stPrmEmptQue;                   /* 设置参数数据空队列 */
#if 1 /* 安检机使用 */
    DSA_QueHndl stInPackageDataQue;             /* 输入包裹数据队列*/

    DSA_QueHndl stOutPackageDataFullQue;        /* 输出包裹检测数据满队列*/

    DSA_QueHndl stOutPackageDataEmptQue;        /* 输出包裹检测数据空队列*/
#endif
    SAL_ThrHndl stJpegThrHandl;

    SAL_ThrHndl stSvaThrHandl;                  /* 主片-送帧线程句柄，从片-接收缓冲线程句柄 */

    SAL_ThrHndl stSvaThrHandl1;                 /* 主片-智能结果接收线程句柄，从片-送帧线程句柄 */

    SAL_ThrHndl stSvaPostProcThrHandl;          /* 主片-智能结果后处理线程 */

    SVA_PROC_INTER_PRM stProcInterPrm;                           /* 同步标志位 */

    void *mChnMutexHdl;                         /* 通道信号量 */

    void *mSyncMutexHdl;                        /* 同步信号锁 */
} SVA_DEV_PRM;

typedef struct tagSvaSlavePrm
{
    SYSTEM_FRAME_INFO stFrameInfo[SVA_MAX_SLAVE_BUFFER_NUM][SVA_DEV_MAX];   /* 从片接收数据的缓存队列 */

    UINT32 uiQueCreated;
    UINT32 uiQueDepth;
    DSA_QueHndl stFrameDataFullQue;              /* 设置参数数据满队列 */
    DSA_QueHndl stFrameDataEmptQue;              /* 设置参数数据空队列 */
} SVA_SLAVE_PRM;

typedef struct _SVA_ALG_RSLT_
{
    UINT32 uiChnCnt;
    SVA_SYNC_RESULT_S stAlgChnRslt[SVA_DEV_MAX];    /* 最多2个通道 */
} SVA_ALG_RSLT_S;

typedef struct _SVA_DBG_COMM_STS_
{
    UINT32 uiInputFrmRate;                          /* 模块输入帧率 */
    UINT32 uiInputCnt;                              /* 模块处理帧数 */
    UINT32 uiOutputCnt;                             /* 模块输出帧数 */
    UINT32 uiOutputCbEndCnt;                        /* 模块输出处理完成帧数 */
    UINT32 uiErrCnt;                                /* 处理异常帧数 */
    UINT32 uiJencCnt;                               /* 编图个数 */
    UINT32 uiJencErrCnt;                            /* 编图失败个数 */

    UINT32 uiDirection;                             /* 方向配置 */
    UINT32 uiOsdExtType;                            /* osd拓展字段类型 */
    UINT32 uiOsdScaleLevel;                         /* osd字体大小配置 */
    CHAR acRgnStr[256];                             /* 检测区域配置 */
    CHAR acAlertStr[SVA_MAX_ALARM_TYPE * 128];      /* 违禁品配置，id(1) + name(20) + conf(1) + color(4) + key(1) + conf(1) + 换行和空格(8) */

    /* 用来验证多芯片传输时接口是否卡住 */
    UINT32 uiSendFrameFlag;                         /* 发送帧数据接口是否卡住 */
    UINT32 uiSendFrameNum;
    UINT32 uiSendFrameSuccNum;
    UINT32 uiSendFrameFailNum;

    UINT32 uiRecvFrameFlag;                         /* 接收帧数据接口是否卡住 */
    UINT32 uiRecvFrameNum;
    UINT32 uiRecvFrameSuccNum;
    UINT32 uiRecvFrameFailNum;

    UINT32 uiSendIaRsltFlag;                        /* 发送智能结果接口是否卡住 */
    UINT32 uiSendIaRsltNum;
    UINT32 uiSendIaRsltSuccNum;
    UINT32 uiSendIaRsltFailNum;

    UINT32 uiRecvIaRsltFlag;                        /* 接收智能结果接口是否卡住 */
    UINT32 uiRecvIaRsltNum;
    UINT32 uiRecvIaRsltSuccNum;
    UINT32 uiRecvIaRsltFailNum;

    UINT32 uiInputEngDataFlag;                      /* 引擎送帧接口是否卡住 */
    UINT32 uiInputEngDataNum;
    UINT32 uiInputEngDataSuccNum;
    UINT32 uiInputEngDataFailNum;

    UINT32 uiJencMirrorFlag;                        /* 编图线程某处是否卡住 */
    UINT32 uiJencBmpFlag;
    UINT32 uiJencjpg1Flag;
    UINT32 uiJencjpg2Flag;
    UINT32 uiJencSplitModeFlag;
    UINT32 uiJencCbFlag;

    CHAR acPrvtStr[32];  /* 模块私有信息，预留 */
} SVA_DBG_COMM_STS_S;

typedef struct _SVA_DBG_ALG_STS_
{
    UINT32 uiProcMode;                              /* 算法处理模式 */

    UINT32 uiTotalAlgCostMs;                        /* 算法处理总体耗时 */
    UINT32 uiTotalAlgProcCnt;                       /* 算法处理总体帧数 */
    float fAveMs;                                   /* 算法处理单帧平均耗时 */
    UINT32 uiMinMs;                                 /* 算法处理单帧最小耗时 */
    UINT32 uiMaxMs;                                 /* 算法处理单帧最大耗时 */
    UINT32 uiInputCnt;                              /* 算法输入帧数 */
    UINT32 uiOutputCnt;                             /* 算法输出帧数 */
    UINT32 uiAlgErrCnt;                             /* 算法处理异常帧数 */

    CHAR acAlgVer[256];                             /* 算法版本号 */
    CHAR acModelVer[256];                           /* 模型版本号 */
    CHAR acMemInfo[256];                            /* 算法内存消耗信息 */

    UINT32 uiPdProcGap;                             /* 包裹分割检测处理间隔 */
    float fObdDownSmpScale;                         /* obd下采样率 */
    UINT32 uiPkgSensity;                            /* 包裹分割灵敏度 */
    float fZoomInOutThrs;                           /* 放大缩小阈值 */
    float fPackOverX2;                              /* 画面边缘认为包裹出全的阈值 */
    UINT32 uiPkSwitch;                              /* PK模式开关 */
    UINT32 uiColorFilterSwitch;                     /* 色彩过滤开关 */
    UINT32 uiSizeFilterSwitch;                      /* 尺寸过滤开关 */
} SVA_DBG_ALG_STS_S;

typedef struct _SVA_DBG_PRM_
{
    VOID *pHandle;                                /* 向sts维护模块注册后获得的通道句柄 */
    STS_MOD_NODE_INFO_S stRootNodeInfo;           /* 层级MAP，用于注册维护模块sts使用 */

    SVA_DBG_COMM_STS_S stCommSts;                 /* 模块通用维护状态结构体 */
    SVA_DBG_ALG_STS_S stAlgSts;                   /* 算法维护状态结构体 */

    /* 局部大堆栈 */
    SVA_PROCESS_IN stIn;
} SVA_DBG_PRM_S;

typedef struct _SVA_PKG_MATCH_COMM_PRM_
{
	/* 双视角包裹匹配索引，仅在关联模式使用，1开始累加 */
	UINT32 u32MatchIdx;
} SVA_PKG_MATCH_COMM_PRM_S;

/* 最大包裹匹配对个数 */
#define SVA_MAX_PKG_MATCH_PAIR_NUM (16)

typedef struct _SVA_PKG_MATCH_PAIR_INFO_
{
    /* 是否有效，0表示空闲，1表示使用中 */
    BOOL bBusy;
    /* 包裹匹配对中两个视角的包裹id */
    UINT32 au32PkgMatchPairId[2];
} SVA_PKG_MATCH_PAIR_INFO_S;

/* 安检算法模块全局参数 */
typedef struct tagSvaCommonSt
{
    UINT32 uiPublic;                              /* 全局参数初始化状态 */

    UINT32 uiDevCnt;                              /* 算法通道个数 */

    UINT32 uiChnCnt;                              /* 通道创建个数 */

    UINT32 uiVideoChnNum;                         /* 视频输入通道数 */

    UINT32 uiMainViewChn;                         /* 主视角通道号 */

    DEVICE_CHIP_TYPE_E uiDevType;                 /* 初始化接收应用下发的设备类型，用于区分单双芯片设备及主从芯片 */

    /* IA_PRODUCT_TYPE_E enProductType;              / * 产品类型 * / */

    SVA_VCAE_PRM_ST stEngProcPrm;                 /* 智能处理参数: 模式、AI模型使能 */

    SVA_DEV_PRM stSva_dev[SVA_DEV_MAX];           /* 算法通道 */

    BOOL bSplitMode;                              /* 集中判图模式开启状态 */

    SVA_SLAVE_PRM stSlavePrm[SVA_DEV_MAX];     /* 从片帧数据接收队列信息，主片不使用 */

    SVA_XTRANS_PRM_S stXtransPrm;               /* xtrans传输库相关参数 */

    SVA_ALG_RSLT_S stAlgRslt[SVA_DEV_MAX];        /* 算法回调的结果存放缓存 */

    SVA_DBG_PRM_S stDbgPrm;

	/* 引擎结果，为规避大堆栈放到全局中 */
	XSIE_SECURITY_XSI_OUT_T stXsieOutInfo;

    /* 包裹匹配通用参数，当前仅有关联索引 */
    SVA_PKG_MATCH_COMM_PRM_S stPkgMatchCommPrm;

    /* 双视角包裹匹配对信息 */
    SVA_PKG_MATCH_PAIR_INFO_S astPkgMatchPairInfo[SVA_MAX_PKG_MATCH_PAIR_NUM];

    void *mRsltMutexHdl;/*结果锁，避免双通道在从片向主片发送智能结果时，同时占用传输通道*/
} SVA_COMMON;

/* 透传镜像参数数据 */
typedef struct tagSvaAiSrcPrm
{
    UINT32 uiPaddingTopH;            /* 显示图像顶部padding高 */
    UINT32 uiPaddingBotH;            /* 显示图像底部padding高 */
    ROTATE_MODE enRotateType;        /* 显示图像相对RAW的旋转角度 */
    MIRROR_MODE enMirrorType;        /* 显示图像相对RAW的镜像类型 */
    /* UINT32 disp_width;               / *显示宽分辨率* / */
    /* UINT32 disp_height;              / *显示分辨率高* / */
} SVA_AI_SRC_PRM;

/*主从芯片传输帧参数结构体 */
typedef struct tagSvaTransFramePrm
{
    UINT32 uichan;               /* 智能检测通道 */
    UINT32 uiPkgWid;             /* 有效包裹宽 */
    UINT32 uiPKgHei;             /* 有效包裹高 */
    UINT32 uiXpackBufMaxW;       /* xpack中copy出来的有效包裹的最大宽度，最大不能超过1600，主要用于缩放*/
	UINT32 uiXpackBufMaxH;       /* xpack中copy出来的有效包裹的最大高度，最大不能超过1280，主要用于缩放*/
	UINT32 uiFrameNum;           /* 当前帧帧号，目前暂未使用*/
    UINT64 u64pts;               /* 当前帧时间戳 */
} SVA_TRANS_FRAME_PRM;

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : Sva_DrvSetDetectRegion
* 描  述  : 设置检测区域
* 输  入  : - chan: 通道号
*         : - prm : 检测区域
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetDetectRegion(UINT32 chan, VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetDectSwitch
* 描  述  : 危险物总开关
* 输  入  : - chan: 通道号
*         : - prm : 总开关
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetDectSwitch(UINT32 chan, VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetChnSize
* 描  述  : 设置安检机通道大小
* 输  入  : - chan: 通道号
*         : - prm : 通道大小
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetChnSize(UINT32 chan, VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetDireTion
* 描  述  : 设置安检机传送带方向
* 输  入  : - chan: 通道号
*         : - prm : 传送带方向
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetDirection(UINT32 chan, VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvGetVersion
* 描  述  : 获取算法版本号，目前引擎没有版本号
* 输  入  : - prm: 版本号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetVersion(VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvModuleInit
* 描  述  : 模块初始化
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvModuleInit(UINT32 chan);

/*******************************************************************************
* 函数名  : Sva_DrvModuleDeInit
* 描  述  : 模块去初始化
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvModuleDeInit(UINT32 chan);

/*******************************************************************************
* 函数名  : Sva_DrvDevInit
* 描  述  : 设备模块初始化
* 输  入  : - pDupHandle: 句柄
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvDevInit(void *pDupHandle);

/*******************************************************************************
* 函数名  : Sva_DrvSetAlertSwitch
* 描  述  : 单个危险物检测开关
* 输  入  : - chan: 通道号
*         : - prm : 开关
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetAlertSwitch(UINT32 chan, VOID *prm);

/**
 * @function:   Sva_DrvSetAlertTarNumThresh
 * @brief:      设置危险品告警个数阈值
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetAlertTarNumThresh(UINT32 chan, VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetAlertColor
* 描  述  : 设置危险物颜色
* 输  入  : - chan: 通道号
*         : - prm : 颜色
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetAlertColor(UINT32 chan, VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetAlertConf
* 描  述  : 设置检测置信度
* 输  入  : - chan: 通道号
*         : - prm : 置信度
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetAlertConf(UINT32 chan, VOID *prm);


/*******************************************************************************
* 函数名  : Sva_DrvSetOsdExtType
* 描  述  : 设置OSD拓展信息展示类型
* 输  入  : - chan: 通道号
*         : - prm : 置信度开关
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetOsdExtType(UINT32 chan, VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetAlertName
* 描  述  : 设置危险物名称
* 输  入  : - chan: 通道号
*         : - prm : 危险物名称
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetAlertName(UINT32 chan, VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetScaleLevel
* 描  述  : 设置OSD字体大小
* 输  入  : - chan: 通道号
*         : - prm : 字体大小
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetScaleLevel(UINT32 chan, VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetSyncMainChan
* 描  述  : 设置同步模式主视角通道(默认1为主视角，0为辅视角)
* 输  入  : - chan: 通道号
*         : - prm : 主视角通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetSyncMainChan(UINT32 chan, VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvGetSyncMainChan
* 描  述  : 设置同步模式主视角通道(默认1为主视角，0为辅视角)
* 输  入  :无
* 输  出  : 无
* 返回值  : 通道号
*******************************************************************************/
UINT32 Sva_DrvGetSyncMainChan();


/*******************************************************************************
* 函数名  : Sva_DrvSetOsdBorderType
* 描  述  : 设置OSD叠框类型
* 输  入  : chan : 通道号
*         : prm  : OSD叠框类型
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetOsdBorderType(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_DrvChgProcMode
* 描  述  : 设置处理模式
* 输  入  : - prm : 处理模式
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvChgProcMode(VOID *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetAiTargetMap
* 描  述  : 设置AI模型目标映射关系
* 输  入  : chan : 通道号
*         : prm  : 映射关系
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetAiTargetMap(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetModelId
* 描  述  : 设置Ai模型ID
* 输  入  : chan : 通道号
*         : prm  : 开关
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetModelId(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetModelName
* 描  述  : 设置Ai模型检测开关
* 输  入  : chan : 通道号
*         : prm  : 开关
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetModelName(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetModelDetectStatus
* 描  述  : 获取模型启用开关
* 输  入  : chan : 通道号
*         : prm  : Ai模型使用标志位
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注  意  : 通道号chan目前暂未启用，为预留变量
*******************************************************************************/
INT32 Sva_DrvSetModelDetectStatus(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_DrvGetModelDetectStatus
* 描  述  : 获取模型启用开关
* 输  入  : chan : 通道号
*         : prm  : Ai模型使用标志位
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注  意  : 通道号chan目前暂未启用，为预留变量
*******************************************************************************/
INT32 Sva_DrvGetModelDetectStatus(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_DrvInit
* 描  述  : 业务层初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvInit(void);

/*******************************************************************************
 * 函数名  : SVA_DrvGetAiSrcPrm
 * 描  述  : 获取镜像参数变量
 * 输  入  : 通道号
 * 输  出  : 对应的通道镜像参数
 * 返回值  : 无
 ********************************************************************************/
SVA_AI_SRC_PRM *SVA_DrvGetAiSrcPrm(UINT32 chan);

/*******************************************************************************
* 函数名  : Sva_DrvSetMirrorChnPrm
* 描  述  : 设置镜像通道参数
* 输  入  : - chan : 通道号
*         : - prm  : vpss组参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetMirrorChnPrm(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_DrvSetScareChnPrm
* 描  述  : 设置缩放通道参数
* 输  入  : - chan : 通道号
*         : - prm  : vpss组参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetScaleChnPrm(UINT32 chan, void *prm);

/*******************************************************************************
* 函数名  : Sva_DrvDbgSampleSwitch
* 描  述  : 设置素材采集开关(调试使用)
* 输  入  : chan: 通道号
*         : pPrm: 开关状态
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvDbgSampleSwitch(UINT32 chan, void *pPrm);

/*******************************************************************************
* 函数名  : Sva_DrvSetInputGapNum
* 描  述  : 设置帧处理间隔
* 输  入  : uiGapNum : 帧间隔
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetInputGapNum(void *pGapNum);

/*******************************************************************************
* 函数名  : Sva_DrvInputAutoTestPic
* 描  述  : 图片模型下进行自动化测试
* 输  入  : pData : 自动化测试使用的图片数据
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 入参有效性由内部接口保证，此处不进行校验
*******************************************************************************/
INT32 Sva_DrvInputAutoTestPic(UINT32 chan, CHAR *pData);

/**
 * @function:   Sva_DrvSetPkgSplitSensity
 * @brief:      设置包裹分割灵敏度
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetPkgSplitSensity(UINT32 chan, void *prm);

/**
 * @function:   Sva_DrvSetPkgSplitFilter
 * @brief:      设置包裹分割过滤阈值
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetPkgSplitFilter(UINT32 chan, void *prm);

/**
 * @function:   Sva_DrvSetZoomInOutVal
 * @brief:      设置放大缩小配置参数
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetZoomInOutVal(UINT32 chan, void *prm);

/**
 * @function:   Sva_DrvSetJpegPosSwitch
 * @brief:      设置Jpeg图片增加Pos信息开关
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetJpegPosSwitch(UINT32 chan, void *prm);

/**
 * @function:   Sva_DrvSetEngModel
 * @brief:      设置引擎模式
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetEngModel(UINT32 chan, void *prm);

/**
 * @function:   Sva_DrvSetTarFilterCfg
 * @brief:      配置目标尺寸过滤参数
 * @param[in]:  SVA_ALERT_SIZE_FILTER_PRM_S *pstSizeFilterPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetTarFilterCfg(SVA_ALERT_SIZE_FILTER_PRM_S *pstSizeFilterPrm);

/**
 * @function:   Sva_DrvGetDefConf
 * @brief:      获取默认置信度
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvGetDefConf(UINT32 chan, VOID *prm);

/**
 * @function:   Sva_DrvSetSprayFilterSwitch
 * @brief:      设置喷灌过滤开关
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetSprayFilterSwitch(UINT32 chan, VOID *prm);

/**
 * @function:   Sva_DrvSetInsCropCnt
 * @brief:	设置即时裁图每秒个数
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:	INT32
 */
INT32 Sva_DrvSetInsCropCnt(void *prm);

/**
 * @function:   Sva_DrvSetPkgBorGapPixel
 * @brief:	  设置包裹左右边沿预留像素
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:	  INT32
 */
INT32 Sva_DrvSetPkgBorGapPixel(UINT32 chan, VOID *prm);

/**
 * @function:   Sva_DrvSetPkgPrm
 * @brief:      设置包裹危险级别参数
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetPkgPrm(UINT32 chan, void *prm);

/**
 * @function:   Sva_DrvSetAlertLevel
 * @brief:      设置危险品级别
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetAlertLevel(UINT32 chan, void *prm);

/**
 * @function:   Sva_DrvSetInsCropCnt
 * @brief:      设置即时裁图每秒个数
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetInsCropCnt(void *prm);

/**
 * @function:   Sva_DrvSetSplitMode
 * @brief:      设置集中判图模式开关
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetSplitMode(void *prm);

/**
 * @function:   Sva_DrvSetPkgCombPixelThresh
 * @brief:      用于更新包裹融合像素阈值
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetPkgCombPixelThresh(UINT32 chan, void *prm);

SVA_DEV_PRM *Sva_DrvGetDev(UINT32 chan);

/**
 * @function    Sva_DrvDbgInit
 * @brief       模块调试信息初始化
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Sva_DrvDbgInit(VOID);

/**
 * @function:   Sva_DrvSetForceSplitSwitch
 * @brief:      设置包裹强制分割开关
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetForceSplitSwitch(UINT32 chan, void *prm);

/**
 * @function:   Sva_DrvSetPkgCombineFlag
 * @brief:      设置包裹融合开关
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetPkgCombineFlag(UINT32 chan, void *prm);

/**
 * @function   Sva_DrvUpgradeModel
 * @brief      模型升级
 * @param[in]  UINT32 chan  
 * @param[in]  VOID *prm    
 * @param[out] None
 * @return     INT32
 */
INT32 Sva_DrvUpgradeModel(UINT32 chan, VOID *prm);

/**
 * @function   Sva_DrvResetTrans
 * @brief      重置通信模块
 * @param[in]  UINT32 chan  
 * @param[in]  VOID *sprm   
 * @param[out] None
 * @return     INT32
 */
INT32 Sva_DrvResetTrans(UINT32 chan, VOID *prm);

#endif

