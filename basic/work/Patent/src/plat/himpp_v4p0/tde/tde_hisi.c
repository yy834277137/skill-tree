/*******************************************************************************
* tde_hisi.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : heshengyuan <heshengyuan@hikvision.com>
* Version: V1.0.0  2014年11月27日 Create
*
* Description : 图形加速函数封装
* Modification:
*******************************************************************************/

#include <sal.h>
#include "platform_sdk.h"
#include "tde_hal_inter.h"

static TDE_PLAT_OPS_ST g_stTdePlatOps;


/*******************************************************************************
* 函数名  : himpi_TdeQuickResize
* 描  述  : 光栅位图缩放
* 输  入  : - pstSrc    : 源位图
*         : - pstSrcRect: 源位图操作区域
*         : - pstDst    : 目标位图
*         : - pstDstRect: 目标位图操作区域
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
HI_S32 tde_hisi_QuickResize(TDE2_SURFACE_S *pstSrc,
                            TDE2_RECT_S *pstSrcRect,
                            TDE2_SURFACE_S *pstDst,
                            TDE2_RECT_S *pstDstRect)
{
    TDE_HANDLE s32Handle;
    HI_S32 s32Ret;

    s32Handle = HI_TDE2_BeginJob();
    if (HI_ERR_TDE_INVALID_HANDLE == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_QuickResize(s32Handle, pstSrc, pstSrcRect, pstDst, pstDstRect);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickResize failed %d %#x!\n", s32Handle, s32Ret);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 100);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Handle);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
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
HI_S32 tde_hisi_QuickResizeTrans(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect)
{
    HI_S32 s32Ret = SAL_SOK;

    TDE2_RECT_S stSrcRect = {0};
    TDE2_RECT_S stDstRect = {0};
    TDE2_SURFACE_S stSrc = {0};
    TDE2_SURFACE_S stDst = {0};

    if (pstSrc == NULL || pstSrcRect == NULL || pstDst == NULL || pstDstRect == NULL)
    {
        SAL_ERROR("tde error prm \n");
        return SAL_FALSE;
    }

    if (pstSrc->enColorFmt != pstDst->enColorFmt)
    {
        SAL_ERROR("tde resize trans color error \n");
        return SAL_FALSE;
    }

    s32Ret = HI_TDE2_Open();
    if (s32Ret < 0)
    {
        SAL_ERROR("HI_TDE2_Open failed :%d!\n", s32Ret);
        return s32Ret;
    }

    /* 0. open tde */
    stSrcRect.s32Xpos = pstSrcRect->s32Xpos;
    stSrcRect.s32Ypos = pstSrcRect->s32Ypos;
    stSrcRect.u32Height = pstSrcRect->u32Height;
    stSrcRect.u32Width = pstSrcRect->u32Width;

    stSrc.u32Width = pstSrc->u32Width;
    stSrc.u32Height = pstSrc->u32Height;
    stSrc.u32Stride = pstSrc->u32Stride;
    stSrc.PhyAddr = pstSrc->PhyAddr;
    stSrc.bAlphaMax255 = HI_TRUE;
    stSrc.u8Alpha0 = 0XFF;
    stSrc.u8Alpha1 = 0XFF;

    stDstRect.s32Xpos = pstDstRect->s32Xpos;
    stDstRect.s32Ypos = pstDstRect->s32Ypos;
    stDstRect.u32Height = pstDstRect->u32Height;
    stDstRect.u32Width = pstDstRect->u32Width;

    stDst.u32Width = pstDst->u32Width;
    stDst.u32Height = pstDst->u32Height;
    stDst.u32Stride = pstDst->u32Stride;
    stDst.PhyAddr = pstDst->PhyAddr;
    stDst.bAlphaMax255 = HI_TRUE;

    switch (pstSrc->enColorFmt)
    {
        case SAL_VIDEO_DATFMT_ARGB_8888:
        {
            stSrc.enColorFmt = TDE2_COLOR_FMT_ARGB8888;
            stDst.enColorFmt = TDE2_COLOR_FMT_ARGB8888;
            stSrc.bAlphaExt1555 = HI_FALSE;
            stDst.bAlphaExt1555 = HI_FALSE;
            break;
        }
        case SAL_VIDEO_DATFMT_ARGB16_1555:
        {
            stSrc.enColorFmt = TDE2_COLOR_FMT_ARGB1555;
            stDst.enColorFmt = TDE2_COLOR_FMT_ARGB1555;
            stSrc.bAlphaExt1555 = HI_TRUE;
            stDst.bAlphaExt1555 = HI_TRUE;
            break;
        }
        default:
        {
            stSrc.enColorFmt = TDE2_COLOR_FMT_ARGB8888;
            stDst.enColorFmt = TDE2_COLOR_FMT_ARGB8888;
            stSrc.bAlphaExt1555 = HI_FALSE;
            stDst.bAlphaExt1555 = HI_FALSE;
            break;
        }
    }

    s32Ret = tde_hisi_QuickResize(&stSrc, &stSrcRect, &stDst, &stDstRect);
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("error\n");
        return s32Ret;
    }

    return s32Ret;
}

/*******************************************************************************
* 函数名  : himpi_TdeQuickFill
* 描  述  : 向任务中添加快速填充操作。
* 输  入  : - pstDst     : 目标位图
*         : - pstDstRect : 目标位图操作区域
*         : - u32FillData: 填充值
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
HI_S32 tde_hisi_QuickFill(TDE2_SURFACE_S *pstDst, TDE2_RECT_S *pstDstRect, HI_U32 u32FillData)
{
    TDE_HANDLE s32Handle;
    HI_S32 s32Ret;

    s32Handle = HI_TDE2_BeginJob();
    if (HI_ERR_TDE_INVALID_HANDLE == s32Handle || HI_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_QuickFill(s32Handle, pstDst, pstDstRect, u32FillData);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickFill failed %d %#x!\n", s32Handle, s32Ret);

        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    /* 提交2D加速任务 */
    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 100);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Handle);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : HAL_TdeQuickCopyYuv
* 描  述  : 使用TDE快速拷贝yuv
* 输  入  :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 tde_hisi_QuickCopyYuv(TDE_HAL_SURFACE *pstSrc,
                            TDE_HAL_RECT  *pstSrcRect,
                            TDE_HAL_SURFACE *pstDst,
                            TDE_HAL_RECT *pstDstRect, 
                            UINT32 u32SkipLen,
							UINT32 bCached)
{
    INT32 s32Ret = 0;
    TDE_HANDLE s32Handle;
    TDE2_SURFACE_S srcSurface, dstSurface;
    TDE2_RECT_S srcRect, dstRect;

    if (bCached)
    {
        s32Ret = HI_MPI_SYS_MmzFlushCache(0, (void *)1, 0);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", s32Ret);
            return s32Ret;
        }
    }

    s32Handle = HI_TDE2_BeginJob();
    if (HI_ERR_TDE_INVALID_HANDLE == s32Handle || HI_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    memset(&srcSurface, 0, sizeof(TDE2_SURFACE_S));
    memset(&dstSurface, 0, sizeof(TDE2_SURFACE_S));
    memset(&srcRect, 0, sizeof(TDE2_RECT_S));
    memset(&dstRect, 0, sizeof(TDE2_RECT_S));

    srcSurface.bAlphaMax255 = HI_TRUE;
    srcSurface.enColorFmt = TDE2_COLOR_FMT_byte;
    dstSurface.bAlphaMax255 = HI_TRUE;
    dstSurface.enColorFmt = TDE2_COLOR_FMT_byte;

    /* Y分量 */
    srcSurface.u32Width = pstSrc->u32Width;
    srcSurface.u32Height = pstSrc->u32Height;
    srcSurface.u32Stride = pstSrc->u32Stride;
    srcSurface.PhyAddr = (HI_U64)pstSrc->PhyAddr;
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = srcSurface.u32Width;
    srcRect.u32Height = srcSurface.u32Height;

    dstSurface.u32Width = pstDst->u32Width;
    dstSurface.u32Height = pstDst->u32Height;
    dstSurface.u32Stride = pstDst->u32Stride;
    dstSurface.PhyAddr = (HI_U64)pstDst->PhyAddr;
    dstRect.s32Xpos = 0;
    dstRect.s32Ypos = 0;
    dstRect.u32Width = dstSurface.u32Width;
    dstRect.u32Height = dstSurface.u32Height;

    s32Ret = HI_TDE2_QuickCopy(s32Handle, &srcSurface, &srcRect, &dstSurface, &dstRect);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickCopy failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    /* UV分量 */
    srcSurface.u32Width = pstSrc->u32Width;
    srcSurface.u32Height = pstSrc->u32Height / 2;
    srcSurface.u32Stride = pstSrc->u32Stride;
    srcSurface.PhyAddr = (HI_U64)pstSrc->PhyAddr + pstSrc->u32Stride * pstSrc->u32Height + u32SkipLen;
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = srcSurface.u32Width;
    srcRect.u32Height = srcSurface.u32Height;

    dstSurface.u32Width = pstDst->u32Width;
    dstSurface.u32Height = pstDst->u32Height / 2;
    dstSurface.u32Stride = pstDst->u32Stride;
    dstSurface.PhyAddr = (HI_U64)pstDst->PhyAddr + pstDst->u32Stride * pstDst->u32Height;
    dstRect.s32Xpos = 0;
    dstRect.s32Ypos = 0;
    dstRect.u32Width = dstSurface.u32Width;
    dstRect.u32Height = dstSurface.u32Height;

    s32Ret = HI_TDE2_QuickCopy(s32Handle, &srcSurface, &srcRect, &dstSurface, &dstRect);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickCopy failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 100);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Ret);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }
    
    if (bCached)
    {
        s32Ret = HI_MPI_SYS_MmzFlushCache(0, (void *)1, 0);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", s32Ret);
            return s32Ret;
        }
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
HI_S32 tde_hisi_QuickCopy(TDE2_SURFACE_S *pstSrc,
                          TDE2_RECT_S *pstSrcRect,
                          TDE2_SURFACE_S *pstDst,
                          TDE2_RECT_S *pstDstRect,
                          UINT32 bCached)
{
    TDE_HANDLE s32Handle;
    HI_S32 s32Ret;

    if (bCached)
    {
        s32Ret = HI_MPI_SYS_MmzFlushCache(0, (void *)1, 0);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", s32Ret);
            return s32Ret;
        }
    }

    s32Handle = HI_TDE2_BeginJob();
    if (HI_ERR_TDE_INVALID_HANDLE == s32Handle || HI_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_QuickCopy(s32Handle, pstSrc, pstSrcRect, pstDst, pstDstRect);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickCopy failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    /* submit the job 阻塞式的提交已创建的TDE 任务。 */
    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 100);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Ret);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    if (bCached)
    {
        s32Ret = HI_MPI_SYS_MmzFlushCache(0, (void *)1, 0);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", s32Ret);
            return s32Ret;
        }
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
HI_S32 tde_hisi_QuickCopyTrans(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect, UINT32 bCached)
{
    INT32 s32Ret = 0;
    TDE2_SURFACE_S srcSurface, dstSurface;
    TDE2_RECT_S srcRect, dstRect;

    if (pstSrc == NULL || pstSrcRect == NULL || pstDst == NULL || pstDstRect == NULL)
    {
        SAL_ERROR("tde error prm \n");
        return SAL_FALSE;
    }

    if (pstSrc->enColorFmt != pstDst->enColorFmt)
    {
        SAL_ERROR("tde copy trans color error \n");
        return SAL_FALSE;
    }

    memset(&srcSurface, 0, sizeof(TDE2_SURFACE_S));
    memset(&dstSurface, 0, sizeof(TDE2_SURFACE_S));
    memset(&srcRect, 0, sizeof(TDE2_RECT_S));
    memset(&dstRect, 0, sizeof(TDE2_RECT_S));

    srcSurface.u32Width = pstSrc->u32Width;
    srcSurface.u32Height = pstSrc->u32Height;
    srcSurface.u32Stride = pstSrc->u32Stride;
    srcSurface.PhyAddr = pstSrc->PhyAddr;
    srcRect.s32Xpos = pstSrcRect->s32Xpos;
    srcRect.s32Ypos = pstSrcRect->s32Ypos;
    srcRect.u32Width = pstSrcRect->u32Width;
    srcRect.u32Height = pstSrcRect->u32Height;

    dstSurface.u32Width = pstDst->u32Width;
    dstSurface.u32Height = pstDst->u32Height;
    dstSurface.u32Stride = pstDst->u32Stride;
    dstSurface.PhyAddr = pstDst->PhyAddr;
    dstRect.s32Xpos = pstDstRect->s32Xpos;
    dstRect.s32Ypos = pstDstRect->s32Ypos;
    dstRect.u32Width = pstDstRect->u32Width;
    dstRect.u32Height = pstDstRect->u32Height;

    switch (pstSrc->enColorFmt)
    {
        case SAL_VIDEO_DATFMT_ARGB_8888:
        {
            srcSurface.enColorFmt = TDE2_COLOR_FMT_ARGB8888;
            dstSurface.enColorFmt = TDE2_COLOR_FMT_ARGB8888;
            srcSurface.bAlphaExt1555 = HI_FALSE;
            dstSurface.bAlphaExt1555 = HI_FALSE;
            srcSurface.bAlphaMax255 = HI_TRUE;
            srcSurface.u8Alpha0 = 0XFF;
            srcSurface.u8Alpha1 = 0XFF;
            break;
        }
        case SAL_VIDEO_DATFMT_ARGB16_1555:
        {
            srcSurface.enColorFmt = TDE2_COLOR_FMT_ARGB1555;
            dstSurface.enColorFmt = TDE2_COLOR_FMT_ARGB1555;
            srcSurface.bAlphaExt1555 = HI_TRUE;
            dstSurface.bAlphaExt1555 = HI_TRUE;
            srcSurface.bAlphaMax255 = HI_TRUE;
            srcSurface.u8Alpha0 = 0XFF;
            srcSurface.u8Alpha1 = 0XFF;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420P:
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        {
            srcSurface.bAlphaMax255 = HI_TRUE;
            srcSurface.enColorFmt = TDE2_COLOR_FMT_byte;
            dstSurface.bAlphaMax255 = HI_TRUE;
            dstSurface.enColorFmt = TDE2_COLOR_FMT_byte;
            break;
        }
        default:
        {
            srcSurface.bAlphaMax255 = HI_TRUE;
            srcSurface.enColorFmt = TDE2_COLOR_FMT_byte;
            dstSurface.bAlphaMax255 = HI_TRUE;
            dstSurface.enColorFmt = TDE2_COLOR_FMT_byte;
            break;
        }

    }

    s32Ret = tde_hisi_QuickCopy(&srcSurface, &srcRect, &dstSurface, &dstRect, bCached);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    himpi_TdeQuickDeflicker
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
HI_S32 tde_hisi_QuickDeflicker(TDE2_SURFACE_S *pstSrc,
                               TDE2_RECT_S *pstSrcRect,
                               TDE2_SURFACE_S *pstDst,
                               TDE2_RECT_S *pstDstRect)
{
    TDE_HANDLE s32Handle;
    HI_S32 s32Ret;

    s32Handle = HI_TDE2_BeginJob();
    if (HI_ERR_TDE_INVALID_HANDLE == s32Handle || HI_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_QuickDeflicker(s32Handle, pstSrc, pstSrcRect, pstDst, pstDstRect);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickFill failed %d %#x!\n", s32Handle, s32Ret);

        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 100);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Handle);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    return s32Ret;
}

/*******************************************************************************
* 函数名  : himpi_TdeQuickResizeBitblit
* 描  述  : 2D加速位图缩放和拼图。
            拼图合成的背景图为pstSrc，背景图操作区域和前景图操作区域都为pstSrcRect，
            拼图合成的目标图为pstDst，目标操作区域为pstDstRect。
* 输  入  : - pstSrc       : 源位图
*         : - pstSrcRect   : 源位图操作区域，同时也是合成拼图的背景操作区域和前景操作区域。
*         : - pstDst       : 目标位图，同时也是合成拼图的目标图。
*         : - pstDstRect   : 目标位图操作区域，同时也是合成拼图的: 背景图目标操作区域、前景位图操作区域、目标位图操作区域。
*         : - pstForeGround: 前景位图
*         : - pstOpt       : 运算参数设置结构
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
HI_S32 tde_hisi_QuickResizeBitblit(TDE2_SURFACE_S *pstSrc,
                                   TDE2_RECT_S *pstSrcRect,
                                   TDE2_SURFACE_S *pstDst,
                                   TDE2_RECT_S *pstDstRect,
                                   TDE2_SURFACE_S *pstForeGround,
                                   TDE2_OPT_S *pstOpt)
{
    TDE_HANDLE s32Handle;
    HI_S32 s32Ret;

    s32Handle = HI_TDE2_BeginJob();
    if (HI_ERR_TDE_INVALID_HANDLE == s32Handle || HI_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_QuickResize(s32Handle, pstSrc, pstSrcRect, pstDst, pstDstRect);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickFill failed %d %#x!\n", s32Handle, s32Ret);

        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_Bitblit(s32Handle, pstDst, pstDstRect, pstForeGround, pstDstRect, pstDst, pstDstRect, pstOpt);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_Bitblit failed,ret=0x%x!\n", s32Ret);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    /* 提交2D加速任务 */
    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 100);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Handle);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    return s32Ret;
}

/*******************************************************************************
* 函数名  : himpi_TdeMb2Mb
* 描  述  : 向视频job 中添加对宏块位图进行有附加功能的搬移操作。将源宏块位图的操
            作区域格式转换后输出到目的宏块位图的操作区域。在转换的过程中可以伴随
            缩放、抗闪烁、Clip 处理
* 输  入  : - pstMBIn   : 输入宏块信息
*         : - pstInRect : 输入宏块操作矩形
*         : - pstMbOut  : 输出宏块信息
*         : - pstOutRect: 输出宏块操作矩形
*         : - pstMbOpt  : 宏块位图的操作属性
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
HI_S32 tde_hisi_Mb2Mb(TDE2_MB_S *pstMBIn,
                      TDE2_RECT_S *pstInRect,
                      TDE2_MB_S *pstMbOut,
                      TDE2_RECT_S *pstOutRect,
                      TDE2_MBOPT_S *pstMbOpt)
{
    TDE_HANDLE s32Handle;
    HI_S32 s32Ret;

    s32Handle = HI_TDE2_BeginJob();
    if (HI_ERR_TDE_INVALID_HANDLE == s32Handle || HI_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

#ifdef HI3516av200
    s32Ret = HI_TDE2_Mb2Mb(s32Handle, pstMBIn, pstInRect, pstMbOut, pstOutRect, pstMbOpt);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_Mb2Mb failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

#endif

    /* submit the job 阻塞式的提交已创建的TDE 任务。 */
    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 20);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Handle);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : himpi_TdeMbBlit
* 描  述  : 位图合成。宏块 surface 指定区域的亮度和色度数据合并成光栅格式输出到
            目标surface 的指定区域。在合并的过程中可以伴随缩放操作，由pstMbOpt
            的enResize 参数指定。如果没有指定缩放，将直接将宏块数据合并的结果输
            出到目标surface 上，超出的部分将剪切掉。当clip 开关打开时，将做剪切
            拷贝；合并过程中也支持抗闪烁处理
* 输  入  : - pstMB     : 宏块surface
*         : - pstMbRect : 宏块操作区域
*         : - pstDst    : 目标位图
*         : - pstDstRect: 目标操作区域
*         : - pstMbOpt  : 宏块操作属性
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
HI_S32 tde_hisi_MbBlit(TDE2_MB_S *pstMB,
                       TDE2_RECT_S *pstMbRect,
                       TDE2_SURFACE_S *pstDst,
                       TDE2_RECT_S *pstDstRect,
                       TDE2_MBOPT_S *pstMbOpt)
{
    TDE_HANDLE s32Handle;
    HI_S32 s32Ret;

    s32Handle = HI_TDE2_BeginJob();
    if (HI_ERR_TDE_INVALID_HANDLE == s32Handle || HI_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_MbBlit(s32Handle, pstMB, pstMbRect, pstDst, pstDstRect, pstMbOpt);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_MbBlit failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    /* submit the job 阻塞式的提交已创建的TDE 任务。 */
    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 20);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Handle);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : himpi_TdeBitblit
* 描  述  : 对光栅位图进行有附加功能的搬移操作
* 输  入  : - *pstForeGround      : 前景位图
          : - *pstForeGroundRect  : 前景位图区域
          : - *pstDst             : 目标位图
          : - *pstDstRect         : 目标位图区域
          : - *pstOpt             : TDE运算的配置信息
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
HI_S32 tde_hisi_Bitblit(TDE2_SURFACE_S *pstForeGround,
                        TDE2_RECT_S *pstForeGroundRect,
                        TDE2_SURFACE_S *pstDst,
                        TDE2_RECT_S *pstDstRect,
                        TDE2_OPT_S *pstOpt)
{
    TDE_HANDLE s32Handle;
    HI_S32 s32Ret;

    s32Handle = HI_TDE2_BeginJob();
    if (HI_ERR_TDE_INVALID_HANDLE == s32Handle || HI_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    s32Ret = HI_TDE2_Bitblit(s32Handle, pstDst, pstDstRect, pstForeGround, pstForeGroundRect, pstDst, pstDstRect, pstOpt);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_MbBlit failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    /* submit the job 阻塞式的提交已创建的TDE 任务。 */
    s32Ret = HI_TDE2_EndJob(s32Handle, HI_FALSE, HI_TRUE, 20);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Handle);
        HI_TDE2_CancelJob(s32Handle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    himpi_TdeInit
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
HI_S32 tde_hisi_Init()
{
    (void)HI_TDE2_Open();

    return SAL_SOK;
}

/**
 * @function	tde_hal_register
 * @brief		对平台相关的内存申请函数进行注册
 * @param[in]
 * @param[out]
 * @return
 */
TDE_PLAT_OPS_ST *tde_hal_register(void)
{
    g_stTdePlatOps.pTdeInit = tde_hisi_Init;
    g_stTdePlatOps.pTdeQuickResize = tde_hisi_QuickResizeTrans;
    g_stTdePlatOps.pTdeQuickCopy = tde_hisi_QuickCopyTrans; 
    g_stTdePlatOps.pTdeQuickCopyYuv = tde_hisi_QuickCopyYuv; 

   return &g_stTdePlatOps;
}


