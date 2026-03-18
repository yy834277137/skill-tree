/**
 * @file   fb_rk3588.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  RK3588图形层接口
 * @author yeyanzhong
 * @date   2022.3.15
 * @note
 * @note \n History
   1.日    期: 2022.3.15
     作    者: yeyanzhong
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include <platform_sdk.h>
#include "fb_rk3588.h"
#include "../../hal/hal_inc_inter/fb_hal_inter.h"



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
static FB_PLAT_OPS g_stFbPlatOps;
extern FB_COMMON g_fb_common[FB_DEV_NUM_MAX];


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */


/*******************************************************************************
* 函数名  : fb_rkGetDevNo
* 描  述  : 对上层传下来的设备号进行转换
* 输  入  : - u32DevNoth  
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static UINT32 fb_rkGetDevNo(UINT32 u32DevNo)
{
    UINT32 u32BoardType;

    u32BoardType = HARDWARE_GetBoardType();

    /* DB_TS3719_V1_0板子使用的设备号是2和3的mipi输出 */
    if (DB_TS3719_V1_0 == u32BoardType || DB_RS20046_V1_0 == u32BoardType)
    {

        return u32DevNo + 2;

    }

    return u32DevNo;
}


/**
 * @function   fb_rk3588RefreshUiFb
 * @brief      刷新图形层
 * @param[in]  UINT32 uiChn        fb设备号
 * @param[in]  FB_COMMON *pfb_chn  fb信息
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_rk3588RefreshUiFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{
    RK_S32 s32Ret = 0;
    VIDEO_FRAME_INFO_S stVideoFrame = {0};
    PIXEL_FORMAT_E enPixFormat = RK_FMT_RGBA8888;
    
    switch (pfb_chn->bitWidth)
    {
        case 32:
        {
            enPixFormat = RK_FMT_BGRA8888;  /*todo: vo layer配置时ARGB格式会报错，RGBA不会报错 */
            break;
        }
        case 16:
        {
            enPixFormat = RK_FMT_RGBA5551;
            break;
        }
        default:
        {
            enPixFormat = RK_FMT_RGBA8888;
            break;
        }
    }

    stVideoFrame.stVFrame.pMbBlk = RK_MPI_MB_VirAddr2Handle(pfb_chn->fb_dev.pBuf);
    stVideoFrame.stVFrame.u32Width = pfb_chn->fb_dev.uiWidth;
    stVideoFrame.stVFrame.u32Height = pfb_chn->fb_dev.uiHeight;
    stVideoFrame.stVFrame.u32VirWidth = pfb_chn->fb_dev.uiWidth;
    stVideoFrame.stVFrame.u32VirHeight = pfb_chn->fb_dev.uiHeight;
    stVideoFrame.stVFrame.enPixelFormat = enPixFormat;

    DISP_LOGD("uiChn %d , bitWidth %d \n", uiChn, pfb_chn->bitWidth);
    s32Ret = RK_MPI_VO_SendLayerFrame(uiChn + VO_LAYER_ESMART0, &stVideoFrame);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("RK_MPI_VO_SendLayerFrame, ret 0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    
    return SAL_SOK;
}

/**
 * @function   fb_rk3588RefreshMouseFb
 * @brief      刷新鼠标
 * @param[in]  UINT32 uiChn               设备号
 * @param[in]  FB_COMMON *pfb_chn         fb设备信息
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  显示坐标
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_rk3588RefreshMouseFb(UINT32 uiChn,FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr)
{
    UINT8 *pu8Src = (UINT8 *)pfb_chn->fb_app.Viraddr;
    UINT32 *pu32Dst = (UINT32 *)pfb_chn->fb_dev.pShowScreen;
    VIDEO_FRAME_INFO_S stVideoFrame = {0};
    PIXEL_FORMAT_E enPixFormat = RK_FMT_BGRA8888;
    RK_U32 s32Ret = RK_SUCCESS;

    if(pfb_chn->DevMMAP == SAL_FALSE || pfb_chn->AppMMAP == SAL_FALSE)
    {
        SAL_ERROR("fb%u is not creat,devmmap:%u,appmmap:%u\n",uiChn, pfb_chn->DevMMAP, pfb_chn->AppMMAP);
    }

 #if 0
    UINT32 i = 0, j = 0;
    UINT32 u32Idx = 0;
    UINT32 u32Alpha = pstHalFbAttr->stScreenAttr[0].alpha_idx;
    
    /* 把app下发的颜色索引值转换成ARGB, hisi刷fb时下发的是ARGB值 */
    for (i = 0; i < pstHalFbAttr->stScreenAttr[0].height; i++)
    {
        for (j = 0; j < pstHalFbAttr->stScreenAttr[0].width; j += 2)
        {
            u32Idx = (*pu8Src >> 4) & 0x0F;
            *pu32Dst++ = (u32Idx == u32Alpha) ? 0x00 : 0xFF000000 | pstHalFbAttr->stScreenAttr[0].palette[u32Idx];

            u32Idx = (*pu8Src) & 0x0F;
            *pu32Dst++ = (u32Idx == u32Alpha) ? 0x00 : 0xFF000000 | pstHalFbAttr->stScreenAttr[0].palette[u32Idx];

            pu8Src++;
        }
    }
#else
    /* app 下发ARGB8888 */
    memcpy((void*)pu32Dst, pu8Src, pstHalFbAttr->stScreenAttr[0].width * pstHalFbAttr->stScreenAttr[0].height * 4); //ARGB888拷贝
#endif
    stVideoFrame.stVFrame.pMbBlk = RK_MPI_MB_VirAddr2Handle(pfb_chn->fb_dev.pBuf);
    stVideoFrame.stVFrame.u32Width = pfb_chn->fb_dev.uiWidth;
    stVideoFrame.stVFrame.u32Height = pfb_chn->fb_dev.uiHeight;
    stVideoFrame.stVFrame.u32VirWidth = pfb_chn->fb_dev.uiWidth;
    stVideoFrame.stVFrame.u32VirHeight = pfb_chn->fb_dev.uiHeight;
    stVideoFrame.stVFrame.enPixelFormat = enPixFormat;

    s32Ret = RK_MPI_SYS_MmzFlushCache(stVideoFrame.stVFrame.pMbBlk, RK_FALSE);
    if(RK_SUCCESS != s32Ret)
    {
        DISP_LOGE("RK_MPI_SYS_MmzFlushCache, ret 0x%x\n", s32Ret);
        return SAL_FAIL;
    }
    
    s32Ret = RK_MPI_VO_SendLayerFrame(uiChn + VO_LAYER_ESMART0, &stVideoFrame);
    if(RK_SUCCESS != s32Ret)
    {
        DISP_LOGE("RK_MPI_VO_SendLayerFrame, ret 0x%x\n", s32Ret);
        return SAL_FAIL;
    }
    
    pfb_chn->fb_dev.uiX = pstHalFbAttr->stScreenAttr[0].x;
    pfb_chn->fb_dev.uiY = pstHalFbAttr->stScreenAttr[0].y;
    
    s32Ret = RK_MPI_VO_SetCursorPostion(uiChn + VO_LAYER_ESMART0, pfb_chn->fb_dev.uiX, pfb_chn->fb_dev.uiY);
    if(RK_SUCCESS != s32Ret)
    {
        DISP_LOGE("RK_MPI_VO_SetCursorPostion, ret 0x%x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   fb_rk3588CreateDevFb3
 * @brief      创建G3显示，鼠标层显示设备
 * @param[in]  UINT32 uiChn                   fb设备号
 * @param[in]  FB_COMMON *pfb_chn             fb设备信息
 * @param[in]  FB_INIT_ATTR_ST *pstHalFbAttr  fb坐标区域
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_rk3588CreateDevFb3(UINT32 uiChn,FB_COMMON *pfb_chn, FB_INIT_ATTR_ST *pstHalFbAttr)
{
    UINT32 bindChn      = 0;
    FB_COMMON *pUiFb_chn = NULL;     
    RK_U32 s32Ret;
    VO_LAYER                   s32VoLayer;
    RK_S32                     s32VoDev;
    VO_VIDEO_LAYER_ATTR_S    stLayerAttr;
    RK_VOID *pMblk;
    PIXEL_FORMAT_E enPixFormat = RK_FMT_BGRA8888;
    UINT32 u32BuffSize;    
    
    if (NULL == pstHalFbAttr)
    {
        SAL_ERROR("pstHalFbAttr is null !!\n");
        return SAL_FAIL;
    }    

    bindChn = pfb_chn->mouseNewChn;
    pUiFb_chn = &g_fb_common[bindChn];
    
    //鼠标必须在UI的FB MAP后才允许MAP
    if(pUiFb_chn->DevMMAP != SAL_TRUE && pUiFb_chn->AppMMAP != SAL_TRUE)
    {
        DISP_LOGE("Fb%d is Umap !\n",bindChn);
        return SAL_FAIL;
    }

    pfb_chn->fb_dev.uiWidth  = pstHalFbAttr->uiW; //这里需要更新为fb的宽高信息
    pfb_chn->fb_dev.uiHeight = pstHalFbAttr->uiH;
    pfb_chn->mouseLastChn    = pfb_chn->mouseNewChn;

    s32VoDev = fb_rkGetDevNo(bindChn);
    s32VoLayer = uiChn + VO_LAYER_ESMART0;
    memset(&stLayerAttr, 0, sizeof(stLayerAttr)); 
    
    RK_MPI_VO_BindLayer(s32VoLayer, s32VoDev, VO_LAYER_MODE_CURSOR);  //voDev要处于关闭状态

    u32BuffSize = RK_MPI_VO_CreateGraphicsFrameBuffer(pfb_chn->fb_dev.uiWidth , pfb_chn->fb_dev.uiHeight, enPixFormat, &pMblk);
    if (u32BuffSize == 0) {
        DISP_LOGE("can not create gfx buffer\n");
        goto err0;
    }

    //RK_MPI_VO_GetFrameInfo(*pMblk, &stFrameInfo);

    pfb_chn->fb_dev.ui64CanvasAddr = 0;
    pfb_chn->fb_dev.pBuf = RK_MPI_MB_Handle2VirAddr(pMblk);
    if(NULL == pfb_chn->fb_dev.pBuf)
    {
        DISP_LOGE("RK_MPI_MB_Handle2VirAddr failed !\n");
        goto err1;
    }
    
    pfb_chn->fb_dev.pShowScreen = pfb_chn->fb_dev.pBuf;
    pfb_chn->fb_dev.offset = pfb_chn->fb_dev.uiWidth * pfb_chn->fb_dev.uiHeight * pfb_chn->u32BytePerPixel; //hisi里多buffer时才会用到该字段
    pfb_chn->fb_dev.uiSize = pfb_chn->fb_dev.uiWidth * pfb_chn->fb_dev.uiHeight * pfb_chn->u32BytePerPixel; //hisi里多buffer时才会用到该字段
    pfb_chn->fb_dev.uiSmem_len = pfb_chn->fb_dev.uiSize;
    pfb_chn->fb_dev.vbBlk = (UINT64)pMblk;
    memset(pfb_chn->fb_dev.pBuf, 0x00, pfb_chn->fb_dev.uiSize);

    s32Ret = RK_MPI_VO_GetLayerAttr(s32VoLayer, &stLayerAttr);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("RK_MPI_VO_GetLayerAttr, ret 0x%x\n", s32Ret);
        goto err1;
    }

    stLayerAttr.enPixFormat                  = enPixFormat;
    stLayerAttr.stDispRect.s32X              = 0;
    stLayerAttr.stDispRect.s32Y              = 0;
    stLayerAttr.stDispRect.u32Width          = pfb_chn->fb_dev.uiWidth;
    stLayerAttr.stDispRect.u32Height         = pfb_chn->fb_dev.uiHeight;
    stLayerAttr.stImageSize.u32Width         = pfb_chn->fb_dev.uiWidth;
    stLayerAttr.stImageSize.u32Height        = pfb_chn->fb_dev.uiHeight;

    s32Ret = RK_MPI_VO_SetLayerAttr(s32VoLayer, &stLayerAttr);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("RK_MPI_VO_SetLayerAttr, ret 0x%x\n", s32Ret);
        goto err1;
    }

    s32Ret = RK_MPI_VO_EnableLayer(s32VoLayer);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("RK_MPI_VO_EnableLayer, ret 0x%x\n", s32Ret);
        goto err1;
    }

        
    DISP_LOGI("Dev fb %d Init Success\n",uiChn);

    return SAL_SOK;
    
err1:
    RK_MPI_VO_DestroyGraphicsFrameBuffer(pMblk);

err0:
    RK_MPI_VO_UnBindLayer(s32VoLayer, s32VoDev); 

    return SAL_FAIL;

}

/**
 * @function   fb_rk3588DeleteFb
 * @brief      释放/销毁显示FB
 * @param[in]  UINT32 uiChn                   fb设备号
 * @param[in]  FB_COMMON *pfb_chn             fb设备信息
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_rk3588DeleteFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{
    RK_U32 s32Ret;
    VO_LAYER                   s32VoLayer;
    RK_S32                     s32VoDev;
    VO_VIDEO_LAYER_ATTR_S    stLayerAttr;
    RK_VOID *pMblk = NULL;

    s32VoDev = fb_rkGetDevNo(uiChn);
    s32VoLayer = uiChn + VO_LAYER_ESMART0;
    memset(&stLayerAttr, 0, sizeof(stLayerAttr));

    /* 如果是鼠标层，关闭图层需要关闭当前使能的鼠标层 */
    if(VO_LAYER_ESMART2 == s32VoLayer)
    {
        s32VoLayer = uiChn + VO_LAYER_ESMART0 + pfb_chn->mouseLastChn;
        s32VoDev = fb_rkGetDevNo(pfb_chn->mouseLastChn);
    }

    s32Ret = RK_MPI_VO_DisableLayer(s32VoLayer);
    if (s32Ret != RK_SUCCESS)
    {
       DISP_LOGE("RK_MPI_VO_DisableLayer error\n");
    }

    s32Ret = RK_MPI_VO_UnBindLayer(s32VoLayer, s32VoDev); 
    if (s32Ret != RK_SUCCESS)
    {
       DISP_LOGE("RK_MPI_VO_UnBindLayer Layer%d VodDve%d error 0x%x\n",s32VoLayer,s32VoDev,s32Ret);
    }

    pMblk = RK_MPI_MB_VirAddr2Handle(pfb_chn->fb_dev.pBuf);
    if (MB_INVALID_HANDLE != pMblk)
    {
        RK_MPI_VO_DestroyGraphicsFrameBuffer(pMblk);
    }
    else
    {
         DISP_LOGE("RK_MPI_MB_VirAddr2Handle error\n");
    }

    return SAL_SOK;
}

/**
 * @function   fb_rk3588CreateDevFb
 * @brief      创建普通显示设备
 * @param[in]  UINT32 uiChn                   fb设备号
 * @param[in]  FB_COMMON *pfb_chn             fb设备信息
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_rk3588CreateDevFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{   
    RK_U32 s32Ret;
    VO_LAYER                   s32VoLayer;
    RK_S32                     s32VoDev;
    VO_VIDEO_LAYER_ATTR_S    stLayerAttr = {0};
    RK_VOID *pMblk;
    PIXEL_FORMAT_E enPixFormat = RK_FMT_BGRA8888; //RK_FMT_RGBA8888;fb_rk3588CreateDevFb3
    UINT32 u32BuffSize;

    s32VoDev = fb_rkGetDevNo(uiChn);
    s32VoLayer = uiChn + VO_LAYER_ESMART0;
    memset(&stLayerAttr, 0, sizeof(stLayerAttr));
    
    s32Ret = RK_MPI_VO_BindLayer(s32VoLayer, s32VoDev, VO_LAYER_MODE_GRAPHIC);  //voDev要处于关闭状态
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("RK_MPI_VO_BindLayer layer:%d vodev %d, ret 0x%x\n",s32VoLayer ,s32VoDev,s32Ret);
        return SAL_FAIL;
    }


    u32BuffSize = RK_MPI_VO_CreateGraphicsFrameBuffer(pfb_chn->fb_dev.uiWidth , pfb_chn->fb_dev.uiHeight, enPixFormat, &pMblk);
    if (u32BuffSize == 0)
    {
        DISP_LOGE("can not create gfx buffer\n");
        goto err0;
    }

    pfb_chn->fb_dev.ui64CanvasAddr = 0;
    pfb_chn->fb_dev.pBuf = RK_MPI_MB_Handle2VirAddr(pMblk);
    if(NULL == pfb_chn->fb_dev.pBuf)
    {
        DISP_LOGE("RK_MPI_MB_Handle2VirAddr failed\n");
        goto err1;
    }
    pfb_chn->fb_dev.offset = pfb_chn->fb_dev.uiWidth * pfb_chn->fb_dev.uiHeight * pfb_chn->u32BytePerPixel; //hisi里多buffer时才会用到该字段
    pfb_chn->fb_dev.uiSize = pfb_chn->fb_dev.uiWidth * pfb_chn->fb_dev.uiHeight * pfb_chn->u32BytePerPixel; //hisi里多buffer时才会用到该字段
    pfb_chn->fb_dev.vbBlk = (UINT64)pMblk;
    memset(pfb_chn->fb_dev.pBuf, 0x00, pfb_chn->fb_dev.uiSize);

    s32Ret = RK_MPI_VO_GetLayerAttr(s32VoLayer, &stLayerAttr);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("RK_MPI_VO_GetLayerAttr, ret 0x%x\n", s32Ret);
        goto err1;
    }
    stLayerAttr.enPixFormat                  = enPixFormat;
    stLayerAttr.stDispRect.s32X              = 0;
    stLayerAttr.stDispRect.s32Y              = 0;
    stLayerAttr.stDispRect.u32Width          = pfb_chn->fb_dev.uiWidth;
    stLayerAttr.stDispRect.u32Height         = pfb_chn->fb_dev.uiHeight;
    stLayerAttr.stImageSize.u32Width         = pfb_chn->fb_dev.uiWidth;
    stLayerAttr.stImageSize.u32Height        = pfb_chn->fb_dev.uiHeight;
    stLayerAttr.u32DispFrmRt                 = 25;

    s32Ret = RK_MPI_VO_SetLayerAttr(s32VoLayer, &stLayerAttr);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("RK_MPI_VO_SetLayerAttr %u, ret 0x%x\n",s32VoLayer, s32Ret);
        goto err1;
    }

    s32Ret = RK_MPI_VO_EnableLayer(s32VoLayer);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("RK_MPI_VO_EnableLayer, ret 0x%x\n", s32Ret);
        goto err1;
    }
        
    DISP_LOGI("Dev fb %d Init Success\n",uiChn);
    return SAL_SOK;

err1:
    RK_MPI_VO_DestroyGraphicsFrameBuffer(pMblk);

err0:
    RK_MPI_VO_UnBindLayer(s32VoLayer, s32VoDev); 

    return SAL_FAIL;
}

/**
 * @function   fb_rk3588CreateAppFb
 * @brief      创建给应用的显示缓存
 * @param[in]  UINT32 uiChn                   fb设备号
 * @param[in]  FB_COMMON *pfb_chn             fb设备信息
 * @param[in]  SCREEN_ATTR *pstHalFbAtt       fb坐标区域
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_rk3588CreateAppFb(UINT32 uiChn,FB_COMMON *pfb_chn, SCREEN_ATTR *pstHalFbAttr)
{
    UINT32 uiSize = 0;
    INT32 s32Ret = 0;
    ALLOC_VB_INFO_S stVbInfo = {0};

    if(uiChn >= FB_DEV_NUM_MAX)
    {
        SAL_ERROR("uiChn not exist, chan = %d!!\n", uiChn);
        return SAL_FAIL;
    }
    
    pfb_chn->fb_app.uiWidth   = pstHalFbAttr->width;
    pfb_chn->fb_app.uiHeight  = pstHalFbAttr->height;
        
    uiSize = (pfb_chn->fb_app.uiWidth * pfb_chn->fb_app.uiHeight * pstHalFbAttr->bitWidth) >> 3;

    if((GRAPHICS_LAYER_G3 != uiChn) || (pfb_chn->AppMMAP == SAL_TRUE))
    {
        uiSize = SAL_align(uiSize, 16);
    }
    else
    {
        if(uiSize == 0x00)
        {
             SAL_ERROR("uiSize %d!!\n", uiSize);
             return SAL_FAIL;
         }
    }

    SAL_LOGI("MMAP : App Get Buff uiWidth %d uiHeight %d size %d !!!\n", pfb_chn->fb_app.uiWidth, pfb_chn->fb_app.uiHeight, uiSize);

    s32Ret = mem_hal_vbAlloc(uiSize, "FB_RK3588", "appFb", NULL, SAL_FALSE, &stVbInfo);
    if(s32Ret != SAL_SOK)
    {
        DISP_LOGE("mem_hal_vbAllocNoHead failed \n");
        return SAL_FAIL;
    }

    pfb_chn->fb_app.Viraddr = stVbInfo.pVirAddr;
    pfb_chn->fb_app.ui64Phyaddr = stVbInfo.u64PhysAddr; //RK里物理地址为0
    pfb_chn->fb_app.vbBlk = stVbInfo.u64VbBlk;
    pfb_chn->fb_app.u32PoolId = stVbInfo.u32PoolId;

    memset(pfb_chn->fb_app.Viraddr, 0x00, stVbInfo.u32Size);
    pfb_chn->fb_app.uiSize = stVbInfo.u32Size;
    pstHalFbAttr->srcAddr  = pfb_chn->fb_app.Viraddr;
    pstHalFbAttr->srcSize  = stVbInfo.u32Size;
    pstHalFbAttr->isUse    = 1;
    
    SAL_LOGI("FB%d App Buff MMZ ADDR : %lu SIZE : 0x%x !!!\n",uiChn, (UINTL)pfb_chn->fb_app.Viraddr, uiSize);

    return SAL_SOK;
}

/**
 * @function   fb_rk3588DeleteAppFb
 * @brief      释放给应用的显示缓存
 * @param[in]  UINT32 uiChn                   fb设备号
 * @param[in]  FB_COMMON *pfb_chn             fb设备信息
 * @param[in]  SCREEN_ATTR *pstHalFbAtt       fb坐标区域
 * @param[out] None
 * @return     static INT32 SAL_SOK  成功
 *                          SAL_FAIL 失败
 */
static INT32 fb_rk3588DeleteAppFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{
    INT32 s32Ret = 0;

    if(FB_DEV_NUM_MAX <= uiChn)
    {
        SAL_ERROR("uiChn is err, uiChn = %d\n",uiChn);
        return SAL_FAIL;
    }
    
    if(pfb_chn->fb_app.Viraddr)
    {
        s32Ret = mem_hal_vbFree(pfb_chn->fb_app.Viraddr, "FB_RK3588", "appFb", pfb_chn->fb_app.uiSize, pfb_chn->fb_app.vbBlk, pfb_chn->fb_app.u32PoolId);
        if(SAL_SOK != s32Ret)
        {
            SAL_ERROR("chan %d error %x\n",uiChn,s32Ret);
            return SAL_FAIL;
        }
    }
    
    pfb_chn->fb_app.Viraddr = NULL;
    
    return SAL_SOK;
}



/**
 * @function   fb_rk3588_SetMouseFbChn
 * @brief      切换鼠标通道绑定到对用VO设备通道
 * @param[in]  UINT32 uiChn        设备号
 * @param[in]  UINT32 uiBindChn    绑定的设备号
 * @param[in]  FB_COMMON *pfb_chn  设备信息
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_rk3588_SetMouseFbChn(UINT32 uiChn, UINT32 uiBindChn, FB_COMMON *pfb_chn)
{

    VO_LAYER                   s32VoLastLayer;
    VO_LAYER                   s32VoNewLayer;
    RK_S32                     s32VoLastDev;
    RK_S32                     s32VoNewDev;
    RK_U32 s32Ret;
    PIXEL_FORMAT_E enPixFormat = RK_FMT_BGRA8888;

    /*hisi3559AV100默认G3为鼠标*/
    FB_COMMON *pfb3_chn = &g_fb_common[GRAPHICS_LAYER_G3];  
    FB_COMMON *pfb0_chn = pfb_chn;
    VO_VIDEO_LAYER_ATTR_S    stLayerAttr;

    memset(&stLayerAttr, 0, sizeof(stLayerAttr)); 

    if (uiBindChn == pfb3_chn->mouseLastChn)
    {
        SAL_ERROR("uiBindChn %d is sam\n",pfb3_chn->mouseLastChn);
        return SAL_SOK;
    }

    pfb3_chn->mouseNewChn = uiBindChn; 

    s32VoLastDev = fb_rkGetDevNo(pfb3_chn->mouseLastChn);
    s32VoNewDev =  fb_rkGetDevNo(pfb3_chn->mouseNewChn);

    /*鼠标必须在UI的FB MAP后才允许MAP*/
    if(pfb0_chn->DevMMAP != SAL_TRUE && pfb0_chn->AppMMAP != SAL_TRUE)
    {
        SAL_ERROR("Fb0 is Umap !\n");
        return SAL_FAIL;
    }

    /* RK不支持鼠标层动态绑定，每个UI图形层需要单独绑定鼠标层*/
    /*故:VO_LAYER_ESMART0绑定VO_LAYER_ESMART2,VO_LAYER_ESMART1绑定VO_LAYER_ESMART3 */
    s32VoLastLayer = GRAPHICS_LAYER_G3 + VO_LAYER_ESMART0 + pfb3_chn->mouseLastChn;
    s32VoNewLayer = GRAPHICS_LAYER_G3 + VO_LAYER_ESMART0 + uiBindChn;

    s32Ret = RK_MPI_VO_DisableLayer(s32VoLastLayer);
    if(RK_SUCCESS != s32Ret)
    {
        SAL_ERROR("RK_MPI_VO_Disable failed %x !\n",s32Ret);
    }

    s32Ret = RK_MPI_VO_UnBindLayer(s32VoLastLayer, s32VoLastDev);  
    if(RK_SUCCESS != s32Ret)
    {
        SAL_ERROR("RK_MPI_VO_UnBindLayer failed %x !\n",s32Ret);
    }


    s32Ret = RK_MPI_VO_BindLayer(s32VoNewLayer, s32VoNewDev, VO_LAYER_MODE_CURSOR);  
    if(RK_SUCCESS != s32Ret)
    {
        SAL_ERROR("RK_MPI_VO_Enable failed %x !\n",s32Ret);
    }

    s32Ret = RK_MPI_VO_GetLayerAttr(s32VoNewLayer, &stLayerAttr);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("RK_MPI_VO_GetLayerAttr, ret 0x%x\n", s32Ret);
    }

    stLayerAttr.enPixFormat                  = enPixFormat;
    stLayerAttr.stDispRect.s32X              = 0;
    stLayerAttr.stDispRect.s32Y              = 0;
    stLayerAttr.stDispRect.u32Width          = pfb3_chn->fb_dev.uiWidth;
    stLayerAttr.stDispRect.u32Height         = pfb3_chn->fb_dev.uiHeight;
    stLayerAttr.stImageSize.u32Width         = pfb3_chn->fb_dev.uiWidth;
    stLayerAttr.stImageSize.u32Height        = pfb3_chn->fb_dev.uiHeight;

    s32Ret = RK_MPI_VO_SetLayerAttr(s32VoNewLayer, &stLayerAttr);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("RK_MPI_VO_SetLayerAttr, ret 0x%x\n", s32Ret);
    }


    s32Ret = RK_MPI_VO_EnableLayer(s32VoNewLayer);
    if(RK_SUCCESS != s32Ret)
    {
        DISP_LOGE("RK_MPI_VO_Disable failed %x !\n",s32Ret);
    }

    pfb3_chn->mouseLastChn = pfb3_chn->mouseNewChn;

    
    DISP_LOGI("INIT fb %d success(%p,len %d)\n",pfb3_chn->mouseLastChn, pfb3_chn->fb_dev.pShowScreen, pfb3_chn->fb_dev.uiSmem_len);

    return SAL_SOK;
}




/**
 * @function   fb_rk3588_Refresh
 * @brief      刷新图形层界面
 * @param[in]  UINT32 uiChn               设备号
 * @param[in]  FB_COMMON *pfb_chn         fb设备信息
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  显示坐标
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_rk3588_Refresh(UINT32 uiChn,FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr)
{
    INT32 s32Ret = SAL_FAIL;

    if(FB_DEV_NUM_MAX <= uiChn)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }

    if(pfb_chn == NULL)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }

    if(pfb_chn->AppMMAP != SAL_TRUE || pfb_chn->DevMMAP != SAL_TRUE)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }
    
    if(uiChn == GRAPHICS_LAYER_G0 || uiChn == GRAPHICS_LAYER_G1)
    {
        s32Ret = fb_rk3588RefreshUiFb(uiChn, pfb_chn);
        if(s32Ret != SAL_SOK)
        {
            SAL_ERROR("error\n");
            return SAL_FAIL;
        }
    }
    else if (uiChn == GRAPHICS_LAYER_G3)
    {
        s32Ret = fb_rk3588RefreshMouseFb(uiChn + pfb_chn->mouseLastChn, pfb_chn, pstHalFbAttr);
        if(s32Ret != SAL_SOK)
        {
            SAL_ERROR("error\n");
            return SAL_FAIL;
        }
    }
          
    return SAL_SOK;
}

/**
 * @function   fb_rk3588_SetOrgin
 * @brief      设置显示启始坐标
 * @param[in]  UINT32 uiChn               
 * @param[in]  FB_COMMON *pfb_chn         
 * @param[in]  POS_ATTR *pstPos 
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_rk3588_SetOrgin(UINT32 uiChn,FB_COMMON *pfb_chn, POS_ATTR *pstPos)
{

    if(FB_DEV_NUM_MAX <= uiChn)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }

    if(pfb_chn == NULL)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }

    if(pfb_chn->AppMMAP != SAL_TRUE || pfb_chn->DevMMAP != SAL_TRUE)
    {
        SAL_ERROR("uiChn %d\n",uiChn);
        return SAL_FAIL;
    }

    if(uiChn == GRAPHICS_LAYER_G3)
    {   
        RK_MPI_VO_SetCursorPostion(uiChn + VO_LAYER_ESMART0, pstPos->x, pstPos->y);       
    }

     return SAL_SOK;
    
}

/**
 * @function   fb_rk3588_CreateFb
 * @brief      创建显示设备
 * @param[in]  UINT32 uiChn               设备号
 * @param[in]  FB_COMMON *pfb_chn         设备信息
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  显示参数，fb坐标区域信息
 * @param[in]  FB_INIT_ATTR_ST *pstFbPrm  图形界面信息，上面的接口把这个参数赋值为voDev的分辨率信息
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_rk3588_CreateFb(UINT32 uiChn, FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr, FB_INIT_ATTR_ST *pstFbPrm)
{

    INT32 i = 0;
    INT32 s32Ret = SAL_SOK;
    FB_INIT_ATTR_ST  stFbHalMmapAttr;
    memset(&stFbHalMmapAttr, 0, sizeof(FB_INIT_ATTR_ST));
    VO_LAYER s32VoGfxLayer;

    /* 获取显示设备设置的分辨率 */      
    if(pstFbPrm == NULL)
    {
        SAL_LOGE("error !!\n");
        return SAL_FAIL;
    }

    for(i = 0; i < pstHalFbAttr->uiCnt; i++)
    {        
        stFbHalMmapAttr.uiW      = pstHalFbAttr->stScreenAttr[i].width;
        stFbHalMmapAttr.uiH      = pstHalFbAttr->stScreenAttr[i].height;      
        pfb_chn->bitWidth        = pstHalFbAttr->stScreenAttr[i].bitWidth;
        
        SAL_LOGI("MMAP: New App Fb %d (%d x %d ) bitWidth %d !\n",uiChn,stFbHalMmapAttr.uiW,stFbHalMmapAttr.uiH,pfb_chn->bitWidth);
        
       if(stFbHalMmapAttr.uiW == 0x00 || stFbHalMmapAttr.uiH == 0x00 || pfb_chn->bitWidth == 0x00)
       {
          SAL_ERROR("error fb w(%d) h(%d) !!\n",stFbHalMmapAttr.uiW,stFbHalMmapAttr.uiH);
          return SAL_FAIL;
       }
        
        SAL_LOGI("MMAP: New Dev Fb %d (%d x %d ) Already exist fb %d (%d x %d )\n",
          uiChn,pstFbPrm->uiW,pstFbPrm->uiH,
          uiChn,pfb_chn->fb_dev.uiWidth,pfb_chn->fb_dev.uiHeight);

         if(pfb_chn->DevMMAP == SAL_TRUE)
        {                
            if(pfb_chn->fb_dev.uiWidth != pstFbPrm->uiW || pfb_chn->fb_dev.uiHeight != pstFbPrm->uiH)
            {
                SAL_LOGW("MMAP: Dev Fb %d Already exist,UMAP first !\n",uiChn);
                
                s32Ret = fb_rk3588DeleteFb(uiChn, pfb_chn);
                if(s32Ret != SAL_SOK)
                {
                    SAL_LOGE("fb_rk3588DeleteFb fb %d fail !\n",uiChn);                
                    return SAL_FAIL;
                }
                
                pfb_chn->DevMMAP = SAL_FALSE;
            }
            else
            {
                SAL_LOGW("MMAP: Dev Fb %d Already MMAP !\n",uiChn);
            }
        }
        
        pfb_chn->fb_dev.uiWidth  = pstFbPrm->uiW; //把voDev的分辨率信息作为fb的宽高信息
        pfb_chn->fb_dev.uiHeight = pstFbPrm->uiH;
        pfb_chn->u32BytePerPixel = pfb_chn->bitWidth / 8;
        
        if(pfb_chn->DevMMAP == SAL_FALSE)
        {   
            /* fb ui */
            s32VoGfxLayer = uiChn + VO_LAYER_ESMART0; /*uiChn上层是按照hisi里的fb号0/1/2传下来，这里映射成RK的图层号 */
            
            if(s32VoGfxLayer == VO_LAYER_ESMART0 || s32VoGfxLayer == VO_LAYER_ESMART1) /* fb ui */
            {
                /* vo init */
                s32Ret = fb_rk3588CreateDevFb(uiChn,  pfb_chn);
                if(SAL_SOK != s32Ret)
                {
                    SAL_ERROR("fb_rk3588CreateDevFb failed !!\n");
                    return SAL_FAIL;
                }
    
            }
            else if(s32VoGfxLayer == VO_LAYER_ESMART2) /* 鼠标 */
            {                
                /*是否合理默认写0*/
                pfb_chn->mouseNewChn = pstHalFbAttr->stScreenAttr[0].mouseBindChn;
                s32Ret = fb_rk3588CreateDevFb3(uiChn, pfb_chn,  &stFbHalMmapAttr);
                if(SAL_SOK != s32Ret)
                {
                    SAL_ERROR("fb_rk3588CreateDevFb3 failed !!\n");
                    return SAL_FAIL;
                }
                
            }
                                  
            pfb_chn->DevMMAP = SAL_TRUE;
        }
    
        if(pfb_chn->AppMMAP == SAL_TRUE)
        {   
            SAL_LOGI("MMAP: New App Fb %d (%d x %d ) srcAddr %lu srcSize %d , Already exist Fb %d (%d x %d ) Viraddr %lu uiSize 0x%x !\n",
                uiChn,stFbHalMmapAttr.uiW,stFbHalMmapAttr.uiH,(UINTL)pstHalFbAttr->stScreenAttr[i].srcAddr,pstHalFbAttr->stScreenAttr[i].srcSize,
                uiChn,pfb_chn->fb_app.uiWidth,pfb_chn->fb_app.uiHeight,(UINTL)pfb_chn->fb_app.Viraddr,pfb_chn->fb_app.uiSize);
    
            if(stFbHalMmapAttr.uiW != pfb_chn->fb_app.uiWidth || stFbHalMmapAttr.uiH != pfb_chn->fb_app.uiHeight)
            {
                SAL_LOGW("MMAP: App Fb %d , Already exist,UMAP first !\n",uiChn);
                
                s32Ret = fb_rk3588DeleteAppFb(uiChn, pfb_chn);
                if(s32Ret != SAL_SOK)
                {
                    SAL_ERROR("UMAP: Already exist fb %d fail !\n",uiChn);                
                    return SAL_FAIL;
                }
                
                pfb_chn->AppMMAP = SAL_FALSE;
            }
            else
            {                
                SAL_LOGW("MMAP: App Fb %d Already MMAP !\n",uiChn);
                
                pstHalFbAttr->stScreenAttr[i].srcAddr = pfb_chn->fb_app.Viraddr;
                pstHalFbAttr->stScreenAttr[i].srcSize = pfb_chn->fb_app.uiSize;
            }
        }
        
        if(pfb_chn->AppMMAP == SAL_FALSE)
        {             
                /* UI fb */
            s32Ret = fb_rk3588CreateAppFb(uiChn, pfb_chn, &pstHalFbAttr->stScreenAttr[i]);
            if(SAL_SOK != s32Ret)
            {                   
                SAL_LOGE("fb_rk3588CreateAppFb failed !!\n");
    
                s32Ret = fb_rk3588DeleteFb(uiChn, pfb_chn);
                if(s32Ret != SAL_SOK)
                {
                    SAL_LOGE("fb_rk3588DeleteFb fb %d fail !\n",uiChn);                
                    return SAL_FAIL;
                }
                
                pfb_chn->DevMMAP = SAL_FALSE;
    
                return SAL_FAIL;
            }                        
            
            pfb_chn->AppMMAP = SAL_TRUE;            
        }
    }
    return SAL_SOK;
}


/**
 * @function   fb_rk3588ReleasFb
 * @brief      释放显示设备
 * @param[in]  UINT32 uiChn        设备号
 * @param[in]  FB_COMMON *pfb_chn  设备信息
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_rk3588_ReleasFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{
    INT32 s32Ret = 0;

    if(pfb_chn->DevMMAP == SAL_TRUE)
    {
        s32Ret = fb_rk3588DeleteFb(uiChn, pfb_chn);
        if(SAL_SOK != s32Ret)
        {
            SAL_ERROR("error \n");
            return SAL_FAIL;
        }

        pfb_chn->DevMMAP = SAL_FALSE;
        
        SAL_LOGI("Dev Fb %d Umap Success !\n",uiChn);
    }
    else
    {
        SAL_LOGW("Dev Fb %d Already Umap !\n",uiChn);
    }

    if(pfb_chn->AppMMAP == SAL_TRUE)
    {
        s32Ret = fb_rk3588DeleteAppFb(uiChn, pfb_chn);
        if(SAL_SOK != s32Ret)
        {
            SAL_ERROR("error \n");
            return SAL_FAIL;
        }
        
        pfb_chn->AppMMAP = SAL_FALSE;

        SAL_LOGI("App Fb %d Umap Success !\n",uiChn);
    }
    else
    {
    
        SAL_LOGW("App Fb %d Already umap !\n",uiChn);
    }    
        
    return SAL_SOK;
}

/**
 * @function   fb_rk3588_GetFbNum
 * @brief      获取设备最大显示设备数
 * @param[in]  void  
 * @param[out] None
 * @return     static INT32 FB_DEV_NUM_MAX 设备数量
 *                      
 */
static INT32 fb_rk3588_GetFbNum(void)
{
    return FB_DEV_NUM_MAX;
}

/**
 * @function   fb_halRegister
 * @brief      初始化fb成员函数
 * @param[in]  void  
 * @param[out] None
 * @return     FB_PLAT_OPS * 返回函数结构体指针
 */
FB_PLAT_OPS *fb_halRegister(void)
{
    memset(&g_stFbPlatOps, 0,sizeof(FB_PLAT_OPS));

    g_stFbPlatOps.createFb   = fb_rk3588_CreateFb;
    g_stFbPlatOps.releaseFb  = fb_rk3588_ReleasFb;
    g_stFbPlatOps.refreshFb  = fb_rk3588_Refresh;
    g_stFbPlatOps.setOrgin   = fb_rk3588_SetOrgin;
    g_stFbPlatOps.setFbChn   = fb_rk3588_SetMouseFbChn;
    g_stFbPlatOps.getFbNum   = fb_rk3588_GetFbNum;

    return &g_stFbPlatOps;
    
}


