/*** 
 * @file   color_space.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  颜色空间转换
 * @author 许朝斌,杨彭举
 * @date   2022-02-24
 * @note   
 * @note History:
 */

#ifndef _COLOR_SPACE_H_
#define _COLOR_SPACE_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal_type.h"
#include "sal_macro.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define CVT_YUV2RGB(y, u, v, r, g, b)  \
    r = SAL_MAX(0, SAL_MIN(R_Y[y] + R_V[v], 255)); \
    g = SAL_MAX(0, SAL_MIN(R_Y[y] - G_U[u] - G_V[v], 255)); \
    b = SAL_MAX(0, SAL_MIN(R_Y[y] + B_U[u], 255));

#define CVT_RGB2YUV(r, g, b, y, u, v)  \
    y = SAL_MAX(0, SAL_MIN(16 + Y_R[r] + Y_G[g] + Y_B[b], 255)); \
    u = SAL_MAX(0, SAL_MIN(128 - U_R[r] - U_G[g] + U_B[b], 255)); \
    v = SAL_MAX(0, SAL_MIN(128 + V_R[r] - V_G[g] - V_B[b], 255));

#define CVT_RGB2Y(r, g, b, y) y = SAL_MAX(0, SAL_MIN(16 + Y_R[(r)] + Y_G[(g)] + Y_B[(b)], 255));

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
typedef enum _COLOR_CVT_TYPE_
{
    /* 像素级别转换 */
    VCA_PIX_RGB_YUV = 0x00,
    VCA_PIX_YUV_RGB = 0x01,
    VCA_PIX_YUV_HSV = 0x02,
    VCA_PIX_HSV_YUV = 0x03,

    /* 图像级别转换 */
    VCA_IMG_YUV420_RGB = 0x10,
    VCA_IMG_YUV420MV_RGB = 0x11,
    VCA_IMG_YUV_RGB = 0x12,
    VCA_IMG_RGB_YUV = 0x13,

    VCA_IMG_YUV420_HSV = 0x20,
    VCA_IMG_YUV420MV_HSV = 0x21,
    VCA_IMG_YUV_HSV = 0x22,
    VCA_IMG_HSV_YUV = 0x23
} COLOR_CVT_TYPE;

/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : SAL_I420ToNV12
* 描  述  : YUV I420 转换到 NV12格式
* 输  入  : - pSrcYuv: 交积在一起的I420格式，原图
*         : - pDstYuv: Y面和UV面的NV12格式，目标图
*         : - Width  : 图像宽高
*         : - Hight  :

   yyyy                yyyy
   uu       ==>        uv
   vv                  uv

* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
void SAL_I420ToNV12(unsigned char *pSrcYuv, unsigned char *pDstYuv, unsigned int width, unsigned int hight);

/*******************************************************************************
* 函数名  : SAL_BGRCross2BGRPlane
* 描  述  : 把 BGR 交积图像转换成RGB三个面
* 输  入  : - bgr_cross: 原图
*         : - bgr_plane: 目标图
*         : - height   : 图像宽高
*         : - width    :
* 输  出  : bgr_plane
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
void SAL_BGRCross2BGRPlane(unsigned char *bgr_cross, unsigned char *bgr_plane, unsigned int height, unsigned int width);

/*******************************************************************************
* 函数名  : SAL_BGR2YUV420
* 描  述  : 把BGR三个面的图像转换为 YUV420交积图
* 输  入  : - pDstYUV       : 输出的YUV420图像
*         : - frame_w       : 图像宽高
*         : - frame_h       :
*         : - pSrcBbgrPlane : 输入的BGR分面图片
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
void SAL_BGR2YUV420(unsigned char *pDstYUV, unsigned int frame_w, unsigned int frame_h, unsigned char *pSrcBbgrPlane);

/*******************************************************************************
* 函数名  : SAL_NV21ToBGRY
* 描  述  : NV12图像转换为 BGRY 四个独立面的源图像
* 输  入  : - pSrcYuv : NV12 的YUV图像
*         : - w       : 图像宽高
*         : - h       :
*         : - pDstBgry: BRGY 独立面面的目标图像
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
void SAL_NV21ToBGRY(unsigned char *pSrcYuv, int w, int h, unsigned char *pDstBgry);

/*******************************************************************************
* 函数名  : SAL_I420To16Align
* 描  述  : YUVI420 16位对齐 存储方式为YYYY U V
* 输  入  : - Width : 原始高
*         : - Height: 原始宽
*         : - pSrc  :
*         : - pDst  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
   修改历史       :
   1.日      期   : 2018年11月12日
   作      者   : liuyun10
   修改内容   : 新生成函数
*******************************************************************************/
int SAL_I420To16Align(unsigned int *pWidth, unsigned int *pHeight, char *pSrc, char *pDst);

/*******************************************************************************
* 函数名  : SAL_RGB24ToARGB1555
* 描  述  :
* 输  入  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
int SAL_RGB24ToARGB1555(UINT32 u32Rgb24, UINT32 *pu32Argb1555, UINT32 u32Alpha);

#endif

