/*** 
 * @file   color_space.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  颜色空间转换
 * @author 许朝斌,杨彭举
 * @date   2022-03-02
 * @note   
 * @note History:
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <string.h>
#include "color_space.h"
#include "color_data.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */



/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/**************************************************************************************************
* 功  能：颜色空间转换，yuv转hsv的基础函数
* 参  数：IN:
*           y                              -  y通道的像素值
*           u                              -  u通道的像素值
*           v                              -  v通道的像素值
        OUT:
*           h_val                          -  h通道的像素值
*           s_val                          -  s道的像素值
*           v_val                          -  v通道的像素值
* 返回值：无
* 备  注:
**************************************************************************************************/
static int VCA_cvt_yuv2hsv(unsigned char y,
                           unsigned char u,
                           unsigned char v,
                           unsigned char *h_val,
                           unsigned char *s_val,
                           unsigned char *v_val)
{
    unsigned char r, g, b;
    /* int rdif,bdif, invgdif; */
    int dif_val;
    int h_tmp, s_tmp, v_tmp;
    int v_min;
    int vr, vg;

    if ((!h_val) || (!s_val) || (!v_val))
    {
        return -1;
    }

    /* 先将YUV转成RGB */
    CVT_YUV2RGB(y, u, v, r, g, b);

    v_tmp = b;
    v_min = b;

    /* 获取RGB最大最小值 */
    v_tmp = SAL_MAX(v_tmp, g);
    v_tmp = SAL_MAX(v_tmp, r);
    v_min = SAL_MIN(v_min, g);
    v_min = SAL_MIN(v_min, r);

    dif_val = v_tmp - v_min;
    vr = v_tmp == r ? -1 : 0;
    vg = v_tmp == g ? -1 : 0;

    /* 倒数使用查表 */
    s_tmp = dif_val * (div_table[v_tmp] >> 12);

    /* 计算h，需要根据rgb值进行选择 */
    h_tmp = ((vr & (g - b)))            /* 红色值最大 */
            + /* 绿色值最大 */
            ((((~vr) & ((vg & (b - r + 2 * dif_val))))
              + /* 蓝色值最大 */
              (((~vg) & (r - g + 4 * dif_val)))));
    h_tmp = (h_tmp * hdiv_table180[dif_val] + (1 << 11)) >> 12;
    h_tmp += h_tmp < 0 ? 180 : 0;       /* hue的范围为[0,180] */

    /* 计算HSV */
    *h_val = (unsigned char)h_tmp;
    *s_val = (unsigned char)s_tmp;
    *v_val = (unsigned char)v_tmp;

    return 0;
}

/**************************************************************************************************
* 功  能：颜色空间转换，像素级别转换
* 参  数：IN:
*           src1                           -  第一通道的像素值
*           src2                           -  第二通道的像素值
*           src3                           -  第三通道的像素值
*           code                           -  转换标示
*         OUT:
*           dst1                           -  第一通道的像素值
*           dst2                           -  第二通道的像素值
*           dst3                           -  第三通道的像素值
* 返回值：无
* 备  注:
**************************************************************************************************/
void VCA_cvt_color_pix(unsigned char src1,
                       unsigned char src2,
                       unsigned char src3,
                       unsigned char *dst1,
                       unsigned char *dst2,
                       unsigned char *dst3,
                       unsigned int code)
{
    if ((!dst1) || (!dst2) || (!dst3))
    {
        return;
    }

    switch (code)
    {
        case VCA_PIX_YUV_RGB:
            CVT_YUV2RGB(src1, src2, src3, *dst1, *dst2, *dst3);
            break;

        case VCA_PIX_RGB_YUV:
            CVT_RGB2YUV(src1, src2, src3, *dst1, *dst2, *dst3);
            break;

        case VCA_PIX_YUV_HSV:
            VCA_cvt_yuv2hsv(src1, src2, src3, dst1, dst2, dst3);
            break;

        default:
            break;
    }
}

/**************************************************************************************************
* 功  能：颜色空间转换，YUV转RGB，图像级别
* 参  数：IN:
*           src1                           -  第一通道的像素值
*           src2                           -  第二通道的像素值
*           src3                           -  第三通道的像素值
*           img_w                          -  图像宽度
*           img_h                          -  图像高度
*         OUT:
*           dst1                           -  第一通道的像素值
*           dst2                           -  第二通道的像素值
*           dst3                           -  第三通道的像素值
* 返回值：无
* 备  注:
**************************************************************************************************/
void VCA_cvt_YUV2RGB_img(unsigned char *src1,
                         unsigned char *src2,
                         unsigned char *src3,
                         unsigned char *dst1,
                         unsigned char *dst2,
                         unsigned char *dst3,
                         unsigned int img_w, unsigned int img_h)
{
    unsigned int i, j, img_size;

    img_size = img_w * img_h;

    for (i = 0; i < img_size; i += 2)
    {
        CVT_YUV2RGB(src1[i], src2[i], src3[i], dst1[i], dst2[i], dst3[i]);
        j = i + 1;
        CVT_YUV2RGB(src1[j], src2[j], src3[j], dst1[j], dst2[j], dst3[j]);
    }
}

/**************************************************************************************************
* 功  能：颜色空间转换，YUV转HSV，图像级别
* 参  数：IN:
*           src1                           -  第一通道的像素值
*           src2                           -  第二通道的像素值
*           src3                           -  第三通道的像素值
*           img_w                          -  图像宽度
*           img_h                          -  图像高度
*         OUT:
*           dst1                           -  第一通道的像素值
*           dst2                           -  第二通道的像素值
*           dst3                           -  第三通道的像素值
* 返回值：无
* 备  注:
**************************************************************************************************/
void VCA_cvt_YUV2HSV_img(unsigned char *src1,
                         unsigned char *src2,
                         unsigned char *src3,
                         unsigned char *dst1,
                         unsigned char *dst2,
                         unsigned char *dst3,
                         unsigned int img_w,
                         unsigned int img_h)
{
    unsigned int i, j, img_size;

    img_size = img_w * img_h;

    for (i = 0; i < img_size; i += 2)
    {
        VCA_cvt_yuv2hsv(src1[i], src2[i], src3[i], &dst1[i], &dst2[i], &dst3[i]);
        j = i + 1;
        VCA_cvt_yuv2hsv(src1[j], src2[j], src3[j], &dst1[j], &dst2[j], &dst3[j]);
    }
}

/**************************************************************************************************
* 功  能：颜色空间转换，RGB转YUV，图像级别
* 参  数：IN:
*           src1                           -  第一通道的像素值
*           src2                           -  第二通道的像素值
*           src3                           -  第三通道的像素值
*           img_w                          -  图像宽度
*           img_h                          -  图像高度
*         OUT:
*           dst1                           -  第一通道的像素值
*           dst2                           -  第二通道的像素值
*           dst3                           -  第三通道的像素值
* 返回值：无
* 备  注:
**************************************************************************************************/
void VCA_cvt_RGB2YUV_img(unsigned char *src1,
                         unsigned char *src2,
                         unsigned char *src3,
                         unsigned char *dst1,
                         unsigned char *dst2,
                         unsigned char *dst3,
                         unsigned int img_w,
                         unsigned int img_h)
{
    unsigned int i, j, img_size;

    img_size = img_w * img_h;

    for (i = 0; i < img_size; i += 2)
    {
        CVT_RGB2YUV(src1[i], src2[i], src3[i], dst1[i], dst2[i], dst3[i]);
        j = i + 1;
        CVT_RGB2YUV(src1[j], src2[j], src3[j], dst1[j], dst2[j], dst3[j]);
    }
}

/**************************************************************************************************
* 功  能：颜色空间转换，YUV（420）格式转为RGB，图像级别
* 参  数：IN:
*           src1                           -  第一通道的像素值
*           src2                           -  第二通道的像素值
*           src3                           -  第三通道的像素值
*           img_w                          -  图像宽度
*           img_h                          -  图像高度
*         OUT:
*           dst1                           -  第一通道的像素值
*           dst2                           -  第二通道的像素值
*           dst3                           -  第三通道的像素值
* 返回值：无
* 备  注:
**************************************************************************************************/
static void VCA_cvt_YUV4202RGB_img(unsigned char *src1,
                                   unsigned char *src2,
                                   unsigned char *src3,
                                   unsigned char *dst1,
                                   unsigned char *dst2,
                                   unsigned char *dst3,
                                   unsigned int img_w,
                                   unsigned int img_h)
{
    unsigned int i, j, oft1, oft2;
    unsigned int qimg_w;

    qimg_w = img_w >> 1;
    oft1 = 0;
    oft2 = 0;

    for (i = 0; i < img_h; i++)
    {
        oft1 = i * img_w;
        oft2 = (i >> 1) * qimg_w;

        for (j = 0; j < img_w; j += 2)
        {
            CVT_YUV2RGB(src1[oft1], src2[oft2], src3[oft2], \
                        dst1[oft1], dst2[oft1], dst3[oft1]);
            CVT_YUV2RGB(src1[oft1 + 1], src2[oft2], src3[oft2], \
                        dst1[oft1 + 1], dst2[oft1 + 1], dst3[oft1 + 1]);
            oft1 += 2;
            oft2++;
        }
    }
}

/**************************************************************************************************
* 功  能：颜色空间转换，YUV（420）格式转为HSV，图像级别
* 参  数：IN:
*           src1                           -  第一通道的像素值
*           src2                           -  第二通道的像素值
*           src3                           -  第三通道的像素值
*           img_w                          -  图像宽度
*           img_h                          -  图像高度
*         OUT:
*           dst1                           -  第一通道的像素值
*           dst2                           -  第二通道的像素值
*           dst3                           -  第三通道的像素值
* 返回值：无
* 备  注:
**************************************************************************************************/
void VCA_cvt_YUV4202HSV_img(unsigned char *src1,
                            unsigned char *src2,
                            unsigned char *src3,
                            unsigned char *dst1,
                            unsigned char *dst2,
                            unsigned char *dst3,
                            unsigned int img_w,
                            unsigned int img_h)
{
    unsigned int i, j, oft1, oft2;
    unsigned int qimg_w;

    qimg_w = img_w >> 1;
    oft1 = 0;
    oft2 = 0;

    for (i = 0; i < img_h; i++)
    {
        oft1 = i * img_w;
        oft2 = (i >> 1) * qimg_w;

        for (j = 0; j < img_w; j += 2)
        {
            VCA_cvt_yuv2hsv(src1[oft1], src2[oft2], src3[oft2], \
                            &dst1[oft1], &dst2[oft1], &dst3[oft1]);
            VCA_cvt_yuv2hsv(src1[oft1 + 1], src2[oft2], src3[oft2], \
                            &dst1[oft1 + 1], &dst2[oft1 + 1], &dst3[oft1 + 1]);
            oft1 += 2;
            oft2++;
        }
    }
}

/**************************************************************************************************
* 功  能：颜色空间转换，YUV（420，uv通道交织）格式转为RGB，图像级别
* 参  数：IN:
*           src1                           -  第一通道的像素值
*           src2                           -  第二,三通道的像素值
*           img_w                          -  图像宽度
*           img_h                          -  图像高度
*         OUT:
*           dst1                           -  第一通道的像素值
*           dst2                           -  第二通道的像素值
*           dst3                           -  第三通道的像素值
* 返回值：无
* 备  注:
**************************************************************************************************/
void VCA_cvt_YUV420MV2RGB_img(unsigned char *src1,
                              unsigned char *src2,
                              unsigned char *dst1,
                              unsigned char *dst2,
                              unsigned char *dst3,
                              unsigned int img_w,
                              unsigned int img_h)
{
    unsigned int i, j, oft1, oft2;

    oft1 = 0;
    oft2 = 0;

    for (i = 0; i < img_h; i++)
    {
        oft1 = i * img_w;
        oft2 = (i >> 1) * img_w;

        for (j = 0; j < img_w; j += 2)
        {
            CVT_YUV2RGB(src1[oft1], src2[oft2], src2[oft2 + 1], \
                        dst1[oft1], dst2[oft1], dst3[oft1]);
            CVT_YUV2RGB(src1[oft1 + 1], src2[oft2 + 2], src2[oft2 + 3], \
                        dst1[oft1 + 1], dst2[oft1 + 1], dst3[oft1 + 1]);
            oft1 += 2;
            oft2 += 2;
        }
    }
}

/**************************************************************************************************
* 功  能：颜色空间转换，YUV（420，uv通道交织）格式转为HSV，图像级别
* 参  数：IN:
*           src1                           -  第一通道的像素值
*           src2                           -  第二,三通道的像素值
*           img_w                          -  图像宽度
*           img_h                          -  图像高度
*         OUT:
*           dst1                           -  第一通道的像素值
*           dst2                           -  第二通道的像素值
*           dst3                           -  第三通道的像素值
* 返回值：无
* 备  注:
**************************************************************************************************/
void VCA_cvt_YUV420MV2HSV_img(unsigned char *src1,
                              unsigned char *src2,
                              unsigned char *dst1,
                              unsigned char *dst2,
                              unsigned char *dst3,
                              unsigned int img_w,
                              unsigned int img_h)
{
    unsigned int i, j, oft1, oft2;

    oft1 = 0;
    oft2 = 0;

    for (i = 0; i < img_h; ++i)
    {
        oft1 = i * img_w;
        oft2 = (i >> 1) * img_w;

        for (j = 0; j < img_w; j += 2)
        {
            VCA_cvt_yuv2hsv(src1[oft1], src2[oft2], src2[oft2 + 1], \
                            &dst1[oft1], &dst2[oft1], &dst3[oft1]);
            VCA_cvt_yuv2hsv(src1[oft1 + 1], src2[oft2 + 2], src2[oft2 + 3], \
                            &dst1[oft1 + 1], &dst2[oft1 + 1], &dst3[oft1 + 1]);
            oft1 += 2;
            oft2 += 2;
        }
    }
}

/**************************************************************************************************
* 功  能：颜色空间转换，图像级别
* 参  数：IN:
*           src1                           -  第一通道的像素值
*           src2                           -  第二通道的像素值
*           src2                           -  第三通道的像素值
*           img_w                          -  图像宽度
*           img_h                          -  图像高度
*           code                           -  图像转换标示
*         OUT:
*           dst1                           -  第一通道的像素值
*           dst2                           -  第二通道的像素值
*           dst3                           -  第三通道的像素值
* 返回值：无
* 备  注:
**************************************************************************************************/
void VCA_cvt_color_img(unsigned char *src1,
                       unsigned char *src2,
                       unsigned char *src3,
                       unsigned char *dst1,
                       unsigned char *dst2,
                       unsigned char *dst3,
                       unsigned int img_w,
                       unsigned int img_h,
                       unsigned int code)
{
    switch (code)
    {

        case VCA_IMG_YUV420_RGB:
            VCA_cvt_YUV4202RGB_img(src1, src2, src3, dst1, dst2, dst3, img_w, img_h);
            break;

        case VCA_IMG_YUV420MV_RGB:
            VCA_cvt_YUV420MV2RGB_img(src1, src2, dst1, dst2, dst3, img_w, img_h);
            break;

        case VCA_IMG_YUV_RGB:
            VCA_cvt_YUV2RGB_img(src1, src2, src3, dst1, dst2, dst3, img_w, img_h);
            break;

        case VCA_IMG_RGB_YUV:
            VCA_cvt_RGB2YUV_img(src1, src2, src3, dst1, dst2, dst3, img_w, img_h);
            break;

        case VCA_IMG_YUV_HSV:
            VCA_cvt_YUV2HSV_img(src1, src2, src3, dst1, dst2, dst3, img_w, img_h);
            break;

        case VCA_IMG_YUV420_HSV:
            VCA_cvt_YUV4202HSV_img(src1, src2, src3, dst1, dst2, dst3, img_w, img_h);
            break;

        case VCA_IMG_YUV420MV_HSV:
            VCA_cvt_YUV420MV2HSV_img(src1, src2, dst1, dst2, dst3, img_w, img_h);
            break;

        default:
            break;
    }
}

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
void SAL_I420ToNV12(unsigned char *pSrcYuv, unsigned char *pDstYuv, unsigned int width, unsigned int hight)
{
    unsigned int i = 0;
    unsigned int Ysize = width * hight;
    unsigned int UVsize = width * hight / 4;

    memcpy(pDstYuv, pSrcYuv, width * hight);

    for (i = 0; i < UVsize; i++)
    {
        pDstYuv[Ysize + 2 * i + 1] = pSrcYuv[Ysize + i];
        pDstYuv[Ysize + 2 * i] = pSrcYuv[Ysize + UVsize + i];
    }
}

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
void SAL_BGRCross2BGRPlane(unsigned char *bgr_cross, unsigned char *bgr_plane, unsigned int height, unsigned int width)
{
    int h = 0;
    int w = 0;

    for (h = 0; h < height; h++)
    {
        for (w = 0; w < width; w++)
        {
            bgr_plane[0 * height * width + h * width + w] = bgr_cross[h * width * 3 + w * 3 + 0]; /* b */
            bgr_plane[1 * height * width + h * width + w] = bgr_cross[h * width * 3 + w * 3 + 1]; /* g */
            bgr_plane[2 * height * width + h * width + w] = bgr_cross[h * width * 3 + w * 3 + 2]; /* r */
        }
    }
}

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
void SAL_BGR2YUV420(unsigned char *pDstYUV, unsigned int frame_w, unsigned int frame_h, unsigned char *pSrcBbgrPlane)
{
    int i, j;
    unsigned char *y, *u, *v, *r, *g, *b;

    int height2 = frame_h / 2;
    int width2 = frame_w / 2;

    int yuv_height = height2 * 2;
    int yuv_width = width2 * 2;

    for (i = 0; i < height2; ++i)
    {
        for (j = 0; j < width2; ++j)
        {
            /* (0, 0) */
            y = pDstYUV + (i * 2 + 0) * yuv_width + j * 2 + 0;
            u = pDstYUV + yuv_height * yuv_width + i * yuv_width + j * 2;
            v = u + 1;
            b = pSrcBbgrPlane + (i * 2 + 0) * frame_w + j * 2 + 0;
            g = b + frame_h * frame_w;
            r = g + frame_h * frame_w;

            VCA_cvt_color_pix(*r, *g, *b, y, u, v, VCA_PIX_RGB_YUV);

            /* (0, 1) */
            y = pDstYUV + (i * 2 + 0) * yuv_width + j * 2 + 1;
            b = pSrcBbgrPlane + (i * 2 + 0) * frame_w + j * 2 + 1;
            g = b + frame_h * frame_w;
            r = g + frame_h * frame_w;

            CVT_RGB2Y(*r, *g, *b, *y);

            /* (1, 0) */
            y = pDstYUV + (i * 2 + 1) * yuv_width + j * 2 + 0;
            b = pSrcBbgrPlane + (i * 2 + 1) * frame_w + j * 2 + 0;
            g = b + frame_h * frame_w;
            r = g + frame_h * frame_w;

            CVT_RGB2Y(*r, *g, *b, *y);

            /* (1, 1) */
            y = pDstYUV + (i * 2 + 1) * yuv_width + j * 2 + 1;
            b = pSrcBbgrPlane + (i * 2 + 1) * frame_w + j * 2 + 1;
            g = b + frame_h * frame_w;
            r = g + frame_h * frame_w;

            CVT_RGB2Y(*r, *g, *b, *y);
        }
    }
}

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
void SAL_NV21ToBGRY(unsigned char *pSrcYuv, int w, int h, unsigned char *pDstBgry)
{
    int count = w;
    int i = 0;
    int r, g, b;
    unsigned char *y = NULL;
    unsigned char *uv = NULL;
    unsigned char _y;
    unsigned char _u;
    unsigned char _v;
    unsigned char *dstB = NULL;
    unsigned char *dstG = NULL;
    unsigned char *dstR = NULL;
    unsigned char *dstY = NULL;

    if ((NULL == pSrcYuv) || (NULL == pDstBgry))
    {
        return;
    }

    dstB = (unsigned char *)pDstBgry;
    dstG = (unsigned char *)(pDstBgry + w * h);
    dstR = (unsigned char *)(pDstBgry + w * h * 2);
    dstY = (unsigned char *)(pDstBgry + w * h * 3);

    for (i = 0; i < h; ++i)
    {
        y = pSrcYuv + w * i;
        uv = pSrcYuv + w * h + w * (i / 2);
        count = w;

        while (count > 1)
        {
            _y = y[0];
            _u = uv[0];
            _v = uv[1];

            CVT_YUV2RGB(_y, _u, _v, r, g, b);
            *dstB = b;
            *dstG = g;
            *dstR = r;
            *dstY = _y;

            dstB++;
            dstG++;
            dstR++;
            dstY++;

            y++;
            _y = y[0];

            CVT_YUV2RGB(_y, _u, _v, r, g, b);
            *dstB = b;
            *dstG = g;
            *dstR = r;
            *dstY = _y;

            dstB++;
            dstG++;
            dstR++;
            dstY++;

            y++;
            uv += 2;

            count -= 2;
        }

        if (count > 0)
        {
            _y = y[0];
            _u = uv[0];
            _v = uv[1];

            CVT_YUV2RGB(_y, _u, _v, r, g, b);
            *dstB = b;
            *dstG = g;
            *dstR = r;
            *dstY = _y;

            dstB++;
            dstG++;
            dstR++;
            dstY++;
        }
    }
}

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
int SAL_I420To16Align(unsigned int *pWidth, unsigned int *pHeight, char *pSrc, char *pDst)
{
    unsigned int width = 0, height = 0;
    char *pYdata = NULL;
    char *pUdata = NULL;
    char *pVdata = NULL;

    char *pSrcTmp = NULL;
    char *pDstTmp = NULL;

    unsigned int Flag = 0;
    int i = 0;

    if ((NULL == pWidth) || (NULL == pHeight) || (NULL == pSrc) || (NULL == pDst))
    {
        /* printf("input param empty\n"); */
        return SAL_FAIL;
    }

    width = *pWidth;
    height = *pHeight;
    pYdata = pSrc;
    pUdata = pSrc + (width * height);
    pVdata = pUdata + (width * height) / 4;

    /* 判断对齐 */
    if (width % 16 != 0)
    {
        Flag = Flag | 0x1;
        width = width & (~0xF);
    }

    if (height % 16 != 0)
    {
        Flag = Flag | 0x2;
        height = height & (~0xF);
    }

    /* 数据对齐，仅高不对齐，去掉最下面不对齐的部分，宽不对齐需要逐行去掉 */
    if (0x2 == Flag)
    {
        /* 仅高不对齐 */
        pSrcTmp = pYdata;
        pDstTmp = pDst;
        memcpy(pDstTmp, pSrcTmp, (width * height)); /* Y */

        pSrcTmp = pUdata;
        pDstTmp += width * height;
        memcpy(pDstTmp, pSrcTmp, (width * height) / 4); /* U */

        pSrcTmp = pVdata;
        pDstTmp += (width * height) / 4;
        memcpy(pDstTmp, pSrcTmp, (width * height) / 4); /* V */
    }
    else
    {
        /* Y */
        pSrcTmp = pYdata;
        pDstTmp = pDst;
        for (i = 0; i < height; i++)
        {
            memcpy(pDstTmp, pSrcTmp, width); /* Y */
            pDstTmp += width;
            pSrcTmp += (*pWidth);
        }

        /* UV */
        pSrcTmp = pUdata;
        for (i = 0; i < height; i++)
        {

            memcpy(pDstTmp, pSrcTmp, width / 2);
            pDstTmp += width / 2;
            pSrcTmp += (*pWidth) / 2;
        }
    }

    *pWidth = width;
    *pHeight = height;
    return SAL_SOK;

}

/*******************************************************************************
* 函数名  : SAL_NV21To16Align
* 描  述  :
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
int SAL_NV21To16Align(unsigned int *pWidth, unsigned int *pHeight, char *pSrc, char *pDst)
{
    unsigned int width = 0, height = 0;
    char *pYdata = NULL;
    char *pUVdata = NULL;

    char *pSrcTmp = NULL;
    char *pDstTmp = NULL;

    unsigned int Flag = 0;
    int i = 0;

    if ((NULL == pWidth) || (NULL == pHeight) || (NULL == pSrc) || (NULL == pDst))
    {
        /* printf("input param empty\n"); */
        return SAL_FAIL;
    }

    width = *pWidth;
    height = *pHeight;
    pYdata = pSrc;
    pUVdata = pSrc + (width * height);


    /* 判断对齐 */
    if (width % 16 != 0)
    {
        Flag = Flag | 0x1;
        width = width & (~0xF);
    }

    if (height % 16 != 0)
    {
        Flag = Flag | 0x2;
        height = height & (~0xF);
    }

    /* 数据对齐，仅高不对齐，去掉最下面不对齐的部分，宽不对齐需要逐行去掉 */
    if (0x2 == Flag)
    {
        /* 仅高不对齐 */
        pSrcTmp = pYdata;
        pDstTmp = pDst;
        memcpy(pDstTmp, pSrcTmp, (width * height)); /* Y */

        pSrcTmp = pUVdata;
        pDstTmp += width * height;
        memcpy(pDstTmp, pSrcTmp, (width * height) / 2); /* UV */

    }
    else
    {
        /* Y */
        pSrcTmp = pYdata;
        pDstTmp = pDst;
        for (i = 0; i < height; i++)
        {
            memcpy(pDstTmp, pSrcTmp, width); /* Y */
            pDstTmp += width;
            pSrcTmp += (*pWidth);
        }

        /* UV */
        pSrcTmp = pUVdata;
        for (i = 0; i < height / 2; i++)
        {

            memcpy(pDstTmp, pSrcTmp, width);
            pDstTmp += width;
            pSrcTmp += (*pWidth);
        }
    }

    *pWidth = width;
    *pHeight = height;
    return SAL_SOK;

}

/*******************************************************************************
* 函数名  : SAL_RGB24ToARGB1555
* 描  述  :
* 输  入  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
inline int SAL_RGB24ToARGB1555(UINT32 u32Rgb24, UINT32 *pu32Argb1555, UINT32 u32Alpha)
{
    *pu32Argb1555 = (((u32Rgb24 >> 19) & 0x1f) << 10) | (((u32Rgb24 >> 11) & 0x1f) << 5) | ((u32Rgb24 & 0xf8) >> 3);
    if (u32Alpha > 0)
    {
        *pu32Argb1555 |= (1 << 15);
    }

    return SAL_SOK;
}

