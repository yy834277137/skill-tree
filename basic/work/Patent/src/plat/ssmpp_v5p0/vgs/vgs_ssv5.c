/**
 * @file   vgs_ssv5.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台 vgs 模块
 * @author liuxianying
 * @date   2021/8/4
 * @note
 * @note \n History
   1.日    期: 2021/8/4
     作    者: liuxianying
     修改历史: 创建文件
 */
#include "platform_sdk.h"
#include "platform_hal.h"
#include "vgs_hal_inter.h"




/*****************************************************************************
                               宏定义
*****************************************************************************/

/*****************************************************************************
                               结构体定义
*****************************************************************************/


/*****************************************************************************
                               全局变量
*****************************************************************************/
static VGS_PLAT_OPS g_stVgsPlatOps = {0};


/**
 * @function   vgs_ssv5DrawLineArray
 * @brief      使用VGS画多线条
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrame  数据帧
 * @param[in]  VGS_DRAW_LINE_PRM *pstLinePrm      画线参数
 * @param[out] None
 * @return     INT32
 */
INT32 vgs_ssv5DrawLineArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_DRAW_LINE_PRM *pstLinePrm)
{
    INT32 s32Ret = 0;
    UINT32 uiLineNum = 0;
    UINT32 freeCnt = 0; /* 剩余未画线 */
    UINT32 drawCnt = 0; /* 已划线 */
    ot_vgs_task_attr stTask = {0};

    /* UINT32 i = 0; */
    /* UINT64 time1 = 0, time2 = 0, time3 = 0,time4 = 0,time5 = 0,time6 = 0; */
    ot_vgs_handle stHandle = 0;

    if ((NULL == pstSystemFrame) || (NULL == pstLinePrm))
    {
        SAL_LOGE("input is NULL %p,%p\n", pstSystemFrame, pstLinePrm);

        return SAL_FAIL;
    }

    memcpy(&stTask.img_in, (ot_video_frame_info *)pstSystemFrame->uiAppData, sizeof(ot_video_frame_info)); /* 输入图像 */
    memcpy(&stTask.img_out, (ot_video_frame_info *)pstSystemFrame->uiAppData, sizeof(ot_video_frame_info)); /* osd画线后输出图像 */

    s32Ret = ss_mpi_vgs_begin_job(&stHandle);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGE("1111 ret %#x \n", s32Ret);

        return SAL_FAIL;
    }

    if (pstLinePrm->uiLineNum >= MAX_VGS_LINE_NUM)
    {
        pstLinePrm->uiLineNum = MAX_VGS_LINE_NUM;
    }

    uiLineNum = pstLinePrm->uiLineNum;
    freeCnt = uiLineNum;
    if (uiLineNum > 0)
    {
        do
        {
            if (freeCnt > 100)
            {
                s32Ret = ss_mpi_vgs_add_draw_line_task(stHandle, &stTask, (ot_vgs_line *)&pstLinePrm->stVgsLinePrm[drawCnt], 100);
                if (s32Ret != TD_SUCCESS)
                {
                    ss_mpi_vgs_cancel_job(stHandle);
                    return SAL_FAIL;
                }

                freeCnt -= 100;
                drawCnt += 100;
            }
            else
            {
                s32Ret = ss_mpi_vgs_add_draw_line_task(stHandle, &stTask, (ot_vgs_line *)&pstLinePrm->stVgsLinePrm[drawCnt], freeCnt);
                if (s32Ret != TD_SUCCESS)
                {
                    ss_mpi_vgs_cancel_job(stHandle);
                    return SAL_FAIL;
                }

                freeCnt = 0;
            }
        }
        while (freeCnt > 0);
    }

    s32Ret = ss_mpi_vgs_end_job(stHandle);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGE("3333 ret %#x \n", s32Ret);
        ss_mpi_vgs_cancel_job(stHandle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   vgs_ssv5ScaleFrame
 * @brief      使用 VGS 缩放
 * @param[in]  SYSTEM_FRAME_INFO *pDstSystemFrame  
 * @param[in]  SYSTEM_FRAME_INFO *pSrcSystemFrame  
 * @param[in]  UINT32 dstW                         
 * @param[in]  UINT32 dstH                         
 * @param[out] None
 * @return     INT32
 */
INT32 vgs_ssv5ScaleFrame(SYSTEM_FRAME_INFO *pDstSystemFrame, SYSTEM_FRAME_INFO *pSrcSystemFrame, UINT32 dstW, UINT32 dstH)
{
    INT32 s32Ret = SAL_SOK;
    ot_vgs_handle stHandle = 0;
    ot_video_frame_info *pDstVideo = NULL;
    ot_video_frame_info *pSrcVideo = NULL;
    ot_vgs_task_attr stTask = {0};


    pDstVideo = (ot_video_frame_info *)pDstSystemFrame->uiAppData;
    pSrcVideo = (ot_video_frame_info *)pSrcSystemFrame->uiAppData;
    pDstVideo->video_frame.width = dstW;
    pDstVideo->video_frame.height = dstH;
    memcpy(&stTask.img_in, pSrcVideo, sizeof(ot_video_frame_info));
    memcpy(&stTask.img_out, pDstVideo, sizeof(ot_video_frame_info));


    s32Ret = ss_mpi_vgs_begin_job(&stHandle);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGE("ss_mpi_vgs_begin_job ret %#x \n", s32Ret);

        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vgs_add_scale_task(stHandle, &stTask, OT_VGS_SCALE_COEF_NORM);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGW("ss_mpi_vgs_add_scale_task %#x \n", s32Ret);
        ss_mpi_vgs_cancel_job(stHandle);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vgs_end_job(stHandle);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGE("ss_mpi_vgs_end_job ret %#x \n", s32Ret);
        ss_mpi_vgs_cancel_job(stHandle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   vgs_ssv5DrawOSDArray
 * @brief      使用 VGS 画osd
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrame  数据帧
 * @param[in]  VGS_ADD_OSD_PRM *pstOsdPrm         画osd的信息参数    
 * @param[out] None
 * @return     INT32
 */
INT32 vgs_ssv5DrawOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    INT32 s32Ret = 0;
    UINT32 uiOsdNum = 0;
    UINT32 i = 0;
    ot_vgs_task_attr stTask = {0};

    ot_vgs_handle stHandle = 0;
    ot_vgs_osd *pstVgsAddOsd = NULL;
    ot_vgs_osd astVgsAddOsd[MAX_VGS_OSD_NUM];

    if ( NULL == pstOsdPrm)
    {
        SAL_LOGE("input is NULL %p,%p\n", pstSystemFrame, pstOsdPrm);

        return SAL_FAIL;
    }
    memcpy(&stTask.img_in, (ot_video_frame_info *)pstSystemFrame->uiAppData, sizeof(ot_video_frame_info)); /* 输入图像 */
    memcpy(&stTask.img_out, (ot_video_frame_info *)pstSystemFrame->uiAppData, sizeof(ot_video_frame_info)); /* osd画线后输出图像 */

    s32Ret = ss_mpi_vgs_begin_job(&stHandle);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGE("1111 ret %#x \n", s32Ret);

        return SAL_FAIL;
    }

    for (i = 0; i < pstOsdPrm->uiOsdNum; i++)
    {
        if (i >= MAX_VGS_OSD_NUM)
        {
            SAL_LOGW("osd index %d > osd max num %d\n", i, MAX_VGS_OSD_NUM);
            break;
        }

        pstVgsAddOsd = &astVgsAddOsd[i];

        memset(pstVgsAddOsd, 0x00, sizeof(ot_vgs_osd));

        pstVgsAddOsd->rect.width = pstOsdPrm->stVgsOsdPrm[i].u32Width;
        pstVgsAddOsd->rect.height = pstOsdPrm->stVgsOsdPrm[i].u32Height;
        pstVgsAddOsd->rect.x = pstOsdPrm->stVgsOsdPrm[i].s32X;
        pstVgsAddOsd->rect.y = pstOsdPrm->stVgsOsdPrm[i].s32Y;
        pstVgsAddOsd->stride = pstOsdPrm->stVgsOsdPrm[i].u32Stride;

        pstVgsAddOsd->phys_addr = pstOsdPrm->stVgsOsdPrm[i].u64PhyAddr;
        switch(pstOsdPrm->stVgsOsdPrm[i].enPixelFmt)
        {
            case SAL_VIDEO_DATFMT_ARGB16_1555:
                pstVgsAddOsd->pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
                break;
            case SAL_VIDEO_DATFMT_RGBA_8888:
                pstVgsAddOsd->pixel_format = OT_PIXEL_FORMAT_ARGB_8888;
                break;
            default:
                SAL_LOGE("can't find osd enPixelFmt = %d \n", pstOsdPrm->stVgsOsdPrm[i].enPixelFmt);
                pstVgsAddOsd->pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
                break;
                
        }
        pstVgsAddOsd->bg_color = pstOsdPrm->stVgsOsdPrm[i].u32BgColor;
        pstVgsAddOsd->bg_alpha = pstOsdPrm->stVgsOsdPrm[i].u32BgAlpha;
        pstVgsAddOsd->fg_alpha = pstOsdPrm->stVgsOsdPrm[i].u32FgAlpha;
        pstVgsAddOsd->osd_inverted_color = OT_VGS_OSD_INVERTED_COLOR_NONE;       /* 暂时不支持osd反色 */
    }

    uiOsdNum = pstOsdPrm->uiOsdNum;
    s32Ret = ss_mpi_vgs_add_osd_task(stHandle, &stTask, astVgsAddOsd, uiOsdNum);
    if (s32Ret != TD_SUCCESS)
    {
        ss_mpi_vgs_cancel_job(stHandle);
        SAL_LOGE("2222 ret %#x %d.\n", s32Ret, uiOsdNum);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vgs_end_job(stHandle);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGE("3333 ret %#x \n", s32Ret);
        ss_mpi_vgs_cancel_job(stHandle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   vgs_ssv5DrawOsdSingle
 * @brief      使用 VGS 画osd
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrame  数据帧
 * @param[in]  VGS_ADD_OSD_PRM *pstOsdPrm         画osd的信息参数
 * @param[out] None
 * @return     INT32
 */
INT32 vgs_ssv5DrawOsdSingle(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    INT32 s32Ret = 0;
    ot_vgs_task_attr stTask = {0};
    ot_vgs_handle stHandle = 0;
    ot_vgs_osd astVgsAddOsd = {0, };

    if ( NULL == pstOsdPrm)
    {
        SAL_LOGE("input is NULL %p, %p\n", pstSystemFrame, pstOsdPrm);

        return SAL_FAIL;
    }
    memcpy(&stTask.img_in, (ot_video_frame_info *)pstSystemFrame->uiAppData, sizeof(ot_video_frame_info)); /* 输入图像 */
    memcpy(&stTask.img_out, (ot_video_frame_info *)pstSystemFrame->uiAppData, sizeof(ot_video_frame_info)); /* osd画线后输出图像 */

    s32Ret = ss_mpi_vgs_begin_job(&stHandle);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGE("1111 ret %#x \n", s32Ret);

        return SAL_FAIL;
    }

    if (0 == pstOsdPrm->stVgsOsdPrm[0].u32Width
        || 0 == pstOsdPrm->stVgsOsdPrm[0].u32Height)
    {
        ss_mpi_vgs_cancel_job(stHandle);
        return SAL_FAIL;
    }

    astVgsAddOsd.rect.width = pstOsdPrm->stVgsOsdPrm[0].u32Width;
    astVgsAddOsd.rect.height = pstOsdPrm->stVgsOsdPrm[0].u32Height;
    astVgsAddOsd.rect.x = pstOsdPrm->stVgsOsdPrm[0].s32X;
    astVgsAddOsd.rect.y = pstOsdPrm->stVgsOsdPrm[0].s32Y;
    astVgsAddOsd.stride = pstOsdPrm->stVgsOsdPrm[0].u32Stride;

    astVgsAddOsd.phys_addr = pstOsdPrm->stVgsOsdPrm[0].u64PhyAddr;
    astVgsAddOsd.pixel_format = pstOsdPrm->stVgsOsdPrm[0].enPixelFmt;

    astVgsAddOsd.bg_color = pstOsdPrm->stVgsOsdPrm[0].u32BgColor;
    astVgsAddOsd.bg_alpha = pstOsdPrm->stVgsOsdPrm[0].u32BgAlpha;
    astVgsAddOsd.fg_alpha = pstOsdPrm->stVgsOsdPrm[0].u32FgAlpha;
    astVgsAddOsd.osd_inverted_color = OT_VGS_OSD_INVERTED_COLOR_NONE;       /* 暂时不支持osd反色 */

    s32Ret = ss_mpi_vgs_add_osd_task(stHandle, &stTask, &astVgsAddOsd, pstOsdPrm->uiOsdNum); 
    {
        ss_mpi_vgs_cancel_job(stHandle);
        SAL_LOGE("2222 ret %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vgs_end_job(stHandle);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGE("3333 ret %#x \n", s32Ret);
        ss_mpi_vgs_cancel_job(stHandle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   vgs_ssv5DrawLineOSDArray
 * @brief      使用 VGS 画osd和框
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrame 数据帧
 * @param[in]  VGS_ADD_OSD_PRM *pstOsdPrm        画osd的信息参数 
 * @param[in]  VGS_DRAW_LINE_PRM *pstLinePrm     画线参数
 * @param[out] None
 * @return     INT32
 */
INT32 vgs_ssv5DrawLineOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm, VGS_DRAW_LINE_PRM *pstLinePrm)
{
    INT32 s32Ret = 0;
    UINT32 i = 0;
    UINT32 u32OsdLeftNum = 0;
    UINT32 u32OsdTmp     = 0;
    UINT32 u32TaskOsdNum = 0;
    UINT32 uiLineNum = 0;
    ot_vgs_handle stHandle = 0;
    ot_vgs_osd *pstVgsAddOsd = NULL;
    ot_vgs_osd astVgsAddOsd[MAX_VGS_OSD_NUM] = {0};
    VGS_OSD_PRM *pstOsd = NULL;
    UINT32 freeCnt = 0;         /* 剩余未画线 */
    UINT32 drawCnt = 0;         /* 已划线 */
    ot_vgs_task_attr stTask = {0};

    memcpy(&stTask.img_in, (ot_video_frame_info *)pstSystemFrame->uiAppData, sizeof(ot_video_frame_info)); /* 输入图像 */
    memcpy(&stTask.img_out, (ot_video_frame_info *)pstSystemFrame->uiAppData, sizeof(ot_video_frame_info)); /* osd画线后输出图像 */

    s32Ret = ss_mpi_vgs_begin_job(&stHandle);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGE("1111 ret %#x \n", s32Ret);

        return SAL_FAIL;
    }

    if (NULL == pstLinePrm)
    {
        goto osd;
    }

    if (pstLinePrm->uiLineNum >= MAX_VGS_LINE_NUM)
    {
        pstLinePrm->uiLineNum = MAX_VGS_LINE_NUM;
    }

    uiLineNum = pstLinePrm->uiLineNum;
    freeCnt = uiLineNum;
    if (uiLineNum > 0)
    {
        do
        {
            if (freeCnt > 100)
            {
                s32Ret = ss_mpi_vgs_add_draw_line_task(stHandle, &stTask, (ot_vgs_line *)&pstLinePrm->stVgsLinePrm[drawCnt], 100);
                if (s32Ret != TD_SUCCESS)
                {
                    ss_mpi_vgs_cancel_job(stHandle);
                    return SAL_FAIL;
                }

                freeCnt -= 100;
                drawCnt += 100;
            }
            else
            {
                s32Ret = ss_mpi_vgs_add_draw_line_task(stHandle, &stTask, (ot_vgs_line *)&pstLinePrm->stVgsLinePrm[drawCnt], freeCnt);
                if (s32Ret != TD_SUCCESS)
                {
                    ss_mpi_vgs_cancel_job(stHandle);
                    return SAL_FAIL;
                }

                freeCnt = 0;
            }
        }
        while (freeCnt > 0);
    }

osd:
    if (NULL == pstOsdPrm)
    {
        ss_mpi_vgs_cancel_job(stHandle);
        return SAL_SOK;
    }

    u32OsdLeftNum = pstOsdPrm->uiOsdNum;
    pstOsd = pstOsdPrm->stVgsOsdPrm;
    while (u32OsdLeftNum > 0)
    {
        u32OsdTmp = u32OsdLeftNum > 100 ? 100 : u32OsdLeftNum;
        u32OsdLeftNum -= u32OsdTmp;
        u32TaskOsdNum = 0;
        for (i = 0; i < u32OsdTmp; i++, pstOsd++)
        {
            if ((0 == pstOsd->u64PhyAddr) || (0 == pstOsd->u32Width) || (0 == pstOsd->u32Height))
            {
                SAL_LOGW("invalid param:addr[0x%llx] width[%u] height[%u]\n",pstOsd->u64PhyAddr,pstOsd->u32Width,pstOsd->u32Height);
                continue;
            }

            pstVgsAddOsd = astVgsAddOsd + u32TaskOsdNum++;
            pstVgsAddOsd->rect.width  = SAL_align(pstOsd->u32Width, 2);
            pstVgsAddOsd->rect.height = SAL_align(pstOsd->u32Height, 2);
            pstVgsAddOsd->rect.x = SAL_align(pstOsd->s32X, 2);
            pstVgsAddOsd->rect.y = SAL_align(pstOsd->s32Y, 2);
            pstVgsAddOsd->stride   = pstOsd->u32Stride;
            pstVgsAddOsd->phys_addr = pstOsd->u64PhyAddr;
            switch(pstOsd->enPixelFmt)
            {
                case SAL_VIDEO_DATFMT_ARGB16_1555:
                    pstVgsAddOsd->pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
                    break;
                case SAL_VIDEO_DATFMT_RGBA_8888:
                    pstVgsAddOsd->pixel_format = OT_PIXEL_FORMAT_ARGB_8888;
                    break;
                default:
                    pstVgsAddOsd->pixel_format = OT_PIXEL_FORMAT_ARGB_1555;
                    break;
            }
            pstVgsAddOsd->bg_color = pstOsd->u32BgColor;
            pstVgsAddOsd->bg_alpha = pstOsd->u32BgAlpha;
            pstVgsAddOsd->fg_alpha = pstOsd->u32FgAlpha;
            pstVgsAddOsd->osd_inverted_color = OT_VGS_OSD_INVERTED_COLOR_NONE;       /* 暂时不支持osd反色 */
        }

        s32Ret = ss_mpi_vgs_add_osd_task(stHandle, &stTask, astVgsAddOsd, u32TaskOsdNum);
        if (s32Ret != TD_SUCCESS)
        {
            ss_mpi_vgs_cancel_job(stHandle);
            SAL_LOGE("draw osd fail %#x %d.\n", s32Ret, u32TaskOsdNum);
            for (i = 0; i < pstOsdPrm->uiOsdNum; i++)
            {
                SAL_LOGW("x %d y %d w %d h %d\n", pstOsdPrm->stVgsOsdPrm[i].s32X, pstOsdPrm->stVgsOsdPrm[i].s32Y,
                         pstOsdPrm->stVgsOsdPrm[i].u32Width, pstOsdPrm->stVgsOsdPrm[i].u32Height);
            }

            return SAL_FAIL;
        }
    }

    s32Ret = ss_mpi_vgs_end_job(stHandle);
    if (s32Ret != TD_SUCCESS)
    {
        SAL_LOGE("vgs end job[%d] fail ret %#x\n", stHandle, s32Ret);
        ss_mpi_vgs_cancel_job(stHandle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   vgs_halRegister
 * @brief      注册hisi disp显示函数
 * @param[in]  void  
 * @param[out] None
 * @return     VGS_PLAT_OPS *
 */
VGS_PLAT_OPS *vgs_halRegister(void)
{
    memset(&g_stVgsPlatOps, 0,sizeof(VGS_PLAT_OPS));

    g_stVgsPlatOps.drawLineOSDArray = vgs_ssv5DrawLineOSDArray;
    g_stVgsPlatOps.drawOsdSingle = vgs_ssv5DrawOsdSingle;
    g_stVgsPlatOps.drawOSDArray = vgs_ssv5DrawOSDArray;
    g_stVgsPlatOps.drawLineArray = vgs_ssv5DrawLineArray;
    g_stVgsPlatOps.scaleFrame = vgs_ssv5ScaleFrame;

    return &g_stVgsPlatOps;
}



