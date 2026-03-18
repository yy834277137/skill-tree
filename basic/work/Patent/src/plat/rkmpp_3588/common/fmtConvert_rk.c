/**
 * @file   fmtConvert_rk.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  把平台无关的像素格式与海思平台rk之间的像素格式转换
 * @author liuxianying
 * @date   2022.03.08
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#include "fmtConvert_rk.h"

#line __LINE__ "fmtConvert_rk.c"

static const PIXEL_FORMAT_MAP_S astPixelFmtMap[] =
{
    {SAL_VIDEO_DATFMT_YUV422SP_UV,  RK_FMT_YUV422SP},
    {SAL_VIDEO_DATFMT_YUV422SP_VU,  RK_FMT_YUV422SP_VU},
    {SAL_VIDEO_DATFMT_YUV422P,      RK_FMT_YUV422P},

    {SAL_VIDEO_DATFMT_YUV420SP_UV,  RK_FMT_YUV420SP},
    {SAL_VIDEO_DATFMT_YUV420SP_VU,  RK_FMT_YUV420SP_VU},
    {SAL_VIDEO_DATFMT_YUV420P,      RK_FMT_YUV420P},

    /* 底层硬核模块采用大端方式 */
    {SAL_VIDEO_DATFMT_ARGB_8888,    RK_FMT_BGRA8888},
    {SAL_VIDEO_DATFMT_BGRA_8888,    RK_FMT_ARGB8888},
    {SAL_VIDEO_DATFMT_RGB24_888,    RK_FMT_BGR888},
    {SAL_VIDEO_DATFMT_BGR24_888,    RK_FMT_RGB888},
};

/**
 * @function    fmtConvert_rk_sal2rk
 * @brief       平台无关的像素格式转换成海思平台的像素格式
 * @param[in]   enSalPixelFmt 平台无关的像素格式
 * @param[out]  penHiPixelFmt 海思平台的像素格式
 * @return
 */
INT32 fmtConvert_rk_sal2rk(SAL_VideoDataFormat enSalPixelFmt, PIXEL_FORMAT_E *penHiPixelFmt)
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
 * @function    fmtConvert_rk_rk2sal
 * @brief       海思平台的像素格式转换成平台无关的像素格式
 * @param[in]   enHiPixelFmt 海思平台的像素格式
 * @param[out]  penSalPixelFmt 平台无关的像素格式
 * @return
 */
INT32 fmtConvert_rk_rk2sal(PIXEL_FORMAT_E enHiPixelFmt, SAL_VideoDataFormat *penSalPixelFmt)
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

