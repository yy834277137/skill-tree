/*******************************************************************************
* vgs_hal.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2018年03月13日 Create
*
* Description : 硬件平台 vgs 模块
* Modification:
*******************************************************************************/
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



/*****************************************************************************
   函 数 名  : vgs_hisiDrawLineArray
   功能描述  : 使用 VGS 画多条线
   输入参数  : stTask vgs画线任务参数
                             pstLinePrm 画线的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hisiDrawLineArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_DRAW_LINE_PRM *pstLinePrm)
{
    INT32 s32Ret = 0;
    UINT32 uiLineNum = 0;
    UINT32 freeCnt = 0; /* 剩余未画线 */
    UINT32 drawCnt = 0; /* 已划线 */
    VGS_TASK_ATTR_S stTask = {0};

    /* UINT32 i = 0; */
    /* UINT64 time1 = 0, time2 = 0, time3 = 0,time4 = 0,time5 = 0,time6 = 0; */
    VGS_HANDLE stHandle = 0;

    if ((NULL == pstSystemFrame) || (NULL == pstLinePrm))
    {
        SAL_LOGE("input is NULL %p,%p\n", pstSystemFrame, pstLinePrm);

        return SAL_FAIL;
    }

    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* 输入图像 */
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* osd画线后输出图像 */

    s32Ret = HI_MPI_VGS_BeginJob(&stHandle);
    if (s32Ret != HI_SUCCESS)
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
                s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(stHandle, &stTask, (VGS_DRAW_LINE_S *)&pstLinePrm->stVgsLinePrm[drawCnt], 100);
                if (s32Ret != HI_SUCCESS)
                {
                    HI_MPI_VGS_CancelJob(stHandle);
                    return SAL_FAIL;
                }

                freeCnt -= 100;
                drawCnt += 100;
            }
            else
            {
                s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(stHandle, &stTask, (VGS_DRAW_LINE_S *)&pstLinePrm->stVgsLinePrm[drawCnt], freeCnt);
                if (s32Ret != HI_SUCCESS)
                {
                    HI_MPI_VGS_CancelJob(stHandle);
                    return SAL_FAIL;
                }

                freeCnt = 0;
            }
        }
        while (freeCnt > 0);
    }

    s32Ret = HI_MPI_VGS_EndJob(stHandle);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_LOGE("3333 ret %#x \n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : vgs_hisiScaleFrame
   功能描述  : 使用 VGS 缩放
   输入参数  : stTask vgs缩放任务参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hisiScaleFrame(SYSTEM_FRAME_INFO *pDstSystemFrame, SYSTEM_FRAME_INFO *pSrcSystemFrame, UINT32 dstW, UINT32 dstH)
{
    INT32 s32Ret = SAL_SOK;
    VGS_HANDLE stHandle = 0;
    VIDEO_FRAME_INFO_S *pDstVideo = NULL;
    VIDEO_FRAME_INFO_S *pSrcVideo = NULL;
    VGS_TASK_ATTR_S stTask = {0};


    pDstVideo = (VIDEO_FRAME_INFO_S *)pDstSystemFrame->uiAppData;
    pSrcVideo = (VIDEO_FRAME_INFO_S *)pSrcSystemFrame->uiAppData;
    memcpy(&stTask.stImgIn, pSrcVideo, sizeof(VIDEO_FRAME_INFO_S));
    pDstVideo->stVFrame.u32Width = dstW;
    pDstVideo->stVFrame.u32Height = dstH;
    memcpy(&stTask.stImgOut, pDstVideo, sizeof(VIDEO_FRAME_INFO_S));


    s32Ret = HI_MPI_VGS_BeginJob(&stHandle);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_LOGE("HI_MPI_VGS_BeginJob ret %#x \n", s32Ret);

        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VGS_AddScaleTask(stHandle, &stTask, VGS_SCLCOEF_NORMAL);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_LOGW("HI_MPI_VGS_AddScaleTask %#x \n", s32Ret);
        HI_MPI_VGS_CancelJob(stHandle);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VGS_EndJob(stHandle);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_LOGE("HI_MPI_VGS_EndJob ret %#x \n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}



/*****************************************************************************
   函 数 名  : vgs_hisiDrawOSDArray
   功能描述  : 使用 VGS 画osd
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hisiDrawOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    INT32 s32Ret = 0;
    UINT32 uiOsdNum = 0;
    UINT32 i = 0;
    VGS_TASK_ATTR_S stTask = {0};

    VGS_HANDLE stHandle = 0;
    VGS_ADD_OSD_S *pstVgsAddOsd = NULL;
    VGS_ADD_OSD_S astVgsAddOsd[MAX_VGS_OSD_NUM];

    if ( NULL == pstOsdPrm)
    {
        SAL_LOGE("input is NULL %p,%p\n", pstSystemFrame, pstOsdPrm);

        return SAL_FAIL;
    }
    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* 输入图像 */
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* osd画线后输出图像 */

    s32Ret = HI_MPI_VGS_BeginJob(&stHandle);
    if (s32Ret != HI_SUCCESS)
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

        memset(pstVgsAddOsd, 0x00, sizeof(VGS_ADD_OSD_S));

        pstVgsAddOsd->stRect.u32Width = pstOsdPrm->stVgsOsdPrm[i].u32Width;
        pstVgsAddOsd->stRect.u32Height = pstOsdPrm->stVgsOsdPrm[i].u32Height;
        pstVgsAddOsd->stRect.s32X = pstOsdPrm->stVgsOsdPrm[i].s32X;
        pstVgsAddOsd->stRect.s32Y = pstOsdPrm->stVgsOsdPrm[i].s32Y;
        pstVgsAddOsd->u32Stride = pstOsdPrm->stVgsOsdPrm[i].u32Stride;

        pstVgsAddOsd->u64PhyAddr = pstOsdPrm->stVgsOsdPrm[i].u64PhyAddr;
        switch(pstOsdPrm->stVgsOsdPrm[i].enPixelFmt)
        {
            case SAL_VIDEO_DATFMT_ARGB16_1555:
                pstVgsAddOsd->enPixelFmt = PIXEL_FORMAT_ARGB_1555;
                break;
            case SAL_VIDEO_DATFMT_RGBA_8888:
                pstVgsAddOsd->enPixelFmt = PIXEL_FORMAT_ARGB_8888;
                break;
            default:
                SAL_LOGE("can't find osd enPixelFmt = %d \n", pstOsdPrm->stVgsOsdPrm[i].enPixelFmt);
                pstVgsAddOsd->enPixelFmt = PIXEL_FORMAT_ARGB_1555;
                break;
                
        }
        pstVgsAddOsd->u32BgColor = pstOsdPrm->stVgsOsdPrm[i].u32BgColor;
        pstVgsAddOsd->u32BgAlpha = pstOsdPrm->stVgsOsdPrm[i].u32BgAlpha;
        pstVgsAddOsd->u32FgAlpha = pstOsdPrm->stVgsOsdPrm[i].u32FgAlpha;
        pstVgsAddOsd->bOsdRevert = 0;       /* 暂时不支持osd反色 */
    }

    uiOsdNum = pstOsdPrm->uiOsdNum;
    s32Ret = HI_MPI_VGS_AddOsdTaskArray(stHandle, &stTask, astVgsAddOsd, uiOsdNum);
    if (s32Ret != HI_SUCCESS)
    {
        HI_MPI_VGS_CancelJob(stHandle);
        SAL_LOGE("2222 ret %#x %d.\n", s32Ret, uiOsdNum);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VGS_EndJob(stHandle);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_LOGE("3333 ret %#x \n", s32Ret);
        HI_MPI_VGS_CancelJob(stHandle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : vgs_hisiDrawOsdSingle
   功能描述  : 使用 VGS 画osd
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hisiDrawOsdSingle(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    INT32 s32Ret = 0;
    VGS_TASK_ATTR_S stTask = {0};
    VGS_HANDLE stHandle = 0;
    VGS_ADD_OSD_S astVgsAddOsd = {0, };

    if ( NULL == pstOsdPrm)
    {
        SAL_LOGE("input is NULL %p, %p\n", pstSystemFrame, pstOsdPrm);

        return SAL_FAIL;
    }
    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* 输入图像 */
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* osd画线后输出图像 */

    s32Ret = HI_MPI_VGS_BeginJob(&stHandle);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_LOGE("1111 ret %#x \n", s32Ret);

        return SAL_FAIL;
    }

    if (0 == pstOsdPrm->stVgsOsdPrm[0].u32Width
        || 0 == pstOsdPrm->stVgsOsdPrm[0].u32Height)
    {
        return SAL_FAIL;
    }

    astVgsAddOsd.stRect.u32Width = pstOsdPrm->stVgsOsdPrm[0].u32Width;
    astVgsAddOsd.stRect.u32Height = pstOsdPrm->stVgsOsdPrm[0].u32Height;
    astVgsAddOsd.stRect.s32X = pstOsdPrm->stVgsOsdPrm[0].s32X;
    astVgsAddOsd.stRect.s32Y = pstOsdPrm->stVgsOsdPrm[0].s32Y;
    astVgsAddOsd.u32Stride = pstOsdPrm->stVgsOsdPrm[0].u32Stride;

    astVgsAddOsd.u64PhyAddr = pstOsdPrm->stVgsOsdPrm[0].u64PhyAddr;
    astVgsAddOsd.enPixelFmt = pstOsdPrm->stVgsOsdPrm[0].enPixelFmt;

    astVgsAddOsd.u32BgColor = pstOsdPrm->stVgsOsdPrm[0].u32BgColor;
    astVgsAddOsd.u32BgAlpha = pstOsdPrm->stVgsOsdPrm[0].u32BgAlpha;
    astVgsAddOsd.u32FgAlpha = pstOsdPrm->stVgsOsdPrm[0].u32FgAlpha;
    astVgsAddOsd.bOsdRevert = 0;       /* 暂时不支持osd反色 */

    s32Ret = HI_MPI_VGS_AddOsdTask(stHandle, &stTask, &astVgsAddOsd);
    if (s32Ret != HI_SUCCESS)
    {
        HI_MPI_VGS_CancelJob(stHandle);
        SAL_LOGE("2222 ret %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VGS_EndJob(stHandle);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_LOGE("3333 ret %#x \n", s32Ret);
        HI_MPI_VGS_CancelJob(stHandle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*****************************************************************************
   函 数 名  : vgs_hisiDrawLineOSDArray
   功能描述  : 使用 VGS 画osd和框
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
                             pstLinePrm画框参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_hisiDrawLineOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm, VGS_DRAW_LINE_PRM *pstLinePrm)
{
    INT32 s32Ret = 0;
    UINT32 i = 0;
    UINT32 u32OsdLeftNum = 0;
    UINT32 u32OsdTmp     = 0;
    UINT32 u32TaskOsdNum = 0;
    UINT32 uiLineNum = 0;
    VGS_HANDLE stHandle = 0;
    VGS_ADD_OSD_S *pstVgsAddOsd = NULL;
    VGS_ADD_OSD_S astVgsAddOsd[MAX_VGS_OSD_NUM] = {0};
    VGS_OSD_PRM *pstOsd = NULL;
    UINT32 freeCnt = 0;         /* 剩余未画线 */
    UINT32 drawCnt = 0;         /* 已划线 */
    VGS_TASK_ATTR_S stTask = {0};

    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* 输入图像 */
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* osd画线后输出图像 */

    s32Ret = HI_MPI_VGS_BeginJob(&stHandle);
    if (s32Ret != HI_SUCCESS)
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
                s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(stHandle, &stTask, (VGS_DRAW_LINE_S *)&pstLinePrm->stVgsLinePrm[drawCnt], 100);
                if (s32Ret != HI_SUCCESS)
                {
                    HI_MPI_VGS_CancelJob(stHandle);
                    return SAL_FAIL;
                }

                freeCnt -= 100;
                drawCnt += 100;
            }
            else
            {
                s32Ret = HI_MPI_VGS_AddDrawLineTaskArray(stHandle, &stTask, (VGS_DRAW_LINE_S *)&pstLinePrm->stVgsLinePrm[drawCnt], freeCnt);
                if (s32Ret != HI_SUCCESS)
                {
                    HI_MPI_VGS_CancelJob(stHandle);
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
            pstVgsAddOsd->stRect.u32Width  = SAL_align(pstOsd->u32Width, 2);
            pstVgsAddOsd->stRect.u32Height = SAL_align(pstOsd->u32Height, 2);
            pstVgsAddOsd->stRect.s32X = SAL_align(pstOsd->s32X, 2);
            pstVgsAddOsd->stRect.s32Y = SAL_align(pstOsd->s32Y, 2);
            pstVgsAddOsd->u32Stride   = pstOsd->u32Stride;
            pstVgsAddOsd->u64PhyAddr = pstOsd->u64PhyAddr;
            switch(pstOsd->enPixelFmt)
            {
                case SAL_VIDEO_DATFMT_ARGB16_1555:
                    pstVgsAddOsd->enPixelFmt = PIXEL_FORMAT_ARGB_1555;
                    break;
                case SAL_VIDEO_DATFMT_RGBA_8888:
                    pstVgsAddOsd->enPixelFmt = PIXEL_FORMAT_ARGB_8888;
                    break;
                default:
                    pstVgsAddOsd->enPixelFmt = PIXEL_FORMAT_ARGB_1555;
                    break;
            }
            pstVgsAddOsd->u32BgColor = pstOsd->u32BgColor;
            pstVgsAddOsd->u32BgAlpha = pstOsd->u32BgAlpha;
            pstVgsAddOsd->u32FgAlpha = pstOsd->u32FgAlpha;
            pstVgsAddOsd->bOsdRevert = 0;       /* 暂时不支持osd反色 */
        }

        s32Ret = HI_MPI_VGS_AddOsdTaskArray(stHandle, &stTask, astVgsAddOsd, u32TaskOsdNum);
        if (s32Ret != HI_SUCCESS)
        {
            HI_MPI_VGS_CancelJob(stHandle);
            SAL_LOGE("draw osd fail %#x %d.\n", s32Ret, u32TaskOsdNum);
            for (i = 0; i < pstOsdPrm->uiOsdNum; i++)
            {
                SAL_LOGW("x %d y %d w %d h %d\n", pstOsdPrm->stVgsOsdPrm[i].s32X, pstOsdPrm->stVgsOsdPrm[i].s32Y,
                         pstOsdPrm->stVgsOsdPrm[i].u32Width, pstOsdPrm->stVgsOsdPrm[i].u32Height);
            }

            return SAL_FAIL;
        }
    }

    s32Ret = HI_MPI_VGS_EndJob(stHandle);
    if (s32Ret != HI_SUCCESS)
    {
        SAL_LOGD("vgs end job[%d] fail ret %#x\n", stHandle, s32Ret);
        HI_MPI_VGS_CancelJob(stHandle);
        return SAL_FAIL;
    }

    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : disp_halRegister
* 描  述  : 注册hisi disp显示函数
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
VGS_PLAT_OPS *vgs_halRegister(void)
{
    memset(&g_stVgsPlatOps, 0,sizeof(VGS_PLAT_OPS));

    g_stVgsPlatOps.drawLineOSDArray = vgs_hisiDrawLineOSDArray;
    g_stVgsPlatOps.drawOsdSingle = vgs_hisiDrawOsdSingle;
    g_stVgsPlatOps.drawOSDArray = vgs_hisiDrawOSDArray;
    g_stVgsPlatOps.drawLineArray = vgs_hisiDrawLineArray;
    g_stVgsPlatOps.scaleFrame = vgs_hisiScaleFrame;

    return &g_stVgsPlatOps;
}



