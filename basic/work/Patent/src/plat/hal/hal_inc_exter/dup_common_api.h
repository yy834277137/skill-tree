/**
 * @file   dup_hal_inter.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#ifndef _DUP_COMMON_H_

#define _DUP_COMMON_H_

#include "sal.h"

typedef enum
{
    DISABLE = 0,
    ENABLE = 1,
} CHN_SWITH_E;

typedef enum
{
    MIN_CFG = 0,
    IMAGE_SIZE_CFG = 1,
    IMAGE_SIZE_EXT_CFG, /* 扩展通道 */
    IMAGE_VALID_RECT_CFG,   /* 图像有效区域配置，安检机双输入单屏时，xsp过来的数据是1920x448，如果只做缩放，会被拉伸 */
    PIX_FORMAT_CFG,
    MIRROR_CFG,
    FPS_CFG,
    ROTATE_CFG,
    CHN_CROP_CFG,
    GRP_CROP_CFG,
    GRP_SIZE_IN_CFG, /* 配置vpe的输入宽高信息，nt98336里需要与前级一致，比如1080P分辨率时vdec出来是1920x1088 */
    VPSS_CHN_MODE,
    SCALING_ALGORITHM_CFG,  /* 缩放算法配置 */
    MAX_CFG,
} CFG_TYPE_E;

typedef enum
{
    CROP_ORIGN = 0, /* 安检机里从[1920x4,1080]扣出原始图像[1920,1080]，NT98336平台需要使用裁剪功能，hi3559a通过stride实现 */
    CROP_SECOND,    /* 全局放大和局部放大时，NT98336在裁剪得到的原始图像[1920,1080]的基础上做全局/局部放大时[x,y]偏移*/
} CROP_TYPE_E;

typedef struct
{
    UINT32 u32X;
    UINT32 u32Y;
    UINT32 u32W;
    UINT32 u32H;
} RECT_AREA_S;


typedef struct
{
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Format;   /*NT平台需要设置输入输出格式*/
} IMAGE_SIZE_S;


typedef struct
{
    RECT_AREA_S stRect;
    IMAGE_SIZE_S stBg;
} IMAGE_VALID_RECT_S;

typedef struct
{
    UINT32 u32Mirror;
    UINT32 u32Flip;
} MIRROR_S;

typedef struct
{
    UINT32 u32CropEnable;
    UINT32 u32X;
    UINT32 u32Y;
    UINT32 u32W;
    UINT32 u32H;
    CROP_TYPE_E enCropType;
    IMAGE_SIZE_S sInImageSize; /* NT98336，安检机里从[1920x4,1080]扣出原始图像[1920,1080]，需要把总的分辨率[1920x4,1080]传入vpe */
} CROP_S;

typedef enum
{
    VPSS_MODE_USER  = 0,           /* User mode. */
    VPSS_MODE_AUTO  = 1,           /* Auto mode. */
    VPSS_MODE_PASSTHROUGH = 2,     /* Pass through mode */
    VPSS_MODE_BUTT,
} VPSS_MODE_E;

typedef struct 
{
    CFG_TYPE_E enType;
    union
    {
        IMAGE_SIZE_S stImgSize;
        IMAGE_VALID_RECT_S stImgValidRect;
        SAL_VideoDataFormat enPixFormat;
        MIRROR_S stMirror;
        UINT32 u32Fps;
        UINT32 u32Rotate;
        CROP_S stCrop;
        VPSS_MODE_E enVpssMode;
        BOOL bNewScalingAlgorithm;
    };
} PARAM_INFO_S;

#endif /* _DUP_COMMON_ */


