/***
 * @file   sal_video.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  视频信息、帧信息、视频数据、帧数据及Buf的定义和描述，方便平台、模块无关性的数据结构。
 * @author liwenbin
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef _SAL_VIDEO_H_
#define _SAL_VIDEO_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
/* 视频帧Buf的魔数字符 SAL VIDEO 的缩写ASIIC码: SVID */
#define SAL_MAGIC_VIDEO_BUF (0x53564944)

/* 基于画面正视图的角度: 视频翻转、镜像、旋转角度，支持扩展 */
typedef enum
{
    SAL_VIDEO_ANGLE_NONE = 0u,
    /* 水平镜像/左右镜像 */
    SAL_VIDEO_ANGLE_HFLIP = 1u,
    /* 垂直镜像/上下镜像 */
    SAL_VIDEO_ANGLE_VFLIP = 2u,
    /* 几种常见的角度 */
    SAL_VIDEO_ANGLE_90 = 90u,
    SAL_VIDEO_ANGLE_180 = 180u,
    SAL_VIDEO_ANGLE_270 = 270u
} SAL_VideoAngleType;

/* 每个像素占用的位数，支持扩展 */
typedef enum
{
    /* 8Bits每个像素 */
    SAL_VIDEO_BPP_BITS8,
    /* 12Bits每个像素 */
    SAL_VIDEO_BPP_BITS12,
    /* 16Bits每个像素 */
    SAL_VIDEO_BPP_BITS16,
    /* 24Bits每个像素 */
    SAL_VIDEO_BPP_BITS24,
    /* 32Bits每个像素 */
    SAL_VIDEO_BPP_BITS32,
    /* 最大数，可用于计数和校验 */
    SAL_VIDEO_BPP_MAX
} SAL_VideoBitsPerPixel;

typedef enum
{
    /* YUV 422交错保存格式 - UYVY。*/
    SAL_VIDEO_DATFMT_UYVY,
    /* YUV 422交错保存格式 - YUYV。*/
    SAL_VIDEO_DATFMT_YUYV,
    /* YUV 422交错保存格式 - YVYU。*/
    SAL_VIDEO_DATFMT_YVYU,
    /* YUV 422交错保存格式 - VYUY。*/
    SAL_VIDEO_DATFMT_VYUY,

    /* YUV 422半平面保存格式 - Y在一个平面，UV在另一平面（交错保存）。*/
    SAL_VIDEO_DATFMT_YUV422SP_UV,
    /* YUV 422半平面保存格式 - Y是一个平面，VU是另一平面（交错保存）。*/
    SAL_VIDEO_DATFMT_YUV422SP_VU,
    /* YUV 422平面保存格式 - Y是一个平面, V是一个平面，U也是一个平面。*/
    SAL_VIDEO_DATFMT_YUV422P,
    
    /* YUV 420半平面保存格式 - Y在一个平面，UV在另一平面（交错保存）。*/
    SAL_VIDEO_DATFMT_YUV420SP_UV,
    /* YUV 420半平面保存格式 - Y是一个平面，VU是另一平面（交错保存）。*/
    SAL_VIDEO_DATFMT_YUV420SP_VU,
    /* YUV 420平面保存格式 - Y是一个平面, U是一个平面，V也是一个平面。*/
    SAL_VIDEO_DATFMT_YUV420P,
    
    /* BGRY 4个平面保存格式，B、G、R、Y 都分别是一个独立面 */
    SAL_VIDEO_DATFMT_PLANAR_Y,
    SAL_VIDEO_DATFMT_BGRY,
    /* BGR 3个平面保存格式，B、G、R 都分别是一个独立面 */
    SAL_VIDEO_DATFMT_BGR,

    /* RGB565 16-bit保存格式，单平面，其中5bits R, 6bits G, 5bits B。*/
    SAL_VIDEO_DATFMT_RGB16_565,
    /* ARGB565 16-bit保存格式，单平面，其中1bits 为透明，5bits R, 5bits G, 5bits B。*/
    SAL_VIDEO_DATFMT_ARGB16_1555,
    /* RGB888 24-bit保存格式，单平面，R、G、B各8bits */
    SAL_VIDEO_DATFMT_RGB24_888,
    SAL_VIDEO_DATFMT_BGR24_888,
    /* RGB888 32-bit保存格式，单平面，R、G、B各8bits，透明度A 8bits */
    SAL_VIDEO_DATFMT_RGBA_8888,
    SAL_VIDEO_DATFMT_ARGB_8888,
    SAL_VIDEO_DATFMT_BGRA_8888,

    /* RAW Bayer保存格式，单平面 */
    SAL_VIDEO_DATFMT_BAYER_RAW,

    /* BITMAP 是常见位图信息 */ 
    SAL_VIDEO_DATFMT_BITMAP8,
    SAL_VIDEO_DATFMT_BITMAP16,
    SAL_VIDEO_DATFMT_BITMAP24,
    SAL_VIDEO_DATFMT_BITMAP32,
    
    /* Bit Strema保存格式，表明是编码后的数据。*/
    SAL_VIDEO_DATFMT_BIT_STREAM_PNG,
    SAL_VIDEO_DATFMT_BIT_STREAM_MJPEG,
    SAL_VIDEO_DATFMT_BIT_STREAM_MPEG4,
    SAL_VIDEO_DATFMT_BIT_STREAM_H264,
    SAL_VIDEO_DATFMT_BIT_STREAM_H265,
	/*单字节数据*/
	SAL_VIDEO_DATFMT_BAYER_FMT_BYTE,
	SAL_VIDEO_DATFMT_BAYER_RAW_DUAL,
	SAL_VIDEO_DATFMT_BAYER_RAW_SINGLE,

    /* 无效的数据格式，可用于初始化数据格式变量. */
    SAL_VIDEO_DATFMT_INVALID    = 0xF00,
    /* 最大数，可用于计数和校验*/
    SAL_VIDEO_DATFMT_MAX
} SAL_VideoDataFormat;

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* 仅仅描述图像宽高信息 */
typedef struct
{
    UINT32 width;
    UINT32 height;
} SAL_VideoSize;

/* 视频帧、单幅图像、开窗大小、抠图的矩形区域表示 */
typedef struct
{
    /* 视频区域的左偏移 */
    UINT32 left;

    /* 视频区域的上偏移 */
    UINT32 top;

    /* 视频区域的宽度 */
    UINT32 width;

    /* 视频区域的高度 */
    UINT32 height;
} SAL_VideoCrop;


typedef struct
{
    /* 基础信息，视频帧的分辨率 */
    UINT32 width;
    UINT32 height;
    /* 基础信息，本视频帧的数据格式，见 SAL_VideoDataFormat 定义的类型 */
    UINT32 dataFormat;
    /* 基础信息，本视频帧的数据位宽、颜色深度，见 SAL_VideoBitsPerPixel 定义的类型 */
    UINT32 bitsPerPixel;
    /* 基础信息，本视频帧基于画面正视图的角度信息，见 SAL_VideoAngleType 定义的类型 */
    UINT32 angle;
    /* 基础信息，本视频帧基于画面正视图的镜像位置，见 SAL_VideoAngleType 定义的类型 */
    UINT32 mirror;

    /* 高级信息，抠图信息 */
    SAL_VideoCrop crop;
    /* 高级信息，视频质量 */
    UINT32 quality;
    /* 高级信息，视频帧率 */
    UINT32 fps;
    /* 高级信息，视频制式 */
    UINT32 standard;
    /* 高级信息，码率控制方式(变码率/定码率),  0:变码率；1：定码率 */
    UINT32 bpsType;
    /* 高级信息，视频码率 */
    UINT32 bps;
    /* 高级信息，视频变类型(h264/h265)  */
    UINT32 encodeType;

    UINT32 privData;

    UINT32 reserved[6];

} SAL_VideoFrameParam;

typedef struct
{
    /* 校验魔数，检查这块Buffer是否被破坏(如DMA、越界操作等), 值固定为: SAL_MAGIC_VIDEO_BUF */
    UINT32 magicNum;
    /* 基础信息，视频基础参数 */
    SAL_VideoFrameParam frameParam;

    /* 基础信息，内存地址，虚拟地址需要通过操作系统位宽来做区别，兼容64bit系统 */
    UINT32 poolId;
    PhysAddr virAddr[4];        // 实际视频帧起始地址
    PhysAddr phyAddr[4];        // 实际视频帧起始地址
    UINT32 stride[4];
    UINT32 vertStride[4]; /* RK3588需要指定垂直方向的跨距/虚高，HISI/NT不需要 */
    /* 基础信息，视频帧数据的总长 */
    UINT32 bufLen;

    /* 高级信息，视频帧的时间戳 */
    UINT64 pts;

    /* 高级信息，物理通道号，区别不同视频源 */
    UINT32 chanId;

    /* 高级信息，同一个视频源下的复用出来的码流ID */
    UINT32 streamId;

    /* 高级信息，模块程序内部使用 */
    UINT32 interId;

    /* 高级信息，帧序号,每路码流都有自己的帧序号。*/
    UINT32 frameNum;

    /* 高级信息，该帧被使用的引用计数，建议内部使用 */
    UINT32 refCnt;

    /* 私有数据，可根据需要设置 */
    PhysAddr privateDate;                     /* vb blk */

    /* 高级信息，视频变类型(h264/h265)  */
    UINT32 encodeType;

    /*RK/NT平台使用为存储帧缓存块地址，privateDate被  sva智能占用*/
    UINT64 vbBlk;

    UINT64 reserved[6];
} SAL_VideoFrameBuf;


/* 使用空类型指针，方便各个模块之间、核间、处理器间交互视频信息，需要进行转换才能使用。
 *  视频帧BUF统一使用这个来进行抽象传递，容易理解当前使用的图像信息参数内存情况。
 *  如: SAL_VideoFrameBuf *pFrameBuf = (SAL_VideoFrameBuf *)pVideoFrame。
 */
typedef void *SAL_VideoFrame;

/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */
/**
 * @function   SAL_dspDataFormatToSal
 * @brief      转换应用下发的格式到内部格式
 * @param[in]  DSP_IMG_DATFMT unImgDataFormat  应用格式,在dspcommon中定义
 * @param[out] None
 * @return     SAL_VideoDataFormat 返回内部格式
 */
SAL_VideoDataFormat SAL_dspDataFormatToSal(DSP_IMG_DATFMT unImgDataFormat);

/**
 * @function   SAL_salDataFormatToDsp
 * @brief      转换内部格式到应用格式
 * @param[in]  SAL_VideoDataFormat unImgDataFormat  内部格式
 * @param[out] None
 * @return     DSP_IMG_DATFMT 返回应用格式
 */
DSP_IMG_DATFMT SAL_salDataFormatToDsp(SAL_VideoDataFormat unImgDataFormat);


UINT32 SAL_getBitsPerPixel(SAL_VideoDataFormat enSalPixelFmt);

UINT32 SAL_getBytesPerPixel(SAL_VideoDataFormat enSalPixelFmt);

#endif

