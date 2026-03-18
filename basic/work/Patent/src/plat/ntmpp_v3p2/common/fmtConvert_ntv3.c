/**
 * @file   fmtConvert_ntv3.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  把平台无关的像素格式与NT平台之间的像素格式转换
 * @author liuxianying
 * @date   2021.11.26
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#include "fmtConvert_ntv3.h"

static const PIXEL_FORMAT_MAP_S astPixelFmtMap[] =
{
    {SAL_VIDEO_DATFMT_YUV422SP_UV,  HD_VIDEO_PXLFMT_YUV422_UYVY},
    {SAL_VIDEO_DATFMT_YUV422SP_VU,  HD_VIDEO_PXLFMT_YUV422_VYUY},
    {SAL_VIDEO_DATFMT_YUV422P,      HD_VIDEO_PXLFMT_YUV422_PLANAR},

    {SAL_VIDEO_DATFMT_YUV420SP_UV,  HD_VIDEO_PXLFMT_YUV420},
    {SAL_VIDEO_DATFMT_YUV420P,      HD_VIDEO_PXLFMT_YUV420_PLANAR},
};

/**
 * @function    fmtConvert_ntv3_sal2plat
 * @brief       NT无关的像素格式转换成海思平台的像素格式
 * @param[in]   enSalPixelFmt 平台无关的像素格式
 * @param[out]  penHiPixelFmt 海思平台的像素格式
 * @return
 */
INT32 fmtConvert_ntv3_sal2plat(SAL_VideoDataFormat enSalPixelFmt, HD_VIDEO_PXLFMT *penHiPixelFmt)
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
 * @function    fmtConvert_ntv3_plat2sal
 * @brief       NT平台的像素格式转换成平台无关的像素格式
 * @param[in]   enHiPixelFmt 海思平台的像素格式
 * @param[out]  penSalPixelFmt 平台无关的像素格式
 * @return
 */
INT32 fmtConvert_ntv3_plat2sal(HD_VIDEO_PXLFMT enHiPixelFmt, SAL_VideoDataFormat *penSalPixelFmt)
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

