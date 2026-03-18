/**
 * @file   fb_hal.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  显示图形层平台抽象HAL层接口
 * @author liuxianying
 * @date   2021/8/5
 * @note
 * @note \n History
   1.日    期: 2021/8/5
     作    者: liuxianying
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <sal.h>
#include <dspcommon.h>
#include <platform_hal.h>
#include <platform_sdk.h>
#include "hal_inc_inter/fb_hal_inter.h"



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
/* SCREEN 个数先固定3个后续修改为按照配置申请   */
#define FB_DEV_NUM          (3)


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */



/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */
FB_COMMON g_fb_common[FB_DEV_NUM];
static FB_PLAT_OPS *pfbPaltOps = NULL;

/**
 * @function   fb_hal_fbCopy
 * @brief      显存内容拷贝
 * @param[in]  UINT32 uiChn       图形层号
 * @param[in]  FB_COMMON *pfbChn  图形层信息
 * @param[out] None
 * @return     INT32  SAL_SOK   成功
 *                    SAL_FAIL  失败
 */
static INT32 fb_hal_fbCopy(UINT32 uiChn, FB_COMMON *pfbChn)
{

    INT32                    s32Ret      = 0;
    TDE_HAL_SURFACE          srcSurface, dstSurface;
    TDE_HAL_RECT             srcRect, dstRect;
    static UINT32            cnt[FB_DEV_NUM_MAX] = {0};
    FB_DUMP_CFG *pstFbDump = NULL;
    CHAR dumpName[128] = {0};

    if(NULL == pfbChn)
    {
        return SAL_FAIL;
    }

    memset(&srcSurface, 0, sizeof(TDE_HAL_SURFACE));
    memset(&dstSurface, 0, sizeof(TDE_HAL_SURFACE));
    memset(&srcRect, 0, sizeof(TDE_HAL_RECT));
    memset(&dstRect, 0, sizeof(TDE_HAL_RECT));

    srcSurface.u32Width   = pfbChn->fb_app.uiWidth;
    srcSurface.u32Height  = pfbChn->fb_app.uiHeight;
    srcSurface.u32Stride  = pfbChn->u32BytePerPixel * pfbChn->fb_app.uiWidth;
    srcSurface.PhyAddr =    pfbChn->fb_app.ui64Phyaddr;
	srcSurface.vbBlk = pfbChn->fb_app.vbBlk;             /* todo: 此处blk需要从平台层获取 */
    switch (pfbChn->bitWidth)
    {
        case FB_DATA_BITWIDTH_32:
        {
            srcSurface.enColorFmt = SAL_VIDEO_DATFMT_ARGB_8888;
            dstSurface.enColorFmt = SAL_VIDEO_DATFMT_ARGB_8888;
            break;
        }
        case FB_DATA_BITWIDTH_16:
        {
            srcSurface.enColorFmt = SAL_VIDEO_DATFMT_ARGB16_1555;
            dstSurface.enColorFmt = SAL_VIDEO_DATFMT_ARGB16_1555;
            break;
        }
        default:
        {
            srcSurface.enColorFmt = SAL_VIDEO_DATFMT_ARGB_8888;
            dstSurface.enColorFmt = SAL_VIDEO_DATFMT_ARGB_8888;
            break;
        }
    }

    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = pfbChn->fb_app.uiWidth;
    srcRect.u32Height = pfbChn->fb_app.uiHeight;

    dstSurface.u32Width   = pfbChn->fb_dev.uiWidth;
    dstSurface.u32Height  = pfbChn->fb_dev.uiHeight;
    dstSurface.u32Stride  = pfbChn->fb_dev.uiWidth * pfbChn->u32BytePerPixel;
    dstSurface.vbBlk      = pfbChn->fb_dev.vbBlk;
    if(FB_DEV_G3 == uiChn)
    {
        dstSurface.PhyAddr = pfbChn->fb_dev.ui64CanvasAddr;
    }
    else
    {
        if (cnt[uiChn] >= FB_BUFF_CNT)
        {
           cnt[uiChn] = 0;
        }

		/* RK平台除了CMA内存，其他不使用物理地址；对RK里说这里起始没啥意义 */
        dstSurface.PhyAddr = pfbChn->fb_dev.ui64CanvasAddr + (cnt[uiChn] * pfbChn->fb_dev.offset);
        cnt[uiChn] ++;
        //mem_hal_mmzFlushCache(srcSurface.vbBlk); //RK底层vb内存申请时标志位已经从NO_MAP改为NO_CACHE，RK里厂商反馈NO_MAP起始是会映射成带cache的

    }

    dstRect.s32Xpos   = 0;
    dstRect.s32Ypos   = 0;
    dstRect.u32Width  = pfbChn->fb_dev.uiWidth;
    dstRect.u32Height = pfbChn->fb_dev.uiHeight;
    
    s32Ret = tde_hal_QuickResize(&srcSurface, &srcRect, &dstSurface, &dstRect);

    if (SAL_SOK == s32Ret)
    {
        pstFbDump = &g_fb_common[uiChn].stDumpCfg;
        if (pstFbDump->u32DumpCnt > 0)
        {
            snprintf(dumpName, sizeof(dumpName), "%s/fb_ch%u_t%llu_w%u_h%u_s%u.bin",
                     pstFbDump->sDumpDir, uiChn, sal_get_tickcnt(), srcSurface.u32Width, srcSurface.u32Height,srcSurface.u32Stride);
            SAL_WriteToFile(dumpName, 0, SEEK_SET, g_fb_common[uiChn].fb_app.Viraddr, srcSurface.u32Stride * srcSurface.u32Height);
            pstFbDump->u32DumpCnt--;
        }
        return SAL_SOK;
    }
    else
    {
        SAL_LOGE("err %#x\n", s32Ret);
        return SAL_FAIL;
    }
}


/**
 * @function   fb_hal_setMouseFbChn
 * @brief      设置鼠标层对用FB通道的绑定关系
 * @param[in]  UINT32 uiBindChn  对应绑定的设备号
 * @param[out] None
 * @return     INT32  SAL_SOK    成功
 *                    SAL_FAIL   失败
 */
INT32 fb_hal_setMouseFbChn(UINT32 uiBindChn)
{
    INT32 s32Ret = 0;
    UINT32 uiChn = 0;
    FB_COMMON *pfb_chn = &g_fb_common[uiBindChn]; 

    if(NULL == pfbPaltOps)
    {
        return SAL_FAIL;
    }

    s32Ret  = pfbPaltOps->setFbChn(uiChn, uiBindChn, pfb_chn);

    if (SAL_SOK != s32Ret)
    {
        SAL_WARN("fb_hal_setMouseFbChn uiChn = %u failed !\n", uiChn);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   fb_hal_refreshFb
 * @brief      刷新FB显示内容
 * @param[in]  UINT32 uiChn              设备号 
 * @param[in]  SCREEN_CTRL *pstHalFbAttr 刷新区域
 * @param[out] None
 * @return     INT32  SAL_SOK   成功
 *                    SAL_FAIL  失败
 */
INT32 fb_hal_refreshFb(UINT32 uiChn, SCREEN_CTRL *pstHalFbAttr)
{

    INT32 s32Ret = SAL_FAIL;
    SCREEN_CTRL *pstFbAttr = pstHalFbAttr;
    FB_COMMON *pfbChn = &g_fb_common[uiChn];

    if(NULL == pfbPaltOps || NULL == pfbPaltOps->refreshFb)
    {
        SAL_ERROR("pstHalFbAttr is err!\n");
        return SAL_FAIL;
    }
    
    SAL_mutexLock(pfbChn->mutex);

    /* 备注：此处流程与R8有差异,海思平台的话，缺少了刷新鼠标位置步骤,存在鼠标没有更新位置的 */
    if (uiChn < 2)
    {
        s32Ret = fb_hal_fbCopy(uiChn, pfbChn);
        if(SAL_SOK != s32Ret)
        {
            SAL_WARN("fb_hal_refreshFb %u failed !\n",uiChn);
            SAL_mutexUnlock(pfbChn->mutex);
            return SAL_FAIL;
        }
    }

    s32Ret = pfbPaltOps->refreshFb(uiChn, pfbChn, pstFbAttr);
    if(SAL_SOK != s32Ret)
    {
        SAL_WARN("fb_hal_refreshFb %u failed !\n",uiChn);
        SAL_mutexUnlock(pfbChn->mutex);
        return SAL_FAIL;
    }
    
    SAL_mutexUnlock(pfbChn->mutex);

    return SAL_SOK;
}


/**
 * @function   fb_hal_setPos
 * @brief      设置位置
 * @param[in]  UINT32 uiChn     设备号 
 * @param[in]  POS_ATTR *pstPos 位置信息
 * @param[out] None
 * @return     INT32  SAL_SOK   成功
 *                    SAL_FAIL  失败
 */
INT32 fb_hal_setPos(UINT32 uiChn, POS_ATTR *pstPos)
{
    INT32 s32Ret = SAL_FAIL;
    FB_COMMON *pfbChn = &g_fb_common[uiChn];

    s32Ret = pfbPaltOps->setOrgin(uiChn, pfbChn, pstPos);
    if(SAL_SOK != s32Ret)
    {
        SAL_WARN("fb_hal_refreshFb %u failed !\n",uiChn);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   fb_hal_snapFb
 * @brief      抓取FB显示数据
 * @param[in]  UINT32  uiChn  Fb设备号
 * @param[out] None
 * @return     INT32  SAL_SOK   成功
 *                    SAL_FAIL  失败
 */
INT32 fb_hal_snapFb(UINT32 uiChn)
{
    return SAL_SOK;
}


/**
 * @function   fb_hal_releaseFb
 * @brief      释放fb缓存
 * @param[in]  UINT32 uiChn     Fb设备号
 * @param[out] None
 * @return     INT32  SAL_SOK   成功
 *                    SAL_FAIL  失败
 */
INT32 fb_hal_releaseFb(UINT32 uiChn)
{
    INT32 s32Ret = 0;
    FB_COMMON *pfb_chn = &g_fb_common[uiChn]; 

    if(NULL == pfbPaltOps)
    {
        return SAL_FAIL;
    }

    s32Ret = pfbPaltOps->releaseFb(uiChn, pfb_chn);
    if ( SAL_SOK != s32Ret)
    {
        SAL_WARN("App Fb %d Already umap !\n",uiChn);
        return SAL_FAIL;
    }
        
    return SAL_SOK;
}

/**
 * @function   fb_hal_getFrameBuffer
 * @brief      获取fb缓存地址和app保持一致
 * @param[in]  UINT32 uiChn               对应设备号
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  显示参数
 * @param[in]  FB_INIT_ATTR_ST *pstFbPrm  图形宽高
 * @param[out] None
 * @return     INT32  SAL_SOK   成功
 *                    SAL_FAIL  失败
 */
INT32 fb_hal_getFrameBuffer(UINT32 uiChn, SCREEN_CTRL *pstHalFbAttr,FB_INIT_ATTR_ST *pstFbPrm)
{

    INT32 s32Ret = SAL_FAIL;
    SCREEN_CTRL *pstFbAttr = pstHalFbAttr;
    FB_COMMON *pfbChn = &g_fb_common[uiChn];

    if(NULL == pstFbAttr || NULL == pfbPaltOps)
    {
        SAL_ERROR("pstHalFbAttr is err!\n");
        return SAL_FAIL;
    }

    s32Ret = pfbPaltOps->createFb(uiChn, pfbChn, pstFbAttr, pstFbPrm);
    if(SAL_SOK != s32Ret)
    {
        SAL_ERROR("fb_hal_getFrameBuffer is err!\n");
        return SAL_FAIL;
    }
    
    return SAL_SOK;
}

/**
 * @function   fb_hal_checkFbIllegal
 * @brief      校验设备号是否超出设备最大值
 * @param[in]  UINT32 chan  设备号
 * @param[out] None
 * @return     INT32
 */
INT32 fb_hal_checkFbIllegal(UINT32 chan)
{
    INT32 fbNum = 0;
    if(NULL == pfbPaltOps)
    {
        return SAL_FAIL;
    }
    
    fbNum = pfbPaltOps->getFbNum();
    if((chan + 1) > fbNum)
    {
        SAL_ERROR("Chan (Illegal parameters),chan %d Max %d (0~%d)\n",chan,(fbNum-1),(fbNum-1));
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      fb_hal_dumpSet
 * @brief   FB Dump调试
 * 
 * @param   [IN] chan 通道号
 * @param   [IN] psDumpDir Dump的路径
 * @param   [IN] u32DumpDp Dump的数据节点
 * @param   [IN] u32DumpCnt Dump次数
 */
void fb_hal_dumpSet(UINT32 chan, CHAR *psDumpDir, UINT32 u32DumpDp, UINT32 u32DumpCnt)
{
    UINT32 u32DirStrlen = 0;
    FB_COMMON *pstFbChn = NULL; 
    CHAR dumpName[128] = {0};

    if (NULL == psDumpDir || 0 == strlen(psDumpDir))
    {
        XSP_LOGE("chan %u, the psDumpDir is invalid\n", chan);
        return;
    }

    if (chan >= FB_DEV_NUM)
    {
        XSP_LOGE("the chan(%u) is invalid, range: [0, %u]\n", chan, FB_DEV_NUM-1);
        return;
    }

    pstFbChn = &g_fb_common[chan];

    /* 存储目录 */
    u32DirStrlen = sizeof(pstFbChn->stDumpCfg.sDumpDir) - 1;
    strncpy(pstFbChn->stDumpCfg.sDumpDir, psDumpDir, u32DirStrlen);
    pstFbChn->stDumpCfg.sDumpDir[u32DirStrlen] = '\0';

    /* DUMP类型 */
    pstFbChn->stDumpCfg.u32DumpDp = u32DumpDp;

    /* DUMP次数 */
    pstFbChn->stDumpCfg.u32DumpCnt = (u32DumpCnt == 0xFFFFFFFF) ? 0xFFFFFFFF : u32DumpCnt;

    if(g_fb_common[chan].fb_app.Viraddr != NULL)
    {
        snprintf(dumpName, sizeof(dumpName), "%s/fb_ch%u_t%llu_w%u_h%u_s%u_cur.bin",
                            pstFbChn->stDumpCfg.sDumpDir, chan, sal_get_tickcnt(), pstFbChn->fb_dev.uiWidth, pstFbChn->fb_dev.uiHeight, pstFbChn->fb_dev.uiWidth* 4);
        SAL_WriteToFile(dumpName, 0, SEEK_SET, g_fb_common[chan].fb_app.Viraddr, pstFbChn->fb_dev.uiWidth * pstFbChn->fb_dev.uiHeight * 4);
    }
    XSP_LOGW("set dump config successfully, DumpDir: %s, DumpDp: 0x%x, DumpCnt: %u\n",
             pstFbChn->stDumpCfg.sDumpDir, pstFbChn->stDumpCfg.u32DumpDp, pstFbChn->stDumpCfg.u32DumpCnt);

    return;
}


/**
 * @function   fb_halRegister
 * @brief      弱函数有平平台不支持该功能时保证编译通过
 * @param[in]  void  
 * @param[out] None
 * @return     __attribute__((weak)) FB_PLAT_OPS *
 
__attribute__((weak)) FB_PLAT_OPS *fb_halRegister(void)
{
    return NULL;
}
*/

/**
 * @function   fb_hal_Init
 * @brief      初始化fb接口函数
 * @param[in]  void  
 * @param[out] None
 * @return     INT32
 */
INT32 fb_hal_Init(void)
{
    UINT32 i =0;
    INT32  iFbNum = 0;
    FB_COMMON *pfb_chn = NULL;

    if (NULL != pfbPaltOps)
    {
        return SAL_SOK;
    }

    pfbPaltOps = fb_halRegister();
    if (NULL == pfbPaltOps)
    {
        return SAL_FAIL;
    }

    iFbNum = pfbPaltOps->getFbNum();
    for (i = 0;i < iFbNum; i++)
    {
        pfb_chn = &g_fb_common[i];
        pfb_chn->fd = -1;
        
        SAL_mutexCreate(SAL_MUTEX_NORMAL, &pfb_chn->mutex);
    }

    return SAL_SOK;
}



