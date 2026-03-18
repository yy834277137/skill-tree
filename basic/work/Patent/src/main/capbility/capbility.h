/**
 * @file   capbility.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  能力集定义
 * @author dsp
 * @date   2022/8/1
 * @note
 * @note \n History
   1.日    期: 2022/8/1
     作    者: dsp
     修改历史: 创建文件
 */

#ifndef __CAPBILITY_H__
#define __CAPBILITY_H__

#ifdef __cplusplus
extern "C" {
#endif

/* ============================ 头文件 ============================= */

#include "sal.h"
#include "common_boardtype.h"
#include "dspcommon.h"


/* ============================ 宏/枚举 ============================ */

#define DUP_VPSS_CHN_MAX_NUM    (4)     /* 海思平台相关 */
#define XSP_RESIZE_FACTOR_MAX   (2)     /* 超分最大系数 */

#define ENABLED		(1)
#define DISABLED	(0)

#define CAPT_DEV_MAX	(2)         /* 采集设备最大通道，要增加请修改宏               */
#define CAPT_CHIP_MAX	(3)         /* 外接采集芯片最大为3，要增加请修改宏               */


/* X光机能量类型 */
typedef enum
{
    XRAY_ENERGY_T_UND = 0,          /* 未定义 */
    XRAY_ENERGY_LOW	= 1,            /* 低能，缩写代码LE */
    XRAY_ENERGY_HIGH = 2,           /* 高能，缩写代码HE */

    XRAY_ENERGY_T_MIN = INT32_MIN,
    XRAY_ENERGY_T_MAX = INT32_MAX
} XRAY_ENERGY_TYPE;

/* 海思各模块之间的连接关系 */
typedef enum
{
    MOD_LINK_TYPE_BIND = 0,         /* HISI内部绑定 */
    MOD_LINK_TYPE_USER,             /* 送帧方式 */
    MOD_LINK_TYPE_BUTT,
} MOD_LINK_TYPE_E;

/* MIPI TX连接的芯片 */
typedef enum
{
    MIPI_TX_CHIP_LT8912B,
    MIPI_TX_CHIP_6211,
} MIPI_TX_CHIP_E;


/* 视角个数 */
typedef enum
{
    XSP_VANGLE_ZERO = 0,           /* 未定义 */
    XSP_VANGLE_SINGLE = 1,         /* 单视角 */
    XSP_VANGLE_DUAL = 2,           /* 双视角 */
} XSP_VANGLE_T;

/* ========================= 结构体/联合体 ========================== */

/**
 * @struct  CAPB_XRAY_IN
 * @brief   X-Ray输入源能力集
 */
typedef struct
{
    UINT32 xray_in_chan_cnt;            /* X-Ray源数量，安检机默认值为1，分析仪默认值为0 */
    XRAY_DET_VENDOR xray_det_vendor;    /* X-Ray探测板供应商，默认值为0 */
    XRAY_ENERGY_NUM xray_energy_num;    /* 单个X-Ray源的能量数，1-单能SE、2-双能DE，默认值为1 */
    struct
    {
        UINT32 integral_time;           /* 单行数据的积分时间，单位：us */
        UINT32 line_per_slice;          /* 每帧的行数，即条带高度，单位：Pixel */
        FLOAT32 belt_speed;             /* 传送带速度，单位：m/s */
    } st_xray_speed[XRAY_FORM_NUM][XRAY_SPEED_NUM]; /* 最多支持3种速度，分别为低速、中速、高速，若某个速度不支持，则该组参数值均为0 */
    UINT32 xraw_width_max;              /* X-Ray RAW数据宽最大值，5030是64*7，6550是64*10 */
    UINT32 xraw_height_max;             /* X-Ray RAW数据高最大值，默认值为1920 */
    UINT32 line_per_slice_max;          /* 最大条带高度 */
    UINT32 line_per_slice_min;          /* 最小条带高度 */
    UINT32 u32MaxSourceDist;            /* 双视角源间隔长度在不同速度下最大采集数据列数，单视角机型固定为100列 */
    UINT32 xraw_bytewidth;              /* X-Ray RAW数据占用字节数，默认值为2，RAW数据大小通过width*height*bytewidth计算 */
} CAPB_XRAY_IN;

/**
 * @struct  CAPB_XSP
 * @brief   XSP软件能力集
 */
typedef struct
{
    UINT32 xsp_original_raw_bw;         /* 原始RAW数据每个像素占用的字节数，双能为4，单能为2 */
    UINT32 xsp_normalized_raw_bw;       /* 归一化RAW数据每个像素占用的字节数，双能为5，单能为2 */
    UINT32 xsp_ori_normalized_raw_bw;    /* 超分之前的归一化NRAW数据每个像素占用的字节数，双能为6，单能为2；目前在液体识别、数据采集时使用 */
    FLOAT32 resize_width_factor;        /* RAW域图像宽度缩放系数（探测器方向） */
    FLOAT32 resize_height_factor;       /* RAW域图像高度缩放系数（时间方向） */
    UINT32 xraw_width_resized;          /* 经过超分处理的NRAW数据宽最大值 */
    UINT32 xraw_height_resized;         /* 经过超分处理的NRAW数据高最大值 */
    UINT32 xraw_width_resized_max;      // 经过最大超分比例处理的RAW数据高最大值
    UINT32 xraw_height_resized_max;     // 经过最大超分比例处理的RAW数据高最大值
    UINT32 xsp_package_line_max;        /* 包裹分割列数阈值，基于未超分的XRaw数据行数，范围：[640, 1600] */
    UINT32 xsp_fsc_proc_width_max;      // 单次全屏处理的最大数据宽度（显示图像）
    DSP_IMG_DATFMT enDispFmt;           /* xsp输出显示数据的格式，yuv或rgb */
} CAPB_XSP;

/**
 * @struct  CAPB_XPACK
 * @brief   XPACK软件能力集
 */
typedef struct
{
    UINT32 xpack_disp_buffer_w;             /* 拼帧送显循环缓冲区宽度，单位：Pixel */
    UINT32 u32XpackSendChnNum;              /* xpack帧数据发送通道个数 */
    INT32 uiCoreDispVpss[MAX_XRAY_CHAN];    /* 显示数据发送线程绑核, <0表示不绑核 */
    UINT32 u32AiDgrSegLen[XRAY_SPEED_NUM];  /* 包裹危险品分段识别长度，0xFFFFFFFF表示不分段 */
    UINT32 u32AiDgrHeight;                  /* 用于包裹危险品识别图像数据的高度 */
} CAPB_XPACK;

/**
 * @struct  CAPB_AI
 * @brief   智能识别能力集
 */
typedef struct
{
    UINT32 ai_chn;                      /* 智能通道数，目前只有6550D安检机和分析仪为双路，其他均为单路 */
    UINT32 uiCbJpgNum;                  /* 智能回调jpg个数，位示图表示，从低位计算 */
    UINT32 uiCbBmpNum;                  /* 智能回调bmp个数，位示图表示，从低位计算 */
    UINT32 uiIaMemType;                 /* 智能内存分配方式，参考MEM_INIT_TYPE_E */
    SVA_MACHINE_TYPE ai_mechine_type;   /* 智能安检机机型类别，用来判断缩放因子 */
} CAPB_AI;

/* 显示画框，OSD使用的模块类型定义 */
typedef enum
{
    DRAW_MOD_VGS = 0,              /* 海思VGS模块 */
    DRAW_MOD_DSP,                  /* 海思DSP模块 */
    DRAW_MOD_CPU,                  /* 软件 */
    DRAW_MOD_BUTT,
} DRAW_MOD_E;

/* 框超出图像怎么画 */
typedef enum
{
    RECT_EXCEED_TYPE_CLOSE = 0,        /* 封闭矩形，在边缘处宽缩小 */
    RECT_EXCEED_TYPE_OPEN,              /* 开放矩形，超出的不画 */
    RECT_EXCEED_TYPE_BUTT,
} RECT_EXCEED_TYPE_E;

/**
 * @struct  CAPB_LINE
 * @brief   画线和框共用该参数
 */
typedef struct
{
    DRAW_MOD_E enDrawMod;           /* 使用的模块 */
    RECT_EXCEED_TYPE_E enRectType;
    UINT32 u32LineWidth;            /* 线宽 */
    UINT32 u32DispLineWidth;        /* 显示画线线宽 */
    BOOL bUseFract;
} CAPB_LINE;

/* OSD字体类型 */
typedef enum
{
    OSD_FONT_LATTICE,       /* 点阵 */
    OSD_FONT_TRUETYPE,
    OSD_FONT_BUTT,
} OSD_FONT_TYPE_E;

/**
 * @struct  CAPB_OSD
 * @brief   OSD参数
 */
typedef struct
{
    DRAW_MOD_E enDrawMod;           /* 使用模块 */
    OSD_FONT_TYPE_E enFontType;     /* 字体类型 */
    UINT32 TrueTypeSize[3];         /* 矢量字体的大小 */
} CAPB_OSD;

/**
 * @struct  CAPB_DISP
 * @brief   显示能力集
 */
typedef struct
{
    UINT32 disp_vodev_cnt;              /* 显示设备个数 */
    UINT32 disp_videv_cnt;              /* 采集通道个数 */
    UINT32 disp_chan_cnt;               /* 显示通道数，默认值为1 */
    UINT32 disp_yuv_w_max;              /* 显示YUV的最大宽度，默认值为1920 */
    UINT32 disp_yuv_h_max;              /* 显示YUV的最大高度 */
    UINT32 disp_yuv_h;                  /* 显示YUV的高度，已经加上了padding，即disp_padding_top+disp_padding_bottom+disp_h_indeed_middle=disp_yuv_h */
    UINT32 disp_h_middle_indeed;        /* 显示YUV图像中间区域实际有效数据高 */
    UINT32 disp_h_top_padding;          /* 显示YUV图像顶部padding高 */
    UINT32 disp_h_bottom_padding;       /* 显示YUV图像底部padding高 */
    UINT32 disp_h_top_blanking;         /* 显示YUV图像顶部blanking高 */
    UINT32 disp_h_bottom_blanking;      /* 显示YUV图像底部blanking高 */
    UINT32 disp_vpsscnt;                /* 显示vpss通道缓存buffer个数 */
    UINT32 disp_screen_bg_color;        /* 显示通道背景色 */
    SAL_VideoDataFormat enInputSalPixelFmt; /* 显示模块的输入像素格式 */
    UINT32 disp_delay_nums;             /* 输出延迟帧数，分析仪里需要等算法处理完毕进行匹配，安检机可设置为0 */
    MOD_LINK_TYPE_E disp_link_type;     /* VO与上一级模块绑定关系 */
    MIPI_TX_CHIP_E disp_mipi_chip;      /* MIPI外接的芯片 */
    UINT32 disp_vpss_gra_chn_0;         /* 送显的vpss通道号 */
    UINT32 disp_vpss_gra_chn_1;         /* 送显的vpss通道号 */
} CAPB_DISP;

/**
 * @struct  CAPB_VENC
 * @brief   编码能力集
 */
typedef struct
{
    UINT32 srcfps; /* 编码输入帧率 */
    UINT32 dstfps; /* 编码输出帧率 */
    UINT32 venc_cnt; /* 解码最大个数 */
    UINT32 resolution; /* 默认分辨率 */
    UINT32 venc_width; /* 默认编码宽度 */
    UINT32 venc_height; /* 默认编码高度 */
    UINT32 venc_bpsType; /* 码率控制模式 cbr vbr */
    UINT32 venc_bps; /* 默认编码码率 */
    UINT32 venc_quailty; /* 图像质量 */
    UINT32 venc_maxwidth; /* 编码最大宽度 */
    UINT32 venc_maxheight; /* 编码最大高度 */
    UINT32 venc_scale;     /* 编码模块的缩放功能是否开启, 0关闭 1开启 */
} CAPB_VENC;

/**
 * @struct  CAPB_VDEC
 * @brief   解码能力集
 */
typedef struct
{
    UINT32 vdec_cnt; /* 解码最大个数 */
    UINT32 vpsschn_cnt; /* vpss通道个数设置 */
} CAPB_VDEC;

/**
 * @struct  CAPB_RECODE
 * @brief   转包能力集
 */
typedef struct
{

} CAPB_RECODE;

/**
 * @struct  CAPB_AUDIO
 * @brief   音频能力集
 */
typedef struct
{
    UINT32 audiodevidx;
} CAPB_AUDIO;

/**
 * @enum  CAPT_CHIP_TYPE_E
 * @brief  采芯片类型
 */
typedef enum
{
    CAPT_CHIP_NONE = 0,
    CAPT_CHIP_FPGA = 1,
    CAPT_CHIP_MST91A4 = 2,
    CAPT_CHIP_LT9211_MIPI = 3,
    CAPT_CHIP_LT86102SXE = 4,
    CAPT_CHIP_TYPE_BUTT = 0xff,
} CAPT_CHIP_TYPE_E;

/**
 * @struct  tagCaptChip
 * @brief   I2C总线
 */
typedef struct tagI2cAttr
{
    UINT8 u8BusIdx;                                     /* i2c总线号 */
    UINT8 u8ChipAddr;                                   /* 芯片设备地址 */
} CAPB_I2C_ATTR_T;

/**
 * @struct  tagCaptChip
 * @brief   UART总线
 */
typedef struct tagUartAttr
{
    UINT8 u8BusIdx;                                     /* uart总线号 */
} CAPB_UART_ATTR_T;

/**
 * @struct  tagCaptChip
 * @brief   SPI总线
 */
typedef struct tagSpiAttr
{
    UINT8 u8BusIdx;                                     /* SPI总线号 */

} CAPB_SPI_ATTR_T;

/**
 * @struct  tagCaptChip
 * @brief   采集芯片
 */
typedef struct tagCaptChip
{
    CAPT_CHIP_TYPE_E enCaptChip;                        /* 芯片名称 */
    union
    {
        CAPB_I2C_ATTR_T stChipI2c;
        CAPB_UART_ATTR_T stChipUart;
        CAPB_SPI_ATTR_T stChipSpi;
    };

} CAPT_CHIP_INFO_S;

/**
 * 采集AD 配置集(set)
 */
typedef struct tagCaptChipCfg
{
    UINT8 u8ChnCnt;
    UINT8 u8ChipNum;                                    /* 该输出通路显示芯片个数 */
    CAPT_CHIP_INFO_S astChipInfo[CAPT_CHIP_MAX];        /* 对应通道下的几个芯片参数 */
} CAPT_CHIP_CFG_S;

/**
 * @struct  CAPB_CAPT
 * @brief   采集能力集
 */
typedef struct
{
    BOOL bFpgaEnable;
    UINT32 uiUserFps;      /* dsp内部生成输入视频的帧率，安检机上使用 */
    UINT32 uiViCnt;        /* 采集输入路数 */
    BOOL bMcuEnable;
    union
    {
        UINT32 auiHwMipi[2];    /* 硬件上连接的MIPI引脚 */
    };
    CAPT_CHIP_CFG_S stCaptChip[CAPT_DEV_MAX];
} CAPB_CAPT;

/**
 * @struct  CAPB_JPEG
 * @brief   抓图能力集
 */
typedef struct
{
    UINT32 videv_cnt; /* 采集通道个数 */
    UINT32 maxdev_num; /* 采集+解码 */
    UINT32 maxchn_num; /* 采集+解码+智能 */
} CAPB_JPEG;

typedef enum
{
    VPSS_DEV_VPE = 0x0,     /* VPE device */
    VPSS_DEV_GPU = 0x1,     /* GPU device */
    VPSS_DEV_RGA = 0x2,     /* RGA device */
    VPSS_DEV_BUTT
} VPSS_DEV_TYPE_E;

typedef struct
{
    UINT32 uiBlkCnt;
    UINT32 uiDepth;
    VPSS_DEV_TYPE_E enVpssDevType;
    UINT32 uiChnEnable[DUP_VPSS_CHN_MAX_NUM];
} VPSS_GROUP_ATTR;

/**
 * @struct  CAPB_DUP
 * @brief   数据分发模块
 */
typedef struct
{
    UINT32 width; /* vpss默认宽度设置 */
    UINT32 height; /* vpss默认高度设置 */
    SAL_VideoDataFormat enInputSalPixelFmt; /* 输入的像素格式 */
    SAL_VideoDataFormat enOutputSalPixelFmt; /* 输出的像素格式 */
    VPSS_GROUP_ATTR frontVpssGrp;
    VPSS_GROUP_ATTR rearVpssGrp;
    UINT32 u32FrontBindRear; /* 1表示两个vpss需要绑定，0不需要绑定。安检机里不需要绑定。 */
} CAPB_DUP;

/**
 * @struct  CAPB_DUP
 * @brief   数据分发模块
 */
typedef struct
{
    UINT32 width; /* vpss默认宽度设置 */
    UINT32 height; /* vpss默认高度设置 */
    SAL_VideoDataFormat enInputSalPixelFmt; /* 输入的像素格式 */
    SAL_VideoDataFormat enOutputSalPixelFmt; /* 输出的像素格式 */
    VPSS_GROUP_ATTR frontVpssGrp;
    VPSS_GROUP_ATTR rearVpssGrp;
    UINT32 u32HasFrontDupScale; /* 1表示有前级DUP做放大，需要前级放大是由于类似SD3403平台VPSS同一时刻只有一个chn能做放大。 */
} CAPB_VDEC_DUP;

/**
 * @struct  CAPB_VPROC
 * @brief   视频处理模块的配置信息，比如分析仪用该模块做镜像功能
 */
typedef struct
{
    UINT32 width; /* vpss默认宽度设置 */
    UINT32 height; /* vpss默认高度设置 */
    SAL_VideoDataFormat enInputSalPixelFmt; /* 输入的像素格式 */
    SAL_VideoDataFormat enOutputSalPixelFmt; /* 输出的像素格式 */
    VPSS_GROUP_ATTR vpssGrp;
} CAPB_VPROC;

/**
 * @struct  CAPB_DSP
 * @brief   DSP核能力集
 */
typedef struct
{
    UINT32 u32UseCoreNum;
    UINT32 au32Core[4];
} CAPB_DSP;

/**
 * @enum  CAPT_INPUT_TYPE_E
 * @brief 采集输入类型
 */
typedef enum
{
    VIDEO_INPUT_OUTSIDE = 0, /*采集由VI输入*/
    VIDEO_INPUT_INSIDE,      /*采集由DSP内部生成*/
    VIDEO_INPUT_BUTT,
} VIDEO_INPUT_TYPE_E;

/**
 * @struct  CAPB_PRODUCT
 * @brief   产品能力集，主要控制模块是否使能，比如视频输入、xpack、xray、xsp等在不同产品里是否使能
 */
typedef struct
{
    VIDEO_INPUT_TYPE_E enInputType;         /* 视频输入类型 */
    BOOL bXPackEnable;                      /* 是否开启XPack */
    BOOL bXRayEnable;                       /* 是否开启XRay */
    BOOL bXspEnable;                        /* 是否开启Xsp */
} CAPB_PRODUCT;

/**
 * @struct  CAPB_PLATFORM
 * @brief   平台能力集
 */
typedef struct
{
    BOOL bHardCopy;                         /* 平台是否支持硬拷贝 */
    BOOL bHardMmap;                         /* 平台硬件模块是否支持做YC伸张 */
    BOOL bFrameCrop;                        /* 平台是否采用帧裁剪，NT98336里xpack大缓冲区需要通过裁剪获取显示帧 */
    BOOL bDiscontAddrOsd;                   /* 是否支持不连续地址画osd */
    BOOL bTdeCoordiateTrans;                /* 调用底层TDE接口之前的坐标转换，这里主要是RK平台TDE拷贝时X/Y坐标要以mb起始点为原点进行计算；而xpack里进行拷贝时下发的参数是以需要拷贝的区域RECT的起始点为原点，所以需要进行X/Y坐标转换 */
} CAPB_PLATFORM;

/**
 * @struct  CAPB_OVERALL
 * @brief   整体能力集
 */
typedef struct
{
    /* 1、产品能力集 */
    CAPB_PRODUCT capb_product;
    /* 2、平台能力集 */
    CAPB_PLATFORM capb_platform;

    /* 3、各个模块的能力集 */
    CAPB_XRAY_IN capb_xrayin;
    CAPB_XSP capb_xsp;
    CAPB_XPACK capb_xpack;
    CAPB_AI capb_ai;
    CAPB_DISP capb_disp;
    CAPB_VENC capb_venc;
    CAPB_VDEC capb_vdec;
    CAPB_AUDIO capb_audio;
    CAPB_CAPT capb_capt;
    CAPB_JPEG capb_jpeg;
    CAPB_RECODE capb_recode;
    CAPB_DUP capb_dup;
    CAPB_VDEC_DUP capb_vdecDup;
    CAPB_VPROC capb_vproc;
    CAPB_DSP capb_dsp;
    CAPB_LINE capb_line;
    CAPB_OSD capb_osd;
} CAPB_OVERALL;

/* =================== 函数申明，static && extern =================== */

/**
 * @fn      capb_get_xrayin
 * @brief   获取CAPB_XRAY_IN能力
 *
 * @return  CAPB_XRAY_IN* 能力参数
 */
CAPB_XRAY_IN *capb_get_xrayin(void);

/**
 * @fn      capb_get_xsp
 * @brief   获取CAPB_XSP能力
 *
 * @return  CAPB_XSP* 能力参数
 */
CAPB_XSP *capb_get_xsp(void);

/**
 * @fn      capb_get_xpack
 * @brief   获取CAPB_XPACK能力
 *
 * @return  CAPB_XPACK* 能力参数
 */
CAPB_XPACK *capb_get_xpack(void);

/**
 * @fn      capb_get_ai
 * @brief   获取CAPB_AI能力
 *
 * @return  CAPB_AI* 能力参数
 */
CAPB_AI *capb_get_ai(void);

/**
 * @fn      capb_get_disp
 * @brief   获取CAPB_DISP能力
 *
 * @return  CAPB_DISP* 能力参数
 */
CAPB_DISP *capb_get_disp(void);

/**
 * @fn      capb_get_line
 * @brief   获取CAPB_LINE能力
 *
 * @return  CAPB_LINE* 能力参数
 */
CAPB_LINE *capb_get_line(void);

/**
 * @fn      capb_get_osd
 * @brief   获取CAPB_OSDE能力
 *
 * @return  CAPB_OSD* 能力参数
 */
CAPB_OSD *capb_get_osd(void);

/**
 * @fn      capb_get_disp_chan_cnt
 * @brief   获取显示通道数
 *
 * @return  UINT32 显示通道数
 */
UINT32 capb_get_disp_chan_cnt(void);

/**
 * @fn      capb_get_disp_height
 * @brief   获取输入VPSS未缩放时的YUV高
 *
 * @return  UINT32 输入VPSS未缩放时的YUV高
 */
UINT32 capb_get_disp_height(void);

/**
 * @fn      capb_get_venc
 * @brief   获取CAPB_VENC能力
 *
 * @return  CAPB_VENC* 能力参数
 */
CAPB_VENC *capb_get_venc(void);

/**
 * @fn      capb_get_vdec
 * @brief   获取CAPB_VDEC能力
 *
 * @return  CAPB_VDEC* 能力参数
 */
CAPB_VDEC *capb_get_vdec(void);

/**
 * @fn      capb_get_audio
 * @brief   获取CAPB_AUDIO能力
 *
 * @return  CAPB_AUDIO* 能力参数
 */
CAPB_AUDIO *capb_get_audio(void);

/**
 * @fn      capb_get_dup
 * @brief   获取CAPB_DUP能力
 *
 * @return  CAPB_DUP* 能力参数
 */
CAPB_DUP *capb_get_dup(void);

/**
 * @fn      capb_get_vdecDup
 * @brief   CAPB_VDEC_DUP
 *
 * @return  CAPB_VDEC_DUP* 能力参数
 */
CAPB_VDEC_DUP *capb_get_vdecDup(void);

/**
 * @fn      capb_get_vproc
 * @brief   获取CAPB_VPROC能力
 *
 * @return  CAPB_VPROC* 能力参数
 */
CAPB_VPROC *capb_get_vproc(void);

/**
 * @fn      capb_get_jpeg
 * @brief   获取CAPB_JPEG能力
 *
 * @return  CAPB_JPEG* 能力参数
 */
CAPB_JPEG *capb_get_jpeg(void);

/**
 * @fn      capb_get_capt
 * @brief   获取CAPB_CAPT能力
 *
 * @return  CAPB_CAPT* 能力参数
 */
CAPB_CAPT *capb_get_capt(void);

/**
 * @fn      capb_get_product
 * @brief   获取CAPB_PRODUCT能力
 *
 * @return  CAPB_PRODUCT* 能力参数
 */
CAPB_PRODUCT *capb_get_product(void);

/**
 * @fn      capb_get_platform
 * @brief   获取CAPB_PRODUCT能力
 *
 * @return  CAPB_PLATFORM* 能力参数
 */
CAPB_PLATFORM *capb_get_platform(void);

/**
 * @fn      capbility_init
 * @brief   能力集初始化
 *
 * @param   board_type[IN] 设备型号
 *
 * @return  SAL_STATUS SAL_SOK-初始化成功，SAL_FAIL-初始化失败
 */
SAL_STATUS capbility_init(DSPINITPARA *pstInit);

/**
 * @fn      capb_ism_init
   void capb_isa_box_init(CAPB_OVERALL *p_overall, const BOARD_TYPE board_type);
 * @brief   安检机能力集初始化
 *
 * @param   p_overall[OUT] 能力集参数
 */
void capb_ism_init(CAPB_OVERALL *p_overall, DSPINITPARA *pstInit);

/**
 * @fn      capb_analyzer_init
 * @brief   分析仪能力集初始化
 *
 * @param   p_overall[OUT] 能力集参数
 */
void capb_isa_box_init(CAPB_OVERALL *p_overall, const BOARD_TYPE board_type);

/**
 * @fn      capb_isa_init
 * @brief   分析仪能力集初始化
 *
 * @param   p_overall[OUT] 能力集参数
 */
void capb_isa_init(CAPB_OVERALL *p_overall, const BOARD_TYPE board_type);

#ifdef __cplusplus
}
#endif

#endif

