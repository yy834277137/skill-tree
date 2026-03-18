/**
 * @file   dspcommon.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  DSP模块对外所提供的头文件
 * @author wangweiqin <wangweiqin@hikvision.com>
 * @date   2017/3/1
 * @note :
 * @note \n History:
   1.Date        : 2019/10/17
     Author      : liwenbin
     Modification: 自研安检机增加接口
 */

#ifndef _DSPCOMMON_H_
#define _DSPCOMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif


/* ========================================================================== */
/*                             系统宏定义                                     */
/* ========================================================================== */

/* 本接口头文件的版本号，每次更新结构的时候需要更新该宏的值 */
#define DSP_COMMON_MAIN_VERSION (8)
#define DSP_COMMON_SUB_VERSION	(3)

/* 目前最多支持使用两个无视频图像 */
#define MAX_NO_SIGNAL_PIC_CNT (8)

/* 系统中支持的最大的通道数 */
#define MAX_VODEV_CNT 		(3)         /* 显示通道数 */
#define MAX_CAPT_CHAN	    (2)         /* 采集通道数 */
#define MAX_VENC_CHAN	    (2)         /* 编码通道数 */

#ifdef DOUBLE_CHIP_SLAVE_CHIP
#define MAX_VDEC_CHAN (4)               /* 解码通道数 */
#else
#define MAX_VDEC_CHAN (16)              /* 解码通道数 */
#endif

#define MAX_VIPC_CHAN	(16)            /* 转包通道数 */
#define MAX_DISP_CHAN	(16)            /* 显示通道(VO Chn)数 */
#define MAX_XRAY_CHAN	(2)             /* X ray通道数 */

/* 最大的保留*/
#define MAX_RESERVED (128)              /* DSPINITPARA保留内存大小，以4字节为最小单位 */

/* 长时间采集不到视频，重启DSP */
#define ERR_NO_VIDEO_INPUT 0x40000001

/* 人脸识别相关，从片支持 */
#define FACE_FEATURE_LENGTH			(272)   /* 人脸特征数据长度 */
#define MAX_FACE_NUM				(53)    /* 当前安检机最大支持存放53个注册登录人脸 */
#define MAX_CAP_FACE_NUM            (3)     /*一帧数据支持最多抓拍人脸数量*/
#define MAX_FACE_CAP_JPG_NUM        (2)     /*抓拍人脸返回的jpg数据，小图抠图 和大图背景图*/
#define FACE_MAX_VERESION_LENGTH	(256)   /* 人脸库版本信息最大长度 */

/* 智能分析回调返回的最大图片数量 */
#define SVA_MAX_CALLBACK_JPEG_NUM	(3)
#define SVA_MAX_CALLBACK_BMP_NUM	(1)

/* ========================================================================== */
/*                             回调定义                                       */
/* ========================================================================== */

/*
    码流信息魔术数定义
 */
#define STREAM_ELEMENT_MAGIC 0xdb1bd987

/*
    回调命令字定义 HOST_CMD_NON 从0x80000000 起步，回调命令避开这段值即可，定从0x80000000
 */
typedef enum _STREAM_TYPE_
{
    STREAM_ELEMENT_JPEG_IMG = 0x90020033,          /* 抓图数据JPEG */
    STREAM_ELEMENT_QRCODE = 0x90020034,            /* 二维码处理结果*/
    STREAM_ELEMENT_MD_RESULT = 0x90020035,         /* 移动侦测结果 */
	STREAM_ELEMENT_FACE_CAP = 0x90020036,		   /*人脸抓拍回调*/
    STREAM_ELEMENT_FR_FFD_IQA = 0x9007004e,        /* 人脸检测及人脸质量评分 */
    STREAM_ELEMENT_FR_BM = 0x9007004f,             /* 人脸建模 */

    STREAM_ELEMENT_FR_CP = 0x90070050,             /* 人脸对比 */
    STREAM_ELEMENT_FR_DEC = 0x90070051,            /* 活体检测 */
    STREAM_ELEMENT_FR_BM_CAP = 0x90070052,         /* 视频抓拍人脸建模 */
    STREAM_ELEMENT_FR_FD_JPG = 0x90070053,         /* JPG人脸检测坐标*/
    STREAM_ELEMENT_FR_PRE_CAP = 0x90070054,        /* JPG预抓功能当前帧前后各2帧*/
    STREAM_ELEMENT_FR_BMP2JPG = 0x90070055,        /* BMP数据转换成JPG数据*/
    STREAM_ELEMENT_FR_JPGSCALE = 0x90070056,       /* 分辨率过大的JPG转化成靠近640*480分辨率的JPG*/
    STREAM_ELEMENT_FR_ENG_ERR = 0x90070057,        /* 引擎初始化失败*/
    STREAM_ELEMENT_FR_JPGOVEROSD = 0x90070058,     /* JPG叠加OSD*/
    STREAM_ELEMENT_VI_VIDEOSTAND = 0x90070059,     /* VI 输入视频制式和vga/hdmi类型*/

    STREAM_ELEMENT_SVA_IMG = 0x90080001,           /* 抓图数据JPEG */
    STREAM_ELEMENT_DEC_BMP = 0x90080002,           /* 解码成bmp */
    STREAM_ELEMENT_SVA_BA = 0x90080003,            /* 行为分析 */
    STREAM_ELEMENT_SAVE_BMP = 0x90080004,          /* 保存为bmp */
    STREAM_ELEMENT_MARK_XRAY = 0x90080006,         /* 保存当前屏幕原始数据，对用户不可见 */
    STREAM_ELEMENT_XPACK_XRAY = 0x90080007,        /* 保存整个包裹原始数据，已不再使用 */
    STREAM_ELEMENT_TRANSFER_XRAY = 0x90080008,     /* xray 转存数据，已不再使用 */
    STREAM_ELEMENT_DIAGNOSE_XRAY = 0x90080009,     /* xray 诊断数据*/
    STREAM_ELEMENT_DEADPIXEL_XRAY = 0x9008000a,    /* xray 坏点结果*/
    STREAM_ELEMENT_CALIBRATION = 0x9008000b,       /* xsp 离线标定(曲线拟合)，暂不使用 */
    STREAM_ELEMENT_TTS_HANDLE = 0x9008000c,        /* tts语音合成*/
    STREAM_ELEMENT_TIP = 0x9008000d,               /* xsp TIP功能*/
    STREAM_ELEMENT_XRAY_RAW = 0x9008000e,          /* xray源raw数据 */
    STREAM_ELEMENT_PPM_IMG = 0x9008000f,           /* 人包关联回调数据 */
    STREAM_ELEMENT_SLICE_NRAW = 0x90080010,        /* 条带归一化RAW数据，回调结构体XSP_SLICE_NRAW */
    STREAM_ELEMENT_PACKAGE_RESULT = 0x90080011,    /* 包裹信息，回调结构体XPACK_RESULT_ST */
    STREAM_ELEMENT_SEGMENT_XPACK = 0x90080012,     /* 安检机集中判图回调信息，回调结构体XPACK_DSP_SEGMENT_OUT */
	STREAM_ELEMENT_SVA_POS = 0x90080013,		   /* sva模块输出的pos信息 */
	STREAM_ELEMENT_SVA_FALSE_DET_PKG = 0x90080014, /* 包裹误检信息回调，当前仅包含包裹id */

    STREAM_ELEMENT_INVALID_BUTT                    /* 无效数据类型*/
} STREAM_TYPE_E;

typedef struct
{
    void *pVirAddr;                     /* 虚拟地址 */
    unsigned long long ullPhyAddr;      /* 物理地址，预留 */
    unsigned int uiSize;                /* 缓存大小 */
} MEMORY_BUFF_S;

/*
    日期、时间
 */
typedef struct
{
    short year;             /* 年 */
    short month;            /* 月 */
    short dayOfWeek;        /* 0:星期日 -6:星期六 */
    short day;              /* 日 */
    short hour;             /* 时 */
    short minute;           /* 分 */
    short second;           /* 秒 */
    short milliSecond;      /* 毫秒 */
} DATE_TIME;

/*
    矩形坐标
 */
typedef struct tagRext
{
    unsigned int uiX;              /*起始X坐标*/
    unsigned int uiY;              /*起始Y坐标*/
    unsigned int uiWidth;          /*宽度*/
    unsigned int uiHeight;         /*高度*/
} RECT;


/*
    回调处理的附加信息
 */
typedef struct _STREAM_ELEMENT_
{
    unsigned int magic;              /* 常数，供定位 */
    unsigned int id;                 /* 数据信息类型 */
    unsigned int chan;               /* 通道号 */
    unsigned int type;               /* 码流类型,STREAM_TYPE_E */
    unsigned int standardTime;       /* TS/PS标准时间,单位为90kHz时钟,取其高32位,即45kHz时钟 */
    DATE_TIME absTime;               /* 绝对时间 */
    unsigned int dataLen;            /* 除该信息外的帧的长度 */
    unsigned int res2[2];            /* 保留 */
} STREAM_ELEMENT;

/* DSP图像数据格式 */
typedef enum
{
    /* 无效的数据格式，可用于初始化数据格式变量 */
    DSP_IMG_DATFMT_UNKNOWN,

    /*==================== YUV420格式 ====================*/
    DSP_IMG_DATFMT_YUV420_MASK          = 0x100,
    /* NV12，YUV420半平面格式，Y在一个平面，UV在另一平面（交错存储，U在前，V在后），Y、U、V各8bit */
    DSP_IMG_DATFMT_YUV420SP_UV,
    /* NV21，YUV420半平面格式，Y在一个平面，VU在另一平面（交错存储，V在前，U在后），Y、U、V各8bit */
    DSP_IMG_DATFMT_YUV420SP_VU,
    /* iYUV，YUV420平面格式，Y、U、V分别在一个平面，Y在前，U在中，V在后，Y、U、V各8bit */
    DSP_IMG_DATFMT_YUV420P,

    /*==================== YUV422格式 ====================*/
    DSP_IMG_DATFMT_YUV422_MASK          = 0x200,

    /*====================   RGB格式  ====================*/
    DSP_IMG_DATFMT_RGB_MASK             = 0x400,
    /* ARGB交叉格式，单平面，B在低地址，GR在中，A在高地址，即BBBBBGGGGGRRRRRA，A为1bit，RGB各5bit */
    DSP_IMG_DATFMT_BGRA16_1555,
    /* RGB平面格式，BGR分别在一个平面，B在前，G在中，R在后，R、G、B各8bit */
    DSP_IMG_DATFMT_BGRP,
    /* RGB平面格式，BGRA分别在一个平面，R、G、B、A各8bit */
    DSP_IMG_DATFMT_BGRAP,
    /* RGB交叉格式，单平面，B在低地址，G在中，R在高地址，即BGRBGRBGR...，1个像素24bit，R、G、B各8bit */
    DSP_IMG_DATFMT_BGR24,
    /* ARGB交叉格式，单平面，B在低地址，GR在中，A在高地址，即BGRABGRABGRA...，1个像素32bit，R、G、B、A各8bit */
    DSP_IMG_DATFMT_BGRA32,

    /*====================  XRay格式  ====================*/
    DSP_IMG_DATFMT_XRAY_MASK            = 0x800,
    /* XRay单能格式，单平面，1个像素16bit */
    DSP_IMG_DATFMT_SP16,
    /* XRay单能格式，单平面，1个像素8bit */
    DSP_IMG_DATFMT_SP8,
    /* XRay多能平面格式，单平面，低能L在前，高能H在后，L、H各16bit */
    DSP_IMG_DATFMT_LHP,
    /* XRay多能平面格式，单平面，低能L在前，高能H在中，原子序数Z在后，L、H各16bit，Z为8bit */
    DSP_IMG_DATFMT_LHZP,
    /* XRay多能平面格式，单平面，低能L在前，高能H在中，原子序数Z在后，L、H各16bit，Z为16bit */
    DSP_IMG_DATFMT_LHZ16P,

} DSP_IMG_DATFMT;

/*
    回调函数类型定义
 */
typedef
void (*DATACALLBACK)
(
    STREAM_ELEMENT *pEle,           /* 回调的附加信息 */
    unsigned char *buf,             /* 附加信息中的 type 所对应的结构体的地址 */
    unsigned int bufLen             /* 附加信息中的 type 所对应的结构体的大小 大小不对，结构体错误 */
);

/*
    下发数据给DSP，立即处理返回，不通过回调的共享内存空间
 */
typedef struct _PROC_SHARE_DATA_
{
    unsigned char *appAddr;          /*app 访问地址*/
    unsigned char *dspAddr;          /*dsp 访问地址*/
} PROC_SHARE_DATA;


/* ========================================================================== */
/*                             命令交互参数定义                               */
/* ========================================================================== */


/****************************************************************************/
/*                                  码流相关参数                            */
/****************************************************************************/

/* 语音对讲*/
#define TALKBACK_FRM_LEN	(160)          /* 对讲音频每帧长度 */
#define TALKBACK_FRM_CNT	(128)          /* 对讲音频缓冲帧个数 */

/* 对讲音频缓冲长度 */
#define TALKBACK_BUF_LEN (TALKBACK_FRM_LEN * TALKBACK_FRM_CNT)


/*
    码流封包类型定义
 */
typedef enum
{
    PS_STREAM = 0x1,                /* PS格式封包  */
    TS_STREAM = 0x2,                /* TS格式封包  */
    RTP_STREAM = 0x4,               /* RTP格式封包 */
    HIK_STREAM = 0x8,               /* HIK格式封包 */
    ORG_STREAM = 0x10,              /* ORG格式封包 */
    MEGA_RTP_STREAM = 0x20,         /* MEGA RTP格式封包 */
    MEGA_PS_STREAM = 0x40,          /* MEGA PS格式封包  */
    SP7_PS_STREAM = 0x80,           /* 萤石ps格式封包 */
} STREAM_PACKET_TYPE;

/*
    码流类型
 */
typedef enum
{
    STREAM_VIDEO = 0x1001,   /* 视频流 */
    STREAM_AUDIO = 0x1002,   /* 音频流 */
    STREAM_MULTI = 0x1003,   /* 复合流 */
    STREAM_PRIVT = 0x1004,   /* 私有流 */
} STREAM_TYPE;


/*
    视频编码类型
 */
typedef enum
{
    H264 = 0,               /* H264编码  */
    MPEG4 = 1,              /* MPEG4编码 */
    MPEG2 = 2,              /* MPEG2编码 */
    MJPEG = 3,              /* MJPEG编码 */
    AVS = 4,                /* AVS  编码 */
    S264 = 5,               /* S264 编码 */
    H265 = 6,               /* S265 编码 */
    RAW_VIDEO = 99          /* 裸流无编码 */
} VIDEO_ENC_TYPE;


/*
    音频编码类型 音频压缩算法
 */
typedef enum
{
    G711_MU = 0,            /* G711的U律编码 */
    G711_A = 1,             /* G711的A律编码 */
    G722_1 = 2,             /* G722     编码 */
    G723 = 3,               /* G723     编码 */
    MP1L2 = 4,              /* MP1      编码 */
    MP2L2 = 5,              /* MP2      编码 */
    G726 = 6,               /* G726     编码 */
    AAC = 7,                /* AAC      编码 */
    RAW = 99                /* 裸流   无编码 */
} AUDIO_ENC_TYPE;

/*
    智能框画线类型
 */
typedef enum
{
    DISP_FRAME_TYPE_FULLLINE = 0,          /* 实线*/
    DISP_FRAME_TYPE_DOTTEDLINE = 1,        /* 虚线*/
    DISP_FRAME_TYPE_DASHDOTLINE = 2,       /* 点画线*/
    DISP_FRAME_TYPE_MAX,                   /* 预留*/
} DISP_FRAME_TYPE;

/*
    智能显示开关参数
 */
typedef struct tagDispSvaSwitch
{
    unsigned int uidevchn;       /*源 通道号*/
    unsigned int dispSvaSwitch;  /*智能显示总开关*/
    unsigned int chn;            /*通道号设置*/
    unsigned int res[20];     /*预留，后期可以单独配置某一个危险品的显示开关*/
} DISP_SVA_SWITCH;


/*
    视频相关信息
 */
typedef struct tagVideoParamsSt
{
    VIDEO_ENC_TYPE videoEncType;          /* 主码流编码格式 */
    VIDEO_ENC_TYPE videoSubEncType;       /* 子码流编码格式 */
    VIDEO_ENC_TYPE videoThirdEncType;     /* third码流编码格式 */
    STREAM_PACKET_TYPE packetType;        /* 主码流封装格式 */
    STREAM_PACKET_TYPE subPacketType;     /* 子码流封装格式 */
    STREAM_PACKET_TYPE ThirdPacketType;   /* third码流封装格式 */
    unsigned int bWaterMark;              /* 水印 */
} VIDEO_PARAM;

/*
    音频编码相关信息
 */
typedef struct tagAudioParamsSt
{
    AUDIO_ENC_TYPE encType;                       /* 音频压缩算法 */
    unsigned int samplesPerSecond;                /* 采样率 */
    unsigned int samplesPerFrame;                 /* 每帧样点数 */
    unsigned int boardSamplesPerSecond;           /* 板子原始音频采样率 */
    unsigned int encChans;                        /* 算法通道数，通常为单声道 */
    unsigned int bitRate;                         /* 输出码率 */
} AUDIO_PARAM;

/*
    语音对讲相关信息
 */
typedef struct tagTalkBackParamsSt
{
    AUDIO_ENC_TYPE talkBackType;                  /* 语音对讲压缩算法 */
    unsigned int talkBackSamplesPerSecond;        /* 语音对讲采样率 */
    unsigned int talkBackSamplesPerFrame;         /* 语音对讲每帧样点数 */
    unsigned int boardOutSamplesPerSecond;        /* 板子原始输出音频采样率 */
    unsigned int talkBackBitRate;                 /* 语音对讲输出码率 */
    unsigned int talkBackChans;                   /* 语音对讲算法通道数，通常为单声道 */
    unsigned int boardOutChans;                   /* 板子输出通道数，davinci为立体声 */
    unsigned int audOutVolume;
} TALK_BACK_PARAM;

/*
    音频与视频码流编码与打包相关信息
 */
typedef struct tagStreamInfoSt
{
    VIDEO_PARAM stVideoParam;                     /* 视频编码属性 */
    AUDIO_PARAM stAudioParam;                     /* 音频编码属性 */
    TALK_BACK_PARAM stTalkBackParam;              /* 语音对讲属性 */
} STREAM_INFO_ST;

/*
    语音对讲功能 数据缓冲区信息
 */
typedef struct tagAudioTBBufInfoSt
{
    volatile unsigned int totalFrame;             /* 缓冲区支持的最大音频帧数 */
    volatile unsigned int frameLen;               /* 音频的每一帧数据的长度   */
    volatile unsigned int hostWriteIdx;           /* 应用写数据的帧数的计数   */
    volatile unsigned int dspWriteIdx;            /* DSP写数据的帧数的计数    */
    volatile unsigned int hostReadIdx;            /* 应用读数据的帧数的计数   */
    volatile unsigned int dspReadIdx;             /* DSP读数据的帧数的计数    */
    unsigned char audTalkbackHost[TALKBACK_BUF_LEN];  /* 应用写入音频数据的缓冲区 */
    unsigned char audTalkbackDsp[TALKBACK_BUF_LEN];   /* DSP 写入音频数据的缓冲区 */
} AUDIO_TB_BUF_INFO_ST;

/*
   语音播放信息
 */
typedef struct tagAudioPlayBackPrm
{
    unsigned int bAudIsPlaying;                  /*当前是否正在播放音频文件：1 是 / 0 不是*/
} AUDIO_PLAY_BACK_PRM;

/*
    水印功能所需要的信息，由应用下发
 */
typedef struct tagWaterMaskInfoSt
{
    unsigned char macAddr[6];  /* MAC地址 */
    unsigned char device_type; /* 型号 */
    unsigned char device_info; /* 附加信息 */
} WATER_MASK_INFO_ST;


/****************************************************************************/
/*                                  图像预处理参数                          */
/****************************************************************************/

/*
    Logo功能缓冲区信息,由应用下发
 */
typedef struct tagLogoBufInfoSt
{
    unsigned char *phyAddr[MAX_VENC_CHAN + MAX_VDEC_CHAN + MAX_DISP_CHAN + MAX_VDEC_CHAN];
    unsigned char *virAddr[MAX_VENC_CHAN + MAX_VDEC_CHAN + MAX_DISP_CHAN + MAX_VDEC_CHAN];
    unsigned int bufLen[MAX_VENC_CHAN + MAX_VDEC_CHAN + MAX_DISP_CHAN + MAX_VDEC_CHAN];
} LOGO_BUF_INFO_ST;


/*
    无视频图像的基本信息，图像格式由应用与底层约定，不定义
 */
typedef struct tagCaptNoSignalInfoSt
{
    unsigned int uiNoSignalImgW[MAX_NO_SIGNAL_PIC_CNT];     /* 无视频图像的宽 */
    unsigned int uiNoSignalImgH[MAX_NO_SIGNAL_PIC_CNT];     /* 无视频图像的高 */
    unsigned int uiNoSignalImgSize[MAX_NO_SIGNAL_PIC_CNT];  /* 无视频图像的数据大小 */
    void *uiNoSignalImgAddr[MAX_NO_SIGNAL_PIC_CNT];  /* 无视频图像的数据起始地址，需要应用分配空间 */
} CAPT_NOSIGNAL_INFO_ST;


/****************************************************************************/
/*                                  码流上传相关参数                        */
/****************************************************************************/

/* 定义分通道上传，录像存储缓冲区I帧信息存储最大数目 */
#define REC_IFRM_INFO_NUM (200)


/*
    编码网络流的RTP头相关信息
 */
typedef struct
{
    unsigned int payload_type;  /* rtp头payload_type    */
    unsigned int seq_num;       /* rtp头seq_num    */
    unsigned int time_stamp;    /* rtp头time_stamp    */
} RTP_INFO;

/*
    录像码流共享缓冲结构中I帧的具体信息
 */
typedef struct
{
    volatile unsigned int uiAddr;        /* 该帧码流的起始地址 */
    volatile unsigned int stdTime;       /* 标准时间 */
    DATE_TIME dspAbsTime;                /* 绝对时间 */
    volatile unsigned int len;           /* 码流长度 */
} STREAM_IFRAME_INFO;

/*
    录像码流共享缓冲结构中I帧信息
 */
typedef struct
{
    STREAM_IFRAME_INFO ifInfo[REC_IFRM_INFO_NUM];     /* 记录下的每一个I帧信息 */
    volatile unsigned int rIdx;                       /* I帧信息读索引 */
    volatile unsigned int wIdx;                       /* I帧信息写索引 */
} IFRAME_INFO_ARRAY;

/*
    编码网传码流共享缓冲结构
 */
typedef struct
{
    unsigned char *vAddr;                 /* 内存映射出的虚拟地址 */
    unsigned char *pAddr;                 /* 物理地址 */
    volatile unsigned int wIdx;           /* 实时码流的DSP写索引 */
    volatile unsigned int delayIdx;       /* 延时预览的DSP写指针*/
    volatile unsigned int totalWLen;      /* 历史数据总长，超过 0xff000000 溢出的临界值进行重新统计 */
    volatile unsigned int totalLen;       /* 内存总长 */
    volatile unsigned int vFrmCounter;    /* 帧计数 */
    volatile unsigned int streamType;     /* 码流类型，视频流、复合流等*/

    /* 下面的成员从NVR-DSP代码中添加移植过来 */
    volatile unsigned int controlLevel;   /* 码流上传控制级别 使用UPLOAD_CONTROL_LEVEL的定义 */
    volatile unsigned int muxType;        /* 封装类型*/
    volatile unsigned int extParam;       /* 扩展参数，用来扩展一些定制功能*/
    volatile RTP_INFO videoInfo;          /* 存储视频每帧第一个rtp包中的关键信息*/
    volatile RTP_INFO audioInfo;          /* 存储音频每帧第一个rtp包中的关键信息*/
    volatile RTP_INFO privtInfo;          /* 存储私有帧每帧第一个rtp包中的关键信息*/
} NET_POOL_INFO;

/*
    编码录像码流共享缓冲结构
 */
typedef struct
{
    unsigned char *vAddr;                   /* 内存映射出的虚拟地址 */
    unsigned char *pAddr;                   /* 物理地址 */
    volatile unsigned int wOffset;          /* 实时码流的DSP写偏移值  */
    volatile unsigned int rOffset;          /* 实时码流的应用读偏移值 */
    volatile unsigned int totalLen;         /* 缓冲区总大小 */
    IFRAME_INFO_ARRAY ifInfo;               /* I帧信息      */
    volatile unsigned int wErrTime;         /* 写内存错误计数 */
    volatile unsigned int lastFrameStdTime;  /* 最后一帧数据的标准时间 */
    DATE_TIME lastFrameAbsTime;              /* 最后一帧数据的绝对时间 */
    volatile unsigned int vFrmCounter;       /* 帧计数 */
    volatile unsigned int streamType;      /* 码流类型，视频流、复合流等 */
    volatile unsigned int controlLevel;    /* 码流上传控制级别 使用UPLOAD_CONTROL_LEVEL的定义 */

    volatile unsigned int muxType;        /* 封装类型 */
    volatile unsigned int bSubRec;        /* 是否是码流录像bSubRec =1:子码流录像；bSubRec =0:主码流录像 */
    volatile unsigned int extParam;       /* 扩展参数，用来扩展一些定制功能 */
    volatile unsigned int streamBps;      /* 记录该通道的码率统计信息 */
    volatile unsigned int curAddrId;      /* 记录当前录像使用的是哪个地址，仅IPC通道有效 */
} REC_POOL_INFO;

/************************************状态相关信息************************************/

/*
    DSP系统模块运行状态
 */
typedef struct tagSysStatusSt
{
    int bXspInitAbnormally;                 /* XSP初始化是否异常，FALSE-已正常初始化，TRUR-初始化异常，已修改参数使其可以初始化，但无法正常运行 */
    unsigned int u32XspErrno;               /* XSP初始化异常时的错误号，无论初始化成功或失败，该值均有效 */
} SYS_STATUS;

/*
    DSP采集模块运行状态
 */
typedef struct tagCaptStatusSt
{
    volatile unsigned int bHaveSignal; /* 是否有视频信号输入,1 表示有，0 表示没有 */
} CAPT_STATUS;

/*
    DSP编码通道运行状态
 */
typedef struct
{
    volatile unsigned int encodeW;      /* 当前视频帧宽 */
    volatile unsigned int encodeH;      /* 当前视频帧高 */
    volatile unsigned int encodeSubW;   /* 当前子码流视频帧宽 */
    volatile unsigned int encodeSubH;   /* 当前子码流视频帧高 */
    volatile unsigned int encodedV;     /* 已编码的视频帧 底层统计 */
    volatile unsigned int encodedSubV;  /* 已编码的子通道视频帧 底层统计 */
    volatile unsigned int encodedA;     /* 已编码的音频帧 底层统计 */
    volatile unsigned int bpsV;         /* 视频码流，底层统计 */
    volatile unsigned int bpsA;         /* 音频码流 底层统计 */
    volatile unsigned int bps;          /* 码流  底层统计 */
    volatile unsigned int fpsEncV;      /* 视频编码帧率 底层统计 */
    volatile unsigned int fpsEncA;      /* 音频编码帧率 底层统计 */
} ENC_STATUS;

/*
    DSP解码通道运行状态
 */
typedef struct
{
    volatile unsigned int status;        /* 当前状态 */
    volatile unsigned int receivedData;  /* 接收到的数据 */
    volatile unsigned int invlidData;    /* 无效的数据 */
    volatile unsigned int decodedV;      /* 解码的视频帧 */
    volatile unsigned int decodedA;      /* 解码的音频帧 */
    volatile unsigned int streamAtype;   /* 当前解码音频编码类型*/

    volatile unsigned int streamVtype;   /* 当前解码视频编码类型 */

    volatile unsigned int decodedW;     /* 当前解码视频帧宽*/

    volatile unsigned int decodedH;     /* 当前解码视频帧高 */
    volatile unsigned int streamVfps;    /* 当前解码视频帧率 */

} DEC_STATUS;

/*
    IPC 通道运行状态
 */
typedef struct
{
    volatile unsigned int status;       /* 当前状态         */
    volatile unsigned int receivedData; /* 接收到的数据     */
    volatile unsigned int invlidData;   /* 无效的数据       */
    volatile unsigned int ipcDecV;      /* IPC 的视频帧     */
    volatile unsigned int ipcDecA;      /* IPC 的音频帧     */
    volatile unsigned int ipcAtype;     /* 当前解码音频编码类型*/
    volatile unsigned int ipcVtype;     /* 当前解码视频编码类型 */
    volatile unsigned int ipcDecW;      /* 当前解码视频帧宽 */
    volatile unsigned int ipcDecH;      /* 当前解码视频帧高 */
    volatile unsigned int ipcVfps;      /* 当前解码视频帧率 */
} IPC_STATUS;

/*
    DSP显示通道运行状态
 */
typedef struct
{
    volatile unsigned int fps;     /* 实时显示帧率 */
    volatile unsigned int uiDispW; /* 实时显示宽 */
    volatile unsigned int uiDispH; /* 实时显示高 */
} DISP_STATUS;

/****************************************************************************/
/*                                 采集                                     */
/****************************************************************************/
/* 视频标准 */
typedef enum VsStandardE
{
    VS_STD_PAL = 0,          /* PAL制  */
    VS_STD_NTSC = 1,         /* NTSC制 */
    VS_STD_BUTT,
} VS_STANDARD_E;

/* 应用场景和安全级 */
typedef enum
{
    ld_indoor_low = 0,          /* 室内 安全级低 */
    ld_indoor_middle = 1,       /* 室内 安全级中 */
    ld_indoor_high = 2,         /* 室内 安全级高 */
    ld_outdoor_low = 3,         /* 室外 安全级低 */
    ld_outdoor_middle = 4,      /* 室外 安全级中 */
    ld_outdoor_high = 5,        /* 室外 安全级高 */
    ld_SCENCE_NONE,
} LD_SCENE;

/*
    配置采集通道属性 HOST_CMD_SET_VIDEO_PARM
 */
typedef struct tagVideoChnPrmSt
{
    unsigned int x;                     /* 输出视频的起点, x     */
    unsigned int y;                     /* 输出视频的起点, y     */
    VS_STANDARD_E eStandard;            /* 视频标准,低分辨率区分 */
    unsigned int eResolution;           /* 分辨率枚举值          */
    unsigned int fps;                   /* 输出视频的帧率        */
} VIDEO_CHN_PRM_ST;

/*
    视频旋转
 */
typedef enum RotateE
{
    ROTATE_MODE_NONE = 0,                   /* 不旋转        */
    ROTATE_MODE_90 = 1,                     /* 顺时针旋转90  */
    ROTATE_MODE_180 = 2,                    /* 顺时针旋转180 */
    ROTATE_MODE_270 = 3,                    /* 顺时针旋转270 */
    ROTATE_MODE_COUNT,                      /* 旋转180度请使用 镜像来实现 */
} ROTATE_MODE;


/*
    配置采集通道旋转属性 HOST_CMD_SET_VIDEO_ROTATE
 */
typedef struct tagVideoRotateAttrSt
{
    unsigned int uiChan;                  /* 指定的通道 ,用于和 DISP_REGION 的各个通道对应 , 用于配置各个预览通道独立的镜像属性*/
    ROTATE_MODE eRotate;                  /* 旋转属性 */
} VIDEO_ROTATE_ATTR_ST;


/*
    视频镜像
 */
typedef enum MirrorE
{
    MIRROR_MODE_NONE = 0,            /* 不做镜像 */
    MIRROR_MODE_HORIZONTAL = 1,      /* 水平镜像/左右镜像 */
    MIRROR_MODE_VERTICAL = 2,        /* 垂直镜像/上下镜像 */
    MIRROR_MODE_CENTER = 3,          /* 中心镜像/镜像和颠倒/旋转180度  */
    MIRROR_MODE_COUNT,
} MIRROR_MODE;


/*
    配置采集通道镜像属性  HOST_CMD_SET_VIDEO_MIRROR
 */
typedef struct tagVideoMirrorAttrSt
{
    unsigned int uiChan;                  /* 指定的通道 ,用于和 DISP_REGION 的各个通道对应 , 用于配置各个预览通道独立的镜像属性*/
    MIRROR_MODE eMirror;                  /* 镜像属性 */
} VIDEO_MIRROR_ATTR_ST;

/*
    视频位置偏移
 */
typedef enum PositionE
{
    MOVE_HORIZONTAL_LEFT = 0,  /* 图像水平向左偏移 */
    MOVE_HORIZONTAL_RIGHT,     /* 图像水平向右偏移 */
    MOVE_VERTICAL_UP,          /* 图像垂直向上偏移 */
    MOVE_VERTICAL_DOWN,        /* 图像垂直向下偏移 */
    MODE_COUNT,
} POSITION_MODE;


/*
    配置采集通道位置偏移属性  HOST_CMD_SET_VIDEO_POS
 */
typedef struct tagVideoPositionAttrSt
{
    unsigned int uiChan;                  /* 指定vi 的通道*/
    unsigned int offset;                  /* 偏移值*/
    POSITION_MODE posMode;                /* 调整偏移模式*/
} VIDEO_POSITION_ATTR_ST;


/*
   输入视频接口模式
 */
typedef enum InputModeE
{
    INPUT_VIDEO_VGA = 0,
    INPUT_VIDEO_HDMI,
    INPUT_VIDEO_DVI,
    INPUT_VIDEO_NONE,
    INPUT_VIODO_BUTT                                  /* 最大的无效值 */
} INPUT_MODE_E;

/* 视频分辨率 */
typedef struct tagVideoResolution
{
    unsigned int uiWidth;
    unsigned int uiHeight;
    unsigned int uiFps;
} VIDEO_RESOLUTION_S;

/* CSC参数结构体 */
typedef struct tagVideoCsc
{
    unsigned int uiLuma;            /* 亮度值   */
    unsigned int uiHue;             /* 色度值   */
    unsigned int uiContrast;        /* 对比度值 */
    unsigned int uiSatuature;       /* 饱和度值 */
} VIDEO_CSC_S;

typedef struct tagInputVideoSt
{
    INPUT_MODE_E inputMode;
    VIDEO_RESOLUTION_S stVidoeRes;
} INPUT_VIDEO_ST;


/* 导入EDID失败的错误类型 */
typedef enum tagCaptEdidError
{
    CAPT_EDID_ERROR_FORMAT,                     /* 文件格式错误 */
    CAPT_EDID_ERROR_NO_EEPROM,                  /* 海思未连接EEPROM，如204的VGA */
    CAPT_EDID_ERROR_IIC,                        /* IIC总线错误 */
    CAPT_EDID_ERROR_BUTT,
} CAPT_EDID_ERROR_E;


/* EDID信息 */
typedef struct tagCaptEdidInfoSt
{
    unsigned int uiByteNum;                                     /* 字节数 */
    unsigned char *pEdidAddr;                                   /* 数据地址 */
    INPUT_MODE_E enInputMode;                                   /* HDMI或者VGA,返回给应用     */
    CAPT_EDID_ERROR_E enError;                                  /* 返回的错误类型 */
} CAPT_EDID_INFO_ST;


/* 显示通道读取显示器的EDID并写道输入中 */
typedef struct tagEdidDisp
{
    INPUT_MODE_E enMode;            /* 下发指令时必须保证对应输出接口是接着的，否则会出错 */
    unsigned int uiDispChn;         /* 输出通道 */
} CAPT_EDID_DISP_S;


/* 获取EDID信息结构体 */
typedef struct
{
    INPUT_MODE_E enMode;            /* 要读EDID的输入口 */
    char szManufactureName[8];      /* 显示器生产厂商 */
    char szMonitorName[32];         /* 显示器名称 */
    VIDEO_RESOLUTION_S stRes;       /* 默认分辨率 */
} CAPT_EDID_INFO_S;


/* 视频相关的芯片 */
typedef enum
{
    /* 输入芯片 */
#define VIDEO_INPUT_CHIP_BASE 0x00000000
    VIDEO_INPUT_CHIP_MSTAR = VIDEO_INPUT_CHIP_BASE + 0,
    VIDEO_INPUT_CHIP_ADV7842 = VIDEO_INPUT_CHIP_BASE + 1,

    /* 输出芯片 */
#define VIDEO_OUTPUT_CHIP_BASE 0x00010000
    VIDEO_OUTPUT_CHIP_CH7053 = VIDEO_OUTPUT_CHIP_BASE + 0,

    /* 其他 */
#define VIDEO_MISC_CHIP_BASE 0x00020000
    VIDEO_MISC_CHIP_FPGA = VIDEO_MISC_CHIP_BASE + 0,
    VIDEO_MISC_CHIP_MCU = VIDEO_MISC_CHIP_BASE + 1,
} VIDEO_CHIP_E;


typedef struct
{
    VIDEO_CHIP_E enChip;            /* 视频外设芯片 */
    char szBuildTime[32];           /* 编译时间 */
} VIDEO_CHIP_BUILD_TIME_S;

/*
    CAMERA 类型
 */
typedef enum
{
    CAM_USB = 0,
    CAM_PLATFORM,
    CAM_USB_940NM,
    CAM_USB_850NM,
    CAM_BOTH,
    CAM_INVALID
} CAM_IDX;

/*
    裁剪属性
 */
typedef struct tagCropInfoSt
{
    unsigned int uiEnable;  /* 是否使能裁剪 */
    unsigned int channel;   /* 该ROI参数给channel通道使用 */
    unsigned int uiX;       /* 起始坐标 x */
    unsigned int uiY;       /* 起始坐标 y */
    unsigned int uiW;       /* 裁剪宽     */
    unsigned int uiH;       /* 裁剪高     */
} CROP_INFO_ST;

/*
    采集通道属性
 */
typedef struct tagVideoInputInitPrmSt
{
    unsigned int eViResolution;         /* 视频输入分辨率            */
    VS_STANDARD_E eViStandard;          /* 输入视频制式，区分30/25帧 */
    CROP_INFO_ST stCropInfo;            /* 视频采集裁剪属性          */
} VI_INIT_PRM;

/*
    采集模块的信息
 */
typedef struct tagVideoInputInitInfoSt
{
    unsigned int uiViChn;                    /* vi 通道数        */
    VI_INIT_PRM stViInitPrm[MAX_CAPT_CHAN];  /* 每个vi通道的信息 */
} VI_INIT_INFO_ST;


/*
    采集模块宽动态的信息 HOST_CMD_SET_VIDEO_WDR
 */
typedef struct tagVideoWDRPrmSt
{
    unsigned int uiEnable;                    /* 是否使能 */
} VI_WDR_PRM_ST;



typedef struct
{
    char bWaitDfrBuf;                   /* 是否在等dfr buffer */
    int procType;
    char preSnap;                       /* 预抓状态       */
    unsigned int djpgFree;              /* free buf的状态 */
    unsigned int solidTime;             /* 立体检测状态   */
    unsigned int salM;
    unsigned int halM;
    unsigned int addtskId;              /* 送任务的ID号   */
    unsigned int rettskId;              /* 返回的ID号     */

    unsigned int tskTrackId[3];         /* 追踪的ID号 */
    unsigned int trackIdx;

    unsigned int list0Add;
    unsigned int list1Add;

    unsigned int list0Del;
    unsigned int list1Del;
    unsigned char buildDate[512];       /*编译日期*/
    unsigned char buildTime[512];       /*编译时间*/
    unsigned int svnVersion;            /*SVN 版本号*/
    unsigned int ldscene;

    unsigned int camIdx;                /*所用的camera类型*/

} DSP_STATUS;

/****************************************************************************/
/*                                 ISP                                      */
/****************************************************************************/

/*
    ISP 透传
    HOST_CMD_SET_ISP_PARAM
    HOST_CMD_GET_ISP_PARAM
 */
typedef struct _ISP_PARAM
{
    unsigned int cmd;
    unsigned int value;
} ISP_PARAM;


/* 补光灯控制 */
typedef struct _LIGHTCTRL_PARAM_
{
    unsigned int ctrlType;      /* 0 为自动控制亮度， 1 为手动调节亮度 */
    unsigned int luminance;     /* 范围 0-100。自动补光调节时: 补光的最大值；手动调节时: 指定的值 */
} LIGHTCTRL_PARAM;


/****************************************************************************/
/*                                 编码                                      */
/****************************************************************************/

/*
    编码属性  HOST_CMD_SET_ENCODER_PARAM
 */
typedef struct
{
    unsigned int muxType;               /* 封装格式，可选TS/PS/RTP/HIK*/
    unsigned int streamType;            /* 码流类型，目前可选视频流和复合流 */
    unsigned int videoType;             /* 视频格式，可选海康H264/标准H264/MPEG4*/
    unsigned int audioType;             /* 音频格式，可选G711/G722/MPEG1L2*/

    unsigned int resolution;            /* 分辨率 */
    unsigned int quant;                 /* 量化系数(1-31),变码率时有效 */
    unsigned int minQuant;              /* 最小量化系数(1-31) */
    unsigned int maxQuant;              /* 最大量化系数(1-31) */
    unsigned int bpsType;               /* 码率控制类型(0:变码率;1:定码率) */
    unsigned int bps;                   /* 码率(定码率时为码率，变码率时为码率上限) */
    unsigned int fps;                   /* 帧率 */
    unsigned int IFrameInterval;        /* I帧间隔 */
    unsigned int BFrameNum;             /* B帧个数 */
    unsigned int EFrameNum;             /* E帧个数 */
    unsigned int b2FieldEncode;         /* 两场编码 */
    unsigned int sysParam;              /* 码流参数，是否包含CRC，水印，RSA等，和文件头中的system_param定义相同 */
    unsigned int max_delay;             /* 码率控制延时时间(1-60),时间越短,瞬时码率越稳定,但图像
                                                     质量变化快,容易产生马赛克.场景变化大的网传模式下将max_delay
                                                     设短,保证码率稳定.推荐值为8. */
    unsigned int fast_motion_opt;       /* 快速运动图像质量优化(场编码有效),0-关闭,1-开启 */
} ENCODER_PARAM;

/* 图片描述信息缓存区的大小 */
#define JPEG_DESCRIPTION_INFO_SIZE (256)

/*
    编码抓图模式类型
 */
typedef enum jpegCaptureModeE
{
    CAP_ONESHOT = 0,        /* 单张抓拍，一次命令抓一张 */
    CAP_CONTINUE,           /* 连续抓拍 */
    CAP_INVALID
} JPEG_MODE;

/*
    编码抓图模式类型
 */
typedef enum jpegSrcTypeE
{
    JEEG_SRCTYPE_VI = 0,         /* 采集抓图 */
    JEEG_SRCTYPE_VDEC,           /* 解码抓图 */
    JEEG_SRCTYPE_NUM
} JPEG_SRC_TYPE;

/*
    编码抓图属性 HOST_CMD_SET_ENC_JPEG_PARAM
 */
typedef struct tagJpegParamSt
{
    unsigned int width;        /* 图片宽 */
    unsigned int height;       /* 图片高 */
    unsigned int quality;      /* 图片质量 */
    JPEG_MODE eJpegMode;       /* 抓图模式 */
    unsigned int uiCaptCnt;    /* 抓拍张数，在连续抓拍模式下有效 */
    unsigned int uiCaptSec;    /* 在 uiCaptSec 秒内均匀抓拍 uiCaptCnt 张数 */
} JPEG_PARAM;

/*
    开启编码抓图附加图片描述属性 HOST_CMD_START_ENC_JPEG
 */
typedef struct tagJpegAddInfo
{
    JPEG_PARAM stJpegPrm;
    unsigned int uiSize;                             /* 图片描述实际大小 */
    JPEG_SRC_TYPE srcType;                           /* 抓图数据源类型 0 采集,1 解码 */
    char cInfo[JPEG_DESCRIPTION_INFO_SIZE];          /* 图片描述缓存     */
} JPEG_ADD_INFO_ST;

typedef struct tagJpegPkgTimeSt
{
	/* 包裹前沿0：包裹进入画面的包裹前沿，1：包裹分包的包裹前沿 */
	float fPkgfwd[2];           

	/* 绝对时间0：包裹进入画面的时间，1：包裹分包的时间 */
    DATE_TIME absTime[2];
} JPEG_PKG_TIME_S;

/*
    编码抓图返回结构体 STREAM_ELEMENT_JPEG_IMG
 */
typedef struct tagJpegSnapCbResultSt
{
    unsigned char *cJpegAddr;       /* 回调的抓图数据的地址 */
    unsigned int uiJpegSize;        /* 回调的抓图数据的大小 */
    unsigned int uiWidth;           /* 回调的抓图宽度 */
    unsigned int uiHeight;          /* 回调的抓图高度 */
    unsigned int uiSyncNum;         /* 回调的同步帧索引 */
    unsigned long long ullTimePts;  /* 回调的同步时间戳 */
} JPEG_SNAP_CB_RESULT_ST;

/*
   编码通道画线控制
 */
typedef struct _ENC_DRAW_PARAM
{
    unsigned int encChn;  /* 编码通道 */
    unsigned int enable;  /* 画线使能标志；0 : 画线关闭、1:画线使能 */
} ENC_DRAW_PARAM;


/****************************************************************************/
/*                                  解码                                    */
/****************************************************************************/

/*
    应用下发待解码码流的共享缓冲区
 */
typedef struct DEC_SHARE_BUF_tag
{
    unsigned char *pPhysAddr;                       /* DSP访问地址 */
    unsigned char *pVirtAddr;                       /* ARM访问地址 */
    volatile unsigned int bufLen;                   /* 解码缓冲区长度 */
    volatile unsigned int writeIdx;                 /* 解码缓冲写索引 */
    volatile unsigned int readIdx;                  /* 解码缓冲读索引 */
    volatile unsigned int decodeStandardTime;       /* 解码器相对时间 */
    volatile unsigned int decodeAbsTime;            /* 解码器绝对时间 */
    volatile unsigned int decodePlayBackStdTime;    /* 解码器倒放相对时间 */
    int bIFrameOnly;                                /* 是否只送I帧 */
} DEC_SHARE_BUF;

/*
    解码模式
 */
typedef enum decModeE
{
    DECODE_MODE_INVALID = 0x0,                  /* 无效                        */
    DECODE_MODE_REALTIME = 0x1,                 /* 实时模式                    */
    DECODE_MODE_FILE = 0x2,                     /* 文件回放模式                */
    DECODE_MODE_FILE_REVERSE = 0x4,             /* 文件倒放模式，解码所有帧    */
    DECODE_MODE_I_FRAME = 0x8,                  /* I帧解码模式，用于高倍速快放 */
    DECODE_MODE_JPEG = 0x10,                    /* 解码JPEG抓图模式            */
    DECODE_BUTT                                  /* 最大的无效值 */
} DEC_MODE_E;

/*
    打包模式
 */
typedef enum muxStreamTypeE
{
    MPEG2MUX_STREAM_TYPE_UNKOWN = 0x0,              /* 未知打包格式       */
    MPEG2MUX_STREAM_TYPE_PS = 0x1,                  /* PS打包             */
    MPEG2MUX_STREAM_TYPE_RTP = 0x2,                 /* RTP打包            */
    MPEG2MUX_STREAM_TYPE_TS = 0x4,                  /* TS打包             */
    MPEG2MUX_STREAM_TYPE_HIK = 0x8,                 /* 海康自定义打包格式 */
    MPEG2MUX_STREAM_TYPE_ES = 0x10,                 /* 裸流               */
    MPEG2MUX_STREAM_TYPE_BUTT                       /* 最大的无效值        */
} MUX_STREAM_TYPE_E;


/*
    视频码流类型
 */
typedef enum videoStreamTypeE
{
    ENCODER_UNKOWN = 0x0,              /* 未知编码格式     */
    ENCODER_H264 = 0x1,                /* HIK 264          */
    ENCODER_S264 = 0x2,                /* Standard H264    */
    ENCODER_MPEG4 = 0x3,               /* MPEG4            */
    ENCODER_MJPEG = 0x4,               /* MJPEG            */
    ENCODER_MPEG2 = 0x5,               /* MPEG2            */
    ENCODER_QUICK = 0x6,               /* 浅编码           */
    ENCODER_MF = 0x7,                  /* 图像倍帧         */
    ENCODER_MFD4 = 0x8,                /* 下采样图像MV计算 */
    ENCODER_H265 = 0x9,                /* h265          */
    ENCODER_BUTT
} VIDEO_STREAM_TYPE_E;

/*
    音频码流类型
 */
typedef enum audioStreamTypeE
{
    STREAM_AUDIO_MPEG1 = 0x03,         /* MPEG1格式编码 */
    STREAM_AUDIO_MPEG2 = 0x04,         /* MPEG2格式编码 */
    STREAM_AUDIO_AAC = 0x0f,           /* AAC格式编码   */
    STREAM_AUDIO_G711A = 0x90,         /* G711格式A律编码 */
    STREAM_AUDIO_G711U = 0x91,         /* G711格式U律编码 */
    STREAM_AUDIO_G722_1 = 0x92,        /* G722格式编码    */
    STREAM_AUDIO_G723_1 = 0x93,        /* G723格式编码    */
    STREAM_AUDIO_G726 = 0x96,          /* G726格式编码    */
    STREAM_AUDIO_G728 = 0x98,          /* G728格式编码    */
    STREAM_AUDIO_G729 = 0x99,          /* G729格式编码    */
    STREAM_AUDIO_AMR_NB = 0x9a,        /* AMR_NB格式编码  */
    STREAM_AUDIO_AMR_WB = 0x9b,        /* AMR_WB格式编码  */
    STREAM_AUDIO_L16 = 0x9c,           /* L16格式编码     */
    STREAM_AUDIO_L8 = 0x9d,            /* L8格式编码      */
    STREAM_AUDIO_DVI4 = 0x9e,          /* DV14格式编码    */
    STREAM_AUDIO_GSM = 0x9f,           /* GSM格式编码     */
    STREAM_AUDIO_GSM_EFR = 0xa0,       /* GSM_EFR格式编码 */
    STREAM_AUDIO_LPC = 0xa1,           /* LPC格式编码     */
    STREAM_AUDIO_QCELP = 0xa2,         /* QCELP格式编码   */
    STREAM_AUDIO_VDVI = 0xa3,          /* VDVI格式编码    */
    STREAM_AUDIO_BUTT                   /* 最大无效值     */
} AUDIO_STREAM_TYPE_E;


/*
    开启解码属性  HOST_CMD_DEC_START
 */
typedef struct
{
    DEC_MODE_E mode;                 /* 解码模式 */
    MUX_STREAM_TYPE_E muxType;       /* 打包模式 */
    VIDEO_STREAM_TYPE_E videoType;   /* 视频码流类型 */
    unsigned int width;              /* 码流的宽 */
    unsigned int height;             /* 码流的高 */
    AUDIO_STREAM_TYPE_E audioType;   /* 音频码流类型 */
    unsigned int audioPlay;          /* 音频解码播放开关 */
    unsigned int audioChannels;      /*音频通道数,0 mono,1 stero*/
    unsigned int audioBitsPerSample; /*8/16*/
    unsigned int audioSamplesPerSec; /*sample rate */
} DECODER_PARAM;


/* 解码模式 */
typedef enum
{
    DECODE_MODE_NORMAL = 0x0,                    /*正常解码，不支持快放慢放*/
    DECODE_MODE_MULT = 0x1,                      /*倍速解码，可以快放、慢放*/
    DECODE_MODE_ONLY_I_FREAM = 0x2,              /*I 帧解码,暂时不支持快放慢放*/
    DECODE_MODE_REVERSE = 0x3,                   /*倒放模式，暂时不支持快放慢放*/
    DECODE_MODE_I_TO_BMP = 0x4,                  /*i帧解码成bmp，暂时不支持快放慢放*/
    DECODE_MODE_DRAG_PLAY = 0x5,                 /*拖放模式，暂时不支持快放慢放*/
    DECODE_MODE_BULL,
} DECODER_MODE;


/*解码器速度*/
typedef enum
{
    DECODE_SPEED_MAX = 0,        /*以最大的速度解码 */
    DECODE_SPEED_256x =	1,
    DECODE_SPEED_128x =	2,
    DECODE_SPEED_64x = 3,
    DECODE_SPEED_32x = 4,
    DECODE_SPEED_16x = 5,
    DECODE_SPEED_8x = 6,         /*快放8倍及以上只解i帧*/
    DECODE_SPEED_4x = 7,
    DECODE_SPEED_2x = 8,
    DECODE_SPEED_1x = 9,
    DECODE_SPEED_12x = 10,       /* 1/2 速度 */
    DECODE_SPEED_14x = 11,       /* 1/4 速度 */
    DECODE_SPEED_18x = 12,       /* 1/8 速度 */
    DECODE_SPEED_116x =	13,      /* 1/16速度*/
    DECODE_SPEED_BULL,
} DECODE_SPEED;

typedef struct
{
    unsigned int speed;     /*解码器速度,见DECODE_SPEED*/
    unsigned int vdecMode;  /* 解码模式，见DECODER_MODE */
} DECODER_ATTR;


typedef enum
{
    SYNC_MODE_NON = 0,
    SYNC_MODE_MASTER = 1,
    SYNC_MODE_SLAVE = 2,
    SYNC_MODE_BULL,
} DECODE_SYNC_MODE;

typedef struct tagDecSyncPrmSt
{
    DECODE_SYNC_MODE mode;             /* 解码同步模式 */
    unsigned int syncMasterChanId;     /* 解码同步的主通道号 */
} DECODER_SYNC_PRM;

typedef struct
{
    unsigned int cover_sign;    /*覆盖帧标志*/
    unsigned int logo_pic_indx; /*logo画面索引*/
} DECODEC_COVER_PICPRM;

typedef struct tagRecodePrmSt
{
    unsigned int srcEncType;        /* 转码源编码类型,使用VIDEO_STREAM_TYPE_E */
    unsigned int srcPackType;       /* 转码源打包包的类型,使用MUX_STREAM_TYPE_E */
    unsigned int dstPackType;       /* 目的打包包的类型 ，使用MUX_STREAM_TYPE_E，多个则使用1左移方式*/
    AUDIO_STREAM_TYPE_E audioType;  /* 音频码流类型 */
    unsigned int audioPackPs;       /* 音频转包ps开关 */
    unsigned int audioPackRtp;      /* 音频转包rtp开关 */
    unsigned int audioChannels;      /*音频通道数,0 mono,1 stero*/
    unsigned int audioBitsPerSample; /*8/16*/
    unsigned int audioSamplesPerSec; /*sample rate */
} RECODE_PRM;


typedef enum
{
    PIC_TYPE_LOGO = 0x0,                       /*LOGO的图片*/
    PIC_TYPE_NO_SIGNAL = 0x1,                  /*无视频信号*/
    PIC_TYPE_ABNOMAL_SIGNAL = 0x2,             /*异常信号图片*/
    PIC_TYPE_BLACK_BACK = 0x3,                 /*黑色背景图片*/
    PIC_TYPE_NO_LINK_SIGNAL = 0x4,             /*无网络信号图片*/
    PIC_TYPE_BULL = 0x5
} DECODE_PIC_TYPE;

typedef struct tagDecPicPrmST
{
    unsigned int enable;                  /* 使能开关*/
    DECODE_PIC_TYPE vdecPicType;          /* 图片类型*/
} DECODER_PIC_PRM;


typedef struct tagDecJpgBmpDataST
{
    unsigned int u32Width;                /* 图像宽    */
    unsigned int u32Height;               /* 图像高 */
	unsigned int u32DataSize;             /* 数据实际大小 */
    char *pDataBuff;                      /* jpg or bmp数据 */
    unsigned int u32BuffSize;             /* buff大小，用于校验防止拷贝越界 */
} DECODER_JPG_BMP_DATA;


typedef struct tagDecJpgToBmpST
{
    DECODER_JPG_BMP_DATA stJpgData;       /*jpg 数据   在jpg2bmp中为输入参数*/
    DECODER_JPG_BMP_DATA stBmpData;       /*bmp 数据 在jpg2bmp中为输出参数*/
} DECODER_JPGTOBMP_PRM;


/*  解码bmp返回结构体 STREAM_ELEMENT_JPEG_IMG
 */
typedef struct tagBmpCbResultSt
{
    unsigned char *cBmpAddr;    /* 回调的BMP数据的地址 */
    unsigned int uiBmpSize;     /* 回调的BMP数据的大小 */
    unsigned int uiSyncNum;     /* 回调的BMP数据同步帧索引 */
} BMP_RESULT_ST;


/****************************************************************************/
/*                                    显示                                   */
/****************************************************************************/

#define DISP_FRM_CNT (MAX_DISP_CHAN)                       /*每路显示图像缓冲个数*/

/* 缩放类型 */
typedef enum
{
    DPTZ_ZOOM_LOCAL,        /* 局部放大 */
    DPTZ_ZOOM_GLOBAL,       /* 全局放大 */
    DPTZ_ZOOM_MODE_NUM,     /* 缩放类型数 */
}DPTZ_ZOOM_MODE;

/* 单个显示窗口的属性 */
typedef struct
{
    unsigned int uiChan;        /* 通道号，应用管理，取值范围[0, stVoInfoPrm[voDev].voChnCnt-1] */
    unsigned int x;             /* 窗口的起始坐标的横坐标 */
    unsigned int y;             /* 窗口的起始坐标的纵坐标 */
    unsigned int w;             /* 窗口的宽度 */
    unsigned int h;             /* 窗口的高度 */
    unsigned int fps;           /* 显示帧率 */
    unsigned int dispW;         /* 显示的视频的宽度 */
    unsigned int dispH;         /* 显示的视频的高度 */
    unsigned int dispPitch;     /* 显示的视频的跨度 */
    unsigned int color;         /* 无视频显示时窗口的颜色值 */
    unsigned int layer;         /* Android 需要此参数作为视频窗口的高度*/
    unsigned int bDispBorder;   /* 是否显示边框*/
    unsigned int BorDerColor;   /* 边框颜色 */
    unsigned int BorDerLineW;   /* 边框线框 */
} DISP_REGION;

typedef struct
{
    unsigned int x;             /* 放大中心点横坐标，该坐标是基于输出屏幕的，比如1080P，则范围为[0, 1919] */
    unsigned int y;             /* 放大中心点纵坐标，该坐标是基于输出屏幕的，比如1080P，则范围为[0, 1079] */
    unsigned int mode;          /* 缩放类型，参考枚举DPTZ_ZOOM_MODE */
    unsigned int w;             /* 输出的缩放后图像宽 */
    unsigned int h;             /* 输出的缩放后图像高 */
    unsigned int voChn;         /* 显示窗口通道号*/
    unsigned int voDev;         /* 显示输出设备号 */
    unsigned int viGlobalChn;   /* 局部时使用，局部放大的源（全局放大窗口号） */
    float ratio;                /* 放大倍数，取值范围[1.0, 16.0]，1.0为关闭放大 */
    unsigned int defaultenlargesign; /* 是否为默认放大，TRUE表示该放大倍数为默认放大倍数 */
} DISP_DPTZ;

/*
    HOST_CMD_DISP_CLEAR 配置不显示的预览窗口
 */
typedef struct tagDispClearCtrlSt
{
    unsigned int uiCnt;                  /* 配置的窗口的个数   */
    unsigned int uiChan[MAX_DISP_CHAN];  /* 窗口clear的通道号  */
} DISP_CLEAR_CTRL;

/*
    配置全局放大结构体
 */
typedef struct tagDispGlobalEnlargePrm
{
    unsigned int x;      /* 返回全局放大中心点x坐标  */
    unsigned int y;      /* 返回全局放大中心点y坐标  */
    float ratio;         /* 返回全局放大缩放比例  */
} DISP_GLOBALENLARGE_PRM;

/*
    HOST_CMD_ALLOC_DISP_REGION 预览以及预览窗口控制参数
 */
typedef struct tagDispCtrlSt
{
    unsigned int uiCnt;                        /* 配置的窗口的个数 */
    DISP_REGION stDispRegion[MAX_DISP_CHAN];   /* 窗口显示的属性   */
} DISP_CTRL;

typedef struct tagDispPosCtrlSt
{
    unsigned int voChn;
    unsigned int voDev;
    unsigned int enable;
    DISP_REGION stDispRegion;                  /* 窗口显示的属性   */
} DISP_POS_CTRL;
typedef struct
{
    unsigned int uiCnt;                        /* 配置的窗口的个数 */
    CROP_INFO_ST stCropRegion[MAX_DISP_CHAN];  /* 窗口显示的属性   */
} CROP_PARAM;

#define SCREEN_LAYER_MAX (3)
#define PALETTE_COLOR_MAX (16)

/*
    位置坐标
 */
typedef struct tagPosAttrSt
{
    unsigned int x;
    unsigned int y;
} POS_ATTR;

/*
    图层的信息，应用在此图层做界面
 */
typedef struct tagScreenAttrSt
{
    unsigned int isUse;                         /* 是否使用该图层 */
    unsigned int x;                             /* 起始坐标 */
    unsigned int y;                             /* 起始坐标 */
    unsigned int width;                         /* 宽度     */
    unsigned int height;                        /* 高度     */
    unsigned int pitch;                         /* 跨度     */
    unsigned int bitWidth;                      /* 位宽     */
    unsigned char *srcAddr;                     /* 显存起始地址 */
    unsigned int srcSize;                       /* 显存大小 */
    unsigned int mouseBindChn;                  /* 鼠标绑定到显示器的通道号*/
    unsigned int palette[PALETTE_COLOR_MAX];    /* 鼠标颜色调色板 */
    unsigned int alpha_idx;                     /* 透明颜色的索引值 */
} SCREEN_ATTR;

/*
   物品类型
 */
typedef enum tagArticleType
{
    SVA_TYPE,                        /* 智能 */
    ORGNAIC_TYPE,                  /* 有机物 */
    TIP_TYPE,                        /* TIP */
    UNPEN_TYPE,                        /* TIP */
    DEFAULT_TYPE,                    /* 默认 */
} ARTICLE_TYPE;

/*
    显示器实时统计各种类型智能信息个数
 */
typedef struct tagDispArticleResult
{
    unsigned int svaresult;           /* 智能信息个数统计 */
    unsigned int orgnaicresult;       /* 有机物信息个数统计 */
    unsigned int tipresult;           /* tip信息个数统计 */
} DISP_ARTICLE_RESULT;

/*
   显示画线闪烁时间
 */

typedef struct tagDispFlickerTime
{
    ARTICLE_TYPE type;                          /*智能 有机物 TIP*/
    unsigned long long timedisplay;                  /* 智能信息显示时间 */
    unsigned long long timedisappear;                /* 智能信息消失时间 */
    unsigned long long currentsystime;
    unsigned int vochn;                              /* 显示通道号*/
} DISPFLICKER_PRM;
/*
   危险品信息的显示方式
 */
typedef enum tagDspDispDrawAIType
{   
    AI_DISP_DRAW_ALL,                /*危险品全部显示*/
    AI_DISP_DRAW_ONE_TYPE,           /*每一类危险品只画一个*/
} DSP_DISP_DRAW_AI_TYPE;

/*
   显示框画线类型
 */
typedef struct tagDispLineType
{
    ARTICLE_TYPE type;                          /* 智能 有机物 TIP，安检机不区分类型，画线参数唯一 */
    DISP_FRAME_TYPE frametype;                  /* 智能框画线类型 */
    DSP_DISP_DRAW_AI_TYPE emDispAIDrawType;     /* 危险品信息的显示方式 */
    unsigned int fulllinelength;                /* 实线长度*/
    unsigned int gaps;                          /* 缝隙长度*/
    unsigned int node;                          /* 缝隙中点的个数，点的总长度要小于缝隙长度*/
    unsigned int vochn;                         /* 显示通道号*/
} DISPLINE_PRM;

/*
    HOST_CMD_DISP_FB_MMAP 获取界面图层地址
 */
typedef struct tagScreenCtrlSt
{
    unsigned int uiCnt;
    SCREEN_ATTR stScreenAttr[SCREEN_LAYER_MAX];
} SCREEN_CTRL;


/*
    HOST_CMD_DISP_FB_SHOW 刷新屏幕指定区域
 */
typedef struct
{
    unsigned int x;                             /* 起始坐标 */
    unsigned int y;                             /* 起始坐标 */
    unsigned int width;                         /* 宽度     */
    unsigned int height;                        /* 高度     */
    unsigned int pitch;                         /* 跨度     */
} LAYER_ATTR;

/*
    HOST_CMD_DISP_FB_SHOW 属性屏幕指定图层窗口的指定区域
 */
typedef struct
{
    unsigned int layer;
    LAYER_ATTR layerAttr;
} SCREEN_SHOW;


/*
    视频采集到显示的映射信息
 */
typedef struct
{
    unsigned int uiCnt;
    unsigned int bCenter[DISP_FRM_CNT];     /* 是否居中显示      目前仅用于安检机培训模式下输入源小于窗口大小时使用 */
    unsigned int addr[DISP_FRM_CNT];        /* 采集编号(DUP编号)或者编码通道 */
    unsigned int out[DISP_FRM_CNT];         /* 显示器上显示通道编号(0-N)*/
    unsigned int draw[DISP_FRM_CNT];        /* 是否叠加智能信息，无智能信息项目可以忽略此参数*/
    unsigned int pip[DISP_FRM_CNT];         /* 是否开启画中画，6550d vi显示画面支持缩略图*/
} DSP_DISP_RCV_PARAM;

typedef struct
{
    unsigned int uiChnNum;                  /* 需要设置OSD的Vo通道总数 */
    unsigned int uiVoChn[DISP_FRM_CNT];     /* 需要配置的Vo通道号 */
    unsigned int uiEnable[DISP_FRM_CNT];        /* 需要配置的各通道OSD开关使能状态 */
} DSP_DISP_OSD_PARAM;

/*
    显示风格枚举
 */
typedef enum StyleE
{
    STYLE_STANDARD_MODE,     /* 标准模式 */
    STYLE_BRIGHT_MODE,       /* 明亮     */
    STYLE_SOFT_MODE,         /* 柔和     */
    STYLE_GLARING_MODE       /* 鲜艳     */
} STYLE_E;

/*
    设置显示风格  HOST_CMD_SET_VO_STYLE
 */
typedef struct tagDispStyleSt
{
    STYLE_E eStyle;        /* 显示风格属性 */
} DISP_STYLE;

/*
    采集包裹有效数据区域
 */
typedef struct tagDispEffReg
{
    unsigned int vodev;  /*设备号*/
    unsigned int vochn;  /*窗口号*/
    unsigned int width;     /* 包裹宽 */
    unsigned int height;    /* 包裹高 */
    RECT rect;       /*有效区域*/
    float fZoomRatioH;  /* 横向显示缩放比例，即原始数据分辨率/显示图像分辨率 */
    float fZoomRatioV;  /* 纵向显示缩放比例，即原始数据分辨率/显示图像分辨率 */
} DISP_EFF_REG;

/*最多菜单显示区域 */
#define MAX_MENU_DISP_NUM 3

/*
    菜单区域属性  HOST_CMD_SET_MENU
 */
typedef struct
{
    int bFilter;                                /* 是否进行垂直滤波，消除抖动 */
    int bFullScreen;                            /* 是否全屏显示               */
    int bEnable[MAX_MENU_DISP_NUM];             /* 菜单开关                   */
    unsigned int menuAlpha[MAX_MENU_DISP_NUM];  /* 菜单与背景图象对比度       */
    /* 非全屏显示时指定显示窗口位置 */
    unsigned int x[MAX_MENU_DISP_NUM];
    unsigned int y[MAX_MENU_DISP_NUM];
    unsigned int w[MAX_MENU_DISP_NUM];
    unsigned int h[MAX_MENU_DISP_NUM];
} MENU_PARAM;


/*
    菜单区域刷新属性  HOST_CMD_REFRESH_MENU_DISP
 */
typedef struct
{
    int bFullScreen;   /*是否全屏刷新*/

    /*非全屏刷新时指定刷新位置*/
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
} MENU_REFRESH_PARAM;

/*
    视频输出标准
 */
typedef enum
{
    VS_NON = 0xa000,
    VS_NTSC = 0xa001,
    VS_PAL = 0xa002,
    VS_WD1_NTSC = 0xa003,
    VS_WD1_PAL = 0xa004,
    VS_720P_24HZ = 0xa005,
    VS_720P_25HZ = 0xa006,
    VS_720P_30HZ = 0xa007,
    VS_720P_50HZ = 0xa008,
    VS_720P_60HZ = 0xa009,
    VS_1080I_50HZ = 0xa00a,
    VS_1080I_60HZ = 0xa00b,
    VS_1080P_24HZ = 0xa00c,
    VS_1080P_25HZ = 0xa00d,
    VS_1080P_30HZ = 0xa00e,
    VS_1080P_50HZ = 0xa00f,
    VS_1080P_60HZ = 0xa010,
    VS_HF1080P_25HZ = 0xa011,
    VS_HF1080P_30HZ = 0xa012,
    VS_HF720P_50HZ = 0xa013,
    VS_HF720P_60HZ = 0xa014,
    VS_I576_50HZ = 0xa015,
    VS_I486_60HZ = 0xa016,
    VS_S1080_24HZ = 0xa017,
    VS_UNKNOWN_VIDEO = 0xa018,
    VS_NO_VIDEO = 0xa019,
    VS_BULL,
} VIDEO_STANDARD;

/*
    显示输出接口类型
 */
typedef enum tagVideoOutputDevEn
{
    VO_DEV_LCD = 0,       /* LCD显示  */
    VO_DEV_VGA = 1,       /* VGA 显示 */
    VO_DEV_HDMI = 2,      /* HDMI显示 */
    VO_DEV_CVBS = 3,      /* CVBS显示 */
    VO_DEV_MIPI = 4,      /* MIPI显示 */
    VO_DEV_LCD_1 = 5,       /* 第二路LCD显示  */
    VO_DEV_VGA_1 = 6,       /* 第二路VGA 显示 */
    VO_DEV_HDMI_1 = 7,      /* 第二路HDMI显示 */
    VO_DEV_CVBS_1 = 8,      /* 第二路CVBS显示 */
    VO_DEV_MIPI_1 = 9,      /* 第二路MIPI显示 */
    VO_DEV_MAX_CNT = 10,   /* 最大无效值 */
} VIDEO_OUTPUT_DEV_E;

/*
    设置显示输出标准  HOST_CMD_SET_VO_STANDARD
 */
typedef struct tagDispStandardSt
{
    VIDEO_STANDARD eStandard;        /* 显示输出标准 */
} DISP_STANDARD;

/*
    设置显示设备属性
 */
typedef struct tagDispDevAttrSt
{
    VIDEO_OUTPUT_DEV_E eVoDev;  /* 显示输出设备类型 */

    int videoOutputWidth;       /* 视频输出设备宽度 */
    int videoOutputHeight;      /* 视频输出设备高度 */
    int videoOutputFps;         /* 视频输出设备刷新帧率 */
} DISP_DEV_ATTR;


/*
    显示通道属性
 */
typedef struct tagVideoOutputInitPrmSt
{
    unsigned int bHaveVo;               /* 是否有视频输出   */
    unsigned int voChnCnt;              /* 应用设置的最大通道数，只接收一次 */
    DISP_DEV_ATTR stDispDevAttr;        /* 显示设备属性     */
} VO_INIT_PRM;

/*
    显示输出信息
 */
typedef struct tagVideoOutputInitInfoSt
{
    unsigned int uiVoCnt;                    /* vo 输出接口数     */
    VO_INIT_PRM stVoInfoPrm[MAX_DISP_CHAN];  /* 每一路vo 输出属性 */
} VO_INIT_INFO_ST;

/*
    获取的输出帧信息 HOST_CMD_DISP_GET_VO_FRAME
 */
typedef struct tagVoFrameInfoSt
{
    unsigned int width;                    /* 图像的宽                 */
    unsigned int height;                   /* 图像的高                 */
    unsigned int stride;                   /* 图像水平跨距             */
    unsigned int dataFormat;               /* 图像格式，预留目前为ARGB  */
    void * pVirAddr;                       /* 图像缓冲的虚拟地址        */
} VO_FRAME_INFO_ST;

/****************************************************************************/
/*                                 OSD                                     */
/****************************************************************************/
#define OSD_MAX_LINE                (16)        /* OSD的最大行数 */
#define MAX_CHAR_PER_LINE           (960 / 8)   /* OSD每行最大字符数 */
#define OSD_PREDEFINED_STRNUM_MAX   (128)       /* 最多预定义OSD String个数 */
#define OSD_PREDEFINED_STRLEN_MAX   (40)        /* 每个预定义OSD String的最大长度 */
#define OSD_PREDEFINED_CODE_BASE    (0x900000)  /* 预定义OSD字符串基址 */

/*
    编码格式
 */
typedef enum tagEncodingFormat
{
    ENC_FMT_GB2312,
    ENC_FMT_UTF8,
    ENC_FMT_ISO_8859_6, /* Arabic */
    ENC_FMT_MAX_CNT,
} ENCODING_FORMAT_E;

/*
    OSD功能所使用的字库信息，由应用下发
 */
typedef struct tagFontLibInfoSt
{
    MEMORY_BUFF_S stHzLib;              /* 汉字字库，保存alipuhui.ttf文件 */
    MEMORY_BUFF_S stAscLib;             /* 英文字库，保存arial.ttf文件，非中文使用 */

    /* TODO: 编码OSD字库先采用HZK16和ASC16，后续可改成freetype */
    MEMORY_BUFF_S stEncHzLib;           /* 编码时使用的汉字字库 */
    MEMORY_BUFF_S stEncAscLib;          /* 编码时使用的英文字库 */
    ENCODING_FORMAT_E enEncFormat;      /* 下发的字符编码格式，初始化时指定，运行期间不允许修改 */
} FONT_LIB_INFO_ST;

/*
    每一行OSD的信息
 */
typedef struct tagOsdLineParamSt
{
    unsigned int charCnt;                   /* 当前行字符个数 */
    unsigned int top;                       /* 高度 */
    unsigned int hScale;                    /* 水平缩放比例，0：无放大，1：放大两倍，2：放大四倍 */
    unsigned int vScale;                    /* 垂直缩放比例，0：无放大，1：放大两倍，2：放大四倍 */

    /**
     * 字符内码
     * ASCII码 ：0 ~ 0xFF
     * 特殊字符：OSD_TAG_CODE_BASE ~ OSD_TAG_CODE_BASE+OSD_TAG_CNT-1
     * 汉字内码：0xA1A1 ~ 0xFEFE，高位字节为区位码（把01~94加上0xA0）
     *           A1~A9为符号区（A1-标点符号，A2-序号，A3-数字，A4/A5-日文，A6-希腊字母，A7-俄文字母，A8/A9-汉语拼音，AA~AF-自定义符号）
     *           B0~FE为汉字区（B0~D7-一级汉字区，D8~F7-二级汉字区，F8~FE-自定义汉字区）
     * 预定义字符串：OSD_PREDEFINED_CODE_BASE ~ OSD_PREDEFINED_CODE_BASE+OSD_PREDEFINED_STRNUM_MAX-1
     */
    unsigned int code[MAX_CHAR_PER_LINE];   /* 仅支持可打印ASCII码（32~126）和预定义字符串 */
} OSD_LINE_PARAM;

/*
    OSD设置属性
    HOST_CMD_SET_ENC_OSD
    HOST_CMD_SET_DISP_OSD
 */
typedef struct tagOsdConfigSt
{
    unsigned int bStart;                /* 是否开启显示   */
    unsigned int flash;                 /*闪烁高8位为显示时间，低8位为停止时间，交替显示即可实现闪烁。例：flash＝(2<<8)|1 表示Logo显示2秒，停止1秒*/
    int bAutoAdjustLum;                 /* 是否自动调整亮度，自动调整时，指定的亮度值无效 */
    unsigned int lum;                   /* OSD的亮度值。   */
    int bTranslucent;                   /* 是否半透明。    */
    OSD_LINE_PARAM line[OSD_MAX_LINE];  /* 每一行OSD的信息 */
} OSD_CONFIG;

/**
 * @struct  OSD_PREDEFINED_PARAM 
 * @brief   OSD预定义字符串参数配置 
 */
typedef struct
{
    unsigned int stringNum; /* 有效的OSD String数量，取值范围[0, OSD_PREDEFINED_STRNUM_MAX-1] */
    char stringVal[OSD_PREDEFINED_STRNUM_MAX][OSD_PREDEFINED_STRLEN_MAX]; /* 每个OSD String的值，GB2312编码 */
} OSD_PREDEFINED_PARAM;

/*
    夏令时数据结构
*/
typedef struct
{       /* 8 bytes */
    unsigned char       mon;
    unsigned char       weekNo;
    unsigned char       date;
    unsigned char       hour;
    unsigned char       min;
    unsigned char       res[3];
}DST_TIME_POINT_S;

typedef struct
{
    unsigned char      uenableDST;      /* 启用夏令时 */
    unsigned int       DSTBias;         /* 时间调整值 */
    DST_TIME_POINT_S   startpoint;      /* 夏令时起止时间 */
    DST_TIME_POINT_S   endpoint;
}OSD_DST_PRAM_S;


#define MAX_LOGO_NUM_PER_CHAN (4)       /*每个通道最多含四个LOGO*/

/*
    LOGO信息属性
 */
typedef struct tagLogoPrmSt
{
    unsigned int flash;     /* 闪烁控制。例：flash＝(2<<8)|1 表示Logo显示2秒，停止1秒 */
    unsigned int maskY;     /* 屏蔽色，当LOGO图像中的值和屏蔽色相等时，LOGO透明，显示原图像 */
    unsigned int maskU;
    unsigned int maskV;
    unsigned int bTranslucent;      /*是否半透明*/
} LOGO_PARAM;

#define LOGO_MAX_W	256
#define LOGO_MAX_H	128

/*
    LOGO信息属性
    HOST_CMD_SET_ENC_LOGO
 */
typedef struct
{
    unsigned int idx;       /* 当前LOGO的序号 最大数为 MAX_LOGO_NUM_PER_CHAN */
    unsigned int x;
    unsigned int y;
    unsigned int w;         /* 不大于LOGO_MAX_W，且为LOGO_H_ALIGN的整数倍 */
    unsigned int h;         /* 不大于LOGO_MAX_H，且为LOGO_V_ALIGN的整数倍 */

    unsigned int uiIsPic;   /* 是否使用图片，0:使用色块， 1:使用图片 */

    LOGO_PARAM param;       /* 纯色块属性 */
    unsigned int uiPicAddr; /* 图片数据内存地址 */
    unsigned int uiPicSize; /* 图片数据大小     */
} LOGO_CONFIG;

/****************************************************************************/
/*                                 音频                                     */
/****************************************************************************/

/*
    音频输出通道信息
 */
typedef struct tagAudioChnInfoSt
{
    unsigned int aoChan;      /* 操作所对应的音频通道 */
    unsigned int addr;        /* 操作所对应的地址     */
} AUDIO_CHN_INFO_S;


/*
    操作音频输出结构体
    HOST_CMD_SET_AUDIO_LOOPBACK
    HOST_CMD_DEC_SET_DSP_AUDIO

    HOST_CMD_SET_TALKBACK
 */
typedef struct
{
    unsigned int bEnable;         /* 是否使能         */
    AUDIO_CHN_INFO_S stAudioChn;  /* 音频输出通道信息 */
} DSP_AUDIO_PARAM;

/*
    网络其他设备和模拟室内机对讲 控制信息
    HOST_CMD_SET_MANAGE_TALKTO_RESIDENT
 */
typedef struct tagAudioManageToResidentPrmSt
{
    unsigned int bEnable;           /* 是否使能 */
} AUDIO_MANAGE_TO_RESIDENT_PRM;

/*
    门口机与模拟室内机通信 控制信息
    HOST_CMD_SET_DOOR_TALKTO_RESIDENT
 */
typedef struct tagAudioDoorToResidentPrmSt
{
    unsigned int bEnable;           /* 是否使能 */
} AUDIO_DOOR_TO_RESIDENT_PRM;


/*
    语音播报类型
 */
typedef enum
{
    AUDIO_PLAY_NON = 0x0,
    AUDIO_PLAY_KEY = 0xa1,
    AUDIO_PLAY_INFO = 0xa2,
    AUDIO_PLAY_ALARM = 0xa3,
    AUDIO_PLAY_BUTT
} AUDIO_PLAY_TYPE;


/*
    语音播报功能 信息
    HOST_CMD_START_AUDIO_PLAY
 */
typedef struct
{
    void *audioBuf;                        /* 音频数据缓存 */
    unsigned int audioDataLen;             /* 音频数据长度，除去文件头 */
    unsigned int channel;                  /* 单通道或者双通道 */
    unsigned int rate;                     /* 采样率 */
    unsigned int bits;                     /* 数据位数 */
    unsigned int uiPlayTime;               /* 播放时间:单位ms，0-播放一遍
                                              非0-播放固定时间，一遍结束后重复播放 */
    AUDIO_PLAY_TYPE stPlayType;            /* 语音播报类型 */
    AUDIO_CHN_INFO_S stAudioChn;           /* 音频输出通道信息 */
    unsigned int priority;                /* 音频播放优先级，值越大优先级越高,范围1~10 */
    unsigned int res[1];                   /* 预留 */
} AUDIO_PLAY_PARAM;

/*
   语音合成启动初始化
 */
typedef struct
{
    unsigned int voiceType;      /* 发声类型0女声1男声 */
    unsigned int enable;        /* 文字文字转语音使能 */
    unsigned int res[1];           /* 预留         */
} AUTO_TTS_START_PARAM;

/*
   语音合成
 */

typedef struct
{
    char *txtBuf;               /* 文字数据缓存 */
    unsigned int txtDataLen;    /* 文字数据长度 */
    unsigned int res[1];           /* 预留         */
} AUTO_TTS_HANDLE_PARAM;


/*
    自动应答功能结构体
    HOST_CMD_START_AUTO_ANSWER
 */
typedef struct
{
    char *audioBuf;               /* 音频数据缓存 */
    unsigned int audioDataLen;    /* 音频数据长度 */
    unsigned int res[2];          /* 预留         */
} AUTO_ANSWER_PARAM;

/*
    开启语音录制
    HOST_CMD_START_AUDIO_RECORD
 */
typedef struct tagAudioRecordPrmSt
{
    unsigned int uiTime;          /* 录制时长        */
} AUDIO_RECORD_PRM_S;

/*
    设置声音超限报警阈值
    HOST_CMD_SET_AUDIO_ALARM_LEVEL
 */
typedef struct tagAudioAlarmLevelPrmSt
{
    unsigned int bEnable;         /* 是否使能 */
    unsigned int uiLevel;         /* 阈值     */
} AUDIO_ALARM_LEVEL_PRM_S;

/*
    设置音频输入音量
    HOST_CMD_SET_AI_VOLUME
    设置音频输出音量
    HOST_CMD_SET_AO_VOLUME
 */
typedef struct tagAudioVolPrmSt
{
    unsigned int uiChn;       /* 音频输入或输出通道 */
    unsigned int uiVol;       /* 音频声音值   */
} AUDIO_VOL_PRM_S;


/*
    设置听筒免提切换
    HOST_CMD_SET_EAR_PIECE
 */
typedef struct tagAudioEarpiecePrmSt
{
    unsigned int uiIsUsed;       /* 是否使用听筒 1 表示听筒，0 表示免提 */
} AUDIO_EARPIECEL_PRM_S;

/****************************************************************************/
/*                         二代安检分析仪                                   */
/****************************************************************************/
#define SVA_XSI_MAX_ALARM_TYPE	(128)        /* XSI模型报警危险最大类型 */
#define SVA_AI_MAX_ALARM_TYPE	(64)        /* AI模型检测最大危险品类型 */
#define SVA_XSI_MAX_ALARM_NUM	(128)       /* XSI模型报警危险最大个数 */
#define SVA_AI_MAX_ALARM_NUM	(64)        /* AI模型报警危险最大个数 */
#define SVA_ALERT_NAME_LEN		(40)        /* 报警物标定名称长度 */
#define SVA_PKG_LEVEL_NAME_LEN	(20)        /* 包裹危险级别名称长度 */
#define SVA_MODEL_NAME_MAX_LEN	(128)       /* 算法模型命名最大长度 */
#define SVA_XSI_MODEL_MAX_NUM	(2)         /* XSI模型支持最大数量 */
#define SVA_DET_MODEL_MAX_NUM	(1)         /* 检测模型支持最大数量 */
#define SVA_PD_MODEL_MAX_NUM	(1)         /* 分包模型支持最大数量 */
#define SVA_CLS_MODEL_MAX_NUM	(1)         /* 分类模型支持最大数量 */
#define SVA_AI_MODEL_MAX_NUM	(1)         /* AI模型支持最大数量 */
#define SVA_AI_MODEL_ID_MAX_LEN (128)       /* AI模型ID长度 */

#define SVA_MAX_ALARM_TYPE	(SVA_XSI_MAX_ALARM_TYPE + SVA_AI_MAX_ALARM_TYPE)      /* 报警危险最大类型 */
#define SVA_MAX_ALARM_NUM	(SVA_XSI_MAX_ALARM_NUM + SVA_AI_MAX_ALARM_NUM)        /* 报警危险最大个数 */

#define SVA_ERR_CODE					(0x77770000)                /* SVA模块交互接口错误码 */
#define SVA_INIT_PROC_BUSY				(0x77770001)                /* 初始化状态忙 */
#define SVA_SET_PROC_MODE_BUSY			(0x77770002)                /* 模式切换状态忙 */
#define SVA_SET_PROC_MODE_FAIL			(0x77770003)                /* 模式切换失败 */
#define SVA_SET_AI_MODEL_NAME_FAIL		(0x77770004)                /* AI模型设置名称失败 */
#define SVA_SET_AI_MODEL_MAP_FAIL		(0x77770005)                /* AI模型危险品映射失败 */
#define SVA_SET_AI_MODEL_STATUS_FAIL	(0x77770006)                /* AI模型使能状态失败 */

/* BA在离岗状态 */
#define MINOR_TYPE_BASE			0x1000
#define PIC_SMD_LEAVE_DETECT	(MINOR_TYPE_BASE + 0x11)         /* 人员离岗 */
#define PIC_SMD_ARRIVE_DETECT	(MINOR_TYPE_BASE + 0x12)         /* 人员在岗 */
#define PIC_SMD_COVER_DETECT    (MINOR_TYPE_BASE + 0x13)         /* 镜头遮挡 */
#define PIC_SMD_DHQ_DETECT      (MINOR_TYPE_BASE + 0x14)         /* 打哈欠   */
#define PIC_SMD_NLF_DETECT      (MINOR_TYPE_BASE + 0x15)         /* 未直视前方 */
#define PIC_SMD_CHAT_DETECT     (MINOR_TYPE_BASE + 0x16)         /* 侧头聊天 */
#define PIC_SMD_TIRED_DETECT    (MINOR_TYPE_BASE + 0x17)         /* 眯眼疲劳 */
#define PIC_SMD_SMOKE_DETECT    (MINOR_TYPE_BASE + 0x18)         /* 抽烟报警 */
#define PIC_SMD_PHONE_DETECT    (MINOR_TYPE_BASE + 0x19)         /* 打电话报警 */

/* 安检机成像识别特殊类 */
#define ISM_XDT_TYPE_BASE       (SVA_MAX_ALARM_TYPE + 1)        // 安检机XSP识别类起始值，包含了TIP这种特殊类型
#define ISM_TIP_TYPE            (ISM_XDT_TYPE_BASE)             // TIP，193
#define ISM_XDT_TYPE_MIN        (ISM_XDT_TYPE_BASE + 1)         // 安检机XSP识别类最小值，从可疑有机物开始，包括难穿透、爆炸物等
#define ISM_ORGNAIC_TYPE        (ISM_XDT_TYPE_MIN)              // 可疑有机物，194
#define ISM_UNPEN_TYPE          (ISM_XDT_TYPE_MIN + 1)          // 难穿透，195
#define ISM_BOMB_TYPE           (ISM_XDT_TYPE_MIN + 2)          // 爆炸物，196
#define ISM_DRUG_TYPE           (ISM_XDT_TYPE_MIN + 3)          // 毒品，197
#define ISM_XDT_TYPE_MAX        (ISM_XDT_TYPE_MIN + 3)          // 安检机XSP识别类型最大值
#define ISM_XDT_TYPE_NUM        (ISM_XDT_TYPE_MAX - ISM_XDT_TYPE_BASE + 1) // 安检机XSP识别类型总数量，包括TIP这种特殊类型

/* 安检机类型 */
typedef enum tagSvaMachineType
{
    SVA_FRCNN_500_300 = 0,
    SVA_FRCNN_650_500 = 1,
    SVA_FRCNN_800_650 = 2,
    SVA_FRCNN_1000_800 = 3,
    SVA_FRCNN_1000_1000 = 4,
    SVA_FRCNN_MACHINE_NUM,
} SVA_MACHINE_TYPE;

/*
    检测区域
 */

/*
    检测告警物体开关和灵敏度
    灵敏度 1-5
 */
/* SVA库目标类型 */
typedef enum tagSvaObjClass
{
    SVA_CLASS_BOTTLEDWATER = 0,  /* 目标类型是瓶装水 */
    SVA_CLASS_FRUITKNIFE = 1,    /* 目标类型是水果刀 */
    SVA_CLASS_HANDGUN = 2,       /* 目标类型是手枪 */
    SVA_CLASS_UMBRELLA = 3,      /* 雨伞 */
    SVA_CLASS_BATTERY = 4,       /* 电池 */
    SVA_CLASS_SCISSORS = 5,      /* 剪刀 */
    SVA_CLASS_SPRAYCAN = 6,      /* 喷罐 */
    SVA_CLASS_MOBILEPHONE = 7,   /* 手机 */
    SVA_CLASS_LAPTOPS = 8,       /* 笔记本电脑 */
    SVA_CLASS_UNKNOWN = 9,       /* 目标类型不确定（未开放） */
    SVA_CLASS_H_DENSITY = 10,    /* 目标类型高密度（未开放) */
    SVA_CLASS_NUM = 11           /* 目标类型数量（未开放) */
} SVA_OBJ_CLASS;

/* SVA目标名称OSD大小等级 */
typedef enum tagSvaOsdScaleLeave
{
    SVA_OSD_SCALE_DEF = 0,    /* 默认大小 */
    SVA_OSD_SCALE_LEAVE1 = 1, /* 原始大小 */
    SVA_OSD_SCALE_LEAVE2 = 2, /* 放大2倍 */
    SVA_OSD_SCALE_LEAVE3 = 3, /* 放大3倍 */
    SVA_OSD_SCALE_LEAVE4 = 4, /* 放大4倍 */
    SVA_OSD_SCALE_NUM = 5
} SVA_OSD_SCALE_LEAVE;

/* 安检机运动方向类型 */
typedef enum tagSvaOrientationType
{
    SVA_ORIENTATION_TYPE_R2L = 0,        /*  画面由右向左运动（目前只支持该方向） */
    SVA_ORIENTATION_TYPE_L2R = 1,        /*  画面由左向右运动（预留参数，当前版本无效） */
    SVA_ORIENTATION_TYPE_NUM
} SVA_ORIENTATION_TYPE;

typedef enum tagSvaSizeFilterType
{
    TARGET_MIN_SIZE_FILTER = 0,
    TARGET_MAX_SIZE_FILTER,
    TARGET_SIZE_FILTER_TYPE_NUM,
} SVA_TARGET_SIZE_FILTER_TYPE_E;

/*
   对接AI开放平台相关
 */

/* 算法模型种类 */
typedef enum tagSvaModelType
{
    /* 检测模型 */
	SVA_DET_MODEL = 0,
	/* 分包模型 */
	SVA_PD_MODEL = 1,
	/* 分类模型 */
	SVA_CLS_MODEL = 2,
	/* AI模型 */
    SVA_AI_MODEL = 3,
    
    SVA_MODEL_TYPE_NUM,
} SVA_MODEL_TYPE_E;

typedef struct tagSvaDetModelAttr
{
	/* 主视角检测模型数量，当前默认为1 */
    unsigned int uiMainModelNum;
	/* 主视角检测模型对应的名称，字符串最大64 */
    unsigned char chMainModelName[SVA_DET_MODEL_MAX_NUM][SVA_MODEL_NAME_MAX_LEN];
	
	/* 侧视角检测模型数量，当前默认为1 */
    unsigned int uiSideModelNum;
	/* 侧视角检测模型对应的名称，字符串最大64 */
    unsigned char chSideModelName[SVA_DET_MODEL_MAX_NUM][SVA_MODEL_NAME_MAX_LEN];
} SVA_DET_MODEL_ATTR_S;

typedef struct tagSvaPdModelAttr
{
	/* 主视角分包模型数量，当前默认为1 */
    unsigned int uiMainModelNum;
	/* 主视角分包模型对应的名称，字符串最大64 */
    unsigned char chMainModelName[SVA_PD_MODEL_MAX_NUM][SVA_MODEL_NAME_MAX_LEN];
	
	/* 侧视角分包模型数量，当前默认为1 */
    unsigned int uiSideModelNum;
	/* 侧视角分包模型对应的名称，字符串最大64 */
    unsigned char chSideModelName[SVA_PD_MODEL_MAX_NUM][SVA_MODEL_NAME_MAX_LEN];
} SVA_PD_MODEL_ATTR_S;

typedef struct tagSvaClsModelAttr
{
	/* 分类模型数量，当前默认为1 */
	unsigned int uiModelNum;
	/* 分类模型对应的名称，字符串最大64 */
	unsigned char chName[SVA_CLS_MODEL_MAX_NUM][SVA_MODEL_NAME_MAX_LEN];
} SVA_CLS_MODEL_ATTR_S;

typedef struct tagSvaAiModelAttr
{
	/* 主视角AI模型数量，当前默认为1 */
    unsigned int uiMainModelNum;
	/* 主视角AI模型对应的名称，字符串最大64 */
    unsigned char chMainModelName[SVA_AI_MODEL_MAX_NUM][SVA_MODEL_NAME_MAX_LEN];

	/* 侧视角AI模型数量，当前默认为1 */
	unsigned int uiSideModelNum;
	/* 侧视角AI模型对应的名称，字符串最大64 */
	unsigned char chSideModelName[SVA_AI_MODEL_MAX_NUM][SVA_MODEL_NAME_MAX_LEN];
} SVA_AI_MODEL_ATTR_S;

typedef union tagSvaModelAttr
{
	/* 检测模型参数 */
	SVA_DET_MODEL_ATTR_S stDetModelAttr;
	/* 分包模型参数 */
	SVA_PD_MODEL_ATTR_S stPdModelAttr;
	/* 分类模型参数 */
	SVA_CLS_MODEL_ATTR_S stClsModelAttr;
	/* AI模型参数 */
	SVA_AI_MODEL_ATTR_S stAiModelAttr;
} SVA_MODEL_ATTR_U;

typedef struct tagSvaModelTypeAttr
{
	/* 模型类别 */
    SVA_MODEL_TYPE_E enModelType;
	/* 该类别的模型下发属性 */
	SVA_MODEL_ATTR_U uModelAttr;
} SVA_MODEL_ATTR_S;

typedef struct tagSvaModelPrm
{
	/* 待配置的模型种类数目 */
    unsigned int uiModelTypeNum;                                        
	/* 配置的特定种类模型相关信息 */
    SVA_MODEL_ATTR_S astModelTypeAttr[SVA_MODEL_TYPE_NUM];
} SVA_MODEL_PARAM_S;

/*
    检测区域
 */
typedef enum tagSvaBorderType
{
    SVA_OSD_NORMAL_TYPE = 0,           /* OSD画在目标框上显示 */
    SVA_OSD_LINE_RECT_TYPE = 1,        /* OSD通过一根线上拉到画面上下部分显示，目标使用矩形进行框选 */
    SVA_OSD_LINE_POINT_TYPE = 2,       /* OSD通过一根线上拉到画面上下部分显示，目标使用6x6的点进行标注 */
    SVA_OSD_TYPE_NO_LINE_TYPE = 3,     /* OSD在画面上下部显示，危险物框选 */
    SVA_OSD_TYPE_CROSS_NO_RECT_TYPE = 4,    /* OSD在包裹上方展示，危险品不框选，需要目标个数过滤 */
    SVA_OSD_TYPE_CROSS_RECT_TYPE = 5,       /* OSD在包裹上方展示，危险品框选，边框2像素，需要目标个数过滤 */
    SVA_OSD_TYPE_NUM,                      /* 当前支持配置的OSD显示类型数量 */
} SVA_BORDER_TYPE_E;

typedef enum tagSvaAlertLevelType
{
    SVA_ALERT_LEVEL_SAFE = 0,         /* 危险级别安全，该级别仅用于包裹显示 */
    SVA_ALERT_LEVEL_4,                /* 危险级别4 */
    SVA_ALERT_LEVEL_3,                /* 危险级别3 */
    SVA_ALERT_LEVEL_2,                /* 危险级别2 */
    SVA_ALERT_LEVEL_1,                /* 危险级别1，危险等级与数序相反，等级1最危险 */
    SVA_ALERT_LEVEL_NUM,              /* 当前支持配置的危险品等级个数 */
} SVA_ALERT_LEVEL_E;

typedef enum tagSvaAlertExtType
{
    SVA_ALERT_EXT_NONE = 0,           /* 危险品OSD不叠加拓展信息 */
    SVA_ALERT_EXT_PERCENT,            /* 危险品OSD叠加百分比 */
    /* SVA_ALERT_EXT_LEVEL,              / * 危险品OSD叠加危险等级 * / */
    SVA_ALERT_EXT_TYPE_NUM,
} SVA_ALERT_EXT_TYPE;

typedef struct tagSvaRectF
{
    float x;                           /* 矩形左上角X轴坐标 */
    float y;                           /* 矩形左上角Y轴坐标 */
    float width;                       /* 矩形宽度 */
    float height;                      /* 矩形高度 */
} SVA_RECT_F;

typedef struct tagSvaAlertKey
{
    unsigned int sva_cnt;                                  /* 危险品个数 */
    unsigned int sva_type[SVA_MAX_ALARM_TYPE];             /* 检测危险物类型 */
    unsigned int sva_key[SVA_MAX_ALARM_TYPE];              /* 检测开关 */
} SVA_ALERT_KEY;

typedef struct tagSvaAlertSensity
{
    unsigned int sva_cnt;                                  /* 危险品个数 */
    unsigned int sva_type[SVA_MAX_ALARM_TYPE];             /* 检测危险物类型 */
    unsigned int sva_conf[SVA_MAX_ALARM_TYPE];			   /* 置信度，支持0~100闭区间配置 */
} SVA_ALERT_CONF;

typedef struct tagSvaAlertColor
{
    unsigned int sva_cnt;                                  /* 危险品个数 */
    unsigned int sva_type[SVA_MAX_ALARM_TYPE];             /* 检测危险物类型 */
    unsigned int sva_color[SVA_MAX_ALARM_TYPE];            /* 画框颜色,也可以在显示部分做 */
} SVA_ALERT_COLOR;

typedef struct tagSvaAlertName
{
    unsigned int sva_cnt;                                           /* 危险品个数 */
    ENCODING_FORMAT_E enEncFormat;                                  /* sva_name的编码类型 */
    unsigned int sva_type[SVA_MAX_ALARM_TYPE];                      /* 检测危险物类型 */
    unsigned char sva_name[SVA_MAX_ALARM_TYPE][SVA_ALERT_NAME_LEN]; /* 待设置的危险品名称 */
} SVA_ALERT_NAME;

typedef struct tagSvaAlertTargetCnt
{
    unsigned int sva_cnt;                                   /* 危险品个数 */
    unsigned int sva_type[SVA_MAX_ALARM_TYPE];              /* 检测危险物告警个数阈值 */
    unsigned int sva_alert_tar_cnt[SVA_MAX_ALARM_TYPE];     /* 危险品个数，达到设定阈值进行告警 */
} SVA_ALERT_TARGET_CNT;

#if 1  /* 包裹等级相关配置，暂不使用 */
typedef struct tagSvaAlertLevel
{
    unsigned int sva_cnt;                                  /* 危险品个数 */
    unsigned int sva_type[SVA_MAX_ALARM_TYPE];             /* 检测危险物类型 */
    SVA_ALERT_LEVEL_E sva_level[SVA_MAX_ALARM_TYPE];       /* 危险品等级 */
} SVA_ALERT_LEVEL;

typedef struct tagSvaAlertPkgPrm
{
    unsigned int pkg_level_cnt;
    unsigned int pkg_color[SVA_ALERT_LEVEL_NUM];            /* 各种危险等级的包裹颜色 */
    /* unsigned char pkg_level_name[SVA_ALERT_LEVEL_NUM];      / * 各种危险等级的包裹级别名称: 目前写死为低、中、高、安全 * / */
} SVA_ALERT_PKG_PRM;
#endif

/* 目标尺寸过滤配置相关 */
typedef struct tagSvaSizeFilterRect
{
    unsigned int uiW;                                /* 矩形的宽，像素单位 */
    unsigned int uiH;                                /* 矩形的高，像素单位 */
} SVA_ALERT_SIZE_RECT_S;

typedef struct tagSvaPackgeSplitThr
{
    unsigned int uiSplitSwitch;                     /* 小包裹检测开关配置0-关 1-开 默认关闭*/                     
	unsigned int uiW;                               /* 小包裹分割阈值宽度设置*/
	unsigned int uiH;                               /* 小包裹分割阈值高度设置*/
}SVA_PACKAGE_SPLIT_THE;

typedef struct tagSvaSizeFilterDiag
{
    unsigned int uiDiagLen;                          /* 对角线长度，像素单位 */
} SVA_ALERT_SIZE_DIAG_S;

typedef union tagSvaFilterSizeInfo
{
    SVA_ALERT_SIZE_RECT_S stRectInfo;                /* 矩形信息 */
    SVA_ALERT_SIZE_DIAG_S stDiagInfo;                /* 对角线信息 */
} SVA_ALERT_FILTER_SIZE_INFO_U;

typedef struct tagSvaTarFilterInfo
{
    unsigned int uiType;
    SVA_ALERT_FILTER_SIZE_INFO_U uFilterSizeInfo;
} SVA_ALERT_FILTER_INFO_S;

typedef struct tagSvaFilterInfo
{
    unsigned int uiCnt;                                          /* 待配置的目标类型个数 */
    SVA_ALERT_FILTER_INFO_S stFilterInfo[SVA_MAX_ALARM_TYPE];      /* 特定目标的过滤尺寸信息 */
} SVA_SIZE_FILTER_CONFIG_INFO_S;

typedef struct tagSvaSizeFilterCfg
{
    SVA_TARGET_SIZE_FILTER_TYPE_E enType;            /* 尺寸过滤类型，目前仅有两种: 最小尺寸(宽+高)和最大尺寸(对角线) */
    SVA_SIZE_FILTER_CONFIG_INFO_S stFilterCfgInfo;   /* 对应尺寸过滤类型的参数信息，需要注意最小尺寸使用宽高，最大尺寸使用对角线长度 */
} SVA_ALERT_SIZE_FILTER_CONFIG_S;

typedef struct tagSvaSizeFilterPrm
{
    unsigned int uiPrmCnt;                           /* 本次调用下发的参数个数，受到过滤类型数量的限制，参考TARGET_SIZE_FILTER_TYPE_NUM */
    SVA_ALERT_SIZE_FILTER_CONFIG_S stSizeFilterCfg[TARGET_SIZE_FILTER_TYPE_NUM];      /* 对应上述个数的具体配置参数 */
    unsigned int reserved[4];
} SVA_ALERT_SIZE_FILTER_PRM_S;

typedef struct _SVA_FORCE_SPLIT_PRM_
{
	/* 包裹强制分割开关，有效值为0或1。0表示关闭，1表示开启 */
	unsigned int u32Flag;
	/* 暂未开放: 满足强制分包的帧间隔，有效范围[60, 300]，默认值300 */
	//unsigned int u32GapFrmCnt;
} SVA_FORCE_SPLIT_PRM_S;

/***************************** OUT *****************************************************/
/* XSI报警目标 */
typedef struct tagSvaDspAlert
{
    unsigned int ID;                                            /* ID */
    unsigned int type;                                          /* 目标类型 */
    unsigned int alarm_flg;                                     /* 目标的报警标志位，TRUE为报警，FALSE为不报警 */
    unsigned int color;                                         /* 画框颜色,也可以在显示部分做。 */
    unsigned int confidence;                                    /* 目标置信度 */
    unsigned int visual_confidence;                             /* 视觉置信度，用于客户端显示置信度。目前初定 visual_confidence = confidence/2.0+0.50.  (0-1000) */
    SVA_RECT_F rect;                                            /* 目标框 */
} SVA_DSP_ALERT;

/* XSI包裹报警信息 */
typedef struct  tagSvaDspAlertPack
{
    unsigned int package_valid;                                  /* 包裹是否有效，1：有效，0：无效 */
    unsigned int candidate_flag;                                 /*  记录当前帧是否可能是候选正样本（一般可能存在刀枪等罕见样本）,用于样本回收。 */
    unsigned int pkg_force_split_flag;                           /* 包裹强制分割标志 */
    unsigned int split_group_start_flag;                         /* 分片开启标记 */
    unsigned int split_group_end_flag;                           /* 当前valid是否为分片序列的结束 */
	unsigned int package_id;                                     /* 包裹id，用于集中判图模式匹配同一个包裹分片序列 */
	unsigned int package_match_id;                               /* 包裹匹配id，用于匹配双视角包裹 */
    SVA_RECT_F package_loc;                                      /* 包裹位置 */
} SVA_DSP_SVA_PACK;

/* 智能分包误检标志位，用于误检包裹统计 */
typedef struct tagSvaDspFalseDetPkgOut
{
	/* 包裹id */
	unsigned int uiPkgId;
} SVA_DSP_FALSE_DET_PKG_OUT_S;

/* 智能处理输出信息 */
typedef struct tagSvaDspOut
{
    unsigned int frame_num;                                         /* 帧号 */
    SVA_DSP_SVA_PACK packbagAlert;                                  /* 包裹报警信息 */
    float fPkgUpGapPixel;                                           /* 完整包裹上边沿到分片上边沿的归一化坐标，相对于分片高度 */
    float fPkgDownGapPixel;                                         /* 完整包裹下边沿到分片下边沿的归一化坐标，相对于分片高度 */
    unsigned int target_num;                                        /* 报警数 */
    SVA_DSP_ALERT target[SVA_XSI_MAX_ALARM_NUM];                    /* 报警目标  128个 */
    unsigned int jpeg_cnt;                                          /* 回调jpeg个数 */
    JPEG_SNAP_CB_RESULT_ST stJpegResult[SVA_MAX_CALLBACK_JPEG_NUM]; /* 回调jpeg图片信息，0存放不叠框完整包裹，1存放分片，2存放叠框完整包裹 */
    unsigned int bmp_cnt;                                           /* 回调bmp个数 */
    BMP_RESULT_ST stBmpResult[SVA_MAX_CALLBACK_BMP_NUM];            /* 回调bmp图片信息 */
    unsigned long framePts;                                         /* 帧pts (单位ms) */
    JPEG_PKG_TIME_S stJpegPkgTime;
} SVA_DSP_OUT;

/* 智能处理输出信息 */
typedef struct tagSvaDspPosOut
{
	/* pos data */
    char *pcPosData;
	/* pos len */
	unsigned int uiDataLen;
} SVA_DSP_POS_OUT;

typedef struct
{
    unsigned int sva_type;          /* 危险品类型，与xml中id值对应，代替原有的枚举定义 */
    unsigned int sva_key;           /* 是否开启叠框 */
    unsigned int sva_color;         /* 画框颜色 */
    unsigned char sva_name[SVA_ALERT_NAME_LEN];      /* 标定危险品名称，从xml中获取，加上前缀”疑似XXXX” */
} SVA_DSP_DET_INFO;

typedef enum tagSvaConfMode
{
    SVA_CONF_MODE_NORMAL = 0,           /* 普通模式下默认置信度 */
    SVA_CONF_MODE_PK,                   /* pk模式下默认置信度 */
    SVA_CONF_MODE_NUM,                  /* 获取默认置信度(灵敏度)的模式个数 */
} SVA_CONF_MODE_E;

/* 违禁品默认置信度结构体 */
typedef struct
{
	SVA_CONF_MODE_E enConfMode;               /* 获取默认置信度的模式，1是pk模式，0是普通模式 */
    int s32DefConf[SVA_XSI_MAX_ALARM_TYPE];   /* 每种危险品的置信度，目前返回支持的所有基础违禁品类别(部分为0)，不包含AI目标 */
} SVA_DSP_DEF_CONF_OUT;

typedef enum tagSvaProcMode
{
    SVA_PROC_MODE_IMAGE  = 0,              /* 图像模式，支持单路和双路 */
    SVA_PROC_MODE_VIDEO = 1,               /* 视频模式，支持单路 */
    SVA_PROC_MODE_DUAL_CORRELATION = 2,    /* 双通道关联模式，两个通道必须都输入数据 */
    SVA_PROC_MODE_PACK_DIV = 3,            /* 包裹分割模式 */
    SVA_PROC_MODE_NUM = 4,
} SVA_PROC_MODE_E;

typedef struct tagSvaVcaePrm
{
    unsigned int uiAiEnable;              /* Ai模型是否使能 */
    SVA_PROC_MODE_E enProcMode;           /* 处理模式 */
    unsigned int uiReserved[0];           /* 预留 */
} SVA_VCAE_PRM_ST;

typedef struct
{
    unsigned int uiCnt;                             /* 当前实际支持的危险品种类个数 */
    ENCODING_FORMAT_E enEncFormat;                  /* 字符串编码类型 */
    SVA_DSP_DET_INFO det_info[SVA_MAX_ALARM_TYPE];  /* 危险品信息结构体，最大支持xsi模型64个，ai模型32个 */
} SVA_DSP_DET_STRU;

typedef struct tagSvaAiModelPrm
{	
	unsigned int uiInputChan[2];				    /* AI模型输入通道号, chan[0]=1:开启通道0，chan[0]=0:关闭通道0， */
	unsigned int uiModelEnable;						/* AI模型开关标志位 */
	unsigned int AiModelUseFlag;					/* AI模型名称和映射使用开关 */
	SVA_MODEL_PARAM_S stchName;						/* AI模型名称 */
    SVA_DSP_DET_STRU stAiMap;						/* AI模型映射 */
}SVA_SET_AIMODEL_PRM;

typedef struct tagSvaVcaeAiModelPrm
{
    unsigned int uiAiEnableFlag;				    /* AI模型开关标志位 */
    SVA_MODEL_PARAM_S stchName;						/* AI模型名称 */
    SVA_DSP_DET_STRU stAiMap;						/* AI模型映射 */
}SVA_VCAE_AIMODEL_PRM_ST;

typedef struct tagSvaDspEngModePrm
{
    unsigned int uiBaseType;                        /* 0:切换关联模式；1：绑定解绑ai模型 */
    unsigned int uiChanFlag[2];				        /* 输入通道号, chan[0]=1:开启通道0，chan[0]=0:关闭通道0 */
    SVA_VCAE_PRM_ST stProcModePrm;                  /* 切换模式参数 */
    SVA_VCAE_AIMODEL_PRM_ST stAiModePrm;            /* AI模型参数 */
}SVA_DSP_ENG_MODE_PARAM;

/****************************************************************************/
/*                                  行为分析                                */
/****************************************************************************/
/* 行为检测报警目标信息 */
typedef struct tagBaDspAlert
{
    unsigned int confidence;                                    /* 行为检测目标置信度 */
    SVA_RECT_F rect;                                            /* 目标框 */
} BA_DSP_TARGET;

typedef enum _BA_ABNOR_BEHAV_LEVEL_
{
    BA_DETECT_LEVEL_LOW = 0,                                    /* 异常行为检测级别: 低 */
	BA_DETECT_LEVEL_MEDIUM,                                     /* 异常行为检测级别: 中 */
	BA_DETECT_LEVEL_HIGH,                                       /* 异常行为检测级别: 高 */
	BA_DETECT_LEVEL_NUM
} BA_ABNOR_BEHAV_LEVEL_E;

/* 行为检测报警输出信息 */
typedef struct tagBaDspOut
{
    unsigned int ba_alert;                                      /* 行为检测是否触发报警 */
    unsigned int ba_alert_mode;                                 /* 行为检测报警状态(在岗:0x1012，离岗:0x1010) */
    unsigned int ba_target_num;                                 /* 行为检测报警数 */
    BA_DSP_TARGET ba_target[10];                                /* 行为检测目标信息，暂定单张图中最多有10个报警信息 */
    JPEG_SNAP_CB_RESULT_ST ba_jpeg_result;
} BA_DSP_OUT;

/****************************************************************************/
/*                                人脸抓拍                              */
/****************************************************************************/

/*人脸属性输出*/
typedef struct tagFaceAttributeOut
{
    int face_glass;              // 眼镜, 0表示无眼镜, 1表示戴眼镜, 2表示带墨镜, -1表示未知
    int face_mask;              // 口罩, 0表示无口罩, 1表示戴口罩, -1表示未知
    int face_hat;               // 帽子, 0表示无帽子,1表示戴帽子, -1表示未知
    int face_beard;             // 涉及隐私不使用。胡须, 0表示无/小胡子, 1表示大胡须, -1表示未知（针对新疆）
    int face_gender;            // 性别, 0表示女, 1表示男, -1表示未知
    int face_race;              // 涉及隐私不使用。种族, 0表示亚洲人, 1表示白人, 2表示黑人, -1表示未知
    int face_nation;           // 涉及隐私不使用。民族, 0表示汉族, 1表示新疆人, -1表示未知
    int face_experess;        // 表情, 0表示中性, 1高兴, 2惊讶, 3表示害怕, 4表示厌恶, 5表示难过, 6表示愤怒, -1表示未知
    int face_age;              // 年龄, 范围0~99, 小于1周岁的都认为0岁, 大于等于99岁都认为99岁
    int face_age_satge;       // 年龄段, 0 表示儿童 0-6岁,  1表示少年 7-17岁, 2表示青年 18-40岁, 3表示中年 41-65岁, 4表示老年 >=66岁
    int face_hair_style;      // 发型, 预留
    int face_shap;            // 脸型, 预留
}FACE_ATTRIBUTE_DSP_OUT;

/*人脸抓拍输出信息*/
typedef struct tagFaceCapDspOut
{   
    unsigned long long ullTimePts;                                    /* 回调的同步时间戳 */
    unsigned int face_num;                                            /*抓拍人脸个数*/
	char face_feature_data[MAX_CAP_FACE_NUM][FACE_FEATURE_LENGTH];    /*人脸特征数据*/
	FACE_ATTRIBUTE_DSP_OUT face_attrubute[MAX_CAP_FACE_NUM];          /*抓拍人脸属性输出*/
	JPEG_SNAP_CB_RESULT_ST face_result[MAX_CAP_FACE_NUM];             /*回调人脸jpg数据,抠图数据,最多3张*/	
	JPEG_SNAP_CB_RESULT_ST face_background_result;                    /*回调人脸jpg数据,背景数据,只有一张*/
}FACE_CAP_DSP_OUT;

/****************************************************************************/
/*                              安检人脸智能                                */
/****************************************************************************/
/* 配置参数通道 */
typedef enum tagDspFaceConfigChannel
{
    FACE_LOGIN_CHAN =0,/*登录通道参数配置*/
	FACE_CAP_CHAN =1, /*抓拍通道参数配置*/
}FACE_DSP_CONFIG_CHANNEL_TYPE_E;

/* 配置参数类型 */
typedef enum tagDspFaceConfigType
{
    FACE_DETECT_THRESHOLD = 0,
    FACE_DETECT_REGION = 1,
    FACE_CONFIG_TYPE_NUM = 2,
} FACE_DSP_CONFIG_TYPE_E;

/* 配置参数结构体 */
typedef union tagDspFaceConfigParam
{
    float fFdThreshold;   /* 人脸抓拍阈值 */
    SVA_RECT_F stRegion;          /* 人脸检测区域 */
    void *reserved;
} FACE_DSP_CONFIG_PARAM_U;

typedef struct tagDspFaceConfigData
{
    FACE_DSP_CONFIG_CHANNEL_TYPE_E chanType; /*配置参数通道*/
    FACE_DSP_CONFIG_TYPE_E enType;        /* 配置参数类型 */
    FACE_DSP_CONFIG_PARAM_U stCfgParam;   /* 配置参数 */
} FACE_DSP_CONFIG_DATA;

/* 注册使用的信息 */
typedef struct tagDspFaceRegJpgParam
{
    unsigned int uiLength;        /* 人脸数据长度 */
    unsigned int uiWidth;        /* 图片宽 */
    unsigned int uiHeight;        /* 图片高 */
    unsigned char *pAddr;       /* 图片地址 */
    void *reserved;           /* 保留数据，无用则删除 */
} FACE_DSP_REGISTER_JPG_PARAM;

/* 数据库人脸信息*/
typedef enum tagDspFaceRegRsltFlag
{
    FACE_REGISTER_FAIL = 0,                          /* 注册失败 */
    FACE_REGISTER_SUCCESS = 1,                       /* 注册成功 */
    FACE_REGISTER_REPEAT = 2,                        /* 重复注册 */
    FACE_REGISTER_OVERSIZE = 3,                      /* 注册使用的人脸分辨率超过1080P */
} FACE_DSP_REG_RESULT_FLAG;

typedef struct tagDspFaceDatabaseParam
{
    unsigned int uiFaceId;                            /* 人脸id */
    unsigned int uiRepeatId;                          /* 重复人脸id */
    FACE_DSP_REG_RESULT_FLAG bFlag;                   /* 人脸模型是否创建成功 */
    unsigned char *pHeader;                           /* 数据头，若不需要使用则删除 */
    unsigned char Featdata[FACE_FEATURE_LENGTH];      /* 人脸特征值 FACE_FEATURE_DATA_LENGTH*/
	unsigned int dataLen;                             /*人脸特征值长度*/
    void *reversed;                                   /* 待后续联调确认补充，不需要则删除 */
} FACE_DSP_DATABASE_PARAM;

/* 人脸注册 */
typedef struct tagDspFaceRegParam
{
    unsigned int uiRegCnt;                                /* 待注册人脸个数 */
    FACE_DSP_DATABASE_PARAM stFeatData[MAX_FACE_NUM];     /* 建模数据 */
    FACE_DSP_REGISTER_JPG_PARAM stJpegData[MAX_FACE_NUM]; /* 建模使用的jpg数据 */
    void *reserved;                                       /* 保留数据，无用则删除 */
} FACE_DSP_REGISTER_PARAM;

/*人脸通道参数*/
typedef struct tagDspFaceCannParam
{
   unsigned int uiFaceLoginChan;
   unsigned int uiFaceCapChan;
}FACE_DSP_CHANNEL_NUM_PARAM;

/* 更新本地人脸数据库*/
typedef struct tagFaceDspUpdateParam
{
    unsigned int uiModelCnt;                          /* 数据库中人脸个数，小于等于50 */
    FACE_DSP_DATABASE_PARAM stFeatData[MAX_FACE_NUM]; /* 目前仅包括人脸ID，特征值信息 */
    void *reversed;                                   /* 待后续联调确认补充，不需要则删除 */
} FACE_DSP_UPDATE_PARAM;

/* 人脸登录输入参数 */
typedef struct tagFaceDspLoginInParam
{
    void *reversed;                                   /* 待后续联调确认补充，不需要则删除 */
} FACE_DSP_LOGIN_INPUT_PARAM;

/* 人脸登录输出参数 */
typedef struct tagFaceDspLoginOutParam
{
    unsigned int bSuccess;                            /* 是否识别成功 */
    unsigned int uiFaceId;                            /* 数据库中的人脸ID */
    void *reversed;                                   /* 待后续联调确认补充，不需要则删除 */
} FACE_DSP_LOGIN_OUTPUT_PARAM;

typedef struct tagFaceDspLoginParam
{
    FACE_DSP_LOGIN_INPUT_PARAM stInputData;            /* 应用传入数据，目前只有解码通道 */
    FACE_DSP_LOGIN_OUTPUT_PARAM stOutputData;          /* 返回应用数据，目前只有人脸ID */
    void *reversed;                                    /* 待后续联调确认补充，不需要则删除 */
} FACE_DSP_LOGIN_PARAM;

/* 更新人脸比对库 */
typedef struct tagDspFaceLibParam
{
    void *reserved;        /* 保留字段 */
} FACE_DSP_UPDATE_DFRLIB_PARAM;

/* 保留回调接口部分 */
typedef struct tagFaceDspOut
{
    JPEG_SNAP_CB_RESULT_ST stFaceJpegResult;
    void *reserved;
} FACE_DSP_OUT;

/*人脸比对接口数据*/
typedef struct tagFaceCompareParam
{
    char stTestFace[FACE_FEATURE_LENGTH];   //待测试人脸Feature数据
	char stLibFace[FACE_FEATURE_LENGTH];    //人脸数据库中的一个人脸数据
	unsigned int length;                   //数据长度
	float fScore;                         //返回的比对相似度值
}FACE_COMPARE_DATA;

/*人脸比对版本信息接口数据*/
typedef struct tagFaceCmpVerData
{
    unsigned int bCheckRst;                 //人脸比对结果  0--人脸建模数据不支持比对 1--人脸建模数据支持比对
    char stTestFace[FACE_FEATURE_LENGTH];   //待测试人脸Feature数据
}FACE_COMPARE_CHECK_DATA;

/**
 *    人包关联
 */
#define PPM_MAX_JPEG_NUM			(2)
#define PPM_MAX_OBJ_NUM				(16)
#define PPM_MAX_REGION_NUM			(3)              /* 支持配置的最大区域个数，参考REGION_TYPE_NUM */
#define PPM_MAX_SUB_RGN_NUM			(2)              /* 单一类型区域支持的最大子区域个数 */
#define PPM_MAX_ARRAY_SIZE			(8)              /* 数据最大深度 */
#define PPM_MAX_CALIB_JPEG_NUM		(8)              /* 标定截图最大组个数(一张三目截图+一张人脸截图为一组) */
#define PPM_MAX_FACE_CALIB_JPEG_NUM (9)              /* 人脸相机标定截图最大组个数 */

#define PPM_ERR_CODE					(0x77780000)                        /* PPM模块交互接口错误码 */
#define PPM_GET_MATRIX_INFO_ERR			(0x77780001)                        /* 获取矩阵信息失败 */
#define PPM_GET_CALIB_BOARD_INFO_ERR	(0x77780002)                        /* 获取标定板信息失败 */
#define PPM_GET_CALIB_PCIINFO_ERR		(0x77780003)                        /* 获取标定图片信息失败 */
#define PPM_INPUT_DATA_ERR				(0x77780004)                        /* 输入数据失败 */
#define PPM_SYNC_PROC_ERR				(0x77780005)                        /* 同步模式失败 */
#define PPM_GET_RMS_ERR					(0x77780006)                        /* 算法RMS值异常导致失败 */
#define PPM_INVALID_REGION_PARAM		(0x77780007)                        /* 检测区域参数非法 */
#define PPM_SET_FACE_CAP_REGION_FAIL	(0x77780008)                        /* 人脸抓拍区域配置失败 */
#define PPM_SET_CAP_AB_REGION_FAIL		(0x77780009)                        /* 抓拍AB区域配置失败 */
#define PPM_SET_OPTICAL_PKG_REGION_FAIL (0x7778000a)                        /* 可见光包裹区域配置失败 */

/* 图像尺寸不匹配 */
#define PPM_MCC_E_IMG_SIZE		(0x7778000b)                            /* 图像尺寸不匹配 */
#define PPM_MCC_E_NULL_POINT	(0x7778000c)                            /* 空指针 */
#define PPM_MCC_E_IMG_NUM		(0x7778000d)                            /* 图像个数错误（[1, MCC_MAX_IMG_NUM]） */
#define PPM_MCC_E_PROC_IN_TYEP	(0x7778000e)                            /* 输入参数类型错误 */
#define PPM_MCC_E_PROC_OUT_TYEP (0x7778000f)                            /* 输出参数类型错误 */
#define PPM_MCC_E_CALIB_IMG_NUM (0x77780010)                            /* 缺少有效的标定板图像 */

/* 区域信息 */
typedef struct _PPM_REGION_INFO_
{
    unsigned int uiSubRgnCnt;                        /* 子区域个数，该参数目前只有CAP_REGION时配置为2，其余为1 */
    SVA_RECT_F stRgnRect[PPM_MAX_SUB_RGN_NUM];       /* 区域信息 */
} PPM_REGION_INFO_S;

/* X光包裹匹配参数，保留: 为RK新方案预留安检机相关参数 */
typedef struct _PPM_XSI_MATCH_PRM_
{
    float fBeltCenterDist;                            /* 传送带中心距离，m */
    float fBeltMovSpeed;                              /* 传送带移动速度，m/s */
} PPM_XSI_MATCH_PRM_S;

/* 匹配检测灵敏度 */
typedef enum _PPM_MATCH_SENSITY_ 
{
    SENSITY_MATCH_LEVEL_STEP1 = 0,                   /* 灵敏度最低等级 */
    SENSITY_MATCH_LEVEL_STEP2,                       /* 灵敏度偏中等级 */
    SENSITY_MATCH_LEVEL_STEP3,                       /* 灵敏度中等等级 */
    SENSITY_MATCH_LEVEL_STEP4,                       /* 灵敏度偏高等级 */
    SENSITY_MATCH_LEVEL_STEP5,                       /* 灵敏度最高等级 */
    SENSITY_LEVEL_NUM,
} PPM_MATCH_SENSITY_E;

/* 人脸RT矩阵参数 */
typedef struct _PPM_MATRIX_PRM_
{
    unsigned int uiRowCnt;
    unsigned int uiColCnt;
    double fMatrix[PPM_MAX_ARRAY_SIZE][PPM_MAX_ARRAY_SIZE];
} PPM_MATRIX_PRM_S;

typedef struct _PPM_MATCH_MATRIX_INFO_
{
    float fCamHeight;                                    /* 三目相机架设高度 */
    PPM_MATRIX_PRM_S stR_MatrixInfo;                     /* R矩阵参数, 3x3 */
    PPM_MATRIX_PRM_S stT_MatrixInfo;                     /* T矩阵参数, 1x3 */
    PPM_MATRIX_PRM_S stFaceInterMatrixInfo;              /* 人脸相机内参，3x3 */
    PPM_MATRIX_PRM_S stFaceDistMatrixInfo;               /* 人脸相机畸变矫正矩阵，1x8 */
    PPM_MATRIX_PRM_S stTriCamInvMatrixInfo;              /* 三目相机内参逆矩阵，3x3 */
} PPM_MATCH_MATRIX_INFO_S;

typedef struct _PPM_FACE_CALIB_MATRIX_PRM_
{
    PPM_MATRIX_PRM_S stCamInterPrm;                      /* 人脸相机内参矩阵, 3x3 */
    PPM_MATRIX_PRM_S stCamDistortMatrix;                 /* 人脸相机矫正参数矩阵，1x8 */
} PPM_FACE_CALIB_MATRIX_PRM_S;

typedef struct _PPM_TRI_CALIB_MATRIX_PRM_
{
    PPM_MATRIX_PRM_S stCamInterPrm;                      /* 三目相机内参矩阵, 3x3 */
    PPM_MATRIX_PRM_S stCamDistortMatrix;                 /* 三目相机矫正参数矩阵，1x8 */
} PPM_TRI_CALIB_MATRIX_PRM_S;

typedef struct _PPM_CALIB_PIC_ATTR_
{
    char *pcJpegAddr;
    unsigned int uiPicLen;
} PPM_CALIB_PIC_ATTR_S;

typedef struct _PPM_CALIB_PIC_PRM_
{
    PPM_CALIB_PIC_ATTR_S stPicAttr[PPM_MAX_CALIB_JPEG_NUM];   /* 标定截图参数 */
} PPM_CALIB_PIC_PRM_S;

typedef struct _PPM_FACE_CALIB_PIC_PRM_
{
    PPM_CALIB_PIC_ATTR_S stPicAttr[PPM_MAX_FACE_CALIB_JPEG_NUM]; /* 人脸相机标定截图参数 */
} PPM_FACE_CALIB_PIC_PRM_S;

typedef struct _PPM_CALIB_PIC_INFO_
{
    unsigned int uiJpegCnt;                              /* 截图个数 */
    unsigned int uiJpegW;                                /* 图片宽 */
    unsigned int uiJpegH;                                /* 图片高 */

    PPM_CALIB_PIC_PRM_S stTriCamCalibJpeg;               /* 三目相机标定截图，与人脸相机的标定截图个数需要一致 */
    PPM_CALIB_PIC_PRM_S stFaceCamCalibJpeg;              /* 人脸相机标定截图 */
} PPM_CALIB_PIC_INFO_S;

typedef struct _PPM_FACE_CALIB_PIC_INFO_
{
    unsigned int uiJpegCnt;                              /* 截图个数 */
    unsigned int uiJpegW;                                /* 图片宽 */
    unsigned int uiJpegH;                                /* 图片高 */

    PPM_FACE_CALIB_PIC_PRM_S stFaceCamCalibJpeg;         /* 人脸相机标定截图 */
} PPM_FACE_CALIB_PIC_INFO_S;

typedef struct _PPM_FACE_CAM_INFO_
{
    unsigned int uiFocalLength;                           /* 相机焦距 */
    float pixelSize;                                      /* 像元尺寸, 单位:um */
} PPM_FACE_CAM_INFO_S;

typedef struct _PPM_CALIB_CALIB_BOARD_INFO_
{
    unsigned int uiHoriCnt;                              /* 标定板宽度方向的方块个数 */
    unsigned int uiVertCnt;                              /* 标定板高度方向的方块个数 */
    float fSquaLen;                                      /* 标定板中黑白方块的宽高尺寸，单位m，如40mm则填入0.04 */
} PPM_CALIB_CALIB_BOARD_INFO_S;

typedef struct _PPM_CALIB_INPUT_PRM_
{
    PPM_FACE_CALIB_MATRIX_PRM_S stFaceCamCalibMatrixInfo;       /* 标定参数之人脸相机 */
    PPM_TRI_CALIB_MATRIX_PRM_S stTriCamCalibMatrixInfo;         /* 标定参数之三目相机 */
    PPM_CALIB_PIC_INFO_S stCalibPicInfo;                 /* 标定参数之截图 */
    PPM_CALIB_CALIB_BOARD_INFO_S stCalibBoardInfo;         /* 标定板相关信息 */
} PPM_CALIB_INPUT_PRM_S;

typedef struct _PPM_CALIB_OUTPUT_PRM_
{
    PPM_MATRIX_PRM_S stR_MatrixInfo;                     /* R矩阵参数, 3x3 */
    PPM_MATRIX_PRM_S stT_MatrixInfo;                     /* T矩阵参数, 1x3 */
    PPM_MATRIX_PRM_S stFaceCamInvMatrixInfo;             /* 人脸相机内参逆矩阵，3x3 */
    PPM_MATRIX_PRM_S stTriCamInvMatrixInfo;              /* 三目相机内参逆矩阵，3x3 */
} PPM_CALIB_OUTPUT_PRM_S;

typedef struct _PPM_CALIB_PRM_
{
    PPM_CALIB_INPUT_PRM_S stCalibInputPrm;                /* 标定入参 */
    PPM_CALIB_OUTPUT_PRM_S stCalibOutputPrm;              /* 标定出参 */
} PPM_CALIB_PRM_S;

/* 人脸相机标定结构体 */
typedef struct _PPM_FACE_CALIB_INPUT_PRM_
{
    PPM_FACE_CAM_INFO_S stCamInfo;                         /* 标定相机参数 */
    PPM_FACE_CALIB_PIC_INFO_S stCalibPicInfo;              /* 标定参数之截图 */
    PPM_CALIB_CALIB_BOARD_INFO_S stCalibBoardInfo;         /* 标定板相关信息 */
} PPM_FACE_CALIB_INPUT_PRM_S;

typedef struct _PPM_FACE_CALIB_OUTPUT_PRM_
{
    double fRMatrix[3][3];
    double fTMatrix[8];
    double fRms;                                           /* rms值 */
} PPM_FACE_CALIB_OUTPUT_PRM_S;

typedef struct _PPM_FACE_CALIB_PRM_
{
    PPM_FACE_CALIB_INPUT_PRM_S stFaceCalibInputPrm;                /* 标定入参 */
    PPM_FACE_CALIB_OUTPUT_PRM_S stFaceCalibOutputPrm;              /* 标定出参 */
} PPM_FACE_CALIB_PRM_S;

/* 暂未使用:相机安装参数，若相机安装位置移动后需要更新配置(海思专属) */
typedef struct _CAM_FIX_PRM_
{
    float fCamHeight;                               /* 双目相机架设高度 */
    float fPitchAngle;                              /* 俯仰角度 */
    float fInclineAngle;                            /* 倾斜角度 */
} PPM_CAM_FIX_PRM;

/* 人包置信度阈值配置 */
typedef struct _CONF_TH_PRM_
{
    int   enable;                               /* 阈值配置开关 */
    float fConfTh;                              /* 置信度阈值 */
} PPM_CONF_TH_PRM;

/* 暂未使用:人脸相机内参(海思专属) */
typedef struct _PPM_FACE_CAM_MATRIX_PRM_
{
    PPM_MATRIX_PRM_S stInterMatrix;                  /* 人脸相机内参矩阵 */
    PPM_MATRIX_PRM_S stCamDistortMatrix;             /* 人脸相机畸变矫正矩阵 */
} PPM_FACE_CAM_MATRIX_PRM_S;

/* 生成算法依赖的三目标定文件 */
typedef enum _PPM_ALG_CFG_FILE_TYPE_
{
    TRI_CAM_LR_CALIB_PARAM_TYPE = 0,                 /* 三目相机左右相机标定参数 */
    TRI_CAM_MR_CALIB_PARAM_TYPE,                     /* 三目相机中右相机标定参数 */
    TRI_CAM_CALIB_PARAM_NUM,
} PPM_ALG_CFG_FILE_TYPE_E;

typedef struct _PPM_ALG_CFG_CAM_PRM_
{
    PPM_MATRIX_PRM_S stCamInterMatrix;                    /* 镜头内参矩阵, 3x3 */
    PPM_MATRIX_PRM_S stCamDistortMatrix;                  /* 畸变矫正参数, 1x8 */

    PPM_MATRIX_PRM_S stCamRotateMatrix;                   /* 双目标定旋转矩阵，3x3 */
    PPM_MATRIX_PRM_S stCamProjectMatrix;                  /* 双目标定投影矩阵，3x4 */
} PPM_ALG_CFG_CAM_PRM_S;

typedef struct _PPM_ALG_CFG_PRVT_LR_PRM_
{
    float fFocalLen;                                      /* 左右相机的相机焦距 */
    unsigned int uiOriImgW;                               /* 图像宽，目前不需要，直接填充0 */
    unsigned int uiOriImgH;                               /* 图像高，目前不需要，直接填充0 */
} PPM_ALG_CFG_PRVT_LR_PRM_S;

typedef struct _PPM_ALG_CFG_PRVT_MR_PRM_
{
    float fRms;                                           /* 中右相机的重投影误差 */
    float fEame;                                          /* 中右相机的极线对齐误差 */
} PPM_ALG_CFG_PRVT_MR_PRM_S;

typedef union _PPM_ALG_CFG_PRVT_PRM_
{
    PPM_ALG_CFG_PRVT_LR_PRM_S stLrPrvtPrm;                /* 左右相机的私有参数 */
    PPM_ALG_CFG_PRVT_MR_PRM_S stMrPrvtPrm;                /* 中右相机的私有参数 */
} PPM_ALG_CFG_PRVT_PRM_U;

typedef struct _PPM_ALG_CFG_PRM_
{
    PPM_ALG_CFG_FILE_TYPE_E enTriCamCalibType;            /* 三目相机中双目标定参数类型，目前生成的配置文件只有lr和mr */

    PPM_ALG_CFG_CAM_PRM_S stFirstCamPrm;                  /* 双目标定第一个相机的参数，lr中的l or mr中的m */
    PPM_ALG_CFG_CAM_PRM_S stSecodCamPrm;                  /* 双目标定第二个相机的参数，lr中的r or mr中的r */

    PPM_MATRIX_PRM_S stL2RCamRotMatrix;                   /* 双目标定第一个相机到第二个相机的旋转矩阵，3x3 */
    PPM_MATRIX_PRM_S stL2RCamTransMatrix;                 /* 双目标定第一个相机到第二个相机的平移矩阵，1x3 */
    PPM_MATRIX_PRM_S stL2RCamProjectMatrix;               /* 双目标定视差转换投影矩阵，4x4 */

    PPM_ALG_CFG_PRVT_PRM_U stPrvtPrm;                     /* 私有参数，保存lr和mr的差异参数 */
} PPM_ALG_CFG_PRM_S;

/* 回调图片数据通用结构体 */
typedef struct tagComPicInfo
{
    unsigned int jpeg_cnt;                                      /* 回调jpeg个数 */
    JPEG_SNAP_CB_RESULT_ST stJpegResult[PPM_MAX_JPEG_NUM];      /* 回调jpeg图片信息 */
} IA_COM_PIC_INFO;

/* 包裹目标私有结构体 */
typedef struct tagPpmDspPkgPrvtInfo
{
    unsigned int target_num;                                    /* 报警数(人包关联暂不使用) */
    SVA_DSP_ALERT target[SVA_XSI_MAX_ALARM_NUM];                /* 报警目标  128个(人包关联暂不使用) */
    SVA_DSP_SVA_PACK packbagAlert;                              /* 包裹报警信息(人包关联暂不使用) */
} PPM_DSP_PKG_PRVT_INFO;

/* X光包裹目标私有结构体 */
typedef struct tagPpmDspXpkgPrvtInfo
{
    unsigned int uiReserved[4];
} PPM_DSP_XPKG_PRVT_INFO;

/* 人脸目标私有结构体 */
typedef struct tagPpmDspFacePrvtInfo
{
    unsigned int uiReserved[4];
} PPM_DSP_FACE_PRVT_INFO;

/* 目标私有信息 */
typedef union tagPpmDspObjPrvtInfo
{
    PPM_DSP_PKG_PRVT_INFO stPkgPrvtInfo;
    PPM_DSP_XPKG_PRVT_INFO stXpkgPrvtInfo;
    PPM_DSP_FACE_PRVT_INFO stFacePrvtInfo;
} PPM_DSP_OBJ_PRVT_INFO;

/* 通用目标信息 */
typedef struct tagPpmDspObjInfo
{
    unsigned int match_id;                                      /* 匹配ID */
    IA_COM_PIC_INFO stCbPicInfo;                                /* 回调图片信息 */

    unsigned int prvt_valid;                                    /* 私有信息是否有效 */
    PPM_DSP_OBJ_PRVT_INFO unObjPrvtInfo;                        /* 目标私有信息，目前仅有三种: 可见光包裹、X光包裹、人脸 */
} PPM_DSP_OBJ_INFO;

/* 人脸相关结果 */
typedef struct tagPpmDspFaceOut
{
    unsigned int face_cnt;                                      /* 人脸个数 */
    PPM_DSP_OBJ_INFO stFaceObjInfo[PPM_MAX_OBJ_NUM];            /* 各人脸信息 */
    unsigned int uiReserve[4];
} PPM_DSP_FACE_OUT;

/* X光包裹相关结果 */
typedef struct tagPpmDspXpkgOut
{
    unsigned int x_pkg_cnt;                                     /* X光包裹个数 */
    PPM_DSP_OBJ_INFO stXpkgObjInfo[PPM_MAX_OBJ_NUM];            /* 各包裹信息 */
    unsigned int uiReserve[4];
} PPM_DSP_XPKG_OUT;

/* 可见光包裹相关结果 */
typedef struct tagPpmDspPkgOut
{
    unsigned int pkg_cnt;                                       /* 包裹个数 */
    PPM_DSP_OBJ_INFO stPkgObjInfo[PPM_MAX_OBJ_NUM];             /* 各包裹信息 */
    unsigned int uiReserve[4];
} PPM_DSP_PKG_OUT;

/* 人包关联输出结果，暂定单一匹配ID对应三类结果，并单次输出 */
typedef struct tagPpmDspOut
{
    PPM_DSP_FACE_OUT stPpmFaceOut;                              /* 人包关联-人脸输出结果 */
    PPM_DSP_XPKG_OUT stPpmXpkgOut;                              /* 人包关联-X光包裹输出结果 */
    PPM_DSP_PKG_OUT stPpmPkgOut;                                /* 人包关联-可见光包裹输出结果 */
} PPM_DSP_OUT;

/****************************************************************************/
/*                    门禁人脸智能部分(保留参考，无用则删除)                */
/****************************************************************************/

/*
    预抓图数量的宏定义
 */
#define PRE_SNAP_NUM (5)

#define MAX_JPGMOD_NUM (4)
/* 智能模型数据最大长度 */
#define FR_MAX_MODEL_DATA_LENTH (10240)
#define FR_MAX_FFD_FACE_NUM		(10)


/*
    FR_LICENCE_STAT 算法加密库授权状态
 */
typedef enum
{
    FR_LICENCE_NO = 0,
    FR_LICENCE_FAIL,
    FR_LICENCE_SUCCESS
} FR_LICENCE_STAT;


/*
    特征点的坐标定义
 */
typedef struct
{
    float x;  /* 特征点坐标 x */
    float y;  /* 特征点坐标 y */
} FR_POSITION;

/*
    特征点结构体
 */
typedef struct _FR_CHARACT_POINTS
{
    float faceX;            /* 人脸坐标 x */
    float faceY;            /* 人脸坐标 y */
    float faceW;            /* 人脸宽度   */
    float faceH;            /* 人脸高度   */

    FR_POSITION leftEye;       /* 左眼坐标  */
    FR_POSITION rightEye;      /* 右眼坐标  */
    FR_POSITION leftMouth;     /* 嘴左边坐标  */
    FR_POSITION rightMouth;    /* 嘴右边坐标  */
    FR_POSITION noseTip;       /* 鼻子坐标  */
} FR_CHARACT_POINTS;

/*
    活体检测控制类型
 */
typedef enum
{
    FR_LD_ONCE = 0,      /* 开启一次活体检测 */
    FR_LD_START = 1,     /* 开启连续活体检测 */
    FR_LD_STOP = 2,      /* 停止连续活体检测 */
    FR_LD_PIC = 3,       /* 使用下发图片活体检测 */
    FR_LD_BUTT
} FR_LD_CRTL_TYPE;


/*
    活体检测命令字结构体 HOST_CMD_FR_DET
 */
typedef struct frSolidPrmSt
{
    unsigned int idx;               /* 回调索引 */
    FR_LD_CRTL_TYPE eCtrlType;      /* 活体检测控制类型 */
    LD_SCENE scene;                 /*活体检测应用场景和安全级 */

    /* 以下值 只有控制类型为 图片检测时有效 */
    unsigned int uiNJpgSize;        /*可见光JPG图片长度*/
    unsigned int uiIrJpgSize;       /*红外JPG图片长度*/
    FR_CHARACT_POINTS nCPoints;     /*可见光特征点坐标*/
    unsigned char *jpgAddr;         /*JPEG 数据的地址*/
    unsigned int totalLen;          /*JPEG 数据总长度*/
    unsigned int retCropFrm;        /*是否返回裁剪后的图像*/
} FR_SOLID_PRM_S;

/*
    活体透传
    HOST_CMD_SET_SOLID_PARAM
    HOST_CMD_GET_SOLID_PARAM
 */
typedef struct frSolidTransPrmSt
{
    unsigned int cmd;
    unsigned int value;
} FR_SOLID_TRANS_PRM_ST;

/*
    JPEG图片数据信息 对应 JPEG_TYPE
 */
typedef struct frBMJpegPrmSt
{
    unsigned int uiSize;                          /* 图片的数据的大小 */
    unsigned char *uiAddr;                        /* 图片的数据的地址 */
    unsigned int picCnt;                          /* 批量传递的JPG数量*/
    unsigned int picLen[MAX_JPGMOD_NUM];          /* 每一张JPG的长度*/
    unsigned int idNum[MAX_JPGMOD_NUM];           /* 对应的ID 号*/
    unsigned int haveChPoints;                    /* 是否包含特征点*/
    FR_CHARACT_POINTS cPoints;                    /* 特征点坐标 匹配第一张*/
} FR_BM_JPEG_PRM_ST;

/*
    建模具体信息，对应 BMP_TYPE 仅支持单张图片
 */
typedef struct frBMBmpPrmSt
{
    unsigned int uiId;        /* 对应的ID*/
    unsigned int uiSize;      /* 图片的数据的大小 */
    unsigned char *uiAddr;    /* 图片的数据的地址 */
} FR_BM_BMP_PRM_ST;

/*
    建模具体信息，对应 PNG_TYPE 仅支持单张图片
 */
typedef struct frBMPngPrmSt
{
    unsigned int uiId;        /* 对应的ID*/
    unsigned int uiSize;      /* 图片的数据的大小 */
    unsigned char *uiAddr;    /* 图片的数据的地址 */
} FR_BM_PNG_PRM_ST;

/*
    建模具体信息，对应 YUV_TYPE 仅支持单帧 YUV 数据
 */
typedef struct frBMYUVPrmSt
{
    unsigned int uiWidth;
    unsigned int uiHeight;
    unsigned int uiSize;      /* YUV 数据的大小 */
    unsigned char *uiAddr;    /* YUV 数据的地址 */
} FR_BM_YUV_PRM_ST;

/*
    采集建模的相关信息，对应 CAPT_TYPE 摄像头采集数据
 */
typedef struct frBMCaptPrmSt
{
    unsigned int uiScore;    /* 采集数据评分的阈值，达到该阈值的才会进行建模 */
} FR_BM_CAPT_PRM_ST;


/*
    模型数据的信息
 */
typedef struct frModelDataInfoSt
{
    unsigned int uiModelId;               /* 模型的id       */
    unsigned int uiModelSize;             /* 模型数据的大小 */
    unsigned char *uiModelDataAddr;       /* 模型数据       */
} FR_MODEL_DATA_INFO_ST;


typedef struct frJPGBMPPrmSt
{
    unsigned int idx;            /*回调索引*/
    unsigned int uiSize;         /* JPG + BMP 数据的总大小 */
    unsigned int uiJpgSize;      /* JPG数据的大小 */
    unsigned int uiBmpSize;      /* BMP数据的大小 */
    unsigned char *uiAddr;       /* JPG和BMP数据地址,BMP在前 */
} FR_BM_JPG_BMP_PRM_ST;

typedef struct frJPGJPGPrmSt
{
    unsigned int uiSize;         /* JPG + BMP 数据的总大小 */
    unsigned int uiJpgSize;      /* JPG数据的大小 */
    unsigned int uiJpgExtSize;   /* BMP数据的大小 */
    unsigned char *uiAddr;       /* JPG和BMP数据地址,BMP在前 */
} FR_BM_JPG_JPG_PRM_ST;

typedef struct frBMPBMPPrmSt
{
    unsigned int uiSize;         /* JPG + BMP 数据的总大小 */
    unsigned int uiBmpSize;      /* JPG数据的大小 */
    unsigned int uiBmpExtSize;   /* BMP数据的大小 */
    unsigned char *uiAddr;       /* JPG和BMP数据地址,BMP在前 */
} FR_BM_BMP_BMP_PRM_ST;

/*
    建模具体信息，对应不同的 FR_DATA_TYPE_E
 */
typedef union frBuildModelPrmU
{
    FR_BM_JPEG_PRM_ST stJpegTypePrm;            /* 对应 JPEG 图片的信息 */
    FR_BM_BMP_PRM_ST stBmpTypePrm;              /* 对应 BMP  图片的信息 */
    FR_BM_PNG_PRM_ST stPngTypePrm;              /* 对应 PNG  图片的信息 */
    FR_BM_YUV_PRM_ST stYUVTypePrm;              /* 对应 YUV  数据的信息 */
    FR_BM_CAPT_PRM_ST stCaptTypePrm;            /* 对应 CAPT 数据的信息 */
    FR_MODEL_DATA_INFO_ST stModelTypePrm;       /* 对应 MODEL数据的信息 */

} FR_BM_PRM_U;

/*
    人脸对比方式类型
 */
typedef enum frCPTypeE
{
    ONE_VS_ONE,               /* 一对一 */
    ONE_VS_N                  /* 一对多 */
} FR_CP_TYPE_E;

/*
    建模数据目标类型
 */
typedef enum frNameListTypeE
{
    WHITE_LIST,          /* 白名单 */
    BLACK_LIST,          /* 黑名单 */
    BOTH,                /* 两者   */
    NO_TYPE              /* 非两者 */
} FR_NAME_LIST_TYPE_E;

/*活体检测及建模错误码定义*/
typedef enum frBMResultType
{
    RESULT_SUCCESS = 0x00,                /* 建模成功 */
    RESULT_LIVEDET_NOLIVE_ERR = 0x10,     /* 活体检测为非活体 */
    RESULT_LIVEDET_NOSURE_ERR = 0x11,     /* 活体检测无法确定 */
    RESULT_ALIGN_WL_ERR = 0x20,           /* 白光数据对齐失败 */
    RESULT_ALIGN_IR_ERR = 0x21,           /* 红光数据对齐失败 */
    RESULT_BM_WL_ERR = 0x30,              /* 白光获取模型数据失败 */
    RESULT_BM_IR_ERR = 0x31,              /* 红光获取模型数据失败 */
    RESULT_SIM_COMPARE_ERR = 0x40,        /* 相似度比对失败*/
    RESULT_FACEDET_IR_ERR = 0x50,         /* 红光人脸检测失败 */
} FR_BM_RESULT_TYPE;

typedef struct frBMResultJPEGSt
{
    unsigned int WLSize;         /* 白光jpg图片大小 */
    unsigned char *pWLAddr;      /* 白光jpg图片地址 */
    unsigned int IRSize;         /* 红光jpg图片大小 */
    unsigned char *pIRAddr;      /* 红光jpg图片地址 */
} FR_BM_RESULT_JPEG_ST;

/*
    STREAM_ELEMENT_FR_BM 建模数据返回结构体
 */
typedef struct _FR_BM_RESULT_
{
    int uiId;                                               /* ID号，返回应用下发 ID   */
    unsigned int bmResult;                                 /* 是否建模成功   */
    unsigned int model_data_size;                          /* 模型数据大小   */
    unsigned char model_data[FR_MAX_MODEL_DATA_LENTH];     /* 人脸模型数据   */

    float pitch;                                           /* 平面外上下俯仰角，人脸朝上为正  */
    float yaw;                                             /* 平面外左右偏转角，人脸朝左为正  */
    float roll;                                            /* 平面内旋转角，人脸顺时针旋转为正*/
    float eyedis;                                          /* 两眼瞳孔距*/
    FR_CHARACT_POINTS cPoints;                             /* 特征点信息*/

    unsigned long bmFNum;                                  /* 建模这张的frame编号,用来提取预抓图像*/
    FR_BM_RESULT_JPEG_ST stBMResultJpeg;                   /* 建模返回JPEG图片信息*/
    FR_BM_RESULT_TYPE stBMResultType;                      /* 建模返回结果类型*/
} FR_BM_RESULT;


typedef struct tagDfrPicInfoSt
{
    unsigned int uiWitdh;            /* 人脸图像宽度     */
    unsigned int uiHeight;           /* 人脸图像高度     */
    unsigned int uiLen;              /* 人脸图像数据长度 */
} DFR_PIC_INFO_ST;

/*
    STREAM_ELEMENT_FR_DEC 活体检测结果返回结构体
 */
typedef struct tagFrDetResultSt
{
    int idx;                            /* 唯一标识符，直接返回应用下发ID */
    unsigned int detectionResult;       /* 是否是活体 */
    unsigned int ifIrFace;              /* 是否红外图像检测到人脸 */

    DFR_PIC_INFO_ST stIrFacePicInfo;    /* 红外抠出来的人脸图像信息   */
    DFR_PIC_INFO_ST stWlFacePicInfo;    /* 可见光抠出来的人脸图像信息 */

    DFR_PIC_INFO_ST stIrPicInfo;        /* 红外原始的图像信息   */
    DFR_PIC_INFO_ST stWlPicInfo;        /* 可见光原始的图像信息 */

    FR_CHARACT_POINTS irCPts;           /* 红外的特征点坐标*/
    unsigned int tLen;                  /* 红外和可见光数据的总长*/
} FR_DET_RESULT;

/* 返回的图片的信息 */
typedef struct tagFrPicInfo
{
    unsigned char *paddr;        /* 智能模块回传的图片        */
    unsigned int size;           /* 智能模块回传图片数据的长度 */
} FR_PIC_INFO;

/* 人脸姿态信息 */
typedef struct tagFrFacePostureInfo
{
    float clearity;                     /* 清晰度评分 */
    float frontal_conf;                 /* 正面评分   */
    float pitch;                        /* 平面外上下俯仰角，人脸朝上为正  */
    float yaw;                          /* 平面外左右偏转角，人脸朝左为正  */
    float roll;                         /* 平面内旋转角，人脸顺时针旋转为正*/
    float eyedis;                       /* 两眼瞳孔距*/
} FR_FACE_POSTURE_INFO;

/*
    STREAM_ELEMENT_FR_FFD_IQA 人脸检测 对比 返回结构体
 */
typedef struct tagFrFfdIqaSingle
{
    int idx;                            /* 唯一标识符，直接返回应用下发ID */
    unsigned int ifface;                /* 是否检测到人脸    */
    FR_CHARACT_POINTS cPoints;          /* 特征点信息*/
    FR_FACE_POSTURE_INFO stFPInfo;      /* 检测到的人脸姿态信息 */
    FR_PIC_INFO stFdPicInfo;            /* 检测到的人脸所对应的图片 */

    unsigned long fdFNum;               /* 人脸检测的frame编号   */
    unsigned int irLength;              /* 灰度图大小            */
    unsigned int beFirstFFD;            /* ==1时为移动侦测前的第一次人脸检测 */
} FR_FFD_IQA_SINGLE;


/*
    STREAM_ELEMENT_FR_FFD_IQA 人脸检测 对比 返回结构体
 */
typedef struct tagFrFfdIqaResult
{
    int idx;                              /* 唯一标识符，直接返回应用下发ID */
    unsigned int facenum;                 /* 检测到的人脸数量    */

    FR_FFD_IQA_SINGLE stFFdIqaResult[FR_MAX_FFD_FACE_NUM];   /* 检测到的人脸信息 */
    FR_PIC_INFO stFrWlPic;                /* 可见光图片       */
    FR_PIC_INFO stFrIrPic;                /* 红外光图片       */
} FR_FFD_IQA_RESULT;

/*
    STREAM_ELEMENT_FR_CP  对比 返回结构体
 */
typedef struct tagFrCpResultSt
{
    int uiID;                                                   /* 唯一标识符，直接返回应用下发ID */
    FR_CP_TYPE_E eCpType;                                      /* 该结构所对应的对比类型 */
    FR_NAME_LIST_TYPE_E eListType;                             /* 黑白名单类型,1VN时才有效*/
    unsigned int uiCpSuc;                                      /* 是否对比成功，成功为1，不成功为 0 */

    float sim;                                                 /* 相似度                 */

    unsigned int uiModelSize;                                  /* 模型数据大小   */
    unsigned char modelData[FR_MAX_MODEL_DATA_LENTH];          /* 人脸模型数据   */

    unsigned long cpFNum;                                      /* 比对成功的图像编号*/
    unsigned int uiJpegSize;                                   /* 抓图图片大小   */
    void *uiJpegData;                                          /* 抓图图片       */
} FR_CP_RESULT;

/*
    DSP 人脸检测的类型
 */
typedef enum
{
    FR_FD_ONESHOT = 0,             /*DSP 被动式检测*/
    FR_FD_CONTINUE,                /*DSP 主动式检测*/
    FR_FD_ONESHOT_JPG,             /*DSP JPEG格式人脸检测返回坐标框 */

    FR_FD_CONTINUE_NO_BM,          /*DSP 主动式检测,但不返回建模数据 */
    FR_FR_INVALID                  /*DSP 无效的检测 停止人脸检测*/
} FR_FD_TYPE;

/*
    建模数据操作类型
 */
typedef enum frDataTypeE
{
    DSP_FR_CAPT_WL = 0x100100,              /* 来源为 YUV  数据 默认为可见光 */
    DSP_FR_CAPT_IR,                         /* 来源为 红外光 YUV  数据       */
    DSP_FR_DOWNLOAD_JPEG = 0x100200,        /* 来源为 下发 JPEG 图片  */
    DSP_FR_DOWNLOAD_BMP,                    /* 来源为 下发 BMP 图片   */
    DSP_FR_DOWNLOAD_PNG,                    /* 来源为 下发 PNG 图片   */
    DSP_FR_DOWNLOAD_MODEL,                  /* 来源为 下发 MODEL 数据 */
    DSP_FR_DOWNLOAD_YUV,                    /* 来源为 下发 YUV  数据  */

    DSP_FR_MAX_TYPE,
} FR_DATA_TYPE_E;


/*
    HOST_CMD_FFD_CTRL   人脸检测信息
    用 STREAM_ELEMENT_FR_FFD_IQA 回调 人脸检测结果与评分
 */
typedef struct frFfdPrmSt
{
    unsigned int idx;             /*回调索引*/
    FR_FD_TYPE fdType;            /*DSP 人脸检测的类型*/
    unsigned int fdnum;           /* 多人脸检测的人脸数量 取值0与1时，DSP 只检测一个人脸*/
    unsigned int solid;           /* 是否是立体的       */
    FR_DATA_TYPE_E eDataType;     /* 数据来源类型       */
    FR_BM_PRM_U uDataInfo;        /* 数据内容           */
    unsigned int eyeDist;         /* 人脸检测标准眼间距 */
} FR_FFD_PRM_S;

/*
    建模数据来源类型
 */
typedef enum frOpTypeE
{
    FR_REGISTER,            /* 建模注册         */
    UN_REGISTER,            /* 去注册           */
    UN_REGISTER_LIST,       /* 整个库删除掉     */
    NO_REGISTER             /* 不做注册相关操作 */
} FR_OP_TYPE_E;

/*
    活体连续检测功能控制信息
 */
typedef struct _FR_LIVTEST_CTRL_
{
    unsigned int ctrl;     /* 是否开启活体检测 */
    unsigned int time;     /* 检测时间 */
} FR_LIVTEST_CTRL;


/*
    建模命令字对应的信息 HOST_CMD_FR_BM
 */
typedef struct frBuildModelPrmSt
{
    int idx;                                /* 唯一标识符，由应用下发ID，回调处理直接返回 */
    FR_NAME_LIST_TYPE_E eListType;          /* 标识模型导入到黑名单、白名单 */
    FR_OP_TYPE_E eOpType;                   /* 建模的同时也同时完成对算法黑白名单库注册 or 去注册 */
    FR_DATA_TYPE_E eDataType;               /* 建模数据来源类型  */
    FR_BM_PRM_U uBMPrm;                     /* 数据来源类型对应的具体信息 */
    FR_LIVTEST_CTRL stLive;                 /* 活体功能控制      */
} FR_BUILD_MODEL_PRM_ST;

/*
    模型数据导入到算法层的数据库 HOST_CMD_FR_MODEL_ADD
 */
typedef struct frModelPrmAddSt
{
    int idx;                                /* 唯一标识符，由应用下发ID，回调处理直接返回 */
    FR_NAME_LIST_TYPE_E eListType;          /* 标识模型导入到黑名单、白名单 */
    FR_MODEL_DATA_INFO_ST stModelInfo;      /* 模型的信息 */
} FR_MODEL_ADD_PRM_ST;

/*
    导出/删除算法层的模型数据 HOST_CMD_FR_MODEL_DEL
 */
typedef struct frModelPrmDelSt
{
    int idx;                                /* 唯一标识符，由应用下发ID，回调处理直接返回 */
    FR_NAME_LIST_TYPE_E eListType;          /* 标识模型导入到黑名单、白名单 */
    FR_OP_TYPE_E eOpType;                   /* 操作类型，注册 or 去注册操作 */

} FR_MODEL_DEL_PRM_ST;

/*
    人脸智能相关信息
 */
typedef struct _FACE_THRESHOLD_
{
    unsigned int stat;               /* 1:登记 2:比对    */
    unsigned int movementVal_rec;    /* 登记移动侦测阈值 */
    unsigned int movementVal_cmp;    /* 比对移动侦测阈值 */
    unsigned int movementCtrl;       /* 移动侦测开关控制 */
    float frontal_conf;              /* 正面评分阈值     */
    float clearity;                  /* 清晰度           */
    float pitch;                     /* ptich阈值        */
    float yaw;                       /* yaw阈值          */
    float roll;                      /* roll阈值         */
} FACE_THRESHOLD;

/*
    人脸对比要求信息
 */
typedef struct frCPStandardSt
{
    unsigned int uiThreshold;          /* 阈值 填10000以内的数字，DSP进行计算 */
    unsigned int uiCpCnt;              /* 需要的最相似的信息的个数 */
} FR_CP_STANDAED_ST;

/*
    人脸模型对比 HOST_CMD_FR_COMPARE
 */
typedef struct frCompareSt
{
    int idx;
    unsigned char baseModel[1536];
    unsigned int baseModelLen;
    unsigned char camModel[1536];
    unsigned int camModelLen;
    float similarity;
} FR_COMPARE_ST;

/*
    人脸对比信息 用于1对1 比对
 */
typedef struct _FR_PROCESS_IN_
{
    unsigned int uiEnable;                   /* 对比使能与去使能，使能为1，去使能为 0 */
    unsigned int retModel;                   /* 是否使用下发的model与下发的ref对比    */
    FR_BUILD_MODEL_PRM_ST refModelParam;     /* 参考人脸模型源数据*/
} FR_PROCESS_IN;

/*
    用于一对多 比对
 */
typedef struct frCP41VsNST
{
    unsigned int uiEnable;                   /* 对比使能与去使能，使能为1，去使能为 0 */
    unsigned int uiNeedPicNum;               /* 一对多对比 需要返回的图片数量         */
    FR_BUILD_MODEL_PRM_ST stMdlPrm;          /* 1VSN 输入数据来源*/
} FR_CP_4_1VSN_ST;


/*
    人脸对比下发信息
 */
typedef union frCPInfoU
{
    FR_PROCESS_IN stFrProIn;        /* 一对一 */
    FR_CP_4_1VSN_ST stFr1VSN;       /* 一对多 */
} FR_CP_INFO_U;


/*
    活体单次检测功能控制信息
 */
typedef struct frLdPrmSt
{
    unsigned int ctrl;     /* 是否开启活体检测 */
} FR_LD_PRM_ST;

/*
    预抓图图片数据的信息
 */
typedef struct
{
    unsigned char *picAddr;   /* 图片的地址 */
    unsigned int picLen;      /* 图片的长度 */
    unsigned int picNum;      /* 图片的序号 */
} FR_PICTURE_ST;


/*
    HOST_CMD_FR_PRESNAP 预抓图信息
 */
typedef struct frPreSnapSt
{
    unsigned long preFrmNum;                  /* 预抓取的frame 编号*/
    FR_PICTURE_ST nPictures[PRE_SNAP_NUM];    /* 可见光图片 */
    FR_PICTURE_ST irPictures[PRE_SNAP_NUM];   /* 红外  图片 */
} FR_PRE_SNAP_ST;


/*
    活体检测方式类型
 */
typedef enum frLDTypeE
{
    ONE_SHOT,               /* 单次检测 */
    CONTINUOUS              /* 多次检测 */
} FR_LD_TYPE_E;

/*
    人脸范围
 */
typedef struct
{
    float x;       /* 人脸坐标 x */
    float y;       /* 人脸坐标 y */
    float width;   /* 人脸宽度   */
    float height;  /* 人脸高度   */
} FACE_RECT;

/*
    人脸 ROI 区域
 */
typedef struct stFrRoiRectSt
{
    int x;
    int y;
    int width;
    int height;
    int roiEnable;  /*ROI 使能开关,HISI平台中端门禁使用以上五个参数   */

    int roiFresh;    /*是否更新ROI参数*/

    float pitch;
    float yaw;
    float eyedis;
    float minFaceW;
    float maxFaceW;
    float minFaceH;
    float maxFaceH;
    int threshEnable;  /*阈值 开关*/
    int threshFresh;   /*是否更新阈值参数*/
} FR_ROI_RECT;


/*
    STREAM_ELEMENT_FR_PRE_CAP 预抓数据返回结构体
 */
typedef struct tagPreCapResultSt
{
    unsigned char *picAddr;           /* 所有图片的基地址，先可见光，再红外的方式叠加 */
    unsigned int picLen[10];          /* 每张可见光图片的长度 */
    unsigned int picCnt;              /* 抓拍的帧数，每一帧红外、可见光各抓1张。最大为5，可能小于5*/
    unsigned int tPicLen;             /* 总的PIC 长度*/
    unsigned int frmNum;              /* 需要抓取的这一帧编号*/
} PRE_CAP_RESULT;

/*
    人脸对比信息结构体 HOST_CMD_FR_CP
 */
typedef struct frCPInfoSt
{
    int idx;                         /* 唯一标识符，由应用下发ID，回调处理直接返回 */
    FR_CP_TYPE_E eCMType;            /* 对比类型 1对1 或 1对多*/
    FR_CP_INFO_U uCpInfo;            /* 对比的信息 */
} FR_CP_INFO_ST;

/*
    STREAM_ELEMENT_CAP  对比 返回结构体
 */
typedef struct
{
    unsigned int capMode;

} JPG_SNAP_RESULT;

/****************************************************************************/
/*                                 XSP成像处理模块                         */
/****************************************************************************/

//#define XSP_UNPEN_MAX_NUM	(10)         /* 一个包裹中能识别到的难穿透区域个数*/
#define XSP_IDENTIFY_NUM	(10)         /*最大区域个数*/

#define XSP_CUVER_VALUE_NUM (16)
#define XSP_CUVER_MODE_NUM	(4)

/*
    XRAY处理模式
 */
typedef enum
{
    XRAY_TYPE_NORMAL = 0,             /* 正常过包数据 */
    XRAY_TYPE_CORREC_FULL = 1,        /* 满载校正 */
    XRAY_TYPE_CORREC_ZERO = 2,        /* 本底校正 */
    XRAY_TYPE_DIAGNOSE = 3,           /* 诊断数据 */
    XRAY_TYPE_PLAYBACK = 4,           /* 回拉数据 */
    XRAY_TYPE_TRANSFER = 5,           /* 转存数据，该类型已不再支持，改走命令方式 */
    XRAY_TYPE_AFTERGLOW = 6,          /* 余辉数据 */
    XRAY_TYPE_CALIBRATION = 7,        /* 标定数据 */
    XRAY_TYPE_AUTO_CORR_FULL = 8,     /* 自动满载校正 */
    XRAY_TYPE_PSEUDO_BLANK = 9,       /* 背景伪空白数据*/

    XRAY_TYPE_BUTT
} XRAY_PROC_TYPE;

/**
 * @enum    XRAY_PB_TYPE
 * @brief   XRAY回拉模式
 */
typedef enum
{
    XRAY_PB_FRAME = 0,          /* 整帧/整包裹回拉 */
    XRAY_PB_SLICE = 1,          /* 条带回拉 */

    XRAY_PB_NUM
} XRAY_PB_TYPE;

/**
 * @enum    XRAY_PB_SPEED
 * @brief   XRAY条带回拉速度，仅在条带回拉时有效
 */
typedef enum
{
    XRAY_PB_SPEED_NORMAL = 0,        /* 普通速度 */
    XRAY_PB_SPEED_XTS = 1,           /* XTS模式速度 */

    XRAY_PB_SPEED_NUM
} XRAY_PB_SPEED;

/**
 * @enum    XRAY_PB_PARAM
 * @brief   XRAY回拉命令参数，包含回拉模式和回拉速度
 */
typedef struct
{
    XRAY_PB_TYPE enPbType;               /* 回拉类型 */
    XRAY_PB_SPEED enPbSpeed;             /* 回拉速度 */
    unsigned int bEnableSync;            /* 是否需要主辅视角回拉同步   0-需要 1-不需要 */
} XRAY_PB_PARAM;

/**
 * @enum    XSP_DISP_MODE_T
 * @brief   图像显示模式
 */
typedef enum
{
    XSP_DISP_COLOR = 0,              /* 彩色模式*/
    XSP_DISP_GRAY = 1,               /* 黑白模式*/
    XSP_DISP_ORGANIC = 2,            /* 有机物剔除模式*/
    XSP_DISP_ORGANIC_PLUS = 3,       /* 有机物剔除+ 模式*/
    XSP_DISP_INORGANIC = 4,          /* 无机物剔除模式*/
    XSP_DISP_INORGANIC_PLUS = 5,     /* 无机物剔除+ 模式*/
    XSP_DISP_ORGANIC_ENHANCE = 6,    /* 可疑有机物增强模式，原子序列7，8，9*/
    XSP_DISP_ORGANIC_ENHANCE_7 = 7,  /* 可疑有机物增强模式，原子序列7*/
    XSP_DISP_ORGANIC_ENHANCE_8 = 8,  /* 可疑有机物增强模式，原子序列8*/
    XSP_DISP_ORGANIC_ENHANCE_9 = 9,  /* 可疑有机物增强模式，原子序列9*/

    XSP_DISP_MODE_NUM                /* 总数*/
} XSP_DISP_MODE_T;

/* 伪彩模式*/
typedef enum
{
    XSP_PSEUDO_COLOR_0 = 0,        /* 伪彩模式0 */
    XSP_PSEUDO_COLOR_1 = 1,        /* 伪彩模式1 */

    XSP_PSEUDO_COLOR_NUM           /* 总数 */
} XSP_PSEUDO_COLOR_T;

/**
 * @enum    XSP_DEFAULT_STYLE_T
 * @brief   默认成像风格
 */
typedef enum
{
    XSP_DEFAULT_STYLE_0,            /*默认成像风格0*/
    XSP_DEFAULT_STYLE_1,            /*默认成像风格1*/
    XSP_DEFAULT_STYLE_2,            /*默认成像风格2*/

    XSP_DEFAULT_STYLE_NUM
} XSP_DEFAULT_STYLE_T;

/**
 * @enum    XSP_COLOR_TABLE_T
 * @brief   成像颜色配置表
 */
typedef enum
{
    XSP_COLOR_TABLE_0,            /*颜色配置表0*/
    XSP_COLOR_TABLE_1,            /*颜色配置表1*/
    XSP_COLOR_TABLE_2,            /*颜色配置表2*/
    XSP_COLOR_TABLE_3,            /*颜色配置表3*/
    XSP_COLOR_TABLE_4,            /*颜色配置表4*/
    XSP_COLOR_TABLE_5,            /*颜色配置表5*/
    XSP_COLOR_TABLE_6,            /*颜色配置表6*/
    XSP_COLOR_TABLE_7,            /*颜色配置表7*/

    XSP_COLOR_TABLE_NUM
} XSP_COLOR_TABLE_T;

/**
 * @enum    XSP_COLORS_IMAGING_T
 * @brief   成像颜色配置表
 */
typedef enum
{
    XSP_COLORS_IMAGING_3S,            /*3色显示*/
    XSP_COLORS_IMAGING_6S,            /*6色显示*/

    XSP_COLORS_IMAGING_NUM
} XSP_COLORS_IMAGING_T;

/**
 * @enum    XSP_ALG_VERSION
 * @brief   成像算法版本，为了兼容不同版本之间的数据，便于转换
 */
typedef enum
{
    XSP_ZDATA_VERSION_V1,            /*成像算法原子序数版本V1*/
    XSP_ZDATA_VERSION_V2,            /*成像算法原子序数版本V2*/

    XSP_ZDATA_VERSION_NUM,
} XSP_ZDATA_VERSION;

/**
 * @enum    XSP_SLICE_CONTENT
 * @brief   条带中的数据内容，空白或包裹
 */
typedef enum
{
    XSP_SLICE_BLANK,                /* 空白条带 */
    XSP_SLICE_PACKAGE,              /* 包裹条带 */
    XSP_SLICE_NEIGHBOUR,            /* 邻域条带，只给算法做图像补边处理，实际不显示 */

    XSP_SLICE_CONTENT_NUM
} XSP_SLICE_CONTENT;

/**
 * @enum    XSP_PACKAGE_TAG
 * @brief   包裹起始、结束标记
 */
typedef enum
{
    XSP_PACKAGE_NONE,           /* 非包裹数据|     | */
    XSP_PACKAGE_START,          /* 包裹开始，R2L|  ---|，L2R|---  | */
    XSP_PACKAGE_END,            /* 包裹结束，R2L|---  |，L2R|  ---| */
    XSP_PACKAGE_MIDDLE,         /* 包裹中间段，即全是包裹数据|-----| */
    XSP_PACKAGE_CENTER,         /* 包裹位于中间，两端有空白，即| --- | */
    XSP_PACKAGE_BOTH_ENDS,      /* 包裹位于两端，中间有空白，即|-  --| */

    XSP_PACKAGE_TAG_NUM
} XSP_PACKAGE_TAG;

/**
 * @enum    XSP_SENSITIVITY_T
 * @brief   灵敏度档位，分低、中、高三档
 */
typedef enum
{
    XSP_SENSITIVITY_LOW,             /*低*/
    XSP_SENSITIVITY_MODDLE,          /*中*/
    XSP_SENSITIVITY_HIGH,            /*高*/

    XSP_SENSITIVITY_NUM
} XSP_SENSITIVITY_T;

/* XSP加速模式 */
typedef enum
{
    XSP_SPEEDUP_NONE        = 0,    // 无加速
    XSP_SPEEDUP_GPU         = 1,    // GPU加速
    XSP_SPEEDUP_MODE_NUM,
} XSP_SPEEDUP_MODE;

/* 包裹分割方法 */
typedef enum
{
    XSP_PACK_DIVIDE_NONE        = 0,    // 无分割
    XSP_PACK_DIVIDE_IMG_IDENTY  = 1,    // 图像分割
    XSP_PACK_DIVIDE_HW_SIGNAL   = 2,    // 硬件信号分割
    XSP_PACK_DIVIDE_METHOD_NUM,         // 分割方法总数
}XSP_PACKAGE_DIVIDE_METHOD;

/* 主辅通道分割结果的选择 */
typedef enum
{
    XSP_PACK_DIVIDE_INDEPENDENT  =0,    // 包裹分割结果主辅相互独立
    XSP_PACK_DIVIDE_USE_MASTER  = 1,    // 包裹分割结果都使用主通道
    XSP_PACK_DIVIDE_USE_SLAVE   = 2,    // 包裹分割结果都使用辅通道
}XSP_PACKAGE_DIVIDE_SEGMASTER;

typedef struct
{
    int bEnable;                    /* 使能开关，0-关闭，1-开启 */
    unsigned int uiLevel;           /* 等级 */
} XSP_ENABLE_A_LEVEL;

typedef struct
{
    unsigned int u32LowerLimit;     /* 范围下限，不得高于u32UpperLimit */
    unsigned int u32UpperLimit;     /* 范围上限，不得低于u32LowerLimit */
} XSP_RANGE;

/**
 * @struct XSP_DEFAULT_STYLE
 * @brief  默认成像风格
 */
typedef struct
{
    XSP_DEFAULT_STYLE_T xsp_style;  /* 默认成像风格 */
    unsigned int reserved[3];       /* 保留字节 */
} XSP_DEFAULT_STYLE;

/**
 * @struct XSP_ORIGINAL_MODE_PARAM
 * @brief  原始模式参数，使用bEnable和uiLevel参数，默认增强等级为50
 */
typedef XSP_ENABLE_A_LEVEL          XSP_ORIGINAL_MODE_PARAM;

/**
 * @struct XSP_STRETCH_MODE_PARAM
 * @brief  图像拉伸模式参数，使用bEnable和uiLevel参数，uiLevel取值范围0~100，小于50图像纵向压缩，大于50为纵向放大（预留拉伸系数，暂不支持纵向拉伸），不开启拉伸时默认50
 */
typedef XSP_ENABLE_A_LEVEL          XSP_STRETCH_MODE_PARAM;

/**
 * @struct XSP_DISP_MODE
 * @brief  显示模式
 */
typedef struct
{
    XSP_DISP_MODE_T disp_mode;      /* 显示模式 */
    unsigned int reserved[3];       /* 保留字节 */
} XSP_DISP_MODE;

/**
 * @struct XSP_ANTI_COLOR_PARAM
 * @brief  反色参数，仅使用bEnable参数
 */
typedef XSP_ENABLE_A_LEVEL          XSP_ANTI_COLOR_PARAM;

/**
 * @struct XSP_EE_PARAM
 * @brief  边缘增强参数，Edge Enhance，使用bEnable和uiLevel参数，默认增强等级为50
 */
typedef XSP_ENABLE_A_LEVEL          XSP_EE_PARAM;

/**
 * @struct XSP_UE_PARAM
 * @brief  超增参数，Ultra Enhance，仅使用bEnable参数
 */
typedef XSP_ENABLE_A_LEVEL          XSP_UE_PARAM;

/**
 * @struct XSP_LE_PARAM
 * @brief  局增参数，Local Enhance，仅使用bEnable参数 
 */
typedef XSP_ENABLE_A_LEVEL          XSP_LE_PARAM;

/**
 * @struct XSP_LP_PARAM
 * @brief  低穿参数，Low Penetrate，仅使用bEnable参数 
 */
typedef XSP_ENABLE_A_LEVEL          XSP_LP_PARAM;

/**
 * @struct XSP_HP_PARAM
 * @brief  高穿参数，High Penetrate，仅使用bEnable参数 
 */
typedef XSP_ENABLE_A_LEVEL          XSP_HP_PARAM;

/**
 * @struct XSP_VAR_ABSOR_PARAM
 * @brief  可变吸收率，Variable Absorption Rate，使用bEnable与uiLevel参数
 * @brief  uiLevel值越小，低吸收率区域增强越明显，值越大，高吸收率区域增强越明显，范围[0, 64]
 */
typedef XSP_ENABLE_A_LEVEL          XSP_VAR_ABSOR_PARAM;

/**
 * @struct XSP_LUMINANCE_PARAM
 * @brief  亮度，仅对单能有效，仅使用uiLevel参数
 * @brief  范围[0, 100]，默认值50，<50为减暗，>50为增亮
 */
typedef XSP_ENABLE_A_LEVEL          XSP_LUMINANCE_PARAM;

/**
 * @struct XSP_ENHANCED_SCAN_PARAM
 * @brief  强扫参数，仅使用bEnable参数 
 */
typedef XSP_ENABLE_A_LEVEL          XSP_ENHANCED_SCAN_PARAM;

/**
 * @struct XSP_VERTICAL_MIRROR_PARAM
 * @brief  垂直镜像参数，仅使用bEnable参数，开启则在基于原始图像做垂直镜像处理 
 */
typedef XSP_ENABLE_A_LEVEL          XSP_VERTICAL_MIRROR_PARAM;

/**
 * @struct XSP_DEFORMITY_CORRECTION
 * @brief  畸形矫正，仅使用bEnable参数 
 */
typedef XSP_ENABLE_A_LEVEL          XSP_DEFORMITY_CORRECTION;

/**
 * @struct XSP_DOSE_CORRECT_PARAM
 * @brief  剂量修正，仅使用uiLevel参数
 * @brief  范围[0, 100]
 */
typedef XSP_ENABLE_A_LEVEL          XSP_DOSE_CORRECT_PARAM;

/**
 * @struct XSP_COLOR_ADJUST_PARAM
 * @brief  颜色调节，仅使用uiLevel参数
 * @brief  范围[0, 100]，默认值50
 */
typedef XSP_ENABLE_A_LEVEL          XSP_COLOR_ADJUST_PARAM;

/**
 * @struct XSP_NOISE_REDUCE_PARAM
 * @brief  降噪等级，仅使用uiLevel参数
 * @brief  范围[0, 100]，0表示关闭降噪，默认值0
 */
typedef XSP_ENABLE_A_LEVEL          XSP_NOISE_REDUCE_PARAM;

/**
 * @struct XSP_CONTRAST_PARAM
 * @brief  对比度等级，仅使用uiLevel参数
 * @brief  范围[1, 100]，默认值2
 */
typedef XSP_ENABLE_A_LEVEL          XSP_CONTRAST_PARAM;

/**
 * @struct XSP_COLDHOT_PARAM
 * @brief  冷热源阈值，仅使用uiLevel参数
 * @brief  范围[0, 800]，默认值100
 */
typedef XSP_ENABLE_A_LEVEL          XSP_COLDHOT_PARAM;

/**
 * @struct  XSP_LE_TH_RANGE
 * @brief   局增灰度阈值范围 
 * @note    范围下限，取值范围[0, 65535]，默认值为0 
 * @note    范围上限，取值范围[0, 65535]，默认值为60000
 */
typedef XSP_RANGE                   XSP_LE_TH_RANGE;

/**
 * @struct XSP_GAMMA_PARAM
 * @brief  gamma开关，1为启用，0为不启用
 * @brief  gamma增强强度范围[0, 100]，默认值50，启用时才需要设置
 */
typedef XSP_ENABLE_A_LEVEL          XSP_GAMMA_PARAM;

/**
 * @struct XSP_AIXSP_RATE_PARAM
 * @brief  AIXSP权重，仅使用uiLevel参数,仅在AI-XSP启用时该参数有效
 * @brief  范围[0, 100]，0表示AIXSP占0%，默认值100
 */
typedef XSP_ENABLE_A_LEVEL          XSP_AIXSP_RATE_PARAM;



/**
 * @struct  XSP_DT_GRAY_RANGE
 * @brief   双能分辨灰度范围，仅双能有效 
 * @note    范围下限，取值范围[0, 65535]，默认值为500 
 * @note    范围上限，取值范围[0, 65535]，默认值为60000
 */
typedef XSP_RANGE                   XSP_DT_GRAY_RANGE;

/**
 * @struct  XSP_SET_RM_BLANK_SLICE
 * @brief   控制空白条带是否删除
 * @brief   bEnable 默认值 0 ，0表示关闭 1表示开启
 * @brief   uilevel范围[0, 100]，默认值0
 * @brief   uilevel 0 表示空白条带均不显示，100 表示空白条带均显示 
 * @brief   从0~100用于显示的空白条带逐渐增多
 */
typedef XSP_ENABLE_A_LEVEL          XSP_SET_RM_BLANK_SLICE;

/**
 * @struct  XSP_PACKAGE_DIVIDE_PARAM
 * @brief   包裹分割参数
 */
typedef struct
{
    XSP_PACKAGE_DIVIDE_METHOD enDivMethod;      /* 分割方法，默认值为XSP_PACK_DIVIDE_IMG_IDENTY */
    unsigned int u32DivSensitivity;             /* 分割灵敏度，取值范围[0, 100]，默认值为50 */
    XSP_PACKAGE_DIVIDE_SEGMASTER u32segMaster;  /* 包裹主辅分割是否同步，0主辅独立分割; 1-主辅都采用主视角分割结果；2-主辅都采用辅视角分割结果；默认取0 */
} XSP_PACKAGE_DIVIDE_PARAM;

/**
 * @struct  XSP_SPEEDUP_PARAM
 * @brief   XSP算法加速参数 
 * @warning 仅在非实时过包、回拉、文件管理查看图片时可设置
 */
typedef struct
{
    XSP_SPEEDUP_MODE enMode;        /* 加速模式 */
    unsigned int res[3];            /* 预留 */
} XSP_SPEEDUP_PARAM;

/**
 * @struct XSP_PSEUDO_COLOR_PARAM
 * @brief  伪彩模式，仅对单能有效
 */
typedef struct
{
    XSP_PSEUDO_COLOR_T pseudo_color;    /* 伪彩模式 */
    unsigned int reserved[1];           /* 保留字节*/
} XSP_PSEUDO_COLOR_PARAM;

/**
 * @struct XSP_COLOR_TABLE_PARAM
 * @brief  色彩模式，仅对双能有效
 */
typedef struct
{
    XSP_COLOR_TABLE_T enConfigueTable;    /* 色彩模式 */
    unsigned int reserved[1];             /* 保留字节*/
} XSP_COLOR_TABLE_PARAM;

/**
 * @struct XSP_COLOR_TABLE_PARAM
 * @brief  配置三色六色显示
 */
typedef struct
{
    XSP_COLORS_IMAGING_T enColorsImaging;    /* 三色六色显示 */
    unsigned int reserved[1];                /* 保留字节*/
} XSP_COLORS_IMAGING_PARAM;

/**
 * @struct XSP_UNPEN_PARAM
 * @brief  难穿透识别参数
 */
typedef struct
{
    unsigned int uiEnable;              /* 使能，0-关闭，1-开启 */
    XSP_SENSITIVITY_T uiSensitivity;    /* 灵敏度 */
    unsigned int uiColor;               /* 画框颜色，RGB格式，红色0xff0000（默认），绿色0x00ff00，蓝色0x0000ff */
    unsigned int reserved[2];           /* 保留 */
} XSP_UNPEN_PARAM;

/**
 * @enum    XSP_ALERT_TYPE
 * @brief   XSP算法识别危险品类型
 */
typedef enum
{
    XSP_ALERT_TYPE_ORG      = 0,    // 可疑有机物类型
    XSP_ALERT_TYPE_UNPEN    = 1,    // 难穿透类型
    XSP_ALERT_TYPE_BOMB     = 2,    // 易爆物类型
    XSP_ALERT_TYPE_DRUG     = 3,    // 毒品类型
    XSP_ALERT_TYPE_NUM,             // XSP算法识别危险品类型总数
} XSP_ALERT_TYPE;

/**
 * @struct  XSP_BKG_PARAM
 * @brief   背景检测参数设置
 */
typedef struct
{
    unsigned int uiBkgDetTh;            /* 背景检测阈值，范围[0, 65535]，小于该值即判定为背景，单能默认为61000，双能默认为63000 */
    unsigned int uiFullUpdateRatio;     /* 满载自动更新平滑比例，范围[0, 100]，0表示不更新，100表示全更新 */
    unsigned int reserved[6];           /* 保留 */
} XSP_BKG_PARAM;

/**
 * @struct  XSP_BKG_COLOR
 * @brief   显示背景颜色值设置
 */
typedef struct
{
    DSP_IMG_DATFMT enDataFmt;   /* 背景颜色格式, DSP_IMG_DATFMT_YUV420SP_VU 或 DSP_IMG_DATFMT_BGRA32 或 DSP_IMG_DATFMT_BGR24 */
    unsigned int uiBkgValue;    /* 背景颜色值，YUV格式按0xFF-Y-U-V，各颜色通道取值范围为16~235，ARGB格式按0xA-R-G-B */
} XSP_BKG_COLOR;

/**
 * @struct XSP_SUS_ALERT
 * @brief  可疑物告警
 */
typedef struct
{
    unsigned int uiEnable;              /* 使能，0-关闭，1-开启 */
    XSP_SENSITIVITY_T uiSensitivity;    /* 灵敏度 */
    unsigned int uiColor;               /* 框体颜色，RGB格式，红色0xff0000（默认），绿色0x00ff00，蓝色0x0000ff */
    unsigned int reserved[2];           /* 保留 */
} XSP_SUS_ALERT;

/**
 * @struct XSP_RECT
 * @brief  XSP相关识别数据矩形区域
 */
typedef struct
{
    unsigned int uiX;              /*起始X坐标*/
    unsigned int uiY;              /*起始Y坐标*/
    unsigned int uiWidth;          /*宽度*/
    unsigned int uiHeight;         /*高度*/
} XSP_RECT;

/**
 * @struct XSP_DATA_SENT
 * @brief 区域识别数据
 */
typedef struct
{
    unsigned int uiNum;                       /* 个数，不超过XSP_IDENTIFY_NUM */
    unsigned int uiResult;                    /* 复用为目标框颜色 */
    unsigned int type;                        /* 智能识别类型，取值范围：ISM_XDT_TYPE_MIN~ISM_XDT_TYPE_MAX */
    SVA_RECT_F stRect[XSP_IDENTIFY_NUM];      /* 识别区域 */
} XSP_DATA_SENT;

/**
 * @struct XSP_LIB_TIME_PARM
 * @brief 算法库耗时统计
 */
typedef struct
{
    unsigned long long timeBeforeProc;   /* 送算法库前时间 */
    unsigned long long timeProcPipe0S;   /* 算法库一级流水开始时间 */
    unsigned long long timeProcPipe0E;   /* 算法库一级流水结束时间 */
    unsigned long long timeProcPipe1S;   /* 算法库二级流水开始时间 */
    unsigned long long timeProcPipe1E;   /* 算法库二级流水结束时间 */
    unsigned long long timeProcPipe2S;   /* 算法库三级流水开始时间 */
    unsigned long long timeProcPipe2E;   /* 算法库三级流水结束时间 */
    unsigned long long timeAfterProc;    /* 送算法库后时间 */
} XSP_LIB_TIME_PARM;

/**
 * @struct  XSP_RT_PARAMS
 * @brief   XSP Realtime Params
 */
typedef struct
{
    XSP_DEFAULT_STYLE         stDefaultStyle;     // 默认成像风格
    XSP_ORIGINAL_MODE_PARAM   stOriginalMode;     // 原始模式
    XSP_DISP_MODE             stDispMode;         // 显示模式
    XSP_ANTI_COLOR_PARAM      stAntiColor;        /* 反色 */
    XSP_EE_PARAM              stEe;               /* 边增 */
    XSP_UE_PARAM              stUe;               /* 超增 */
    XSP_LP_PARAM              stLp;               /* 低穿 */
    XSP_HP_PARAM              stHp;               /* 高穿 */
    XSP_LE_PARAM              stLe;               /* 局增 */
    XSP_VAR_ABSOR_PARAM       stVarAbsor;         /* 可变吸收率 */
    XSP_LUMINANCE_PARAM       stLum;              /* 亮度 */
    XSP_PSEUDO_COLOR_PARAM    stPseudoColor;      /* 伪彩，仅支持单能 */
    XSP_ENHANCED_SCAN_PARAM   stEnhancedScan;     /* 强扫 */
    XSP_BKG_PARAM             stBkg;              /* 背景 */
    XSP_COLOR_TABLE_PARAM     stColorTable;       /* 颜色表 */
    XSP_COLORS_IMAGING_PARAM  stColorsImaging;    /* 三色六色显示 */
    XSP_BKG_COLOR             stBkgColor;         /* 背景颜色 */
} XSP_RT_PARAMS;

/**
 * @struct XSP_TIP_NORMALIZED
 * @brief 从应用获取Tip数据
 */
typedef struct
{
    unsigned int uiXrayW;         /*数据宽度*/
    unsigned int uiXrayH;         /*数据高度*/
    unsigned int uiBufLen;        /*数据缓冲区长度*/
    unsigned short *pXrayBuf;     /*数据缓存区*/
} XSP_TIP_NORMALIZED;

/**
 * @struct XSP_TIP_DATA_ST
 * @brief Tip考核图像信息
 */
typedef struct
{
    unsigned long long uiTime;                    /*反应时间，单位ms*/
    unsigned int uiEnable;                  /*TIP插入设置：0表示取消，1表示插入*/
    unsigned int uiPackScan;                /*包裹扫描行数*/
    unsigned int uiColor;                   /*叠框颜色，比如红色:0xff0000*/
    XSP_TIP_NORMALIZED stTipNormalized[2];  /*TIP数据，最多支持两个视角*/
} XSP_TIP_DATA_ST;


/**
 * @struct XSP_TIP_RESULT
 * @brief  上报TIP结果
 */
typedef struct
{
    int result;                   /*0表示失败，1表示成功*/
} XSP_TIP_RESULT;

/**
 * @struct  XSP_TIP_RAW_CUTOUT
 * @brief	从包裹RAW数据中扣取TIP
 * @note    p_raw_buf作为输入时，Buffer有效数据长度（单位：字节）：
 * @note        单能：package_width*package_height*2
 * @note        双能：package_width*package_height*2*2 + package_width*package_height
 * @note    p_raw_buf作为输出时，Buffer有效数据长度（单位：字节）：tip_data_size
 */
typedef struct
{
    unsigned int package_width;         /* 保存的整个包裹RAW数据的宽，64*n，5030为448，6550为640 */
    unsigned int package_height;        /* 保存的整个包裹RAW数据的高，不固定 */
    XSP_RECT tip_rect;                  /* TIP矩形框，相对于显示窗口的坐标 */
    unsigned char *p_raw_buf;           /* APP输入：包裹RAW Buffer，DSP输出：TIP RAW */
    unsigned int tip_data_size;         /* DSP输出TIP RAW数据长度，双能包裹低高能+原子序数，单位：字节 */
    unsigned int tip_raw_width;         /* 输出TIP RAW数据的宽 */
    unsigned int tip_raw_height;        /* 输出TIP RAW数据的高 */
} XSP_TIP_RAW_CUTOUT;


/**
 * @struct  XSP_RESULT_DATA
 * @brief   XSP输出的图像识别信息
 */
typedef struct
{
    XSP_DATA_SENT stUnpenSent;         /*XSP难穿透识别信息*/
    XSP_DATA_SENT stZeffSent;          /*XSP可疑物识别信息*/
} XSP_RESULT_DATA;

/**
 * @struct  XSP_TRANSFER_INFO
 * @brief   包裹分割相关信息
 */
typedef struct
{
    unsigned long long uiSyncTime;      /* 包裹结束条带的相对时间（系统启动后运行的时长），单位ms，该参数采传会透传给DSP，DSP再转给应用 */
    unsigned long long uiPackageStartTime; /* 包裹开始条带出现的时间（相对系统系统启动的时间，ms）*/
    unsigned long long uiTrigStartTime; /* 包裹开始触发光障的相对时间（系统启动后运行的时长），单位ms，该参数采传会透传给DSP，DSP再转给应用 */
    unsigned long long uiTrigEndTime;   /* 包裹离开光障的相对时间（系统启动后运行的时长），单位ms，该参数采传会透传给DSP，DSP再转给应用 */
    unsigned int uiColStartNo;          /* 包裹起始条带的序号，该参数采传会透传给DSP，DSP再转给应用 */
    unsigned int uiColEndNo;            /* 包裹结束条带的序号，该参数采传会透传给DSP，DSP再转给应用 */
    unsigned int uiNoramlDirection;     /* 正常过包的出图方向，后续回拉、转存的智能识别坐标会根据该值进行转换 */
    unsigned int uiIsVerticalFlip;      /* 正常过包时是否设置垂直镜像 */
    unsigned int uiIsForcedToSeparate;  /* 是否是强制分割的包裹，1-是，0-不是 */
    unsigned int uiZdataVersion;        /* 成像算法原子序数版本，对应XSP_ZDATA_VERSION，xspd(研究院双能) 1.3.0_201116版本及其以后使用V2，
                                           以前的版本使用V1，libxray和xsps一直是 V1，见XSP_ZDATA_VERSION)*/
    unsigned int uiPackTop;             /* 整包的上边界（YUV域的y坐标）*/
    unsigned int uiPackBottom;          /* 整包的下边界（YUV域的y坐标 + 高度h）*/
    unsigned int uiSvaResultTag;        /* 智能识别超时导致的无结果标识，0-超时异常，1-正常 */
} XSP_TRANSFER_INFO;

/**
 * @struct  XSP_PACK_INFO
 * @brief   XSP PROcess输出的信息
 */
typedef struct
{
    XSP_RESULT_DATA stResultData;       // XSP输出的图像识别信息，包括难穿透和可疑有机物
    XSP_TRANSFER_INFO stTransferInfo;   // 包裹分割相关信息
} XSP_PACK_INFO;

/**
 * @struct XSP_SLICE_NRAW
 * @brief  回调实时过包条带的归一化RAW数据 
 */
typedef struct
{
    unsigned char *pSliceNrawAddr;  /* 条带归一化RAW数据的Buffer地址 */
    unsigned int u32SliceNrawSize;  /* 条带归一化RAW数据的大小，单位：字节 */
    unsigned int u32Width;          /* 条带的宽度，探测器方向 */
    unsigned int u32Hight;          /* 条带的高度，传送带方向 */
    unsigned int uiColNo;           /* 条带的序号，采传透传给DSP，DSP再回调给应用 */
    unsigned long long u64SyncTime; /* 输入数据的时间，采传透传给DSP，DSP再回调给应用 */
    XSP_SLICE_CONTENT enSliceCont;  /* 包裹条带或空白条带 */
} XSP_SLICE_NRAW;


/****************************************************************************/
/*                                  X光功能相关参数                         */
/****************************************************************************/

/*
    X光数据交互魔术数定义
 */
#define XRAY_ELEMENT_MAGIC	(0xd5ca6e7f)
#define XRAY_PACKAGE_MAGIC	(0xd8ce9a6f)

/*
    X光数据坏点数
 */
#define XRAY_MAX_DEAD_PIXEL_NUM	100   /* 坏点最大个数 */

/*
    单次数据中包裹数
 */
#define XRAY_MAX_PACK_NUM 1          /* 包裹最大个数 */


/*
    XRAY转存目的类型
 */
typedef enum
{
    XRAY_TRANS_BMP = 0,               /* 转存为BMP图片 */
    XRAY_TRANS_JPG = 1,               /* 转存为JPG图片 */
    XRAY_TRANS_GIF = 2,               /* 转存为GIF图片 */
    XRAY_TRANS_PNG = 3,               /* 转存为PNG图片 ，目前只有X86在用 */ 
    XRAY_TRANS_TIF = 4,               /* 转存为TIFF图片，目前只有X86在用    */
    XRAY_TRANS_BUTT
} XRAY_TRANS_TYPE;

/*
    XRAY转存图像处理类型
 */
typedef union 
{
    struct XSP_PROC_TYPE_ST
    {
        unsigned int XSP_PROC_DISP           : 1;       /* 显示模式 */
        unsigned int XSP_PROC_ANTI           : 1;       /* 反色 */
        unsigned int XSP_PROC_EE             : 1;       /* 边增 */
        unsigned int XSP_PROC_UE             : 1;       /* 超增 */
        unsigned int XSP_PROC_LE             : 1;       /* 局增 */
        unsigned int XSP_PROC_LP             : 1;       /* 低穿 */
        unsigned int XSP_PROC_HP             : 1;       /* 高穿 */
        unsigned int XSP_PROC_ABSOR          : 1;       /* 可变吸收率 */
        unsigned int XSP_PROC_LUMI           : 1;       /* 亮度 */
        unsigned int XSP_PROC_PSEDOR         : 1;       /* 伪彩，仅单能有效 */
        unsigned int XSP_PROC_ORI            : 1;       /* 原始模式 */
        unsigned int XSP_PROC_DEF            : 1;       /* 默认成像风格 */
        unsigned int XSP_PROC_COLORTABLE     : 1;       /* 颜色表 */
        unsigned int XSP_PROC_COLORSIMAGING  : 1;       /* 三色六色显示 */
        unsigned int XSP_PROC_BGCOLOR        : 1;       /* 背景颜色 */
        unsigned int XSP_PROC_RES            : 17;      /* 保留 */
    } stXspProcType;
    unsigned int u32XspProcType;
} XSP_PROC_TYPE_UN;

/*
    XRAY方向
 */
typedef enum
{
    XRAY_DIRECTION_NONE = 0,          /* 出图方向，无 */
    XRAY_DIRECTION_RIGHT = 1,         /* 出图方向，目的右，左--->右，L2R */
    XRAY_DIRECTION_LEFT = 2,          /* 出图方向，目的左，右--->左，R2L */
    XRAY_DIRECTION_BUTT
} XRAY_PROC_DIRECTION;

/*
    XRAY速度
 */
typedef enum
{
    XRAY_SPEED_LOW = 0,               /* 出图速度，低 */
    XRAY_SPEED_MIDDLE = 1,            /* 出图速度，中 */
    XRAY_SPEED_HIGH = 2,              /* 出图速度，高 */
    XRAY_SPEED_SHIGH = 3,             /* 出图速度，超高 */
    XRAY_SPEED_UHIGH = 4,             /* 出图速度，极高 */
    XRAY_SPEED_SUHIGH = 5,            /* 出图速度，超极高 */
    XRAY_SPEED_NUM                    /* 出图速度数 */
} XRAY_PROC_SPEED;

/* XRAY包裹形态，与XRAY速度同时使用 */
typedef enum
{
    XRAY_FORM_ORIGINAL      = 0,        /* 显示原始比例 */
    XRAY_FORM_STRETCH       = 1,        /* 显示横向拉伸 */
    XRAY_FORM_NUM
} XRAY_PROC_FORM;

// XRAY-RAW数据类型
typedef enum
{
    XRAW_DT_UNDEF,          // 未定义的数据类型，预留
    XRAW_DT_BOOT_CORR,      // 开机校正数据
    XRAW_DT_MANUAL_CORR,    // 用户手动触发校正数据
    XRAW_DT_AUTO_CORR,      // 过包前自动校正数据
    XRAW_DT_AUTO_UPDATE,    // 成像算法库自动更新模板
    XRAW_DT_PACKAGE_SCANED  // 过包源扫描数据，未经任何处理
}XRAW_DT; // XRAW DATA TYPE，数据类型

/**
 * @struct  XRAY_PREVIEW
 * @brief   实时过包条带
 */
typedef struct
{
    XRAY_PROC_DIRECTION direction;  /* 出图方向 */
    unsigned int columnNo;          /* 条带列号 */
    XRAY_PROC_SPEED speed;          /* 出图速度 */
    unsigned int packageSign[8];    /* 是否有包裹，按位表示，0-无，1-有，低位对应先扫描行数据，高位对应后扫描行数据 */
    unsigned long long uiTrigTime;        /* 包裹触发光障的时间，单位ms */
} XRAY_PREVIEW;

/**
 * 回拉share buffer中回拉帧的存放格式： 
 * {XRAY_ELEMENT + [XRAY_PB_SLICE_HEADER + (XRAY_PB_PACKAGE_RESULT)? + 条带数据] * N } * M
 * 其中，XRAY_PB_PACKAGE_RESULT结构体是可选的，根据XRAY_PB_SLICE_HEADER中的bAppendPackageResult标记来判断，
 * 若该标记为TRUE，则后面继续紧跟结构体XRAY_PB_PACKAGE_RESULT，在包裹START和END时，需要携带包裹信息
 */

/**
 * @struct  XRAY_PLAYBACK
 * @brief   回拉帧头信息
 */
typedef struct
{
    XRAY_PROC_DIRECTION direction;  /* 出图方向，XRAY_DIRECTION_RIGHT - L2R，XRAY_DIRECTION_LEFT - R2L */
    XRAY_PB_TYPE uiWorkMode;        /* 0-整包回拉（文件管理转回拉、TIP抠图），1-条带回拉（实时预览转回拉） */
    unsigned int uiSliceNum;        /* 回拉帧中条带个数，即XRAY_PB_SLICE_HEADER的个数，半个条带计一个条带，XSP_SLICE_NEIGHBOUR类型的条带也要计入 */
    unsigned int uiPackageNum;      /* 回拉帧中包裹个数 */
} XRAY_PLAYBACK;

/**
 * @struct  XRAY_PB_PACKAGE_RESULT
 * @brief   回拉帧中的包裹信息，包含智能识别和包裹分割信息 
 * @note    若是条带回拉，在包裹起始和结束时最后一个条带写入 
 * @note    若是整包回拉，在包裹数据前写入 
 */
typedef struct
{
    unsigned int u32PackageId;      /* 包裹ID号 */
    unsigned int u32PackageWidth;   /* 包裹宽，基于RAW数据 */
    unsigned int u32PackageHeight;  /* 包裹高，基于RAW数据 */
    SVA_DSP_OUT stSvaInfo;          /* 智能信息 */
    XSP_PACK_INFO stProPrm;           /* 成像信息 */
} XRAY_PB_PACKAGE_RESULT;

/**
 * @struct  XRAY_PB_SLICE_HEADER
 * @brief   回拉帧中的条带信息 
 * @note    若是整包回拉，整个包裹当做是一个大条带处理 
 */
typedef struct
{
    unsigned long long u64ColNo;    /* 条带的序号 */
    unsigned int u32Size;           /* 条带数据大小，若为空白条带，且该值为0时，DSP会根据u32Width和u32HeightIn构建条带，单位：字节 */
    unsigned int u32Width;          /* 条带的宽度，探测器方向 */
    unsigned int u32HeightIn;       /* 条带的输入高度，传送带方向，小于u32HightTotal时表示仅有半个条带 */
    unsigned int u32HeightTotal;    /* 条带的总高度，传送带方向，等于u32HeightIn时表示整个条带 */
    XSP_SLICE_CONTENT enSliceCont;  /* 空白条带、包裹条带，仅用于是否存储条带数据的标识，不作为包裹分割的依据，包裹分割需要根据包裹回调的START和END判断 */
    XSP_PACKAGE_TAG enPackTag;      /* 包裹起始、结束标记 */
    int bAppendPackageResult;       /* 该结构体后是否有追加包裹信息XRAY_PB_PACKAGE_RESULT */
} XRAY_PB_SLICE_HEADER;

/**
 * 应用与DSP共享Buffer的数据头信息 
 * 共享buffer有两种：①实时过包，DSPINITPARA中的xrayShareBuf，②回拉，DSPINITPARA中的xrayPbBuf 
 */
typedef struct
{
    unsigned int magic;               /* 常数，供定位，值为XRAY_ELEMENT_MAGIC */
    unsigned int chan;                /* 通道号 */
    XRAY_PROC_TYPE type;              /* 数据类型，XRAY_PROC_TYPE */
    unsigned long long time;                /* 数据获取的时间，从开机起，单位为ms */

    unsigned int width;               /* 数据宽度 */
    unsigned int height;              /* 数据高度 */
    unsigned int dataLen;             /* 除该结构体外的数据长度 */

    union
    {
        XRAY_PREVIEW stXrayPreview;   /* 正常预览参数 */
        XRAY_PLAYBACK stXrayPlayback; /* 回拉处理参数 */
    } xrayProcParam;
} XRAY_ELEMENT;


/*
    XRAY处理参数
 */
typedef struct
{
    unsigned int marginTop;           /* 留白，上端留白的像素 */
    unsigned int marginBottom;        /* 留白，下端留白的像素 */
} XRAY_PARAM;

/*
    XRAY诊断回调参数
 */
typedef struct
{
    unsigned char *pu8DiagData;       /* 诊断的数据     */
    unsigned int u32DiagSize;         /* 诊断的数据大小 */
    unsigned int u32Width;            /* 诊断的数据宽 */
    unsigned int u32Height;           /* 诊断的数据长 */
} XRAY_DIAGNOSE_RESULT;

/*
    XRAY转存回调参数
 */
typedef struct
{
    XRAY_TRANS_TYPE transType;        /* 转存的数据类型 */
    unsigned int transParam;          /* 转存透传的参数*/
    unsigned char *pcTransData;       /* 转存的数据指针 */
    unsigned int u32TransSize;        /* 转存的数据大小 */
} XRAY_TRANSFER_RESULT;

/*
    XRAY坏点
 */
typedef struct
{
    unsigned short offset;              // 坏点位置
    unsigned short classify;            // 坏点类型，0-NA，1-本底亮坏点，2-满载暗坏点
    unsigned char energy_board;         // 坏点所在能量板，0-NA，1-低能板，2-高能板
    unsigned char handle_method;        // 处理方式
    unsigned char res[3];
} XRAY_DEADPIXEL;

/*
    XRAY坏点回调参数, 连续3个及以上坏点，总数超过10个坏点上报
 */
typedef struct
{
    unsigned int deadPixelNum;       /* 坏点个数 */
    XRAY_DEADPIXEL deadPixel[XRAY_MAX_DEAD_PIXEL_NUM];  /* 坏点信息 */
} XRAY_DEADPIXEL_RESULT;

/**
 * @struct  XRAY_TRANS_GET_BUFFER_SIZR_PARAM
 * @brief   获取存放转存图片内存大小的处理参数
 */
typedef struct
{
    unsigned int u32PackageWidth;       ///< 输入参数，保存的整个包裹RAW数据的宽
    unsigned int u32PackageHeight;      ///< 输入参数，保存的整个包裹RAW数据的高
    XRAY_TRANS_TYPE enImgType;          ///< 输入参数，转存图片类型
    unsigned int u32ImgBufSize;         ///< 输出参数，转存图片占用内存大小
} XRAY_TRANS_GET_BUFFER_SIZR_PARAM;

/**
 * @struct  XRAY_TRANS_PROC_PARAM
 * @brief   归一化数据转存成图片的处理参数
 */
typedef struct
{
    unsigned int u32PackageWidth;       ///< 输入参数，保存的整个包裹RAW数据的宽，64*n，5030为448，6550为640
    unsigned int u32PackageHeight;      ///< 输入参数，保存的整个包裹RAW数据的高
    unsigned char *pu8RawBuf;           ///< 输入参数，包裹归一化RAW Buffer，默认读取的数据长度为：
                                        ///< 单能：package_width * package_height * 2 
                                        ///< 双能：package_width * package_height * 5 
    unsigned int u32RawSize;            ///< 输入参数，包裹归一化RAW数据实际大小，单位：字节
    XRAY_TRANS_TYPE enImgType;          ///< 输入参数，转存图片类型，参考枚举XRAY_TRANS_TYPE
    XSP_PROC_TYPE_UN unXspProcType;     ///< 输入参数，转存图片图像处理类型
    XSP_RT_PARAMS stXspProcParam;       ///< 输入参数，转存图片图像处理参数
    unsigned char *pu8ImgBuf;           ///< 输出参数，转存后JPG/BMP图片存放的Buffer
    unsigned int u32ImgBufSize;         ///< 输入参数，图像存放的Buffer大小，值通过HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE命令获取
    unsigned int u32ImgDataSize;        ///< 输出参数，图像数据实际大小
    SVA_DSP_OUT stSvaInfo;              ///< 智能信息
    XSP_PACK_INFO stXspInfo;              ///< 成像信息
} XRAY_TRANS_PROC_PARAM;

/**
 * @struct  XRAY_SPEED_PARAM
 * @brief   切换传送带速度参数
 */
typedef struct
{
    XRAY_PROC_SPEED enSpeedUsr;         ///< 用户选择的过包速度，低速、中速、高速
    XRAY_PROC_FORM enFormUsr;           ///< 用户选择的包裹形态，原始、拉伸
    unsigned int res[2];
} XRAY_SPEED_PARAM;

/**
 * @struct  XRAY_NRAW_SPLIT_PARAM
 * @brief   归一化RAW数据拆分参数
 */
typedef struct
{
    unsigned int u32SrcWidth;           ///< 输入参数，归一化RAW数据宽
    unsigned int u32SrcHeight;          ///< 输入参数，归一化RAW数据高
    unsigned char *pSrcBuf;             ///< 输入参数，归一化RAW数据Buffer
    unsigned int u32StartLine;          ///< 输入参数，需截取的RAW数据起始行（包含），取值范围[0, u32SrcHeight-1]
    unsigned int u32EndLine;            ///< 输入参数，需截取的RAW数据结束行（包含），取值范围[0, u32SrcHeight-1]
                                        ///< 正向截取时，u32StartLine < u32EndLine；反向截取时，u32StartLine > u32EndLine
    unsigned char *pDestBuf;            ///< 输出参数，截取的归一化RAW数据Buffer
} XRAY_NRAW_SPLIT_PARAM;

/**
 * @struct  XRAY_NRAW_SPLICE_PARAM
 * @brief   归一化RAW数据拼接参数，将若干个连续条带数据拼接成一整块
 * 
 * @note    以双能为例，拼接2个条带为一整块平面归一化RAW数据：
 * 00------------- 低能 -------------
 * 01------------- 低能 -------------
 * 02------------- 高能 -------------
 * 03------------- 高能 -------------
 * 04----------- 原子序数 -----------
 * 05----------- 原子序数 -----------
 * 06------------- 低能 -------------
 * 07------------- 低能 -------------
 * 08------------- 高能 -------------
 * 09------------- 高能 -------------
 * 10----------- 原子序数 -----------
 * 11----------- 原子序数 -----------
 *
 *   ↓========== 正序拼接后 ==========↓                ↓========== 逆序拼接后 ==========↓
 *
 * 00------------- 低能 -------------               06------------- 低能 -------------
 * 01------------- 低能 -------------               07------------- 低能 -------------
 * 06------------- 低能 -------------               00------------- 低能 -------------
 * 07------------- 低能 -------------               01------------- 低能 -------------
 * 02------------- 高能 -------------               08------------- 高能 -------------
 * 03------------- 高能 -------------               09------------- 高能 -------------
 * 08------------- 高能 -------------               02------------- 高能 -------------
 * 09------------- 高能 -------------               03------------- 高能 -------------
 * 04----------- 原子序数 -----------                 10----------- 原子序数 -----------
 * 05----------- 原子序数 -----------                 11----------- 原子序数 -----------
 * 10----------- 原子序数 -----------                 04----------- 原子序数 -----------
 * 11----------- 原子序数 -----------                 05----------- 原子序数 -----------
 */
typedef struct
{
    unsigned int u32SliceOrder;         ///< 输入参数，拼接顺序 0为正序 1位逆序
    unsigned int u32SrcWidth;           ///< 输入参数，归一化RAW数据宽
    unsigned int u32SrcHeightTotal;     ///< 输入参数，归一化RAW数据总高
    unsigned int u32BufSize;            ///< 输入参数，输入数据大小
    unsigned int u32SliceHeight;        ///< 输入参数，单个条带的高
    unsigned char *pSrcBuf;             ///< 输入参数，归一化RAW数据Buffer
    unsigned char *pDestBuf;            ///< 输出参数，截取的归一化RAW数据Buffer
} XRAY_NRAW_SPLICE_PARAM;

/**
 * @struct  XRAY_NRAW_RESIZE_PARAM
 * @brief   缩放NRAW数据尺寸至指定宽高
 */
typedef struct
{
    unsigned char *pSrcBuf;             ///< 输入参数，缩放前的归一化RAW数据Buffer
    unsigned char *pDestBuf;            ///< 输出参数，缩放后的归一化RAW数据Buffer
    unsigned int u32SrcW;               ///< 输入参数，源归一化数据NRAW宽
    unsigned int u32SrcH;               ///< 输入参数，源归一化数据NRAW高
    unsigned int u32DstW;               ///< 输入参数，目标归一化数据NRAW宽
    unsigned int u32DstH;               ///< 输入参数，目标归一化数据NRAW高
} XRAY_NRAW_RESIZE_PARAM;

/**
 * @struct  XRAY_SHARE_BUF
 * @brief   共享循环Buffer，应用写入需要处理的RAW数据、编解码数据，DSP读取
 */
typedef struct
{
    unsigned char *pPhysAddr;         /* DSP访问地址 */
    unsigned char *pVirtAddr;         /* ARM访问地址 */
    volatile unsigned int bufLen;     /* 缓冲区长度 */
    volatile unsigned int writeIdx;   /* 缓冲写索引 */
    volatile unsigned int readIdx;    /* 缓冲读索引 */
} XRAY_SHARE_BUF;

/**
 * @struct PACKAGE_HEADER
 * @brief 存储的包裹头信息,包裹信息头(64Byte)+图像处理信息+智能信息+包裹数据
 */
typedef struct _XRAY_PACKAGE_HEADER_
{
    unsigned int startCode;        /* 起始码, 用于校验, 0xd8ce9a6f      */
    unsigned char picture : 1;     /* 是否存在图像处理信息          */
    unsigned char smart : 1;       /* 是否存在智能处理信息          */
    unsigned char extBit : 6;      /* 扩展保留字段 */
    unsigned char version;         /* 版本信息                      */
    unsigned char type;            /* 数据类型                      */
    unsigned char res;             /* 保留信息，默认为0            */
    unsigned short width;          /* 包裹中数据，宽度              */
    unsigned short height;         /* 包裹中数据，高度              */

    unsigned int packageOffset;    /* 包裹数据的偏移量              */
    unsigned int xPackLen;         /* 包裹数据总长度，不包含本包裹头    */
    unsigned int pictureOffset;    /* 图像处理信息偏移量            */
    unsigned int pictureLen;       /* 图像处理信息长度              */
    unsigned int smartOffset;      /* 智能处理信息偏移量            */
    unsigned int smartLen;         /* 智能处理信息长度              */
    unsigned int ext[7];           /* 保留信息             */
} XRAY_PACKAGE_HEADER;

// 回调保存的源RAW数据结构体
typedef struct
{
    XRAW_DT enDataType;                   // 数据类型
    void *pXrawBuf;                       // 源RAW数据地址
    unsigned int uiXrawSize;              // 源RAW数据大小
    unsigned long long syncTimeStart;     // 源RAW数据开始时间
    unsigned long long syncTimeEnd;       // 源RAW数据结束时间
} XRAY_RAW_DATA;

/**
 * @struct  XRAY_RAW_FULL_DATA
 * @brief   回调保存的满载校正源RAW数据结构体
 */
typedef struct
{
    XRAY_PROC_TYPE type;            // 类型，仅支持XRAY_TYPE_CORREC_FULL和XRAY_TYPE_CORREC_ZERO
    unsigned char *pCorrBuf;        // 本底、满载校正数据Buffer
    unsigned int u32BufSize;        // Buffer长度，输入参数，不少于探测器像素×sizeof(UINT16)，单位：字节
    unsigned int u32CorrDataLen;    // Buffer中本底校正数据的实际大小，输出参数，单位：字节
} XRAY_RAW_FULL_OR_ZERO_DATA;

/****************************************************************************/
/*                              移动侦测与遮挡                              */
/****************************************************************************/
/* 移动侦测设置 */
#define MD_BLOCK_W		(32)                            /* 块宽度     */
#define MD_BLOCK_H		(32)                            /* 块高度     */
#define MD_BLOCK_CNT	(704 / MD_BLOCK_W)              /* 每行块个数 */
#define MD_MAX_LINE_CNT (576 / MD_BLOCK_H)              /* 最大的行数 */
#define MD_MAX_LEVEL	(6)                             /* 灵敏度级别，最大为6(灵敏度最低)，最小为0(灵敏度最高) */

/* 遮挡报警设置 */
#define CD_MAX_LEVEL 15

/* 屏蔽设置 */
#define MASK_MAX_REGION 4               /* 最多支持区域个数 */
#define MASK_ALIGN_H	8               /* W、X对齐 */
#define MASK_ALIGN_V	2               /* H、Y对齐 */

/*
    移动侦测的配置信息  HOST_CMD_SET_MD
 */
typedef struct
{
    unsigned int level;                         /* 级别0(灵敏度最高)-6(灵敏度最低),默认为2 */
    unsigned int fastInterval;                  /* 快速检测侦间隔，默认为3 */
    unsigned int slowInterval;                  /* 慢速检测帧间隔，默认为15 */
    unsigned int delay;                         /* 延时 */
    unsigned int bNeedDetail;                   /* 如果为真，DSP会将移动侦测的详细结构上传给主机 */
    unsigned int mask[MD_MAX_LINE_CNT];         /* 屏蔽值，对应位为1时判断该块，只有在bNeedDetail为FALSE时才有效 */
    unsigned int bAuto;                         /* 是否自适应 */
} MD_CONFIG;

/*
    遮挡报警的配置信息  HOST_CMD_SET_CD
 */
typedef struct
{
    unsigned int bEnable;                       /* 使能 */
    unsigned int level;                         /* 遮挡级别  */
    unsigned int delay;                         /* 延时，暂时未使用 */
    unsigned int x;                             /* 遮挡区域位置参数 */
    unsigned int y;
    unsigned int w;
    unsigned int h;
} CD_CONFIG;


/*
    屏蔽的配置信息  HOST_CMD_SET_MASK
 */
typedef struct
{
    unsigned int regionCnt;                     /* 区域个数 */
    unsigned int x[MASK_MAX_REGION];            /* 区域位置参数 */
    unsigned int y[MASK_MAX_REGION];
    unsigned int w[MASK_MAX_REGION];
    unsigned int h[MASK_MAX_REGION];
    unsigned int flgMaskEnable;                 /* 使能 */
} MASK_CONFIG;

/*
    水印数据信息
    HOST_CMD_SET_WATERMARK
    HOST_CMD_SET_SUB_WATERMARK
 */
typedef struct
{
    unsigned int global_time;       /*当前的全局时间(绝对时间)，需要和GROUP_HEADER中的时间相同*/
    unsigned int device_sn;         /*设备的序列号(整数表示，非BCD码)*/
    unsigned char mac_addr[6];      /*码流所属设备的MAC地址*/
    unsigned char device_type;      /*设备的类型(型号)*/
    unsigned char device_info;      /*设备的附加信息(区域、语言、版本等)*/
    unsigned char channel_num;      /*码流所属的通道号*/
    unsigned char reserved[47];     /*填补，保证按64对齐*/
} WATERMARK_VER1;

/*
    移动侦测结果返回 STREAM_ELEMENT_MD_RESULT
 */
typedef struct tagMdCbResultSt
{
    unsigned int uiMoved;                      /* 移动侦测状态:0 静止1 :移动 */
    unsigned int uiMdResult[MD_MAX_LINE_CNT];
} MD_CB_RESULT_ST;

/*
    包裹处理模块参数
 */
typedef enum xpackSaveTypeE
{
    XPACK_SAVE_SRC = 0,                   /* 原始图片 */
    XPACK_SAVE_BMP = 1,                   /* BMP 图片*/
    XPACK_SAVE_JPEG = 2,                  /* jpeg 图片*/
    XPACK_SAVE_BUTT
} XPACK_SAVE_TYPE_E;

/*
	xsp、xpack、sva透传结构体
*/
typedef struct tagAiPackPrm
{
    unsigned int timeRef;           /* 帧序号 */
    unsigned int packIndx;          /* 包裹索引 */
    unsigned int completePackMark;  /* 完整包裹标志 */
    unsigned int packTop;           /* 包裹上边沿的坐标 */
    unsigned int packBottom;        /* 包裹下边沿的坐标 */
    unsigned int packOffsetX;       /* 包裹最做边缘危险品距离前沿offset */
    unsigned int packStartX;        /* 包裹起始X位置 */
}AI_PACK_PRM;

typedef struct tagXpackMarkSt
{
    unsigned int inChn;                      /* 输入通道号 */
    unsigned int bSvaFlag;                   /* 是否保存sva智能信息 */
} XPACK_MARK_ST;

/*显示yuv画面上下偏移结构体
*/
typedef struct tagXpackYuvShowOffset
{
	int reset;		/*复位 剧中画面*/
	int offset;		/*相对于上一次的偏移量*/
}XPACK_YUVSHOW_OFFSET;


/* xpack智能信息回调结构体 */
typedef struct tagXpackSvaResultOut
{
    unsigned int target_num;                                        /* 报警数 */
    SVA_DSP_ALERT target[SVA_XSI_MAX_ALARM_NUM];                    /* 智能信息 */
} XPACK_SVA_RESULT_OUT;

typedef struct tagXpackSavePrm
{
    unsigned int bSvaFlag;                   /* 输入参数 是否画智能信息，暂不支持 */
    XPACK_SAVE_TYPE_E outType;               /* 输入参数 输出图片类型 */
    unsigned int u32Width;                   /* 输出参数 图像宽 */
    unsigned int u32Height;                  /* 输出参数 图像高 */
	unsigned int u32DataSize;                /* 输出参数 返回数据实际大小 */
    XSP_PACK_INFO igmPrm;                    /* 输出参数 图像处理信息 */
    XPACK_SVA_RESULT_OUT savPrm;             /* 输出参数 智能处理信息 */
	unsigned int uiSyncNum;                  /* 输出参数 BMP数据同步帧索引 */
	char *pOutBuff;                          /* 输入参数 save数据 buff由调用者申请 */
    unsigned int u32BuffSize;                /* 输入参数 应用申请的buff大小，用于校验防止拷贝越界 */
} XPACK_SAVE_PRM;


/* xpack分片信息回调结构体 */
typedef struct tagXpackSegmentData
{
	XPACK_SAVE_TYPE_E SegmentDataTpye;       /* 分片数据类型 */
	unsigned int u32SegmentWidth;            /* 回调图片宽 */
	unsigned int u32SegmentHeight;           /* 回调图片高 */
	unsigned int u32DataSize;                /* 返回数据实际大小 */
    char *pOutBuff;                          /* 回调数据地址   */
	unsigned int u32BuffSize;                /* 实际的buff大小，用于校验防止拷贝越界 */
} XPACK_SEGMENT_DATA;

/* xpack集中判图数据回调结构体 */
typedef struct tagXpackDspSegmentOut
{
    XRAY_PROC_DIRECTION  Direction;          /* 实时过包方向 */
    unsigned int u32PackIndx;                /* 包裹序号 */
    unsigned int flag;                       /* 包裹位置 0-开始，1-中间，2-结束   */
	unsigned int uiIsForcedToSeparate;       /* 是否是强制分割的包裹，1-是，0-不是 */
    unsigned int u32SegmentIndx;             /* 分片序号 */
	XPACK_SEGMENT_DATA stSegmentDataPrm;
} XPACK_DSP_SEGMENT_OUT;

/* xpack集中判图参数设置结构体 */
typedef struct tagXpackDspSegmentSet
{
    unsigned int SegmentWidth;   /*分片宽度 必须大于32 且是4对齐*/
	unsigned int bSegFlag;        /*分片开关     0-关闭 1-开启 */
	XPACK_SAVE_TYPE_E SegmentDataTpyeTpye;      /*分片数据类型 */
} XPACK_DSP_SEGMENT_SET;

/*
    TIP/可疑物 危险品数据保存
 */
typedef struct tagOrgSvaPrm
{  
   unsigned int   orgNameFlag;//有机物标记名字是否有修改 1修改
   unsigned char  orgname[SVA_ALERT_NAME_LEN];//有机物名称
   unsigned int   orgScaleFlag;//有机物标记缩放等级是否有修改 1修改
   unsigned int   orgScaleLevel;//有机物缩放等级
   unsigned int   orgColorFlag;//有机物标记背景色是否有修改 1修改
   unsigned int   orgBackColor; //有机物背景色
   unsigned int   orgDrawFlag;//有机物标记画线类型是否有修改 1修改
   unsigned int   orgDrawType;//有机物画线类型	 
   
   unsigned int   tipNameFlag;//tip标记名字是否有修改 1 修改
   unsigned char  tipname[SVA_ALERT_NAME_LEN];//tip名称
   unsigned int   tipScaleFlag;//tip标记缩放等级是否有修改 1修改
   unsigned int   tipScaleLevel;//tip缩放等级
   unsigned int   tipColorFlag;//tip标记背景色是否有修改 1修改
   unsigned int   tipBackColor; //tip背景色
   unsigned int   tipDrawFlag;//tip标记画线类型是否有修改 1修改
   unsigned int   tipDrawType;//tip画线类型
}ORG_SVAPRM;


/* 二维码扫码区域配置，使用图像像素偏移坐标 HOST_CMD_QR_START  */
typedef struct tagQrRect
{
    unsigned int roiEnable;                   /* ROI 使能开关 */
    unsigned int x;                           /* 扫码区域配置 */
    unsigned int y;
    unsigned int width;
    unsigned int height;
} QR_ROI_RECT;

/* TDCL_POINT_F 点坐标结构体 */
typedef struct _QR_POINT_F
{
    float x;                                /* x坐标 */
    float y;                                /* y坐标 */
} QR_POINT_F;

/* QR码编码信息 */
typedef struct _QR_CODE_INFO_
{
    unsigned char *charater;                /* 字符信息 */
    int len;                                /* 字符长度 */
} QR_CODE_INFO;

/* QR码中心点坐标、旋转角度结构体 */
typedef struct _QR_LOCATION_INFO
{
    QR_POINT_F centerPt;                    /* QR码中心点坐标 */

    QR_POINT_F start_pt;                    /* 拟合直线起始点 */
    QR_POINT_F end_pt;                      /* 拟合直线终止点 */
    float rotateAngle;                      /* QR码图像的旋转角度 */
} QR_LOCATION_INFO;

/* 二维码检测返回结果 STREAM_ELEMENT_QRCODE */
typedef struct tagQrCodeCbResultSt
{
    QR_CODE_INFO codeInfo;                  /* 编码信息 */
    QR_LOCATION_INFO locInfo;               /* QR码位置角度信息 */
} QR_CODE_RESULT;

/*  mark数返回结构体
 */
typedef struct tagMarkCbResultSt
{
    char *cXrayAddr;            /* 回调的mark数据的地址 */
    unsigned int uiXraySize;    /* 回调的mark数据的大小 */
    unsigned int width;         /* 回调的列数 */
    unsigned int hight;         /* 回调的行数 */
    XSP_PACK_INFO igmPrm;         /* 图像处理信息*/
    SVA_DSP_OUT savPrm;         /* 智能处理信息*/
} MARK_RESULT_ST;

/*  包裹分割原始数返回结构体
 */
typedef struct tagXpackCbResultSt
{
    char *cXrayAddr;            /* 回调的包裹RAW数据的地址(未超分)*/
    unsigned int uiOriXraySize;    /* 回调的包裹RAW数据的大小(未超分)，通过地址和size直接获取整块内存 */
    unsigned int uiXraySize;       /* 回调的包裹RAW数据的大小(已超分)，包裹归一化数据大小 */
    unsigned int packIndx;      /* 包裹序号 */
    unsigned int width;         /* 回调的包裹RAW数据的宽度，探测器方向(已超分) */
    unsigned int hight;         /* 回调的包裹RAW数据的高度，传送带方向(已超分)*/
    XSP_PACK_INFO igmPrm;       /* 图像处理信息 */
    XPACK_SVA_RESULT_OUT stPackSavPrm;       /* 包裹智能处理信息 */
    JPEG_SNAP_CB_RESULT_ST stJpgResultOut;   /* 回调的jpg信息 */
    XPACK_SVA_RESULT_OUT stJpgSavPrm;        /* jpg图片的智能处理信息 */
} XPACK_RESULT_ST;

/*  XPACK jpg抓图参数设置
 */
typedef struct tagXpackJpgSetSt
{
    unsigned int u32JpgBackWidth;       /* JPG背板宽 */
    unsigned int u32JpgBackHeight;      /* JPG背板高，宽高均为0表示不使用背板 */
    unsigned int bJpgDrawSwitch;        /* 画智能识别框开关，0-关闭，1-开启，默认关闭 */
    unsigned int bJpgCropSwitch;        /* 包裹抠取开关，0-关闭，1-开启，默认开启 */
    float f32WidthResizeRatio;          /* 横向缩放比例，范围[0.1, 1.0]，指标模式为1.0，正常过包模式为0.75，默认值为0.75 */
    float f32HeightResizeRatio;         /* 纵向缩放比例，范围[0.1, 1.0]，指标模式为1.0，正常过包模式为0.75，默认值为0.75 */
} XPACK_JPG_SET_ST;

/* 
硬件加密申请内存结构体
*/
typedef struct tagCipherAddrSt
{
    unsigned int         poolId;
	unsigned int         memSize;                /* 地址空间总大小 */
    unsigned long long   phyAddr;                /* 物理地址 */
    void*                virAddr;                /* 虚拟地址 */
}CIPHER_HAL_ADDR;

typedef struct tagCipherPrmSt
{
    unsigned int         CipherHandle;          /*  CIPHER 句柄    预留 */
    unsigned char        aes_key[16];           /*  加解密密钥      默认16个字节 */
    unsigned char        aes_IV[16];            /*  加解密初始向量        默认16个字节 */
    CIPHER_HAL_ADDR      CipherAddr;            /*  加解密地址 */
}CIPHER_TSK_PRM;


/****************************************************************************/
/*                                  系统                                    */
/****************************************************************************/

/* DSP程序的版本信息 */
typedef struct tagDspSysVersion
{
    int mainVersion;                     /* DSP库接口的主版本 */
    int subVersion;                      /* DSP库接口的子版本 */
    int svnVersion;                      /* DSP程序库的代码svn版本号 */
    char buildData[32];                  /* DSP程序库编译日期 */
    char buildTime[32];                  /* DSP程序库编译时间 */
} DSP_SYS_VERSION;


/* 成像算法XSP库的来源*/
typedef enum
{
    XSP_LIB_RAYIN           = 0x0,      // 本业务部开发的普通XSP库
    XSP_LIB_RAYIN_AI        = 0x1,      // 本业务部开发的AI-XSP库, 运行于X86核显，以及RK睿影自研AI，在5030骨架统一项目上使用
    XSP_LIB_RAYIN_AI_EXT    = 0x2,      // 本业务部开发的AI-XSP库，运行于独显
    XSP_LIB_RAYIN_AI_OTH    = 0x3,      // 本业务部集成的研究院AI-XSP库
    XSP_LIB_FOREIGN         = 0x10,     // 研究院开发的XSP库
    XSP_LIB_FOREIGN_AI      = 0x11,     // 研究院开发的AI-XSP库

    /*可以扩展*/
    XSP_LIB_NUM
}XSP_LIB_SRC;

/* 单双模型区分 */
typedef enum tagModelType
{
    SINGLE_MODEL_TYPE = 0,       /* 目前分析仪已经删除了单模型版本 */
	DOUBLE_MODEL_TYPE,
	MODEL_TYPE_NUM,
} MODEL_TYPE_E;

/* 单双芯片设备区分 */
typedef enum tagDeviceChipType
{
    SINGLE_CHIP_TYPE = 0,
	DOUBLE_CHIP_MASTER_TYPE,
	DOUBLE_CHIP_SLAVE_TYPE,
    DOUBLE_CHIP_ISM_MASTER_TYPE,        /*安检机(双芯片)主片设备*/
    DOUBLE_CHIP_ISM_SLAVE_TYPE,         /*安检机(双芯片)从片设备*/
	DEVICE_CHIP_TYPE_NUM,
} DEVICE_CHIP_TYPE_E;

/* 产品类型 */
typedef enum
{
    PRODUCT_TYPE_UNDEF  = 0,    /* 预留 */
    PRODUCT_TYPE_ISA    = 1,    /* 分析仪，Intelligent Security Machine */
    PRODUCT_TYPE_ISM    = 2,    /* 安检机，Intelligent Security Machine */
    PRODUCT_TYPE_NUM,
} PRODUCT_TYPE_E;

/* 语言类型 */
typedef enum
{
    LANGUAGE_TYPE_UNDEF      = 0,    /* 预留 */
    LANGUAGE_TYPE_SIMP_CN    = 1,    /* 中文*/
    LANGUAGE_TYPE_ENGLISH    = 2,    /* 英文 */
    LANGUAGE_TYPE_NUM,
} LANGUAGE_TYPE_E;


/* X-Ray探测板供应商 */
typedef enum 
{ 
    XRAYDV_DT          = 1,     /*DT探测板*/
    XRAYDV_SUNFY,               /*尚飞探测板*/
    XRAYDV_IRAY,                /*奕瑞探测板*/
    XRAYDV_HAMAMATSU,           /*滨松探测板*/
    XRAYDV_TYW,                 /*同源微探测板*/
    XRAYDV_VIDETECT,            /*中威晶源探测板*/
    XRAYDV_RAYINZIYAN,          /*睿影自研探测板*/
    XRAYDV_DTCA,                /*DT探测板，民航专用*/
    XRAYDV_RAYINZIYAN_QIPAN,    /*睿影自研探测板，奇攀的X-sensor，睿影的底板*/
    XRAYDV_NAME_MAX, 
} XRAY_DET_VENDOR;


/* X光机能量类型数 */
typedef enum
{
    XRAY_ENERGY_N_UND = 0,          /* 预留 */
    XRAY_ENERGY_SINGLE = 1,         /* 单能，分为单低能和单高能，缩写代码SE */
    XRAY_ENERGY_DUAL = 2,           /* 双能，低能+高能，缩写代码DE */
} XRAY_ENERGY_NUM;

/* X-RAY速度参数 */
typedef struct tagXraySpeedSt
{
    unsigned int integral_time;     /* 积分时间，单位：us */
    unsigned int slice_height;      /* 实时过包各速度的条带高度 */
    float belt_speed;               /* 传送带速度，单位：m/s */
} XRAY_SPEED_PARAM_ST;

/* 
 * 安检机机型大类，0~3位表示SG、SC子型号，第8~11位表示机型大类
 */
typedef enum ISM_DEVTYPE
{
    ISM_NUDXX      = 0x0,
    ISM_SC6550XX   = 0x100,
    ISM_SG6550XX   = 0x101,
    ISM_SC5030XX   = 0x200,
    ISM_SG5030XX   = 0x201,    // 预留
    ISM_SC100100XX = 0x300,
    ISM_SG100100XX = 0x301,    // 预留
    ISM_SC4233XX   = 0x400,
    ISM_SG4233XX   = 0x401,    // 预留
    ISM_SC140100XX = 0x500,
    ISM_SG140100XX = 0x501,    // 预留
} ISM_DEVTYPE_E;

/* 
 * 射线源型号，24~31位表示厂商，第16~23位表示源型号
 */
typedef enum
{
    /* 射线源厂商-机电院 */
    XRAY_SOURCE_JDY_T160      = 0x00000000,   //射线源型号-T160
    XRAY_SOURCE_JDY_T140      = 0x00010000,   //T140
    XRAY_SOURCE_JDY_T80       = 0x00020000,   //T80
    XRAY_SOURCE_JDY_TD120     = 0x00030000,   //TD120  4233
    XRAY_SOURCE_JDY_T120      = 0x00040000,   //T120   5030D
    XRAY_SOURCE_JDY_T160YT    = 0x00050000,   //160YT
    XRAY_SOURCE_JDY_T2050     = 0x00060000,   //T2050
    XRAY_SOURCE_JDY_T140RT    = 0x00070000,   //机电院RT140
    XRAY_SOURCE_JDY_TM80      = 0x000A0000,   //模拟源
    /* 射线源厂商-凯威信达 */
    XRAY_SOURCE_KWXD_X160     = 0x01000000,   //X160
    /* 射线源厂商-超群 */
    XRAY_SOURCE_CQ_T200       = 0x03010000,   //C200
    /* 射线源厂商-博思得 */
    XRAY_SOURCE_BSD_T160      = 0x04000000,   //博思得T160
    XRAY_SOURCE_BSD_T140      = 0x04000001,   //博思得T140
    /* 射线源厂商-spellman */
    XRAY_SOURCE_SPM_S180      = 0x05000000,   //S180
    /* 射线源厂商-力能 */
    XRAY_SOURCE_LIOENERGY_LXB80 = 0x06000000,    //力能  LXB-80/48B5-D

} XRAY_SOURCE_TYPE_E;

/* 安检机XRay图像显示模式 */
typedef enum
{
    ISM_DISP_MODE_DEFA              = 0, /* 默认模式，即一个屏幕显示一路输入源 */
    ISM_DISP_MODE_DOUBLE_UPDOWN     = 1, /* 单屏双显，上下两路源 */
    ISM_DISP_MODE_DOUBLE_LEFTRIGHT  = 2, /* 单屏双显，左右两路源 */
} ISM_DISP_MODE_E;

typedef struct
{
    float resize_width_factor;      /* RAW图像横向（探测器方向）缩放系数，范围(0, 2.0] */
    float resize_height_factor;     /* RAW图像纵向（时间方向）缩放系数，范围(0, 2.0] */
} XSP_RESIZE_FACTOR;

/**
 * 应用下发能力级相关参数
 *
 * 关于padding_top、padding_bottom、blanking_top与blanking_bottom的图示 
 *  --------------------------------
 *  |         blanking_top         |         过包画面向下可拖动距离
 *  --------------------------------      _
 *  |         padding_top          |       |
 *  --------------------------------       |
 *  |                              |       |
 *  |         Dispplay YUV         |       | 过包画面显示区域
 *  |                              |       |
 *  --------------------------------       |
 *  |        padding_bottom        |       |
 *  --------------------------------      _|
 *  |        blanking_bottom       |         过包画面向上可拖动距离
 *  --------------------------------
 */
typedef struct tagDspCapbParamSt
{
    PRODUCT_TYPE_E dev_tpye;                /* 设备类型 */
    ISM_DEVTYPE_E ism_dev_type;	            /* 安检机机型大类 */
    XRAY_SOURCE_TYPE_E aenXraySrcType[MAX_XRAY_CHAN];       /* 安检机射线源类型 */
    unsigned int xray_width_max;            /* X-RAY源输入数据宽最大值，即探测板像素数×探测板数量 */
    unsigned int xray_height_max;           /* X-RAY源输入数据高最大值，即全屏显示横向对应的RAW数据长度 */
    XRAY_DET_VENDOR xray_det_vendor;        /* X-Ray探测板供应商 */
    XRAY_ENERGY_NUM xray_energy_num;        /* X光机能量类型数 */
    XRAY_SPEED_PARAM_ST xray_speed[XRAY_FORM_NUM][XRAY_SPEED_NUM]; /* X光机速度参数，0-低速，1-中速，2-高速，若不支持某速度，该速度对应的参数均填0 */
    unsigned int padding_top;               /* 显示YUV图像顶部padding高，与padding_bottom、实际图像高之和缩放到显示窗口纵向高 */
    unsigned int padding_bottom;            /* 显示YUV图像底部padding高，与padding_top、实际图像高之和缩放到显示窗口纵向高 */
    unsigned int blanking_top;              /* 显示YUV图像顶部空白高，当把过包画面往下拖动时，能拖动的最大距离 */
    unsigned int blanking_bottom;           /* 显示YUV图像底部空白高，当把过包画面往上拖动时，能拖动的最大距离 */
    unsigned int package_len_max;           /* 包裹强制分割长度，单位：像素（基于采传的XRaw，未超分），范围：[640, 1600] */
    unsigned int audio_dev_cnt;             /* 音频输出通道个数 */
    ISM_DISP_MODE_E ism_disp_mode;          /* 安检机XRay图像显示模式 */
    XSP_RESIZE_FACTOR xsp_resize_factor;    /* 超分伸缩系数 */
} DSP_CAPB_PARAM_ST;

/* 智能分析支持的模块索引 */
typedef enum tagIaModIdx
{
    IA_MOD_SVA = 0,                         /* 违禁品检测智能 */
	IA_MOD_BA,                              /* 行为分析智能 */
	IA_MOD_FACE,                            /* 人脸智能 */
	IA_MOD_PPM,                             /* 人包关联智能 */

	IA_MOD_MAX_NUM,
} IA_MOD_IDX_E;

/* 智能模块支持跨芯片传输初始化参数 */
typedef struct tagIaCapTransPrm
{
	unsigned int uiDualChipTransFlag;       /* 是否需要跨芯片传输数据，0不需要，1需要 */
	unsigned int uiRunChipId;               /* 运行的芯片id，用于片间通信 */
} IA_CAPB_TRANS_PRM_S;

/* 智能模块初始化私有参数 */
typedef union tagIaCapPrvtPrm
{
    unsigned int uiReserved[8];             /* 暂未启用 */
} IA_CAPB_PRVT_PRM_U;

/* 智能模块初始化参数 */
typedef struct tagIaCapPrm
{
    unsigned int uiEnableFlag;              /* 智能模块使能标记 */
	
	IA_CAPB_TRANS_PRM_S stIaModTransPrm;    /* 模块是否需要跨芯片传输 */
	IA_CAPB_PRVT_PRM_U stIaCapbPrvtPrm;     /* 智能模块私有能力 */
} IA_CAPB_PRM_S;

/* 所以支持的智能模块初始化参数 */
typedef struct tagDspIaInitMap
{
	IA_CAPB_PRM_S stIaModCapbPrm[IA_MOD_MAX_NUM];             /* 违禁品检测智能能力级参数 */
} DSP_IA_INIT_MAP_S;

/* 应用与DSP模块共享信息结构体 */
typedef struct tagDspInitParaSt
{
    /***********************************************************************/
    /*                    设备类型区分信息                                 */
    /***********************************************************************/
    unsigned int viType;                     /* 视频输入类型 */
    unsigned int aiType;                     /* 音频输入模式 */
    unsigned int machineType;                /* 机器型号(对讲有用来区分有音频和无音频)
                                                            暂时保留，尽量不要用该参数，有无音频用
                                                            aiType来区分
                                              */
    unsigned int boardType;                  /* 设备类型   */
    unsigned int sn;                         /* 设备序列号 */
    DEVICE_CHIP_TYPE_E deviceType;           /* 设备种类，用于区分单双芯片 */
    MODEL_TYPE_E modelType;                  /* 模型种类，用于区分算法资源 */
    LANGUAGE_TYPE_E languageType;            /* 语言类型 */

    unsigned int dspIdx;                     /* DSP芯片索引，用于多芯片情况 */

    unsigned int xspLibSrc;                  /* DSP成像算法库，枚举XSP_LIB_SRC*/

	DSP_CAPB_PARAM_ST dspCapbPar;            /* APP 下发能力级参数 */
    /***********************************************************************/
    /*                    黑白名单数量                                     */
    /***********************************************************************/
    unsigned int blackListNums;
    unsigned int whiteListNums;

    /***********************************************************************/
    /*                    DSP模块通道数定义信息                            */
    /***********************************************************************/
    unsigned int encChanCnt;                 /* 同时支持编码通道个数     */
    unsigned int decChanCnt;                 /* 同时支持解码通道个数     */
    unsigned int ipcChanCnt;                 /* 同时支持转包通道个数     */
    unsigned int xrayDispChanCnt;            /* X光显示通道个数，RK主板总共4个输出口，前面两个为X光输出通道                   */
    unsigned int xrayChanCnt;                /* X ray通道个数(预览+回拉) */

    unsigned int rtpPackLen;                 /* 最大RTP打包包长      */

    /* 视频采集参数配置 MAX_CAPT_CHAN */
    VI_INIT_INFO_ST stViInitInfoSt;          /* 采集模块初始化属性 */

    /* 视频输出参数配置 MAX_DISP_CHAN */
    VO_INIT_INFO_ST stVoInitInfoSt;          /* 显示输出模块初始化属性 */

    /* 算法危险品参数配置 ,最大两路*/
    SVA_DSP_DET_STRU stSvaInitInfoSt[2];
	DSP_IA_INIT_MAP_S stIaInitMapPrm;        /* 智能模块能力级初始化参数 */

    /* 界面信息 */
    SCREEN_CTRL stScreenCtrl[MAX_DISP_CHAN]; /* GUI 图形层地址与信息 */

    /***********************************************************************/
    /*                  应用下发信息，用于实现对应功能                     */
    /***********************************************************************/
    /* 采集无视频信号时，所使用的无视频信号图像信息 */
    CAPT_NOSIGNAL_INFO_ST stCaptNoSignalInfo;
    FONT_LIB_INFO_ST stFontLibInfo;          /* OSD功能所使用字库的信息 */
    STREAM_INFO_ST stStreamInfo;             /* 码流编码与封包信息      */
    FACE_THRESHOLD face_threshold;           /* 人脸相关参数阈值 */
    WATER_MASK_INFO_ST stWaterMaskInfo;      /* 水印功能信息 */
    AUDIO_TB_BUF_INFO_ST stAudioTBBufInfo;   /* 语音对讲功能缓冲区信息 */
    AUDIO_PLAY_BACK_PRM stAudioPlayBcakPrm;  /* 语音文件播放信息*/
    /* Logo功能缓冲区信息，由应用为该字段分配内存，且为logo 缓冲区分配内存 */
    LOGO_BUF_INFO_ST stLogBufInfo;           /* todo */
    PROC_SHARE_DATA stProcData;

    /***********************************************************************/
    /*                      码流共享缓冲区信息                             */
    /***********************************************************************/
    REC_POOL_INFO RecPoolMain[MAX_VENC_CHAN];    /* 编码录像主码流buf描述 */
    REC_POOL_INFO RecPoolSub[MAX_VENC_CHAN];     /* 编码录像子码流buf描述 */
    REC_POOL_INFO RecPoolThird[MAX_VENC_CHAN];   /* 编码录像third码流buf描述 */
    NET_POOL_INFO NetPoolMain[MAX_VENC_CHAN];    /* 编码网络主码流buf描述 */
    NET_POOL_INFO NetPoolSub[MAX_VENC_CHAN];     /* 编码网络子码流buf描述 */

    DEC_SHARE_BUF decShareBuf[MAX_VDEC_CHAN];    /* 解码码流共享缓冲区(PS/RTP) */
    DEC_SHARE_BUF ipcShareBuf[MAX_VIPC_CHAN];    /* 转包码流共享缓冲区 */

    REC_POOL_INFO RecPoolRecode[MAX_VIPC_CHAN];  /* 转包后ps  码流buf描述 */
    NET_POOL_INFO NetPoolRecode[MAX_VIPC_CHAN];  /* 转包后rtp 码流buf描述 */

    /***********************************************************************/
    /*                      X RAY共享缓冲区信息                            */
    /***********************************************************************/
    XRAY_SHARE_BUF xrayShareBuf[MAX_XRAY_CHAN]; /* 实时过包XRAW共享Buffer，DSP收到命令HOST_CMD_XRAY_INPUT_START后从该Buffer读数据 */
    XRAY_SHARE_BUF xrayPbBuf[MAX_XRAY_CHAN];    /* 回拉归一化XRAW共享Buffer，DSP收到命令HOST_CMD_XRAY_PLAKBACK_START后从该Buffer读数据 */
    DISP_GLOBALENLARGE_PRM GlobalEnlarge[MAX_XRAY_CHAN][MAX_DISP_CHAN];   /*返回给应用全局放大的中心点坐标以及缩放比例*/

    /***********************************************************************/
    /*                      成像共享信息                            */
    /***********************************************************************/

    XSP_RT_PARAMS stXspPicPrm;  /* 成像参数 */
    XSP_LIB_TIME_PARM stTimePram;               /* 算法库耗时统计 */

    /***********************************************************************/
    /*                      DSP模块状态信息                                */
    /***********************************************************************/
    /* 模块状态信息，应用按需获取，应用分配内存 */
    SYS_STATUS stSysStatus;                  /* 系统状态 */
    CAPT_STATUS CaptStatus[MAX_CAPT_CHAN];   /* 采集 */
    ENC_STATUS VencStatus[MAX_VENC_CHAN];    /* 编码 */
    DEC_STATUS VdecStatus[MAX_VDEC_CHAN];    /* 解码 */
    DISP_STATUS DispStatus[MAX_DISP_CHAN];   /* 显示 */
    IPC_STATUS VipcStatus[MAX_VIPC_CHAN];    /* 远程解码 */

    /***********************************************************************/
    /*                      DSP功能扩展区定义                              */
    /***********************************************************************/

    /*
        此处给出地址数组，用于保存对应功能的结构体的地址，另需要根据功能定义
        相关的结构体，并由应用为其分配内存，传给DSP，以实现共享的作用
     */
    unsigned int reserved[MAX_RESERVED];

} DSPINITPARA;


/****************************************************************************/
/*                                 命令字                                   */
/****************************************************************************/

/*
    应用与DSP交互命令定义
 */
typedef enum
{
    HOST_CMD_NON                      = 0x7FFFFFFF,         /* 起始 无效值 */
    /* 初始化DSP模块 */
    HOST_CMD_MODULE_SYS               = 0x80000000,         /* 系统模块的能力级 */
    HOST_CMD_DSP_INIT                 = 0x80000001,         /* 初始化DSP库 */
    HOST_CMD_DSP_DEINIT               = 0x80000002,         /* 去初始化DSP库 */
    HOST_CMD_DSP_GET_STATUS           = 0x80000003,
    HOST_CMD_DSP_GET_SYS_VERISION     = 0x80000004,         /* 获取DSP程序的版本信息 */

     /* 采集 */
    HOST_CMD_MODULE_VI                = 0x80010000,         /* 采集模块的能力级 */
    HOST_CMD_START_VIDEO_INPUT        = 0x80010001,         /* 启动视频采集 */
    HOST_CMD_STOP_VIDEO_INPUT         = 0x80010002,         /* 停止视频采集 */
    HOST_CMD_SET_VIDEO_PARM           = 0x80010003,         /* 配置视频采集属性 */
    HOST_CMD_SET_VIDEO_ROTATE         = 0x80010004,         /* 设置视频旋转属性 */
    HOST_CMD_SET_VIDEO_MIRROR         = 0x80010005,         /* 设置视频镜像属性 */
    HOST_CMD_SET_VIDEO_WDR            = 0x80010006,         /* 设置视频宽动态   */
    HOST_CMD_SET_VIDEO_POS            = 0x80010007,         /* 设置视频偏移位置属性 */
    HOST_CMD_SET_EDID                 = 0x80010008,         /* 设置输入的EDID */
    HOST_CMD_SET_CSC                  = 0x80010009,         /* 设置CSC参数 */
    HOST_CMD_SET_VIDEO_DISP_EDID      = 0x8001000A,         /* 将显示器的EDID写到输入 */
    HOST_CMD_GET_VIDEO_EDID_INFO      = 0x8001000B,         /* 获取当前EDID信息 */
    HOST_CMD_MSTAR_UPDATE             = 0x8001000C,         /* mstar固件升级 */
    HOST_CMD_MCU_UPDATE               = 0x8001000D,         /* mcu固件升级 */
    HOST_CMD_MCU_BEAT_HEART_START     = 0x8001000E,         /* 开始发送mcu心跳 */
    HOST_CMD_MCU_BEAT_HEART_STOP      = 0x8001000F,         /* 停止发送mcu心跳 */
    HOST_CMD_GET_CHIP_BUILD_TIME      = 0x80010010,         /* 获取视频相关芯片的固件编译时间 */
    HOST_CMD_CHIP_RESET               = 0x80010011,         /* 视频相关芯片复位 */

    /* 编码 */
    HOST_CMD_MODULE_VENC              = 0x80020000,         /* 编码模块的能力级 */
    HOST_CMD_START_ENCODE             = 0x80020001,         /* 开启编码功能 */
    HOST_CMD_STOP_ENCODE              = 0x80020002,         /* 停止编码功能 */
    HOST_CMD_INSERT_I_FRAME           = 0x80020003,         /* 强制编码I帧 */
    HOST_CMD_SET_ENCODER_PARAM        = 0x80020004,         /* 设置编码器属性 */

    HOST_CMD_START_SUB_ENCODE         = 0x80020005,         /* 开始子码流编码功能 */
    HOST_CMD_STOP_SUB_ENCODE          = 0x80020006,         /* 停止子码流编码功能 */
    HOST_CMD_INSERT_SUB_I_FRAME       = 0x80020007,         /* 强制子码流编码I帧 */
    HOST_CMD_SET_SUB_ENCODER_PARAM    = 0x80020008,         /* 设置子码流编码器属性 */

    HOST_CMD_START_THIRD_ENCODE       = 0x80020009,         /* 开始第三通道码流编码功能 */
    HOST_CMD_STOP_THIRD_ENCODE        = 0x8002000a,         /* 停止第三通道码流编码功能 */
    HOST_CMD_INSERT_THIRD_I_FRAME     = 0x8002000b,         /* 强制第三通道码流编码I帧 */
    HOST_CMD_SET_THIRD_ENCODE_PARAM   = 0x8002000c,         /* 设置第三通道码流编码器属性 */
    HOST_CMD_SET_ENC_REGION           = 0x8002000d,         /* 设置编码ROI */
    HOST_CMD_ENC_DRAW_SWITCH          = 0x8002000e,         /* 编码叠框开关 */

    /* 抓图 */
    HOST_CMD_START_ENC_JPEG           = 0x80020100,         /* 开启编码抓图功能 */
    HOST_CMD_STOP_ENC_JPEG            = 0x80020101,         /* 停止编码抓图功能 */
    HOST_CMD_SET_ENC_JPEG_PARAM       = 0x80020102,         /* 设置编码抓图属性 */

    /* 解码 */
    HOST_CMD_MODULE_DEC               = 0x80030000,         /* 解码模块的能力级 */
    HOST_CMD_DEC_START                = 0x80030001,         /* 开启解码功能 */
    HOST_CMD_DEC_STOP                 = 0x80030002,         /* 停止解码功能 */
    HOST_CMD_DEC_RESET                = 0x80030003,         /* 重启解码器 */
    HOST_CMD_DEC_NEXT_FRM             = 0x80030004,         /* 解码单帧显示 */
    HOST_CMD_DEC_GET_IMG              = 0x80030005,         /* 设置解码抓图属性 */
    HOST_CMD_DEC_PAUSE                = 0x80030006,         /* 暂停解码功能 */
    HOST_CMD_DEC_ATTR                 = 0x80030008,         /* 设置解码属性*/
    HOST_CMD_DEC_SYNC                 = 0x80030009,         /* 设置解码同步参数*/
    HOST_CMD_DEC_AUD_ENABLE           = 0x8003000b,         /* 设置音频解码开关*/
    HOST_CMD_DEC_JPG_TO_BMP           = 0x8003000c,         /* jpg转bmp接口*/

    /* 显示 */
    HOST_CMD_MODULE_DISP              = 0x80040000,         /* 显示模块的能力级 */
    HOST_CMD_ALLOC_DISP_REGION        = 0x80040001,         /* 配置显示窗口属性 */
    HOST_CMD_DISP_CLEAR               = 0x80040002,         /* 清除显示窗口属性 */
    HOST_CMD_SET_OUTPUT_REGION        = 0x80040003,         /* 配置显示通道的输出区域 */
    HOST_CMD_SET_DISP_ROTATE          = 0x80040004,         /* 设置视频显示旋转属性 */
    HOST_CMD_SET_DISP_MIRROR          = 0x80040005,         /* 设置视频镜像属性 */

    HOST_CMD_SET_VI_DISP              = 0x80040006,         /* 配置采集预览功能 */
    HOST_CMD_SET_DEC_DISP             = 0x80040007,         /* 配置解码预览功能 */

    HOST_CMD_SET_VO_STYLE             = 0x80040008,         /* 设置显示输出风格 */
    HOST_CMD_SET_VO_CSC               = 0x80040009,         /* 设置显示输出效果 */

    HOST_CMD_DISP_FB_MMAP             = 0x8004000a,         /* UI层内存映射 */
    HOST_CMD_DISP_FB_UMAP             = 0x8004000b,         /* UI层内存去映射 */
    HOST_CMD_DISP_FB_SHOW             = 0x8004000c,         /* 刷新UI显示 */
    HOST_CMD_DISP_FB_SNAP             = 0x8004000d,
    HOST_CMD_SET_MENU                 = 0x8004000e,         /* 设置菜单属性 */
    HOST_CMD_REFRESH_MENU_DISP        = 0x8004000f,         /* 刷新菜单显示 */
    HOST_CMD_SET_VO_STANDARD          = 0x80040010,         /* 修改CVBS输出制式 */
    HOST_CMD_STOP_DISP                = 0x80040011,         /* 解除VO CHN输入源关系 */
    HOST_CMD_SET_VO_SWITCH            = 0x80040012,         /* Vo显示智能叠框开关 */
    HOST_CMD_CLEAR_VO_BUFFER          = 0x80040013,         /* 清空Vo显示缓存 */
    HOST_CMD_PIP_DISP_REGION          = 0x80040014,         /* 设置放大镜参数 */
    HOST_CMD_DPTZ_DISP_PRM            = 0x80040015,         /* 设置通道缩放倍数 */
    HOST_CMD_SET_THUMBNAIL_PRM        = 0x80040016,         /* 设置缩略图参数 */
    HOST_CMD_SET_DISPFLICKER_PRM      = 0x80040017,         /* 设置智能框闪烁 */
    HOST_CMD_SET_DISPLINETYPE_PRM     = 0x80040018,         /* 智能信息画线类型 */
    HOST_CMD_SET_MOUSE_BIND_CHN       = 0x80040019,         /* 配置鼠标绑定窗口 */
    HOST_CMD_DISP_FB_INIT             = 0x8004001a,         /* UI初始化 */
    HOST_CMD_SET_AI_DISP_SWITCH       = 0x8004001b,         /* 智能AI 显示开关 */
    HOST_CMD_DISP_GET_ARTICLE_RESULT  = 0x8004001c,         /* 获取显示智能信息结果 */
    HOST_CMD_DISP_SET_ORG_SVAPRM      = 0x8004001d,         /* 设置TIP或有机物参数配置 */
    HOST_CMD_DISP_GET_CUREFFECTREG    = 0x8004001e,         /* 获取当前屏幕有效区域 */
    HOST_CMD_SET_DISPLINEWIDTH_PRM    = 0x8004001f,         /* 智能信息画线宽 */
    HOST_CMD_DISP_ADD_VO              = 0x80040020,         /* add vo窗口 */
	HOST_CMD_PIP_VDECDISP_REGION      = 0x80040021,         /* 移动对应的vo窗口 */
    HOST_CMD_SET_MOUSE_POS            = 0x80040022,         /* 设置鼠标坐标 */
    HOST_CMD_DISP_WBC_ENABLE          = 0x80040023,         /* 开启关闭回写使能 */
    HOST_CMD_DISP_GET_DISP_WBC        = 0x80040024,         /* 获取回写视频帧 */


    /* 音频 */
    HOST_CMD_MODULE_AUDIO             = 0x80050000,         /* 音频模块的能力级 */
    HOST_CMD_SET_AUDIO_LOOPBACK       = 0x80050001,         /* 设置音频采集回环 */
    HOST_CMD_SET_DEC_PLAY             = 0x80050002,         /* 设置音频解码播放 */
    HOST_CMD_SET_TALKBACK             = 0x80050003,         /* 设置语音对讲功能属性 */
    HOST_CMD_START_AUDIO_PLAY         = 0x80050004,         /* 开启语音播报 */
    HOST_CMD_STOP_AUDIO_PLAY          = 0x80050005,         /* 停止语音播报 */
    HOST_CMD_START_AUTO_ANSWER        = 0x80050006,         /* 开启自动应答功能 */
    HOST_CMD_STOP_AUTO_ANSWER         = 0x80050007,         /* 停止自动应答功能 */
    HOST_CMD_START_AUDIO_RECORD       = 0x80050008,         /* 启动语音录制功能 */
    HOST_CMD_STOP_AUDIO_RECORD        = 0x80050009,         /* 停止语音录制功能 */
    HOST_CMD_START_AUDIO_OUTPUT       = 0x8005000a,         /* 启动语音输出 */
    HOST_CMD_STOP_AUDIO_OUTPUT        = 0x8005000b,         /* 停止语音输出 */
    HOST_CMD_START_AUDIO_INPUT        = 0x8005000c,         /* 启动语音输入 */
    HOST_CMD_STOP_AUDIO_INPUT         = 0x8005000d,         /* 停止语音输入 */
    HOST_CMD_SET_AUDIO_ALARM_LEVEL    = 0x8005000e,         /* 设置声音超限报警阈值 */
    HOST_CMD_SET_AI_VOLUME            = 0x8005000f,         /* 设置音频输入音量 */
    HOST_CMD_SET_AO_VOLUME            = 0x80050010,         /* 设置音频输出音量 */

    HOST_CMD_SET_EAR_PIECE            = 0x80050011,         /* 设置听筒免提切换 */
    HOST_CMD_START_SOUND_RECORD       = 0x80050012,         /* 开启音频输入录音 */
    HOST_CMD_STOP_SOUND_RECORD        = 0x80050013,         /* 停止音频输入录音 */
    HOST_CMD_SET_MANAGE_TALKTO_RESIDENT = 0x80050014,       /* 启动网络设备与模拟室内机对讲功能 */
    HOST_CMD_SET_DOOR_TALKTO_RESIDENT = 0x80050015,         /* 门口机与模拟室内机对讲功能 */
    HOST_CMD_SET_AUDIO_ENC_TYPE       = 0x80050016,         /* 设置音频编码类型 */
    HOST_CMD_SET_AUDIO_TTS_START      = 0x80050017,         /* 合成语音启动 */
    HOST_CMD_SET_AUDIO_TTS_HANDLE     = 0x80050018,         /* 语音合成 */
    HOST_CMD_SET_AUDIO_TTS_STOP       = 0x80050019,         /* 合成语音结束 */
    

    /* OSD */
    HOST_CMD_MODULE_OSD               = 0x80060000,         /* OSD模块的能力级 */
    HOST_CMD_SET_ENC_OSD              = 0x80060001,         /* 设置编码通道OSD属性 */
    HOST_CMD_SET_ENC_DEFAULT_OSD      = 0x80060002,         /* 设置编码通道默认OSD属性 */
    HOST_CMD_START_ENC_OSD            = 0x80060003,         /* 开启编码通道OSD功能 */
    HOST_CMD_STOP_ENC_OSD             = 0x80060004,         /* 停止编码通道OSD功能 */
    HOST_CMD_SET_DST_OSD              = 0x80060005,         /* OSD模块设置夏令时 */

    HOST_CMD_SET_DISP_OSD             = 0x80060100,         /* 设置显示通道OSD功能 */
    HOST_CMD_START_DISP_OSD           = 0x80060101,         /* 开启显示通道OSD功能 */
    HOST_CMD_STOP_DISP_OSD            = 0x80060102,         /* 停止显示通道OSD功能 */

    HOST_CMD_SET_ENC_LOGO             = 0x80060200,         /* 设置编码通道LOGO属性 */
    HOST_CMD_START_ENC_LOGO           = 0x80060201,         /* 开启编码通道LOGO功能 */
    HOST_CMD_STOP_ENC_LOGO            = 0x80060202,         /* 停止编码通道LOGO功能 */

    HOST_CMD_SET_DISP_LOGO            = 0x80060300,         /* 设置显示通道LOGO属性 */
    HOST_CMD_START_DISP_LOGO          = 0x80060301,         /* 开启显示通道LOGO功能 */
    HOST_CMD_STOP_DISP_LOGO           = 0x80060302,         /* 停止显示通道LOGO功能 */

    HOST_CMD_UPDATE_FONT_LIB          = 0x80060400,         /* 更新OSD 字库 */

    /* ISP */
    HOST_CMD_MODULE_ISP               = 0x80070000,         /* ISP模块的能力级 */
    HOST_CMD_LIGHTCTR                 = 0x80070001,
    HOST_CMD_SET_ISP_PARAM            = 0x80070002,         /* 设置ISP参数 */
    HOST_CMD_GET_ISP_PARAM            = 0x80070003,         /* 获取ISP参数 */
    HOST_CMD_SET_EXPOSURE             = 0x80070004,

    HOST_CMD_SENSOR_START             = 0x80070100,         /* 启动 sensor 输出 */
    HOST_CMD_SENSOR_STOP              = 0x80070101,         /* 停止 sensor 输出 */

    /* 智能模块 门禁人脸识别 */
    HOST_CMD_MODULE_FR                = 0x80080000,         /* 智能模块的能力级 */
    HOST_CMD_INIT_FR                  = 0x80080001,         /* 初始化智能模块 */
    HOST_CMD_FFD_CTRL                 = 0x80080002,         /* 人脸检测功能控制属性 */
    HOST_CMD_FR_BM                    = 0x80080003,         /* 人脸建模功能 */
    HOST_CMD_FR_MODEL_ADD             = 0x80080004,         /* 模型数据导入 */
    HOST_CMD_FR_MODEL_DEL             = 0x80080005,         /* 模型数据导出 */
    HOST_CMD_FR_CP                    = 0x80080006,
    HOST_CMD_FR_DET                   = 0x80080007,         /* 活体检测命令字 */
    HOST_CMD_GET_FR_VER               = 0x80080008,
    HOST_CMD_FR_COMPARE               = 0x80080009,
    HOST_CMD_FR_PRESNAP               = 0x8008000a,

    HOST_CMD_FR_JPGSCALE              = 0x8008000d,
    HOST_CMD_FR_BMP2JPG               = 0x8008000e,
    HOST_CMD_FR_ROI                   = 0x8008000f,
    HOST_CMD_FR_GET_BLKLIST_NUMS      = 0x80080010,         /* 获取黑白名单个数 */

    /* 微智能模块 */
    HOST_CMD_MODULE_VDA               = 0x80090000,         /* 视频侦测模块的能力级 */
    HOST_CMD_START_MD                 = 0x80090001,         /* 启动移动侦测 */
    HOST_CMD_STOP_MD                  = 0x80090002,         /* 停止移动侦测 */
    HOST_CMD_SET_MD                   = 0x80090003,         /* 设置移动侦测属性 */
    HOST_CMD_SET_CD                   = 0x80090004,         /* 设置遮挡检测 */
    HOST_CMD_SET_MASK                 = 0x80090005,         /* 设置视频遮挡 */
    HOST_CMD_QR_START                 = 0x80090006,         /* 开启二维码扫码 */
    HOST_CMD_QR_STOP                  = 0x80090007,         /* 关闭二维码扫码 */

    HOST_CMD_MODULE_PRIVATE           = 0x80090200,         /* 设置私有信息 */
    HOST_CMD_SET_WATERMARK            = 0x80090201,         /* 设置水印 */
    HOST_CMD_SET_SUB_WATERMARK        = 0x80090202,         /* 设置子通道水印 */

    HOST_CMD_MODULE_FLD               = 0x800a0000,         /* 活体检测能力级 */
    HOST_CMD_SET_SOLID_PARAM          = 0x800a0001,         /* 设置活体参数 */
    HOST_CMD_GET_SOLID_PARAM          = 0x800a0002,         /* 获取活体参数 */

    /* 智能模块 安检分析仪 */
    HOST_CMD_MODULE_SVA               = 0x800b0000,         /* 备用 */
    HOST_CMD_SVA_GET_VERSION          = 0x800b0001,         /* 获取算法和引擎版本号 */
    HOST_CMD_SVA_INIT                 = 0x800b0002,         /* 初始化算法库 */
    HOST_CMD_SVA_DEINIT               = 0x800b0003,         /* 去初始化算法库 */
    HOST_CMD_SVA_SET_DETECT_REGION    = 0x800b0004,         /* 设置检测区域 */
    HOST_CMD_SVA_SET_DIRETION         = 0x800b0005,         /* 设置传送带方向 */
    HOST_CMD_SVA_UPDATE_MODEL         = 0x800b0006,         /* 升级模型 */
    HOST_CMD_SVA_DECT_SWITCH          = 0x800b0007,         /* 检测总开关 */
    HOST_CMD_SVA_ALERT_SWITCH         = 0x800b0008,         /* 危险物检测开关 */
    HOST_CMD_SVA_ALERT_COLOR          = 0x800b0009,         /* 危险物检测颜色 */
    HOST_CMD_SVA_SET_CONF             = 0x800b000a,         /* 危险物检测置信度 */
    HOST_CMD_SVA_SET_CHN_SIZE         = 0x800b000b,         /* 设置安检机通道大小 */
    HOST_CMD_SVA_SET_EXT_TYPE         = 0x800b000c,         /* 设置OSD拓展信息展示方式 */
    HOST_CMD_SVA_ALERT_NAME           = 0x800b000d,         /* 设置报警物名称 */
    HOST_CMD_SVA_OSD_SCALE_LEAVE      = 0x800b000e,         /* 设置报警物osd缩放等级 */
    HOST_CMD_SVA_SYNC_MAIN_CHAN	      = 0x800b0010,         /* 设置双通道采集主视角通道号 */
    HOST_CMD_SVA_PROC_MODE            = 0x800b0011,         /* 设置双通道是否同步 */
    HOST_CMD_SVA_OSD_BORDER_TYPE      = 0x800b0012,         /* 设置OSD叠框类型 */
    HOST_CMD_SVA_SET_AI_MODEL_STATUS  = 0x800b0013,         /* 设置AI模型开关 */
    HOST_CMD_SVA_GET_AI_MODEL_STATUS  = 0x800b0014,         /* 获取AI模型开关 */
    HOST_CMD_SVA_SET_AI_MODEL_ID      = 0x800b0015,         /* 设置AI模型ID */
    HOST_CMD_SVA_SET_AI_MODEL_NAME    = 0x800b0016,         /* 设置AI模型名 */
    HOST_CMD_SVA_SET_AI_MODEL_ALERT_MAP = 0x800b0017,       /* 设置AI目标物映射 */
    HOST_CMD_SVA_SET_SAMPLE_COLLECT_SWITCH = 0x800b0018,    /* 设置素材采集开关 */
    HOST_CMD_SVA_SET_INPUT_GAP_NUM    = 0x800b0019,         /* 设置智能送帧为全帧率还是隔帧送 */
    HOST_CMD_SVA_INPUT_AUTO_TEST_PIC  = 0x800b001a,         /* 送入BMP图片用于算法智能测试使用 */
    HOST_CMD_SVA_SET_PKG_SPLIT_SENSITY = 0x800b001b,        /* 设置包裹分割灵敏度 */
    HOST_CMD_SVA_SET_PKG_SPLIT_FILTER = 0x800b001c,         /* 设置包裹分割过滤阈值 */
    HOST_CMD_SVA_SET_ZOOM_INOUT_PRM   = 0x800b001d,         /* 设置放大缩小配置参数 */
    HOST_CMD_SVA_SET_JPEGPOS_SWITCH   = 0x800b001e,         /* 设置增加Jpeg图片pos信息开关 */
    HOST_CMD_SVA_SET_ALERT_TAR_CNT    = 0x800b001f,         /* 设置危险品告警个数阈值 */
    HOST_CMD_SVA_SET_ENG_MODEL        = 0x800b0020,         /* 设置引擎模式 */
    HOST_CMD_SVA_SET_SIZE_FILTER      = 0x800b0021,         /* 设置目标尺寸过滤 */
    HOST_CMD_SVA_SET_SPRAY_FILTER_SWITCH = 0x800b0022,      /* 设置喷灌过滤开关 */
    HOST_CMD_SVA_SET_INS_PROC_CNT     = 0x800b0023,         /* 设置每秒裁剪张数(成都定制, 保留) */
    HOST_CMD_SVA_SET_PKG_BORDER_PIXEL = 0x800b0024,         /* 设置包裹边沿预留像素(成都定制, 保留) */
    HOST_CMD_SVA_SET_SPLIT_MODE       = 0x800b0025,         /* 设置集中判图模式开关 */
    HOST_CMD_SVA_SET_PKG_COMB_THRESH  = 0x800b0026,		    /* 设置包裹是否需要融合的像素阈值，[-50,50] */
    HOST_CMD_SVA_SET_FORCE_SPLIT_PRM  = 0x800b0027,			/* 设置包裹强制分割参数，暂未开放 */
    HOST_CMD_SVA_SET_PKG_COMBINE_FLAG = 0x800b0028,			/* 设置包裹融合开关 */
    HOST_CMD_SVA_SET_AIMODEL_SWITCH   = 0x800b0029,			/* ai模型绑定解绑开关 */
    HOST_CMD_SVA_DEINIT_USB_CMD       = 0x800b002a,         /* 去初始化USB通信*/
    HOST_CMD_SVA_INIT_USB_CMD         = 0x800b002b,         /* 初始化USB通信*/
    HOST_CMD_SVA_GET_CONF             = 0x800b002c,         /* PK模式默认置信度获取命令*/

    /* IPC转包模块 */
    HOST_CMD_MODULE_RECODE           = 0x800c0000,          /* IPC转包模块*/
    HOST_CMD_RECODE_START            = 0x800c0001,          /* 启动转包模块 */
    HOST_CMD_RECODE_STOP             = 0x800c0002,          /* 停止转包模块 */
    HOST_CMD_RECODE_SET              = 0x800c0003,          /* 设置转包模块参数 */
    HOST_CMD_RECODE_AUD_PACK         = 0x800c0004,          /* 设置音频转包开关 */

    /* 智能模块 行为分析 */
    HOST_CMD_MODULE_BA               = 0x800d0000,          /* 行为分析模块 */
    HOST_CMD_BA_INIT                 = 0x800d0001,          /* 设置行为检测打开 */
    HOST_CMD_BA_DEINIT               = 0x800d0002,          /* 设置行为检测关闭 */
    HOST_CMD_SET_BA_REGION           = 0x800d0003,          /* 设置行为分析检测区域(当前仅对在离岗和遮挡有效) */
    HOST_CMD_SET_BA_CAPTURE          = 0x800d0004,          /* 设置行为检测抓拍开关(当前未使用，默认抓拍) */
	HOST_CMD_GET_BA_VERSION          = 0x800d0005,          /* 获取行为分析版本信息 */
	HOST_CMD_SET_BA_DET_LEVEL        = 0x800d0006,          /* 设置行为分析检测级别，该配置对在离岗和遮挡不生效，仅对另外四种生效 */

    /* X ray处理*/
    HOST_CMD_MODULE_XRAY             = 0x800e0000,          /* X-Ray功能处理模块 */
    HOST_CMD_XRAY_INPUT_START        = 0x800e0001,          /* 启动采集，区分通道号，无参数 */
    HOST_CMD_XRAY_INPUT_STOP         = 0x800e0002,          /* 停止采集，区分通道号，无参数 */
    HOST_CMD_XRAY_PLAYBACK_START     = 0x800e0003,          /* 启动回拉，区分通道号，结构体XRAY_PB_PARAM */
    HOST_CMD_XRAY_PLAYBACK_STOP      = 0x800e0004,          /* 停止回拉，不区分通道号，无参数 */
    HOST_CMD_XRAY_TRANS              = 0x800e0005,          /* 转存，将RAW数据转为常用图片格式，区分通道号，结构体XRAY_TRANS_PROC_PARAM */
    HOST_CMD_XRAY_SET_PARAM          = 0x800e0006,          /* 功能初始化参数，包括置白等，区分通道号，结构体XRAY_PARAM */
    HOST_CMD_XRAW_STORE              = 0x800e0007,          /* 保存源RAW数据开关，区分通道号，单个参数，1-开启，0-关闭 */
    HOST_CMD_XRAY_CHANGE_SPEED       = 0x800e0008,          /* 切换传送带速度，不区分通道号（通道号写0），结构体XRAY_SPEED_PARAM */
    HOST_CMD_XRAY_GET_SLICE_NUM_AFTER_CLS = 0x800e0009,     /* 获取切换速度或方向清屏后屏幕上显示了多少个条带，区分通道号，单个参数 */
    HOST_CMD_XRAY_SPLIT_NRAW         = 0x800e000a,          /* 拆分归一化RAW数据，从输入的RAW数据中截取一段，不区分通道号，结构体XRAY_NRAW_SPLIT_PARAM */
    HOST_CMD_XRAY_SPLICE_NRAW        = 0x800e000b,          /* 拼接归一化RAW数据，将输入的多个RAW数据段拼成一整块平面，不区分通道号，结构体XRAY_NRAW_SPLICE_PARAM */
    HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE = 0x800e000c,       /* 获取转存时图片存放内存大小，不区分通道号，结构体XRAY_TRANS_GET_BUFFER_SIZR_PARAM */
    HOST_CMD_XRAY_GET_CORR_DATA      = 0x800e000d,          /* 获取本底与满载校正数据，区分通道号，结构体XRAY_RAW_FULL_OR_ZERO_DATA */
    HOST_CMD_XRAY_RESIZE_NRAW        = 0x800e000e,          /* 缩放归一化RAW数据，不区分通道号，结构体XRAY_NRAW_RESIZE_PARAM */

    /* 智能模块 安检人脸 */
    HOST_CMD_MODULE_FACE             = 0x800f0000,          /* 安检人脸模块 */
    HOST_CMD_FACE_INIT               = 0x800f0001,          /* 人脸模块初始化 */
    HOST_CMD_FACE_DEINIT             = 0x800f0002,          /* 人脸模块去初始化 */
    HOST_CMD_FACE_LOGIN              = 0x800f0003,          /* 人脸登录 */
    HOST_CMD_FACE_REGISTER           = 0x800f0004,          /* 人脸注册 */
    HOST_CMD_FACE_UPDATE_COMPARE_LIB = 0x800f0005,          /* 更新人脸比对库 */
    HOST_CMD_FACE_UPDATE_DATABASE    = 0x800f0006,          /* 更新人脸数据库 */
    HOST_CMD_FACE_SET_CONFIG         = 0x800f0007,          /* 设置人脸配置参数 */
    HOST_CMD_FACE_GET_VERSION        = 0x800f0008,          /* 获取版本信息 */
    HOST_CMD_FACE_CMP_FACE           = 0x800f0009,          /* 人脸比对接口命令 */
    HOST_CMD_FACE_CAP_STATR          = 0x800f000a,          /* 人脸抓拍开启 */
    HOST_CMD_FACE_CAP_STOP           = 0x800f000b,          /* 人脸抓拍关闭 */
    HOST_CMD_FACE_FEATURE_CMP        = 0x800f000c,          /* 人脸特征数据版本判断接口 */

    /*XSP成像处理模块*/
    HOST_CMD_MODULE_XSP           = 0x80100000,             /* 处理模块 */
    HOST_CMD_MODULE_XSP_INIT      = 0x80100001,             /* 模块初始化，不支持该功能 */
    HOST_CMD_MODULE_XSP_DEINIT    = 0x80100002,             /* 模块去初始化，不支持该功能 */
    HOST_CMD_XSP_SET_DEFAULT      = 0x80100003,             /* 设置默认图像模式，不区分通道号，结构体XSP_DEFAULT_STYLE */
    HOST_CMD_XSP_SET_ORIGINAL     = 0x80100004,             /* 设置原始图像模式，不区分通道号，结构体XSP_ORIGINAL_MODE_PARAM */
    HOST_CMD_XSP_SET_DISP         = 0x80100005,             /* 设置显示，不区分通道号，结构体XSP_DISP_MODE */
    HOST_CMD_XSP_SET_ANTI         = 0x80100006,             /* 设置反色，不区分通道号，结构体XSP_ANTI_COLOR_PARAM */
    HOST_CMD_XSP_SET_EE           = 0x80100007,             /* 设置边缘增强，不区分通道号，结构体XSP_EE_PARAM */
    HOST_CMD_XSP_SET_UE           = 0x80100008,             /* 设置超级增强，不区分通道号，结构体XSP_UE_PARAM */
    HOST_CMD_XSP_SET_LE           = 0x80100009,             /* 设置局部增强，不区分通道号，结构体XSP_LE_PARAM */
    HOST_CMD_XSP_SET_LP           = 0x8010000a,             /* 设置低穿，不区分通道号，结构体XSP_LP_PARAM */
    HOST_CMD_XSP_SET_HP           = 0x8010000b,             /* 设置高穿，不区分通道号，结构体XSP_HP_PARAM */
    HOST_CMD_XSP_SET_ABSOR        = 0x8010000c,             /* 设置可变吸收率，不区分通道号，结构体XSP_VAR_ABSOR_PARAM */
    HOST_CMD_XSP_SET_LUMINANCE    = 0x8010000d,             /* 设置亮度，不区分通道号，结构体XSP_LUMINANCE_PARAM */
    HOST_CMD_XSP_SET_PSEUDO_COLOR = 0x8010000e,             /* 设置伪彩，不区分通道号，结构体XSP_PSEUDO_COLOR_PARAM */
    HOST_CMD_XSP_SET_SPEEDUP_MODE = 0x8010000f,             /* 设置加速模式，DSP初始化后立即设置，不区分通道号（通道号写0），结构体XSP_SPEEDUP_PARAM */

    HOST_CMD_XSP_GET_VERSION         = 0x80100010,          /* 获取版本号，不区分通道号 */
    HOST_CMD_XSP_SET_MIRROR          = 0x80100011,          /* 设置镜像，区分通道号，结构体XSP_VERTICAL_MIRROR_PARAM */
    HOST_CMD_XSP_SET_UNPEN           = 0x80100012,          /* 设置难穿透识别，不区分通道号，结构体XSP_UNPEN_PARAM */
    HOST_CMD_XSP_SET_SUS_ALERT       = 0x80100013,          /* 设置可疑物识别，不区分通道号，结构体XSP_SUS_ALERT */
    HOST_CMD_XSP_SET_TIP             = 0x80100014,          /* 设置TIP参数，不区分通道号，对应结构体XSP_TIP_DATA_ST */
    HOST_CMD_XSP_GET_TIP_RAW         = 0x80100015,          /* 从包裹RAW数据中获取TIP，不区分通道号，结构体XSP_TIP_RAW_CUTOUT */
    HOST_CMD_XSP_SET_ENHANCED_SCAN   = 0x80100016,          /* 设置强扫，区分通道号，对应结构体XSP_ENHANCED_SCAN_PARAM */
    HOST_CMD_XSP_SET_BKG             = 0x80100017,          /* 设置背景检测参数，不区分通道号，对应结构体XSP_BKG_PARAM */
    HOST_CMD_XSP_SET_DEFORMITY_CORRECTION = 0x80100018,     /* 设置畸形矫正，不区分通道号，对应结构体XSP_DEFORMITY_CORRECTION */
    HOST_CMD_XSP_SET_COLOR_TABLE     = 0x80100019,          /* 设置成像配置表，不区分通道号，对应结构体XSP_COLOR_TABLE_PARAM */
    HOST_CMD_XSP_COLOR_ADJUST        = 0x8010001a,          /* 设置冷暖颜色调节，区分通道号，对应结构体XSP_COLOR_ADJUST_PARAM */
    HOST_CMD_XSP_NOISE_REDUCE        = 0x8010001b,          /* 设置降噪等级，区分通道号，对应结构体XSP_NOISE_REDUCE_PARAM，等级设置为1~20时开启指标模式 */
    HOST_CMD_XSP_SET_CONTRAST        = 0x8010001c,          /* 设置对比度等级，区分通道号，对应结构体XSP_CONTRAST_PARAM */
    HOST_CMD_XSP_SET_LE_TH           = 0x8010001d,          /* 设置局增阈值，区分通道号，对应结构体XSP_LE_TH_RANGE */
    HOST_CMD_XSP_SET_DT_GRAY         = 0x8010001e,          /* 设置双能分辨灰度范围，区分通道号，对应结构体XSP_DT_GRAY_RANGE */
    HOST_CMD_XSP_PACKAGE_DIVIDE      = 0x8010001f,          /* 设置包裹分割参数，不区分通道号（通道号写0），对应结构体XSP_PACKAGE_DIVIDE_PARAM */
    HOST_CMD_XSP_DOSE_CORRECT        = 0x80100020,          /* 剂量波动修正，不区分通道号，对应结构体XSP_DOSE_CORRECT_PARAM */
    HOST_CMD_XSP_SET_STRETCH         = 0x80100021,          /* 设置拉伸模式，依靠算法内部实现，区分通道号，对应结构体XSP_STRETCH_MODE_PARAM*/
    HOST_CMD_XSP_SET_BKGCOLOR        = 0x80100022,          /* 设置显示图像背景色，区分通道号，结构体XSP_BKG_COLOR */
    HOST_CMD_XSP_SET_GAMMA           = 0x80100023,          /* 设置gamma参数，区分通道号，结构体XSP_GAMMA_PARAM */
    HOST_CMD_XSP_SET_AIXSP_RATE      = 0x80100024,          /* 设置AIXSP和传统XSP权重，区分通道号，结构体XSP_AIXSP_RATE_PARAM */
    HOST_CMD_XSP_SET_COLORS_IMAGING  = 0x80100025,          /* 设置三色六色显示，不区分通道号，对应结构体XSP_COLORS_IMAGING_PARAM */
    HOST_CMD_XSP_SET_BLANK_SLICE     = 0x80100026,          /* 设置空白条带是否丢弃，不区分通道号（通道号写0） 对应结构体 XSP_SET_RM_BLANK_SLICE */
    HOST_CMD_XSP_SET_COLDHOT_THRESHOLD = 0x80100027,        /* 设置冷热源阈值, 区分通道号, 结构体 XSP_COLDHOT_PARAM */
    HOST_CMD_XSP_RELOAD_ZTABLE       = 0x80100028,          /* 重新加载Z值表 */

    /* 包裹处理模块 */
    HOST_CMD_MODULE_XPACK             = 0x80110000,         /* 射线包裹处理模块 */
    HOST_CMD_XPACK_MARK               = 0x80110001,         /* 包裹MARK，暂不支持 */
    HOST_CMD_XPACK_SAVE               = 0x80110002,         /* 包裹SAVE，区分通道号，结构体XPACK_SAVE_PRM */
    HOST_CMD_XPACK_SET_YUVSHOW_OFFSET = 0x80110003,         /* 设置显示纵向偏移量，区分通道号，结构体XPACK_YUVSHOW_OFFSET */
    HOST_CMD_XPACK_SET_SYNC           = 0x80110006,         /* 设置输入同步参数需要在Xray数据输入(DSP)前调用 */
    HOST_CMD_XPACK_SET_JPG            = 0x80110007,         /* 设置JPG抓图参数，对应结构体XPACK_JPG_SET_ST */
    HOST_CMD_XPACK_SET_SEGMENT        = 0x80110008,         /* 设置集中判图分片参数 */
    HOST_CMD_SET_DISP_SYNC_TIME       = 0x80110009,         /* 设置双视角显示同步超时时间，不区分通道号，时间设置为0表示关闭双视角同步机制，仅双视角机型有效 */

	/* 数据加密 */
    HOST_CMD_MODULE_CIHPER           = 0x80120000,          /* 加解密模块 */
    HOST_CMD_CIPHER_GETMEM           = 0x80120001,          /* 申请加密所需的内存 */
    HOST_CMD_CIPHER_AES_ENC          = 0x80120002,          /* AES加密 */
    HOST_CMD_CIPHER_AES_DEC          = 0x80120003,          /* AEC解密 */

    /* 人包关联模块 */
    HOST_CMD_MODULE_PPM              = 0x80120000,          /* 人包关联模块 */
    HOST_CMD_PPM_MODULE_INIT         = 0x80120001,          /* 模块资源初始化 */
    HOST_CMD_PPM_MODULE_DEINIT       = 0x80120002,          /* 模块资源退出 */
    HOST_CMD_PPM_SET_PKG_CAPT_REGION = 0x80120003,          /* 配置可见光抓拍区域 */
    HOST_CMD_PPM_SET_FACE_DET_REGION = 0x80120004,          /* 配置人脸ROI区域 */
    HOST_CMD_PPM_SET_CAPT_AB_REGION  = 0x80120005,          /* 配置AB抓拍区域 */
    HOST_CMD_PPM_SET_XSI_MATCH_PRM   = 0x80120006,          /* 保留: 配置X光包裹匹配参数，相关结构体参考PPM_XSI_MATCH_PRM_S */
    HOST_CMD_PPM_SET_DETECT_SENSITY  = 0x80120007,          /* 配置匹配检测灵敏度，相关结构体参考PPM_MATCH_SENSITY_E */
    HOST_CMD_PPM_SET_FACE_MATCH_THRS = 0x80120008,          /* 配置人脸匹配iou阈值 */
    HOST_CMD_PPM_SET_MATCH_MATRIX_PRM = 0x80120009,         /* 配置匹配矩阵参数 */
    HOST_CMD_PPM_SET_FACE_SCORE_FILTER = 0x8012000a,        /* 配置人脸评分过滤阈值 */
    HOST_CMD_PPM_SET_CAMERA_FIX_PRM  = 0x8012000b,          /* 暂未使用:配置相机安装固定参数(海思专属) */
    HOST_CMD_PPM_SET_CALIB_PRM       = 0x8012000c,          /* 配置标定参数，获取标定结果 */
    HOST_CMD_PPM_SET_FACE_MATRIX_PRM = 0x8012000d,          /* 暂未使用:配置人脸相机矩阵参数(海思专属)  */
    HOST_CMD_PPM_ENABLE_CHANNEL      = 0x8012000e,          /* 开启通道，目前只有一个通道，使用通道0，下同 */
    HOST_CMD_PPM_DISABLE_CHANNEL     = 0x8012000f,          /* 关闭通道，目前只有一个通道，使用通道0 */
    HOST_CMD_PPM_GENERATE_ALG_CFG_FILE = 0x80120010,        /* 保留: 生成算法配置文件 */
    HOST_CMD_PPM_GET_VERSION         = 0x80120011,          /* 获取底层版本号 */
    HOST_CMD_PPM_DEBUG               = 0x80120012,          /* 暂未使用:调试接口(海思专属)   */
    HOST_CMD_PPM_SAVE_CAMERA_FIX_PRM = 0x80120013,          /* 暂未使用:保存相机安装固定参数(海思专属)      */
    HOST_CMD_PPM_SET_FACE_CALIB_PRM  = 0x80120014,          /* 配置人脸相机标定参数，获取标定结果 */
    HOST_CMD_PPM_SET_SET_CONFIDENCE_TH = 0x80120015,        /* 配置置信度阈值 */
    HOST_CMD_PPM_SET_POS_SWITCH      = 0x80120016,          /* 配置人包pos的开关 */

    /* 其他 */
    HOST_CMD_INVALID                 = 0xffffffff,          /* 最大无效值 */
} HOST_CMD;

/*************************************************************************************
                            接口定义
*************************************************************************************/
int SendCmdToDsp(HOST_CMD cmd, unsigned int chan, unsigned int *pParam, void *pBuf);


/*******************************************************************************
* 函数名  : InitDspSys
* 描  述  : 预初始化DSP运行起来需要的基础资源，完成全局结构的空间分配；多进程下
            还会进行前期进程通讯资源的建立，调用这个接口成功返回后，APP就可以进
            行 DSPINITPARA 参数的配置了；参数配置完毕通过 HOST_CMD_DSP_INIT 命令
            把 DSP 所有资源及业务启动。
* 输  入  : - ppDspInitPara: APP将要指向全局结构体的指针
*         : - pFunc        : APP传递给DSP的回调函数体
* 输  出  : - ppDspInitPara: APP获取到的全局结构体
* 返回值  : 0  : 成功
*           -1 : 失败
*******************************************************************************/
int InitDspSys(DSPINITPARA **ppDspInitPara, DATACALLBACK pFunc);

#ifdef __cplusplus
}
#endif


#endif /* _DSPCOMMON_H_ */

