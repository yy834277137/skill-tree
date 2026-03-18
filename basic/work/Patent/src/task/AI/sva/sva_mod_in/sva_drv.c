/*******************************************************************************
* sva_drv.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : huangshuxin <huangshuxin@hikvision.com>
* Version: V1.0.0  2019年2月14日 Create
*
* Description :
* Modification:
*******************************************************************************/


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
#include "sva_drv.h"
#include "system_prm_api.h"
#include "task_ism.h"
#include "capbility.h"
#include "osd_drv_api.h"
#include "drawChar.h"
#include "vdec_tsk_api.h"

/* 双视角包裹匹配 */
#include "dual_pkg_match.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define SVA_DRV_CHECK_CHAN(chan, value) {if (chan > (SVA_DEV_MAX - 1)) {SVA_LOGE("Chan %d (Illegal parameters)\n", chan); return (value); }}
#define SVA_DRV_CHECK_PRM(ptr, value)	{if (!ptr) {SVA_LOGE("Ptr (The address is empty or Value is 0 )\n"); return (value); }}
extern SAL_STATUS xpack_package_launch_aidgr(UINT32 u32Chan, SYSTEM_FRAME_INFO *pstSysFrame);/*获取xpack传过来的帧信息*/

#define SVA_DRV_CHECK_RET(ret, loop, str) \
    { \
        if (ret) \
        { \
            SVA_LOGE("%s, ret: 0x%x \n", str, ret); \
            goto loop; \
        } \
    }

#define SVA_DRV_CHECK_RET_NO_LOOP(ret, str) \
    { \
        if (ret) \
        { \
            SVA_LOGE("%s, ret: 0x%x \n", str, ret); \
        } \
    }

#define SVA_DRV_CHECK_PTR(ptr, loop, str) \
    { \
        if (!ptr) \
        { \
            SVA_LOGE("%s \n", str); \
            goto loop; \
        } \
    }

#ifdef SVA_DRV_SYNC_FLG
#define THRESHOLD_0 (1000 / (capt_func_chipGetViSrcRate(0) / Sva_DrvGetInputGapNum()))                 /* 手动抽帧通道0的帧间隔，单位ms */
#define THRESHOLD_1 (1000 / (capt_func_chipGetViSrcRate(1) / Sva_DrvGetInputGapNum()))                 /* 手动抽帧通道1的帧间隔，单位ms */
#endif

#define SVA_SYNC_FRAME_GAP_THRESH (3)

#define SVA_JPEG_MASK_NO_RECT	(0x01)                /* 智能回调抓图无叠框jpeg掩码 */
#define SVA_JPEG_MASK_RECT		(0x10)                /* 智能回调抓图叠框jpeg掩码 */

#define SVA_BMP_MASK_NO_RECT (0x01)                   /* 智能回调抓图无叠框BMP掩码 */

#define SVA_NEW_PKG_PIXEL_GAP	(100)              /* 判断是否有新包裹出现的包裹前沿跳变值 */
#define SVA_NEW_PKG_FLOAT_GAP	((float)SVA_NEW_PKG_PIXEL_GAP / (float)SVA_MODULE_WIDTH)              /* 判断是否有新包裹出现的包裹前沿跳变值 */

#define SVA_VALID_PKG_MIN_PIXEL_WIDTH	(100) /* (50)                                                           / * 过滤较窄的包裹，暂定过滤像素值为50 * / */
#define SVA_VALID_PKG_FILTER_THRES		((float)SVA_VALID_PKG_MIN_PIXEL_WIDTH / (float)SVA_MODULE_WIDTH) /* 过滤较窄包裹阈值，float */
#define SVA_INS_CUT_TIMES_PER_SEC		(g_InsProcCntPs) /* 快速编图每秒裁剪张数 */

#define SVA_PKG_MIN_WIDTH	(0.09375)              /* 包裹前沿与画面边沿距离，默认过滤小于120像素的包裹，120/1280 */
#define ERR_FLOAT_VAL		(2.0f)                 /* 异常坐标结果，用于异常处理 */
#define SVA_MIN_SLICE_WIDTH (0.025f)               /* 最小分片宽度，32像素 */

#define SVA_PKG_FILTER_MAX_THRES		(0.8) /* 过滤较宽包裹阈值，float */
#define SVA_LAST_SPLIT_PIC_OVER_TIME_MS (1000)     /* 最后一个分片超时时间，暂定1000ms */

#define SVA_CAL_DIFF(a, b) (a > b ? a - b : b - a)
#define SVA_DEFAULT_TIMEOUT_NUM		(2000) /* 默认超时次数 */
#define SVA_DEFAULT_SINGLE_WAIT_MS	(1)   /* 默认单次休眠ms */
#define SVA_SYNC_ERR_TIMEOUT		(0x77770001)    /* 智能模块同步超时 */
#define SVA_JPEG_CNT_MAX_NUM		(80)    /* 编图最大违禁品个数 */

#define SVA_DMA_MMAP_TYPE			(2)
#define SVA_DMA_TRANS_TIMEOUT_CNT	(1000)

#define SVA_MAX_PACK_IN_BUFFER		(10)         /* 智能安检机输入队列缓存*/
#define SVA_MAX_PROCESS_OUT_BUFFER	(10)         /* 智能安检机输出结果队列缓存*/

#define SVA_ALG_Y_SIZE		(SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT)
#define SVA_ALG_UV_SIZE		(SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT / 2)
#define SVA_ALG_YUV_SIZE	(SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2)

#define XSIE_POS_SIZE			(1024 * 600)
#define XSIE_POS_FIRST_SIZE		(320 * 1280)
#define XSIE_POS_SECOND_SIZE	(XSIE_POS_SIZE - XSIE_POS_FIRST_SIZE)

#define SVA_POS_YUV_SIZE (SVA_ALG_YUV_SIZE + XSIE_POS_SIZE)

static BOOL g_bRsetFlag = SAL_TRUE;



/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
extern INT32 capt_func_chipGetViSrcRate(UINT32 viChn);  /*待sva重构后移动到task层*/
extern INT32 disp_osdClearSvaResult(UINT32 chan, DISP_CLEAR_SVA_TYPE *clearprm); /*待sva重构后移动到task层*/

static SVA_PROC_MODE_E g_proc_mode = SVA_PROC_MODE_IMAGE;
static UINT32 guiAiModelEnable = SAL_FALSE;       /* AI模型使能开关 */
static UINT32 gDbgSaveJpg[2] = {0, 0};            /* 调试使用: 置1保存jpg */
static UINT32 gDbgSampleCollect[2] = {0, 0};      /* 素材收集bmp数据开关，后续开放接口给上层应用 */
/* static UINT32 uiLastFrameNum[SVA_DEV_MAX] = {0, 0};   // SVA_REFAC */
static SVA_AI_SRC_PRM stAiSrcPrm[2] = {0};     /*透传数据:用于后续智能显示坐标变化*/
static UINT32 guiPosWriteFlag = 0;                  /*调试使用，置1开启pos信息*/

float fBorGap = 0.011718; /* 15.0f / (float)SVA_MODULE_WIDTH;    / * 裁剪包裹的左右边界 * / */
/* float fMinGap = 0.003125; / * 4.0f / (float)SVA_MODULE_WIDTH;     / * 最小边界 * / * / */

static SVA_COMMON g_sva_common =
{
    .uiDevCnt = 0,
    .uiChnCnt = 0,
    .stPkgMatchCommPrm =
    {
        .u32MatchIdx = 1,
    },
};

/*计算算法整体耗时时间*/
static UINT32 uiEngineStartTime0 = 0;
static UINT32 uiEngineStartTime1 = 0;
#if 0
/*计算sva整体耗时时间*/
static UINT32 uiSvaStartTime[2] = {0, 0};
static UINT32 uiSvaEndTime[2] = {1, 1};



#endif

static UINT32 g_uiInputGapNum = 1;                 /* 帧间隔 */

static UINT32 uiSwitch[3] = {0, 0, 60};            /* 0: 测试75fps+4fps, 1: 开启导出yuv数据, 2: 输入数据帧率 */

/* 违禁品大类合并后，细分类别在所属大类中的区分索引，用于OSD叠加展示与回溯区分
    e.g.

        [大类]     [细分类别-索引]

        剪刀类----- 常规剪刀-1
   |-- 折叠剪刀-2
   |-- U型刺绣剪刀-3

    说明:
        1. 剪刀大类 包含的细分子类有3个，区分索引依次是1、2、3，这个索引值和细分类别Type不冲突;
        2. 区分索引使用的地方主要是OSD叠加，细分类别在叠加时的效果如下:
           常规剪刀(54.01%)
           折叠剪刀(23.02%)
           U型刺绣剪刀(88.03%)
        3. 如2所述，区分索引仅影响OSD展示时的小数位，当前由于特定大类包含超过10种的细分子类，所以统一约定小数点后展示2位;
        4. 对于大类即细分类别的情况，当前约定不展示小数点，置信度OSD仅精确到整数，如:
            手机(66%)
        5. 当前仅处理基础违禁品类别，不支持AI违禁品处理;
        6. 违禁品类别本身即细分类别时，细分类别索引为0;
        7. 支持AI违禁品处理;

    [add]: sunzelin@2022.04.27 Add Comments
    [mod]: sunzelin@2022.05.05 补充说明5、6
    [mod]: sunzelin@2022.11.03 补充7，支持AI
 */
static UINT32 au32ClassSubIdxTab[SVA_MAX_ALARM_TYPE] = {0};

/* 对于支持合并的细分类别，使用所述大类中最小的类别id进行标记，用于显示包裹模式下进行大类合并展示 */
static UINT32 au32ClassSubMinIdxTab[SVA_MAX_ALARM_TYPE] = {0};

#if 0
static float g_fDownScaleSizeTab[IA_PRODUCT_TYPE_NUM][SVA_FRCNN_MACHINE_NUM] =
{
    /* 分析仪支持不同型号安检机的缩放因子 */
    {2.3, 1.914, 1.5, 1.0, 1.0},

    /* 不同型号的安检机对应的缩放因子 */
    {1.0, 1.16, 1.5, 1.0, 1.0},
};
#endif

SVA_OFFSET_BUF_TAB_S gstOffsetBufTab[SVA_DEV_MAX] = {0};
static UINT32 uiJpegAddposSwitch = 0;

static UINT32 uiInitFlag = SAL_FALSE;             /* 对初始化过程进行保护 */
static UINT32 uiProcModeFlag = SAL_FALSE;         /* 对模式切换过程进行保护 */
static SVA_VCAE_PRM_ST stTmpChgProcMode = {0};               /* 用于模式切换线程中传入的全局符号，避免栈变量回收 */
static UINT32 uiProcRsltSts = SAL_FAIL;           /* 默认处理状态为fail */

/* 算法底层资源是否有修改 */
static BOOL bAlgResChg = SAL_FALSE;

static UINT32 uiLastTimeRef[SVA_DEV_MAX] = {0};
static SVA_PKG_TAR_CNT_S stPostProcPkgCntInfo[2] = {0};

static UINT32 g_InsProcCntPs = 4;            /* 即时编图裁剪个数，默认为4，每秒裁两次 */
static UINT32 u32SplitFlag = 0;              /* split调试信息开关 */
static UINT32 uiLastFrmOffsetflag = 1; /* 0：帧号, 1：包裹前沿 */

static UINT32 uiStillCnt[SVA_DEV_MAX] = {0};    /* 集中判图使用，判断是否画面静止 */


/* 控制参数: 打印智能向外部模块输出的结果信息，当前仅安检机图片模式使用 */
static UINT32 g_bPrOutInfo = 0; /*0表示关闭，1表示只打印引擎传出的智能结果，2表示只打印SVA模块传出的智能结果，>=3表示两者都打印*/

typedef enum _SVA_EVENT_TYPE_
{
    SVA_INIT_END_STATUS = 0,                    /* 初始化是否结束-事件 */
    SVA_PROC_MODE_STATUS,                       /* 模式切换是否结束-事件 */
    SVA_EVENT_TYPE_NUM,
} SVA_EVENT_TYPE_E;

typedef enum _SVA_EVENT_STS_
{
    SVA_EVENT_STS_OK = 0,                       /* 状态已结束 */
    SVA_EVENT_STS_FAIL,                         /* 状态进行中 */
    SVA_EVENT_STS_NUM,
} SVA_EVENT_STS_E;

/* 维护模块节点参数 */
typedef struct _SVA_DBG_NODE_PRM_
{
    CHAR acNodeName[32];                          /* 节点名称 */
    STS_NODE_VAL_TYPE_E enNodeValType;            /* 节点存放变量类型 */
    sts_get_outer_val pFunc;                      /* 回调函数，用于维护模块获取节点值 */
} SVA_DBG_NODE_PRM_S;

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */
/* extern INT32 Xpack_drvGetAiYuvBuff(UINT32 srcChn, SYSTEM_FRAME_INFO *pstFrame, AI_YUV_SRC_PRM *aiprm); */
/* extern HKA_STATUS HIKXSI_GetConfig(HKA_VOID *handle, HKA_S32 cfg_type, HKA_VOID *cfg_buf, HKA_SZT cfg_size); */

/* 统计平均耗时 */
#define SVA_MAX_TRANS_DBG_GROUP_NUM			(16)     /* 最大group个数 */
#define SVA_MAX_TRANS_DBG_GROUP_NAME_LEN	(64)     /* 每个group名称最长长度 */

#define SVA_TRANS_DBG_OFFSET (64)
#define SVA_TRANS_GET_IDX(a, offset)	(a % offset)
#define SVA_TRANS_CONV_IDX(a)			(SVA_TRANS_GET_IDX(a, SVA_TRANS_DBG_OFFSET) - 1)

#define MST_YUV_TYPE_BASE_IDX		(0x0)
#define MST_YUV_GET_CHN_FRAME_TYPE	(MST_YUV_TYPE_BASE_IDX + 0x1)
#define MST_YUV_CHECK_MSG_STS_TYPE	(MST_YUV_TYPE_BASE_IDX + 0x2)
#define MST_YUV_SET_PHY_ADDR_TYPE	(MST_YUV_TYPE_BASE_IDX + 0x3)
#define MST_YUV_DMA_IOCTL_TYPE		(MST_YUV_TYPE_BASE_IDX + 0x4)
#define MST_YUV_SET_STS_TRUE_TYPE	(MST_YUV_TYPE_BASE_IDX + 0x5)
#define MST_YUV_TOTAL_TYPE			(MST_YUV_TYPE_BASE_IDX + 0x6)

#define SLV_YUV_TYPE_BASE_IDX		(0x40)
#define SLV_YUV_CHECK_MSG_STS_TYPE	(SLV_YUV_TYPE_BASE_IDX + 0x1)
#define SLV_YUV_GET_PHY_ADDR_TYPE	(SLV_YUV_TYPE_BASE_IDX + 0x2)
#define SLV_YUV_DMA_IOCTL_TYPE		(SLV_YUV_TYPE_BASE_IDX + 0x3)
#define SLV_YUV_SET_STS_FALSE_TYPE	(SLV_YUV_TYPE_BASE_IDX + 0x4)
#define SLV_YUV_TOTAL_TYPE			(SLV_YUV_TYPE_BASE_IDX + 0x5)
#define SLV_YUV_GET_QUEUE_TYPE		(SLV_YUV_TYPE_BASE_IDX + 0x6)

#define MST_PRM_TYPE_BASE_IDX		(0x80)
#define MST_PRM_CPY_TO_TMP_BUF_TYPE (MST_PRM_TYPE_BASE_IDX + 0x1)
#define MST_PRM_CHECK_MSG_STS_TYPE	(MST_PRM_TYPE_BASE_IDX + 0x2)
#define MST_PRM_MMAP_MEMCPY_TYPE	(MST_PRM_TYPE_BASE_IDX + 0x3)
#define MST_PRM_SET_STS_TRUE_TYPE	(MST_PRM_TYPE_BASE_IDX + 0x4)
#define MST_PRM_TOTAL_TYPE			(MST_PRM_TYPE_BASE_IDX + 0x5)

#define SLV_PRM_TYPE_BASE_IDX		(0xc0)
#define SLV_PRM_CHECK_MSG_STS_TYPE	(SLV_PRM_TYPE_BASE_IDX + 0x1)
#define SLV_PRM_MMAP_MEMCPY_TYPE	(SLV_PRM_TYPE_BASE_IDX + 0x2)
#define SLV_PRM_SET_STS_FALSE_TYPE	(SLV_PRM_TYPE_BASE_IDX + 0x3)
#define SLV_PRM_TOTAL_TYPE			(SLV_PRM_TYPE_BASE_IDX + 0x4)

#define MST_RSLT_TYPE_BASE_IDX		(0x100)
#define MST_RSLT_CHECK_MSG_STS_TYPE (MST_RSLT_TYPE_BASE_IDX + 0x1)
#define MST_RSLT_SET_PHY_ADDR_TYPE	(MST_RSLT_TYPE_BASE_IDX + 0x2)
#define MST_RSLT_DMA_IOCTL_TYPE		(MST_RSLT_TYPE_BASE_IDX + 0x3)
#define MST_RSLT_SET_STS_TRUE_TYPE	(MST_RSLT_TYPE_BASE_IDX + 0x4)
#define MST_RSLT_TOTAL_TYPE			(MST_RSLT_TYPE_BASE_IDX + 0x5)

#define SLV_RSLT_TYPE_BASE_IDX		(0x140)
#define SLV_RSLT_CHECK_MSG_STS_TYPE (SLV_RSLT_TYPE_BASE_IDX + 0x1)
#define SLV_RSLT_GET_PHY_ADDR_TYPE	(SLV_RSLT_TYPE_BASE_IDX + 0x2)
#define SLV_RSLT_DMA_IOCTL_TYPE		(SLV_RSLT_TYPE_BASE_IDX + 0x3)
#define SLV_RSLT_SET_STS_FALSE_TYPE (SLV_RSLT_TYPE_BASE_IDX + 0x4)
#define SLV_RSLT_TOTAL_TYPE			(SLV_RSLT_TYPE_BASE_IDX + 0x5)

typedef enum _SVA_TRANS_DBG_TYPE_
{
    SVA_TRANS_DBG_MASTER_YUV_TYPE = 0,          /* 主片发送YUV */
    SVA_TRANS_DBG_SLAVE_YUV_TYPE,               /* 从片接收YUV */

    SVA_TRANS_DBG_MASTER_PARAM_TYPE,            /* 主片发送帧参数 */
    SVA_TRANS_DBG_SLAVE_PARAM_TYPE,             /* 从片接收帧参数 */

    SVA_TRANS_DBG_MASTER_RESULT_TYPE,           /* 主片接收智能结果 */
    SVA_TRANS_DBG_SLAVE_RESULT_TYPE,            /* 从片发送智能结果 */

    SVA_TRANS_DBG_TYPE_NUM,
} SVA_TRANS_DBG_TYPE_E;

typedef struct _SVA_TRANS_DBG_GROUP_INFO_
{
    CHAR acName[SVA_MAX_TRANS_DBG_GROUP_NAME_LEN];     /* 当前统计的子类 */
    UINT32 u32DbgTotalCalTime;                         /* 该子类累积统计耗时，单位ms */
} SVA_TRANS_DBG_GROUP_INFO_S;

typedef struct _SVA_TRANS_DBG_GROUP_PRM_
{
    UINT32 u32CalCnt;                                      /* 统计次数，当前按照1s统计一次，换算到帧数 */
    CHAR acName[SVA_MAX_TRANS_DBG_GROUP_NAME_LEN];         /* 当前Group名称 */

    UINT32 u32SubCnt;                                      /* 统计的子类个数 */
    SVA_TRANS_DBG_GROUP_INFO_S stTransDbgGroupInfo[];      /* 子类详细统计耗时信息 */
} SVA_TRANS_DBG_GROUP_PRM_S;

typedef struct _SVA_TRANS_DBG_PRM_
{
    BOOL bInit;                                                             /* 初始化状态位 */
    SVA_TRANS_DBG_GROUP_PRM_S *pstSvaDbgGroupPrm[SVA_TRANS_DBG_TYPE_NUM];   /* 多个group信息 */
} SVA_TRANS_DBG_PRM_S;

/* 传输调试初始化结构体，主片发送YUV */
static SVA_TRANS_DBG_GROUP_PRM_S stSvaTransDbgMstYuvGrpPrm =
{
    .u32CalCnt = 0,
    .acName = "master yuv",

    .u32SubCnt = 6,
    .stTransDbgGroupInfo =
    {
        {"get chn frame", 0},
        {"check msg sts", 0},
        {"set phy addr", 0},
        {"dma ioctl", 0},
        {"set sts [true]", 0},
        {"total", 0},
    },
};

/* 传输调试初始化结构体，从片接收YUV */
static SVA_TRANS_DBG_GROUP_PRM_S stSvaTransDbgSlvYuvGrpPrm =
{
    .u32CalCnt = 0,
    .acName = "slave yuv",

    .u32SubCnt = 6,
    .stTransDbgGroupInfo =
    {
        {"check msg sts", 0},
        {"get phy addr", 0},
        {"dma ioctl", 0},
        {"set sts [false]", 0},
        {"total", 0},
        {"get queue", 0},
    },
};

/* 传输调试初始化结构体，主片发送帧参数 */
static SVA_TRANS_DBG_GROUP_PRM_S stSvaTransDbgMstPrmGrpPrm =
{
    .u32CalCnt = 0,
    .acName = "master prm",

    .u32SubCnt = 5,
    .stTransDbgGroupInfo =
    {
        {"cpy to tmp buf", 0},
        {"check msg sts", 0},
        {"mmap memcpy", 0},
        {"set sts [true]", 0},
        {"total", 0},
    },
};

/* 传输调试初始化结构体，从片接收帧参数 */
static SVA_TRANS_DBG_GROUP_PRM_S stSvaTransDbgSlvPrmGrpPrm =
{
    .u32CalCnt = 0,
    .acName = "slave prm",

    .u32SubCnt = 4,
    .stTransDbgGroupInfo =
    {
        {"check msg sts", 0},
        {"mmap memcpy", 0},
        {"set sts [false]", 0},
        {"total", 0},
    },
};

/* 传输调试初始化结构体，主片接收智能结果 */
static SVA_TRANS_DBG_GROUP_PRM_S stSvaTransDbgMstRsltGrpPrm =
{
    .u32CalCnt = 0,
    .acName = "master rslt",

    .u32SubCnt = 5,
    .stTransDbgGroupInfo =
    {
        {"check msg sts", 0},
        {"set phy addr", 0},
        {"dma ioctl", 0},
        {"set sts [true]", 0},
        {"total", 0},
    },
};

/* 传输调试初始化结构体，从片发送智能结果 */
static SVA_TRANS_DBG_GROUP_PRM_S stSvaTransDbgSlvRsltGrpPrm =
{
    .u32CalCnt = 0,
    .acName = "slave rslt",

    .u32SubCnt = 5,
    .stTransDbgGroupInfo =
    {
        {"check msg sts", 0},
        {"get phy addr", 0},
        {"dma ioctl", 0},
        {"set sts [false]", 0},
        {"total", 0},
    },
};

static SVA_TRANS_DBG_PRM_S stSvaTransDbgPrm = {0};                 /* SVA模块传输调试全局结构体 */

static BOOL bShowPrtInfo = SAL_FALSE;

INT32 Sva_DrvDbgInit(VOID);    /* 维护模块初始化接口声明 */

/**
 * @function    Sva_DrvDelPackResFromEngine
 * @brief         外部模块删除一帧包裹智能处理结果(安检机使用)
 * @param[in]  chan-通道号
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Sva_DrvDelPackResFromEngine(UINT32 chan)
{
    /*入参校验*/
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PROCESS_OUT_DATA *pstSvaPackDataInfo = NULL;
    pstSva_dev = Sva_DrvGetDev(chan);
    DSA_QueHndl *pstSvaPackResFullQue = NULL;
    DSA_QueHndl *pstSvaPackResEmptQue = NULL;
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);
    pstSvaPackResFullQue = &pstSva_dev->stOutPackageDataFullQue;
    pstSvaPackResEmptQue = &pstSva_dev->stOutPackageDataEmptQue;
    INT32 s32Ret = 0;
    INT32 uiQuueCount = 0;
    /*获取数据前先查看队列是否为空*/
    uiQuueCount = DSA_QueGetQueuedCount(pstSvaPackResFullQue);
    if (uiQuueCount <= 0)
    {
        SVA_LOGE("Sva Output Que have No Result!!\n");
        goto err;
    }

    /*出队列*/
    s32Ret = DSA_QueGet(pstSvaPackResFullQue, (void **)&pstSvaPackDataInfo, SAL_TIMEOUT_FOREVER);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Sva Output Que No Date!\n");
        goto err;
    }

    /*数据回收*/
    DSA_QuePut(pstSvaPackResEmptQue, (void *)pstSvaPackDataInfo, SAL_TIMEOUT_NONE);
    return SAL_SOK;
err:
    return SAL_FAIL;

}

/**
 * @function    Sva_DrvGetPackResFromEngine
 * @brief         外部模块获取显示数据结果队列数据(安检机使用)
 * @param[in]  chan-通道号
 * @param[in]  pstSvaProcessDataToXpack-其他模块需要输入引擎的数据
 * @param[out] NULL
 * @return  SAL_SOK
 */
INT32 Sva_DrvGetPackResFromEngine(UINT32 chan, SVA_PROCESS_OUT_DATA *pstSvaProcessDataToXpack)
{
    /*入参校验*/
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSvaProcessDataToXpack, SAL_FAIL);
    SVA_DEV_PRM *pstSva_dev = NULL;
    DSA_QueHndl *pstSvaPackResQue = NULL;
	SVA_PROCESS_OUT_DATA *pstSvaProcessData = NULL;
    INT32 s32Ret = 0;
    INT32 uiQuueCount = 0;
    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstSvaPackResQue = &pstSva_dev->stOutPackageDataFullQue;

    /*获取数据前先查看队列是否为空*/
    uiQuueCount = DSA_QueGetQueuedCount(pstSvaPackResQue);
    if (uiQuueCount <= 0)
    {
        goto err;
    }

    /*只是获取输出队列数据，不会删除当前节点*/
    s32Ret = DSA_QuePeek(pstSvaPackResQue, (void **)&pstSvaProcessData);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Sva Output Que No Date!\n");
        goto err;
    }

	/*直接copy进xpack的buffer后，我们可以直接释放这个队列*/
	sal_memcpy_s(pstSvaProcessDataToXpack, sizeof(SVA_PROCESS_OUT_DATA), pstSvaProcessData, sizeof(SVA_PROCESS_OUT_DATA));

    SVA_LOGD("SvaPackOut %p time_ref %d frame_num %d xsi_num %d \n", pstSvaProcessData, pstSvaProcessData->uiSvaPackPrm.timeRef, pstSvaProcessData->uiSvaProcessRes.frame_num, pstSvaProcessData->uiSvaProcessRes.target_num);

    /*释放队列*/
	Sva_DrvDelPackResFromEngine(chan);
	
	return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Sva_DrvGetRunState
 * @brief         外部模块获取智能模块状态信息(安检机使用)
 * @param[in]   chan - 通道号
 * @param[out] pChnStatus - 智能模块运行状态信息
 * @param[out] pUiSwitch   - 是否回调智能结果开关
 * @return 帧数据地址
 */
INT32 Sva_DrvGetRunState(UINT32 chan, UINT32 *pChnStatus, UINT32 *pUiSwitch)
{
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pChnStatus, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pUiSwitch, SAL_FAIL);

    SVA_DEV_PRM *pstSva_dev = NULL;
    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    *pChnStatus = pstSva_dev->uiChnStatus;
    *pUiSwitch = pstSva_dev->uiSwitch;
    SVA_LOGD("get uiChnStatus %d uiWitch %d\n", (UINT32)*pChnStatus, (UINT32)*pUiSwitch);

    return SAL_SOK;

}

/**
 * @function    Sva_DrvGetSvaNodeFrame
 * @brief       给外部模块输入一帧AI数据接口(安检机使用)
 * @param[in]   chan - 通道号
 * @param[out] NULL
 * @return 帧数据地址
 */
SYSTEM_FRAME_INFO *Sva_DrvGetSvaNodeFrame(UINT32 chan)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, NULL);

    if (NULL != &pstSva_dev->stGetPackSysFrame[chan])
    {
        if (0 == pstSva_dev->stGetPackSysFrame[chan].uiAppData)
        {
            (VOID)sys_hal_allocVideoFrameInfoSt(&pstSva_dev->stGetPackSysFrame[chan]);
        }

        return &pstSva_dev->stGetPackSysFrame[chan];
    }
    else
    {
        SVA_LOGE("Sva Frme is NULL!!!\n");
        goto err;
    }

err:
    return NULL;
}

/**
 * @function    Sva_DrvSendAiDataToEngine
 * @brief        外部模块输入给智能引擎一帧数据(安检机使用)
 * @param[in]  chan-通道号
 * @param[in]  pstSvaAiData-其他模块需要输入引擎的数据
 * @param[out] SAL_SOK
 * @return SAL_SOK
 */
INT32 Sva_DrvSendAiDataToEngine(UINT32 chan, SVA_PACKAGE_DATE_IN *pstSvaAiData)
{
    /*入参校验*/
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSvaAiData, SAL_FAIL);
    SVA_DEV_PRM *pstSva_dev = NULL;
    DSA_QueHndl *pstSvaInDataQue = NULL;
    INT32 s32Ret = 0;
    INT32 uiQuueCount = 0;
    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);
    pstSvaInDataQue = &pstSva_dev->stInPackageDataQue;
    /*智能通道开启之前不输入数据*/
    if (SAL_TRUE != pstSva_dev->uiChnStatus)
    {
        /* SVA_LOGE("Sva init is not ok!!! wait...\n"); */
        return SAL_FAIL;
    }

    /*塞数据前先确认队列是否已满*/
    uiQuueCount = DSA_QueGetQueuedCount(pstSvaInDataQue);
    if (uiQuueCount >= SVA_MAX_PACK_IN_BUFFER)
    {
        SVA_LOGE("Sva Input Que Already full!!\n");
        goto err;
    }

    /*队列塞数据*/
    s32Ret = DSA_QuePut(pstSvaInDataQue, (void *)pstSvaAiData, SAL_TIMEOUT_NONE);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("put Sva Data Err!!\n");
        goto err;
    }

    SVA_LOGD("Input data to SvaEngine!!!\n");
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Sva_DrvTransSetPrtFlag
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_DrvTransSetPrtFlag(BOOL bFlag)
{
    bShowPrtInfo = bFlag;
    return;
}

/**
 * @function    Sva_DrvTransGetPrtFlag
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static BOOL Sva_DrvTransGetPrtFlag(VOID)
{
    return bShowPrtInfo;
}

/**
 * @function:   Sva_DrvTransDbgClr
 * @brief:     清空统计信息
 * @param[in]:  SVA_TRANS_DBG_TYPE_E enType
 * @return:  VOID
 */
static VOID Sva_DrvTransDbgClr(SVA_TRANS_DBG_TYPE_E enType)
{
    UINT32 i = 0;

    SVA_TRANS_DBG_GROUP_PRM_S *pstDbgGroupPrm = NULL;

    if (enType >= SVA_TRANS_DBG_TYPE_NUM)
    {
        SAL_ERROR("invalid prm! type [%d] \n", enType);
        return;
    }

    pstDbgGroupPrm = stSvaTransDbgPrm.pstSvaDbgGroupPrm[enType];
    if (NULL == pstDbgGroupPrm)
    {
        SAL_ERROR("ptr null! type %d \n", enType);
        return;
    }

    if (pstDbgGroupPrm->u32CalCnt < 60)
    {
        return;
    }

    pstDbgGroupPrm->u32CalCnt = 0;

    for (i = 0; i < pstDbgGroupPrm->u32SubCnt; i++)
    {
        pstDbgGroupPrm->stTransDbgGroupInfo[i].u32DbgTotalCalTime = 0;
    }

    return;
}

/**
 * @function:   Sva_DrvTransDbgPrTime
 * @brief:     打印调试耗时
 * @param[in]:  SVA_TRANS_DBG_TYPE_E enType
 * @return:  VOID
 */
static VOID Sva_DrvTransDbgPrTime(SVA_TRANS_DBG_TYPE_E enType)
{
    UINT32 i = 0;

    SVA_TRANS_DBG_GROUP_PRM_S *pstDbgGroupPrm = NULL;

    if (enType >= SVA_TRANS_DBG_TYPE_NUM)
    {
        SAL_ERROR("invalid prm! type [%d] \n", enType);
        return;
    }

    pstDbgGroupPrm = stSvaTransDbgPrm.pstSvaDbgGroupPrm[enType];
    if (NULL == pstDbgGroupPrm)
    {
        SAL_ERROR("ptr null! type %d \n", enType);
        return;
    }

    if ((SAL_TRUE != Sva_DrvTransGetPrtFlag()) || (pstDbgGroupPrm->u32CalCnt < 60))
    {
        return;
    }

    printf("%s: ", pstDbgGroupPrm->acName);
    for (i = 0; i < pstDbgGroupPrm->u32SubCnt; i++)
    {
        printf("%s: %d, ", pstDbgGroupPrm->stTransDbgGroupInfo[i].acName, pstDbgGroupPrm->stTransDbgGroupInfo[i].u32DbgTotalCalTime / pstDbgGroupPrm->u32CalCnt);
    }

    printf("\n");

    return;
}

/**
 * @function:   Sva_DrvTransDbgAddCalCnt
 * @brief:     累加调试帧数
 * @param[in]:  SVA_TRANS_DBG_TYPE_E enType
 * @return:  VOID
 */
static VOID Sva_DrvTransDbgAddCalCnt(SVA_TRANS_DBG_TYPE_E enType)
{
    SVA_TRANS_DBG_GROUP_PRM_S *pstDbgGroupPrm = NULL;

    if (enType >= SVA_TRANS_DBG_TYPE_NUM)
    {
        SAL_ERROR("invalid prm! type [%d] \n", enType);
        return;
    }

    pstDbgGroupPrm = stSvaTransDbgPrm.pstSvaDbgGroupPrm[enType];
    if (NULL == pstDbgGroupPrm)
    {
        SAL_ERROR("ptr null! type %d \n", enType);
        return;
    }

    pstDbgGroupPrm->u32CalCnt++;
    return;
}

/**
 * @function:   Sva_DrvTransDbgAddTime
 * @brief:     累加调试耗时
 * @param[in]:  SVA_TRANS_DBG_TYPE_E enType
 * @param[in]:  UINT32 u32SubIdx
 * @param[in]:  UINT32 u32StartTime
 * @param[in]:  UINT32 u32EndTime
 * @return:  VOID
 */
static VOID Sva_DrvTransDbgAddTime(SVA_TRANS_DBG_TYPE_E enType, UINT32 u32SubIdx, UINT32 u32StartTime, UINT32 u32EndTime)
{
    SVA_TRANS_DBG_GROUP_PRM_S *pstDbgGroupPrm = NULL;

    if (enType >= SVA_TRANS_DBG_TYPE_NUM || u32EndTime < u32StartTime)
    {
        SAL_ERROR("invalid prm! type [%d], start[%d], end[%d] \n", enType, u32StartTime, u32EndTime);
        return;
    }

    if (SAL_TRUE != stSvaTransDbgPrm.bInit)
    {
        SAL_ERROR("sva trans dbg prm is not initialized! \n");
        return;
    }

    pstDbgGroupPrm = stSvaTransDbgPrm.pstSvaDbgGroupPrm[enType];
    if (NULL == pstDbgGroupPrm || u32SubIdx >= pstDbgGroupPrm->u32SubCnt)
    {
        SAL_ERROR("ptr null or invalid subidx! type %d, idx %d \n", enType, u32SubIdx);
        return;
    }

    pstDbgGroupPrm->stTransDbgGroupInfo[u32SubIdx].u32DbgTotalCalTime += (u32EndTime - u32StartTime);
    return;
}

/**
 * @function:   Sva_DrvInitTransDbgPrm
 * @brief:     初始化全局参数
 * @param[in]:  SVA_TRANS_DBG_PRM_S *pstSvaTransDbgPrm
 * @param[out]: SVA_TRANS_DBG_PRM_S *pstSvaTransDbgPrm
 * @return:  INT32
 */
static INT32 Sva_DrvInitTransDbgPrm(SVA_TRANS_DBG_PRM_S *pstSvaTransDbgPrm)
{
    if (NULL == pstSvaTransDbgPrm)
    {
        SAL_ERROR("ptr null! \n");
        return SAL_FAIL;
    }

    if (SAL_TRUE == pstSvaTransDbgPrm->bInit)
    {
        SAL_WARN("sva trans dbg prm is initialized! return success! \n");
        return SAL_SOK;
    }

    pstSvaTransDbgPrm->pstSvaDbgGroupPrm[SVA_TRANS_DBG_MASTER_YUV_TYPE] = &stSvaTransDbgMstYuvGrpPrm;
    pstSvaTransDbgPrm->pstSvaDbgGroupPrm[SVA_TRANS_DBG_SLAVE_YUV_TYPE] = &stSvaTransDbgSlvYuvGrpPrm;

    pstSvaTransDbgPrm->pstSvaDbgGroupPrm[SVA_TRANS_DBG_MASTER_PARAM_TYPE] = &stSvaTransDbgMstPrmGrpPrm;
    pstSvaTransDbgPrm->pstSvaDbgGroupPrm[SVA_TRANS_DBG_SLAVE_PARAM_TYPE] = &stSvaTransDbgSlvPrmGrpPrm;

    pstSvaTransDbgPrm->pstSvaDbgGroupPrm[SVA_TRANS_DBG_MASTER_RESULT_TYPE] = &stSvaTransDbgMstRsltGrpPrm;
    pstSvaTransDbgPrm->pstSvaDbgGroupPrm[SVA_TRANS_DBG_SLAVE_RESULT_TYPE] = &stSvaTransDbgSlvRsltGrpPrm;

    pstSvaTransDbgPrm->bInit = SAL_TRUE;

    return SAL_SOK;
}

/**
 * @function    Sva_DrvSlaveThrCalDbgTime
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvSlaveThrCalDbgTime(VOID)
{
    (VOID)Sva_DrvTransDbgAddCalCnt(SVA_TRANS_DBG_SLAVE_YUV_TYPE);
    (VOID)Sva_DrvTransDbgPrTime(SVA_TRANS_DBG_SLAVE_YUV_TYPE);
    (VOID)Sva_DrvTransDbgClr(SVA_TRANS_DBG_SLAVE_YUV_TYPE);

    (VOID)Sva_DrvTransDbgAddCalCnt(SVA_TRANS_DBG_SLAVE_PARAM_TYPE);
    (VOID)Sva_DrvTransDbgPrTime(SVA_TRANS_DBG_SLAVE_PARAM_TYPE);
    (VOID)Sva_DrvTransDbgClr(SVA_TRANS_DBG_SLAVE_PARAM_TYPE);

    return;
}

/**
 * @function    Sva_DrvMasterThrCalDbgTime
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvMasterThrCalDbgTime(VOID)
{
    (VOID)Sva_DrvTransDbgAddCalCnt(SVA_TRANS_DBG_MASTER_YUV_TYPE);
    (VOID)Sva_DrvTransDbgPrTime(SVA_TRANS_DBG_MASTER_YUV_TYPE);
    (VOID)Sva_DrvTransDbgClr(SVA_TRANS_DBG_MASTER_YUV_TYPE);

    (VOID)Sva_DrvTransDbgAddCalCnt(SVA_TRANS_DBG_MASTER_PARAM_TYPE);
    (VOID)Sva_DrvTransDbgPrTime(SVA_TRANS_DBG_MASTER_PARAM_TYPE);
    (VOID)Sva_DrvTransDbgClr(SVA_TRANS_DBG_MASTER_PARAM_TYPE);

    return;
}

/**
 * @function    Sva_DrvSlaveRsltCalDbgTime
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvSlaveRsltCalDbgTime(VOID)
{
    (VOID)Sva_DrvTransDbgAddCalCnt(SVA_TRANS_DBG_SLAVE_RESULT_TYPE);
    (VOID)Sva_DrvTransDbgPrTime(SVA_TRANS_DBG_SLAVE_RESULT_TYPE);
    (VOID)Sva_DrvTransDbgClr(SVA_TRANS_DBG_SLAVE_RESULT_TYPE);

    return;
}

/**
 * @function    Sva_DrvMasterRsltCalDbgTime
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvMasterRsltCalDbgTime(VOID)
{
    (VOID)Sva_DrvTransDbgAddCalCnt(SVA_TRANS_DBG_MASTER_RESULT_TYPE);
    (VOID)Sva_DrvTransDbgPrTime(SVA_TRANS_DBG_MASTER_RESULT_TYPE);
    (VOID)Sva_DrvTransDbgClr(SVA_TRANS_DBG_MASTER_RESULT_TYPE);

    return;
}

/*******************************************************************************
 * 函数名  : SVA_DrvGetAiSrcPrm
 * 描  述  : 获取镜像参数变量
 * 输  入  : 无
 * 输  出  : 对应的通道镜像参数
 * 返回值  : 无
 ********************************************************************************/
SVA_AI_SRC_PRM *SVA_DrvGetAiSrcPrm(UINT32 chan)
{
    return &stAiSrcPrm[chan];
}

/*******************************************************************************
* 函数名  : Sva_DrvGetDev
* 描  述  : 获取通道全局变量
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
SVA_DEV_PRM *Sva_DrvGetDev(UINT32 chan)
{
    if (chan > (SVA_DEV_MAX - 1))
    {
        SVA_LOGE("Chan %d (Illegal parameters)\n", chan);
        return NULL;
    }

    return &g_sva_common.stSva_dev[chan];
}

/**
 * @function:   Sva_DrvGetDupHandle
 * @brief:      获取智能通道对应的dup handle
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static VOID *
 */
static VOID *Sva_DrvGetDupHandle(UINT32 chan)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, NULL);

    return pstSva_dev->DupHandle.pHandle;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetInitFlag
* 描  述  : 获取初始化标志
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static UINT32 Sva_DrvGetInitFlag(void)
{
    return uiInitFlag;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetInitFlag
* 描  述  : 设置初始化标志
* 输  入  : - UINT32 uiFlag: 初始化标志
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static void Sva_DrvSetInitFlag(UINT32 uiFlag)
{
    if (uiFlag > SAL_TRUE)
    {
        SVA_LOGE("Invalid flag %d \n", uiFlag);
        return;
    }

    uiInitFlag = uiFlag;
    return;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetProcFlag
* 描  述  : 获取模式切换状态标志
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static UINT32 Sva_DrvGetProcFlag(void)
{
    return uiProcModeFlag;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetProcFlag
* 描  述  : 设置模式切换状态标志
* 输  入  : - UINT32 uiFlag: 初始化标志
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static void Sva_DrvSetProcFlag(UINT32 uiFlag)
{
    if (uiFlag > SAL_TRUE)
    {
        SVA_LOGE("Invalid flag %d \n", uiFlag);
        return;
    }

    uiProcModeFlag = uiFlag;
    return;
}

/**
 * @function:   Sva_DrvGetSlavePrm
 * @brief:      获取从片相关参数
 * @param[in]:  void
 * @param[out]: None
 * @return:     SVA_SLAVE_PRM *
 */
SVA_SLAVE_PRM *Sva_DrvGetSlavePrm(UINT32 chan)
{
    return &g_sva_common.stSlavePrm[chan];
}

/*******************************************************************************
* 函数名  : Sva_drvChnWait
* 描  述  :
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static void *Sva_drvChnWait(SVA_DEV_PRM *pstSva_dev)
{
    SVA_DRV_CHECK_PRM(pstSva_dev, NULL);

    SAL_mutexLock(pstSva_dev->mChnMutexHdl);

    SAL_mutexWait(pstSva_dev->mChnMutexHdl);

    SAL_mutexUnlock(pstSva_dev->mChnMutexHdl);

    return NULL;
}

/*******************************************************************************
* 函数名  : Sva_drvChnSignal
* 描  述  :
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static void *Sva_drvChnSignal(void *prm)
{
    SVA_DEV_PRM *pstSva_dev = (SVA_DEV_PRM *)prm;

    SVA_DRV_CHECK_PRM(pstSva_dev, NULL);

    SAL_mutexLock(pstSva_dev->mChnMutexHdl);

    SAL_mutexSignal(pstSva_dev->mChnMutexHdl);

    SAL_mutexUnlock(pstSva_dev->mChnMutexHdl);

    return NULL;
}

/**
 * @function:   Sva_DrvGetCurMs
 * @brief:      获取当前时间(ms)
 * @param[in]:  DEBUG_LEVEL_E enLevel
 * @param[in]:  UINT32 *pTime
 * @param[out]: None
 * @return:     void
 */
void Sva_DrvGetCurMs(DEBUG_LEVEL_E enLevel, UINT32 *pTime)
{
    if (Sva_HalGetDbgLevel() < enLevel)
    {
        return;
    }

    *pTime = SAL_getCurMs();
    return;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetSvaOffsetBufTab
* 描  述  : 获取通道全局变量
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : 成功: 返回偏移量的缓存表指针
*           失败: 返回NULL
*******************************************************************************/
SVA_OFFSET_BUF_TAB_S *Sva_DrvGetSvaOffsetBufTab(UINT32 chan)
{
    SVA_DRV_CHECK_CHAN(chan, NULL);

    return &gstOffsetBufTab[chan];
}

/*******************************************************************************
* 函数名  : Sva_DrvGetSvaFromPool
* 描  述  : 获取当前智能检测结果
* 输  入  : - chan: 通道号
*                : - uiTimeRef: 搜索时用于匹配的帧号
*                : - *pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetSvaFromPool(UINT32 chan, UINT32 uiTimeRef, SVA_PROCESS_OUT *pstSvaOut)
{
    /* 入参有效性检验 */
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSvaOut, SAL_FAIL);

    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    if (NULL == pstSva_dev)
    {
        SVA_LOGE("pstSva_dev == NULL! \n");
        return SAL_FAIL;
    }

    /* 获取Hal层当前智能结果 */
    Sva_HalGetFromPool(chan, uiTimeRef, pstSvaOut);

#if 0
    /* 更新功能性参数，这部分参数由应用配置需动态更新 */
    pstSvaOut->stTargetPrm.confidence = pstSva_dev->confidence;
    pstSvaOut->stTargetPrm.scaleLevel = pstSva_dev->scaleLevel;
    pstSvaOut->stTargetPrm.color_cnt = pstSva_dev->stIn.sva_color_cnt;
    pstSvaOut->stTargetPrm.name_cnt = pstSva_dev->stIn.sva_name_cnt;
#endif

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetCfgParam
* 描  述  : 获取外部配置的智能模块参数
* 输  入  : - chan: 通道号
*         : - *pOut: 外部配置的智能参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetCfgParam(UINT32 chan, SVA_PROCESS_IN *pOut)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pOut, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    /* 拷贝模块外部配置参数 */
    sal_memcpy_s(pOut, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetTargetRectByPack
* 描  述  : 获取基于包裹坐标
* 输  入  : - chan: 通道号
*                : - *pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Sva_DrvGetTargetRectByPack(UINT32 chan, SVA_PROCESS_OUT *pstSvaOut)
{
	UINT32 i = 0;

    SVA_DRV_CHECK_PRM(pstSvaOut, SAL_FAIL);
	
    for (i = 0; i < pstSvaOut->target_num; i++)
    {
		/*基于包裹坐标计算，包裹坐标是计算基于包裹坐标的归一化坐标*/
		pstSvaOut->raw_target[i].rect.x = (float)(pstSvaOut->target[i].rect.x * (float) SVA_MODULE_WIDTH) / (float)pstSvaOut->uiImgW;
        pstSvaOut->raw_target[i].rect.width = (float)(pstSvaOut->target[i].rect.width * (float) SVA_MODULE_WIDTH) / (float)pstSvaOut->uiImgW;
        pstSvaOut->raw_target[i].rect.y = (float)(pstSvaOut->target[i].rect.y * (float)SVA_MODULE_HEIGHT)/ (float)pstSvaOut->uiImgH;
        pstSvaOut->raw_target[i].rect.height = (float)(pstSvaOut->target[i].rect.height * (float)SVA_MODULE_HEIGHT)/ (float)pstSvaOut->uiImgH;

        SVA_LOGD("chan %d, id_%d, i_%d, uiTimeRef_%d, type_%d, target[x_%f y_%f w_%f h_%f], raw_target[x_%f y_%f w_%f h_%f] offset_%d, imgW_%d, imgH_%d\n", \
                 chan, pstSvaOut->target[i].ID, i, pstSvaOut->uiTimeRef, pstSvaOut->target[i].type, \
                 pstSvaOut->target[i].rect.x, pstSvaOut->target[i].rect.y, pstSvaOut->target[i].rect.width, pstSvaOut->target[i].rect.height, \
                 pstSvaOut->raw_target[i].rect.x, pstSvaOut->raw_target[i].rect.y, pstSvaOut->raw_target[i].rect.width, pstSvaOut->raw_target[i].rect.height, \
                 pstSvaOut->uiOffsetX, pstSvaOut->uiImgW, pstSvaOut->uiImgH);
    }

    return SAL_SOK;
}


/**
 * @function   Sva_DrvSetPrOutInfoFlag
 * @brief      设置控制参数，打印智能输出结果
 * @param[in]  UINT32 u32Flag
 * @param[out] None
 * @return     VOID
 */
VOID Sva_DrvSetPrOutInfoFlag(UINT32 u32Flag)
{
    g_bPrOutInfo = u32Flag;

    if (1 == g_bPrOutInfo || 3 <= g_bPrOutInfo)
    {
        SVA_LOGW("=== set print engine out info [%d]! \n", g_bPrOutInfo);
    }
    
    if (2 == g_bPrOutInfo || 3 <= g_bPrOutInfo)
    {
        SVA_LOGW("=== set print sva out info [%d]! \n", g_bPrOutInfo);
    }
    
}
static VOID Sva_DrvDrawRectCPU(CHAR *pcFrmDataY, UINT32 uiFrmW, UINT32 uiFrmH,
                               UINT32 uiRectX, UINT32 uiRectY, UINT32 uiRectW, UINT32 uiRectH);
/**
 * @function   Sva_DrvPrOutInfo
 * @brief      打印智能输出结果（当前仅安检机在用）
 * @param[in]  UINT32 chan
 * @param[in]  SVA_PROCESS_OUT *pstSvaOut
 * @param[out] None
 * @return     static VOID
 */
static VOID Sva_DrvPrOutInfo(UINT32 chan, SVA_PROCESS_OUT *pstSvaOut)
{
    if (0 == g_bPrOutInfo || 1 == g_bPrOutInfo)
    {
        return;
    }

    UINT32 i = 0;
    UINT32 w = 0;
    UINT32 h = 0;
    UINT32 x = 0;
    UINT32 y = 0;
	
    CHAR acPath[64] = {0};
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_LOGW("========== print sva [%d] out info! \n", chan);

    for (i = 0; i < pstSvaOut->target_num; i++)
    {
        SVA_LOGW("target: i %d, type %d, conf %d, [%f, %f] [%f, %f] \n", i,
                 pstSvaOut->target[i].type, pstSvaOut->target[i].confidence,
                 pstSvaOut->target[i].rect.x, pstSvaOut->target[i].rect.y,
                 pstSvaOut->target[i].rect.width, pstSvaOut->target[i].rect.height);
    }
    /*大于3，则保存智能结果画框后的yuv数据*/
    if (3 < g_bPrOutInfo)
    {
        SVA_LOGW("========== dump sva [%d] out info YUV! \n", chan);
        for (i = 0; i < pstSvaOut->target_num; i++)
        {
            x = (UINT32)(pstSvaOut->target[i].rect.x * (float)SVA_MODULE_WIDTH);
            y = (UINT32)(pstSvaOut->target[i].rect.y * (float)SVA_MODULE_HEIGHT);
            w = (UINT32)(pstSvaOut->target[i].rect.width * (float)SVA_MODULE_WIDTH);
            h = (UINT32)(pstSvaOut->target[i].rect.height * (float)SVA_MODULE_HEIGHT);
            (VOID)Sva_DrvDrawRectCPU((VOID *)pstSva_dev->stSysFrame.uiDataAddr, SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, x, y, w, h);
        }
        snprintf(acPath, 64, "%s/result_frm_%llu_ch_%d_w_%d_h_%d.nv21", INPUT_IMG_DUMP_PATH, pstSvaOut->frame_stamp, chan, SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT);
        Ia_DumpYuvData(acPath, (CHAR *)pstSva_dev->stSysFrame.uiDataAddr, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2);
}
}

/**
 * @function    Sva_DrvISMDealResQueueOut
 * @brief         安检机输出结果处理接口
 * @param[in]   chan-通道号
 * @param[out] pstSvaOut-安检机处理后的智能处理结果
 * @return  SAL_SOK
 */
static INT32 Sva_DrvISMDealResQueueOut(UINT32 chan, SVA_PROCESS_OUT *pstSvaOut)
{
    /*若是从片则不需要给xpack送结果*/
    if(DOUBLE_CHIP_SLAVE_TYPE == g_sva_common.uiDevType)
	{
		return SAL_SOK;
    }
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSvaOut, SAL_FAIL);
    SVA_PROCESS_OUT_DATA *pstSvaPackProcessData = NULL;   /*输出数据信息*/
    SVA_DEV_PRM *pstSva_dev = NULL;
    DSA_QueHndl *pstSvaPackInfoFullQueue = NULL;          /*输出包裹信息队列*/
    DSA_QueHndl *pstSvaPackInfoEmptQueue = NULL;          /*输出包裹信息队列*/
    UINT32 uiQueueCnt = 0;
    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);
    pstSvaPackInfoFullQueue = &pstSva_dev->stOutPackageDataFullQue;
    pstSvaPackInfoEmptQueue = &pstSva_dev->stOutPackageDataEmptQue;
    SVA_LOGI("------------Get result !!chan %d target_num %d packIndex %d top %d endFlag %d frame_stamp %llu-----------\n", chan, pstSvaOut->target_num, \
             pstSvaOut->aipackinfo.packIndx, pstSvaOut->aipackinfo.packTop, pstSvaOut->aipackinfo.completePackMark, pstSvaOut->frame_stamp);
#if 0
    /*获取智能过滤后的结果，若没有目标，则down图片*/
    INT8 framename[64] = {0};
    if (0 == pstSvaOut->target_num && NULL != (CHAR *)pstSva_dev->stSysFrame.uiDataAddr)
    {
        sprintf((CHAR *)framename, "%d_%d_noresult.yuv", chan, pstSvaOut->uiTimeRef);
        Sva_HalDebugDumpData((CHAR *)pstSva_dev->stSysFrame.uiDataAddr, (CHAR *)framename, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2, 0);
    }

#endif
    /* 打印智能结果 */
    Sva_DrvPrOutInfo(chan, pstSvaOut);

    /*输出包裹数据队列*/

    uiQueueCnt = DSA_QueGetQueuedCount(pstSvaPackInfoEmptQueue);
    if (uiQueueCnt)
    {

        DSA_QueGet(pstSvaPackInfoEmptQueue, (void **)&pstSvaPackProcessData, SAL_TIMEOUT_FOREVER);
        sal_memset_s(&pstSvaPackProcessData->uiSvaProcessRes, sizeof(SVA_PROCESS_OUT), 0x00, sizeof(SVA_PROCESS_OUT));
        sal_memcpy_s(&pstSvaPackProcessData->uiSvaProcessRes, sizeof(SVA_PROCESS_OUT), pstSvaOut, sizeof(SVA_PROCESS_OUT));
#ifdef DSP_ISM
        if (SAL_SOK != Sva_DrvGetTargetRectByPack(chan, &(pstSvaPackProcessData->uiSvaProcessRes)))
        {
            SVA_LOGE("Sva Rect Chg Failed! chan %d \n", chan);
            return SAL_FAIL;
        }

#endif
        /*输出数据check*/
        /* (void)Sva_DrvCheckResultPos(&pstSvaPackProcessData->uiSvaProcessRes); */

        sal_memset_s(&pstSvaPackProcessData->uiSvaPackPrm, sizeof(AI_PACK_PRM), 0x00, sizeof(AI_PACK_PRM));
        sal_memcpy_s(&pstSvaPackProcessData->uiSvaPackPrm, sizeof(AI_PACK_PRM), &(pstSvaOut->aipackinfo), sizeof(AI_PACK_PRM));
        pstSvaPackProcessData->uiSvaPackPrm.timeRef = pstSvaOut->uiTimeRef;
        DSA_QuePut(pstSvaPackInfoFullQueue, (void *)pstSvaPackProcessData, SAL_TIMEOUT_NONE);
    }
    else
    {
        SVA_LOGE("Dis Que is empty!!\n");
    }

    pstSvaOut->packbagAlert.package_valid = SAL_TRUE;
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetSvaOut
* 描  述  : 获取当前智能检测结果
* 输  入  : - chan: 通道号
*                : - *pstSvaOut: 智能结果
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetSvaOut(UINT32 chan, SVA_PROCESS_OUT *pstSvaOut)
{
    SVA_DEV_PRM *pstSva_dev = NULL;
    CAPB_PRODUCT *capb_product = capb_get_product();

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSvaOut, SAL_FAIL);
    SVA_DRV_CHECK_PRM(capb_product, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    if (NULL == pstSva_dev)
    {
        SVA_LOGE("pstSva_dev == NULL! \n");
        return SAL_FAIL;
    }

    /* 获取Hal层当前智能结果 */
    (VOID)Sva_HalGetSvaOut(chan, pstSvaOut);
    /* 临时增加 ,后期全部挪到其他模块做 */
    if (capb_product->enInputType == VIDEO_INPUT_INSIDE)
    {
#ifdef DSP_ISM
        (void)Sva_DrvGetTargetRectByPack(chan, pstSvaOut);
#endif
    }

#if 0
    /* 更新功能性参数，这部分参数由应用配置需动态更新 */
    pstSvaOut->stTargetPrm.confidence = pstSva_dev->confidence;
    pstSvaOut->stTargetPrm.scaleLevel = pstSva_dev->scaleLevel;
    pstSvaOut->stTargetPrm.color_cnt = pstSva_dev->stIn.sva_color_cnt;
    pstSvaOut->stTargetPrm.name_cnt = pstSva_dev->stIn.sva_name_cnt;

    pstSvaOut->drawType = pstSva_dev->enOsdBorderType;
#endif

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvChangeAdaptor
* 描  述  : 安检机因子转换
* 输  入  : - enSize: 通道大小
*         : - fScale: 缩放因子
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 该接口用于智能分析仪产品，对接不同通道大小的安检机使用
*******************************************************************************/
INT32 Sva_DrvChangeAdaptor(SVA_MACHINE_TYPE enSize, float *fScale)
{
    if (enSize > SVA_FRCNN_MACHINE_NUM - 1)
    {
        *fScale = 1.914;
        goto exit;
    }

    /* 当前分析仪和安检机送入引擎的下采样率都与机型无关，故此处均使用1.0f用于精简全局变量g_fDownScaleSizeTab */
    *fScale = 1.0f; /* g_fDownScaleSizeTab[IA_ANALYZER_TYPE][enSize]; */

exit:
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvUpdataPrm
* 描  述  : 更新参数
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_UNUSED INT32 Sva_DrvUpdataPrm(UINT32 chan)
{
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SVA_LOGE("not enought pstPrmEmptQue\n");
        return SAL_SOK;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSaveProcMode
* 描  述  : 保存处理模式
* 输  入  : - prm : 处理模式
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注意点  : 模式切换需要退出引擎资源(句柄等)，然后设置模式后重新创建引擎句柄
*******************************************************************************/
INT32 Sva_DrvSaveProcMode(void *prm)
{
    g_proc_mode = *(SVA_PROC_MODE_E *)prm;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetDetectRegion
* 描  述  : 设置检测区域
* 输  入  : - chan: 通道号
*         : - prm : 检测区域
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetDetectRegion(UINT32 chan, VOID *prm)
{
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    SVA_RECT_F *sva_rect = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    sva_rect = (SVA_RECT_F *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if ((sva_rect->x < 0.0f || sva_rect->x > 1.0f)
        || (sva_rect->y < 0.0f || sva_rect->y > 1.0f)
        || (sva_rect->width < 0.0f || sva_rect->width > 1.0f)
        || (sva_rect->height < 0.0f || sva_rect->height > 1.0f))
    {
        SVA_LOGE("Illegal parameters (%.4f ,%.4f) (%.4f ,%.4f) \n", sva_rect->x, sva_rect->y, sva_rect->width, sva_rect->height);
        return SAL_FAIL;
    }

    /* 传区域信息 */
    pstSva_dev->stIn.rect.x = sva_rect->x;
    pstSva_dev->stIn.rect.y = sva_rect->y;
    pstSva_dev->stIn.rect.width = sva_rect->width;
    pstSva_dev->stIn.rect.height = sva_rect->height;

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SVA_LOGE("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SVA_LOGI("chan %d Set Region (%.4f ,%.4f) (%.4f ,%.4f)\n", chan, sva_rect->x, sva_rect->y, sva_rect->width, sva_rect->height);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetAlertSwitch
* 描  述  : 单个危险物检测开关
* 输  入  : - chan: 通道号
*         : - prm : 开关
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetAlertSwitch(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SVA_ALERT_KEY *key = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    key = (SVA_ALERT_KEY *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    for (i = 0; i < key->sva_cnt; i++)
    {
        if ((key->sva_type[i] > (SVA_MAX_ALARM_TYPE - 1)) || (key->sva_key[i] > 1))
        {
            SVA_LOGE("chan %d: type (%d) or key (%d) is Illegal !\n", chan, key->sva_type[i], key->sva_key[i]);
            return SAL_FAIL;
        }

        /* 危险物开关 */
        if (pstSva_dev->stIn.alert[key->sva_type[i]].sva_key == key->sva_key[i])
        {
            SVA_LOGI("chan %d Alert Type %d Same Key %d \n", chan, key->sva_type[i], key->sva_key[i]);
            continue;
        }

        pstSva_dev->stIn.alert[key->sva_type[i]].sva_key = key->sva_key[i];

        SVA_LOGI("chan %d Alert Type %d Key %d \n", chan, key->sva_type[i], key->sva_key[i]);
    }

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SVA_LOGI("Set Key End! chan %d\n", chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetAlertTarNumThresh
 * @brief:      设置危险品告警个数阈值
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetAlertTarNumThresh(UINT32 chan, VOID *prm)
{
    UINT32 i;
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    SVA_ALERT_TARGET_CNT *pstAlertTarCntInfo = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pstAlertTarCntInfo = (SVA_ALERT_TARGET_CNT *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    for (i = 0; i < pstAlertTarCntInfo->sva_cnt; i++)
    {
        if (pstAlertTarCntInfo->sva_type[i] > (SVA_MAX_ALARM_TYPE - 1))
        {
            SVA_LOGE("Type (%d) is Illegal !\n", pstAlertTarCntInfo->sva_type[i]);
            return SAL_FAIL;
        }

        /* 危险物告警个数阈值相同 */
        if (pstSva_dev->stIn.alert[pstAlertTarCntInfo->sva_type[i]].sva_alert_tar_cnt == pstAlertTarCntInfo->sva_alert_tar_cnt[i])
        {
            continue;
        }

        pstSva_dev->stIn.alert[pstAlertTarCntInfo->sva_type[i]].sva_alert_tar_cnt = pstAlertTarCntInfo->sva_alert_tar_cnt[i];
        SVA_LOGD("chan %d Alert Type %d alert tar cnt %d \n", chan, pstAlertTarCntInfo->sva_type[i], pstAlertTarCntInfo->sva_alert_tar_cnt[i]);
    }

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QueGet(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetAlertColor
* 描  述  : 设置危险物颜色
* 输  入  : - chan: 通道号
*         : - prm : 颜色
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetAlertColor(UINT32 chan, VOID *prm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    OSD_SET_ARRAY_S stOsdArray;
    OSD_SET_PRM_S *pstOsdPrm = NULL;

    SVA_ALERT_COLOR *color = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    color = (SVA_ALERT_COLOR *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    stOsdArray.u32StringNum = 0;
    stOsdArray.pstOsdPrm = SAL_memMalloc(sizeof(OSD_SET_PRM_S) * color->sva_cnt, "SVA", "sva_osdprm");
    if (NULL == stOsdArray.pstOsdPrm)
    {
        SAL_ERROR("malloc osd array buff fail\n");
        return SAL_FAIL;
    }

    (VOID) osd_func_writeStart(); /* 扩大加锁范围，确保配置信息中的颜色和点阵颜色同步 */

    for (i = 0; i < color->sva_cnt; i++)
    {
        if (color->sva_type[i] > (SVA_MAX_ALARM_TYPE - 1))
        {
            SVA_LOGE("Type (%d) is Illegal !\n", color->sva_type[i]);
            SAL_memfree(stOsdArray.pstOsdPrm, "SVA", "sva_osdprm");
            return SAL_FAIL;
        }

        /* 危险物开关 */
        if (pstSva_dev->stIn.alert[color->sva_type[i]].sva_color == color->sva_color[i])
        {
            SVA_LOGI("chan %d Alert Type %d Same Color 0x%x \n", chan, color->sva_type[i], color->sva_color[i]);
            continue;
        }

        pstSva_dev->stIn.alert[color->sva_type[i]].sva_color = color->sva_color[i];

        /* 更新OSD字体颜色 */
        pstOsdPrm = stOsdArray.pstOsdPrm + stOsdArray.u32StringNum++;
        pstOsdPrm->u32Idx = color->sva_type[i];
        pstOsdPrm->szString = NULL;
        pstOsdPrm->u32Color = pstSva_dev->stIn.alert[color->sva_type[i]].sva_color;
        pstOsdPrm->u32BgColor = OSD_COLOR_INVALID;

        SVA_LOGI("chan %d Alert Type %d Color 0x%x \n", chan, color->sva_type[i], color->sva_color[i]);
    }

    s32Ret = osd_func_blockSet(OSD_BLOCK_IDX_STRING, &stOsdArray);
    (VOID)osd_func_writeEnd();
    SAL_memfree(stOsdArray.pstOsdPrm, "SVA", "sva_osdprm");
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("update osd color fail\n");
    }

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    pstSva_dev->stIn.sva_color_cnt++;      /* 记录颜色配置次数 */

    SVA_LOGI("Set Sva Color End! chan %d\n ", chan);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetAlertConf
* 描  述  : 设置检测置信度
* 输  入  : - chan: 通道号
*         : - prm : 置信度
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetAlertConf(UINT32 chan, VOID *prm)
{
	INT32 s32Ret = SAL_SOK;
	SVA_ALERT_CONF *pstConf = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pstConf = (SVA_ALERT_CONF *)prm;

	/*若为双芯片中的主片，则直接返回成功*/
	if(DOUBLE_CHIP_MASTER_TYPE == g_sva_common.uiDevType)
	{
	    SVA_LOGW("Double Chip!!! The SVA engine of master chip is not inited (chan %d)\n", chan);
		return SAL_SOK;
	}
	s32Ret = Sva_HalSetAlertConf(chan, pstConf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Sva_HalSetAlertConf Failed! chan %d \n", chan);
        return SAL_FAIL;
    }  
    SVA_LOGI("Set Conf End! chan %d\n", chan);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetDectSwitch
* 描  述  : 危险物总开关
* 输  入  : - chan: 通道号
*         : - prm : 总开关
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetDectSwitch(UINT32 chan, VOID *prm)
{
    UINT32 *uiValue = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    uiValue = (UINT32 *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (*uiValue > 1)
    {
        SVA_LOGE("Invalid prm: %d, Pls Check!\n", *uiValue);
        return SAL_FAIL;
    }

    /* 危险物总开关 */
    if (pstSva_dev->uiSwitch == *uiValue)
    {
        SVA_LOGI("chan %d Alert Switch %d \n", chan, pstSva_dev->uiSwitch);
        return SAL_SOK;
    }

    if (*uiValue != SAL_FALSE)
    {
        pstSva_dev->uiSwitch = SAL_TRUE;
    }
    else
    {
        pstSva_dev->uiSwitch = SAL_FALSE;
    }

    SVA_LOGI("chan %d Alert Switch %d \n", chan, pstSva_dev->uiSwitch);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetChnSize
* 描  述  : 设置安检机通道大小
* 输  入  : - chan: 通道号
*         : - prm : 通道大小
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetChnSize(UINT32 chan, VOID *prm)
{
    UINT32 uiQueueCnt = 0;

    UINT32 *pSvaSize = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SVA_PROCESS_IN *pstIn = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pSvaSize = (UINT32 *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (*pSvaSize > SVA_FRCNN_1000_1000)
    {
        SVA_LOGE("Illegal chnSize %d \n", *pSvaSize);
        return SAL_FAIL;
    }

    if (pstSva_dev->enChnSize == *pSvaSize)
    {
        SVA_LOGI("chan %d chnSize %d uiScale %f \n", chan, pstSva_dev->enChnSize, pstSva_dev->stIn.fScale);
        return SAL_SOK;
    }

    pstSva_dev->enChnSize = *pSvaSize;

    Sva_DrvChangeAdaptor(pstSva_dev->enChnSize, &pstSva_dev->stIn.fScale);

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SVA_LOGE("not enought pstPrmEmptQue\n");
        return SAL_SOK;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SVA_LOGI("chan %d chnSize %d uiScale %f \n", chan, pstSva_dev->enChnSize, pstSva_dev->stIn.fScale);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetOsdExtType
* 描  述  : 设置OSD拓展信息展示类型
* 输  入  : - chan: 通道号
*         : - prm : 置信度开关
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetOsdExtType(UINT32 chan, VOID *prm)
{
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SVA_ALERT_EXT_TYPE enOsdExtType = 0;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    enOsdExtType = *(SVA_ALERT_EXT_TYPE *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (pstSva_dev->stIn.stTargetPrm.enOsdExtType == enOsdExtType)
    {
        SVA_LOGI("chan %d confidence %d \n", chan, pstSva_dev->stIn.stTargetPrm.enOsdExtType);
        return SAL_SOK;
    }

    pstSva_dev->stIn.stTargetPrm.enOsdExtType = enOsdExtType;

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SVA_LOGI("chan %d osd_type %d \n", chan, pstSva_dev->stIn.stTargetPrm.enOsdExtType);
    return SAL_SOK;
}

/**
 * @function    Sva_DrvInitDbgGlobalVar
 * @brief       模块全局维护变量初始化
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvInitDbgGlobalVar(VOID)
{
    /* 模块通用状态变量初始化 */
    sal_memset_s(&g_sva_common.stDbgPrm.stCommSts, sizeof(SVA_DBG_COMM_STS_S), 0x00, sizeof(SVA_DBG_COMM_STS_S));

    /* 算法状态变量初始化 */
    sal_memset_s(&g_sva_common.stDbgPrm.stAlgSts, sizeof(SVA_DBG_ALG_STS_S), 0x00, sizeof(SVA_DBG_ALG_STS_S));

    g_sva_common.stDbgPrm.stAlgSts.uiMinMs = (UINT32)(-1);  /* 初始化为无符号最大值 */

    /* 清空hal层维护变量 */
    Sva_HalResetDbgInfo();

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetProcMode
 * @brief:      执行模式切换
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Sva_DrvSetProcMode(void *prm)
{
    SVA_VCAE_PRM_ST *pstVacePrm = NULL;

    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pstVacePrm = (SVA_VCAE_PRM_ST *)prm;

    SVA_LOGI("from app: %p, proc Mode %d, aiEnable %d \n", (void *)pstVacePrm, pstVacePrm->enProcMode, pstVacePrm->uiAiEnable);

    /* sva模块使能且单芯片运行 or sva模块使能且在双芯片从片上使能 */
    if (SAL_TRUE == IA_GetAlgRunFlag(IA_MOD_SVA))
    {
        /* 切换模式需要重置模块全局维护信息 */
        (VOID)Sva_DrvInitDbgGlobalVar();

        if (SAL_SOK != Sva_HalSetVcaePrm((void *)pstVacePrm))
        {
            SVA_LOGE("Set Proc Mode %d, Ai Flag %d Fail! Pls Check.\n", pstVacePrm->enProcMode, pstVacePrm->uiAiEnable);
            return SAL_FAIL;
        }
    }

    /* after success, Save Proc Mode */
    g_sva_common.stEngProcPrm.enProcMode = pstVacePrm->enProcMode;
    g_sva_common.stEngProcPrm.uiAiEnable = pstVacePrm->uiAiEnable;
    g_proc_mode = pstVacePrm->enProcMode;

    SVA_LOGI("Set Proc Mode %d, Ai Flag %d OK!\n", pstVacePrm->enProcMode, pstVacePrm->uiAiEnable);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetProcModeThread
 * @brief:      配置模式切换
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     void *
 */
static void *Sva_DrvSetProcModeThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    SAL_SET_THR_NAME();

    s32Ret = Sva_DrvSetProcMode(prm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Sva_DrvSetProcMode_dbg Failed! \n");
        goto EXIT;
    }

    uiProcRsltSts = SAL_SOK;

EXIT:
    (VOID)Sva_DrvSetProcFlag(SAL_FALSE);
    SVA_LOGI("Sva_DrvSetProcMode End! \n");

    SAL_thrExit(NULL);
    return NULL;
}

/**
 * @function:   Sva_DrvStartSetProcModeThread
 * @brief:      创建模式切换执行线程
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Sva_DrvStartSetProcModeThread(void *prm)
{
    SAL_ThrHndl Handle;

    /* hcnn库中存在1M多的栈内存使用，故此处创建2M的线程栈用于模式切换 */
    if (SAL_SOK != SAL_thrCreate(&Handle, Sva_DrvSetProcModeThread, SAL_THR_PRI_DEFAULT, 2 * 1024 * 1024, prm))
    {
        SVA_LOGE("Create Engine Init Thread Failed! \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != SAL_thrDetach(&Handle))
    {
        SVA_LOGE("pthread detach failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvGetEventStatus
 * @brief:      获取事件状态
 * @param[in]:  SVA_EVENT_TYPE_E enEvent
 * @param[in]:  SVA_EVENT_STS_E *penSts
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Sva_DrvGetEventStatus(SVA_EVENT_TYPE_E enEvent, SVA_EVENT_STS_E *penSts)
{
    SVA_EVENT_STS_E enStatus = 0;

    if (enEvent > SVA_EVENT_TYPE_NUM - 1 || NULL == penSts)
    {
        SAL_ERROR("invalid event type %d, or ptr %p null \n", enEvent, penSts);
        return SAL_FAIL;
    }

    switch (enEvent)
    {
        case SVA_INIT_END_STATUS:
        {
            enStatus = Sva_DrvGetInitFlag() ? SVA_EVENT_STS_OK : SVA_EVENT_STS_FAIL;
            break;
        }
        case SVA_PROC_MODE_STATUS:
        {
            enStatus = Sva_DrvGetProcFlag() ? SVA_EVENT_STS_FAIL : SVA_EVENT_STS_OK;
            break;
        }
        default:
        {
            SAL_WARN("default!!!! \n");
            break;
        }
    }

    *penSts = enStatus;
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvWaitEvent
 * @brief:      等待时间发生
 * @param[in]:  SVA_EVENT_TYPE_E enEvent
 * @param[in]:  UINT32 uiSingleMs
 * @param[in]:  INT32 iWaitCnt
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Sva_DrvWaitEvent(SVA_EVENT_TYPE_E enEvent, UINT32 uiSingleMs, INT32 iWaitCnt)
{
    INT32 iTmpCnt = iWaitCnt;
    SVA_EVENT_STS_E enStatus = 0;

    do
    {
        if ((SAL_SOK == Sva_DrvGetEventStatus(enEvent, &enStatus))
            && (SVA_EVENT_STS_OK == enStatus))
        {
            break;
        }

        /* 休眠单位为s */
        if (iTmpCnt > 0)
        {
            iTmpCnt--;
        }

        usleep(1000 * uiSingleMs);
    }
    while (iTmpCnt > 0);

    if (iWaitCnt > 0 && iTmpCnt <= 0)
    {
        SVA_LOGE("wait event over time! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   Sva_DrvDoProcModeOnce
 * @brief      执行一次底层引擎资源重新创建
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvDoProcModeOnce(VOID *prm)
{
    UINT32 uiTimeOut = 350;              /* 350 * 100ms = 35s */

    /* 判断当前初始化是否结束和当前是否正在切换模式，当前默认最长等待5s */
    if (SAL_TRUE != Sva_DrvGetInitFlag() || SAL_TRUE == Sva_DrvGetProcFlag())
    {
        if (SAL_SOK != Sva_DrvWaitEvent(SVA_INIT_END_STATUS, 100, 50))
        {
            SVA_LOGE("Set Vcae Prm Err! Wait TimeOut! \n");
            return SVA_INIT_PROC_BUSY;
        }

        if (SAL_SOK != Sva_DrvWaitEvent(SVA_PROC_MODE_STATUS, 100, 50))
        {
            SVA_LOGE("Set Vcae Prm Err! Wait TimeOut! \n");
            return SVA_SET_PROC_MODE_BUSY;
        }
    }

    bAlgResChg = SAL_TRUE;

    (VOID)Sva_DrvSetProcFlag(SAL_TRUE);   /* 保护底层资源处理 */

    uiProcRsltSts = SAL_FAIL;

    if (SAL_SOK != Sva_DrvStartSetProcModeThread(prm))
    {
        SVA_LOGD("Sva_DrvStartSetProcModeThread failed! \n");
        (VOID)Sva_DrvSetProcFlag(SAL_FALSE);   /* 释放底层资源处理标志位 */

        return SAL_FAIL;
    }

    while (Sva_DrvGetProcFlag() && uiTimeOut > 0)
    {
        usleep(100 * 1000);

        if (uiTimeOut > 0)
        {
            uiTimeOut--;
        }
    }

    SVA_LOGI("start set proc mode thread end! %d \n", uiTimeOut);
    return uiProcRsltSts;
}

/**
 * @function   Sva_DrvUpgradeModel
 * @brief      模型升级
 * @param[in]  UINT32 chan
 * @param[in]  VOID *prm
 * @param[out] None
 * @return     INT32
 */
INT32 Sva_DrvUpgradeModel(UINT32 chan, VOID *prm)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i, j;

    SVA_MODEL_PARAM_S *pstModelPrm = NULL;
    SVA_MODEL_ATTR_S *pstModelTypeAttr = NULL;
    SVA_DET_MODEL_ATTR_S *pstDetModelAttr = NULL;
    SVA_PD_MODEL_ATTR_S *pstPdModelAttr = NULL;
    SVA_CLS_MODEL_ATTR_S *pstClsModelAttr = NULL;

    IA_ISA_UPDATA_CFG_JSON_PARA stUpJsonPrm = {0};

    if (NULL == prm)
    {
        SVA_LOGE("prm == null! \n");
        goto exit;
    }

    pstModelPrm = (SVA_MODEL_PARAM_S *)prm;

    /* 判断当前初始化是否结束和当前是否正在切换模式，当前默认最长等待5s */
    if (SAL_TRUE != Sva_DrvGetInitFlag() || SAL_TRUE == Sva_DrvGetProcFlag())
    {
        if (SAL_SOK != Sva_DrvWaitEvent(SVA_INIT_END_STATUS, 100, 50))
        {
            SVA_LOGE("Set Vcae Prm Err! Wait TimeOut! \n");
            return SVA_INIT_PROC_BUSY;
        }

        if (SAL_SOK != Sva_DrvWaitEvent(SVA_PROC_MODE_STATUS, 100, 50))
        {
            SVA_LOGE("Set Vcae Prm Err! Wait TimeOut! \n");
            return SVA_SET_PROC_MODE_BUSY;
        }
    }

    SVA_LOGW("get model type num %d \n", pstModelPrm->uiModelTypeNum);

    /* 修改配置文件中的模型名 */
    for (i = 0; i < pstModelPrm->uiModelTypeNum; i++)
    {
        pstModelTypeAttr = &pstModelPrm->astModelTypeAttr[i];

        switch (pstModelTypeAttr->enModelType)
        {
            case SVA_DET_MODEL:
            {
                pstDetModelAttr = &pstModelTypeAttr->uModelAttr.stDetModelAttr;

                /* 修改主视角OBD模型个数及路径 */
                stUpJsonPrm.uiObdModelNum = pstDetModelAttr->uiMainModelNum;
                for (j = 0; j < pstDetModelAttr->uiMainModelNum; j++)
                {
                    stUpJsonPrm.enChgType = CHANGE_MAIN_OBD_MODEL_PATH;
                    stUpJsonPrm.pcObdModelPath[j] = (char *)pstDetModelAttr->chMainModelName[j];

                    SVA_LOGW("obd main: j %d, %s \n", j, stUpJsonPrm.pcObdModelPath[j]);
                }

                s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE("IA_HalModifyCfgFile failed! chan %d \n", chan);
                    return SAL_FAIL;
                }

                /* 修改侧视角OBD模型个数及路径 */
                stUpJsonPrm.uiObdModelNum = pstDetModelAttr->uiSideModelNum;
                for (j = 0; j < pstDetModelAttr->uiSideModelNum; j++)
                {
                    stUpJsonPrm.enChgType = CHANGE_SIDE_OBD_MODEL_PATH;
                    stUpJsonPrm.pcObdModelPath[j] = (char *)pstDetModelAttr->chSideModelName[j];

                    SVA_LOGW("obd side: j %d, %s \n", j, stUpJsonPrm.pcObdModelPath[j]);
                }

                s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE("IA_HalModifyCfgFile failed! chan %d \n", chan);
                    return SAL_FAIL;
                }

                break;
            }

            case SVA_PD_MODEL:
            {
                pstPdModelAttr = &pstModelTypeAttr->uModelAttr.stPdModelAttr;

                /* 修改主视角PD模型个数及路径 */
                stUpJsonPrm.uiPdModelNum = pstPdModelAttr->uiMainModelNum;
                for (j = 0; j < pstPdModelAttr->uiMainModelNum; j++)
                {
                    stUpJsonPrm.enChgType = CHANGE_MAIN_PD_MODEL_PATH;
                    stUpJsonPrm.pcPdModelPath[j] = (char *)pstPdModelAttr->chMainModelName[j];

                    SVA_LOGW("pd main: j %d, %s \n", j, stUpJsonPrm.pcPdModelPath[j]);
                }

                s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE("IA_HalModifyCfgFile failed! chan %d \n", chan);
                    return SAL_FAIL;
                }

                /* 修改侧视角PD模型个数及路径 */
                stUpJsonPrm.uiPdModelNum = pstPdModelAttr->uiSideModelNum;
                for (j = 0; j < pstPdModelAttr->uiSideModelNum; j++)
                {
                    stUpJsonPrm.enChgType = CHANGE_SIDE_PD_MODEL_PATH;
                    stUpJsonPrm.pcPdModelPath[j] = (char *)pstPdModelAttr->chSideModelName[j];

                    SVA_LOGW("pd side: j %d, %s \n", j, stUpJsonPrm.pcPdModelPath[j]);
                }

                s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE("IA_HalModifyCfgFile failed! chan %d \n", chan);
                    return SAL_FAIL;
                }

                break;
            }

            case SVA_CLS_MODEL:
            {
                pstClsModelAttr = &pstModelTypeAttr->uModelAttr.stClsModelAttr;

                /* 修改主视角PD模型个数及路径 */
                stUpJsonPrm.uiClsModelNum = pstClsModelAttr->uiModelNum;
                for (j = 0; j < pstClsModelAttr->uiModelNum; j++)
                {
                    stUpJsonPrm.enChgType = CHANGE_CLS_MODEL_PATH;
                    stUpJsonPrm.pcClsModelPath[j] = (char *)pstClsModelAttr->chName[j];

                    SVA_LOGW("cls: j %d, %s \n", j, stUpJsonPrm.pcClsModelPath[j]);
                }

                s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE("IA_HalModifyCfgFile failed! chan %d \n", chan);
                    return SAL_FAIL;
                }

                break;
            }

            default:
            {
                SVA_LOGW("invalid model type %d \n", pstModelTypeAttr->enModelType);
                continue;
            }
        }
    }

    /* 升级模型后，算法分析模式和AI模式按照上次保存的全局状态进行恢复 */
    stTmpChgProcMode.enProcMode = g_sva_common.stEngProcPrm.enProcMode;
    stTmpChgProcMode.uiAiEnable = g_sva_common.stEngProcPrm.uiAiEnable;

    SVA_LOGI("chg model json end! enProcMode %d, uiAiEnable %d \n",
             stTmpChgProcMode.enProcMode, stTmpChgProcMode.uiAiEnable);

    s32Ret = Sva_DrvDoProcModeOnce(&stTmpChgProcMode);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("do proc mode once failed! \n");
        goto exit;
    }

exit:
    return s32Ret;
}

/*******************************************************************************
* 函数名  : Sva_DrvChgProcMode
* 描  述  : 设置处理模式
* 输  入  : - prm : 处理模式
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvChgProcMode(void *prm)
{
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    memcpy(&stTmpChgProcMode, prm, sizeof(SVA_VCAE_PRM_ST));   /* 保存入参到静态变量 */

    SVA_LOGI("from app: proc Mode %d, aiEnable %d \n", stTmpChgProcMode.enProcMode, stTmpChgProcMode.uiAiEnable);

    if (stTmpChgProcMode.enProcMode >= SVA_PROC_MODE_NUM || stTmpChgProcMode.uiAiEnable > SAL_TRUE)
    {
        SVA_LOGE("Invalid Proc Param, mode %d, ai flag %d \n", stTmpChgProcMode.enProcMode, stTmpChgProcMode.uiAiEnable);
        return SAL_FAIL;
    }

    /* 保存设置的proc mode */
    if (stTmpChgProcMode.enProcMode == g_sva_common.stEngProcPrm.enProcMode
        && stTmpChgProcMode.uiAiEnable == g_sva_common.stEngProcPrm.uiAiEnable)
    {
        SVA_LOGI("Same Proc Mode %d, Ai enable %d, return success!\n", stTmpChgProcMode.enProcMode, stTmpChgProcMode.uiAiEnable);
        return SAL_SOK;
    }

    return Sva_DrvDoProcModeOnce(&stTmpChgProcMode);
}

/*******************************************************************************
* 函数名  : Sva_DrvSetSyncMainChan
* 描  述  : 设置同步模式主视角通道(默认1为主视角，0为辅视角)
* 输  入  : - chan: 通道号
*         : - prm : 主视角通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetSyncMainChan(UINT32 chan, VOID *prm)
{
    /* 入参有效性校验 */
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    UINT32 uiMainChan = 0;

    uiMainChan = *(UINT32 *)prm;

    if (uiMainChan >= SVA_DEV_MAX)
    {
        SVA_LOGE("chan %d Invalid MainView %d error\n", chan, uiMainChan);
        return SAL_FAIL;
    }

    if (uiMainChan == g_sva_common.uiMainViewChn)
    {
        SVA_LOGW("chan [%d] Same Main View Chan [%d]\n", chan, uiMainChan);
        return SAL_SOK;
    }

    g_sva_common.uiMainViewChn = uiMainChan;

    SVA_LOGI("chan %d Set Main View chan %d OK!\n", chan, g_sva_common.uiMainViewChn);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetSyncMainChan
* 描  述  : 获取同步模式主视角通道(默认1为主视角，0为辅视角)
* 输  入  : 无
* 输  出  : 无
* 返回值  : 通道号
*******************************************************************************/
UINT32 Sva_DrvGetSyncMainChan()
{
    return g_sva_common.uiMainViewChn;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetAlgMode
* 描  述  :
* 输  入  : 无
* 输  出  : 无
* 返回值  : 通道号
*******************************************************************************/
UINT32 Sva_DrvGetAlgMode()
{
    return g_proc_mode;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetScaleLevel
* 描  述  : 设置OSD字体大小
* 输  入  : - chan: 通道号
*         : - prm : 字体大小
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetScaleLevel(UINT32 chan, VOID *prm)
{
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    UINT32 *scaleLevel = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;
    OSD_BLOCK_PRM_S stBlockPrm;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    scaleLevel = (UINT32 *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    /*
       if (*scaleLevel > 4)
       {
        SVA_LOGE("chan %d scaleLevel %d error \n", chan, *scaleLevel);
        return SAL_FAIL;
       }
     */
    if (*scaleLevel >= 4)
    {
        SVA_LOGW("chan %d scaleLevel %d set error, the ScaleLevel is setted 3\n", chan, *scaleLevel);
        *scaleLevel = 3;
    }

    if (pstSva_dev->stIn.stTargetPrm.scaleLevel == *scaleLevel)
    {
        SVA_LOGW("chan %d scaleLevel %d \n", chan, pstSva_dev->stIn.stTargetPrm.scaleLevel);
        return SAL_SOK;
    }

    pstSva_dev->stIn.stTargetPrm.scaleLevel = *scaleLevel;

    /* TODO: 后续拓展使用，当前无效 */
    if (*scaleLevel > 4)
    {
        if (OSD_FONT_TRUETYPE == capb_get_osd()->enFontType)
        {
            stBlockPrm.u32LatNum = 3;

            stBlockPrm.au32LatSize[0] = 16;
            stBlockPrm.au32FontNum[0] = 1;
            stBlockPrm.au32FontSize[0][0] = 16;

            stBlockPrm.au32LatSize[1] = 32;
            stBlockPrm.au32FontNum[1] = 1;
            stBlockPrm.au32FontSize[1][0] = 32;

            stBlockPrm.au32LatSize[2] = *scaleLevel;
            stBlockPrm.au32FontNum[2] = 1;
            stBlockPrm.au32FontSize[2][0] = *scaleLevel;

            (VOID)osd_func_writeStart();

            if (osd_func_updateLatSize(OSD_BLOCK_IDX_STRING, &stBlockPrm))
            {
                SVA_LOGE("update alert name osd size[%u] fail\n", *scaleLevel);
            }

            if (osd_func_updateLatSize(OSD_BLOCK_IDX_ASCII, &stBlockPrm))
            {
                SVA_LOGE("update ASCII osd size[%u] fail\n", *scaleLevel);
            }

            if (osd_func_updateLatSize(OSD_BLOCK_IDX_NUM_PAREN, &stBlockPrm))
            {
                SVA_LOGE("update num osd with parenthneses size[%u] fail\n", *scaleLevel);
            }

            if (osd_func_updateLatSize(OSD_BLOCK_IDX_NUM_MUL, &stBlockPrm))
            {
                SVA_LOGE("update num osd size[%u] fail\n", *scaleLevel);
            }

            if (osd_func_updateLatSize(OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT, &stBlockPrm))
            {
                SVA_LOGE("update num osd size[%u] fail\n", *scaleLevel);
            }

#ifdef DSP_ISA
            if (osd_func_updateLatSize(OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL, &stBlockPrm))
            {
                SVA_LOGE("update num osd size[%u] fail\n", *scaleLevel);
            }

#endif
            (VOID)osd_func_writeEnd();
        }
    }

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SVA_LOGI("chan %d scaleLevel %d \n", chan, pstSva_dev->stIn.stTargetPrm.scaleLevel);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetAlertName
* 描  述  : 设置危险物名称
* 输  入  : - chan: 通道号
*         : - prm : 危险物名称
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetAlertName(UINT32 chan, VOID *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_ALERT_NAME *sva_name = NULL;

    OSD_SET_ARRAY_S stOsdArray;
    OSD_SET_PRM_S *pstOsdPrm = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    sva_name = (SVA_ALERT_NAME *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    stOsdArray.u32StringNum = 0;
    stOsdArray.pstOsdPrm = SAL_memMalloc(sva_name->sva_cnt * sizeof(OSD_SET_PRM_S), "SVA", "sva_osdprm");
    if (NULL == stOsdArray.pstOsdPrm)
    {
        SVA_LOGE("malloc osd block buff fail\n");
        return SAL_FAIL;
    }

    for (i = 0; i < sva_name->sva_cnt; i++)
    {
        if (sva_name->sva_type[i] > SVA_MAX_ALARM_TYPE - 1)
        {
            SVA_LOGE("Invalid type[%d], chan %d \n", sva_name->sva_type[i], chan);
            SAL_memfree(stOsdArray.pstOsdPrm, "SVA", "sva_osdprm");
            return SAL_FAIL;
        }

        sal_memcpy_s(pstSva_dev->stIn.alert[sva_name->sva_type[i]].sva_name, SVA_ALERT_NAME_LEN, sva_name->sva_name[i], SVA_ALERT_NAME_LEN);

        pstOsdPrm = stOsdArray.pstOsdPrm + stOsdArray.u32StringNum++;
        pstOsdPrm->u32Idx = sva_name->sva_type[i];
        pstOsdPrm->szString = (char *)sva_name->sva_name[i];
        pstOsdPrm->u32Color = OSD_COLOR_INVALID;
        pstOsdPrm->u32BgColor = OSD_COLOR_INVALID;
        pstOsdPrm->enEncFormat = sva_name->enEncFormat;

        SVA_LOGD("chan %d type %d sva_name %s \n", chan, sva_name->sva_type[i], sva_name->sva_name[i]);
    }

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        SAL_memfree(stOsdArray.pstOsdPrm, "SVA", "sva_osdprm");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    pstSva_dev->stIn.sva_name_cnt++;

    (VOID)osd_func_writeStart();
    s32Ret = osd_func_blockSet(OSD_BLOCK_IDX_STRING, &stOsdArray);
    (VOID)osd_func_writeEnd();
    SAL_memfree(stOsdArray.pstOsdPrm, "SVA", "sva_osdprm");
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("set osd name fail\n"); /* 从片osd没有初始化，所以会报错 */
    }

    SVA_LOGI("Set Alert Name End! chan %d\n", chan);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetDirection
* 描  述  : 设置安检机传送带方向
* 输  入  : - chan: 通道号
*         : - prm : 传送带方向
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetDirection(UINT32 chan, VOID *prm)
{
    UINT32 uiQueueCnt = 0;

    static UINT32 uiWaitCnt[2] = {0, 0};
    PARAM_INFO_S stParamInfo = {0};

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_ORIENTATION_TYPE *sva_diretion = NULL;
    DUP_ChanHandle *pDupChnHandle = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    sva_diretion = (SVA_ORIENTATION_TYPE *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);
#ifdef DSP_ISM
    return SAL_SOK;
#endif

    if (*sva_diretion > 1)
    {
        SVA_LOGE("direction %d > max 1, pls check!\n", *sva_diretion);
        return SAL_FAIL;
    }

    if (pstSva_dev->stIn.enDirection == *sva_diretion)
    {
        SVA_LOGW("chan %d Dir Orientation %d \n", chan, pstSva_dev->stIn.enDirection);
        return SAL_SOK;
    }

    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA)
        && DOUBLE_CHIP_SLAVE_TYPE != g_sva_common.uiDevType)
    {
        if (SAL_TRUE == pstSva_dev->uiChnStatus)
        {
            pstSva_dev->uiChnMirrorSync = 1;
            while (pstSva_dev->uiChnMirrorSync)
            {
                usleep(1000);
                uiWaitCnt[chan]++;

                if (uiWaitCnt[chan] > 500)
                {
                    SVA_LOGW("Wait for 500ms. TimeOut! \n");
                    break;
                }
            }
        }

        uiWaitCnt[chan] = 0;

        /* 智能通道进行帧镜像，算法内部统一按照从右向左进行处理 */
        stParamInfo.enType = MIRROR_CFG;
        stParamInfo.stMirror.u32Flip = 0;
        stParamInfo.stMirror.u32Mirror = *sva_diretion;

        pDupChnHandle = (DUP_ChanHandle *)pstSva_dev->DupHandle.pHandle;
        if (SAL_SOK != pDupChnHandle->dupOps.OpDupSetBlitPrm((VOID *)pDupChnHandle, &stParamInfo))
        {
            /* 非关键步骤，无需返回失败 */
            SVA_LOGE("Set Vpss Chn Mirror Failed! \n");
            goto EXIT;
        }
    }

    /* 传送带方向 */
    pstSva_dev->stIn.enDirection = *sva_diretion;

    if (DOUBLE_CHIP_SLAVE_TYPE != g_sva_common.uiDevType)
    {
        if (SAL_TRUE == pstSva_dev->uiChnStatus)
        {
            (VOID)Sva_drvChnSignal(pstSva_dev);    /* 线程同步使用 */
        }

        pstSva_dev->uiChnMirrorSync = 0;
        SVA_LOGI("Set Vpss Chn Mirror %d End! chan %d \n", *sva_diretion, chan);
    }

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SVA_LOGI("Set Direction %d End, chan %d \n", pstSva_dev->stIn.enDirection, chan);
    return SAL_SOK;

EXIT:
    if (SAL_TRUE == pstSva_dev->uiChnStatus)
    {
        (VOID)Sva_drvChnSignal(pstSva_dev);
    }

    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetOsdBorderType
* 描  述  : 设置OSD叠框类型
* 输  入  : chan : 通道号
*         : prm  : OSD叠框类型
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetOsdBorderType(UINT32 chan, void *prm)
{
    /* 入参有效性检验 */
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    /* 定义变量 */
    UINT32 i = 0;
    SVA_BORDER_TYPE_E enOsdBorderType = 0;
    UINT32 uiQueueCnt = 0;
    UINT32 uiSvaDevCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    SVA_DEV_PRM *pstSva_dev = NULL;

    /* 入参本地化 */
    enOsdBorderType = *(SVA_BORDER_TYPE_E *)prm;
    if (enOsdBorderType >= SVA_OSD_TYPE_NUM)
    {
        SVA_LOGE("Invalid Osd Border Type %d! Pls Check! \n", enOsdBorderType);
        return SAL_FAIL;
    }

    uiSvaDevCnt = g_sva_common.uiDevCnt;

    /* 目前该配置对上层不区分通道 */
    for (i = 0; i < uiSvaDevCnt; i++)
    {
        pstSva_dev = Sva_DrvGetDev(i);
        SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

        if (enOsdBorderType == pstSva_dev->stIn.drawType)
        {
            SVA_LOGW("Same Osd Border Type %d, chan %d \n", pstSva_dev->stIn.drawType, i);
            continue;
        }

        pstSva_dev->stIn.drawType = enOsdBorderType;

        SVA_LOGI("Set Osd Border Type %d End! chan %d \n", enOsdBorderType, i);
        pstPrmFullQue = &pstSva_dev->stPrmFullQue;
        pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

        uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
        if (0x00 == uiQueueCnt)
        {
            SAL_ERROR("chan %d not enought pstPrmEmptQue\n", chan);
            return SAL_FAIL;
        }

        /* 获取 buffer */
        DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
        sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
        DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetAiTargetMap
* 描  述  : 设置AI模型目标映射关系
* 输  入  : chan : 通道号
*         : prm  : 映射关系
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetAiTargetMap(UINT32 chan, void *prm)
{
    /* 入参有效性检验 */
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    /* 定义变量 */
    UINT32 i = 0;
    INT32 s32Ret = 0;
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_DSP_DET_INFO *pstAiTargetAttr = NULL;
    SVA_DSP_DET_STRU *pstAiTargetInfo = NULL;

    OSD_SET_ARRAY_S stOsdArray;
    OSD_SET_PRM_S *pstOsdPrm = NULL;

    DSPINITPARA *pstInit = NULL;

    /* judge if main view */
    if (chan >= SVA_DEV_MAX)
    {
        SVA_LOGE("Invalid chan %d \n", chan);
        return SAL_FAIL;
    }

    pstInit = SystemPrm_getDspInitPara();

    if (1 == pstInit->stViInitInfoSt.uiViChn && 1 == chan)
    {
        SVA_LOGE("Invalid chan %d \n", chan);
        return SAL_SOK;
    }

    /* 入参本地化 */
    pstAiTargetInfo = (SVA_DSP_DET_STRU *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);

    stOsdArray.u32StringNum = 0;
    stOsdArray.pstOsdPrm = SAL_memMalloc((pstAiTargetInfo->uiCnt) * sizeof(OSD_SET_PRM_S), "SVA", "sva_osdprm");
    if (NULL == stOsdArray.pstOsdPrm)
    {
        SVA_LOGE("malloc osd block buff fail\n");
        return SAL_FAIL;
    }

    for (i = 0; i < pstAiTargetInfo->uiCnt; i++)
    {
        pstAiTargetAttr = &pstAiTargetInfo->det_info[i];

        if (pstAiTargetAttr->sva_type >= SVA_MAX_ALARM_TYPE
            || pstAiTargetAttr->sva_type < SVA_AI_MAX_ALARM_TYPE)
        {
            SVA_LOGE("invalid type %d \n", pstAiTargetAttr->sva_type);
            return SAL_FAIL;
        }

        pstSva_dev->stIn.alert[pstAiTargetAttr->sva_type].bInit = SAL_TRUE;
        pstSva_dev->stIn.alert[pstAiTargetAttr->sva_type].sva_type = pstAiTargetAttr->sva_type;
        pstSva_dev->stIn.alert[pstAiTargetAttr->sva_type].sva_key = pstAiTargetAttr->sva_key;
        pstSva_dev->stIn.alert[pstAiTargetAttr->sva_type].sva_color = pstAiTargetAttr->sva_color;

        sal_memcpy_s(pstSva_dev->stIn.alert[pstAiTargetAttr->sva_type].sva_name,
                     SVA_ALERT_NAME_LEN,
                     pstAiTargetAttr->sva_name,
                     SVA_ALERT_NAME_LEN);

        /* 设置OSD字体参数 */
        pstOsdPrm = stOsdArray.pstOsdPrm + stOsdArray.u32StringNum++;
        pstOsdPrm->u32Idx = pstSva_dev->stIn.alert[pstAiTargetAttr->sva_type].sva_type;
        pstOsdPrm->szString = (char *)pstSva_dev->stIn.alert[pstAiTargetAttr->sva_type].sva_name;
        pstOsdPrm->u32Color = pstSva_dev->stIn.alert[pstAiTargetAttr->sva_type].sva_color;
        pstOsdPrm->u32BgColor = OSD_COLOR24_WHITE;
        pstOsdPrm->enEncFormat = pstAiTargetInfo->enEncFormat;

        SVA_LOGD("idx %d, Set type %d, key %d, color %x, name %s namelen %d\n",
                 i,
                 pstAiTargetAttr->sva_type,
                 pstAiTargetAttr->sva_key,
                 pstAiTargetAttr->sva_color,
                 pstAiTargetAttr->sva_name,
                 (INT32)strlen((CHAR *)pstAiTargetAttr->sva_name));
    }

    (VOID)osd_func_writeStart();
    s32Ret = osd_func_blockSet(OSD_BLOCK_IDX_STRING, &stOsdArray);
    (VOID)osd_func_writeEnd();
    SAL_memfree(stOsdArray.pstOsdPrm, "SVA", "sva_osdprm");
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGW("set ai target osd fail\n");
    }

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);


    SVA_LOGI("Set Ai Target Map End! chan %d \n", chan);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetModelId
* 描  述  : 设置Ai模型ID
* 输  入  : chan : 通道号
*         : prm  : 开关
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetModelId(UINT32 chan, void *prm)
{
    /* 入参有效性检验 */
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    /* 变量定义 */
    UINT8 *pcModelId = NULL;

    pcModelId = (UINT8 *)prm;

    SVA_LOGI("Set Model ID %s End! chan %d \n", pcModelId, chan);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetModelName
* 描  述  : 设置Ai模型名称
* 输  入  : chan : 通道号
*         : prm  : 开关
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetModelName(UINT32 chan, void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    IA_ISA_UPDATA_CFG_JSON_PARA stUpJsonPrm = {0};

    SVA_AI_MODEL_ATTR_S *pstAiModelAttr = NULL;
    SVA_MODEL_PARAM_S *pstModelPrm = NULL;

    /* 入参有效性检验 */
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    /* 入参本地化 */
    pstModelPrm = (SVA_MODEL_PARAM_S *)prm;

    if (SAL_TRUE != IA_GetAlgRunFlag(IA_MOD_SVA))
    {
        SVA_LOGI("master chip no need set ai model name! return success! \n");
        return SAL_SOK;
    }

    if (1 != pstModelPrm->uiModelTypeNum
        || SVA_AI_MODEL != pstModelPrm->astModelTypeAttr[0].enModelType)
    {
        SVA_LOGE("invalid model type num %d, model type %d, should be %d \n",
                 pstModelPrm->uiModelTypeNum, pstModelPrm->astModelTypeAttr[0].enModelType, SVA_AI_MODEL);
        return SAL_FAIL;
    }

    pstAiModelAttr = &pstModelPrm->astModelTypeAttr[0].uModelAttr.stAiModelAttr;

    for (i = 0; i < pstAiModelAttr->uiMainModelNum; i++)
    {
        if (0 == strlen((CHAR *)pstAiModelAttr->chMainModelName[i])
            || strlen((CHAR *)pstAiModelAttr->chMainModelName[i]) >= SVA_MODEL_NAME_MAX_LEN)
        {
            SVA_LOGE("main ai Model Name Length err %d \n", (UINT32)strlen((CHAR *)pstAiModelAttr->chMainModelName[i]));
            return SAL_FAIL;
        }

        stUpJsonPrm.pcAiModelPath[i] = (CHAR *)pstAiModelAttr->chMainModelName[i];
        SVA_LOGD("Set Model Name is %s End! i %d, cnt %d \n", stUpJsonPrm.pcAiModelPath[i], i, pstAiModelAttr->uiMainModelNum);

        /* 修改配置文件中的模型名 */
        stUpJsonPrm.enChgType = CHANGE_MAIN_AI_MODEL_PATH;
        s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("IA_HalModifyCfgFile failed! chan %d \n", chan);
            return SAL_FAIL;
        }
    }

    for (i = 0; i < pstAiModelAttr->uiSideModelNum; i++)
    {
        if (0 == strlen((CHAR *)pstAiModelAttr->chSideModelName[i])
            || strlen((CHAR *)pstAiModelAttr->chSideModelName[i]) >= SVA_MODEL_NAME_MAX_LEN)
        {
            SVA_LOGE("side ai Model Name Length err %d \n", (UINT32)strlen((CHAR *)pstAiModelAttr->chSideModelName[i]));
            return SAL_FAIL;
        }

        stUpJsonPrm.pcAiModelPath[i] = (CHAR *)pstAiModelAttr->chSideModelName[i];
        SVA_LOGD("Set Model Name is %s End! i %d, cnt %d \n", stUpJsonPrm.pcAiModelPath[i], i, pstAiModelAttr->uiSideModelNum);

        /* 修改配置文件中的模型名 */
        stUpJsonPrm.enChgType = CHANGE_SIDE_AI_MODEL_PATH;
        s32Ret = Sva_IsaHalModifyCfgFile(&stUpJsonPrm);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("IA_HalModifyCfgFile failed! chan %d \n", chan);
            return SAL_FAIL;
        }
    }

    SVA_LOGI("Set Model Name End! chan %d \n", chan);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetModelDetectStatus
* 描  述  : 设置模型启用开关
* 输  入  : chan : 通道号
*         : prm  : Ai模型使用标志位
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 注  意  : 通道号chan目前暂未启用，为预留变量
*******************************************************************************/
INT32 Sva_DrvSetModelDetectStatus(UINT32 chan, void *prm)
{
    UINT32 uiModelEnable = SAL_FALSE;
    UINT32 i = 0;

    SVA_VCAE_PRM_ST stVacePrm = {0};
    SVA_DEV_PRM *pstSva_dev = NULL;

    /* Input Args Checker */
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    uiModelEnable = *(UINT32 *)prm;

    SVA_LOGI("Set Ai Model Enable Flag Entering! ai_flag %d, current mode %d \n", uiModelEnable, g_proc_mode);

    if (uiModelEnable > SAL_TRUE)
    {
        SVA_LOGE("Invalid Model Flag %d! chan %d \n", uiModelEnable, chan);
        return SAL_FAIL;
    }

    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        pstSva_dev = Sva_DrvGetDev(i);
        if (SAL_TRUE == pstSva_dev->uiChnStatus)
        {
            SVA_LOGE("Chan %d is runnning! Pls Close all Sva Channels First! \n", i);
            return SAL_FAIL;
        }
    }

    if (SAL_TRUE == IA_GetAlgRunFlag(IA_MOD_SVA))
    {
        stVacePrm.enProcMode = g_proc_mode;
        stVacePrm.uiAiEnable = uiModelEnable;

        if (SAL_SOK != Sva_DrvChgProcMode(&stVacePrm))
        {
            SVA_LOGE("Set Vcae Prm Failed! mode %d, ai_flag %d \n", g_proc_mode, stVacePrm.uiAiEnable);
            return SAL_FAIL;
        }
    }

    guiAiModelEnable = uiModelEnable;

    SVA_LOGI("Set Model Status %d End, chan %d, current mode %d \n", uiModelEnable, chan, g_proc_mode);
    return SAL_SOK;
}

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
INT32 Sva_DrvGetModelDetectStatus(UINT32 chan, void *prm)
{
    /* 入参有效性检验 */
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    /* 定义变量 */
    UINT32 *puiEnableFlag = NULL;

    /* judge if main view */
    if (0 != chan)
    {
        SVA_LOGE("Invalid chan %d \n", chan);
        return SAL_FAIL;
    }

    /* 入参本地化 */
    puiEnableFlag = (UINT32 *)prm;

    *puiEnableFlag = guiAiModelEnable;

    SVA_LOGI("Get Model Status %d End, chan %d \n", *puiEnableFlag, chan);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetVersion
* 描  述  : 获取算法版本号，目前引擎没有版本号
* 输  入  : - prm: 版本号信息
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetVersion(VOID *prm)
{
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

#ifdef DSP_ISM
    /*双芯片主片无法获取算法版本号，直接跳过*/
    if (DOUBLE_CHIP_MASTER_TYPE == g_sva_common.uiDevType)
    {
        SVA_LOGW("The master can not get the version under double chip\n");
        return SAL_SOK;
    }

#endif

    if (SAL_SOK != Sva_HalGetVersion(prm))
    {
        SVA_LOGE("get version error\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}
/**
 * @function:   Sva_DrvGetDefConf
 * @brief:      获取默认置信度
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */

INT32 Sva_DrvGetDefConf(UINT32 chan, VOID *prm)
{
    INT32 s32Ret = SAL_SOK;
	
    SVA_DSP_DEF_CONF_OUT *pstXsiDefConf = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pstXsiDefConf = (SVA_DSP_DEF_CONF_OUT *)prm;


    /*从默认置信度表格中获取默认置信度*/
    s32Ret = Sva_HalGetDefConf(chan, pstXsiDefConf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Sva_HalGetDefConf! chan %d \n", chan);
        return SAL_FAIL;
    }
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetTarFilterCfg
 * @brief:      配置目标尺寸过滤参数
 * @param[in]:  SVA_ALERT_SIZE_FILTER_PRM_S *pstSizeFilterPrm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetTarFilterCfg(SVA_ALERT_SIZE_FILTER_PRM_S *pstSizeFilterPrm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    VOID *pHandle = NULL;
    XSI_COMMON *pXsiCommon = NULL;

    pXsiCommon = Sva_HalGetXsiCommon();
    SVA_DRV_CHECK_PRM(pXsiCommon, SAL_FAIL);

    for (i = 0; i < pXsiCommon->uiChannelNum; i++)
    {
        pHandle = pXsiCommon->stEngChnPrm[i].pHandle;

#ifdef SIZEFILTER
        if (SVA_PROC_MODE_IMAGE != g_proc_mode)
        {
            pstSizeFilterPrm->reserved[0] = XSIE_XSI_NODE;
        }
        else
        {
            pstSizeFilterPrm->reserved[0] = XSIE_XSI_NODE_1;     /* 海思平台图片模式下用33 */
        }

        SVA_LOGI("hi3559a \n");
#else
        SVA_LOGI("rk3588 \n");
        if (SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode)
        {
            pstSizeFilterPrm->reserved[0] = XSIE_XSI_NODE;
        }
        else
        {
            pstSizeFilterPrm->reserved[0] = XSIE_XSI_NODE_1;     /* RK平台图片和视频单路模式下用33 */
        }

#endif

        s32Ret = Sva_HalSetTarSizeFilterCfg(pHandle, pstSizeFilterPrm);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("set tar filter cfg failed! i %d \n", i);
            return SAL_FAIL;
        }
    }

    SVA_LOGI("set tar filter cfg end! i %d \n", i);

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetPkModeSwitch
 * @brief:      设置PK模式开关
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
static INT32 ATTRIBUTE_UNUSED Sva_DrvSetPkModeSwitch(UINT32 chan, VOID *prm)
{
    UINT32 uiFlag = 0;
    UINT32 uiQueueCnt = 0;

    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PROCESS_IN *pstIn = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    uiFlag = *(UINT32 *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstSva_dev->stIn.uiPkSwitch = uiFlag > SAL_TRUE ? SAL_TRUE : uiFlag;

    /* 若PK模式已经打开，则关闭喷灌过滤，反之打开喷灌过滤 */
    pstSva_dev->stIn.uiSprayFilterSwitch = !(pstSva_dev->stIn.uiPkSwitch);

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SVA_LOGI("set pk mode switch %d end! chan %d \n", uiFlag, chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetSprayFilterSwitch
 * @brief:      设置喷灌过滤开关
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetSprayFilterSwitch(UINT32 chan, VOID *prm)
{
    UINT32 uiFlag = 0;
    UINT32 uiQueueCnt = 0;

    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PROCESS_IN *pstIn = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    uiFlag = *(UINT32 *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    /* 若PK模式已经打开，则关闭喷灌过滤 */
    if (pstSva_dev->stIn.uiPkSwitch)
    {
        pstSva_dev->stIn.uiSprayFilterSwitch = SAL_FALSE;

        SVA_LOGW("pk mode is on! no need enable spary filter switch, return success! chan %d \n", chan);
        return SAL_SOK;
    }

    pstSva_dev->stIn.uiSprayFilterSwitch = uiFlag > SAL_TRUE ? SAL_TRUE : uiFlag;

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SVA_LOGI("set sprary filter switch %d end! chan %d \n", uiFlag, chan);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetFrameMem
* 描  述  :
* 输  入  : - imgW              :
*         : - imgH              :
*         : - pstSystemFrameInfo:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetFrameMem(UINT32 imgW, UINT32 imgH, BOOL bCached, SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    INT32 s32Ret = SAL_SOK;

    UINT64 u64BlkSize = (UINT64)((UINT64)imgW * (UINT64)imgH * 3 / 2);
    UINT32 u32LumaSize = 0;
    /* UINT32 u32ChrmSize = 0; */

    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    ALLOC_VB_INFO_S stAllocVbInfo = {0};

    s32Ret = sys_hal_allocVideoFrameInfoSt(pstSystemFrameInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("alloc video frame info st failed! \n");
        goto ERR_1;
    }

    s32Ret = mem_hal_vbAlloc(u64BlkSize, "SVA", "sva_frame_mem", NULL, bCached, &stAllocVbInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Alloc Mem Failed! \n");
        goto ERR_2;
    }

    u32LumaSize = (imgW * imgH);
    /* u32ChrmSize = (imgW * imgH / 4); */

    stVideoFrmBuf.poolId = stAllocVbInfo.u32PoolId;
    stVideoFrmBuf.vbBlk = stAllocVbInfo.u64VbBlk;

    stVideoFrmBuf.virAddr[0] = (PhysAddr)stAllocVbInfo.pVirAddr;
    stVideoFrmBuf.virAddr[1] = stVideoFrmBuf.virAddr[0] + u32LumaSize;
    stVideoFrmBuf.virAddr[2] = stVideoFrmBuf.virAddr[1];
    stVideoFrmBuf.phyAddr[0] = (PhysAddr)stAllocVbInfo.u64PhysAddr;
    stVideoFrmBuf.phyAddr[1] = (PhysAddr)(stVideoFrmBuf.phyAddr[0] + u32LumaSize);
    stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];
    stVideoFrmBuf.frameParam.width = imgW;
    stVideoFrmBuf.frameParam.height = imgH;
    stVideoFrmBuf.stride[0] = imgW;
    stVideoFrmBuf.stride[1] = imgW;
    stVideoFrmBuf.stride[2] = imgW;
    stVideoFrmBuf.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

    s32Ret = sys_hal_buildVideoFrame(&stVideoFrmBuf, pstSystemFrameInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("build video frame info st failed! \n");
        goto ERR_2;
    }

    pstSystemFrameInfo->uiDataAddr = (PhysAddr)stAllocVbInfo.pVirAddr;
    pstSystemFrameInfo->uiDataWidth = imgW;
    pstSystemFrameInfo->uiDataHeight = imgH;

    return SAL_SOK;

ERR_2:
    if (0x00 != pstSystemFrameInfo->uiAppData)
    {
        (VOID)sys_hal_rleaseVideoFrameInfoSt(pstSystemFrameInfo);
    }

ERR_1:
    if (NULL != stAllocVbInfo.pVirAddr)
    {
        if (SAL_SOK != mem_hal_vbFree(stAllocVbInfo.pVirAddr, "SVA", "sva_frame_mem", stAllocVbInfo.u32Size, stAllocVbInfo.u64VbBlk, stAllocVbInfo.u32PoolId))
        {
            SVA_LOGE("free mmz failed! \n");
        }
    }

    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvPutFrameMem
* 描  述  :
* 输  入  :
*         : - pstSystemFrameInfo:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Sva_DrvPutFrameMem(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    SVA_DRV_CHECK_PRM(pstSystemFrameInfo, SAL_FAIL);

    if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSystemFrameInfo, &stVideoFrmBuf))
    {
        SVA_LOGE("get video frame info failed! \n");
        return SAL_FAIL;
    }

    if (NULL != (Ptr)stVideoFrmBuf.virAddr[0])
    {
        if (SAL_SOK != mem_hal_vbFree((Ptr)stVideoFrmBuf.virAddr[0],
                                      "SVA",
                                      "sva_frame_mem",
                                      stVideoFrmBuf.frameParam.width * stVideoFrmBuf.frameParam.height * 3 / 2,
                                      stVideoFrmBuf.vbBlk,
                                      stVideoFrmBuf.poolId))
        {
            SVA_LOGE("free mmz failed! \n");
        }
    }

    if (0x00 != pstSystemFrameInfo->uiAppData)
    {
        (VOID)sys_hal_rleaseVideoFrameInfoSt(pstSystemFrameInfo);
    }

    return SAL_SOK;

}

/**
 * @function:   Sva_DrvGetChnSts
 * @brief:      获取通道状态
 * @param[in]:  UINT32 *puiChn
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvGetChnSts(UINT32 *puiChn)
{
    UINT32 i = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_PRM(puiChn, SAL_FAIL);

    /* 获取智能通道状态 */
    for (i = 0; i < SVA_DEV_MAX; i++)
    {
        pstSva_dev = Sva_DrvGetDev(i);
        SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

        puiChn[i] = pstSva_dev->uiChnStatus;
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvGetXsiSts
 * @brief:      获取xsi通道状态
 * @param[in]:  UINT32 *puiChn
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvGetXsiSts(UINT32 *puiChn)
{
    UINT32 i = 0;
    XSI_DEV *pstXsi = NULL;

    SVA_DRV_CHECK_PRM(puiChn, SAL_FAIL);

    /* 获取智能通道状态 */
    for (i = 0; i < SVA_DEV_MAX; i++)
    {
        pstXsi = Sva_HalGetDev(i);
        if (NULL == pstXsi)
        {
            SVA_LOGE("chan %d, pstXsi == NULL\n", 0);
            continue;
        }

        puiChn[i] = pstXsi->xsi_status;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvPutFrameDataFromQue
* 描  述  : 释放队列数据(双芯片从片使用)
* 输  入  : - ppFrame : 通道0数据
*         : - ppFrame1: 通道1数据
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvPutFrameDataFromQue(UINT32 chan, SYSTEM_FRAME_INFO *pFrame, UINT32 timeout)
{
    SVA_SLAVE_PRM *pstSlavePrm = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    pstSlavePrm = Sva_DrvGetSlavePrm(chan);
    SVA_DRV_CHECK_PRM(pstSlavePrm, SAL_FAIL);

    pstEmptQue = &pstSlavePrm->stFrameDataEmptQue;

    (VOID)DSA_QuePut(pstEmptQue, (void *)pFrame, timeout);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetFrameDataFromQue
* 描  述  : 获取队列数据(双芯片从片使用)
* 输  入  : - ppFrame : 通道0数据
*         : - ppFrame1: 通道1数据
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetFrameDataFromQue(UINT32 chan, SYSTEM_FRAME_INFO **ppFrame, SYSTEM_FRAME_INFO **ppFrame1, UINT32 timeout)
{
    SVA_SLAVE_PRM *pstSlavePrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    SYSTEM_FRAME_INFO *pstSysFrameInfo = NULL;

    pstSlavePrm = Sva_DrvGetSlavePrm(chan);
    SVA_DRV_CHECK_PRM(pstSlavePrm, SAL_FAIL);

    pstFullQue = &pstSlavePrm->stFrameDataFullQue;

    DSA_QueGet(pstFullQue, (void **)&pstSysFrameInfo, timeout);

    *ppFrame = (SYSTEM_FRAME_INFO *)&pstSysFrameInfo[0];
    *ppFrame1 = (SYSTEM_FRAME_INFO *)&pstSysFrameInfo[1];

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvDeInitSlaveFrameQue
* 描  述  : 去初始化从片队列相关资源(双芯片从片使用)
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Sva_DrvDeInitSlaveFrameQue(void)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 k = 0;

    SVA_SLAVE_PRM *pstSlavePrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    SYSTEM_FRAME_INFO *pstSysFrameInfo = NULL;

    for (k = 0; k < 2; k++)
    {
        pstSlavePrm = Sva_DrvGetSlavePrm(k);
        SVA_DRV_CHECK_PRM(pstSlavePrm, SAL_FAIL);

        pstEmptQue = &pstSlavePrm->stFrameDataEmptQue;
        pstFullQue = &pstSlavePrm->stFrameDataFullQue;

        if (SAL_TRUE != pstSlavePrm->uiQueCreated)
        {
            SVA_LOGW("Queue is not created! \n");
            return SAL_SOK;
        }

        for (i = 0; i < SVA_MAX_SLAVE_BUFFER_NUM; i++)
        {
            DSA_QueGet(pstFullQue, (void **)&pstSysFrameInfo, SAL_TIMEOUT_FOREVER);
            if (NULL != pstSysFrameInfo)
            {
                DSA_QuePut(pstEmptQue, (void *)pstSysFrameInfo, SAL_TIMEOUT_NONE);
            }

            pstSysFrameInfo = NULL;
        }

        for (i = 0; i < SVA_MAX_SLAVE_BUFFER_NUM; i++)
        {
            for (j = 0; j < SVA_DEV_MAX; j++)
            {
                pstSysFrameInfo = &pstSlavePrm->stFrameInfo[i][j];

                if (NULL == pstSysFrameInfo)
                {
                    continue;
                }

                s32Ret = disp_hal_putFrameMem(pstSysFrameInfo);
                if (s32Ret != SAL_SOK)
                {
                    SVA_LOGE("disp_hal_putFrameMem error !!!\n");
                    goto err;
                }
            }

            SVA_LOGW("Destroy Que Frame! i %d\n", i);
        }

        s32Ret = DSA_QueDelete(pstEmptQue);
        s32Ret |= DSA_QueDelete(pstFullQue);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Delete Queue Failed! \n");
            goto err;
        }
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvInitSlaveFrameQue
* 描  述  : 初始化从片队列相关资源(双芯片从片使用)
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Sva_DrvInitSlaveFrameQue(void)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 k = 0;

    SVA_SLAVE_PRM *pstSlavePrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;

    SYSTEM_FRAME_INFO *pstSysFrameInfo = NULL;

    for (k = 0; k < 2; k++)
    {
        pstSlavePrm = Sva_DrvGetSlavePrm(k);
        SVA_DRV_CHECK_PRM(pstSlavePrm, SAL_FAIL);

        /* 1. 创建管理满队列 */
        pstFullQue = &pstSlavePrm->stFrameDataFullQue;
        if (SAL_SOK != DSA_QueCreate(pstFullQue, SVA_MAX_SLAVE_BUFFER_NUM))
        {
            SVA_LOGE("error !!!\n");
            goto err;
        }

        /* 2. 创建管理空队列 */
        pstEmptQue = &pstSlavePrm->stFrameDataEmptQue;
        if (SAL_SOK != DSA_QueCreate(pstEmptQue, SVA_MAX_SLAVE_BUFFER_NUM))
        {
            SVA_LOGE("error !!!\n");
            goto err;
        }

        /* 3. 缓存区放入空队列 */
        for (i = 0; i < SVA_MAX_SLAVE_BUFFER_NUM; i++)
        {
            for (j = 0; j < SVA_DEV_MAX; j++)
            {
                pstSysFrameInfo = &pstSlavePrm->stFrameInfo[i][j];

                s32Ret = Sva_DrvGetFrameMem(SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, SAL_FALSE, pstSysFrameInfo);
                if (s32Ret != SAL_SOK)
                {
                    SVA_LOGE("Sva_DrvGetFrameMem error !!!\n");
                    goto err;
                }
            }

            s32Ret = DSA_QuePut(pstEmptQue, (void *)pstSlavePrm->stFrameInfo[i], SAL_TIMEOUT_NONE);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("Put Data to Queue Failed! \n");
                goto err;
            }

            SVA_LOGW("put emtpy que %p ! i %d, %p %p \n",
                     pstSlavePrm->stFrameInfo[i],
                     i, &pstSlavePrm->stFrameInfo[i][0], &pstSlavePrm->stFrameInfo[i][1]);
        }

        pstSlavePrm->uiQueCreated = SAL_TRUE;
        pstSlavePrm->uiQueDepth = SVA_MAX_SLAVE_BUFFER_NUM;
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Sva_DrvClrFullQue
 * @brief:      清空满队列数据
 * @param[in]:  void
 * @param[out]: None
 * @return:     void
 */
void Sva_DrvClrFullQue(UINT32 chan)
{
    UINT32 i = 0;
    SVA_SLAVE_PRM *pstSlavePrm = NULL;
    SYSTEM_FRAME_INFO *pstSysFrameInfo[2] = {NULL, NULL};

    pstSlavePrm = Sva_DrvGetSlavePrm(chan);

    for (i = 0; i < pstSlavePrm->uiQueDepth; i++)
    {
        pstSysFrameInfo[0] = NULL;
        pstSysFrameInfo[1] = NULL;

        (VOID)Sva_DrvGetFrameDataFromQue(chan, &pstSysFrameInfo[0], &pstSysFrameInfo[1], SAL_TIMEOUT_NONE);

        if (NULL != pstSysFrameInfo[0] || NULL != pstSysFrameInfo[1])
        {
            (VOID)Sva_DrvPutFrameDataFromQue(chan, pstSysFrameInfo[0], SAL_TIMEOUT_NONE);
/* #ifdef DEBUG_LOG */
            SVA_LOGW("szl_dbg: rls full que end! i %d \n", i);
/* #endif */
        }
    }

    return;
}

/**
 * @function    Sva_DrvCreateBusiDev
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvCreateBusiDev(xtrans_msg_module_id_e enModuleId,
                                  XTRANS_BUSSINESS_STM_S *pstStmInfo,
                                  XTRANS_BUSSINESS_MSG_S *pstMsgInfo,
                                  XTRANS_BUSSINESS_DEV_S **ppstBusiDev)
{
    INT32 s32Ret = SAL_SOK;

    /* 码流数据传输使用XTRANS_MSG_MODULE_VENC */
    s32Ret = xtrans_create_bus_dev(enModuleId, pstStmInfo, pstMsgInfo, ppstBusiDev);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("create business failed! mod %d \n", enModuleId);
        (VOID)xtrans_destory_bus_dev(*ppstBusiDev);
        return SAL_FAIL;
    }

    s32Ret = (*ppstBusiDev)->init(*ppstBusiDev);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("init business failed! mod %d \n", enModuleId);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvSendFramePrmByMmap
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvSendFramePrmByMmap(VOID *pData, UINT64 u64DataSize)
{
    /* xtrans组件库中使用传输码流的消息头传递帧参数，故此处直接返回 */
    return SAL_SOK;
}

/**
 * @function    Sva_DrvRecvFramePrmByMmap_V2
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvRecvFramePrmByMmap(VOID *pData, UINT64 u64DataSize)
{
    /* 帧参数在码流发送的消息中发送到从片，故此接口直接返回成功 */
    return SAL_SOK;
}

/**
 * @function    Sva_DrvSetRemoteDesc
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvSetRemoteDesc(int mod_id)
{
    if (SAL_TRUE == IA_GetModTransFlag(mod_id))
    {
        DSPINITPARA *pstInit = SystemPrm_getDspInitPara();

        g_sva_common.stXtransPrm.uiToChipId = pstInit->stIaInitMapPrm.stIaModCapbPrm[mod_id].stIaModTransPrm.uiRunChipId;
    }

    return SAL_SOK;
}

static xtrans_data_info_s stbufDataInfo = {0};   /* 帧参数结构信息，通过码流传输的msg head传过去 */

/**
 * @function    Sva_DrvGetMsgData
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static xtrans_data_info_s *Sva_DrvGetMsgData(VOID)
{
    return &stbufDataInfo;
}

/**
 * @function    Sva_DrvSetMsgData
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvSetMsgData(VOID *pTmpBuf, xtrans_data_info_s *pstDataInfo, UINT32 uiSize)
{
    if (NULL == pTmpBuf || NULL == pstDataInfo)
    {
        SVA_LOGE("ptr NULL! %p, %p \n", pTmpBuf, pstDataInfo);
        return;
    }

    /* 拷贝帧参数 */
    sal_memcpy_s(pstDataInfo->datainfo, 3 * 1024, pTmpBuf, uiSize);

    return;
}

/**
 * @function    Sva_DrvTransFrmWake
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Sva_DrvTransFrmWake(const xtrans_stm_info_s *stmInfo)   /* 入参未使用 */
{
    SAL_SemPost(g_sva_common.stXtransPrm.pSem);
    return SAL_SOK;
}

/**
 * @function    Sva_DrvTransFrmWait
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Sva_DrvTransFrmWait(VOID)
{
    SAL_SemWait(g_sva_common.stXtransPrm.pSem);
    return SAL_SOK;
}

/**
 * @function    Sva_DrvTransRsltWake
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Sva_DrvTransRsltWake(const xtrans_stm_info_s *stmInfo)
{
    /* 信号量通知帧发送已经结束 */
    SAL_SemPost(g_sva_common.stXtransPrm.pRsltSem);

    return SAL_SOK;
}

/**
 * @function    Sva_DrvTransRsltWait
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Sva_DrvTransRsltWait(VOID)
{
    SAL_SemWait(g_sva_common.stXtransPrm.pRsltSem);
    return SAL_SOK;
}

/**
 * @function   Sva_DrvCreateMasterSendBusDevice
 * @brief      主片创建发送xtrans business设备
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvCreateMasterSendBusDevice(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    XTRANS_BUSSINESS_STM_S stm = {0};
    XTRANS_BUSSINESS_MSG_S msg = {0};

    stm.u8Func = XTRANS_PROT_STREAM_SEND;
    stm.stmSendOver = Sva_DrvTransFrmWake;

    /* 码流数据传输使用XTRANS_MSG_MODULE_VENC */
    s32Ret = Sva_DrvCreateBusiDev(XTRANS_MSG_MODULE_VENC, &stm, &msg, &g_sva_common.stXtransPrm.pstBusiDevSendFrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("create business device failed! modId %d \n", XTRANS_PROT_STREAM_SEND);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvSendFrameByDma_V2
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvSendFrameByDma(PhysAddr u64PhyAddr, UINT64 u64FrameSize)
{
    INT32 s32Ret = SAL_SOK;

    xtrans_comm_desc_s stRemoteDesc = {0};
    xtrans_buf_info_s stBufInfo = {0};

    xtrans_data_info_s *pstbufDataInfo = NULL;

    stRemoteDesc.u8ChipId = g_sva_common.stXtransPrm.uiToChipId;

    stBufInfo.pvVirAddr = NULL; /* DMA传输无需虚拟地址 */
    stBufInfo.uiptrPhyAddr = u64PhyAddr;
    stBufInfo.s32BufSize = u64FrameSize;

    pstbufDataInfo = Sva_DrvGetMsgData();

    if (!g_sva_common.stXtransPrm.pstBusiDevSendFrm)
    {
        s32Ret = Sva_DrvCreateMasterSendBusDevice();
        if (SAL_SOK != s32Ret || !g_sva_common.stXtransPrm.pstBusiDevSendFrm)
        {
            SVA_LOGE("create master send xtrans failed! \n");
            return SAL_FAIL;
        }
    }

    /* 支持xtrans库进行主从通信 */
    s32Ret = g_sva_common.stXtransPrm.pstBusiDevSendFrm->sendStream(g_sva_common.stXtransPrm.pstBusiDevSendFrm,
                                                                    stRemoteDesc,
                                                                    &stBufInfo,
                                                                    pstbufDataInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("xtrans send stream failed! \n");
        return SAL_FAIL;
    }

    /* 等待xtrans返回发送结束信号 */
    (VOID)Sva_DrvTransFrmWait();

    return SAL_SOK;
}

static PhysAddr g_u64RecvBufPhyAddr = 0;

/**
 * @function    Sva_DrvSetRecvBufInfo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvSetRecvBufInfo(PhysAddr u64PhyAddr)
{
    g_u64RecvBufPhyAddr = u64PhyAddr;
    return SAL_SOK;
}

/**
 * @function    Sva_DrvGetRecvBufInfo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvGetRecvBufInfo(PhysAddr *pu64PhyAddr)
{
    *pu64PhyAddr = g_u64RecvBufPhyAddr;
    return SAL_SOK;
}

static BOOL bPhyReady = SAL_FALSE;    /* 空闲物理地址状态是否ready */
static CHAR acTmp[512] = {0};    /* 用于存放从码流传输的msg head获取的帧参数 */

/**
 * @function    Sva_DrvSetRecvMsg
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvSetRecvMsg(VOID *pSrcBuf, UINT32 uiSize, VOID *pDstBuf)
{
    if (NULL == pDstBuf || NULL == pSrcBuf)
    {
        SVA_LOGE("ptr null! %p, %p \n", pDstBuf, pSrcBuf);
        return SAL_FAIL;
    }

    sal_memcpy_s(pDstBuf, uiSize, pSrcBuf, uiSize);
    return SAL_SOK;
}

/**
 * @function    Sva_DrvGetRecvMsg
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvGetRecvMsg(const VOID *pstMsgHead, VOID *pTmpBuf, UINT32 uiSize)
{
    XTRANS_BUF_HEAD_S *pstHeadBuf = NULL;

    if (NULL == pstMsgHead || NULL == pTmpBuf)
    {
        SVA_LOGE("ptr null! %p, %p \n", pstMsgHead, pTmpBuf);
        return SAL_FAIL;
    }

    pstHeadBuf = (XTRANS_BUF_HEAD_S *)pstMsgHead;

    sal_memcpy_s(pTmpBuf, uiSize, &pstHeadBuf->stBufDataInfo, sizeof(xtrans_data_info_s));
    return SAL_SOK;
}

/**
 * @function    Sva_DrvRecvFrmMsgCb
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvRecvFrmMsgCb(const void *pstStmInfo /*XTRANS_BUF_HEAD_S*/,
                                 xtrans_comm_desc_s stTransFromDesc, /* 来自的传输模块的设备描述 */
                                 const char *pstInfo)
{
    if (SAL_SOK != Sva_DrvGetRecvMsg(pstStmInfo, acTmp, sizeof(SAL_VideoFrameBuf)))
    {
        SVA_LOGE("get recv msg failed! \n");
        return SAL_FAIL;
    }

/*    bPhyReady = SAL_FALSE; */
    return SAL_SOK;
}

/**
 * @function    Sva_DrvMallocRecvBuf
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvMallocRecvBuf(HPR_UINT32 u32Size, intptr_t *aiphyAddr, void **avpVirAddr)
{
    INT32 s32TimeOut = 2000;           /* 等待空闲物理地址超时时间 */

    while (SAL_TRUE != bPhyReady)
    {
        usleep(1000);
        if (s32TimeOut <= 0)
        {
            s32TimeOut = 2000;
            SVA_LOGE("time out! wait for free phy over 2s! \n");
        }

        s32TimeOut--;
    }

    *avpVirAddr = NULL;
    return Sva_DrvGetRecvBufInfo((PhysAddr *)aiphyAddr);
}

/**
 * @function    Sva_DrvFreeRecvBuf
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvFreeRecvBuf(intptr_t aiphyAddr, void *pvVirAddr)
{
    /* 接收失败时的内存释放动作外部管理 */
    bPhyReady = SAL_FALSE;

    /* 唤醒码流接收线程 */
    Sva_DrvTransFrmWake(NULL);

    return SAL_SOK;
}

/**
 * @function   Sva_DrvCreateSlaveRecvFrmBusDevice
 * @brief      从片接收帧数据创建business device
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvCreateSlaveRecvFrmBusDevice(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    XTRANS_BUSSINESS_STM_S stm = {0};
    XTRANS_BUSSINESS_MSG_S msg = {0};

    stm.u8Func = XTRANS_PROT_STREAM_RECEIVE;
    stm.msgRecvProcess = Sva_DrvRecvFrmMsgCb;
    stm.phyMalloc = Sva_DrvMallocRecvBuf;          /* 向组件内部注册的获取接收数据的内存函数 */
    stm.free = Sva_DrvFreeRecvBuf;                 /* 向组件内部注册的释放接收数据的内存函数 */

    /* 码流数据传输使用XTRANS_MSG_MODULE_VENC */
    s32Ret = Sva_DrvCreateBusiDev(XTRANS_MSG_MODULE_VENC, &stm, &msg, &g_sva_common.stXtransPrm.pstBusiDevSendFrm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("create business device failed! modId %d \n", XTRANS_PROT_STREAM_RECEIVE);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvRecvFrameByDma
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvRecvFrameByDma(PhysAddr u64PhyAddr, UINT64 u64FrameSize)
{
    if (!g_sva_common.stXtransPrm.pstBusiDevSendFrm)
    {
        if (SAL_SOK != Sva_DrvCreateSlaveRecvFrmBusDevice())
        {
            SVA_LOGE("slave create recv frm business device failed! \n");
            return SAL_FAIL;
        }
    }

    /* 保存接收码流数据的物理地址，用于xtrans库处理完成后通过注册的回调获取 */
    if (SAL_SOK != Sva_DrvSetRecvBufInfo(u64PhyAddr))
    {
        SVA_LOGE("set recv buf info failed! \n");
        return SAL_FAIL;
    }

    /* 获取到空闲物理地址，可用于接收通信组件xtrans发送过来的数据 */
    bPhyReady = SAL_TRUE;

    /* 等到从片接收码流传输结束 */
    Sva_DrvTransFrmWait();

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSlaveFrameToQueThread
* 描  述  : 主从通信数据入队列(双芯片从片使用)
* 输  入  : - prm: 全局变量参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
void *Sva_DrvSlaveFrameToQueThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    INT8 path[64] = {0};
    UINT32 i = 0;

    UINT32 time00 = 0;
    UINT32 time01 = 0;

    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 cnt = 0;
    UINT32 uiProcGap = 0;
    UINT32 uiMainViewChn = 0;
    UINT32 uiTmpChn = 0;
    /* UINT32 uiClrFlag = SAL_FALSE; */
    UINT64 u64FrameSize = SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2;
    UINT64 u64FrameExtSize = sizeof(SAL_VideoFrameBuf);
    UINT64 u64PhyAddr = 0;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    INT8 *pFrmExtBuf = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DEV_PRM *pstSva_dev1 = NULL;
    SVA_DEV_PRM *pstSva_dev2 = NULL;
    SVA_SLAVE_PRM *pstSlavePrm = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    SYSTEM_FRAME_INFO *pstSysFrameInfo = NULL;
    SAL_VideoFrameBuf *pstVideoFrmBuf = NULL;

    SVA_DRV_CHECK_PRM(prm, NULL);
    /* SVA_PROC_INTER_PRM *pstProcInterPrm = NULL; */

    pstSva_dev = (SVA_DEV_PRM *)prm;
    /* pstProcInterPrm = &pstSva_dev->stProcInterPrm; */

    s32Ret = Sva_DrvInitSlaveFrameQue();
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Init Slave Frame Queue Failed! \n");
        goto EXIT;
    }

    pstSlavePrm = Sva_DrvGetSlavePrm(0);

    pstFullQue = &pstSlavePrm->stFrameDataFullQue;
    pstEmptQue = &pstSlavePrm->stFrameDataEmptQue;

    s32Ret = mem_hal_mmzAlloc((UINT32)u64FrameExtSize, "SVA", "sva_slv_prm", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&pFrmExtBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("mem_hal_mmzAlloc failed! \n");
        goto EXIT;
    }

    SAL_SET_THR_NAME();
    SAL_SetThreadCoreBind(((pstSva_dev->chan + 1) % 2));

    while (1)
    {
#if 0
        if ((0 == g_sva_common.uiChnCnt)
            || (SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode && g_sva_common.uiChnCnt < SVA_DEV_MAX)
            || (SAL_TRUE != Sva_DrvGetInitFlag()))
        {
            usleep(100 * 1000);

            /* 满队列数据进行释放，避免旧数据残留 */
            if (uiClrFlag)
            {
                (VOID)Sva_DrvClrFullQue();
                uiClrFlag = SAL_FALSE;
            }

            if (pstProcInterPrm->uiSyncFlag || !pstProcInterPrm->uiProcFlag)
            {
                pstProcInterPrm->uiSyncFlag = SAL_FALSE;
                SVA_LOGW("SAL_mutexWait, mode %d, cnt %d\n", g_proc_mode, g_sva_common.uiChnCnt);
                SAL_mutexWait(pstProcInterPrm->mChnMutexHd1);
                SVA_LOGW("SAL_mutexsignal, mode %d, cnt %d\n", g_proc_mode, g_sva_common.uiChnCnt);
            }

            continue;
        }

        pstProcInterPrm->uiSyncFlag = SAL_FALSE;

        uiClrFlag = SAL_TRUE;
#endif

        time00 = SAL_getCurMs();

        pstSysFrameInfo = NULL;

        /* 线程不阻塞实现 */
        DSA_QueGet(pstEmptQue, (void **)&pstSysFrameInfo, SAL_TIMEOUT_NONE);

        time01 = SAL_getCurMs();
        (VOID)Sva_DrvTransDbgAddTime(SVA_TRANS_DBG_SLAVE_YUV_TYPE, SVA_TRANS_CONV_IDX(SLV_YUV_GET_QUEUE_TYPE), time00, time01);

        if (NULL == pstSysFrameInfo)
        {
            SVA_LOGD("Get Empty que Failed! Que Data ERR! \n");
            usleep(10 * 1000);
            continue;
        }

        uiMainViewChn = g_sva_common.uiMainViewChn;

        for (i = 0; i < SVA_DEV_MAX; i++)
        {
            uiTmpChn = (0 == i) ? uiMainViewChn : 1 - uiMainViewChn;
            pstSva_dev = Sva_DrvGetDev(uiTmpChn);

            if (NULL == pstSva_dev || SAL_TRUE != pstSva_dev->uiChnStatus)
            {
                continue;
            }

            if (SAL_SOK != sys_hal_getVideoFrameInfo(&pstSysFrameInfo[uiTmpChn], &stVideoFrmBuf))
            {
                SVA_LOGE("get video frame failed! \n");
                goto putData;
            }

            (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time0);

            g_sva_common.stDbgPrm.stCommSts.uiRecvFrameFlag = 1;    /* 送帧之前标记 */

            g_sva_common.stDbgPrm.stCommSts.uiRecvFrameNum++;
            /* 接收主片发送过来的帧数据 */
            s32Ret = Sva_DrvRecvFrameByDma(stVideoFrmBuf.phyAddr[0], u64FrameSize);

            if (SAL_SOK != s32Ret)
            {
                g_sva_common.stDbgPrm.stCommSts.uiRecvFrameFailNum++;
            }
            else
            {
                g_sva_common.stDbgPrm.stCommSts.uiRecvFrameSuccNum++;
            }

            g_sva_common.stDbgPrm.stCommSts.uiRecvFrameFlag = 0;    /* 送帧之后标记 */

            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("dual-chip check data status failed! \n");
                goto putData;
            }

            (VOID)Sva_DrvSetRecvMsg(acTmp, u64FrameExtSize, pFrmExtBuf);

            (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time1);
            (VOID)Sva_DrvTransDbgAddTime(SVA_TRANS_DBG_SLAVE_YUV_TYPE, SVA_TRANS_CONV_IDX(SLV_YUV_TOTAL_TYPE), time0, time1);

            /* 接收主片发送过来的帧参数 */
            s32Ret = Sva_DrvRecvFramePrmByMmap(pFrmExtBuf, u64FrameExtSize);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("dual-chip set data status failed! \n");
                goto putData;
            }

            (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time2);
            (VOID)Sva_DrvTransDbgAddTime(SVA_TRANS_DBG_SLAVE_PARAM_TYPE, SVA_TRANS_CONV_IDX(SLV_PRM_TOTAL_TYPE), time1, time2);

            pstVideoFrmBuf = (SAL_VideoFrameBuf *)pFrmExtBuf;

            stVideoFrmBuf.privateDate = pstVideoFrmBuf->privateDate;
            stVideoFrmBuf.pts = pstVideoFrmBuf->pts;
            stVideoFrmBuf.frameNum = pstVideoFrmBuf->frameNum;
            uiProcGap = (UINT32)pstVideoFrmBuf->reserved[0];

            (VOID)Sva_HalSetSlaveProcGap(uiProcGap);   /* 设置从片的ProcessGap */

            if (SAL_SOK != sys_hal_buildVideoFrame(&stVideoFrmBuf, &pstSysFrameInfo[uiTmpChn]))
            {
                SVA_LOGE("get video frame failed! \n");
                goto putData;
            }

            SAL_LOGD("get---i %u, proc gap %d, Slave get timeref %u, pts %llu, obdCnt %llu, %p, %p \n",
                     i, uiProcGap, stVideoFrmBuf.frameNum, stVideoFrmBuf.pts,
                     stVideoFrmBuf.privateDate, &pstSysFrameInfo[0], &pstSysFrameInfo[1]);

            if (SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode
                && 0 == i
                && 1 != (stVideoFrmBuf.privateDate % Sva_HalGetPdProcGap()))
            {
                break;
            }

            /* export src yuv data */
            if (uiSwitch[1])
            {
                if (0 == cnt % Sva_HalGetPdProcGap())
                {
                    SAL_clear(path);
                    sprintf((CHAR *)path, "./%d_dst_%d_%dx%d.yuv", i, cnt, stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height);

                    (VOID)Sva_HalDebugDumpData((CHAR *)stVideoFrmBuf.virAddr[0], (CHAR *)path, (UINT32)u64FrameSize, 0);
                }
            }
        }

        cnt++;

        /* 调试接口: 从片接收线程统计耗时 */
        (VOID)Sva_DrvSlaveThrCalDbgTime();

        /* TODO:从片没开通道时，释放数据，帧数据不放入   满队列 */
        pstSva_dev1 = Sva_DrvGetDev(0);
        pstSva_dev2 = Sva_DrvGetDev(1);
        if (((SAL_TRUE != pstSva_dev1->uiChnStatus || SAL_TRUE != pstSva_dev2->uiChnStatus) && SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode)
            || (SAL_TRUE != pstSva_dev1->uiChnStatus && SAL_TRUE != pstSva_dev2->uiChnStatus && SVA_PROC_MODE_VIDEO == g_proc_mode))
        {
            SVA_LOGD("ywn:chn0 %d, or chn1 %d not open \n", pstSva_dev1->uiChnStatus, pstSva_dev2->uiChnStatus);
            goto putData;
        }

        DSA_QuePut(pstFullQue, pstSysFrameInfo, SAL_TIMEOUT_NONE);
        continue;

putData:
        if (NULL != pstSysFrameInfo)
        {
            DSA_QuePut(pstEmptQue, pstSysFrameInfo, SAL_TIMEOUT_NONE);
            pstSysFrameInfo = NULL;
        }
    }

EXIT:
    mem_hal_mmzFree(pFrmExtBuf, "SVA", "sva_slv_prm");
    pFrmExtBuf = NULL;

    (VOID)Sva_DrvDeInitSlaveFrameQue();
    SVA_LOGE("Thread %s exit! \n", __FUNCTION__);

    return NULL;
}

/**
 * @function:   Sva_DrvGetFps
 * @brief:      帧率统计
 * @param[in]:  UINT32 *puiStartTime
 * @param[in]:  UINT32 *puiEndTime
 * @param[in]:  UINT32 *puiFps
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvGetFps(UINT32 *puiStartTime, UINT32 *puiEndTime, UINT32 *puiFps)
{
    UINT32 uiTime_0 = 0;
    UINT32 uiTime_1 = 0;
    UINT32 uiFps = 0;

    SVA_DRV_CHECK_PRM(puiStartTime, SAL_FAIL);
    SVA_DRV_CHECK_PRM(puiEndTime, SAL_FAIL);
    SVA_DRV_CHECK_PRM(puiFps, SAL_FAIL);

    uiTime_0 = *puiStartTime;
    uiTime_1 = *puiEndTime;
    uiFps = *puiFps;

    /* 统计线程帧率 */
    if ((uiTime_1 - uiTime_0 >= 990) && (uiFps > 0))
    {
        SVA_LOGW("fps %d, gap %u. \n", uiFps, uiTime_1 - uiTime_0);
        uiFps = 0;
    }

    if (uiFps == 0)
    {
        uiTime_0 = SAL_getCurMs();
    }

    uiTime_1 = SAL_getCurMs();

    *puiStartTime = uiTime_0;
    *puiEndTime = uiTime_1;
    *puiFps = uiFps;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSlaveSyncThread
* 描  述  : 送帧线程
* 输  入  : - prm: 全局变量参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 分析仪使用，双路检测通道均使用此线程，不区分模式(关联or非关联)
*******************************************************************************/
void *Sva_DrvSlaveSyncThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 chan = 0;
    UINT32 i = 0;
    UINT32 fps = 0;
    UINT32 time000 = 0;
    UINT32 time111 = 0;

    UINT32 time00 = 0;
    UINT32 time01 = 0;
    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;
    UINT32 time4 = 0;
    UINT32 uiFrameAmount = 0;
    UINT32 uiFrameNum = 0;                              /*虚拟帧号，用来避免通信过程中第一帧帧号不为0，保证送算法第一帧帧号为0*/

    UINT32 uiInputErrCnt = 0;
    UINT32 uiQueueCnt = 0;
    SVA_PROC_MODE_E enProcMode = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_DEV_PRM *pstSva_dev1 = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SYSTEM_FRAME_INFO *pstSysFrameInfo[SVA_DEV_MAX] = {NULL, NULL};

    UINT32 chnStatus[SVA_DEV_MAX] = {0, 0};             /* Drv层通道创建状态 */
    UINT32 xsiStatus[SVA_DEV_MAX] = {0, 0};             /* Hal层通道创建状态 */

    SVA_PROCESS_IN *pastIn = NULL;

    SVA_DRV_CHECK_PRM(prm, NULL);

    pstSva_dev = (SVA_DEV_PRM *)prm;

    pstSva_dev1 = Sva_DrvGetDev(0);

    chan = pstSva_dev->chan;

    pastIn = SAL_memZalloc(SVA_DEV_MAX * sizeof(SVA_PROCESS_IN), "SVA", "stIn_stk");
    if (NULL == pastIn)
    {
        SVA_LOGE("zalloc failed! \n");
        return NULL;
    }

    SAL_SET_THR_NAME();
    SAL_SetThreadCoreBind(((pstSva_dev->chan + 1) % 2));

    while (1)
    {
        s32Ret = Sva_DrvGetFrameDataFromQue(0, &pstSysFrameInfo[0], &pstSysFrameInfo[1], SAL_TIMEOUT_FOREVER);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Get Data Failed! \n");
            goto reuse;
        }

        if (enProcMode != g_proc_mode)
        {
            uiFrameAmount = 0;
            g_bRsetFlag = SAL_TRUE;
        }

        enProcMode = g_proc_mode;

        time00 = SAL_getCurMs();

        /*可能存在残留帧，丢弃残留帧*/
        if (SVA_MAX_SLAVE_BUFFER_NUM > uiFrameAmount)
        {
            uiFrameAmount++;
            goto reuse;
        }

        SAL_VideoFrameBuf stVideoFrmBuf = {0};

        if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSysFrameInfo[g_sva_common.uiMainViewChn], &stVideoFrmBuf))
        {
            SVA_LOGE("get video frame info failed! \n");
            goto reuse;
        }

        if (SAL_TRUE == g_bRsetFlag)
        {
            /* 取测视角帧号为1 */
            if (0 != (UINT32)stVideoFrmBuf.privateDate % Sva_HalGetPdProcGap())
            {
                goto reuse;
            }
            else
            {
                g_bRsetFlag = SAL_FALSE;
                uiFrameNum = 0;
            }
        }

        /* 虚拟帧号赋值，保证第一帧帧号为0 */
        pstSysFrameInfo[0]->uiDataLen = uiFrameNum;
        pstSysFrameInfo[1]->uiDataLen = uiFrameNum;


        time01 = SAL_getCurMs();

        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time0);

        /* 统计送帧线程帧率 */
        (VOID)Sva_DrvGetFps(&time000, &time111, &fps);

        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time1);

        s32Ret = Sva_DrvGetChnSts(&chnStatus[0]);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Get xsi sts Failed! chan %d \n", chan);
            goto reuse;
        }

        for (i = 0; i < SVA_DEV_MAX; i++)
        {
            xsiStatus[i] = SAL_TRUE;
        }

        for (i = 0; i < SVA_DEV_MAX; i++)
        {
            pstSva_dev = Sva_DrvGetDev(i);
            if (NULL == pstSva_dev)
            {
                SVA_LOGE("pstSva_dev == null! chan %d \n", i);
                goto reuse;
            }

            pstPrmFullQue = &pstSva_dev->stPrmFullQue;
            pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;
            uiQueueCnt = DSA_QueGetQueuedCount(pstPrmFullQue);
            if (uiQueueCnt)
            {
                /* 获取 buffer */
                DSA_QueGet(pstPrmFullQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
                sal_memcpy_s(&pastIn[i], sizeof(SVA_PROCESS_IN), pstIn, sizeof(SVA_PROCESS_IN));
                DSA_QuePut(pstPrmEmptQue, (void *)pstIn, SAL_TIMEOUT_NONE);
            }
        }

        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time2);

        if (SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode
            && (((pstSysFrameInfo[0]->uiDataLen % Sva_HalGetPdProcGap() != 1) && (stVideoFrmBuf.privateDate % Sva_HalGetPdProcGap() == 1))
                || ((pstSysFrameInfo[0]->uiDataLen % Sva_HalGetPdProcGap() == 1) && (stVideoFrmBuf.privateDate % Sva_HalGetPdProcGap() != 1))))
        {
            g_bRsetFlag = SAL_TRUE;
            SVA_LOGW("chan %d, true frmnum %lld != vir frmnum %d \n", i, stVideoFrmBuf.privateDate, pstSysFrameInfo[0]->uiDataLen);
            goto reuse;
        }

        SAL_mutexLock(pstSva_dev1->mChnMutexHdl);

        s32Ret = Sva_HalVcaeSyncPutData(pstSysFrameInfo, pastIn, &chnStatus[0], &xsiStatus[0], g_sva_common.uiMainViewChn, enProcMode);
        if (s32Ret != SAL_SOK)
        {
            uiInputErrCnt++;
            if (uiInputErrCnt > 20)
            {
                SVA_LOGW("chan %d continue !\n", pstSva_dev->chan);
                uiInputErrCnt = 0;
                usleep(10 * 1000);
            }
        }
        else
        {
            g_sva_common.stDbgPrm.stCommSts.uiInputCnt++;
            SVA_LOGD("chan %d Success ! input_cnt %d, cb_cnt %d \n",
                     chan, g_sva_common.stDbgPrm.stCommSts.uiInputCnt, g_sva_common.stDbgPrm.stCommSts.uiOutputCnt);
            uiInputErrCnt = 0;
        }

        fps++;
        uiFrameNum++;

        SAL_mutexUnlock(pstSva_dev1->mChnMutexHdl);

        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time3);

reuse:
        /* 队列数据释放 */
        if (NULL != pstSysFrameInfo[0] && NULL != pstSysFrameInfo[1])
        {
            pstSysFrameInfo[0]->uiDataLen = 0;
            pstSysFrameInfo[1]->uiDataLen = 0;
        }

        if (NULL != pstSysFrameInfo[0])
        {
            s32Ret = Sva_DrvPutFrameDataFromQue(0, pstSysFrameInfo[0], SAL_TIMEOUT_NONE);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("Put Frame Data To Queue Failed! \n");
            }
        }

        pstSysFrameInfo[0] = NULL;
        pstSysFrameInfo[1] = NULL;

        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time4);

        if (Sva_HalGetAlgDbgFlag() && time4 - time0 > 17)
        {
            SZL_DBG_LOG("slave sync2222: from que %d, %d, %d, %d, total: %d \n",
                        time01 - time00, time1 - time0, time2 - time1, time3 - time2, time4 - time0);
        }
    }

    return NULL;
}

/**
 * @function    Sva_DrvCreateSendRsltBusiDev
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvCreateSendRsltBusiDev(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    static BOOL bInit = SAL_FALSE;

    XTRANS_BUSSINESS_MSG_S msg = {0};
    xtrans_msg_module_id_e enModId = XTRANS_MSG_MODULE_VPSS;

    if (SAL_TRUE == bInit)
    {
        return SAL_SOK;
    }

#ifdef XTRANS_DUAL_DMA

    XTRANS_BUSSINESS_STM_S stm = {0};

    stm.u8Func = XTRANS_PROT_STREAM_SEND;
    stm.stmSendOver = Sva_DrvTransRsltWake;

    /* 码流数据传输使用XTRANS_MSG_MODULE_VENC */
    s32Ret = Sva_DrvCreateBusiDev(enModId, &stm, &msg, &g_sva_common.stXtransPrm.pstBusiDevSendRslt);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("create business device failed! \n");
        return SAL_FAIL;
    }

#else

    /* 码流数据传输使用XTRANS_MSG_MODULE_VENC */
    s32Ret = Sva_DrvCreateBusiDev(enModId, NULL, &msg, &g_sva_common.stXtransPrm.pstBusiDevSendRslt);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("create business device failed! \n");
        return SAL_FAIL;
    }

#endif

    /* 该business device已经创建成功 */
    bInit = SAL_TRUE;

    return SAL_SOK;
}

static VOID *g_pTmpRsltBuf = NULL;    /* 用于从xtrans组件中获取msg */

/**
 * @function    Sva_DrvRecgMsgCb0
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvRecgMsgCb0(xtrans_MsgqMsg *pstMsg)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(0);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (NULL == g_pTmpRsltBuf)
    {
        SVA_LOGE("recv rslt buf null! \n");
        goto exit;
    }

    /* 拷贝消息数据到外层业务模块 */
    sal_memcpy_s(g_pTmpRsltBuf, sizeof(SVA_ALG_RSLT_S), (VOID *)((char *)pstMsg->pPrm + XTRANS_MSG_COMM_SIZE), sizeof(SVA_ALG_RSLT_S));

exit:
    Sva_DrvTransRsltWake(NULL);
    return SAL_SOK;
}

/**
 * @function    Sva_DrvRecvRsltCb
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvRecvRsltCb(const void *pstStmInfo /*XTRANS_BUF_HEAD_S*/,
                               xtrans_comm_desc_s stTransFromDesc,   /* 来自的传输模块的设备描述 */
                               const char *pstInfo)
{
    return SAL_SOK;
}

static BOOL bPhyRecvRsltReady = SAL_FALSE;    /* 空闲物理地址状态是否ready */
static PhysAddr PhyRecvRsltBuf = 0;           /* 接收结果的物理缓存 */

/**
 * @function    Sva_DrvMallocRecvRsltBuf
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvMallocRecvRsltBuf(HPR_UINT32 u32Size, intptr_t *aiphyAddr, void **avpVirAddr)
{
    INT32 s32TimeOut = 2000;           /* 等待空闲物理地址超时时间 */

    while (SAL_TRUE != bPhyRecvRsltReady)
    {
        usleep(1000);
        if (s32TimeOut <= 0)
        {
            s32TimeOut = 2000;
            SVA_LOGE("host: time out! wait for free phy over 2s! \n");
        }

        s32TimeOut--;
    }

    *avpVirAddr = NULL;
    *aiphyAddr = PhyRecvRsltBuf;     /* 将物理地址赋值给xtrans组件用于接收结果 */

    return SAL_SOK;
}

/**
 * @function    Sva_DrvFreeRecvRsltBuf
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvFreeRecvRsltBuf(intptr_t aiphyAddr, void *pvVirAddr)
{
    /* 接收失败时的内存释放动作外部管理 */
    bPhyRecvRsltReady = SAL_FALSE;

    /* 唤醒码流接收线程 */
    Sva_DrvTransRsltWake(NULL);

    return SAL_SOK;
}

/**
 * @function   Sva_DrvCreateMasterRecvRsltBusDevice
 * @brief      主片接收结果创建business device
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvCreateMasterRecvRsltBusDevice(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    XTRANS_BUSSINESS_MSG_S msg = {0};

#ifdef XTRANS_DUAL_DMA

    XTRANS_BUSSINESS_STM_S stm = {0};
    xtrans_msg_module_id_e enModId = XTRANS_MSG_MODULE_RSLT;

    stm.u8Func = XTRANS_PROT_STREAM_RECEIVE;
    stm.msgRecvProcess = Sva_DrvRecvRsltCb;
    stm.phyMalloc = Sva_DrvMallocRecvRsltBuf;          /* szl_todo: 向组件内部注册的获取接收数据的内存函数 */
    stm.free = Sva_DrvFreeRecvRsltBuf;                 /* 向组件内部注册的释放接收数据的内存函数 */

    /* 码流数据传输使用XTRANS_MSG_MODULE_VENC */
    s32Ret = Sva_DrvCreateBusiDev(enModId, &stm, &msg, &g_sva_common.stXtransPrm.pstBusiDevSendRslt);
    SVA_DRV_CHECK_RET(SAL_SOK != s32Ret, exit, "Sva_DrvCreateBusiDev failed!");

#else

    xtrans_msg_module_id_e enModId = XTRANS_MSG_MODULE_VPSS;

    msg.recvMsgProcCallback = Sva_DrvRecgMsgCb0;

    /* 码流数据传输使用XTRANS_MSG_MODULE_VENC */
    s32Ret = Sva_DrvCreateBusiDev(enModId, NULL, &msg, &g_sva_common.stXtransPrm.pstBusiDevSendRslt);
    SVA_DRV_CHECK_RET(SAL_SOK != s32Ret, exit, "Sva_DrvCreateBusiDev failed!");

#endif

exit:
    return s32Ret;
}

/**
 * @function    Sva_DrvRecvRsltByDualDma
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvRecvRsltByDualDma(VOID)
{
    if (!g_sva_common.stXtransPrm.pstBusiDevSendRslt)
    {
        if (SAL_SOK != Sva_DrvCreateMasterRecvRsltBusDevice())
        {
            SVA_LOGE("create master recv rslt business device failed! \n");
            return SAL_FAIL;
        }
    }

    Sva_DrvTransRsltWait();
    return SAL_SOK;
}

/**
 * @function    Sva_DrvRecvRsltByMMap
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvRecvRsltByMMap(VOID)
{
    if (!g_sva_common.stXtransPrm.pstBusiDevSendRslt)
    {
        if (SAL_SOK != Sva_DrvCreateMasterRecvRsltBusDevice())
        {
            SVA_LOGE("create master recv rslt business device failed! \n");
            return SAL_FAIL;
        }
    }

    Sva_DrvTransRsltWait();
    return SAL_SOK;
}

/**
 * @function    Sva_DrvSendRsltByMMap
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvSendRsltByMMap(UINT32 chan, VOID *pBuf, UINT32 uiSize)
{
    INT32 s32Ret = SAL_SOK;

    xtrans_comm_desc_s astRemoteDesc[XTRANS_DEVICE_MAX_NUM] = {{0}};
    xtrans_comm_desc_s desc = {0};

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    INT32 s32RemoteCount = XTRANS_DEVICE_MAX_NUM;
    (VOID)xtrans_GetRemoteDesc(astRemoteDesc, &s32RemoteCount);

    desc.u8ChipId = astRemoteDesc[0].u8ChipId;   /* 0表示想要发送msg的对端逻辑标识，而不是chip id */

    if (0 == uiSize)
    {
        SVA_LOGW("send size 0!!!!!!!!!! warning! chip %d \n", desc.u8ChipId);
        return SAL_FAIL;
    }

    s32Ret = g_sva_common.stXtransPrm.pstBusiDevSendRslt->sendMsg(g_sva_common.stXtransPrm.pstBusiDevSendRslt,
                                                                  0x0,
                                                                  desc,
                                                                  XTRANS_MSG_WAIT_ACK,
                                                                  (VOID *)pBuf,
                                                                  uiSize,
                                                                  1);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("send msg failed! chan %d, size %d \n", chan, uiSize);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvSendRsltByDualDma
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvSendRsltByDualDma(PhysAddr PhyBuf, UINT64 u64Size)
{
    INT32 s32Ret = SAL_SOK;

    xtrans_comm_desc_s stRemoteDesc = {0};
    xtrans_buf_info_s stBufInfo = {0};

    stRemoteDesc.u8ChipId = g_sva_common.stXtransPrm.uiToChipId;

    stBufInfo.pvVirAddr = NULL; /* DMA传输无需虚拟地址 */
    stBufInfo.uiptrPhyAddr = PhyBuf;
    stBufInfo.s32BufSize = (INT32)u64Size;

    /* 支持xtrans库进行主从通信 */
    s32Ret = g_sva_common.stXtransPrm.pstBusiDevSendRslt->sendStream(g_sva_common.stXtransPrm.pstBusiDevSendRslt,
                                                                     stRemoteDesc,
                                                                     &stBufInfo,
                                                                     &stbufDataInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("xtrans send stream failed! \n");
        return SAL_FAIL;
    }

    /* 等待xtrans返回发送结束信号 */
    Sva_DrvTransRsltWait();

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvGetFrmData
 * @brief:      按照输入的宽高拷贝帧数据
 * @param[in]:  SAL_VideoFrameBuf *pstVideoFrmBuf
 * @param[in]:  UINT32 u32XpackBufMaxW
 * @param[in]:  UINT32 u32XpackBufMaxH
 * @param[out]: INT8 *pCopyFrmVirAddr
 * @return:     INT32
 */
INT32 Sva_DrvCopyFrmData(SAL_VideoFrameBuf *pstVideoFrmBuf, UINT32 u32XpackBufMaxW, UINT32 u32XpackBufMaxH, INT8 *pCopyFrmVirAddr)
{
    INT32 s32Ret = SAL_SOK;
	
	XPACK_IMG stFrameDataXpackCopySrc = {0};
    XPACK_IMG stFrameDataXpackCopyDst = {0};

	stFrameDataXpackCopySrc.picPrm.u32Width = pstVideoFrmBuf->frameParam.width;
	stFrameDataXpackCopySrc.picPrm.u32Height = pstVideoFrmBuf->frameParam.height;
	stFrameDataXpackCopySrc.picPrm.u32Stride = pstVideoFrmBuf->stride[0];
	stFrameDataXpackCopySrc.picPrm.phyAddr[0] = pstVideoFrmBuf->phyAddr[0];
	stFrameDataXpackCopySrc.picPrm.VirAddr[0] = (VOID *)pstVideoFrmBuf->virAddr[0];
	stFrameDataXpackCopySrc.picPrm.VirAddr[1] = (VOID *)pstVideoFrmBuf->virAddr[1];
	stFrameDataXpackCopySrc.picPrm.enPixFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
	stFrameDataXpackCopySrc.picPrm.vbBlk = pstVideoFrmBuf->vbBlk;

	stFrameDataXpackCopyDst.picPrm.u32Width = pstVideoFrmBuf->frameParam.width;
	stFrameDataXpackCopyDst.picPrm.u32Stride = u32XpackBufMaxW;
	stFrameDataXpackCopyDst.picPrm.u32Height = pstVideoFrmBuf->frameParam.height;
	//stFrameDataXpackCopyDst.picPrm.phyAddr[0] = u64Pa;/*软拷贝不需要物理地址*/
	stFrameDataXpackCopyDst.picPrm.VirAddr[0] = pCopyFrmVirAddr;
	stFrameDataXpackCopyDst.picPrm.VirAddr[1] = pCopyFrmVirAddr + u32XpackBufMaxW * u32XpackBufMaxH;
	stFrameDataXpackCopyDst.picPrm.enPixFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
	stFrameDataXpackCopyDst.picPrm.vbBlk = 0;

	/*将背景设置为白色*/
	memset((char *)pCopyFrmVirAddr, 0xFF, u32XpackBufMaxW * u32XpackBufMaxH);
	memset((char *)pCopyFrmVirAddr + u32XpackBufMaxW * u32XpackBufMaxH, 0x80, u32XpackBufMaxW * u32XpackBufMaxH / 2);
	/*copy图片*/
	s32Ret = SAL_halDataCopy(&stFrameDataXpackCopySrc, &stFrameDataXpackCopyDst);
	if (SAL_SOK != s32Ret)
	{
		SVA_LOGE("Data copy error! ret: %d \n", s32Ret);
		return SAL_FAIL;
	}	
	return SAL_SOK;
}

//#define DEBUG_SVA_SCALE

/**
 * @function:   Sva_DrvScaleFrame
 * @brief:      缩放帧数据到1280*1024
 * @param[in]:  SYSTEM_FRAME_INFO *pstScaleframeSrc
 * @param[out]: SYSTEM_FRAME_INFO *pstScaleframeDst
 * @return:     INT32
 */
INT32 Sva_DrvScaleFrame(SYSTEM_FRAME_INFO *pstScaleframeSrc, SYSTEM_FRAME_INFO *pstScaleframeDst)
{
    SAL_VideoFrameBuf stVideoFrameBuf = {0};

    SVA_DRV_CHECK_PRM(pstScaleframeSrc, SAL_FAIL);
	SVA_DRV_CHECK_PRM(pstScaleframeDst, SAL_FAIL);
	
#ifdef DEBUG_SVA_SCALE
	if (SAL_SOK != sys_hal_getVideoFrameInfo(pstScaleframeSrc, &stVideoFrameBuf))
	{
		SVA_LOGE("get video frame failed! \n");
		goto err;
	}

	Sva_HalDebugDumpData((VOID *)stVideoFrameBuf.virAddr[0],
						 "./before_scale.nv21",
						 stVideoFrameBuf.frameParam.width * stVideoFrameBuf.frameParam.height * 3 / 2,
						 1);

	SVA_LOGE("wxl_dbg1111: w:%d h:%d,stride[%d %d %d], dataFormat:%d, vir s0 %d, vir s1 %d\n",
			 stVideoFrameBuf.frameParam.width, stVideoFrameBuf.frameParam.height,
			 stVideoFrameBuf.stride[0], stVideoFrameBuf.stride[1], stVideoFrameBuf.stride[2],
			 stVideoFrameBuf.frameParam.dataFormat,
			 stVideoFrameBuf.vertStride[0], stVideoFrameBuf.vertStride[1]);
	
#endif
	if (SAL_SOK != sys_hal_getVideoFrameInfo(pstScaleframeDst, &stVideoFrameBuf))
	{
		SVA_LOGE("get video frame failed! \n");
		goto err;
	}

	memset((VOID *)stVideoFrameBuf.virAddr[0], 0x00, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2);

	if (SAL_SOK != vgs_hal_scaleFrame(pstScaleframeDst, pstScaleframeSrc, SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT))
	{
		SVA_LOGE("vgs scale frame failed! src_w %d, src_h %d, dst_w %d, dst_h %d \n",
				 SVA_BUF_WIDTH_MAX, SVA_BUF_HEIGHT_MAX, SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT);
		goto err;
	}
	
#ifdef DEBUG_SVA_SCALE
	if (SAL_SOK != sys_hal_getVideoFrameInfo(pstScaleframeDst, &stVideoFrameBuf))
	{
		SVA_LOGE("get video frame failed! \n");
		goto err;
	}

	SVA_LOGE("wxl_dbg: w:%d h:%d,stride[%d %d %d], dataFormat:%d, vir s0 %d, vir s1 %d \n",
			 stVideoFrameBuf.frameParam.width, stVideoFrameBuf.frameParam.height,
			 stVideoFrameBuf.stride[0], stVideoFrameBuf.stride[1], stVideoFrameBuf.stride[2],
			 stVideoFrameBuf.frameParam.dataFormat,
			 stVideoFrameBuf.vertStride[0], stVideoFrameBuf.vertStride[1]);

	Sva_HalDebugDumpData((VOID *)stVideoFrameBuf.virAddr[0],
						 "./after_scale.nv21",
						 SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2,
						 1);
#endif
    return SAL_SOK;
err:
	return SAL_FAIL;


}

/**
 * @function:   Sva_DrvFrmInfoPrcess
 * @brief:      帧数据后处理，是否缩放
 * @param[in]:  UINT32 u32XpackBufMaxW
 * @param[in]:  UINT32 u32XpackBufMaxH
 * @param[in]:  INT8 *pFrmVirAddr
 * @param[in]:  SYSTEM_FRAME_INFO *pstFrameDataSrc
 * @param[out]: SYSTEM_FRAME_INFO *pstFrameDataDst
 * @return:     INT32
 */
INT32 Sva_DrvFrmDataPrcess(UINT32 u32XpackBufMaxW, UINT32 u32XpackBufMaxH, INT8 *pFrmVirAddr, SYSTEM_FRAME_INFO *pstFrameDataSrc, SYSTEM_FRAME_INFO *pstFrameDataDst)
{

    INT32 s32Ret = SAL_SOK;
	SAL_VideoFrameBuf stVideoFrmBuf = {0};
	SAL_VideoFrameBuf stVideoTmpFrmBuf = {0};

	SVA_DRV_CHECK_PRM(pstFrameDataSrc, SAL_FAIL);
	SVA_DRV_CHECK_PRM(pstFrameDataDst, SAL_FAIL);


	/* 若xpack包裹宽高不等于当前算法模型要求的1280x1024，则需要放缩，目前依照高来判断 */
	if (SVA_MODULE_HEIGHT < u32XpackBufMaxH || SVA_MODULE_WIDTH < u32XpackBufMaxW)
	{
        /*缩放帧组帧进行缩放*/
		if (SAL_SOK != sys_hal_getVideoFrameInfo(pstFrameDataSrc, &stVideoFrmBuf))
		{
			SVA_LOGE("get video frame failed! \n");
			return SAL_FAIL;
		}
		
		memset(&stVideoFrmBuf, 0x00, sizeof(SAL_VideoFrameBuf));
		
		stVideoFrmBuf.frameParam.width = u32XpackBufMaxW;
		stVideoFrmBuf.frameParam.height = u32XpackBufMaxH;
		stVideoFrmBuf.stride[0] = stVideoFrmBuf.stride[1] = u32XpackBufMaxW;
		stVideoFrmBuf.virAddr[0] = (PhysAddr)pFrmVirAddr;
		stVideoFrmBuf.virAddr[1] = stVideoFrmBuf.virAddr[0] + u32XpackBufMaxW * u32XpackBufMaxH;
		stVideoFrmBuf.virAddr[2] = stVideoFrmBuf.virAddr[1];
		stVideoFrmBuf.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;
		if (SAL_SOK != sys_hal_buildVideoFrame(&stVideoFrmBuf, pstFrameDataSrc))
		{
			SVA_LOGE("build video frame failed! \n");
			return SAL_FAIL;
		}
		
		s32Ret = Sva_DrvScaleFrame(pstFrameDataSrc, pstFrameDataDst);
		if (SAL_SOK != s32Ret)
		{
			SVA_LOGE(" Scale frame failed! \n");
			return SAL_FAIL;
		}
	}
	else
	{
		/*无需缩放直接copy帧数据*/
		if (SAL_SOK != sys_hal_getVideoFrameInfo(pstFrameDataDst, &stVideoTmpFrmBuf))
		{
			SVA_LOGE("get video frame failed! \n");
			return SAL_FAIL;
		}	
		sal_memcpy_s((VOID *)stVideoTmpFrmBuf.virAddr[0], SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2, pFrmVirAddr, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2);
	}
	return SAL_SOK;
}

/**
 * @function:   Sva_DrvGetFrmInfo
 * @brief:      获取帧数据(包含拷贝，缩放过程)
 * @param[in]:  UINT32 chan
 * @param[in]:  SYSTEM_FRAME_INFO *pstRecXpackSysFrame
 * @param[in]:  INT8 *pCopyFrmVirAddr
 * @param[out]: SYSTEM_FRAME_INFO *pstSysScaleFrameInfo[XSI_DEV_MAX]
 * @return:     INT32
 */
INT32 Sva_DrvGetFrmInfo(UINT32 chan, SYSTEM_FRAME_INFO *pstRecXpackSysFrame, SYSTEM_FRAME_INFO *pstCopyFrame, SYSTEM_FRAME_INFO *pstSysScaleFrameInfo[XSI_DEV_MAX])
{

	INT32 s32Ret = SAL_SOK;
	INT8 *pCopyFrmVirAddr = NULL;
	
	/*为保证copy出来图片的宽高比，与1280,1024比例相同，以宽高中最大的为标准进行计算*/
    UINT32 u32XpackBufMaxW = 0; /* 用于保存从xpack中copy出来的有效包裹的最大宽度，最大不能超过1600*/
    UINT32 u32XpackBufMaxH = 0; /* 用于保存从xpack中copy出来的有效包裹的最大高度，最大不能超过1280*/
    float fRatio = 0.0; /*需要缩放的比例*/

	SAL_VideoFrameBuf stVideoFrmBuf = {0};
	SAL_VideoFrameBuf stVideoTmpFrmBuf = {0};
 
	SVA_DRV_CHECK_PRM(pstRecXpackSysFrame, SAL_FAIL);
	SVA_DRV_CHECK_PRM(pstCopyFrame, SAL_FAIL);
	SVA_DRV_CHECK_PRM(pstSysScaleFrameInfo, SAL_FAIL);
	
    /*获取copy缓存地址，用于copyXpack传来的yuv*/
	if (SAL_SOK != sys_hal_getVideoFrameInfo(pstCopyFrame, &stVideoFrmBuf))
	{
		SVA_LOGE("get video frame failed! \n");
		return SAL_FAIL;
	}
	pCopyFrmVirAddr = (INT8 *)stVideoFrmBuf.virAddr[0];
	
	if (SAL_SOK != sys_hal_getVideoFrameInfo(pstRecXpackSysFrame, &stVideoFrmBuf))
	{
		SVA_LOGE("get video frame failed! \n");
		return SAL_FAIL;
	}
	
	/*打印帧信息相关参数*/
	SVA_LOGI("FrameInfo(From Xpack): chan_%d stride[0]_%d width_%d height_%d dataFormat_%d frameNum_%d pts_%llu \n", chan, \
				 stVideoFrmBuf.stride[0], stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height, stVideoFrmBuf.frameParam.dataFormat, stVideoFrmBuf.frameNum, stVideoFrmBuf.pts);

	/*xpack送过来的最大包裹宽高的获取,主要在需要缩放的机型中使用,如果有效宽高都小于1280和1024，u32XpackBufMaxW和u32XpackBufMaxH则将分别赋值为1280和1024*/
	if (stVideoFrmBuf.frameParam.width > SVA_MODULE_WIDTH || stVideoFrmBuf.frameParam.height > SVA_MODULE_HEIGHT)
	{
		/*获取xpack送过来的最大包裹宽高,主要在需要缩放的机型中使用*/
		u32XpackBufMaxH = stVideoFrmBuf.frameParam.height;
		fRatio = (float)stVideoFrmBuf.frameParam.height / SVA_MODULE_HEIGHT;
		u32XpackBufMaxW = SVA_MODULE_WIDTH * fRatio;
		/*若有效宽，大于有效高计算出来的最大宽，则以有效宽为标准，计算最大高*/
		if (stVideoFrmBuf.frameParam.width > u32XpackBufMaxW)
		{
			u32XpackBufMaxW = stVideoFrmBuf.frameParam.width;
			fRatio = (float)stVideoFrmBuf.frameParam.width / SVA_MODULE_WIDTH;
			u32XpackBufMaxH = SVA_MODULE_HEIGHT * fRatio;
		}
	}
	else
	{
		u32XpackBufMaxW = SVA_MODULE_WIDTH;
		u32XpackBufMaxH = SVA_MODULE_HEIGHT;
		fRatio = 1.0;
	}

	/* 调试接口，dump获取xpack传进来的图像 */
	Sva_HalDumpInputImg((CHAR *)stVideoFrmBuf.virAddr[0],
						stVideoFrmBuf.stride[0],
						stVideoFrmBuf.frameParam.height,
						INPUT_IMG_DUMP_PATH, chan,
						stVideoFrmBuf.pts,
						SVA_DUMP_YUV_XPACK);

	/*拷贝帧数据*/
    s32Ret = Sva_DrvCopyFrmData(&stVideoFrmBuf, u32XpackBufMaxW, u32XpackBufMaxH, pCopyFrmVirAddr);
	if (SAL_SOK != s32Ret)
	{
		SVA_LOGE("Copy Frame Data Failed! \n");
		return SAL_FAIL;
	}

	/* 调试接口，dump从xpack中copy出来的yuv图片 */
	Sva_HalDumpInputImg((CHAR *)pCopyFrmVirAddr,
						u32XpackBufMaxW,
						u32XpackBufMaxH,
						INPUT_IMG_DUMP_PATH, chan,
						stVideoFrmBuf.pts,
						SVA_DUMP_YUV_COPY_XPACK);
	
    /*帧数据后处理，是否缩放*/
	s32Ret = Sva_DrvFrmDataPrcess(u32XpackBufMaxW, u32XpackBufMaxH, pCopyFrmVirAddr, pstCopyFrame, pstSysScaleFrameInfo[chan]);
	if (SAL_SOK != s32Ret)
	{
		SVA_LOGE("Frame Data Prcess Failed! \n");
		return SAL_FAIL;
	}
		
	/*获取有效宽高以及帧号*/
	if (SAL_SOK != sys_hal_getVideoFrameInfo(pstRecXpackSysFrame, &stVideoFrmBuf))
	{
		SVA_LOGE("get video frame failed! \n");
		return SAL_FAIL;
	}
	if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSysScaleFrameInfo[chan], &stVideoTmpFrmBuf))
	{
		SVA_LOGE("get video frame failed! \n");
		return SAL_FAIL;
	}
	stVideoTmpFrmBuf.frameParam.height = stVideoFrmBuf.frameParam.height * SVA_MODULE_HEIGHT/ u32XpackBufMaxH;
	stVideoTmpFrmBuf.frameParam.width = stVideoFrmBuf.frameParam.width * SVA_MODULE_WIDTH / u32XpackBufMaxW;
	stVideoTmpFrmBuf.frameNum = stVideoFrmBuf.frameNum;	
	stVideoTmpFrmBuf.pts = stVideoFrmBuf.pts;
	if (SAL_SOK != sys_hal_buildVideoFrame(&stVideoTmpFrmBuf, pstSysScaleFrameInfo[chan]))
	{
		SVA_LOGE("build video frame failed! \n");
		return SAL_FAIL;
	}
    /*打印缩放前xpack送过来的最大包裹宽高，缩放后有效包裹宽高，缩放率，帧号*/
	SVA_LOGI("Frame Scale Info: chan_%d, u32XpackBufMaxW_%d u32XpackBufMaxH_%d After scale:w_%d: h_%d ratio_%f frameNum_%d pst_%llu\n", chan, \
			 u32XpackBufMaxW, u32XpackBufMaxH, stVideoTmpFrmBuf.frameParam.width, stVideoTmpFrmBuf.frameParam.height, fRatio, stVideoTmpFrmBuf.frameNum, stVideoTmpFrmBuf.pts);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvVcaeThread_ISM
* 描  述  : 安检机单芯片智能检测线程
* 输  入  : - prm: 全局变量
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 安检机使用，基于图片流模式从包裹分割获取yuv
*******************************************************************************/
static void *Sva_DrvVcaeThread_ISM(void *prm)
{
    INT32 s32Ret = SAL_SOK;
	
	/*计算耗时*/
    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
	UINT32 time3 = 0;
	
    UINT32 chnStatus[SVA_DEV_MAX] = {0, 0};             /* Drv通道创建状态 */
    UINT32 xsiStatus[SVA_DEV_MAX] = {0, 0};             /* Hal通道创建状态 */
    UINT32 uiQueueCnt = 0; 
	UINT32 chan = 0, i = 0;
	
	CAPB_AI *pstIsmSvaCapb = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PROCESS_IN *pstTmpIn = NULL;
	SVA_PROCESS_IN *pstIn = NULL;     /* SVA_DEV_MAX 此处是否需要设成一个指针数组?*/
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL; 
    SYSTEM_FRAME_INFO *pstSysScaleFrameInfo[SVA_DEV_MAX] = {NULL, NULL};
	

	SYSTEM_FRAME_INFO stRecXpackSysFrame = {0};
	SYSTEM_FRAME_INFO stCopyFrame = {0};
	
    SVA_DRV_CHECK_PRM(prm, NULL);

    pstSva_dev = (SVA_DEV_PRM *)prm;
    chan = pstSva_dev->chan;
    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue; 
	
    /*获取能力集，打印智能通道个数*/
    pstIsmSvaCapb = capb_get_ai();
    if (NULL == pstIsmSvaCapb)
    {
        SVA_LOGE("Err!ai_cap is NULL!\n");
        return NULL;
    }
    SVA_LOGI("The ai_chn is %d \n", (INT32)pstIsmSvaCapb->ai_chn);

	/*申请xpack送来的帧信息内存*/
    s32Ret = sys_hal_allocVideoFrameInfoSt(&stRecXpackSysFrame);
	if (SAL_SOK != s32Ret)
	{
		SVA_LOGE("alloc video frame info st failed! \n");
		goto err;
	}

    /*算法引擎参数结构体申请缓存*/
    pstIn = SAL_memZalloc(SVA_DEV_MAX * sizeof(SVA_PROCESS_IN), "sva", "stIn_stk");
    if (NULL == pstIn)
    {
        SVA_LOGE("zalloc failed! \n");
        goto err;
    }
	
	/*用于从xpack中获取的帧数据进行拷贝以及缩放申请的缓存*/	 
	s32Ret = Sva_DrvGetFrameMem(SVA_BUF_WIDTH_MAX, SVA_BUF_WIDTH_MAX, SAL_FALSE, &stCopyFrame);
	if (s32Ret != SAL_SOK)
	{
		SVA_LOGE("Sva_DrvGetFrameMem error !!!\n");
		goto err;
	}

    /*申请缩放使用缓存，完成图片的缩放后送进引擎*/
    for (i = 0; i < SVA_DEV_MAX; i++)
    {
        if (NULL == pstSysScaleFrameInfo[i])
        {
            pstSysScaleFrameInfo[i] =  SAL_memMalloc(sizeof(SYSTEM_FRAME_INFO), "SVA", "sva_frameinfo");
            if (NULL == pstSysScaleFrameInfo[i])
            {
                SVA_LOGE("malloc failed! i %d \n", i);
                goto err;
            }

            s32Ret = Sva_DrvGetFrameMem(SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, SAL_FALSE, pstSysScaleFrameInfo[i]);
            if (s32Ret != SAL_SOK)
            {
                SVA_LOGE("Sva_DrvGetFrameMem error !!!\n");
                goto err;
            }
        }
    }

    SAL_SET_THR_NAME();

    while (1)
    {
        if (SAL_TRUE != pstSva_dev->uiChnStatus)
        {
            SAL_msleep(80);
            continue;
        }

        /* 参数改变时 */ /* TODO:是否应该放在智能通道开启前?避免没开智能通道设置参数，导致队列无法及时更新或者满了 */
        uiQueueCnt = DSA_QueGetQueuedCount(pstPrmFullQue);
        if (uiQueueCnt)
        {
            /* 获取 buffer */
            DSA_QueGet(pstPrmFullQue, (void **)&pstTmpIn, SAL_TIMEOUT_FOREVER);
            sal_memcpy_s(&pstIn[chan], sizeof(SVA_PROCESS_IN), pstTmpIn, sizeof(SVA_PROCESS_IN));
            DSA_QuePut(pstPrmEmptQue, (void *)pstTmpIn, SAL_TIMEOUT_NONE);
        }
		(VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time0);

        /*获取xpack传来的帧信息*/
		s32Ret = xpack_package_launch_aidgr(chan, &stRecXpackSysFrame);
		if (SAL_FAIL == s32Ret)
		{
			SVA_LOGD("Receive no XpackSysFrame !!!! \n");
			SAL_msleep(80);
			continue;
	    }

		SVA_LOGI("Receive the XpackSysFrame !!!! \n");
		
		(VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time1);

        /*记录从xpack中获取到帧数据和帧参数的时间,用于一帧图像在sva模块整体耗时*/
        if (0 == pstSva_dev->chan)
        {
            uiEngineStartTime0 = SAL_getCurMs();
        }
		
        /*获取帧信息(帧数据，帧参数)，包含copy，缩放过程*/
		s32Ret = Sva_DrvGetFrmInfo(chan, &stRecXpackSysFrame, &stCopyFrame, pstSysScaleFrameInfo);
		if (SAL_FAIL == s32Ret)
		{
			SVA_LOGE("Get The Famedata Failed!!!! \n");
			SAL_msleep(80);
			continue;
		}
        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time2);

		/* 通道状态互斥:为了图片模式推帧时保证只推送一路通道的数据 */
        chnStatus[chan] = pstSva_dev->uiChnStatus;
        chnStatus[1 - chan] = SAL_FALSE;

		xsiStatus[chan] = SAL_TRUE;
		xsiStatus[1 - chan] = SAL_FALSE;

#if 0 /*将对应图片直接送入引擎，用于测试误检问题*/
		FILE *yuvfile = NULL;
		unsigned int n = 0;
		SAL_VideoFrameBuf stVideoFrameBuf = {0};

		yuvfile = fopen("./source/engine_frm_5905580032552_ch_0_w_1280_h_1024.nv21", "rb");
		if (NULL == yuvfile)
		{
			printf("open file yuv failed !!!\n");

		}
		sys_hal_getVideoFrameInfo(pstSysScaleFrameInfo[chan], &stVideoFrameBuf);
		n = fread((CHAR *)stVideoFrameBuf.virAddr[0], 1, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2, yuvfile);
		stVideoFrameBuf.frameParam.height = (n == 0) ? 0 : 448;
		stVideoFrameBuf.frameParam.width = (n == 0) ? 0 : 552;
		sys_hal_buildVideoFrame(&stVideoFrameBuf,pstSysScaleFrameInfo[chan]);
		fclose(yuvfile);
#endif

        /*将缩放完的图片(有的机型不需要缩放)，算法参数，算法通道开关状态送入推帧接口*/
        s32Ret = Sva_HalVcaeSyncPutData(pstSysScaleFrameInfo, &pstIn[0], &chnStatus[0], &xsiStatus[0], g_sva_common.uiMainViewChn, SVA_PROC_MODE_IMAGE);
        if (SAL_SOK != s32Ret)
        {
        	SVA_LOGE("Put Data To Engine Failed!!!! \n");
            continue;
        }
		
		(VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time3);
		SVA_LOGD("Wxl_dbg: Rec Xpack frame info cost: %d ms! get frame info cost time %d ms total :%d\n", time1 - time0, time2 - time1, time3 - time0);
    }

err:
	if(0x00 != (&stRecXpackSysFrame)->uiAppData)
	{
		(void)sys_hal_rleaseVideoFrameInfoSt(&stRecXpackSysFrame);
	}

    if (NULL != pstIn)
    {
        SAL_memfree(pstIn, "sva", "stIn_stk");
        pstIn = NULL;
    }

	(void)Sva_DrvPutFrameMem(&stCopyFrame);

    for (i = 0; i < SVA_DEV_MAX; i++)
    {
        (void)Sva_DrvPutFrameMem(pstSysScaleFrameInfo[i]);
        if (NULL != pstSysScaleFrameInfo[i])
        {
			SAL_memfree(pstSysScaleFrameInfo[i], "SVA", "sva_frameinfo");
            pstSysScaleFrameInfo[i] = 0;
        }
    }

    return NULL;
}

/**
 * @function:   Sva_DrvGetTransFramePrm
 * @brief:      获取传输帧参数
 * @param[in]:  UINT32 chan
 * @param[in]:  SYSTEM_FRAME_INFO *pstRecXpackSysFrame
 * @param[out]: SVA_TRANS_FRAME_PRM *pstFramePrm
 * @return:     INT32
 */
INT32 Sva_DrvGetTransFramePrm(UINT32 chan, SYSTEM_FRAME_INFO *pstRecXpackSysFrame, SVA_TRANS_FRAME_PRM *pstFramePrm)
{
    float fratio = 0.0;
	SAL_VideoFrameBuf stVideoFrmBuf = {0};
	
    if (SAL_SOK != sys_hal_getVideoFrameInfo(pstRecXpackSysFrame, &stVideoFrmBuf))
	{
	    SVA_LOGE("get video frame failed! \n");
	    return SAL_FAIL;
	}
	
	pstFramePrm->uichan = chan;
	pstFramePrm->uiPkgWid = stVideoFrmBuf.frameParam.width;
	pstFramePrm->uiPKgHei = stVideoFrmBuf.frameParam.height;
	pstFramePrm->uiFrameNum = stVideoFrmBuf.frameNum;
	pstFramePrm->u64pts = stVideoFrmBuf.pts;
	
    /*获取xpack送过来的最大包裹宽高,主要在需要缩放的机型中使用,如果有效宽高都小于1280和1024，u32XpackBufMaxW和u32XpackBufMaxH则将分别赋值为1280和1024*/
    if (stVideoFrmBuf.frameParam.width > SVA_MODULE_WIDTH || stVideoFrmBuf.frameParam.height > SVA_MODULE_HEIGHT)
    {
        pstFramePrm->uiXpackBufMaxH = stVideoFrmBuf.frameParam.height;
        fratio = (float)stVideoFrmBuf.frameParam.height / SVA_MODULE_HEIGHT;
        pstFramePrm->uiXpackBufMaxW = SVA_MODULE_WIDTH * fratio;
        /*若有效宽，大于有效高计算出来的最大宽，则以有效宽为标准，计算最大高*/
        if (stVideoFrmBuf.frameParam.width > pstFramePrm->uiXpackBufMaxW)
        {
            pstFramePrm->uiXpackBufMaxW = stVideoFrmBuf.frameParam.width;
            fratio = (float)stVideoFrmBuf.frameParam.width / SVA_MODULE_WIDTH;
            pstFramePrm->uiXpackBufMaxH = SVA_MODULE_HEIGHT * fratio;
        }
    }
    else
    {
        pstFramePrm->uiXpackBufMaxW = SVA_MODULE_WIDTH;
        pstFramePrm->uiXpackBufMaxH = SVA_MODULE_HEIGHT;
        fratio = 1.0;
    }
	
	SVA_LOGI("Trans Frame Prm (master send): uichan_%d uiPkgWid_%d uiPKgHei_%d uiXpackBufMaxW_%d uiXpackBufMaxH_%d uiFrameNum_%d u64pts_%llu\n",pstFramePrm->uichan,\
		pstFramePrm->uiPkgWid, pstFramePrm->uiPKgHei, pstFramePrm->uiXpackBufMaxW, pstFramePrm->uiXpackBufMaxH, pstFramePrm->uiFrameNum, pstFramePrm->u64pts);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvMasterSendFrmInfo
 * @brief:      主片发送帧信息
 * @param[in]:  SVA_TRANS_FRAME_PRM *pstFramePrm
 * @param[in]:  UINT64 u64FrmPrmSize
 * @param[in]:  UINT64 u64FrmPhysAddr
 * @param[out]: NULL
 * @return:     INT32
 */
INT32 Sva_DrvMasterSendFrmInfo(SVA_TRANS_FRAME_PRM *pstFramePrm, UINT64 u64FrmPrmSize, UINT64 u64FrmPhysAddr)
{
    INT32 s32Ret = 0;
	
	UINT32 uiType = 0;
	
    TRANS_HAL_DATA_INFO stTransDataInfo = {0};
    TRANS_HAL_DATA_INFO stTransSetDmaPhyaddr = {0};

    /*主从片通过mmap传输帧参数(主片发送)*/
	uiType = 0;
	uiType = uiType | 0x0 | 0 << 4;    /* mmap */
	
	/*发送传输帧参数*/
	stTransDataInfo.uiArgCnt = 4;
	stTransDataInfo.u64Arg[0] = u64FrmPrmSize;
	stTransDataInfo.u64Arg[1] = (UINT64)pstFramePrm;
	stTransDataInfo.u64Arg[2] = uiType;
	s32Ret = Trans_HalCmdProc(TRANS_HAL_SEND_DATA, &stTransDataInfo);
	if (SAL_SOK != s32Ret)
	{
		SVA_LOGE("dual_chip send data failed! size %llu \n", u64FrmPrmSize);
		return SAL_FAIL;
	}

	/*主从片dma传输帧数据*/

	/*设置使用dma主从传输时的物理地址，等待从片发起接收*/
	stTransSetDmaPhyaddr.uiArgCnt = 2;
	stTransSetDmaPhyaddr.u64Arg[0] = 2; /* 帧数据传输 */
	stTransSetDmaPhyaddr.u64Arg[1] = u64FrmPhysAddr; /* stVideoFrameBuf.phyAddr[0];//主片物理地址给从片; */
	s32Ret = Trans_HalCmdProc(TRANS_HAL_SET_DMA_PHYADDR, &stTransSetDmaPhyaddr);
	if (SAL_SOK != s32Ret)
	{
		SVA_LOGE("Set dma phyaddr error! ret: %d \n", s32Ret);
		return SAL_FAIL;
	}
	
	return SAL_SOK;

}

/**
 * @function    Sva_DrvVcaMasterThread_ISM
 * @brief         安检机双芯片主片线程
 * @param[in]  void *prm
 * @param[out] NULL
 * @return  NULL
 */
void *Sva_DrvVcaMasterThread_ISM(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    UINT64 u64FrmPrmSize = sizeof(SVA_TRANS_FRAME_PRM);   /* 传输帧参数尺寸 */
	UINT64 u64TransFrmDataSize = SVA_BUF_WIDTH_MAX * SVA_BUF_HEIGHT_MAX * 3 / 2; /*申请大缓存区，兼容140100*/
    UINT64 u64FrmPhysAddr = 0;
	//UINT64 u64MbBlk = 0;

    void *pFrmVirAddr = NULL;
	CAPB_AI *pstIsmSvaCapb = NULL;
	SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PROCESS_IN *pstIn = NULL;
    
    TRANS_HAL_DATA_INFO stTransSetDataStaus = {0};
	TRANS_HAL_DATA_INFO stTransCheckDataStaus = {0};
	SYSTEM_FRAME_INFO stRecXpackSysFrame = {0};
	SAL_VideoFrameBuf stVideoFrmBuf = {0};
	SVA_TRANS_FRAME_PRM stFramePrm = {0};
	ALLOC_VB_INFO_S stVbInfo = {0};

    SVA_DRV_CHECK_PRM(prm, NULL);
    pstSva_dev = (SVA_DEV_PRM *)prm;

    pstIsmSvaCapb = capb_get_ai();
    if (NULL == pstIsmSvaCapb)
    {
        SVA_LOGE("Err!ai_cap is NULL!\n");
        return NULL;
    }
    /*申请xpack送来的帧信息内存*/
    s32Ret = sys_hal_allocVideoFrameInfoSt(&stRecXpackSysFrame);
	if (SAL_SOK != s32Ret)
	{
		SVA_LOGE("alloc video frame info st failed! \n");
		goto err;
	}

    /*申请传输帧数据所需要的连续物理内存*/
    s32Ret = mem_hal_cmaMmzAlloc(u64TransFrmDataSize, "SVA", "debug_cma", NULL, SAL_FALSE, &stVbInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("alloc cma error! ret: %d \n", s32Ret);
        goto err;
    }
    /*获取申请的连续物理内存的物理地址，虚拟地址以及u64VbBlk*/
    u64FrmPhysAddr = stVbInfo.u64PhysAddr;
    pFrmVirAddr = stVbInfo.pVirAddr;
	//u64MbBlk = stVbInfo.u64VbBlk;
    if (NULL == pFrmVirAddr || 0 == u64FrmPhysAddr )
    {
        SVA_LOGE("The pFrmVirAddr(%p) or u64FrmPhysAddr(%llu) is NULL\n",pFrmVirAddr,u64FrmPhysAddr);
        goto err;
    }

    SAL_SET_THR_NAME();
    SAL_SetThreadCoreBind(((pstSva_dev->chan + 1) % 2));

    /* 主片发送数据给到从片 */
    while (1)
    {
        /*由于sva在从片上跑，需要对主片中的参数队列进行消耗，相关参数设置在从片上生效*/
        for (i = 0; i < pstIsmSvaCapb->ai_chn; i++)
        {
            pstSva_dev = Sva_DrvGetDev(i);
            if (NULL == pstSva_dev)
            {
                SVA_LOGE("pstSva_dev is null\n");
				SAL_msleep(80);
                continue;
            }

            if (SAL_TRUE != pstSva_dev->uiChnStatus)
            {
                SVA_LOGD("pstSva_dev is %p or chn close %d! chan %d \n", pstSva_dev, pstSva_dev->uiChnStatus, i);
				SAL_msleep(80);
                continue;
            }

            /* 主片需要将参数队列中堆积的缓存消耗掉，避免配置失败 */
            while (DSA_QueGetQueuedCount(&pstSva_dev->stPrmFullQue))
            {
                /* 主片不需要使用相关参数，直接消耗掉 */
                DSA_QueGet(&pstSva_dev->stPrmFullQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
                DSA_QuePut(&pstSva_dev->stPrmEmptQue, (void *)pstIn, SAL_TIMEOUT_NONE);
            }
        }

        /*主片只起一个线程做包裹数据接收,根据ai通道数量进行判断,暂时修改(后期再进行处理)*/
        for (i = 0; i < pstIsmSvaCapb->ai_chn; i++)
        {
            /*接收xpack传过来的帧信息*/
			s32Ret = xpack_package_launch_aidgr(i, &stRecXpackSysFrame);
			if (SAL_FAIL == s32Ret)
			{
				SVA_LOGD("Receive no XpackSysFrame !!!! \n");
				SAL_msleep(80);
				continue;
		    }
			SVA_LOGI("Receive the XpackSysFrame !!!! \n");

			/* 确认主从传输的数据状态(此处将mmap和dma传输绑定，即将帧参数和帧数据绑定一起发送，所以将只检查/设置mmap的状态，) */
			stTransCheckDataStaus.uiArgCnt = 3;
			stTransCheckDataStaus.u64Arg[0] = 0;         /* 帧参数(主->从) */
			stTransCheckDataStaus.u64Arg[1] = SAL_FALSE; /* 0为不忙 */
			stTransCheckDataStaus.u64Arg[2] = -1;        /*-1表示会一直等待*/
			s32Ret = Trans_HalCmdProc(TRANS_HAL_CHECK_DATA_STATUS, &stTransCheckDataStaus);
			if (SAL_TRUE != s32Ret)
			{
				SVA_LOGE("Check data status error! ret: %d \n", s32Ret);
				continue;
			}

			/*获取帧参数*/
            s32Ret = Sva_DrvGetTransFramePrm(i, &stRecXpackSysFrame, &stFramePrm);
			if (SAL_SOK != s32Ret)
			{
				SVA_LOGE("Get Frame Frm error! ret: %d \n", s32Ret);
				continue;
			}
            
		 	/*获取帧数据并拷贝帧数据(目前采用软考贝)*/
            if (SAL_SOK != sys_hal_getVideoFrameInfo(&stRecXpackSysFrame, &stVideoFrmBuf))
            {
                SVA_LOGE("get video frame failed! \n");
                continue;
            }
			
			/* 调试接口，dump获取xpack传进来的图像 */
            Sva_HalDumpInputImg((CHAR *)stVideoFrmBuf.virAddr[0],
                                stVideoFrmBuf.stride[0],
                                stVideoFrmBuf.frameParam.height,
                                INPUT_IMG_DUMP_PATH, i,
                                stVideoFrmBuf.pts,
                                SVA_DUMP_YUV_XPACK);
			
		    s32Ret = Sva_DrvCopyFrmData(&stVideoFrmBuf, stFramePrm.uiXpackBufMaxW, stFramePrm.uiXpackBufMaxH, pFrmVirAddr);
			if (SAL_SOK != s32Ret)
			{
				SVA_LOGE("Copy Frame Data Failed! \n");
				continue;
			}

            /* 调试接口，dump获取xpack传进来后copy出来的图像 */
            Sva_HalDumpInputImg((CHAR *)pFrmVirAddr,
                                stFramePrm.uiXpackBufMaxW,
                                stFramePrm.uiXpackBufMaxH,
                                INPUT_IMG_DUMP_PATH, stFramePrm.uichan,
                                stFramePrm.u64pts,
                                SVA_DUMP_YUV_COPY_XPACK);

			/*主片发送帧信息(帧参数和帧数据)*/
            s32Ret = Sva_DrvMasterSendFrmInfo(&stFramePrm, u64FrmPrmSize, u64FrmPhysAddr);
			if (SAL_SOK != s32Ret)
			{
				SVA_LOGE("The Master Send Frame Info Failed!!!!!!\n");
			    continue;
			}
			
			/* 设置主从传输的数据状态 */
			stTransSetDataStaus.uiArgCnt = 2;
			stTransSetDataStaus.u64Arg[0] = 0;
			stTransSetDataStaus.u64Arg[1] = SAL_TRUE;
			
			s32Ret = Trans_HalCmdProc(TRANS_HAL_SET_DATA_STATUS, &stTransSetDataStaus);
			if (SAL_SOK != s32Ret)
			{
				SVA_LOGE("Set data status error! ret: %d \n", s32Ret);
				continue;
			}
        }
    }

err:
	if(0x00 != (&stRecXpackSysFrame)->uiAppData)
	{
		(void)sys_hal_rleaseVideoFrameInfoSt(&stRecXpackSysFrame);
	}
		
    if (pFrmVirAddr)
    {
        mem_hal_mmzFree(pFrmVirAddr, "SVA", "debug_cma");
        pFrmVirAddr = NULL;
    }

    return NULL;
}

/*******************************************************************************
* 函数名  : Sva_DrvVcaeResultThread_ISM
* 描  述  : 接收检测结果线程(双芯片主片使用)
* 输  入  : - prm: 全局变量参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
void *Sva_DrvVcaeResultThread_ISM(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT64 u64AlgRsltSize = sizeof(SVA_ALG_RSLT_S);
    UINT32 uiType = 0;
    UINT64 u64SyncRsltPhysAddr = 0;
    TRANS_HAL_DATA_INFO stTransDataInfo = {0};
    TRANS_HAL_DATA_INFO stTransCheckDataStaus = {0};
    TRANS_HAL_DATA_INFO stTransSetDataStaus = {0};

    SVA_ALG_RSLT_S *pstSyncRslt = NULL;

    s32Ret = mem_hal_mmzAlloc(sizeof(SVA_ALG_RSLT_S), "SVA", "sva_slv_rslt", NULL, SAL_FALSE, &u64SyncRsltPhysAddr, (VOID **)&pstSyncRslt);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("!!!!!!!!!!! mmz alloc failed! \n");
        goto err;
    }

    sal_memset_s(pstSyncRslt, sizeof(SVA_ALG_RSLT_S), 0x00, sizeof(SVA_ALG_RSLT_S));

    SAL_SET_THR_NAME();

    while (1)
    {
        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time0);

        /*主片通过mmp接收智能结果*/

        uiType = 0;
        uiType = uiType | 0x0 | 0x1 << 4;   /* mmap */

        /* 确认主从传输的数据状态 */
        stTransCheckDataStaus.uiArgCnt = 3;
        stTransCheckDataStaus.u64Arg[0] = 1; /* 1表示选用对应的bar，利用mmp传输智能结果 */
        stTransCheckDataStaus.u64Arg[1] = SAL_TRUE; /* 1为忙 */
        stTransCheckDataStaus.u64Arg[2] = -1; /* 一直等待状态 */
        s32Ret = Trans_HalCmdProc(TRANS_HAL_CHECK_DATA_STATUS, &stTransCheckDataStaus);
        if (SAL_TRUE != s32Ret)
        {
            SVA_LOGE("Check data status error! ret: %d \n", s32Ret);
            SAL_msleep(80);
            continue;
        }

        /* 获取智能结果 */
        stTransDataInfo.uiArgCnt = 4;
        stTransDataInfo.u64Arg[0] = u64AlgRsltSize;
        stTransDataInfo.u64Arg[1] = (UINT64)pstSyncRslt;
        stTransDataInfo.u64Arg[2] = uiType;
        s32Ret = Trans_HalCmdProc(TRANS_HAL_RECV_DATA, &stTransDataInfo);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("ioctl error! ret: %d \n", s32Ret);
            SAL_msleep(80);
            continue;
        }

        /* 设置主从传输mmp的数据状态 */
        stTransSetDataStaus.uiArgCnt = 2;
        stTransSetDataStaus.u64Arg[0] = 1; /* 1表示选用对应的bar，利用mmp传输智能结果 */
        stTransSetDataStaus.u64Arg[1] = SAL_FALSE; /* 接收完毕，设置为空闲 */
        s32Ret = Trans_HalCmdProc(TRANS_HAL_SET_DATA_STATUS, &stTransSetDataStaus);

        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("ioctl error! ret: %d \n", s32Ret);
        }

        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time1);

        if (Sva_HalGetDbgLevel() >= DEBUG_LEVEL4)
        {
            SVA_LOGW("master recv rslt: %d ms. \n", time1 - time0);
        }

        for (i = 0; i < pstSyncRslt->uiChnCnt; i++)
        {
            if (pstSyncRslt->stAlgChnRslt[i].chan > 1)
            {
                SVA_LOGE("read Invalid chan %d \n", pstSyncRslt->stAlgChnRslt[i].chan);
                continue;
            }

            SVA_LOGI("Result (Master Receive): chan_%d target_num_%d frame_stamp_%llu time1-time0_%d \n",
                     pstSyncRslt->stAlgChnRslt[i].chan, pstSyncRslt->stAlgChnRslt[i].stProcOut.target_num, pstSyncRslt->stAlgChnRslt[i].stProcOut.frame_stamp, time1 - time0);

            /* 保存从片发送回来的结果，用于外部模块使用 */
            (VOID)Sva_HalUpDateOut(pstSyncRslt->stAlgChnRslt[i].chan, &pstSyncRslt->stAlgChnRslt[i].stProcOut);

            /* 保存到缓存池，显示线程使用 */
            (VOID)Sva_HalSaveToPool(pstSyncRslt->stAlgChnRslt[i].chan, &pstSyncRslt->stAlgChnRslt[i].stProcOut);

            /*安检机获取数据之后,对相关的智能框做一下chg送显示,不需要送编图线程*/
            (void)Sva_DrvISMDealResQueueOut(pstSyncRslt->stAlgChnRslt[i].chan, &pstSyncRslt->stAlgChnRslt[i].stProcOut);
        }
    }

err:

    if (NULL != pstSyncRslt)
    {
        mem_hal_mmzFree(pstSyncRslt, "SVA", "sva_slv_rslt");
        pstSyncRslt = NULL;
    }

    return NULL;
}

/**
 * @function:   Sva_DrvSlaveRecFrmInfo
 * @brief:      从片接收帧信息
 * @param[in]:  UINT64 u64FrmPrmSize
 * @param[in]:  UINT64 u64TransFrmDataSize
 * @param[out]:  SVA_TRANS_FRAME_PRM *pstFramePrm
 * @param[out]:  UINT64 u64FrmPhysAddr
 * @return:     INT32
 */
INT32 Sva_DrvSlaveRecFrmInfo(SVA_TRANS_FRAME_PRM *pstFramePrm, UINT64 u64FrmPrmSize, UINT64 u64FrmPhysAddr, UINT64 u64TransFrmDataSize)
{
	INT32 s32Ret = SAL_SOK;
    UINT32 uiType = 0;
	
	TRANS_HAL_DATA_INFO stTransDataInfo = {0};
    TRANS_HAL_DATA_INFO stTransGetDmaPhyaddr = {0};

	/*主从片mmp传输帧参数(从片接收)*/
	uiType = 0;
	uiType = uiType | 0x0 | 0x0 << 4;	 /* mmap */

    /* 获取帧参数 */
    stTransDataInfo.uiArgCnt = 4;
    stTransDataInfo.u64Arg[0] = u64FrmPrmSize;
    stTransDataInfo.u64Arg[1] = (UINT64)pstFramePrm;
    stTransDataInfo.u64Arg[2] = uiType;
    /* stTransDataInfo.u64Arg[3] = stVideoFrameBuf.phyAddr[0];//mmp不需要目的地址 */
    s32Ret = Trans_HalCmdProc(TRANS_HAL_RECV_DATA, &stTransDataInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Receive frame prm error! ret: %d \n", s32Ret);
        return SAL_FAIL;
    }

	/* 帧数据利用dma进行传输(从片发起接收) */
	uiType = 0;
	uiType = uiType | 0x1 | 0x0 << 4; /*dma*/

	/* 使用DMA主从传输时，获取主片的物理地址(主片),放在stTransGetDmaPhyaddr.u64Arg[1]中 */
	stTransGetDmaPhyaddr.uiArgCnt = 2;
	stTransGetDmaPhyaddr.u64Arg[0] = 2;
	/* stTransGetDmaPhyaddr.u64Arg[1] = u64PhyAddr; */
	s32Ret = Trans_HalCmdProc(TRANS_HAL_GET_DMA_PHYADDR, &stTransGetDmaPhyaddr);
	if (SAL_SOK != s32Ret)
	{
		SVA_LOGE("Get dma phyaddr error! ret: %d \n", s32Ret);
		return SAL_FAIL;
	}

    /*从片发起帧数据接收*/
	stTransDataInfo.uiArgCnt = 4;
	stTransDataInfo.u64Arg[0] = u64TransFrmDataSize;
	stTransDataInfo.u64Arg[1] = stTransGetDmaPhyaddr.u64Arg[1]; /* 主片的物理地址，原地址 */
	stTransDataInfo.u64Arg[2] = uiType;  /* phy */
	stTransDataInfo.u64Arg[3] = u64FrmPhysAddr; /* 从片的物理地址，目的地址stVideoFrameBuf.phyAddr[0]slave_u64Pa */
	s32Ret = Trans_HalCmdProc(TRANS_HAL_RECV_DATA, &stTransDataInfo);
	if (SAL_SOK != s32Ret)
	{
		SVA_LOGE("Receive frame data failed! size: %llu \n", u64TransFrmDataSize);
		return SAL_FAIL;
	}
	
	return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSlaveFrameToQueThread_ISM
* 描  述  : 主从通信数据入队列(安检机双芯片从片使用)
* 输  入  : - prm: 全局变量参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
void *Sva_DrvSlaveFrameToQueThread_ISM(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;

	UINT64 u64FrmPrmSize = sizeof(SVA_TRANS_FRAME_PRM);   /* 传输帧参数尺寸 */
	UINT64 u64TransFrmDataSize = SVA_BUF_WIDTH_MAX * SVA_BUF_HEIGHT_MAX * 3 / 2; /*申请大缓存区，兼容140100*/	
    UINT64 u64FrmPhysAddr = 0;
	//UINT64 u64MbBlk = 0;
    void *pFrmVirAddr = NULL;

    /*主从片传输使用的参数*/
    TRANS_HAL_DATA_INFO stTransCheckDataStaus = {0};
    TRANS_HAL_DATA_INFO stTransSetDataStaus = {0};
	SVA_TRANS_FRAME_PRM stFramePrm = {0};
	SYSTEM_FRAME_INFO stScaleFrame = {0};
    ALLOC_VB_INFO_S stVbInfo = {0};
	SAL_VideoFrameBuf stVideoFrmBuf = {0};
	
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_SLAVE_PRM *pstSlavePrm[2] = {NULL, NULL};
    DSA_QueHndl *pstFullQue[2] = {NULL, NULL};
    DSA_QueHndl *pstEmptQue[2] = {NULL, NULL};
	SYSTEM_FRAME_INFO *pstSysFrameInfo = NULL; /*从片接收数据的缓存队列*/

    SVA_DRV_CHECK_PRM(prm, NULL);

    pstSva_dev = (SVA_DEV_PRM *)prm;

    /* 申请接收帧数据的连续物理内存 */
	s32Ret = mem_hal_cmaMmzAlloc(u64TransFrmDataSize, "SVA", "debug_cma", NULL, SAL_FALSE, &stVbInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("alloc cma error! ret: %d \n", s32Ret);
        goto err;
    }
    /*获取申请的连续物理内存的物理地址，虚拟地址以及u64VbBlk*/
    u64FrmPhysAddr = stVbInfo.u64PhysAddr;
    pFrmVirAddr = stVbInfo.pVirAddr;
	//u64MbBlk = stVbInfo.u64VbBlk;
    if (NULL == pFrmVirAddr || 0 == u64FrmPhysAddr )
    {
        SVA_LOGE("The pFrmVirAddr or u64FrmPhysAddr is NULL\n");
        goto err;
    }

	/*缩放帧数据缓存申请*/
    s32Ret = sys_hal_allocVideoFrameInfoSt(&stScaleFrame);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("alloc video frame info st failed! \n");
        goto err;
    }

    /*初始化从片帧信息缓存队列*/
    s32Ret = Sva_DrvInitSlaveFrameQue();
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Init Slave Frame Queue Failed! \n");
        goto err;
    }

    for (i = 0; i < SVA_DEV_MAX; i++)
    {
        pstSlavePrm[i] = Sva_DrvGetSlavePrm(i);
        pstFullQue[i] = &pstSlavePrm[i]->stFrameDataFullQue;
        pstEmptQue[i] = &pstSlavePrm[i]->stFrameDataEmptQue;
    }

    SAL_SET_THR_NAME();
    SAL_SetThreadCoreBind(((pstSva_dev->chan + 1) % 2));

    while (1)
    {
        /* todo: 增加是否有待接收数据的判断 */
        {
            (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time0);

            /* 确认主从传输的数据状态(此处将mmap和dma传输绑定，即将帧参数和帧数据绑定一起发送，所以将只检查/设置mmap的状态，) */
            stTransCheckDataStaus.uiArgCnt = 3;
            stTransCheckDataStaus.u64Arg[0] = 0;        /* mmap映射内存使用方式类别,0表示主片向从片发送帧参数 */
            stTransCheckDataStaus.u64Arg[1] = SAL_TRUE; /* 1为忙 */
            stTransCheckDataStaus.u64Arg[2] = -1;       /*-1表示一直等待*/
            s32Ret = Trans_HalCmdProc(TRANS_HAL_CHECK_DATA_STATUS, &stTransCheckDataStaus);
            if (SAL_TRUE != s32Ret)
            {
                SVA_LOGE("Check data status error! ret: %d \n", s32Ret);
                continue;
            }
			
			/*从片接收帧参数和帧数据*/
			s32Ret = Sva_DrvSlaveRecFrmInfo(&stFramePrm, u64FrmPrmSize, u64FrmPhysAddr, u64TransFrmDataSize);
			if (SAL_SOK != s32Ret)
			{
				SVA_LOGE("The Slave Receive Frame Info Failed!!!!!!\n");
			    continue;
			}
			
			/*检测帧参数是否错误*/
			SVA_LOGI("Trans Frame Prm (slave receive): uichan_%d uiPkgWid_%d uiPKgHei_%d uiXpackBufMaxW_%d uiXpackBufMaxH_%d uiFrameNum_%d u64pts_%llu\n",stFramePrm.uichan,\
					stFramePrm.uiPkgWid, stFramePrm.uiPKgHei, stFramePrm.uiXpackBufMaxW, stFramePrm.uiXpackBufMaxH, stFramePrm.uiFrameNum, stFramePrm.u64pts);
			
			/* 调试接口，dump获取从片接收的YUV图像 */
			Sva_HalDumpInputImg((CHAR *)pFrmVirAddr,
								stFramePrm.uiXpackBufMaxW,
								stFramePrm.uiXpackBufMaxH,
								INPUT_IMG_DUMP_PATH, stFramePrm.uichan,
								stFramePrm.u64pts,
								SVA_DUMP_YUV_SLAVE_REC);

            (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time1);
			
            /*将帧信息(帧参数和帧数据)放入从片帧信息缓存队列*/
			pstSysFrameInfo = NULL;
		    DSA_QueGet(pstEmptQue[stFramePrm.uichan], (void **)&pstSysFrameInfo, SAL_TIMEOUT_NONE); /*获取队列buff时，会获得图片的宽，高，跨距，图片类型等信息，队列初始化时赋的值*/
		    if (NULL == pstSysFrameInfo)
		    {
		        SVA_LOGE("Get Empty que Failed! Que Data ERR! \n");
		        usleep(10 * 1000);
		        continue;
		    }
			
		    /*帧数据后处理，是否缩放*/
			s32Ret = Sva_DrvFrmDataPrcess(stFramePrm.uiXpackBufMaxW, stFramePrm.uiXpackBufMaxH, pFrmVirAddr, &stScaleFrame, &pstSysFrameInfo[0]);
			if (SAL_SOK != s32Ret)
			{
				SVA_LOGE("Frame Data Prcess Failed! \n");
				goto reuse;
			}

		    /*帧参数赋值*/
			if (SAL_SOK != sys_hal_getVideoFrameInfo(&pstSysFrameInfo[0], &stVideoFrmBuf))
		    {
		        SVA_LOGE("get video frame failed! \n");
		        goto reuse;
		    }
			
		    stVideoFrmBuf.frameParam.width = stFramePrm.uiPkgWid * SVA_MODULE_WIDTH / stFramePrm.uiXpackBufMaxW; /* 修改为缩放后的包裹有效宽度 */
		    stVideoFrmBuf.frameParam.height = stFramePrm.uiPKgHei * SVA_MODULE_HEIGHT / stFramePrm.uiXpackBufMaxH; /* 修改为缩放后的包裹有效高度 */
		    stVideoFrmBuf.frameNum = stFramePrm.uiFrameNum;
			stVideoFrmBuf.pts = stFramePrm.u64pts;
			
			/*调试信息，打印送入从片缓存队列的帧参数*/
			SVA_LOGI("Frame Prm to SlaveQue: chan_%d width_%d height_%d frameNum_%d pts_%llu \n",\
				stFramePrm.uichan, stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height, stVideoFrmBuf.frameNum, stVideoFrmBuf.pts);
			
		    if (SAL_SOK != sys_hal_buildVideoFrame(&stVideoFrmBuf, &pstSysFrameInfo[0]))
		    {
		        SVA_LOGE("get video frame failed! \n");
		        goto reuse;
		    }
reuse:
		    DSA_QuePut(pstFullQue[stFramePrm.uichan], pstSysFrameInfo, SAL_TIMEOUT_NONE);
	
			(VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time2);
			
			/* 设置主从传输的数据状态 */
			stTransSetDataStaus.uiArgCnt = 2;
			stTransSetDataStaus.u64Arg[0] = 0;
			stTransSetDataStaus.u64Arg[1] = SAL_FALSE; /* 接收完毕，设置为空闲 */
			s32Ret = Trans_HalCmdProc(TRANS_HAL_SET_DATA_STATUS, &stTransSetDataStaus);

			if (SAL_SOK != s32Ret)
			{
				SVA_LOGE("Set data status error! ret: %d \n", s32Ret);
				continue;
			}

            (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time3);
            if (Sva_HalGetDbgLevel() >= DEBUG_LEVEL4)
            {
                SVA_LOGW("slave recv: wait trans statue: %d ms, receive frameprm: %d ms, receive frame data: %d ms, total %d ms \n", time1 - time0, time2 - time1, time3 - time2, time3 - time0);
            }
        }
    }

err:
    (VOID)Sva_DrvDeInitSlaveFrameQue();
    SVA_LOGE("Thread %s exit! \n", __FUNCTION__);

    if (pFrmVirAddr)
    {
        mem_hal_mmzFree(pFrmVirAddr, "SVA", "debug_cma");
        pFrmVirAddr = NULL;
    }
	
	if(0x00 != stScaleFrame.uiAppData)
	{
		(VOID)sys_hal_rleaseVideoFrameInfoSt(&stScaleFrame);

	}
    return NULL;
}

/*******************************************************************************
* 函数名  : Sva_DrvSlaveSyncThread_ISM
* 描  述  : 送帧线程
* 输  入  : - prm: 全局变量参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 安检机使用，将帧信息，智能通道开关，算法参数，引擎模式送入推帧引擎
*******************************************************************************/
void *Sva_DrvSlaveSyncThread_ISM(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
	
    UINT32 i = 0;
    UINT32 chan = 0;
	UINT32 uiCount = 0;
    UINT32 uiInputErrCnt = 0;
    UINT32 uiQueueCnt = 0;
    UINT32 chnStatus[SVA_DEV_MAX] = {0, 0};             /* Drv层通道创建状态 */
    UINT32 xsiStatus[SVA_DEV_MAX] = {0, 0};             /* Hal层通道创建状态 */

    SVA_PROC_MODE_E enProcMode = 0;
    SAL_VideoFrameBuf stVideoFrameBuf = {0};

    SVA_PROCESS_IN *pstTmpIn = NULL;
	SVA_PROCESS_IN *pstIn = NULL;     /* SVA_DEV_MAX 此处是否需要设成一个指针数组?*/
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_DEV_PRM *pstSva_devTmp = NULL;
    SVA_SLAVE_PRM *pstSvaSlave_dev = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SYSTEM_FRAME_INFO *pstSysFrameInfoTmp = NULL;
    SYSTEM_FRAME_INFO *pstSysFrameInfo[SVA_DEV_MAX] = {NULL, NULL};

    SVA_DRV_CHECK_PRM(prm, NULL);
    pstSva_dev = (SVA_DEV_PRM *)prm;

    /*算法引擎参数结构体申请缓存*/
    pstIn = SAL_memZalloc(SVA_DEV_MAX * sizeof(SVA_PROCESS_IN), "sva", "stIn_stk");
    if (NULL == pstIn)
    {
        SVA_LOGE("zalloc failed! \n");
        goto err;
    }

    SAL_SET_THR_NAME();
    SAL_SetThreadCoreBind(((pstSva_dev->chan + 1) % 2));

    while (1)
    {

        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time0);
        SVA_LOGD("Before Get From Full Que! mode %d \n", enProcMode);

        /*获取上层配置的sva算法参数*/
        for (i = 0; i < SVA_DEV_MAX; i++)
        {
            pstSva_dev = Sva_DrvGetDev(i);
            if (NULL == pstSva_dev)
            {
                SVA_LOGE("pstSva_dev == null! chan %d \n", i);
                return NULL;
            }

            pstPrmFullQue = &pstSva_dev->stPrmFullQue;
            pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;
            uiQueueCnt = DSA_QueGetQueuedCount(pstPrmFullQue);
            if (uiQueueCnt)
            {
                /* 获取 buffer */
				DSA_QueGet(pstPrmFullQue, (void **)&pstTmpIn, SAL_TIMEOUT_FOREVER);
	            sal_memcpy_s(&pstIn[i], sizeof(SVA_PROCESS_IN), pstTmpIn, sizeof(SVA_PROCESS_IN));
	            DSA_QuePut(pstPrmEmptQue, (void *)pstTmpIn, SAL_TIMEOUT_NONE);
            }
        }

        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time1);

        for (chan = 0; chan < SVA_DEV_MAX; chan++)
        {
            /*从从片缓存队列中获取帧信息*/
            pstSvaSlave_dev = Sva_DrvGetSlavePrm(chan);
            uiCount = DSA_QueGetQueuedCount(&pstSvaSlave_dev->stFrameDataFullQue);
            if (uiCount)
            {
                DSA_QueGet(&pstSvaSlave_dev->stFrameDataFullQue, (void **)&pstSysFrameInfoTmp, SAL_TIMEOUT_FOREVER);
                pstSysFrameInfo[chan] = &pstSysFrameInfoTmp[0];
                pstSysFrameInfo[1 - chan] = NULL;
            }
            else
            {
                SAL_msleep(20);
                continue;
            }

			SVA_LOGD("after Get From Full Que! \n");
            
            /* 通道状态互斥:为了图片模式推帧时保证只推送一路通道的数据 */
			pstSva_devTmp = Sva_DrvGetDev(chan);
            chnStatus[chan] = pstSva_devTmp->uiChnStatus;
            chnStatus[1 - chan] = SAL_FALSE;

            /* 避免从片获取第一帧数据时, moduleinit还没有初始化完全，导致送入引擎的数据为空 */
            xsiStatus[chan] = SAL_TRUE;
            xsiStatus[1 - chan] = SAL_FALSE;

#if 0       /*将对应图片直接送入引擎，用于测试误检问题*/
            FILE *yuvfile = NULL;
            unsigned int n = 0;

            yuvfile = fopen("./source/error-398-1024.nv21", "rb");
            if (NULL == yuvfile)
            {
                printf("open file yuv failed !!!\n");

            }

            sys_hal_getVideoFrameInfo(&pstTmpppp[0], &stVideoFrameBuf);
            n = fread((CHAR *)stVideoFrameBuf.virAddr[0], 1, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2, yuvfile);
            stVideoFrameBuf.frameParam.height = (n == 0) ? 0 : 1024;
            stVideoFrameBuf.frameParam.width = (n == 0) ? 0 : 1280;
			sys_hal_buildVideoFrame(&stVideoFrameBuf,pstSysFrameInfo[k]);
            fclose(yuvfile);
#endif
#if 0       /*检测送入算法引擎的帧数据是否正确*/
            sys_hal_getVideoFrameInfo(&pstTmpppp[0], &stVideoFrameBuf);
			sys_hal_getVideoFrameInfo(pstSysFrameInfo[k], &stVideoFrameBuf);

            INT8 framename[64] = {0};
            //sprintf((CHAR *)framename, "%d-%d slaveputengine.yuv", g_stIn_slv_recv[0].stAiPrm[k].timeRef, k);
            snprintf((CHAR *)framename, sizeof(framename),"%d-%d slaveputengine.yuv", stVideoFrameBuf.frameNum, k);
            Sva_HalDebugDumpData((CHAR *)stVideoFrameBuf.virAddr[0], (CHAR *)framename, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2, 0);
#endif
			/*打印送引擎前的帧数据参数*/
			(VOID)sys_hal_getVideoFrameInfo(pstSysFrameInfo[chan], &stVideoFrameBuf);
			SVA_LOGI("Frame Prm Before Engine: chan_%d stride[0]_%d width_%d height_%d dataFormat_%d pts_%llu frameNum_%d\n",chan, \
					stVideoFrameBuf.stride[0],stVideoFrameBuf.frameParam.width, stVideoFrameBuf.frameParam.height,\
					stVideoFrameBuf.frameParam.dataFormat,stVideoFrameBuf.pts,stVideoFrameBuf.frameNum);
			
			/*将传输来的图像，算法参数，算法通道开关状态送入推帧接口*/
            s32Ret = Sva_HalVcaeSyncPutData(pstSysFrameInfo, pstIn, &chnStatus[0], &xsiStatus[0], g_sva_common.uiMainViewChn, SVA_PROC_MODE_IMAGE);
            if (s32Ret != SAL_SOK)
            {
                uiInputErrCnt++;
                if (uiInputErrCnt > 20)
                {
                    SVA_LOGW("chan %d continue !\n", pstSva_dev->chan);
                    uiInputErrCnt = 0;
                    usleep(10 * 1000);
                }
            }
            else
            {
                uiInputErrCnt = 0;
            }

            /* 队列数据释放 */
            if (NULL != pstSysFrameInfoTmp)
            {
                s32Ret = Sva_DrvPutFrameDataFromQue(chan, pstSysFrameInfoTmp, SAL_TIMEOUT_NONE);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE("Put Frame Data To Queue Failed! \n");
                }
            }
        }

        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time2);

        if (Sva_HalGetAlgDbgFlag() && time2 - time0 > 17)
        {
            SVA_LOGW("slave sync: %d, %d, total: %d \n", time1 - time0, time2 - time1, time2 - time0);
        }
    }
err:
    s32Ret = SAL_memfree(pstIn, "sva", "stIn_stk");
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("mem free Failed! \n");
    }
    pstIn = NULL;
    return NULL;
}

/*******************************************************************************
* 函数名  : Sva_DrvSendRsltToMainChip_ISM
* 描  述  : 发送结果给主片
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSendRsltToMainChip_ISM(SVA_ALG_RSLT_S *pstAlgRslt)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiType = 0;
    UINT32 uiTime0 = 0;
    UINT32 uiTime1 = 0;

    static VOID *pRsltBuf = NULL;

    UINT64 u64AlgRsltSize = sizeof(SVA_ALG_RSLT_S); /* SAL_align(sizeof(SVA_SYNC_RESULT_S), 1024); */

    TRANS_HAL_DATA_INFO stTransDataInfo = {0};
    TRANS_HAL_DATA_INFO stTransCheckDataStaus = {0};
    TRANS_HAL_DATA_INFO stTransSetDataStaus = {0};

    ALLOC_VB_INFO_S stVbInfo = {0};

    if (NULL == pRsltBuf)
    {
        s32Ret = mem_hal_vbAlloc(sizeof(SVA_ALG_RSLT_S), "SVA", "sva_res", NULL, SAL_FALSE, &stVbInfo);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("malloc failed! \n");
            goto err;
        }
        else
        {
            pRsltBuf = stVbInfo.pVirAddr;
            SVA_LOGW("szl_dbg: mem alloc end! \n");
        }
    }

    SVA_LOGD("send to main:size %llu phy %p. cpyAddr %p\n", u64AlgRsltSize, pRsltBuf, pstAlgRslt);
    (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &uiTime0);

    /*加锁，避免双通道在从片向主片发送智能结果时，同时占用传输通道*/
    SAL_mutexLock(g_sva_common.mRsltMutexHdl);
	
    /*传输算法结果*/

    uiType = 0;
    uiType = uiType | 0x0 | 0x1 << 4;   /* mmap */

    /* 确认主从传输的数据状态 */
    stTransCheckDataStaus.uiArgCnt = 3;
    stTransCheckDataStaus.u64Arg[0] = 1; /* 从片传输智能数据 */
    stTransCheckDataStaus.u64Arg[1] = SAL_FALSE; /* 0为不忙 */
    stTransCheckDataStaus.u64Arg[2] = -1;
    s32Ret = Trans_HalCmdProc(TRANS_HAL_CHECK_DATA_STATUS, &stTransCheckDataStaus);
    if (SAL_TRUE != s32Ret)
    {
        SVA_LOGE("Check data status error! ret: %d \n", s32Ret);
		SAL_mutexUnlock(g_sva_common.mRsltMutexHdl);
        return SAL_FAIL;
    }
	
    /*将智能结果数据放入传输buffer中*/
    sal_memcpy_s(pRsltBuf, u64AlgRsltSize, pstAlgRslt, u64AlgRsltSize);

    /* 传输数据 */
    stTransDataInfo.uiArgCnt = 4;
    stTransDataInfo.u64Arg[0] = u64AlgRsltSize;
    stTransDataInfo.u64Arg[1] = (UINT64)pRsltBuf;
    stTransDataInfo.u64Arg[2] = uiType;
    s32Ret = Trans_HalCmdProc(TRANS_HAL_SEND_DATA, &stTransDataInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("dual-chip send data failed! size: %llu \n", u64AlgRsltSize);
		SAL_mutexUnlock(g_sva_common.mRsltMutexHdl);
        return SAL_FAIL;
    }

    /*显示从主片传输回来的智能结果信息*/
    SVA_LOGI("Result (Slave send):uiChnCnt_%d chan_%d frame_stamp_%llu target_num_%d uiImgW_%d uiImgH_%d u64FrameExtSize_%lld\n",
             pstAlgRslt->uiChnCnt,
             pstAlgRslt->stAlgChnRslt[0].chan,
             pstAlgRslt->stAlgChnRslt[0].stProcOut.frame_stamp,
             pstAlgRslt->stAlgChnRslt[0].stProcOut.target_num,
             pstAlgRslt->stAlgChnRslt[0].stProcOut.uiImgW,
             pstAlgRslt->stAlgChnRslt[0].stProcOut.uiImgH,
             u64AlgRsltSize);

    /* 设置主从传输的数据状态 */
    stTransSetDataStaus.uiArgCnt = 2;
    stTransSetDataStaus.u64Arg[0] = 1;
    stTransSetDataStaus.u64Arg[1] = SAL_TRUE;
    s32Ret = Trans_HalCmdProc(TRANS_HAL_SET_DATA_STATUS, &stTransSetDataStaus);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Set data status error! ret: %d \n", s32Ret);
		SAL_mutexUnlock(g_sva_common.mRsltMutexHdl);
        return SAL_FAIL;
    }

    /*解锁*/
    SAL_mutexUnlock(g_sva_common.mRsltMutexHdl);

    (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &uiTime1);

    if (Sva_HalGetAlgDbgFlag())
    {
        SVA_LOGW("send to main: %d ms. \n", uiTime1 - uiTime0);
    }

    SVA_LOGI("send to main----ok!!!!: size %llu phy %p. cpyAddr %p\n", u64AlgRsltSize, pRsltBuf, pstAlgRslt);
    return SAL_SOK;

err: 
    if(NULL != stVbInfo.pVirAddr)
    {
        s32Ret = mem_hal_vbFree(stVbInfo.pVirAddr, "SVA", "sva_res", sizeof(SVA_ALG_RSLT_S), stVbInfo.u64VbBlk, stVbInfo.u32PoolId);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("mem_hal_vbFree failed %p\n", (Ptr)stVbInfo.pVirAddr);
            return SAL_FAIL;
        }
    }

    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvSendRsltToMainChip
* 描  述  : 发送结果给主片
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSendRsltToMainChip(SVA_ALG_RSLT_S *pstAlgRslt)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 chan = 0;
    UINT32 uiTime0 = 0;
    UINT32 uiTime1 = 0;

    UINT64 u64FrameExtSize = sizeof(SVA_ALG_RSLT_S);

    static INT8 *pRsltBuf = NULL;
    static UINT64 u64PhyAddr = 0;

    if (!pRsltBuf)
    {
        s32Ret = mem_hal_mmzAlloc(u64FrameExtSize, "SVA", "sva_rslt_buf", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&pRsltBuf);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("alloc mmz failed !!!\n");
            goto err;
        }
    }

    s32Ret = Sva_DrvCreateSendRsltBusiDev(chan);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("create send rslt busi dev failed! chan %d \n", 0);
        return SAL_FAIL;
    }

#ifndef PCIE_S2M_DEBUG
    sal_memcpy_s(pRsltBuf, u64FrameExtSize, pstAlgRslt, u64FrameExtSize);
#else
    sal_memset_s(pRsltBuf, u64FrameExtSize, 0x00, u64FrameExtSize);
#endif

    (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &uiTime0);

    g_sva_common.stDbgPrm.stCommSts.uiSendIaRsltFlag = 1;    /* 送帧之前标记 */

    g_sva_common.stDbgPrm.stCommSts.uiSendIaRsltNum++;

#ifdef XTRANS_DUAL_DMA

    s32Ret = Sva_DrvSendRsltByDualDma(u64PhyAddr, u64FrameExtSize);

#else

    s32Ret = Sva_DrvSendRsltByMMap(chan, pRsltBuf, u64FrameExtSize);

#endif

    g_sva_common.stDbgPrm.stCommSts.uiSendIaRsltFlag = 0;    /* 送帧之后标记 */
    if (SAL_SOK != s32Ret)
    {
        g_sva_common.stDbgPrm.stCommSts.uiSendIaRsltFailNum++;
        SVA_LOGE("dual-chip set data status failed! \n");
        goto exit;
    }
    else
    {
        g_sva_common.stDbgPrm.stCommSts.uiSendIaRsltSuccNum++;
    }

    (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &uiTime1);
    (VOID)Sva_DrvTransDbgAddTime(SVA_TRANS_DBG_SLAVE_RESULT_TYPE, SVA_TRANS_CONV_IDX(SLV_RSLT_TOTAL_TYPE), uiTime0, uiTime1);

    /* 调试接口: 从片发送智能结果的统计耗时 */
    (VOID)Sva_DrvSlaveRsltCalDbgTime();

    (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &uiTime1);

    if (Sva_HalGetAlgDbgFlag())
    {
        SVA_LOGW("send to main: %d ms. \n", uiTime1 - uiTime0);
    }

    return SAL_SOK;

err:
    if (NULL != pRsltBuf)
    {
        (VOID)mem_hal_mmzFree(pRsltBuf, "SVA", "sva_rslt_buf");
        pRsltBuf = NULL;
    }

exit:
    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvSlaveSendRsltThread
* 描  述  : 送帧线程
* 输  入  : - prm: 全局变量参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 分析仪使用，双路检测通道均使用此线程，不区分模式(关联or非关联)
*******************************************************************************/
void *Sva_DrvSlaveSendRsltThread(void *prm)
{
/*    INT32 s32Ret = SAL_SOK; */

    /*   UINT32 chan = 0; */
    UINT32 time0 = 0;
    UINT32 time1 = 0;

    SAL_SET_THR_NAME();

    while (1)
    {
        if ((0 == g_sva_common.uiChnCnt)
            || (SVA_PROC_MODE_IMAGE == g_proc_mode)
            || (SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode && g_sva_common.uiChnCnt < 2)
            || (SAL_TRUE != Sva_DrvGetInitFlag()))
        {
            usleep(100 * 1000);
            continue;             /*未初始化等情况进行待，同步R7*/
        }

        time0 = SAL_getCurMs();

#if 0
        s32Ret = Sva_DrvSendRsltToMainChip(1);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Send Rslt To Main Chip Failed! \n");
            usleep(10 * 1000);
        }

        s32Ret = Sva_DrvSendRsltToMainChip(0);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Send Rslt To Main Chip Failed! \n");
            usleep(10 * 1000);
        }

#endif
        time1 = SAL_getCurMs();

        usleep((13 > (time1 - time0) ? (13 + time0 - time1) : 1) * 1000);

        SVA_LOGD("Send Rslt to Main Chip end! \n");
    }

/* exit: */
    return NULL;
}

/**
 * @function:   Sva_DrvUnlockMatchFrm
 * @brief:      解锁(匹配帧数据使用)
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvUnlockMatchFrm(UINT32 chan)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    SVA_DRV_CHECK_PRM(pstSva_dev->pstMatchBufInfo, SAL_FAIL);

    SAL_mutexUnlock(pstSva_dev->pstMatchBufInfo->pMutex);

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvLockMatchFrm
 * @brief:      加锁(匹配帧数据使用)
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvLockMatchFrm(UINT32 chan)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    SVA_DRV_CHECK_PRM(pstSva_dev->pstMatchBufInfo, SAL_FAIL);

    SAL_mutexLock(pstSva_dev->pstMatchBufInfo->pMutex);

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvPrintMatchFrmInfo
 * @brief:      打印匹配帧信息（调试使用）
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static VOID
 */
static INT32 Sva_DrvPrintMatchFrmInfo(UINT32 chan)
{
    UINT32 i = 0;
    UINT32 uiIdx = 0;
    UINT32 uiCnt = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_MATCH_BUF_INFO *pstMatchBufInfo = NULL;
    SVA_MATCH_BUF_DATA *pstMatchBufData = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstMatchBufInfo = pstSva_dev->pstMatchBufInfo;
    SVA_DRV_CHECK_PRM(pstMatchBufInfo, SAL_FAIL);

    uiIdx = pstMatchBufInfo->uiW_Idx;
    uiCnt = pstMatchBufInfo->uiCnt;

    SVA_LOGD("cur idx %d, uiCnt %d, chan %d \n", uiIdx, uiCnt, chan);

    for (i = 0; i < uiCnt; i++)
    {
        pstMatchBufData = &pstMatchBufInfo->stMatchBufData[(uiIdx + i) % uiCnt];
        SVA_LOGD("i %d, frmNum %d, chan %d \n", i, pstMatchBufData->uiFrameNum, chan);
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvProcTarOffset
 * @brief:      包裹和目标坐标偏移
 * @param[in]:  SVA_SYNC_RESULT_S *pstSyncRslt
 * @param[in]:  float fOffset
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvProcTarOffset(SVA_SYNC_RESULT_S *pstSyncRslt, float fOffset)
{
    UINT32 i = 0;

    SVA_TARGET *pstTarInfo = NULL;
    SVA_PACK_ALERT *pstPkgInfo = NULL;

    /* 包裹坐标偏移 */
    pstPkgInfo = &pstSyncRslt->stProcOut.packbagAlert;
    pstPkgInfo->package_loc.x = fOffset > 0.0 \
                                ? (pstPkgInfo->package_loc.x > fOffset ? pstPkgInfo->package_loc.x - fOffset : 0.0) \
                                : (pstPkgInfo->package_loc.x - fOffset > 1.0 ? 1.0 : pstPkgInfo->package_loc.x - fOffset);

    for (i = 0; i < pstSyncRslt->stProcOut.packbagAlert.MainViewPackageNum; i++)
    {
        pstSyncRslt->stProcOut.packbagAlert.MainViewPackageLoc[i].PackageRect.x = fOffset > 0.0 \
                                                                                  ? (pstSyncRslt->stProcOut.packbagAlert.MainViewPackageLoc[i].PackageRect.x > fOffset ? pstSyncRslt->stProcOut.packbagAlert.MainViewPackageLoc[i].PackageRect.x - fOffset : 0.0) \
                                                                                  : (pstSyncRslt->stProcOut.packbagAlert.MainViewPackageLoc[i].PackageRect.x - fOffset > 1.0 ? 1.0 : pstSyncRslt->stProcOut.packbagAlert.MainViewPackageLoc[i].PackageRect.x - fOffset);
    }

    /* 目标坐标偏移 */
    for (i = 0; i < pstSyncRslt->stProcOut.target_num; i++)
    {
        pstTarInfo = &pstSyncRslt->stProcOut.target[i];

        pstTarInfo->rect.x = fOffset > 0.0 \
                             ? (pstTarInfo->rect.x > fOffset ? pstTarInfo->rect.x - fOffset : 0.0) \
                             : (pstTarInfo->rect.x - fOffset > 1.0 ? 1.0 : pstTarInfo->rect.x - fOffset);
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvCalOffset
 * @brief:      计算偏移量
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static float
 */
static float Sva_DrvCalOffset(UINT32 chan)
{
    UINT32 i = 0;
    float fOffset = 0.0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_MATCH_BUF_INFO *pstMatchBufInfo = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstMatchBufInfo = pstSva_dev->pstMatchBufInfo;
    SVA_DRV_CHECK_PRM(pstMatchBufInfo, SAL_FAIL);

    for (i = 0; i < SVA_MATCH_OFFSET_TAB_CNT; i++)
    {
        fOffset += pstMatchBufInfo->fPastOffset[i];
    }

    fOffset /= (float)SVA_MATCH_OFFSET_TAB_CNT;

    return fOffset;
}

/**
 * @function:   Sva_DrvFindMatchBuf
 * @brief:      寻找匹配帧数据
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 uiFrmNum
 * @param[in]:  SYSTEM_FRAME_INFO **ppstSysFrmInfo
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvFindMatchBuf(SVA_SYNC_RESULT_S *pstSyncRslt, SYSTEM_FRAME_INFO **ppstSysFrmInfo)
{
    UINT32 i = 0;
    UINT32 chan = 0;
    UINT32 uiFrmNum = 0;
    UINT32 uiIdx = 0;
    UINT32 uiCnt = 0;
    UINT32 uiTmpIdx = 0;
    UINT32 uiMatchIdx = 0;
    UINT32 uiMinGap = 0xffffffff;
    float fOffset = 0.0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_MATCH_BUF_INFO *pstMatchBufInfo = NULL;
    SVA_MATCH_BUF_DATA *pstMatchBufData = NULL;

    chan = pstSyncRslt->chan;
    uiFrmNum = pstSyncRslt->stProcOut.frame_num;

    SVA_DRV_CHECK_PRM(pstSyncRslt, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstMatchBufInfo = pstSva_dev->pstMatchBufInfo;
    SVA_DRV_CHECK_PRM(pstMatchBufInfo, SAL_FAIL);

    uiIdx = pstMatchBufInfo->uiW_Idx;
    uiCnt = pstMatchBufInfo->uiCnt;

    for (i = 0; i < uiCnt; i++)
    {
        uiTmpIdx = (uiIdx + uiCnt - i - 1) % uiCnt;

        pstMatchBufData = &pstMatchBufInfo->stMatchBufData[uiTmpIdx];
        if (pstMatchBufData->uiFrameNum == uiFrmNum)
        {
            uiMatchIdx = uiTmpIdx;
            SVA_LOGD("find frm! chan %d, FrmNum %d \n", chan, uiFrmNum);
            break;
        }

        if (SVA_CAL_DIFF(pstMatchBufData->uiFrameNum, uiFrmNum) < uiMinGap)
        {
            uiMinGap = SVA_CAL_DIFF(pstMatchBufData->uiFrameNum, uiFrmNum);
            uiMatchIdx = uiTmpIdx;
        }
    }

    if (i >= uiCnt)
    {
        SVA_LOGE("szl_dbg: can not find frm[%d]! uiMinGap %d, uiMatchIdx %d, chan %d \n", uiFrmNum, uiMinGap, uiMatchIdx, chan);

        /* 打印当前匹配帧缓存的相关信息，用于问题定位 */
        (VOID)Sva_DrvPrintMatchFrmInfo(chan);

        fOffset = Sva_DrvCalOffset(chan);

        SVA_LOGI("get cal offset %f, chan %d \n", fOffset, chan);

        fOffset *= (float)uiMinGap;

        SVA_LOGI("gap %d, final offset %f, chan %d \n", uiMinGap, fOffset, chan);

        (VOID)Sva_DrvProcTarOffset(pstSyncRslt, fOffset);
    }

    /* 保留过去3帧偏移值 */
    pstMatchBufInfo->fPastOffset[pstMatchBufInfo->uiOffsetW_Idx] = pstSyncRslt->stProcOut.frame_offset;
    pstMatchBufInfo->uiOffsetW_Idx = (pstMatchBufInfo->uiOffsetW_Idx + 1) % SVA_MATCH_OFFSET_TAB_CNT;

    *ppstSysFrmInfo = &pstMatchBufInfo->stMatchBufData[uiMatchIdx].stSysFrameInfo;
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvFindMatchBuf
 * @brief:      寻找匹配帧数据
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 uiFrmNum
 * @param[in]:  SYSTEM_FRAME_INFO **ppstSysFrmInfo
 * @param[out]: None
 * @return:     static INT32
 */
static ATTRIBUTE_UNUSED INT32 Sva_DrvFindMatchBuf_V1(UINT32 chan, UINT32 uiFrmNum, SYSTEM_FRAME_INFO **ppstSysFrmInfo)
{
    UINT32 i = 0;
    UINT32 uiIdx = 0;
    UINT32 uiCnt = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_MATCH_BUF_INFO *pstMatchBufInfo = NULL;
    SVA_MATCH_BUF_DATA *pstMatchBufData = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstMatchBufInfo = pstSva_dev->pstMatchBufInfo;
    SVA_DRV_CHECK_PRM(pstMatchBufInfo, SAL_FAIL);

    uiIdx = pstMatchBufInfo->uiW_Idx;
    uiCnt = pstMatchBufInfo->uiCnt;

    for (i = 0; i < uiCnt; i++)
    {
        pstMatchBufData = &pstMatchBufInfo->stMatchBufData[(uiIdx + uiCnt - i - 1) % uiCnt];
        if (pstMatchBufData->uiFrameNum == uiFrmNum)
        {
            SVA_LOGD("find frm! chan %d, FrmNum %d \n", chan, uiFrmNum);
            break;
        }
    }

    if (i >= uiCnt)
    {
        SVA_LOGE("szl_dbg: can not find frm! chan %d \n", chan);
        goto err;
    }

    *ppstSysFrmInfo = &pstMatchBufData->stSysFrameInfo;
    return SAL_SOK;

err:
    (VOID)Sva_DrvPrintMatchFrmInfo(chan);  /* 打印当前匹配帧缓存的相关信息，用于问题定位 */

    *ppstSysFrmInfo = NULL;
    return SAL_FAIL;
}

/**
 * @function:   Sva_DrvSendEncYuv
 * @brief:      yuv送入进行编图处理
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrame
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvSendEncYuv(UINT32 chan, SVA_PROCESS_OUT *pstOut, SYSTEM_FRAME_INFO *pstSysFrame)
{
    UINT32 uiQueueCnt = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;

    DSA_QueHndl *pstJpegFullQue = NULL;
    DSA_QueHndl *pstJpegEmptQue = NULL;
    SVA_JPEG_INFO_ST *pstJpegFrame = NULL;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstOut, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSysFrame, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstJpegFullQue = &pstSva_dev->stJpegVenc.pstFullQue;
    pstJpegEmptQue = &pstSva_dev->stJpegVenc.pstEmptQue;

    /* 需要抓图时送入编码队列 */
    uiQueueCnt = DSA_QueGetQueuedCount(pstJpegEmptQue);
    if (uiQueueCnt)
    {
        /* 获取 buffer */
        DSA_QueGet(pstJpegEmptQue, (void **)&pstJpegFrame, SAL_TIMEOUT_FOREVER);

        (VOID)Ia_TdeQuickCopy(pstSysFrame,
                              &pstJpegFrame->stFrame,
                              0, 0,
                              pstSysFrame->uiDataWidth, pstSysFrame->uiDataHeight, SAL_FALSE);

        sal_memset_s(&pstJpegFrame->szJpegOut, sizeof(SVA_PROCESS_OUT), 0x00, sizeof(SVA_PROCESS_OUT));
        sal_memcpy_s(&pstJpegFrame->szJpegOut, sizeof(SVA_PROCESS_OUT), pstOut, sizeof(SVA_PROCESS_OUT));

        if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSysFrame, &stVideoFrmBuf))
        {
            SVA_LOGE("get video frame failed! \n");
            return SAL_FAIL;
        }

        /* 保存用于同步的原始数据时间戳、图片宽高等信息 */
        pstJpegFrame->szJpegOut.frame_stamp = (UINT64)stVideoFrmBuf.privateDate;
        pstJpegFrame->szJpegOut.uiImgW = pstSysFrame->uiDataWidth;
        pstJpegFrame->szJpegOut.uiImgH = pstSysFrame->uiDataHeight;
        pstJpegFrame->szJpegOut.uiSourceImgW = pstOut->uiImgW;
        pstJpegFrame->szJpegOut.uiSourceImgH = pstOut->uiImgH;
        pstJpegFrame->szJpegOut.uiOffsetX = pstSysFrame->uiDataAddr;

        if (SAL_SOK != Sva_DrvGetCfgParam(chan, &pstJpegFrame->stIn))
        {
            SVA_LOGE("Get Cfg Failed! \n");
            return SAL_FAIL;
        }

        DSA_QuePut(pstJpegFullQue, (void *)pstJpegFrame, SAL_TIMEOUT_NONE);
    }
    else
    {
        SVA_LOGE("Jpeg Queue is Full!!!!!!!!!! chan %d \n", chan);
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvCheckTargetInfo
 * @brief:      目标信息校验
 * @param[in]:  SVA_PROCESS_OUT *pstProcOut
 * @param[in]:  const CHAR *pcCallerName
 * @param[in]:  const UINT32 uiCallerLine
 * @param[out]: None
 * @return:     VOID
 */
VOID Sva_DrvCheckTargetInfo(SVA_PROCESS_OUT *pstProcOut, const CHAR *pcCallerName, const UINT32 uiCallerLine)
{
    BOOL bTarInfoValid = SAL_TRUE;

    /* checker */
    SVA_DRV_CHECK_PTR(pstProcOut, exit, "pstProcOut == null!");
    SVA_DRV_CHECK_PTR(pcCallerName, exit, "pcCallerName == null!");

    if (pstProcOut->target_num > SVA_XSI_MAX_ALARM_NUM)
    {
        bTarInfoValid = SAL_FALSE;
        SVA_LOGE("invalid target num[%d] > max[%d] \n", pstProcOut->target_num, SVA_XSI_MAX_ALARM_NUM);
    }

    int i;
    for (i = 0; i < pstProcOut->target_num; i++)
    {
        if (i >= SVA_XSI_MAX_ALARM_NUM)
        {
            bTarInfoValid = SAL_FALSE;
            SVA_LOGE("invalid target num! continue! i %d, target_num %d, max %d \n",
                     i, pstProcOut->target_num, SVA_XSI_MAX_ALARM_NUM);
            continue;
        }

        if (pstProcOut->target[i].type >= SVA_MAX_ALARM_TYPE)
        {
            bTarInfoValid = SAL_FALSE;
            SVA_LOGE("invalid target type %d, max %d \n", pstProcOut->target[i].type, SVA_MAX_ALARM_TYPE);
        }

        if (pstProcOut->target[i].rect.x < 0.0f || pstProcOut->target[i].rect.y < 0.0f
                                                                                  || pstProcOut->target[i].rect.width > 1.0f || pstProcOut->target[i].rect.height > 1.0f)
        {
            bTarInfoValid = SAL_FALSE;

            SVA_LOGE("invalid target rect! [%f, %f], [%f, %f] \n",
                     pstProcOut->target[i].rect.x, pstProcOut->target[i].rect.y,
                     pstProcOut->target[i].rect.width, pstProcOut->target[i].rect.height);
        }
    }

    if (!bTarInfoValid)
    {
        SVA_LOGE("!!!!!!!!! invalid target info. caller %s, line %d \n", pcCallerName, uiCallerLine);
    }

exit:
    return;
}

/**
 * @function    Sva_DrvPrPkgInfo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvPrPkgInfo(SVA_PROCESS_OUT *pstOut, const CHAR *pcCallerName, const UINT32 uiCallerLine)
{
    return;  /* 默认关闭 */

    UINT32 i, w, h;

    if (pstOut->packbagAlert.MainViewPackageNum > 16)
    {
        SVA_LOGE("invalid main view pack num %d \n", pstOut->packbagAlert.MainViewPackageNum);
    }

    for (i = 0; i < pstOut->packbagAlert.MainViewPackageNum; i++)
    {
        w = SAL_alignDown((UINT32)(pstOut->packbagAlert.MainViewPackageLoc[i].PackageRect.width * 1280.0), 2);
        h = SAL_alignDown((UINT32)(pstOut->packbagAlert.MainViewPackageLoc[i].PackageRect.height * 1024.0), 2);

        if (w == 0 || h == 0)
        {
            SVA_LOGE("dbg: err! num %d, i %d, [%f, %f] [%f, %f], %s, %d\n", pstOut->packbagAlert.MainViewPackageNum, i,
                     pstOut->packbagAlert.MainViewPackageLoc[i].PackageRect.x,
                     pstOut->packbagAlert.MainViewPackageLoc[i].PackageRect.y,
                     pstOut->packbagAlert.MainViewPackageLoc[i].PackageRect.width,
                     pstOut->packbagAlert.MainViewPackageLoc[i].PackageRect.height,
                     pcCallerName, uiCallerLine);
        }
    }
}

/**
 * @function    Sva_DrvRsltChecker
 * @brief       结果有效性校验
 * @param[in]   SVA_SYNC_RESULT_S *pstRslt
 * @param[out]  SVA_SYNC_RESULT_S *pstRslt
 * @return
 */
static INT32 Sva_DrvRsltChecker(SVA_SYNC_RESULT_S *pstRslt)
{
    /* 算法分包不准确的窄包裹进行过滤 */
    UINT32 uiPkgTmpW = 0;
    SVA_DEV_PRM *pstSva_dev = NULL;

    uiPkgTmpW = (UINT32)(pstRslt->stProcOut.packbagAlert.package_loc.width * (float)SVA_MODULE_WIDTH);
    pstSva_dev = Sva_DrvGetDev(pstRslt->chan);
    SVA_DRV_CHECK_PTR(pstSva_dev, err, "pstSva_dev = null!");

    if (uiPkgTmpW <= pstSva_dev->stIn.uiPkgSplitFilter && pstRslt->stProcOut.packbagAlert.package_valid)
    {
        pstRslt->stProcOut.packbagAlert.package_valid = SAL_FALSE;
        pstRslt->stProcOut.target_num = 0;

        SVA_LOGW("pkg filter! uiPkgTmpW[%d], uiPkgSplitFilter[%d], frmNum[%d] \n",
                 uiPkgTmpW, pstSva_dev->stIn.uiPkgSplitFilter, pstRslt->stProcOut.frame_num);
    }

    /* 违禁品个数大于60，不编图 */
    if (pstRslt->stProcOut.target_num >= SVA_JPEG_CNT_MAX_NUM)
    {
        pstRslt->stProcOut.packbagAlert.package_valid = SAL_FALSE;
        SVA_LOGE("chan %d, pkgvalid %d, targetNum %d \n",
                 pstRslt->chan, pstRslt->stProcOut.packbagAlert.package_valid, pstRslt->stProcOut.target_num);
    }

    Sva_DrvCheckTargetInfo(&pstRslt->stProcOut, __FUNCTION__, __LINE__);

    Sva_DrvPrPkgInfo(&pstRslt->stProcOut, __FUNCTION__, __LINE__);

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Sva_DrvJudgeTarLocValid
 * @brief:      判断目标是否在裁剪包裹内部
 * @param[in]:  SVA_RECT_F *pstTarLoc
 * @param[in]:  SVA_RECT_F *pstCutPkgLoc
 * @param[in]:  SVA_RECT_F *pstCutPkgValidLoc
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL Sva_DrvJudgeTarLocValid(SVA_RECT_F *pstTarLoc, SVA_RECT_F *pstCutPkgLoc, SVA_RECT_F *pstCutPkgValidLoc)
{
    pstCutPkgValidLoc->x = pstCutPkgLoc->x;
    pstCutPkgValidLoc->y = pstCutPkgLoc->y;
    pstCutPkgValidLoc->width = pstCutPkgLoc->width;
    pstCutPkgValidLoc->height = pstCutPkgLoc->height;

    if (pstTarLoc->x + pstTarLoc->width / 2 > pstCutPkgValidLoc->x
        && pstTarLoc->y + pstTarLoc->height / 2 > pstCutPkgValidLoc->y
        && pstTarLoc->x + pstTarLoc->width / 2 < pstCutPkgValidLoc->x + pstCutPkgValidLoc->width
        && pstTarLoc->y + pstTarLoc->height / 2 < pstCutPkgValidLoc->y + pstCutPkgValidLoc->height)
    {
        return SAL_TRUE;
    }

#ifdef DEBUG_TRACE
    SAL_WARN("tar[%f, %f] [%f, %f], pkg[%f, %f] [%f, %f] \n",
             pstTarLoc->x, pstTarLoc->y, pstTarLoc->width, pstTarLoc->height,
             pstCutPkgValidLoc->x, pstCutPkgValidLoc->y, pstCutPkgValidLoc->width, pstCutPkgValidLoc->height);
#endif
    return SAL_FALSE;
}

/**
 * @function:   Sva_DrvGetTarCntFilterRslt
 * @brief:      对目标类别出现个数进行统计，便于后续类别过滤
 * @param[in]:  SVA_PROCESS_OUT *pstHalOut
 * @param[in]:  UINT32 *auiTarExistCnt
 * @param[in]:  UINT32 uiArrCnt
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Sva_DrvGetTarCntFilterRslt(SVA_PROCESS_OUT *pstHalOut, UINT32 *auiTarExistCnt, UINT32 uiArrCnt)
{
    UINT32 i = 0;
    UINT32 uiTarType = 0;

    SVA_DRV_CHECK_PRM(pstHalOut, SAL_FAIL);
    SVA_DRV_CHECK_PRM(auiTarExistCnt, SAL_FAIL);

    memset(auiTarExistCnt, 0x00, uiArrCnt * sizeof(UINT32));

    for (i = 0; i < pstHalOut->target_num; i++)
    {
        uiTarType = pstHalOut->target[i].type;
        if (uiTarType >= SVA_MAX_ALARM_TYPE)
        {
            SVA_LOGE("invalid target, i %d, type %d \n", i, uiTarType);
            continue;
        }

        auiTarExistCnt[uiTarType]++;
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvTransSplitTarLoc
 * @brief:      将集中判图分片中的目标坐标实现空间转换
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_RECT_F *pstTarLoc
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL Sva_DrvTransSplitTarLoc(UINT32 chan, SVA_PROCESS_OUT *pstOut, SVA_PROCESS_OUT *pstRealOut, UINT32 uiTarIdx)
{
    BOOL bValid = SAL_FALSE;
    UINT32 uiIdx = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_RECT_F *pstTarLoc = NULL;
    SVA_RECT_F *pstInsTotalPkgRect = NULL;
    SVA_TARGET *pstTarInfo = NULL;

    SVA_RECT_F stInsTarRect = {0};
    SVA_RECT_F stInsPkgValidRect = {0};

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PTR(pstSva_dev, err, "pstSva_dev == null!");

    pstInsTotalPkgRect = &pstRealOut->packbagAlert.stCutTotalPkgRect;

    pstTarInfo = &pstRealOut->target[uiTarIdx];
    pstTarLoc = &pstTarInfo->rect;
    bValid = Sva_DrvJudgeTarLocValid(pstTarLoc, pstInsTotalPkgRect, &stInsPkgValidRect);
    if (!bValid)
    {
        SVA_LOGD("target is not located int cut pkg! chan %d \n", chan);
        return SAL_FALSE;
    }

    uiIdx = pstSva_dev->u32SliceTarNum;
    sal_memcpy_s(&pstSva_dev->stSliceTarget[uiIdx], sizeof(SVA_TARGET), pstTarInfo, sizeof(SVA_TARGET));

#if 1  /* 裁剪上下边沿的整包目标坐标 */
    stInsTarRect.x = pstTarLoc->x <= stInsPkgValidRect.x \
                     ? 0.0    \
                     : (pstTarLoc->x - stInsPkgValidRect.x) / stInsPkgValidRect.width;
    stInsTarRect.y = pstTarLoc->y <= pstOut->packbagAlert.fRealPkgY \
                     ? 0.0 \
                     : (pstTarLoc->y - pstOut->packbagAlert.fRealPkgY) / pstOut->packbagAlert.fRealPkgH;
    stInsTarRect.width = pstTarLoc->x <= stInsPkgValidRect.x \
                         ? (pstTarLoc->x + pstTarLoc->width - stInsPkgValidRect.x) / stInsPkgValidRect.width \
                         : (pstTarLoc->x + pstTarLoc->width - pstTarLoc->x) / stInsPkgValidRect.width;
    stInsTarRect.height = pstTarLoc->y <= pstOut->packbagAlert.fRealPkgY \
                          ? (pstTarLoc->y + pstTarLoc->height - pstOut->packbagAlert.fRealPkgY) / pstOut->packbagAlert.fRealPkgH \
                          : (pstTarLoc->y + pstTarLoc->height - pstTarLoc->y) / pstOut->packbagAlert.fRealPkgH;
    sal_memcpy_s(&pstSva_dev->stSliceTarget[uiIdx].rect, sizeof(SVA_RECT_F), &stInsTarRect, sizeof(SVA_RECT_F));
#endif

#if 0  /* 未裁剪上下边沿的分片目标坐标 */

    sal_memcpy_s(&pstOut->raw_target[uiIdx], sizeof(SVA_TARGET), pstTarInfo, sizeof(SVA_TARGET));

    stInsTarRect.x = pstTarLoc->x <= stInsPkgValidRect.x \
                     ? 0.0  \
                     : (pstTarLoc->x - stInsPkgValidRect.x) / stInsPkgValidRect.width;
    stInsTarRect.y = pstTarLoc->y <= pstOut->packbagAlert.package_loc.y \
                     ? 0.0 \
                     : (pstTarLoc->y - pstOut->packbagAlert.package_loc.y) / pstOut->packbagAlert.package_loc.height;
    stInsTarRect.width = pstTarLoc->x <= stInsPkgValidRect.x \
                         ? (pstTarLoc->x + pstTarLoc->width - stInsPkgValidRect.x) / stInsPkgValidRect.width \
                         : (pstTarLoc->width) / stInsPkgValidRect.width;
    stInsTarRect.height = pstTarLoc->y <= pstOut->packbagAlert.package_loc.y \
                          ? (pstTarLoc->y + pstTarLoc->height - pstOut->packbagAlert.package_loc.y) / pstOut->packbagAlert.fRealPkgH \
                          : (pstTarLoc->height) / pstOut->packbagAlert.package_loc.height;

    sal_memcpy_s(&pstOut->raw_target[uiIdx].rect, sizeof(SVA_RECT_F), &stInsTarRect, sizeof(SVA_RECT_F));
#endif

    pstSva_dev->u32SliceTarNum++;

    SVA_LOGD("chan %d, tarNum %d, add new target! info: [%f, %f] [%f, %f] \n",
             chan, pstSva_dev->u32SliceTarNum, stInsTarRect.x, stInsTarRect.y, stInsTarRect.width, stInsTarRect.height);
    return SAL_TRUE;
err:
    return SAL_FAIL;
}

#if 1 /* 智能目标逻辑链处理，目前仅支持两种逻辑链: 1.笔记本中的电池不展示 2.手机中的电池不展示 */
#define SVA_OBJ_BOND_TAR_NUM	(2)           /* 逻辑关联链中的目标个数，目前仅一一对应 */
#define SVA_OBJ_MAX_NUM			(32)          /* 同一类型的目标单帧最多出现个数用于存放中间结果，暂定32 */

#define SVA_OBJ_BOND_TAR_CNT		(3)       /* 目标链涉及的目标种类数量 */
#define SVA_OBJ_BOND_LAPTOP_IDX		(8)       /* 笔记本的type索引 */
#define SVA_OBJ_BOND_PHONE_IDX		(7)       /* 手机的type索引 */
#define SVA_OBJ_BOND_BATTERY_IDX	(4)       /* 电池的type索引 */

/* 当前存在目标链类型枚举 */
typedef enum _SVA_OBJ_BOND_TYPE_
{
    SVA_OBJ_BOND_LAPTOP_BATTERY = 0,          /* 笔记本-电池 */
    SVA_OBJ_BOND_PHONE_BATTERY,               /* 手机-电池 */
    SVA_OBJ_BOND_TYPE_NUM,
} SVA_OBJ_BOND_TYPE_E;


/**
 * @function:   Sva_DrvJudgeObjBondTarExist
 * @brief:      判断物品类型是否存在于待过滤的目标链中
 * @param[in]:  UINT32 uiTarIdx
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL Sva_DrvJudgeObjBondTarExist(UINT32 uiTarIdx)
{
    BOOL bTarExist = SAL_FALSE;

    if (SVA_OBJ_BOND_LAPTOP_IDX == uiTarIdx
        || SVA_OBJ_BOND_PHONE_IDX == uiTarIdx
        || SVA_OBJ_BOND_BATTERY_IDX == uiTarIdx)
    {
        bTarExist = SAL_TRUE;
    }

    return bTarExist;
}

/**
 * @function:   Sva_DrvJudgeTarInRgn
 * @brief:      判断特定目标是否存在于指定区域
 * @param[in]:  SVA_RECT_F *pstTar
 * @param[in]:  SVA_RECT_F *pstTarRgn
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL Sva_DrvJudgeTarInRgn(SVA_RECT_F *pstTar, SVA_RECT_F *pstTarRgn)
{
    BOOL bTarInRgn = SAL_FALSE;

    if (pstTar->x + pstTar->width / 2.0 > pstTarRgn->x
        && pstTar->x + pstTar->width / 2.0 < pstTarRgn->x + pstTarRgn->width
        && pstTar->y + pstTar->height / 2.0 > pstTarRgn->y
        && pstTar->y + pstTar->height / 2.0 < pstTarRgn->y + pstTarRgn->height)
    {
        bTarInRgn = SAL_TRUE;
    }

    SVA_LOGD("bTarInRgn %d \n", bTarInRgn);
    return bTarInRgn;
}

/**
 * @function:   Sva_DrvObjBondFilter
 * @brief:      对单帧画面中的目标进行逻辑链过滤
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_PROCESS_OUT *pstXsiOut
 * @param[in]:  SVA_PROCESS_OUT *pstFilterOut
 * @param[out]: None
 * @return:     INT32
 */
static INT32 ATTRIBUTE_UNUSED Sva_DrvObjBondFilter(UINT32 chan, SVA_PROCESS_OUT *pstXsiOut, SVA_PROCESS_OUT *pstFilterOut)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 uiTarType = 0;

    UINT32 auiObjBondTab[SVA_OBJ_BOND_TYPE_NUM][2] = {0};
    UINT32 auiTypeIdx[SVA_OBJ_BOND_TAR_CNT][SVA_OBJ_MAX_NUM] = {0};     /* 暂定同种危险品最多出现32个 */
    UINT32 auiTypeIdxValid[SVA_OBJ_MAX_NUM] = {0};     /* 待过滤的电池索引 */
    UINT32 auiTypeIdxCnt[SVA_OBJ_BOND_TAR_CNT] = {0};     /* 逻辑链目标存在个数 */

    UINT32 auiTypeMapTab[SVA_OBJ_BOND_TAR_CNT] = {SVA_OBJ_BOND_LAPTOP_IDX, SVA_OBJ_BOND_PHONE_IDX, SVA_OBJ_BOND_BATTERY_IDX};

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    if (NULL == pstXsiOut || NULL == pstFilterOut)
    {
        SVA_LOGE("ptr null! %p, %p \n", pstXsiOut, pstFilterOut);
        return SAL_FAIL;
    }

    /* 不是集中判图模式，不进行目标过滤 */
    if (SAL_TRUE != pstXsiOut->bSplitMode)
    {
        return SAL_SOK;
    }

    /* 清空目标信息，用于后续进行逻辑关联过滤后更新 */
    pstFilterOut->target_num = 0;
    memset(&pstFilterOut->target[0], 0x00, sizeof(SVA_TARGET) * SVA_XSI_MAX_ALARM_NUM);

    /* 遍历一遍，将目标分成两类: 1.无需进行判断关联的目标；2.需要进行后续关联判断过滤的目标 */
    for (i = 0; i < pstXsiOut->target_num; i++)
    {
        uiTarType = pstXsiOut->target[i].type;
        if (Sva_DrvJudgeObjBondTarExist(uiTarType))
        {
            /* 置位存在的逻辑链目标标志+保存对应目标的索引下表 */
            if (uiTarType == auiTypeMapTab[0])
            {
                auiObjBondTab[SVA_OBJ_BOND_LAPTOP_BATTERY][0] = SAL_TRUE;

                if (auiTypeIdxCnt[0] + 1 > SVA_OBJ_MAX_NUM)
                {
                    SAL_ERROR("type %d obj cnt > 32! chan %d, target_num %d \n", auiTypeMapTab[0], chan, pstXsiOut->target_num);
                    goto save;
                }

                auiTypeIdx[0][auiTypeIdxCnt[0]++] = i;
            }
            else if (uiTarType == auiTypeMapTab[1])
            {
                auiObjBondTab[SVA_OBJ_BOND_PHONE_BATTERY][0] = SAL_TRUE;

                if (auiTypeIdxCnt[1] + 1 > SVA_OBJ_MAX_NUM)
                {
                    SAL_ERROR("type %d obj cnt > 32! chan %d, target_num %d \n", auiTypeMapTab[1], chan, pstXsiOut->target_num);
                    goto save;
                }

                auiTypeIdx[1][auiTypeIdxCnt[1]++] = i;
            }
            else if (uiTarType == auiTypeMapTab[2])
            {
                auiObjBondTab[SVA_OBJ_BOND_LAPTOP_BATTERY][1] = SAL_TRUE;
                auiObjBondTab[SVA_OBJ_BOND_PHONE_BATTERY][1] = SAL_TRUE;

                if (auiTypeIdxCnt[2] + 1 > SVA_OBJ_MAX_NUM)
                {
                    SAL_ERROR("type %d obj cnt > 32! chan %d, target_num %d \n", auiTypeMapTab[2], chan, pstXsiOut->target_num);
                    continue;     /* 异常目标都丢掉 */
                }

                auiTypeIdx[2][auiTypeIdxCnt[2]] = i;
                auiTypeIdxValid[auiTypeIdxCnt[2]++] = SAL_TRUE;

                continue;     /* 目前只有电池需要过滤 */
            }
        }

save:
        /* 保存非逻辑关联目标信息，作为有效数据处理 */
        memcpy(&pstFilterOut->target[pstFilterOut->target_num++], &pstXsiOut->target[i], sizeof(SVA_TARGET));
    }

    SVA_LOGD("0-----------battery cnt %d, chan %d, tarNum %d, real %d \n",
             auiTypeIdxCnt[2], chan, pstFilterOut->target_num, pstXsiOut->target_num);

    if (auiObjBondTab[0][1] && auiObjBondTab[0][0])
    {
        /* 对待过滤的目标判断是否满足需要关联判断的条件 */
        for (i = 0; i < auiTypeIdxCnt[2]; i++)
        {
            if (SAL_TRUE != auiTypeIdxValid[i])
            {
                SVA_LOGD("1-2 continue \n");
                continue;
            }

            SVA_LOGD("1-2-------------i %d entering! \n", i);

            /*
               当前处理逻辑:
               1. 依次遍历待过滤的目标，目前这个目标类别仅有电池一个；
               2. 针对每次过滤的目标，依次遍历涉及到的目标链(此处新增父、子节点概念，比如笔记本为父节点，笔记本中的电池为子节点)，
                  是否满足子节点完全包含在父节点中的条件。若是，此子节点的标志位置为FALSE无效。
             */
            for (j = 0; j < auiTypeIdxCnt[0]; j++)
            {
                SVA_LOGD("1-2----------[%f, %f], [%f, %f], [%f, %f], [%f, %f], i %d, laptop_cnt %d \n",
                         pstXsiOut->target[auiTypeIdx[2][i]].rect.x, pstXsiOut->target[auiTypeIdx[2][i]].rect.y,
                         pstXsiOut->target[auiTypeIdx[2][i]].rect.width, pstXsiOut->target[auiTypeIdx[2][i]].rect.height,
                         pstXsiOut->target[auiTypeIdx[0][j]].rect.x, pstXsiOut->target[auiTypeIdx[0][j]].rect.y,
                         pstXsiOut->target[auiTypeIdx[0][j]].rect.width, pstXsiOut->target[auiTypeIdx[0][j]].rect.height,
                         i, auiTypeIdxCnt[0]);

                if (Sva_DrvJudgeTarInRgn(&pstXsiOut->target[auiTypeIdx[2][i]].rect, &pstXsiOut->target[auiTypeIdx[0][j]].rect))
                {
                    auiTypeIdxValid[i] = SAL_FALSE;
                    SVA_LOGD("1-2---------- i %d, j %d \n", i, j);
                }
            }
        }
    }

    if (auiObjBondTab[1][1] && auiObjBondTab[1][0])
    {
        /* 对待过滤的目标判断是否满足需要关联判断的条件 */
        for (i = 0; i < auiTypeIdxCnt[2]; i++)
        {
            if (SAL_TRUE != auiTypeIdxValid[i])
            {
                SVA_LOGD("1-3 continue \n");
                continue;
            }

            /*
               当前处理逻辑:
               1. 依次遍历待过滤的目标，目前这个目标类别仅有电池一个；
               2. 针对每次过滤的目标，依次遍历涉及到的目标链(此处新增父、子节点概念，比如笔记本为父节点，笔记本中的电池为子节点)，
                  是否满足子节点完全包含在父节点中的条件。若是，此子节点的标志位置为FALSE无效。
             */
            SVA_LOGD("1-3----------[%f, %f], [%f, %f], [%f, %f], [%f, %f], i %d, phone_cnt %d \n",
                     pstXsiOut->target[auiTypeIdx[2][i]].rect.x, pstXsiOut->target[auiTypeIdx[2][i]].rect.y,
                     pstXsiOut->target[auiTypeIdx[2][i]].rect.width, pstXsiOut->target[auiTypeIdx[2][i]].rect.height,
                     pstXsiOut->target[auiTypeIdx[1][j]].rect.x, pstXsiOut->target[auiTypeIdx[1][j]].rect.y,
                     pstXsiOut->target[auiTypeIdx[1][j]].rect.width, pstXsiOut->target[auiTypeIdx[1][j]].rect.height,
                     i, auiTypeIdxCnt[1]);

            for (j = 0; j < auiTypeIdxCnt[1]; j++)
            {
                if (Sva_DrvJudgeTarInRgn(&pstXsiOut->target[auiTypeIdx[2][i]].rect, &pstXsiOut->target[auiTypeIdx[1][j]].rect))
                {
                    auiTypeIdxValid[i] = SAL_FALSE;
                    SVA_LOGD("1-3---------- i %d, j %d \n", i, j);
                }
            }
        }
    }

    /* 将过滤后的目标拷贝到外部数据结构作为有效目标 */
    for (i = 0; i < auiTypeIdxCnt[2]; i++)
    {
        if (SAL_TRUE != auiTypeIdxValid[i])
        {
            continue;
        }

        /* 保存有效的子节点目标信息 */
        memcpy(&pstFilterOut->target[pstFilterOut->target_num++], &pstXsiOut->target[auiTypeIdx[2][i]], sizeof(SVA_TARGET));
    }

    SVA_LOGD("2---------tarNum %d, chan %d \n", pstFilterOut->target_num, chan);
    return SAL_SOK;
}

#endif

/**
 * @function:   Sva_DrvClearInsEncTmpVal
 * @brief:      清空即时编图临时变量
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     VOID
 */
static INT32 Sva_DrvClearInsEncTmpVal(UINT32 chan)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstSva_dev->stInsEncPrm.fPkgFwdX = 0.0;
    pstSva_dev->stInsEncPrm.fCutTotalW = 0.0;
    pstSva_dev->stInsEncPrm.fStartCutGap = 0.0;
    pstSva_dev->stInsEncPrm.fCutWidth = 0.0;
    pstSva_dev->stInsEncPrm.fStepCutGap = 0.0;
    pstSva_dev->stInsEncPrm.uiCalFrmNum = 0;     /* 重新开始计数 */
    pstSva_dev->stInsEncPrm.bCurPkgEnd = SAL_FALSE;
    pstSva_dev->stInsEncPrm.bPkgProcFlag = SAL_FALSE;

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvFillCutPkgInfo
 * @brief:      填充裁剪的包裹信息
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_PROCESS_OUT *pstHalOut
 * @param[out]: None
 * @return:     VOID
 */
static VOID Sva_DrvFillCutPkgInfo(UINT32 chan, SVA_PROCESS_IN *pstIn, SVA_PROCESS_OUT *pstHalOut)
{
    float fRealBorGap = 0.0;
    float fTmp = 0.0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_RECT_F *pstInsPkgRect = NULL;
    SVA_RECT_F *pstInsTotalPkgRect = NULL;
    SAV_INS_ENC_PRM_S *pstInsEncPrm = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    if (NULL == pstSva_dev)
    {
        SVA_LOGE("pstSva_dev == nulll! \n");
        return;
    }

    pstInsEncPrm = &pstSva_dev->stInsEncPrm;

    pstInsPkgRect = &pstInsEncPrm->stInsEncRslt.stInsPkgRect;
    pstInsTotalPkgRect = &pstInsEncPrm->stCutTotalPkgRect;

    fRealBorGap = pstInsEncPrm->bNewPkg ? fBorGap : 0.0;

    /* 用于规避包裹前沿检测不准时，连续包裹有效时裁剪remain数据 */
    if (pstSva_dev->stInsEncPrm.bLastPkgValid)
    {
        pstInsEncPrm->stInsEncRslt.bRemainValid = SAL_FALSE;
    }

    pstSva_dev->stInsEncPrm.bLastPkgValid = pstHalOut->packbagAlert.bSplitEnd;

    if (pstHalOut->packbagAlert.bSplitEnd)
    {
        /* 清空临时变量用于下一次处理 */
        if (SAL_TRUE != pstSva_dev->stInsEncPrm.bForceSplitProcFlag)
        {
            SVA_LOGI("not force split and clear tmp val enter! chan %d \n", chan);
            (VOID)Sva_DrvClearInsEncTmpVal(chan);
        }
        else
        {
            pstSva_dev->stInsEncPrm.bForceSplitProcFlag = SAL_FALSE;
            SVA_LOGI("reset force split proc flag! chan %d \n", chan);
        }

        pstInsPkgRect->x = pstHalOut->packbagAlert.package_loc.x > fBorGap  \
                           ? pstHalOut->packbagAlert.package_loc.x - fBorGap \
                           : 0.0;
        pstInsPkgRect->width = pstHalOut->packbagAlert.package_loc.x + pstHalOut->packbagAlert.package_loc.width + fBorGap > 1.0  \
                               ? 1.0 - pstInsPkgRect->x \
                               : pstHalOut->packbagAlert.package_loc.x + pstHalOut->packbagAlert.package_loc.width + fBorGap - pstInsPkgRect->x;
        pstInsPkgRect->y = pstHalOut->packbagAlert.fRealPkgY;
        pstInsPkgRect->height = pstHalOut->packbagAlert.fRealPkgH;

        pstInsTotalPkgRect->x = pstInsPkgRect->x;
        pstInsTotalPkgRect->width = pstInsPkgRect->width;

        /* remain cut rect proc */
        if (pstInsEncPrm->stInsEncRslt.bRemainValid)
        {
            fTmp = pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.x;

            pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.x = 0 == pstIn->enDirection  \
                                                              ? pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.x  \
                                                              : (fTmp > pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.width \
                                                                 ? fTmp - pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.width \
                                                                 : 0.0);

            pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.width = 0 == pstIn->enDirection  \
                                                                  ? (pstHalOut->packbagAlert.package_loc.x + pstHalOut->packbagAlert.package_loc.width + fBorGap > 1.0  \
                                                                     ? 1.0 - pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.x  \
                                                                     : pstHalOut->packbagAlert.package_loc.x + pstHalOut->packbagAlert.package_loc.width + fBorGap - pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.x) \
                                                                  : (fTmp > pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.x \
                                                                     ? fTmp - pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.x \
                                                                     : fTmp);     /* TODO: 增加异常保护 */
        }
    }
    else
    {
        if (pstInsEncPrm->bNewPkg)
        {
            pstInsEncPrm->bNewPkg = SAL_FALSE;
        }

        SVA_LOGD("cut gap %f, dir %d, fCutWidth %f, fStartCutGap %f \n",
                 fRealBorGap, pstIn->enDirection, pstInsEncPrm->fCutWidth, pstInsEncPrm->fStartCutGap);

        pstInsPkgRect->x = 0 == pstIn->enDirection   \
                           ? (pstInsEncPrm->fStartCutGap > fRealBorGap \
                              ? pstInsEncPrm->fStartCutGap - fRealBorGap   \
                              : 0.0)
                           : (pstInsEncPrm->fStartCutGap > pstInsEncPrm->fCutWidth  \
                              ? pstInsEncPrm->fStartCutGap - pstInsEncPrm->fCutWidth \
                              : 0.0);
        pstInsPkgRect->width = 0 == pstIn->enDirection   \
                               ? (pstInsEncPrm->fStartCutGap + pstInsEncPrm->fCutWidth < 1.0  \
                                  ? pstInsEncPrm->fStartCutGap + pstInsEncPrm->fCutWidth - pstInsPkgRect->x \
                                  : 1.0 - pstInsPkgRect->x) \
                               : (pstInsEncPrm->fStartCutGap + fRealBorGap <= 1.0f \
                                  ? pstInsEncPrm->fStartCutGap + fRealBorGap - pstInsPkgRect->x \
                                  : 1.0f - pstInsPkgRect->x);
        pstInsPkgRect->y = pstHalOut->packbagAlert.package_loc.y;
        pstInsPkgRect->height = pstHalOut->packbagAlert.package_loc.height;

        pstInsTotalPkgRect->x = 0 == pstIn->enDirection  \
                                ? (pstInsEncPrm->fPkgFwdX > fBorGap \
                                   ? pstInsEncPrm->fPkgFwdX - fBorGap   \
                                   : 0.0) \
                                : (pstInsEncPrm->fPkgFwdX > pstInsEncPrm->fCutTotalW \
                                   ? pstInsEncPrm->fPkgFwdX - pstInsEncPrm->fCutTotalW \
                                   : 0.0);
        pstInsTotalPkgRect->width = 0 == pstIn->enDirection  \
                                    ? (pstInsEncPrm->fPkgFwdX + pstInsEncPrm->fCutTotalW > 1.0f \
                                       ? 1.0f - pstInsTotalPkgRect->x \
                                       : pstInsEncPrm->fPkgFwdX + pstInsEncPrm->fCutTotalW - pstInsTotalPkgRect->x) \
                                    : (pstInsEncPrm->fPkgFwdX + fBorGap > 1.0f \
                                       ? 1.0f - pstInsTotalPkgRect->x \
                                       : pstInsEncPrm->fPkgFwdX + fBorGap - pstInsTotalPkgRect->x);
    }

    pstInsTotalPkgRect->y = pstInsPkgRect->y;
    pstInsTotalPkgRect->height = pstInsPkgRect->height;

    SVA_LOGI("Cut Rect: chan %d, dir %d, pkgFwdLoc %f, [%f, %f], [%f, %f], remain [%f, %f], [%f, %f], valid %d \n",
             chan, pstIn->enDirection, pstHalOut->packbagAlert.fPkgFwdLoc,
             pstInsPkgRect->x, pstInsPkgRect->y, pstInsPkgRect->width, pstInsPkgRect->height,
             pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.x, pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.y,
             pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.width, pstInsEncPrm->stInsEncRslt.stCutRemainPkgRect.height,
             pstHalOut->packbagAlert.bSplitEnd);

    return;
}

/**
 * @function:   Sva_DrvJudgePkgExist
 * @brief:      判断包裹是否已经出现
 * @param[in]:  UINT32 uiDir
 * @param[in]:  SVA_PROCESS_OUT *pstHalOut
 * @param[in]:  BOOL *puiPkgExist
 * @param[out]: None
 * @return:     BOOL
 */
static INT32 Sva_DrvJudgePkgExist(UINT32 uiDir, float fPkgFwdLoc, SVA_PROCESS_OUT *pstHalOut, BOOL *pbPkgExist)
{
    BOOL bPkgExist = SAL_FALSE;

#if 0 /* 不同于V1.6先产生包裹前沿再pkgValid时产生包裹location。算法升级到V2.0后，包裹前沿同包裹坐标一起出来 */
    if (0 == uiDir)
    {
        if (fPkgFwdLoc + SVA_VALID_PKG_FILTER_THRES < pstHalOut->packbagAlert.package_loc.x + pstHalOut->packbagAlert.package_loc.width)
        {
            bPkgExist = SAL_TRUE;
        }
    }
    else if (1 == uiDir)
    {
        if (fPkgFwdLoc > pstHalOut->packbagAlert.package_loc.x + SVA_VALID_PKG_FILTER_THRES)
        {
            bPkgExist = SAL_TRUE;
        }
    }
    else
    {
        SAL_WARN("invalid direction %d \n", uiDir);
        return SAL_FAIL;
    }

#endif
    *pbPkgExist = bPkgExist;
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvJudgeNewPkgExist
 * @brief:      判断是否有新包裹出现
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 uiDirection
 * @param[in]:  SVA_PROCESS_OUT *pstHalOut
 * @param[in]:  BOOL *pbNewPkg
 * @param[in]:  BOOL *pbPkgValid
 * @param[out]: None
 * @return:     INT32
 */
static INT32 ATTRIBUTE_UNUSED Sva_DrvJudgeNewPkgExist(UINT32 chan, UINT32 uiDirection, SVA_PROCESS_OUT *pstHalOut, BOOL *pbNewPkg, BOOL *pbPkgValid)
{
    BOOL bNewPkgExist = SAL_FALSE;

    float fNewPkgHead = 0.0;
    float fPkgLeftX = 0.0;
    float fPkgWidth = 0.0;

    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    fNewPkgHead = pstHalOut->packbagAlert.fPkgFwdLoc;
    fPkgLeftX = pstHalOut->packbagAlert.package_loc.x;
    fPkgWidth = pstHalOut->packbagAlert.package_loc.width;

    if (fPkgWidth > 0.001)
    {
        *pbPkgValid = SAL_TRUE;
    }

    /* 0表示从右往左过包，1表示从左往右 */
    if (!uiDirection)
    {
        if ((fNewPkgHead > fPkgLeftX + fPkgWidth
             || fNewPkgHead >= (pstSva_dev->stInsEncPrm.stRsltBufInfo.fLastFwdXBuf + SVA_NEW_PKG_FLOAT_GAP))
            && fNewPkgHead != 1.0f)
        {
            bNewPkgExist = SAL_TRUE;
        }
    }
    else if (1 == uiDirection)
    {
        if ((fNewPkgHead + SVA_VALID_PKG_FILTER_THRES < pstSva_dev->stInsEncPrm.stRsltBufInfo.fLastFwdXBuf
             || fNewPkgHead < fPkgLeftX)
            && fNewPkgHead != 0.0f)
        {
            bNewPkgExist = SAL_TRUE;
        }
    }
    else
    {
        SAL_WARN("invalid direction %d \n", uiDirection);
        return SAL_FAIL;
    }

    *pbNewPkg = bNewPkgExist;
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvJudgePkgFwdValid
 * @brief:      判断包裹前沿坐标是否有效
 * @param[in]:  UINT32 uiDirection
 * @param[in]:  SVA_PROCESS_OUT *pstHalOut
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL Sva_DrvJudgePkgFwdValid(SVA_PROCESS_IN *pstIn, SVA_PROCESS_OUT *pstHalOut)
{
    BOOL bValid = SAL_FALSE;
    UINT32 uiDirection = pstIn->enDirection;

    /* 0表示从右往左过包，1表示从左往右 */
    if (!uiDirection)
    {
        bValid = pstHalOut->packbagAlert.fPkgFwdLoc < pstIn->rect.x \
                 ? SAL_FALSE : SAL_TRUE;
    }
    else if (1 == uiDirection)
    {
        bValid = pstHalOut->packbagAlert.fPkgFwdLoc > pstIn->rect.x + pstIn->rect.width \
                 ? SAL_FALSE : SAL_TRUE;
    }
    else
    {
        SAL_WARN("invalid direction %d \n", uiDirection);
        return SAL_FALSE;
    }

    return bValid;
}

/**
 * @function:   Sva_DrvJudgeUseLastPkgFwd
 * @brief:      判断是否需要使用上一个包裹的前沿坐标进行处理
 * @param[in]:  BOOL bNewPkgExist
 * @param[in]:  SAV_INS_ENC_PRM_S *pstInsEncPrm
 * @param[out]: None
 * @return:     static BOOL
 */
static BOOL Sva_DrvJudgeUseLastPkgFwd(BOOL bNewPkgExist, SAV_INS_ENC_PRM_S *pstInsEncPrm)
{
    return ((SAL_TRUE == pstInsEncPrm->stPkgFwdCorctInfo.bPkgFwdNeedCorct) \
            || (SAL_TRUE == bNewPkgExist && SAL_TRUE == pstInsEncPrm->bPkgProcFlag));
}

/**
 * @function:   Sva_DrvClrPkgFwdCorctInfo
 * @brief:      清空包裹前沿矫正信息
 * @param[in]:  SAV_INS_ENC_PRM_S *pstInsEncPrm
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Sva_DrvClrPkgFwdCorctInfo(SVA_PROCESS_OUT *pstHalOut, SAV_INS_ENC_PRM_S *pstInsEncPrm)
{
    if (NULL == pstHalOut || NULL == pstInsEncPrm)
    {
        SVA_LOGE("ptr null! %p, %p \n", pstHalOut, pstInsEncPrm);
        return;
    }

    /* 包裹强制分割的这一帧不需要清空 */
    if (SAL_TRUE == pstHalOut->bPkgForceSplit)
    {
        SAL_ERROR("force split no need clr fwd corct info! \n");
        return;
    }

    pstInsEncPrm->stPkgFwdCorctInfo.bPkgFwdNeedCorct = SAL_FALSE;
    pstInsEncPrm->stPkgFwdCorctInfo.fSavedPkgFwd = 0.0f;
    pstInsEncPrm->stPkgFwdCorctInfo.uiFrmNum = 0;

    return;
}

/**
 * @function:   Sva_DrvSavePkgFwdCorctInfo
 * @brief:      保存包裹前沿矫正信息
 * @param[in]:  SAV_INS_ENC_PRM_S *pstInsEncPrm
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Sva_DrvSavePkgFwdCorctInfo(SAV_INS_ENC_PRM_S *pstInsEncPrm)
{
    UINT32 u32LastFwdBufIdx = 0;

    if (NULL == pstInsEncPrm)
    {
        SVA_LOGE("pstInsEncPrm == null! \n");
        return;
    }

    pstInsEncPrm->stPkgFwdCorctInfo.bPkgFwdNeedCorct = SAL_TRUE;
    pstInsEncPrm->stPkgFwdCorctInfo.fSavedPkgFwd = pstInsEncPrm->bLastPkgRealFwd;

    u32LastFwdBufIdx = (pstInsEncPrm->stRsltBufInfo.uiFwdBufIdx + SVA_PKG_FWD_BUF_NUM - 1) % SVA_PKG_FWD_BUF_NUM;
    pstInsEncPrm->stPkgFwdCorctInfo.uiFrmNum = pstInsEncPrm->stRsltBufInfo.u32FrmNum[u32LastFwdBufIdx];

    SVA_LOGD("save corct info! fSavedPkgFwd %f, uiFrmNum %d \n",
             pstInsEncPrm->stPkgFwdCorctInfo.fSavedPkgFwd, pstInsEncPrm->stPkgFwdCorctInfo.uiFrmNum);
    return;
}

/**
 * @function:   Sva_DrvGetPkgFwdCorctCalOffset
 * @brief:      基于包裹前沿矫正缓存计算偏移offset
 * @param[in]:  SAV_INS_ENC_PRM_S *pstInsEncPrm
 * @param[in]:  float *pfCalOffset
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvGetPkgFwdCorctCalOffset(SAV_INS_ENC_PRM_S *pstInsEncPrm, float *pfCalOffset)
{
    UINT32 i = 0;
    UINT32 u32SavedFrmNum = 0;

    float fCalFwdOffset = 0.0;

    if (NULL == pstInsEncPrm || NULL == pfCalOffset)
    {
        SVA_LOGE("ptr null! %p, %p \n", pstInsEncPrm, pfCalOffset);
        return SAL_FAIL;
    }

    u32SavedFrmNum = pstInsEncPrm->stPkgFwdCorctInfo.uiFrmNum;

    for (i = pstInsEncPrm->stRsltBufInfo.uiFwdBufIdx + SVA_PKG_FWD_BUF_NUM - 1; \
         pstInsEncPrm->stRsltBufInfo.u32FrmNum[i % SVA_PKG_FWD_BUF_NUM] > u32SavedFrmNum && i > pstInsEncPrm->stRsltBufInfo.uiFwdBufIdx; \
         i--)
    {
        fCalFwdOffset += pstInsEncPrm->stRsltBufInfo.fLastOffsetBuf[i % SVA_PKG_FWD_BUF_NUM];
    }

    if (i <= pstInsEncPrm->stRsltBufInfo.uiFwdBufIdx)
    {
        SAL_WARN("not found valid fwd buf! fCalFwdOffset %f, u32SavedFrmNum %d \n", fCalFwdOffset, u32SavedFrmNum);
        /* return SAL_FAIL; */
    }

    *pfCalOffset = fCalFwdOffset;
    return SAL_SOK;
}

#define SVA_UPDATE_FRM_GAP_FOR_FWD_CORCT_INFO (64)

/**
 * @function    Sva_DrvUpdateLastPkgFwd
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvUpdateLastPkgFwd(UINT32 uiDir, UINT32 uiCurFrmNum, float fCurOffset, SAV_INS_ENC_PRM_S *pstInsEncPrm)
{
    UINT32 i = 0;
    float fCalOffset = 0.0;

    SVA_PKG_FWD_CORCT_INFO_S *pstFwdCorctInfo = NULL;
    SVA_RSLT_BUF_INFO_S *pstRsltBufInfo = NULL;

    pstFwdCorctInfo = &pstInsEncPrm->stPkgFwdCorctInfo;
    pstRsltBufInfo = &pstInsEncPrm->stRsltBufInfo;

    if (uiCurFrmNum >= pstFwdCorctInfo->uiFrmNum + SVA_UPDATE_FRM_GAP_FOR_FWD_CORCT_INFO)
    {
        for (i = pstRsltBufInfo->uiFwdBufIdx + SVA_PKG_FWD_BUF_NUM - 1; \
             i > pstRsltBufInfo->uiFwdBufIdx; \
             i--)
        {
            if (pstRsltBufInfo->u32FrmNum[i % SVA_PKG_FWD_BUF_NUM] == pstFwdCorctInfo->uiFrmNum)
            {
                break;
            }

            fCalOffset += pstRsltBufInfo->fLastOffsetBuf[i % SVA_PKG_FWD_BUF_NUM];
        }

        if (i <= pstRsltBufInfo->uiFwdBufIdx)
        {
            SVA_LOGE("update corct info failed! not found last frm Num! \n");
            return;
        }

        SVA_LOGD("cocrt info update: from frm %d to %d, fwd %f to %f, cal %f \n",
                 pstFwdCorctInfo->uiFrmNum, uiCurFrmNum - 1, pstFwdCorctInfo->fSavedPkgFwd, pstFwdCorctInfo->fSavedPkgFwd - fCalOffset, fCalOffset);

        pstFwdCorctInfo->uiFrmNum = uiCurFrmNum - 1;
        pstFwdCorctInfo->fSavedPkgFwd = (1 == uiDir) ? pstFwdCorctInfo->fSavedPkgFwd + fCalOffset : pstFwdCorctInfo->fSavedPkgFwd - fCalOffset;
    }

    return;
}

/**
 * @function:   Sva_DrvGetNewPkgFwd
 * @brief:      获取真实包裹前沿位置
 * @param[in]:  UINT32 uiDir
 * @param[in]:  float *pfPkgFwd
 * @param[in]:  SAV_INS_ENC_PRM_S *pstInsEncPrm
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Sva_DrvGetNewPkgFwd(UINT32 uiDir,
                          BOOL bNewPkg,
                          UINT32 uiFrameNum,
                          float fCurOffset,
                          float *pfPkgFwd,
                          SAV_INS_ENC_PRM_S *pstInsEncPrm)
{
    UINT32 time0 = 0;
    UINT32 time1 = 0;

    float fCalFwdOffset = 0.0;
    float fPkgFwd = 0.0;

    if (NULL == pstInsEncPrm || NULL == pfPkgFwd)
    {
        SVA_LOGE("ptr null! %p, %p \n", pstInsEncPrm, pfPkgFwd);
        return SAL_FAIL;
    }

    time0 = SAL_getCurMs();

    fPkgFwd = *pfPkgFwd;

    if (SAL_TRUE != Sva_DrvJudgeUseLastPkgFwd(bNewPkg, pstInsEncPrm))
    {
        SVA_LOGD("no need use last pkg fwd info and do offset! \n");
        return SAL_SOK;
    }

    SVA_LOGD("need use last pkg fwd! raw %f, bNewPkg %d, bPkgProcFlag %d \n",
             fPkgFwd, bNewPkg, pstInsEncPrm->bPkgProcFlag);

    if (SAL_TRUE == bNewPkg && SAL_TRUE == pstInsEncPrm->bPkgProcFlag)
    {
        (VOID)Sva_DrvSavePkgFwdCorctInfo(pstInsEncPrm);
    }

    /* 满足包裹前沿纠正的条件时，需要更新用于矫正的包裹前沿坐标避免缓存个数不够 */
    (VOID)Sva_DrvUpdateLastPkgFwd(uiDir, uiFrameNum, fCurOffset, pstInsEncPrm);

    /* 目前存在这样一种情况: 当前包裹valid时，pkgFwd指向的是下一个包裹，此处进行规避。跳变值使用50pixels + 累积offset */
    if (SAL_SOK != Sva_DrvGetPkgFwdCorctCalOffset(pstInsEncPrm, &fCalFwdOffset))
    {
        SVA_LOGD("get pkg fwd corct cal offset failed! \n");
        return SAL_FAIL;
    }

    /* 需要计入当前帧的偏移量 */
    fCalFwdOffset += fCurOffset;

    SVA_LOGD("pkg fwd corct: raw %f \n", fPkgFwd);
    fPkgFwd = (0 == uiDir) \
              ? (pstInsEncPrm->stPkgFwdCorctInfo.fSavedPkgFwd - fCalFwdOffset) \
              : (pstInsEncPrm->stPkgFwdCorctInfo.fSavedPkgFwd + fCalFwdOffset);

    *pfPkgFwd = fPkgFwd;
    SVA_LOGD("get new %f, cal %f  \n", fPkgFwd, fCalFwdOffset);

    time1 = SAL_getCurMs();

    if (time1 - time0 > 1)
    {
        SAL_WARN("%s cost %d ms! \n", __FUNCTION__, time1 - time0);
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvJudgeProc
 * @brief:      判断是否需要裁图处理
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 uiDir
 * @param[in]:  SVA_PROCESS_OUT *pstHalOut
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL Sva_DrvJudgeProc(UINT32 chan, SVA_PROCESS_IN *pstIn, SVA_PROCESS_OUT *pstHalOut)
{
    BOOL bPkgExist = SAL_FALSE;
    BOOL bPkgStill = SAL_FALSE;

    UINT32 uiInputFrt = 0;
    UINT32 uiProcGap = 0;
    UINT32 uiDir = pstIn->enDirection;

    float fNewPkgHead = 0.0;
    float fPkgSplitFilter = 0.0;
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_RECT_F *pstPkgRemainRect = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    fNewPkgHead = pstHalOut->packbagAlert.fPkgFwdLoc;
    fPkgSplitFilter = (float)(pstIn->uiPkgSplitFilter) / (float)(SVA_MODULE_WIDTH);

    /* 不是集中判图模式，不进行后续处理 */
    if (SAL_TRUE != pstHalOut->bSplitMode)
    {
        return SAL_FALSE;
    }

    /* 更新分片序列开始FLAG */
    pstHalOut->packbagAlert.bSplitStart = pstSva_dev->stInsEncPrm.bNewPkg;

    /* 从右到左过包时不存在包裹前沿移动到画面左端尚未满足pkgEnd的情况，从左到右过包同理 */
    if (SAL_TRUE != Sva_DrvJudgePkgFwdValid(pstIn, pstHalOut))
    {
        SVA_LOGD("not valid pkgFwd value %f! chan %d \n", fNewPkgHead, chan);
        goto exit1;
    }

    /* 计算是否为新的包裹 */
#if 0 /* 使用是否为历史包裹更新判断是否为新包裹的条件，临时保留 */
    s32Ret = Sva_DrvJudgeNewPkgExist(chan, uiDir, pstHalOut, &bNewPkg, &bPkgValid);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("judge new pkg exist failed! chan %d \n", chan);
        goto exit1;
    }

    if (bNewPkg)
    {
        SVA_LOGD("get new pkg fwd %f ! frm %d \n", fNewPkgHead, pstHalOut->frame_num);
    }

#endif

#if 0
    /* 获取校正后的包裹前沿坐标，考虑到当前包裹valid前存在包裹前沿跳变的情况 */
    if (SAL_SOK != Sva_DrvGetNewPkgFwd(uiDir,
                                       bNewPkg,
                                       pstHalOut->frame_num,
                                       pstHalOut->frame_offset,
                                       &fNewPkgHead,
                                       &pstSva_dev->stInsEncPrm))
    {
        /* 非关键路径，失败不返回 */
        SVA_LOGD("get new pkg fwd failed! chan %d \n", chan);
    }

#endif
    pstSva_dev->stInsEncPrm.bLastPkgRealFwd = fNewPkgHead;

    /* 清空 */
    pstPkgRemainRect = &pstSva_dev->stInsEncPrm.stInsEncRslt.stCutRemainPkgRect;
    memset(pstPkgRemainRect, 0x00, sizeof(SVA_RECT_F));

    /* 若是新包裹，将当前状态信息复位 */
    SVA_LOGD("bNewPkg %d, bPkgProcFlag %d, last_valid %d, valid %d, fNewPkgHead %f, [%f, %f], [%f, %f], chan %d \n",
             pstSva_dev->stInsEncPrm.bNewPkg, pstSva_dev->stInsEncPrm.bPkgProcFlag, pstSva_dev->stInsEncPrm.bLastPkgValid,
             pstHalOut->packbagAlert.bSplitEnd, fNewPkgHead,
             pstHalOut->packbagAlert.package_loc.x, pstHalOut->packbagAlert.package_loc.y,
             pstHalOut->packbagAlert.package_loc.width, pstHalOut->packbagAlert.package_loc.height, chan);

    /* 新包裹出现 */
    if (pstSva_dev->stInsEncPrm.bNewPkg && !pstSva_dev->stInsEncPrm.bPkgProcFlag)
    {
        (VOID)Sva_DrvClearInsEncTmpVal(chan);

        pstSva_dev->stInsEncPrm.fPkgFwdX = fNewPkgHead;     /* 包裹前沿位置, 用于计算裁剪的部分小图的相对坐标 */
        pstSva_dev->stInsEncPrm.bPkgProcFlag = SAL_TRUE;
        pstSva_dev->stInsEncPrm.stInsEncRslt.bRemainValid = SAL_FALSE;

        SVA_LOGI("new pkg! chan %d, frmNum %d, fwd %f, uiCalFrmNum %d, filter %f \n",
                 chan, pstHalOut->frame_num, fNewPkgHead, pstSva_dev->stInsEncPrm.uiCalFrmNum, fPkgSplitFilter);
    }

    /* 若包裹信息完整，保存裁图坐标信息并返回，增加包裹宽度较小分包错误的过滤处理 */
    if (SAL_TRUE == pstHalOut->packbagAlert.bSplitEnd)
    {
        SVA_LOGI("szl_dbg: stillCnt %d, forceSplit %d, chan %d \n", uiStillCnt[chan], pstHalOut->bPkgForceSplit, chan);
        uiStillCnt[chan] = 0;

        /* 清空包裹前沿矫正信息 */
        (VOID)Sva_DrvClrPkgFwdCorctInfo(pstHalOut, &pstSva_dev->stInsEncPrm);

        if (pstHalOut->packbagAlert.package_loc.width > fPkgSplitFilter)
        {
            SVA_LOGI("full pkg exist chan %d, frmNum %d, uiCalFrmNum %d, %f! pkgFwd %f, id %d \n",
                     chan, pstHalOut->frame_num, pstSva_dev->stInsEncPrm.uiCalFrmNum, pstSva_dev->stInsEncPrm.fCutWidth, fNewPkgHead,
                     pstHalOut->packbagAlert.u32SplitPackageId);

            if (pstHalOut->packbagAlert.package_loc.width >= 0.6)
            {
                SVA_LOGW("Biggggggg Pkg exist! width size over 60 percent of the whole picture! [%f, %f] [%f, %f], chan %d \n",
                         pstHalOut->packbagAlert.package_loc.x, pstHalOut->packbagAlert.package_loc.y,
                         pstHalOut->packbagAlert.package_loc.width, pstHalOut->packbagAlert.package_loc.height, chan);
            }

            goto exit2;
        }
        else
        {
            SVA_LOGE("pkg w %f < fileter %f, chan %d \n", pstHalOut->packbagAlert.package_loc.width, fPkgSplitFilter, chan);
            goto exit1;
        }
    }

    /* 完整包裹回调结束后 到 下一个新包裹到来前，不进行包裹裁剪处理 */
    if (!pstSva_dev->stInsEncPrm.bNewPkg && !pstSva_dev->stInsEncPrm.bPkgProcFlag)
    {
        (VOID)Sva_DrvClearInsEncTmpVal(chan);

        if (pstHalOut->packbagAlert.bSplitEnd)
        {
            pstHalOut->packbagAlert.bSplitEnd = SAL_FALSE;
            SAL_WARN("Now in start status OR waiting for next pkg coming! And set pkgValid to false! frmNum %d, last_valid %d, valid %d, fNewPkgHead %f, [%f, %f], [%f, %f], chan %d \n",
                     pstHalOut->frame_num, pstSva_dev->stInsEncPrm.bLastPkgValid,
                     pstHalOut->packbagAlert.bSplitEnd, fNewPkgHead,
                     pstHalOut->packbagAlert.package_loc.x, pstHalOut->packbagAlert.package_loc.y,
                     pstHalOut->packbagAlert.package_loc.width, pstHalOut->packbagAlert.package_loc.height, chan);
        }

        SVA_LOGD("!!! %d, %d \n", pstSva_dev->stInsEncPrm.bNewPkg, pstSva_dev->stInsEncPrm.bPkgProcFlag);
        goto exit1;
    }

    /* 若包裹静止则不进行判断 */
    (VOID)Sva_HalJudgeStill(chan, pstHalOut, &bPkgStill);
    if (bPkgStill)     /* (pstHalOut->frame_offset <= 0.0) */
    {
        uiStillCnt[chan]++;

        SVA_LOGD("pkg still! pkgFwd %f, offset %f, frm %d, chan %d \n", fNewPkgHead, pstHalOut->frame_offset, pstHalOut->frame_num, chan);
        goto exit1;
    }

    /* DEBUG LOG */
    if (pstHalOut->frame_offset > 0.1 || pstHalOut->frame_offset < -0.1)
    {
        SVA_LOGE("dbg: invalid frame_offset %f, frmNum %d, chan %d \n", pstHalOut->frame_offset, pstHalOut->frame_num, chan);
    }

    /* 若当前包裹已结束，则等待完整包裹信息(主要是包含目标，因为目前算法返回的包裹信息先于目标信息) */
    if (pstSva_dev->stInsEncPrm.bCurPkgEnd)
    {
        SVA_LOGD("pkg end! frmNum %d, chan %d, pkgValid %d, calFrmNum %d \n",
                 pstHalOut->frame_num, chan, pstHalOut->packbagAlert.bSplitEnd, pstSva_dev->stInsEncPrm.uiCalFrmNum);
        pstSva_dev->stInsEncPrm.uiCalFrmNum = 0;
    }

    uiInputFrt = Sva_HalGetViInputFrmRate();     /* use moving pkg info */
    uiProcGap = uiInputFrt / SVA_INS_CUT_TIMES_PER_SEC;     /* 裁图间隔 */

    pstSva_dev->stInsEncPrm.uiCalFrmNum++;     /* 实际统计的帧数 */

    /*
       当前策略:
       若满足裁图间隔，保存裁图信息送入编图;
       若不满足裁图间隔，需要进行如下判断:
       1. 若满足包裹前沿指代的包裹未出现(注: 此处不包含包裹内的目标信息，注释中说明的完整信息均指包裹信息+目标信息，因为存在包裹先于目标出来)
          1.1 若满足包裹出现条件，置位相关标志位，等待完整包裹出现并送去裁图；
          1.2 若不满足，则送去累加计数偏移量；
       2. 若包裹前沿指向的包裹已出现
          1.1 若完整包裹已出现(见上述注明何为完整包裹)，则保存裁图信息并进入下一步；
          1.2 若未出现，则直接返回不送入后续裁图处理，并等待后续可能存在的完整包裹。

       处理周期以新包裹前沿信息出现即时刷新。
     */

    /* 当分片宽度不足32像素，且分片拓宽为32后未超出画面，则将分片宽度拓宽为32。否则继续在后续帧中累积分片宽度 */
    if (pstSva_dev->stInsEncPrm.uiCalFrmNum >= uiProcGap
        && (pstSva_dev->stInsEncPrm.fStepCutGap - SVA_MIN_SLICE_WIDTH) < 1e-6
        && (((0 == uiDir) && (fNewPkgHead + pstSva_dev->stInsEncPrm.fCutTotalW + pstSva_dev->stInsEncPrm.fStepCutGap) < 1.0f)
            || ((1 == uiDir) && (fNewPkgHead - pstSva_dev->stInsEncPrm.fCutTotalW - pstSva_dev->stInsEncPrm.fStepCutGap) > 0.0f)))
    {
        pstSva_dev->stInsEncPrm.fStepCutGap = SVA_MIN_SLICE_WIDTH;
        goto split;
    }

    if (pstSva_dev->stInsEncPrm.uiCalFrmNum < uiProcGap
        || ((pstSva_dev->stInsEncPrm.fStepCutGap - SVA_MIN_SLICE_WIDTH) < 1e-6
            && (((0 == uiDir) && (fNewPkgHead + pstSva_dev->stInsEncPrm.fCutTotalW + pstSva_dev->stInsEncPrm.fStepCutGap) > 1.0f)
                || ((1 == uiDir) && (fNewPkgHead - pstSva_dev->stInsEncPrm.fCutTotalW - pstSva_dev->stInsEncPrm.fStepCutGap) < 0.0f))))
    {
        if (!pstSva_dev->stInsEncPrm.bCurPkgEnd)
        {
            if (SAL_SOK != Sva_DrvJudgePkgExist(uiDir, fNewPkgHead, pstHalOut, &bPkgExist))
            {
                SAL_ERROR("judge pkg exist failed! chan %d \n", chan);
                goto exit1;
            }

            if (bPkgExist)
            {
                pstSva_dev->stInsEncPrm.bCurPkgEnd = SAL_TRUE;
                SVA_LOGE("set cur pkg flag end! chan %d, frmNum %d, [%f, %f] [%f, %f], fNewPkgHead %f, fCutTotalW %f, fStepCutGap %f,  \n",
                         chan, pstHalOut->frame_num,
                         pstHalOut->packbagAlert.package_loc.x, pstHalOut->packbagAlert.package_loc.y,
                         pstHalOut->packbagAlert.package_loc.width, pstHalOut->packbagAlert.package_loc.height,
                         fNewPkgHead, pstSva_dev->stInsEncPrm.fCutTotalW, pstSva_dev->stInsEncPrm.fStepCutGap);
            }
            else
            {
                SVA_LOGD("full pkg not exist chan %d! frmNum %d, uiCalFrmNum %d, fStepCutGap %f, pkgFwd %f \n",
                         chan, pstHalOut->frame_num, pstSva_dev->stInsEncPrm.uiCalFrmNum,
                         pstSva_dev->stInsEncPrm.fStepCutGap, fNewPkgHead);
            }

            goto add;
        }
        else
        {
            if (pstHalOut->packbagAlert.bSplitEnd)
            {
                SAL_INFO("full pkg exist chan %d, frmNum %d, uiCalFrmNum %d, %f! pkgFwd %f \n",
                         chan, pstHalOut->frame_num, pstSva_dev->stInsEncPrm.uiCalFrmNum, pstSva_dev->stInsEncPrm.fCutWidth, fNewPkgHead);
                goto exit2;
            }
            else
            {
                SVA_LOGD("pkg end! chan %d, fNewPkgHead %f, frmNum %d \n", chan, fNewPkgHead, pstHalOut->frame_num);
                goto add;
            }
        }
    }

split:
    if (pstSva_dev->stInsEncPrm.uiCalFrmNum > uiProcGap)
    {
        SVA_LOGE("cal: split wait [%d] frames! \n", pstSva_dev->stInsEncPrm.uiCalFrmNum - uiProcGap);
    }

    SVA_LOGI("meet full gap! frmNum %d, pkgFwd %f, id %d, uiCalFrmNum %d, offset %f, fStartCutGap %f, fCutTotalW %f, fCutWidth %f, fStepCutGap %f, chan %d \n",
             pstHalOut->frame_num, fNewPkgHead, pstHalOut->packbagAlert.u32SplitPackageId, pstSva_dev->stInsEncPrm.uiCalFrmNum, pstHalOut->frame_offset, pstSva_dev->stInsEncPrm.fStartCutGap,
             pstSva_dev->stInsEncPrm.fCutTotalW, pstSva_dev->stInsEncPrm.fCutWidth, pstSva_dev->stInsEncPrm.fStepCutGap, chan);
    SVA_LOGD("get input frmRate %d, procGap %d \n", uiInputFrt, uiProcGap);

    pstSva_dev->stInsEncPrm.fPkgFwdX = fNewPkgHead;
    pstSva_dev->stInsEncPrm.fStartCutGap = 0 == uiDir     \
                                           ? pstSva_dev->stInsEncPrm.fPkgFwdX + pstSva_dev->stInsEncPrm.fCutTotalW  \
                                           : (pstSva_dev->stInsEncPrm.fPkgFwdX > pstSva_dev->stInsEncPrm.fCutTotalW  \
                                              ? pstSva_dev->stInsEncPrm.fPkgFwdX - pstSva_dev->stInsEncPrm.fCutTotalW \
                                              : 0.0);
    pstSva_dev->stInsEncPrm.fCutTotalW += pstSva_dev->stInsEncPrm.fStepCutGap;
    pstSva_dev->stInsEncPrm.fCutWidth = pstSva_dev->stInsEncPrm.fStepCutGap;
    pstSva_dev->stInsEncPrm.fStepCutGap = 0.0;
    pstSva_dev->stInsEncPrm.uiCalFrmNum = 0;     /* 重新开始计数 */

exit2:
#if 1   /* TODO: 判断当前裁剪位置是否超出包裹位置，若没有则将剩余位置进行裁剪一张图发送给应用 */
    SVA_LOGD("debug_cut: valid %d, dir %d, offset %f, fStartCutGap %f, fCutWidth %f, fPkgFwdX %f, fCutTotalW %f, [%f, %f] [%f, %f], chan %d \n",
             pstHalOut->packbagAlert.bSplitEnd, uiDir, pstHalOut->frame_offset, pstSva_dev->stInsEncPrm.fStartCutGap,
             pstSva_dev->stInsEncPrm.fCutWidth, fNewPkgHead, pstSva_dev->stInsEncPrm.fCutTotalW,
             pstHalOut->packbagAlert.package_loc.x, pstHalOut->packbagAlert.package_loc.y,
             pstHalOut->packbagAlert.package_loc.width, pstHalOut->packbagAlert.package_loc.height, chan);
    if (pstHalOut->packbagAlert.bSplitEnd && pstHalOut->packbagAlert.package_loc.width > fPkgSplitFilter)
    {
        pstPkgRemainRect->x = 0 == uiDir     \
                              ? (fNewPkgHead + pstSva_dev->stInsEncPrm.fCutTotalW < 1.0 \
                                 ? fNewPkgHead + pstSva_dev->stInsEncPrm.fCutTotalW \
                                 : ERR_FLOAT_VAL) \
                              : (fNewPkgHead > pstSva_dev->stInsEncPrm.fCutTotalW \
                                 ? fNewPkgHead - pstSva_dev->stInsEncPrm.fCutTotalW \
                                 : ERR_FLOAT_VAL);

        pstPkgRemainRect->y = pstIn->rect.y;

        pstPkgRemainRect->width = 0 == uiDir     \
                                  ? pstHalOut->packbagAlert.package_loc.x + pstHalOut->packbagAlert.package_loc.width + fBorGap > 1.0 \
                                  ? 1.0 - pstPkgRemainRect->x \
                                  : (pstHalOut->packbagAlert.package_loc.x + pstHalOut->packbagAlert.package_loc.width + fBorGap - pstPkgRemainRect->x) \
                                  : pstHalOut->packbagAlert.package_loc.x > fBorGap \
                                  ? pstPkgRemainRect->x - pstHalOut->packbagAlert.package_loc.x + fBorGap \
                                  : pstPkgRemainRect->x;

        pstPkgRemainRect->height = pstIn->rect.height;

        pstSva_dev->stInsEncPrm.stInsEncRslt.bRemainValid = SAL_FALSE;
        if (pstPkgRemainRect->x > 0.0 && pstPkgRemainRect->x < 1.0 && fabs(pstSva_dev->stInsEncPrm.fCutTotalW) > 1e-5)
        {
            pstSva_dev->stInsEncPrm.stInsEncRslt.bRemainValid = SAL_TRUE;
        }

        /* 若是强制包裹分割处理过程中，则不进行Remain分片逻辑 */
        if (SAL_TRUE == pstSva_dev->stInsEncPrm.bForceSplitProcFlag || pstPkgRemainRect->width < 1e-6)
        {
            pstSva_dev->stInsEncPrm.stInsEncRslt.bRemainValid = SAL_FALSE;

            SVA_LOGE("remain flag NAH!!! bForceSplitProcFlag %d, width %f \n", pstSva_dev->stInsEncPrm.bForceSplitProcFlag, pstPkgRemainRect->width);
        }

        SVA_LOGI("debug_cut: get remain valid true! id %d, remain_valid %d, fCutTotalW %f, chan %d, [%f, %f] [%f, %f], pkg: [%f, %f] [%f, %f] \n",
                 pstHalOut->packbagAlert.u32SplitPackageId, pstSva_dev->stInsEncPrm.stInsEncRslt.bRemainValid, pstSva_dev->stInsEncPrm.fCutTotalW, chan, pstPkgRemainRect->x, pstPkgRemainRect->y, pstPkgRemainRect->width, pstPkgRemainRect->height,
                 pstHalOut->packbagAlert.package_loc.x, pstHalOut->packbagAlert.package_loc.y,
                 pstHalOut->packbagAlert.package_loc.width, pstHalOut->packbagAlert.package_loc.height);
    }

#endif
    return SAL_TRUE;
add:
    pstSva_dev->stInsEncPrm.fStepCutGap += pstHalOut->frame_offset;
exit1:
    return SAL_FALSE;
}

/**
 * @function:   Sva_DrvJudgeZoomFastMovSts
 * @brief:      判断是否画面存在放大缩小或前拉回拉
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[out]: None
 * @return:     static BOOL
 */
static BOOL ATTRIBUTE_UNUSED Sva_DrvJudgeZoomFastMovSts(SVA_PROCESS_OUT *pstOut)
{
    /* 如果存在放大缩小+前拉回拉状态, 需要清空历史缓存包裹信息 */
    return (pstOut->bFastMovValid || pstOut->bZoomValid);
}

/*******************************************************************************
* 函数名  : Sva_DrvGetOutFromHalOut
* 描  述  : 保存算法回调接口返回的结果
* 输  入  : - chan     : 通道号
*         : - pstHalOut: 结果参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvGetOutFromHalOut(UINT32 chan, SVA_PROCESS_OUT *pstHalOut)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 type = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PROCESS_IN *pstIn = NULL;
    SVA_PROCESS_OUT *pstOut = NULL;
    SVA_PROCESS_OUT *pstRealOut = NULL;

    SVA_DRV_CHECK_PRM(pstHalOut, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstIn = &pstSva_dev->stIn;

    /* 当上一个包裹结束且需要更新gap标记使能时，进行gap更新 */
    if (pstIn->uiGapUpFlag && pstSva_dev->stInsEncPrm.bLastPkgValid)
    {
        fBorGap = (float)pstIn->uiGapPixel / (float)SVA_MODULE_WIDTH;
        pstIn->uiGapUpFlag = SAL_FALSE;

        SAL_ERROR("update gap pixel %d, %f \n", pstIn->uiGapPixel, fBorGap);
    }

    pstOut = &pstSva_dev->stTmpOut;

    SVA_DRV_CHECK_PRM(pstOut, SAL_FAIL);

    sal_memset_s(pstOut, sizeof(SVA_PROCESS_OUT), 0x00, sizeof(SVA_PROCESS_OUT));

    /* 定制项目: 将包裹高度区域设置为检测区域高度，临时规避裁剪小图时高度和Y不确定的问题 */
    pstHalOut->packbagAlert.fRealPkgY = pstHalOut->packbagAlert.package_loc.y;
    pstHalOut->packbagAlert.fRealPkgH = pstHalOut->packbagAlert.package_loc.height;
    pstHalOut->packbagAlert.package_loc.y = pstIn->rect.y;
    pstHalOut->packbagAlert.package_loc.height = pstIn->rect.height;

    /* 外部配置的集中判图模式开关 */
    pstOut->bSplitMode = g_sva_common.bSplitMode;

    if (SAL_TRUE == pstHalOut->bPkgForceSplit)
    {
        pstSva_dev->stInsEncPrm.bForceSplitProcFlag = pstHalOut->bPkgForceSplit;
        SVA_LOGW("save force split proc flag! \n");
    }

    /* SAL_WARN("dbg: chan %d, idx %d \n", chan, pstSva_dev->stInsEncPrm.uiFwdBufIdx); */
    /* 保存结果缓存信息，用于后续包裹前沿坐标矫正 */
    pstSva_dev->stInsEncPrm.stRsltBufInfo.fLastFwdXBuf = pstHalOut->packbagAlert.fPkgFwdLoc;
    pstSva_dev->stInsEncPrm.stRsltBufInfo.u32FrmNum[pstSva_dev->stInsEncPrm.stRsltBufInfo.uiFwdBufIdx] = pstHalOut->frame_num;
    pstSva_dev->stInsEncPrm.stRsltBufInfo.fLastOffsetBuf[pstSva_dev->stInsEncPrm.stRsltBufInfo.uiFwdBufIdx] = pstHalOut->frame_offset;
    pstSva_dev->stInsEncPrm.stRsltBufInfo.uiFwdBufIdx = (pstSva_dev->stInsEncPrm.stRsltBufInfo.uiFwdBufIdx + 1) % SVA_PKG_FWD_BUF_NUM;

#ifndef DSP_ISM/*安检机此处无效，无需警告打印*/
#if 1 /* todo: 将single_frame_offset的计算放到外层发送结果到主片前 */
    if (0 != uiLastTimeRef[chan])
    {
        if (pstHalOut->uiTimeRef > uiLastTimeRef[chan])
        {
            pstHalOut->single_frame_offset = pstHalOut->frame_offset / (float)((float)(pstHalOut->uiTimeRef - uiLastTimeRef[chan]) / 2.0);
        }
        else
        {
            SVA_LOGE("cur timeref[%d] <= last timeref[%d], chan %d \n", pstHalOut->uiTimeRef, uiLastTimeRef[chan], chan);
        }
    }
    else
    {
        SVA_LOGW("1st save timeref[%d], chan %d \n", pstHalOut->uiTimeRef, chan);
    }

    uiLastTimeRef[chan] = pstHalOut->uiTimeRef;
#endif
#endif
    /* 为了规避sd应用的sd操作，此处将包裹相关信息拿到外面，临时修改 */
    pstOut->packbagAlert.candidate_flag = pstHalOut->packbagAlert.candidate_flag;
    pstOut->packbagAlert.package_valid = pstHalOut->packbagAlert.package_valid;
    pstOut->packbagAlert.package_id = pstHalOut->packbagAlert.package_id;
    pstOut->packbagAlert.fPkgFwdLoc = pstHalOut->packbagAlert.fPkgFwdLoc;
    pstOut->packbagAlert.package_loc.x = pstHalOut->packbagAlert.package_loc.x;
    pstOut->packbagAlert.package_loc.width = pstHalOut->packbagAlert.package_loc.width;
    pstOut->packbagAlert.package_loc.y = pstHalOut->packbagAlert.package_loc.y;
    pstOut->packbagAlert.package_loc.height = pstHalOut->packbagAlert.package_loc.height;
    pstOut->packbagAlert.fRealPkgY = pstHalOut->packbagAlert.fRealPkgY;
    pstOut->packbagAlert.fRealPkgH = pstHalOut->packbagAlert.fRealPkgH;
    pstOut->packbagAlert.bSplitEnd = pstHalOut->packbagAlert.package_valid;  /* 分包完整，分片序列结束标记 */
    pstOut->packbagAlert.u32SplitPackageId = pstHalOut->packbagAlert.package_id;

    /* 在最新包裹出全后，更新包裹抠图两边预留像素宽度信息，避免突变 */
    pstSva_dev->stInsEncPrm.bLastPkgValid = pstOut->packbagAlert.bSplitEnd;

    pstOut->uiTimeRef = pstHalOut->uiTimeRef;
    pstOut->frame_offset = pstHalOut->frame_offset;
    pstOut->single_frame_offset = pstHalOut->single_frame_offset;
    pstOut->frame_sync_idx = pstHalOut->frame_sync_idx;
	pstOut->frame_stamp = pstHalOut->frame_stamp;

    if (pstSva_dev->uiSwitch == SAL_TRUE)
    {
        pstOut->bZoomValid = pstHalOut->bZoomValid;
        pstOut->bFastMovValid = pstHalOut->bFastMovValid;
        pstOut->bPkgForceSplit = pstHalOut->bPkgForceSplit;
        pstOut->frame_num = pstHalOut->frame_num;
        pstOut->frame_sync_idx = pstHalOut->frame_sync_idx;
        pstOut->frame_stamp = pstHalOut->frame_stamp;
        pstOut->uiTimeRef = pstHalOut->uiTimeRef;
        pstOut->frame_offset = (0 == pstIn->enDirection) ? pstHalOut->frame_offset : 0 - pstHalOut->frame_offset;
        pstOut->single_frame_offset = (0 == pstIn->enDirection) ? pstHalOut->single_frame_offset : 0 - pstHalOut->single_frame_offset;
        pstOut->xsi_out_type = pstHalOut->xsi_out_type;
        pstOut->packbagAlert.candidate_flag = pstHalOut->packbagAlert.candidate_flag;
        pstOut->packbagAlert.fPkgFwdLoc = pstHalOut->packbagAlert.fPkgFwdLoc;
        pstOut->packbagAlert.package_valid = pstHalOut->packbagAlert.package_valid;
        pstOut->packbagAlert.package_loc.x = pstHalOut->packbagAlert.package_loc.x;
        pstOut->packbagAlert.package_loc.y = pstHalOut->packbagAlert.package_loc.y;
        pstOut->packbagAlert.package_loc.width = pstHalOut->packbagAlert.package_loc.width;
        pstOut->packbagAlert.package_loc.height = pstHalOut->packbagAlert.package_loc.height;

        pstOut->packbagAlert.MainViewPackageNum = pstHalOut->packbagAlert.MainViewPackageNum;
        for (i = 0; i < pstOut->packbagAlert.MainViewPackageNum; i++)
        {
            pstOut->packbagAlert.MainViewPackageLoc[i].PackageRect.x = pstHalOut->packbagAlert.MainViewPackageLoc[i].PackageRect.x;
            pstOut->packbagAlert.MainViewPackageLoc[i].PackageRect.y = pstHalOut->packbagAlert.MainViewPackageLoc[i].PackageRect.y;
            pstOut->packbagAlert.MainViewPackageLoc[i].PackageRect.width = pstHalOut->packbagAlert.MainViewPackageLoc[i].PackageRect.width;
            pstOut->packbagAlert.MainViewPackageLoc[i].PackageRect.height = pstHalOut->packbagAlert.MainViewPackageLoc[i].PackageRect.height;
            pstOut->packbagAlert.MainViewPackageLoc[i].PackageValid = pstHalOut->packbagAlert.MainViewPackageLoc[i].PackageValid;
        }

        pstOut->packbagAlert.OriginalPackageNum = pstHalOut->packbagAlert.OriginalPackageNum;
        for (i = 0; i < pstOut->packbagAlert.OriginalPackageNum; i++)
        {
            memcpy(&pstOut->packbagAlert.OriginalPackageLoc[i], \
                   &pstHalOut->packbagAlert.OriginalPackageLoc[i], \
                   sizeof(SVA_XSIE_PACKAGE_T));
        }

        pstOut->uiOffsetX = pstHalOut->uiOffsetX;
        pstOut->uiImgW = pstHalOut->uiImgW;
        pstOut->uiImgH = pstHalOut->uiImgH;

        /* 安检机包裹分片处理，删掉可能会导致重复框 */
        pstOut->aipackinfo.packIndx = pstHalOut->aipackinfo.packIndx;
        pstOut->aipackinfo.packTop = pstHalOut->aipackinfo.packTop;
        pstOut->aipackinfo.packBottom = pstHalOut->aipackinfo.packBottom;
        pstOut->aipackinfo.completePackMark = pstHalOut->aipackinfo.completePackMark;

        (VOID)Sva_DrvGetTarCntFilterRslt(pstHalOut, (UINT32 *)&stPostProcPkgCntInfo[chan].uiTarCnt[0], SVA_MAX_ALARM_TYPE);

        /*
           针对存在逻辑关联的目标进行处理，目前仅支持两种关联逻辑过滤:
           1. 笔记本内部的电池
           2. 手机内部的电池
         */
        memcpy(&pstSva_dev->stFilterOut, pstHalOut, sizeof(SVA_PROCESS_OUT));

        /* (VOID)Sva_DrvObjBondFilter(chan, pstHalOut, &pstSva_dev->stFilterOut); */

        pstRealOut = &pstSva_dev->stFilterOut;    /* 使用过滤后的目标信息进行处理 */

        j = 0;
        for (i = 0; i < pstRealOut->target_num; i++)
        {
            type = pstRealOut->target[i].type;

            SVA_LOGD("cb: i %d, type %d, [%f, %f], [%f, %f] \n",
                     i, pstRealOut->target[i].type, pstRealOut->target[i].rect.x, pstRealOut->target[i].rect.y,
                     pstRealOut->target[i].rect.width, pstRealOut->target[i].rect.height);

            /* 算法给出所有危险品的结果，不支持的危险品需要dsp过滤  */
            if (SAL_TRUE != pstIn->alert[type].bInit)
            {
                SVA_LOGD("not support target %d \n", type);
                continue;
            }

            /* 未开启识别的细分类别需要过滤 */
            if (!pstIn->alert[type].sva_key)
            {
                continue;
            }

            if ((stPostProcPkgCntInfo[chan].uiTarCnt[0][type]) < pstIn->alert[type].sva_alert_tar_cnt)
            {
                stPostProcPkgCntInfo[chan].uiTarCnt[0][type] = 0;
                SVA_LOGD("type %d not meet up target cnt filter! \n", type);
                continue;
            }

            pstOut->target[j].alarm_flg = pstRealOut->target[i].alarm_flg;
            pstOut->target[j].ID = pstRealOut->target[i].ID;
            pstOut->target[j].type = type;
            pstOut->target[j].confidence = pstRealOut->target[i].confidence;
            pstOut->target[j].visual_confidence = pstRealOut->target[i].visual_confidence;
            pstOut->target[j].offset = pstRealOut->target[i].offset;
            pstOut->target[j].u32SubTypeIdx = au32ClassSubIdxTab[type];
            pstOut->target[j].merge_type = au32ClassSubMinIdxTab[type];

            pstOut->target[j].rect.x = pstRealOut->target[i].rect.x;
            pstOut->target[j].rect.y = pstRealOut->target[i].rect.y;
            pstOut->target[j].rect.width = pstRealOut->target[i].rect.width;
            pstOut->target[j].rect.height = pstRealOut->target[i].rect.height;

            pstOut->target[j].color = pstIn->alert[type].sva_color;

            sal_memcpy_s(pstOut->target[j].sva_name, SVA_ALERT_NAME_LEN, pstIn->alert[type].sva_name, SVA_ALERT_NAME_LEN);

            pstOut->target_num++;
            j++;
            SVA_LOGD("Obj Info Copy: type %d, key is on! \n", type);
        }

        pstOut->pos_info.xsi_pos_size = pstHalOut->pos_info.xsi_pos_size;
        sal_memcpy_s(pstOut->pos_info.xsi_pos_buf, SVA_POST_MSG_LEN, pstHalOut->pos_info.xsi_pos_buf, SVA_POST_MSG_LEN);
    }

    /* 中间结果拷贝到输出结构体中 */
    sal_memcpy_s(pstHalOut, sizeof(SVA_PROCESS_OUT), pstOut, sizeof(SVA_PROCESS_OUT));

    return SAL_SOK;
}

/**
 * @function   Sva_DrvSyncRsltToOuterModule
 * @brief      智能结果更新到外部模块(显示、封装)
 * @param[in]  SVA_SYNC_RESULT_S *pstRslt
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvSyncRsltToOuterModule(SVA_SYNC_RESULT_S *pstRslt)
{
    INT32 s32Ret = SAL_SOK;

    if (SAL_TRUE == IA_GetModTransFlag(IA_MOD_SVA)
        && DOUBLE_CHIP_SLAVE_TYPE == g_sva_common.uiDevType)
    {
        SVA_LOGD("slave chip, return success! \n");
        goto exit;
    }

    (VOID)Sva_HalUpDateOut(pstRslt->chan, &pstRslt->stProcOut);
    (VOID)Sva_HalSaveToPool(pstRslt->chan, &pstRslt->stProcOut);

exit:
    return s32Ret;
}

/**
 * @function    Sva_DrvGetLockFrame
 * @brief       加锁，获取匹配帧用于编图
 * @param[in]   SVA_SYNC_RESULT_S *pstRslt
 * @param[in]   SYSTEM_FRAME_INFO **ppSysFrmInfo
 * @param[in]   BOOL *pbNeedLock
 * @param[out]  none
 * @return      SAL_SOK or SAL_FAIL
 */
static INT32 Sva_DrvGetLockFrame(SVA_SYNC_RESULT_S *pstRslt, SYSTEM_FRAME_INFO **ppSysFrmInfo, BOOL *pbNeedLock)
{
    BOOL bTmpNeedLock = SAL_FALSE;
    SYSTEM_FRAME_INFO *pstTmpFrmInfo = NULL;

    /* checker */
    if (NULL == pstRslt || NULL == ppSysFrmInfo || NULL == pbNeedLock)
    {
        SVA_LOGE("ptr null! %p, %p, %p \n", pstRslt, ppSysFrmInfo, pbNeedLock);
        return SAL_FAIL;
    }

    /* 当前跑智能XSIE的设备有两种形态: 1. 智能跑在双芯片上，主片发帧数据从片跑智能；2. 智能跑在单芯片上，无需片间数据传输 */
    if (SAL_TRUE != IA_GetModTransFlag(IA_MOD_SVA))
    {
        SVA_DEV_PRM *pstSva_dev = Sva_DrvGetDev(pstRslt->chan);
        SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

        pstTmpFrmInfo = &pstSva_dev->stSysFrame;
    }
    else if (DOUBLE_CHIP_MASTER_TYPE == g_sva_common.uiDevType)
    {
        bTmpNeedLock = SAL_TRUE;

        (VOID)Sva_DrvLockMatchFrm(pstRslt->chan);

        (VOID)Sva_DrvFindMatchBuf(pstRslt, &pstTmpFrmInfo);

        SVA_DRV_CHECK_PTR(pstTmpFrmInfo, err, "pstTmpFrmInfo == null!");

        pstTmpFrmInfo->uiDataWidth = SVA_MODULE_WIDTH;
        pstTmpFrmInfo->uiDataHeight = SVA_MODULE_HEIGHT;
    }

    *ppSysFrmInfo = pstTmpFrmInfo;
    *pbNeedLock = bTmpNeedLock;

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Sva_DrvImgProcess
 * @brief       图像送编码处理
 * @param[in]   SVA_SYNC_RESULT_S *pstRslt
 * @param[in]   SYSTEM_FRAME_INFO *pstSysFrmInfo
 * @param[out]  none
 * @return
 */
static INT32 Sva_DrvImgProcess(SVA_SYNC_RESULT_S *pstRslt)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 ImgModFlag = 0;
    BOOL bNeedLock = SAL_FALSE;

    SYSTEM_FRAME_INFO *pstSysFrmInfo = NULL;

    SVA_DRV_CHECK_PTR(pstRslt, exit, "pstRslt == null!");

    if (SAL_TRUE == IA_GetModTransFlag(IA_MOD_SVA)
        && DOUBLE_CHIP_SLAVE_TYPE == g_sva_common.uiDevType)
    {
        SVA_LOGD("slave chip, return success! \n");
        return SAL_SOK;
    }

    /* fixme: 图片模式默认包裹无效，需要进行编图处理，此处为特殊处理，后续待整理 */
    if (SVA_PROC_MODE_IMAGE == pstRslt->enProcMode)
    {
        /* 图像模式不需要同步智能结果 */
        Sva_HalUpDateOut(pstRslt->chan, &pstRslt->stProcOut);
        Sva_HalSaveToPool(pstRslt->chan, &pstRslt->stProcOut);

        pstRslt->stProcOut.packbagAlert.package_valid = SAL_TRUE;
        ImgModFlag = SAL_TRUE;
    }

    /* 送入编图队列 */
    if (((SAL_TRUE == gDbgSaveJpg[pstRslt->chan]) && (SVA_PROC_MODE_IMAGE != pstRslt->enProcMode))
        || (pstRslt->stProcOut.stInsEncRslt.uiInsEncFlag)
        || ((SAL_TRUE == pstRslt->stProcOut.packbagAlert.package_valid)
            && (pstRslt->stProcOut.packbagAlert.package_loc.width > SVA_VALID_PKG_FILTER_THRES))
        || (SAL_TRUE == ImgModFlag))
    {
        s32Ret = Sva_DrvGetLockFrame(pstRslt, &pstSysFrmInfo, &bNeedLock);
        SVA_DRV_CHECK_RET(s32Ret, unlock, "Sva_DrvGetLockFrame failed!");

        if (pstRslt->stProcOut.packbagAlert.package_loc.y < fabs(1e-6)
            || pstRslt->stProcOut.packbagAlert.package_loc.height < fabs(1e-6))
        {
            SVA_LOGW("invalid! y %f, h %f, chan %d \n",
                     pstRslt->stProcOut.packbagAlert.package_loc.y,
                     pstRslt->stProcOut.packbagAlert.package_loc.height, pstRslt->chan);
        }

        /* 失败不返回报错，不影响核心处理逻辑，仅影响编图结果 */
        if (SAL_SOK != Sva_DrvSendEncYuv(pstRslt->chan, &pstRslt->stProcOut, pstSysFrmInfo))
        {
            SVA_LOGE("Send Enc Yuv Failed! chan %d \n", pstRslt->chan);
        }

unlock:
        if (SAL_TRUE == bNeedLock)
        {
            (VOID)Sva_DrvUnlockMatchFrm(pstRslt->chan);
        }
    }

    return SAL_SOK;
exit:
    return s32Ret;
}

/**
 * @function    Sva_DrvSendDataDualChip
 * @brief       跨芯片数据发送
 * @param[in]   SVA_ALG_RSLT_S *pstAlgRslt
 * @param[out]  none
 * @return
 */
static INT32 Sva_DrvSendDataDualChip(SVA_ALG_RSLT_S *pstAlgRslt)
{
    INT32 s32Ret = SAL_SOK;

    if (DOUBLE_CHIP_SLAVE_TYPE != g_sva_common.uiDevType)
    {
        SVA_LOGD("not slave chip, return success! \n");
        return SAL_SOK;
    }

#ifndef PCIE_M2S_DEBUG
#ifdef DSP_ISA
    /* 将算法返回的结果发送到主片 */
    s32Ret = Sva_DrvSendRsltToMainChip(pstAlgRslt);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvSendRsltToMainChip failed!");
#elif DSP_ISM
    s32Ret = Sva_DrvSendRsltToMainChip_ISM(pstAlgRslt);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvSendRsltToMainChip failed!");
#endif
#endif
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function   Sva_DrvDoSplitOnce
 * @brief      集中判图处理
 * @param[in]  UINT32 chan
 * @param[in]  SVA_PROCESS_OUT *pstHalOut
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvDoSplitOnce(UINT32 chan, SVA_PROCESS_OUT *pstHalOut)
{
    BOOL bNeedCutProc = SAL_FALSE;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SAV_INS_ENC_RSLT_S *pstInsEncRslt = NULL;
    SVA_PROCESS_OUT *pstOut = NULL;
    SVA_PROCESS_IN *pstIn = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstIn = &pstSva_dev->stIn;
    pstOut = &pstSva_dev->stTmpOut;

    pstInsEncRslt = &pstSva_dev->stInsEncPrm.stInsEncRslt;
    SAL_clear(pstInsEncRslt);

    /* pstOut->packbagAlert.bSplitEnd = pstHalOut->packbagAlert.package_valid; */
    /* pstOut->packbagAlert.bSplitStart = 0;//pstHalOut->packbagAlert.bSplitStart;  // 包裹起始标记在Sva_DrvJudgeProc接口内赋值 */

    /* 判断是否需要计算坐标。否则返回成功，并将即刻编图标志置为0 */
    bNeedCutProc = Sva_DrvJudgeProc(chan, pstIn, pstHalOut);
    if (!bNeedCutProc)
    {
        SVA_LOGD("no need proc! \n");
        pstInsEncRslt->bRemainValid = SAL_FALSE;

        /* 未满足集中判图分片处理条件，直接返回成功 */
        return SAL_SOK;
    }

    /* 分片处理标记置为true */
    pstInsEncRslt->uiInsEncFlag = SAL_TRUE;

    /* 若分包结束且最后一个分片无效，则不需要将uiInsEncFlag置为TRUE */
    if ((SAL_TRUE == pstHalOut->packbagAlert.bSplitEnd) && (SAL_TRUE != pstInsEncRslt->bRemainValid))
    {
        pstInsEncRslt->uiInsEncFlag = SAL_FALSE;
    }

    /* 填充裁剪位置信息 */
    (VOID)Sva_DrvFillCutPkgInfo(chan, pstIn, pstHalOut);

    SVA_LOGD("after fill cut info: chan %d, frmNum %d \n", chan, pstHalOut->frame_num);

    sal_memcpy_s(&pstOut->stInsEncRslt, sizeof(SAV_INS_ENC_RSLT_S), pstInsEncRslt, sizeof(SAV_INS_ENC_RSLT_S));

    return SAL_SOK;
}

/**
 * @function   Sva_DrvDeletePkgBufById
 * @brief      从包裹后处理缓存中删除特定目标
 * @param[in]  UINT32 chan
 * @param[in]  UINT32 u32DelIdx
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvDeletePkgBufById(UINT32 chan, UINT32 u32DelIdx)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (u32DelIdx >= SVA_PACKAGE_MAX_NUM)
    {
        SVA_LOGE("invalid delete idx %d, chan %d \n", u32DelIdx, chan);
        return SAL_FAIL;
    }

    memset(&pstSva_dev->astPostProcPkgInfo[u32DelIdx], 0x00, sizeof(SVA_POST_PROC_PKG_INFO_S));
    return SAL_SOK;
}

/**
 * @function   Sva_DrvInsertNewPkgBuf
 * @brief      向包裹后处理缓存中插入新目标
 * @param[in]  UINT32 chan
 * @param[in]  UINT32 u32PkgId
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvInsertNewPkgBuf(UINT32 chan, SVA_XSIE_PACKAGE_T *pstInstPkgInfo)
{
    UINT32 i;

    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    /* 寻找空闲分片组 */
    for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
    {
        if (SAL_TRUE == pstSva_dev->astPostProcPkgInfo[i].bValid)
        {
            continue;
        }

        break;
    }

    if (i >= SVA_PACKAGE_MAX_NUM)
    {
        SVA_LOGE("post process pkg group full! chan %d \n", chan);
        return SAL_FAIL;
    }

    pstSva_dev->astPostProcPkgInfo[i].bValid = SAL_TRUE;
    pstSva_dev->astPostProcPkgInfo[i].u32PkgId = pstInstPkgInfo->PakageID;

    /* 清空集中判图分片组信息 */
    memset(&pstSva_dev->astPostProcPkgInfo[i].stSliceBufPrm.stInsEncPrm, 0x00, sizeof(SAV_INS_ENC_PRM_S));
    pstSva_dev->astPostProcPkgInfo[i].stSliceBufPrm.stInsEncPrm.bNewPkg = SAL_TRUE;

    /* 记录新包裹前沿和当前绝对时间 */
    pstSva_dev->astPostProcPkgInfo[i].stPkgStartInfo.fPkgfwd = pstInstPkgInfo->PackageForwardLocat;

    SAL_getDateTime_tz(&pstSva_dev->astPostProcPkgInfo[i].stPkgStartInfo.absTime);
    SVA_LOGW("start: chan %d, id %d, fwd %f, time: %d-%d-%d %d-%d-%d:%d \n",
             chan, pstInstPkgInfo->PakageID, pstInstPkgInfo->PackageForwardLocat,
             pstSva_dev->astPostProcPkgInfo[i].stPkgStartInfo.absTime.year,
             pstSva_dev->astPostProcPkgInfo[i].stPkgStartInfo.absTime.month,
             pstSva_dev->astPostProcPkgInfo[i].stPkgStartInfo.absTime.day,
             pstSva_dev->astPostProcPkgInfo[i].stPkgStartInfo.absTime.hour,
             pstSva_dev->astPostProcPkgInfo[i].stPkgStartInfo.absTime.minute,
             pstSva_dev->astPostProcPkgInfo[i].stPkgStartInfo.absTime.second,
             pstSva_dev->astPostProcPkgInfo[i].stPkgStartInfo.absTime.milliSecond);

    return SAL_SOK;
}

#if 0  /* 包裹后处理缓存重置接口，暂时保留 */

/**
 * @function   Sva_DrvResetSliceGroup
 * @brief      重置分片组
 * @param[in]  UINT32 chan
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvResetSliceGroup(UINT32 chan)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    memset(&pstSva_dev->astSliceBufPrm[0], 0x00, sizeof(SVA_SPLIT_SLICE_BUF_PRM_S) * SVA_PACKAGE_MAX_NUM);

    SVA_LOGW("reset slice group end! chan %d \n", chan);
    return SAL_SOK;
}

#endif

/**
 * @function   Sva_DrvFindPkgBufByPkgId
 * @brief      寻找包裹后处理缓存中特定包裹id
 * @param[in]  UINT32 chan
 * @param[in]  UINT32 u32PkgId
 * @param[out] None
 * @return     static BOOL
 */
static BOOL Sva_DrvFindPkgBufByPkgId(UINT32 chan, UINT32 u32PkgId)
{
    UINT32 i;
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
    {
        if (SAL_TRUE == pstSva_dev->astPostProcPkgInfo[i].bValid
            && pstSva_dev->astPostProcPkgInfo[i].u32PkgId == u32PkgId)
        {
            return SAL_TRUE;
        }
    }

    return SAL_FALSE;
}

/**
 * @function   Sva_DrvUpdateFalseDetRslt
 * @brief      更新误检包裹信息到输出结构体中
 * @param[in]  UINT32 chan
 * @param[in]  UINT32 u32FalseDetPkgId
 * @param[in]  SVA_PROCESS_OUT *pstProcOut
 * @param[out] None
 * @return     VOID
 */
static VOID Sva_DrvUpdateFalseDetRslt(UINT32 chan, const UINT32 u32FalseDetPkgId, SVA_PROCESS_OUT *pstProcOut)
{
    SVA_PKG_FALSE_DET_RLST_S *pstFalseDetPkgRslt = NULL;

    /* checker */
    SVA_DRV_CHECK_PTR(pstProcOut, exit, "pstProcOut == null!");

    /* 包裹结果信息 */
    pstFalseDetPkgRslt = &pstProcOut->packbagAlert.stFalseDetPkgRslt;

    if (pstFalseDetPkgRslt->u32FalseDetPkgCnt >= SVA_MAX_PACKAGE_BUF_NUM)
    {
        SVA_LOGW("false det pkg buf full! chan %d \n", chan);
        goto exit;
    }

    /* 将误检包裹写入输出结构体，个数加一 */
    pstFalseDetPkgRslt->au32FalseDetPkgId[pstFalseDetPkgRslt->u32FalseDetPkgCnt++] = u32FalseDetPkgId;

    SVA_LOGD("fill false det pkg %d, chan %d \n", u32FalseDetPkgId, chan);
exit:
    return;
}

/**
 * @function   Sva_DrvProcFalseDetPkgCb
 * @brief      将误检包裹id回调给应用
 * @param[in]  UINT32 chan
 * @param[in]  UINT32 u32FalseDetPkgId
 * @param[out] None
 * @return     static VOID
 */
static VOID Sva_DrvProcFalseDetPkgCb(UINT32 chan, UINT32 u32FalseDetPkgId)
{
    UINT32 time[8];

    STREAM_ELEMENT stStreamEle = {0};
    SVA_DSP_FALSE_DET_PKG_OUT_S stSvaDspOut = {0};

    stStreamEle.chan = chan;
    stStreamEle.type = STREAM_ELEMENT_SVA_FALSE_DET_PKG;

    stSvaDspOut.uiPkgId = u32FalseDetPkgId;

    time[0] = SAL_getCurMs();

    /* 将消失包裹，即误检包裹回调给上层 */
    SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stSvaDspOut, sizeof(SVA_DSP_FALSE_DET_PKG_OUT_S));

    time[1] = SAL_getCurMs();

    SVA_LOGE("after proc false detect pkg [%d]! chan %d, cb cost %d ms. \n", stSvaDspOut.uiPkgId, chan, time[1] - time[0]);

    return;
}

/**
 * @function   Sva_DrvFalseDetPkgCbProc
 * @brief      误检包裹回调处理（仅双芯片主片使用）
 * @param[in]  SVA_SYNC_RESULT_S *pstRslt
 * @param[out] None
 * @return     VOID
 */
static VOID Sva_DrvFalseDetPkgCbProc(SVA_SYNC_RESULT_S *pstRslt)
{
    UINT32 i = 0;
    SVA_PKG_FALSE_DET_RLST_S *pstFalseDetPkgRslt = NULL;

    /* checker */
    SVA_DRV_CHECK_PTR(pstRslt, exit, "pstRslt == null!");

    if (DOUBLE_CHIP_MASTER_TYPE != g_sva_common.uiDevType)
    {
        SVA_LOGW("this is not master chip! no need proc pkg false detect! return ok \n");
        goto exit;
    }

    /* 误检包裹结果信息 */
    pstFalseDetPkgRslt = &pstRslt->stProcOut.packbagAlert.stFalseDetPkgRslt;

    /* 将误检包裹id回调给上层，用于及时清除集中判图客户端残留的误检包裹分片 */
    for (i = 0; i < pstFalseDetPkgRslt->u32FalseDetPkgCnt; i++)
    {
        Sva_DrvProcFalseDetPkgCb(pstRslt->chan, pstFalseDetPkgRslt->au32FalseDetPkgId[i]);
    }

exit:
    return;
}

/**
 * @function   Sva_DrvUpdatePkgPostProcBufInfo
 * @brief      包裹后处理1: 更新后处理缓存信息
 * @param[in]  UINT32 chan
 * @param[in]  SVA_PROCESS_OUT *pstProcOut
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvUpdatePkgPostProcBufInfo(UINT32 chan, SVA_PROCESS_OUT *pstProcOut)
{
    UINT32 i, j;

    SVA_DEV_PRM *pstSva_dev = NULL;

#if 0  /* 画面异常操作时（放大缩小、回拉前拉）清空动作 */
    if (SAL_TRUE != pstProcOut->bSplitMode)
    {
        /* 未开启集中判图模式，直接返回成功 */
        return SAL_SOK;
    }

    if (pstProcOut->bFastMovValid || pstProcOut->bZoomValid)
    {
        SVA_LOGE("slice: clear split group info!!! fastMove %d, zoom %d, chan %d \n",
                 pstProcOut->bFastMovValid, pstProcOut->bZoomValid, chan);

        (VOID)Sva_DrvResetSliceGroup(chan);
    }

#endif

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    /* 每一帧处理分片前需要将消失的包裹id清空掉，避免包裹误检占位导致buf满 */
    for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
    {
        if (SAL_TRUE != pstSva_dev->astPostProcPkgInfo[i].bValid)
        {
            continue;
        }

        for (j = 0; j < pstProcOut->packbagAlert.OriginalPackageNum; j++)
        {
            if (pstSva_dev->astPostProcPkgInfo[i].u32PkgId == pstProcOut->packbagAlert.OriginalPackageLoc[j].PakageID)
            {
                break;
            }
        }

        if (j >= pstProcOut->packbagAlert.OriginalPackageNum)
        {
            if (SAL_TRUE != pstSva_dev->astPostProcPkgInfo[i].bPkgDetValid)
            {
                /* 保存误检包裹id到输出结构体 */
                Sva_DrvUpdateFalseDetRslt(chan, pstSva_dev->astPostProcPkgInfo[i].u32PkgId, pstProcOut);

                /* 将误检包裹id回调给上层应用，用于及时清空集中判图客户端误检包裹分片 */
                Sva_DrvProcFalseDetPkgCb(chan, pstSva_dev->astPostProcPkgInfo[i].u32PkgId);
            }

            /* 包裹目标消失 or 误检目标需要从缓存中删除 */
            if (SAL_SOK != Sva_DrvDeletePkgBufById(chan, i))
            {
                SVA_LOGW("delete disappear pkg %d failed! chan %d \n", pstSva_dev->astPostProcPkgInfo[i].u32PkgId, chan);
            }
        }
    }

    for (i = 0; i < pstProcOut->packbagAlert.OriginalPackageNum; i++)
    {
        /* 如果该包裹是历史包裹，或者已经在后处理包裹缓存中存在则跳过 */
        if (SAL_TRUE == pstProcOut->packbagAlert.OriginalPackageLoc[i].IsHistoryPackage
            || SAL_TRUE == Sva_DrvFindPkgBufByPkgId(chan, pstProcOut->packbagAlert.OriginalPackageLoc[i].PakageID))
        {
            continue;
        }

        /* 新包裹插入后处理包裹缓存 */
        if (SAL_SOK != Sva_DrvInsertNewPkgBuf(chan, &pstProcOut->packbagAlert.OriginalPackageLoc[i]))
        {
            SVA_LOGE("slice: insert new pkg failed! chan %d \n", chan);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   Sva_DrvProcSplitSlice
 * @brief      包裹后处理2: 集中判图分片组
 * @param[in]  SVA_SYNC_RESULT_S *pstRslt
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvProcSplitSlice(SVA_SYNC_RESULT_S *pstRslt)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 j, k, tmp; /* tmp用于备份 */
    float f_tmp[8] = {0};
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PROCESS_IN *pstIn = NULL;

    /* 若集中判图未开启，直接返回成功 */
    if (SAL_TRUE != pstRslt->stProcOut.bSplitMode)
    {
        pstRslt->stProcOut.stInsEncRslt.uiInsEncFlag = SAL_FALSE;
        return SAL_SOK;
    }

    pstSva_dev = Sva_DrvGetDev(pstRslt->chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstIn = &pstSva_dev->stIn;

    tmp = pstRslt->stProcOut.packbagAlert.bSplitEnd;
    f_tmp[0] = pstRslt->stProcOut.packbagAlert.fPkgFwdLoc;
    f_tmp[1] = pstRslt->stProcOut.packbagAlert.package_loc.x;
    f_tmp[2] = pstRslt->stProcOut.packbagAlert.package_loc.width;

    /* 全局Rslt中分片组个数重置 */
    pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum = 0;

    for (j = 0; j < SVA_PACKAGE_MAX_NUM; j++)
    {
        /* 遍历寻找可用的分片组信息 */
        if (!pstSva_dev->astPostProcPkgInfo[j].bValid)
        {
            continue;
        }

        /* 在整屏画面中寻找指定包裹目标 */
        for (k = 0; k < pstRslt->stProcOut.packbagAlert.OriginalPackageNum; k++)
        {
            if (pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[k].PakageID == pstSva_dev->astPostProcPkgInfo[j].u32PkgId)
            {
                break;
            }
        }

        if (k >= pstRslt->stProcOut.packbagAlert.OriginalPackageNum)
        {
            SVA_LOGD("slice proc: not found id %d, OriginalPackageNum %d, chan %d \n",
                     pstSva_dev->astPostProcPkgInfo[j].u32PkgId, pstRslt->stProcOut.packbagAlert.OriginalPackageNum, pstRslt->chan);

            for (int on = 0; on < pstRslt->stProcOut.packbagAlert.OriginalPackageNum; on++)
            {
                SVA_LOGD("on %d, id %d, ish %d, [%f, %f] [%f, %f] \n", on,
                         pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[on].PakageID,
                         pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[on].IsHistoryPackage,
                         pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[on].PackageRect.x,
                         pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[on].PackageRect.y,
                         pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[on].PackageRect.width,
                         pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[on].PackageRect.height);
            }

            continue;
        }

        /* 获取待集中判图处理的包裹前沿，用于接下来的分片处理 */
        pstRslt->stProcOut.packbagAlert.fPkgFwdLoc = (SVA_ORIENTATION_TYPE_R2L == pstIn->enDirection) \
                                                     ? pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[k].PackageForwardLocat \
                                                     : 1.0f - pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[k].PackageForwardLocat;
        pstRslt->stProcOut.packbagAlert.package_loc.x = pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[k].PackageRect.x;
        pstRslt->stProcOut.packbagAlert.package_loc.width = pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[k].PackageRect.width;

        /* 若分片组包裹id不同于当前pkg_valid包裹id，那么需要将pkg_valid临时置为false */
        if (tmp)
        {
            pstRslt->stProcOut.packbagAlert.bSplitEnd = !!(pstSva_dev->astPostProcPkgInfo[j].u32PkgId == pstRslt->stProcOut.packbagAlert.u32SplitPackageId);

            SVA_LOGI("get pkg id %d, slice pkg id %d, chan %d, frmNum %d, tmp %d, bSplitEnd %d \n",
                     pstRslt->stProcOut.packbagAlert.u32SplitPackageId, pstSva_dev->astPostProcPkgInfo[j].u32PkgId, pstRslt->chan,
                     pstRslt->stProcOut.frame_num, tmp, pstRslt->stProcOut.packbagAlert.bSplitEnd);
        }

        /* 获取上一帧中全局分片组信息 */
        memcpy(&pstSva_dev->stInsEncPrm, &pstSva_dev->astPostProcPkgInfo[j].stSliceBufPrm.stInsEncPrm, sizeof(SAV_INS_ENC_PRM_S));

        /* 集中判图处理 */
        s32Ret = Sva_DrvDoSplitOnce(pstRslt->chan, &pstRslt->stProcOut);
        SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvSplitSliceProc failed!");

        /* 将当前帧分片组回写更新到全局分片组 */
        memcpy(&pstSva_dev->astPostProcPkgInfo[j].stSliceBufPrm.stInsEncPrm, &pstSva_dev->stInsEncPrm, sizeof(SAV_INS_ENC_PRM_S));

        /* 将更新后的帧分片组信息写到Rslt全局结构体中，用于后续编图 */
        if (pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum < SVA_PACKAGE_MAX_NUM)
        {
            memcpy(&pstRslt->stProcOut.packbagAlert.astSliceInsRslt[pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum].stSliceInsRsltBuf, \
                   &pstSva_dev->stInsEncPrm.stInsEncRslt, \
                   sizeof(SAV_INS_ENC_RSLT_S));

            /* 拷贝累积分片区域，用于计算目标坐标映射 */
            memcpy(&pstRslt->stProcOut.packbagAlert.astSliceInsRslt[pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum].stCutTotalPkgRect, \
                   &pstSva_dev->stInsEncPrm.stCutTotalPkgRect, \
                   sizeof(SVA_RECT_F));

            pstRslt->stProcOut.packbagAlert.astSliceInsRslt[pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum].u32PkgId = pstSva_dev->astPostProcPkgInfo[j].u32PkgId;
            pstRslt->stProcOut.packbagAlert.astSliceInsRslt[pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum].bStartFlag = pstRslt->stProcOut.packbagAlert.bSplitStart;
            pstRslt->stProcOut.packbagAlert.astSliceInsRslt[pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum].bEndFlag = pstRslt->stProcOut.packbagAlert.bSplitEnd;

            SVA_LOGD("debug: chan %d, id %d, start %d, end %d, valid %d \n",
                     pstRslt->chan, pstSva_dev->astPostProcPkgInfo[j].u32PkgId,
                     pstRslt->stProcOut.packbagAlert.bSplitStart,
                     pstRslt->stProcOut.packbagAlert.bSplitEnd,
                     pstRslt->stProcOut.packbagAlert.package_valid);

            pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum++;
        }
        else
        {
            SVA_LOGW("slice ins rslt num %d > max %d, chan %d \n",
                     pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum, SVA_PACKAGE_MAX_NUM, pstRslt->chan);
        }
    }

    pstRslt->stProcOut.packbagAlert.bSplitEnd = tmp;
    pstRslt->stProcOut.packbagAlert.fPkgFwdLoc = f_tmp[0];
    pstRslt->stProcOut.packbagAlert.package_loc.x = f_tmp[1];
    pstRslt->stProcOut.packbagAlert.package_loc.width = f_tmp[2];

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Sva_DrvProcPersonPkgMatch
 * @brief      包裹后处理3：人包精准关联
 * @param[in]  SVA_SYNC_RESULT_S *pstRslt
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvProcPersonPkgMatch(SVA_SYNC_RESULT_S *pstRslt)
{
    UINT32 i, j;
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(pstRslt->chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    /* 若包裹已出全，更新人包匹配缓存 */
    for (i = 0; i < pstRslt->stProcOut.packbagAlert.OriginalPackageNum; i++)
    {
        if (SAL_TRUE == pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[i].IsHistoryPackage
            || SAL_TRUE != pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[i].PackageValid)
        {
            continue;
        }

        for (j = 0; j < SVA_PACKAGE_MAX_NUM; j++)
        {
            if (pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[i].PakageID == pstSva_dev->astPostProcPkgInfo[j].u32PkgId)
            {
                pstSva_dev->astPostProcPkgInfo[j].stPkgEndInfo.fPkgfwd = pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[i].PackageForwardLocat;

                SAL_getDateTime_tz(&pstSva_dev->astPostProcPkgInfo[j].stPkgEndInfo.absTime);
                SVA_LOGW("end: chan %d, id %d, fwd %f, time: %d-%d-%d %d-%d-%d:%d \n",
                         pstRslt->chan, pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[i].PakageID,
                         pstSva_dev->astPostProcPkgInfo[j].stPkgEndInfo.fPkgfwd,
                         pstSva_dev->astPostProcPkgInfo[j].stPkgEndInfo.absTime.year,
                         pstSva_dev->astPostProcPkgInfo[j].stPkgEndInfo.absTime.month,
                         pstSva_dev->astPostProcPkgInfo[j].stPkgEndInfo.absTime.day,
                         pstSva_dev->astPostProcPkgInfo[j].stPkgEndInfo.absTime.hour,
                         pstSva_dev->astPostProcPkgInfo[j].stPkgEndInfo.absTime.minute,
                         pstSva_dev->astPostProcPkgInfo[j].stPkgEndInfo.absTime.second,
                         pstSva_dev->astPostProcPkgInfo[j].stPkgEndInfo.absTime.milliSecond);

                break;
            }
        }
    }

    if (SAL_TRUE == pstRslt->stProcOut.packbagAlert.package_valid)
    {
        /* 若包裹已出全，拷贝人包匹配参数到外部结构体 */
        for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
        {
            if ((SAL_TRUE != pstSva_dev->astPostProcPkgInfo[i].bValid)
                || (pstRslt->stProcOut.packbagAlert.package_id != pstSva_dev->astPostProcPkgInfo[i].u32PkgId))
            {
                continue;
            }

            break;
        }

        if (i < SVA_PACKAGE_MAX_NUM)
        {
            /* 拷贝包裹出现在画面中及分包结束时的位置 */
            memcpy(&pstRslt->stProcOut.packbagAlert.stPkgStartInfo, &pstSva_dev->astPostProcPkgInfo[i].stPkgStartInfo, sizeof(SVA_PERSION_PKG_MATCH_PRM_S));
            memcpy(&pstRslt->stProcOut.packbagAlert.stPkgEndInfo, &pstSva_dev->astPostProcPkgInfo[i].stPkgEndInfo, sizeof(SVA_PERSION_PKG_MATCH_PRM_S));
        }
        else
        {
            SVA_LOGE("not found valid pkg [%d] in pkgPostProc buf!!! chan %d \n", pstRslt->stProcOut.packbagAlert.package_id, pstRslt->chan);

            /* 当匹配对的目标未找到时，打印当前后处理包裹缓存中的所有包裹id */
            for (j = 0; j < SVA_PACKAGE_MAX_NUM; j++)
            {
                if (SAL_TRUE != pstSva_dev->astPostProcPkgInfo[j].bValid)
                {
                    continue;
                }

                SVA_LOGW("pr post pkg: j %d, id %d \n", j, pstSva_dev->astPostProcPkgInfo[j].u32PkgId);
            }

            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   Sva_DrvDeletePkgPostProcBuf
 * @brief      包裹后处理4：删除分包完成的包裹缓存
 * @param[in]  SVA_SYNC_RESULT_S *pstRslt
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvDeletePkgPostProcBuf(SVA_SYNC_RESULT_S *pstRslt)
{
    UINT32 i;
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(pstRslt->chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (pstRslt->stProcOut.packbagAlert.package_valid)
    {
        for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
        {
            if (SAL_TRUE == pstSva_dev->astPostProcPkgInfo[i].bValid
                && pstSva_dev->astPostProcPkgInfo[i].u32PkgId == pstRslt->stProcOut.packbagAlert.package_id)
            {
                break;
            }
        }

        if (i >= SVA_PACKAGE_MAX_NUM)
        {
            SVA_LOGE("not found pkg [%d] in pkg post proc buf! chan %d \n", pstRslt->stProcOut.packbagAlert.package_id, pstRslt->chan);
            return SAL_FAIL;
        }

        pstSva_dev->astPostProcPkgInfo[i].bPkgDetValid = SAL_TRUE;

        /* 因双路包裹精准匹配需求，考虑到包裹上下堆叠，侧视角目标仅在画面中消失时才清空缓存，主视角或者单视角目标在检测完成后清空 */
        if (pstRslt->enProcMode != SVA_PROC_MODE_DUAL_CORRELATION
            || g_sva_common.uiMainViewChn == pstRslt->chan)
        {
            if (SAL_SOK != Sva_DrvDeletePkgBufById(pstRslt->chan, i))
            {
                SVA_LOGE("delete slice pkg [%d] failed! chan %d \n", pstRslt->stProcOut.packbagAlert.package_id, pstRslt->chan);
                return SAL_FAIL;
            }

            SVA_LOGI("====== delete pkg %d from buf! chan %d, valid %d, ins %d, end %d \n",
                     pstRslt->stProcOut.packbagAlert.package_id, pstRslt->chan, pstRslt->stProcOut.packbagAlert.package_valid,
                     pstRslt->stProcOut.stInsEncRslt.uiInsEncFlag, pstRslt->stProcOut.packbagAlert.bSplitEnd);
        }
    }

    return SAL_SOK;
}

/**
 * @function   Sva_DrvPkgPostProc
 * @brief      包裹后处理
 * @param[in]  SVA_SYNC_RESULT_S *pstRslt
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvPkgPostProc(SVA_SYNC_RESULT_S *pstRslt)
{
    INT32 s32Ret = SAL_FAIL;

    /* 包裹后处理1: 更新后处理缓存信息，包括删除已消失误检包裹目标+插入新包裹目标 */
    s32Ret = Sva_DrvUpdatePkgPostProcBufInfo(pstRslt->chan, &pstRslt->stProcOut);
    SVA_DRV_CHECK_RET(s32Ret, exit, "Sva_DrvUpdateSliceGroupInfo failed!");

    /* 包裹后处理2: 集中判图分片组处理，分片处理 */
    s32Ret = Sva_DrvProcSplitSlice(pstRslt);
    SVA_DRV_CHECK_RET(s32Ret, exit, "Sva_DrvProcSplitSlice failed!");

    /* 包裹后处理3：人包精准关联信息填充 */
    s32Ret = Sva_DrvProcPersonPkgMatch(pstRslt);
    SVA_DRV_CHECK_RET(s32Ret, exit, "Sva_DrvProcPersonPkgMatch failed!");

    /* 包裹后处理4：删除分包完成的包裹缓存 */
    s32Ret = Sva_DrvDeletePkgPostProcBuf(pstRslt);
    SVA_DRV_CHECK_RET(s32Ret, exit, "Sva_DrvDeletePkgPostProcBuf failed!");

exit:
    return s32Ret;
}

/**
 * @function   Sva_DrvSplitImgProcess
 * @brief      集中判图模式下送编图接口
 * @param[in]  SVA_SYNC_RESULT_S *pstRslt
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvSplitImgProcess(SVA_SYNC_RESULT_S *pstRslt)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 j, tmp[3];
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(pstRslt->chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    /* 备份 */
    tmp[0] = pstRslt->stProcOut.packbagAlert.package_valid;
    tmp[1] = pstRslt->stProcOut.packbagAlert.package_id;
    tmp[2] = pstRslt->stProcOut.packbagAlert.package_match_index;

    for (j = 0; j < pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum; j++)
    {
        memcpy(&pstRslt->stProcOut.stInsEncRslt, \
               &pstRslt->stProcOut.packbagAlert.astSliceInsRslt[j].stSliceInsRsltBuf, \
               sizeof(SAV_INS_ENC_RSLT_S));

        memcpy(&pstRslt->stProcOut.packbagAlert.stCutTotalPkgRect, \
               &pstRslt->stProcOut.packbagAlert.astSliceInsRslt[j].stCutTotalPkgRect, \
               sizeof(SVA_RECT_F));

        /* 更新分片组标记到外部，指导后续编图 */
        pstRslt->stProcOut.packbagAlert.package_valid = (tmp[0] && (pstRslt->stProcOut.packbagAlert.astSliceInsRslt[j].u32PkgId == pstRslt->stProcOut.packbagAlert.package_id));
        pstRslt->stProcOut.packbagAlert.bSplitStart = pstRslt->stProcOut.packbagAlert.astSliceInsRslt[j].bStartFlag;
        pstRslt->stProcOut.packbagAlert.bSplitEnd = pstRslt->stProcOut.packbagAlert.astSliceInsRslt[j].bEndFlag;
        pstRslt->stProcOut.packbagAlert.package_id = pstRslt->stProcOut.packbagAlert.astSliceInsRslt[j].u32PkgId;

        /* 单帧处理中存在多个包裹的分片处理，此处对于非valid的包裹match_id置0 */
        if (!pstRslt->stProcOut.packbagAlert.package_valid)
        {
            pstRslt->stProcOut.packbagAlert.package_match_index = 0;
        }

        /* 送编图 */
        s32Ret = Sva_DrvImgProcess(pstRslt);
        SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvImgProcess failed!");

        /* 恢复，用于下一次循环使用 */
        pstRslt->stProcOut.packbagAlert.package_id = tmp[1];
        pstRslt->stProcOut.packbagAlert.package_match_index = tmp[2];
    }

    /* 恢复 */
    pstRslt->stProcOut.packbagAlert.package_valid = tmp[0];
    pstRslt->stProcOut.packbagAlert.package_id = tmp[1];
    pstRslt->stProcOut.packbagAlert.package_match_index = tmp[2];

    /* 当没有分片数据时，仅处理整包结果 */
    if (0 == pstRslt->stProcOut.packbagAlert.u32SliceInsRsltNum
        && SAL_TRUE == pstRslt->stProcOut.packbagAlert.package_valid)
    {
        /* 送编图 */
        s32Ret = Sva_DrvImgProcess(pstRslt);
        SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvImgProcess failed!");
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Sva_DrvDoPostProcOnce
 * @brief      处理单个通道的后处理
 * @param[in]  SVA_SYNC_RESULT_S *pstRslt
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvDoPostProcOnce(SVA_SYNC_RESULT_S *pstRslt)
{
    INT32 s32Ret = SAL_FAIL;

    /* 结果有效性校验，比如小包裹宽度<50进行valid清空处理 */
    s32Ret = Sva_DrvRsltChecker(pstRslt);
    SVA_DRV_CHECK_RET(s32Ret, exit, "Sva_DrvRsltChecker failed!");

    /* 根据stIn解析hal层返回结果 */
    s32Ret = Sva_DrvGetOutFromHalOut(pstRslt->chan, &pstRslt->stProcOut);
    SVA_DRV_CHECK_RET(s32Ret, exit, "Sva_DrvGetOutFromHalOut failed!");

exit:
    return s32Ret;

}

/* 双视角包裹匹配初始化参数，静态不可改 */
static const DUAL_PKG_MATCH_INIT_PRM_S g_stDpmInitPrm =
{
    /* 初始化通道数，当前最多仅有两个视角进行匹配，故为2 */
    .iInitChnCnt = DUAL_PKG_MATCH_MAX_SUPT_CHN_NUM,

    /* 视角通道参数 */
    .astInitChnPrm =
    {
        /* 主视角 */
        {
            .enViewType = DUAL_PKG_MATCH_MAIN_VIEW,
            .stQuePrm =
            {
                .uiQueDepth = 16,
            },
        },

        /* 侧视角 */
        {
            .enViewType = DUAL_PKG_MATCH_SIDE_VIEW,
            .stQuePrm =
            {
                .uiQueDepth = 16,
            },
        },
    },
};

/**
 * @function    Sva_DrvGetPkgRectById
 * @brief       获取特定视角特定id的包裹矩形bbox
 * @param[in]   UINT32 u32PkgId
 * @param[in]   SVA_PACK_ALERT *pstPkgAlert
 * @param[out]  DUAL_PKG_MATCH_RECT_INFO_S *pstRect
 * @return
 */
static INT32 Sva_DrvGetPkgRectById(UINT32 u32PkgId,
                                   SVA_PACK_ALERT *pstPkgAlert,
                                   DUAL_PKG_MATCH_RECT_INFO_S *pstRect)
{
    UINT32 i;

    for (i = 0; i < pstPkgAlert->OriginalPackageNum; i++)
    {
        if (u32PkgId == pstPkgAlert->OriginalPackageLoc[i].PakageID)
        {
            memcpy(pstRect, &pstPkgAlert->OriginalPackageLoc[i].PackageRect, sizeof(DUAL_PKG_MATCH_RECT_INFO_S));

            SVA_LOGD("debug: get id %d, rect: [%f, %f], [%f, %f] \n", u32PkgId,
                     pstRect->x, pstRect->y, pstRect->w, pstRect->h);

            return SAL_SOK;
        }
    }

    SVA_LOGD("not found pkg %d from original xsi out! OriginalPackageNum %d \n", u32PkgId, pstPkgAlert->OriginalPackageNum);
    return SAL_FAIL;
}

/**
 * @function    Sva_DrvDpmGetPkgRectCb
 * @brief       双视角包裹匹配获取特定id的包裹Loc(回调函数)
 * @param[in]   VOID *pXsiRawOutInfo
 * @param[in]   DUAL_PKG_MATCH_VIEW_TYPE_E enViewType
 * @param[in]   INT32 iPkgId
 * @param[out]  DUAL_PKG_MATCH_RECT_INFO_S *pstRect
 * @return
 */
static INT32 Sva_DrvDpmGetPkgRectCb(VOID *pXsiRawOutInfo,
                                    DUAL_PKG_MATCH_VIEW_TYPE_E enViewType,
                                    INT32 iPkgId,
                                    DUAL_PKG_MATCH_RECT_INFO_S *pstRect)
{
    INT32 s32Ret = SAL_FAIL;

    /* 通道号向视角类型的映射表 */
    UINT32 au32ViewType2ChnIdx[3] =
    {
        /* 非法值 */
        -1,
        /* 对应主视角 */
        0,
        /* 对应侧视角 */
        1,
    };

    SVA_ALG_RSLT_S *pstAlgRslt = (SVA_ALG_RSLT_S *)pXsiRawOutInfo;
    SVA_SYNC_RESULT_S *pstSyncRslt = NULL;

    if (enViewType > DUAL_PKG_MATCH_MAX_SUPT_CHN_NUM)
    {
        SVA_LOGE("invalid view type %d \n", enViewType);
        goto exit;
    }

    pstSyncRslt = &pstAlgRslt->stAlgChnRslt[au32ViewType2ChnIdx[enViewType]];

    s32Ret = Sva_DrvGetPkgRectById(iPkgId, &pstSyncRslt->stProcOut.packbagAlert, pstRect);

exit:
    return s32Ret;
}

/**
 * @function    Sva_DrvUpdatePkgMatchRslt
 * @brief       更新包裹匹配后的包裹结果
 * @param[in]   DUAL_PKG_MATCH_RESULT_S *pstMatchRslt
 * @param[in]   SVA_ALG_RSLT_S *pstAlgRslt
 * @param[out]  SVA_ALG_RSLT_S *pstAlgRslt
 * @return
 */
static INT32 Sva_DrvUpdatePkgMatchRslt(const DUAL_PKG_MATCH_RESULT_S *pstMatchRslt,
                                       SVA_ALG_RSLT_S *pstAlgRslt)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i;

    SVA_SYNC_RESULT_S *pstSyncRslt = NULL;
    DUAL_PKG_MATCH_RECT_INFO_S stRect = {0};

    /* 如果非关联模式 or 集中判图开启，无需进行包裹匹配处理，直接返回成功 */
    if (SVA_PROC_MODE_DUAL_CORRELATION != pstAlgRslt->stAlgChnRslt[0].enProcMode)
    {
        return SAL_SOK;
    }

    for (i = 0; i < pstAlgRslt->uiChnCnt; i++)
    {
        pstSyncRslt = &pstAlgRslt->stAlgChnRslt[i];

        if (!pstMatchRslt->bHasMatch)
        {
            continue;
        }

        s32Ret = Sva_DrvGetPkgRectById(pstMatchRslt->auiMatchId[i], &pstSyncRslt->stProcOut.packbagAlert, &stRect);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("i %d, get rect failed! id %d \n", i, pstMatchRslt->auiMatchId[i]);
            goto exit;
        }

        /* 若找到指定id的包裹，将其rect更新到编图包裹信息中，指导后续编图处理 */
        memcpy(&pstSyncRslt->stProcOut.packbagAlert.package_loc, &stRect, sizeof(SVA_RECT_F));
        pstSyncRslt->stProcOut.packbagAlert.package_id = pstMatchRslt->auiMatchId[i];

        /* 此处为了兼容集中判图分片模式，pkg_valid时包裹裁图需要使用fRealPkgY和fRealPkgH */
        pstSyncRslt->stProcOut.packbagAlert.fRealPkgY = stRect.y;
        pstSyncRslt->stProcOut.packbagAlert.fRealPkgH = stRect.h;

        /* 为匹配上的包裹填充关联索引，当前使用0开始累加计数 */
        pstSyncRslt->stProcOut.packbagAlert.package_match_index = g_sva_common.stPkgMatchCommPrm.u32MatchIdx;

        /* 遍历完成后自加 */
        if (i == pstAlgRslt->uiChnCnt - 1)
        {
            ++g_sva_common.stPkgMatchCommPrm.u32MatchIdx;
        }
    }

    s32Ret = SAL_SOK;

exit:
    /* 若包裹未匹配上则不使能pkg_valid，即不送入后级编图处理 */
    pstAlgRslt->stAlgChnRslt[0].stProcOut.packbagAlert.package_valid   \
        = pstAlgRslt->stAlgChnRslt[1].stProcOut.packbagAlert.package_valid = (pstMatchRslt->bHasMatch && SAL_SOK == s32Ret);

    SVA_LOGD("match post proc: %d %d, bHasMatch %d, id_0 %d, id_1 %d, ret %d, u32MatchIdx %d \n",
             pstAlgRslt->stAlgChnRslt[0].stProcOut.packbagAlert.package_valid,
             pstAlgRslt->stAlgChnRslt[1].stProcOut.packbagAlert.package_valid,
             pstMatchRslt->bHasMatch, pstMatchRslt->auiMatchId[0], pstMatchRslt->auiMatchId[1],
             s32Ret, g_sva_common.stPkgMatchCommPrm.u32MatchIdx);

    return s32Ret;
}

/**
 * @function   Sva_DrvSetDualPkgMatchThresh
 * @brief      设置包裹匹配阈值
 * @param[in]  float fMatchThresh
 * @param[out] None
 * @return     INT32
 */
INT32 Sva_DrvSetDualPkgMatchThresh(float fMatchThresh)
{
    return dpm_set_match_thresh(fMatchThresh);
}

/**
 * @function   Sva_DrvPrintDpmDebugSts
 * @brief      打印包裹匹配模块内部调试信息
 * @param[in]  None
 * @param[out] None
 * @return     INT32
 */
INT32 Sva_DrvPrintDpmDebugSts(VOID)
{
    return dpm_print_debug_status();
}

/**
 * @function   Sva_DrvDualPkgMatch
 * @brief      双视角包裹匹配
 * @param[in]  SVA_ALG_RSLT_S *pstAlgRslt
 * @param[in]  DUAL_PKG_MATCH_RESULT_S *pstMatchRslt
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvDualPkgMatch(SVA_ALG_RSLT_S *pstAlgRslt, DUAL_PKG_MATCH_RESULT_S *pstMatchRslt)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i;
    /* 通道号向视角类型的映射表 */
    DUAL_PKG_MATCH_VIEW_TYPE_E aenChnIdx2ViewType[DUAL_PKG_MATCH_MAX_SUPT_CHN_NUM] = \
    {
        DUAL_PKG_MATCH_MAIN_VIEW,
        DUAL_PKG_MATCH_SIDE_VIEW
    };

    SVA_SYNC_RESULT_S *pstSyncRslt = NULL;

    DUAL_PKG_MATCH_OUTER_PKG_INFO_S stInsertPkgInfo = {0};
    DUAL_PKG_MATCH_CTRL_PRM_S stMatchCtrl = {0};

    /* 如果非关联模式 or 集中判图开启，无需进行包裹匹配处理，直接返回成功 */
    if (SVA_PROC_MODE_DUAL_CORRELATION != pstAlgRslt->stAlgChnRslt[0].enProcMode)
    {
        return SAL_SOK;
    }

    s32Ret = dpm_init(&g_stDpmInitPrm);
    SVA_DRV_CHECK_RET(s32Ret, exit, "dpm_init failed!");

    for (i = 0; i < pstAlgRslt->uiChnCnt; i++)
    {
        pstSyncRslt = &pstAlgRslt->stAlgChnRslt[i];

        if (!pstSyncRslt->stProcOut.packbagAlert.bSplitEnd)
        {
            continue;
        }

        if (pstSyncRslt->chan >= DUAL_PKG_MATCH_MAX_SUPT_CHN_NUM)
        {
            SVA_LOGE("invalid chan %d \n", pstSyncRslt->chan);
            s32Ret = SAL_FAIL;
            goto exit;
        }

        /* 填充待送入双视角包裹匹配模块的包裹信息，当前仅有pkg_id */
        stInsertPkgInfo.uiPkgId = pstSyncRslt->stProcOut.packbagAlert.package_id;

        s32Ret = dpm_insert_pkg_queue(aenChnIdx2ViewType[i], &stInsertPkgInfo);
        SVA_DRV_CHECK_RET(s32Ret, exit, "dpm_insert_pkg_queue failed!");
    }

    stMatchCtrl.pProcFunc = Sva_DrvDpmGetPkgRectCb;
    stMatchCtrl.pRawOutInfo = pstAlgRslt;

    s32Ret = dpm_do_pkg_match(&stMatchCtrl, pstMatchRslt);
    SVA_DRV_CHECK_RET(s32Ret, exit, "dpm_do_pkg_match failed!");

exit:
    return s32Ret;
}

/**
 * @function    Sva_DrvSendImgProcOnce
 * @brief       执行一次送编图处理
 * @param[in]   SVA_SYNC_RESULT_S *pstRslt
 * @param[out]  none
 * @return
 */
static INT32 Sva_DrvSendImgProcOnce(SVA_SYNC_RESULT_S *pstRslt)
{
    INT32 s32Ret = SAL_FAIL;

    /* 集中判图开启，需要循环遍历当前帧所有valid的分片组，送入后级编图处理。非集中判图时仅处理当前帧的pkgValid的包裹即可 */
    if (pstRslt->stProcOut.bSplitMode)
    {
        /* 集中判图，送编图 */
        s32Ret = Sva_DrvSplitImgProcess(pstRslt);
        SVA_DRV_CHECK_RET(s32Ret, exit, "Sva_DrvSplitImgProcess failed!");
    }
    else
    {
        /* 非集中判图，送编图 */
        s32Ret = Sva_DrvImgProcess(pstRslt);
        SVA_DRV_CHECK_RET(s32Ret, exit, "Sva_DrvImgProcess failed!");
    }

exit:
    return s32Ret;
}

/**
 * @function   Sva_DrvMergeDualViewTarget
 * @brief      将双视角匹配的包裹目标合并
 * @param[in]  const DUAL_PKG_MATCH_RESULT_S *pstMatchRslt
 * @param[in]  SVA_ALG_RSLT_S *pstAlgRslt
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvMergeDualViewTarget(const DUAL_PKG_MATCH_RESULT_S *pstMatchRslt, SVA_ALG_RSLT_S *pstAlgRslt)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i, j, k, u32PkgId, u32Idx;
    float fMidX, fMidY;

    SVA_PKG_MATCH_PAIR_INFO_S *pstPkgMatchPair = NULL;
    SVA_SYNC_RESULT_S *pstRslt = NULL;

    SVA_RECT_F stPkgRect[SVA_DEV_MAX] = {0};

    /* 非双路关联模式，无需处理双视角目标合并，直接返回成功 */
    if (SVA_PROC_MODE_DUAL_CORRELATION != pstAlgRslt->stAlgChnRslt[0].enProcMode
        || SVA_DEV_MAX != pstAlgRslt->uiChnCnt)
    {
        return SAL_SOK;
    }

    for (i = 0; i < SVA_MAX_PKG_MATCH_PAIR_NUM; i++)
    {
        pstPkgMatchPair = &g_sva_common.astPkgMatchPairInfo[i];
        if (SAL_TRUE != pstPkgMatchPair->bBusy)
        {
            continue;
        }

        for (j = 0; j < pstAlgRslt->uiChnCnt; j++)
        {
            u32PkgId = pstPkgMatchPair->au32PkgMatchPairId[1 - j];
            pstRslt = &pstAlgRslt->stAlgChnRslt[1 - j];

            for (k = 0; k < pstRslt->stProcOut.packbagAlert.OriginalPackageNum; k++)
            {
                if (u32PkgId == pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[k].PakageID)
                {
                    memcpy(&stPkgRect[1 - j], &pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[k].PackageRect, sizeof(SVA_RECT_F));
                    break;
                }
            }

            if (k >= pstRslt->stProcOut.packbagAlert.OriginalPackageNum)
            {
                SVA_LOGE("not found pkg %d, j %d \n", u32PkgId, j);
                goto exit;
            }
        }

        for (j = 0; j < pstAlgRslt->uiChnCnt; j++)
        {
            pstRslt = &pstAlgRslt->stAlgChnRslt[1 - j];

            for (k = 0; k < pstRslt->stProcOut.target_num; k++)
            {
                /* 另一个视角的目标不进行统计 */
                if (SAL_TRUE == pstRslt->stProcOut.target[k].bAnotherViewTar)
                {
                    continue;
                }

                fMidX = pstRslt->stProcOut.target[k].rect.x + pstRslt->stProcOut.target[k].rect.width / 2.0;
                fMidY = pstRslt->stProcOut.target[k].rect.y + pstRslt->stProcOut.target[k].rect.height / 2.0;

                if (fMidX >= stPkgRect[1 - j].x && fMidX <= stPkgRect[1 - j].x + stPkgRect[1 - j].width
                    && fMidY >= stPkgRect[1 - j].y && fMidY <= stPkgRect[1 - j].y + stPkgRect[1 - j].height)
                {
                    u32Idx = pstAlgRslt->stAlgChnRslt[j].stProcOut.target_num;

                    if (u32Idx >= SVA_XSI_MAX_ALARM_NUM)
                    {
                        SVA_LOGW("target num >= max %d \n", SVA_XSI_MAX_ALARM_NUM);
                        goto exit;
                    }

                    memcpy(&pstAlgRslt->stAlgChnRslt[j].stProcOut.target[u32Idx], \
                           &pstRslt->stProcOut.target[k], \
                           sizeof(SVA_TARGET));

                    /* 将该目标标记为来源于另一个视角，bbox设定为当前视角包裹几何中心，宽高为包裹宽高的一半 */
                    pstAlgRslt->stAlgChnRslt[j].stProcOut.target[u32Idx].bAnotherViewTar = SAL_TRUE;
                    pstAlgRslt->stAlgChnRslt[j].stProcOut.target[u32Idx].rect.x = stPkgRect[j].x + stPkgRect[j].width / 4.0;
                    pstAlgRslt->stAlgChnRslt[j].stProcOut.target[u32Idx].rect.y = stPkgRect[j].y + stPkgRect[j].height / 4.0;
                    pstAlgRslt->stAlgChnRslt[j].stProcOut.target[u32Idx].rect.width = stPkgRect[j].width / 2.0;
                    pstAlgRslt->stAlgChnRslt[j].stProcOut.target[u32Idx].rect.height = stPkgRect[j].height / 2.0;

                    pstAlgRslt->stAlgChnRslt[j].stProcOut.target_num = ++u32Idx;
                }
            }
        }
    }

    s32Ret = SAL_SOK;

exit:
    return s32Ret;
}

/**
 * @function   Sva_DrvUpdatePkgMatchPair
 * @brief      更新包裹匹配对信息(包含删除、新增等操作)
 * @param[in]  DUAL_PKG_MATCH_RESULT_S *pstMatchRslt
 * @param[in]  SVA_ALG_RSLT_S *pstAlgRslt
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvUpdatePkgMatchPair(const DUAL_PKG_MATCH_RESULT_S *pstMatchRslt, SVA_ALG_RSLT_S *pstAlgRslt)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i, j, k, u32PkgId;
    BOOL bExist = SAL_TRUE;  /* 默认包裹仍然存在 */

    SVA_PKG_MATCH_PAIR_INFO_S *pstPkgMatchPair = NULL;
    SVA_SYNC_RESULT_S *pstRslt = NULL;

    /* 非双路关联模式，无需更新双视角包裹匹配对，直接返回成功 */
    if (SVA_PROC_MODE_DUAL_CORRELATION != pstAlgRslt->stAlgChnRslt[0].enProcMode)
    {
        return SAL_SOK;
    }

    /* 删除已经消失的目标匹配对，满足至少其中一个视角的包裹已经消失 */
    for (i = 0; i < SVA_MAX_PKG_MATCH_PAIR_NUM; i++)
    {
        pstPkgMatchPair = &g_sva_common.astPkgMatchPairInfo[i];
        if (SAL_TRUE != pstPkgMatchPair->bBusy)
        {
            continue;
        }

        /* 每一个包裹匹配对缓存默认都存在于画面中 */
        bExist = SAL_TRUE;

        for (j = 0; j < pstAlgRslt->uiChnCnt; j++)
        {
            u32PkgId = pstPkgMatchPair->au32PkgMatchPairId[j];
            pstRslt = &pstAlgRslt->stAlgChnRslt[j];

            for (k = 0; k < pstRslt->stProcOut.packbagAlert.OriginalPackageNum; k++)
            {
                if (u32PkgId == pstRslt->stProcOut.packbagAlert.OriginalPackageLoc[k].PakageID)
                {
                    break;
                }
            }

            if (k >= pstRslt->stProcOut.packbagAlert.OriginalPackageNum)
            {
                bExist = SAL_FALSE;
            }
        }

        if (SAL_TRUE != bExist)
        {
            pstPkgMatchPair->bBusy = SAL_FALSE;
        }
    }

    /* 插入新的包裹匹配对信息 */
    if (pstMatchRslt->bHasMatch)
    {
        for (i = 0; i < SVA_MAX_PKG_MATCH_PAIR_NUM; i++)
        {
            if (SAL_TRUE != g_sva_common.astPkgMatchPairInfo[i].bBusy)
            {
                break;
            }
        }

        if (i >= SVA_MAX_PKG_MATCH_PAIR_NUM)
        {
            SVA_LOGW("pkg match pair buf full! \n");
            goto exit;
        }

        pstPkgMatchPair = &g_sva_common.astPkgMatchPairInfo[i];

        pstPkgMatchPair->bBusy = SAL_TRUE;
        pstPkgMatchPair->au32PkgMatchPairId[0] = pstMatchRslt->auiMatchId[0];
        pstPkgMatchPair->au32PkgMatchPairId[1] = pstMatchRslt->auiMatchId[1];
    }

    s32Ret = SAL_SOK;

exit:
    return s32Ret;
}

/**
 * @function    Sva_DrvPostProcess
 * @brief       数据后处理
 * @param[in]   SVA_ALG_RSLT_S *pstAlgRslt
 * @param[out]  none
 * @return
 */
static INT32 Sva_DrvPostProcess(SVA_ALG_RSLT_S *pstAlgRslt)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i;
    DUAL_PKG_MATCH_RESULT_S stMatchRslt = {0};

    for (i = 0; i < pstAlgRslt->uiChnCnt; i++)
    {
        s32Ret = Sva_DrvDoPostProcOnce(&pstAlgRslt->stAlgChnRslt[i]);
        SVA_DRV_CHECK_RET_NO_LOOP(s32Ret, "Sva_DrvImgProcess failed!"); \

#ifdef DSP_ISM  /* szl_todo: 安检机处理逻辑待整理 */
        /* 安检机场景进行智能data数据发送 */
        s32Ret = Sva_DrvISMDealResQueueOut(pstAlgRslt->stAlgChnRslt[i].chan, &pstAlgRslt->stAlgChnRslt[i].stProcOut);
        SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvImgProcess failed!");
        return s32Ret;
#endif
    }

    /* 双视角包裹匹配处理，所有依赖双视角关联结果的处理都要放在这一步之后 */
    s32Ret = Sva_DrvDualPkgMatch(pstAlgRslt, &stMatchRslt);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvImgProcess failed!");

    /* 双视角包裹关联结果后处理1，主要实现将匹配成功的包裹信息更新到packbagAlert中 */
    s32Ret = Sva_DrvUpdatePkgMatchRslt(&stMatchRslt, pstAlgRslt);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvPkgMatchPostProc failed!");

    /* 双视角包裹关联结果后处理2，更新全局包裹匹配对，包括删除已经消失的匹配对目标 */
    s32Ret = Sva_DrvUpdatePkgMatchPair(&stMatchRslt, pstAlgRslt);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvPkgMatchPostProc failed!");

    /* 双视角包裹关联结果后处理3，双视角目标合并展示 */
    s32Ret = Sva_DrvMergeDualViewTarget(&stMatchRslt, pstAlgRslt);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvMergeDualViewTarget failed!");

    for (i = 0; i < pstAlgRslt->uiChnCnt; i++)
    {
        /* 智能结果更新到外部模块(显示、封装) */
        s32Ret = Sva_DrvSyncRsltToOuterModule(&pstAlgRslt->stAlgChnRslt[i]);
        SVA_DRV_CHECK_RET_NO_LOOP(s32Ret, "Sva_DrvSyncRsltToOuterModule failed!");

        /* 包裹后处理 */
        s32Ret = Sva_DrvPkgPostProc(&pstAlgRslt->stAlgChnRslt[i]);
        SVA_DRV_CHECK_RET_NO_LOOP(s32Ret, "Sva_DrvPkgPostProc failed!");

        /* 送编图处理 */
        s32Ret = Sva_DrvSendImgProcOnce(&pstAlgRslt->stAlgChnRslt[i]);
        SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvSendImgProcOnce failed!");
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvVcaePostProcThread
* 描  述  : 后处理线程(双芯片主片使用)
* 输  入  : - prm: 全局变量参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
void *Sva_DrvVcaePostProcThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    UINT32 time[5] = {0};

    SVA_DEV_PRM *pstSva_dev = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    SVA_SYNC_RESULT_S *pstAlgRslt = NULL;

    PhysAddr u64PhyAddr = 0;

    SVA_DRV_CHECK_PTR(prm, err, "prm == null!");

    pstSva_dev = (SVA_DEV_PRM *)prm;
    pstSva_dev->stPostProcQue.uiQueLen = SVA_MAX_POST_PROC_QUE_LEN;

    /* 1. 创建管理满队列 */
    pstFullQue = &pstSva_dev->stPostProcQue.stFullQue;

    s32Ret = DSA_QueCreate(pstFullQue, SVA_MAX_POST_PROC_QUE_LEN);
    SVA_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "DSA_QueCreate failed!");

    /* 2. 创建管理空队列 */
    pstEmptQue = &pstSva_dev->stPostProcQue.stEmptQue;

    s32Ret = DSA_QueCreate(pstEmptQue, SVA_MAX_POST_PROC_QUE_LEN);
    SVA_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "DSA_QueCreate failed!");

    /* 3. 缓存区放入空队列 */
    for (i = 0; i < SVA_MAX_POST_PROC_QUE_LEN; i++)
    {
        pstAlgRslt = pstSva_dev->stPostProcQue.pstAlgOutRslt[i];
        s32Ret = mem_hal_mmzAlloc(sizeof(SVA_SYNC_RESULT_S), "sva", "post_proc_buf", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&pstAlgRslt);
        SVA_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "mem_hal_mmzAlloc failed!");

        s32Ret = DSA_QuePut(pstEmptQue, (void *)pstAlgRslt, SAL_TIMEOUT_NONE);
        SVA_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "DSA_QuePut failed!");
    }

    SAL_SET_THR_NAME();

    while (1)
    {
        time[0] = SAL_getCurMs();

        /* 获取队列数据 */
        DSA_QueGet(pstFullQue, (VOID **)&pstAlgRslt, SAL_TIMEOUT_FOREVER);

        time[1] = SAL_getCurMs();

        /* 智能更新到外部模块 */
        s32Ret = Sva_DrvSyncRsltToOuterModule(pstAlgRslt);
        SVA_DRV_CHECK_RET(s32Ret, reuse, "Sva_DrvSyncRsltToOuterModule failed!");

        /* 误检包裹后处理。应用层需要获取误检包裹id，用于及时更新集中判图客户端残留的误检包裹分片 */
        Sva_DrvFalseDetPkgCbProc(pstAlgRslt);

        /* 编图后处理 */
        s32Ret = Sva_DrvSendImgProcOnce(pstAlgRslt);
        SVA_DRV_CHECK_RET(s32Ret, reuse, "Sva_DrvSendImgProcOnce failed!");

        time[2] = SAL_getCurMs();
reuse:
        /* 队列释放 */
        DSA_QuePut(pstEmptQue, (VOID *)pstAlgRslt, SAL_TIMEOUT_NONE);

        time[3] = SAL_getCurMs();

        if (time[3] - time[1] > 100)
        {
            SVA_LOGW("post proc cost %d ms > 100 ms! %d, %d, %d \n",
                     time[3] - time[1],
                     time[1] - time[0],
                     time[2] - time[1],
                     time[3] - time[2]);
        }
    }

err:
    SVA_LOGW("%s: exit! \n", __FUNCTION__);
    return NULL;
}

static UINT32 uiGetQueFlag = 0;   /* dbg, check if get empty queue stuck! */

/**
 * @function    Sva_DrvGetQueDbgInfo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_DrvGetQueDbgInfo(void)
{
    UINT32 i = 0;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_LOGE("dbg: stuck flag %d \n", uiGetQueFlag);

    for (i = 0; i < 2; i++)
    {
        pstSva_dev = Sva_DrvGetDev(i);

        SVA_LOGE("dbg: chan %d, empty %d, full %d \n",
                 i, DSA_QueGetQueuedCount(&pstSva_dev->stPostProcQue.stEmptQue),
                 DSA_QueGetQueuedCount(&pstSva_dev->stPostProcQue.stFullQue));
    }

    return;
}

/**
 * @function    Sva_DrvCpyInputData
 * @brief         拷贝数据帧,将xpack接收的数据贴在固定分辨率上
 * @param[in]  SYSTEM_FRAME_INFO *pstSrcFrame-传入包裹帧数据
 * @param[in]  SYSTEM_FRAME_INFO *pstDstFrame-待贴包裹帧数据
 * @param[out] NULL
 * @return
 */
static INT32 Sva_DrvCpyInputData(INT32 chan, SYSTEM_FRAME_INFO *pstSrcFrame, SYSTEM_FRAME_INFO *pstDstFrame)
{
    INT32 s32Ret = SAL_SOK;
    SAL_VideoFrameBuf stVideoFrameBuf = {0};

    SVA_DRV_CHECK_PRM(pstSrcFrame, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstDstFrame, SAL_FAIL);

    TDE_HAL_SURFACE stSrcSurface = {0};
    TDE_HAL_SURFACE stDstSurface = {0};
    TDE_HAL_RECT stSrcRect = {0};
    TDE_HAL_RECT stDstRect = {0};

    if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSrcFrame, &stVideoFrameBuf))
    {
        SVA_LOGE("get video frame info failed! \n");
        return SAL_FAIL;
    }

    stSrcSurface.enColorFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    stSrcSurface.PhyAddr = stVideoFrameBuf.phyAddr[0];
    stSrcSurface.u32Width = stVideoFrameBuf.frameParam.width;
    stSrcSurface.u32Height = stVideoFrameBuf.frameParam.height;
    stSrcSurface.u32Stride = stVideoFrameBuf.frameParam.width; /* stVideoFrameBuf.stride[0]; */
    stSrcSurface.vbBlk = stVideoFrameBuf.vbBlk;

    stSrcRect.s32Xpos = 0;
    stSrcRect.s32Ypos = 0;
    stSrcRect.u32Width = stVideoFrameBuf.frameParam.width;
    stSrcRect.u32Height = stVideoFrameBuf.frameParam.height;

    if (SAL_SOK != sys_hal_getVideoFrameInfo(pstDstFrame, &stVideoFrameBuf))
    {
        SVA_LOGE("get video frame info failed! \n");
        return SAL_FAIL;
    }

    stDstSurface.enColorFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
    stDstSurface.PhyAddr = stVideoFrameBuf.phyAddr[0];
    stDstSurface.u32Width = stVideoFrameBuf.frameParam.width;
    stDstSurface.u32Height = stVideoFrameBuf.frameParam.height;
    stDstSurface.u32Stride = stVideoFrameBuf.frameParam.width;
    stDstSurface.vbBlk = stVideoFrameBuf.vbBlk;

    stDstRect.s32Xpos = (UINT32)pstSrcFrame->uiDataAddr;
    stDstRect.s32Ypos = 0;
    stDstRect.u32Width = SVA_MODULE_WIDTH;
    stDstRect.u32Height = SVA_MODULE_HEIGHT;

    /*cpy之前进行clear操作*/
    memset((VOID *)stVideoFrameBuf.virAddr[0], 0x00, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2);

    s32Ret = tde_hal_QuickCopy(&stSrcSurface, &stSrcRect, &stDstSurface, &stDstRect, SAL_FALSE);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("tde quick copy failed! \n");
        return SAL_FAIL;
    }

#if 0
    XSI_DEV *pstXsi = NULL;
    SVA_FRAME_MEM *pstDevTmpFrameMem = NULL;

    INT32 uBufSize = SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2;
    pstXsi = Sva_HalGetDev(chan);
    SVA_DRV_CHECK_PRM(pstXsi, SAL_FAIL);

    VIDEO_FRAME_INFO_S *pstSrcFrameInfo = NULL;
    VIDEO_FRAME_INFO_S *pstDstFrameInfo = NULL;
    pstSrcFrameInfo = (VIDEO_FRAME_INFO_S *)pstSrcFrame->uiAppData;
    SVA_DRV_CHECK_PRM(pstSrcFrameInfo, SAL_FAIL);
    pstDstFrameInfo = (VIDEO_FRAME_INFO_S *)pstDstFrame->uiAppData;
    SVA_DRV_CHECK_PRM(pstDstFrameInfo, SAL_FAIL);

    HAL_BUFFTRANS_POS_INFO srcPos = {0}; /*源数据抠图区域*/
    HAL_BUFFTRANS_POS_INFO dstPos = {0}; /*目标数据cpy区域*/

    srcPos.uiX = 0;
    srcPos.uiY = 0;
    srcPos.uiWidth = pstSrcFrameInfo->stVFrame.u32Width;
    srcPos.uiHeight = pstSrcFrameInfo->stVFrame.u32Height;
    dstPos.uiX = (UINT32)pstSrcFrame->uiDataAddr;
    dstPos.uiY = 0;
    dstPos.uiWidth = SVA_MODULE_WIDTH;
    dstPos.uiHeight = SVA_MODULE_HEIGHT;
    pstDstFrameInfo->stVFrame.u32Width = SVA_MODULE_WIDTH;
    pstDstFrameInfo->stVFrame.u32Height = SVA_MODULE_HEIGHT;
    pstDstFrameInfo->stVFrame.u32Stride[0] = SVA_MODULE_WIDTH;
    pstDstFrameInfo->stVFrame.u32Stride[1] = SVA_MODULE_WIDTH;
    pstDstFrameInfo->stVFrame.u32Stride[2] = SVA_MODULE_WIDTH;
    pstDstFrameInfo->stVFrame.u64PhyAddr[0] = HI_ADDR_P2U64(pstDevTmpFrameMem->pPhyMemory);
    pstDstFrameInfo->stVFrame.u64PhyAddr[1] = pstDstFrameInfo->stVFrame.u64PhyAddr[0] + SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT;
    SVA_LOGI("Cpy yuv data !!! pkg_wid %d pkg_hei %d offset %d\n", srcPos.uiWidth, srcPos.uiHeight, dstPos.uiX);

    /*cpy之前进行clear操作*/
    memset((char *)pstDevTmpFrameMem->pVirMemory, 0x00, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2);
    s32Ret = HAL_bufQuickCopyYuv(pstSrcFrame, pstDstFrame, &srcPos, &dstPos);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Cpy Data Err!!!\n");
    }

#endif

    return s32Ret;
}

/*******************************************************************************
* 函数名  : Sva_DrvVcaeResultThread
* 描  述  : 接收检测结果线程(双芯片主片使用)
* 输  入  : - prm: 全局变量参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
void *Sva_DrvVcaeResultThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 time0 = 0;
    UINT32 time1 = 0;

    UINT32 i = 0;

    UINT64 u64ResultLen = sizeof(SVA_ALG_RSLT_S);
    UINT64 u64PhyAddr = 0;

    SVA_ALG_RSLT_S *pstSyncRslt = NULL;
    SVA_SYNC_RESULT_S *pstAlgRslt = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_PTR(prm, exit, "prm == null!");

    s32Ret = mem_hal_mmzAlloc(u64ResultLen, "SVA", "sva_slv_rslt", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&pstSyncRslt);
    SVA_DRV_CHECK_RET(SAL_SOK != s32Ret, exit, "mem_hal_mmzAlloc failed!");

    sal_memset_s(pstSyncRslt, u64ResultLen, 0x00, u64ResultLen);

#ifdef XTRANS_DUAL_DMA
    PhyRecvRsltBuf = u64PhyAddr;
#else
    g_pTmpRsltBuf = (VOID *)pstSyncRslt;
#endif

    SAL_SET_THR_NAME();

    while (1)
    {
#if 0
#ifndef PCIE_M2S_DEBUG
        if ((0 == g_sva_common.uiChnCnt)
            || (SVA_PROC_MODE_IMAGE == g_proc_mode)
            || (SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode && g_sva_common.uiChnCnt < 2)
            || (SAL_TRUE != Sva_DrvGetInitFlag()))
#endif
        {
            usleep(100 * 1000);
            continue;             /*未初始化等情况进行待，同步R7*/
        }

#endif
        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time0);

        g_sva_common.stDbgPrm.stCommSts.uiRecvIaRsltFlag = 1;   /* 送帧之前标记 */

        g_sva_common.stDbgPrm.stCommSts.uiRecvIaRsltNum++;

#ifdef XTRANS_DUAL_DMA

        bPhyRecvRsltReady = SAL_TRUE;      /* 标记接收结果的物理地址已经ready */
        s32Ret = Sva_DrvRecvRsltByDualDma();

#else

        s32Ret = Sva_DrvRecvRsltByMMap();

#endif

        g_sva_common.stDbgPrm.stCommSts.uiRecvIaRsltFlag = 0;   /* 送帧之后标记 */

        if (SAL_SOK != s32Ret)
        {
            g_sva_common.stDbgPrm.stCommSts.uiRecvIaRsltFailNum++;
            SVA_LOGE("dual-chip set data status failed! \n");
            continue;
        }
        else
        {
            g_sva_common.stDbgPrm.stCommSts.uiRecvIaRsltSuccNum++;
        }

        (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time1);
        (VOID)Sva_DrvTransDbgAddTime(SVA_TRANS_DBG_MASTER_RESULT_TYPE, SVA_TRANS_CONV_IDX(MST_RSLT_TOTAL_TYPE), time0, time1);

        /* 调试接口: 主片接收智能结果的统计耗时 */
        (VOID)Sva_DrvMasterRsltCalDbgTime();

        if (Sva_HalGetDbgLevel() >= DEBUG_LEVEL4)
        {
            SZL_DBG2_LOG("mastet recv rslt: %d ms. \n", time1 - time0);
        }

#if 0   /* 调试打印信息，用于问题定位保留 */
        SVA_LOGW("uiChnIdx %d \n", pstSyncRslt->uiChnCnt);
        for (i = 0; i < pstSyncRslt->uiChnCnt; i++)
        {
            SVA_LOGW("i %d, chan %d, frm %d, tar %d, offset %f, [%f, %f], [%f, %f] \n",
                     i, pstSyncRslt->stAlgChnRslt[i].chan, pstSyncRslt->stAlgChnRslt[i].stProcOut.frame_num,
                     pstSyncRslt->stAlgChnRslt[i].stProcOut.target_num, pstSyncRslt->stAlgChnRslt[i].stProcOut.frame_offset,
                     pstSyncRslt->stAlgChnRslt[i].stProcOut.packbagAlert.package_loc.x,
                     pstSyncRslt->stAlgChnRslt[i].stProcOut.packbagAlert.package_loc.y,
                     pstSyncRslt->stAlgChnRslt[i].stProcOut.packbagAlert.package_loc.width,
                     pstSyncRslt->stAlgChnRslt[i].stProcOut.packbagAlert.package_loc.height);
        }

#endif

        uiGetQueFlag = 1;

        /* 将接收到从片发送过来的智能结果拷贝到后级后处理线程中 */
        for (i = 0; i < pstSyncRslt->uiChnCnt; i++)
        {
            pstSva_dev = Sva_DrvGetDev(pstSyncRslt->stAlgChnRslt[i].chan);
            SVA_DRV_CHECK_PTR(pstSva_dev, reuse, "pstSva_dev == null!");

            DSA_QueGet(&pstSva_dev->stPostProcQue.stEmptQue, (VOID **)&pstAlgRslt, SAL_TIMEOUT_FOREVER);
            sal_memcpy_s(pstAlgRslt, sizeof(SVA_SYNC_RESULT_S), &pstSyncRslt->stAlgChnRslt[i], sizeof(SVA_SYNC_RESULT_S));
            DSA_QuePut(&pstSva_dev->stPostProcQue.stFullQue, (VOID *)pstAlgRslt, SAL_TIMEOUT_NONE);

            SVA_LOGD("recv: i %d, chan %d \n", i, pstSyncRslt->stAlgChnRslt[i].chan);

reuse:
            continue;
        }

        uiGetQueFlag = 0;

        continue;
    }

exit:
    if (NULL != pstSyncRslt)
    {
        (VOID)mem_hal_mmzFree(pstSyncRslt, "SVA", "sva_slv_rslt");
        pstSyncRslt = NULL;
    }

    return NULL;
}

/**
 * @function:   Sva_DrvFillMatchBufFrmNum
 * @brief:      填充帧序号（帧匹配使用）
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 uiFrameNum
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvFillMatchBufFrmNum(UINT32 chan, UINT32 uiFrameNum)
{
    UINT32 uiIdx = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_MATCH_BUF_DATA *pstMatchBufData = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    SVA_DRV_CHECK_PRM(pstSva_dev->pstMatchBufInfo, SAL_FAIL);

    uiIdx = pstSva_dev->pstMatchBufInfo->uiW_Idx;

    pstMatchBufData = &pstSva_dev->pstMatchBufInfo->stMatchBufData[uiIdx];
    pstMatchBufData->uiFrameNum = uiFrameNum;

    SVA_LOGD("chan %d, idx %d, save frmNum %d ! \n", chan, uiIdx, uiFrameNum);

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvAddMatchFrmWIdx
 * @brief:      匹配帧数据自加
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvAddMatchFrmWIdx(UINT32 chan)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    SVA_DRV_CHECK_PRM(pstSva_dev->pstMatchBufInfo, SAL_FAIL);

    pstSva_dev->pstMatchBufInfo->uiW_Idx = (pstSva_dev->pstMatchBufInfo->uiW_Idx + 1) % pstSva_dev->pstMatchBufInfo->uiCnt;

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvClearMatchBufData
 * @brief:      清空匹配帧缓存结构体，避免脏数据残留
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 uiIdx
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvClearMatchBufData(UINT32 chan, UINT32 uiIdx)
{
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_MATCH_BUF_DATA *pstMatchBufData = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    SVA_DRV_CHECK_PRM(pstSva_dev->pstMatchBufInfo, SAL_FAIL);

    pstMatchBufData = &pstSva_dev->pstMatchBufInfo->stMatchBufData[uiIdx];
    pstMatchBufData->uiFrameNum = 0;
    pstMatchBufData->bDataValid = 0;

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvPutMatchFrame
 * @brief:      释放匹配帧数据
 * @param[in]:  UINT32 chan
 * @param[in]:  SYSTEM_FRAME_INFO *pstFrameInfo
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvPutMatchFrame(UINT32 chan, SYSTEM_FRAME_INFO *pstFrameInfo)
{
    INT32 s32Ret = SAL_SOK;

    VOID *pDupChnHandle = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstFrameInfo, SAL_FAIL);

    pDupChnHandle = Sva_DrvGetDupHandle(chan);

    s32Ret = Sva_HalPutFrame(pDupChnHandle, pstFrameInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Put Match Frm Failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvResetMatchBuf
 * @brief:      清空匹配帧数据
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvResetMatchBuf(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_MATCH_BUF_INFO *pstMatchBufInfo = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (SAL_TRUE != pstSva_dev->uiInitStatus)
    {
        return SAL_SOK;
    }

    pstMatchBufInfo = pstSva_dev->pstMatchBufInfo;
    SVA_DRV_CHECK_PRM(pstMatchBufInfo, SAL_FAIL);

    (VOID)Sva_DrvLockMatchFrm(chan);

    for (i = 0; i < pstMatchBufInfo->uiCnt; i++)
    {
        if (SAL_TRUE != pstMatchBufInfo->stMatchBufData[i].bDataValid)
        {
            continue;
        }

        s32Ret = Sva_DrvPutMatchFrame(chan, &pstMatchBufInfo->stMatchBufData[i].stSysFrameInfo);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("chan %d, put match frame failed! i %d \n", chan, i);
        }

        (VOID)Sva_DrvClearMatchBufData(chan, pstMatchBufInfo->stMatchBufData[i].stSysFrameInfo.uiDataLen);
    }

    (VOID)Sva_DrvUnlockMatchFrm(chan);

    SAL_INFO("reset match buf frame end! chan %d \n", chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvGetMatchFrame
 * @brief:      获取匹配帧数据
 * @param[in]:  UINT32 chan
 * @param[in]:  SYSTEM_FRAME_INFO **ppFrameInfo
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvGetMatchFrame(UINT32 chan, SYSTEM_FRAME_INFO **ppFrameInfo)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 time0 = 0;
    UINT32 time1 = 0;

    UINT32 uiW_Idx = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SYSTEM_FRAME_INFO *pstSysFrmInfo = NULL;
    SVA_MATCH_BUF_INFO *pstMatchBufInfo = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstMatchBufInfo = pstSva_dev->pstMatchBufInfo;
    SVA_DRV_CHECK_PRM(pstMatchBufInfo, SAL_FAIL);

    time0 = SAL_getCurMs();

    (VOID)Sva_DrvLockMatchFrm(chan);

    time1 = SAL_getCurMs();

    if (time1 - time0 > 10)
    {
        SVA_LOGE("Get Lock cost %d ms. chan %d \n", time1 - time0, chan);
        (VOID)Sva_DrvUnlockMatchFrm(chan);
        return SAL_FAIL;
    }

    uiW_Idx = pstMatchBufInfo->uiW_Idx;
    pstSysFrmInfo = &pstMatchBufInfo->stMatchBufData[uiW_Idx].stSysFrameInfo;

    if (SAL_TRUE == pstMatchBufInfo->stMatchBufData[uiW_Idx].bDataValid)
    {
        (VOID)Sva_DrvPutMatchFrame(chan, pstSysFrmInfo);
        (VOID)Sva_DrvClearMatchBufData(chan, uiW_Idx);
    }

    s32Ret = Sva_HalGetFrame((DUP_ChanHandle *)pstSva_dev->DupHandle.pHandle, pstSysFrmInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Get Match Frm Failed! chan %d, uiW_Idx %d\n", chan, uiW_Idx);
        (VOID)Sva_DrvUnlockMatchFrm(chan);
        return SAL_FAIL;
    }

    /* 复用，用于清空特定索引的信息 */
    pstSysFrmInfo->uiDataLen = uiW_Idx;

    /* 标记数据有效 */
    pstMatchBufInfo->stMatchBufData[uiW_Idx].bDataValid = SAL_TRUE;

    *ppFrameInfo = pstSysFrmInfo;

    (VOID)Sva_DrvUnlockMatchFrm(chan);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvVcaeMasterSyncThread
* 描  述  : 检测线程(分析仪使用)
* 输  入  : - prm: 全局变量参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 分析仪使用，双路检测通道均使用此线程，不区分模式(关联or非关联)
*******************************************************************************/
void *Sva_DrvVcaeMasterSyncThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiTmp1 = 0;
    UINT32 uiTmp2 = 0;
    UINT32 uiTmp3 = 0;
    UINT32 uiTmp4 = 0;
    UINT32 uiTmp5 = 0;
    UINT32 uiTmpFlag = 0;
    UINT32 uiMainViewChn = 0;
    UINT32 uiTmpChn = 0;
    SVA_DEV_PRM *pstSva_dev = NULL;
    UINT32 i = 0;
    UINT32 noVideoflag[2] = {0};
    UINT32 startFlag = 0;
    UINT32 time0 = 0, time1 = 0;
    INT64L framePTS[2] = {0, 0};
    INT64L oldFramePTS[2] = {0, 0};
    UINT32 chnStatus[2] = {0, 0};             /* 通道创建状态 */
    UINT32 needSync = 0;
    UINT32 syncFlg[2] = {0, 0};
    UINT32 noVideo[2] = {0, 0};
    UINT32 userPic[2] = {0, 0};
    UINT32 checkCnt[2] = {0, 0};
    UINT32 srcFrame[2] = {60, 60};
    UINT32 viChage[2] = {1, 1};
    INT64L threshold[2] = {17, 17};
    SYSTEM_FRAME_INFO *pstSysFrameInfo[2] = {NULL, NULL};
    UINT64 u64Pts = 0;
    INT64L defPts[2] = {0};
    UINT32 uiProcGap = 0;
    SVA_PROC_MODE_E enProcMode = 0;
    UINT64 u64PhyAddr = 0;

    UINT64 u64FrameExtSize = sizeof(SAL_VideoFrameBuf);
    UINT32 cnt = 0;
    INT8 path[64] = {0};

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    UINT32 uiObdCnt = 0;
    UINT32 uiQueueCnt = 0;
    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    UINT64 u64FrameSize = SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2;
    void *vir[2] = {NULL, NULL};
    UINT32 time3 = 0;
    UINT32 time5 = 0;
    UINT32 time8 = 0;

    UINT32 time_frm_0 = 0;
    UINT32 time_prm_0 = 0;
    UINT32 time000 = 0;
    UINT32 time111 = 0;
    INT8 *pBufWr = NULL;
    UINT32 fps = 0;
    SVA_PROC_INTER_PRM *pstProcInterPrm = NULL;

    SVA_DRV_CHECK_PRM(prm, NULL);

    pstSva_dev = (SVA_DEV_PRM *)prm;
    pstProcInterPrm = &pstSva_dev->stProcInterPrm;

    for (i = 0; i < SVA_DEV_MAX; i++)
    {
        if (NULL == pstSysFrameInfo[i])
        {
            s32Ret = mem_hal_mmzAlloc(sizeof(SYSTEM_FRAME_INFO), "SVA", "sva_vf_st", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&pstSysFrameInfo[i]);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("mem_hal_mmzAlloc failed! i %d \n", i);
                goto err1;
            }

            s32Ret = sys_hal_allocVideoFrameInfoSt(pstSysFrameInfo[i]);
            if (s32Ret != SAL_SOK)
            {
                SVA_LOGE("sys_hal_allocVideoFrameInfoSt error !\n");
                goto err2;
            }
        }
    }

    s32Ret = mem_hal_mmzAlloc(u64FrameExtSize, "SVA", "sva_mst_buf", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&pBufWr);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("mem_hal_mmzAlloc failed! \n");
        goto err3;
    }

    SAL_SET_THR_NAME();
    SAL_SetThreadCoreBind(((pstSva_dev->chan + 1) % 2));
    while (1)
    {
        /* TODO: 锁实现，睡眠存在问题--时间长度是否合适+轮询对CPU的浪费 */
        /* pthread_mutex_lock(&MutexHandle->lock); */
        #if 0   /* 加锁版本，应用下发双通道同步信号 */
        pthread_mutex_lock(&MutexHandle->lock);
        while (2 != g_sva_common.uiChnCnt)
        {
            pthread_cond_wait(&MutexHandle->cond, &MutexHandle->lock);
        }

        pthread_mutex_unlock(&MutexHandle->lock);
        #else /* 双通道默认同步版本 */
        if ((0 == g_sva_common.uiChnCnt)
            || (SVA_PROC_MODE_IMAGE == g_proc_mode)
            || (SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode
                && (SAL_TRUE != g_sva_common.stSva_dev[0].uiChnStatus || SAL_TRUE != g_sva_common.stSva_dev[1].uiChnStatus))
            || (SVA_PROC_MODE_VIDEO == g_proc_mode
                && (SAL_TRUE != g_sva_common.stSva_dev[0].uiChnStatus && SAL_TRUE != g_sva_common.stSva_dev[1].uiChnStatus))
            || (SAL_TRUE != Sva_DrvGetInitFlag()))
        {
            oldFramePTS[0] = 0;
            oldFramePTS[1] = 0;

            for (i = 0; i < SVA_DEV_MAX; i++)
            {
                (VOID)Sva_DrvResetMatchBuf(i);
            }

            if (pstProcInterPrm->uiSyncFlag || !pstProcInterPrm->uiProcFlag)
            {
                pstProcInterPrm->uiSyncFlag = SAL_FALSE;
                SVA_LOGW("SAL_mutexWait, mode %d, cnt %d\n", g_proc_mode, g_sva_common.uiChnCnt);
                SAL_mutexWait(pstProcInterPrm->mChnMutexHd1);
                SVA_LOGW("SAL_mutexSignal, mode %d, cnt %d\n", g_proc_mode, g_sva_common.uiChnCnt);
                pstProcInterPrm->uiSyncFlag = SAL_FALSE;
            }

            continue;
        }

        #endif

        enProcMode = g_proc_mode;

        if (bAlgResChg) /* (enProcMode != g_proc_mode) */
        {
            uiObdCnt = 0;
            bAlgResChg = SAL_FALSE;
        }

        time0 = SAL_getCurMs();

        (VOID)Sva_DrvGetFps(&time000, &time111, &fps);

        if (g_sva_common.stSva_dev[0].uiChnMirrorSync)
        {
            g_sva_common.stSva_dev[0].uiChnMirrorSync = 0;

            (VOID)Sva_drvChnWait(&g_sva_common.stSva_dev[0]);
        }

        /* 通道0的对应采集通道有视频输入且开启则获取数据 */
        noVideo[0] = 1; /* capt_func_chipCheckStatus(0); */
        chnStatus[0] = g_sva_common.stSva_dev[0].uiChnStatus;
        if (SAL_TRUE == chnStatus[0] && noVideo[0])    /* TODO: refacturing this code!!!! */
        {
            s32Ret = Sva_DrvGetMatchFrame(0, &pstSysFrameInfo[0]);
            if (SAL_SOK != s32Ret)
            {
                usleep(10 * 1000);
                SVA_LOGE("%s \n", __func__);
                continue;
            }

            if (SAL_SOK != Sva_HalGetFramePTS(&u64Pts, pstSysFrameInfo[0]))
            {
                u64Pts = 1000;
                SVA_LOGW("sva get pts error\n");
            }

            framePTS[0] = u64Pts / 1000;
        }

        if (g_sva_common.stSva_dev[1].uiChnMirrorSync)
        {
            g_sva_common.stSva_dev[1].uiChnMirrorSync = 0;

            (VOID)Sva_drvChnWait(&g_sva_common.stSva_dev[1]);
        }

        /* 通道1的对应采集通道有视频输入且开启则获取数据 */
        noVideo[1] = 1; /* capt_func_chipCheckStatus(1); */
        chnStatus[1] = g_sva_common.stSva_dev[1].uiChnStatus;
        if (SAL_TRUE == chnStatus[1] && noVideo[1])
        {
            s32Ret = Sva_DrvGetMatchFrame(1, &pstSysFrameInfo[1]);
            if (SAL_SOK != s32Ret)
            {
                if (SAL_TRUE == chnStatus[0] && noVideo[0])
                {
                    (VOID)Sva_DrvPutMatchFrame(0, pstSysFrameInfo[0]);
                    (VOID)Sva_DrvClearMatchBufData(0, pstSysFrameInfo[0]->uiDataLen);
                }

                SVA_LOGE("%s \n", __func__);

                usleep(10 * 1000);
                continue;
            }

            if (SAL_SOK != Sva_HalGetFramePTS(&u64Pts, pstSysFrameInfo[1]))
            {
                u64Pts = 1000;
                SVA_LOGW("sva get pts error\n");
            }

            framePTS[1] = u64Pts / 1000;

        }

        /* 两个通道同时开启且有视频输入做同步控制 */
        if (g_sva_common.uiChnCnt >= 2)
        {
            /* if (noVideo[1] && noVideo[0]) */
            if (chnStatus[0] && chnStatus[1] && (0 == capt_hal_getCaptUserPicStatue(0)) && (0 == capt_hal_getCaptUserPicStatue(1)))
            {
                needSync = 1;
            }
            else
            {
                needSync = 0;
                syncFlg[0] = 0;
                syncFlg[1] = 0;
            }
        }
        else
        {
            needSync = 0;
            syncFlg[0] = 0;
            syncFlg[1] = 0;
        }

        /* 通道0上电第一次和采集变化后重新获取采集的帧率 */
        if (1 == viChage[0])
        {
            srcFrame[0] = capt_func_chipGetViSrcRate(0);

            threshold[0] = 1000 / srcFrame[0] + 2;
            if (2 >= threshold[0])
            {
                threshold[0] = 17;
            }
        }

        /* 通道1上电第一次和采集变化后重新获取采集的帧率 */
        if (1 == viChage[1])
        {
            srcFrame[1] = capt_func_chipGetViSrcRate(1);

            threshold[1] = 1000 / srcFrame[1] + 2;
            if (2 >= threshold[1])
            {
                threshold[1] = 17;
            }
        }

        viChage[0] = 0;
        viChage[1] = 0;

        if (framePTS[0] < oldFramePTS[0])
        {
            oldFramePTS[0] = 0;
        }

        if (framePTS[1] < oldFramePTS[1])
        {
            oldFramePTS[1] = 0;
        }

        /* 做同步控制加抽帧 */
        if (needSync)
        {
            checkCnt[0] = 0;
            checkCnt[1] = 0;
check:
            /* SAL_INFO("sva  pts0 %llu pts1 %llu,old %llu,%llu\n", framePTS[0],framePTS[1],oldFramePTS[0], oldFramePTS[1]); */
            if ((framePTS[0] <= (framePTS[1] + SVA_SYNC_FRAME_GAP_THRESH * threshold[1]) && framePTS[0] >= (framePTS[1] - SVA_SYNC_FRAME_GAP_THRESH * threshold[1]))
                || (framePTS[1] <= (framePTS[0] + SVA_SYNC_FRAME_GAP_THRESH * threshold[0]) && framePTS[1] >= (framePTS[0] - SVA_SYNC_FRAME_GAP_THRESH * threshold[0])))
            {
                if (oldFramePTS[0] > 0 || oldFramePTS[1] > 0)
                {
                    if (!(framePTS[0] - oldFramePTS[0] > (THRESHOLD_0 - defPts[0] - threshold[0] + 5) && framePTS[0] - oldFramePTS[0] < (THRESHOLD_0 + defPts[0] + threshold[0]))
                        || !(framePTS[1] - oldFramePTS[1] > (THRESHOLD_1 - defPts[1] - threshold[1] + 5) && framePTS[1] - oldFramePTS[1] < (THRESHOLD_1 + defPts[1] + threshold[1])))
                    {
                        if (framePTS[0] - oldFramePTS[0] > (THRESHOLD_0 + threshold[0]) || framePTS[1] - oldFramePTS[1] > (THRESHOLD_1 + threshold[1]))
                        {
                            /* SAL_WARN("sva chn0 %d,defPts %d,chn1 %d,defPts %d,\n", framePTS[0]-oldFramePTS[0],defPts[0],framePTS[1]-oldFramePTS[1],defPts[1]); */
                            if (framePTS[0] - oldFramePTS[0] > (THRESHOLD_0 + threshold[0]))
                            {
                                defPts[0] = framePTS[0] - oldFramePTS[0] - THRESHOLD_0;
                                if (defPts[0] > THRESHOLD_0)
                                {
                                    defPts[0] = THRESHOLD_0;
                                }
                            }

                            if (framePTS[1] - oldFramePTS[1] > (THRESHOLD_1 + threshold[1]))
                            {
                                defPts[1] = framePTS[1] - oldFramePTS[1] - THRESHOLD_1;
                                if (defPts[1] > THRESHOLD_1)
                                {
                                    defPts[1] = THRESHOLD_1;
                                }
                            }
                        }
                        else
                        {
                            /* SAL_INFO("sva sync will skip\n"); */
                            defPts[0] = 0;
                            defPts[1] = 0;

                            uiTmp1 = (1 == uiTmpFlag) ? uiTmp1 + 1 : 0;
                            if (uiTmp1 > 10)
                            {
                                SVA_LOGW("cpu using warning 0! \n");
                                usleep(10 * 1000);
                                uiTmp1 = 0;
                            }

                            uiTmpFlag = 1;
                            goto putData;
                        }
                    }
                    else
                    {
                        defPts[0] = 0;
                        defPts[1] = 0;
                    }
                }
            }
            else
            {
                /* SAL_WARN("sva pts0 %llu pts1 %llu\n", framePTS[0],framePTS[1]); */
                /* 通道1时间戳比通道0时间戳小(时间差值在3帧内)，重新获取通道1新的数据 */
                if ((checkCnt[1] == 0) && ((framePTS[0] - 3 * threshold[0] > framePTS[1]) && (framePTS[0] - 5 * threshold[0] < framePTS[1])))
                {
                    (void)Sva_DrvPutMatchFrame(1, pstSysFrameInfo[1]);
                    (VOID)Sva_DrvClearMatchBufData(1, pstSysFrameInfo[1]->uiDataLen);

                    s32Ret = Sva_DrvGetMatchFrame(1, &pstSysFrameInfo[1]);
                    if (SAL_SOK != s32Ret)
                    {
                        if (SAL_TRUE == chnStatus[0] && noVideo[0])
                        {
                            (void)Sva_DrvPutMatchFrame(0, pstSysFrameInfo[0]);
                            (VOID)Sva_DrvClearMatchBufData(0, pstSysFrameInfo[0]->uiDataLen);
                        }

                        SVA_LOGE("%s \n", __func__);

                        usleep(10 * 1000);
                        continue;
                    }

                    if (SAL_SOK != Sva_HalGetFramePTS(&u64Pts, pstSysFrameInfo[1]))
                    {
                        u64Pts = 1000;
                        SVA_LOGW("sva get pts error\n");
                    }

                    framePTS[1] = u64Pts / 1000;
                    checkCnt[1] = 1;
                    /* SAL_INFO("sva 1 will get data again\n"); */

                    usleep(10 * 1000);
                    goto check;
                }
                /* 通道0时间戳比通道1时间戳小(时间差值在3帧内)，重新获取通道0新的数据 */
                else if ((checkCnt[0] == 0) && (framePTS[1] - 3 * threshold[1] > framePTS[0]) && (framePTS[1] - 5 * threshold[1] < framePTS[0]))
                {
                    (void)Sva_DrvPutMatchFrame(0, pstSysFrameInfo[0]);
                    (VOID)Sva_DrvClearMatchBufData(0, pstSysFrameInfo[0]->uiDataLen);

                    s32Ret = Sva_DrvGetMatchFrame(0, &pstSysFrameInfo[0]);
                    if (SAL_SOK != s32Ret)
                    {
                        if (SAL_TRUE == chnStatus[1] && noVideo[1])
                        {
                            (void)Sva_DrvPutMatchFrame(1, pstSysFrameInfo[1]);
                            (VOID)Sva_DrvClearMatchBufData(1, pstSysFrameInfo[1]->uiDataLen);
                        }

                        SVA_LOGE("%s \n", __func__);

                        usleep(10 * 1000);
                        continue;
                    }

                    if (SAL_SOK != Sva_HalGetFramePTS(&u64Pts, pstSysFrameInfo[0]))
                    {
                        u64Pts = 1000;
                        SVA_LOGW("sva get pts error\n");
                    }

                    framePTS[0] = u64Pts / 1000;
                    checkCnt[0] = 1;
                    /* SAL_INFO("sva 0 will get data again\n"); */

                    uiTmp2 = 2 == uiTmpFlag ? uiTmp2 + 1 : 0;
                    if (uiTmp2 > 10)
                    {
                        SVA_LOGW("cpu using warning 1! \n");
                        usleep(10 * 1000);
                        uiTmp2 = 0;
                    }

                    uiTmpFlag = 2;
                    goto check;
                }
                else /* 两通道时间戳差值较大丢掉 */
                {
                    /* SAL_WARN("sva will putdata pts %llu pts %llu check0 %d check1 %d threshold0 %d threshold1 %d\n", */
                    /*          framePTS[0],framePTS[1], checkCnt[0], checkCnt[1], threshold[0], threshold[1]); */
                    uiTmp3 = 3 == uiTmpFlag ? uiTmp3 + 1 : 0;
                    if (uiTmp3 > 10)
                    {
                        SVA_LOGW("cpu using warning 2! \n");
                        usleep(10 * 1000);
                        uiTmp3 = 0;
                    }

                    uiTmpFlag = 3;
                    goto putData;
                }
            }
        }
        else /* 不做同步控制只做抽帧 */
        {
            for (i = 0; i < 2; i++)
            {
                if ((1 != chnStatus[i]) || (1 != noVideo[i]))
                {
                    uiTmp4 = 4 == uiTmpFlag ? uiTmp4 + 1 : 0;
                    if (uiTmp4 > 10)
                    {
                        SVA_LOGW("cpu using warning 3! \n");
                        usleep(10 * 1000);
                        uiTmp4 = 0;
                    }

                    uiTmpFlag = 4;
                    continue;
                }

                if (oldFramePTS[i] > 0)
                {
                    if (!(framePTS[i] - oldFramePTS[i] > (THRESHOLD_0 - threshold[i] + 5) && framePTS[i] - oldFramePTS[i] < (THRESHOLD_0 + threshold[i])))
                    {
                        if (framePTS[i] - oldFramePTS[i] > (THRESHOLD_0 + threshold[i]))
                        {
                            SVA_LOGD("sva chn0 %d,chn1 %llu.[%llu,%llu]\n",
                                     i, framePTS[i] - oldFramePTS[i], framePTS[i], oldFramePTS[i]);
                            defPts[i] = time1 - time0 - THRESHOLD_0;
                            if (defPts[i] > THRESHOLD_0)
                            {
                                defPts[i] = THRESHOLD_0;
                            }

                            defPts[i] = defPts[1 - i];
                        }
                        else
                        {
                            /* SAL_INFO("sva will skip\n"); */
                            /* SAL_WARN("szl_dbg: goto exit 2  \n"); */
                            uiTmp5 = 5 == uiTmpFlag ? uiTmp5 + 1 : 0;
                            if (uiTmp5 > 10)
                            {
                                SVA_LOGW("cpu using warning 4! \n");
                                usleep(10 * 1000);
                                uiTmp5 = 0;
                            }

                            uiTmpFlag = 5;
                            goto putData;
                        }
                    }
                }
            }
        }

        uiTmp1 = 0;
        uiTmp2 = 0;
        uiTmp3 = 0;
        uiTmp4 = 0;
        uiTmp5 = 0;

        for (i = 0; i < SVA_DEV_MAX; i++)
        {
            pstSva_dev = Sva_DrvGetDev(i);
            if (NULL == pstSva_dev)
            {
                SVA_LOGE("pstSva_dev == null! chan %d \n", i);
                goto putData;
            }

            pstPrmFullQue = &pstSva_dev->stPrmFullQue;
            pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;
            uiQueueCnt = DSA_QueGetQueuedCount(pstPrmFullQue);
            if (uiQueueCnt)
            {
                /* 主片不需要使用相关参数，直接消耗掉 */
                DSA_QueGet(pstPrmFullQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
                DSA_QuePut(pstPrmEmptQue, (void *)pstIn, SAL_TIMEOUT_NONE);
            }

            if (SAL_TRUE != chnStatus[i])
            {
                continue;
            }

            (VOID)Sva_DrvFillMatchBufFrmNum(i, uiObdCnt);
            (VOID)Sva_DrvAddMatchFrmWIdx(i);
        }

        for (i = 0; i < SVA_DEV_MAX; i++)
        {

            /* 保存算法支持的分辨率 */
            if (chnStatus[i] && noVideo[i])
            {
                if (needSync)
                {
                    syncFlg[i] = 1;
                    if (!(1 == syncFlg[0]
                          && 1 == syncFlg[1]))
                    {
                        SVA_LOGW("continue %d,%d !\n", syncFlg[0], syncFlg[1]);
                        SAL_msleep(10);
                        startFlag = 1;
                        continue;
                    }

                    /* 无视频信号的时候不送算法 */
                    userPic[i] = 0; /* capt_hal_getCaptUserPicStatue(i); */
                    if (userPic[i])
                    {
                        noVideoflag[i] = 1;
                        SAL_msleep(20);
                        continue;
                    }

                    /* 上次处理是无视频信号两个通道同时返回，保证时间参数一致，以便做同步处理 */
                    if (noVideoflag[i])
                    {
                        noVideoflag[0] = 0;
                        noVideoflag[1] = 0;
                        framePTS[0] = 0;
                        framePTS[1] = 0;
                        oldFramePTS[0] = 0;
                        oldFramePTS[1] = 0;
                        /* getFrameNum[0] = 0; */
                        /* getFrameNum[1] = 0; */
                        viChage[0] = 1;
                        viChage[1] = 1;
                        i = 2;
                        SAL_msleep(10);
                        continue;
                    }
                }

                if (startFlag)
                {
                    startFlag = 0;
                    /* SAL_mutexUnlock(pstSva_dev1->mChnMutexHdl); */
                    goto putData;
                }

                oldFramePTS[i] = framePTS[i];
            }
        }

        time8 = SAL_getCurMs();
        (VOID)Sva_DrvTransDbgAddTime(SVA_TRANS_DBG_MASTER_YUV_TYPE, SVA_TRANS_CONV_IDX(MST_YUV_GET_CHN_FRAME_TYPE), time0, time8);

        uiMainViewChn = g_sva_common.uiMainViewChn;

        /* 主片发送帧数据到从片 */
        for (i = 0; i < SVA_DEV_MAX; i++)
        {
            uiTmpChn = (0 == i) ? uiMainViewChn : 1 - uiMainViewChn;
            if (SAL_TRUE != chnStatus[uiTmpChn])
            {
                continue;
            }

            if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSysFrameInfo[uiTmpChn], &stVideoFrmBuf))
            {
                SVA_LOGE("get video frame failed! \n");
                continue;
            }

            time_frm_0 = SAL_getCurMs();

            memset(pBufWr, 0x00, u64FrameExtSize);
            stVideoFrmBuf.privateDate = (PhysAddr)uiObdCnt;
            uiProcGap = Sva_HalGetViInputFrmRate();
            stVideoFrmBuf.reserved[0] = (UINT64)uiProcGap;    /* 保留位0作为VI输入帧率，当前该值仅作为集中判图使用 */
            sal_memcpy_s((INT8 *)pBufWr, u64FrameExtSize, &stVideoFrmBuf, u64FrameExtSize);

            (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time5);
            (VOID)Sva_DrvTransDbgAddTime(SVA_TRANS_DBG_MASTER_PARAM_TYPE, SVA_TRANS_CONV_IDX(MST_PRM_CPY_TO_TMP_BUF_TYPE), time_frm_0, time5);

            (VOID)Sva_DrvSetMsgData(pBufWr, &stbufDataInfo, u64FrameExtSize);

            g_sva_common.stDbgPrm.stCommSts.uiSendFrameFlag = 1;    /* 送帧之前标记 */

            g_sva_common.stDbgPrm.stCommSts.uiSendFrameNum++;
            s32Ret = Sva_DrvSendFrameByDma((PhysAddr)stVideoFrmBuf.phyAddr[0], u64FrameSize);
            if (SAL_SOK != s32Ret)
            {
                g_sva_common.stDbgPrm.stCommSts.uiSendFrameFailNum++;
            }
            else
            {
                g_sva_common.stDbgPrm.stCommSts.uiSendFrameSuccNum++;
            }

            /* dump yuv数据用于问题定位 */
            if ((SVA_PROC_MODE_DUAL_CORRELATION == enProcMode && 0 == i)
                || (SVA_PROC_MODE_VIDEO == enProcMode))
            {
                if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSysFrameInfo[uiTmpChn], &stVideoFrmBuf))
                {
                    SVA_LOGE("get video frame failed! \n");
                }
                else
                {
                    Sva_HalDumpRecvYuvData((VOID *)stVideoFrmBuf.virAddr[0],
                                           SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT,
                                           DUMP_SEND_YUV_PATH);
                }
            }

            g_sva_common.stDbgPrm.stCommSts.uiSendFrameFlag = 0;    /* 送帧之后标记 */

            if (SAL_SOK != s32Ret)
            {
                SAL_ERROR("send frame by dma failed! \n");
                goto putData;
            }

            (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time3);
            (VOID)Sva_DrvTransDbgAddTime(SVA_TRANS_DBG_MASTER_YUV_TYPE, SVA_TRANS_CONV_IDX(MST_YUV_TOTAL_TYPE), time5, time3);

            s32Ret = Sva_DrvSendFramePrmByMmap((VOID *)pBufWr, u64FrameExtSize);
            if (SAL_SOK != s32Ret)
            {
                SAL_ERROR("send frame param failed! \n");
                goto putData;
            }

            (VOID)Sva_DrvGetCurMs(DEBUG_LEVEL3, &time_prm_0);
            (VOID)Sva_DrvTransDbgAddTime(SVA_TRANS_DBG_MASTER_PARAM_TYPE, SVA_TRANS_CONV_IDX(MST_PRM_TOTAL_TYPE), time5, time_prm_0);

            if ((enProcMode == SVA_PROC_MODE_DUAL_CORRELATION)
                && (0 == i)
                && (1 != uiObdCnt % Sva_HalGetPdProcGap()))
            {
                SVA_LOGD("chan %d, obdCnt %d, break \n", i, uiObdCnt);
                break;
            }

            /* export src yuv data */
            if (uiSwitch[1])
            {
                SAL_clear(path);
                sprintf((CHAR *)path, "./%d_src_%d_%d_%dx%d.yuv", i, cnt, uiObdCnt, stVideoFrmBuf.frameParam.width, stVideoFrmBuf.frameParam.height);

                (VOID)Sva_HalDebugDumpData((CHAR *)vir[i], (CHAR *)path, u64FrameSize, 0);
            }
        }

        g_sva_common.stDbgPrm.stCommSts.uiInputCnt++;

        cnt++;
        fps++;

        /* 调试接口: 统计传输各环节耗时 */
        (VOID)Sva_DrvMasterThrCalDbgTime();

        uiObdCnt++;

        time1 = SAL_getCurMs();
        if ((1 == syncFlg[0]
             && 1 == syncFlg[1]))
        {
            if (time1 - time0 > (THRESHOLD_1 + 20))
            {
                if ((oldFramePTS[0] == framePTS[0]) || (oldFramePTS[1] == framePTS[1]))
                {
                    defPts[0] = time1 - time0 - THRESHOLD_1;
                    if (defPts[0] > THRESHOLD_1)
                    {
                        defPts[0] = THRESHOLD_1;
                    }

                    defPts[1] = defPts[0];
                    SVA_LOGD("chn %d, defTime %d !\n", i, time1 - time0);
                }
            }
        }

        continue;

putData:
        for (i = 0; i < SVA_DEV_MAX; i++)
        {
            if (SAL_TRUE != chnStatus[i] || SAL_TRUE != noVideo[i])
            {
                continue;
            }

            (VOID)Sva_DrvPutMatchFrame(i, pstSysFrameInfo[i]);
            (VOID)Sva_DrvClearMatchBufData(i, pstSysFrameInfo[i]->uiDataLen);
            pstSysFrameInfo[i] = NULL;
        }
    }

err3:
    if (pBufWr)
    {
        mem_hal_mmzFree(pBufWr, "SVA", "sva_mst_buf");
        pBufWr = NULL;
    }

err2:
    for (i = 0; i < SVA_DEV_MAX; i++)
    {
        (VOID)sys_hal_rleaseVideoFrameInfoSt(pstSysFrameInfo[i]);
    }

err1:
    for (i = 0; i < SVA_DEV_MAX; i++)
    {
        if (NULL == pstSysFrameInfo[i])
        {
            continue;
        }

        mem_hal_mmzFree(pstSysFrameInfo[i], "SVA", "sva_vf_st");
        pstSysFrameInfo[i] = NULL;
    }

    return NULL;
}

/*******************************************************************************
* 函数名  : Sva_DrvVcaeSyncThread
* 描  述  : 检测线程(分析仪使用)
* 输  入  : - prm: 全局变量参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 分析仪使用，双路检测通道均使用此线程，不区分模式(关联or非关联)
*******************************************************************************/
void *Sva_DrvVcaeSyncThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiQueueCnt = 0;
    UINT32 uiTmp1 = 0;
    UINT32 uiTmp2 = 0;
    UINT32 uiTmp3 = 0;
    UINT32 uiTmp4 = 0;
    UINT32 uiTmp5 = 0;
    UINT32 uiTmpFlag = 0;
    UINT32 uiInputErrCnt = 0;
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_DEV_PRM *pstSva_dev1 = NULL;
    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    UINT32 i = 0;
    SVA_PROCESS_IN *pastIn = NULL;   /* SVA_DEV_MAX  */
    UINT32 noVideoflag[2] = {0};
    UINT32 startFlag = 0;
    UINT32 time0 = 0, time1 = 0;
    INT64L framePTS[2] = {0, 0};
    INT64L oldFramePTS[2] = {0, 0};
    UINT32 chnStatus[2] = {0, 0};             /* 通道创建状态 */
    UINT32 xsiStatus[2] = {0, 0};             /* 通道创建状态 */
    UINT32 needSync = 0;
    UINT32 syncFlg[2] = {0, 0};
    UINT32 noVideo[2] = {0, 0};
    UINT32 userPic[2] = {0, 0};
    UINT32 checkCnt[2] = {0, 0};
    UINT32 srcFrame[2] = {60, 60};
    UINT32 viChage[2] = {1, 1};
    INT64L threshold[2] = {17, 17};
    SYSTEM_FRAME_INFO *pstSysFrameInfo[2] = {NULL, NULL};
    UINT64 u64Pts = 0;
    INT64L defPts[2] = {0};
    SVA_PROC_MODE_E enProcMode = 0;
    VOID *pDupChnHandle = NULL;

    UINT32 time000 = 0;
    UINT32 time111 = 0;
    UINT32 fps = 0;
    UINT32 d_time[16] = {0};  /* debug: 线程耗时调试使用 */
    UINT64 u64PhyAddr = 0;
    SVA_PROC_INTER_PRM *pstProcInterPrm = NULL;

    SVA_DRV_CHECK_PRM(prm, NULL);

    pstSva_dev = (SVA_DEV_PRM *)prm;
    pstProcInterPrm = &pstSva_dev->stProcInterPrm;

    pstSva_dev1 = Sva_DrvGetDev(0);

    for (i = 0; i < SVA_DEV_MAX; i++)
    {
        if (NULL == pstSysFrameInfo[i])
        {
            s32Ret = mem_hal_mmzAlloc(sizeof(SYSTEM_FRAME_INFO), "SVA", "sva_vf_st", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&pstSysFrameInfo[i]);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("mem_hal_mmzAlloc failed! i %d \n", i);
                return NULL;
            }

            s32Ret = sys_hal_allocVideoFrameInfoSt(pstSysFrameInfo[i]);
            if (s32Ret != SAL_SOK)
            {
                SVA_LOGE("Sva_HalGetVideoFrameSt error !\n");
                return NULL;
            }
        }
    }

    pastIn = SAL_memZalloc(SVA_DEV_MAX * sizeof(SVA_PROCESS_IN), "sva", "stIn_stk");
    if (NULL == pastIn)
    {
        SVA_LOGE("zalloc failed! \n");
        return NULL;
    }

    SAL_SET_THR_NAME();
    SAL_SetThreadCoreBind(((pstSva_dev->chan + 1) % 2));

    while (1)
    {
        /* TODO: 锁实现，睡眠存在问题--时间长度是否合适+轮询对CPU的浪费 */
        /* pthread_mutex_lock(&MutexHandle->lock); */
        #if 0   /* 加锁版本，应用下发双通道同步信号 */
        pthread_mutex_lock(&MutexHandle->lock);
        while (2 != g_sva_common.uiChnCnt)
        {
            pthread_cond_wait(&MutexHandle->cond, &MutexHandle->lock);
        }

        pthread_mutex_unlock(&MutexHandle->lock);
        #else /* 双通道默认同步版本 */
        if ((0 == g_sva_common.uiChnCnt)
            || (SVA_PROC_MODE_IMAGE == g_proc_mode)
            || (SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode
                && (SAL_TRUE != g_sva_common.stSva_dev[0].uiChnStatus || SAL_TRUE != g_sva_common.stSva_dev[1].uiChnStatus))
            || (SVA_PROC_MODE_VIDEO == g_proc_mode
                && (SAL_TRUE != g_sva_common.stSva_dev[0].uiChnStatus && SAL_TRUE != g_sva_common.stSva_dev[1].uiChnStatus))
            || (SAL_TRUE != Sva_DrvGetInitFlag()))
        {
            oldFramePTS[0] = 0;
            oldFramePTS[1] = 0;

            if (pstProcInterPrm->uiSyncFlag || !pstProcInterPrm->uiProcFlag)
            {
                pstProcInterPrm->uiSyncFlag = SAL_FALSE;
                SVA_LOGW("SAL_mutexWait, mode %d, cnt %d \n", g_proc_mode, g_sva_common.uiChnCnt);
                SAL_mutexWait(pstProcInterPrm->mChnMutexHd1);
                SVA_LOGW("SAL_mutexSignal, mode %d, cnt %d \n", g_proc_mode, g_sva_common.uiChnCnt);
                pstProcInterPrm->uiSyncFlag = SAL_FALSE;
            }

            continue;
        }

        #endif

        /* 帧率统计 */
        (VOID)Sva_DrvGetFps(&time000, &time111, &fps);

        enProcMode = g_proc_mode;

        time0 = SAL_getCurMs();

        d_time[0] = SAL_getCurMs();

        if (g_sva_common.stSva_dev[0].uiChnMirrorSync)
        {
            g_sva_common.stSva_dev[0].uiChnMirrorSync = 0;

            (VOID)Sva_drvChnWait(&g_sva_common.stSva_dev[0]);
        }

        /* 通道0的对应采集通道有视频输入且开启则获取数据 */
        noVideo[0] = 1; /* capt_func_chipCheckStatus(0); */
        s32Ret = Sva_DrvGetXsiSts(&xsiStatus[0]);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Get xsi sts Failed!\n");
            continue;
        }

        chnStatus[0] = g_sva_common.stSva_dev[0].uiChnStatus;
        chnStatus[1] = g_sva_common.stSva_dev[1].uiChnStatus;
        if (SAL_TRUE == chnStatus[0] && noVideo[0])    /* TODO: refacturing this code!!!! */
        {
            pDupChnHandle = Sva_DrvGetDupHandle(0);

            d_time[15] = SAL_getCurMs();
            s32Ret = Sva_HalGetFrame(pDupChnHandle, pstSysFrameInfo[0]);
            if (SAL_SOK != s32Ret)
            {
                usleep(10 * 1000);
                continue;
            }

            if (SAL_SOK != Sva_HalGetFramePTS(&u64Pts, pstSysFrameInfo[0]))
            {
                u64Pts = 1000;
                SVA_LOGW("sva get pts error\n");
            }

            d_time[14] = SAL_getCurMs();

            framePTS[0] = u64Pts / 1000;

            SVA_LOGD("0: get frame cost %d ms. pts %llu \n", d_time[14] - d_time[15], u64Pts);
        }

        if (g_sva_common.stSva_dev[1].uiChnMirrorSync)
        {
            g_sva_common.stSva_dev[1].uiChnMirrorSync = 0;

            (VOID)Sva_drvChnWait(&g_sva_common.stSva_dev[1]);
        }

        /* 通道1的对应采集通道有视频输入且开启则获取数据 */
        noVideo[1] = 1; /* capt_func_chipCheckStatus(1); */
        if (SAL_TRUE == chnStatus[1] && noVideo[1])
        {
            pDupChnHandle = Sva_DrvGetDupHandle(1);

            d_time[13] = SAL_getCurMs();

            s32Ret = Sva_HalGetFrame(pDupChnHandle, pstSysFrameInfo[1]);
            if (SAL_SOK != s32Ret)
            {
                if (SAL_TRUE == chnStatus[0] && noVideo[0])
                {
                    pDupChnHandle = Sva_DrvGetDupHandle(0);
                    (void)Sva_HalPutFrame(pDupChnHandle, pstSysFrameInfo[0]);
                }

                usleep(10 * 1000);
                continue;
            }

            if (SAL_SOK != Sva_HalGetFramePTS(&u64Pts, pstSysFrameInfo[1]))
            {
                u64Pts = 1000;
                SVA_LOGW("sva get pts error\n");
            }

            d_time[12] = SAL_getCurMs();

            framePTS[1] = u64Pts / 1000;
            SVA_LOGD("1: get frame cost %d ms. pts %llu \n", d_time[12] - d_time[13], u64Pts);
        }

        /* 两个通道同时开启且有视频输入做同步控制 */
        if (g_sva_common.uiChnCnt >= 2)
        {
            /* if (noVideo[1] && noVideo[0]) */
            if (chnStatus[0] && chnStatus[1] && (0 == capt_hal_getCaptUserPicStatue(0)) && (0 == capt_hal_getCaptUserPicStatue(1)))
            {
                needSync = 1;
            }
            else
            {
                needSync = 0;
                syncFlg[0] = 0;
                syncFlg[1] = 0;
            }
        }
        else
        {
            needSync = 0;
            syncFlg[0] = 0;
            syncFlg[1] = 0;
        }

        /* 通道0上电第一次和采集变化后重新获取采集的帧率 */
        if (1 == viChage[0])
        {
            srcFrame[0] = capt_func_chipGetViSrcRate(0);

            threshold[0] = 1000 / srcFrame[0] + 2;
            if (2 >= threshold[0])
            {
                threshold[0] = 17;
            }
        }

        /* 通道1上电第一次和采集变化后重新获取采集的帧率 */
        if (1 == viChage[1])
        {
            srcFrame[1] = capt_func_chipGetViSrcRate(1);

            threshold[1] = 1000 / srcFrame[1] + 2;
            if (2 >= threshold[1])
            {
                threshold[1] = 17;
            }
        }

        viChage[0] = 0;
        viChage[1] = 0;

        if (framePTS[0] < oldFramePTS[0])
        {
            oldFramePTS[0] = 0;
        }

        if (framePTS[1] < oldFramePTS[1])
        {
            oldFramePTS[1] = 0;
        }

        /* 做同步控制加抽帧 */
        if (needSync)
        {
            checkCnt[0] = 0;
            checkCnt[1] = 0;
check:
            /* SAL_INFO("sva  pts0 %llu pts1 %llu,old %llu,%llu\n", framePTS[0],framePTS[1],oldFramePTS[0], oldFramePTS[1]); */
            if ((framePTS[0] <= (framePTS[1] + 3 * threshold[1]) && framePTS[0] >= (framePTS[1] - 3 * threshold[1]))
                || (framePTS[1] <= (framePTS[0] + 3 * threshold[0]) && framePTS[1] >= (framePTS[0] - 3 * threshold[0])))
            {
                if (oldFramePTS[0] > 0 || oldFramePTS[1] > 0)
                {
                    if (!(framePTS[0] - oldFramePTS[0] > (THRESHOLD_0 - defPts[0] - threshold[0] + 5) && framePTS[0] - oldFramePTS[0] < (THRESHOLD_0 + defPts[0] + threshold[0]))
                        || !(framePTS[1] - oldFramePTS[1] > (THRESHOLD_1 - defPts[1] - threshold[1] + 5) && framePTS[1] - oldFramePTS[1] < (THRESHOLD_1 + defPts[1] + threshold[1])))
                    {
                        if (framePTS[0] - oldFramePTS[0] > (THRESHOLD_0 + threshold[0]) || framePTS[1] - oldFramePTS[1] > (THRESHOLD_1 + threshold[1]))
                        {
                            /* SAL_WARN("sva chn0 %d,defPts %d,chn1 %d,defPts %d,\n", framePTS[0]-oldFramePTS[0],defPts[0],framePTS[1]-oldFramePTS[1],defPts[1]); */
                            if (framePTS[0] - oldFramePTS[0] > (THRESHOLD_0 + threshold[0]))
                            {
                                defPts[0] = framePTS[0] - oldFramePTS[0] - THRESHOLD_0;
                                if (defPts[0] > THRESHOLD_0)
                                {
                                    defPts[0] = THRESHOLD_0;
                                }
                            }

                            if (framePTS[1] - oldFramePTS[1] > (THRESHOLD_1 + threshold[1]))
                            {
                                defPts[1] = framePTS[1] - oldFramePTS[1] - THRESHOLD_1;
                                if (defPts[1] > THRESHOLD_1)
                                {
                                    defPts[1] = THRESHOLD_1;
                                }
                            }
                        }
                        else
                        {
                            /* SAL_INFO("sva sync will skip\n"); */
                            defPts[0] = 0;
                            defPts[1] = 0;

                            uiTmp1 = (1 == uiTmpFlag) ? uiTmp1 + 1 : 0;
                            if (uiTmp1 > 10)
                            {
                                SVA_LOGW("cpu using warning 0! \n");
                                usleep(10 * 1000);
                                uiTmp1 = 0;
                            }

                            uiTmpFlag = 1;
                            goto putData;
                        }
                    }
                    else
                    {
                        defPts[0] = 0;
                        defPts[1] = 0;
                    }
                }
            }
            else
            {
                /* SAL_WARN("sva pts0 %llu pts1 %llu\n", framePTS[0],framePTS[1]); */
                /* 通道1时间戳比通道0时间戳小(时间差值在3帧内)，重新获取通道1新的数据 */
                if ((checkCnt[1] == 0) && ((framePTS[0] - 3 * threshold[0] > framePTS[1]) && (framePTS[0] - 5 * threshold[0] < framePTS[1])))
                {
                    pDupChnHandle = Sva_DrvGetDupHandle(1);
                    (void)Sva_HalPutFrame(pDupChnHandle, pstSysFrameInfo[1]);
                    s32Ret = Sva_HalGetFrame(pDupChnHandle, pstSysFrameInfo[1]);
                    if (SAL_SOK != s32Ret)
                    {
                        if (SAL_TRUE == chnStatus[0] && noVideo[0])
                        {
                            pDupChnHandle = Sva_DrvGetDupHandle(0);
                            (void)Sva_HalPutFrame(pDupChnHandle, pstSysFrameInfo[0]);
                        }

                        usleep(10 * 1000);
                        continue;
                    }

                    if (SAL_SOK != Sva_HalGetFramePTS(&u64Pts, pstSysFrameInfo[1]))
                    {
                        u64Pts = 1000;
                        SVA_LOGW("sva get pts error\n");
                    }

                    framePTS[1] = u64Pts / 1000;
                    checkCnt[1] = 1;
                    /* SAL_INFO("sva 1 will get data again\n"); */

                    uiTmp4 = 4 == uiTmpFlag ? uiTmp4 + 1 : 0;
                    if (uiTmp4 > 10)
                    {
                        SVA_LOGW("cpu using warning 3! \n");
                        usleep(10 * 1000);
                        uiTmp4 = 0;
                    }

                    uiTmpFlag = 4;
                    goto check;
                }
                /* 通道0时间戳比通道1时间戳小(时间差值在3帧内)，重新获取通道0新的数据 */
                else if ((checkCnt[0] == 0) && (framePTS[1] - 3 * threshold[1] > framePTS[0]) && (framePTS[1] - 5 * threshold[1] < framePTS[0]))
                {
                    pDupChnHandle = Sva_DrvGetDupHandle(0);
                    (void)Sva_HalPutFrame(pDupChnHandle, pstSysFrameInfo[0]);
                    s32Ret = Sva_HalGetFrame(pDupChnHandle, pstSysFrameInfo[0]);
                    if (SAL_SOK != s32Ret)
                    {
                        if (SAL_TRUE == chnStatus[1] && noVideo[1])
                        {
                            pDupChnHandle = Sva_DrvGetDupHandle(1);
                            (void)Sva_HalPutFrame(pDupChnHandle, pstSysFrameInfo[1]);
                        }

                        usleep(10 * 1000);
                        continue;
                    }

                    if (SAL_SOK != Sva_HalGetFramePTS(&u64Pts, pstSysFrameInfo[0]))
                    {
                        u64Pts = 1000;
                        SVA_LOGW("sva get pts error\n");
                    }

                    framePTS[0] = u64Pts / 1000;
                    checkCnt[0] = 1;
                    /* SAL_INFO("sva 0 will get data again\n"); */

                    uiTmp2 = 2 == uiTmpFlag ? uiTmp2 + 1 : 0;
                    if (uiTmp2 > 10)
                    {
                        SVA_LOGW("cpu using warning 1! \n");
                        usleep(10 * 1000);
                        uiTmp2 = 0;
                    }

                    uiTmpFlag = 2;
                    goto check;
                }
                else /* 两通道时间戳差值较大丢掉 */
                {
                    /* SAL_WARN("sva will putdata pts %llu pts %llu check0 %d check1 %d threshold0 %d threshold1 %d\n", */
                    /*          framePTS[0],framePTS[1], checkCnt[0], checkCnt[1], threshold[0], threshold[1]); */

                    uiTmp3 = 3 == uiTmpFlag ? uiTmp3 + 1 : 0;
                    if (uiTmp3 > 10)
                    {
                        SVA_LOGW("cpu using warning 2! \n");
                        usleep(10 * 1000);
                        uiTmp3 = 0;
                    }

                    uiTmpFlag = 3;
                    goto putData;
                }
            }
        }
        else /* 不做同步控制只做抽帧 */
        {
            for (i = 0; i < 2; i++)
            {
                if ((1 != chnStatus[i]) || (1 != noVideo[i]))
                {
                    continue;
                }

                if (oldFramePTS[i] > 0)
                {
                    if (!(framePTS[i] - oldFramePTS[i] > (THRESHOLD_0 - threshold[i] + 5) && framePTS[i] - oldFramePTS[i] < (THRESHOLD_0 + threshold[i])))
                    {
                        if (framePTS[i] - oldFramePTS[i] > (THRESHOLD_0 + threshold[i]))
                        {
                            SVA_LOGD("sva chn0 %d,chn1 %llu.[%llu,%llu]\n",
                                     i, framePTS[i] - oldFramePTS[i], framePTS[i], oldFramePTS[i]);
                            defPts[i] = time1 - time0 - THRESHOLD_0;
                            if (defPts[i] > THRESHOLD_0)
                            {
                                defPts[i] = THRESHOLD_0;
                            }

                            defPts[i] = defPts[1 - i];
                        }
                        else
                        {
                            /* SAL_INFO("sva will skip\n"); */
                            /* SAL_WARN("szl_dbg: goto exit 2  \n"); */
                            uiTmp5 = 5 == uiTmpFlag ? uiTmp5 + 1 : 0;
                            if (uiTmp5 > 10)
                            {
                                SVA_LOGW("cpu using warning 4! \n");
                                usleep(10 * 1000);
                                uiTmp5 = 0;
                            }

                            uiTmpFlag = 5;
                            goto putData;
                        }
                    }
                }
            }
        }

        uiTmp1 = 0;
        uiTmp2 = 0;
        uiTmp3 = 0;
        uiTmp4 = 0;
        uiTmp5 = 0;

        d_time[1] = SAL_getCurMs();

        /* 此处加锁保护，在完成给引擎推帧前不允许退出引擎资源 */
        SAL_mutexLock(pstSva_dev1->mChnMutexHdl);
        for (i = 0; i < SVA_DEV_MAX; i++)
        {
            pstSva_dev = &g_sva_common.stSva_dev[i];
            pstPrmFullQue = &pstSva_dev->stPrmFullQue;
            pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;
            uiQueueCnt = DSA_QueGetQueuedCount(pstPrmFullQue);
            if (uiQueueCnt)
            {
                /* 获取 buffer */
                DSA_QueGet(pstPrmFullQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
                sal_memcpy_s(&pastIn[i], sizeof(SVA_PROCESS_IN), pstIn, sizeof(SVA_PROCESS_IN));
                DSA_QuePut(pstPrmEmptQue, (void *)pstIn, SAL_TIMEOUT_NONE);
            }
        }

        for (i = 0; i < SVA_DEV_MAX; i++)
        {
            pstSva_dev = &g_sva_common.stSva_dev[i];

            /* 保存算法支持的分辨率 */
            if (SAL_TRUE == pstSva_dev->uiChnStatus && chnStatus[i] && noVideo[i])
            {
                if (needSync)
                {
                    syncFlg[i] = 1;
                    if (!(1 == syncFlg[0]
                          && 1 == syncFlg[1]))
                    {
                        SVA_LOGW("continue %d,%d !\n", syncFlg[0], syncFlg[1]);
                        SAL_msleep(10);
                        startFlag = 1;
                        continue;
                    }

                    /* 无视频信号的时候不送算法 */
                    userPic[i] = 0; /* capt_hal_getCaptUserPicStatue(i); */
                    if (userPic[i])
                    {
                        noVideoflag[i] = 1;
                        SAL_msleep(20);
                        continue;
                    }

                    /* 上次处理是无视频信号两个通道同时返回，保证时间参数一致，以便做同步处理 */
                    if (noVideoflag[i])
                    {
                        noVideoflag[0] = 0;
                        noVideoflag[1] = 0;
                        framePTS[0] = 0;
                        framePTS[1] = 0;
                        oldFramePTS[0] = 0;
                        oldFramePTS[1] = 0;
                        /* getFrameNum[0] = 0; */
                        /* getFrameNum[1] = 0; */
                        viChage[0] = 1;
                        viChage[1] = 1;
                        i = 2;
                        SAL_msleep(10);
                        continue;
                    }
                }

                if (startFlag)
                {
                    startFlag = 0;
                    SAL_mutexUnlock(pstSva_dev1->mChnMutexHdl);

                    usleep(10 * 1000);
                    goto putData;
                }

                oldFramePTS[i] = framePTS[i];
            }
        }

        d_time[2] = SAL_getCurMs();
        s32Ret = Sva_HalVcaeSyncPutData(pstSysFrameInfo, pastIn, &chnStatus[0], &xsiStatus[0], g_sva_common.uiMainViewChn, enProcMode);
        if (s32Ret != SAL_SOK)
        {
            SVA_LOGD("chan %d continue ! mode %d input_cnt %d, cb_cnt %d \n",
                     pstSva_dev->chan, enProcMode, g_sva_common.stDbgPrm.stCommSts.uiInputCnt, g_sva_common.stDbgPrm.stCommSts.uiOutputCnt);

            uiInputErrCnt++;
            if (uiInputErrCnt > 20)
            {
                SVA_LOGW("chan %d continue !\n", pstSva_dev->chan);
                uiInputErrCnt = 0;
                usleep(10 * 1000);
            }
        }
        else
        {
            g_sva_common.stDbgPrm.stCommSts.uiInputCnt++;
            SVA_LOGD("chan %d Success ! input_cnt %d, cb_cnt %d \n",
                     pstSva_dev->chan, g_sva_common.stDbgPrm.stCommSts.uiInputCnt, g_sva_common.stDbgPrm.stCommSts.uiOutputCnt);

            uiInputErrCnt = 0;
        }

        SAL_mutexUnlock(pstSva_dev1->mChnMutexHdl);

        fps++;

        d_time[3] = SAL_getCurMs();

        time1 = SAL_getCurMs();
        if ((1 == syncFlg[0]
             && 1 == syncFlg[1]))
        {
            if (time1 - time0 > (THRESHOLD_1 + 20))
            {
                if ((oldFramePTS[0] == framePTS[0]) || (oldFramePTS[1] == framePTS[1]))
                {
                    defPts[0] = time1 - time0 - THRESHOLD_1;
                    if (defPts[0] > THRESHOLD_1)
                    {
                        defPts[0] = THRESHOLD_1;
                    }

                    defPts[1] = defPts[0];
                    SVA_LOGD("chn %d,defTime %d !\n", i, time1 - time0);
                }
            }
        }

putData:
        if (SAL_TRUE == chnStatus[0] && noVideo[0])
        {
            pDupChnHandle = Sva_DrvGetDupHandle(0);
            (void)Sva_HalPutFrame(pDupChnHandle, pstSysFrameInfo[0]);
        }

        if (SAL_TRUE == chnStatus[1] && noVideo[1])
        {
            pDupChnHandle = Sva_DrvGetDupHandle(1);
            (void)Sva_HalPutFrame(pDupChnHandle, pstSysFrameInfo[1]);
        }

        d_time[4] = SAL_getCurMs();

        if (Sva_HalGetAlgDbgFlag() && d_time[4] - d_time[0] > 16)
        {
            /* if (d_time[4] - d_time[0] > 16) */
            SAL_WARN("debug: %d, %d, %d, %d, total %d \n",
                     d_time[1] - d_time[0], d_time[2] - d_time[1],
                     d_time[3] - d_time[2], d_time[4] - d_time[3], d_time[4] - d_time[0]);
        }

        memset(d_time, 0x00, 5 * sizeof(UINT32));
    }
}

/**
 * @function:   Sva_DrvPutMirrorFrame
 * @brief:      释放镜像帧数据
 * @param[in]:  SVA_DEV_PRM *pstSva_dev
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Sva_DrvPutMirrorFrame(SVA_DEV_PRM *pstSva_dev)
{
    INT32 s32Ret = SAL_SOK;

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    s32Ret = Sva_HalPutFrame(pstSva_dev->DupHandle.pVpHandle, &pstSva_dev->stJpegVenc.stMirFrameBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("put frame faield! chan %d \n", pstSva_dev->chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvGetMirrorFrame
 * @brief:      获取镜像帧数据
 * @param[in]:  SVA_DEV_PRM *pstSva_dev
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrame
 * @param[in]:  SYSTEM_FRAME_INFO **ppstOutFrm
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Sva_DrvGetMirrorFrame(SVA_DEV_PRM *pstSva_dev,
                                   SYSTEM_FRAME_INFO *pstSysFrame,
                                   SYSTEM_FRAME_INFO **ppstOutFrm)
{
    INT32 s32Ret = SAL_SOK;

    DUP_ChanHandle *pDupChnHandle = NULL;
    SYSTEM_FRAME_INFO *pstMirrorFrmBuf = NULL;

    DUP_COPY_DATA_BUFF stDupDataBuf = {0};

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSysFrame, SAL_FAIL);

    pDupChnHandle = (DUP_ChanHandle *)pstSva_dev->DupHandle.pVpHandle;

    s32Ret = dup_task_sendToDup(pDupChnHandle->dupModule, pstSysFrame);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("send to dup failed! ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    pstMirrorFrmBuf = &pstSva_dev->stJpegVenc.stMirFrameBuf;

    stDupDataBuf.pstDstSysFrame = pstMirrorFrmBuf;
    s32Ret = pDupChnHandle->dupOps.OpDupGetBlit((VOID *)pDupChnHandle, &stDupDataBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Get Mirror Frame Failed! ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    *ppstOutFrm = pstMirrorFrmBuf;

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvGetCropInfo
 * @brief:      获取裁剪信息
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  CROP_S *pstCropInfo
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 ATTRIBUTE_UNUSED Sva_DrvGetCropInfo(SVA_PROCESS_OUT *pstOut, CROP_S *pstCropInfo)
{
    UINT32 u32Tmp = 0;

    if (NULL == pstOut || NULL == pstCropInfo)
    {
        SVA_LOGE("ptr null! %p, %p \n", pstOut, pstCropInfo);
        return SAL_FAIL;
    }

    /* 包裹上下左右裁剪，各预留50像素 */
    pstCropInfo->u32CropEnable = pstOut->packbagAlert.package_valid;

    u32Tmp = (UINT32)(pstOut->packbagAlert.package_loc.x * (float)SVA_MODULE_WIDTH);
    u32Tmp = u32Tmp > 50 ? u32Tmp - 50 : 0;
    pstCropInfo->u32X = SAL_alignDown(u32Tmp, 16);

    u32Tmp = (UINT32)(pstOut->packbagAlert.package_loc.y * (float)SVA_MODULE_HEIGHT);
    u32Tmp = u32Tmp > 50 ? u32Tmp - 50 : 0;
    pstCropInfo->u32Y = SAL_alignDown(u32Tmp, 16);

    u32Tmp = (UINT32)((pstOut->packbagAlert.package_loc.x + pstOut->packbagAlert.package_loc.width) * (float)SVA_MODULE_WIDTH);
    u32Tmp = u32Tmp + 50 >= SVA_MODULE_WIDTH ? SVA_MODULE_WIDTH : u32Tmp + 50;
    pstCropInfo->u32W = SAL_alignDown(u32Tmp - pstCropInfo->u32X, 4);

    u32Tmp = (UINT32)((pstOut->packbagAlert.package_loc.y + pstOut->packbagAlert.package_loc.height) * (float)SVA_MODULE_HEIGHT);
    u32Tmp = u32Tmp + 50 >= SVA_MODULE_HEIGHT ? SVA_MODULE_HEIGHT : u32Tmp + 50;
    pstCropInfo->u32H = SAL_alignDown(u32Tmp - pstCropInfo->u32Y, 4);

    /* 保护裁剪区域不超出画面分辨率 */
    pstCropInfo->u32Y = pstCropInfo->u32Y <= 0 ? 0 : pstCropInfo->u32Y;
    pstCropInfo->u32X = pstCropInfo->u32X <= 0 ? 0 : pstCropInfo->u32X;
    pstCropInfo->u32W = pstCropInfo->u32X + pstCropInfo->u32W >= SVA_MODULE_WIDTH \
                        ? SVA_MODULE_WIDTH - pstCropInfo->u32X \
                        : pstCropInfo->u32W;
    pstCropInfo->u32H = pstCropInfo->u32Y + pstCropInfo->u32H >= SVA_MODULE_HEIGHT \
                        ? SVA_MODULE_HEIGHT - pstCropInfo->u32Y \
                        : pstCropInfo->u32H;
    return SAL_SOK;
}

/**
 * @function   Sva_DrvGetSliceCropInfo
 * @brief      计算裁剪区域内目标的坐标信息
 * @param[in]  SVA_PROCESS_OUT *pstOut
 * @param[in]  CROP_S *pstCropInfo
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvGetSliceCropInfo(SVA_PROCESS_OUT *pstOut, CROP_S *pstCropInfo)
{
    UINT32 uiBorGapPixel = 0;
    SVA_RECT_F *pstCutPkgLoc = NULL;

    pstCutPkgLoc = pstOut->stInsEncRslt.bRemainValid ? &pstOut->stInsEncRslt.stCutRemainPkgRect : &pstOut->stInsEncRslt.stInsPkgRect;

    pstCropInfo->u32CropEnable = SAL_TRUE;
    pstCropInfo->u32X = SAL_alignDown((UINT32)(pstCutPkgLoc->x * (float)SVA_MODULE_WIDTH) + 1, 2); /* ALIGN_UP((UINT32)(pstCutPkgLoc->x * (float)SVA_MODULE_WIDTH), 16); */
    pstCropInfo->u32Y = SAL_alignDown((UINT32)(pstCutPkgLoc->y * (float)SVA_MODULE_HEIGHT) + 1, 2);
    pstCropInfo->u32W = SAL_alignDown((UINT32)(pstCutPkgLoc->width * (float)SVA_MODULE_WIDTH) + 2, 4);
    pstCropInfo->u32H = SAL_alignDown((UINT32)(pstCutPkgLoc->height * (float)SVA_MODULE_HEIGHT) + 2, 4);

    if (pstCropInfo->u32W < 32)
    {
        SVA_LOGW("w[%d] < 32! \n", pstCropInfo->u32W);

        pstCropInfo->u32W = 32;
        pstCropInfo->u32X = pstCropInfo->u32X + pstCropInfo->u32W >= SVA_MODULE_WIDTH - uiBorGapPixel \
                            ? SVA_MODULE_WIDTH - uiBorGapPixel - pstCropInfo->u32W \
                            : pstCropInfo->u32X;
    }

    if (pstCropInfo->u32X + pstCropInfo->u32W >= SVA_MODULE_WIDTH - uiBorGapPixel)
    {
        pstCropInfo->u32W = SVA_MODULE_WIDTH - uiBorGapPixel - pstCropInfo->u32X;
    }

    if (pstCropInfo->u32H < 32)
    {
        SVA_LOGW("h[%d] < 32! \n", pstCropInfo->u32H);

        pstCropInfo->u32H = 32;
        pstCropInfo->u32Y = pstCropInfo->u32Y + pstCropInfo->u32H >= SVA_MODULE_HEIGHT - uiBorGapPixel \
                            ? SVA_MODULE_HEIGHT - uiBorGapPixel - pstCropInfo->u32H \
                            : pstCropInfo->u32Y;
    }

    if (pstCropInfo->u32Y + pstCropInfo->u32H >= SVA_MODULE_HEIGHT - uiBorGapPixel)
    {
        pstCropInfo->u32H = SVA_MODULE_HEIGHT - uiBorGapPixel - pstCropInfo->u32Y;
    }

    return SAL_SOK;
}

#if 1  /* 集中判图调试功能，验证最后一个分片是否超时 */
#define SPLIT_OVER_TIME_FRM_BUF_NUM (7)
typedef struct _SVA_OVER_TIME_INFO_
{
    UINT32 uiIdx;
    UINT32 uiFrmNum[SPLIT_OVER_TIME_FRM_BUF_NUM];
} SVA_OVER_TIME_INFO_S;

/**
 * @function    Sva_DrvFillOverTimeFrmInfo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvFillOverTimeFrmInfo(UINT32 uiFrmNum, SVA_OVER_TIME_INFO_S *pstOverTimeInfo)
{
    pstOverTimeInfo->uiFrmNum[pstOverTimeInfo->uiIdx] = uiFrmNum;
    pstOverTimeInfo->uiIdx = (pstOverTimeInfo->uiIdx + 1) % SPLIT_OVER_TIME_FRM_BUF_NUM;

    return;
}

/**
 * @function    Sva_DrvPrintOverTimeFrmInfo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvPrintOverTimeFrmInfo(UINT32 chan, SVA_OVER_TIME_INFO_S *pstOverTimeInfo)
{
    UINT32 i = 0;

    printf("=================== chan[%d] print over time frm info ===================", chan);
    for (i = pstOverTimeInfo->uiIdx + SPLIT_OVER_TIME_FRM_BUF_NUM - 1; i > pstOverTimeInfo->uiIdx; i--)
    {
        if (0 == pstOverTimeInfo->uiFrmNum[i % SPLIT_OVER_TIME_FRM_BUF_NUM])
        {
            continue;
        }

        printf("%d ", pstOverTimeInfo->uiFrmNum[i % SPLIT_OVER_TIME_FRM_BUF_NUM]);
    }

    printf("\n");
    return;
}

/**
 * @function    Sva_DrvSetSplitDbgSwitch
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_DrvSetSplitDbgSwitch(UINT32 u32Flag)
{
    u32SplitFlag = u32Flag;
    return;
}

/**
 * @function    Sva_DrvPrintDbgSplitInfo
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvPrintDbgSplitInfo(UINT32 chan, UINT32 u32OverTimeCnt, SVA_OVER_TIME_INFO_S *pstOverTimeInfo)
{
    if (0 == u32SplitFlag)
    {
        return;
    }

    SAL_WARN("chan %d, over time cnt[%d] \n", chan, u32OverTimeCnt);

    (VOID)Sva_DrvPrintOverTimeFrmInfo(chan, pstOverTimeInfo);
    return;
}

#endif

/**
 * @function:   Sva_DrvTransTarLoc
 * @brief:      目标坐标转换
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  UINT32 *puiRealTarNum
 * @param[in]:  SVA_TARGET *pstRealTarArr
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Sva_DrvTransTarLoc(SVA_PROCESS_OUT *pstOut, UINT32 *puiRealTarNum, SVA_TARGET *pstRealTarArr)
{
    UINT32 j = 0;
    UINT32 uiRealTarNum = 0;
    UINT32 uiGapPixel = 0;

    float fPkgLeftX = 0.0;
    float fPkgRightX = 0.0;
    float fPkgUpY = 0.0;
    float fPkgDownY = 0.0;

    float fTarX = 0.0;
    float fTarY = 0.0;
    float fTarMidX = 0.0;
    float fTarMidY = 0.0;

    SVA_RECT_F *pstPkgLoc = NULL;
    SVA_TARGET *pstTarInfo = NULL;
    SVA_TARGET *pstRealTarInfo = NULL;

    if (NULL == pstOut || NULL == puiRealTarNum || NULL == pstRealTarArr)
    {
        SVA_LOGE("ptr null! %p, %p, %p \n", pstOut, puiRealTarNum, pstRealTarArr);
        return;
    }

    pstPkgLoc = &pstOut->packbagAlert.package_loc;

    for (j = 0; j < pstOut->target_num; j++)
    {
        pstRealTarInfo = pstRealTarArr + uiRealTarNum;
        pstTarInfo = &pstOut->target[j];

        fTarMidX = pstTarInfo->rect.x + pstTarInfo->rect.width / 2.0;
        fTarMidY = pstTarInfo->rect.y + pstTarInfo->rect.height / 2.0;

        fPkgLeftX = pstPkgLoc->x - (float)uiGapPixel / (float)SVA_MODULE_WIDTH;
        fPkgRightX = pstPkgLoc->x + pstPkgLoc->width + (float)uiGapPixel / (float)SVA_MODULE_WIDTH;
        fPkgUpY = pstPkgLoc->y;   /* pstPkgLoc->y - (float)uiGapPixel / (float)SVA_MODULE_HEIGHT; */
        fPkgDownY = pstPkgLoc->y + pstPkgLoc->height; /* pstPkgLoc->y + pstPkgLoc->height + (float)uiGapPixel / (float)SVA_MODULE_HEIGHT; */

        fPkgLeftX = fPkgLeftX <= 0.0 ? 0.001 : fPkgLeftX;
        fPkgRightX = fPkgRightX >= 1.0 ? 0.999 : fPkgRightX;
        fPkgUpY = fPkgUpY <= 0.0 ? 0.001 : fPkgUpY;
        fPkgDownY = fPkgDownY >= 1.0 ? 0.999 : fPkgDownY;

        if (fTarMidX < fPkgLeftX || fTarMidX > fPkgRightX
            || fTarMidY < fPkgUpY || fTarMidY > fPkgDownY)
        {
            continue;
        }

        memcpy(pstRealTarInfo, pstTarInfo, sizeof(SVA_TARGET));

        fTarX = pstTarInfo->rect.x <= fPkgLeftX ? fPkgLeftX + 0.001 : pstTarInfo->rect.x;
        fTarY = pstTarInfo->rect.y <= fPkgUpY ? fPkgUpY + 0.001 : pstTarInfo->rect.y;

#if 1
        /* 更新目标在包裹内的相对坐标 */
        pstRealTarInfo->rect.x = (pstTarInfo->rect.x - fPkgLeftX) / (fPkgRightX - fPkgLeftX);
        pstRealTarInfo->rect.y = (pstTarInfo->rect.y - fPkgUpY) / (fPkgDownY - fPkgUpY);
        pstRealTarInfo->rect.width = (pstTarInfo->rect.x + pstTarInfo->rect.width >= fPkgRightX) \
                                     ? (fPkgRightX - fTarX) / (fPkgRightX - fPkgLeftX) \
                                     : (pstTarInfo->rect.x + pstTarInfo->rect.width - fTarX) / (fPkgRightX - fPkgLeftX);
        pstRealTarInfo->rect.height = (pstTarInfo->rect.y + pstTarInfo->rect.height >= fPkgDownY) \
                                      ? (fPkgDownY - fTarY) / (fPkgDownY - fPkgUpY) \
                                      : (pstTarInfo->rect.y + pstTarInfo->rect.height - fTarY) / (fPkgDownY - fPkgUpY);

        pstRealTarInfo->rect.x = pstRealTarInfo->rect.x <= 0.0 ? 0.001 : pstRealTarInfo->rect.x;
        pstRealTarInfo->rect.y = pstRealTarInfo->rect.y <= 0.0 ? 0.001 : pstRealTarInfo->rect.y;
        pstRealTarInfo->rect.width = pstRealTarInfo->rect.width + pstRealTarInfo->rect.x >= 1.0 \
                                     ? 0.999 - pstRealTarInfo->rect.x \
                                     : pstRealTarInfo->rect.width;
        pstRealTarInfo->rect.height = pstRealTarInfo->rect.height + pstRealTarInfo->rect.y >= 1.0 \
                                      ? 0.999 - pstRealTarInfo->rect.y \
                                      : pstRealTarInfo->rect.height;
#endif

        uiRealTarNum++;
    }

    *puiRealTarNum = uiRealTarNum;
    return;
}

/**
 * @function:   Sva_DrvResetVencVar
 * @brief:      重置编码相关参数
 * @param[in]:  SVA_DEV_PRM *pstSva_dev
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Sva_DrvResetVencVar(SVA_DEV_PRM *pstSva_dev)
{
    INT32 i = 0;

    for (i = 0; i < SVA_MAX_CALLBACK_JPEG_NUM; i++)
    {
        pstSva_dev->stJpegVenc.uiPicJpegSize[i] = 0;
    }

    for (i = 0; i < SVA_MAX_CALLBACK_BMP_NUM; i++)
    {
        pstSva_dev->stJpegVenc.uiPicBmpSize[i] = 0;
    }

    return;
}

/**
 * @function:   Sva_DrvImgTransTarLoc
 * @brief:      图片模式下目标坐标转换
 * @param[in]:  SVA_PROCESS_OUT *pstOut
 * @param[in]:  UINT32 *puiRealTarNum
 * @param[in]:  SVA_TARGET *pstRealTarArr
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Sva_DrvImgTransTarLoc(SVA_PROCESS_OUT *pstOut, UINT32 *puiRealTarNum, SVA_TARGET *pstRealTarArr)
{
    UINT32 i = 0;
    SVA_TARGET *pstTmpPstOutTarInfo = NULL;
    SVA_TARGET *pstTmpTarArrInfo = NULL;

    if (NULL == pstOut || NULL == puiRealTarNum || NULL == pstRealTarArr)
    {
        SVA_LOGE("ptr null! %p, %p, %p \n", pstOut, puiRealTarNum, pstRealTarArr);
        return;
    }

    if (g_proc_mode == SVA_PROC_MODE_IMAGE)
    {
        *puiRealTarNum = pstOut->target_num;
        for (i = 0; i < *puiRealTarNum; i++)
        {
            pstTmpTarArrInfo = pstRealTarArr + i;
            pstTmpPstOutTarInfo = &pstOut->target[i];
            sal_memcpy_s(pstTmpTarArrInfo, sizeof(SVA_TARGET), pstTmpPstOutTarInfo, sizeof(SVA_TARGET));

            SVA_LOGD("jpeg tar:i %d, id %d, type %d, xy[%f, %f], wh[%f, %f] \n", i,
                     pstTmpTarArrInfo->ID, pstTmpTarArrInfo->type,
                     pstTmpTarArrInfo->rect.x, pstTmpTarArrInfo->rect.y,
                     pstTmpTarArrInfo->rect.width, pstTmpTarArrInfo->rect.height);
        }
    }

    return;
}

/**
 * @function:   Sva_DrvProcSplitFlag
 * @brief:      分片模式下回传应用层的分片信号后处理
 * @param[in]:  SVA_DEV_PRM *pstSva_dev
 * @param[in]:  SVA_DSP_SVA_PACK *pstDspOutPkgInfo
 * @param[out]: SVA_DSP_SVA_PACK *pstDspOutPkgInfo
 * @return:     static VOID
 */
static VOID ATTRIBUTE_UNUSED Sva_DrvProcSplitFlag(SVA_DEV_PRM *pstSva_dev, SVA_DSP_SVA_PACK *pstDspOutPkgInfo)
{
    if (NULL == pstSva_dev
        || NULL == pstDspOutPkgInfo)
    {
        SVA_LOGW("ptr null! %p, %p \n", pstSva_dev, pstDspOutPkgInfo);
        goto exit;
    }

    /* 如果分片开始，全局标记置1 */
    if (pstDspOutPkgInfo->split_group_start_flag)
    {
        pstSva_dev->bSplitSerialStart = SAL_TRUE;
    }

    /* 如果包裹出全，全局标记置0 */
    if (pstDspOutPkgInfo->package_valid)
    {
        /* 如果包裹已经出全，但是全局标记为0，则默认将该完整包裹作为一个分片信号返回上层 */
        if (SAL_TRUE != pstSva_dev->bSplitSerialStart)
        {
            pstDspOutPkgInfo->split_group_start_flag = SAL_TRUE;
            SVA_LOGW("no start flag, proc as a whole package! \n");
        }

        pstSva_dev->bSplitSerialStart = SAL_FALSE;
    }

exit:
    return;
}

/**
 * @function   Sva_DrvGetJencCropInfo
 * @brief      获取编图裁剪信息
 * @param[in]  IN UINT32 uiJpgIdx
 * @param[in]  IN SVA_PROCESS_OUT *pstOut
 * @param[in]  OUT CROP_S *pstCropPrm
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvGetJencCropInfo(IN UINT32 uiJpgIdx,
                                    IN SVA_PROCESS_OUT *pstOut,
                                    OUT CROP_S *pstCropInfo)
{
    if (1 == uiJpgIdx)
    {
        /* 获取分片裁剪信息 */
        Sva_DrvGetSliceCropInfo(pstOut, pstCropInfo);
    }
    else if (0 == uiJpgIdx || 2 == uiJpgIdx)
    {
        /* 获取整包裁剪信息 */
        float fPkgX = 0.0;
        float fPkgY = 0.0;
        float fPkgW = 0.0;
        float fPkgH = 0.0;

        if (pstOut->packbagAlert.package_valid)
        {
            fPkgX = pstOut->packbagAlert.package_loc.x;
            fPkgY = pstOut->packbagAlert.fRealPkgY;
            fPkgW = pstOut->packbagAlert.package_loc.width;
            fPkgH = pstOut->packbagAlert.fRealPkgH;
        }

        SVA_LOGD("raw crop info: [%f, %f] [%f, %f], valid %d \n", fPkgX, fPkgY, fPkgW, fPkgH, pstOut->packbagAlert.package_valid);

        pstCropInfo->u32CropEnable = pstOut->packbagAlert.package_valid;
        pstCropInfo->u32X = SAL_alignDown((UINT32)(fPkgX * SVA_MODULE_WIDTH) + 1, 2);
        pstCropInfo->u32Y = SAL_alignDown((UINT32)(fPkgY * SVA_MODULE_HEIGHT) + 1, 2);
        pstCropInfo->u32W = SAL_alignDown((UINT32)(fPkgW * SVA_MODULE_WIDTH) + 2, 4); /* SVA_MODULE_WIDTH; */
        pstCropInfo->u32H = SAL_alignDown((UINT32)(fPkgH * SVA_MODULE_HEIGHT) + 2, 4); /* SVA_MODULE_HEIGHT; */

        /* protection */
        pstCropInfo->u32W = (pstCropInfo->u32X + pstCropInfo->u32W >= SVA_MODULE_WIDTH  \
                             ? SVA_MODULE_WIDTH - pstCropInfo->u32X : pstCropInfo->u32W);
        pstCropInfo->u32H = (pstCropInfo->u32Y + pstCropInfo->u32H >= SVA_MODULE_HEIGHT    \
                             ? SVA_MODULE_HEIGHT - pstCropInfo->u32Y : pstCropInfo->u32H);

        pstCropInfo->u32W = SAL_alignDown(pstCropInfo->u32W, 16);
        pstCropInfo->u32H = SAL_alignDown(pstCropInfo->u32H, 2);
    }
    else
    {
        SVA_LOGW("invalid jpeg idx %d \n", uiJpgIdx);
    }

    return SAL_SOK;
}

#if 0

/**
 * @function    Sva_DrvDebugDumpData
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void Sva_DrvDebugDumpData(CHAR *pData, CHAR *pPath, UINT32 uiSize, UINT32 uiFlag)
{
    INT32 ret = 0;
    CHAR sz[2][16] = {"w+", "a+"};

    FILE *fp = NULL;

    fp = fopen(pPath, sz[uiFlag]);
    if (!fp)
    {
        SAL_ERROR("fopen %s failed! \n", pPath);
        goto exit;
    }

    ret = fwrite(pData, uiSize, 1, fp);
    if (ret < 0)
    {
        SAL_ERROR("fwrite err! \n");
        goto exit;
    }

    fflush(fp);

exit:
    if (NULL != fp)
    {
        fclose(fp);
        fp = NULL;
    }

    return;
}

#endif

/**
 * @function   Sva_DrvDoJencOnce
 * @brief      执行一次编图
 * @param[in]  SVA_DEV_PRM *pstSva_dev
 * @param[in]  SVA_PROCESS_OUT *pstOut
 * @param[in]  BOOL bOsdOverlay
 * @param[in]  SYSTEM_FRAME_INFO *pstSrcFrameInfo
 * @param[in]  SVA_DSP_OUT *pstDspCbOut
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvDoJencOnce(SVA_DEV_PRM *pstSva_dev,
                               SVA_PROCESS_OUT *pstOut,
                               UINT32 uiJpgIdx,
                               BOOL bOsdOverlay,
                               SYSTEM_FRAME_INFO *pstSrcFrameInfo,
                               SVA_DSP_OUT *pstDspCbOut)
{
    INT32 s32Ret = SAL_FAIL;

    CROP_S stCropPrm = {0};
    JPEG_COMMON_ENC_PRM_S jpegEncPrm = {0};
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    Sva_DrvGetJencCropInfo(uiJpgIdx, pstOut, &stCropPrm); /* (VOID)Sva_DrvGetRealCropInfo(pstOut, &stCropPrm); */

    if (SVA_PROC_MODE_IMAGE == g_proc_mode && (SAL_FALSE == stCropPrm.u32W || SAL_FALSE == stCropPrm.u32H))
    {
        stCropPrm.u32W = pstOut->uiSourceImgW;
        stCropPrm.u32H = pstOut->uiSourceImgH;
    }

    s32Ret = sys_hal_getVideoFrameInfo(&pstSva_dev->stJpegVenc.stTmpJpegFrameBuf, &stVideoFrmBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("sys_hal_getVideoFrameInfo failed !\n");
        return SAL_FAIL;
    }

    sal_memset_s((void *)stVideoFrmBuf.virAddr[0], 1280 * 1024, 0xff, 1280 * 1024);
    sal_memset_s((void *)stVideoFrmBuf.virAddr[0] + 1280 * 1024, 1280 * 1024 / 2, 0x80, 1280 * 1024 / 2);

    if (SAL_SOK != Ia_TdeQuickCopy(pstSrcFrameInfo, &pstSva_dev->stJpegVenc.stTmpJpegFrameBuf,
                                   stCropPrm.u32X, stCropPrm.u32Y,
                                   stCropPrm.u32W, stCropPrm.u32H, SAL_TRUE))
    {
        SVA_LOGE("copy V1 failed! [%d, %d][%d, %d] \n",
                 stCropPrm.u32X, stCropPrm.u32Y, stCropPrm.u32W, stCropPrm.u32H);
    }

    sal_memcpy_s(&jpegEncPrm.stSysFrame, sizeof(SYSTEM_FRAME_INFO), &pstSva_dev->stJpegVenc.stTmpJpegFrameBuf, sizeof(SYSTEM_FRAME_INFO));
    jpegEncPrm.pOutJpeg = pstSva_dev->stJpegVenc.pPicJpeg[uiJpgIdx];

    /* 为了兼容分片模式，裁剪X、Y需要16对齐。为了规避16对齐带来的拼接像素误差，故将其拷贝到从左上角(0,0)开始计算 */
    stCropPrm.u32X = stCropPrm.u32Y = 0;

    /* jpeg编图: 无叠框信息 */
    if (SVA_PROC_MODE_IMAGE != g_proc_mode)
    {
        (VOID)jpeg_drv_cropEnc(pstSva_dev->stJpegVenc.pstJpegChnInfo,
                               &jpegEncPrm,
                               &stCropPrm);
    }
    else
    {
        (VOID)jpeg_drv_cropEnc(pstSva_dev->stJpegVenc.pstJpegChnInfo,
                               &jpegEncPrm,
                               NULL);
    }

    pstSva_dev->stJpegVenc.uiPicJpegSize[uiJpgIdx] = jpegEncPrm.outSize;

    /* 回传第一张无叠框的jpeg数据 */
    pstDspCbOut->stJpegResult[uiJpgIdx].cJpegAddr = (UINT8 *)pstSva_dev->stJpegVenc.pPicJpeg[uiJpgIdx];
    pstDspCbOut->stJpegResult[uiJpgIdx].uiJpegSize = pstSva_dev->stJpegVenc.uiPicJpegSize[uiJpgIdx];
    pstDspCbOut->stJpegResult[uiJpgIdx].uiWidth = stCropPrm.u32W;
    pstDspCbOut->stJpegResult[uiJpgIdx].uiHeight = stCropPrm.u32H;
    pstDspCbOut->stJpegResult[uiJpgIdx].uiSyncNum = pstOut->frame_sync_idx;
    pstDspCbOut->stJpegResult[uiJpgIdx].ullTimePts = pstOut->frame_stamp;
    SVA_LOGD("jpeg_%d: chan %d Sync %d jpeg_size %d\n", uiJpgIdx, pstSva_dev->chan,
             pstDspCbOut->stJpegResult[uiJpgIdx].uiSyncNum, pstDspCbOut->stJpegResult[uiJpgIdx].uiJpegSize);

    /* TODO: pos info 未使用，待确认 */
    sal_memcpy_s(pstDspCbOut->stJpegResult[uiJpgIdx].cJpegAddr + pstDspCbOut->stJpegResult[uiJpgIdx].uiJpegSize,
                 pstOut->pos_info.xsi_pos_size,
                 pstOut->pos_info.xsi_pos_buf,
                 pstOut->pos_info.xsi_pos_size);

    pstDspCbOut->stJpegResult[uiJpgIdx].uiJpegSize += pstOut->pos_info.xsi_pos_size;

    return SAL_SOK;
}

/**
 * @function   Sva_DrvPrVideoFrmInfo
 * @brief      打印帧信息(调试接口)
 * @param[in]  SYSTEM_FRAME_INFO *pstFrmInfo
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvPrVideoFrmInfo(SYSTEM_FRAME_INFO *pstFrmInfo)
{
    INT32 s32Ret = SAL_FAIL;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    s32Ret = sys_hal_getVideoFrameInfo(pstFrmInfo, &stVideoFrmBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("sys_hal_getVideoFrameInfo failed !\n");
        return SAL_FAIL;
    }
    else
    {
        SVA_LOGD("sva_venc_2: get vir[%p, %p, %p], phy[%p, %p, %p] \n",
                 (VOID *)stVideoFrmBuf.virAddr[0], (VOID *)stVideoFrmBuf.virAddr[1], (VOID *)stVideoFrmBuf.virAddr[2],
                 (VOID *)stVideoFrmBuf.phyAddr[0], (VOID *)stVideoFrmBuf.phyAddr[1], (VOID *)stVideoFrmBuf.phyAddr[2]);
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvDoJencOsdOnce
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvDoJencOsdOnce(UINT32 chan, SYSTEM_FRAME_INFO *pstSrc, SYSTEM_FRAME_INFO *pstDst, CROP_S *pstCropPrm,
                                  SVA_PROCESS_IN *pstIn,
                                  SVA_PROCESS_OUT *pstSvaOut,
                                  SVA_TARGET *pstSvaTarget,
                                  SVA_OSD_JPG_PRM_S *pstOsdJpgPrm)
{
    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    UINT32 uiDstX = 0;
    UINT32 uiDstY = 0;
    SVA_OSD_JPG_PRM_S stSvaPicTarPrm = {0};

    if (SAL_SOK != Sva_HalJpegCalcTargetNum(chan, pstIn, pstSvaOut, &stSvaPicTarPrm))
    {
        SVA_LOGE("Sva_HalJpegCalcTargetNum fail!  \n");
        return SAL_FAIL;
    }

    pstOsdJpgPrm->uiUpLen = stSvaPicTarPrm.uiUpLen;
    pstOsdJpgPrm->uiDownLen = stSvaPicTarPrm.uiDownLen;
    pstOsdJpgPrm->uiUpTarNum = stSvaPicTarPrm.uiUpTarNum;
    pstOsdJpgPrm->uiDownTarNum = stSvaPicTarPrm.uiDownTarNum;

    SVA_LOGD("calc: UpTarNum %d, UpLen %d, DownTarNum %d, DownLen %d, OsdWidth %d, OsdHeight %d \n ",
             stSvaPicTarPrm.uiUpTarNum, stSvaPicTarPrm.uiUpLen,
             stSvaPicTarPrm.uiDownTarNum, stSvaPicTarPrm.uiDownLen,
             stSvaPicTarPrm.uiOsdWidth, stSvaPicTarPrm.uiOsdHeight);

    uiDstY += stSvaPicTarPrm.uiUpLen;

    (VOID)sys_hal_getVideoFrameInfo(pstDst, &stVideoFrmBuf);
    sal_memset_s((void *)stVideoFrmBuf.virAddr[0], SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH, 0xff, SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH);
    sal_memset_s((void *)stVideoFrmBuf.virAddr[0] + SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH, SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH / 2, 0x80, SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH / 2);

    if (SVA_PROC_MODE_IMAGE == g_proc_mode && (SAL_FALSE == pstCropPrm->u32W || SAL_FALSE == pstCropPrm->u32H))
    {
        pstCropPrm->u32W = pstSvaOut->uiSourceImgW;
        pstCropPrm->u32H = pstSvaOut->uiSourceImgH;
    }

    if (SAL_SOK != Ia_TdeQuickCopyTmp(pstSrc, pstDst,
                                      pstCropPrm->u32X, pstCropPrm->u32Y, uiDstX, uiDstY, pstCropPrm->u32W, pstCropPrm->u32H, SAL_FALSE))
    {
        SVA_LOGE("Ia_TdefQuickCopyVideo failed! \n");
        return SAL_FAIL;
    }

    pstDst->uiDataWidth = SVA_MODULE_WIDTH;
    pstDst->uiDataHeight = SVA_MODULE_HEIGHT;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvJpegVencThread
* 描  述  : 编图线程
* 输  入  : - prm: 通道全局变量
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static void *Sva_DrvJpegVencThread(void *prm)
{
    UINT32 j = 0;
    UINT32 k = 0;
    UINT32 uiSingleProcErrCnt = 0;
    UINT32 uiJpgIdx = 0;
    UINT32 uiBmpIdx = 0;

    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;
    UINT32 time4 = 0;
    UINT32 time5 = 0;
    UINT32 time6 = 0;
    UINT32 time7 = 0;
    UINT32 time8 = 0;
    UINT32 time9 = 0;
    UINT32 time10 = 0;

    UINT32 uiRealTarNum = 0;
    SVA_TARGET *pastRealTarInfo = NULL;

    UINT32 u32LastTime[2] = {0};                   /* 用于统计最后一个分片的耗时 */
    UINT32 u32OverTimeCnt[2] = {0};                /* 用于统计最后一个分片耗时超时时的目标信息 */
    SVA_OVER_TIME_INFO_S stOverTimeInfo[2] = {0};  /* 最后一个分片耗时超时的帧序号，用于追溯具体分片和整包信息 */

    SVA_ORIENTATION_TYPE enDirection = 0;
    INT32 s32Ret = SAL_FAIL;
    UINT32 uiOffsetX = 0;
    /* UINT32 uiRealTarNum = 0; */
    UINT64 u64PhyAddr = 0;

    SVA_JPEG_INFO_ST *pstJpegFrame = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    SYSTEM_FRAME_INFO *pstSrcFrameInfo = NULL;
    SVA_RECT_F *pstPkgRect = NULL;

    /* 创建 JPEG 编码器 */
    SAL_VideoFrameParam stHalPrm = {0};
    STREAM_ELEMENT stStreamEle = {0};
    SVA_DSP_OUT stSvaDspOut = {0};
    CROP_S stCropPrm = {0};
    SVA_OSD_JPG_PRM_S stOsdJpgPrm = {0};

    SVA_DEV_PRM *pstSva_dev = (SVA_DEV_PRM *)prm;

    CAPB_AI *pstCapAiPrm = NULL;

    SVA_DRV_CHECK_PRM(pstSva_dev, NULL);

    pastRealTarInfo = &pstSva_dev->stRealtarget[0];

    pstCapAiPrm = capb_get_ai();

    sal_memset_s(&stHalPrm, sizeof(SAL_VideoFrameParam), 0x00, sizeof(SAL_VideoFrameParam));

    stHalPrm.fps = 1;
    stHalPrm.width = SVA_MODULE_WIDTH;
    stHalPrm.height = SVA_MODULE_HEIGHT;
    stHalPrm.encodeType = MJPEG;
    stHalPrm.quality = 80;
    stHalPrm.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

    s32Ret = jpeg_drv_createChn((void **)&pstSva_dev->stJpegVenc.pstJpegChnInfo, &stHalPrm);
    if (s32Ret != SAL_SOK)
    {
        SVA_LOGE("error !!!\n");
        return NULL;
    }

    /* pstSva_dev->stJpegVenc.uiJpegEncChn = ((JPEG_DRV_CHN_INFO_S *)pstSva_dev->stJpegVenc.pstJpegChnInfo)->u32Chan; */

    SVA_LOGI("Xsi Chan %d JpegEncChnHdl %p \n", pstSva_dev->chan, (VOID *)pstSva_dev->stJpegVenc.pstJpegChnInfo);

    /* 内存申请，用来存放编图 */
    for (j = 0; j < SVA_MAX_CALLBACK_JPEG_NUM; j++)
    {
        s32Ret = mem_hal_mmzAlloc(SVA_JPEG_SIZE, "SVA", "sva_jpeg_enc", NULL, SAL_TRUE, &u64PhyAddr, (VOID **)&pstSva_dev->stJpegVenc.pPicJpeg[j]);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("error !!!\n");
            return NULL;
        }

        SAL_clear(pstSva_dev->stJpegVenc.pPicJpeg[j]);
    }

    /* TODO: 安检机不需要回调bmp数据 */
    for (j = 0; j < SVA_MAX_CALLBACK_BMP_NUM; j++)
    {
        s32Ret = mem_hal_mmzAlloc(SVA_BMP_SIZE, "SVA", "sva_bmp_enc", NULL, SAL_TRUE, &u64PhyAddr, (VOID **)&pstSva_dev->stJpegVenc.pPicBmp[j]);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("error !!!\n");
            return NULL;
        }

        SAL_clear(pstSva_dev->stJpegVenc.pPicBmp[j]);
    }

    /* 用于Yuv转Bmp使用 */
    s32Ret = Sva_DrvGetFrameMem(SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, SAL_TRUE, &pstSva_dev->stJpegVenc.stTmpFrameBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Get Frame MMZ failed \n");
        return NULL;
    }

    /* 用于Yuv转Bmp使用 */
    s32Ret = Sva_DrvGetFrameMem(SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, SAL_TRUE, &pstSva_dev->stJpegVenc.stTmpJpegFrameBuf);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Get Frame MMZ failed \n");
        return NULL;
    }

    s32Ret = Sva_DrvGetFrameMem(SVA_BUF_WIDTH_MAX, SVA_MODULE_WIDTH, SAL_FALSE, &pstSva_dev->stJpegVenc.stTmpOsdJpegFrameBuf);
    if (s32Ret != SAL_SOK)
    {
        SVA_LOGE("Disp_halGetFrameMem error !!!\n");
        return NULL;
    }

    /* SAL_DEBUG("Xsi Chan %d JpegEncChn %d \n", pstSva_dev->chan, pstSva_dev->stJpegVenc.uiJpegEncChn); */

    /* 1. 创建管理满队列 */
    pstFullQue = &pstSva_dev->stJpegVenc.pstFullQue;
    if (SAL_SOK != DSA_QueCreate(pstFullQue, SVA_MAX_JPEG_FRM_BUF))
    {
        SVA_LOGE("error !!!\n");
        return NULL;
    }

    /* 2. 创建管理空队列 */
    pstEmptQue = &pstSva_dev->stJpegVenc.pstEmptQue;
    if (SAL_SOK != DSA_QueCreate(pstEmptQue, SVA_MAX_JPEG_FRM_BUF))
    {
        SVA_LOGE("error !!!\n");
        return NULL;
    }

    /* 3. 缓存区放入空队列 */
    for (j = 0; j < SVA_MAX_JPEG_FRM_BUF; j++)
    {
        pstJpegFrame = &pstSva_dev->stJpegVenc.stJpegFrame[j];

        s32Ret = Sva_DrvGetFrameMem(SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, SAL_FALSE, &pstJpegFrame->stFrame);
        if (s32Ret != SAL_SOK)
        {
            SVA_LOGE("Sva_DrvGetFrameMem error !!!\n");
            return NULL;
        }

        if (SAL_SOK != DSA_QuePut(pstEmptQue, (void *)pstJpegFrame, SAL_TIMEOUT_NONE))
        {
            SVA_LOGE("error !!!\n");
            return NULL;
        }
    }

    if (0x00 == pstSva_dev->stJpegVenc.stMirFrameBuf.uiAppData)
    {
        if (SAL_SOK != sys_hal_allocVideoFrameInfoSt(&pstSva_dev->stJpegVenc.stMirFrameBuf))
        {
            SVA_LOGE("alloc video frame info st failed! \n");
            return NULL;
        }
    }

    SAL_SET_THR_NAME();

    SAL_SetThreadCoreBind((pstSva_dev->chan % 2) + 2);

    while (1)
    {
        /* 获取 buffer */
        DSA_QueGet(pstFullQue, (void **)&pstJpegFrame, SAL_TIMEOUT_FOREVER);

        uiSingleProcErrCnt = 0;
        g_sva_common.stDbgPrm.stCommSts.uiJencCnt++;
        g_sva_common.stDbgPrm.stCommSts.uiJencMirrorFlag = 1;    /* 标记 */

        SVA_LOGD("venc enter! chan %d, frmNum %d, time %d \n", pstSva_dev->chan, pstJpegFrame->szJpegOut.frame_num, SAL_getCurMs());
        time0 = SAL_getCurMs();

        /* 清空上一次回调的结果 */
        sal_memset_s(&stStreamEle, sizeof(STREAM_ELEMENT), 0, sizeof(STREAM_ELEMENT));
        sal_memset_s(&stSvaDspOut, sizeof(SVA_DSP_OUT), 0, sizeof(SVA_DSP_OUT));

        (VOID)Sva_DrvResetVencVar(pstSva_dev);

        stStreamEle.type = STREAM_ELEMENT_SVA_IMG;
        stStreamEle.chan = pstSva_dev->chan;

        stCropPrm.u32CropEnable = SAL_TRUE;
        stCropPrm.u32X = 0;
        stCropPrm.u32Y = 0;
        stCropPrm.u32W = pstJpegFrame->szJpegOut.uiImgW;
        stCropPrm.u32H = pstJpegFrame->szJpegOut.uiImgH;

        Sva_DrvPrVideoFrmInfo(&pstJpegFrame->stFrame);

        if (SVA_MODULE_WIDTH == pstJpegFrame->szJpegOut.uiImgW
            && SVA_MODULE_HEIGHT == pstJpegFrame->szJpegOut.uiImgH)
        {
            stCropPrm.u32CropEnable = SAL_FALSE;
        }

        pstSrcFrameInfo = &pstJpegFrame->stFrame;
        enDirection = pstJpegFrame->stIn.enDirection;

        /* 分析仪从左到右过包需要进行帧数据镜像处理，送入算法的镜像拿到DSP侧进行了处理 */
        if ((SVA_JPEG_MASK_RECT & pstCapAiPrm->uiCbJpgNum)
            && (SVA_ORIENTATION_TYPE_L2R == enDirection))
        {
            s32Ret = Sva_DrvGetMirrorFrame(pstSva_dev, &pstJpegFrame->stFrame, &pstSrcFrameInfo);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("Get Mirror Frame Failed! chan %d \n", pstSva_dev->chan);
                uiSingleProcErrCnt++;
                goto putData;
            }
        }

        g_sva_common.stDbgPrm.stCommSts.uiJencMirrorFlag = 0;    /* 标记 */
        g_sva_common.stDbgPrm.stCommSts.uiJencjpg1Flag = 1;      /* 标记 */

        time1 = SAL_getCurMs();

        uiJpgIdx = 0;
        uiBmpIdx = 0;

        /* 分片处理 */
        if (pstJpegFrame->szJpegOut.stInsEncRslt.uiInsEncFlag)
        {
            s32Ret = Sva_DrvDoJencOnce(pstSva_dev,
                                       &pstJpegFrame->szJpegOut,
                                       1, /* 分片索引 */
                                       SAL_FALSE,
                                       pstSrcFrameInfo,
                                       &stSvaDspOut);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("do jenc once failed! \n");
                uiSingleProcErrCnt++;
            }
        }

        /* 分包结果处理，编第一张不叠框的JPEG */
        if (SVA_JPEG_MASK_NO_RECT & pstCapAiPrm->uiCbJpgNum
            && pstJpegFrame->szJpegOut.packbagAlert.package_valid)
        {
            /* 编第一张不叠框的JPEG */
            s32Ret = Sva_DrvDoJencOnce(pstSva_dev,
                                       &pstJpegFrame->szJpegOut,
                                       0, /* 第一张不叠框jpeg */
                                       SAL_FALSE,
                                       pstSrcFrameInfo,
                                       &stSvaDspOut);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("do jenc once failed! \n");
                uiSingleProcErrCnt++;
            }
        }

        g_sva_common.stDbgPrm.stCommSts.uiJencjpg1Flag = 0;    /* 标记 */
        g_sva_common.stDbgPrm.stCommSts.uiJencBmpFlag = 1;     /* 标记 */

        time5 = SAL_getCurMs();

        Sva_DrvPrVideoFrmInfo(&pstJpegFrame->stFrame);

        /* 编第二张不叠框的BMP */
        if (SVA_BMP_MASK_NO_RECT & pstCapAiPrm->uiCbBmpNum
            && pstJpegFrame->szJpegOut.packbagAlert.package_valid)
        {
            s32Ret = Sva_HalYuv2Bmp(pstSrcFrameInfo,
                                    &pstSva_dev->stJpegVenc.stTmpFrameBuf,
                                    &pstJpegFrame->stIn,
                                    &pstJpegFrame->szJpegOut,
                                    gDbgSampleCollect[pstSva_dev->chan],
                                    pstSva_dev->stJpegVenc.pPicBmp[uiBmpIdx],
                                    &pstSva_dev->stJpegVenc.uiPicBmpSize[uiBmpIdx],
                                    SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, SAL_TRUE);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("yuv 2 bmp failed! \n");
                uiSingleProcErrCnt++;
                goto REUSE;
            }

            /* 回传第三张有叠框的bmp数据 */
            stSvaDspOut.stBmpResult[uiBmpIdx].cBmpAddr = (UINT8 *)pstSva_dev->stJpegVenc.pPicBmp[uiBmpIdx];
            stSvaDspOut.stBmpResult[uiBmpIdx].uiBmpSize = pstSva_dev->stJpegVenc.uiPicBmpSize[uiBmpIdx];
            stSvaDspOut.stBmpResult[uiBmpIdx].uiSyncNum = pstJpegFrame->szJpegOut.frame_sync_idx;

            uiBmpIdx++;
        }

        g_sva_common.stDbgPrm.stCommSts.uiJencBmpFlag = 0;      /* 标记 */
        g_sva_common.stDbgPrm.stCommSts.uiJencjpg2Flag = 1;     /* 标记 */

        Sva_DrvPrVideoFrmInfo(&pstJpegFrame->stFrame);

        time6 = SAL_getCurMs();

        /*
            编第三张叠框的JPEG。
            西安定制: 为了规避包裹分割时目标不全的问题，需要增加目标更新的信息，同时增加画面叠框编图数据给应用用于图片查询。
         */
        if (SVA_JPEG_MASK_RECT & pstCapAiPrm->uiCbJpgNum
            && pstJpegFrame->szJpegOut.packbagAlert.package_valid)
        {
            uiJpgIdx = 2;   /* 回调jpg索引2存放叠框完整包裹 */

            if (SVA_PROC_MODE_IMAGE != g_proc_mode)
            {
                (VOID)Sva_DrvGetJencCropInfo(uiJpgIdx, &pstJpegFrame->szJpegOut, &stCropPrm);
                sal_memset_s(&stOsdJpgPrm, sizeof(SVA_OSD_JPG_PRM_S), 0x00, sizeof(SVA_OSD_JPG_PRM_S));
                /* 编图需要16字节对齐，裁剪大小小于目标大小 */
                stCropPrm.u32X = (SAL_alignDown(stCropPrm.u32X + 8, 16) + stCropPrm.u32W <= SVA_MODULE_WIDTH)
                                 ? SAL_alignDown(stCropPrm.u32X + 8, 16) : SAL_alignDown(stCropPrm.u32X + 8, 16) - 16;
                stCropPrm.u32Y = (SAL_alignDown(stCropPrm.u32Y + 8, 16) + stCropPrm.u32H <= SVA_MODULE_HEIGHT)
                                 ? SAL_alignDown(stCropPrm.u32Y + 8, 16) : SAL_alignDown(stCropPrm.u32Y + 8, 16) - 16;

                s32Ret = Sva_DrvDoJencOsdOnce(pstSva_dev->chan,
                                              pstSrcFrameInfo,
                                              &pstSva_dev->stJpegVenc.stTmpOsdJpegFrameBuf,
                                              &stCropPrm,
                                              &pstJpegFrame->stIn,
                                              &pstJpegFrame->szJpegOut,
                                              pastRealTarInfo,
                                              &stOsdJpgPrm);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGE("Sva do jenc osd error, corp [%d, %d][%d, %d]  \n", stCropPrm.u32X, stCropPrm.u32Y, stCropPrm.u32W, stCropPrm.u32H);
                }

                /* jpeg编图+yuv转bmp: 有叠框信息 */
                Sva_HalJpegDrawLine(pstSva_dev->chan,
                                    &pstSva_dev->stJpegVenc.stTmpOsdJpegFrameBuf,
                                    &stCropPrm,
                                    &pstJpegFrame->stIn,
                                    &pstJpegFrame->szJpegOut,
                                    pstSva_dev->stJpegVenc.pstJpegChnInfo,
                                    pstSva_dev->stJpegVenc.pPicJpeg[uiJpgIdx],
                                    &pstSva_dev->stJpegVenc.uiPicJpegSize[uiJpgIdx],
                                    SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, g_proc_mode, &stOsdJpgPrm);
            }
            else
            {
                /* jpeg编图+yuv转bmp: 有叠框信息 */
                Sva_HalJpegDrawLine(pstSva_dev->chan,
                                    pstSrcFrameInfo,
                                    NULL,
                                    &pstJpegFrame->stIn,
                                    &pstJpegFrame->szJpegOut,
                                    pstSva_dev->stJpegVenc.pstJpegChnInfo,
                                    pstSva_dev->stJpegVenc.pPicJpeg[uiJpgIdx],
                                    &pstSva_dev->stJpegVenc.uiPicJpegSize[uiJpgIdx],
                                    SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, g_proc_mode, &stOsdJpgPrm);
            }

            /* 回传第二张有叠框的jpeg数据 */
            stSvaDspOut.stJpegResult[uiJpgIdx].cJpegAddr = (UINT8 *)pstSva_dev->stJpegVenc.pPicJpeg[uiJpgIdx];
            stSvaDspOut.stJpegResult[uiJpgIdx].uiJpegSize = pstSva_dev->stJpegVenc.uiPicJpegSize[uiJpgIdx];
            stSvaDspOut.stJpegResult[uiJpgIdx].uiWidth = SVA_MODULE_WIDTH;
            stSvaDspOut.stJpegResult[uiJpgIdx].uiHeight = SVA_MODULE_HEIGHT;
            stSvaDspOut.stJpegResult[uiJpgIdx].uiSyncNum = pstJpegFrame->szJpegOut.frame_sync_idx;
            stSvaDspOut.stJpegResult[uiJpgIdx].ullTimePts = pstJpegFrame->szJpegOut.frame_stamp;
            stSvaDspOut.stJpegPkgTime.fPkgfwd[0] = pstJpegFrame->szJpegOut.packbagAlert.stPkgStartInfo.fPkgfwd;
            sal_memcpy_s(&stSvaDspOut.stJpegPkgTime.absTime[0], sizeof(DATE_TIME), &pstJpegFrame->szJpegOut.packbagAlert.stPkgStartInfo.absTime, sizeof(DATE_TIME));
            stSvaDspOut.stJpegPkgTime.fPkgfwd[1] = pstJpegFrame->szJpegOut.packbagAlert.stPkgEndInfo.fPkgfwd;
            sal_memcpy_s(&stSvaDspOut.stJpegPkgTime.absTime[1], sizeof(DATE_TIME), &pstJpegFrame->szJpegOut.packbagAlert.stPkgEndInfo.absTime, sizeof(DATE_TIME));
            SVA_LOGD("jpeg_%d: chan %d Sync %d jpeg_size %d\n", uiJpgIdx, pstSva_dev->chan,
                     stSvaDspOut.stJpegResult[uiJpgIdx].uiSyncNum, stSvaDspOut.stJpegResult[uiJpgIdx].uiJpegSize);

            sal_memcpy_s(stSvaDspOut.stJpegResult[uiJpgIdx].cJpegAddr + stSvaDspOut.stJpegResult[uiJpgIdx].uiJpegSize,
                         pstJpegFrame->szJpegOut.pos_info.xsi_pos_size,
                         pstJpegFrame->szJpegOut.pos_info.xsi_pos_buf,
                         pstJpegFrame->szJpegOut.pos_info.xsi_pos_size);

            stSvaDspOut.stJpegResult[uiJpgIdx].uiJpegSize += pstJpegFrame->szJpegOut.pos_info.xsi_pos_size;
        }

        g_sva_common.stDbgPrm.stCommSts.uiJencjpg2Flag = 0;     /* 标记 */
        g_sva_common.stDbgPrm.stCommSts.uiJencSplitModeFlag = 1;     /* 标记 */

        time7 = SAL_getCurMs();

        stSvaDspOut.jpeg_cnt = SVA_MAX_CALLBACK_JPEG_NUM;
        stSvaDspOut.bmp_cnt = SVA_MAX_CALLBACK_BMP_NUM;

        /* 帧信息 */
        stSvaDspOut.frame_num = pstJpegFrame->szJpegOut.frame_num;
        stSvaDspOut.framePts = pstJpegFrame->szJpegOut.frame_stamp;

        /* 包裹信息 */
        stSvaDspOut.packbagAlert.candidate_flag = pstJpegFrame->szJpegOut.packbagAlert.candidate_flag;
        stSvaDspOut.packbagAlert.package_valid = pstJpegFrame->szJpegOut.packbagAlert.package_valid;
        stSvaDspOut.packbagAlert.pkg_force_split_flag = pstJpegFrame->szJpegOut.bPkgForceSplit;
        stSvaDspOut.packbagAlert.split_group_end_flag = pstJpegFrame->szJpegOut.packbagAlert.bSplitEnd;
        stSvaDspOut.packbagAlert.split_group_start_flag = pstJpegFrame->szJpegOut.packbagAlert.bSplitStart;
        stSvaDspOut.packbagAlert.package_match_id = pstJpegFrame->szJpegOut.packbagAlert.package_match_index;
        stSvaDspOut.packbagAlert.package_id = pstJpegFrame->szJpegOut.packbagAlert.package_id;

        /* 集中判图模式需要对分片内的目标坐标实现从整个画面到分片的映射 */
        if (SAL_TRUE == pstJpegFrame->szJpegOut.bSplitMode)
        {
            /* 更新算法V2.0后不存在这种没有包裹开始的情况，故此接口注释掉 */
            /* (VOID)Sva_DrvProcSplitFlag(pstSva_dev, &stSvaDspOut.packbagAlert); */

            pstPkgRect = &pstJpegFrame->szJpegOut.stInsEncRslt.stInsPkgRect;

            stSvaDspOut.fPkgUpGapPixel = (pstJpegFrame->szJpegOut.packbagAlert.fRealPkgY - pstJpegFrame->stIn.rect.y) > 0
                                         ? (pstJpegFrame->szJpegOut.packbagAlert.fRealPkgY - pstJpegFrame->stIn.rect.y) / (float)pstJpegFrame->stIn.rect.height
                                         : 0;
            stSvaDspOut.fPkgDownGapPixel = (pstJpegFrame->stIn.rect.y + pstJpegFrame->stIn.rect.height) > (pstJpegFrame->szJpegOut.packbagAlert.fRealPkgY + pstJpegFrame->szJpegOut.packbagAlert.fRealPkgH)
                                           ? (pstJpegFrame->stIn.rect.y + pstJpegFrame->stIn.rect.height - pstJpegFrame->szJpegOut.packbagAlert.fRealPkgY - pstJpegFrame->szJpegOut.packbagAlert.fRealPkgH) / (float)pstJpegFrame->stIn.rect.height
                                           : 0;

            /* 无分片时，客户端那边将整包结果作为分片上传到平台，无需上下边沿裁剪 */
            if (SAL_TRUE == stSvaDspOut.packbagAlert.split_group_start_flag
                && SAL_TRUE == stSvaDspOut.packbagAlert.package_valid)
            {
                stSvaDspOut.fPkgUpGapPixel = stSvaDspOut.fPkgDownGapPixel = 0.0f;
            }

            /* 复检台使用，整包包裹及目标信息 */
            stSvaDspOut.packbagAlert.package_loc.x = pstPkgRect->x;
            stSvaDspOut.packbagAlert.package_loc.y = pstJpegFrame->szJpegOut.packbagAlert.fRealPkgY;
            stSvaDspOut.packbagAlert.package_loc.width = pstPkgRect->width;
            stSvaDspOut.packbagAlert.package_loc.height = pstJpegFrame->szJpegOut.packbagAlert.fRealPkgH;

            /* 若满足分片处理条件，则获取分片内包含的违禁品目标信息及坐标转换Location */
            pstSva_dev->u32SliceTarNum = 0;
            for (j = 0; j < pstJpegFrame->szJpegOut.target_num; j++)
            {
                /* 目标信息坐标空间转换 */
                (VOID)Sva_DrvTransSplitTarLoc(pstSva_dev->chan, &pstJpegFrame->szJpegOut, &pstJpegFrame->szJpegOut, j);
            }

            /* 报警物信息, 集中判图定制需求: 此处只回调新包裹目标信息给上层应用 */
            stSvaDspOut.target_num = pstSva_dev->u32SliceTarNum;
            SVA_LOGD("chan %d, target %d \n", stStreamEle.chan, stSvaDspOut.target_num);

            k = 0;
            for (j = 0; j < stSvaDspOut.target_num; j++)
            {
                if (SAL_TRUE == pstSva_dev->stSliceTarget[j].bAnotherViewTar)
                {
                    continue;
                }

                stSvaDspOut.target[k].ID = pstSva_dev->stSliceTarget[j].ID;
                stSvaDspOut.target[k].type = pstSva_dev->stSliceTarget[j].type;
                stSvaDspOut.target[k].color = pstSva_dev->stSliceTarget[j].color;
                stSvaDspOut.target[k].alarm_flg = pstSva_dev->stSliceTarget[j].alarm_flg;
                stSvaDspOut.target[k].confidence = pstSva_dev->stSliceTarget[j].confidence;
                stSvaDspOut.target[k].visual_confidence = pstSva_dev->stSliceTarget[j].visual_confidence;

                if (SVA_JPEG_MASK_RECT & pstCapAiPrm->uiCbJpgNum)
                {
                    stSvaDspOut.target[k].rect.x = pstSva_dev->stSliceTarget[j].rect.x;
                    stSvaDspOut.target[k].rect.y = pstSva_dev->stSliceTarget[j].rect.y;
                    stSvaDspOut.target[k].rect.width = pstSva_dev->stSliceTarget[j].rect.width;
                    stSvaDspOut.target[k].rect.height = pstSva_dev->stSliceTarget[j].rect.height;
                }
                else /* TODO_XA: 安检机暂未适配 */
                {
                    stSvaDspOut.target[k].rect.x = (float)((pstJpegFrame->szJpegOut.target[j].rect.x * SVA_MODULE_WIDTH - (float)uiOffsetX) / (float)pstJpegFrame->szJpegOut.uiImgW);
                    stSvaDspOut.target[k].rect.y = (float)(pstJpegFrame->szJpegOut.target[j].rect.y * SVA_MODULE_HEIGHT / (float)pstJpegFrame->szJpegOut.uiImgH);
                    stSvaDspOut.target[k].rect.width = (float)(pstJpegFrame->szJpegOut.target[j].rect.width * SVA_MODULE_WIDTH / (float)pstJpegFrame->szJpegOut.uiImgW);
                    stSvaDspOut.target[k].rect.height = (float)(pstJpegFrame->szJpegOut.target[j].rect.height * SVA_MODULE_HEIGHT / (float)pstJpegFrame->szJpegOut.uiImgH);
                }

                k++;

                SVA_LOGD("j %d, [%f, %f] [%f, %f] \n", k,
                         stSvaDspOut.target[k].rect.x, stSvaDspOut.target[k].rect.y,
                         stSvaDspOut.target[k].rect.width, stSvaDspOut.target[k].rect.height);
            }

            stSvaDspOut.target_num = k;
        }

        /* 存储的图片需要回调包裹内的违禁品目标信息，并实现坐标从整个画面到包裹内的映射 */
        if (pstJpegFrame->szJpegOut.packbagAlert.package_valid)
        {
            /* 广州南站回传所有的目标信息给上层，不需要过滤当前包裹内部 */
            uiRealTarNum = 0;

            /* 非集中判图模式，需要将包裹实际宽高替换回来 */
            pstJpegFrame->szJpegOut.packbagAlert.package_loc.y = pstJpegFrame->szJpegOut.packbagAlert.fRealPkgY;
            pstJpegFrame->szJpegOut.packbagAlert.package_loc.height = pstJpegFrame->szJpegOut.packbagAlert.fRealPkgH;

            (VOID)Sva_HalTransTarLoc(&pstJpegFrame->szJpegOut, &uiRealTarNum, pastRealTarInfo);

            stSvaDspOut.packbagAlert.package_loc.x = pstJpegFrame->szJpegOut.packbagAlert.package_loc.x;
            stSvaDspOut.packbagAlert.package_loc.y = pstJpegFrame->szJpegOut.packbagAlert.package_loc.y;
            stSvaDspOut.packbagAlert.package_loc.width = pstJpegFrame->szJpegOut.packbagAlert.package_loc.width;
            stSvaDspOut.packbagAlert.package_loc.height = pstJpegFrame->szJpegOut.packbagAlert.package_loc.height;

            /* 兼容图片模式下的危险品坐标信息 */
            (VOID)Sva_DrvImgTransTarLoc(&pstJpegFrame->szJpegOut, &uiRealTarNum, pastRealTarInfo);

            k = 0;
            for (j = 0; j < uiRealTarNum; j++)
            {
                if (SAL_TRUE == pastRealTarInfo[j].bAnotherViewTar)
                {
                    continue;
                }

                stSvaDspOut.target[k].ID = pastRealTarInfo[j].ID;
                stSvaDspOut.target[k].type = pastRealTarInfo[j].type;
                stSvaDspOut.target[k].color = pastRealTarInfo[j].color;
                stSvaDspOut.target[k].alarm_flg = pastRealTarInfo[j].alarm_flg;
                stSvaDspOut.target[k].confidence = pastRealTarInfo[j].confidence;
                stSvaDspOut.target[k].visual_confidence = pastRealTarInfo[j].visual_confidence;

                if (SVA_JPEG_MASK_RECT & pstCapAiPrm->uiCbJpgNum)
                {
                    stSvaDspOut.target[k].rect.x = pastRealTarInfo[j].rect.x;
                    stSvaDspOut.target[k].rect.y = pastRealTarInfo[j].rect.y;
                    stSvaDspOut.target[k].rect.width = pastRealTarInfo[j].rect.width;
                    stSvaDspOut.target[k].rect.height = pastRealTarInfo[j].rect.height;
                }
                else
                {
                    stSvaDspOut.target[k].rect.x = (float)((pstJpegFrame->szJpegOut.target[j].rect.x * SVA_MODULE_WIDTH - (float)uiOffsetX) / (float)pstJpegFrame->szJpegOut.uiImgW);
                    stSvaDspOut.target[k].rect.y = (float)(pstJpegFrame->szJpegOut.target[j].rect.y * SVA_MODULE_HEIGHT / (float)pstJpegFrame->szJpegOut.uiImgH);
                    stSvaDspOut.target[k].rect.width = (float)(pstJpegFrame->szJpegOut.target[j].rect.width * SVA_MODULE_WIDTH / (float)pstJpegFrame->szJpegOut.uiImgW);
                    stSvaDspOut.target[k].rect.height = (float)(pstJpegFrame->szJpegOut.target[j].rect.height * SVA_MODULE_HEIGHT / (float)pstJpegFrame->szJpegOut.uiImgH);
                }

                k++;
            }

            stSvaDspOut.target_num = k;
        }

        g_sva_common.stDbgPrm.stCommSts.uiJencSplitModeFlag = 0;     /* 标记 */

        SAL_getDateTime_tz(&stStreamEle.absTime);

        SVA_LOGD("time: %d-%d-%d %d-%d-%d:%d \n",
                 stStreamEle.absTime.year, stStreamEle.absTime.month, stStreamEle.absTime.day,
                 stStreamEle.absTime.hour, stStreamEle.absTime.minute, stStreamEle.absTime.second, stStreamEle.absTime.milliSecond);

        time8 = SAL_getCurMs();

        if (SAL_TRUE == stSvaDspOut.packbagAlert.package_valid
            && SVA_CAL_DIFF(u32LastTime[stStreamEle.chan], time8) > SVA_LAST_SPLIT_PIC_OVER_TIME_MS)
        {
            u32OverTimeCnt[stStreamEle.chan]++;
            (VOID)Sva_DrvFillOverTimeFrmInfo(stSvaDspOut.frame_num, &stOverTimeInfo[stStreamEle.chan]);
            SVA_LOGD("Last Split Pic cost overtime! %d, chan %d \n", SVA_CAL_DIFF(u32LastTime[stStreamEle.chan], time8), stStreamEle.chan);
        }

        u32LastTime[stStreamEle.chan] = time8;

        (VOID)Sva_DrvPrintDbgSplitInfo(stStreamEle.chan, u32OverTimeCnt[stStreamEle.chan], &stOverTimeInfo[stStreamEle.chan]);

        SVA_LOGI("to app: chan %d target_num %d Sync_0 %d, size[%d, %d, %d], time %d, frame_num %d pkgValid %d, fUp %f, fDown %f, split_start_flag %d, split_end_flag %d, bPkgSplit %d, pkg_id %d, match_id %d, %d, [fPkgfwd %f, time: %d-%d-%d %d-%d-%d:%d], pd [fPkgfwd %f, time: %d-%d-%d %d-%d-%d:%d]\n",
                 stStreamEle.chan,
                 stSvaDspOut.target_num,
                 stSvaDspOut.stJpegResult[0].uiSyncNum,
                 pstSva_dev->stJpegVenc.uiPicJpegSize[0],
                 pstSva_dev->stJpegVenc.uiPicJpegSize[1],
                 pstSva_dev->stJpegVenc.uiPicJpegSize[2],
                 time8,
                 stSvaDspOut.frame_num,
                 stSvaDspOut.packbagAlert.package_valid,
                 stSvaDspOut.fPkgUpGapPixel,
                 stSvaDspOut.fPkgDownGapPixel,
                 stSvaDspOut.packbagAlert.split_group_start_flag,
                 stSvaDspOut.packbagAlert.split_group_end_flag,
                 stSvaDspOut.packbagAlert.pkg_force_split_flag,
                 stSvaDspOut.packbagAlert.package_id,
                 stSvaDspOut.packbagAlert.package_match_id,
                 pstJpegFrame->szJpegOut.packbagAlert.package_id,
                 stSvaDspOut.stJpegPkgTime.fPkgfwd[0],
                 stSvaDspOut.stJpegPkgTime.absTime[0].year,
                 stSvaDspOut.stJpegPkgTime.absTime[0].month,
                 stSvaDspOut.stJpegPkgTime.absTime[0].day,
                 stSvaDspOut.stJpegPkgTime.absTime[0].hour,
                 stSvaDspOut.stJpegPkgTime.absTime[0].minute,
                 stSvaDspOut.stJpegPkgTime.absTime[0].second,
                 stSvaDspOut.stJpegPkgTime.absTime[0].milliSecond,
                 stSvaDspOut.stJpegPkgTime.fPkgfwd[1],
                 stSvaDspOut.stJpegPkgTime.absTime[1].year,
                 stSvaDspOut.stJpegPkgTime.absTime[1].month,
                 stSvaDspOut.stJpegPkgTime.absTime[1].day,
                 stSvaDspOut.stJpegPkgTime.absTime[1].hour,
                 stSvaDspOut.stJpegPkgTime.absTime[1].minute,
                 stSvaDspOut.stJpegPkgTime.absTime[1].second,
                 stSvaDspOut.stJpegPkgTime.absTime[1].milliSecond);

        time9 = SAL_getCurMs();

        g_sva_common.stDbgPrm.stCommSts.uiJencCbFlag = 1;     /* 标记 */
        SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stSvaDspOut, sizeof(SVA_DSP_OUT));
        time10 = SAL_getCurMs();

        if (time8 - time0 > 80 || time10 - time9 > 10)
        {
            SVA_LOGI("jenc cost: chan %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, total %d ms. \n", stStreamEle.chan,
                     time0, time1 - time0, time2 - time1, time3 - time2, time4 - time3,
                     time5 - time4, time6 - time5, time7 - time6, time8 - time7, time9 - time8,
                     time10 - time9, time10 - time0);
        }

REUSE:
        if ((SVA_JPEG_MASK_RECT & pstCapAiPrm->uiCbJpgNum)
            && (SVA_ORIENTATION_TYPE_L2R == enDirection))
        {
            s32Ret = Sva_DrvPutMirrorFrame(pstSva_dev);
            if (SAL_SOK != s32Ret)
            {
                SAL_ERROR("Put Mirror Frame Failed! chan %d \n", pstSva_dev->chan);
            }
        }

        g_sva_common.stDbgPrm.stCommSts.uiJencErrCnt += (uiSingleProcErrCnt != 0 ? 1 : 0);

putData:
        /* 回收数据 */
        DSA_QuePut(pstEmptQue, (void *)pstJpegFrame, SAL_TIMEOUT_NONE);
        g_sva_common.stDbgPrm.stCommSts.uiJencCbFlag = 0;     /* 标记 */
    }

    return NULL;
}

/*******************************************************************************
* 函数名  : Sva_DrvSaveOffset
* 描  述  : 保存帧偏移量到循环Buf中
* 输  入  : - chan     : 通道号
*         : - pOut     : 结果参数
*         : - pSysFrame: 帧数据
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static ATTRIBUTE_UNUSED INT32 Sva_DrvSaveOffset(UINT32 chan, void *pPrm)
{
    /* 入参有效性判断 */
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pPrm, SAL_FAIL);

    /* 定义变量 */
    /* INT32 s32Ret = SAL_SOK; */
    UINT32 uiCurBufIdx = 0;

    SVA_OFFSET_PRM_S *pstOffsetPrm = NULL;
    SVA_OFFSET_BUF_TAB_S *pstOffsetBufTab = NULL;

    /* 入参本地化 */
    pstOffsetPrm = (SVA_OFFSET_PRM_S *)pPrm;

    pstOffsetBufTab = Sva_DrvGetSvaOffsetBufTab(chan);
    if (NULL == pstOffsetBufTab)
    {
        SVA_LOGE("pstOffsetBufTab == NULL, chan %d \n", chan);
        return SAL_FAIL;
    }

    /* 将对应当前智能帧的帧间隔结果保存到全局buf表中，供显示模块调用 */
    uiCurBufIdx = pstOffsetBufTab->uiIdx;
    sal_memcpy_s(&pstOffsetBufTab->stOffsetPrm[uiCurBufIdx], sizeof(SVA_OFFSET_PRM_S), pstOffsetPrm, sizeof(SVA_OFFSET_PRM_S));

    /* 记录下一次的寻址索引值 */
    pstOffsetBufTab->uiIdx = (pstOffsetBufTab->uiIdx + 1) % SVA_OFFSET_BUF_LEN;     /* pstOffsetBufTab->uiMaxLen; */

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvDrawRectCPU
 * @brief:      调试在yuv上画框
 * @param[in]:  CHAR *pcFrmDataY
 * @param[in]:  UINT32 uiFrmW
 * @param[in]:  UINT32 uiFrmH
 * @param[in]:  UINT32 uiRectX
 * @param[in]:  UINT32 uiRectY
 * @param[in]:  UINT32 uiRectW
 * @param[in]:  UINT32 uiRectH
 * @param[out]: None
 * @return:     static void
 */
static VOID Sva_DrvDrawRectCPU(CHAR *pcFrmDataY, UINT32 uiFrmW, UINT32 uiFrmH,
                               UINT32 uiRectX, UINT32 uiRectY, UINT32 uiRectW, UINT32 uiRectH)
{
    UINT32 i = 0;

    CHAR *tmp1 = NULL;
    CHAR *tmp2 = NULL;

    if (uiRectX + uiRectW >= uiFrmW)
    {
        uiRectW = uiFrmW - 1 - uiRectX;
    }

    if (uiRectY + uiRectH >= uiFrmH)
    {
        uiRectH = uiFrmH - 1 - uiRectY;
    }

    tmp1 = pcFrmDataY + uiRectY * uiFrmW + uiRectX;
    tmp2 = pcFrmDataY + (uiRectY + uiRectH) * uiFrmW + uiRectX;

    for (i = 0; i < uiRectW; i++)
    {
        *tmp1 = *tmp2 = 0;
        tmp1++;
        tmp2++;
    }

    tmp1 = pcFrmDataY + uiRectY * uiFrmW + uiRectX;
    tmp2 = pcFrmDataY + uiRectY * uiFrmW + uiRectX + uiRectW;

    for (i = 0; i < uiRectH; i++)
    {
        *tmp1 = *tmp2 = 0;
        tmp1 += uiFrmW;
        tmp2 += uiFrmW;
    }

    return;
}

/**
 * @function:   Sva_DrvDebugDumpYuv
 * @brief:      调试导出画线的YUV数据
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_PROCESS_OUT *pstHalOut
 * @param[in]:  SYSTEM_FRAME_INFO *pstSysFrame
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Sva_DrvDebugDumpYuv(UINT32 chan, SVA_PROCESS_OUT *pstHalOut, SYSTEM_FRAME_INFO *pstSysFrame)
{
    UINT32 i = 0;
    UINT32 w = 0;
    UINT32 h = 0;
    UINT32 x = 0;
    UINT32 y = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    CHAR *cPath[XSI_DEV_MAX] = {"/mnt/nfs/hi3559a/xsi/0_dbg.yuv", "/mnt/nfs/hi3559a/xsi/1_dbg.yuv"};

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    if (!pstSva_dev->stDbgPrm.uiFlag)
    {
        strncpy(pstSva_dev->stDbgPrm.cPath, cPath[chan], strlen(pstSva_dev->stDbgPrm.cPath));
        pstSva_dev->stDbgPrm.uiFlag = SAL_TRUE;
    }

    if (0 == chan)
    {
        if (!pstSva_dev->stDbgPrm.pFile && pstSva_dev->stDbgPrm.uiDbgCnt >= pstSva_dev->stDbgPrm.uiDbgDumpNum)
        {
            SVA_LOGE("dbg end! chan %d \n", chan);
            return SAL_SOK;
        }

        if (pstSva_dev->stDbgPrm.pFile && pstSva_dev->stDbgPrm.uiDbgCnt >= pstSva_dev->stDbgPrm.uiDbgDumpNum)
        {
            fclose(pstSva_dev->stDbgPrm.pFile);
            pstSva_dev->stDbgPrm.pFile = NULL;

            return SAL_SOK;
        }

        if (!pstSva_dev->stDbgPrm.pFile)
        {
            pstSva_dev->stDbgPrm.pFile = fopen(pstSva_dev->stDbgPrm.cPath, "a+");
            if (!pstSva_dev->stDbgPrm.pFile)
            {
                SVA_LOGE("fopen failed! chan %d \n", chan);
                return SAL_FAIL;
            }
        }

        (VOID)sys_hal_getVideoFrameInfo(pstSysFrame, &stVideoFrmBuf);

        for (i = 0; i < pstHalOut->target_num; i++)
        {
            x = (UINT32)(pstHalOut->target[i].rect.x * (float)SVA_MODULE_WIDTH);
            y = (UINT32)(pstHalOut->target[i].rect.y * (float)SVA_MODULE_HEIGHT);
            w = (UINT32)(pstHalOut->target[i].rect.width * (float)SVA_MODULE_WIDTH);
            h = (UINT32)(pstHalOut->target[i].rect.height * (float)SVA_MODULE_HEIGHT);
            (VOID)Sva_DrvDrawRectCPU((VOID *)stVideoFrmBuf.virAddr[0], SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, x, y, w, h);
        }

        fwrite((VOID *)stVideoFrmBuf.virAddr[0], SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2, 1, pstSva_dev->stDbgPrm.pFile);
        fflush(pstSva_dev->stDbgPrm.pFile);

        pstSva_dev->stDbgPrm.uiDbgCnt++;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvChnMap
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static UINT32 Sva_DrvChnMap(SVA_PROC_MODE_E enProcMode, UINT32 uiChnId, UINT32 uiMainViewChn)
{
    return (SVA_PROC_MODE_DUAL_CORRELATION == enProcMode && 1 == uiMainViewChn ? 1 - uiChnId : uiChnId);
}

/**
 * @function    Sva_DrvGetOutputUsrInfo
 * @brief       获取算法回调的用户私有数据
 * @param[in]   XSIE_USER_PRVT_INFO *pstXsieUserPrvtInfo
 * @param[out]  SVA_ALG_RSLT_S *pstAlgRslt
 * @return
 */
static INT32 Sva_DrvGetOutputUsrInfo(XSIE_USER_PRVT_INFO *pstXsieUserPrvtInfo, SVA_ALG_RSLT_S *pstAlgRslt)
{
    UINT32 uiChnIdx = 0;
    UINT32 chan = 0;
    SVA_PROC_MODE_E enProcMode = 0;
    SVA_SYNC_RESULT_S *pstRslt = NULL;

    uiChnIdx = pstAlgRslt->uiChnCnt;
    if (uiChnIdx >= SVA_DEV_MAX)
    {
        SVA_LOGE("invalid chn idx %d > max %d \n", uiChnIdx, SVA_DEV_MAX);
        return SAL_FAIL;
    }

    /* 获取处理模式 */
    enProcMode = pstXsieUserPrvtInfo->mode;
    chan = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].devChn;

    pstRslt = &pstAlgRslt->stAlgChnRslt[uiChnIdx];

    /* 保存用户私有透传的处理模式 */
    pstRslt->enProcMode = pstXsieUserPrvtInfo->mode;
    pstRslt->chan = chan;

    pstRslt->stProcOut.frame_sync_idx = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].syncIdx;
    pstRslt->stProcOut.uiTimeRef = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].timeRef;
    pstRslt->stProcOut.frame_stamp = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].pts;

    /*兼容双路图片模式*/
    if (SVA_PROC_MODE_IMAGE == enProcMode)
    {
        pstRslt->stProcOut.uiOffsetX = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].offsetX;
        pstRslt->stProcOut.uiImgW = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].imgW;
        pstRslt->stProcOut.uiImgH = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].imgH;
        pstRslt->stProcOut.aipackinfo.packIndx = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].stAiPrm.packIndx;
        pstRslt->stProcOut.aipackinfo.packTop = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].stAiPrm.packTop;
        pstRslt->stProcOut.aipackinfo.packBottom = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].stAiPrm.packBottom;
        pstRslt->stProcOut.aipackinfo.completePackMark = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].stAiPrm.completePackMark;
        pstRslt->stProcOut.uiTimeRef = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].stAiPrm.timeRef;

        SVA_LOGD("chan %d: uiChnIdx %d frame_sync_idx %d, uiTimeRef %d, frame_stamp %llu, uiOffsetX %d, packIndx %d completePackMark %d\n",
                 pstRslt->chan, uiChnIdx, pstRslt->stProcOut.frame_sync_idx, pstRslt->stProcOut.uiTimeRef,
                 pstRslt->stProcOut.frame_stamp, pstRslt->stProcOut.uiOffsetX, pstRslt->stProcOut.aipackinfo.packIndx, \
                 pstRslt->stProcOut.aipackinfo.completePackMark);
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvProcOffset
 * @brief       计算偏移量
 * @param[in]   SVA_ALG_RSLT_S *pstAlgRslt
 * @param[out]  SVA_ALG_RSLT_S *pstAlgRslt
 * @return
 */
static INT32 Sva_DrvProcOffset(UINT32 uiDirection, SVA_ALG_RSLT_S *pstAlgRslt)
{
    UINT32 uiChnIdx = 0;
    UINT32 uiBpFlag = 0;
    UINT32 uiFpFlag = 0;

    SVA_SYNC_RESULT_S *pstRslt = NULL;

    /* checker */
    if (NULL == pstAlgRslt)
    {
        SVA_LOGE("ptr null! %p \n", pstAlgRslt);
        return SAL_FAIL;
    }

    uiChnIdx = pstAlgRslt->uiChnCnt;
    if (uiChnIdx >= SVA_DEV_MAX)
    {
        SVA_LOGE("invalid chn idx %d > max %d \n", uiChnIdx, SVA_DEV_MAX);
        return SAL_FAIL;
    }

    pstRslt = &pstAlgRslt->stAlgChnRslt[uiChnIdx];

    if (pstRslt->stProcOut.frame_offset < 0)
    {
        SVA_LOGD(" frm_offset %f, TimRef %u, frm_num %u \n",
                 pstRslt->stProcOut.frame_offset, pstRslt->stProcOut.uiTimeRef, pstRslt->stProcOut.frame_num);
    }

    /* 判断是否为回拉或前拉状态，若是将目标数置为0 */
    (VOID)Sva_HalJudgeBackPull(0, &pstRslt->stProcOut, &uiBpFlag);
    (VOID)Sva_HalJudgeForwPull(0, &pstRslt->stProcOut, &uiFpFlag);

    if ((SAL_TRUE == uiBpFlag)
        || (SAL_TRUE == uiFpFlag))
    {
        pstRslt->stProcOut.bFastMovValid = SAL_TRUE;
        SVA_LOGW("result cleared!, frame_offset %f, uiBpFlag %d, uiFpFlag %d \n",
                 pstRslt->stProcOut.frame_offset, uiBpFlag, uiFpFlag);
    }

    if (SAL_TRUE == pstRslt->stProcOut.bFastMovValid)
    {
        pstRslt->stProcOut.target_num = 0;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvGetOutputFrm
 * @brief       获取算法回调返回的帧数据信息
 * @param[in]   VOID *pstRawAlgOutput
 * @param[out]  SVA_ALG_RSLT_S *pstAlgRslt
 * @return
 */
static INT32 Sva_DrvGetOutputFrm(XSIE_SECURITY_XSI_OUT_T *pstXsieOut,
                                 XSIE_USER_PRVT_INFO *pstXsieUserPrvtInfo,
                                 SVA_ALG_RSLT_S *pstAlgRslt)
{
    UINT32 uiChnIdx = 0;
    UINT32 chan = 0;
    SVA_PROC_MODE_E enProcMode = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_SYNC_RESULT_S *pstRslt = NULL;
    XSIE_SECURITY_INPUT_INFO_T *pstXsiAddInfo = NULL;

    VOID *pImgVirAddr = 0;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    uiChnIdx = pstAlgRslt->uiChnCnt;
    if (uiChnIdx >= SVA_DEV_MAX)
    {
        SVA_LOGE("invalid chn idx %d > max %d \n", uiChnIdx, SVA_DEV_MAX);
        return SAL_FAIL;
    }

    pstRslt = &pstAlgRslt->stAlgChnRslt[uiChnIdx];

    /* 获取处理模式 */
    enProcMode = pstXsieUserPrvtInfo->mode;

    /* 获取帧号 */
    if (SAL_TRUE != IA_GetModTransFlag(IA_MOD_SVA))
    {
        pstRslt->stProcOut.frame_num = pstXsieOut->XsiOutInfo.FrameNum; /* pstInputYuvData->stInputInfo.stBlobData[0].nFrameNum; */
    }
    else
    {
        /*跨芯片传输时使用私有信息的真实帧号*/
#ifdef DSP_ISM
        pstRslt->stProcOut.frame_num = pstXsieOut->XsiOutInfo.FrameNum;
#else
        pstRslt->stProcOut.frame_num = pstXsieUserPrvtInfo->nRealFrameNum;
#endif
    }

    pstSva_dev = Sva_DrvGetDev(pstRslt->chan);
    SVA_DRV_CHECK_PTR(pstSva_dev, err, "pstSva_dev == null!");

    if (0x00 == pstSva_dev->stSysFrame.uiAppData)
    {
        pstSva_dev->stSysFrame.uiDataAddr = 0;
        pstSva_dev->stSysFrame.uiDataHeight = 0;
        pstSva_dev->stSysFrame.uiDataWidth = 0;
        pstSva_dev->stSysFrame.uiDataLen = 0;
        if (SAL_SOK != sys_hal_allocVideoFrameInfoSt(&pstSva_dev->stSysFrame))
        {
            SVA_LOGE("alloc video frame info st failed! \n");
            return SAL_FAIL;
        }
    }

    /* 非关联模式下，以主通道的数据为准；关联模型下需区分主、侧通道的图像帧数据 */
    pstXsiAddInfo = (XSIE_SECURITY_INPUT_INFO_T *)&pstXsieOut->XsiAddInfo;

    if (SVA_PROC_MODE_DUAL_CORRELATION != g_proc_mode || (pstRslt->chan == g_sva_common.uiMainViewChn))
    {
        pImgVirAddr = pstXsiAddInfo->MainYuvData[0].y;
    }
    else
    {
        pImgVirAddr = pstXsiAddInfo->SideYuvData[0].y;
    }

    stVideoFrmBuf.phyAddr[0] = 0x00;        /* 物理地址在平台抽象层中进行获取 */
    stVideoFrmBuf.phyAddr[1] = 0x00;
    stVideoFrmBuf.phyAddr[2] = 0x00;
    stVideoFrmBuf.virAddr[0] = (PhysAddr)pImgVirAddr;
    stVideoFrmBuf.virAddr[1] = (PhysAddr)(stVideoFrmBuf.virAddr[0] + SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT);
    stVideoFrmBuf.virAddr[2] = stVideoFrmBuf.virAddr[1];
    stVideoFrmBuf.frameParam.width = SVA_MODULE_WIDTH;
    stVideoFrmBuf.frameParam.height = SVA_MODULE_HEIGHT;
    stVideoFrmBuf.stride[0] = SVA_MODULE_WIDTH;
    stVideoFrmBuf.stride[1] = SVA_MODULE_WIDTH;
    stVideoFrmBuf.stride[2] = SVA_MODULE_WIDTH;
    stVideoFrmBuf.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;
    /* stVideoFrmBuf.frameNum = uiTimeRef[0]; */

#if 0
    /*down下图片，对比智能结果，检查是否正确*/
    INT8 framename[64] = {0};
    sprintf((CHAR *)framename, "dumpdata/%d_%d_masterresult.yuv", pstXsieOut->XsiOutInfo.FrameNum, pstXsieUserPrvtInfo->stUsrPrm[0].devChn);
    Sva_HalDebugDumpData((CHAR *)pstXsiAddInfo->MainYuvData[0].y, (CHAR *)framename, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT * 3 / 2, 0);
#endif

#if 0  /* dump data */
    if (g_dumpDataCnt > 0)
    {
        CHAR acPath[64] = {'\0'};
        sprintf(acPath, "%s/%d_%d_%s", "./xsi", uiChnIdx, pstRslt->stProcOut.frame_num, "cb.nv21");

        Ia_DumpYuvData(acPath, (VOID *)stVideoFrmBuf.virAddr[0], SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT);
        g_dumpDataCnt--;
    }

#endif

    if (SVA_PROC_MODE_IMAGE == enProcMode)
    {
        chan = pstXsieUserPrvtInfo->stUsrPrm[uiChnIdx].devChn;
        stVideoFrmBuf.privateDate = (PhysAddr)pstXsieUserPrvtInfo->stUsrPrm[chan].syncPts;

        pstSva_dev->stSysFrame.uiDataWidth = pstXsieUserPrvtInfo->stUsrPrm[chan].imgW;
        pstSva_dev->stSysFrame.uiDataHeight = pstXsieUserPrvtInfo->stUsrPrm[chan].imgH;
        pstSva_dev->stSysFrame.uiDataAddr = pstXsieUserPrvtInfo->stUsrPrm[chan].offsetX;
    }
    else
    {
        pstSva_dev->stSysFrame.uiDataWidth = SVA_MODULE_WIDTH;
        pstSva_dev->stSysFrame.uiDataHeight = SVA_MODULE_HEIGHT;
    }

    if (SAL_SOK != sys_hal_buildVideoFrame(&stVideoFrmBuf, &pstSva_dev->stSysFrame))
    {
        SVA_LOGE("build video frame failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Sva_DrvDumpPosYuv
 * @brief      dump pos
 * @param[in]  UINT32 chan
 * @param[in]  XSIE_SECURITY_XSI_POS_INFO_T *pstXsiPosInfo
 * @param[in]  XSIE_SECURITY_INPUT_INFO_T *pstXsiAddInfo
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvDumpPosYuv(UINT32 chan,
                               XSIE_SECURITY_XSI_POS_INFO_T *pstXsiPosInfo,
                               XSIE_SECURITY_INPUT_INFO_T *pstXsiAddInfo)
{
    UINT32 time[16] = {0};

    STREAM_ELEMENT stStreamEle = {0};
    SVA_DSP_POS_OUT stSvaDspPosOut = {0};

    static char *p = NULL;
    static unsigned int uiPosProcCnt = 0;
    static unsigned int uiPosProcCostTime = 0;

    time[0] = SAL_getCurMs();

    if (!p)
    {
        p = SAL_memMalloc(SVA_POS_YUV_SIZE, "SVA", "sva_posyuv");
        if (!p)
        {
            SVA_LOGE("pos: malloc failed! size %d \n", SVA_POS_YUV_SIZE);
            return SAL_FAIL;
        }
    }

    time[2] = SAL_getCurMs();

    /* 分段存储，先存储原视频Y */
    memcpy(p, pstXsiAddInfo->MainYuvData[0].y, SVA_ALG_Y_SIZE);

    /* 分段存储，再存储XSIE_POS前半部分 */
    memcpy(p + SVA_ALG_Y_SIZE, (char *)pstXsiPosInfo->XsiePosBuf, XSIE_POS_FIRST_SIZE);

    /* 再存储原视频UV部分 */
    memcpy(p + SVA_ALG_Y_SIZE + XSIE_POS_FIRST_SIZE, (char *)pstXsiAddInfo->MainYuvData[0].u, SVA_ALG_UV_SIZE);

    /* 再储存XSIE_POS后半部分 */
    memcpy(p + SVA_ALG_YUV_SIZE + XSIE_POS_FIRST_SIZE, (char *)pstXsiPosInfo->XsiePosBuf + XSIE_POS_FIRST_SIZE, XSIE_POS_SECOND_SIZE);

#if 0
    static FILE *fp = NULL;
    static int flag = 1;
    char *path = "dump_yuv.nv21";

    if (!fp && flag)
    {
        fp = fopen(path, "a+");
        if (NULL == fp)
        {
            SVA_LOGE("fopen failed! %s \n", path);
            return SAL_FAIL;
        }

        SVA_LOGI("fopen success! %s \n", path);
    }

    if (fp)
    {
        fwrite(pstXsiAddInfo->MainYuvData[0].y, SVA_ALG_YUV_SIZE, 1, fp);
        fflush(fp);
    }

    if (uiPosProcCnt >= 10 && fp)
    {
        fclose(fp);
        fp = NULL;

        flag = 0;
    }

#endif

    time[3] = SAL_getCurMs();

    /* 填充回调数据类型 */
    stStreamEle.chan = chan;
    stStreamEle.type = STREAM_ELEMENT_SVA_POS;

    /* 填充回调数据地址和长度信息 */
    stSvaDspPosOut.pcPosData = p;
    stSvaDspPosOut.uiDataLen = SVA_POS_YUV_SIZE;
    SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stSvaDspPosOut, sizeof(SVA_DSP_POS_OUT));

    time[4] = SAL_getCurMs();

    uiPosProcCnt++;
    uiPosProcCostTime += time[4] - time[0];

    if (time[4] - time[0] > 15)
    {
        SVA_LOGW("dump pos cost %d > 15 ms! %d, %d, %d \n", time[4] - time[0],
                 time[2] - time[0], time[3] - time[2], time[4] - time[3]);
    }

    /* debug: print average time costumed by this interface every 60 frame */
    if (uiPosProcCnt && uiPosProcCnt % 60 == 0)
    {
        SVA_LOGI("uiPosProcCnt %d, ave: %f ms! \n", uiPosProcCnt, (float)uiPosProcCostTime / (float)uiPosProcCnt);
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvGetOutputPos
 * @brief       获取算法回调的pos信息
 * @param[in]   XSIE_SECURITY_XSI_OUT_T *pstXsieOut
 * @param[out]  SVA_ALG_RSLT_S *pstAlgRslt
 * @return
 */
static INT32 Sva_DrvGetOutputPos(XSIE_SECURITY_XSI_OUT_T *pstXsieOut, SVA_ALG_RSLT_S *pstAlgRslt)
{
    UINT32 uiChnIdx = 0;

    /* 通过开关控制pos信息 */
    if (guiPosWriteFlag == SAL_FALSE)
    {
        return SAL_SOK;
    }

    uiChnIdx = pstAlgRslt->uiChnCnt;
    if (uiChnIdx >= SVA_DEV_MAX)
    {
        SVA_LOGE("invalid chn idx %d > max %d \n", uiChnIdx, SVA_DEV_MAX);
        return SAL_FAIL;
    }

    /* debug dump pos */
    Sva_DrvDumpPosYuv(pstAlgRslt->uiChnCnt, &pstXsieOut->XsiPosInfo, &pstXsieOut->XsiAddInfo);

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvCheckRectAndPrint
 * @brief:      校验区域参数有效性并返回是否打印的标志
 * @param[in]:  SVA_RECT_F *pstRect
 * @param[out]: None
 * @return:     BOOL
 */
static BOOL Sva_DrvCheckRectAndPrint(SVA_RECT_F *pstRect)
{
    BOOL bPrint = SAL_FALSE;

    if (pstRect->x >= 1.0)
    {
        pstRect->x = 0.998;     /* 2.5 pixels */
        bPrint = SAL_TRUE;
    }
    else if (pstRect->x < 0.0)
    {
        pstRect->x = 0.002;
        bPrint = SAL_TRUE;
    }

    if (pstRect->x + pstRect->width > 1.0)
    {
        pstRect->width = pstRect->x < 0.998 ? 0.998 - pstRect->x : 0.0;
        bPrint = SAL_TRUE;
    }

    if (pstRect->y >= 1.0)
    {
        pstRect->y = 0.998;     /* 2.5 pixels */
        bPrint = SAL_TRUE;
    }
    else if (pstRect->y < 0.0)
    {
        pstRect->y = 0.002;
        bPrint = SAL_TRUE;
    }

    if (pstRect->y + pstRect->height > 1.0)
    {
        pstRect->height = pstRect->y < 0.998 ? 0.998 - pstRect->y : 0.0;
        bPrint = SAL_TRUE;
    }

    return bPrint;
}

/*******************************************************************************
* 函数名  : Sva_DrvPrintResultCb
* 描  述  : 打印回调结果(调试使用)
* 输  入  : pstXsiOutInfo: 算法返回的报警信息
*         : pstXsiAddInfo: 算法返回的附加信息，包括主侧视角的数据
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static void Sva_DrvPrintResultCb(UINT32 chan, XSIE_SECURITY_XSI_INFO_T *pstXsiOutInfo, XSIE_SECURITY_INPUT_INFO_T *pstXsiAddInfo)
{
    UINT32 i = 0;

#ifndef DSP_ISM
    return;
#endif
	SVA_LOGI("engine cb: chan %d, frame_num %d, main_view_target_num %d, side_view_target_num %d \n",
			 chan, pstXsiOutInfo->FrameNum, pstXsiOutInfo->Alert.MainViewTargetNum, pstXsiOutInfo->Alert.SideViewTargetNum);

    if (0 == g_bPrOutInfo || 2 == g_bPrOutInfo)
    {
        return;
    }

    SVA_LOGW("=================== result ======================= \n");
    SVA_LOGW("engine cb: chan %d, frame_num %d, main_view_target_num %d, side_view_target_num %d \n",
             chan, pstXsiOutInfo->FrameNum, pstXsiOutInfo->Alert.MainViewTargetNum, pstXsiOutInfo->Alert.SideViewTargetNum);

    SVA_LOGW("engine cb: pts %d, candidate_flag %d, frame_offset %.4f \n",
             pstXsiOutInfo->TimeStamp, pstXsiOutInfo->Alert.CandidateFlag, pstXsiOutInfo->Alert.FrameOffset);

    SVA_LOGW("=================== Add Info ======================= \n");
    SVA_LOGW("szl_dbg: Main Yuv Data Info: format %d, w,h [%d,%d], pitch_y,pitch_uv [%d,%d], scale_rate %d \n",
             pstXsiAddInfo->MainYuvData[0].format, pstXsiAddInfo->MainYuvData[0].image_w, pstXsiAddInfo->MainYuvData[0].image_h, 
             pstXsiAddInfo->MainYuvData[0].pitch_y, pstXsiAddInfo->MainYuvData[0].pitch_uv, 
             pstXsiAddInfo->MainYuvData[0].scale_rate);

    SVA_LOGW("szl_dbg: Side Yuv Data Info: format %d, w,h [%d,%d], pitch_y,pitch_uv [%d,%d], scale_rate %d \n",
             pstXsiAddInfo->SideYuvData[0].format, pstXsiAddInfo->SideYuvData[0].image_w, pstXsiAddInfo->SideYuvData[0].image_h,
             pstXsiAddInfo->SideYuvData[0].pitch_y, pstXsiAddInfo->SideYuvData[0].pitch_uv,
             pstXsiAddInfo->SideYuvData[0].scale_rate);

    SVA_LOGW("=================== Main ========================= \n");
    SVA_LOGW("szl_dbg: main_view_pack_info--x %.4f, y %.4f, w %.4f, h %.4f \n",
             pstXsiOutInfo->Alert.MainViewPackageLoc[0].PackageRect.x,
             pstXsiOutInfo->Alert.MainViewPackageLoc[0].PackageRect.y,
             pstXsiOutInfo->Alert.MainViewPackageLoc[0].PackageRect.width,
             pstXsiOutInfo->Alert.MainViewPackageLoc[0].PackageRect.height);

    SVA_LOGW("engine cb: main_view_target_num %d \n", pstXsiOutInfo->Alert.MainViewTargetNum);
    for (i = 0; i < pstXsiOutInfo->Alert.MainViewTargetNum; i++)
    {
        SVA_LOGW("szl_dbg: i %d, ID %d, type %d, alarm_flg %d, confidence %.4f, visual_confidence %.4f \n",
                 i,
                 pstXsiOutInfo->Alert.MainViewTarget[i].ID,
                 pstXsiOutInfo->Alert.MainViewTarget[i].Type,
                 pstXsiOutInfo->Alert.MainViewTarget[i].AlarmFlg,
                 pstXsiOutInfo->Alert.MainViewTarget[i].Confidence,
                 pstXsiOutInfo->Alert.MainViewTarget[i].VisualConfidence);
        SVA_LOGW("szl_dbg: i %d, x %.4f, y %.4f, w %.4f, h %.4f \n",
                 i,
                 pstXsiOutInfo->Alert.MainViewTarget[i].Rect.x,
                 pstXsiOutInfo->Alert.MainViewTarget[i].Rect.y,
                 pstXsiOutInfo->Alert.MainViewTarget[i].Rect.width,
                 pstXsiOutInfo->Alert.MainViewTarget[i].Rect.height);
    }

    SVA_LOGW("=================== Side ========================= \n");
    SVA_LOGW("szl_dbg: side_view_pack_info--x %.4f, y %.4f, w %.4f, h %.4f \n",
             pstXsiOutInfo->Alert.SideViewPackageLoc[0].PackageRect.x,
             pstXsiOutInfo->Alert.SideViewPackageLoc[0].PackageRect.y,
             pstXsiOutInfo->Alert.SideViewPackageLoc[0].PackageRect.width,
             pstXsiOutInfo->Alert.SideViewPackageLoc[0].PackageRect.height);

    SVA_LOGW("szl_dbg: side_view_target_num %d \n", pstXsiOutInfo->Alert.SideViewTargetNum);
    for (i = 0; i < pstXsiOutInfo->Alert.SideViewTargetNum; i++)
    {
        SVA_LOGW("szl_dbg: i %d, ID %d, type %d, alarm_flg %d, confidence %.4f, visual_confidence %.4f \n",
                 i,
                 pstXsiOutInfo->Alert.SideViewTarget[i].ID,
                 pstXsiOutInfo->Alert.SideViewTarget[i].Type,
                 pstXsiOutInfo->Alert.SideViewTarget[i].AlarmFlg,
                 pstXsiOutInfo->Alert.SideViewTarget[i].Confidence,
                 pstXsiOutInfo->Alert.SideViewTarget[i].VisualConfidence);
        SVA_LOGW("szl_dbg: i %d, x %.4f, y %.4f, w %.4f, h %.4f \n",
                 i,
                 pstXsiOutInfo->Alert.SideViewTarget[i].Rect.x,
                 pstXsiOutInfo->Alert.SideViewTarget[i].Rect.y,
                 pstXsiOutInfo->Alert.SideViewTarget[i].Rect.width,
                 pstXsiOutInfo->Alert.SideViewTarget[i].Rect.height);
    }

    return;
}

/**
 * @function    Sva_DrvViewTypeMap
 * @brief       业务层通道索引映射主侧视角结构体
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvViewTypeMap(IN UINT32 uiChnIdx,
                                IN XSIE_SECURITY_ALERT_T *pstXsiAlertInfo,
                                OUT UINT32 *puiPkgCnt,
                                OUT XSIE_PACKAGE_T **ppastPkgInfo,
                                OUT UINT32 *puiPkgDetCnt,
                                OUT XSIE_PACKAGE_DET_T **ppastPkgDetInfo,
                                OUT UINT32 *puiTarCnt,
                                OUT XSIE_SECURITY_ALERT_TARGET_T **ppastTarInfo)
{
    BOOL bMainView = SAL_TRUE;

    if (uiChnIdx >= SVA_DEV_MAX)
    {
        SVA_LOGE("invalid chn idx %d > max %d \n", uiChnIdx, SVA_DEV_MAX);
        return SAL_FAIL;
    }

    bMainView = ((uiChnIdx == 0) ? SAL_TRUE : SAL_FALSE);

    if (bMainView)
    {
        /* 获取包裹分割结果 */
        if (NULL != puiPkgCnt
            && NULL != ppastPkgInfo)
        {
            *puiPkgCnt = (pstXsiAlertInfo->MainViewPackageNum > SVA_PACKAGE_MAX_NUM ? SVA_PACKAGE_MAX_NUM : pstXsiAlertInfo->MainViewPackageNum);
            *ppastPkgInfo = &pstXsiAlertInfo->MainViewPackageLoc[0];
        }

        /* 获取包裹检测结果 */
        if (NULL != puiPkgDetCnt
            && NULL != ppastPkgDetInfo)
        {
            *puiPkgDetCnt = (pstXsiAlertInfo->MainViewPackDetNum > SVA_DET_PACKAGE_MAX_NUM ? SVA_DET_PACKAGE_MAX_NUM : pstXsiAlertInfo->MainViewPackDetNum);
            *ppastPkgDetInfo = &pstXsiAlertInfo->MainViewPackDet[0];
        }

        /* 获取目标检测结果 */
        if (NULL != puiTarCnt
            && NULL != ppastTarInfo)
        {
            *puiTarCnt = pstXsiAlertInfo->MainViewTargetNum;
            *ppastTarInfo = &pstXsiAlertInfo->MainViewTarget[0];
        }
    }
    else
    {
        /* 获取包裹分割结果 */
        if (NULL != puiPkgCnt
            && NULL != ppastPkgInfo)
        {
            *puiPkgCnt = (pstXsiAlertInfo->SideViewPackageNum > SVA_PACKAGE_MAX_NUM ? SVA_PACKAGE_MAX_NUM : pstXsiAlertInfo->SideViewPackageNum);
            *ppastPkgInfo = &pstXsiAlertInfo->SideViewPackageLoc[0];
        }

        /* 获取包裹检测结果 */
        if (NULL != puiPkgDetCnt
            && NULL != ppastPkgDetInfo)
        {
            *puiPkgDetCnt = (pstXsiAlertInfo->SideViewPackDetNum > SVA_DET_PACKAGE_MAX_NUM ? SVA_DET_PACKAGE_MAX_NUM : pstXsiAlertInfo->SideViewPackDetNum);
            *ppastPkgDetInfo = &pstXsiAlertInfo->SideViewPackDet[0];
        }

        /* 获取目标检测结果 */
        if (NULL != puiTarCnt
            && NULL != ppastTarInfo)
        {
            *puiTarCnt = pstXsiAlertInfo->SideViewTargetNum;
            *ppastTarInfo = &pstXsiAlertInfo->SideViewTarget[0];
        }
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvGetTarInfoFromEng
 * @brief       从引擎接口获取目标信息
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvGetTarInfoFromEng(IN UINT32 uiTarCnt,
                                     IN XSIE_SECURITY_ALERT_TARGET_T *pastTarInfo,
                                     IN UINT32 uiDirection,
                                     OUT SVA_PROCESS_OUT *pstProcOut)
{
    UINT32 i = 0;
    CAPB_PRODUCT *pstProductPrm = capb_get_product();    /* 获取能力级，用于区分安检机还是分析仪 */

    if (NULL == pstProductPrm)
    {
        SVA_LOGE("get platform capbility fail\n");
        return;
    }

    pstProcOut->target_num = uiTarCnt >= SVA_XSI_MAX_ALARM_NUM ? SVA_XSI_MAX_ALARM_NUM : uiTarCnt;

    for (i = 0; i < pstProcOut->target_num; i++)
    {
        pstProcOut->target[i].ID = pastTarInfo[i].ID;
        pstProcOut->target[i].type = pastTarInfo[i].Type;
        pstProcOut->target[i].alarm_flg = pastTarInfo[i].AlarmFlg;
        pstProcOut->target[i].confidence = (UINT32)pastTarInfo[i].Confidence;
        pstProcOut->target[i].visual_confidence = pstProcOut->target[i].confidence / 10;

        pstProcOut->target[i].rect.x = pastTarInfo[i].Rect.x;
        pstProcOut->target[i].rect.y = pastTarInfo[i].Rect.y;
        pstProcOut->target[i].rect.width = pastTarInfo[i].Rect.width;
        pstProcOut->target[i].rect.height = pastTarInfo[i].Rect.height;

        /* 若是从左到右过包需要进行坐标镜像 */
        if (VIDEO_INPUT_OUTSIDE == pstProductPrm->enInputType
            && 1 == uiDirection)
        {
            pstProcOut->target[i].rect.x = 1.0 - pstProcOut->target[i].rect.x - pstProcOut->target[i].rect.width;
        }
    }

    return;
}

#if 1  /* 包裹融合调试接口 */

/**
 * @function    pr_sva_pkg_info
 * @brief       打印sva全局包裹信息
 * @param[in]
 * @param[out]
 * @return
 */
static void pr_sva_pkg_info(SVA_PROCESS_OUT *pstProcOut)
{
    return;   /* 默认关闭 */

    UINT32 i = 0;
    for (i = 0; i < pstProcOut->packbagAlert.MainViewPackageNum; i++)
    {
        SVA_LOGW("pr_sva: i %d, [%f, %f] [%f, %f] \n",
                 i, pstProcOut->packbagAlert.MainViewPackageLoc[i].PackageRect.x,
                 pstProcOut->packbagAlert.MainViewPackageLoc[i].PackageRect.y,
                 pstProcOut->packbagAlert.MainViewPackageLoc[i].PackageRect.width,
                 pstProcOut->packbagAlert.MainViewPackageLoc[i].PackageRect.height);
    }

    return;
}

/**
 * @function    pr_sva_valid_pkg_info
 * @brief       打印sva valid pkg信息
 * @param[in]
 * @param[out]
 * @return
 */
static void pr_sva_valid_pkg_info(SVA_PKG_VALID_INFO_S *pstValidPkgInfo)
{
    return;   /* 默认关闭 */

    int i;
    if (SAL_TRUE != pstValidPkgInfo->bDataValid)
    {
        SVA_LOGE("not valid pkg buf! \n");
        return;
    }

    SVA_LOGE("enPkgValidType %d, uiPkgLogicId %d, uiMaxSubPkgId %d \n",
             pstValidPkgInfo->enPkgValidType, pstValidPkgInfo->uiPkgLogicId, pstValidPkgInfo->uiMaxSubPkgId);

    for (i = 0; i < pstValidPkgInfo->uiPkgLogicCnt; i++)
    {
        SVA_LOGW("pr_sva_valid: i %d, id %d, exist %d \n", i, pstValidPkgInfo->auiPkgID[i], pstValidPkgInfo->abPkgExist[i]);
    }

    return;
}

/**
 * @function    debug_pr_all_pkg_info
 * @brief       调试接口，打印所有包裹信息
 * @param[in]
 * @param[out]
 * @return
 */
void debug_pr_all_pkg_info(UINT32 uiPkgCnt, XSIE_PACKAGE_T *pastPkgInfo, UINT32 uiPkgDetCnt, XSIE_PACKAGE_DET_T *pastPkgDetInfo)
{
    return;   /* 默认关闭 */

    int i = 0;
    for (i = 0; i < uiPkgCnt; i++)
    {
        SVA_LOGW("id %d, valid %d, life %d, IsHistoryPackage %d, fwd %f, [%f, %f] [%f, %f] \n",
                 pastPkgInfo[i].PakageID,
                 pastPkgInfo[i].PackageValid,
                 pastPkgInfo[i].PackLifeFrame,
                 pastPkgInfo[i].IsHistoryPackage,
                 pastPkgInfo[i].PackageForwardLocat,
                 pastPkgInfo[i].PackageRect.x,
                 pastPkgInfo[i].PackageRect.y,
                 pastPkgInfo[i].PackageRect.width,
                 pastPkgInfo[i].PackageRect.height);
    }

    for (i = 0; i < uiPkgDetCnt; i++)
    {
        SVA_LOGE("det: id %d, proc %d, frm %d, packValid %d, dataValid %d, [%f, %f] [%f, %f] \n",
                 pastPkgDetInfo[i].PakageID,
                 pastPkgDetInfo[i].PackDetProc,
                 pastPkgDetInfo[i].PackFrameNum,
                 pastPkgDetInfo[i].PackValid,
                 pastPkgDetInfo[i].DataValid,
                 pastPkgDetInfo[i].PackageRect.x,
                 pastPkgDetInfo[i].PackageRect.y,
                 pastPkgDetInfo[i].PackageRect.width,
                 pastPkgDetInfo[i].PackageRect.height);
    }

    return;
}

#endif

/**
 * @function    Sva_DrvUpCombPkgLoc
 * @brief       更新包裹融合位置信息
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvUpCombPkgLoc(IN UINT32 chan,
                                 IN SVA_PKG_VALID_INFO_S *pstValidPkgInfo, /* 注意!!!此idx为包裹融合缓存数组中的读写索引值，非包裹id */
                                 OUT SVA_PROCESS_OUT *pstProcOut)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 uiPkgNoTarFlag = 0;
    BOOL bFindValidSubPkgFlag = SAL_FALSE;

    float fTmpMinX = 1.0;
    float fTmpMaxX = 0.0;
    float fTmpMinY = 1.0;
    float fTmpMaxY = 0.0;

    float fTmp = 0.0;

    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_PTR(pstProcOut, err, "pstProcOut == null!");

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PTR(pstSva_dev, err, "pstSva_dev == null!");

    SVA_LOGD("up comm: cnt %d \n", pstValidPkgInfo->uiPkgLogicCnt);

    uiPkgNoTarFlag = 1;
    pstProcOut->packbagAlert.up_pkg_idx = 0;  /* 复用: up_pkg_idx用于记录合并后的包裹id，用于更新外部包裹id */

    for (i = 0; i < pstValidPkgInfo->uiPkgLogicCnt; i++)
    {
        if (i >= SVA_PACKAGE_MAX_NUM)
        {
            SVA_LOGE("szl_dbg: invalid pkg logic cnt %d, i %d \n", pstValidPkgInfo->uiPkgLogicCnt, i);
            continue;
        }

        /* 包裹消失则跳过 */
        if (SAL_TRUE != pstValidPkgInfo->abPkgExist[i])
        {
            SVA_LOGD("pkg not exist! i %d, id %d \n", i, pstValidPkgInfo->auiPkgID[i]);

            uiPkgNoTarFlag &= 1;
            continue;
        }

        uiPkgNoTarFlag &= 0;

        for (j = 0; j < pstProcOut->packbagAlert.MainViewPackageNum; j++)
        {
            if (pstValidPkgInfo->auiPkgID[i] == pstProcOut->packbagAlert.MainViewPackageLoc[j].PakageID)
            {
                break;
            }
        }

        /* 若包裹已消失则返回失败 */
        if (j >= pstProcOut->packbagAlert.MainViewPackageNum)
        {
            pr_sva_valid_pkg_info(pstValidPkgInfo);

            SVA_LOGD("NOT FOUND pkdID %d, cnt %d, i %d \n",
                     pstValidPkgInfo->auiPkgID[i], pstValidPkgInfo->uiPkgLogicCnt, i);

            pstValidPkgInfo->abPkgExist[i] = SAL_FALSE;
            continue;
        }

#if 0  /* 误检包裹无法过滤，但该代码保留供参考 */
        if ((pstProcOut->packbagAlert.MainViewPackageLoc[j].PakageID < pstValidPkgInfo->uiPkgLogicId)
            && !pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageValid
            && !pstProcOut->packbagAlert.MainViewPackageLoc[j].IsHistoryPackage)
        {
            SVA_LOGW("suspicious pkg! id %d, logicId %d, cnt %d, max %d \n",
                     pstProcOut->packbagAlert.MainViewPackageLoc[j].PakageID,
                     pstValidPkgInfo->uiPkgLogicId,
                     pstValidPkgInfo->uiPkgLogicCnt,
                     pstValidPkgInfo->uiMaxSubPkgId);

            pstProcOut->packbagAlert.package_loc.x = 0.0f;
            pstProcOut->packbagAlert.package_loc.width = 1.0f;

            continue;
        }

#endif

        bFindValidSubPkgFlag = SAL_TRUE;

        pstProcOut->packbagAlert.up_pkg_idx = pstValidPkgInfo->auiPkgID[i] >= pstProcOut->packbagAlert.up_pkg_idx \
                                              ? pstValidPkgInfo->auiPkgID[i] \
                                              : pstProcOut->packbagAlert.up_pkg_idx;

        fTmp = pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageRect.x;
        fTmpMinX = (fTmp < fTmpMinX) ? fTmp : fTmpMinX;

        fTmp = pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageRect.x + pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageRect.width;
        fTmpMaxX = (fTmp > fTmpMaxX) ? fTmp : fTmpMaxX;

        fTmp = pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageRect.y;
        fTmpMinY = (fTmp < fTmpMinY) ? fTmp : fTmpMinY;

        fTmp = pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageRect.y + pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageRect.height;
        fTmpMaxY = (fTmp > fTmpMaxY) ? fTmp : fTmpMaxY;
    }

    /* 若包裹融合缓存中所有包裹都已经消失，则返回失败 */
    if (0 != uiPkgNoTarFlag)
    {
        SVA_LOGD("no pkg tar exist! cnt %d \n", pstValidPkgInfo->uiPkgLogicCnt);
        goto err;
    }

    if (bFindValidSubPkgFlag)
    {
        pstProcOut->packbagAlert.package_loc.x = fTmpMinX < 0.002f ? 0.002f : fTmpMinX;
        pstProcOut->packbagAlert.package_loc.y = fTmpMinY < 0.002f ? 0.002f : fTmpMinY;
        pstProcOut->packbagAlert.package_loc.width = fTmpMaxX - fTmpMinX > 0.998f ? 0.998f : fTmpMaxX - fTmpMinX;
        pstProcOut->packbagAlert.package_loc.height = fTmpMaxY - fTmpMinY > 0.998f ? 0.998f : fTmpMaxY - fTmpMinY;

        return SAL_SOK;
    }

    SVA_LOGD("no find valid sub pkg! \n");

err:
    return SAL_FAIL;
}

/**
 * @function    Sva_DrvPutValidPkgBuf
 * @brief       释放指定id的包裹缓存
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvPutValidPkgBuf(UINT32 uiFreeIdx, SVA_PKG_VALID_INFO_S *pastPkgValidBuf)
{
    UINT32 i = 0;

    for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
    {
        if (SAL_TRUE != pastPkgValidBuf[i].bDataValid)
        {
            continue;
        }
    }

    memset(&pastPkgValidBuf[uiFreeIdx], 0x00, sizeof(SVA_PKG_VALID_INFO_S));
    return SAL_SOK;
}

/**
 * @function    Sva_DrvFindFreeValidPkgBuf
 * @brief       寻找空闲的有效包裹缓存空间
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvFindFreeValidPkgBuf(SVA_PKG_VALID_INFO_S *pastPkgValidBuf, UINT32 *puiFreeIdx)
{
    UINT32 i;

    for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
    {
        if (SAL_TRUE != pastPkgValidBuf[i].bDataValid)
        {
            pastPkgValidBuf[i].bDataValid = SAL_TRUE;
            break;
        }
    }

    if (i >= SVA_PACKAGE_MAX_NUM)
    {
        SVA_LOGE("valid pkg buf full!!! \n");
        return SAL_FAIL;
    }

    *puiFreeIdx = i;
    return SAL_SOK;
}

/**
 * @function    Sva_DrvUpdatePackbagFwdLoc
 * @brief       更新包裹前沿
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvUpdatePackbagFwdLoc(UINT32 chan, UINT32 uiDirection, SVA_PROCESS_OUT *pstProcOut)
{
    UINT32 i, j, k;
    INT32 iPackageNum = -1;
    UINT32 uiTmpPkgLogicId = 0;
    float fTmpFwd = 0.0f;
    SVA_PKG_VALID_INFO_S *pstValidPkgInfo = NULL;
    SVA_PKG_COMB_PRM_S *pstPkgCombPrm = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_PTR(pstProcOut, err, "pstProcOut == null!");

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PTR(pstSva_dev, err, "pstSva_dev == null!");

    pstPkgCombPrm = &pstSva_dev->stPkgCombPrm;
    /* 查找包裹最小ID */
    for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
    {
        pstValidPkgInfo = &pstPkgCombPrm->stPkgValidInfo[i];

        if (SAL_TRUE != pstValidPkgInfo->bDataValid)
        {
            continue;
        }

        if (uiTmpPkgLogicId > pstValidPkgInfo->uiPkgLogicId || -1 == iPackageNum)
        {
#if 0
            for (j = 0; j < pstValidPkgInfo->uiPkgLogicCnt; j++)
            {
                /* 判断包裹是否消失，包裹没有消失，包裹ID才有效 */
                if (pstValidPkgInfo->abPkgExist[j] == SAL_TRUE)
                {
                    uiTmpPkgLogicId = pstValidPkgInfo->uiPkgLogicId;
                    iPackageNum = i;
                }
            }

#endif
            uiTmpPkgLogicId = pstValidPkgInfo->uiPkgLogicId;
            iPackageNum = i;
        }
    }

    fTmpFwd = (0 == uiDirection) ? 1.0f : 0.0f;
    if (-1 == iPackageNum)
    {
        goto reuse;                                                         /* 没找到包裹 */
    }

    pstValidPkgInfo = &pstPkgCombPrm->stPkgValidInfo[iPackageNum];
    for (j = 0; j < pstValidPkgInfo->uiPkgLogicCnt; j++)
    {
        if (j >= SVA_PACKAGE_MAX_NUM)
        {
            SVA_LOGE("szl_dbg: invalid pkg logic cnt %d, j %d \n", pstValidPkgInfo->uiPkgLogicCnt, j);
            continue;
        }

        if (SAL_TRUE != pstValidPkgInfo->abPkgExist[j])
        {
            continue;
        }

        for (k = 0; k < pstProcOut->packbagAlert.MainViewPackageNum; k++)
        {
            /* fixme: 当前对融合包裹内部所有的子包裹统计实现包裹前沿计算，未考虑子包裹误检情况 */
            if (pstValidPkgInfo->auiPkgID[j] != pstProcOut->packbagAlert.MainViewPackageLoc[k].PakageID)
            {
                continue;
            }

            /* 没找到返回上一级循环 */
            if (k >= pstProcOut->packbagAlert.MainViewPackageNum)
            {
                break;
            }

            if (0 == uiDirection)
            {
                fTmpFwd = (fTmpFwd < pstProcOut->packbagAlert.MainViewPackageLoc[k].PackageForwardLocat) \
                          ? fTmpFwd \
                          : pstProcOut->packbagAlert.MainViewPackageLoc[k].PackageForwardLocat;
            }
            else
            {
                fTmpFwd = (fTmpFwd > pstProcOut->packbagAlert.MainViewPackageLoc[k].PackageForwardLocat) \
                          ? fTmpFwd \
                          : pstProcOut->packbagAlert.MainViewPackageLoc[k].PackageForwardLocat;
            }
        }
    }

reuse:
    /* 从左向右方向包裹坐标做转换 */
    if (1 == uiDirection)
    {
        pstProcOut->packbagAlert.fPkgFwdLoc = (1.0f - fTmpFwd);
    }
    else
    {
        pstProcOut->packbagAlert.fPkgFwdLoc = fTmpFwd;       /* 保存当前所有逻辑包裹计算后的包裹前沿用于后续集中判图处理 */
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/* 判断两个包裹是否属于包含关系的IOU阈值 */
#define SVA_PKG_CONTAIN_IOU_THRESH (0.2f)
#define GET_W_R_CNT(w, r, len) ((w >= r) ? (w - r) : (len - r + w))

/**
 * @function    Sva_DrvCalPkgCombIOU
 * @brief       计算两个子包裹IOU，计算公式: 交叠像素面积/并集像素面积
 * @param[in]
 * @param[out]
 * @return
 */
static float Sva_DrvCalPkgCombIOU(SVA_RECT_F *pstPkgRect_1, SVA_RECT_F *pstPkgRect_2)
{
    float f32PkgCombIOU = 0.0f;
    float f32LeftX_1, f32LeftY_1, f32RightX_1, f32RightY_1, f32LeftX_2, f32LeftY_2, f32RightX_2, f32RightY_2;
    float f32OneDimLen[2] = {0.0f, 0.0f};

    f32LeftX_1 = pstPkgRect_1->x;
    f32LeftY_1 = pstPkgRect_1->y;
    f32RightX_1 = pstPkgRect_1->x + pstPkgRect_1->width;
    f32RightY_1 = pstPkgRect_1->y + pstPkgRect_1->height;

    f32LeftX_2 = pstPkgRect_2->x;
    f32LeftY_2 = pstPkgRect_2->y;
    f32RightX_2 = pstPkgRect_2->x + pstPkgRect_2->width;
    f32RightY_2 = pstPkgRect_2->y + pstPkgRect_2->height;

    /* x方向 */
    f32OneDimLen[0] = SVA_GET_ABS(f32LeftX_1, f32RightX_2);
    f32OneDimLen[1] = SVA_GET_ABS(f32LeftX_2, f32RightX_1);

    /* 复用 */
    f32LeftX_1 = pstPkgRect_1->width + pstPkgRect_2->width - SVA_GET_MAX(f32OneDimLen[0], f32OneDimLen[1]);

    if (f32LeftX_1 < fabs(1e-6))
    {
        f32PkgCombIOU = 0.0f;

        SVA_LOGD("111 f32LeftX_1 %f, w_1 %f, w_2 %f, f32OneDimLen_0 %f, f32OneDimLen_1 %f \n",
                 f32LeftX_1, pstPkgRect_1->width, pstPkgRect_2->width, f32OneDimLen[0], f32OneDimLen[1]);
        goto exit;
    }

    /* y方向 */
    f32OneDimLen[0] = SVA_GET_ABS(f32LeftY_1, f32RightY_2);
    f32OneDimLen[1] = SVA_GET_ABS(f32LeftY_2, f32RightY_1);

    /* 复用 */
    f32LeftY_1 = pstPkgRect_1->height + pstPkgRect_2->height - SVA_GET_MAX(f32OneDimLen[0], f32OneDimLen[1]);

    if (f32LeftY_1 < fabs(1e-6))
    {
        f32PkgCombIOU = 0.0f;
        SVA_LOGD("222 f32LeftX_1 %f, h_1 %f, h_2 %f, f32OneDimLen_0 %f, f32OneDimLen_1 %f \n",
                 f32LeftY_1, pstPkgRect_1->height, pstPkgRect_2->height, f32OneDimLen[0], f32OneDimLen[1]);
        goto exit;
    }

#if 0
    if ((UINT32)(pstPkgRect_1->width * 1000.0) * (UINT32)
        (pstPkgRect_1->height * 1000.0)
        < (UINT32)(pstPkgRect_2->width * 1000.0) * (UINT32)
        (pstPkgRect_2->height * 1000.0))
    {
        pstSmallerPkg = pstPkgRect_1;
    }
    else
    {
        pstSmallerPkg = pstPkgRect_2;
    }

#endif

    f32PkgCombIOU = (f32LeftX_1 * 1000.0 * f32LeftY_1 * 1000.0) \
                    / (pstPkgRect_1->width * 1000.0 * pstPkgRect_1->height * 1000.0 \
                       + pstPkgRect_2->width * 1000.0 * pstPkgRect_2->height * 1000.0 \
                       - f32LeftX_1 * 1000.0 * f32LeftY_1 * 1000.0);

    SVA_LOGI("get f32PkgCombIOU %f, [%f, %f] [%f, %f]---[%f, %f] [%f, %f] \n",
             f32PkgCombIOU,
             pstPkgRect_1->x, pstPkgRect_1->y, pstPkgRect_1->width, pstPkgRect_1->height,
             pstPkgRect_2->x, pstPkgRect_2->y, pstPkgRect_2->width, pstPkgRect_2->height);

exit:
    return f32PkgCombIOU;
}

/**
 * @function    Sva_DrvCalPkgCombIOU
 * @brief       计算两个子包裹IOU
 * @param[in]
 * @param[out]
 * @return
 */
static BOOL Sva_DrvPkgCoverFlag(UINT32 chan, UINT32 u32DetPkgId, SVA_RECT_F *pstDetPkgRect)
{
    UINT32 i, j, cnt;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PKG_COMB_PRM_S *pstPkgCombPrm = NULL;
    SVA_LAST_VALID_PKG_PRM_S *pstLastValidPkgPrm = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PTR(pstSva_dev, err, "pstSva_dev == null!");

    pstPkgCombPrm = &pstSva_dev->stPkgCombPrm;

    cnt = GET_W_R_CNT(pstPkgCombPrm->uiWIdx, pstPkgCombPrm->uiRIdx, SVA_PACKAGE_MAX_NUM);

    for (i = pstPkgCombPrm->uiRIdx; i < pstPkgCombPrm->uiRIdx + cnt; i++)
    {
        pstLastValidPkgPrm = &pstPkgCombPrm->stLastValidPkgPrm[i % SVA_PACKAGE_MAX_NUM];

        if (u32DetPkgId == pstLastValidPkgPrm->uiMaxSubPkgId)
        {
            break;
        }
    }

    if (i >= pstPkgCombPrm->uiRIdx + cnt)
    {
        SVA_LOGD("szl_dbgggggg: u32DetPkgId %d \n", u32DetPkgId);
        return SAL_FALSE;
    }

    /* 构造一个valid pkg info */
    SVA_PKG_VALID_INFO_S stTmpValidPkgInfo = {0};

    {
        memset(&stTmpValidPkgInfo, 0x00, sizeof(SVA_PKG_VALID_INFO_S));

        stTmpValidPkgInfo.bDataValid = SAL_TRUE;
        stTmpValidPkgInfo.uiPkgLogicCnt = pstPkgCombPrm->stLastValidPkgPrm[i % SVA_PACKAGE_MAX_NUM].uiPkgLogicCnt;

        if (stTmpValidPkgInfo.uiPkgLogicCnt > SVA_PACKAGE_MAX_NUM)
        {
            SVA_LOGE("szl_dbg: invalid pkg logic cnt %d, r %d, w %d \n",
                     stTmpValidPkgInfo.uiPkgLogicCnt, pstPkgCombPrm->uiRIdx, pstPkgCombPrm->uiWIdx);
            goto err;
        }

        for (j = 0; j < stTmpValidPkgInfo.uiPkgLogicCnt; j++)
        {
            if (j >= SVA_PACKAGE_MAX_NUM)
            {
                SVA_LOGE("szl_dbg: invalid j %d >= max %d, logic cnt %d \n",
                         j, SVA_PACKAGE_MAX_NUM, stTmpValidPkgInfo.uiPkgLogicCnt);
                continue;
            }

            stTmpValidPkgInfo.abPkgExist[j] = SAL_TRUE;
            stTmpValidPkgInfo.auiPkgID[j] = pstPkgCombPrm->stLastValidPkgPrm[i % SVA_PACKAGE_MAX_NUM].auiPkgID[j];
        }

        if (SAL_SOK != Sva_DrvUpCombPkgLoc(chan, &stTmpValidPkgInfo, &pstPkgCombPrm->stTmpOut))
        {
            SVA_LOGD("get comb pkg loc failed! chan %d \n", chan);
            goto err;
        }
    }

    return (SVA_PKG_CONTAIN_IOU_THRESH < Sva_DrvCalPkgCombIOU(&pstPkgCombPrm->stTmpOut.packbagAlert.package_loc, (SVA_RECT_F *)pstDetPkgRect));
err:
    return SAL_FALSE;
}

/**
 * @function    Sva_DrvValidatePkgFlag
 * @brief       逻辑判断是否包裹使能
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvValidatePkgFlag(IN UINT32 chan,
                                    IN UINT32 uiPkgDetCnt,
                                    IN XSIE_PACKAGE_DET_T *pastPkgDetInfo,
                                    IN UINT32 uiDirection,
                                    OUT SVA_PROCESS_OUT *pstProcOut)
{
    UINT32 i, j, k;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PKG_COMB_PRM_S *pstPkgCombPrm = NULL;
    SVA_LAST_VALID_PKG_PRM_S *pstLastValidPkgPrm = NULL;

    SVA_RECT_F stPackageRect = {0};

    SVA_DRV_CHECK_PTR(pastPkgDetInfo, err, "pastPkgDetInfo == null!");
    SVA_DRV_CHECK_PTR(pstProcOut, err, "pstProcOut == null!");

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PTR(pstSva_dev, err, "pstSva_dev == null!");

    pstPkgCombPrm = &pstSva_dev->stPkgCombPrm;

    /* 默认包裹未出全 */
    pstProcOut->packbagAlert.package_valid = SAL_FALSE;

    SVA_LOGD("szl_dbg: det cnt %d, chan %d, frm %d \n", uiPkgDetCnt, chan, pstProcOut->frame_num);

    for (i = 0; i < uiPkgDetCnt; i++)
    {
        sal_memcpy_s(&pstPkgCombPrm->stTmpOut, sizeof(SVA_PROCESS_OUT), pstProcOut, sizeof(SVA_PROCESS_OUT));

        /* 用于规避融合包裹已经det valid，之后子包裹再出现det valid的问题。且此处考虑包裹区域IOU。非包裹融合模式，需要跳过。 */
        if (SAL_TRUE == !!(pstSva_dev->u32PkgCombFlag))
        {
            if (SVA_ORIENTATION_TYPE_L2R == uiDirection)
            {
                sal_memcpy_s(&stPackageRect, sizeof(SVA_RECT_F), &pastPkgDetInfo[i].PackageRect, sizeof(VCA_RECT_F));
                stPackageRect.x = 1.0 - stPackageRect.x - stPackageRect.width;

            }

            if (SAL_TRUE == Sva_DrvPkgCoverFlag(chan, pstPkgCombPrm->u32CurMaxDetId, &stPackageRect))
            {
                SVA_LOGI("old det pkg %d, cur max %d, plus IOU > 0.2 \n", pastPkgDetInfo[i].PakageID, pstPkgCombPrm->u32CurMaxDetId);
                continue;
            }
        }

        for (j = 0; j < SVA_PACKAGE_MAX_NUM; j++)
        {
            if (SAL_TRUE != pstPkgCombPrm->stPkgValidInfo[j].bDataValid)
            {
                continue;
            }

            if (pastPkgDetInfo[i].PakageID == pstPkgCombPrm->stPkgValidInfo[j].uiMaxSubPkgId)
            {
                if (SAL_TRUE != pastPkgDetInfo[i].PackValid || SAL_TRUE != pastPkgDetInfo[i].PackDetProc)
                {
                    SVA_LOGI("id %d, not det proc yet! frm %d, pkgValid %d, proc %d, reason %d \n",
                             pastPkgDetInfo[i].PakageID, pastPkgDetInfo[i].PackFrameNum, pastPkgDetInfo[i].PackValid,
                             pastPkgDetInfo[i].PackDetProc, pastPkgDetInfo[i].ObdLossType);
                    continue;
                }

                break;
            }
        }

        pstPkgCombPrm->u32CurMaxDetId = pastPkgDetInfo[i].PakageID;

        if (j >= SVA_PACKAGE_MAX_NUM)
        {
            SVA_LOGW("chan %u, uiPkgDetCnt %d, not found det id %d!!! \n",
                     chan, uiPkgDetCnt, pastPkgDetInfo[i].PakageID);

#if 0   /* 调试打印，保留用于问题定位 */
            for (k = 0; k < SVA_PACKAGE_MAX_NUM; k++)
            {
                SVA_LOGW("k %d \n", k);
                pr_sva_valid_pkg_info(&pstPkgCombPrm->stPkgValidInfo[j]);
            }

#endif
            continue;
        }

        /* 更新全局包裹信息用于编图，若失败则当前帧不进行valid处理 */
        if (SAL_SOK != Sva_DrvUpCombPkgLoc(chan, &pstPkgCombPrm->stPkgValidInfo[j], pstProcOut))
        {
            SVA_LOGE("update combine pkg loc failed! chan %d \n", chan);
            goto reuse;
        }

        /* 找到det目标中找到对应包裹id的目标后，包裹有效 */
        pstProcOut->packbagAlert.package_valid = SAL_TRUE;
        pstProcOut->packbagAlert.package_id = pastPkgDetInfo[i].PakageID;

        /* pr_sva_pkg_info(pstProcOut);  / * 调试信息 * / */

        SVA_LOGI("set validate:chan %u, id %d, frmNum %d, j %d valid %d, [%f, %f] [%f, %f], frmNum %d \n",
                 chan,
                 pastPkgDetInfo[i].PakageID,
                 pstProcOut->frame_num,
                 j,
                 pstProcOut->packbagAlert.package_valid,
                 pstProcOut->packbagAlert.package_loc.x,
                 pstProcOut->packbagAlert.package_loc.y,
                 pstProcOut->packbagAlert.package_loc.width,
                 pstProcOut->packbagAlert.package_loc.height,
                 pstProcOut->frame_num);

        /* 注: 如果包裹融合处理中丢失了有效包裹目标，即丢失了valid信号，没有及时清空全局缓存，可能会有异常的问题 */

reuse:
        if ((pstPkgCombPrm->uiWIdx + 1) % SVA_PACKAGE_MAX_NUM == pstPkgCombPrm->uiRIdx)
        {
            SVA_LOGW("save last valid pkg failed! full! w %d, r %d \n", pstPkgCombPrm->uiWIdx, pstPkgCombPrm->uiRIdx);
            pstPkgCombPrm->uiRIdx = (pstPkgCombPrm->uiRIdx + 1) % SVA_PACKAGE_MAX_NUM;
        }

        if (pstPkgCombPrm->uiWIdx >= SVA_PACKAGE_MAX_NUM)
        {
            SVA_LOGE("szl_dbg: invalid w %d > max %d \n", pstPkgCombPrm->uiWIdx, SVA_PACKAGE_MAX_NUM);
            continue;
        }

        SVA_LOGD("szl_dbg: j %d, logic cnt %d \n", j, pstPkgCombPrm->stPkgValidInfo[j].uiPkgLogicCnt);

        if (pstPkgCombPrm->stPkgValidInfo[j].uiPkgLogicCnt >= SVA_PACKAGE_MAX_NUM)
        {
            SVA_LOGE("szl_dbg: j %d, uiPkgLogicCnt %d > max %d, pstPkgCombPrm->uiWIdx %d \n",
                     j, pstPkgCombPrm->stPkgValidInfo[j].uiPkgLogicCnt, SVA_PACKAGE_MAX_NUM, pstPkgCombPrm->uiWIdx);
            continue;
        }

        /* 备份当前valid使能的逻辑包裹信息，用于更新全局包裹信息 */
        pstLastValidPkgPrm = &pstPkgCombPrm->stLastValidPkgPrm[pstPkgCombPrm->uiWIdx % SVA_PACKAGE_MAX_NUM];

        pstLastValidPkgPrm->uiPkgLogicCnt = pstPkgCombPrm->stPkgValidInfo[j].uiPkgLogicCnt;
        pstLastValidPkgPrm->uiMaxSubPkgId = 0;

        for (k = 0; k < pstLastValidPkgPrm->uiPkgLogicCnt; k++)
        {
            pstLastValidPkgPrm->auiPkgID[k] = pstPkgCombPrm->stPkgValidInfo[j].auiPkgID[k];
            pstLastValidPkgPrm->uiMaxSubPkgId = pstLastValidPkgPrm->auiPkgID[k] > pstLastValidPkgPrm->uiMaxSubPkgId \
                                                ? pstLastValidPkgPrm->auiPkgID[k] \
                                                : pstLastValidPkgPrm->uiMaxSubPkgId;
        }

        pstPkgCombPrm->uiWIdx = (pstPkgCombPrm->uiWIdx + 1) % SVA_PACKAGE_MAX_NUM;

        /* 释放有效包裹数组buf */
        (VOID)Sva_DrvPutValidPkgBuf(j, &pstPkgCombPrm->stPkgValidInfo[0]);

        for (j = 0; j < pstProcOut->packbagAlert.MainViewPackageNum; j++)
        {
            SVA_LOGD("id %d, [%f, %f] [%f, %f] \n",
                     pstProcOut->packbagAlert.MainViewPackageLoc[j].PakageID,
                     pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageRect.x,
                     pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageRect.y,
                     pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageRect.width,
                     pstProcOut->packbagAlert.MainViewPackageLoc[j].PackageRect.height);
        }
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Sva_DrvJudgePkgValid
 * @brief       逻辑判断该帧是否为60帧强制分割
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvJudgePkgValid(IN XSIE_PACKAGE_DET_T *pastPkgDetInfo,
                                  IN XSIE_SECURITY_ALERT_T *pstXsiAlertInfo,
                                  OUT SVA_PROCESS_OUT *pstProcOut)
{
    if (pstXsiAlertInfo->FrameOffset >= 1e-6)
    {
        uiLastFrmOffsetflag = 0;
    }

    if ((pastPkgDetInfo->PackValid == 0) && (pastPkgDetInfo->PackDetProc == 1))
    {
        if (uiLastFrmOffsetflag)
        {
            pstProcOut->packbagAlert.package_valid = SAL_FALSE;
        }

        uiLastFrmOffsetflag = 1;
        SVA_LOGW("packvalid %d, detProc %d, frmoffset %f, framenum %d, valid %d, lastfrmoffsetflag %d \n",
                 pastPkgDetInfo->PackValid,
                 pastPkgDetInfo->PackDetProc,
                 pstXsiAlertInfo->FrameOffset,
                 pstProcOut->frame_num,
                 pstProcOut->packbagAlert.package_valid,
                 uiLastFrmOffsetflag);
    }

    return SAL_SOK;
}

#define SVA_PKG_COMBINE_THRESH (10.0 / (float)SVA_MODULE_WIDTH)

/**
 * @function    Sva_DrvGetPkgLocById
 * @brief       获取对应包裹id的定位信息
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvGetPkgLocById(IN UINT32 uiPkgId, IN SVA_PROCESS_OUT *pstProcOut, OUT SVA_RECT_F *pstPkgLoc)
{
    UINT32 i;

    for (i = 0; i < pstProcOut->packbagAlert.MainViewPackageNum; i++)
    {
        if (uiPkgId == pstProcOut->packbagAlert.MainViewPackageLoc[i].PakageID)
        {
            break;
        }
    }

    if (i >= pstProcOut->packbagAlert.MainViewPackageNum)
    {
        SVA_LOGD("not found pkg id[%d]!!! \n", uiPkgId);
        return SAL_FAIL;
    }

    sal_memcpy_s(pstPkgLoc, sizeof(SVA_RECT_F), &pstProcOut->packbagAlert.MainViewPackageLoc[i].PackageRect, sizeof(SVA_RECT_F));

    return SAL_SOK;
}

/**
 * @function    Sva_DrvPkgCombination
 * @brief       包裹融合
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvPkgCombination(IN UINT32 chan,
                                   OUT SVA_PROCESS_OUT *pstProcOut)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i, j;
    float f32Comb_W = 0.0;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PKG_COMB_PRM_S *pstPkgCombPrm = NULL;
    SVA_XSIE_PACKAGE_T *pstPkgInfo = NULL;
    SVA_PKG_VALID_INFO_S *pstValidPkgInfo = NULL;

    SVA_RECT_F stPkgLoc = {0};

    SVA_DRV_CHECK_PTR(pstProcOut, err, "pstProcOut == null!");

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PTR(pstSva_dev, err, "pstSva_dev == null!");

    pstPkgCombPrm = &pstSva_dev->stPkgCombPrm;

    if (SAL_TRUE == pstProcOut->bZoomValid || SAL_TRUE == pstProcOut->bFastMovValid)
    {
        sal_memset_s(pstPkgCombPrm, sizeof(SVA_PKG_COMB_PRM_S),
                     0x00, sizeof(SVA_PKG_COMB_PRM_S));
        SVA_LOGE("result cleared!  chan: %u \n", chan);
    }

    for (i = 0; i < pstProcOut->packbagAlert.MainViewPackageNum; i++)
    {
        pstPkgInfo = &pstProcOut->packbagAlert.MainViewPackageLoc[i];

        /* 历史包裹不处理 */
        if ((SAL_TRUE == pstPkgInfo->IsHistoryPackage)
            || (SAL_TRUE != !!(pstSva_dev->u32PkgCombFlag) && SAL_TRUE != pstPkgInfo->PackageValid))
        {
            continue;
        }

        /* fixme: 存在无用循环，待优化 */
        /* 包裹粘连条件需要更新，使用像素阈值进行 */
        for (j = 0; j < SVA_PACKAGE_MAX_NUM; j++)
        {
            pstValidPkgInfo = &pstPkgCombPrm->stPkgValidInfo[j];
            if (SAL_TRUE != pstValidPkgInfo->bDataValid
                || (SAL_TRUE != !!pstSva_dev->u32PkgCombFlag))
            {
                continue;
            }

            /* 如果满足交叠情况，则修改类型为包裹融合类型，且更新id */
            if (SVA_VALID_PKG_TYPE_SINGLE == pstValidPkgInfo->enPkgValidType)
            {
                s32Ret = Sva_DrvGetPkgLocById(pstValidPkgInfo->uiPkgLogicId, pstProcOut, &stPkgLoc);
            }
            else if (SVA_VALID_PKG_TYPE_COMBINE == pstValidPkgInfo->enPkgValidType)
            {
                /* 获取融合后的包裹坐标 */
                s32Ret = Sva_DrvUpCombPkgLoc(chan, pstValidPkgInfo, pstProcOut);
                memcpy(&stPkgLoc, &pstProcOut->packbagAlert.package_loc, sizeof(SVA_RECT_F));
            }
            else
            {
                s32Ret = SAL_FAIL;
                SVA_LOGE("invalid pkg type %d \n", pstValidPkgInfo->enPkgValidType);
            }

            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("get pkg combine loc failed! type %d \n", pstValidPkgInfo->enPkgValidType);
                continue;
            }

            SVA_LOGD("get j %d, type %d, loc [%f, %f] [%f, %f], cur id %d, [%f, %f] [%f, %f] \n",
                     j, pstValidPkgInfo->enPkgValidType, stPkgLoc.x, stPkgLoc.y, stPkgLoc.width, stPkgLoc.height,
                     pstPkgInfo->PakageID, pstPkgInfo->PackageRect.x, pstPkgInfo->PackageRect.y, pstPkgInfo->PackageRect.width, pstPkgInfo->PackageRect.height);

            /* 判断包裹是否存在X交叠，即是否满足包裹融合条件，若满足则更新valid数组 */
            f32Comb_W = SVA_CAL_DIFF(pstPkgInfo->PackageRect.x, stPkgLoc.x + stPkgLoc.width) \
                        < SVA_CAL_DIFF(stPkgLoc.x, pstPkgInfo->PackageRect.x + pstPkgInfo->PackageRect.width) \
                        ? SVA_CAL_DIFF(stPkgLoc.x, pstPkgInfo->PackageRect.x + pstPkgInfo->PackageRect.width) \
                        : SVA_CAL_DIFF(pstPkgInfo->PackageRect.x, stPkgLoc.x + stPkgLoc.width);

            if ((SAL_TRUE == !!(pstSva_dev->u32PkgCombFlag))
                && (f32Comb_W - stPkgLoc.width - pstPkgInfo->PackageRect.width) < (pstSva_dev->f32PkgCombThr + fabs(1e-6)))
            {
                /* 当存在新包裹时需要新增融合包裹的源目标，否则仅更新包裹坐标 */
                if (pstPkgInfo->PakageID > pstValidPkgInfo->uiMaxSubPkgId)
                {
                    /* 融合包裹缓存满 */
                    if ((pstValidPkgInfo->uiW_Idx + 1) % SVA_PACKAGE_MAX_NUM == pstValidPkgInfo->uiR_Idx)
                    {
                        pstValidPkgInfo->uiR_Idx = (pstValidPkgInfo->uiR_Idx + 1) % SVA_PACKAGE_MAX_NUM;
                        SVA_LOGW("valid sub pkg cnt up to 16! \n");
                    }

                    pstValidPkgInfo->enPkgValidType = SVA_VALID_PKG_TYPE_COMBINE;
                    pstValidPkgInfo->auiPkgID[pstValidPkgInfo->uiW_Idx % SVA_PACKAGE_MAX_NUM] = pstPkgInfo->PakageID;
                    pstValidPkgInfo->abOldPkg[pstValidPkgInfo->uiW_Idx % SVA_PACKAGE_MAX_NUM] = pstPkgInfo->IsHistoryPackage;
                    pstValidPkgInfo->abPkgExist[pstValidPkgInfo->uiW_Idx % SVA_PACKAGE_MAX_NUM] = SAL_TRUE;

                    pstValidPkgInfo->uiW_Idx = (pstValidPkgInfo->uiW_Idx + 1) % SVA_PACKAGE_MAX_NUM;
                    pstValidPkgInfo->uiPkgLogicCnt = GET_W_R_CNT(pstValidPkgInfo->uiW_Idx, pstValidPkgInfo->uiR_Idx, SVA_PACKAGE_MAX_NUM);

                    pstValidPkgInfo->uiMaxSubPkgId = pstPkgInfo->PakageID;      /* 更新包裹逻辑id */

                    SVA_LOGD("need combine enter! cur id %d, cnt %d, maxSubId %d \n",
                             pstPkgInfo->PakageID, pstValidPkgInfo->uiPkgLogicCnt, pstValidPkgInfo->uiMaxSubPkgId);

                    pr_sva_valid_pkg_info(pstValidPkgInfo);
                }

                SVA_LOGD("need combine! j %d, id %d, maxSubId %d, type %d, cnt %d \n",
                         j, pstPkgInfo->PakageID, pstValidPkgInfo->uiMaxSubPkgId, pstValidPkgInfo->enPkgValidType, pstValidPkgInfo->uiPkgLogicCnt);

                if (pstPkgInfo->PackageValid && pstPkgInfo->PakageID > pstValidPkgInfo->uiPkgLogicId)
                {
                    pstValidPkgInfo->uiPkgLogicId = pstPkgInfo->PakageID;
                }

                /* 若找到包裹融合的目标后，则直接退出不进行后续的遍历。
                   目前只存在single-single-combine的情况，不存在combine-single的顺序，故将single判断放在前面 */
                break;
            }
        }

        if (j >= SVA_PACKAGE_MAX_NUM)
        {
            UINT32 uiFreeIdx = 0;
            if (SAL_SOK != Sva_DrvFindFreeValidPkgBuf(&pstPkgCombPrm->stPkgValidInfo[0], &uiFreeIdx))
            {
                SVA_LOGE("find free valid pkg buf failed! \n");
                continue;
            }

            pr_sva_pkg_info(pstProcOut);  /* 调试信息 */

            /* 初始化包裹融合参数，避免后续判断是否满足包裹融合条件时脏数据 */
            pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].uiPkgLogicCnt = 0;
            memset(pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].auiPkgID, 0x00, sizeof(UINT32) * SVA_PACKAGE_MAX_NUM);

            pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].enPkgValidType = SVA_VALID_PKG_TYPE_SINGLE;
            pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].uiPkgLogicId = pstPkgInfo->PakageID;
            pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].auiPkgID[0] = pstPkgInfo->PakageID;
            pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].abOldPkg[0] = pstPkgInfo->IsHistoryPackage;    /* 历史包裹信息 */
            pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].abPkgExist[0] = SAL_TRUE;        /* 标记为包裹存在 */
            pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].uiMaxSubPkgId = pstPkgInfo->PakageID;
            pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].uiW_Idx = 1;
            pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].uiR_Idx = 0;
            pstPkgCombPrm->stPkgValidInfo[uiFreeIdx].uiPkgLogicCnt = 1;
            SVA_LOGI("valid enter! chan %u,i %d, id %d, frmNum %d, freeidx %d, pkg_valid %d, IsHistoryPackage %d \n",
                     chan, i, pstPkgInfo->PakageID, pstProcOut->frame_num, uiFreeIdx, pstPkgInfo->PackageValid, pstPkgInfo->IsHistoryPackage);
        }
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Sva_DrvGetPkgInfoFromEng
 * @brief       从引擎接口获取包裹相关信息
 * @param[in]
 * @param[out]
 * @return
 */
static VOID Sva_DrvGetPkgInfoFromEng(IN UINT32 uiPkgCnt,
                                     IN XSIE_PACKAGE_T *pastPkgInfo,
                                     IN UINT32 uiPkgDetCnt,
                                     IN XSIE_PACKAGE_DET_T *pastPkgDetInfo,
                                     IN UINT32 uiDirection,
                                     IN UINT32 uiChan,
                                     OUT SVA_PACK_ALERT *pstSvaPkgOut)
{
    UINT32 i, j;
    CAPB_PRODUCT *pstProductPrm = capb_get_product();    /* 获取能力级，用于区分安检机还是分析仪 */

    /* checker */
    SVA_DRV_CHECK_PTR(pstProductPrm, err, "pstProductPrm == null!");
    SVA_DRV_CHECK_PTR(pastPkgInfo, err, "pastPkgInfo == null!");
    SVA_DRV_CHECK_PTR(pastPkgDetInfo, err, "pastPkgDetInfo == null!");
    SVA_DRV_CHECK_PTR(pstSvaPkgOut, err, "pstSvaPkgOut == null!");

#if 1    /*打印记录det时间*/
    for (i = 0; i < uiPkgDetCnt; i++)
    {
        if (i >= XSIE_PACK_DET_NUM)
        {
            SVA_LOGE("szl_dbg: invalid i %d, detCnt %d \n", i, uiPkgDetCnt);
            continue;
        }

        if (pastPkgDetInfo[i].PackValid == SAL_TRUE)
        {
            SVA_LOGI("ch %u det: id %d, proc %d, frm %d, packValid %d, dataValid %d, [%f, %f] [%f, %f]\n",
                     uiChan,
                     pastPkgDetInfo[i].PakageID,
                     pastPkgDetInfo[i].PackDetProc,
                     pastPkgDetInfo[i].PackFrameNum,
                     pastPkgDetInfo[i].PackValid,
                     pastPkgDetInfo[i].DataValid,
                     pastPkgDetInfo[i].PackageRect.x,
                     pastPkgDetInfo[i].PackageRect.y,
                     pastPkgDetInfo[i].PackageRect.width,
                     pastPkgDetInfo[i].PackageRect.height);
        }
    }

#endif

    j = 0;
    for (i = 0; i < VCA_MIN(uiPkgCnt, 16); i++)
    {
        if (-1 == pastPkgInfo[i].PakageID)
        {
            continue;
        }

        pstSvaPkgOut->MainViewPackageLoc[j].PakageID = pastPkgInfo[i].PakageID;
        pstSvaPkgOut->MainViewPackageLoc[j].PackageRect.x = pastPkgInfo[i].PackageRect.x;
        pstSvaPkgOut->MainViewPackageLoc[j].PackageRect.y = pastPkgInfo[i].PackageRect.y;
        pstSvaPkgOut->MainViewPackageLoc[j].PackageRect.width = pastPkgInfo[i].PackageRect.width;
        pstSvaPkgOut->MainViewPackageLoc[j].PackageRect.height = pastPkgInfo[i].PackageRect.height;
        pstSvaPkgOut->MainViewPackageLoc[j].PackageValid = pastPkgInfo[i].PackageValid;
        pstSvaPkgOut->MainViewPackageLoc[j].PackageForwardLocat = pastPkgInfo[i].PackageForwardLocat;
        pstSvaPkgOut->MainViewPackageLoc[j].IsHistoryPackage = pastPkgInfo[i].IsHistoryPackage;

        /* 从左向右方向包裹坐标做转换 */
        if (VIDEO_INPUT_OUTSIDE == pstProductPrm->enInputType
            && 1 == uiDirection)
        {
            pstSvaPkgOut->MainViewPackageLoc[j].PackageRect.x = 1.0   \
                                                                - pstSvaPkgOut->MainViewPackageLoc[j].PackageRect.x   \
                                                                - pstSvaPkgOut->MainViewPackageLoc[j].PackageRect.width;
        }

        if (pastPkgInfo[i].PackageValid)
        {
            /* 包裹分包结束，开始obd检测 */
            SVA_LOGI("chan %d start obd, packid %d ! main_packnum %d\n", uiChan, pastPkgInfo[i].PakageID, uiPkgCnt);

            SVA_LOGI("ch %u, id %d, valid %d, fwd %f, [%f, %f] [%f, %f]\n",
                     uiChan,
                     pastPkgInfo[i].PakageID,
                     pastPkgInfo[i].PackageValid,
                     pastPkgInfo[i].PackageForwardLocat,
                     pastPkgInfo[i].PackageRect.x,
                     pastPkgInfo[i].PackageRect.y,
                     pastPkgInfo[i].PackageRect.width,
                     pastPkgInfo[i].PackageRect.height);

            /* debug */
            debug_pr_all_pkg_info(uiPkgCnt, pastPkgInfo, uiPkgDetCnt, pastPkgDetInfo);
        }

        j++;
    }

    pstSvaPkgOut->MainViewPackageNum = j;

    SVA_XSIE_PACKAGE_T stTmpPkgInfo = {0};

    /* 升序排列bubble */
    for (i = 0; pstSvaPkgOut->MainViewPackageNum > 0 && i < pstSvaPkgOut->MainViewPackageNum - 1; i++)
    {
        for (j = 0; j < pstSvaPkgOut->MainViewPackageNum - i - 1; j++)
        {
            if (pstSvaPkgOut->MainViewPackageLoc[j].PakageID > pstSvaPkgOut->MainViewPackageLoc[j + 1].PakageID)
            {
                sal_memcpy_s(&stTmpPkgInfo, sizeof(SVA_XSIE_PACKAGE_T), &pstSvaPkgOut->MainViewPackageLoc[j], sizeof(SVA_XSIE_PACKAGE_T));
                sal_memcpy_s(&pstSvaPkgOut->MainViewPackageLoc[j], sizeof(SVA_XSIE_PACKAGE_T), &pstSvaPkgOut->MainViewPackageLoc[j + 1], sizeof(SVA_XSIE_PACKAGE_T));
                sal_memcpy_s(&pstSvaPkgOut->MainViewPackageLoc[j + 1], sizeof(SVA_XSIE_PACKAGE_T), &stTmpPkgInfo, sizeof(SVA_XSIE_PACKAGE_T));
            }
        }
    }

    /* 备份原始引擎返回的包裹数据，排序后 */
    pstSvaPkgOut->OriginalPackageNum = pstSvaPkgOut->MainViewPackageNum;
    memcpy(&pstSvaPkgOut->OriginalPackageLoc[0], &pstSvaPkgOut->MainViewPackageLoc[0], sizeof(SVA_XSIE_PACKAGE_T) * SVA_PACKAGE_MAX_NUM);

err:
    return;
}

/**
 * @function    Sva_DrvUpdatePackbagAlert
 * @brief       更新包裹报警信息坐标,取历史有效包裹
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvUpdatePackbagAlert(UINT32 chan, UINT32 uiDirection, IN SVA_PKG_COMB_PRM_S *pstPkgCombPrm, OUT SVA_PROCESS_OUT *pstProcOut)
{
    UINT32 i, j;
    SVA_LAST_VALID_PKG_PRM_S *pstLastValidPkg = NULL;
    SVA_PKG_VALID_INFO_S stTmpLastVaildPkg = {0};

    SVA_DRV_CHECK_PTR(pstPkgCombPrm, err, "pstPkgCombPrm == null!");
    SVA_DRV_CHECK_PTR(pstProcOut, err, "pstProcOut == null!");

    /*
       fixme: 此处规避回传多次同一个包裹的编图结果的问题。

       --------------
     |   pkg_1    |
     |   id: 123  |
     |            |
       --------------
       --------------
     |   pkg_2    |
     |   id: 124  |
     |            |
       --------------

       1. 接口设计初衷:
          用于集中判图时非pkg_valid帧实时更新包裹位置信息，用于分片裁剪。

       2. 问题背景:
          以从右到左过包为例，实际的复杂过包场景中存在124包裹先det valid，其次123包裹再det valid。
          基于这种情形，此接口的逻辑会导致回调两次124的包裹(具体逻辑看实现)。

       3. 规避理由:
          考虑到此接口仅在集中判图时有使用，且集中判图场景简单且单一，该功能仅在简单场景下单一过包，故不存在这种问题。
          曾考虑将此接口实现多场景兼容，发现场景太多且算法返回结果不存在明确规律，故不花太多时间精力在此接口中。

       4. 目前已知异常场景:
          a. 实例所述；
          b. TBD...

       MODYFIED BY SUNZELIN@2022.4.19
     */

    /* 此帧处理有效包裹时，直接返回不更新实时包裹坐标 */
    if (pstProcOut->packbagAlert.package_valid)
    {
        return SAL_SOK;
    }

    for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
    {
        pstLastValidPkg = &pstPkgCombPrm->stLastValidPkgPrm[i];
        /* 无效包裹 */
        if (pstLastValidPkg->uiPkgLogicCnt == 0)
        {
            continue;
        }

        /* fixme: 此处的cnt值来源于前级Sva_DrvValidatePkgFlag接口更新，维护性较差需修复 */
        stTmpLastVaildPkg.uiPkgLogicCnt = pstLastValidPkg->uiPkgLogicCnt;
        for (j = 0; j < stTmpLastVaildPkg.uiPkgLogicCnt; j++)
        {
            stTmpLastVaildPkg.abPkgExist[j] = SAL_TRUE;
            stTmpLastVaildPkg.auiPkgID[j] = pstLastValidPkg->auiPkgID[j];
        }

        if (Sva_DrvUpCombPkgLoc(chan, &stTmpLastVaildPkg, &pstPkgCombPrm->stTmpOut) != SAL_SOK)
        {
            /* SVA_LOGW("Sva_DrvUpCombPkgLoc err\n"); */
            continue;
        }

        if (1 == uiDirection)
        {
            /* 从左到右，取坐标小的 */
            if (pstProcOut->packbagAlert.package_loc.x > pstPkgCombPrm->stTmpOut.packbagAlert.package_loc.x)
            {
                memcpy(&pstProcOut->packbagAlert.package_loc,
                       &pstPkgCombPrm->stTmpOut.packbagAlert.package_loc,
                       sizeof(SVA_RECT_F));
            }
        }
        else
        {
            /* 从右到左，取坐标大的 */
            if (pstProcOut->packbagAlert.package_loc.x < pstPkgCombPrm->stTmpOut.packbagAlert.package_loc.x)
            {
                memcpy(&pstProcOut->packbagAlert.package_loc,
                       &pstPkgCombPrm->stTmpOut.packbagAlert.package_loc,
                       sizeof(SVA_RECT_F));
            }
        }
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Sva_DrvSyncCombPkgInfo
 * @brief       同步包裹融合结果到全局信息
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvSyncCombPkgInfo(UINT32 chan, UINT32 uiDirection, SVA_PROCESS_OUT *pstProcOut)
{
    UINT32 i, j, cnt, uiLastValidPkgCnt, uiTmpRIdx;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PKG_COMB_PRM_S *pstPkgCombPrm = NULL;
    SVA_PKG_VALID_INFO_S *pstValidPkgInfo = NULL;

    SVA_DRV_CHECK_PTR(pstProcOut, err, "pstProcOut == null!");

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PTR(pstSva_dev, err, "pstSva_dev == null!");

    pstPkgCombPrm = &pstSva_dev->stPkgCombPrm;

    /* 使用中间变量拷贝全局结果，避免后续被覆盖 */
    memcpy(&pstPkgCombPrm->stTmpOut, pstProcOut, sizeof(SVA_PROCESS_OUT));

    cnt = 0;

    /* 将历史融合包裹中经过obd检测的包裹整理到外部结构体 */
    {
        /* 构造一个valid pkg info */
        SVA_PKG_VALID_INFO_S stTmpValidPkgInfo = {0};

        uiLastValidPkgCnt = GET_W_R_CNT(pstPkgCombPrm->uiWIdx, pstPkgCombPrm->uiRIdx, SVA_PACKAGE_MAX_NUM);
        uiTmpRIdx = pstPkgCombPrm->uiRIdx;

        for (j = 0; j < uiLastValidPkgCnt; j++)
        {
            memset(&stTmpValidPkgInfo, 0x00, sizeof(SVA_PKG_VALID_INFO_S));

            stTmpValidPkgInfo.bDataValid = SAL_TRUE;
            stTmpValidPkgInfo.uiPkgLogicCnt = pstPkgCombPrm->stLastValidPkgPrm[(j + uiTmpRIdx) % SVA_PACKAGE_MAX_NUM].uiPkgLogicCnt;

            if (stTmpValidPkgInfo.uiPkgLogicCnt > SVA_PACKAGE_MAX_NUM)
            {
                SVA_LOGE("szl_dbg: invalid pkg logic cnt %d, j %d, uiLastValidPkgCnt %d, r %d, w %d \n",
                         stTmpValidPkgInfo.uiPkgLogicCnt, j, uiLastValidPkgCnt, pstPkgCombPrm->uiRIdx, pstPkgCombPrm->uiWIdx);
                continue;
            }

            for (i = 0; i < stTmpValidPkgInfo.uiPkgLogicCnt; i++)
            {
                if (i >= SVA_PACKAGE_MAX_NUM)
                {
                    SVA_LOGE("szl_dbg: invalid i %d >= max %d, logic cnt %d \n",
                             i, SVA_PACKAGE_MAX_NUM, stTmpValidPkgInfo.uiPkgLogicCnt);
                    continue;
                }

                stTmpValidPkgInfo.abPkgExist[i] = SAL_TRUE;
                stTmpValidPkgInfo.auiPkgID[i] = pstPkgCombPrm->stLastValidPkgPrm[(j + uiTmpRIdx) % SVA_PACKAGE_MAX_NUM].auiPkgID[i];
            }

            /* pr_sva_valid_pkg_info(&stTmpValidPkgInfo); */

            if (SAL_SOK != Sva_DrvUpCombPkgLoc(chan, &stTmpValidPkgInfo, &pstPkgCombPrm->stTmpOut))
            {
                SVA_LOGD("get comb pkg loc failed! chan %d \n", chan);
#if 0
                /* 若获取缓存的last valid pkg融合包裹信息失败，则ridx+1。因为融合包裹是入队列先入先出，所以此处直接读索引累加 */
                if ((pstPkgCombPrm->uiRIdx + 1) % SVA_PACKAGE_MAX_NUM != pstPkgCombPrm->uiWIdx)
                {
                    pstPkgCombPrm->uiRIdx = (pstPkgCombPrm->uiRIdx + 1) % SVA_PACKAGE_MAX_NUM;
                }

#endif
                continue;
            }

            if (cnt > SVA_PACKAGE_MAX_NUM)
            {
                SVA_LOGE("szl_dbg_1: invalid cnt %d \n", cnt);
                continue;
            }

            pstProcOut->packbagAlert.MainViewPackageLoc[cnt].PakageID = pstPkgCombPrm->stTmpOut.packbagAlert.up_pkg_idx;
            memcpy(&pstProcOut->packbagAlert.MainViewPackageLoc[cnt].PackageRect,
                   &pstPkgCombPrm->stTmpOut.packbagAlert.package_loc,
                   sizeof(SVA_RECT_F));
            cnt++;
        }
    }

    /* 遍历valid pkg数组，将融合包裹信息同步到外部结构体进行后续处理 */
    for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
    {
        pstValidPkgInfo = &pstPkgCombPrm->stPkgValidInfo[i];

        if (SAL_TRUE != pstValidPkgInfo->bDataValid)
        {
            continue;
        }

        if (SAL_SOK != Sva_DrvUpCombPkgLoc(chan, &pstPkgCombPrm->stPkgValidInfo[i], &pstPkgCombPrm->stTmpOut))
        {
            SVA_LOGD("get comb pkg loc failed! chan %d, i %d \n", chan, i);
            continue;
        }

        if (cnt > SVA_PACKAGE_MAX_NUM)
        {
            SVA_LOGE("szl_dbg_2: invalid cnt %d \n", cnt);
            continue;
        }

        pstProcOut->packbagAlert.MainViewPackageLoc[cnt].PakageID = pstPkgCombPrm->stTmpOut.packbagAlert.up_pkg_idx;
        memcpy(&pstProcOut->packbagAlert.MainViewPackageLoc[cnt].PackageRect,
               &pstPkgCombPrm->stTmpOut.packbagAlert.package_loc,
               sizeof(SVA_RECT_F));
        cnt++;
    }

    pstProcOut->packbagAlert.MainViewPackageNum = cnt;

    /*更新包裹报警信息，用于集中判图*/
    Sva_DrvUpdatePackbagAlert(chan, uiDirection, pstPkgCombPrm, pstProcOut);

    return SAL_SOK;
err:
    if (NULL != pstProcOut)
    {
        pstProcOut->packbagAlert.MainViewPackageNum = 0;  /* 异常时包裹个数为0 */
    }

    return SAL_FAIL;
}

/**
 * @function    Sva_DrvUpPkgCombPrm
 * @brief       更新融合包裹参数
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvUpPkgCombPrm(IN UINT32 chan,
                                 IN SVA_PROCESS_OUT *pstProcOut)
{
    UINT32 i, j, k, tmpFlag;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_PKG_COMB_PRM_S *pstPkgCombPrm = NULL;
    SVA_PKG_VALID_INFO_S *pstValidPkgInfo = NULL;

    SVA_DRV_CHECK_PTR(pstProcOut, err, "pstProcOut == null!");

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PTR(pstSva_dev, err, "pstSva_dev == null!");

    pstPkgCombPrm = &pstSva_dev->stPkgCombPrm;

    for (i = 0; i < SVA_PACKAGE_MAX_NUM; i++)
    {
        pstValidPkgInfo = &pstPkgCombPrm->stPkgValidInfo[i];

        if (SAL_TRUE != pstValidPkgInfo->bDataValid)
        {
            continue;
        }

        tmpFlag = 1;

        for (j = 0; j < pstValidPkgInfo->uiPkgLogicCnt; j++)
        {
            if (j >= SVA_PACKAGE_MAX_NUM)
            {
                SVA_LOGE("szl_dbg: i %d, invalid j %d, logic cnt %d \n", i, j, pstValidPkgInfo->uiPkgLogicCnt);
                continue;
            }

            /* 包裹消失则不进行处理 */
            if (SAL_TRUE != pstValidPkgInfo->abPkgExist[j])
            {
                tmpFlag &= 0x1;
                continue;
            }

            tmpFlag &= 0x0;

            for (k = 0; k < pstProcOut->packbagAlert.MainViewPackageNum; k++)
            {
                if (pstValidPkgInfo->auiPkgID[j] == pstProcOut->packbagAlert.MainViewPackageLoc[k].PakageID)
                {
                    break;
                }
            }

            /* 在当前帧结果中找不到对应包裹则标记为已消失 */
            if (k >= pstProcOut->packbagAlert.MainViewPackageNum)
            {
                pstValidPkgInfo->abPkgExist[j] = SAL_FALSE;
                continue;
            }

            pstValidPkgInfo->abPkgExist[j] = SAL_TRUE;
            pstValidPkgInfo->abOldPkg[j] = pstProcOut->packbagAlert.MainViewPackageLoc[k].IsHistoryPackage;
        }

        /* 若融合包裹内部所有子包裹都已经消失，则清空该缓存 */
        if (0 != tmpFlag)
        {
            memset(pstValidPkgInfo, 0x00, sizeof(SVA_PKG_VALID_INFO_S));
        }
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Sva_DrvImgUpdatePkgLoc
 * @brief:      图片模式下更新包裹坐标
 * @param[in]:  UINT32 uiPkgCnt
 * @param[out]: SVA_PROCESS_OUT *pstAlgRslt
 * @return:     BOOL
 */
INT32 Sva_DrvImgUpdatePkgLoc(IN UINT32 uiPkgCnt,
                             IN XSIE_PACKAGE_T *pastPkgInfo,
                             OUT SVA_PROCESS_OUT *pstAlgRslt)
{
    UINT32 i = 0;

    if (g_proc_mode == SVA_PROC_MODE_IMAGE)
    {
        for (i = 0; i < VCA_MIN(uiPkgCnt, 16); i++)
        {
            if (-1 == pastPkgInfo[i].PakageID)
            {
                continue;
            }

            sal_memcpy_s(&pstAlgRslt->packbagAlert.package_loc, sizeof(SVA_RECT_F),
                         &pastPkgInfo[i].PackageRect, sizeof(SVA_RECT_F));
        }
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvGetOutputRslt
 * @brief:      将算法返回的结果转换为智能模块内部的数据结构中
 * @param[in]:  VOID *pstRawAlgOutput
 * @param[out]: SVA_ALG_RSLT_S *pstAlgRslt
 * @return:     BOOL
 */
static INT32 Sva_DrvGetOutputRslt(XSIE_SECURITY_XSI_OUT_T *pstXsieOut,
                                  XSIE_USER_PRVT_INFO *pstXsieUserPrvtInfo,
                                  SVA_ALG_RSLT_S *pstAlgRslt)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiChnIdx = 0;
    UINT32 uiDirection = 0;
    UINT32 uiPkgCnt = 0;
    UINT32 uiPkgDetCnt = 0;
    UINT32 uiTarCnt = 0;

    SVA_SYNC_RESULT_S *pstRslt = NULL;
    XSIE_SECURITY_XSI_INFO_T *pstXsiOutInfo = NULL;
    XSIE_PACKAGE_T *pastPkgInfo = NULL;
    XSIE_PACKAGE_DET_T *pastPkgDetInfo = NULL;
    XSIE_SECURITY_ALERT_TARGET_T *pastTarInfo = NULL;

    uiChnIdx = pstAlgRslt->uiChnCnt;
    if (uiChnIdx >= SVA_DEV_MAX)
    {
        SVA_LOGE("invalid chn idx %d > max %d \n", uiChnIdx, SVA_DEV_MAX);
        return SAL_FAIL;
    }

    pstXsiOutInfo = (XSIE_SECURITY_XSI_INFO_T *)&pstXsieOut->XsiOutInfo;

    /* 调试打印 */
    (VOID)Sva_DrvPrintResultCb(uiChnIdx, pstXsiOutInfo, &pstXsieOut->XsiAddInfo);

    /* 获取用户私有信息 */
    s32Ret = Sva_DrvGetOutputUsrInfo(pstXsieUserPrvtInfo, pstAlgRslt);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvGetOutputUsrInfo failed!");

    pstRslt = &pstAlgRslt->stAlgChnRslt[uiChnIdx];

    /* 智能通道号映射 */
    pstRslt->chan = Sva_DrvChnMap(pstRslt->enProcMode, pstRslt->chan, g_sva_common.uiMainViewChn);   /* fixme: 主视角通道号通过全局获取 */

    pstRslt->stProcOut.frame_offset = pstXsiOutInfo->Alert.FrameOffset;
    pstRslt->stProcOut.packbagAlert.fPkgFwdLoc = pstXsiOutInfo->Alert.PackageForwardLocat;
    pstRslt->stProcOut.packbagAlert.candidate_flag = pstXsiOutInfo->Alert.CandidateFlag;
    pstRslt->stProcOut.bPkgForceSplit = pstXsiOutInfo->Alert.Package_force;
    pstRslt->stProcOut.bZoomValid = pstXsiOutInfo->Alert.Zoom_valid;
    SVA_LOGD("get output!!  chan %d uiChnIdx %d target_num %d completePackMark %d packIndx %d packTop %d packBottom %d  packOffsetX %d timeRef %d\n",
             pstRslt->chan, uiChnIdx, pstXsiOutInfo->Alert.MainViewTargetNum, pstRslt->stProcOut.aipackinfo.completePackMark, \
             pstRslt->stProcOut.aipackinfo.packIndx, pstRslt->stProcOut.aipackinfo.packTop, \
             pstRslt->stProcOut.aipackinfo.packBottom, pstRslt->stProcOut.aipackinfo.packOffsetX, pstRslt->stProcOut.aipackinfo.timeRef);

    if (pstXsiOutInfo->Alert.Zoom_valid)
    {
        SVA_LOGE("get zoom ! uiChnIdx %d \n", uiChnIdx);
    }

    uiDirection = pstXsieUserPrvtInfo->uiDirection;

    /* 获取通道idx到主侧视角的映射 */
    Sva_DrvViewTypeMap(uiChnIdx,
                       &pstXsiOutInfo->Alert,
                       &uiPkgCnt,
                       &pastPkgInfo,
                       &uiPkgDetCnt,
                       &pastPkgDetInfo,
                       &uiTarCnt,
                       &pastTarInfo);

    /* 调试打印 */
    debug_pr_all_pkg_info(pstXsiOutInfo->Alert.MainViewPackageNum, &pstXsiOutInfo->Alert.MainViewPackageLoc[0],
                          pstXsiOutInfo->Alert.MainViewPackDetNum, &pstXsiOutInfo->Alert.MainViewPackDet[0]);

    /* 获取包裹信息 */
    (VOID)Sva_DrvGetPkgInfoFromEng(uiPkgCnt, pastPkgInfo, uiPkgDetCnt, pastPkgDetInfo, uiDirection, pstAlgRslt->uiChnCnt, &pstRslt->stProcOut.packbagAlert);

    /* 获取目标信息 */
    (VOID)Sva_DrvGetTarInfoFromEng(uiTarCnt, pastTarInfo, uiDirection, &pstRslt->stProcOut);

    /* checker */
    if (SAL_TRUE == Sva_DrvCheckRectAndPrint(&pstRslt->stProcOut.packbagAlert.package_loc))
    {
        SAL_LOGD("invalid xsi out pkg info-side: [%f, %f] [%f, %f] \n",
                 pstRslt->stProcOut.packbagAlert.package_loc.x, pstRslt->stProcOut.packbagAlert.package_loc.y,
                 pstRslt->stProcOut.packbagAlert.package_loc.width, pstRslt->stProcOut.packbagAlert.package_loc.height);
    }

    /* 获取输出帧数据结果 */
    s32Ret = Sva_DrvGetOutputFrm(pstXsieOut, pstXsieUserPrvtInfo, pstAlgRslt);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvGetOutputFrm failed!");

    /* 获取pos结果 */
    s32Ret = Sva_DrvGetOutputPos(pstXsieOut, pstAlgRslt);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvGetOutputPos failed!");

    /* fixme: 后面将偏移量处理作为后处理模块部分调用 */
    s32Ret = Sva_DrvProcOffset(uiDirection, pstAlgRslt);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvProcOffset failed!");

    /* 逐帧更新包裹融合控制参数，主要用于更新画面中消失的包裹标记 */
    s32Ret = Sva_DrvUpPkgCombPrm(pstRslt->chan, &pstRslt->stProcOut);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvRsltChecker failed!");

    /* 包裹融合后处理，主要实现将二维X重叠的valid包裹无效化 */
    s32Ret = Sva_DrvPkgCombination(pstRslt->chan, &pstRslt->stProcOut);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvRsltChecker failed!");

    /* 逐帧更新包裹融合控制参数，主要用于更新画面中消失的包裹标记 */
    s32Ret = Sva_DrvUpPkgCombPrm(pstRslt->chan, &pstRslt->stProcOut);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvRsltChecker failed!");

    /* 更新包裹前沿，用于集中判图 */
    s32Ret = Sva_DrvUpdatePackbagFwdLoc(pstRslt->chan, uiDirection, &pstRslt->stProcOut);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvRsltChecker failed!");

    /* 使能业务模块的package_valid标记，指导后续编图和集中判图处理 */
    s32Ret = Sva_DrvValidatePkgFlag(pstRslt->chan, uiPkgDetCnt, pastPkgDetInfo, uiDirection, &pstRslt->stProcOut);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvRsltChecker failed!");

    /* 根据obd检测数据判断当前帧是否为60帧强制分割并对帧差判断，即当无过包数据时不编图 */
    s32Ret = Sva_DrvJudgePkgValid(pastPkgDetInfo, &pstXsiOutInfo->Alert, &pstRslt->stProcOut);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvJudgePkgValid failed!");

    /* 更新融合后的包裹信息到全局 */
    s32Ret = Sva_DrvSyncCombPkgInfo(pstRslt->chan, uiDirection, &pstRslt->stProcOut);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvRsltChecker failed!");

    /* 图片模式下更新包裹坐标 */
    s32Ret = Sva_DrvImgUpdatePkgLoc(uiPkgCnt, pastPkgInfo, &pstRslt->stProcOut);
    SVA_DRV_CHECK_RET(s32Ret, err, "Sva_DrvImgProc failed!");

    Sva_DrvPrPkgInfo(&pstRslt->stProcOut, __FUNCTION__, __LINE__);

    /* 通道号累加 */
    pstAlgRslt->uiChnCnt++;

    if (SAL_TRUE == pstRslt->stProcOut.packbagAlert.package_valid)
    {
        SVA_LOGD("dbg: uiChnCnt %d, chan %d, frm %d, target %d, [%f, %f], [%f, %f] \n",
                 pstAlgRslt->uiChnCnt, pstRslt->chan, pstRslt->stProcOut.frame_num, pstRslt->stProcOut.target_num,
                 pstRslt->stProcOut.packbagAlert.package_loc.x, pstRslt->stProcOut.packbagAlert.package_loc.y,
                 pstRslt->stProcOut.packbagAlert.package_loc.width, pstRslt->stProcOut.packbagAlert.package_loc.height);
    }

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvInitDefaultPrm
* 描  述  : 模块初始化参数
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvInitDefaultPrm(UINT32 chan)
{
    UINT32 j = 0;
    INT32 s32Ret = 0;
    UINT32 sva_type = 0;
    UINT64 u64PhyAddrSvaPrm = 0;
    SVA_PROCESS_IN *pstIn = NULL;

    SVA_DEV_PRM *pstSva_dev = NULL;
    DSPINITPARA *pDspInitPara = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

#ifdef DSP_ISM
    UINT64 u64PhyAddrPackQue = 0;
    CAPB_PRODUCT *pstCapbProduct = NULL;
    SVA_PROCESS_OUT_DATA *pstPackOut = NULL;
    DSA_QueHndl *pstInPackageDataQue = NULL;
    DSA_QueHndl *pstOutPackageDataFullQue = NULL;
    DSA_QueHndl *pstOutPackageDataEmptQue = NULL;
#endif

    SVA_DSP_DET_STRU *stSvaInitInfoSt = NULL;     /* &g_pDspInitPara.stSvaInitInfoSt; */
    OSD_SET_ARRAY_S stOsdArray;
    OSD_SET_PRM_S *pstOsdPrm = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pDspInitPara = SystemPrm_getDspInitPara();
    if (pDspInitPara == NULL)
    {
        SVA_LOGE("error\n");
        return SAL_FAIL;
    }

    stSvaInitInfoSt = &pDspInitPara->stSvaInitInfoSt[chan];
    pstSva_dev->enChnSize = 1.914;
    pstSva_dev->f32PkgCombThr = 0.0f;    /* 默认值0，完全依据算法返回的包裹坐标判断是否粘连 */
    pstSva_dev->u32PkgCombFlag = 1;
    pstSva_dev->stForceSplitPrm.u32Flag = 0;

    pstSva_dev->uiSwitch = SAL_TRUE;
    pstSva_dev->stIn.stTargetPrm.enOsdExtType = SVA_ALERT_EXT_PERCENT;
    pstSva_dev->stIn.stTargetPrm.scaleLevel = 1;

    pstSva_dev->stIn.sva_name_cnt = 1;

    pstSva_dev->stIn.enDirection = SVA_ORIENTATION_TYPE_R2L;
    pstSva_dev->stIn.uiPkgSensity = 2;       /* 算法推荐默认包裹分割灵敏度为2 */
    pstSva_dev->stIn.uiPkgSplitFilter = 50;
    pstSva_dev->stIn.fZoomInOutVal = 0.025;
    pstSva_dev->stIn.uiPkSwitch = 1;/*pk模式默认常开*/
    pstSva_dev->stIn.uiSprayFilterSwitch = 0;

    pstSva_dev->stIn.rect.x = INPUT_X;
    pstSva_dev->stIn.rect.y = INPUT_Y;
    pstSva_dev->stIn.rect.width = INPUT_WIDTH;
    pstSva_dev->stIn.rect.height = INPUT_HEIGHT;

#ifdef DSP_ISM
    g_sva_common.stEngProcPrm.enProcMode = SVA_PROC_MODE_IMAGE;
#else
    g_sva_common.stEngProcPrm.enProcMode = SVA_PROC_MODE_DUAL_CORRELATION;
#endif

    g_sva_common.stEngProcPrm.uiAiEnable = SAL_FALSE;

    if (stSvaInitInfoSt->uiCnt > SVA_XSI_MAX_ALARM_TYPE)
    {
        SVA_LOGE("sva Cnt %d error!!! max %d\n", stSvaInitInfoSt->uiCnt, SVA_XSI_MAX_ALARM_TYPE);
        return SAL_FAIL;
    }

    SVA_LOGI("from app: chan is %d uiCnt = %d, drawType is %d, size %d, size2 %d, size3 %d \n",
             chan, stSvaInitInfoSt->uiCnt, pstSva_dev->stIn.drawType,
             (UINT32)sizeof(SAV_INS_ENC_PRM_S), (UINT32)sizeof(SAV_INS_ENC_RSLT_S), (UINT32)sizeof(SVA_XSIE_PACKAGE_T) * SVA_PACKAGE_MAX_NUM);
    if (stSvaInitInfoSt->uiCnt <= 0)
    {
        SVA_LOGE("invalid init target type cnt %d \n", stSvaInitInfoSt->uiCnt);
        return SAL_FAIL;
    }

    stOsdArray.u32StringNum = 0;
    stOsdArray.pstOsdPrm = SAL_memMalloc(stSvaInitInfoSt->uiCnt * sizeof(OSD_SET_PRM_S), "SVA", "sva_osdprm");
    if (NULL == stOsdArray.pstOsdPrm)
    {
        SVA_LOGE("malloc osd block buff fail\n");
        return SAL_FAIL;
    }

    for (j = 0; j < stSvaInitInfoSt->uiCnt; j++)
    {
        sva_type = stSvaInitInfoSt->det_info[j].sva_type;
        if (sva_type >= SVA_XSI_MAX_ALARM_TYPE)
        {
            SVA_LOGE("invalid type %d, chan %d \n", sva_type, chan);
            continue;
        }

        pstSva_dev->stIn.alert[sva_type].bInit = SAL_TRUE;
        pstSva_dev->stIn.alert[sva_type].sva_type = stSvaInitInfoSt->det_info[j].sva_type;
        pstSva_dev->stIn.alert[sva_type].sva_key = stSvaInitInfoSt->det_info[j].sva_key;
        pstSva_dev->stIn.alert[sva_type].sva_color = stSvaInitInfoSt->det_info[j].sva_color;
        pstSva_dev->stIn.alert[sva_type].sva_alert_tar_cnt = SVA_DEFAULT_ALERT_TARGET_CNT;
        snprintf((char *)pstSva_dev->stIn.alert[sva_type].sva_name, SVA_ALERT_NAME_LEN, "%s", (char *)stSvaInitInfoSt->det_info[j].sva_name);
        SVA_LOGD("j %d, sva_name is %s, sva_type %d, sva_key %d\n",
                 j, stSvaInitInfoSt->det_info[j].sva_name, stSvaInitInfoSt->det_info[j].sva_type, stSvaInitInfoSt->det_info[j].sva_key);

        pstOsdPrm = stOsdArray.pstOsdPrm + stOsdArray.u32StringNum++;
        pstOsdPrm->u32Idx = stSvaInitInfoSt->det_info[j].sva_type;
        pstOsdPrm->szString = (char *)stSvaInitInfoSt->det_info[j].sva_name;
        pstOsdPrm->u32Color = stSvaInitInfoSt->det_info[j].sva_color;
        pstOsdPrm->u32BgColor = OSD_COLOR24_WHITE;
        pstOsdPrm->enEncFormat = pDspInitPara->stFontLibInfo.enEncFormat;
    }

    (VOID)osd_func_writeStart();
    s32Ret = osd_func_blockSet(OSD_BLOCK_IDX_STRING, &stOsdArray);
    (VOID)osd_func_writeEnd();
    SAL_memfree(stOsdArray.pstOsdPrm, "SVA", "sva_osdprm");
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGW("osd set fail\n");
    }

    Sva_DrvChangeAdaptor(pstSva_dev->enChnSize, &pstSva_dev->stIn.fScale);

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    /* 1. 创建管理满队列 */
    if (SAL_SOK != DSA_QueCreate(pstPrmFullQue, SVA_MAX_PRM_BUFFER))
    {
        SVA_LOGE("error !!!\n");
        return SAL_FAIL;
    }

    /* 2. 创建管理空队列 */
    if (SAL_SOK != DSA_QueCreate(pstPrmEmptQue, SVA_MAX_PRM_BUFFER))
    {
        SVA_LOGE("error !!!\n");
        return SAL_FAIL;
    }

    /* 3. 缓存区放入空队列 */
    for (j = 0; j < SVA_MAX_PRM_BUFFER; j++)
    {
        if (SAL_SOK != mem_hal_mmzAlloc(sizeof(SVA_PROCESS_IN), "SVA", "sva_prm", NULL, SAL_FALSE, &u64PhyAddrSvaPrm, (VOID **)&pstIn))
        {
            SVA_LOGE("error !!!\n");
            return SAL_FAIL;
        }

        SAL_clear(pstIn);
        sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));

        if (SAL_SOK != DSA_QuePut(pstPrmEmptQue, (void *)pstIn, SAL_TIMEOUT_NONE))
        {
            SVA_LOGE("error !!!\n");
            return SAL_FAIL;
        }
    }

#ifdef DSP_ISM
    pstInPackageDataQue = &pstSva_dev->stInPackageDataQue;            /*安检机输入数据队列*/
    pstOutPackageDataFullQue = &pstSva_dev->stOutPackageDataFullQue;   /*安检机处理包裹输出结果队列*/
    pstOutPackageDataEmptQue = &pstSva_dev->stOutPackageDataEmptQue;   /*安检机处理包裹输出结果队列*/

    pstCapbProduct = capb_get_product();
    if (NULL == pstCapbProduct)
    {
        SVA_LOGE("pstCapbProduct == null! chan %d\n", chan);
        return SAL_FAIL;
    }

    /*安检机添加输入包裹数据以及处理结果队列创建*/
    if (pstCapbProduct->enInputType == VIDEO_INPUT_INSIDE)
    {
        s32Ret = DSA_QueCreate(pstInPackageDataQue, SVA_MAX_PACK_IN_BUFFER);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Creat Sva PackIn Que Err!!!!\n");
            return SAL_FAIL;
        }

        s32Ret = DSA_QueCreate(pstOutPackageDataFullQue, SVA_MAX_PROCESS_OUT_BUFFER);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Creat Sva PackOut Que Err!!!!\n");
            return SAL_FAIL;
        }

        s32Ret = DSA_QueCreate(pstOutPackageDataEmptQue, SVA_MAX_PROCESS_OUT_BUFFER);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Creat Sva PackOut Que Err!!!!\n");
            return SAL_FAIL;
        }

        /*申请缓存内存数据*/
        for (j = 0; j < SVA_MAX_PROCESS_OUT_BUFFER; j++)
        {
            /*包裹空队列数据*/
            if (SAL_SOK != mem_hal_mmzAlloc(sizeof(SVA_PROCESS_OUT_DATA), "SVA", "sva_Pack_que", NULL, SAL_FALSE, &u64PhyAddrPackQue, (void **)&pstPackOut))
                if (NULL == pstPackOut)
                {
                    SVA_LOGE("error !!!\n");
                    return SAL_FAIL;
                }

            if (SAL_SOK != DSA_QuePut(pstOutPackageDataEmptQue, (void *)pstPackOut, SAL_TIMEOUT_NONE))
            {
                SVA_LOGE("error !!!\n");
                return SAL_FAIL;
            }
        }
    }

#endif

    /* 默认参数走起来 */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SVA_LOGI("Init Default Param End! chan %d \n", chan);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvModuleTsk
* 描  述  : 任务模块初始化
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Sva_DrvModuleTsk(UINT32 chan, DEVICE_CHIP_TYPE_E deviceType)
{
    INT32 s32Ret = SAL_FAIL;

    SVA_DEV_PRM *pstSva_dev = NULL;
    CAPB_PRODUCT *pstProduct = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    s32Ret = Sva_DrvInitDefaultPrm(chan);
    if (s32Ret != SAL_SOK)
    {
        SVA_LOGE("error !!!\n");
        return SAL_FAIL;
    }
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstSva_dev->mChnMutexHdl);
    /* 初始化信号+锁资源，用于线程同步 */
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstSva_dev->stProcInterPrm.mChnMutexHd1);

    SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstSva_dev->mSyncMutexHdl);

    pstProduct = capb_get_product();
    if (NULL == pstProduct)
    {
        SVA_LOGE("get platform capbility fail\n");
        return SAL_FAIL;
    }

    /*TODO:只有分析仪才有编图线程,安检机不需要*/
    if (DOUBLE_CHIP_SLAVE_TYPE != deviceType && (pstProduct->enInputType == VIDEO_INPUT_OUTSIDE))
    {
        SAL_thrCreate(&pstSva_dev->stJpegThrHandl, Sva_DrvJpegVencThread, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
    }

    /* TODO: 暂时使用显示能力集区分是安检机还是分析仪，做线程调度区分 */
    if (pstProduct->enInputType == VIDEO_INPUT_OUTSIDE)     /*分析仪*/
    {
        SVA_LOGI("This is ISA!!!\n");
        if (chan == 0)
        {
            /* 帧数据发送注册回调，信号通知发送结束 */
            g_sva_common.stXtransPrm.pSem = SAL_SemCreate(SAL_FALSE, 0);
            g_sva_common.stXtransPrm.pRsltSem = SAL_SemCreate(SAL_FALSE, 0);

            if (SAL_TRUE != IA_GetModTransFlag(IA_MOD_SVA))
            {
                SAL_thrCreate(&pstSva_dev->stSvaThrHandl, Sva_DrvVcaeSyncThread, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
            }
            else
            {
                if (DOUBLE_CHIP_MASTER_TYPE == deviceType)
                {
#ifndef PCIE_S2M_DEBUG
                    SAL_thrCreate(&pstSva_dev->stSvaThrHandl, Sva_DrvVcaeMasterSyncThread, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
#endif

#ifndef PCIE_M2S_DEBUG
                    SAL_thrCreate(&pstSva_dev->stSvaThrHandl1, Sva_DrvVcaeResultThread, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
#endif

                    SVA_LOGW("szl_dbg: create mst thread end! chan %d \n", chan);
                }

                if (DOUBLE_CHIP_SLAVE_TYPE == deviceType)
                {
#ifndef PCIE_S2M_DEBUG
                    SAL_thrCreate(&pstSva_dev->stSvaThrHandl, Sva_DrvSlaveFrameToQueThread, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
                    SAL_thrCreate(&pstSva_dev->stSvaThrHandl1, Sva_DrvSlaveSyncThread, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
#endif

#ifdef PCIE_S2M_DEBUG
                    SAL_thrCreate(&pstSva_dev->stSvaThrHandl, Sva_DrvSlaveSendRsltThread, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
#endif
                    SVA_LOGW("szl_dbg: create slv thread end! chan %d \n", chan);
                }
            }
        }

        SAL_thrCreate(&pstSva_dev->stSvaPostProcThrHandl, Sva_DrvVcaePostProcThread, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);

        SVA_LOGI("szl_dbg: create recv rslt thread end! chan %d \n", chan);
    }
    else     /*安检机*/
    {
        SVA_LOGI("This is ISM!!!\n");

        /*安检机单芯片版本*/
        if (SINGLE_CHIP_TYPE == deviceType)
        {
            SAL_thrCreate(&pstSva_dev->stSvaThrHandl, Sva_DrvVcaeThread_ISM, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
        }

        if (chan == 0) /*双芯片检测线程只创建一遍*/
        {
            /*安检机双芯片主片*/
            if (DOUBLE_CHIP_MASTER_TYPE == deviceType)
            {
                SAL_thrCreate(&pstSva_dev->stSvaThrHandl, Sva_DrvVcaMasterThread_ISM, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
                SAL_thrCreate(&pstSva_dev->stSvaThrHandl1, Sva_DrvVcaeResultThread_ISM, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
            }

            /*安检机双芯片从片*/
            if (DOUBLE_CHIP_SLAVE_TYPE == deviceType)
            {
                SAL_thrCreate(&pstSva_dev->stSvaThrHandl, Sva_DrvSlaveFrameToQueThread_ISM, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
                SAL_thrCreate(&pstSva_dev->stSvaThrHandl1, Sva_DrvSlaveSyncThread_ISM, SAL_THR_PRI_DEFAULT, 0, pstSva_dev);
            }
        }
    }

    SVA_LOGI("Module Tsk End! chan %d \n", chan);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvJudgeChnSts
* 描  述  : 判断通道状态
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static BOOL Sva_DrvJudgeChnSts(UINT32 chan)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FALSE);

    pstSva_dev = Sva_DrvGetDev(0);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FALSE);

    if (pstSva_dev->uiChnStatus)
    {
        return chan == 0;
    }
    else
    {
        return chan == 1;
    }

    return SAL_FALSE;
}

/**
 * @function:   SVA_DrvSyncProc
 * @brief:      线程同步处理
 * @param[in]:  VFST_SYNC_PRM_S *pstSyncPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvSyncProc(SVA_SYNC_PRM_S *pstSyncPrm)
{
    SVA_SYNC_TYPE_E enSyncType = 0;
    INT32 iTimeOut = 0;
    UINT32 uiWaitMs = 0;
    UINT32 *puiSyncVal = NULL;

    SVA_DRV_CHECK_PRM(pstSyncPrm, SAL_FAIL);

    enSyncType = pstSyncPrm->enType;
    iTimeOut = pstSyncPrm->iTimeOut;
    uiWaitMs = pstSyncPrm->uiSingleWaitMs;
    puiSyncVal = pstSyncPrm->puiMonitSts;

    if (DOUBLE_CHIP_SLAVE_TYPE == g_sva_common.uiDevType)
    {
        return SAL_SOK;
    }

    switch (enSyncType)
    {
        case SVA_SYNC_TYPE_ENABLE:
        {
            *puiSyncVal = SAL_TRUE;

            (VOID)SAL_mutexSignal(pstSyncPrm->pSyncMutex);

            do
            {
                if (!(*puiSyncVal))
                {
                    break;
                }

                usleep(uiWaitMs * 1000);

                if (iTimeOut > 0)
                {
                    iTimeOut--;
                }
            }
            while (iTimeOut > 0);

            break;
        }
        case SVA_SYNC_TYPE_DISABLE:
        {
            *puiSyncVal = SAL_TRUE;

            do
            {
                if (!(*puiSyncVal))
                {
                    break;
                }

                usleep(uiWaitMs * 1000);

                if (iTimeOut > 0)
                {
                    iTimeOut--;
                }
            }
            while (iTimeOut > 0);

            break;
        }
        default:
        {
            SVA_LOGW("invalid type %d! DO NOTHING!!!! \n", enSyncType);
            break;
        }
    }

    if (pstSyncPrm->iTimeOut > 0 && iTimeOut <= 0)
    {
        SVA_LOGW("TimeOut!!! %d, %d, %d\n", pstSyncPrm->iTimeOut, iTimeOut, uiWaitMs);
        return SVA_SYNC_ERR_TIMEOUT;
    }

    SVA_LOGI("sync end! \n");
    return SAL_SOK;
}

/**
 * @function   Sva_DrvInitPkgMatchPrm
 * @brief      初始化双视角包裹匹配参数
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvInitPkgMatchPrm(VOID)
{
    memset(&g_sva_common.stPkgMatchCommPrm, 0x00, sizeof(SVA_PKG_MATCH_COMM_PRM_S));

    /* 应用层约定匹配索引从1开始计数 */
    g_sva_common.stPkgMatchCommPrm.u32MatchIdx = 1;

    SVA_LOGI("reset package match param end! \n");
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvModuleInit
* 描  述  : 模块初始化
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvModuleInit(UINT32 chan)
{
    /* 计算接口耗时使用 */
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;
    UINT32 time4 = 0;
    SVA_PROC_MODE_E enProcMode = 0;
    INT32 s32Ret = SAL_FAIL;
    SVA_SYNC_PRM_S stSyncPrm = {0};
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_DEV_PRM *pstSva_dev1 = NULL;
    CAPB_PRODUCT *pstProduct = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    time1 = SAL_getCurMs();

    if (SAL_TRUE != Sva_DrvGetInitFlag() || SAL_TRUE == Sva_DrvGetProcFlag())
    {
        SVA_LOGW("Engine Init Is Not Finished! Pls Wait! chan %d \n", chan);
        return SAL_FAIL;
    }

    /* 此处进行规避: 上层应用在单路分析模式下发两个通道开启的命令 */
    if (SVA_PROC_MODE_VIDEO == g_proc_mode && g_sva_common.uiChnCnt == 1 && !(Sva_DrvJudgeChnSts(chan)))
    {
        SVA_LOGW("mode %d not support 2 channel! chan %d \n", g_proc_mode, chan);
        return SAL_FAIL;
    }

    if (chan >= g_sva_common.uiDevCnt)
    {
        SVA_LOGW("invalid chan %d, devCnt %d \n", chan, g_sva_common.uiDevCnt);
        return SAL_SOK;
    }

    pstSva_dev = Sva_DrvGetDev(chan);
    pstSva_dev1 = Sva_DrvGetDev(0);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSva_dev->uiInitStatus, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSva_dev1, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSva_dev1->uiInitStatus, SAL_FAIL);

    if (SAL_TRUE == pstSva_dev->uiChnStatus)
    {
        SVA_LOGW("chan %d is initialized! return success! \n", chan);
        return SAL_SOK;
    }

    time3 = SAL_getCurMs();

    s32Ret = Sva_HalVcaeStart(chan);
    if (s32Ret != SAL_SOK)
    {
        pstSva_dev->uiChnStatus = SAL_FALSE;

        SVA_LOGE("error\n");
        return SAL_FAIL;
    }

    time4 = SAL_getCurMs();

    g_sva_common.uiChnCnt++;

    pstSva_dev->uiChnStatus = SAL_TRUE;

    time2 = SAL_getCurMs();

    pstProduct = capb_get_product();
    SVA_DRV_CHECK_PRM(pstProduct, SAL_FAIL);

    /* 安检机暂未适配同步机制，先不执行*/
    if (pstProduct->enInputType == VIDEO_INPUT_OUTSIDE)
    {
        stSyncPrm.enType = SVA_SYNC_TYPE_ENABLE;        /* 标志位 */
        stSyncPrm.iTimeOut = SVA_DEFAULT_TIMEOUT_NUM;
        stSyncPrm.uiSingleWaitMs = SVA_DEFAULT_SINGLE_WAIT_MS;
        stSyncPrm.puiMonitSts = &pstSva_dev1->stProcInterPrm.uiSyncFlag;
        stSyncPrm.pSyncMutex = pstSva_dev1->stProcInterPrm.mChnMutexHd1;
        s32Ret = Sva_DrvSyncProc(&stSyncPrm);
        if (s32Ret != 0)
        {
            SVA_LOGE("Sva_DrvSyncProc Err! chan %d, s32Ret %x\n", chan, s32Ret);
        }
    }

    /* 用于视频模式、双通道关联模式下的对应线程状态控制，uiProcFlag=0:线程阻塞，1：线程不阻塞，关联模式时，第1次开启通道相应线程进入阻塞状态，第2次开启线程不进入阻塞状态 */
    enProcMode = g_proc_mode;
    if (enProcMode == SVA_PROC_MODE_VIDEO)
    {
        pstSva_dev1->stProcInterPrm.uiProcFlag = 1;         /* 0:线程休眠，1：线程不休眠 */
    }
    else if (enProcMode == SVA_PROC_MODE_DUAL_CORRELATION && g_sva_common.uiChnCnt == 1)
    {
        pstSva_dev1->stProcInterPrm.uiProcFlag = 0;
    }
    else if (enProcMode == SVA_PROC_MODE_DUAL_CORRELATION && g_sva_common.uiChnCnt == 2)
    {
        pstSva_dev1->stProcInterPrm.uiProcFlag = 1;
    }

    SVA_LOGI("chan %d Sva Start OK uiChnCnt is %d, vs %d, total %d, mode %d\n",
             chan, g_sva_common.uiChnCnt, time4 - time3, time2 - time1, g_proc_mode);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvClearXpackRslt
 * @brief:      外部模块清空全局参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvClearXpackRslt(UINT32 chan)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);

    /* 清空转存智能信息 */
    sal_memset_s(&pstSva_dev->stXpackRlst, sizeof(SVA_PROCESS_OUT), 0x00, sizeof(SVA_PROCESS_OUT));

    return SAL_SOK;
}

/**
 * @function    Sva_DrvClrInsPrm
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 Sva_DrvClrInsPrm(UINT32 chan)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstSva_dev->stInsEncPrm.bLastPkgValid = SAL_FALSE;

    pstSva_dev->stInsEncPrm.stRsltBufInfo.uiFwdBufIdx = 0;
    pstSva_dev->stInsEncPrm.stRsltBufInfo.fLastFwdXBuf = 0.0f;
    memset(pstSva_dev->stInsEncPrm.stRsltBufInfo.fLastOffsetBuf, 0x00, SVA_PKG_FWD_BUF_NUM * sizeof(float));
    memset(pstSva_dev->stInsEncPrm.stRsltBufInfo.u32FrmNum, 0x00, SVA_PKG_FWD_BUF_NUM * sizeof(UINT32));

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvModuleDeInit
* 描  述  : 模块去初始化
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvModuleDeInit(UINT32 chan)
{
    /* 计算接口耗时使用 */
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;
    UINT32 time4 = 0;
    SVA_PROC_MODE_E enProcMode = 0;

    time1 = SAL_getCurMs();

    INT32 s32Ret = SAL_FAIL;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_DEV_PRM *pstSva_dev1 = NULL;     /* 验证加锁是否能实现 */
    DISP_CLEAR_SVA_TYPE stClearPrm = {0};
    SVA_SYNC_PRM_S stSyncPrm = {0};
    CAPB_PRODUCT *pstProduct = NULL;

    pstProduct = capb_get_product();
    SVA_DRV_CHECK_PRM(pstProduct, SAL_FAIL);

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);

    pstSva_dev1 = Sva_DrvGetDev(0);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (chan >= g_sva_common.uiDevCnt)
    {
        SVA_LOGW("invalid chan %d, devCnt %d \n", chan, g_sva_common.uiDevCnt);
        return SAL_SOK;
    }

    SVA_DRV_CHECK_PRM(pstSva_dev->uiInitStatus, SAL_FAIL);

    if (SAL_TRUE != Sva_DrvGetInitFlag())
    {
        SVA_LOGW("Engine Init Is Not Finished! Pls Wait! chan %d \n", chan);
        return SAL_FAIL;
    }

    uiLastTimeRef[0] = 0;
    uiLastTimeRef[1] = 0;

    if (pstSva_dev->uiChnStatus != SAL_TRUE)
    {
        SAL_WARN("chan %d is closed! \n", chan);
        return SAL_SOK;
    }

    /* 先关闭通道状态，等待送帧线程休眠同步 */
    pstSva_dev->uiChnStatus = SAL_FALSE;

    /* 只有视频模式时的第一次关闭需要等待超时 */
    if (pstProduct->enInputType == VIDEO_INPUT_OUTSIDE
        && ((SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode && g_sva_common.uiChnCnt == 2)
            || (SVA_PROC_MODE_VIDEO == g_proc_mode && g_sva_common.uiChnCnt == 1)))
    {
        stSyncPrm.enType = SVA_SYNC_TYPE_DISABLE;        /* 标志位 */
        stSyncPrm.iTimeOut = SVA_DEFAULT_TIMEOUT_NUM;
        stSyncPrm.uiSingleWaitMs = SVA_DEFAULT_SINGLE_WAIT_MS;
        stSyncPrm.puiMonitSts = &pstSva_dev1->stProcInterPrm.uiSyncFlag;
        stSyncPrm.pSyncMutex = pstSva_dev1->stProcInterPrm.mChnMutexHd1;
        s32Ret = Sva_DrvSyncProc(&stSyncPrm);
        if (s32Ret != 0)
        {
            SVA_LOGE("Sva_DrvSyncProc Err! chan %d, s32Ret %x\n", chan, s32Ret);
        }
    }

    /* 通道成功关闭后，通道统计个数减一 */
    if (g_sva_common.uiChnCnt > 0)
    {
        g_sva_common.uiChnCnt--;
    }

    /* 加锁保护，当两帧未推给引擎时不能切换模式 */
    SAL_mutexLock(pstSva_dev1->mChnMutexHdl);

    time3 = SAL_getCurMs();

    s32Ret = Sva_HalVcaeStop(chan);
    if (s32Ret != SAL_SOK)
    {
        pstSva_dev->uiChnStatus = SAL_TRUE;
        SVA_LOGE("error\n");
        SAL_mutexUnlock(pstSva_dev1->mChnMutexHdl);
        return SAL_FAIL;
    }

    SAL_mutexUnlock(pstSva_dev1->mChnMutexHdl);

    time4 = SAL_getCurMs();

    SVA_LOGI("chan %d VCAE STOP cost time %d ms, mode %d \n", chan, time4 - time3, g_proc_mode);

    /* START: 智能分析关闭时，将sva和disp存放智能信息的全局变量置为空 */
    if (SAL_SOK != Sva_HalClearSvaOut(chan))
    {
        /* 非关键路径，允许失败 */
        SVA_LOGE("Clear Sva Out Err! chan %d \n", chan);
    }

    if (SAL_SOK != Sva_HalClrPoolResult(chan))
    {
        /* 非关键路径，允许失败 */
        SVA_LOGE("Clear Sva Match Out Err! chan %d \n", chan);
    }

    stClearPrm.svaclear = 1;
    stClearPrm.tip = 1;
    stClearPrm.orgnaic = 1;

#if 1  /* 注释掉安检机相关部分，若有需要可打开 */
       /* xpack转存智能信息清空 */
    if (SAL_SOK != disp_osdClearSvaResult(chan, &stClearPrm))
    {
        /* 非关键路径，允许失败 */
        SVA_LOGE("Clear Sva Out Err! chan %d \n", chan);
    }

    /* 清空转存智能信息 */
    sal_memset_s(&pstSva_dev->stXpackRlst, sizeof(SVA_PROCESS_OUT), 0x00, sizeof(SVA_PROCESS_OUT));
    /* END */
#endif

    /* 清空即刻编图相关参数 */
    (VOID)Sva_DrvClrInsPrm(chan);

    time2 = SAL_getCurMs();

    /* 用于视频模式、双通道关联模式下的对应线程状态控制，uiProcFlag=0:线程阻塞，1：线程不阻塞，关联模式时，第1次关闭通道相应线程进入阻塞状态，第2次关闭线程进入阻塞状态 */
    enProcMode = g_proc_mode;
    if (enProcMode == SVA_PROC_MODE_VIDEO)
    {
        pstSva_dev1->stProcInterPrm.uiProcFlag = 0;
    }
    else if (enProcMode == SVA_PROC_MODE_DUAL_CORRELATION && g_sva_common.uiChnCnt == 1)
    {
        pstSva_dev1->stProcInterPrm.uiProcFlag = 0;
    }
    else if (enProcMode == SVA_PROC_MODE_DUAL_CORRELATION && g_sva_common.uiChnCnt == 0)
    {
        pstSva_dev1->stProcInterPrm.uiProcFlag = 0;
    }

    SVA_LOGI("chan %d Sva Stop OK uiChnCnt %d cost time %d ms, mode %d\n", chan, g_sva_common.uiChnCnt, time2 - time1, g_proc_mode);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetMirrorChnPrm
* 描  述  : 设置镜像通道参数
* 输  入  : - chan : 通道号
*         : - prm  : vpss组参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetMirrorChnPrm(UINT32 chan, void *prm)
{
    SVA_DEV_PRM *pstSva_dev = NULL;
    DUP_ChanHandle *pDupChnHandle = NULL;

    PARAM_INFO_S stParamInfo = {0};

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    /* 保存镜像的vproc handle */
    pstSva_dev->DupHandle.pVpHandle = prm;

    pDupChnHandle = (DUP_ChanHandle *)prm;

    stParamInfo.enType = IMAGE_SIZE_CFG;
    stParamInfo.stImgSize.u32Width = SVA_MODULE_WIDTH;
    stParamInfo.stImgSize.u32Height = SVA_MODULE_HEIGHT;

    if (SAL_SOK != pDupChnHandle->dupOps.OpDupSetBlitPrm((VOID *)pDupChnHandle, &stParamInfo))
    {
        /* 非关键步骤，无需返回失败 */
        SVA_LOGE("Set Vpss Chn Resolution Failed! \n");
        return SAL_FAIL;
    }

    if (!pstSva_dev->DupHandle.bMirror)
    {
        stParamInfo.enType = MIRROR_CFG;
        stParamInfo.stMirror.u32Flip = 0;
        stParamInfo.stMirror.u32Mirror = 1;
        if (SAL_SOK != pDupChnHandle->dupOps.OpDupSetBlitPrm((VOID *)pDupChnHandle, &stParamInfo))
        {
            /* 非关键步骤，无需返回失败 */
            SVA_LOGE("Set Vpss Chn Mirror Failed! \n");
        }

        pstSva_dev->DupHandle.bMirror = 1;
        SVA_LOGI("Set Vpss Mirror End! sts: %d \n", pstSva_dev->DupHandle.bMirror);
    }

    if (SAL_SOK != pDupChnHandle->dupOps.OpDupStartBlit((VOID *)pDupChnHandle))
    {
        /* 非关键步骤，无需返回失败 */
        SVA_LOGE("Start Vpss Chn Resolution Failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvChnInit
 * @brief:      智能模块通道初始化
 * @param[in]:  UINT32 chan
 * @param[in]:  void *pDupHandle
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvChnInit(UINT32 chan, void *pDupHandle)
{
    INT32 s32Ret = SAL_FAIL;

    SVA_DEV_PRM *pstSva_dev = NULL;

    PARAM_INFO_S stDupPrmInfo = {0};

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (SAL_TRUE == IA_GetModEnableFlag(IA_MOD_SVA)
        && SAL_TRUE == IA_GetModTransFlag(IA_MOD_SVA)
        && DOUBLE_CHIP_SLAVE_TYPE == g_sva_common.uiDevType)
    {
        SVA_LOGI("Slave Chip dont need init chn! return success! chan %d \n", chan);
        return SAL_SOK;
    }

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pDupHandle, SAL_FAIL);

    if (pstSva_dev->DupHandle.pHandle == pDupHandle)
    {
        SVA_LOGW("Sva %d is done \n", chan);
        return SAL_SOK;
    }

    pstSva_dev->DupHandle.pHandle = pDupHandle;

#if defined (SS928V100_SPC010) && defined (SVA_USE_EXT_CHN)
    stDupPrmInfo.enType = IMAGE_SIZE_EXT_CFG;
#else
    stDupPrmInfo.enType = IMAGE_SIZE_CFG;
#endif
    stDupPrmInfo.stImgSize.u32Width = SVA_MODULE_WIDTH;
    stDupPrmInfo.stImgSize.u32Height = SVA_MODULE_HEIGHT;

    s32Ret = ((DUP_ChanHandle *)pDupHandle)->dupOps.OpDupSetBlitPrm(pDupHandle, &stDupPrmInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("dup_task_setDupParam failed! \n");
        return SAL_FAIL;
    }

    memset(&stDupPrmInfo, 0x00, sizeof(PARAM_INFO_S));

    stDupPrmInfo.enType = PIX_FORMAT_CFG;
    stDupPrmInfo.enPixFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

    s32Ret = ((DUP_ChanHandle *)pDupHandle)->dupOps.OpDupSetBlitPrm(pDupHandle, &stDupPrmInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("dup_task_setDupParam failed! \n");
        return SAL_FAIL;
    }

    s32Ret = ((DUP_ChanHandle *)pDupHandle)->dupOps.OpDupStartBlit(pDupHandle);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("OpDupStartBlit failed! \n");
        return SAL_FAIL;
    }

    SVA_LOGI("sva chan %d prm init end! \n", chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvFreeMatchBufMem
 * @brief:      释放匹配帧数据缓存
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_MATCH_BUF_INFO *pstMatchBufInfo
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvFreeMatchBufMem(UINT32 chan, SVA_MATCH_BUF_INFO *pstMatchBufInfo)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    SYSTEM_FRAME_INFO *pstSysFrmInfo = NULL;

    SVA_DRV_CHECK_PRM(pstMatchBufInfo, SAL_FAIL);

    for (i = 0; i < pstMatchBufInfo->uiCnt; i++)
    {
        pstSysFrmInfo = &pstMatchBufInfo->stMatchBufData[i].stSysFrameInfo;
        if (0x00 == pstSysFrmInfo->uiAppData)
        {
            SVA_LOGW("Match Buf is Released! chan %d, i %d \n", chan, i);
            continue;
        }

        s32Ret = sys_hal_rleaseVideoFrameInfoSt(pstSysFrmInfo);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Get Video Frame st failed! chan %d, i %d \n", chan, i);
            continue;
        }
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvAllocMatchBufMem
 * @brief:      申请匹配帧数据缓存
 * @param[in]:  UINT32 chan
 * @param[in]:  SVA_MATCH_BUF_INFO *pstMatchBufInfo
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvAllocMatchBufMem(UINT32 chan, SVA_MATCH_BUF_INFO *pstMatchBufInfo)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    SYSTEM_FRAME_INFO *pstSysFrmInfo = NULL;

    SVA_DRV_CHECK_PRM(pstMatchBufInfo, SAL_FAIL);

    for (i = 0; i < pstMatchBufInfo->uiCnt; i++)
    {
        pstSysFrmInfo = &pstMatchBufInfo->stMatchBufData[i].stSysFrameInfo;
        if (0x00 != pstSysFrmInfo->uiAppData)
        {
            SVA_LOGW("Match Buf is created! chan %d, i %d \n", chan, i);
            continue;
        }

        pstSysFrmInfo->uiDataAddr = 0;
        pstSysFrmInfo->uiDataHeight = 0;
        pstSysFrmInfo->uiDataWidth = 0;
        pstSysFrmInfo->uiDataLen = 0;
        s32Ret = sys_hal_allocVideoFrameInfoSt(pstSysFrmInfo);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Get Video Frame st failed! chan %d, i %d \n", chan, i);
            goto err;
        }

        SVA_LOGD("chan %d, create match buf %d end! \n", chan, i);
    }

    pstMatchBufInfo->uiOffsetW_Idx = 0;
    sal_memset_s(pstMatchBufInfo->fPastOffset, SVA_MATCH_OFFSET_TAB_CNT * sizeof(float), 0x00, SVA_MATCH_OFFSET_TAB_CNT * sizeof(float));

    return SAL_SOK;

err:
    (VOID)Sva_DrvFreeMatchBufMem(chan, pstMatchBufInfo);
    return SAL_FAIL;
}

/**
 * @function    Sva_DrvDeInitMatchBuf
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 ATTRIBUTE_UNUSED Sva_DrvDeInitMatchBuf(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_MATCH_BUF_INFO *pstMatchBufInfo = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    /* 单芯片 or 双芯片从片不需要 */
    if ((SAL_TRUE != IA_GetModEnableFlag(IA_MOD_SVA))
        || (SAL_TRUE != IA_GetModTransFlag(IA_MOD_SVA))
        || (DOUBLE_CHIP_SLAVE_TYPE == g_sva_common.uiDevType))
    {
        SVA_LOGI(" dev type %d no need Deinit match buf! return success \n", g_sva_common.uiDevType);
        return SAL_SOK;
    }

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstMatchBufInfo = pstSva_dev->pstMatchBufInfo;
    if (NULL == pstMatchBufInfo)
    {
        SVA_LOGW("Match Buf Info is De-Initialize! return Success! chan %d \n", chan);
        return SAL_SOK;
    }

    if (NULL != pstMatchBufInfo->pMutex)
    {
        (VOID)SAL_mutexDelete(pstMatchBufInfo->pMutex);
    }

    s32Ret = Sva_DrvFreeMatchBufMem(chan, pstMatchBufInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Alloc Match Mem Failed! chan %d \n", chan);
    }

    if (NULL != pstSva_dev->pstMatchBufInfo)
    {
        free(pstMatchBufInfo);
        pstSva_dev->pstMatchBufInfo = NULL;
    }

    SVA_LOGI("Match Buf DeInit End! chan %d \n", chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvMatchBufInit
 * @brief:      主片（双芯片设备）匹配帧数据初始化
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_DrvInitMatchBuf(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT64 u64PhyAddr = 0;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);

    if ((SAL_TRUE != IA_GetModEnableFlag(IA_MOD_SVA))
        || (SAL_TRUE != IA_GetModTransFlag(IA_MOD_SVA))
        || (DOUBLE_CHIP_SLAVE_TYPE == g_sva_common.uiDevType))
    {
        SVA_LOGI(" dev type %d no need init match buf! return success \n", g_sva_common.uiDevType);
        return SAL_SOK;
    }

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (NULL != pstSva_dev->pstMatchBufInfo)
    {
        SVA_LOGW("Match Buf Info is Initialized! return Success! chan %d \n", chan);
        return SAL_SOK;
    }

    s32Ret = mem_hal_mmzAlloc(sizeof(SVA_MATCH_BUF_INFO), "SVA", "sva_mat_buf", NULL, SAL_FALSE, &u64PhyAddr, (VOID **)&pstSva_dev->pstMatchBufInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("mem_hal_mmzAlloc failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    if (NULL == pstSva_dev->pstMatchBufInfo)
    {
        SVA_LOGE("mem_hal_mmzAlloc failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    sal_memset_s(pstSva_dev->pstMatchBufInfo, sizeof(SVA_MATCH_BUF_INFO), 0x00, sizeof(SVA_MATCH_BUF_INFO));

    pstSva_dev->pstMatchBufInfo->uiCnt = SVA_MAX_MATCH_BUF_NUM;
    pstSva_dev->pstMatchBufInfo->uiW_Idx = 0;

    s32Ret = Sva_DrvAllocMatchBufMem(chan, pstSva_dev->pstMatchBufInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Alloc Match Mem Failed! chan %d \n", chan);
        goto err;
    }

    s32Ret = SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstSva_dev->pstMatchBufInfo->pMutex);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Create Mutex Failed! chan %d \n", chan);
        goto err;
    }

    SVA_LOGI("Match Buf Init End! chan %d \n", chan);
    return SAL_SOK;

err:
    if (pstSva_dev && pstSva_dev->pstMatchBufInfo)
    {
        if (NULL != pstSva_dev->pstMatchBufInfo->pMutex)
        {
            (VOID)SAL_mutexDelete(pstSva_dev->pstMatchBufInfo->pMutex);
            pstSva_dev->pstMatchBufInfo->pMutex = NULL;
        }

        (VOID)Sva_DrvFreeMatchBufMem(chan, pstSva_dev->pstMatchBufInfo);

        if (NULL != pstSva_dev->pstMatchBufInfo)
        {
            mem_hal_mmzFree(pstSva_dev->pstMatchBufInfo, "SVA", "sva_mat_buf");
            pstSva_dev->pstMatchBufInfo = NULL;
        }
    }

    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvDevInit
* 描  述  : 设备模块初始化
* 输  入  : - pDupHandle: 句柄
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvDevInit(void *pDupHandle)
{
    UINT32 chan = 0;

    SVA_DEV_PRM *pstSva_dev = NULL;

    chan = g_sva_common.uiDevCnt;

    SVA_DRV_CHECK_PRM(g_sva_common.uiPublic, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    pstSva_dev->chan = chan;

    if (SAL_SOK != Sva_DrvInitMatchBuf(chan))
    {
        SVA_LOGE("Sva Match Buf Init Failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    if (SAL_SOK != Sva_DrvModuleTsk(chan, g_sva_common.uiDevType))
    {
        SVA_LOGE("Sva_DrvModuleTsk.\n");
        return SAL_FAIL;
    }

#ifdef DSP_ISA  /* fixme: 安检机不需要使用 */
    if (SAL_SOK != Sva_DrvChnInit(chan, pDupHandle))
    {
        SVA_LOGE("Sva Get Handle Failed! chan %d \n", chan);
        return SAL_FAIL;
    }

#endif

    Sva_HalDrawInit(chan, DRAW_MOD_CPU, DRAW_MOD_VGS);

    pstSva_dev->uiInitStatus = SAL_TRUE;

    g_sva_common.uiDevCnt++;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetInitMode
* 描  述  : 初始化模式配置
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static void Sva_DrvSetInitMode(VOID)
{
    SVA_PROC_MODE_E enMode = SVA_PROC_MODE_DUAL_CORRELATION;

    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();     /* TODO: 后续更新能力集配置，取消上层下发通道数 */

    /* CAPB_CAPT *pstViCapPrm = NULL; */
    CAPB_PRODUCT *pstProductPrm = NULL;

    /* pstViCapPrm = capb_get_capt(); */
    pstProductPrm = capb_get_product();
    if (NULL == pstProductPrm)
    {
        SVA_LOGW("get platform capbility fail!");
        return;
    }

    /* Capbility MUST BE Correct! No Checker Here! */
    if (VIDEO_INPUT_INSIDE == pstProductPrm->enInputType)
    {
        enMode = SVA_PROC_MODE_IMAGE;
    }
    else
    {
        if (pDspInitPara->stViInitInfoSt.uiViChn <= 1)     /* if (pstViCapPrm->uiViCnt <= 1) */
        {
            enMode = SVA_PROC_MODE_VIDEO;
        }
    }

    g_proc_mode = enMode;
    g_sva_common.stEngProcPrm.enProcMode = enMode;

    SVA_LOGI("plat input type %d, set mode %d \n", pstProductPrm->enInputType, enMode);

    (void)Sva_HalSetInitMode(enMode);
    return;
}

/**
 * @function    Sva_DrvGetAlgRsltBuf
 * @brief       获取存放算法结果的中间缓存
 * @param[in]   VOID *pstOutput
 * @param[out]
 * @return
 */
static SVA_ALG_RSLT_S *Sva_DrvGetAlgRsltBuf(XSIE_USER_PRVT_INFO *pstUserPrvtInfo)
{
    UINT32 uiEngHdlIdx = 0;

    uiEngHdlIdx = pstUserPrvtInfo->stUsrPrm[0].uiEngHdlIdx;
    if (uiEngHdlIdx >= SVA_DEV_MAX)
    {
        SVA_LOGE("invalid engine handle idx %d \n", uiEngHdlIdx);
        goto err;
    }

    /* 在填充数据前需要先将历史数据清空 */
    sal_memset_s(&g_sva_common.stAlgRslt[uiEngHdlIdx], sizeof(SVA_ALG_RSLT_S), 0x00, sizeof(SVA_ALG_RSLT_S));

    return &g_sva_common.stAlgRslt[uiEngHdlIdx];

err:
    return NULL;
}

/**
 * @function    Sva_DrvCalcSingleFrmProcMs
 * @brief       维护接口-计算单帧处理耗时
 * @param[in]   XSIE_USER_PRVT_INFO *pstUserPrvtInfo
 * @param[out]
 * @return
 */
static INT32 Sva_DrvCalcSingleFrmProcMs(XSIE_USER_PRVT_INFO *pstUserPrvtInfo)
{
    UINT32 uiCbTime = 0;
    UINT32 uiInputTime = 0;

    uiCbTime = SAL_getCurMs();
    uiInputTime = pstUserPrvtInfo->stUsrPrm[0].uiInputTime;

    /* 图片模式打印算法耗时 */
    if (SVA_PROC_MODE_IMAGE == pstUserPrvtInfo->mode)
    {
        if (uiCbTime - uiInputTime > 500)
        {
            SVA_LOGW("alg proc cost: %d ms! attention! \n", uiCbTime - uiInputTime);
            /* todo: 增加调试信息收集，用于智能模块调试 */


        }
        else
        {
            SVA_LOGI("alg proc cost: %d ms \n", uiCbTime - uiInputTime);
        }
    }

    g_sva_common.stDbgPrm.stAlgSts.uiMinMs = (g_sva_common.stDbgPrm.stAlgSts.uiMinMs < uiCbTime - uiInputTime \
                                              ? g_sva_common.stDbgPrm.stAlgSts.uiMinMs \
                                              : uiCbTime - uiInputTime);

    g_sva_common.stDbgPrm.stAlgSts.uiMaxMs = (g_sva_common.stDbgPrm.stAlgSts.uiMaxMs > uiCbTime - uiInputTime \
                                              ? g_sva_common.stDbgPrm.stAlgSts.uiMaxMs \
                                              : uiCbTime - uiInputTime);

    g_sva_common.stDbgPrm.stAlgSts.uiTotalAlgCostMs += (uiCbTime - uiInputTime);
    g_sva_common.stDbgPrm.stAlgSts.uiTotalAlgProcCnt++;

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvCheckXsieCbRsltValidation
 * @brief:      校验xsie回调的结果有效性
 * @param[in]:  XSIE_SECURITY_XSI_OUT_T *pstXsieOut
 * @param[in]:  const CHAR *pcCallerName
 * @param[in]:  const UINT32 uiCallerLine
 * @param[out]: none
 * @return:     none
 */
static VOID Sva_DrvCheckXsieCbRsltValidation(XSIE_SECURITY_XSI_OUT_T *pstXsieOut,
                                             const CHAR *pcCallerName,
                                             const UINT32 uiCallerLine)
{
    BOOL bXsieRsltValid = SAL_TRUE;

    XSIE_SECURITY_XSI_INFO_T *pstXsiOutInfo = NULL;

    pstXsiOutInfo = (XSIE_SECURITY_XSI_INFO_T *)&pstXsieOut->XsiOutInfo;

    /* target num */
    if (pstXsiOutInfo->Alert.MainViewTargetNum > SVA_XSI_MAX_ALARM_NUM
        || pstXsiOutInfo->Alert.SideViewTargetNum > SVA_XSI_MAX_ALARM_NUM)
    {
        bXsieRsltValid = SAL_FALSE;
        SVA_LOGE("invalid target num! MainViewTargetNum %d, SideViewTargetNum %d \n",
                 pstXsiOutInfo->Alert.MainViewTargetNum, pstXsiOutInfo->Alert.SideViewTargetNum);
    }

    int i;

    /* main target */
    for (i = 0; i < pstXsiOutInfo->Alert.MainViewTargetNum; i++)
    {
        if (i > SVA_XSI_MAX_ALARM_NUM)
        {
            bXsieRsltValid = SAL_FALSE;

            SVA_LOGE("invalid main target num %d! i %d, max %d, continue \n",
                     pstXsiOutInfo->Alert.MainViewTargetNum, i, SVA_XSI_MAX_ALARM_NUM);
            continue;
        }

        if (pstXsiOutInfo->Alert.MainViewTarget[i].Type >= SVA_MAX_ALARM_TYPE)
        {
            bXsieRsltValid = SAL_FALSE;

            SVA_LOGE("invalid main target type %d, max %d, i %d, num %d \n",
                     pstXsiOutInfo->Alert.MainViewTarget[i].Type, SVA_MAX_ALARM_TYPE,
                     i, pstXsiOutInfo->Alert.MainViewTargetNum);
        }

        if (pstXsiOutInfo->Alert.MainViewTarget[i].Rect.x < 0.0f || pstXsiOutInfo->Alert.MainViewTarget[i].Rect.y < 0.0f
                                                                                                                    || pstXsiOutInfo->Alert.MainViewTarget[i].Rect.width > 1.0f || pstXsiOutInfo->Alert.MainViewTarget[i].Rect.height > 1.0f)
        {
            bXsieRsltValid = SAL_FALSE;

            SVA_LOGE("invalid main target rect! [%f, %f], [%f, %f] \n",
                     pstXsiOutInfo->Alert.MainViewTarget[i].Rect.x, pstXsiOutInfo->Alert.MainViewTarget[i].Rect.y,
                     pstXsiOutInfo->Alert.MainViewTarget[i].Rect.width, pstXsiOutInfo->Alert.MainViewTarget[i].Rect.height);
        }
    }

    /* side target */
    for (i = 0; i < pstXsiOutInfo->Alert.SideViewTargetNum; i++)
    {
        if (i > SVA_XSI_MAX_ALARM_NUM)
        {
            bXsieRsltValid = SAL_FALSE;

            SVA_LOGE("invalid side target num %d! i %d, max %d, continue \n",
                     pstXsiOutInfo->Alert.SideViewTargetNum, i, SVA_XSI_MAX_ALARM_NUM);
            continue;
        }

        if (pstXsiOutInfo->Alert.SideViewTarget[i].Type >= SVA_MAX_ALARM_TYPE)
        {
            bXsieRsltValid = SAL_FALSE;

            SVA_LOGE("invalid side target type %d, max %d, i %d, num %d \n",
                     pstXsiOutInfo->Alert.SideViewTarget[i].Type, SVA_MAX_ALARM_TYPE,
                     i, pstXsiOutInfo->Alert.SideViewTargetNum);
        }

        if (pstXsiOutInfo->Alert.SideViewTarget[i].Rect.x < 0.0f || pstXsiOutInfo->Alert.SideViewTarget[i].Rect.y < 0.0f
                                                                                                                    || pstXsiOutInfo->Alert.SideViewTarget[i].Rect.width > 1.0f || pstXsiOutInfo->Alert.SideViewTarget[i].Rect.height > 1.0f)
        {
            bXsieRsltValid = SAL_FALSE;

            SVA_LOGE("invalid side target rect! [%f, %f], [%f, %f] \n",
                     pstXsiOutInfo->Alert.SideViewTarget[i].Rect.x, pstXsiOutInfo->Alert.SideViewTarget[i].Rect.y,
                     pstXsiOutInfo->Alert.SideViewTarget[i].Rect.width, pstXsiOutInfo->Alert.SideViewTarget[i].Rect.height);
        }
    }

    /* pack num */
    if (pstXsiOutInfo->Alert.MainViewPackageNum > XSIE_PACKAGE_NUM
        || pstXsiOutInfo->Alert.SideViewPackageNum > XSIE_PACKAGE_NUM
        || pstXsiOutInfo->Alert.MainViewPackDetNum > XSIE_PACKAGE_NUM
        || pstXsiOutInfo->Alert.SideViewPackDetNum > XSIE_PACKAGE_NUM)
    {
        bXsieRsltValid = SAL_FALSE;
        SVA_LOGE("invalid pack num! MainViewPackageNum %d, SideViewPackageNum %d, MainViewPackDetNum %d, SideViewPackDetNum %d \n",
                 pstXsiOutInfo->Alert.MainViewPackageNum, pstXsiOutInfo->Alert.SideViewPackageNum,
                 pstXsiOutInfo->Alert.MainViewPackDetNum, pstXsiOutInfo->Alert.SideViewPackDetNum);
    }

    if (!bXsieRsltValid)
    {
        SVA_LOGE("invalid xsie rslt! caller: %s, line %d \n", pcCallerName, uiCallerLine);
    }

    return;
}

/* 拷贝用户私有数据 */
static INT32 Sva_DrvGetUsrPrvtInfo(VOID *pUsrIn, VOID *pUsrOut)
{
    if (NULL == pUsrIn || NULL == pUsrOut)
    {
        SAL_LOGE("ptr null! %p, %p \n", pUsrIn, pUsrOut);
        return SAL_FAIL;
    }

    sal_memcpy_s(pUsrOut, sizeof(XSIE_USER_PRVT_INFO), pUsrIn, sizeof(XSIE_USER_PRVT_INFO));

    return SAL_SOK;
}

/* 获取算法节点处理结果 */
static INT32 Sva_DrvGetXsiNodeRslt(VOID *pNodeIn, VOID *pNodeOut)
{
    if (NULL == pNodeIn || NULL == pNodeOut)
    {
        SAL_LOGE("ptr null! %p, %p \n", pNodeIn, pNodeOut);
        return SAL_FAIL;
    }

    sal_memcpy_s(pNodeOut, sizeof(XSIE_SECURITY_XSI_OUT_T), pNodeIn, sizeof(XSIE_SECURITY_XSI_OUT_T));

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvDetectResultCb0
* 描  述  : 结果回调函数(通道0)
* 输  入  : INT32     nNodeID        节点ID
*         : INT32     nCallBackType  回调结果类型
*         : void      *pstOutPut     输出结果
*         : UINT32    nSize          大小
*         : void      *pUsr          用户指针
*         : INT32     nUserSize      用户空间大小
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvDetectResultCb0(INT32 nNodeID,
                             INT32 nCallBackType,
                             void *pstOutput,
                             UINT32 nSize,
                             void *pUsr,
                             INT32 nUserSize)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 time3 = 0;
    UINT32 time4 = 0;
    UINT32 time5 = 0;
    UINT32 time6 = 0;

    SVA_ALG_RSLT_S *pstAlgRslt = NULL;

    ENGINE_COMM_OUTPUT_DATA_S stIcfCommOutputRslt = {0};
    XSIE_USER_PRVT_INFO stXsieUserPrvtInfo = {0};
    CAPB_PRODUCT *pstProduct = NULL;

    /* XSIE_SECURITY_XSI_OUT_T stXsieOutInfo = {0}; */

    /* debug info */
    g_sva_common.stDbgPrm.stAlgSts.uiOutputCnt++;
    g_sva_common.stDbgPrm.stCommSts.uiOutputCnt++;

    /* 清空上一次输出结果 */
    sal_memset_s(&g_sva_common.stXsieOutInfo, sizeof(XSIE_SECURITY_XSI_OUT_T), 0x00, sizeof(XSIE_SECURITY_XSI_OUT_T));

    SVA_LOGD("get result! \n");

    time0 = SAL_getCurMs();
    SVA_DRV_CHECK_PTR(pstOutput, alg_err_cnt, "pstOutput == null!");

    /* 控制信息，用于获取用户透传参数 */
    stIcfCommOutputRslt.stOutputCtlPrm.pUserInfo = (VOID *)&stXsieUserPrvtInfo;
    stIcfCommOutputRslt.stOutputCtlPrm.pFuncGetUserInfo = Sva_DrvGetUsrPrvtInfo;

    /* 控制信息，用于获取算法节点结果 */
    stIcfCommOutputRslt.stOutputCtlPrm.enNodeId = ENGINE_COMM_XSI_NODE;
    stIcfCommOutputRslt.stOutputCtlPrm.pAlgRsltInfo = (VOID *)&g_sva_common.stXsieOutInfo;
    stIcfCommOutputRslt.stOutputCtlPrm.pFuncGetNodeRslt = Sva_DrvGetXsiNodeRslt;

    s32Ret = Sva_HalGetIcfCommFuncP()->get_rslt(pstOutput,
                                                &stIcfCommOutputRslt);
    SVA_DRV_CHECK_RET(s32Ret, alg_err_cnt, "get alg rslt failed!");

    /* 计算单帧处理耗时 */
    s32Ret = Sva_DrvCalcSingleFrmProcMs(&stXsieUserPrvtInfo);
    SVA_DRV_CHECK_RET(s32Ret, mod_sts_err, "Sva_DrvCalcSingleFrmProcMs failed!");

    /* 校验引擎返回的结果有效性 */
    Sva_DrvCheckXsieCbRsltValidation(&g_sva_common.stXsieOutInfo, __FUNCTION__, __LINE__);

    /* 获取存放算法返回结果的中间缓存 */
    pstAlgRslt = Sva_DrvGetAlgRsltBuf(&stXsieUserPrvtInfo);
    SVA_DRV_CHECK_PTR(pstAlgRslt, mod_sts_err, "pstAlgRslt == null!");

    time1 = SAL_getCurMs();

    /* 处理第一路: 主视角 */
    s32Ret = Sva_DrvGetOutputRslt(&g_sva_common.stXsieOutInfo, &stXsieUserPrvtInfo, pstAlgRslt);
    SVA_DRV_CHECK_RET(s32Ret, mod_sts_err, "0: Sva_DrvGetOutputRslt failed!");

    time2 = SAL_getCurMs();

    /* 处理第二路: 侧视角 */
    if (SVA_PROC_MODE_DUAL_CORRELATION == pstAlgRslt->stAlgChnRslt[0].enProcMode)
    {
        s32Ret = Sva_DrvGetOutputRslt(&g_sva_common.stXsieOutInfo, &stXsieUserPrvtInfo, pstAlgRslt);
        SVA_DRV_CHECK_RET(s32Ret, mod_sts_err, "1: Sva_DrvGetOutputRslt failed!");
    }

    time3 = SAL_getCurMs();

    /* 释放input附加信息缓存标记 */
    (VOID)Sva_HalRlsPrvtUserInfo(&stXsieUserPrvtInfo);
    time4 = SAL_getCurMs();

    /* 结果后处理，主要实现引擎结果转换、包裹融合、集中判图、包裹匹配和编图等功能 */
    s32Ret = Sva_DrvPostProcess(pstAlgRslt);
    SVA_DRV_CHECK_RET_NO_LOOP(s32Ret, "Sva_DrvPostProcess failed!");
    time5 = SAL_getCurMs();

    /* 跨芯片发送数据(从片) */
    s32Ret = Sva_DrvSendDataDualChip(pstAlgRslt);
    SVA_DRV_CHECK_RET_NO_LOOP(s32Ret, "Sva_DrvSendDataDualChip failed!");

    g_sva_common.stDbgPrm.stCommSts.uiOutputCbEndCnt++;

    pstProduct = capb_get_product();
    if (NULL == pstProduct)
    {
        DISP_LOGE("get platform capblity fail\n");
        return SAL_FAIL;
    }

    if (VIDEO_INPUT_INSIDE == pstProduct->enInputType)
    {
        uiEngineStartTime1 = SAL_getCurMs();
        SVA_LOGI("Time SvaTime(%d)\n", uiEngineStartTime1 - uiEngineStartTime0);
    }

    time6 = SAL_getCurMs();

    /* 60fps对应的是每秒传输一帧耗时16.7ms，阈值设置为17 */
    if (time6 - time0 > 17)
    {
        SVA_LOGW("get output rslt cost %d, %d, %d, %d, %d, total %d ms \n", time1 - time0, time2 - time1, time3 - time2, time4 - time3, time5 - time4, time6 - time0);
    }

    return SAL_SOK;

mod_sts_err:
    (VOID)Sva_HalRlsPrvtUserInfo(&stXsieUserPrvtInfo);

alg_err_cnt:
    g_sva_common.stDbgPrm.stCommSts.uiErrCnt++;
    g_sva_common.stDbgPrm.stAlgSts.uiAlgErrCnt++;    /* 此处处理中临时将算法处理异常次数和模块异常次数合并，后续有需要可以分开 */

    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvRegCbFunc
* 描  述  : Drv层回调函数注册到Hal层
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Sva_DrvRegCbFunc(void)
{
    (VOID)Sva_HalRegCbFunc((SVACALLBACK)Sva_DrvDetectResultCb0);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvCommonInit
* 描  述  : 智能分析通用资源初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvCommonInit(void)
{
    UINT32 i = 0;
    XSI_DEV *pstXsi = NULL;
    DSPINITPARA *pstInit = NULL;

    /* Initialize Mutex Resource */
    for (i = 0; i < XSI_DEV_MAX; i++)
    {
        pstXsi = Sva_HalGetDev(i);
        SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstXsi->mOutMutexHdl);
        SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstXsi->mChnMutexHdl);
    }

    /*创建结果锁(仅双芯片使用)，避免双通道在从片向主片发送智能结果时，同时占用传输通道*/
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &g_sva_common.mRsltMutexHdl);

    (void)Sva_DrvSetInitMode();

    /* Register Result CallBack Function */
    (void)Sva_DrvRegCbFunc();

    pstInit = SystemPrm_getDspInitPara();

    /* 主视角通道号 */
    g_sva_common.uiMainViewChn = 1;
    g_sva_common.uiDevType = pstInit->deviceType;
    g_sva_common.uiVideoChnNum = pstInit->stViInitInfoSt.uiViChn;

    if (SAL_SOK != Sva_DrvSetRemoteDesc(IA_MOD_SVA))
    {
        SVA_LOGE("set remote desc failed! \n");
        return SAL_FAIL;
    }

    /* 重置包裹匹配参数 */
    (VOID)Sva_DrvInitPkgMatchPrm();

    if (SAL_SOK != Sva_DrvDbgInit())
    {
        SVA_LOGE("set remote desc failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvStartEngineInitThread
* 描  述  : 启动引擎相关资源初始化资源
* 输  入  : 无
* 输  出  : 无
* 返回值  : NULL
*******************************************************************************/
void *Sva_DrvEngineInitThread(void *prm)
{
    INT32 s32Ret = SAL_FAIL;

    /* 设置线程名称*/
    SAL_SET_THR_NAME();

    do
    {
        (VOID)Sva_DrvSetInitFlag(SAL_FALSE);
        s32Ret = Sva_HalEngineInit();
        if (s32Ret != SAL_SOK)
        {
            SVA_LOGE("Engine Init Failed! \n");
            goto EXIT;
        }

        (VOID)Sva_DrvSetInitFlag(SAL_TRUE);

        SVA_LOGI("Engine Init End! \n");
    }
    while (0);

EXIT:
    SAL_thrExit(NULL);
    return NULL;
}

/*******************************************************************************
* 函数名  : Sva_DrvStartEngineInitThread
* 描  述  : 启动引擎相关资源初始化资源
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvStartEngineInitThread(void)
{
    SAL_ThrHndl Handle;

    if (SAL_SOK != SAL_thrCreate(&Handle, Sva_DrvEngineInitThread, SAL_THR_PRI_DEFAULT, 2 * 1024 * 1024, NULL))
    {
        SVA_LOGE("Create Engine Init Thread Failed! \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != SAL_thrDetach(&Handle))
    {
        SVA_LOGE("pthread detach failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#ifdef DSP_ISM

/*******************************************************************************
* 函数名  : Sva_DrvModuleInitThread
* 描  述  : 智能通道自启动线程(仅安检机使用)
* 输  入  : 无
* 输  出  : 无
* 返回值  : NULL
*******************************************************************************/
static void *Sva_DrvModuleInitThread(void *prm)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    UINT32 u32TimeOut = 30;  /* 超时30s */

    CAPB_AI *pstAiCapb = NULL;

    /* 设置线程名称*/
    SAL_SET_THR_NAME();

    do
    {
        if (NULL != (pstAiCapb = capb_get_ai()))
        {
            SVA_LOGI("get capb ai chn num %d \n", pstAiCapb->ai_chn);

            /* 此处根据智能通道能力级进行遍历 */
            while (g_sva_common.uiDevCnt < pstAiCapb->ai_chn && u32TimeOut > 0)
            {
                u32TimeOut--;
                sleep(1);
            }

            while (!(Sva_DrvGetInitFlag()) && u32TimeOut > 0)
            {
                u32TimeOut--;
                sleep(1);
            }

            if ((g_sva_common.uiDevCnt < pstAiCapb->ai_chn || !(Sva_DrvGetInitFlag()))
                && 0 == u32TimeOut)
            {
                SVA_LOGE("timeOut! wait 30s for sva dev init! devCnt %d, aiCapCnt %d, init flag %d \n",
                         g_sva_common.uiDevCnt, pstAiCapb->ai_chn, Sva_DrvGetInitFlag());
                goto EXIT;
            }

            /* 开启智能通道 */
            for (i = 0; i < pstAiCapb->ai_chn; i++)
            {
                s32Ret = Sva_DrvModuleInit(i);
                if (SAL_SOK != s32Ret)
                {
                    SVA_LOGW("sva module init failed after engine init! chan %d \n", i);
                }
            }
        }
        else
        {
            SVA_LOGE("capb ai null! \n");
            goto EXIT;
        }
    }
    while (0);

EXIT:
    SAL_thrExit(NULL);
    return NULL;
}

/*******************************************************************************
* 函数名  : Sva_DrvStartThreadModuleInit
* 描  述  : 线程异步自启动智能通道(仅安检机使用)
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Sva_DrvStartThreadModuleInit(void)
{
    SAL_ThrHndl Handle;

    if (SAL_SOK != SAL_thrCreate(&Handle, Sva_DrvModuleInitThread, SAL_THR_PRI_DEFAULT, 2 * 1024 * 1024, NULL))
    {
        SVA_LOGE("Create Engine Init Thread Failed! \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != SAL_thrDetach(&Handle))
    {
        SVA_LOGE("pthread detach failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#endif

#if 1 /* 违禁品类别合并 */
/* 当前最多有21个大类 */
#define SVA_MAX_CLASS_UNIT_NUM (128)

/* 当前每个大类最多包含的子类个数 */
#define SVA_MAX_SUB_CLASS_TYPE_NUM (16)

typedef enum _SVA_CLASS_UNIT_TYPE_
{
    /* 无细分类别大类 */
    NONE_SUB_CLASS_TYPE = 0,
    /* 瓶子容器类 */
    BOTTLE_CLASS_TYPE,
    /* 管制刀具类 */
    KNIFE_CLASS_TYPE,
    /* 电池类 */
    BATTERY_CLASS_TYPE,
    /* 剪刀类 */
    SCISSOR_CLASS_TYPE,
    /* 喷灌类 */
    SPRAY_CAN_CLASS_TYPE,
    /* 打火机类 */
    LIGHTER_CLASS_TYPE,
    /* 枪支类 */
    GUN_CLASS_TYPE,
    /* 枪支零部件类 */
    GUN_PARTS_CLASS_TYPE,
    /* 家用工具类 */
    TOOL_CLASS_TYPE,
    /* 管制器具 */
    STICK_CLASS_TYPE,

    /* extension... */

} SVA_CLASS_UNIT_TYPE_E;

typedef struct _SVA_SUB_CLASS_TYPE_MAP_
{
    /* 是否启用合并 */
    BOOL bEnable;
    /* 违禁品类别ID */
    unsigned int uiType;
    /* 细分子类序号 */
    unsigned int uiSubSeq;
} SVA_SUB_CLASS_TYPE_MAP_S;

typedef struct _SVA_CLASS_UNIT_
{
    /* 大类单元类型 */
    SVA_CLASS_UNIT_TYPE_E enClassUnitType;

    /* 包含子类个数 */
    unsigned int uiSubClassCnt;
    /* 子类细分序列映射信息 */
    SVA_SUB_CLASS_TYPE_MAP_S astSubClassTypeMap[SVA_MAX_SUB_CLASS_TYPE_NUM];
} SVA_CLASS_UNIT_S;

typedef struct _SVA_CLASS_MAP_
{
    /* 当前支持的大类个数 */
    unsigned int uiCnt;
    /* 每个大类单元包含的子类映射信息 */
    SVA_CLASS_UNIT_S astClassUnit[SVA_MAX_CLASS_UNIT_NUM];
} SVA_CLASS_MAP_S;

/* AI目标合并类型映射，当前AI不支持合并，每一类都是单独的类别 */
SVA_CLASS_MAP_S stAiClassMap =
{
    .uiCnt = SVA_AI_MAX_ALARM_TYPE,
};

/* 基础违禁品目标合并类型映射 */
SVA_CLASS_MAP_S stBasicClassMap =
{
    .uiCnt = 21,
    .astClassUnit =
    {
        /* 瓶子容器类 */
        {
            .enClassUnitType = BOTTLE_CLASS_TYPE,
            .uiSubClassCnt = 4,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 0,
                    .uiSubSeq = 1,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 22,
                    .uiSubSeq = 2,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 23,
                    .uiSubSeq = 3,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 24,
                    .uiSubSeq = 4,
                },
            },
        },

        /* 管制刀具类 */
        {
            .enClassUnitType = KNIFE_CLASS_TYPE,
            .uiSubClassCnt = 2,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 1,
                    .uiSubSeq = 1,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 57,
                    .uiSubSeq = 2,
                },
            }

        },

        /* 电池类 */
        {
            .enClassUnitType = BATTERY_CLASS_TYPE,
            .uiSubClassCnt = 4,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 4,
                    .uiSubSeq = 1,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 29,
                    .uiSubSeq = 2,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 40,
                    .uiSubSeq = 3,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 41,
                    .uiSubSeq = 4,
                },
            }

        },

        /* 剪刀类 */
        {
            .enClassUnitType = SCISSOR_CLASS_TYPE,
            .uiSubClassCnt = 3,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 5,
                    .uiSubSeq = 1,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 55,
                    .uiSubSeq = 2,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 56,
                    .uiSubSeq = 3,
                },
            }

        },

        /* 喷灌类 */
        {
            .enClassUnitType = SPRAY_CAN_CLASS_TYPE,
            .uiSubClassCnt = 4,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 6,
                    .uiSubSeq = 1,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 25,
                    .uiSubSeq = 2,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 26,
                    .uiSubSeq = 3,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 58,
                    .uiSubSeq = 3,
                },
            }

        },

        /* 打火机类 */
        {
            .enClassUnitType = LIGHTER_CLASS_TYPE,
            .uiSubClassCnt = 2,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 14,
                    .uiSubSeq = 1,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 27,
                    .uiSubSeq = 2,
                },
            }

        },

        /* 枪支类 */
        {
            .enClassUnitType = GUN_CLASS_TYPE,
            .uiSubClassCnt = 3,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 2,
                    .uiSubSeq = 1,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 21,
                    .uiSubSeq = 2,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 28,
                    .uiSubSeq = 3,
                },
            }

        },

        /* 枪支零部件类 */
        {
            .enClassUnitType = GUN_PARTS_CLASS_TYPE,
            .uiSubClassCnt = 4,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 9,
                    .uiSubSeq = 1,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 10,
                    .uiSubSeq = 2,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 11,
                    .uiSubSeq = 3,
                },
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 12,
                    .uiSubSeq = 4,
                },
            }

        },

        /* 家用工具类 */
        {
            .enClassUnitType = TOOL_CLASS_TYPE,
            .uiSubClassCnt = 10,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 20,
                    .uiSubSeq = 1,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 30,
                    .uiSubSeq = 2,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 31,
                    .uiSubSeq = 3,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 32,
                    .uiSubSeq = 4,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 33,
                    .uiSubSeq = 5,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 34,
                    .uiSubSeq = 6,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 43,
                    .uiSubSeq = 7,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 45,
                    .uiSubSeq = 8,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 50,
                    .uiSubSeq = 9,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 53,
                    .uiSubSeq = 10,
                },
            }

        },

        /* 管制器具类 */
        {
            .enClassUnitType = STICK_CLASS_TYPE,
            .uiSubClassCnt = 6,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 46,
                    .uiSubSeq = 1,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 15,
                    .uiSubSeq = 2,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 17,
                    .uiSubSeq = 3,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 18,
                    .uiSubSeq = 4,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 19,
                    .uiSubSeq = 5,
                },
                {
                    .bEnable = SAL_TRUE,
                    .uiType = 38,
                    .uiSubSeq = 6,
                },
            }

        },

        /* 雨伞 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 3,
                    .uiSubSeq = 0,
                },
            }

        },

        /* 手机 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 7,
                    .uiSubSeq = 0,
                },
            }

        },

        /* 笔记本电脑 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 8,
                    .uiSubSeq = 0,
                },
            }

        },

        /* 子弹 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 13,
                    .uiSubSeq = 0,
                },
            }

        },

        /* 烟花 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 16,
                    .uiSubSeq = 0,
                },
            }

        },

        /* 公章 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 35,
                    .uiSubSeq = 0,
                },
            }

        },

        /* 雷管 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 36,
                    .uiSubSeq = 0,
                },
            }

        },

        /* 香烟 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 37,
                    .uiSubSeq = 0,
                },
            }

        },

        /* 充电宝 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 39,
                    .uiSubSeq = 0,
                },
            }

        },

        /* 利器 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 47,
                    .uiSubSeq = 0,
                },
            }

        },

        /* 指甲剪 */
        {
            .enClassUnitType = NONE_SUB_CLASS_TYPE,
            .uiSubClassCnt = 1,
            .astSubClassTypeMap =
            {
                {
                    .bEnable = SAL_FALSE,
                    .uiType = 48,
                    .uiSubSeq = 0,
                },
            }

        },
    }

};

/**
 * @function:   Sva_DrvInitClassMapTab
 * @brief:      初始化违禁品大类映射表
 * @param[in]:  SVA_CLASS_MAP_S *pstClassMap
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvInitClassMapTab(SVA_CLASS_MAP_S *pstClassMap)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i, j, min;
    SVA_CLASS_UNIT_S *pstClassUnit = NULL;
    SVA_SUB_CLASS_TYPE_MAP_S *pstSubClassTypeMap = NULL;

    SVA_DRV_CHECK_PTR(pstClassMap, exit, "ptr null!");

    if (pstClassMap->uiCnt > SVA_MAX_CLASS_UNIT_NUM)
    {
        SVA_LOGE("invalid class num %d > max %d \n", pstClassMap->uiCnt, SVA_MAX_CLASS_UNIT_NUM);
        goto exit;
    }

    for (i = 0; i < pstClassMap->uiCnt; i++)
    {
        pstClassUnit = &pstClassMap->astClassUnit[i];
        if (pstClassUnit->uiSubClassCnt > SVA_MAX_SUB_CLASS_TYPE_NUM)
        {
            SVA_LOGE("invalid sub class cnt %d > max %d, class type %d \n",
                     pstClassUnit->uiSubClassCnt, SVA_MAX_SUB_CLASS_TYPE_NUM, pstClassUnit->enClassUnitType);
            goto exit;
        }

        if (0 == pstClassUnit->uiSubClassCnt)
        {
            SVA_LOGE("zerooooo sub class cnt! i %d \n", i);
            continue;
        }

        /* 若大类即细分类别，则细分序列号为0 */
        if (NONE_SUB_CLASS_TYPE == pstClassUnit->enClassUnitType)
        {
            /* 不支持合并的类别使用自身作为标识 */
            au32ClassSubMinIdxTab[pstClassUnit->astSubClassTypeMap[0].uiType] = pstClassUnit->astSubClassTypeMap[0].uiType;

            SVA_LOGD("i %d, NONE!!! type %d \n", i, pstClassUnit->astSubClassTypeMap[0].uiType);
            continue;
        }

        /* 获取当前所属合并大类中的最小类别id，标识当前大类中的所有类别 */
        for (min = 1024, j = 0; j < pstClassUnit->uiSubClassCnt; j++)
        {
            if (min > pstClassUnit->astSubClassTypeMap[j].uiType)
            {
                min = pstClassUnit->astSubClassTypeMap[j].uiType;
            }
        }

        for (j = 0; j < pstClassUnit->uiSubClassCnt; j++)
        {
            SVA_LOGD("j %d, EXIST!!! type %d \n", j, pstClassUnit->astSubClassTypeMap[j].uiType);

            pstSubClassTypeMap = &pstClassUnit->astSubClassTypeMap[j];

            if (pstSubClassTypeMap->uiType >= SVA_MAX_ALARM_TYPE)
            {
                SVA_LOGE("invalid type %d >= max %d \n", pstSubClassTypeMap->uiType, SVA_MAX_ALARM_TYPE);
                goto exit;
            }

            au32ClassSubIdxTab[pstSubClassTypeMap->uiType] = pstSubClassTypeMap->uiSubSeq;

            /* 大类中的部分子类未使能合并 */
            if (SAL_TRUE != pstSubClassTypeMap->bEnable)
            {
                /* 不支持合并的类别使用自身作为标识 */
                au32ClassSubMinIdxTab[pstSubClassTypeMap->uiType] = pstSubClassTypeMap->uiType;

                SVA_LOGD("j %d, merge not enable!!! type %d \n", j, pstSubClassTypeMap->uiType);
                continue;
            }

            /* 支持合并的类别使用所属大类中最小的类别id作为标识 */
            au32ClassSubMinIdxTab[pstSubClassTypeMap->uiType] = min;
        }
    }

    s32Ret = SAL_SOK;
    SVA_LOGI("init class map table end! \n");

exit:
    return s32Ret;
}

/**
 * @function:   Sva_DrvPrClassMap
 * @brief:      打印大类映射表
 * @param[in]:  None
 * @param[out]: None
 * @return:     INT32
 */
VOID Sva_DrvPrClassMap(VOID)
{
    UINT32 i, j;
    SVA_CLASS_UNIT_S *pstClassUnit = NULL;

    for (i = 0; i < stBasicClassMap.uiCnt; i++)
    {
        pstClassUnit = &stBasicClassMap.astClassUnit[i];
        SVA_LOGW("i %d, \n", i);

        for (j = 0; j < pstClassUnit->uiSubClassCnt; j++)
        {
            /* 若大类即细分类别，则细分序列号为0 */
            if (NONE_SUB_CLASS_TYPE == pstClassUnit->enClassUnitType)
            {
                SVA_LOGE("j %d, NONE!!! type %d \n", j, pstClassUnit->astSubClassTypeMap[j].uiType);
                continue;
            }

            SVA_LOGI("j %d, EXIST!!! type %d \n", j, pstClassUnit->astSubClassTypeMap[j].uiType);
        }
    }

    for (i = 0; i < stAiClassMap.uiCnt; i++)
    {
        pstClassUnit = &stAiClassMap.astClassUnit[i];
        SVA_LOGW("i %d, \n", i);

        for (j = 0; j < pstClassUnit->uiSubClassCnt; j++)
        {
            /* 若大类即细分类别，则细分序列号为0 */
            if (NONE_SUB_CLASS_TYPE == pstClassUnit->enClassUnitType)
            {
                SVA_LOGE("j %d, NONE!!! type %d \n", j, pstClassUnit->astSubClassTypeMap[j].uiType);
                continue;
            }

            SVA_LOGI("j %d, EXIST!!! type %d \n", j, pstClassUnit->astSubClassTypeMap[j].uiType);
        }
    }

    for (i = 0; i < SVA_MAX_ALARM_TYPE; i++)
    {
        if (i % 10 == 0)
        {
            printf("\n");
        }

        printf("%d ", au32ClassSubIdxTab[i]);
    }

    printf("=============== merge type ================ \n");
    for (i = 0; i < SVA_MAX_ALARM_TYPE; i++)
    {
        if (i % 10 == 0)
        {
            printf("\n");
        }

        printf("%d ", au32ClassSubMinIdxTab[i]);
    }

    return;
}

#endif

/**
 * @function   Sva_DrvInitAiClassMap
 * @brief      初始化AI违禁品目标的合并类别映射
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvInitAiClassMap(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    SVA_CLASS_UNIT_S stClassUnit = {0};

    stClassUnit.enClassUnitType = NONE_SUB_CLASS_TYPE;
    stClassUnit.uiSubClassCnt = 1;

    /* checker */
    if (SVA_AI_MAX_ALARM_TYPE != stAiClassMap.uiCnt)
    {
        SVA_LOGW("global ai class map cnt err! \n");
        stAiClassMap.uiCnt = SVA_AI_MAX_ALARM_TYPE;
    }

    /* step1: 初始化AI全局映射表 */
    for (i = 0; i < SVA_AI_MAX_ALARM_TYPE; i++)
    {
        /* AI类别索引基于基础违禁品最大类别值累加 */
        stClassUnit.astSubClassTypeMap[0].uiType = SVA_XSI_MAX_ALARM_TYPE + i;
        stClassUnit.astSubClassTypeMap[0].uiSubSeq = 0;

        sal_memcpy_s(&stAiClassMap.astClassUnit[i], sizeof(SVA_CLASS_UNIT_S), &stClassUnit, sizeof(SVA_CLASS_UNIT_S));
    }

    /* step2: 更新全局合并映射表 */
    s32Ret = Sva_DrvInitClassMapTab(&stAiClassMap);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Sva_DrvInitClassMapTab failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   Sva_DrvInitGlobalClassMapTab
 * @brief      初始化全局违禁品合并类别映射表
 * @param[in]  VOID
 * @param[out] None
 * @return     static INT32
 */
static INT32 Sva_DrvInitGlobalClassMapTab(VOID)
{
    /* 初始化基础类别违禁品目标的合并映射表 */
    if (SAL_SOK != Sva_DrvInitClassMapTab(&stBasicClassMap))
    {
        SVA_LOGE("Sva_DrvInitClassMapTab failed! \n");
        return SAL_FAIL;
    }

    /* 初始化AI违禁品目标的合并映射表，当前不支持AI合并 */
    if (SAL_SOK != Sva_DrvInitAiClassMap())
    {
        SVA_LOGE("Sva_DrvInitAiClassMap failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   Sva_DrvResetTrans
 * @brief      重置通信模块
 * @param[in]  UINT32 chan
 * @param[in]  VOID *sprm
 * @param[out] None
 * @return     INT32
 */
INT32 Sva_DrvResetTrans(UINT32 chan, VOID *prm)
{
    INT32 s32Ret = SAL_FAIL;

    /* 去初始化主片发送帧数据的business device */
    if (g_sva_common.stXtransPrm.pstBusiDevSendFrm)
    {
        s32Ret = g_sva_common.stXtransPrm.pstBusiDevSendFrm->deinit(g_sva_common.stXtransPrm.pstBusiDevSendFrm);
        s32Ret |= xtrans_destory_bus_dev(g_sva_common.stXtransPrm.pstBusiDevSendFrm);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("destroy master send frm business device failed! \n");
        }

        /* reset device ptr null */
        g_sva_common.stXtransPrm.pstBusiDevSendFrm = NULL;
    }

    /* 去初始化主片接收帧结果的business device */
    if (g_sva_common.stXtransPrm.pstBusiDevSendRslt)
    {
        s32Ret = g_sva_common.stXtransPrm.pstBusiDevSendRslt->deinit(g_sva_common.stXtransPrm.pstBusiDevSendRslt);
        s32Ret |= xtrans_destory_bus_dev(g_sva_common.stXtransPrm.pstBusiDevSendRslt);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("destroy master recv rslt business device failed! \n");
        }

        /* reset device ptr null */
        g_sva_common.stXtransPrm.pstBusiDevSendRslt = NULL;
    }

    /* 重新初始化xtrans */
    s32Ret = xtrans_Init();
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("xtrans init failed! \n");
        return SAL_FAIL;
    }

    SVA_LOGI("reset xtrans end! \n");
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvInit
* 描  述  : 业务层初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvInit(void)
{
    INT32 s32Ret = SAL_FAIL;

    DSPINITPARA *pstInit = NULL;

    pstInit = SystemPrm_getDspInitPara();

    s32Ret = Sva_DrvCommonInit();
    if (s32Ret != SAL_SOK)
    {
        SVA_LOGE("Hal Layer Common Init Failed! \n");
        return SAL_FAIL;
    }

    /* 初始化全局违禁品类别的合并映射表，包含基础违禁品和AI违禁品，当前基础违禁品53类合并为21类，AI不支持合并 */
    s32Ret = Sva_DrvInitGlobalClassMapTab();
    if (s32Ret != SAL_SOK)
    {
        SVA_LOGE("Sva_DrvInitGlobalClassMapTab Failed! \n");
        return SAL_FAIL;
    }

    /* Sva_DrvPrClassMap(); */

    /* 双芯片通信相关统计耗时初始化 */
    if (SINGLE_CHIP_TYPE != pstInit->deviceType)
    {
        s32Ret = Sva_DrvInitTransDbgPrm(&stSvaTransDbgPrm);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("init trans dbg prm failed! \n");
            return SAL_FAIL;
        }
    }

    /* TODO: 与基线主片不运行安检智能XSIE进行兼容 */
    if (SAL_TRUE == IA_GetAlgRunFlag(IA_MOD_SVA))
    {
        #if 1 /* multi-thread init */
        s32Ret = Sva_DrvStartEngineInitThread();
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Start Engine Init Thread Failed! \n");
            g_sva_common.uiPublic = SAL_FALSE;
            return SAL_FAIL;
        }

        #else /* single-thread init */
        (VOID)Sva_DrvSetInitFlag(SAL_FALSE);
        s32Ret = Sva_HalEngineInit();
        if (SAL_SOK != s32Ret)
        {
            SAL_ERROR("Engine Init Failed! \n");
            return SAL_FAIL;
        }

        (VOID)Sva_DrvSetInitFlag(SAL_TRUE);
        #endif
    }
    else
    {
        (VOID)Sva_DrvSetInitFlag(SAL_TRUE);
    }

    /* 自启动模块通道，仅安检机使用 */
#ifdef DSP_ISM
    (VOID)Sva_DrvStartThreadModuleInit();
#endif

    g_sva_common.uiPublic = SAL_TRUE;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvDbgSaveJpg
* 描  述  : 设置保存图片信息开关
* 输  入  : - chan: 通道号
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_DrvDbgSaveJpg(UINT32 chan)
{
    UINT32 i = 0;

    if (chan > 1)
    {
        SVA_LOGE("Invalid chan %d \n", chan);
        return;
    }

    gDbgSaveJpg[chan] = 1 - gDbgSaveJpg[chan];

    for (i = 0; i < 2; i++)
    {
        SVA_LOGI("chan %d: switch %d \n", i, gDbgSaveJpg[i]);
    }

    return;
}

/**
 * @function    Sva_DrvChgPosWriteFlag
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Sva_DrvChgPosWriteFlag(UINT32 uiFlag)
{
    IA_ISA_UPDATA_CFG_JSON_PARA stUpJsonPrm = {0};

    if (uiFlag > SAL_TRUE)
    {
        uiFlag = SAL_TRUE;
    }

    guiPosWriteFlag = uiFlag;

    /* Modify Cfg Json Mode Enbale Switch */
    stUpJsonPrm.enChgType = CHANGE_POS_WRITE_FLAG;
    stUpJsonPrm.uiPosWriteFlag = uiFlag;
    if (SAL_SOK != Sva_IsaHalModifyCfgFile(&stUpJsonPrm))
    {
        SVA_LOGE("Modify Cfg File Failed! Pos Write flag %d \n", stUpJsonPrm.uiAiEnable);
    }

    SVA_LOGI("Chane Cfg Json Pos Write Flag, Flag is %d \n", uiFlag);
    return;
}

/*******************************************************************************
* 函数名  : Sva_DrvDbgSampleSwitch
* 描  述  : 设置素材采集开关(调试使用)
* 输  入  : chan: 通道号
*         : pPrm: 开关状态
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvDbgSampleSwitch(UINT32 chan, void *pPrm)
{
    UINT32 uiSwitch = 0;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pPrm, SAL_FAIL);

    uiSwitch = *(UINT32 *)pPrm;

    uiSwitch = uiSwitch > SAL_TRUE ? SAL_TRUE : uiSwitch;

    gDbgSampleCollect[chan] = uiSwitch;

    SVA_LOGI("chan %d: sample switch %d \n", chan, uiSwitch);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetInputGapNum
* 描  述  : 获取帧处理间隔
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
UINT32 Sva_DrvGetInputGapNum(void)
{
    return g_uiInputGapNum;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetInputGapNum
* 描  述  : 设置帧处理间隔
* 输  入  : pGapNum : 帧间隔
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_DrvSetInputGapNum(void *pGapNum)
{
    UINT32 uiGapNum = 0;

    SVA_DRV_CHECK_PRM(pGapNum, SAL_FAIL);

    uiGapNum = *(UINT32 *)pGapNum;

    /* 当前帧间隔只支持全帧率(gapNum为1)和隔帧检测(gapNum为2) */
    if (0 == uiGapNum)
    {
        SVA_LOGW("Invalid gap num %d, set default gap num 2! \n", uiGapNum);
        uiGapNum = 1;
    }

    g_uiInputGapNum = uiGapNum;

    SVA_LOGI("Set Input Gap Num %d End! \n", g_uiInputGapNum);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetDbLevel
* 描  述  : 设置调试打印等级
* 输  入  : uiDbgLevel : 调试打印等级
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
void Sva_DrvSetDbLevel(UINT32 uiDbgLevel)
{
    (VOID)Sva_HalSetDbLevel(uiDbgLevel);
    return;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetDCtestLevel
* 描  述  : 设置双芯片调试开关
* 输  入  : uiFlag         : 调试总开关
*         : uiYuvFlag      : 导出yuv调试开关
*         : uiInputFrmRate : 数据输入帧率
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
void Sva_DrvSetDCtestLevel(UINT32 uiFlag, UINT32 uiYuvFlag, UINT32 uiInputFrmRate)
{
    uiSwitch[0] = uiFlag;
    uiSwitch[1] = uiYuvFlag;
    uiSwitch[2] = uiInputFrmRate;

    return;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetDCtestLevel
* 描  述  : 获取双芯片调试开关
* 输  入  : idx : 开关索引值
* 输  出  : 无
* 返回值  : 索引值对应的开关状态
*******************************************************************************/
UINT32 Sva_DrvGetDCtestLevel(UINT32 idx)
{
    return uiSwitch[idx];
}

/*******************************************************************************
* 函数名  : yuv_dump
* 描  述  : 生成推帧的yuv文件
* 输  入  : pYuv : 自动化测试使用的图片数据
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 入参有效性由内部接口保证，此处不进行校验
*******************************************************************************/
int yuv_dump(CHAR *pYuv, int id, int width, int height)
{
    FILE *fp = NULL;
    char path[64] = {0};

    if (NULL == pYuv)
    {
        SAL_ERROR("Input pYuv is empty!!!\n");
        return SAL_FAIL;
    }

    sprintf(path, "/mnt/nfs/bmp/test_%d.yuv", id);

    fp = fopen(path, "w+");
    if (!fp)
    {
        printf("fopen %s failed! \n", path);
        return SAL_FAIL;
    }

    fwrite(pYuv, width * height * 3 / 2, 1, fp);
    fflush(fp);

    fclose(fp);
    fp = NULL;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvSetScaleChnPrm
* 描  述  : 设置vpss缩放参数
* 输  入  : UINT32 chan
          : void *prm
          : UINT32 w
          : UINT32 h
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 入参有效性由内部接口保证，此处不进行校验
*******************************************************************************/
INT32 Sva_DrvSetScaleChnPrm(UINT32 chan, void *prm)
{
    SVA_DEV_PRM *pstSva_dev = NULL;
    DUP_ChanHandle *pDupChnHandle = NULL;

    PARAM_INFO_S stParamInfo = {0};

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    /* 保存镜像的vproc handle */
    pstSva_dev->DupHandle.pVpScaleHandle = prm;

    pDupChnHandle = (DUP_ChanHandle *)prm;

    stParamInfo.enType = IMAGE_SIZE_CFG;
    stParamInfo.stImgSize.u32Width = SVA_MODULE_WIDTH;
    stParamInfo.stImgSize.u32Height = SVA_MODULE_HEIGHT;

    if (SAL_SOK != pDupChnHandle->dupOps.OpDupSetBlitPrm((VOID *)pDupChnHandle, &stParamInfo))
    {
        /* 非关键步骤，无需返回失败 */
        SVA_LOGE("Set Vpss Chn Resolution Failed! \n");
        return SAL_FAIL;
    }

    SVA_LOGI("get vpss chn w %d, h %d, type %d\n ", stParamInfo.stImgSize.u32Width, stParamInfo.stImgSize.u32Height, stParamInfo.enType);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_DrvGetScaleFrame
* 描  述  : 获取vpss缩放帧
* 输  入  : SVA_DEV_PRM *pstSva_dev
          : SYSTEM_FRAME_INFO *pstSysFrame
          : UINT32 w
          : UINT32 h
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 入参有效性由内部接口保证，此处不进行校验
*******************************************************************************/
static INT32 Sva_DrvGetScaleFrame(SVA_DEV_PRM *pstSva_dev,
                                  SYSTEM_FRAME_INFO *pstSrcFrame,
                                  SYSTEM_FRAME_INFO *pstDstFrame)
{
    INT32 s32Ret = SAL_SOK;

    DUP_ChanHandle *pDupChnHandle = NULL;
    PARAM_INFO_S stParamInfo = {0};
    DUP_COPY_DATA_BUFF stDupDataBuf = {0};

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstSrcFrame, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pstDstFrame, SAL_FAIL);

    pDupChnHandle = (DUP_ChanHandle *)pstSva_dev->DupHandle.pVpScaleHandle;

    sal_memset_s(&stParamInfo, sizeof(PARAM_INFO_S), 0x00, sizeof(PARAM_INFO_S));

    stParamInfo.enType = IMAGE_SIZE_CFG;
    s32Ret = pDupChnHandle->dupOps.OpDupGetBlitPrm((VOID *)pDupChnHandle, &stParamInfo);
    SVA_DRV_CHECK_RET(s32Ret, err, "Get Vpss Chn Resolution Failed! \n");

    s32Ret = pDupChnHandle->dupOps.OpDupStartBlit(pDupChnHandle);
    SVA_DRV_CHECK_RET(s32Ret, err, "start Vpss Chn Resolution Failed! \n");

    s32Ret = dup_task_sendToDup(pDupChnHandle->dupModule, pstSrcFrame);
    SVA_DRV_CHECK_RET(s32Ret, err, "send to dup failed! ret 0x%x \n");

    stDupDataBuf.pstDstSysFrame = pstDstFrame;
    s32Ret = pDupChnHandle->dupOps.OpDupGetBlit((VOID *)pDupChnHandle, &stDupDataBuf);
    SVA_DRV_CHECK_RET(s32Ret, err, "Get scare Frame Failed! ret 0x%x \n");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvPutScaleFrame
* 描  述  : 释放vpss缩放帧
* 输  入  : SVA_DEV_PRM *pstSva_dev
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 入参有效性由内部接口保证，此处不进行校验
*******************************************************************************/
static INT32 Sva_DrvPutScaleFrame(SVA_DEV_PRM *pstSva_dev, SYSTEM_FRAME_INFO *pstSysFrameInfo)
{
    INT32 s32Ret = SAL_SOK;

    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    s32Ret = Sva_HalPutFrame(pstSva_dev->DupHandle.pVpScaleHandle, pstSysFrameInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("put frame faield! chan %d \n", pstSva_dev->chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvSoftCopyYuv
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Sva_DrvSoftCopyYuv(SYSTEM_FRAME_INFO *pstSrc, SYSTEM_FRAME_INFO *pstDst)
{
    INT32 j = 0;
    UINT32 s32Ret = 0;
    CHAR *srcY = NULL;
    CHAR *srcVU = NULL;
    CHAR *dstY = NULL;
    CHAR *dstVU = NULL;
    SAL_VideoFrameBuf stSrcVideoFrmBuf = {0};
    SAL_VideoFrameBuf stDstVideoFrmBuf = {0};

    if (pstSrc == NULL || pstDst == NULL)
    {
        SAL_LOGE("input prm is NULL \n");
        goto err;
    }

    s32Ret = sys_hal_getVideoFrameInfo(pstSrc, &stSrcVideoFrmBuf);
    SVA_DRV_CHECK_RET(s32Ret, err, "get video frame info st failed! \n");

    s32Ret = sys_hal_getVideoFrameInfo(pstDst, &stDstVideoFrmBuf);
    SVA_DRV_CHECK_RET(s32Ret, err, "get video frame info st failed! \n");

    srcY = (char *)stSrcVideoFrmBuf.virAddr[0];
    srcVU = (char *)stSrcVideoFrmBuf.virAddr[1];
    dstY = (char *)stDstVideoFrmBuf.virAddr[0];
    dstVU = (char *)stDstVideoFrmBuf.virAddr[1];

    SAL_LOGD("srcy %p dsty %p w %d h %d s %d d %d\n", srcY, dstY, stSrcVideoFrmBuf.frameParam.width, stSrcVideoFrmBuf.frameParam.height, stSrcVideoFrmBuf.stride[0], stDstVideoFrmBuf.stride[0]);
    for (j = 0; j < stSrcVideoFrmBuf.frameParam.height; j++)
    {
        memcpy(dstY, srcY, stSrcVideoFrmBuf.frameParam.width);
        srcY += stSrcVideoFrmBuf.stride[0];
        dstY += stDstVideoFrmBuf.stride[0];
        if (j % 2 == 0)
        {
            memcpy(dstVU, srcVU, stSrcVideoFrmBuf.frameParam.width);

            srcVU += stSrcVideoFrmBuf.stride[0];
            dstVU += stDstVideoFrmBuf.stride[0];
        }
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : Sva_DrvInputAutoTestPic
* 描  述  : 送入BMP图片用于算法智能测试使用 
* 输  入  : pData : 自动化测试使用的图片数据
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
* 说  明  : 入参有效性由内部接口保证，此处不进行校验
*******************************************************************************/
INT32 Sva_DrvInputAutoTestPic(UINT32 chan, CHAR *pData)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time2 = 0;
    UINT32 uiChnSts[SVA_DEV_MAX] = {0, 0};
    UINT32 uiSourcePicW = 0;
    UINT32 uiSourcePicH = 0;

    VDEC_TSK_PIC_PRM stAutoPci = {0};
    SAL_VideoFrameBuf stVideoFrmBuf = {0};
    SVA_PROCESS_IN stIn[SVA_DEV_MAX] = {0};
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(pData, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    /* TODO: 资源和引擎模式异常判断 */
    if (SAL_TRUE != Sva_DrvGetInitFlag())
    {
        SAL_WARN("Engine Init Is Not Finished! Pls Wait! chan %d \n", chan);
        return SAL_FAIL;
    }

    if (SVA_PROC_MODE_IMAGE != g_proc_mode)
    {
        SAL_ERROR("mode %d not image type! \n", g_proc_mode);
        return SAL_FAIL;
    }

    if (0x00 == pstSva_dev->stAtSysFrame[0].uiAppData)
    {
        s32Ret = Sva_DrvGetFrameMem(SVA_BUF_WIDTH_MAX, SVA_MODULE_WIDTH, SAL_FALSE, &pstSva_dev->stAtSysFrame[0]);
        SVA_DRV_CHECK_RET(s32Ret, err, "Get Frame MMZ failed \n");
    }

    s32Ret = sys_hal_getVideoFrameInfo(&pstSva_dev->stAtSysFrame[0], &stVideoFrmBuf);
    SVA_DRV_CHECK_RET(s32Ret, err, "get video frame info st failed! \n");
    sal_memset_s((void *)stVideoFrmBuf.virAddr[0], SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH, 0xff, SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH);
    sal_memset_s((void *)stVideoFrmBuf.virAddr[0] + SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH,
                 SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH / 2, 0x80, SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH / 2);

    if (0x00 == pstSva_dev->stAtSysFrame[2].uiAppData)
    {
        s32Ret = Sva_DrvGetFrameMem(SVA_MODULE_WIDTH, SVA_MODULE_HEIGHT, SAL_FALSE, &pstSva_dev->stAtSysFrame[2]);
        SVA_DRV_CHECK_RET(s32Ret, err, "Get Frame MMZ failed \n");
    }

    s32Ret = sys_hal_getVideoFrameInfo(&pstSva_dev->stAtSysFrame[2], &stVideoFrmBuf);
    SVA_DRV_CHECK_RET(s32Ret, err, "get video frame info st failed! \n");
    sal_memset_s((void *)stVideoFrmBuf.virAddr[0], SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT,
                 0xff, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT);
    sal_memset_s((void *)stVideoFrmBuf.virAddr[0] + SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT,
                 SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT / 2, 0x80, SVA_MODULE_WIDTH * SVA_MODULE_HEIGHT / 2);


    time0 = SAL_getCurMs();

    /* TODO: 帧数据初始化 */
    s32Ret = sys_hal_getVideoFrameInfo(&pstSva_dev->stAtSysFrame[0], &stVideoFrmBuf);
    SVA_DRV_CHECK_RET(s32Ret, err, "sys_hal_getVideoFrameInfo failed!");
    s32Ret = vdec_tsk_Bmp2Nv21(pData, (CHAR *)stVideoFrmBuf.virAddr[0], (void *)&stAutoPci);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("Get Yuv Data Failed! chan %d \n", chan);
        goto err;
    }

    time1 = SAL_getCurMs();

    SAL_INFO("autopci: wxh [%dx%d], u32Stride[0] %d, u32Height %d, viraddr0 %lld, viraddr1 %lld, viraddr2 %lld, phyaddr0 %lld, phyaddr1 %lld, phyaddr2 %lld \n ",
             stAutoPci.uiW, stAutoPci.uiH, stVideoFrmBuf.stride[0], stVideoFrmBuf.frameParam.height,
             stVideoFrmBuf.virAddr[0], stVideoFrmBuf.virAddr[1], stVideoFrmBuf.virAddr[2],
             stVideoFrmBuf.phyAddr[0], stVideoFrmBuf.phyAddr[1], stVideoFrmBuf.phyAddr[2]);

    /* TODO: 原始数据赋值给 SysFrame0     */
    stVideoFrmBuf.stride[0] = stAutoPci.uiW;
    stVideoFrmBuf.stride[1] = stAutoPci.uiW;
    stVideoFrmBuf.stride[2] = stAutoPci.uiW;
    stVideoFrmBuf.frameParam.width = stAutoPci.uiW;
    stVideoFrmBuf.frameParam.height = stAutoPci.uiH;
    stVideoFrmBuf.virAddr[1] = stVideoFrmBuf.virAddr[0] + stAutoPci.uiW * stAutoPci.uiH;
    stVideoFrmBuf.virAddr[2] = stVideoFrmBuf.virAddr[1];
    stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + stAutoPci.uiW * stAutoPci.uiH;

    s32Ret = sys_hal_buildVideoFrame(&stVideoFrmBuf, &pstSva_dev->stAtSysFrame[0]);
    SVA_DRV_CHECK_RET(s32Ret, err, "get video frame info st failed! ");


    /* TODO: 判断图片分辨率，是否做裁剪 */
    if (stAutoPci.uiW <= SVA_MODULE_WIDTH && stAutoPci.uiH <= SVA_MODULE_HEIGHT)
    {
        /* 拷贝帧数据 */
        pstSva_dev->stAtSysFrame[2].uiDataWidth = stAutoPci.uiW;
        pstSva_dev->stAtSysFrame[2].uiDataHeight = stAutoPci.uiH;
        s32Ret = Ia_TdeQuickCopy(&pstSva_dev->stAtSysFrame[0], &pstSva_dev->stAtSysFrame[2],
                                 0, 0, SAL_alignDown(stAutoPci.uiW, 16), SAL_alignDown(stAutoPci.uiH, 2), SAL_FALSE);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGW("Ia_TdeQuickCopy failed!,use softcopy w %d, h %d \n", stAutoPci.uiW, stAutoPci.uiH);

            /* TODO:使用软拷贝  */
            s32Ret = Sva_DrvSoftCopyYuv(&pstSva_dev->stAtSysFrame[0], &pstSva_dev->stAtSysFrame[2]);
            SVA_DRV_CHECK_RET(s32Ret, err, "get video frame info st failed! \n");
        }

        uiSourcePicW = stAutoPci.uiW;
        uiSourcePicH = stAutoPci.uiH;
    }
    else
    {
        if (0x00 == pstSva_dev->stAtSysFrame[1].uiAppData)
        {
            s32Ret = Sva_DrvGetFrameMem(SVA_BUF_WIDTH_MAX, SVA_MODULE_WIDTH, SAL_FALSE, &pstSva_dev->stAtSysFrame[1]);
            SVA_DRV_CHECK_RET(s32Ret, err, "Get Frame MMZ failed \n");
        }

        s32Ret = sys_hal_getVideoFrameInfo(&pstSva_dev->stAtSysFrame[1], &stVideoFrmBuf);
        SVA_DRV_CHECK_RET(s32Ret, err, "get video frame info st failed! \n");
        sal_memset_s((void *)stVideoFrmBuf.virAddr[0], SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH, 0xff, SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH);
        sal_memset_s((void *)stVideoFrmBuf.virAddr[0] + SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH,
                     SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH / 2, 0x80, SVA_BUF_WIDTH_MAX * SVA_MODULE_WIDTH / 2)
        ;
        /* SVA_LOGE("ywn 1111:stride[0] %d, stAutoPci.uiW allon %d  \n", stVideoFrmBuf.stride[0], SAL_alignDown(stAutoPci.uiW, 16)); */

        pstSva_dev->stAtSysFrame[1].uiDataWidth = stAutoPci.uiW;
        pstSva_dev->stAtSysFrame[1].uiDataHeight = stAutoPci.uiH;

        s32Ret = Ia_TdeQuickCopy(&pstSva_dev->stAtSysFrame[0], &pstSva_dev->stAtSysFrame[1],
                                 0, 0, SAL_alignDown(stAutoPci.uiW, 16), SAL_alignDown(stAutoPci.uiH, 2), SAL_FALSE);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGW("Ia_TdeQuickCopy failed!, use softcopy w %d, h %d \n", stAutoPci.uiW, stAutoPci.uiH);
            /* TODO:使用软拷贝  */
            s32Ret = Sva_DrvSoftCopyYuv(&pstSva_dev->stAtSysFrame[0], &pstSva_dev->stAtSysFrame[1]);
            SVA_DRV_CHECK_RET(s32Ret, err, "get video frame info st failed! \n");
        }

        /* TODO:1600x1280图片等比例0.8倍缩放1280x1024,发送并获取帧数据,裁剪到目的帧缓存buf */
        s32Ret = Sva_DrvGetScaleFrame(pstSva_dev, &pstSva_dev->stAtSysFrame[1], &pstSva_dev->stAtSysFrame[2]);
        SVA_DRV_CHECK_RET(s32Ret, err1, "build video frame info st failed! \n");

        uiSourcePicW = (UINT32)((float)stAutoPci.uiW * 0.8);
        uiSourcePicH = (UINT32)((float)stAutoPci.uiH * 0.8);
    }

    /* 推送数据给引擎 */
    pstSva_dev->stAtSysFrame[2].uiDataWidth = SVA_MODULE_WIDTH;
    pstSva_dev->stAtSysFrame[2].uiDataHeight = SVA_MODULE_HEIGHT;

    uiChnSts[chan] = SAL_TRUE;
    (VOID)Sva_DrvGetCfgParam(chan, &stIn[chan]);

    /* TODO: 图片模式，送入算法进行智能分析 */
    s32Ret = Sva_HalVcaeSyncImgmodePutData(&pstSva_dev->stAtSysFrame[2], &stIn[0], &uiChnSts[0], 1, SVA_PROC_MODE_IMAGE, uiSourcePicW, uiSourcePicH);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("Send Data Failed! chan %d \n", chan);
        goto err1;
    }

    if (stAutoPci.uiW > SVA_MODULE_WIDTH || stAutoPci.uiH > SVA_MODULE_HEIGHT)
    {
        s32Ret = Sva_DrvPutScaleFrame(pstSva_dev, &pstSva_dev->stAtSysFrame[2]);
        SVA_DRV_CHECK_RET_NO_LOOP(s32Ret, "put scale frame failed! \n");
    }

    time2 = SAL_getCurMs();

    SAL_INFO("bmp 2 nv21 cost %d ms, total %d ms. chan %d, img w %d, h %d \n", time1 - time0, time2 - time0, chan, uiSourcePicW, uiSourcePicH);
    return SAL_SOK;

err1:
    if (uiSourcePicW > SVA_MODULE_WIDTH || uiSourcePicH > SVA_MODULE_HEIGHT)
    {
        s32Ret = Sva_DrvPutScaleFrame(pstSva_dev, &pstSva_dev->stAtSysFrame[2]);
        SVA_DRV_CHECK_RET_NO_LOOP(s32Ret, "put scale frame failed! \n");
    }

err:
    return SAL_FAIL;
}

/**
 * @function:   Sva_DrvSetPkgSplitSensity
 * @brief:      设置包裹分割灵敏度
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetPkgSplitSensity(UINT32 chan, void *prm)
{
    UINT32 uiPkgSen = 0;
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    uiPkgSen = *(UINT32 *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (uiPkgSen > 10 || uiPkgSen == 0)
    {
        SAL_WARN("invalid pkg sensity %d, chan %d \n", uiPkgSen, chan);
        return SAL_FAIL;
    }

    pstSva_dev->stIn.uiPkgSensity = uiPkgSen;

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SAL_INFO("set pkg split sensity %d end! chan %d \n", uiPkgSen, chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetPkgSplitFilter
 * @brief:      设置包裹分割过滤阈值
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetPkgSplitFilter(UINT32 chan, void *prm)
{
    UINT32 uiPkgSplitFilter = 0;
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    uiPkgSplitFilter = *(UINT32 *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (uiPkgSplitFilter > 300 || 0 == uiPkgSplitFilter)
    {
        SAL_ERROR("Invalid filter thresh %d, chan %d \n", uiPkgSplitFilter, chan);
        return SAL_FAIL;
    }

    pstSva_dev->stIn.uiPkgSplitFilter = uiPkgSplitFilter;

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SAL_INFO("set pkg split filter %d end! chan %d \n", uiPkgSplitFilter, chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetZoomInOutVal
 * @brief:      设置放大缩小配置参数
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetZoomInOutVal(UINT32 chan, void *prm)
{
    float fZoomVal = 0.0;
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    fZoomVal = *(float *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (fZoomVal < 0.0)
    {
        SAL_ERROR("Invalid fZoomVal %f, chan %d \n", fZoomVal, chan);
        return SAL_FAIL;
    }

    pstSva_dev->stIn.fZoomInOutVal = fZoomVal;

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SAL_INFO("set fZoomVal %f end! chan %d \n", fZoomVal, chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetJpegPosSwitch
 * @brief:     设置Jpeg图片增加Pos信息开关
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:  INT32
 */
INT32 Sva_DrvSetJpegPosSwitch(UINT32 chan, void *prm)
{
    UINT32 uiJpegPosSwitch = 0;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    uiJpegPosSwitch = *(UINT32 *)prm;

    if (uiJpegPosSwitch >= 2)
    {
        SAL_ERROR("Invalid uiJpegPosSwitch %d, chan %d \n", uiJpegPosSwitch, chan);
        return SAL_FAIL;
    }

    uiJpegAddposSwitch = uiJpegPosSwitch;

    SAL_INFO("set chan %d uiJpegPosSwitch %u end!  \n", chan, uiJpegAddposSwitch);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvEngModelPrmPrintf
 * @brief:      打印接口输入的参数
 * @param[in]:  int chan, SVA_SET_AIMODEL_PRM *pstSvaAiModelPrm
 * @param[out]：None
 * @return:  void
 */
void Sva_DrvEngModelPrmPrintf(int chan, SVA_DSP_ENG_MODE_PARAM *pstEngModePrm)
{
    int i, j;
    SVA_DSP_DET_INFO *pstAiTargetAttr = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;

    pstSva_dev = Sva_DrvGetDev(chan);
    if (NULL == pstSva_dev)
    {
        SVA_LOGE("pstSva_dev == null! chan %d \n", chan);
        return;
    }

    SVA_LOGI("form app: type(0-proc,1-aimodel) %d, chan flag[%d, %d] \n", pstEngModePrm->uiBaseType, pstEngModePrm->uiChanFlag[0], pstEngModePrm->uiChanFlag[1]);

    switch (pstEngModePrm->uiBaseType)
    {
        case 0:   /* 切换关联模式 */
        {
            SVA_LOGI("base enable flag %d, proc mode %d \n",
                     pstEngModePrm->stProcModePrm.uiAiEnable, pstEngModePrm->stProcModePrm.enProcMode);
            break;
        }
        case 1:   /* 绑定ai模型 */
        {
            SVA_LOGI("Ai enable flag %d \n", pstEngModePrm->stAiModePrm.uiAiEnableFlag);
            SVA_LOGI("model type num %d \n", pstEngModePrm->stAiModePrm.stchName.uiModelTypeNum);
            for (i = 0; i < pstEngModePrm->stAiModePrm.stchName.uiModelTypeNum; i++)
            {
                SVA_LOGI("i %d, enModelType %d, uiMainModelNum %d, uiSideModelNum %d \n",
                         i, pstEngModePrm->stAiModePrm.stchName.astModelTypeAttr[i].enModelType,
                         pstEngModePrm->stAiModePrm.stchName.astModelTypeAttr[i].uModelAttr.stAiModelAttr.uiMainModelNum,
                         pstEngModePrm->stAiModePrm.stchName.astModelTypeAttr[i].uModelAttr.stAiModelAttr.uiSideModelNum);

                for (j = 0; j < pstEngModePrm->stAiModePrm.stchName.astModelTypeAttr[i].uModelAttr.stAiModelAttr.uiMainModelNum; j++)
                {
                    SVA_LOGI("j %d, main ai model name: %s \n",
                             j, pstEngModePrm->stAiModePrm.stchName.astModelTypeAttr[i].uModelAttr.stAiModelAttr.chMainModelName[j]);
                }

                for (j = 0; j < pstEngModePrm->stAiModePrm.stchName.astModelTypeAttr[i].uModelAttr.stAiModelAttr.uiSideModelNum; j++)
                {
                    SVA_LOGI("j %d, side ai model name: %s \n",
                             j, pstEngModePrm->stAiModePrm.stchName.astModelTypeAttr[i].uModelAttr.stAiModelAttr.chSideModelName[j]);
                }
            }

            SVA_LOGI("Ai map cnt %d \n", pstEngModePrm->stAiModePrm.stAiMap.uiCnt);

            for (i = 0; i < pstEngModePrm->stAiModePrm.stAiMap.uiCnt; i++)
            {
                pstAiTargetAttr = &pstEngModePrm->stAiModePrm.stAiMap.det_info[i];

                SVA_LOGI("target idx %d, Set type %d, key %d, color %x, name %s namelen %d\n",
                         i,
                         pstAiTargetAttr->sva_type,
                         pstAiTargetAttr->sva_key,
                         pstAiTargetAttr->sva_color,
                         pstAiTargetAttr->sva_name,
                         (UINT32)strlen((CHAR *)pstAiTargetAttr->sva_name));
            }

            break;
        }
        default:
        {
            SVA_LOGE("invalid base type %d \n", pstEngModePrm->uiBaseType);
            break;
        }
    }
}

/**
 * @function:   Sva_DrvModuleInitChn
 * @brief:      配置模式切换
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     void *
 */
static int Sva_DrvModuleInitChn(void *prm)
{
    UINT32 i = 0;
    SVA_DSP_ENG_MODE_PARAM *pstSvaEngModelPrm = NULL;

    pstSvaEngModelPrm = (SVA_DSP_ENG_MODE_PARAM *)prm;

    SAL_SET_THR_NAME();

    SVA_LOGW("Proc Mode %d,%d, Ai enable %d,%d !\n",
             pstSvaEngModelPrm->stProcModePrm.enProcMode, g_sva_common.stEngProcPrm.enProcMode,
             pstSvaEngModelPrm->stProcModePrm.uiAiEnable, g_sva_common.stEngProcPrm.uiAiEnable);

    if ((SAL_TRUE == pstSvaEngModelPrm->uiChanFlag[1] && 1 == g_sva_common.uiDevCnt))
    {
        SVA_LOGW("mode %d not support 2 channel! chanflag %d, %d \n", g_proc_mode, pstSvaEngModelPrm->uiChanFlag[0], pstSvaEngModelPrm->uiChanFlag[1]);
        return SAL_FAIL;
    }

    if (((SAL_TRUE == pstSvaEngModelPrm->uiChanFlag[0]) && (SAL_FALSE == pstSvaEngModelPrm->uiChanFlag[1]) && (SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode))
        || ((SAL_FALSE == pstSvaEngModelPrm->uiChanFlag[0]) && (SAL_TRUE == pstSvaEngModelPrm->uiChanFlag[1]) && (SVA_PROC_MODE_DUAL_CORRELATION == g_proc_mode)))
    {
        SVA_LOGW("mode %d not open 2 channel! chanflag %d, %d \n", g_proc_mode, pstSvaEngModelPrm->uiChanFlag[0], pstSvaEngModelPrm->uiChanFlag[1]);
    }

    for (i = 0; i < SVA_DEV_MAX; i++)
    {
        if (SAL_TRUE == pstSvaEngModelPrm->uiChanFlag[i])
        {
            if (SAL_SOK != Sva_DrvModuleInit(i))
            {
                SVA_LOGE("chan %d init error! \n", i);
                goto EXIT;
            }

            usleep(1000 * 1000); /* 延时?? */
        }
    }

EXIT:
    SVA_LOGI("Sva_DrvModuleInitChn End! \n");

    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetEngmodel
 * @brief:      设置引擎模式
 * @param[in]:  UINT32 chan, void *prm
 * @param[out]：None
 * @return:  INT32
 */
INT32 Sva_DrvSetEngModel(UINT32 chan, void *prm)
{
    UINT32 i = 0;

    SVA_DSP_ENG_MODE_PARAM *pstSvaEngModelPrm = NULL;
    SVA_VCAE_PRM_ST *pstProcMode = NULL;
    SVA_VCAE_AIMODEL_PRM_ST *pstAiModelPrm = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pstSvaEngModelPrm = (SVA_DSP_ENG_MODE_PARAM *)prm;
    pstProcMode = &pstSvaEngModelPrm->stProcModePrm;
    pstAiModelPrm = &pstSvaEngModelPrm->stAiModePrm;

    (void)Sva_DrvEngModelPrmPrintf(chan, pstSvaEngModelPrm);

    for (i = 0; i < g_sva_common.uiDevCnt; i++)
    {
        if (SAL_SOK != Sva_DrvModuleDeInit(i))
        {
            SVA_LOGE("chan %d deinit error! \n", i);
            return SAL_FAIL;
        }
    }

    switch (pstSvaEngModelPrm->uiBaseType)
    {
        case 0:   /* 切换关联模式 */
        {
            if (SAL_SOK != Sva_DrvChgProcMode(pstProcMode))
            {
                SVA_LOGE("Set Vcae Prm Failed! mode %d, ai_flag %d \n", pstProcMode->enProcMode, pstProcMode->uiAiEnable);
                return SVA_SET_PROC_MODE_FAIL;
            }

            break;
        }
        case 1:   /* 绑定ai模型 */
        {
            if (SAL_FALSE != pstAiModelPrm->uiAiEnableFlag)
            {
                if (SAL_SOK != Sva_DrvSetModelName(chan, &pstAiModelPrm->stchName))
                {
                    SVA_LOGE("Set Sva_DrvSetModelName Failed! \n");
                    return SVA_SET_AI_MODEL_NAME_FAIL;
                }
            }

            for (i = 0; i < SVA_DEV_MAX; i++)
            {
                if (SAL_SOK != Sva_DrvSetAiTargetMap(i, &pstAiModelPrm->stAiMap))
                {
                    SVA_LOGE("Set Sva_DrvSetAiTargetMap Failed! \n");
                    return SVA_SET_AI_MODEL_MAP_FAIL;
                }
            }

            if (SAL_SOK != Sva_DrvSetModelDetectStatus(chan, &pstAiModelPrm->uiAiEnableFlag))
            {
                SVA_LOGE("Set Sva_DrvSetModelDetectStatus Failed! \n");
                return SVA_SET_AI_MODEL_STATUS_FAIL;
            }

            break;
        }
        default:
        {
            SVA_LOGE("invalid base type %d \n", pstSvaEngModelPrm->uiBaseType);
            break;
        }
    }

    if (SAL_SOK != Sva_DrvModuleInitChn(pstSvaEngModelPrm))
    {
        SVA_LOGE("Sva_DrvStartSetChgFlagThread failed! \n");
        return SVA_SET_PROC_MODE_FAIL;
    }

    SAL_LOGI("sva set eng model end! chan %d \n", chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetPkgPrm
 * @brief:	  设置包裹危险级别参数
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:	  INT32
 */
INT32 Sva_DrvSetPkgPrm(UINT32 chan, void *prm)
{
    UINT32 i = 0;
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_ALERT_PKG_PRM *pstAlertPkgPrm = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pstAlertPkgPrm = (SVA_ALERT_PKG_PRM *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (pstAlertPkgPrm->pkg_level_cnt > SVA_ALERT_LEVEL_NUM)
    {
        SAL_ERROR("invalid pkg level cnt %d, chan %d \n", pstAlertPkgPrm->pkg_level_cnt, chan);
        return SAL_FAIL;
    }

    for (i = 0; i < pstAlertPkgPrm->pkg_level_cnt; i++)
    {
        if (pstSva_dev->stIn.stPkgPrm[i].uiColor == pstAlertPkgPrm->pkg_color[i])
        {
            SAL_WARN("chan %d level %d Same color 0x%x \n", chan, i, pstAlertPkgPrm->pkg_color[i]);
            continue;
        }

        pstSva_dev->stIn.stPkgPrm[i].uiColor = pstAlertPkgPrm->pkg_color[i];
        SAL_INFO("set level %d color 0x%x, chan %d \n", i, pstAlertPkgPrm->pkg_color[i], chan);
    }

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SAL_INFO("set pkg prm end! chan %d \n", chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetAlertLevel
 * @brief:	  设置危险品级别
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:	  INT32
 */
INT32 Sva_DrvSetAlertLevel(UINT32 chan, void *prm)
{
    UINT32 i = 0;
    UINT32 uiQueueCnt = 0;

    SVA_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;
    SVA_DEV_PRM *pstSva_dev = NULL;
    SVA_ALERT_LEVEL *pstAlertLevel = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pstAlertLevel = (SVA_ALERT_LEVEL *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    for (i = 0; i < pstAlertLevel->sva_cnt; i++)
    {
        if ((pstAlertLevel->sva_type[i] > (SVA_MAX_ALARM_TYPE - 1)) || (pstAlertLevel->sva_level[i] > SVA_ALERT_LEVEL_NUM - 1))
        {
            SAL_ERROR("chan %d: type (%d) or level (%d) is Illegal !\n", chan, pstAlertLevel->sva_type[i], pstAlertLevel->sva_level[i]);
            return SAL_FAIL;
        }

        pstSva_dev->stIn.enAlertLevel[pstAlertLevel->sva_type[i]] = pstAlertLevel->sva_level[i];
        SAL_INFO("set alert %d level %d end! chan %d \n", pstAlertLevel->sva_type[i], pstAlertLevel->sva_level[i], chan);
    }

    pstPrmFullQue = &pstSva_dev->stPrmFullQue;
    pstPrmEmptQue = &pstSva_dev->stPrmEmptQue;

    uiQueueCnt = DSA_QueGetQueuedCount(pstPrmEmptQue);
    if (0x00 == uiQueueCnt)
    {
        SAL_ERROR("not enought pstPrmEmptQue\n");
        return SAL_FAIL;
    }

    /* 获取 buffer */
    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    sal_memcpy_s(pstIn, sizeof(SVA_PROCESS_IN), &pstSva_dev->stIn, sizeof(SVA_PROCESS_IN));
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    SAL_INFO("set alert level end! cnt %d, chan %d \n", pstAlertLevel->sva_cnt, chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetInsCropCnt
 * @brief:	  设置即时裁图每秒个数
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:	  INT32
 */
INT32 Sva_DrvSetInsCropCnt(void *prm)
{
    UINT32 uiCnt = 0;

    uiCnt = *(UINT32 *)prm;

    g_InsProcCntPs = uiCnt;

    SAL_LOGI("set ins crop cnt %d per sec! \n", uiCnt);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetPkgBorGapPixel
 * @brief:      设置包裹左右边沿预留像素
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetPkgBorGapPixel(UINT32 chan, VOID *prm)
{
    UINT32 uiGapPixel = 0.0;

    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    uiGapPixel = *(UINT32 *)prm;

    pstSva_dev->stIn.uiGapPixel = uiGapPixel;
    pstSva_dev->stIn.uiGapUpFlag = SAL_TRUE;

    SAL_INFO("set pkg border gap pixel %d end, chan %d \n", uiGapPixel, chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetSplitMode
 * @brief:      设置集中判图模式开关
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetSplitMode(void *prm)
{
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    BOOL bSplitMode = !!(*(UINT32 *)prm);

    if (bSplitMode == g_sva_common.bSplitMode)
    {
        SVA_LOGW("same split flag %d, return success \n", bSplitMode);
        return SAL_SOK;
    }

    g_sva_common.bSplitMode = bSplitMode;

    SVA_LOGI("set split mode %d end! \n", bSplitMode);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetPkgCombPixelThresh
 * @brief:      用于更新包裹融合像素阈值
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetPkgCombPixelThresh(UINT32 chan, void *prm)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    INT32 s32PkgCombThr = *(INT32 *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (s32PkgCombThr > 50 || s32PkgCombThr < -50)
    {
        SVA_LOGE("invalid pkg comb thresh %d, should be in range[-50, 50], chan %d \n", s32PkgCombThr, chan);
        return SAL_FAIL;
    }

    pstSva_dev->f32PkgCombThr = (float)s32PkgCombThr / (float)SVA_MODULE_WIDTH;

    SVA_LOGI("set pkg comb pixel thresh %d end ==> %f, chan %d \n", s32PkgCombThr, pstSva_dev->f32PkgCombThr, chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetForceSplitSwitch
 * @brief:      设置包裹强制分割开关
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetForceSplitSwitch(UINT32 chan, void *prm)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    SVA_FORCE_SPLIT_PRM_S *pstForceSplitPrm = (SVA_FORCE_SPLIT_PRM_S *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (pstForceSplitPrm->u32Flag > 1)
    {
        SVA_LOGE("invalid force split flag %d, chan %d \n", pstForceSplitPrm->u32Flag, chan);
        return SAL_FAIL;
    }

    memcpy(&pstSva_dev->stForceSplitPrm, pstForceSplitPrm, sizeof(SVA_FORCE_SPLIT_PRM_S));
    if (pstSva_dev->stForceSplitPrm.u32Flag)
    {
        /* todo: 当前配置文件不支持动态修改强制分割帧数，故此接口暂时仅处理开关，按照配置文件中固定的帧数进行处理 */

    }

    SVA_LOGI("set force split prm end! flag: %d, chan %d \n", pstSva_dev->stForceSplitPrm.u32Flag, chan);
    return SAL_SOK;
}

/**
 * @function:   Sva_DrvSetPkgCombineFlag
 * @brief:      设置包裹融合开关
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_DrvSetPkgCombineFlag(UINT32 chan, void *prm)
{
    SVA_DEV_PRM *pstSva_dev = NULL;

    SVA_DRV_CHECK_CHAN(chan, SAL_FAIL);
    SVA_DRV_CHECK_PRM(prm, SAL_FAIL);

    UINT32 u32Flag = *(UINT32 *)prm;

    pstSva_dev = Sva_DrvGetDev(chan);
    SVA_DRV_CHECK_PRM(pstSva_dev, SAL_FAIL);

    if (u32Flag > 1)
    {
        SVA_LOGE("invalid pkg combine flag %d, chan %d \n", u32Flag, chan);
        return SAL_FAIL;
    }

    pstSva_dev->u32PkgCombFlag = u32Flag;

    SVA_LOGI("set pkg combine flag end! flag: %d, chan %d \n", pstSva_dev->u32PkgCombFlag, chan);
    return SAL_SOK;
}

#if 0  /* test vgs scale api, check 1920x1080 -> 1280x1024 ok */
#define DEBUG_SVA_SCALE_SRC_W	(1920)
#define DEBUG_SVA_SCALE_SRC_H	(1080)

#define DEBUG_SVA_SCALE_DST_W	(1280)
#define DEBUG_SVA_SCALE_DST_H	(1024)

#define DEBUG_SVA_SCALE_SRC_IMG_SIZE	(DEBUG_SVA_SCALE_SRC_W * DEBUG_SVA_SCALE_SRC_H * 3 / 2)
#define DEBUG_SVA_SCALE_DST_IMG_SIZE	(DEBUG_SVA_SCALE_DST_W * DEBUG_SVA_SCALE_DST_H * 3 / 2)

/**
 * @function    Sva_DrvDebugScaleFrame
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
int Sva_DrvDebugScaleFrame(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 u32ReadSize = 0;
    CHAR *src_path = "./yuv/0.yuv";

    ALLOC_VB_INFO_S stVbInfo = {0};
    ALLOC_VB_INFO_S stDstVbInfo = {0};

    SAL_VideoFrameBuf stVideoFrameBuf = {0};

    SYSTEM_FRAME_INFO stSrcFrameInfo = {0};
    SYSTEM_FRAME_INFO stDstFrameInfo = {0};

    /* 读取文件，原始1080P */
    FILE *fp = NULL;

    {
        fp = fopen(src_path, "r+");
        if (NULL == fp)
        {
            SVA_LOGE("fopen failed! %s \n", src_path);
            goto exit;
        }

        SVA_LOGW("fopen end! %s \n", src_path);
    }

    /* 申请放缩前的原始图像，1080P */
    s32Ret = mem_hal_vbAlloc(DEBUG_SVA_SCALE_SRC_IMG_SIZE, "SVA", "debug_scale_src", NULL, SAL_TRUE, &stVbInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE(" vb alloc failed! \n");
        goto close_file;
    }

    if (0x00 == stSrcFrameInfo.uiAppData)
    {
        if (SAL_SOK != sys_hal_allocVideoFrameInfoSt(&stSrcFrameInfo))
        {
            SVA_LOGE("alloc video frame info st failed! \n");
            goto exit1;
        }

        stVideoFrameBuf.frameParam.width = DEBUG_SVA_SCALE_SRC_W;
        stVideoFrameBuf.frameParam.height = DEBUG_SVA_SCALE_SRC_H;
        stVideoFrameBuf.stride[0] = stVideoFrameBuf.stride[1] = DEBUG_SVA_SCALE_SRC_W;
        stVideoFrameBuf.virAddr[0] = (PhysAddr)stVbInfo.pVirAddr;
        stVideoFrameBuf.virAddr[1] = stVideoFrameBuf.virAddr[0] + DEBUG_SVA_SCALE_SRC_W * DEBUG_SVA_SCALE_SRC_H;
        stVideoFrameBuf.virAddr[2] = stVideoFrameBuf.virAddr[1];
        stVideoFrameBuf.phyAddr[0] = stVbInfo.u64PhysAddr;
        stVideoFrameBuf.phyAddr[1] = stVideoFrameBuf.phyAddr[0] + DEBUG_SVA_SCALE_SRC_W * DEBUG_SVA_SCALE_SRC_H;
        stVideoFrameBuf.phyAddr[2] = stVideoFrameBuf.phyAddr[1];
        stVideoFrameBuf.vbBlk = stVbInfo.u64VbBlk;
        stVideoFrameBuf.poolId = stVbInfo.u32PoolId;
        stVideoFrameBuf.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

        sys_hal_buildVideoFrame(&stVideoFrameBuf, &stSrcFrameInfo);
    }

    /* 申请放缩后的原始图像，1280x1024 */
    s32Ret = mem_hal_vbAlloc(DEBUG_SVA_SCALE_DST_IMG_SIZE, "SVA", "debug_scale_dst", NULL, SAL_TRUE, &stDstVbInfo);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE(" vb alloc failed! \n");
        goto exit1;
    }

    if (0x00 == stDstFrameInfo.uiAppData)
    {
        if (SAL_SOK != sys_hal_allocVideoFrameInfoSt(&stDstFrameInfo))
        {
            SVA_LOGE("alloc video frame info st failed! \n");
            goto exit2;
        }

        stVideoFrameBuf.frameParam.width = DEBUG_SVA_SCALE_DST_W;
        stVideoFrameBuf.frameParam.height = DEBUG_SVA_SCALE_DST_H;
        stVideoFrameBuf.stride[0] = stVideoFrameBuf.stride[1] = DEBUG_SVA_SCALE_DST_W;
        stVideoFrameBuf.virAddr[0] = (PhysAddr)stDstVbInfo.pVirAddr;
        stVideoFrameBuf.virAddr[1] = stVideoFrameBuf.virAddr[0] + DEBUG_SVA_SCALE_DST_W * DEBUG_SVA_SCALE_DST_H;
        stVideoFrameBuf.virAddr[2] = stVideoFrameBuf.virAddr[1];
        stVideoFrameBuf.phyAddr[0] = stDstVbInfo.u64PhysAddr;
        stVideoFrameBuf.phyAddr[1] = stVideoFrameBuf.phyAddr[0] + DEBUG_SVA_SCALE_DST_W * DEBUG_SVA_SCALE_DST_H;
        stVideoFrameBuf.phyAddr[2] = stVideoFrameBuf.phyAddr[1];
        stVideoFrameBuf.vbBlk = stDstVbInfo.u64VbBlk;
        stVideoFrameBuf.poolId = stDstVbInfo.u32PoolId;
        stVideoFrameBuf.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

        sys_hal_buildVideoFrame(&stVideoFrameBuf, &stDstFrameInfo);
    }

    /* 读取原始1080P图像数据 */
    if (SAL_SOK != sys_hal_getVideoFrameInfo(&stSrcFrameInfo, &stVideoFrameBuf))
    {
        SVA_LOGE("get video frame failed! \n");
        goto exit2;

    }

    u32ReadSize = fread((VOID *)stVideoFrameBuf.virAddr[0], DEBUG_SVA_SCALE_SRC_IMG_SIZE, 1, fp);
    /* if (DEBUG_SVA_SCALE_SRC_IMG_SIZE != u32ReadSize) */
    {
        SVA_LOGE("fread faield! read size %d \n", u32ReadSize);
        /* goto exit2; */
    }

    if (SAL_SOK != sys_hal_getVideoFrameInfo(&stSrcFrameInfo, &stVideoFrameBuf))
    {
        SVA_LOGE("get video frame failed! \n");
        goto exit2;

    }

    Sva_HalDebugDumpData((VOID *)stVideoFrameBuf.virAddr[0],
                         "./before_scale.nv21",
                         DEBUG_SVA_SCALE_SRC_IMG_SIZE,
                         0);
    SVA_LOGE("dump before nv21 end! \n");

    s32Ret = vgs_hal_scaleFrame(&stDstFrameInfo, &stSrcFrameInfo, DEBUG_SVA_SCALE_DST_W, DEBUG_SVA_SCALE_DST_H);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("vgs scale frame failed! src_w %d, src_h %d, dst_w %d, dst_h %d \n",
                 DEBUG_SVA_SCALE_SRC_W, DEBUG_SVA_SCALE_SRC_H, DEBUG_SVA_SCALE_DST_W, DEBUG_SVA_SCALE_DST_H);
        goto exit2;
    }

    if (SAL_SOK != sys_hal_getVideoFrameInfo(&stDstFrameInfo, &stVideoFrameBuf))
    {
        SVA_LOGE("get video frame failed! \n");
        goto exit2;

    }

    Sva_HalDebugDumpData((VOID *)stVideoFrameBuf.virAddr[0],
                         "./after_scale.nv21",
                         DEBUG_SVA_SCALE_DST_IMG_SIZE,
                         0);
    SVA_LOGE("dump after nv21 end! \n");

    s32Ret = SAL_SOK;

exit2:
    memset(&stVideoFrameBuf, 0x00, sizeof(SAL_VideoFrameBuf));
    if (SAL_SOK != sys_hal_getVideoFrameInfo(&stDstFrameInfo, &stVideoFrameBuf))
    {
        SVA_LOGE("get video frame failed! \n");

    }

    if (NULL != (VOID *)stVideoFrameBuf.virAddr[0])
    {
        s32Ret = mem_hal_vbFree((VOID *)stVideoFrameBuf.virAddr[0], "SVA", "debug_scale_dst", DEBUG_SVA_SCALE_DST_IMG_SIZE, stDstVbInfo.u64VbBlk, stDstVbInfo.u32PoolId);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("free mmz failed! \n");
        }
    }

exit1:
    memset(&stVideoFrameBuf, 0x00, sizeof(SAL_VideoFrameBuf));
    if (SAL_SOK != sys_hal_getVideoFrameInfo(&stSrcFrameInfo, &stVideoFrameBuf))
    {
        SVA_LOGE("get video frame failed! \n");

    }

    if (NULL != (VOID *)stVideoFrameBuf.virAddr[0])
    {
        s32Ret = mem_hal_vbFree((VOID *)stVideoFrameBuf.virAddr[0], "SVA", "debug_scale_src", DEBUG_SVA_SCALE_DST_IMG_SIZE, stVbInfo.u64VbBlk, stVbInfo.u32PoolId);
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("free mmz failed! \n");
        }
    }

close_file:
    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }

exit:
    return s32Ret;
}

#endif

#if 1  /* 维护信息 */

/*
   sva
   |----comm
 |      |----sts
 |      |----app_cfg
 |      |----prvt_info
 |
 |||||||||||||||---- alg
 |||||||||||||||----sts
 |||||||||||||||----alg_cfg

    注: 子节点的索引值均从0开始计数.
 */

#define SVA_MOD_NAME ("sva")        /* 模块名 */

/* 根节点 */
#define SVA_MOD_ROOT_IDX (0)

/* comm子节点 */
#define SVA_MOD_COMM_IDX (0)

#define SVA_MOD_COMM_STS_IDX		(0)        /* comm子树下sts子节点 */
#define SVA_MOD_COMM_APP_CFG_IDX	(1)        /* comm子树下cfg子节点 */
#define SVA_MOD_COMM_PRVT_INFO_IDX	(2)        /* comm子树下prvt子节点 */

/* alg子节点 */
#define SVA_MOD_ALG_TREE_IDX (1)

#define SVA_MOD_ALG_STS_IDX (0)                /* alg子树下sts子节点 */
#define SVA_MOD_ALG_CFG_IDX (1)                /* alg子树下cfg子节点 */

#if 1 /* comm sts */

/**
 * @function:   Sva_DrvGetDbgCommViFrmRate
 * @brief:      维护信息-获取vi帧率
 * @param[in]:
 * @param[out]:
 * @return:
 */
static void *Sva_DrvGetDbgCommViFrmRate(void)
{
    g_sva_common.stDbgPrm.stCommSts.uiInputFrmRate = Sva_HalGetViInputFrmRate();
    return &g_sva_common.stDbgPrm.stCommSts.uiInputFrmRate;
}

/**
 * @function    Sva_DrvGetDbgCommInputCnt
 * @brief       维护信息-获取模块输入帧数
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommInputCnt(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiInputCnt;
}

/**
 * @function    Sva_DrvGetDbgCommOutputCnt
 * @brief       维护信息-获取模块输出帧数
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommOutputCnt(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiOutputCnt;
}

/**
 * @function    Sva_DrvGetDbgCommOutputEndCnt
 * @brief       维护信息-获取模块输出结束帧数
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommOutputEndCnt(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiOutputCbEndCnt;
}

/**
 * @function    Sva_DrvGetDbgCommErrCnt
 * @brief       维护信息-获取模块出错帧数
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommErrCnt(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiErrCnt;
}

/**
 * @function    Sva_DrvGetDbgCommJencCnt
 * @brief       维护信息-获取模块编图次数
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommJencCnt(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiJencCnt;
}

/**
 * @function    Sva_DrvGetDbgCommJencErrCnt
 * @brief       维护信息-获取模块编图出错帧数
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommJencErrCnt(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiJencErrCnt;
}

#endif

#if 1 /* todo: comm cfg */

/**
 * @function    Sva_DrvGetDbgCommDirectionCfg
 * @brief       维护信息-获取过包方向
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommDirectionCfg(void)
{
    (VOID)Sva_DrvGetCfgParam(0, &g_sva_common.stDbgPrm.stIn);

    g_sva_common.stDbgPrm.stCommSts.uiDirection = g_sva_common.stDbgPrm.stIn.enDirection;

    return &g_sva_common.stDbgPrm.stCommSts.uiDirection;
}

/**
 * @function    Sva_DrvGetDbgCommOsdExtTypeCfg
 * @brief       维护信息-获取osd拓展字段展示类型
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommOsdExtTypeCfg(void)
{
    (VOID)Sva_DrvGetCfgParam(0, &g_sva_common.stDbgPrm.stIn);

    g_sva_common.stDbgPrm.stCommSts.uiOsdExtType = g_sva_common.stDbgPrm.stIn.stTargetPrm.enOsdExtType;

    return &g_sva_common.stDbgPrm.stCommSts.uiOsdExtType;
}

/**
 * @function    Sva_DrvGetDbgCommOsdScaleLevelCfg
 * @brief       维护信息-获取osd字体大小
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommOsdScaleLevelCfg(void)
{
    (VOID)Sva_DrvGetCfgParam(0, &g_sva_common.stDbgPrm.stIn);

    g_sva_common.stDbgPrm.stCommSts.uiOsdScaleLevel = g_sva_common.stDbgPrm.stIn.stTargetPrm.scaleLevel;

    return &g_sva_common.stDbgPrm.stCommSts.uiOsdScaleLevel;
}

/**
 * @function    Sva_DrvGetDbgCommRegionCfg
 * @brief       维护信息-获取检测区域信息
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommRegionCfg(void)
{
    (VOID)Sva_DrvGetCfgParam(0, &g_sva_common.stDbgPrm.stIn);

    {
        sprintf(g_sva_common.stDbgPrm.stCommSts.acRgnStr,
                "x: %f, y: %f, w: %f, h: %f",
                g_sva_common.stDbgPrm.stIn.rect.x,
                g_sva_common.stDbgPrm.stIn.rect.y,
                g_sva_common.stDbgPrm.stIn.rect.width,
                g_sva_common.stDbgPrm.stIn.rect.height);
    }

    return g_sva_common.stDbgPrm.stCommSts.acRgnStr;
}

/**
 * @function    Sva_DrvGetDbgCommAlertCfg
 * @brief       维护信息-获取危险品检测配置
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommAlertCfg(void)
{
    UINT32 i;
    CHAR acTmp[128] = {'\0'};

    /* id name sva_conf color key conf */
    (VOID)Sva_DrvGetCfgParam(0, &g_sva_common.stDbgPrm.stIn);

    /* 格式要求: 违禁品信息打印时需要先进行换行 */
    memset(g_sva_common.stDbgPrm.stCommSts.acAlertStr, 0x00, SVA_MAX_ALARM_TYPE * 128);
    g_sva_common.stDbgPrm.stCommSts.acAlertStr[0] = '\n';

    for (i = 0; i < SVA_MAX_ALARM_TYPE; i++)
    {
        if (!g_sva_common.stDbgPrm.stIn.alert[i].bInit)
        {
            continue;
        }

        memset(acTmp, '\0', 128);

        sprintf(acTmp,
                "%d: type: %d, name: %s, key: %d, color %x \n", i + 1,
                g_sva_common.stDbgPrm.stIn.alert[i].sva_type,
                g_sva_common.stDbgPrm.stIn.alert[i].sva_name,
                g_sva_common.stDbgPrm.stIn.alert[i].sva_key,
                g_sva_common.stDbgPrm.stIn.alert[i].sva_color);
        strcat(g_sva_common.stDbgPrm.stCommSts.acAlertStr, acTmp);
    }

    return g_sva_common.stDbgPrm.stCommSts.acAlertStr;
}

#endif

#if 1  /* comm prvt info */

/**
 * @function    Sva_DrvGetDbgCommPrvtSendFrmInfo
 * @brief       维护信息-获取模块私有发送帧数据接口是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtSendFrmInfo(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiSendFrameFlag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtSendFrmNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtSendFrmNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiSendFrameNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtSendFrmSuccNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtSendFrmSuccNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiSendFrameSuccNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtSendFrmFailNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtSendFrmFailNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiSendFrameFailNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtRecvFrmInfo
 * @brief       维护信息-获取模块私有接收帧数据接口是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtRecvFrmInfo(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiRecvFrameFlag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtRecvFrmNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtRecvFrmNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiRecvFrameNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtRecvFrmSuccNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtRecvFrmSuccNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiRecvFrameSuccNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtRecvFrmFailNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtRecvFrmFailNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiRecvFrameFailNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtSendIaRsltInfo
 * @brief       维护信息-获取模块私有发送智能结果是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtSendIaRsltInfo(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiSendIaRsltFlag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtSendIaRsltNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtSendIaRsltNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiSendIaRsltNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtSendIaRsltSuccNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtSendIaRsltSuccNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiSendIaRsltSuccNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtSendIaRsltFailNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtSendIaRsltFailNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiSendIaRsltFailNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtRecvIaRsltInfo
 * @brief       维护信息-获取模块私有接收智能结果接口是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtRecvIaRsltInfo(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiRecvIaRsltFlag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtRecvIaRsltNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtRecvIaRsltNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiRecvIaRsltNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtRecvIaRsltSuccNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtRecvIaRsltSuccNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiRecvIaRsltSuccNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtRecvIaRsltFailNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtRecvIaRsltFailNum(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiRecvIaRsltFailNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtIptEngDataInfo
 * @brief       维护信息-获取模块私有引擎送帧接口是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtIptEngDataInfo(void)
{
    g_sva_common.stDbgPrm.stCommSts.uiInputEngDataFlag = Sva_HalGetIptEngStuckFlag();

    return &g_sva_common.stDbgPrm.stCommSts.uiInputEngDataFlag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtIptEngDataNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtIptEngDataNum(void)
{
    g_sva_common.stDbgPrm.stCommSts.uiInputEngDataNum = Sva_HalGetIptEngNum();

    return &g_sva_common.stDbgPrm.stCommSts.uiInputEngDataNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtIptEngSuccDataNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtIptEngSuccDataNum(void)
{
    g_sva_common.stDbgPrm.stCommSts.uiInputEngDataSuccNum = Sva_HalGetIptEngSuccNum();

    return &g_sva_common.stDbgPrm.stCommSts.uiInputEngDataSuccNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtIptEngFailDataNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtIptEngFailDataNum(void)
{
    g_sva_common.stDbgPrm.stCommSts.uiInputEngDataFailNum = Sva_HalGetIptEngFailNum();

    return &g_sva_common.stDbgPrm.stCommSts.uiInputEngDataFailNum;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtIptEngDataInfo
 * @brief       维护信息-获取模块私有引擎送帧接口是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtJencMirrorFlag(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiJencMirrorFlag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtIptEngDataInfo
 * @brief       维护信息-获取模块私有引擎送帧接口是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtJencBmpFlag(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiJencBmpFlag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtIptEngDataInfo
 * @brief       维护信息-获取模块私有引擎送帧接口是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtJencJpg1Flag(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiJencjpg1Flag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtIptEngDataInfo
 * @brief       维护信息-获取模块私有引擎送帧接口是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtJencJpg2Flag(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiJencjpg2Flag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtIptEngDataInfo
 * @brief       维护信息-获取模块私有引擎送帧接口是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtJencSplitModeFlag(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiJencSplitModeFlag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtIptEngDataInfo
 * @brief       维护信息-获取模块私有引擎送帧接口是否卡住
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgCommPrvtJencCbFlag(void)
{
    return &g_sva_common.stDbgPrm.stCommSts.uiJencCbFlag;
}

/**
 * @function    Sva_DrvGetDbgCommPrvtInfo
 * @brief       维护信息-获取模块私有维护信息
 * @param[in]
 * @param[out]
 * @return
 */
static ATTRIBUTE_UNUSED void *Sva_DrvGetDbgCommPrvtInfo(void)
{
    sprintf(g_sva_common.stDbgPrm.stCommSts.acPrvtStr, "%s", "[none]\n");

    return g_sva_common.stDbgPrm.stCommSts.acPrvtStr;
}

#endif

#if 1  /* alg sts */

/**
 * @function    Sva_DrvGetDbgAlgModeInfo
 * @brief       维护信息-获取算法处理模式
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgModeInfo(void)
{
    g_sva_common.stDbgPrm.stAlgSts.uiProcMode = g_proc_mode;

    return &g_sva_common.stDbgPrm.stAlgSts.uiProcMode;
}

/**
 * @function    Sva_DrvGetDbgAlgAveMs
 * @brief       维护信息-获取算法处理平均耗时
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgAveMs(void)
{
    g_sva_common.stDbgPrm.stAlgSts.fAveMs = ((float)g_sva_common.stDbgPrm.stAlgSts.uiTotalAlgCostMs / (float)g_sva_common.stDbgPrm.stAlgSts.uiTotalAlgProcCnt);

    return &g_sva_common.stDbgPrm.stAlgSts.fAveMs;
}

/**
 * @function    Sva_DrvGetDbgAlgMinMs
 * @brief       维护信息-获取算法处理最小耗时
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgMinMs(void)
{
    return &g_sva_common.stDbgPrm.stAlgSts.uiMinMs;
}

/**
 * @function    Sva_DrvGetDbgAlgMaxMs
 * @brief       维护信息-获取算法处理最大耗时
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgMaxMs(void)
{
    return &g_sva_common.stDbgPrm.stAlgSts.uiMaxMs;
}

/**
 * @function    Sva_DrvGetDbgAlgInputCnt
 * @brief       维护信息-获取算法处理输入帧数
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgInputCnt(void)
{
    g_sva_common.stDbgPrm.stAlgSts.uiInputCnt = Sva_HalGetInputEngCnt();

    return &g_sva_common.stDbgPrm.stAlgSts.uiInputCnt;
}

/**
 * @function    Sva_DrvGetDbgAlgOutputCnt
 * @brief       维护信息-获取算法处理输出帧数
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgOutputCnt(void)
{
    return &g_sva_common.stDbgPrm.stAlgSts.uiOutputCnt;
}

/**
 * @function    Sva_DrvGetDbgAlgErrCnt
 * @brief       维护信息-获取算法处理出错次数
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgErrCnt(void)
{
    return &g_sva_common.stDbgPrm.stAlgSts.uiAlgErrCnt;
}

/**
 * @function    Sva_DrvGetDbgAlgVersion
 * @brief       维护信息-获取算法版本号
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgVersion(void)
{
    CHAR acTmp[256] = {'\0'};
    CHAR *p_start = NULL;
    CHAR *p_end = NULL;
    XSI_COMMON *pXsiCommon = NULL;

    pXsiCommon = Sva_HalGetXsiCommon();
    if (NULL == pXsiCommon)
    {
        return NULL;
    }

    sal_memcpy_s((unsigned char *)acTmp, sizeof(pXsiCommon->version),
                 pXsiCommon->version, strlen((char *)pXsiCommon->version));

    sprintf(g_sva_common.stDbgPrm.stAlgSts.acAlgVer, "%s", "no icf version");

    if (NULL != (p_start = strstr(acTmp, "ICF Version:")))
    {
        p_start += strlen("ICF Version:");

        if (NULL != (p_end = strstr(p_start, ",")))
        {
            *p_end = '\0';
        }

        strncpy(g_sva_common.stDbgPrm.stAlgSts.acAlgVer, p_start, 255);
    }

    return g_sva_common.stDbgPrm.stAlgSts.acAlgVer;
}

/**
 * @function    Sva_DrvGetDbgAlgModelVer
 * @brief       维护信息-获取模型版本号
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgModelVer(void)
{
    CHAR acTmp[256] = {'\0'};
    CHAR *p_start = NULL;
    CHAR *p_end = NULL;
    XSI_COMMON *pXsiCommon = NULL;

    pXsiCommon = Sva_HalGetXsiCommon();
    if (NULL == pXsiCommon)
    {
        return NULL;
    }

    sal_memcpy_s((unsigned char *)acTmp, sizeof(pXsiCommon->version),
                 pXsiCommon->version, strlen((char *)pXsiCommon->version));

    sprintf(g_sva_common.stDbgPrm.stAlgSts.acModelVer, "%s", "no model version");

    if (NULL != (p_start = strstr(acTmp, "MODEL Version:")))
    {
        p_start += strlen("MODEL Version:");

        if (NULL != (p_end = strstr(p_start, ",")))
        {
            *p_end = '\0';
        }

        strncpy(g_sva_common.stDbgPrm.stAlgSts.acModelVer, p_start, 255);
    }

    return g_sva_common.stDbgPrm.stAlgSts.acModelVer;
}

/**
 * @function    Sva_DrvGetDbgAlgMemInfo
 * @brief       维护信息-获取算法消耗内存信息
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgMemInfo(void)
{
    UINT32 idx = 1;    /* cal mmz */
    float fTotalMem = 0;
    UINT32 initMemSize[IA_MEM_TYPE_NUM] = {0};

    (VOID)IA_GetModMemInitSize(IA_MOD_SVA, initMemSize);

    while (idx < IA_MEM_TYPE_NUM)
    {
        fTotalMem += (float)initMemSize[idx] / 1024.0 / 1024.0;
        idx++;
    }

    sprintf(g_sva_common.stDbgPrm.stAlgSts.acMemInfo, \
            "SYS[%f MB], MMZ[%f MB]\nMalloc[%f MB], Cache[%f MB], NoCache[%f MB], CachePri[%f MB], CacheNoPri[%f MB], RemapNone[%f MB], RemapNoCache[%f MB], RemapCache[%f MB]", \
            (float)initMemSize[IA_MALLOC] / 1024.0 / 1024.0,
            fTotalMem,
            (float)initMemSize[IA_MALLOC] / 1024.0 / 1024.0,
            (float)initMemSize[IA_HISI_MMZ_CACHE] / 1024.0 / 1024.0,
            (float)initMemSize[IA_HISI_MMZ_NO_CACHE] / 1024.0 / 1024.0,
            (float)initMemSize[IA_HISI_MMZ_CACHE_PRIORITY] / 1024.0 / 1024.0,
            (float)initMemSize[IA_HISI_MMZ_CACHE_NO_PRIORITY] / 1024.0 / 1024.0,
            (float)initMemSize[IA_HISI_VB_REMAP_NONE] / 1024.0 / 1024.0,
            (float)initMemSize[IA_HISI_VB_REMAP_NO_CACHED] / 1024.0 / 1024.0,
            (float)initMemSize[IA_HISI_VB_REMAP_CACHED] / 1024.0 / 1024.0);

    return g_sva_common.stDbgPrm.stAlgSts.acMemInfo;
}

#endif

#if 1  /* alg cfg */

/**
 * @function    Sva_DrvGetDbgAlgPdProcGapCfg
 * @brief       维护信息-获取算法包裹分割处理帧间隔
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgPdProcGapCfg(void)
{
    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    if (NULL == pstXsiCommon)
    {
        return NULL;
    }

    SVA_INPUT_DATA *pstSvaInputData = NULL;

    pstSvaInputData = (SVA_INPUT_DATA *)&pstXsiCommon->stEngChnPrm[0].astSvaInputData[0];

    g_sva_common.stDbgPrm.stAlgSts.uiPdProcGap = pstSvaInputData->stXsieAlgPrm.PDProcessGap;

    return &g_sva_common.stDbgPrm.stAlgSts.uiPdProcGap;
}

/**
 * @function    Sva_DrvGetDbgAlguiObdDownSmpScaleCfg
 * @brief       维护信息-获取算法obd下采样率
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlguiObdDownSmpScaleCfg(void)
{
    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    if (NULL == pstXsiCommon)
    {
        return NULL;
    }

    SVA_INPUT_DATA *pstSvaInputData = NULL;

    pstSvaInputData = (SVA_INPUT_DATA *)&pstXsiCommon->stEngChnPrm[0].astSvaInputData[0];

    g_sva_common.stDbgPrm.stAlgSts.fObdDownSmpScale = pstSvaInputData->stXsieAlgPrm.ObdDownsampleScale;

    return &g_sva_common.stDbgPrm.stAlgSts.fObdDownSmpScale;
}

/**
 * @function    Sva_DrvGetDbgAlgPkgSensityCfg
 * @brief       维护信息-获取算法包裹分割灵敏度配置
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgPkgSensityCfg(void)
{
    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    if (NULL == pstXsiCommon)
    {
        return NULL;
    }

    SVA_INPUT_DATA *pstSvaInputData = NULL;

    pstSvaInputData = (SVA_INPUT_DATA *)&pstXsiCommon->stEngChnPrm[0].astSvaInputData[0];

    g_sva_common.stDbgPrm.stAlgSts.uiPkgSensity = pstSvaInputData->stXsieAlgPrm.PackageSensity;

    return &g_sva_common.stDbgPrm.stAlgSts.uiPkgSensity;
}

/**
 * @function    Sva_DrvGetDbgAlgZoomInOutThrsCfg
 * @brief       维护信息-获取算法放大缩小阈值
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgZoomInOutThrsCfg(void)
{
    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    if (NULL == pstXsiCommon)
    {
        return NULL;
    }

    SVA_INPUT_DATA *pstSvaInputData = NULL;

    pstSvaInputData = (SVA_INPUT_DATA *)&pstXsiCommon->stEngChnPrm[0].astSvaInputData[0];

    g_sva_common.stDbgPrm.stAlgSts.fZoomInOutThrs = pstSvaInputData->stXsieAlgPrm.ZoomInOutThres;

    return &g_sva_common.stDbgPrm.stAlgSts.fZoomInOutThrs;
}

/**
 * @function    Sva_DrvGetDbgAlgPackOverX2Cfg
 * @brief       维护信息-获取算法包裹出全阈值
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgPackOverX2Cfg(void)
{
    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    if (NULL == pstXsiCommon)
    {
        return NULL;
    }

    SVA_INPUT_DATA *pstSvaInputData = NULL;

    pstSvaInputData = (SVA_INPUT_DATA *)&pstXsiCommon->stEngChnPrm[0].astSvaInputData[0];

    g_sva_common.stDbgPrm.stAlgSts.fPackOverX2 = pstSvaInputData->stXsieAlgPrm.PackOverX2;

    return &g_sva_common.stDbgPrm.stAlgSts.fPackOverX2;
}

/**
 * @function    Sva_DrvGetDbgAlgPkSwitchCfg
 * @brief       维护信息-获取算法pk模式开关
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgPkSwitchCfg(void)
{
    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    if (NULL == pstXsiCommon)
    {
        return NULL;
    }

    SVA_INPUT_DATA *pstSvaInputData = NULL;

    pstSvaInputData = (SVA_INPUT_DATA *)&pstXsiCommon->stEngChnPrm[0].astSvaInputData[0];

    g_sva_common.stDbgPrm.stAlgSts.uiPkSwitch = pstSvaInputData->stXsieAlgPrm.PkSwitch;

    return &g_sva_common.stDbgPrm.stAlgSts.uiPkSwitch;
}

/**
 * @function    Sva_DrvGetDbgAlgColorFilterSwitchCfg
 * @brief       维护信息-获取算法颜色过滤开关
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgColorFilterSwitchCfg(void)
{
    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    if (NULL == pstXsiCommon)
    {
        return NULL;
    }

    SVA_INPUT_DATA *pstSvaInputData = NULL;

    pstSvaInputData = (SVA_INPUT_DATA *)&pstXsiCommon->stEngChnPrm[0].astSvaInputData[0];

    g_sva_common.stDbgPrm.stAlgSts.uiColorFilterSwitch = pstSvaInputData->stXsieAlgPrm.ColorFilterSwitch;

    return &g_sva_common.stDbgPrm.stAlgSts.uiColorFilterSwitch;
}

/**
 * @function    Sva_DrvGetDbgAlgSizeFilterSwitchCfg
 * @brief       维护信息-获取算法尺寸过滤开关
 * @param[in]
 * @param[out]
 * @return
 */
static void *Sva_DrvGetDbgAlgSizeFilterSwitchCfg(void)
{
    XSI_COMMON *pstXsiCommon = Sva_HalGetXsiCommon();

    if (NULL == pstXsiCommon)
    {
        return NULL;
    }

    SVA_INPUT_DATA *pstSvaInputData = NULL;

    pstSvaInputData = (SVA_INPUT_DATA *)&pstXsiCommon->stEngChnPrm[0].astSvaInputData[0];

    g_sva_common.stDbgPrm.stAlgSts.uiSizeFilterSwitch = pstSvaInputData->stXsieAlgPrm.SizeFilterSwitch;

    return &g_sva_common.stDbgPrm.stAlgSts.uiSizeFilterSwitch;
}

#endif

/**
 * @function    Sva_DrvDbgCommModStsInit
 * @brief       模块通用状态子树初始化
 * @param[in]   UINT32 uiNodeId  节点ID
 *              STS_MOD_NODE_INFO_S *pstNodeInfo  节点信息
 * @param[out]
 * @return
 */
static INT32 Sva_DrvDbgCommModStsInit(UINT32 uiNodeId, STS_MOD_NODE_INFO_S *pstNodeInfo)
{
    UINT32 i;

    SVA_DBG_NODE_PRM_S astNodePrm[7] =
    {
        {
            .acNodeName = "input_frmRate",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommViFrmRate,
        },

        {
            .acNodeName = "input_cnt",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommInputCnt,
        },

        {
            .acNodeName = "output_cnt",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommOutputCnt,
        },

        {
            .acNodeName = "output_end_cnt",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommOutputEndCnt,
        },

        {
            .acNodeName = "err_cnt",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommErrCnt,
        },

        {
            .acNodeName = "jenc_cnt",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommJencCnt,
        },

        {
            .acNodeName = "jenc_errCnt",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommJencErrCnt,
        },
    };

    STS_MOD_NODE_INFO_S *pstTmpNode = NULL;

    pstNodeInfo->layer_id = uiNodeId;
    pstNodeInfo->stNodePrm.enValType = STS_VAL_TYPE_NONE;
    sprintf(pstNodeInfo->stNodePrm.acNodeName, "%s", "mod_sts");

    pstNodeInfo->child_node_cnt = 7;

    for (i = 0; i < pstNodeInfo->child_node_cnt; i++)
    {
        pstNodeInfo->pNextNode[i] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");

        pstTmpNode = (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[i];

        pstTmpNode->layer_id = i;
        /* snprintf(pstTmpNode->stNodePrm.acNodeName, 32, "%s", astNodePrm[i].acNodeName); */
        sal_memcpy_s(pstTmpNode->stNodePrm.acNodeName, sizeof(pstTmpNode->stNodePrm.acNodeName), astNodePrm[i].acNodeName, sizeof(astNodePrm[i].acNodeName));
        pstTmpNode->stNodePrm.enValType = astNodePrm[i].enNodeValType;
        pstTmpNode->stNodePrm.stNodeValInfo.pGetValCb = (sts_get_outer_val)astNodePrm[i].pFunc;

        pstTmpNode->child_node_cnt = 0;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvDbgCommAppCfgInit
 * @brief       应用层配置子树初始化
 * @param[in]   UINT32 uiNodeId  节点ID
 *              STS_MOD_NODE_INFO_S *pstNodeInfo  节点信息
 * @param[out]
 * @return
 */
static INT32 Sva_DrvDbgCommAppCfgInit(UINT32 uiNodeId, STS_MOD_NODE_INFO_S *pstNodeInfo)
{
    UINT32 i;

    SVA_DBG_NODE_PRM_S astNodePrm[5] =
    {
        {
            .acNodeName = "direction",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommDirectionCfg,
        },

        {
            .acNodeName = "osd_extType",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommOsdExtTypeCfg,
        },

        {
            .acNodeName = "osd_scaleLevel",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommOsdScaleLevelCfg,
        },

        {
            .acNodeName = "region",
            .enNodeValType = STS_VAL_TYPE_STRING,
            .pFunc = Sva_DrvGetDbgCommRegionCfg,
        },

        {
            .acNodeName = "alert",
            .enNodeValType = STS_VAL_TYPE_STRING,
            .pFunc = Sva_DrvGetDbgCommAlertCfg,
        },
    };

    STS_MOD_NODE_INFO_S *pstTmpNode = NULL;

    pstNodeInfo->layer_id = uiNodeId;
    pstNodeInfo->stNodePrm.enValType = STS_VAL_TYPE_NONE;
    sprintf(pstNodeInfo->stNodePrm.acNodeName, "%s", "app_cfg");

    pstNodeInfo->child_node_cnt = 5;

    for (i = 0; i < pstNodeInfo->child_node_cnt; i++)
    {
        pstNodeInfo->pNextNode[i] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");

        pstTmpNode = (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[i];

        pstTmpNode->layer_id = i;
        sal_memcpy_s(pstTmpNode->stNodePrm.acNodeName, sizeof(pstTmpNode->stNodePrm.acNodeName), astNodePrm[i].acNodeName, sizeof(astNodePrm[i].acNodeName));
        /* snprintf(pstTmpNode->stNodePrm.acNodeName, sizeof(pstTmpNode->stNodePrm.acNodeName), "%s", astNodePrm[i].acNodeName); */
        pstTmpNode->stNodePrm.enValType = astNodePrm[i].enNodeValType;
        pstTmpNode->stNodePrm.stNodeValInfo.pGetValCb = (sts_get_outer_val)astNodePrm[i].pFunc;

        pstTmpNode->child_node_cnt = 0;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvDbgCommPrvtCfgInit
 * @brief       模块私有配置子树初始化
 * @param[in]   UINT32 uiNodeId  节点ID
 *              STS_MOD_NODE_INFO_S *pstNodeInfo  节点信息
 * @param[out]
 * @return
 */
static INT32 Sva_DrvDbgCommPrvtCfgInit(UINT32 uiNodeId, STS_MOD_NODE_INFO_S *pstNodeInfo)
{
    UINT32 i;

    STS_MOD_NODE_INFO_S *pstTmpNode = NULL;

    SVA_DBG_NODE_PRM_S astNodePrm[26] =
    {
        {
            .acNodeName = "SendFrmStuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtSendFrmInfo,
        },
        {
            .acNodeName = "SendFrameNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtSendFrmNum,
        },
        {
            .acNodeName = "SendFrameSuccNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtSendFrmSuccNum,
        },
        {
            .acNodeName = "SendFrameFailNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtSendFrmFailNum,
        },

        {
            .acNodeName = "RecvFrmStuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtRecvFrmInfo,
        },
        {
            .acNodeName = "RecvFrameNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtRecvFrmNum,
        },
        {
            .acNodeName = "RecvFrameSuccNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtRecvFrmSuccNum,
        },
        {
            .acNodeName = "RecvFrameFailNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtRecvFrmFailNum,
        },

        {
            .acNodeName = "SendIaRsltStuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtSendIaRsltInfo,
        },
        {
            .acNodeName = "SendIaRsltNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtSendIaRsltNum,
        },
        {
            .acNodeName = "SendIaRsltSuccNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtSendIaRsltSuccNum,
        },
        {
            .acNodeName = "SendIaRsltFailNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtSendIaRsltFailNum,
        },
        {
            .acNodeName = "RecvIaRsltStuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtRecvIaRsltInfo,
        },
        {
            .acNodeName = "RecvIaRsltNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtRecvIaRsltNum,
        },
        {
            .acNodeName = "RecvIaRsltSuccNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtRecvIaRsltSuccNum,
        },
        {
            .acNodeName = "RecvIaRsltFailNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtRecvIaRsltFailNum,
        },
        {
            .acNodeName = "InputEngDataStuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtIptEngDataInfo,
        },
        {
            .acNodeName = "InputEngDataNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtIptEngDataNum,
        },
        {
            .acNodeName = "InputEngDataSuccNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtIptEngSuccDataNum,
        },
        {
            .acNodeName = "InputEngDataFailNum",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtIptEngFailDataNum,
        },
        {
            .acNodeName = "JencMirrorStuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtJencMirrorFlag,
        },
        {
            .acNodeName = "JencBmpStuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtJencBmpFlag,
        },
        {
            .acNodeName = "JencJpg1StuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtJencJpg1Flag,
        },
        {
            .acNodeName = "JencJpg2StuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtJencJpg2Flag,
        },
        {
            .acNodeName = "JencSplitModeStuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtJencSplitModeFlag,
        },
        {
            .acNodeName = "JencCbStuckFlag",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgCommPrvtJencCbFlag,
        },
    };

    pstNodeInfo->layer_id = uiNodeId;
    sprintf(pstNodeInfo->stNodePrm.acNodeName, "%s", "prvt");
    pstNodeInfo->stNodePrm.enValType = STS_VAL_TYPE_NONE;          /* 非叶子节点不需要打印变量 */

    pstNodeInfo->child_node_cnt = 26;
    for (i = 0; i < pstNodeInfo->child_node_cnt; i++)
    {
        pstNodeInfo->pNextNode[i] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");
        if (NULL == pstNodeInfo->pNextNode[i])
        {
            SVA_LOGE("zalloc failed! \n");
            continue;
        }

        pstTmpNode = (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[i];

        pstTmpNode->layer_id = i;
        /* snprintf(pstTmpNode->stNodePrm.acNodeName, 32, "%s", astNodePrm[i].acNodeName); */
        sal_memcpy_s(pstTmpNode->stNodePrm.acNodeName, sizeof(pstTmpNode->stNodePrm.acNodeName), astNodePrm[i].acNodeName, sizeof(astNodePrm[i].acNodeName));
        pstTmpNode->stNodePrm.enValType = astNodePrm[i].enNodeValType;
        pstTmpNode->stNodePrm.stNodeValInfo.pGetValCb = (sts_get_outer_val)astNodePrm[i].pFunc;

        pstTmpNode->child_node_cnt = 0;
    }

    /* 暂无模块私有信息 */
    return SAL_SOK;
}

/**
 * @function    Sva_DrvDbgCommInit
 * @brief       模块通用状态子树初始化
 * @param[in]   UINT32 uiNodeId  节点ID
 *              STS_MOD_NODE_INFO_S *pstNodeInfo  节点信息
 * @param[out]
 * @return
 */
static INT32 Sva_DrvDbgCommInit(UINT32 uiNodeId, STS_MOD_NODE_INFO_S *pstNodeInfo)
{
    /* 初始化第一个子树---comm */
    pstNodeInfo->layer_id = uiNodeId;
    sprintf(pstNodeInfo->stNodePrm.acNodeName, "%s", "comm");
    pstNodeInfo->stNodePrm.enValType = STS_VAL_TYPE_NONE;          /* 非叶子节点不需要打印变量 */

    pstNodeInfo->child_node_cnt = 3;
    pstNodeInfo->pNextNode[0] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");
    pstNodeInfo->pNextNode[1] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");
    pstNodeInfo->pNextNode[2] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");

    /* 子树: 模块通用状态sts */
    (VOID)Sva_DrvDbgCommModStsInit(SVA_MOD_COMM_STS_IDX, (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[0]);

    /* 子树: 外部配置参数app_cfg */
    (VOID)Sva_DrvDbgCommAppCfgInit(SVA_MOD_COMM_APP_CFG_IDX, (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[1]);

    /* 子树: 模块私有信息prvt_info */
    (VOID)Sva_DrvDbgCommPrvtCfgInit(SVA_MOD_COMM_PRVT_INFO_IDX, (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[2]);

    return SAL_SOK;
}

/**
 * @function    Sva_DrvDbgAlgStsInit
 * @brief       算法通用状态子树初始化
 * @param[in]   UINT32 uiNodeId  节点ID
 *              STS_MOD_NODE_INFO_S *pstNodeInfo  节点信息
 * @param[out]
 * @return
 */
static INT32 Sva_DrvDbgAlgStsInit(UINT32 uiNodeId, STS_MOD_NODE_INFO_S *pstNodeInfo)
{
    UINT32 i;

    SVA_DBG_NODE_PRM_S astNodePrm[10] =
    {
        {
            .acNodeName = "proc_mode",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgAlgModeInfo,
        },

        {
            .acNodeName = "average_ms",
            .enNodeValType = STS_VAL_TYPE_FLOAT,
            .pFunc = Sva_DrvGetDbgAlgAveMs,
        },
        {
            .acNodeName = "min_ms",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgAlgMinMs,
        },
        {
            .acNodeName = "max_ms",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgAlgMaxMs,
        },

        {
            .acNodeName = "input_cnt",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgAlgInputCnt,
        },

        {
            .acNodeName = "output_cnt",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgAlgOutputCnt,
        },

        {
            .acNodeName = "err_cnt",
            .enNodeValType = STS_VAL_TYPE_INT,
            .pFunc = Sva_DrvGetDbgAlgErrCnt,
        },

        {
            .acNodeName = "icf_version",
            .enNodeValType = STS_VAL_TYPE_STRING,
            .pFunc = Sva_DrvGetDbgAlgVersion,
        },

        {
            .acNodeName = "model_version",
            .enNodeValType = STS_VAL_TYPE_STRING,
            .pFunc = Sva_DrvGetDbgAlgModelVer,
        },

        {
            .acNodeName = "mem_info",
            .enNodeValType = STS_VAL_TYPE_STRING,
            .pFunc = Sva_DrvGetDbgAlgMemInfo,
        },
    };

    STS_MOD_NODE_INFO_S *pstTmpNode = NULL;

    pstNodeInfo->layer_id = uiNodeId;
    pstNodeInfo->stNodePrm.enValType = STS_VAL_TYPE_NONE;
    sprintf(pstNodeInfo->stNodePrm.acNodeName, "%s", "alg_sts");

    pstNodeInfo->child_node_cnt = 10;

    for (i = 0; i < pstNodeInfo->child_node_cnt; i++)
    {
        pstNodeInfo->pNextNode[i] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");

        pstTmpNode = (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[i];

        pstTmpNode->layer_id = i;
        /* sprintf(pstTmpNode->stNodePrm.acNodeName, "%s", astNodePrm[i].acNodeName); */
        sal_memcpy_s(pstTmpNode->stNodePrm.acNodeName, sizeof(pstTmpNode->stNodePrm.acNodeName), astNodePrm[i].acNodeName, sizeof(astNodePrm[i].acNodeName));
        pstTmpNode->stNodePrm.enValType = astNodePrm[i].enNodeValType;
        pstTmpNode->stNodePrm.stNodeValInfo.pGetValCb = (sts_get_outer_val)astNodePrm[i].pFunc;

        pstTmpNode->child_node_cnt = 0;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvDbgAlgCfgInit
 * @brief       算法配置子树初始化
 * @param[in]   UINT32 uiNodeId  节点ID
 *              STS_MOD_NODE_INFO_S *pstNodeInfo  节点信息
 * @param[out]
 * @return
 */
static INT32 Sva_DrvDbgAlgCfgInit(UINT32 uiNodeId, STS_MOD_NODE_INFO_S *pstNodeInfo)
{
    UINT32 i;

    CHAR *pcNodeName[8] =
    {
        "pd_procGap", "obd_downSmpScale", "pkg_sensity", "zoom_inout",
        "pack_overX2", "pk_switch", "color_filterSwitch", "size_filterSwitch"
    };

    STS_NODE_VAL_TYPE_E aenNodeType[8] =
    {
        STS_VAL_TYPE_INT, STS_VAL_TYPE_FLOAT, STS_VAL_TYPE_INT, STS_VAL_TYPE_FLOAT,
        STS_VAL_TYPE_FLOAT, STS_VAL_TYPE_INT, STS_VAL_TYPE_INT, STS_VAL_TYPE_INT
    };

    sts_get_outer_val pFunc[8] =
    {
        Sva_DrvGetDbgAlgPdProcGapCfg, Sva_DrvGetDbgAlguiObdDownSmpScaleCfg, Sva_DrvGetDbgAlgPkgSensityCfg, Sva_DrvGetDbgAlgZoomInOutThrsCfg,
        Sva_DrvGetDbgAlgPackOverX2Cfg, Sva_DrvGetDbgAlgPkSwitchCfg, Sva_DrvGetDbgAlgColorFilterSwitchCfg, Sva_DrvGetDbgAlgSizeFilterSwitchCfg
    };

    STS_MOD_NODE_INFO_S *pstTmpNode = NULL;

    pstNodeInfo->layer_id = uiNodeId;
    pstNodeInfo->stNodePrm.enValType = STS_VAL_TYPE_NONE;
    sprintf(pstNodeInfo->stNodePrm.acNodeName, "%s", "alg_cfg");

    pstNodeInfo->child_node_cnt = 8;

    for (i = 0; i < pstNodeInfo->child_node_cnt; i++)
    {
        pstNodeInfo->pNextNode[i] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");

        pstTmpNode = (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[i];

        pstTmpNode->layer_id = i;
        sprintf(pstTmpNode->stNodePrm.acNodeName, "%s", pcNodeName[i]);
        pstTmpNode->stNodePrm.enValType = aenNodeType[i];
        pstTmpNode->stNodePrm.stNodeValInfo.pGetValCb = (sts_get_outer_val)pFunc[i];

        pstTmpNode->child_node_cnt = 0;
    }

    return SAL_SOK;
}

/**
 * @function    Sva_DrvDbgAlgInit
 * @brief       算法状态子树初始化
 * @param[in]   UINT32 uiNodeId  节点ID
 *              STS_MOD_NODE_INFO_S *pstNodeInfo  节点信息
 * @param[out]
 * @return
 */
static INT32 Sva_DrvDbgAlgInit(UINT32 uiNodeId, STS_MOD_NODE_INFO_S *pstNodeInfo)
{
    /* 初始化第二个子树---alg */
    pstNodeInfo->layer_id = uiNodeId;
    sprintf(pstNodeInfo->stNodePrm.acNodeName, "%s", "alg");
    pstNodeInfo->stNodePrm.enValType = STS_VAL_TYPE_NONE;          /* 非叶子节点不需要打印变量 */

    pstNodeInfo->child_node_cnt = 2;
    pstNodeInfo->pNextNode[0] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");
    pstNodeInfo->pNextNode[1] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");

    /* 子树: 算法通用状态 */
    (VOID)Sva_DrvDbgAlgStsInit(SVA_MOD_ALG_STS_IDX, (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[0]);

    /* 子树: 算法配置参数 */
    (VOID)Sva_DrvDbgAlgCfgInit(SVA_MOD_ALG_CFG_IDX, (STS_MOD_NODE_INFO_S *)pstNodeInfo->pNextNode[1]);

    return SAL_SOK;
}

/**
 * @function    Sva_DrvDbgInit
 * @brief       模块调试信息初始化
 * @param[in]
 * @param[out]
 * @return
 */
INT32 Sva_DrvDbgInit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    sal_memset_s(&g_sva_common.stDbgPrm.stRootNodeInfo, sizeof(STS_MOD_NODE_INFO_S), 0x00, sizeof(STS_MOD_NODE_INFO_S));

    /* 根节点初始化 */
    g_sva_common.stDbgPrm.stRootNodeInfo.layer_id = SVA_MOD_ROOT_IDX;
    g_sva_common.stDbgPrm.stRootNodeInfo.stNodePrm.enValType = STS_VAL_TYPE_NONE;
    sprintf(g_sva_common.stDbgPrm.stRootNodeInfo.stNodePrm.acNodeName, "%s", SVA_MOD_NAME);

    g_sva_common.stDbgPrm.stRootNodeInfo.child_node_cnt = 2;
    g_sva_common.stDbgPrm.stRootNodeInfo.pNextNode[0] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");
    g_sva_common.stDbgPrm.stRootNodeInfo.pNextNode[1] = SAL_memZalloc(sizeof(STS_MOD_NODE_INFO_S), "sva", "dbg");

    /* comm子树初始化 */
    s32Ret = Sva_DrvDbgCommInit(SVA_MOD_COMM_IDX, (STS_MOD_NODE_INFO_S *)g_sva_common.stDbgPrm.stRootNodeInfo.pNextNode[0]);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("dbg comm init failed! \n");
        return SAL_FAIL;
    }

    /* alg子树初始化 */
    s32Ret = Sva_DrvDbgAlgInit(SVA_MOD_ALG_TREE_IDX, (STS_MOD_NODE_INFO_S *)g_sva_common.stDbgPrm.stRootNodeInfo.pNextNode[1]);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("dbg comm init failed! \n");
        return SAL_FAIL;
    }

    /* 全局维护变量初始化 */
    s32Ret = Sva_DrvInitDbgGlobalVar();
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("init dbg global var failed! \n");
        return SAL_FAIL;
    }

    /* 维护模块注册 */
    s32Ret = sts_reg_entry(&g_sva_common.stDbgPrm.stRootNodeInfo, (VOID **)&g_sva_common.stDbgPrm.pHandle);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("init module dbg prm failed! s32Ret: 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    SVA_LOGI("sva dbg info init end! \n");
    return SAL_SOK;
}

#endif

