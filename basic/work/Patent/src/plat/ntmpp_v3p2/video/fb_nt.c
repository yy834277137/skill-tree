
/*******************************************************************************
* disp_nt.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : 
* Version: V1.0.0  2021年11月18日 Create
*
* Description :
* Modification:
*******************************************************************************/

#include <sal.h>
#include <linux/fb.h>
#include "fb_hal_inter.h"
#include "disp_hal_api.h"
#include "platform_hal.h"
#include "platform_sdk.h"
#include "lcd300_fb.h"

#define FB_DEV_NUM          (3)

static struct fb_bitfield s_a32 = {24, 8, 0};
static struct fb_bitfield s_r32 = {16, 8, 0};
static struct fb_bitfield s_g32 = {8,  8, 0};
static struct fb_bitfield s_b32 = {0,  8, 0};

#if 0
static UINT32 g_au32CMap[] = {
    /* 黑色，    深红，    深绿，    深黄，    深蓝，    深紫，    深青,     深灰 */
    0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080, 0x808080,
    /* 浅灰，    红色，    绿色，    黄色，    蓝色，    紫色，    青色,     白色 */
    0xC0C0C0, 0xFF0000, 0x00FF00, 0xFFFF00, 0x0000FF, 0xFF00FF, 0x00FFFF, 0xFFFFFF,
};
#endif
static FB_PLAT_OPS g_stFbPlatOps;
static const char *g_aszUiFdName[] = {
    "/dev/fb1", "/dev/fb4",
};
static const char *g_aszMouseFdName[] = {
    "/dev/fb2", "/dev/fb5",
};

static FB_COMMON g_astMouseFb[sizeof(g_aszMouseFdName)/sizeof(g_aszMouseFdName[0])];
extern FB_COMMON g_fb_common[FB_DEV_NUM];
extern HD_PATH_ID g_aCtrlIds[DISP_HAL_MAX_DEV_NUM];
static UINT8 g_au8MouseImage[64 * 64 / 2] = {0};
static UINT16 g_au16R[16] = {0};
static UINT16 g_au16G[16] = {0};
static UINT16 g_au16B[16] = {0};
static UINT16 g_au16T[16] = {0};

/**
 * @function   fb_ntFbInit
 * @brief      创建显示设备
 * @param[in]  UINT32 uiChn               设备号
 * @param[in]  FB_COMMON *pfb_chn         设备信息
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  显示参数
 * @param[in]  FB_INIT_ATTR_ST *pstFbPrm  图形界面信息
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ntFbInit(UINT32 uiChn, FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr)
{
    HD_RESULT enRet = HD_OK;
    HD_FB_FMT stFbFmt;
    HD_FB_DIM stFbDim;
    HD_FB_ENABLE stFbEnable;
    HD_FB_ID enFbId;
    HD_PATH_ID u32PathId;
    HD_VIDEO_PXLFMT enPxlFmt;

    switch (uiChn) {
    case 0:
        u32PathId = g_aCtrlIds[0];
        enFbId = HD_FB1;
        enPxlFmt = HD_VIDEO_PXLFMT_ARGB8888;
        break;
    case 1:
        u32PathId = g_aCtrlIds[1];
        enFbId = HD_FB1;
        enPxlFmt = HD_VIDEO_PXLFMT_ARGB8888;
        break;
    case 2:
        u32PathId = g_aCtrlIds[0];
        enFbId = HD_FB2;
        enPxlFmt = HD_VIDEO_PXLFMT_I4;
        break;
    case 3:
        u32PathId = g_aCtrlIds[1];
        enFbId = HD_FB2;
        enPxlFmt = HD_VIDEO_PXLFMT_I4;
        break;
    default:
        DISP_LOGE("unsupport chn:%u\n", uiChn);
        return SAL_FAIL;
    }

    stFbFmt.fb_id = enFbId;
    stFbFmt.fmt = enPxlFmt;
    if (HD_OK != (enRet = hd_videoout_set(u32PathId, HD_VIDEOOUT_PARAM_FB_FMT, &stFbFmt)))
    {
        DISP_LOGE("chn:%u set fb fmt fail:%d\n", uiChn, enRet);
        return SAL_FAIL;
    }

    if (HD_FB1 == enFbId)
    {
        stFbDim.fb_id = enFbId;
        stFbDim.input_dim.w = pstHalFbAttr->stScreenAttr[0].width;
        stFbDim.input_dim.h = pstHalFbAttr->stScreenAttr[0].height;
        stFbDim.output_rect.x = 0;
        stFbDim.output_rect.y = 0;
        stFbDim.output_rect.w = pstHalFbAttr->stScreenAttr[0].width;
        stFbDim.output_rect.h = pstHalFbAttr->stScreenAttr[0].height;
        if (HD_OK != (enRet = hd_videoout_set(u32PathId, HD_VIDEOOUT_PARAM_FB_DIM, &stFbDim)))
        {
            DISP_LOGE("chn:%u set fb dim:%uX%u fail:%d\n", uiChn,stFbDim.input_dim.w, stFbDim.input_dim.h, enRet);
            return SAL_FAIL;
        }
    }

    stFbEnable.fb_id = enFbId;
    stFbEnable.enable = 1;
    if (HD_OK != (enRet = hd_videoout_set(u32PathId, HD_VIDEOOUT_PARAM_FB_ENABLE, &stFbEnable)))
    {
        DISP_LOGE("chn:%u set fb enable fail:%d\n", uiChn, enRet);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   fb_ntFbDeInit
 * @brief      创建显示设备
 * @param[in]  UINT32 uiChn 设备号
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ntFbDeInit(UINT32 uiChn)
{
    HD_RESULT enRet = HD_OK;
    HD_FB_ENABLE stFbEnable;
    HD_PATH_ID u32PathId;
    HD_FB_ID enFbId;

    switch (uiChn) {
    case 0:
        u32PathId = g_aCtrlIds[0];
        enFbId = HD_FB1;
        break;
    case 1:
        u32PathId = g_aCtrlIds[1];
        enFbId = HD_FB1;
        break;
    case 2:
        u32PathId = g_aCtrlIds[0];
        enFbId = HD_FB2;
        break;
    case 3:
        u32PathId = g_aCtrlIds[1];
        enFbId = HD_FB2;
        break;
    default:
        DISP_LOGE("unsupport chn:%u\n", uiChn);
        return SAL_FAIL;
    }

    stFbEnable.fb_id = enFbId;
    stFbEnable.enable = 0;
    if (HD_OK != (enRet = hd_videoout_set(u32PathId, HD_VIDEOOUT_PARAM_FB_ENABLE, &stFbEnable)))
    {
        DISP_LOGE("chn:%u set fb enable fail:%d\n", uiChn, enRet);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   fb_ntCreateFb
 * @brief      创建显示设备
 * @param[in]  UINT32 uiChn               设备号
 * @param[in]  FB_COMMON *pfb_chn         设备信息
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  显示参数
 * @param[in]  FB_INIT_ATTR_ST *pstFbPrm  图形界面信息
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ntCreateFb(UINT32 uiChn, FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr, FB_INIT_ATTR_ST *pstFbPrm)
{
    int ret = -1;
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    struct fb_var_screeninfo stVarInfo;
    struct fb_fix_screeninfo stFixInfo;
    struct fb_cursor stFbc;
    FB_COMMON *pstMouseFb = NULL;
    UINT32 u32Size = 0;
    UINT64 u64PhyAddr = 0;
    VOID *pVirAddr = NULL;
    UINT8 au8Mask[64 * 64 / 2];

    if (uiChn > 3)
    {
        DISP_LOGE("invalid chn:%u\n", uiChn);
        return SAL_FAIL;
    }

    if (2 == uiChn)
    {
        memset(&stFbc, 0, sizeof(stFbc));
        memset(g_au8MouseImage, 0, sizeof(g_au8MouseImage));
        memset(au8Mask, 0xff, sizeof(au8Mask));

        stFbc.set    = FB_CUR_SETALL;
        stFbc.mask   = (const char *)au8Mask;
        stFbc.enable = 0;
        stFbc.image.data = (const char *)g_au8MouseImage;
        stFbc.image.dx   = 0;
        stFbc.image.dy   = 0;
        stFbc.image.width  = 64;
        stFbc.image.height = 64;
        stFbc.image.depth  = 4;
        stFbc.image.cmap.start  = 1;
        stFbc.image.cmap.len    = 16;
        stFbc.image.cmap.red    = g_au16R;
        stFbc.image.cmap.green  = g_au16G;
        stFbc.image.cmap.blue   = g_au16B;
        stFbc.image.cmap.transp = g_au16T;

        for (i = 0; i < sizeof(g_aszMouseFdName)/sizeof(g_aszMouseFdName[0]); i++)
        {
            pstMouseFb = g_astMouseFb + i;

            if (pstMouseFb->fd <= 0)
            {
                strcpy((char *)pstMouseFb->fbName, g_aszMouseFdName[i]);
                pstMouseFb->fd = open((char *)pstMouseFb->fbName, O_RDWR, 0);
                if (pstMouseFb->fd < 0)
                {
                    // continue or return??????
                    DISP_LOGW("open %s failed!\n", pstMouseFb->fbName);
                    //return SAL_FAIL;
                    continue;
                }
            }

            if (SAL_FALSE == pstMouseFb->DevMMAP || pstMouseFb->fb_dev.uiWidth != pstHalFbAttr->stScreenAttr[0].width || pstMouseFb->fb_dev.uiHeight != pstHalFbAttr->stScreenAttr[0].height)
            {
                if (SAL_SOK != fb_ntFbInit(2 + i, pstMouseFb, pstHalFbAttr))
                {
                    DISP_LOGE("chn[%u] fb init failed!\n", uiChn);
                    return SAL_FAIL;
                }
                
                if (ioctl(pstMouseFb->fd, LCD300_IOC_SET_IOCURSOR, &stFbc) < 0)
                {
                    DISP_LOGE("config cursor fail\n");
                    return SAL_FAIL;
                }

                pstMouseFb->DevMMAP = SAL_TRUE;
                pstMouseFb->fb_dev.uiWidth  = pstHalFbAttr->stScreenAttr[0].width;
                pstMouseFb->fb_dev.uiHeight = pstHalFbAttr->stScreenAttr[0].height;
            }
        }

        pfb_chn->DevMMAP = SAL_TRUE;
        pfb_chn->fb_dev.uiWidth  = pstHalFbAttr->stScreenAttr[0].width;
        pfb_chn->fb_dev.uiHeight = pstHalFbAttr->stScreenAttr[0].height;
        pfb_chn->mouseLastChn = 0;
        pfb_chn->fd = g_astMouseFb[pfb_chn->mouseLastChn].fd;
    }
    else
    {
        if (pfb_chn->fd <= 0)
        {
            strcpy((char *)pfb_chn->fbName, g_aszUiFdName[uiChn]);
            pfb_chn->fd = open((char *)pfb_chn->fbName, O_RDWR, 0);
            if (pfb_chn->fd < 0)
            {
                DISP_LOGE("open %s failed, errno: %d, %s\n", pfb_chn->fbName, errno, strerror(errno));
                return SAL_FAIL;
            }
        }

        pfb_chn->bitWidth = FB_DATA_BITWIDTH_32;
        pfb_chn->u32BytePerPixel = SAL_CEIL(pfb_chn->bitWidth, 8);

        if (SAL_FALSE == pfb_chn->DevMMAP || pfb_chn->fb_dev.uiWidth != pstHalFbAttr->stScreenAttr[0].width || pfb_chn->fb_dev.uiHeight != pstHalFbAttr->stScreenAttr[0].height)
        {
            if (SAL_SOK != fb_ntFbInit(uiChn, pfb_chn, pstHalFbAttr))
            {
                DISP_LOGE("chn[%u] fb init failed!\n", uiChn);
                return SAL_FAIL;
            }

            ret = ioctl(pfb_chn->fd, FBIOGET_VSCREENINFO, &stVarInfo);
            if (ret < 0)
            {
                DISP_LOGE("get var screen info fail\n");
                return SAL_FAIL;
            }

            stVarInfo.xres = pstHalFbAttr->stScreenAttr[0].width;
            stVarInfo.yres = pstHalFbAttr->stScreenAttr[0].height;
            stVarInfo.xres_virtual = pstHalFbAttr->stScreenAttr[0].width;
            stVarInfo.yres_virtual = pstHalFbAttr->stScreenAttr[0].height;
            stVarInfo.bits_per_pixel = 32;
            stVarInfo.transp = s_a32;
            stVarInfo.red    = s_r32;
            stVarInfo.green  = s_g32;
            stVarInfo.blue   = s_b32;
            stVarInfo.activate = FB_ACTIVATE_NOW;
            stVarInfo.xoffset  = 0;
            stVarInfo.yoffset  = 0;
            ret = ioctl(pfb_chn->fd, FBIOPUT_VSCREENINFO, &stVarInfo);
            if (ret < 0)
            {
                DISP_LOGE("set var screen info fail\n");
                return SAL_FAIL;
            }

            pfb_chn->DevMMAP = SAL_TRUE;
            pfb_chn->fb_dev.uiWidth  = pstHalFbAttr->stScreenAttr[0].width;
            pfb_chn->fb_dev.uiHeight = pstHalFbAttr->stScreenAttr[0].height;

            ret = ioctl(pfb_chn->fd, FBIOGET_FSCREENINFO, &stFixInfo);
            if (ret < 0)
            {
                DISP_LOGE("get fix screen info fail\n");
                return SAL_FAIL;
            }

            pfb_chn->fb_dev.ui64CanvasAddr = stFixInfo.smem_start;
            pfb_chn->fb_dev.uiSmem_len     = stFixInfo.smem_len;
            pfb_chn->fb_dev.pBuf           = mmap(NULL, stFixInfo.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, pfb_chn->fd, 0);
            if (NULL == pfb_chn->fb_dev.pBuf)
            {
                DISP_LOGE("map fail, addr:0x%llx, size:%u\n", pfb_chn->fb_dev.ui64CanvasAddr, pfb_chn->fb_dev.uiSmem_len);
                return SAL_FAIL;
            }
            memset(pfb_chn->fb_dev.pBuf, 0xff, pfb_chn->fb_dev.uiSmem_len);
            DISP_LOGI("fb1 buff init success, phy:0x%llx, vir:%p, size:%u\n", pfb_chn->fb_dev.ui64CanvasAddr, pfb_chn->fb_dev.pBuf, pfb_chn->fb_dev.uiSmem_len);
        }
    }
    
    if (SAL_FALSE == pfb_chn->AppMMAP || pfb_chn->fb_app.uiWidth != pstHalFbAttr->stScreenAttr[0].width || pfb_chn->fb_app.uiHeight != pstHalFbAttr->stScreenAttr[0].height)
    {
        u32Size = pstHalFbAttr->stScreenAttr[0].width * pstHalFbAttr->stScreenAttr[0].height * pstHalFbAttr->stScreenAttr[0].bitWidth / 8;
        s32Ret = mem_hal_mmzAlloc(u32Size, "fb", "fb", NULL, SAL_FALSE, &u64PhyAddr, &pVirAddr);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("alloc %u bytes fail\n", u32Size);
            return SAL_FAIL;
        }

        pfb_chn->AppMMAP = SAL_TRUE;
        pfb_chn->fb_app.uiWidth  = pstHalFbAttr->stScreenAttr[0].width;
        pfb_chn->fb_app.uiHeight = pstHalFbAttr->stScreenAttr[0].height;
        pfb_chn->fb_app.uiSize   = u32Size;
        pfb_chn->fb_app.ui64Phyaddr = (PhysAddr)u64PhyAddr;
        pfb_chn->fb_app.Viraddr = pVirAddr;
        memset(pVirAddr, 0, u32Size);

        if (2 == uiChn)
        {
            for (i = 0; i < sizeof(g_aszMouseFdName)/sizeof(g_aszMouseFdName[0]); i++)
            {
                pstMouseFb = g_astMouseFb + i;
                pstMouseFb->AppMMAP = SAL_TRUE;
                pstMouseFb->fb_app.uiWidth  = pstHalFbAttr->stScreenAttr[0].width;
                pstMouseFb->fb_app.uiHeight = pstHalFbAttr->stScreenAttr[0].height;
                pstMouseFb->fb_app.uiSize   = u32Size;
                pstMouseFb->fb_app.ui64Phyaddr = (PhysAddr)u64PhyAddr;
                pstMouseFb->fb_app.Viraddr = pVirAddr;
            }
        }
    }

    pstHalFbAttr->stScreenAttr[0].srcAddr = pfb_chn->fb_app.Viraddr;
    pstHalFbAttr->stScreenAttr[0].srcSize = pfb_chn->fb_app.uiSize;

    DISP_LOGI("%u create fb success, width: %u, height: %u\n", uiChn, pstHalFbAttr->stScreenAttr[0].width, pstHalFbAttr->stScreenAttr[0].height);

    return SAL_SOK;
}


/**
 * @function   fb_ntReleasFb
 * @brief      释放显示设备
 * @param[in]  UINT32 uiChn        设备号
 * @param[in]  FB_COMMON *pfb_chn  设备信息
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_ntReleasFb(UINT32 uiChn, FB_COMMON *pfb_chn)
{
    VOID *pvVirAddr = NULL;
    struct fb_cursor stFbc;
    UINT32 i = 0;
    FB_COMMON *pstMouseFb = NULL;

    if (SAL_TRUE == pfb_chn->AppMMAP)
    {
        pvVirAddr = pfb_chn->fb_app.Viraddr;
        mem_hal_mmzFree(pvVirAddr, "fb", "fb");

        pfb_chn->AppMMAP = SAL_FALSE;
        pfb_chn->fb_app.uiWidth  = 0;
        pfb_chn->fb_app.uiHeight = 0;
        pfb_chn->fb_app.uiSize   = 0;
        pfb_chn->fb_app.ui64Phyaddr = 0;
        pfb_chn->fb_app.Viraddr     = NULL;

        if (2 == uiChn)
        {
            for (i = 0; i < sizeof(g_astMouseFb)/sizeof(g_astMouseFb[0]); i++)
            {
                pstMouseFb = g_astMouseFb + i;
                pstMouseFb->AppMMAP = SAL_FALSE;
                pstMouseFb->fb_app.uiWidth  = 0;
                pstMouseFb->fb_app.uiHeight = 0;
                pstMouseFb->fb_app.uiSize   = 0;
                pstMouseFb->fb_app.ui64Phyaddr = 0;
                pstMouseFb->fb_app.Viraddr     = NULL;
            }
        }
    }

    if (SAL_TRUE == pfb_chn->DevMMAP)
    {
        if (2 != uiChn)
        {
            munmap(pfb_chn->fb_dev.pShowScreen, pfb_chn->fb_dev.uiSmem_len);
            if (SAL_SOK != fb_ntFbDeInit(uiChn))
            {
                DISP_LOGE("chn[%u] fb deinit fail\n", uiChn);
            }

            pfb_chn->DevMMAP = SAL_FALSE;
            pfb_chn->fb_dev.uiWidth  = 0;
            pfb_chn->fb_dev.uiHeight = 0;
            pfb_chn->fb_dev.ui64CanvasAddr = 0;
            pfb_chn->fb_dev.uiSmem_len     = 0;
            pfb_chn->fb_dev.pShowScreen    = NULL;
            if (pfb_chn->fd > 0)
            {
                close(pfb_chn->fd);
            }
            pfb_chn->fd = -1;
        }
        else
        {
            memset(&stFbc, 0, sizeof(stFbc));
            stFbc.set    = FB_CUR_SETPOS;
            stFbc.enable = 0;
            stFbc.image.dx = 0;
            stFbc.image.dy = 0;
            stFbc.image.width  = 64;
            stFbc.image.height = 64;
            stFbc.image.depth  = 4;
            
            for (i = 0; i < sizeof(g_astMouseFb)/sizeof(g_astMouseFb[0]); i++)
            {
                pstMouseFb = g_astMouseFb + i;

                if (pstMouseFb->fd <= 0)
                {
                    strcpy((char *)pstMouseFb->fbName, g_aszMouseFdName[i]);
                    pstMouseFb->fd = open((char *)pstMouseFb->fbName, O_RDWR, 0);
                    if (pstMouseFb->fd < 0)
                    {
                        DISP_LOGE("open %s failed!\n", pstMouseFb->fbName);
                        return SAL_FAIL;
                    }
                }

                if (ioctl(pstMouseFb->fd, LCD300_IOC_SET_IOCURSOR, &stFbc) < 0)
                {
                    DISP_LOGE("set cursor fail\n");
                }

                if (SAL_SOK != fb_ntFbDeInit(2 + i))
                {
                    DISP_LOGE("chn[%u] fb deinit fail\n", uiChn);
                }

                pstMouseFb->DevMMAP = SAL_FALSE;
                pstMouseFb->fb_dev.uiWidth  = 0;
                pstMouseFb->fb_dev.uiHeight = 0;
                pstMouseFb->fb_dev.ui64CanvasAddr = 0;
                pstMouseFb->fb_dev.uiSmem_len     = 0;
                pstMouseFb->fb_dev.pShowScreen    = NULL;
                close(pstMouseFb->fd);
                pstMouseFb->fd = -1;
            }
        }
    }

    return SAL_SOK;
}


/**
 * @function   fb_ntRefreshMouse
 * @brief      刷新鼠标
 * @param[in]  FB_COMMON *pfb_chn         fb设备信息
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  形状信息
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ntRefreshMouse(FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr)
{
    UINT32 i, j;
    UINT8 *pu8Src = pfb_chn->fb_app.Viraddr;
    UINT8 *pu8Dst = g_au8MouseImage;
    UINT8 u8Idx = 0;;
    struct fb_cursor stFbc;
    UINT8 au8Mask[64 * 64 / 2];
    UINT8 au8Map[16] = {0};

    if (pstHalFbAttr->stScreenAttr[0].alpha_idx >= PALETTE_COLOR_MAX)
    {
        DISP_LOGE("invalid alpha index:%u\n", pstHalFbAttr->stScreenAttr[0].alpha_idx);
        return SAL_FAIL;
    }

    if (pfb_chn->fd <= 0)
    {
        strcpy((char *)pfb_chn->fbName, g_aszMouseFdName[pfb_chn->mouseLastChn]);
        pfb_chn->fd = open((char *)pfb_chn->fbName, O_RDWR, 0);
        if (pfb_chn->fd < 0)
        {
            DISP_LOGE("open %s failed!\n", pfb_chn->fbName);
            return SAL_FAIL;
        }
    }

    i = 0;
    for (j = 0; j < PALETTE_COLOR_MAX; j++)
    {
        if (j < pstHalFbAttr->stScreenAttr[0].alpha_idx)
        {
            au8Map[j] = j + 1;
        }
        else if (j > pstHalFbAttr->stScreenAttr[0].alpha_idx)
        {
            au8Map[j] = j;
        }
        else
        {
            au8Map[j] = 0; /* app把不常用的绿色索引值0x0a作为透明色，而NT平台时0x00表示透明色，所以这里颜色索引值要重新映射一下 */
            continue;
        }

        g_au16R[i] = (pstHalFbAttr->stScreenAttr[0].palette[j] >> 16) & 0x00FF;
        g_au16G[i] = (pstHalFbAttr->stScreenAttr[0].palette[j] >> 8) & 0x00FF;
        g_au16B[i] = pstHalFbAttr->stScreenAttr[0].palette[j] & 0x00FF;
        g_au16T[i] = 0;
        i++;
    }

    memset(&stFbc, 0, sizeof(stFbc));
    memset(g_au8MouseImage, 0, sizeof(g_au8MouseImage));
    memset(au8Mask, 0xff, sizeof(au8Mask));

    for (i = 0; i < pstHalFbAttr->stScreenAttr[0].height; i++)
    {
        pu8Dst = g_au8MouseImage + (64 / 2) * i;
        for (j = 0; j < pstHalFbAttr->stScreenAttr[0].width; j += 2)
        {
            u8Idx = (au8Map[(*pu8Src >> 4) & 0x0F] << 4) & 0xF0;
            u8Idx |= (au8Map[*pu8Src & 0x0F] & 0x0F);
            *pu8Dst++ = u8Idx;
            pu8Src++;
        }
    }

    stFbc.set    = FB_CUR_SETCMAP;
    stFbc.mask   = (const char *)au8Mask;
    stFbc.enable = 0;
    stFbc.image.data = (const char *)g_au8MouseImage; /* NT下发的是颜色索引值 */
    stFbc.image.width  = 64;
    stFbc.image.height = 64;
    stFbc.image.depth  = 4;
    stFbc.image.cmap.start  = 1;
    stFbc.image.cmap.len    = PALETTE_COLOR_MAX;
    stFbc.image.cmap.red    = g_au16R;
    stFbc.image.cmap.green  = g_au16G;
    stFbc.image.cmap.blue   = g_au16B;
    stFbc.image.cmap.transp = g_au16T;
    if (ioctl(pfb_chn->fd, LCD300_IOC_SET_IOCURSOR, &stFbc) < 0)
    {
        DISP_LOGE("set cursor fail\n");
        return SAL_FAIL;
    }

    stFbc.set = FB_CUR_SETIMAGE;
    if (ioctl(pfb_chn->fd, LCD300_IOC_SET_IOCURSOR, &stFbc) < 0)
    {
        DISP_LOGE("set cursor fail\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   fb_ntRefresh
 * @brief      刷新UI显示
 * @param[in]  FB_COMMON *pfb_chn         fb设备信息
 * @param[in]  SCREEN_CTRL *pstHalFbAttr  UI
 * @param[out] None
 * @return     INT32 SAL_SOK   成功
 *                   SAL_FAIL  失败
 */
INT32 fb_ntRefresh(UINT32 uiChn, FB_COMMON *pfb_chn, SCREEN_CTRL *pstHalFbAttr)
{
    if (2 == uiChn)
    {
        return fb_ntRefreshMouse(pfb_chn, pstHalFbAttr);
    }

    return SAL_SOK;
}


/**
 * @function   fb_ntSetOrgin
 * @brief      设置显示启始坐标
 * @param[in]  UINT32 uiChn               
 * @param[in]  FB_COMMON *pfb_chn         
 * @param[in]  POS_ATTR *pstPos  
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 fb_ntSetOrgin(UINT32 uiChn,FB_COMMON *pfb_chn, POS_ATTR *pstPos)
{
    struct fb_cursor stFbc;

    if (2 == uiChn)
    {
        if (pfb_chn->fd <= 0)
        {
            strcpy((char *)pfb_chn->fbName, g_aszMouseFdName[pfb_chn->mouseLastChn]);
            pfb_chn->fd = open((char *)pfb_chn->fbName, O_RDWR, 0);
            if (pfb_chn->fd < 0)
            {
                DISP_LOGE("open %s failed!\n", pfb_chn->fbName);
                return SAL_FAIL;
            }
        }

        memset(&stFbc, 0, sizeof(stFbc));
        stFbc.set    = FB_CUR_SETPOS;
        stFbc.enable = 1;
        stFbc.image.dx = pstPos->x;
        stFbc.image.dy = pstPos->y;
        stFbc.image.width  = 64;
        stFbc.image.height = 64;
        stFbc.image.depth  = 4;
        if (ioctl(pfb_chn->fd, LCD300_IOC_SET_IOCURSOR, &stFbc) < 0)
        {
            DISP_LOGE("set cursor fail\n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/**
 * @function   fb_ntSetMouseFbChn
 * @brief      切换鼠标通道绑定到对用VO设备通道
 * @param[in]  UINT32 uiChn        设备号
 * @param[in]  UINT32 uiBindChn    绑定的设备号
 * @param[in]  FB_COMMON *pfb_chn  设备信息
 * @param[out] None
 * @return     static INT32  SAL_SOK   成功
 *                           SAL_FAIL  失败
 */
static INT32 fb_ntSetMouseFbChn(UINT32 uiChn, UINT32 uiBindChn, FB_COMMON *pfb_chn)
{
    FB_COMMON *pstNewMouseFb = NULL;
    FB_COMMON *pLastMouseFb  = g_fb_common + 2;  
    struct fb_cursor stFbc;
    UINT8 au8Mask[64 * 64 / 2] = {0};

    if (uiBindChn == pLastMouseFb->mouseLastChn)
    {
        DISP_LOGW("uiBindChn %d is same\n", uiBindChn);
        return SAL_SOK;
    }

    memset(au8Mask, 0xff, sizeof(au8Mask));
    memset(&stFbc, 0, sizeof(stFbc));

    stFbc.set    = FB_CUR_SETPOS;
    stFbc.enable = 0;
    stFbc.image.dx = 0;
    stFbc.image.dy = 0;
    stFbc.image.width  = 64;
    stFbc.image.height = 64;
    stFbc.image.depth  = 4;
    if (ioctl(pLastMouseFb->fd, LCD300_IOC_SET_IOCURSOR, &stFbc) < 0)
    {
        DISP_LOGE("set cursor fail:%s\n", strerror(errno));
    }

    pstNewMouseFb = g_astMouseFb + uiBindChn;
    memcpy(pLastMouseFb, pstNewMouseFb, sizeof(*pstNewMouseFb));
    pLastMouseFb->mouseLastChn = uiBindChn;

    stFbc.set    = FB_CUR_SETCMAP;
    stFbc.mask   = (const char *)au8Mask;
    stFbc.enable = 0;
    stFbc.image.data = (const char *)g_au8MouseImage;
    stFbc.image.width  = 64;
    stFbc.image.height = 64;
    stFbc.image.depth  = 4;
    stFbc.image.cmap.start  = 1;
    stFbc.image.cmap.len    = PALETTE_COLOR_MAX;
    stFbc.image.cmap.red    = g_au16R;
    stFbc.image.cmap.green  = g_au16G;
    stFbc.image.cmap.blue   = g_au16B;
    stFbc.image.cmap.transp = g_au16T;
    if (ioctl(pstNewMouseFb->fd, LCD300_IOC_SET_IOCURSOR, &stFbc) < 0)
    {
        DISP_LOGE("set cursor fail:%s\n", strerror(errno));
        return SAL_FAIL;
    }

    stFbc.set = FB_CUR_SETIMAGE;
    if (ioctl(pstNewMouseFb->fd, LCD300_IOC_SET_IOCURSOR, &stFbc) < 0)
    {
        DISP_LOGE("set cursor fail:%s\n", strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function   fb_ntGetFbNum
 * @brief      获取设备最大显示设备数
 * @param[in]  void  
 * @param[out] None
 * @return     static INT32 FB_DEV_NUM_MAX 设备数量
 *                      
 */
static INT32 fb_ntGetFbNum(void)
{
    return FB_DEV_NUM_MAX;
}


/**
 * @function   fb_halRegister
 * @brief      初始化fb成员函数
 * @param[in]  void  
 * @param[out] None
 * @return     FB_PLAT_OPS * 返回函数结构体指针
 */
FB_PLAT_OPS *fb_halRegister(void)
{
    memset(&g_stFbPlatOps, 0,sizeof(FB_PLAT_OPS));

    g_stFbPlatOps.createFb   = fb_ntCreateFb;
    g_stFbPlatOps.releaseFb  = fb_ntReleasFb;
    g_stFbPlatOps.refreshFb  = fb_ntRefresh;
    g_stFbPlatOps.setOrgin  =  fb_ntSetOrgin;
    g_stFbPlatOps.setFbChn   = fb_ntSetMouseFbChn;
    g_stFbPlatOps.getFbNum   = fb_ntGetFbNum;

    return &g_stFbPlatOps;
}

