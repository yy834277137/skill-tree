/**
 * @file   fmtConvert_hisi.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  把平台无关的像素格式与海思平台hi3559a之间的像素格式转换
 * @author yeyanzhong
 * @date   2021.7.29
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#include "fmtConvert_ssV5.h"

static const PIXEL_FORMAT_MAP_S astPixelFmtMap[] =
{
    {SAL_VIDEO_DATFMT_YUV422SP_UV,  OT_PIXEL_FORMAT_YUV_SEMIPLANAR_422},
    {SAL_VIDEO_DATFMT_YUV422SP_VU,  OT_PIXEL_FORMAT_YVU_SEMIPLANAR_422},
    {SAL_VIDEO_DATFMT_YUV422P,      OT_PIXEL_FORMAT_YVU_PLANAR_422},

    {SAL_VIDEO_DATFMT_YUV420SP_UV,  OT_PIXEL_FORMAT_YUV_SEMIPLANAR_420},
    {SAL_VIDEO_DATFMT_YUV420SP_VU,  OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420},
    {SAL_VIDEO_DATFMT_YUV420P,      OT_PIXEL_FORMAT_YVU_PLANAR_420},
};

/**
 * @function    fmtConvert_hisi_sal2hisi
 * @brief       平台无关的像素格式转换成海思平台的像素格式
 * @param[in]   enSalPixelFmt 平台无关的像素格式
 * @param[out]  penHiPixelFmt 海思平台的像素格式
 * @return
 */
INT32 fmtConvert_ssV5_sal2plat(SAL_VideoDataFormat enSalPixelFmt, ot_pixel_format *penHiPixelFmt)
{
    UINT32 i = 0;
    UINT32 u32Num = 0;

    u32Num = sizeof(astPixelFmtMap) / sizeof(astPixelFmtMap[0]);

    for (i = 0; i < u32Num; i++)
    {
        if (enSalPixelFmt == astPixelFmtMap[i].enSalPixelFmt)
        {
            *penHiPixelFmt = astPixelFmtMap[i].enHiPixelFmt;
            return SAL_SOK;
        }
    }

    SYS_LOGE("enSalPixelFmt %d is not support! \n", enSalPixelFmt);
    return SAL_FAIL;

}

/**
 * @function    fmtConvert_hisi_hisi2sal
 * @brief       海思平台的像素格式转换成平台无关的像素格式
 * @param[in]   enHiPixelFmt 海思平台的像素格式
 * @param[out]  penSalPixelFmt 平台无关的像素格式
 * @return
 */
INT32 fmtConvert_ssV5_plat2sal(ot_pixel_format enHiPixelFmt, SAL_VideoDataFormat *penSalPixelFmt)
{
    UINT32 i = 0;
    UINT32 u32Num = 0;

    u32Num = sizeof(astPixelFmtMap) / sizeof(astPixelFmtMap[0]);

    for (i = 0; i < u32Num; i++)
    {
        if (enHiPixelFmt == astPixelFmtMap[i].enHiPixelFmt)
        {
            *penSalPixelFmt = astPixelFmtMap[i].enSalPixelFmt;
            return SAL_SOK;
        }
    }

    SYS_LOGE("enHiPixelFmt %d is not support! \n", enHiPixelFmt);
    return SAL_FAIL;
}

