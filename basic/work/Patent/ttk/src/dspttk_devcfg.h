
#ifndef _DSPTTK_DEVCFG_H_
#define _DSPTTK_DEVCFG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "sal_type.h"
#include "dspcommon.h"
#include "dspttk_posix_ipc.h"

#define MAX_RANG_TYPE   (32)        /* 射线源最大数组范围 从json中rang中获取*/
#define MAX_STRING      (16)        /* 数组存储的最大字符串数 */

typedef struct
{
    CHAR cfgVersion[16];
    CHAR boardType[16];
} DSPTTK_INIT_COMMON;

typedef struct
{
    UINT32 integralTime;
    UINT32 sliceHeight;
    FLOAT32 beltSpeed;
    UINT32 dispfps;
} DSPTTK_INIT_XRAY_SPEED;
typedef struct
{
    sem_t semSetTip[2];               // TIP插入信号量
    XSP_TIP_DATA_ST stTipData;
} DSPTTK_XSP_TIP_SET;

typedef struct
{
    UINT32 xrayChanCnt;
    CHAR xrayDetVendor[MAX_STRING];         // 探测板型号
    CHAR xraySourceType[MAX_XRAY_CHAN][MAX_STRING]; // 射线源型号，区分主辅视角
    CHAR szXraySrcTypeRang[MAX_XRAY_CHAN][MAX_RANG_TYPE][MAX_STRING]; // 射线源范围 
    CHAR xrayEnergy[MAX_STRING];            // single：单能，dual：双能
    XSP_LIB_SRC enXspLibSrc;        // XSP算法源
    UINT32 xrayWidthMax;            // XRaw宽
    UINT32 xrayHeightMax;           // XRaw高
    UINT32 xrayResW;                // 超分后的宽
    UINT32 xrayResH;                // 超分后的高
    FLOAT32 resizeFactor;           // XSP超分系数
    UINT32 packageLenMax;           // XSP强制包裹分割长度
    DSPTTK_INIT_XRAY_SPEED stSpeed[XRAY_FORM_NUM][XRAY_SPEED_NUM]; // 过包速度与积分时间
} DSPTTK_INIT_XRAY;

typedef struct
{
    BOOL enable;
    UINT32 voChnCnt;
    CHAR voInterface[16];
    UINT32 width;
    UINT32 height;
    UINT32 fpsDefault;
} DSPTTK_INIT_DISPLAY_VODEV_ATTR;

typedef struct
{
    UINT32 voDevCnt;
    UINT32 paddingTop;
    UINT32 paddingBottom;
    UINT32 blankingTop;
    UINT32 blankingBottom;
    UINT32 u32DispWithMax;
    UINT32 u32DispHeightMax;
    DSPTTK_INIT_DISPLAY_VODEV_ATTR stVoDevAttr[2];
} DSPTTK_INIT_DISPLAY;

typedef struct
{
    UINT32 encChanCnt;
    UINT32 encRecPsBufLen[MAX_VENC_CHAN][3];
    UINT32 encNetRtpBufLen[MAX_VENC_CHAN][2];
    UINT32 decChanCnt;
    UINT32 decBufLen[MAX_VDEC_CHAN];
    UINT32 ipcStreamPackTransChanCnt;
    UINT32 ipcStreamPackTransBufLen[MAX_VIPC_CHAN][3];
} DSPTTK_INIT_ENCDEC;

typedef struct
{
    DEVICE_CHIP_TYPE_E enDevChipType; // 单芯片OR双芯片
} DSPTTK_INIT_SVA;

typedef struct
{
    UINT32 aoDevCnt;
} DSPTTK_INIT_AUDIO;

typedef struct
{
    DSPTTK_INIT_COMMON stCommon;
    DSPTTK_INIT_XRAY stXray;
    DSPTTK_INIT_DISPLAY stDisplay;
    DSPTTK_INIT_ENCDEC stEncDec;
    DSPTTK_INIT_SVA stSva;
    DSPTTK_INIT_AUDIO stAudio;
} DSPTTK_INIT_PARAM;

typedef struct
{
    CHAR rtXraw[64];        // 实时过包XRaw数据，递归遍历文件夹
    CHAR tipNraw[64];       // TIP NRaw数据，存放于根文件夹
    CHAR ipcStream[64];     // IPC流，包括RTP和PS流，文件扩展名rtp和ps，存放于根文件夹
    CHAR aiModel[64];       // AI开放平台的模型，存放于根文件夹
    CHAR alramAudio[64];    // 报警音频，存放于根文件夹
    CHAR noSignalImg[64];   // 无视频信号图像文件夹
    CHAR font[64];          // 字体文件夹
    CHAR gui[64];           // gui图片文件夹
} DSPTTK_SETTING_ENV_INPUT;

typedef struct
{
    CHAR sliceNraw[64];     // 回调的条带RAW数据
    CHAR packageImg[64];    // 回调的包裹图像，暂仅有JPG
    CHAR saveScreen[64];    // Save的当前屏幕图像
    CHAR packageSeg[64];    // 回调的包裹分片
    CHAR encStream[64];     // 实时预览（模拟通道）编码，包括RTP和PS流
    CHAR packageTrans[64];  // 包裹转存文件夹
    CHAR streamTrans[64];   // 转封装流
    CHAR xrawStore[64];      // 原始raw数据回调保存
} DSPTTK_SETTING_ENV_OUTPUT;

typedef struct
{
    BOOL bInputDirRtLoop;                   // 实时过包xraw数据文件加是否循环
    BOOL bOutputDirAutoClear;               // 重启后是否自动清除文件夹
    CHAR runtimeDir[64];                    // PC端该程序的路径
    DSPTTK_SETTING_ENV_INPUT stInputDir;    // 输入数据目录
    DSPTTK_SETTING_ENV_OUTPUT stOutputDir;  // 输出数据目录
} DSPTTK_SETTING_ENV;

typedef struct
{
    BOOL bTransTypeEn[XRAY_TRANS_BUTT];                 /* 转存格式，以XRAY_TRANS_TYPE做索引，TRUE-开启 */
    UINT32 u32BkgColorFormat;                           /* 设置显示图像背景色格式，区分通道号，HOST_CMD_XSP_SET_BKGCOLOR 对应结构体XSP_BKG_COLOR*/
    UINT32 u32InputDelayTime;                           /* 主辅视角时间差 */
    CHAR cBkgColorValue[16];                            /* 设置显示图像背景色，区分通道号，HOST_CMD_XSP_SET_BKGCOLOR 对应结构体XSP_BKG_COLOR*/
    XRAY_SPEED_PARAM stSpeedParam;                      /* 实时过包速度参数 */
    XRAY_PROC_DIRECTION enDispDir;                      /* 实时预览出图方向 */
    XSP_ENHANCED_SCAN_PARAM stEnhancedScan;             /* 强扫参数 HOST_CMD_XSP_SET_ENHANCED_SCAN */
    XSP_VERTICAL_MIRROR_PARAM stVMirror[MAX_XRAY_CHAN]; /* 垂直镜像 HOST_CMD_XSP_SET_MIRROR */ 
    XRAY_PARAM stFillinBlank[MAX_XRAY_CHAN];            /* 上下置白 HOST_CMD_XRAY_SET_PARAM*/
    XSP_COLOR_TABLE_PARAM stColorTable;                 /* 颜色表，从0开始，0-颜色表1，1-颜色表2，…… HOST_CMD_XSP_SET_COLOR_TABLE */
    XSP_PSEUDO_COLOR_PARAM stPseudoColor;               /* 伪彩模式 HOST_CMD_XSP_SET_PSEUDO_COLOR */
    XSP_DEFAULT_STYLE stXspStyle;                       /* 默认成像风格 预设模式 HOST_CMD_XSP_SET_DEFAULT */
    XSP_UNPEN_PARAM stXspUnpen;                         /* 设置难穿透  HOST_CMD_XSP_SET_UNPEN*/
    XSP_SUS_ALERT stSusAlert;                           /* 设置难穿透  HOST_CMD_XSP_SET_UNPEN*/
    DSPTTK_XSP_TIP_SET stTipSet;                           /* Tip考核图像信息 HOST_CMD_XSP_SET_TIP */
    XSP_BKG_PARAM stBkg;                                /* 背景阈值 HOST_CMD_XSP_SET_BKG */
    XSP_DEFORMITY_CORRECTION stDeformityCorrection;     /* 畸形矫正 HOST_CMD_XSP_SET_DEFORMITY_CORRECTION */
    XSP_COLOR_ADJUST_PARAM stColorAdjust;               /* 冷暖色调调节 HOST_CMD_XSP_COLOR_ADJUST */
    XSP_ENABLE_A_LEVEL stRmBlankSlice;                  /* 包裹后的空白条带控制结构体  HOST_CMD_XSP_SET_BLANK_SLICE */
    XSP_PACKAGE_DIVIDE_PARAM stPackagePrm;              /* 包裹分割结构体  HOST_CMD_XSP_PACKAGE_DIVIDE */
    XSP_STRETCH_MODE_PARAM stretchParam;                /* 设置拉伸模式，依靠算法内部实现，不区分通道号，HOST_CMD_XSP_SET_STRETCH */
    XSP_GAMMA_PARAM stXspGamma;                         /* 设置gamma参数，不区分通道号，HOST_CMD_XSP_SET_GAMMA */
    XSP_AIXSP_RATE_PARAM stXspAixsprate;                /* 设置AIXSP和传统XSP权重，不区分通道号 HOST_CMD_XSP_SET_AIXSP_RATE*/
    XSP_COLORS_IMAGING_PARAM stColorImage;              /* 设置六色显示 */
    XSP_ORIGINAL_MODE_PARAM stDefaultEnhance;           /* 设置默认增强 */
    XSP_CONTRAST_PARAM stContrast;                      /* 设置对比度 */
} DSPTTK_SETTING_XRAY;

typedef struct
{
    UINT32 u32DispSyncTimeSet;                             /* 主辅视角超时时间 */
    XPACK_DSP_SEGMENT_SET stSegmentSet;
    XPACK_JPG_SET_ST stXpackJpgSet;
    XPACK_YUVSHOW_OFFSET stXpackYuvOffset;
} DSPTTK_SETTING_XPACK;

/* 输入源 */
typedef enum
{
    DSPTTK_INPUT_SOURCE_NULL = 12,                      /* 输入源模式为NULL */
    DSPTTK_INPUT_SOURCE_VI,                             /* 输入源模式为VIDEOIN */
    DSPTTK_INPUT_SOURCE_DECODE,                         /* 输入源模式为解码模式 */
    DSPTTK_INPUT_SOURCE_VPSS,                             /* 输入源模式为VIDEOOUT */
}DSPTTK_INPUT_SOURCE;

/* 输入源参数设置 */
typedef struct
{
    UINT32  u32SrcChn;                                  //输入源通道号
    DSPTTK_INPUT_SOURCE enInSrcMode;                    //输入源模式 
}DSPTTK_INSOURCE_PARAM;

/* FrameBuffer参数配置 */
typedef struct
{
    BOOL bFBEn;                                         /* FB使能 */
    BOOL bMouseEn;                                      /* 鼠标使能 */
    POS_ATTR stMousePos;                                /* 鼠标参数设置 */
    SCREEN_CTRL stScreenCtl;                            /* 命令HOST_CMD_DISP_FB_MMAP对应的结构体参数 */
}DSPTTK_FB_PARAM;

/* Win页面下的窗口填充色和画框颜色 */
typedef struct
{
    CHAR cBorderLineColor[16];                          /* 线框颜色,对应DISP_REGION中的BorDerColor*/
    CHAR cFillWinColor[16];                             /* 窗口填充背景色 对应DISP_REGION中的color */
}DSPTTK_WIN_COLOR_PARAM;

typedef struct dspttk_setting_disp
{
    DSPTTK_FB_PARAM stFBParam[MAX_VODEV_CNT];                           /* FB参数 */
    DISP_POS_CTRL stWinParam[MAX_VODEV_CNT][MAX_DISP_CHAN];             /* 窗口内参数配置 */
    DSPTTK_WIN_COLOR_PARAM stWinColorPrm[MAX_VODEV_CNT][MAX_DISP_CHAN];               /* 窗口内颜色配置 */
    DSPTTK_INSOURCE_PARAM stInSrcParam[MAX_VODEV_CNT][MAX_DISP_CHAN];   /* 输入源参数配置 */
	DISP_GLOBALENLARGE_PRM stGlobalEnlargePrm[MAX_VODEV_CNT][MAX_DISP_CHAN]; /* 全局放大配置参数 */
    DISP_GLOBALENLARGE_PRM stLocalEnlargePrm[MAX_VODEV_CNT][MAX_DISP_CHAN];  /* 局部放大配置参数 */

}DSPTTK_SETTING_DISP;

/* 码流类型 */
typedef enum
{
    DSPTTK_ENC_MAIN_STREAM  = 0,             /* 码流类型为主码流 */
    DSPTTK_ENC_SUB_STREAM  = 1,              /* 码流类型为子码流 */
    DSPTTK_ENC_STREAMTYPE_NUM                /* 码流个数 (目前支持主码流|子码流两种) */
}DSPTTK_ENC_STREAMTYPE;


/* 编码参数配置*/
typedef struct dspttk_setting_multi_enc
{
    BOOL bStartEn;               /* 编码功能使能 */
    BOOL bSaveEn;                /* 编码保存功能使能 */
    ENCODER_PARAM stEncParam;    /* 编码属性  HOST_CMD_SET_ENCODER_PARAM */
}DSPTTK_ENC_MULTI_PARAM;;

/* 编码参数配置 */
typedef struct dspttk_setting_enc
{
    DSPTTK_ENC_MULTI_PARAM stEncMultiParam[DSPTTK_ENC_STREAMTYPE_NUM];  /* 编码参数配置 共支持两种类型码流(主码流|子码流)*/
}DSPTTK_SETTING_ENC;

/* Dec码流格式 */
typedef enum
{
    DSPTTK_DEC_PACKET_PS  = 0,                              /* 解码码流格式为PS */
    DSPTTK_DEC_PACKET_RTP_VIDEOTYPE_H264 = 1,               /* 解码码流格式为RTP, 视频格式为H264 */
    DSPTTK_DEC_PACKET_RTP_VIDEOTYPE_H265 = 2               /* 解码码流格式为RTP, 视频格式为H265 */
}DSPTTK_DEC_STREAMTYPE;

/* Dec重复解码配置 */
typedef enum
{
    DSPTTK_DEC_REPEAT_NULL = 0,         /* 不重复解码(或转包) */
    DSPTTK_DEC_REPEAT_FILE = 1,         /* 重复解码(或转包)当前文件 */
    DSPTTK_DEC_REPEAT_DIR  = 2          /* 重复解码(或转包)当前文件夹 */
}DSPTTK_DEC_REPEAT_MODE;

/* Dec参数设置 */
typedef struct 
{
    BOOL bDecode;                            /* 解码功能是否使能 */
    BOOL bRecode;                            /* 转封装是否使能 */
    DSPTTK_DEC_STREAMTYPE enMixStreamType;   /* 码流格式(PS|RTP-H264|RTP-H265) */
    DECODER_PARAM stDecPrm;                  /* 设置封装格式相关参数 与HOST_CMD_DEC_START参数相关 */
    DECODER_ATTR stDecAttrPrm;               /* 设置速率和模式 */
    DECODER_SYNC_PRM stDecSyncPrm;           /* 设置同步参数 */
    RECODE_PRM stRecodePrm;                  /* 设置转封装参数 */
    DECODEC_COVER_PICPRM stCoverPrm;         /* 设置封面相关参数 */
    DSPTTK_DEC_REPEAT_MODE enRepeat;         /* 设置重放模式 */
    UINT32 u32JumpPercent;                   /* 设置跳转百分比 */

}DSPTTK_DEC_MULTI_PARAM;;

/* Dec参数设置 */
typedef struct 
{
    DSPTTK_DEC_MULTI_PARAM stDecMultiParam[MAX_VDEC_CHAN];  /* Dec参数设置 共支持MAX_VDEC_CHAN(16)路通道设置 */
}DSPTTK_SETTING_DEC;

/* Sva参数设置 */
typedef struct 
{
    BOOL enSvaInit;                    /* sva栏中的危险品识别开关 */
    BOOL enSvaPackConse;               /* sva栏中包裹连续发送 用于测试引擎在包裹高峰期的识别问题 */
} DSPTTK_SETTING_SVA;

/* osd参数设置 */
typedef struct 
{
    BOOL enOsdAllDisp;                    /* OSD栏中的显示开关 */
    BOOL enOsdConfDisp;                   /* OSD栏中的置信度开关 */
    CHAR cDispColor[16];                  /* OSD栏中的颜色选项 */
    SVA_BORDER_TYPE_E enBorderType;       /* OSD栏中的线型选择 */
    UINT32 u32ScaleLevel;                 /* OSD栏中字体 */
    DISPLINE_PRM stDispLinePrm;           /* OSD栏中设置同名危险品合并 DSP_DISP_DRAW_AI_TYPE 中选择 AI_DISP_DRAW_ONE_TYPE */
} DSPTTK_SETTING_OSD;

typedef struct
{
    DSPTTK_SETTING_ENV stEnv;           /* setting栏中Env参数设置 */
    DSPTTK_SETTING_XRAY stXray;         /* setting栏中XRay参数设置 */
    DSPTTK_SETTING_XPACK stXpack;       /* setting栏中Xpack参数设置 */
    DSPTTK_SETTING_DISP stDisp;         /* setting栏中Display参数设置 */
    DSPTTK_SETTING_SVA stSva;        /* setting栏中SVA参数设置 */
    DSPTTK_SETTING_OSD stOsd;           /* setting栏中OSD参数设置 */
    DSPTTK_SETTING_ENC stEnc;           /* setting栏中ENCODE参数设置 */
    DSPTTK_SETTING_DEC stDec;           /* setting栏中DECODE参数设置 */
} DSPTTK_SETTING_PARAM;


typedef struct
{
    DSPTTK_INIT_PARAM stInitParam;          // 初始化参数
    DSPTTK_SETTING_PARAM stSettingParam;    // 设置参数
} DSPTTK_DEVCFG;

typedef enum
{
    DSPTTK_RUNSTAT_READY    = 0,    // 准备就绪，可以发起启动DSP组件
    DSPTTK_RUNSTAT_INITING  = 1,    // DSP组件正在启动中
    DSPTTK_RUNSTAT_RUNNING  = 2,    // DSP组件正在运行中
} DSPTTK_RUN_STATUS_E;

typedef enum
{
    DSPTTK_STATUS_NONE        = 0, // 当前无任何处理
    DSPTTK_STATUS_RTPREVIEW   = 1, // 实时预览
    DSPTTK_STATUS_RTPREVIEW_STOP   = 2, // 实时预览暂停
    DSPTTK_STATUS_PLAYBACK    = 3  // 回拉
} DSPTTK_XRAY_PROC_STATUS_E;

typedef enum
{
    DSPTTK_STATUS_PB_SLICE    = 0,  // 条带回拉
    DSPTTK_STATUS_PB_PACKAGE  = 1,  // 整包回拉
    DSPTTK_STATUS_PB_SAVE  = 2      // SAVE图片
} DSPTTK_XRAY_PB_STATUS_E;

typedef struct
{
    UINT64 u64SliceNo;              // 当前屏幕上最新一个条带的条带号
    UINT32 u32SliceHeight;          // 当前屏幕上最新一个条带的高度
    XRAY_PROC_DIRECTION enDir;      // 当前屏幕上最新一个条带的显示方向
    UINT64 u64SliceNoL2R;           // 采传下发的最后一个从左向右（L2R）条带的条带号
    UINT64 u64SliceNoR2L;           // 采传下发的最后一个从右向左（R2L）条带的条带号
} DSPTTK_XRAY_RT_SLICE_STATUS;

typedef struct
{
    UINT64 u64CurPbPackageId;               //当前需要回拉的包裹id
    XRAY_PROC_DIRECTION enDir;
    SVA_DSP_OUT stSvaInfo;
    XSP_PACK_INFO stXspInfo;
    UINT8 *pu8PbSpliceDataSrc;             //整包回拉源数据
    UINT8 *pu8PbSpliceDataDst;             //整包回拉拼接后的数据
} DSPTTK_XRAY_PB_PACKAGE;

typedef struct
{
    UINT32 u32SaveNo;
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Size;
    CHAR cNrawName[128];
    UINT8 *pu8SaveDataSrc;                 //save源数据
} DSPTTK_XRAY_PB_SAVE;

typedef struct
{
    DSPTTK_XRAY_PB_STATUS_E enPbStatus;     // 回拉模式选择 包括整包回拉和条带回拉
    DSPTTK_XRAY_PB_PACKAGE stPbPack;
    DSPTTK_XRAY_PB_SAVE stPbSave;
} DSPTTK_XRAY_PB_INFO;

typedef struct
{
    pthread_mutex_t mutexStatus;            // 状态的互斥锁
    DSPTTK_RUN_STATUS_E enRunStatus;        // DSP组件的运行状态
    DSPTTK_XRAY_PROC_STATUS_E enXrayPs;     // XRay运行状态
    DSPTTK_XRAY_RT_SLICE_STATUS stRtSliceStatus[MAX_XRAY_CHAN]; // 当前屏幕上最新一个条带的状态
    DSPTTK_XRAY_PB_INFO stPbInfo;        // 回拉的信息
} DSPTTK_GLOBAL_STATUS;


/**
 * @fn      dspttk_devcfg_get
 * @brief   获取DSP TTK的配置文件
 * 
 * @return  DSPTTK_DEVCFG* 配置文件Buffer地址
 */
DSPTTK_DEVCFG *dspttk_devcfg_get(void);

/**
 * @fn      dspttk_devcfg_load
 * @brief   加载DSP TTK的配置文件
 *
 * @param   [IN] pJsonFile Json类型默认参数配置文件
 *
 * @return  SAL_STATUS SAL_SOK：加载成功，SAL_FAIL：创建失败
 */
SAL_STATUS dspttk_devcfg_load(CHAR *pJsonFile);

/**
 * @fn      dspttk_devcfg_save
 * @brief   保存DSP TTK的配置文件
 * @warning 修改配置文件中参数与调用该接口前，均需要加锁gMutexDevcfg
 *  
 * @return  SAL_STATUS SAL_SOK：保存成功，SAL_FAIL：失败
 */
SAL_STATUS dspttk_devcfg_save(void);

/**
 * @fn      dspttk_devcfg_init
 * @brief   从指定路径读取DSP TTK的配置文件，若不存在则创建
 * 
 * @return  SAL_STATUS SAL_SOK：保存成功，SAL_FAIL：失败
 */
SAL_STATUS dspttk_devcfg_init(void);

extern pthread_mutex_t gMutexDevcfg; // 读写DSP TTK的配置参数的互斥锁
extern DSPTTK_GLOBAL_STATUS gstGlobalStatus;

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_DEVCFG_H_ */

