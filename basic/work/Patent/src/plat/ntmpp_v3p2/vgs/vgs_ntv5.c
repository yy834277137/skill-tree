/**
 * @file   vgs_ntv3.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  硬件平台 vgs 模块
 * @author liuxianying
 * @date   2021/12/02
 * @note
 * @note \n History
   1.日    期: 2021/12/02
     作    者: liuxianying
     修改历史: 创建文件
 */
#include "platform_sdk.h"
#include "platform_hal.h"
#include "vgs_hal_inter.h"


/*****************************************************************************
                               宏定义
*****************************************************************************/
#define OSD_USE_OSG         (1)

#define OSG_MAX_NUM         (HD_STAMP_EX_MAX - HD_STAMP_EX_BASE + 1)
#define OSG_IN_ID           (3)

/* NT平台gfx拷贝叠OSD的图像，申请的内存宽高，不考虑超分和100100机型 */
#define OSG_FRAME_WIDTH     (1920) /* 最大包裹分割宽度g_pDspInitPara.dspCapbPar.package_len_max(640-1600) + OSD区域最大扩展宽度XPACK_OSD_DISP_EXPAND_MAX*/
#define OSD_FRAME_HEIGHT    (1080) /* 显示画面高度 */

/*****************************************************************************
                               结构体定义
*****************************************************************************/
typedef enum _HD_RGN_ALPHA_TYPE {
    ALPHA_0 = 0, ///< alpha 0%
    ALPHA_25,    ///< alpha 25%
    ALPHA_37_5,  ///< alpha 37.5%
    ALPHA_50,    ///< alpha 50%
    ALPHA_62_5,  ///< alpha 62.5%
    ALPHA_75,    ///< alpha 75%
    ALPHA_87_5,  ///< alpha 87.5%
    ALPHA_100,    ///< alpha 100%
    RGN_ALPHA_TYPE_MAX, ENUM_DUMMY4WORD(HD_RGN_ALPHA_TYPE)
} HD_RGN_ALPHA_TYPE;

/*****************************************************************************
                               全局变量
*****************************************************************************/
static VGS_PLAT_OPS g_stVgsPlatOps = {0};
static HD_PATH_ID g_enOsgId = 0;
static HD_PATH_ID g_aenOsdIds[OSG_MAX_NUM] = {0};
static Handle g_stOsgMutex;

/**
 * @function   vgs_ntv3ScaleFrame
 * @brief      使用 VGS 缩放
 * @param[in]  SYSTEM_FRAME_INFO *pDstSystemFrame  
 * @param[in]  SYSTEM_FRAME_INFO *pSrcSystemFrame  
 * @param[in]  UINT32 dstW                         
 * @param[in]  UINT32 dstH                         
 * @param[out] None
 * @return     INT32
 */
INT32 vgs_ntv3ScaleFrame(SYSTEM_FRAME_INFO *pDstSystemFrame, SYSTEM_FRAME_INFO *pSrcSystemFrame, UINT32 dstW, UINT32 dstH)
{
    INT32 s32Ret = SAL_SOK;
    HD_GFX_SCALE stScale;
    HD_VIDEO_FRAME *pDstVideo = NULL;
    HD_VIDEO_FRAME *pSrcVideo= NULL;


    pDstVideo = (HD_VIDEO_FRAME *)pDstSystemFrame->uiAppData;
    pSrcVideo = (HD_VIDEO_FRAME *)pSrcSystemFrame->uiAppData;

   
    stScale.src_img.dim.w = pSrcVideo->dim.w;
    stScale.src_img.dim.h = pSrcVideo->dim.h;
    stScale.src_img.p_phy_addr[0] = pSrcVideo->phy_addr[0];
    stScale.src_img.ddr_id = DDR1_ADDR_START > pSrcVideo->phy_addr[0] ? DDR_ID0:DDR_ID1;
    stScale.src_img.format = HD_VIDEO_PXLFMT_YUV420;
    
    stScale.src_region.x = 0;
    stScale.src_region.y = 0;
    stScale.src_region.w = pSrcVideo->dim.w;
    stScale.src_region.h = pSrcVideo->dim.h;
    
    stScale.dst_img.dim.w = dstW;
    stScale.dst_img.dim.h = dstH;
    stScale.dst_img.p_phy_addr[0] = pDstVideo->phy_addr[0];
    stScale.dst_img.ddr_id = DDR1_ADDR_START > pDstVideo->phy_addr[0] ? DDR_ID0:DDR_ID1;
    stScale.dst_img.format = HD_VIDEO_PXLFMT_YUV420;

    stScale.dst_region.x = 0;
    stScale.dst_region.y = 0;
    stScale.dst_region.w = dstW;
    stScale.dst_region.h = dstH;
    
    stScale.quality = HD_GFX_SCALE_QUALITY_NULL;

    s32Ret = hd_gfx_scale(&stScale);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("hd_gfx_scale fail, ret:0x%x\n, src:%u x %u, dst:%u x %u\n", 
                 s32Ret, stScale.src_img.dim.w, stScale.src_img.dim.h, stScale.dst_img.dim.w, stScale.dst_img.dim.h);
        return SAL_FAIL;
    }

    pDstVideo->dim.w = dstW;
    pDstVideo->dim.h = dstH;
    pDstVideo->phy_addr[1] = pDstVideo->phy_addr[0] + dstW * dstH;

    return SAL_SOK;
}

#if (OSD_USE_OSG)
/*****************************************************************************
   函 数 名  : osg_ntInit
   功能描述  : osg画osd初始化
   输入参数  : VOID
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
              失败SAL_FAIL
*****************************************************************************/
static INT32 osg_ntInit(VOID)
{
    HD_RESULT enRet = HD_OK;
    INT32 i = 0;

    enRet = hd_videoout_open(HD_VIDEOOUT_IN(OSG_IN_ID, 0), HD_VIDEOOUT_OUT(OSG_IN_ID, 0), &g_enOsgId);
    if (HD_OK != enRet)
    {
        SAL_LOGE("hd_videoout_open fail:%d\n", enRet);
        return SAL_FAIL;
    }

    for (i = 0; i < sizeof(g_aenOsdIds)/sizeof(g_aenOsdIds[0]); i++)
    {
        enRet = hd_videoout_open(HD_VIDEOOUT_IN(OSG_IN_ID, 0), HD_VIDEOOUT_OUT(OSG_IN_ID, HD_STAMP_EX(i)), g_aenOsdIds + i);
        if (HD_OK != enRet)
        {
            SAL_LOGE("hd_videoout_open %u fail:%d\n", i, enRet);
            goto exit;
        }
    }

    SAL_mutexCreate(SAL_MUTEX_NORMAL, &g_stOsgMutex);

    return SAL_SOK;

exit:
    while (--i >= 0)
    {
        enRet = hd_videoout_close(g_aenOsdIds[i]);
        if (HD_OK != enRet)
        {
            SAL_LOGW("hd_videoout_close %u fail:%d\n", i, enRet);
        }
        g_aenOsdIds[i] = 0;
    }

    enRet = hd_videoout_close(g_enOsgId);
    if (HD_OK != enRet)
    {
        SAL_LOGW("hd_videoout_close fail:%d\n", enRet);
    }
    g_enOsgId = 0;

    return SAL_FAIL;
}


/*****************************************************************************
   函 数 名  : osg_ntDrawOSDArray
   功能描述  : 画osd
   输入参数  : pstSystemFrame 帧
              pstOsdPrm      osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
              失败SAL_FAIL
*****************************************************************************/
static INT32 osg_ntDrawOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    HD_RESULT enRet = HD_OK;
    UINT32 i = 0, osdTaskNum = 0, osdDrawnNum = 0;
    HD_OSG_STAMP_IMG stStampImg;
    HD_OSG_STAMP_ATTR stStampAttr;
    HD_VIDEO_FRAME *pVideoFrmInfo = NULL;
    HD_GFX_COPY stCopy = {0};
    ALLOC_VB_INFO_S stVbInfo = {0};
    static HD_VIDEO_FRAME stVideoFrmInfo = {0};

    if (0 == g_enOsgId)
    {
        if (SAL_SOK != osg_ntInit())
        {
            SAL_LOGE("osg init fail\n");
            return SAL_FAIL;
        }

        SAL_LOGI("osg init success\n");
    }

    if (stVideoFrmInfo.phy_addr[0] == 0) /* 第一次画OSD，申请画OSD帧空间 */
    {
        enRet = mem_hal_vbAlloc(OSG_FRAME_WIDTH * OSD_FRAME_HEIGHT * 3 / 2, "osg", "osg_draw_osd", NULL, SAL_TRUE, &stVbInfo);
        if (SAL_FAIL == enRet)
        {
            SAL_LOGE("mem_hal_vbAlloc failed size %u !\n", OSG_FRAME_WIDTH * OSD_FRAME_HEIGHT * 3 / 2);
            return SAL_FAIL;
        }

        stVideoFrmInfo.sign = MAKEFOURCC('V', 'F', 'R', 'M');
        stVideoFrmInfo.blk = stVbInfo.u64VbBlk;
        stVideoFrmInfo.dim.w = OSG_FRAME_WIDTH;
        stVideoFrmInfo.dim.h = OSD_FRAME_HEIGHT;
        stVideoFrmInfo.pxlfmt = HD_VIDEO_PXLFMT_YUV420;
        /* 引擎只给了虚拟地址，所以这里需要从虚拟地址转为物理地址。DSP其他模块物理地址需要从上层传下来 */
        stVideoFrmInfo.phy_addr[0] = stVbInfo.u64PhysAddr;
        stVideoFrmInfo.phy_addr[1] = stVbInfo.u64PhysAddr + stVideoFrmInfo.dim.w * stVideoFrmInfo.dim.h;
        stVideoFrmInfo.phy_addr[2] = stVbInfo.u64PhysAddr + stVideoFrmInfo.dim.w * stVideoFrmInfo.dim.h;
        stVideoFrmInfo.ddr_id = DDR1_ADDR_START > stVbInfo.u64PhysAddr ? DDR_ID0 : DDR_ID1;
        stVideoFrmInfo.timestamp = 0; 
        stVideoFrmInfo.count = 1;
        stVideoFrmInfo.loff[0] = OSG_FRAME_WIDTH;
        stVideoFrmInfo.loff[1] = OSG_FRAME_WIDTH;
        stVideoFrmInfo.ph[0] = OSD_FRAME_HEIGHT; /* 记录buffer高度(跨距) */
    }

    SAL_mutexLock(g_stOsgMutex);
    pVideoFrmInfo = (HD_VIDEO_FRAME *)pstSystemFrame->uiAppData;
    /* NT画OSD不支持横向和纵向的跨距 */
    pVideoFrmInfo->dim.w = pVideoFrmInfo->loff[0]; /* NT不使用loff作为横向偏移量，将整个buff的跨距作为宽传给NT画OSD */
    if (pVideoFrmInfo->phy_addr[1] - pVideoFrmInfo->phy_addr[0] != pVideoFrmInfo->dim.w * pVideoFrmInfo->dim.h) /* 纵向有跨距时需要给NT送纵向跨距作为图像高 */
    {
        pVideoFrmInfo->dim.h = pVideoFrmInfo->ph[0]; /* NT当前osg画OSD不使用ph作为图像高 */
    }

    /* 通过GFX将需要叠OSD的图像拷贝出来 */
    stCopy.dst_img.dim.w = stVideoFrmInfo.dim.w;
    stCopy.dst_img.dim.h = stVideoFrmInfo.dim.h;
    stCopy.dst_img.format = HD_VIDEO_PXLFMT_YUV420;
    stCopy.dst_img.p_phy_addr[0] = stVideoFrmInfo.phy_addr[0];
    stCopy.dst_img.p_phy_addr[1] = stVideoFrmInfo.phy_addr[1];
    stCopy.dst_img.p_phy_addr[2] = stVideoFrmInfo.phy_addr[2];
    stCopy.dst_img.ddr_id = stVideoFrmInfo.ddr_id;

    stCopy.dst_pos.x = 0;
    stCopy.dst_pos.y = 0;

    stCopy.src_region.x = pstSystemFrame->u32LeftOffset;
    stCopy.src_region.y = pstSystemFrame->u32TopOffset;
    stCopy.src_region.w = pstSystemFrame->uiDataWidth;
    stCopy.src_region.h = pstSystemFrame->uiDataHeight;

    stCopy.src_img.dim.w = pVideoFrmInfo->dim.w;
    stCopy.src_img.dim.h = pVideoFrmInfo->dim.h;
    stCopy.src_img.format = HD_VIDEO_PXLFMT_YUV420;
    stCopy.src_img.p_phy_addr[0] = pVideoFrmInfo->phy_addr[0];
    stCopy.src_img.ddr_id = pVideoFrmInfo->ddr_id;
    enRet = hd_gfx_copy(&stCopy);
    if (HD_OK != enRet)
    {
        SAL_LOGE("hd_gfx_copy fail:%d\n", enRet);
        goto exit1;
    }

    /* 使用OSG叠OSD */
    memset(&stStampImg, 0, sizeof(stStampImg));
    memset(&stStampAttr, 0, sizeof(stStampAttr));
    while (osdDrawnNum < pstOsdPrm->uiOsdNum)
    {
        osdTaskNum = pstOsdPrm->uiOsdNum - osdDrawnNum > OSG_MAX_NUM ? OSG_MAX_NUM : pstOsdPrm->uiOsdNum - osdDrawnNum;
        for (i = 0; i < osdTaskNum; i++)
        {
            stStampImg.p_addr = (UINTPTR)pstOsdPrm->stVgsOsdPrm[osdDrawnNum + i].u64PhyAddr;
            stStampImg.ddr_id = pstOsdPrm->stVgsOsdPrm[osdDrawnNum + i].u64PhyAddr > DDR1_ADDR_START ? DDR_ID1 : DDR_ID0;
            stStampImg.dim.w = pstOsdPrm->stVgsOsdPrm[osdDrawnNum + i].u32Width;
            stStampImg.dim.h = pstOsdPrm->stVgsOsdPrm[osdDrawnNum + i].u32Height;
            stStampImg.fmt = HD_VIDEO_PXLFMT_ARGB1555;
            enRet = hd_videoout_set(g_aenOsdIds[i], HD_VIDEOOUT_PARAM_OUT_STAMP_IMG, (VOID *)&stStampImg);
            if (HD_OK != enRet)
            {
                SAL_LOGE("%u set stamp img fail:%d\n", i, enRet);
                goto exit1;
            }

            stStampAttr.align_type = HD_OSG_ALIGN_TYPE_TOP_LEFT;
            stStampAttr.alpha = ALPHA_100;
            stStampAttr.position.x = pstOsdPrm->stVgsOsdPrm[osdDrawnNum + i].s32X;
            stStampAttr.position.y = pstOsdPrm->stVgsOsdPrm[osdDrawnNum + i].s32Y;
            enRet = hd_videoout_set(g_aenOsdIds[i], HD_VIDEOOUT_PARAM_OUT_STAMP_ATTR, (VOID *)&stStampAttr);
            if (HD_OK != enRet)
            {
                SAL_LOGE("%u set stamp attr fail:%d\n", i, enRet);
                goto exit1;
            }
        }

        enRet = hd_videoout_start_list(g_aenOsdIds, i);
        if (HD_OK != enRet)
        {
            SAL_LOGE("%u start fail:%d\n", i, enRet);
            goto exit1;
        }

        enRet = hd_videoout_push_in_buf(g_enOsgId, &stVideoFrmInfo, NULL, 500);
        if (HD_OK != enRet)
        {
            SAL_LOGE("vo push in buf fail\n");
            goto exit2;
        }

        enRet = hd_videoout_pull_out_buf(g_enOsgId, &stVideoFrmInfo, 500);
        if (HD_OK != enRet)
        {
            SAL_LOGE("vo pull out buf fail, err:%d\n", enRet);
            goto exit2;
        }

        enRet = hd_videoout_stop_list(g_aenOsdIds, i);
        if (HD_OK != enRet)
        {
            SAL_LOGE("vo stop fail\n");
            goto exit2;
        }
        osdDrawnNum += osdTaskNum;
    }

    /* 通过GFX将叠好OSD的图像拷贝回去 */
    stCopy.dst_img.dim.w = pVideoFrmInfo->dim.w;
    stCopy.dst_img.dim.h = pVideoFrmInfo->dim.h;
    stCopy.dst_img.format = HD_VIDEO_PXLFMT_YUV420;
    stCopy.dst_img.p_phy_addr[0] = pVideoFrmInfo->phy_addr[0];
    stCopy.dst_img.p_phy_addr[1] = pVideoFrmInfo->phy_addr[1];
    stCopy.dst_img.p_phy_addr[2] = pVideoFrmInfo->phy_addr[2];
    stCopy.dst_img.ddr_id = pVideoFrmInfo->ddr_id;

    stCopy.dst_pos.x = pstSystemFrame->u32LeftOffset;
    stCopy.dst_pos.y = pstSystemFrame->u32TopOffset;

    stCopy.src_region.x = 0;
    stCopy.src_region.y = 0;
    stCopy.src_region.w = pstSystemFrame->uiDataWidth;
    stCopy.src_region.h = pstSystemFrame->uiDataHeight;

    stCopy.src_img.dim.w = stVideoFrmInfo.dim.w;
    stCopy.src_img.dim.h = stVideoFrmInfo.dim.h;
    stCopy.src_img.format = HD_VIDEO_PXLFMT_YUV420;
    stCopy.src_img.p_phy_addr[0] = stVideoFrmInfo.phy_addr[0];
    stCopy.src_img.ddr_id = stVideoFrmInfo.ddr_id;
    enRet = hd_gfx_copy(&stCopy);
    if (HD_OK != enRet)
    {
        SAL_LOGE("hd_gfx_copy fail:%d\n", enRet);
        goto exit2;
    }

    SAL_mutexUnlock(g_stOsgMutex);
    return SAL_SOK;

exit2:
    enRet = hd_videoout_stop_list(g_aenOsdIds, i);
    if (HD_OK != enRet)
    {
        SAL_LOGE("vo stop fail\n");
    }

exit1:
    SAL_LOGE("drawOSD state: totall:%d finish:%d drawing:%d/%d\n", pstOsdPrm->uiOsdNum, osdDrawnNum, i, osdTaskNum);
    SAL_mutexUnlock(g_stOsgMutex);

    return SAL_FAIL;
}
#endif

/*****************************************************************************
   函 数 名  : gfx_ntDrawOsdSingle
   功能描述  : 画osd
   输入参数  : pstSystemFrame  帧
              VGS_ADD_OSD_PRM 画osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
              失败SAL_FAIL
*****************************************************************************/
INT32 gfx_ntDrawOsdSingle(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    HD_RESULT enRet = HD_OK;
    HD_GFX_COPY stCopy;
    HD_VIDEO_FRAME *pstVideoFrame = (HD_VIDEO_FRAME *)pstSystemFrame->uiAppData;

    stCopy.dst_img.dim.w = pstVideoFrame->dim.w;
    stCopy.dst_img.dim.h = pstVideoFrame->dim.h;
    stCopy.dst_img.format = pstVideoFrame->pxlfmt;
    stCopy.dst_img.p_phy_addr[0] = pstVideoFrame->phy_addr[0];
    stCopy.dst_img.p_phy_addr[1] = pstVideoFrame->phy_addr[1];
    stCopy.dst_img.p_phy_addr[2] = pstVideoFrame->phy_addr[2];
    stCopy.dst_img.ddr_id = pstVideoFrame->ddr_id;

    stCopy.dst_pos.x = pstOsdPrm->stVgsOsdPrm[0].s32X;
    stCopy.dst_pos.y = pstOsdPrm->stVgsOsdPrm[0].s32Y;

    stCopy.src_region.x = 0;
    stCopy.src_region.y = 0;
    stCopy.src_region.w = pstOsdPrm->stVgsOsdPrm[0].u32Width;
    stCopy.src_region.h = pstOsdPrm->stVgsOsdPrm[0].u32Height;

    stCopy.src_img.dim.w = pstOsdPrm->stVgsOsdPrm[0].u32Width;
    stCopy.src_img.dim.h = pstOsdPrm->stVgsOsdPrm[0].u32Height;
    stCopy.src_img.format = pstOsdPrm->stVgsOsdPrm[0].enPixelFmt;
    stCopy.src_img.p_phy_addr[0] = pstOsdPrm->stVgsOsdPrm[0].u64PhyAddr;
    stCopy.src_img.ddr_id = pstOsdPrm->stVgsOsdPrm[0].u64PhyAddr > 0x100000000 ? DDR_ID1 : DDR_ID0;;

    stCopy.colorkey = pstOsdPrm->stVgsOsdPrm[0].u32BgColor;
    stCopy.alpha    = (pstOsdPrm->stVgsOsdPrm[0].u32FgAlpha & 0xF0) | ((pstOsdPrm->stVgsOsdPrm[0].u32BgAlpha >> 4) & 0xF0);
    enRet = hd_gfx_copy(&stCopy);
    if (HD_OK != enRet)
    {
        SAL_LOGE("hd_gfx_copy fail:%d\n", enRet);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*****************************************************************************
   函 数 名  : gfx_ntDrawOsdArray
   功能描述  : 画osd
   输入参数  : pstSystemFrame  帧
              VGS_ADD_OSD_PRM 画osd的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
              失败SAL_FAIL
*****************************************************************************/
INT32 gfx_ntDrawOsdArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm)
{
    HD_RESULT enRet = HD_OK;
    HD_GFX_COPY stCopy;
    HD_VIDEO_FRAME *pstVideoFrame = (HD_VIDEO_FRAME *)pstSystemFrame->uiAppData;
    UINT32 i = 0;

    stCopy.dst_img.dim.w = pstVideoFrame->dim.w;
    stCopy.dst_img.dim.h = pstVideoFrame->dim.h;
    stCopy.dst_img.format = pstVideoFrame->pxlfmt;
    stCopy.dst_img.p_phy_addr[0] = pstVideoFrame->phy_addr[0];
    stCopy.dst_img.p_phy_addr[1] = pstVideoFrame->phy_addr[1];
    stCopy.dst_img.p_phy_addr[2] = pstVideoFrame->phy_addr[2];
    stCopy.dst_img.ddr_id = pstVideoFrame->ddr_id;

    for (i = 0; i < pstOsdPrm->uiOsdNum; i++)
    {
        stCopy.dst_pos.x = pstOsdPrm->stVgsOsdPrm[i].s32X;
        stCopy.dst_pos.y = pstOsdPrm->stVgsOsdPrm[i].s32Y;

        stCopy.src_region.x = 0;
        stCopy.src_region.y = 0;
        stCopy.src_region.w = pstOsdPrm->stVgsOsdPrm[i].u32Width;
        stCopy.src_region.h = pstOsdPrm->stVgsOsdPrm[i].u32Height;

        stCopy.src_img.dim.w = pstOsdPrm->stVgsOsdPrm[i].u32Width;
        stCopy.src_img.dim.h = pstOsdPrm->stVgsOsdPrm[i].u32Height;
        stCopy.src_img.format = HD_VIDEO_PXLFMT_ARGB1555;
        stCopy.src_img.p_phy_addr[0] = pstOsdPrm->stVgsOsdPrm[i].u64PhyAddr > 0x100000000 ? DDR_ID1 : DDR_ID0;
        stCopy.src_img.ddr_id = DDR_ID0;
        enRet = hd_gfx_copy(&stCopy);
        if (HD_OK != enRet)
        {
            SAL_LOGE("hd_gfx_copy fail:%d\n", enRet);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/*****************************************************************************
   函 数 名  : gfx_ntDrawLineArray
   功能描述  : 画多条线
   输入参数  : pstSystemFrame 帧
              pstLinePrm 画线的信息参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
              失败SAL_FAIL
*****************************************************************************/
INT32 gfx_ntDrawLineArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_DRAW_LINE_PRM *pstLinePrm)
{
    HD_RESULT enRet = HD_OK;
    HD_GFX_DRAW_LINE stLine;
    HD_VIDEO_FRAME *pstVideoFrame = (HD_VIDEO_FRAME *)pstSystemFrame->uiAppData;
    UINT32 u32Color;
    UINT32 i = 0;

    stLine.dst_img.dim.w = pstVideoFrame->dim.w;
    stLine.dst_img.dim.h = pstVideoFrame->dim.h;
    stLine.dst_img.format = pstVideoFrame->pxlfmt;
    stLine.dst_img.p_phy_addr[0] = pstVideoFrame->phy_addr[0];
    stLine.dst_img.p_phy_addr[1] = pstVideoFrame->phy_addr[1];
    stLine.dst_img.p_phy_addr[2] = pstVideoFrame->phy_addr[2];
    stLine.dst_img.ddr_id = pstVideoFrame->ddr_id;

    for (i = 0; i < pstLinePrm->uiLineNum; i++)
    {
        stLine.start.x   = pstLinePrm->stVgsLinePrm[i].stStartPoint.s32X;
        stLine.start.y   = pstLinePrm->stVgsLinePrm[i].stStartPoint.s32Y;
        stLine.end.x     = pstLinePrm->stVgsLinePrm[i].stEndPoint.s32X;
        stLine.end.y     = pstLinePrm->stVgsLinePrm[i].stEndPoint.s32Y;
        stLine.thickness = pstLinePrm->stVgsLinePrm[i].u32Thick;

        u32Color = pstLinePrm->stVgsLinePrm[i].u32Color;
        stLine.color = 0x000000FF | ((u32Color >> 8) & 0x0000FF00) | ((u32Color <<  8) & 0x00FF0000) | ((u32Color << 24) & 0xFF000000);
        enRet = hd_gfx_draw_line(&stLine);
        if (HD_OK != enRet)
        {
            SAL_LOGE("hd_gfx_draw_line fail:%d\n", enRet);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/*****************************************************************************
   函 数 名  : gfx_ntDrawLineOSDArray
   功能描述  : 画osd和框
   输入参数  : pstSystemFrame 帧
              pstOsdPrm      osd的信息参数
              pstLinePrm     框参数
   输出参数  : 无
   返 回 值  : 成功SAL_SOK
              失败SAL_FAIL
*****************************************************************************/
INT32 gfx_ntDrawLineOSDArray(SYSTEM_FRAME_INFO *pstSystemFrame, VGS_ADD_OSD_PRM *pstOsdPrm, VGS_DRAW_LINE_PRM *pstLinePrm)
{
    if (NULL != pstLinePrm)
    {
        if (SAL_SOK != gfx_ntDrawLineArray(pstSystemFrame, pstLinePrm))
        {
            SAL_LOGE("draw line array fail\n");
        }
    }

    if (NULL != pstOsdPrm)
    {
#if (OSD_USE_OSG)
        if (SAL_SOK != osg_ntDrawOSDArray(pstSystemFrame, pstOsdPrm))
#else
        if (SAL_SOK != gfx_ntDrawOsdArray(pstSystemFrame, pstOsdPrm))
#endif
        {
            SAL_LOGE("draw osd array fail\n");
        }
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

    g_stVgsPlatOps.drawLineOSDArray = gfx_ntDrawLineOSDArray;
    g_stVgsPlatOps.drawOsdSingle    = gfx_ntDrawOsdSingle;
    g_stVgsPlatOps.drawOSDArray     = gfx_ntDrawOsdArray;
    g_stVgsPlatOps.drawLineArray    = gfx_ntDrawLineArray;
    g_stVgsPlatOps.scaleFrame = vgs_ntv3ScaleFrame;

    return &g_stVgsPlatOps;
}

