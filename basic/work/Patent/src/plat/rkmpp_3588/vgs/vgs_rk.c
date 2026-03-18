/*******************************************************************************
* vgs_rk.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com.cn>
* Version: V1.0.0  2022年03月08日 Create
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
#define MAX_RKVGS_OSD_NUM	(100)


/*****************************************************************************
   函 数 名  : vgs_rk_DrawLineArray
   功能描述  : 使用 VGS 画多条线
   输入参数  : stTask vgs画线任务参数
               pstLinePrm 画线的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_rk_DrawLineArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_DRAW_LINE_PRM *pstLinePrm)
{
    INT32 s32Ret = SAL_FAIL;
	
    VGS_HANDLE hHandle = -1;
	
    UINT32 uiLineNum = 0;
    UINT32 freeCnt = 0; /* 剩余未画线 */
    UINT32 drawCnt = 0; /* 已划线 */
	
    VGS_TASK_ATTR_S stTask = {0};

    if ((NULL == pstSystemFrame) || (NULL == pstLinePrm))
    {
        SAL_LOGE("input is NULL %p,%p\n", pstSystemFrame, pstLinePrm);
		goto exit;
    }

    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S));

    s32Ret = RK_MPI_VGS_BeginJob(&hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_BeginJob failed! ret: 0x%x \n", s32Ret);
		goto exit;
    }

    if (pstLinePrm->uiLineNum >= MAX_VGS_LINE_NUM)
    {
        pstLinePrm->uiLineNum = MAX_VGS_LINE_NUM;
    }

    uiLineNum = pstLinePrm->uiLineNum;
    freeCnt = uiLineNum;

	/* fixme: 
	   单个job处理多于100条线的task时分多次调用addArray进行处理，与底层硬件资源的单job最多200task(rk3588_V1.0.0)是否存在冲突
	*/
    if (uiLineNum > 0)
    {
        do
        {
            if (freeCnt > 100)
            {
                s32Ret = RK_MPI_VGS_AddDrawLineTaskArray(hHandle, &stTask, (VGS_DRAW_LINE_S *)&pstLinePrm->stVgsLinePrm[drawCnt], 100);
				if (RK_SUCCESS != s32Ret)
                {
					SAL_LOGE("RK_MPI_VGS_AddDrawLineTaskArray 100 failed! ret: 0x%x \n", s32Ret);
                    goto cancel;
                }

                freeCnt -= 100;
                drawCnt += 100;
            }
            else
            {
                s32Ret = RK_MPI_VGS_AddDrawLineTaskArray(hHandle, &stTask, (VGS_DRAW_LINE_S *)&pstLinePrm->stVgsLinePrm[drawCnt], freeCnt);
				if (RK_SUCCESS != s32Ret)
                {
					SAL_LOGE("RK_MPI_VGS_AddDrawLineTaskArray last failed! ret: 0x%x \n", s32Ret);
					goto cancel;
                }

                freeCnt = 0;
            }
        }
        while (freeCnt > 0);
    }

    s32Ret = RK_MPI_VGS_EndJob(hHandle);
    if (RK_SUCCESS != s32Ret)
    {
		SAL_LOGE("RK_MPI_VGS_EndJob failed! ret: 0x%x \n", s32Ret);
        goto cancel;
    }
	
	return SAL_SOK;
	
cancel:
    (VOID)RK_MPI_VGS_CancelJob(hHandle);

exit:
    return s32Ret;
}

/*****************************************************************************
   函 数 名  : vgs_rk_ScaleFrame
   功能描述  : 使用 VGS 缩放
   输入参数  : stTask vgs缩放任务参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_rk_ScaleFrame(SYSTEM_FRAME_INFO *pDstSystemFrame, SYSTEM_FRAME_INFO *pSrcSystemFrame, UINT32 dstW, UINT32 dstH)
{
    INT32 s32Ret = SAL_FAIL;
	
    VGS_HANDLE hHandle = -1;
	
    VIDEO_FRAME_INFO_S *pDstVideo = NULL;
    VIDEO_FRAME_INFO_S *pSrcVideo = NULL;
	
    VGS_TASK_ATTR_S stTask = {0};

	if (NULL == pDstSystemFrame
		|| NULL == pSrcSystemFrame)
	{
		SAL_LOGE("ptr null! %p %p \n", pDstSystemFrame, pSrcSystemFrame);
		goto exit;
	}
	
    pDstVideo = (VIDEO_FRAME_INFO_S *)pDstSystemFrame->uiAppData;
    pSrcVideo = (VIDEO_FRAME_INFO_S *)pSrcSystemFrame->uiAppData;
	
    memcpy(&stTask.stImgIn, pSrcVideo, sizeof(VIDEO_FRAME_INFO_S));
    pDstVideo->stVFrame.u32Width = dstW;
    pDstVideo->stVFrame.u32Height = dstH;
    memcpy(&stTask.stImgOut, pDstVideo, sizeof(VIDEO_FRAME_INFO_S));

    s32Ret = RK_MPI_VGS_BeginJob(&hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_BeginJob! ret: 0x%x \n", s32Ret);
		goto exit;
    }

    s32Ret = RK_MPI_VGS_AddScaleTask(hHandle, &stTask, VGS_SCLCOEF_NORMAL);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_AddScaleTask! ret: 0x%x \n", s32Ret);
        goto cancel;
    }

    s32Ret = RK_MPI_VGS_EndJob(hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_EndJob! ret: 0x%x \n", s32Ret);
        goto cancel;
    }
	
	return SAL_SOK;
	
cancel:
    (VOID)RK_MPI_VGS_CancelJob(hHandle);

exit:
    return s32Ret;
}

typedef struct 
{
    VOID *pVirAddr;
    UINT64 u64physAddr;
    MB_BLK pMbBlk;
	UINT32 size;
} VGS_OSD_MEM_S;

static VGS_OSD_MEM_S astOsdArrayMem[MAX_VGS_OSD_NUM];
static VGS_OSD_MEM_S astOsdSingleMem;
static Handle g_mutexOsdArrayMem;
static Handle g_mutexOsdSingleMem;


static VOID vgs_osdMemInit()
{
    ALLOC_VB_INFO_S stVbInfo = {0};
    INT32 i = 0;
    INT32 s32Ret = 0;
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &g_mutexOsdArrayMem);
    SAL_mutexCreate(SAL_MUTEX_NORMAL, &g_mutexOsdSingleMem);
    for (i = 0; i < MAX_RKVGS_OSD_NUM; i++)
    {
        if (!astOsdArrayMem[i].pVirAddr )
        {
            s32Ret = mem_hal_vbAlloc(64*64*20, "vgs_rk", "osdArray", NULL, SAL_FALSE, &stVbInfo);/*大小够使用*/
            if (SAL_SOK != s32Ret)
            {
                VDEC_LOGE("Alloc Vb Buf For vgs osd Failed! uiBlkSize %d\n", 64*64*20);
                return;
            }
            astOsdArrayMem[i].pMbBlk = (MB_BLK)stVbInfo.u64VbBlk;
            astOsdArrayMem[i].pVirAddr = stVbInfo.pVirAddr;
            astOsdArrayMem[i].u64physAddr = stVbInfo.u64PhysAddr;
			astOsdArrayMem[i].size = 64*64*20;
            memset(astOsdArrayMem[i].pVirAddr, 0, stVbInfo.u32Size);
        }

    }

    if (!astOsdSingleMem.pVirAddr)
    {
        s32Ret = mem_hal_vbAlloc(64*64*20, "vgs_rk", "osdArray", NULL, SAL_FALSE, &stVbInfo);
        if (SAL_SOK != s32Ret)
        {
            VDEC_LOGE("Alloc Vb Buf For vgs osd Failed! uiBlkSize %d\n", 64*64*20);
            return ;
        }
        
        astOsdSingleMem.pMbBlk = (MB_BLK)stVbInfo.u64VbBlk;
        astOsdSingleMem.pVirAddr = stVbInfo.pVirAddr;
        astOsdSingleMem.u64physAddr = stVbInfo.u64PhysAddr;
        memset(astOsdSingleMem.pVirAddr, 0, stVbInfo.u32Size);
    }

    return ;

}

/*****************************************************************************
   函 数 名  : vgs_rk_GetSPPixelByte
   功能描述  : 获取Single Plane单平面每个像素字节数
   输入参数  : PIXEL_FORMAT_E fmt 图像格式
   输出参数  : 无
   返 回 值  : 单平面每个像素字节数
*****************************************************************************/
INT32 vgs_rk_GetSPPixelByte(PIXEL_FORMAT_E fmt)
{
    switch (fmt)
    {
        case RK_FMT_YUV420P_VU:
            return 1;
        case RK_FMT_RGB888:
        case RK_FMT_BGR888:
            return 3;
        case RK_FMT_ARGB8888:
        case RK_FMT_ABGR8888:
        case RK_FMT_BGRA8888:
        case RK_FMT_RGBA8888:
            return 4;
        default :
            OSD_LOGE("PIXEL_FORMAT_E not support! fmt:%d\n",fmt);
            return 1;
    }
}

/*****************************************************************************
   函 数 名  : vgs_rk_DrawOSDArray
   功能描述  : 使用 VGS 画osd
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_rk_DrawOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    INT32 s32Ret = SAL_FAIL;
    INT32 bpp = 0;
    UINT32 i = 0, k = 0;
    UINT32 osdTaskNum = 0, uiOsdNum = 0, osdDrawnNum = 0; /* 一次提交画OSD源点阵数；该次提交中实际有效点阵数；已经执行（提交）画OSD点阵数 */
    UINT32 u32OsdXOffset = 0;
    UINT32 u32OsdYOffset = 0;

    VGS_HANDLE hHandle = -1;
    VGS_TASK_ATTR_S stTask = {0};
    VGS_ADD_OSD_S *pstVgsAddOsd = NULL;
    VGS_ADD_OSD_S astVgsAddOsd[MAX_RKVGS_OSD_NUM];
    VIDEO_FRAME_INFO_S *pVideoFrmInfo = NULL;

    if (NULL == pstSystemFrame || NULL == pstOsdPrm)
    {
        OSD_LOGE("input is NULL! %p,%p \n", pstSystemFrame, pstOsdPrm);
        return SAL_FAIL;
    }

    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* 输入图像 */
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* osd画线后输出图像 */

    /* rk使用pMbBlk作为图像数据叠OSD，需要计算需要叠OSD的图像和pMbBlk的偏移量，单位：像素 */
    pVideoFrmInfo = (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData;
    bpp = vgs_rk_GetSPPixelByte(pVideoFrmInfo->stVFrame.enPixelFormat);
    u32OsdXOffset = (RK_VOID *)pVideoFrmInfo->stVFrame.pVirAddr[0] - RK_MPI_MB_Handle2VirAddr(pVideoFrmInfo->stVFrame.pMbBlk);
    u32OsdXOffset = u32OsdXOffset / bpp;                                 /* 输入图像与MbBlk偏移字节转像素 */
    u32OsdYOffset = u32OsdXOffset / pVideoFrmInfo->stVFrame.u32VirWidth; /* 输入图像与MbBlk纵向偏移像素量 */
    u32OsdXOffset = u32OsdXOffset % pVideoFrmInfo->stVFrame.u32VirWidth; /* 输入图像与MbBlk横向偏移像素量 */

    /* 使用u32VirWidth计算pMbBlk的u32VirHeight */
    stTask.stImgIn.stVFrame.u32VirHeight = stTask.stImgOut.stVFrame.u32VirHeight = RK_MPI_MB_GetSize(pVideoFrmInfo->stVFrame.pMbBlk) / pVideoFrmInfo->stVFrame.u32VirWidth / bpp;

    /* 保护用于叠OSD的内存astOsdArrayMem只有一个线程在用，因为RK里叠OSD需要传入mb，上层的osd内存是所有的违禁品名字OSD内存一起申请的 */
    SAL_mutexLock(g_mutexOsdArrayMem);

    while (osdDrawnNum < pstOsdPrm->uiOsdNum)
    {
        /* 当画osd个数超过hal层限制MAX_VGS_OSD_NUM时，分次提交 */
        osdTaskNum = pstOsdPrm->uiOsdNum - osdDrawnNum > MAX_RKVGS_OSD_NUM ? MAX_RKVGS_OSD_NUM : pstOsdPrm->uiOsdNum - osdDrawnNum;
        uiOsdNum = 0;
        for (i = 0,k = osdDrawnNum; i < osdTaskNum; i++,k++)
        {
            /* 当该osd点阵跨度为0时，即点阵为空，忽略此OSD（防止跨度为0，RK底层报错） */
            if(!(pstOsdPrm->stVgsOsdPrm[k].u32Stride))
            {
                SAL_LOGW("pstOsdPrm->stVgsOsdPrm[%d].u32Stride = 0\n", k);
                continue;
            }

            pstVgsAddOsd = &astVgsAddOsd[uiOsdNum];/* 实际有效的OSD个数 */
            memset(pstVgsAddOsd, 0x00, sizeof(VGS_ADD_OSD_S));

            pstVgsAddOsd->stRect.u32Width = SAL_align(pstOsdPrm->stVgsOsdPrm[k].u32Width, 32); /*TODO：fixme，RK里VGS需要32字节对齐，这个对齐可能需要在上层做，还需进一步确认*/
            pstVgsAddOsd->stRect.u32Height = pstOsdPrm->stVgsOsdPrm[k].u32Height;
            pstVgsAddOsd->stRect.s32X = pstOsdPrm->stVgsOsdPrm[k].s32X + u32OsdXOffset;
            pstVgsAddOsd->stRect.s32Y = pstOsdPrm->stVgsOsdPrm[k].s32Y + u32OsdYOffset;

            memset(astOsdArrayMem[i].pVirAddr, 0, astOsdArrayMem[i].size);     /* 每次叠osd前，将整块内存置为0 */
            /*TODO: fixme, osd 宽度需要32对齐，这里拷贝的长度还需要进一步确认 */
            for (int j = 0; j < pstOsdPrm->stVgsOsdPrm[k].u32Height; j++)
            {
                
                memcpy(astOsdArrayMem[i].pVirAddr + j * pstOsdPrm->stVgsOsdPrm[k].u32Stride / pstOsdPrm->stVgsOsdPrm[k].u32Width * SAL_align(pstOsdPrm->stVgsOsdPrm[k].u32Width, 32),\
                    pstOsdPrm->stVgsOsdPrm[k].pVirAddr + j*pstOsdPrm->stVgsOsdPrm[k].u32Stride, pstOsdPrm->stVgsOsdPrm[k].u32Stride);
            }
            pstVgsAddOsd->pMbBlk = astOsdArrayMem[i].pMbBlk;
            pstVgsAddOsd->u32BgAlpha = pstOsdPrm->stVgsOsdPrm[k].u32BgAlpha;
            pstVgsAddOsd->u32FgAlpha = pstOsdPrm->stVgsOsdPrm[k].u32FgAlpha;

            switch(pstOsdPrm->stVgsOsdPrm[k].enPixelFmt)
            {
                case SAL_VIDEO_DATFMT_ARGB16_1555:
                {
                    pstVgsAddOsd->enPixelFmt = RK_FMT_BGRA5551;
                    break;
                }
                case SAL_VIDEO_DATFMT_RGBA_8888:
                {
                    pstVgsAddOsd->enPixelFmt = RK_FMT_ARGB8888;
                    break;
                }
                case SAL_VIDEO_DATFMT_YUV420SP_UV:
                {
                    pstVgsAddOsd->enPixelFmt = RK_FMT_YUV420SP;
                    break;
                }
                default:
                {
                    SAL_LOGE("can't find osd enPixelFmt = %d \n", pstOsdPrm->stVgsOsdPrm[k].enPixelFmt);
                    pstVgsAddOsd->enPixelFmt = RK_FMT_BGRA5551;
                    break;      
                }
            }
            uiOsdNum++;
        }

        if(uiOsdNum <= 0)//需要叠加的OSD数量需要大于0
        {
            goto exit;
        }

        s32Ret = RK_MPI_VGS_BeginJob(&hHandle);
        if (RK_SUCCESS != s32Ret)
        {
            OSD_LOGE("RK_MPI_VGS_BeginJob failed! ret: 0x%x \n", s32Ret);
            goto exit;
        }

        s32Ret = RK_MPI_VGS_AddOsdTaskArray(hHandle, &stTask, astVgsAddOsd, uiOsdNum);
        if (RK_SUCCESS != s32Ret)
        {
            OSD_LOGE("RK_MPI_VGS_AddOsdTaskArray failed! ret: 0x%x, %d.\n", s32Ret, uiOsdNum);
            goto cancel;
        }

        s32Ret = RK_MPI_VGS_EndJob(hHandle);
        if (RK_SUCCESS != s32Ret)
        {
            OSD_LOGE("RK_MPI_VGS_EndJob failed! ret: 0x%x \n", s32Ret);
            goto cancel;
        }
        osdDrawnNum += osdTaskNum;
    }

    SAL_mutexUnlock(g_mutexOsdArrayMem);
    return SAL_SOK;

cancel:
    (VOID)RK_MPI_VGS_CancelJob(hHandle);

exit:
    SAL_LOGE("drawOSD state: totall:%d finish:%d drawing:%d/%d\n", pstOsdPrm->uiOsdNum, osdDrawnNum, i, osdTaskNum);
    SAL_mutexUnlock(g_mutexOsdArrayMem);
    return s32Ret;
}
/*****************************************************************************
   函 数 名  : vgs_rk_DrawOsdSingle
   功能描述  : 使用 VGS 画osd
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_rk_DrawOsdSingle(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    INT32 s32Ret = SAL_FAIL;

    VGS_HANDLE hHandle = -1;

    VGS_TASK_ATTR_S stTask = {0};
    VGS_ADD_OSD_S stVgsAddOsd = {0};

    if (NULL == pstSystemFrame
		|| NULL == pstOsdPrm
		|| 1 != pstOsdPrm->uiOsdNum
		|| 0 == pstOsdPrm->stVgsOsdPrm[0].u32Width
        || 0 == pstOsdPrm->stVgsOsdPrm[0].u32Height)
    {
        SAL_LOGE("input err! %p, %p, %d, %d, %d \n", 
			      pstSystemFrame, 
			      pstOsdPrm, 
			      (NULL != pstOsdPrm) ? pstOsdPrm->uiOsdNum : 0, 
			      (NULL != pstOsdPrm) ? pstOsdPrm->stVgsOsdPrm[0].u32Width : 0, 
			      (NULL != pstOsdPrm) ? pstOsdPrm->stVgsOsdPrm[0].u32Height : 0);
        return SAL_FAIL;
    }
	
    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* 输入图像 */
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S)); /* osd画线后输出图像 */

    SAL_mutexLock(g_mutexOsdSingleMem);

    s32Ret = RK_MPI_VGS_BeginJob(&hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_BeginJob failed! ret: 0x%x \n", s32Ret);
        goto exit;
    }

    /*TODO：fixme，RK里VGS需要32字节对齐，这个对齐可能需要在上层做，还需进一步确认*/
    stVgsAddOsd.stRect.u32Width = pstOsdPrm->stVgsOsdPrm[0].u32Width;
    stVgsAddOsd.stRect.u32Height = pstOsdPrm->stVgsOsdPrm[0].u32Height;
    stVgsAddOsd.stRect.s32X = pstOsdPrm->stVgsOsdPrm[0].s32X;
    stVgsAddOsd.stRect.s32Y = pstOsdPrm->stVgsOsdPrm[0].s32Y;

    /*TODO: fixme, osd 宽度需要32对齐，这里拷贝的长度还需要进一步确认 */
    //memcpy(astOsdSingleMem.pVirAddr, pstOsdPrm->stVgsOsdPrm[0].pVirAddr, pstOsdPrm->stVgsOsdPrm[0].u32Stride * pstOsdPrm->stVgsOsdPrm[0].u32Height);
    stVgsAddOsd.pMbBlk = astOsdSingleMem.pMbBlk;
    stVgsAddOsd.u32BgAlpha = pstOsdPrm->stVgsOsdPrm[0].u32BgAlpha;
    stVgsAddOsd.u32FgAlpha = pstOsdPrm->stVgsOsdPrm[0].u32FgAlpha;

	switch(pstOsdPrm->stVgsOsdPrm[0].enPixelFmt)
	{
		case SAL_VIDEO_DATFMT_ARGB16_1555:
		{
			stVgsAddOsd.enPixelFmt = RK_FMT_ARGB1555;
			break;
		}
		case SAL_VIDEO_DATFMT_RGBA_8888:
		{
			stVgsAddOsd.enPixelFmt = RK_FMT_ARGB8888;
			break;
		}
		default:
		{
			SAL_LOGE("can't find osd enPixelFmt = %d \n", pstOsdPrm->stVgsOsdPrm[0].enPixelFmt);
			stVgsAddOsd.enPixelFmt = RK_FMT_ARGB1555;
			break;		
		}
	}

    s32Ret = RK_MPI_VGS_AddOsdTask(hHandle, &stTask, &stVgsAddOsd);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_AddOsdTask failed! ret: 0x%x\n", s32Ret);
	    goto cancel;
    }

    s32Ret = RK_MPI_VGS_EndJob(hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_EndJob failed! ret: 0x%x \n", s32Ret);
        goto cancel;
    }

    SAL_mutexUnlock(g_mutexOsdSingleMem);
	return SAL_SOK;
	
cancel:
    (VOID)RK_MPI_VGS_CancelJob(hHandle);	

exit:
    SAL_mutexUnlock(g_mutexOsdSingleMem);
    return s32Ret;
}

/*****************************************************************************
   函 数 名  : vgs_rk_DrawLineOSDArray
   功能描述  : 使用 VGS 画osd和框
   输入参数  : stTask vgs画osd任务参数
                             pstOsdPrm 画osd的信息参数
                             pstLinePrm画框参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
                         失败SAL_FAIL
*****************************************************************************/
INT32 vgs_rk_DrawLineOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm, VGS_DRAW_LINE_PRM *pstLinePrm)
{
    INT32 s32Ret = SAL_SOK;

	if (NULL == pstSystemFrame)
	{
		SAL_LOGE("ptr null! %p %p %p \n", pstSystemFrame, pstOsdPrm, pstLinePrm);
		goto exit;
	}

	/* fixme: 此处将画线和画osd分成两个job进行处理，效率上与在同一个job需要进行对比测试 */
    if (NULL != pstLinePrm)
    {
        s32Ret = vgs_rk_DrawLineArray(pstSystemFrame, pstLinePrm);
		if (SAL_SOK != s32Ret)
		{
			// no need return
			SAL_LOGE(
"vgs_hisiDrawLineArray failed! \n");
		}
    }

	if (NULL != pstOsdPrm)
	{
	    s32Ret = vgs_rk_DrawOSDArray(pstSystemFrame, pstOsdPrm);
		if (SAL_SOK != s32Ret)
		{
			// no need return
			SAL_LOGE(
"vgs_hisiDrawLineArray failed! \n");
		}
	}

exit:
    return s32Ret;
}

/*****************************************************************************
* 函 数 名  : vgs_rk_DrawCoverArray
* 功能描述  : 使用 VGS 画 cover
* 输入参数  : SYSTEM_FRAME_INFO *pstSystemFrame
              VGS_ADD_COVER_PRM_S *pstCoverPrm
* 输出参数  : 无
* 返 回 值  : 成功SAL_SOK
*             失败SAL_FAIL
*****************************************************************************/
INT32 ATTRIBUTE_UNUSED vgs_rk_DrawCoverArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_COVER_PRM_S *pstCoverPrm)
{
    INT32 s32Ret = SAL_FAIL;

    VGS_HANDLE hHandle = -1;
	
    UINT32 i = 0;
	UINT32 u32CoverNum = 0;
	
    VGS_TASK_ATTR_S stTask = {0};

    VGS_ADD_COVER_S *pstVgsSingleCoverPrm = NULL;
    VGS_ADD_COVER_S astVgsAddCoverArray[MAX_VGS_COVER_NUM];

    if (NULL == pstSystemFrame
		|| NULL == pstCoverPrm)
    {
        SAL_LOGE("input is NULL! %p,%p \n", pstSystemFrame, pstCoverPrm);
        goto exit;
    }

    s32Ret = RK_MPI_VGS_BeginJob(&hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_BeginJob failed! ret: 0x%x \n", s32Ret);
        goto exit;
    }

    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S));
	
    for (i = 0; i < pstCoverPrm->u32CoverNum; i++, u32CoverNum++)
    {
    	/* 单次job超过hal层限制MAX_VGS_OSD_NUM时，直接抛弃多余的子任务单元 */
        if (i >= MAX_VGS_COVER_NUM)
        {
            SAL_LOGW("cover index %d > cover max num %d \n", i, MAX_VGS_COVER_NUM);
            break;
        }

        pstVgsSingleCoverPrm = &astVgsAddCoverArray[i];

        memset(pstVgsSingleCoverPrm, 0x00, sizeof(VGS_ADD_COVER_S));

		pstVgsSingleCoverPrm->enCoverType = (VGS_COVER_TYPE_E)pstCoverPrm->astCoverPrm[i].enCoverType;
		pstVgsSingleCoverPrm->u32Color = pstCoverPrm->astCoverPrm[i].u32Color;

		switch(pstVgsSingleCoverPrm->enCoverType)
		{
			case COVER_QUAD_RANGLE:
			{
				pstVgsSingleCoverPrm->stQuadRangle.bSolid = pstCoverPrm->astCoverPrm[i].stQuadRangle.bSolid;
				pstVgsSingleCoverPrm->stQuadRangle.stPoint[0].s32X = pstCoverPrm->astCoverPrm[i].stQuadRangle.stPoint[0].s32X;
				pstVgsSingleCoverPrm->stQuadRangle.stPoint[0].s32Y = pstCoverPrm->astCoverPrm[i].stQuadRangle.stPoint[0].s32Y;
				pstVgsSingleCoverPrm->stQuadRangle.stPoint[1].s32X = pstCoverPrm->astCoverPrm[i].stQuadRangle.stPoint[1].s32X;
				pstVgsSingleCoverPrm->stQuadRangle.stPoint[1].s32Y = pstCoverPrm->astCoverPrm[i].stQuadRangle.stPoint[1].s32Y;
				pstVgsSingleCoverPrm->stQuadRangle.stPoint[2].s32X = pstCoverPrm->astCoverPrm[i].stQuadRangle.stPoint[2].s32X;
				pstVgsSingleCoverPrm->stQuadRangle.stPoint[2].s32Y = pstCoverPrm->astCoverPrm[i].stQuadRangle.stPoint[2].s32Y;
				pstVgsSingleCoverPrm->stQuadRangle.stPoint[3].s32X = pstCoverPrm->astCoverPrm[i].stQuadRangle.stPoint[3].s32X;
				pstVgsSingleCoverPrm->stQuadRangle.stPoint[3].s32Y = pstCoverPrm->astCoverPrm[i].stQuadRangle.stPoint[3].s32Y;
				pstVgsSingleCoverPrm->stQuadRangle.u32Thick = pstCoverPrm->astCoverPrm[i].stQuadRangle.u32Thick;

				break;
			}
			case COVER_RECT:
			default:/* 默认使用rect */
			{
				pstVgsSingleCoverPrm->stDstRect.s32X = pstCoverPrm->astCoverPrm[i].stDstRect.s32X;
				pstVgsSingleCoverPrm->stDstRect.s32Y = pstCoverPrm->astCoverPrm[i].stDstRect.s32Y;
				pstVgsSingleCoverPrm->stDstRect.u32Width = pstCoverPrm->astCoverPrm[i].stDstRect.u32Width;
				pstVgsSingleCoverPrm->stDstRect.u32Height = pstCoverPrm->astCoverPrm[i].stDstRect.u32Height;

				break;
			}
		}
    }

    s32Ret = RK_MPI_VGS_AddCoverTaskArray(hHandle, &stTask, astVgsAddCoverArray, u32CoverNum);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_AddOsdTaskArray failed! ret: 0x%x, %d.\n", s32Ret, u32CoverNum);
        goto cancel;
    }

    s32Ret = RK_MPI_VGS_EndJob(hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_EndJob failed! ret: 0x%x \n", s32Ret);
		goto cancel;
    }
	
	return SAL_SOK;
	
cancel:
	(VOID)RK_MPI_VGS_CancelJob(hHandle);

exit:
    return s32Ret;
}

/*****************************************************************************
* 函 数 名  : vgs_rk_DrawMosaicArray
* 功能描述  : 使用 VGS 画 mosaic
* 输入参数  : SYSTEM_FRAME_INFO *pstSystemFrame
              VGS_ADD_MOSAIC_PRM_S *pstMosaicPrm
* 输出参数  : 无
* 返 回 值  : 成功SAL_SOK
*             失败SAL_FAIL
*****************************************************************************/
INT32 ATTRIBUTE_UNUSED vgs_rk_DrawMosaicArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_MOSAIC_PRM_S *pstMosaicPrm)
{
    INT32 s32Ret = SAL_FAIL;

    VGS_HANDLE hHandle = -1;
	
    UINT32 i = 0;
	UINT32 u32MosaicNum = 0;
	
    VGS_TASK_ATTR_S stTask = {0};

    VGS_MOSAIC_S *pstVgsSingleMosaicPrm = NULL;
    VGS_MOSAIC_S astVgsAddMosaicArray[MAX_VGS_MOSAIC_NUM];

    if (NULL == pstSystemFrame
		|| NULL == pstMosaicPrm)
    {
        SAL_LOGE("input is NULL! %p,%p \n", pstSystemFrame, pstMosaicPrm);
        goto exit;
    }

    s32Ret = RK_MPI_VGS_BeginJob(&hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_BeginJob failed! ret: 0x%x \n", s32Ret);
        goto exit;
    }

    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S));
	
    for (i = 0; i < pstMosaicPrm->u32MosaicNum; i++, u32MosaicNum++)
    {
    	/* 单次job超过hal层限制MAX_VGS_MOSAIC_NUM时，直接抛弃多余的子任务单元 */
        if (i >= MAX_VGS_MOSAIC_NUM)
        {
            SAL_LOGW("mosaic index %d > mosaic max num %d \n", i, MAX_VGS_MOSAIC_NUM);
            break;
        }

        pstVgsSingleMosaicPrm = &astVgsAddMosaicArray[i];

        memset(pstVgsSingleMosaicPrm, 0x00, sizeof(VGS_ADD_OSD_S));

		pstVgsSingleMosaicPrm->enBlkSize = (VGS_MOSAIC_BLK_SIZE_E)pstMosaicPrm->astMosaicPrm[i].enBlkSize;
		pstVgsSingleMosaicPrm->stDstRect.s32X = pstMosaicPrm->astMosaicPrm[i].stDstRect.s32X;
		pstVgsSingleMosaicPrm->stDstRect.s32Y = pstMosaicPrm->astMosaicPrm[i].stDstRect.s32Y;
		pstVgsSingleMosaicPrm->stDstRect.u32Width = pstMosaicPrm->astMosaicPrm[i].stDstRect.u32Width;
		pstVgsSingleMosaicPrm->stDstRect.u32Height = pstMosaicPrm->astMosaicPrm[i].stDstRect.u32Height;
    }

    s32Ret = RK_MPI_VGS_AddMosaicTaskArray(hHandle, &stTask, astVgsAddMosaicArray, u32MosaicNum);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_AddOsdTaskArray failed! ret: 0x%x, %d.\n", s32Ret, u32MosaicNum);
        goto cancel;
    }

    s32Ret = RK_MPI_VGS_EndJob(hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_EndJob failed! ret: 0x%x \n", s32Ret);
		goto cancel;
    }

	return SAL_SOK;
	
cancel:
	(VOID)RK_MPI_VGS_CancelJob(hHandle);

exit:
    return s32Ret;
}

/*****************************************************************************
* 函 数 名  : vgs_rk_CropFrame
* 功能描述  : 使用 VGS crop 一帧图像
* 输入参数  : SYSTEM_FRAME_INFO *pstSystemFrame
              VGS_HAL_CROP_PRM_S *pstCropPrm
* 输出参数  : 无
* 返 回 值  : 成功SAL_SOK
*             失败SAL_FAIL
*****************************************************************************/
INT32 ATTRIBUTE_UNUSED vgs_rk_CropFrame(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_HAL_CROP_PRM_S *pstCropPrm)
{
    INT32 s32Ret = SAL_FAIL;

    VGS_HANDLE hHandle = -1;
		
    VGS_TASK_ATTR_S stTask = {0};
	VGS_CROP_INFO_S stCropInfo = {0};

    if (NULL == pstSystemFrame
		|| NULL == pstCropPrm)
    {
        SAL_LOGE("input is NULL! %p,%p \n", pstSystemFrame, pstCropPrm);
        goto exit;
    }

    s32Ret = RK_MPI_VGS_BeginJob(&hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_BeginJob failed! ret: 0x%x \n", s32Ret);
        goto exit;
    }

    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S));

	stCropInfo.enCropCoordinate = (VGS_CROP_COORDINATE_E)pstCropPrm->enCropCoordinate;
	stCropInfo.stCropRect.s32X = pstCropPrm->stCropRect.s32X;
	stCropInfo.stCropRect.s32Y = pstCropPrm->stCropRect.s32Y;
	stCropInfo.stCropRect.u32Width = pstCropPrm->stCropRect.u32Width;
	stCropInfo.stCropRect.u32Height = pstCropPrm->stCropRect.u32Height;

	s32Ret = RK_MPI_VGS_AddCropTask(hHandle, &stTask, &stCropInfo);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_AddCropTask failed! ret: 0x%x \n", s32Ret);
        goto cancel;
    }

    s32Ret = RK_MPI_VGS_EndJob(hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_EndJob failed! ret: 0x%x \n", s32Ret);
		goto cancel;
    }

	return SAL_SOK;
	
cancel:
	(VOID)RK_MPI_VGS_CancelJob(hHandle);

exit:
    return s32Ret;
}

/*****************************************************************************
* 函 数 名  : vgs_rk_RotateFrame
* 功能描述  : 使用 VGS rotate 一帧图像
* 输入参数  : SYSTEM_FRAME_INFO *pstSystemFrame
              VGS_HAL_CROP_PRM_S *pstCropPrm
* 输出参数  : 无
* 返 回 值  : 成功SAL_SOK
*             失败SAL_FAIL
*****************************************************************************/
INT32 ATTRIBUTE_UNUSED vgs_rk_RotateFrame(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_HAL_ROTATE_TYPE_E enRotateType)
{
    INT32 s32Ret = SAL_FAIL;

    VGS_HANDLE hHandle = -1;
		
    VGS_TASK_ATTR_S stTask = {0};

    if (NULL == pstSystemFrame
		|| enRotateType >= VGS_HAL_ROTATION_BUTT)
    {
        SAL_LOGE("input invalid! %p,%d \n", pstSystemFrame, enRotateType);
        goto exit;
    }

    s32Ret = RK_MPI_VGS_BeginJob(&hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_BeginJob failed! ret: 0x%x \n", s32Ret);
        goto exit;
    }

    memcpy(&stTask.stImgIn, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S));
    memcpy(&stTask.stImgOut, (VIDEO_FRAME_INFO_S *)pstSystemFrame->uiAppData, sizeof(VIDEO_FRAME_INFO_S));

	s32Ret = RK_MPI_VGS_AddRotationTask(hHandle, &stTask, (ROTATION_E)enRotateType);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_AddCropTask failed! ret: 0x%x \n", s32Ret);
        goto cancel;
    }

    s32Ret = RK_MPI_VGS_EndJob(hHandle);
    if (RK_SUCCESS != s32Ret)
    {
        SAL_LOGE("RK_MPI_VGS_EndJob failed! ret: 0x%x \n", s32Ret);
		goto cancel;
    }

	return SAL_SOK;
	
cancel:
	(VOID)RK_MPI_VGS_CancelJob(hHandle);

exit:
    return s32Ret;
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

    vgs_osdMemInit();

    g_stVgsPlatOps.drawLineOSDArray = vgs_rk_DrawLineOSDArray;
    g_stVgsPlatOps.drawOsdSingle = vgs_rk_DrawOsdSingle;
    g_stVgsPlatOps.drawOSDArray = vgs_rk_DrawOSDArray;
    g_stVgsPlatOps.drawLineArray = vgs_rk_DrawLineArray;
    g_stVgsPlatOps.scaleFrame = vgs_rk_ScaleFrame;

    return &g_stVgsPlatOps;
}



