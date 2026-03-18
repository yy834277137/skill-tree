/**
 * @file   sal_video.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  视频通用功能
 * @author liwenbin
 * @date   2022/7/6
 * @note
 * @note \n History
   1.日    期: 2022/7/6
     作    者: liwenbin
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "sal.h"
#include "dspcommon.h"

#line __LINE__ "sal_video.c"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */


/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/**
 * @function   SAL_dspDataFormatToSal
 * @brief      转换应用下发的格式到内部格式
 * @param[in]  DSP_IMG_DATFMT enImgDataFormat  应用格式,在dspcommon中定义
 * @param[out] None
 * @return     SAL_VideoDataFormat 返回内部格式
 */
SAL_VideoDataFormat SAL_dspDataFormatToSal(DSP_IMG_DATFMT enImgDataFormat)
{
    SAL_VideoDataFormat pixFmt = SAL_VIDEO_DATFMT_MAX;

    switch (enImgDataFormat)
    {
    case DSP_IMG_DATFMT_BGR24:
        pixFmt = SAL_VIDEO_DATFMT_RGB24_888;
        break;

    case DSP_IMG_DATFMT_BGRA32:
        pixFmt = SAL_VIDEO_DATFMT_ARGB_8888;
        break;

    case DSP_IMG_DATFMT_YUV420SP_VU:
        pixFmt = SAL_VIDEO_DATFMT_YUV420SP_VU;
        break;

    case DSP_IMG_DATFMT_YUV420SP_UV:
        pixFmt = SAL_VIDEO_DATFMT_YUV420SP_UV;
        break;

    case DSP_IMG_DATFMT_YUV420P:
        pixFmt = SAL_VIDEO_DATFMT_YUV420P;
        break;

    case DSP_IMG_DATFMT_SP16:
        pixFmt = SAL_VIDEO_DATFMT_BAYER_RAW_DUAL;
        break;

    case DSP_IMG_DATFMT_LHZP:
        pixFmt = SAL_VIDEO_DATFMT_BAYER_RAW_DUAL;
        break;

    default:
        SAL_LOGE("unsupport this image data format: 0x%x\n", enImgDataFormat);
        break;
    }

    return pixFmt;
}


/**
 * @function   SAL_salDataFormatToDsp
 * @brief      转换内部格式到应用格式
 * @param[in]  SAL_VideoDataFormat enImgDataFormat  内部格式
 * @param[out] None
 * @return     DSP_IMG_DATFMT 返回应用格式
 */
DSP_IMG_DATFMT SAL_salDataFormatToDsp(SAL_VideoDataFormat enImgDataFormat)
{
    DSP_IMG_DATFMT pixFmt = DSP_IMG_DATFMT_UNKNOWN;

    switch (enImgDataFormat)
    {
    case SAL_VIDEO_DATFMT_RGB24_888:
        pixFmt = DSP_IMG_DATFMT_BGR24;
        break;

    case SAL_VIDEO_DATFMT_ARGB_8888:
        pixFmt = DSP_IMG_DATFMT_BGRA32;
        break;

    case SAL_VIDEO_DATFMT_YUV420SP_VU:
        pixFmt = DSP_IMG_DATFMT_YUV420SP_VU;
        break;

    case SAL_VIDEO_DATFMT_YUV420SP_UV:
        pixFmt = DSP_IMG_DATFMT_YUV420SP_UV;
        break;

    case SAL_VIDEO_DATFMT_YUV420P:
        pixFmt = DSP_IMG_DATFMT_YUV420P;
        break;

    default:
        SAL_LOGE("unsupport this image data format: 0x%x\n", enImgDataFormat);
        break;
    }

    return pixFmt;
}


UINT32 SAL_getBitsPerPixel(SAL_VideoDataFormat enSalPixelFmt)
{
    UINT32 u32Bits = 0;
    
    switch (enSalPixelFmt)
    {
        case SAL_VIDEO_DATFMT_YUV420SP_VU: 
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
            u32Bits = 12;
            break;

        case SAL_VIDEO_DATFMT_YUV422SP_VU: 
        case SAL_VIDEO_DATFMT_YUV422SP_UV:
            u32Bits = 16;
            break;
        
        case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
            u32Bits = 8;
            break;
        
        case SAL_VIDEO_DATFMT_RGB24_888:
        case SAL_VIDEO_DATFMT_BGR24_888:
            u32Bits = 24;
            break;

        case SAL_VIDEO_DATFMT_ARGB_8888:
        case SAL_VIDEO_DATFMT_RGBA_8888:
            u32Bits = 32;
            break;

        default:
            SAL_LOGE("can't support image      format:%d\n", enSalPixelFmt);
            u32Bits = 12;
            break;
    }

    return u32Bits;
}


