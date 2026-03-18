/*******************************************************************************
* tde_rk.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2022年3月7日 Create
*
* Description : 图形加速函数封装
* Modification:
*******************************************************************************/

#include <sal.h>
#include "platform_sdk.h"
#include "tde_hal_inter.h"
#include "plat_com_inter_hal.h"

static TDE_PLAT_OPS_ST g_stTdePlatOps;

/*******************************************************************************
* 函数名  : tde_rk_TransOuter2Inner
* 描  述  : 将用户层参数转换为平台支持的数据结构
* 输  入  : - pstSrc   : 输入宏块信息
*         : - pstSrcRect : 输入宏块操作矩形
*         : - pstDst  : 输出宏块信息
*         : - pstDstRect: 输出宏块操作矩形
* 输  出  : - pstSrcSurface   : 平台层输入宏块信息
*         : - pstSrcSurfaceRect : 平台层输入宏块操作矩形
*         : - pstDstSurface  : 平台层输出宏块信息
*         : - pstDstSurfaceRect: 平台层输出宏块操作矩形
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
static INT32 tde_rk_TransOuter2Inner(IN TDE_HAL_SURFACE *pstHalSrc, 
                                     IN TDE_HAL_RECT *pstHalSrcRect, 
                                     IN TDE_HAL_SURFACE *pstHalDst, 
                                     IN TDE_HAL_RECT *pstHalDstRect,
                                     OUT TDE_SURFACE_S *pstSrcSurface,
                                     OUT TDE_RECT_S *pstSrcSurfaceRect,
                                     OUT TDE_SURFACE_S *pstDstSurface,
                                     OUT TDE_RECT_S *pstDstSurfaceRect)
{

    /* checker. 默认pstHalSrc必须有效 */
    if (NULL == pstHalSrc 
        || NULL == pstHalSrcRect
        || NULL == pstSrcSurface
        || NULL == pstSrcSurfaceRect
        || (pstHalDstRect && NULL == pstDstSurfaceRect)
        || (pstHalDst && NULL == pstDstSurface))
    {
        SAL_LOGE("tde invalid prm! %p, %p, %p, %p, %p, %p, %p, %p \n",
                  pstHalSrc, pstHalSrcRect, pstHalDst, pstHalDstRect, 
                  pstSrcSurface, pstSrcSurfaceRect, pstDstSurface, pstDstSurfaceRect);
        goto err;
    }

    
    pstSrcSurfaceRect->s32Xpos = pstHalSrcRect->s32Xpos;
    pstSrcSurfaceRect->s32Ypos = pstHalSrcRect->s32Ypos;
    pstSrcSurfaceRect->u32Height = pstHalSrcRect->u32Height;
    pstSrcSurfaceRect->u32Width = pstHalSrcRect->u32Width;

    pstSrcSurface->u32Width = pstHalSrc->u32Width;
    pstSrcSurface->u32Height = pstHalSrc->u32Height;

    /* 若vb_blk未赋值，底层会报错 */
    if (0 == pstHalSrc->vbBlk)
    {
        SAL_LOGE("MB_BLK %llu ! \n", pstHalSrc->vbBlk);
        goto err;
    }
    
    pstSrcSurface->pMbBlk = (MB_BLK)pstHalSrc->vbBlk;  
    pstSrcSurface->enComprocessMode = COMPRESS_MODE_NONE;    // 当前默认外部业务模块使用非压缩模式

    if (pstHalDstRect)
    {       
        pstDstSurfaceRect->s32Xpos = pstHalDstRect->s32Xpos;
        pstDstSurfaceRect->s32Ypos = pstHalDstRect->s32Ypos;
        pstDstSurfaceRect->u32Height = pstHalDstRect->u32Height;
        pstDstSurfaceRect->u32Width = pstHalDstRect->u32Width;
    }
    
    if (pstHalDst)
    {

        if (0 == pstHalDst->vbBlk)
        {
            SAL_LOGE("MB_BLK %llu ! \n",  pstHalDst->vbBlk);
            goto err;
        }
        
        if (pstHalSrc->enColorFmt != pstHalDst->enColorFmt)
        {
            SAL_LOGD("tde resize trans color error \n");
            //goto err;
        }

        pstDstSurface->u32Width = pstHalDst->u32Width;
        pstDstSurface->u32Height = pstHalDst->u32Height;
        pstDstSurface->pMbBlk = (MB_BLK)pstHalDst->vbBlk;
        pstDstSurface->enComprocessMode = COMPRESS_MODE_NONE;    // 当前默认外部业务模块使用非压缩模式
    }


    switch (pstHalSrc->enColorFmt)
    {
        case SAL_VIDEO_DATFMT_ARGB_8888:
        {
            pstSrcSurface->enColorFmt = RK_FMT_BGRA8888;
            break;
        }
        case SAL_VIDEO_DATFMT_RGB24_888:
        {
            pstSrcSurface->enColorFmt = RK_FMT_BGR888;
            break;
        }
        case SAL_VIDEO_DATFMT_ARGB16_1555:
        {
            pstSrcSurface->enColorFmt = RK_FMT_ARGB1555;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420SP_VU:
        {
            pstSrcSurface->enColorFmt = RK_FMT_YUV420SP_VU;
            break;
        }
        case SAL_VIDEO_DATFMT_YUV420SP_UV:
        {
            pstSrcSurface->enColorFmt = RK_FMT_YUV420SP;
            break;
        }
        case SAL_VIDEO_DATFMT_RGBA_8888:
        {
            pstSrcSurface->enColorFmt = RK_FMT_BGRA8888;
            break;
        } 
        case SAL_VIDEO_DATFMT_BAYER_RAW_DUAL:
        case SAL_VIDEO_DATFMT_BAYER_RAW_SINGLE:
        case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
        {       
            pstSrcSurface->enColorFmt = RK_FMT_RGB_BAYER_SBGGR_8BPP;
            break;
        }

        default:
        {           
            pstSrcSurface->enColorFmt = RK_FMT_BGRA8888;
            break;
        }
    }
    
    if (pstHalDst)
    {
        switch (pstHalDst->enColorFmt)
        {
            case  SAL_VIDEO_DATFMT_ARGB_8888:
            {
                pstDstSurface->enColorFmt = RK_FMT_BGRA8888;
                break;
            }
            case SAL_VIDEO_DATFMT_ARGB16_1555:
            {
                pstDstSurface->enColorFmt = RK_FMT_ARGB1555;
                break;
            }
            case SAL_VIDEO_DATFMT_YUV420SP_VU:
            {
                pstDstSurface->enColorFmt = RK_FMT_YUV420SP_VU;
                break;
            }
            case SAL_VIDEO_DATFMT_YUV420SP_UV:
            {
                pstDstSurface->enColorFmt = RK_FMT_YUV420SP;
                break;
            }
            case SAL_VIDEO_DATFMT_RGB24_888:
            {
                pstDstSurface->enColorFmt = RK_FMT_BGR888;
                break;
            }
            case SAL_VIDEO_DATFMT_BAYER_RAW_DUAL:
            case SAL_VIDEO_DATFMT_BAYER_RAW_SINGLE:
            case SAL_VIDEO_DATFMT_BAYER_FMT_BYTE:
            {
                pstDstSurface->enColorFmt = RK_FMT_RGB_BAYER_SBGGR_8BPP;
                break;
            }
            default:
            {
                pstDstSurface->enColorFmt = RK_FMT_BGRA8888;
                break;
            }
        }
    }

    return SAL_SOK;
    
err:
    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : tde_rk_QuickResize
* 描  述  : 光栅位图缩放入参整合 因海思结构体不能出现上层而封装
* 输  入  : - pstSrc    : 源位图
*         : - pstSrcRect: 源位图操作区域
*         : - pstDst    : 目标位图
*         : - pstDstRect: 目标位图操作区域
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 tde_rk_QuickResize(TDE_HAL_SURFACE *pstSrc, 
                         TDE_HAL_RECT *pstSrcRect, 
                         TDE_HAL_SURFACE *pstDst, 
                         TDE_HAL_RECT *pstDstRect)
{
    INT32 s32Ret = SAL_FAIL;

    TDE_HANDLE hHandle = -1;

    TDE_RECT_S stSrcRect = {0};
    TDE_RECT_S stDstRect = {0};
    TDE_SURFACE_S stSrc = {0};
    TDE_SURFACE_S stDst = {0};

    /* 将用户层参数转换为平台支持的结构体 */
    s32Ret = tde_rk_TransOuter2Inner(pstSrc, pstSrcRect, pstDst, pstDstRect,
                                     &stSrc, &stSrcRect, &stDst, &stDstRect);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("tde_rk_TransOuter2Inner failed! \n");
        goto exit;
    }

    hHandle = RK_TDE_BeginJob();
    if (RK_ERR_TDE_INVALID_HANDLE == hHandle)
    {
        SAL_LOGE("start job fail \n");
        goto exit;
    }

    s32Ret = RK_TDE_QuickResize(hHandle, &stSrc, &stSrcRect, &stDst, &stDstRect);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_QuickResize failed! 0x%x! \n", s32Ret);
        goto cancel;
    }

    s32Ret = RK_TDE_EndJob(hHandle, RK_FALSE, RK_TRUE, 10);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_EndJob failed! 0x%x! \n", s32Ret);
        goto cancel;
    }
    
    s32Ret = RK_TDE_WaitForDone(hHandle);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_WaitForDone failed! 0x%x! \n", s32Ret);
        goto cancel;
    }

    return SAL_SOK;

cancel:
    (VOID)RK_TDE_CancelJob(hHandle);

exit:
    return s32Ret;
}

/*******************************************************************************
* 函数名  : tde_rk_QuickCopy
* 描  述  : 使用TDE快速拷贝图像
* 输  入  : - pstSrc   : 输入宏块信息
*         : - pstSrcRect : 输入宏块操作矩形
*         : - pstDst  : 输出宏块信息
*         : - pstDstRect: 输出宏块操作矩形
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 tde_rk_QuickCopy(TDE_HAL_SURFACE *pstSrc, 
                       TDE_HAL_RECT *pstSrcRect, 
                       TDE_HAL_SURFACE *pstDst, 
                       TDE_HAL_RECT *pstDstRect,
                       UINT32 bCached)
{
    INT32 s32Ret = SAL_FAIL;

    TDE_HANDLE hHandle = -1;

    TDE_RECT_S stSrcRect = {0};
    TDE_RECT_S stDstRect = {0};
    TDE_SURFACE_S stSrc = {0};
    TDE_SURFACE_S stDst = {0};

    if (bCached)
    {
        s32Ret = RK_MPI_SYS_MmzFlushCache((void*)pstSrc->vbBlk, 0);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", s32Ret);
            return s32Ret;
        }
    }
    if (bCached)
    {
        s32Ret = RK_MPI_SYS_MmzFlushCache((void*)pstDst->vbBlk, 0);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", s32Ret);
            return s32Ret;
        }
    }

    /* 将用户层参数转换为平台支持的结构体 */
    s32Ret = tde_rk_TransOuter2Inner(pstSrc, pstSrcRect, pstDst, pstDstRect,
                                     &stSrc, &stSrcRect, &stDst, &stDstRect);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("tde_rk_TransOuter2Inner failed! \n");
        goto exit;
    }

    hHandle = RK_TDE_BeginJob();
    if (RK_ERR_TDE_INVALID_HANDLE == hHandle)
    {
        SAL_LOGE("start job fail \n");
        goto exit;
    }

    s32Ret = RK_TDE_QuickCopy(hHandle, &stSrc, &stSrcRect, &stDst, &stDstRect);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_QuickCopy failed! 0x%x! \n", s32Ret);
        goto cancel;
    }

    s32Ret = RK_TDE_EndJob(hHandle, RK_FALSE, RK_TRUE, 10);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_EndJob failed! 0x%x! \n", s32Ret);
        goto cancel;
    }
    
    s32Ret = RK_TDE_WaitForDone(hHandle);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_WaitForDone failed! 0x%x! \n", s32Ret);
        goto cancel;
    }

    if (bCached)
    {
        s32Ret = RK_MPI_SYS_MmzFlushCache((void*)pstSrc->vbBlk, 0);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", s32Ret);
            return s32Ret;
        }
    }
    if (bCached)
    {
        s32Ret = RK_MPI_SYS_MmzFlushCache((void*)pstDst->vbBlk, 0);
        if (SAL_SOK != s32Ret)
        {
            SAL_LOGE("HI_MPI_SYS_MmzFlushCache failed, error code :%#x \n", s32Ret);
            return s32Ret;
        }
    }

    return SAL_SOK;

cancel:
    (VOID)RK_TDE_CancelJob(hHandle);

exit:

    return s32Ret;
}

/*******************************************************************************
* 函数名  : tde_rk_QuickCopyYuv
* 描  述  : 使用TDE快速拷贝yuv
* 输  入  :
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 tde_rk_QuickCopyYuv(TDE_HAL_SURFACE *pstSrc,
                          TDE_HAL_RECT  *pstSrcRect,
                          TDE_HAL_SURFACE *pstDst,
                          TDE_HAL_RECT *pstDstRect,
                          UINT32 u32SkipLen,
                          UINT32 bCached)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = tde_rk_QuickCopy(pstSrc, pstSrcRect, pstDst, pstDstRect, bCached);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("tde copy y planar failed! \n");
        goto exit;
    }
exit:
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
INT32 ATTRIBUTE_UNUSED tde_rk_QuickFill(TDE_HAL_SURFACE *pstSrc, 
                                        TDE_HAL_RECT *pstSrcRect, 
                                        UINT32 u32FillData)
{
    INT32 s32Ret = SAL_FAIL;

    TDE_HANDLE hHandle = -1;

    TDE_RECT_S stSrcRect = {0};
    TDE_SURFACE_S stSrc = {0};

    /* 将用户层参数转换为平台支持的结构体 */
    s32Ret = tde_rk_TransOuter2Inner(pstSrc, pstSrcRect, NULL, NULL,
                                     &stSrc, &stSrcRect, NULL, NULL);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("tde_rk_TransOuter2Inner failed! \n");
        goto exit;
    }

    hHandle = RK_TDE_BeginJob();
    if (RK_ERR_TDE_INVALID_HANDLE == hHandle)
    {
        SAL_LOGE("start job fail \n");
        goto exit;
    }

    s32Ret = RK_TDE_QuickFill(hHandle, &stSrc, &stSrcRect, u32FillData);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_QuickFill failed! 0x%x! \n", s32Ret);
        goto cancel;
    }

    s32Ret = RK_TDE_EndJob(hHandle, RK_FALSE, RK_TRUE, 10);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_EndJob failed! 0x%x! \n", s32Ret);
        goto cancel;
    }
    
    s32Ret = RK_TDE_WaitForDone(hHandle);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_WaitForDone failed! 0x%x! \n", s32Ret);
        goto cancel;
    }
    
    return SAL_SOK;

cancel:
    (VOID)RK_TDE_CancelJob(hHandle);

exit:
    return s32Ret;
}

/*******************************************************************************
* 函数名  : tde_rk_QuickRotate
* 描  述  : 向任务中添加快速旋转操作。
* 输  入  : - pstSrc    : 源位图
*         : - pstSrcRect: 源位图操作区域
*         : - pstDst    : 目标位图
*         : - pstDstRect: 目标位图操作区域
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 ATTRIBUTE_UNUSED tde_rk_QuickRotate(TDE_HAL_SURFACE *pstSrc,
                                          TDE_HAL_RECT  *pstSrcRect,
                                          TDE_HAL_SURFACE *pstDst,
                                          TDE_HAL_RECT *pstDstRect,
                                          TDE_HAL_ROTATION_E enRotateType)
{
    INT32 s32Ret = SAL_FAIL;

    TDE_HANDLE hHandle = -1;

    TDE_RECT_S stSrcRect = {0};
    TDE_RECT_S stDstRect = {0};
    TDE_SURFACE_S stSrc = {0};
    TDE_SURFACE_S stDst = {0};

    /* 将用户层参数转换为平台支持的结构体 */
    s32Ret = tde_rk_TransOuter2Inner(pstSrc, pstSrcRect, pstDst, pstDstRect,
                                     &stSrc, &stSrcRect, &stDst, &stDstRect);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("tde_rk_TransOuter2Inner failed! \n");
        goto exit;
    }

    hHandle = RK_TDE_BeginJob();
    if (RK_ERR_TDE_INVALID_HANDLE == hHandle)
    {
        SAL_LOGE("start job fail \n");
        goto exit;
    }

    s32Ret = RK_TDE_Rotate(hHandle, &stSrc, &stSrcRect, &stDst, &stDstRect, enRotateType);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_QuickFill failed! 0x%x! \n", s32Ret);
        goto cancel;
    }

    s32Ret = RK_TDE_EndJob(hHandle, RK_FALSE, RK_TRUE, 10);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_EndJob failed! 0x%x! \n", s32Ret);
        goto cancel;
    }
    
    s32Ret = RK_TDE_WaitForDone(hHandle);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_WaitForDone failed! 0x%x! \n", s32Ret);
        goto cancel;
    }

    return SAL_SOK;

cancel:
    (VOID)RK_TDE_CancelJob(hHandle);

exit:
    return s32Ret;
}

/*******************************************************************************
* 函数名  : tde_rk_QuickMirror
* 描  述  : 向任务中添加快速镜像操作。
* 输  入  : - pstSrc    : 源位图
*         : - pstSrcRect: 源位图操作区域
*         : - pstDst    : 目标位图
*         : - pstDstRect: 目标位图操作区域
*         : - TDE_HAL_MIRROR_E enMirrorType: 镜像类型
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 ATTRIBUTE_UNUSED tde_rk_QuickMirror(TDE_HAL_SURFACE *pstSrc,
                                          TDE_HAL_RECT  *pstSrcRect,
                                          TDE_HAL_SURFACE *pstDst,
                                          TDE_HAL_RECT *pstDstRect,
                                          TDE_HAL_MIRROR_E enMirrorType)
{
    INT32 s32Ret = SAL_FAIL;

    TDE_HANDLE hHandle = -1;

    TDE_RECT_S stSrcRect = {0};
    TDE_RECT_S stDstRect = {0};
    TDE_SURFACE_S stSrc = {0};
    TDE_SURFACE_S stDst = {0};
    TDE_OPT_S stOpt = {0};

    /* 将用户层参数转换为平台支持的结构体 */
    s32Ret = tde_rk_TransOuter2Inner(pstSrc, pstSrcRect, pstDst, pstDstRect,
                                     &stSrc, &stSrcRect, &stDst, &stDstRect);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("tde_rk_TransOuter2Inner failed! \n");
        goto exit;
    }

    hHandle = RK_TDE_BeginJob();
    if (RK_ERR_TDE_INVALID_HANDLE == hHandle)
    {
        SAL_LOGE("start job fail \n");
        goto exit;
    }

    /* 用户参数 */
    stOpt.enMirror = (MIRROR_E)enMirrorType;
    stOpt.stClipRect.s32Xpos = pstDstRect->s32Xpos;
    stOpt.stClipRect.s32Ypos = pstDstRect->s32Ypos;
    stOpt.stClipRect.u32Width = pstDstRect->u32Width;
    stOpt.stClipRect.u32Height = pstDstRect->u32Height;

    s32Ret = RK_TDE_Bitblit(hHandle, 
                            &stDst, &stDstRect,   /* 背景位图 */
                            &stSrc, &stSrcRect,   /* 前景位图 */
                            &stDst, &stDstRect,   /* 目的位图 */
                            &stOpt);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_Bitblit Mirror failed! 0x%x! \n", s32Ret);
        goto cancel;
    }

    s32Ret = RK_TDE_EndJob(hHandle, RK_FALSE, RK_TRUE, 10);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_EndJob failed! 0x%x! \n", s32Ret);
        goto cancel;
    }
    
    s32Ret = RK_TDE_WaitForDone(hHandle);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_WaitForDone failed! 0x%x! \n", s32Ret);
        goto cancel;
    }

    return SAL_SOK;

cancel:
    (VOID)RK_TDE_CancelJob(hHandle);

exit:
    return s32Ret;
}

/*******************************************************************************
* 函数名  : tde_rk_QuickColorKey
* 描  述  : 向任务中添加快速ColorKey操作。
* 输  入  : - pstSrc    : 源位图
*         : - pstSrcRect: 源位图操作区域
*         : - pstDst    : 目标位图
*         : - pstDstRect: 目标位图操作区域
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 ATTRIBUTE_UNUSED tde_rk_QuickColorKey(TDE_HAL_SURFACE *pstSrc,
                                            TDE_HAL_RECT  *pstSrcRect,
                                            TDE_HAL_SURFACE *pstDst,
                                            TDE_HAL_RECT *pstDstRect,
                                            INT32 s32ColorVal)
{
    INT32 s32Ret = SAL_FAIL;

    TDE_HANDLE hHandle = -1;

    TDE_RECT_S stSrcRect = {0};
    TDE_RECT_S stDstRect = {0};
    TDE_SURFACE_S stSrc = {0};
    TDE_SURFACE_S stDst = {0};
    TDE_OPT_S stOpt = {0};

    /* 将用户层参数转换为平台支持的结构体 */
    s32Ret = tde_rk_TransOuter2Inner(pstSrc, pstSrcRect, pstDst, pstDstRect,
                                     &stSrc, &stSrcRect, &stDst, &stDstRect);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("tde_rk_TransOuter2Inner failed! \n");
        goto exit;
    }
    
    hHandle = RK_TDE_BeginJob();
    if (RK_ERR_TDE_INVALID_HANDLE == hHandle)
    {
        SAL_LOGE("start job fail \n");
        goto exit;
    }

    /* 用户参数 */
    stOpt.enColorKeyMode = TDE_COLORKEY_MODE_NONE;
    stOpt.unColorKeyValue = s32ColorVal;
    stOpt.u32GlobalAlpha  = 0xff;

    s32Ret = RK_TDE_Bitblit(hHandle, 
                            &stDst, &stDstRect,   /* 背景位图 */
                            &stSrc, &stSrcRect,   /* 前景位图 */
                            &stDst, &stDstRect,   /* 目的位图 */
                            &stOpt);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_Bitblit ColorKey failed! 0x%x! \n", s32Ret);
        goto cancel;
    }

    s32Ret = RK_TDE_EndJob(hHandle, RK_FALSE, RK_TRUE, 10);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_EndJob failed! 0x%x! \n", s32Ret);
        goto cancel;
    }
    
    s32Ret = RK_TDE_WaitForDone(hHandle);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_TDE_WaitForDone failed! 0x%x! \n", s32Ret);
        goto cancel;
    }
    
    return SAL_SOK;
    
cancel:
    (VOID)RK_TDE_CancelJob(hHandle);

exit:
    return s32Ret;
}

/**
 * @function    tde_rk_Init
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
INT32 tde_rk_Init(void)
{
    return RK_TDE_Open();
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
    g_stTdePlatOps.pTdeInit = tde_rk_Init;
    g_stTdePlatOps.pTdeQuickResize = tde_rk_QuickResize;
    g_stTdePlatOps.pTdeQuickCopy = tde_rk_QuickCopy; 
    g_stTdePlatOps.pTdeQuickCopyYuv = tde_rk_QuickCopyYuv; 
    g_stTdePlatOps.pTdeQuickRotate = tde_rk_QuickRotate;

   return &g_stTdePlatOps;
}


