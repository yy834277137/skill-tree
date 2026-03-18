
/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "system_prm_api.h"
#include "xpack_drv.h"
#include "dup_tsk.h"
#include "sal_timer.h"
#include "osd_drv_api.h"
#include "drawChar.h"
#include "debug_time.h"

#line __LINE__ "xpack_drv.c"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define XPACK_THRESH_FRAME_NUM          (120)   // XPACK显示进入新数据缓存的静止帧数阈值
#define XPACK_PACK_SEGCB_STORE_NUM      (8)     /* 包裹缓存个数 */
#define XAPCK_SEGMENT_DEFAULT_WIDTH     (32)
#define XAPCK_SEGMENT_MAX_WIDTH	        (128)
#define XAPCK_SEGMENT_TIMEOUT           (200)

#define XPACK_PACK_JPGCB_NODE_NUM       (4)     /* 包裹jpeg编码队列缓存个数 */
#define XPACK_JPG_MAX_WIDTH             (2496)
#define XPACK_JPG_MAX_HEIGHT            (1280)
#define XPACK_JPG_MIN_WIDTH             (672)
#define XPACK_JPG_MIN_HEIGHT            (672)
#define XPACK_JPG_DEFAULT_WIDTH         (1280)
#define XPACK_JPG_DEFAULT_HEIGHT        (1024)

#define XPACK_PACKAGE_ONSCN_NUM_MAX     (32)    /* 整屏图像（On SCreen）上最多允许的包裹数，按1920/64计算 */
#define XPACK_PACKAGE_AIDGR_NUM_MAX     (4)     /* 当前正在或等待智能识别（AI Recognize）的最多包裹数 */
#define XPACK_PACKAGE_XRIDT_NUM_MAX     (4)     /* 当前正在或等待XRay识别（XRay Identify）的最多包裹数，该类型数据也用于回调的包裹Raw数据 */

#define XPACK_TOI_REGION_MIN            (8*8)   // 最小目标尺寸，8*8Pixels，AiDgr和XrIdt输出的识别区域若小于该值，则丢弃
#define XPACK_OSD_TEXT_FONT_NUM         (16)    // 单个文本OSD中字体最多单元数，注：每个字体单元可包含多个汉字或英文字符，它是一个组合
#define XPACK_OSD_TEXT_HOR_INTERVAL     (8)     // 相邻文本OSD间隔，单位：Pixel
#define XPACK_OSD_TARGET_LABEL_RECT     (6)     // 标识目标的矩形大小，必须2对齐，单位：Pixel，即用6×6的矩形标记目标
#define XPACK_OSD_DISP_EXPAND_MAX       (320)   // 显示屏幕上包裹右侧最多可扩展OSD的区域宽度

/* XPACK处理模块通道号校验 */
#define XPACK_CHECK_CHAN(chan, value)  \
    do { \
        if (chan >= capb_get_xrayin()->xray_in_chan_cnt) \
        { \
            XPACK_LOGE("Invalid chan(%d), should be less than %u!\n", chan, capb_get_xrayin()->xray_in_chan_cnt); \
            return (value); \
        } \
    } while (0)

/* XPACK处理模块指针校验 */
#define XPACK_CHECK_PTR_NULL(ptr, errno) \
    do { \
        if (!ptr) \
        { \
            XPACK_LOGE("the '%s' is NULL\n", # ptr); \
            return (errno); \
        } \
    } while (0)

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/* XPack显示数据类型，各类型的数据量是完全同步的 */
typedef enum
{
    XPACK_DISPDT_PURE,      /* 前级输入的纯图像数据，用于编码、包裹图片、更新OSD的输入源等，作为基数据不显示输出，必须存在 */
    XPACK_DISPDT_OSD,       /* 在纯图像数据上叠加了OSD，标识危险品、包裹条形码等附件信息 */
    XPACK_DISPDT_ENHANCE,   /* 在纯图像数据上做了特定的增强处理，比如黑白、高穿等，有该类型时，XPACK_DISPDT_OSD类型也必须存在 */
    XPACK_DISPDT_CNT,       /* 目前最多支持3种 */
} XPACK_DISP_DATA_TYPE;

/* XPack重画OSD的方式，值越大，优先级越高 */
typedef enum
{
    XPACK_OSD_REDRAW_NONE,      // 无重新画的操作
    XPACK_OSD_REDRAW_DIRECTLY,  // 直接在图像上重新画
    XPACK_OSD_REDRAW_CLEAROLD,  // 需清除原有的OSD（即复制一幅无OSD的纯图像），然后再重新画
} XPACK_OSD_REDRAW_MODE;

/* XPack显示OSD的等级，值越大，优先级越高 */
typedef enum
{
    XPACK_SHOW_OSD_NONE,        // 全部智能信息都不显示（tip，ai，xsp）
    XPACK_SHOW_OSD_TIP,         // 仅显示tip信息，不显示ai和xsp识别信息
    XPACK_SHOW_OSD_ALL,         // 全部智能信息都显示（tip，ai，xsp）
} XPACK_SHOW_OSD_MODE;

/* Xpack dump数据类型 */
typedef union 
{
    struct XPACK_DUMP_TYPE_ST
    {
        UINT32 XPACK_DP_FSCRAW      : 1;     /* 全屏tip-raw数据 */
        UINT32 XPACK_DP_SENDRAW     : 1;     /* xsp传输的raw数据 */
        UINT32 XPACK_DP_SENDDISP    : 1;     /* xsp传输的显示数据 */
        UINT32 XPACK_DP_SENDYUV     : 1;     /* xsp传输的yuv数据 */

        UINT32 XPACK_DP_SEGCB       : 1;     /* 包裹分片回调数据 */
        UINT32 XPACK_DP_PACKJPG     : 1;     /* 包裹回调编码各过程数据 */
        UINT32 XPACK_DP_SCNSHOT     : 1;     /* 送vpss显示数据 */
        UINT32 XPACK_DP_PACKOSD     : 1;     /* Draw OSD前后的包裹图像 */

        UINT32 XPACK_DP_PACKAI      : 1;     /* 送智能识别的包裹图像 */
        UINT32 XPACK_DP_RES         : 23;    /* 保留 */
    } stXpackDumpType;
    UINT32 u32XpackDumpType;
} XPACK_DUMP_TYPE_UN;

typedef struct
{
    CHAR chDumpDir[64];            /* 数据存储目录 */
    XPACK_DUMP_TYPE_UN unDumpDp;   /* 按位表示 */
    UINT32 u32DumpCnt;
} XPACK_DUMP_CFG;

typedef struct xpackSendDupThreadPrm
{
    UINT32 chan;
    INT32 fd;                                   /*高精度定时器句柄*/;
    UINT64 framePts;
    XPACK_DISP_DATA_TYPE enSendDataType;

    void *mChnMutexHdl;                         /* 通道信号量*/
    SAL_ThrHndl stChnThrHandl;                  /* 通道线程 */
    DupHandle pDupHandle;
    SYSTEM_FRAME_INFO stGetPutFrame0;

    debug_time pSendDupDTime;
} XPACK_SENDDUP_PRM;

/* 包裹jpeg编码线程参数 */
typedef struct 
{
    BOOL bJpegEncFinished;                  // Jpeg编码是否已完成
    COND_T condJpegEnc;                     // 控制送显数据缓存同步
    UINT32 PackId;
    UINT32 u32InClipTop;                    // 图像裁剪上边界
    UINT32 u32InClipBot;                    // 图像裁剪下边界
    XIMAGE_DATA_ST *pstImg;                 // 需编码成jpeg的原始图像
    XPACK_SVA_RESULT_OUT *pstAiDgrResult;   // 包裹危险品识别结果
    XSP_RESULT_DATA *pstXspIdtResult;       // XSP输出的图像识别信息，包括难穿透、可疑有机物等
    XPACK_COOR_TRANS *pstJpgCoorTrans;      // jpeg图像坐标转换信息，用于外部坐标转换
    XPACK_SVA_RESULT_OUT *pstJpegSvaOut;    // jpeg图像转换后的智能坐标信息
} PACK_JPEG_ENC_PRM;

typedef struct tagXpackJpgAddrSt
{
    UINT32 u32JpegWidth;                    /* jpeg图像宽 */
    UINT32 u32JpegHeight;                   /* jpeg图像高 */
    UINT32 u32JpegSize;                     /* jpeg图像尺寸 */
    void *pPicJpeg;                         /* JPG图像数据缓冲 */

    BOOL bJpgDrawSwitch;                    /* 画智能识别框开关，0-关闭，1-开启，默认关闭 */
    BOOL bJpgCropSwitch;                    /* 包裹抠取开关，去上下空白，0-关闭，1-开启，默认开启 */
    FLOAT32 f32WidthResizeRatio;            /* 横向缩放比例，范围[0.1, 1.0]，默认值为0.75 */
    FLOAT32 f32HeightResizeRatio;           /* 纵向缩放比例，范围[0.1, 1.0]，默认值为0.75 */
    UINT32 u32BackW;                        /* 应用设置的固定抓图宽 */
    UINT32 u32BackH;                        /* 应用设置的固定抓图高 */

    XIMAGE_DATA_ST stImgResized;            /* 包裹图像缩放缓冲 */
    XIMAGE_DATA_ST stImgFilled;             /* 包裹图像填充背板缓冲 */
    SAL_ThrHndl stJpegEncThrHandl;
    DSA_LIST *lstJpegEnc;                   /*  */
    PACK_JPEG_ENC_PRM astJpgEncNode[XPACK_PACK_JPGCB_NODE_NUM];

} PACK_JPG_CB_CTRL;

/*
 * Xpack视角通道参数
 */
typedef struct 
{
    BOOL bIfRefresh;                        /* 是否有重复全屏处理数据，全屏处理时置位，包裹分片回调时复位 */
    XRAY_PROC_DIRECTION enRtDirection;      /* 当前实时预览的过包显示方向 */
    XRAY_PROC_DIRECTION enPbDirection;      /* 仅记录条带回拉的回拉方向，Xpack_DrvSendPbSegORefresh更新 */
    XRAY_PROC_STATUS_E enProcStat;          /* 记录当前是过包/回拉状态 */
    UINT32 u32DispBgColor;                  /* 屏幕显示背景色 */
    UINT32 u32DispVertOffset;               /* 显示画面拖动纵向偏移量 */
    XPACK_SHOW_OSD_MODE enOsdShowMode;      /* 危险品识别与XSP难穿透&可疑有机物的OSD是否显示 */
    XPACK_OSD_REDRAW_MODE enOsdRedraw4Disp; /* 给显示图像重新画OSD的方式 */
    pthread_mutex_t mutexOsdDraw;           /* 同步和异步画OSD不可并行 */
    XIMAGE_DATA_ST stSaveDispBuf;           /* save的全屏图像缓存 */

    XPACK_SENDDUP_PRM stSendDupPrm[XPACK_DISPDT_CNT];
    PACK_JPG_CB_CTRL stPackJpgCbCtrl;
    XPACK_DUMP_CFG stDumpCfg;
} XPACK_CHN_PRM;

/*
 * Xpack全局状态参数
 */
typedef struct tagXpackGlobalPrm
{
    UINT32 xpack_fps;               /* 源输出帧率 */
    UINT32 u32RefreshCol;           /* 每帧刷新列数 */
    UINT32 u32DispSyncTime;         /* 显示同步超时时间，单视角为0，双视角可配置，单位：ms */
    COND_T condDispCacheSync[XPACK_DISPDT_CNT]; /* 控制送显数据缓存同步 */

    XPACK_CHN_PRM astXpackChnPrm[MAX_XRAY_CHAN];
} XPACK_GLOBAL_PRM;

/*
    包裹分片信息
 */
typedef struct tagPackSegmentPrm
{
    UINT32 cnttime;                 /* 超时计数，用于包裹中间停止复位 */
    UINT32 flag;                    /* 包裹位置 0-开始，1-中间，2-结束 */
    UINT32 uiIsForcedToSeparate;    /* 是否是强制分割的包裹，1-是，0-不是 */
    UINT32 u32PackIndx;             /* 当前包裹序号 */
    UINT32 u32PackWidth;            /* 当前包裹宽度 */
    UINT64 u64DispDataLoc;          /* 包裹起始在显示缓存中的位置 */
    UINT32 u32RawDataLoc;           /* 包裹起始在全屏raw缓存中的位置 */
} XPACK_PACK_SEGMENT_PRM;

typedef struct tagXpackSegmentJpgCtrl
{
    UINT32 enable;                      /* 是否是能包裹分片回调 */
    UINT32 u32SegmentWidth;             /* 包裹分片宽度 */
    XPACK_SAVE_TYPE_E enSegmentType;    /* 分片编码类型 */
    void *mChnMutexHdl;                 /* 通道信号量*/
    SAL_ThrHndl stChnThrHandl;          /* 通道线程 */
    UINT32 u32CurSegInfoIdx;            /* 当前保存的分片信息索引 */
    XPACK_PACK_SEGMENT_PRM astPackSegInfo[XPACK_PACK_SEGCB_STORE_NUM];
} XPACK_SEGMENT_CB_CTRL;


typedef enum
{
    XPACK_XAI_STAGE_NONE,       // 尚未开始
    XPACK_XAI_STAGE_ACQDATA,    // 采集数据中
    XPACK_XAI_STAGE_START,      // 识别已开始
    XPACK_XAI_STAGE_END,        // 已获取识别结果
    XPACK_XAI_STAGE_OSD,        // 已画识别结果的OSD
} XPACK_XAI_STAGE;

/**
 * 显示图像，宽：从RING_SCAN_BUFF (u64ReadDataLoc-s32ScreenWidth)开始，到u64WriteDataLoc结束 
 *         高：CAPB_DISP->disp_yuv_h_max，包括上下Padding与Blanking
 * 显示区域，宽：与显示图像一致 
 *         高：从(拖动偏移+全局放大纵坐标)开始，与全局放大高度一致 
 */
/* 当前显示图像上各包裹位置及其属性 */
typedef struct
{
    UINT32 u32Id;           // 包裹ID，从1开始计数
    BOOL bCompleted;        // 包裹是否已完整
    BOOL bJpegEncStart;     // 包裹是否已开始JPG编码
    BOOL bStoredCb;         // 包裹是否已回调存储
    UINT64 u64GenerateTime; // 包裹生成时间，以包裹结束的时间为准，单位：ms
    UINT64 u64DispLocStart; // 记录包裹在Display Buffer中的开始位置，是从0开始的列数累加值
                            // 回拉模式，以正向回拉为Loc的增长方向，即回拉的start与end正好与过包的end与start相反
    UINT64 u64DispLocEnd;   // 记录包裹在Display Buffer中的结束位置，是从0开始的列数累加值
    UINT32 u32RawWidth;     // 包裹宽，基于NRaw，有超分则为超分后，与显示图像高一致
    UINT32 u32RawHeight;    // 包裹高，基于NRaw，有超分则为超分后，与显示图像宽一致

    INT32 s32AiDgrBufIdx;   // stAiDgrImg的索引，-1为无效值，危险品识别关闭时，不申请该Buffer
    UINT32 u32AiDgrWTotal;  // 用于AI危险品识别的包裹宽度，基于YUV Image
    UINT32 u32AiDgrWSend;   // 当前包裹已发起AI危险品识别的宽度，0表示还未发起识别，该值永远不会超过包裹的宽度u32AiDgrWTotal
    UINT32 u32AiDgrWRecv;   // 当前包裹已做完AI危险品识别的宽度，该值永远不会超过u32AiDgrWSend
    UINT32 u32AiDgrWOsd;    // 当前包裹已画AI危险品OSD的宽度，该值永远不会超过u32AiDgrWRecv
    INT32 s32XrIdtBufIdx;   // stXrIdtBuf的索引，-1为无效值，XSP难穿透&可疑有机物识别关闭时，不申请该Buffer
    XPACK_XAI_STAGE enXrIdtStage;           // 当前包裹XSP难穿透&可疑有机物识别状态
    XPACK_XAI_STAGE enTipStage;             // 当前TIP标识状态
    XPACK_OSD_REDRAW_MODE enSyncRedrawMode; // 包裹同步画OSD模式时的画OSD方式
    VOID *pTimerHandle;                     // 画TIP标识的定时器

    XSP_RESULT_DATA stXspIdtResult; // XSP输出的图像识别信息，包括难穿透、可疑有机物等
    XSP_TRANSFER_INFO stPackDivInfo; // 包裹分割相关信息
    XPACK_SVA_RESULT_OUT stAiDgrResult; // 包裹危险品识别结果
    XPACK_PACKAGE_LABEL stLabelTip; // 包裹的Tip标签信息
}XPACK_PACKAGE_ATTR;

/* 当前显示区域内各包裹OSD信息，OSD会随着窗口放大、移动等一起变化 */
typedef struct
{
    UINT32 u32Id; // 包裹ID，仅调试使用
    XIMAGE_DATA_ST stImgOsd; // 用于画OSD的图像数据，其中横向区域内中有且仅有一个包裹（半个包裹也算一个，其他区域会认为空白），否则OSD会画到其他包裹上
    UINT64 u64CropLoc; // 显示区域ImgOsd在显示图像中的位置索引（即从该位置开始截取ImgOsd图像数据）
    XIMG_RECT stDispArea; // 显示区域ImgOsd在显示图像中的位置，Y基于disp_yuv_h_max，Width包含一个完整包裹，是实际显示区域的扩展
    XIMG_RECT stXScanArea; // X光扫描图占显示区域stImg图像中的区域，除去padding、blanking和包裹右侧的空白
    XIMG_BORDER stProfileBorder; // 包裹外轮廓边界，基于stXScanArea，去除X光扫描图中的上下空白，当前仅Top和Bottom有效
    XSP_RESULT_DATA stXspIdtResult; // XSP输出的图像识别信息，包括难穿透、可疑有机物等，相对于stXScanArea的归一化坐标
    XPACK_SVA_RESULT_OUT stAiDgrResult; // 包裹危险品识别结果，相对于stXScanArea的归一化坐标
    XPACK_PACKAGE_LABEL stLabelTip; // 包裹的Tip标签信息
    XPACK_OSD_REDRAW_MODE enOsdRedrawMode; // 给单个包裹图像重新画OSD的方式，增加OSD为XPACK_OSD_REDRAW_NONE，直接画一次所有的OSD为XPACK_OSD_REDRAW_DIRECTLY
}XPACK_DRAW_ONDISP;

typedef struct
{
    UINT32 u32IdLastest; // 最新的包裹ID，从1开始计数，各通道独立
    XPACK_PACKAGE_ATTR stPackageAttr[XPACK_PACKAGE_ONSCN_NUM_MAX]; // 包裹属性，记录显示图像上所有的包裹
    DSA_LIST *lstPackage; // 包裹队列，Loc大（实时预览较新的包裹、正向回拉新的包裹）的在队列尾，Loc小的在队列头
    XIMAGE_DATA_ST stAiDgrBuf[XPACK_PACKAGE_AIDGR_NUM_MAX]; // AI Dangerous Goods Recognition Buffer, NV21格式
    XIMAGE_DATA_ST stXrIdtBuf[XPACK_PACKAGE_XRIDT_NUM_MAX]; // XRay Identify Buffer, DSP_IMG_DATFMT_SE16或DSP_IMG_DATFMT_LHZP格式
    XPACK_DRAW_ONDISP stDrawOnDisp[XPACK_PACKAGE_ONSCN_NUM_MAX]; // 显示区域内所有包裹的OSD信息
}XPACK_PACKAGE_CTRL;


typedef struct
{
    BOOL bConfidence;               // 是否叠加置信度
    BOOL bMergeSameName;            // 是否合并相同名字的危险品，注：跟随模式不支持合并
    UINT32 u32Color;                // 危险品颜色，默认颜色{deepred-0xB81612，blue-0x0000FF，yellow-0xFFFF00, green-0x00FF00}
    UINT32 u32ScaleLevel;           // 字体大小等级，取值范围{1, 2, 3}，1-小，2-中，3-大
    SVA_BORDER_TYPE_E enDrawMode;   // 目标展现方式，取值范围{0, 1, 2, 3}，0-跟随，1-连线，2-点线，3-分离
    DISPLINE_PRM stAiDgrLine;       // 危险品线的展现形式
    CHAR sAiDgrName[SVA_MAX_ALARM_TYPE][SVA_ALERT_NAME_LEN]; // 危险品名称
} XPACK_OSD_CONFIG_AIDGR;


/**
 * 注： 
 * 只画框，无OSD文本，选择0-跟随模式，u32TextFontCnt置0
 * 只画OSD文本，不画框，选择3-分离模式，stBoxRegion的宽高填0，X坐标为文本的横向偏移
 */
typedef struct
{
    SVA_BORDER_TYPE_E enDrawMode; // 目标展现方式，取值范围{0, 1, 2, 3}，0-跟随，1-连线，2-点线，3-分离
    UINT32 u32TextFontCnt; // 有效的字体单元数，注：每个字体单元可包含多个汉字或英文字符，一个字体单元的表现形式为一幅点阵二维图
    OSD_REGION_S *pstTextFont[XPACK_OSD_TEXT_FONT_NUM]; // 各字体单元点阵二维图指针，组成了OSD文本
    UINT32 u32TextColor; // OSD文本颜色，BGRA32格式，B在低位，A在高位
    BOOL bTextOnTop; // OSD文本是否置顶，置顶时OSD文本独占一行，上下空白区域跳过置顶OSD文本高，非跟随模式才支持
    XIMG_RECT stTextRegion; // OSD文本区域，矩形
    XIMG_RECT stBoxRegion; // 目标区域，矩形
    XIMG_LINE stLine; // 目标区域与OSD文本区域的连线
} XPACK_OSD_TOI_LAYOUT;

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
XPACK_GLOBAL_PRM gGlobalPrm = {0};
XPACK_PACKAGE_CTRL gstRtPackageCtrl[MAX_XRAY_CHAN] = {0}; // 实时预览包裹管理数据结构，新出现的包裹插入到队列尾，Loc较大
XPACK_PACKAGE_CTRL gstPbPackageCtrl[MAX_XRAY_CHAN] = {0}; // 回拉包裹管理数据结构，正向回拉（即实时预览较旧）的包裹插入到队列尾，Loc较大
DISPLINE_PRM gstAiDgrLineParam[MAX_XRAY_CHAN] = {0};      // Xpack画OSD线条相关参数配置

ring_buf_handle gXpackRingBufHandle[XPACK_CHN_MAX][XPACK_DISPDT_CNT] = {NULL};
segment_buf_handle gRtTipRawSegBufHandle[XPACK_CHN_MAX] = {NULL};
segment_buf_handle gRtBlendSegBufHandle[XPACK_CHN_MAX] = {NULL};
segment_buf_handle gPbRawSegBufHandle[XPACK_CHN_MAX] = {NULL};
segment_buf_handle gPbBlendSegBufHandle[XPACK_CHN_MAX] = {NULL};
XPACK_SEGMENT_CB_CTRL gXpackSegmentPicCtrl[XPACK_CHN_MAX] = {0};

/* dup句柄信息，外部定义*/
extern DupHandle dupHandle1[MAX_XRAY_CHAN];
extern DupHandle dupHandle2[MAX_XRAY_CHAN];

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

static VOID xpack_package_clear_rtlst(UINT32 chan);
static VOID xpack_osd_tip_start_drawing(INT32 chan_packid);
static VOID xpack_package_launch_xridt(UINT32 u32Chan, XPACK_PACKAGE_ATTR *pstPkgAttr);
static SAL_STATUS xpack_update_segment_cb_info(UINT32 chan, UINT32 u32PackId, XSP_PACKAGE_TAG enPackTag, UINT32 u32Width, BOOL bForceDiv, UINT64 u64DispDataLoc, UINT32 u32RawDataLoc);
static VOID xpack_osd_draw_on_screen(UINT32 chan, RECT *pstGlobalZoomArea, BOOL bSyncUpdate, XPACK_OSD_REDRAW_MODE enOsdRedraw4Disp);

extern SAL_STATUS disp_tskGetDptzoffset(UINT32 chan, FLOAT32 *fZoomRatio, RECT *pstDispArea);
extern BOOL Xsp_DrvPackageIdentifyIsRunning(UINT32 u32Chan);
extern SAL_STATUS Xsp_DrvPackageIdentifySend(UINT32 u32Chan, XIMAGE_DATA_ST *pstPackData, SAL_SYSFRAME_PRIVATE *pstXPackPriv, UINT32 u32Timeout);
extern SAL_STATUS Xsp_DrvPackageIdentifyGetResults(UINT32 u32Chan, XSP_RESULT_DATA *pstIdtResults, SAL_SYSFRAME_PRIVATE *pstXPackPriv, UINT32 u32Timeout);
extern XRAY_PROC_SPEED xsp_get_current_rtspeed(UINT32 u32Chan);

/**
 * @fn      xpack_media_buf_alloc
 * @brief   根据内存大小或图像格式与宽高申请媒体Buffer
 * 
 * @param   [IN/OUT] pstMbAttr 媒体Buffer属性，输入参数可选①类或②类，当①类参数非0时，优先使用①类 
 *           ① [IN] memSize 内存大小，单位：Byte
 *           ② [IN] enImgFmt 图像格式，见枚举DSP_IMG_DATFMT
 *           ② [IN] u32Width 图像宽，单位：Pixel
 *           ② [IN] u32Height 图像高，单位：Pixel
 *              [IN] bCached Buffer是否为Cached，非硬件访问一般使用Cached内存，以提高访问速率
 *             [OUT] phyAddr 内存物理地址（有些Soc不支持，该值为0）
 *             [OUT] virAddr 内存虚拟地址
 *             [OUT] poolId 内存池ID，各SOC私有信息，外部仅透传 
 *             [OUT] u64MbBlk Media Buffer block属性，各SOC私有信息，可为block编号，也可为block属性指针，外部仅透传
 *             [OUT] u32Width 图像宽，已做16像素对齐处理（有些Soc要求），仅在输入为②类参数时输出，单位：Pixel
 *             [OUT] u32Height 图像高，已做16像素对齐处理（有些Soc要求），仅在输入为②类参数时输出，单位：Pixel
 *             [OUT] u32Stride 内存跨距，仅在输入为②类参数时输出，单位：Byte，N个平面即N个元素有效
 *             [OUT] u32PitchOffset 图像每个平面场相对于起始的偏移，仅在输入为②类参数时输出，单位：Byte，N个平面即N个元素有效，首个一般为0
 * @param   [IN] szMemName Buffer名，统计内存使用率时使用
 * 
 * @return  SAL_STATUS SAL_SOK：申请成功，SAL_FAIL：申请失败
 */
SAL_STATUS xpack_media_buf_alloc(MEDIA_BUF_ATTR *pstMbAttr, CHAR *szMemName)
{
    INT32 s32Ret = SAL_SOK;
    ALLOC_VB_INFO_S stVbInfo = {0};

    XPACK_CHECK_PTR_NULL(pstMbAttr, SAL_FAIL);

    if (0 == pstMbAttr->memSize)
    {
        XPACK_LOGE("memory size is %d\n", pstMbAttr->memSize);
        return SAL_FAIL;
    }

    s32Ret = mem_hal_vbAlloc(pstMbAttr->memSize, "xpack", szMemName, NULL, pstMbAttr->bCached, &stVbInfo);
    if (s32Ret == SAL_SOK)
    {
        pstMbAttr->poolId = stVbInfo.u32PoolId;
        pstMbAttr->phyAddr = stVbInfo.u64PhysAddr;
        pstMbAttr->virAddr = stVbInfo.pVirAddr;
        pstMbAttr->u64MbBlk = stVbInfo.u64VbBlk;
    }
    else
    {
        XPACK_LOGE("mem_hal_vbAlloc failed, memory size: %u\n", pstMbAttr->memSize);
    }

    return s32Ret;
}

VOID xpack_media_buf_free(MEDIA_BUF_ATTR *pstMbAttr)
{
    if (NULL == pstMbAttr)
    {
        XPACK_LOGE("pstMbAttr is NULL\n");
        return;
    }
    if (NULL == pstMbAttr->virAddr)
    {
        XPACK_LOGE("pstMbAttr->virAddr is NULL\n");
        return;
    }

    if (SAL_SOK != mem_hal_vbFree(pstMbAttr->virAddr, "xpack", "xpack", pstMbAttr->memSize, pstMbAttr->u64MbBlk, pstMbAttr->poolId))
    {
        XPACK_LOGE("mem_hal_vbFree failed, virAddr %p, memSize %u\n", pstMbAttr->virAddr, pstMbAttr->memSize);
    }

    return;
}

static VOID xpack_destroy_sys_frame(SYSTEM_FRAME_INFO *pstSysFrame)
{
    if (NULL != pstSysFrame && 0 != pstSysFrame->uiAppData)
    {
        sys_hal_rleaseVideoFrameInfoSt(pstSysFrame);
    }

    return;
}


static void ximg_rect_fix_bounds_u(XIMG_RECT *pstRectU, UINT32 u32ImgWidth, UINT32 u32ImgHeight)
{
    if (u32ImgWidth < 2 || u32ImgHeight < 2 || NULL == pstRectU)
    {
        XPACK_LOGE("the u32ImgWidth(%u) OR u32ImgHeight(%u) OR pstRectU(%p) is invalid\n", u32ImgWidth, u32ImgHeight, pstRectU);
        return;
    }

    /* 源位置+源区域横向不越界 */
    if (pstRectU->uiX + pstRectU->uiWidth > u32ImgWidth)
    {
        XPACK_LOGW("fix horizontal bounds, img res: %u x %u, rect x width: %u %u -> %u %u\n", u32ImgWidth, u32ImgHeight, 
                   pstRectU->uiX, pstRectU->uiWidth, SAL_MIN(pstRectU->uiX, u32ImgWidth-1), SAL_SUB_SAFE(u32ImgWidth, pstRectU->uiX));
        // 限制X坐标在图像区域内
        pstRectU->uiX = SAL_MIN(pstRectU->uiX, u32ImgWidth-1);

        // 纠正矩形区域宽度
        pstRectU->uiWidth = SAL_SUB_SAFE(u32ImgWidth, pstRectU->uiX);
    }

    /* 源位置+源区域纵向不越界 */
    if (pstRectU->uiY + pstRectU->uiHeight > u32ImgHeight)
    {
        XPACK_LOGW("fix vertical bounds, img res: %u x %u, rect y height: %u %u -> %u %u\n", u32ImgWidth, u32ImgHeight, 
                   pstRectU->uiY, pstRectU->uiHeight, SAL_MIN(pstRectU->uiY, u32ImgHeight-1), SAL_SUB_SAFE(u32ImgHeight, pstRectU->uiY));
        // 限制Y坐标在图像区域内
        pstRectU->uiY = SAL_MIN(pstRectU->uiY, u32ImgHeight-1);

        // 纠正矩形区域宽度
        pstRectU->uiHeight = SAL_SUB_SAFE(u32ImgHeight, pstRectU->uiY);
    }

    return;
}


static void ximg_rect_u2f(SVA_RECT_F *pstRectF, XIMG_RECT *pstRectU, UINT32 u32ImgWidth, UINT32 u32ImgHeight)
{
    XIMG_RECT stRectFixed = {0};

    if (u32ImgWidth < 2 || u32ImgHeight < 2)
    {
        XPACK_LOGE("the u32ImgWidth(%u) OR u32ImgHeight(%u) is invalid\n", u32ImgWidth, u32ImgHeight);
        return;
    }
    if (NULL == pstRectF || NULL == pstRectU)
    {
        XPACK_LOGE("the pstRectF(%p) OR pstRectU(%p) is NULL\n", pstRectF, pstRectU);
        return;
    }

    memcpy(&stRectFixed, pstRectU, sizeof(XIMG_RECT));
    ximg_rect_fix_bounds_u(&stRectFixed, u32ImgWidth, u32ImgHeight);
    pstRectF->x = (FLOAT32)stRectFixed.uiX / u32ImgWidth;
    pstRectF->width = (FLOAT32)stRectFixed.uiWidth / u32ImgWidth;
    pstRectF->y = (FLOAT32)stRectFixed.uiY / u32ImgHeight;
    pstRectF->height = (FLOAT32)stRectFixed.uiHeight / u32ImgHeight;

    return;
}


static void ximg_rect_f2u(XIMG_RECT *pstRectU, SVA_RECT_F *pstRectF, UINT32 u32ImgWidth, UINT32 u32ImgHeight)
{
    if (u32ImgWidth < 2 || u32ImgHeight < 2)
    {
        XPACK_LOGE("the u32ImgWidth(%u) OR u32ImgHeight(%u) is invalid\n", u32ImgWidth, u32ImgHeight);
        return;
    }
    if (NULL == pstRectF || NULL == pstRectU)
    {
        XPACK_LOGE("the pstRectF(%p) OR pstRectU(%p) is NULL\n", pstRectF, pstRectU);
        return;
    }

    pstRectU->uiX = (UINT32)(pstRectF->x * u32ImgWidth);
    pstRectU->uiWidth = (UINT32)(pstRectF->width * u32ImgWidth);
    pstRectU->uiY = (UINT32)(pstRectF->y * u32ImgHeight);
    pstRectU->uiHeight = (UINT32)(pstRectF->height * u32ImgHeight);
    ximg_rect_fix_bounds_u(pstRectU, u32ImgWidth, u32ImgHeight);

    return;
}


static void ximg_rect_fix_bounds_f(SVA_RECT_F *pstRectF, UINT32 u32ImgWidth, UINT32 u32ImgHeight)
{
    XIMG_RECT stRectU = {0};

    if (u32ImgWidth < 2 || u32ImgHeight < 2 || NULL == pstRectF)
    {
        XPACK_LOGE("the u32ImgWidth(%u) OR u32ImgHeight(%u) OR pstRectF(%p) is invalid\n", u32ImgWidth, u32ImgHeight, pstRectF);
        return;
    }

    ximg_rect_f2u(&stRectU, pstRectF, u32ImgWidth, u32ImgHeight);
    pstRectF->x = (FLOAT32)stRectU.uiX / u32ImgWidth;
    pstRectF->width = (FLOAT32)stRectU.uiWidth / u32ImgWidth;
    pstRectF->y = (FLOAT32)stRectU.uiY / u32ImgHeight;
    pstRectF->height = (FLOAT32)stRectU.uiHeight / u32ImgHeight;

    return;
}

/**
 * @function   Xpack_DrvChangeProcType
 * @brief      修改xpack处理状态，进入条带回拉会将预览全屏raw数据缓存拷贝到回拉全屏raw数据缓存中，且若过包不足一屏，会将回拉raw数据缓存移到屏幕另一侧位置
 * @param[IN]  UINT32             chan          视角通道号
 * @param[IN]  XRAY_PROC_STATUS_E enProcStat    目标处理状态
 * @param[IN]  UINT32             u32RefreshCol 每帧刷新数据的列数
 * @param[IN]  UINT32             u32RtLineNum  清屏后预览过包的数据列数
 * @param[OUT] None
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS Xpack_DrvChangeProcType(UINT32 chan, XRAY_PROC_STATUS_E enProcStat, UINT32 u32RefreshCol, UINT32 u32RtLineNum)
{
    UINT32 i = 0;
    UINT64 u64DispWLoc = 0, u64DispWLocOut = 0;
    BOOL bPackageAllIn = SAL_FALSE; // 包裹是否完全在显示图像内
    XIMAGE_DATA_ST stBlankData = {0};
    ring_buf_handle ringBufHandle = NULL;
    RING_BUF_DIR enRingBufDir = RING_DIR_UP;
    RING_BUF_MODE enRingBufMode = RING_MODE_ONEWAY;
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;
    XPACK_PACKAGE_ATTR *pstPkgAttrRt = NULL, *pstPkgAttrPb = NULL;
    DSA_NODE *pNodeRt = NULL, *pNodePb = NULL;
    CAPB_XSP *pstCapXsp = capb_get_xsp();
    CAPB_XPACK *pstCapbXpack = capb_get_xpack();

    XPACK_CHECK_CHAN(chan, SAL_FAIL);
    pstXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];

    if (XRAY_PS_NUM <= enProcStat)
    {
        SAL_LOGE("chan %u, invalid process type[%d]\n", chan, enProcStat);
        return SAL_FAIL;
    }

    if (XRAY_PS_PLAYBACK_SLICE_FAST == enProcStat)
    {
        /* 将过包全屏raw数据更新到回拉全屏缓存中 */
        segment_buf_reset(gPbRawSegBufHandle[chan]);
        segment_buf_reset(gPbBlendSegBufHandle[chan]);
        segment_buf_update(gPbRawSegBufHandle[chan], gRtTipRawSegBufHandle[chan]);
        segment_buf_update(gPbBlendSegBufHandle[chan], gRtBlendSegBufHandle[chan]);

        /* 过包不满一屏时将向回拉全屏raw数据缓存添加空白数据，将图像挤到屏幕另一侧 */
        if (u32RtLineNum < pstCapXsp->xraw_height_resized)
        {
            stBlankData.u32Width = pstCapXsp->xraw_width_resized;
            stBlankData.u32Height = pstCapXsp->xraw_height_resized - u32RtLineNum;
            if (SAL_SOK != segment_buf_put(gPbRawSegBufHandle[chan], &stBlankData, RING_DIR_NONE, NULL))
            {
                XPACK_LOGE("chan %u segment_buf_put Raw failed\n", chan);
                return SAL_FAIL;
            }
            if (SAL_SOK != segment_buf_put(gPbBlendSegBufHandle[chan], &stBlankData, RING_DIR_NONE, NULL))
            {
                XPACK_LOGE("chan %u segment_buf_put Blend failed\n", chan);
                return SAL_FAIL;
            }
        }

        if (ring_buf_size(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], &u64DispWLoc, NULL, &u64DispWLocOut, NULL) < 0)
        {
            XPACK_LOGE("chan %u, ring_buf_size failed\n", chan);
            return SAL_FAIL;
        }

        // 清空回拉包裹管理队列
        SAL_mutexTmLock(&gstPbPackageCtrl[chan].lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (!DSA_LstIsEmpty(gstPbPackageCtrl[chan].lstPackage)) // 非空则删
        {
            DSA_LstPop(gstPbPackageCtrl[chan].lstPackage);
        }

        // 复制实时预览最后一屏的包裹信息到回拉中
        SAL_mutexTmLock(&gstRtPackageCtrl[chan].lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        pNodeRt = DSA_LstGetTail(gstRtPackageCtrl[chan].lstPackage);
        while (NULL != pNodeRt) // 从最新的包裹往前查找，直到其横向区域完全不在显示图像中
        {
            pstPkgAttrRt = (XPACK_PACKAGE_ATTR *)pNodeRt->pAdData;
            if (pstPkgAttrRt->u64DispLocEnd > u64DispWLocOut) // 包裹部分在显示图像中
            {
                if (pstPkgAttrRt->bCompleted)
                {
                    bPackageAllIn = (pstPkgAttrRt->u64DispLocStart >= u64DispWLocOut) ? SAL_TRUE : SAL_FALSE;
                    pNodePb = DSA_LstGetIdleNode(gstPbPackageCtrl[chan].lstPackage);
                    if (NULL != pNodePb)
                    {
                        pstPkgAttrPb = (XPACK_PACKAGE_ATTR *)pNodePb->pAdData;
                        memset(pstPkgAttrPb, 0, sizeof(XPACK_PACKAGE_ATTR));
                        pstPkgAttrPb->u32Id = ++i;
                        pstPkgAttrPb->u64GenerateTime = sal_get_tickcnt();
                        pstPkgAttrPb->u64DispLocStart = u64DispWLoc - pstPkgAttrRt->u64DispLocEnd + stBlankData.u32Height;
                        if (bPackageAllIn)
                        {
                            pstPkgAttrPb->bCompleted = SAL_TRUE;
                            pstPkgAttrPb->u32RawWidth = pstPkgAttrRt->u32RawWidth;
                            pstPkgAttrPb->u32RawHeight = pstPkgAttrRt->u32RawHeight;
                            pstPkgAttrPb->stPackDivInfo.uiPackTop = pstPkgAttrRt->stPackDivInfo.uiPackTop;
                            pstPkgAttrPb->stPackDivInfo.uiPackBottom = pstPkgAttrRt->stPackDivInfo.uiPackBottom;
                            pstPkgAttrPb->u64DispLocEnd = u64DispWLoc - pstPkgAttrRt->u64DispLocStart + stBlankData.u32Height;
                            if (pstPkgAttrRt->u32AiDgrWRecv == pstPkgAttrRt->u32AiDgrWTotal) // 包裹的危险品识别结果已经完整
                            {
                                pstPkgAttrPb->u32AiDgrWTotal = pstPkgAttrRt->u32AiDgrWTotal;
                                pstPkgAttrPb->u32AiDgrWOsd = pstPkgAttrRt->u32AiDgrWOsd;
                                pstPkgAttrPb->u32AiDgrWRecv = pstPkgAttrRt->u32AiDgrWRecv;
                                memcpy(&pstPkgAttrPb->stAiDgrResult, &pstPkgAttrRt->stAiDgrResult, sizeof(XPACK_SVA_RESULT_OUT));
                            }
                            if (XPACK_XAI_STAGE_OSD == pstPkgAttrRt->enXrIdtStage)
                            {
                                pstPkgAttrPb->enXrIdtStage = XPACK_XAI_STAGE_OSD;
                                memcpy(&pstPkgAttrPb->stXspIdtResult, &pstPkgAttrRt->stXspIdtResult, sizeof(XSP_RESULT_DATA));
                            }
                        }
                        else
                        {
                            pstPkgAttrPb->bCompleted = SAL_FALSE;
                            pstPkgAttrPb->u32AiDgrWTotal = 0;
                            pstPkgAttrPb->u64DispLocEnd = 0;
                            pstPkgAttrPb->u32AiDgrWRecv = 0;
                            pstPkgAttrPb->enXrIdtStage = XPACK_XAI_STAGE_NONE;
                        }
                        DSA_LstPush(gstPbPackageCtrl[chan].lstPackage, pNodePb);
                        XPACK_LOGI("xpack chan %u, DispWLoc %llu~%llu, Blank %u, Rt: ID %u, Completed %d, Loc %llu~%llu AllIn %d, Pb: Loc %llu~%llu\n", 
                                   chan, u64DispWLocOut, u64DispWLoc, stBlankData.u32Height, pstPkgAttrRt->u32Id, pstPkgAttrRt->bCompleted, 
                                   pstPkgAttrRt->u64DispLocStart, pstPkgAttrRt->u64DispLocEnd, bPackageAllIn, pstPkgAttrPb->u64DispLocStart, pstPkgAttrPb->u64DispLocEnd);
                    }
                }
            }
            else
            {
                break;
            }
            pNodeRt = pNodeRt->prev;
        }
        SAL_mutexTmUnlock(&gstRtPackageCtrl[chan].lstPackage->sync.mid, __FUNCTION__, __LINE__);
        SAL_mutexTmUnlock(&gstPbPackageCtrl[chan].lstPackage->sync.mid, __FUNCTION__, __LINE__);
    }

    enRingBufDir = (XRAY_DIRECTION_LEFT == pstXpackChnPrm->enRtDirection) ? RING_DIR_UP : RING_DIR_DOWN;
    enRingBufMode = RING_MODE_ONEWAY;
    /* 条带回拉设置循环缓冲方向与过包时相反 */
    if (XRAY_PS_PLAYBACK_SLICE_FAST == enProcStat)
    {
        enRingBufDir = (RING_DIR_UP == enRingBufDir) ? RING_DIR_DOWN : RING_DIR_UP;
        enRingBufMode = RING_MODE_TWOWAY;
    }
    /* 条带回拉和过包/整包回拉互相切换时需要清空缓冲区 */
    if ((XRAY_PS_PLAYBACK_SLICE_FAST == pstXpackChnPrm->enProcStat && 
        (XRAY_PS_RTPREVIEW == enProcStat || XRAY_PS_PLAYBACK_FRAME == enProcStat)) 
        || 
        (XRAY_PS_PLAYBACK_SLICE_FAST == enProcStat && 
        (XRAY_PS_RTPREVIEW == pstXpackChnPrm->enProcStat || XRAY_PS_PLAYBACK_FRAME == pstXpackChnPrm->enProcStat)))
    {
        for (i = 0; i < pstCapbXpack->u32XpackSendChnNum; i++)
        {
            ringBufHandle = gXpackRingBufHandle[chan][i];
            ring_buf_reset(ringBufHandle, enRingBufDir, enRingBufMode, pstXpackChnPrm->u32DispBgColor);
        }
    }

    XPACK_LOGW("chan %u xpack change proc status[0x%x -> 0x%x], refreshCol:%u, rtLineNum:%u\n", 
               chan, pstXpackChnPrm->enProcStat, enProcStat, u32RefreshCol, u32RtLineNum);

    /* 更新全局信息 */
    gGlobalPrm.u32RefreshCol = u32RefreshCol;
    pstXpackChnPrm->enProcStat = enProcStat;

    return SAL_SOK;
}


/**
 * @fn      Xpack_DrvSetDispVertOffset
 * @brief   设置显示纵向的偏移，该偏移是基于整个显示图像的，即disp_yuv_h_max
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstDispVertOffset reset：重置为默认值，即纵向偏移h_top_blanking 
 *                                 offset：基于当前位置的纵向偏移量，小于0则往上偏，大于0则往下偏
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS Xpack_DrvSetDispVertOffset(UINT32 chan, XPACK_YUVSHOW_OFFSET *pstDispVertOffset)
{
    INT32 s32DispVertOffset = 0;
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;

    XPACK_CHECK_CHAN(chan, SAL_FAIL);
    XPACK_CHECK_PTR_NULL(pstDispVertOffset, SAL_FAIL);

    pstXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];
    if (pstDispVertOffset->reset)
    {
        // 默认显示纵向偏移h_top_blanking（基于整个显示图像）
        pstXpackChnPrm->u32DispVertOffset = pstCapbDisp->disp_h_top_blanking;
    }
    else
    {
        s32DispVertOffset = pstXpackChnPrm->u32DispVertOffset;
        s32DispVertOffset += pstDispVertOffset->offset;
        s32DispVertOffset = SAL_CLIP(s32DispVertOffset, 0, pstCapbDisp->disp_h_top_blanking+pstCapbDisp->disp_h_bottom_blanking);
        pstXpackChnPrm->u32DispVertOffset = SAL_align(s32DispVertOffset, 2);
    }

    return SAL_SOK;
}


/**
 * @function   Xpack_DrvSetDispSync
 * @brief      设置为了双视角显示同步的等待超时时间，仅双视角机型有效
 * @param[IN]  UINT32 u32SyncSetTime 同步超时时间
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS Xpack_DrvSetDispSync(UINT32 u32SyncSetTime)
{
    UINT32 u32DispSyncTimeMax = 0;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_XPACK *pstCapbXpack = capb_get_xpack();
    XPACK_GLOBAL_PRM *pstGlobalPrm = &gGlobalPrm;

    if (XRAY_PS_PLAYBACK_MASK & pstGlobalPrm->astXpackChnPrm[0].enProcStat)
    {
        XPACK_LOGE("Setting display sync timeout is only supported in real-time preview mode, but now system is in playback mode\n");
        return SAL_FAIL;
    }
    if (0 != Xpack_DrvGetDispBuffStatus(0))
    {
        XPACK_LOGE("Setting display sync timeout is only supported in idle mode, but now system is in running\n");
        return SAL_FAIL;
    }
    /* 粗略按照显示循环缓冲区循环覆盖一圈所用的时间作为双视角数据同步超时时间最大值 */
    u32DispSyncTimeMax = 1000 * pstCapbXpack->xpack_disp_buffer_w / (pstGlobalPrm->u32RefreshCol * pstGlobalPrm->xpack_fps);
    if (u32SyncSetTime > u32DispSyncTimeMax)
    {
        XPACK_LOGE("Setting display sync timeout is too large, set time[%u ms] should not exceed %u ms(time of overwriting display ring buf)\n", u32SyncSetTime, u32DispSyncTimeMax);
        return SAL_FAIL;
    }

    pstGlobalPrm->u32DispSyncTime = (2 == pstCapbXrayIn->xray_in_chan_cnt) ? u32SyncSetTime : 0;

    XPACK_LOGI("set display sync timeout to %u ms\n", pstGlobalPrm->u32DispSyncTime);

    return SAL_SOK;
}

/**
 * @function   xpack_disp_data_cache
 * @brief      每次中断过包后开始过包时缓存显示数据，双视角同步机制启用时，需双视角都缓存了一定量的数据（一般为8帧刷新数据）才往后送显，
 *             双视角同步机制未启用时，当每个视角各自缓存了一定量的数据后即送显，解决因采传下发数据的时间间隔不稳定、线程调度等导致的卡顿问题
 *             当显示循环缓冲中已无缓存数据时送显的帧作为静止帧，当连续静止帧数超过阈值，则认为是下一次开始过包，缓冲实时条带帧，  
 * @param[in]  UINT32               chan            视角通道号
 * @param[in]  XPACK_DISP_DATA_TYPE emDataType      送显类型
 * @param[in]  UINT32               u32CacheDstCol  缓存的目标数据
 * @param[out] None
 * @return     void *     无
 */
static void xpack_disp_data_cache(UINT32 chan, XPACK_DISP_DATA_TYPE emDataType, UINT32 u32CacheDstCol)
{
    INT32 s32WaitCount = 0;
    UINT32 u32SleepMsOnce = 10;         /* 单次睡眠时间，单位：ms */
    UINT32 u32WaitTime = 0;             /* 缓存数据的超时时间，单位：ms */
    UINT64 t0 = 0, t1 = 0, t2 = 0;
    BOOL bHasData = SAL_FALSE;
    ring_buf_handle pDispRingBuff = gXpackRingBufHandle[chan][emDataType];
    ring_buf_handle pDispRingBuffOtherChan = NULL;
    COND_T *pstCond = NULL;
    XPACK_GLOBAL_PRM *pstGlobalPrm = &gGlobalPrm;

    static BOOL lastStatus[XPACK_CHN_MAX] = {SAL_FALSE};                    /* 上一帧时缓存是否有数据未显示（bHasData状态） */
    static UINT64 u64StillFrmCnt[XPACK_CHN_MAX] = {XPACK_THRESH_FRAME_NUM, XPACK_THRESH_FRAME_NUM}; /* 屏幕未移动情况下刷新的帧数（时间）,通过静止帧数可以计算设备未过包时间 */

    bHasData = (0 < ring_buf_size(pDispRingBuff, NULL, NULL, NULL, NULL)) ? SAL_TRUE : SAL_FALSE;
    /* 上一帧有缓存数据，说明不是开始过包，直接退出 */
    if (SAL_TRUE == lastStatus[chan])
    {
        lastStatus[chan] = bHasData;
        return;
    }
    /* 上一帧缓存没有数据时 */
    else
    {
        lastStatus[chan] = bHasData;
        /* 上一帧和当前帧均没有缓存数据时, 不缓存，静止帧计数加1 */
        if (SAL_FALSE == bHasData)
        {
            u64StillFrmCnt[chan]++;
            return;
        }
        /* 接收到新的过包数据, 静止帧数清空，若静止帧数超过阈值则置进入缓存逻辑 */
        else
        {
            XPACK_LOGW("chan %d comes data, stillFrmCnt:%llu\n", chan, u64StillFrmCnt[chan]);
            if (XPACK_THRESH_FRAME_NUM > u64StillFrmCnt[chan])
            {
                u64StillFrmCnt[chan] = 1;
                /* 静止帧数未达阈值，直接退出 */
                return;
            }
            else
            {
                u64StillFrmCnt[chan] = 1;
                /* 往下执行缓存数据逻辑 */
            }
        }
    }

    pstCond = &pstGlobalPrm->condDispCacheSync[emDataType];
    u32CacheDstCol = SAL_MAX(u32CacheDstCol, 1); /* 至少刷新1列/帧 */
    /* 
     * xpack缓存8帧刷新的数据，若u32DispSyncTime为0，表示无双视角同步，则直接按照基本缓存时间300ms缓存数据
     * 否则按照u32DispSyncTime缓存数据
     */
    u32WaitTime = (0 == pstGlobalPrm->u32DispSyncTime) ? 300 : pstGlobalPrm->u32DispSyncTime;
    s32WaitCount = u32WaitTime / u32SleepMsOnce;
    t0 = sal_get_tickcnt();
    while (ring_buf_size(pDispRingBuff, NULL, NULL, NULL, NULL) < u32CacheDstCol)   /* xpack缓存8帧刷新的数据 */
    {
        if (--s32WaitCount > 0)
        {
            SAL_msleep(u32SleepMsOnce);
        }
        else
        {
            break;
        }
    }
    t1 = sal_get_tickcnt();
    /* 条件变量信号通知另一视角开始送显 */
    if (SAL_SOK == SAL_mutexTmLock(&pstCond->mid, pstGlobalPrm->u32DispSyncTime, __FUNCTION__, __LINE__))
    {
        SAL_CondSignal(pstCond, SAL_COND_ST_BROADCAST,  __FUNCTION__, __LINE__);
        SAL_mutexUnlock(&pstCond->mid);
    }

    /* 若另一视角显示数据不足8帧，则进入条件变量等待，只有双视角能设置u32DispSyncTime，单视角时u32DispSyncTime始终为0 */
    if (0 < pstGlobalPrm->u32DispSyncTime)
    {
        pDispRingBuffOtherChan = gXpackRingBufHandle[1 - chan][emDataType];
        if (SAL_SOK == SAL_mutexTmLock(&pstCond->mid, pstGlobalPrm->u32DispSyncTime, __FUNCTION__, __LINE__))
        {
            if (ring_buf_size(pDispRingBuffOtherChan, NULL, NULL, NULL, NULL) < u32CacheDstCol)
            {
                SAL_CondWait(pstCond, pstGlobalPrm->u32DispSyncTime,  __FUNCTION__, __LINE__);
            }
            SAL_mutexUnlock(&pstCond->mid);
        }
    }
    t2 = sal_get_tickcnt();

    XPACK_LOGW("chan %u, cached disp data, cost time:%llu + %llums, ColNum:[Cached:%u, Dst:%u]\n",
                chan, t1 - t0, t2 - t1, ring_buf_size(pDispRingBuff, NULL, NULL, NULL, NULL), u32CacheDstCol);

    return ;
}

/**
 * @function:   xpack_disp_data_reset
 * @brief:      显示相关缓冲复位
 * @param[in]:  UINT32 chan                   视角通道号
 * @param[in]:  XRAY_PROC_DIRECTION enDispDir 循环缓冲句柄
 * @return:     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
static SAL_STATUS xpack_disp_data_reset(UINT32 chan, XRAY_PROC_DIRECTION enDispDir)
{
    INT32 i = 0;
    ring_buf_handle ringBufHandle = NULL;
    RING_BUF_DIR enRingBufDir = RING_DIR_UP;
    RING_BUF_MODE enRingBufMode = RING_MODE_ONEWAY;
    XPACK_GLOBAL_PRM *pGlobalPrm = &gGlobalPrm;
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;
    CAPB_XPACK *pstCapbXpack = capb_get_xpack();

    XPACK_CHECK_CHAN(chan, SAL_FAIL);

    if (XRAY_DIRECTION_LEFT == enDispDir)
    {
        enRingBufDir = RING_DIR_UP;
    }
    else if (XRAY_DIRECTION_RIGHT == enDispDir)
    {
        enRingBufDir = RING_DIR_DOWN;
    }
    else
    {
        XPACK_LOGE("chan %d invalid disp dir:%d\n", chan, enDispDir);
        return SAL_FAIL;
    }
    pstXpackChnPrm = &pGlobalPrm->astXpackChnPrm[chan];
    enRingBufMode = (XRAY_PS_PLAYBACK_SLICE_FAST == pstXpackChnPrm->enProcStat) ? RING_MODE_TWOWAY : RING_MODE_ONEWAY;

    for (i = 0; i < pstCapbXpack->u32XpackSendChnNum; i++)
    {
        ringBufHandle = gXpackRingBufHandle[chan][i];
        ring_buf_reset(ringBufHandle, enRingBufDir, enRingBufMode, pstXpackChnPrm->u32DispBgColor);
    }

    /* tipRaw/blend缓存清除 */
    segment_buf_reset(gRtTipRawSegBufHandle[chan]);
    segment_buf_reset(gRtBlendSegBufHandle[chan]);
    segment_buf_reset(gPbRawSegBufHandle[chan]);
    segment_buf_reset(gPbBlendSegBufHandle[chan]);

    return SAL_SOK;
}


/**
 * @fn      xpack_disp_clear_osd_by_pure
 * @brief   通过复制一份纯图像数据来清除XPACK_DISPDT_OSD图像上的OSD
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] u32Width 需清除图像的宽度
 * @param   [IN] u64StartLoc 从该Loc开始往后清除
 */
static VOID xpack_disp_clear_osd_by_pure(UINT32 chan, UINT32 u32Width, UINT64 u64StartLoc)
{
    SAL_VideoCrop stDispCrop = {0};
    XIMAGE_DATA_ST stImgPure = {0};
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_DISP *pstCapbDisp = capb_get_disp();

    if (chan >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXrayIn->xray_in_chan_cnt-1);
        return;
    }
    if (0 == u32Width)
    {
        XSP_LOGE("xpack chan %u, the width is 0, do nothing\n", chan);
        return;
    }

    // 从XPACK_DISPDT_PURE拷贝一份纯图像数据，清除在包裹区域外的OSD
    stDispCrop.width = u32Width;
    stDispCrop.height = pstCapbDisp->disp_yuv_h_max;
    if (SAL_SOK == ring_buf_crop(gXpackRingBufHandle[chan][XPACK_DISPDT_PURE], u64StartLoc, 0, &stDispCrop, &stImgPure))
    {
        if (SAL_SOK != ring_buf_update(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], &stImgPure, 
                                       u64StartLoc, RING_REFRESH_NONE, stDispCrop.top))
        {
            XPACK_LOGE("xpack chan %u, ring_buf_update failed, CropLoc %llu, top %u, image w %u h %u\n", 
                       chan, u64StartLoc, stDispCrop.top, stImgPure.u32Width, stImgPure.u32Height);
        }
    }
    else
    {
        XPACK_LOGE("xpack chan %u, ring_buf_crop failed, CropLoc %llu, width %u, height %u\n", 
                   chan, u64StartLoc, stDispCrop.width, stDispCrop.height);
    }

    return;
}


/**
 * @function:       Xpack_DrvGetScreenRaw
 * @brief:          获取缓存的当前屏幕raw数据，仅获取地址，不做数据拷贝
 * @param[IN]:      UINT32       chan            视角通道号
 * @param[OUT]:     SEG_BUF_DATA pstRawBuffPrm   Raw数据信息,过包时为tipraw,回拉时为raw
 * @param[OUT]:     SEG_BUF_DATA pstBlendBuffPrm blend数据信息
 * @param[IN/OUT]:  UINT32       *pu32Length     获取的raw数据高度，非空时传入期望获取的高度，返回实际获取的高度，该值介于屏幕宽度和分段缓存长度之间
 * @return:         SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS Xpack_DrvGetScreenRaw(UINT32 chan, SEG_BUF_DATA *pstRawBuffPrm, SEG_BUF_DATA *pstBlendBuffPrm, UINT32 *pu32Length)
{
    INT32 ret = SAL_SOK;
    segment_buf_handle rawSegHandle = NULL;
    segment_buf_handle blendSegHandle = NULL;
    XPACK_DUMP_CFG *pstDumpCfg = NULL;
    XPACK_GLOBAL_PRM *pGlobalPrm = &gGlobalPrm;

    XPACK_CHECK_PTR_NULL(pstRawBuffPrm, SAL_FAIL);
    XPACK_CHECK_PTR_NULL(pstBlendBuffPrm, SAL_FAIL);
    pstDumpCfg = &pGlobalPrm->astXpackChnPrm[chan].stDumpCfg;

    if (XRAY_PS_RTPREVIEW == pGlobalPrm->astXpackChnPrm[chan].enProcStat)
    {
        rawSegHandle = gRtTipRawSegBufHandle[chan];
        blendSegHandle = gRtBlendSegBufHandle[chan];
    }
    else
    {
        rawSegHandle = gPbRawSegBufHandle[chan];
        blendSegHandle = gPbBlendSegBufHandle[chan];
    }

    ret = segment_buf_get(rawSegHandle, pstRawBuffPrm, pu32Length);
    if (SAL_SOK != ret)
    {
        XPACK_LOGE("chan %u segment_buf_get tip raw failed", chan);
        return SAL_FAIL;
    }
    ret = segment_buf_get(blendSegHandle, pstBlendBuffPrm, pu32Length);
    if (SAL_SOK != ret)
    {
        XPACK_LOGE("chan %u segment_buf_get blend failed", chan);
        return SAL_FAIL;
    }

    if (pstDumpCfg->u32DumpCnt > 0 && pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_FSCRAW)
    {
        ximg_dump(&pstRawBuffPrm->stFscRawData, chan, pstDumpCfg->chDumpDir, "xpack-fsc-raw", 
                 (XRAY_PS_RTPREVIEW == pGlobalPrm->astXpackChnPrm[chan].enProcStat) ? "rt" : "pb", pstDumpCfg->u32DumpCnt);
        ximg_dump(&pstBlendBuffPrm->stFscRawData, chan, pstDumpCfg->chDumpDir, "xpack-fsc-blend", 
                 (XRAY_PS_RTPREVIEW == pGlobalPrm->astXpackChnPrm[chan].enProcStat) ? "rt" : "pb", pstDumpCfg->u32DumpCnt);
        pstDumpCfg->u32DumpCnt = SAL_SUB_SAFE(pstDumpCfg->u32DumpCnt, 1);
    }

    return SAL_SOK;
}

/**
 * @fn      Xpack_DrvGetDispBuffStatus
 * @brief   获取实时过包时显示 Buffer中还未送显的数据量
 * @param   [IN] chan 通道号
 * @return  INT32 SAL_FAIL：获取失败，其他：显示 Buffer中还未送显的数据量
 */
INT32 Xpack_DrvGetDispBuffStatus(UINT32 chan)
{
    INT32 s32DispBufLeftSize = 0;

    XPACK_CHECK_CHAN(chan, SAL_FAIL);

    s32DispBufLeftSize = ring_buf_size(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], NULL, NULL, NULL, NULL);
    if (-1 == s32DispBufLeftSize)
    {
        XPACK_LOGE("chan %d ring_buf_size faild\n", chan);
        return SAL_FAIL;
    }

    return s32DispBufLeftSize;
}


/**
 * @function:   Xpack_DrvClearScreen
 * @brief:      清空当前屏幕所有显示和缓存包裹信息，修改帧率和过包数据刷新速度
 * @param[IN]:  UINT32              chan            视角通道号
 * @param[IN]:  UINT32              u32RefreshCol   每帧刷新数据的列数
 * @param[IN]:  XRAY_PROC_DIRECTION enDispDir       预览过包出图方向
 * @param[OUT]: None
 * @return:     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS Xpack_DrvClearScreen(UINT32 chan, UINT32 u32RefreshCol, XRAY_PROC_DIRECTION enDispDir)
{
    UINT32 u32VideoOutputFps = 0, j = 0;
    XPACK_SEGMENT_CB_CTRL *pSegmentJpgCtrl = NULL;

    XPACK_GLOBAL_PRM *pGlobalPrm = &gGlobalPrm;
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;
    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();

    XPACK_CHECK_CHAN(chan, SAL_FAIL);
    pstXpackChnPrm = &pGlobalPrm->astXpackChnPrm[chan];
    pSegmentJpgCtrl = &gXpackSegmentPicCtrl[chan];

    u32VideoOutputFps = pstDspInfo->stVoInitInfoSt.stVoInfoPrm[0].stDispDevAttr.videoOutputFps;

    /* 设置刷新频率 */
    if (pGlobalPrm->xpack_fps != u32VideoOutputFps)
    {
        SAL_TimerHpSetHz(pstXpackChnPrm->stSendDupPrm[XPACK_DISPDT_PURE].fd, u32VideoOutputFps);
        SAL_TimerHpSetHz(pstXpackChnPrm->stSendDupPrm[XPACK_DISPDT_OSD].fd, u32VideoOutputFps);
        pGlobalPrm->xpack_fps = u32VideoOutputFps;
    }

    /* 复位显示缓存 */
    if (SAL_SOK != xpack_disp_data_reset(chan, enDispDir))
    {
        XPACK_LOGE("chan %d xpack_disp_data_reset clear fail, dir:%d\n", chan, enDispDir);
        return SAL_FAIL;
    }

    /* 更新全局信息 */
    pGlobalPrm->u32RefreshCol = u32RefreshCol;
    pstXpackChnPrm->enRtDirection = enDispDir;

    /* 清空包裹智能信息 */
    xpack_package_clear_rtlst(chan);

    /* 清空分片回调缓存信息 */
    SAL_mutexLock(pSegmentJpgCtrl->mChnMutexHdl);
    for (j = 0; j < XPACK_PACK_SEGCB_STORE_NUM; j++)
    {
        pSegmentJpgCtrl->astPackSegInfo[j].flag = 0;
        pSegmentJpgCtrl->astPackSegInfo[j].u32PackIndx = 0;
        pSegmentJpgCtrl->astPackSegInfo[j].u32PackWidth = 0;
        pSegmentJpgCtrl->astPackSegInfo[j].cnttime = 0;
    }
    SAL_mutexUnlock(pSegmentJpgCtrl->mChnMutexHdl);

    XPACK_LOGW("chan %u xpack clear screen, fps:%d, refreshCol:%u, dir:%d\n", chan, pGlobalPrm->xpack_fps, u32RefreshCol, enDispDir);

    return SAL_SOK;
}

/**
 * @function    xpack_send_data_4_display
 * @brief
 * @param[IN]   UINT32             chan             显示通道号
 * @param[IN]   XPACK_DATA4DISPLAY *pstData4Disp    源数据
 * @param[OUT]  UINT64             *pu64DispDataLoc 显示条带数据在循环缓冲中的位置索引，可以根据此值获取循环缓冲中该索引位置的数据
 * @param[OUT]  UINT32             *pu32RawDataLoc  条带raw数据在分段全屏缓冲中的位置索引，可以根据此值获取分段全屏缓冲中该索引位置的数据
 * @return      SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
static SAL_STATUS xpack_send_data_4_display(UINT32 chan, XPACK_DATA4DISPLAY *pstData4Disp, UINT64 *pu64DispDataLoc, UINT32 *pu32RawDataLoc)
{
    INT32 i = 0;
    XRAY_PROC_DIRECTION enDispDir = XRAY_DIRECTION_NONE;
    RING_BUF_DIR enRingBufDir = RING_DIR_UP;
    ring_buf_handle ringBufHandle = NULL;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_XPACK *pstCapbXpack = capb_get_xpack();
    XPACK_CHN_PRM *pXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];
    XPACK_DUMP_CFG *pstDumpCfg = NULL;

    XPACK_CHECK_PTR_NULL(pstData4Disp, SAL_FAIL);
    XPACK_CHECK_PTR_NULL(pu64DispDataLoc, SAL_FAIL);
    if (chan >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, xray_in_chan_cnt is %d.\n", chan, pstCapbXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }
    if (((NULL != pstData4Disp->stDispData.pPlaneVir[0]) && 
         (0 == pstData4Disp->stDispData.u32Width || 0 == pstData4Disp->stDispData.u32Height || 0 == pstData4Disp->stDispData.u32Stride[0])) || 
        (NULL != pstData4Disp->stRawData.pPlaneVir[0] && 0 == pstData4Disp->stRawData.u32Stride[0]))
    {
        XPACK_LOGE("chan %d invalid data size, w:%d, h:%d, disp_s:%d, raw_s:%u\n", 
                   chan, pstData4Disp->stDispData.u32Width, pstData4Disp->stDispData.u32Height, 
                   pstData4Disp->stDispData.u32Stride[0], pstData4Disp->stRawData.u32Stride[0]);
        return SAL_FAIL;
    }
    enDispDir = pstData4Disp->enDispDir;
    if (XRAY_DIRECTION_LEFT != enDispDir && XRAY_DIRECTION_RIGHT != enDispDir)
    {
        XPACK_LOGE("chan %u, invalild disp dir:%d\n", chan, enDispDir);
        return SAL_FAIL;
    }
    pstDumpCfg = &pXpackChnPrm->stDumpCfg;

    if (XRAY_DIRECTION_LEFT == enDispDir)
    {
        enRingBufDir = RING_DIR_UP;
    }
    else if (XRAY_DIRECTION_RIGHT == enDispDir)
    {
        enRingBufDir = RING_DIR_DOWN;
    }

    /* 拷贝送显示数据 */
    for (i = 0; i < pstCapbXpack->u32XpackSendChnNum; i++)
    {
        ringBufHandle = gXpackRingBufHandle[chan][i];
        if (SAL_TRUE == pstData4Disp->bIfRefresh)
        {
            XPACK_LOGW("xpack chan %u, bIfRefresh should not be TRUE\n", chan);
            pXpackChnPrm->u32DispBgColor = pstData4Disp->u32DispBgColor;
            ring_buf_update(ringBufHandle, &pstData4Disp->stDispData, -1, RING_REFRESH_LEFT, 0);
        }
        else
        {
            *pu64DispDataLoc = ring_buf_put(ringBufHandle, &pstData4Disp->stDispData, enRingBufDir);
            if (-1 == *pu64DispDataLoc)
            {
                *pu64DispDataLoc = -1;
                XPACK_LOGE("chan %u ring_buf_put failed\n", chan);
                return SAL_FAIL;
            }
        }
    }

    /* 拷贝tip-raw数据 */
    if (SAL_SOK != segment_buf_put(gRtTipRawSegBufHandle[chan], &pstData4Disp->stRawData, RING_DIR_NONE, pu32RawDataLoc))
    {
        XPACK_LOGE("chan %u segment_buf_put failed\n", chan);
        return SAL_FAIL;
    }
    /* 拷贝blend灰度数据 */
    if (SAL_SOK != segment_buf_put(gRtBlendSegBufHandle[chan], &pstData4Disp->stBlendData, RING_DIR_NONE, NULL))
    {
        XPACK_LOGE("chan %u segment_buf_put failed\n", chan);
        return SAL_FAIL;
    }

    if (pstDumpCfg->u32DumpCnt > 0)
    {
        if (pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_SENDDISP)
        {
            ximg_dump(&pstData4Disp->stDispData, chan, pstDumpCfg->chDumpDir, "send-disp", NULL, pstDumpCfg->u32DumpCnt);
        }
        if (pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_SENDRAW)
        {
            ximg_dump(&pstData4Disp->stRawData, chan, pstDumpCfg->chDumpDir, "send-tipraw", NULL, pstDumpCfg->u32DumpCnt);
            ximg_dump(&pstData4Disp->stBlendData, chan, pstDumpCfg->chDumpDir, "send-blend", NULL, pstDumpCfg->u32DumpCnt);
        }
    }

    return SAL_SOK;
}

/**
 * @fn      xpack_send_data_4_package
 * @brief   发送包裹条带数据给XPack，用于智能识别与包裹回调存储
 * 
 * @param   [IN] u32Chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstData4Pkg 包裹条带数据与一些必要信息
 * @param   [IN] pstLabelTip Tip标签，仅在为包裹条带时有效，可为NULL
 * @param   [IN] pstPackDiv 包裹分割信息，仅在包裹结束时有效，可为NULL
 * @param   [IN] UINT64 u64DispDataLoc 条带显示数据在循环缓冲中的存储位置索引
 * @param   [IN] UINT32 u32RawDataLoc  条带raw数据在raw数据全屏缓存中的存储位置索引
 * 
 * @return  SAL_STATUS SAL_SOK：发送成功，SAL_FAIL：发送失败
 */
static SAL_STATUS xpack_send_data_4_package(UINT32 u32Chan, XPACK_DATA4PACKAGE_RT *pstData4Pkg, XPACK_PACKAGE_LABEL *pstLabelTip, XSP_TRANSFER_INFO *pstPackDiv, UINT64 u64DispDataLoc, UINT32 u32RawDataLoc)
{
    UINT32 i = 0;
    INT32 ret = SAL_SOK;
    UINT32 u32InRawHeight = 0;       // 超分的raw数据高度
    BOOL bSvaRunning = SAL_FALSE, bOsdEn = SAL_FALSE;
    BOOL bPackageEnd = SAL_FALSE;
    DSA_NODE *pNode = NULL;
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL;
    SAL_VideoCrop stDstArea = {0};
    XIMAGE_DATA_ST *pstAiDgrImg = NULL; // 包裹的AI-YUV Buffer
    XIMAGE_DATA_ST *pstXrIdtBuf = NULL; // 包裹的NRaw Buffer
    XPACK_DUMP_CFG *pstDumpCfg = NULL;

    if (u32Chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", u32Chan, pstCapbXray->xray_in_chan_cnt-1);
        return SAL_FAIL;
    }
    if (NULL == pstData4Pkg)
    {
        XPACK_LOGE("xpack chan %u, pstData4Pkg is NULL\n", u32Chan);
        return SAL_FAIL;
    }
    if (pstData4Pkg->enDir != XRAY_DIRECTION_LEFT && pstData4Pkg->enDir != XRAY_DIRECTION_RIGHT)
    {
        XPACK_LOGE("xpack chan %u, enDir(%d) is invalid\n", u32Chan, pstData4Pkg->enDir);
        return SAL_FAIL;
    }
    if (pstData4Pkg->enPkgTag > XSP_PACKAGE_MIDDLE)
    {
        XPACK_LOGE("xpack chan %u, enPkgTag(%d) is invalid\n", u32Chan, pstData4Pkg->enPkgTag);
        return SAL_FAIL;
    }

    pstPkgCtrl = gstRtPackageCtrl + u32Chan;
    pstDumpCfg = &gGlobalPrm.astXpackChnPrm[u32Chan].stDumpCfg;

    SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    if (NULL != (pNode = DSA_LstGetTail(pstPkgCtrl->lstPackage)))
    {
        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
        /**
         * 异常处理，直接删除该包裹 
         * 1、当前条带为开始或空白，但之前包裹尚未结束 
         * 2、当前条带为中间或结束，但显示方向或镜像发生了变化 
         */
        if (((XSP_PACKAGE_START == pstData4Pkg->enPkgTag || XSP_PACKAGE_NONE == pstData4Pkg->enPkgTag) && (!pstPkgAttr->bCompleted)) ||
            ((XSP_PACKAGE_MIDDLE == pstData4Pkg->enPkgTag || XSP_PACKAGE_END == pstData4Pkg->enPkgTag) && 
             (pstPkgAttr->stPackDivInfo.uiNoramlDirection != pstData4Pkg->enDir || pstPkgAttr->stPackDivInfo.uiIsVerticalFlip != pstData4Pkg->bVMirror)))
        {
            if (pstPkgAttr->s32AiDgrBufIdx > 0)
            {
                pstAiDgrImg = pstPkgCtrl->stAiDgrBuf + pstPkgAttr->s32AiDgrBufIdx;
                pstAiDgrImg->u32Width = 0;
                pstPkgAttr->s32AiDgrBufIdx = -1;
            }
            if (pstPkgAttr->s32XrIdtBufIdx > 0)
            {
                pstXrIdtBuf = pstPkgCtrl->stXrIdtBuf + pstPkgAttr->s32XrIdtBufIdx;
                pstXrIdtBuf->u32Height = 0;
                pstXrIdtBuf->u32Width = 0;
                pstPkgAttr->s32XrIdtBufIdx = -1;
            }
            XPACK_LOGW("xpack chan %u, the lastest package tag: %d, Completed %d, Dir %d, VMirror %d, NEW slice Dir %d, VMirror %d, delete it\n", 
                       u32Chan, pstData4Pkg->enPkgTag, pstPkgAttr->bCompleted, pstPkgAttr->stPackDivInfo.uiNoramlDirection, 
                       pstPkgAttr->stPackDivInfo.uiIsVerticalFlip, pstData4Pkg->enDir, pstData4Pkg->bVMirror);
            DSA_LstDelete(pstPkgCtrl->lstPackage, pNode);
        }
        pstPkgAttr = NULL;
    }

    u32InRawHeight = (UINT32)(pstCapbXsp->resize_height_factor * pstData4Pkg->stXrIdtBuf.u32Height);
    if (XSP_PACKAGE_START == pstData4Pkg->enPkgTag)
    {
        // 申请空闲的节点，创建一个新的包裹
        if (NULL != (pNode = DSA_LstGetIdleNode(pstPkgCtrl->lstPackage)))
        {
            pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
            memset(pstPkgAttr, 0, sizeof(XPACK_PACKAGE_ATTR));
            pstPkgAttr->s32AiDgrBufIdx = -1;
            pstPkgAttr->s32XrIdtBufIdx = -1;
            pstPkgAttr->u32Id = ++pstPkgCtrl->u32IdLastest;
            pstPkgAttr->u32RawWidth = pstCapbXsp->xraw_width_resized;
            pstPkgAttr->u32RawHeight = u32InRawHeight;
            
            pstPkgAttr->stPackDivInfo.uiNoramlDirection = pstData4Pkg->enDir;
            pstPkgAttr->stPackDivInfo.uiIsVerticalFlip = pstData4Pkg->bVMirror;
            pstPkgAttr->u64DispLocStart = u64DispDataLoc + SAL_SUB_SAFE(pstData4Pkg->u32SliceNrawH, u32InRawHeight);
            pstPkgAttr->u64DispLocEnd = u64DispDataLoc + u32InRawHeight;

            if (pstData4Pkg->u32Top < pstData4Pkg->u32Bottom)
            {
                pstPkgAttr->stPackDivInfo.uiPackTop = pstData4Pkg->u32Top;
                pstPkgAttr->stPackDivInfo.uiPackBottom = pstData4Pkg->u32Bottom;
            }
            else // 非法时直接使用最大范围
            {
                pstPkgAttr->stPackDivInfo.uiPackTop = 0;
                pstPkgAttr->stPackDivInfo.uiPackBottom = pstCapbXray->xraw_width_max - 1;
            }

            // SVA启用时，查找空闲的包裹AiDgr Buffer
            if (SAL_SOK == Sva_DrvGetRunState(u32Chan, (UINT32 *)&bSvaRunning, (UINT32 *)&bOsdEn) && bSvaRunning)
            {
                for (i = 0; i < XPACK_PACKAGE_AIDGR_NUM_MAX; i++)
                {
                    pstAiDgrImg = pstPkgCtrl->stAiDgrBuf + i;
                    if (0 == pstAiDgrImg->u32Width && 0 == pstAiDgrImg->u32Height) // 根据包裹AI图像的宽高来判断当前AiDgr Buffer是否存有数据
                    {
                        pstPkgAttr->s32AiDgrBufIdx = i;
                        break;
                    }
                }
            }
            // XSP识别启用时，查找空闲的包裹NRaw Buffer
            for (i = 0; i < XPACK_PACKAGE_XRIDT_NUM_MAX; i++)
            {
                pstXrIdtBuf = pstPkgCtrl->stXrIdtBuf + i;
                if (0 == pstXrIdtBuf->u32Height && 0 == pstXrIdtBuf->u32Width) // 根据包裹NRaw的宽高属性来判断当前XrIdt Nraw Buffer是否存有数据
                {
                    pstPkgAttr->s32XrIdtBufIdx = i; // 空闲状态则占用
                    pstPkgAttr->enXrIdtStage = XPACK_XAI_STAGE_ACQDATA;
                    break;
                }
            }

            DSA_LstPush(pstPkgCtrl->lstPackage, pNode);
        }
        else
        {
            XPACK_LOGE("xpack chan %u, new package failed, cannot get idle node\n", u32Chan);
        }
    }
    else if (XSP_PACKAGE_MIDDLE == pstData4Pkg->enPkgTag)
    {
        if (NULL != (pNode = DSA_LstGetTail(pstPkgCtrl->lstPackage)))
        {
            pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
            if (!pstPkgAttr->bCompleted) // 包裹未结束，继续积累数据
            {
                pstPkgAttr->u32RawHeight += u32InRawHeight;
                pstPkgAttr->u64DispLocEnd = u64DispDataLoc + u32InRawHeight;
                if (pstData4Pkg->u32Top < pstData4Pkg->u32Bottom)
                {
                    pstPkgAttr->stPackDivInfo.uiPackTop = pstData4Pkg->u32Top;
                    pstPkgAttr->stPackDivInfo.uiPackBottom = pstData4Pkg->u32Bottom;
                }
                else // 非法时直接使用最大范围
                {
                    pstPkgAttr->stPackDivInfo.uiPackTop = 0;
                    pstPkgAttr->stPackDivInfo.uiPackBottom = pstCapbXray->xraw_width_max - 1;
                }
            }
        }
    }
    else if (XSP_PACKAGE_END == pstData4Pkg->enPkgTag)
    {
        if (NULL != (pNode = DSA_LstGetTail(pstPkgCtrl->lstPackage)))
        {
            pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
            if (!pstPkgAttr->bCompleted) // 包裹未结束，且该条带是最后一个条带
            {
                pstPkgAttr->u32RawHeight += u32InRawHeight;
                pstPkgAttr->u64DispLocEnd = u64DispDataLoc + u32InRawHeight;
                pstPkgAttr->bCompleted = SAL_TRUE;
                pstPkgAttr->u64GenerateTime = sal_get_tickcnt();
                bPackageEnd = SAL_TRUE;
                if (NULL != pstPackDiv)
                {
                    memcpy(&pstPkgAttr->stPackDivInfo, pstPackDiv, sizeof(XSP_TRANSFER_INFO));
                }
            }
        }
    }
    SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

    if (NULL != pstPkgAttr)
    {
        // 拷贝数据
        if (pstPkgAttr->s32AiDgrBufIdx >= 0)
        {
            pstAiDgrImg = pstPkgCtrl->stAiDgrBuf + pstPkgAttr->s32AiDgrBufIdx;
            // L2R，从Buffer的右端开始，左右镜像拷贝写；R2L，从Buffer的左端开始写，直接拷贝
            if (pstData4Pkg->stAiDgrBuf.u32Width > 0) // 开始、结束条带AI图像宽度可能为0
            {
                stDstArea.top = 0;
                stDstArea.left = pstAiDgrImg->u32Width;
                stDstArea.width = pstData4Pkg->stAiDgrBuf.u32Width;
                stDstArea.height = pstData4Pkg->stAiDgrBuf.u32Height;
                pstAiDgrImg->u32Width += pstData4Pkg->stAiDgrBuf.u32Width;
                pstAiDgrImg->u32Height = pstData4Pkg->stAiDgrBuf.u32Height;
                if (SAL_SOK != ximg_copy_nb(&pstData4Pkg->stAiDgrBuf, pstAiDgrImg, NULL, &stDstArea, 
                                            (pstData4Pkg->enDir == XRAY_DIRECTION_RIGHT) ? XIMG_FLIP_HORZ : XIMG_FLIP_NONE))
                {
                    XPACK_LOGE("xpack chan %u, ximg_copy_nb failed\n", u32Chan);
                }
            }
            pstPkgAttr->u32AiDgrWTotal = pstAiDgrImg->u32Width;
        }
        if (pstPkgAttr->s32XrIdtBufIdx >= 0)
        {
            pstXrIdtBuf = pstPkgCtrl->stXrIdtBuf + pstPkgAttr->s32XrIdtBufIdx;
            // NRaw数据拷贝
            if (pstData4Pkg->stXrIdtBuf.u32Height > 0) // 开始、结束条带高度可能为0
            {
                stDstArea.left = 0;
                stDstArea.top = pstXrIdtBuf->u32Height;
                stDstArea.width = pstData4Pkg->stXrIdtBuf.u32Width;
                stDstArea.height = pstData4Pkg->stXrIdtBuf.u32Height;
                pstXrIdtBuf->u32Width = pstData4Pkg->stXrIdtBuf.u32Width;
                pstXrIdtBuf->u32Height += pstData4Pkg->stXrIdtBuf.u32Height;
                if (SAL_SOK != ximg_copy_nb(&pstData4Pkg->stXrIdtBuf, pstXrIdtBuf, NULL, &stDstArea, XIMG_FLIP_NONE))
                {
                    XPACK_LOGE("xpack chan %u, ximg_copy_nb failed\n", u32Chan);
                }
            }
        }

        // 打印包裹信息
        if (XSP_PACKAGE_START == pstData4Pkg->enPkgTag)
        {
            XPACK_LOGI("xpack chan %u, new package start, ID: %u, AiDgrBufIdx %d-%ux%u, XrIdtBufIdx %d-%ux%u, Dir %d, VMirror %d\n", 
                       u32Chan, pstPkgAttr->u32Id, pstPkgAttr->s32AiDgrBufIdx, (pstPkgAttr->s32AiDgrBufIdx >= 0) ? pstAiDgrImg->u32Width : 0, 
                       (pstPkgAttr->s32AiDgrBufIdx >= 0) ? pstAiDgrImg->u32Height : 0, 
                       pstPkgAttr->s32XrIdtBufIdx, 
                       (pstPkgAttr->s32XrIdtBufIdx >= 0) ? pstXrIdtBuf->u32Width : 0, (pstPkgAttr->s32XrIdtBufIdx >= 0) ? pstXrIdtBuf->u32Height : 0,
                       pstPkgAttr->stPackDivInfo.uiNoramlDirection, pstPkgAttr->stPackDivInfo.uiIsVerticalFlip);
        }
        if (pstPkgAttr->bCompleted && XSP_PACKAGE_END == pstData4Pkg->enPkgTag)
        {
            XPACK_LOGI("xpack chan %u, new package end, ID: %u, Sync %llu, Time %llu, RawWidth %u, RawHeight %u, XrIdtStage %d, AiDgrWidth %u, AiDgrHeight %u\n", 
                       u32Chan, pstPkgAttr->u32Id, pstPkgAttr->stPackDivInfo.uiSyncTime, pstPkgAttr->u64GenerateTime, pstPkgAttr->u32RawWidth, 
                       pstPkgAttr->u32RawHeight, pstPkgAttr->enXrIdtStage, (pstPkgAttr->s32AiDgrBufIdx >= 0) ? pstAiDgrImg->u32Width : 0, 
                       (pstPkgAttr->s32AiDgrBufIdx >= 0) ? pstAiDgrImg->u32Height : 0);
        }

        if (NULL != pstLabelTip) // 创建画TIP标签的定时器，在pstLabelTip->u32DelayTime后开始画OSD
        {
            memcpy(&pstPkgAttr->stLabelTip, pstLabelTip, sizeof(XPACK_PACKAGE_LABEL));

            /* 在包裹完成的下一个条带返回TIP注入成功，pstPkgAttr->u32RawHeight不包含该条带宽度 */
            if (XSP_PACKAGE_NONE == pstData4Pkg->enPkgTag)
            {
                pstPkgAttr->stLabelTip.stRect.uiX -= pstData4Pkg->u32SliceNrawH;
            }
            pstPkgAttr->stLabelTip.stRect.uiX = SAL_SUB_SAFE(pstPkgAttr->u32RawHeight, pstPkgAttr->stLabelTip.stRect.uiX);
            pstPkgAttr->enTipStage = XPACK_XAI_STAGE_START;
            pstPkgAttr->pTimerHandle = SAL_TimerCreate((UINT32)(pstLabelTip->u64DelayTime * 1000), 1, 
                                                       xpack_osd_tip_start_drawing, (INT32)((u32Chan << 28) | (pstPkgAttr->u32Id & 0xFFFFFFF)));
        }
        if (bPackageEnd)
        {
            // 包裹结束发起识别等可能阻塞的操作，放到互斥锁外，提高效率
            xpack_package_launch_xridt(u32Chan, pstPkgAttr); // 发起XSP难穿透&可疑有机物识别
        }

        xpack_update_segment_cb_info(u32Chan, pstPkgAttr->u32Id, pstData4Pkg->enPkgTag, u32InRawHeight, SAL_FALSE, u64DispDataLoc, u32RawDataLoc);
    }

    if (pstDumpCfg->u32DumpCnt > 0)
    {
        if (pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_SENDYUV)
        {
            ximg_dump(&pstData4Pkg->stAiDgrBuf, u32Chan, pstDumpCfg->chDumpDir, "send-aiyuv", NULL, pstDumpCfg->u32DumpCnt);
        }
        if (pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_SENDRAW)
        {
            ximg_dump(&pstData4Pkg->stXrIdtBuf, u32Chan, pstDumpCfg->chDumpDir, "send-slice-raw", NULL, pstDumpCfg->u32DumpCnt);
        }
        if (pstDumpCfg->unDumpDp.u32XpackDumpType & 0xF)
        {
            pstDumpCfg->u32DumpCnt = SAL_SUB_SAFE(pstDumpCfg->u32DumpCnt, 1);
        }
    }

    return ret;
}


/**
 * @fn      Xpack_DrvSendRtSlice
 * @brief   发送条带数据给XPack
 * 
 * @param   [IN] u32Chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] XPACK_DATA4DISPLAY *pstDataSrc 源数据
 * @param   [IN] pstData4Pkg 包裹条带数据与一些必要信息
 * @param   [IN] pstLabelTip Tip标签，仅在为包裹条带时有效，可为NULL
 * @param   [IN] pstPackDiv 包裹分割信息，仅在包裹结束时有效，可为NULL
 * 
 * @return  SAL_STATUS SAL_SOK：发送成功，SAL_FAIL：发送失败
 */
SAL_STATUS Xpack_DrvSendRtSlice(UINT32 chan, XPACK_DATA4DISPLAY *pstData4Disp, XPACK_DATA4PACKAGE_RT *pstData4Pkg, XPACK_PACKAGE_LABEL *pstLabelTip, XSP_TRANSFER_INFO *pstPackDiv)
{
    SAL_STATUS ret = SAL_SOK;
    UINT32 u32RawDataLoc = 0;
    UINT64 u64DispDataLoc = 0;

    XPACK_CHECK_PTR_NULL(pstData4Disp, SAL_FAIL);
    XPACK_CHECK_PTR_NULL(pstData4Pkg, SAL_FAIL);

    ret = xpack_send_data_4_display(chan, pstData4Disp, &u64DispDataLoc, &u32RawDataLoc);
    if (SAL_SOK != ret)
    {
        XPACK_LOGE("chan %u, xpack_send_data_4_display failed\n", chan);
        return SAL_FAIL;
    }
    ret = xpack_send_data_4_package(chan, pstData4Pkg, pstLabelTip, pstPackDiv, u64DispDataLoc, u32RawDataLoc);
    if (SAL_SOK != ret)
    {
        XPACK_LOGE("chan %u, xpack_send_data_4_package failed\n", chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function    Xpack_DrvSendPbSegORefresh
 * @brief       发送回拉段或全屏成像处理的显示数据
 * @param[in]   UINT32              chan        显示通道号
 * @param[in]   XPACK_DATA4DISPLAY  *pstDataSrc 源数据
 * @param[in]   XPACK_DATA4PACKAGE_PB *pstData4Pkg    包裹信息，仅需显示数据时可为NULL
 * @param[out]  NONE
 * @return      SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
SAL_STATUS Xpack_DrvSendPbSegORefresh(UINT32 chan, XPACK_DATA4DISPLAY *pstData4Disp, XPACK_DATA4PACKAGE_PB *pstData4Pkg)
{
    UINT32 i = 0, u32Width = 0;
    UINT64 u64DispRLoc= 0;     // 当前显示的读位置
    UINT64 u64DispDataLoc = 0, u64DispLocStart = 0, u64DispLocEnd = 0;
    XRAY_PROC_DIRECTION enDispDir = XRAY_DIRECTION_NONE;
    RING_BUF_DIR enRingBufDir = RING_DIR_UP;
    RING_BUF_REFRESH_LOC enRingFreshLoc = RING_REFRESH_LEFT;
    ring_buf_handle ringBufHandle = NULL;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_XPACK *pstCapbXpack = capb_get_xpack();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    XPACK_CHN_PRM *pXpackChnPrm = NULL;
    XPACK_DUMP_CFG *pstDumpCfg = NULL;
    SEG_BUF_DATA stRawDumpBuff = {0};
    SEG_BUF_DATA stBlendDumpBuff = {0};
    DSA_NODE *pNode = NULL, *pInst = NULL, *pDel = NULL;
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL;
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;

    XPACK_CHECK_PTR_NULL(pstData4Disp, SAL_FAIL);
    XPACK_CHECK_PTR_NULL(pstData4Disp->stDispData.pPlaneVir[0], SAL_FAIL);

    if (chan >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, xray_in_chan_cnt is %d.\n", chan, pstCapbXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }
    enDispDir = pstData4Disp->enDispDir;
    if (XRAY_DIRECTION_LEFT != enDispDir && XRAY_DIRECTION_RIGHT != enDispDir)
    {
        XPACK_LOGE("chan %u, invalild disp dir:%d\n", chan, enDispDir);
        return SAL_FAIL;
    }
    if (0 == pstData4Disp->stDispData.u32Width || 0 == pstData4Disp->stDispData.u32Height || 0 == pstData4Disp->stDispData.u32Stride[0])
    {
        XPACK_LOGE("chan %d invalid data size, w:%d, h:%d, disp_s:%d\n", 
                   chan, pstData4Disp->stDispData.u32Width, pstData4Disp->stDispData.u32Height, pstData4Disp->stDispData.u32Stride[0]);
        return SAL_FAIL;
    }

    pXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];
    pstDumpCfg = &pXpackChnPrm->stDumpCfg;
    enRingBufDir = (XRAY_DIRECTION_LEFT == enDispDir) ? RING_DIR_UP : RING_DIR_DOWN;

    /* 拷贝送显示数据 */
    for (i = 0; i < pstCapbXpack->u32XpackSendChnNum; i++)
    {
        ringBufHandle = gXpackRingBufHandle[chan][i];
        /* 非全屏处理的条带回拉才是往循环缓冲添加数据，整包回拉和全屏处理是刷新循环缓冲数据 */
        if ((XRAY_PS_PLAYBACK_SLICE_FAST == pXpackChnPrm->enProcStat || XRAY_PS_PLAYBACK_SLICE_XTS == pXpackChnPrm->enProcStat) && 
            (!pstData4Disp->bIfRefresh))
        {
            if (-1 == (u64DispDataLoc = ring_buf_put(ringBufHandle, &pstData4Disp->stDispData, enRingBufDir)))
            {
                XPACK_LOGE("chan %u ring_buf_put failed\n", chan);
                return SAL_FAIL;
            }
            if (enDispDir == pXpackChnPrm->enRtDirection) // 反向回拉，当前显示图像开始位置u64DispLocStart与u64DispDataLoc相同
            {
                u64DispLocStart = u64DispDataLoc;
                u64DispLocEnd = u64DispLocStart + pstCapbDisp->disp_yuv_w_max;
            }
            else // 正向回拉，当前显示图像结束位置u64DispLocEnd基于u64DispDataLoc增加回拉图像宽度
            {
                u64DispLocEnd = u64DispDataLoc + pstData4Disp->stDispData.u32Width;
                u64DispLocStart = SAL_SUB_SAFE(u64DispLocEnd, pstCapbDisp->disp_yuv_w_max);
            }
        }
        else
        {
            if (SAL_TRUE == pstData4Disp->bIfRefresh)
            {
                /* 全屏刷新背景色变化时重置显示背景色 */
                pXpackChnPrm->u32DispBgColor = pstData4Disp->u32DispBgColor;
                pXpackChnPrm->bIfRefresh = SAL_TRUE;
                enRingFreshLoc = RING_REFRESH_LEFT;
            }
            if (XRAY_PS_PLAYBACK_FRAME == pXpackChnPrm->enProcStat)
            {
                // TODO: 整包回拉的包裹宽度（显示域）会超过一整屏的宽
                if (SAL_SOK != ring_buf_clear(ringBufHandle, pXpackChnPrm->u32DispBgColor))
                {
                    XPACK_LOGE("chan %u ring_buf_reset failed\n", chan);
                    return SAL_FAIL;
                }
                enRingFreshLoc = RING_REFRESH_MIDDLE;
                // 清空回拉包裹管理队列
                SAL_mutexTmLock(&gstPbPackageCtrl[chan].lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                if (NULL != pstData4Pkg)
                {
                    while (!DSA_LstIsEmpty(gstPbPackageCtrl[chan].lstPackage)) // 非空则删
                    {
                        DSA_LstPop(gstPbPackageCtrl[chan].lstPackage);
                    }

                    pNode = DSA_LstGetIdleNode(gstPbPackageCtrl[chan].lstPackage);
                    if (NULL != pNode)
                    {
                        ring_buf_size(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], NULL, &u64DispRLoc, NULL, NULL);
                        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
                        memset(pstPkgAttr, 0, sizeof(XPACK_PACKAGE_ATTR));
                        pstPkgAttr->u64DispLocStart = SAL_SUB_SAFE(pstCapbDisp->disp_yuv_w_max, pstData4Disp->stDispData.u32Width) / 2;
                        pstPkgAttr->u64DispLocStart = pstPkgAttr->u64DispLocStart + SAL_SUB_SAFE(u64DispRLoc, pstCapbDisp->disp_yuv_w_max);
                        pstPkgAttr->u64DispLocEnd = pstPkgAttr->u64DispLocStart + pstData4Disp->stDispData.u32Width;
                        pstPkgAttr->bCompleted = SAL_TRUE;
                        pstPkgAttr->u64GenerateTime = sal_get_tickcnt();
                        pstPkgAttr->u32RawWidth = pstData4Pkg->u32RawWidth;
                        pstPkgAttr->u32RawHeight = pstData4Pkg->u32RawHeight;
                        pstPkgAttr->stPackDivInfo.uiPackTop = pstData4Pkg->u32Top;
                        pstPkgAttr->stPackDivInfo.uiPackBottom = pstData4Pkg->u32Bottom;
                        pstPkgAttr->u32AiDgrWOsd = pstData4Pkg->u32RawHeight; // 同步模式使用enSyncRedrawMode标识是否需要画OSD，而不使用AiDgrWOsd
                        pstPkgAttr->u32AiDgrWTotal = pstData4Pkg->u32RawHeight;
                        pstPkgAttr->u32AiDgrWRecv = pstData4Pkg->u32RawHeight;
                        pstPkgAttr->enXrIdtStage = XPACK_XAI_STAGE_OSD; // 同步模式使用enSyncRedrawMode标识是否需要画OSD，而不使用XrIdtStage
                        pstPkgAttr->enSyncRedrawMode = XPACK_OSD_REDRAW_DIRECTLY; // 整包回拉包裹需要直接画OSD
                        if (pstData4Pkg->pstAiDgrResult != NULL)
                        {
                            pstPkgAttr->stAiDgrResult.target_num = pstData4Pkg->pstAiDgrResult->target_num;
                            memcpy(pstPkgAttr->stAiDgrResult.target, pstData4Pkg->pstAiDgrResult->target, 
                                sizeof(SVA_DSP_ALERT) * pstData4Pkg->pstAiDgrResult->target_num);
                        }
                        if (pstData4Pkg->pstXrIdtResult != NULL)
                        {
                            memcpy(&pstPkgAttr->stXspIdtResult, pstData4Pkg->pstXrIdtResult, sizeof(XSP_RESULT_DATA));
                        }
                        DSA_LstInst(gstPbPackageCtrl[chan].lstPackage, pInst, pNode);
                    }
                }
                SAL_mutexTmUnlock(&gstPbPackageCtrl[chan].lstPackage->sync.mid, __FUNCTION__, __LINE__);
            }

            if (SAL_SOK != ring_buf_update(ringBufHandle, &pstData4Disp->stDispData, -1, enRingFreshLoc, 0))
            {
                XPACK_LOGE("chan %u ring_buf_update failed\n", chan);
                pXpackChnPrm->bIfRefresh = SAL_FALSE;
                return SAL_FAIL;
            }

            if (SAL_TRUE == pstData4Disp->bIfRefresh)
            {
                // 全屏成像处理发送了新的图像，完整包裹需要重新画OSD
                pstPkgCtrl = (XRAY_PS_PLAYBACK_MASK & pXpackChnPrm->enProcStat) ? (gstPbPackageCtrl + chan) : (gstRtPackageCtrl + chan);
                SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                pNode = DSA_LstGetTail(pstPkgCtrl->lstPackage);
                while (NULL != pNode) // 遍历所有包裹
                {
                    pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
                    pstPkgAttr->enSyncRedrawMode = pstPkgAttr->bCompleted == SAL_TRUE ? XPACK_OSD_REDRAW_DIRECTLY : XPACK_OSD_REDRAW_NONE;
                    pNode = pNode->prev;
                }
                SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);
            }
        }
    }

    if (NULL != pstData4Disp->stRawData.pPlaneVir[0])
    {
        if (SAL_SOK != segment_buf_put(gPbRawSegBufHandle[chan], &pstData4Disp->stRawData, enRingBufDir, NULL))
        {
            XPACK_LOGE("chan %u segment_buf_put failed\n", chan);
            return SAL_FAIL;
        }
    }
    if (NULL != pstData4Disp->stBlendData.pPlaneVir[0])
    {
        if (SAL_SOK != segment_buf_put(gPbBlendSegBufHandle[chan], &pstData4Disp->stBlendData, enRingBufDir, NULL))
        {
            XPACK_LOGE("chan %u segment_buf_put failed\n", chan);
            return SAL_FAIL;
        }
    }

    if (NULL != pstData4Pkg)
    {
        /**
         * 条带回拉，正向回拉新出现的包裹，是实时预览中较旧的包裹，Loc较大，插入到队列尾，同时队列头的包裹可能出显示区域 
         *         反向回拉新出现的包裹，是实时预览中较新的包裹，Loc较小，插入到队列头，同时队列尾的包裹可能出显示区域 
         */
        if (XRAY_PS_PLAYBACK_SLICE_FAST == pXpackChnPrm->enProcStat && !pstData4Disp->bIfRefresh)
        {
            SAL_mutexTmLock(&gstPbPackageCtrl[chan].lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            if (0 == pstData4Pkg->u32StartLoc && 0 == pstData4Pkg->u32EndLoc) // 全空白无包裹
            {
                // 仅做包裹出显示区域后的删除，无新包裹插入

                // 条带是一个包裹的中间部分也走该分支
                /* 左右切换过包方向时，新数据是下发的无OSD图像，需要对旧数据OSD清除 */
                if (enDispDir != pXpackChnPrm->enPbDirection)
                {
                    if (enDispDir == pXpackChnPrm->enRtDirection) // 反向回拉，更新头包裹
                    {
                        pNode = DSA_LstGetHead(gstPbPackageCtrl[chan].lstPackage);
                    }
                    else // 正向回拉，更新尾包裹
                    {
                        pNode = DSA_LstGetTail(gstPbPackageCtrl[chan].lstPackage);
                    }
                    if (NULL != pNode)
                    {
                        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
                        if (!pstPkgAttr->bCompleted) /* 包裹不完整说明是包裹中间段，清除包裹中残留OSD */
                        {
                            memset(&pstPkgAttr->stAiDgrResult, 0, sizeof(XPACK_SVA_RESULT_OUT));
                            memset(&pstPkgAttr->stXspIdtResult, 0, sizeof(XSP_RESULT_DATA));
                            pstPkgAttr->enSyncRedrawMode = XPACK_OSD_REDRAW_CLEAROLD; // 智能信息被清除，只进行图像拷贝
                        }
                    }
                }
            }
            else
            {
                if (pstData4Pkg->u32StartLoc != XSP_PACK_LINE_INFINITE) // 有新的包裹进入
                {
                    pNode = DSA_LstGetIdleNode(gstPbPackageCtrl[chan].lstPackage);
                    if (NULL != pNode)
                    {
                        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
                        memset(pstPkgAttr, 0, sizeof(XPACK_PACKAGE_ATTR));
                        pstPkgAttr->u32RawWidth = pstData4Disp->stRawData.u32Width;
                        pstPkgAttr->u32RawHeight = (pstData4Pkg->u32EndLoc == XSP_PACK_LINE_INFINITE) ? 
                            (pstData4Disp->stDispData.u32Width - pstData4Pkg->u32StartLoc) : (pstData4Pkg->u32EndLoc - pstData4Pkg->u32StartLoc + 1);
                        if (enDispDir == pXpackChnPrm->enRtDirection) // 反向回拉，从Loc小端出，插入到队列头，更新包裹的End Loc
                        {
                            pstPkgAttr->u64DispLocEnd = u64DispLocStart + (pstData4Disp->stDispData.u32Width - pstData4Pkg->u32StartLoc);
                            pstPkgAttr->u64DispLocStart = pstPkgAttr->u64DispLocEnd - pstPkgAttr->u32RawHeight;
                            pInst = DSA_LstGetHead(gstPbPackageCtrl[chan].lstPackage);
                            if (NULL != pInst)
                            {
                                if (((XPACK_PACKAGE_ATTR *)pInst->pAdData)->u64DispLocStart == 0) // 队列头包裹并未完整，重新结束属于异常
                                {
                                    XPACK_LOGE("xpack chan %u, expect to insert to head, but head package is uncompleted, loc %llu ~ %llu\n", 
                                               chan, ((XPACK_PACKAGE_ATTR *)pInst->pAdData)->u64DispLocStart, ((XPACK_PACKAGE_ATTR *)pInst->pAdData)->u64DispLocEnd);
                                    pNode = NULL;
                                }
                                else
                                {
                                    pstPkgAttr->u32Id = SAL_SUB_SAFE(((XPACK_PACKAGE_ATTR *)pInst->pAdData)->u32Id, 1);
                                    pInst = NULL;
                                }
                            }
                        }
                        else // 正向回拉，从Loc大端出，插入到队列尾，更新包裹的Start Loc
                        {
                            pstPkgAttr->u64DispLocStart = u64DispLocEnd - (pstData4Disp->stDispData.u32Width - pstData4Pkg->u32StartLoc);
                            pstPkgAttr->u64DispLocEnd = pstPkgAttr->u64DispLocStart + pstPkgAttr->u32RawHeight;
                            pInst = DSA_LstGetTail(gstPbPackageCtrl[chan].lstPackage);
                            if (NULL != pInst)
                            {
                                if (((XPACK_PACKAGE_ATTR *)pInst->pAdData)->u64DispLocEnd == 0) // 队列尾包裹并未完整，重新开始属于异常
                                {
                                    XPACK_LOGE("xpack chan %u, expect to insert to tail, but tail package is uncompleted, loc %llu ~ %llu\n", 
                                               chan, ((XPACK_PACKAGE_ATTR *)pInst->pAdData)->u64DispLocStart, ((XPACK_PACKAGE_ATTR *)pInst->pAdData)->u64DispLocEnd);
                                    pNode = NULL;
                                }
                                else
                                {
                                    pstPkgAttr->u32Id = ((XPACK_PACKAGE_ATTR *)pInst->pAdData)->u32Id + 1;
                                }
                            }
                        }
                        if (NULL != pNode)
                        {
                            XPACK_LOGI("xpack chan %u, pb dir %d, width %u, package start at %u, new %s, id %u, disp loc %llu ~ %llu\n", 
                                       chan, enDispDir, pstData4Disp->stDispData.u32Width, pstData4Pkg->u32StartLoc, 
                                       (enDispDir == pXpackChnPrm->enRtDirection) ? "head" : "tail", pstPkgAttr->u32Id, u64DispLocStart, u64DispLocEnd);
                            DSA_LstInst(gstPbPackageCtrl[chan].lstPackage, pInst, pNode);
                        }
                        else
                        {
                            DSA_LstDelete(gstPbPackageCtrl[chan].lstPackage, pInst);
                        }
                    }
                }

                if (enDispDir == pXpackChnPrm->enRtDirection) // 反向回拉，更新头包裹
                {
                    pNode = DSA_LstGetHead(gstPbPackageCtrl[chan].lstPackage);
                }
                else // 正向回拉，更新尾包裹
                {
                    pNode = DSA_LstGetTail(gstPbPackageCtrl[chan].lstPackage);
                }
                if (NULL != pNode)
                {
                    pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
                    if (pstData4Pkg->u32EndLoc != XSP_PACK_LINE_INFINITE) // 有包裹结束，更新包裹信息
                    {
                        if (enDispDir == pXpackChnPrm->enRtDirection) // 反向回拉，从Loc小端出，更新头包裹的Start Loc
                        {
                            pstPkgAttr->u64DispLocStart = u64DispLocStart + (pstData4Disp->stDispData.u32Width - pstData4Pkg->u32EndLoc - 1);
                        }
                        else // 正向回拉，从Loc大端出，更新尾包裹的End Loc
                        {
                            pstPkgAttr->u64DispLocEnd = u64DispLocEnd - (pstData4Disp->stDispData.u32Width - pstData4Pkg->u32EndLoc - 1);
                        }
                        if (SAL_SUB_SAFE(pstPkgAttr->u64DispLocEnd, pstPkgAttr->u64DispLocStart) == pstData4Pkg->u32RawHeight)
                        {
                            pstPkgAttr->bCompleted = SAL_TRUE;
                            pstPkgAttr->u32RawWidth = pstData4Pkg->u32RawWidth;
                            pstPkgAttr->u32RawHeight = pstData4Pkg->u32RawHeight;
                            pstPkgAttr->stPackDivInfo.uiPackTop = pstData4Pkg->u32Top;
                            pstPkgAttr->stPackDivInfo.uiPackBottom = pstData4Pkg->u32Bottom;
                            pstPkgAttr->u32AiDgrWOsd = pstData4Pkg->u32RawHeight; // 同步模式使用enSyncRedrawMode标识是否需要画OSD，而不使用AiDgrWOsd
                            pstPkgAttr->u32AiDgrWTotal = pstData4Pkg->u32RawHeight;
                            pstPkgAttr->u32AiDgrWRecv = pstData4Pkg->u32RawHeight;
                            pstPkgAttr->enXrIdtStage = XPACK_XAI_STAGE_OSD; // 同步模式使用enSyncRedrawMode标识是否需要画OSD，而不使用XrIdtStage
                            pstPkgAttr->stAiDgrResult.target_num = pstData4Pkg->pstAiDgrResult->target_num;
                            memcpy(pstPkgAttr->stAiDgrResult.target, pstData4Pkg->pstAiDgrResult->target, 
                                   sizeof(SVA_DSP_ALERT) * pstData4Pkg->pstAiDgrResult->target_num);
                            memcpy(&pstPkgAttr->stXspIdtResult, pstData4Pkg->pstXrIdtResult, sizeof(XSP_RESULT_DATA));
                            /* 包裹完整需画OSD，不是第一次完整可能有OSD残留需要更新 */
                            pstPkgAttr->enSyncRedrawMode = pstPkgAttr->u64GenerateTime > 0 ? XPACK_OSD_REDRAW_CLEAROLD : XPACK_OSD_REDRAW_DIRECTLY;
                            pstPkgAttr->u64GenerateTime = sal_get_tickcnt();
                        }
                        else
                        {
                            XPACK_LOGE("xpack chan %u, package id %u, loc %llu ~ %llu, unequal to raw height %u, delete it\n", 
                                       chan, pstPkgAttr->u32Id, pstPkgAttr->u64DispLocStart, pstPkgAttr->u64DispLocEnd, pstData4Pkg->u32RawHeight);
                            DSA_LstDelete(gstPbPackageCtrl[chan].lstPackage, pNode);
                        }
                    }
                    else // 包裹持续增长，更新Start或End Loc信息
                    {
                        if (pstData4Pkg->u32StartLoc == XSP_PACK_LINE_INFINITE)
                        {
                            if (enDispDir == pXpackChnPrm->enRtDirection) // 反向回拉，从Loc小端出，更新头包裹的Start Loc
                            {
                                pstPkgAttr->u64DispLocStart += pstData4Disp->stDispData.u32Width;
                            }
                            else // 正向回拉，从Loc大端出，更新尾包裹的End Loc
                            {
                                pstPkgAttr->u64DispLocEnd += pstData4Disp->stDispData.u32Width;
                            }
                        }
                        else
                        {
                            // 在上面最开始的分支中已处理
                        }
                    }
                }
            }

            if (enDispDir == pXpackChnPrm->enRtDirection) // 反向回拉，从队列尾往前遍历，删除完全出屏幕的包裹，队列头包裹进入屏幕，更新不完整包裹的OSD
            {
                pNode = DSA_LstGetTail(gstPbPackageCtrl[chan].lstPackage);
                while (NULL != pNode)
                {
                    pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
                    pDel = pNode;
                    pNode = pNode->prev;
                    XPACK_LOGI("xpack chan %u, rt dir %d, pb dir %d, Reverse, Package(%u) Loc %llu ~ %llu, Screen Loc %llu ~ %llu, %s tail\n", 
                               chan, pXpackChnPrm->enRtDirection, enDispDir, pstPkgAttr->u32Id, pstPkgAttr->u64DispLocStart, pstPkgAttr->u64DispLocEnd, 
                               u64DispLocStart, u64DispLocEnd, (pstPkgAttr->u64DispLocStart >= u64DispLocEnd) ? "Delete" : "Reserve");
                    if (pstPkgAttr->u64DispLocStart >= u64DispLocEnd) // 完全出屏幕
                    {
                        if (XRAY_DIRECTION_LEFT == enDispDir) // 过包方向与回拉方向均为R2L，Loc左侧为end，右侧为start
                        {
                            u32Width = pstPkgAttr->u64DispLocStart - ((NULL != pNode) ? ((XPACK_PACKAGE_ATTR *)pNode->pAdData)->u64DispLocEnd : u64DispLocStart);
                            u32Width = SAL_MIN(u32Width, XPACK_OSD_DISP_EXPAND_MAX);
                            xpack_disp_clear_osd_by_pure(chan, u32Width, pstPkgAttr->u64DispLocStart-u32Width);
                        }
                        DSA_LstDelete(gstPbPackageCtrl[chan].lstPackage, pDel);
                    }
                    else if (pstPkgAttr->u64DispLocEnd > u64DispLocEnd) // 部分出屏幕，置包裹状态为不完整
                    {
                        // pstPkgAttr->u64DispLocEnd = u64DispLocEnd;
                        pstPkgAttr->bCompleted = SAL_FALSE;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            else // 正向回拉，从队列头往后遍历，直到包裹在屏幕内
            {
                pNode = DSA_LstGetHead(gstPbPackageCtrl[chan].lstPackage);
                while (NULL != pNode)
                {
                    pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
                    pDel = pNode;
                    pNode = pNode->next;
                    XPACK_LOGI("xpack chan %u, rt dir %d, pb dir %d, Forward, Package(%u) Loc %llu ~ %llu, Screen Loc %llu ~ %llu, %s head\n", 
                               chan, pXpackChnPrm->enRtDirection, enDispDir, pstPkgAttr->u32Id, pstPkgAttr->u64DispLocStart, pstPkgAttr->u64DispLocEnd, 
                               u64DispLocStart, u64DispLocEnd, (pstPkgAttr->u64DispLocEnd <= u64DispLocStart) ? "Delete" : "Reserve");
                    if (pstPkgAttr->u64DispLocEnd <= u64DispLocStart) // 完全出屏幕
                    {
                        if (XRAY_DIRECTION_LEFT == enDispDir) // 过包方向L2R，回拉方向R2L，Loc左侧为start，右侧为end
                        {
                            u32Width = ((NULL != pNode) ? ((XPACK_PACKAGE_ATTR *)pNode->pAdData)->u64DispLocStart : u64DispLocEnd) - pstPkgAttr->u64DispLocEnd;
                            u32Width = SAL_MIN(u32Width, XPACK_OSD_DISP_EXPAND_MAX);
                            xpack_disp_clear_osd_by_pure(chan, u32Width, pstPkgAttr->u64DispLocEnd);
                        }
                        DSA_LstDelete(gstPbPackageCtrl[chan].lstPackage, pDel);
                    }
                    else if (pstPkgAttr->u64DispLocStart < u64DispLocStart) // 部分出屏幕，置包裹状态为不完整
                    {
                        // pstPkgAttr->u64DispLocStart = u64DispLocStart;
                        pstPkgAttr->bCompleted = SAL_FALSE;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            SAL_mutexTmUnlock(&gstPbPackageCtrl[chan].lstPackage->sync.mid, __FUNCTION__, __LINE__);
        }
    }

    pXpackChnPrm->enPbDirection = enDispDir; /* 更新条带回拉方向 */
    if (SAL_SOK == SAL_mutexTmLock(&pXpackChnPrm->mutexOsdDraw, 1000, __FUNCTION__, __LINE__))
    {
        xpack_osd_draw_on_screen(chan, NULL, SAL_TRUE, XPACK_OSD_REDRAW_DIRECTLY);
        SAL_mutexTmUnlock(&pXpackChnPrm->mutexOsdDraw, __FUNCTION__, __LINE__);
    }

    if (pstDumpCfg->u32DumpCnt > 0)
    {
        if ((pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_FSCRAW) && 
            (NULL != pstData4Disp->stRawData.pPlaneVir[0] || NULL != pstData4Disp->stBlendData.pPlaneVir[0]))
        {
            if (SAL_SOK != segment_buf_get(gPbRawSegBufHandle[chan], &stRawDumpBuff, NULL))
            {
                XPACK_LOGE("chan %u segment_buf_get tip raw failed\n", chan);
                return SAL_FAIL;
            }
            if (SAL_SOK != segment_buf_get(gPbBlendSegBufHandle[chan], &stBlendDumpBuff, NULL))
            {
                XPACK_LOGE("chan %u segment_buf_get blend failed\n", chan);
                return SAL_FAIL;
            }
            ximg_dump(&stRawDumpBuff.stFscRawData, chan, pstDumpCfg->chDumpDir, "xpack-fsc-raw", 
                      (XRAY_PS_RTPREVIEW == pXpackChnPrm->enProcStat) ? "rt" : "pb", pstDumpCfg->u32DumpCnt);
            ximg_dump(&stBlendDumpBuff.stFscRawData, chan, pstDumpCfg->chDumpDir, "xpack-fsc-blend", 
                      (XRAY_PS_RTPREVIEW == pXpackChnPrm->enProcStat) ? "rt" : "pb", pstDumpCfg->u32DumpCnt);
        }
        if (pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_SENDRAW && NULL != pstData4Disp->stRawData.pPlaneVir[0] && NULL != pstData4Disp->stBlendData.pPlaneVir[0])
        {
            ximg_dump(&pstData4Disp->stRawData, chan, pstDumpCfg->chDumpDir, "send-pb-raw", NULL, pstDumpCfg->u32DumpCnt);
            ximg_dump(&pstData4Disp->stBlendData, chan, pstDumpCfg->chDumpDir, "send-pb-blend", NULL, pstDumpCfg->u32DumpCnt);
        }
        if (pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_SENDDISP)
        {
            ximg_dump(&pstData4Disp->stDispData, chan, pstDumpCfg->chDumpDir, "send-disp", NULL, pstDumpCfg->u32DumpCnt);
        }
        if (pstDumpCfg->unDumpDp.u32XpackDumpType & 0xF)
        {
            pstDumpCfg->u32DumpCnt = SAL_SUB_SAFE(pstDumpCfg->u32DumpCnt, 1);
        }
    }

    return SAL_SOK;
}


/**
 * @fn      Xpack_DrvSaveScreen
 * @brief   保存当前屏幕图像为jpeg、bmp或raw格式数据
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstSavePrm 保存设置参数
 * 
 * @return  SAL_STATUS SAL_SOK：SAVE成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvSaveScreen(UINT32 chan, XPACK_SAVE_PRM *pstSavePrm)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 u32RetLeft = 0;
    XPACK_SAVE_TYPE_E enOutType = XPACK_SAVE_SRC;
    XRAY_TRANS_TYPE enSaveType = XRAY_TRANS_BMP;

    SAL_VideoCrop stSrcCrop = {0};
    SAL_VideoCrop stDstCrop = {0};
    XIMAGE_DATA_ST *pstSaveDispImg = {0}; // 保存全屏图像的信息
    XIMAGE_DATA_ST stTmpDispImg = {0}; // 获取的循环显示缓冲区信息
    XIMAGE_DATA_ST *pstDispImg = &stTmpDispImg;

    SEG_BUF_DATA stRawFscBuf = {0};
    XIMAGE_DATA_ST stOutFscRaw = {0};

    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;

    XPACK_CHECK_PTR_NULL(pstSavePrm, SAL_FAIL);
    XPACK_CHECK_CHAN(chan, SAL_FAIL);

    pstXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];
    if (XRAY_PS_PLAYBACK_MASK & pstXpackChnPrm->enProcStat)
    {
        XPACK_LOGE("chan %d playback mode[%d] is not supported in SAVE_FUNCTION\n", chan, pstXpackChnPrm->enProcStat);
        return SAL_FAIL;
    }

    enOutType = pstSavePrm->outType;
    pstSaveDispImg = &pstXpackChnPrm->stSaveDispBuf;

    /* raw数据保存 */
    if (XPACK_SAVE_SRC == enOutType)
    {
        segment_buf_get(gRtTipRawSegBufHandle[chan], &stRawFscBuf, NULL);
        ximg_create(pstCapbXsp->xraw_width_resized, pstCapbXsp->xraw_height_resized, 
                    stRawFscBuf.stFscRawData.enImgFmt, NULL, pstSavePrm->pOutBuff, &stOutFscRaw);
        if (pstSavePrm->u32BuffSize < stOutFscRaw.stMbAttr.memSize)
        {
            XPACK_LOGE("Input raw buffer size[%u] is not smaller than needed size[%u]\n", pstSavePrm->u32BuffSize, stOutFscRaw.stMbAttr.memSize);
            return SAL_FAIL;
        }

        /* 拷贝前部数据 */
        stSrcCrop.top = stRawFscBuf.u32FrontPartOffset;
        stSrcCrop.left = 0;
        stSrcCrop.width = stOutFscRaw.u32Width;
        stSrcCrop.height = stRawFscBuf.u32FrontPartLen;

        stDstCrop.top = 0;
        stDstCrop.left = 0;
        stDstCrop.width = stSrcCrop.width;
        stDstCrop.height = stSrcCrop.height;
        ximg_crop(&stRawFscBuf.stFscRawData, &stOutFscRaw, &stSrcCrop, &stDstCrop);

        /* 拷贝后部数据 */
        stSrcCrop.top = stRawFscBuf.u32LatterPartOffset;
        stSrcCrop.left = 0;
        stSrcCrop.width = stOutFscRaw.u32Width;
        stSrcCrop.height = stRawFscBuf.u32LatterPartLen;

        stDstCrop.top = stRawFscBuf.u32FrontPartLen;
        stDstCrop.left = 0;
        stDstCrop.width = stSrcCrop.width;
        stDstCrop.height = stSrcCrop.height;
        ximg_crop(&stRawFscBuf.stFscRawData, &stOutFscRaw, &stSrcCrop, &stDstCrop);

        pstSavePrm->u32DataSize = stOutFscRaw.stMbAttr.memSize;
        pstSavePrm->u32Width = stOutFscRaw.u32Width;
        pstSavePrm->u32Height = stOutFscRaw.u32Height;

        XPACK_LOGI("chan %d save xdata success w:%d h:%d XraySize:%d tiporgnum %d buffsize %d\n", chan, pstSavePrm->u32Width, pstSavePrm->u32Height, pstSavePrm->u32DataSize, pstSavePrm->igmPrm.stResultData.stZeffSent.uiNum, pstSavePrm->u32BuffSize);
    }
    else if (XPACK_SAVE_BMP == enOutType || XPACK_SAVE_JPEG == enOutType) /* bmp/jpeg抓图 */
    {
        u32RetLeft = ring_buf_get(gXpackRingBufHandle[chan][XPACK_DISPDT_PURE], 0, pstXpackChnPrm->u32DispVertOffset, &pstDispImg, 1);
        if (-1 == u32RetLeft)
        {
            XPACK_LOGE("chan %u ring_buf_get failed\n", chan);
            return SAL_FAIL;
        }

        stSrcCrop.top = pstCapbDisp->disp_h_top_blanking;
        stSrcCrop.left = 0;
        stSrcCrop.width = pstCapbDisp->disp_yuv_w_max;
        stSrcCrop.height = pstCapbDisp->disp_yuv_h;

        stDstCrop.top = 0;
        stDstCrop.left = 0;
        stDstCrop.width = stSrcCrop.width;
        stDstCrop.height = stSrcCrop.height;
        /* 从循环缓冲中将全屏数据拷贝出来 */
        s32Ret = ximg_crop(pstDispImg, pstSaveDispImg, &stSrcCrop, &stDstCrop);
        if (SAL_SOK != s32Ret)
        {
            XPACK_LOGE("ximg_crop failed\n");
            return SAL_FAIL;
        }

        if (pstSavePrm->bSvaFlag)
        {
            /* draw osd */
        }
        
        if (XPACK_SAVE_BMP == enOutType)
        {
            enSaveType = XRAY_TRANS_BMP;
        }
        else if (XPACK_SAVE_JPEG == enOutType)
        {
            enSaveType = XRAY_TRANS_JPG;
        }

        /* 将原始图像数据保存为对应图片格式 */
        if (SAL_SOK != ximg_save(pstSaveDispImg, enSaveType, pstSavePrm->pOutBuff, &pstSavePrm->u32DataSize, 0))
        {
            XSP_LOGE("chan %u img_save failed, save type:%d, w:%u, h:%u, s:%u\n", 
                     chan, enSaveType, pstSaveDispImg->u32Width, pstSaveDispImg->u32Height, pstSaveDispImg->u32Stride[0]);
            return SAL_FAIL;
        }

        /* 回调 */
        pstSavePrm->u32Width = pstSaveDispImg->u32Width;
        pstSavePrm->u32Height = pstSaveDispImg->u32Height;

        XPACK_LOGI("chan %d save success type:%d, w:%u, h:%u, outDataSize:%d, u32BuffSize %d\n", 
                   chan, enOutType, pstSavePrm->u32Width, pstSavePrm->u32Height, pstSavePrm->u32DataSize, pstSavePrm->u32BuffSize);
    }
    else
    {
        XPACK_LOGE("Chn %d OutType %d is error !!!\n", chan, enOutType);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      Xpack_DrvSetPackJpgCbPrm
 * @brief   设置包裹回调的JPG图片参数
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstJpgCbPrm 包裹回调的JPG图片参数
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvSetPackJpgCbPrm(UINT32 chan, XPACK_JPG_SET_ST *pstJpgCbPrm)
{
    PACK_JPG_CB_CTRL *pPackJpgCbCtrl = NULL;

    XPACK_CHECK_PTR_NULL(pstJpgCbPrm, SAL_FAIL);
    XPACK_CHECK_CHAN(chan, SAL_FAIL);

    if (pstJpgCbPrm->f32WidthResizeRatio < 0.1 || pstJpgCbPrm->f32WidthResizeRatio > 1 ||
        pstJpgCbPrm->f32HeightResizeRatio < 0.1 || pstJpgCbPrm->f32HeightResizeRatio > 1)
    {
        XPACK_LOGE("chan %d ratioW %f ratioH %f is illegal \n", chan, pstJpgCbPrm->f32WidthResizeRatio, pstJpgCbPrm->f32HeightResizeRatio);
        return SAL_FAIL;
    }

    if (pstJpgCbPrm->u32JpgBackWidth < XPACK_JPG_MIN_WIDTH || pstJpgCbPrm->u32JpgBackWidth > XPACK_JPG_MAX_WIDTH ||
        pstJpgCbPrm->u32JpgBackHeight < XPACK_JPG_MIN_HEIGHT || pstJpgCbPrm->u32JpgBackHeight > XPACK_JPG_MAX_HEIGHT)
    {
        /* 背板宽高均为0时表示不使用背板 */
        if (0 != pstJpgCbPrm->u32JpgBackWidth || 0 != pstJpgCbPrm->u32JpgBackHeight)
        {
            XPACK_LOGE("chan %d BACK width %d height %d is illegal, require [%d x %d]~[%d x %d]\n", 
                       chan, pstJpgCbPrm->u32JpgBackWidth, pstJpgCbPrm->u32JpgBackHeight, 
                       XPACK_JPG_MIN_WIDTH, XPACK_JPG_MIN_HEIGHT, XPACK_JPG_MAX_WIDTH, XPACK_JPG_MAX_HEIGHT);
            return SAL_FAIL;
        }
    }

    pPackJpgCbCtrl = &gGlobalPrm.astXpackChnPrm[chan].stPackJpgCbCtrl;

    pPackJpgCbCtrl->bJpgDrawSwitch = pstJpgCbPrm->bJpgDrawSwitch;
    pPackJpgCbCtrl->bJpgCropSwitch = pstJpgCbPrm->bJpgCropSwitch;
    pPackJpgCbCtrl->u32BackW = SAL_alignDown(pstJpgCbPrm->u32JpgBackWidth, 16);
    pPackJpgCbCtrl->u32BackH = SAL_alignDown(pstJpgCbPrm->u32JpgBackHeight, 2);
    pPackJpgCbCtrl->f32WidthResizeRatio = pstJpgCbPrm->f32WidthResizeRatio;
    pPackJpgCbCtrl->f32HeightResizeRatio = pstJpgCbPrm->f32HeightResizeRatio;

    XPACK_LOGI("chn %d u32BackW %d u32BackH %d drawSwitch %u cropSwitch %u ratioW %f ratioH %f\n", 
               chan, pPackJpgCbCtrl->u32BackW, pPackJpgCbCtrl->u32BackH, 
               pPackJpgCbCtrl->bJpgDrawSwitch, pPackJpgCbCtrl->bJpgCropSwitch, 
               pPackJpgCbCtrl->f32WidthResizeRatio, pPackJpgCbCtrl->f32HeightResizeRatio);

    return SAL_SOK;
}

/**
 * @function   xpack_trans_jpgcb_coor
 * @brief      
 * @param[IN] XPACK_SVA_RESULT_OUT  *pPackSvaOut     
 * @param[IN] XPACK_COOR_TRANS      *pJpgCoorTransPrm 
 * @param[OUT] XPACK_SVA_RESULT_OUT *pstTransedSvaOut 
 * @param[OUT] XIMG_BORDER          *pstProfileBorder 可为空
 * @return     SAL_STATUS 成功-SAL_SOK，失败-SAL_FAIL
 */
static SAL_STATUS xpack_trans_jpgcb_coor(XPACK_SVA_RESULT_OUT *pPackSvaOut, XPACK_COOR_TRANS *pJpgCoorTransPrm, XPACK_SVA_RESULT_OUT *pstTransedSvaOut, XIMG_BORDER *pstProfileBorder)
{
    INT32 i = 0;
    UINT32 u32PackStartW = 0;   /* 包裹在背板中的起始位置 */
    UINT32 u32PackStartH = 0;
    UINT32 u32PackWidth = 0, u32PackHeight = 0;
    UINT32 u32JpgOriWidth = 0, u32JpgOriHeight = 0;
    UINT32 u32JpgScaledWidth = 0, u32JpgScaledHeight = 0;
    UINT32 u32JpgBackpanelWidth = 0, u32JpgBackpanelHeight = 0; /* 背板图像的分辨率 */
    FLOAT32 fRatioW = 1.0f, fRatioH = 1.0f;

    DISP_SVA_RECT_F stTmpSvaRect = {0};
    XPACK_CHECK_PTR_NULL(pPackSvaOut, SAL_FAIL);
    XPACK_CHECK_PTR_NULL(pJpgCoorTransPrm, SAL_FAIL);
    XPACK_CHECK_PTR_NULL(pstTransedSvaOut, SAL_FAIL);

    u32PackWidth = pJpgCoorTransPrm->u32PackWidth;
    u32PackHeight = pJpgCoorTransPrm->u32PackHeight;
    u32JpgOriWidth = pJpgCoorTransPrm->u32JpgCropedWidth;
    u32JpgOriHeight = pJpgCoorTransPrm->u32JpgCropedHeight;
    u32JpgScaledWidth = pJpgCoorTransPrm->u32JpgScaleWidth;
    u32JpgScaledHeight = pJpgCoorTransPrm->u32JpgScaleHeight;
    u32JpgBackpanelWidth = pJpgCoorTransPrm->u32BackWidth;
    u32JpgBackpanelHeight = pJpgCoorTransPrm->u32BackHeight;

    u32PackStartW = u32JpgBackpanelWidth > u32JpgScaledWidth ? (u32JpgBackpanelWidth - u32JpgScaledWidth) / 2 : 0;
    u32PackStartW = SAL_alignDown(u32PackStartW, 2);
    u32PackStartH = u32JpgBackpanelHeight > u32JpgScaledHeight ? (u32JpgBackpanelHeight - u32JpgScaledHeight) / 2 : 0;
    u32PackStartH = SAL_alignDown(u32PackStartH, 2);
    fRatioW = (FLOAT32)u32JpgScaledWidth / u32JpgOriWidth;
    fRatioH = (FLOAT32)u32JpgScaledHeight / u32JpgOriHeight;

    pstTransedSvaOut->target_num = pPackSvaOut->target_num;
    for (i = 0; i < pPackSvaOut->target_num; i++)
    {
        /* 转为整数型 */
        stTmpSvaRect.x      = pPackSvaOut->target[i].rect.x * u32PackWidth;
        stTmpSvaRect.y      = pPackSvaOut->target[i].rect.y * u32PackHeight;
        stTmpSvaRect.width  = pPackSvaOut->target[i].rect.width * u32PackWidth;
        stTmpSvaRect.height = pPackSvaOut->target[i].rect.height * u32PackHeight;

        /* 抠图后坐标转换 */
        stTmpSvaRect.y = SAL_SUB_SAFE(stTmpSvaRect.y, pJpgCoorTransPrm->u32CropTop);

        /* 画面缩放坐标转换 */
        stTmpSvaRect.x      = (UINT32)(stTmpSvaRect.x * fRatioW);
        stTmpSvaRect.y      = (UINT32)(stTmpSvaRect.y * fRatioH);
        stTmpSvaRect.width  = (UINT32)(stTmpSvaRect.width * fRatioW);
        stTmpSvaRect.height = (UINT32)(stTmpSvaRect.height * fRatioH);

        /* 包裹填充坐标转换 */
        memcpy(&pstTransedSvaOut->target[i], &pPackSvaOut->target[i], sizeof(SVA_DSP_ALERT));
        pstTransedSvaOut->target[i].rect.x      = (FLOAT32)SAL_alignDown(u32PackStartW + stTmpSvaRect.x, 2) / u32JpgBackpanelWidth;
        pstTransedSvaOut->target[i].rect.y      = (FLOAT32)SAL_alignDown(u32PackStartH + stTmpSvaRect.y, 2) / u32JpgBackpanelHeight;
        pstTransedSvaOut->target[i].rect.width  = (FLOAT32)SAL_alignDown(stTmpSvaRect.width, 2) / u32JpgBackpanelWidth;
        pstTransedSvaOut->target[i].rect.height = (FLOAT32)SAL_alignDown(stTmpSvaRect.height, 2) / u32JpgBackpanelHeight;
    }

    if (NULL != pstProfileBorder)
    {
        pstProfileBorder->u32Top = u32PackStartH;
        if (0 == pJpgCoorTransPrm->u32CropTop)
        {
            /* 不抠图，原图的top和bottom需要等比例缩放 */
            pstProfileBorder->u32Top += (UINT32)(pJpgCoorTransPrm->u32PackTop * fRatioH);
        }
        pstProfileBorder->u32Bottom = SAL_MIN(pstProfileBorder->u32Top + u32JpgScaledHeight, u32JpgBackpanelHeight);
        pstProfileBorder->u32Left = u32PackStartW;
        pstProfileBorder->u32Right = SAL_MIN(u32PackStartW + u32JpgScaledWidth, u32JpgBackpanelWidth);
    }

    return SAL_SOK;
}

/**
 * @function    xpack_package_jpg_enc_thread
 * @brief       将包裹原始显示图像进行值域缩放、尺寸缩放、背板填充、jpeg编码处理
 * @param[IN]  VOID  *pPrm     通道号
 * @return      VOID *
 */
static void *xpack_package_jpg_enc_thread(VOID *pPrm)
{
    BOOL bDump = SAL_FALSE;
    UINT32 chan = 0, u32BgColor = 0;
    UINT32 u32InClipTop = 0, u32InClipBot = 0;                  /* 外部传入抠图上下边界 */
    UINT32 u32CropTop = 0, u32CropBottom = 0;                   /* 实际抠图下边界 */
    UINT32 u32ImgWidth = 0, u32ImgHeight = 0;                   /* 原图数据宽高 */
    UINT32 u32JpgWidthOri = 0, u32JpgHeightOri = 0;             /* 原图中需转换为JPG部分的尺寸 */
    UINT32 u32JpgWidthAligned = 0, u32JpgHeightAligned = 0;     /* 原图整图宽高对齐后的尺寸 */
    UINT32 u32JpgOriScaleW = 0, u32JpgOriScaleH = 0;            /* 原图中实际需转换为jpg部分经过缩放后的尺寸 */
    UINT32 u32JpgAlignedScaleW = 0, u32JpgAlignedScaleH = 0;    /* 原图整图宽高对齐后再经过缩放的尺寸 */
    UINT32 u32JpgWidthBackpanel = 0, u32JpgHeightBackpanel = 0; /* 背板图像的分辨率 */
    FLOAT32 fRatioW = 1.0f, fRatioH = 1.0f;    /* 数据宽与背板宽比值 */

    PACK_JPG_CB_CTRL *pstPackJpegCtrl = NULL;
    DSA_NODE *pNode = NULL;
    PACK_JPEG_ENC_PRM *pstJpgEncNode = NULL;

    SAL_VideoCrop stCropSrc = {0}, stCropDst = {0};
    XIMAGE_DATA_ST *pstSrcImg = NULL;
    XIMAGE_DATA_ST *pstResizedImg = NULL;
    XIMAGE_DATA_ST *pstFilledImg = NULL;
    XPACK_SVA_RESULT_OUT *pstSvaResult = NULL;      // 源图的智能信息
    XPACK_SVA_RESULT_OUT *pstJpegSvaResult = NULL;  // 图像经过转换后的智能信息
    // XSP_RESULT_DATA *pstXspResult = NULL;
    XPACK_COOR_TRANS stJpgCoorTrans = {0};
    XPACK_COOR_TRANS *pstLocJpgCoorTrans = NULL;
    XIMG_BORDER stProfileBorder = {0};
    XPACK_PACKAGE_OSD stPackOsd = {0};
    XPACK_DUMP_CFG *pstDumpCfg = NULL;
    CHAR dumpName[128] = {0};

    chan = (UINT32)(PhysAddr)pPrm;
    XPACK_CHECK_CHAN(chan, NULL);

    pstDumpCfg = &gGlobalPrm.astXpackChnPrm[chan].stDumpCfg;
    pstPackJpegCtrl = &gGlobalPrm.astXpackChnPrm[chan].stPackJpgCbCtrl;

    while (1)
    {
        SAL_mutexTmLock(&pstPackJpegCtrl->lstJpegEnc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (NULL == (pNode = DSA_LstGetHead(pstPackJpegCtrl->lstJpegEnc)))
        {
            SAL_CondWait(&pstPackJpegCtrl->lstJpegEnc->sync, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }
        SAL_mutexTmUnlock(&pstPackJpegCtrl->lstJpegEnc->sync.mid, __FUNCTION__, __LINE__);

        u32BgColor = gGlobalPrm.astXpackChnPrm[chan].u32DispBgColor;
        pstJpgEncNode = (PACK_JPEG_ENC_PRM *)pNode->pAdData;
        pstSrcImg = pstJpgEncNode->pstImg;
        u32InClipTop = pstJpgEncNode->u32InClipTop;
        u32InClipBot = pstJpgEncNode->u32InClipBot;

        if (SAL_TRUE == pstJpgEncNode->bJpegEncFinished)
        {
            // 已经编码完成但是还未删除的节点
            SAL_msleep(100);
            continue;
        }
        if (NULL == pstSrcImg)
        {
            XPACK_LOGE("chan %u encoding jpeg ximg is null\n", chan);
            goto EXIT;
        }
        if (u32InClipTop >= u32InClipBot || u32InClipTop > pstSrcImg->u32Height || u32InClipBot > pstSrcImg->u32Height)
        {
            XPACK_LOGE("Invalid pack top[%u] and bottom[%u], pack top should be less than bottom and both less than pack height[%u]\n", 
                    u32InClipTop, u32InClipBot, pstSrcImg->u32Height);
            goto EXIT;
        }
        pstSvaResult = pstJpgEncNode->pstAiDgrResult;
        pstJpegSvaResult = pstJpgEncNode->pstJpegSvaOut;
        // pstXspResult = pstJpgEncNode->pstXspIdtResult;
        pstResizedImg = &pstPackJpegCtrl->stImgResized;
        pstFilledImg = &pstPackJpegCtrl->stImgFilled;
        pstLocJpgCoorTrans = (NULL != pstJpgEncNode->pstJpgCoorTrans) ? pstJpgEncNode->pstJpgCoorTrans : &stJpgCoorTrans;

        u32ImgWidth = pstSrcImg->u32Width;
        u32ImgHeight = pstSrcImg->u32Height;
        u32InClipTop = SAL_align(u32InClipTop, 2);
        u32InClipBot = SAL_align(u32InClipBot, 2);
        if (SAL_TRUE == pstPackJpegCtrl->bJpgCropSwitch)
        {
            u32CropTop = u32InClipTop;
            u32CropBottom = u32InClipBot;
        }
        else
        {
            u32CropTop = 0;
            u32CropBottom = u32ImgHeight;
        }

        /* 计算原包裹宽高 */
        u32JpgWidthOri = SAL_align(u32ImgWidth, 2);
        u32JpgHeightOri = SAL_align(u32CropBottom - u32CropTop, 2);
        u32JpgWidthAligned = SAL_align(u32ImgWidth, 64);                // NT平台缩放需要宽和stride相等，rk平台缩放需要stride 64对齐
        u32JpgHeightAligned = SAL_align(u32ImgHeight, 2);

        ximg_set_dimension(pstSrcImg, u32JpgWidthAligned, pstSrcImg->u32Stride[0], SAL_FALSE, u32BgColor);

        if (pstDumpCfg->u32DumpCnt > 0 && pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_PACKJPG)
        {
            bDump = SAL_TRUE;
            ximg_dump(pstSrcImg, chan, pstDumpCfg->chDumpDir, "pack-cb-ori", NULL, pstJpgEncNode->PackId);
        }

        /* 背板宽高设置为0时使用原图宽高作为背板，且背板宽高不能超过最大值 */
        u32JpgWidthBackpanel = (0 == pstPackJpegCtrl->u32BackW) ? u32JpgWidthOri : pstPackJpegCtrl->u32BackW;
        u32JpgWidthBackpanel = SAL_MIN(u32JpgWidthBackpanel, XPACK_JPG_MAX_WIDTH);
        u32JpgHeightBackpanel = (0 == pstPackJpegCtrl->u32BackH) ? u32JpgHeightOri : pstPackJpegCtrl->u32BackH;
        u32JpgHeightBackpanel = SAL_MIN(u32JpgHeightBackpanel, XPACK_JPG_MAX_HEIGHT);

        /* 计算缩放后的包裹宽高，由于NT底层不支持抠图缩放，所以先将原图完整缩放后再抠取包裹部分 */
        u32JpgAlignedScaleW = SAL_align((UINT32)(pstPackJpegCtrl->f32WidthResizeRatio * u32JpgWidthAligned), 2);
        u32JpgAlignedScaleH = SAL_align((UINT32)(pstPackJpegCtrl->f32HeightResizeRatio * u32JpgHeightAligned), 2);
        u32JpgOriScaleW = SAL_align((UINT32)(pstPackJpegCtrl->f32WidthResizeRatio * u32JpgWidthOri), 2);
        u32JpgOriScaleH = SAL_align((UINT32)(pstPackJpegCtrl->f32HeightResizeRatio * u32JpgHeightOri), 2);

        /* 缩放后包裹宽高不允许超过背板宽高 */
        if (u32JpgOriScaleW > u32JpgWidthBackpanel)
        {
            u32JpgAlignedScaleW = SAL_align((UINT32)(u32JpgAlignedScaleW * u32JpgWidthBackpanel / u32JpgOriScaleW), 2);
            u32JpgAlignedScaleH = SAL_align((UINT32)(u32JpgAlignedScaleH * u32JpgWidthBackpanel / u32JpgOriScaleW), 2);
            u32JpgOriScaleH = SAL_align((UINT32)(u32JpgOriScaleH * u32JpgWidthBackpanel / u32JpgOriScaleW), 2);
            u32JpgOriScaleW = SAL_align((UINT32)(u32JpgOriScaleW * u32JpgWidthBackpanel / u32JpgOriScaleW), 2);
        }
        if (u32JpgOriScaleH > u32JpgHeightBackpanel)
        {
            u32JpgAlignedScaleW = SAL_align((UINT32)(u32JpgAlignedScaleW * u32JpgHeightBackpanel / u32JpgOriScaleH), 2);
            u32JpgAlignedScaleH = SAL_align((UINT32)(u32JpgAlignedScaleW * u32JpgHeightBackpanel / u32JpgOriScaleH), 2);
            u32JpgOriScaleW = SAL_align((UINT32)(u32JpgOriScaleW * u32JpgHeightBackpanel / u32JpgOriScaleH), 2);
            u32JpgOriScaleH = SAL_align((UINT32)(u32JpgOriScaleH * u32JpgHeightBackpanel / u32JpgOriScaleH), 2);
        }

        /* 先对整图做图像缩放 */
        if (u32JpgWidthOri != u32JpgOriScaleW || u32JpgHeightOri != u32JpgOriScaleH)
        {
            pstResizedImg->u32Height = u32JpgAlignedScaleH;
            /* 将图像宽度扩展为与stride相等，图像缩放时需要 */
            ximg_set_dimension(pstResizedImg, u32JpgAlignedScaleW, u32JpgAlignedScaleW, SAL_FALSE, u32BgColor);
            if (SAL_SOK != ximg_resize(pstSrcImg, pstResizedImg))  // scale
            {
                goto EXIT;
            }

            if (SAL_TRUE == bDump)
            {
                ximg_dump(pstResizedImg, chan, pstDumpCfg->chDumpDir, "pack-cb-resize", NULL, pstJpgEncNode->PackId);
            }
        }
        else
        {
            pstResizedImg = pstSrcImg;
        }

        /* 确定最终真实缩放比例和实际转换为jpg部分缩放后的宽高 */
        fRatioW = 1.0f * pstResizedImg->u32Width / u32JpgWidthAligned;
        fRatioH = 1.0f * pstResizedImg->u32Height / u32JpgHeightAligned;
        u32JpgOriScaleW = (UINT32)(fRatioW * u32JpgWidthOri);
        u32JpgOriScaleH = (UINT32)(fRatioH * u32JpgHeightOri);

        /* 抠图和图像背板填充 */
        stCropSrc.left = 0;
        stCropSrc.top = SAL_align((UINT32)(u32CropTop * fRatioH), 2);
        stCropDst.width  = stCropSrc.width  = SAL_MIN(u32JpgOriScaleW, pstResizedImg->u32Width);
        stCropDst.height = stCropSrc.height = SAL_MIN(u32JpgOriScaleH, pstResizedImg->u32Height - stCropSrc.top);

        pstFilledImg->u32Height = u32JpgHeightBackpanel;
        ximg_set_dimension(pstFilledImg, u32JpgWidthBackpanel, u32JpgWidthBackpanel, SAL_FALSE, u32BgColor);
        stCropDst.left = SAL_align(SAL_SUB_SAFE(pstFilledImg->u32Width, stCropSrc.width) / 2, 2);
        stCropDst.top = SAL_align(SAL_SUB_SAFE(pstFilledImg->u32Height, stCropSrc.height) / 2, 2);
        ximg_fill_color(pstFilledImg, 0, pstFilledImg->u32Height, 0, pstFilledImg->u32Width, u32BgColor);
        if (SAL_SOK != ximg_crop(pstResizedImg, pstFilledImg, &stCropSrc, &stCropDst))  // fill blank
        {
            goto EXIT;
        }
        if (SAL_TRUE == bDump)
        {
            ximg_dump(pstFilledImg, chan, pstDumpCfg->chDumpDir, "pack-cb-fill", NULL, pstJpgEncNode->PackId);
        }

        /* 智能坐标转换 */
        pstLocJpgCoorTrans->u32PackWidth = u32ImgWidth;
        pstLocJpgCoorTrans->u32PackHeight = u32ImgHeight;
        pstLocJpgCoorTrans->u32JpgCropedWidth = u32JpgWidthOri;
        pstLocJpgCoorTrans->u32JpgCropedHeight = u32JpgHeightOri;
        pstLocJpgCoorTrans->u32JpgScaleWidth = u32JpgOriScaleW;
        pstLocJpgCoorTrans->u32JpgScaleHeight = u32JpgOriScaleH;
        pstLocJpgCoorTrans->u32BackWidth = u32JpgWidthBackpanel;
        pstLocJpgCoorTrans->u32BackHeight = u32JpgHeightBackpanel;
        pstLocJpgCoorTrans->u32PackTop = u32InClipTop;
        pstLocJpgCoorTrans->u32CropTop = u32CropTop;
        if (NULL != pstSvaResult && NULL != pstJpegSvaResult)
        {
            if (0 < pstSvaResult->target_num)
            {
                /* 坐标转换 */
                if (SAL_SOK != xpack_trans_jpgcb_coor(pstSvaResult, pstLocJpgCoorTrans, pstJpegSvaResult, &stProfileBorder))
                {
                    pstJpegSvaResult->target_num = 0;  /* 坐标转换错误的时候就不给平台坐标数据，不给包裹进行叠框  */
                    XPACK_LOGW("xpack_trans_jpgcb_coor failed, raw:w %d, h %d, ori:w %d, h %d, scaled:w %d, h %d, backpanel:w %d, h %d\n", 
                            u32ImgWidth, u32ImgHeight, u32JpgWidthOri, u32JpgHeightOri, u32JpgOriScaleW, u32JpgOriScaleH, u32JpgWidthBackpanel, u32JpgHeightBackpanel);
                    goto EXIT;
                }

                /* 智能叠框 */    
                if (pstPackJpegCtrl->bJpgDrawSwitch)
                {
                    stPackOsd.u32Id = 0; /* 设置ID为无效值0 */
                    stPackOsd.pstImgOsd = pstFilledImg;
                    stPackOsd.stToiLimitedArea.uiX = 0;
                    stPackOsd.stToiLimitedArea.uiY = 0;
                    stPackOsd.stToiLimitedArea.uiWidth = pstFilledImg->u32Width;
                    stPackOsd.stToiLimitedArea.uiHeight = pstFilledImg->u32Height;
                    memcpy(&stPackOsd.stProfileBorder, &stProfileBorder, sizeof(XIMG_BORDER));
                    stPackOsd.pstAiDgrResult = pstJpegSvaResult;
                    stPackOsd.pstXrIdtResult = NULL;
                    stPackOsd.pstLabelTip = NULL;
                    if (0 > Xpack_DrvDrawOsdOnPackage(0, &stPackOsd, SAL_FALSE, 0))
                    {
                        goto EXIT;
                    }
                    if (SAL_TRUE == bDump)
                    {
                        ximg_dump(pstFilledImg, chan, pstDumpCfg->chDumpDir, "pack-cb-osd", NULL, pstJpgEncNode->PackId);
                    }
                }
            }
        }

        /* jpeg编码 */
        if (SAL_SOK != ximg_save(pstFilledImg, XRAY_TRANS_JPG, pstPackJpegCtrl->pPicJpeg, &pstPackJpegCtrl->u32JpegSize, 0))
        {
            goto EXIT;
        }
        pstPackJpegCtrl->u32JpegWidth = pstFilledImg->u32Width;
        pstPackJpegCtrl->u32JpegHeight = pstFilledImg->u32Height;
        if (SAL_TRUE == bDump)
        {
            snprintf(dumpName, sizeof(dumpName), "%s/packcb_no%u_w%u_h%u.jpg", 
                    pstDumpCfg->chDumpDir, pstJpgEncNode->PackId, pstPackJpegCtrl->u32JpegWidth, pstPackJpegCtrl->u32JpegHeight);
            SAL_WriteToFile(dumpName, 0, SEEK_SET, pstPackJpegCtrl->pPicJpeg, pstPackJpegCtrl->u32JpegSize);
        }

        goto NORMAL_EXIT;

    EXIT:
        XPACK_LOGE("Pack jpeg enc proc failed, pack[w:%u h:%u], JpgOri[w:%u h:%u], JpgScaled[w:%u h:%u], BackPanel[w:%u h:%u], ratio[w:%.2f->%.2f h:%.2f->%.2f]\n", 
                u32ImgWidth, u32ImgHeight, u32JpgWidthOri, u32JpgHeightOri, 
                u32JpgOriScaleW, u32JpgOriScaleH, u32JpgWidthBackpanel, u32JpgHeightBackpanel, 
                pstPackJpegCtrl->f32WidthResizeRatio, fRatioW, pstPackJpegCtrl->f32HeightResizeRatio, fRatioH);

    NORMAL_EXIT:
        pstJpgEncNode->bJpegEncFinished = SAL_TRUE;
        if (SAL_SOK == SAL_mutexTmLock(&pstJpgEncNode->condJpegEnc.mid, 100, __FUNCTION__, __LINE__))
        {
            SAL_CondSignal(&pstJpgEncNode->condJpegEnc, SAL_COND_ST_BROADCAST,  __FUNCTION__, __LINE__);
            SAL_mutexUnlock(&pstJpgEncNode->condJpegEnc.mid);
        }
    }
    return NULL;
}


/**
 * @fn      xpack_package_launch_aidgr
 * @brief   获取AI危险品识别图像，发起AI危险品识别，AI Dangerous Goods Recognition
 * 
 * @param   [IN] u32Chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstSysFrame 系统帧信息，除输出AI-YUV数据及其属性，还有pts和frameNum 
 *               pts和frameNum需和智能识别结果一起回传 
 * 
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：获取失败
 */
SAL_STATUS xpack_package_launch_aidgr(UINT32 u32Chan, SYSTEM_FRAME_INFO *pstSysFrame)
{
    INT32 sRet = SAL_SOK;
    UINT32 u32AiDgrWSendBk = 0; // 用于备份分段识别时当前已识别的包裹宽度
    DSA_NODE *pNode = NULL;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    CAPB_XPACK *pstCapbXpack = capb_get_xpack();
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;
    XIMAGE_DATA_ST stAiDgrImg = {0}, *pstAiDgrImg = NULL;
    SAL_SYSFRAME_PRIVATE stSysFrmPriv = {0};
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL;
    XPACK_DUMP_CFG *pstDumpCfg = NULL;

    if (u32Chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", u32Chan, pstCapbXray->xray_in_chan_cnt-1);
        return SAL_FAIL;
    }

    if (NULL == pstSysFrame)
    {
        XPACK_LOGE("xpack chan %u, the pstSysFrame is NULL\n", u32Chan);
        return SAL_FAIL;
    }

    pstPkgCtrl = gstRtPackageCtrl + u32Chan;
    if (pstPkgCtrl->lstPackage == NULL)
    {
        XPACK_LOGD("xpack chan %u, pstPkgCtrl->lstPackage is NULL, wait Xpack_drvInit!\n", u32Chan);
        return SAL_FAIL;
    }
    pstDumpCfg = &gGlobalPrm.astXpackChnPrm[u32Chan].stDumpCfg;

    SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    pNode = DSA_LstGetHead(pstPkgCtrl->lstPackage);
    while (NULL != pNode)
    {
        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
        if (pstPkgAttr->s32AiDgrBufIdx >= 0)
        {
            pstAiDgrImg = pstPkgCtrl->stAiDgrBuf + pstPkgAttr->s32AiDgrBufIdx;
            if ((!pstPkgAttr->bCompleted && SAL_SUB_SAFE(pstAiDgrImg->u32Width, pstPkgAttr->u32AiDgrWSend) >= pstCapbXpack->u32AiDgrSegLen[xsp_get_current_rtspeed(u32Chan)]) || 
                (pstPkgAttr->bCompleted && pstPkgAttr->u32AiDgrWSend < pstAiDgrImg->u32Width))
            {
                XPACK_LOGI("xpack chan %u, Completed %d, AiDgrWSend %u, BufWidth %u, SegLen %u\n", u32Chan, pstPkgAttr->bCompleted, pstPkgAttr->u32AiDgrWSend, 
                           pstAiDgrImg->u32Width, pstCapbXpack->u32AiDgrSegLen[xsp_get_current_rtspeed(u32Chan)]);
                memcpy(&stAiDgrImg, pstAiDgrImg, sizeof(XIMAGE_DATA_ST)); // 参数本地化，规避同步问题，u32Width会在另一个线程中更新
                u32AiDgrWSendBk = pstPkgAttr->u32AiDgrWSend; // 备份当前已识别的宽度，若build system frame失败，则回退该值
                pstPkgAttr->u32AiDgrWSend = stAiDgrImg.u32Width;
                SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

                stSysFrmPriv.u64Pts = ((UINT64)pstPkgAttr->u32Id << 32) | stAiDgrImg.u32Width;
                //stSysFrmPriv.u32FrameNum = pstPkgAttr->u32Id;
                sRet = Sal_halBuildSysFrame(pstSysFrame, NULL, &stAiDgrImg, &stSysFrmPriv);
                if (SAL_SOK != sRet)
                {
                    SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                    pstPkgAttr->u32AiDgrWSend = u32AiDgrWSendBk;
                    SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);
                }

                if (pstDumpCfg->u32DumpCnt > 0 && pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_PACKAI)
                {
                    ximg_dump(&stAiDgrImg, u32Chan, pstDumpCfg->chDumpDir, "aidgr", NULL, pstPkgAttr->u32Id);
                }

                return sRet;
            }
            else if (pstPkgAttr->bCompleted && pstPkgAttr->u32AiDgrWSend == pstPkgAttr->u32AiDgrWTotal) // 清除SVA已经获取过的Buffer
            {
                pstAiDgrImg->u32Width = 0;
                pstAiDgrImg->u32Height = 0;
                pstPkgAttr->s32AiDgrBufIdx = -1;
            }
        }
        pNode = pNode->next;
    }
    SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

    return SAL_FAIL;
}


/**
 * @fn      xpack_package_launch_xridt
 * @brief   发起XRay难穿透、可疑有机物等识别，XRay Identify
 * 
 * @param   [IN] u32Chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstPkgAttr 包裹属性
 * 
 * @return  无
 */
static VOID xpack_package_launch_xridt(UINT32 u32Chan, XPACK_PACKAGE_ATTR *pstPkgAttr)
{
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;
    XIMAGE_DATA_ST *pstXrIdtBuf = NULL;
    SAL_SYSFRAME_PRIVATE stPriv = {0};

    if (u32Chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", u32Chan, pstCapbXray->xray_in_chan_cnt-1);
        return;
    }

    if (NULL == pstPkgAttr)
    {
        XPACK_LOGE("xpack chan %u, the pstPkgAttr is NULL\n", u32Chan);
        return;
    }

    pstPkgCtrl = gstRtPackageCtrl + u32Chan;

    if (Xsp_DrvPackageIdentifyIsRunning(u32Chan) && pstPkgAttr->s32XrIdtBufIdx >= 0)
    {
        pstXrIdtBuf = pstPkgCtrl->stXrIdtBuf + pstPkgAttr->s32XrIdtBufIdx;
        stPriv.u32FrameNum = pstPkgAttr->u32Id;
        if (SAL_SOK == Xsp_DrvPackageIdentifySend(u32Chan, pstXrIdtBuf, &stPriv, 0))
        {
            pstPkgAttr->enXrIdtStage = XPACK_XAI_STAGE_START;
        }
        else
        {
            pstPkgAttr->enXrIdtStage = XPACK_XAI_STAGE_NONE; // 发起识别失败，重置状态
        }
    }

    return;
}


/**
 * @fn      xpack_package_release_xridt
 * @brief   释放用于XRay难穿透、可疑有机物等识别的NRaw数据Buffer
 * 
 * @param   [IN] u32Chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstPriv Xsp_DrvPackageIdentifySend()输入的XPack透传信息
 */
VOID xpack_package_release_xridt(UINT32 u32Chan, SAL_SYSFRAME_PRIVATE *pstPriv)
{
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;
    DSA_NODE *pNode = NULL;
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL;
    XIMAGE_DATA_ST *pstXrIdtBuf = NULL;

    if (u32Chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", u32Chan, pstCapbXray->xray_in_chan_cnt-1);
        return;
    }

    if (NULL == pstPriv)
    {
        XPACK_LOGE("xpack chan %u, the pstPriv is NULL\n", u32Chan);
        return;
    }

    pstPkgCtrl = gstRtPackageCtrl + u32Chan;
    SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    pNode = DSA_LstGetTail(pstPkgCtrl->lstPackage);
    while (NULL != pNode) // 从队列后往前查找，检索速率会快一些
    {
        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
        if (pstPriv->u32FrameNum == pstPkgAttr->u32Id)
        {
            if (pstPkgAttr->bCompleted && pstPkgAttr->s32XrIdtBufIdx >= 0)
            {
                pstXrIdtBuf = pstPkgCtrl->stXrIdtBuf + pstPkgAttr->s32XrIdtBufIdx;
                pstXrIdtBuf->u32Height = 0;
                pstXrIdtBuf->u32Width = 0;
                pstPkgAttr->s32XrIdtBufIdx = -1;
            }
            else
            {
                XPACK_LOGW("xpack chan %u, current package is uncompleted: %d %d\n", 
                           u32Chan, pstPkgAttr->bCompleted, pstPkgAttr->s32XrIdtBufIdx);
            }
            break;
        }
        pNode = pNode->prev;
    }
    SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

    return;
}


/**
 * @fn      xpack_rt_package_is_on_screen
 * @brief   预览过包时检测包裹是否在屏幕上
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstPkgAttr 包裹属性
 * 
 * @return  BOOL SAL_TRUE：在屏幕上，SAL_FALSE：不在屏幕上
 */
static BOOL xpack_rt_package_is_on_screen(UINT32 chan, XPACK_PACKAGE_ATTR *pstPkgAttr)
{
    UINT64 u64DispRLocOut = 0;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XPACK_CHN_PRM *pXpackChnPrm = NULL;

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return SAL_FALSE;
    }

    if (NULL == pstPkgAttr)
    {
        XPACK_LOGE("xpack chan %u, pstPkgAttr is NULL\n", chan);
        return SAL_FALSE;
    }
    pXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];
    if(XRAY_PS_RTPREVIEW == pXpackChnPrm->enProcStat)
    {
        // 获取当前屏幕的显示区域，从屏幕上最旧的开始，到当前写索引
        if (ring_buf_size(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], NULL, NULL, &u64DispRLocOut, NULL) < 0)
        {
            XPACK_LOGE("chan %u, ring_buf_size failed\n", chan);
            return SAL_FALSE;
        }

        // 当前包裹在显示图像中
        return (pstPkgAttr->u64DispLocEnd > u64DispRLocOut) ? SAL_TRUE : SAL_FALSE;
    }
    else
    {
        return SAL_TRUE;
    }
}

/**
 * @fn      xpack_package_store_cb_thread
 * @brief   实时预览包裹回调线程，获取包裹的识别结果也在该线程中执行
 * 
 * @param   [IN] pThrParam 实时预览包裹管理
 */
static void *xpack_package_store_cb_thread(VOID *pThrParam)
{
    UINT32 i = 0, j = 0, chan = 0;
    UINT32 u32AiDgrWidth = 0;
    UINT64 u64CbTime = 0;
    CHAR sDbgString[256] = {0};
    DSA_NODE *pNode = NULL, *pNodeMatched = NULL;
    SAL_VideoCrop stCropPrm = {0};
    XIMAGE_DATA_ST stPackDispImg = {0};
    XPACK_PACKAGE_CTRL *pstPkgCtrl = pThrParam;
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL, *pstPkgMatched = NULL;
    SVA_PROCESS_OUT_DATA *pstSvaOut = NULL;
    STREAM_ELEMENT stStrmElem = {0};
    XPACK_RESULT_ST stPackageCb = {0};
    SAL_SYSFRAME_PRIVATE stPriv = {0};

    DSA_NODE *pJpgEncNode = NULL;
    PACK_JPEG_ENC_PRM *pstJpegEncPrm = NULL;
    PACK_JPG_CB_CTRL *pstPackJpgCbCtrl = NULL;
    XPACK_SVA_RESULT_OUT stJpegSvaOut = {0};
    XPACK_COOR_TRANS stJpgCoorTrans = {0};

    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    CAPB_XSP *pstCapXsp = capb_get_xsp();
    CAPB_XPACK *pstCapXpack = capb_get_xpack();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    MEDIA_BUF_ATTR stSvaMb = {
        .memSize = sizeof(SVA_PROCESS_OUT_DATA),
        .bCached = SAL_TRUE,
    };
    XIMAGE_DATA_ST stPackOriNrawImg = {0};

    if (SAL_SOK == xpack_media_buf_alloc(&stSvaMb, "svaout-cb"))
    {
        pstSvaOut = (SVA_PROCESS_OUT_DATA *)stSvaMb.virAddr;
    }
    else
    {
        XPACK_LOGE("xpack_media_buf_alloc failed, buffer size: %zu\n", sizeof(SVA_PROCESS_OUT_DATA));
        return NULL;
    }

    for (i = 0; i < pstCapbXray->xray_in_chan_cnt; i++)
    {
        if (gstRtPackageCtrl + i == pstPkgCtrl)
        {
            chan = i;
            break;
        }
    }
    if (i == pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("cannot find matched xpack chan\n");
        return NULL;
    }

    stPackDispImg.stMbAttr.bCached = SAL_FALSE;
    if (SAL_SOK != ximg_create(SAL_align(pstCapXsp->xsp_package_line_max, 64) * pstCapXsp->resize_height_factor, pstCapXsp->xraw_width_resized, pstCapXsp->enDispFmt, "pack_jpeg_cb", NULL, &stPackDispImg))
    {
        XPACK_LOGE("ximg_create failed\n");
        return NULL;
    }
    
    stPackOriNrawImg.stMbAttr.bCached = SAL_FALSE;
    if (SAL_SOK != ximg_create(pstCapbXray->xraw_width_max, pstCapXsp->xsp_package_line_max, DSP_IMG_DATFMT_LHZ16P, "pack_OriNraw_cb", NULL, &stPackOriNrawImg))
    {
        XPACK_LOGE("ximg_create failed\n");
        return NULL;
    }

    pstPackJpgCbCtrl = &gGlobalPrm.astXpackChnPrm[chan].stPackJpgCbCtrl;
    while (1)
    {
        /**
         * 包裹的存储回调优先级最高，显示在屏幕上优先级次之，且智能识别结果与回调的保持一致 
         * 即：若回调时无智能识别结果，即使后续获得了该包裹的智能识别结果，仍不会叠加OSD标识 
         * 包裹的回调必须按时间顺序的，ID号较小的先回调，较大的后回调 
         */
        SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        pNode = DSA_LstGetHead(pstPkgCtrl->lstPackage);
        while (NULL != pNode)
        {
            pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
            if (pstPkgAttr->bStoredCb) // 若已回调，判断该包裹是否还在屏幕上，若不在则删除，并取下一个包裹
            {
                pNode = pNode->next;
                if (!xpack_rt_package_is_on_screen(chan, pstPkgAttr)) // 已出屏幕
                {
                    DSA_LstPop(pstPkgCtrl->lstPackage);
                }
            }
            else
            {
                break;
            }
        }
        SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

        // 尝试获取识别结果
        if (NULL != pNode)
        {
            /* 完整包裹且jpeg图像不需要智能叠框且jpeg图像未编码 */
            if (pstPkgAttr->bCompleted && SAL_FALSE == pstPkgAttr->bJpegEncStart)
            {
                stCropPrm.top = pstCapbDisp->disp_h_top_blanking + pstCapbDisp->disp_h_top_padding;
                stCropPrm.left = 0;
                stCropPrm.width = SAL_align(pstPkgAttr->u32RawHeight, 2);
                stCropPrm.height = pstPkgAttr->u32RawWidth;
                stPackDispImg.u32Height = stCropPrm.height;
                /* 将图像stride设置为64对齐，图像缩放时需要 */
                ximg_set_dimension(&stPackDispImg, stCropPrm.width, SAL_align(stCropPrm.width, 64), SAL_FALSE, gGlobalPrm.astXpackChnPrm[chan].u32DispBgColor);
                ring_buf_crop(gXpackRingBufHandle[chan][XPACK_DISPDT_PURE], pstPkgAttr->u64DispLocStart, 0, &stCropPrm, &stPackDispImg);

                SAL_mutexTmLock(&pstPackJpgCbCtrl->lstJpegEnc->sync.mid, 100, __FUNCTION__, __LINE__);
                pJpgEncNode = DSA_LstGetIdleNode(pstPackJpgCbCtrl->lstJpegEnc);
                SAL_mutexTmUnlock(&pstPackJpgCbCtrl->lstJpegEnc->sync.mid, __FUNCTION__, __LINE__);
                if (NULL != pJpgEncNode)
                {
                    pstJpegEncPrm = (PACK_JPEG_ENC_PRM *)pJpgEncNode->pAdData;

                    pstJpegEncPrm->PackId = pstPkgAttr->u32Id;
                    pstJpegEncPrm->u32InClipTop = pstPkgAttr->stPackDivInfo.uiPackTop;
                    pstJpegEncPrm->u32InClipBot = pstPkgAttr->stPackDivInfo.uiPackBottom;
                    pstJpegEncPrm->pstImg = &stPackDispImg;
                    pstJpegEncPrm->bJpegEncFinished = SAL_FALSE;
                    pstJpegEncPrm->pstJpgCoorTrans = &stJpgCoorTrans;
                    /* 
                     * jpeg上不需要叠框时，直接在包裹智能结果未出来之前进行jpeg编码，和包裹智能识别同步进行处理，节省包裹回调耗时
                     * 包裹智能识别完成且jpeg编码完成后，再将原包裹智能坐标转换为相对于jpeg图像上的智能坐标
                     */
                    if (SAL_FALSE == pstPackJpgCbCtrl->bJpgDrawSwitch)
                    {
                        pstPkgAttr->bJpegEncStart = SAL_TRUE;
                        pstJpegEncPrm->pstAiDgrResult = NULL;
                        pstJpegEncPrm->pstXspIdtResult = NULL;
                        pstJpegEncPrm->pstJpegSvaOut = NULL;

                        DSA_LstPush(pstPackJpgCbCtrl->lstJpegEnc, pJpgEncNode); /* 将数据节点PUSH进队列，jpen编码开始处理 */
                        SAL_mutexTmLock(&pstPackJpgCbCtrl->lstJpegEnc->sync.mid, 100, __FUNCTION__, __LINE__);
                        SAL_CondSignal(&pstPackJpgCbCtrl->lstJpegEnc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
                        SAL_mutexTmUnlock(&pstPackJpgCbCtrl->lstJpegEnc->sync.mid, __FUNCTION__, __LINE__);
                    }
                }
            }

            if (SAL_SOK == Sva_DrvGetPackResFromEngine(chan, pstSvaOut))
            {
                SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                pNodeMatched = pNode; // 从当前未回调的包裹开始查找，一般均能命中，但不排除异常情况，先送入识别的后输出结果
                while (NULL != pNodeMatched)
                {
                    pstPkgMatched = (XPACK_PACKAGE_ATTR *)pNodeMatched->pAdData;
                    // 匹配时间戳与帧序号
                    if (pstPkgMatched->u32Id == (pstSvaOut->uiSvaProcessRes.frame_stamp >> 32)) // Bingo the Package
                    {
                        // 识别的包裹宽度大于当前已做完的，且不超过已发起的，认定为有效值
                        u32AiDgrWidth = pstSvaOut->uiSvaProcessRes.frame_stamp & 0xFFFFFFFF;
                        if (u32AiDgrWidth > pstPkgMatched->u32AiDgrWRecv && u32AiDgrWidth <= pstPkgMatched->u32AiDgrWSend)
                        {
                            pstPkgMatched->u32AiDgrWRecv = u32AiDgrWidth;
                            pstPkgMatched->stAiDgrResult.target_num = SAL_MIN(pstSvaOut->uiSvaProcessRes.target_num, SVA_XSI_MAX_ALARM_NUM);
                            for (i = 0, j = 0; i < pstPkgMatched->stAiDgrResult.target_num; i++)
                            {
                                pstPkgMatched->stAiDgrResult.target[j].ID = pstSvaOut->uiSvaProcessRes.target[i].ID;
                                pstPkgMatched->stAiDgrResult.target[j].type = SAL_MIN(pstSvaOut->uiSvaProcessRes.target[i].type, SVA_MAX_ALARM_TYPE-1);
                                pstPkgMatched->stAiDgrResult.target[j].color = pstSvaOut->uiSvaProcessRes.target[i].color;
                                pstPkgMatched->stAiDgrResult.target[j].alarm_flg = pstSvaOut->uiSvaProcessRes.target[i].alarm_flg;
                                pstPkgMatched->stAiDgrResult.target[j].confidence = pstSvaOut->uiSvaProcessRes.target[i].confidence;
                                pstPkgMatched->stAiDgrResult.target[j].visual_confidence = SAL_MIN(pstSvaOut->uiSvaProcessRes.target[i].visual_confidence, 99);
                                memcpy(&pstPkgMatched->stAiDgrResult.target[j].rect, &pstSvaOut->uiSvaProcessRes.raw_target[i].rect, sizeof(SVA_RECT_F));
                                ximg_rect_fix_bounds_f(&pstPkgMatched->stAiDgrResult.target[j].rect, u32AiDgrWidth, pstCapXpack->u32AiDgrHeight);
                                if (XRAY_DIRECTION_RIGHT == pstPkgAttr->stPackDivInfo.uiNoramlDirection) // L2R时，送入识别的图像做了水平镜像，因此需将目标区域镜像回来
                                {
                                    pstPkgMatched->stAiDgrResult.target[j].rect.x = 1.0 - 
                                        pstPkgMatched->stAiDgrResult.target[j].rect.x - pstPkgMatched->stAiDgrResult.target[j].rect.width;
                                }
                                XPACK_LOGD("xpack chan %u, target: %u/%u, type %u, color 0x%x, conf %u, rect:[%f,%f %fx%f]\n",
                                           chan, i + 1, pstPkgMatched->stAiDgrResult.target_num, pstPkgMatched->stAiDgrResult.target[j].type,
                                           pstPkgMatched->stAiDgrResult.target[j].color, pstPkgMatched->stAiDgrResult.target[j].visual_confidence,
                                           pstPkgMatched->stAiDgrResult.target[j].rect.x, pstPkgMatched->stAiDgrResult.target[j].rect.y,
                                           pstPkgMatched->stAiDgrResult.target[j].rect.width, pstPkgMatched->stAiDgrResult.target[j].rect.height);
                                if (pstPkgMatched->stAiDgrResult.target[j].rect.width * u32AiDgrWidth * /* 过滤太小的识别目标 */
                                    pstPkgMatched->stAiDgrResult.target[j].rect.height * pstCapXpack->u32AiDgrHeight > XPACK_TOI_REGION_MIN)
                                {
                                    j++;
                                }
                                else
                                {
                                    XPACK_LOGW("xpack chan %u, drop this small target: %u/%u, type %u, rect-ori %f x %f, rect-fixed %f x %f, package size %u x %u\n",
                                               chan, i + 1, pstPkgMatched->stAiDgrResult.target_num, pstPkgMatched->stAiDgrResult.target[j].type, 
                                               pstSvaOut->uiSvaProcessRes.raw_target[i].rect.width, pstSvaOut->uiSvaProcessRes.raw_target[i].rect.height,
                                               pstPkgMatched->stAiDgrResult.target[j].rect.width, pstPkgMatched->stAiDgrResult.target[j].rect.height, 
                                               u32AiDgrWidth, pstCapXpack->u32AiDgrHeight);
                                }
                            }
                            pstPkgMatched->stAiDgrResult.target_num = j;
                        }
                        else
                        {
                            XPACK_LOGW("xpack chan %u, this sva result is invalid, id: %u/%u, should in range: (%u, %u]\n", chan, 
                                       pstPkgMatched->u32Id, u32AiDgrWidth, pstPkgMatched->u32AiDgrWRecv, pstPkgMatched->u32AiDgrWSend);
                        }

                        break;
                    }
                    pNodeMatched = pNodeMatched->next;
                }
                SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

                if (NULL == pNodeMatched)
                {
                    pNodeMatched = pNode;
                    sDbgString[0] = '\0';
                    while (NULL != pNodeMatched)
                    {
                        snprintf(sDbgString+strlen(sDbgString), sizeof(sDbgString)-strlen(sDbgString), "%u ", ((XPACK_PACKAGE_ATTR *)pNodeMatched->pAdData)->u32Id);
                        pNodeMatched = pNodeMatched->next;
                    }
                    XPACK_LOGE("xpack chan %u, cannot find matched package, sva id %llu, width %llu, current packages: %s\n",
                               chan, pstSvaOut->uiSvaProcessRes.frame_stamp >> 32, pstSvaOut->uiSvaProcessRes.frame_stamp & 0xFFFFFFFF, sDbgString);
                }
            }

            // XSP难穿透&可疑有机物识别仅在包裹完整后才会发起，且由XPack自身主动发起，这里的stPackageCb.igmPrm.stResultData作为了临时Buffer
            if (SAL_SOK == Xsp_DrvPackageIdentifyGetResults(chan, &stPackageCb.igmPrm.stResultData, &stPriv, 0)) 
            {
                SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                pNodeMatched = pNode; // 从当前包裹往后开始查找，一般均能命中，但不排除异常情况，先送入识别的后输出结果
                while (NULL != pNodeMatched)
                {
                    pstPkgMatched = (XPACK_PACKAGE_ATTR *)pNodeMatched->pAdData;
                    // 匹配时间戳与帧序号
                    if (pstPkgMatched->u32Id == stPriv.u32FrameNum) // Bingo the Package
                    {
                        if (XPACK_XAI_STAGE_START == pstPkgMatched->enXrIdtStage)
                        {
                            pstPkgMatched->enXrIdtStage = XPACK_XAI_STAGE_END;
                            memcpy(&pstPkgMatched->stXspIdtResult, &stPackageCb.igmPrm.stResultData, sizeof(XSP_RESULT_DATA));
                        }
                        break;
                    }
                    pNodeMatched = pNodeMatched->next;
                }
                SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);
                if (NULL == pNodeMatched)
                {
                    pNodeMatched = pNode;
                    sDbgString[0] = '\0';
                    while (NULL != pNodeMatched)
                    {
                        snprintf(sDbgString+strlen(sDbgString), sizeof(sDbgString)-strlen(sDbgString), "%u ", ((XPACK_PACKAGE_ATTR *)pNodeMatched->pAdData)->u32Id);
                        pNodeMatched = pNodeMatched->next;
                    }
                    XPACK_LOGE("xpack chan %u, cannot find matched package, xai id %u, current packages: %s\n", chan, stPriv.u32FrameNum, sDbgString);
                }
            }

            if (pstPkgAttr->bCompleted) // 包裹完整，再判断是否满足回调条件
            {
                u64CbTime = sal_get_tickcnt();
                if (((pstPkgAttr->u32AiDgrWRecv == pstPkgAttr->u32AiDgrWTotal) && // 危险品识别已启用且完成，或未启用
                     (pstPkgAttr->enXrIdtStage >= XPACK_XAI_STAGE_END || pstPkgAttr->enXrIdtStage == XPACK_XAI_STAGE_NONE)) // XSP难穿透&可疑有机物已启用且完成，或未启用
                    || (u64CbTime - pstPkgAttr->u64GenerateTime > 800)) // 超时强制回调，属于异常结束
                {
                    stStrmElem.type = STREAM_ELEMENT_PACKAGE_RESULT;
                    stStrmElem.chan = chan;

                    /* 需要在包裹jpeg中叠智能框，则在获取了智能结果之后再进行jpeg编码 */
                    if (SAL_FALSE == pstPkgAttr->bJpegEncStart && SAL_TRUE == pstPackJpgCbCtrl->bJpgDrawSwitch)
                    {
                        pstPkgAttr->bJpegEncStart = SAL_TRUE;
                        if (NULL != pJpgEncNode && NULL != pstJpegEncPrm)
                        {
                            pstJpegEncPrm->pstAiDgrResult = &pstPkgAttr->stAiDgrResult;
                            pstJpegEncPrm->pstXspIdtResult = &pstPkgAttr->stXspIdtResult;
                            pstJpegEncPrm->pstJpegSvaOut = &stJpegSvaOut;
                            DSA_LstPush(pstPackJpgCbCtrl->lstJpegEnc, pJpgEncNode); /* 将数据节点PUSH进队列，jpen编码开始处理 */
                            SAL_mutexTmLock(&pstPackJpgCbCtrl->lstJpegEnc->sync.mid, 100, __FUNCTION__, __LINE__);
                            SAL_CondSignal(&pstPackJpgCbCtrl->lstJpegEnc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
                            SAL_mutexTmUnlock(&pstPackJpgCbCtrl->lstJpegEnc->sync.mid, __FUNCTION__, __LINE__);
                        }
                    }

                    /* 图像基本信息与智能识别结果回调 */
                    stPackageCb.packIndx = pstPkgAttr->u32Id;
                    stPackageCb.width = pstPkgAttr->u32RawWidth;
                    stPackageCb.hight = pstPkgAttr->u32RawHeight;
                    if (pstPkgAttr->s32XrIdtBufIdx >= 0) // 回调的数据是固定宽高，内存是一整块
                    {
                        stPackageCb.uiOriXraySize = pstPkgCtrl->stXrIdtBuf[pstPkgAttr->s32XrIdtBufIdx].stMbAttr.memSize;
                        stPackageCb.uiXraySize = stPackageCb.width * stPackageCb.hight * pstCapXsp->xsp_normalized_raw_bw;
                        stPackageCb.cXrayAddr = pstPkgCtrl->stXrIdtBuf[pstPkgAttr->s32XrIdtBufIdx].stMbAttr.virAddr;
                    }
                    
                    memcpy(&stPackageCb.igmPrm.stResultData, &pstPkgAttr->stXspIdtResult, sizeof(XSP_RESULT_DATA));
                    memcpy(&stPackageCb.igmPrm.stTransferInfo, &pstPkgAttr->stPackDivInfo, sizeof(XSP_TRANSFER_INFO));
                    if (pstPkgAttr->u32AiDgrWRecv > 0 && pstPkgAttr->u32AiDgrWRecv < pstPkgAttr->u32AiDgrWTotal)
                    {
                        // TODO: 包裹仅识别了部分，将部分结果转为整包结果
                    }
                    else
                    {
                        if (pstPkgAttr->stAiDgrResult.target_num > 0)
                        {
                            memcpy(&stPackageCb.stPackSavPrm, &pstPkgAttr->stAiDgrResult, sizeof(XPACK_SVA_RESULT_OUT));
                        }
                        else
                        {
                            stPackageCb.stPackSavPrm.target_num = 0;
                        }
                    }

                    /* 等待jpeg编码完成 */
                    if (SAL_TRUE == pstPkgAttr->bJpegEncStart)
                    {
                        if (SAL_FALSE == pstJpegEncPrm->bJpegEncFinished)
                        {
                            if (SAL_SOK == SAL_mutexTmLock(&pstJpegEncPrm->condJpegEnc.mid, 100, __FUNCTION__, __LINE__))
                            {
                                // 等待jpeg编码完成，超时时间500ms
                                if (SAL_SOK != SAL_CondWait(&pstJpegEncPrm->condJpegEnc, 500,  __FUNCTION__, __LINE__))
                                {
                                    XPACK_LOGW("chan %u encoding package[ID:%u] jpeg timeout[500ms]\n", chan, pstPkgAttr->u32Id);
                                }
                                SAL_mutexUnlock(&pstJpegEncPrm->condJpegEnc.mid);
                            }
                        }
                        pstPkgAttr->bJpegEncStart = SAL_FALSE;
                        pstJpegEncPrm->bJpegEncFinished = SAL_TRUE;
                    }
                    if (SAL_TRUE == pstJpegEncPrm->bJpegEncFinished)
                    {
                        stPackageCb.stJpgResultOut.cJpegAddr = (UINT8 *)pstPackJpgCbCtrl->pPicJpeg;
                        stPackageCb.stJpgResultOut.uiJpegSize = pstPackJpgCbCtrl->u32JpegSize;
                        stPackageCb.stJpgResultOut.uiWidth = pstPackJpgCbCtrl->u32JpegWidth;
                        stPackageCb.stJpgResultOut.uiHeight = pstPackJpgCbCtrl->u32JpegHeight;
                        stPackageCb.stJpgResultOut.uiSyncNum = 0;
                        stPackageCb.stJpgResultOut.ullTimePts = stPackageCb.igmPrm.stTransferInfo.uiSyncTime;
                        // jpeg图像中没有智能叠框，jpeg编码完成后转换智能坐标
                        if (NULL != pstJpegEncPrm->pstJpgCoorTrans && 
                            NULL == pstJpegEncPrm->pstJpegSvaOut && 
                            0 < stPackageCb.stPackSavPrm.target_num)
                        {
                            if (SAL_SOK != xpack_trans_jpgcb_coor(&pstPkgAttr->stAiDgrResult, &stJpgCoorTrans, &stJpegSvaOut, NULL))
                            {
                                stPackageCb.stPackSavPrm.target_num = 0;  /* 坐标转换错误的时候就不给平台坐标数据，不给包裹进行叠框  */
                                XPACK_LOGW("xpack_trans_jpgcb_coor failed, raw:w %d, h %d, ori:w %d, h %d, scaled:w %d, h %d, backpanel:w %d, h %d\n", 
                                           stJpgCoorTrans.u32PackWidth, stJpgCoorTrans.u32PackHeight, stJpgCoorTrans.u32JpgCropedWidth, stJpgCoorTrans.u32JpgCropedHeight, 
                                           stJpgCoorTrans.u32JpgScaleWidth, stJpgCoorTrans.u32JpgScaleHeight, stJpgCoorTrans.u32BackWidth, stJpgCoorTrans.u32BackHeight);
                            }
                        }
                        memcpy(&stPackageCb.stJpgSavPrm, &stPackageCb.stPackSavPrm, sizeof(XPACK_SVA_RESULT_OUT));
                        for (i = 0; i < stPackageCb.stPackSavPrm.target_num; i++)
                        {
                            stPackageCb.stJpgSavPrm.target[i].rect.x = stJpegSvaOut.target[i].rect.x;
                            stPackageCb.stJpgSavPrm.target[i].rect.y = stJpegSvaOut.target[i].rect.y;
                            stPackageCb.stJpgSavPrm.target[i].rect.width = stJpegSvaOut.target[i].rect.width;
                            stPackageCb.stJpgSavPrm.target[i].rect.height = stJpegSvaOut.target[i].rect.height;
                        }
                    }

                    XPACK_LOGI("xpack chan %u, store callback start, ID %u, Width %u, Height %u, DetectNum %u + %u + %u, Gen-Cb-Delta %llu %llu %llu, "
                               "AiDgrBufIdx %d, WTotal-Send-Recv-OSD %u %u %u %u, XrIdtBufIdx %d, Stage %d\n", 
                               chan, stPackageCb.packIndx, stPackageCb.width, stPackageCb.hight, pstPkgAttr->stAiDgrResult.target_num, 
                               pstPkgAttr->stXspIdtResult.stUnpenSent.uiNum, pstPkgAttr->stXspIdtResult.stZeffSent.uiNum,
                               pstPkgAttr->u64GenerateTime, u64CbTime, u64CbTime - pstPkgAttr->u64GenerateTime, 
                               pstPkgAttr->s32AiDgrBufIdx, pstPkgAttr->u32AiDgrWTotal, pstPkgAttr->u32AiDgrWSend, 
                               pstPkgAttr->u32AiDgrWRecv, pstPkgAttr->u32AiDgrWOsd, pstPkgAttr->s32XrIdtBufIdx, pstPkgAttr->enXrIdtStage);
                    SystemPrm_cbFunProc(&stStrmElem, (UINT8 *)&stPackageCb, sizeof(XPACK_RESULT_ST));

                    pstPkgAttr->bStoredCb = SAL_TRUE;
                    if (pstPkgAttr->s32AiDgrBufIdx >= 0)
                    {
                        // 回调后释放包裹智能识别数据Buffer
                        SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, 100, __FUNCTION__, __LINE__);
                        pstPkgCtrl->stAiDgrBuf[pstPkgAttr->s32AiDgrBufIdx].u32Height = 0;
                        pstPkgCtrl->stAiDgrBuf[pstPkgAttr->s32AiDgrBufIdx].u32Width = 0;
                        pstPkgAttr->s32AiDgrBufIdx = -1;
                        SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);
                    }
                    if (pstPkgAttr->s32XrIdtBufIdx >= 0)
                    {
                        // 回调后释放包裹NRaw数据Buffer
                        SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, 100, __FUNCTION__, __LINE__);
                        pstPkgCtrl->stXrIdtBuf[pstPkgAttr->s32XrIdtBufIdx].u32Height = 0;
                        pstPkgCtrl->stXrIdtBuf[pstPkgAttr->s32XrIdtBufIdx].u32Width = 0;
                        pstPkgAttr->s32XrIdtBufIdx = -1;
                        SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);
                    }
                    if (NULL != pJpgEncNode)
                    {
                        SAL_mutexTmLock(&pstPackJpgCbCtrl->lstJpegEnc->sync.mid, 100, __FUNCTION__, __LINE__);
                        DSA_LstDelete(pstPackJpgCbCtrl->lstJpegEnc, pJpgEncNode); /* 从jpeg编码链表中删除该节点 */
                        SAL_mutexTmUnlock(&pstPackJpgCbCtrl->lstJpegEnc->sync.mid, __FUNCTION__, __LINE__);
                    }
                }
            }
        }

        sal_msleep_by_nano(20);
    }
}


/**
 * @fn      xpack_package_clear_rtlst
 * @brief   清空实时预览包裹管理队列，在切换过包方向、速度清屏时调用 
 * 
 * @param   [IN] u32Chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 */
static VOID xpack_package_clear_rtlst(UINT32 chan)
{
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL;
    DSA_NODE *pNode = NULL;
    XIMAGE_DATA_ST *pstImageData = NULL;
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return;
    }

    pstPkgCtrl = gstRtPackageCtrl + chan;
    if (pstPkgCtrl->lstPackage == NULL)
    {
        XPACK_LOGD("xpack chan %u, pstPkgCtrl->lstPackage is NULL, wait Xpack_drvInit!\n", chan);
        return;
    }

    SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    while (NULL != (pNode = DSA_LstGetHead(pstPkgCtrl->lstPackage)))
    {
        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
        if (pstPkgAttr->s32AiDgrBufIdx >= 0)
        {
            pstImageData = pstPkgCtrl->stAiDgrBuf + pstPkgAttr->s32AiDgrBufIdx;
            pstImageData->u32Width = 0; // 重置数据宽度，表示无数据
            pstImageData->u32Height = 0; // 重置数据高度，表示无数据
        }
        if (pstPkgAttr->s32XrIdtBufIdx >= 0)
        {
            pstImageData = pstPkgCtrl->stXrIdtBuf + pstPkgAttr->s32XrIdtBufIdx;
            pstImageData->u32Width = 0; // 重置数据宽度，表示无数据
            pstImageData->u32Height = 0; // 重置数据高度，表示无数据
        }
        if (NULL != pstPkgAttr->pTimerHandle)
        {
            SAL_TimerDestroy(pstPkgAttr->pTimerHandle);
            pstPkgAttr->pTimerHandle = NULL;
        }
        DSA_LstPop(pstPkgCtrl->lstPackage);
    }
    SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

    return;
}


/**
 * @fn      xpack_package_module_init
 * @brief   初始化包裹管理业务
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：失败
 */
static SAL_STATUS xpack_package_module_init(void)
{
    UINT32 i = 0, j = 0;
    DSA_NODE *pNode = NULL;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    XIMAGE_DATA_ST *pstImageData = NULL;
    DSP_IMG_DATFMT enImgFmt = DSP_IMG_DATFMT_UNKNOWN;
    SAL_ThrHndl threadHandle;

    for (i = 0; i < pstCapbXray->xray_in_chan_cnt; i++)
    {
        // 初始化实时预览包裹链表
        gstRtPackageCtrl[i].lstPackage = DSA_LstInit(XPACK_PACKAGE_ONSCN_NUM_MAX);
        if (NULL == gstRtPackageCtrl[i].lstPackage)
        {
            XPACK_LOGE("xpack chan %u, DSA_LstInit failed, node num: %d\n", i, XPACK_PACKAGE_ONSCN_NUM_MAX);
            return SAL_FAIL;
        }

        SAL_mutexTmLock(&gstRtPackageCtrl[i].lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        for (j = 0; j < XPACK_PACKAGE_ONSCN_NUM_MAX; j++)
        {
            if (NULL != (pNode = DSA_LstGetIdleNode(gstRtPackageCtrl[i].lstPackage)))
            {
                pNode->pAdData = gstRtPackageCtrl[i].stPackageAttr + j;
                DSA_LstPush(gstRtPackageCtrl[i].lstPackage, pNode);
            }
        }
        for (j = 0; j < XPACK_PACKAGE_ONSCN_NUM_MAX; j++)
        {
            DSA_LstPop(gstRtPackageCtrl[i].lstPackage);
        }
        SAL_mutexTmUnlock(&gstRtPackageCtrl[i].lstPackage->sync.mid, __FUNCTION__, __LINE__);

        // 初始化回拉包裹链表
        gstPbPackageCtrl[i].lstPackage = DSA_LstInit(XPACK_PACKAGE_ONSCN_NUM_MAX);
        if (NULL == gstPbPackageCtrl[i].lstPackage)
        {
            XPACK_LOGE("xpack chan %u, DSA_LstInit failed, node num: %d\n", i, XPACK_PACKAGE_ONSCN_NUM_MAX);
            return SAL_FAIL;
        }

        SAL_mutexTmLock(&gstPbPackageCtrl[i].lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        for (j = 0; j < XPACK_PACKAGE_ONSCN_NUM_MAX; j++)
        {
            if (NULL != (pNode = DSA_LstGetIdleNode(gstPbPackageCtrl[i].lstPackage)))
            {
                pNode->pAdData = gstPbPackageCtrl[i].stPackageAttr + j;
                DSA_LstPush(gstPbPackageCtrl[i].lstPackage, pNode);
            }
        }
        for (j = 0; j < XPACK_PACKAGE_ONSCN_NUM_MAX; j++)
        {
            DSA_LstPop(gstPbPackageCtrl[i].lstPackage);
        }
        SAL_mutexTmUnlock(&gstPbPackageCtrl[i].lstPackage->sync.mid, __FUNCTION__, __LINE__);

        // 初始化包裹危险品识别的Buffer
        for (j = 0; j < XPACK_PACKAGE_AIDGR_NUM_MAX; j++)
        {
            pstImageData = gstRtPackageCtrl[i].stAiDgrBuf + j;
            memset(pstImageData, 0x0, sizeof(XIMAGE_DATA_ST));
            pstImageData->stMbAttr.bCached = SAL_FALSE;
            if (SAL_SOK == ximg_create(pstCapbXsp->xsp_package_line_max, pstCapbXray->xraw_width_max, DSP_IMG_DATFMT_YUV420SP_VU, "AiDrg", NULL, pstImageData))
            {
                pstImageData->u32Width = 0; // 重置数据宽度，表示无数据
                pstImageData->u32Height = 0; // 重置数据高度，表示无数据
            }
            else
            {
                XPACK_LOGE("xpack chan %u, alloc for 'AiDrg' failed, idx: %u\n", i, j);
                return SAL_FAIL;
            }
        }

        // 初始化包裹XSP识别的Buffer
        for (j = 0; j < XPACK_PACKAGE_XRIDT_NUM_MAX; j++)
        {
            pstImageData = gstRtPackageCtrl[i].stXrIdtBuf + j;
            if (XRAY_ENERGY_SINGLE == pstCapbXray->xray_energy_num)
            {
                enImgFmt = DSP_IMG_DATFMT_SP16;
            }
            else
            {
                enImgFmt = DSP_IMG_DATFMT_LHZ16P;
            }
            if (SAL_SOK == ximg_create(pstCapbXray->xraw_width_max, pstCapbXsp->xsp_package_line_max, enImgFmt, "XrIdt", NULL, pstImageData))
            {
                pstImageData->u32Width = 0; // 重置数据宽度，表示无数据
                pstImageData->u32Height = 0; // 重置数据高度，表示无数据
            }
            else
            {
                XPACK_LOGE("xpack chan %u, alloc for 'XrIdt' failed, idx: %u\n", i, j);
                return SAL_FAIL;
            }
        }

        SAL_thrCreate(&threadHandle, xpack_package_store_cb_thread, SAL_THR_PRI_DEFAULT, 0, &gstRtPackageCtrl[i]);
    }

    return SAL_SOK;
}


/**
 * @fn      Xpack_DrvSetAiXrOsdShow
 * @brief   设置危险品识别与XSP难穿透&可疑有机物的OSD是否显示 
 * @note    开关TIP考核时调用，开启TIP考核时不显示危险品等辅助信息
 * 
 * @param   [IN] UINT32          chan        XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] DISP_SVA_SWITCH *pstDispOsd 仅dispSvaSwitch有效，OSD是否显示，0-不显示，1-显示
 * @param   [IN] BOOL            bShowTip    tip是否显示，0-不显示，1-显示
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvSetAiXrOsdShow(UINT32 chan, DISP_SVA_SWITCH *pstDispOsd, BOOL bShowTip)
{
    UINT64 u64DispImgLocS, u64DispImgLocE = 0;
    XPACK_SHOW_OSD_MODE enOsdShowModeSet = XPACK_SHOW_OSD_NONE;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;

    XPACK_CHECK_PTR_NULL(pstDispOsd, SAL_FAIL);
    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return SAL_FAIL;
    }

    pstXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];

    if (SAL_FALSE == pstDispOsd->dispSvaSwitch)
    {
        enOsdShowModeSet = (SAL_TRUE == bShowTip) ? XPACK_SHOW_OSD_TIP : XPACK_SHOW_OSD_NONE;
    }
    else
    {
        enOsdShowModeSet = XPACK_SHOW_OSD_ALL;
    }
    XPACK_LOGI("xpack chan %u, set showSvaSwitch %d, tip switch %d, show osd mode %d -> %d\n", 
               chan, pstDispOsd->dispSvaSwitch, bShowTip, pstXpackChnPrm->enOsdShowMode, enOsdShowModeSet);
    // 从XPACK_DISPDT_PURE拷贝一份纯图像数据，清除当前屏幕上所有的OSD
    if (pstXpackChnPrm->enOsdShowMode > enOsdShowModeSet)
    {
        if (ring_buf_size(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], NULL, NULL, &u64DispImgLocS, &u64DispImgLocE) < 0)
        {
            XPACK_LOGE("xpack chan %u, ring_buf_size failed\n", chan);
            return SAL_FAIL;
        }
        xpack_disp_clear_osd_by_pure(chan, (UINT32)SAL_SUB_SAFE(u64DispImgLocE, u64DispImgLocS), u64DispImgLocS);
    }
    /* 重新叠加OSD */
    else if (pstXpackChnPrm->enOsdShowMode < enOsdShowModeSet)
    {
        pstXpackChnPrm->enOsdShowMode = enOsdShowModeSet;
        pstXpackChnPrm->enOsdRedraw4Disp = XPACK_OSD_REDRAW_DIRECTLY;
    }

    pstXpackChnPrm->enOsdShowMode = enOsdShowModeSet;

    return SAL_SOK;
}


/**
 * @fn      Xpack_DrvSetAidgrLinePrm
 * @brief   设置危险品目标的画线参数
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstAiDgrLine 危险品目标的画线参数
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvSetAidgrLinePrm(UINT32 chan, DISPLINE_PRM *pstAiDgrLine)
{
    DISPLINE_PRM stAiDgrLineTmp = {0};
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    CAPB_DISP *pstCapbDisp = capb_get_disp();

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return SAL_FAIL;
    }

    if (pstAiDgrLine == NULL)
    {
        XPACK_LOGE("xpack chan %u, the pstAiDgrLine is NULL\n", chan);
        return SAL_FAIL;
    }

    if(pstAiDgrLine->frametype != DISP_FRAME_TYPE_DASHDOTLINE && pstAiDgrLine->node > 0)
    {
        XPACK_LOGW("xpack chan %u,node is invalid:%d, only DISP_FRAME_TYPE_DASHDOTLINE support set node.\n", chan, pstAiDgrLine->node);
        pstAiDgrLine->node = 0;
    }

    stAiDgrLineTmp.frametype = SAL_MIN(pstAiDgrLine->frametype, DISP_FRAME_TYPE_MAX-1); // 画线类型
    stAiDgrLineTmp.emDispAIDrawType = SAL_MIN(pstAiDgrLine->emDispAIDrawType, AI_DISP_DRAW_ONE_TYPE); // 是否合并
    stAiDgrLineTmp.fulllinelength = SAL_align(pstAiDgrLine->fulllinelength, 2); // 实线
    stAiDgrLineTmp.gaps = SAL_align(pstAiDgrLine->gaps, 2); // 虚线
    stAiDgrLineTmp.node = pstAiDgrLine->node; //点的数量

    if (gstAiDgrLineParam[chan].frametype == DISP_FRAME_TYPE_DOTTEDLINE || // 虚线
        gstAiDgrLineParam[chan].frametype == DISP_FRAME_TYPE_DASHDOTLINE) // 点线
    {
        if(pstAiDgrLine->frametype != DISP_FRAME_TYPE_FULLLINE)
        {
            if ((pstAiDgrLine->fulllinelength == 0) || (pstAiDgrLine->gaps == 0) || 
            (pstAiDgrLine->fulllinelength + pstAiDgrLine->gaps > SAL_MAX(pstCapbDisp->disp_yuv_w_max, pstCapbDisp->disp_yuv_h_max)))
            {
                XPACK_LOGE("xpack chan %u, fulllinelength(%u) OR gaps(%u) is invalid\n", chan, pstAiDgrLine->fulllinelength, pstAiDgrLine->gaps);
                return SAL_FAIL;
            }
        }
    }

    osd_func_writeStart();
    memcpy(&gstAiDgrLineParam[chan], &stAiDgrLineTmp, sizeof(DISPLINE_PRM));
    osd_func_writeEnd();

    return SAL_SOK;
}


/**
 * @fn      xpack_osd_get_aidgr_config
 * @brief   获取AI危险品识别的OSD配置
 * @note    危险品名称或颜色修改后，置OSD刷新标志并刷新屏幕上的所有包裹OSD 
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstOsdConfig AI危险品识别的OSD配置
 * 
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS xpack_osd_get_aidgr_config(UINT32 chan, XPACK_OSD_CONFIG_AIDGR *pstOsdConfig)
{
    SAL_STATUS s32Ret = SAL_SOK;
    UINT32 i = 0;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    SVA_PROCESS_IN stSvaCfg = {0};

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return SAL_FAIL;
    }
    if (NULL == pstOsdConfig)
    {
        XPACK_LOGE("xpack chan %u, pstOsdConfig is NULL\n", chan);
        return SAL_FAIL;
    }

    s32Ret = Sva_DrvGetCfgParam(0, &stSvaCfg); /* 不区分视角，均使用主视角参数 */
    if (s32Ret != SAL_SOK)
    {
        XPACK_LOGE("xpack chan %u, Sva_DrvGetCfgParam failed\n", chan);
        return SAL_FAIL;
    }

    /* 是否叠加置信度 */
    pstOsdConfig->bConfidence = (SVA_ALERT_EXT_NONE == stSvaCfg.stTargetPrm.enOsdExtType) ? SAL_FALSE : SAL_TRUE;

    /* 是否合并相同名字的危险品，注：跟随模式不支持合并 */
    if (SVA_OSD_NORMAL_TYPE != stSvaCfg.drawType && AI_DISP_DRAW_ONE_TYPE == gstAiDgrLineParam[chan].emDispAIDrawType)
    {
        pstOsdConfig->bMergeSameName = SAL_TRUE;
    }
    else
    {
        pstOsdConfig->bMergeSameName = SAL_FALSE;
    }

    /* 危险品颜色，默认颜色相同{deepred-0xB81612，blue-0x0000FF，yellow-0xFFFF00, green-0x00FF00}，有机物难穿透颜色不可修改 */
    for (i = 0; i < SVA_MAX_ALARM_TYPE; i++)
    {
        if (stSvaCfg.alert[i].bInit == SAL_TRUE) /* 危险品启用 */
        {
            pstOsdConfig->u32Color = stSvaCfg.alert[i].sva_color;
            break;
        }
    }

    /* 字体大小等级，取值范围{1, 2, 3}，1-小，2-中，3-大 */
    pstOsdConfig->u32ScaleLevel = SAL_CLIP(stSvaCfg.stTargetPrm.scaleLevel, 1, 3);

    /* 目标展现方式，取值范围{0, 1, 2, 3}，0-跟随，1-连线，2-点线，3-分离 */
    pstOsdConfig->enDrawMode = SAL_MIN(stSvaCfg.drawType, SVA_OSD_TYPE_NO_LINE_TYPE);

    /* xpack画OSD线相关参数配置 */
    memcpy(&pstOsdConfig->stAiDgrLine, &gstAiDgrLineParam[chan], sizeof(DISPLINE_PRM));

    /* 危险品名字 */
    for (i = 0; i < SVA_MAX_ALARM_TYPE; i++)
    {
        snprintf(pstOsdConfig->sAiDgrName[i], SVA_ALERT_NAME_LEN, "%s", (CHAR *)stSvaCfg.alert[i].sva_name);
    }

    return SAL_SOK;
}


/**
 * @fn      xpack_osd_merge_aidgr_by_name
 * @brief   合并相同名称（注：不是相同类型，不同类型可以有相同名称）的AI危险品目标
 * 
 * @param   [OUT] pstDst 合并后的AI危险品目标
 * @param   [IN] pstSrc 未合并的AI危险品目标
 * @param   [IN] pstOsdConfig AI危险品识别的OSD配置
 */
static VOID xpack_osd_merge_aidgr_by_name(XPACK_SVA_RESULT_OUT *pstDst, XPACK_SVA_RESULT_OUT *pstSrc, XPACK_OSD_CONFIG_AIDGR *pstOsdConfig)
{
    UINT32 i = 0, j = 0;
    UINT32 u32SrcType = 0, u32DstType = 0; // 源与目标危险品报警类型

    if (NULL == pstDst || NULL == pstSrc || NULL == pstOsdConfig)
    {
        XPACK_LOGE("pstDst(%p) OR pstSrc(%p) OR pstOsdConfig(%p) is NULL", pstDst, pstSrc, pstOsdConfig);
        return;
    }

    for (i = 0; i < pstSrc->target_num; i++)
    {
        u32SrcType = pstSrc->target[i].type;
        if (u32SrcType >= SVA_MAX_ALARM_TYPE)
        {
            continue;
        }

        for (j = 0; j < pstDst->target_num; j++) /* 遍历所有已存在的同类危险品 */
        {
            u32DstType = pstDst->target[j].type;
            if (0 == strcmp(pstOsdConfig->sAiDgrName[u32DstType], pstOsdConfig->sAiDgrName[u32SrcType])) /* Bingo!! */
            {
                break;
            }
        }

        if (j == pstDst->target_num) /* 该类型的危险品不存在，则创建 */
        {
            pstDst->target_num++;
            memcpy(&pstDst->target[j], &pstSrc->target[i], sizeof(SVA_DSP_ALERT));
            pstDst->target[j].ID = i; /* 复用为：最高置信度的危险品在智能识别结果pstSrc中的索引位置 */
            pstDst->target[j].alarm_flg = 1; /* 复用为：同名危险品的数量 */
        }
        else /* 该类型的危险品已经存在 */
        {
            /* 校验置信度，将高置信度的危险品替换低置信度的 */
            if (pstSrc->target[i].visual_confidence > pstDst->target[j].visual_confidence) // Src中的置信度更高
            {
                /* 清除Dst中低置信度的参数，并以现Src中高置信度的为标准 */
                pstDst->target[j].ID = i; /* 复用为：最高置信度的危险品在智能识别结果pstSrc中的索引位置 */
                pstDst->target[j].alarm_flg++; /* 复用为：同名危险品的数量 */
                pstDst->target[j].type = u32SrcType;
                pstDst->target[j].color = pstSrc->target[i].color;
                pstDst->target[j].confidence = pstSrc->target[i].confidence;
                pstDst->target[j].visual_confidence = pstSrc->target[i].visual_confidence;
                memcpy(&pstDst->target[j].rect, &pstSrc->target[i].rect, sizeof(SVA_RECT_F));
            }
            else // Dst中的同名危险品置信度更高，对Dst中同名危险品数+1
            {
                pstDst->target[j].alarm_flg++; /* 复用为：同名危险品的数量 */
            }
        }
    }

    return;
}


/**
 * @fn      xpack_osd_aidgr_2_toi
 * @brief   转换危险品识别结果为通用的OSD目标信息
 * 
 * @param   [OUT] pstToi OSD目标信息，包括目标区域和OSD文本Font，目标区域坐标是基于BaseW×BaseH的
 * @param   [IN] pstOsdConfig OSD配置
 * @param   [IN] pstAiDgrSrc 危险品识别结果
 * @param   [IN] u32BaseW 危险品目标区域基于的图像宽
 * @param   [IN] u32BaseH 危险品目标区域基于的图像高
 * 
 * @return  UINT32 OSD目标数量，0表示没有目标
 */
UINT32 xpack_osd_aidgr_2_toi(XPACK_OSD_TOI_LAYOUT *pstToi, XPACK_OSD_CONFIG_AIDGR *pstOsdConfig, XPACK_SVA_RESULT_OUT *pstAiDgrSrc, UINT32 u32BaseW, UINT32 u32BaseH)
{
    UINT32 i = 0, j = 0, k = 0;
    UINT32 u32ToiCnt = 0;
    UINT32 u32LatSize = 0, u32FontSize = 0; // 点阵字体与矢量字体大小
    CAPB_OSD *pstCapbOsd = capb_get_osd();
    BOOL bBgEnable = SAL_FALSE;
    CHAR sPostfix[16] = "( %) x "; // 危险品名称的后缀，包括置信度与复数
    OSD_REGION_S *pstFont = NULL;
    OSD_REGION_S *pstLat = NULL;
    XPACK_SVA_RESULT_OUT stAiDgrDst = {0}, *pstAiDgrDst = NULL, stAiDgrTmp = {0};
    XIMG_POINT stCentralPoint = {0}; // 矩形中心点
    UINT32 u32OsdColor1555 = 0;
    UINT32 u32BgColor1555 = 0;

    if (NULL == pstToi || NULL == pstAiDgrSrc || NULL == pstOsdConfig)
    {
        XPACK_LOGE("pstToi(%p) OR pstAiDgrSrc(%p) OR pstOsdConfig(%p) is NULL\n", pstToi, pstAiDgrSrc, pstOsdConfig);
        return 0;
    }
    if (u32BaseW * u32BaseH < XPACK_TOI_REGION_MIN)
    {
        XPACK_LOGE("XScanArea is too small, Width %u, Height %u, should be bigger than %d\n", 
                   u32BaseW, u32BaseH, XPACK_TOI_REGION_MIN);
        return 0;
    }

    for (i = 0, j = 0; i < pstAiDgrSrc->target_num; i++)
    {
        memcpy(&stAiDgrTmp.target[j], &pstAiDgrSrc->target[i], sizeof(SVA_DSP_ALERT)); /* 拷贝保护入参 */
        ximg_rect_fix_bounds_f(&stAiDgrTmp.target[j].rect, u32BaseW, u32BaseH);
        if ((stAiDgrTmp.target[j].rect.width * u32BaseW *  /* 过滤太小的识别目标 */
             stAiDgrTmp.target[j].rect.height * u32BaseH > XPACK_TOI_REGION_MIN) &&
            (stAiDgrTmp.target[i].type < SVA_MAX_ALARM_TYPE)) /* 危险品类型正常 */
        {
            j++;
        }
    }
    stAiDgrTmp.target_num = j;

    // 合并模式，合并同名危险品
    if (pstOsdConfig->bMergeSameName)
    {
        xpack_osd_merge_aidgr_by_name(&stAiDgrDst, &stAiDgrTmp, pstOsdConfig);
        pstAiDgrDst = &stAiDgrDst;
    }
    else
    {
        pstAiDgrDst = &stAiDgrTmp;
    }

    bBgEnable = (SVA_OSD_NORMAL_TYPE == pstOsdConfig->enDrawMode) ? SAL_FALSE : SAL_TRUE;
    if (OSD_FONT_TRUETYPE == pstCapbOsd->enFontType) // 矢量字体直接从能力集中获取字体大小
    {
        switch (pstOsdConfig->u32ScaleLevel)
        {
        case 2: // 中号字体
            u32LatSize = pstCapbOsd->TrueTypeSize[1];
            break;
        case 3: // 大号字体
            u32LatSize = pstCapbOsd->TrueTypeSize[2];
            break;
        default: // 默认为小号字体
            u32LatSize = pstCapbOsd->TrueTypeSize[0];
            break;
        }
        u32FontSize = u32LatSize;
    }
    else // 点阵字体以16Pixel为单位
    {
        u32LatSize = 16;
        u32FontSize = pstOsdConfig->u32ScaleLevel * 16;
    }

    for (i = 0; i < pstAiDgrDst->target_num; i++)
    {
        /* Reset参数 */
        memset(pstToi + u32ToiCnt, 0, sizeof(XPACK_OSD_TOI_LAYOUT));
        k = 0;

        /* 获取危险品名称字体及其尺寸 */
        if (OSD_FONT_TRUETYPE == pstCapbOsd->enFontType) // 矢量字体
        {
            // 危险品名称字体单元
            pstFont = osd_func_getFontRegion(OSD_BLOCK_IDX_STRING, pstAiDgrDst->target[i].type, u32LatSize, u32FontSize, bBgEnable);
            if (NULL != pstFont)
            {
                pstToi[u32ToiCnt].pstTextFont[k++] = pstFont;
                pstToi[u32ToiCnt].stTextRegion.uiWidth = pstFont->u32Width;
                pstToi[u32ToiCnt].stTextRegion.uiHeight = pstFont->u32Height;

                /* SVA_OSD_NORMAL_TYPE模式OSD无背景，目标颜色为字体颜色，其余模式目标颜色为背景色，字体白色 */
                if (SVA_OSD_NORMAL_TYPE == pstOsdConfig->enDrawMode)
                {
                    SAL_RGB24ToARGB1555(pstOsdConfig->u32Color, &u32OsdColor1555, 1);
                    u32BgColor1555 = OSD_BACK_COLOR;
                }
                else
                {
                    SAL_RGB24ToARGB1555(pstOsdConfig->u32Color, &u32BgColor1555, 1);
                    u32OsdColor1555 = OSD_COLOR_WHITE;
                }

                // 危险品后缀字体单元
                memset(sPostfix, 0, sizeof(sPostfix));
                if (pstOsdConfig->bConfidence) // 也可根据pstAiDgrResult->target[i].confidence这个值为每种危险品分别配置是否需要置信度
                {
                    snprintf(sPostfix, sizeof(sPostfix), "(%u%%)", SAL_MIN(pstAiDgrDst->target[i].visual_confidence, 99)); // 置信度最大为99
                }

                if (pstOsdConfig->bMergeSameName && pstAiDgrDst->target[i].alarm_flg > 1)
                {
                    snprintf(sPostfix+strlen(sPostfix), sizeof(sPostfix)-strlen(sPostfix), " x%u", pstAiDgrDst->target[i].alarm_flg);
                }

                for (j = 0; j < strlen(sPostfix) && k < XPACK_OSD_TEXT_FONT_NUM; j++)
                {
                    pstFont = osd_func_getFontRegion(OSD_BLOCK_IDX_ASCII, SAL_SUB_SAFE(sPostfix[j], OSD_CHAR_ASC_PRT_START), u32LatSize, u32FontSize, bBgEnable);
                    if (NULL != pstFont)
                    {
                        pstLat = osd_func_getLatRegion(OSD_BLOCK_IDX_ASCII, SAL_SUB_SAFE(sPostfix[j], OSD_CHAR_ASC_PRT_START), u32LatSize);
                        if (SAL_SOK != osd_func_FillFont(pstLat, u32LatSize, pstFont, u32FontSize, u32OsdColor1555, u32BgColor1555))
                        {
                            XPACK_LOGW("osd_func_FillFont failed, idx %d, code %u-0x%x, LatSize %u, FontSize %u\n",
                                       OSD_BLOCK_IDX_ASCII, j, sPostfix[j], u32LatSize, u32FontSize);
                        }
                        pstToi[u32ToiCnt].pstTextFont[k++] = pstFont;
                        pstToi[u32ToiCnt].stTextRegion.uiWidth += pstFont->u32Width;
                        pstToi[u32ToiCnt].stTextRegion.uiHeight = SAL_MAX(pstToi[u32ToiCnt].stTextRegion.uiHeight, pstFont->u32Height);
                    }
                    else
                    {
                        XPACK_LOGW("osd_func_getFontRegion failed, idx %d, code %u-0x%x, LatSize %u, FontSize %u\n", 
                                   OSD_BLOCK_IDX_ASCII, j, sPostfix[j], u32LatSize, u32FontSize);
                    }
                }

                /* 计算目标矩形框区域 */
                if (SVA_OSD_LINE_POINT_TYPE == pstOsdConfig->enDrawMode) // 点线模式，危险品名称OSD与其矩形框中心点对齐；其他模式与矩形框左边沿对齐
                {
                    // 先计算矩形中心点
                    stCentralPoint.u32X = (UINT32)((pstAiDgrDst->target[i].rect.x + pstAiDgrDst->target[i].rect.width / 2) * u32BaseW);
                    stCentralPoint.u32Y = (UINT32)((pstAiDgrDst->target[i].rect.y + pstAiDgrDst->target[i].rect.height / 2) * u32BaseH);
                    // 中心点不变，扩展到XPACK_OSD_TARGET_LABEL_RECT大的标识矩形
                    pstToi[u32ToiCnt].stBoxRegion.uiX = SAL_SUB_SAFE(stCentralPoint.u32X, (XPACK_OSD_TARGET_LABEL_RECT / 2));
                    pstToi[u32ToiCnt].stBoxRegion.uiY = SAL_SUB_SAFE(stCentralPoint.u32Y, (XPACK_OSD_TARGET_LABEL_RECT / 2));
                    pstToi[u32ToiCnt].stBoxRegion.uiX = SAL_alignDown(pstToi[u32ToiCnt].stBoxRegion.uiX, 2); // 向下2对齐，即坐标向原点靠近
                    pstToi[u32ToiCnt].stBoxRegion.uiY = SAL_alignDown(pstToi[u32ToiCnt].stBoxRegion.uiY, 2); // 向下2对齐，即坐标向原点靠近
                    pstToi[u32ToiCnt].stBoxRegion.uiWidth = pstToi[u32ToiCnt].stBoxRegion.uiHeight = XPACK_OSD_TARGET_LABEL_RECT;
                }
                else
                {
                    ximg_rect_f2u(&pstToi[u32ToiCnt].stBoxRegion, &pstAiDgrDst->target[i].rect, u32BaseW, u32BaseH);
                }

                pstToi[u32ToiCnt].u32TextFontCnt = k;
                pstToi[u32ToiCnt].u32TextColor = pstOsdConfig->u32Color;
                pstToi[u32ToiCnt].enDrawMode = pstOsdConfig->enDrawMode;
                u32ToiCnt++;
            }
            else
            {
                XPACK_LOGW("osd_func_getFontRegion failed, idx %d, code %u, LatSize %u, FontSize %u\n", 
                           OSD_BLOCK_IDX_STRING, pstAiDgrDst->target[i].type, u32LatSize, u32FontSize);
            }
        }
        else // 点阵字体
        {
            XPACK_LOGE("unsupport lattice!!!!\n");
        }
    }

    return u32ToiCnt;
}


/**
 * @fn      xpack_osd_xridt_2_toi
 * @brief   转换XSP难穿透&可疑有机物识别结果为通用的OSD目标信息
 * 
 * @param   [OUT] pstToi OSD目标信息，包括目标区域和OSD文本Font，目标区域坐标是基于BaseW×BaseH的
 * @param   [IN] pstXspIdtResult XSP难穿透&可疑有机物识别结果
 * @param   [IN] u32BaseW 难穿透&可疑有机物目标区域基于的图像宽
 * @param   [IN] u32BaseH 难穿透&可疑有机物目标区域基于的图像高
 * 
 * @return  UINT32 OSD目标数量，0表示没有目标
 */
UINT32 xpack_osd_xridt_2_toi(XPACK_OSD_TOI_LAYOUT *pstToi, XSP_RESULT_DATA *pstXspIdtResult, UINT32 u32BaseW, UINT32 u32BaseH)
{
    UINT32 i = 0, j = 0;
    UINT32 u32ToiCnt = 0;

    if (NULL == pstToi || NULL == pstXspIdtResult)
    {
        XPACK_LOGE("pstToi(%p) OR pstXspIdtResult(%p) is NULL\n", pstToi, pstXspIdtResult);
        return 0;
    }
    if (u32BaseW * u32BaseH < XPACK_TOI_REGION_MIN)
    {
        XPACK_LOGE("XScanArea is too small, Width %u, Height %u, should be bigger than %d\n", 
                   u32BaseW, u32BaseH, XPACK_TOI_REGION_MIN);
        return 0;
    }

    u32ToiCnt = SAL_MIN(pstXspIdtResult->stUnpenSent.uiNum, XSP_IDENTIFY_NUM);
    for (i = 0; i < u32ToiCnt; i++)
    {
        ximg_rect_f2u(&pstToi[i].stBoxRegion, &pstXspIdtResult->stUnpenSent.stRect[i], u32BaseW, u32BaseH);
        pstToi[i].u32TextColor = pstXspIdtResult->stUnpenSent.uiResult;
    }

    j = u32ToiCnt;
    u32ToiCnt += SAL_MIN(pstXspIdtResult->stZeffSent.uiNum, XSP_IDENTIFY_NUM);
    for (i = j, j = 0; i < u32ToiCnt; i++)
    {
        ximg_rect_f2u(&pstToi[i].stBoxRegion, &pstXspIdtResult->stZeffSent.stRect[j++], u32BaseW, u32BaseH);
        pstToi[i].u32TextColor = pstXspIdtResult->stZeffSent.uiResult;
    }
    for (i = 0; i < u32ToiCnt; i++)
    {
        XPACK_LOGT("%u/%u: Box (%u, %u) - %u x %u, Color 0x%x\n", i, u32ToiCnt, pstToi[i].stBoxRegion.uiX, pstToi[i].stBoxRegion.uiY, 
                   pstToi[i].stBoxRegion.uiWidth, pstToi[i].stBoxRegion.uiHeight, pstToi[i].u32TextColor);
    }

    return u32ToiCnt;
}


/**
 * @fn      xpack_osd_tip_2_toi
 * @brief   转换TIP标签信息为通用的OSD目标信息
 * 
 * @param   [OUT] pstToi OSD目标信息，包括目标区域和OSD文本Font
 * @param   [IN] pstLabelTip TIP标签信息
 * @param   [IN] u32BaseW TIP目标区域基于的图像宽
 * @param   [IN] u32BaseH TIP目标区域基于的图像高
 * 
 * @return  UINT32 OSD目标数量，0表示没有目标
 */
UINT32 xpack_osd_tip_2_toi(XPACK_OSD_TOI_LAYOUT *pstToi, XPACK_PACKAGE_LABEL *pstLabelTip, UINT32 u32BaseW, UINT32 u32BaseH)
{
    UINT32 u32LatSize = 0, u32FontSize = 0; // 点阵字体与矢量字体大小
    CAPB_OSD *pstCapbOsd = capb_get_osd();
    OSD_REGION_S *pstFont = NULL;
    XIMG_POINT stCentralPoint = {0}; // 矩形中心点

    if (NULL == pstToi || NULL == pstLabelTip)
    {
        XPACK_LOGE("pstToi(%p) OR pstLabelTip(%p) is NULL\n", pstToi, pstLabelTip);
        return 0;
    }
    if (u32BaseW * u32BaseH < XPACK_TOI_REGION_MIN)
    {
        XPACK_LOGE("XScanArea is too small, Width %u, Height %u, should be bigger than %d\n", 
                   u32BaseW, u32BaseH, XPACK_TOI_REGION_MIN);
        return 0;
    }

    ximg_rect_fix_bounds_u((XIMG_RECT *)&pstLabelTip->stRect, u32BaseW, u32BaseH);

    if (OSD_FONT_TRUETYPE == pstCapbOsd->enFontType) // 矢量字体
    {
        // TIP名称字体单元
        u32LatSize = pstCapbOsd->TrueTypeSize[1]; // 中号字体
        u32FontSize = u32LatSize;
        pstFont = osd_func_getFontRegion(OSD_BLOCK_IDX_STRING, ISM_TIP_TYPE, u32LatSize, u32FontSize, SAL_TRUE);
        if (NULL != pstFont)
        {
            pstToi->enDrawMode = SVA_OSD_LINE_POINT_TYPE; // TIP标签默认使用点线模式
            pstToi->u32TextFontCnt = 1;
            pstToi->pstTextFont[0] = pstFont;
            pstToi->u32TextColor = OSD_COLOR24_RED;
            pstToi->bTextOnTop = SAL_TRUE;
            pstToi->stTextRegion.uiWidth = pstFont->u32Width;
            pstToi->stTextRegion.uiHeight = pstFont->u32Height;

            // 先计算矩形中心点
            stCentralPoint.u32X = pstLabelTip->stRect.uiX + pstLabelTip->stRect.uiWidth / 2;
            stCentralPoint.u32Y = pstLabelTip->stRect.uiY ;
            //中心点不变，扩展到XPACK_OSD_TARGET_LABEL_RECT大的标识矩形
            pstToi->stBoxRegion.uiX = SAL_SUB_SAFE(stCentralPoint.u32X, (XPACK_OSD_TARGET_LABEL_RECT / 2));
            pstToi->stBoxRegion.uiY = SAL_SUB_SAFE(stCentralPoint.u32Y, (XPACK_OSD_TARGET_LABEL_RECT / 2));
            pstToi->stBoxRegion.uiX = SAL_alignDown(pstToi->stBoxRegion.uiX, 2); // 向下2对齐，即坐标向原点靠近
            pstToi->stBoxRegion.uiY = SAL_alignDown(pstToi->stBoxRegion.uiY, 2); // 向下2对齐，即坐标向原点靠近
            pstToi->stBoxRegion.uiWidth = pstToi->stBoxRegion.uiHeight = XPACK_OSD_TARGET_LABEL_RECT;

            return 1;
        }
        else
        {
            XPACK_LOGE("osd_func_getFontRegion failed, idx %d, code %u, LatSize %u, FontSize %u\n", 
                       OSD_BLOCK_IDX_STRING, ISM_TIP_TYPE, u32LatSize, u32FontSize);
            return 0;
        }
    }
    else // 点阵字体
    {
        XPACK_LOGE("unsupport lattice!!!!\n");
        return 0;
    }
}


/**
 * @fn      xpack_osd_layout_toi
 * @brief   布局（排布）OSD目标，标记OSD文本、目标框、连线的位置
 * 
 * @param   [IN] pstToi OSD目标信息，数组，数组个数为u32ToiCnt
 * @param   [IN/OUT] u32ToiCnt 输入：OSD目标数量，输出：实际可画的OSD目标数量
 * @param   [IN] pstImg 图像区域信息
 * @param   [IN] pstProfBorder 包裹外轮廓区域，基于pstImg
 * 
 * @return  INT32 <0：布局失败，其他：布局所有OSD目标需要的扩展宽度（图像太窄OSD文本太长导致无法画）
 */
INT32 xpack_osd_layout_toi(XPACK_OSD_TOI_LAYOUT *pstToi, UINT32 *u32ToiCnt, XIMAGE_DATA_ST *pstImg, XIMG_BORDER *pstProfBorder)
{
    UINT32 i = 0, j = 0, k = 0, u32ToiIdxMax = 0, u32ToiIdx = 0;
    UINT32 u32BlankTop = 0, u32BlankBottom = 0; // 上下可画OSD的空白区域
    UINT32 u32LineCntTop = 0, u32LineCntBottom = 0, u32LineTotal = 0;             /* 上下空白区域可画OSD文本的行数 */
    UINT32 u32LineDrawIdx = 0, u32LineCoverMin = 0, u32LineEndX = 0;              /* 标记画哪一行覆盖OSD文本区最少 */
    UINT32 u32FontUnivHeight = 0, u32FontOnTopHeight = 0, *pu32FontHeight = NULL; /* 一行OSD文本高度，需2对齐 */
    UINT32 u32TextOnTopCnt = 0, u32TextOnTopOffset = 0; // 置顶的文本数
    UINT32 u32TextUnTopCnt = 0; // 非置顶的文本数
    UINT32 u32PixelNeed = 0; /* 输入图像填充下OSD文本，横向还需要扩充多少个像素 */
    INT32 as32ToiIdx[SVA_XSI_MAX_ALARM_NUM] = {0}; /* 从左向右排序的目标索引值 */
    XPACK_OSD_TOI_LAYOUT astToiTmp[SVA_XSI_MAX_ALARM_NUM] = {0};
    XPACK_OSD_TOI_LAYOUT *pstToiTmp = NULL;

    if (NULL == pstToi || NULL == u32ToiCnt || NULL == pstImg || NULL == pstProfBorder)
    {
        XPACK_LOGE("pstToi(%p) OR u32ToiCnt(%p) OR pstImg(%p) OR pstProfBorder(%p) is NULL\n", 
                   pstToi, u32ToiCnt, pstImg, pstProfBorder);
        return -1;
    }
    if (0 == *u32ToiCnt)
    {
        XPACK_LOGE("the u32ToiCnt is ZERO\n");
        return -1;
    }

    u32ToiIdxMax = SAL_MIN(*u32ToiCnt, SVA_XSI_MAX_ALARM_NUM) - 1; // 注意是索引的最大值

    for (j = 0; j <= u32ToiIdxMax; j++)
    {
        // 对每个OSD文本的高度进行判断，所有置顶OSD的高度一致，非置顶OSD的高度也一致
        if (pstToi[j].u32TextFontCnt > 0)
        {
            if (pstToi[j].bTextOnTop)
            {
                pu32FontHeight = &u32FontOnTopHeight;
                if (SVA_OSD_NORMAL_TYPE != pstToi[j].enDrawMode)
                {
                    u32TextOnTopCnt++; // 跟随模式不支持置顶文本
                }
            }
            else
            {
                pu32FontHeight = &u32FontUnivHeight;
                if (SVA_OSD_NORMAL_TYPE != pstToi[j].enDrawMode)
                {
                    u32TextUnTopCnt++;
                }
            }
            if (*pu32FontHeight > 0)
            {
                if (*pu32FontHeight != pstToi[j].stTextRegion.uiHeight)
                {
                    XPACK_LOGE("the font height(%u) is different form %u\n", 
                               pstToi[j].stTextRegion.uiHeight, *pu32FontHeight);
                    return -1;
                }
            }
            else
            {
                *pu32FontHeight = pstToi[j].stTextRegion.uiHeight;
                if (0 == *pu32FontHeight || 0 != *pu32FontHeight % 2) // 字体高度目前要求2对齐，不然OSD文本会有部分内容重叠
                {
                    XPACK_LOGE("the FontHeight(%u) is invalid\n", *pu32FontHeight);
                    return -1;
                }
            }
        }
    }

    u32TextOnTopOffset = u32FontOnTopHeight * u32TextOnTopCnt;
    if (pstImg->u32Height < u32FontUnivHeight * 2 + u32TextOnTopOffset)
    {
        XPACK_LOGE("the image height(%u) is too small, should be bigger than '2 * FontUnivHeight(%u) + FontOnTopHeight(%u) * OnTopTextCnt(%u)'\n", 
                   pstImg->u32Height, u32FontUnivHeight, u32FontOnTopHeight, u32TextOnTopCnt);
        return -1; // 图像高度比2行文本和置顶文本总行还要小，无法画，直接返回
    }

    for (j = 0, k = 0, i = u32TextOnTopCnt; j <= u32ToiIdxMax; j++)
    {
        pstToiTmp = pstToi + j;
        if (SVA_OSD_NORMAL_TYPE == pstToiTmp->enDrawMode) // 跟随模式无需给OSD文本布局，直接画在目标矩形框上方
        {
            if (pstToiTmp->u32TextFontCnt > 0) // 有目标框与OSD文本
            {
                if (pstToiTmp->stTextRegion.uiWidth > pstImg->u32Width) // OSD文本超过了图像本身的长度，跳过不画
                {
                    XPACK_LOGW("%u/%u: the text region width(%u) is bigger than image(%u)\n", 
                               j, u32ToiIdxMax, pstToiTmp->stTextRegion.uiWidth, pstImg->u32Width);
                    if (pstToiTmp->stTextRegion.uiWidth - pstImg->u32Width > u32PixelNeed) // 计算能画OSD文本至少需要的图像宽度
                    {
                        u32PixelNeed = SAL_align(pstToiTmp->stTextRegion.uiWidth - pstImg->u32Width, 2);
                    }
                }
                else
                {
                    if (pstToiTmp->stBoxRegion.uiX + pstToiTmp->stTextRegion.uiWidth < pstImg->u32Width) /* 右边界小于图像的最右侧 */
                    {
                        pstToiTmp->stTextRegion.uiX = SAL_alignDown(pstToiTmp->stBoxRegion.uiX, 2);
                    }
                    else
                    {
                        pstToiTmp->stTextRegion.uiX = SAL_alignDown(pstImg->u32Width - pstToiTmp->stTextRegion.uiWidth, 2); /* 往左偏移，使其不越界 */
                    }
                    pstToiTmp->stTextRegion.uiY = SAL_SUB_SAFE(pstToiTmp->stBoxRegion.uiY, u32FontUnivHeight);

                    // 拷贝到临时数组astToiTmp中
                    memcpy(astToiTmp + u32ToiIdx++, pstToiTmp, sizeof(XPACK_OSD_TOI_LAYOUT));
                }
            }
            else // 只有目标框
            {
                memcpy(astToiTmp + u32ToiIdx++, pstToiTmp, sizeof(XPACK_OSD_TOI_LAYOUT));
            }
        }
        else
        {
            if (pstToiTmp->u32TextFontCnt > 0)
            {
                if (pstToiTmp->bTextOnTop) // 置顶文本，非跟随模式才支持
                {
                    as32ToiIdx[k++] = j;
                }
                else // 对非置顶的文本需要排序
                {
                    as32ToiIdx[i++] = j;
                }
            }
            else
            {
                XPACK_LOGW("%u/%u: the toi draw mode is %d, but osd text font is 0, drop it\n", j, u32ToiIdxMax, pstToiTmp->enDrawMode);
            }
        }
    }

    if (u32TextOnTopCnt + u32TextUnTopCnt > 0)
    {
        u32ToiIdxMax = u32TextOnTopCnt + u32TextUnTopCnt - 1;
        if (u32TextUnTopCnt > 1) // 从左向右对非置顶OSD对应的TOI排序
        {
            for (i = u32TextOnTopCnt; i < u32ToiIdxMax; i++)
            {
                for (j = u32TextOnTopCnt; j < u32ToiIdxMax - i; j++)
                {
                    if (pstToi[as32ToiIdx[j]].stBoxRegion.uiX > pstToi[as32ToiIdx[j+1]].stBoxRegion.uiX)
                    {
                        SAL_SWAP(as32ToiIdx[j], as32ToiIdx[j+1]);
                    }
                }
            }
        }
    }
    else
    {
        // 无OSD文本需要画，下面一路跳过，直接返回
    }

    if (u32FontUnivHeight > 0 && u32TextUnTopCnt > 0) // 计算有多少行空白可画非置顶OSD文本
    {
        // 转换为基于显示区域的Top和Bottom
        u32BlankTop = pstProfBorder->u32Top;
        u32BlankBottom = pstProfBorder->u32Bottom;
        if (u32BlankTop >= u32BlankBottom)
        {
            XPACK_LOGE("BlankTop %u, BlankBottom %u is INVALID, Img Height %u\n", u32BlankTop, u32BlankBottom, pstImg->u32Height);
            u32BlankTop = 0;
            u32BlankBottom = pstImg->u32Height - 1;
        }
        else
        {
            // 跳过置顶OSD文本区域
            u32BlankTop = SAL_SUB_SAFE(u32BlankTop, u32TextOnTopOffset);
            u32BlankBottom = SAL_MAX(u32BlankBottom, u32TextOnTopOffset);
        }

        // 计算上下空白区可画多少行非置顶文本
        u32LineCntTop = (u32BlankTop + u32FontUnivHeight / 2) / u32FontUnivHeight; // u32FontUnivHeight / 2: 四舍五入，有半行以上空白即算一行
        u32LineCntBottom = (pstImg->u32Height - u32BlankBottom + u32FontUnivHeight / 2) / u32FontUnivHeight;
        if (0 == u32LineCntTop + u32LineCntBottom) // 上下均无空白的情况
        {
            // 上下10%留做文本区
            u32LineCntBottom = u32LineCntTop = (pstImg->u32Height - u32TextOnTopOffset) / 10 / u32FontUnivHeight;
            if (0 == u32LineCntTop + u32LineCntBottom) // 上下10%还不够画一行文本，则强制只画一行
            {
                u32LineCntTop = 1;
            }
        }
    }

    u32LineTotal = u32TextOnTopCnt + u32LineCntTop + u32LineCntBottom;
    if (u32LineTotal > 0)
    {
        struct
        {
            UINT32 u32Free; // 每行可画OSD文本的空余长度
            UINT32 u32StartYPos; // 行的起始纵坐标
        } stTextLine[u32LineTotal];
        for (i = 0; i < u32TextOnTopCnt; i++) // 上置顶区，每个OSD文本独占一行
        {
            stTextLine[i].u32StartYPos = i * u32FontOnTopHeight;
        }
        for (i = 0, j = u32TextOnTopCnt; i < u32LineCntTop; i++, j++) // 上空白区，从左上往下顺序排列
        {
            stTextLine[j].u32StartYPos = u32TextOnTopOffset + i * u32FontUnivHeight;
            stTextLine[j].u32Free = pstImg->u32Width;
        }
        for (i = 0, j = u32TextOnTopCnt + u32LineCntTop; i < u32LineCntBottom; i++, j++)  // 下空白区，从左下往上顺序排列
        {
            stTextLine[j].u32StartYPos = SAL_SUB_SAFE(pstImg->u32Height, (i+1) * u32FontUnivHeight) >> 1 << 1; // 2对齐，并向原点靠近
            stTextLine[j].u32Free = pstImg->u32Width;
        }

        for (j = 0; j <= u32ToiIdxMax; j++)
        {
            pstToiTmp = pstToi + as32ToiIdx[j];
            if (pstToiTmp->stTextRegion.uiWidth > pstImg->u32Width) // OSD文本超过了图像本身的长度，跳过不画
            {
                XPACK_LOGW("%u/%u: the text region width(%u) is bigger than image(%u)\n", 
                           as32ToiIdx[j], u32ToiIdxMax, pstToiTmp->stTextRegion.uiWidth, pstImg->u32Width);
                as32ToiIdx[j] = -1; // 索引置为非法值
                if (pstToiTmp->stTextRegion.uiWidth - pstImg->u32Width > u32PixelNeed) // 计算能画OSD文本至少需要的图像宽度
                {
                    u32PixelNeed = SAL_align(pstToiTmp->stTextRegion.uiWidth - pstImg->u32Width, 2);
                }
                continue;
            }

            if (j < u32TextOnTopCnt) // 前u32TextOnTopCnt为上置顶，每个OSD文本独占一行
            {
                if (pstToiTmp->stBoxRegion.uiX + pstToiTmp->stTextRegion.uiWidth < pstImg->u32Width)
                {
                    pstToiTmp->stTextRegion.uiX = pstToiTmp->stBoxRegion.uiX;
                }
                else
                {
                    pstToiTmp->stTextRegion.uiX = SAL_alignDown(pstImg->u32Width - pstToiTmp->stTextRegion.uiWidth, 2);
                }
                pstToiTmp->stTextRegion.uiY = stTextLine[j].u32StartYPos;
            }
            else
            {
                u32LineCoverMin = pstImg->u32Width;
                u32LineDrawIdx = u32TextOnTopCnt;

                // 上空白区按从上往下，下空白区按从下往上，顺序遍历，判断是否可容纳OSD文本
                for (i = u32TextOnTopCnt; i < u32LineTotal; i++)
                {
                    u32LineEndX = pstImg->u32Width - stTextLine[i].u32Free;
                    if ((pstToiTmp->stBoxRegion.uiX >= u32LineEndX) &&                                      /* 文本标签起点不小于空余区域的 */
                        (pstToiTmp->stBoxRegion.uiX + pstToiTmp->stTextRegion.uiWidth <= pstImg->u32Width)) /* 文本标签结束小于一行的最右侧 */
                    {
                        u32LineDrawIdx = i;
                        break;
                    }
                    else
                    {
                        if (SAL_SUB_SAFE(u32LineEndX, pstToiTmp->stBoxRegion.uiX) < u32LineCoverMin) /* 记录覆盖最少的行，尽量少的覆盖左邻OSD文本 */
                        {
                            u32LineCoverMin = SAL_SUB_SAFE(u32LineEndX, pstToiTmp->stBoxRegion.uiX);
                            u32LineDrawIdx = i;
                        }
                    }
                }

                // 计算OSD文本区域的左上角坐标
                pstToiTmp->stTextRegion.uiX = pstToiTmp->stBoxRegion.uiX;
                pstToiTmp->stTextRegion.uiY = stTextLine[u32LineDrawIdx].u32StartYPos;
                if (pstToiTmp->stBoxRegion.uiX + pstToiTmp->stTextRegion.uiWidth > pstImg->u32Width) /* 直接画越界，需要调整回屏幕 */
                {
                    pstToiTmp->stTextRegion.uiX = pstImg->u32Width - pstToiTmp->stTextRegion.uiWidth;
                }

                // 重新计算当前行可画OSD文本的空余长度
                stTextLine[u32LineDrawIdx].u32Free = pstImg->u32Width - (pstToiTmp->stTextRegion.uiX + pstToiTmp->stTextRegion.uiWidth);
                stTextLine[u32LineDrawIdx].u32Free = SAL_SUB_SAFE(stTextLine[u32LineDrawIdx].u32Free, XPACK_OSD_TEXT_HOR_INTERVAL);
            }
        }

        // 标定线的位置，并将pstToi中内容按as32ToiIdx[]重新排序
        for (j = 0, i = 0; j <= u32ToiIdxMax; j++)
        {
            if (as32ToiIdx[j] >= 0)
            {
                pstToiTmp = pstToi + as32ToiIdx[j];
                // 标定线的位置（框线和点线模式才需要）
                if (SVA_OSD_LINE_RECT_TYPE == pstToiTmp->enDrawMode || SVA_OSD_LINE_POINT_TYPE == pstToiTmp->enDrawMode)
                {
                    if (pstToiTmp->stBoxRegion.uiY > pstToiTmp->stTextRegion.uiY &&
                        pstToiTmp->stBoxRegion.uiY + pstToiTmp->stBoxRegion.uiHeight > pstToiTmp->stTextRegion.uiY) // 目标框完全在OSD文本上方
                    {
                        pstToiTmp->stLine.u32PointEnd.u32X = pstToiTmp->stLine.u32PointStart.u32X = pstToiTmp->stBoxRegion.uiX; // 横坐标与目标框对齐
                        pstToiTmp->stLine.u32PointStart.u32Y = pstToiTmp->stTextRegion.uiY + pstToiTmp->stTextRegion.uiHeight;  // 起始纵坐标与OSD文本左下角对齐
                        pstToiTmp->stLine.u32PointEnd.u32Y = pstToiTmp->stBoxRegion.uiY;                                        // 结束纵坐标与目标框左上角对齐
                    }
                    else if (pstToiTmp->stTextRegion.uiY > pstToiTmp->stBoxRegion.uiY &&
                             pstToiTmp->stTextRegion.uiY + pstToiTmp->stTextRegion.uiHeight > pstToiTmp->stBoxRegion.uiY) // OSD文本完全在目标框上方
                    {
                        pstToiTmp->stLine.u32PointEnd.u32X = pstToiTmp->stLine.u32PointStart.u32X = pstToiTmp->stBoxRegion.uiX; // 横坐标与目标框对齐
                        pstToiTmp->stLine.u32PointStart.u32Y = pstToiTmp->stBoxRegion.uiY + pstToiTmp->stBoxRegion.uiHeight;    // 起始纵坐标与目标框左下角对齐
                        pstToiTmp->stLine.u32PointEnd.u32Y = pstToiTmp->stTextRegion.uiY;                                       // 结束纵坐标与OSD文本左上角对齐
                    }
                    else
                    {
                        // OSD文本与目标框有重叠，不画线
                    }
                }

                // 拷贝到临时数组astToiTmp中
                memcpy(astToiTmp + u32ToiIdx++, pstToiTmp, sizeof(XPACK_OSD_TOI_LAYOUT));
            }
        }
    }

    *u32ToiCnt = u32ToiIdx;
    memcpy(pstToi, astToiTmp, sizeof(XPACK_OSD_TOI_LAYOUT) * u32ToiIdx); // 拷贝回pstToi中输出

    return (INT32)u32PixelNeed;
}


/**
 * @fn      xpack_osd_draw_on_img
 * @brief   在图像pstImgOsd上画TOI的OSD
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstImgOsd 图像数据
 * @param   [IN] pstToiOsd TOI目标信息
 * @param   [IN] u32ToiCnt TOI目标数
 */
VOID xpack_osd_draw_on_img(UINT32 chan, XIMAGE_DATA_ST *pstImgOsd, XPACK_OSD_TOI_LAYOUT *pstToiOsd, UINT32 u32ToiCnt, UINT32 u32id)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0, j = 0;

    UINT32 u32OsdColor1555 = 0;
    UINT32 u32TextFontTotal = 0, u32TextFontOffset = 0;
    UINT32 u32LineIdx = 0;

    VGS_DRAW_LINE_PRM *pstLineArray = NULL;
    VGS_RECT_ARRAY_S *pstRectArray = NULL;
    VGS_DRAW_RECT_S *pstDrawRect = NULL;
    VGS_DRAW_PRM_S *pstDrawLine = NULL;
    VGS_OSD_PRM *pstDrawOsd = NULL;

    SYSTEM_FRAME_INFO stSysFrame = {0};
    SAL_VideoFrameBuf stVidFrame = {0};

    CAPB_LINE *pstCapbLine = capb_get_line();
    CAPB_OSD *pstCapbOsd = capb_get_osd();
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();

    MEDIA_BUF_ATTR stMbAttr = {0};
    struct _stDrawParam{
        VGS_DRAW_AI_PRM stLinePrm;
        VGS_ADD_OSD_PRM stTextPrm;
    } *pstDrawParam = NULL;

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return;
    }

    if (NULL == pstImgOsd || NULL == pstToiOsd)
    {
        XPACK_LOGE("xpack chan %u, pstImgOsd(%p) OR pstToiOsd(%p) is NULL\n", chan, pstImgOsd, pstToiOsd);
        return;
    }

    if (0 == u32ToiCnt)
    {
        XPACK_LOGE("xpack chan %u, the u32ToiCnt is 0\n", chan);
        return;
    }

    stMbAttr.bCached = SAL_TRUE;
    stMbAttr.memSize = sizeof(struct _stDrawParam);
    if (SAL_SOK == xpack_media_buf_alloc(&stMbAttr, "xpack_osd"))
    {
        pstDrawParam = stMbAttr.virAddr;
    }
    else
    {
        XPACK_LOGE("xpack chan %u, xpack_media_buf_alloc failed, buffer size: %u\n", chan, stMbAttr.memSize);
        return;
    }

    osd_func_readStart();

    /* 初始化OSD文本参数 */
    pstDrawParam->stTextPrm.uiOsdNum = 0;
    u32TextFontTotal = SAL_arraySize(pstDrawParam->stTextPrm.stVgsOsdPrm);

    /* 初始化框参数 */
    pstRectArray = &pstDrawParam->stLinePrm.VgsDrawRect;
    pstRectArray->u32RectNum = 0;
    pstRectArray->enExceedType = pstCapbLine->enRectType; // 开放矩形 OR 封闭矩形

    /* 初始化线参数 */
    pstLineArray = &pstDrawParam->stLinePrm.VgsDrawVLine;
    pstLineArray->uiLineNum = 0;

    for (i = 0; i < u32ToiCnt; i++)
    {
        /* OSD文本参数 */
        // 定义OSD文本的前景色与背景色
        SAL_RGB24ToARGB1555(pstToiOsd[i].u32TextColor, &u32OsdColor1555, SAL_TRUE); // Alpha为1表示不透明
        if (DRAW_MOD_VGS == pstCapbOsd->enDrawMod)
        {
            if (pstToiOsd[i].u32TextFontCnt <= u32TextFontTotal - pstDrawParam->stTextPrm.uiOsdNum)
            {
                u32TextFontOffset = pstToiOsd[i].stTextRegion.uiX;
                for (j = 0; j < pstToiOsd[i].u32TextFontCnt; j++)
                {
                    pstDrawOsd = pstDrawParam->stTextPrm.stVgsOsdPrm + pstDrawParam->stTextPrm.uiOsdNum++;
                    pstDrawOsd->u64PhyAddr = pstToiOsd[i].pstTextFont[j]->stAddr.u64PhyAddr;
                    pstDrawOsd->pVirAddr = pstToiOsd[i].pstTextFont[j]->stAddr.pu8VirAddr;
                    pstDrawOsd->enPixelFmt = SAL_VIDEO_DATFMT_ARGB16_1555;
                    pstDrawOsd->s32X = u32TextFontOffset;
                    pstDrawOsd->s32Y = pstToiOsd[i].stTextRegion.uiY;
                    pstDrawOsd->u32Width = pstToiOsd[i].pstTextFont[j]->u32Width;
                    pstDrawOsd->u32Height = pstToiOsd[i].pstTextFont[j]->u32Height;
                    pstDrawOsd->u32Stride = pstToiOsd[i].pstTextFont[j]->u32Stride;
                    pstDrawOsd->u32BgColor = (SVA_OSD_NORMAL_TYPE == pstToiOsd[i].enDrawMode) ? OSD_BACK_COLOR : u32OsdColor1555; // 除跟随模式外，背景色设置为字体颜色
                    pstDrawOsd->u32BgAlpha = 0;     // 背景透明
                    pstDrawOsd->u32FgAlpha = 255;   // 前景不透明
                    u32TextFontOffset += pstToiOsd[i].pstTextFont[j]->u32Width;
                }
            }
            else
            {
                continue; // 如果OSD文本无法画了，框和线也不画了
            }
        }

        /* 框与线参数 */
        if (DRAW_MOD_CPU == pstCapbLine->enDrawMod)
        {
            /* 框 */
            if (pstToiOsd[i].stBoxRegion.uiWidth > 0 && pstToiOsd[i].stBoxRegion.uiHeight > 0) // 需要画框的条件
            {
                pstDrawRect = pstRectArray->astRect + pstRectArray->u32RectNum++;
                pstDrawRect->s32X = pstToiOsd[i].stBoxRegion.uiX;
                pstDrawRect->s32Y = pstToiOsd[i].stBoxRegion.uiY;
                pstDrawRect->u32Width = pstToiOsd[i].stBoxRegion.uiWidth;
                pstDrawRect->u32Height = pstToiOsd[i].stBoxRegion.uiHeight;
                pstDrawRect->u32Thick = pstCapbLine->u32LineWidth;
                pstDrawRect->u32Color = pstToiOsd[i].u32TextColor; // 矩形框颜色与OSD文本颜色相同
                pstDrawRect->f32StartXFract = -1;
                pstDrawRect->f32EndXFract = -1;
                memcpy(&pstDrawRect->stLinePrm, &gstAiDgrLineParam[chan], sizeof(DISPLINE_PRM));
            }

            /* 线 */
            if (pstToiOsd[i].stLine.u32PointEnd.u32X > pstToiOsd[i].stLine.u32PointStart.u32X ||
                pstToiOsd[i].stLine.u32PointEnd.u32Y > pstToiOsd[i].stLine.u32PointStart.u32Y) // 需要画线的条件
            {
                u32LineIdx = pstLineArray->uiLineNum++;
                pstDrawLine = pstLineArray->stVgsLinePrm + u32LineIdx;
                pstDrawLine->stStartPoint.s32X = pstToiOsd[i].stLine.u32PointStart.u32X;
                pstDrawLine->stStartPoint.s32Y = pstToiOsd[i].stLine.u32PointStart.u32Y;
                pstDrawLine->stEndPoint.s32X = pstToiOsd[i].stLine.u32PointEnd.u32X;
                pstDrawLine->stEndPoint.s32Y = pstToiOsd[i].stLine.u32PointEnd.u32Y;
                pstDrawLine->u32Thick = pstCapbLine->u32LineWidth;
                pstDrawLine->u32Color = pstToiOsd[i].u32TextColor; // 线颜色与OSD文本颜色相同
                pstLineArray->af32XFract[u32LineIdx] = -1;
                memcpy(pstLineArray->linetype + u32LineIdx, &gstAiDgrLineParam[chan], sizeof(DISPLINE_PRM));
            }
        }
    }

    XPACK_LOGI("xpack chan %u, package ID %d, ToiCnt %u, OsdNum %u, RectNum %u, LineNum %u\n",
               chan, u32id, u32ToiCnt, pstDrawParam->stTextPrm.uiOsdNum, pstRectArray->u32RectNum, pstLineArray->uiLineNum);

    /* 执行画OSD、框、线操作 */
    if (SAL_SOK == Sal_halBuildSysFrame(&stSysFrame, &stVidFrame, pstImgOsd, NULL))
    {
        if (DRAW_MOD_CPU == pstCapbLine->enDrawMod) //先画框线，避免覆盖文本
        {
            s32Ret = vgs_func_drawRectArraySoft(&stVidFrame, pstRectArray, SAL_FALSE, SAL_FALSE);
            s32Ret |= vgs_func_drawLineSoft(&stVidFrame, pstLineArray, SAL_FALSE, SAL_FALSE);
            if (SAL_SOK != s32Ret)
            {
                XPACK_LOGE("xpack chan %u, vgs_func_drawLineSoft failed\n", chan);
            }
        }

        if (DRAW_MOD_VGS == pstCapbOsd->enDrawMod)
        {
            s32Ret = vgs_hal_drawLineOSDArray(&stSysFrame, &pstDrawParam->stTextPrm, NULL);
            if (SAL_SOK != s32Ret)
            {
                XPACK_LOGE("xpack chan %u, vgs_hal_drawLineOSDArray failed\n", chan);
            }
        }
    }
    else
    {
        XPACK_LOGE("xpack chan %u, Sal_halBuildSysFrame failed\n", chan);
    }

    osd_func_readEnd();
    xpack_media_buf_free(&stMbAttr);
    xpack_destroy_sys_frame(&stSysFrame);

    return;
}


/**
 * @fn      xpack_osd_select_new_onscn
 * @brief   根据显示区域转换目标框与OSD信息，仅输出在显示区域内的（注：该策略仅针对纵向，横向都输出）
 * 
 * @param   [IN] pstPkgAttr 包裹信息
 * @param   [IN] u64DispImgLocS 当前显示图像（不仅是屏幕区域）的起始Loc
 * @param   [IN] u64DispImgLocE 当前显示图像（不仅是屏幕区域）的结束Loc
 * @param   [IN] enDirMix 实时过包的方向或正向回拉的方向
 * @param   [OUT] pstPackOsd 在显示区域内的目标信息
 * 
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：失败
 */
static SAL_STATUS xpack_osd_select_new_onscn(XPACK_PACKAGE_ATTR *pstPkgAttr, UINT64 u64DispImgLocS, UINT64 u64DispImgLocE, XRAY_PROC_DIRECTION enDirMix, XPACK_DRAW_ONDISP *pstPackOsd)
{
    UINT32 i = 0, j = 0, k = 0;
    UINT32 u32YOffset = 0; /* 显示区域相对于X光扫描图上沿的偏移，=0往上偏或齐平，>0则往下偏 */
    UINT32 u32ToiBottom = 0; // 除空白外的有效区域上沿和下沿
    XIMG_RECT stToiArea = {0}; // 基于显示X光扫描图的目标区域
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    BOOL bToiInHor = SAL_FALSE, bToiInVer = SAL_FALSE;
    BOOL bShowAll = SAL_FALSE;
    XSP_DATA_SENT *pstXrIdtSrc[ISM_XDT_TYPE_NUM] = {0};
    XSP_DATA_SENT *pstXrIdtDst[ISM_XDT_TYPE_NUM] = {0};

    if (NULL == pstPkgAttr || NULL == pstPackOsd)
    {
        return SAL_FAIL;
    }
    if (pstPackOsd->stDispArea.uiY + pstPackOsd->stXScanArea.uiY < pstCapbDisp->disp_h_top_padding + pstCapbDisp->disp_h_top_blanking)
    {
        return SAL_FAIL;
    }
    else
    {
        u32YOffset = pstPackOsd->stDispArea.uiY + pstPackOsd->stXScanArea.uiY - pstCapbDisp->disp_h_top_padding - pstCapbDisp->disp_h_top_blanking;
    }

    bShowAll = (XPACK_OSD_REDRAW_NONE == pstPackOsd->enOsdRedrawMode) ? SAL_FALSE : SAL_TRUE;
    XPACK_LOGD("ShowAll %d, Disp Loc %llu ~ %llu, Package(%u) Loc %llu ~ %llu, AiDgr Num %u, WTotal-WRecv %u %u, IMG w %u, h %u; "
               "DispArea (%u, %u) - %u x %u; XScanArea (%u, %u) - %u x %u; ProfBorder %u - %u\n", 
               bShowAll, u64DispImgLocS, u64DispImgLocE, pstPkgAttr->u32Id, pstPkgAttr->u64DispLocStart, pstPkgAttr->u64DispLocEnd,
               pstPkgAttr->stAiDgrResult.target_num, pstPkgAttr->u32AiDgrWTotal, pstPkgAttr->u32AiDgrWRecv, 
               pstPackOsd->stImgOsd.u32Width, pstPackOsd->stImgOsd.u32Height, pstPackOsd->stDispArea.uiX, pstPackOsd->stDispArea.uiY, 
               pstPackOsd->stDispArea.uiWidth, pstPackOsd->stDispArea.uiHeight, pstPackOsd->stXScanArea.uiX, pstPackOsd->stXScanArea.uiY, 
               pstPackOsd->stXScanArea.uiWidth, pstPackOsd->stXScanArea.uiHeight, pstPackOsd->stProfileBorder.u32Top, pstPackOsd->stProfileBorder.u32Bottom);

    /**
     * ①包裹的水平和垂直方向只有部分在显示区域内 
     * ②水平方向图像宽大于已做危险品识别的包裹宽 
     * 以上这2种情况才需要删除不在显示区域内的目标或转换归一化坐标 
     */
    if (bShowAll || pstPkgAttr->u32AiDgrWOsd < pstPkgAttr->u32AiDgrWRecv)
    {
        if ((pstPackOsd->stXScanArea.uiHeight < pstCapbDisp->disp_h_middle_indeed) || // 垂直方向仅有部分包裹在显示区域上
            (pstPkgAttr->u64DispLocStart < u64DispImgLocS || pstPkgAttr->u64DispLocEnd > u64DispImgLocE) || // 水平方向仅有部分包裹在显示区域上
            (0 < pstPkgAttr->u32AiDgrWRecv && pstPkgAttr->u32AiDgrWRecv < pstPkgAttr->u32AiDgrWTotal)) // 仅部分包裹做完危险品识别，显示区域与危险品识别区域不一致，也需要转换
        {
            // 检测包裹中的目标区域是否在显示图像中，并转换归一化坐标（包裹的宽高发生了变化）
            for (i = 0, j = 0; i < pstPkgAttr->stAiDgrResult.target_num; i++)
            {
                // 转换归一化坐标到基于显示区域包裹的坐标
                ximg_rect_f2u(&stToiArea, &pstPkgAttr->stAiDgrResult.target[i].rect, 
                              pstPkgAttr->u32AiDgrWRecv * pstPkgAttr->u32RawHeight / pstPkgAttr->u32AiDgrWTotal, pstPkgAttr->u32RawWidth);
                bToiInVer = ((stToiArea.uiY > u32YOffset) && (stToiArea.uiY + stToiArea.uiHeight < u32YOffset + pstPackOsd->stXScanArea.uiHeight)) ? SAL_TRUE : SAL_FALSE;
                if (XRAY_DIRECTION_RIGHT == enDirMix)
                {
                    bToiInHor = (pstPkgAttr->u64DispLocEnd - stToiArea.uiX - stToiArea.uiWidth >= u64DispImgLocS) ? SAL_TRUE : SAL_FALSE;
                }
                else
                {
                    bToiInHor = (stToiArea.uiX + pstPkgAttr->u64DispLocStart >= u64DispImgLocS) ? SAL_TRUE : SAL_FALSE;
                }
                if (bToiInHor && bToiInVer) // 目标矩形框完全在显示图像内，重新计算起始点坐标
                {
                    memcpy(&pstPackOsd->stAiDgrResult.target[j], &pstPkgAttr->stAiDgrResult.target[i], sizeof(SVA_DSP_ALERT)-sizeof(SVA_RECT_F)); // 复制除坐标外的其他信息
                    stToiArea.uiY -= u32YOffset;
                    if (0 < pstPkgAttr->u32AiDgrWRecv && pstPkgAttr->u32AiDgrWRecv < pstPkgAttr->u32AiDgrWTotal) // 仅部分包裹做完危险品识别
                    {
                        if (XRAY_DIRECTION_RIGHT == enDirMix) // X坐标，L2R的有偏移，R2L是不变的
                        {
                            stToiArea.uiX += (pstPkgAttr->u32AiDgrWTotal - pstPkgAttr->u32AiDgrWRecv) * pstPkgAttr->u32RawHeight / pstPkgAttr->u32AiDgrWTotal;
                        }
                    }
                    ximg_rect_u2f(&pstPackOsd->stAiDgrResult.target[j++].rect, &stToiArea, pstPkgAttr->u32RawHeight, pstPackOsd->stXScanArea.uiHeight);

                    // 目标区域可能会在stProfBorder外，重新计算包裹外轮廓，使目标区域均在stProfBorder内
                    pstPackOsd->stProfileBorder.u32Top = SAL_MIN(stToiArea.uiY, pstPackOsd->stProfileBorder.u32Top);
                    u32ToiBottom = stToiArea.uiY + stToiArea.uiHeight - 1;
                    pstPackOsd->stProfileBorder.u32Bottom = SAL_MAX(u32ToiBottom, pstPackOsd->stProfileBorder.u32Bottom);
                }
            }
            pstPackOsd->stAiDgrResult.target_num = j;
        }
        else
        {
            memcpy(&pstPackOsd->stAiDgrResult, &pstPkgAttr->stAiDgrResult, 
                   sizeof(pstPkgAttr->stAiDgrResult.target_num) + sizeof(SVA_DSP_ALERT) * pstPkgAttr->stAiDgrResult.target_num);
        }
        pstPkgAttr->u32AiDgrWOsd = pstPkgAttr->u32AiDgrWRecv;
    }

    if ((pstPkgAttr->enXrIdtStage == XPACK_XAI_STAGE_OSD && bShowAll) || XPACK_XAI_STAGE_END == pstPkgAttr->enXrIdtStage)
    {
        pstXrIdtSrc[0] = &pstPkgAttr->stXspIdtResult.stZeffSent; // 可疑有机物
        pstXrIdtDst[0] = &pstPackOsd->stXspIdtResult.stZeffSent;
        pstXrIdtSrc[1] = &pstPkgAttr->stXspIdtResult.stUnpenSent; // 难穿透
        pstXrIdtDst[1] = &pstPackOsd->stXspIdtResult.stUnpenSent;

        for (k = 0; k < ISM_XDT_TYPE_NUM; k++)
        {
            if (NULL == pstXrIdtSrc[k])
            {
                break;
            }

            if ((pstPackOsd->stXScanArea.uiHeight < pstCapbDisp->disp_h_middle_indeed) || // 垂直方向仅有部分包裹在显示区域上
                (pstPkgAttr->u64DispLocStart < u64DispImgLocS || pstPkgAttr->u64DispLocEnd > u64DispImgLocE)) // 水平方向仅有部分包裹在显示区域上
            {
                for (i = 0, j = 0; i < pstXrIdtSrc[k]->uiNum; i++)
                {
                    // 转换归一化坐标到基于显示区域包裹的坐标
                    ximg_rect_f2u(&stToiArea, &pstXrIdtSrc[k]->stRect[i], pstPkgAttr->u32RawHeight, pstPkgAttr->u32RawWidth);
                    bToiInVer = ((stToiArea.uiY > u32YOffset) && (stToiArea.uiY + stToiArea.uiHeight < u32YOffset + pstPackOsd->stXScanArea.uiHeight)) ? SAL_TRUE : SAL_FALSE;
                    if (XRAY_DIRECTION_RIGHT == enDirMix)
                    {
                        bToiInHor = (pstPkgAttr->u64DispLocEnd - stToiArea.uiX - stToiArea.uiWidth >= u64DispImgLocS) ? SAL_TRUE : SAL_FALSE;
                    }
                    else
                    {
                        bToiInHor = (stToiArea.uiX + pstPkgAttr->u64DispLocStart >= u64DispImgLocS) ? SAL_TRUE : SAL_FALSE;
                    }
                    if (bToiInHor && bToiInVer) // 目标矩形框完全在显示图像内，重新计算起始点坐标
                    {
                        stToiArea.uiY -= u32YOffset;
                        ximg_rect_u2f(&pstXrIdtDst[k]->stRect[j++], &stToiArea, pstPkgAttr->u32RawHeight, pstPackOsd->stXScanArea.uiHeight);

                        // 目标区域可能会在stProfBorder外，重新计算包裹外轮廓，使目标区域均在stProfBorder内
                        pstPackOsd->stProfileBorder.u32Top = SAL_MIN(stToiArea.uiY, pstPackOsd->stProfileBorder.u32Top);
                        u32ToiBottom = stToiArea.uiY + stToiArea.uiHeight - 1;
                        pstPackOsd->stProfileBorder.u32Bottom = SAL_MAX(u32ToiBottom, pstPackOsd->stProfileBorder.u32Bottom);
                    }
                }
                pstXrIdtDst[k]->uiNum = j;
                pstXrIdtDst[k]->type = pstXrIdtSrc[k]->type;
                pstXrIdtDst[k]->uiResult = pstXrIdtSrc[k]->uiResult;
            }
            else
            {
                if (pstXrIdtSrc[k]->uiNum > 0)
                {
                    memcpy(pstXrIdtDst[k], pstXrIdtSrc[k], sizeof(XSP_DATA_SENT));
                }
                else
                {
                    pstXrIdtDst[k]->uiNum = 0;
                }
            }
        }
        pstPkgAttr->enXrIdtStage = XPACK_XAI_STAGE_OSD;
    }

    if ((pstPkgAttr->enTipStage == XPACK_XAI_STAGE_OSD && bShowAll) || XPACK_XAI_STAGE_END == pstPkgAttr->enTipStage)
    {
        if ((pstPackOsd->stXScanArea.uiHeight < pstCapbDisp->disp_h_middle_indeed) || // 垂直方向仅有部分包裹在显示区域上
            (pstPkgAttr->u64DispLocStart < u64DispImgLocS || pstPkgAttr->u64DispLocEnd > u64DispImgLocE)) // 水平方向仅有部分包裹在显示区域上
        {
            memcpy(&stToiArea, &pstPkgAttr->stLabelTip.stRect, sizeof(XIMG_RECT));
            bToiInVer = ((stToiArea.uiY > u32YOffset) && (stToiArea.uiY + stToiArea.uiHeight < u32YOffset + pstPackOsd->stXScanArea.uiHeight)) ? SAL_TRUE : SAL_FALSE;
            if (XRAY_DIRECTION_RIGHT == enDirMix)
            {
                stToiArea.uiX = pstPkgAttr->u32RawHeight - stToiArea.uiX - stToiArea.uiWidth; // L2R矩形起点在右上，转换为左上
                bToiInHor = (pstPkgAttr->u64DispLocEnd - stToiArea.uiX - stToiArea.uiWidth >= u64DispImgLocS) ? SAL_TRUE : SAL_FALSE;
            }
            else
            {
                bToiInHor = (stToiArea.uiX + pstPkgAttr->u64DispLocStart >= u64DispImgLocS) ? SAL_TRUE : SAL_FALSE;
            }
            if (bToiInHor && bToiInVer) // 目标矩形框完全在显示图像内，重新计算起始点坐标
            {
                stToiArea.uiY -= u32YOffset;
                memcpy(&pstPackOsd->stLabelTip.stRect, &stToiArea, sizeof(XIMG_RECT));
                strcpy(pstPackOsd->stLabelTip.szName, pstPkgAttr->stLabelTip.szName);
            }
        }
        else
        {
            memcpy(&pstPackOsd->stLabelTip, &pstPkgAttr->stLabelTip, sizeof(XPACK_PACKAGE_LABEL));
            if (XRAY_DIRECTION_RIGHT == enDirMix) // L2R矩形起点在右上，转换为左上
            {
                pstPackOsd->stLabelTip.stRect.uiX = SAL_SUB_SAFE(pstPkgAttr->u32RawHeight, pstPkgAttr->stLabelTip.stRect.uiX + pstPkgAttr->stLabelTip.stRect.uiWidth);
            }
        }
        pstPkgAttr->enTipStage = XPACK_XAI_STAGE_OSD;
    }

    return SAL_SOK;
}


/**
 * @fn      xpack_osd_draw_on_screen
 * @brief   在当前（实时预览、条带回拉、整包回拉）显示图像上画OSD
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 */
static VOID xpack_osd_draw_on_screen(UINT32 chan, RECT *pstGlobalZoomArea, BOOL bSyncUpdate, XPACK_OSD_REDRAW_MODE enOsdRedraw4Disp)
{
    UINT32 i = 0, u32PackCnt = 0, u32PackH = 0, u32MaxTmp = 0, u32RetLeft = 0;
    UINT32 u32ProfTop = 0, u32ProfBottom = 0; // 除空白外的有效区域上沿和下沿
    UINT32 u32DispTop = 0, u32DispBottom = 0; // 显示区域的上下边界
    UINT64 u64DispImgLocS = 0, u64DispImgLocE = 0; // 当前显示图像的读写索引
    UINT64 u64DispRLoc = 0, u64DispWLoc = 0;
    XRAY_PROC_STATUS_E enProcStat = XRAY_PS_NONE;
    XIMAGE_DATA_ST stImgPure = {0}; // 纯显示区域图像数据，无OSD，与stImgOsd区域范围相同
    XIMAGE_DATA_ST stImgScreen = {0}, *pstImgScreen = &stImgScreen;
    RECT stGlobalZoomArea = {0}; // 全局放大区域
    CHAR szDbgInfo1[64] = {0}, szDbgInfo2[64] = {0};
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL;
    XPACK_DRAW_ONDISP *pstDrawOnDisp = NULL;
    XPACK_PACKAGE_OSD stPackOsd = {0};
    SAL_VideoCrop stDispCrop = {0};
    DSA_NODE *pNode = NULL;
    XRAY_PROC_DIRECTION enDirMix = XRAY_DIRECTION_NONE;

    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;
    XPACK_DUMP_CFG *pstDumpCfg = NULL;
    UINT32 u32DispHTopBlank = pstCapbDisp->disp_h_top_padding + pstCapbDisp->disp_h_top_blanking;

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return;
    }
    if (NULL == gXpackRingBufHandle[chan][XPACK_DISPDT_OSD])
    {
        return;
    }

    pstXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];
    enProcStat = pstXpackChnPrm->enProcStat;
    pstPkgCtrl = (XRAY_PS_PLAYBACK_MASK & enProcStat) ? (gstPbPackageCtrl + chan) : (gstRtPackageCtrl + chan);

    if (NULL == pstGlobalZoomArea)
    {
        if (SAL_SOK == disp_tskGetDptzoffset(chan, NULL, &stGlobalZoomArea))
        {
            if (pstCapbDisp->disp_h_top_blanking + pstCapbDisp->disp_h_bottom_blanking > 0) // 支持上下拖动
            {
                stGlobalZoomArea.uiY += pstXpackChnPrm->u32DispVertOffset; // 叠加上下拖动区域，转换为相对于CAPB_DISP->disp_yuv_h_max的Y值
            }
            pstGlobalZoomArea = &stGlobalZoomArea;
        }
        else
        {
            XPACK_LOGE("xpack chan %u, disp_tskGetDptzoffset failed\n", chan);
            return;
        }
    }

    // 获取当前屏幕的显示区域，从屏幕上最旧的开始，到当前写索引
    if (ring_buf_size(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], &u64DispWLoc, &u64DispRLoc, &u64DispImgLocS, &u64DispImgLocE) < 0)
    {
        XPACK_LOGE("xpack chan %u, ring_buf_size failed\n", chan);
        return;
    }

    // 先判断显示纵向区域是否包含X光扫描图
    u32PackH = SAL_SUB_SAFE(pstGlobalZoomArea->uiY, u32DispHTopBlank) +
        SAL_SUB_SAFE(u32DispHTopBlank + pstCapbDisp->disp_h_middle_indeed, pstGlobalZoomArea->uiY + pstGlobalZoomArea->uiHeight);
    u32PackH = SAL_SUB_SAFE(pstCapbDisp->disp_h_middle_indeed, u32PackH); /* 计算包裹区域在CAPB_DISP->disp_h_middle_indeed中的高度 */

    if (u32PackH > 0)
    {
        SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        pNode = DSA_LstGetTail(pstPkgCtrl->lstPackage);
        while (NULL != pNode) // 从最新的包裹往前查找，直到其横向区域不在显示图像中
        {
            pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
            if (pstPkgAttr->u64DispLocEnd > u64DispImgLocS && pstPkgAttr->u64DispLocStart < u64DispImgLocE) // 当前包裹在显示图像中
            {
                if (XPACK_OSD_REDRAW_NONE != enOsdRedraw4Disp || /* 重新画 */
                    pstPkgAttr->u32AiDgrWOsd < pstPkgAttr->u32AiDgrWRecv || /* 危险品识别结果有更新，重画该包裹的所有OSD */
                    pstPkgAttr->enXrIdtStage == XPACK_XAI_STAGE_END || /* 已获得XSP难穿透&可疑有机物识别结果，增加该OSD */
                    pstPkgAttr->enTipStage == XPACK_XAI_STAGE_END) /* 已到TIP考核需显示标签的时间，增加该OSD */
                {
                    pstDrawOnDisp = &pstPkgCtrl->stDrawOnDisp[u32PackCnt];
                    memset(pstDrawOnDisp, 0, sizeof(XPACK_DRAW_ONDISP));
                    pstDrawOnDisp->u32Id = pstPkgAttr->u32Id;
                    XPACK_LOGD("xpack chan %u, RedrawMode %d, package ID %u, AiDgr WRecv-WOsd %u %u, XrIdt Stage %d, bSyncUpdate %d, enSyncRedrawMode %d, Package Loc %llu ~ %llu\n",
                               chan, enOsdRedraw4Disp, pstPkgAttr->u32Id, pstPkgAttr->u32AiDgrWRecv, pstPkgAttr->u32AiDgrWOsd,
                               pstPkgAttr->enXrIdtStage, bSyncUpdate, pstPkgAttr->enSyncRedrawMode, pstPkgAttr->u64DispLocStart, pstPkgAttr->u64DispLocEnd);

                    /* 同步模式使用包裹独立的画OSD标记 */
                    if (bSyncUpdate)
                    {
                        pstDrawOnDisp->enOsdRedrawMode = pstPkgAttr->enSyncRedrawMode;
                        pstPkgAttr->enSyncRedrawMode = XPACK_OSD_REDRAW_NONE;
                        if (pstDrawOnDisp->enOsdRedrawMode == XPACK_OSD_REDRAW_NONE)
                        {
                            pNode = pNode->prev;
                            continue;
                        }
                    }
                    else
                    {
                        /**
                         * 异步画OSD模式下，当前已有危险品识别OSD且有识别目标更新，
                         * 需清除原有OSD然后重新画，则从Enc数据拷贝一份纯图像数据来叠加OSD
                         */
                        if (pstPkgAttr->u32AiDgrWOsd > 0 && pstPkgAttr->u32AiDgrWOsd < pstPkgAttr->u32AiDgrWRecv)
                        {
                            pstDrawOnDisp->enOsdRedrawMode = XPACK_OSD_REDRAW_CLEAROLD;
                        }
                        else
                        {
                            pstDrawOnDisp->enOsdRedrawMode = enOsdRedraw4Disp;
                        }
                    }

                    // 计算X光扫描图占显示区域stImg图像中的区域，除去padding、blanking和包裹右侧的空白
                    pstDrawOnDisp->stXScanArea.uiX = 0;
                    pstDrawOnDisp->stXScanArea.uiY = SAL_SUB_SAFE(u32DispHTopBlank, pstGlobalZoomArea->uiY);
                    pstDrawOnDisp->stXScanArea.uiHeight = u32PackH;
                    pstDrawOnDisp->stXScanArea.uiWidth = (UINT32)(pstPkgAttr->u64DispLocEnd - pstPkgAttr->u64DispLocStart);

                    /**
                     * 全都转化为基于显示图像的 
                     * 计算包裹外轮廓边界，基于stXScanArea，去除X光扫描图中的上下空白，当前仅Top和Bottom有效 
                     */
                    u32ProfTop = u32DispHTopBlank + pstPkgAttr->stPackDivInfo.uiPackTop;
                    u32ProfBottom = u32DispHTopBlank + pstPkgAttr->stPackDivInfo.uiPackBottom;
                    u32DispTop = pstGlobalZoomArea->uiY;
                    u32DispBottom = pstGlobalZoomArea->uiY + pstGlobalZoomArea->uiHeight;
                    if (u32DispTop >= u32ProfBottom || u32DispBottom <= u32ProfTop) // 显示区域与外轮廓无交集
                    {
                        pstDrawOnDisp->stProfileBorder.u32Top = pstDrawOnDisp->stXScanArea.uiHeight - 1;
                        pstDrawOnDisp->stProfileBorder.u32Bottom = 0;
                    }
                    else if (u32DispTop >= u32ProfTop && u32DispBottom <= u32ProfBottom) // 显示区域完全在外轮廓内
                    {
                        pstDrawOnDisp->stProfileBorder.u32Top = 0;
                        pstDrawOnDisp->stProfileBorder.u32Bottom = pstDrawOnDisp->stXScanArea.uiHeight - 1;
                    }
                    else // 显示区域与外轮廓有部分重叠
                    {
                        u32MaxTmp = SAL_MAX(u32DispHTopBlank, u32DispTop);
                        pstDrawOnDisp->stProfileBorder.u32Top = SAL_SUB_SAFE(u32ProfTop, u32MaxTmp);
                        pstDrawOnDisp->stProfileBorder.u32Bottom = (u32ProfBottom > u32DispBottom) ? (pstDrawOnDisp->stXScanArea.uiHeight - 1) : (u32ProfBottom - u32MaxTmp);
                    }

                    /**
                     * 从显示图像抠取包裹的显示区域，抠取的范围横向在全局放大（如果有）区域上有扩展： 
                     * 横向，起点：当前包裹的左侧图像位置 
                     *      终点：右邻包裹的左侧图像位置，包含当前包裹右侧空白（该空白用来延伸OSD）
                     * 纵向，与全局放大（如果有）的显示区域相同，基于CAPB_DISP->disp_yuv_h_max 
                     */
                    pstDrawOnDisp->stDispArea.uiY = pstGlobalZoomArea->uiY;
                    pstDrawOnDisp->stDispArea.uiHeight = pstGlobalZoomArea->uiHeight;
                    /* 统一取右侧包裹间的空白 */
                    if ((XRAY_PS_RTPREVIEW == enProcStat && pstXpackChnPrm->enRtDirection == XRAY_DIRECTION_RIGHT) ||
                        ((enProcStat & XRAY_PS_PLAYBACK_MASK) && pstXpackChnPrm->enRtDirection == XRAY_DIRECTION_LEFT)) /* 取上一个，较当前更旧 */
                    {
                        enDirMix = XRAY_DIRECTION_RIGHT;
                        pstDrawOnDisp->u64CropLoc = pstPkgAttr->u64DispLocEnd; // L2R，以包裹的结束为图像数据起点
                        if (NULL != pNode->prev)
                        {
                            pstDrawOnDisp->stDispArea.uiWidth = pstPkgAttr->u64DispLocEnd - ((XPACK_PACKAGE_ATTR *)pNode->prev->pAdData)->u64DispLocEnd;
                        }
                        else // 右侧无包裹，至少抠取当前包裹的宽度，若当前包裹在显示屏幕内，直接抠取到将要出屏幕的位置
                        {
                            pstDrawOnDisp->stDispArea.uiWidth = SAL_SUB_SAFE(pstPkgAttr->u64DispLocEnd, SAL_MIN(u64DispImgLocS, pstPkgAttr->u64DispLocStart));
                        }
                    }
                    else // R2L，取与下一个（右侧，较当前更新）包裹间的空白
                    {
                        enDirMix = XRAY_DIRECTION_LEFT;
                        pstDrawOnDisp->u64CropLoc = pstPkgAttr->u64DispLocStart; // R2L，以包裹的开始为图像数据起点
                        if (NULL != pNode->next)
                        {
                            pstDrawOnDisp->stDispArea.uiWidth = ((XPACK_PACKAGE_ATTR *)pNode->next->pAdData)->u64DispLocStart - pstPkgAttr->u64DispLocStart;
                        }
                        else
                        {
                            pstDrawOnDisp->stDispArea.uiWidth = u64DispImgLocE - pstPkgAttr->u64DispLocStart; // 右侧无包裹，直接抠取到当前写入的位置
                        }
                    }
                    if (XRAY_PS_PLAYBACK_FRAME == enProcStat) /* 整包回拉将抠图区域设置在屏幕中间 */
                    {
                        enDirMix = XRAY_DIRECTION_LEFT;
                        pstDrawOnDisp->u64CropLoc = pstPkgAttr->u64DispLocStart;
                        pstDrawOnDisp->stDispArea.uiWidth = SAL_SUB_SAFE(pstPkgAttr->u64DispLocEnd, pstPkgAttr->u64DispLocStart);
                    }
                    pstDrawOnDisp->stDispArea.uiWidth = SAL_MIN(pstDrawOnDisp->stXScanArea.uiWidth+XPACK_OSD_DISP_EXPAND_MAX, 
                                                                pstDrawOnDisp->stDispArea.uiWidth); // 最多延伸扩展XPACK_OSD_ONSCN_EXPAND_MAXPixel的空白图像

                    stDispCrop.left = 0;
                    stDispCrop.top = pstGlobalZoomArea->uiY;
                    stDispCrop.height = pstGlobalZoomArea->uiHeight;
                    stDispCrop.width = pstDrawOnDisp->stDispArea.uiWidth;
                    // ring_buf_crop()接口从小索引往大索引方向抠图，L2R时，反向计算其起始索引
                    if (XRAY_DIRECTION_RIGHT == enDirMix)
                    {
                        pstDrawOnDisp->u64CropLoc -= pstDrawOnDisp->stDispArea.uiWidth;
                    }
                    pstDrawOnDisp->stImgOsd.u32Width = stDispCrop.width;
                    pstDrawOnDisp->stImgOsd.u32Height = stDispCrop.height;
                    if (SAL_SOK == ring_buf_crop(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], pstDrawOnDisp->u64CropLoc, 0, &stDispCrop, &pstDrawOnDisp->stImgOsd))
                    {
                        if (SAL_SOK == xpack_osd_select_new_onscn(pstPkgAttr, u64DispImgLocS, u64DispImgLocE, enDirMix, pstDrawOnDisp))
                        {
                            // 无论后续叠加是否成功，均在xpack_osd_select_new_onscn()更新叠加OSD的标记，提高互斥锁的效率
                            u32PackCnt++;
                        }
                        else
                        {
                            XPACK_LOGE("xpack chan %u, xpack_osd_select_new_onscn failed, Disp Loc %llu ~ %llu, PackIdx %u\n", 
                                    chan, u64DispImgLocS, u64DispImgLocE, u32PackCnt);
                        }
                    }
                    else
                    {
                        XPACK_LOGE("chan %u, ring_buf_crop in 'XPACK_DISPDT_OSD' failed, ID %u, CropLoc %llu, top %u, width %u, height %u\n", 
                                chan, pstPkgAttr->u32Id, pstDrawOnDisp->u64CropLoc, stDispCrop.top, stDispCrop.width, stDispCrop.height);
                        if (pstPkgAttr->u32AiDgrWOsd < pstPkgAttr->u32AiDgrWRecv)
                        {
                            pstPkgAttr->u32AiDgrWOsd = pstPkgAttr->u32AiDgrWRecv;
                        }
                        if (XPACK_XAI_STAGE_END == pstPkgAttr->enXrIdtStage)
                        {
                            pstPkgAttr->enXrIdtStage = XPACK_XAI_STAGE_OSD;
                        }
                        if (XPACK_XAI_STAGE_END == pstPkgAttr->enTipStage)
                        {
                            pstPkgAttr->enTipStage = XPACK_XAI_STAGE_OSD;
                        }
                    }
                }
            }
            else
            {
                if (XRAY_PS_RTPREVIEW == enProcStat) // 实时过包不在显示图像中则跳出，回拉需遍历完整个队列
                {
                    break;
                }
            }
            pNode = pNode->prev;
        }
        SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);
    }
    else
    {
        return; // 全局放大的区域内不包含任何X光扫描图，不可能有包裹，直接返回
    }

    pstDumpCfg = &gGlobalPrm.astXpackChnPrm[chan].stDumpCfg;
    if (pstDumpCfg->u32DumpCnt > 0 && pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_PACKOSD && u32PackCnt > 0)
    {
        pstDumpCfg->u32DumpCnt--;
    }
    for (i = 0; i < u32PackCnt; i++)
    {
        pstDrawOnDisp = pstPkgCtrl->stDrawOnDisp + i;
        if (XPACK_OSD_REDRAW_CLEAROLD == pstDrawOnDisp->enOsdRedrawMode) // 需要清除旧的OSD，然后重新画
        {
            // 从XPACK_DISPDT_PURE拷贝一份纯图像数据叠加OSD
            stDispCrop.left = 0;
            stDispCrop.top = pstGlobalZoomArea->uiY;
            stDispCrop.height = pstGlobalZoomArea->uiHeight;
            stDispCrop.width = pstDrawOnDisp->stDispArea.uiWidth;
            memset(&stImgPure, 0, sizeof(XIMAGE_DATA_ST));
            stImgPure.u32Width = stDispCrop.width;
            stImgPure.u32Height = stDispCrop.height;
            if (SAL_SOK == ring_buf_crop(gXpackRingBufHandle[chan][XPACK_DISPDT_PURE], pstDrawOnDisp->u64CropLoc, 0, &stDispCrop, &stImgPure))
            {
                if (SAL_SOK != ximg_copy_nb(&stImgPure, &pstDrawOnDisp->stImgOsd, NULL, NULL, XIMG_FLIP_NONE))
                {
                    XPACK_LOGE("xpack chan %u, ximg_copy_nb failed\n", chan);
                    continue;
                }
            }
            else
            {
                XPACK_LOGE("xpack chan %u, ring_buf_crop in 'XPACK_DISPDT_PURE' failed, CropLoc %llu, top %u, width %u, height %u\n", 
                        chan, pstDrawOnDisp->u64CropLoc, stDispCrop.top, stDispCrop.width, stDispCrop.height);
                continue;
            }
        }

        // 校验预览、回拉的处理状态是否发生变化，发生变化则退出
        if (((XRAY_PS_PLAYBACK_MASK & enProcStat) && XRAY_PS_RTPREVIEW == pstXpackChnPrm->enProcStat) ||
            (XRAY_PS_RTPREVIEW == enProcStat && (XRAY_PS_PLAYBACK_MASK & pstXpackChnPrm->enProcStat)))
        {
            XPACK_LOGE("xpack chan %u, ProcStat has changed from %d to %d\n", chan, enProcStat, pstXpackChnPrm->enProcStat);
            break;
        }

        if (pstDumpCfg->u32DumpCnt > 0 && pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_PACKOSD)
        {
            snprintf(szDbgInfo1, sizeof(szDbgInfo1), "%sosd_loc%llu", (XRAY_PS_RTPREVIEW==enProcStat) ? "rt": "pb", pstDrawOnDisp->u64CropLoc);
            snprintf(szDbgInfo2, sizeof(szDbgInfo2), "cropy%u_redraw%d_1", pstGlobalZoomArea->uiY, pstDrawOnDisp->enOsdRedrawMode);
            ximg_dump(&pstDrawOnDisp->stImgOsd, chan, pstDumpCfg->chDumpDir, szDbgInfo1, szDbgInfo2, pstDrawOnDisp->u32Id);
        }
        stPackOsd.pstImgOsd = &pstDrawOnDisp->stImgOsd;
        stPackOsd.u32Id = pstDrawOnDisp->u32Id;
        memcpy(&stPackOsd.stToiLimitedArea, &pstDrawOnDisp->stXScanArea, sizeof(XIMG_RECT));
        stPackOsd.stProfileBorder.u32Left = pstDrawOnDisp->stXScanArea.uiX;
        stPackOsd.stProfileBorder.u32Top = pstDrawOnDisp->stXScanArea.uiY + pstDrawOnDisp->stProfileBorder.u32Top;
        stPackOsd.stProfileBorder.u32Right = pstDrawOnDisp->stXScanArea.uiX + pstDrawOnDisp->stXScanArea.uiWidth;
        stPackOsd.stProfileBorder.u32Bottom = pstDrawOnDisp->stXScanArea.uiY + pstDrawOnDisp->stProfileBorder.u32Bottom;
        stPackOsd.pstAiDgrResult = (pstXpackChnPrm->enOsdShowMode >= XPACK_SHOW_OSD_ALL) ? &pstDrawOnDisp->stAiDgrResult : NULL;
        stPackOsd.pstXrIdtResult = (pstXpackChnPrm->enOsdShowMode >= XPACK_SHOW_OSD_ALL) ? &pstDrawOnDisp->stXspIdtResult : NULL;
        stPackOsd.pstLabelTip = (pstXpackChnPrm->enOsdShowMode >= XPACK_SHOW_OSD_TIP) ? &pstDrawOnDisp->stLabelTip : NULL;
        Xpack_DrvDrawOsdOnPackage(chan, &stPackOsd, SAL_FALSE, 0);

        if (pstDumpCfg->u32DumpCnt > 0 && pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_PACKOSD)
        {
            snprintf(szDbgInfo2, sizeof(szDbgInfo2), "cropy%u_redraw%d_2", pstGlobalZoomArea->uiY, pstDrawOnDisp->enOsdRedrawMode);
            ximg_dump(&pstDrawOnDisp->stImgOsd, chan, pstDumpCfg->chDumpDir, szDbgInfo1, szDbgInfo2, pstDrawOnDisp->u32Id);
            memset(pstImgScreen, 0, sizeof(XIMAGE_DATA_ST));
            u32RetLeft = ring_buf_get(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], 0, 0, &pstImgScreen, 1);
            if (-1 != u32RetLeft)
            {
                ximg_dump(pstImgScreen, chan, pstDumpCfg->chDumpDir, szDbgInfo1, "up1", pstDrawOnDisp->u32Id);
            }
        }
        if (SAL_SOK != ring_buf_update(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], &pstDrawOnDisp->stImgOsd, 
                                       pstDrawOnDisp->u64CropLoc, RING_REFRESH_NONE, pstGlobalZoomArea->uiY))
        {
            XPACK_LOGE("xpack chan %u, ring_buf_update failed, CropLoc %llu, top %u, image w %u h %u\n", 
                    chan, pstDrawOnDisp->u64CropLoc, pstGlobalZoomArea->uiY, pstDrawOnDisp->stImgOsd.u32Width, pstDrawOnDisp->stImgOsd.u32Height);
        }
        if (pstDumpCfg->u32DumpCnt > 0 && pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_PACKOSD)
        {
            if (-1 != u32RetLeft)
            {
                ximg_dump(pstImgScreen, chan, pstDumpCfg->chDumpDir, szDbgInfo1, "up2", pstDrawOnDisp->u32Id);
            }
        }
    }

    return;
}


/**
 * @fn      xpack_osd_tip_start_drawing
 * @brief   发起画TIP的流程，一个包裹最多仅支持一个TIP
 * 
 * @param   [IN] chan_packid 高4位表示通道号，低28位表示包裹ID
 */
static VOID xpack_osd_tip_start_drawing(INT32 chan_packid)
{
    UINT32 chan = (UINT32)chan_packid >> 28;
    UINT32 u32Id = (UINT32)chan_packid & 0xFFFFFFF;
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL;
    DSA_NODE *pNode = NULL;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return;
    }

    pstPkgCtrl = gstRtPackageCtrl + chan;
    SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    pNode = DSA_LstGetTail(pstPkgCtrl->lstPackage);
    while (NULL != pNode)
    {
        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
        if (pstPkgAttr->u32Id == u32Id) // Bingo the package
        {
            if (XPACK_XAI_STAGE_START == pstPkgAttr->enTipStage)
            {
                pstPkgAttr->enTipStage = XPACK_XAI_STAGE_END; // 等待TIP考核时间结束，开始画TIP的OSD
            }
            SAL_TimerDestroy(pstPkgAttr->pTimerHandle);
            pstPkgAttr->pTimerHandle = NULL;
            break;
        }
        pNode = pNode->prev;
    }
    SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

    return;
}


/**
 * @fn      xpack_osd_add_label_tip
 * @brief   注册TIP的OSD字符，默认为红色白底、点线模式
 */
static VOID xpack_osd_add_label_tip(void)
{
    INT32 s32Ret = SAL_SOK;
    OSD_SET_ARRAY_S stOsdTip = {0};
    OSD_SET_PRM_S stOsdPrm = {0};

    #if 0
    memset(pDrawAiCtl->orgPrm, 0, sizeof(pDrawAiCtl->orgPrm));
    // TIP
    idx = ISM_TIP_TYPE - ISM_XDT_TYPE_BASE;
    pDrawAiCtl->orgPrm[idx].u32ScaleLevel = 2;
    pDrawAiCtl->orgPrm[idx].u32BackColor = 0x7C00;
    pDrawAiCtl->orgPrm[idx].enDrawType = SVA_OSD_LINE_POINT_TYPE;
    pDrawAiCtl->orgPrm[idx].u32BgAlpha = 255;
    pDrawAiCtl->orgPrm[idx].u32FgAlpha = 255;
    strcpy(pDrawAiCtl->orgPrm[idx].sName, sTipName);
    #endif

    stOsdTip.u32StringNum = 1;
    stOsdTip.pstOsdPrm = &stOsdPrm;

    stOsdTip.pstOsdPrm->u32Idx = ISM_TIP_TYPE;
    stOsdTip.pstOsdPrm->szString = "TIP";
    stOsdTip.pstOsdPrm->u32Color = OSD_COLOR24_RED;
    stOsdTip.pstOsdPrm->u32BgColor = OSD_COLOR24_WHITE;
    stOsdTip.pstOsdPrm->enEncFormat = ENC_FMT_GB2312;

    osd_func_writeStart();
    s32Ret = osd_func_blockSet(OSD_BLOCK_IDX_STRING, &stOsdTip);
    osd_func_writeEnd();
    if (SAL_SOK != s32Ret)
    {
        XPACK_LOGE("osd_func_blockSet for 'TIP' failed\n");
    }

    return;
}


/**
 * @fn      xpack_update_screen_osd_thread
 * @brief   在当前（实时预览、条带回拉、整包回拉）显示图像上画OSD
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 */
static VOID *xpack_update_screen_osd_thread(VOID *prm)
{
    UINT32 chan = 0;
    XPACK_OSD_CONFIG_AIDGR stOsdConfig = {0};
    RECT stGlobalZoomArea = {0}; // 全局放大区域
    XPACK_OSD_REDRAW_MODE enOsdRedraw4Disp = XPACK_OSD_REDRAW_NONE; /* 给显示图像重新画OSD的方式 */

    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;
    struct
    {
        UINT32 u32DispY;
        UINT32 u32DispHeight;
    } stDispAreaLast = {0};
    XPACK_OSD_CONFIG_AIDGR stAiDgrOsdLast = {0};

    chan = (UINT32)(PhysAddr)prm;
    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return NULL;
    }

    pstXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];

    while (1)
    {
        SAL_msleep(100);
        if (NULL == gXpackRingBufHandle[chan][XPACK_DISPDT_OSD])
        {
            continue;
        }

        enOsdRedraw4Disp = XPACK_OSD_REDRAW_NONE; /* 给显示图像重新画OSD的方式 */

        if (SAL_SOK == disp_tskGetDptzoffset(chan, NULL, &stGlobalZoomArea))
        {
            if (pstCapbDisp->disp_h_top_blanking + pstCapbDisp->disp_h_bottom_blanking > 0) // 支持上下拖动
            {
                stGlobalZoomArea.uiY += pstXpackChnPrm->u32DispVertOffset; // 叠加上下拖动区域，转换为相对于CAPB_DISP->disp_yuv_h_max的Y值
            }
        }
        else
        {
            XPACK_LOGE("xpack chan %u, disp_tskGetDptzoffset failed\n", chan);
            continue;
        }

        /* 检测纵向显示区域是否发生变化，是则重新画OSD */
        if ((stDispAreaLast.u32DispY != stGlobalZoomArea.uiY || stDispAreaLast.u32DispHeight != stGlobalZoomArea.uiHeight))
        {
            enOsdRedraw4Disp = XPACK_OSD_REDRAW_CLEAROLD;
            stDispAreaLast.u32DispY = stGlobalZoomArea.uiY;
            stDispAreaLast.u32DispHeight = stGlobalZoomArea.uiHeight;
        }

        /* 检查危险品识别OSD配置是否发生变化，是则重新画OSD */
        if (SAL_SOK != xpack_osd_get_aidgr_config(chan, &stOsdConfig))
        {
            XPACK_LOGE("xpack chan %u, xpack_osd_get_aidgr_config failed\n", chan);
            continue;
        }
        if (0 != memcmp(&stAiDgrOsdLast, &stOsdConfig, sizeof(XPACK_OSD_CONFIG_AIDGR)))
        {
            enOsdRedraw4Disp = XPACK_OSD_REDRAW_CLEAROLD;
            memcpy(&stAiDgrOsdLast, &stOsdConfig, sizeof(XPACK_OSD_CONFIG_AIDGR));
        }

        if (XPACK_OSD_REDRAW_NONE != pstXpackChnPrm->enOsdRedraw4Disp)
        {
            enOsdRedraw4Disp = SAL_MAX(pstXpackChnPrm->enOsdRedraw4Disp, enOsdRedraw4Disp); // 选择优先级较高的画OSD方式
            pstXpackChnPrm->enOsdRedraw4Disp = XPACK_OSD_REDRAW_NONE;
        }

        if (SAL_SOK == SAL_mutexTmLock(&pstXpackChnPrm->mutexOsdDraw, 1000, __FUNCTION__, __LINE__))
        {
            xpack_osd_draw_on_screen(chan, &stGlobalZoomArea, SAL_FALSE, enOsdRedraw4Disp);
            SAL_mutexTmUnlock(&pstXpackChnPrm->mutexOsdDraw, __FUNCTION__, __LINE__);
        }
    }

    return NULL;
}


static SAL_STATUS xpack_osd_module_int(void)
{
    UINT32 i = 0;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    SAL_ThrHndl stDrawOsdThrHandl = {0};

    xpack_osd_add_label_tip();

    for (i = 0; i < pstCapbXray->xray_in_chan_cnt; i++)
    {
        SAL_thrCreate(&stDrawOsdThrHandl, xpack_update_screen_osd_thread, SAL_THR_PRI_DEFAULT, 0, (VOID *)((PhysAddr)i));
    }

    return SAL_SOK;
}


/**
 * 画文本OSD的三要素：
 * 1. 包裹在图像中的区域，去除padding与blanking；
 * 2. 除空白外的有效区域，在上述①区域内；
 * 3. 上述2区域内的目标位置，相对于①区域的归一化坐标，该位置可能会在②区域外；
 * 注：图像宽指定的区域内中有且仅有一个包裹（半个包裹也算一个），否则OSD会画到其他包裹上 
 */
/**
 * @fn      Xpack_DrvDrawOsdOnPackage
 * @brief   基于包裹画目标的OSD
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN/OUT] pstPackOsd 输入：包裹的图像及其目标信息 
 *                              输出：当bAutoExpandToShowAll为TRUE时，自动扩展图像区域，pstPackOsd->stImgOsd中的width会变大
 * @param   [IN] bAutoExpandToShowAll 是否自动扩展图像以显示所有的目标，OSD文本超过图像宽度时才会起作用 
 *                                    而允许扩展必须满足pstPackOsd->stImgOsd的stride大于width 
 * @param   [IN] u32DispBgColor 自动扩展图像时，图像扩展区域被重置的背景色
 * 
 * @return  INT32 <0：Draw失败，其他：布局所有OSD目标需要的扩展宽度（图像太窄OSD文本太长导致无法画）
 */
INT32 Xpack_DrvDrawOsdOnPackage(UINT32 chan, XPACK_PACKAGE_OSD *pstPackOsd, BOOL bAutoExpandToShowAll, UINT32 u32DispBgColor)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT32 u32ToiCnt = 0, u32AiDgrToiCnt = 0, u32XrIdtToiCnt = 0, u32TipToiCnt = 0;
    UINT32 u32ToiBaseW = 0, u32ToiBaseH = 0, u32ToiBottom = 0;
    XPACK_OSD_CONFIG_AIDGR stOsdConfig = {0};
    XPACK_OSD_TOI_LAYOUT astToiOsd[SVA_XSI_MAX_ALARM_NUM] = {0};
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return -1;
    }

    if (NULL == pstPackOsd)
    {
        XPACK_LOGE("xpack chan %u, pstPackOsd is NULL\n", chan);
        return -1;
    }
    XPACK_CHECK_PTR_NULL(pstPackOsd->pstImgOsd, -1);

    u32ToiBaseW = pstPackOsd->stToiLimitedArea.uiWidth;
    u32ToiBaseH = pstPackOsd->stToiLimitedArea.uiHeight;

TRY_AGAIN:
    /* 将危险品识别结果转换为通用的OSD目标信息 */
    if (NULL != pstPackOsd->pstAiDgrResult && pstPackOsd->pstAiDgrResult->target_num > 0)
    {
        // 获取当前危险品标识的绘制参数
        if (SAL_SOK != xpack_osd_get_aidgr_config(chan, &stOsdConfig))
        {
            XPACK_LOGE("xpack chan %u, xpack_osd_get_aidgr_config failed\n", chan);
            return -1;
        }

        u32AiDgrToiCnt = xpack_osd_aidgr_2_toi(astToiOsd, &stOsdConfig, pstPackOsd->pstAiDgrResult, u32ToiBaseW, u32ToiBaseH);
        u32ToiCnt = u32AiDgrToiCnt;
        if (u32AiDgrToiCnt == 0)
        {
            XPACK_LOGW("xpack chan %u, xpack_osd_aidgr_2_toi failed\n", chan);
        }
    }

    /* 将XSP难穿透&可疑有机物识别结果转换为通用的OSD目标信息 */
    if (NULL != pstPackOsd->pstXrIdtResult && 
        pstPackOsd->pstXrIdtResult->stZeffSent.uiNum + pstPackOsd->pstXrIdtResult->stUnpenSent.uiNum > 0)
    {
        u32XrIdtToiCnt = xpack_osd_xridt_2_toi(astToiOsd+u32ToiCnt, pstPackOsd->pstXrIdtResult, u32ToiBaseW, u32ToiBaseH);
        u32ToiCnt += u32XrIdtToiCnt;
        if (u32XrIdtToiCnt == 0)
        {
            XPACK_LOGW("xpack chan %u, xpack_osd_xridt_2_toi failed\n", chan);
        }
    }

    /* 将TIP标签转换为通用的OSD目标信息 */
    if (NULL != pstPackOsd->pstLabelTip && strlen(pstPackOsd->pstLabelTip->szName) > 0)
    {
        u32TipToiCnt = xpack_osd_tip_2_toi(astToiOsd+u32ToiCnt, pstPackOsd->pstLabelTip, u32ToiBaseW, u32ToiBaseH);
        u32ToiCnt += u32TipToiCnt;
        if (u32TipToiCnt == 0)
        {
            XPACK_LOGW("xpack chan %u, xpack_osd_tip_2_toi failed\n", chan);
        }
    }

    if (u32ToiCnt == 0)
    {
        XPACK_LOGD("xpack chan %u, u32ToiCnt is 0, don't need to draw on package\n", chan);
        return 0;
    }

    /* 转换目标框基于显示区域图像 */
    for (i = 0; i < u32ToiCnt; i++)
    {
        // 标定框的位置，转换为基于显示图像区域的坐标
        astToiOsd[i].stBoxRegion.uiX += pstPackOsd->stToiLimitedArea.uiX;
        astToiOsd[i].stBoxRegion.uiY += pstPackOsd->stToiLimitedArea.uiY;

        // 标定框向下2对齐，保证YUV格式显示正常
        astToiOsd[i].stBoxRegion.uiX = SAL_alignDown(astToiOsd[i].stBoxRegion.uiX, 2);
        astToiOsd[i].stBoxRegion.uiY = SAL_alignDown(astToiOsd[i].stBoxRegion.uiY, 2);
        astToiOsd[i].stBoxRegion.uiWidth = SAL_alignDown(astToiOsd[i].stBoxRegion.uiWidth, 2);
        astToiOsd[i].stBoxRegion.uiHeight = SAL_alignDown(astToiOsd[i].stBoxRegion.uiHeight, 2);

        // 扩展目标外轮廓，外轮廓需包含所有目标区域
        u32ToiBottom = astToiOsd[i].stBoxRegion.uiY + astToiOsd[i].stBoxRegion.uiHeight - 1;
        pstPackOsd->stProfileBorder.u32Top = SAL_MIN(astToiOsd[i].stBoxRegion.uiY, pstPackOsd->stProfileBorder.u32Top);
        pstPackOsd->stProfileBorder.u32Bottom = SAL_MAX(u32ToiBottom, pstPackOsd->stProfileBorder.u32Bottom);
    }

    /* 布局目标OSD文本与线的位置 */
    s32Ret = xpack_osd_layout_toi(astToiOsd, &u32ToiCnt, pstPackOsd->pstImgOsd, &pstPackOsd->stProfileBorder);
    if (s32Ret < 0)
    {
        XPACK_LOGE("xpack chan %u, xpack_osd_layout_toi failed, ToiCnt %u\n", chan, u32ToiCnt);
    }
    else
    {
        if (s32Ret > 0 && bAutoExpandToShowAll)
        {
            XPACK_LOGI("xpack chan %u, expand %u pixel to show all TOIs\n", chan, s32Ret);
            if (s32Ret > SAL_SUB_SAFE(pstPackOsd->pstImgOsd->u32Stride[0], pstPackOsd->pstImgOsd->u32Width))
            {
                XPACK_LOGE("xpack chan %u, expansion area(%u) is too small, at least %u\n", 
                           chan, SAL_SUB_SAFE(pstPackOsd->pstImgOsd->u32Stride[0], pstPackOsd->pstImgOsd->u32Width), s32Ret);
            }
            else
            {
                // 扩展图像的宽度，扩展区域填充指定背景色，然后重新尝试layout
                if (SAL_SOK == ximg_set_dimension(pstPackOsd->pstImgOsd, pstPackOsd->pstImgOsd->u32Width + s32Ret, pstPackOsd->pstImgOsd->u32Stride[0], SAL_FALSE, u32DispBgColor))
                {
                    bAutoExpandToShowAll = SAL_FALSE; // 关闭自动扩展功能，防止二次扩展
                    goto TRY_AGAIN;
                }
                else
                {
                    XPACK_LOGE("xpack chan %u, ximg_set_dimension failed, start %u, fill %u, bg 0x%x\n", 
                               chan, pstPackOsd->pstImgOsd->u32Width, s32Ret, u32DispBgColor);
                }
            }
        }

        if (u32ToiCnt > 0)
        {
            XPACK_LOGT("xpack chan %u,ID:%d IMG[%u x %u]; ToiLimitedArea (%u, %u)[%u x %u]; ProfileBorder T:%u B:%u; aidgr toi(%u) layout info: \n",
                       chan, pstPackOsd->u32Id, pstPackOsd->pstImgOsd->u32Width, pstPackOsd->pstImgOsd->u32Height, pstPackOsd->stToiLimitedArea.uiX,
                       pstPackOsd->stToiLimitedArea.uiY, pstPackOsd->stToiLimitedArea.uiWidth, pstPackOsd->stToiLimitedArea.uiHeight,
                       pstPackOsd->stProfileBorder.u32Top, pstPackOsd->stProfileBorder.u32Bottom, u32ToiCnt);
            for (i = 0; i < u32ToiCnt; i++)
            {
                XPACK_LOGT("%u/%u: Box (%u, %u)[%u x %u]; Text (%u, %u)[%u x %u], FontCnt %u, Color 0x%x; DrawMode %d, Line (%u, %u) - (%u, %u)\n", i, u32ToiCnt, 
                           astToiOsd[i].stBoxRegion.uiX, astToiOsd[i].stBoxRegion.uiY, astToiOsd[i].stBoxRegion.uiWidth, astToiOsd[i].stBoxRegion.uiHeight, 
                           astToiOsd[i].stTextRegion.uiX, astToiOsd[i].stTextRegion.uiY, astToiOsd[i].stTextRegion.uiWidth, astToiOsd[i].stTextRegion.uiHeight,
                           astToiOsd[i].u32TextFontCnt, astToiOsd[i].u32TextColor, astToiOsd[i].enDrawMode, astToiOsd[i].stLine.u32PointStart.u32X, 
                           astToiOsd[i].stLine.u32PointStart.u32Y, astToiOsd[i].stLine.u32PointEnd.u32X, astToiOsd[i].stLine.u32PointEnd.u32Y);
            }

            xpack_osd_draw_on_img(chan, pstPackOsd->pstImgOsd, astToiOsd, u32ToiCnt, pstPackOsd->u32Id);
        }
        else
        {
            XPACK_LOGW("xpack chan %u, after xpack_osd_layout_toi(), ToiCnt being 0\n", chan);
        }
    }

    return s32Ret;
}


/**
 * @fn      Xpack_GetSvaResultForPos
 * @brief   根据预览/回拉包裹队列获取当前屏幕智能信息
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [OUT] pstSvaOut 输出：当前屏幕显示区域违禁品POS信息 
 */
SAL_STATUS Xpack_GetSvaResultForPos(UINT32 chan, SVA_PROCESS_OUT *pstSvaOut)
{
    INT32 s32Ret = 0;
    UINT32 i = 0, j = 0;
    UINT32 target_num = 0;
    UINT64 u64DispReadLoc = 0;
    DSA_NODE *pNode = NULL;
    XIMG_RECT stRectU = {0};
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL;
    XPACK_SVA_RESULT_OUT stSvaOut = {0};
    SVA_PROCESS_IN stSvaCfg = {0};
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();

    if (XRAY_PS_RTPREVIEW == gGlobalPrm.astXpackChnPrm[chan].enProcStat)
    {
        pstPkgCtrl = gstRtPackageCtrl + chan; /* 回拉暂不支持 */
    }
    else
    {
        return SAL_SOK;
    }

    s32Ret = Sva_DrvGetCfgParam(0, &stSvaCfg);
    if (s32Ret != SAL_SOK)
    {
        XPACK_LOGE("xpack chan %u, Sva_DrvGetCfgParam failed\n", chan);
        return SAL_FAIL;
    }

    pNode = DSA_LstGetTail(pstPkgCtrl->lstPackage);
    s32Ret = ring_buf_size(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD], NULL, &u64DispReadLoc, NULL, NULL);
    if (s32Ret == SAL_FAIL)
    {
        XPACK_LOGE("xpack chan %u, ring_buf_size failed\n", chan);
        return SAL_FAIL;
    }

    pstSvaOut->target_num = 0;
    while (pNode != NULL)
    {
        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
        SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        if (!xpack_rt_package_is_on_screen(chan, pstPkgAttr)) /* 判断包裹是否在屏幕内，超出则剩下的都超出，跳过 */
        {
            SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);
            break;
        }
        pNode = pNode->prev;
        memcpy(&stSvaOut, &pstPkgAttr->stAiDgrResult, sizeof(XPACK_SVA_RESULT_OUT));
        SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

        for (i = 0, j = 0; i < stSvaOut.target_num; i++)
        {
            if (target_num + i >= SVA_XSI_MAX_ALARM_NUM) /* 记录的违禁品数达到数组上限，剩余的丢弃 */
            {
                break;
            }

            pstSvaOut->target[target_num + j].ID = stSvaOut.target[i].ID;
            pstSvaOut->target[target_num + j].type = stSvaOut.target[i].type;
            pstSvaOut->target[target_num + j].color = stSvaCfg.alert[i].sva_color; /* 包裹携带智能颜色无效，引擎颜色准确 */
            pstSvaOut->target[target_num + j].alarm_flg = stSvaOut.target[i].alarm_flg;
            pstSvaOut->target[target_num + j].confidence = stSvaOut.target[i].confidence;
            pstSvaOut->target[target_num + j].visual_confidence = stSvaOut.target[i].visual_confidence;
            pstSvaOut->target[target_num + j].cnt = 1; /* 没有数据来源暂时赋值1 */

            memcpy(pstSvaOut->target[target_num + j].sva_name, stSvaCfg.alert[pstSvaOut->target[target_num + j].type].sva_name, SVA_ALERT_NAME_LEN);

            /* 转换违禁品位置，从基于识别的包裹到基于显示码流显示区域 */
            ximg_rect_f2u(&stRectU, &stSvaOut.target[i].rect, (UINT32)(pstCapbXsp->resize_height_factor * pstPkgAttr->u32AiDgrWRecv), pstPkgAttr->u32RawWidth);
            if (gGlobalPrm.astXpackChnPrm->enRtDirection == XRAY_DIRECTION_LEFT)
            {
                stRectU.uiX += pstPkgAttr->u64DispLocStart + pstCapbDisp->disp_yuv_w_max - u64DispReadLoc;
            }
            else
            {
                stRectU.uiX += u64DispReadLoc - pstPkgAttr->u64DispLocStart - pstPkgAttr->u32RawHeight;
            }
            stRectU.uiY += pstCapbDisp->disp_h_top_padding;

            if ((signed int)stRectU.uiX < 0 ||                               /* 该违禁品左侧超出屏幕，整个不画 */
                stRectU.uiX + stRectU.uiWidth > pstCapbDisp->disp_yuv_w_max) /* 该违禁品右侧超出屏幕，整个不画 */
            {
                continue;
            }
            ximg_rect_u2f(&pstSvaOut->target[target_num + j].rect, &stRectU, pstCapbDisp->disp_yuv_w_max, pstCapbDisp->disp_yuv_h_max);
            j++;
        }
        target_num += j; /* 累计各个包裹危险品总数 */
        pstSvaOut->target_num = target_num;
    }

    return SAL_SOK;
}


/**
 * @fn      Xpack_DrvClearOsdOnScreen
 * @brief   清空屏幕上所有包裹的OSD，包括实时过包与回拉
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 */
VOID Xpack_DrvClearOsdOnScreen(UINT32 chan)
{
    UINT32 i = 0;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL;
    DSA_NODE *pNode = NULL;
    XIMAGE_DATA_ST *pstImageData = NULL;
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;
    XPACK_PACKAGE_CTRL *apstPkgCtrl[] = {gstRtPackageCtrl + chan, gstPbPackageCtrl + chan};

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return;
    }

    for (i = 0; i < SAL_arraySize(apstPkgCtrl); i++)
    {
        pstPkgCtrl = apstPkgCtrl[i];
        SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        pNode = DSA_LstGetHead(pstPkgCtrl->lstPackage);
        while (NULL != pNode)
        {
            pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;

            // 清除危险品识别结果
            pstPkgAttr->u32AiDgrWTotal = 0;
            pstPkgAttr->u32AiDgrWSend = 0;
            pstPkgAttr->u32AiDgrWRecv = 0;
            pstPkgAttr->stAiDgrResult.target_num = 0;
            if (pstPkgAttr->s32AiDgrBufIdx >= 0) // 释放内存
            {
                pstImageData = pstPkgCtrl->stAiDgrBuf + pstPkgAttr->s32AiDgrBufIdx;
                pstImageData->u32Width = 0; // 重置数据宽度，表示无数据
                pstImageData->u32Height = 0; // 重置数据高度，表示无数据
            }

            // 清除XSP识别信息
            pstPkgAttr->enXrIdtStage = XPACK_XAI_STAGE_NONE;
            pstPkgAttr->stXspIdtResult.stUnpenSent.uiNum = 0;
            pstPkgAttr->stXspIdtResult.stZeffSent.uiNum = 0;
            if (pstPkgAttr->s32XrIdtBufIdx >= 0)
            {
                pstImageData = pstPkgCtrl->stXrIdtBuf + pstPkgAttr->s32XrIdtBufIdx;
                pstImageData->u32Width = 0; // 重置数据宽度，表示无数据
                pstImageData->u32Height = 0; // 重置数据高度，表示无数据
            }

            // 清除TIP信息
            if (NULL != pstPkgAttr->pTimerHandle)
            {
                SAL_TimerDestroy(pstPkgAttr->pTimerHandle);
                pstPkgAttr->pTimerHandle = NULL;
            }
            pstPkgAttr->enTipStage = XPACK_XAI_STAGE_NONE;
            memset(&pstPkgAttr->stLabelTip, 0, sizeof(XPACK_PACKAGE_LABEL));

            pNode = pNode->next;
        }
        SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);
    }

    return;
}


/**
 * @fn      xpack_disp_data_send_thread
 * @brief   发送显示数据的线程
 * 
 * @param   [IN] pPrm 结构体XPACK_SENDDUP_PRM
 */
static void *xpack_disp_data_send_thread(void *pPrm)
{
    UINT32 chan = 0, u32RetLeft = 0, u32CacheDstCol = 0;
    INT32 s32DispBufLeftSize = 0;
    INT32 s32DTimeItemIdx = 0;
    XPACK_DISP_DATA_TYPE enSendDataType = XPACK_DISPDT_PURE;
    XPACK_SENDDUP_PRM *pstSendDupPrm = (XPACK_SENDDUP_PRM *)pPrm;
    DupHandle pDupHandle = 0;
    ring_buf_handle pDispBufHandle = NULL;

    XIMAGE_DATA_ST stDispImg = {0};
    XIMAGE_DATA_ST *pstDispImg = NULL;

    PARAM_INFO_S stParamInfo = {0};
    SAL_SYSFRAME_PRIVATE stPriv = {0};
    SYSTEM_FRAME_INFO *pstGetPutFrame = NULL;
    SAL_VideoFrameBuf stVideoFrame = {0};
    
    debug_time pSendVpssDTime = NULL;
    XPACK_GLOBAL_PRM *pGlobalPrm = &gGlobalPrm;
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;
    XPACK_DUMP_CFG *pstDumpCfg = NULL;
    DSPINITPARA *pstDspInitPrm = SystemPrm_getDspInitPara();
    CAPB_PLATFORM *pstPlatform = capb_get_platform();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    CAPB_XPACK *pstCapbXpack = capb_get_xpack();

    XPACK_CHECK_PTR_NULL(pPrm, NULL);
    chan = pstSendDupPrm->chan;
    XPACK_CHECK_CHAN(chan, NULL);

    /* CPU绑核处理*/
    if (pstCapbXpack->uiCoreDispVpss[chan] > 0)
    {
        SAL_SetThreadCoreBind(pstCapbXpack->uiCoreDispVpss[chan]);
    }
    /* 设置线程名称*/
    SAL_SET_THR_NAME();

    /* 获取帧信息结构体，单视角双显区分*/
    pstGetPutFrame = &pstSendDupPrm->stGetPutFrame0;
    if (pstGetPutFrame->uiAppData == 0x00)
    {
        XPACK_LOGE("uiAppData null error !\n");
        return NULL;
    }

    enSendDataType = pstSendDupPrm->enSendDataType;
    pDupHandle = pstSendDupPrm->pDupHandle;
    pDispBufHandle = gXpackRingBufHandle[chan][enSendDataType];
    pSendVpssDTime = pstSendDupPrm->pSendDupDTime;
    
    pstXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];
    pstDumpCfg = &pstXpackChnPrm->stDumpCfg;
    pstDispImg = &stDispImg;

    while (1)
    {
        /* 循环缓存有待显示数据时，耗时调试模块记录条目增1 */
        s32DispBufLeftSize = ring_buf_size(pDispBufHandle, NULL, NULL, NULL, NULL);
        if (0 < s32DispBufLeftSize)
        {
            s32DTimeItemIdx = dtime_add_item(pSendVpssDTime, pstSendDupPrm->framePts, 0, 0);
        }
        else
        {
            dtime_clear_item(pSendVpssDTime, s32DTimeItemIdx);
        }
        dtime_add_time_point(pSendVpssDTime, s32DTimeItemIdx, SAL_TRUE, "cacheS", -1, SAL_TRUE);

        /* 过包模式启用显示缓存 */
        if (XPACK_DISPDT_PURE != enSendDataType)
        {
            u32CacheDstCol = (XRAY_PS_RTPREVIEW == pstXpackChnPrm->enProcStat) ? (8 * pGlobalPrm->u32RefreshCol) : (2 * pGlobalPrm->u32RefreshCol);
            xpack_disp_data_cache(chan, enSendDataType, u32CacheDstCol);
        }
        dtime_add_time_point(pSendVpssDTime, s32DTimeItemIdx, SAL_TRUE, "cacheE", -1, SAL_TRUE);

        /* 从循环显示缓冲获取一帧图像 */
        u32RetLeft = ring_buf_get(pDispBufHandle, pGlobalPrm->u32RefreshCol, pstXpackChnPrm->u32DispVertOffset, &pstDispImg, 1);
        if (-1 == u32RetLeft)
        {
            XPACK_LOGW("chan %d get ring buf data failed, emDataType:%d, refresh col:%u, vert offset:%u\n", chan, enSendDataType, pGlobalPrm->u32RefreshCol, pstXpackChnPrm->u32DispVertOffset);
            continue;
        }
        if (XPACK_DISPDT_OSD == enSendDataType && pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_SCNSHOT)
        {
            ximg_dump(pstDispImg, chan, pstDumpCfg->chDumpDir, "xpack-send-vpss", NULL, pstSendDupPrm->framePts);
            pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_SCNSHOT = SAL_FALSE;     // 截屏时输入一次命令截屏一次
        }
        dtime_add_time_point(pSendVpssDTime, s32DTimeItemIdx, SAL_TRUE, "gotData", -1, SAL_TRUE);

        /* 获取系统时间值 */
        if (SAL_SOK != sys_hal_GetSysCurPts(&(pstSendDupPrm->framePts)))
        {
            pstSendDupPrm->framePts += 15;
        }

        /* 构建系统帧 */
        stPriv.u64Pts = pstSendDupPrm->framePts;
        Sal_halBuildSysFrame(pstGetPutFrame, &stVideoFrame, pstDispImg, &stPriv);

        /* 将数据送给vpss */
        if (pstPlatform->bFrameCrop)
        {
            memset(&stParamInfo, 0, sizeof(PARAM_INFO_S));
            stParamInfo.enType = GRP_CROP_CFG;
            stParamInfo.stCrop.u32CropEnable = 1;
            stParamInfo.stCrop.enCropType = CROP_ORIGN;
            if (pstDspInitPrm->dspCapbPar.ism_disp_mode == ISM_DISP_MODE_DOUBLE_UPDOWN && ISM_5030D_INPUT_DUP_CHG)
            {
                stParamInfo.stCrop.u32H = pstDspInitPrm->dspCapbPar.xray_width_max;
                stParamInfo.stCrop.u32W = pstDspInitPrm->dspCapbPar.xray_height_max;
                stParamInfo.stCrop.u32X = stVideoFrame.frameParam.crop.left;
                stParamInfo.stCrop.u32Y = stVideoFrame.frameParam.crop.top + pstCapbDisp->disp_h_top_padding;
                stParamInfo.stCrop.sInImageSize.u32Width = stVideoFrame.frameParam.width;
                stParamInfo.stCrop.sInImageSize.u32Height = stVideoFrame.frameParam.height;
            }
            else
            {
                stParamInfo.stCrop.u32H = stVideoFrame.frameParam.crop.height;
                stParamInfo.stCrop.u32W = stVideoFrame.frameParam.crop.width;
                stParamInfo.stCrop.u32X = stVideoFrame.frameParam.crop.left;
                stParamInfo.stCrop.u32Y = stVideoFrame.frameParam.crop.top;
                stParamInfo.stCrop.sInImageSize.u32Width = pstCapbXpack->xpack_disp_buffer_w;
                stParamInfo.stCrop.sInImageSize.u32Height = pstCapbDisp->disp_yuv_h_max;
            }

            if (SAL_SOK != dup_task_setDupParam(pDupHandle, &stParamInfo))
            {
                XPACK_LOGW("dup_task_setDupParam viChn %d error !\n", chan);
            }
        }

        /* 送往硬件处理模块 */
        dtime_add_time_point(pSendVpssDTime, s32DTimeItemIdx, SAL_TRUE, "sendStart", -1, SAL_TRUE);
        if (SAL_SOK != dup_task_sendToDup(pDupHandle, pstGetPutFrame))
        {
            XPACK_LOGE("dup_task_sendToDup viChn %d error, send data type:%d, top offset:%u, disp img[w:%u, h:%u, s:%u]!\n", 
                       chan, enSendDataType, pstXpackChnPrm->u32DispVertOffset, pstDispImg->u32Width, pstDispImg->u32Height, pstDispImg->u32Stride[0]);
        }
        dtime_add_time_point(pSendVpssDTime, s32DTimeItemIdx, SAL_TRUE, "sendEnd", -1, SAL_TRUE);

        /* 等待定时器唤醒，控制帧率 */
        SAL_TimerHpWait(pstSendDupPrm->fd);
        dtime_add_time_point(pSendVpssDTime, s32DTimeItemIdx, SAL_TRUE, "timerE", -1, SAL_TRUE);
        dtime_add_time_point(pSendVpssDTime, s32DTimeItemIdx, SAL_TRUE, "dispRes", s32DispBufLeftSize, SAL_FALSE);
    }

    return NULL;
}


/**
 * @function:   xpack_update_segment_cb_info
 * @brief:      更新分片信息
 * @param[IN]:  UINT32          chan            视角通道号
 * @param[IN]:  UINT32          u32PackId       包裹id
 * @param[IN]:  XSP_PACKAGE_TAG enPackTag       条带数据的包裹标记
 * @param[IN]:  UINT32          u32Width        条带宽度
 * @param[IN]:  BOOL            bForceDiv       是否是强制分割标记
 * @param[IN]:  UINT64          u64DispDataLoc  包裹起始在显示缓存中的存储位置索引
 * @param[IN]:  UINT32          u32RawDataLoc   包裹起始在raw数据全屏分段缓存中的存储位置索引
 * @return:     SAL_STATUS      成功-SAL_SOK，失败-SAL_FAIL
 */
static SAL_STATUS xpack_update_segment_cb_info(UINT32 chan, UINT32 u32PackId, XSP_PACKAGE_TAG enPackTag, UINT32 u32Width, BOOL bForceDiv, UINT64 u64DispDataLoc, UINT32 u32RawDataLoc)
{
    XPACK_SEGMENT_CB_CTRL *pstSegCbCtrl = NULL;
    XPACK_PACK_SEGMENT_PRM *pstPackSegInfo = NULL;

    pstSegCbCtrl = &gXpackSegmentPicCtrl[chan];
    if (SAL_TRUE != pstSegCbCtrl->enable)
    {
        return SAL_TRUE;
    }

    pstPackSegInfo = &pstSegCbCtrl->astPackSegInfo[pstSegCbCtrl->u32CurSegInfoIdx];
    SAL_mutexLock(pstSegCbCtrl->mChnMutexHdl);

    if (XSP_PACKAGE_START == enPackTag)
    {
        pstPackSegInfo->uiIsForcedToSeparate = 0;
        pstPackSegInfo->u32PackIndx = u32PackId;
        pstPackSegInfo->u64DispDataLoc = u64DispDataLoc;
        pstPackSegInfo->u32RawDataLoc = u32RawDataLoc;
        pstPackSegInfo->flag = 0;
        pstPackSegInfo->u32PackWidth = u32Width;    // 最后更新包裹宽度，回调线程根据宽度进行分片回调
    }
    else if (XSP_PACKAGE_MIDDLE == enPackTag)
    {
        if (0 != pstPackSegInfo->u32PackIndx)
        {
            pstPackSegInfo->uiIsForcedToSeparate = 0;
            pstPackSegInfo->flag = 1;
            pstPackSegInfo->u32PackWidth += u32Width;       // 最后更新包裹宽度，回调线程根据宽度进行分片回调
        }
        else
        {
            XPACK_LOGW("chan %u pack seg info got pack middle flag but without start flag, skip it\n", chan);
        }
    }
    else if (XSP_PACKAGE_END == enPackTag)
    {
        if (0 != pstPackSegInfo->u32PackIndx)
        {
            pstPackSegInfo->uiIsForcedToSeparate = bForceDiv;
            pstPackSegInfo->flag = 2;
            pstPackSegInfo->u32PackWidth += u32Width;       // 最后更新包裹宽度，回调线程根据宽度进行分片回调

            pstSegCbCtrl->u32CurSegInfoIdx++;
            if (pstSegCbCtrl->u32CurSegInfoIdx >= XPACK_PACK_SEGCB_STORE_NUM)
            {
                pstSegCbCtrl->u32CurSegInfoIdx = 0;
            }
        }
        else
        {
            XPACK_LOGW("chan %u pack seg info got pack end flag but without start flag, skip it\n", chan);
        }
    }
    else if (XSP_PACKAGE_NONE == enPackTag)
    {
        /* do nothing */
    }
    else
    {
        XPACK_LOGE("chan %u invalide pack flag[%d]\n", chan, enPackTag);
    }
    SAL_mutexUnlock(pstSegCbCtrl->mChnMutexHdl);

    return SAL_SOK;
}


/**
 * @function:   xpack_get_seg_cb_data
 * @brief:      获取分片数据
 * @param[IN]:  UINT32                 chan            视角通道号
 * @param[IN]:  XPACK_SAVE_TYPE_E      enSegCbType     分片回调类型
 * @param[IN]:  XPACK_PACK_SEGMENT_PRM *pstPackSegInfo 包裹分片信息
 * @param[IN]:  UINT32                 u32UsedLen      已回调数据长度,jpg/bmp为图像宽,raw为图像高 
 * @param[IN]:  UINT32                 u32SegLen       获取数据长度,jpg/bmp为图像宽,raw为图像高 
 * @param[OUT]: XIMAGE_DATA_ST         *pstImgOut      输出图像,宽高由函数内部指定为分片宽高
 * @return:     SAL_STATUS             成功-SAL_SOK，失败-SAL_FAIL
 */
static SAL_STATUS xpack_get_seg_cb_data(UINT32 chan, XPACK_SAVE_TYPE_E enSegCbType, XPACK_PACK_SEGMENT_PRM *pstPackSegInfo, UINT32 u32UsedLen, UINT32 u32SegLen, XIMAGE_DATA_ST *pstImgOut)
{
    SAL_VideoCrop stCropPrm = {0};
    CAPB_DISP *pstCapbDisp = capb_get_disp();

    XPACK_CHECK_PTR_NULL(pstPackSegInfo, SAL_FAIL);
    XPACK_CHECK_PTR_NULL(pstImgOut, SAL_FAIL);

    if (XPACK_SAVE_BMP == enSegCbType || XPACK_SAVE_JPEG == enSegCbType)
    {
        stCropPrm.top = pstCapbDisp->disp_h_top_blanking + pstCapbDisp->disp_h_top_padding;
        stCropPrm.left = 0;
        stCropPrm.width = u32SegLen;
        stCropPrm.height = pstCapbDisp->disp_h_middle_indeed;
        pstImgOut->u32Height = stCropPrm.height;
        if (DSP_IMG_DATFMT_BGRA32 == pstImgOut->enImgFmt || DSP_IMG_DATFMT_BGR24 == pstImgOut->enImgFmt)
        {
            /* rgb在rk或x86平台使用，rk分平台jpg编码时stride需要64对齐，所以在此提前设置stride为64对齐，避免编码时需要调整内存 */
            ximg_set_dimension(pstImgOut, stCropPrm.width, SAL_align(stCropPrm.width, 64), SAL_FALSE, 0);
        }
        else if (pstImgOut->enImgFmt & DSP_IMG_DATFMT_YUV420_MASK)
        {
            /* yuv在nt平台使用，nt分平台jpg编码时不支持stride，故在此提前将图像宽度和stride设置相等 */
            ximg_set_dimension(pstImgOut, stCropPrm.width, stCropPrm.width, SAL_FALSE, 0);
        }

        if (SAL_SOK != ring_buf_crop(gXpackRingBufHandle[chan][XPACK_DISPDT_PURE], pstPackSegInfo->u64DispDataLoc, u32UsedLen, &stCropPrm, pstImgOut))
        {
            XPACK_LOGE("chan %u ring_buf_crop failed\n", chan);
            return SAL_FAIL;
        }
    }
    else if (XPACK_SAVE_SRC == enSegCbType)
    {
        if (SAL_SOK != segment_buf_crop(gRtTipRawSegBufHandle[chan], pstPackSegInfo->u32RawDataLoc, u32UsedLen, u32SegLen, pstImgOut))
        {
            XPACK_LOGE("chan %u segment_buf_crop failed\n", chan);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/**
 * @fn      xpack_package_segment_thread
 * @brief   包裹分片回调线程
 * 
 * @param   [IN] pPrm XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 */
static void *xpack_package_segment_thread(void *pPrm)
{
    INT32 s32CachedIndx = 0;        // pack cached index, 当前分片回调的包裹缓存索引
    INT32 s32CbSegIndx = 0;         // callback segment index, 回调的分片索引
    INT32 ret = 0;
    UINT32 chan = 0;
    BOOL bIfExecuteCb = SAL_FALSE;
    UINT32 u32SegUsedWidth = 0;
    UINT32 u32OutDataSize = 0;
    UINT32 u32SegSetWidth = 0;
    UINT32 u32RemainWidth = 0;
    UINT32 u32PackUsedWidth = 0;
    XPACK_SAVE_TYPE_E enSegmentType = XPACK_SAVE_JPEG; /* 分片编码类型 JPEG BMP NRAW*/
    XRAY_TRANS_TYPE enSaveType = XRAY_TRANS_BMP;
    XPACK_SEGMENT_CB_CTRL *pstSegCbCtrl = NULL;
    XPACK_PACK_SEGMENT_PRM *pstPackSegInfo = NULL;

    UINT64 u64PhyAddr = 0;
    VOID *OutAddrPrm = NULL;
    XIMAGE_DATA_ST stSegCbImg = {0};
    XIMAGE_DATA_ST stSegCbRaw = {0};
    XIMAGE_DATA_ST *pstSegCb = NULL;
    XPACK_DSP_SEGMENT_OUT stXpackSegmentOut = {0};
    STREAM_ELEMENT stStreamEle = {0};

    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    DSP_IMG_DATFMT enRawImgFmt = DSP_IMG_DATFMT_LHZ16P;
    XPACK_DUMP_CFG *pstDumpCfg = NULL;
    XPACK_GLOBAL_PRM *pGlobalPrm = &gGlobalPrm;
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;

    static UINT32 su32LastPackIndx[XPACK_CHN_MAX] = {0};

    chan = (UINT32)(PhysAddr)pPrm;
    pstSegCbCtrl = &gXpackSegmentPicCtrl[chan];
    pstXpackChnPrm = &pGlobalPrm->astXpackChnPrm[chan];
    pstDumpCfg = &pstXpackChnPrm->stDumpCfg;
    pstSegCbCtrl->u32SegmentWidth = XAPCK_SEGMENT_DEFAULT_WIDTH; /*默认分片大小*/

    if (XRAY_ENERGY_SINGLE == pstCapbXray->xray_energy_num)
    {
        enRawImgFmt = DSP_IMG_DATFMT_SP16;
    }
    else
    {
        enRawImgFmt = DSP_IMG_DATFMT_LHZP;
    }

    stSegCbImg.stMbAttr.bCached = SAL_FALSE;
    ret = ximg_create(XAPCK_SEGMENT_MAX_WIDTH, pstCapbXsp->xraw_width_resized, pstCapbXsp->enDispFmt, "seg_cb_disp", NULL, &stSegCbImg);
    if (ret != SAL_SOK)
    {
        XPACK_LOGE("ximg_create failed\n");
        return NULL;
    }
    ret = mem_hal_mmzAlloc(XAPCK_SEGMENT_MAX_WIDTH * pstCapbXsp->xraw_width_resized * 5, "xpack", "xpack_segment", NULL, SAL_TRUE,
                           &u64PhyAddr, (VOID **)&OutAddrPrm);
    if (ret != SAL_SOK)
    {
        XPACK_LOGE("mem_hal_mmzAlloc failed with %#x\n", ret);
        return NULL;
    }

    SAL_SET_THR_NAME();
    while (1)
    {
        SAL_mutexLock(pstSegCbCtrl->mChnMutexHdl);
        if (SAL_FALSE == pstSegCbCtrl->enable)
        {
            SAL_mutexWait(pstSegCbCtrl->mChnMutexHdl);
        }

        SAL_mutexUnlock(pstSegCbCtrl->mChnMutexHdl);

        memset(&stStreamEle, 0, sizeof(STREAM_ELEMENT));
        stStreamEle.type = STREAM_ELEMENT_SEGMENT_XPACK;
        stStreamEle.chan = chan;

        for (s32CachedIndx = 0; s32CachedIndx < XPACK_PACK_SEGCB_STORE_NUM; )
        {
            pstPackSegInfo = &pstSegCbCtrl->astPackSegInfo[s32CachedIndx];
            if (XRAY_PS_RTPREVIEW != pstXpackChnPrm->enProcStat)
            {
                SAL_msleep(300);
                continue;
            }
            if (0 == pstPackSegInfo->u32PackWidth)
            {
                SAL_msleep(50);
                s32CachedIndx++;
                continue;
            }

            if (su32LastPackIndx[chan] != pstPackSegInfo->u32PackIndx)
            {
                /*防止前级不发结束标志 又出现下发开始标志 */
                s32CbSegIndx = 0;
                u32PackUsedWidth = 0;
                su32LastPackIndx[chan] = pstPackSegInfo->u32PackIndx;
            }
            enSegmentType = pstSegCbCtrl->enSegmentType;
            /* 图像处理后，重新发送当前包裹分片 */
            if (0 != u32PackUsedWidth && SAL_TRUE == pstXpackChnPrm->bIfRefresh && XPACK_SAVE_SRC != enSegmentType)
            {
                pstXpackChnPrm->bIfRefresh = SAL_FALSE;
                s32CbSegIndx = 0;
                u32PackUsedWidth = 0;
                pstPackSegInfo->cnttime = 0;
            }

            /*剩余包裹宽*/
            u32SegSetWidth = pstSegCbCtrl->u32SegmentWidth;
            u32RemainWidth = SAL_SUB_SAFE(pstPackSegInfo->u32PackWidth, u32PackUsedWidth);

            if (XPACK_SAVE_BMP == enSegmentType)
            {
                enSaveType = XRAY_TRANS_BMP;
                pstSegCb = &stSegCbImg;
            }
            else if (XPACK_SAVE_JPEG == enSegmentType)
            {
                enSaveType = XRAY_TRANS_JPG;
                pstSegCb = &stSegCbImg;
            }

            /* 包裹宽度超出分片宽度，正常回调分片 */
            if (u32RemainWidth >= u32SegSetWidth)
            {
                bIfExecuteCb = SAL_TRUE;
                u32SegUsedWidth = u32SegSetWidth;
                if (s32CbSegIndx == 0)
                {
                    stXpackSegmentOut.flag = 0;
                }
                else if (u32RemainWidth == u32SegSetWidth && 2 == pstPackSegInfo->flag)
                {
                    stXpackSegmentOut.flag = 2;
                }
                else
                {
                    stXpackSegmentOut.flag = 1;
                }
            }
            /* 包裹已完整且剩余回调部分不足一个分片 */
            else if (u32RemainWidth > 0 && u32RemainWidth < u32SegSetWidth && 2 == pstPackSegInfo->flag)
            {
                bIfExecuteCb = SAL_TRUE;
                u32SegUsedWidth = u32RemainWidth < 32 ? 32 : SAL_align(u32RemainWidth, 16); /* jpg抓图最小32 16对齐 */
                stXpackSegmentOut.flag = 2;
                /* 空白背景色填充 */
                ximg_fill_color(pstSegCb, 0, pstSegCb->u32Height, 0, pstSegCb->u32Width, pstXpackChnPrm->u32DispBgColor);
            }
            /* 包裹已完整且已回调全部分片，重新发上一个分片，将其置为结束分片 */
            else if (u32RemainWidth == 0 && pstPackSegInfo->flag == 2)
            {
                bIfExecuteCb = SAL_FALSE;   // 直接重新回调，不用重新获取数据
                stXpackSegmentOut.flag = 2;
                SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stXpackSegmentOut, sizeof(XPACK_DSP_SEGMENT_OUT));

                XPACK_LOGI("chan %d repeat_cb, packIndx:%d, cachedIndx:%d, CbSegIndx:%d, flag:%d forceDiv:%d packW:%d usedW:%d remainW:%d segW:%d dataLoc:(%llu, %u), size:%u\n",
                            chan, stXpackSegmentOut.u32PackIndx, s32CachedIndx, stXpackSegmentOut.u32SegmentIndx, stXpackSegmentOut.flag, stXpackSegmentOut.uiIsForcedToSeparate, 
                            pstPackSegInfo->u32PackWidth, u32PackUsedWidth, u32RemainWidth, u32SegSetWidth, pstPackSegInfo->u64DispDataLoc, pstPackSegInfo->u32RawDataLoc, stXpackSegmentOut.stSegmentDataPrm.u32DataSize);
                /* 已回调结束分片，重置控制参数 */
                SAL_mutexLock(pstSegCbCtrl->mChnMutexHdl);
                memset(pstPackSegInfo, 0, sizeof(XPACK_PACK_SEGMENT_PRM));
                SAL_mutexUnlock(pstSegCbCtrl->mChnMutexHdl);
                s32CachedIndx++;
                s32CbSegIndx = 0;
                u32PackUsedWidth = 0;
                u32RemainWidth = 0;
            }
            /* 超时处理，包裹没有结束表示时使用 默认超时250ms */
            else if (pstPackSegInfo->cnttime > XAPCK_SEGMENT_TIMEOUT && u32RemainWidth > 0 && 0)
            {
                bIfExecuteCb = SAL_FALSE;
                /* 复位 */
                XPACK_LOGW("time out chan %d, cachedIndx %d packindx %d flag %d u32RemainWidth %d\n", chan, s32CachedIndx, pstPackSegInfo->u32PackIndx, pstPackSegInfo->flag, u32RemainWidth);
                stXpackSegmentOut.flag = 2;
            }
            else
            {
                bIfExecuteCb = SAL_FALSE;
                pstPackSegInfo->cnttime++;
                SAL_msleep(15);
            }

            if (SAL_TRUE == bIfExecuteCb)
            {
                /* 获取分片数据 */
                if (XPACK_SAVE_SRC == enSegmentType)
                {
                    ret = ximg_create(pstCapbXsp->xraw_width_resized, u32SegUsedWidth, enRawImgFmt, NULL, OutAddrPrm, &stSegCbRaw);
                    if (ret != SAL_SOK)
                    {
                        XPACK_LOGE("ximg_create failed\n");
                        return NULL;
                    }
                    pstSegCb = &stSegCbRaw;
                }

                ret = xpack_get_seg_cb_data(chan, enSegmentType, pstPackSegInfo, u32PackUsedWidth, u32SegUsedWidth, pstSegCb);
                if (ret != SAL_SOK)
                {
                    XPACK_LOGE("chan %d xpack_get_seg_cb_data failed, packIndx:%d, cachedIndx:%d, CbSegIndx:%d, flag:%d, packW:%d usedW:%d remainW:%d segW:%d dataLoc:%llu\n", 
                                chan, pstPackSegInfo->u32PackIndx, s32CachedIndx, s32CbSegIndx, pstPackSegInfo->flag, 
                                pstPackSegInfo->u32PackWidth, u32PackUsedWidth, u32RemainWidth, u32SegUsedWidth, pstPackSegInfo->u64DispDataLoc);
                }
                if (pstDumpCfg->u32DumpCnt > 0 && pstDumpCfg->unDumpDp.stXpackDumpType.XPACK_DP_SEGCB)
                {
                    ximg_dump(pstSegCb, chan, pstDumpCfg->chDumpDir, "seg-cb", NULL, pstDumpCfg->u32DumpCnt);
                    pstDumpCfg->u32DumpCnt = SAL_SUB_SAFE(pstDumpCfg->u32DumpCnt, 1);
                }

                if (enSegmentType == XPACK_SAVE_JPEG || enSegmentType == XPACK_SAVE_BMP)
                {
                    /* 将原始图像数据保存为对应图片格式 */
                    if (SAL_SOK != ximg_save(pstSegCb, enSaveType, OutAddrPrm, &u32OutDataSize, 0))
                    {
                        XSP_LOGE("chan %u ximg_save failed, save type:%d, w:%u, h:%u, s:%u\n", 
                                    chan, enSaveType, pstSegCb->u32Width, pstSegCb->u32Height, pstSegCb->u32Stride[0]);
                    }
                }
                else
                {
                    if (SAL_SOK != ximg_get_size(pstSegCb, &u32OutDataSize))
                    {
                        XSP_LOGE("chan %u ximg_get_size failed\n", chan);
                    }
                }

                /* 分片数据回调 */
                stXpackSegmentOut.uiIsForcedToSeparate              = pstPackSegInfo->uiIsForcedToSeparate;
                stXpackSegmentOut.u32PackIndx                       = pstPackSegInfo->u32PackIndx;
                stXpackSegmentOut.u32SegmentIndx                    = s32CbSegIndx + 1;
                stXpackSegmentOut.Direction                         = pstXpackChnPrm->enRtDirection;
                stXpackSegmentOut.stSegmentDataPrm.SegmentDataTpye  = enSegmentType;
                stXpackSegmentOut.stSegmentDataPrm.u32SegmentWidth  = pstSegCb->u32Width;
                stXpackSegmentOut.stSegmentDataPrm.u32SegmentHeight = pstSegCb->u32Height;
                stXpackSegmentOut.stSegmentDataPrm.u32DataSize      = u32OutDataSize;
                stXpackSegmentOut.stSegmentDataPrm.pOutBuff         = OutAddrPrm;
                SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stXpackSegmentOut, sizeof(XPACK_DSP_SEGMENT_OUT));

                if (1 != stXpackSegmentOut.flag)
                {
                    XPACK_LOGI("chan %d, packIndx:%d, cachedIndx:%d, CbSegIndx:%d, flag:%d forceDiv:%d packW:%d usedW:%d remainW:%d segW:%d dataLoc:%llu, %u, size:%u\n",
                                chan, stXpackSegmentOut.u32PackIndx, s32CachedIndx, stXpackSegmentOut.u32SegmentIndx, stXpackSegmentOut.flag, stXpackSegmentOut.uiIsForcedToSeparate, 
                                pstPackSegInfo->u32PackWidth, u32PackUsedWidth, u32RemainWidth, u32SegUsedWidth, pstPackSegInfo->u64DispDataLoc, pstPackSegInfo->u32RawDataLoc, stXpackSegmentOut.stSegmentDataPrm.u32DataSize);
                }

                /* 更新控制参数 */
                if (2 == stXpackSegmentOut.flag)
                {
                    /* 已回调结束分片，重置控制参数 */
                    SAL_mutexLock(pstSegCbCtrl->mChnMutexHdl);
                    memset(pstPackSegInfo, 0, sizeof(XPACK_PACK_SEGMENT_PRM));
                    SAL_mutexUnlock(pstSegCbCtrl->mChnMutexHdl);
                    s32CachedIndx++;
                    s32CbSegIndx = 0;
                    u32PackUsedWidth = 0;
                    u32RemainWidth = 0;
                }
                else
                {
                    s32CbSegIndx++;
                    u32PackUsedWidth += u32SegUsedWidth;
                    u32RemainWidth = SAL_SUB_SAFE(pstPackSegInfo->u32PackWidth, u32PackUsedWidth);
                    pstPackSegInfo->cnttime = 0;
                }
            }
        }
    }
}


/**
 * @fn      Xpack_DrvSetSegCbPrm
 * @brief   包裹分片回调参数设置
 * 
 * @param   [IN] chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * @param   [IN] pstSegPrm 包裹分片参数
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvSetSegCbPrm(UINT32 chan, XPACK_DSP_SEGMENT_SET *pstSegPrm)
{
    int i = 0;
    XPACK_SEGMENT_CB_CTRL *pSegCbCtrl = NULL;

    XPACK_CHECK_PTR_NULL(pstSegPrm, SAL_FAIL);
    XPACK_CHECK_CHAN(chan, SAL_FAIL);

    if (pstSegPrm->SegmentWidth > XAPCK_SEGMENT_MAX_WIDTH || pstSegPrm->SegmentWidth % 4 != 0 || pstSegPrm->SegmentWidth < 32)
    {
        XPACK_LOGE("chan %d segment width %d is illegal \n", chan, pstSegPrm->SegmentWidth);
        return SAL_FAIL;
    }
    if (pstSegPrm->SegmentDataTpyeTpye < 0 || pstSegPrm->SegmentDataTpyeTpye > 2)
    {
        XPACK_LOGE("chan %d segment type %d is illegal \n", chan, pstSegPrm->SegmentDataTpyeTpye);
        return SAL_FAIL;
    }

    pSegCbCtrl = &gXpackSegmentPicCtrl[chan];

    SAL_mutexLock(pSegCbCtrl->mChnMutexHdl);

    if (pSegCbCtrl->enable != pstSegPrm->bSegFlag)
    {
        for (i = 0; i < XPACK_PACK_SEGCB_STORE_NUM; i++)
        {
            memset(&pSegCbCtrl->astPackSegInfo[i], 0, sizeof(XPACK_PACK_SEGMENT_PRM));
        }
    }

    pSegCbCtrl->enable = pstSegPrm->bSegFlag;
    pSegCbCtrl->u32SegmentWidth = pstSegPrm->SegmentWidth;
    pSegCbCtrl->enSegmentType = pstSegPrm->SegmentDataTpyeTpye;
    SAL_mutexSignal(pSegCbCtrl->mChnMutexHdl);
    SAL_mutexUnlock(pSegCbCtrl->mChnMutexHdl);

    XPACK_LOGI("chn %d enable %d enctype %d width %d\n", chan, pSegCbCtrl->enable, pSegCbCtrl->enSegmentType, pSegCbCtrl->u32SegmentWidth);
    return SAL_SOK;
}


/**
 * @fn      xpack_init_disp_data_send
 * @brief   初始化显示图像的发送业务，包括线程及其参数
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：失败
 */
static SAL_STATUS xpack_init_disp_data_send(VOID)
{
    SAL_STATUS s32Ret = SAL_SOK;
    INT32 i = 0, j = 0;
    UINT32 u32ThreadPri = 0;
    XPACK_GLOBAL_PRM *pGlobalPrm = &gGlobalPrm;
    XPACK_SENDDUP_PRM *pstSendDupPrm = NULL;
    CAPB_XRAY_IN *pstCapXray = capb_get_xrayin();
    CAPB_XPACK *pstCapbXpack = capb_get_xpack();

    for (i = 0; i < pstCapXray->xray_in_chan_cnt; i++)
    {
        for (j = 0; j < pstCapbXpack->u32XpackSendChnNum; j++)
        {
            pstSendDupPrm = &pGlobalPrm->astXpackChnPrm[i].stSendDupPrm[j];
            pstSendDupPrm->chan = i;
            pstSendDupPrm->enSendDataType = j;
            pstSendDupPrm->pSendDupDTime = dtime_create(XPACK_DUP_TIME_NUM);

            memset(&pstSendDupPrm->stGetPutFrame0, 0x00, sizeof(SYSTEM_FRAME_INFO));
            if (pstSendDupPrm->stGetPutFrame0.uiAppData == 0x00)
            {
                s32Ret = sys_hal_allocVideoFrameInfoSt(&pstSendDupPrm->stGetPutFrame0);
                if (s32Ret != SAL_SOK)
                {
                    XPACK_LOGE("Disp_halGetFrameMem error !\n");
                    return s32Ret;
                }
            }

            pstSendDupPrm->fd = SAL_TimerHpCreate(pGlobalPrm->xpack_fps);
            if (pstSendDupPrm->fd < 0)
            {
                XPACK_LOGE("chan %d send data type:%d, timer fd create failed, fps:%u\n", i, j, pGlobalPrm->xpack_fps);
                return SAL_FAIL;
            }

            if (XPACK_DISPDT_PURE == j)
            {
                u32ThreadPri = SAL_THR_PRI_DEFAULT;
                pstSendDupPrm->pDupHandle = dupHandle2[i];
            }
            else if (XPACK_DISPDT_OSD == j)
            {
                u32ThreadPri = 80;
                pstSendDupPrm->pDupHandle = dupHandle1[i];
            }
            else
            {
                XPACK_LOGE("Not supported send display data type[XPACK_DISPDT_ENHANCE]\n");
                return SAL_FAIL;
            }

            SAL_thrCreate(&pstSendDupPrm->stChnThrHandl, xpack_disp_data_send_thread, u32ThreadPri, 0, (void *)pstSendDupPrm);
        }
    }
    for (j = 0; j < pstCapbXpack->u32XpackSendChnNum; j++)
    {
        if (SAL_SOK != SAL_CondInit(&pGlobalPrm->condDispCacheSync[j]))
        {
            XSP_LOGE("SAL_CondInit 'condDispCacheSync' failed, xpack send data type:%d\n", j);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/**
 * @fn      xpack_init_package_segment
 * @brief   初始化包裹分片业务，包括线程及其参数
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：失败
 */
static SAL_STATUS xpack_init_package_segment(VOID)
{
    INT32 chan = 0;
    XPACK_SEGMENT_CB_CTRL *pXpackSegmentPicCtrl = NULL;
    CAPB_XRAY_IN *pstCapXray = capb_get_xrayin();

    for (chan = 0; chan < pstCapXray->xray_in_chan_cnt; chan++)
    {
        pXpackSegmentPicCtrl = &gXpackSegmentPicCtrl[chan];
        pXpackSegmentPicCtrl->enable = 0;
        SAL_mutexCreate(SAL_MUTEX_NORMAL, &pXpackSegmentPicCtrl->mChnMutexHdl);
        SAL_thrCreate(&(pXpackSegmentPicCtrl->stChnThrHandl), xpack_package_segment_thread, SAL_THR_PRI_DEFAULT, 0, (void *)((PhysAddr)chan));
    }

    return SAL_SOK;
}


/**
 * @fn      xpack_init_package_segment
 * @brief   初始化显示相关数据（显示RGB/YUV、NRaw、融合灰度图等）Buffer
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：失败
 */
static SAL_STATUS xpack_init_disp_data_buf(VOID)
{
    INT32 i = 0;
    SAL_STATUS ret = SAL_SOK;
    UINT32 chan = 0;
    DSP_IMG_DATFMT enImgFmt = DSP_IMG_DATFMT_UNKNOWN;
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    CAPB_XPACK *pstCapbXpack = capb_get_xpack();
    CAPB_XRAY_IN *pstCapXray = capb_get_xrayin();

    for (chan = 0; chan < pstCapXray->xray_in_chan_cnt; chan++)
    {
        for (i = 0; i < pstCapbXpack->u32XpackSendChnNum; i++)
        {
            gXpackRingBufHandle[chan][i] = ring_buf_init(pstCapbXpack->xpack_disp_buffer_w, pstCapbDisp->disp_yuv_h_max, pstCapbDisp->disp_yuv_w_max, pstCapbDisp->disp_yuv_h, 
                                                         SAL_MAX(pstCapbXsp->xsp_fsc_proc_width_max, pstCapbXsp->xsp_package_line_max), pstCapbXsp->enDispFmt, 1);
            if (NULL == gXpackRingBufHandle[chan][i])
            {
                XPACK_LOGE("ring_buf_init failed, w:%u, h:%u, screen w:%u, max crop w:%u\n", pstCapbXpack->xpack_disp_buffer_w, pstCapbDisp->disp_yuv_h_max,
                           pstCapbDisp->disp_yuv_w_max, pstCapbXsp->xsp_fsc_proc_width_max);
                return SAL_FAIL;
            }
        }
        if (XRAY_ENERGY_SINGLE == pstCapXray->xray_energy_num)
        {
            enImgFmt = DSP_IMG_DATFMT_SP16;
        }
        else
        {
            enImgFmt = DSP_IMG_DATFMT_LHZP;
        }
        /* 初始化tip-raw缓存 */
        gRtTipRawSegBufHandle[chan] = segment_buf_init(pstCapbXsp->xraw_width_resized, pstCapbXsp->xsp_fsc_proc_width_max, 
                                                       pstCapbDisp->disp_yuv_w_max, enImgFmt);
        /* 初始化融合灰度图缓存 */
        gRtBlendSegBufHandle[chan] = segment_buf_init(pstCapbXsp->xraw_width_resized, pstCapbXsp->xsp_fsc_proc_width_max, 
                                                     pstCapbDisp->disp_yuv_w_max, DSP_IMG_DATFMT_SP16);
        /* 初始化回拉全屏raw数据缓存 */
        gPbRawSegBufHandle[chan] = segment_buf_init(pstCapbXsp->xraw_width_resized, pstCapbXsp->xsp_fsc_proc_width_max, 
                                                    pstCapbDisp->disp_yuv_w_max, enImgFmt);
        /* 初始化回拉全屏blend数据缓存 */
        gPbBlendSegBufHandle[chan] = segment_buf_init(pstCapbXsp->xraw_width_resized, pstCapbXsp->xsp_fsc_proc_width_max, 
                                                      pstCapbDisp->disp_yuv_w_max, DSP_IMG_DATFMT_SP16);

        if (NULL == gRtTipRawSegBufHandle[chan] || NULL == gRtTipRawSegBufHandle[chan] || 
            NULL == gPbRawSegBufHandle[chan] || NULL == gPbBlendSegBufHandle[chan])
        {
            XPACK_LOGE("segment_buf_init failed\n");
            return SAL_FAIL;
        }

        xpack_disp_data_reset(chan, XRAY_DIRECTION_LEFT);
    }

    return ret;
}


/**
 * @fn      xpack_init_package_jpg_cb
 * @brief   初始化包裹JPG的回调
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：失败
 */
static SAL_STATUS xpack_init_package_jpg_cb(VOID)
{
    INT32 ret = SAL_SOK;
    INT32 chan = 0, i = 0;
    MEDIA_BUF_ATTR stJpegMb = {0};
    DSA_NODE *pNode = NULL;
    PACK_JPG_CB_CTRL *pstPackJpgCbCtrl = NULL;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    CAPB_XSP *pstCapXsp = capb_get_xsp();

    for (chan = 0; chan < pstCapbXray->xray_in_chan_cnt; chan++)
    {
        pstPackJpgCbCtrl = &gGlobalPrm.astXpackChnPrm[chan].stPackJpgCbCtrl;
        pstPackJpgCbCtrl->bJpgDrawSwitch = SAL_FALSE;
        pstPackJpgCbCtrl->bJpgCropSwitch = SAL_TRUE;
        pstPackJpgCbCtrl->u32BackW = XPACK_JPG_DEFAULT_WIDTH;
        pstPackJpgCbCtrl->u32BackH = XPACK_JPG_DEFAULT_HEIGHT;
        pstPackJpgCbCtrl->f32WidthResizeRatio = 1.0;
        pstPackJpgCbCtrl->f32HeightResizeRatio = 1.0;

        stJpegMb.memSize = XPACK_JPG_MAX_WIDTH * XPACK_JPG_MAX_HEIGHT * 4;
        ret = xpack_media_buf_alloc(&stJpegMb, "jpeg");
        if (ret != SAL_SOK)
        {
            XPACK_LOGE("inChn %d error !\n", chan);
            return SAL_FAIL;
        }
        pstPackJpgCbCtrl->pPicJpeg = stJpegMb.virAddr;

        /* 尺寸缩放后图像内存 */
        pstPackJpgCbCtrl->stImgResized.stMbAttr.bCached = SAL_FALSE;
        ret = ximg_create(XPACK_JPG_MAX_WIDTH, XPACK_JPG_MAX_HEIGHT, pstCapXsp->enDispFmt, "jpg-imgResize", NULL, &pstPackJpgCbCtrl->stImgResized);
        if (ret != SAL_SOK)
        {
            XPACK_LOGE("ximg_create failed, w:%u, h:%u, fmt:0x%x\n", XPACK_JPG_MAX_WIDTH, XPACK_JPG_MAX_HEIGHT, pstCapXsp->enDispFmt);
            return SAL_FAIL;
        }

        /* 背板填充图像内存 */
        pstPackJpgCbCtrl->stImgFilled.stMbAttr.bCached = SAL_FALSE;
        ret = ximg_create(XPACK_JPG_MAX_WIDTH, XPACK_JPG_MAX_HEIGHT, pstCapXsp->enDispFmt, "jpg-imgBack", NULL, &pstPackJpgCbCtrl->stImgFilled);
        if (ret != SAL_SOK)
        {
            XPACK_LOGE("ximg_create failed, w:%u, h:%u, fmt:0x%x\n", XPACK_JPG_MAX_WIDTH, XPACK_JPG_MAX_HEIGHT, pstCapXsp->enDispFmt);
            return SAL_FAIL;
        }

        /* 初始化jpeg编码列表 */
        if (NULL == (pstPackJpgCbCtrl->lstJpegEnc = DSA_LstInit(XPACK_PACK_JPGCB_NODE_NUM)))
        {
            XPACK_LOGE("chan %u, init 'lstJpegEnc' failed\n", chan);
            return SAL_FAIL;
        }
        if (SAL_SOK != SAL_CondInit(&pstPackJpgCbCtrl->lstJpegEnc->sync))
        {
            XPACK_LOGE("SAL_CondInit 'lstJpegEnc->sync' failed\n");
            return SAL_FAIL;
        }
        for (i = 0; i < XPACK_PACK_JPGCB_NODE_NUM; i++)
        {
            if (NULL != (pNode = DSA_LstGetIdleNode(pstPackJpgCbCtrl->lstJpegEnc)))
            {
                pNode->pAdData = pstPackJpgCbCtrl->astJpgEncNode + i;
                SAL_CondInit(&(pstPackJpgCbCtrl->astJpgEncNode + i)->condJpegEnc);
                DSA_LstPush(pstPackJpgCbCtrl->lstJpegEnc, pNode);
            }
        }
        for (i = 0; i < XPACK_PACK_JPGCB_NODE_NUM; i++)
        {
            DSA_LstPop(pstPackJpgCbCtrl->lstJpegEnc);
        }

        /* 创建jpeg编码线程 */
        SAL_thrCreate(&pstPackJpgCbCtrl->stJpegEncThrHandl, xpack_package_jpg_enc_thread, SAL_THR_PRI_DEFAULT, 0, (void *)(PhysAddr)chan);
    }

    return SAL_SOK;
}


/**
 * @fn      xpack_init_global_prm
 * @brief   xpack全局参数初始化
 */
static VOID xpack_init_global_prm(void)
{
    UINT32 i = 0;
    DSPINITPARA *pDspInitPara = SystemPrm_getDspInitPara();
    XPACK_GLOBAL_PRM *pGlobalPrm = &gGlobalPrm;
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    XIMAGE_DATA_ST *pstSaveImg = NULL;

    pGlobalPrm->xpack_fps = pDspInitPara->stVoInitInfoSt.stVoInfoPrm[0].stDispDevAttr.videoOutputFps;
    pGlobalPrm->u32RefreshCol = 2;

    for (i = 0; i < pstCapbXray->xray_in_chan_cnt; i++)
    {
        pGlobalPrm->astXpackChnPrm[i].enProcStat = XRAY_PS_RTPREVIEW;
        pGlobalPrm->astXpackChnPrm[i].enRtDirection = XRAY_DIRECTION_LEFT;
        pGlobalPrm->astXpackChnPrm[i].u32DispBgColor = (DSP_IMG_DATFMT_YUV420_MASK & pstCapbXsp->enDispFmt) ? XIMG_BG_DEFAULT_YUV : XIMG_BG_DEFAULT_RGB;
        pGlobalPrm->astXpackChnPrm[i].u32DispVertOffset = pstCapbDisp->disp_h_top_blanking;
        pGlobalPrm->astXpackChnPrm[i].enOsdShowMode = XPACK_SHOW_OSD_ALL;
        SAL_mutexTmInit(&pGlobalPrm->astXpackChnPrm[i].mutexOsdDraw, SAL_MUTEX_ERRORCHECK);

        // SAVA业务参数初始化
        pstSaveImg = &pGlobalPrm->astXpackChnPrm[i].stSaveDispBuf;
        if (SAL_SOK != ximg_create(pstCapbDisp->disp_yuv_w_max, pstCapbDisp->disp_yuv_h, pstCapbXsp->enDispFmt, "SAVE-disp", NULL, pstSaveImg))
        {
            XPACK_LOGE("chan %u ximg_create stSaveDispBuf error, w:%u, h:%u, fmt:0x%x!\n", 
                        i, pstCapbDisp->disp_yuv_w_max, pstCapbDisp->disp_yuv_h, pstCapbXsp->enDispFmt);
            return;
        }
    }

    return;
}


/******************************************************************
 * @function:   xpack_set_dump_prm
 * @brief:      数据下载设置
 * @param[in]:  UINT32 u32Chan    通道号
 * @param[in]:  BOOL bEnable      开关
 * @param[in]:  CHAR *pchDumpDir  下载路径
 * @param[in]:  UINT32 u32DumpDp  下载
 * @param[in]:  UINT32 u32DumpCnt 下载个数
 * @return:  void
 *******************************************************************/
SAL_STATUS xpack_set_dump_prm(UINT32 chan, BOOL bEnable, CHAR *pchDumpDir, UINT32 u32DumpDp, UINT32 u32DumpCnt)
{
    UINT32 u32DirStrlen = 0;
    XPACK_CHN_PRM *pstXpackChn = NULL;
    XPACK_GLOBAL_PRM *pGlobalPrm = &gGlobalPrm;

    if (NULL == pchDumpDir || 0 == strlen(pchDumpDir))
    {
        XPACK_LOGE("the pchDumpDir is NULL\n");
        return SAL_FAIL;
    }

    XPACK_CHECK_CHAN(chan, SAL_FAIL);

    pstXpackChn = &pGlobalPrm->astXpackChnPrm[chan];

    /* 存储目录 */
    u32DirStrlen = sizeof(pstXpackChn->stDumpCfg.chDumpDir) - 1;
    strncpy(pstXpackChn->stDumpCfg.chDumpDir, pchDumpDir, u32DirStrlen);
    pstXpackChn->stDumpCfg.chDumpDir[u32DirStrlen] = '\0';

    /* DUMP类型 */
    pstXpackChn->stDumpCfg.unDumpDp.u32XpackDumpType = u32DumpDp;

    /* DUMP次数 */
    if (bEnable)
    {
        pstXpackChn->stDumpCfg.u32DumpCnt = (u32DumpCnt == 0xFFFFFFFF) ? 0xFFFFFFFF : (u32DumpCnt + 1);
    }
    else
    {
        pstXpackChn->stDumpCfg.u32DumpCnt = 0;
    }

    XSP_LOGW("chan %u xpack_set_dump_prm successfully, DumpDir: %s, DumpDp: 0x%x, DumpCnt: %u\n",
             chan, pstXpackChn->stDumpCfg.chDumpDir, pstXpackChn->stDumpCfg.unDumpDp.u32XpackDumpType, pstXpackChn->stDumpCfg.u32DumpCnt);

    return SAL_FAIL;
}


/**
 * @function    xpack_show_dup_time
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void xpack_show_dup_time(UINT32 chan, UINT32 u32PrintType)
{
    CAPB_XRAY_IN *pCapbXray = capb_get_xrayin();
    debug_time pDupDbgTime = gGlobalPrm.astXpackChnPrm[chan].stSendDupPrm[XPACK_DISPDT_OSD].pSendDupDTime;

    if (chan >= pCapbXray->xray_in_chan_cnt)
    {
        SAL_print("the chan[%u] is invalid, < %u", chan, pCapbXray->xray_in_chan_cnt);
        return;
    }

    if (u32PrintType & 0x1)
    {
        dtime_print_time_point(pDupDbgTime, 160);
    }
    if (u32PrintType & 0x2)
    {
        dtime_print_duration(pDupDbgTime, 160);
    }
    if (u32PrintType & 0x4)
    {
        dtime_print_interval(pDupDbgTime, 160);
    }

    return;
}


VOID xpack_show_display_info(UINT32 chan)
{
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XPACK_CHN_PRM *pstXpackChnPrm = NULL;

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return;
    }

    pstXpackChnPrm = &gGlobalPrm.astXpackChnPrm[chan];
    SAL_print("\nXPack %u > ps | rt dir | pb dir | VOffs | FPS | RPF | SyncTime | bOSDShow |\n", chan);
    SAL_print("%12d | %s | %s | %5u | %3u | %3u | %8u | %8d |\n", pstXpackChnPrm->enProcStat, (pstXpackChnPrm->enRtDirection==XRAY_DIRECTION_RIGHT) ? "-> L2R" : "<- R2L", 
              (pstXpackChnPrm->enPbDirection==XRAY_DIRECTION_RIGHT) ? "-> L2R" : "<- R2L",pstXpackChnPrm->u32DispVertOffset, gGlobalPrm.xpack_fps, gGlobalPrm.u32RefreshCol, gGlobalPrm.u32DispSyncTime, pstXpackChnPrm->enOsdShowMode);
    ring_buf_status(gXpackRingBufHandle[chan][XPACK_DISPDT_OSD]);
    return;
}


VOID xpack_show_package_info(UINT32 chan)
{
    UINT32 u32PackageIdx = 0, u32PackageCnt = 0;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    DSA_NODE *pNode = NULL;
    XPACK_PACKAGE_CTRL *pstPkgCtrl = NULL;
    XPACK_PACKAGE_ATTR *pstPkgAttr = NULL;

    if (chan >= pstCapbXray->xray_in_chan_cnt)
    {
        XPACK_LOGE("xpack chan(%u) is invalid, range: [0, %u]\n", chan, pstCapbXray->xray_in_chan_cnt-1);
        return;
    }

    u32PackageIdx = 0;
    pstPkgCtrl = gstRtPackageCtrl + chan;
    SAL_print("\nXPack-Rt %u > idx      id cp jpg cb RawW RawH    GenTime   LocStart     LocEnd  Top Botm AiDgr Num Totl-Send-Recv-Oosd XrIdt Num St TipSt\n", chan);
    SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    u32PackageCnt = DSA_LstGetCount(pstPkgCtrl->lstPackage);
    pNode = DSA_LstGetHead(pstPkgCtrl->lstPackage);
    while (NULL != pNode)
    {
        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
        SAL_print("%13u/%2u %7u %2d %3d %2d %4u %4u %10llu %10llu %10llu %4u %4u %5d %3u %4u %4u %4u %4u %5d %1u|%1u %2d %5d\n", u32PackageIdx++, u32PackageCnt, 
                  pstPkgAttr->u32Id, pstPkgAttr->bCompleted, pstPkgAttr->bJpegEncStart, pstPkgAttr->bStoredCb, pstPkgAttr->u32RawWidth, 
                  pstPkgAttr->u32RawHeight, pstPkgAttr->u64GenerateTime, pstPkgAttr->u64DispLocStart, pstPkgAttr->u64DispLocEnd,
                  pstPkgAttr->stPackDivInfo.uiPackTop, pstPkgAttr->stPackDivInfo.uiPackBottom, 
                  pstPkgAttr->s32AiDgrBufIdx, pstPkgAttr->stAiDgrResult.target_num, pstPkgAttr->u32AiDgrWTotal, 
                  pstPkgAttr->u32AiDgrWSend, pstPkgAttr->u32AiDgrWRecv, pstPkgAttr->u32AiDgrWOsd,
                  pstPkgAttr->s32XrIdtBufIdx, pstPkgAttr->stXspIdtResult.stUnpenSent.uiNum, pstPkgAttr->stXspIdtResult.stZeffSent.uiNum, 
                  pstPkgAttr->enXrIdtStage, pstPkgAttr->enTipStage);
        pNode = pNode->next;
    }
    SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

    u32PackageIdx = 0;
    pstPkgCtrl = gstPbPackageCtrl + chan;
    SAL_print("\nXPack-Pb %u > idx      id cp jpg cb RawW RawH    GenTime   LocStart     LocEnd  Top Botm AiDgr Num Totl-Send-Recv-Oosd XrIdt Num St\n", chan);
    SAL_mutexTmLock(&pstPkgCtrl->lstPackage->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    u32PackageCnt = DSA_LstGetCount(pstPkgCtrl->lstPackage);
    pNode = DSA_LstGetHead(pstPkgCtrl->lstPackage);
    while (NULL != pNode)
    {
        pstPkgAttr = (XPACK_PACKAGE_ATTR *)pNode->pAdData;
        SAL_print("%13u/%2u %7u %2d %3d %2d %4u %4u %10llu %10llu %10llu %4u %4u %5d %3u %4u %4u %4u %4u %5d %1u|%1u %2d\n", u32PackageIdx++, u32PackageCnt, 
                  pstPkgAttr->u32Id, pstPkgAttr->bCompleted, pstPkgAttr->bJpegEncStart, pstPkgAttr->bStoredCb, pstPkgAttr->u32RawWidth, 
                  pstPkgAttr->u32RawHeight, pstPkgAttr->u64GenerateTime, pstPkgAttr->u64DispLocStart, pstPkgAttr->u64DispLocEnd,
                  pstPkgAttr->stPackDivInfo.uiPackTop, pstPkgAttr->stPackDivInfo.uiPackBottom, 
                  pstPkgAttr->s32AiDgrBufIdx, pstPkgAttr->stAiDgrResult.target_num, pstPkgAttr->u32AiDgrWTotal, 
                  pstPkgAttr->u32AiDgrWSend, pstPkgAttr->u32AiDgrWRecv, pstPkgAttr->u32AiDgrWOsd,
                  pstPkgAttr->s32XrIdtBufIdx, pstPkgAttr->stXspIdtResult.stUnpenSent.uiNum, pstPkgAttr->stXspIdtResult.stZeffSent.uiNum, 
                  pstPkgAttr->enXrIdtStage);
        pNode = pNode->next;
    }
    SAL_mutexTmUnlock(&pstPkgCtrl->lstPackage->sync.mid, __FUNCTION__, __LINE__);

    return;
}


/**
 * @fn      Xpack_DrvInit
 * @brief   XPack业务初始化 
 * @note    初始化顺序：①XPack后级（显示、编码等），②XPack，③XPack前级（XSP、XRay） 
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：失败
 */
SAL_STATUS Xpack_DrvInit(VOID)
{
    xpack_init_global_prm();

    if (SAL_SOK != xpack_osd_module_int())
    {
        XPACK_LOGE("xpack_osd_module_int failed!!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != xpack_package_module_init())
    {
        XPACK_LOGE("xpack_package_module_init failed!!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != xpack_init_package_jpg_cb())
    {
        XPACK_LOGE("xpack_init_package_jpg_cb failed!!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != xpack_init_package_segment())
    {
        XPACK_LOGE("xpack_init_package_segment failed!!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != xpack_init_disp_data_buf())
    {
        XPACK_LOGE("xpack_init_disp_data_buf failed!!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != xpack_init_disp_data_send())
    {
        XPACK_LOGE("xpack_init_disp_data_send failed!!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

