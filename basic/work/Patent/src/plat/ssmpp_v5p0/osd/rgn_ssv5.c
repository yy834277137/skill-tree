/**
 * @file   rgn_ssv5.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  rgn---OSD处理接口封装
 * @author wangweiqin
 * @date   2017年09月08日 Create
 * @note
 * @note \n History
   1.Date        : 2017年09月08日 Create
     Author      : wangweiqin
     Modification: 新建文件
   2.Date        : 2021/08/
   9     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#include <platform_sdk.h>
#include "rgn_ssv5.h"

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/*****************************************************************************
                               宏定义
*****************************************************************************/

#define SAL_REQUEST(_C_, _ERR_)               \
    do                                       \
    {                                        \
        if (!(_C_))                          \
        {                                   \
            OSD_LOGE("%s s32Ret %x\n",     \
                     # _C_, _ERR_);                   \
            goto EXIT;                      \
        }                                   \
    }                                        \
    while (0)

/*****************************************************************************
                               全局变量
*****************************************************************************/

/* static HI_S8 ascFont[16] = {0x0  ,0x0  ,0x38 ,0x6c ,0xc6 ,0xc6 ,0xd6 ,0xd6 ,0xc6 ,0xc6 ,0x6c ,0x38 ,0x0  ,0x0  ,0x0  ,0x0 } ; */


/*****************************************************************************
                                函数
*****************************************************************************/

#define TDE_SURFACE_SET(_SURFACE_, _FMT_, _ALPHAM_, _CLUTPA_, _PA_, _STRIDE_, _W_, _H_)  \
    do                                                                        \
    {                                                                         \
        _SURFACE_.enColorFmt = (_FMT_);                                  \
        _SURFACE_.bAlphaMax255 = (_ALPHAM_);                               \
        _SURFACE_.ClutPhyAddr = (_CLUTPA_);                               \
        _SURFACE_.PhyAddr = (_PA_);                                   \
        _SURFACE_.u32Stride = (_STRIDE_);                               \
        _SURFACE_.u32Width = (_W_);                                    \
        _SURFACE_.u32Height = (_H_);                                    \
    }                                                                         \
    while (0)

#define OSD_SCALE_SET(_XD_, _YD_, _XM_, _YM_, _WD_, _HD_, _WM_, _HM_)                    \
    do                                                                        \
    {                                                                         \
        xDiv = _XD_;                                                       \
        yDiv = _YD_;                                                       \
        xMul = _XM_;                                                       \
        yMul = _YM_;                                                       \
        wDiv = _WD_;                                                       \
        hDiv = _HD_;                                                       \
        wMul = _WM_;                                                       \
        hMul = _HM_;                                                       \
    }                                                                         \
    while (0)

#if 0

/*******************************************************************************
   Function:    OsdRectCheck
   Description: 检测OSD区域是否合适
   Input:
   Output:      N/A
   Return:
    0:          Successful
    ohters:     Failed
*******************************************************************************/
static td_s32 OsdRectCheck(TDE2_RECT_S *pRect, TDE2_RECT_S *pDstRect)
{
    INT32 xa1, ya1, xa2, ya2, xb1, yb1, xb2, yb2;

    xa1 = pRect->s32Xpos;
    ya1 = pRect->s32Ypos;

    xa2 = pRect->s32Xpos + pRect->u32Width;
    ya2 = pRect->s32Ypos + pRect->u32Height;

    xb1 = pDstRect->s32Xpos;
    yb1 = pDstRect->s32Ypos;

    xb2 = pDstRect->s32Xpos + pDstRect->u32Width;
    yb2 = pDstRect->s32Ypos + pDstRect->u32Height;
    if ((MAX2(xa1, xb1) <= MIN2(xa2, xb2)) && (MAX2(ya1, yb1) <= MIN2(ya2, yb2)))
    {
        return TD_TRUE;
    }
    else
    {
        return TD_FALSE;
    }
}

#endif

/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   rgn_create
 * @brief      创建rgn通道
 * @param[in]  RGN_SSV5_S *pstRegion rgn参数
 * @param[in]  td_u32 u32BgColor 背景色
 * @param[in]  td_u32 u32Width,td_u32 u32Height rgn大小
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 rgn_create(RGN_SSV5_S *pstRegion, td_u32 u32BgColor, td_u32 u32Width, td_u32 u32Height)
{
    td_s32 s32Ret = TD_SUCCESS;
    ot_rgn_attr *pstRgnAttr = NULL;

    if (NULL == pstRegion)
    {
        OSD_LOGE("is null\n");
        return SAL_FAIL;
    }

    if (TD_FALSE != pstRegion->bCreated)
    {
        return SAL_SOK;
    }

    pstRgnAttr = &pstRegion->stRgnAttr;

    pstRgnAttr->type = OT_RGN_OVERLAY;
    pstRgnAttr->attr.overlay.pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
    pstRgnAttr->attr.overlay.size.width = u32Width;
    pstRgnAttr->attr.overlay.size.height = u32Height;
    pstRgnAttr->attr.overlay.bg_color = u32BgColor;
    pstRgnAttr->attr.overlay.canvas_num = 2;
    /*像素为 HI_PIXEL_FORMAT_ARGB_CLUT2 或HI_PIXEL_FORMAT_ARGB_CLUT4 时，才进行参数范围检查*/
    /*pstRgnAttr->attr.overlay.clut*/

    s32Ret = ss_mpi_rgn_create(pstRegion->handle, pstRgnAttr);
    if (TD_SUCCESS != s32Ret)
    {
        OSD_LOGE("HI_MPI_RGN_Create Error : %#x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    pstRegion->bCreated = SAL_TRUE;
    OSD_LOGI("u32Width %d u32Height %d u32BgColor %d,handle 0x%x Created ok\n", u32Width, u32Height, u32BgColor, pstRegion->handle);

    return SAL_SOK;
}

/**
 * @function   rgn_updateBM
 * @brief      刷新OSD位图
 * @param[in]  RGN_SSV5_S *pRegion rgn参数
 * @param[in]  void *addr 位图buf
 * @param[in]  td_s32 x, td_s32 y,td_u32 w,td_u32 h 位图叠加区域信息
 * @param[in]  td_u32 stride 无效参数
 * @param[in]  td_u32 bUp2 这个操作没看懂^_^
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 rgn_updateBM(RGN_SSV5_S *pstRegion, void *pAddr, td_s32 x, td_s32 y, td_u32 w, td_u32 h, td_u32 u32Stride, td_u32 bUp2)
{
    td_s32 s32Ret = TD_SUCCESS;

    ot_bmp stBitMap;

    memset(&stBitMap, 0, sizeof(ot_bmp));

    stBitMap.pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
    stBitMap.data = pAddr;
    stBitMap.height = h;
    stBitMap.width = w;

    s32Ret = ss_mpi_rgn_set_bmp(pstRegion->handle, &stBitMap);
    if (TD_SUCCESS != s32Ret)
    {
        OSD_LOGE("ret %#x!!!\n", s32Ret);

        return SAL_FAIL;
    }

    if (TD_TRUE == bUp2)
    {
        s32Ret = ss_mpi_rgn_set_bmp(pstRegion->handle, &stBitMap);
        if (TD_SUCCESS != s32Ret)
        {
            OSD_LOGE("!!!\n");

            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   rgn_attach
 * @brief      OSD区域叠加到编码通道上
 * @param[in]  RGN_SSV5_S *pRegion rgn参数
 * @param[in]  UINT32 u32EncChan venc hal 通道
 * @param[in]  UINT32 u32DstX,u32DstY dstY 位置信息
 * @param[in]  bTranslucent 是否半透明
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 rgn_attach(RGN_SSV5_S *pstRegion, UINT32 u32EncChan, UINT32 u32DstX, UINT32 u32DstY, UINT32 bTranslucent)
{
    td_s32 s32Ret = TD_SUCCESS;
    ot_rgn_chn_attr *pstRgnChnAttr = &pstRegion->stRgnChnAttr;
    ot_mpp_chn stChn = {0};

    if (TD_FALSE != pstRegion->bAttached)
    {
        return SAL_SOK;
    }

    memset(&pstRegion->stRgnChnAttr, 0, sizeof(ot_rgn_chn_attr));

    stChn.mod_id = OT_ID_VENC;
    stChn.dev_id = 0;
    stChn.chn_id = u32EncChan;

    pstRgnChnAttr->is_show = TD_TRUE;
    pstRgnChnAttr->type = OT_RGN_OVERLAY;
    pstRgnChnAttr->attr.overlay_chn.point.x = u32DstX;
    pstRgnChnAttr->attr.overlay_chn.point.y = u32DstY;
    pstRgnChnAttr->attr.overlay_chn.bg_alpha = bTranslucent ? 64 : 128;
    pstRgnChnAttr->attr.overlay_chn.fg_alpha = 0;
    pstRgnChnAttr->attr.overlay_chn.layer = 0;
    pstRgnChnAttr->attr.overlay_chn.qp_info.is_abs_qp = TD_FALSE;
    pstRgnChnAttr->attr.overlay_chn.qp_info.qp_val = 0;
    pstRgnChnAttr->attr.overlay_chn.qp_info.enable = TD_FALSE; /*??*/
    pstRgnChnAttr->attr.overlay_chn.dst = OT_RGN_ATTACH_JPEG_MAIN;

    s32Ret = ss_mpi_rgn_attach_to_chn(pstRegion->handle, &stChn, pstRgnChnAttr);
    if (TD_SUCCESS != s32Ret)
    {
        OSD_LOGE("HI_MPI_RGN_AttachToChn %#x u32DstX %d u32DstY %d !!!\n", s32Ret, u32DstX, u32DstY);

        return SAL_FAIL;
    }

    pstRegion->bAttached = TD_TRUE;
    OSD_LOGI("u32EncChan = %d,xy[%d,%d]\n", u32EncChan, u32DstX, u32DstY);
    return SAL_SOK;
}

/**
 * @function   rgn_ssV5_destroy
 * @brief      销毁OSD句柄
 * @param[in]  UINT32 chan venc hal层通道号
 * @param[in]  void *pHandle OSD句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 rgn_ssV5_destroy(UINT32 u32EncChan, void *pHandle)
{
    INT32 s32Ret = SAL_SOK;
    ot_mpp_chn stChn = {0};
    RGN_SSV5_S *pstHalRegion = NULL;

    if (NULL == pHandle)
    {
        OSD_LOGE("u32EncChan %d, pHandle is null\n", u32EncChan);
        return SAL_FAIL;
    }

    pstHalRegion = (RGN_SSV5_S *)pHandle;

    if (SAL_FALSE == pstHalRegion->bCreated)
    {
        OSD_LOGE("u32EncChan %d,not create\n", u32EncChan);
        return SAL_SOK;
    }

    if (0xFFFF == pstHalRegion->handle)
    {
        OSD_LOGE("u32EncChan %d,pRegion is null\n", u32EncChan);
        return SAL_SOK;
    }

    stChn.mod_id = OT_ID_VENC;
    stChn.dev_id = 0;
    stChn.chn_id = u32EncChan;

    s32Ret = ss_mpi_rgn_detach_from_chn(pstHalRegion->handle, &stChn);
    if (TD_SUCCESS != s32Ret)
    {
        OSD_LOGE("detach err, %#x!!!\n", s32Ret);

        return SAL_FAIL;
    }

    s32Ret = ss_mpi_rgn_destroy(pstHalRegion->handle);
    if (TD_SUCCESS != s32Ret)
    {
        OSD_LOGE("destroy err, %#x!!!\n", s32Ret);
        return SAL_FAIL;
    }

    pstHalRegion->bAttached = SAL_FALSE;
    pstHalRegion->bCreated = SAL_FALSE;

    return SAL_SOK;
}

/**
 * @function   rgn_ssV5_process
 * @brief      osd叠加处理
 * @param[in]  UINT32 chan venc hal层通道号
 * @param[in]   void *pHandle hal层OSD句柄
 * @param[in]   PUINT8 pCharImgBuf 输入buf
 * @param[in]   UINT32 u32DstW, UINT32 u32DstH, UINT32 u32DstX, UINT32 u32DstY OSD区域信息
 * @param[in]   UINT32 u32Color OSD颜色
 * @param[in]   UINT32 bTranslucent 是否半透明
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 rgn_ssV5_process(UINT32 u32Chan, void *pHandle, PUINT8 pCharImgBuf,
                       UINT32 u32DstW, UINT32 u32DstH, UINT32 u32DstX, UINT32 u32DstY, UINT32 u32Color, UINT32 bTranslucent)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32Stride = 0;
    RGN_SSV5_S *pstRegion = NULL;

    if ((NULL == pHandle) || (NULL == pCharImgBuf))
    {
        OSD_LOGE("prm err\n");
        return SAL_FAIL;
    }

    pstRegion = (RGN_SSV5_S *)pHandle;

    s32Ret = rgn_create(pstRegion, u32Color, u32DstW, u32DstH);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("u32Chan %d!!!\n", u32Chan);
        return SAL_FAIL;
    }

    u32Stride = u32DstW * 2;
    s32Ret = rgn_updateBM(pstRegion, pCharImgBuf, 0, 0, u32DstW, u32DstH, u32Stride, TD_FALSE);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("!!!\n");
        return SAL_FAIL;
    }

    s32Ret = rgn_attach(pstRegion, u32Chan, u32DstX, u32DstY, bTranslucent);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("!!!\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   rgn_ssV5_create
 * @brief    创建OSD句柄
 * @param[in]  UINT32 u32Idx rgn id
 * @param[in]  unsigned char *pInBuf 输入buf，外部申请
 * @param[out] void **ppHandle hal层OSD句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 rgn_ssV5_create(UINT32 u32Idx, void **pHandle, unsigned char *pInBuf)
{
    RGN_SSV5_S *pstRegion = NULL;

    if ((NULL == pHandle) || (NULL == pInBuf))
    {
        OSD_LOGE("inv prm!!!\n");
        return SAL_FAIL;
    }

    memset_s(pInBuf, sizeof(RGN_SSV5_S), 0, sizeof(RGN_SSV5_S));

    pstRegion = (RGN_SSV5_S *)pInBuf;
    pstRegion->handle = u32Idx;

    *pHandle = pInBuf;

    return SAL_SOK;
}

/**
 * @function   rgn_ssV5_getMemSize
 * @brief    获取osd hdl所需内存大小
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
UINT32 rgn_ssV5_getMemSize(void)
{
    return sizeof(RGN_SSV5_S);
}

/**
 * @function   osd_hal_register
 * @brief      注册hal层回调函数
 * @param[in]  None
 * @param[out] OSD_PLAT_OPS_S * 回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_hal_register(OSD_PLAT_OPS_S *pstOsdPlatOps)
{
    if (NULL == pstOsdPlatOps)
    {
        return SAL_FAIL;
    }

    /* 注册hal层处理函数 */
    pstOsdPlatOps->GetMemSize = rgn_ssV5_getMemSize;
    pstOsdPlatOps->Create = rgn_ssV5_create;
    pstOsdPlatOps->Destroy = rgn_ssV5_destroy;
    pstOsdPlatOps->Process = rgn_ssV5_process;

    return SAL_SOK;
}

