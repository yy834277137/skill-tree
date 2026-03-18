/*******************************************************************************
* vgs_hal.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2018年03月13日 Create
*
* Description : 硬件平台 vgs 模块
* Modification:
*******************************************************************************/
#include "platform_hal.h"
#include "vgs_drv_api.h"
#include "color_data.h"
#include "plat_com_inter_hal.h"
/*****************************************************************************
                               宏定义
*****************************************************************************/

/*****************************************************************************
                               结构体定义
*****************************************************************************/
typedef struct
{
    UINT32  u32Bpp;
    UINT32  u32Stride;
    UINT32  u32Width;
    UINT32  u32Thick;      /* 线宽 */
    UINT32  u32Height;
    FLOAT32 f32XFract;
    UINT32 *pstColor;
    UINT8  *pu8YAddr;
}ARGB_LINE_PARAM_S;


/*****************************************************************************
                               全局变量
*****************************************************************************/


/*****************************************************************************
                                ~{:/J}~}
*****************************************************************************/

// TODO: 一些函数不适合放到该文件中，后续需要整理相应的函数到对应文件中

static inline VOID vgs_drvFillColor2X2(UINT8 *pu8AddrY, UINT16 *pu16AddrVU, UINT32 u32FrameWidth, 
                                                UINT8 u8ColorY, UINT16 u16ColorVU)
{
    *pu8AddrY                       = u8ColorY;
    *(pu8AddrY + 1)                 = u8ColorY;
    *(pu8AddrY + u32FrameWidth)     = u8ColorY;
    *(pu8AddrY + u32FrameWidth + 1) = u8ColorY;
    *pu16AddrVU                     = u16ColorVU;

    return;
}
/*******************************************************************************
* 函数名  : vgs_drvDrawLineHorSoft
* 描  述  : 该函数用来申请使用CPU画横线
* 输  入  : - pu8YAddr     : Y分量地址
*         : - pu8UVAddr : UV分量地址
*         : - u32Stride   : 图像跨距
*         : - u32Height : 线高度
*         : - u32Thick： 线宽
*         : - pstColor： YUV颜色分量值
*         : - pstLine ： 显示框画线类型
*         : - f32XFract：x坐标小数部分
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败

*******************************************************************************/
static INT32 vgs_drvDrawLineHorSoft(UINT8 *pu8YAddr, UINT8 *pu8UVAddr, UINT32 u32Stride,UINT32 u32PicWidth, UINT32 u32Width, UINT32 u32Thick, COLOR_YUV_S *pstColor, DISPLINE_PRM *pstLine)
{
    UINT32 w = 0, h = 0, i = 0;
    UINT32 u32FullLen  = pstLine->fulllinelength;
    UINT32 u32NodeNum  = pstLine->node;
    UINT32 u32DotGap   = (pstLine->gaps - 2 * u32NodeNum) / (u32NodeNum + 1);    // 默认点的长度为2
    UINT32 u32DrawNum  = 0;
    UINT32 u32WidthTmp = u32FullLen + u32NodeNum * 2 + (u32NodeNum + 1) * u32DotGap;
    UINT32 u32DrawLen  = 0;
    UINT8 *pu8YTmp     = NULL;
    UINT16 *pu16VUTmp  = NULL;
    UINT16 u16VU       = (((UINT16)pstColor->u8U << 8) & 0xFF00) | (UINT16)pstColor->u8V;   // 同时写UV比分开写效率高

    if ((((UINT64)(pu8YAddr)) & 0x01) != 0)
    {
        pu8YAddr++;
        pu8UVAddr++;
        u32Width = SAL_alignDown(u32Width, 2);
    }

    for (h = 0; h < u32Thick; h += 2)
    {
        pu8YTmp   = pu8YAddr + u32Stride * h;
        pu16VUTmp = (UINT16 *)(pu8UVAddr + u32Stride * h / 2);
        for (w = 0; w < u32Width; w += u32WidthTmp)
        {
            u32DrawLen = ((u32Width - w) > u32FullLen) ? u32FullLen : (u32Width - w);

            /* 填充Y分量 */
            memset(pu8YTmp, pstColor->u8Y, u32DrawLen);
            memset(pu8YTmp + u32Stride, pstColor->u8Y, u32DrawLen);

            /* 填充VU分量 */
            for (i = 0; i < u32DrawLen / 2; i++)
            {
                *(pu16VUTmp + i) = u16VU;
            }

            u32DrawNum = (u32Width - w - u32DrawLen) / (u32DotGap + 2);
            u32DrawNum = (u32DrawNum > u32NodeNum) ? u32NodeNum : u32DrawNum;
            
            pu8YTmp   += (u32DotGap + u32DrawLen);
            pu16VUTmp += ((u32DotGap + u32DrawLen) / 2);
            for (i = 0; i < u32DrawNum; i++)
            {
                (VOID)vgs_drvFillColor2X2(pu8YTmp, pu16VUTmp, u32PicWidth, pstColor->u8Y, u16VU);
                pu8YTmp   += (u32DotGap + 2);
                pu16VUTmp += (u32DotGap / 2 + 1);
            }
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : vgs_drvDrawLineVerSoft
* 描  述  : 该函数用来申请使用CPU画竖线线
* 输  入  : - pu8YAddr     : Y分量地址
*         : - pu8UVAddr : UV分量地址
*         : - u32Stride   : 图像跨距
*         : - u32Height : 线高度
*         : - u32Thick： 线宽
*         : - pstColor： YUV颜色分量值
*         : - pstLine ： 显示框画线类型
*         : - f32XFract：x坐标小数部分
* 输  出  :
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 vgs_drvDrawLineVerSoft(UINT8 *pu8YAddr, UINT8 *pu8UVAddr, UINT32 u32Stride, UINT32 u32Height, UINT32 u32Thick, COLOR_YUV_S *pstColor, DISPLINE_PRM *pstLine, FLOAT32 f32XFract, BOOL bSnap)
{
    UINT32 w = 0, h = 0, i = 0, j = 0;
    UINT32 u32FullLen    = pstLine->fulllinelength;
    UINT32 u32NodeNum    = pstLine->node;
    UINT32 u32DotGap     = (pstLine->gaps - 2 * u32NodeNum) / (u32NodeNum + 1);    // 默认点的长度为2
    UINT32 u32DrawNum    = 0;
    UINT32 u32HeightTmp  = u32FullLen + u32NodeNum * 2 + (u32NodeNum + 1) * u32DotGap;
    UINT32 u32DrawLen    = 0, u32TmpDrawLen = 0;
    UINT32 u32VUNum      = 0;
    UINT64 u64UVAddr     = (UINT64)(pu8UVAddr);
    UINT8 *pu8YTmp       = NULL;
    UINT16 *pu16VUTmp    = NULL;
    UINT16 u16VU         = (((UINT16)pstColor->u8U << 8) & 0xFF00) | (UINT16)pstColor->u8V;   // 同时写UV比分开写效率高
    BOOL bUseFract       = ((f32XFract > 1) || (f32XFract < 0)) ? SAL_FALSE : SAL_TRUE;
    UINT8 *pu8UvTmp = 0;
    
    if ((u64UVAddr % 2) != 0)
    {
        pu16VUTmp = (UINT16 *)(u64UVAddr + 1);
        u32VUNum   = (4 > u32Thick ) ? 1 : (u32Thick / 2 - 1);
    }
    else
    {
        pu16VUTmp = (UINT16 *)(u64UVAddr);
        u32VUNum   = u32Thick / 2;
    }

    for (h = 0; h < u32Height; h += u32HeightTmp)
    {
        u32DrawLen = ((u32Height - h) > u32FullLen) ? u32FullLen : (u32Height - h);
        u32TmpDrawLen = u32DrawLen / 2;
        /* 画实线 */
        for (i = 0; i < u32TmpDrawLen; i++)
        {
            /* 填充Y分量 */
            if ((u64UVAddr % 2) != 0 && (u32Thick <= 2) && SAL_TRUE == bSnap)
            {
                for (j = 0; j < 2; j ++)
                {
                    pu8YTmp =  pu8YAddr - 1;
                    memset(pu8YTmp, pstColor->u8Y, u32Thick);
                    pu8YAddr += u32Stride;
                }
            }
            else
            {
                if (SAL_TRUE == bUseFract)
                {
                    for (j = 0; j < 2; j ++)
                    {
                        pu8YTmp  = pu8YAddr;
                        *pu8YTmp = (1 - f32XFract) * pstColor->u8Y + f32XFract * (*pu8YTmp);
                        pu8YTmp++;
                        memset(pu8YTmp, pstColor->u8Y, u32Thick - 2);
                        pu8YTmp  += (u32Thick - 2);
                        *pu8YTmp  = f32XFract * pstColor->u8Y + (1 - f32XFract) * (*pu8YTmp);
                        pu8YAddr += u32Stride;
                    }
                }
                else
                {
                    for (j = 0; j < 2; j ++)
                    {
                        memset(pu8YAddr, pstColor->u8Y, u32Thick);
                        pu8YAddr += u32Stride;
                    }
                }
            }
        }
        for (i = 0; i < u32TmpDrawLen; i++)
        {
            /* 填充VU分量 */
            if ((u64UVAddr % 2) != 0 && (u32Thick <= 2))
            {
                if (SAL_TRUE == bSnap )
                {
                    pu16VUTmp = (UINT16 *)(pu8UVAddr - 1);
                    for (w = 0; w < u32VUNum; w++)
                    {
                        *(pu16VUTmp + w) = u16VU;
                    }
                    pu8UVAddr +=  u32Stride;
                }
                else
                {
                    /*当启始坐标为奇数时uv填充4个字节2个uv值按权重虚化线条否者会出现闪烁*/
                    pu8UvTmp =  pu8UVAddr - 1;
                    *pu8UvTmp = ((1 - f32XFract) * pstColor->u8V) + f32XFract * (*pu8UvTmp);
                    pu8UvTmp++;
                    *pu8UvTmp = ((1 - f32XFract) * pstColor->u8U) + f32XFract * (*pu8UvTmp);
                    pu8UvTmp++;
                    *pu8UvTmp = ((f32XFract) * pstColor->u8V) + (1-f32XFract) * (*pu8UvTmp);
                    pu8UvTmp++;
                    *pu8UvTmp = ((f32XFract) * pstColor->u8U) + (1-f32XFract) * (*pu8UvTmp);
                    pu8UVAddr +=  u32Stride;
                }

            }
            else
            {
                for (w = 0; w < u32VUNum; w++)
                {
                    *(pu16VUTmp + w) = u16VU;
                }
            }
            pu16VUTmp += (u32Stride / 2);
        }

        pu8YAddr  += u32Stride * u32DotGap;
        pu8UVAddr += (u32Stride * u32DotGap) >> 1;
        pu16VUTmp += (u32Stride * u32DotGap) / 4;
        
        u32DrawNum = (u32Height - h - u32DrawLen) / (u32DotGap + 2);
        u32DrawNum = (u32DrawNum > u32NodeNum) ? u32NodeNum : u32DrawNum;
        /* 画点 */
        for (i = 0; i < u32DrawNum; i++)
        {
            /* 填充Y分量 画点和画线保持一致*/
            if ((u64UVAddr % 2) != 0 && (u32Thick <= 2) && SAL_TRUE == bSnap)
            {
                for (j = 0; j < 2; j ++)
                {
                    pu8YTmp =  pu8YAddr - 1;
                    memset(pu8YTmp, pstColor->u8Y, u32Thick);
                    pu8YAddr += u32Stride;
                }
            }
            else
            {
                if (SAL_TRUE == bUseFract)
                {
                    for (j = 0; j < 2; j ++)
                    {
                        pu8YTmp  = pu8YAddr;
                        *pu8YTmp = (1 - f32XFract) * pstColor->u8Y + f32XFract * (*pu8YTmp);
                        pu8YTmp++;
                        memset(pu8YTmp, pstColor->u8Y, u32Thick - 2);
                        pu8YTmp  += (u32Thick - 2);
                        *pu8YTmp  = f32XFract * pstColor->u8Y + (1 - f32XFract) * (*pu8YTmp);
                        pu8YAddr += u32Stride;
                    }
                }
                else
                {
                    for (j = 0; j < 2; j ++)
                    {
                        memset(pu8YAddr, pstColor->u8Y, u32Thick);
                        pu8YAddr += u32Stride;
                    }
                }
            }
            /* 填充VU分量 */
            if ((u64UVAddr % 2) != 0 && (u32Thick <= 2))
            {
                if (SAL_TRUE == bSnap )
                {
                    pu16VUTmp = (UINT16 *)(pu8UVAddr - 1);
                    for (w = 0; w < u32VUNum; w++)
                    {
                        *(pu16VUTmp + w) = u16VU;
                    }
                    pu8UVAddr +=  u32Stride;
                }
                else
                {
                    /*当启始坐标为奇数时uv填充4个字节2个uv值按权重虚化线条否者会出现闪烁*/
                    pu8UvTmp =  pu8UVAddr - 1;
                    *pu8UvTmp = ((1 - f32XFract) * pstColor->u8V) + f32XFract * (*pu8UvTmp);
                    pu8UvTmp++;
                    *pu8UvTmp = ((1 - f32XFract) * pstColor->u8U) + f32XFract * (*pu8UvTmp);
                    pu8UvTmp++;
                    *pu8UvTmp = ((f32XFract) * pstColor->u8V) + (1-f32XFract) * (*pu8UvTmp);
                    pu8UvTmp++;
                    *pu8UvTmp = ((f32XFract) * pstColor->u8U) + (1-f32XFract) * (*pu8UvTmp);
                    pu8UVAddr +=  u32Stride;
                }

            }
            else
            {
                for (w = 0; w < u32VUNum; w++)
                {
                    *(pu16VUTmp + w) = u16VU;
                }
            }

            pu8YAddr  += u32Stride * u32DotGap;
            pu8UVAddr += (u32Stride * u32DotGap) >> 1;
            pu16VUTmp += u32Stride * (u32DotGap + 2) / 4;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : vgs_drvDrawVLineSoft
* 描  述  : 该函数用来申请使用CPU画竖线
* 输  入  : 
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static UINT32 vgs_drvDrawVLineSoft(UINT8 *pAddr_y, UINT8 *pAddr_vu, UINT32 u32Stride, VGS_DRAW_PRM_S *pstLinePrm, DISPLINE_PRM *pstLinetype, FLOAT32 f32XFract, BOOL bSnap)
{
    UINT8 r = 0, g = 0, b = 0;
    COLOR_YUV_S stColorYUV;
    UINT8 *pYbuff = NULL;
    UINT8 *pVUbuff = NULL;
    UINT32 u32Height = 0;

    if ((pAddr_y == NULL) || (pAddr_vu == NULL) || (pstLinePrm == NULL) || (pstLinetype == NULL))
    {
        SAL_LOGE("prm NULL err\n");
        return SAL_FAIL;
    }

    b = pstLinePrm->u32Color & 0xff;
    g = (pstLinePrm->u32Color >> 8) & 0xff;
    r = (pstLinePrm->u32Color >> 16) & 0xff;

    stColorYUV.u8Y = 16  + Y_R[r] + Y_G[g] + Y_B[b];
    stColorYUV.u8U = 128 - U_R[r] - U_G[g] + U_B[b];
    stColorYUV.u8V = 128 + V_R[r] - V_G[g] - V_B[b];

    pYbuff = pAddr_y + pstLinePrm->stStartPoint.s32Y * u32Stride + pstLinePrm->stStartPoint.s32X;
    pVUbuff = pAddr_vu + (pstLinePrm->stStartPoint.s32Y * u32Stride) / 2 + pstLinePrm->stStartPoint.s32X;
    
    if (pstLinePrm->stEndPoint.s32Y <= pstLinePrm->stStartPoint.s32Y)
    {
        SAL_ERROR("invalid var: end_y %d, start_y %d \n", pstLinePrm->stEndPoint.s32Y, pstLinePrm->stStartPoint.s32Y);
        return SAL_SOK;
    }
    
    u32Height = pstLinePrm->stEndPoint.s32Y - pstLinePrm->stStartPoint.s32Y;
    u32Height = SAL_align(u32Height, 2);

    /* 为了画实线，虚线和点画线时不用区分 */
    if (DISP_FRAME_TYPE_FULLLINE == pstLinetype->frametype)
    {
        pstLinetype->fulllinelength = u32Height;
        pstLinetype->gaps = 0;
        pstLinetype->node = 0;
    }
    else if (DISP_FRAME_TYPE_DOTTEDLINE == pstLinetype->frametype)
    {
        pstLinetype->node = 0;
    }

    (VOID)vgs_drvDrawLineVerSoft(pYbuff, pVUbuff, u32Stride, u32Height, pstLinePrm->u32Thick, &stColorYUV, pstLinetype, f32XFract, bSnap);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : vgs_halDrawHLineSoft
* 描  述  : 该函数用来申请使用CPU画横线
* 输  入  : 
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
UINT32 vgs_halDrawHLineSoft(UINT8 *pAddr_y, UINT8 *pAddr_vu, UINT32 u32Stride, 
                            VGS_DRAW_PRM_S *pstLinePrm, DISPLINE_PRM *pstLinetype)
{
    UINT8 r = 0, g = 0, b = 0;
    COLOR_YUV_S stColorYUV;
    UINT8 *pYbuff = NULL;
    UINT8 *pVUbuff = NULL;
    UINT32 u32Width = 0;

    if ((pAddr_y == NULL) || (pAddr_vu == NULL) || (pstLinePrm == NULL) || (pstLinetype == NULL))
    {
        SAL_LOGE("prm NULL err\n");
        return SAL_FAIL;
    }

    b = pstLinePrm->u32Color & 0xff;
    g = (pstLinePrm->u32Color >> 8) & 0xff;
    r = (pstLinePrm->u32Color >> 16) & 0xff;

    stColorYUV.u8Y = 16  + Y_R[r] + Y_G[g] + Y_B[b];
    stColorYUV.u8U = 128 - U_R[r] - U_G[g] + U_B[b];
    stColorYUV.u8V = 128 + V_R[r] - V_G[g] - V_B[b];
    
    pstLinePrm->stStartPoint.s32Y = SAL_align(pstLinePrm->stStartPoint.s32Y, 2);

    pYbuff = pAddr_y + pstLinePrm->stStartPoint.s32Y * u32Stride + pstLinePrm->stStartPoint.s32X;
    pVUbuff = pAddr_vu + (pstLinePrm->stStartPoint.s32Y * u32Stride) / 2 + pstLinePrm->stStartPoint.s32X;

    u32Width = pstLinePrm->stEndPoint.s32X - pstLinePrm->stStartPoint.s32X;
    u32Width = SAL_align(u32Width, 2);

    /* 为了画实线，虚线和点画线时不用区分 */
    if (DISP_FRAME_TYPE_FULLLINE == pstLinetype->frametype)
    {
        pstLinetype->fulllinelength = u32Width;
        pstLinetype->gaps = 0;
        pstLinetype->node = 0;
    }
    else if (DISP_FRAME_TYPE_DOTTEDLINE == pstLinetype->frametype)
    {
        pstLinetype->node = 0;
    }

    (VOID)vgs_drvDrawLineHorSoft(pYbuff, pVUbuff, u32Stride,u32Stride,u32Width, pstLinePrm->u32Thick, &stColorYUV, pstLinetype);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : vgs_drvDrawYuv420LineHorSoft
* 描  述  : 该函数用来申请使用CPU画横线
* 输  入  : 
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 vgs_drvDrawARGBLineHorSoft(ARGB_LINE_PARAM_S *pstLineParam, DISPLINE_PRM *pstLine)
{
    UINT32 w = 0, h = 0, i = 0,k = 0;
    UINT32 u32FullLen  = 0;
    UINT32 u32NodeNum  = 0;
    UINT32 u32DotGap   = 0;
    UINT32 u32DrawNum  = 0;
    UINT32 u32WidthTmp = 0;
    UINT32 u32DrawLen  = 0;
    UINT32 u32Stride   = 0;
    UINT32 u32Width    = 0;
    UINT32 u32Thick    = 0;
    UINT32 u32Bpp      = 0;
    UINT8 *pu8YTmp     = NULL;
    UINT8 *pu8YAddr    = NULL;
    UINT32 *pstColor   = NULL;
    
    if ((pstLineParam == NULL) || (pstLine == NULL))
    {
        SAL_LOGE("pstLineParam or pstLine is NULL err\n");
        return SAL_FAIL;
    }

    u32FullLen  = pstLine->fulllinelength;
    u32NodeNum  = pstLine->node;
    u32DotGap   = (pstLine->gaps - 2 * u32NodeNum) / (u32NodeNum + 1);    // 默认点的长度为2
    u32WidthTmp = u32FullLen + u32NodeNum * 2 + (u32NodeNum + 1) * u32DotGap;
    
    pu8YAddr  = pstLineParam->pu8YAddr;
    u32Stride = pstLineParam->u32Stride;
    u32Width  = pstLineParam->u32Width;
    u32Thick  = pstLineParam->u32Thick;
    pstColor  = pstLineParam->pstColor;
    u32Bpp    = pstLineParam->u32Bpp;
    
    for (h = 0; h < u32Thick; h++)
    {
        pu8YTmp   = pu8YAddr + u32Stride * h;
        for (w = 0; w < u32Width; w += u32WidthTmp)
        {
            u32DrawLen = ((u32Width - w) > u32FullLen) ? u32FullLen : (u32Width - w);

            /* 填充Y分量 */
            
            for (k = 0;k < u32DrawLen;k++)
            {
                memcpy(pu8YTmp + k * u32Bpp, pstColor, u32Bpp);
                //memcpy(pu8YTmp + u32Stride + k * u32Bpp, pstColor, u32Bpp);
            }

            u32DrawNum = (u32Width - w - u32DrawLen) / (u32DotGap + 2);
            u32DrawNum = (u32DrawNum > u32NodeNum) ? u32NodeNum : u32DrawNum;
            
            pu8YTmp   += (u32DotGap + u32DrawLen) * u32Bpp;
            for (i = 0; i < u32DrawNum; i++)
            {
                pu8YTmp += (u32DotGap + 2) * u32Bpp;
            }
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : vgs_drvDrawARGBLineVerSoft
* 描  述  : 该函数用来申请使用CPU画竖线
* 输  入  : 
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 vgs_drvDrawARGBLineVerSoft(ARGB_LINE_PARAM_S *pstLineParam, DISPLINE_PRM *pstLine)
{
    UINT32  h = 0, i = 0, j = 0, k = 0;
    UINT32  u32FullLen    = 0;
    UINT32  u32NodeNum    = 0;
    UINT32  u32DotGap     = 0;
    UINT32  u32DrawNum    = 0;
    UINT32  u32HeightTmp  = 0;
    UINT32  u32DrawLen    = 0;
    UINT32  u32Stride     = 0;
    UINT32  u32Height     = 0;
    UINT32  u32Thick      = 0;
    UINT32  u32Bpp        = 0;
    UINT8  *pu8YAddr      = NULL;
    UINT32 *pstColor      = NULL;
    // UINT8 *pu8YTmp       = NULL;
    // BOOL bUseFract       = ((f32XFract > 1) || (f32XFract < 0)) ? SAL_FALSE : SAL_TRUE;

    if ((pstLineParam == NULL) || (pstLine == NULL))
    {
        SAL_LOGE("pstLineParam or pstLine is NULL err\n");
        return SAL_FAIL;
    }
    
    u32FullLen    = pstLine->fulllinelength;
    u32NodeNum    = pstLine->node;
    u32DotGap     = (pstLine->gaps - 2 * u32NodeNum) / (u32NodeNum + 1);    // 默认点的长度为2
    u32HeightTmp  = u32FullLen + u32NodeNum * 2 + (u32NodeNum + 1) * u32DotGap;

    pu8YAddr      = pstLineParam->pu8YAddr;
    u32Stride     = pstLineParam->u32Stride;
    u32Height     = pstLineParam->u32Height;
    u32Thick      = pstLineParam->u32Thick;
    pstColor      = pstLineParam->pstColor;
    u32Bpp        = pstLineParam->u32Bpp;

    for (h = 0; h < u32Height; h += u32HeightTmp)
    {
        u32DrawLen = ((u32Height - h) > u32FullLen) ? u32FullLen : (u32Height - h);
        /* 画实线 */
        for (i = 0; i < u32DrawLen / 2; i++)
        {
            {
                for (j = 0; j < 2; j ++)
                {
                    for (k = 0;k < u32Thick;k++)
                    {
                        memcpy(pu8YAddr + k * u32Bpp, pstColor, u32Bpp);
                    }
                    pu8YAddr += u32Stride;
                }
            }
        }

        pu8YAddr  += u32Stride * u32DotGap;
        
        u32DrawNum = (u32Height - h - u32DrawLen) / (u32DotGap + 2);
        u32DrawNum = (u32DrawNum > u32NodeNum) ? u32NodeNum : u32DrawNum;
        /* 画点 */
        for (i = 0; i < u32DrawNum; i++)
        {
            {
                for (j = 0; j < 2; j ++)
                {
                    for (k = 0;k < u32Thick;k++)
                    {
                        memcpy(pu8YAddr + k * u32Bpp, pstColor, u32Bpp);
                    }
                    pu8YAddr += u32Stride;
                }
            }

            pu8YAddr  += u32Stride * u32DotGap;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : vgs_drvDrawARGBHLineSoft
* 描  述  : 该函数用来申请使用CPU画横线
* 输  入  : 
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static UINT32 vgs_drvDrawARGBHLineSoft(UINT8 *pAddr_y, UINT32 u32Stride, VGS_DRAW_PRM_S *pstLinePrm, DISPLINE_PRM *pstLinetype, FLOAT32 f32XFract,UINT32 u32Bpp)
{
    UINT32 argb      = 0xff000000;
    UINT8 *pYbuff    = NULL;
    UINT32 u32Width = 0;
    ARGB_LINE_PARAM_S stLineParam = {0};

    if ((pAddr_y == NULL) || (pstLinePrm == NULL) || (pstLinetype == NULL))
    {
        SAL_LOGE("prm NULL err\n");
        return SAL_FAIL;
    }

    argb = 0xff000000 | (pstLinePrm->u32Color & 0x00ffffff);

    pYbuff = pAddr_y + pstLinePrm->stStartPoint.s32Y * u32Stride + pstLinePrm->stStartPoint.s32X * u32Bpp;

    u32Width = pstLinePrm->stEndPoint.s32X - pstLinePrm->stStartPoint.s32X;
//    u32Width = SAL_align(u32Width, 2);

    /* 为了画实线，虚线和点画线时不用区分 */
    if (DISP_FRAME_TYPE_FULLLINE == pstLinetype->frametype)
    {
        pstLinetype->fulllinelength = u32Width;
        pstLinetype->gaps = 0;
        pstLinetype->node = 0;
    }
    else if (DISP_FRAME_TYPE_DOTTEDLINE == pstLinetype->frametype)
    {
        pstLinetype->node = 0;
    }
    
    stLineParam.pu8YAddr  = pYbuff;
    stLineParam.u32Stride = u32Stride;
    stLineParam.u32Width = u32Width;
    stLineParam.u32Thick  = pstLinePrm->u32Thick;
    stLineParam.pstColor  = &argb;
    stLineParam.u32Bpp    = u32Bpp;
    stLineParam.f32XFract = f32XFract;

    (VOID)vgs_drvDrawARGBLineHorSoft(&stLineParam, pstLinetype);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : vgs_drvDrawARGBVLineSoft
* 描  述  : 该函数用来申请使用CPU画竖线
* 输  入  : 
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static UINT32 vgs_drvDrawARGBVLineSoft(UINT8 *pAddr_y, UINT32 u32Stride, VGS_DRAW_PRM_S *pstLinePrm, DISPLINE_PRM *pstLinetype, FLOAT32 f32XFract,UINT32 u32Bpp)
{
    UINT32 argb      = 0xff000000;
    UINT8 *pYbuff    = NULL;
    UINT32 u32Height = 0;
    ARGB_LINE_PARAM_S stLineParam = {0};

    if ((pAddr_y == NULL) || (pstLinePrm == NULL) || (pstLinetype == NULL))
    {
        SAL_LOGE("prm NULL err\n");
        return SAL_FAIL;
    }

    argb = 0xff000000 | (pstLinePrm->u32Color & 0x00ffffff);

    pYbuff = pAddr_y + pstLinePrm->stStartPoint.s32Y * u32Stride + pstLinePrm->stStartPoint.s32X * u32Bpp;

    u32Height = pstLinePrm->stEndPoint.s32Y - pstLinePrm->stStartPoint.s32Y;
    u32Height = SAL_align(u32Height, 2);

    /* 为了画实线，虚线和点画线时不用区分 */
    if (DISP_FRAME_TYPE_FULLLINE == pstLinetype->frametype)
    {
        pstLinetype->fulllinelength = u32Height;
        pstLinetype->gaps = 0;
        pstLinetype->node = 0;
    }
    else if (DISP_FRAME_TYPE_DOTTEDLINE == pstLinetype->frametype)
    {
        pstLinetype->node = 0;
    }
    
    stLineParam.pu8YAddr  = pYbuff;
    stLineParam.u32Stride = u32Stride;
    stLineParam.u32Height = u32Height;
    stLineParam.u32Thick  = pstLinePrm->u32Thick;
    stLineParam.pstColor  = &argb;
    stLineParam.u32Bpp    = u32Bpp;
    stLineParam.f32XFract = f32XFract;

    (VOID)vgs_drvDrawARGBLineVerSoft(&stLineParam, pstLinetype);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : vgs_drvDrawARGBRectSoft
* 描  述  : 该函数用来申请使用CPU画框,右竖线和下横线回退一个线宽，保证以左上角为起点框长宽不越界，
            横向超出图片是动态过包正常现象进行调整，纵向超出是数据错误不画框
* 输  入  : 
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 vgs_drvDrawARGBRectSoft(SAL_VideoFrameBuf *pstFrame, VGS_DRAW_RECT_S *pstRect, RECT_EXCEED_TYPE_E enRectType, BOOL bSnap)
{
    UINT32 argb = 0xff000000;
    UINT32 u32Bpp        = 0;
    UINT8 *pYbuff        = NULL;
    UINT8 *pu8YTmp       = NULL;
    UINT32 u32RectWidth  = 0;
    UINT32 u32RectHeight = 0;
    UINT32 u32PicWidth   = 0;
    UINT32 u32PicHeight  = 0;
    UINT32 u32Stride0    = 0;
    INT32  s32StartX     = 0;
    INT32  s32EndX       = 0;
    BOOL   bRightExceed  = SAL_FALSE;
    BOOL   bLeftExceed   = SAL_FALSE;
    ARGB_LINE_PARAM_S stLineParam = {0};

    if ((pstFrame == NULL) || (pstRect == NULL))
    {
        SAL_LOGE("prm NULL err\n");
        return SAL_FAIL;
    }

    SAL_halGetPixelBpp(pstFrame->frameParam.dataFormat, &u32Bpp);
    
    u32PicWidth   = pstFrame->frameParam.width;
    u32PicHeight  = pstFrame->frameParam.height;
    
    /* rk中的stride是图像虚宽,没有乘图像位数 */
    u32Stride0    = pstFrame->stride[0] * u32Bpp;
    u32RectWidth  = pstRect->u32Width;
    u32RectHeight = pstRect->u32Height;
    s32StartX     = pstRect->s32X;
    s32EndX       = s32StartX + pstRect->u32Width;
    argb = 0xff000000 | (pstRect->u32Color & 0x00ffffff);

    /* 为了画实线，虚线和点画线时不用区分 */
    if (DISP_FRAME_TYPE_FULLLINE == pstRect->stLinePrm.frametype)
    {
        pstRect->stLinePrm.gaps = 0;
        pstRect->stLinePrm.node = 0;
    }
    else if (DISP_FRAME_TYPE_DOTTEDLINE == pstRect->stLinePrm.frametype)
    {
        pstRect->stLinePrm.node = 0;
    }

    /* 左上角超出图片左侧 */
    if (s32StartX <= 0)
    {
        s32StartX = 0;
        if (s32StartX >= s32EndX)
        {
            return SAL_SOK;
        }
        u32RectWidth = s32EndX - s32StartX;
        bLeftExceed = SAL_TRUE;
    }

    /* 左上角超出图片右侧不画 */
    if (s32StartX + pstRect->u32Thick >= u32PicWidth)
    {
        return SAL_SOK;
    }

    /* 框部分超出图片右侧 */
    if (s32EndX > u32PicWidth)
    {
        u32RectWidth = u32PicWidth - s32StartX;
        bRightExceed = SAL_TRUE;
    }

    /* 左上角超出图片上边界、框下边界超出图片下边界 */
    if (pstRect->s32Y < 0 || pstRect->s32Y + pstRect->u32Height > u32PicHeight)
    {
        SAL_LOGE("rect Y:%d Height:%d  PicHeight:%d\n", pstRect->s32Y, pstRect->u32Height, u32PicHeight);
        return SAL_SOK;
    }

    if ((u32RectWidth <= pstRect->u32Thick) || (u32RectHeight <= pstRect->u32Thick))
    {
        return SAL_SOK;
    }

    /* 定位到左上角 */
    pYbuff = (UINT8 *)(pstFrame->virAddr[0] + pstRect->s32Y * u32Stride0 + s32StartX * u32Bpp);

    /* 实线模式需要根据框宽更新实线长度 */
    if (DISP_FRAME_TYPE_FULLLINE == pstRect->stLinePrm.frametype)
    {
        pstRect->stLinePrm.fulllinelength = u32RectWidth;
    }

    /* 上横线 */
    pu8YTmp = pYbuff;
    stLineParam.pu8YAddr  = pu8YTmp;
    stLineParam.u32Stride = u32Stride0;
    stLineParam.u32Width  = u32RectWidth;
    stLineParam.u32Thick  = pstRect->u32Thick;
    stLineParam.pstColor  = &argb;
    stLineParam.u32Bpp    = u32Bpp;
    (VOID)vgs_drvDrawARGBLineHorSoft(&stLineParam, &pstRect->stLinePrm);

    /* 下横线，回退一个线宽 */
    pu8YTmp = pYbuff + (u32RectHeight - pstRect->u32Thick) * u32Stride0;
    stLineParam.pu8YAddr = pu8YTmp;
    (VOID)vgs_drvDrawARGBLineHorSoft(&stLineParam, &pstRect->stLinePrm);

    /* 实线模式需要根据框高更新实线长度 */
    if (DISP_FRAME_TYPE_FULLLINE == pstRect->stLinePrm.frametype)
    {
        pstRect->stLinePrm.fulllinelength = u32RectHeight;
    }

    if ((SAL_TRUE != bLeftExceed) || (RECT_EXCEED_TYPE_CLOSE == enRectType))
    {
        /* 左竖线 */
        pu8YTmp = pYbuff;
        stLineParam.pu8YAddr  = pu8YTmp;
        stLineParam.u32Height = u32RectHeight;
        stLineParam.f32XFract = pstRect->f32StartXFract;
        (VOID)vgs_drvDrawARGBLineVerSoft(&stLineParam, &pstRect->stLinePrm);
    }

    if ((SAL_TRUE != bRightExceed) || (RECT_EXCEED_TYPE_CLOSE == enRectType))
    {
        /* 右竖线，回退一个线宽 */
        pu8YTmp = pYbuff + (u32RectWidth - pstRect->u32Thick) * u32Bpp;
        stLineParam.pu8YAddr = pu8YTmp;
        (VOID)vgs_drvDrawARGBLineVerSoft(&stLineParam, &pstRect->stLinePrm);
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : vgs_drvDrawRectSoft
* 描  述  : 该函数用来申请使用CPU画框，右竖线和下横线回退一个线宽，保证以左上角为起点框长宽不越界，
            横向超出图片是动态过包正常现象进行调整，纵向超出是数据错误不画框
* 输  入  : 
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 vgs_drvDrawRectSoft(SAL_VideoFrameBuf *pstFrame, VGS_DRAW_RECT_S *pstRect, RECT_EXCEED_TYPE_E enRectType, BOOL bSnap)
{
    UINT8 r = 0, g = 0, b = 0;
    COLOR_YUV_S stColorYUV;
    UINT8 *pYbuff        = NULL;
    UINT8 *pVUbuff       = NULL;
    UINT8 *pu8YTmp       = NULL;
    UINT8 *pu8VUTmp      = NULL;
    UINT32 u32RectWidth  = 0;
    UINT32 u32RectHeight = 0;
    UINT32 u32PicWidth   = 0;
    UINT32 u32PicHeight  = 0;
    UINT32 u32Stride0    = 0;
    UINT32 u32Stride1    = 0;
    INT32 s32StartX   = 0;
    INT32 s32EndX     = 0;
    BOOL bRightExceed = SAL_FALSE;
    BOOL bLeftExceed  = SAL_FALSE;

    if ((pstFrame == NULL) || (pstRect == NULL))
    {
        SAL_LOGE("prm NULL err\n");
        return SAL_FAIL;
    }

    u32PicWidth   = pstFrame->frameParam.width;
    u32PicHeight  = pstFrame->frameParam.height;
    u32Stride0    = pstFrame->stride[0];
    u32Stride1    = pstFrame->stride[1];
    u32RectWidth  = pstRect->u32Width;
    u32RectHeight = pstRect->u32Height;
    s32StartX     = pstRect->s32X;
    s32EndX       = s32StartX + pstRect->u32Width;
    
    b = pstRect->u32Color & 0xff;
    g = (pstRect->u32Color >> 8) & 0xff;
    r = (pstRect->u32Color >> 16) & 0xff;

    stColorYUV.u8Y = 16  + Y_R[r] + Y_G[g] + Y_B[b];
    stColorYUV.u8U = 128 - U_R[r] - U_G[g] + U_B[b];
    stColorYUV.u8V = 128 + V_R[r] - V_G[g] - V_B[b];

    /* 为了画实线，虚线和点画线时不用区分 */
    if (DISP_FRAME_TYPE_FULLLINE == pstRect->stLinePrm.frametype)
    {
        pstRect->stLinePrm.gaps = 0;
        pstRect->stLinePrm.node = 0;
    }
    else if (DISP_FRAME_TYPE_DOTTEDLINE == pstRect->stLinePrm.frametype)
    {
        pstRect->stLinePrm.node = 0;
    }

    /* 左上角超出图片左侧 */
    if (s32StartX < 2) // 画竖线为防闪烁对相邻列uv值进行虚化，预留2像素防止内存越界
    {
        s32StartX = 2;
        if (s32StartX >= s32EndX)
        {
            return SAL_SOK;
        }
        u32RectWidth = s32EndX - s32StartX;
        bLeftExceed = SAL_TRUE;
    }

    /* 左上角超出图片右侧不画 */
    if (s32StartX + pstRect->u32Thick >= u32PicWidth)
    {
        return SAL_SOK;
    }

    /* 框部分超出图片右侧 */
    if (s32EndX + 2 > u32PicWidth) // 画竖线为防闪烁对相邻列uv值进行虚化，预留2像素防止内存越界
    {
        u32RectWidth = u32PicWidth - s32StartX - 2;
        // SAL_LOGI("change rectwidth:%d - %d - 2 = %d\n", u32PicWidth, s32StartX, u32RectWidth);
        bRightExceed = SAL_TRUE;
    }

    /* 左上角超出图片上边界、框下边界超出图片下边界 */
    if (pstRect->s32Y < 0 || pstRect->s32Y + pstRect->u32Height > u32PicHeight)
    {
        SAL_LOGE("rect Y:%d Height:%d  PicHeight:%d\n", pstRect->s32Y, pstRect->u32Height, u32PicHeight);
        return SAL_SOK;
    }

    if (u32RectWidth <= pstRect->u32Thick || pstRect->u32Height <= pstRect->u32Thick)
    {
        return SAL_SOK;
    }

    /* 定位到左上角 */
    pYbuff  = (UINT8 *)(pstFrame->virAddr[0] + pstRect->s32Y * u32Stride0 + s32StartX);
    pVUbuff = (UINT8 *)(pstFrame->virAddr[1] + pstRect->s32Y * u32Stride1 / 2 + s32StartX);

    /* 实线模式需要根据框宽更新实线长度 */
    if (DISP_FRAME_TYPE_FULLLINE == pstRect->stLinePrm.frametype)
    {
        pstRect->stLinePrm.fulllinelength = u32RectWidth;
    }

    /* 上横线 */
    pu8YTmp = pYbuff;
    pu8VUTmp = pVUbuff;
    (VOID) vgs_drvDrawLineHorSoft(pu8YTmp, pu8VUTmp, u32Stride0, u32PicWidth, u32RectWidth, pstRect->u32Thick, &stColorYUV, &pstRect->stLinePrm);

    /* 下横线，回退一个线宽 */
    pu8YTmp = pYbuff + (u32RectHeight - pstRect->u32Thick) * u32Stride0;
    pu8VUTmp = pVUbuff + ((u32RectHeight - pstRect->u32Thick) * u32Stride1) / 2;
    (VOID) vgs_drvDrawLineHorSoft(pu8YTmp, pu8VUTmp, u32Stride0, u32PicWidth, u32RectWidth, pstRect->u32Thick, &stColorYUV, &pstRect->stLinePrm);

    /* 实线模式需要根据框高更新实线长度 */
    if (DISP_FRAME_TYPE_FULLLINE == pstRect->stLinePrm.frametype)
    {
        pstRect->stLinePrm.fulllinelength = u32RectHeight + pstRect->u32Thick;
    }

    /* 左竖线 */
    if ((SAL_TRUE != bLeftExceed) || (RECT_EXCEED_TYPE_CLOSE == enRectType))
    {
        pu8YTmp = pYbuff;
        pu8VUTmp = pVUbuff;
        (VOID) vgs_drvDrawLineVerSoft(pu8YTmp, pu8VUTmp, u32Stride0, u32RectHeight, pstRect->u32Thick, &stColorYUV, &pstRect->stLinePrm, pstRect->f32StartXFract, bSnap);
    }

    /* 右竖线，回退一个线宽 */
    if ((SAL_TRUE != bRightExceed) || (RECT_EXCEED_TYPE_CLOSE == enRectType))
    {
        pu8YTmp = pYbuff + u32RectWidth - pstRect->u32Thick;
        pu8VUTmp = pVUbuff + u32RectWidth - pstRect->u32Thick;
        (VOID) vgs_drvDrawLineVerSoft(pu8YTmp, pu8VUTmp, u32Stride0, u32RectHeight, pstRect->u32Thick, &stColorYUV, &pstRect->stLinePrm, pstRect->f32EndXFract, bSnap);
    }

    return SAL_SOK;
}

/******************************************************************
   Function:   vgs_drvgetTimeMilli
   Description:
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
static UINT64 vgs_drvgetTimeMilli()
{
    struct timeval tv;
    UINT64 time = 0;

    gettimeofday(&tv, NULL);
    time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return time;
}

/*******************************************************************************
* 函数名  : vgs_func_drawLineSoft
* 描  述  : 使用CPU进行垂直画线
* 输  入  : - stTask     : 帧数据
*         : - pstLinePrm : 画线数据
*         : - needMmap   : 是否需要对地址进行map
*         : - ailinetype : 画线类型
* 输  出  : - stTask     : 帧数据
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 vgs_func_drawLineSoft(SAL_VideoFrameBuf *pstSalFrame, VGS_DRAW_LINE_PRM *pstLinePrm, UINT32 needMmap, BOOL bSnap)
{
    INT32 iRet = SAL_SOK;
    UINT32 i = 0;
    /* UINT32 sizeY = 0, sizeVU = 0; */
    UINT8 *virtAddr_Y = 0;
    UINT8 *virtAddr_VU = 0;
    UINT64 time0 = 0, time1 = 0, time2 = 0;
    UINT32 u32Bpp = 0;
    UINT32 u32Stride = 0;
    VGS_DRAW_PRM_S *pstLineInfo = NULL;
    
    time0 = vgs_drvgetTimeMilli();
    if (pstLinePrm == NULL || pstSalFrame == NULL)
    {
        SAL_LOGE("NULL err\n");
        return -1;
    }
#if 0   /* liuxinying 2021.07.20 后续所有获取帧时都会映射虚拟地址不用单独获取，待验证后删除*/
    if (needMmap)
    {
        sizeY = pstSalFrame->stride[0] * pstSalFrame->frameParam.height;
        /* sizeVU = (stTask->stImgOut.stVFrame.u32Stride[0] * stTask->stImgOut.stVFrame.u32Height) / 2; */

        void *virAddr = NULL;
        iRet = HI_MPI_VB_GetBlockVirAddr(pstSalFrame->poolId, pstSalFrame->phyAddr[0], (void **)&virAddr);
        if (HI_SUCCESS == iRet)
        {
            /* pstVideoFrame->stVFrame.u64VirAddr[0] = (HI_U64)virAddr; */
            virtAddr_Y = (UINT8 *)virAddr; /* (UINT8*)HI_MPI_SYS_Mmap(stTask->stImgOut.stVFrame.u64PhyAddr[0], sizeY + sizeVU); */
        }
        else
        {
            SAL_LOGE("u32PoolId %d error\n", pstSalFrame->poolId);
            return -1;
        }

        /* virtAddr_Y = (UINT8*)HI_MPI_SYS_Mmap(stTask->stImgOut.stVFrame.u64PhyAddr[0], sizeY + sizeVU); */
        if (NULL == virtAddr_Y)
        {
            SAL_LOGE("u32PoolId %d error\n", pstSalFrame->poolId);
            return -1;
        }
    }
    else
#endif

    virtAddr_Y = (UINT8 *)(pstSalFrame->virAddr[0]);
    if (virtAddr_Y == NULL)
    {
        SAL_LOGE("virtAddr_Y %p,phy :%p\n", virtAddr_Y, (UINT8 *)(pstSalFrame->phyAddr[0]));
        return -1;
    }

    virtAddr_VU = (UINT8 *)(pstSalFrame->virAddr[1]);
    if ((pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_YUV420SP_UV || pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_YUV420SP_VU)
     && (virtAddr_VU == NULL))
    {
        SAL_LOGE("virtAddr_VU %p,phy :%p\n", virtAddr_VU, (UINT8 *)(pstSalFrame->phyAddr[1]));
        return -1;
    }

    SAL_halGetPixelBpp(pstSalFrame->frameParam.dataFormat, &u32Bpp);
    u32Stride = pstSalFrame->stride[0] * u32Bpp;

    time1 = vgs_drvgetTimeMilli();
    for (i = 0; i < pstLinePrm->uiLineNum; i++)
    {
        pstLineInfo = &pstLinePrm->stVgsLinePrm[i];
        //防止线画出显示范围外
        pstLineInfo->stStartPoint.s32X = pstLineInfo->stStartPoint.s32X >= 0 ? pstLineInfo->stStartPoint.s32X : 0;
        pstLineInfo->stStartPoint.s32Y = pstLineInfo->stStartPoint.s32Y >= 0 ? pstLineInfo->stStartPoint.s32Y : 0;
        pstLineInfo->stEndPoint.s32X = pstLineInfo->stEndPoint.s32X >= 0 ? pstLineInfo->stEndPoint.s32X : 0;
        pstLineInfo->stEndPoint.s32Y = pstLineInfo->stEndPoint.s32Y >= 0 ? pstLineInfo->stEndPoint.s32Y : 0;
        pstLineInfo->stStartPoint.s32X = pstLineInfo->stStartPoint.s32X < (pstSalFrame->frameParam.width - pstLineInfo->u32Thick) ? pstLineInfo->stStartPoint.s32X : (pstSalFrame->frameParam.width - pstLineInfo->u32Thick);
        pstLineInfo->stStartPoint.s32Y = pstLineInfo->stStartPoint.s32Y < (pstSalFrame->frameParam.height - pstLineInfo->u32Thick) ? pstLineInfo->stStartPoint.s32Y : (pstSalFrame->frameParam.height - pstLineInfo->u32Thick); 
        pstLineInfo->stEndPoint.s32X = pstLineInfo->stEndPoint.s32X < (pstSalFrame->frameParam.width - pstLineInfo->u32Thick) ? pstLineInfo->stEndPoint.s32X : (pstSalFrame->frameParam.width - pstLineInfo->u32Thick);
        pstLineInfo->stEndPoint.s32Y = pstLineInfo->stEndPoint.s32Y < (pstSalFrame->frameParam.height - pstLineInfo->u32Thick) ? pstLineInfo->stEndPoint.s32Y : (pstSalFrame->frameParam.height - pstLineInfo->u32Thick);

        if (pstLineInfo->stStartPoint.s32X > pstLineInfo->stEndPoint.s32X
            || pstLineInfo->stStartPoint.s32X >= pstSalFrame->frameParam.width - pstLineInfo->u32Thick
            || pstLineInfo->stEndPoint.s32X >= pstSalFrame->frameParam.width - pstLineInfo->u32Thick
            || pstLineInfo->stStartPoint.s32Y >= pstSalFrame->frameParam.height - pstLineInfo->u32Thick
            || pstLineInfo->stEndPoint.s32Y >= pstSalFrame->frameParam.height - pstLineInfo->u32Thick)
        {
            SAL_DEBUG("invalid line prm! i %d, start[%d, %d], end[%d, %d], w %d, h %d \n", 
                      i, pstLineInfo->stStartPoint.s32X, pstLineInfo->stStartPoint.s32Y,
                      pstLineInfo->stEndPoint.s32X, pstLineInfo->stEndPoint.s32Y, pstSalFrame->frameParam.width, pstSalFrame->frameParam.height);
            continue;
        }
        if ((pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_BGRA_8888) 
            || (pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_ARGB_8888)
            || (pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_RGB24_888))
        {
            if (pstLineInfo->stStartPoint.s32X == pstLineInfo->stEndPoint.s32X)
            {
                (VOID)vgs_drvDrawARGBVLineSoft(virtAddr_Y, u32Stride, 
                                 pstLinePrm->stVgsLinePrm + i, pstLinePrm->linetype + i, pstLinePrm->af32XFract[i], u32Bpp);
            }
            else
            {
                (VOID)vgs_drvDrawARGBHLineSoft(virtAddr_Y, u32Stride, 
                                 pstLinePrm->stVgsLinePrm + i, pstLinePrm->linetype + i, pstLinePrm->af32XFract[i], u32Bpp);
            }
        }
        else if (pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_YUV420SP_UV || pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_YUV420SP_VU)
        {
            if (pstLineInfo->stStartPoint.s32X == pstLineInfo->stEndPoint.s32X)
            {
                (VOID)vgs_drvDrawVLineSoft(virtAddr_Y, virtAddr_VU, pstSalFrame->stride[0], 
                                           pstLineInfo, &pstLinePrm->linetype[i], pstLinePrm->af32XFract[i], bSnap);
            }
            else
            {
                (VOID)vgs_halDrawHLineSoft(virtAddr_Y, virtAddr_VU, pstSalFrame->stride[0], pstLineInfo, &pstLinePrm->linetype[i]);
            }
        }
        else
        {
            SAL_LOGE("dataFormat %d is not support\n", pstSalFrame->frameParam.dataFormat);
        }
    }

    time2 = vgs_drvgetTimeMilli();

    if (time2 - time0 > 5)
    {
        SAL_LOGD("!!!!! time de %llu,%llu\n", time2 - time0, time2 - time1);
    }

    return iRet;
}

/*****************************************************************************
   函 数 名  : vgs_func_drawRectArraySoft
   功能描述  : 软件画矩形
   输入参数  : stTask 任务参数
                             pstLinePrm 线的信息参数
                             needMmap 若虚拟地址存在则不需要重新映射否则必须映射
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_func_drawRectArraySoft(SAL_VideoFrameBuf *pstSalFrame, VGS_RECT_ARRAY_S *pstRectArray, BOOL bNeedMmap, BOOL bSnap)
{
    INT32 iRet = SAL_SOK;
    UINT32 i = 0;
    UINT64 time0 = 0, time1 = 0, time2 = 0;

    time0 = vgs_drvgetTimeMilli();
    if (pstRectArray == NULL || pstSalFrame == NULL)
    {
        SAL_LOGE("NULL err\n");
        return SAL_FAIL;
    }

#if 0   /* liuxinying 2021.07.20 后续所有获取帧时都会映射虚拟地址不用单独获取，待验证后删除*/
    if (SAL_TRUE == bNeedMmap)
    {
        void *virAddr = NULL;
        iRet = HI_MPI_VB_GetBlockVirAddr(pstSalFrame->poolId, pstSalFrame->phyAddr[0], (void **)&virAddr);
        if (SAL_SOK == iRet)
        {
            pstSalFrame->virAddr[0] = (UINT64)virAddr; /* (UINT8*)HI_MPI_SYS_Mmap(pstFrame->stVFrame.u64PhyAddr[0], sizeY + sizeVU); */
            pstSalFrame->virAddr[1] = pstSalFrame->virAddr[0] + pstSalFrame->stride[0] * pstSalFrame->frameParam.height;
            pstSalFrame->virAddr[2] = pstSalFrame->virAddr[1]; //??? + pstSalFrame->stride[0] * pstSalFrame->frameParam.height;
        }
        else
        {
            SAL_LOGE("u32PoolId %d error\n", pstSalFrame->poolId);
            return -1;
        }

        /* virtAddr_Y = (UINT8*)HI_MPI_SYS_Mmap(pstFrame->stVFrame.u64PhyAddr[0], sizeY + sizeVU); */
        if (0 == pstSalFrame->virAddr[0])
        {
            SAL_LOGE("u32PoolId %d error\n", pstSalFrame->poolId);
            return -1;
        }
    }
    else
#endif
    {
        if (pstSalFrame->virAddr[0] == 0)
        {
            SAL_LOGE("virtAddr_Y 0x%llx,phy :%p\n", pstSalFrame->virAddr[0], (UINT8 *)(pstSalFrame->phyAddr[0]));
            return -1;
        }
    }

    time1 = vgs_drvgetTimeMilli();

    for (i = 0; i < pstRectArray->u32RectNum; i++)
    {
        if ((pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_BGRA_8888) 
            || (pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_ARGB_8888)
            || (pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_RGB24_888))
        {
            (VOID)vgs_drvDrawARGBRectSoft(pstSalFrame, pstRectArray->astRect + i, pstRectArray->enExceedType, bSnap);
        }
        else if (pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_YUV420SP_UV || pstSalFrame->frameParam.dataFormat == SAL_VIDEO_DATFMT_YUV420SP_VU)
        {
            (VOID)vgs_drvDrawRectSoft(pstSalFrame, pstRectArray->astRect + i, pstRectArray->enExceedType, bSnap);
        }
        else
        {
            SAL_LOGE("dataFormat %d is not support\n", pstSalFrame->frameParam.dataFormat);
        }
    }

    time2 = vgs_drvgetTimeMilli();

    if (time2 - time0 > 15)
    {
        SAL_LOGI("time de %llu,%llu\n", time2 - time0, time2 - time1);
    }

    return iRet;
}

/*******************************************************************************
* 函数名  : vgs_func_drawLineSoftInit
* 描  述  : 该函数用来申请使用CPU画框和线的缓存
* 输  入  : 
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 vgs_func_drawLineSoftInit(VGS_DRAW_AI_PRM **ppstPrm)
{
    if (NULL == ppstPrm)
    {
        SAL_LOGE("null pointer\n");
        return SAL_FAIL;
    }

    *ppstPrm = SAL_memMalloc(sizeof(VGS_DRAW_AI_PRM), "vgs_drv", "vgs_mem");
    if (NULL == *ppstPrm)
    {
        SAL_LOGE("malloc fail\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}




