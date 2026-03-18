/**
 * @file   tde_ntv3.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台 tde 模块
 * @author liuxianying
 * @date   2021/12/02
 * @note
 * @note \n History
   1.日    期: 2021/12/02
     作    者: liuxianying
     修改历史: 创建文件
 */


#include <sal.h>
#include "tde_hal_inter.h"


static TDE_PLAT_OPS_ST g_stTdePlatOps = {0};


/*******************************************************************************
* 函数名  : HAL_TdeQuickCopyYuv
* 描  述  : 使用TDE快速拷贝yuv
* 输  入  :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 tde_ntv3_QuickCopyYuv(TDE_HAL_SURFACE *pstSrc,
                            TDE_HAL_RECT  *pstSrcRect,
                            TDE_HAL_SURFACE *pstDst,
                            TDE_HAL_RECT *pstDstRect,
                            UINT32 u32SkipLen,
                            UINT32 bCached)
{
    HD_GFX_COPY copy_param;
    HD_RESULT ret = HD_OK;


    copy_param.src_region.x = 0;
    copy_param.src_region.y = 0;
    copy_param.src_region.w = pstSrc->u32Width;
    copy_param.src_region.h = pstSrc->u32Height;

    copy_param.src_img.dim.w = pstSrc->u32Stride;
    copy_param.src_img.dim.h = pstSrc->u32Height;
    copy_param.src_img.p_phy_addr[0] = (UINTPTR)pstSrc->PhyAddr;
    copy_param.src_img.ddr_id =  DDR1_ADDR_START > (UINTPTR)pstSrc->PhyAddr ? DDR_ID0:DDR_ID1;
    copy_param.src_img.format = HD_VIDEO_PXLFMT_Y8;

    
    copy_param.dst_pos.x = 0;
    copy_param.dst_pos.y = 0;
    copy_param.dst_img.dim.w = pstDst->u32Stride;
    copy_param.dst_img.dim.h = pstDst->u32Height;
    copy_param.dst_img.p_phy_addr[0] = (UINTPTR)pstDst->PhyAddr;
    copy_param.dst_img.ddr_id =  DDR1_ADDR_START > (UINTPTR)pstDst->PhyAddr ? DDR_ID0:DDR_ID1;
    copy_param.dst_img.format = HD_VIDEO_PXLFMT_Y8;

        
    ret = hd_gfx_copy(&copy_param);
    if (ret != HD_OK) 
    {
        SAL_ERROR("hd_gfx_copy fail\n");
        return ret;
    }

    copy_param.src_region.x = 0;
    copy_param.src_region.y = 0;
    copy_param.src_region.w = pstSrc->u32Width;
    copy_param.src_region.h = pstSrc->u32Height / 2;

    copy_param.src_img.dim.w = pstSrc->u32Stride;
    copy_param.src_img.dim.h = pstSrc->u32Height / 2;
    copy_param.src_img.p_phy_addr[0] = pstSrc->PhyAddr + pstSrc->u32Stride * pstSrc->u32Height + u32SkipLen;
    copy_param.src_img.ddr_id =  DDR1_ADDR_START > (UINTPTR)pstSrc->PhyAddr ? DDR_ID0:DDR_ID1;
    copy_param.src_img.format = HD_VIDEO_PXLFMT_Y8;

    
    copy_param.dst_pos.x = 0;
    copy_param.dst_pos.y = 0;
    copy_param.dst_img.dim.w = pstDst->u32Stride;
    copy_param.dst_img.dim.h = pstDst->u32Height / 2;
    copy_param.dst_img.p_phy_addr[0] = pstDst->PhyAddr + pstDst->u32Stride * pstDst->u32Height;
    copy_param.dst_img.ddr_id =  DDR1_ADDR_START > (UINTPTR)pstDst->PhyAddr ? DDR_ID0:DDR_ID1;
    copy_param.dst_img.format = HD_VIDEO_PXLFMT_Y8;

        
    ret = hd_gfx_copy(&copy_param);
    if (ret != HD_OK) 
    {
        SAL_ERROR("hd_gfx_copy fail\n");
        return ret;
    }


    return SAL_SOK;
}


/**
 * @function   tde_ntv3_DmaCopyYuv
 * @brief      dma快速拷贝
 * @param[in]  PhysAddr srcAddrPhy         
 * @param[in]  PhysAddr dstAddrPhy         
 * @param[in]  UINT32 u32Width                
 * @param[in]  UINT32 u32Height               
 * @param[in]  SAL_VideoDataFormat format  
 * @param[out] None
 * @return     INT32
 */
#if 0
static INT32 tde_ntv3_DmaCopy(UINT32 u32SrcWidth, UINT32 u32SrcHeight,  PhysAddr srcAddrPhy, 
                        UINT32 u32DstWidth, UINT32 u32DstHeight, PhysAddr dstAddrPhy, SAL_VideoDataFormat format)
{
   
    HD_RESULT ret = HD_OK;
    UINTPTR srcPhyAddr = 0;
    UINTPTR dstPhyAddr = 0;
    UINT32 u32Size = 0;
    HD_COMMON_MEM_DDR_ID srcId =  DDR1_ADDR_START > srcAddrPhy ? DDR_ID0:DDR_ID1;
    HD_COMMON_MEM_DDR_ID dstId =  DDR1_ADDR_START > dstAddrPhy ? DDR_ID0:DDR_ID1;

    srcPhyAddr = srcAddrPhy;
    dstPhyAddr = dstAddrPhy;
    u32Size = u32SrcWidth * u32SrcHeight;
    ret = hd_common_dmacopy(srcId, srcPhyAddr, dstId, dstPhyAddr,u32Size);
    if(HD_OK != ret)
    {
        SAL_ERROR("hd_common_dmacopy fail\n");
        return ret;
    }

    srcPhyAddr = srcAddrPhy + u32SrcWidth * u32SrcHeight;
    dstPhyAddr = dstAddrPhy + u32DstWidth * u32DstHeight;
    u32Size = (u32SrcWidth * u32SrcHeight) >> 1;
    
    ret = hd_common_dmacopy(srcId, srcPhyAddr, dstId, dstPhyAddr,u32Size);
    if(HD_OK != ret)
    {
        SAL_ERROR("hd_common_dmacopy fail\n");
        return ret;
    }

    return SAL_SOK;
}
#endif
static INT32 tde_ntv3_DmaCopy(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect)
{
    static UINT32 count = 0;
     HD_RESULT ret = HD_OK;
     UINTPTR srcPhyAddr = 0;
     UINTPTR dstPhyAddr = 0;
     UINT32 u32Size = 0;
     HD_COMMON_MEM_DDR_ID srcId =  DDR1_ADDR_START > pstSrc->PhyAddr ? DDR_ID0:DDR_ID1;
     HD_COMMON_MEM_DDR_ID dstId =  DDR1_ADDR_START > pstDst->PhyAddr ? DDR_ID0:DDR_ID1;

     if ((pstSrcRect->u32Width != pstSrc->u32Width) || (pstSrc->u32Width != pstDst->u32Width))
     {
        SAL_ERROR("NT not suppot two dimession DMA copy, srcRect width %d != Dst width %d \n", pstSrcRect->u32Width, pstDst->u32Width);
        return SAL_FAIL;
     }
     srcPhyAddr = pstSrc->PhyAddr + pstSrc->u32Width * pstSrcRect->s32Ypos + pstSrcRect->s32Xpos;
     dstPhyAddr = pstDst->PhyAddr + pstDst->u32Width * pstDstRect->s32Ypos + pstDstRect->s32Xpos;
     u32Size = pstSrc->u32Width * pstSrc->u32Height;
     ret = hd_common_dmacopy(srcId, srcPhyAddr, dstId, dstPhyAddr, u32Size);
     if(HD_OK != ret)
     {
         SAL_ERROR("hd_common_dmacopy fail\n");
         return ret;
     }

     count++;
     if ((count % 300) > 0 &&  (count % 300) < 5)
     {
        SAL_DEBUG("src: Addr %llx, w %u, h %u, Rect x %d, y %d, w %u, h %u \n", pstSrc->PhyAddr, pstSrc->u32Width, pstSrc->u32Height,  pstSrcRect->s32Xpos, pstSrcRect->s32Ypos, pstSrcRect->u32Width, pstSrcRect->u32Height);
        SAL_DEBUG(" dst: Addr %llx, w %u, h %u, Rect x %d, y %d, w %u, h %u \n",pstDst->PhyAddr, pstDst->u32Width, pstDst->u32Height,  pstDstRect->s32Xpos, pstDstRect->s32Ypos, pstDstRect->u32Width, pstDstRect->u32Height);
     }
     srcPhyAddr = pstSrc->PhyAddr + pstSrc->u32Width * pstSrc->u32Height + pstSrc->u32Width * pstSrcRect->s32Ypos / 2 + pstSrcRect->s32Xpos;
     dstPhyAddr = pstDst->PhyAddr + pstDst->u32Width * pstDst->u32Height + pstDst->u32Width * pstDstRect->s32Ypos / 2 + pstDstRect->s32Xpos;
     u32Size = (pstSrc->u32Width * pstSrc->u32Height) >> 1;
     
     ret = hd_common_dmacopy(srcId, srcPhyAddr, dstId, dstPhyAddr,u32Size);
     if(HD_OK != ret)
     {
         SAL_ERROR("hd_common_dmacopy fail\n");
         return ret;
     }
    
     return SAL_SOK;
}


/*******************************************************************************
* 函数名  : himpi_TdeQuickCopy
* 描  述  : 使用TDE快速拷贝图像
* 输  入  : - pstSrc   : 输入宏块信息
*         : - pstSrcRect : 输入宏块操作矩形
*         : - pstDst  : 输出宏块信息
*         : - pstDstRect: 输出宏块操作矩形
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 tde_ntv3_QuickCopyTrans(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect, UINT32 bCached)
{
    HD_GFX_COPY copy_param = {0};
    HD_RESULT ret = HD_OK;
    const UINT32 u32GfxCopyAlign = 4; // 拷贝至少需要对齐的字节数

    /* 异常校验与处理 */
    /* 源位置+源区域本身横向不越界 */
    if (pstSrcRect->s32Xpos + pstSrcRect->u32Width > pstSrc->u32Width)
    {
        // 纠正X坐标
        if (pstSrcRect->s32Xpos >= pstSrc->u32Width)
        {
            pstSrcRect->s32Xpos = SAL_SUB_SAFE(pstSrc->u32Width, u32GfxCopyAlign);
        }
        // 纠正X坐标后，重新校验是否恢复正常，若再次越界，则纠正源区域宽度
        if (pstSrcRect->s32Xpos + pstSrcRect->u32Width > pstSrc->u32Width)
        {
            pstSrcRect->u32Width = SAL_SUB_SAFE(pstSrc->u32Width, pstSrcRect->s32Xpos);
        }
    }

    /* 源位置+源区域本身纵向不越界 */
    if (pstSrcRect->s32Ypos + pstSrcRect->u32Height > pstSrc->u32Height)
    {
        // 纠正Y坐标
        if (pstSrcRect->s32Ypos >= pstSrc->u32Height)
        {
            pstSrcRect->s32Ypos = SAL_SUB_SAFE(pstSrc->u32Height, u32GfxCopyAlign);
        }
        // 纠正Y坐标后，重新校验是否恢复正常，若再次越界，则纠正源区域宽度
        if (pstSrcRect->s32Ypos + pstSrcRect->u32Height > pstSrc->u32Height)
        {
            pstSrcRect->u32Height = SAL_SUB_SAFE(pstSrc->u32Height, pstSrcRect->s32Ypos);
        }
    }

    /* 目的位置+源区域横向不越界 */
    if (pstDstRect->s32Xpos + pstSrcRect->u32Width > pstDst->u32Width)
    {
        // 纠正X坐标
        if (pstDstRect->s32Xpos >= pstDst->u32Width)
        {
            pstDstRect->s32Xpos = SAL_SUB_SAFE(pstDst->u32Width, u32GfxCopyAlign);
        }
        // 纠正X坐标后，重新校验是否恢复正常，若再次越界，则纠正源区域宽度
        if (pstDstRect->s32Xpos + pstSrcRect->u32Width > pstDst->u32Width)
        {
            pstSrcRect->u32Width = SAL_SUB_SAFE(pstDst->u32Width, pstDstRect->s32Xpos);
        }
    }

    /* 目的位置+源区域本身纵向不越界 */
    if (pstDstRect->s32Ypos + pstSrcRect->u32Height > pstDst->u32Height)
    {
        // 纠正Y坐标
        if (pstDstRect->s32Ypos >= pstDst->u32Height)
        {
            pstDstRect->s32Ypos = SAL_SUB_SAFE(pstDst->u32Height, u32GfxCopyAlign);
        }
        // 纠正Y坐标后，重新校验是否恢复正常，若再次越界，则纠正源区域宽度
        if (pstDstRect->s32Ypos + pstSrcRect->u32Height > pstDst->u32Height)
        {
            pstSrcRect->u32Height = SAL_SUB_SAFE(pstDst->u32Height, pstDstRect->s32Ypos);
        }
    }

    /* 源区域 */
    copy_param.src_region.x = pstSrcRect->s32Xpos;
    copy_param.src_region.y = pstSrcRect->s32Ypos;
    copy_param.src_region.w = pstSrcRect->u32Width;
    copy_param.src_region.h = pstSrcRect->u32Height;

    copy_param.src_img.dim.w = pstSrc->u32Width;
    copy_param.src_img.dim.h = pstSrc->u32Height;
    copy_param.src_img.p_phy_addr[0] = pstSrc->PhyAddr;
    copy_param.src_img.ddr_id = DDR1_ADDR_START > pstSrc->PhyAddr ? DDR_ID0:DDR_ID1;

    /* 目的区域 */
    copy_param.dst_pos.x = pstDstRect->s32Xpos;
    copy_param.dst_pos.y = pstDstRect->s32Ypos;

    copy_param.dst_img.dim.w = pstDst->u32Width;
    copy_param.dst_img.dim.h = pstDst->u32Height;
    copy_param.dst_img.p_phy_addr[0] = pstDst->PhyAddr;
    copy_param.dst_img.ddr_id = DDR1_ADDR_START > pstDst->PhyAddr ? DDR_ID0:DDR_ID1;

    switch(pstSrc->enColorFmt)
    {
        case SAL_VIDEO_DATFMT_ARGB_8888:
            copy_param.src_img.format = HD_VIDEO_PXLFMT_ARGB8888;
            break;
        case SAL_VIDEO_DATFMT_YUV420P:
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
            copy_param.src_img.format = HD_VIDEO_PXLFMT_YUV420;
            break;
        case SAL_VIDEO_DATFMT_BAYER_RAW_DUAL:
        case SAL_VIDEO_DATFMT_BAYER_RAW_SINGLE:
        case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
            copy_param.src_img.dim.w = (pstSrc->u32Stride == 0) ? pstSrc->u32Width : pstSrc->u32Stride;
            copy_param.src_img.format = HD_VIDEO_PXLFMT_Y8;
            break;
        default:
            copy_param.src_img.format = HD_VIDEO_PXLFMT_ARGB8888;
            break;
    }

    switch(pstDst->enColorFmt)
    {
        case SAL_VIDEO_DATFMT_ARGB_8888:
            copy_param.dst_img.format = HD_VIDEO_PXLFMT_ARGB8888;
            break;
        case SAL_VIDEO_DATFMT_YUV420P:
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
            copy_param.dst_img.format = HD_VIDEO_PXLFMT_YUV420;
            break;
        case SAL_VIDEO_DATFMT_BAYER_RAW_DUAL:
        case SAL_VIDEO_DATFMT_BAYER_RAW_SINGLE:
        case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
            copy_param.dst_img.dim.w = (pstDst->u32Stride == 0) ? pstDst->u32Width : pstDst->u32Stride;
            copy_param.dst_img.format = HD_VIDEO_PXLFMT_Y8;
            break;
        default:
            copy_param.dst_img.format = HD_VIDEO_PXLFMT_ARGB8888;
            break;
    }

    ret = hd_gfx_copy(&copy_param);
    if (ret != HD_OK) 
    {
        SAL_ERROR("hd_gfx_copy fail\n");
        return ret;
    }


    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : himpi_TdeQuickResizeInte
* 描  述  : 光栅位图缩放入参整合 因海思结构体不能出现上层而封装
* 输  入  : - pstSrc    : 源位图
*         : - pstSrcRect: 源位图操作区域
*         : - pstDst    : 目标位图
*         : - pstDstRect: 目标位图操作区域
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 tde_ntv3_QuickResizeTrans(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect)
{
    return tde_ntv3_QuickCopyTrans(pstSrc, pstSrcRect, pstDst, pstDstRect, 0);
}


/**
 * @function    himpi_TdeInit
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 tde_ntv3_Init()
{
    hd_gfx_init();
    return SAL_SOK;
}

/**
 * @function    tde_hal_register
 * @brief       对平台相关的内存申请函数进行注册
 * @param[in]
 * @param[out]
 * @return
 */
TDE_PLAT_OPS_ST *tde_hal_register(void)
{
    g_stTdePlatOps.pTdeInit = tde_ntv3_Init;
    g_stTdePlatOps.pTdeQuickResize = tde_ntv3_QuickResizeTrans;
    g_stTdePlatOps.pTdeQuickCopy = tde_ntv3_QuickCopyTrans;
    g_stTdePlatOps.pTdeQuickCopyYuv = tde_ntv3_QuickCopyYuv;
    g_stTdePlatOps.pTdeDmaCopy = tde_ntv3_DmaCopy;

    return &g_stTdePlatOps;
}

