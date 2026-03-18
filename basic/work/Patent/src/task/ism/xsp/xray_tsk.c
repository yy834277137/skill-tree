/**
 * @file   xray_tsk.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  X ray解析和预处理任务
 * @author liwenbin
 * @date   2019/10/19
 * @note :
 * @note \n History:
   1.Date        : 2019/10/19
     Author      : liwenbin
     Modification: Created file
   1.Date        : 2019/10/29
     Author      : liwenbin
     Modification: 增加转存通道相关功能
 */


/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"
#include "system_prm_api.h"
#include "xray_tsk.h"
#include "xsp_drv.h"
#include "ximg_proc.h"

#line __LINE__ "xray_tsk.c"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
/* XRAY模块空指针校验 */
#define XRAY_CHECK_PTR_IS_NULL(ptr, errno) \
    do { \
        if (!ptr) \
        { \
            XSP_LOGE("the '%s' is NULL\n", # ptr); \
            return (errno); \
        } \
    } while (0)

#define XRAY_CHECK_PTR_NULL(ptr) \
    do { \
        if (!ptr) \
        { \
            XSP_LOGE("the '%s' is NULL\n", # ptr); \
        } \
    } while (0)

/* X ray处理的数据信息 */
#define XRAY_RAW_BUFFER_NUM             2
#define XRAY_CORR_BUFFER_NUM            2
#define XRAY_XSENSOR_STAT_NUM           160
#define XRAY_DEBUG_PB_FARME_NUM         32
#define XRAY_DEBUG_PB_SLICE_NUM         320
#define XRAY_DEBUG_PB_PACK_RST_NUM      64

#define DP_BRIGHT_ZERO_LE_TH	        10000   /* 本底上的亮坏点阈值，低能板，高于此阈值认为是亮坏点 */
#define DP_BRIGHT_ZERO_HE_TH	        10000   /* 本底上的亮坏点阈值，高能板，高于此阈值认为是亮坏点 */
#define DP_DARK_FULL_LE_TH		        2000    /* 满载上的暗坏点阈值，低能板，低于此阈值认为是暗坏点 */
#define DP_DARK_FULL_HE_TH		        2000    /* 满载上的暗坏点阈值，高能板，低于此阈值认为是暗坏点 */
#define DP_CONTINUOUS_UNALBE_TO_HANDLE	3       /* 连续X个坏点以上（包含X）当前算法无法处理 */
#define DP_UNHANDLED_TOTAL_NUM			10      /* 坏点总数超过10个以上的，视为硬件存在问题，不处理 */

#define SEPARATE_XRAW_SIZE              0x6400000   /* XRAW分割对应的字节大小（100MB） */


/* 坏点处理方式，XRAY_DEADPIXEL中handle_method对应的值 */
typedef enum
{
    XRAW_DP_HANDLE_METHOD_NA = 0,       /* 坏点未处理 */
    XRAW_DP_UNHANDLED,                  /* 无法处理的坏点 */
    XRAW_DP_COPY_PRE,                   /* 拷贝前坏点 */
    XRAW_DP_COPY_PRE_X2,                /* 拷贝前前坏点 */
    XRAW_DP_COPY_NEXT,                  /* 拷贝后坏点 */
    XRAW_DP_COPY_NEXT_X2,               /* 拷贝后后坏点 */
    XRAW_DP_INTERPOLATE                 /* 插值 */
} XRAW_DP_HANDLE_METHOD;


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef struct
{
    UINT8 *pPhysAddr;       /* 物理地址 */
    UINT8 *pVirtAddr;       /* 虚拟地址 */
    UINT32 u32BufSize;      /* Buffer长度 */
    UINT32 u32WriteIdx;     /* 写索引 */
    UINT32 u32ReadIdx;      /* 读索引 */
    pthread_mutex_t mutexCtrl;
} XRAW_BUFFER;

typedef struct
{
    UINT32 u32FrameNo;              /* 回拉帧序号 */
    UINT32 u32Width;                /* 数据宽度 */
    UINT32 u32Height;               /* 数据高度 */
    UINT32 u32DataLen;              /* 除该结构体外的数据长度 */
    XRAY_PROC_DIRECTION direction;  /* 出图方向，XRAY_DIRECTION_RIGHT - L2R，XRAY_DIRECTION_LEFT - R2L */
    XRAY_PB_TYPE uiWorkMode;        /* 0-整包回拉（文件管理转回拉、TIP抠图），1-条带回拉（实时预览转回拉） */
    UINT32 uiSliceNum;              /* 回拉帧中条带个数，即XRAY_PB_SLICE_HEADER的个数，半个条带计一个条带，XSP_SLICE_NEIGHBOUR类型的条带也要计入 */
    UINT32 uiPackageNum;            /* 回拉帧中包裹个数 */
    XSP_PB_OPDIR enPbOpDir;         /* 回拉的操作方向，1-正向回拉，2-反向回拉 */
    UINT32 u32PbNeighbourTop;       /* 回拉数据上邻域高度 */
    UINT32 u32PbNeighbourBottom;    /* 回拉数据下邻域高度 */
} XRAY_DEBUG_PB_FARME;

typedef struct
{
    UINT64 u64ColNo;                /* 条带的序号 */
    UINT32 u32Size;                 /* 条带数据大小，若为空白条带，且该值为0时，DSP会根据u32Width和u32HeightIn构建条带，单位：字节 */
    UINT32 u32Width;                /* 条带的宽度，探测器方向 */
    UINT32 u32HeightIn;             /* 条带的输入高度，传送带方向，小于u32HightTotal时表示仅有半个条带 */
    UINT32 u32HeightTotal;          /* 条带的总高度，传送带方向，等于u32HeightIn时表示整个条带 */
    XSP_SLICE_CONTENT enSliceCont;  /* 空白条带、包裹条带 */
    XSP_PACKAGE_TAG enPackTag;      /* 包裹起始、结束标记 */
    BOOL bAppendPackageResult;      /* 该结构体后是否有追加包裹信息XRAY_PB_PACKAGE_RESULT */
    UINT32 u32SliceIdx;             /* 条带索引号 */
    UINT32 u32AssociatedFrameNo;    /* 关联回拉帧序号 */
} XRAY_DEBUG_PB_SLICE;

typedef struct
{
    UINT32 u32PackageId;            /* 包裹ID号 */
    UINT32 u32PackageWidth;         /* 包裹宽 */
    UINT32 u32PackageHeight;        /* 包裹高 */
    SVA_DSP_OUT stSvaInfo;          /* 智能信息 */
    XSP_PACK_INFO stProPrm;           /* 成像信息 */
    UINT64 u64AssociatedColNo;      /* 关联条带序号 */
} XRAY_DEBUG_PB_PACK_RST;

typedef struct
{
    UINT32 u32FrameStartIdx;        /* 回拉帧起始索引，范围[0, XRAY_DEBUG_PB_FARME_NUM-1] */
    XRAY_DEBUG_PB_FARME stPbFrame[XRAY_DEBUG_PB_FARME_NUM];
    UINT32 u32SliceStartIdx;        /* 条带起始索引，范围[0, XRAY_DEBUG_PB_SLICE_NUM-1] */
    XRAY_DEBUG_PB_SLICE stPbSlice[XRAY_DEBUG_PB_SLICE_NUM];
    UINT32 u32PackRstStartIdx;      /* 包裹信息起始索引，范围[0, XRAY_DEBUG_PB_PACK_RST_NUM-1] */
    XRAY_DEBUG_PB_PACK_RST stPackRst[XRAY_DEBUG_PB_PACK_RST_NUM];
} XRAY_PB_DEBUG;

typedef struct
{
    UINT32 u32Colno;            // Xsensor输入的条带号统计
    UINT64 u64SyncTime;         // Xsensor输入的时间统计
    XRAY_PROC_TYPE enProcType;  // Xsensor输入的数据类型
    UINT32 au32PackSign[8];    /* 是否有包裹，按位表示，0-无，1-有，低位对应先扫描行数据，高位对应后扫描行数据 */
} XSENSOR_DEBUG;

typedef struct
{
    UINT32 u32XrayRtProcCnt;    /**< 实时预览处理次数 */
    UINT32 u32XrayPbProcCnt;    /**< 回拉处理次数 */
    UINT32 u32FullCorrCnt;      /**< 手动满载校正次数 */
    UINT32 u32ZeroCorrCnt;      /**< 手动本底校正次数 */
    BOOL bXrawMark;             /**< XRAW数据打标记 */
    UINT32 u32XsensorStartIdx;  /**< 起始索引，范围[0, XRAY_XSENSOR_STAT_NUM-1] */
    XSENSOR_DEBUG stXsensor[XRAY_XSENSOR_STAT_NUM];
} XRAY_RT_DEBUG;

/**
 * @struct XRAY_TSK_CTRL
 * @brief X ray解析预处理任务控制结构体
 */
typedef struct _XRAY_TSK_CTRL_
{
    UINT32 u32Chan;                     /**< 通道号 */
    BOOL bXrawStoreEn;                  /**< 保存源RAW是否使能 */
    UINT32 u32FscSliceNumMax;           /**< 全屏数据条带最多个数 */
    XSP_ZDATA_VERSION enPbZVer;         /**< 最后一次条带回拉包裹中的Z值版本号 */

    XRAY_DEADPIXEL_RESULT dp_info;      /**< 坏点信息 */
    XRAY_DEADPIXEL_RESULT dp_log;       /**< 坏点信息 */

    BOOL bShareBufReading;              /**< 是否正在读取共享Buffer */
    COND_T condShareBufRead;            /**< 共享缓存区读指针的状态变化，与enShareBufStat一起使用 */
    XRAY_SHARE_BUF *pstShareBufRt;      /**< 实时过包共享Buffer */
    XRAY_SHARE_BUF *pstShareBufPb;      /**< 回拉共享Buffer */
    XRAY_ELEMENT *pstElemHdrBuf;        /**< 存放Element Header的Buffer，当Element Header在共享Buffer中回头时，会将Element Header先COPY到该Buffer */
    XRAY_PB_SLICE_HEADER *pstPbHdr;     /**< 存放XRAY_PB_SLICE_HEADER的Buffer，当XRAY_PB_SLICE_HEADER在共享Buffer中回头时，会先COPY到该Buffer */
    XRAY_PB_PACKAGE_RESULT *pstPackRst; /**< 存放XRAY_PB_PACKAGE_RESULT的Buffer，当XRAY_PB_PACKAGE_RESULT在共享Buffer中回头时，会先COPY到该Buffer */

    XRAY_PB_DEBUG stPbDebug;            /**< 回拉调试信息 */
    XRAY_RT_DEBUG stRtDebug;            /**< 实时预览信息 */
    UINT32 u32XrawLineNum;              /**< XRAW行数 */
    UINT32 u32XrawBufIdx;               /**< XRAW Ping-Pong Buffer索引 */
    XRAW_BUFFER stXrawBuf[XRAY_RAW_BUFFER_NUM]; // use as Ping-Pong Buffer
    UINT32 u32XcorrBufIdx;              /**< XCORR Ping-Pong Buffer索引 */
    XRAW_BUFFER stXcorrBuf[XRAY_CORR_BUFFER_NUM]; // use as Ping-Pong Buffer
    XRAW_BUFFER stXcorrFullSplit;       /**< 满载校正分割线 */
    XRAW_BUFFER stXcorrZeroSplit;       /**< 本底校正分割线 */
    BOOL bSyncTimeMarkCb;               /**< 记录是否回调条带时间 */
    UINT64 u64SyncTimeStart;            /**< 回调给应用的xraw开始条带时间 */
    UINT64 u64SyncTimeEnd;              /**< 回调给应用的xraw结束条带时间 */
} XRAY_TSK_CTRL;

typedef struct
{
    pthread_mutex_t mutexTrans;     ///< 转存的互斥锁，控制Host_XrayTransform接口的不可重入
    UINT32 u32TransTimeout;         ///< 单次转存的超时时间，单位ms
} XRAY_TRANS_CTRL;


/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */

/* X ray解析预处理任务控制参数 */
static XRAY_TSK_CTRL gstXrayTskCtrl[MAX_XRAY_CHAN];
static XRAY_TRANS_CTRL gstXrayTransCtrl;


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */
extern SAL_STATUS Xsp_ConfigTransProcParam(BOOL bSetFlag, XSP_PROC_TYPE_UN unXspPrcoType, XSP_RT_PARAMS *punXspProcParamIn);
extern void trans2bitstr(CHAR *pStr, void *pData, UINT32 u32BitWidth);


static void *Xray_HalModMalloc(UINT32 uiBufSize, UINT32 alineBytes)
{
    return SAL_memMalloc(uiBufSize, "xray", "tsk");
}


static void Xray_HalModFree(void *pPtr)
{
    SAL_memfree(pPtr, "xray", "tsk");
}


/**
 * @fn      xraw_dp_merge
 * @brief   本底亮坏点和满载暗坏点合并，并根据位置从小到大排序
 * @note    若同一位置即有亮坏点，又有暗坏点，是以p_dp1为准的，理论上不存在这种情况
 *
 * @param   p_dp_merge[OUT] 合并后的坏点结果
 * @param   p_dp1[IN] 坏点1，本底亮坏点或满载暗坏点均可
 * @param   p_dp2[IN] 坏点2，本底亮坏点或满载暗坏点均可
 */
static void xraw_dp_merge(XRAY_DEADPIXEL_RESULT *p_dp_merge, XRAY_DEADPIXEL_RESULT *p_dp1, XRAY_DEADPIXEL_RESULT *p_dp2)
{
    UINT32 i = 0, j = 0, k = 0; /* i: p_dp1's index, j: p_dp2's index, k: p_dp_merge's index */
    UINT32 dp_num_total = p_dp1->deadPixelNum + p_dp2->deadPixelNum;

    if (0 == dp_num_total)
    {
        return;
    }
    else if (dp_num_total > XRAY_MAX_DEAD_PIXEL_NUM)
    {
        dp_num_total = XRAY_MAX_DEAD_PIXEL_NUM;
    }

    /* 如果p_dp1或p_dp2为空，则直接将p_dp2或p_dp1作为合并结果 */
    if (0 == p_dp1->deadPixelNum)
    {
        memcpy(p_dp_merge, p_dp2, sizeof(XRAY_DEADPIXEL_RESULT));
    }
    else if (0 == p_dp2->deadPixelNum)
    {
        memcpy(p_dp_merge, p_dp1, sizeof(XRAY_DEADPIXEL_RESULT));
    }
    else /* p_dp1和p_dp2都不为空，求它们并集，按坏点位置从小到大进行排序 */
    {
        while (k < dp_num_total)
        {
            if (i < p_dp1->deadPixelNum && j < p_dp2->deadPixelNum) /* step 1 */
            {
                /* p_dp1与p_dp2能量板类型相同的情况 */
                if ((XRAY_ENERGY_LOW == p_dp1->deadPixel[i].energy_board && XRAY_ENERGY_LOW == p_dp2->deadPixel[j].energy_board)
                    || (XRAY_ENERGY_HIGH == p_dp1->deadPixel[i].energy_board && XRAY_ENERGY_HIGH == p_dp2->deadPixel[j].energy_board)) /* step 1.1 */
                {
                    if (p_dp1->deadPixel[i].offset < p_dp2->deadPixel[j].offset)
                    {
                        memcpy(&p_dp_merge->deadPixel[k++], &p_dp1->deadPixel[i++], sizeof(XRAY_DEADPIXEL));
                    }
                    else if (p_dp1->deadPixel[i].offset > p_dp2->deadPixel[j].offset)
                    {
                        memcpy(&p_dp_merge->deadPixel[k++], &p_dp2->deadPixel[j++], sizeof(XRAY_DEADPIXEL));
                    }
                    else /* 既是亮坏点又是暗坏点的情况，这种属于异常 */
                    {
                        memcpy(&p_dp_merge->deadPixel[k++], &p_dp1->deadPixel[i++], sizeof(XRAY_DEADPIXEL));
                        j++;   /*单独j++导致K未增加，导致k< dp_num_total一直成立死循环*/
                    }
                }
                else if (XRAY_ENERGY_LOW == p_dp1->deadPixel[i].energy_board
                         && XRAY_ENERGY_HIGH == p_dp2->deadPixel[j].energy_board) /* step 1.2 */
                {
                    /* 将所有p_dp1中XRAY_ENERGY_LOW属性的坏点直接合并到p_dp_merge，使遍历完p_dp1 */
                    while (i < p_dp1->deadPixelNum && k < dp_num_total && XRAY_ENERGY_LOW == p_dp1->deadPixel[i].energy_board)
                    {
                        memcpy(&p_dp_merge->deadPixel[k++], &p_dp1->deadPixel[i++], sizeof(XRAY_DEADPIXEL));
                    }
                }
                else if (XRAY_ENERGY_HIGH == p_dp1->deadPixel[i].energy_board
                         && XRAY_ENERGY_LOW == p_dp2->deadPixel[j].energy_board) /* step 1.3 */
                {
                    /* 将所有p_dp2中XRAY_ENERGY_LOW属性的坏点直接合并到p_dp_merge，使遍历完p_dp2 */
                    while (j < p_dp2->deadPixelNum && k < dp_num_total && XRAY_ENERGY_LOW == p_dp2->deadPixel[j].energy_board)
                    {
                        memcpy(&p_dp_merge->deadPixel[k++], &p_dp2->deadPixel[j++], sizeof(XRAY_DEADPIXEL));
                    }
                }
                else
                {
                    if((XRAY_ENERGY_LOW != p_dp1->deadPixel[i].energy_board) 
                        && (XRAY_ENERGY_HIGH != p_dp1->deadPixel[i].energy_board))
                    {
                        XSP_LOGE("dp[%u]'s energy_board[%u] is invalid\n", i, p_dp1->deadPixel[i].energy_board);
                        i++;
                    }
                    if((XRAY_ENERGY_LOW != p_dp2->deadPixel[j].energy_board) 
                        && (XRAY_ENERGY_HIGH != p_dp2->deadPixel[j].energy_board))
                    {
                        XSP_LOGE("dp[%u]'s energy_board[%u] is invalid\n", j, p_dp2->deadPixel[j].energy_board);
                        j++;
                    }
                }
            }
            else if (i == p_dp1->deadPixelNum) /* step 2.1 p_dp1先遍历完，直接合并p_dp2剩余的 */
            {
                while (j < p_dp2->deadPixelNum && k < dp_num_total)
                {
                    memcpy(&p_dp_merge->deadPixel[k++], &p_dp2->deadPixel[j++], sizeof(XRAY_DEADPIXEL));
                }
                break;    /*j和k有一个满了直接退出*/
            }
            else if (j == p_dp2->deadPixelNum) /* step 2.2 p_dp2先遍历完，直接合并p_dp1剩余的 */
            {
                while (i < p_dp1->deadPixelNum && k < dp_num_total)
                {
                    memcpy(&p_dp_merge->deadPixel[k++], &p_dp1->deadPixel[i++], sizeof(XRAY_DEADPIXEL));
                }
                break;   /*i和k有一个满了直接退出*/
            }
            else
            {
                break;
            }
        }

        p_dp_merge->deadPixelNum = k;
    }

    return;
}

/**
 * @fn      xraw_dp_detect
 * @brief   基于X-Ray RAW数据的坏点检测
 *
 * @param   p_dp_result[IN/OUT] 坏点信息，输入历史的坏点校正结果，接口内根据校正数据重新计算坏点，并和之前的合并
 * @param   p_corr_buf[IN] 本底、满载校正数据
 * @param   corr_type[IN] 校正数据类型
 * @param   raw_w[IN] 校正RAW数据宽
 * @param   raw_h[IN] 校正RAW数据宽
 * @param   energy_num[IN] 能量板数量，1-单能板，2-双能板
 *
 * @return SAL_STATUS
 */
static SAL_STATUS xraw_dp_detect(XRAY_DEADPIXEL_RESULT *p_dp_result, UINT16 *p_corr_buf, XSP_CORRECT_TYPE corr_type,
                                 UINT32 raw_w, UINT32 raw_h, UINT32 energy_num)
{
    UINT32 i = 0, j = 0, k = 0, m = 0, n = 1;
    UINT32 buf_size = 0, dp_bright_zero_th = 0, dp_dark_full_th = 0;
    UINT32 dp_remaining_num = 0; /* 除backup的坏点外，结构体XRAY_DEADPIXEL_RESULT剩余可容纳的坏点数 */
    UINT32 *p_col_avg = NULL;
    XRAY_DEADPIXEL_RESULT dp_bright = {0}, dp_dark = {0};
    XRAY_DEADPIXEL_RESULT *p_dp_new = NULL, *p_dp_backup = NULL;

    if (NULL == p_dp_result || NULL == p_corr_buf)
    {
        XSP_LOGE("the p_dp_result[%p] OR p_corr_buf[%p] is NULL\n", p_dp_result, p_corr_buf);
        return SAL_FAIL;
    }

    if (energy_num != 1 && energy_num != 2)
    {
        XSP_LOGE("the energy_num is invalid: %u\n", energy_num);
        return SAL_FAIL;
    }

    if (0 == raw_w || 0 == raw_h)
    {
        XSP_LOGE("raw_w[%u] OR raw_h[%u] is invalid\n", raw_w, raw_h);
        return SAL_FAIL;
    }

    buf_size = raw_w * sizeof(UINT32);
    p_col_avg = (UINT32 *)Xray_HalModMalloc(buf_size, 16);
    if (NULL == p_col_avg)
    {
        XSP_LOGE("malloc for 'p_col_avg' failed, buffer size: %u\n", buf_size);
        return SAL_FAIL;
    }

    if (XSP_CORR_EMPTYLOAD == corr_type)
    {
        p_dp_new = &dp_bright; /* 重置本底亮坏点 */
        p_dp_backup = &dp_dark; /* 备份满载暗坏点 */
    }
    else if (XSP_CORR_FULLLOAD == corr_type)
    {
        p_dp_new = &dp_dark; /* 清空所有的满载暗坏点 */
        p_dp_backup = &dp_bright; /* 备份本底亮坏点 */
    }
    else
    {
        XSP_LOGE("corr_type [%d] is error.\n", corr_type);
        return SAL_FAIL;
    }

    /* 从输入的p_dp_result中备份坏点 */
    for (i = 0; i < p_dp_result->deadPixelNum; i++)
    {
        if (corr_type != p_dp_result->deadPixel[i].classify)
        {
            memcpy(&p_dp_backup->deadPixel[p_dp_backup->deadPixelNum], &p_dp_result->deadPixel[i], sizeof(XRAY_DEADPIXEL));
            p_dp_backup->deadPixelNum++;
        }
    }

    dp_remaining_num = XRAY_MAX_DEAD_PIXEL_NUM - p_dp_backup->deadPixelNum;

    /* ============== 搜索新的坏点，1-低能，2-高能 ============== */
    while (n <= energy_num && k < dp_remaining_num)
    {
        memset(p_col_avg, 0, buf_size);
        if (1 == n) /* 低能 */
        {
            dp_bright_zero_th = DP_BRIGHT_ZERO_LE_TH; /* 本底 */
            dp_dark_full_th = DP_DARK_FULL_LE_TH; /* 满载 */
        }
        else if (2 == n) /* 高能 */
        {
            dp_bright_zero_th = DP_BRIGHT_ZERO_HE_TH; /* 本底 */
            dp_dark_full_th = DP_DARK_FULL_HE_TH; /* 满载 */
        }
        else
        {
            break;
        }

        /* 分别计算每列数据平均值 */
        for (i = 0; i < raw_h; i++) /* 计算每列数据的和 */
        {
            for (j = 0; j < raw_w; j++)
            {
                p_col_avg[j] += p_corr_buf[m++];
            }
        }

        for (j = 0; j < raw_w; j++) /* 计算每列数据的平均值 */
        {
            p_col_avg[j] /= raw_h;
            if ((XSP_CORR_EMPTYLOAD == corr_type && p_col_avg[j] > dp_bright_zero_th)    /* 本底亮坏点 */
                || (XSP_CORR_FULLLOAD == corr_type && p_col_avg[j] < dp_dark_full_th)) /* 满载暗坏点 */
            {
                p_dp_new->deadPixel[k].offset = j;
                p_dp_new->deadPixel[k].energy_board = n;
                p_dp_new->deadPixel[k].classify = corr_type;
                k++; /* while循环结束后，p_dp_new中有k个坏点 */
                XSP_LOGD("%s energy board, %s dead pixel detected, pos: %u, num: %u\n", \
                         (1 == n) ? "low" : "high",
                         (XSP_CORR_EMPTYLOAD == corr_type) ? "bright" : "dark", j, k);
                if (k >= dp_remaining_num)
                {
                    XSP_LOGW("%s energy board, too many dead pixels detected\n", (1 == n) ? "low" : "high");
                    break;
                }
            }
        }

        #if 0
        /* 本底、满载均值 */
        CHAR corr_avg_dump[128] = {0};
        snprintf(corr_avg_dump, sizeof(corr_avg_dump), "/mnt/dspdump/dp_%s_%s_w%u_h%u.xbin", \
                 (1 == n) ? "le" : "he", (XSP_CORR_EMPTYLOAD == corr_type) ? "emptyload" : "fullload", raw_w, raw_h);
        SAL_WriteToFile(corr_avg_dump, 0, SEEK_SET, p_col_avg, buf_size);
        #endif

        XSP_LOGW("total %u %s dead pixels after detecting on %s energy board\n", \
                 k, (XSP_CORR_EMPTYLOAD == corr_type) ? "bright" : "dark", (1 == n) ? "low" : "high");

        n++;
    }

    p_dp_new->deadPixelNum = k;
    Xray_HalModFree(p_col_avg);

    memset(p_dp_result, 0, sizeof(XRAY_DEADPIXEL_RESULT));
    xraw_dp_merge(p_dp_result, p_dp_new, p_dp_backup);

    return SAL_SOK;
}

/**
 * @fn      xraw_dp_make_up_handle_method
 * @brief   生成每个坏点的处理方式，XRAW_DP_HANDLE_METHOD
 *
 * @param   p_dp_result[IN/OUT] 输入坏点信息，输出坏点的处理方式
 * @param   raw_w[IN] 校正RAW数据宽
 */
static void xraw_dp_make_up_handle_method(XRAY_DEADPIXEL_RESULT *p_dp_result, UINT32 raw_w)
{
    UINT32 i = 0, j = 0, k = 0;
    UINT32 group_start_pos = 0, cnt = 1; /* group_start_pos：连续坏点起始位置，cnt：连续坏点个数 */

    if (p_dp_result->deadPixelNum > DP_UNHANDLED_TOTAL_NUM)
    {
        XSP_LOGW("the dead pixel number is %u, more than %u, no need to filter\n",
                 p_dp_result->deadPixelNum, DP_UNHANDLED_TOTAL_NUM);
        return;
    }

    /* 遍历连续坏点 */
    for (i = 0, j = 1; i < p_dp_result->deadPixelNum; i++, j++)
    {
        if (j < p_dp_result->deadPixelNum
            && p_dp_result->deadPixel[i].offset + 1 == p_dp_result->deadPixel[j].offset)
        {
            cnt++;
        }
        else
        {
            group_start_pos = p_dp_result->deadPixel[i].offset - (cnt - 1);

            /**
             * 对所有连续超过DP_CONTINUOUS_UNALBE_TO_HANDLE个坏点的均不处理
             * 若头部和尾部有多个连续坏点，则认为是遮挡，不处理
             * 若中间有连续多个坏点，认为采集板有问题，产测需要检出不良
             */
            if (cnt >= DP_CONTINUOUS_UNALBE_TO_HANDLE)
            {
                for (k = i; k < cnt; k++)
                {
                    p_dp_result->deadPixel[k].handle_method = XRAW_DP_UNHANDLED;
                }
            }
            else if (cnt == 2) /* 连续两个坏点 */
            {
                if (0 == group_start_pos) /* 头部连续2个坏点，copy next->next & next */
                {
                    p_dp_result->deadPixel[i].handle_method = XRAW_DP_COPY_NEXT_X2;
                    p_dp_result->deadPixel[j].handle_method = XRAW_DP_COPY_NEXT;
                }
                else if (raw_w - 2 == group_start_pos) /* 尾部连续2个坏点，copy pre & pre->pre */
                {
                    p_dp_result->deadPixel[i].handle_method = XRAW_DP_COPY_PRE;
                    p_dp_result->deadPixel[j].handle_method = XRAW_DP_COPY_PRE_X2;
                }
                else /* 中间连续2个坏点，分别copy pre & next */
                {
                    p_dp_result->deadPixel[i].handle_method = XRAW_DP_COPY_PRE;
                    p_dp_result->deadPixel[j].handle_method = XRAW_DP_COPY_NEXT;
                }
            }
            else /* 单个坏点 */
            {
                if (0 == group_start_pos) /* 头部坏点，copy next */
                {
                    p_dp_result->deadPixel[i].handle_method = XRAW_DP_COPY_NEXT;
                }
                else if (raw_w - 1 == group_start_pos) /* 尾部坏点，copy pre */
                {
                    p_dp_result->deadPixel[i].handle_method = XRAW_DP_COPY_PRE;
                }
                else /* 中间坏点，插值 */
                {
                    p_dp_result->deadPixel[i].handle_method = XRAW_DP_INTERPOLATE;
                }
            }

            XSP_LOGD("[%u] continuous dps number: %u\n", i, cnt);

            /* 重置连续坏点个数 */
            cnt = 1;
        }
    }

    if (p_dp_result->deadPixelNum > 0)
    {
        XSP_LOGD("dp  offset  cls  eb  hm\n");
        for (i = 0; i < p_dp_result->deadPixelNum; i++)
        {
            XSP_LOGD("%2u  %6u  %3u  %2u  %2u\n", i, p_dp_result->deadPixel[i].offset, p_dp_result->deadPixel[i].classify, \
                     p_dp_result->deadPixel[i].energy_board, p_dp_result->deadPixel[i].handle_method);
        }
    }

    return;
}

/**
 * @fn      xraw_dp_correct
 * @brief   基于X-Ray RAW数据的坏点检测
 * @param   p_dp_result[IN] 坏点信息，主要使用坏点位置和校正方式
 * @param   p_raw_buf[IN] 需要校正的RAW数据Buffer
 * @param   raw_w[IN] 需要校正的RAW数据宽
 * @param   raw_h[IN] 需要校正的RAW数据高
 */
void xraw_dp_correct(XRAY_DEADPIXEL_RESULT *p_dp_result, UINT16 *p_raw_buf, UINT32 raw_w, UINT32 raw_h)
{
    UINT32 i = 0, j = 0;
    UINT16 *p_line = NULL, dp_offset = 0;

    if (NULL == p_dp_result || NULL == p_raw_buf)
    {
        XSP_LOGE("the p_dp_result[%p] OR p_raw_buf[%p] is NULL\n", p_dp_result, p_raw_buf);
        return;
    }

    static UINT32 prt_cnt = 0;

    for (i = 0; i < p_dp_result->deadPixelNum; i++)
    {
        if (XRAY_ENERGY_LOW == p_dp_result->deadPixel[i].energy_board)
        {
            p_line = p_raw_buf;
        }
        else if (XRAY_ENERGY_HIGH == p_dp_result->deadPixel[i].energy_board)
        {
            p_line = p_raw_buf + raw_w * raw_h;
        }
        else
        {
            continue;
        }

        dp_offset = p_dp_result->deadPixel[i].offset;
        switch (p_dp_result->deadPixel[i].handle_method)
        {
            case XRAW_DP_COPY_PRE:
                for (j = 0; j < raw_h; j++)
                {
                    p_line[dp_offset] = p_line[dp_offset - 1];
                    p_line += raw_w;
                }

                break;

            case XRAW_DP_COPY_PRE_X2:
                for (j = 0; j < raw_h; j++)
                {
                    p_line[dp_offset] = p_line[dp_offset - 2];
                    p_line += raw_w;
                }

                break;

            case XRAW_DP_COPY_NEXT:
                for (j = 0; j < raw_h; j++)
                {
                    p_line[dp_offset] = p_line[dp_offset + 1];
                    p_line += raw_w;
                }

                break;

            case XRAW_DP_COPY_NEXT_X2:
                for (j = 0; j < raw_h; j++)
                {
                    p_line[dp_offset] = p_line[dp_offset + 2];
                    p_line += raw_w;
                }

                break;

            case XRAW_DP_INTERPOLATE:
                for (j = 0; j < raw_h; j++)
                {
                    if (prt_cnt > 1 && prt_cnt < 4)
                    {
                        XSP_LOGD("before correct: %u, after: %u\n", p_line[dp_offset], (p_line[dp_offset - 1] + p_line[dp_offset + 1]) >> 1);
                    }

                    p_line[dp_offset] = (p_line[dp_offset - 1] + p_line[dp_offset + 1]) >> 1;
                    p_line += raw_w;
                }

                break;

            default:
                break;
        }
    }

    prt_cnt++;

    return;
}


/**
 * @fn      xraw_dp_get_info
 * @brief   获取坏点信息
 * 
 * @param   [IN] chan 通道号
 * 
 * @return  XRAY_DEADPIXEL_RESULT* 坏点信息
 */
XRAY_DEADPIXEL_RESULT *xraw_dp_get_info(UINT32 chan)
{
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();

    if (chan < pCapbXrayin->xray_in_chan_cnt)
    {
        return &gstXrayTskCtrl[chan].dp_info;
    }
    else
    {
        XSP_LOGE("the chan(%u) is invalid, range: [0, %u)\n", chan, pCapbXrayin->xray_in_chan_cnt);
        return NULL;
    }
}


/**
 * @fn      xraw_dp_log_report
 * @brief   当有坏点信息有变化时，上报坏点信息给应用，用于日志记录
 * @param   chan[IN]
 * @param   p_dp_info[IN]
 * @return  SAL_STATUS
 */
static SAL_STATUS xraw_dp_log_report(UINT32 chan, XRAY_DEADPIXEL_RESULT *p_dp_info)
{
    STREAM_ELEMENT stream_elem = {0};

    XRAY_CHECK_PTR_IS_NULL(p_dp_info, SAL_FAIL);

    stream_elem.type = STREAM_ELEMENT_DEADPIXEL_XRAY;
    stream_elem.chan = chan;

    SystemPrm_cbFunProc(&stream_elem, (unsigned char *)p_dp_info, sizeof(XRAY_DEADPIXEL_RESULT));

    return SAL_SOK;
}


void Xray_ShowStatus(void)
{
    UINT32 i = 0;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;

    SAL_print("\n                           |         RT-preview         |          Playback          |            parsed count            |");
    SAL_print("\nXRAY share buffer >>> chan |   bufLen writeIdx  readIdx |   bufLen writeIdx  readIdx |    rtcnt    pbcnt     full     zero|\n");
    for (i = 0; i < pCapbXrayin->xray_in_chan_cnt; i++)
    {
        pstXrayCtrl = &gstXrayTskCtrl[i];
        SAL_print("               %11u | %8u %8u %8u | %8u %8u %8u | %8u %8u %8u %8u|\n", i, 
                  pstXrayCtrl->pstShareBufRt->bufLen, pstXrayCtrl->pstShareBufRt->writeIdx, pstXrayCtrl->pstShareBufRt->readIdx, 
                  pstXrayCtrl->pstShareBufPb->bufLen, pstXrayCtrl->pstShareBufPb->writeIdx, pstXrayCtrl->pstShareBufPb->readIdx,
                  pstXrayCtrl->stRtDebug.u32XrayRtProcCnt, pstXrayCtrl->stRtDebug.u32XrayPbProcCnt, 
                  pstXrayCtrl->stRtDebug.u32FullCorrCnt, pstXrayCtrl->stRtDebug.u32ZeroCorrCnt);
    }

    return;
}


void Xray_ShowXsensor(UINT32 chan)
{
    UINT32 i = 0, u32StartIdx = 0;
    XSENSOR_DEBUG *pstXsensor = NULL;
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    CHAR sPackSign[256] = {0};

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        SAL_print("the chan(%u) is invalid, should be < %u\n", chan, pCapbXrayin->xray_in_chan_cnt);
        return;
    }

    pstXrayCtrl = &gstXrayTskCtrl[chan];
    u32StartIdx = pstXrayCtrl->stRtDebug.u32XsensorStartIdx;

    SAL_print("\nXSensor > %d    ColNo Type   SyncTime PackageSign\n", chan);
    for (i = u32StartIdx; i < u32StartIdx+XRAY_XSENSOR_STAT_NUM; i++)
    {
        pstXsensor = &pstXrayCtrl->stRtDebug.stXsensor[i%XRAY_XSENSOR_STAT_NUM];
        memset(sPackSign, 0, sizeof(sPackSign));
        trans2bitstr(sPackSign, pstXsensor->au32PackSign, 128);
        SAL_print("%20u %4u %10llu %s\n", pstXsensor->u32Colno, pstXsensor->enProcType, 
                  pstXsensor->u64SyncTime, sPackSign);
    }

    return;
}


void Xray_ShowPbFrame(UINT32 chan)
{
    UINT32 i = 0;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    XRAY_DEBUG_PB_FARME *pstDbgPbFrame = NULL;

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        SAL_print("the chan(%u) is invalid, should be < %u\n", chan, pCapbXrayin->xray_in_chan_cnt);
        return;
    }

    SAL_print("\nPBFrame > %d  FrameNo Width Height  DataLen Dir Mode SNum Pnum OpDir NbTop NbBot\n", chan);
    for (i = gstXrayTskCtrl[chan].stPbDebug.u32FrameStartIdx; i < XRAY_DEBUG_PB_FARME_NUM; i++)
    {
        pstDbgPbFrame = &gstXrayTskCtrl[chan].stPbDebug.stPbFrame[i];
        SAL_print("%20u %5u %6u %8u %3d %4d %4u %4u %5d %5u %5u\n", pstDbgPbFrame->u32FrameNo, pstDbgPbFrame->u32Width, 
                  pstDbgPbFrame->u32Height, pstDbgPbFrame->u32DataLen, pstDbgPbFrame->direction, pstDbgPbFrame->uiWorkMode, 
                  pstDbgPbFrame->uiSliceNum, pstDbgPbFrame->uiPackageNum, pstDbgPbFrame->enPbOpDir, 
                  pstDbgPbFrame->u32PbNeighbourTop, pstDbgPbFrame->u32PbNeighbourBottom);
    }
    for (i = 0; i < gstXrayTskCtrl[chan].stPbDebug.u32FrameStartIdx; i++)
    {
        pstDbgPbFrame = &gstXrayTskCtrl[chan].stPbDebug.stPbFrame[i];
        SAL_print("%20u %5u %6u %8u %3d %4d %4u %4u %5d %5u %5u\n", pstDbgPbFrame->u32FrameNo, pstDbgPbFrame->u32Width, 
                  pstDbgPbFrame->u32Height, pstDbgPbFrame->u32DataLen, pstDbgPbFrame->direction, pstDbgPbFrame->uiWorkMode, 
                  pstDbgPbFrame->uiSliceNum, pstDbgPbFrame->uiPackageNum, pstDbgPbFrame->enPbOpDir, 
                  pstDbgPbFrame->u32PbNeighbourTop, pstDbgPbFrame->u32PbNeighbourBottom);
    }

    return;
}


void Xray_ShowPbSlice(UINT32 chan)
{
    UINT32 i = 0;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    XRAY_DEBUG_PB_SLICE *pstDbgPbSlice = NULL;

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        SAL_print("the chan(%u) is invalid, should be < %u\n", chan, pCapbXrayin->xray_in_chan_cnt);
        return;
    }

    SAL_print("\nPBSlice > %d Idx AssFrmNo HIn HTotal Cont Tag PRst               No\n", chan);
    for (i = gstXrayTskCtrl[chan].stPbDebug.u32SliceStartIdx; i < XRAY_DEBUG_PB_SLICE_NUM; i++)
    {
        pstDbgPbSlice = &gstXrayTskCtrl[chan].stPbDebug.stPbSlice[i];
        SAL_print("%15u %8u %3u %6u %4d %3d %4d %16llx\n", pstDbgPbSlice->u32SliceIdx, pstDbgPbSlice->u32AssociatedFrameNo, 
                  pstDbgPbSlice->u32HeightIn, pstDbgPbSlice->u32HeightTotal, pstDbgPbSlice->enSliceCont, pstDbgPbSlice->enPackTag,
                  pstDbgPbSlice->bAppendPackageResult, pstDbgPbSlice->u64ColNo);
    }
    for (i = 0; i < gstXrayTskCtrl[chan].stPbDebug.u32SliceStartIdx; i++)
    {
        pstDbgPbSlice = &gstXrayTskCtrl[chan].stPbDebug.stPbSlice[i];
        SAL_print("%15u %8u %3u %6u %4d %3d %4d %16llx\n", pstDbgPbSlice->u32SliceIdx, pstDbgPbSlice->u32AssociatedFrameNo, 
                  pstDbgPbSlice->u32HeightIn, pstDbgPbSlice->u32HeightTotal, pstDbgPbSlice->enSliceCont, pstDbgPbSlice->enPackTag,
                  pstDbgPbSlice->bAppendPackageResult, pstDbgPbSlice->u64ColNo);
    }

    return;
}


void Xray_ShowPbPack(UINT32 chan)
{
    UINT32 i = 0, j = 0;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    XRAY_DEBUG_PB_PACK_RST *pstDbgPackRst = NULL;

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        SAL_print("the chan(%u) is invalid, should be < %u\n", chan, pCapbXrayin->xray_in_chan_cnt);
        return;
    }

    SAL_print("\nPBPack > %d       Id       AssSliceNo PackW PackH    Start      End RtDir VFlip  Top Botm Z Target Type Conf Rect\n", chan);
    for (i = gstXrayTskCtrl[chan].stPbDebug.u32PackRstStartIdx; i < XRAY_DEBUG_PB_PACK_RST_NUM; i++)
    {
        pstDbgPackRst = &gstXrayTskCtrl[chan].stPbDebug.stPackRst[i];
        SAL_print("%19x %16llx %5u %5u %8u %8u %5d %5d %4u %4u %1u %6u\n", pstDbgPackRst->u32PackageId, 
                  pstDbgPackRst->u64AssociatedColNo, pstDbgPackRst->u32PackageWidth, pstDbgPackRst->u32PackageHeight, 
                  pstDbgPackRst->stProPrm.stTransferInfo.uiColStartNo, pstDbgPackRst->stProPrm.stTransferInfo.uiColEndNo, 
                  pstDbgPackRst->stProPrm.stTransferInfo.uiNoramlDirection, pstDbgPackRst->stProPrm.stTransferInfo.uiIsVerticalFlip,
                  pstDbgPackRst->stProPrm.stTransferInfo.uiPackTop, pstDbgPackRst->stProPrm.stTransferInfo.uiPackBottom,
                  pstDbgPackRst->stProPrm.stTransferInfo.uiZdataVersion, pstDbgPackRst->stSvaInfo.target_num);
        for (j = 0; j < pstDbgPackRst->stSvaInfo.target_num; j++)
        {
            SAL_print("%97d] %4u %4u [%f, %f] - [%f, %f]\n",
                      j, pstDbgPackRst->stSvaInfo.target[j].type, pstDbgPackRst->stSvaInfo.target[j].visual_confidence, 
                      pstDbgPackRst->stSvaInfo.target[j].rect.x, pstDbgPackRst->stSvaInfo.target[j].rect.y, 
                      pstDbgPackRst->stSvaInfo.target[j].rect.width, pstDbgPackRst->stSvaInfo.target[j].rect.height);
        }

        for (j = 0; j < pstDbgPackRst->stProPrm.stResultData.stZeffSent.uiNum; j++)
        {
            SAL_print("%97d] %4u %4u [%f, %f] - [%f, %f]\n",
                      j, ISM_ORGNAIC_TYPE, 0,
                      pstDbgPackRst->stProPrm.stResultData.stZeffSent.stRect[j].x, pstDbgPackRst->stProPrm.stResultData.stZeffSent.stRect[j].y, 
                      pstDbgPackRst->stProPrm.stResultData.stZeffSent.stRect[j].width,pstDbgPackRst->stProPrm.stResultData.stZeffSent.stRect[j].height);
        }

        for (j = 0; j < pstDbgPackRst->stProPrm.stResultData.stUnpenSent.uiNum; j++)
        {
            SAL_print("%97d] %4u %4u [%f, %f] - [%f, %f]\n",
                      j, ISM_UNPEN_TYPE, 0,
                      pstDbgPackRst->stProPrm.stResultData.stUnpenSent.stRect[j].x, pstDbgPackRst->stProPrm.stResultData.stUnpenSent.stRect[j].y, 
                      pstDbgPackRst->stProPrm.stResultData.stUnpenSent.stRect[j].width,pstDbgPackRst->stProPrm.stResultData.stUnpenSent.stRect[j].height);
        }
    }
    for (i = 0; i < gstXrayTskCtrl[chan].stPbDebug.u32PackRstStartIdx; i++)
    {
        pstDbgPackRst = &gstXrayTskCtrl[chan].stPbDebug.stPackRst[i];
        SAL_print("%19x %16llx %5u %5u %8u %8u %5d %5d %4u %4u %1u %6u\n", pstDbgPackRst->u32PackageId, 
                  pstDbgPackRst->u64AssociatedColNo, pstDbgPackRst->u32PackageWidth, pstDbgPackRst->u32PackageHeight, 
                  pstDbgPackRst->stProPrm.stTransferInfo.uiColStartNo, pstDbgPackRst->stProPrm.stTransferInfo.uiColEndNo, 
                  pstDbgPackRst->stProPrm.stTransferInfo.uiNoramlDirection, pstDbgPackRst->stProPrm.stTransferInfo.uiIsVerticalFlip,
                  pstDbgPackRst->stProPrm.stTransferInfo.uiPackTop, pstDbgPackRst->stProPrm.stTransferInfo.uiPackBottom,
                  pstDbgPackRst->stProPrm.stTransferInfo.uiZdataVersion, pstDbgPackRst->stSvaInfo.target_num);
        for (j = 0; j < pstDbgPackRst->stSvaInfo.target_num; j++)
        {
            SAL_print("%97d] %4u %4u [%f, %f] - [%f, %f]\n",
                      j, pstDbgPackRst->stSvaInfo.target[j].type, pstDbgPackRst->stSvaInfo.target[j].visual_confidence, 
                      pstDbgPackRst->stSvaInfo.target[j].rect.x, pstDbgPackRst->stSvaInfo.target[j].rect.y, 
                      pstDbgPackRst->stSvaInfo.target[j].rect.width, pstDbgPackRst->stSvaInfo.target[j].rect.height);
        }

        for (j = 0; j < pstDbgPackRst->stProPrm.stResultData.stZeffSent.uiNum; j++)
        {
            SAL_print("%97d] %4u %4u [%f, %f] - [%f, %f]\n",
                      j, ISM_ORGNAIC_TYPE, 0,
                      pstDbgPackRst->stProPrm.stResultData.stZeffSent.stRect[j].x, pstDbgPackRst->stProPrm.stResultData.stZeffSent.stRect[j].y, 
                      pstDbgPackRst->stProPrm.stResultData.stZeffSent.stRect[j].width,pstDbgPackRst->stProPrm.stResultData.stZeffSent.stRect[j].height);
        }

        for (j = 0; j < pstDbgPackRst->stProPrm.stResultData.stUnpenSent.uiNum; j++)
        {
            SAL_print("%97d] %4u %4u [%f, %f] - [%f, %f]\n",
                      j, ISM_UNPEN_TYPE, 0,
                      pstDbgPackRst->stProPrm.stResultData.stUnpenSent.stRect[j].x, pstDbgPackRst->stProPrm.stResultData.stUnpenSent.stRect[j].y, 
                      pstDbgPackRst->stProPrm.stResultData.stUnpenSent.stRect[j].width,pstDbgPackRst->stProPrm.stResultData.stUnpenSent.stRect[j].height);
        }

    }

    return;
}


static void xray_statistic(XRAY_TSK_CTRL *pstXrayCtrl, XRAY_ELEMENT *pstElemHdr)
{
    XSENSOR_DEBUG *pstXsensor = NULL;

    if (XRAY_TYPE_PLAYBACK != pstElemHdr->type)
    {
        pstXsensor = &pstXrayCtrl->stRtDebug.stXsensor[pstXrayCtrl->stRtDebug.u32XsensorStartIdx];
        pstXrayCtrl->stRtDebug.u32XsensorStartIdx = (pstXrayCtrl->stRtDebug.u32XsensorStartIdx + 1) % XRAY_XSENSOR_STAT_NUM;
        pstXsensor->u32Colno = pstElemHdr->xrayProcParam.stXrayPreview.columnNo;
        pstXsensor->u64SyncTime = pstElemHdr->time;
        pstXsensor->enProcType = pstElemHdr->type;
        memcpy(pstXsensor->au32PackSign, pstElemHdr->xrayProcParam.stXrayPreview.packageSign, sizeof(pstXsensor->au32PackSign));
    }

    switch (pstElemHdr->type)
    {
    case XRAY_TYPE_NORMAL:
    case XRAY_TYPE_PSEUDO_BLANK:
        pstXrayCtrl->stRtDebug.u32XrayRtProcCnt++;
        break;
    case XRAY_TYPE_PLAYBACK:
        pstXrayCtrl->stRtDebug.u32XrayPbProcCnt++;
        break;
    case XRAY_TYPE_CORREC_FULL:
        pstXrayCtrl->stRtDebug.u32FullCorrCnt++;
        break;
    case XRAY_TYPE_CORREC_ZERO:
        pstXrayCtrl->stRtDebug.u32ZeroCorrCnt++;
        break;
    default:
        break;
    }

    return;
}


void Xray_SetXrawMark(UINT32 chan, BOOL bEnable)
{
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        SAL_print("the chan(%u) is invalid, should be < %u\n", chan, pCapbXrayin->xray_in_chan_cnt);
        return;
    }

    gstXrayTskCtrl[chan].stRtDebug.bXrawMark = !!bEnable;

    return;
}


static void xraw_data_mark(XRAY_TSK_CTRL *pstXrayCtrl, XRAY_ELEMENT *pstElemHdr, XSP_DATA_NODE *pstDataNode)
{
    UINT32 i = 0, u32FillingEnd = 0;
    UINT16 *pu16FillingStart = NULL;
    XIMAGE_DATA_ST *pstXrawIn = NULL;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();

    XRAY_CHECK_PTR_NULL(pstXrayCtrl);
    XRAY_CHECK_PTR_NULL(pstElemHdr);
    XRAY_CHECK_PTR_NULL(pstDataNode);

    pstXrawIn = &pstDataNode->stXRawInBuf;

    if (pstXrayCtrl->stRtDebug.bXrawMark && XRAY_TYPE_NORMAL == pstDataNode->enProcType)
    {
        // 标记条带
        u32FillingEnd = pstXrawIn->u32Height / 4; // 显示纵向前1/4条带列数据填充标记色
        pu16FillingStart = (UINT16 *)pstXrawIn->pPlaneVir[0] + pstXrawIn->u32Width - 64;
        for (i = 0; i < u32FillingEnd; i++) // 低能
        {
            memset(pu16FillingStart, 0, 32); // 显示横向倒数第64行开始填充32/2行的标记色
            pu16FillingStart += pstXrawIn->u32Width;
        }
        if (XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num)
        {
            u32FillingEnd = pstXrawIn->u32Height * 5 / 4; // 显示纵向前1/4条带列数据填充标记色
            pu16FillingStart = (UINT16 *)pstXrawIn->pPlaneVir[0] + (pstXrawIn->u32Height+1) * pstXrawIn->u32Width - 64;
            for (i = pstXrawIn->u32Height; i < u32FillingEnd; i++) // 高能
            {
                memset(pu16FillingStart, 0, 32); // 显示横向倒数第64行开始填充32/2行的标记色
                pu16FillingStart += pstXrawIn->u32Width;
            }
        }

        // 标记条带是否为包裹
        if (0 != pstElemHdr->xrayProcParam.stXrayPreview.packageSign[0] ||
            0 != pstElemHdr->xrayProcParam.stXrayPreview.packageSign[1] ||
            0 != pstElemHdr->xrayProcParam.stXrayPreview.packageSign[2] ||
            0 != pstElemHdr->xrayProcParam.stXrayPreview.packageSign[3] ||
            0 != pstElemHdr->xrayProcParam.stXrayPreview.packageSign[4] ||
            0 != pstElemHdr->xrayProcParam.stXrayPreview.packageSign[5] ||
            0 != pstElemHdr->xrayProcParam.stXrayPreview.packageSign[6] ||
            0 != pstElemHdr->xrayProcParam.stXrayPreview.packageSign[7])
        {
            /* 显示纵向前2列数据填充标记色，显示横向正数第0行开始填充128/2行的标记色 */
            for (i = 0; i < 2; i++) // 低能
            {
                memset((UINT16 *)pstXrawIn->pPlaneVir[0] + i * pstXrawIn->u32Width, 0, 128);
            }
            if (XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num)
            {
                for (i = pstXrawIn->u32Height; i < pstXrawIn->u32Height+2; i++) // 高能
                {
                    memset((UINT16 *)pstXrawIn->pPlaneVir[0] + i * pstXrawIn->u32Width, 0, 128);
                }
            }
        }
    }

    return;
}


static void xraw_data_store(UINT32 chan, UINT32 u32DataSize, XSP_DATA_NODE *pstDataNode, DSA_SEG_INFO *pstElemDataSeg)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32WriteIdx = 0, u32XrawBufIdxPre = 0, u32Size = 0, u32EstimateLineNum = 0;
    UINT8 *pu8XrawDstAddr = NULL;
    XRAY_TSK_CTRL *pstXrayCtrl = &gstXrayTskCtrl[chan];
    STREAM_ELEMENT stStrmElem = {0};
    XRAY_RAW_DATA stXrawData = {0};
    CAPB_XSP *pstCapXsp = capb_get_xsp();

    if (pstXrayCtrl->bXrawStoreEn) // 保存原始数据
    {
        stStrmElem.type = STREAM_ELEMENT_XRAY_RAW;
        stStrmElem.chan = chan;

        if (XRAY_TYPE_NORMAL == pstDataNode->enProcType || XRAY_TYPE_PSEUDO_BLANK == pstDataNode->enProcType)
        {
            s32Ret = SAL_mutexTmLock(&pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].mutexCtrl, 1000, __FUNCTION__, __LINE__);
            if (SAL_SOK == s32Ret)
            {
                if (pstXrayCtrl->bSyncTimeMarkCb == SAL_FALSE)
                {
                    pstXrayCtrl->u64SyncTimeStart = pstDataNode->u64SyncTime; /* 记录第一个条带的时间 */
                    pstXrayCtrl->bSyncTimeMarkCb = SAL_TRUE;
                }
                u32WriteIdx = pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].u32WriteIdx;
                pstXrayCtrl->u32XrawLineNum += pstDataNode->stXRawInBuf.u32Height;

                // Buffer未满，继续写入
                if (u32WriteIdx + u32DataSize < pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].u32BufSize)
                {
                    pu8XrawDstAddr = pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].pVirtAddr + u32WriteIdx;
                    memcpy(pu8XrawDstAddr, pstElemDataSeg->p1, pstElemDataSeg->len1);
                    if (pstElemDataSeg->len2 > 0 && NULL != pstElemDataSeg->p2)
                    {
                        memcpy(pu8XrawDstAddr+pstElemDataSeg->len1, pstElemDataSeg->p2, pstElemDataSeg->len2);
                    }
                    pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].u32WriteIdx += u32DataSize;
                    SAL_mutexTmUnlock(&pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].mutexCtrl, __FUNCTION__, __LINE__);
                }
                else // Buffer已满，保存Ping Buffer，并交换Ping-Pong Buffer，并定期保存自动满载更新数据
                {
                    SAL_mutexTmUnlock(&pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].mutexCtrl, __FUNCTION__, __LINE__);

                    // 更换Pong Buffer
                    u32XrawBufIdxPre = pstXrayCtrl->u32XrawBufIdx; // 记录Ping Buffer
                    pstXrayCtrl->u32XrawBufIdx = (pstXrayCtrl->u32XrawBufIdx + 1) % 2;

                    s32Ret = SAL_mutexTmLock(&pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].mutexCtrl, 1000, __FUNCTION__, __LINE__);
                    if (SAL_SOK == s32Ret)
                    {
                        pu8XrawDstAddr = pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].pVirtAddr; // Buffer起始
                        memcpy(pu8XrawDstAddr, pstElemDataSeg->p1, pstElemDataSeg->len1);
                        if (pstElemDataSeg->len2 > 0 && NULL != pstElemDataSeg->p2)
                        {
                            memcpy(pu8XrawDstAddr+pstElemDataSeg->len1, pstElemDataSeg->p2, pstElemDataSeg->len2);
                        }
                        pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].u32WriteIdx += u32DataSize;

                        if(pstXrayCtrl->bSyncTimeMarkCb == SAL_TRUE)
                        {
                            pstXrayCtrl->u64SyncTimeEnd = pstDataNode->u64SyncTime; /* 记录回调前最后一个条带的时间 */
                            pstXrayCtrl->bSyncTimeMarkCb = SAL_FALSE;
                        }

                        // 回调保存XRAW数据，与交换Ping-Pong Buffer并行
                        stXrawData.enDataType = XRAW_DT_PACKAGE_SCANED;
                        stXrawData.pXrawBuf = pstXrayCtrl->stXrawBuf[u32XrawBufIdxPre].pVirtAddr;
                        stXrawData.uiXrawSize = u32WriteIdx;
                        stXrawData.syncTimeStart = pstXrayCtrl->u64SyncTimeStart;
                        stXrawData.syncTimeEnd = pstXrayCtrl->u64SyncTimeEnd;
                        SystemPrm_cbFunProc(&stStrmElem, (unsigned char *)&stXrawData, sizeof(XRAY_RAW_DATA));
                        pstXrayCtrl->stXrawBuf[u32XrawBufIdxPre].u32WriteIdx = 0; // 置0表示保存完成
                        SAL_mutexTmUnlock(&pstXrayCtrl->stXrawBuf[pstXrayCtrl->u32XrawBufIdx].mutexCtrl, __FUNCTION__, __LINE__);

                        // xraw文件大小100MB左右时，自动获取一次校正数据
                        u32EstimateLineNum = SEPARATE_XRAW_SIZE / (pstDataNode->stXRawInBuf.u32Width * pstCapXsp->xsp_original_raw_bw);
                        if (pstXrayCtrl->u32XrawLineNum >= u32EstimateLineNum)
                        {
                            pstXrayCtrl->u32XrawLineNum = 0;

                            // Ping Buffer作为存储缓存
                            pu8XrawDstAddr = pstXrayCtrl->stXcorrBuf[pstXrayCtrl->u32XcorrBufIdx].pVirtAddr; // Buffer起始
                            u32WriteIdx = 0;

                            // 本底分割线
                            memcpy(pu8XrawDstAddr, pstXrayCtrl->stXcorrZeroSplit.pVirtAddr, pstXrayCtrl->stXcorrZeroSplit.u32BufSize);
                            u32WriteIdx += pstXrayCtrl->stXcorrZeroSplit.u32BufSize;

                            // 本底校正数据
                            u32Size = pstXrayCtrl->stXcorrBuf[pstXrayCtrl->u32XcorrBufIdx].u32BufSize - u32WriteIdx;
                            if (SAL_SOK == xsp_get_corr_zero(chan, pu8XrawDstAddr+u32WriteIdx, &u32Size))
                            {
                                u32WriteIdx += u32Size;
                            }
                            else
                            {
                                XSP_LOGW("xsp_get_corr_zero failed, chan: %u, u32Size: %u\n", chan, u32Size);
                            }

                            // 满载分割线
                            memcpy(pu8XrawDstAddr+u32WriteIdx, pstXrayCtrl->stXcorrFullSplit.pVirtAddr, pstXrayCtrl->stXcorrFullSplit.u32BufSize);
                            u32WriteIdx += pstXrayCtrl->stXcorrFullSplit.u32BufSize;

                            // 满载校正数据
                            u32Size = pstXrayCtrl->stXcorrBuf[pstXrayCtrl->u32XcorrBufIdx].u32BufSize - u32WriteIdx;
                            if (SAL_SOK == xsp_get_corr_full(chan, pu8XrawDstAddr+u32WriteIdx, &u32Size))
                            {
                                u32WriteIdx += u32Size;
                            }
                            else
                            {
                                XSP_LOGW("xsp_get_corr_full failed, chan: %u, u32Size: %u\n", chan, u32Size);
                            }

                            // 回调保存校正数据
                            stXrawData.enDataType = XRAW_DT_AUTO_UPDATE;
                            stXrawData.pXrawBuf = pu8XrawDstAddr;
                            stXrawData.uiXrawSize = u32WriteIdx;
                            //XSP_LOGI("XRAW UPDATE STREAM_ELEMENT_XRAY_RAW : chan: %d, enDataType %d.\n", chan, stXrawData.enDataType);
                            SystemPrm_cbFunProc(&stStrmElem, (unsigned char *)&stXrawData, sizeof(XRAY_RAW_DATA));

                            // 交换Ping-Pong buffer
                            pstXrayCtrl->u32XcorrBufIdx = (pstXrayCtrl->u32XcorrBufIdx + 1) % 2;
                        }
                    }
                    else
                    {
                        XSP_LOGE("SAL_mutexTmLock 'XrawBuf' failed, chan: %u, u32XrawBufIdx: %u\n", chan, pstXrayCtrl->u32XrawBufIdx);
                    }
                }
            }
            else
            {
                XSP_LOGE("SAL_mutexTmLock 'XrawBuf' failed, chan: %u, u32XrawBufIdx: %u\n", chan, pstXrayCtrl->u32XrawBufIdx);
            }
        }
        else if (XRAY_TYPE_CORREC_FULL == pstDataNode->enProcType) // 手动校正时，有部分延时可以接受的，直接用输入Buffer保存
        {
            if (1 == pstXrayCtrl->stRtDebug.u32FullCorrCnt)
            {
                stXrawData.enDataType = XRAW_DT_BOOT_CORR;
            }
            else
            {
                stXrawData.enDataType = XRAW_DT_MANUAL_CORR;
            }

            pstXrayCtrl->u32XrawLineNum = 0;
            // 满载分割线
            stXrawData.pXrawBuf = pstXrayCtrl->stXcorrFullSplit.pVirtAddr;
            stXrawData.uiXrawSize = pstXrayCtrl->stXcorrFullSplit.u32BufSize;
            SystemPrm_cbFunProc(&stStrmElem, (unsigned char *)&stXrawData, sizeof(XRAY_RAW_DATA));

            // 满载校正数据
            stXrawData.pXrawBuf = pstElemDataSeg->p1;
            stXrawData.uiXrawSize = pstElemDataSeg->len1;
            SystemPrm_cbFunProc(&stStrmElem, (unsigned char *)&stXrawData, sizeof(XRAY_RAW_DATA));
            if (pstElemDataSeg->len2 > 0 && NULL != pstElemDataSeg->p2)
            {
                stXrawData.pXrawBuf = pstElemDataSeg->p2;
                stXrawData.uiXrawSize = pstElemDataSeg->len2;
                SystemPrm_cbFunProc(&stStrmElem, (unsigned char *)&stXrawData, sizeof(XRAY_RAW_DATA));
            }
        }
        else if (XRAY_TYPE_CORREC_ZERO == pstDataNode->enProcType) // 手动校正时，有部分延时可以接受的，直接用输入Buffer保存
        {
            if (1 == pstXrayCtrl->stRtDebug.u32ZeroCorrCnt)
            {
                stXrawData.enDataType = XRAW_DT_BOOT_CORR;
            }
            else
            {
                stXrawData.enDataType = XRAW_DT_MANUAL_CORR;
            }

            pstXrayCtrl->u32XrawLineNum = 0;
            // 本底分割线
            stXrawData.pXrawBuf = pstXrayCtrl->stXcorrZeroSplit.pVirtAddr;
            stXrawData.uiXrawSize = pstXrayCtrl->stXcorrZeroSplit.u32BufSize;
            SystemPrm_cbFunProc(&stStrmElem, (unsigned char *)&stXrawData, sizeof(XRAY_RAW_DATA));

            // 本底校正数据
            stXrawData.pXrawBuf = pstElemDataSeg->p1;
            stXrawData.uiXrawSize = pstElemDataSeg->len1;
            SystemPrm_cbFunProc(&stStrmElem, (unsigned char *)&stXrawData, sizeof(XRAY_RAW_DATA));
            if (pstElemDataSeg->len2 > 0 && NULL != pstElemDataSeg->p2)
            {
                stXrawData.pXrawBuf = pstElemDataSeg->p2;
                stXrawData.uiXrawSize = pstElemDataSeg->len2;
                SystemPrm_cbFunProc(&stStrmElem, (unsigned char *)&stXrawData, sizeof(XRAY_RAW_DATA));
            }
        }
        else if (XRAY_TYPE_AUTO_CORR_FULL == pstDataNode->enProcType)
        {
            pstXrayCtrl->u32XrawLineNum = 0;
            // Ping Buffer作为存储缓存
            pu8XrawDstAddr = pstXrayCtrl->stXcorrBuf[pstXrayCtrl->u32XcorrBufIdx].pVirtAddr; // Buffer起始
            u32WriteIdx = 0;

            // 本底分割线
            memcpy(pu8XrawDstAddr, pstXrayCtrl->stXcorrZeroSplit.pVirtAddr, pstXrayCtrl->stXcorrZeroSplit.u32BufSize);
            u32WriteIdx += pstXrayCtrl->stXcorrZeroSplit.u32BufSize;

            // 本底校正数据
            u32Size = pstXrayCtrl->stXcorrBuf[pstXrayCtrl->u32XcorrBufIdx].u32BufSize - u32WriteIdx;
            if (SAL_SOK == xsp_get_corr_zero(chan, pu8XrawDstAddr+u32WriteIdx, &u32Size))
            {
                u32WriteIdx += u32Size;
            }
            else
            {
                XSP_LOGW("xsp_get_corr_zero failed, chan: %u, u32Size: %u\n", chan, u32Size);
            }

            // 满载分割线
            memcpy(pu8XrawDstAddr+u32WriteIdx, pstXrayCtrl->stXcorrFullSplit.pVirtAddr, pstXrayCtrl->stXcorrFullSplit.u32BufSize);
            u32WriteIdx += pstXrayCtrl->stXcorrFullSplit.u32BufSize;

            // 满载校正数据
            u32Size = pstXrayCtrl->stXcorrBuf[pstXrayCtrl->u32XcorrBufIdx].u32BufSize - u32WriteIdx;
            u32Size = SAL_MIN(u32DataSize, u32Size);
            memcpy(pu8XrawDstAddr+u32WriteIdx, pstElemDataSeg->p1, pstElemDataSeg->len1);
            if (pstElemDataSeg->len2 > 0 && NULL != pstElemDataSeg->p2)
            {
                memcpy(pu8XrawDstAddr+u32WriteIdx+pstElemDataSeg->len1, pstElemDataSeg->p2, pstElemDataSeg->len2);
            }
            u32WriteIdx += u32Size;

            // 回调保存校正数据
            stXrawData.enDataType = XRAW_DT_AUTO_CORR;
            stXrawData.pXrawBuf = pu8XrawDstAddr;
            stXrawData.uiXrawSize = u32WriteIdx;
            SystemPrm_cbFunProc(&stStrmElem, (unsigned char *)&stXrawData, sizeof(XRAY_RAW_DATA));

            // 交换Ping-Pong buffer
            pstXrayCtrl->u32XcorrBufIdx = (pstXrayCtrl->u32XcorrBufIdx + 1) % 2;
        }
    }

    return;
}


/**
 * @fn      xraw_interleave_2_planar
 * @brief   XRAW数据交叉存储格式转平面存储格式
 * @warning 仅对双能有效
 * @param   pu16OutBuf[OUT] 输出的平面存储格式Buffer
 * @param   pu16INBuf[IN] 输入的交叉存储格式Buffer
 * @param   w[IN] RAW数据宽
 * @param   h[IN] RAW数据高
 */
static void xraw_interleave_2_planar(UINT16 *pu16OutBuf, UINT16 *pu16INBuf, UINT32 w, UINT32 h)
{
    UINT32 i = 0, line_size = w * sizeof(U16);
    UINT16 *p_line_in = pu16INBuf;
    UINT16 *p_line_out_le = pu16OutBuf; /* 低能 */
    UINT16 *p_line_out_he = pu16OutBuf + w * h; /* 高能 */

    /*采集发来的数据一行低能，一行高能，交叉存储，内部拆分成平面存储，先存低能，后存高能*/
    for (i = 0; i < h; i++)
    {
        /* 低能数据 */
        memcpy(p_line_out_le, p_line_in, line_size);
        p_line_in += w; /* 源数据偏移一行 */
        p_line_out_le += w; /* 目的低能数据偏移一行 */

        /* 高能数据 */
        memcpy(p_line_out_he, p_line_in, line_size);
        p_line_in += w; /* 源数据偏移一行 */
        p_line_out_he += w; /* 目的高能数据偏移一行 */
    }

    return;
}

static XRAY_ELEMENT *search_element_header(UINT32 chan, XRAY_SHARE_BUF *pstShareBuf, UINT32 *pu32SearchIdx)
{
    UINT32 u32ReadIdx = 0, u32WriteIdx = 0;     /* Share Buffer中的读写索引 */
    UINT32 u32SearchIdx = 0;                    /* 搜索Element Header的索引 */
    BOOL bElemHdrFound = SAL_FALSE;             /* 是否找到Element Header */
    UINT32 u32DataLen = 0;                      /* 缓冲区中有效数据长度 */
    UINT32 u32ShareBufLen = 0;                  /* 共享缓冲区大小 */
    PUINT8 pu8ShareBuf = NULL;
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;

    /* 本地参数初始化 */
    pstXrayCtrl = &gstXrayTskCtrl[chan];

    /* 计算当前共享缓冲区中数据量 */
    pu8ShareBuf = pstShareBuf->pVirtAddr;
    u32ShareBufLen = pstShareBuf->bufLen;
    u32ReadIdx = pstShareBuf->readIdx;
    u32WriteIdx = pstShareBuf->writeIdx;
    u32DataLen = DIST(u32WriteIdx, u32ReadIdx, u32ShareBufLen);

    /* 数据量过小直接返回 */
    if (u32DataLen < sizeof(XRAY_ELEMENT))
    {
        *pu32SearchIdx = u32ReadIdx;
        return NULL;
    }

    if (u32WriteIdx > u32ReadIdx) // share buffer中数据未回头
    {
        for (u32SearchIdx = u32ReadIdx; u32SearchIdx < u32WriteIdx; u32SearchIdx += 4) // 以4个字节为单位往后检索
        {
            if (XRAY_ELEMENT_MAGIC == *(UINT32 *)&pu8ShareBuf[u32SearchIdx]) // bingo
            {
                *pu32SearchIdx = u32SearchIdx;
                return (XRAY_ELEMENT *)&pu8ShareBuf[u32SearchIdx];
            }
        }
    }
    else // if (u32ReadIdx > u32WriteIdx) // share buffer中数据有回头
    {
        // 先查找从u32ReadIdx到share buffer结尾
        for (u32SearchIdx = u32ReadIdx; u32SearchIdx < u32ShareBufLen; u32SearchIdx += 4) // 以4个字节为单位往后检索
        {
            if (XRAY_ELEMENT_MAGIC == *(UINT32 *)&pu8ShareBuf[u32SearchIdx]) // bingo
            {
                bElemHdrFound = SAL_TRUE;
                break;
            }
        }

        // 若上一步骤未找到，则从头开始继续查找到u32WriteIdx
        if (!bElemHdrFound)
        {
            for (u32SearchIdx = 0; u32SearchIdx < u32WriteIdx; u32SearchIdx += 4) // 以4个字节为单位往后检索
            {
                if (XRAY_ELEMENT_MAGIC == *(UINT32 *)&pu8ShareBuf[u32SearchIdx]) // bingo
                {
                    bElemHdrFound = SAL_TRUE;
                    break;
                }
            }
        }

        if (bElemHdrFound)
        {
            *pu32SearchIdx = u32SearchIdx;
            u32DataLen = u32ShareBufLen - u32ReadIdx;
            if (u32DataLen < sizeof(XRAY_ELEMENT)) // 若Element Header有回头，则将其拷贝到pu8ElemHdrBuf中返回连续数据
            {
                memcpy(pstXrayCtrl->pstElemHdrBuf, pu8ShareBuf+u32ReadIdx, u32DataLen);
                memcpy((UINT8 *)pstXrayCtrl->pstElemHdrBuf+u32DataLen, pu8ShareBuf, sizeof(XRAY_ELEMENT)-u32DataLen);
                return pstXrayCtrl->pstElemHdrBuf;
            }
            else
            {
                return (XRAY_ELEMENT *)&pu8ShareBuf[u32SearchIdx];
            }
        }
    }

    // 未找到Element Header，则下次直接从写索引开始查找
    *pu32SearchIdx = u32WriteIdx;

    return NULL;
}


static XRAY_PB_SLICE_HEADER *get_pb_header(UINT32 chan, UINT8 *pu8Buf, UINT32 *pu32ReadIdx, UINT32 u32BufLen)
{
    UINT32 u32DataLen = 0;
    UINT32 u32ReadIdx = *pu32ReadIdx;
    XRAY_TSK_CTRL *pstXrayCtrl = &gstXrayTskCtrl[chan];

    if (u32ReadIdx + sizeof(XRAY_PB_SLICE_HEADER) > u32BufLen) // XRAY_PB_SLICE_HEADER回头
    {
        u32DataLen = u32BufLen - u32ReadIdx;
        memcpy(pstXrayCtrl->pstPbHdr, pu8Buf+u32ReadIdx, u32DataLen);
        memcpy((UINT8 *)pstXrayCtrl->pstPbHdr+u32DataLen, pu8Buf, sizeof(XRAY_PB_SLICE_HEADER)-u32DataLen);
        *pu32ReadIdx = sizeof(XRAY_PB_SLICE_HEADER) - u32DataLen;
        return pstXrayCtrl->pstPbHdr;
    }
    else
    {
        *pu32ReadIdx = u32ReadIdx + sizeof(XRAY_PB_SLICE_HEADER);
        return (XRAY_PB_SLICE_HEADER *)(pu8Buf + u32ReadIdx);
    }
}


static XRAY_PB_PACKAGE_RESULT *get_pb_pack_rst(UINT32 chan, UINT8 *pu8Buf, UINT32 *pu32ReadIdx, UINT32 u32BufLen)
{
    UINT32 u32DataLen = 0;
    UINT32 u32ReadIdx = *pu32ReadIdx;
    XRAY_TSK_CTRL *pstXrayCtrl = &gstXrayTskCtrl[chan];

    if (u32ReadIdx + sizeof(XRAY_PB_PACKAGE_RESULT) > u32BufLen) // XRAY_PB_PACKAGE_RESULT回头
    {
        u32DataLen = u32BufLen - u32ReadIdx;
        memcpy(pstXrayCtrl->pstPackRst, pu8Buf+u32ReadIdx, u32DataLen);
        memcpy((UINT8 *)pstXrayCtrl->pstPackRst+u32DataLen, pu8Buf, sizeof(XRAY_PB_PACKAGE_RESULT)-u32DataLen);
        *pu32ReadIdx = sizeof(XRAY_PB_PACKAGE_RESULT) - u32DataLen;
        return pstXrayCtrl->pstPackRst;
    }
    else
    {
        *pu32ReadIdx = u32ReadIdx + sizeof(XRAY_PB_PACKAGE_RESULT);
        return (XRAY_PB_PACKAGE_RESULT *)(pu8Buf + u32ReadIdx);
    }
}


static UINT8 *read_data_from_ringbuf(UINT8 *pu8Dst, UINT8 *pu8SrcBuf, const UINT32 u32SrcBufLen, UINT32 *pu32ReadIdx, const UINT32 u32ReadSize)
{
    UINT32 u32ReadIdx = 0, u32DataLen = 0;

    if (NULL == pu32ReadIdx || NULL == pu8Dst || NULL == pu8SrcBuf)
    {
        XSP_LOGE("the pu32ReadIdx[%p] OR pu8Dst[%p] OR pu8SrcBuf[%p] is NULL\n", pu32ReadIdx, pu8Dst, pu8SrcBuf);
        return NULL;
    }

    u32ReadIdx = *pu32ReadIdx;

    if (u32ReadIdx + u32ReadSize > u32SrcBufLen) // 数据段在share buffer中有回头
    {
        u32DataLen = u32SrcBufLen - u32ReadIdx;
        memcpy(pu8Dst, pu8SrcBuf + u32ReadIdx, u32DataLen);
        memcpy(pu8Dst + u32DataLen, pu8SrcBuf, u32ReadSize - u32DataLen);
        *pu32ReadIdx = u32ReadSize - u32DataLen; // 读索引偏移
    }
    else
    {
        memcpy(pu8Dst, pu8SrcBuf + u32ReadIdx, u32ReadSize);
        *pu32ReadIdx = u32ReadIdx + u32ReadSize; // 读索引偏移
    }

    return (pu8Dst+u32ReadSize);
}


static void patch_pb_neighbour(XSP_DATA_NODE *pstDataNode, BOOL bNeighBourTop, UINT8 *pu8LeStart, UINT8 *pu8HeStart, UINT8 *pu8ZStart)
{
    UINT32 i = 0, u32Height = 0;
    UINT32 u32NrawWidth = 0, u32NrawHeight = 0;
    UINT32 u32LeLineSize = 0, u32HeLineSize = 0, u32ZLineSize = 0;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    UINT8 *pu8LeSrc = NULL, *pu8HeSrc = NULL, *pu8ZSrc = NULL;
    UINT8 *pu8LeDest = NULL, *pu8HeDest = NULL, *pu8ZDest = NULL;

    if (NULL == pstDataNode || NULL == pu8LeStart)
    {
        XSP_LOGE("pstDataNode[%p] OR pu8LeStart[%p] is NULL\n", pstDataNode, pu8LeStart);
        return;
    }
    if (XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num && (NULL == pu8HeStart || NULL == pu8ZStart))
    {
        XSP_LOGE("dual energy, but pu8HeStart[%p] OR pu8ZStart[%p] is NULL\n", pu8HeStart, pu8ZStart);
        return;
    }

    u32NrawWidth = pstDataNode->stNRawInBuf.u32Width;
    u32NrawHeight = pstDataNode->stNRawInBuf.u32Height - XSP_NEIGHBOUR_H_MAX * 2;
    u32LeLineSize = XRAW_LE_SIZE(u32NrawWidth, 1);
    u32HeLineSize = XRAW_HE_SIZE(u32NrawWidth, 1);
    u32ZLineSize = XRAW_Z_SIZE(u32NrawWidth, 1);

    if (bNeighBourTop) /* 上邻域 */
    {
        /**
         * 上邻域为空的情况，复制包裹数据的边缘一列
         * 上邻域不为空，但数据量小于最小限制，则先将数据往下移到与包裹数据相邻，然后开头空白区域复制边缘一列 
         */
        if (pstDataNode->u32PbNeighbourTop > 0 && pstDataNode->u32PbNeighbourTop < XSP_NEIGHBOUR_H_MIN)
        {
            /* 先将数据往下移到与包裹数据相邻 */
            pu8LeDest = pu8LeStart + XRAW_LE_SIZE(u32NrawWidth, XSP_NEIGHBOUR_H_MIN-pstDataNode->u32PbNeighbourTop);
            memmove(pu8LeDest, pu8LeStart, XRAW_LE_SIZE(u32NrawWidth, pstDataNode->u32PbNeighbourTop));
            if (XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num)
            {
                pu8HeDest = pu8HeStart + XRAW_HE_SIZE(u32NrawWidth, XSP_NEIGHBOUR_H_MIN-pstDataNode->u32PbNeighbourTop);
                memmove(pu8HeDest, pu8HeStart, XRAW_HE_SIZE(u32NrawWidth, pstDataNode->u32PbNeighbourTop));

                pu8ZDest = pu8ZStart + XRAW_Z_SIZE(u32NrawWidth, XSP_NEIGHBOUR_H_MIN-pstDataNode->u32PbNeighbourTop);
                memmove(pu8ZDest, pu8ZStart, XRAW_Z_SIZE(u32NrawWidth, pstDataNode->u32PbNeighbourTop));
            }

            /* 然后开头空白区域复制边缘一列 */
            pu8LeSrc = pu8LeDest;
            pu8LeDest = pu8LeStart;
            if (XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num)
            {
                pu8HeSrc = pu8HeDest;
                pu8HeDest = pu8HeStart;
                pu8ZSrc = pu8ZDest;
                pu8ZDest = pu8ZStart;
            }
            for (i = 0; i < XSP_NEIGHBOUR_H_MIN-pstDataNode->u32PbNeighbourTop; i++)
            {
                memcpy(pu8LeDest, pu8LeSrc, u32LeLineSize);
                pu8LeDest += u32LeLineSize;
                if (XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num)
                {
                    memcpy(pu8HeDest, pu8HeSrc, u32HeLineSize);
                    pu8HeDest += u32HeLineSize;
                    memcpy(pu8ZDest, pu8ZSrc, u32ZLineSize);
                    pu8ZDest += u32ZLineSize;
                }
            }

            pstDataNode->u32PbNeighbourTop = XSP_NEIGHBOUR_H_MIN;
        }
        else if (0 == pstDataNode->u32PbNeighbourTop)
        {
            // 预留的邻域高度为XSP_NEIGHBOUR_H_MIN
            pu8LeSrc = pu8LeStart + XRAW_LE_SIZE(u32NrawWidth, XSP_NEIGHBOUR_H_MIN);
            pu8LeDest = pu8LeStart;
            if (XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num)
            {
                pu8HeSrc = pu8HeStart + XRAW_HE_SIZE(u32NrawWidth, XSP_NEIGHBOUR_H_MIN);
                pu8HeDest = pu8HeStart;
                pu8ZSrc = pu8ZStart + XRAW_Z_SIZE(u32NrawWidth, XSP_NEIGHBOUR_H_MIN);
                pu8ZDest = pu8ZStart;
            }
            for (i = 0; i < XSP_NEIGHBOUR_H_MIN; i++)
            {
                memcpy(pu8LeDest, pu8LeSrc, u32LeLineSize);
                pu8LeDest += u32LeLineSize;
                if (XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num)
                {
                    memcpy(pu8HeDest, pu8HeSrc, u32HeLineSize);
                    pu8HeDest += u32HeLineSize;
                    memcpy(pu8ZDest, pu8ZSrc, u32ZLineSize);
                    pu8ZDest += u32ZLineSize;
                }
            }

            pstDataNode->u32PbNeighbourTop = XSP_NEIGHBOUR_H_MIN;
        }
    }
    else /* 下邻域 */
    {
        /**
         * 下邻数据量小于最小限制，复制包裹数据或邻域数据的边缘一列
         */
        if (pstDataNode->u32PbNeighbourBottom < XSP_NEIGHBOUR_H_MIN)
        {
            u32Height = pstDataNode->u32PbNeighbourTop + u32NrawHeight + pstDataNode->u32PbNeighbourBottom;
            pu8LeSrc = pu8LeStart + XRAW_LE_SIZE(u32NrawWidth, u32Height - 1);
            pu8LeDest = pu8LeStart + XRAW_LE_SIZE(u32NrawWidth, u32Height);
            if (XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num)
            {
                pu8HeSrc = pu8HeStart + XRAW_HE_SIZE(u32NrawWidth, u32Height - 1);
                pu8HeDest = pu8HeStart + XRAW_HE_SIZE(u32NrawWidth, u32Height);
                pu8ZSrc = pu8ZStart + XRAW_Z_SIZE(u32NrawWidth, u32Height - 1);
                pu8ZDest = pu8ZStart + XRAW_Z_SIZE(u32NrawWidth, u32Height);
            }
            for (i = 0; i < XSP_NEIGHBOUR_H_MIN-pstDataNode->u32PbNeighbourBottom; i++)
            {
                memcpy(pu8LeDest, pu8LeSrc, u32LeLineSize);
                pu8LeDest += u32LeLineSize;
                if (XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num)
                {
                    memcpy(pu8HeDest, pu8HeSrc, u32HeLineSize);
                    pu8HeDest += u32HeLineSize;
                    memcpy(pu8ZDest, pu8ZSrc, u32ZLineSize);
                    pu8ZDest += u32ZLineSize;
                }
            }

            pstDataNode->u32PbNeighbourBottom = XSP_NEIGHBOUR_H_MIN;
        }
    }

    return;
}

/******************************************************************
 * @function:   parse_pb_slice
 * @brief:      解析回拉数据信息
 * @param[in]:  UINT32          chan
 * @param[in]:  XRAY_SHARE_BUF  *pstShareBuf
 * @param[in]:  XRAY_ELEMENT    *pstElemHdr
 * @param[in]:  UINT32          u32ReadIdxData
 * @param[out]: XSP_DATA_NODE   *pstDataNode
 * @return:     SAL_STATUS
 *******************************************************************/
static SAL_STATUS parse_pb_slice(UINT32 chan, XRAY_SHARE_BUF *pstShareBuf, XRAY_ELEMENT *pstElemHdr, UINT32 u32ReadIdxData, XSP_DATA_NODE *pstDataNode)
{
    UINT32 i = 0, j = 0, k = 0;
    UINT32 u32PackCnt = 0, u32PackId = 0, u32PackCol = 0;
    UINT64 u64SliceNo = 0; /* 用于校验条带编号是正序还是逆序 */
    XSP_PACKAGE_TAG enSliceTag = XSP_PACKAGE_TAG_NUM; /* 用于校验相邻条带的包裹分割标志是否正常 */
    BOOL bDispSliceRd = SAL_FALSE; // 是否已读取到显示条带数据（包括包裹和空白数据）
    UINT8 *pu8LeStart = NULL, *pu8HeStart = NULL, *pu8ZStart = NULL;
    UINT8 *pu8LeReal = NULL, *pu8HeReal = NULL, *pu8ZReal = NULL;
    UINT32 u32LeSize = 0, u32HeSize = 0, u32ZSize = 0;
    UINT32 u32ReadIdxPb = u32ReadIdxData;
    UINT32 u32ShareBufLen = pstShareBuf->bufLen; /* 数据共享缓冲区大小 */
    UINT8 *pu8ShareBuf = pstShareBuf->pVirtAddr; /* 共享缓冲区 */

    DSP_IMG_DATFMT enNrawFmt = DSP_IMG_DATFMT_UNKNOWN;
    XIMAGE_DATA_ST *pstNodeNraw = NULL;
    XIMAGE_DATA_ST *pstNodeXraw = NULL;
    XRAY_PB_SLICE_HEADER *pstPbHdr = NULL;
    XRAY_PB_PACKAGE_RESULT *pstPackRst = NULL;
    CAPB_XRAY_IN *pstCapbXrayin = capb_get_xrayin();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    XSP_COMMON *pstXspCommon = Xsp_DrvGetCommon();
    XSP_CHN_PRM *pstXspChnPrm = &pstXspCommon->stXspChn[chan];
    XRAY_DEBUG_PB_SLICE *pstDbgPbSlice = NULL;
    XRAY_DEBUG_PB_PACK_RST *pstDbgPackRst = NULL;

    XRAY_CHECK_PTR_IS_NULL(pstShareBuf, SAL_FAIL);
    XRAY_CHECK_PTR_IS_NULL(pstElemHdr, SAL_FAIL);
    XRAY_CHECK_PTR_IS_NULL(pstDataNode, SAL_FAIL);

    pstNodeXraw = &pstDataNode->stXRawInBuf;
    pstNodeNraw = &pstDataNode->stNRawInBuf;
    memset(pstNodeNraw, 0, sizeof(XIMAGE_DATA_ST));
    /* 使用节点中stXRawInBuf的内存重新创建一个高低能类型图像 */
    enNrawFmt = XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num ? DSP_IMG_DATFMT_LHZP : DSP_IMG_DATFMT_SP16;
    ximg_create(pstElemHdr->width, pstElemHdr->height + XSP_NEIGHBOUR_H_MAX * 2, enNrawFmt, NULL, pstNodeXraw->pPlaneVir[0], pstNodeNraw);

    pstDataNode->enDispDir = pstElemHdr->xrayProcParam.stXrayPlayback.direction; /* 回拉显示方向 */
    pstDataNode->enPbMode = pstElemHdr->xrayProcParam.stXrayPlayback.uiWorkMode; /* 整包回拉OR条带回拉 */
    pstDataNode->u32PbPackNum = pstElemHdr->xrayProcParam.stXrayPlayback.uiPackageNum;

    if (XRAY_PB_SLICE == pstDataNode->enPbMode) // 条带回拉，校验显示方向；整包回拉，后面流程会强制显示方向为过包时方向
    {
        if (XRAY_DIRECTION_LEFT != pstDataNode->enDispDir && XRAY_DIRECTION_RIGHT != pstDataNode->enDispDir)
        {
            XSP_LOGE("chan %u, the enDispDir[%d] is invalid in slice playback\n", chan, pstDataNode->enDispDir);
            return SAL_FAIL;
        }
    }

    // TODO: 小于XSP成像处理限制的条带，需要填充白数据
    pu8LeReal = pu8LeStart = pstNodeNraw->pPlaneVir[0];
    if (DSP_IMG_DATFMT_LHZP == pstNodeNraw->enImgFmt)
    {
        pu8HeReal = pu8HeStart = pstNodeNraw->pPlaneVir[1];
        pu8ZReal = pu8ZStart = pstNodeNraw->pPlaneVir[2];
    }

    if (pstDataNode->enDispDir == pstXspChnPrm->stNormalData.uiRtDirection) // 与实时过包方向相同
    {
        pstDataNode->enPbOpDir = XSP_PB_OPDIR_POSITIVE;
    }
    else // 与实时过包方向相反
    {
        pstDataNode->enPbOpDir = XSP_PB_OPDIR_OPPOSITE;
    }

    for (i = 0; i < pstElemHdr->xrayProcParam.stXrayPlayback.uiSliceNum; i++)
    {
        /* 获取回拉头信息 */
        pstPbHdr = get_pb_header(chan, pu8ShareBuf, &u32ReadIdxPb, u32ShareBufLen);
        pstDbgPbSlice = &gstXrayTskCtrl[chan].stPbDebug.stPbSlice[gstXrayTskCtrl[chan].stPbDebug.u32SliceStartIdx];
        memcpy(pstDbgPbSlice, pstPbHdr, sizeof(XRAY_PB_SLICE_HEADER));
        pstDbgPbSlice->u32SliceIdx = i;
        j = (gstXrayTskCtrl[chan].stPbDebug.u32FrameStartIdx - 1) % XRAY_DEBUG_PB_FARME_NUM;
        pstDbgPbSlice->u32AssociatedFrameNo = gstXrayTskCtrl[chan].stPbDebug.stPbFrame[j].u32FrameNo;
        gstXrayTskCtrl[chan].stPbDebug.u32SliceStartIdx = (gstXrayTskCtrl[chan].stPbDebug.u32SliceStartIdx + 1) % XRAY_DEBUG_PB_SLICE_NUM;

        if (i == 0)
        {
            u64SliceNo = pstPbHdr->u64ColNo;
            if (XSP_SLICE_NEIGHBOUR != pstPbHdr->enSliceCont) // 邻域条带不判断包裹标记
            {
                enSliceTag = pstPbHdr->enPackTag;
            }
        }
        else
        {
            /* 校验条带序号排列 */
            if (pstPbHdr->u64ColNo > u64SliceNo) // 条带序号递增，与实时过包方向相同，实时过包的START即START，END即END
            {
                if (XSP_PB_OPDIR_POSITIVE != pstDataNode->enPbOpDir)
                {
                    XSP_LOGE("chan %u, pre: %llu, cur: %u - %llu, RtDir: %d, PbDir: %d, PbOpDir: %d, slices in this frame are out of order\n", 
                             chan, u64SliceNo, i, pstPbHdr->u64ColNo, pstXspChnPrm->stNormalData.uiRtDirection, pstDataNode->enDispDir, pstDataNode->enPbOpDir);
                    return SAL_FAIL;
                }
            }
            else if (pstPbHdr->u64ColNo < u64SliceNo) // 条带序号递减，与实时过包方向相反，实时过包的START即END，END即START
            {
                if (XSP_PB_OPDIR_OPPOSITE != pstDataNode->enPbOpDir)
                {
                    XSP_LOGE("chan %u, pre: %llu, cur: %u - %llu, RtDir: %d, PbDir: %d, PbOpDir: %d, slices in this frame are out of order\n", 
                             chan, u64SliceNo, i, pstPbHdr->u64ColNo, pstXspChnPrm->stNormalData.uiRtDirection, pstDataNode->enDispDir, pstDataNode->enPbOpDir);
                    return SAL_FAIL;
                }
            }
            else
            {
                /* 当前条带序号与前一个相同，只存在一种情况：条带被一分为二了，否则就是异常 */
                if (pstPbHdr->u32HeightIn == pstPbHdr->u32HeightTotal)
                {
                    XSP_LOGE("chan %u, pre:%llu,  cur:%u - %llu, slices in this frame are out of order\n", chan, u64SliceNo, i, pstPbHdr->u64ColNo);
                    return SAL_FAIL;
                }
            }

            if (XSP_SLICE_NEIGHBOUR != pstPbHdr->enSliceCont) // 邻域条带不判断包裹标记
            {    /* 无包裹开始或包裹结束的情况 */
                if ((pstPbHdr->enPackTag == XSP_PACKAGE_NONE && enSliceTag == XSP_PACKAGE_MIDDLE) ||
                    (pstPbHdr->enPackTag == XSP_PACKAGE_MIDDLE && enSliceTag == XSP_PACKAGE_NONE))
                {
                    XSP_LOGE("chan %u, preTag: %d, curNo. %u curTag %d sliceCont %d, slice package tag is invalid\n", \
                                                    chan, enSliceTag, i, pstPbHdr->enPackTag, pstPbHdr->enSliceCont);
                    return SAL_FAIL;
                }

                enSliceTag = pstPbHdr->enPackTag;
            }
            u64SliceNo = pstPbHdr->u64ColNo;
        }

        if (XSP_SLICE_NEIGHBOUR != pstPbHdr->enSliceCont)
        {
            if (pstPbHdr->bAppendPackageResult) /* XSP_PACKAGE_START 或 XSP_PACKAGE_END 才会携带 */
            {
                // 获取包裹分割与智能识别信息
                pstPackRst = get_pb_pack_rst(chan, pu8ShareBuf, &u32ReadIdxPb, u32ShareBufLen);
                pstDbgPackRst = &gstXrayTskCtrl[chan].stPbDebug.stPackRst[gstXrayTskCtrl[chan].stPbDebug.u32PackRstStartIdx];
                pstDbgPackRst->u32PackageId = pstPackRst->u32PackageId;
                pstDbgPackRst->u32PackageWidth = pstPackRst->u32PackageWidth;
                pstDbgPackRst->u32PackageHeight = pstPackRst->u32PackageHeight;

                memcpy(&pstDbgPackRst->stSvaInfo, &pstPackRst->stSvaInfo, sizeof(SVA_DSP_OUT));
                memcpy(&pstDbgPackRst->stProPrm, &pstPackRst->stProPrm, sizeof(XSP_PACK_INFO));
                pstDbgPackRst->u64AssociatedColNo = pstPbHdr->u64ColNo;
                gstXrayTskCtrl[chan].stPbDebug.u32PackRstStartIdx = (gstXrayTskCtrl[chan].stPbDebug.u32PackRstStartIdx + 1) % XRAY_DEBUG_PB_PACK_RST_NUM;

                if ((XSP_PACKAGE_END == pstPbHdr->enPackTag) || (XRAY_PB_FRAME == pstDataNode->enPbMode))
                {
                    /* 回拉包裹信息打印 */
                    XSP_LOGI("chn %u, col:%llu, pbPackResult[id:%u, w:%u, h:%u, col:%u~%u, top:%u, bot:%u, forceDiv:%u, z:%u, svaTag:%u], target_num:%u_%u_%u.\n",
                             chan, pstPbHdr->u64ColNo, 
                             pstPackRst->u32PackageId, pstPackRst->u32PackageWidth, pstPackRst->u32PackageHeight, 
                             pstPackRst->stProPrm.stTransferInfo.uiColStartNo, pstPackRst->stProPrm.stTransferInfo.uiColEndNo,
                             pstPackRst->stProPrm.stTransferInfo.uiPackTop, pstPackRst->stProPrm.stTransferInfo.uiPackBottom, 
                             pstPackRst->stProPrm.stTransferInfo.uiIsForcedToSeparate, pstPackRst->stProPrm.stTransferInfo.uiZdataVersion, 
                             pstPackRst->stProPrm.stTransferInfo.uiSvaResultTag, 
                             pstPackRst->stSvaInfo.target_num, 
                             pstPackRst->stProPrm.stResultData.stUnpenSent.uiNum, 
                             pstPackRst->stProPrm.stResultData.stZeffSent.uiNum);
                    /* 打印智能识别坐标, 避免刷屏，最多打印5个 */
                    for (k = 0; k < SAL_CLIP(pstPackRst->stSvaInfo.target_num, 0, 5); k++)
                    {
                        XSP_LOGI("chn[%u]sva[%u], confidence:%u, rect:[%.3f, %.3f, %.3f, %.3f]\n", 
                                 chan, k, pstPackRst->stSvaInfo.target[k].visual_confidence, 
                                 pstPackRst->stSvaInfo.target[k].rect.x, pstPackRst->stSvaInfo.target[k].rect.y, 
                                 pstPackRst->stSvaInfo.target[k].rect.width, pstPackRst->stSvaInfo.target[k].rect.height);
                    }
                    /* 打印难穿透坐标 */
                    for (k = 0; k < SAL_CLIP(pstPackRst->stProPrm.stResultData.stUnpenSent.uiNum, 0, 5); k++)
                    {
                        XSP_LOGI("chn[%u]unpen[%u], rect:[%.3f, %.3f, %.3f, %.3f]\n", chan, k, 
                                 pstPackRst->stProPrm.stResultData.stUnpenSent.stRect[k].x, pstPackRst->stProPrm.stResultData.stUnpenSent.stRect[k].y, 
                                 pstPackRst->stProPrm.stResultData.stUnpenSent.stRect[k].width, pstPackRst->stProPrm.stResultData.stUnpenSent.stRect[k].height);
                    }
                    /* 打印可疑有机物识别坐标 */
                    for (k = 0; k < SAL_CLIP(pstPackRst->stProPrm.stResultData.stZeffSent.uiNum, 0, 5); k++)
                    {
                        XSP_LOGI("chn[%u]sus[%u], rect:[%.3f, %.3f, %.3f, %.3f]\n", chan, k, 
                                 pstPackRst->stProPrm.stResultData.stZeffSent.stRect[k].x, pstPackRst->stProPrm.stResultData.stZeffSent.stRect[k].y, 
                                 pstPackRst->stProPrm.stResultData.stZeffSent.stRect[k].width, pstPackRst->stProPrm.stResultData.stZeffSent.stRect[k].height);
                    }
                }

                if (0 == pstPackRst->u32PackageId)
                {
                    XSP_LOGW("chan %u, current PackageId is ZERO, sliceIdx: %u, ColNo: %llu, PackTag: %d\n", 
                             chan, i, pstPbHdr->u64ColNo, pstPbHdr->enPackTag);
                }
                if (u32PackId != pstPackRst->u32PackageId) // 与前一个包裹ID不同，则表示为新的包裹
                {
                    u32PackId = pstPackRst->u32PackageId;
                    pstDataNode->stPbPack[u32PackCnt].u32PackageWidth = pstPackRst->u32PackageWidth;
                    pstDataNode->stPbPack[u32PackCnt].u32PackageHeight = pstPackRst->u32PackageHeight;
                    memcpy(&pstDataNode->stPbPack[u32PackCnt].stPackSplit, &pstPackRst->stProPrm.stTransferInfo, sizeof(XSP_TRANSFER_INFO));
                    memcpy(&pstDataNode->stPbPack[u32PackCnt].stIndenResult, &pstPackRst->stProPrm.stResultData, sizeof(XSP_RESULT_DATA));
                    memcpy(&pstDataNode->stPbPack[u32PackCnt].stSvaInfo, &pstPackRst->stSvaInfo, sizeof(SVA_DSP_OUT));
                    pstDataNode->stPbPack[u32PackCnt].stPackSplit.uiColStartNo = XSP_PACK_LINE_INFINITE;
                    pstDataNode->stPbPack[u32PackCnt].stPackSplit.uiColEndNo = XSP_PACK_LINE_INFINITE;
                    u32PackCnt++;
                }

                if (u32PackCnt > 0)
                {
                    // 根据正向回拉和反向回拉重新计算START和END；整包回拉时应用下发的enPackTag为0
                    if (XSP_PACKAGE_START == pstPbHdr->enPackTag)
                    {
                        if (XSP_PB_OPDIR_POSITIVE == pstDataNode->enPbOpDir) /* 回拉方向与过包方向相同，实时过包的START即START */
                        {
                            pstDataNode->stPbPack[u32PackCnt-1].stPackSplit.uiColStartNo = u32PackCol;
                        }
                        else /* 回拉方向与过包方向相反，实时过包的START即END */
                        {
                            pstDataNode->stPbPack[u32PackCnt-1].stPackSplit.uiColEndNo = u32PackCol + pstPbHdr->u32HeightIn - 1;
                        }
                    }
                    else if (XSP_PACKAGE_END == pstPbHdr->enPackTag)
                    {
                        if (XSP_PB_OPDIR_POSITIVE == pstDataNode->enPbOpDir) /* 回拉方向与过包方向相同，实时过包的END即END */
                        {
                            pstDataNode->stPbPack[u32PackCnt-1].stPackSplit.uiColEndNo = u32PackCol + pstPbHdr->u32HeightIn - 1;
                        }
                        else /* 回拉方向与过包方向相反，实时过包的END即START */
                        {
                            pstDataNode->stPbPack[u32PackCnt-1].stPackSplit.uiColStartNo = u32PackCol;
                        }
                    }

                    if (XRAY_PB_FRAME == pstDataNode->enPbMode)
                    {
                        pstDataNode->stPbPack[u32PackCnt-1].stPackSplit.uiColStartNo = 0;
                        pstDataNode->stPbPack[u32PackCnt-1].stPackSplit.uiColEndNo =  pstPbHdr->u32HeightIn - 1;
                    }

                    // 条带回拉，记录Z值版本号，并校验所有包裹的Z值版本号是否统一
                    // TODO: Z值版本，如果条带回拉单次下发的包裹中有2个Z值版本号，目前处理不了
                    if (XRAY_PB_SLICE == pstDataNode->enPbMode)
                    {
                        if (1 == u32PackCnt)
                        {
                            gstXrayTskCtrl[chan].enPbZVer = pstPackRst->stProPrm.stTransferInfo.uiZdataVersion;
                        }
                        else
                        {
                            if (gstXrayTskCtrl[chan].enPbZVer != pstPackRst->stProPrm.stTransferInfo.uiZdataVersion)
                            {
                                XSP_LOGE("chan %u, cannot handle different Z version in one pb frame, pre: %d, cur: %d\n", 
                                         chan, gstXrayTskCtrl[chan].enPbZVer, pstPackRst->stProPrm.stTransferInfo.uiZdataVersion);
                                return SAL_FAIL;
                            }
                        }
                    }
                }
            }

            u32PackCol += pstPbHdr->u32HeightIn;
        }

        /* 获取回拉数据 */
        u32LeSize = XRAW_LE_SIZE(pstPbHdr->u32Width, pstPbHdr->u32HeightIn);
        u32HeSize = XRAW_HE_SIZE(pstPbHdr->u32Width, pstPbHdr->u32HeightIn);
        u32ZSize = XRAW_Z_SIZE(pstPbHdr->u32Width, pstPbHdr->u32HeightIn);
        if (XSP_SLICE_PACKAGE == pstPbHdr->enSliceCont || XSP_SLICE_BLANK == pstPbHdr->enSliceCont) // 包裹数据与空白数据
        {
            if (pstPbHdr->u32Width == pstCapbXsp->xraw_width_resized &&
                0 == pstDataNode->u32PbNeighbourBottom) // 下邻域高度若大于0，之后不允许再出现包裹数据
            {
                /* 首次读取到包裹数据，但发现上邻域数据不足，预留最小限制区域，在拷贝包裹数据后再做填充 */
                if (!bDispSliceRd && pstDataNode->u32PbNeighbourTop < XSP_NEIGHBOUR_H_MIN)
                {
                    pu8LeReal = pu8LeStart + XRAW_LE_SIZE(pstPbHdr->u32Width, XSP_NEIGHBOUR_H_MIN);
                    if (XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num)
                    {
                        pu8HeReal = pu8HeStart + XRAW_HE_SIZE(pstPbHdr->u32Width, XSP_NEIGHBOUR_H_MIN);
                        pu8ZReal = pu8ZStart + XRAW_Z_SIZE(pstPbHdr->u32Width, XSP_NEIGHBOUR_H_MIN);
                    }
                }

                // 拷贝包裹数据
                if (pstPbHdr->u32Width * pstPbHdr->u32HeightIn * pstCapbXsp->xsp_normalized_raw_bw == pstPbHdr->u32Size)
                {
                    pu8LeReal = read_data_from_ringbuf(pu8LeReal, pu8ShareBuf, u32ShareBufLen, &u32ReadIdxPb, u32LeSize);
                    if (XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num)
                    {
                        pu8HeReal = read_data_from_ringbuf(pu8HeReal, pu8ShareBuf, u32ShareBufLen, &u32ReadIdxPb, u32HeSize);
                        pu8ZReal = read_data_from_ringbuf(pu8ZReal, pu8ShareBuf, u32ShareBufLen, &u32ReadIdxPb, u32ZSize);
                    }
                }
                else if (0 == pstPbHdr->u32Size && XSP_SLICE_BLANK == pstPbHdr->enSliceCont) /* 空白数据支持u32Size为0的情况，此时内部自动填充 */
                {
                    memset(pu8LeReal, XSP_RAW_BLANKBG_LHE, u32LeSize);
                    pu8LeReal += u32LeSize;
                    if (XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num)
                    {
                        memset(pu8HeReal, XSP_RAW_BLANKBG_LHE, u32HeSize);
                        pu8HeReal += u32HeSize;
                        memset(pu8ZReal, XSP_RAW_BLANKBG_Z, u32ZSize);
                        pu8ZReal += u32ZSize;
                    }
                }
                else
                {
                    XSP_LOGE("chan %u, this slice is invalid, enSliceCont: %d, u32Width: %u, u32HeightIn: %u, u32HeightTotal: %u, u32Size: %u\n", 
                             chan, pstPbHdr->enSliceCont, pstPbHdr->u32Width, pstPbHdr->u32HeightIn, pstPbHdr->u32HeightTotal, pstPbHdr->u32Size);
                    return SAL_FAIL;
                }

                if (!bDispSliceRd && pstDataNode->u32PbNeighbourTop < XSP_NEIGHBOUR_H_MIN)
                {
                    patch_pb_neighbour(pstDataNode, SAL_TRUE, pu8LeStart, pu8HeStart, pu8ZStart);
                }

                bDispSliceRd = SAL_TRUE;
            }
            else
            {
                // 数据错误，直接丢弃该节点
                XSP_LOGE("chan %u, this slice is invalid, enSliceCont: %d, u32Width: %u, u32PbNeighbourBottom: %u\n", 
                         chan, pstPbHdr->enSliceCont, pstPbHdr->u32Width, pstDataNode->u32PbNeighbourBottom);
                return SAL_FAIL;
            }
        }
        else if (XSP_SLICE_NEIGHBOUR == pstPbHdr->enSliceCont) // 邻域条带，只给算法做图像补边处理，实际不显示
        {

            if (pstPbHdr->u32Width == pstCapbXsp->xraw_width_resized &&
                pstPbHdr->u32Width * pstPbHdr->u32HeightIn * pstCapbXsp->xsp_normalized_raw_bw == pstPbHdr->u32Size)
            {
                if (!bDispSliceRd) // 尚未读取到包裹数据，即为上邻域
                {
                    if (pstDataNode->u32PbNeighbourTop + pstPbHdr->u32HeightIn > XSP_NEIGHBOUR_H_MAX) // 上邻域高度超过了最大限制，最顶部数据强制抹除
                    {
                        XSP_LOGW("chan %u, too much top neighbour slice, u32PbNeighbourTop: %u, u32HeightIn: %u\n", 
                                 chan, pstDataNode->u32PbNeighbourTop, pstPbHdr->u32HeightIn);
                        memmove(pu8LeStart, 
                                pu8LeStart + XRAW_LE_SIZE(pstPbHdr->u32Width, pstPbHdr->u32HeightIn+pstDataNode->u32PbNeighbourTop-XSP_NEIGHBOUR_H_MAX), 
                                XRAW_LE_SIZE(pstPbHdr->u32Width, XSP_NEIGHBOUR_H_MAX));
                        pu8LeReal -= XRAW_LE_SIZE(pstPbHdr->u32Width, pstPbHdr->u32HeightIn+pstDataNode->u32PbNeighbourTop-XSP_NEIGHBOUR_H_MAX);
                        if (XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num)
                        {
                            memmove(pu8HeStart, 
                                    pu8HeStart + XRAW_HE_SIZE(pstPbHdr->u32Width, pstPbHdr->u32HeightIn+pstDataNode->u32PbNeighbourTop-XSP_NEIGHBOUR_H_MAX), 
                                    XRAW_HE_SIZE(pstPbHdr->u32Width, XSP_NEIGHBOUR_H_MAX));
                            pu8HeReal -= XRAW_HE_SIZE(pstPbHdr->u32Width, pstPbHdr->u32HeightIn+pstDataNode->u32PbNeighbourTop-XSP_NEIGHBOUR_H_MAX);
                            memmove(pu8ZStart, 
                                    pu8ZStart + XRAW_Z_SIZE(pstPbHdr->u32Width, pstPbHdr->u32HeightIn+pstDataNode->u32PbNeighbourTop-XSP_NEIGHBOUR_H_MAX), 
                                    XRAW_Z_SIZE(pstPbHdr->u32Width, XSP_NEIGHBOUR_H_MAX));
                            pu8ZReal -= XRAW_Z_SIZE(pstPbHdr->u32Width, pstPbHdr->u32HeightIn+pstDataNode->u32PbNeighbourTop-XSP_NEIGHBOUR_H_MAX);
                        }
                        pstDataNode->u32PbNeighbourTop = XSP_NEIGHBOUR_H_MAX;
                    }
                    else
                    {
                        pstDataNode->u32PbNeighbourTop += pstPbHdr->u32HeightIn;
                    }
                }
                else // 已读取到包裹数据，即为下邻域
                {
                    if (pstDataNode->u32PbNeighbourBottom + pstPbHdr->u32HeightIn > XSP_NEIGHBOUR_H_MAX) // 下邻域高度超过了最大限制，直接丢弃
                    {
                        XSP_LOGW("chan %u, too much bottom neighbour slice, u32PbNeighbourBottom: %u, u32HeightIn: %u\n", 
                                 chan, pstDataNode->u32PbNeighbourBottom, pstPbHdr->u32HeightIn);
                        /* 拷贝该段数据需要用一块临时Buffer，然后从这块临时Buffer中取XSP_NEIGHBOUR_H_MAX-pstDataNode->u32PbNeighbourBottom高度的数据，暂用不到先不支持 */
                        break;
                    }
                    else
                    {
                        pstDataNode->u32PbNeighbourBottom += pstPbHdr->u32HeightIn;
                    }
                }

                pu8LeReal = read_data_from_ringbuf(pu8LeReal, pu8ShareBuf, u32ShareBufLen, &u32ReadIdxPb, u32LeSize);
                if (XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num)
                {
                    pu8HeReal = read_data_from_ringbuf(pu8HeReal, pu8ShareBuf, u32ShareBufLen, &u32ReadIdxPb, u32HeSize);
                    pu8ZReal = read_data_from_ringbuf(pu8ZReal, pu8ShareBuf, u32ShareBufLen, &u32ReadIdxPb, u32ZSize);
                }
            }
            else
            {
                // 数据错误，直接丢弃该节点
                XSP_LOGE("chan %u, this slice is invalid, enSliceCont: %d, u32Width: %u, u32HeightIn: %u, u32HeightTotal: %u, u32Size: %u\n", 
                         chan, pstPbHdr->enSliceCont, pstPbHdr->u32Width, pstPbHdr->u32HeightIn, pstPbHdr->u32HeightTotal, pstPbHdr->u32Size);
                return SAL_FAIL;
            }
        }
        else
        {
            XSP_LOGE("chan %u, this slice is invalid, enSliceCont: %d, u32Width: %u, u32HeightIn: %u, u32HeightTotal: %u, u32Size: %u\n", 
                     chan, pstPbHdr->enSliceCont, pstPbHdr->u32Width, pstPbHdr->u32HeightIn, pstPbHdr->u32HeightTotal, pstPbHdr->u32Size);
            return SAL_FAIL;
        }
    }

    /* 所有条带读完，但发现下邻域数据不足，自动填充 */
    if (pstDataNode->u32PbNeighbourBottom < XSP_NEIGHBOUR_H_MIN)
    {
        patch_pb_neighbour(pstDataNode, SAL_FALSE, pu8LeStart, pu8HeStart, pu8ZStart);
    }

    if (pstDataNode->u32PbPackNum != u32PackCnt)
    {
        XSP_LOGE("chan %u, the u32PbPackNum[%u] is not equal to u32PackCnt[%u], maybe the PackIds are identical\n", 
                 chan, pstDataNode->u32PbPackNum, u32PackCnt);
        return SAL_FAIL;
    }

    /* 校验包裹分割标志，中间包裹的开始条带号和结束条带号不允许出现XSP_PACK_LINE_INFINITE */
    if (u32PackCnt > 1)
    {
        if (XSP_PACK_LINE_INFINITE == pstDataNode->stPbPack[0].stPackSplit.uiColEndNo ||
            XSP_PACK_LINE_INFINITE == pstDataNode->stPbPack[u32PackCnt-1].stPackSplit.uiColStartNo)
        {
            XSP_LOGE("chan %u, the first package ColEndNo OR last package ColStartNo is invalid, PackCnt: %u\n", chan, u32PackCnt);
            return SAL_FAIL;
        }
        for (i = 1; i < u32PackCnt-1; i++)
        {
            if (XSP_PACK_LINE_INFINITE == pstDataNode->stPbPack[i].stPackSplit.uiColStartNo ||
                XSP_PACK_LINE_INFINITE == pstDataNode->stPbPack[i].stPackSplit.uiColEndNo)
            {
                XSP_LOGE("chan %u, the middle package ColStartNo OR ColEndNo is invalid, PackCnt: %u, idx: %u\n", chan, u32PackCnt, i);
                return SAL_FAIL;
            }
        }
    }

    if ((pstNodeNraw->u32Height - 2 * XSP_NEIGHBOUR_H_MAX) != u32PackCol)
    {
        XSP_LOGE("chan %u, the u32RawInHeight[%u - 2 * %d] is not equal to u32PackCol[%u], check the slice number and context\n", 
                 chan, pstNodeNraw->u32Height, XSP_NEIGHBOUR_H_MAX, u32PackCol);
        return SAL_FAIL;
    }

    if (XRAY_PB_FRAME == pstDataNode->enPbMode) // 整包回拉，强制显示方向为过包时方向
    {
        pstDataNode->enDispDir = pstDataNode->stPbPack[0].stPackSplit.uiNoramlDirection;
        if (XRAY_DIRECTION_LEFT != pstDataNode->enDispDir && XRAY_DIRECTION_RIGHT != pstDataNode->enDispDir)
        {
            XSP_LOGE("chan %u, the enDispDir[%d] is invalid in frame playback, u32PackCnt: %u\n", chan, pstDataNode->enDispDir, u32PackCnt);
            return SAL_FAIL;
        }
    }

    /**
     * 假如这个回拉数据中没有完整的包裹，则没有Z值版本号，策略为：
     * 1. 沿用上次回拉包裹的Z值版本号
     * 2. 若上次回拉包裹的Z值版本号也没有（开机过半个包就回拉），则使用从实时过包算法中获取
     */
    if (0 == u32PackCnt)
    {
        if (XRAY_PB_FRAME == pstDataNode->enPbMode) /* 整包回拉中至少有1个包裹 */
        {
            XSP_LOGE("chan %u, XRAY_PB_FRAME mode, but u32PackCnt is 0 in this pb frame\n", chan);
            return SAL_FAIL;
        }
        else
        {
            if (gstXrayTskCtrl[chan].enPbZVer < XSP_ZDATA_VERSION_NUM)
            {
                pstDataNode->stPbPack[0].stPackSplit.uiZdataVersion = gstXrayTskCtrl[chan].enPbZVer;
            }
            else
            {
                pstDataNode->stPbPack[0].stPackSplit.uiZdataVersion = pstXspChnPrm->enRtZVer;
            }
        }
    }

    return SAL_SOK;
}


static SAL_STATUS parse_element_header(UINT32 chan, XRAY_SHARE_BUF *pstShareBuf, XRAY_ELEMENT *pstElemHdr, UINT32 *pu32ReadIdx)
{
    SAL_STATUS retVal = SAL_FAIL;
    BOOL bDTimeAutoSort = SAL_TRUE;
    UINT32 u32ReadIdxHdr = 0;       /* Share Buffer中的读索引，指向XRAY_ELEMENT */
    UINT32 u32ReadIdxData = 0;      /* Share Buffer中的读索引，指向Element数据段 */
    UINT32 u32ShareBufLen = 0;      /* 数据共享缓冲区大小 */
    UINT32 u32RawInDataSize = 0;
    UINT8 *pu8ShareBuf = NULL;      /* 共享缓冲区 */
    CHAR dumpName[128] = {0};
    XIMAGE_DATA_ST *pstXrawIn = NULL;
    XSP_CORRECT_TYPE corr_type = XSP_CORR_UNDEF;
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;
    DSA_NODE *pNode = NULL;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    XSP_COMMON *pstXspCommon = Xsp_DrvGetCommon();
    XSP_CHN_PRM *pstXspChnPrm = &pstXspCommon->stXspChn[chan];
    XSP_DATA_NODE *pstDataNode = NULL;
    DSA_SEG_INFO stElemDataSeg = {0};
    XRAY_DEBUG_PB_FARME *pstDbgPbFrame = NULL;

    /* 入口参数校验*/
    XRAY_CHECK_PTR_IS_NULL(pstShareBuf, SAL_FAIL);

    /* 本地参数初始化*/
    pstXrayCtrl = &gstXrayTskCtrl[chan];

    /* 计算当前共享缓冲区中数据量 */
    pu8ShareBuf = pstShareBuf->pVirtAddr;
    u32ShareBufLen = pstShareBuf->bufLen;
    u32ReadIdxHdr = pstShareBuf->readIdx;
    u32ReadIdxData = (pstShareBuf->readIdx + sizeof(XRAY_ELEMENT)) % u32ShareBufLen; // 跳过结构体XRAY_ELEMENT

    // 校验share buffer中的数据量是否充足
    if (DIST(pstShareBuf->writeIdx, u32ReadIdxData, u32ShareBufLen) < pstElemHdr->dataLen)
    {
        XSP_LOGI("chan %u, current data size in share buffer is insufficient, writeIdx: %u, readIdx: %u, actual size: %u, "
                 "XRAY_ELEMENT: %lu, dataLen: %u(w:%u, h:%u)\n", chan, pstShareBuf->writeIdx, u32ReadIdxData, 
                 DIST(pstShareBuf->writeIdx, u32ReadIdxData, u32ShareBufLen), sizeof(XRAY_ELEMENT), 
                 pstElemHdr->dataLen, pstElemHdr->width, pstElemHdr->height);
        *pu32ReadIdx = u32ReadIdxHdr; // 若share buffer中的数据量不充足，输出的读索引保持不变
        return SAL_FAIL;
    }

    /* 校验信息头中参数，无效时丢弃 */
    if (XRAY_TYPE_DIAGNOSE == pstElemHdr->type || XRAY_TYPE_TRANSFER == pstElemHdr->type || 
        XRAY_TYPE_AFTERGLOW == pstElemHdr->type || XRAY_TYPE_CALIBRATION == pstElemHdr->type ||
        pstElemHdr->type >= XRAY_TYPE_BUTT)
    {
        XSP_LOGE("chan %u, get a invalid element, type: %d\n", chan, pstElemHdr->type);
        *pu32ReadIdx = (u32ReadIdxData + pstElemHdr->dataLen) % u32ShareBufLen; // 跳过该Element
        return SAL_FAIL;
    }
    else
    {
        SAL_mutexTmLock(&pstXspChnPrm->condProcStat.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        if (XRAY_PS_RTPREVIEW == pstXspChnPrm->enProcStat)
        {
            SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
            // 实时预览模式仅支持下面这几种类型
            if (XRAY_TYPE_NORMAL != pstElemHdr->type && XRAY_TYPE_CORREC_FULL != pstElemHdr->type &&
                XRAY_TYPE_CORREC_ZERO != pstElemHdr->type && XRAY_TYPE_AUTO_CORR_FULL != pstElemHdr->type &&
                XRAY_TYPE_PSEUDO_BLANK != pstElemHdr->type)
            {
                XSP_LOGE("chan %u, get a invalid element in RTPREVIEW mode, type: %d\n", chan, pstElemHdr->type);
                *pu32ReadIdx = (u32ReadIdxData + pstElemHdr->dataLen) % u32ShareBufLen; // 跳过该Element
                return SAL_FAIL;
            }
        }
        else if (XRAY_PS_PLAYBACK_MASK & pstXspChnPrm->enProcStat)
        {
            SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
            // 回拉模式仅支持回拉的一种类型
            if (XRAY_TYPE_PLAYBACK != pstElemHdr->type)
            {
                XSP_LOGE("chan %u, get a invalid element in PLAYBACK mode, type: %d\n", chan, pstElemHdr->type);
                *pu32ReadIdx = (u32ReadIdxData + pstElemHdr->dataLen) % u32ShareBufLen; // 跳过该Element
                return SAL_FAIL;
            }
        }
        else // 当然处理模式已跳转到XRAY_PS_NONE，则直接清空Buffer
        {
            SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
            XSP_LOGW("chan %u, current proc stat has changed to: XRAY_PS_NONE, no need to continue\n", chan);
            *pu32ReadIdx = pstShareBuf->writeIdx; // 直接清空Buffer
            return SAL_FAIL;
        }
    }

    if (XRAY_TYPE_PLAYBACK == pstElemHdr->type) // dump回拉Share buffer中的帧
    {
        if (pstXspChnPrm->stDumpCfg.u32DumpCnt > 0 && pstXspChnPrm->stDumpCfg.u32DumpDp & XSP_DDP_PB_SHBUF)
        {
            snprintf(dumpName, sizeof(dumpName), "%s/shpb_ch%u_%u_t%llu_w%u_h%u_%zu-%u-%zu-%zu.bin",
                     pstXspChnPrm->stDumpCfg.chDumpDir, chan, pstXspChnPrm->stDbgStatus.u32ProcCnt, sal_get_tickcnt(), pstElemHdr->width, 
                     pstElemHdr->height, sizeof(XRAY_ELEMENT), pstElemHdr->dataLen, sizeof(XRAY_PB_SLICE_HEADER), sizeof(XRAY_PB_PACKAGE_RESULT));
        }
    }
    else // 均dump实时预览Share Buffer中的帧
    {
        if (pstXspChnPrm->stDumpCfg.u32DumpCnt > 0 && pstXspChnPrm->stDumpCfg.u32DumpDp & XSP_DDP_RT_SHBUF)
        {
            snprintf(dumpName, sizeof(dumpName), "%s/shrt_ch%u_%u_t%llu_w%u_h%u_%zu-%u.bin",
                     pstXspChnPrm->stDumpCfg.chDumpDir, chan, pstXspChnPrm->stDbgStatus.u32ProcCnt, sal_get_tickcnt(), 
                     pstElemHdr->width, pstElemHdr->height, sizeof(XRAY_ELEMENT), pstElemHdr->dataLen);
        }
    }
    if (pstXspChnPrm->stDumpCfg.u32DumpCnt > 0 && strlen(dumpName) > 0)
    {
        SAL_WriteToFile(dumpName, 0, SEEK_SET, pstElemHdr, sizeof(XRAY_ELEMENT));
        if (u32ReadIdxData + pstElemHdr->dataLen > u32ShareBufLen)
        {
            SAL_WriteToFile(dumpName, 0, SEEK_END, pu8ShareBuf+u32ReadIdxData, u32ShareBufLen-u32ReadIdxData);
            SAL_WriteToFile(dumpName, 0, SEEK_END, pu8ShareBuf, pstElemHdr->dataLen+u32ReadIdxData-u32ShareBufLen);
        }
        else
        {
            SAL_WriteToFile(dumpName, 0, SEEK_END, pu8ShareBuf+u32ReadIdxData, pstElemHdr->dataLen);
        }
    }

    if ((pstElemHdr->width == 0) || (pstElemHdr->width > pstCapbXsp->xraw_width_resized) || (pstElemHdr->height == 0) || 
        (pstElemHdr->height > (UINT32)(SAL_MAX(pstCapbXsp->xsp_package_line_max , pCapbXrayin->xraw_height_max) * pstCapbXsp->resize_height_factor)) || 
        (pstElemHdr->dataLen == 0))
    {
        XSP_LOGE("chan %u, get a invalid element, type: %d, width: %u, height: %u, dataLen: %u\n", 
                 chan, pstElemHdr->type, pstElemHdr->width, pstElemHdr->height, pstElemHdr->dataLen);
        *pu32ReadIdx = (u32ReadIdxData + pstElemHdr->dataLen) % u32ShareBufLen; // 跳过该Element
        return SAL_FAIL;
    }

    if ((XRAY_TYPE_NORMAL == pstElemHdr->type) || (XRAY_TYPE_PSEUDO_BLANK == pstElemHdr->type))
    {
        if (pstElemHdr->height != pCapbXrayin->st_xray_speed[pstXspChnPrm->stNormalData.enRtForm][pstXspChnPrm->stNormalData.enRtSpeed].line_per_slice)
        {
            XSP_LOGE("chan %u, get a invalid element, type: %d, height: %u, Form: %d, Speed: %d, expected: %u\n", 
                     chan, pstElemHdr->type, pstElemHdr->height, pstXspChnPrm->stNormalData.enRtForm, pstXspChnPrm->stNormalData.enRtSpeed, 
                     pCapbXrayin->st_xray_speed[pstXspChnPrm->stNormalData.enRtForm][pstXspChnPrm->stNormalData.enRtSpeed].line_per_slice);
            *pu32ReadIdx = (u32ReadIdxData + pstElemHdr->dataLen) % u32ShareBufLen; // 跳过该Element
            return SAL_FAIL;
        }
        if (pstElemHdr->dataLen > pCapbXrayin->xraw_height_max*pCapbXrayin->xraw_width_max*pstCapbXsp->xsp_normalized_raw_bw)
        {
            XSP_LOGE("chan %u, get a invalid element, dataLen: %u\n", chan, pstElemHdr->dataLen);
            return SAL_FAIL;
        }
    }
    else if (XRAY_TYPE_PLAYBACK == pstElemHdr->type)
    {
        if (pstElemHdr->dataLen >= u32ShareBufLen)
        {
            XSP_LOGE("chan %u, get a invalid element, dataLen: %u\n", chan, pstElemHdr->dataLen);
            return SAL_FAIL;
        }
        if (pstElemHdr->xrayProcParam.stXrayPlayback.uiPackageNum > XSP_PACK_NUM_MAX) // 一次回拉数据中，最多支持的包裹个数
        {
            XSP_LOGE("chan %u, cannot handle so many packages: %u, max: %d\n", chan, 
                     pstElemHdr->xrayProcParam.stXrayPlayback.uiPackageNum, XSP_PACK_NUM_MAX);
            return SAL_FAIL;
        }
        if (pstElemHdr->xrayProcParam.stXrayPlayback.uiSliceNum > pstXrayCtrl->u32FscSliceNumMax + 8) // +8：左右两边最多各有4个条带
        {
            XSP_LOGE("chan %u, uiSliceNum[%u] is too many, max: %u\n", chan, 
                     pstElemHdr->xrayProcParam.stXrayPlayback.uiSliceNum, pstXrayCtrl->u32FscSliceNumMax + 4);
            return SAL_FAIL;
        }
    }

    // 先判断链表中是否有空闲
    SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    if (DSA_LstIsFull(pstXspChnPrm->lstDataProc))
    {
        retVal = SAL_CondWait(&pstXspChnPrm->lstDataProc->sync, 2000, __FUNCTION__, __LINE__);
        if (SAL_FAIL == retVal)
        {
            SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
            XSP_LOGW("chan %u, wait for 'lstDataProc' to be free timeouted, count: %u\n", chan, DSA_LstGetCount(pstXspChnPrm->lstDataProc));
            *pu32ReadIdx = u32ReadIdxHdr;
            return SAL_FAIL;
        }
    }

    pNode = DSA_LstGetIdleNode(pstXspChnPrm->lstDataProc);
    if (NULL != pNode)
    {
        if (XRAY_PS_NONE != pstXspChnPrm->enProcStat)
        {
            pstXspChnPrm->stDbgStatus.u32ProcCnt++;
            pstDataNode = (XSP_DATA_NODE *)pNode->pAdData;
            memset(pstDataNode, 0, offsetof(XSP_DATA_NODE, pu8RtNrawTip));
            pstDataNode->enProcStage = XSP_PSTG_READY;
            pstDataNode->enProcType = pstElemHdr->type;

            xray_statistic(pstXrayCtrl, pstElemHdr);
            pstDataNode->u32DTimeItemIdx = dtime_add_item(pstXspChnPrm->pXspDbgTime, 0, 0, 0);
            bDTimeAutoSort = (XRAY_PS_RTPREVIEW == pstXspChnPrm->enProcStat) ? SAL_TRUE : SAL_FALSE;
            dtime_add_time_point(pstXspChnPrm->pXspDbgTime, pstDataNode->u32DTimeItemIdx, bDTimeAutoSort, "xsensor", pstElemHdr->time, SAL_TRUE);
            dtime_add_time_point(pstXspChnPrm->pXspDbgTime, pstDataNode->u32DTimeItemIdx, bDTimeAutoSort, "dspRawIn", -1, SAL_TRUE);
            if (XRAY_TYPE_PLAYBACK == pstDataNode->enProcType)
            {
                /* 回拉调试信息赋值 */
                pstDbgPbFrame = &pstXrayCtrl->stPbDebug.stPbFrame[pstXrayCtrl->stPbDebug.u32FrameStartIdx];
                pstDbgPbFrame->u32FrameNo = pstXrayCtrl->stPbDebug.stPbFrame[(pstXrayCtrl->stPbDebug.u32FrameStartIdx-1)%XRAY_DEBUG_PB_FARME_NUM].u32FrameNo + 1;
                pstDbgPbFrame->u32Width = pstElemHdr->width;
                pstDbgPbFrame->u32Height = pstElemHdr->height;
                pstDbgPbFrame->u32DataLen = pstElemHdr->dataLen;
                pstDbgPbFrame->direction = pstElemHdr->xrayProcParam.stXrayPlayback.direction;
                pstDbgPbFrame->uiWorkMode = pstElemHdr->xrayProcParam.stXrayPlayback.uiWorkMode;
                pstDbgPbFrame->uiSliceNum = pstElemHdr->xrayProcParam.stXrayPlayback.uiSliceNum;
                pstDbgPbFrame->uiPackageNum = pstElemHdr->xrayProcParam.stXrayPlayback.uiPackageNum;
                pstDbgPbFrame->enPbOpDir = (pstDbgPbFrame->direction == pstXspChnPrm->stNormalData.uiRtDirection) ? XSP_PB_OPDIR_POSITIVE : XSP_PB_OPDIR_OPPOSITE;
                pstXrayCtrl->stPbDebug.u32FrameStartIdx = (pstXrayCtrl->stPbDebug.u32FrameStartIdx + 1) % XRAY_DEBUG_PB_FARME_NUM;
                pstDataNode->u32RtSliceNo = pstDbgPbFrame->u32FrameNo; /* 复用u32RtSliceNo，仅作为调试 */

                XSP_LOGI("chn:%u, XRAY_ELEMENT, type:%d, w:%u, h:%u, dir:%d, mode:%d, sliNum:%u, packNum:%u\n", 
                         chan, pstElemHdr->type, pstElemHdr->width, pstElemHdr->height, 
                         pstElemHdr->xrayProcParam.stXrayPlayback.direction, 
                         pstElemHdr->xrayProcParam.stXrayPlayback.uiWorkMode, 
                         pstElemHdr->xrayProcParam.stXrayPlayback.uiSliceNum, 
                         pstElemHdr->xrayProcParam.stXrayPlayback.uiPackageNum);

                retVal = parse_pb_slice(chan, pstShareBuf, pstElemHdr, u32ReadIdxData, pstDataNode);
                if (SAL_SOK != retVal)
                {
                    XSP_LOGE("chan %u, parse_pb_slice failed, jump this element\n", chan);
                    *pu32ReadIdx = (u32ReadIdxData + pstElemHdr->dataLen) % u32ShareBufLen; // 跳过该Element
                    SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
                    return SAL_FAIL;
                }
                else
                {
                    pstDbgPbFrame->u32PbNeighbourTop = pstDataNode->u32PbNeighbourTop;
                    pstDbgPbFrame->u32PbNeighbourBottom = pstDataNode->u32PbNeighbourBottom;
                }
            }
            else
            {
                pstXrawIn = &pstDataNode->stXRawInBuf;
                pstXrawIn->enImgFmt = (XRAY_ENERGY_SINGLE == pCapbXrayin->xray_energy_num) ? DSP_IMG_DATFMT_SP16 : DSP_IMG_DATFMT_LHP;
                pstXrawIn->u32Height = pstElemHdr->height;
                ximg_set_dimension(pstXrawIn, pstElemHdr->width, pstElemHdr->width, SAL_FALSE, 0xFF);
                u32RawInDataSize = pstElemHdr->dataLen;
                pstDataNode->u64SyncTime = pstElemHdr->time;
                if ((XRAY_TYPE_NORMAL == pstDataNode->enProcType) || (XRAY_TYPE_PSEUDO_BLANK == pstDataNode->enProcType) || 
                    (XRAY_TYPE_AUTO_CORR_FULL == pstDataNode->enProcType))
                {
                    pstDataNode->enDispDir = pstElemHdr->xrayProcParam.stXrayPreview.direction;
                    pstDataNode->u32RtSliceNo = pstElemHdr->xrayProcParam.stXrayPreview.columnNo;
                    pstDataNode->u64TrigTime = pstElemHdr->xrayProcParam.stXrayPreview.uiTrigTime;
                    memcpy(pstDataNode->u32ColumnCont, pstElemHdr->xrayProcParam.stXrayPreview.packageSign, sizeof(pstDataNode->u32ColumnCont));
                }

                /* 拷贝Element数据 */
                if (u32ReadIdxData + u32RawInDataSize > u32ShareBufLen) // Element数据在Share Buffer中有回头
                {
                    stElemDataSeg.p1 = pu8ShareBuf + u32ReadIdxData;
                    stElemDataSeg.len1 = u32ShareBufLen - u32ReadIdxData;
                    stElemDataSeg.p2 = pu8ShareBuf;
                    stElemDataSeg.len2 = u32ReadIdxData + u32RawInDataSize - u32ShareBufLen;
                    /* 双能，实时过包源RAW数据与校正数据，复用pu16RtNrawTip，先并存放到pu16RtNrawTip中，然后转平面格式到stRawInBuf */
                    if ((XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num) &&
                        (XRAY_TYPE_NORMAL == pstDataNode->enProcType || XRAY_TYPE_AUTO_CORR_FULL == pstDataNode->enProcType ||
                         XRAY_TYPE_CORREC_FULL == pstDataNode->enProcType || XRAY_TYPE_CORREC_ZERO == pstDataNode->enProcType))
                    {
                        memcpy(pstDataNode->pu8RtNrawTip, stElemDataSeg.p1, stElemDataSeg.len1); // 先拷贝从u32ReadIdx到Share Buffer尾
                        memcpy(pstDataNode->pu8RtNrawTip + stElemDataSeg.len1, stElemDataSeg.p2, stElemDataSeg.len2); // 再拷贝从Share Buffer头到Element数据结束
                        xraw_interleave_2_planar((UINT16 *)pstXrawIn->pPlaneVir[0], (UINT16 *)pstDataNode->pu8RtNrawTip, 
                                                 pstXrawIn->u32Width, pstXrawIn->u32Height);
                    }
                    else if (XRAY_TYPE_PSEUDO_BLANK != pstDataNode->enProcType) // 其他非空白数据均直接拷贝到stRawInBuf中
                    {
                        memcpy(pstXrawIn->pPlaneVir[0], stElemDataSeg.p1, stElemDataSeg.len1); // 先拷贝从u32ReadIdx到Share Buffer尾
                        memcpy(pstXrawIn->pPlaneVir[0] + stElemDataSeg.len1, stElemDataSeg.p2, stElemDataSeg.len2); // 再拷贝从Share Buffer头到Element数据结束
                    }
                }
                else
                {
                    stElemDataSeg.p1 = pu8ShareBuf + u32ReadIdxData;
                    stElemDataSeg.len1 = u32RawInDataSize;
                    /* 双能，实时过包源RAW数据与校正数据，直接从Element数据转平面格式到stRawInBuf */
                    if ((XRAY_ENERGY_DUAL == pCapbXrayin->xray_energy_num) &&
                        (XRAY_TYPE_NORMAL == pstDataNode->enProcType || XRAY_TYPE_AUTO_CORR_FULL == pstDataNode->enProcType ||
                         XRAY_TYPE_CORREC_FULL == pstDataNode->enProcType || XRAY_TYPE_CORREC_ZERO == pstDataNode->enProcType))
                    {
                        xraw_interleave_2_planar((UINT16 *)pstXrawIn->pPlaneVir[0], (UINT16 *)stElemDataSeg.p1, 
                                                 pstXrawIn->u32Width, pstXrawIn->u32Height);
                    }
                    else if (XRAY_TYPE_PSEUDO_BLANK != pstDataNode->enProcType) // 其他非空白数据均直接拷贝到stRawInBuf中
                    {
                        memcpy(pstXrawIn->pPlaneVir[0], stElemDataSeg.p1, stElemDataSeg.len1);
                    }
                }

                /* 若是手动校正数据，则更新坏点校正的预置点 */
                if (XRAY_TYPE_CORREC_FULL == pstDataNode->enProcType || XRAY_TYPE_CORREC_ZERO == pstDataNode->enProcType)
                {
                    corr_type = (XRAY_TYPE_CORREC_FULL == pstDataNode->enProcType) ? XSP_CORR_FULLLOAD : XSP_CORR_EMPTYLOAD;
                    xraw_dp_detect(&pstXrayCtrl->dp_info, (UINT16 *)pstXrawIn->pPlaneVir[0], corr_type, pstXrawIn->u32Width, pstXrawIn->u32Height, XSP_VANGLE_DUAL);
                    if (pstXrayCtrl->dp_info.deadPixelNum != pstXrayCtrl->dp_log.deadPixelNum || 
                        0 != memcmp(pstXrayCtrl->dp_info.deadPixel, pstXrayCtrl->dp_log.deadPixel, pstXrayCtrl->dp_info.deadPixelNum * sizeof(XRAY_DEADPIXEL)))
                    {
                        pstXrayCtrl->dp_log.deadPixelNum = pstXrayCtrl->dp_info.deadPixelNum;
                        memcpy(pstXrayCtrl->dp_log.deadPixel, pstXrayCtrl->dp_info.deadPixel, pstXrayCtrl->dp_info.deadPixelNum * sizeof(XRAY_DEADPIXEL));
                        xraw_dp_log_report(chan, &pstXrayCtrl->dp_log);
                    }

                    xraw_dp_make_up_handle_method(&pstXrayCtrl->dp_info, pstXrawIn->u32Width);
                }

                /* 对实时过包源RAW数据与校正数据做坏点校正 */
                if (XRAY_TYPE_NORMAL == pstDataNode->enProcType || XRAY_TYPE_AUTO_CORR_FULL == pstDataNode->enProcType ||
                    XRAY_TYPE_CORREC_FULL == pstDataNode->enProcType || XRAY_TYPE_CORREC_ZERO == pstDataNode->enProcType)
                {
                    xraw_dp_correct(&pstXrayCtrl->dp_info, (UINT16 *)pstXrawIn->pPlaneVir[0], pstXrawIn->u32Width, pstXrawIn->u32Height);
                }
                else if (XRAY_TYPE_PSEUDO_BLANK == pstDataNode->enProcType) // 将背景空白数据转成过包源RAW数据
                {
                    Xsp_DrvGetPseudoBlankData(chan, (UINT16 *)pstXrawIn->pPlaneVir[0], pstXrawIn->u32Width, pstXrawIn->u32Height, SAL_TRUE);
                }

                xraw_data_mark(pstXrayCtrl, pstElemHdr, pstDataNode); // 调试，给每个条带数据上做标记
            }
            dtime_update_tag(pstXspChnPrm->pXspDbgTime, pstDataNode->u32DTimeItemIdx, pstDataNode->u32RtSliceNo, pstElemHdr->width, pstElemHdr->height);

            DSA_LstPush(pstXspChnPrm->lstDataProc, pNode); /* 将数据节点PUSH进队列，XSP开始处理 */
            SAL_CondSignal(&pstXspChnPrm->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
            SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);

            /* 回调XRAW源数据用于保存 */
            xraw_data_store(chan, u32RawInDataSize, pstDataNode, &stElemDataSeg);

            /* 更新读索引 */
            *pu32ReadIdx = (u32ReadIdxData + pstElemHdr->dataLen) % u32ShareBufLen;
            return SAL_SOK;
        }
        else // 因有阻塞，阻塞结束后，若处理模式已跳转到XRAY_PS_NONE，则不再处理了，直接清空Buffer
        {
            SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
            XSP_LOGW("chan %u, current proc stat has changed to: XRAY_PS_NONE\n", chan);
            *pu32ReadIdx = pstShareBuf->writeIdx; // 直接清空Buffer
            return SAL_FAIL;
        }
    }
    else
    {
        SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
        //XSP_LOGW("chan %u, get idle node from lstDataProc failed, count: %u, try again\n", chan, DSA_LstGetCount(pstXspChnPrm->lstDataProc));
        *pu32ReadIdx = u32ReadIdxHdr;
        return SAL_FAIL;
    }
}


/**
 * @function:   xray_analysis
 * @brief:      X RAY解析和预处理函数
 * @param[in]:  UINT chan    通道号
 * @param[out]: None
 * @return:     static INT32 成功SAL_SOK，失败SAL_FAIL
 */
static SAL_STATUS xray_analysis(UINT32 chan)
{
    SAL_STATUS retVal = SAL_SOK;
    XRAY_SHARE_BUF *pstXrayShareBuf = NULL;
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;
    XRAY_ELEMENT *pstElemHdr = NULL;
    XSP_COMMON *pstXspCommon = Xsp_DrvGetCommon();
    XSP_CHN_PRM *pstXspChnPrm = &pstXspCommon->stXspChn[chan];

    /* 本地参数初始化*/
    pstXrayCtrl = &gstXrayTskCtrl[chan];

    /* 获取当前的处理模式：实时预览/回拉 */
    SAL_mutexTmLock(&pstXspChnPrm->condProcStat.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    if (XRAY_PS_RTPREVIEW == pstXspChnPrm->enProcStat)
    {
        SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
        pstXrayShareBuf = pstXrayCtrl->pstShareBufRt;
    }
    else if (XRAY_PS_PLAYBACK_MASK & pstXspChnPrm->enProcStat)
    {
        SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
        pstXrayShareBuf = pstXrayCtrl->pstShareBufPb;
    }
    else
    {
        XSP_LOGW("chan %u, current ps is %d, stop analysising\n", chan, pstXspChnPrm->enProcStat);
        SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
        return SAL_FAIL;
    }

    // 检测是否有按键成像处理，按键成像处理的优先级更高
    if (SAL_SOK != xsp_launch_image_process(chan))
    {
        XSP_LOGW("chan %u, xsp_launch_image_process failed, try again\n", chan);
        return SAL_FAIL;
    }

    /* 设置Buffer的状态 */
    if (SAL_SOK == SAL_mutexTmLock(&pstXrayCtrl->condShareBufRead.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
    {
        pstXrayCtrl->bShareBufReading = SAL_TRUE;
        SAL_mutexTmUnlock(&pstXrayCtrl->condShareBufRead.mid, __FUNCTION__, __LINE__);
    }
    else
    {
        XSP_LOGE("chan %u, SAL_mutexTmLock 'condShareBufRead.mid' failed\n", chan);
        return SAL_FAIL;
    }

    /* 查找Element Header，并更新读索引到Element Header起始位置 */
    pstElemHdr = search_element_header(chan, pstXrayShareBuf, (UINT32 *)&pstXrayShareBuf->readIdx);
    if (NULL == pstElemHdr) // 未搜索到Element Header，重置share buffer的状态，并返回
    {
        if (SAL_SOK == SAL_mutexTmLock(&pstXrayCtrl->condShareBufRead.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
        {
            pstXrayCtrl->bShareBufReading = SAL_FALSE;
            SAL_CondSignal(&pstXrayCtrl->condShareBufRead, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
            SAL_mutexTmUnlock(&pstXrayCtrl->condShareBufRead.mid, __FUNCTION__, __LINE__);
        }
        else
        {
            XSP_LOGE("chan %u, SAL_mutexTmLock 'condShareBufRead.mid' failed\n", chan);
        }
        return SAL_FAIL;
    }

    /* 解析Element Header，并判断其参数是否正确，若正确则PUSH进队列，返回SAL_SOK */
    retVal = parse_element_header(chan, pstXrayShareBuf, pstElemHdr, (UINT32 *)&pstXrayShareBuf->readIdx);

    if (SAL_SOK == SAL_mutexTmLock(&pstXrayCtrl->condShareBufRead.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
    {
        pstXrayCtrl->bShareBufReading = SAL_FALSE;
        SAL_CondSignal(&pstXrayCtrl->condShareBufRead, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        SAL_mutexTmUnlock(&pstXrayCtrl->condShareBufRead.mid, __FUNCTION__, __LINE__);
    }
    else
    {
        XSP_LOGE("chan %u, SAL_mutexTmLock 'condShareBufRead.mid' failed\n", chan);
    }

    return retVal;
}


/**
 * @function:   xray_proc_thread
 * @brief:      X RAY解析和预处理线程
 * @param[in]:  void *prm  线程参数
 * @param[out]: None
 * @return:     void *     线程返回结果指针
 */
void *xray_proc_thread(void *args)
{
    UINT32 chan = 0;
    CHAR sTaskName[SAL_THR_NAME_LEN_MAX] = {0};
    XRAY_TSK_CTRL *pstXrayCtrl = args;
    XSP_COMMON *pstXspCommon = Xsp_DrvGetCommon();
    XSP_CHN_PRM *pstXspChnPrm = NULL;

    /* 入口参数检查 */
    XRAY_CHECK_PTR_IS_NULL(pstXrayCtrl, NULL);

    chan = pstXrayCtrl->u32Chan;
    pstXspChnPrm = &pstXspCommon->stXspChn[chan];

    /* 修改系统默认线程名称 */
    snprintf(sTaskName, sizeof(sTaskName), "xray_proc-%d", chan);
    prctl(PR_SET_NAME, (unsigned long)sTaskName);

    /* 绑A73核，通道0绑定CPU2，通道0绑定CPU3 */
    // SAL_SetThreadCoreBind((chan % 2) + 2);

    while (1)
    {
        /* 阻塞等待模块开关*/
        SAL_mutexTmLock(&pstXspChnPrm->condProcStat.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (XRAY_PS_NONE == pstXspChnPrm->enProcStat)
        {
            SAL_CondWait(&pstXspChnPrm->condProcStat, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }
        SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);

        /* 数据解析和预处理*/
        if (SAL_SOK != xray_analysis(chan))
        {
            sal_msleep_by_nano(10); // 出错时等待100ms
        }
    }

    return NULL;
}


/**
 * @function:   xray_tsk_init
 * @brief:      X RAY处理模块初始化
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32 成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS xray_tsk_init(void)
{
    SAL_STATUS ret_val = SAL_SOK;
    UINT32 i = 0, j = 0, k = 0;
    UINT32 u32SliceLineMin = 0xFFFF; // 各速度下的最窄条带
    UINT32 u32IntegralTimeMin = 0xFFFF; // 各速度下的最小积分时间
    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;
    SAL_ThrHndl task = {0};
    CAPB_XRAY_IN *pstCapbXrayin = capb_get_xrayin();

    /* 校验配置的通道参数 */
    if (pstDspInfo->xrayChanCnt == 0 || pstDspInfo->xrayChanCnt > pstCapbXrayin->xray_in_chan_cnt)
    {
        XSP_LOGE("the DSPINITPARA.xrayChanCnt(%u) is invalid, range: [1, %u]\n", 
                  pstDspInfo->xrayChanCnt, pstCapbXrayin->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    /* 1. 先初始化后级的XSP模块 */
    ret_val = Xsp_DrvInit();
    if (ret_val != SAL_SOK)
    {
        XSP_LOGE("oops, Xsp_DrvInit failed\n");
        return SAL_FAIL;
    }

    /* 2. 再初始化前级的XRAY模块，XRAY模块会使用XSP模块中的一些参数，所以XRAY需后初始化 */
    for (i = 0; i < pstCapbXrayin->xray_in_chan_cnt; i++)
    {
        pstXrayCtrl = &gstXrayTskCtrl[i];

        memset(pstXrayCtrl, 0, sizeof(XRAY_TSK_CTRL));
        pstXrayCtrl->u32Chan = i;
        u32SliceLineMin = 0xFFFF; // 初始化最窄条带为大值
        for (k = 0; k < XRAY_FORM_NUM; k++)
        {
            for (j = 0; j < XRAY_SPEED_NUM; j++)
            {
                if (pstCapbXrayin->st_xray_speed[k][j].line_per_slice > 0 && pstCapbXrayin->st_xray_speed[k][j].line_per_slice < u32SliceLineMin)
                {
                    u32SliceLineMin = pstCapbXrayin->st_xray_speed[k][j].line_per_slice;
                }
            }
        }
        pstXrayCtrl->u32FscSliceNumMax = (pstCapbXrayin->xraw_height_max + u32SliceLineMin - 1) / u32SliceLineMin;
        pstXrayCtrl->enPbZVer = XSP_ZDATA_VERSION_NUM; // 初始化一个非法值，后续要判断该值有没有被重新赋值
        pstXrayCtrl->pstShareBufRt = &pstDspInfo->xrayShareBuf[i];
        pstXrayCtrl->pstShareBufPb = &pstDspInfo->xrayPbBuf[i];
        pstXrayCtrl->bShareBufReading = SAL_FALSE;
        if (SAL_SOK != SAL_CondInit(&pstXrayCtrl->condShareBufRead))
        {
            XSP_LOGE("SAL_CondInit 'condShareBufRead' failed\n");
            return SAL_FAIL;
        }

        // 初始化stXrawBuf
        u32IntegralTimeMin = 0xFFFF; // 初始化最小积分时间为大值
        for (k = 0; k < XRAY_FORM_NUM; k++)
        {
            for (j = 0; j < XRAY_SPEED_NUM; j++)
            {
                if (pstCapbXrayin->st_xray_speed[k][j].integral_time > 0 && pstCapbXrayin->st_xray_speed[k][j].integral_time < u32IntegralTimeMin)
                {
                    u32IntegralTimeMin = pstCapbXrayin->st_xray_speed[k][j].integral_time;
                }
            }
        }
        for (j = 0; j < XRAY_RAW_BUFFER_NUM; j++)
        {
            SAL_mutexTmInit(&pstXrayCtrl->stXrawBuf[j].mutexCtrl, SAL_MUTEX_ERRORCHECK);
            pstXrayCtrl->stXrawBuf[j].u32BufSize = pstCapbXrayin->xraw_width_max * pstCapbXrayin->xray_energy_num * 
                pstCapbXrayin->xraw_bytewidth * ((1000000 + u32IntegralTimeMin/2) / u32IntegralTimeMin) * 5 >> 2;
            pstXrayCtrl->stXrawBuf[j].pVirtAddr = Xray_HalModMalloc(pstXrayCtrl->stXrawBuf[j].u32BufSize, 16);
            if (NULL == pstXrayCtrl->stXrawBuf[j].pVirtAddr)
            {
                XSP_LOGE("Xray_HalModMalloc failed, buffer size: %u\n", pstXrayCtrl->stXrawBuf[j].u32BufSize);
                return SAL_FAIL;
            }
        }

        // 初始化stXcorrBuf，最多保存128行校正数据
        for (j = 0; j < XRAY_CORR_BUFFER_NUM; j++)
        {
            SAL_mutexTmInit(&pstXrayCtrl->stXcorrBuf[j].mutexCtrl, SAL_MUTEX_ERRORCHECK);
            pstXrayCtrl->stXcorrBuf[j].u32BufSize = pstCapbXrayin->xraw_width_max * pstCapbXrayin->xray_energy_num * 
                pstCapbXrayin->xraw_bytewidth * 2 * 128; // *2：本底、满载
            pstXrayCtrl->stXcorrBuf[j].pVirtAddr = Xray_HalModMalloc(pstXrayCtrl->stXcorrBuf[j].u32BufSize, 16);
            if (NULL == pstXrayCtrl->stXcorrBuf[j].pVirtAddr)
            {
                XSP_LOGE("Xray_HalModMalloc failed, buffer size: %u\n", pstXrayCtrl->stXcorrBuf[j].u32BufSize);
                return SAL_FAIL;
            }
        }

        // 初始化stXcorrFullSplit
        pstXrayCtrl->stXcorrFullSplit.u32BufSize = pstCapbXrayin->xraw_width_max * pstCapbXrayin->xray_energy_num * pstCapbXrayin->xraw_bytewidth;
        pstXrayCtrl->stXcorrFullSplit.pVirtAddr = Xray_HalModMalloc(pstXrayCtrl->stXcorrFullSplit.u32BufSize, 16);
        if (NULL != pstXrayCtrl->stXcorrFullSplit.pVirtAddr)
        {
            memset(pstXrayCtrl->stXcorrFullSplit.pVirtAddr, 0xFF, pstXrayCtrl->stXcorrFullSplit.u32BufSize);
            strcpy((CHAR *)pstXrayCtrl->stXcorrFullSplit.pVirtAddr, "RAYIN");
            strcpy((CHAR *)pstXrayCtrl->stXcorrFullSplit.pVirtAddr+pstXrayCtrl->stXcorrFullSplit.u32BufSize-strlen("RAYIN")-1, "RAYIN");
        }
        else
        {
            XSP_LOGE("Xray_HalModMalloc failed, buffer size: %u\n", pstXrayCtrl->stXcorrFullSplit.u32BufSize);
            return SAL_FAIL;
        }

        // 初始化stXcorrZeroSplit
        pstXrayCtrl->stXcorrZeroSplit.u32BufSize = pstCapbXrayin->xraw_width_max * pstCapbXrayin->xray_energy_num * pstCapbXrayin->xraw_bytewidth;
        pstXrayCtrl->stXcorrZeroSplit.pVirtAddr = Xray_HalModMalloc(pstXrayCtrl->stXcorrZeroSplit.u32BufSize, 16);
        if (NULL != pstXrayCtrl->stXcorrZeroSplit.pVirtAddr)
        {
            memset(pstXrayCtrl->stXcorrZeroSplit.pVirtAddr, 0, pstXrayCtrl->stXcorrZeroSplit.u32BufSize);
            strcpy((CHAR *)pstXrayCtrl->stXcorrZeroSplit.pVirtAddr, "RAYIN");
            strcpy((CHAR *)pstXrayCtrl->stXcorrZeroSplit.pVirtAddr+pstXrayCtrl->stXcorrZeroSplit.u32BufSize-strlen("RAYIN")-1, "RAYIN");
        }
        else
        {
            XSP_LOGE("Xray_HalModMalloc failed, buffer size: %u\n", pstXrayCtrl->stXcorrZeroSplit.u32BufSize);
            return SAL_FAIL;
        }

        pstXrayCtrl->pstElemHdrBuf = Xray_HalModMalloc(sizeof(XRAY_ELEMENT), 16);
        if (NULL == pstXrayCtrl->pstElemHdrBuf)
        {
            XSP_LOGE("Xray_HalModMalloc for 'pstElemHdrBuf' failed, buffer size: %zu\n", sizeof(XRAY_ELEMENT));
            return SAL_FAIL;
        }

        pstXrayCtrl->pstPbHdr = Xray_HalModMalloc(sizeof(XRAY_PB_SLICE_HEADER), 16);
        if (NULL == pstXrayCtrl->pstPbHdr)
        {
            XSP_LOGE("Xray_HalModMalloc for 'pstPbHdr' failed, buffer size: %zu\n", sizeof(XRAY_PB_SLICE_HEADER));
            return SAL_FAIL;
        }

        pstXrayCtrl->pstPackRst = Xray_HalModMalloc(sizeof(XRAY_PB_PACKAGE_RESULT), 16);
        if (NULL == pstXrayCtrl->pstPackRst)
        {
            XSP_LOGE("Xray_HalModMalloc for 'pstPackRst' failed, buffer size: %zu\n", sizeof(XRAY_PB_PACKAGE_RESULT));
            return SAL_FAIL;
        }

        /* 创建任务线程*/
        SAL_thrCreate(&task, xray_proc_thread, SAL_THR_PRI_DEFAULT, 0, pstXrayCtrl);
    }

    /* 3. 转存参数初始化 */
    if (SAL_SOK != SAL_mutexTmInit(&gstXrayTransCtrl.mutexTrans, SAL_MUTEX_ERRORCHECK))
    {
        XSP_LOGE("SAL_mutexTmInit 'gstXrayTransCtrl.mutexTrans' failed\n");
        return SAL_FAIL;
    }
    gstXrayTransCtrl.u32TransTimeout = 1500; // 转存超时时间，1500ms

    return SAL_SOK;
}


/**
 * @function:   Host_XrayInputStart
 * @brief:      启动X ray数据输入，包括本地预览或回拉，均通过本接口。使用不
                同的通道来区分预览、回拉
 * @param[in]:  UINT32 chan     通道号
 * @param[in]:  UINT32* pParam  命令参数
 * @param[in]:  UINT32* pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     int             成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Host_XrayInputStart(UINT32 chan, UINT32 *pParam, UINT32 *pBuf)
{
    UINT64 time1, time2 = 0;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;
    XSP_COMMON *pstXspCommon = Xsp_DrvGetCommon();
    XSP_CHN_PRM *pstXspChnPrm = NULL;

    time1 = sal_get_tickcnt();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d !\n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    pstXrayCtrl = &gstXrayTskCtrl[chan];
    pstXspChnPrm = &pstXspCommon->stXspChn[chan];

    SAL_mutexTmLock(&pstXspChnPrm->condProcStat.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    if (XRAY_PS_NONE != pstXspChnPrm->enProcStat)
    {
        SAL_WARN("chan %u, %s has started, return directly\n", 
                 chan, (XRAY_PS_RTPREVIEW == pstXspChnPrm->enProcStat)?"RT Preview":"Playback");
        SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
        return SAL_FAIL;
    }
    SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);

    /* 切换到新的运行模式 */
    if (SAL_SOK != Xsp_DrvPrepareNewProcType(chan, XRAY_PS_RTPREVIEW, NULL))
    {
        XSP_LOGE("chan %u, Xsp_DrvPrepareNewProcType XRAY_PS_RTPREVIEW failed\n", chan);
        return SAL_FAIL;
    }
    Xsp_DrvChangeProcType(chan, XRAY_PS_RTPREVIEW);

    time2 = sal_get_tickcnt();
    XSP_LOGD("Start preview, chan: %d, readIdx: %u, writeIdx: %u, ShareBufStat: %d, time: %lldms\n", \
             chan, pstXrayCtrl->pstShareBufRt->readIdx, pstXrayCtrl->pstShareBufRt->writeIdx, 
             pstXrayCtrl->bShareBufReading, time2 - time1);

    return SAL_SOK;
}


/**
 * @function:   Host_XrayInputStop
 * @brief:      停止X ray数据输入，包括本地预览或回拉，均通过本接口。使用不
                同的通道来区分预览、回拉
 * @param[in]:  UINT32 chan     通道号
 * @param[in]:  UINT32* pParam  命令参数
 * @param[in]:  UINT32* pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     int             成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Host_XrayInputStop(UINT32 chan, UINT32 *pParam, UINT32 *pBuf)
{
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;
    XSP_COMMON *pstXspCommon = Xsp_DrvGetCommon();
    XSP_CHN_PRM *pstXspChnPrm = NULL;
    BOOL bForceStop = SAL_FALSE;
	UINT64 ts = 0, te = 0, tl = 3000; // 最长等待3000ms，若3000ms仍无返回，则肯定出现异常了
    UINT64 time1, time2 = 0;

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d!\n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    time1 = sal_get_tickcnt();
    pstXrayCtrl = &gstXrayTskCtrl[chan];
    pstXspChnPrm = &pstXspCommon->stXspChn[chan];

    SAL_mutexTmLock(&pstXspChnPrm->condProcStat.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    if (XRAY_PS_NONE == pstXspChnPrm->enProcStat)
    {
        SAL_WARN("chan %u, RT Preview has stopped, return directly\n", chan);
        SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
        return SAL_FAIL;
    }
    SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);

    XSP_LOGD("Stop preview, chan: %d, readIdx: %u, writeIdx: %u, datalen: %u, ShareBufStat: %d\n", \
             chan, pstXrayCtrl->pstShareBufRt->readIdx, pstXrayCtrl->pstShareBufRt->writeIdx, 
             DIST(pstXrayCtrl->pstShareBufRt->writeIdx, pstXrayCtrl->pstShareBufRt->readIdx, pstXrayCtrl->pstShareBufRt->bufLen), 
             pstXrayCtrl->bShareBufReading);

    /* 等待Xpack与显示处理完成，最多等待1500ms */
    if (SAL_SOK != Xsp_WaitDispBufferDescend(chan, 100, 15, 0))
    {
        XSP_LOGE("chan %d, Xsp_WaitDispBufferDescend failed, force to stop\n", chan);
        bForceStop = SAL_TRUE;
    }

    /* 切换运行模式 */
    Xsp_DrvChangeProcType(chan, XRAY_PS_NONE);

    if (bForceStop)
    {
        /* 等待当前读结束，读结束后进入Xray_Proc_Wait */
        SAL_mutexTmLock(&pstXrayCtrl->condShareBufRead.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (pstXrayCtrl->bShareBufReading)
        {
            if (tl + ts > te)
            {
                tl = tl + ts - te;
                ts = sal_get_tickcnt();
                SAL_CondWait(&pstXrayCtrl->condShareBufRead, tl, __FUNCTION__, __LINE__);
                te = sal_get_tickcnt();
            }
            else
            {
                XSP_LOGE("wait enShareBufStat turn to 'EVENT_HNS_END' timeout...\n");
                break; // 超时退出
            }
        }
        pstXrayCtrl->pstShareBufRt->readIdx = pstXrayCtrl->pstShareBufRt->writeIdx; /* 清空缓存 */
        SAL_mutexTmUnlock(&pstXrayCtrl->condShareBufRead.mid, __FUNCTION__, __LINE__);

        /* 等待Xpack与显示处理完成，最多等待1000ms */
        if (SAL_SOK != Xsp_WaitDispBufferDescend(chan, 50, 20, 0))
        {
            XSP_LOGE("chan %d, Xsp_WaitDispBufferDescend failed, force to stop\n", chan);
            return SAL_FAIL;
        }
    }
    time2 = sal_get_tickcnt();

    XSP_LOGD("Stop preview done, chan: %d, readIdx: %u, writeIdx: %u, ShareBufStat: %d, time: %lldms\n",\
             chan, pstXrayCtrl->pstShareBufRt->readIdx, pstXrayCtrl->pstShareBufRt->writeIdx, pstXrayCtrl->bShareBufReading, time2-time1);

    return SAL_SOK;
}


/**
 * @function:   Host_XrayPlaybackStart
 * @brief:      启动X ray数据输入，回拉通过本接口。
 * @param[in]:  UINT32 chan     通道号
 * @param[in]:  UINT32* pParam  命令参数
 * @param[in]:  UINT32* pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     int             成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Host_XrayPlaybackStart(UINT32 chan, UINT32 *pParam, UINT32 *pBuf)
{
    XRAY_PROC_STATUS_E enProcType = XRAY_PS_NONE;
    XRAY_PB_PARAM *pstPbParam = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;
    XSP_COMMON *pstXspCommon = Xsp_DrvGetCommon();
    XSP_CHN_PRM *pstXspChnPrm = NULL;
    UINT64 time1, time2 = 0;

    /* 输入参数校验*/
    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d! \n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }
    if (NULL == pBuf)
    {
        XSP_LOGE("chan %u, the 'pBuf' is NULL\n", chan);
        return SAL_FAIL;
    }

    pstPbParam = (XRAY_PB_PARAM *)(pBuf);
    if (pstPbParam->enPbType >= XRAY_PB_NUM)
    {
        XSP_LOGE("chan %u, PbType(%d) is invalid\n", chan, pstPbParam->enPbType);
        return SAL_FAIL;
    }
    if (pstPbParam->enPbSpeed >= XRAY_PB_SPEED_NUM)
    {
        XSP_LOGE("chan %u, PbSpeed(%d) is invalid\n", chan, pstPbParam->enPbSpeed);
        return SAL_FAIL;
    }

    time1 = sal_get_tickcnt();
    pstXrayCtrl = &gstXrayTskCtrl[chan];
    pstXspChnPrm = &pstXspCommon->stXspChn[chan];

    SAL_mutexTmLock(&pstXspChnPrm->condProcStat.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    if (XRAY_PS_NONE != pstXspChnPrm->enProcStat)
    {
        SAL_WARN("chan %u, %s has started, return directly\n", 
                 chan, (XRAY_PS_RTPREVIEW == pstXspChnPrm->enProcStat)?"RT Preview":"Playback");
        SAL_mutexUnlock(&pstXspChnPrm->condProcStat.mid);
        return SAL_FAIL;
    }
    SAL_mutexUnlock(&pstXspChnPrm->condProcStat.mid);

    /* 切换到新的运行模式 */
    if (XRAY_PB_SLICE == pstPbParam->enPbType)
    {
        enProcType = (XRAY_PB_SPEED_NORMAL == pstPbParam->enPbSpeed) ? XRAY_PS_PLAYBACK_SLICE_FAST : XRAY_PS_PLAYBACK_SLICE_XTS;
    }
    else
    {
        enProcType = XRAY_PS_PLAYBACK_FRAME;
    }
    if (SAL_SOK != Xsp_DrvPrepareNewProcType(chan, enProcType, pstPbParam))
    {
        XSP_LOGE("chan %u, Xsp_DrvPrepareNewProcType XRAY_PS_PLAYBACK failed, PbType: %d, PbSpeed: %d\n", 
                chan, pstPbParam->enPbType, pstPbParam->enPbSpeed);
        return SAL_FAIL;
    }
    Xsp_DrvChangeProcType(chan, enProcType);

    time2 = sal_get_tickcnt();
    XSP_LOGD("chan: %d, Start playback, readIdx: %u, writeIdx: %u, ShareBufStat: %d, enPbType: %d, enPbSpeed: %d, time: %lldms\n", \
             chan, pstXrayCtrl->pstShareBufPb->readIdx, pstXrayCtrl->pstShareBufPb->writeIdx, 
             pstXrayCtrl->bShareBufReading, pstPbParam->enPbType,  pstPbParam->enPbSpeed, time2-time1);

    return SAL_SOK; 
}

/**
 * @function:   Host_XrayPlaybackStop
 * @brief:      停止Xray数据输入，回拉通过本接口。
 * @param[in]:  UINT32 chan     通道号，不生效
 * @param[in]:  UINT32* pParam  命令参数
 * @param[in]:  UINT32* pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     int             成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Host_XrayPlaybackStop(UINT32 chan, UINT32 *pParam, UINT32 *pBuf)
{
	UINT64 ts = 0, te = 0, tl = 3000; // 最长等待3000ms，若3000ms仍无返回，则肯定出现异常了
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;
    UINT64 time1, time2 = 0;
    XSP_COMMON *pstXspCommon = Xsp_DrvGetCommon();
    XSP_CHN_PRM *pstXspChnPrm = NULL;

    if (NULL == pstCapXrayIn)
    {
        XSP_LOGE("pstCapXrayIn is null!");
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d! \n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    time1 = sal_get_tickcnt();

    for (chan = 0; chan < pstCapXrayIn->xray_in_chan_cnt; chan++)
    {
        pstXrayCtrl = &gstXrayTskCtrl[chan];
        pstXspChnPrm = &pstXspCommon->stXspChn[chan];

        SAL_mutexTmLock(&pstXspChnPrm->condProcStat.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        if (XRAY_PS_NONE == pstXspChnPrm->enProcStat)
        {
            SAL_WARN("chan %u, Playback has stopped, return directly\n", chan);
            SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
            return SAL_FAIL;
        }
        SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
    }

    XSP_LOGD("Stop playback, chan: %d, readIdx: %u, writeIdx: %u, ShareBufStat: %d\n", \
             chan, pstXrayCtrl->pstShareBufPb->readIdx, pstXrayCtrl->pstShareBufPb->writeIdx, pstXrayCtrl->bShareBufReading);

    /* 切换运行模式，XSP会自动丢弃现有回拉数据 */
    for (chan = 0; chan < pstCapXrayIn->xray_in_chan_cnt; chan++)
    {
        Xsp_DrvChangeProcType(chan, XRAY_PS_NONE);
    }

    /* 等待当前读结束，读结束后进入Xray_Proc_Wait */
    for (chan = 0; chan < pstCapXrayIn->xray_in_chan_cnt; chan++)
    {
        pstXrayCtrl = &gstXrayTskCtrl[chan];
        pstXspChnPrm = &pstXspCommon->stXspChn[chan];

        /* 
         * 应用下发此命令之前需要停止往share buffer写入数据，否则此处可能会导致share buffer有遗留数据 
         * 此处先将share buffer清空是为了减少后续等待队列为空的时间
         */
        SAL_mutexTmLock(&pstXrayCtrl->condShareBufRead.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (pstXrayCtrl->bShareBufReading)
        {
            if (tl + ts > te)
            {
                tl = tl + ts - te;
                ts = sal_get_tickcnt();
                SAL_CondWait(&pstXrayCtrl->condShareBufRead, tl, __FUNCTION__, __LINE__);
                te = sal_get_tickcnt();
            }
            else
            {
                XSP_LOGE("wait enShareBufStat turn to 'EVENT_HNS_END' timeout...\n");
                break; // 超时退出
            }
        }
        pstXrayCtrl->pstShareBufPb->readIdx = pstXrayCtrl->pstShareBufPb->writeIdx; /* 清空缓存 */
        SAL_mutexTmUnlock(&pstXrayCtrl->condShareBufRead.mid, __FUNCTION__, __LINE__);

        /* 等待处理队列为空 */
        Xsp_WaitProcListEmpty(chan, SAL_TRUE, 100, 20);
    }

    time2 = sal_get_tickcnt();
    XSP_LOGD("Stop playback done, chan: %d, readIdx: %u, writeIdx: %u, ShareBufStat: %d, time: %lld\n", \
             chan, pstXrayCtrl->pstShareBufPb->readIdx, pstXrayCtrl->pstShareBufPb->writeIdx, pstXrayCtrl->bShareBufReading, time2-time1);

    return SAL_SOK;
}


/**
 * @function:   Host_XraySetParam
 * @brief:      设置X ray处理参数
 * @param[in]:  UINT32 chan     通道号
 * @param[in]:  UINT32* pParam  命令参数
 * @param[in]:  UINT32* pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     int             成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Host_XraySetParam(UINT32 chan, UINT32 *pParam, UINT32 *pBuf)
{
    INT32 s32Ret = SAL_FAIL;
    XRAY_PARAM *pstParam = (XRAY_PARAM *)pBuf;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();

    /* 入口参数校验*/
    XRAY_CHECK_PTR_IS_NULL(pBuf, SAL_FAIL);

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        XSP_LOGE("the chan(%u) is invalid, range: [0, %u)\n", chan, pCapbXrayin->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    s32Ret = Xsp_DrvSetWhiteArea(chan, pstParam->marginTop, pstParam->marginBottom);

    return s32Ret;
}


/**
 * @fn      Host_XrayRawStoreEn
 * @brief   保存源RAW功能的开启和关闭
 * 
 * @param   chan[IN] 通道号
 * @param   pParam[IN] 命令参数，保存源RAW是否开启
 * @param   pBuf[IN] 不使用，无效
 * 
 * @return  SAL_STATUS 
 */
SAL_STATUS Host_XrayRawStoreEn(UINT32 chan, UINT32 *pParam, UINT32 *pBuf)
{
    UINT32 i = 0;
    BOOL bEnable = SAL_FALSE;
    XRAY_TSK_CTRL *pstXrayCtrl = NULL;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();

    XRAY_CHECK_PTR_IS_NULL(pParam, SAL_FAIL);
    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        XSP_LOGE("the chan(%u) is invalid, range: [0, %u)\n", chan, pCapbXrayin->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    pstXrayCtrl = &gstXrayTskCtrl[chan];
    bEnable = *pParam;

    if (bEnable)
    {
        gstXrayTskCtrl[chan].bXrawStoreEn = SAL_TRUE;
    }
    else
    {
        gstXrayTskCtrl[chan].bXrawStoreEn = SAL_FALSE;
        // TODO: 保存Buffer中残留的数据

        // 清空Buffer
        for (i = 0; i < XRAY_RAW_BUFFER_NUM; i++)
        {
            if (SAL_SOK == SAL_mutexTmLock(&pstXrayCtrl->stXrawBuf[i].mutexCtrl, 2000, __FUNCTION__, __LINE__))
            {
                pstXrayCtrl->stXrawBuf[i].u32WriteIdx = 0;
                SAL_mutexTmUnlock(&pstXrayCtrl->stXrawBuf[i].mutexCtrl, __FUNCTION__, __LINE__);
            }
            else
            {
                XSP_LOGE("SAL_mutexTmLock 'XrawBuf' failed, chan: %u, idx: %u\n", chan, i);
            }
        }
    }

    return SAL_SOK;
}


/**
 * @fn      Host_XrayTransform
 * @brief   转存归一化RAW数据图片
 * @param   chan[IN] 通道号
 * @param   pParam[IN] 命令参数
 * @param   pBuf[IN/OUT] 命令参数Buffer，结构体
 * @return  SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS Host_XrayTransform(UINT32 chan, UINT32 *pParam, VOID *pBuf)
{
    SAL_STATUS ret_val = SAL_FAIL;
    XRAY_TRANS_PROC_PARAM *pstTransParamApp = (XRAY_TRANS_PROC_PARAM *)pBuf;
    XSP_TRANS_PARAM stTransParamInner = {0};
    UINT64 t1 = 0, t2 = 0, t3 = 0;

    XRAY_CHECK_PTR_IS_NULL(pBuf, SAL_FAIL);
    // SAL_SetThreadCoreBind(3);

    t1 = sal_get_tickcnt();
    ret_val = SAL_mutexTmLock(&gstXrayTransCtrl.mutexTrans, gstXrayTransCtrl.u32TransTimeout, __FUNCTION__, __LINE__);
    if (SAL_SOK == ret_val)
    {
        XSP_LOGI("RawW:%u, RawH:%u, RawSize:%u, dstType:%d, procType:0x%x, ImgBuf:%p, BufSize:%u, DataSize:%u, sva_num:%u, Unpen:%u, Sus:%u, top:%u, bot:%u, Dir:%d, Mirror:%d, zVer:%d\n", \
                 pstTransParamApp->u32PackageWidth, pstTransParamApp->u32PackageHeight, pstTransParamApp->u32RawSize, 
                 pstTransParamApp->enImgType, pstTransParamApp->unXspProcType.u32XspProcType, 
                 pstTransParamApp->pu8ImgBuf, pstTransParamApp->u32ImgBufSize, pstTransParamApp->u32ImgDataSize, 
                 pstTransParamApp->stSvaInfo.target_num, pstTransParamApp->stXspInfo.stResultData.stUnpenSent.uiNum, pstTransParamApp->stXspInfo.stResultData.stZeffSent.uiNum, 
                 pstTransParamApp->stXspInfo.stTransferInfo.uiPackTop, pstTransParamApp->stXspInfo.stTransferInfo.uiPackBottom,
                 pstTransParamApp->stXspInfo.stTransferInfo.uiNoramlDirection, pstTransParamApp->stXspInfo.stTransferInfo.uiIsVerticalFlip,  
                 pstTransParamApp->stXspInfo.stTransferInfo.uiZdataVersion);
        
        stTransParamInner.u32RawWidth = pstTransParamApp->u32PackageWidth;
        stTransParamInner.u32RawHeight = pstTransParamApp->u32PackageHeight;
        stTransParamInner.pu8RawBuf = pstTransParamApp->pu8RawBuf;
        stTransParamInner.u32RawSize = pstTransParamApp->u32RawSize;
        stTransParamInner.enImgType = pstTransParamApp->enImgType;
        stTransParamInner.pu8ImgBuf = pstTransParamApp->pu8ImgBuf;
        stTransParamInner.u32ImgBufSize = pstTransParamApp->u32ImgBufSize;
        stTransParamInner.unXspProcType.u32XspProcType = pstTransParamApp->unXspProcType.u32XspProcType;

        memcpy(&stTransParamInner.stXspInfo, &pstTransParamApp->stXspInfo, sizeof(XSP_PACK_INFO));
        memcpy(&stTransParamInner.stXspProcParam, &pstTransParamApp->stXspProcParam, sizeof(XSP_RT_PARAMS));

        /*旋转镜像转换*/
        Xsp_DrvSetRotateAndMirror(chan, XSP_HANDLE_TRANS, stTransParamInner.stXspInfo.stTransferInfo.uiNoramlDirection, XRAY_TYPE_TRANSFER);

        /* 转存前设置成像参数 */
        if (SAL_SOK != Xsp_DrvConfigTransProcParam(SAL_TRUE, stTransParamInner.unXspProcType, &stTransParamInner.stXspProcParam))
        {
            XSP_LOGE("Xsp_DrvConfigTransProcParam failed, proc type:0x%x\n", stTransParamInner.unXspProcType.u32XspProcType);
            SAL_mutexTmUnlock(&gstXrayTransCtrl.mutexTrans, __FUNCTION__, __LINE__);
            return SAL_FAIL;
        }

        t2 = sal_get_tickcnt();
        ret_val = xsp_trans_raw2img(chan, &stTransParamInner, &pstTransParamApp->stSvaInfo);
        if (SAL_SOK == ret_val)
        {
            pstTransParamApp->u32ImgDataSize = stTransParamInner.u32ImgDataSize;
        }

        /* 转存结束后恢复成像参数 */
        Xsp_DrvConfigTransProcParam(SAL_FALSE, stTransParamInner.unXspProcType, &stTransParamInner.stXspProcParam);

        SAL_mutexTmUnlock(&gstXrayTransCtrl.mutexTrans, __FUNCTION__, __LINE__);
    }
    t3 = sal_get_tickcnt();
    XSP_LOGI("xray trans time, waiting: %llu, processing: %llu, total: %llu\n", t2-t1, t3-t2, t3-t1);

    return ret_val;
}


/**
 * @fn      Host_XrayChangeSpeed
 * @brief   切换传送带速度 
 *  
 * @param   chan[IN] 通道号，仅支持通道0，并自动复制到其他通道
 * @param   pParam[IN] 命令参数，该接口对此参数不作要求，不会使用该参数
 * @param   pBuf[IN] 命令参数Buffer 
 *  
 * @return  SAL_STATUS SAL_SOK-设置成功，SAL_FAIL-失败
 */
SAL_STATUS Host_XrayChangeSpeed(UINT32 chan, UINT32 *pParam, VOID *pBuf)
{
    UINT32 i = 0;
    SAL_STATUS retval = 0;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    XRAY_CHECK_PTR_IS_NULL(pBuf, SAL_FAIL);
    XSP_LOGD("chan %u, set conveyor speed: %d\n", chan, ((XRAY_SPEED_PARAM *)pBuf)->enSpeedUsr);

    if (chan != 0) // 判断通道号，仅支持通道0
    {
        XSP_LOGW("chan %u is invalid, just support chan 0\n", chan);
        return SAL_FAIL;
    }

    if (((XRAY_SPEED_PARAM *)pBuf)->enSpeedUsr >= XRAY_SPEED_NUM)
    {
        XSP_LOGE("the enSpeedUsr is invalid: %d, range: [%d, %d]\n", ((XRAY_SPEED_PARAM *)pBuf)->enSpeedUsr, XRAY_SPEED_LOW, XRAY_SPEED_NUM - 1);
        return SAL_FAIL;
    }

    if (((XRAY_SPEED_PARAM *)pBuf)->enFormUsr >= XRAY_FORM_NUM)
    {
        XSP_LOGE("the enFormUsr is invalid: %d, range: [%d, %d]\n", ((XRAY_SPEED_PARAM *)pBuf)->enFormUsr, XRAY_FORM_ORIGINAL, XRAY_FORM_NUM - 1);
        return SAL_FAIL;
    }

    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        retval |= Xsp_DrvChangeSpeed(i, ((XRAY_SPEED_PARAM *)pBuf)->enFormUsr, ((XRAY_SPEED_PARAM *)pBuf)->enSpeedUsr);
    }

    return retval;
}


/**
 * @fn      Host_XrayGetSliceNumAfterCls
 * @brief   获取清屏后当前屏幕已显示的条带数 
 *  
 * @param   chan[IN] 通道号
 * @param   pParam[OUT] 命令参数，清屏后当前屏幕已显示的条带数
 * @param   pBuf[IN] 命令参数Buffer，该接口对此参数不作要求，不会使用该参数 
 *  
 * @return  SAL_STATUS SAL_SOK-设置成功，SAL_FAIL-失败
 */
SAL_STATUS Host_XrayGetSliceNumAfterCls(UINT32 chan, UINT32 *pParam, VOID *pBuf)
{
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    XSP_COMMON *pstXspCommon = Xsp_DrvGetCommon();
    XSP_CHN_PRM *pstXspChnPrm = NULL;

    /* 输入参数校验*/
    if (NULL == pParam)
    {
        XSP_LOGE("chan %u, the 'pParam' is NULL\n", chan);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan[%d] is invalid, range: [0, %u)\n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    pstXspChnPrm = &pstXspCommon->stXspChn[chan];
    *pParam = pstXspChnPrm->stNormalData.u32RtSliceNumAfterCls;

    return SAL_SOK;
}

/*******************************************************************
 * @function    Host_XrayGetTransBufferSize
 * @brief       获取转存后存放图片的内存大小
 * @param[in]       UINT32  chan    通道号，未使用
 * @param[in]       UINT32  *pParam 命令参数，未使用
 * @param[in/out]   VOID    *pBuf   命令参数，结构体XRAY_TRANS_GET_BUFFER_SIZR_PARAM
 * @return
 *******************************************************************/
SAL_STATUS Host_XrayGetTransBufferSize(UINT32 chan, UINT32 *pParam, VOID *pBuf)
{
    INT32 s32ExtraHeight = 800;     // 额外高度，避免智能信息超出包裹被截断
    INT32 s32AlignedWidth = 0;      // 对齐后的包裹宽度
    INT32 s32AlignedHeight = 0;     // 对齐后的包裹高度和额外高度
    XRAY_TRANS_GET_BUFFER_SIZR_PARAM *pstParam = NULL;

    XRAY_CHECK_PTR_IS_NULL(pBuf, SAL_FAIL);

    pstParam = (XRAY_TRANS_GET_BUFFER_SIZR_PARAM *)pBuf;

    if (pstParam->u32PackageWidth <= 0 || pstParam->u32PackageHeight <= 0)
    {
        XSP_LOGE("Invalid parameters, width:%d, height:%d.\n", pstParam->u32PackageWidth, pstParam->u32PackageHeight);
        return SAL_FAIL;
    }
    s32AlignedWidth = SAL_align(pstParam->u32PackageWidth, 16);
    s32AlignedHeight = SAL_align(pstParam->u32PackageHeight + s32ExtraHeight, 16);
    pstParam->u32ImgBufSize = xsp_cal_need_img_buf_size(pstParam->enImgType, s32AlignedWidth, s32AlignedHeight);

    return SAL_SOK;
}

/**
 * @fn      HOST_XrawFullOrZeroData
 * @brief   获得满载或本底校正数据
 * @param   chan[IN] 通道号
 * @param   pParam[IN] 命令参数，该接口对此参数不作要求，不会使用该参数
 * @param   pBuf[IN] 命令参数Buffer
 * @return  SAL_STATUS SAL_SOK-设置成功，SAL_FAIL-失败
 */
SAL_STATUS HOST_XrawFullOrZeroData(UINT32 chan, UINT32 *pParam, VOID *pBuf)
{
    XRAY_RAW_FULL_OR_ZERO_DATA *pstXrayData = (XRAY_RAW_FULL_OR_ZERO_DATA *)pBuf;

    if (XRAY_TYPE_CORREC_FULL == pstXrayData->type)
    {
        // 满载校正数据
        if (SAL_SOK == xsp_get_corr_full(chan, pstXrayData->pCorrBuf, &pstXrayData->u32BufSize))
        {
            pstXrayData->u32CorrDataLen = pstXrayData->u32BufSize;
        }
        else
        {
            XSP_LOGW("xsp_get_corr_full failed, chan: %u, u32Size: %u\n", chan, pstXrayData->u32BufSize);
        }
    }
    else if (XRAY_TYPE_CORREC_ZERO == pstXrayData->type)
    {
        // 本底校正数据
        if (SAL_SOK == xsp_get_corr_zero(chan, pstXrayData->pCorrBuf, &pstXrayData->u32BufSize))
        {
            pstXrayData->u32CorrDataLen = pstXrayData->u32BufSize;
        }
        else
        {
            XSP_LOGW("xsp_get_corr_zero failed, chan: %u, u32Size: %u\n", chan, pstXrayData->u32BufSize);
        }
    }
    else
    {
        XSP_LOGW("type ERROR, type = %d\n", pstXrayData->type);
        return SAL_FAIL;
    }

    return SAL_SOK;
}