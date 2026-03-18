/**
 * @file   ximg_proc.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  图像处理公共头文件，统一安检机各种类型的数据，各种类型的数据均可通过此接口统一表示和处理，持续优化
 *  
 * @date   2022/03/04
 */
#ifndef _XIMG_PROC_H_
#define _XIMG_PROC_H_


/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "sal.h"


/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define XIMAGE_PLANE_MAX            (4)     // 图像平面数，最多支持4个平面

#define XIMAGE_WIDTH_MAX            (3840)
#define XIMAGE_HEIGHT_MAX           (2560)

#define XIMG_32BIT_Y_VAL(x)         ((x) >> 16 & 0xFF)
#define XIMG_32BIT_U_VAL(x)         ((x) >> 8 & 0xFF)
#define XIMG_32BIT_V_VAL(x)         ((x) >> 0 & 0xFF)
#define XIMG_32BIT_A_VAL(x)         ((x) >> 24 & 0xFF)
#define XIMG_32BIT_B_VAL(x)         ((x) >> 16 & 0xFF)
#define XIMG_32BIT_G_VAL(x)         ((x) >> 8 & 0xFF)
#define XIMG_32BIT_R_VAL(x)         ((x) >> 0 & 0xFF)
#define XIMG_BG_DEFAULT_YUV         (0xFFEB8080)        /* 默认背景YUV值 */
#define XIMG_BG_DEFAULT_RGB         (0xFFFFFFFF)        /* 默认背景ARGB值 */
#define XIMG_BG_ANTI_YUV(x)         ((0xFF << 24) | \
                                    ((0xEB - XIMG_32BIT_Y_VAL(x)) << 16) | \
                                    ((XIMG_32BIT_U_VAL(x)) << 8) | \
                                    ((XIMG_32BIT_V_VAL(x)) << 0))
#define XIMG_BG_ANTI_RGB(x)         (~(x) | 0xFF000000)


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

typedef RECT XIMG_RECT; // 矩形框定义

typedef struct // 边界定义，围成的矩形宽为（right-left+1），高为（bottom-top+1）
{
    UINT32 u32Top;        // 上边界，与图像上沿的距离
    UINT32 u32Bottom;     // 下边界，与图像上沿的距离
    UINT32 u32Left;       // 左边界，与图像左沿的距离
    UINT32 u32Right;      // 右边界，与图像左沿的距离
} XIMG_BORDER;

typedef struct // 点定义
{
    UINT32 u32X; // 横坐标
    UINT32 u32Y; // 纵坐标
} XIMG_POINT;

typedef struct // 直线定义
{
    XIMG_POINT u32PointStart; // 起始点
    XIMG_POINT u32PointEnd; // 结束点
} XIMG_LINE;

/* Media Buffer 属性 */
typedef struct
{
    PhysAddr phyAddr;           /* 内存起始物理地址，初始化后不再修改 */
    VOID *virAddr;              /* 内存起始虚拟地址，初始化后不再修改 */
    UINT32 memSize;             /* 内存大小，初始化后不再修改，单位：字节 */
    UINT32 poolId;              /* 内存池ID，各SOC私有信息，外部仅透传 */
    UINT64 u64MbBlk;            /* Media Buffer block属性，各SOC私有信息，可为block编号，也可为block属性指针，外部仅透传 */
    BOOL bCached;               /* 是否为Cached内存，TRUE-Cached内存，记得调用Flush Cache将数据写回内存 */
} MEDIA_BUF_ATTR;

/* 图像统一格式，支持表示BGR，YUV，融合图像，高低能原子序数图像，高低能输入图像 */
typedef struct
{
    DSP_IMG_DATFMT enImgFmt;                /* 图像格式 */
    UINT32 u32Width;                        /* 图像宽 */
    UINT32 u32Height;                       /* 图像高 */
    UINT32 u32Stride[XIMAGE_PLANE_MAX];     /* 图像跨距 */
    VOID *pPlaneVir[XIMAGE_PLANE_MAX];      /* 每个图像平面的数据起始地址 */
    MEDIA_BUF_ATTR stMbAttr;                /* Media Buffer属性 */
} XIMAGE_DATA_ST;

typedef enum _stXImageFlipMode_
{
    XIMG_FLIP_NONE = 0,  // 无镜像
    XIMG_FLIP_VERT = 1,  // 竖直方向镜像，高度方向
    XIMG_FLIP_HORZ = 2,  // 水平方向镜像，宽度方向
} XIMAGE_FLIP_MODE;

/* ========================================================================== */
/*                        Function Declarations                               */
/* ========================================================================== */

/**
 * @fn      ximg_create
 * @brief   根据图像宽高和格式，创建一张图像，可在接口内部申请图像内存，也可自指定图像内存
 * @param[IN]   UINT32          u32ImgWidth   图像宽度
 * @param[IN]   UINT32          u32ImgHeight  图像高度
 * @param[IN]   DSP_IMG_DATFMT  enDataFmt     图像格式
 * @param[IN]   CHAR            *szMemName    Buffer名，统计内存使用率时使用
 * @param[IN]   VOID            *pDataVir     指定的数据内存，为空时由接口内部申请内存
 * @param[OUT]  XIMAGE_DATA_ST  *pstImageData 输出的图片
 * @param[OUT]      enImgFmt  图像格式，见枚举DSP_IMG_DATFMT
 * @param[OUT]      u32Width  图像宽，单位：Pixel
 * @param[OUT]      u32Height 图像高，单位：Pixel
 * @param[OUT]      u32Stride 内存跨距，单位：Byte，N个平面即N个元素有效
 * @param[IN/OUT]   stMbAttr:
 * @param[IN]           bCached Buffer是否为Cached，非硬件访问一般使用Cached内存，以提高访问速率
 * @param[OUT]          memSize 内存大小，单位：Byte
 * @param[OUT]          phyAddr 内存物理地址（有些Soc不支持，该值为0）
 * @param[OUT]          virAddr 内存虚拟地址
 * @param[OUT]          poolId  内存池ID，各SOC私有信息，外部仅透传 
 * @param[OUT]          u64MbBlk Media Buffer block属性，各SOC私有信息，可为block编号，也可为block属性指针，外部仅透传
 * @return  SAL_STATUS SAL_SOK：申请成功，SAL_FAIL：申请失败
 */
SAL_STATUS ximg_create(UINT32 u32ImgWidth, UINT32 u32ImgHeight, DSP_IMG_DATFMT enDataFmt, CHAR *szMemName, VOID *pDataVir, XIMAGE_DATA_ST *pstImageData);

/**
 * @fn          ximg_create_sub
 * @brief       将原图中的一部分构造为一张子图像，仍使用原图内存
 * @param[IN]  XIMAGE_DATA_ST *pstSrcImg  源图像数据
 * @param[OUT] XIMAGE_DATA_ST *pstDstImg  目标图像数据
 * @param[IN]  SAL_VideoCrop  *pstCropPrm 子图像范围
 * @return      SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS ximg_create_sub(XIMAGE_DATA_ST *pstSrcImg, XIMAGE_DATA_ST *pstDstImg, SAL_VideoCrop *pstCropPrm);

/**
 * @fn      ximg_destroy
 * @brief   销毁一张图片，由图像接口内部申请的内存时回收内存
 * @param[IN]   XIMAGE_DATA_ST *pstImageData 输出的图片
 * @return      SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS ximg_destroy(XIMAGE_DATA_ST *pstImageData);

/**
 * @function   ximg_get_size
 * @brief      获取不同格式图像数据尺寸
 * @param[IN]  XIMAGE_DATA_ST *pstImg        输入图像
 * @param[OUT] UINT32         *pu32DataSize  输出的图像数据尺寸
 * @return     SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_get_size(XIMAGE_DATA_ST *pstImg, UINT32 *pu32DataSize);

/**
 * @function   ximg_get_height
 * @brief      获取不同格式图像数据高度(yuv格式返回y平面高)
 * @param[IN]  XIMAGE_DATA_ST *pstImg   输入图像
 * @return     图像数据高度,-1表示失败
 */
INT32 ximg_get_height(XIMAGE_DATA_ST *pstImg);

/**
 * @function   ximg_set_dimension
 * @brief      修改图像维度信息，不做内存调整，修改后重新计算各个平面起始地址，修改跨距时确保在对图像修改前进行，否则图像会错乱
 * @param[IN]  XIMAGE_DATA_ST *pstImg       输入图像
 * @param[IN]  UINT32         u32ImgWidth   期望的图像宽度，不得大于u32ImgStride
 * @param[IN]  UINT32         u32ImgStride  期望的图像跨距
 * @param[IN]  BOOL           bAdjustMemory 是否重新调整内存排布
 * @param[IN]  UINT32         u32Color      图像宽度增加时对增加部分预填充的图像颜色
 * @return     SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_set_dimension(XIMAGE_DATA_ST *pstImage, UINT32 u32ImgWidth, UINT32 u32ImgStride, BOOL bAdjustMemory, UINT32 u32Color);

/**
 * @fn        ximg_fill_color
 * @brief     对图像填充颜色，暂仅支持DSP_IMG_DATFMT_BGRA32，DSP_IMG_DATFMT_BGR24，DSP_IMG_DATFMT_YUV420SP_VU/UV，DSP_IMG_DATFMT_SP16格式
 * @param[IN] XIMAGE_DATA_ST *pstImageData 输入的图像   
 * @param[IN] UINT32         u32StartRow    开始填充行（含），范围为 0 ~ image height - 1
 * @param[IN] UINT32         u32FillHeight  填充的高度，单位：pixel
 * @param[IN] UINT32         u32StartCol   开始填充列（含），范围为 0 ~ image width - 1
 * @param[IN] UINT32         u32FillWidth  填充的宽度，单位：pixel
 * @param[IN] UINT32         u32BgValue     填充的颜色值
 * @return    SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS ximg_fill_color(XIMAGE_DATA_ST *pstImageData, UINT32 u32StartRow, UINT32 u32FillHeight, UINT32 u32StartCol, UINT32 u32FillWidth, UINT32 u32BgValue);

/**
 * @function       ximg_flip
 * @brief          将源图像按照指定镜像方向拷贝给目标图像，镜像时支持就地处理，直接修改原图像
 * @param[IN]      XIMAGE_DATA_ST   *pstSrcImg  源图像数据
 * @param[IN/OUT]  XIMAGE_DATA_ST   *pstDstImg  目标图像数据
 * @param[IN/OUT]  XIMAGE_FLIP_MODE enFlipMode  图像镜像方向
 * @return         SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_flip(XIMAGE_DATA_ST *pstSrcImg, XIMAGE_DATA_ST *pstDstImg, XIMAGE_FLIP_MODE enFlipMode);

/**
 * @function       ximg_crop
 * @brief          从源图像中抠取指定范围内的图像贴到目标图像的指定位置
 * @param[IN]      XIMAGE_DATA_ST *pstImageSrc   源图像数据
 * @param[IN/OUT]  XIMAGE_DATA_ST *pstImageDst   目标图像数据，为空时，在原图上操作
 * @param[IN]           enImgFmt  图像格式，见枚举DSP_IMG_DATFMT
 * @param[IN]           u32Width  目标图像宽，单位：Pixel
 * @param[IN]           u32Height 目标图像高，单位：Pixel
 * @param[IN]           u32Stride 内存跨距，单位：Byte，N个平面即N个元素有效
 * @param[OUT]          pPlaneVir 抠取完成后的目标图像数据
 * @param[IN]           stMbAttr  内存信息
 * @param[IN/OUT]  SAL_VideoCrop *pstSrcCropPrm 源图像抠取范围，为空时表示抠取全图
 * @param[IN/OUT]  SAL_VideoCrop *pstDstCropPrm 目标图像中存放抠图位置，宽高与源图抠取宽高相同，为空时表示抠取全图，
 *                                              源抠取参数和目标抠取参数均为空时，原图和目标图片需相同尺寸
 * @return         SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_crop(XIMAGE_DATA_ST *pstImageSrc, XIMAGE_DATA_ST *pstImageDst, SAL_VideoCrop *pstSrcCropPrm, SAL_VideoCrop *pstDstCropPrm);

/**
 * @fn      ximg_copy_nb
 * @brief   非常牛掰的ximg拷贝，支持各种局部拷贝&镜像拷贝
 * 
 * @param   pstImageSrc[IN] 源图像数据，除stMbAttr外其他参数均需赋值
 * @param   pstImageDst[IN/OUT] 目标图像内存，
 *                         [IN] 当pstDstCropPrm非NULL时，除stMbAttr外其他参数均需赋值，反之仅需赋值enImgFmt、u32Stride和pPlaneVir
 *                        [OUT] 图像平面pPlaneVir内存填充图像数据
 * @param   pstSrcCropPrm[IN] 源图像抠取范围，非NULL时该区域必须在pstImageSrc指定的图像（Width×Height）内，为NULL时抠取全图
 * @param   pstDstCropPrm[IN] 目标内存中存放图像位置，
 *                            非NULL时，宽高必须与pstSrcCropPrm中宽高相同，且该区域必须在pstImageDst指定的图像（Width×Height）内，
 *                            为NULL时，则直接从源图全图或抠图拷贝到目的图像内存
 * @param   enFlipMode[IN] 图像镜像方向
 * 
 * @return  SAL_STATUS SAL_FAIL：失败，SAL_SOK：成功
 */
SAL_STATUS ximg_copy_nb(XIMAGE_DATA_ST *pstImageSrc, XIMAGE_DATA_ST *pstImageDst, SAL_VideoCrop *pstSrcCropPrm, SAL_VideoCrop *pstDstCropPrm, XIMAGE_FLIP_MODE enFlipMode);

/**
 * @function       ximg_resize
 * @brief          将原图图像宽高缩放到目标图像指定大小，缩放目标图像宽高最低为64（低于64时自动调整至64），支持缩放图像大小为64x64到8192x8192，yuv格式仅支持缩放连续内存内存图像
 * @param[IN]      XIMAGE_DATA_ST *pstImageSrc   源图像数据，u32Stride需等于u32Width
 * @param[IN]           enImgFmt  图像格式，见枚举DSP_IMG_DATFMT
 * @param[IN]           u32Width  源图像宽，64对齐，单位：Pixel
 * @param[IN]           u32Height 源图像高，2对齐，单位：Pixel
 * @param[OUT]          u32Stride 内存跨距，需等于u32Width，单位：Pixel
 * @param[OUT]          pPlaneVir 需要缩放的图像数据
 * @param[IN]           stMbAttr  内存信息
 * @param[IN/OUT]  XIMAGE_DATA_ST *pstImageDst   目标图像，由外部通过ximg_create(_sub)接口创建空白图像
 * @param[IN]           enImgFmt  图像格式，见枚举DSP_IMG_DATFMT
 * @param[IN]           u32Width  目标图像宽，2对齐，单位：Pixel
 * @param[IN]           u32Height 目标图像高，2对齐，单位：Pixel
 * @param[IN]           u32Stride 内存跨距，单位：Pixel
 * @param[OUT]          pPlaneVir 缩放后的图像数据
 * @param[IN]           stMbAttr  内存信息
 * @return         SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_resize(XIMAGE_DATA_ST *pstImageSrc, XIMAGE_DATA_ST *pstImageDst);

/**
 * @function   ximg_yc_stretch
 * @brief      YUV值域转换，“YC伸张”，将像素值范围由16~235转换为0~255，就地处理，最大支持尺寸：2048x1280
 *             硬件处理：  是否支持横向跨距    跨距对齐
 *                             V             16
 * @param[IN/OUT]  XIMAGE_DATA_ST *pstSrcImg    源数据，处理完成后为值域拉伸后的图像数据，硬件处理需跨距16对齐
 * @return         SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_yc_stretch(XIMAGE_DATA_ST *pstSrcImg);

/**
 * @function   ximg_convert_fmt
 * @brief      原始图像格式转换，先尝试硬件方法转换，硬件方法失败后会使用软件方法转换
 * @param[IN]  XIMAGE_DATA_ST  *pstImageSrc    输入原始图片数据
 * @param[IN]  XIMAGE_DATA_ST  *pstImageDst    输出图片数据
 * @return     SAL_STATUS SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_convert_fmt(XIMAGE_DATA_ST *pstImageSrc, XIMAGE_DATA_ST *pstImageDst);

/**
 * @function      ximg_save
 * @brief         将原始图像数据转化为图片格式，图像尺寸不满足编码要求时可根据在一定条件下自动修改图像尺寸
 *                编码格式要求：
 *                     源数据格式 目标格式  是否支持横向跨距    跨距对齐    宽度对齐
 *                                      (跨距大于图像宽度）
*                                 BMP         V              2          1
*                        RGB      JPG         V              64         1
*                                 GIF         X              2          1
* 
*                                 BMP         V              2          2
*                        YUV      JPG         X              16         16
*                                 GIF         X              2          2
 * @param[IN]     XIMAGE_DATA_ST  *pstImageData   输入原始图片数据，注意！！！YUV格式的0-235数据会转化成full_range的与显示保持一致！！！
 * @param[IN]     XRAY_TRANS_TYPE *enSaveType     目标图片格式
 * @param[OUT]    void    * const pOutBuf         目标图片存储内存
 * @param[IN/OUT] UINT32  * const pu32OutDataSize 目标图片尺寸,gif格式时输入gif图像内存尺寸，输出gif图像实际尺寸
 * @param[IN]     UINT32          u32FillColor    自动宽度填充部分的颜色值，按照默认yuv/rgb颜色值格式
 * @return        SAL_STATUS SAL_FAIL 失败 SAL_SOK 成功
 */
SAL_STATUS ximg_save(XIMAGE_DATA_ST *pstImageData, XRAY_TRANS_TYPE enSaveType, void * const pOutBuf, UINT32 * const pu32OutDataSize, UINT32 u32FillColor);

/**
 * @function   ximg_saveAs
 * @brief      将原始图像数据存储为图片格式，调试中
 * @param[IN]  XIMAGE_DATA_ST  *pstImageData   输入原始图片数据
 * @param[IN]  XRAY_TRANS_TYPE *enSaveType     目标图片格式
 * @param[OUT] void    * const pOutBuf         目标图片存储内存
 * @param[OUT] UINT32  * const pu32OutDataSize 目标图片尺寸
 * @return     SAL_STATUS SAL_FAIL 失败 SAL_SOK 成功
 */
SAL_STATUS ximg_saveAs(XIMAGE_DATA_ST *pstImageData, XRAY_TRANS_TYPE enSaveType, UINT32 chan, CHAR *dumpDir, CHAR *sTag, UINT32 u32NoTag);

/**
 * @function    ximg_dump
 * @brief       将图像数据保存成文件，文件名根据自定义的tag标签，通道号chan，dump数，图像宽高、格式组成，
 *              暂仅支持以下格式：DSP_IMG_DATFMT_BGRA32，DSP_IMG_DATFMT_BGR24，DSP_IMG_DATFMT_YUV420SP_VU，
 *                             DSP_IMG_DATFMT_SP16，DSP_IMG_DATFMT_LHP，DSP_IMG_DATFMT_LHZP
 * @param[IN]   XIMAGE_DATA_ST *pstImageData 需要保存的图像
 * @param[IN]   UINT32        chan          通道号，仅用于生成文件名
 * @param[IN]   CHAR          *dumpDir      文件保存路径
 * @param[IN]   CHAR          *sTagPre      自定义文件名前缀标签，仅用于生成文件名(可选)
 * @param[IN]   CHAR          *sTagPost     自定义文件名后缀标签，仅用于生成文件名(可选)
 * @param[IN]   UINT32        u32NoTag      数字标签，可为条带号，处理次数，dump次数
 * @param[OUT]  None
 * @return      SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS ximg_dump(XIMAGE_DATA_ST *pstImageData, UINT32 chan, CHAR *dumpDir, CHAR *sTagPre, CHAR *sTagPost, UINT32 u32NoTag);

/**
 * @function    ximg_print
 * @brief       打印图像信息
 * @param[IN]   XIMAGE_DATA_ST *pstImageData 需要保存的图像
 * @param[IN]   CHAR           *sTag         用于打印的字符串标签，可为NULL
 * @param[OUT]  None
 * @return      SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS ximg_print(XIMAGE_DATA_ST *pstImage, CHAR *sTag);

#endif
