/**
 * @file   rgn_hisi.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  rgn---OSD处理接口封装
 * @author wangweiqin
 * @date   2017年09月08日 Create
 * @note
 * @note \n History
   1.Date        : 2017年09月08日 Create
     Author      : wangweiqin
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#include <platform_sdk.h>
#include "rgn_hisi.h"

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
static HI_S32 OsdRectCheck(TDE2_RECT_S *pRect, TDE2_RECT_S *pDstRect)
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
        return HI_TRUE;
    }
    else
    {
        return HI_FALSE;
    }
}

#endif

/*****************************************************************************
                            函数定义
*****************************************************************************/

/**
 * @function   rgn_create
 * @brief      创建rgn通道
 * @param[in]  RGN_HISI_S *pstRegion rgn参数
 * @param[in]  HI_U32 u32BgColor 背景色
 * @param[in]  HI_U32 u32Width,HI_U32 u32Height rgn大小
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 rgn_create(RGN_HISI_S *pstRegion, HI_U32 u32BgColor, HI_U32 u32Width, HI_U32 u32Height)
{
    HI_S32 s32Ret = HI_SUCCESS;
    RGN_ATTR_S *pstRgnAttr = NULL;

    if (NULL == pstRegion)
    {
        OSD_LOGE("is null\n");
        return SAL_FAIL;
    }

    if (HI_FALSE != pstRegion->bCreated)
    {
        return SAL_SOK;
    }

    pstRgnAttr = &pstRegion->stRgnAttr;

    pstRgnAttr->enType = OVERLAY_RGN;
    pstRgnAttr->unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_ARGB_1555;
    pstRgnAttr->unAttr.stOverlay.u32BgColor = u32BgColor;
    pstRgnAttr->unAttr.stOverlay.stSize.u32Width = u32Width;
    pstRgnAttr->unAttr.stOverlay.stSize.u32Height = u32Height;

    pstRgnAttr->unAttr.stOverlay.u32CanvasNum = 2;

    s32Ret = HI_MPI_RGN_Create(pstRegion->handle, pstRgnAttr);
    if (HI_SUCCESS != s32Ret)
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
 * @param[in]  RGN_HISI_S *pRegion rgn参数
 * @param[in]  void *addr 位图buf
 * @param[in]  HI_S32 x, HI_S32 y,HI_U32 w,HI_U32 h 位图叠加区域信息
 * @param[in]  HI_U32 stride 无效参数
 * @param[in]  HI_U32 bUp2 这个操作没看懂^_^
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 rgn_updateBM(RGN_HISI_S *pstRegion, void *pAddr, HI_S32 x, HI_S32 y, HI_U32 w, HI_U32 h, HI_U32 u32Stride, HI_U32 bUp2)
{
    HI_S32 s32Ret = HI_SUCCESS;

    BITMAP_S stBitMap;

    memset(&stBitMap, 0, sizeof(BITMAP_S));

    stBitMap.enPixelFormat = PIXEL_FORMAT_ARGB_1555;
    stBitMap.pData = pAddr;
    stBitMap.u32Height = h;
    stBitMap.u32Width = w;

    s32Ret = HI_MPI_RGN_SetBitMap(pstRegion->handle, &stBitMap);
    if (HI_SUCCESS != s32Ret)
    {
        OSD_LOGE("ret %#x!!!\n", s32Ret);

        return SAL_FAIL;
    }

    if (HI_TRUE == bUp2)
    {
        s32Ret = HI_MPI_RGN_SetBitMap(pstRegion->handle, &stBitMap);
        if (HI_SUCCESS != s32Ret)
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
 * @param[in]  RGN_HISI_S *pRegion rgn参数
 * @param[in]  UINT32 u32EncChan venc hal 通道
 * @param[in]  UINT32 u32DstX,u32DstY dstY 位置信息
 * @param[in]  bTranslucent 是否半透明
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 rgn_attach(RGN_HISI_S *pstRegion, UINT32 u32EncChan, UINT32 u32DstX, UINT32 u32DstY, UINT32 bTranslucent)
{
    HI_S32 s32Ret = HI_SUCCESS;
    RGN_CHN_ATTR_S *pstRgnChnAttr = &pstRegion->stRgnChnAttr;
    MPP_CHN_S stChn;

    if (HI_FALSE != pstRegion->bAttached)
    {
        return SAL_SOK;
    }

    memset(&pstRegion->stRgnChnAttr, 0, sizeof(RGN_CHN_ATTR_S));

    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = u32EncChan;

    pstRgnChnAttr->bShow = HI_TRUE;
    pstRgnChnAttr->enType = OVERLAY_RGN;
    pstRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32X = u32DstX;
    pstRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32Y = u32DstY;
    pstRgnChnAttr->unChnAttr.stOverlayChn.u32BgAlpha = bTranslucent ? 64 : 128;
    pstRgnChnAttr->unChnAttr.stOverlayChn.u32FgAlpha = 0;
    pstRgnChnAttr->unChnAttr.stOverlayChn.u32Layer = 0;
    pstRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.bAbsQp = HI_FALSE;
    pstRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.s32Qp = 0;
    pstRgnChnAttr->unChnAttr.stOverlayChn.enAttachDest = ATTACH_JPEG_MAIN;

    s32Ret = HI_MPI_RGN_AttachToChn(pstRegion->handle, &stChn, pstRgnChnAttr);
    if (HI_SUCCESS != s32Ret)
    {
        OSD_LOGE("HI_MPI_RGN_AttachToChn %#x u32DstX %d u32DstY %d !!!\n", s32Ret, u32DstX, u32DstY);

        return SAL_FAIL;
    }

    pstRegion->bAttached = HI_TRUE;
    OSD_LOGI("u32EncChan = %d,xy[%d,%d]\n", u32EncChan, u32DstX, u32DstY);
    return SAL_SOK;
}

/**
 * @function   rgn_hisi_destroy
 * @brief      销毁OSD句柄
 * @param[in]  UINT32 chan venc hal层通道号
 * @param[in]  void *pHandle OSD句柄
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 rgn_hisi_destroy(UINT32 u32EncChan, void *pHandle)
{
    INT32 s32Ret = SAL_SOK;
    MPP_CHN_S stChn = {0};
    RGN_HISI_S *pstHalRegion = NULL;

    if (NULL == pHandle)
    {
        OSD_LOGE("u32EncChan %d, pHandle is null\n", u32EncChan);
        return SAL_FAIL;
    }

    pstHalRegion = (RGN_HISI_S *)pHandle;

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

    stChn.enModId = HI_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = u32EncChan;
    s32Ret = HI_MPI_RGN_DetachFromChn(pstHalRegion->handle, &stChn);
    if (HI_SUCCESS != s32Ret)
    {
        OSD_LOGE("detach err, %#x!!!\n", s32Ret);

        return SAL_FAIL;
    }

    s32Ret = HI_MPI_RGN_Destroy(pstHalRegion->handle);
    if (HI_SUCCESS != s32Ret)
    {
        OSD_LOGE("destroy err, %#x!!!\n", s32Ret);
        return SAL_FAIL;
    }

    pstHalRegion->bAttached = SAL_FALSE;
    pstHalRegion->bCreated = SAL_FALSE;

    return SAL_SOK;
}

/**
 * @function   rgn_hisi_process
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
INT32 rgn_hisi_process(UINT32 u32Chan, void *pHandle, PUINT8 pCharImgBuf, UINT64 u64PhyAddr,
                       UINT32 u32DstW, UINT32 u32DstH, UINT32 u32DstX, UINT32 u32DstY, UINT32 u32Color, UINT32 bTranslucent)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32Stride = 0;
    RGN_HISI_S *pstRegion = NULL;

    if ((NULL == pHandle) || (NULL == pCharImgBuf))
    {
        OSD_LOGE("prm err\n");
        return SAL_FAIL;
    }

    pstRegion = (RGN_HISI_S *)pHandle;

    s32Ret = rgn_create(pstRegion, u32Color, u32DstW, u32DstH);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("u32Chan %d!!!\n", u32Chan);
        return SAL_FAIL;
    }

    u32Stride = u32DstW * 2;
    s32Ret = rgn_updateBM(pstRegion, pCharImgBuf, 0, 0, u32DstW, u32DstH, u32Stride, HI_FALSE);
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
 * @function   rgn_hisi_create
 * @brief    创建OSD句柄
 * @param[in]  UINT32 u32Idx rgn id
 * @param[in]  unsigned char *pInBuf 输入buf，外部申请
 * @param[out] void **ppHandle hal层OSD句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 rgn_hisi_create(UINT32 u32Start, UINT32 u32Idx, void **pHandle, unsigned char *pInBuf)
{
    RGN_HISI_S *pstRegion = NULL;

    if ((NULL == pHandle) || (NULL == pInBuf))
    {
        OSD_LOGE("inv prm!!!\n");
        return SAL_FAIL;
    }

    SAL_UNUSED(u32Start);

    memset_s(pInBuf, sizeof(RGN_HISI_S),0, sizeof(RGN_HISI_S));

    pstRegion = (RGN_HISI_S *)pInBuf;
    pstRegion->handle = u32Idx;

    *pHandle = pInBuf;

    return SAL_SOK;
}

/**
 * @function   rgn_hisi_getMemSize
 * @brief    获取osd hdl所需内存大小
 * @param[in]  None
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
UINT32 rgn_hisi_getMemSize(void)
{
    return sizeof(RGN_HISI_S);
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
    pstOsdPlatOps->GetMemSize = rgn_hisi_getMemSize;
    pstOsdPlatOps->Create = rgn_hisi_create;
    pstOsdPlatOps->Destroy = rgn_hisi_destroy;
    pstOsdPlatOps->Process = rgn_hisi_process;

    return SAL_SOK;
}

