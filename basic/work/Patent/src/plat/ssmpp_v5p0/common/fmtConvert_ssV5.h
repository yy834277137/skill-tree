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
#include "sal.h"
#include "platform_sdk.h"

typedef struct
{
    SAL_VideoDataFormat enSalPixelFmt;  /* 平台无关的像素格式 */
    ot_pixel_format enHiPixelFmt;        /* 海思平台的像素格式 */
} PIXEL_FORMAT_MAP_S;

/**
 * @function    fmtConvert_hisi_sal2hisi
 * @brief       平台无关的像素格式转换成海思平台的像素格式
 * @param[in]   enSalPixelFmt 平台无关的像素格式
 * @param[out]  penHiPixelFmt 海思平台的像素格式
 * @return
 */
INT32 fmtConvert_ssV5_sal2plat(SAL_VideoDataFormat enSalPixelFmt, ot_pixel_format *penHiPixelFmt);

/**
 * @function    fmtConvert_hisi_hisi2sal
 * @brief       海思平台的像素格式转换成平台无关的像素格式
 * @param[in]   enHiPixelFmt 海思平台的像素格式
 * @param[out]  penSalPixelFmt 平台无关的像素格式
 * @return
 */
INT32 fmtConvert_ssV5_plat2sal(ot_pixel_format enHiPixelFmt, SAL_VideoDataFormat *penSalPixelFmt);
