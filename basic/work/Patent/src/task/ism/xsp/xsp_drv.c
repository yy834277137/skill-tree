/**
 * @file   xsp_drv.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  XSP成像处理驱动文件
 * @author liwenbin
 * @date   2019/10/23
 * @note :
 * @note \n History:
   1.Date        : 2019/10/23
     Author      : liwenbin
     Modification: Created file
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include "system_prm_api.h"
#include "xsp_drv.h"

#line __LINE__ "xsp_drv.c"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
/* XSP处理模块通道号校验 */
#define XSP_TSK_CHECK_CHAN(chan, value)  \
    do { \
        if (chan >= XSP_MAX_CHN_CNT) \
        { \
            XSP_LOGE("Chan (Illegal parameters)\n"); \
            return (value); \
        } \
    } while (0)

/* XSP处理模块指针校验 */
#define XSP_TSK_CHECK_PRM(ptr, value) \
    do { \
        if (!ptr) \
        { \
            XSP_LOGE("Ptr (The address is empty or Value is 0 )\n"); \
            return (value); \
        } \
    } while (0)

/* XSP处理模块指针校验 */
#define XSP_CHECK_PTR_IS_NULL(ptr, errno) \
    do { \
        if (!ptr) \
        { \
            XSP_LOGE("the '%s' is NULL\n", # ptr); \
            return (errno); \
        } \
    } while (0)


/* XSP转存模块，OSD使用 */
#define XSP_PB_SLICE_LINE_PER_FRAME 24          /* 条带回拉每帧刷新列数，目前是固定的24列 */
#define XSP_MAX_IMGEN_WIDTH         (4500)      /* 对齐成像算法的XRAY_LIB_MAX_IMGENHANCE_LENGTH宏 */
#define CACHE_SLICE_DIVIDE_MAX      (100)       /* 同步包裹分割结果所用缓存 */
#define APP2DSP_DATA_H      (718)               /* 应用在图像设置中需要显示的数据高 */

typedef struct
{
    void *pXspHandle;                       // XSP算法句柄 */
    XIMAGE_DATA_ST stBlendTmp;              // 转存处理存放融合灰度图使用的Buffer
    XIMAGE_DATA_ST stTransDisp;             // 转存显示图像数据
} XSP_TRANS_PROC;


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */
void *Xsp_DrvPackageIdentifyThread(void *args);
static void xsp_rtproc_pipeline_cb(void *pXspHandle, XSP_RTPROC_OUT *pstRtProcOut);
extern XRAY_DEADPIXEL_RESULT *xraw_dp_get_info(UINT32 chan);
extern void xraw_dp_correct(XRAY_DEADPIXEL_RESULT *p_dp_result, UINT16 *p_raw_buf, UINT32 raw_w, UINT32 raw_h);
DISP_CHN_COMMON *disp_tskGetVoChn(UINT32 uiDev, UINT32 uiChn);
extern int xsp_xraw_resize(DSP_IMG_DATFMT enImgFmt, 
                           void *pDstXraw, unsigned int u32DstW, unsigned int u32DstH, 
                           void *pSrcXraw, unsigned int u32SrcW, unsigned int u32SrcH);


/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
/* XSP处理本地控制*/
static XSP_COMMON g_xsp_common = {0};
static XSP_TRANS_PROC gstXspTransProc = {0}; /* 转存功能参数 */


/**
 * @fn      convert_rect_region_rotate
 * @brief   矩形框随图像旋转的重定位
 * @param   ff_width[IN] 整幅图像宽，不小于输入矩形的X坐标与宽的和
 * @param   ff_height[IN] 整幅图像高，不小于输入矩形的Y坐标与高的和
 * @param   rotate[IN] 旋转角度，顺时针方向，具体参考枚举定义
 * @param   p_coord_in[IN] 需转换的矩形框
 * @param   p_coord_out[OUT] 转换后的矩形框
 */
static void convert_rect_region_rotate(IN U32 ff_width, IN U32 ff_height, IN ROTATE_MODE rotate, IN XSP_RECT *p_coord_in, OUT XSP_RECT *p_coord_out)
{
    XSP_RECT stRectTmp = {0}; /* 若p_coord_in与p_coord_out地址相同，需要用到该临时变量做转换 */
    XSP_RECT *pRect = NULL;

    if (NULL == p_coord_in || NULL == p_coord_out)
    {
        XSP_LOGE("p_coord_in(%p) OR p_coord_out(%p) is NULL\n", p_coord_in, p_coord_out);
        return;
    }

    if (ff_height < p_coord_in->uiY + p_coord_in->uiHeight)
    {
        XSP_LOGE("ff_height(%u) is smaller than uiY(%u) + uiHeight(%u)\n", ff_height, p_coord_in->uiY, p_coord_in->uiHeight);
        return;
    }

    if (ff_width < p_coord_in->uiX + p_coord_in->uiWidth)
    {
        XSP_LOGE("ff_width(%u) is smaller than uiX(%u) + uiWidth(%u)\n", ff_width, p_coord_in->uiX, p_coord_in->uiWidth);
        return;
    }

    if (p_coord_in == p_coord_out)
    {
        memcpy(&stRectTmp, p_coord_in, sizeof(XSP_RECT));
        pRect = &stRectTmp;
    }
    else
    {
        pRect = p_coord_in;
    }

    switch (rotate) /* 顺时针旋转 */
    {
        case ROTATE_MODE_90: /* 长变宽，宽变长 */
            p_coord_out->uiX = ff_height - (pRect->uiY + pRect->uiHeight);
            p_coord_out->uiY = pRect->uiX;
            p_coord_out->uiWidth = pRect->uiHeight;
            p_coord_out->uiHeight = pRect->uiWidth;
            break;

        case ROTATE_MODE_180: /* 相当于垂直镜像+水平镜像 */
            p_coord_out->uiX = ff_width - (pRect->uiX + pRect->uiWidth);
            p_coord_out->uiY = ff_height - (pRect->uiY + pRect->uiHeight);
            p_coord_out->uiWidth = pRect->uiWidth;
            p_coord_out->uiHeight = pRect->uiHeight;
            break;

        case ROTATE_MODE_270:
            p_coord_out->uiX = pRect->uiY;
            p_coord_out->uiY = ff_width - (pRect->uiX + pRect->uiWidth);
            p_coord_out->uiWidth = pRect->uiHeight;
            p_coord_out->uiHeight = pRect->uiWidth;
            break;

        default:
            p_coord_out->uiX = pRect->uiX;
            p_coord_out->uiY = pRect->uiY;
            p_coord_out->uiWidth = pRect->uiWidth;
            p_coord_out->uiHeight = pRect->uiHeight;
            break;
    }

    return;
}

/**
 * @fn      convert_rect_region_mirror
 * @brief   矩形框随图像镜像的重定位
 * @param   ff_width[IN] 整幅图像宽，不小于输入矩形的X坐标与宽的和
 * @param   ff_height[IN] 整幅图像高，不小于输入矩形的Y坐标与高的和
 * @param   mirror[IN] 镜像类型，具体参考枚举定义
 * @param   p_coord_in[IN] 需转换的矩形框
 * @param   p_coord_out[OUT] 转换后的矩形框
 */
static void convert_rect_region_mirror(IN U32 ff_width, IN U32 ff_height, IN MIRROR_MODE mirror, IN XSP_RECT *p_coord_in, OUT XSP_RECT *p_coord_out)
{
    XSP_RECT stRectTmp = {0}; /* 若p_coord_in与p_coord_out地址相同，需要用到该临时变量做转换 */
    XSP_RECT *pRect = NULL;

    if (NULL == p_coord_in || NULL == p_coord_out)
    {
        XSP_LOGE("p_coord_in(%p) OR p_coord_out(%p) is NULL\n", p_coord_in, p_coord_out);
        return;
    }

    if (ff_height < p_coord_in->uiY + p_coord_in->uiHeight)
    {
        XSP_LOGE("ff_height(%u) is smaller than uiY(%u) + uiHeight(%u)\n", ff_height, p_coord_in->uiY, p_coord_in->uiHeight);
        return;
    }

    if (ff_width < p_coord_in->uiX + p_coord_in->uiWidth)
    {
        XSP_LOGE("ff_width(%u) is smaller than uiX(%u) + uiWidth(%u)\n", ff_width, p_coord_in->uiX, p_coord_in->uiWidth);
        return;
    }

    if (p_coord_in == p_coord_out)
    {
        memcpy(&stRectTmp, p_coord_in, sizeof(XSP_RECT));
        pRect = &stRectTmp;
    }
    else
    {
        pRect = p_coord_in;
    }

    switch (mirror)
    {
        case MIRROR_MODE_HORIZONTAL: /* 除了X，其他都不变 */
            p_coord_out->uiX = ff_width - (pRect->uiX + pRect->uiWidth);
            p_coord_out->uiY = pRect->uiY;
            p_coord_out->uiWidth = pRect->uiWidth;
            p_coord_out->uiHeight = pRect->uiHeight;
            break;

        case MIRROR_MODE_VERTICAL: /* 除了Y，其他都不变 */
            p_coord_out->uiX = pRect->uiX;
            p_coord_out->uiY = ff_height - (pRect->uiY + pRect->uiHeight);
            p_coord_out->uiWidth = pRect->uiWidth;
            p_coord_out->uiHeight = pRect->uiHeight;
            break;

        default:
            p_coord_out->uiX = pRect->uiX;
            p_coord_out->uiY = pRect->uiY;
            p_coord_out->uiWidth = pRect->uiWidth;
            p_coord_out->uiHeight = pRect->uiHeight;
            break;
    }

    return;
}

/**
 * @function    xsp_getNodeNum
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static UINT32 xsp_getNodeNum(CAPB_XRAY_IN *pstCapbXrayIn)
{
    if (!pstCapbXrayIn)
    {
        XSP_LOGE("pstCapbXrayIn is NULL\n");
        return 0;
    }

    if (pstCapbXrayIn->line_per_slice_min <= 24) /* 最小条带高度小于等于24时，分配的节点数多一点 */
    {
        return XSP_DATA_NODE_NUM_MAX;
    }
    else
    {
        return XSP_DATA_NODE_NUM;
    }
}

/**
 * @fn      build_disp_bgcolor
 * @brief   修改反色后系统背景色值
 * @note    反色操作需调用该接口重新构建背景色
 *
 * @param   b_anti[IN] 是否反色，FALSE-否，背景为白色，TRUE-是，背景为黑色
 */
static void build_disp_bgcolor(U32 chan, BOOL b_anti)
{
    U32 i = 0;
    U32 u32TopStartLine = 0, u32BotStartLine, u32FillHeight = 0; /* 图像填充区域的开始行和填充高度 */
    UINT32 u32XspDataNodeNum = 0;       /* XSP处理队列的节点数 */
    U32 *pu32BgColorStd = &g_xsp_common.stXspChn[chan].u32XspBgColorStd;
    U32 *pu32BgColorUsed = &g_xsp_common.stXspChn[chan].u32XspBgColorUsed;

    XSP_DATA_NODE *pstDataNode = NULL;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();

    u32XspDataNodeNum = xsp_getNodeNum(pstCapbXrayIn);

    /*合法性校验*/
    if ((NULL == pstCapbXrayIn) || (NULL == pstCapbXsp))
    {
        XSP_LOGE("pstCapbXrayIn [%p], pstCapbXsp [%p] is error!\n", pstCapbXrayIn, pstCapbXsp);
        return;
    }

    if (chan >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan [%d] is error, chan cnt is %d!\n", chan, pstCapbXrayIn->xray_in_chan_cnt);
        return;
    }

    u32TopStartLine = 0;
    u32BotStartLine = pstCapbDisp->disp_h_middle_indeed + pstCapbDisp->disp_h_top_padding + pstCapbDisp->disp_h_top_blanking;
    u32FillHeight = pstCapbDisp->disp_h_top_blanking + pstCapbDisp->disp_h_top_padding;
    *pu32BgColorUsed = *pu32BgColorStd;
    if (b_anti) /* 之前是正色状态，现在是反色状态 */
    {
        *pu32BgColorUsed = (pstCapbXsp->enDispFmt & DSP_IMG_DATFMT_YUV420_MASK) ? XIMG_BG_ANTI_YUV(*pu32BgColorUsed) : XIMG_BG_ANTI_RGB(*pu32BgColorUsed);
    }

    for (i = 0; i < u32XspDataNodeNum; i++)
    {
        pstDataNode = g_xsp_common.stXspChn[chan].stDataNode + i;

        /* 修改stDispFscData缓冲区的背景填充色 */
        /* 顶部区域 */
        if (SAL_SOK != ximg_fill_color(&pstDataNode->stDispFscData, u32TopStartLine, u32FillHeight, 0, pstDataNode->stDispFscData.u32Width, *pu32BgColorUsed))
        {
            XSP_LOGW("ximg_fill_color for disp fsc top data failed\n");
        }

        /* 底部区域 */
        if (SAL_SOK != ximg_fill_color(&pstDataNode->stDispFscData, u32BotStartLine, u32FillHeight, 0, pstDataNode->stDispFscData.u32Width, *pu32BgColorUsed))
        {
            XSP_LOGW("ximg_fill_color disp fsc bottom data failed\n");
        }
    }

    return;
}

/******************************************************************
 * @function:   Xsp_ThreadSync
 * @brief:      数据发送线程同步,先到的线程进入睡眠，后到的线程直接往下执行，通过条件变量唤醒先到的线程
 * @param[in]:  UINT32 u32Chan 通道号
 * @return:
 *******************************************************************/
static void Xsp_ThreadSync(UINT32 chan, UINT64 u64TimeOut)
{
    XSP_FSC_SYNC *pstFscSync = &g_xsp_common.stFscSync;
    clock_t start = 0;

    SAL_mutexTmLock(&pstFscSync->condFscSync.mid, 2000, __FUNCTION__, __LINE__);
    pstFscSync->enSendReadyNum++;
    if (XSP_FSC_SEND_NUM_2 != pstFscSync->enSendReadyNum)
    {
        SAL_CondWait(&pstFscSync->condFscSync, u64TimeOut, __FUNCTION__, __LINE__);
        pstFscSync->enSendReadyNum--;
        SAL_mutexTmUnlock(&pstFscSync->condFscSync.mid, __FUNCTION__, __LINE__);
        start = clock();
        while (0 != pstFscSync->enSendReadyNum)
        {
            if ((clock() - start) >= 300)
            {
                XSP_LOGW("chan: %d, send sync timeout\n", chan);
                break;      /* 超时退出 */
            }
        }
    }
    else
    {
        SAL_CondSignal(&pstFscSync->condFscSync, 2000, __FUNCTION__, __LINE__);
        pstFscSync->enSendReadyNum--;
        SAL_mutexTmUnlock(&pstFscSync->condFscSync.mid, __FUNCTION__, __LINE__);
        start = clock();
        while (0 != pstFscSync->enSendReadyNum)
        {
            if ((clock() - start) >= 300)
            {
                XSP_LOGW("chan: %d, send sync timeout\n", chan);
                break;      /* 超时退出 */
            }
        }
    }
}


/**
 * @function    xsp_dump_current_zero_corr
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void xsp_dump_current_zero_corr(UINT32 chan)
{
    UINT32 corr_data_size = 0;
    CHAR dumpName[128] = {0};
    XSP_CHN_PRM *pstXspChn = NULL;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        SAL_print("the chan[%u] is invalid, < %u", chan, pCapbXrayin->xray_in_chan_cnt);
        return;
    }

    pstXspChn = &g_xsp_common.stXspChn[chan];
    if (NULL != pstXspChn->pEmptyLoadData)
    {
        if (SAL_SOK == xsp_get_correction_data(pstXspChn->handle, XSP_CORR_EMPTYLOAD, pstXspChn->pEmptyLoadData, &corr_data_size))
        {
            snprintf(dumpName, sizeof(dumpName), "%s/zerocur_ch%u_%u_w%u_h%u_t%llu.xraw", pstXspChn->stDumpCfg.chDumpDir,
                     chan, pstXspChn->stDbgStatus.u32ProcCnt, pCapbXrayin->xraw_width_max, 1, sal_get_tickcnt());
            SAL_WriteToFile(dumpName, 0, SEEK_SET, pstXspChn->pEmptyLoadData, corr_data_size);
        }
    }

    return;
}

/**
 * @function    xsp_dump_current_full_corr
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void xsp_dump_current_full_corr(UINT32 chan)
{
    UINT32 corr_data_size = 0;
    CHAR dumpName[128] = {0};
    XSP_CHN_PRM *pstXspChn = NULL;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        SAL_print("the chan[%u] is invalid, < %u", chan, pCapbXrayin->xray_in_chan_cnt);
        return;
    }

    pstXspChn = &g_xsp_common.stXspChn[chan];
    if (NULL != pstXspChn->pFullLoadData)
    {
        if (SAL_SOK == xsp_get_correction_data(pstXspChn->handle, XSP_CORR_FULLLOAD, pstXspChn->pFullLoadData, &corr_data_size))
        {
            snprintf(dumpName, sizeof(dumpName), "%s/fullcur_ch%u_%u_w%u_h%u_t%llu.xraw", pstXspChn->stDumpCfg.chDumpDir,
                     chan, pstXspChn->stDbgStatus.u32ProcCnt, pCapbXrayin->xraw_width_max, 1, sal_get_tickcnt());
            SAL_WriteToFile(dumpName, 0, SEEK_SET, pstXspChn->pFullLoadData, corr_data_size);
        }
    }

    return;
}

/******************************************************************
 * @function   Xsp_DumpSet
 * @brief      数据下载设置
 * @param[in]  UINT32 u32Chan 通道号
 * @param[in]  BOOL bEnable 开关
 * @param[in]  CHAR *pchDumpDir 下载路径
 * @param[in]  UINT32 u32DumpDp 下载
 * @param[in] UINT32 u32DumpCnt 下载个数
 * @return  OK or ERR Information
 *******************************************************************/
void Xsp_DumpSet(UINT32 u32Chan, BOOL bEnable, CHAR *pchDumpDir, UINT32 u32DumpDp, UINT32 u32DumpCnt)
{
    UINT32 u32DirStrlen = 0;
    XSP_CHN_PRM *pstXspChn = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == pchDumpDir || 0 == strlen(pchDumpDir))
    {
        XSP_LOGE("the pchDumpDir is NULL\n");
        return;
    }

    if (u32Chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("the u32Chan[%u] is invalid, range: [0, %u)\n", u32Chan, pstCapXrayIn->xray_in_chan_cnt);
        return;
    }

    pstXspChn = &g_xsp_common.stXspChn[u32Chan];

    /* 存储目录 */
    u32DirStrlen = sizeof(pstXspChn->stDumpCfg.chDumpDir) - 1;
    strncpy(pstXspChn->stDumpCfg.chDumpDir, pchDumpDir, u32DirStrlen);
    pstXspChn->stDumpCfg.chDumpDir[u32DirStrlen] = '\0';

    /* DUMP类型 */
    pstXspChn->stDumpCfg.u32DumpDp = u32DumpDp;

    /* DUMP次数 */
    if (bEnable)
    {
        pstXspChn->stDumpCfg.u32DumpCnt = (u32DumpCnt == 0xFFFFFFFF) ? 0xFFFFFFFF : (u32DumpCnt + 1);
    }
    else
    {
        pstXspChn->stDumpCfg.u32DumpCnt = 0;
    }

    XSP_LOGW("Xsp_DumpSet successfully, DumpDir: %s, DumpDp: 0x%x, DumpCnt: %u\n",
             pstXspChn->stDumpCfg.chDumpDir, pstXspChn->stDumpCfg.u32DumpDp, pstXspChn->stDumpCfg.u32DumpCnt);

    if (bEnable && (u32DumpDp & XSP_DDP_FULL_IN))
    {
        xsp_dump_current_full_corr(u32Chan);
    }

    if (bEnable && (u32DumpDp & XSP_DDP_ZERO_IN))
    {
        xsp_dump_current_zero_corr(u32Chan);
    }

    return;
}

/**
 * @fn      Xsp_HalModMalloc
 * @brief   XSP模块软件算法内存申请
 * @note    该接口申请的内存均为Cache的，使用硬件处理数据时需非Cache内存，不用该接口
 *
 * @param   [IN] uiBufSize 内存大小
 * @param   [IN] alineBytes 对齐，单位：字节
 *
 * @return  void* 申请内存地址
 */
static void *Xsp_HalModMalloc(UINT32 uiBufSize, UINT32 alineBytes, CHAR *pcMemName)
{
    SAL_STATUS ret_val = SAL_SOK;
    UINT64 u64PhysicalAddr = 0;
    UINT64 *pu64VirAddr = NULL;

    ret_val = mem_hal_mmzAlloc(uiBufSize, "xsp", pcMemName, NULL, SAL_TRUE, &u64PhysicalAddr, (VOID **)&pu64VirAddr);
    if (SAL_SOK != ret_val || NULL == pu64VirAddr)
    {
        XSP_LOGE("mem_hal_mmzAlloc failed.\n");
        return NULL;
    }

    return pu64VirAddr;
}

/**
 * @function    Xsp_HalModFree
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void Xsp_HalModFree(void *pPtr, CHAR *pcMemName)
{
    mem_hal_mmzFree(pPtr, "xsp", pcMemName);
}

/**
 * @function:   Xsp_DrvGetCommon
 * @brief:     通用参数获取
 * @param[in]:  void
 * @param[out]: None
 * @return:     XSP_COMMON* 通用参数指针
 */
XSP_COMMON *Xsp_DrvGetCommon(void)
{
    return &g_xsp_common;
}

/******************************************************************
   Function   tip_cut_raw_out
   Description:截取原始图像数据中对应位置数据
   Input:
   Output:
   Return   OK or ERR Information
 *******************************************************************/
static SAL_STATUS tip_cut_raw_out(IN U08 *p_raw_in, IN U32 raw_bytewidth, IN U32 raw_width, IN U32 raw_height, XSP_RECT *p_crop_rect, OUT U08 *p_tip_out)
{
    U08 *p_raw_h = NULL, *p_out_h = p_tip_out;
    U32 i = 0;
    U32 tip_line_size = p_crop_rect->uiWidth * raw_bytewidth;
    U32 raw_line_size = raw_width * raw_bytewidth;

    if (p_crop_rect->uiX + p_crop_rect->uiWidth > raw_width
        || p_crop_rect->uiY + p_crop_rect->uiHeight > raw_height)
    {
        XSP_LOGE("the crop_rect is invalid: %u + %u > %u || %u + %u > %u\n", \
                 p_crop_rect->uiX, p_crop_rect->uiWidth, raw_width, \
                 p_crop_rect->uiY, p_crop_rect->uiHeight, raw_height);
        return SAL_FAIL;
    }

    p_raw_h = p_raw_in + raw_width * p_crop_rect->uiY * raw_bytewidth; /* 跳过前面Y行高 */
    p_raw_h += p_crop_rect->uiX * raw_bytewidth; /* 跳过前面X列长 */
    for (i = 0; i < p_crop_rect->uiHeight; i++)
    {
        memcpy(p_out_h, p_raw_h, tip_line_size);
        p_raw_h += raw_line_size; /* 跳过一整行 */
        p_out_h += tip_line_size;
    }

    return SAL_SOK;
}

/**
 * @fn      Xsp_DrvGetTipRaw
 * @brief   从包裹RAW数据中扣取TIP
 *
 * @param   chan[IN] 通道号
 * @param   prm[IN/OUT] 结构体XSP_TIP_RAW_CUTOUT，TIP输入输出信息
 *
 * @return SAL_STATUS
 */
SAL_STATUS Xsp_DrvGetTipRaw(UINT32 chan, VOID *prm)
{
    XSP_TIP_RAW_CUTOUT *p_tip_raw = (XSP_TIP_RAW_CUTOUT *)prm;
    /* 已做过几何变换（Geometric transformation）的包裹RAW数据宽、高 */
    UINT32 raw_width_geo_trans = 0, raw_height_geo_trans = 0;
    UINT32 raw_width_original = 0, raw_height_original = 0;
    UINT32 tip_origin_data_size = 0;
    UINT32 left_padding = 0;
    UINT32 u32DispZoomRatioWidth = 0;
    UINT32 u32DispZoomRatioHeight = 0;
    UINT8 *p_tip_buf = NULL;
    CHAR dumpName[128] = {0};
    void *handle = g_xsp_common.stXspChn[chan].handle;
    SAL_STATUS ret_val = SAL_SOK;
    MIRROR_MODE mirror_type = MIRROR_MODE_VERTICAL;
    ROTATE_MODE rotate_type = ROTATE_MODE_90;
    DISP_CHN_COMMON *pDispChnPrm = NULL;
    DSPINITPARA *pstDspInitPrm = SystemPrm_getDspInitPara();
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();

    if ((NULL == pstCapbDisp) || (NULL == pstCapbXrayIn))
    {
        XSP_LOGE("pstCapbDisp[%p] or pstCapXrayIn[%p] is error !\n",
                 pstCapbDisp,
                 pstCapbXrayIn);
        return SAL_FAIL;
    }

    XSP_RECT tip_crop_rect = {0}; /* YUV上TIP抠图的抠图坐标，从显示坐标上根据缩放比例得到 */
    XSP_RECT raw_rect_geo_trans = {0}; /* 已做过几何变换（Geometric transformation），相对于显示包裹左上角的坐标 */
    XSP_RECT raw_rect_original = {0}; /* 相对于原始未做过几何变换的RAW图像（p_tip_raw->p_raw_buf）坐标 */
    XSP_RECT raw_rect_tmp = {0};

    if (NULL == p_tip_raw)
    {
        XSP_LOGE("the 'p_tip_raw' is NULL\n");
        return SAL_FAIL;
    }

    pDispChnPrm = disp_tskGetVoChn(chan, 0); /* 获取显示窗口参数，默认使用0窗口来显示安检机画面 */
    if (pDispChnPrm == NULL)
    {
        XSP_LOGE("voDev %d chn %d pDispChn null err\n", chan, 0);
        return SAL_FAIL;
    }

    u32DispZoomRatioWidth = (UINT32)((pDispChnPrm->uiW << 15) / pstCapbDisp->disp_yuv_w_max);  /*最终显示图像的横向缩放比例，缩放的目的是使显示包裹与实际尺寸比例保持一致，Q15格式 */
    if (ISM_DISP_MODE_DOUBLE_UPDOWN == pstDspInitPrm->dspCapbPar.ism_disp_mode)
    {
        u32DispZoomRatioHeight = (UINT32)((pDispChnPrm->uiH << 15) / (pstCapbDisp->disp_yuv_h - pstCapbDisp->disp_h_top_padding - pstCapbDisp->disp_h_bottom_padding));
    }
    else
    {
        u32DispZoomRatioHeight = (UINT32)((pDispChnPrm->uiH << 15) / pstCapbDisp->disp_yuv_h);
    }

    XSP_LOGW("before tip cutout, %p, w%u, h%u, (%u, %u)-[%u, %u] ratio[%d %d]\n", \
             p_tip_raw->p_raw_buf, p_tip_raw->package_width, p_tip_raw->package_height, p_tip_raw->tip_rect.uiX, p_tip_raw->tip_rect.uiY,
             p_tip_raw->tip_rect.uiWidth, p_tip_raw->tip_rect.uiHeight, u32DispZoomRatioWidth, u32DispZoomRatioHeight);

    if (g_xsp_common.stXspChn[0].stDumpCfg.u32DumpCnt > 0)
    {
        if (g_xsp_common.stXspChn[0].stDumpCfg.u32DumpDp & XSP_DDP_XRAW_IN)
        {
            snprintf(dumpName, sizeof(dumpName), "%s/tipin_t%llu_w%u_h%u.nraw", g_xsp_common.stXspChn[0].stDumpCfg.chDumpDir,
                     sal_get_tickcnt(), p_tip_raw->package_width, p_tip_raw->package_height);
            SAL_WriteToFile(dumpName, 0, SEEK_SET, p_tip_raw->p_raw_buf,
                            p_tip_raw->package_width * p_tip_raw->package_height * pstCapbXsp->xsp_normalized_raw_bw);
        }
    }

    raw_width_geo_trans = p_tip_raw->package_height;
    raw_height_geo_trans = p_tip_raw->package_width;
    raw_width_original = p_tip_raw->package_width;
    raw_height_original = p_tip_raw->package_height;

    #if 0 /* 这里面计算的tip_rect是相对于包裹的显示坐标 */
    /* 计算缩放前的且已做过几何变换的RAW数据坐标 */
    raw_rect_geo_trans.uiX = (p_tip_raw->tip_rect.uiX << 10) / g_xsp_prm_cap.stYuvAttr.disp_w_zoom_ratio;
    raw_rect_geo_trans.uiY = (p_tip_raw->tip_rect.uiY << 10) / g_xsp_prm_cap.stYuvAttr.disp_h_zoom_ratio;
    raw_rect_geo_trans.uiWidth = (p_tip_raw->tip_rect.uiWidth << 10) / g_xsp_prm_cap.stYuvAttr.disp_w_zoom_ratio;
    raw_rect_geo_trans.uiHeight = (p_tip_raw->tip_rect.uiHeight << 10) / g_xsp_prm_cap.stYuvAttr.disp_h_zoom_ratio;
    #else /* 这里面计算的tip_rect是相对于整屏显示的坐标 */
    /* 先做坐标缩放，从显示坐标转换为原始YUV坐标 */
    tip_crop_rect.uiX = (p_tip_raw->tip_rect.uiX << 15) / u32DispZoomRatioWidth;
    tip_crop_rect.uiY = (p_tip_raw->tip_rect.uiY << 15) / u32DispZoomRatioHeight;
    tip_crop_rect.uiWidth = (p_tip_raw->tip_rect.uiWidth << 15) / u32DispZoomRatioWidth;
    tip_crop_rect.uiHeight = (p_tip_raw->tip_rect.uiHeight << 15) / u32DispZoomRatioHeight;

    /* 再计算相对于显示包裹左上角的坐标 */
    left_padding = SAL_SUB_SAFE(pstCapbDisp->disp_yuv_w_max, raw_width_geo_trans) >> 1; /* 左填白区域 */
    raw_rect_geo_trans.uiX = SAL_SUB_SAFE(tip_crop_rect.uiX, left_padding); /* 原始图像上框的横向坐标 */
    if (ISM_DISP_MODE_DOUBLE_UPDOWN == pstDspInitPrm->dspCapbPar.ism_disp_mode)
    {
        raw_rect_geo_trans.uiY = tip_crop_rect.uiY;
    }
    else
    {
        raw_rect_geo_trans.uiY = SAL_SUB_SAFE(tip_crop_rect.uiY, pstCapbDisp->disp_h_top_padding);
    }

    raw_rect_geo_trans.uiWidth = tip_crop_rect.uiWidth;
    raw_rect_geo_trans.uiHeight = tip_crop_rect.uiHeight;
    #endif
    /*判断截取数据是否超过原数据边界*/
    if (raw_rect_geo_trans.uiX + raw_rect_geo_trans.uiWidth > raw_width_geo_trans
        || raw_rect_geo_trans.uiY + raw_rect_geo_trans.uiHeight > raw_height_geo_trans)
    {
        XSP_LOGE("the tip rectangle is invalid, left pad %d, [%u %u %u %u] -> [%u %u %u %u] -> [%u %u %u %u]\n", left_padding,
                 tip_crop_rect.uiX, tip_crop_rect.uiY, tip_crop_rect.uiWidth, tip_crop_rect.uiHeight,
                 p_tip_raw->tip_rect.uiX, p_tip_raw->tip_rect.uiY, p_tip_raw->tip_rect.uiWidth, p_tip_raw->tip_rect.uiHeight,
                 raw_rect_geo_trans.uiX, raw_rect_geo_trans.uiY, raw_rect_geo_trans.uiWidth, raw_rect_geo_trans.uiHeight);
        return SAL_FAIL;
    }

    /**
     * L2R，旋转90°的逆变换：旋转270°
     * R2L，旋转270°+垂直镜像的逆变换：垂直镜像+旋转90°
     * 注意几何变换的顺序：如果成像时先做旋转再做镜像，则逆操作需要先做镜像再做旋转
     */
    /* 先做逆镜像 */
    g_xsp_lib_api.get_mirror(handle, (U32 *)&mirror_type);
    convert_rect_region_mirror(raw_width_geo_trans, raw_height_geo_trans, mirror_type, &raw_rect_geo_trans, &raw_rect_tmp);

    /* 再做逆旋转 */
    g_xsp_lib_api.get_rotate(handle, (U32 *)&rotate_type);
    convert_rect_region_rotate(raw_width_geo_trans, raw_height_geo_trans, ROTATE_MODE_COUNT - rotate_type, &raw_rect_tmp, &raw_rect_original);

    XSP_LOGI("mirror: %u, rotate: %u, raw_rect_geo_trans: (%u, %u)-[%u, %u], raw_rect_original: (%u, %u)-[%u, %u]\n",
             mirror_type, rotate_type, raw_rect_geo_trans.uiX, raw_rect_geo_trans.uiY, raw_rect_geo_trans.uiWidth, raw_rect_geo_trans.uiHeight, \
             raw_rect_original.uiX, raw_rect_original.uiY, raw_rect_original.uiWidth, raw_rect_original.uiHeight);

    /* 宽高4字节对齐 */
    raw_rect_original.uiWidth = SAL_align(raw_rect_original.uiWidth, 4);
    raw_rect_original.uiHeight = SAL_align(raw_rect_original.uiHeight, 4);
    if (raw_rect_original.uiX + raw_rect_original.uiWidth > raw_width_original)
    {
        raw_rect_original.uiWidth -= 4;
    }

    if (raw_rect_original.uiY + raw_rect_original.uiHeight > raw_height_original)
    {
        raw_rect_original.uiHeight -= 4;
    }

    /*转换后的宽高回传给应用需要原始尺寸*/
    p_tip_raw->tip_raw_width = (UINT32)((FLOAT32)raw_rect_original.uiWidth / pstCapbXsp->resize_height_factor);
    p_tip_raw->tip_raw_height = (UINT32)((FLOAT32)raw_rect_original.uiHeight / pstCapbXsp->resize_height_factor);
    p_tip_raw->tip_data_size = p_tip_raw->tip_raw_width * p_tip_raw->tip_raw_height * pstCapbXsp->xsp_normalized_raw_bw;

    if (XRAY_ENERGY_SINGLE == pstCapbXrayIn->xray_energy_num) /* 单能 */
    {
        tip_origin_data_size = raw_rect_original.uiWidth * raw_rect_original.uiHeight * 2;
        p_tip_buf = (UINT8 *)Xsp_HalModMalloc(tip_origin_data_size, 16, "getTipRaw");
        if (NULL != p_tip_buf)
        {
            ret_val = tip_cut_raw_out(p_tip_raw->p_raw_buf, pstCapbXrayIn->xraw_bytewidth,
                                      raw_width_original, raw_height_original, &raw_rect_original, p_tip_buf);
        }
        else
        {
            XSP_LOGE("malloc for 'p_tip_buf' failed, buffer size: %u\n", tip_origin_data_size);
            ret_val = SAL_FAIL;
        }
    }
    else if (XRAY_ENERGY_DUAL == pstCapbXrayIn->xray_energy_num) /* 双能 */
    {
        tip_origin_data_size = raw_rect_original.uiWidth * raw_rect_original.uiHeight * 5;
        p_tip_buf = (UINT8 *)Xsp_HalModMalloc(tip_origin_data_size, 16, "getTipRaw");
        if (NULL != p_tip_buf)
        {
            /* 低能 */
            ret_val = tip_cut_raw_out(XRAW_LE_OFFSET(p_tip_raw->p_raw_buf, raw_width_original, raw_height_original),
                                      pstCapbXrayIn->xraw_bytewidth, raw_width_original, raw_height_original, &raw_rect_original,
                                      XRAW_LE_OFFSET(p_tip_buf, raw_rect_original.uiWidth, raw_rect_original.uiHeight));
            /* 高能 */
            ret_val |= tip_cut_raw_out(XRAW_HE_OFFSET(p_tip_raw->p_raw_buf, raw_width_original, raw_height_original),
                                       pstCapbXrayIn->xraw_bytewidth, raw_width_original, raw_height_original, &raw_rect_original,
                                       XRAW_HE_OFFSET(p_tip_buf, raw_rect_original.uiWidth, raw_rect_original.uiHeight));
            /* 原子序数 */
            ret_val |= tip_cut_raw_out(XRAW_Z_OFFSET(p_tip_raw->p_raw_buf, raw_width_original, raw_height_original),
                                       sizeof(UINT8), raw_width_original, raw_height_original, &raw_rect_original,
                                       XRAW_Z_OFFSET(p_tip_buf, raw_rect_original.uiWidth, raw_rect_original.uiHeight));
        }
        else
        {
            XSP_LOGE("malloc for 'p_tip_buf' failed, buffer size: %u\n", p_tip_raw->tip_data_size);
            ret_val = SAL_FAIL;
        }
    }

    if (SAL_SOK != ret_val)
    {
        XSP_LOGE("GetTipRaw failed ret[%d].\n", ret_val);
        goto EXIT;
    }

    if (pstCapbXsp->resize_height_factor == 1 && pstCapbXsp->resize_width_factor == 1)
    {
        memcpy(p_tip_raw->p_raw_buf, p_tip_buf, p_tip_raw->tip_data_size);
    }
    else
    {
        ret_val = xsp_xraw_resize(DSP_IMG_DATFMT_LHZP, p_tip_raw->p_raw_buf, p_tip_raw->tip_raw_width,
                                  p_tip_raw->tip_raw_height, p_tip_buf, raw_rect_original.uiWidth, raw_rect_original.uiHeight);
        if (SAL_SOK != ret_val)
        {
            XSP_LOGE("xsp_xraw_resize failed ret:%d.\n", ret_val);
            goto EXIT;
        }
    }

    XSP_LOGI("tip cutout successfully, w: %u, h: %u, s: %u, raw-original: (%u, %u)-[%u, %u]\n", \
             p_tip_raw->tip_raw_width, p_tip_raw->tip_raw_height, p_tip_raw->tip_data_size, \
             raw_rect_original.uiX, raw_rect_original.uiY, raw_rect_original.uiWidth, raw_rect_original.uiHeight);

    if (g_xsp_common.stXspChn[0].stDumpCfg.u32DumpCnt > 0)
    {
        if (g_xsp_common.stXspChn[0].stDumpCfg.u32DumpDp & XSP_DDP_XRAW_OUT)
        {
            snprintf(dumpName, sizeof(dumpName), "%s/tipout_t%llu_w%u_h%u_(%u,%u).nraw", g_xsp_common.stXspChn[0].stDumpCfg.chDumpDir,
                     sal_get_tickcnt(), p_tip_raw->tip_rect.uiHeight, p_tip_raw->tip_rect.uiWidth, raw_rect_original.uiX, raw_rect_original.uiY);
            SAL_WriteToFile(dumpName, 0, SEEK_SET, p_tip_raw->p_raw_buf, p_tip_raw->tip_data_size);
            snprintf(dumpName, sizeof(dumpName), "%s/tip_sup_t%llu__(%u,%u)-w%u_h%u.nraw", g_xsp_common.stXspChn[0].stDumpCfg.chDumpDir,
                     sal_get_tickcnt(), raw_rect_original.uiX, raw_rect_original.uiY, raw_rect_original.uiWidth, raw_rect_original.uiHeight);
            SAL_WriteToFile(dumpName, 0, SEEK_SET, p_tip_buf, raw_rect_original.uiWidth * raw_rect_original.uiHeight * pstCapbXsp->xsp_normalized_raw_bw);
        }
    }

EXIT:
    if (NULL != p_tip_buf)
    {
        Xsp_HalModFree(p_tip_buf, "getTipRaw");
    }

    return ret_val;
}

/**
 * @fn      Xsp_DrvFbTipResult
 * @brief   返回TIP插入状态给应用
 *
 * @param   [IN] chan 通道号
 * @param   [IN] enTipedStatus TIP插入状态
 */
static void Xsp_DrvFbTipResult(UINT32 chan, XSP_TIPED_STATUS enTipedStatus)
{
    XSP_TIP_RESULT stTipResult = {0};
    STREAM_ELEMENT stStreamEle = {0};
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan(%u) is invalid, range: [0, %u]\n", chan, pstCapXrayIn->xray_in_chan_cnt - 1);
        return;
    }

    if (XSP_TIPED_SUCESS == enTipedStatus)
    {
        stTipResult.result = SAL_TRUE;
    }
    else
    {
        stTipResult.result = SAL_FALSE;
    }

    stStreamEle.type = STREAM_ELEMENT_TIP;
    stStreamEle.chan = chan;
    XSP_LOGI("chan %d, feedback tip result %d, status %d\n", chan, stTipResult.result, enTipedStatus); /* 先打印信息后回调 */
    SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stTipResult, sizeof(XSP_TIP_RESULT));

    return;
}

/**
 * @function   Xsp_DrvTipReady
 * @brief      TIP数据准备开始注入
 * @param[in]  UINT32 chan 算法通道号
 * @param[in]  XSP_XRAY_DATA_OUT *pstDataOut 输出缓冲区数据
 * @param[in]  XSP_RTPROC_OUT *pstRtProcOut 成像算法处理的输出数据
 * @return     static INT32 成功SAL_SOK，失败SAL_FALSE
 */
static INT32 Xsp_DrvTipReady(UINT32 chan, XSP_RTPROC_OUT *pstRtProcOut, UINT32 u32RtSliceNo)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT32 uiOtherChan = 0;
    UINT32 uiPackCurLine = 0;
    XSP_RECT *pstPackRect = NULL;
    XSP_TIP_DATA_ST *pstTipData = NULL;
    XSP_TIP_CTRL_ST *pstTipCtrl = NULL;
    XSP_TIP_PROCESS *pstTipProcess = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    CAPB_XSP *pstCapXsp = capb_get_xsp();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan(%u) is invalid, range: [0, %u]\n", chan, pstCapXrayIn->xray_in_chan_cnt - 1);
        return SAL_FAIL;
    }

    if (NULL == pstRtProcOut)
    {
        XSP_LOGE("chan %u, the pstPackRect is NULL !\n", chan);
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    pstTipProcess = &g_xsp_common.stXspChn[chan].stTipProcess;
    pstTipData = &g_xsp_common.stTipAttr.stTipData;
    pstTipCtrl = &g_xsp_common.stTipAttr.stTipCtrl[chan];
    uiPackCurLine = g_xsp_common.stXspChn[chan].stRtPackDivS.uiPackCurLine;
    pstPackRect = &pstRtProcOut->st_package_indentify.package_rect[0];

    XSP_LOGT("Tip ready: chan %d, curline %d, x %d, y %d, w %d, h %d, uiPackScan %d!\n",
             chan,
             uiPackCurLine,
             pstPackRect->uiX,
             pstPackRect->uiY,
             pstPackRect->uiWidth,
             pstPackRect->uiHeight,
             pstTipData->uiPackScan);

    /*数据是否更新，没有直接返回成功*/
    SAL_mutexLock(pstTipCtrl->pDataMutex);
    if (SAL_TRUE != pstTipCtrl->uiUpdate)
    {
        SAL_mutexUnlock(pstTipCtrl->pDataMutex);
        s32Ret = SAL_SOK;
        goto EXIT;
    }

    SAL_mutexUnlock(pstTipCtrl->pDataMutex);

    /*校验Xray高度范围*/
    if ((pstTipData->stTipNormalized[chan].uiXrayH >= pstCapXsp->xraw_height_resized)
        || (pstTipData->stTipNormalized[chan].uiXrayH == 0))
    {
        XSP_LOGE("Tip chan %d height [%d] is error !\n", chan, pstTipData->stTipNormalized[chan].uiXrayH);
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    /*校验Xray宽度范围*/
    if ((pstTipData->stTipNormalized[chan].uiXrayW >= pstCapXsp->xraw_width_resized)
        || (pstTipData->stTipNormalized[chan].uiXrayW == 0))
    {
        XSP_LOGE("Tip chan %d width [%d] is error !\n", chan, pstTipData->stTipNormalized[chan].uiXrayW);
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    /*注入超时*/
    if (pstTipCtrl->u32SetTime + XSP_TIP_OVERTIME < SAL_getCurMs())
    {
        XSP_LOGE("Tip chan %d: tip insert time out, set time %d, overtime %d, cur time %d!\n",
                 chan,
                 pstTipCtrl->u32SetTime,
                 XSP_TIP_OVERTIME,
                 SAL_getCurMs());

        #if 0 /*超时判断*/
        s32Ret = SAL_FAIL;
        goto EXIT;
        #endif
    }
    else
    {
        XSP_LOGI("chan %u, tip time out, because set time %d + overtime %d = %d, cur time %d\n",
                 chan, pstTipCtrl->u32SetTime, XSP_TIP_OVERTIME,
                 pstTipCtrl->u32SetTime + XSP_TIP_OVERTIME, SAL_getCurMs());
    }

    /*Tip高度，与包裹剩余多少列数比较*/
    if ((pstTipData->uiPackScan - uiPackCurLine) + XSP_TIP_COLUMN_OFFSET < (U32)SAL_DIV_SAFE(pstTipData->stTipNormalized[chan].uiXrayH, pstCapXsp->resize_width_factor))
    {
        XSP_LOGE("Tip chan %d uiPackScan(%d) - uiPackCurLine(%d) + xsp offset(%d) < Tip yuv W(%d/%.3f), return!\n",
                 chan,
                 pstTipData->uiPackScan,
                 uiPackCurLine,
                 XSP_TIP_COLUMN_OFFSET,
                 pstTipData->stTipNormalized[chan].uiXrayH,
                 pstCapXsp->resize_width_factor);
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    /*Tip宽度，未符合条件直接返回，即注入时机的判断*/
    if (pstPackRect->uiWidth < pstTipData->stTipNormalized[chan].uiXrayW)
    {
        return SAL_SOK;
    }

    /*注入的位置，相对左上角，包裹尺寸中间*/
    pstTipProcess->uiCol = pstPackRect->uiX + ((pstPackRect->uiWidth - pstTipData->stTipNormalized[chan].uiXrayW) >> 1);

    if (pstTipProcess->uiCol == 0 || pstTipProcess->uiCol >= pstCapXsp->xraw_width_resized)
    {
        XSP_LOGE("chan %u, uiCol(%d) is error, x %u, w %u !\n", chan, pstTipProcess->uiCol, pstPackRect->uiX, pstPackRect->uiWidth);
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    /*单视角*/
    if (XSP_VANGLE_SINGLE == pstCapXrayIn->xray_in_chan_cnt)
    {
        pstTipProcess->uiEnable = SAL_TRUE;

        XSP_LOGI("Tip [chan %d]: PackRect uiX %d, uiW %d, cur line %d, Tip W %d, H %d, Enable %d, uiCol %d, sliceNo. %d !\n",
                 chan,
                 pstPackRect->uiX,
                 pstPackRect->uiWidth,
                 uiPackCurLine,
                 pstTipData->stTipNormalized[chan].uiXrayW,
                 pstTipData->stTipNormalized[chan].uiXrayH,
                 pstTipProcess->uiEnable,
                 pstTipProcess->uiCol,
                 u32RtSliceNo);

        /*数据清空*/
        pstTipCtrl->uiReady = SAL_FALSE;
        pstTipCtrl->uiUpdate = SAL_FALSE;
    }
    /*双视角*/
    else if (XSP_VANGLE_DUAL == pstCapXrayIn->xray_in_chan_cnt)
    {
        pstTipCtrl->uiReady = SAL_TRUE;
        XSP_LOGI("Tip[chan %d]: PackRect uiX %d, uiW %d, cur line %d, Tip W %d, H %d, Enable %d, uiCol %d!\n",
                 chan,
                 pstPackRect->uiX,
                 pstPackRect->uiWidth,
                 uiPackCurLine,
                 pstTipData->stTipNormalized[chan].uiXrayW,
                 pstTipData->stTipNormalized[chan].uiXrayH,
                 pstTipProcess->uiEnable,
                 pstTipProcess->uiCol);

        /************双视角同步****************/

        /*另一通道准备就绪, 则开始注入，uiEnable置1*/
        uiOtherChan = (chan + 1) % 2;
        if (SAL_TRUE == g_xsp_common.stTipAttr.stTipCtrl[uiOtherChan].uiReady)
        {
            for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
            {
                g_xsp_common.stXspChn[i].stTipProcess.uiEnable = SAL_TRUE;
                XSP_LOGW("Tip enable: chan %d, uiEnable %d, col %d sliceNo. %d\n",
                         i, g_xsp_common.stXspChn[i].stTipProcess.uiEnable, g_xsp_common.stXspChn[i].stTipProcess.uiCol, u32RtSliceNo);
                g_xsp_common.stTipAttr.stTipCtrl[i].uiReady = SAL_FALSE;
                g_xsp_common.stTipAttr.stTipCtrl[i].uiUpdate = SAL_FALSE;
            }
        }
    }

    return SAL_SOK;

EXIT:

    if (SAL_SOK != s32Ret)
    {
        for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
        {
            SAL_mutexLock(g_xsp_common.stTipAttr.stTipCtrl[i].pDataMutex);
            g_xsp_common.stTipAttr.stTipCtrl[i].uiUpdate = SAL_FALSE;
            SAL_mutexUnlock(g_xsp_common.stTipAttr.stTipCtrl[i].pDataMutex);
            g_xsp_common.stTipAttr.stTipCtrl[i].uiReady = SAL_FALSE;
        }

        (void) Xsp_DrvFbTipResult(0, XSP_TIPED_FAIL);
    }

    return s32Ret;
}

/**
 * @function   Xsp_DrvTipOsd
 * @brief      TIP的osd叠加坐标计算
 * @param[in]  UINT32 chan 算法通道号
 * @param[in]  XSP_DATA_NODE *pstDataNode 叠加tip数据
 * @return     static INT32 成功SAL_SOK，失败SAL_FALSE
 */
static INT32 Xsp_DrvTipOsd(UINT32 chan, XSP_DATA_NODE *pstDataNode)
{
    UINT32 uiTipYuvH = 0;
    UINT32 uiTipYuvW = 0;
    UINT32 uiXrayYuvW = 0;
    UINT32 mirror_type = 0;

    void *pHandle = NULL;
    XSP_TIP_PROCESS *pstTipProcess = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    CAPB_DISP *pstCapDisp = capb_get_disp();
    DSPINITPARA *pstInit = SystemPrm_getDspInitPara();

    if ((NULL == pstCapXrayIn) || (NULL == pstCapDisp) || (NULL == pstDataNode))
    {
        XSP_LOGE("chan %u, pstCapXrayIn [%p] OR pstCapDisp [%p] OR pstDataNode[%p] is NULL\n",
                 chan, pstCapXrayIn, pstCapDisp, pstDataNode);
        goto EXIT;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("uiChn [%d] is error , xray_in_chan_cnt is %d!\n", chan, pstCapXrayIn->xray_in_chan_cnt);
        goto EXIT;
    }

    pHandle = g_xsp_common.stXspChn[chan].handle;
    if (NULL == pHandle)
    {
        XSP_LOGE("uiChn %d, handle [%p] is errord!\n", chan, pHandle);
        goto EXIT;
    }

    pstTipProcess = &g_xsp_common.stXspChn[chan].stTipProcess;

    /*yuv转置*/
    uiTipYuvH = g_xsp_common.stTipAttr.stTipData.stTipNormalized[chan].uiXrayW;
    uiTipYuvW = g_xsp_common.stTipAttr.stTipData.stTipNormalized[chan].uiXrayH;
    uiXrayYuvW = pstDataNode->stXRawInBuf.u32Height * pstInit->dspCapbPar.xsp_resize_factor.resize_height_factor;
    pstDataNode->stTipOsd.uiEnable = SAL_TRUE;
    pstDataNode->stTipOsd.uiDelayTime = g_xsp_common.stTipAttr.stTipData.uiTime;
    pstDataNode->stTipOsd.uiColor = g_xsp_common.stTipAttr.stTipData.uiColor;
    pstDataNode->stTipOsd.stTipResult.uiWidth = uiTipYuvW;
    pstDataNode->stTipOsd.stTipResult.uiHeight = uiTipYuvH;
    g_xsp_lib_api.get_mirror(pHandle, &mirror_type);

    XSP_LOGI("chan %d: uiHold %d, uiAsySlice %d\n",
             chan, g_xsp_common.stTipAttr.stTipCtrl[chan].uiHold, g_xsp_common.stTipAttr.stTipCtrl[chan].uiAsySlice);

    // TIP注入成功后，算法要在下一个条带才置pstRtProcOut->st_xraw_tiped.en_tiped_status为XSP_TIPED_SUCESS，补偿一个条带宽度
    // 记录含TIP的条带宽度，反推TIP注入左上角坐标（TIP必定从开始条带的第一列像素开始注入）
    // 受叠框策略和TIP延时显示影响，出图方向向右的横坐标必须在叠框获取图像数据时候转换
    pstDataNode->stTipOsd.stTipResult.uiX = SAL_align(uiTipYuvW + uiXrayYuvW, uiXrayYuvW);

    /*出图方向、垂直镜像影响纵坐标*/
    if (MIRROR_MODE_VERTICAL == mirror_type)
    {
        if (XRAY_DIRECTION_RIGHT == pstDataNode->enDispDir)
        {
            pstDataNode->stTipOsd.stTipResult.uiY = pstCapDisp->disp_h_middle_indeed - pstTipProcess->uiCol - uiTipYuvH/2; // 计算镜像后的TIP坐标
        }
        else if( XRAY_DIRECTION_LEFT == pstDataNode->enDispDir )
        {
            pstDataNode->stTipOsd.stTipResult.uiY = pstTipProcess->uiCol + uiTipYuvH/2 ; // 计算镜像后的TIP坐标
        }
        
    }
    else /* 其它清空 */
    {
        if (XRAY_DIRECTION_RIGHT == pstDataNode->enDispDir)
        {
            pstDataNode->stTipOsd.stTipResult.uiY = pstTipProcess->uiCol + uiTipYuvH/2; // 计算镜像后的TIP坐标
        }
        else if( XRAY_DIRECTION_LEFT == pstDataNode->enDispDir )
        {
            pstDataNode->stTipOsd.stTipResult.uiY = pstCapDisp->disp_h_middle_indeed - pstTipProcess->uiCol - uiTipYuvH/2;  // 计算镜像后的TIP坐标
        }
        else
        {
            XSP_LOGE("chan %u, uiDirection %d is error\n", chan, pstDataNode->enDispDir);
            goto EXIT;
        }

        XSP_LOGI("chan %d, mirror_type %d, pstTipProcess->uiCol %d, pstCapDisp->disp_h_middle_indeed %d, uiTipYuvH %d, uiY %d\n",
                 chan, mirror_type, pstTipProcess->uiCol, pstCapDisp->disp_h_middle_indeed, uiTipYuvH, pstDataNode->stTipOsd.stTipResult.uiY);
    }

    return SAL_SOK;

EXIT:
    return SAL_FAIL;
}

/**
 * @function   Xsp_DrvTipProcess
 * @brief      TIP的结果上报，叠框流程
 * @param[in]  UINT32 u32Chn 算法通道号
 * @param[in]  XSP_RTPROC_OUT pstRtProcOut 成像算法处理的输出结构体
 * @param[in]  XSP_DATA_NODE *pstDataNode 输入缓冲区数据
 * @return     static INT32 成功SAL_SOK，失败SAL_FALSE
 */
static INT32 Xsp_DrvTipProcess(UINT32 uiChn, XSP_DATA_NODE *pstDataNode, XSP_RTPROC_OUT *pstRtProcOut)
{
    UINT32 uiOtherChn = 0;
    UINT32 i = 0;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if ((NULL == pstDataNode) || (NULL == pstRtProcOut))
    {
        XSP_LOGE("pstDataNode[%p] OR RtProcOut[%p] is NULL !\n", pstDataNode, pstRtProcOut);
        return SAL_FAIL;
    }

    if (uiChn >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan(%u) is invalid, range: [0, %u]\n", uiChn, pstCapXrayIn->xray_in_chan_cnt - 1);
        return SAL_FAIL;
    }

    /********注入判断***********/
    if (XSP_PACKAGE_MIDDLE == pstDataNode->stPackDivRes[0].enPackageTag) /* 当前条带是包裹中间段时，发起TIP操作 */
    {
        Xsp_DrvTipReady(uiChn, pstRtProcOut, pstDataNode->u32RtSliceNo);
    }

    /*研究院算法col参数xsp给定，tiped_pos参数一直 < 0，自研算法col参数其内部给定*/
    if (pstRtProcOut->st_xraw_tiped.tiped_pos >= 0)
    {
        g_xsp_common.stXspChn[uiChn].stTipProcess.uiCol = pstRtProcOut->st_xraw_tiped.tiped_pos; /* 自研算法TIPy坐标 */
    }

    /********上报、叠框***********/

    if (XSP_VANGLE_SINGLE == pstCapXrayIn->xray_in_chan_cnt)
    {
        if ((XSP_TIPED_SUCESS == pstRtProcOut->st_xraw_tiped.en_tiped_status)
            || (XSP_TIPED_FAIL == pstRtProcOut->st_xraw_tiped.en_tiped_status))
        {
            (void) Xsp_DrvFbTipResult(uiChn, pstRtProcOut->st_xraw_tiped.en_tiped_status);
        }

        /*1. 如果注入成功，则进行叠框*/
        if (XSP_TIPED_SUCESS == pstRtProcOut->st_xraw_tiped.en_tiped_status)
        {
            (void) Xsp_DrvTipOsd(uiChn, pstDataNode);
        }
    }
    else if (XSP_VANGLE_DUAL == pstCapXrayIn->xray_in_chan_cnt)
    {
        uiOtherChn = (uiChn + 1) % 2;

        /*保持成功和失败状态*/
        if ((XSP_TIPED_SUCESS == pstRtProcOut->st_xraw_tiped.en_tiped_status)
            || (XSP_TIPED_FAIL == pstRtProcOut->st_xraw_tiped.en_tiped_status))
        {
            g_xsp_common.stTipAttr.stTipCtrl[uiChn].uiHold = pstRtProcOut->st_xraw_tiped.en_tiped_status;
        }

        /*异步叠框*/
        if (XSP_TIPED_SUCESS == pstRtProcOut->st_xraw_tiped.en_tiped_status)
        {
            (void) Xsp_DrvTipOsd(uiChn, pstDataNode);
        }

        if (XSP_TIPED_FAIL == g_xsp_common.stTipAttr.stTipCtrl[uiChn].uiHold)
        {
            if (XSP_TIPED_SUCESS == g_xsp_common.stTipAttr.stTipCtrl[uiOtherChn].uiHold)
            {
                /*一个视角注入成功，则上报成功*/
                (void) Xsp_DrvFbTipResult(uiChn, XSP_TIPED_SUCESS);
                for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
                {
                    g_xsp_common.stTipAttr.stTipCtrl[i].uiHold = XSP_TIPED_NONE;
                    g_xsp_common.stTipAttr.stTipCtrl[i].uiAsySlice = 0;
                    g_xsp_common.stXspChn[i].stTipProcess.uiCol = 0;
                }
            }
            else if (XSP_TIPED_FAIL == g_xsp_common.stTipAttr.stTipCtrl[uiOtherChn].uiHold)
            {
                /*两个通道失败，则上报失败*/
                (void) Xsp_DrvFbTipResult(uiChn, XSP_TIPED_FAIL);
                for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
                {
                    g_xsp_common.stTipAttr.stTipCtrl[i].uiHold = XSP_TIPED_NONE;
                    g_xsp_common.stTipAttr.stTipCtrl[i].uiAsySlice = 0;
                    g_xsp_common.stXspChn[i].stTipProcess.uiCol = 0;
                }
            }
        }
        else if (XSP_TIPED_SUCESS == g_xsp_common.stTipAttr.stTipCtrl[uiChn].uiHold)
        {
            if ((XSP_TIPED_SUCESS == g_xsp_common.stTipAttr.stTipCtrl[uiOtherChn].uiHold)
                || (XSP_TIPED_FAIL == g_xsp_common.stTipAttr.stTipCtrl[uiOtherChn].uiHold))
            {
                /*一个通道成功，则上报成功*/
                (void) Xsp_DrvFbTipResult(uiChn, XSP_TIPED_SUCESS);

                for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
                {
                    g_xsp_common.stTipAttr.stTipCtrl[i].uiHold = XSP_TIPED_NONE;
                    g_xsp_common.stTipAttr.stTipCtrl[i].uiAsySlice = 0;
                    g_xsp_common.stXspChn[i].stTipProcess.uiCol = 0;
                }
            }
        }

        XSP_LOGT("chan %d: status %d, uiHold %d, slice %d\n",
                 uiChn,
                 pstRtProcOut->st_xraw_tiped.en_tiped_status,
                 g_xsp_common.stTipAttr.stTipCtrl[uiChn].uiHold,
                 g_xsp_common.stTipAttr.stTipCtrl[uiChn].uiAsySlice);
    }

    return SAL_SOK;
}

/**
 * @fn      Xsp_DrvSetTipPrm
 * @brief   设置TIP参数，并发起TIP插入操作
 *
 * @param   [NA] chn 该参数不使用
 * @param   [IN] prm TIP参数，结构体XSP_TIP_DATA_ST，多个通道同步设置
 *
 * @return  SAL_STATUS
 */
SAL_STATUS Xsp_DrvSetTipPrm(UINT32 chn, VOID *prm)
{
    UINT32 i = 0;
    CHAR dumpName[128] = {0};
    XSP_TIP_DATA_ST *pstTipData = (XSP_TIP_DATA_ST *)prm;
    XSP_TIP_NORMALIZED *pstTipNraw = NULL;
    XSP_TIP_ATTR *pstTipAttr = &g_xsp_common.stTipAttr;
    CAPB_XSP *pstCapXsp = capb_get_xsp();
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    /************** 输入参数校验开始 **************/
    if (NULL == pstTipData)
    {
        XSP_LOGE("the pstTipData is null!\n");
        return SAL_FAIL;
    }

    if (pstTipData->uiEnable) /* 下面参数只有TIP使能才有效，不使能时无需校验 */
    {
        if ((pstTipData->uiPackScan >= pstCapXsp->xraw_height_resized_max) || (0 == pstTipData->uiPackScan))
        {
            XSP_LOGE("tip uiPackScan(%u) is invalid, range: (0, %u)\n", pstTipData->uiPackScan, pstCapXsp->xsp_package_line_max);
            return SAL_FAIL;
        }

        for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
        {
            pstTipNraw = &pstTipData->stTipNormalized[i];
            if (NULL == pstTipNraw->pXrayBuf)
            {
                XSP_LOGE("chan %d, pXrayBuf is NULL\n", i);
                return SAL_FAIL;
            }

            if ((pstTipNraw->uiXrayH > XSP_TIP_MAX_H) || (0 == pstTipNraw->uiXrayH)
                || (pstTipNraw->uiXrayW > XSP_TIP_MAX_W) || (0 == pstTipNraw->uiXrayW))
            {
                XSP_LOGE("chan %u, tip w(%u) OR h(%u) is out of range, width (0, %d], height (0, %d]\n", i,
                         pstTipData->stTipNormalized[i].uiXrayW, pstTipData->stTipNormalized[i].uiXrayH, XSP_TIP_MAX_W, XSP_TIP_MAX_H);
                return SAL_FAIL;
            }

            if (pstTipNraw->uiBufLen < pstTipNraw->uiXrayW * pstTipNraw->uiXrayH * pstCapXsp->xsp_normalized_raw_bw)
            {
                XSP_LOGE("chan %u, BufLen(%u) is too small, w %u, h %u, should be bigger than %u\n", i, pstTipNraw->uiBufLen,
                         pstTipNraw->uiXrayW, pstTipNraw->uiXrayH, pstTipNraw->uiXrayW * pstTipNraw->uiXrayH * pstCapXsp->xsp_normalized_raw_bw);
                return SAL_FAIL;
            }
        }
    }

    /************** 输入参数校验结束 **************/

    if (pstTipData->uiEnable)
    {
        for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
        {
            pstTipNraw = &pstTipData->stTipNormalized[i];

            SAL_mutexLock(pstTipAttr->stTipCtrl[i].pDataMutex);
            /* TIP参数赋值 */
            pstTipAttr->stTipData.uiTime = pstTipData->uiTime;
            pstTipAttr->stTipData.uiPackScan = pstTipData->uiPackScan;
            pstTipAttr->stTipData.uiColor = pstTipData->uiColor;
            pstTipAttr->stTipData.stTipNormalized[i].uiXrayW = pstTipNraw->uiXrayW;
            pstTipAttr->stTipData.stTipNormalized[i].uiXrayH = pstTipNraw->uiXrayH;
            pstTipAttr->stTipData.stTipNormalized[i].uiBufLen = pstTipNraw->uiBufLen;
            memcpy(pstTipAttr->stTipData.stTipNormalized[i].pXrayBuf, pstTipNraw->pXrayBuf, pstTipNraw->uiBufLen);

            pstTipAttr->stTipCtrl[i].u32SetTime = SAL_getCurMs();
            pstTipAttr->stTipCtrl[i].uiUpdate = SAL_TRUE;
            SAL_mutexUnlock(pstTipAttr->stTipCtrl[i].pDataMutex);

            XSP_LOGI("chan %u, set tip enable, time %llu, PackScan %d, Color %x, W %d, H %d, BufLen %u, BufAddr %p\n",
                     i, pstTipData->uiTime, pstTipData->uiPackScan, pstTipData->uiColor, pstTipNraw->uiXrayW,
                     pstTipNraw->uiXrayH, pstTipNraw->uiBufLen, pstTipNraw->pXrayBuf);

            if (g_xsp_common.stXspChn[i].stDumpCfg.u32DumpCnt > 0)
            {
                if (g_xsp_common.stXspChn[i].stDumpCfg.u32DumpDp & XSP_DDP_XRAW_TIPIN)
                {
                    snprintf(dumpName, sizeof(dumpName), "%s/tipin_ch%u_t%llu_w%u_h%u_l%u.nraw", g_xsp_common.stXspChn[i].stDumpCfg.chDumpDir,
                             i, sal_get_tickcnt(), pstTipNraw->uiXrayW, pstTipNraw->uiXrayH, pstTipNraw->uiBufLen);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pstTipNraw->pXrayBuf, pstTipNraw->uiBufLen);
                }
            }
        }
    }
    else
    {
        /*复位标志*/
        XSP_LOGW("Reset Tip Status\n");
        for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
        {
            SAL_mutexLock(pstTipAttr->stTipCtrl[i].pDataMutex);
            pstTipAttr->stTipCtrl[i].uiUpdate = SAL_FALSE;
            SAL_mutexUnlock(pstTipAttr->stTipCtrl[i].pDataMutex);
        }

        /* 不使能时，直接返回TIP插入失败的标志给应用 */
        Xsp_DrvFbTipResult(0, XSP_TIPED_FAIL);
    }

    return SAL_SOK;
}

/**
 * @fn      xsp_alg_create_rt_pb
 * @brief   创建XSP成像算法句柄，该句柄适用于实时预览和回拉两种功能
 *
 * @param   chan[IN] X-Ray源输入通道号
 *
 * @return  SAL_STATUS SAL_SOK-成功，SAL_FALSE-失败
 */
static SAL_STATUS xsp_alg_create_rt_pb(UINT32 chan)
{
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    XSP_VISUAL_ANGLE enVisualAngle = (1 == chan) ? XSP_VANGLE_SECONDARY : XSP_VANGLE_PRIMARY; /* 通道1为辅视角，0为主视角 */
    DSPINITPARA *pstInitPrm = NULL;
    XRAY_DEV_ATTR stXrayDevAttr = {0};
    XSP_CHN_PRM *pstXspChn = NULL;

    pstInitPrm = SystemPrm_getDspInitPara();
    if (NULL == pstInitPrm)
    {
        XSP_LOGE("chan[%u], pstInitPrm [%p] is error!\n", chan, pstInitPrm);
        return SAL_FAIL;
    }

    if (chan >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("the chan[%u] is invalid, should be less than %u!", chan, pstCapbXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    pstXspChn = &g_xsp_common.stXspChn[chan];

    stXrayDevAttr.ism_dev_type = pstInitPrm->dspCapbPar.ism_dev_type;
    stXrayDevAttr.xray_width_max = pstCapbXrayIn->xraw_width_max;
    stXrayDevAttr.xray_height_max = pstCapbXrayIn->xraw_height_max;
    stXrayDevAttr.xray_slice_height_max = pstCapbXrayIn->line_per_slice_max;
    stXrayDevAttr.xray_slice_resize = pstInitPrm->dspCapbPar.xsp_resize_factor.resize_width_factor;
    stXrayDevAttr.xray_det_vendor = pstInitPrm->dspCapbPar.xray_det_vendor;
    stXrayDevAttr.xray_energy_num = pstInitPrm->dspCapbPar.xray_energy_num;
    stXrayDevAttr.xray_in_chan_cnt = pstCapbXrayIn->xray_in_chan_cnt;
    stXrayDevAttr.visual_angle = enVisualAngle;
    stXrayDevAttr.enXraySource = pstInitPrm->dspCapbPar.aenXraySrcType[chan];
    stXrayDevAttr.enXspLibSrc = pstInitPrm->xspLibSrc;
    stXrayDevAttr.passth_data_size = pstXspChn->stNormalData.u32RtPassthDataSize;
    stXrayDevAttr.rtproc_pipeline_cb = xsp_rtproc_pipeline_cb;

    pstXspChn->raw_cover_left = 0;
    pstXspChn->raw_cover_right = 0;
    pstXspChn->handle = g_xsp_lib_api.create_handle(XSP_HANDLE_RT_PB, &stXrayDevAttr);

    /* handle创建为NULL仍返回OK，确保应用能显示UI显示errno NULL状态下无法过包和转存*/
    XSP_LOGI("chan[%u], HKA %s XSP lib initialized %s, handle: %p\n", \
                chan, (XRAY_ENERGY_SINGLE == pstCapbXrayIn->xray_energy_num) ? "Single-Energy" : "Dual-Energy", \
                (NULL == pstXspChn->handle) ? "unsuccessfully" :"successfully", \
                pstXspChn->handle);
    /* 获取实时预览XSP算法的Z值版本号 */
    g_xsp_lib_api.get_zdata_version(pstXspChn->handle, &pstXspChn->enRtZVer);

    return SAL_SOK;
}

/**
 * @fn      xsp_alg_create_trans
 * @brief   创建XSP成像算法句柄，该句柄适用于转存功能
 *
 * @return  SAL_STATUS SAL_SOK-成功，SAL_FALSE-失败
 */
static SAL_STATUS xsp_alg_create_trans(void)
{
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    DSPINITPARA *pstInitPrm = NULL;
    XRAY_DEV_ATTR stXrayDevAttr = {0};

    pstInitPrm = SystemPrm_getDspInitPara();
    if (NULL == pstInitPrm)
    {
        XSP_LOGE("pstInitPrm [%p] is error!\n", pstInitPrm);
        return SAL_FAIL;
    }

    stXrayDevAttr.ism_dev_type = pstInitPrm->dspCapbPar.ism_dev_type;
    stXrayDevAttr.xray_det_vendor = pstInitPrm->dspCapbPar.xray_det_vendor;
    stXrayDevAttr.xray_energy_num = pstInitPrm->dspCapbPar.xray_energy_num;
    stXrayDevAttr.xray_width_max = pstCapbXrayIn->xraw_width_max;
    stXrayDevAttr.xray_height_max = pstCapbXrayIn->xraw_height_max;
    stXrayDevAttr.xray_slice_height_max = pstCapbXrayIn->line_per_slice_max;
    stXrayDevAttr.xray_slice_resize = pstInitPrm->dspCapbPar.xsp_resize_factor.resize_width_factor;
    stXrayDevAttr.xray_in_chan_cnt = pstCapbXrayIn->xray_in_chan_cnt;
    stXrayDevAttr.visual_angle = 0;
    stXrayDevAttr.enXraySource = pstInitPrm->dspCapbPar.aenXraySrcType[0];
    stXrayDevAttr.enXspLibSrc = pstInitPrm->xspLibSrc;
    stXrayDevAttr.passth_data_size = 0;
    stXrayDevAttr.rtproc_pipeline_cb = NULL;

    gstXspTransProc.pXspHandle = g_xsp_lib_api.create_handle(XSP_HANDLE_TRANS, &stXrayDevAttr);

    /* handle创建为NULL仍返回OK，确保应用能显示UI显示errno */
    XSP_LOGI("%s XSP lib for 'trans' initialized %s, handle: %p\n", \
                (XRAY_ENERGY_SINGLE == pstCapbXrayIn->xray_energy_num) ? "Single-Energy" : "Dual-Energy", \
                (NULL == gstXspTransProc.pXspHandle) ? "unsuccessfully" :"successfully", \
                gstXspTransProc.pXspHandle);

    return SAL_SOK;
}

/**
 * @function   Xsp_DrvReloadZTable
 * @brief      重新加载Z值表
 * @param[in]  UINT32 chan     通道号
 * @param[out] None
 * @return     int             成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Xsp_DrvReloadZTable(UINT32 chan, void *prm)
{
    XRAY_DEV_ATTR stXrayDevAttr = {0};
    DSPINITPARA *pstInitPrm = NULL;
    XRAY_SOURCE_TYPE_E enXraySrcType = 0;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    XSP_VISUAL_ANGLE enVisualAngle = (1 == chan) ? XSP_VANGLE_SECONDARY : XSP_VANGLE_PRIMARY; /* 通道1为辅视角，0为主视角 */

    if (NULL == prm)
    {
        XSP_LOGE("chan[%u], prm is NULL \n", chan);
        return SAL_FAIL;
    }
    pstInitPrm = SystemPrm_getDspInitPara();
    if (NULL == pstInitPrm)
    {
        XSP_LOGE("chan[%u], pstInitPrm [%p] is error!\n", chan, pstInitPrm);
        return SAL_FAIL;
    }
    if (chan >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("the chan[%u] is invalid, range: [0, %u)\n", chan, pstCapbXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    enXraySrcType = *(XRAY_SOURCE_TYPE_E *)prm;

    stXrayDevAttr.ism_dev_type = pstInitPrm->dspCapbPar.ism_dev_type;
    stXrayDevAttr.xray_det_vendor = pstInitPrm->dspCapbPar.xray_det_vendor;
    stXrayDevAttr.xray_energy_num = pstInitPrm->dspCapbPar.xray_energy_num;
    stXrayDevAttr.xray_in_chan_cnt = pstCapbXrayIn->xray_in_chan_cnt;
    stXrayDevAttr.visual_angle = enVisualAngle;
    stXrayDevAttr.enXraySource = enXraySrcType;
    stXrayDevAttr.enXspLibSrc = pstInitPrm->xspLibSrc;

    /* 只需要对RT handle的R值曲线进行reload */
    if (SAL_SOK == g_xsp_lib_api.reload_zTable(g_xsp_common.stXspChn[chan].handle, &stXrayDevAttr))
    {
        XSP_LOGW("chan[%u], XSP lib reload_zTable successfully  ^_^\n", chan);
        return SAL_SOK;
    }
    else
    {
        XSP_LOGE("chan[%u], XSP lib reload_zTable failed !!!\n", chan);
        return SAL_FAIL;
    }
}

/**
 * @fn      xsp_get_corr_full
 * @brief   获取满载校正数据
 *
 * @param   u32Chan[IN] 源通道号
 * @param   pu8CorrBuf[OUT] 校正数据Buffer
 * @param   pu32Size[IN/OUT] 输入：Buffer的大小，输出：校正数据长度
 *
 * @return  SAL_STATUS
 */
SAL_STATUS xsp_get_corr_full(UINT32 u32Chan, UINT8 *pu8CorrBuf, UINT32 *pu32Size)
{
    void *handle = g_xsp_common.stXspChn[u32Chan].handle;

    XSP_CHECK_PTR_IS_NULL(pu8CorrBuf, SAL_FAIL);
    XSP_CHECK_PTR_IS_NULL(pu32Size, SAL_FAIL);

    return xsp_get_correction_data(handle, XSP_CORR_FULLLOAD, (UINT16 *)pu8CorrBuf, pu32Size);
}

/**
 * @fn      xsp_get_corr_zero
 * @brief   获取本底校正数据
 *
 * @param   u32Chan[IN] 源通道号
 * @param   pu8CorrBuf[OUT] 校正数据Buffer
 * @param   pu32Size[IN/OUT] 输入：Buffer的大小，输出：校正数据长度
 *
 * @return  SAL_STATUS
 */
SAL_STATUS xsp_get_corr_zero(UINT32 u32Chan, UINT8 *pu8CorrBuf, UINT32 *pu32Size)
{
    void *handle = g_xsp_common.stXspChn[u32Chan].handle;

    XSP_CHECK_PTR_IS_NULL(pu8CorrBuf, SAL_FAIL);
    XSP_CHECK_PTR_IS_NULL(pu32Size, SAL_FAIL);

    return xsp_get_correction_data(handle, XSP_CORR_EMPTYLOAD, (UINT16 *)pu8CorrBuf, pu32Size);
}

/**
 * @fn      trans2bitstr
 * @brief   将内存数据转换为bit类型的字符串
 *
 * @param   [OUT] pStr 转换后的字符串，从低位到高位依次排列
 * @param   [IN] pData 内存数据
 * @param   [IN] u32BitWidth Bit位宽
 */
void trans2bitstr(CHAR *pStr, void *pData, UINT32 u32BitWidth)
{
    UINT32 i = 0;
    UINT32 *pu32Tmp = NULL;

    if (NULL == pStr || NULL == pData)
    {
        XSP_LOGE("the pStr(%p) OR pData(%p) is NULL\n", pStr, pData);
        return;
    }

    for (i = 0; i < u32BitWidth; i++)
    {
        pu32Tmp = (UINT32 *)pData + i / 32;
        strcat(pStr, (*pu32Tmp & (1 << (i % 32))) ? "1" : "0");
        if (0 == (i + 1) % 32)
        {
            strcat(pStr, "  ");
        }
        else if (0 == (i + 1) % 8)
        {
            strcat(pStr, " ");
        }
    }

    return;
}

/**
 * @function    bitcopy
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void bitcopy(void *pDestData, UINT32 u32DestBitOffs, void *pSrcData, UINT32 u32SrcBitOffs, UINT32 u32CopyBitLen)
{
    UINT32 u32SrcByte = 0, u32SrcBitModByte = 0;
    UINT32 u32DestByte = 0, u32DestBitModByte = 0;
    UINT32 u32BitLenSurplus = u32CopyBitLen;
    UINT8 *pu8Dest = (UINT8 *)pDestData, *pu8Src = (UINT8 *)pSrcData;

    if ((NULL == pSrcData) || (NULL == pDestData))
    {
        return;
    }

    while (u32BitLenSurplus > 0)
    {
        u32DestByte = ((u32DestBitOffs + u32BitLenSurplus) / 8);
        u32DestBitModByte = ((u32DestBitOffs + u32BitLenSurplus) % 8);
        u32SrcByte = ((u32SrcBitOffs + u32BitLenSurplus) / 8);
        u32SrcBitModByte = ((u32SrcBitOffs + u32BitLenSurplus) % 8);

        if (0 == u32SrcBitModByte)
        {
            u32SrcBitModByte = 8;
            u32SrcByte = SAL_SUB_SAFE(u32SrcByte, 1);
        }

        if (0 == u32DestBitModByte)
        {
            u32DestBitModByte = 8;
            u32DestByte = SAL_SUB_SAFE(u32DestByte, 1);
        }

        pu8Dest[u32DestByte] &= ~(1 << (u32DestBitModByte - 1));
        if ((pu8Src[u32SrcByte] & ((UINT8)((UINT8)1 << (u32SrcBitModByte - 1)))) != 0)
        {
            pu8Dest[u32DestByte] |= ((UINT8)1 << (u32DestBitModByte - 1));
        }

        u32BitLenSurplus--;
    }

    return;
}

/**
 * @function   xsp_update_img_proc_param_to_app
 * @brief      更新成像参数给应用
 * @param[in]  UINT32 u32Chn  算法通道号
 * @param[out] None
 * @return
 */
static void xsp_update_img_proc_param_to_app(XSP_RT_PARAMS *pstXspPicPrm)
{
    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();

    if (NULL == pstDspInfo)
    {
        XSP_LOGE("SystemPrm_getDspInitPara failed\n");
        return;
    }

    if (NULL == pstXspPicPrm)
    {
        XSP_LOGE("the pstXspPicPrm is NULL\n");
        return;
    }

    memcpy(&pstDspInfo->stXspPicPrm, pstXspPicPrm, sizeof(XSP_RT_PARAMS));

    return;
}

/**
 * @function    xsp_rtproc_get_pack_edge
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void xsp_rtproc_get_pack_edge(UINT32 u32Chn, XSP_DATA_NODE *pstDataNode, UINT32 u32Idx, XSP_PACKAGE_INDENTIFY *pstPackIndentify)
{
    UINT32 u32YuvSliceTop = 0; /* 条带中的包裹数据的YUV域的上边界*/
    UINT32 u32YuvSliceBottom = 0; /* slice条带中的包裹数据的YUV域的下边界*/
    UINT32 u32ExpandHeight = 0; /* 包裹高小于64时上下边界需要扩长时的高度*/
    XSP_PACK_DIV_STAT *pstRtPackDivS = NULL;
    XIMAGE_DATA_ST *pstSlliceNrawOut = NULL;

    CHECK_PTR_NULL(pstDataNode,);

    pstRtPackDivS = &(g_xsp_common.stXspChn[u32Chn].stRtPackDivS);
    pstSlliceNrawOut = &pstDataNode->stSliceNrawBuf;

    if (XSP_SLICE_PACKAGE == pstDataNode->stPackDivRes[u32Idx].enSliceCont)
    {
        if (SAL_TRUE == g_xsp_common.stUpdatePrm.stBasePrm.stMirrorPrm[u32Chn].bEnable)
        {
            u32YuvSliceTop = SAL_SUB_SAFE(pstSlliceNrawOut->u32Width, pstPackIndentify->package_rect[0].uiX + pstPackIndentify->package_rect[0].uiWidth);
            u32YuvSliceBottom = SAL_SUB_SAFE(pstSlliceNrawOut->u32Width, pstPackIndentify->package_rect[0].uiX);
        }
        else
        {
            u32YuvSliceTop = pstPackIndentify->package_rect[0].uiX;
            u32YuvSliceBottom = u32YuvSliceTop + pstPackIndentify->package_rect[0].uiWidth;
        }

        /* 只有u32YuvSliceTop与u32YuvSliceBottom正确时才使用 */
        if (u32YuvSliceTop < u32YuvSliceBottom && u32YuvSliceBottom <= pstSlliceNrawOut->u32Width)
        {
            if (XSP_PACKAGE_START == pstDataNode->stPackDivRes[u32Idx].enPackageTag)
            {
                pstRtPackDivS->u32CurTop = u32YuvSliceTop;
                pstRtPackDivS->u32CurBottom = u32YuvSliceBottom;
            }
            else if (XSP_PACKAGE_MIDDLE == pstDataNode->stPackDivRes[u32Idx].enPackageTag || XSP_PACKAGE_END == pstDataNode->stPackDivRes[u32Idx].enPackageTag)
            {
                pstRtPackDivS->u32CurTop = SAL_MIN(pstRtPackDivS->u32CurTop, u32YuvSliceTop);
                pstRtPackDivS->u32CurBottom = SAL_MAX(pstRtPackDivS->u32CurBottom, u32YuvSliceBottom);
            }
        }
    }

    /* 赋值到pstDataNode中，并做异常处理，强制为最大包裹上下边界 */
    if ((pstRtPackDivS->u32CurBottom <= pstRtPackDivS->u32CurTop)
        || (pstRtPackDivS->u32CurBottom > pstSlliceNrawOut->u32Width))
    {
        if (XSP_PACKAGE_END == pstDataNode->stPackDivRes[u32Idx].enPackageTag)
        {
            XSP_LOGE("chan %u, u32CurTop[%d] OR u32Bottom[%d] is error!\n",
                     u32Chn, pstRtPackDivS->u32CurTop, pstRtPackDivS->u32CurBottom);
        }

        pstDataNode->stPackDivRes[u32Idx].u32Top = 0;
        pstDataNode->stPackDivRes[u32Idx].u32Bottom = pstSlliceNrawOut->u32Width;
    }
    else
    {
        pstDataNode->stPackDivRes[u32Idx].u32Top = pstRtPackDivS->u32CurTop;
        pstDataNode->stPackDivRes[u32Idx].u32Bottom = pstRtPackDivS->u32CurBottom;
    }

    /* 强制包裹最小高度为64 */
    if (SAL_SUB_SAFE(pstRtPackDivS->u32CurBottom, pstRtPackDivS->u32CurTop) < XSP_PACK_MIN_LINE) 
    {
        u32ExpandHeight = SAL_align(XSP_PACK_MIN_LINE - SAL_SUB_SAFE(pstRtPackDivS->u32CurBottom, pstRtPackDivS->u32CurTop), 2) / 2;

        if(pstRtPackDivS->u32CurTop < u32ExpandHeight)
        {
            /* 高度扩充超出图像上边界，直接取图像顶64行作为包裹图像 */
            pstRtPackDivS->u32CurTop = 0;
            pstRtPackDivS->u32CurBottom = XSP_PACK_MIN_LINE;
        }
        else if (pstRtPackDivS->u32CurBottom + u32ExpandHeight > pstSlliceNrawOut->u32Width)
        {
            /* 高度扩充超出图像下边界，直接取图像底64行作为包裹图像 */
            pstRtPackDivS->u32CurTop = pstSlliceNrawOut->u32Width - XSP_PACK_MIN_LINE;
            pstRtPackDivS->u32CurBottom = pstSlliceNrawOut->u32Width;
        }
        else
        {
            pstRtPackDivS->u32CurTop = pstRtPackDivS->u32CurTop - u32ExpandHeight;
            pstRtPackDivS->u32CurBottom = pstRtPackDivS->u32CurBottom + u32ExpandHeight;
        }
    }

    /* 包裹结束时，清空数据 */
    if (XSP_PACKAGE_END == pstDataNode->stPackDivRes[u32Idx].enPackageTag)
    {
        pstRtPackDivS->u32CurTop = pstSlliceNrawOut->u32Width;
        pstRtPackDivS->u32CurBottom = 0;
    }

    return;
}

/**
 * @function    xsp_rtproc_rm_blank_slice
 * @brief       实时预览包裹空白条带是否显示
 * @param[in] u32Chn 通道号
 * @param[in] pstDataNode 数据节点
 */
static void xsp_rtproc_rm_blank_slice(UINT32 u32Chn, XSP_DATA_NODE *pstDataNode)
{
    UINT32 i = 0;
    XSP_PACK_DIV_STAT *pstRtPackDivS = NULL;
    XSP_PACK_DIV_RES *pstNodePackDiv = NULL;

    if (NULL == pstDataNode)
    {
        XSP_LOGE(" pstDataNode[%p] is NULL\n", pstDataNode);
        return;
    }

    pstRtPackDivS = &g_xsp_common.stXspChn[u32Chn].stRtPackDivS;

    /*未开启空白条带移除功能或正在TIP插入状态下直接return*/
    if (!pstRtPackDivS->bRmBlankSlice || g_xsp_common.stTipAttr.stTipCtrl[u32Chn].uiUpdate)
    {
        return;
    }

    for (i = 0; i < g_xsp_common.stXspChn[u32Chn].stNormalData.u32RtSliceRefreshCnt; i++) /* 以小条带为单位 */
    {
        pstNodePackDiv = &pstDataNode->stPackDivRes[i];
        if (pstNodePackDiv->enPackageTag == XSP_PACKAGE_START || pstNodePackDiv->enPackageTag == XSP_PACKAGE_MIDDLE) /* 包裹开始中间标记为不移除*/
        {
            pstNodePackDiv->bRmBlankSliceSub = SAL_FALSE;
        }
        else if (pstNodePackDiv->enPackageTag == XSP_PACKAGE_END) /* 从包裹结束开始，一共保留u32BlkSliceDispCnt个空白条带*/
        {
            pstNodePackDiv->bRmBlankSliceSub = SAL_FALSE;
            pstRtPackDivS->u32BlkSliceDispCurCnt = pstRtPackDivS->u32BlkSliceDispCnt;
        }
        else    /* 判断空白条带是否用于显示，由应用下发参数决定 */
        {
            if (pstRtPackDivS->u32BlkSliceDispCurCnt == 0) /* 当前已无显示的空白条带 */
            {
                pstNodePackDiv->bRmBlankSliceSub = SAL_TRUE;
            }
            else
            {
                pstNodePackDiv->bRmBlankSliceSub = SAL_FALSE;
                pstRtPackDivS->u32BlkSliceDispCurCnt--;         /* 每标记一个-1*/
            }
        }
    }

    return;
}

/**
 * @fn      xsp_rtproc_divide_package
 * @brief   实时预览包裹分割
 *
 * @warning 包裹分割基于输入的XRAW数据，不以超分缩放后的数据宽高为依据
 *
 * @param   [IN] u32Chn 通道号
 * @param   [IN] pstDataNode 数据节点
 * @param   [IN] pstRtOut XSP算法实时预览输出信息
 */
static void xsp_rtproc_divide_package(UINT32 u32Chn, XSP_DATA_NODE *pstDataNode, XSP_PACKAGE_INDENTIFY *pstPackIndentify)
{
    UINT32 i = 0, j = 0, u32BitOffset = 0, u32SegLine = 0;
    XSP_PACK_DIV_STAT *pstRtPackDivS = NULL;
    XSP_PACK_DIV_RES *pstNodePackDiv = NULL;
    CAPB_XSP *pstCapXsp = capb_get_xsp();
    UINT32 *pu32Tmp = NULL;

    if ((NULL == pstDataNode) || (NULL == pstPackIndentify))
    {
        XSP_LOGE("pstDataNode[%p] OR pstPackIndentify[%p] is NULL\n", pstDataNode, pstPackIndentify);
        return;
    }

    pstRtPackDivS = &g_xsp_common.stXspChn[u32Chn].stRtPackDivS;
    u32SegLine = pstDataNode->stXRawInBuf.u32Height / g_xsp_common.stXspChn[u32Chn].stNormalData.u32RtSliceRefreshCnt;

    pstRtPackDivS->u32LastSliceNo = pstDataNode->u32RtSliceNo;
    for (i = 0; i < g_xsp_common.stXspChn[u32Chn].stNormalData.u32RtSliceRefreshCnt; i++)
    {
        pstNodePackDiv = &pstDataNode->stPackDivRes[i];
        if (XSP_PACK_DIVIDE_HW_SIGNAL == pstRtPackDivS->enMethod)
        {
            for (j = 0; j < u32SegLine; j++)
            {
                u32BitOffset = u32SegLine * i + j;
                pu32Tmp = (UINT32 *)pstDataNode->u32ColumnCont + u32BitOffset / 32;
                if (*pu32Tmp & (1 << (u32BitOffset % 32))) /* 有光障信号 */
                {
                    pstNodePackDiv->enSliceCont = XSP_SLICE_PACKAGE;
                    break;
                }
            }

            if (pstRtPackDivS->u32PackIdx + pstDataNode->stXRawInBuf.u32Height < XSP_DEBUG_PACK_SIGN_MAX * 8)
            {
                bitcopy(pstRtPackDivS->au8PackSign, pstRtPackDivS->u32PackIdx,
                        pstDataNode->u32ColumnCont, u32SegLine * i, u32SegLine);
                pstRtPackDivS->u32PackIdx += u32SegLine;
            }
        }

        /* 判断当前是否有包裹信息，若开启强扫，强制条带为包裹 */
        if ((XSP_PACK_DIVIDE_IMG_IDENTY == pstRtPackDivS->enMethod && pstPackIndentify->package_num > 0)
            || (XSP_PACK_DIVIDE_HW_SIGNAL == pstRtPackDivS->enMethod && XSP_SLICE_PACKAGE == pstNodePackDiv->enSliceCont)
            || g_xsp_common.stUpdatePrm.stXspRtParm.stEnhancedScan.bEnable)
        {
            pstNodePackDiv->enSliceCont = XSP_SLICE_PACKAGE;
            if (XSP_PACKAGE_NONE == pstRtPackDivS->enPackTagLast    /* 前一个条带是非包裹，当前条带是包裹，则为包裹开始 */
                || XSP_PACKAGE_END == pstRtPackDivS->enPackTagLast) /* 前一个条带是包裹结束，当前条带是包裹，则为包裹开始 */
            {
                pstNodePackDiv->enPackageTag = XSP_PACKAGE_START;
                pstRtPackDivS->u32PackStartSliceNo = pstDataNode->u32RtSliceNo;
                pstRtPackDivS->u64PackStartSliceTime = pstDataNode->u64SyncTime;
                pstRtPackDivS->u64PackStartTrigTime = pstDataNode->u64TrigTime;
                pstRtPackDivS->uiPackCurLine = u32SegLine; /* 包裹起始标志时，重置包裹长度 */
                pstRtPackDivS->u32SliceCnt = 1; /* 包裹起始标志时，重置条带数 */
                pstRtPackDivS->u32SliceNo[0] = (pstDataNode->u32RtSliceNo << 4) | i;
            }
            else /* 前一个条带是开始、中间，当前条带也是包裹的情况 */
            {
                /* 先判断是否超过强制分割阈值，若超过则强制包裹结束 */
                pstRtPackDivS->uiPackCurLine += u32SegLine;
                if (pstRtPackDivS->u32SliceCnt < XSP_PACK_HAS_SLICE_NUM_MAX)
                {
                    pstRtPackDivS->u32SliceNo[pstRtPackDivS->u32SliceCnt] = (pstDataNode->u32RtSliceNo << 4) | i;
                    pstRtPackDivS->u32SliceCnt++;
                }

                if (pstRtPackDivS->uiPackCurLine > pstCapXsp->xsp_package_line_max - u32SegLine)
                {
                    pstNodePackDiv->enPackageTag = XSP_PACKAGE_END;
                    pstNodePackDiv->bForcedDivided = SAL_TRUE;
                    XSP_LOGW("chan %u, force divide, packline: %u, SegLine: %u\n", u32Chn, pstRtPackDivS->uiPackCurLine, u32SegLine);
                }
                else /* 前一个条带是开始、中间，当前条带也是包裹，且未达到包裹强制分割的阈值，则为包裹中间部分 */
                {
                    pstNodePackDiv->enPackageTag = XSP_PACKAGE_MIDDLE;
                }
            }
        }
        else
        {
            pstNodePackDiv->enSliceCont = XSP_SLICE_BLANK;
            if (XSP_PACKAGE_NONE == pstRtPackDivS->enPackTagLast    /* 前一个条带是非包裹，当前条带也是非包裹 */
                || XSP_PACKAGE_END == pstRtPackDivS->enPackTagLast) /* 前一个条带是包裹结束，当前条带是非包裹 */
            {
                pstNodePackDiv->enPackageTag = XSP_PACKAGE_NONE;
            }
            else /* 前一个条带是开始、中间，当前条带是空白的情况 */
            {
                pstRtPackDivS->uiPackCurLine += u32SegLine; /* 包裹结束条带仍然是包裹的一部分，因此需要更新包裹长度 */
                if (pstRtPackDivS->u32SliceCnt < XSP_PACK_HAS_SLICE_NUM_MAX)
                {
                    pstRtPackDivS->u32SliceNo[pstRtPackDivS->u32SliceCnt] = (pstDataNode->u32RtSliceNo << 4) | i;
                    pstRtPackDivS->u32SliceCnt++;
                }

                /* 先判断是否小于最小包裹阈值，若小于，则强制认为包裹不可结束 */
                if (pstRtPackDivS->uiPackCurLine < XSP_PACK_MIN_LINE)
                {
                    pstNodePackDiv->enPackageTag = XSP_PACKAGE_MIDDLE;
                    pstRtPackDivS->bForceToLimited = SAL_TRUE;
                }
                else /* 前一个条带是开始、中间，当前条带是空白，且不小于最小包裹阈值，则为包裹结束 */
                {
                    pstNodePackDiv->enPackageTag = XSP_PACKAGE_END;
                }
            }
        }

        /* 计算包裹显示方向的上下边缘 */
        xsp_rtproc_get_pack_edge(u32Chn, pstDataNode, i, pstPackIndentify);

        pstRtPackDivS->enPackTagLast = pstNodePackDiv->enPackageTag;
        pstNodePackDiv->u32PackLine = pstRtPackDivS->uiPackCurLine;
        pstNodePackDiv->u32SliceCnt = pstRtPackDivS->u32SliceCnt;
        for (j = 0; j < pstNodePackDiv->u32SliceCnt; j++)
        {
            pstNodePackDiv->u32SliceNo[j] = pstRtPackDivS->u32SliceNo[j];
        }

        /* 包裹结束时，匹配包裹开始的条带号，并重置包裹长度为0 */
        if (XSP_PACKAGE_END == pstNodePackDiv->enPackageTag)
        {
            pstNodePackDiv->u32StartSliceNo = pstRtPackDivS->u32PackStartSliceNo;
            pstNodePackDiv->u64StartSliceTime = pstRtPackDivS->u64PackStartSliceTime;
            pstNodePackDiv->u64StartTrigTime = pstRtPackDivS->u64PackStartTrigTime;
            if (pstRtPackDivS->bForceToLimited
                || pstNodePackDiv->u32PackLine < XSP_PACK_MIN_LINE + u32SegLine) /* 包裹列数小于最小包裹列数+发送列数，归属于最小包裹 */
            {
                pstNodePackDiv->bForcedDivided = SAL_TRUE; /* 最小限制分割也归属于强制分割 */
            }

            if (!pstNodePackDiv->bForcedDivided) /* 非强制包裹分割时，最后一个条带肯定是空白，不计入包裹总长中 */
            {
                pstNodePackDiv->u32PackLine -= u32SegLine;
            }

            if (XSP_PACK_DIVIDE_HW_SIGNAL == pstRtPackDivS->enMethod)
            {
                pstNodePackDiv->u32PackIdx = pstRtPackDivS->u32PackIdx;
                pstRtPackDivS->u32PackIdx = 0;
            }

            pstRtPackDivS->uiPackCurLine = 0;
            pstRtPackDivS->u32SliceCnt = 0;
            pstRtPackDivS->bForceToLimited = SAL_FALSE;
        }
    }

    return;
}

/**
 * @function:   Xsp_DrvSyncPackDivision
 * @brief:      同步双视角包裹分割信息，选择独立分割、还是以通道0或通道1分割结果为准
 * @param[in]:  UINT32 chan 算法通道号
 * @param[in]:  XSP_XRAY_DATA_OUT *pstXrayDataOut 输出缓冲区数据
 * @param[in]:  XSP_RTPROC_OUT *pstRtOut 算法输出数据
 * @return:     INT32 成功SAL_SOK，失败SAL_FALSE
 */
static INT32 Xsp_DrvSyncPackDivision(UINT32 chan, XSP_DATA_NODE *pstDataNode, XSP_PACKAGE_INDENTIFY *pstPackIndentify)
{
    INT32 s32WaitCount = 0;
    UINT32 u32CachedResIdx = 0;
    XSP_PACK_DIV_STAT *pstLocalPackDivS = NULL;     // 当前通道的包裹分割状态
    XSP_PACK_DIV_STAT *pstPackDivS0 = NULL;
    XSP_PACK_DIV_STAT *pstPackDivS1 = NULL;

    static UINT32 au32ColumnCont[CACHE_SLICE_DIVIDE_MAX][8] = {0};      // 各列含有的包裹有无信息
    static UINT32 au32XspIdentifyNum[CACHE_SLICE_DIVIDE_MAX] = {0};     // 各列含有的包裹有无信息
    static INT64L au64RtSliceCol[CACHE_SLICE_DIVIDE_MAX] = {-1};        // 用静态变量存储通道0分割条带号
    static INT32 as32DivideMainChan[MAX_XRAY_CHAN] = {0};               // 所依赖的包裹分割结果的视角的通道号

    XSP_CHECK_PTR_IS_NULL(pstDataNode, SAL_FAIL);
    XSP_CHECK_PTR_IS_NULL(pstPackIndentify, SAL_FAIL);

    /* 一次完整的包裹分割结束后才同步参数 */
    pstLocalPackDivS = &g_xsp_common.stXspChn[chan].stRtPackDivS;
    if (XSP_PACKAGE_NONE == pstLocalPackDivS->enPackTagLast || XSP_PACKAGE_END == pstLocalPackDivS->enPackTagLast)
    {
        switch(pstLocalPackDivS->u32segMaster)
        {
        case XSP_PACK_DIVIDE_INDEPENDENT:
            as32DivideMainChan[chan] = -1;                  // 主辅通道独立分割
            break;
        case XSP_PACK_DIVIDE_USE_MASTER:
            as32DivideMainChan[chan] = 0;                   // 以通道0分割结果为准
            break;
        case XSP_PACK_DIVIDE_USE_SLAVE:
            as32DivideMainChan[chan] = 1;                   //以通道1分割结果为准
            break;
        default:
            XSP_LOGE("u32segMaster :%u out of range\n", pstLocalPackDivS->u32segMaster);
            break;
        }
    }

    u32CachedResIdx = pstDataNode->u32RtSliceNo % CACHE_SLICE_DIVIDE_MAX;
    if(0 <= as32DivideMainChan[chan] && as32DivideMainChan[0] == as32DivideMainChan[1])
    {
        pstPackDivS0 = &g_xsp_common.stXspChn[as32DivideMainChan[chan]].stRtPackDivS;
        if (as32DivideMainChan[chan] == chan)      // 所依赖的通道进行包裹分割信息缓存
        {
            memcpy(au32ColumnCont[u32CachedResIdx], pstDataNode->u32ColumnCont, sizeof(pstDataNode->u32ColumnCont));
            au32XspIdentifyNum[u32CachedResIdx] = pstPackIndentify->package_num;
            au64RtSliceCol[u32CachedResIdx] = pstDataNode->u32RtSliceNo;
        }
        else                // 另一通道从缓存中等待包裹分割信息
        {
            pstPackDivS1 = &g_xsp_common.stXspChn[chan].stRtPackDivS;
            s32WaitCount = 20;
            while (1)
            {
                if (pstDataNode->u32RtSliceNo == au64RtSliceCol[u32CachedResIdx])
                {
                    /* 使用缓存的所依赖视角的信息进行包裹判断 */
                    memcpy(pstDataNode->u32ColumnCont, au32ColumnCont[u32CachedResIdx], sizeof(pstDataNode->u32ColumnCont));
                    pstPackIndentify->package_num = au32XspIdentifyNum[u32CachedResIdx];

                    break;
                }
                else
                {
                    if (pstPackDivS1->u32LastSliceNo < pstPackDivS0->u32LastSliceNo)
                    {
                        XSP_LOGW("chan: %d, col:%u is smaller than the other chan col:%u, break!\n", chan, pstPackDivS1->u32LastSliceNo, pstPackDivS0->u32LastSliceNo);
                        break;
                    }
                    if (s32WaitCount-- > 0)
                    {
                        SAL_msleep(5);         /*超时间不能过长，否则一次匹配不到，之后就可能一直无法匹配*/
                    }
                    else
                    {
                        XSP_LOGW("chan: %d, col:%u, pack division info sync timeout, the other chan col:%u!\n", chan, pstDataNode->u32RtSliceNo, pstPackDivS0->u32LastSliceNo);
                        break;      /* 超时退出 */
                    }
                }
            }
        }
    }

    xsp_rtproc_divide_package(chan, pstDataNode, pstPackIndentify);

    /*空白条带移除*/
    xsp_rtproc_rm_blank_slice(chan, pstDataNode);

    /* 打印同步信息 */
    if (XSP_PACKAGE_START == pstDataNode->stPackDivRes[0].enPackageTag || XSP_PACKAGE_END == pstDataNode->stPackDivRes[0].enPackageTag)
    {
        XSP_LOGI("chan:%u, slice sync info, slice col:%u, tag:%d, divide main chan:%d\n", 
                 chan, pstDataNode->u32RtSliceNo, pstDataNode->stPackDivRes[0].enPackageTag, as32DivideMainChan[chan]);
    }
    if (XSP_PACKAGE_NONE == pstLocalPackDivS->enPackTagLast && XSP_PACKAGE_MIDDLE == pstDataNode->stPackDivRes[0].enPackageTag)
    {
        XSP_LOGW("chan:%u, slice col:%u, pack middle, package division without package start\n", chan, pstDataNode->u32RtSliceNo);
    }
    if ((XSP_PACKAGE_START == pstLocalPackDivS->enPackTagLast || XSP_PACKAGE_MIDDLE == pstLocalPackDivS->enPackTagLast) && 
        XSP_PACKAGE_NONE == pstDataNode->stPackDivRes[0].enPackageTag)
    {
        XSP_LOGW("chan:%u, slice col:%u, pack blank, package division without package end\n", chan, pstDataNode->u32RtSliceNo);
    }

    return SAL_SOK;
}

/**
 * @function    get_node_by_stage
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static DSA_NODE *get_node_by_stage(DSA_LIST *lst, const XSP_PROC_STAGE enStage)
{
    DSA_NODE *pNode = NULL;

    if (DSA_LstGetCount(lst) > 0)
    {
        pNode = DSA_LstGetHead(lst);
        while (NULL != pNode)
        {
            if (enStage == ((XSP_DATA_NODE *)pNode->pAdData)->enProcStage)
            {
                return pNode;
            }
            else
            {
                pNode = pNode->next;
            }
        }
    }

    return NULL;
}

/**
 * @function    get_count_by_stage
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static UINT32 get_count_by_stage(DSA_LIST *lst, const XSP_PROC_STAGE enStage)
{
    UINT32 u32NodeCnt = 0;
    DSA_NODE *pNode = NULL;

    if (DSA_LstGetCount(lst) > 0)
    {
        pNode = DSA_LstGetHead(lst);
        while (NULL != pNode)
        {
            if (enStage == ((XSP_DATA_NODE *)pNode->pAdData)->enProcStage)
            {
                u32NodeCnt++;
            }

            pNode = pNode->next;
        }
    }

    return u32NodeCnt;
}

/**
 * @function   Xsp_WaitProcListEmpty
 * @brief      等待XSP处理队列中非Ready状态节点清空，即XSP_PSTG_PROCING节点数为1，XSP_PSTG_PROCED和XSP_PSTG_SENDING节点数都为0
 * @param[in]  UINT32 u32Chn            算法通道号
 * @param[in]  BOOL   bAllClear         是否是队列完全清空，SAL_TRUE-完全清空，SAL_FALSE-仅清空处理完成状态之后的节点
 * @param[in]  INT32  s32TryCnt         最大尝试次数
 * @param[in]  UINT32 u32SleepMsOnce    每次休眠时间(ms)
 * @param[out] None
 * @return     SAL_STATUS 成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Xsp_WaitProcListEmpty(UINT32 chan, BOOL bAllClear, UINT32 u32MaxTryCnt, UINT32 u32SleepMsOnce)
{
    UINT32 u32TryCnt = 0;
    UINT32 ts = 0, te = 0, tl = 0;
    XSP_CHN_PRM *pstXspChn = &g_xsp_common.stXspChn[chan];

    /* 先等待XSP处理节点清空 */
    if (SAL_TRUE == bAllClear)
    {
        tl = u32MaxTryCnt * u32SleepMsOnce;
        SAL_mutexTmLock(&pstXspChn->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (DSA_LstGetCount(pstXspChn->lstDataProc) > 0)
        {
            if (tl + ts > te)
            {
                tl = tl + ts - te;
                ts = sal_get_tickcnt();
                SAL_CondWait(&pstXspChn->lstDataProc->sync, tl, __FUNCTION__, __LINE__);
                te = sal_get_tickcnt();
            }
            else
            {
                XSP_LOGE("chan %u, wait lstDataProc to be empty timeout[%u ms], number: %u\n",
                        chan, u32MaxTryCnt * u32SleepMsOnce, DSA_LstGetCount(pstXspChn->lstDataProc));
                break; /* 超时退出 */
            }
        }
        SAL_mutexUnlock(&pstXspChn->lstDataProc->sync.mid);
    }
    else
    {
        /* 等待XSP把所有数据均已发送给XPack，超时时间s32TryCnt * u32SleepMsOnce */
        while (get_count_by_stage(pstXspChn->lstDataProc, XSP_PSTG_PROCING) > 1 ||  /* 仅当前全屏处理的节点处于XSP_PSTG_PROCING状态 */
               get_count_by_stage(pstXspChn->lstDataProc, XSP_PSTG_PROCED) > 0 || 
               get_count_by_stage(pstXspChn->lstDataProc, XSP_PSTG_SENDING) > 0)
        {
            if (u32TryCnt < u32MaxTryCnt)
            {
                u32TryCnt++;
                SAL_msleep(u32SleepMsOnce);
            }
            else
            {
                XSP_LOGW("chan %u, wait for 'lstDataProc' to be free timeouted[%u ms]\n", chan, u32TryCnt * u32SleepMsOnce);
                return SAL_FAIL;
            }
        }
    }

    return SAL_SOK;
}

/**
 * @function   Xsp_WaitDispBufferDescend
 * @brief      等待xpack显示缓存中未显示数据量减小到不大于s32Target的指定值
 * @param[in]  UINT32 u32Chn         算法通道号
 * @param[in]  UINT32 u32MaxTryCnt   最大尝试次数
 * @param[in]  UINT32 u32SleepMsOnce 每次休眠时间(ms)
 * @param[in]  INT32  s32Target       目标最大最大缓存数据量
 * @param[out] None
 * @return     SAL_STATUS 成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Xsp_WaitDispBufferDescend(UINT32 chan, UINT32 u32MaxTryCnt, UINT32 u32SleepMsOnce, INT32 s32Target)
{
    UINT32 u32TryCnt = 0;

    while (s32Target < Xpack_DrvGetDispBuffStatus(chan))
    {
        if (u32TryCnt < u32MaxTryCnt)
        {
            u32TryCnt++;
            SAL_msleep(u32SleepMsOnce);
        }
        else
        {
            XSP_LOGE("chan %u, wait xpack display buffer to descend timeout[%u ms], left data size:%d, target:%d\n",
                     chan, u32TryCnt * u32SleepMsOnce, Xpack_DrvGetDispBuffStatus(chan), s32Target);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   xsp_set_image_proc_param
 * @brief      XSP参数设置
 *    1) 模式(默认)设置:
 *       双能：
 *       默认模式0：增强模式0
 *       默认模式1：增强模式1
 *       默认模式2：增强模式2
 *       单能：
 *       默认模式0：所有功能恢复默认，开启伪彩模式0；
 *       默认模式1：所有功能恢复默认，开启伪彩模式1；
 *    2) 原始模式设置：
 *       开启：开启原始模式，所有功能关闭，即恢复默认；
 *       关闭：关闭原始模式，其它功能不变；
 * 研究院算法：
 *    3) 双能：低穿/高穿/局增 关系互斥；单能无低穿和高穿
 * 自研算法：
 *    4) 低穿/高穿互斥；单能无低穿，无高穿；
 *    5) 边增/超增互斥；
 *    互斥的关系，最多只能一个功能开启
 * 转存功能支持：镜像、颜色表、默认增强
 * @param[in]  UINT32 u32Chn  算法通道号
 * @param[out] None
 * @return     static INT32 成功SAL_SOK，失败SAL_FALSE
 */
static SAL_STATUS xsp_set_image_proc_param(UINT32 u32Chn)
{
    SAL_STATUS s32Ret = SAL_SOK;
    UINT32 u32ImgProcMode = XSP_IMG_PROC_NONE;
    U32 u32XspBgColorStd = g_xsp_common.stXspChn[u32Chn].u32XspBgColorStd;
    XSP_DEFAULT_STATE enhance_state = XSP_DEFAULT_STATE_MODE0;
    XSP_RT_PARAMS stPicPrm = {0};
    XSP_BASE_PRM stBasePrm = {0};

    XSP_CHN_PRM *pstXspChn = NULL;
    void *pRtPbHandle = NULL; /*预览通道句柄*/
    void *pTransHandle = gstXspTransProc.pXspHandle; /*转存通道句柄*/
    XSP_UPDATE_PRM *pstUpdatePrm = &g_xsp_common.stUpdatePrm;
    DSPINITPARA *pstDspInitPrm = SystemPrm_getDspInitPara();
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (u32Chn >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("the chan[%u] is invalid, should be less than %u!", u32Chn, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    pstXspChn = &g_xsp_common.stXspChn[u32Chn];
    pRtPbHandle = pstXspChn->handle;
    /*取出配置参数*/
    SAL_mutexTmLock(&pstUpdatePrm->mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    u32ImgProcMode = pstXspChn->stNormalData.u32ImgProcMode;

    /*没有参数设置直接返回*/
    if (u32ImgProcMode == XSP_IMG_PROC_NONE)
    {
        SAL_mutexTmUnlock(&pstUpdatePrm->mutexImgProc, __FUNCTION__, __LINE__);
        return SAL_SOK;
    }
    else
    {
        /* 结束当前次处理，可接受下一次图像处理请求，处理模式已记录在u32ImgProcMode中，后续若设置图像参数返回错误，直接忽略不做二次尝试 */
        pstXspChn->stNormalData.u32ImgProcMode = XSP_IMG_PROC_NONE;
        pstXspChn->stNormalData.bImgProcStarted = SAL_FALSE;
        memcpy(&stPicPrm, &pstUpdatePrm->stXspRtParm, sizeof(XSP_RT_PARAMS));
        memcpy(&stBasePrm, &pstUpdatePrm->stBasePrm, sizeof(XSP_BASE_PRM));
        SAL_mutexTmUnlock(&pstUpdatePrm->mutexImgProc, __FUNCTION__, __LINE__);
    }

    XSP_LOGI("chan %u, Image Process Mode 0x%x, disp %d, style %d, ori %d-%u, anti %d, ee %d-%u, ue %d, "
             "lp %d, hp %d, le %d, absor %d, pseudo %d, es %d, lum %u, bkg-th %u, bkg-upr: %u\n",
             u32Chn, u32ImgProcMode, stPicPrm.stDispMode.disp_mode, stPicPrm.stDefaultStyle.xsp_style, stPicPrm.stOriginalMode.bEnable,
             stPicPrm.stOriginalMode.uiLevel, stPicPrm.stAntiColor.bEnable, stPicPrm.stEe.bEnable, stPicPrm.stEe.uiLevel, stPicPrm.stUe.bEnable,
             stPicPrm.stLp.bEnable, stPicPrm.stHp.bEnable, stPicPrm.stLe.bEnable, stPicPrm.stVarAbsor.bEnable, stPicPrm.stPseudoColor.pseudo_color,
             stPicPrm.stEnhancedScan.bEnable, stPicPrm.stLum.uiLevel, stPicPrm.stBkg.uiBkgDetTh, stPicPrm.stBkg.uiFullUpdateRatio);

    /**
     * 默认模式和原始模式关联的图像处理模式有：
     * 1、显示模式：彩色、黑白、有机物剔除、无机物剔除
     * 2、反色
     * 3、超增
     * 4、边增
     * 5、低穿、高穿
     * 6、可变吸收率
     * 7、局增
     * 8、亮度（仅单能）
     * 9、伪彩（仅单能）
     */
    if (XSP_IMG_PROC_DEFAULT & u32ImgProcMode || XSP_IMG_PROC_ORIGINAL & u32ImgProcMode) /* 默认模式和原始模式，互斥不能共存 */
    {
        s32Ret = g_xsp_lib_api.set_disp_color(pRtPbHandle, stPicPrm.stDispMode.disp_mode); /* 显示 */
        s32Ret |= g_xsp_lib_api.set_anti(pRtPbHandle, stPicPrm.stAntiColor.bEnable); /*反色*/
        s32Ret |= g_xsp_lib_api.set_absor(pRtPbHandle, stPicPrm.stVarAbsor.bEnable, stPicPrm.stVarAbsor.uiLevel); /*关闭可变吸收 */
        s32Ret |= g_xsp_lib_api.set_luminance(pRtPbHandle, stPicPrm.stLum.uiLevel); /*亮度*/
        s32Ret |= g_xsp_lib_api.set_pseudo_color(pRtPbHandle, stPicPrm.stPseudoColor.pseudo_color); /*伪彩*/
        s32Ret |= g_xsp_lib_api.set_pseudo_color(pTransHandle, stPicPrm.stPseudoColor.pseudo_color);

        /* 注：自研算法和研究院算法逻辑不同，参考函数头注释 */
        if (pstDspInitPrm->xspLibSrc <= XSP_LIB_RAYIN_AI_OTH && XSP_LIB_RAYIN <= pstDspInitPrm->xspLibSrc)
        {
            /*低穿高穿互斥，局增独立*/
            if (SAL_TRUE == stPicPrm.stLp.bEnable) /*低穿*/
            {
                s32Ret |= g_xsp_lib_api.set_lp(pRtPbHandle, stPicPrm.stLp.bEnable);
            }
            else if (SAL_TRUE == stPicPrm.stHp.bEnable) /*高穿*/
            {
                s32Ret |= g_xsp_lib_api.set_hp(pRtPbHandle, stPicPrm.stHp.bEnable);
            }
            else /*关闭*/
            {
                s32Ret |= g_xsp_lib_api.set_lp(pRtPbHandle, SAL_FALSE);
            }

            s32Ret |= g_xsp_lib_api.set_le(pRtPbHandle, stPicPrm.stLe.bEnable); /*局增*/
        }
        else /*研究院算法*/
        {
            /*低穿高穿局增互斥*/
            if (SAL_TRUE == stPicPrm.stLp.bEnable)
            {
                s32Ret |= g_xsp_lib_api.set_lp(pRtPbHandle, stPicPrm.stLp.bEnable);
            }
            else if (SAL_TRUE == stPicPrm.stHp.bEnable)
            {
                s32Ret |= g_xsp_lib_api.set_hp(pRtPbHandle, stPicPrm.stHp.bEnable);
            }
            else if (SAL_TRUE == stPicPrm.stLe.bEnable)
            {
                s32Ret |= g_xsp_lib_api.set_le(pRtPbHandle, stPicPrm.stLe.bEnable);
            }
            else /*关闭*/
            {
                s32Ret |= g_xsp_lib_api.set_le(pRtPbHandle, SAL_FALSE);
            }
        }

        /* 超增与边增互斥，算法内部同一个接口 */
        if (SAL_TRUE == stPicPrm.stUe.bEnable)
        {
            s32Ret |= g_xsp_lib_api.set_ue(pRtPbHandle, stPicPrm.stUe.bEnable, stPicPrm.stUe.uiLevel); /*超增*/
        }
        else if (SAL_TRUE == stPicPrm.stEe.bEnable)
        {
            s32Ret |= g_xsp_lib_api.set_ee(pRtPbHandle, stPicPrm.stEe.bEnable, stPicPrm.stEe.uiLevel); /*边增*/
        }
        else
        {
            s32Ret |= g_xsp_lib_api.set_ue(pRtPbHandle, SAL_FALSE, stPicPrm.stUe.uiLevel); 
        }

        /*默认增强模式*/
        if ((XSP_IMG_PROC_ORIGINAL & u32ImgProcMode) && stPicPrm.stOriginalMode.bEnable)
        {
            /* 打开原始模式则关闭增强模式，关闭原始模式则配置之前的默认增强模式 */
            enhance_state = XSP_DEFAULT_STATE_CLOSE;
        }
        else
        {
            switch (stPicPrm.stDefaultStyle.xsp_style)
            {
                case XSP_DEFAULT_STYLE_0:
                    enhance_state = XSP_DEFAULT_STATE_MODE0;
                    break;

                case XSP_DEFAULT_STYLE_1:
                    enhance_state = XSP_DEFAULT_STATE_MODE1;
                    break;

                case XSP_DEFAULT_STYLE_2:
                    enhance_state = XSP_DEFAULT_STATE_MODE2;
                    break;

                default:
                    XSP_LOGE("chan %u, the xsp_style is invalid: %d\n", u32Chn, stPicPrm.stDefaultStyle.xsp_style);
                    return SAL_FAIL;
            }
        }

        s32Ret |= g_xsp_lib_api.set_default_enhance(pRtPbHandle, enhance_state, stPicPrm.stOriginalMode.uiLevel);
        /*转存通道默认增强配置，忽略配置的返回值，即使配置失败，仍然执行实时过包或回拉的图像处理*/
        g_xsp_lib_api.set_default_enhance(pTransHandle, enhance_state, stPicPrm.stOriginalMode.uiLevel);
    }
    else
    {
        if (XSP_IMG_PROC_DISP & u32ImgProcMode) /*显示*/
        {
            s32Ret |= g_xsp_lib_api.set_disp_color(pRtPbHandle, stPicPrm.stDispMode.disp_mode);
        }

        if (XSP_IMG_PROC_ANTI & u32ImgProcMode) /* 反色 */
        {
            s32Ret |= g_xsp_lib_api.set_anti(pRtPbHandle, stPicPrm.stAntiColor.bEnable);
        }

        if (XSP_IMG_PROC_UE & u32ImgProcMode) /* 超增 */
        {
            s32Ret |= g_xsp_lib_api.set_ue(pRtPbHandle, stPicPrm.stUe.bEnable, stPicPrm.stUe.uiLevel);
        }

        if (XSP_IMG_PROC_EE & u32ImgProcMode) /* 边增 */
        {
            s32Ret |= g_xsp_lib_api.set_ee(pRtPbHandle, stPicPrm.stEe.bEnable, stPicPrm.stEe.uiLevel);
        }

        if (XSP_IMG_PROC_LP & u32ImgProcMode) /* 低穿 */
        {
            s32Ret |= g_xsp_lib_api.set_lp(pRtPbHandle, stPicPrm.stLp.bEnable);
        }

        if (XSP_IMG_PROC_HP & u32ImgProcMode) /* 高穿 */
        {
            s32Ret |= g_xsp_lib_api.set_hp(pRtPbHandle, stPicPrm.stHp.bEnable);
        }

        if (XSP_IMG_PROC_ABSOR & u32ImgProcMode) /*可变吸收率*/
        {
            s32Ret |= g_xsp_lib_api.set_absor(pRtPbHandle, stPicPrm.stVarAbsor.bEnable, stPicPrm.stVarAbsor.uiLevel);
        }

        if (XSP_IMG_PROC_LE & u32ImgProcMode) /* 局增 */
        {
            s32Ret |= g_xsp_lib_api.set_le(pRtPbHandle, stPicPrm.stLe.bEnable);
        }

        if (XSP_IMG_PROC_LUMINANCE & u32ImgProcMode) /* 亮度 */
        {
            s32Ret |= g_xsp_lib_api.set_luminance(pRtPbHandle, stPicPrm.stLum.uiLevel);
        }

        if (XSP_IMG_PROC_PSEUDO_COLOR & u32ImgProcMode) /* 伪彩 */
        {
            s32Ret |= g_xsp_lib_api.set_pseudo_color(pRtPbHandle, stPicPrm.stPseudoColor.pseudo_color);
            s32Ret |= g_xsp_lib_api.set_pseudo_color(pTransHandle, stPicPrm.stPseudoColor.pseudo_color);
        }
    }

    if (XSP_IMG_PROC_FORCE_FLUSH & u32ImgProcMode) /* 强制刷新 */
    {
        /* Do Nothing */
    }

    if (XSP_IMG_PROC_BKG_COLOR  & u32ImgProcMode)
    {
        /* 设置XSP算法背景色 */
        s32Ret |= g_xsp_lib_api.set_bkg_color(pRtPbHandle, u32XspBgColorStd);
        if (0 == u32Chn)
        {
            s32Ret |= g_xsp_lib_api.set_bkg_color(pTransHandle, u32XspBgColorStd);
        }
        if (SAL_SOK != s32Ret)
        {
            XSP_LOGE("chan %u, set bkg color failed, bkg value: 0x%x\n", u32Chn, u32XspBgColorStd);
        }
    }

    if (XSP_IMG_PROC_MIRROR & u32ImgProcMode) /*镜像*/
    {
        s32Ret |= Xsp_DrvConvertMirror(u32Chn, XSP_HANDLE_RT_PB);
    }

    if (XSP_IMG_PROC_BKG_PARAM & u32ImgProcMode) /* 背景阈值 */
    {
        s32Ret |= g_xsp_lib_api.set_bg(pRtPbHandle, stPicPrm.stBkg.uiBkgDetTh, stPicPrm.stBkg.uiFullUpdateRatio);
        s32Ret |= g_xsp_lib_api.set_bg(pTransHandle, stPicPrm.stBkg.uiBkgDetTh, stPicPrm.stBkg.uiFullUpdateRatio);
    }

    if (XSP_IMG_PROC_COLOR_TABLE & u32ImgProcMode) /* 颜色表 */
    {
        s32Ret |= g_xsp_lib_api.set_color_table(pRtPbHandle, stPicPrm.stColorTable.enConfigueTable);
        /*转存通道颜色表配置*/
        g_xsp_lib_api.set_color_table(pTransHandle, stPicPrm.stColorTable.enConfigueTable);
    }

    if (XSP_IMG_PROC_COLORS_IMAGING & u32ImgProcMode) /* 三色六色显示 */
    {
        s32Ret |= g_xsp_lib_api.set_colors_imaging(pRtPbHandle, stPicPrm.stColorsImaging.enColorsImaging);
        /*转存通道颜色表配置*/
        g_xsp_lib_api.set_colors_imaging(pTransHandle, stPicPrm.stColorsImaging.enColorsImaging);
    }

    if (XSP_IMG_PROC_COLOR_ADJUST & u32ImgProcMode) /* 色彩微调 */
    {
        s32Ret |= g_xsp_lib_api.set_color_adjust(pRtPbHandle, stBasePrm.stColorAdjust[u32Chn].uiLevel);
        g_xsp_lib_api.set_color_adjust(pTransHandle, stBasePrm.stColorAdjust[0].uiLevel); /* 转存以主视角为准 */
    }

    if (XSP_IMG_PROC_CONTRAST & u32ImgProcMode) /* 对比度 */
    {
        s32Ret |= g_xsp_lib_api.set_contrast(pRtPbHandle, stBasePrm.stContrast[u32Chn].uiLevel);
        s32Ret |= g_xsp_lib_api.set_contrast(pTransHandle, stBasePrm.stContrast[u32Chn].uiLevel);
    }

    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("chan %u, set image process parameters failed, u32ImgProcMode: 0x%x\n", u32Chn, u32ImgProcMode);
    }

    return s32Ret;
}

/**
 * @function   Xsp_DrvConfigTransProcParam
 * @brief      转存通道XSP参数设置
 * @param[in]  BOOL             bSetFlag          置位标志，1-设置xsp参数，0-复位xsp参数
 * @param[in]  XSP_PROC_TYPE_UN unXspProcType     算法处理类型
 * @param[in]  XSP_PROC_PARAMS *pstXspProcParamIn 算法处理参数
 * @param[out] None
 * @return     SAL_STATUS 成功SAL_SOK，失败SAL_FALSE
 */
SAL_STATUS Xsp_DrvConfigTransProcParam(BOOL bSetFlag, XSP_PROC_TYPE_UN unXspProcType, XSP_RT_PARAMS *pstXspProcParamIn)
{
    SAL_STATUS s32Ret = SAL_SOK;
    UINT32 TransBkgValue = 0;
    U08 u8RValue = 0, u8GValue = 0, u8BValue = 0;
    U08 u8YValue = 0, u8UValue = 0, u8VValue = 0;
    XSP_DEFAULT_STATE enhance_state = XSP_DEFAULT_STATE_MODE0;
    XSP_RT_PARAMS *pstXspProcParamSet = NULL;
    static XSP_RT_PARAMS stDefXspProcParam = {0};     // 记录默认成像处理模式
    XSP_RT_PARAMS stPicPrm = {0};
    XSP_UPDATE_PRM *pstUpdatePrm = &g_xsp_common.stUpdatePrm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    void *pTransHandle = gstXspTransProc.pXspHandle;    /*转存通道句柄*/

    memcpy(&stPicPrm, &pstUpdatePrm->stXspRtParm, sizeof(XSP_RT_PARAMS));

    if (SAL_TRUE == bSetFlag)
    {
        XSP_CHECK_PTR_IS_NULL(pstXspProcParamIn, SAL_FAIL);
        pstXspProcParamSet = pstXspProcParamIn;     // 设置成像参数时使用传入参数
    }
    else
    {
        pstXspProcParamSet = &stDefXspProcParam;    // 恢复成像参数时使用默认参数
    }

    if (unXspProcType.stXspProcType.XSP_PROC_DISP)
    {
        stDefXspProcParam.stDispMode.disp_mode = XSP_DISP_COLOR;
        s32Ret = g_xsp_lib_api.set_disp_color(pTransHandle, pstXspProcParamSet->stDispMode.disp_mode);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_ANTI)
    {
        stDefXspProcParam.stAntiColor.bEnable = SAL_FALSE;
        s32Ret = g_xsp_lib_api.set_anti(pTransHandle, pstXspProcParamSet->stAntiColor.bEnable);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_EE)
    {
        stDefXspProcParam.stEe.bEnable = SAL_FALSE;
        s32Ret = g_xsp_lib_api.set_ee(pTransHandle, pstXspProcParamSet->stEe.bEnable, pstXspProcParamSet->stEe.uiLevel);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_UE)
    {
        stDefXspProcParam.stUe.bEnable = SAL_FALSE;
        s32Ret = g_xsp_lib_api.set_ue(pTransHandle, pstXspProcParamSet->stUe.bEnable, pstXspProcParamSet->stUe.uiLevel);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_LE)
    {
        stDefXspProcParam.stLe.bEnable = SAL_FALSE;
        s32Ret = g_xsp_lib_api.set_le(pTransHandle, pstXspProcParamSet->stLe.bEnable);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_LP)
    {
        stDefXspProcParam.stLp.bEnable = SAL_FALSE;
        s32Ret = g_xsp_lib_api.set_lp(pTransHandle, pstXspProcParamSet->stLp.bEnable);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_HP)
    {
        stDefXspProcParam.stHp.bEnable = SAL_FALSE;
        s32Ret = g_xsp_lib_api.set_hp(pTransHandle, pstXspProcParamSet->stHp.bEnable);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_ABSOR)
    {
        stDefXspProcParam.stVarAbsor.bEnable = SAL_FALSE;
        s32Ret = g_xsp_lib_api.set_absor(pTransHandle, pstXspProcParamSet->stVarAbsor.bEnable, pstXspProcParamSet->stVarAbsor.uiLevel);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_LUMI)
    {
        stDefXspProcParam.stLum.uiLevel = XSP_LUM_DEFUALT_LEVEL;
        s32Ret = g_xsp_lib_api.set_luminance(pTransHandle, pstXspProcParamSet->stLum.uiLevel);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_PSEDOR)
    {
        /*仅单能支持*/
        if (XRAY_ENERGY_SINGLE != pstCapXrayIn->xray_energy_num)
        {
            XSP_LOGE("Pseudo Color is not supported by dual [%d]!\n", pstCapXrayIn->xray_energy_num);
            return SAL_FAIL;
        }
        stDefXspProcParam.stPseudoColor.pseudo_color = stPicPrm.stPseudoColor.pseudo_color;
        s32Ret = g_xsp_lib_api.set_pseudo_color(pTransHandle, pstXspProcParamSet->stPseudoColor.pseudo_color);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_ORI)
    {
        if (pstXspProcParamSet->stOriginalMode.bEnable)
        {
            enhance_state = XSP_DEFAULT_STATE_CLOSE;
        }
        stDefXspProcParam.stOriginalMode.bEnable = SAL_FALSE;
        s32Ret = g_xsp_lib_api.set_default_enhance(pTransHandle, enhance_state, stPicPrm.stOriginalMode.uiLevel);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_DEF)
    {
        switch (pstXspProcParamSet->stDefaultStyle.xsp_style)
        {
        case XSP_DEFAULT_STYLE_0:
            enhance_state = XSP_DEFAULT_STATE_MODE0;
            break;
        case XSP_DEFAULT_STYLE_1:
            enhance_state = XSP_DEFAULT_STATE_MODE1;
            break;
        case XSP_DEFAULT_STYLE_2:
            enhance_state = XSP_DEFAULT_STATE_MODE2;
            break;
        default:
            XSP_LOGE("The xsp_style is invalid: %d\n", pstXspProcParamSet->stDefaultStyle.xsp_style);
            return SAL_FAIL;
        }
        stDefXspProcParam.stDefaultStyle.xsp_style = stPicPrm.stDefaultStyle.xsp_style;
        s32Ret = g_xsp_lib_api.set_default_enhance(pTransHandle, enhance_state, stPicPrm.stOriginalMode.uiLevel);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_COLORTABLE)
    {
        stDefXspProcParam.stColorTable.enConfigueTable = stPicPrm.stColorTable.enConfigueTable;
        s32Ret = g_xsp_lib_api.set_color_table(pTransHandle, pstXspProcParamSet->stColorTable.enConfigueTable);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_COLORSIMAGING)
    {
        stDefXspProcParam.stColorsImaging.enColorsImaging = stPicPrm.stColorsImaging.enColorsImaging;
        s32Ret = g_xsp_lib_api.set_colors_imaging(pTransHandle, pstXspProcParamSet->stColorsImaging.enColorsImaging);
    }
    if (unXspProcType.stXspProcType.XSP_PROC_BGCOLOR)
    {
        if (pstCapbXsp->enDispFmt == pstXspProcParamSet->stBkgColor.enDataFmt) /* xsp输出显示数据RGB格式设置和还原转存背景色、YUV格式还原转存背景色 */
        {
            /* 直接设置背景 */
            TransBkgValue = pstXspProcParamSet->stBkgColor.uiBkgValue;
        }
        else if (pstXspProcParamSet->stBkgColor.enDataFmt == DSP_IMG_DATFMT_BGR24 || pstXspProcParamSet->stBkgColor.enDataFmt == DSP_IMG_DATFMT_BGRA32) /* xsp输出显示数据YUV格式设置背景色 */
        {
            u8RValue = pstXspProcParamSet->stBkgColor.uiBkgValue >> 16 & 0xFF;
            u8GValue = pstXspProcParamSet->stBkgColor.uiBkgValue >> 8 & 0xFF;
            u8BValue = pstXspProcParamSet->stBkgColor.uiBkgValue >> 0 & 0xFF;

            u8YValue = SAL_CLIP(( 0.2126 * u8RValue +  0.7152 * u8GValue +  0.0722 * u8BValue +   0), 16.0, 235.0);
            u8UValue = SAL_CLIP((-0.1146 * u8RValue + -0.3854 * u8GValue +  0.5000 * u8BValue + 128), 16.0, 235.0);
            u8VValue = SAL_CLIP(( 0.5000 * u8RValue + -0.4542 * u8GValue + -0.0468 * u8BValue + 128), 16.0, 235.0);

            TransBkgValue = u8YValue << 16 | u8UValue << 8 | u8VValue << 0;
        }
        else
        {
            XSP_LOGE("invalid bkg format! only support fmt:0x%x and fmt:0x%x, received fmt:0x%x, value:0x%x\n",
                     DSP_IMG_DATFMT_BGR24, DSP_IMG_DATFMT_BGRA32, pstXspProcParamSet->stBkgColor.enDataFmt, pstXspProcParamSet->stBkgColor.uiBkgValue);
            return SAL_FAIL;
        }

        stDefXspProcParam.stBkgColor.enDataFmt = pstCapbXsp->enDispFmt;
        stDefXspProcParam.stBkgColor.uiBkgValue = g_xsp_common.stXspChn[0].u32XspBgColorStd;
        s32Ret = g_xsp_lib_api.set_bkg_color(pTransHandle, TransBkgValue);
    }

    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("Setting xsp transform channel param failed, proc type 0x%x.\n", unXspProcType.u32XspProcType);
    }

    return s32Ret;
}

/**
 * @function    xsp_launch_image_process
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS xsp_launch_image_process(UINT32 chan)
{
    SAL_STATUS retVal = SAL_SOK;
    INT32 s32AntiVal = 0;
    CHAR dbgInfo[128] = {0};
    DSA_NODE *pNode, *pNodeInst = NULL;
    XSP_DATA_NODE *pstDataNode = NULL;
    XSP_CHN_PRM *pstXspChn = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    XSP_UPDATE_PRM *pstUpdatePrm = &g_xsp_common.stUpdatePrm;

    memset(dbgInfo, 0, sizeof(dbgInfo));

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("the chan[%u] is invalid, range: [0, %u)\n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    pstXspChn = &g_xsp_common.stXspChn[chan];
    if (SAL_SOK != SAL_mutexTmLock(&pstUpdatePrm->mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
    {
        XSP_LOGE("chan %u, SAL_mutexTmLock 'mutexImgProc' failed\n", chan);
        return SAL_FAIL;
    }

    if (pstXspChn->stNormalData.bImgProcStarted || XSP_IMG_PROC_NONE == pstXspChn->stNormalData.u32ImgProcMode)
    {
        SAL_mutexTmUnlock(&pstUpdatePrm->mutexImgProc, __FUNCTION__, __LINE__);
        return SAL_SOK;
    }
    else
    {
        pstXspChn->stNormalData.bImgProcStarted = SAL_TRUE;
        SAL_mutexTmUnlock(&pstUpdatePrm->mutexImgProc, __FUNCTION__, __LINE__);

        SAL_mutexTmLock(&pstXspChn->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        /* 整帧回拉的图像处理，从链表中寻找处理类型为XSP_PSTG_RESERVED的节点 */
        if (XRAY_PS_PLAYBACK_FRAME == pstXspChn->enProcStat)
        {
            pNode = get_node_by_stage(pstXspChn->lstDataProc, XSP_PSTG_RESERVED);
            if (NULL != pNode)
            {
                if (pNode != DSA_LstGetHead(pstXspChn->lstDataProc))
                {
                    XSP_LOGE("chan %u, the XSP_PSTG_RESERVED node is invalid, delete it\n", chan);
                    DSA_LstDelete(pstXspChn->lstDataProc, pNode);
                    retVal = SAL_FAIL;
                    goto ERR_EXIT;
                }
            }
            else
            {
                /**
                 * 获取最后一个节点的属性，若其也为整包回拉，则认为是需要做成像处理的节点，返回SAL_EBUSY，延迟处理
                 * 这种情况该节点很有可能在XSP_PSTG_PROCING阶段，该阶段最耗时，其他阶段可能性较小
                 */
                pNode = DSA_LstGetTail(pstXspChn->lstDataProc);
                if (NULL != pNode)
                {
                    pstDataNode = (XSP_DATA_NODE *)pNode->pAdData;
                    if (XRAY_TYPE_PLAYBACK == pstDataNode->enProcType)
                    {
                        XSP_LOGW("chan %u, get XSP_PSTG_RESERVED node failed, but the tail node maybe matched, Stage: %d, bFscImgProc: %d, try again\n",
                                 chan, pstDataNode->enProcStage, pstDataNode->bFscImgProc);
                        retVal = SAL_EBUSY;
                    }
                    else
                    {
                        XSP_LOGE("chan %u, get XSP_PSTG_RESERVED node failed, and the tail node is mismatched, ProcType: %d, PbMode: %d\n",
                                 chan, pstDataNode->enProcType, pstDataNode->enPbMode);
                        retVal = SAL_FAIL;
                    }
                }
                else
                {
                    XSP_LOGE("chan %u, get XSP_PSTG_RESERVED node failed, set image process parameters directly\n", chan);
                    retVal = SAL_FAIL;
                }

                goto ERR_EXIT;
            }
        }
        else
        {
            if (DSA_LstIsFull(pstXspChn->lstDataProc))
            {
                retVal = SAL_CondWait(&pstXspChn->lstDataProc->sync, 2000, __FUNCTION__, __LINE__);
                if (SAL_FAIL == retVal)
                {
                    XSP_LOGW("chan %u, wait for 'lstDataProc' to be free timeouted, try again...\n", chan);
                    retVal = SAL_ETIMEOUT;
                    goto ERR_EXIT;
                }
            }

            pNode = DSA_LstGetIdleNode(pstXspChn->lstDataProc);
        }

        if (NULL != pNode)
        {
            if (XRAY_PS_NONE != pstXspChn->enProcStat)
            {
                pstDataNode = (XSP_DATA_NODE *)pNode->pAdData;
                if (XRAY_PS_PLAYBACK_FRAME == pstXspChn->enProcStat)
                {
                    pstDataNode->bFscImgProc = SAL_TRUE; /* 全屏图像处理请求 */
                    pstDataNode->enProcStage = XSP_PSTG_READY;
                    pstDataNode->u32DTimeItemIdx = dtime_add_item(pstXspChn->pXspDbgTime, 0, 0, 0);
                }
                else
                {
                    memset(pstDataNode, 0, offsetof(XSP_DATA_NODE, pu8RtNrawTip));
                    pstDataNode->bFscImgProc = SAL_TRUE; /* 全屏图像处理请求 */
                    pstDataNode->enProcStage = XSP_PSTG_READY;
                    if (NULL != (pNodeInst = get_node_by_stage(pstXspChn->lstDataProc, XSP_PSTG_READY)))
                    {
                        /* 将全屏图像处理的请求节点放到XSP_PSTG_READY节点之前，以快速响应 */
                        DSA_LstInst(pstXspChn->lstDataProc, pNodeInst->prev, pNode);
                    }
                    else
                    {
                        DSA_LstPush(pstXspChn->lstDataProc, pNode); /* 将节点放到队列尾 */
                    }
                    pstDataNode->u32DTimeItemIdx = dtime_add_item(pstXspChn->pXspDbgTime, 0, 0, 0);
                }
                dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_FALSE, "dspRawIn", -1, SAL_TRUE);

                SAL_CondSignal(&pstXspChn->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
                SAL_mutexTmUnlock(&pstXspChn->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
                return SAL_SOK;
            }
            else /* 因有阻塞，阻塞结束后，若处理模式已跳转到XRAY_PS_NONE，则不再处理了，直接设置图像参数 */
            {
                XSP_LOGW("chan %u, current proc stat has changed to: XRAY_PS_NONE\n", chan);
                retVal = SAL_EINTR;
                goto ERR_EXIT;
            }
        }
        else
        {
            pNode = DSA_LstGetHead(pstXspChn->lstDataProc);
            while (NULL != pNode)
            {
                snprintf(dbgInfo + strlen(dbgInfo), sizeof(dbgInfo) - strlen(dbgInfo), "%d ", ((XSP_DATA_NODE *)pNode->pAdData)->enProcStage);
                pNode = pNode->next;
            }

            XSP_LOGW("chan %u, lstDataProc is full, count %u, status %s, try again\n", chan, DSA_LstGetCount(pstXspChn->lstDataProc), dbgInfo);
            retVal = SAL_EBUSY;
            goto ERR_EXIT;
        }
    }

ERR_EXIT:
    SAL_mutexTmUnlock(&pstXspChn->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
    if (SAL_EBUSY == retVal || SAL_ETIMEOUT == retVal)
    {
        if (SAL_SOK == SAL_mutexTmLock(&pstUpdatePrm->mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
        {
            pstXspChn->stNormalData.bImgProcStarted = SAL_FALSE; /* 加入处理队列失败，重置该标志，并继续尝试 */
            SAL_mutexTmUnlock(&pstUpdatePrm->mutexImgProc, __FUNCTION__, __LINE__);
        }
        else
        {
            XSP_LOGE("chan %u, SAL_mutexTmLock 'mutexImgProc' failed\n", chan);
        }

        if (SAL_EBUSY == retVal) /* 忙时等待200ms，200ms为一个节点大概的处理耗时 */
        {
            SAL_msleep(200);
        }
    }
    else /* 其他情况直接设置图像参数，不再尝试 */
    {
        xsp_set_image_proc_param(chan);
        /* 配置色温等图像参数之后，从文件管理里小图直接切到整包回拉时，由于没有数据node所以不做全屏成像处理，但设置图像参数之后需要对padding区域填充背景颜色 */
        g_xsp_lib_api.get_anti(g_xsp_common.stXspChn[chan].handle, &s32AntiVal);
        /*重构反色后的背景颜色*/
        build_disp_bgcolor(chan, s32AntiVal);
    }

    return SAL_FAIL;
}

/**
 * @function   xsp_fsc_image_process
 * @brief      实时过包与回拉的全屏成像处理
 *
 * @param[in]  UINT32 chan                 通道号
 * @param[in]  XSP_DATA_NODE *pstDataNode  XSP模块处理节点
 * @param[out] NONE
 * @return     static INT32 成功SAL_SOK，失败SAL_FALSE
 */
static SAL_STATUS xsp_fsc_image_process(UINT32 chan, XSP_DATA_NODE *pstDataNode)
{
    INT32 i = 0;
    INT32 s32LeftDispCol = 0;
    UINT32 u32NrawWidth = 0;
    UINT32 u32NrawHeight = 0;
    UINT32 u32FscProcLen = 0;       // 全屏处理数据长度，全屏数据加上显示缓存余量
    UINT32 u32FscDataOffset = 0;    // 分段处理时算法输出全屏数据的偏移像素数
    INT32 s32SkipProcIdx = 0;       // 分段处理时第几次处理需要偏移算法输出数据地址
    XIMAGE_DATA_ST astRawInImg[2] = {0};
    XIMAGE_DATA_ST astBlendInImg[2] = {0};
    XRAY_PROC_DIRECTION enProcDir = XRAY_DIRECTION_NONE;

    SEG_BUF_DATA stBlendBuffPrm = {0};    /* 全屏的融合灰度图 */
    SEG_BUF_DATA stRawBuffPrm = {0};   /* 全屏的归一化RAW数据 */
    SAL_VideoCrop stCropPrm = {0};
    XSP_PBPROC_IN stFscProcIn = {0};
    XSP_PBPROC_OUT stFscProcOut = {0};

    XSP_CHN_PRM *pstXspChn = &g_xsp_common.stXspChn[chan];
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();

    /* 还未启动XPack时，不进行成像处理 */
    if (!pstXspChn->stNormalData.bXPackStarted)
    {
        XSP_LOGE("chan %u, XPack has not started, wait for a moment\n", chan);
        return SAL_FAIL;
    }

    /* 整包回拉使用节点数据 */
    if (XRAY_PS_PLAYBACK_FRAME == pstXspChn->enProcStat)
    {
        stFscProcIn.bUseBlend = SAL_FALSE;
        u32FscDataOffset = 0;
        stCropPrm.left = 0;
        stCropPrm.top = pstDataNode->u32PbNeighbourTop;
        stCropPrm.width = pstDataNode->stNRawInBuf.u32Width;
        stCropPrm.height = pstDataNode->stNRawInBuf.u32Height - pstDataNode->u32PbNeighbourTop - pstDataNode->u32PbNeighbourBottom;
        ximg_create_sub(&pstDataNode->stNRawInBuf, &astRawInImg[0], &stCropPrm);
        ximg_create_sub(&pstDataNode->stBlendBuf, &astBlendInImg[0], NULL);
    }
    /* 实时过包或条带回拉从xpack获取全屏raw数据 */
    else
    {
        stFscProcIn.zdata_ver = pstXspChn->enRtZVer;
        stFscProcIn.bUseBlend = SAL_TRUE;

        s32LeftDispCol = Xpack_DrvGetDispBuffStatus(chan);
        if (0 > s32LeftDispCol)
        {
            XSP_LOGW("chan %u Xpack_DrvGetDispBuffStatus failed\n", chan);
            s32LeftDispCol = 0;
        }
        u32FscProcLen = pstCapbDisp->disp_yuv_w_max + SAL_MIN(s32LeftDispCol, pstCapbXray->u32MaxSourceDist);
        u32FscProcLen = SAL_MIN(u32FscProcLen, XSP_MAX_IMGEN_WIDTH);     // 全屏处理数据长度不能超过成像算法支持最大图像宽度，FIXME：成像算法能否将支持的最大图像宽度增大为5000
        
        /* 获取当前显示的整屏图像对应的融合灰度图，RAW数据，包括低高能和原子序数（单能仅有低能数据），这些数据均是未做几何变换的 */
        if (SAL_SOK != Xpack_DrvGetScreenRaw(chan, &stRawBuffPrm, &stBlendBuffPrm, &u32FscProcLen))
        {
            XSP_LOGE("chan %u, Xpack_DrvGetScreenRaw failed\n", chan);
            return SAL_FAIL;
        }

        /* 记录后部融合灰度图 */
        stCropPrm.left = 0;
        stCropPrm.top = stBlendBuffPrm.u32LatterPartOffset;
        stCropPrm.width = stBlendBuffPrm.stFscRawData.u32Width;
        stCropPrm.height = stBlendBuffPrm.u32LatterPartLen;
        ximg_create_sub(&stBlendBuffPrm.stFscRawData, &astBlendInImg[0], &stCropPrm);

        /* 记录后部归一化数据 */
        stCropPrm.left = 0;
        stCropPrm.top = stRawBuffPrm.u32LatterPartOffset;
        stCropPrm.width = stRawBuffPrm.stFscRawData.u32Width;
        stCropPrm.height = stRawBuffPrm.u32LatterPartLen;
        ximg_create_sub(&stRawBuffPrm.stFscRawData, &astRawInImg[0], &stCropPrm);

        /* 分段后部不足，从分段前部取剩下数据 */
        if (0 < stBlendBuffPrm.u32FrontPartLen)
        {
            /* 记录后部融合灰度图 */
            stCropPrm.left = 0;
            stCropPrm.top = stBlendBuffPrm.u32FrontPartOffset;
            stCropPrm.width = stBlendBuffPrm.stFscRawData.u32Width;
            stCropPrm.height = stBlendBuffPrm.u32FrontPartLen;
            ximg_create_sub(&stBlendBuffPrm.stFscRawData, &astBlendInImg[1], &stCropPrm);

            /* 记录后部归一化数据 */
            stCropPrm.left = 0;
            stCropPrm.top = stRawBuffPrm.u32FrontPartOffset;
            stCropPrm.width = stRawBuffPrm.stFscRawData.u32Width;
            stCropPrm.height = stRawBuffPrm.u32FrontPartLen;
            ximg_create_sub(&stRawBuffPrm.stFscRawData, &astRawInImg[1], &stCropPrm);
        }

        /* 几何变换设置，预览和回拉都根据当前过包方向设置旋转镜像 */
        enProcDir = pstXspChn->stNormalData.uiRtDirection;

        if (XRAY_DIRECTION_LEFT == enProcDir)
        {
            /* 左向出图时，raw数据向左旋转，第一次处理的LatterPart位于屏幕右侧，故需跳过屏幕左侧部分数据 */
            s32SkipProcIdx = 0;
            u32FscDataOffset = stBlendBuffPrm.u32FrontPartLen;
        }
        else if (XRAY_DIRECTION_RIGHT == enProcDir)
        {
            /* 右向出图时，raw数据向右旋转，第一次处理的LatterPart位于屏幕左侧，故需在第二次处理屏幕右侧部分数据时跳过屏幕左侧部分数据 */
            s32SkipProcIdx = 1;
            u32FscDataOffset = stBlendBuffPrm.u32LatterPartLen;
        }
        
        if (SAL_SOK != Xsp_DrvSetRotateAndMirror(chan, XSP_HANDLE_RT_PB, enProcDir, XRAY_TYPE_NORMAL))
        {
            XSP_LOGW("chan: %d, dir: %d, Xsp_DrvSetRotateAndMirror failed!\n", chan, pstDataNode->enDispDir);
        }
    }

    dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_FALSE, "pipe0S", -1, SAL_TRUE);

    /* 全屏处理时将全屏显示缓冲内存宽高恢复为全屏宽高 */
    /* 分次处理 */
    for (i = 0; i < 2; i++)
    {
        u32NrawWidth = pstCapbXsp->xraw_width_resized;
        u32NrawHeight = astRawInImg[i].u32Height;

        if (0 == u32NrawHeight)
        {
            continue;
        }

        XSP_LOGI("chan %u, fsc proc:%d, dir:%d, skip idx:%d, fsc offset:%u, raw_w:%u, raw_h:%u, leftCol:%u, ProcType:%d, PbMode:%d, lstDataProc:%u, RtZVer:%d\n",
                 chan, i, pstDataNode->enDispDir, s32SkipProcIdx, u32FscDataOffset, u32NrawWidth, u32NrawHeight, s32LeftDispCol, pstDataNode->enProcType, 
                 pstDataNode->enPbMode, DSA_LstGetCount(pstXspChn->lstDataProc), pstXspChn->enRtZVer);

        stFscProcIn.neighbour_top = 0;
        stFscProcIn.neighbour_bottom = 0;
        /* 成像处理的全屏图像处理，处理流程均采用整包回拉处理模式 */
        ximg_create_sub(&astBlendInImg[i], &stFscProcIn.stBlendIn, NULL);
        ximg_create_sub(&astRawInImg[i], &stFscProcIn.stNrawIn, NULL);

        stCropPrm.top = pstCapbDisp->disp_h_top_blanking + pstCapbDisp->disp_h_top_padding;
        stCropPrm.left = (i == s32SkipProcIdx) ? u32FscDataOffset : 0;
        stCropPrm.width = u32NrawHeight;
        stCropPrm.height = u32NrawWidth;
        if (XRAY_PS_PLAYBACK_FRAME == pstXspChn->enProcStat)
        {
            /* 在stDispFscData中抠取一个回拉帧部分作为stDispDataSub */
            stCropPrm.top = 0;
            stCropPrm.height = pstCapbDisp->disp_yuv_h_max;
            ximg_create_sub(&pstDataNode->stDispFscData, &pstDataNode->stDispDataSub, &stCropPrm);
            
            /* 显示图像 */
            stCropPrm.top = pstCapbDisp->disp_h_top_blanking + pstCapbDisp->disp_h_top_padding;
            stCropPrm.height = u32NrawWidth;
            ximg_create_sub(&pstDataNode->stDispDataSub, &stFscProcOut, &stCropPrm);
            dtime_update_tag(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, 0, u32NrawWidth, u32NrawHeight);
        }
        else
        {
            pstDataNode->stDispFscData.u32Width = u32FscProcLen;
            ximg_create_sub(&pstDataNode->stDispFscData, &stFscProcOut, &stCropPrm);
            dtime_update_tag(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, 0, u32NrawWidth, u32FscProcLen);
        }

        if (0 < pstXspChn->stDumpCfg.u32DumpCnt)
        {
            if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_XRAW_BLEND) /* 输入的融合灰度图 */
            {
                ximg_dump(&stFscProcIn.stBlendIn, chan, pstXspChn->stDumpCfg.chDumpDir, "fsc-blend-in", NULL, pstXspChn->stDumpCfg.u32DumpCnt);
            }
            if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_XRAW_IN) /* 输入的归一化数据 */
            {
                ximg_dump(&stFscProcIn.stNrawIn, chan, pstXspChn->stDumpCfg.chDumpDir, "fsc-nraw-in", NULL, pstXspChn->stDumpCfg.u32DumpCnt);
            }
        }

        if (SAL_SOK != g_xsp_lib_api.playback_process(pstXspChn->handle, &stFscProcIn, &stFscProcOut))
        {
            XSP_LOGE("chan %u, playback_process failed\n", chan);
            return SAL_FAIL;
        }
        if (0 < pstXspChn->stDumpCfg.u32DumpCnt && pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_DISP_OUT)
        {
            ximg_dump(&pstDataNode->stDispFscData, chan, pstXspChn->stDumpCfg.chDumpDir, "fsc-out", NULL, pstXspChn->stDumpCfg.u32DumpCnt);
        }
        if (pstXspChn->stDumpCfg.u32DumpCnt > 0)
        {
            pstXspChn->stDumpCfg.u32DumpCnt--;
        }
    }
    dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_FALSE, "pipe0E", -1, SAL_TRUE);
    
    return SAL_SOK;
}


/**
 * @function    xsp_rtproc_pipeline_cb
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static void xsp_rtproc_pipeline_cb(void *pXspHandle, XSP_RTPROC_OUT *pstRtProcOut)
{
    UINT32 chan = ~0U, i = 0;
    BOOL bRmBlankSliceSub = SAL_FALSE;
    CHAR debugInfo[64] = {0};
    DSA_NODE *pNode = NULL;
    XSP_CHN_PRM *pstXspChn = NULL;
    XSP_DATA_NODE *pstDataNode = NULL;
    XSP_SLICE_NRAW stSliceNraw = {0};
    STREAM_ELEMENT stStrmElem = {0};
    XSP_DEBUG_SLICE_CB *pstSliceCb = NULL;
    XSP_LIB_TIME_PARM pstUp2AppTimePram; /* 更新给应用的算法库耗时 */
    XIMAGE_DATA_ST stTipRawDumpImg = {0};

    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();

    if (NULL == pXspHandle || NULL == pstRtProcOut)
    {
        XSP_LOGE("pXspHandle(%p) OR pstRtProcOut(%p) is NULL\n", pXspHandle, pstRtProcOut);
        return;
    }

    /***************************** 通过XSP句柄匹配通道号 *****************************/
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        if (pXspHandle == g_xsp_common.stXspChn[i].handle)
        {
            chan = i;
            pstXspChn = g_xsp_common.stXspChn + i;
            break;
        }
    }

    if (NULL == pstXspChn || chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("the cbinfo maybe invalid, handle: %p\n", pXspHandle);
        return;
    }

    /***************************** 通过条带号匹配XSP处理的数据节点 *****************************/
    if (SAL_SOK != SAL_mutexTmLock(&pstXspChn->lstDataProc->sync.mid, 2000, __FUNCTION__, __LINE__))
    {
        XSP_LOGE("chan %u, SAL_mutexTmLock 'lstDataProc->sync.mid' failed\n", chan);
        return;
    }

    if (DSA_LstGetCount(pstXspChn->lstDataProc) > 0)
    {
        pNode = DSA_LstGetHead(pstXspChn->lstDataProc);
        while (NULL != pNode)
        {
            if (XSP_PSTG_PROCING == ((XSP_DATA_NODE *)pNode->pAdData)->enProcStage
                && ((XSP_DATA_NODE *)pstRtProcOut->pPassthData)->u32RtSliceNo == ((XSP_DATA_NODE *)pNode->pAdData)->u32RtSliceNo)
            {
                pstDataNode = (XSP_DATA_NODE *)pNode->pAdData;
                break;
            }
            else
            {
                pNode = pNode->next;
            }
        }
    }

    SAL_mutexTmUnlock(&pstXspChn->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
    if (NULL == pstDataNode)
    {
        pNode = DSA_LstGetHead(pstXspChn->lstDataProc);
        while (NULL != pNode)
        {
            snprintf(debugInfo + strlen(debugInfo), sizeof(debugInfo) - strlen(debugInfo), "%u(%d) ",
                     ((XSP_DATA_NODE *)pNode->pAdData)->u32RtSliceNo, ((XSP_DATA_NODE *)pNode->pAdData)->enProcStage);
            pNode = pNode->next;
        }

        XSP_LOGE("chan %u, mismatch process node, SliceNo cb: %u, cur: %s\n", chan, ((XSP_DATA_NODE *)pstRtProcOut->pPassthData)->u32RtSliceNo, debugInfo);
        return;
    }

    if (SAL_SOK != pstRtProcOut->enResult)
    {
        if (SAL_SOK == SAL_mutexTmLock(&pstXspChn->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
        {
            XSP_LOGE("chan %u, RT-Process failed, result: %d, SliceNo: %u\n", chan, pstRtProcOut->enResult, pstDataNode->u32RtSliceNo);
            DSA_LstDelete(pstXspChn->lstDataProc, pNode);
            SAL_mutexTmUnlock(&pstXspChn->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pstXspChn->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
        else
        {
            XSP_LOGE("chan %u, SAL_mutexTmLock 'lstDataProc->sync.mid' failed\n", chan);
        }

        return;
    }

    /* 非TIP插入阶段，复用包裹存储的NRAW */
    if (XSP_TIPED_PROCESSING != pstRtProcOut->st_xraw_tiped.en_tiped_status)
    {
        pstDataNode->bTipNrawMultiplex = SAL_TRUE;
    }

    /* 切换方向重置包裹分割的参数 */
    if (pstXspChn->stNormalData.uiRtDirection != pstDataNode->enDispDir)
    {
        pstXspChn->stRtPackDivS.uiPackCurLine = 0;
        pstXspChn->stRtPackDivS.u32SliceCnt = 0;
        pstXspChn->stRtPackDivS.enPackTagLast = XSP_PACKAGE_NONE;
        pstXspChn->stNormalData.u32RtSliceNumAfterCls = 0; /* 重置屏幕上实时过包条带数 */
    }
    else
    {
        pstXspChn->stNormalData.u32RtSliceNumAfterCls++;
    }

    if(SAL_SOK != Xsp_DrvSyncPackDivision(chan, pstDataNode, &pstRtProcOut->st_package_indentify))
    {
        XSP_LOGE("chan %u, Xsp_DrvSyncPackDivision failed\n", chan);
    }

    /* TIP结果处理 */
    if (SAL_SOK != Xsp_DrvTipProcess(chan, pstDataNode, pstRtProcOut))
    {
        XSP_LOGE("chan %u, Xsp_DrvTipProcess failed\n", chan);
    }

    /***************************** 处理XSP输出结果 *****************************/
    dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_TRUE, "pipe0S", pstRtProcOut->st_debug.u64TimeProcPipe0S, SAL_TRUE);
    dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_TRUE, "pipe0E", pstRtProcOut->st_debug.u64TimeProcPipe0E, SAL_TRUE);
    dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_TRUE, "pipe1S", pstRtProcOut->st_debug.u64TimeProcPipe1S, SAL_TRUE);
    dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_TRUE, "pipe1E", pstRtProcOut->st_debug.u64TimeProcPipe1E, SAL_TRUE);
    dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_TRUE, "pipe2S", pstRtProcOut->st_debug.u64TimeProcPipe2S, SAL_TRUE);
    dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_TRUE, "pipe2E", pstRtProcOut->st_debug.u64TimeProcPipe2E, SAL_TRUE);

    /* 将耗时同步到应用用于debug显示 */
    pstUp2AppTimePram.timeBeforeProc = dtime_get_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, "beforeXsp");
    pstUp2AppTimePram.timeProcPipe0S = pstRtProcOut->st_debug.u64TimeProcPipe0S;
    pstUp2AppTimePram.timeProcPipe0E = pstRtProcOut->st_debug.u64TimeProcPipe0E;
    pstUp2AppTimePram.timeProcPipe1S = pstRtProcOut->st_debug.u64TimeProcPipe1S;
    pstUp2AppTimePram.timeProcPipe1E = pstRtProcOut->st_debug.u64TimeProcPipe1E;
    pstUp2AppTimePram.timeProcPipe2S = pstRtProcOut->st_debug.u64TimeProcPipe2S;
    pstUp2AppTimePram.timeProcPipe2E = pstRtProcOut->st_debug.u64TimeProcPipe2E;
    pstUp2AppTimePram.timeAfterProc  = dtime_get_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, "afterXsp");
    if(pstDspInfo != NULL)
    {
        memcpy(&pstDspInfo->stTimePram, &pstUp2AppTimePram, sizeof(XSP_LIB_TIME_PARM));
    }
    else
    {
        XSP_LOGE("error pstDspInfo = NULL\n");
    }
    /***************************** Dump输出数据 *****************************/
    if (0 < pstXspChn->stDumpCfg.u32DumpCnt)
    {
        if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_DISP_OUT)
        {
            ximg_dump(&pstDataNode->stDispDataSub, chan, pstXspChn->stDumpCfg.chDumpDir, "slice-disp", NULL, pstDataNode->u32RtSliceNo);
        }
        if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_AI_YUV)
        {
            ximg_dump(&pstDataNode->stAiYuvBuf, chan, pstXspChn->stDumpCfg.chDumpDir, "slice-aiyuv", NULL, pstDataNode->u32RtSliceNo);
        }
        if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_XRAW_OUT) /* 归一化RAW数据 */
        {
            ximg_dump(&pstDataNode->stSliceNrawBuf, chan, pstXspChn->stDumpCfg.chDumpDir, "slice-nraw", NULL, pstDataNode->u32RtSliceNo);
        }
        if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_XRAW_TIP) /* 带有TIP的归一化RAW数据 */
        {
            ximg_create(pstDataNode->stSliceNrawBuf.u32Width, pstDataNode->stSliceNrawBuf.u32Height, pstDataNode->stSliceNrawBuf.enImgFmt, 
                        NULL, pstDataNode->pu8RtNrawTip, &stTipRawDumpImg);
            ximg_dump(&stTipRawDumpImg, chan, pstXspChn->stDumpCfg.chDumpDir, "slice-tipRaw", NULL, pstDataNode->u32RtSliceNo);
        }
        if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_XRAW_BLEND) /* XSP处理的中间结果，融合灰度图 */
        {
            ximg_dump(&pstDataNode->stBlendBuf, chan, pstXspChn->stDumpCfg.chDumpDir, "slice-blend", NULL, pstDataNode->u32RtSliceNo);
        }
    }
    dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_TRUE, "afterXsp", -1, SAL_TRUE);

    /***************************** 条带RAW回调给应用 *****************************/
    /* 实时过包条带，将超分后归一化RAW数据回调给应用 */
    stSliceNraw.pSliceNrawAddr = pstRtProcOut->stNrawOut.pPlaneVir[0];
    stSliceNraw.u32Width = pstRtProcOut->stNrawOut.u32Width;
    stSliceNraw.u32Hight = pstRtProcOut->stNrawOut.u32Height;
    /* 用每一列的低能数据的最低位存储该列包裹内容信息 */
    for (i = 0; i < pstDataNode->stXRawInBuf.u32Height; i++)
    {
        *(stSliceNraw.pSliceNrawAddr + i * stSliceNraw.u32Width * 2) &= ~0x1; /* 将最低位置0 */
        *(stSliceNraw.pSliceNrawAddr + i * stSliceNraw.u32Width * 2) |= (pstDataNode->u32ColumnCont[i / 32] >> (i % 32)) & 0x1;
    }

    ximg_get_size(&pstRtProcOut->stNrawOut, &stSliceNraw.u32SliceNrawSize);
    stSliceNraw.u64SyncTime = pstDataNode->u64SyncTime;
    stSliceNraw.uiColNo = pstDataNode->u32RtSliceNo;
    for (i = 0; i < pstXspChn->stNormalData.u32RtSliceRefreshCnt; i++) 
    {
        stSliceNraw.enSliceCont |= pstDataNode->stPackDivRes[i].enSliceCont;/* 采传下发条带中只要有一个刷新条带是包裹，则置为XSP_SLICE_PACKAGE */
        bRmBlankSliceSub &= pstDataNode->stPackDivRes[i].bRmBlankSliceSub;  /* 空白比例消除 子条带都移除则移除大条带 */
    }
    if (bRmBlankSliceSub == SAL_FALSE) /* 需要删除的空白条带直接不回调 */
    {
        stStrmElem.type = STREAM_ELEMENT_SLICE_NRAW;
        stStrmElem.chan = chan;
        SystemPrm_cbFunProc(&stStrmElem, (unsigned char *)&stSliceNraw, sizeof(XSP_SLICE_NRAW));

        dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_TRUE, "sliceCb", -1, SAL_TRUE);
        pstSliceCb = &pstXspChn->stDbgSliceCb[pstXspChn->u32DbgSliceCbIdx];
        pstSliceCb->pSliceNrawAddr = stSliceNraw.pSliceNrawAddr;
        pstSliceCb->u32Width = stSliceNraw.u32Width;
        pstSliceCb->u32Hight = stSliceNraw.u32Hight;
        pstSliceCb->u32SliceNrawSize = stSliceNraw.u32SliceNrawSize;
        pstSliceCb->uiColNo = stSliceNraw.uiColNo;
        pstSliceCb->enSliceCont = stSliceNraw.enSliceCont;
        pstSliceCb->u32Top = pstDataNode->stPackDivRes[0].u32Top;
        pstSliceCb->u32Bottom = pstDataNode->stPackDivRes[0].u32Bottom;
        pstXspChn->u32DbgSliceCbIdx = (pstXspChn->u32DbgSliceCbIdx + 1) % XSP_DEBUG_SLICE_CB_NUM;
    }
    
    // ximg_fill_color(&pstDataNode->stDispDataSub, pstDspInfo->dspCapbPar.blanking_top + pstDspInfo->dspCapbPar.padding_top, 
    //                 (pstRtProcOut->st_debug.u32SliceNo % 10 + 1) * 64, 0, 4, 0xFFFFFF00);

    // if (pstRtProcOut->st_package_indentify.blank_prob > 0)
    // {
    //     ximg_fill_color(&pstDataNode->stDispDataSub, pstDspInfo->dspCapbPar.blanking_top + pstDspInfo->dspCapbPar.padding_top,
    //                     pstRtProcOut->st_package_indentify.blank_prob * 10, 4, 4, 0xFF000000);
    // }
    /***************************** 切换处理状态，准备发送给后级 *****************************/
    if (SAL_SOK == SAL_mutexTmLock(&pstXspChn->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
    {
        pstDataNode->enProcStage = XSP_PSTG_PROCED; /* 转入处理完成状态 */
        SAL_mutexTmUnlock(&pstXspChn->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
        SAL_CondSignal(&pstXspChn->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
    }
    else
    {
        XSP_LOGE("chan %u, SAL_mutexTmLock 'lstDataProc->sync.mid' failed\n", chan);
    }

    return;
}

/**
 * @function   Xsp_HalModProcess
 * @brief      XSP成像处理
 * @param[in]  UINT32 chan                        算法通道
 * @param[in]  XSP_XRAY_DATA_IN *pstXrayDataIn    输入数据
 * @param[in]  XSP_XRAY_DATA_OUT *pstXrayDataOut  输出数据
 * @param[out] None
 * @return     static INT32                       成功SAL_SOK，失败SAL_FALSE
 */
static SAL_STATUS Xsp_HalModProcess(UINT32 chan, XSP_DATA_NODE *pstDataNode)
{
    SAL_STATUS retVal = SAL_SOK;
    UINT32 corr_data_size = 0;
    UINT32 xraw_in_width = 0, xraw_in_height = 0; /* 输入XRAW数据图像分辨率 */
    UINT32 nraw_out_width = 0, nraw_out_height = 0; /* 实时预览，输出的NRAW条带数据图像分辨率 */
    CHAR dumpName[128] = {0};

    SAL_VideoCrop stCropPrm = {0};
    XSP_RTPROC_IN stRtProcIn = {0};
    XSP_RTPROC_OUT stRtProcOut = {0};
    XSP_PBPROC_IN stPbProcIn = {0};
    XSP_PBPROC_OUT stPbProcOut = {0};

    XSP_CORRECT_TYPE enCorrectType = XSP_CORR_UNDEF;

    void *handle = NULL;
    XRAY_DEADPIXEL_RESULT *p_dp_info = NULL;
    XSP_TIP_PROCESS *pstTipProcess = NULL;
    XSP_CHN_PRM *pstXspChn = NULL;
    XSP_TIP_DATA_ST *pstTip = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    CAPB_DISP *pstCapbDisp = capb_get_disp();

    XSP_CHECK_PTR_IS_NULL(pstCapXrayIn, SAL_FAIL);
    XSP_CHECK_PTR_IS_NULL(pstDataNode, SAL_FAIL);
    
    pstXspChn = &g_xsp_common.stXspChn[chan];
    handle = pstXspChn->handle;
    pstTipProcess = &pstXspChn->stTipProcess;
    xraw_in_width = pstDataNode->stXRawInBuf.u32Width;
    xraw_in_height = pstDataNode->stXRawInBuf.u32Height;
    p_dp_info = xraw_dp_get_info(chan);

    if (XRAY_TYPE_NORMAL == pstDataNode->enProcType || XRAY_TYPE_PSEUDO_BLANK == pstDataNode->enProcType) /* 实时过包 */
    {
        /*tip参数*/
        pstTip = &g_xsp_common.stTipAttr.stTipData;

        stRtProcIn.st_tip_inject.b_enable = pstTipProcess->uiEnable;
        if (SAL_TRUE == stRtProcIn.st_tip_inject.b_enable)
        {
            XSP_LOGI("chan %d: Tip process enable [%d].\n", chan, pstTipProcess->uiEnable);
            stRtProcIn.st_tip_inject.position = pstTipProcess->uiCol; /*研究院算法需要输入tip位置信息*/
            stRtProcIn.st_tip_inject.tip_width = pstTip->stTipNormalized[chan].uiXrayW;
            stRtProcIn.st_tip_inject.tip_height = pstTip->stTipNormalized[chan].uiXrayH;
            stRtProcIn.st_tip_inject.p_tip_normalized = pstTip->stTipNormalized[chan].pXrayBuf;

            /*复位*/
            pstTipProcess->uiEnable = SAL_FALSE;
        }

        /* 几何变换设置 */
        retVal = Xsp_DrvSetRotateAndMirror(chan, XSP_HANDLE_RT_PB, pstDataNode->enDispDir, XRAY_TYPE_NORMAL);
        if (SAL_SOK != retVal)
        {
            XSP_LOGW("chan: %d, dir: %d, Xsp_DrvSetRotateAndMirror failed!\n", chan, pstDataNode->enDispDir);
        }

        nraw_out_width = (UINT32)(pstCapbXsp->resize_width_factor * xraw_in_width);
        nraw_out_height = (UINT32)(pstCapbXsp->resize_height_factor * xraw_in_height);

        /* 实时过包输入数据 */
        stRtProcIn.bPseudo = (XRAY_TYPE_PSEUDO_BLANK == pstDataNode->enProcType) ? SAL_TRUE : SAL_FALSE;
        stRtProcIn.pPassthData = pstDataNode;
        stRtProcIn.st_debug.u32SliceNo = pstDataNode->u32RtSliceNo;

        stCropPrm.top = 0;
        stCropPrm.left = 0;
        stCropPrm.width = xraw_in_width;
        stCropPrm.height = xraw_in_height;
        ximg_create_sub(&pstDataNode->stXRawInBuf, &stRtProcIn.stXrawIn, &stCropPrm);
        /* 未超分的条带raw数据 */
        pstDataNode->stSliceOriNrawBuf.u32Height = xraw_in_height;
        ximg_create_sub(&pstDataNode->stSliceOriNrawBuf, &stRtProcOut.stOriNrawOut, NULL);

        /* 由于超分的条带raw数据回调需要高低能数据连续，所以将raw构造成一块连续内存图像 */
        pstDataNode->stSliceNrawBuf.u32Height = nraw_out_height;
        ximg_set_dimension(&pstDataNode->stSliceNrawBuf, nraw_out_width, nraw_out_width, SAL_FALSE, pstXspChn->u32XspBgColorStd);
        ximg_create_sub(&pstDataNode->stSliceNrawBuf, &stRtProcOut.stNrawOut, NULL);
        stRtProcOut.st_xraw_tiped.p_xraw_normalized = (U16 *)pstDataNode->pu8RtNrawTip;
        stRtProcOut.st_xraw_tiped.tiped_pos = -1; /*自研算法检测后输出tip位置信息*/

        /* 融合图像 */
        pstDataNode->stBlendBuf.u32Height = nraw_out_height;
        ximg_create_sub(&pstDataNode->stBlendBuf, &stRtProcOut.stBlendOut, NULL);

        /* 在stDispFscData中抠取一个条带部分作为stDispDataSub */
        stCropPrm.left = 0;
        stCropPrm.top = 0;
        stCropPrm.width = nraw_out_height;
        stCropPrm.height = pstCapbDisp->disp_yuv_h_max;
        ximg_create_sub(&pstDataNode->stDispFscData, &pstDataNode->stDispDataSub, &stCropPrm);

        /* 显示图像 */
        stCropPrm.left = 0;
        stCropPrm.top = pstCapbDisp->disp_h_top_blanking + pstCapbDisp->disp_h_top_padding;
        stCropPrm.width = nraw_out_height;
        stCropPrm.height = nraw_out_width;
        ximg_create_sub(&pstDataNode->stDispDataSub, &stRtProcOut.stDispOut, &stCropPrm);

        /* 送智能识别图像 */
        pstDataNode->stAiYuvBuf.u32Width = xraw_in_height;
        ximg_create_sub(&pstDataNode->stAiYuvBuf, &stRtProcOut.stAiYuvOut, NULL);

        if (0 < pstXspChn->stDumpCfg.u32DumpCnt)
        {
            if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_XRAW_IN)
            {
                if (pstDataNode->enProcType == XRAY_TYPE_PSEUDO_BLANK)
                {
                    ximg_dump(&stRtProcIn.stXrawIn, chan, pstXspChn->stDumpCfg.chDumpDir, "slice-blank", NULL, pstDataNode->u32RtSliceNo);
                }
                else
                {
                    ximg_dump(&stRtProcIn.stXrawIn, chan, pstXspChn->stDumpCfg.chDumpDir, "slice-xraw-in", NULL, pstDataNode->u32RtSliceNo);
                }
            }
            if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_RTPIPELINE)
            {
                stRtProcIn.st_debug.bDumpEnable = SAL_TRUE;
                strcpy(stRtProcIn.st_debug.chDumpDir, pstXspChn->stDumpCfg.chDumpDir);
            }
        }
        if (pstDataNode->enProcType == XRAY_TYPE_PSEUDO_BLANK)
        {
            pstDataNode->enProcType = XRAY_TYPE_NORMAL; // 统一为正常过包数据
        }

        /* 实时过包处理流程，包括成像处理，包裹检测，难穿透识别，可疑物识别，tip注入等*/
        retVal = g_xsp_lib_api.rtpreview_process(handle, stRtProcIn, &stRtProcOut);
        if (SAL_SOK != retVal)
        {
            if (SAL_EINTR == retVal)
            {
                XSP_LOGW("chan %u, rtpreview_process return SAL_EINTR\n", chan);
            }
            else
            {
                XSP_LOGE("chan %u, rtpreview_process failed, errno: %d\n", chan, retVal);
                retVal = SAL_FAIL;
            }
        }
    }
    else if (XRAY_TYPE_PLAYBACK == pstDataNode->enProcType) /* 回拉 */
    {
        /* 几何变换设置 */
        retVal = Xsp_DrvSetRotateAndMirror(chan, XSP_HANDLE_RT_PB, pstDataNode->enDispDir, XRAY_TYPE_PLAYBACK);
        if (SAL_SOK != retVal)
        {
            XSP_LOGW("chan: %d, dir: %d, Xsp_DrvSetRotateAndMirror failed!\n", chan, pstDataNode->enDispDir);
        }

        xraw_in_width = pstDataNode->stNRawInBuf.u32Width;
        xraw_in_height = pstDataNode->stNRawInBuf.u32Height - XSP_NEIGHBOUR_H_MAX * 2;

        /* 条带回拉时使用高低能数据 */
        stPbProcIn.bUseBlend = SAL_FALSE;
        stPbProcIn.zdata_ver = pstDataNode->stPbPack[0].stPackSplit.uiZdataVersion; /* 默认取第一个包裹的Z值版本号 */
        stPbProcIn.neighbour_top = pstDataNode->u32PbNeighbourTop;
        stPbProcIn.neighbour_bottom = pstDataNode->u32PbNeighbourBottom;

        pstDataNode->stNRawInBuf.u32Height = xraw_in_height + stPbProcIn.neighbour_top + stPbProcIn.neighbour_bottom;
        ximg_create_sub(&pstDataNode->stNRawInBuf, &stPbProcIn.stNrawIn, NULL);
        pstDataNode->stBlendBuf.u32Height = pstDataNode->stNRawInBuf.u32Height;
        ximg_create_sub(&pstDataNode->stBlendBuf, &stPbProcIn.stBlendIn, NULL);

        /* 在stDispFscData中抠取一个回拉帧部分作为stDispDataSub */
        stCropPrm.left = 0;
        stCropPrm.top = 0;
        stCropPrm.width = xraw_in_height;
        stCropPrm.height = pstCapbDisp->disp_yuv_h_max;
        ximg_create_sub(&pstDataNode->stDispFscData, &pstDataNode->stDispDataSub, &stCropPrm);
        
        /* 显示图像 */
        stCropPrm.left = 0;
        stCropPrm.top = pstCapbDisp->disp_h_top_blanking + pstCapbDisp->disp_h_top_padding;
        stCropPrm.width = xraw_in_height;
        stCropPrm.height = xraw_in_width;
        ximg_create_sub(&pstDataNode->stDispDataSub, &stPbProcOut, &stCropPrm);

        if (0 < pstXspChn->stDumpCfg.u32DumpCnt && pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_XRAW_IN)
        {
            ximg_dump(&stPbProcIn.stNrawIn, chan, pstXspChn->stDumpCfg.chDumpDir, "pb-nraw-in", NULL, pstXspChn->stDumpCfg.u32DumpCnt);
        }
        dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_FALSE, "pipe0S", -1, SAL_TRUE);

        /*回拉处理流程*/
        retVal = g_xsp_lib_api.playback_process(handle, &stPbProcIn, &stPbProcOut);
        if (SAL_SOK == retVal)
        {
            /* 整包回拉的数据会被复用，在playback_process接口中做过原子序数的转换，更新Z版本号与算法一致 */
            if (XRAY_PS_PLAYBACK_FRAME == pstXspChn->enProcStat)
            {
                pstDataNode->stPbPack[0].stPackSplit.uiZdataVersion = pstXspChn->enRtZVer;
            }

            if (0 < pstXspChn->stDumpCfg.u32DumpCnt)
            {
                if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_XRAW_BLEND) /* XSP处理的中间结果，融合灰度图 */
                {
                    ximg_dump(&stPbProcIn.stBlendIn, chan, pstXspChn->stDumpCfg.chDumpDir, "pb-blend-in", NULL, pstXspChn->stDumpCfg.u32DumpCnt);
                }
                if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_DISP_OUT)
                {
                    ximg_dump(&pstDataNode->stDispFscData, chan, pstXspChn->stDumpCfg.chDumpDir, "pb-disp", NULL, pstXspChn->stDumpCfg.u32DumpCnt);
                }
            }
        }
        else
        {
            XSP_LOGE("chan %u, playback_process failed\n", chan);
        }
        dtime_add_time_point(pstXspChn->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_FALSE, "pipe0E", -1, SAL_TRUE);
    }
    else if (XRAY_TYPE_CORREC_FULL == pstDataNode->enProcType) /* 满载数据 */
    {
        /* 基于满载数据检测到的暗坏点，此暗坏点上的本底数据也要校正并重新设置 */
        if (NULL != p_dp_info && NULL != pstXspChn->pEmptyLoadData && p_dp_info->deadPixelNum > 0)
        {
            if (SAL_SOK == xsp_get_correction_data(handle, XSP_CORR_EMPTYLOAD, pstXspChn->pEmptyLoadData, &corr_data_size))
            {
                xraw_dp_correct(p_dp_info, pstXspChn->pEmptyLoadData, xraw_in_width, 1);
                if (SAL_SOK == xsp_set_correction_data(handle, XSP_CORR_EMPTYLOAD, pstXspChn->pEmptyLoadData, xraw_in_width, 1))
                {
                    XSP_LOGI("chan[%u], update emptyload correction success, width: %u, height: %u\n", \
                             chan, xraw_in_width, 1);
                }
                else
                {
                    XSP_LOGE("chan[%u], update emptyload correction failed, width: %u, height: 1\n", \
                             chan, xraw_in_width);
                }
            }
        }

        enCorrectType = XSP_CORR_FULLLOAD;
    }
    else if (XRAY_TYPE_CORREC_ZERO == pstDataNode->enProcType) /* 本底数据 */
    {
        /* 基于本底数据检测到的亮坏点，此亮坏点上的满载数据也要校正并重新设置 */
        if (NULL != p_dp_info && NULL != pstXspChn->pFullLoadData && p_dp_info->deadPixelNum > 0)
        {
            if (SAL_SOK == xsp_get_correction_data(handle, XSP_CORR_FULLLOAD, pstXspChn->pFullLoadData, &corr_data_size))
            {
                xraw_dp_correct(p_dp_info, pstXspChn->pFullLoadData, xraw_in_width, 1);
                if (SAL_SOK == xsp_set_correction_data(handle, XSP_CORR_FULLLOAD, pstXspChn->pFullLoadData, xraw_in_width, 1))
                {
                    XSP_LOGI("chan[%u], update fullload correction success, width: %u, height: %u\n", \
                             chan, xraw_in_width, 1);
                }
                else
                {
                    XSP_LOGE("chan[%u], update fullload correction failed, width: %u, height: %u\n", \
                             chan, xraw_in_width, 1);
                }
            }
        }

        enCorrectType = XSP_CORR_EMPTYLOAD;
    }
    else if (XRAY_TYPE_AUTO_CORR_FULL == pstDataNode->enProcType) /* 自动满载数据 */
    {
        enCorrectType = XSP_CORR_AUTOFULL;
    }
    else
    {
        XSP_LOGW("xsp process can not handle this type: %d\n", pstDataNode->enProcType);
        retVal = SAL_EINTR;
    }

    /* 更新模板 */
    if (XSP_CORR_UNDEF != enCorrectType)
    {
        if (SAL_SOK == xsp_set_correction_data(handle, enCorrectType, (UINT16 *)pstDataNode->stXRawInBuf.pPlaneVir[0], xraw_in_width, xraw_in_height))
        {
            if (XSP_CORR_AUTOFULL != enCorrectType)
            {
                XSP_LOGW("chan %u, update correction template success, type:%d, width:%u, height:%u, col:%d\n", \
                         chan, enCorrectType, xraw_in_width, xraw_in_height, pstDataNode->u32RtSliceNo);
            }
            retVal = SAL_EINTR;
        }
        else
        {
            XSP_LOGE("chan %u, update correction template failed, type:%d, width: %u, height: %u, col:%d\n", \
                     chan, enCorrectType, xraw_in_width, xraw_in_height, pstDataNode->u32RtSliceNo);
            retVal = SAL_FAIL;
        }

        if (pstXspChn->stDumpCfg.u32DumpCnt > 0)
        {
            if (pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_FULL_IN
                || pstXspChn->stDumpCfg.u32DumpDp & XSP_DDP_ZERO_IN)
            {
                snprintf(dumpName, sizeof(dumpName), "%s/cor_ch%u_%u_w%u_h%u_n%u_type%d.xraw", pstXspChn->stDumpCfg.chDumpDir,
                         chan, pstXspChn->stDbgStatus.u32ProcCnt, xraw_in_width, xraw_in_height, pstDataNode->u32RtSliceNo, enCorrectType);
                SAL_WriteToFile(dumpName, 0, SEEK_SET, pstDataNode->stXRawInBuf.pPlaneVir[0], xraw_in_width * xraw_in_height * pstCapbXsp->xsp_original_raw_bw);
            }
        }
    }

    return retVal;
}

/**
 * @function   Xsp_DrvProcThread
 * @brief      XSP成像数据处理线程
 * @param[in]  void *prm  参数
 * @param[out] None
 * @return     void *     无
 */
static void *Xsp_DrvProcThread(void *args)
{
    INT32 s32Ret = SAL_SOK;
    INT32 s32AntiVal = 0;
    CHAR sTaskName[SAL_THR_NAME_LEN_MAX] = {0};
    UINT32 i = 0, u32Chn = 0;
    CHAR dbgInfo[128] = {0};
    DSA_NODE *pNode = NULL;
    DSA_NODE *pNodeForPrint = NULL;
    XSP_DATA_NODE *pstDataNode = NULL;
    XIMAGE_DATA_ST *pstRawInBuf = NULL; 
    XSP_CHN_PRM *pstXspChnPrm = (XSP_CHN_PRM *)args;
    XSP_NORMAL_DATA *pstNormalData = NULL;
    debug_time pXspDbgTime = NULL;
    XSP_UPDATE_PRM *pstUpdatePrm = &g_xsp_common.stUpdatePrm;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();

    if ((NULL == args) || (NULL == pstCapbDisp) || (NULL == pstCapbXrayIn) || (NULL == pstCapbXsp))
    {
        XSP_LOGE("args[%p] OR CapbDisp[%p] OR CapXrayIn[%p] OR CapbXsp[%p] is NULL\n",
                 args, pstCapbDisp, pstCapbXrayIn, pstCapbXsp);
        return NULL;
    }

    u32Chn = pstXspChnPrm->u32Chn;
    if (u32Chn >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("u32Chn[%u] is invalid, range:[0, %u)\n", u32Chn, pstCapbXrayIn->xray_in_chan_cnt);
        return NULL;
    }

    pstNormalData = &pstXspChnPrm->stNormalData;

    /* 修改系统默认线程名称 */
    snprintf(sTaskName, sizeof(sTaskName), "xsp_proc-%d", u32Chn);
    prctl(PR_SET_NAME, (unsigned long)sTaskName);

    /* 绑核处理，通道0使用逻辑核2，通道1使用逻辑核3 */
    /* SAL_SetThreadCoreBind((u32Chn % 2) + 2); */

    while (1)
    {
        SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (NULL == (pNode = get_node_by_stage(pstXspChnPrm->lstDataProc, XSP_PSTG_READY)))
        {
            SAL_CondWait(&pstXspChnPrm->lstDataProc->sync, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }

        pstDataNode = (XSP_DATA_NODE *)pNode->pAdData;
        pstDataNode->enProcStage = XSP_PSTG_PROCING; /* 转入正在处理的状态 */
        SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);

        pXspDbgTime = pstXspChnPrm->pXspDbgTime;

        if (!pstDataNode->bFscImgProc)
        {
            if (XRAY_TYPE_NORMAL == pstDataNode->enProcType || XRAY_TYPE_PSEUDO_BLANK == pstDataNode->enProcType)
            {
                dtime_add_time_point(pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_TRUE, "beforeXsp", -1, SAL_TRUE);
                pstRawInBuf = &pstDataNode->stXRawInBuf;
            }
            else
            {
                dtime_add_time_point(pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_FALSE, "beforeXsp", -1, SAL_TRUE);
                pstRawInBuf = &pstDataNode->stNRawInBuf;
            }
            if ((pstRawInBuf->u32Width > pstCapbXsp->xraw_width_resized_max)
                || (pstRawInBuf->u32Height > pstCapbXsp->xraw_height_resized_max + XSP_NEIGHBOUR_H_MAX * 2))
            {
                XSP_LOGW("chan %u, the u32RawInWidth[%u] OR u32RawInHeight[%u] is invalid, "
                         "width shold be equal to %u, height should be less than %u\n",
                         u32Chn, pstRawInBuf->u32Width, pstRawInBuf->u32Height,
                         pstCapbXsp->xraw_width_resized_max, pstCapbXsp->xraw_height_resized_max + XSP_NEIGHBOUR_H_MAX * 2);
                SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode); /* 从链表中删除该节点 */
                SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
                SAL_CondSignal(&pstXspChnPrm->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
                continue;
            }

            /* 对节点数据对应的属性初始化 */
            if (XRAY_TYPE_NORMAL == pstDataNode->enProcType || XRAY_TYPE_PSEUDO_BLANK == pstDataNode->enProcType)
            {
                /* 因实时过包条带有缓存 */
                SAL_SWAP(pstDataNode->u32RtSliceNo, pstNormalData->u32RtSliceNoLast); /* 实时过包条带的编号调整为上一个的 */
                SAL_SWAP(pstDataNode->u64SyncTime, pstNormalData->u64RtSliceSyncTimeLast); /* 实时过包条带的时间调整为上一个的 */
                SAL_SWAP(pstDataNode->u64TrigTime, pstNormalData->u64RtSliceTrigTimeLast); /* 实时过包包裹触发光障的时间调整为上一个的 */
                for (i = 0; i < 8; i++) /* 实时过包条带的光障触发信号调整为上一个的 */
                {
                    SAL_SWAP(pstDataNode->u32ColumnCont[i], pstNormalData->u32ColumnContLast[i]);
                }
            }

            s32Ret = Xsp_HalModProcess(u32Chn, pstDataNode);
            if (s32Ret != SAL_SOK) /* 校正数据该接口返回SAL_EINTR，直接continue */
            {
                SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode); /* 从链表中删除该节点 */
                SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
                SAL_CondSignal(&pstXspChnPrm->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
                if (XRAY_TYPE_CORREC_ZERO != pstDataNode->enProcType
                    && XRAY_TYPE_CORREC_FULL != pstDataNode->enProcType
                    && XRAY_TYPE_AUTO_CORR_FULL != pstDataNode->enProcType)
                {
                    XSP_LOGW("chan %u, Xsp_HalModProcess failed, col:%d, errno: %d\n", u32Chn, pstDataNode->u32RtSliceNo, s32Ret);
                    sal_msleep_by_nano(10); /* 非校正数据处理失败后，等待一个时间片 */
                }

                continue;
            }

            /**
             * @warning XRAY_TYPE_NORMAL类型的数据，
             *          在xsp_rtproc_pipeline_cb()中异步切换处理状态为XSP_PSTG_PROCED，
             *          此线程中无需继续处理
             */

            if (XRAY_TYPE_PLAYBACK == pstDataNode->enProcType)
            {
                dtime_add_time_point(pstXspChnPrm->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_FALSE, "afterXsp", -1, SAL_TRUE);

                pstNormalData->uiPbDirection = pstDataNode->enDispDir;
                if (SAL_SOK == SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
                {
                    pstDataNode->enProcStage = XSP_PSTG_PROCED; /* 转入处理完成状态 */
                    SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
                    SAL_CondSignal(&pstXspChnPrm->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
                }
                else
                {
                    XSP_LOGE("chan %u, SAL_mutexTmLock 'lstDataProc->sync.mid' failed\n", u32Chn);
                }
            }
        }
        else
        {
            dtime_add_time_point(pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_FALSE, "beforeXsp", -1, SAL_TRUE);
            /* 补充全屏处理数据节点信息 */
            SAL_mutexTmLock(&pstXspChnPrm->condProcStat.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

            if (XRAY_PS_RTPREVIEW == pstXspChnPrm->enProcStat)
            {
                SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
                pstDataNode->enProcType = XRAY_TYPE_NORMAL;
                pstDataNode->enDispDir = pstNormalData->uiRtDirection; /* 设置显示方向，与实时过包方向一致 */
            }
            else if (XRAY_PS_PLAYBACK_MASK & pstXspChnPrm->enProcStat)
            {
                SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
                if (XRAY_PB_SLICE == pstNormalData->enPbMode)
                {
                    pstDataNode->enProcType = XRAY_TYPE_PLAYBACK;
                    pstDataNode->enDispDir = pstNormalData->uiPbDirection; /* 设置显示方向，与最后一次回拉方向一致 */
                    pstDataNode->enPbMode = pstNormalData->enPbMode;
                }
            }
            else /* 当处理模式已跳转到XRAY_PS_NONE，则直接返回 */
            {
                SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);

                if (SAL_SOK == SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
                {
                    DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode); /* 从链表中删除该节点，等待下次继续处理 */
                    SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
                    SAL_CondSignal(&pstXspChnPrm->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
                }
                else
                {
                    XSP_LOGE("chan %u, SAL_mutexTmLock 'lstDataProc->sync.mid' failed\n", u32Chn);
                }

                SAL_mutexTmLock(&pstUpdatePrm->mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                pstXspChnPrm->stNormalData.bImgProcStarted = SAL_FALSE; /* 重置未处理标志 */
                SAL_mutexTmUnlock(&pstUpdatePrm->mutexImgProc, __FUNCTION__, __LINE__);

                XSP_LOGW("chan %u, current proc stat has changed to: XRAY_PS_NONE, delete this node\n", u32Chn);
                continue;
            }

            /* 成像参数配置，需要在设置参数之前等待队列清空，否则设置参数时队列中还有正在处理的条带节点 */
            if (SAL_SOK != Xsp_WaitProcListEmpty(u32Chn, SAL_FALSE, 200, 10))
            {
                pNodeForPrint = pNode;
                while (NULL != pNodeForPrint)
                {
                    snprintf(dbgInfo + strlen(dbgInfo), sizeof(dbgInfo) - strlen(dbgInfo), "%d ", ((XSP_DATA_NODE *)pNodeForPrint->pAdData)->enProcStage);
                    pNodeForPrint = pNodeForPrint->next;
                }

                XSP_LOGW("chan %u, Xsp_WaitProcListEmpty timeout, node count: %u, node status %s\n",
                         u32Chn, DSA_LstGetCount(pstXspChnPrm->lstDataProc), dbgInfo);
                memset(dbgInfo, 0, sizeof(dbgInfo));
            }

            if (SAL_SOK == xsp_set_image_proc_param(u32Chn))
            {
                /* 成像参数设置成功，做全屏成像处理 */
                s32Ret = xsp_fsc_image_process(u32Chn, pstDataNode);
                /* 避免快速按反色按钮上下padding被错误修改问题，在图像全屏处理之后再修改padding填充 */
                g_xsp_lib_api.get_anti(g_xsp_common.stXspChn[u32Chn].handle, &s32AntiVal);
                /*重构反色后的背景颜色*/
                build_disp_bgcolor(u32Chn, s32AntiVal);
                if (s32Ret != SAL_SOK)
                {
                    XSP_LOGE("chan %u, xsp_fsc_image_process failed\n", u32Chn);

                    SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                    if (XRAY_PS_PLAYBACK_FRAME == pstXspChnPrm->enProcStat)
                    {
                        /* 删除历史的XSP_PSTG_RESERVED节点 */
                        while (NULL != (pNode = get_node_by_stage(pstXspChnPrm->lstDataProc, XSP_PSTG_RESERVED)))
                        {
                            DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode);
                        }

                        pstDataNode->enProcStage = XSP_PSTG_RESERVED; /* 新的节点成为XSP_PSTG_RESERVED */
                    }
                    else
                    {
                        DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode); /* 从链表中删除该节点，放弃该次成像处理 */
                    }

                    SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
                    SAL_CondSignal(&pstXspChnPrm->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
                    continue;
                }

                if (SAL_SOK == SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
                {
                    pstDataNode->enProcStage = XSP_PSTG_PROCED; /* 转入处理完成状态 */
                    SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
                    SAL_CondSignal(&pstXspChnPrm->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
                }
                else
                {
                    XSP_LOGE("chan %u, SAL_mutexTmLock 'lstDataProc->sync.mid' failed\n", u32Chn);
                }
            }
            else
            {
                XSP_LOGE("chan %u, xsp_set_image_proc_param failed\n", u32Chn);

                SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                if (XRAY_PS_PLAYBACK_FRAME == pstXspChnPrm->enProcStat)
                {
                    /* 删除历史的XSP_PSTG_RESERVED节点 */
                    while (NULL != (pNode = get_node_by_stage(pstXspChnPrm->lstDataProc, XSP_PSTG_RESERVED)))
                    {
                        DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode);
                    }

                    pstDataNode->enProcStage = XSP_PSTG_RESERVED; /* 新的节点成为XSP_PSTG_RESERVED */
                }
                else
                {
                    DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode); /* 从链表中删除该节点，放弃该次成像处理 */
                }

                SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
                SAL_CondSignal(&pstXspChnPrm->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);

                SAL_mutexTmLock(&pstUpdatePrm->mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                pstXspChnPrm->stNormalData.bImgProcStarted = SAL_FALSE; /* 重置未处理标志 */
                pstXspChnPrm->stNormalData.u32ImgProcMode = XSP_IMG_PROC_NONE;
                SAL_mutexTmUnlock(&pstUpdatePrm->mutexImgProc, __FUNCTION__, __LINE__);
            }

            dtime_add_time_point(pstXspChnPrm->pXspDbgTime, pstDataNode->u32DTimeItemIdx, SAL_FALSE, "afterXsp", -1, SAL_TRUE);
        }
    }

    return NULL;
}

/**
 * @function   Xsp_DrvSendThread
 * @brief      XSP成像输出发送线程
 * @param[in]  void *prm  线程参数，XSP_CHN_PRM
 * @param[out] None
 * @return     void *     无
 */
void *Xsp_DrvSendThread(void *args)
{
    UINT32 i = 0, j = 0, k = 0, u32Chn = 0;
    SAL_STATUS retVal = SAL_SOK;
    UINT32 u32PbPkgCnt = 0;                 /* 单个回拉帧中的包裹个数 */
    BOOL bPbNodeReversed = SAL_FALSE; /* 是否保留该整包回拉数据节点，用于后续的全屏成像处理 */
    BOOL bDTimeAutoSort = SAL_TRUE;
    UINT32 u32XpackRemainSize = 0;
    UINT32 u32OriNrawWidth = 0, u32OriNrawHeight = 0;       // 未超分的raw数据宽高
    UINT32 u32NrawWidth = 0, u32NrawHeight = 0;             // 超分后的raw数据宽高
    UINT32 u32SendOriNrawHeight = 0, u32SendNrawHeight = 0; // 超分前后每次发送的raw数据高
    UINT32 u32SendDispWidth = 0;
    UINT32 u32SendAiYuvWidth = 0;
    CHAR sTaskName[SAL_THR_NAME_LEN_MAX] = {0};

    SAL_VideoCrop stCropPrm = {0};
    XIMAGE_DATA_ST stTipRawImg = {0};
    XPACK_DATA4DISPLAY stData4Disp = {0};
    XPACK_DATA4PACKAGE_RT stData4PackRt = {0};
    XPACK_DATA4PACKAGE_PB astData4PackPb[XSP_PACK_NUM_MAX] = {0};
    XPACK_PACKAGE_LABEL stLabelTip = {0}, *pstLabelTip = NULL;

    DSA_NODE *pNode = NULL;
    XSP_DATA_NODE *pstDataNode = NULL;
    XSP_TRANSFER_INFO stTransferInfo = {0};
    // XRAY_PB_PARAM stPbParam = {0};
    XSP_PACK_DIV_RES *pstNodePackDiv = NULL;
    SVA_RECT_F *pstRectFt = NULL;
    XSP_CHN_PRM *pstXspChnPrm = args;
    XSP_NORMAL_DATA *pstNormalData = NULL;

    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    XSP_DEBUG_PB2XPACK *pstPb2Xpack = NULL;

    struct pb_seg_attr
    {
        UINT32 u32DispOffset;
        UINT32 u32DispWidth;
    } stPbSeg[XSP_PACK_NUM_MAX];

    if ((NULL == args) || (NULL == pstCapbDisp) || (NULL == pstCapbXrayIn) || (NULL == pstCapbXsp))
    {
        XSP_LOGE("args[%p] OR CapbDisp[%p] OR CapbXrayIn[%p] OR CapbXsp[%p] is NULL\n",
                 args, pstCapbDisp, pstCapbXrayIn, pstCapbXsp);
        return NULL;
    }

    u32Chn = pstXspChnPrm->u32Chn;
    if (u32Chn >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("u32Chn[%u] is invalid, range:[0, %u)\n", u32Chn, pstCapbXrayIn->xray_in_chan_cnt);
        return NULL;
    }

    pstNormalData = &pstXspChnPrm->stNormalData;

    /* 修改系统默认线程名称 */
    snprintf(sTaskName, sizeof(sTaskName), "xsp_send-%d", u32Chn);
    prctl(PR_SET_NAME, (unsigned long)sTaskName);

    /* 绑核处理，XSP使用2,3核,不同的平台可能不同*/
    /* SAL_SetThreadCoreBind((u32Chn % 2) + 2); */

    SAL_msleep(300); /* XSP在XPACK之前启动，再等待300ms启动XPACK */
    pstNormalData->bXPackStarted = SAL_TRUE;

    while (1)
    {
        /* 等待有节点状态切换为XSP_PSTG_PROCED */
        SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (NULL == (pNode = get_node_by_stage(pstXspChnPrm->lstDataProc, XSP_PSTG_PROCED)))
        {
            SAL_CondWait(&pstXspChnPrm->lstDataProc->sync, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }

        pstDataNode = (XSP_DATA_NODE *)pNode->pAdData;
        pstDataNode->enProcStage = XSP_PSTG_SENDING; /* 转入正在发送的状态 */
        SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);

        /* 无方向的数据直接丢弃 */
        if ((XRAY_DIRECTION_RIGHT != pstDataNode->enDispDir) && (XRAY_DIRECTION_LEFT != pstDataNode->enDispDir))
        {
            XSP_LOGE("chan %u, enDispDir[%d] is invalid, drop this node\n", u32Chn, pstDataNode->enDispDir);
            goto CONTINUE;
        }

        bPbNodeReversed = SAL_FALSE; /* 重置整包回拉节点数据保留标志，默认为不保留 */
        bDTimeAutoSort = SAL_FALSE;
        if (XRAY_TYPE_NORMAL == pstDataNode->enProcType && SAL_FALSE == pstDataNode->bFscImgProc)
        {
            bDTimeAutoSort = SAL_TRUE;
        }
        u32XpackRemainSize = Xpack_DrvGetDispBuffStatus(u32Chn);
        dtime_add_time_point(pstXspChnPrm->pXspDbgTime, pstDataNode->u32DTimeItemIdx, bDTimeAutoSort, "sendStart", -1, SAL_TRUE);
        /* 发送全屏图像处理的数据，整包回拉全屏处理以外的所有情况 */
        if (pstDataNode->bFscImgProc)
        {
            /* 确保XPack缓存少于全屏处理的图像宽度，避免出现屏幕图像部分处理，部分未处理的情况 */
            Xsp_WaitDispBufferDescend(u32Chn, 150, 10, SAL_SUB_SAFE(pstDataNode->stDispFscData.u32Width, pstCapbDisp->disp_yuv_w_max));

            /* 拷贝图像数据 */
            memset(&stData4Disp, 0, sizeof(XPACK_DATA4DISPLAY));
            if (XRAY_PS_PLAYBACK_FRAME == pstXspChnPrm->enProcStat)
            {
                bPbNodeReversed = SAL_TRUE; /* 整包回拉时，保留该节点数据 */
                ximg_create_sub(&pstDataNode->stDispDataSub, &stData4Disp.stDispData, NULL);
            }
            else
            {
                ximg_create_sub(&pstDataNode->stDispFscData, &stData4Disp.stDispData, NULL);
            }

            stData4Disp.bIfRefresh = SAL_TRUE;
            stData4Disp.enDispDir = pstDataNode->enDispDir;
            stData4Disp.u32DispBgColor = pstXspChnPrm->u32XspBgColorUsed;
            XSP_LOGI("chn %u, fsc send, dataPrm w:%d, h:%d, s:%d, bg:0x%x, enDispDir:%d\n",
                     u32Chn, stData4Disp.stDispData.u32Width, stData4Disp.stDispData.u32Height, stData4Disp.stDispData.u32Stride[0], stData4Disp.u32DispBgColor, pstDataNode->enDispDir);

            pstXspChnPrm->stDbgStatus.u32RefreshSendReqCnt++;

            /* 全屏数据发送线程同步,先到的线程进入睡眠，后到的线程直接往下执行，通过条件变量唤醒先到的线程 */
            if (pstCapbXrayIn->xray_in_chan_cnt >= 2)
            {
                Xsp_ThreadSync(u32Chn, 300);
            }

            retVal = Xpack_DrvSendPbSegORefresh(u32Chn, &stData4Disp, NULL);
            pstXspChnPrm->stDbgStatus.u32RefreshSendSuccedCnt++;
            if (SAL_SOK != retVal)
            {
                XSP_LOGE("chan %d, Xpack_DrvSendPbSegORefresh failed\n", u32Chn);
                goto CONTINUE;
            }
        }
        else
        {
            /* 避免xpack显示缓存过多仍继续往显示缓存写数据导致数据覆盖 */
            Xsp_WaitDispBufferDescend(u32Chn, 150, 10, pstCapbDisp->disp_yuv_w_max / 2);
            /* 实时过包数据 */
            if (pstDataNode->enProcType == XRAY_TYPE_NORMAL)
            {
                if (pstNormalData->uiRtDirection != pstDataNode->enDispDir)
                {
                    pstNormalData->uiRtDirection = pstDataNode->enDispDir;
                    Xpack_DrvClearScreen(u32Chn, pstNormalData->u32RtRefreshPerFrame, pstDataNode->enDispDir);
                }

                u32OriNrawWidth = pstDataNode->stSliceOriNrawBuf.u32Width;
                u32OriNrawHeight = pstDataNode->stSliceOriNrawBuf.u32Height;
                u32NrawWidth = pstDataNode->stSliceNrawBuf.u32Width;
                u32NrawHeight = pstDataNode->stSliceNrawBuf.u32Height;

                u32SendOriNrawHeight = u32OriNrawHeight / pstNormalData->u32RtSliceRefreshCnt;
                u32SendNrawHeight = u32NrawHeight / pstNormalData->u32RtSliceRefreshCnt;

                u32SendDispWidth = pstDataNode->stDispDataSub.u32Width / pstNormalData->u32RtSliceRefreshCnt;
                u32SendAiYuvWidth = pstDataNode->stAiYuvBuf.u32Width / pstNormalData->u32RtSliceRefreshCnt; /* AI-YUV不超分 */
                /* FIXME：现在分小条带发送可能有问题，未经验证 */
                for (k = 0; k < pstNormalData->u32RtSliceRefreshCnt; k++)
                {
                    pstNodePackDiv = &pstDataNode->stPackDivRes[k];
                    /* 已开启空白条带移除且子条带不需要发送给xpack的条带 */
                    if(pstNodePackDiv->bRmBlankSliceSub) /* 需要移除的空白条带直接不送xpack */
                    {
                        continue;
                    }

                    /******************* 发送显示相关数据 *******************/
                    /* 显示用的NRaw数据，TIP-NRaw */
                    memset(&stData4Disp, 0, sizeof(XPACK_DATA4DISPLAY));
                    pstNormalData->u64RtSliceSendTime = sal_get_tickcnt();
                    stData4Disp.bIfRefresh = SAL_FALSE;
                    stData4Disp.enDispDir = pstDataNode->enDispDir;

                    stCropPrm.top = u32SendNrawHeight * k;
                    stCropPrm.left = 0;
                    stCropPrm.width = u32NrawWidth;
                    stCropPrm.height = u32SendNrawHeight;
                    if (pstDataNode->bTipNrawMultiplex) // 不在Tip处理阶段，显示Raw数据与存储Raw数据是一致的，复用
                    {
                        ximg_create_sub(&pstDataNode->stSliceNrawBuf, &stData4Disp.stRawData, &stCropPrm);
                    }
                    else
                    {
                        /* 将pu8RtNrawTip构造为一个ximg对象 */
                        ximg_create(pstDataNode->stSliceNrawBuf.u32Width, pstDataNode->stSliceNrawBuf.u32Height, pstDataNode->stSliceNrawBuf.enImgFmt, 
                                    NULL, pstDataNode->pu8RtNrawTip, &stTipRawImg);
                        ximg_create_sub(&stTipRawImg, &stData4Disp.stRawData, &stCropPrm);
                    }

                    /* XSP处理中间结果（融合灰度图） */
                    stCropPrm.top = u32SendNrawHeight * k;
                    stCropPrm.left = 0;
                    stCropPrm.width = u32NrawWidth;
                    stCropPrm.height = u32SendNrawHeight;
                    ximg_create_sub(&pstDataNode->stBlendBuf, &stData4Disp.stBlendData, &stCropPrm);

                    /* Display数据 */
                    stCropPrm.top = 0;
                    stCropPrm.left = u32SendDispWidth * k;
                    stCropPrm.width = u32SendDispWidth;
                    stCropPrm.height = pstDataNode->stDispDataSub.u32Height;
                    ximg_create_sub(&pstDataNode->stDispDataSub, &stData4Disp.stDispData, &stCropPrm);

                    /******************* 包裹相关数据 *******************/
                    // 存储用的包裹NRaw
                    stCropPrm.top = u32SendOriNrawHeight * k;
                    stCropPrm.left = 0;
                    stCropPrm.width = u32OriNrawWidth;
                    stCropPrm.height = u32SendOriNrawHeight;
                    ximg_create_sub(&pstDataNode->stSliceOriNrawBuf, &stData4PackRt.stXrIdtBuf, &stCropPrm);
                    
                    // 危险品识别用的AI-YUV
                    stCropPrm.top = 0;
                    stCropPrm.left = u32SendAiYuvWidth * k;
                    stCropPrm.width = u32SendAiYuvWidth;
                    stCropPrm.height = pstDataNode->stAiYuvBuf.u32Height;
                    ximg_create_sub(&pstDataNode->stAiYuvBuf, &stData4PackRt.stAiDgrBuf, &stCropPrm);

                    pstNodePackDiv = &pstDataNode->stPackDivRes[k];
                    if (XSP_PACKAGE_START == pstNodePackDiv->enPackageTag)
                    {
                        XSP_LOGI("chn %u RT pack_s, col:%d, slice height:%u, XSyncTime:%llu.\n", 
                                 u32Chn, pstDataNode->u32RtSliceNo, pstDataNode->stSliceNrawBuf.u32Height, pstDataNode->u64SyncTime);
                    }
                    else if (XSP_PACKAGE_END == pstNodePackDiv->enPackageTag) /*以包裹结束为标志传输数据*/
                    {
                        stTransferInfo.uiNoramlDirection = pstDataNode->enDispDir; /* 若切换方向，以最后一次方向为参考，比如从R2L切换到L2R，则方向为L2R */
                        stTransferInfo.uiSyncTime = pstDataNode->u64SyncTime;
                        stTransferInfo.uiPackageStartTime = pstNodePackDiv->u64StartSliceTime;
                        stTransferInfo.uiTrigStartTime = pstNodePackDiv->u64StartTrigTime;
                        stTransferInfo.uiTrigEndTime = pstDataNode->u64TrigTime;
                        stTransferInfo.uiColStartNo = pstNodePackDiv->u32StartSliceNo;
                        if (!pstNodePackDiv->bForcedDivided && pstNodePackDiv->u32SliceCnt > 1) /* 非强制分割时，最后一个是空白条带，去掉 */
                        {
                            stTransferInfo.uiColEndNo = pstNodePackDiv->u32SliceNo[pstNodePackDiv->u32SliceCnt - 2] >> 4;
                            stData4PackRt.stAiDgrBuf.u32Width = 0;
                            stData4PackRt.stXrIdtBuf.u32Height = 0;
                        }
                        else
                        {
                            stTransferInfo.uiColEndNo = pstDataNode->u32RtSliceNo;
                        }

                        stTransferInfo.uiIsVerticalFlip = g_xsp_common.stUpdatePrm.stBasePrm.stMirrorPrm[u32Chn].bEnable; /* 是否镜像操作 */
                        stTransferInfo.uiIsForcedToSeparate = pstNodePackDiv->bForcedDivided;
                        stTransferInfo.uiZdataVersion = pstXspChnPrm->enRtZVer;
                        stTransferInfo.uiPackTop = pstNodePackDiv->u32Top;
                        stTransferInfo.uiPackBottom = pstNodePackDiv->u32Bottom;

                        XSP_LOGI("chn %u RT pack_e, pack[%d,%d], packH:%d, forceDiv:%d, top:%d, bot:%d, Dir:%d, Z:%d, Time[Sync:%llu, PackS:%llu, TrigS:%llu, TrigE:%llu].\n",
                                    u32Chn, stTransferInfo.uiColStartNo, stTransferInfo.uiColEndNo, pstNodePackDiv->u32PackLine, stTransferInfo.uiIsForcedToSeparate,
                                    stTransferInfo.uiPackTop, stTransferInfo.uiPackBottom, stTransferInfo.uiNoramlDirection, stTransferInfo.uiZdataVersion,
                                    stTransferInfo.uiSyncTime, stTransferInfo.uiPackageStartTime, stTransferInfo.uiTrigStartTime, stTransferInfo.uiTrigEndTime);
                    }

                    // TIP信息
                    if (SAL_TRUE == pstDataNode->stTipOsd.uiEnable)
                    {
                        strcpy(stLabelTip.szName, "TIP");
                        memcpy(&stLabelTip.stRect, &pstDataNode->stTipOsd.stTipResult, sizeof(XSP_RECT));
                        stLabelTip.u64DelayTime = pstDataNode->stTipOsd.uiDelayTime; /* 延时显示时间 */
                        stLabelTip.u32ShowTime = 0xFFFFFFFF;
                        pstLabelTip = &stLabelTip;
                    }
                    else
                    {
                        pstLabelTip = NULL;
                    }

                    // 基本信息
                    stData4PackRt.enDir = pstDataNode->enDispDir;
                    stData4PackRt.bVMirror = g_xsp_common.stUpdatePrm.stBasePrm.stMirrorPrm[u32Chn].bEnable; /* 是否垂直镜像 */
                    stData4PackRt.enPkgTag = pstNodePackDiv->enPackageTag;
                    stData4PackRt.u32Top = pstNodePackDiv->u32Top; /* 包裹的上下边界，xpack模块的叠框不覆盖包裹的计算 */
                    stData4PackRt.u32Bottom = pstNodePackDiv->u32Bottom;
                    stData4PackRt.u32SliceNrawH = u32SendNrawHeight;
                    
                    pstXspChnPrm->stDbgStatus.u32RtSendReqCnt++;
                    retVal = Xpack_DrvSendRtSlice(u32Chn, &stData4Disp, &stData4PackRt, pstLabelTip, &stTransferInfo);
                    pstXspChnPrm->stDbgStatus.u32RtSendSuccedCnt++;
                    if (SAL_SOK != retVal)
                    {
                        XSP_LOGE("chan %d, Xpack_DrvSendRtSlice failed\n", u32Chn);
                        goto CONTINUE;
                    }
                }
            }
            else if (pstDataNode->enProcType == XRAY_TYPE_PLAYBACK)
            {
                /* 初始化，至少发送1次数据，即使全屏成像处理，也需要发送1次数据的 */
                memset(&stData4Disp, 0, sizeof(XPACK_DATA4DISPLAY));
                u32PbPkgCnt = 1;
                memset(&stPbSeg[0], 0, XSP_PACK_NUM_MAX * sizeof(struct pb_seg_attr));
                memset(astData4PackPb, 0, XSP_PACK_NUM_MAX * sizeof(XPACK_DATA4PACKAGE_PB));
                stPbSeg[0].u32DispWidth = pstDataNode->stDispDataSub.u32Width;

                /* 整理回拉帧中包裹信息 */
                if (0 == pstDataNode->u32PbPackNum) /* 无包裹开始或结束 */
                {
                    astData4PackPb[0].u32StartLoc = 0;
                    astData4PackPb[0].u32EndLoc = 0;
                }
                else if (1 == pstDataNode->u32PbPackNum) /* 1个包裹的情况 */
                {
                    astData4PackPb[0].u32StartLoc = pstDataNode->stPbPack[0].stPackSplit.uiColStartNo;
                    astData4PackPb[0].u32EndLoc = pstDataNode->stPbPack[0].stPackSplit.uiColEndNo;
                }
                else /* 2个及以上包裹的情况 */
                {
                    /* 第一个包裹，从回拉帧数据起始到包裹结束，前段可能有空白，形态为XSP_PACKAGE_END或XSP_PACKAGE_CENTER */
                    u32PbPkgCnt = pstDataNode->u32PbPackNum;
                    for (i = 0; i < u32PbPkgCnt; i++)
                    {
                        if (0 == i) /* 第一个包裹，从回拉帧数据起始到包裹结束，前段可能有空白，形态为XSP_PACKAGE_END或XSP_PACKAGE_CENTER */
                        {
                            stPbSeg[i].u32DispWidth = pstDataNode->stPbPack[i].stPackSplit.uiColEndNo + 1;
                            astData4PackPb[i].u32StartLoc = pstDataNode->stPbPack[i].stPackSplit.uiColStartNo;
                            astData4PackPb[i].u32EndLoc = pstDataNode->stPbPack[i].stPackSplit.uiColEndNo;
                        }
                        else if (u32PbPkgCnt - 1 == i) /* 最后一个包裹，从倒数第二个包裹结束到回拉帧数据结束，前后端可能有空白，形态为XSP_PACKAGE_START或XSP_PACKAGE_CENTER */
                        {
                            stPbSeg[i].u32DispWidth = pstDataNode->stDispDataSub.u32Width - pstDataNode->stPbPack[i - 1].stPackSplit.uiColEndNo - 1;
                            astData4PackPb[i].u32StartLoc = pstDataNode->stPbPack[i].stPackSplit.uiColStartNo - pstDataNode->stPbPack[i - 1].stPackSplit.uiColEndNo - 1;
                            if (XSP_PACK_LINE_INFINITE == pstDataNode->stPbPack[i].stPackSplit.uiColEndNo) /* 只有包裹的开始 */
                            {
                                astData4PackPb[i].u32EndLoc = XSP_PACK_LINE_INFINITE;
                            }
                            else
                            {
                                astData4PackPb[i].u32EndLoc = pstDataNode->stPbPack[i].stPackSplit.uiColEndNo - pstDataNode->stPbPack[i - 1].stPackSplit.uiColEndNo - 1;
                            }
                        }
                        else /* 中间包裹，从前一个包裹结束到当前包裹结束，前段有空白，形态为XSP_PACKAGE_CENTER */
                        {
                            stPbSeg[i].u32DispWidth = pstDataNode->stPbPack[i].stPackSplit.uiColEndNo - pstDataNode->stPbPack[i - 1].stPackSplit.uiColEndNo;
                            astData4PackPb[i].u32StartLoc = pstDataNode->stPbPack[i].stPackSplit.uiColStartNo - pstDataNode->stPbPack[i - 1].stPackSplit.uiColEndNo - 1;
                            astData4PackPb[i].u32EndLoc = stPbSeg[i].u32DispWidth - 1;
                        }

                        /* 对分段的Display Image做偏移计算 */
                        if (XRAY_DIRECTION_RIGHT == pstDataNode->enDispDir)
                        {
                            for (j = 0; j <= i; j++)
                            {
                                stPbSeg[i].u32DispOffset += stPbSeg[j].u32DispWidth;
                            }

                            stPbSeg[i].u32DispOffset = SAL_SUB_SAFE(pstDataNode->stDispDataSub.u32Width, stPbSeg[i].u32DispOffset);
                        }
                        else
                        {
                            for (j = 0; j < i; j++)
                            {
                                stPbSeg[i].u32DispOffset += stPbSeg[j].u32DispWidth;
                            }
                        }
                    }
                }

                for (i = 0; i < u32PbPkgCnt; i++)
                {
                    /* 在包裹结束标志时，发送包裹的宽度、智能识别信息、上下边界 */
                    if (astData4PackPb[i].u32EndLoc != XSP_PACK_LINE_INFINITE) // 包裹结束位置不为XSP_PACK_LINE_INFINITE时，即有真实的包裹结束
                    {
                        astData4PackPb[i].u32RawWidth = pstDataNode->stPbPack[i].u32PackageWidth; /* 包裹RAW数据的宽度 */
                        astData4PackPb[i].u32RawHeight = pstDataNode->stPbPack[i].u32PackageHeight; /* 包裹RAW数据的高度 */
                        astData4PackPb[i].pstAiDgrResult = &pstDataNode->stPbPack[i].stSvaInfo; /* 包裹危险品识别结果 */
                        astData4PackPb[i].pstXrIdtResult = &pstDataNode->stPbPack[i].stIndenResult; /* XSP输出的图像识别信息，包括难穿透、可疑有机物等 */
                        astData4PackPb[i].pstXrIdtResult->stUnpenSent.uiResult = g_xsp_common.stUpdatePrm.stBasePrm.stUnpenSet.uiColor;
                        astData4PackPb[i].pstXrIdtResult->stZeffSent.uiResult = g_xsp_common.stUpdatePrm.stBasePrm.stSusAlert.uiColor;
                        /* 包裹上下边缘、智能识别框Y坐标与镜像操作有关系，若现在镜像配置与过包时的不一致，则转换这些参数，仅当有包裹结束标志时起效 */
                        if (pstDataNode->stPbPack[i].stPackSplit.uiIsVerticalFlip != g_xsp_common.stUpdatePrm.stBasePrm.stMirrorPrm[u32Chn].bEnable)
                        {
                            astData4PackPb[i].u32Top = SAL_SUB_SAFE(astData4PackPb[i].u32RawWidth, pstDataNode->stPbPack[i].stPackSplit.uiPackBottom);
                            astData4PackPb[i].u32Bottom = SAL_SUB_SAFE(astData4PackPb[i].u32RawWidth, pstDataNode->stPbPack[i].stPackSplit.uiPackTop);
                            /* 全屏成像处理的坐标值不做转换：条带回拉图像处理无需发坐标，整包回拉的坐标在非图像处理时已经转换 */
                            for (j = 0; j < pstDataNode->stPbPack[i].stSvaInfo.target_num; j++)
                            {
                                pstRectFt = &pstDataNode->stPbPack[i].stSvaInfo.target[j].rect;
                                pstRectFt->y = SAL_SUB_SAFE(1.0, pstRectFt->y + pstRectFt->height);
                            }
                            for (j = 0; j < pstDataNode->stPbPack[i].stIndenResult.stZeffSent.uiNum; j++)
                            {
                                pstRectFt = &pstDataNode->stPbPack[i].stIndenResult.stZeffSent.stRect[j];
                                pstRectFt->y = SAL_SUB_SAFE(1.0, pstRectFt->y + pstRectFt->height);
                            }
                            for (j = 0; j < pstDataNode->stPbPack[i].stIndenResult.stUnpenSent.uiNum; j++)
                            {
                                pstRectFt = &pstDataNode->stPbPack[i].stIndenResult.stUnpenSent.stRect[j];
                                pstRectFt->y = SAL_SUB_SAFE(1.0, pstRectFt->y + pstRectFt->height);
                            }
                        }
                        else
                        {
                            astData4PackPb[i].u32Top = pstDataNode->stPbPack[i].stPackSplit.uiPackTop;
                            astData4PackPb[i].u32Bottom = pstDataNode->stPbPack[i].stPackSplit.uiPackBottom;
                        }

                        /**
                         * 智能识别X坐标与过包方向有关系
                         * 条带回拉：若当前的实时过包方向与回拉包裹的实时过包方向不一致，则对X坐标执行左右镜像，镜像的中心点是包裹中心点
                         * 整包回拉：整包回拉的显示方向即包裹实时过包的方向，所以X坐标无需转换
                         */
                        if (pstDataNode->stPbPack[i].stPackSplit.uiNoramlDirection != pstXspChnPrm->stNormalData.uiRtDirection)
                        {
                            if (XRAY_PB_SLICE == pstDataNode->enPbMode)
                            {
                                for (j = 0; j < pstDataNode->stPbPack[i].stSvaInfo.target_num; j++)
                                {
                                    pstRectFt = &pstDataNode->stPbPack[i].stSvaInfo.target[j].rect;
                                    pstRectFt->x = SAL_SUB_SAFE(1.0, pstRectFt->x + pstRectFt->width);
                                }
                                for (j = 0; j < pstDataNode->stPbPack[i].stIndenResult.stZeffSent.uiNum; j++)
                                {
                                    pstRectFt = &pstDataNode->stPbPack[i].stIndenResult.stZeffSent.stRect[j];
                                    pstRectFt->x = SAL_SUB_SAFE(1.0, pstRectFt->x + pstRectFt->width);
                                }
                                for (j = 0; j < pstDataNode->stPbPack[i].stIndenResult.stUnpenSent.uiNum; j++)
                                {
                                    pstRectFt = &pstDataNode->stPbPack[i].stIndenResult.stUnpenSent.stRect[j];
                                    pstRectFt->x = SAL_SUB_SAFE(1.0, pstRectFt->x + pstRectFt->width);
                                }
                            }
                        }
                    }
                }

                /* 发送线程同步,先到的线程进入睡眠，后到的线程直接往下执行，通过条件变量唤醒先到的线程 */
                // if (pstCapbXrayIn->xray_in_chan_cnt >= 2 && XRAY_PB_SPEED_XTS != stPbParam.enPbSpeed && 1 != stPbParam.bEnableSync)
                // {
                //     Xsp_ThreadSync(u32Chn, 300);
                // }

                stData4Disp.u32DispBgColor = pstXspChnPrm->u32XspBgColorUsed;
                stData4Disp.enDispDir = pstDataNode->enDispDir;
                stData4Disp.bIfRefresh = SAL_FALSE;
                /* 只有整包回拉或全屏处理时才进行屏幕刷新 */
                if (XRAY_PB_FRAME == pstDataNode->enPbMode)
                {
                    stData4Disp.bIfRefresh = SAL_TRUE;
                    bPbNodeReversed = SAL_TRUE; /* 整包回拉时，保留该节点数据 */
                }

                for (i = 0; i < u32PbPkgCnt; i++)
                {
                    memset(&stData4Disp.stRawData, 0, sizeof(stData4Disp.stRawData));
                    memset(&stData4Disp.stBlendData, 0, sizeof(stData4Disp.stRawData));
                    if (0 == i && SAL_FALSE == stData4Disp.bIfRefresh) // 仅第一次发送raw数据
                    {
                        /* 回拉时算法输出的融合图像也包含邻域，所以需要跳过邻域部分 */
                        stCropPrm.top = pstDataNode->u32PbNeighbourTop;
                        stCropPrm.left = 0;
                        stCropPrm.width = pstDataNode->stNRawInBuf.u32Width;
                        stCropPrm.height = pstDataNode->stNRawInBuf.u32Height - pstDataNode->u32PbNeighbourTop - pstDataNode->u32PbNeighbourBottom;
                        ximg_create_sub(&pstDataNode->stNRawInBuf, &stData4Disp.stRawData, &stCropPrm);
                        ximg_create_sub(&pstDataNode->stBlendBuf, &stData4Disp.stBlendData, &stCropPrm);
                    }

                    stCropPrm.top = 0;
                    stCropPrm.left = stPbSeg[i].u32DispOffset;
                    stCropPrm.width = stPbSeg[i].u32DispWidth;
                    stCropPrm.height = pstDataNode->stDispDataSub.u32Height;
                    if (SAL_SOK != ximg_create_sub(&pstDataNode->stDispDataSub, &stData4Disp.stDispData, &stCropPrm))
                    {
                        XSP_LOGE("ximg_create_sub failed, pack send idx:%u, pack cnt:%u\n", i, u32PbPkgCnt);
                        for (j = 0; j < u32PbPkgCnt; j++)
                        {
                            XSP_LOGE("pack idx:%u, w:%u, offset:%u\n", j, stPbSeg[j].u32DispWidth, stPbSeg[j].u32DispOffset);
                        }
                        break;
                    }
                    XSP_LOGD("pack idx:%u, w:%u, offset:%u, sLoc:%u, eLoc:%u, raw_w:%u, raw_h:%u\n", 
                             i, stPbSeg[i].u32DispWidth, stPbSeg[i].u32DispOffset, 
                             astData4PackPb[i].u32StartLoc, astData4PackPb[i].u32EndLoc, 
                             astData4PackPb[i].u32RawWidth, astData4PackPb[i].u32RawHeight);

                    /* 只有当处理状态为回拉时，才执行send操作，若已切换到其他处理，拦截所有回拉数据，切换到新的处理状态 */
                    SAL_mutexTmLock(&pstXspChnPrm->condProcStat.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
                    if (XRAY_PS_PLAYBACK_MASK & pstXspChnPrm->enProcStat)
                    {
                        SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
                        pstXspChnPrm->stDbgStatus.u32PbSendReqCnt++;
                        retVal = Xpack_DrvSendPbSegORefresh(u32Chn, &stData4Disp, astData4PackPb + i);
                        pstXspChnPrm->stDbgStatus.u32PbSendSuccedCnt++;
                        if (SAL_SOK != retVal)
                        {
                            XSP_LOGE("chan %u, Xpack_DrvSendPbSegORefresh failed, addr: %p, w: %u, h: %u, s: %u\n",
                                     u32Chn, stData4Disp.stDispData.pPlaneVir[0], stData4Disp.stDispData.u32Width, stData4Disp.stDispData.u32Height, 
                                     stData4Disp.stDispData.u32Stride[0]);
                            break;
                        }

                        pstPb2Xpack = &pstXspChnPrm->stPb2Xpack[pstXspChnPrm->u32Pb2XapckStartIdx];
                        pstPb2Xpack->u32FrameNo = pstDataNode->u32RtSliceNo;
                        pstPb2Xpack->u32YuvOffset = stPbSeg[i].u32DispOffset;
                        pstPb2Xpack->u32Width = stData4Disp.stDispData.u32Width;
                        pstPb2Xpack->u32Height = stData4Disp.stDispData.u32Height;
                        pstPb2Xpack->u32Stride = stData4Disp.stDispData.u32Stride[0];
                        pstPb2Xpack->u32DispDir = pstDataNode->enDispDir;
                        pstPb2Xpack->u32PackStart = astData4PackPb[i].u32StartLoc;
                        pstPb2Xpack->u32PackEnd = astData4PackPb[i].u32EndLoc;
                        pstPb2Xpack->u32PackWidth = astData4PackPb[i].u32RawWidth;
                        pstPb2Xpack->u32PackHeight = astData4PackPb[i].u32RawHeight;
                        pstPb2Xpack->u32PackTop = astData4PackPb[i].u32Top;
                        pstPb2Xpack->u32PackBottom = astData4PackPb[i].u32Bottom;
                        memcpy(&pstPb2Xpack->stSvaInfo, &pstDataNode->stPbPack[i].stSvaInfo, sizeof(SVA_DSP_OUT));
                        pstXspChnPrm->u32Pb2XapckStartIdx = (pstXspChnPrm->u32Pb2XapckStartIdx + 1) % XSP_DEBUG_PB2XPACK_NUM;
                    }
                    else
                    {
                        XSP_LOGW("chan %u, current proc stat has changed to: %d, break directly\n", u32Chn, pstXspChnPrm->enProcStat);
                        SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
                        break;
                    }
                }
            }
            else
            {
                XSP_LOGW("chan %u, unsupport this ProcType: %d, delete this node\n", u32Chn, pstDataNode->enProcType);
            }
        }
        if (pstXspChnPrm->stDumpCfg.u32DumpCnt > 0)
        {
            pstXspChnPrm->stDumpCfg.u32DumpCnt--;
        }

CONTINUE:
        dtime_add_time_point(pstXspChnPrm->pXspDbgTime, pstDataNode->u32DTimeItemIdx, bDTimeAutoSort, "sendEnd", -1, SAL_TRUE);
        dtime_add_time_point(pstXspChnPrm->pXspDbgTime, pstDataNode->u32DTimeItemIdx, bDTimeAutoSort, "xpackRes", u32XpackRemainSize, SAL_FALSE);

        if (SAL_SOK == SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
        {
            if (bPbNodeReversed)
            {
                if (XRAY_PS_PLAYBACK_MASK & pstXspChnPrm->enProcStat)
                {
                    /* 先删除历史的XSP_PSTG_RESERVED节点 */
                    while (NULL != (pNode = get_node_by_stage(pstXspChnPrm->lstDataProc, XSP_PSTG_RESERVED)))
                    {
                        DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode);
                    }

                    pstDataNode->enProcStage = XSP_PSTG_RESERVED; /* 然后该的节点成为新的XSP_PSTG_RESERVED */
                }
                else /* 若已切换到非回拉模式，也无需置节点为XSP_PSTG_RESERVED，直接从链表中删除该节点 */
                {
                    DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode); /* 先删除该节点 */
                    /* 然后删除历史的XSP_PSTG_RESERVED节点 */
                    while (NULL != (pNode = get_node_by_stage(pstXspChnPrm->lstDataProc, XSP_PSTG_RESERVED)))
                    {
                        DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode);
                    }

                    XSP_LOGW("chan %u, current proc stat has changed to: %d, delete this one and reserved nodes\n", u32Chn, pstXspChnPrm->enProcStat);
                }
            }
            else
            {
                DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode); /* 从链表中删除该节点 */
            }

            SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pstXspChnPrm->lstDataProc->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
        else
        {
            XSP_LOGE("chan %u, SAL_mutexTmLock 'lstDataProc->sync.mid' failed\n", u32Chn);
        }
    }

    return NULL;
}

/**
 * @fn        xsp_cal_need_img_buf_size
 * @brief     计算存放一张jpeg或bmp、gif图像需要的内存大小
 * @param[IN] XRAY_TRANS_TYPE enImgType    RAW转换的相关参数
 * @param[IN] UINT32          u32ImgWidth  智能识别信息
 * @param[IN] UINT32          u32ImgHeight 智能识别信息
 * @return  UINT32 需要的内存大小，0表示失败
 */
UINT32 xsp_cal_need_img_buf_size(XRAY_TRANS_TYPE enImgType, UINT32 u32ImgWidth, UINT32 u32ImgHeight)
{
    UINT32 u32NeededBufSize = 0;

    if (XRAY_TRANS_BMP == enImgType)
    {
        u32NeededBufSize = XSP_BMP_TRANS_BUF_SIZE(u32ImgWidth, u32ImgHeight);
    }
    else if (XRAY_TRANS_JPG == enImgType)
    {
        u32NeededBufSize = XSP_JPG_TRANS_BUF_SIZE(u32ImgWidth, u32ImgHeight);
    }
    else if (XRAY_TRANS_GIF == enImgType)
    {
        u32NeededBufSize = XSP_GIF_TRANS_BUF_SIZE(u32ImgWidth, u32ImgHeight);
    }
    else if (XRAY_TRANS_PNG == enImgType)
    {
        u32NeededBufSize = XSP_PNG_TRANS_BUF_SIZE(u32ImgWidth, u32ImgHeight);
    }
    else if (XRAY_TRANS_TIF == enImgType)
    {
        u32NeededBufSize = XSP_TIF_TRANS_BUF_SIZE(u32ImgWidth, u32ImgHeight);
    }
    else
    {
        XSP_LOGE("Invalid transform format:%d\n", enImgType);
        u32NeededBufSize = 0;
    }

    return u32NeededBufSize;
}

/**
 * @fn            xsp_trans_raw2img
 * @brief         归一化RAW转换成JPG或者BMP图片
 * @param[IN/OUT] XSP_TRANS_PARAM pstTransParam RAW转换的相关参数
 * @param[IN]     SVA_DSP_OUT     pstSvaInfo    智能识别信息
 * @return  SAL_STATUS SAL_SOK-转换成功，SAL_FALSE-转换失败
 */
SAL_STATUS xsp_trans_raw2img(U32 u32Chn, XSP_TRANS_PARAM *pstTransParam, SVA_DSP_OUT *pstSvaInfo)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 u32DumpCnt = 0;
    UINT32 j = 0, uiTemp = 0, u32BgColor = XIMG_BG_DEFAULT_YUV;
    UINT32 u32ImgWidth = 0, u32ImgHeight = 0;
    float *pfX = NULL, *pfY = NULL, *pfW = NULL, *pfH = NULL;
    CHAR dumpName[128] = {0};
    UINT64 dumpTime = sal_get_tickcnt();
    /* 填充背景色与主视角保持一致 */
    CHAR cTypeSuffix[XRAY_TRANS_BUTT][64] = {"bmp", "jpg", "gif", "png", "tiff"};
    DSP_IMG_DATFMT enRawFmt = DSP_IMG_DATFMT_UNKNOWN;
    SAL_VideoCrop stCropPrm = {0};
    XSP_TRANS_IN stTransIn = {0};
    XSP_TRANS_OUT stTransOut = {0};
    XPACK_PACKAGE_OSD stPackOsd = {0};
    XPACK_SVA_RESULT_OUT stAiDgrResult = {0};
    U08 u8RValue = 0, u8GValue = 0, u8BValue = 0;
    U08 u8YValue = 0, u8UValue = 0, u8VValue = 0;

    XSP_CHN_PRM *pstXspChnPrm = &g_xsp_common.stXspChn[0];
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();

    XSP_CHECK_PTR_IS_NULL(pstTransParam, SAL_FAIL);
    XSP_CHECK_PTR_IS_NULL(pstTransParam->pu8ImgBuf, SAL_FAIL);
    XSP_CHECK_PTR_IS_NULL(pstTransParam->pu8RawBuf, SAL_FAIL);
    XSP_CHECK_PTR_IS_NULL(pstSvaInfo, SAL_FAIL);
    XSP_CHECK_PTR_IS_NULL(gstXspTransProc.pXspHandle, SAL_FAIL);

    /* check u32Width and u32Height */
    if (0 == pstTransParam->u32RawWidth || pstTransParam->u32RawWidth > pstCapbXsp->xraw_width_resized_max)
    {
        XSP_LOGE("the 'u32Width'[%u] is invalid, range: (0, %u]\n", pstTransParam->u32RawWidth, pstCapbXsp->xraw_width_resized);
        return SAL_FAIL;
    }

    if (0 == pstTransParam->u32RawHeight || pstTransParam->u32RawHeight > pstCapbXsp->xraw_height_resized_max)
    {
        XSP_LOGE("the 'u32Height'[%u] is invalid, range: (0, %u]\n", pstTransParam->u32RawHeight, pstCapbXsp->xraw_height_resized);
        return SAL_FAIL;
    }

    /* YUV在RAW域图像上有90°或270°旋转 */
    u32ImgWidth = SAL_align(pstTransParam->u32RawHeight, 2);
    u32ImgHeight = SAL_align(pstTransParam->u32RawWidth, 2);
    u32BgColor = g_xsp_common.stXspChn[0].u32XspBgColorStd;
    if (pstTransParam->unXspProcType.stXspProcType.XSP_PROC_BGCOLOR)
    {
        if (pstCapbXsp->enDispFmt == DSP_IMG_DATFMT_BGR24 || pstCapbXsp->enDispFmt == DSP_IMG_DATFMT_BGRA32)
        {
            u32BgColor = pstTransParam->stXspProcParam.stBkgColor.uiBkgValue;
        }
        else if (pstCapbXsp->enDispFmt == DSP_IMG_DATFMT_YUV420SP_VU)
        {
            u8RValue = pstTransParam->stXspProcParam.stBkgColor.uiBkgValue >> 16 & 0xFF;
            u8GValue = pstTransParam->stXspProcParam.stBkgColor.uiBkgValue >> 8 & 0xFF;
            u8BValue = pstTransParam->stXspProcParam.stBkgColor.uiBkgValue >> 0 & 0xFF;

            u8YValue = SAL_CLIP(( 0.2126 * u8RValue +  0.7152 * u8GValue +  0.0722 * u8BValue +   0), 16.0, 235.0);
            u8UValue = SAL_CLIP((-0.1146 * u8RValue + -0.3854 * u8GValue +  0.5000 * u8BValue + 128), 16.0, 235.0);
            u8VValue = SAL_CLIP(( 0.5000 * u8RValue + -0.4542 * u8GValue + -0.0468 * u8BValue + 128), 16.0, 235.0);

            u32BgColor = u8YValue << 16 | u8UValue << 8 | u8VValue << 0;
        }
        else
        {
            XSP_LOGE("invalid bkg format! only support fmt:0x%x and fmt:0x%x, received fmt:0x%x, value:0x%x\n",
                     DSP_IMG_DATFMT_BGR24, DSP_IMG_DATFMT_BGRA32, pstTransParam->stXspProcParam.stBkgColor.enDataFmt, pstTransParam->stXspProcParam.stBkgColor.uiBkgValue);
            return SAL_FAIL;
        }
    }

    if (pstTransParam->unXspProcType.stXspProcType.XSP_PROC_ANTI && pstTransParam->stXspProcParam.stAntiColor.bEnable)
    {
        u32BgColor = (pstCapbXsp->enDispFmt & DSP_IMG_DATFMT_RGB_MASK) ? XIMG_BG_ANTI_RGB(u32BgColor) : XIMG_BG_ANTI_YUV(u32BgColor);
    }
    /* check pu32ImgSize */
    if (pstTransParam->u32ImgBufSize < xsp_cal_need_img_buf_size(pstTransParam->enImgType, u32ImgWidth, u32ImgHeight))
    {
        XSP_LOGE("Insufficient transform buffer size[%u], expected size:%u, format:%d\n",
                 pstTransParam->u32ImgBufSize, xsp_cal_need_img_buf_size(pstTransParam->enImgType, u32ImgWidth, u32ImgHeight), pstTransParam->enImgType);
        return SAL_FAIL;
    }

    /* 转存输入参数 */
    stTransIn.zdata_ver = pstTransParam->stXspInfo.stTransferInfo.uiZdataVersion;
    enRawFmt = (XRAY_ENERGY_DUAL == pstCapbXrayIn->xray_energy_num) ? DSP_IMG_DATFMT_LHZP : DSP_IMG_DATFMT_SP16;
    ximg_create(pstTransParam->u32RawWidth, pstTransParam->u32RawHeight, enRawFmt, NULL, pstTransParam->pu8RawBuf, &stTransIn.stNrawIn);
    ximg_create_sub(&gstXspTransProc.stBlendTmp, &stTransIn.stBlendIn, NULL);

    /* 转存输出参数 */
    stCropPrm.left = 0;
    stCropPrm.top = 0;
    stCropPrm.width = u32ImgWidth;
    stCropPrm.height = u32ImgHeight;
    ximg_create_sub(&gstXspTransProc.stTransDisp, &stTransOut, &stCropPrm);
    // 重新计算各平面地址，保证连续
    ximg_set_dimension(&stTransOut, u32ImgWidth, gstXspTransProc.stTransDisp.u32Stride[0], SAL_FALSE, u32BgColor);

    if (0 < pstXspChnPrm->stDumpCfg.u32DumpCnt && pstXspChnPrm->stDumpCfg.u32DumpDp & XSP_DDP_TRANS)
    {
        u32DumpCnt = pstXspChnPrm->stDumpCfg.u32DumpCnt;
        pstXspChnPrm->stDumpCfg.u32DumpCnt--;
        ximg_dump(&stTransIn.stNrawIn, 0, pstXspChnPrm->stDumpCfg.chDumpDir, "trans-rawin", NULL, u32DumpCnt);
    }

    /* 1.将RAW转成显示数据（YUV/ARGB） */
    sRet = g_xsp_lib_api.transform_process(gstXspTransProc.pXspHandle, &stTransIn, &stTransOut);
    if (SAL_SOK != sRet)
    {
        XSP_LOGE("transfer_process failed\n");
        return SAL_FAIL;
    }

    if (0 < u32DumpCnt)
    {
        ximg_dump(&stTransOut, 0, pstXspChnPrm->stDumpCfg.chDumpDir, "trans-disp", NULL, u32DumpCnt);
    }

    /* 3.智能目标信息解析和叠框 */
    pstSvaInfo->target_num = SAL_MIN(pstSvaInfo->target_num, SVA_XSI_MAX_ALARM_NUM);
    stAiDgrResult.target_num = pstSvaInfo->target_num;
    /* 坐标信息超出范围判断 */
    for (j = 0; j < stAiDgrResult.target_num; j++)
    {
        memcpy(&stAiDgrResult.target[j], &pstSvaInfo->target[j], sizeof(SVA_DSP_ALERT));
        XSP_LOGI("[%u], type:%u, confidence:%u, rect:[%.3f, %.3f, %.3f, %.3f]\n",
                 j, stAiDgrResult.target[j].type, stAiDgrResult.target[j].visual_confidence, 
                 stAiDgrResult.target[j].rect.x, stAiDgrResult.target[j].rect.y, 
                 stAiDgrResult.target[j].rect.width, stAiDgrResult.target[j].rect.height);

        /* pfX、pfY、pfW、pfH，临时变量，仅为了缩减代码长度 */
        pfX = &stAiDgrResult.target[j].rect.x;
        pfY = &stAiDgrResult.target[j].rect.y;
        pfW = &stAiDgrResult.target[j].rect.width;
        pfH = &stAiDgrResult.target[j].rect.height;

        /* 当X+W或Y+H大于等于0.99时，边框是画不出来的，或者超出了边界（下边框画到图像上边沿，右边框画到左边沿），强制其和最大为0.99 */
        if (*pfX + *pfW >= 0.9999) /*边框本身有宽度*/
        {
            XSP_LOGW("[%u], horizontal coordinate is out of bounds, x: %f, w: %f\n", j, *pfX, *pfW);
            *pfW = SAL_SUB_SAFE(0.9999, *pfX);
        }

        if (*pfY + *pfH >= 0.9999) /* 5030智能识别是基于高640分辨率的，底框会有溢出风险，若底框溢出，则置底框为边界 */
        {
            XSP_LOGW("[%u], vertical coordinate is out of bounds, y: %f, h: %f\n", j, *pfY, *pfH);
            *pfH = SAL_SUB_SAFE(0.9999, *pfY);
        }

        /* 垂直镜像Y坐标转换*/
        if (g_xsp_common.stUpdatePrm.stBasePrm.stMirrorPrm[u32Chn].bEnable
            != pstTransParam->stXspInfo.stTransferInfo.uiIsVerticalFlip)
        {
            *pfY = SAL_SUB_SAFE(1.0, (*pfY + *pfH));
        }
    }

    if (g_xsp_common.stUpdatePrm.stBasePrm.stMirrorPrm[u32Chn].bEnable != pstTransParam->stXspInfo.stTransferInfo.uiIsVerticalFlip)
    {
        /* 垂直镜像Y坐标转换*/
        for (j = 0; j < pstTransParam->stXspInfo.stResultData.stZeffSent.uiNum; j++)
        {
            pfY = &pstTransParam->stXspInfo.stResultData.stZeffSent.stRect[j].y;
            pfH = &pstTransParam->stXspInfo.stResultData.stZeffSent.stRect[j].height;
            *pfY = SAL_SUB_SAFE(1.0, (*pfY + *pfH));
        }

        for (j = 0; j < pstTransParam->stXspInfo.stResultData.stUnpenSent.uiNum; j++)
        {
            pfY = &pstTransParam->stXspInfo.stResultData.stUnpenSent.stRect[j].y;
            pfH = &pstTransParam->stXspInfo.stResultData.stUnpenSent.stRect[j].height;
            *pfY = SAL_SUB_SAFE(1.0, (*pfY + *pfH));
        }

        /*包裹上下边缘跟镜像操作有关系，如果有变化，则上下边缘参数调换*/
        uiTemp = pstTransParam->stXspInfo.stTransferInfo.uiPackTop;
        pstTransParam->stXspInfo.stTransferInfo.uiPackTop = SAL_SUB_SAFE(u32ImgHeight, pstTransParam->stXspInfo.stTransferInfo.uiPackBottom);
        pstTransParam->stXspInfo.stTransferInfo.uiPackBottom = SAL_SUB_SAFE(u32ImgHeight, uiTemp);
        XSP_LOGW("%u!=%u, top and bot, Y need vertical mirror, top:%d bot:%d.\n",
                 pstTransParam->stXspInfo.stTransferInfo.uiIsVerticalFlip, g_xsp_common.stUpdatePrm.stBasePrm.stMirrorPrm[u32Chn].bEnable,
                 pstTransParam->stXspInfo.stTransferInfo.uiPackTop, pstTransParam->stXspInfo.stTransferInfo.uiPackBottom);
    }

    /* 画框 */
    if (0 < stAiDgrResult.target_num || 
        0 < (pstTransParam->stXspInfo.stResultData.stZeffSent.uiNum + pstTransParam->stXspInfo.stResultData.stUnpenSent.uiNum))
    {
        stPackOsd.u32Id = 0; /* 设置ID为无效值0 */
        stPackOsd.pstImgOsd = &stTransOut;
        stPackOsd.stToiLimitedArea.uiX = 0;
        stPackOsd.stToiLimitedArea.uiY = 0;
        stPackOsd.stToiLimitedArea.uiWidth = stTransOut.u32Width;
        stPackOsd.stToiLimitedArea.uiHeight = stTransOut.u32Height;
        stPackOsd.stProfileBorder.u32Left = 0;
        stPackOsd.stProfileBorder.u32Right = stTransOut.u32Width;
        stPackOsd.stProfileBorder.u32Top = pstTransParam->stXspInfo.stTransferInfo.uiPackTop;
        stPackOsd.stProfileBorder.u32Bottom = pstTransParam->stXspInfo.stTransferInfo.uiPackBottom;
        stPackOsd.pstAiDgrResult = &stAiDgrResult;
        stPackOsd.pstXrIdtResult = &pstTransParam->stXspInfo.stResultData;
        stPackOsd.pstXrIdtResult->stUnpenSent.uiResult = g_xsp_common.stUpdatePrm.stBasePrm.stUnpenSet.uiColor;
        stPackOsd.pstXrIdtResult->stZeffSent.uiResult = g_xsp_common.stUpdatePrm.stBasePrm.stSusAlert.uiColor;
        stPackOsd.pstLabelTip = NULL;
        if (0 > Xpack_DrvDrawOsdOnPackage(0, &stPackOsd, SAL_TRUE, u32BgColor))
        {
            XSP_LOGE("Draw frame[chan %d] failed.\n", u32Chn);
            return SAL_FAIL;
        }

        if (0 < u32DumpCnt)
        {
            ximg_dump(&stTransOut, 0, pstXspChnPrm->stDumpCfg.chDumpDir, "trans_osd", NULL, u32DumpCnt);
        }
    }

    /* 4.转存图片 */
    pstTransParam->u32ImgDataSize = pstTransParam->u32ImgBufSize;
    sRet = ximg_save(&stTransOut, pstTransParam->enImgType, pstTransParam->pu8ImgBuf, &pstTransParam->u32ImgDataSize, u32BgColor);
    if (SAL_SOK != sRet)
    {
        XSP_LOGE("ximg_save failed, save type:%d, w:%u, h:%u, s:%u\n", 
                 pstTransParam->enImgType, stTransOut.u32Width, stTransOut.u32Height, stTransOut.u32Stride[0]);
        return SAL_FAIL;
    }
    if (0 < u32DumpCnt)
    {
        snprintf(dumpName, sizeof(dumpName), "%s/trans_no%u_w%u_h%u_t%llu_.%s", 
                 pstXspChnPrm->stDumpCfg.chDumpDir, u32DumpCnt, u32ImgWidth, u32ImgHeight, dumpTime, cTypeSuffix[pstTransParam->enImgType]);
        SAL_WriteToFile(dumpName, 0, SEEK_SET, pstTransParam->pu8ImgBuf, pstTransParam->u32ImgDataSize);
    }

    return SAL_SOK;
}

/**
 * @function   Xsp_DrvPrmCreate
 * @brief      XSP功能参数创建
 * @param[in]  None
 * @param[out]: None
 * @return     static INT32 成功SAL_SOK，失败SAL_FALSE
 */
static SAL_STATUS Xsp_DrvPrmCreate(void)
{
    SAL_STATUS s32Ret = SAL_SOK;
    int i = 0;
    XSP_UPDATE_PRM *pstUpdatePrm = NULL;
    XSP_TIP_ATTR *pstTipAttr = NULL;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();

    if (NULL == pstCapbXrayIn)
    {
        XSP_LOGE("pstCapbXrayIn is %p.\n", pstCapbXrayIn);
        return SAL_FAIL;
    }

    pstUpdatePrm = &g_xsp_common.stUpdatePrm;
    pstTipAttr = &g_xsp_common.stTipAttr;

    /*创建参数设置互斥锁*/
    if (SAL_SOK != SAL_mutexTmInit(&pstUpdatePrm->mutexImgProc, SAL_MUTEX_ERRORCHECK))
    {
        XSP_LOGE("SAL_mutexTmInit 'pstUpdatePrm->mutexImgProc' failed\n");
        return SAL_FAIL;
    }

    for (i = 0; i < pstCapbXrayIn->xray_in_chan_cnt; i++)
    {
        /*创建tip数据互斥锁*/
        s32Ret = SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstTipAttr->stTipCtrl[i].pDataMutex);
        if (SAL_SOK != s32Ret)
        {
            XSP_LOGE("stTipCtrl[%d].pDataMutex.\n", i);
            return SAL_FAIL;
        }
    }

    /* 初始化用于双视角哦显示同步的条件变量 */
    if (SAL_SOK != SAL_CondInit(&g_xsp_common.stFscSync.condFscSync))
    {
        XSP_LOGE("SAL_CondInit 'condFscSync' failed\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvCreate
 * @brief:      XSP功能通道创建
 * @param[in]:  UINT32 chan  通道号
 * @param[out]: None
 * @return:     static INT32 成功SAL_SOK，失败SAL_FALSE
 */
static SAL_STATUS Xsp_DrvCreate(void)
{
    UINT32 chan = 0, i = 0;
    UINT32 u32XspDataNodeNum = 0;       /* XSP处理队列的节点数 */
    UINT32 u32BufferSize = 0, u32TipResMax = 0, u32TransDispW = 0;
    DSP_IMG_DATFMT enImgFmt = DSP_IMG_DATFMT_UNKNOWN;
    DSA_NODE *pNode = NULL;
    XSP_DATA_NODE *pstDataNode = NULL;
    XSP_CHN_PRM *pstXspChnPrm = NULL;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_DISP *pstCapbDisp = capb_get_disp();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();

    u32XspDataNodeNum = xsp_getNodeNum(pstCapbXrayIn);

    u32TipResMax = XSP_TIP_MAX_H * XSP_TIP_MAX_W;

    /* 以能力集中的X-Ray输入源数量进行通道的初始化实时预览、回拉两种功能 */
    for (chan = 0; chan < pstCapbXrayIn->xray_in_chan_cnt; chan++)
    {
        pstXspChnPrm = &g_xsp_common.stXspChn[chan];

        pstXspChnPrm->u32XspBgColorStd = (DSP_IMG_DATFMT_YUV420_MASK & pstCapbXsp->enDispFmt) ? XIMG_BG_DEFAULT_YUV : XIMG_BG_DEFAULT_RGB;
        pstXspChnPrm->u32XspBgColorUsed = pstXspChnPrm->u32XspBgColorStd;
        pstXspChnPrm->pXspDbgTime = dtime_create(160);

        /* 1. 创建管理满队列 */
        pstXspChnPrm->lstDataProc = DSA_LstInit(u32XspDataNodeNum);
        if (NULL == pstXspChnPrm->lstDataProc)
        {
            XSP_LOGE("chan %u, init 'lstDataProc' failed\n", chan);
            return SAL_FAIL;
        }

        /* 将XSP数据节点挂到队列中的节点上 */
        SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        for (i = 0; i < u32XspDataNodeNum; i++)
        {
            if (NULL != (pNode = DSA_LstGetIdleNode(pstXspChnPrm->lstDataProc)))
            {
                pNode->pAdData = pstXspChnPrm->stDataNode + i;
                DSA_LstPush(pstXspChnPrm->lstDataProc, pNode);
            }
        }

        for (i = 0; i < u32XspDataNodeNum; i++)
        {
            DSA_LstPop(pstXspChnPrm->lstDataProc);
        }

        SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);

        XSP_LOGW("chan[%u] RAW: w[%u] h[%u], DISP-YUV: w[%u] h[%u]\n", chan, \
                 pstCapbXsp->xraw_width_resized, pstCapbXsp->xraw_height_resized, \
                 pstCapbDisp->disp_yuv_w_max, pstCapbDisp->disp_yuv_h_max);

        if (SAL_SOK != SAL_CondInit(&pstXspChnPrm->condProcStat))
        {
            XSP_LOGE("SAL_CondInit 'condProcStat' failed, chan: %u\n", chan);
            return SAL_FAIL;
        }

        /* 3. 申请缓存区并放入空队列 */
        for (i = 0; i < u32XspDataNodeNum; i++)
        {
            pstDataNode = pstXspChnPrm->stDataNode + i;

            /* 
             * raw数据输入缓冲的大小，实时预览为高低能数据，回拉为高低能数据+原子序数，并包括邻域数据， 
             * 过包时是DSP_IMG_DATFMT_LHP格式，回拉时是DSP_IMG_DATFMT_LHZP格式，按最大格式申请内存
             */
            pstDataNode->stXRawInBuf.stMbAttr.bCached = SAL_TRUE;
            if (SAL_SOK != ximg_create(pstCapbXsp->xraw_width_resized, 
                                       pstCapbXsp->xraw_height_resized + XSP_NEIGHBOUR_H_MAX * 2, 
                                       DSP_IMG_DATFMT_LHZP, "xsp-raw-in", NULL, &pstDataNode->stXRawInBuf))
            {
                XSP_LOGE("ximg_create for raw in buf failed, w:%u, h:%u, fmt:0x%x\n", 
                         pstCapbXsp->xraw_width_resized, pstCapbXsp->xraw_height_resized + XSP_NEIGHBOUR_H_MAX * 2, DSP_IMG_DATFMT_LHZP);
                return SAL_FAIL;
            }

            /* 输出归一化原始数据缓冲 */
            enImgFmt = (XRAY_ENERGY_SINGLE == pstCapbXrayIn->xray_energy_num) ? DSP_IMG_DATFMT_SP16 : DSP_IMG_DATFMT_LHZP;
            pstDataNode->stSliceNrawBuf.stMbAttr.bCached = SAL_TRUE;
            if (SAL_SOK != ximg_create(pstCapbXsp->xraw_width_resized, SAL_align((UINT32)(pstCapbXrayIn->line_per_slice_max * pstCapbXsp->resize_height_factor), 2), 
                                       enImgFmt, "xsp-raw-out", NULL, &pstDataNode->stSliceNrawBuf))
            {
                XSP_LOGE("ximg_create for slice nraw buf failed, w:%u, h:%u, fmt:0x%x\n", 
                         pstCapbXsp->xraw_width_resized, SAL_align((UINT32)(pstCapbXrayIn->line_per_slice_max * pstCapbXsp->resize_height_factor), 2), DSP_IMG_DATFMT_LHZP);
                return SAL_FAIL;
            }
            /* 输出未超分的归一化原始数据缓冲 */
            enImgFmt = (XRAY_ENERGY_SINGLE == pstCapbXrayIn->xray_energy_num) ? DSP_IMG_DATFMT_SP16 : DSP_IMG_DATFMT_LHZ16P;
            pstDataNode->stSliceOriNrawBuf.stMbAttr.bCached = SAL_TRUE;
            if (SAL_SOK != ximg_create(pstCapbXrayIn->xraw_width_max, pstCapbXrayIn->line_per_slice_max, enImgFmt, "xsp-raw-ori-out", NULL, &pstDataNode->stSliceOriNrawBuf))
            {
                XSP_LOGE("ximg_create for slice nraw buf failed, w:%u, h:%u, fmt:0x%x\n", 
                         pstCapbXsp->xraw_width_resized, pstCapbXrayIn->line_per_slice_max, DSP_IMG_DATFMT_LHZP);
                return SAL_FAIL;
            }

            /* 带有TIP的条带数据 */
            u32BufferSize = pstCapbXsp->xraw_width_resized * 300 * pstCapbXsp->xsp_normalized_raw_bw; /* 校正时临时使用，取300列超出校正数据量 */
            pstDataNode->pu8RtNrawTip = (UINT8 *)Xsp_HalModMalloc(u32BufferSize, 128, "RtNrawTip");
            if (NULL == pstDataNode->pu8RtNrawTip)
            {
                XSP_LOGE("Xsp_HalModMalloc for 'pu8RtNrawTip' failed, buffer size: %u\n", u32BufferSize);
                return SAL_FAIL;
            }

            /* 输出XSP处理中间结果Buffer,回拉需使用,申请一屏大小 */
            pstDataNode->stBlendBuf.stMbAttr.bCached = SAL_TRUE;
            if (SAL_SOK != ximg_create(pstCapbXsp->xraw_width_resized, (pstCapbXsp->xraw_height_resized + XSP_NEIGHBOUR_H_MAX * 2), DSP_IMG_DATFMT_SP16, "xsp-blend", NULL, &pstDataNode->stBlendBuf))
            {
                XSP_LOGE("ximg_create for blend data failed, w:%u, h:%u, fmt:0x%x\n", 
                         pstCapbXsp->xraw_width_resized, pstCapbXsp->xraw_height_resized, DSP_IMG_DATFMT_SP16);
                return SAL_FAIL;
            }

            /* 输出DISP数据缓冲 */
            pstDataNode->stDispFscData.stMbAttr.bCached = SAL_FALSE;
            if (SAL_SOK != ximg_create(pstCapbXsp->xsp_fsc_proc_width_max, pstCapbDisp->disp_yuv_h_max, 
                                       pstCapbXsp->enDispFmt, "xsp-fsc-disp", NULL, &pstDataNode->stDispFscData))
            {
                XSP_LOGE("ximg_create for disp fsc data failed, w:%u, h:%u, fmt:0x%x\n", 
                         pstCapbXsp->xsp_fsc_proc_width_max, pstCapbDisp->disp_yuv_h_max, pstCapbXsp->enDispFmt);
                return SAL_FAIL;
            }

            /* 输出AI YUV数据缓冲 */
            pstDataNode->stAiYuvBuf.stMbAttr.bCached = SAL_FALSE;
            if (SAL_SOK != ximg_create(pstCapbXrayIn->line_per_slice_max, pstCapbXrayIn->xraw_width_max, DSP_IMG_DATFMT_YUV420SP_VU, "xsp-ai-yuv", NULL, &pstDataNode->stAiYuvBuf))
            {
                XSP_LOGE("ximg_create for ai yuv data failed, w:%u, h:%u, fmt:0x%x\n", 
                         pstCapbXrayIn->line_per_slice_max, pstCapbXrayIn->xraw_width_max, DSP_IMG_DATFMT_YUV420SP_VU);
                return SAL_FAIL;
            }
        }

        /* 5. XSP算法创建 */
        if (SAL_SOK != xsp_alg_create_rt_pb(chan))
        {
            XSP_LOGE("chan %u, xsp_alg_create_rt_pb failed\n", chan);
            return SAL_FAIL;
        }

        /* 申请校正数据内存 */
        xsp_get_correction_data(pstXspChnPrm->handle, XSP_CORR_UNDEF, NULL, &u32BufferSize);
        pstXspChnPrm->pFullLoadData = (UINT16 *)Xsp_HalModMalloc(u32BufferSize, 16, "FullLoadData");
        pstXspChnPrm->pEmptyLoadData = (UINT16 *)Xsp_HalModMalloc(u32BufferSize, 16, "EmptyLoadData");
        if (NULL == pstXspChnPrm->pFullLoadData || NULL == pstXspChnPrm->pEmptyLoadData)
        {
            XSP_LOGE("chan %u, Xsp_HalModMalloc for 'pFullLoadData[%p]' or 'pEmptyLoadData[%p]' failed, buffer size: %u\n",
                     chan, pstXspChnPrm->pFullLoadData, pstXspChnPrm->pEmptyLoadData, u32BufferSize);
            return SAL_FAIL;
        }

        /*TIP存储缓冲区*/
        u32BufferSize = u32TipResMax * pstCapbXsp->xsp_normalized_raw_bw;
        g_xsp_common.stTipAttr.stTipData.stTipNormalized[chan].pXrayBuf = (UINT16 *)Xsp_HalModMalloc(u32BufferSize, 128, "TipData");
        if (NULL == g_xsp_common.stTipAttr.stTipData.stTipNormalized[chan].pXrayBuf)
        {
            XSP_LOGE("chan %u, Xsp_HalModMalloc for 'stTipNormalized' failed, buffer size: %u\n", chan, u32BufferSize);
            return SAL_FAIL;
        }

        build_disp_bgcolor(chan, SAL_FALSE);
    }

    /******************* 转存功能初始化 *******************/
    /* 1. XSP算法创建 */
    if (SAL_SOK != xsp_alg_create_trans())
    {
        XSP_LOGE("oops, xsp_alg_create_trans failed\n");
        return SAL_FAIL;
    }

    /* 2. 申请转存使用的Buffer */
    /* 转存XSP处理的中间结果，即融合灰度图 */
    if (SAL_SOK != ximg_create(pstCapbXsp->xraw_width_resized, pstCapbXsp->xraw_height_resized, 
                              DSP_IMG_DATFMT_SP16, "xsp-trans-blend", NULL, &gstXspTransProc.stBlendTmp))
    {
        XSP_LOGE("ximg_create for trans blend data failed, w:%u, h:%u, fmt:0x%x\n", 
                 pstCapbXsp->xraw_width_resized, pstCapbXsp->xraw_height_resized, DSP_IMG_DATFMT_SP16);
        return SAL_FAIL; 
    }

    /* 转存的显示图像内存 */
    gstXspTransProc.stTransDisp.stMbAttr.bCached = SAL_FALSE;
    u32TransDispW = SAL_MAX(pstCapbXsp->xraw_height_resized, pstCapbXsp->xsp_package_line_max) + 100;
    if (SAL_SOK != ximg_create(SAL_align(u32TransDispW, 64), (APP2DSP_DATA_H >= pstCapbXsp->xraw_width_resized) ? APP2DSP_DATA_H : pstCapbXsp->xraw_width_resized, pstCapbXsp->enDispFmt, "xsp-trans-disp", NULL, &gstXspTransProc.stTransDisp))
    {
        XSP_LOGE("ximg_create for xsp trans disp failed, w:%u, h:%u, fmt:0x%x\n", 
                 pstCapbXsp->xraw_width_resized, pstCapbXsp->xraw_height_resized, pstCapbXsp->enDispFmt);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvModuleCreate
 * @brief:      XSP驱动模块功能创建
 * @param[in]:  void
 * @param[out]: None
 * @return:     static INT32  成功SAL_SOK，失败SAL_FALSE
 */
static INT32 Xsp_DrvModuleCreate(void)
{
    INT32 s32Ret = SAL_SOK;
    DSPINITPARA *dsp_init_prm = NULL;
    U32 en_lib_src = XSP_LIB_RAYIN;
    CHAR *libSrcString[XSP_LIB_NUM] =
    {
        "rayin-normal",
        "rayin-ai",
        "rayin-ai-external",
        "rayin-ai-other",
    };

    dsp_init_prm = SystemPrm_getDspInitPara();
    if (NULL == dsp_init_prm)
    {
        XSP_LOGE("dsp Init Prm is error!");
        return SAL_FAIL;
    }

    en_lib_src = (dsp_init_prm->xspLibSrc >= XSP_LIB_NUM) ? (XSP_LIB_NUM - 1) : dsp_init_prm->xspLibSrc;
    XSP_LOGI("Xsp lib src is %d (%s).\n!", en_lib_src, libSrcString[en_lib_src]);

    if (SAL_SOK != hka_xsp_lib_reg(en_lib_src))
    {
        XSP_LOGE("define HKA XSP lib failed\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != Xsp_DrvCreate())
    {
        XSP_LOGE("Xsp_drvCreate failed\n");
        return SAL_FAIL;
    }

    s32Ret = Xsp_DrvPrmCreate();
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("Xsp_DrvMutexCreate error.\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      Xsp_DrvModuleTsk
 * @brief   XSP模块线程创建
 *
 * @return  SAL_STATUS SAL_SOK-成功，SAL_FALSE-失败
 */
static SAL_STATUS Xsp_DrvModuleTsk(void)
{
    UINT32 i = 0;
    XSP_CHN_PRM *pstXspChnprm = NULL;
    INT32 status = SAL_SOK;
    UINT32 stackSize = 2 * 1024 * 1024; /*线程栈*/
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();

    /* XSP处理线程 */
    for (i = 0; i < pstCapbXrayIn->xray_in_chan_cnt; i++)
    {
        pstXspChnprm = &g_xsp_common.stXspChn[i];

        status = SAL_thrCreate(&(pstXspChnprm->stThrHandlProc), Xsp_DrvProcThread, SAL_THR_PRI_DEFAULT, stackSize, pstXspChnprm);
        if (SAL_SOK != status)
        {
            XSP_LOGE("SAL_thrCreate 'Xsp_DrvProcThread' failed\n");
            return SAL_FAIL;
        }

        status = SAL_thrCreate(&(pstXspChnprm->stThrHandlSend), Xsp_DrvSendThread, SAL_THR_PRI_DEFAULT, stackSize, pstXspChnprm);
        if (SAL_SOK != status)
        {
            XSP_LOGE("SAL_thrCreate 'Xsp_DrvSendThread' failed\n");
            return SAL_FAIL;
        }

        status = SAL_thrCreate(&(pstXspChnprm->stThrHandlIdt), Xsp_DrvPackageIdentifyThread, SAL_THR_PRI_DEFAULT, stackSize, pstXspChnprm);
        if (SAL_SOK != status)
        {
            XSP_LOGE("SAL_thrCreate 'Xsp_DrvIndentifyProcess' failed\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvCommonInit
 * @brief:      XSP模块通用参数初始化
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32 成功SAL_SOK，失败SAL_FALSE
 */
static void Xsp_DrvCommonInit(void)
{
    INT32 i = 0, j = 0, k = 0;
    UINT32 u32IntegralTime = 0, u32VideoOutputFps = 0;
    XSP_RT_PARAMS *pstPicPrm = &g_xsp_common.stUpdatePrm.stXspRtParm;
    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();
    XSP_CHN_PRM *pstXspChnPrm = NULL;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    UINT32 u32LinePerSecond = 0; /* 1s raw数据采集的行数 */

    u32VideoOutputFps = pstDspInfo->stVoInitInfoSt.stVoInfoPrm[0].stDispDevAttr.videoOutputFps;
    /* 设置默认参数 */
    for (i = 0; i < pstCapbXrayIn->xray_in_chan_cnt; i++)
    {
        pstXspChnPrm = &g_xsp_common.stXspChn[i];
        memset(pstXspChnPrm, 0, sizeof(XSP_CHN_PRM));
        pstXspChnPrm->u32Chn = i;
        pstXspChnPrm->stNormalData.u32RtPassthDataSize = offsetof(XSP_DATA_NODE, enPbMode);
        pstXspChnPrm->stNormalData.u32ImgProcMode = XSP_IMG_PROC_NONE;
        pstXspChnPrm->stNormalData.uiPbDirection = XRAY_DIRECTION_RIGHT;
        pstXspChnPrm->stNormalData.uiRtDirection = XRAY_DIRECTION_LEFT;
        pstXspChnPrm->stRtPackDivS.enMethod = XSP_PACK_DIVIDE_IMG_IDENTY;
        pstXspChnPrm->stRtPackDivS.u32segMaster = XSP_PACK_DIVIDE_INDEPENDENT;//主辅通道默认分别分割包裹
        for (k = 0; k < XRAY_FORM_NUM; k++)
        {
            for (j = 0; j < XRAY_SPEED_NUM; j++)
            {
                if (pstCapbXrayIn->st_xray_speed[k][j].line_per_slice > 0)
                {
                    pstXspChnPrm->stNormalData.enRtForm = k;
                    pstXspChnPrm->stNormalData.enRtSpeed = j;
                    u32IntegralTime = pstCapbXrayIn->st_xray_speed[k][j].integral_time;
                    pstXspChnPrm->stNormalData.u32RtSliceRefreshCnt = 1;
                    u32LinePerSecond = ((1000000 + u32IntegralTime / 2) / u32IntegralTime) * pstCapbXsp->resize_height_factor;
                    pstXspChnPrm->stNormalData.u32RtRefreshPerFrame = (u32LinePerSecond + u32VideoOutputFps / 2) / u32VideoOutputFps;
                    break;
                }
            }

            if (u32IntegralTime > 0) /* 跳出二层循环 */
            {
                break;
            }
        }
    }

    pstPicPrm->stDispMode.disp_mode = XSP_DISP_COLOR;              /*显示模式*/
    pstPicPrm->stAntiColor.bEnable = SAL_FALSE;                    /*反色模式*/
    pstPicPrm->stEe.bEnable = SAL_FALSE;                           /*边缘增强*/
    pstPicPrm->stUe.bEnable = SAL_FALSE;                           /*超级增强*/
    pstPicPrm->stLum.uiLevel = XSP_LUM_DEFUALT_LEVEL;              /*亮度*/
    pstPicPrm->stPseudoColor.pseudo_color = XSP_PSEUDO_COLOR_0;    /*伪彩*/
    g_xsp_common.stUpdatePrm.stBasePrm.stSusAlert.uiColor = XSP_OSD_RED;
    g_xsp_common.stUpdatePrm.stBasePrm.stUnpenSet.uiColor = XSP_OSD_BLUE;

    /*共享成像参数初始化，dsp写入，应用读取*/
    if (NULL != pstDspInfo)
    {
        memset(&pstDspInfo->stXspPicPrm, 0, sizeof(XSP_RT_PARAMS));
        pstDspInfo->stXspPicPrm.stLum.uiLevel = XSP_LUM_DEFUALT_LEVEL;
    }

    return;
}

/**
 * @function:   Xsp_DrvInit
 * @brief:      XSP模块驱动初始化
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32 成功SAL_SOK，失败SAL_FALSE
 */
INT32 Xsp_DrvInit(void)
{
    /*参数初始化*/
    Xsp_DrvCommonInit();

    /*模块创建*/
    if (SAL_SOK != Xsp_DrvModuleCreate())
    {
        XSP_LOGE("Xsp_DrvModuleCreate is error.\n");
        return SAL_FAIL;
    }

    /*任务线程*/
    if (SAL_SOK != Xsp_DrvModuleTsk())
    {
        XSP_LOGE("Sva_DrvModuleTsk is error.\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      xsp_set_rt_default
 * @brief   针对键盘上的默认和原始模式，恢复部分图像处理参数
 * @warning 恢复的部分图像参数也仅限于键盘上按键支持的，UI界面上配置的不允许恢复
 *
 * @param   pstXspParam[OUT] 恢复的部分图像处理参数
 */
static void xsp_set_rt_default(XSP_RT_PARAMS *pstXspParam)
{
    if (NULL == pstXspParam)
    {
        return;
    }

    pstXspParam->stOriginalMode.bEnable = SAL_FALSE;
    pstXspParam->stDispMode.disp_mode = XSP_DISP_COLOR;
    pstXspParam->stAntiColor.bEnable = SAL_FALSE;
    pstXspParam->stEe.bEnable = SAL_FALSE;
    pstXspParam->stUe.bEnable = SAL_FALSE;
    pstXspParam->stLp.bEnable = SAL_FALSE;
    pstXspParam->stHp.bEnable = SAL_FALSE;
    pstXspParam->stLe.bEnable = SAL_FALSE;
    pstXspParam->stVarAbsor.bEnable = SAL_FALSE;
    // pstXspParam->stLum.uiLevel = XSP_LUM_DEFUALT_LEVEL;
    pstXspParam->stEnhancedScan.bEnable = SAL_FALSE;

    return;
}

/**
 * @fn      Xsp_DrvSetWhiteArea
 * @brief   设置置白区域，仅支持纵向的顶部和底部置白，不支持横向的左侧和右侧置白
 *
 * @param   chan[IN] 通道号
 * @param   top_margin[IN] 顶部置白区域，相对于默认R2L显示时，RAW有效区域的顶部
 * @param   bottom_margin[IN] 底部置白区域，相对于默认R2L显示时，RAW有效区域的底部
 *
 * @return SAL_STATUS SAL_SOK-设置成功，SAL_FAIL-设置失败
 */
SAL_STATUS Xsp_DrvSetWhiteArea(UINT32 chan, UINT32 top_margin, UINT32 bottom_margin)
{
    CAPB_DISP *pstCapbDisp = capb_get_disp();

    if (top_margin + bottom_margin < pstCapbDisp->disp_h_middle_indeed)
    {
        g_xsp_common.stXspChn[chan].raw_cover_left = top_margin;
        g_xsp_common.stXspChn[chan].raw_cover_right = bottom_margin;
        return g_xsp_lib_api.set_blank_region(g_xsp_common.stXspChn[chan].handle, top_margin, bottom_margin);
    }
    else
    {
        XSP_LOGE("the top_margin[%u] OR bottom_margin[%u] is out of range, their sum[%u] should be less than image height[%u]\n", \
                 top_margin, bottom_margin, top_margin + bottom_margin, pstCapbDisp->disp_h_middle_indeed);
        return SAL_FAIL;
    }
}

/**
 * @function:   Xsp_DrvSetAnti
 * @brief:      设置反色
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetAnti(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_ANTI_COLOR_PARAM *setXspAnti = (XSP_ANTI_COLOR_PARAM *)prm;
    XSP_RT_PARAMS *pstPicPrm = &g_xsp_common.stUpdatePrm.stXspRtParm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm)
    {
        XSP_LOGE("Set anti prm [%p] is error!\n", prm);
        return SAL_FAIL;
    }

    if (NULL == pstCapXrayIn)
    {
        XSP_LOGE("pstCapXrayIn [%p] is error!\n", pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set anti chan %d is error!", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("set Anti enble %d\n", setXspAnti->bEnable);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_ANTI;
    }

    pstPicPrm->stAntiColor.bEnable = !!setXspAnti->bEnable;
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetEe
 * @brief:      设置边缘增强
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetEe(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_EE_PARAM *setEdgeEnhance = (XSP_EE_PARAM *)prm;
    XSP_RT_PARAMS *pstPicPrm = &g_xsp_common.stUpdatePrm.stXspRtParm;
    DSPINITPARA *pstInitPrm = SystemPrm_getDspInitPara();
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn || NULL == pstInitPrm)
    {
        XSP_LOGE("Set Ee prm [%p] or pstCapXrayIn [%p] or stInitPrm [%p] is error!\n", prm, pstCapXrayIn, pstInitPrm);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set Ee chan %d is error!", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("set Edge Enhance enble %d, level %d.\n", setEdgeEnhance->bEnable, setEdgeEnhance->uiLevel);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_EE;
    }

    pstPicPrm->stEe.bEnable = !!setEdgeEnhance->bEnable;
    pstPicPrm->stEe.uiLevel = setEdgeEnhance->uiLevel;

    g_xsp_common.stUpdatePrm.stXspRtParm.stUe.bEnable = SAL_FALSE;
    g_xsp_common.stUpdatePrm.stXspRtParm.stLe.bEnable = SAL_FALSE;

    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetUe
 * @brief:      设置超级增强
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetUe(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_UE_PARAM *setSuperEnhance = (XSP_UE_PARAM *)prm;
    XSP_RT_PARAMS *pstPicPrm = &g_xsp_common.stUpdatePrm.stXspRtParm;
    DSPINITPARA *pstInitPrm = SystemPrm_getDspInitPara();
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn || NULL == pstInitPrm)
    {
        XSP_LOGE("Set Ue prm [%p] , pstCapXrayIn [%p] or dspInitPrm [%p] is error!\n", prm, pstCapXrayIn, pstInitPrm);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set Ue chan %d is error!", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("set Super Enhance enble %d, level %d.\n", setSuperEnhance->bEnable, setSuperEnhance->uiLevel);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_UE;
    }

    pstPicPrm->stUe.bEnable = !!setSuperEnhance->bEnable;
    pstPicPrm->stUe.uiLevel = setSuperEnhance->uiLevel;
    g_xsp_common.stUpdatePrm.stXspRtParm.stEe.bEnable = SAL_FALSE;
    g_xsp_common.stUpdatePrm.stXspRtParm.stLe.bEnable = SAL_FALSE;

    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/*
 * @function:   Xsp_DrvSetDisp
 * @brief:      设置显示模式
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetDisp(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_DISP_MODE *pstDispMode = (XSP_DISP_MODE *)prm;
    XSP_RT_PARAMS *pstPicPrm = &g_xsp_common.stUpdatePrm.stXspRtParm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set disp prm [%p] or pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set disp chan %d is error!", chan);
        return SAL_FAIL;
    }

    /*能量类型*/
    if (XRAY_ENERGY_SINGLE == pstCapXrayIn->xray_energy_num)
    {
        if (pstDispMode->disp_mode > XSP_DISP_GRAY)
        {
            XSP_LOGE("Disp mode is error with %d\n", pstDispMode->disp_mode);
            return SAL_FAIL;
        }
    }
    else if (XRAY_ENERGY_DUAL == pstCapXrayIn->xray_energy_num)
    {
        if (pstDispMode->disp_mode > XSP_DISP_MODE_NUM - 1)
        {
            XSP_LOGE("Disp mode is error with %d\n", pstDispMode->disp_mode);
            return SAL_FAIL;
        }
    }

    XSP_LOGI("set disp Mode %d \n", pstDispMode->disp_mode);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_DISP;
    }

    pstPicPrm->stDispMode.disp_mode = pstDispMode->disp_mode;
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetMode
 * @brief:      设置默认模式    ，双能3种模式
                单能2种模式：0伪彩模式0,1是伪彩模式1
 * @param[in]:  UINT32 chan   通道号，预留
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetMode(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_DEFAULT_STYLE *pstModeSet = (XSP_DEFAULT_STYLE *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set default mode prm [%p] or pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set default mode chan %d is error!", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("previous u32ImgProcMode: 0x%x, xsp_style: %d\n", g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode, pstModeSet->xsp_style);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        /* 清空单模式按键成像处理 */
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode &= (~XSP_IMG_PROC_DEFORI_RANGE);
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_DEFAULT;
    }

    xsp_set_rt_default(&g_xsp_common.stUpdatePrm.stXspRtParm);

    /*不同的默认模式，打开若干功能*/
    if (XRAY_ENERGY_SINGLE == pstCapXrayIn->xray_energy_num)
    {
        switch (pstModeSet->xsp_style)
        {
            case XSP_DEFAULT_STYLE_0:
                g_xsp_common.stUpdatePrm.stXspRtParm.stPseudoColor.pseudo_color = XSP_DEFAULT_STYLE_0;
                break;

            case XSP_DEFAULT_STYLE_1:
                g_xsp_common.stUpdatePrm.stXspRtParm.stPseudoColor.pseudo_color = XSP_DEFAULT_STYLE_1;
                break;

            default:
                XSP_LOGE("Set Default mode %d is error!", pstModeSet->xsp_style);
                SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);
                return SAL_FAIL;
        }
    }
    else if (XRAY_ENERGY_DUAL == pstCapXrayIn->xray_energy_num)
    {
        switch (pstModeSet->xsp_style)
        {
            case XSP_DEFAULT_STYLE_0:
                g_xsp_common.stUpdatePrm.stXspRtParm.stDefaultStyle.xsp_style = XSP_DEFAULT_STYLE_0;
                break;

            case XSP_DEFAULT_STYLE_1:
                g_xsp_common.stUpdatePrm.stXspRtParm.stDefaultStyle.xsp_style = XSP_DEFAULT_STYLE_1;
                break;

            case XSP_DEFAULT_STYLE_2:
                g_xsp_common.stUpdatePrm.stXspRtParm.stDefaultStyle.xsp_style = XSP_DEFAULT_STYLE_2;
                break;

            default:
                XSP_LOGE("Set Default mode %d is error!", pstModeSet->xsp_style);
                SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);
                return SAL_FAIL;
        }
    }
    else
    {
        XSP_LOGE("set default: xray_energy_num %d is error!", pstCapXrayIn->xray_energy_num);
        SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);
        return SAL_FAIL;
    }

    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);
    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetOriginal
 * @brief:       设置原始参数
 * @param[in]:  UINT32 chan   通道号，预留
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetOriginal(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_ORIGINAL_MODE_PARAM *pstOriginal = (XSP_ORIGINAL_MODE_PARAM *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set Original prm [%p] or pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("the chan[%u] is invalid\n", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("previous u32ImgProcMode: 0x%x, Original enable: %d; level %d\n", g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode, pstOriginal->bEnable, pstOriginal->uiLevel);
    /* 原始模式开启时，所有图像处理均恢复默认；原始模式关闭时，所有图像处理保持不变，并设置默认增强等级 */
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        /* 清空单模式按键成像处理 */
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode &= (~XSP_IMG_PROC_DEFORI_RANGE);
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_ORIGINAL;
    }

    if (pstOriginal->bEnable)
    {
        xsp_set_rt_default(&g_xsp_common.stUpdatePrm.stXspRtParm);
    }

    g_xsp_common.stUpdatePrm.stXspRtParm.stOriginalMode.bEnable = pstOriginal->bEnable;
    g_xsp_common.stUpdatePrm.stXspRtParm.stOriginalMode.uiLevel = pstOriginal->uiLevel;

    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);
    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvGetVersion
 * @brief:      获取版本号
 * @param[in]:  UINT32 chan   通道号，预留
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvGetVersion(UINT32 chan, VOID *prm)
{
    CHAR *pVersionStr = NULL;

    /*通道0句柄*/
    if (NULL == g_xsp_common.stXspChn[0].handle)
    {
        XSP_LOGE("the XSP handle(chan 0) version is null\n");
        return SAL_FAIL;
    }

    pVersionStr = g_xsp_lib_api.get_version(g_xsp_common.stXspChn[0].handle);

    XSP_LOGI("xsp version is : %s\n", pVersionStr);

    if (strlen(pVersionStr) > 0)
    {
        memcpy((CHAR *)prm, pVersionStr, strlen(pVersionStr));
        return SAL_SOK;
    }
    else
    {
        XSP_LOGE("the XSP lib \n");
        return SAL_FAIL;
    }
}

/**
 * @function:   Xsp_DrvSetLp
 * @brief:       设置低穿参数
 * @param[in]:  UINT32 chan   通道号，预留
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetLp(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_LP_PARAM *pstLpSet = (XSP_LP_PARAM *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    DSPINITPARA *pstInitPrm = SystemPrm_getDspInitPara();

    if (NULL == prm || NULL == pstCapXrayIn || NULL == pstInitPrm)
    {
        XSP_LOGE("Set Lp prm [%p] pstCapXrayIn [%p] or pstInitPrm [%p] is error!\n", prm, pstCapXrayIn, pstInitPrm);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set Lp chan %d is error!", chan);
        return SAL_FAIL;
    }

    /*仅双能支持低穿*/
    if (XRAY_ENERGY_DUAL != pstCapXrayIn->xray_energy_num)
    {
        XSP_LOGE("Lp: xray_energy_num %d is not support!", pstCapXrayIn->xray_energy_num);
        return SAL_FAIL;
    }

    XSP_LOGI("set Lp: enable %d\n", pstLpSet->bEnable);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_LP;
    }

    /* 存在互斥关系 */
    g_xsp_common.stUpdatePrm.stXspRtParm.stLp.bEnable = pstLpSet->bEnable;
    g_xsp_common.stUpdatePrm.stXspRtParm.stHp.bEnable = SAL_FALSE;

    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);
    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetHp
 * @brief:       设置高穿参数
 * @param[in]:  UINT32 chan   通道号，预留
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetHp(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_HP_PARAM *pstHpSet = (XSP_HP_PARAM *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    DSPINITPARA *pstInitPrm = SystemPrm_getDspInitPara();

    if (NULL == prm || NULL == pstCapXrayIn || NULL == pstInitPrm)
    {
        XSP_LOGE("Set Hp prm [%p] pstCapXrayIn [%p] or pstInitPrm [%p] is error!\n", prm, pstCapXrayIn, pstInitPrm);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set Hp chan %d is error!", chan);
        return SAL_FAIL;
    }

    /*仅双能支持高穿*/
    if (XRAY_ENERGY_DUAL != pstCapXrayIn->xray_energy_num)
    {
        XSP_LOGE("Hp: xray_energy_num %d is error!", pstCapXrayIn->xray_energy_num);
        return SAL_FAIL;
    }

    XSP_LOGI("set Hp: enable %d\n", pstHpSet->bEnable);

    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_HP;
    }

    /*存在互斥关系，且互斥关系自研和研究院不同*/
    g_xsp_common.stUpdatePrm.stXspRtParm.stHp.bEnable = pstHpSet->bEnable;
    g_xsp_common.stUpdatePrm.stXspRtParm.stLp.bEnable = SAL_FALSE;

    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetHp
 * @brief:       设置局增参数
 * @param[in]:  UINT32 chan   通道号，预留
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetLe(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_LE_PARAM *pstLeSet = (XSP_LE_PARAM *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    DSPINITPARA *pstInitPrm = SystemPrm_getDspInitPara();

    if (NULL == prm || NULL == pstCapXrayIn || NULL == pstInitPrm)
    {
        XSP_LOGE("Set Le prm [%p] pstCapXrayIn [%p] or pstInitPrm [%p] is error!\n", prm, pstCapXrayIn, pstInitPrm);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set Le chan %d is error!", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("set Le: enable %d\n", pstLeSet->bEnable);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_LE;
    }

    g_xsp_common.stUpdatePrm.stXspRtParm.stLe.bEnable = pstLeSet->bEnable;

    g_xsp_common.stUpdatePrm.stXspRtParm.stUe.bEnable = SAL_FALSE;
    g_xsp_common.stUpdatePrm.stXspRtParm.stEe.bEnable = SAL_FALSE;

    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);
    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetAbsor
 * @brief:      设置可变吸收率
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetAbsor(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_VAR_ABSOR_PARAM *pstSetAbsor = (XSP_VAR_ABSOR_PARAM *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set Absor prm [%p] pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set Absor chan %d is error!", chan);
        return SAL_FAIL;
    }

    if (pstSetAbsor->uiLevel >= XSP_VAR_ABSORP_LEVEL_MAX)
    {
        XSP_LOGE("set absor level %d is error!\n", pstSetAbsor->uiLevel);
        return SAL_FAIL;
    }

    XSP_LOGI("set Absor enble %d, level %d.\n", pstSetAbsor->bEnable, pstSetAbsor->uiLevel);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_ABSOR;
    }

    g_xsp_common.stUpdatePrm.stXspRtParm.stVarAbsor.bEnable = !!pstSetAbsor->bEnable;
    g_xsp_common.stUpdatePrm.stXspRtParm.stVarAbsor.uiLevel = pstSetAbsor->uiLevel;
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetLum
 * @brief:    设置亮度
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetLum(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_LUMINANCE_PARAM *pstLum = (XSP_LUMINANCE_PARAM *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set Lum prm [%p] pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set Lum chan %d is error!", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("Set Lum uiLevel %u\n", pstLum->uiLevel);

    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

    if (g_xsp_common.stUpdatePrm.stXspRtParm.stLum.uiLevel == pstLum->uiLevel)
    {
        XSP_LOGI("lum is the same, as %u\n", pstLum->uiLevel);
        SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);
        return SAL_SOK;
    }

    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_LUMINANCE;
    }

    g_xsp_common.stUpdatePrm.stXspRtParm.stLum.uiLevel = pstLum->uiLevel;

    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);
    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetUnpen
 * @brief:      设置Unpen
 * @param[in]:  UINT32 chan   通道号，预留
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetUnpen(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    SAL_STATUS s32Ret = SAL_SOK;
    XSP_UNPEN_PARAM *pstSetUnpen = (XSP_UNPEN_PARAM *)prm;
    XSP_BASE_PRM *pstBasePrm = &g_xsp_common.stUpdatePrm.stBasePrm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("the prm[%p] OR pstCapXrayIn[%p] is NULL\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    XSP_LOGI("Enable: %d, Sensitivity: %d, Color: %d\n", pstSetUnpen->uiEnable, pstSetUnpen->uiSensitivity, pstSetUnpen->uiColor);

    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        s32Ret = g_xsp_lib_api.set_unpen_alert(g_xsp_common.stXspChn[i].handle, pstSetUnpen->uiEnable, pstSetUnpen->uiSensitivity, pstSetUnpen->uiColor);
        if (SAL_SOK != s32Ret)
        {
            XSP_LOGE("chan %u, set_unpen_alert failed, Enable: %d, Sensitivity: %d, Color: %d\n", i,
                     pstSetUnpen->uiEnable, pstSetUnpen->uiSensitivity, pstSetUnpen->uiColor);
            break;
        }
    }

    if (SAL_SOK == s32Ret)
    {
        memcpy(&pstBasePrm->stUnpenSet, pstSetUnpen, sizeof(XSP_UNPEN_PARAM));
        pstBasePrm->stUnpenSet.uiColor = XSP_OSD_BLUE;
    }

    return s32Ret;
}

/**
 * @function:   Xsp_DrvSetPseudoColor
 * @brief:      设置伪彩
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetPseudoColor(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_PSEUDO_COLOR_PARAM *pstPseudoColor = (XSP_PSEUDO_COLOR_PARAM *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set PseudoColor prm [%p] pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set PseudoColor chan %d is error!", chan);
        return SAL_FAIL;
    }

    /*仅单能支持*/
    if (XRAY_ENERGY_SINGLE != pstCapXrayIn->xray_energy_num)
    {
        XSP_LOGE("Pseudo Color is not supported by dual [%d]!", pstCapXrayIn->xray_energy_num);
        return SAL_FAIL;
    }

    if (pstPseudoColor->pseudo_color >= XSP_PSEUDO_COLOR_NUM)
    {
        XSP_LOGE("Pseudo Color: mode %d is error!", pstPseudoColor->pseudo_color);
        return SAL_FAIL;
    }

    XSP_LOGI("Set Pseudo Color: uiMode %d\n", pstPseudoColor->pseudo_color);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_PSEUDO_COLOR;
    }

    g_xsp_common.stUpdatePrm.stXspRtParm.stPseudoColor.pseudo_color = pstPseudoColor->pseudo_color;
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetMirror
 * @brief:      设置镜像
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetMirror(UINT32 chan, VOID *prm)
{
    XSP_VERTICAL_MIRROR_PARAM *pstMirror = (XSP_VERTICAL_MIRROR_PARAM *)prm;
    XSP_BASE_PRM *pstBasePrm = &g_xsp_common.stUpdatePrm.stBasePrm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set Mirror prm [%p] pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set Mirror chan %d is error!", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("set mirror[chan %d]: enable: %u, base mirror enable %d.\n",
             chan, pstMirror->bEnable, pstBasePrm->stMirrorPrm[chan].bEnable);

    /*用户设置的镜像操作，需要清除智能信息*/
    if (pstMirror->bEnable != pstBasePrm->stMirrorPrm[chan].bEnable)
    {
        Xpack_DrvClearOsdOnScreen(chan);
    }

    /*单视角，只支持通道0设置；双视角，分别设置镜像操作*/
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    memcpy(&pstBasePrm->stMirrorPrm[chan], pstMirror, sizeof(XSP_VERTICAL_MIRROR_PARAM));
    g_xsp_common.stXspChn[chan].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_MIRROR;
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetSusAlert
 * @brief:      设置可疑物报警
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetSusAlert(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    SAL_STATUS s32Ret = SAL_SOK;
    XSP_SUS_ALERT *pstSusAlert = (XSP_SUS_ALERT *)prm;
    XSP_BASE_PRM *pstBasePrm = &g_xsp_common.stUpdatePrm.stBasePrm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("the prm[%p] OR pstCapXrayIn[%p] is NULL\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    XSP_LOGI("Enable %d, Sensitivity %d, Color %d\n", pstSusAlert->uiEnable, pstSusAlert->uiSensitivity, pstSusAlert->uiColor);

    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        s32Ret = g_xsp_lib_api.set_sus_alert(g_xsp_common.stXspChn[i].handle, pstSusAlert->uiEnable, pstSusAlert->uiSensitivity, pstSusAlert->uiColor);
        if (SAL_SOK != s32Ret)
        {
            XSP_LOGE("chan %u, set_sus_alert failed, Enable %d, Sensitivity %d, Color %d\n", i,
                     pstSusAlert->uiEnable, pstSusAlert->uiSensitivity, pstSusAlert->uiColor);
            break;
        }
    }

    if (SAL_SOK == s32Ret)
    {
        memcpy(&pstBasePrm->stSusAlert, pstSusAlert, sizeof(XSP_SUS_ALERT));
        pstBasePrm->stSusAlert.uiColor = XSP_OSD_RED; /*画框默认显示红色*/
    }

    return s32Ret;
}

/**
 * @function:   Xsp_DrvSetEnhancedScan
 * @brief:      设置超薄扫描
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetEnhancedScan(UINT32 chan, VOID *prm)
{
    XSP_ENHANCED_SCAN_PARAM *pstEnhancedScan = (XSP_ENHANCED_SCAN_PARAM *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set EnhancedScan prm [%p] pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set EnhancedScan chan %d is error!\n", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("Set Ultrathin Scan: enable %d\n", pstEnhancedScan->bEnable);

    /*强扫用于包裹分割，不涉及到成像处理*/
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    memcpy(&g_xsp_common.stUpdatePrm.stXspRtParm.stEnhancedScan, pstEnhancedScan, sizeof(XSP_ENHANCED_SCAN_PARAM));
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);
    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetImgParam
 * @brief:      设置成像参数
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetBkgParam(UINT32 chan, XSP_BKG_PARAM *pstBkgParam)
{
    UINT32 i = 0;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == pstBkgParam || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set pstBkgParam [%p] pstCapXrayIn [%p] is error!\n", pstBkgParam, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set pstBkgParam chan %d is error!", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("set BkgDetTh: %u, FullUpdateRatio: %u\n", pstBkgParam->uiBkgDetTh, pstBkgParam->uiFullUpdateRatio);

    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_BKG_PARAM;
    }

    g_xsp_common.stUpdatePrm.stXspRtParm.stBkg.uiBkgDetTh = pstBkgParam->uiBkgDetTh;
    g_xsp_common.stUpdatePrm.stXspRtParm.stBkg.uiFullUpdateRatio = pstBkgParam->uiFullUpdateRatio;
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetColorTable
 * @brief:      设置颜色表
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
INT32 Xsp_DrvSetColorTable(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_COLOR_TABLE_PARAM *pstColorTable = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set ColorTable prm [%p] pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set ColorTable chan %d is error!", chan);
        return SAL_FAIL;
    }

    /*仅双能支持设置颜色表*/
    if (XRAY_ENERGY_DUAL != pstCapXrayIn->xray_energy_num)
    {
        XSP_LOGE("Color Table: xray_energy_num %d is error!", pstCapXrayIn->xray_energy_num);
        return SAL_FAIL;
    }

    pstColorTable = (XSP_COLOR_TABLE_PARAM *)prm;
    if (pstColorTable->enConfigueTable >= XSP_COLOR_TABLE_NUM)
    {
        XSP_LOGE("Color Table: mode %d is error!", pstColorTable->enConfigueTable);
        return SAL_FAIL;
    }

    XSP_LOGI("Set Color Table: uiMode %d\n", pstColorTable->enConfigueTable);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_COLOR_TABLE;
    }

    g_xsp_common.stUpdatePrm.stXspRtParm.stColorTable.enConfigueTable = pstColorTable->enConfigueTable;
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetColorsImaging
 * @brief:      设置三色六色显示
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
INT32 Xsp_DrvSetColorsImaging(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    XSP_COLORS_IMAGING_PARAM *pstColorsImaging = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set Colors Imaging prm [%p] pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set Colors Imaging chan %d is error!", chan);
        return SAL_FAIL;
    }

    pstColorsImaging = (XSP_COLORS_IMAGING_PARAM *)prm;
    if (pstColorsImaging->enColorsImaging >= XSP_COLORS_IMAGING_NUM)
    {
        XSP_LOGE("Colors Imaging: mode %d is error!", pstColorsImaging->enColorsImaging);
        return SAL_FAIL;
    }

    XSP_LOGI("Set Colors Imaging: uiMode %d\n", pstColorsImaging->enColorsImaging);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_COLORS_IMAGING;
    }

    g_xsp_common.stUpdatePrm.stXspRtParm.stColorsImaging.enColorsImaging = pstColorsImaging->enColorsImaging;
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    xsp_update_img_proc_param_to_app(&g_xsp_common.stUpdatePrm.stXspRtParm);

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetBlankSlice
 * @brief:      设置数据处理类型，数据类型变换需要重新设置参数
 * @param[in]:  UINT32 chan   通道号，预留
 * @param[in]:  XSP_PROCESS_PARAM * pstProcessParam     参数
 * @param[out]: None
 * @return:     INT32
 */
INT32 Xsp_DrvSetBlankSlice(UINT32 chan, XSP_SET_RM_BLANK_SLICE *pstRmBlankSlice)
{
    UINT32 i = 0;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == pstRmBlankSlice)
    {
        XSP_LOGE("chan %u, the pstRmBlankSlice is NULL\n", chan);
        return SAL_FAIL;
    }

    XSP_LOGI("chan %u, set blank slice : %d level:%d\n", chan, pstRmBlankSlice->bEnable, pstRmBlankSlice->uiLevel);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stRtPackDivS.bRmBlankSlice = (pstRmBlankSlice->uiLevel >= 100) ? SAL_FALSE : !!pstRmBlankSlice->bEnable;
        g_xsp_common.stXspChn[i].stRtPackDivS.u32BlkSliceDispCnt = SAL_CLIP(pstRmBlankSlice->uiLevel, 0, 100);
    }

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetDeformityCorrection
 * @brief:      设置畸变矫正，不区分通道号
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetDeformityCorrection(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    SAL_STATUS s32Ret = SAL_SOK;
    XSP_DEFORMITY_CORRECTION *pstDeformityCorr = (XSP_DEFORMITY_CORRECTION *)prm;
    XSP_BASE_PRM *pstBasePrm = &g_xsp_common.stUpdatePrm.stBasePrm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set DeformityCorrection prm [%p] pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    XSP_LOGI("DeformityCor: enable: %d, base DeformityCor enable %d\n",
             pstDeformityCorr->bEnable, pstBasePrm->stDeformityCor.bEnable);

    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        s32Ret = g_xsp_lib_api.set_deformity_correction(g_xsp_common.stXspChn[i].handle, pstDeformityCorr->bEnable);
        if (SAL_SOK != s32Ret)
        {
            XSP_LOGE("chan %u, set_deformity_correction failed\n", i);
            break;
        }

        /*所有视角：设置畸变校正，清除智能信息*/
        if (pstDeformityCorr->bEnable != pstBasePrm->stDeformityCor.bEnable)
        {
            Xpack_DrvClearOsdOnScreen(i);
        }
    }

    if (SAL_SOK == s32Ret)
    {
        memcpy(&pstBasePrm->stDeformityCor, pstDeformityCorr, sizeof(XSP_DEFORMITY_CORRECTION));
    }

    return s32Ret;
}

/**
 * @function:   Xsp_DrvSetColorAdjust
 * @brief:      设置颜色调节
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetColorAdjust(UINT32 chan, VOID *prm)
{
    XSP_COLOR_ADJUST_PARAM *pstColorAdjust = (XSP_COLOR_ADJUST_PARAM *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d! \n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("ColorAdjust Set prm [%p] pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    XSP_LOGI("set pstColorAdjust: uiLevel: %u\n", pstColorAdjust->uiLevel);
    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    g_xsp_common.stXspChn[chan].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_COLOR_ADJUST;
    g_xsp_common.stUpdatePrm.stBasePrm.stColorAdjust[chan].uiLevel = pstColorAdjust->uiLevel;
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    return SAL_SOK;
}


/**
 * @function    Xsp_DrvSetNoiseReduce
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS Xsp_DrvSetNoiseReduce(UINT32 chan, UINT32 u32NrLevel)
{
    SAL_STATUS s32Ret = SAL_SOK;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d! \n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    XSP_LOGI("chan %u, set noise reduce level: %u\n", chan, u32NrLevel);

    s32Ret = g_xsp_lib_api.set_noise_reduction(g_xsp_common.stXspChn[chan].handle, SAL_CLIP(u32NrLevel, 0, 100));
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("chan %u, set_noise_reduction failed, NrLevel: %u\n", chan, u32NrLevel);
        s32Ret = SAL_FAIL;
    }

    return s32Ret;
}

/**
 * @function    Xsp_DrvSetContrast
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS Xsp_DrvSetContrast(UINT32 chan, UINT32 u32ContrastLevel)
{
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d! \n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    XSP_LOGI("chan %u, set contrast level: %u\n", chan, u32ContrastLevel);

    SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    g_xsp_common.stXspChn[chan].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_CONTRAST;
    g_xsp_common.stUpdatePrm.stBasePrm.stContrast[chan].uiLevel = SAL_CLIP(u32ContrastLevel, 0, 100);
    SAL_mutexTmUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc, __FUNCTION__, __LINE__);

    return SAL_SOK;
}

/**
 * @function    Xsp_DrvSetLeThRange
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS Xsp_DrvSetLeThRange(UINT32 chan, UINT32 u32LeThLower, UINT32 u32LeThUpper)
{
    SAL_STATUS s32Ret = SAL_SOK;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d! \n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    if (u32LeThLower > u32LeThUpper)
    {
        XSP_LOGE("chan %d, the LeThLower[%u] is bigger than LeThUpper[%u]\n", chan, u32LeThLower, u32LeThUpper);
        return SAL_FAIL;
    }

    XSP_LOGI("chan %u, set LE th range: %u ~ %u\n", chan, u32LeThLower, u32LeThUpper);

    s32Ret = g_xsp_lib_api.set_le_th_range(g_xsp_common.stXspChn[chan].handle, u32LeThLower, u32LeThUpper);
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("chan %u, set_le_th_range failed, range: %u ~ %u\n", chan, u32LeThLower, u32LeThUpper);
        s32Ret = SAL_FAIL;
    }

    return s32Ret;
}

/**
 * @function    Xsp_DrvSetDtGrayRange
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS Xsp_DrvSetDtGrayRange(UINT32 chan, UINT32 u32DtGrayLower, UINT32 u32DtGrayUpper)
{
    SAL_STATUS s32Ret = SAL_SOK;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d! \n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    if (u32DtGrayLower > u32DtGrayUpper)
    {
        XSP_LOGE("chan %d, the DtGrayLower[%u] is bigger than DtGrayUpper[%u]\n", chan, u32DtGrayLower, u32DtGrayUpper);
        return SAL_FAIL;
    }

    XSP_LOGI("chan %u, set DT gray range: %u ~ %u\n", chan, u32DtGrayLower, u32DtGrayUpper);

    s32Ret = g_xsp_lib_api.set_dt_gray_range(g_xsp_common.stXspChn[chan].handle, u32DtGrayLower, u32DtGrayUpper);
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("chan %u, set_dt_gray_range failed, range: %u ~ %u\n", chan, u32DtGrayLower, u32DtGrayUpper);
        s32Ret = SAL_FAIL;
    }

    return s32Ret;
}

/**
 * @fn      Xsp_DrvSetPackageDivide
 * @brief   设置包裹分割参数
 *
 * @param   [IN] chan 通道号，无效
 * @param   [IN] XSP_PACKAGE_DIVIDE_PARAM *pstDivParam 分割参数
 * @return  SAL_STATUS SAL_SOK-设置成功，SAL_FAIL-设置失败
 */
SAL_STATUS Xsp_DrvSetPackageDivide(UINT32 chan, XSP_PACKAGE_DIVIDE_PARAM *pstDivParam)
{
    UINT32 i = 0;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    XSP_CHECK_PTR_IS_NULL(pstDivParam, SAL_FAIL);
    if (XRAY_PS_PLAYBACK_MASK & g_xsp_common.stXspChn[0].enProcStat)
    {
        XSP_LOGE("Setting package divide is only supported in real-time preview mode, but now system is in playback mode\n");
        return SAL_FAIL;
    }
    if (0 != Xpack_DrvGetDispBuffStatus(0))
    {
        XSP_LOGE("Setting package divide is only supported in idle mode, but now system is in running\n");
        return SAL_FAIL;
    }
    if (1 == pstCapXrayIn->xray_in_chan_cnt && XSP_PACK_DIVIDE_USE_SLAVE == pstDivParam ->u32segMaster)
    {
        XSP_LOGE("Setting package divide as using slave is only supported in multi view machine, current view number is 1\n");
        return SAL_FAIL;
    }
    if (pstDivParam->enDivMethod >= XSP_PACK_DIVIDE_METHOD_NUM)
    {
        XSP_LOGE("the enDivMethod(%d) is invalid, should be less than %d\n",
                 pstDivParam->enDivMethod, XSP_PACK_DIVIDE_METHOD_NUM);
        return SAL_FAIL;
    }

    XSP_LOGI("set package divide method: %d, sensitivity: %u ,u32segMaster :%d\n", pstDivParam->enDivMethod, pstDivParam->u32DivSensitivity,pstDivParam ->u32segMaster);
    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        g_xsp_common.stXspChn[i].stRtPackDivS.enMethod = pstDivParam->enDivMethod;
        g_xsp_common.stXspChn[i].stRtPackDivS.u32segMaster = pstDivParam ->u32segMaster;
    }

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetDoseCorrect
 * @brief:      剂量波动修正
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetDoseCorrect(UINT32 chan, VOID *prm)
{
    UINT32 i = 0;
    SAL_STATUS s32Ret = SAL_SOK;
    XSP_DOSE_CORRECT_PARAM *pstDoseCorrect = (XSP_DOSE_CORRECT_PARAM *)prm;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (NULL == prm || NULL == pstCapXrayIn)
    {
        XSP_LOGE("Set DoseCorrect prm [%p] pstCapXrayIn [%p] is error!\n", prm, pstCapXrayIn);
        return SAL_FAIL;
    }

    if (pstDoseCorrect->uiLevel > 100)
    {
        XSP_LOGE("Dose Correct Level: %d is error!", pstDoseCorrect->uiLevel);
        return SAL_FAIL;
    }

    XSP_LOGI("Dose Correct Level: %d\n", pstDoseCorrect->uiLevel);

    for (i = 0; i < pstCapXrayIn->xray_in_chan_cnt; i++)
    {
        s32Ret = g_xsp_lib_api.set_dose_correct(g_xsp_common.stXspChn[i].handle, pstDoseCorrect->uiLevel);
        if (SAL_SOK != s32Ret)
        {
            XSP_LOGE("chan %u, set_dose_correct failed!\n", i);
            s32Ret = SAL_FAIL;
            break;
        }
    }

    return s32Ret;
}

/**
 * @function    Xsp_DrvSetStrech
 * @brief       设置拉伸比例
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID *prm     参数指针
 * @param[out]
 * @return
 */
SAL_STATUS Xsp_DrvSetStretch(UINT32 chan, XSP_STRETCH_MODE_PARAM *prm)
{
    SAL_STATUS s32Ret = SAL_SOK;
    XSP_STRETCH_MODE_PARAM *ratio = prm;
    UINT32 stretchratio;  /* 送给算法的拉伸比例 */
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d! \n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    /*拉伸比例为50-100暂不支持，且应用下发拉伸比例不能超过100*/
    if (ratio->uiLevel > 50 && ratio->uiLevel < 100)
    {
        XSP_LOGW("This range is not supported at this time \n");
    }
    else if (ratio->uiLevel > 100)
    {
        XSP_LOGE("ERROR : The stretch ratio is out of range\n");
        return SAL_FAIL;
    }

    /*不开启拉伸模式时，将拉伸比例默认设置为100*/
    if (ratio->bEnable == SAL_FALSE)
    {
        stretchratio = 100;
        s32Ret = g_xsp_lib_api.set_stretch_ratio(g_xsp_common.stXspChn[chan].handle, stretchratio);
        if (SAL_SOK != s32Ret)
        {
            XSP_LOGE("chan %u, set stretch ratio failed, ratio: %u\n", chan, stretchratio);
            s32Ret = SAL_FAIL;
        }
    }
    else
    {
        /*应用输入拉伸比例为0~50，算法接收的范围80~100，该处传给算法前进行换算*/
        stretchratio = 80 + (UINT32)(ratio->uiLevel * 0.4);
        s32Ret = g_xsp_lib_api.set_stretch_ratio(g_xsp_common.stXspChn[chan].handle, stretchratio);
        if (SAL_SOK != s32Ret)
        {
            XSP_LOGE("chan %u, set stretch ratio failed, ratio: %u\n", chan, stretchratio);
            s32Ret = SAL_FAIL;
        }
    }

    XSP_LOGI("Xsp_DrvSetStretch, chan:%d, enable:%d, level:%d.\n", chan, ratio->bEnable, ratio->uiLevel);

    return s32Ret;
}

/**
 * @function:   Xsp_DrvSetBkgColor
 * @brief:      
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID   *prm   参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetBkgColor(UINT32 chan, VOID *prm)
{
    SAL_STATUS ret = SAL_SOK;
    U08 u8RValue = 0, u8GValue = 0, u8BValue = 0;
    U08 u8YValue = 0, u8UValue = 0, u8VValue = 0;
    XSP_BKG_COLOR *pstXspBkgColor = (XSP_BKG_COLOR *)prm;
    CAPB_XSP *pstCapbXsp = capb_get_xsp();

    XSP_CHECK_PTR_IS_NULL(prm, SAL_FAIL);

    if (pstXspBkgColor->enDataFmt != DSP_IMG_DATFMT_BGR24 && pstXspBkgColor->enDataFmt != DSP_IMG_DATFMT_BGRA32)
    {
        XSP_LOGE("chan %d, invalid bkg format! only support fmt:0x%x and fmt:0x%x, received fmt:0x%x, value:0x%x\n",
                 chan, DSP_IMG_DATFMT_BGR24, DSP_IMG_DATFMT_BGRA32, pstXspBkgColor->enDataFmt, pstXspBkgColor->uiBkgValue);
        return SAL_FAIL;
    }

    if (DSP_IMG_DATFMT_YUV420SP_VU == pstCapbXsp->enDispFmt)
    {
        u8RValue = pstXspBkgColor->uiBkgValue >> 16 & 0xFF;
        u8GValue = pstXspBkgColor->uiBkgValue >> 8 & 0xFF;
        u8BValue = pstXspBkgColor->uiBkgValue >> 0 & 0xFF;

        u8YValue = SAL_CLIP(( 0.2126 * u8RValue +  0.7152 * u8GValue +  0.0722 * u8BValue +   0), 16.0, 235.0);
        u8UValue = SAL_CLIP((-0.1146 * u8RValue + -0.3854 * u8GValue +  0.5000 * u8BValue + 128), 16.0, 235.0);
        u8VValue = SAL_CLIP(( 0.5000 * u8RValue + -0.4542 * u8GValue + -0.0468 * u8BValue + 128), 16.0, 235.0);

        g_xsp_common.stXspChn[chan].u32XspBgColorStd = u8YValue << 16 | u8UValue << 8 | u8VValue << 0;
    }
    else
    {
        g_xsp_common.stXspChn[chan].u32XspBgColorStd = pstXspBkgColor->uiBkgValue;
    }

    XSP_LOGI("chan %d, set bkg color, received fmt:0x%x value:0x%x, xsp bkg format:0x%x set value:0x%x\n",
             chan, pstXspBkgColor->enDataFmt, pstXspBkgColor->uiBkgValue, pstCapbXsp->enDispFmt, g_xsp_common.stXspChn[chan].u32XspBgColorStd);

    g_xsp_common.stXspChn[chan].stNormalData.u32ImgProcMode |= XSP_IMG_PROC_BKG_COLOR;

    return ret;
}

/**
 * @function:   Xsp_DrvSetGamma
 * @brief:      
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID   *prm   参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetGamma(UINT32 chan, VOID *prm)
{
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    XSP_GAMMA_PARAM *pstXspGamma = (XSP_GAMMA_PARAM *)prm;
    SAL_STATUS s32Ret = SAL_SOK;

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d! \n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }
    XSP_LOGI("chan %u, set gamma mode_switch %d value level: %u\n", chan, pstXspGamma->bEnable, pstXspGamma->uiLevel);
    
    s32Ret = g_xsp_lib_api.set_gamma(g_xsp_common.stXspChn[chan].handle, pstXspGamma->bEnable,pstXspGamma->uiLevel);
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("chan %u, rt pb set gamma failed mode_switch %d value level: %u\n", chan, pstXspGamma->bEnable, pstXspGamma->uiLevel);
        s32Ret = SAL_FAIL;
    }
    
    s32Ret = g_xsp_lib_api.set_gamma(gstXspTransProc.pXspHandle, pstXspGamma->bEnable,pstXspGamma->uiLevel);
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("chan %u, trans set gamma failed mode_switch %d value level: %u\n", chan, pstXspGamma->bEnable, pstXspGamma->uiLevel);
        s32Ret = SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function:   Xsp_DrvSetAixspRate
 * @brief:      
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID   *prm   参数指针
 * @param[out]: None
 * @return:     INT32
 */
SAL_STATUS Xsp_DrvSetAixspRate(UINT32 chan, UINT32 rate)
{
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    SAL_STATUS s32Ret = SAL_SOK;

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %d is error, chan cnt is %d! \n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }
    XSP_LOGI("chan %u, set aixsp %d \n", chan, rate);
    
    s32Ret = g_xsp_lib_api.set_aixsp_rate(g_xsp_common.stXspChn[chan].handle, SAL_CLIP(rate, 0, 100));
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGI("chan %d, set aixsp fail rate %d \n", chan, rate);
        s32Ret = SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Xsp_DrvSetColdHotThreshold
 * @brief:      设置冷热源阈值
 * @param[in]:  UINT32 chan   通道号
 * @param[in]:  VOID   *prm   参数指针
 * @return:     成功:SAL_SOK 失败:SAL_FAIL
 */
SAL_STATUS Xsp_DrvSetColdHotThreshold(UINT32 chan, VOID* prm)
{
    SAL_STATUS ret = SAL_SOK;
    XSP_COLDHOT_PARAM stColdHotPrm = {0};
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    XSP_CHECK_PTR_IS_NULL(prm, SAL_FAIL);
    XSP_CHECK_PTR_IS_NULL(pstCapXrayIn, SAL_FAIL);

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("Set ColdHotThreshold chan %d is error!\n", chan);
        return SAL_FAIL;
    }

    XSP_CHN_PRM *pstXspChn = &g_xsp_common.stXspChn[chan];
    sal_memcpy_s(&stColdHotPrm, sizeof(XSP_COLDHOT_PARAM), prm, sizeof(XSP_COLDHOT_PARAM));

    ret = g_xsp_lib_api.set_coldhot_threshold(pstXspChn->handle, stColdHotPrm.uiLevel);
    return ret;
}

/**
 * @function   Xsp_DrvSetAlgKey
 * @brief      设置算法库中key键对应参数的值，包括图像处理和参数设置
 * @param[in]  UINT32 chan     通道号
 * @param[in]  S32 s32OptNum   命令参数数量
 * @param[in]  S32 s32Key      键
 * @param[in]  S32 s32Val1     设置的value1
 * @param[in]  S32 s32Val2     设置的value2
 * @param[out] None
 * @return     int             成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Xsp_DrvSetAlgKey(UINT32 chan, S32 s32OptNum, S32 s32Key, S32 s32Val1, S32 s32Val2)
{
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        if (2 == chan && NULL != gstXspTransProc.pXspHandle)
        {
            return g_xsp_lib_api.set_akey(gstXspTransProc.pXspHandle, s32OptNum, s32Key, s32Val1, s32Val2);
        }
        else
        {
            XSP_LOGE("Invalid chan:%u\n", chan);
            return SAL_FAIL;
        }
    }
    else
    {
        return g_xsp_lib_api.set_akey(g_xsp_common.stXspChn[chan].handle, s32OptNum, s32Key, s32Val1, s32Val2);
    }
}

/**
 * @function   Xsp_DrvGetAlgKey
 * @brief      获取算法库中key键对应的值，包括图像处理和参数设置
 * @param[in]  UINT32 chan     通道号
 * @param[in]  S32 s32Key      键
 * @param[out] S32 *pVal1      获取的value1
 * @param[out] S32 *pVal2      获取的value2
 * @return     int             成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Xsp_DrvGetAlgKey(UINT32 chan, S32 s32Key, S32 *pImageVal, S32 *pVal1, S32 *pVal2)
{
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("the chan[%u] is invalid, range: [0, %u)\n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    return g_xsp_lib_api.get_akey(g_xsp_common.stXspChn[chan].handle, s32Key, pImageVal, pVal1, pVal2);
}

/**
 * @function   Xsp_DrvShowAllAlgKeyAndValue
 * @brief      在终端上打印所有的key和对应的value
 * @param[in]  UINT32 chan     通道号
 * @param[out] None
 * @return     int             成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS Xsp_DrvShowAllAlgKeyAndValue(UINT32 chan)
{
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        if (2 == chan && NULL != gstXspTransProc.pXspHandle)
        {
            return g_xsp_lib_api.get_all_key(gstXspTransProc.pXspHandle);
        }
        else
        {
            XSP_LOGE("Invalid chan:%u\n", chan);
            return SAL_FAIL;
        }
    }
    else
    {
        return g_xsp_lib_api.get_all_key(g_xsp_common.stXspChn[chan].handle);
    }
}


/**
 * @function:   Xsp_DrvChangeProcType
 * @brief:      设置数据处理类型，数据类型变换需要重新设置参数
 * @param[in]:  UINT32 chan   通道号，预留
 * @param[in]:  XSP_PROCESS_PARAM * pstProcessParam     参数
 * @param[out]: None
 * @return:     INT32
 */
void Xsp_DrvChangeProcType(UINT32 chan, XRAY_PROC_STATUS_E enProcStat)
{
    SAL_STATUS ret_val = SAL_SOK;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    XSP_CHN_PRM *pstXspChnPrm = NULL;
    DSA_NODE *pNode = NULL;

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %u is invalid, range: [0, %u)\n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return;
    }

    pstXspChnPrm = &g_xsp_common.stXspChn[chan];

    XSP_LOGW("chan %u, change xray proc type to 0x%x.\n", chan, enProcStat);

    if (XRAY_PS_NONE == enProcStat) /* 清空链表中处理状态为XSP_PSTG_RESERVED的节点 */
    {
        SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (NULL != (pNode = get_node_by_stage(pstXspChnPrm->lstDataProc, XSP_PSTG_RESERVED)))
        {
            DSA_LstDelete(pstXspChnPrm->lstDataProc, pNode); /* 从链表中删除该节点 */
            XSP_LOGW("chan %u, delete a 'XSP_PSTG_RESERVED' node.\n", chan);
        }

        SAL_mutexTmUnlock(&pstXspChnPrm->lstDataProc->sync.mid, __FUNCTION__, __LINE__);
    }

    SAL_mutexTmLock(&pstXspChnPrm->condProcStat.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    pstXspChnPrm->enProcStat = enProcStat;
    ret_val = SAL_CondSignal(&pstXspChnPrm->condProcStat, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
    SAL_mutexTmUnlock(&pstXspChnPrm->condProcStat.mid, __FUNCTION__, __LINE__);
    if (SAL_SOK != ret_val)
    {
        XSP_LOGE("chan %u, SAL_CondSignal failed\n", chan);
    }

    return;
}

/**
 * @function    Xsp_DrvChangeSpeed
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS Xsp_DrvChangeSpeed(UINT32 u32Chan, XRAY_PROC_FORM enForm, XRAY_PROC_SPEED enSpeed)
{
    SAL_STATUS retStat = SAL_SOK;
    UINT32 u32IntegralTime = 0, u32VideoOutputFps = 0, u32LinePerSlice = 0;
    UINT32 u32LinePerSecond = 0;
    UINT32 ts = 0, te = 0, tl = 2000; /* 超时时间为2秒 */
    XSP_CHN_PRM *pstXspChnPrm = NULL;
    XSP_NORMAL_DATA *pstNormalData = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    CAPB_XSP *pstCapXsp = capb_get_xsp();
    DSPINITPARA *pstDspInfo = SystemPrm_getDspInitPara();

    if (u32Chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("u32Chan %u is invalid, range: [0, %u)\n", u32Chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    if (XRAY_PS_PLAYBACK_MASK & g_xsp_common.stXspChn[u32Chan].enProcStat)
    {
        XSP_LOGE("chan %u, the current ProcStat is invalid: %d\n", u32Chan, g_xsp_common.stXspChn[u32Chan].enProcStat);
        return SAL_FAIL;
    }

    if (enForm >= XRAY_FORM_NUM)
    {
        XSP_LOGE("chan %u, the input form is invalid: %d\n", u32Chan, enForm);
        return SAL_FAIL;
    }

    if (enSpeed >= XRAY_SPEED_NUM)
    {
        XSP_LOGE("chan %u, the input speed is invalid: %d\n", u32Chan, enSpeed);
        return SAL_FAIL;
    }

    if (0 == pstCapXrayIn->st_xray_speed[enForm][enSpeed].line_per_slice) /* 不支持该速度 */
    {
        XSP_LOGE("chan %u, the input speed is unsupported: %d - %d\n", u32Chan, enForm, enSpeed);
        return SAL_FAIL;
    }

    pstXspChnPrm = &g_xsp_common.stXspChn[u32Chan];
    pstNormalData = &pstXspChnPrm->stNormalData;
    u32IntegralTime = pstCapXrayIn->st_xray_speed[enForm][enSpeed].integral_time;
    u32VideoOutputFps = pstDspInfo->stVoInitInfoSt.stVoInfoPrm[0].stDispDevAttr.videoOutputFps;
    u32LinePerSlice = pstCapXrayIn->st_xray_speed[enForm][enSpeed].line_per_slice;
    XSP_LOGI("chan %u, Form %d -> %d, Speed %d -> %d, LPS: %u, SliceIntegralTime: %uus, fps:%d\n",
             u32Chan, pstNormalData->enRtForm, enForm, pstNormalData->enRtSpeed, enSpeed, u32LinePerSlice,
             u32LinePerSlice * u32IntegralTime, u32VideoOutputFps);

    /* 更新实时过包条带的高度与积分时间 */
    pstNormalData->enRtForm = enForm;
    pstNormalData->enRtSpeed = enSpeed;
    u32LinePerSecond = ((1000000 + u32IntegralTime / 2) / u32IntegralTime) * pstCapXsp->resize_height_factor;
    pstNormalData->u32RtRefreshPerFrame = (u32LinePerSecond + u32VideoOutputFps / 2) / u32VideoOutputFps;

    /* 更新条带分段发送次数 */
    if (0 == u32LinePerSlice % pstNormalData->u32RtRefreshPerFrame)
    {
        /* 目前仅在有光障分割的情况下，才支持条带分段发送 */
        if (XSP_PACK_DIVIDE_HW_SIGNAL == pstXspChnPrm->stRtPackDivS.enMethod)
        {
            pstNormalData->u32RtSliceRefreshCnt = u32LinePerSlice / pstNormalData->u32RtRefreshPerFrame;
            if (pstNormalData->u32RtSliceRefreshCnt > XSP_FRESH_PER_SLICE)
            {
                XSP_LOGE("chan %u, the max refresh count is %u, but current is %u\n",
                         u32Chan, XSP_FRESH_PER_SLICE, pstNormalData->u32RtSliceRefreshCnt);
                return SAL_FAIL;
            }
        }
        else
        {
            pstNormalData->u32RtSliceRefreshCnt = 1;
        }
    }
    else
    {
        XSP_LOGW("chan %u, slice height %d, refresh col[%u] per frame are invalid, form:%d, speed:%d, integral time:%u\n",
                 u32Chan, u32LinePerSlice, pstNormalData->u32RtRefreshPerFrame, 
                 enForm, enSpeed, u32IntegralTime);
    }

    SAL_mutexTmLock(&pstXspChnPrm->lstDataProc->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    while (DSA_LstGetCount(pstXspChnPrm->lstDataProc) > 0)
    {
        if (tl + ts > te)
        {
            tl = tl + ts - te;
            ts = sal_get_tickcnt();
            SAL_CondWait(&pstXspChnPrm->lstDataProc->sync, tl, __FUNCTION__, __LINE__);
            te = sal_get_tickcnt();
        }
        else
        {
            XSP_LOGE("chan %u, wait lstDataProc to be empty timeout, number: %u\n",
                     u32Chan, DSA_LstGetCount(pstXspChnPrm->lstDataProc));
            break; /* 超时退出 */
        }
    }
    SAL_mutexUnlock(&pstXspChnPrm->lstDataProc->sync.mid);

    /* 清屏 */
    if (SAL_SOK == Xpack_DrvClearScreen(u32Chan, pstNormalData->u32RtRefreshPerFrame, pstNormalData->uiRtDirection))
    {
        /* 切换速度重置包裹分割参数 */
        pstXspChnPrm->stRtPackDivS.uiPackCurLine = 0;
        pstXspChnPrm->stRtPackDivS.u32SliceCnt = 0;
        pstXspChnPrm->stRtPackDivS.enPackTagLast = XSP_PACKAGE_NONE;
        pstNormalData->u32RtSliceNumAfterCls = 0; /* 重置屏幕上实时过包条带数 */

        retStat = g_xsp_lib_api.set_belt_speed(pstXspChnPrm->handle, pstCapXrayIn->st_xray_speed[enForm][enSpeed].belt_speed,
                                               pstCapXrayIn->st_xray_speed[enForm][enSpeed].line_per_slice, enSpeed);
        if (SAL_SOK != retStat)
        {
            XSP_LOGE("chan %u, set belt speed failed, Form: %d, Speed: %d, belt speed:%f, line_per_slice:%d \n",
                     u32Chan, enForm, enSpeed, pstCapXrayIn->st_xray_speed[enForm][enSpeed].belt_speed, pstCapXrayIn->st_xray_speed[enForm][enSpeed].line_per_slice);
        }
    }
    else
    {
        XSP_LOGE("chan %u, change speed to %d failed\n", u32Chan, enSpeed);
    }

    return retStat;
}


/**
 * @fn      xsp_get_current_rtspeed
 * @brief   获取当前实时预览过包速度
 * 
 * @param   [IN] u32Chan XRay通道号，取值范围[0, MAX_XRAY_CHAN-1]
 * 
 * @return  XRAY_PROC_SPEED 速度索引
 */
XRAY_PROC_SPEED xsp_get_current_rtspeed(UINT32 u32Chan)
{
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();

    if (u32Chan < pstCapXrayIn->xray_in_chan_cnt)
    {
        return g_xsp_common.stXspChn[u32Chan].stNormalData.enRtSpeed;
    }
    else
    {
        XSP_LOGE("u32Chan %u is invalid, range: [0, %u]\n", u32Chan, pstCapXrayIn->xray_in_chan_cnt-1);
        return XRAY_SPEED_LOW;
    }
}

/**
 * @fn      Xsp_DrvPrepareNewProcType
 * @brief   XSP切换处理模式（预览和回拉）
 * @param   [IN] chan 通道号
 * @param   [IN] enProcType 处理类型
 * @param   [IN] pstPbParam 回拉参数，仅enProcType为XRAY_TYPE_PLAYBACK有效
 * @return  SAL_STATUS SAL_SOK-成功，SAL_FALSE-失败
 */
SAL_STATUS Xsp_DrvPrepareNewProcType(UINT32 chan, XRAY_PROC_STATUS_E enProcType, XRAY_PB_PARAM *pstPbParam)
{
    UINT32 u32RtLineNumAfterCls = 0; /* 清屏后已显示的实时过包列数 */
    UINT32 u32RefreshCol = 0;           /* 每帧刷新列数 */
    XRAY_PROC_FORM enRtForm = XRAY_FORM_ORIGINAL;
    XRAY_PROC_SPEED enRtSpeed = XRAY_SPEED_LOW;
    XSP_CHN_PRM *pstXspChnPrm = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    CAPB_XSP *pstCapXsp = capb_get_xsp();

    if (chan >= pstCapXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %u is invalid, range: [0, %u)\n", chan, pstCapXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    pstXspChnPrm = &g_xsp_common.stXspChn[chan];
    if (XRAY_PS_RTPREVIEW == enProcType)
    {
        XSP_LOGW("chan: %d, to XRAY_PS_RTPREVIEW, ProcType:%d\n", chan, enProcType);
    }
    else
    {
        CHECK_PTR_NULL(pstPbParam, SAL_FAIL);
        XSP_LOGW("chan:%d, to XRAY_PS_PLAYBACK, PbType:%d, PbSpeed:%d, RtSliceNumAfterCls:%d\n", chan, pstPbParam->enPbType,
                 pstPbParam->enPbSpeed, pstXspChnPrm->stNormalData.u32RtSliceNumAfterCls);
    }

    /* 等待xsp处理节点清空 */
    Xsp_WaitProcListEmpty(chan, SAL_TRUE, 100, 20);

    enRtForm = pstXspChnPrm->stNormalData.enRtForm;
    enRtSpeed = pstXspChnPrm->stNormalData.enRtSpeed;
    if (enRtForm < XRAY_FORM_NUM && enRtSpeed < XRAY_SPEED_NUM)
    {
        u32RtLineNumAfterCls = (UINT32)(pstXspChnPrm->stNormalData.u32RtSliceNumAfterCls * 
                                pstCapXrayIn->st_xray_speed[enRtForm][enRtSpeed].line_per_slice * pstCapXsp->resize_height_factor);
    }

    /* 通知xpack模块模式切换 */
    if (XRAY_PS_RTPREVIEW == enProcType)
    {
        u32RefreshCol = pstXspChnPrm->stNormalData.u32RtRefreshPerFrame;
    }
    else
    {
        if (XRAY_PS_PLAYBACK_SLICE_FAST == enProcType)
        {
            u32RefreshCol = XSP_PB_SLICE_LINE_PER_FRAME; /* 其他情况回拉速度为默认值 */
        }
        else if (XRAY_PS_PLAYBACK_SLICE_XTS == enProcType)
        {
            /* XTS培训模式下的条带回拉，回拉每帧的刷新速率与当前实时过包的一致 */
            u32RefreshCol = pstXspChnPrm->stNormalData.u32RtRefreshPerFrame;
            pstXspChnPrm->stRtPackDivS.uiPackCurLine = 0;
            pstXspChnPrm->stRtPackDivS.enPackTagLast = XSP_PACKAGE_NONE;
            pstXspChnPrm->stNormalData.u32RtSliceNumAfterCls = 0; /* 重置屏幕上实时过包条带数 */
        }
        pstXspChnPrm->stNormalData.enPbMode = pstPbParam->enPbType;
        pstXspChnPrm->stNormalData.enPbSpeed = pstPbParam->enPbSpeed;
        pstXspChnPrm->stNormalData.bEnableSync = pstPbParam->bEnableSync;
    }
    Xpack_DrvChangeProcType(chan, enProcType, u32RefreshCol, u32RtLineNumAfterCls);

    if (enProcType == XRAY_PS_RTPREVIEW || enProcType == XRAY_PS_PLAYBACK_SLICE_FAST)
    {
        /* 
         * 切换到实时过包时，界面不清屏，对切换之前的全屏RAW数据做成像处理，强制刷新屏幕上的显示图像 
         * 切换到条带回拉时，过包不满一屏时需将图像刷新到屏幕另一侧
         */
        SAL_mutexTmLock(&g_xsp_common.stUpdatePrm.mutexImgProc, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        pstXspChnPrm->stNormalData.u32ImgProcMode |= XSP_IMG_PROC_FORCE_FLUSH;
        SAL_mutexUnlock(&g_xsp_common.stUpdatePrm.mutexImgProc);
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   Xsp_ShowStatus
 * @brief:
 * @param[in]:
 * @param[out]:
 * @return:  OK or ERR Information
 *******************************************************************/
void Xsp_ShowStatus(void)
{
    UINT32 i = 0, j = 0;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    XSP_CHN_PRM *pstXspChnPrm = NULL;
    DSA_NODE *pNode = NULL;
    XSP_DATA_NODE *pstDataNode = NULL;
    XSP_DEBUG_STATUS *pstDbgStatus = NULL;
    DSPINITPARA *pstDspShareParam = SystemPrm_getDspInitPara();

    SAL_print("\nXSP >>> chan | bxsp    errno |  proccnt | nodecnt | ps pt pb dir width height fsc\n");
    for (i = 0; i < pCapbXrayin->xray_in_chan_cnt; i++)
    {
        pstXspChnPrm = &g_xsp_common.stXspChn[i];
        SAL_print("  %10u | %2d %11x| %8u | %7u | \n", i, \
                pstDspShareParam->stSysStatus.bXspInitAbnormally, pstDspShareParam->stSysStatus.u32XspErrno,\
                pstXspChnPrm->stDbgStatus.u32ProcCnt, DSA_LstGetCount(pstXspChnPrm->lstDataProc));

        j = 0;
        pNode = DSA_LstGetHead(pstXspChnPrm->lstDataProc);
        while (NULL != pNode)
        {
            pstDataNode = (XSP_DATA_NODE *)pNode->pAdData;
            SAL_print("  %47u | %2d %2d %2d %3d %5u %6u %3d\n", j,
                      pstDataNode->enProcStage, pstDataNode->enProcType, pstDataNode->enPbMode, pstDataNode->enDispDir,
                      pstDataNode->stXRawInBuf.u32Width, pstDataNode->stXRawInBuf.u32Height, pstDataNode->bFscImgProc);
            pNode = pNode->next;
            j++;
        }
    }

    SAL_print("\n                      |      RT-slice     |      Playback     |      Refresh      |");
    SAL_print("\nXSP send cnt >>> chan | reqested  succeed | reqested  succeed | reqested  succeed |\n"); 
    for (i = 0; i < pCapbXrayin->xray_in_chan_cnt; i++)
    {
        pstDbgStatus = &g_xsp_common.stXspChn[i].stDbgStatus;
        SAL_print("                    %1u | %8u %8u | %8u %8u | %8u %8u |\n", i, 
                  pstDbgStatus->u32RtSendReqCnt, pstDbgStatus->u32RtSendSuccedCnt, 
                  pstDbgStatus->u32PbSendReqCnt, pstDbgStatus->u32PbSendSuccedCnt, 
                  pstDbgStatus->u32RefreshSendReqCnt, pstDbgStatus->u32RefreshSendSuccedCnt);
    }

    return;
}

/**
 * @function    Xsp_ShowPb2Xpack
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void Xsp_ShowPb2Xpack(UINT32 chan)
{
    UINT32 i = 0, j = 0, u32StartIdx = 0;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    XSP_DEBUG_PB2XPACK *pstPb2Xpack = NULL;

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        SAL_print("the chan[%u] is invalid, < %u", chan, pCapbXrayin->xray_in_chan_cnt);
        return;
    }

    u32StartIdx = g_xsp_common.stXspChn[chan].u32Pb2XapckStartIdx;
    SAL_print("\nPB2XPack > %d  FrameNo YuvO YuvW YuvH Stride Dir Flag Start  End PackW PackH Top Botm SA Target Type Conf Rect\n", chan);
    for (i = u32StartIdx; i < u32StartIdx + XSP_DEBUG_PB2XPACK_NUM; i++)
    {
        pstPb2Xpack = &g_xsp_common.stXspChn[chan].stPb2Xpack[i % XSP_DEBUG_PB2XPACK_NUM];
        SAL_print("%21u %4u %4u %4u %6u %3u %4u %5d %4d %5u %5u %3u %4u %2u %6u\n", pstPb2Xpack->u32FrameNo, pstPb2Xpack->u32YuvOffset,
                  pstPb2Xpack->u32Width, pstPb2Xpack->u32Height, pstPb2Xpack->u32Stride, pstPb2Xpack->u32DispDir, pstPb2Xpack->u32PackFlag,
                  (INT32)pstPb2Xpack->u32PackStart, (INT32)pstPb2Xpack->u32PackEnd, pstPb2Xpack->u32PackWidth, pstPb2Xpack->u32PackHeight,
                  pstPb2Xpack->u32PackTop, pstPb2Xpack->u32PackBottom,  pstPb2Xpack->u32ShowAll, pstPb2Xpack->stSvaInfo.target_num);
        for (j = 0; j < pstPb2Xpack->stSvaInfo.target_num; j++)
        {
            SAL_print("%106d] %3u %4u [%f, %f] - [%f, %f]\n",
                      j, pstPb2Xpack->stSvaInfo.target[j].type, pstPb2Xpack->stSvaInfo.target[j].visual_confidence,
                      pstPb2Xpack->stSvaInfo.target[j].rect.x, pstPb2Xpack->stSvaInfo.target[j].rect.y,
                      pstPb2Xpack->stSvaInfo.target[j].rect.width, pstPb2Xpack->stSvaInfo.target[j].rect.height);
        }
    }

    return;
}

/**
 * @function    Xsp_ShowTime
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void Xsp_ShowTime(UINT32 chan, UINT32 u32PrintType)
{
    CAPB_XRAY_IN *pCapbXray = capb_get_xrayin();
    debug_time pXspDbgTime = NULL;

    if (chan >= pCapbXray->xray_in_chan_cnt)
    {
        SAL_print("the chan[%u] is invalid, < %u", chan, pCapbXray->xray_in_chan_cnt);
        return;
    }
    pXspDbgTime = g_xsp_common.stXspChn[chan].pXspDbgTime;

    if (u32PrintType & 0x1)
    {
        dtime_print_time_point(pXspDbgTime, 160);
    }
    if (u32PrintType & 0x2)
    {
        dtime_print_duration(pXspDbgTime, 160);
    }
    if (u32PrintType & 0x4)
    {
        dtime_print_interval(pXspDbgTime, 160);
    }

    return;
}

/**
 * @function    Xsp_ShowIdentify
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void Xsp_ShowIdentify(UINT32 chan)
{
    UINT32 i = 0, j = 0, u32StartIdx = 0;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    XSP_DEBUG_IDENTIFY *pstDbgIdt = NULL;

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        SAL_print("chan(%u) is invalid, range: [0, %d)\n", chan, pCapbXrayin->xray_in_chan_cnt);
        return;
    }

    u32StartIdx = g_xsp_common.stXspChn[chan].u32DbgIdtIdx;
    SAL_print("\nIdt > %d PackId Width Height PackDivide     IdtReq   IdtStart     IdtEnd RstPackGet Target Type Coor\n", chan);
    for (i = u32StartIdx; i < u32StartIdx + XSP_DEBUG_IDENTIFY_NUM; i++)
    {
        pstDbgIdt = &g_xsp_common.stXspChn[chan].stDbgIdentify[i % XSP_DEBUG_IDENTIFY_NUM];
        SAL_print("%14u %5d %6u %10llu %10llu %10llu %10llu %10llu %2u %3u\n", pstDbgIdt->s32PackId,
                  pstDbgIdt->u32Width, pstDbgIdt->u32Height, pstDbgIdt->u64TimePackDivided,
                  pstDbgIdt->u64TimeIdentifyReq, pstDbgIdt->u64TimeIdentifyStart, pstDbgIdt->u64TimeIdentifyEnd,
                  pstDbgIdt->u64TimeIdtRstPackGet, pstDbgIdt->stIdtSusOrg.num, pstDbgIdt->stIdtUnpen.num);
        for (j = 0; j < pstDbgIdt->stIdtSusOrg.num; j++)
        {
            SAL_print("%114d.SusOrg | %5d /%3u [%u, %u] - [%u, %u]\n", 1, j, pstDbgIdt->stIdtSusOrg.num,
                      pstDbgIdt->stIdtSusOrg.rect[j].uiX, pstDbgIdt->stIdtSusOrg.rect[j].uiY,
                      pstDbgIdt->stIdtSusOrg.rect[j].uiWidth, pstDbgIdt->stIdtSusOrg.rect[j].uiHeight);
        }

        for (j = 0; j < pstDbgIdt->stIdtUnpen.num; j++)
        {
            SAL_print("%114d.Unpen  | %5d /%3u [%u, %u] - [%u, %u]\n", 2, j, pstDbgIdt->stIdtUnpen.num,
                      pstDbgIdt->stIdtUnpen.rect[j].uiX, pstDbgIdt->stIdtUnpen.rect[j].uiY,
                      pstDbgIdt->stIdtUnpen.rect[j].uiWidth, pstDbgIdt->stIdtUnpen.rect[j].uiHeight);
        }
    }

    return;
}

/**
 * @function    Xsp_ShowSliceCb
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void Xsp_ShowSliceCb(UINT32 chan)
{
    UINT32 i = 0, u32StartIdx = 0;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();
    XSP_DEBUG_SLICE_CB *pstDbgSliceCb = NULL;

    if (chan >= pCapbXrayin->xray_in_chan_cnt)
    {
        SAL_print("the chan[%u] is invalid, < %u", chan, pCapbXrayin->xray_in_chan_cnt);
        return;
    }

    u32StartIdx = g_xsp_common.stXspChn[chan].u32DbgSliceCbIdx;
    SAL_print("\nSliceCB > %d   ColNo Width Height Cont T   A   G Top Botm  Size       Addr\n", chan);
    for (i = u32StartIdx; i < u32StartIdx + XSP_DEBUG_SLICE_CB_NUM; i++)
    {
        pstDbgSliceCb = &g_xsp_common.stXspChn[chan].stDbgSliceCb[i % XSP_DEBUG_SLICE_CB_NUM];
        SAL_print("%19u %5u %6u %4u |%u %u %u %u| %4u %4u %6u %p\n", pstDbgSliceCb->uiColNo, pstDbgSliceCb->u32Width,
                  pstDbgSliceCb->u32Hight, pstDbgSliceCb->enSliceCont,
                  (pstDbgSliceCb->enPackageTag & 0xFF000000) >> 24, (pstDbgSliceCb->enPackageTag & 0xFF0000) >> 16,
                  (pstDbgSliceCb->enPackageTag & 0xFF00) >> 8, (pstDbgSliceCb->enPackageTag & 0xFF),
                  pstDbgSliceCb->u32Top, pstDbgSliceCb->u32Bottom, pstDbgSliceCb->u32SliceNrawSize, pstDbgSliceCb->pSliceNrawAddr);
    }

    return;
}

/**
 * @function:   Xsp_DrvGetPseudoBlankData
 * @brief:      获取伪过包数据，该数据从算法的矫正模板中获取
 * @param[in]:  UINT32 chan   通道号，预留
 * @param[in]:  UINT32 uiXrayWidth     xray数据宽
 * @param[in]:  UINT32 uiXrayHeight    xray数据高
 * @param[in]:  UINT16 *pu16Buf        数据buf
 * @param[out]: None
 * @return:     SAL_STATUS
 */
SAL_STATUS Xsp_DrvGetPseudoBlankData(UINT32 chan, UINT16 *pu16Buf, UINT32 u32Width, UINT32 u32Height, BOOL bPlanarFormat)
{
    UINT32 i = 0;
    UINT32 u32CorrSize = 0;
    INT32 s32Ret = 0;
    UINT8 *pu8LineDest = NULL, *pu8LineSrc = NULL;
    CAPB_XRAY_IN *pstCapXrayIn = capb_get_xrayin();
    CAPB_XSP *pstCapXsp = capb_get_xsp();

    if ((chan >= pstCapXrayIn->xray_in_chan_cnt) || (NULL == pu16Buf)
        || (u32Width != pstCapXrayIn->xraw_width_max) || (0 == u32Height))
    {
        XSP_LOGE("chan %u, pu16Buf[%p] OR u32Width[%u] OR u32Height[%u] is invalid\n",
                 chan, pu16Buf, u32Width, u32Height);
        return SAL_FAIL;
    }

    if (NULL == g_xsp_common.stXspChn[chan].handle)
    {
        XSP_LOGE("chan %u, the handle is NULL\n", chan);
        return SAL_FAIL;
    }

    s32Ret = g_xsp_lib_api.get_correction_data(g_xsp_common.stXspChn[chan].handle, XSP_CORR_FULLLOAD, pu16Buf, &u32CorrSize);
    if (SAL_SOK != s32Ret || u32CorrSize != pstCapXrayIn->xraw_width_max * pstCapXsp->xsp_original_raw_bw)
    {
        XSP_LOGE("chan %u, get_correction_data failed, s32Ret: %d, u32CorrSize: %u\n", chan, s32Ret, u32CorrSize);
        return SAL_FAIL;
    }

    if (XRAY_ENERGY_DUAL == pstCapXrayIn->xray_energy_num && bPlanarFormat) /* 双能平面格式，将低能与高能拆开，分别复制 */
    {
        /* 高能 */
        u32CorrSize = XRAW_HE_SIZE(u32Width, 1);
        pu8LineSrc = XRAW_HE_OFFSET(pu16Buf, u32Width, 1);
        pu8LineDest = XRAW_HE_OFFSET(pu16Buf, u32Width, u32Height); /* 定位到平面格式高能数据起始区 */
        for (i = 0; i < u32Height; i++)
        {
            memcpy(pu8LineDest, pu8LineSrc, u32CorrSize);
            pu8LineDest += u32CorrSize;
        }

        /* 低能 */
        u32CorrSize = XRAW_LE_SIZE(u32Width, 1);
        pu8LineSrc = XRAW_LE_OFFSET(pu16Buf, u32Width, 1);
        pu8LineDest = XRAW_LE_OFFSET(pu16Buf, u32Width, u32Height) + u32CorrSize; /* 跳过低能第一行 */
        for (i = 1; i < u32Height; i++)
        {
            memcpy(pu8LineDest, pu8LineSrc, u32CorrSize);
            pu8LineDest += u32CorrSize;
        }
    }
    else /* 双能交叉格式或单能，直接复制第一行 */
    {
        pu8LineSrc = (UINT8 *)pu16Buf;
        pu8LineDest = (UINT8 *)pu16Buf + u32CorrSize;
        for (i = 1; i < u32Height; i++)
        {
            memcpy(pu8LineDest, pu8LineSrc, u32CorrSize);
            pu8LineDest += u32CorrSize;
        }
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   Xsp_DrvConvertMirror
 * @brief:      镜像转换，用户态的上下镜像转换成成像算法态的旋转和镜像操作
 * @param[in]:  UINT32 u32Chn 通道号
 * @param[in]:  XSP_HANDLE_TYPE enHandleType 通道类型
 * @return:     返回类型
 *******************************************************************/
SAL_STATUS Xsp_DrvConvertMirror(UINT32 u32Chn, XSP_HANDLE_TYPE enHandleType)
{
    INT32 s32Ret = SAL_FAIL;             /*结果返回*/
    UINT32 u32Mirror = MIRROR_MODE_NONE; /*镜像类型*/
    UINT32 u32Rotate = ROTATE_MODE_NONE; /*旋转类型*/
    void *pXspHandle = NULL;             /*算法通道*/
    XSP_VERTICAL_MIRROR_PARAM *pstMirrorPrm = NULL;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();

    /*参数校验*/
    if (NULL == pCapbXrayin)
    {
        XSP_LOGE("pCapbXrayin %p is error.\n", pCapbXrayin);
        return SAL_FAIL;
    }

    /*通道校验*/
    if (u32Chn >= pCapbXrayin->xray_in_chan_cnt)
    {
        XSP_LOGE("u32Chn %d is error, xray_in_chan_cnt is %d.\n",
                 u32Chn,
                 pCapbXrayin->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    /*获取通道句柄*/
    if (XSP_HANDLE_RT_PB == enHandleType)
    {
        pXspHandle = g_xsp_common.stXspChn[u32Chn].handle;
    }
    else if (XSP_HANDLE_TRANS == enHandleType)
    {
        pXspHandle = gstXspTransProc.pXspHandle;
    }
    else
    {
        XSP_LOGE("enHandleType %d is error.\n", enHandleType);
        return SAL_FAIL;
    }

    if (NULL == pXspHandle)
    {
        XSP_LOGE("pXspHandle %p is error.\n", pXspHandle);
        return SAL_FAIL;
    }

    /*获取当前算法态的镜像和旋转类型*/
    s32Ret = g_xsp_lib_api.get_mirror(pXspHandle, &u32Mirror);
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("get_mirror failed!\n");
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    s32Ret = g_xsp_lib_api.get_rotate(pXspHandle, &u32Rotate);
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("get_rotate failed!\n");
        s32Ret = SAL_FAIL;
        goto EXIT;
    }
    pstMirrorPrm = &g_xsp_common.stUpdatePrm.stBasePrm.stMirrorPrm[u32Chn];

    XSP_LOGI("get mirror_type %u, rotate_type: %u\n", u32Mirror, u32Rotate);

    /*左向: 旋转270，镜像是正常出图方式*/
    if (ROTATE_MODE_270 == u32Rotate)
    {
        /*当前是关闭状态，则开启*/
        if (SAL_TRUE == pstMirrorPrm->bEnable)
        {
            if (MIRROR_MODE_VERTICAL == u32Mirror)
            {
                u32Mirror = MIRROR_MODE_NONE;
            }
        }
        else
        {
            if (MIRROR_MODE_NONE == u32Mirror)
            {
                u32Mirror = MIRROR_MODE_VERTICAL;
            }
        }
    }
    /*右向: 旋转90，无镜像是正常出图方式*/
    else if (ROTATE_MODE_90 == u32Rotate)
    {
        /*当前是关闭状态，则开启*/
        if (SAL_TRUE == pstMirrorPrm->bEnable)
        {
            if (MIRROR_MODE_NONE == u32Mirror)
            {
                u32Mirror = MIRROR_MODE_VERTICAL;
            }
        }
        else
        {
            if (MIRROR_MODE_VERTICAL == u32Mirror)
            {
                u32Mirror = MIRROR_MODE_NONE;
            }
        }
    }
    else
    {
        XSP_LOGE("error rotate type!\n");
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    s32Ret = g_xsp_lib_api.set_mirror(pXspHandle, u32Mirror);
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("set_mirror failed!\n");
        s32Ret = SAL_FAIL;
        goto EXIT;
    }

    return SAL_SOK;

EXIT:
    return s32Ret;
}

/******************************************************************
 * @function:   Xsp_DrvSetRotateAndMirror
 * @brief:      算法态的镜像，旋转操作
 * @param[in]:  UINT32 u32Chn 通道号
 * @param[in]:  XSP_HANDLE_TYPE enHandleType 通道类型
 * @param[in]:  UINT32 u32Direction 实时过包的方向
 * @return:     返回类型
 *******************************************************************/
SAL_STATUS Xsp_DrvSetRotateAndMirror(UINT32 u32Chn, XSP_HANDLE_TYPE enHandleType, XRAY_PROC_DIRECTION u32Direction, XRAY_PROC_TYPE enType)
{
    INT32 s32Ret = SAL_FAIL;             /*结果返回*/
    UINT32 u32Mirror = MIRROR_MODE_NONE; /*镜像类型*/
    UINT32 u32Rotate = ROTATE_MODE_NONE; /*旋转类型*/
    void *pXspHandle = NULL;             /*算法通道*/
    XSP_VERTICAL_MIRROR_PARAM *pstMirrorPrm = NULL;
    CAPB_XRAY_IN *pCapbXrayin = capb_get_xrayin();

    /*参数校验*/
    if (NULL == pCapbXrayin)
    {
        XSP_LOGE("pCapbXrayin %p is error.\n", pCapbXrayin);
        return SAL_FAIL;
    }

    /*通道校验*/
    if (u32Chn >= pCapbXrayin->xray_in_chan_cnt)
    {
        XSP_LOGE("u32Chn %d is error, xray_in_chan_cnt is %d.\n",
                 u32Chn,
                 pCapbXrayin->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    /*方向判断*/
    if ((XRAY_DIRECTION_RIGHT != u32Direction) && (XRAY_DIRECTION_LEFT != u32Direction))
    {
        XSP_LOGW("trans: u32Chn [%d], direction [%d] is error!\n",
                 u32Chn,
                 u32Direction);
        return SAL_FAIL;
    }

    /*获取通道句柄*/
    if (XSP_HANDLE_RT_PB == enHandleType)
    {
        pXspHandle = g_xsp_common.stXspChn[u32Chn].handle;
    }
    else if (XSP_HANDLE_TRANS == enHandleType)
    {
        pXspHandle = gstXspTransProc.pXspHandle;
    }
    else
    {
        XSP_LOGE("enHandleType %d is error.\n", enHandleType);
        return SAL_FAIL;
    }

    if (NULL == pXspHandle)
    {
        XSP_LOGE("pXspHandle %p is error.\n", pXspHandle);
        return SAL_FAIL;
    }

    pstMirrorPrm = &g_xsp_common.stUpdatePrm.stBasePrm.stMirrorPrm[u32Chn];

    /*左向: 旋转270，镜像是正常出图方式*/
    if (XRAY_DIRECTION_LEFT == u32Direction)
    {
        /*旋转操作*/
        u32Rotate = ROTATE_MODE_270;

        /*镜像操作：当前是开启上下镜像，则设置镜像NONE*/
        /*当前是关闭上下镜像，则设置垂直镜像*/
        u32Mirror = (SAL_TRUE == pstMirrorPrm->bEnable) ? MIRROR_MODE_NONE : MIRROR_MODE_VERTICAL;
    }
    /*右向: 旋转90，无镜像是正常出图方式*/
    else if (XRAY_DIRECTION_RIGHT == u32Direction)
    {
        /*旋转操作*/
        u32Rotate = ROTATE_MODE_90;

        /*镜像操作：当前是开启上下镜像，则设置垂直镜像*/
        /*当前是关闭上下镜像，则设置镜像NONE*/
        u32Mirror = (SAL_TRUE == pstMirrorPrm->bEnable) ? MIRROR_MODE_VERTICAL : MIRROR_MODE_NONE;
    }

    s32Ret = g_xsp_lib_api.set_rotate(pXspHandle, u32Rotate, enType);
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("set_rotate[%d] enType[%d] failed!\n", u32Rotate, enType);
        return SAL_FAIL;
    }

    s32Ret = g_xsp_lib_api.set_mirror(pXspHandle, u32Mirror);
    if (SAL_SOK != s32Ret)
    {
        XSP_LOGE("set_mirror[%d] failed!\n", u32Mirror);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      xsp_normalize_idt_results
 * @brief   将算法输出的难穿透&可疑有机物识别坐标转换为SVA使用的归一化坐标
 *
 * @param   [OUT] pstIdtResult 归一化坐标
 * @param   [IN] pstNodeOut 算法输出的基于包裹RAW数据的识别坐标
 */
static void xsp_normalize_idt_results(XSP_RESULT_DATA *pstIdtResult, XSP_PIDT_NODE_OUT *pstNodeOut)
{
    UINT32 i = 0;
    UINT32 u32YuvWidth = 0, u32YuvHeight = 0;           /*包裹YUV的宽度*/
    ROTATE_MODE enRotate = ROTATE_MODE_NONE;
    BOOL bVMirror = SAL_FALSE;
    XSP_RECT astSusOrgRect[XSP_IDENTIFY_NUM] = {0};     /* 可疑有机物矩形框 */
    XSP_RECT astUnpenRect[XSP_IDENTIFY_NUM] = {0};      /* 难穿透矩形框 */

    if (NULL == pstIdtResult || NULL == pstNodeOut)
    {
        XSP_LOGE("pstIdtResult(%p) OR pstNodeOut(%p) is NULL\n", pstIdtResult, pstNodeOut);
        return;
    }

    u32YuvWidth = pstNodeOut->u32NrawHeight;
    u32YuvHeight = pstNodeOut->u32NrawWidth;

    if (XRAY_DIRECTION_LEFT == pstNodeOut->enDir)
    {
        enRotate = ROTATE_MODE_270; /* R2L旋转270° */
        bVMirror = pstNodeOut->bVMirror ? SAL_FALSE : SAL_TRUE; /* R2L本身有上下镜像 */
    }
    else
    {
        enRotate = ROTATE_MODE_90; /* L2R旋转90° */
        bVMirror = pstNodeOut->bVMirror ? SAL_TRUE : SAL_FALSE;
    }

    /* 旋转矩形坐标转换 */
    for (i = 0; i < pstNodeOut->stIdtSusOrg.num; i++) /* 可疑有机物区域 */
    {
        convert_rect_region_rotate(pstNodeOut->u32NrawWidth, pstNodeOut->u32NrawHeight, enRotate, &pstNodeOut->stIdtSusOrg.rect[i], &astSusOrgRect[i]);
    }

    for (i = 0; i < pstNodeOut->stIdtUnpen.num; i++) /* 难穿透区域 */
    {
        convert_rect_region_rotate(pstNodeOut->u32NrawWidth, pstNodeOut->u32NrawHeight, enRotate, &pstNodeOut->stIdtUnpen.rect[i], &astUnpenRect[i]);
    }

    /* 镜像矩形坐标转换 */
    if (bVMirror) /* 旋转后，矩形区域坐标已经是基于YUV图像的了 */
    {
        for (i = 0; i < pstNodeOut->stIdtSusOrg.num; i++) /* 可疑有机物区域 */
        {
            convert_rect_region_mirror(u32YuvWidth, u32YuvHeight, MIRROR_MODE_VERTICAL, &astSusOrgRect[i], &astSusOrgRect[i]);
        }

        for (i = 0; i < pstNodeOut->stIdtUnpen.num; i++) /* 难穿透区域 */
        {
            convert_rect_region_mirror(u32YuvWidth, u32YuvHeight, MIRROR_MODE_VERTICAL, &astUnpenRect[i], &astUnpenRect[i]);
        }
    }

    /* 可疑有机物矩形坐标转归一化 */
    pstIdtResult->stZeffSent.uiNum = SAL_MIN(pstNodeOut->stIdtSusOrg.num, XSP_IDENTIFY_NUM);
    pstIdtResult->stZeffSent.type = ISM_ORGNAIC_TYPE;
    pstIdtResult->stZeffSent.uiResult = g_xsp_common.stUpdatePrm.stBasePrm.stSusAlert.uiColor;
    for (i = 0; i < pstIdtResult->stZeffSent.uiNum; i++)
    {
        pstIdtResult->stZeffSent.stRect[i].x = (float)astSusOrgRect[i].uiX / u32YuvWidth;
        pstIdtResult->stZeffSent.stRect[i].y = (float)astSusOrgRect[i].uiY / u32YuvHeight;
        pstIdtResult->stZeffSent.stRect[i].width = (float)astSusOrgRect[i].uiWidth / u32YuvWidth;
        pstIdtResult->stZeffSent.stRect[i].height = (float)astSusOrgRect[i].uiHeight / u32YuvHeight;
    }

    /* 难穿透矩形坐标转归一化 */
    pstIdtResult->stUnpenSent.uiNum = SAL_MIN(pstNodeOut->stIdtUnpen.num, XSP_IDENTIFY_NUM);
    pstIdtResult->stUnpenSent.type = ISM_UNPEN_TYPE;
    pstIdtResult->stUnpenSent.uiResult = g_xsp_common.stUpdatePrm.stBasePrm.stUnpenSet.uiColor;
    for (i = 0; i < pstIdtResult->stUnpenSent.uiNum; i++)
    {
        pstIdtResult->stUnpenSent.stRect[i].x = (float)astUnpenRect[i].uiX / u32YuvWidth;
        pstIdtResult->stUnpenSent.stRect[i].y = (float)astUnpenRect[i].uiY / u32YuvHeight;
        pstIdtResult->stUnpenSent.stRect[i].width = (float)astUnpenRect[i].uiWidth / u32YuvWidth;
        pstIdtResult->stUnpenSent.stRect[i].height = (float)astUnpenRect[i].uiHeight / u32YuvHeight;
    }

    return;
}


/**
 * @fn      Xsp_DrvPackageIdentifyIsRunning
 * @brief   XSP难穿透&可疑有机物识别是否开启
 * 
 * @param   [IN] u32Chan XRay通道号，该参数暂不使用
 * 
 * @return  BOOL SAL_TRUE：开启，SAL_FALSE：关闭
 */
BOOL Xsp_DrvPackageIdentifyIsRunning(UINT32 u32Chan)
{
    return (g_xsp_common.stUpdatePrm.stBasePrm.stSusAlert.uiEnable || g_xsp_common.stUpdatePrm.stBasePrm.stUnpenSet.uiEnable);
}


/**
 * @fn      Xsp_DrvPackageIdentifySend
 * @brief   启动XSP难穿透&可疑有机物识别，发送识别数据，XSP将此次请求加入到FIFO队列尾
 *
 * @param   [IN] u32Chan 通道号，主视角为0，辅视角为1
 * @param   [IN] pstPackPrm 需要识别包裹NRAW数据Buffer
 * @param   [IN] pstXPackPriv 需要识别包裹的相关信息（透传）
 * @param   [IN] u32Timeout 超时时间，若识别性能不足，超过该时间直接返回，单位ms
 *
 * @return  SAL_STATUS SAL_SOK：启动成功，SAL_ERROR：启动失败
 */
SAL_STATUS Xsp_DrvPackageIdentifySend(UINT32 u32Chan, XIMAGE_DATA_ST *pstPackData, SAL_SYSFRAME_PRIVATE *pstXPackPriv, UINT32 u32Timeout)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSA_NODE *pNode = NULL;
    UINT64 ts = 0, te = 0, tl = u32Timeout;
    XSP_PACKAGE_IDENTIFY *pstPackIdt = NULL;
    XSP_PIDT_NODE_IN *pstNodeIn = NULL; /* 输入节点 */
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    CAPB_XSP *pstCapbXsp = capb_get_xsp();

    /*================== 输入参数校验 ==================*/
    /* 通道号校验 */
    if (u32Chan >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("xsp chan %u is invalid, range: [0, %d)\n", u32Chan, pstCapbXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    if (NULL == pstPackData || NULL == pstXPackPriv)
    {
        XSP_LOGE("xsp chan %u, pstPackData(%p) OR pstXPackPriv(%p) is NULL\n", u32Chan, pstPackData, pstXPackPriv);
        return SAL_FAIL;
    }

    if (pstPackData->u32Width != pstCapbXrayIn->xraw_width_max)
    {
        XSP_LOGE("xsp chan %u, the w(%u) is invalid, should be: %u\n", u32Chan,
                 pstPackData->u32Width, pstCapbXrayIn->xraw_width_max);
        return SAL_FAIL;
    }
    if (pstPackData->u32Height == 0 || pstPackData->u32Height > pstCapbXsp->xsp_package_line_max)
    {
        XSP_LOGE("xsp chan %u, the h(%u) is invalid, range: (0, %u]\n",
                 u32Chan, pstPackData->u32Height, pstCapbXsp->xsp_package_line_max);
        return SAL_FAIL;
    }
    if ((NULL == pstPackData->pPlaneVir[0]) || 
        (pstPackData->enImgFmt == DSP_IMG_DATFMT_LHZP && (NULL == pstPackData->pPlaneVir[1] || NULL == pstPackData->pPlaneVir[2])))
    {
        XSP_LOGE("xsp chan %d, ImgFmt: 0x%x, NULL virAddr: %p %p %p\n", u32Chan, pstPackData->enImgFmt, pstPackData->pPlaneVir[0], 
                 pstPackData->pPlaneVir[1], pstPackData->pPlaneVir[2]);
        return SAL_FAIL;
    }

    /*================== 拷贝识别的NRAW数据，加入队列 ==================*/
    pstPackIdt = &g_xsp_common.stXspChn[u32Chan].stPackIdt;
    SAL_mutexTmLock(&pstPackIdt->lstIn->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    /* 获取队列lstIn的空闲结点 */
    while (NULL == (pNode = DSA_LstGetIdleNode(pstPackIdt->lstIn)))
    {
        if (tl + ts > te)
        {
            tl = tl + ts - te;
            ts = sal_get_tickcnt();
            SAL_CondWait(&pstPackIdt->lstIn->sync, tl, __FUNCTION__, __LINE__);
            te = sal_get_tickcnt();
        }
        else
        {
            break; /* 超时退出 */
        }
    }

    if (NULL != pNode)
    {
        pstNodeIn = (XSP_PIDT_NODE_IN *)pNode->pAdData;

        /* 基本信息赋值 */
        memcpy(&pstNodeIn->stPackPrm, pstPackData, sizeof(XIMAGE_DATA_ST));
        memcpy(&pstNodeIn->stXPackPriv, pstXPackPriv, sizeof(SAL_SYSFRAME_PRIVATE));
        pstNodeIn->u64TimeReq = sal_get_tickcnt();
        pstNodeIn->u64TimeStart = 0;
        pstNodeIn->u64TimeEnd = 0;

        DSA_LstPush(pstPackIdt->lstIn, pNode);
        SAL_CondSignal(&pstPackIdt->lstIn->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        sRet = SAL_SOK;
    }
    SAL_mutexTmUnlock(&pstPackIdt->lstIn->sync.mid, __FUNCTION__, __LINE__);

    return sRet;
}


static SAL_STATUS Xsp_DrvPackageIdentifyPutResults(UINT32 u32Chan, DSA_LIST *lstOut, UINT32 u32NrawWidth, UINT32 u32NrawHeight,
                                                   XSP_IDENTIFY_S **aXspIdtResult, SAL_SYSFRAME_PRIVATE *pstXPackPriv, XSP_DEBUG_IDENTIFY *pstDbgIdt)
{
    SAL_STATUS sRet = SAL_SOK;
    DSA_NODE *pNode = NULL;
    XSP_IDENTIFY_S *pstIdtSusOrg = NULL;
    XSP_IDENTIFY_S *pstIdtUnpen = NULL;
    XSP_IDENTIFY_S *pstIdtExplosive = NULL;
    XSP_PIDT_NODE_OUT *pstNodeOut = NULL; /* 识别结果输出节点 */

    XSP_CHECK_PTR_IS_NULL(lstOut, SAL_FALSE);
    XSP_CHECK_PTR_IS_NULL(aXspIdtResult, SAL_FALSE);
    XSP_CHECK_PTR_IS_NULL(pstXPackPriv, SAL_FALSE);

    if (NULL != aXspIdtResult[XSP_ALERT_TYPE_ORG])
    {
        pstIdtSusOrg = aXspIdtResult[XSP_ALERT_TYPE_ORG];
    }

    if (NULL != aXspIdtResult[XSP_ALERT_TYPE_UNPEN])
    {
        pstIdtUnpen = aXspIdtResult[XSP_ALERT_TYPE_UNPEN];
    }

    if (NULL != aXspIdtResult[XSP_ALERT_TYPE_BOMB])
    {
        pstIdtExplosive = aXspIdtResult[XSP_ALERT_TYPE_BOMB];
    }

    SAL_mutexTmLock(&lstOut->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    if (DSA_LstIsFull(lstOut)) /* 若队列满，则等待100ms */
    {
        SAL_CondWait(&lstOut->sync, 100, __FUNCTION__, __LINE__);
    }

    pNode = DSA_LstGetIdleNode(lstOut);
    if (NULL != pNode)
    {
        pstNodeOut = (XSP_PIDT_NODE_OUT *)pNode->pAdData;
        pstNodeOut->u32NrawWidth = u32NrawWidth;
        pstNodeOut->u32NrawHeight = u32NrawHeight;
        pstNodeOut->enDir = g_xsp_common.stXspChn[u32Chan].stNormalData.uiRtDirection;
        pstNodeOut->bVMirror = g_xsp_common.stUpdatePrm.stBasePrm.stMirrorPrm[u32Chan].bEnable;
        pstNodeOut->pstDbgIdt = pstDbgIdt;
        memcpy(&pstNodeOut->stIdtSusOrg, pstIdtSusOrg, sizeof(XSP_IDENTIFY_S));
        memcpy(&pstNodeOut->stIdtUnpen, pstIdtUnpen, sizeof(XSP_IDENTIFY_S));
        memcpy(&pstNodeOut->stIdtExplosive, pstIdtExplosive, sizeof(XSP_IDENTIFY_S));
        memcpy(&pstNodeOut->stXPackPriv, pstXPackPriv, sizeof(SAL_SYSFRAME_PRIVATE));
        DSA_LstPush(lstOut, pNode);
        SAL_CondSignal(&lstOut->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        sRet = SAL_SOK;
    }
    else
    {
        sRet = SAL_FAIL;
    }

    SAL_mutexTmUnlock(&lstOut->sync.mid, __FUNCTION__, __LINE__);

    return sRet;
}


/**
 * @fn      Xsp_DrvPackageIdentifyGetResultsOnPack
 * @brief   获取XSP难穿透&可疑有机物识别结果，获取FIFO队列头指向基于包裹的识别结果
 *
 * @param   [IN] u32Chan 通道号，主视角为0，辅视角为1
 * @param   [OUT] pstIdtResults 基于包裹的识别区域信息
 * @param   [OUT] pstXPackPriv 回传识别包裹的一些透传信息
 * @param   [IN] u32Timeout 超时时间，若识别性能不足，超过该时间直接返回，单位ms
 *
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_ERROR：获取失败
 */
SAL_STATUS Xsp_DrvPackageIdentifyGetResults(UINT32 u32Chan, XSP_RESULT_DATA *pstIdtResults, SAL_SYSFRAME_PRIVATE *pstXPackPriv, UINT32 u32Timeout)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSA_NODE *pNode = NULL;
    UINT64 ts = 0, te = 0, tl = u32Timeout;
    DSA_LIST *lstOut = NULL;
    XSP_PIDT_NODE_OUT *pstNodeOut = NULL; /* 识别结果输出节点 */
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();

    /*================== 输入参数校验 ==================*/
    /* 通道号校验 */
    if (u32Chan >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %u is invalid, range: [0, %d)\n", u32Chan, pstCapbXrayIn->xray_in_chan_cnt);
        return SAL_FAIL;
    }

    if (NULL == pstXPackPriv || NULL == pstIdtResults)
    {
        XSP_LOGE("chan %u, pstXPackPriv(%p) OR pstIdtResults(%p) is NULL\n", u32Chan, pstXPackPriv, pstIdtResults);
        return SAL_FAIL;
    }

    lstOut = g_xsp_common.stXspChn[u32Chan].stPackIdt.lstOutPack;
    SAL_mutexTmLock(&lstOut->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    /* 获取队列lstOut的头结点 */
    while (NULL == (pNode = DSA_LstGetHead(lstOut)))
    {
        if (tl + ts > te)
        {
            tl = tl + ts - te;
            ts = sal_get_tickcnt();
            SAL_CondWait(&lstOut->sync, tl, __FUNCTION__, __LINE__);
            te = sal_get_tickcnt();
        }
        else
        {
            break; /* 超时退出 */
        }
    }

    if (NULL != pNode)
    {
        /* 坐标转换并输出 */
        pstNodeOut = (XSP_PIDT_NODE_OUT *)pNode->pAdData;
        xsp_normalize_idt_results(pstIdtResults, pstNodeOut);
        memcpy(pstXPackPriv, &pstNodeOut->stXPackPriv, sizeof(SAL_SYSFRAME_PRIVATE));
        pstNodeOut->pstDbgIdt->u64TimeIdtRstPackGet = sal_get_tickcnt();
        DSA_LstDelete(lstOut, pNode);
        SAL_CondSignal(&lstOut->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        sRet = SAL_SOK;
    }
    SAL_mutexTmUnlock(&lstOut->sync.mid, __FUNCTION__, __LINE__);

    return sRet;
}


static SAL_STATUS xsp_package_identify_init(XSP_PACKAGE_IDENTIFY *pstPackIdt)
{
    UINT32 i = 0;
    DSA_NODE *pNode = NULL;

    if (NULL == pstPackIdt)
    {
        SAL_LOGE("the 'pstPackIdt' is NULL\n");
        return SAL_FAIL;
    }

    /* 输入队列初始化 */
    pstPackIdt->lstIn = DSA_LstInit(XSP_PIDT_QUEUE_LEN);
    if (NULL == pstPackIdt->lstIn)
    {
        XSP_LOGE("init 'lstIn' failed\n");
        return SAL_FAIL;
    }

    SAL_mutexTmLock(&pstPackIdt->lstIn->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < XSP_PIDT_QUEUE_LEN; i++)
    {
        if (NULL != (pNode = DSA_LstGetIdleNode(pstPackIdt->lstIn)))
        {
            pNode->pAdData = pstPackIdt->stNodeIn + i; /* 绑定队列节点与数据 */
            DSA_LstPush(pstPackIdt->lstIn, pNode);
        }
    }

    for (i = 0; i < XSP_PIDT_QUEUE_LEN; i++)
    {
        DSA_LstPop(pstPackIdt->lstIn);
    }

    SAL_mutexTmUnlock(&pstPackIdt->lstIn->sync.mid, __FUNCTION__, __LINE__);

    /* 基于包裹的识别结果输出队列初始化 */
    pstPackIdt->lstOutPack = DSA_LstInit(XSP_PIDT_QUEUE_LEN);
    if (NULL == pstPackIdt->lstOutPack)
    {
        XSP_LOGE("init 'lstOutPack' failed\n");
        return SAL_FAIL;
    }

    SAL_mutexTmLock(&pstPackIdt->lstOutPack->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < XSP_PIDT_QUEUE_LEN; i++)
    {
        if (NULL != (pNode = DSA_LstGetIdleNode(pstPackIdt->lstOutPack)))
        {
            pNode->pAdData = pstPackIdt->stNodeOutPack + i; /* 绑定队列节点与数据 */
            DSA_LstPush(pstPackIdt->lstOutPack, pNode);
        }
    }

    for (i = 0; i < XSP_PIDT_QUEUE_LEN; i++)
    {
        DSA_LstPop(pstPackIdt->lstOutPack);
    }

    SAL_mutexTmUnlock(&pstPackIdt->lstOutPack->sync.mid, __FUNCTION__, __LINE__);

    return SAL_SOK;
}


/**
 * @fn      Xsp_DrvPackageIdentifyThread
 * @brief   难穿透&可疑有机物识别线程，循环执行永不退出
 *
 * @param   [IN] args XSP通道参数，结构体XSP_CHN_PRM
 *
 * @return  void* 无
 */
void *Xsp_DrvPackageIdentifyThread(void *args)
{
    SAL_STATUS sRet = SAL_FAIL; /*函数返回值*/
    UINT32 u32Chn = 0; /*通道号*/
    CHAR sTaskName[SAL_THR_NAME_LEN_MAX] = {0};
    DSA_NODE *pNode = NULL;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();
    XSP_CHN_PRM *pstXspChnPrm = (XSP_CHN_PRM *)args;
    XSP_PACKAGE_IDENTIFY *pstPackIdt = NULL;
    XSP_PIDT_NODE_IN *pstNodeIn = NULL; /* 输入节点 */
    XSP_DEBUG_IDENTIFY *pstDbgIdt = NULL;
    XSP_IDT_IN *pstIdtIn = NULL;         /* 输入XSP算法的NRAW数据 */
    XSP_IDENTIFY_S *aXspIdtResult[XSP_ALERT_TYPE_NUM] = {NULL};  /* XSP算法识别结果, 可扩充 */
    XSP_IDENTIFY_S stIdtSusOrg = {0};           /* 可疑有机物 */
    XSP_IDENTIFY_S stIdtUnpen = {0};            /* 难穿透 */
    XSP_IDENTIFY_S stIdtExplosive = {0};        /* 疑似爆炸物 */
    SAL_SYSFRAME_PRIVATE stXPackPriv = {0};   /* XPack的私有信息 */

    aXspIdtResult[XSP_ALERT_TYPE_ORG] = &stIdtSusOrg;
    aXspIdtResult[XSP_ALERT_TYPE_UNPEN] = &stIdtUnpen;
    aXspIdtResult[XSP_ALERT_TYPE_BOMB] = &stIdtExplosive;

    if (NULL == args) /*参数校验*/
    {
        XSP_LOGE("the args is NULL\n");
        return NULL;
    }

    u32Chn = pstXspChnPrm->u32Chn;
    if (u32Chn >= pstCapbXrayIn->xray_in_chan_cnt)
    {
        XSP_LOGE("chan %u is invalid, range: [0, %d)\n", u32Chn, pstCapbXrayIn->xray_in_chan_cnt);
        return NULL;
    }

    /* 修改系统默认线程名称 */
    snprintf(sTaskName, sizeof(sTaskName), "xsp_idt-%u", u32Chn);
    prctl(PR_SET_NAME, (unsigned long)sTaskName);

    /* 绑核处理，通道0使用逻辑核2，通道1使用逻辑核3 */
    /* SAL_SetThreadCoreBind((u32Chn % 2) + 2); */

    pstPackIdt = &pstXspChnPrm->stPackIdt;
    if (SAL_SOK != xsp_package_identify_init(pstPackIdt))
    {
        return NULL;
    }

    while (1)
    {
        /* 等待包裹NRAW数据输入 */
        SAL_mutexTmLock(&pstPackIdt->lstIn->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (NULL == (pNode = DSA_LstGetHead(pstPackIdt->lstIn)))
        {
            SAL_CondWait(&pstPackIdt->lstIn->sync, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }

        pstNodeIn = (XSP_PIDT_NODE_IN *)pNode->pAdData;
        SAL_mutexTmUnlock(&pstPackIdt->lstIn->sync.mid, __FUNCTION__, __LINE__);

        /* 难穿透&可疑有机物识别 */
        pstIdtIn = &pstNodeIn->stPackPrm;

        stIdtSusOrg.num = 0;         /* 重置可疑有机物区域数量 */
        stIdtUnpen.num = 0;          /* 重置难穿透区域数量 */
        stIdtExplosive.num = 0;      /* 重置爆炸物区域数量 */
        memcpy(&stXPackPriv, &pstNodeIn->stXPackPriv, sizeof(SAL_SYSFRAME_PRIVATE)); /* 备份XPack透传信息 */
        pstNodeIn->u64TimeStart = sal_get_tickcnt(); /* 统计难穿透&可疑有机物识别耗时，开始时间 */

        if (0 < pstXspChnPrm->stDumpCfg.u32DumpCnt && pstXspChnPrm->stDumpCfg.u32DumpDp & XSP_DDP_IDT_IN)
        {
            ximg_dump(pstIdtIn, u32Chn, pstXspChnPrm->stDumpCfg.chDumpDir, "idt-rawin", NULL, pstXspChnPrm->stDumpCfg.u32DumpCnt);
            pstXspChnPrm->stDumpCfg.u32DumpCnt--;
        }

        sRet = g_xsp_lib_api.identify_process(pstXspChnPrm->handle, pstIdtIn, aXspIdtResult);
        if (SAL_SOK != sRet)
        {
            /* 处理失败，仍然将结果放入输出队列中，而不是直接continue */
            XSP_LOGE("chan %u, identify_process failed\n", u32Chn);
        }
        pstNodeIn->u64TimeEnd = sal_get_tickcnt(); /* 统计难穿透&可疑有机物识别耗时，结束时间 */

        // xpack_package_release_xridt(u32Chn, &stXPackPriv); 因包裹NRaw数据需要回调给应用存储，因此屏蔽这里的release接口，统一在回调后再release
        pstDbgIdt = &pstXspChnPrm->stDbgIdentify[pstXspChnPrm->u32DbgIdtIdx];
        pstXspChnPrm->u32DbgIdtIdx = (pstXspChnPrm->u32DbgIdtIdx + 1) % XSP_DEBUG_IDENTIFY_NUM;
        memset(pstDbgIdt, 0, sizeof(XSP_DEBUG_IDENTIFY));
        pstDbgIdt->u64TimeIdentifyReq = pstNodeIn->u64TimeReq;
        pstDbgIdt->u64TimeIdentifyStart = pstNodeIn->u64TimeStart;
        pstDbgIdt->u64TimeIdentifyEnd = pstNodeIn->u64TimeEnd;
        pstDbgIdt->s32PackId = pstNodeIn->stXPackPriv.u32FrameNum;
        pstDbgIdt->u32Width = pstIdtIn->u32Width;
        pstDbgIdt->u32Height = pstIdtIn->u32Height;
        memcpy(&pstDbgIdt->stIdtSusOrg, &stIdtSusOrg, sizeof(XSP_IDENTIFY_S));
        memcpy(&pstDbgIdt->stIdtUnpen, &stIdtUnpen, sizeof(XSP_IDENTIFY_S));
        memcpy(&pstDbgIdt->stIdtExplosive, &stIdtExplosive, sizeof(XSP_IDENTIFY_S));

        /* 删除输入队列头结点 */
        SAL_mutexTmLock(&pstPackIdt->lstIn->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        DSA_LstPop(pstPackIdt->lstIn);
        SAL_CondSignal(&pstPackIdt->lstIn->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        SAL_mutexTmUnlock(&pstPackIdt->lstIn->sync.mid, __FUNCTION__, __LINE__);

        /* 将识别结果放入基于包裹的识别结果队列 */
        sRet = Xsp_DrvPackageIdentifyPutResults(u32Chn, pstPackIdt->lstOutPack, pstIdtIn->u32Width, pstIdtIn->u32Height,
                                                aXspIdtResult, &stXPackPriv, pstDbgIdt);
        if (SAL_FAIL == sRet)
        {
            XSP_LOGW("chan %u, put results to 'lstOutPack' failed, count: %u\n", u32Chn, DSA_LstGetCount(pstPackIdt->lstOutPack));
        }
    }

    return NULL;
}