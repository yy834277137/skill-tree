/**
 * @file   xsp_drv.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  XSP成像处理驱动文件
 * @author liwenbin
 * @date   2019/10/23
 * @note :
 * @note \n History:
   1.Date        : 2019/10/23
     Author      : liwenbin
     Modification: Created file
 */
#ifndef __XSP_DRV_H__
#define __XSP_DRV_H__


/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "xsp_wrap.h"
#include "platform_hal.h"
#include "xpack_drv.h"
#include "debug_time.h"


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */

/*模块通道*/
#define XSP_MAX_CHN_CNT         (MAX_XRAY_CHAN)     /*算法最大通道数*/
#define XSP_STATS_COUNT         (64)                /*数据统计的次数*/
#define XSP_DEBUG_PB2XPACK_NUM  (64)
#define XSP_DEBUG_TIME_NUM      (160)
#define XSP_DEBUG_IDENTIFY_NUM  (32)
#define XSP_DEBUG_SLICE_CB_NUM  (160)
#define XSP_DEBUG_EMUL_PACK_NUM (32)
#define XSP_DEBUG_EMUL_SLIC_NUM (2048)
#define XSP_DEBUG_PACK_SIGN_MAX (4092)              /*(4096-4)字节*/
#define XSP_BMP_TRANS_BUF_SIZE(w, h)  ((w + 50) * (h) * 4 + 128)    /* +50是因为当图像宽度小于OSD长度时需要扩充图像宽度，避免扩充后的图像超出内存 */
#define XSP_JPG_TRANS_BUF_SIZE(w, h)  ((w + 50) * (h) * 2 + 2048)
#define XSP_GIF_TRANS_BUF_SIZE(w, h)  ((w + 50) * (h) * 2 + 2048)
#define XSP_PNG_TRANS_BUF_SIZE(w, h)  ((w + 50) * (h) * 3 + 128)
#define XSP_TIF_TRANS_BUF_SIZE(w, h)  ((w + 50) * (h) * 4 + 1024)

/*图像处理*/
#define XSP_DATA_NODE_NUM_MAX   (4)                 /*图像处理数据节点最大个数*/
#define XSP_DATA_NODE_NUM       (2)                 /*条带高度大于24时，节点个数为2个，为了节省内存的目的*/

#define XSP_OSD_RED             (0xff0000)          /*OSD叠框颜色，红色*/
#define XSP_OSD_BLUE            (0x0000ff)          /*OSD叠框颜色，蓝色*/

#define XSP_RAW_BLANKBG_LHE     (0xff)              /*RAW空白背景低高能值*/
#define XSP_RAW_BLANKBG_Z       (0x0)               /*RAW空白背景低Z值*/
#define XSP_NEIGHBOUR_H_MIN     (16)                /*RAW补边邻域最小高度*/
#define XSP_NEIGHBOUR_H_MAX     (160)               /*RAW补边邻域最大高度*/
#define XSP_LUM_DEFUALT_LEVEL   (50)                /*亮度默认等级*/

/*数据识别*/
#define XSP_PACK_MIN_LINE       (64)                /*包裹的最小识别行数*/
#define XSP_PACK_NUM_MAX        (16)                /*单一数据节点内支持的最大包裹个数*/
#define XSP_PACK_LINE_INFINITE  (0xFFFFFFFF)        /*包裹起始结束行的特殊标志，表示该起始结束行并不是包裹真实的，真实起始在该帧数据之前，真实结束在之后*/
#define TIP_CHN_CNT             (MAX_XRAY_CHAN)     /*TIP通道数*/
#define XSP_TIP_MAX_W           (720)               /*TIP数据最大XRAY宽度*/
#define XSP_TIP_MAX_H           (720)               /*TIP数据最大XRAY高度*/
#define XSP_TIP_COLUMN_OFFSET   (0)                 /*TIP数据补偿，防止TIP注入到包裹后面*/
#define XSP_TIP_OVERTIME        (10000)             /*TIP超时时间*/
#define XSP_SUS_MIN_W           (32)                /*可疑物识别最小宽度*/
#define XSP_SUS_MIN_H           (16)                /*可疑物识别最小高度*/
#define XSP_PACKAGE_BLOCK_ANA   (288)               /*智能识别的包裹段长度*/
#define XSP_PINGPONG_BUF_NUM    (2)                 /*PING-PONG Buffer个数*/
#define XSP_PIDT_QUEUE_LEN		(4)					/*包裹识别队列长度*/
#define XSP_FRESH_PER_SLICE     (6)                 /*每个条带的刷新次数*/
#define XRAY_FULL_ZERO_BUFFER_H_MAX  (200)          /* 用于申请内存最大高度 */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/**
 * @enum    XSP_IMG_PROC_MODE_E
 * @brief   成像参数配置
 */
typedef enum
{
    XSP_IMG_PROC_NONE           = 0,
    XSP_IMG_PROC_FORCE_FLUSH    = 0x1,
    XSP_IMG_PROC_DEFAULT        = 0x2,          /* 默认模式 */
    XSP_IMG_PROC_ORIGINAL       = 0x4,          /* 原始模式 */

    /* 单模式按键图像处理，范围0x10~0x80000 */
    XSP_IMG_PROC_DISP           = 0x10,         /* 显示模式 */
    XSP_IMG_PROC_ANTI           = 0x20,         /* 反色显示 */
    XSP_IMG_PROC_EE             = 0x40,         /* 边缘增强 */
    XSP_IMG_PROC_UE             = 0x80,         /* 超级增强 */
    XSP_IMG_PROC_LE             = 0x100,        /* 局增 */
    XSP_IMG_PROC_LP             = 0x200,        /* 低穿 */
    XSP_IMG_PROC_HP             = 0x400,        /* 高穿 */
    XSP_IMG_PROC_ABSOR          = 0x800,        /* 可变吸收率 */
    XSP_IMG_PROC_LUMINANCE      = 0x1000,       /* 亮度参数 */
    XSP_IMG_PROC_PSEUDO_COLOR   = 0x2000,       /* 伪彩参数 */
    XSP_IMG_PROC_DEFORI_RANGE   = 0xFFFF0,      /* 单模式按键图像处理的位宽范围 */

    /* 图像参数设置 */
    XSP_IMG_PROC_MIRROR         = 0x100000,     /* 镜像参数 */
    XSP_IMG_PROC_BKG_PARAM      = 0x200000,     /* 背景参数 */
    XSP_IMG_PROC_COLOR_TABLE    = 0x400000,     /* 成像配置表 */
    XSP_IMG_PROC_COLOR_ADJUST   = 0x800000,     /* 颜色微调 */
    XSP_IMG_PROC_CONTRAST       = 0x1000000,    /* 对比度 */
    XSP_IMG_PROC_BKG_COLOR      = 0x2000000,    /* 背景颜色 */
    XSP_IMG_PROC_COLORS_IMAGING = 0x4000000,    /* 三色六色显示 */

} XSP_IMG_PROC_MODE_E;

/**
 * @enum    XSP_PROC_STAGE
 * @brief   XSP流程的处理阶段
 */
typedef enum
{
    XSP_PSTG_NONE       = 0,        // 无
    XSP_PSTG_READY      = 1,        // 就绪，XRAY收到数据并解析完成
    XSP_PSTG_PROCING    = 2,        // 正在成像处理
    XSP_PSTG_PROCED     = 3,        // 成像处理完成
    XSP_PSTG_SENDING    = 4,        // 正在发送给XPACK模块
    XSP_PSTG_RESERVED   = 5         // 已发送给XPACK模块，并等待后续处理
}XSP_PROC_STAGE;

/**
 * @enum    XSP_DUMP_DATA_POINT_E
 * @brief   XSP流程中DUMP数据的节点
 */
typedef enum
{
    XSP_DDP_XRAW_IN                  = 0x1,
    XSP_DDP_XRAW_OUT                 = 0x2,
    XSP_DDP_XRAW_TIP                 = 0x4,
    XSP_DDP_XRAW_BLEND               = 0x8,
    XSP_DDP_DISP_OUT                 = 0x10,
    XSP_DDP_AI_YUV                   = 0x20,
    XSP_DDP_XRAW_TIPIN               = 0x40,
    XSP_DDP_RTPIPELINE               = 0x80,
    XSP_DDP_TRANS                    = 0x100,
    XSP_DDP_IDT_IN                   = 0x200,
    XSP_DDP_FULL_IN                  = 0x400,
    XSP_DDP_ZERO_IN                  = 0x800,
    XSP_DDP_RT_SHBUF                 = 0x1000,
    XSP_DDP_PB_SHBUF                 = 0x2000,
    XSP_DDP_RESERVED1                = 0x4000,
    XSP_DDP_RESERVED2                = 0x8000,
} XSP_DUMP_DATA_POINT_E;

/**
 * @enum    XSP_PB_OPDIR
 * @brief   回拉的操作方向，正向回拉OR反向回拉
 */
typedef enum
{
    XSP_PB_OPDIR_NONE       = 0,    // 无方向
    XSP_PB_OPDIR_POSITIVE   = 1,    // 回拉出图方向与实时过包方向相同
    XSP_PB_OPDIR_OPPOSITE   = 2,    // 回拉出图方向与实时过包方向相反
}XSP_PB_OPDIR;

/**
 * @enum    XSP_FSC_SEND_NUM
 * @brief   已准备全屏数据发送的线程数记录
 */
typedef enum
{
    XSP_FSC_SEND_NUM_0 = 0,    // 无线程准备好发送全屏处理数据
    XSP_FSC_SEND_NUM_1 = 1,    // 一个线程准备好发送全屏处理数据
    XSP_FSC_SEND_NUM_2 = 2,    // 两个线程准备好发送全屏处理数据
}XSP_FSC_SEND_NUM;

/**
 * @struct XSP_BASE_PRM
 * @brief  基本参数配置
 */
typedef struct
{
    XSP_VERTICAL_MIRROR_PARAM stMirrorPrm[MAX_XRAY_CHAN];   /*镜像，区分通道*/
    XSP_SUS_ALERT stSusAlert;                               /*可疑物报警*/
    XSP_UNPEN_PARAM stUnpenSet;                             /*难穿透识别*/
    XSP_COLOR_TABLE_PARAM stColorTable;                     /*颜色表*/
    XSP_DEFORMITY_CORRECTION stDeformityCor;                /*畸形矫正*/
    XSP_COLOR_ADJUST_PARAM stColorAdjust[MAX_XRAY_CHAN];    /*颜色调节，区分通道*/
    XSP_CONTRAST_PARAM stContrast[MAX_XRAY_CHAN];           /*对比度，区分通道*/
    XSP_GAMMA_PARAM stGamma[MAX_XRAY_CHAN];                 /*gamma参数，区分通道*/
} XSP_BASE_PRM;


/**
 * @struct XSP_UPDATE_PRM
 * @brief  参数配置到xsp算法
 */
typedef struct
{
    pthread_mutex_t mutexImgProc;
    XSP_PACK_INFO stXspProPrm;                        /*数据*/
    XSP_RT_PARAMS stXspRtParm;                      /*成像参数*/
    XSP_BASE_PRM stBasePrm;                         /*基本参数*/
} XSP_UPDATE_PRM;


/**
 * @struct XSP_PACK_DIV_RES
 * @brief  实时过包单个条带的包裹分割结果
 */
typedef struct
{
    BOOL bForcedDivided;            /**< 是否强制分割 */
    XSP_SLICE_CONTENT enSliceCont;  /**< 包裹条带或空白条带 */
    XSP_PACKAGE_TAG enPackageTag;   /**< 包裹的起始结束标记 */
    UINT32 u32StartSliceNo;         /**< 包裹的起始条带号，仅在enPackageTag为XSP_PACKAGE_END有效 */
    UINT64 u64StartSliceTime;       /**< 包裹的起始条带时间 */
    UINT64 u64StartTrigTime;        /**< 包裹开始触发光障时间，仅在enPackageTag为XSP_PACKAGE_END有效 */
    UINT32 u32PackLine;             /**< 包裹列数，当前条带已计入 */
    UINT32 u32SliceCnt;             /**< 当前包裹中的子条带数，取值范围[1, XSP_PACK_HAS_SLICE_NUM_MAX] */
    UINT32 u32SliceNo[XSP_PACK_HAS_SLICE_NUM_MAX]; /**< 当前包裹中的条带编号，高28位表示主条带号，低4位表示子条带号 */
    BOOL bRmBlankSliceSub;                      /**< 当前空白子条带是否移除 */
    UINT32 u32Top;                  /**< 包裹的上边界（YUV域的y坐标）*/
    UINT32 u32Bottom;               /**< 包裹的下边界（YUV域的y坐标+包裹高度）*/
    UINT32 u32PackIdx;              /**< MAX: XSP_DEBUG_PACK_SIGN_MAX*8 - 1 */
} XSP_PACK_DIV_RES;

/**
 * @struct  XSP_PACK_DIV_STAT
 * @brief   实时过包的当前整个包裹分割状态信息
 */
typedef struct
{
    XSP_PACKAGE_DIVIDE_METHOD enMethod; /**< 包裹分割方法 */
    UINT32 uiPackCurLine;               /**< 当前包裹数据占用的行数，切换速度、方向等情况操作会重置该参数为0 */
    UINT32 u32SliceCnt;                 /**< 当前包裹中的子条带数，取值范围[1, XSP_PACK_HAS_SLICE_NUM_MAX] */
    UINT32 u32SliceNo[XSP_PACK_HAS_SLICE_NUM_MAX]; /**< 当前包裹中的条带编号，高28位表示主条带号，低4位表示子条带号 */
    UINT32 u32PackIdx;                  /**< MAX: XSP_DEBUG_PACK_SIGN_MAX*8 - 1 */
    UINT32 u32LastSliceNo;              /**< 上一次处理的条带号 */
    UINT32 u32PackStartSliceNo;         /**< 实时过包最新一个包裹的起始条带号 */
    UINT64 u64PackStartSliceTime;       /**< 实时过包最新一个包裹的起始条带时间 */
    UINT64 u64PackStartTrigTime;        /**< 实时过包最新一个包裹开始触发光障的时间 */
    UINT32 u32CurTop;                   /**< 当前包裹的上边界（YUV域的y坐标） */
    UINT32 u32CurBottom;                /**< 当前包裹的下边界（YUV域的y坐标 + 高度h） */
    XSP_PACKAGE_TAG enPackTagLast;      /**< 实时过包最后一个条带的包裹开始、结束等标志 */
    BOOL bForceToLimited;               /**< 是否强制最小分割标志，TRUE-是，FALSE-否 */
    BOOL bRmBlankSlice;                        /**< 是否开启移除空白条带功能， TRUE-是，FALSE-否 */
    UINT32 u32BlkSliceDispCnt;                 /* 包裹后需要显示的空白条带数（小条带）*/
    UINT32 u32BlkSliceDispCurCnt;              /* 当前包裹后剩余显示的空白条带数（小条带） */
    UINT8 au8PackSign[XSP_DEBUG_PACK_SIGN_MAX];
    XSP_PACKAGE_DIVIDE_SEGMASTER u32segMaster;/**< 包裹主辅分割是否同步 */
} XSP_PACK_DIV_STAT;

/**
 * @struct XSP_XRAY_ATTR
 * @brief TIP数据结构体
 */
typedef struct
{
    XSP_RECT stTipResult; /*坐标信息*/
    UINT32 uiEnable;      /*是否需要画框*/
    UINT64 uiDelayTime;   /*延时显示时间，毫秒*/
    UINT32 uiColor;       /*框体颜色,红色(0xff0000),绿色(0x00ff00), 蓝色(0x0000ff)*/
} XSP_TIP_OSD;


/**
 * @struct XSP_TIP_CTRL_ST
 * @brief TIP注入控制结构体
 */
typedef struct
{
    void *pDataMutex;             /*数据锁*/
    UINT32 uiUpdate;              /*更新标志，如果是true，则需要进行注入操作*/
    UINT32 uiReady;               /*准备标志，如果是true，则告诉算法开始注入*/
    UINT32 uiHold;                /*保持标志，如果是ture，则一直是true，如果是false，一直是false*/
    UINT32 uiAsySlice;            /*双视角：两通道的TIP返回结果异步情况，值越大，两通道返回结果偏差越大*/
    UINT32 u32SetTime;            /*接受到命令时间，单位ms*/
} XSP_TIP_CTRL_ST;

/**
 * @struct XSP_TIP_ATTR
 * @brief xsp TIP数据属性
 */
typedef struct
{
    XSP_TIP_DATA_ST stTipData;                /*应用下发的tip数据*/
    XSP_TIP_CTRL_ST stTipCtrl[TIP_CHN_CNT]; /*两个通道的TIP状态数据*/
} XSP_TIP_ATTR;

typedef struct
{
    UINT32 u32RawWidth;             // 归一化RAW宽
    UINT32 u32RawHeight;            // 归一化RAW高
    UINT8 *pu8RawBuf;               // 归一化RAW数据Buffer
    UINT32 u32RawSize;              // 归一化RAW数据长度
    XRAY_TRANS_TYPE enImgType;      // 转存图片类型
    XSP_PROC_TYPE_UN unXspProcType; // 转存图片图像处理类型
    XSP_RT_PARAMS stXspProcParam;   // 转存图片图像处理参数
    UINT8 *pu8ImgBuf;               // 转存后图片数据的存放Buffer
    UINT32 u32ImgBufSize;           // 图片数据存放Buffer的大小
    UINT32 u32ImgDataSize;          // 转存后图片数据的实际大小
    XSP_PACK_INFO stXspInfo;        // 成像信息
} XSP_TRANS_PARAM;


/**
 * @struct  XSP_PB_PACKAGE
 * @brief   回拉包裹信息
 */
typedef struct
{
    UINT32 u32PackageWidth;             /* 包裹宽，基于RAW数据 */
    UINT32 u32PackageHeight;            /* 包裹高，基于RAW数据 */
    XSP_TRANSFER_INFO stPackSplit;      /* 包裹分割信息 */
    XSP_RESULT_DATA stIndenResult;      /* 区域识别数据*/
    SVA_DSP_OUT stSvaInfo;              /* 智能信息 */
} XSP_PB_PACKAGE;


/**
 * @struct  XSP_DATA_NODE
 * @brief   XSP数据属性
 */
typedef struct
{
    /*---------- 以下参数适用于实时过包（RT）和回拉（PB） ----------*/
    XSP_PROC_STAGE enProcStage;         /* 节点数据处于的处理阶段，受list中的sync锁保护 */
    XRAY_PROC_TYPE enProcType;          /* 处理类型，RT&PB */
    XRAY_PROC_DIRECTION enDispDir;      /* 显示方向，RT&PB */
    UINT32 u32DTimeItemIdx;
    BOOL bFscImgProc;                   /* 是否全屏成像处理 */
    BOOL bResetXPack;                   /* 是否重置Xpack */

    /*---------- 以下参数仅适用于实时过包（RT） ----------*/
    BOOL bTipNrawMultiplex;             /* 显示用的TIP Nraw是否能复用存储包裹的Nraw，非TIP插入阶段可复用 */
    UINT64 u64SyncTime;                 /* 输入数据的时间，仅RT */
    UINT64 u64TrigTime;                 /* 包裹触发光障的时间，仅RT */
    UINT32 u32RtSliceNo;                /* 实时过包条带号，仅RT，回拉时会复用为帧序号的调试信息 */
    UINT32 u32ColumnCont[8];            /* 各列含有的包裹有无信息 */
    XSP_PACK_DIV_RES stPackDivRes[XSP_FRESH_PER_SLICE];     /* 实时过包单个条带的包裹分割信息，仅RT */
    XSP_TIP_OSD stTipOsd;               /* TIP画框参数 */

    /*---------- 以下参数仅适用于回拉（PB） ----------*/
    XRAY_PB_TYPE enPbMode;              /* 回拉工作模式，0-整包（包裹居中显示），1-条带 */
    XSP_PB_OPDIR enPbOpDir;             /* 回拉的操作方向，1-正向回拉，2-反向回拉 */
    UINT32 u32PbNeighbourTop;           /* 该回拉数据上邻域高度 */
    UINT32 u32PbNeighbourBottom;        /* 该回拉数据下邻域高度 */
    UINT32 u32PbPackNum;                /* 该回拉数据节点内包裹个数 */
    XSP_PB_PACKAGE stPbPack[XSP_PACK_NUM_MAX]; /* 回拉包裹信息 */

    /*---------- 以下参数为存储数据的Buffer地址 ----------*/
    /**
     * @warning 
     * 成员pu8RtNrawTip的位置不允许变更，必须放在众Buffer地址的开始
     * 当获取到新的结点时，会寻找pu8RtNrawTip的偏移，并清空pu8RtNrawTip以上的所有数据
     * 回拉帧输入时，低能、高能、原子序数的存储并不是连续的，而是占用固定的高度：xraw_height_max + XSP_NEIGHBOUR_H_MAX * 2
     */
    UINT8 *pu8RtNrawTip;                /* 实时过包使用，叠加TIP的归一化的数据缓冲，用于实时过包全屏图像处理显示 */
    XIMAGE_DATA_ST stXRawInBuf;         /* 实时过包使用，输入的XRAW数据 */
    XIMAGE_DATA_ST stNRawInBuf;         /* 回拉使用，输入的归一化RAW数据,高度包含邻域高度，不实际申请内存,与stXRawInBuf使用同一块内存 */
    XIMAGE_DATA_ST stSliceNrawBuf;      /* 实时过包使用，超分之后归一化RAW数据Buffer，XSP成像处理输出，保存每个条带算法输出的nraw数据信息 */
    XIMAGE_DATA_ST stSliceOriNrawBuf;   /* 实时过包使用，超分之前归一化RAW数据Buffer，XSP成像处理输出，保存每个条带算法输出的nraw数据信息 */
    XIMAGE_DATA_ST stBlendBuf;          /* 回拉和实时过包使用，XSP成像处理的中间结果，融合灰度图，RT&PB，若已插入TIP，该中间结果中也有TIP */
    XIMAGE_DATA_ST stDispFscData;       /* 全屏缓存，算法输出全屏display数据，大小为全屏尺寸的ARGB数据量 */
    XIMAGE_DATA_ST stDispDataSub;       /* 回拉和实时过包使用，算法输出display数据，不实际申请内存,与stDispFscData使用同一块内存 */
    XIMAGE_DATA_ST stAiYuvBuf;          /* 实时过包使用，输出AI YUV Buffer */
} XSP_DATA_NODE;

/*记录不同处理类型下的状态*/
typedef struct
{
    UINT64 u64RtSliceSendTime;          /* 实时过包，最后一个条带发送给XPACK的时间，用于控制缓存条带平滑显示 */
    UINT32 u32RtSliceRefreshCnt;        /* 实时过包，每个条带的刷新次数，每个条带能刷新多少帧，不同过包速度是不一致的 */
    XRAY_PB_TYPE enPbMode;              /* 回拉类型 */
    XRAY_PB_SPEED enPbSpeed;            /* 回拉速度 */
    UINT32 bEnableSync;                 /* 是否需要主辅视角回拉同步   0-需要 1-不需要 */
    UINT32 u32RtSliceNoLast;            /* 实时过包，最后一个条带的编号 */
    UINT64 u64RtSliceSyncTimeLast;      /* 实时过包，最后一个条带的时间，这个时间是XSensor给的，用于给应用做匹配 */
    UINT64 u64RtSliceTrigTimeLast;      /* 实时过包，包裹触发光障的时间，这个时间是XSensor给的，用于给应用做匹配 */
    UINT32 u32ColumnContLast[8];        /* 实时过包，后一个条带的每列包裹信息 */
    XSP_SLICE_CONTENT enSliceContLast;  /* 实时过包，最后一个条带是否为包裹，仅在bHwSignalLast为TRUE时有效 */
    UINT32 u32RtRefreshPerFrame;        /* 实时过包，每帧刷新列数 */
    BOOL bXPackStarted;                 /* XPack是否开启 */
    UINT32 u32ImgProcMode;              /* 成像处理模式，按位表示，每一位的定义见XSP_IMG_PROC_MODE_E */
    BOOL bImgProcStarted;               /* 成像处理是否开始，加入到队列lstDataProc中即开始 */

    UINT32 u32RtPassthDataSize;         /* 实时过包流水线透传数据大小 */
    XRAY_PROC_DIRECTION uiPbDirection;  /*最后一次回拉的显示方向*/
    XRAY_PROC_DIRECTION uiRtDirection;  /*当前实时预览的过包显示方向*/
    XRAY_PROC_FORM enRtForm;            /*当前实时预览的包裹形态*/
    XRAY_PROC_SPEED enRtSpeed;          /*当前实时预览的过包速度*/
    UINT32 u32RtSliceNumAfterCls;       /**< 清屏后已显示的实时过包条带数 */
} XSP_NORMAL_DATA;

typedef struct
{
    CHAR chDumpDir[64];                /* 数据存储目录 */
    UINT32 u32DumpDp;                  /* 按位表示，XSP_DUMP_DATA_POINT中值的或 */
    UINT32 u32DumpCnt;
} XSP_DUMP_CFG;


/*TIP数据*/
typedef struct
{
    UINT32 uiEnable;                   /*使能开关，1表示开始，0表示结束*/
    UINT32 uiCol;                      /*注入位置*/
} XSP_TIP_PROCESS;

/**
 * @struct  XSP_DEBUG_STATS_S
 * @brief   调试统计数据
 */
typedef struct
{
    UINT32 u32ProcCnt;                  /**< 处理次数 */
    UINT32 u32RtSendReqCnt;             /**< 发送次数之请求 */
    UINT32 u32RtSendSuccedCnt;          /**< 发送次数之请求 */
    UINT32 u32PbSendReqCnt;             /**< 发送次数之回拉请求 */
    UINT32 u32PbSendSuccedCnt;          /**< 发送次数之回拉请求 */
    UINT32 u32RefreshSendReqCnt;        /**< 发送次数之重复帧请求 */
    UINT32 u32RefreshSendSuccedCnt;     /**< 发送次数之重复帧请求 */
} XSP_DEBUG_STATUS;

typedef struct
{
    UINT32 u32FrameNo;
    UINT32 u32YuvOffset;
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Stride;
    UINT32 u32DispDir;
    UINT32 u32PackFlag;
    UINT32 u32PackStart;
    UINT32 u32PackEnd;
    UINT32 u32PackWidth;
    UINT32 u32PackHeight;
    UINT32 u32PackTop;
    UINT32 u32PackBottom;
    UINT32 u32ShowAll;
    SVA_DSP_OUT stSvaInfo;
} XSP_DEBUG_PB2XPACK;

typedef struct
{
    INT32 s32PackId;                /* 包裹索引号 */
    UINT32 u32Width;
    UINT32 u32Height;
    UINT64 u64TimePackDivided;      /* 包裹分割时间 */
    UINT64 u64TimeIdentifyReq;      /* 识别请求时间 */
    UINT64 u64TimeIdentifyStart;    /* 识别开始时间 */
    UINT64 u64TimeIdentifyEnd;      /* 识别结束时间 */
    UINT64 u64TimeIdtRstPackGet;    /* 获取基于包裹的识别结果时间 */
    XSP_IDENTIFY_S stIdtSusOrg;     /* XSP算法输出的可疑有机物识别结果 */
    XSP_IDENTIFY_S stIdtUnpen;      /* XSP算法输出的难穿透识别结果 */
    XSP_IDENTIFY_S stIdtExplosive;  /* XSP算法输出的难穿透识别结果 */
} XSP_DEBUG_IDENTIFY;

typedef struct
{
    UINT8 *pSliceNrawAddr;          /* 条带归一化RAW数据的Buffer地址 */
    UINT32 u32SliceNrawSize;        /* 条带归一化RAW数据的大小，单位：字节 */
    UINT32 u32Width;                /* 条带的宽度，探测器方向 */
    UINT32 u32Hight;                /* 条带的高度，传送带方向 */
    UINT32 uiColNo;                 /* 条带的序号，采传透传给DSP，DSP再回调给应用 */
    XSP_SLICE_CONTENT enSliceCont;  /* 包裹条带或空白条带 */
    XSP_PACKAGE_TAG enPackageTag;   /**< 包裹的起始结束标记 */
    UINT32 u32Top;                  /**< 包裹的上边界（YUV域的y坐标）*/
    UINT32 u32Bottom;               /**< 包裹的下边界（YUV域的y坐标+包裹高度）*/
} XSP_DEBUG_SLICE_CB;

/**
 * @struct 	XSP_PIDT_NODE_IN
 * @brief 	XSP包裹难穿透&可疑有机物识别的输入数据节点属性
 */
typedef struct
{
    XIMAGE_DATA_ST stPackPrm;     /* 用于识别的包裹数据 */
    SAL_SYSFRAME_PRIVATE stXPackPriv; /* XPack需要透传的信息，输入后再直接输出 */
    UINT64 u64TimeReq;                  /* 识别请求的时间 */
    UINT64 u64TimeStart;                /* 识别开始的时间 */
    UINT64 u64TimeEnd;                  /* 识别结束的时间 */
}XSP_PIDT_NODE_IN;

/**
 * @struct 	XSP_PIDT_NODE_OUT
 * @brief 	XSP包裹难穿透&可疑有机物识别的输出结果节点属性
 */
typedef struct
{
    UINT32 u32NrawWidth;                /* 归一化包裹数据的宽 */
    UINT32 u32NrawHeight;               /* 归一化包裹数据的高 */
    XRAY_PROC_DIRECTION enDir;          /* 过包显示方向 */
    BOOL bVMirror;                      /* 是否垂直镜像 */
    XSP_IDENTIFY_S stIdtSusOrg;         /* 可疑有机物检测结果 */
    XSP_IDENTIFY_S stIdtUnpen;          /* 难穿透检测结果 */
    XSP_IDENTIFY_S stIdtExplosive;      /* 爆炸物检测结果 */
    SAL_SYSFRAME_PRIVATE stXPackPriv; /* XPack模块需要透传的信息，输入后再直接输出 */
    XSP_DEBUG_IDENTIFY *pstDbgIdt;      /* 难穿透&可疑有机物识别调试信息 */
}XSP_PIDT_NODE_OUT;

/**
 * @struct XSP_AREA_INDENTIFY_S
 * @brief 成像区域识别结构体
 */
typedef struct
{
    DSA_LIST *lstIn;            /* 输入队列 */
    DSA_LIST *lstOutPack;       /* 基于包裹的识别结果输出队列 */
    XSP_PIDT_NODE_IN stNodeIn[XSP_PIDT_QUEUE_LEN];          /* 输入队列节点 */
    XSP_PIDT_NODE_OUT stNodeOutPack[XSP_PIDT_QUEUE_LEN];    /* 基于包裹的识别结果输出队列节点 */
}XSP_PACKAGE_IDENTIFY;

/**
 * @struct XSP_FSC_SYNC
 * @brief 全屏处理数据同步发送控制结构体
 */
typedef struct
{
    COND_T           condFscSync;        /* 控制先后发送全屏处理数据线程的执行 */
    volatile XSP_FSC_SEND_NUM enSendReadyNum;     /* 记录准备好发送全屏处理数据的线程的数量 */
}XSP_FSC_SYNC;

/**
 * @struct XSP_CHN_PRM
 * @brief XSP算法模块参数
 */
typedef struct
{
    UINT32 u32Chn;                              /**< 通道号 */
    SAL_ThrHndl stThrHandlProc;                 /**< 处理线程 */
    SAL_ThrHndl stThrHandlSend;                 /**< 发送线程 */
    SAL_ThrHndl stThrHandlIdt;                  /**< 识别线程 */
    void *handle;                               /**< 算法句柄 */
    XRAY_PROC_STATUS_E enProcStat;              /**< 处理状态 */
    COND_T condProcStat;

    UINT32 u32XspBgColorStd;                    /**< 记录应用配置的Xsp背景色值*/
    UINT32 u32XspBgColorUsed;                   /**< 实际的XSP背景色值，YUV格式，受反色设置改变 */
    UINT32 raw_cover_left;                      /* 置白功能：XRAW数据的左侧置白宽度 */
    UINT32 raw_cover_right;                     /* 置白功能：XRAW数据的右侧置白宽度 */
    XSP_ZDATA_VERSION enRtZVer;                 /**< 实时预览XSP算法的Z值版本号 */

    UINT16 *pEmptyLoadData;                     /**< 本地校正数据Buffer */
    UINT16 *pFullLoadData;                      /**< 满载校正数据Buffer */

    DSA_LIST *lstDataProc;                          /**< XSP数据处理队列 */
    XSP_DATA_NODE stDataNode[XSP_DATA_NODE_NUM_MAX];/**< XSP数据节点 */

    XSP_NORMAL_DATA stNormalData;
    XSP_PACK_DIV_STAT stRtPackDivS;
    XSP_TIP_PROCESS stTipProcess;
	XSP_PACKAGE_IDENTIFY stPackIdt;

    XSP_DUMP_CFG stDumpCfg;
    XSP_DEBUG_STATUS stDbgStatus;               /**< XSP调试统计 */
    UINT32 u32Pb2XapckStartIdx;                 /**< 发送给XPACK的回拉帧起始索引，范围[0, XSP_DEBUG_PB2XPACK_NUM-1] */
    XSP_DEBUG_PB2XPACK stPb2Xpack[XSP_DEBUG_PB2XPACK_NUM];
    debug_time pXspDbgTime;
    UINT32 u32DbgIdtIdx;
    XSP_DEBUG_IDENTIFY stDbgIdentify[XSP_DEBUG_IDENTIFY_NUM];
    UINT32 u32DbgSliceCbIdx;
    XSP_DEBUG_SLICE_CB stDbgSliceCb[XSP_DEBUG_SLICE_CB_NUM];
} XSP_CHN_PRM;


/**
 * @struct XSP_COMMON
 * @brief XSP模块全局参数
 */
typedef struct
{
    XSP_UPDATE_PRM stUpdatePrm;                    /*算法相关数据*/
    XSP_CHN_PRM stXspChn[XSP_MAX_CHN_CNT];          /*XSP通道参数*/
    XSP_TIP_ATTR stTipAttr;
    XSP_FSC_SYNC stFscSync;
} XSP_COMMON;

/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

INT32 Xsp_DrvInit(void);
XSP_COMMON *Xsp_DrvGetCommon(void);

/**
 * @function   Xsp_DrvConfigTransProcParam
 * @brief      转存通道XSP参数设置
 * @param[in]  BOOL             bSetFlag          置位标志，1-设置xsp参数，0-复位xsp参数
 * @param[in]  XSP_PROC_TYPE_UN unXspProcType     算法处理类型
 * @param[in]  XSP_PROC_PARAMS *pstXspProcParamIn 算法处理参数
 * @param[out] None
 * @return     SAL_STATUS 成功SAL_SOK，失败SAL_FALSE
 */
SAL_STATUS Xsp_DrvConfigTransProcParam(BOOL bSetFlag, XSP_PROC_TYPE_UN unXspPrcoType, XSP_RT_PARAMS *punXspProcParamIn);

SAL_STATUS xsp_launch_image_process(UINT32 chan);
INT32 Xsp_DrvBlankSend(void);
INT32 Xsp_DrvSetAnti(UINT32 chan, VOID *prm);
INT32 Xsp_DrvSetEe(UINT32 chan, VOID *prm);
INT32 Xsp_DrvSetUe(UINT32 chan, VOID *prm);
INT32 Xsp_DrvSetDisp(UINT32 chan, VOID *prm);
INT32 Xsp_DrvSetFunc(UINT32 chan, VOID *prm);
INT32 Xsp_DrvSetHsv(UINT32 chan, VOID *prm);
INT32 Xsp_DrvSetUnpen(UINT32 chan, VOID *prm);
INT32 Xsp_DrvSetMode(UINT32 chan, VOID *prm);
INT32 Xsp_DrvSetOriginal(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetWhiteArea(UINT32 chan, UINT32 top_margin, UINT32 bottom_margin);
SAL_STATUS Xsp_DrvSetBlankSlice(UINT32 chan, XSP_SET_RM_BLANK_SLICE *pstRmBlankSlice);
SAL_STATUS Xsp_DrvGetVersion(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetLp(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetHp(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetLe(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetAbsor(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetLum(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetPseudoColor(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetMirror(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetSusAlert(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetEnhancedScan(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetBkgParam(UINT32 chan, XSP_BKG_PARAM *pstBkgParam);
SAL_STATUS Xsp_DrvSetColorTable(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetColorsImaging(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetDeformityCorrection(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetColorAdjust(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetStretch(UINT32 chan, XSP_STRETCH_MODE_PARAM *prm);
SAL_STATUS Xsp_DrvSetNoiseReduce(UINT32 chan, UINT32 u32NrLevel);
SAL_STATUS Xsp_DrvSetContrast(UINT32 chan, UINT32 u32ContrastLevel);
SAL_STATUS Xsp_DrvSetLeThRange(UINT32 chan, UINT32 u32LeThLower, UINT32 u32LeThUpper);
SAL_STATUS Xsp_DrvSetDtGrayRange(UINT32 chan, UINT32 u32DtGrayLower, UINT32 u32DtGrayUpper);
SAL_STATUS Xsp_DrvSetPackageDivide(UINT32 chan, XSP_PACKAGE_DIVIDE_PARAM *pstDivParam);
SAL_STATUS Xsp_DrvSetDoseCorrect(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetBkgColor(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetColdHotThreshold(UINT32 chan, VOID* prm);
SAL_STATUS Xsp_DrvSetGamma(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetAixspRate(UINT32 chan, UINT32 u32rate);
SAL_STATUS Xsp_DrvReloadZTable(UINT32 chan, void *prm);
SAL_STATUS Xsp_DrvSetAlgKey(UINT32 chan, S32 s32OptNum, S32 s32Key, S32 s32Val1, S32 s32Val2);
SAL_STATUS Xsp_DrvGetAlgKey(UINT32 chan, S32 s32Key, S32 *pImageVal, S32 *pVal1, S32 *pVal2);
SAL_STATUS Xsp_DrvShowAllAlgKeyAndValue(UINT32 chan);
SAL_STATUS Xsp_DrvGetTipRaw(UINT32 chan, VOID *prm);
SAL_STATUS Xsp_DrvSetTipPrm(UINT32 chn, VOID *prm);
void Xsp_DrvChangeProcType(UINT32 chan, XRAY_PROC_STATUS_E enProcStat);
SAL_STATUS Xsp_DrvPrepareNewProcType(UINT32 chan, XRAY_PROC_STATUS_E enProcType, XRAY_PB_PARAM *pstPbParam);
SAL_STATUS xsp_get_corr_full(UINT32 u32Chan, UINT8 *pu8CorrBuf, UINT32 *pu32Size);
SAL_STATUS xsp_get_corr_zero(UINT32 u32Chan, UINT8 *pu8CorrBuf, UINT32 *pu32Size);
void Xsp_DumpSet(UINT32 u32Chan, BOOL bEnable, CHAR *pchDumpDir, UINT32 u32DumpDp, UINT32 u32DumpCnt);
SAL_STATUS Xsp_DrvGetPseudoBlankData(UINT32 chan, UINT16 *pu16Buf, UINT32 u32Width, UINT32 u32Height, BOOL bPlanarFormat);
void Xsp_ShowPb2Xpack(UINT32 chan);
void Xsp_ShowTime(UINT32 chan, UINT32 u32PrintType);
void Xsp_ShowIdentify(UINT32 chan);
void Xsp_ShowSliceCb(UINT32 chan);
SAL_STATUS Xsp_DrvSetRotateAndMirror(UINT32 u32Chn, XSP_HANDLE_TYPE enHandleType, UINT32 u32Direction, XRAY_PROC_TYPE enType);
SAL_STATUS Xsp_DrvConvertMirror(UINT32 u32Chn, XSP_HANDLE_TYPE enHandleType);
SAL_STATUS Xsp_DrvChangeSpeed(UINT32 u32Chan, XRAY_PROC_FORM enForm, XRAY_PROC_SPEED enSpeed);

/**
 * @function   Xsp_WaitProcListEmpty
 * @brief      等待XSP处理队列中非Ready状态节点清空，即XSP_PSTG_PROCING节点数为1，XSP_PSTG_PROCED和XSP_PSTG_SENDING节点数都为0
 * @param[in]  UINT32 u32Chn            算法通道号
 * @param[in]  BOOL   bAllClear         是否是队列完全清空，SAL_TRUE-完全清空，SAL_FALSE-仅清空处理完成状态之后的节点
 * @param[in]  INT32  s32TryCnt         最大尝试次数
 * @param[in]  UINT32 u32SleepMsOnce    每次休眠时间(ms)
 * @param[out] None
 * @return     SAL_STATUS 成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Xsp_WaitProcListEmpty(UINT32 chan, BOOL bAllClear, UINT32 u32MaxTryCnt, UINT32 u32SleepMsOnce);

/**
 * @function   Xsp_WaitDispBufferDescend
 * @brief      等待xpack显示缓存中未显示数据量减小到不大于s32Target的指定值
 * @param[in]  UINT32 u32Chn         算法通道号
 * @param[in]  UINT32 u32MaxTryCnt   最大尝试次数
 * @param[in]  UINT32 u32SleepMsOnce 每次休眠时间(ms)
 * @param[in]  INT32  s32Target       目标最大最大缓存数据量
 * @param[out] None
 * @return     SAL_STATUS 成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Xsp_WaitDispBufferDescend(UINT32 chan, UINT32 u32MaxTryCnt, UINT32 u32SleepMsOnce, INT32 s32Target);

/**
 * @fn        xsp_cal_need_img_buf_size
 * @brief     计算存放一张jpeg或bmp、gif图像需要的内存大小
 * @param[IN] XRAY_TRANS_TYPE enImgType    RAW转换的相关参数
 * @param[IN] UINT32          u32ImgWidth  智能识别信息
 * @param[IN] UINT32          u32ImgHeight 智能识别信息
 * @return  UINT32 需要的内存大小，0表示失败
 */
UINT32 xsp_cal_need_img_buf_size(XRAY_TRANS_TYPE enImgType, UINT32 u32ImgWidth, UINT32 u32ImgHeight);

/**
 * @fn            xsp_trans_raw2img
 * @brief         归一化RAW转换成JPG或者BMP图片
 * @param[IN/OUT] XSP_TRANS_PARAM pstTransParam RAW转换的相关参数
 * @param[IN]     SVA_DSP_OUT     pstSvaInfo    智能识别信息
 * @return  SAL_STATUS SAL_SOK-转换成功，SAL_FALSE-转换失败
 */
SAL_STATUS xsp_trans_raw2img(U32 u32Chn, XSP_TRANS_PARAM *pstTransParam, SVA_DSP_OUT *pstSvaInfo);

#endif

