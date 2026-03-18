/**
 * @file   fmtConvert_hisi.c
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
#include "sal.h"
#include "platform_sdk.h"

typedef struct
{
    SAL_VideoDataFormat enSalPixelFmt;  /* 平台无关的像素格式 */
    HD_VIDEO_PXLFMT enHiPixelFmt;        /* 海思平台的像素格式 */
} PIXEL_FORMAT_MAP_S;

/**
 * @function    fmtConvert_ntv3_sal2plat
 * @brief       平台无关的像素格式转换成海思平台的像素格式
 * @param[in]   enSalPixelFmt 平台无关的像素格式
 * @param[out]  penHiPixelFmt 海思平台的像素格式
 * @return
 */
INT32 fmtConvert_ntv3_sal2plat(SAL_VideoDataFormat enSalPixelFmt, HD_VIDEO_PXLFMT *penHiPixelFmt);

/**
 * @function    fmtConvert_ntv3_plat2sal
 * @brief       海思平台的像素格式转换成平台无关的像素格式
 * @param[in]   enHiPixelFmt 海思平台的像素格式
 * @param[out]  penSalPixelFmt 平台无关的像素格式
 * @return
 */
INT32 fmtConvert_ntv3_plat2sal(HD_VIDEO_PXLFMT enHiPixelFmt, SAL_VideoDataFormat *penSalPixelFmt);
