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
td_s32 tde_ssv5_QuickResize(ot_tde_single_src *single_src)
{
    td_s32 s32Handle;
    td_s32 s32Ret;

    s32Handle = ss_tde_begin_job();
    if (OT_ERR_TDE_INVALID_HANDLE == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    s32Ret = ss_tde_quick_resize(s32Handle, single_src);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickResize failed %d %#x!\n", s32Handle, s32Ret);
        ss_tde_cancel_job(s32Handle);
        return SAL_FAIL;
    }

    s32Ret = ss_tde_end_job(s32Handle, TD_FALSE, TD_TRUE, 100);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Handle);
        ss_tde_cancel_job(s32Handle);
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
td_s32 tde_ssv5_QuickResizeTrans(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect)
{
    td_s32 s32Ret = SAL_SOK;
    ot_tde_single_src single_src = {0};
    ot_tde_rect src_rect = {0};
    ot_tde_rect dst_rect = {0};
    ot_tde_surface src_surface = {0};
    ot_tde_surface dst_surface = {0};

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

    s32Ret = ss_tde_open();
    if (s32Ret < 0)
    {
        SAL_ERROR("HI_TDE2_Open failed :%d!\n", s32Ret);
        return s32Ret;
    }

    /* 0. open tde */
    src_rect.pos_x = pstSrcRect->s32Xpos;
    src_rect.pos_x = pstSrcRect->s32Ypos;
    src_rect.height = pstSrcRect->u32Height;
    src_rect.width = pstSrcRect->u32Width;

    src_surface.width = pstSrc->u32Width;
    src_surface.height = pstSrc->u32Height;
    src_surface.stride = pstSrc->u32Stride;
    src_surface.phys_addr = pstSrc->PhyAddr;
    src_surface.alpha_max_is_255 = TD_TRUE;
    src_surface.alpha0 = 0XFF;
    src_surface.alpha1 = 0XFF;

    dst_rect.pos_x = pstDstRect->s32Xpos;
    dst_rect.pos_y = pstDstRect->s32Ypos;
    dst_rect.height = pstDstRect->u32Height;
    dst_rect.width = pstDstRect->u32Width;

    dst_surface.width = pstDst->u32Width;
    dst_surface.height = pstDst->u32Height;
    dst_surface.stride = pstDst->u32Stride;
    dst_surface.phys_addr = pstDst->u32Height;
    dst_surface.alpha_max_is_255 = TD_TRUE;

    switch (pstSrc->enColorFmt)
    {
        case SAL_VIDEO_DATFMT_ARGB_8888:
        {
            src_surface.color_format = OT_TDE_COLOR_FORMAT_ARGB8888;
            dst_surface.color_format = OT_TDE_COLOR_FORMAT_ARGB8888;
            src_surface.support_alpha_ex_1555 = TD_FALSE;
            dst_surface.support_alpha_ex_1555 = TD_FALSE;
            break;
        }
        case SAL_VIDEO_DATFMT_ARGB16_1555:
        {
            src_surface.color_format = OT_TDE_COLOR_FORMAT_ARGB1555;
            dst_surface.color_format = OT_TDE_COLOR_FORMAT_ARGB1555;
            src_surface.support_alpha_ex_1555 = TD_TRUE;
            dst_surface.support_alpha_ex_1555 = TD_TRUE;
            break;
        }
        default:
        {
            src_surface.color_format = OT_TDE_COLOR_FORMAT_ARGB8888;
            dst_surface.color_format = OT_TDE_COLOR_FORMAT_ARGB8888;
            src_surface.support_alpha_ex_1555 = TD_FALSE;
            dst_surface.support_alpha_ex_1555 = TD_FALSE;
            break;
        }
    }

    single_src.src_surface = &src_surface;
    single_src.dst_surface = &dst_surface;
    single_src.src_rect = &src_rect;
    single_src.dst_rect = &dst_rect;

    s32Ret = tde_ssv5_QuickResize(&single_src);
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("error\n");
        return s32Ret;
    }

    return s32Ret;
}

/*******************************************************************************
* 函数名  : HAL_TdeQuickCopyYuv
* 描  述  : 使用TDE快速拷贝yuv
* 输  入  :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 tde_ssv5_QuickCopyYuv(TDE_HAL_SURFACE *pstSrc,
                            TDE_HAL_RECT  *pstSrcRect,
                            TDE_HAL_SURFACE *pstDst,
                            TDE_HAL_RECT *pstDstRect,
                            UINT32 u32SkipLen,
							UINT32 bCached)
{
    INT32 s32Ret = 0;
    td_s32 s32Handle;
    ot_tde_single_src single_src = {0};
    ot_tde_rect src_rect = {0};
    ot_tde_rect dst_rect = {0};
    ot_tde_surface src_surface = {0};
    ot_tde_surface dst_surface = {0};

    s32Handle = ss_tde_begin_job();

    if (OT_ERR_TDE_INVALID_HANDLE == s32Handle || OT_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    src_surface.alpha_max_is_255 = TD_TRUE;
    src_surface.color_format = OT_TDE_COLOR_FORMAT_BYTE;
    dst_surface.alpha_max_is_255 = TD_TRUE;
    dst_surface.color_format = OT_TDE_COLOR_FORMAT_BYTE;

    /* Y分量 */
    src_surface.width = pstSrc->u32Width;
    src_surface.height = pstSrc->u32Height;
    src_surface.stride = pstSrc->u32Stride;
    src_surface.phys_addr = (td_phys_addr_t)pstSrc->PhyAddr;
    src_rect.pos_x = 0;
    src_rect.pos_y = 0;
    src_rect.width = pstSrc->u32Width;
    src_rect.height = pstSrc->u32Height;

    dst_surface.width = pstDst->u32Width;
    dst_surface.height = pstDst->u32Height;
    dst_surface.stride = pstDst->u32Stride;
    dst_surface.phys_addr = (td_phys_addr_t)pstDst->PhyAddr;
    dst_rect.pos_x = 0;
    dst_rect.pos_y = 0;
    dst_rect.width = pstDst->u32Width;
    dst_rect.height = pstDst->u32Height;

    if (bCached)
    {
        s32Ret = ss_mpi_sys_flush_cache(0, (void *)1, 0);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("ss_mpi_sys_flush_cache failed, error code :%#x \n", s32Ret);
            return s32Ret;
        }
    }

    s32Ret = ss_tde_quick_copy(s32Handle, &single_src);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickCopy failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        ss_tde_cancel_job(s32Handle);
        return SAL_FAIL;
    }

    /* UV分量 */
    src_surface.width = pstSrc->u32Width;
    src_surface.height = pstSrc->u32Height / 2;
    src_surface.stride = pstSrc->u32Stride;
    src_surface.phys_addr = (td_phys_addr_t)pstSrc->PhyAddr + pstSrc->u32Stride * pstSrc->u32Height + u32SkipLen;
    src_rect.pos_x = 0;
    src_rect.pos_y = 0;
    src_rect.width = pstSrc->u32Width;
    src_rect.height = pstSrc->u32Height;

    dst_surface.width = pstDst->u32Width;
    dst_surface.height = pstDst->u32Height / 2;
    dst_surface.stride = pstDst->u32Stride;
    dst_surface.phys_addr = (td_phys_addr_t)pstDst->PhyAddr + pstDst->u32Stride * pstDst->u32Height;
    dst_rect.pos_x = 0;
    dst_rect.pos_y = 0;
    dst_rect.width = pstDst->u32Width;
    dst_rect.height = pstDst->u32Height;

    single_src.src_surface = &src_surface;
    single_src.dst_surface = &dst_surface;
    single_src.src_rect = &src_rect;
    single_src.dst_rect = &dst_rect;

    s32Ret = ss_tde_quick_copy(s32Handle, &single_src);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickCopy failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        ss_tde_cancel_job(s32Handle);
        return SAL_FAIL;
    }

    s32Ret = ss_tde_end_job(s32Handle, TD_FALSE, TD_TRUE, 100);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Ret);
        ss_tde_cancel_job(s32Handle);
        return SAL_FAIL;
    }

    if (bCached)
    {
        s32Ret = ss_mpi_sys_flush_cache(0, (void *)1, 0);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("ss_mpi_sys_flush_cache failed, error code :%#x \n", s32Ret);
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
td_s32 tde_ssv5_QuickCopy(ot_tde_single_src *single_src, UINT32 bCached)
{
    td_s32 s32Handle;
    td_s32 s32Ret;
    
    if (bCached)
    {
        s32Ret = ss_mpi_sys_flush_cache(0, (void *)1, 0);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("ss_mpi_sys_flush_cache failed, error code :%#x \n", s32Ret);
            return s32Ret;
        }
    }

    s32Handle = ss_tde_begin_job();
    if (OT_ERR_TDE_INVALID_HANDLE == s32Handle || OT_ERR_TDE_DEV_NOT_OPEN == s32Handle)
    {
        SAL_ERROR("HI_TDE2_BeginJob failed %#x!\n", s32Handle);
        return SAL_FAIL;
    }

    s32Ret = ss_tde_quick_copy(s32Handle, single_src);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_QuickCopy failed %d %#x!\n", s32Handle, s32Ret);
        /* 取消TDE 任务及已经成功加入到该任务中的操作。  */
        ss_tde_cancel_job(s32Handle);
        return SAL_FAIL;
    }

    /* submit the job 阻塞式的提交已创建的TDE 任务。 */
    s32Ret = ss_tde_end_job(s32Handle, TD_FALSE, TD_TRUE, 100);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_ERROR("HI_TDE2_EndJob failed %#x!\n", s32Ret);
        ss_tde_cancel_job(s32Handle);
        return SAL_FAIL;
    }

    if (bCached)
    {
        s32Ret = ss_mpi_sys_flush_cache(0, (void *)1, 0);
        if (HI_SUCCESS != s32Ret)
        {
            SAL_LOGE("ss_mpi_sys_flush_cache failed, error code :%#x \n", s32Ret);
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
td_s32 tde_ssv5_QuickCopyTrans(TDE_HAL_SURFACE *pstSrc, TDE_HAL_RECT *pstSrcRect, TDE_HAL_SURFACE *pstDst, TDE_HAL_RECT *pstDstRect, UINT32 bCached)
{
    INT32 s32Ret = 0;
    ot_tde_single_src single_src = {0};
    ot_tde_rect src_rect = {0};
    ot_tde_rect dst_rect = {0};
    ot_tde_surface src_surface = {0};
    ot_tde_surface dst_surface = {0};

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

    src_surface.width = pstSrc->u32Width;
    src_surface.height = pstSrc->u32Height;
    src_surface.stride = pstSrc->u32Stride;
    src_surface.phys_addr = pstSrc->PhyAddr;
    src_rect.pos_x = pstSrcRect->s32Xpos;
    src_rect.pos_y = pstSrcRect->s32Ypos;
    src_rect.width = pstSrcRect->u32Width;
    src_rect.height = pstSrcRect->u32Height;

    dst_surface.width = pstDst->u32Width;
    dst_surface.height = pstDst->u32Height;
    dst_surface.stride = pstDst->u32Stride;
    dst_surface.phys_addr = pstDst->PhyAddr;
    dst_rect.pos_x = pstSrcRect->s32Xpos;
    dst_rect.pos_y = pstSrcRect->s32Ypos;
    dst_rect.width = pstSrcRect->u32Width;
    dst_rect.height = pstSrcRect->u32Height;

    switch (pstSrc->enColorFmt)
    {
        case SAL_VIDEO_DATFMT_ARGB_8888:
        {
            src_surface.color_format = OT_TDE_COLOR_FORMAT_ARGB8888;
            dst_surface.color_format = OT_TDE_COLOR_FORMAT_ARGB8888;
            src_surface.support_alpha_ex_1555 = TD_FALSE;
            dst_surface.support_alpha_ex_1555 = TD_FALSE;
            src_surface.alpha_max_is_255 = TD_TRUE;
            src_surface.alpha0 = 0XFF;
            src_surface.alpha1 = 0XFF;
            break;
        }
        case SAL_VIDEO_DATFMT_ARGB16_1555:
        {
            src_surface.color_format = OT_TDE_COLOR_FORMAT_ARGB1555;
            dst_surface.color_format = OT_TDE_COLOR_FORMAT_ARGB1555;
            src_surface.support_alpha_ex_1555 = TD_TRUE;
            dst_surface.support_alpha_ex_1555 = TD_TRUE;
            src_surface.alpha_max_is_255 = TD_TRUE;
            src_surface.alpha0 = 0XFF;
            src_surface.alpha1 = 0XFF;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420P:
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        {
            src_surface.alpha_max_is_255 = TD_TRUE;
            src_surface.color_format = OT_TDE_COLOR_FORMAT_BYTE;
            dst_surface.alpha_max_is_255 = TD_TRUE;
            dst_surface.color_format = OT_TDE_COLOR_FORMAT_BYTE;
            break;
        }
        default:
        {
            src_surface.alpha_max_is_255 = TD_TRUE;
            src_surface.color_format = OT_TDE_COLOR_FORMAT_BYTE;
            dst_surface.alpha_max_is_255 = TD_TRUE;
            dst_surface.color_format = OT_TDE_COLOR_FORMAT_BYTE;
            break;
        }

    }

    single_src.src_surface = &src_surface;
    single_src.dst_surface = &dst_surface;
    single_src.src_rect = &src_rect;
    single_src.dst_rect = &dst_rect;

    s32Ret = tde_ssv5_QuickCopy(&single_src, bCached);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("err %#x\n", s32Ret);
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
td_s32 tde_ssv5_Init()
{
    (void)ss_tde_open();

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
    g_stTdePlatOps.pTdeInit = tde_ssv5_Init;
    g_stTdePlatOps.pTdeQuickResize = tde_ssv5_QuickResizeTrans;
    g_stTdePlatOps.pTdeQuickCopy = tde_ssv5_QuickCopyTrans;
    g_stTdePlatOps.pTdeQuickCopyYuv = tde_ssv5_QuickCopyYuv;

    return &g_stTdePlatOps;
}

