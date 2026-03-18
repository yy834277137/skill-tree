/**
 * @file   rgn_rk.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  rgn---OSD处理接口封装
 * @author wangzhenya5
 * @date   2022年03月31日 Create
 * @note
 */

#include <platform_sdk.h>
#include "rgn_rk.h"
#line __LINE__ "rgn_rk.c"

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

/* static RK_S8 ascFont[16] = {0x0  ,0x0  ,0x38 ,0x6c ,0xc6 ,0xc6 ,0xd6 ,0xd6 ,0xc6 ,0xc6 ,0x6c ,0x38 ,0x0  ,0x0  ,0x0  ,0x0 } ; */


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

/*****************************************************************************
                            函数定义
*****************************************************************************/
/* 编码osd 导出位图数据调试代码 */
#if 0
UINT32 dump_osdnum = 0;

INT32 dump_osd(INT32 EncChan, UINT32 RgnHal ,UINT32 len,const UINT8 * const addr)
{
    FILE   *fp;
    char    sztempTri[64] = {0};
    UINT32  WriteNum;
    
    sprintf(sztempTri, "/mnt/bgra5551/Enc%dRgn%ddump_osdnum%d.bgra5551", EncChan, RgnHal,dump_osdnum);
    
    if((fp = fopen(sztempTri, "ab+")) == NULL)
    {
        OSD_LOGE("open file Fail,close!\n");
        return 0;
    }
    fseek(fp,0,SEEK_END);
    
    WriteNum = fwrite(addr, 1, len, fp);
    
    OSD_LOGE("write %d \n ", WriteNum);
    
    fflush(fp);

    fclose(fp);
    
    dump_osdnum ++;

    return 0;
}
#endif

/**
 * @function   rgn_create
 * @brief      创建rgn通道
 * @param[in]  RGN_RK_S *pstRegion rgn参数
 * @param[in]  RK_U32 u32BgColor 背景色
 * @param[in]  RK_U32 u32Width,RK_U32 u32Height rgn大小
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 rgn_create(RGN_RK_S *pstRegion, RK_U32 u32BgColor, RK_U32 u32Width, RK_U32 u32Height)
{
    RK_S32 s32Ret = RK_SUCCESS;
    RGN_ATTR_S *pstRgnAttr = NULL;

    if (NULL == pstRegion)
    {
        OSD_LOGE("is null\n");
        return SAL_FAIL;
    }

    if (RK_FALSE != pstRegion->bCreated)
    {
        return SAL_SOK;
    }

    pstRgnAttr = &pstRegion->stRgnAttr;

    pstRgnAttr->enType = OVERLAY_RGN;

    pstRgnAttr->unAttr.stOverlay.enPixelFmt = RK_FMT_BGRA5551;

    pstRgnAttr->unAttr.stOverlay.stSize.u32Width = SAL_align(u32Width,16);
    pstRgnAttr->unAttr.stOverlay.stSize.u32Height = SAL_align(u32Height,16);
    pstRgnAttr->unAttr.stOverlay.u32CanvasNum = 2;

    s32Ret = RK_MPI_RGN_Create(pstRegion->handle, pstRgnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        OSD_LOGE("RK_MPI_RGN_Create Error : %#x !!!\n", s32Ret);
        return SAL_FAIL;
    }

    pstRegion->bCreated = SAL_TRUE;
    OSD_LOGI("handle %d Width %d Height %d align w*h[%d*%d] BgColor %#x, Created ok\n", pstRegion->handle,u32Width, u32Height, 
        pstRgnAttr->unAttr.stOverlay.stSize.u32Width, pstRgnAttr->unAttr.stOverlay.stSize.u32Height,u32BgColor);

    return SAL_SOK;
}

/**
 * @function   rgn_updateBM
 * @brief      刷新OSD位图
 * @param[in]  RGN_RK_S *pRegion rgn参数
 * @param[in]  void *addr 位图buf
 * @param[in]  RK_S32 x, RK_S32 y,RK_U32 w,RK_U32 h 位图叠加区域信息
 * @param[in]  RK_U32 stride 无效参数
 * @param[in]  RK_U32 bUp2 这个操作没看懂^_^
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 rgn_updateBM(RGN_RK_S *pstRegion, void *pAddr, RK_S32 x, RK_S32 y, RK_U32 w, RK_U32 h, RK_U32 u32Stride, RK_U32 bUp2)
{
    RK_S32 s32Ret = RK_SUCCESS;

    BITMAP_S stBitMap;

    memset(&stBitMap, 0, sizeof(BITMAP_S));

    stBitMap.enPixelFormat = RK_FMT_BGRA5551;
    stBitMap.pData = pAddr;
    stBitMap.u32Height = h;
    stBitMap.u32Width = w;

    s32Ret = RK_MPI_RGN_SetBitMap(pstRegion->handle, &stBitMap);
    if (RK_SUCCESS != s32Ret)
    {
        OSD_LOGE("RGN handle %d Set Bit Map Fail,Prm:enPixelFormat %#x pData %p w %d h %d!!! ret %#x!!!\n", pstRegion->handle, 
                    stBitMap.enPixelFormat, stBitMap.pData, stBitMap.u32Width, stBitMap.u32Height, s32Ret);
        return SAL_FAIL;
    }

    if (RK_TRUE == bUp2)
    {
        s32Ret = RK_MPI_RGN_SetBitMap(pstRegion->handle, &stBitMap);
        if (RK_SUCCESS != s32Ret)
        {
            OSD_LOGE("RGN handle %d Set Bit Map Fail,Prm:enPixelFormat %#x pData %p w %d h %d!!! ret %#x!!!\n", pstRegion->handle, 
                        stBitMap.enPixelFormat, stBitMap.pData, stBitMap.u32Width, stBitMap.u32Height, s32Ret);
            return SAL_FAIL;
        }
    }

    return  SAL_SOK;

}

/**
 * @function   rgn_attach
 * @brief      OSD区域叠加到编码通道上
 * @param[in]  RGN_RK_S *pRegion rgn参数
 * @param[in]  UINT32 u32EncChan venc hal 通道
 * @param[in]  UINT32 u32DstX,u32DstY dstY 位置信息
 * @param[in]  bTranslucent 是否半透明
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 rgn_attach(RGN_RK_S *pstRegion, UINT32 u32EncChan, UINT32 u32DstX, UINT32 u32DstY, UINT32 bTranslucent)
{
    RK_S32 s32Ret = RK_SUCCESS;
    RGN_CHN_ATTR_S *pstRgnChnAttr = &pstRegion->stRgnChnAttr;
    MPP_CHN_S stChn;
    UINT32 u32RgnLayer;

    if (RK_FALSE != pstRegion->bAttached)
    {
        return SAL_SOK;
    }

    memset(&pstRegion->stRgnChnAttr, 0, sizeof(RGN_CHN_ATTR_S));

    stChn.enModId = RK_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = u32EncChan;
    u32RgnLayer = pstRegion->handle % 8;

    pstRgnChnAttr->bShow = RK_TRUE;
    pstRgnChnAttr->enType = OVERLAY_RGN;
    pstRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32X = SAL_alignDown(u32DstX,16);
    pstRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32Y = SAL_alignDown(u32DstY,16);
    pstRgnChnAttr->unChnAttr.stOverlayChn.u32BgAlpha = 0;
    pstRgnChnAttr->unChnAttr.stOverlayChn.u32FgAlpha = 255;
    pstRgnChnAttr->unChnAttr.stOverlayChn.u32Layer = u32RgnLayer;
    pstRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.bEnable = RK_FALSE;
    pstRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.bAbsQp = RK_FALSE;
    pstRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.bForceIntra = RK_FALSE;
    pstRgnChnAttr->unChnAttr.stOverlayChn.stQpInfo.s32Qp = 0;

    s32Ret = RK_MPI_RGN_AttachToChn(pstRegion->handle, &stChn, pstRgnChnAttr);
    if (RK_SUCCESS != s32Ret)
    {
        OSD_LOGE("RGN handle %d RgnLayer %d Attach fail ret %#x EncChan %d align xy[%d,%d] Dstxy[%d,%d]!!!\n",pstRegion->handle, 
                u32RgnLayer,s32Ret, u32EncChan, pstRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32X,
                pstRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32Y,u32DstX, u32DstY);
        return SAL_FAIL;
    }

    pstRegion->bAttached = RK_TRUE;
    OSD_LOGI("RGN handle %d u32EncChan = %d, align xy[%d,%d] xy[%d,%d]u32RgnLayer %d\n", pstRegion->handle,u32EncChan,
        pstRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32X,pstRgnChnAttr->unChnAttr.stOverlayChn.stPoint.s32Y,
        u32DstX, u32DstY,u32RgnLayer);
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
INT32 rgn_rk_destroy(UINT32 u32EncChan, void *pHandle)
{
    INT32 s32Ret = SAL_SOK;
    MPP_CHN_S stChn = {0};
    RGN_RK_S *pstHalRegion = NULL;

    if (NULL == pHandle)
    {
        OSD_LOGE("u32EncChan %d, pHandle is null\n", u32EncChan);
        return SAL_FAIL;
    }

    pstHalRegion = (RGN_RK_S *)pHandle;

    if (SAL_FALSE == pstHalRegion->bCreated)
    {
        OSD_LOGW("u32EncChan %d,not create\n", u32EncChan);
        return SAL_SOK;
    }

    if (0xFFFF == pstHalRegion->handle)
    {
        OSD_LOGE("u32EncChan %d,pRegion is null\n", u32EncChan);
        return SAL_SOK;
    }

    stChn.enModId = RK_ID_VENC;
    stChn.s32DevId = 0;
    stChn.s32ChnId = u32EncChan;
    s32Ret = RK_MPI_RGN_DetachFromChn(pstHalRegion->handle, &stChn);
    if (RK_SUCCESS != s32Ret)
    {
        OSD_LOGE("detach err, %#x!!!\n", s32Ret);

        return SAL_FAIL;
    }

    s32Ret = RK_MPI_RGN_Destroy(pstHalRegion->handle);
    if (RK_SUCCESS != s32Ret)
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
INT32 rgn_rk_process(UINT32 u32Chan, void *pHandle, PUINT8 pCharImgBuf, UINT64 u64PhyAddr,
                       UINT32 u32DstW, UINT32 u32DstH, UINT32 u32DstX, UINT32 u32DstY, UINT32 u32Color, UINT32 bTranslucent)
{
    INT32 s32Ret = SAL_FAIL;
    UINT32 u32Stride = 0;
    RGN_RK_S *pstRegion = NULL;

    if ((NULL == pHandle) || (NULL == pCharImgBuf))
    {
        OSD_LOGE("prm err\n");
        return SAL_FAIL;
    }
    

    SAL_UNUSED(u64PhyAddr);
    
    pstRegion = (RGN_RK_S *)pHandle;

    s32Ret = rgn_create(pstRegion, u32Color, u32DstW, u32DstH);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("u32Chan %d rgn creat Fail!!!\n", u32Chan);
        return SAL_FAIL;
    }

    u32Stride = u32DstW * 2;
    s32Ret = rgn_updateBM(pstRegion, pCharImgBuf, 0, 0, u32DstW, u32DstH, u32Stride, RK_FALSE);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("u32Chan %d rgn updateBM Fail!!!\n", u32Chan);
        return SAL_FAIL;
    }

    s32Ret = rgn_attach(pstRegion, u32Chan, u32DstX, u32DstY, bTranslucent);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("u32Chan %d rgn attach Fail!!!\n", u32Chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
* @function   rgn_rk_create
* @brief    创建OSD句柄
* @param[in]  UINT32 u32Idx rgn id
* @param[in]  unsigned char *pInBuf 输入buf，外部申请
* @param[out] void **ppHandle hal层OSD句柄
* @return      int  成功SAL_SOK，失败SAL_FAIL
*/
INT32 rgn_rk_create(UINT32 u32Start, UINT32 u32Idx, void **pHandle, unsigned char *pInBuf)
{
   RGN_RK_S *pstRegion = NULL;

   if ((NULL == pHandle) || (NULL == pInBuf))
   {
       OSD_LOGE("inv prm!!!\n");
       return SAL_FAIL;
   }

   SAL_UNUSED(u32Start);

   memset(pInBuf, 0, sizeof(RGN_RK_S));

   pstRegion = (RGN_RK_S *)pInBuf;
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
UINT32 rgn_rk_getMemSize(void)
{
    return sizeof(RGN_RK_S);
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
    pstOsdPlatOps->GetMemSize = rgn_rk_getMemSize;
    pstOsdPlatOps->Create = rgn_rk_create;
    pstOsdPlatOps->Destroy = rgn_rk_destroy;
    pstOsdPlatOps->Process = rgn_rk_process;

    return SAL_SOK;
}

