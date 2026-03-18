/**
 * @file   ximg_proc.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  图像处理公共接口，统一安检机各种类型的数据，各种类型的数据均可通过此接口统一表示和处理，持续优化
 * @date   2022/03/04
 * @note
 * @note
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "plat_com_inter_hal.h"
#include "jpeg_drv_api.h"
#include "yuv2gif.h"
#include "ximg_proc.h"

#line __LINE__ "ximg_proc.c"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define CHECK_PTR_IS_NULL(ptr, errno) \
    do { \
        if (!ptr) \
        { \
            XSP_LOGE("the '%s' is NULL\n", # ptr); \
            return (errno); \
        } \
    } while (0)

#define JPEG_ENC_QUALITY    (99)        /* JPEG编码质量，默认为最高的99 */
#define JPEG_HANDLE_MAX     (4)         /* 支持同时jpeg编码的通道个数 */

/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */

/*
 * jpeg编码通道控制结构体
 */
typedef struct 
{
    void *pJpegHandle;              // jpeg编码句柄
    BOOL bHandleInUse;              // jpeg编码通道是否正在被使用
    SYSTEM_FRAME_INFO stJpegSysFrm; // jpeg编码帧
} XIMG_JPEG_CTRL;

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */


/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/**
 * @function   ximg_get_bpp
 * @brief      获取不同格式图像每个像素点占用内存的字节数
 * @param[IN]  DSP_IMG_DATFMT format       图像格式
 * @param[IN]  INT32          u32PlaneIdx  多平面存储图像的图像平面索引
 * @return     SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
static INT32 ximg_get_bpp(DSP_IMG_DATFMT format, INT32 u32PlaneIdx)
{
    INT32 bpp = 0;
    switch (format)
    {
    case DSP_IMG_DATFMT_YUV420SP_UV:
    case DSP_IMG_DATFMT_YUV420SP_VU:
        bpp = (1 == u32PlaneIdx) ? 2 : 1; /* UV平面按单像素占两字节计算，宽高均为Y平面一半 */
        break;
    case DSP_IMG_DATFMT_BGRP:
    case DSP_IMG_DATFMT_BGRAP:
    case DSP_IMG_DATFMT_SP8:
        bpp = 1;
        break;
    case DSP_IMG_DATFMT_BGR24:
        bpp = 3;
        break;
    case DSP_IMG_DATFMT_BGRA32:
        bpp = 4;
        break;
    case DSP_IMG_DATFMT_SP16:
    case DSP_IMG_DATFMT_LHP:
    case DSP_IMG_DATFMT_LHZP:
        bpp = (2 == u32PlaneIdx) ? 1 : 2;   /* Z平面按单像素占单字节 */
        break;
    case DSP_IMG_DATFMT_LHZ16P:
        bpp = 2;
        break;
    default:
        SAL_LOGE("Unsupported image format 0x%x, plane %d\n", format, u32PlaneIdx);
        bpp = -1;
    }

    return bpp;
}

/**
 * @function   ximg_get_size
 * @brief      获取不同格式图像数据尺寸
 * @param[IN]  XIMAGE_DATA_ST *pstImg        输入图像
 * @param[OUT] UINT32         *pu32DataSize  输出的图像数据尺寸
 * @return     SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_get_size(XIMAGE_DATA_ST *pstImg, UINT32 *pu32DataSize)
{
    UINT32 u32ImgHeight = 0;
    UINT32 u32ImgStride = 0;
    DSP_IMG_DATFMT enDataFmt = DSP_IMG_DATFMT_UNKNOWN;

    CHECK_PTR_IS_NULL(pstImg, SAL_FAIL);
    CHECK_PTR_IS_NULL(pu32DataSize, SAL_FAIL);

    enDataFmt = pstImg->enImgFmt;
    u32ImgHeight = pstImg->u32Height;
    u32ImgStride = pstImg->u32Stride[0];

    switch (enDataFmt)
    {
    case DSP_IMG_DATFMT_YUV420SP_UV:
    case DSP_IMG_DATFMT_YUV420SP_VU:
    case DSP_IMG_DATFMT_YUV420P:
        *pu32DataSize = u32ImgStride * u32ImgHeight * 3 / 2;
        break;
    case DSP_IMG_DATFMT_YUV422_MASK:
        *pu32DataSize = u32ImgStride * u32ImgHeight * 2;
        break;
    case DSP_IMG_DATFMT_BGRA16_1555:
        *pu32DataSize = u32ImgStride * u32ImgHeight * 2;
        break;
    case DSP_IMG_DATFMT_BGRP:
    case DSP_IMG_DATFMT_BGR24:
        *pu32DataSize = u32ImgStride * u32ImgHeight * 3;
        break;
    case DSP_IMG_DATFMT_BGRA32:
    case DSP_IMG_DATFMT_BGRAP:
        *pu32DataSize = u32ImgStride * u32ImgHeight * 4;
        break;
    case DSP_IMG_DATFMT_SP16:
        *pu32DataSize = u32ImgStride * u32ImgHeight * 2;
        break;
    case DSP_IMG_DATFMT_SP8:
        *pu32DataSize = u32ImgStride * u32ImgHeight;
        break;
    case DSP_IMG_DATFMT_LHP:
        *pu32DataSize = u32ImgStride * u32ImgHeight * 4;
        break;
    case DSP_IMG_DATFMT_LHZP:
        *pu32DataSize = u32ImgStride * u32ImgHeight * 5;
        break;
    case DSP_IMG_DATFMT_LHZ16P:
        *pu32DataSize = u32ImgStride * u32ImgHeight * 6;
        break;
    default:
        *pu32DataSize = 0;
        SAL_LOGE("Not supported image format[0x%x]\n", enDataFmt);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   ximg_get_height
 * @brief      获取不同格式图像数据高度(yuv格式返回y平面高)
 * @param[IN]  XIMAGE_DATA_ST *pstImg   输入图像
 * @return     图像数据高度,-1表示失败
 */
INT32 ximg_get_height(XIMAGE_DATA_ST *pstImg)
{
    UINT32 u32ImgSize = 0;
    UINT32 u32ImgHeight = 0;
    UINT32 u32ImgStride = 0;
    DSP_IMG_DATFMT enDataFmt = DSP_IMG_DATFMT_UNKNOWN;

    CHECK_PTR_IS_NULL(pstImg, SAL_FAIL);

    enDataFmt = pstImg->enImgFmt;
    u32ImgSize = pstImg->stMbAttr.memSize;
    u32ImgStride = pstImg->u32Stride[0];
    if (0 == u32ImgStride)
    {
        return SAL_FAIL;
    }

    switch (enDataFmt)
    {
    case DSP_IMG_DATFMT_YUV420SP_UV:
    case DSP_IMG_DATFMT_YUV420SP_VU:
    case DSP_IMG_DATFMT_YUV420P:
        u32ImgHeight = u32ImgSize / (u32ImgStride * 3 / 2);
        break;
    case DSP_IMG_DATFMT_YUV422_MASK:
        u32ImgHeight = u32ImgSize / (u32ImgStride * 2);
        break;
    case DSP_IMG_DATFMT_BGRA16_1555:
        u32ImgHeight = u32ImgSize / (u32ImgStride * 2);
        break;
    case DSP_IMG_DATFMT_BGRP:
    case DSP_IMG_DATFMT_BGR24:
        u32ImgHeight = u32ImgSize / (u32ImgStride * 3);
        break;
    case DSP_IMG_DATFMT_BGRA32:
    case DSP_IMG_DATFMT_BGRAP:
        u32ImgHeight = u32ImgSize / (u32ImgStride * 4);
        break;
    case DSP_IMG_DATFMT_SP16:
        u32ImgHeight = u32ImgSize / (u32ImgStride * 2);
        break;
    case DSP_IMG_DATFMT_SP8:
        u32ImgHeight = u32ImgSize / u32ImgStride;
        break;
    case DSP_IMG_DATFMT_LHP:
        u32ImgHeight = u32ImgSize / (u32ImgStride * 4);
        break;
    case DSP_IMG_DATFMT_LHZP:
        u32ImgHeight = u32ImgSize / (u32ImgStride * 5);
        break;
    case DSP_IMG_DATFMT_LHZ16P:
        u32ImgHeight = u32ImgSize / (u32ImgStride * 6);
        break;
    default:
        u32ImgHeight = -1;
        SAL_LOGE("Not supported image format[0x%x]\n", enDataFmt);
        return SAL_FAIL;
    }

    return u32ImgHeight;
}

/**
 * @function   ximg_verify_size
 * @brief      校验图片宽高跨距是否合适，校验图片内存，图片宽高和格式所需内存不应超过实际buf最大内存
 * @param[IN]  XIMAGE_DATA_ST *pstImg 校验的图像
 * @param[IN]  const CHAR     *_func_ 调用处函数名，用于异常信息打印
 * @param[IN]  const UINT32   _line_  调用处行号，用于异常信息打印
 * @return     SAL_STATUS  SAL_FAIL 校验不通过，图像超出内存范围 SAL_SOK 校验通过
 */
static SAL_STATUS ximg_verify_size(XIMAGE_DATA_ST *pstImg, const CHAR *_func_, const UINT32 _line_)
{
    DSP_IMG_DATFMT enDataFmt = DSP_IMG_DATFMT_UNKNOWN;
    UINT32 u32ImgWidth = 0;
    UINT32 u32ImgHeight = 0;
    UINT32 u32ImgStride = 0;
    UINT32 u32ImgSize = 0;

    CHECK_PTR_IS_NULL(pstImg, SAL_FAIL);
    CHECK_PTR_IS_NULL(_func_, SAL_FAIL);
    enDataFmt = pstImg->enImgFmt;
    u32ImgWidth = pstImg->u32Width;
    u32ImgHeight = pstImg->u32Height;
    u32ImgStride = pstImg->u32Stride[0];

    if (0 == u32ImgWidth || 0 ==  u32ImgHeight || 0 ==  u32ImgStride || u32ImgWidth > u32ImgStride)
    {
        SAL_LOGE("ximg size is invalid, w:%u, h:%u, s:%u, fmt:0x%x, Func:%s|%u\n",  
                 u32ImgWidth, u32ImgHeight, u32ImgStride, enDataFmt, _func_, _line_);
        return SAL_FAIL;
    }
    if (enDataFmt & DSP_IMG_DATFMT_YUV420_MASK)
    {
        if (0 != (u32ImgWidth % 2) || 0 != (u32ImgHeight % 2) || 0 != (u32ImgStride % 2))
        {
            SAL_LOGE("ximg dimension is invalid(need alinged to 2), w:%u, h:%u, s:%u, fmt:0x%x, Func:%s|%u\n",  
                     u32ImgWidth, u32ImgHeight, u32ImgStride, enDataFmt, _func_, _line_);
            return SAL_FAIL;
        }
    }

    if (SAL_SOK != ximg_get_size(pstImg, &u32ImgSize))
    {
        SAL_LOGE("ximg_get_size failed, fmt[0x%x], Func:%s|%u\n", enDataFmt, _func_, _line_);
        return SAL_FAIL;
    }

    if (u32ImgSize > pstImg->stMbAttr.memSize)
    {
        SAL_LOGE("ximg size is inappropriate, w:%u, h:%u, s:%u, need_size:%u > buf_size:%u, fmt:0x%x, Func:%s|%u\n", 
                 u32ImgWidth, u32ImgHeight, u32ImgStride, 
                 u32ImgSize, pstImg->stMbAttr.memSize, enDataFmt, 
                 _func_, _line_);
        return SAL_FAIL;
    }
    
    return SAL_SOK;
}

/**
 * @function   ximg_cal_plane_vir
 * @brief      重新计算图像各平面首地址（根据图像图像宽距和高度从首平面依次向后排布）
 * @param[IN]  XIMAGE_DATA_ST *pstImg      输入图像
 * @param[IN]  UINT32         u32ImgStride 期望的图像跨距
 * @return     SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
static SAL_STATUS ximg_cal_plane_vir(XIMAGE_DATA_ST *pstImage, UINT32 u32ImgStride)
{
    UINT32 u32ImgHeight = 0;
    DSP_IMG_DATFMT enImgFmt = DSP_IMG_DATFMT_UNKNOWN;

    CHECK_PTR_IS_NULL(pstImage, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstImage->pPlaneVir[0], SAL_FAIL);

    enImgFmt = pstImage->enImgFmt;
    u32ImgHeight = pstImage->u32Height;

    switch (enImgFmt)
    {
    case DSP_IMG_DATFMT_YUV420SP_UV:
    case DSP_IMG_DATFMT_YUV420SP_VU:
        pstImage->u32Stride[0] = u32ImgStride;
        pstImage->u32Stride[1] = u32ImgStride;
        pstImage->pPlaneVir[1] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight;
        pstImage->pPlaneVir[2] = NULL;
        pstImage->pPlaneVir[3] = NULL;
        break;

    case DSP_IMG_DATFMT_YUV420P:
        pstImage->u32Stride[0] = u32ImgStride;
        pstImage->u32Stride[1] = u32ImgStride / 2;
        pstImage->u32Stride[2] = u32ImgStride / 2;
        pstImage->pPlaneVir[1] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight;
        pstImage->pPlaneVir[2] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight * 5 / 4;
        pstImage->pPlaneVir[3] = NULL;
        break;

    case DSP_IMG_DATFMT_YUV422_MASK:
        pstImage->u32Stride[0] = u32ImgStride;
        pstImage->u32Stride[1] = u32ImgStride;
        pstImage->u32Stride[2] = u32ImgStride;
        pstImage->pPlaneVir[1] = NULL;
        pstImage->pPlaneVir[2] = NULL;
        pstImage->pPlaneVir[3] = NULL;
        break;

    case DSP_IMG_DATFMT_BGRA16_1555:
        pstImage->u32Stride[0] = u32ImgStride * 2;
        break;

    case DSP_IMG_DATFMT_BGRP:
        pstImage->u32Stride[0] = u32ImgStride;
        pstImage->u32Stride[1] = u32ImgStride;
        pstImage->u32Stride[2] = u32ImgStride;
        pstImage->pPlaneVir[1] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight;
        pstImage->pPlaneVir[2] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight * 2;
        pstImage->pPlaneVir[3] = NULL;
        break;

    case DSP_IMG_DATFMT_BGRAP:
        pstImage->u32Stride[0] = u32ImgStride;
        pstImage->u32Stride[1] = u32ImgStride;
        pstImage->u32Stride[2] = u32ImgStride;
        pstImage->u32Stride[3] = u32ImgStride;
        pstImage->pPlaneVir[1] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight;
        pstImage->pPlaneVir[2] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight * 2;
        pstImage->pPlaneVir[3] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight * 3;
        break;

    case DSP_IMG_DATFMT_BGR24:
    case DSP_IMG_DATFMT_BGRA32:
        pstImage->u32Stride[0] = u32ImgStride;
        pstImage->pPlaneVir[1] = NULL;
        pstImage->pPlaneVir[2] = NULL;
        pstImage->pPlaneVir[3] = NULL;
        break;

    case DSP_IMG_DATFMT_SP16:
    case DSP_IMG_DATFMT_SP8:
        pstImage->u32Stride[0] = u32ImgStride;
        pstImage->pPlaneVir[1] = NULL;
        pstImage->pPlaneVir[2] = NULL;
        pstImage->pPlaneVir[3] = NULL;
        break;
    
    case DSP_IMG_DATFMT_LHP:
        pstImage->u32Stride[0] = u32ImgStride;
        pstImage->u32Stride[1] = u32ImgStride;
        pstImage->pPlaneVir[1] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight * 2; // 高能数据偏移
        pstImage->pPlaneVir[2] = NULL;
        pstImage->pPlaneVir[3] = NULL;
        break;

    case DSP_IMG_DATFMT_LHZP:
    case DSP_IMG_DATFMT_LHZ16P:
        pstImage->u32Stride[0] = u32ImgStride; // 低能
        pstImage->u32Stride[1] = u32ImgStride; // 高能
        pstImage->u32Stride[2] = u32ImgStride; // 原子序数
        pstImage->pPlaneVir[1] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight * 2; // 高能数据偏移
        pstImage->pPlaneVir[2] = pstImage->pPlaneVir[0] + u32ImgStride * u32ImgHeight * 4; // 原子序数数据偏移
        pstImage->pPlaneVir[3] = NULL;
        break;

    default:
        SAL_LOGE("Not supported image format[0x%x]\n", enImgFmt);
        return SAL_FAIL;
    }

    return SAL_TRUE;
}

/**
 * @function       ximg_hard_copy
 * @brief          使用硬件模块进行数据拷贝
 * @param[IN]      XIMAGE_DATA_ST   *pstSrcImg      源图像数据
 * @param[IN/OUT]  XIMAGE_DATA_ST   *pstDstImg      目标图像数据
 * @param[OUT]     SAL_VideoCrop    *pstSrcCropPrm  源图像拷贝区域，不可为NULL
 * @param[OUT]     SAL_VideoCrop    *pstDstCropPrm  源图像拷贝区域，不可为NULL
 * @return         SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
static SAL_STATUS ximg_hard_copy(XIMAGE_DATA_ST *pstSrcImg, XIMAGE_DATA_ST *pstDstImg, SAL_VideoCrop *pstSrcCropPrm, SAL_VideoCrop *pstDstCropPrm)
{
    SAL_STATUS ret = SAL_SOK;
    UINT64 srcTmpVbBlk = 0, dstTmpVbBlk = 0, tmpVbBlk = 0;
    TDE_HAL_SURFACE stSrcSurface = {0}, stDstSurface = {0};
    TDE_HAL_RECT stSrcRect = {0}, stDstRect = {0};
    CAPB_PLATFORM *pstPlatform = NULL;

    CHECK_PTR_IS_NULL(pstSrcImg, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstDstImg, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstSrcCropPrm, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstDstCropPrm, SAL_FAIL);
    if (0 == pstSrcImg->stMbAttr.phyAddr || 0 == pstDstImg->stMbAttr.phyAddr)
    {
        SAL_LOGE("Physical address of source ximg[%llu] or dst ximg[%llu] is 0\n", pstSrcImg->stMbAttr.phyAddr, pstDstImg->stMbAttr.phyAddr);
        return SAL_FAIL;
    }

    // 源图像
    stSrcSurface.u32Width = pstSrcImg->u32Stride[0];
    stSrcSurface.u32Height = pstSrcImg->u32Height;
    stSrcSurface.u32Stride = pstSrcImg->u32Stride[0];
    stSrcSurface.PhyAddr = pstSrcImg->stMbAttr.phyAddr + (pstSrcImg->pPlaneVir[0] - pstSrcImg->stMbAttr.virAddr);
    stSrcSurface.virAddr = pstSrcImg->stMbAttr.virAddr;     // 未使用
    stSrcSurface.enColorFmt = SAL_dspDataFormatToSal(pstSrcImg->enImgFmt);
    stSrcSurface.vbBlk = pstSrcImg->stMbAttr.u64MbBlk;
    // 源图像的拷贝区域
    stSrcRect.s32Xpos = pstSrcCropPrm->left;
    stSrcRect.s32Ypos = pstSrcCropPrm->top;
    stSrcRect.u32Width = pstSrcCropPrm->width;
    stSrcRect.u32Height = pstSrcCropPrm->height;

    // 目标图像
    stDstSurface.u32Width = pstDstImg->u32Stride[0];
    stDstSurface.u32Height = pstDstImg->u32Height;
    stDstSurface.u32Stride = pstDstImg->u32Stride[0];
    stDstSurface.PhyAddr = pstDstImg->stMbAttr.phyAddr + (pstDstImg->pPlaneVir[0] - pstDstImg->stMbAttr.virAddr);
    stDstSurface.virAddr = pstDstImg->stMbAttr.virAddr;     // 未使用
    stDstSurface.enColorFmt = SAL_dspDataFormatToSal(pstDstImg->enImgFmt);
    stDstSurface.vbBlk = pstDstImg->stMbAttr.u64MbBlk;
    // 目标图像的拷贝区域
    stDstRect.s32Xpos = pstDstCropPrm->left;
    stDstRect.s32Ypos = pstDstCropPrm->top;
    stDstRect.u32Width = pstDstCropPrm->width;
    stDstRect.u32Height = pstDstCropPrm->height;

    /* rk3588 tde拷贝时需要对xpack下发的坐标参数转换为以mb起始点为原点的坐标 */
    pstPlatform = capb_get_platform();
    if (pstPlatform->bTdeCoordiateTrans)
    {
        if(0 == stSrcSurface.vbBlk || 0 == stDstSurface.vbBlk)
        {
            //RK硬拷贝MB必须有效
            SAL_LOGW("invalid vbBlk %lld %lld\n",stSrcSurface.vbBlk,stDstSurface.vbBlk);
            return SAL_FALSE;
        }
        if (!(stSrcSurface.virAddr) || !(stDstSurface.virAddr))
        {
            SAL_LOGW("virAddr is NULL, pstSrc->virAddr %p, pstDst->virAddr %p \n", stSrcSurface.virAddr, stDstSurface.virAddr);
            return SAL_FALSE;
        }
        
        /* 坐标偏移 */
        VOID *srcMbVir = NULL;
        VOID *dstMbVir = NULL;
        UINT64 u64SrcSize = 0;
        UINT64 u64DstSize = 0;
        UINT32 bpp = 0;
        mem_hal_vbGetVirAddr(stSrcSurface.vbBlk, &srcMbVir);
        SAL_halGetPixelBpp(stSrcSurface.enColorFmt, &bpp);
        /* TDE拷贝时上层下发的是源mb拷贝区域rect的起始虚拟地址和src rect X/Y为0，RK根据此和mb的起始虚拟地址算出rect X/Y坐标;
            (上层最好改为下发mb的起始地址和src rect X/Y，但改动地方较多所以只能在RK plat里转换)*/
        if (stSrcSurface.virAddr - srcMbVir)
        {
            /* 把以pstSrc->virAddr为原点的[X,Y]转换为以整个mb起始点为原点的[X,Y]*/
            stSrcRect.s32Xpos = ((stSrcSurface.virAddr - srcMbVir) % (bpp * stSrcSurface.u32Stride)) / bpp;
            stSrcRect.s32Ypos = (stSrcSurface.virAddr - srcMbVir) / (bpp * stSrcSurface.u32Stride);
        }
        /* pstSrc->virAddr与mb起始虚拟地址即使相等时也需要重新计算mb的宽高 */
        stSrcSurface.u32Width = stSrcSurface.u32Stride;
        mem_hal_vbGetSize(stSrcSurface.vbBlk,&u64SrcSize);
        stSrcSurface.u32Height = u64SrcSize / (stSrcSurface.u32Stride * SAL_getBitsPerPixel(stSrcSurface.enColorFmt)/8 ); /* 如果把这一套RK TDE参数转换逻辑放到rk_tde.c里，那么分析仪智能模块tde拷贝时高度计算就有问题，因为智能源数据是从vpss获取，mb size 是1920x1088x3*/

        mem_hal_vbGetVirAddr(stDstSurface.vbBlk, &dstMbVir);
        SAL_halGetPixelBpp(stDstSurface.enColorFmt, &bpp);
        if (stDstSurface.virAddr - dstMbVir)
        {
            stDstRect.s32Xpos = ((stDstSurface.virAddr - dstMbVir) % (bpp * stDstSurface.u32Stride)) / bpp;
            stDstRect.s32Ypos = (stDstSurface.virAddr - dstMbVir) / (bpp * stDstSurface.u32Stride);
        }
        stDstSurface.u32Width = stDstSurface.u32Stride;
        mem_hal_vbGetSize(stDstSurface.vbBlk,&u64DstSize);
        //stDstSurface.u32Height = u64DstSize / (stDstSurface.u32Stride * SAL_getBitsPerPixel(stSrcSurface.enColorFmt)/8); /* 用SAL_getBitsPerPixel主要是为了兼容YUV420*/
        stDstSurface.u32Height = stDstRect.s32Ypos + stDstRect.u32Height; /* 高度不能通过缓冲区大小除以宽度获取，因为有的包裹宽度很小，导致高度值很大SDK底层就会报错 */
        SAL_LOGD("srvVir %p, srcMbVir %p, srcMb %llx, srcFormat %d, srcStride %d, h %d, srcSize %llu; dstVir %p, dstMbVir %p, dstMb %llx, dstFormat %d, dstStride %d, h %d, stSize %llu;", \
                 stSrcSurface.virAddr, srcMbVir, stSrcSurface.vbBlk, stSrcSurface.enColorFmt, stSrcSurface.u32Stride, stSrcSurface.u32Height, u64SrcSize,\
                 stDstSurface.virAddr, dstMbVir, stDstSurface.vbBlk, stDstSurface.enColorFmt, stDstSurface.u32Stride, stDstSurface.u32Height, u64DstSize);
        
        /* 安检机xpack里显示大缓冲区里从外部拷贝到头部时，由于mb的offset一直在变，所以需要create一个新的mb，新的mb offset为0，但存放图像数据内存还是指向原来的内存 */
        /* 创建无偏移的vb */
        ret = mem_hal_vbMap(stSrcSurface.vbBlk, &tmpVbBlk);
        if (ret != SAL_SOK)
        {
            SAL_LOGE("mem_hal_vbMap failed\n ");
            return SAL_FALSE;
        }
        srcTmpVbBlk = stSrcSurface.vbBlk;
        stSrcSurface.vbBlk = tmpVbBlk;
        
        /* 创建无偏移的vb */
        ret = mem_hal_vbMap(stDstSurface.vbBlk, &tmpVbBlk);
        if (ret != SAL_SOK)
        {
            SAL_LOGE("mem_hal_vbMap failed\n ");
            goto deleteVb;
        }
        dstTmpVbBlk = stDstSurface.vbBlk;
        stDstSurface.vbBlk = tmpVbBlk;
    }

    switch (stSrcSurface.enColorFmt)
    {
        case SAL_VIDEO_DATFMT_ARGB_8888:
        case SAL_VIDEO_DATFMT_ARGB16_1555:
        case SAL_VIDEO_DATFMT_RGB24_888:
        case SAL_VIDEO_DATFMT_BAYER_RAW_DUAL:
        case SAL_VIDEO_DATFMT_BAYER_RAW_SINGLE:
        case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        case SAL_VIDEO_DATFMT_YUV420P:
        {
            ret = tde_hal_QuickCopy(&stSrcSurface, &stSrcRect, &stDstSurface, &stDstRect, SAL_FALSE);
            break;
        }
        default:
        {
            SAL_LOGE("Not supported tde copy enSalPixFmt 0x%x\n ", stSrcSurface.enColorFmt);
            break;
        }
    }

    if (ret != SAL_SOK)
    {
        SAL_LOGE("tde copy failed enSalPixFmt 0x%x, src[%u x %u] dst[%u x %u]\n ", 
                    stSrcSurface.enColorFmt, stSrcSurface.u32Width, stSrcSurface.u32Height, stDstSurface.u32Width, stDstSurface.u32Height);
    }

deleteVb:
    if(0 != srcTmpVbBlk)
    {
        mem_hal_vbDelete(stSrcSurface.vbBlk);
        stSrcSurface.vbBlk = srcTmpVbBlk;
    }

    if(0 != dstTmpVbBlk)
    {
        mem_hal_vbDelete(stDstSurface.vbBlk);
        stDstSurface.vbBlk = dstTmpVbBlk;
    }

    return ret;
}

/**
 * @function       ximg_copy_kernel
 * @brief          将源图像拷贝一份给目标图像，不做任何修改，不支持就地处理
 * @param[IN]      XIMAGE_DATA_ST   *pstSrcImg  源图像数据
 * @param[IN/OUT]  XIMAGE_DATA_ST   *pstDstImg  目标图像数据
 * @return         SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
static SAL_STATUS ximg_copy_kernel(XIMAGE_DATA_ST *pstSrcImg, XIMAGE_DATA_ST *pstDstImg)
{
    INT32 i = 0, j = 0, bpp = 1;
    UINT32 u32SrcWidth = 0, u32SrcHeight = 0, u32SrcStride = 0;        // 源数据宽、高、跨距，单位：像素
    UINT32 u32DstStride = 0;        // 目标数据跨距，单位：像素
    UINT32 sstep = 0;               // 源数据行步长，单位：字节
    UINT32 dstep = 0;               // 目标数据行步长，单位：字节
    UINT32 u32LineDataCpyByte = 0;      // 按行拷贝字节数
    UINT32 u32AllDataCpyByte = 0;       // 一次性拷贝字节数
    VOID *pSrcDataVir = NULL;
    VOID *pDstDataVir = NULL;

    CHECK_PTR_IS_NULL(pstSrcImg, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstDstImg, SAL_FAIL);
    if (pstSrcImg->enImgFmt != pstDstImg->enImgFmt)
    {
        SAL_LOGE("Only support ximg copy of same ximg format, src:0x%x, dst:0x%x\n", pstSrcImg->enImgFmt, pstDstImg->enImgFmt);
        return SAL_FAIL;
    }
    if (pstSrcImg->u32Width != pstDstImg->u32Width || pstSrcImg->u32Height != pstDstImg->u32Height)
    {
        SAL_LOGE("Variable ximg dimension copy is not supported, src[%u x %u], dst[%u x %u]\n", 
                 pstSrcImg->u32Width, pstSrcImg->u32Height, pstDstImg->u32Width, pstDstImg->u32Height);
        return SAL_FAIL;
    }

    u32SrcWidth = pstSrcImg->u32Width;
    u32SrcHeight = pstSrcImg->u32Height;

    for (i = 0; i < XIMAGE_PLANE_MAX; i++)
    {
        /* 依次拷贝各个平面 */
        pSrcDataVir = pstSrcImg->pPlaneVir[i];
        pDstDataVir = pstDstImg->pPlaneVir[i];
        if (NULL == pSrcDataVir || NULL == pDstDataVir)
        {
            break;
        }

        u32SrcStride = pstSrcImg->u32Stride[i];
        u32DstStride = pstDstImg->u32Stride[i];
        bpp = ximg_get_bpp(pstSrcImg->enImgFmt, i);
        if (0 > bpp)
        {
            SAL_LOGE("ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d\n", pstSrcImg->enImgFmt, i, bpp);
            return SAL_FAIL;
        }
        /* UV平面按单像素占两字节计算，宽高均为Y平面一半 */
        if ((DSP_IMG_DATFMT_YUV420SP_VU == pstSrcImg->enImgFmt || DSP_IMG_DATFMT_YUV420SP_UV == pstSrcImg->enImgFmt) && (1 == i)) // 1表示UV平面
        {
            u32SrcWidth >>= 1;
            u32SrcHeight >>= 1;
            u32SrcStride >>= 1;
            u32DstStride >>= 1;
        }
        sstep = u32SrcStride * bpp;
        dstep = u32DstStride * bpp;
        u32LineDataCpyByte = u32SrcWidth * bpp;

        if (u32SrcWidth == u32SrcStride && u32SrcStride == u32DstStride)
        {
            /* SrcWidth、SrcStride、DstStride三者相同，数据连续，直接拷贝 */
            u32AllDataCpyByte = u32LineDataCpyByte * u32SrcHeight;
            memmove(pDstDataVir, pSrcDataVir, u32AllDataCpyByte);
        }
        else
        {
            /* 数据不连续时，按行分别拷贝 */
            for (j = 0; j < u32SrcHeight; j++)
            {
                memmove(pDstDataVir, pSrcDataVir, u32LineDataCpyByte);
                pSrcDataVir += sstep;
                pDstDataVir += dstep;
            }
        }
    }

    return SAL_SOK;
}

/**
 * @function   ximg_set_dimension
 * @brief      修改图像维度信息，重新计算图像各平面首地址（根据图像图像宽距和高度从首平面依次向后排布），仅修改跨距不修改内存排布时确保在对图像修改前进行，否则图像会错乱
 *             仅图像内存调整标志有效时改变相应改变图像内存排布以适应新的跨距，否则内存排布不变（暂仅支持跨距变小时调整内存排布）
 * @param[IN]  XIMAGE_DATA_ST *pstImg       输入图像
 * @param[IN]  UINT32         u32ImgWidth   期望的图像宽度，不得大于u32ImgStride
 * @param[IN]  UINT32         u32ImgStride  期望的图像跨距
 * @param[IN]  BOOL           bAdjustMemory 是否重新调整内存排布
 * @param[IN]  UINT32         u32Color      图像宽度增加时对增加部分预填充的图像颜色
 * @return     SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_set_dimension(XIMAGE_DATA_ST *pstImage, UINT32 u32ImgWidth, UINT32 u32ImgStride, BOOL bAdjustMemory, UINT32 u32Color)
{
    UINT32 u32ImgOriWidth = 0, u32ImgHeight = 0, u32ImgOriStride = 0;
    DSP_IMG_DATFMT enImgFmt = DSP_IMG_DATFMT_UNKNOWN;
    XIMAGE_DATA_ST stVerifyImg = {0};
    XIMAGE_DATA_ST stDstImg = {0};

    CHECK_PTR_IS_NULL(pstImage, SAL_FAIL);
    ximg_create_sub(pstImage, &stVerifyImg, NULL);
    stVerifyImg.u32Width = u32ImgWidth;
    stVerifyImg.u32Stride[0] = u32ImgStride;
    if (SAL_SOK != ximg_verify_size(&stVerifyImg, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }

    enImgFmt = pstImage->enImgFmt;
    u32ImgHeight = pstImage->u32Height;
    u32ImgOriWidth = pstImage->u32Width;
    u32ImgOriStride = pstImage->u32Stride[0];

    pstImage->u32Width = u32ImgWidth;

    /* stride变小时，可调整内存排布 */
    if (u32ImgOriStride > u32ImgStride && SAL_TRUE == bAdjustMemory)
    {
        ximg_create(u32ImgWidth, u32ImgHeight, enImgFmt, NULL, pstImage->pPlaneVir[0], &stDstImg);
        ximg_cal_plane_vir(&stDstImg, u32ImgStride);
        ximg_copy_kernel(pstImage, &stDstImg);
    }
    ximg_cal_plane_vir(pstImage, u32ImgStride);

    if (u32ImgWidth > u32ImgOriWidth && 0 != u32ImgOriWidth)
    {
        ximg_fill_color(pstImage, 0, u32ImgHeight, u32ImgOriWidth, u32ImgWidth - u32ImgOriWidth, u32Color);
    }

    return SAL_SOK;
}

/**
 * @function   ximg_flip_vert
 * @brief      竖直方向镜像图像，支持就地处理，每次操作16字节数据以加速
 * @param[IN]  XIMAGE_DATA_ST *pstSrcImg 源图像数据
 * @param[IN]  XIMAGE_DATA_ST *pstDstImg 目标图像数据
 * @return     SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
static SAL_STATUS ximg_flip_vert(XIMAGE_DATA_ST *pstSrcImg, XIMAGE_DATA_ST *pstDstImg)
{
    INT32 i = 0, j = 0, bpp = 1;
    UINT32 u32SrcWidth = 0, u32SrcHeight = 0, u32SrcStride = 0;        // 源数据宽、高、跨距，单位：像素
    UINT32 u32DstStride = 0;        // 目标数据跨距，单位：像素
    UINT32 sstep = 0;   // 源数据行步长，单位：字节
    UINT32 dstep = 0;   // 目标数据行步长，单位：字节
    UINT32 u32SrcByteWidth = 0;
    const U08 *src0 = NULL;
    const U08 *src1 = NULL;
    U08 *dst0 = NULL;
    U08 *dst1 = NULL;

    CHECK_PTR_IS_NULL(pstSrcImg, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstDstImg, SAL_FAIL);
    if (SAL_SOK != ximg_verify_size(pstSrcImg, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }

    u32SrcWidth = pstSrcImg->u32Width;
    u32SrcHeight = pstSrcImg->u32Height;

    for (i = 0; i < XIMAGE_PLANE_MAX; i++)
    {
        src0 = pstSrcImg->pPlaneVir[i];
        dst0 = pstDstImg->pPlaneVir[i];

        if (NULL == src0 || NULL == dst0)
        {
            break;
        }
        
        u32SrcStride = pstSrcImg->u32Stride[i];
        u32DstStride = pstDstImg->u32Stride[i];
        bpp = ximg_get_bpp(pstSrcImg->enImgFmt, i);
        if (0 > bpp)
        {
            SAL_LOGE("ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d", pstSrcImg->enImgFmt, i, bpp);
            return SAL_FAIL;
        }
        /* UV平面按单像素占两字节计算，宽高均为Y平面一半 */
        if ((DSP_IMG_DATFMT_YUV420SP_VU == pstSrcImg->enImgFmt || DSP_IMG_DATFMT_YUV420SP_UV == pstSrcImg->enImgFmt) && 1 == i)
        {
            u32SrcWidth >>= 1;
            u32SrcHeight >>= 1;
            u32SrcStride >>= 1;
            u32DstStride >>= 1;
        }

        sstep = u32SrcStride * bpp;
        dstep = u32DstStride * bpp;
        src1 = src0 + (u32SrcHeight - 1) * sstep;
        dst1 = dst0 + (u32SrcHeight - 1) * dstep;

        for(int y = 0; y < (u32SrcHeight + 1) / 2; y++, src0 += sstep, src1 -= sstep,
                                                        dst0 += dstep, dst1 -= dstep)
        {
            j = 0;
            u32SrcByteWidth = u32SrcWidth * bpp;
            for( ; j <= u32SrcByteWidth - 16; j += 16)
            {
                int t0 = ((int*)(src0 + j))[0];
                int t1 = ((int*)(src1 + j))[0];

                ((int*)(dst0 + j))[0] = t1;
                ((int*)(dst1 + j))[0] = t0;

                t0 = ((int*)(src0 + j))[1];
                t1 = ((int*)(src1 + j))[1];

                ((int*)(dst0 + j))[1] = t1;
                ((int*)(dst1 + j))[1] = t0;

                t0 = ((int*)(src0 + j))[2];
                t1 = ((int*)(src1 + j))[2];

                ((int*)(dst0 + j))[2] = t1;
                ((int*)(dst1 + j))[2] = t0;

                t0 = ((int*)(src0 + j))[3];
                t1 = ((int*)(src1 + j))[3];

                ((int*)(dst0 + j))[3] = t1;
                ((int*)(dst1 + j))[3] = t0;
            }

            for( ; j <= u32SrcByteWidth - 4; j += 4 )
            {
                int t0 = ((int*)(src0 + j))[0];
                int t1 = ((int*)(src1 + j))[0];

                ((int*)(dst0 + j))[0] = t1;
                ((int*)(dst1 + j))[0] = t0;
            }

            for( ; j < u32SrcByteWidth; j++ )
            {
                unsigned char t0 = src0[j];
                unsigned char t1 = src1[j];

                dst0[j] = t1;
                dst1[j] = t0;
            }
        }
    }
    return SAL_SOK;
}

/**
 * @function   ximg_flip_horz
 * @brief      水平方向镜像图像，支持就地处理
 * @param[IN]  XIMAGE_DATA_ST *pstSrcImg 源图像数据
 * @param[IN]  XIMAGE_DATA_ST *pstDstImg 目标图像数据
 * @return     SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
static SAL_STATUS ximg_flip_horz(XIMAGE_DATA_ST *pstSrcImg, XIMAGE_DATA_ST *pstDstImg)
{
    INT32 i = 0, j = 0, k = 0, l = 0;
    INT32 bpp = 1;
    INT32 limit = 0;
    UINT32 u32SrcWidth = 0, u32SrcHeight = 0, u32SrcStride = 0;        // 源数据宽、高、跨距，单位：像素
    UINT32 u32DstStride = 0;         // 目标数据跨距，单位：像素
    UINT32 sstep = 0;
    UINT32 dstep = 0;
    const U08 *src = NULL;
    U08 *dst = NULL;
    U08 cData0 = 0, cData1 = 0;
    short tab[7680] = {0};

    CHECK_PTR_IS_NULL(pstSrcImg, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstDstImg, SAL_FAIL);
    if (SAL_SOK != ximg_verify_size(pstSrcImg, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }

    u32SrcWidth = pstSrcImg->u32Width;
    u32SrcHeight = pstSrcImg->u32Height;
    limit = (INT32)(((u32SrcWidth + 1) / 2) * 2);

    for (l = 0; l < XIMAGE_PLANE_MAX; l++)
    {
        src = pstSrcImg->pPlaneVir[l];
        dst = pstDstImg->pPlaneVir[l];

        if (NULL == src || NULL == dst)
        {
            break;
        }
        
        u32SrcStride = pstSrcImg->u32Stride[l];
        u32DstStride = pstDstImg->u32Stride[l];
        bpp = ximg_get_bpp(pstSrcImg->enImgFmt, l);
        if (0 > bpp)
        {
            SAL_LOGE("ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d", pstSrcImg->enImgFmt, l, bpp);
            return SAL_FAIL;
        }
        /* UV平面按单像素占两字节计算，宽高均为Y平面一半 */
        if ((DSP_IMG_DATFMT_YUV420SP_VU == pstSrcImg->enImgFmt || DSP_IMG_DATFMT_YUV420SP_UV == pstSrcImg->enImgFmt) && 1 == l)
        {
            u32SrcWidth >>= 1;
            u32SrcHeight >>= 1;
            u32SrcStride >>= 1;
            u32DstStride >>= 1;
        }

        sstep = u32SrcStride * bpp;
        dstep = u32DstStride * bpp;

        for(i = 0; i < u32SrcWidth; i++)
        {
            for(k = 0; k < bpp; k++ )
            {
                tab[i * bpp + k] = (short)((u32SrcWidth - i - 1) * bpp + k);
            }
        }

        for(i = 0; i < u32SrcHeight; i++, src += sstep, dst += dstep)
        {
            for(k = 0; k < limit; k++)
            {
                j = tab[k];
                cData0 = src[k], cData1 = src[j];
                dst[k] = cData1; dst[j] = cData0;
            }
        }
    }

    return SAL_SOK;
}

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
SAL_STATUS ximg_create(UINT32 u32ImgWidth, UINT32 u32ImgHeight, DSP_IMG_DATFMT enDataFmt, CHAR *szMemName, VOID *pDataVir, XIMAGE_DATA_ST *pstImageData)
{
    INT32 s32Ret = SAL_SOK;
    UINT64 u64PhysicalAddr = 0;
    ALLOC_VB_INFO_S stVbInfo = {0};
    MEDIA_BUF_ATTR *pstMbAttr = NULL;

    CHECK_PTR_IS_NULL(pstImageData, SAL_FAIL);

    pstMbAttr = &pstImageData->stMbAttr;
    if (0 == u32ImgWidth * u32ImgHeight)
    {
        XPACK_LOGE("memory size is 0, Width %u, Height %u, memSize %u\n", pstImageData->u32Width, pstImageData->u32Height, pstMbAttr->memSize);
        return SAL_FAIL;
    }

    pstImageData->u32Width = u32ImgWidth;
    pstImageData->u32Height = u32ImgHeight;
    pstImageData->u32Stride[0] = u32ImgWidth;
    pstImageData->enImgFmt = enDataFmt;
    ximg_get_size(pstImageData, &pstMbAttr->memSize);

    /* 若自定义内存为空，则内部申请内存，否则使用自定义内存 */
    if (NULL == pDataVir)
    {
        CHECK_PTR_IS_NULL(szMemName, SAL_FAIL);
        if (SAL_TRUE == pstMbAttr->bCached)
        {
            /* 非硬件访问内存 */
            s32Ret = mem_hal_mmzAlloc(pstMbAttr->memSize, "ximage", szMemName, NULL, SAL_TRUE, &u64PhysicalAddr, (VOID **)&pstImageData->pPlaneVir[0]);
        }
        else
        {
            /* 硬件访问内存 */
            s32Ret = mem_hal_vbAlloc(pstMbAttr->memSize, "ximage", szMemName, NULL, pstMbAttr->bCached, &stVbInfo);
        }
        if (SAL_SOK != s32Ret)
        {
            XPACK_LOGE("memory aclloc failed, w:%u, h:%u, size:%u fmt:0x%x, bCached:%d\n", 
                    u32ImgWidth, u32ImgHeight, pstMbAttr->memSize, pstImageData->enImgFmt, pstMbAttr->bCached);
            return SAL_FAIL;
        }

        if (SAL_FALSE == pstMbAttr->bCached)
        {
            pstMbAttr->poolId = stVbInfo.u32PoolId;
            pstMbAttr->phyAddr = stVbInfo.u64PhysAddr;
            pstMbAttr->u64MbBlk = stVbInfo.u64VbBlk;
            pstImageData->pPlaneVir[0] = stVbInfo.pVirAddr;
        }
        pstMbAttr->virAddr = pstImageData->pPlaneVir[0];
    }
    else
    {
        pstImageData->pPlaneVir[0] = pDataVir;
    }

    ximg_cal_plane_vir(pstImageData, u32ImgWidth);

    return s32Ret;
}

/**
 * @fn          ximg_create_sub
 * @brief       将原图中的一部分构造为一张子图像，仍使用原图内存
 * @param[IN]  XIMAGE_DATA_ST *pstSrcImg  源图像数据
 * @param[OUT] XIMAGE_DATA_ST *pstDstImg  目标图像数据
 * @param[IN]  SAL_VideoCrop  *pstCropPrm 子图像范围
 * @return      SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS ximg_create_sub(XIMAGE_DATA_ST *pstSrcImg, XIMAGE_DATA_ST *pstDstImg, SAL_VideoCrop *pstCropPrm)
{
    INT32 i = 0, bpp = 1;
    UINT32 u32SrcStride = 0;        // 源数据跨距，单位：像素
    UINT32 u32MemoryOffset = 0;
    SAL_VideoCrop stLocCropPrm = {0};

    CHECK_PTR_IS_NULL(pstSrcImg, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstDstImg, SAL_FAIL);

    if (NULL == pstCropPrm)
    {
        stLocCropPrm.top = 0;
        stLocCropPrm.left = 0;
        stLocCropPrm.width = pstSrcImg->u32Width;
        stLocCropPrm.height = pstSrcImg->u32Height;
    }
    else
    {
        memcpy(&stLocCropPrm, pstCropPrm, sizeof(SAL_VideoCrop));
    }

    /* 判断抠图区域是否超出图像范围 */
    if ((stLocCropPrm.top + stLocCropPrm.height) > pstSrcImg->u32Height || 
        (stLocCropPrm.left + stLocCropPrm.width) > pstSrcImg->u32Width)
    {
        SAL_LOGE("crop size exceeds image size, src[%u x %u], crop[%u %u %u %u].\n", 
                 pstSrcImg->u32Width, pstSrcImg->u32Height, 
                 stLocCropPrm.left, stLocCropPrm.top, stLocCropPrm.width, stLocCropPrm.height);
        return SAL_FAIL;
    }

    pstDstImg->enImgFmt = pstSrcImg->enImgFmt;
    pstDstImg->u32Width = stLocCropPrm.width;
    pstDstImg->u32Height = stLocCropPrm.height;
    /* media buf信息使用原图信息 */
    memcpy(&pstDstImg->stMbAttr, &pstSrcImg->stMbAttr, sizeof(MEDIA_BUF_ATTR));

    for (i = 0; i < XIMAGE_PLANE_MAX; i++)
    {
        if (NULL == pstSrcImg->pPlaneVir[i])
        {
            break;  //遇到空平面指针直接退出，不再处理后续平面
        }

        u32SrcStride = pstSrcImg->u32Stride[i];
        bpp = ximg_get_bpp(pstSrcImg->enImgFmt, i);
        if (0 > bpp)
        {
            SAL_LOGE("ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d\n", pstSrcImg->enImgFmt, i, bpp);
            return SAL_FAIL;
        }
        if ((DSP_IMG_DATFMT_YUV420SP_VU == pstSrcImg->enImgFmt || DSP_IMG_DATFMT_YUV420SP_UV == pstSrcImg->enImgFmt) && 1 == i)
        {
            u32SrcStride >>= 1;
            stLocCropPrm.top >>= 1;
            stLocCropPrm.left >>= 1;
        }
        u32MemoryOffset = (u32SrcStride * stLocCropPrm.top + stLocCropPrm.left) * bpp;

        pstDstImg->u32Stride[i] = pstSrcImg->u32Stride[i];
        pstDstImg->pPlaneVir[i] = pstSrcImg->pPlaneVir[i] + u32MemoryOffset;
    }

    return SAL_SOK;
}

/**
 * @fn      ximg_destroy
 * @brief   销毁一张图片，由图像接口内部申请的内存时回收内存
 * @param[IN]   XIMAGE_DATA_ST *pstImageData 输出的图片
 * @return      SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS ximg_destroy(XIMAGE_DATA_ST *pstImageData)
{
    MEDIA_BUF_ATTR *pstMbAttr = NULL;
    CHECK_PTR_IS_NULL(pstImageData, SAL_FAIL);

    pstMbAttr = &pstImageData->stMbAttr;
    if (NULL != pstMbAttr->virAddr)
    {
        if (SAL_TRUE == pstMbAttr->bCached)
        {
            mem_hal_mmzFree(pstImageData->pPlaneVir[0], "xsp", "xsp_drv");
        }
        else
        {
            if (SAL_SOK != mem_hal_vbFree(pstMbAttr->virAddr, "xpack", "xpack", pstMbAttr->memSize, pstMbAttr->u64MbBlk, pstMbAttr->poolId))
            {
                XPACK_LOGE("mem_hal_vbFree failed, virAddr %p, memSize %u\n", pstMbAttr->virAddr, pstMbAttr->memSize);
            }
        }

    }
    SAL_clear(pstImageData);

    return SAL_SOK;
}

/**
 * @fn        ximg_fill_color
 * @brief     对图像填充颜色，暂仅支持DSP_IMG_DATFMT_BGRA32，DSP_IMG_DATFMT_BGR24，DSP_IMG_DATFMT_YUV420SP_VU/UV，DSP_IMG_DATFMT_SP16格式
 * @param[IN] XIMAGE_DATA_ST *pstImageData 输入的图像   
 * @param[IN] UINT32         u32StartRow   开始填充行（含），范围为 0 ~ image height - 1
 * @param[IN] UINT32         u32FillHeight 填充的高度，单位：pixel
 * @param[IN] UINT32         u32StartCol   开始填充列（含），范围为 0 ~ image width - 1
 * @param[IN] UINT32         u32FillWidth  填充的宽度，单位：pixel
 * @param[IN] UINT32         u32BgValue    填充的颜色值
 * @return    SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS ximg_fill_color(XIMAGE_DATA_ST *pstImageData, UINT32 u32StartRow, UINT32 u32FillHeight, UINT32 u32StartCol, UINT32 u32FillWidth, UINT32 u32BgValue)
{
    UINT8 u8YValue = 0, u8UValue = 0, u8VValue = 0;
    UINT32 i = 0, j = 0, bpp = 0, u32Offset = 0, u32PixelNum = 0;
    UINT32 u32RawVal = 0;
    UINT32 u32ImgWidth = 0, u32ImgHeight = 0, u32ImgStride = 0;
    UINT32 u32EndRow = 0;           // 填充结束行，不包含自身行
    UINT32 step = 0;                // 数据行步长，单位：字节
    UINT32 u32LineDataByte = 0;     // 按行字节数
    UINT8 *pu8Y = NULL, *pu8UV = NULL;
    UINT16 *pu16 = NULL, u16Value = 0;
    UINT32 *pRgb32Addr = NULL;
    VOID *pSrcDataVir = NULL;

    struct rgb24
    {
        UINT8 r;
        UINT8 g;
        UINT8 b;
    };
    struct rgb24 stRgb24 = {0};
    struct rgb24 *pRgb24Addr = NULL;

    CHECK_PTR_IS_NULL(pstImageData, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstImageData->pPlaneVir[0], SAL_FAIL);
    if (DSP_IMG_DATFMT_YUV420_MASK == pstImageData->enImgFmt)
    {
        CHECK_PTR_IS_NULL(pstImageData->pPlaneVir[1], SAL_FAIL);
    }

    u32ImgWidth = pstImageData->u32Width;
    u32ImgHeight = pstImageData->u32Height;
    u32ImgStride = pstImageData->u32Stride[0];
    u32EndRow = u32StartRow + u32FillHeight;
    if (0 == u32ImgWidth || 0 == u32ImgStride || u32ImgWidth > u32ImgStride || 
        u32StartRow + u32FillHeight > u32ImgHeight || u32StartCol + u32FillWidth > u32ImgWidth)
    {
        XSP_LOGE("the input parameters are invalid, w:%u, h:%u, s:%u, start row:%u, fill height:%u, start col:%u, fill width:%u\n", 
                 u32ImgWidth, u32ImgHeight, u32ImgStride, u32StartRow, u32FillHeight, u32StartCol, u32FillWidth);
        return SAL_FAIL;
    }
    
    if (DSP_IMG_DATFMT_BGR24 == pstImageData->enImgFmt)
    {
        stRgb24.r = XIMG_32BIT_R_VAL(u32BgValue);
        stRgb24.g = XIMG_32BIT_G_VAL(u32BgValue);
        stRgb24.b = XIMG_32BIT_B_VAL(u32BgValue);
        if (u32ImgWidth == u32ImgStride && u32ImgWidth == u32FillWidth)    // 内存宽度等于stride时，减少for循环,加快速度
        {
            /* rgb */
            pRgb24Addr = (struct rgb24 *)pstImageData->pPlaneVir[0] + u32StartRow * u32ImgWidth;
            u32PixelNum = u32FillHeight * u32ImgWidth; /*填充像素数*/
            for (i = 0; i < u32PixelNum; i++)
            {
                *pRgb24Addr = stRgb24;
                pRgb24Addr++;
            }
        }
        else
        {
            for (i = u32StartRow; i < u32EndRow; i++)
            {
                pRgb24Addr = (struct rgb24 *)pstImageData->pPlaneVir[0] + i * u32ImgStride + u32StartCol;
                for (j = 0; j < u32FillWidth; j++)
                {
                    *pRgb24Addr = stRgb24;
                    pRgb24Addr++;
                }
            }
        }
    }
    else if (DSP_IMG_DATFMT_BGRA32 == pstImageData->enImgFmt)
    {
        if (u32ImgWidth == u32ImgStride && u32ImgWidth == u32FillWidth)    // 内存宽度等于stride时，减少for循环,加快速度
        {
            /* argb */
            pRgb32Addr = (UINT32 *)pstImageData->pPlaneVir[0] + u32StartRow * u32ImgWidth;
            u32PixelNum = u32FillHeight * u32ImgWidth; /*填充像素数*/
            for (i = 0; i < u32PixelNum; i++)
            {
                *pRgb32Addr = u32BgValue;
                pRgb32Addr++;
            }
        }
        else
        {
            for (i = u32StartRow; i < u32EndRow; i++)
            {
                pRgb32Addr = (UINT32 *)pstImageData->pPlaneVir[0] + i * u32ImgStride + u32StartCol;
                for (j = 0; j < u32FillWidth; j++)
                {
                    *pRgb32Addr = u32BgValue;
                    pRgb32Addr++;
                }
            }
        }
    }
    else if (DSP_IMG_DATFMT_YUV420SP_VU == pstImageData->enImgFmt || DSP_IMG_DATFMT_YUV420SP_UV == pstImageData->enImgFmt)
    {
        pu8Y = pstImageData->pPlaneVir[0];
        pu8UV = pstImageData->pPlaneVir[1];
        u8YValue = XIMG_32BIT_Y_VAL(u32BgValue);
        u8UValue = XIMG_32BIT_U_VAL(u32BgValue);
        u8VValue = XIMG_32BIT_V_VAL(u32BgValue);
        if (u32ImgWidth == u32ImgStride && u32ImgWidth == u32FillWidth)    // 内存宽度等于stride时，减少for循环,加快速度
        {
            /* Y */
            pu8Y += u32StartRow * u32ImgStride; /* 跳过前面u32StartRow行，开始覆盖 */
            u32PixelNum = u32FillHeight * u32ImgStride; /*填充像素数*/
            memset(pu8Y, u8YValue, u32PixelNum);

            /* UV */
            pu8UV += u32StartRow * u32ImgStride >> 1; /*色度地址*/
            u32PixelNum >>= 1; /*数据个数*/
            if (u8UValue == u8VValue) /* U、V相等的情况 */
            {
                memset(pu8UV, u8UValue, u32PixelNum);
            }
            else /* U、V不相等 */
            {
                for (i = 0; i < u32PixelNum / 2; i++)
                {
                    *pu8UV = u8VValue; /* V */
                    pu8UV++;
                    *pu8UV = u8UValue; /* U */
                    pu8UV++;
                }
            }
        }
        else
        {
            /* Y */
            for (i = u32StartRow; i < u32EndRow; i++)
            {
                u32Offset = i * u32ImgStride + u32StartCol;
                memset(pu8Y + u32Offset, u8YValue, u32FillWidth);

                /* V,U */
                if (i % 2 == 0)
                {
                    u32Offset = ((i * u32ImgStride) >> 1) + u32StartCol;
                    if (u8UValue == u8VValue) /* U、V相等的情况 */
                    {
                        memset(pu8UV + u32Offset, u8UValue, u32FillWidth);
                    }
                    else /* U、V不相等 */
                    {
                        for (j = 0; j < u32FillWidth; j += 2)
                        {
                            *(pu8UV + u32Offset + j) = u8VValue;
                            *(pu8UV + u32Offset + j + 1) = u8UValue;
                        }
                    }
                }
            }
        }
    }
    else if (DSP_IMG_DATFMT_SP16 == pstImageData->enImgFmt)
    {
        pu16 = (UINT16 *)pstImageData->pPlaneVir[0];
        u16Value = u32BgValue & 0xFFFF;
        for (i = u32StartRow; i < u32EndRow; i++)
        {
            u32Offset = i * u32ImgStride + u32StartCol;
            for (j = 0; j < u32FillWidth; j++)
            {
                *(pu16 + u32Offset + j) = u16Value;
            }
        }
    }
    else if (DSP_IMG_DATFMT_SP8 == pstImageData->enImgFmt)
    {
        pu8Y = (UINT8 *)pstImageData->pPlaneVir[0];
        u8YValue = u32BgValue & 0xFF;
        if (u32ImgWidth == u32ImgStride && u32ImgWidth == u32FillWidth) // 内存宽度等于stride时，减少for循环，加快速度
        {
            pu8Y += u32StartRow * u32ImgStride; /* 跳过前面u32StartRow行，开始覆盖 */
            u32PixelNum = u32FillHeight * u32ImgStride; /* 填充像素数 */
            memset(pu8Y, u8YValue, u32PixelNum);
        }
        else
        {
            for (i = u32StartRow; i < u32EndRow; i++)
            {
                u32Offset = i * u32ImgStride + u32StartCol;
                memset(pu8Y + u32Offset, u8YValue, u32FillWidth);
            }
        }
    }
    else if (DSP_IMG_DATFMT_LHZP == pstImageData->enImgFmt)
    {
        for (i = 0; i < XIMAGE_PLANE_MAX; i++)
        {
            pSrcDataVir = pstImageData->pPlaneVir[i];
            if (NULL == pSrcDataVir)
            {
                break;
            }
            u32RawVal = 0xFF;
            if (2 == i)
            {
                u32RawVal = 0x00;
            }

            bpp = ximg_get_bpp(pstImageData->enImgFmt, i);
            if (0 > bpp)
            {
                SAL_LOGE("ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d\n", pstImageData->enImgFmt, i, bpp);
                return SAL_FAIL;
            }

            step = pstImageData->u32Stride[i] * bpp;
            u32LineDataByte = u32ImgWidth * bpp;
            for (j = 0; j < u32FillHeight; j++)
            {
                memset(pSrcDataVir, u32RawVal, u32LineDataByte);
                pSrcDataVir += step;
            }
        }
    }
    else
    {
        XSP_LOGE("Not supported data format[0x%x] for ximg_fill_color\n", pstImageData->enImgFmt);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function       ximg_flip
 * @brief          将源图像按照指定镜像方向拷贝给目标图像，镜像时支持就地处理，直接修改原图像
 * @param[IN]      XIMAGE_DATA_ST   *pstSrcImg  源图像数据
 * @param[IN/OUT]  XIMAGE_DATA_ST   *pstDstImg  目标图像数据
 * @param[IN/OUT]  XIMAGE_FLIP_MODE enFlipMode  图像镜像方向
 * @return         SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_flip(XIMAGE_DATA_ST *pstSrcImg, XIMAGE_DATA_ST *pstDstImg, XIMAGE_FLIP_MODE enFlipMode)
{
    CHECK_PTR_IS_NULL(pstSrcImg, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstDstImg, SAL_FAIL);
    if (SAL_SOK != ximg_verify_size(pstSrcImg, __FUNCTION__, __LINE__) || SAL_SOK != ximg_verify_size(pstDstImg, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }
    if (pstSrcImg->enImgFmt != pstDstImg->enImgFmt)
    {
        SAL_LOGE("Variable format flip is not supported, fmt_src[0x%x], fmt_dst[0x%x]\n", pstSrcImg->enImgFmt, pstDstImg->enImgFmt);
        return SAL_FAIL;
    }
    if (pstSrcImg->u32Width != pstDstImg->u32Width || pstSrcImg->u32Height != pstDstImg->u32Height)
    {
        SAL_LOGE("Variable ximg dimension flip is not supported, src[%u x %u], dst[%u x %u]\n", 
                 pstSrcImg->u32Width, pstSrcImg->u32Height, pstDstImg->u32Width, pstDstImg->u32Height);
        return SAL_FAIL;
    }

    if (XIMG_FLIP_NONE == enFlipMode)
    {
        ximg_copy_kernel(pstSrcImg, pstDstImg); // 无Flip时直接执行拷贝操作
    }
    else if (XIMG_FLIP_VERT == enFlipMode)
    {
        ximg_flip_vert(pstSrcImg, pstDstImg);
    }
    else if (XIMG_FLIP_HORZ == enFlipMode)
    {
        ximg_flip_horz(pstSrcImg, pstDstImg);
    }
    else
    {
        SAL_LOGE("Unsupported flip copy mode[0x%x]\n", enFlipMode);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

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
SAL_STATUS ximg_crop(XIMAGE_DATA_ST *pstImageSrc, XIMAGE_DATA_ST *pstImageDst, SAL_VideoCrop *pstSrcCropPrm, SAL_VideoCrop *pstDstCropPrm)
{
    SAL_VideoCrop stLocSrcCropPrm = {0};
    SAL_VideoCrop stLocDstCropPrm = {0};
    XIMAGE_DATA_ST stSrcCropImg = {0};
    XIMAGE_DATA_ST stDstCropImg = {0};
    XIMAGE_DATA_ST *pstLocDstImg = {0};

    CHECK_PTR_IS_NULL(pstImageSrc, SAL_FAIL);
    pstLocDstImg = pstImageDst;
    if (NULL == pstImageDst)
    {
        pstLocDstImg = pstImageSrc;
    }
    if (SAL_SOK != ximg_verify_size(pstImageSrc, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }
    if (SAL_SOK != ximg_verify_size(pstLocDstImg, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }

    /* 抠图参数为空时，默认抠取全图 */
    if (NULL == pstSrcCropPrm)
    {
        stLocSrcCropPrm.top = 0;
        stLocSrcCropPrm.left = 0;
        stLocSrcCropPrm.width = pstImageSrc->u32Width;
        stLocSrcCropPrm.height = pstImageSrc->u32Height;
    }
    else
    {
        memcpy(&stLocSrcCropPrm, pstSrcCropPrm, sizeof(SAL_VideoCrop));
    }

    if (NULL == pstDstCropPrm)
    {
        stLocDstCropPrm.top = 0;
        stLocDstCropPrm.left = 0;
        stLocDstCropPrm.width = pstImageDst->u32Width;
        stLocDstCropPrm.height = pstImageDst->u32Height;
    }
    else
    {
        memcpy(&stLocDstCropPrm, pstDstCropPrm, sizeof(SAL_VideoCrop));
    }

    /* 判断原图和目标抠图区域的宽高是否一致 */
    if ((stLocSrcCropPrm.width != stLocDstCropPrm.width) || (stLocSrcCropPrm.height != stLocDstCropPrm.height))
    {
        SAL_LOGE("crop prm src[%u x %u] and dst[%u x %u] are invalid, which are supposed to be equal, crop_prm_vir[src:%p, dst:%p].\n", 
                 stLocSrcCropPrm.width, stLocSrcCropPrm.height, stLocDstCropPrm.width, stLocDstCropPrm.height, pstSrcCropPrm, pstDstCropPrm);
        return SAL_FAIL;
    }
    // HARDWARE COPY
#ifdef NT98336      // nt上纯软件拷贝耗时过大，使用硬件拷贝， 且硬件拷贝raw数据时有问题，暂仅在拷贝yuv数据时使用
    if (pstImageSrc->enImgFmt & DSP_IMG_DATFMT_YUV420_MASK)
    {
        if (0 != pstImageSrc->stMbAttr.phyAddr && 0 != pstImageDst->stMbAttr.phyAddr && 
            pstImageSrc->u32Stride[0] * pstImageSrc->u32Height == (pstImageSrc->pPlaneVir[1] - pstImageSrc->pPlaneVir[0]) && 
            pstImageDst->u32Stride[0] * pstImageDst->u32Height == (pstImageDst->pPlaneVir[1] - pstImageDst->pPlaneVir[0]))    // Y和UV平面连续时
        {
            if (SAL_SOK == ximg_hard_copy(pstImageSrc, pstImageDst, &stLocSrcCropPrm, &stLocDstCropPrm))
            {
                return SAL_SOK;
            }
            SAL_LOGW("ximg_hard_copy failed, use software copy\n");
        }
    }
#endif

    if (SAL_SOK != ximg_create_sub(pstImageSrc, &stSrcCropImg, &stLocSrcCropPrm))
    {
        SAL_LOGE("ximg_create_sub for src ximg faild\n");
        return SAL_FAIL;
    }
    if (SAL_SOK != ximg_create_sub(pstLocDstImg, &stDstCropImg, &stLocDstCropPrm))
    {
        SAL_LOGE("ximg_create_sub for dst ximg faild\n");
        return SAL_FAIL;
    }

    // SOFTWARE COPY
    if (SAL_SOK != ximg_copy_kernel(&stSrcCropImg, &stDstCropImg))
    {
        SAL_LOGE("ximg_copy_kernel faild\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


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
SAL_STATUS ximg_copy_nb(XIMAGE_DATA_ST *pstImageSrc, XIMAGE_DATA_ST *pstImageDst, SAL_VideoCrop *pstSrcCropPrm, SAL_VideoCrop *pstDstCropPrm, XIMAGE_FLIP_MODE enFlipMode)
{
    XIMAGE_DATA_ST stImgSubSrc = {0}, stImgSubDst = {0};
    XIMAGE_DATA_ST *pstImgSrc = pstImageSrc, *pstImgDst = pstImageDst;

    if (enFlipMode == XIMG_FLIP_NONE)
    {
        if (SAL_SOK != ximg_crop(pstImageSrc, pstImageDst, pstSrcCropPrm, pstDstCropPrm))
        {
            SAL_LOGE("ximg_crop failed\n");
            return SAL_FAIL;
        }
    }
    else
    {
        if (NULL != pstSrcCropPrm)
        {
            if (SAL_SOK != ximg_create_sub(pstImageSrc, &stImgSubSrc, pstSrcCropPrm))
            {
                SAL_LOGE("ximg_create_sub faild\n");
                return SAL_FAIL;
            }
            pstImgSrc = &stImgSubSrc;
        }
        if (NULL != pstDstCropPrm)
        {
            if (SAL_SOK != ximg_create_sub(pstImageDst, &stImgSubDst, pstDstCropPrm))
            {
                SAL_LOGE("ximg_create_sub faild\n");
                return SAL_FAIL;
            }
            pstImgDst = &stImgSubDst;
        }
        if (SAL_SOK != ximg_flip(pstImgSrc, pstImgDst, enFlipMode))
        {
            SAL_LOGE("ximg_flip faild\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

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
SAL_STATUS ximg_resize(XIMAGE_DATA_ST *pstImageSrc, XIMAGE_DATA_ST *pstImageDst)
{
    SAL_STATUS ret = SAL_SOK;
    UINT32 u32ScaleTmp = 0;
    UINT32 u32SrcW = 0, u32SrcH = 0;
    UINT32 *pu32ScaleW = NULL, *pu32ScaleH = NULL;

    static SYSTEM_FRAME_INFO stSrcFrame = {0};
    static SYSTEM_FRAME_INFO stDstFrame = {0};
    static pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;

    CHECK_PTR_IS_NULL(pstImageSrc, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstImageDst, SAL_FAIL);
    if (SAL_SOK != ximg_verify_size(pstImageSrc, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }
    if (pstImageSrc->enImgFmt != pstImageDst->enImgFmt)
    {
        SAL_LOGE("Invalid img fmt, src:0x%x, dst:0x%x\n", pstImageSrc->enImgFmt, pstImageDst->enImgFmt);
        return SAL_FAIL;
    }

    u32SrcW = pstImageSrc->u32Width;
    u32SrcH = pstImageSrc->u32Height;
    pu32ScaleW = &pstImageDst->u32Width;
    pu32ScaleH = &pstImageDst->u32Height;

    if (*pu32ScaleW < 64)
    {
        u32ScaleTmp = SAL_align((UINT32)(*pu32ScaleH * 64.0f / *pu32ScaleW), 2);
        if (*pu32ScaleH < u32SrcH)
        {
            u32ScaleTmp = SAL_MIN(u32ScaleTmp, u32SrcH);
        }
        *pu32ScaleH = u32ScaleTmp;
        *pu32ScaleW = 64;
    }
    if (*pu32ScaleH < 64)
    {
        u32ScaleTmp = SAL_align((UINT32)(*pu32ScaleW * 64.0f / *pu32ScaleH), 2);
        if (*pu32ScaleW < u32SrcW)
        {
            u32ScaleTmp = SAL_MIN(u32ScaleTmp, u32SrcW);
        }
        *pu32ScaleW = u32ScaleTmp;
        *pu32ScaleH = 64;
    }
    ximg_set_dimension(pstImageDst, *pu32ScaleW, *pu32ScaleW, SAL_FALSE, 0);
    if (SAL_SOK != ximg_verify_size(pstImageDst, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }

    if (SAL_SOK == SAL_mutexTmLock(&sLock, 500, __FUNCTION__, __LINE__))
    {
        Sal_halBuildSysFrame(&stSrcFrame, NULL, pstImageSrc, NULL);
        Sal_halBuildSysFrame(&stDstFrame, NULL, pstImageDst, NULL);
        ret = vgs_hal_scaleFrame(&stDstFrame, &stSrcFrame, pstImageDst->u32Width, pstImageDst->u32Height);
        SAL_mutexTmUnlock(&sLock, __FUNCTION__, __LINE__);
        if (SAL_SOK != ret)
        {
            SAL_LOGE("vgs_hal_scaleFrame failed\n");
            return SAL_FAIL;
        }
    }
    else
    {
        SAL_LOGE("sal_mutex_lock failed.\n");
        return SAL_FAIL;
    }
    return SAL_SOK;
}

/**
 * @function   ximg_yc_stretch
 * @brief      YUV值域转换，“YC伸张”，将像素值范围由16~235转换为0~255，就地处理，最大支持尺寸：2048x1280
 *             硬件处理：  是否支持横向跨距    跨距对齐
 *                             V             16
 * @param[IN/OUT]  XIMAGE_DATA_ST *pstSrcImg    源数据，处理完成后为值域拉伸后的图像数据，硬件处理需跨距16对齐
 * @return         SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_yc_stretch(XIMAGE_DATA_ST *pstSrcImg)
{
    INT32 i = 0, j = 0;
    const UINT32 u32YuvMaxW = 2048, u32YuvMaxH = 1280;
    UINT32 u32ImgWidth = 0, u32AlignedWidth = 0, u32ImgHeight = 0, u32ImgStride = 0;
    UINT8 *pu8YPixel = NULL, *pu8UvPiexel = NULL;
    SAL_VideoSize stSrcSize = {0}, stDstSize = {0};
    SAL_VideoCrop stCropPrm = {0};

    static const INT16 as16CoefY[] =
    {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,                           /* 作为数据填充 */
        0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 17,
        19, 20, 21, 22, 23, 24, 26, 27, 28, 29, 30, 31, 33, 34, 35, 36,
        37, 38, 40, 41, 42, 43, 44, 45, 47, 48, 49, 50, 51, 52, 54, 55,
        56, 57, 58, 59, 61, 62, 63, 64, 65, 66, 68, 69, 70, 71, 72, 73,
        75, 76, 77, 78, 79, 80, 82, 83, 84, 85, 86, 87, 88, 90, 91, 92,
        93, 94, 95, 97, 98, 99, 100, 101, 102, 104, 105, 106, 107, 108, 109, 111,
        112, 113, 114, 115, 116, 118, 119, 120, 121, 122, 123, 125, 126, 127, 128, 129,
        130, 132, 133, 134, 135, 136, 137, 139, 140, 141, 142, 143, 144, 146, 147, 148,
        149, 150, 151, 153, 154, 155, 156, 157, 158, 160, 161, 162, 163, 164, 165, 167,
        168, 169, 170, 171, 172, 173, 175, 176, 177, 178, 179, 180, 182, 183, 184, 185,
        186, 187, 189, 190, 191, 192, 193, 194, 196, 197, 198, 199, 200, 201, 203, 204,
        205, 206, 207, 208, 210, 211, 212, 213, 214, 215, 217, 218, 219, 220, 221, 222,
        224, 225, 226, 227, 228, 229, 231, 232, 233, 234, 235, 236, 238, 239, 240, 241,
        242, 243, 245, 246, 247, 248, 249, 250, 252, 253, 254, 255,
        236, 237, 238, 239,                                                             /* 作为数据填充 */
        240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, /* 作为数据填充 */
    };
    static const INT16 as16CoefUV[] =
    {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                           /* 作为数据填充 */
        0,   1,   2,   3,   5,   6,   7,   8,   9,   10,  11,  13,  14,  15,  16,  17,
        18,  19,  20,  22,  23,  24,  25,  26,  27,  28,  30,  31,  32,  33,  34,  35,
        36,  38,  39,  40,  41,  42,  43,  44,  46,  47,  48,  49,  50,  51,  52,  54,
        55,  56,  57,  58,  59,  60,  61,  63,  64,  65,  66,  67,  68,  69,  71,  72,
        73,  74,  75,  76,  77,  79,  80,  81,  82,  83,  84,  85,  87,  88,  89,  90,
        91,  92,  93,  94,  96,  97,  98,  99,  100, 101, 102, 104, 105, 106, 107, 108,
        109, 110, 112, 113, 114, 115, 116, 117, 118, 120, 121, 122, 123, 124, 125, 126,
        128, 129, 130, 131, 132, 133, 134, 135, 137, 138, 139, 140, 141, 142, 143, 145,
        146, 147, 148, 149, 150, 151, 153, 154, 155, 156, 157, 158, 159, 161, 162, 163,
        164, 165, 166, 167, 168, 170, 171, 172, 173, 174, 175, 176, 178, 179, 180, 181,
        182, 183, 184, 186, 187, 188, 189, 190, 191, 192, 194, 195, 196, 197, 198, 199,
        200, 201, 203, 204, 205, 206, 207, 208, 209, 211, 212, 213, 214, 215, 216, 217,
        219, 220, 221, 222, 223, 224, 225, 227, 228, 229, 230, 231, 232, 233, 235, 236,
        237, 238, 239, 240, 241, 242, 244, 245, 246, 247, 248, 249, 250, 252, 253, 254,
        255,
        255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,      /* 作为数据填充 */
    };
    static XIMAGE_DATA_ST stTmpScaleImg = {0};
    static pthread_mutex_t sLock = PTHREAD_MUTEX_INITIALIZER;

    CHECK_PTR_IS_NULL(pstSrcImg, SAL_FAIL);
    if (pstSrcImg->u32Stride[0] > u32YuvMaxW || pstSrcImg->u32Height > u32YuvMaxH)
    {
        XSP_LOGE("ximg dimension[w:%u, h%u, s:%u] for yc-strecth exceeds max supported dimension[%u x %u]\n", 
                 pstSrcImg->u32Width, pstSrcImg->u32Height, pstSrcImg->u32Stride[0], u32YuvMaxW, u32YuvMaxH);
        return SAL_FAIL;
    }
    if (SAL_SOK != ximg_verify_size(pstSrcImg, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }

    /* 初始化临时缩放后图像内存 */
    if (NULL == stTmpScaleImg.pPlaneVir[0])
    {
        stTmpScaleImg.stMbAttr.bCached = SAL_FALSE;
        if (SAL_SOK != ximg_create(u32YuvMaxW, u32YuvMaxH, DSP_IMG_DATFMT_YUV420SP_VU, "img-stretch-yuv", NULL, &stTmpScaleImg))
        {
            XSP_LOGE("ximg_create for yc-stetch failed, w:%u, h:%u, fmt:0x%x\n", u32YuvMaxW, u32YuvMaxH, DSP_IMG_DATFMT_YUV420SP_VU);
            return SAL_FAIL;
        }
    }

    u32ImgWidth = pstSrcImg->u32Width;
    u32ImgHeight = pstSrcImg->u32Height;
    u32ImgStride = pstSrcImg->u32Stride[0];
    u32AlignedWidth = SAL_MIN(SAL_align(u32ImgWidth, 16), u32ImgStride);

    // HARDWARE:
    if (u32ImgWidth >= 64 && u32ImgHeight >= 64 && pstSrcImg->stMbAttr.phyAddr && 0 == (u32ImgStride % 16))
    {
        /* 通过互斥锁防止多线程同时调用硬件接口，且下一次调用硬件接口时上一次的结果已经被取出 */
        if (SAL_SOK == SAL_mutexTmLock(&sLock, 1000, __FUNCTION__, __LINE__))
        {
            stTmpScaleImg.u32Height = u32ImgHeight;
            ximg_set_dimension(&stTmpScaleImg, u32ImgWidth, u32AlignedWidth, SAL_FALSE, XIMG_BG_DEFAULT_YUV);

            stSrcSize.width = u32ImgStride;
            stSrcSize.height = u32ImgHeight;
            stDstSize.width = stTmpScaleImg.u32Stride[0];
            stDstSize.height = stTmpScaleImg.u32Height;
            stCropPrm.left = 0;
            stCropPrm.top = 0;
            stCropPrm.width = u32ImgWidth;
            stCropPrm.height = u32ImgHeight;
            if (SAL_SOK == dup_hal_scaleYuvRange(&stSrcSize, &stDstSize, &stCropPrm, &stCropPrm, 
                                                 pstSrcImg->pPlaneVir[0], stTmpScaleImg.pPlaneVir[0], SAL_FALSE))
            {
                /* 将缩放后数据拷贝回源图像 */
                ximg_flip(&stTmpScaleImg, pstSrcImg, XIMG_FLIP_NONE);
                SAL_mutexTmUnlock(&sLock, __FUNCTION__, __LINE__);
                return SAL_SOK;
            }
            SAL_mutexTmUnlock(&sLock, __FUNCTION__, __LINE__);
            SAL_LOGW("dup_hal_scaleYuvRange failed, use software scale.\n");
        }
    }

    // SOFTWARE:
    /* 宽高不足64无法使用SCA模块转换或者硬件处理失败，使用软件转换 */    
    for (i = 0; i < u32ImgHeight; i++)
    {
        for (j = 0; j < u32ImgWidth; j++)
        {
            pu8YPixel = (UINT8 *)pstSrcImg->pPlaneVir[0] + i * u32ImgStride + j;
            *pu8YPixel = (UINT8)as16CoefY[*pu8YPixel];
            if (0 == i % 2)
            {
                pu8UvPiexel = (UINT8 *)pstSrcImg->pPlaneVir[1] + (i >> 1) * u32ImgStride + j;
                *pu8UvPiexel       = (UINT8)as16CoefUV[*pu8UvPiexel];
                *(pu8UvPiexel + 1) = (UINT8)as16CoefUV[*(pu8UvPiexel + 1)];
            }
        }
    }

    return SAL_SOK;
}

/**
 * @function       ximg_convert_yuv2rgb
 * @brief          将图像yuv格式转换为rgb24格式，r_g_b单平面输出(yuv要求是16-235的，输入rgb是0-255的)
 * @param[IN]      XIMAGE_DATA_ST *pstYuvData  原始yuv格式图像,tv range(16-235)
 * @param[IN/OUT]  XIMAGE_DATA_ST *pstRgbData  输出RGB格式数据，full range(0-255)
 * @return         SAL_STATUS  SAL_FAIL 失败 SAL_SOK  成功
 */
static SAL_STATUS ximg_convert_yuv2rgb(XIMAGE_DATA_ST *pstYuvData, XIMAGE_DATA_ST *pstRgbData)
{
    INT16 r = 0, g = 0, b = 0; /* , a = 0; */
    UINT8 y = 0, u = 0, v = 0;
    UINT32 i = 0, j = 0, k = 0;
    UINT32 GOffset = 0, BOffset = 0;
    UINT32 u32ImgWidth = 0;
    UINT32 u32ImgHeight = 0;
    UINT32 u32ImgStride = 0;
    SAL_VideoDataFormat enDstColorType = SAL_VIDEO_DATFMT_INVALID;  /* 目标图像格式 */
    UINT8 *pRgbDataAddr = NULL;
    UINT8 *p8YDataAddr = NULL;              /* 输入的yuv起始地址 */
    UINT8 *p8UvDataStartAddr = NULL;        /* 输入的yuv的uv数据的起始地址 */
    UINT8 *p8YDataRowStart = NULL;          /* y数据的行首地址 */
    UINT8 *p8UvDataRowStart = NULL;         /* uv数据的行首地址 */
    UINT8 *p8VDataAddr = NULL;              /* v数据的行首地址 */

    IVE_HAL_IMAGE stIveYuv = {0};
    IVE_HAL_IMAGE stIveRgb = {0};
    IVE_HAL_MODE_CTRL stCscCtrl = {0};

    const INT16 R_Y_1[] =
    {
        -18, -17, -16, -15, -13, -12, -11, -10, -9, -8, -6, -5, -4, -3,
        -2, -1, 0, 1, 2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 15, 16, 17,
        18, 19, 20, 22, 23, 24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 36,
        37, 38, 39, 40, 41, 43, 44, 45, 46, 47, 48, 50, 51, 52, 53, 54,
        55, 57, 58, 59, 60, 61, 62, 64, 65, 66, 67, 68, 69, 71, 72, 73,
        74, 75, 76, 77, 79, 80, 81, 82, 83, 84, 86, 87, 88, 89, 90, 91,
        93, 94, 95, 96, 97, 98, 100, 101, 102, 103, 104, 105, 107, 108,
        109, 110, 111, 112, 114, 115, 116, 117, 118, 119, 121, 122, 123,
        124, 125, 126, 128, 129, 130, 131, 132, 133, 135, 136, 137, 138,
        139, 140, 142, 143, 144, 145, 146, 147, 148, 150, 151, 152, 153,
        154, 155, 157, 158, 159, 160, 161, 162, 164, 165, 166, 167, 168,
        169, 171, 172, 173, 174, 175, 176, 178, 179, 180, 181, 182, 183,
        185, 186, 187, 188, 189, 190, 192, 193, 194, 195, 196, 197, 199,
        200, 201, 202, 203, 204, 206, 207, 208, 209, 210, 211, 213, 214,
        215, 216, 217, 218, 219, 221, 222, 223, 224, 225, 226, 228, 229,
        230, 231, 232, 233, 235, 236, 237, 238, 239, 240, 242, 243, 244,
        245, 246, 247, 249, 250, 251, 252, 253, 254, 256, 257, 258, 259,
        260, 261, 263, 264, 265, 266, 267, 268, 270, 271, 272, 273, 274,
        275, 277, 278
    };

    const INT16 R_Cr[] =
    {
        -204, -202, -201, -199, -197, -196, -194, -193, -191, -189, -188,
        -186, -185, -183, -181, -180, -178, -177, -175, -173, -172, -170,
        -169, -167, -165, -164, -162, -161, -159, -158, -156, -154, -153,
        -151, -150, -148, -146, -145, -143, -142, -140, -138, -137, -135,
        -134, -132, -130, -129, -127, -126, -124, -122, -121, -119, -118,
        -116, -114, -113, -111, -110, -108, -106, -105, -103, -102, -100,
        -98, -97, -95, -94, -92, -90, -89, -87, -86, -84, -82, -81, -79,
        -78, -76, -75, -73, -71, -70, -68, -67, -65, -63, -62, -60, -59,
        -57, -55, -54, -52, -51, -49, -47, -46, -44, -43, -41, -39, -38,
        -36, -35, -33, -31, -30, -28, -27, -25, -23, -22, -20, -19, -17,
        -15, -14, -12, -11, -9, -7, -6, -4, -3, -1, 0, 1, 3, 4, 6, 7, 9,
        11, 12, 14, 15, 17, 19, 20, 22, 23, 25, 27, 28, 30, 31, 33, 35,
        36, 38, 39, 41, 43, 44, 46, 47, 49, 51, 52, 54, 55, 57, 59, 60,
        62, 63, 65, 67, 68, 70, 71, 73, 75, 76, 78, 79, 81, 82, 84, 86,
        87, 89, 90, 92, 94, 95, 97, 98, 100, 102, 103, 105, 106, 108, 110,
        111, 113, 114, 116, 118, 119, 121, 122, 124, 126, 127, 129, 130,
        132, 134, 135, 137, 138, 140, 142, 143, 145, 146, 148, 150, 151,
        153, 154, 156, 158, 159, 161, 162, 164, 165, 167, 169, 170, 172,
        173, 175, 177, 178, 180, 181, 183, 185, 186, 188, 189, 191, 193,
        194, 196, 197, 199, 201, 202
    };

    const INT16 G_Cr[] =
    {
        -104, -103, -102, -101, -100, -99, -99, -98, -97, -96, -95, -95,
        -94, -93, -92, -91, -91, -90, -89, -88, -87, -86, -86, -85, -84, -83,
        -82, -82, -81, -80, -79, -78, -78, -77, -76, -75, -74, -73, -73, -72,
        -71, -70, -69, -69, -68, -67, -66, -65, -65, -64, -63, -62, -61, -60,
        -60, -59, -58, -57, -56, -56, -55, -54, -53, -52, -52, -51, -50, -49,
        -48, -47, -47, -46, -45, -44, -43, -43, -42, -41, -40, -39, -39, -38,
        -37, -36, -35, -34, -34, -33, -32, -31, -30, -30, -29, -28, -27, -26,
        -26, -25, -24, -23, -22, -21, -21, -20, -19, -18, -17, -17, -16, -15,
        -14, -13, -13, -12, -11, -10, -9, -8, -8, -7, -6, -5, -4, -4, -3, -2,
        -1, 0, 0, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 8, 9, 10, 11, 12, 13, 13, 14,
        15, 16, 17, 17, 18, 19, 20, 21, 21, 22, 23, 24, 25, 26, 26, 27, 28, 29,
        30, 30, 31, 32, 33, 34, 34, 35, 36, 37, 38, 39, 39, 40, 41, 42, 43, 43,
        44, 45, 46, 47, 47, 48, 49, 50, 51, 52, 52, 53, 54, 55, 56, 56, 57, 58,
        59, 60, 60, 61, 62, 63, 64, 65, 65, 66, 67, 68, 69, 69, 70, 71, 72, 73,
        73, 74, 75, 76, 77, 78, 78, 79, 80, 81, 82, 82, 83, 84, 85, 86, 86, 87,
        88, 89, 90, 91, 91, 92, 93, 94, 95, 95, 96, 97, 98, 99, 99, 100, 101,
        102, 103
    };

    const INT16 G_Cb[] =
    {
        -50, -49, -49, -48, -48, -48, -47, -47, -46, -46, -46, -45, -45, -44,
        -44, -44, -43, -43, -43, -42, -42, -41, -41, -41, -40, -40, -39, -39,
        -39, -38, -38, -37, -37, -37, -36, -36, -35, -35, -35, -34, -34, -34,
        -33, -33, -32, -32, -32, -31, -31, -30, -30, -30, -29, -29, -28, -28,
        -28, -27, -27, -26, -26, -26, -25, -25, -25, -24, -24, -23, -23, -23,
        -22, -22, -21, -21, -21, -20, -20, -19, -19, -19, -18, -18, -17, -17,
        -17, -16, -16, -16, -15, -15, -14, -14, -14, -13, -13, -12, -12, -12,
        -11, -11, -10, -10, -10, -9, -9, -8, -8, -8, -7, -7, -7, -6, -6, -5,
        -5, -5, -4, -4, -3, -3, -3, -2, -2, -1, -1, -1, 0, 0, 0, 0, 0, 1, 1,
        1, 2, 2, 3, 3, 3, 4, 4, 5, 5, 5, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 10, 10,
        10, 11, 11, 12, 12, 12, 13, 13, 14, 14, 14, 15, 15, 16, 16, 16, 17, 17,
        17, 18, 18, 19, 19, 19, 20, 20, 21, 21, 21, 22, 22, 23, 23, 23, 24, 24,
        25, 25, 25, 26, 26, 26, 27, 27, 28, 28, 28, 29, 29, 30, 30, 30, 31, 31,
        32, 32, 32, 33, 33, 34, 34, 34, 35, 35, 35, 36, 36, 37, 37, 37, 38, 38,
        39, 39, 39, 40, 40, 41, 41, 41, 42, 42, 43, 43, 43, 44, 44, 44, 45, 45,
        46, 46, 46, 47, 47, 48, 48, 48, 49, 49
    };

    const INT16 B_Cb[] =
    {
        -258, -256, -254, -252, -250, -248, -246, -244, -242, -240, -238, -236,
        -234, -232, -230, -228, -226, -223, -221, -219, -217, -215, -213, -211,
        -209, -207, -205, -203, -201, -199, -197, -195, -193, -191, -189, -187,
        -185, -183, -181, -179, -177, -175, -173, -171, -169, -167, -165, -163,
        -161, -159, -157, -155, -153, -151, -149, -147, -145, -143, -141, -139,
        -137, -135, -133, -131, -129, -127, -125, -123, -121, -119, -117, -115,
        -113, -110, -108, -106, -104, -102, -100, -98, -96, -94, -92, -90, -88,
        -86, -84, -82, -80, -78, -76, -74, -72, -70, -68, -66, -64, -62, -60,
        -58, -56, -54, -52, -50, -48, -46, -44, -42, -40, -38, -36, -34, -32,
        -30, -28, -26, -24, -22, -20, -18, -16, -14, -12, -10, -8, -6, -4, -2,
        0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36,
        38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 68, 70, 72,
        74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106,
        108, 110, 113, 115, 117, 119, 121, 123, 125, 127, 129, 131, 133, 135, 137,
        139, 141, 143, 145, 147, 149, 151, 153, 155, 157, 159, 161, 163, 165, 167,
        169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 195, 197,
        199, 201, 203, 205, 207, 209, 211, 213, 215, 217, 219, 221, 223, 226, 228,
        230, 232, 234, 236, 238, 240, 242, 244, 246, 248, 250, 252, 254, 256
    };

    CHECK_PTR_IS_NULL(pstYuvData, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstRgbData, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstYuvData->pPlaneVir[0], SAL_FAIL);
    CHECK_PTR_IS_NULL(pstYuvData->pPlaneVir[1], SAL_FAIL);
    CHECK_PTR_IS_NULL(pstRgbData->pPlaneVir[0], SAL_FAIL);

    u32ImgWidth = pstYuvData->u32Width;
    u32ImgHeight = pstYuvData->u32Height;
    u32ImgStride = pstYuvData->u32Stride[0];
    if (DSP_IMG_DATFMT_BGR24 == pstRgbData->enImgFmt)
    {
        enDstColorType = SAL_VIDEO_DATFMT_RGB24_888;
    }
    else if (DSP_IMG_DATFMT_BGRP == pstRgbData->enImgFmt || DSP_IMG_DATFMT_BGRAP == pstRgbData->enImgFmt)
    {
        enDstColorType = SAL_VIDEO_DATFMT_BGR;
    }
    else
    {
        SAL_LOGE("Not supported dst ximg format[0x%x]\n", pstRgbData->enImgFmt);
        return SAL_FAIL;
    }

    p8YDataAddr = pstYuvData->pPlaneVir[0];
    p8UvDataStartAddr = pstYuvData->pPlaneVir[1];
    pRgbDataAddr = (UINT8 *)(pstRgbData->pPlaneVir[0]);

    if (pstYuvData->stMbAttr.phyAddr && pstRgbData->stMbAttr.phyAddr)
    {
        if (SAL_SOK != ximg_yc_stretch(pstYuvData)) /* tv range --> full range */
        {
            XSP_LOGW("ximg_yc_stretch failed, w:%u,h:%u\n", pstYuvData->u32Width, pstYuvData->u32Height);
            return SAL_FAIL;
        }

        stIveYuv.enColorType = SAL_VIDEO_DATFMT_YUV420SP_UV;      /*YUV420 SemiPlanar*/
        stIveYuv.u64VirAddr[0] = (UINT64)(pstYuvData->pPlaneVir[0]);
        stIveYuv.u64VirAddr[1] = (UINT64)(pstYuvData->pPlaneVir[1]);
        stIveYuv.u64PhyAddr[0] = pstYuvData->stMbAttr.phyAddr;
        stIveYuv.u64PhyAddr[1] = stIveYuv.u64PhyAddr[0] + stIveYuv.u64VirAddr[1] - stIveYuv.u64VirAddr[0];
        stIveYuv.u32Stride[0] = u32ImgStride; /*需要16对齐*/
        stIveYuv.u32Stride[1] = u32ImgStride;
        stIveYuv.u32Width = u32ImgWidth;
        stIveYuv.u32Height = u32ImgHeight;

        stIveRgb.enColorType = enDstColorType;
        stIveRgb.u64VirAddr[0] = (UINT64)(pstRgbData->pPlaneVir[0]);
        stIveRgb.u64VirAddr[1] = (UINT64)(pstRgbData->pPlaneVir[1]);
        stIveRgb.u64VirAddr[2] = (UINT64)(pstRgbData->pPlaneVir[2]);
        stIveRgb.u64PhyAddr[0] = pstRgbData->stMbAttr.phyAddr;
        stIveRgb.u64PhyAddr[1] = stIveRgb.u64PhyAddr[0] + stIveRgb.u64VirAddr[1] - stIveRgb.u64VirAddr[0];
        stIveRgb.u64PhyAddr[2] = stIveRgb.u64PhyAddr[0] + stIveRgb.u64VirAddr[2] - stIveRgb.u64VirAddr[0];
        stIveRgb.u32Stride[0] = u32ImgStride;
        stIveRgb.u32Stride[1] = u32ImgStride;
        stIveRgb.u32Stride[2] = u32ImgStride;
        stIveRgb.u32Width = u32ImgWidth;
        stIveRgb.u32Height = u32ImgHeight;

        /* 输出B G R模式，地址16对齐*/
        stCscCtrl.u32enMode = IVE_CSC_PIC_BT601_YUV2RGB;
        if (SAL_SOK != ive_hal_CSC(&stIveYuv, &stIveRgb, &stCscCtrl))
        {
            XSP_LOGW("ive_hal_CSC failed!\n ");
            goto SOFTWARE;
        }

        return SAL_SOK;
    }

SOFTWARE:

    if(enDstColorType == SAL_VIDEO_DATFMT_RGB24_888)
    {
        for (i = 0; i < u32ImgHeight; i++)
        {
            p8YDataRowStart = p8YDataAddr + i * u32ImgStride;
            p8UvDataRowStart = p8UvDataStartAddr + (i >> 1) * u32ImgStride;
            for (j = 0; j < u32ImgWidth; j++)
            {
                p8VDataAddr = p8UvDataRowStart + ((j >> 1) << 1);
                y = *(p8YDataRowStart + j);
                v = *p8VDataAddr;
                u = *(p8VDataAddr + 1);

                r = SAL_CLIP(R_Y_1[y] + R_Cr[v], 0, 255);
                g = SAL_CLIP(R_Y_1[y] - G_Cr[v] - G_Cb[u], 0, 255);
                b = SAL_CLIP(R_Y_1[y] + B_Cb[u], 0, 255);

                /* r = ((y-offs[0])*coef[0][0] + (u-offs[1])*coef[0][1] + (v-offs[2])*coef[0][2] + 512) >> 10; */
                /* g = ((y-offs[0])*coef[1][0] + (u-offs[1])*coef[1][1] + (v-offs[2])*coef[1][2] + 512) >> 10; */
                /* b = ((y-offs[0])*coef[2][0] + (u-offs[1])*coef[2][1] + (v-offs[2])*coef[2][2] + 512) >> 10; */
                /* r = SAL_CLIP(r, 0, 255); */
                /* g = SAL_CLIP(g, 0, 255); */
                /* b = SAL_CLIP(b, 0, 255); */

                *pRgbDataAddr++ = (UINT8)b;
                *pRgbDataAddr++ = (UINT8)g;
                *pRgbDataAddr++ = (UINT8)r;
            }
        }
    }
    else /* SAL_VIDEO_DATFMT_BGR */
    {
        k = 0;
        GOffset = u32ImgHeight * u32ImgStride;
        BOffset = u32ImgHeight * u32ImgStride * 2;

        for (i = 0; i < u32ImgHeight; i++)
        {
            p8YDataRowStart = p8YDataAddr + i * u32ImgStride;
            p8UvDataRowStart = p8UvDataStartAddr + (i >> 1) * u32ImgStride;
            for (j = 0; j < u32ImgWidth; j++)
            {
                p8VDataAddr = p8UvDataRowStart + ((j >> 1) << 1);
                y = *(p8YDataRowStart + j);
                v = *p8VDataAddr;
                u = *(p8VDataAddr + 1);

                r = SAL_CLIP(R_Y_1[y] + R_Cr[v], 0, 255);
                g = SAL_CLIP(R_Y_1[y] - G_Cr[v] - G_Cb[u], 0, 255);
                b = SAL_CLIP(R_Y_1[y] + B_Cb[u], 0, 255);

                pRgbDataAddr[k] = (UINT8)r;
                pRgbDataAddr[GOffset + k] = (UINT8)g;
                pRgbDataAddr[BOffset + k] = (UINT8)b;
                k++;
            }
        }
    }

    return SAL_SOK;
}

/**
 * @function   ximg_convert_fmt
 * @brief      原始图像格式转换，先尝试硬件方法转换，硬件方法失败后会使用软件方法转换
 * @param[IN]  XIMAGE_DATA_ST  *pstImageSrc    输入原始图片数据
 * @param[IN]  XIMAGE_DATA_ST  *pstImageDst    输出图片数据
 * @return     SAL_STATUS SAL_FAIL 失败 SAL_SOK  成功
 */
SAL_STATUS ximg_convert_fmt(XIMAGE_DATA_ST *pstImageSrc, XIMAGE_DATA_ST *pstImageDst)
{
    INT32 i = 0, j = 0, bpp = 0;
    UINT32 u32ImgWidth = 0, u32ImgHeight = 0;
    UINT32 sstep = 0, dstep[4] = {0};               // 数据行步长，单位：字节
    UINT8 *pu08SrcLineAddr = NULL;
    UINT8 *pu08SrcPixelAddr = NULL;
    UINT8 *pu08DstLineAddr0 = NULL;
    UINT8 *pu08DstLineAddr1 = NULL;
    UINT8 *pu08DstLineAddr2 = NULL;
    UINT8 *pu08DstLineAddr3 = NULL;

    CHECK_PTR_IS_NULL(pstImageSrc, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstImageDst, SAL_FAIL);
    if (SAL_SOK != ximg_verify_size(pstImageSrc, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }
    if (SAL_SOK != ximg_verify_size(pstImageDst, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }
    if (pstImageSrc->u32Width != pstImageDst->u32Width || pstImageSrc->u32Height != pstImageDst->u32Height)
    {
        SAL_LOGE("The dimension of source[%u x %u] and dst[%u x %u] is not matched\n", 
                 pstImageSrc->u32Width, pstImageSrc->u32Height, pstImageDst->u32Width, pstImageDst->u32Height);
        return SAL_FAIL;
    }

    u32ImgWidth = pstImageSrc->u32Width;
    u32ImgHeight = pstImageSrc->u32Height;

    if ((DSP_IMG_DATFMT_YUV420_MASK & pstImageSrc->enImgFmt) && (DSP_IMG_DATFMT_BGR24 & pstImageDst->enImgFmt))
    {
        /* yuv转rgb格式(tv range --> full range) */
        if (SAL_SOK != ximg_convert_yuv2rgb(pstImageSrc, pstImageDst))
        {
            XSP_LOGE("ximg_convert_yuv2rgb failed.\n");
            return SAL_FAIL;
        }
    }
    else if (((DSP_IMG_DATFMT_BGR24 & pstImageSrc->enImgFmt) || (DSP_IMG_DATFMT_BGRA32 & pstImageSrc->enImgFmt)) && 
             ((DSP_IMG_DATFMT_BGRP & pstImageDst->enImgFmt) || (DSP_IMG_DATFMT_BGRAP & pstImageDst->enImgFmt)))
    {
        /* rgb交叉转平面格式 */
        bpp = ximg_get_bpp(pstImageSrc->enImgFmt, 0);
        if (0 > bpp)
        {
            SAL_LOGE("ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d\n", pstImageSrc->enImgFmt, 0, bpp);
            return SAL_FAIL;
        }

        pu08SrcLineAddr = (UINT8 *)pstImageSrc->pPlaneVir[0];
        pu08DstLineAddr0 = (UINT8 *)pstImageDst->pPlaneVir[0];
        pu08DstLineAddr1 = (UINT8 *)pstImageDst->pPlaneVir[1];
        pu08DstLineAddr2 = (UINT8 *)pstImageDst->pPlaneVir[2];
        if (DSP_IMG_DATFMT_BGRA32 & pstImageSrc->enImgFmt && DSP_IMG_DATFMT_BGRAP & pstImageDst->enImgFmt)
        {
            pu08DstLineAddr3 = (UINT8 *)pstImageDst->pPlaneVir[3];
        }

        sstep = pstImageSrc->u32Stride[0] * bpp;
        dstep[0] = pstImageDst->u32Stride[0] * ximg_get_bpp(pstImageDst->enImgFmt, 0);
        dstep[1] = pstImageDst->u32Stride[1] * ximg_get_bpp(pstImageDst->enImgFmt, 1);
        dstep[2] = pstImageDst->u32Stride[2] * ximg_get_bpp(pstImageDst->enImgFmt, 2);
        dstep[3] = pstImageDst->u32Stride[3] * ximg_get_bpp(pstImageDst->enImgFmt, 3);
        /* BGR平面化 */
        for (i = 0; i < u32ImgHeight; i++)
        {
            pu08SrcPixelAddr = pu08SrcLineAddr;
            for (j = 0; j < u32ImgWidth; j++)
            {
                *(pu08DstLineAddr2 + j) = *(pu08SrcPixelAddr);
                *(pu08DstLineAddr1 + j) = *(pu08SrcPixelAddr + 1);
                *(pu08DstLineAddr0 + j) = *(pu08SrcPixelAddr + 2);
                pu08SrcPixelAddr += bpp;
            }
            pu08SrcLineAddr += sstep;
            pu08DstLineAddr0 += dstep[0];
            pu08DstLineAddr1 += dstep[1];
            pu08DstLineAddr2 += dstep[2];
        }
        /* A平面化，使用场景少，单独处理，加快速度 */
        if (NULL != pu08DstLineAddr3)
        {
            pu08SrcLineAddr = (UINT8 *)pstImageSrc->pPlaneVir[0];
            for (i = 0; i < u32ImgHeight; i++)
            {
                pu08SrcPixelAddr = pu08SrcLineAddr;
                for (j = 0; j < u32ImgWidth; j++)
                {
                    *(pu08DstLineAddr3 + j) = *(pu08SrcPixelAddr + 3);
                    pu08SrcPixelAddr += bpp;
                }
                pu08SrcLineAddr += sstep;
                pu08DstLineAddr3 += dstep[3];
            }
        }
    }
    else
    {
        SAL_LOGE("Not supported image format convert, src:0x%x, dst:0x%x\n", pstImageSrc->enImgFmt, pstImageDst->enImgFmt);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    ximg_save_bmp
 * @brief       将图像转换成bmp
 * @param[IN]   XIMAGE_DATA_ST *pstImageData 图像源数据
 * @param[OUT]  void   * const pBmpOutBuf    bmp图像输出内存
 * @param[OUT]  UINT32         *pu32BmpSize  bmp图像尺寸
 * @return      SAL_STATUS SAL_FAIL 失败 SAL_SOK  成功
 */
static SAL_STATUS ximg_save_bmp(XIMAGE_DATA_ST *pstImageData, void * const pBmpOutBuf, UINT32 * const pu32BmpSize)
{
    SAL_STATUS s32Ret = SAL_SOK;
    INT32 bpp = 1;
    UINT32 u32Offset = 0;
    UINT32 u32ImgWidth = 0;
    UINT32 u32ImgHeight = 0;
    DSP_IMG_DATFMT enSrcFmt = DSP_IMG_DATFMT_YUV420SP_VU, enDstFmt = DSP_IMG_DATFMT_YUV420SP_VU;
    XIMAGE_DATA_ST stOutBmpData = {0};

    BITMAPFILEHEADER stFHdr = {0};
    BITMAPINFOHEADER stInfoHdr = {0};

    CHECK_PTR_IS_NULL(pstImageData, SAL_FAIL);
    CHECK_PTR_IS_NULL(pBmpOutBuf, SAL_FAIL);
    CHECK_PTR_IS_NULL(pu32BmpSize, SAL_FAIL);

    enSrcFmt = pstImageData->enImgFmt;
    enDstFmt = DSP_IMG_DATFMT_BGR24;
    u32ImgWidth = pstImageData->u32Width;
    u32ImgHeight = pstImageData->u32Height;

    if (DSP_IMG_DATFMT_BGR24 == enSrcFmt || DSP_IMG_DATFMT_BGRA32 == enSrcFmt)
    {
        enDstFmt = enSrcFmt;
        bpp = ximg_get_bpp(enSrcFmt, 0);
        if (0 > bpp)
        {
            SAL_LOGE("img_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d\n", enSrcFmt, 0, bpp);
            return SAL_FAIL;
        }
    }
    else if (DSP_IMG_DATFMT_YUV420SP_UV == enSrcFmt || DSP_IMG_DATFMT_YUV420SP_VU == enSrcFmt)
    {
        bpp = 3;    // 将yuv转换为bgr格式
    }
    else if (DSP_IMG_DATFMT_SP8 == enSrcFmt)
    {
        bpp = 1;
    }
    else if (DSP_IMG_DATFMT_SP16 == enSrcFmt)
    {
        bpp = 1;
        u32ImgHeight = u32ImgHeight * 2;
    }
    else if (DSP_IMG_DATFMT_LHZP == enSrcFmt)
    {
        bpp = 1;
        u32ImgHeight = u32ImgHeight * 5;    // 将L、H、Z拼成一张图像 
    }
    else
    {
        SAL_LOGE("Not supported image format[0x%x]\n", enSrcFmt);
        return SAL_FAIL;
    }

    /*bmp图像头*/
    stFHdr.bfType = 0x4D42;                                     /* 文件类型头，固定，对应的字符串为BM */
    stFHdr.bfSize = 54 + u32ImgWidth * u32ImgHeight * bpp;      /* 文件大小，包括图像头，图像信息，数据 */
    stFHdr.bfReserved1 = 0;                                     /* 保留字节，一般为0 */
    stFHdr.bfReserved2 = 0;                                     /* 保留字节，一般为0 */
    stFHdr.bfOffBits = 54;                                      /* 从文件开始到数据的偏移大小，包括图像头，图像信息，调色板 */

    /*bmp图像信息*/
    stInfoHdr.biSize = 0x28;                                    /* bmp图像信息大小，sizeof(BITMAPINFOHEADER) */
    stInfoHdr.biWidth = u32ImgWidth;                            /* bmp图像宽 */
    stInfoHdr.biHeight = u32ImgHeight;                          /* bmp图像高 */
    stInfoHdr.biPlanes = 1;                                     /* bmp图像帧数，一般为1 */
    stInfoHdr.biBitCount = bpp * 8;                             /* 像素所占比特数，RGB888占24 bit */
    stInfoHdr.biCompression = 0;                                /* 压缩类型，0表示为压缩 */
    stInfoHdr.biSizeImage = u32ImgWidth * u32ImgHeight * bpp;   /* 真实bmp数据大小，字节为单位，w*h*bpp */
    stInfoHdr.biXPelsPerMeter = 0;                              /* 说明水平分辨率，像素/米表示，一般为0 */
    stInfoHdr.biYPelsPerMeter = 0;                              /* 说明垂直分辨率，像素/米表示，一般为0 */
    stInfoHdr.biClrUsed = 0;                                    /* 颜色使用的个数，如果是0，表示都使用. */
    stInfoHdr.biClrImportant = 0;                               /* 颜色重要的个数，如果是0，表示都重要 */

    /*写入图像头*/
    u32Offset = 0;
    memcpy(pBmpOutBuf, &stFHdr, sizeof(BITMAPFILEHEADER));
    u32Offset += sizeof(BITMAPFILEHEADER);

    /*写入图像信息*/
    memcpy(pBmpOutBuf + u32Offset, &stInfoHdr, sizeof(BITMAPINFOHEADER));
    u32Offset += sizeof(BITMAPINFOHEADER);

    /* 根据输出图像地址构造一张图片 */
    ximg_create(u32ImgWidth, u32ImgHeight, enDstFmt, NULL, pBmpOutBuf + u32Offset, &stOutBmpData);

    /* 写入图像数据 */
    if (DSP_IMG_DATFMT_BGRA32 == enSrcFmt || DSP_IMG_DATFMT_BGR24 == enSrcFmt ||
        DSP_IMG_DATFMT_LHZP == enSrcFmt)
    {
        ximg_flip_vert(pstImageData, &stOutBmpData);
    }
    else if (enSrcFmt & DSP_IMG_DATFMT_YUV420_MASK)
    {
        /* 将yuv转换为rgb */
        s32Ret = ximg_convert_fmt(pstImageData, &stOutBmpData);
        if (SAL_SOK != s32Ret)
        {
            XSP_LOGE("ximg_convert_fmt failed.\n");
            return SAL_FAIL;
        }
        ximg_flip_vert(&stOutBmpData, &stOutBmpData);
    }

    *pu32BmpSize = stFHdr.bfSize;

    return SAL_SOK;
}

/**
 * @function    ximg_save_jpg
 * @brief       将图像转换成jpg,jpg格式时跨距需不小于64
 * @param[IN]   XIMAGE_DATA_ST *pstImageData 图像源数据
 * @param[OUT]  void   * const pJpgOutBuf   jpeg图像输出内存
 * @param[OUT]  UINT32         *pu32JpgSize jpeg图像尺寸
 * @return      SAL_STATUS SAL_FAIL 失败 SAL_SOK  成功
 */
static SAL_STATUS ximg_save_jpg(XIMAGE_DATA_ST *pstImageData, void * const pJpgOutBuf, UINT32 * const pu32JpgSize)
{
    INT32 i = 0;
    INT32 s32WaitTime = 0;  // 等待空闲jpeg句柄总时间
    SAL_STATUS sRet = SAL_SOK;
    UINT32 u32ImgWidth = 0;
    UINT32 u32ImgHeight = 0;
    MEDIA_BUF_ATTR *pstImgMb = NULL;
    /* jpeg编码相关 */
    SAL_VideoFrameParam stHalPrm = {0};
    JPEG_COMMON_ENC_PRM_S stJpegEncPrm = {0};
    XIMG_JPEG_CTRL *pstCurJpegCtrl = NULL;

    static XIMG_JPEG_CTRL stXimgJpegCtrl[JPEG_HANDLE_MAX] = {0};

    /* 等待空闲jpeg编码句柄 */
    for (i = 0; i < JPEG_HANDLE_MAX; i++)
    {
        if (SAL_FALSE == stXimgJpegCtrl[i].bHandleInUse)
        {
            pstCurJpegCtrl = &stXimgJpegCtrl[i];
            pstCurJpegCtrl->bHandleInUse = SAL_TRUE;
            break;
        }
        /* 所有jpeg编码句柄已被占用，等待一定时间后再次搜索可用句柄 */
        if (JPEG_HANDLE_MAX - 1 == i && NULL == pstCurJpegCtrl)
        {
            i = 0;
            s32WaitTime += 15;
            if (600 < s32WaitTime)  // 超时时间600ms
            {
                SAL_LOGE("Waitting for idle jpeg handle timeout[600ms], wait_time:%dms\n", s32WaitTime);
                return SAL_FAIL;
            }
            SAL_msleep(15);
        }
    }

    /* 首次使用时初始化jpeg编码句柄 */
    if (NULL == pstCurJpegCtrl->pJpegHandle)
    {
        stHalPrm.fps = 1;
        stHalPrm.width = XIMAGE_WIDTH_MAX;
        stHalPrm.height = XIMAGE_HEIGHT_MAX;
        stHalPrm.quality = JPEG_ENC_QUALITY;
        stHalPrm.encodeType = MJPEG;
        stHalPrm.dataFormat = SAL_dspDataFormatToSal(pstImageData->enImgFmt);

        sRet = jpeg_drv_createChn((void **)&pstCurJpegCtrl->pJpegHandle, &stHalPrm);
        if (SAL_SOK != sRet)
        {
            XSP_LOGE("jpeg_drv_createChn failed, width: %u, height: %u\n", stHalPrm.width, stHalPrm.height);
            goto EXIT;
        }
        if (0x00 == pstCurJpegCtrl->stJpegSysFrm.uiAppData)
        {
            if (SAL_SOK != sys_hal_allocVideoFrameInfoSt(&pstCurJpegCtrl->stJpegSysFrm))
            {
                XSP_LOGE("sys_hal_allocVideoFrameInfoSt failed\n");
                goto EXIT;
            }
        }
    }

    u32ImgWidth = pstImageData->u32Width;
    u32ImgHeight = pstImageData->u32Height;
    pstImgMb = &pstImageData->stMbAttr;

    /* 构建视频帧信息送jpeg编码 */
    Sal_halBuildSysFrame(&pstCurJpegCtrl->stJpegSysFrm, NULL, pstImageData, NULL);

    stJpegEncPrm.OutWidth = u32ImgWidth;
    stJpegEncPrm.OutHeight = u32ImgHeight;
    stJpegEncPrm.quality = JPEG_ENC_QUALITY;
    stJpegEncPrm.pOutJpeg = (INT8 *)pJpgOutBuf;
    memcpy(&stJpegEncPrm.stSysFrame,  &pstCurJpegCtrl->stJpegSysFrm, sizeof(SYSTEM_FRAME_INFO));
    sRet = jpeg_drv_cropEnc(pstCurJpegCtrl->pJpegHandle, &stJpegEncPrm, NULL);
    if (SAL_SOK != sRet)
    {
        *pu32JpgSize = 0;
        XSP_LOGE("jpeg_drv_cropEnc failed, pVirAddr:%p, phyAddr:%llu, poolId:%u, w:%u, h:%u\n",
                 pstImageData->pPlaneVir[0], pstImgMb->phyAddr, pstImgMb->poolId,
                 stJpegEncPrm.OutWidth, stJpegEncPrm.OutHeight);
        goto EXIT;
    }

    *pu32JpgSize = stJpegEncPrm.outSize;

EXIT:
    pstCurJpegCtrl->bHandleInUse = SAL_FALSE;
    pstCurJpegCtrl = NULL;

    return sRet;
}

/**
 * @function       ximg_save_gif
 * @brief          将图像转换成gif
 * @param[IN]      XIMAGE_DATA_ST *pstImageData 图像源数据
 * @param[OUT]     void   * const pGifOutBuf    gif图像输出内存
 * @param[IN/OUT]  UINT32         *pu32GifSize  输入gif图像内存尺寸，输出gif图像实际尺寸
 * @return         SAL_STATUS SAL_FAIL 失败 SAL_SOK  成功
 */
static SAL_STATUS ximg_save_gif(XIMAGE_DATA_ST *pstImageData, void * const pGifOutBuf, UINT32 * const pu32GifSize)
{
    UINT32 u32ImgWidth = 0;
    UINT32 u32ImgHeight = 0;
    RGB_ATTR_S stRgbAttr = {0};
    GIF_ATTR_S stGifAttr = {0};
    static XIMAGE_DATA_ST stRgbPlanar = {0};

    CHECK_PTR_IS_NULL(pstImageData, SAL_FAIL);
    CHECK_PTR_IS_NULL(pGifOutBuf, SAL_FAIL);
    CHECK_PTR_IS_NULL(pu32GifSize, SAL_FAIL);

    /* 初始化临时rgb平面格式中间内存 */
    /* FIXME：添加互斥锁访问，控制此函数不可重入 */
    if (NULL == stRgbPlanar.pPlaneVir[0])
    {
        stRgbPlanar.stMbAttr.bCached = SAL_FALSE;
        /* RGB部分内存存放rgb平面数据，A部分存放rgb8格式数据 */
        if (SAL_SOK != ximg_create(XIMAGE_WIDTH_MAX, XIMAGE_HEIGHT_MAX, DSP_IMG_DATFMT_BGRAP, "save-gif-rgb", NULL, &stRgbPlanar))
        {
            XSP_LOGE("ximg_create for gif_rgb_planar failed, w:%u, h:%u, fmt:0x%x\n", XIMAGE_WIDTH_MAX, XIMAGE_HEIGHT_MAX, DSP_IMG_DATFMT_BGRA32);
            return SAL_FAIL;
        }
    }

    u32ImgWidth = pstImageData->u32Width;
    u32ImgHeight = pstImageData->u32Height;

    stRgbPlanar.u32Height = u32ImgHeight;
    ximg_set_dimension(&stRgbPlanar, u32ImgWidth, u32ImgWidth, SAL_FALSE, 0x0);

    /*配置RGB属性*/
    stRgbAttr.uiWidth = u32ImgWidth;
    stRgbAttr.uiHeight = u32ImgHeight;
    stRgbAttr.pRgb24Buf[0] = stRgbPlanar.pPlaneVir[0];
    stRgbAttr.pRgb24Buf[1] = stRgbPlanar.pPlaneVir[1];
    stRgbAttr.pRgb24Buf[2] = stRgbPlanar.pPlaneVir[2];
    stRgbAttr.u64PhyAddr = stRgbPlanar.stMbAttr.phyAddr;  // yuv转rgb格式时使用
    stRgbAttr.pRgb8Buf = stRgbPlanar.pPlaneVir[3];

    /*配置GIF属性*/
    stGifAttr.uiWidth = u32ImgWidth;
    stGifAttr.uiHeight = u32ImgHeight;
    stGifAttr.uiWriteId = 0;
    stGifAttr.uiSizeMax = *pu32GifSize; // 此处由外层传入，使用完后重置，再保存实际图像大小
    stGifAttr.pGifBuf = pGifOutBuf;

    *pu32GifSize = 0;   // 重置为0

    /* 源数据为yuv格式时：yuv转rgb平面格式，源数据为rgb格式时，rgb交叉转平面格式 */
    if (SAL_SOK != ximg_convert_fmt(pstImageData, &stRgbPlanar))
    {
        SAL_LOGE("ximg_convert_fmt failed\n");
        return SAL_FAIL;
    }

    if (GIF_OK != Gif_DrvRgb2Gif(&stRgbAttr, &stGifAttr))
    {
        XSP_LOGE("Gif_DrvRgb2Gif failed\n");
        return SAL_FAIL;
    }

    *pu32GifSize = stGifAttr.uiWriteId;

    return SAL_SOK;
}

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
SAL_STATUS ximg_save(XIMAGE_DATA_ST *pstImageData, XRAY_TRANS_TYPE enSaveType, void * const pOutBuf, UINT32 * const pu32OutDataSize, UINT32 u32FillColor)
{
    SAL_STATUS ret = SAL_SOK;
    UINT32 u32ImgWidth = 0, u32AlignedImgWidth = 0;
    CHECK_PTR_IS_NULL(pstImageData, SAL_FAIL);
    CHECK_PTR_IS_NULL(pOutBuf, SAL_FAIL);
    CHECK_PTR_IS_NULL(pu32OutDataSize, SAL_FAIL);

    if (SAL_SOK != ximg_verify_size(pstImageData, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }
    if (pstImageData->enImgFmt != DSP_IMG_DATFMT_BGRA32 && pstImageData->enImgFmt != DSP_IMG_DATFMT_BGR24 && 
        pstImageData->enImgFmt != DSP_IMG_DATFMT_YUV420SP_VU && pstImageData->enImgFmt != DSP_IMG_DATFMT_YUV420SP_UV && 
        pstImageData->enImgFmt != DSP_IMG_DATFMT_SP8 && pstImageData->enImgFmt != DSP_IMG_DATFMT_SP16 && pstImageData->enImgFmt != DSP_IMG_DATFMT_LHZP)
    {
        SAL_LOGE("Unsupported image format 0x%x\n", pstImageData->enImgFmt);
        return SAL_FAIL;
    }

    u32ImgWidth = pstImageData->u32Width;

    /* BMP和GIF在ximg_convert_yuv2rgb中，在SOFTWARE将yuv转rgb时做了数据范围拓展,与显示颜色保持一致 */
    if (XRAY_TRANS_BMP == enSaveType)
    {
        ret = ximg_save_bmp(pstImageData, pOutBuf, pu32OutDataSize);
    }
    else if (XRAY_TRANS_JPG == enSaveType)
    {
        if (DSP_IMG_DATFMT_BGRA32 == pstImageData->enImgFmt || DSP_IMG_DATFMT_BGR24 == pstImageData->enImgFmt)
        {
            /* rgb在rk或x86平台使用，rk平台jpg编码时stride需要64对齐 */
            if (0 != pstImageData->u32Stride[0] % 64)
            {
                ximg_set_dimension(pstImageData, u32ImgWidth, SAL_align(u32ImgWidth, 64) , SAL_TRUE, u32FillColor);
            }
        }
        else if (pstImageData->enImgFmt & DSP_IMG_DATFMT_YUV420_MASK)
        {
            u32AlignedImgWidth = (0 != u32ImgWidth % 16) ? SAL_align(u32ImgWidth, 16) : u32ImgWidth;

            /* yuv在nt平台使用，nt平台jpg编码时不支持stride，将图像内存排布连续 */
            if (u32AlignedImgWidth <= pstImageData->u32Stride[0])
            {
                ximg_set_dimension(pstImageData, u32AlignedImgWidth, u32AlignedImgWidth, SAL_TRUE, u32FillColor);
            }
            else
            {
                SAL_LOGE("Unsupported image dimension, width[%u] need to be aligned to 16, but alinged width[%u] > stride[%u]\n", 
                         u32ImgWidth, u32AlignedImgWidth, pstImageData->u32Stride[0]);
                return SAL_FAIL;
            }

            /* 将YUV数据从16-235的ximg_yc_stretch到0-255,与显示颜色、bmp和gif颜色保持一致 */
            if (SAL_SOK != ximg_yc_stretch(pstImageData))
            {
                XSP_LOGW("ximg_yc_stretch failed, w:%u,h:%u\n", pstImageData->u32Width, pstImageData->u32Height);
                return SAL_FAIL;
            }
        }
        else
        {
            SAL_LOGE("Unsupported image format 0x%x\n", pstImageData->enImgFmt);
            return SAL_FAIL;
        }
        ret = ximg_save_jpg(pstImageData, pOutBuf, pu32OutDataSize);
    }
    else if (XRAY_TRANS_GIF == enSaveType)
    {
        /* NT平台yuv转rgb时不支持stride，将图像内存排布连续 */
        ximg_set_dimension(pstImageData, pstImageData->u32Width, pstImageData->u32Width, SAL_TRUE, 0);
        ret = ximg_save_gif(pstImageData, pOutBuf, pu32OutDataSize);
    }
    else
    {
        SAL_LOGE("Unsupported image save type 0x%x\n", enSaveType);
        return SAL_FAIL;
    }

    if (SAL_SOK != ret)
    {
        SAL_LOGE("ximg_save_type:%d failed, w:%u, h:%u, s:%u\n", enSaveType, pstImageData->u32Width, pstImageData->u32Height, pstImageData->u32Stride[0]);
    }

    return ret;
}

/**
 * @function   ximg_saveAs
 * @brief      将原始图像数据存储为图片格式，调试中
 * @param[IN]  XIMAGE_DATA_ST  *pstImageData   输入原始图片数据
 * @param[IN]  XRAY_TRANS_TYPE *enSaveType     目标图片格式
 * @param[OUT] void    * const pOutBuf         目标图片存储内存
 * @param[OUT] UINT32  * const pu32OutDataSize 目标图片尺寸
 * @return     SAL_STATUS SAL_FAIL 失败 SAL_SOK 成功
 */
SAL_STATUS ximg_saveAs(XIMAGE_DATA_ST *pstImageData, XRAY_TRANS_TYPE enSaveType, UINT32 chan, CHAR *dumpDir, CHAR *sTag, UINT32 u32NoTag)
{
    CHAR dumpName[128] = {0};
    static void *pOutBuf = NULL;
    UINT32 u32OutDataSize = 0;
    CHAR cTypeSuffix[XRAY_TRANS_BUTT][64] = {"bmp", "jpg", "gif", "png", "tiff"};

    if (NULL == pOutBuf)
    {
        pOutBuf = malloc(1920*1000*3);
    }

    ximg_save(pstImageData, enSaveType, pOutBuf, &u32OutDataSize, 0);

    if (XRAY_TRANS_BUTT < enSaveType)
    {
        return SAL_FAIL;
    }

    if (NULL != sTag)
    {
        snprintf(dumpName, sizeof(dumpName), "%s/%s-ch%u_no%u_w%u_h%u_s%u_t%llu.%s", 
                 dumpDir, sTag, chan, u32NoTag, pstImageData->u32Width, pstImageData->u32Height, pstImageData->u32Stride[0], sal_get_tickcnt(), cTypeSuffix[enSaveType]);
    }
    else
    {
        snprintf(dumpName, sizeof(dumpName), "%s/ch%u_no%u_w%u_h%u_s%u_t%llu.%s", 
                 dumpDir, chan, u32NoTag, pstImageData->u32Width, pstImageData->u32Height, pstImageData->u32Stride[0], sal_get_tickcnt(), cTypeSuffix[enSaveType]);
    }

    SAL_WriteToFile(dumpName, 0, SEEK_SET, pOutBuf, u32OutDataSize);

    return SAL_SOK;
}

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
SAL_STATUS ximg_dump(XIMAGE_DATA_ST *pstImageData, UINT32 chan, CHAR *dumpDir, CHAR *sTagPre, CHAR *sTagPost, UINT32 u32NoTag)
{
    CHAR dumpName[256] = {0};
    CHAR pu8CommonName[128] = {0};
    SAL_STATUS ret = SAL_SOK;
    INT32 s32WriteRet = 0;
    INT32 i = 0, j = 0, bpp = 1;
    INT32 s32Whence = SEEK_SET;     // 文件开始写入的位置
    FILE *fp = NULL;

    UINT32 u32DataWidth = 0;
    UINT32 u32DataHeight = 0;
    UINT32 u32DataStride = 0;
    UINT32 sstep = 0;               // 源数据行步长，单位：字节
    UINT32 u32LineDataCpyByte = 0;      // 按行拷贝字节数
    UINT32 u32AllDataCpyByte = 0;       // 一次性拷贝字节数
    VOID *pDataVir = NULL;
    DSP_IMG_DATFMT dataFmt = DSP_IMG_DATFMT_YUV420SP_VU;

    CHECK_PTR_IS_NULL(pstImageData, SAL_FAIL);
    CHECK_PTR_IS_NULL(pstImageData->pPlaneVir[0], SAL_FAIL);
    if (SAL_SOK != ximg_verify_size(pstImageData, __FUNCTION__, __LINE__))
    {
        return SAL_FAIL;
    }

    dataFmt = pstImageData->enImgFmt;
    u32DataWidth = pstImageData->u32Width;
    u32DataHeight = pstImageData->u32Height;
    u32DataStride = pstImageData->u32Stride[0];

    if (NULL != sTagPre && NULL != sTagPost)
    {
        snprintf(pu8CommonName, sizeof(pu8CommonName), "%s/%s-ch%u_no%05u_w%u_h%u_s%u_t%llu-%s", 
                dumpDir, sTagPre, chan, u32NoTag, u32DataWidth, u32DataHeight, u32DataStride, sal_get_tickcnt(), sTagPost);
    }
    else if (NULL != sTagPre && NULL == sTagPost)
    {
        snprintf(pu8CommonName, sizeof(pu8CommonName), "%s/%s-ch%u_no%05u_w%u_h%u_s%u_t%llu", 
                dumpDir, sTagPre, chan, u32NoTag, u32DataWidth, u32DataHeight, u32DataStride, sal_get_tickcnt());
    }
    else if (NULL == sTagPre && NULL != sTagPost)
    {
        snprintf(pu8CommonName, sizeof(pu8CommonName), "%s/ch%u_no%05u_w%u_h%u_s%u_t%llu-%s", 
                dumpDir, chan, u32NoTag, u32DataWidth, u32DataHeight, u32DataStride, sal_get_tickcnt(), sTagPost);
    }
    else
    {
        snprintf(pu8CommonName, sizeof(pu8CommonName), "%s/ch%u_no%05u_w%u_h%u_s%u_t%llu", 
                dumpDir, chan, u32NoTag, u32DataWidth, u32DataHeight, u32DataStride, sal_get_tickcnt());
    }

    switch (dataFmt)
    {
    case DSP_IMG_DATFMT_YUV420SP_UV:
        snprintf(dumpName, sizeof(dumpName), "%s.nv12", pu8CommonName);
        break;
    case DSP_IMG_DATFMT_YUV420SP_VU:
        snprintf(dumpName, sizeof(dumpName), "%s.nv21", pu8CommonName);
        break;
    case DSP_IMG_DATFMT_BGRP:
        snprintf(dumpName, sizeof(dumpName), "%s.bgrp", pu8CommonName);
        break;
    case DSP_IMG_DATFMT_BGRAP:
        snprintf(dumpName, sizeof(dumpName), "%s.bgrap", pu8CommonName);
        break;
    case DSP_IMG_DATFMT_BGR24:
        snprintf(dumpName, sizeof(dumpName), "%s.rgb24", pu8CommonName);
        break;
    case DSP_IMG_DATFMT_BGRA32:
        snprintf(dumpName, sizeof(dumpName), "%s.bgra32", pu8CommonName);
        break;
    case DSP_IMG_DATFMT_SP16:
        snprintf(dumpName, sizeof(dumpName), "%s.raw", pu8CommonName);
        break;
    case DSP_IMG_DATFMT_SP8:
        snprintf(dumpName, sizeof(dumpName), "%s.raw", pu8CommonName);
        break;
    case DSP_IMG_DATFMT_LHP:
        snprintf(dumpName, sizeof(dumpName), "%s.lh", pu8CommonName);
        break;
    case DSP_IMG_DATFMT_LHZP:
        snprintf(dumpName, sizeof(dumpName), "%s.lhz8", pu8CommonName);
        break;
    case DSP_IMG_DATFMT_LHZ16P:
        snprintf(dumpName, sizeof(dumpName), "%s.lhz16", pu8CommonName);
        break;
    default:
        SAL_LOGE("Not supported image format[0x%x]\n", dataFmt);
        return SAL_FAIL;
    }

    for (i = 0; i < XIMAGE_PLANE_MAX; i++)
    {
        /* 依次拷贝各个平面 */
        pDataVir = pstImageData->pPlaneVir[i];
        u32DataStride = pstImageData->u32Stride[i];

        if (NULL == pDataVir || 0 == u32DataStride)
        {
            break;
        }

        bpp = ximg_get_bpp(dataFmt, i);
        if (0 > bpp)
        {
            SAL_LOGE("ximg_get_bpp failed, fmt:0x%x, plane idx:%d, bpp:%d\n", dataFmt, i, bpp);
            return SAL_FAIL;
        }
        if ((DSP_IMG_DATFMT_YUV420SP_VU == dataFmt || DSP_IMG_DATFMT_YUV420SP_UV == dataFmt) && 1 == i)
        {
            /* UV平面按单像素占两字节计算，宽高均为Y平面一半 */
            u32DataWidth >>= 1;
            u32DataHeight >>= 1;
            u32DataStride >>= 1;
        }
        sstep = u32DataStride * bpp;
        u32LineDataCpyByte = u32DataWidth * bpp;

        s32Whence = (0 == i) ? SEEK_SET : SEEK_END;
        if (u32DataWidth == u32DataStride)
        {
            /* 源和目标的数据宽度和宽距均相等，数据是连续的，直接一次写入 */
            u32AllDataCpyByte = u32LineDataCpyByte * u32DataHeight;
            ret = SAL_WriteToFile(dumpName, 0, s32Whence, pDataVir, u32AllDataCpyByte);
        }
        else
        {
            /* 数据不连续时，按行分别拷贝 */
            ret = SAL_WriteToFile(dumpName, 0, s32Whence, pDataVir, u32LineDataCpyByte);
            pDataVir += sstep;

            fp = fopen(dumpName, "a");
            if (NULL == fp)
            {
                SAL_LOGE("open %s failed, mode: %s, errno:%d, %s\n", dumpName, "a", errno, strerror(errno));
                return SAL_FAIL;
            }
            ret = fseek(fp, 0, SEEK_END);
            if (0 == ret)
            {
                s32WriteRet = u32LineDataCpyByte;
                for (j = 1; j < u32DataHeight && s32WriteRet == u32LineDataCpyByte; j++)
                {
                    s32WriteRet = fwrite(pDataVir, sizeof(U08), u32LineDataCpyByte, fp);
                    pDataVir += sstep;
                }
            }
            fflush(fp);
            fclose(fp);
        }
    }

    if (SAL_SOK != ret)
    {
        SAL_LOGW("Write to file error, format:0x%x, w:%u, h:%u, s:%u, i:%d, j:%d, errno:%d, %s\n", dataFmt, u32DataWidth, u32DataHeight, u32DataStride, i, j, errno, strerror(errno));
        return SAL_FAIL;
    }
    return SAL_SOK;
}

/**
 * @function    ximg_print
 * @brief       打印图像信息
 * @param[IN]   XIMAGE_DATA_ST *pstImageData 需要保存的图像
 * @param[IN]   CHAR           *sTag         用于打印的字符串标签，可为NULL
 * @param[OUT]  None
 * @return      SAL_STATUS SAL_SOK-成功，SAL_FAIL-失败
 */
SAL_STATUS ximg_print(XIMAGE_DATA_ST *pstImage, CHAR *sTag)
{
    MEDIA_BUF_ATTR *pstMbAttr = NULL;
    CHECK_PTR_IS_NULL(pstImage, SAL_FAIL);

    pstMbAttr = &pstImage->stMbAttr;

    SAL_print("%s, format:0x%x, w:%u, h:%u, s:[%u %u %u %u], plane[%p %p %p %p]\n"
              "mbAttr[phy:0x%x, vir:%p, size:%u, poolId:%u, blk:%llu, bCache:%d]\n",
              (NULL == sTag) ? "" : sTag, pstImage->enImgFmt, pstImage->u32Width, pstImage->u32Height, 
              pstImage->u32Stride[0], pstImage->u32Stride[1], pstImage->u32Stride[2], pstImage->u32Stride[3], 
              pstImage->pPlaneVir[0], pstImage->pPlaneVir[1], pstImage->pPlaneVir[2], pstImage->pPlaneVir[3], 
              (UINT32)pstMbAttr->phyAddr, pstMbAttr->virAddr, pstMbAttr->memSize, pstMbAttr->poolId, pstMbAttr->u64MbBlk, pstMbAttr->bCached);

    return SAL_SOK;
}