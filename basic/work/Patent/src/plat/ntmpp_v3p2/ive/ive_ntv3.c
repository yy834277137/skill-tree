/**
 * @file   ive_ntv3.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief
 * @author
 * @date    2021.6.17
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
#include "platform_sdk.h"
#include "ive_hal_inter.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          全局变量定义区                                    */
/* ========================================================================== */

static IVE_PLAT_OPS_ST g_stIvePlatOps;


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */


/**
 * @function    ive_ntv3_CSC
 * @brief
 * @param[in]
 *
 * @param[out]
 *
 * @return
 */
static INT32 ive_ntv3_CSC(IVE_HAL_IMAGE *pstSrc, IVE_HAL_IMAGE *pstDst, IVE_HAL_MODE_CTRL *pstIveCscCtrl)
{
    HD_GFX_COPY copy_param;
    HD_RESULT ret = HD_OK;

    if (pstSrc == NULL || pstDst == NULL || pstIveCscCtrl == NULL)
    {
        SAL_ERROR("ive error prm \n");
        return SAL_FALSE;
    }


    copy_param.src_region.x = 0;
    copy_param.src_region.y = 0;
    copy_param.src_region.w = pstSrc->u32Width;
    copy_param.src_region.h = pstSrc->u32Height;

    copy_param.src_img.dim.w = pstSrc->u32Width;
    copy_param.src_img.dim.h = pstSrc->u32Height;
    copy_param.src_img.p_phy_addr[0] = pstSrc->u64PhyAddr[0];
    copy_param.src_img.p_phy_addr[1] = pstSrc->u64PhyAddr[1];
    copy_param.src_img.p_phy_addr[2] = pstSrc->u64PhyAddr[2];
    copy_param.src_img.ddr_id = DDR1_ADDR_START > pstSrc->u64PhyAddr[0]?DDR_ID0:DDR_ID1;

    
    copy_param.dst_pos.x = 0;
    copy_param.dst_pos.y = 0;
    copy_param.dst_img.dim.w = pstDst->u32Width;
    copy_param.dst_img.dim.h = pstDst->u32Height;
    copy_param.dst_img.p_phy_addr[0] = pstDst->u64PhyAddr[0];
    copy_param.dst_img.p_phy_addr[1] = pstDst->u64PhyAddr[1];
    copy_param.dst_img.p_phy_addr[2] = pstDst->u64PhyAddr[2];
    copy_param.dst_img.ddr_id = DDR1_ADDR_START > pstDst->u64PhyAddr[0]?DDR_ID0:DDR_ID1;

    switch(pstSrc->enColorType)
    {
       case SAL_VIDEO_DATFMT_YUV420SP_UV:
           copy_param.src_img.format = HD_VIDEO_PXLFMT_YUV420;
           break;
        case SAL_VIDEO_DATFMT_YUV420P:
           copy_param.src_img.format = HD_VIDEO_PXLFMT_YUV420_PLANAR;
           break;
       default:
           copy_param.src_img.format = HD_VIDEO_PXLFMT_YUV420;
           break;
    }

    switch(pstDst->enColorType)
    {
        case SAL_VIDEO_DATFMT_RGB24_888:
            copy_param.dst_img.format = HD_VIDEO_PXLFMT_BGR888;
            break;
        case SAL_VIDEO_DATFMT_BGR:
           /*  BGR分开存储          存储在三个平面内 */
            copy_param.dst_img.format = HD_VIDEO_PXLFMT_RGB888_PLANAR;
            break;
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        case SAL_VIDEO_DATFMT_ARGB16_1555:
        case SAL_VIDEO_DATFMT_RGBA_8888:
        {
            break;
        }
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

    return ret;
}



/**
 * @function    ive_ssv5_QuickCopy
 * @brief       ive快速拷贝
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 ive_ssv5_QuickCopy(SYSTEM_FRAME_INFO *pSrcSysFrame, SYSTEM_FRAME_INFO *pDstSysFrame, IVE_HAL_RECT *pstSrcRect, IVE_HAL_RECT *pstDstRect, UINT32 bCached)
{
    SAL_ERROR("ive_ssv5_QuickCopy fail!\n");
    return SAL_FAIL;
}


/**
 * @function	ive_hal_register
 * @brief		对平台相关的内存申请函数进行注册
 * @param[in]
 * @param[out]
 * @return
 */
IVE_PLAT_OPS_ST *ive_hal_register(void)
{
    g_stIvePlatOps.pIveCSC = ive_ntv3_CSC;
    g_stIvePlatOps.pIveQuickCopy = ive_ssv5_QuickCopy;

    return &g_stIvePlatOps;
}

