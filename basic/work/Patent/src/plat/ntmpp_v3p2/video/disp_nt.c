/**
 * @file   disp_nt.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */

/*******************************************************************************
* disp_nt.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author :
* Version: V1.0.0  2021年11月16日 Create
*
* Description :
* Modification:
*******************************************************************************/

#include <sal.h>
#include "platform_hal.h"
#include "platform_sdk.h"
#include "disp_hal_inter.h"
#include <execinfo.h>

#define HD_COMMON_VIDEO_OUT_BT1120		(9)
#define VIDEOOUT_BT1120_1920X1080P60	(3)
#define VIDEOOUT_BT1120_1920X1080P50	(9)

#line __LINE__ "disp_nt.c"

typedef struct
{
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Fps;
    UINT32 u32Id;
} NT_RES_MAP_S;

typedef struct
{
    HD_COMMON_MEM_VB_BLK blk;
    UINT32 size;
    UINTPTR phy_addr;
    VOID *storage;
    HD_COMMON_MEM_DDR_ID ddr_id;
} app_buffer_t;

typedef struct _VIDEO_LIVEVIEW
{
    HD_PATH_ID videoout_path[2];   /* video out module */
    HD_PATH_ID videoout_ctrl_path;               /* video out ctrl module */
    HD_PATH_ID videoout_2nd_ctrl_path;               /* video out ctrl module */
    HD_VIDEOOUT_SYSCAPS videoout_syscaps;        /* video out capabilities */
    HD_FB_FMT videoout_2nd_fb;
} VIDEO_LIVEVIEW;



static NT_RES_MAP_S g_astInMap[] = {
    {1920, 1080, 60, HD_VIDEOOUT_IN_1920x1080},
    {1920, 1080, 50, HD_VIDEOOUT_IN_1920x1080},
};

static NT_RES_MAP_S g_astHdmiOutMap[] = {
    {1920, 1080, 60, HD_VIDEOOUT_HDMI_1920X1080P60, },
    {1920, 1080, 50, HD_VIDEOOUT_HDMI_1920X1080P50, },
};
static NT_RES_MAP_S g_astBt1120OutMap[] = {
    {1920, 1080, 60, VIDEOOUT_BT1120_1920X1080P60, },
    {1920, 1080, 50, VIDEOOUT_BT1120_1920X1080P50, },
};

HD_PATH_ID g_aCtrlIds[DISP_HAL_MAX_DEV_NUM] = {0};
HD_PATH_ID g_aOutIds[DISP_HAL_MAX_DEV_NUM][DISP_HAL_MAX_CHN_NUM] = {0};
static DISP_PLAT_OPS g_stDispPlatOps;


/*******************************************************************************
* 函数名  : disp_ntModInit
* 描  述  : 创建显示
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntModInit(VOID *prm)
{
    HD_RESULT enRet = HD_OK;

    SAL_UNUSED(prm);

    if ((enRet = hd_videoout_init()) != HD_OK)
    {
        DISP_LOGE("videoout init fail:%d\n", enRet);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntGetResId
* 描  述  :
* 输  入  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntGetResId(UINT32 u32Width, UINT32 u32Height, UINT32 u32Fps, UINT32 *pu32Res, NT_RES_MAP_S *pstMap, UINT32 u32Size)
{
    UINT32 i = 0;
    NT_RES_MAP_S *pstRes = pstMap;

    for ( ; i < u32Size; i++)
    {
        if (pstRes->u32Width == u32Width && pstRes->u32Height == u32Height && pstRes->u32Fps == u32Fps)
        {
            *pu32Res = pstRes->u32Id;
            return SAL_SOK;
        }

        pstRes++;
    }

    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : disp_ntPrintCaps
* 描  述  : 打印能力级
* 输  入  : HD_PATH_ID path_id
* 输  出  : 无
* 返回值  : 无
*******************************************************************************/
VOID disp_ntPrintCaps(HD_PATH_ID path_id)
{
    HD_RESULT enRet = HD_OK;
    HD_VIDEOOUT_SYSCAPS stCaps;
    UINT32 i = 0;

    if (HD_OK != (enRet = hd_videoout_get(path_id, HD_VIDEOOUT_PARAM_SYSCAPS, &stCaps)))
    {
        DISP_LOGW("get output caps fail:%d\n", enRet);
    }

    printf("\n\n");
    printf("------------------- videoout caps start -------------------\n\n");
    printf("dev_id:%u\n", stCaps.dev_id);
    printf("chip_id:%u\n", stCaps.chip_id);
    printf("max_in_count:%u\n", stCaps.max_in_count);
    printf("max_out_count:%u\n", stCaps.max_out_count);
    printf("max_fb_count:%u\n", stCaps.max_fb_count);
    printf("dev_caps:0x%x\n", stCaps.dev_caps);
    printf("in_caps:\n");
    for (i = 0; i < stCaps.max_in_count; i++)
    {
        printf("    in_caps[%u]:0x%x\n", i, stCaps.in_caps[i]);
    }

    printf("out_caps:\n");
    for (i = 0; i < stCaps.max_out_count; i++)
    {
        printf("    out_caps[%u]:0x%x\n", i, stCaps.out_caps[i]);
    }

    printf("fb_caps:\n");
    for (i = 0; i < stCaps.max_fb_count; i++)
    {
        printf("    fb_caps[%u]:0x%x\n", i, stCaps.fb_caps[i]);
    }

    printf("output_dim:%uX%u\n", stCaps.output_dim.w, stCaps.output_dim.h);
    printf("input_dim:%uX%u\n", stCaps.input_dim.w, stCaps.input_dim.h);
    printf("fps:%u\n", stCaps.fps);
    printf("max_scale_up_w:%u\n", stCaps.max_scale_up_w);
    printf("max_scale_up_h:%u\n", stCaps.max_scale_up_h);
    printf("max_scale_down_w:%u\n", stCaps.max_scale_down_w);
    printf("max_scale_down_h:%u\n", stCaps.max_scale_down_h);
    printf("max_out_stamp:%u\n", stCaps.max_out_stamp);
    printf("max_out_stamp_ex:%u\n", stCaps.max_out_stamp_ex);
    printf("max_out_mask:%u\n", stCaps.max_out_mask);
    printf("max_out_mask_ex:%u\n", stCaps.max_out_mask_ex);
    printf("\n-------------------- videoout caps end --------------------\n");
    printf("\n\n");
}

/*******************************************************************************
* 函数名  : disp_ntCreateDev
* 描  述  : 创建显示
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntCreateDev(DISP_DEV_COMMON *pDispDev)
{
    HD_RESULT enRet = HD_OK;
    UINT32 u32DevNo = 0;
    UINT32 u32Size = 0;
    UINT32 u32InId = 0;
    UINT32 u32OutId = 0;
    VOID *pvVirAddr = NULL;
    HD_PATH_ID u32Id = 0;
    HD_VIDEOOUT_MODE stMode = {0};
    HD_VIDEOOUT_DEV_CONFIG stConfig = {0};
    HD_COMMON_MEM_POOL_INFO stDispFb = {0};
    VENDOR_VIDEOOUT_DISP_MAX_FPS stMaxFps = {0};
    HD_FB_FMT stFbFmt;
    HD_FB_DIM stFbDim;
    HD_FB_ENABLE stFbEnable;

    if (NULL == pDispDev)
    {
        DISP_LOGE("null pointer\n");
        return SAL_FAIL;
    }

    u32DevNo = pDispDev->uiDevNo;

    if (0 == u32DevNo)
    {
        if (SAL_SOK != disp_ntGetResId(pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps, &u32OutId, g_astHdmiOutMap, sizeof(g_astHdmiOutMap) / sizeof(g_astHdmiOutMap[0])))
        {
            DISP_LOGE("unsupport resolution:%uX%uP%u\n", pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps);
            return SAL_FAIL;
        }

        stMode.output_type = HD_COMMON_VIDEO_OUT_HDMI;
        stMode.output_mode.hdmi = u32OutId;
    }
    else if (1 == u32DevNo)
    {
        if (SAL_SOK != disp_ntGetResId(pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps, &u32OutId, g_astBt1120OutMap, sizeof(g_astBt1120OutMap) / sizeof(g_astBt1120OutMap[0])))
        {
            DISP_LOGE("unsupport resolution:%uX%uP%u\n", pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps);
            return SAL_FAIL;
        }

        stMode.output_type = HD_COMMON_VIDEO_OUT_BT1120;
        stMode.output_mode.lcd = u32OutId;
    }
    else
    {
        DISP_LOGE("invalid dev no[%u]\n", u32DevNo);
        return SAL_FAIL;
    }

    if (SAL_SOK != disp_ntGetResId(pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps, &u32InId, g_astInMap, sizeof(g_astInMap) / sizeof(g_astInMap[0])))
    {
        DISP_LOGE("unsupport resolution:%uX%uP%u\n", pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps);
        return SAL_FAIL;
    }

    stMode.input_dim = u32InId;

    if (HD_OK != (enRet = hd_videoout_open(0, HD_VIDEOOUT_CTRL(u32DevNo), &u32Id)))
    {
        DISP_LOGE("%u hd_videoout_open fail:%d\n", u32DevNo, enRet);
        return SAL_FAIL;
    }

    DISP_LOGI("dev:%u open path:0x%x success\n", u32DevNo, u32Id);
    g_aCtrlIds[u32DevNo] = u32Id;

#if 0
    (VOID)disp_ntPrintCaps(u32Id);
#endif

    stConfig.devnvr_cfg.mode = stMode;
    stConfig.devnvr_cfg.homology = 0;
    stConfig.devnvr_cfg.chip_state = 1;
    stDispFb.type = HD_COMMON_MEM_DISP0_IN_POOL + u32DevNo;
    stDispFb.ddr_id = DDR_ID1;
    if (HD_OK != (enRet = hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, (VOID *)&stDispFb)))
    {
        DISP_LOGE("pool[%d] ddr[%d] get mem fail:%d\n", stDispFb.type, stDispFb.ddr_id, enRet);
        return SAL_FAIL;
    }

    stConfig.devnvr_cfg.plane[0].max_w = pDispDev->uiDevWith;
    stConfig.devnvr_cfg.plane[0].max_h = pDispDev->uiDevHeight;
    stConfig.devnvr_cfg.plane[0].max_bpp = 12;
    stConfig.devnvr_cfg.plane[0].gui_rld_enable = 0;
    stConfig.devnvr_cfg.plane[0].ddr_id = stDispFb.ddr_id;
    stConfig.devnvr_cfg.plane[0].buf_paddr = stDispFb.start_addr;
    u32Size = SAL_align(stConfig.devnvr_cfg.plane[0].max_w, 16) * SAL_align(stConfig.devnvr_cfg.plane[0].max_h, 16) * stConfig.devnvr_cfg.plane[0].max_bpp / 8;
    stConfig.devnvr_cfg.plane[0].buf_len = u32Size;
    stConfig.devnvr_cfg.plane[0].rle_buf_paddr = 0;
    stConfig.devnvr_cfg.plane[0].rle_buf_len = 0;

    pvVirAddr = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, stConfig.devnvr_cfg.plane[0].buf_paddr, stConfig.devnvr_cfg.plane[0].buf_len);
    if (NULL != pvVirAddr)
    {
        memset(pvVirAddr, 0xeb, stConfig.devnvr_cfg.plane[0].max_w * stConfig.devnvr_cfg.plane[0].max_h);
        memset(pvVirAddr + stConfig.devnvr_cfg.plane[0].max_w * stConfig.devnvr_cfg.plane[0].max_h, 0x80, stConfig.devnvr_cfg.plane[0].max_w * stConfig.devnvr_cfg.plane[0].max_h / 2);
        hd_common_mem_munmap(pvVirAddr, stConfig.devnvr_cfg.plane[0].buf_len);
    }

    stDispFb.type = HD_COMMON_MEM_DISP0_FB_POOL + u32DevNo;
    stDispFb.ddr_id = DDR_ID1;
    if (HD_OK != (enRet = hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, (VOID *)&stDispFb)))
    {
        DISP_LOGE("pool[%d] ddr[%d] get mem fail:%d\n", stDispFb.type, stDispFb.ddr_id, enRet);
        return SAL_FAIL;
    }

    stConfig.devnvr_cfg.plane[1].max_w = pDispDev->uiDevWith;
    stConfig.devnvr_cfg.plane[1].max_h = pDispDev->uiDevHeight;
    stConfig.devnvr_cfg.plane[1].max_bpp = 32;
    stConfig.devnvr_cfg.plane[1].gui_rld_enable = 0;
    stConfig.devnvr_cfg.plane[1].ddr_id = stDispFb.ddr_id;
    stConfig.devnvr_cfg.plane[1].buf_paddr = stDispFb.start_addr;
    u32Size = SAL_align(stConfig.devnvr_cfg.plane[1].max_w, 16) * SAL_align(stConfig.devnvr_cfg.plane[1].max_h, 16) * stConfig.devnvr_cfg.plane[1].max_bpp / 8;
    stConfig.devnvr_cfg.plane[1].buf_len = u32Size;
    stConfig.devnvr_cfg.plane[1].rle_buf_paddr = 0;
    stConfig.devnvr_cfg.plane[1].rle_buf_len = 0;

    pvVirAddr = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, stConfig.devnvr_cfg.plane[1].buf_paddr, stConfig.devnvr_cfg.plane[1].buf_len);
    if (NULL != pvVirAddr)
    {
        memset(pvVirAddr, 0x00, stConfig.devnvr_cfg.plane[1].buf_len);
        hd_common_mem_munmap(pvVirAddr, stConfig.devnvr_cfg.plane[1].buf_len);
    }

    stConfig.devnvr_cfg.plane[2].max_w = 64;
    stConfig.devnvr_cfg.plane[2].max_h = 64;
    stConfig.devnvr_cfg.plane[2].max_bpp = 32;
    stConfig.devnvr_cfg.plane[2].gui_rld_enable = 0;
    stConfig.devnvr_cfg.plane[2].ddr_id = stDispFb.ddr_id;
    stConfig.devnvr_cfg.plane[2].buf_paddr = stDispFb.start_addr + stConfig.devnvr_cfg.plane[1].buf_len;
    u32Size = SAL_align(stConfig.devnvr_cfg.plane[2].max_w, 16) * SAL_align(stConfig.devnvr_cfg.plane[2].max_h, 16) * stConfig.devnvr_cfg.plane[2].max_bpp / 8;
    stConfig.devnvr_cfg.plane[2].buf_len = u32Size;
    stConfig.devnvr_cfg.plane[2].rle_buf_paddr = 0;
    stConfig.devnvr_cfg.plane[2].rle_buf_len = 0;

    pvVirAddr = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, stConfig.devnvr_cfg.plane[2].buf_paddr, stConfig.devnvr_cfg.plane[2].buf_len);
    if (NULL != pvVirAddr)
    {
        memset(pvVirAddr, 0x00, stConfig.devnvr_cfg.plane[2].buf_len);
        hd_common_mem_munmap(pvVirAddr, stConfig.devnvr_cfg.plane[2].buf_len);
    }

    if (HD_OK != (enRet = hd_videoout_set(g_aCtrlIds[u32DevNo], HD_VIDEOOUT_PARAM_DEV_CONFIG, &stConfig)))
    {
        if (HD_ERR_NG == enRet)
        {
            DISP_LOGW("%u HD_VIDEOOUT_PARAM_DEV_CONFIG has already set:%d\n", u32DevNo, enRet);
        }
        else
        {
            DISP_LOGE("dev:%u set config fail:%d\n", u32DevNo, enRet);
            return SAL_FAIL;
        }
    }

    if (HD_OK != (enRet = hd_videoout_set(g_aCtrlIds[u32DevNo], HD_VIDEOOUT_PARAM_MODE, &stMode)))
    {
        DISP_LOGE("dev:%u set resolution:%uX%uP%u fail:%d\n", u32DevNo, pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps, enRet);
        return SAL_FAIL;
    }

    stFbFmt.fb_id = HD_FB0;
    stFbFmt.fmt = HD_VIDEO_PXLFMT_YUV420;
    if (HD_OK != (enRet = hd_videoout_set(g_aCtrlIds[u32DevNo], HD_VIDEOOUT_PARAM_FB_FMT, &stFbFmt)))
    {
        DISP_LOGE("dev:%u set fb fmt fail:%d\n", u32DevNo, enRet);
        return SAL_FAIL;
    }

    stFbDim.fb_id = HD_FB0;
    stFbDim.input_dim.w = pDispDev->uiDevWith;
    stFbDim.input_dim.h = pDispDev->uiDevHeight;
    stFbDim.output_rect.x = 0;
    stFbDim.output_rect.y = 0;
    stFbDim.output_rect.w = pDispDev->uiDevWith;
    stFbDim.output_rect.h = pDispDev->uiDevHeight;
    if (HD_OK != (enRet = hd_videoout_set(g_aCtrlIds[u32DevNo], HD_VIDEOOUT_PARAM_FB_DIM, &stFbDim)))
    {
        DISP_LOGE("dev:%u set fb dim:%uX%uP%u fail:%d\n", u32DevNo, pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps, enRet);
        return SAL_FAIL;
    }

    stFbEnable.fb_id = HD_FB0;
    stFbEnable.enable = 1;
    if (HD_OK != (enRet = hd_videoout_set(g_aCtrlIds[u32DevNo], HD_VIDEOOUT_PARAM_FB_ENABLE, &stFbEnable)))
    {
        DISP_LOGE("dev[%u] id[%u] set fb enable fail:%d\n", u32DevNo, g_aCtrlIds[u32DevNo], enRet);
        return SAL_FAIL;
    }

    stMaxFps.max_fps = 60;
    if (HD_OK != (enRet = vendor_videoout_set(g_aCtrlIds[u32DevNo], VENDOR_VIDEOOUT_PARAM_DISP_MAX_FPS, &stMaxFps)))
    {
        DISP_LOGE("dev[%u] id[%u] max fps fail:%d\n", u32DevNo, g_aCtrlIds[u32DevNo], enRet);
        return SAL_FAIL;
    }

    DISP_LOGI("%u create dev success, uiDevWith: %u, uiDevHeight: %u, uiDevFps: %u\n",
              u32DevNo, pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntDeleteDev
* 描  述  : 删除显示层
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntDeleteDev(VOID *prm)
{
    HD_RESULT enRet = HD_OK;
    DISP_DEV_COMMON *pDispDev = (DISP_DEV_COMMON *)prm;
    HD_FB_ENABLE stFbEnable;
    UINT32 u32DevNo;

    if (pDispDev == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    u32DevNo = pDispDev->uiDevNo;

    stFbEnable.fb_id = HD_FB0;
    stFbEnable.enable = 0;
    if (HD_OK != (enRet = hd_videoout_set(g_aCtrlIds[u32DevNo], HD_VIDEOOUT_PARAM_FB_ENABLE, &stFbEnable)))
    {
        DISP_LOGE("dev:%u set fb disenable fail:%d\n", u32DevNo, enRet);
        return SAL_FAIL;
    }

    if (HD_OK != (enRet = hd_videoout_close(g_aCtrlIds[u32DevNo])))
    {
        DISP_LOGE("hd_videoout_close fail:%d\n", enRet);
        return SAL_FAIL;
    }

    g_aCtrlIds[u32DevNo] = 0;

    DISP_LOGI("%u delete dev success\n", u32DevNo);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_ntCreateLayer
* 描  述  : 创建视频层
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_ntCreateLayer(VOID *prm)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_ntSetChnPos
* 描  述  : 设置vo参数（放大镜和小窗口）
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntSetChnPos(VOID *prm)
{
    HD_RESULT enRet = HD_OK;
    UINT32 u32Dev = 0;
    UINT32 u32Chn = 0;
    HD_PATH_ID u32Id = 0;
    HD_VIDEOOUT_WIN_ATTR stWin;
    DISP_CHN_COMMON *pDispChn = (DISP_CHN_COMMON *)prm;

    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    u32Dev = pDispChn->uiDevNo;
    u32Chn = pDispChn->uiChn;

    if (0 == g_aOutIds[u32Dev][u32Chn])
    {
        if (HD_OK != (enRet = hd_videoout_open(HD_VIDEOOUT_IN(u32Dev, u32Chn), HD_VIDEOOUT_OUT(u32Dev, 0), &u32Id)))
        {
            DISP_LOGE("hd_videoout_open fail:%d\n", enRet);
            return SAL_FAIL;
        }

        g_aOutIds[u32Dev][u32Chn] = u32Id;
    }

    stWin.rect.x = pDispChn->uiX;
    stWin.rect.y = pDispChn->uiY;
    stWin.rect.w = pDispChn->uiW;
    stWin.rect.h = pDispChn->uiH;
    stWin.visible = 1;
    stWin.layer = HD_LAYER1;
    if (HD_OK != (enRet = hd_videoout_set(g_aOutIds[u32Dev][u32Chn], HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &stWin)))
    {
        DISP_LOGE("dev[%u] chn[%u] set win fail:%d\n", u32Dev, u32Chn, enRet);
        hd_videoout_close(u32Id);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntSetVideoLayerCsc
* 描  述  : 设置视频层图像输出效果
* 输  入  : - uiChn:
*         : - Lua  :
*         : - Con  :
*         : - Hue  :
*         : - Sat  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntSetVideoLayerCsc(UINT32 uiVoLayer, void *prm)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_ntEnableChn
* 描  述  : 使能vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntEnableChn(VOID *prm)
{
    HD_RESULT enRet = HD_OK;
    UINT32 u32Dev = 0;
    UINT32 u32Chn = 0;
    HD_PATH_ID u32Id = 0;
    HD_VIDEOOUT_WIN_ATTR stWin;
    DISP_CHN_COMMON *pDispChn = (DISP_CHN_COMMON *)prm;

    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    if (pDispChn->uiChn != 0)
    {
        DISP_LOGW("win stitch mode only need       to  open chn 0, not need to open chn %u \n", pDispChn->uiChn);
        return SAL_SOK;
    }

    u32Dev = pDispChn->uiDevNo;
    u32Chn = pDispChn->uiChn;

    if (0 == g_aOutIds[u32Dev][u32Chn])
    {
        if (HD_OK != (enRet = hd_videoout_open(HD_VIDEOOUT_IN(u32Dev, u32Chn), HD_VIDEOOUT_OUT(u32Dev, 0), &u32Id)))
        {
            DISP_LOGE("dec:%u chn:%u hd_videoout_open fail:%d\n", u32Dev, u32Chn, enRet);
            return SAL_FAIL;
        }

        DISP_LOGI("dev:%u chn:%u open path:0x%x success\n", u32Dev, u32Chn, u32Id);
        g_aOutIds[u32Dev][u32Chn] = u32Id;
    }
    else
    {
        u32Id = g_aOutIds[u32Dev][u32Chn];
    }

    stWin.rect.x = 0; /* pDispChn->uiX; */
    stWin.rect.y = 0;  /* pDispChn->uiY; */
    stWin.rect.w = 1920;  /* pDispChn->uiW;  //fixme 手动全部拼接成一帧时vo的分辨率要配置成vo输出分辨率 */
    stWin.rect.h = 1080;  /* pDispChn->uiH; */
    stWin.visible = 1;
    stWin.layer = HD_LAYER1;
    if (HD_OK != (enRet = hd_videoout_set(u32Id, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &stWin)))
    {
        DISP_LOGE("dev[%u] chn[%u] set win fail:%d\n", u32Dev, u32Chn, enRet);
        return SAL_FAIL;
    }

    DISP_LOGI("dev:%u chn:%u create success, pos(%u, %u), rect(%uX%u)\n", u32Dev, u32Chn, stWin.rect.x, stWin.rect.y, stWin.rect.w, stWin.rect.h);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntClearVoBuf
* 描  述  : 清空指定输出通道的缓存 buffer 数据
* 输  入  : - uiLayerNo: Vo设备号
*         : - uiVoChn  : Vo通道号
          : - bFlag    : 选择模式
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntClearVoBuf(UINT32 uiVoLayer, UINT32 uiVoChn, UINT32 bFlag)
{
    HD_RESULT enRet = HD_OK;
    HD_VIDEO_FRAME stVideoFrame;
    HD_PATH_ID u32Id = g_aOutIds[uiVoLayer][uiVoChn];
    UINT32 u32TimeOut = 10;

    if (0 == u32Id)
    {
        DISP_LOGE("invalid dev:%u chn:%u\n", uiVoLayer, uiVoChn);
        return SAL_FAIL;
    }

    while (u32TimeOut--)
    {
        enRet = hd_videoout_pull_out_buf(u32Id, &stVideoFrame, 100);
        if (HD_OK != enRet)
        {
            return SAL_SOK;
        }

        enRet = hd_videoout_release_out_buf(u32Id, &stVideoFrame);
        if (HD_OK != enRet)
        {
            DISP_LOGE("dev:%u chn:%u release buff fail\n", uiVoLayer, uiVoChn);
            return SAL_FAIL;
        }
    }

    DISP_LOGE("dev:%u chn:%u clear vo buff fail, please stop push in frame\n", uiVoLayer, uiVoChn);
    return SAL_FAIL;
}

/*******************************************************************************
* 函数名  : disp_ntDisableChn
* 描  述  : 禁止vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntDisableChn(VOID *prm)
{
    HD_RESULT enRet = HD_OK;
    UINT32 u32Dev = 0;
    UINT32 u32Chn = 0;
    DISP_CHN_COMMON *pDispChn = (DISP_CHN_COMMON *)prm;

    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    if (pDispChn->uiChn != 0)
    {
        DISP_LOGW("win stitch mode only need to close chn 0, not need to close chn %u \n", pDispChn->uiChn);
        return SAL_SOK;
    }

    u32Dev = pDispChn->uiDevNo;
    u32Chn = pDispChn->uiChn;

    if (HD_OK != (enRet = hd_videoout_close(g_aOutIds[u32Dev][u32Chn])))
    {
        DISP_LOGE("hd_videoout_close fail:%d, dev %u, chn: %u, outid: %u\n", enRet, u32Dev, u32Chn, g_aOutIds[u32Dev][u32Chn]);
        return SAL_FAIL;
    }

    g_aOutIds[u32Dev][u32Chn] = 0;

    DISP_LOGI("dev:%u chn:%u delete success\n", u32Dev, u32Chn);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntStartChn
* 描  述  : vo开始显示
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntStartChn(VOID *prm)
{
    HD_RESULT enRet = HD_OK;
    UINT32 u32Dev = 0;
    UINT32 u32Chn = 0;
    DISP_CHN_COMMON *pDispChn = (DISP_CHN_COMMON *)prm;

    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    if (pDispChn->uiChn != 0)
    {
        DISP_LOGW("win stitch mode only nend to start chn 0, not need to start chn %u \n", pDispChn->uiChn);
        return SAL_SOK;
    }

    u32Dev = pDispChn->uiDevNo;
    u32Chn = pDispChn->uiChn;

    if (HD_OK != (enRet = hd_videoout_start(g_aOutIds[u32Dev][u32Chn])))
    {
        DISP_LOGE("dev:%u chn:%u hd_videoout_start fail:%d\n", u32Dev, u32Chn, enRet);
        return SAL_FAIL;
    }

    DISP_LOGI("dev:%u chn:%u start\n", u32Dev, u32Chn);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntStopChn
* 描  述  : vo停止显示
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntStopChn(VOID *prm)
{
    HD_RESULT enRet = HD_OK;
    UINT32 u32Dev = 0;
    UINT32 u32Chn = 0;
    DISP_CHN_COMMON *pDispChn = (DISP_CHN_COMMON *)prm;

    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    if (pDispChn->uiChn != 0)
    {
        DISP_LOGW("win stitch mode only nend to        stop chn 0, not need to stop chn %u \n", pDispChn->uiChn);
        return SAL_SOK;
    }

    u32Dev = pDispChn->uiDevNo;
    u32Chn = pDispChn->uiChn;

    if (HD_OK != (enRet = hd_videoout_stop(g_aOutIds[u32Dev][u32Chn])))
    {
        DISP_LOGE("dev:%u chn:%u hd_videoout_stop fail:%d\n", u32Dev, u32Chn, enRet);
        return SAL_FAIL;
    }

    DISP_LOGI("dev:%u chn:%u stop\n", u32Dev, u32Chn);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_ntDeinitStartingup
* 描  述  : 开机时显示反初始化
* 输  入  : - uiDev  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntDeinitStartingup(UINT32 uiDev)
{
    if (HD_OK != hd_videoout_uninit())
    {
        DISP_LOGE("hd_videoout_uninit fail\n");
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntPutFrameInfo
* 描  述  : 将数据帧赋值给显示帧
* 输  入  : - prm        :
*         : - pFrame     :
*         : - s32MilliSec:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntPutFrameInfo(SAL_VideoFrameBuf *videoFrame, VOID *pFrame)
{
    HD_VIDEO_FRAME *pstVideoFrame = (HD_VIDEO_FRAME *)pFrame;;

    UINT32 u32Width = 0;
    UINT32 u32Height = 0;
    VOID *pvVirSrc = NULL;
    VOID *pvVirDst = NULL;

    if ((NULL == videoFrame) || (NULL == pFrame))
    {
        DISP_LOGE("null pointer\n");
        return SAL_FAIL;
    }

    u32Width = SAL_align(videoFrame->frameParam.width, 16);
    u32Height = SAL_align(videoFrame->frameParam.height, 2);

    pvVirDst = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, pstVideoFrame->phy_addr[0], u32Width * u32Height * 3 / 2);
    if (NULL == pvVirDst)
    {
        DISP_LOGE("map phy addr[0x%lx] fail\n", pstVideoFrame->phy_addr[0]);
        return SAL_FAIL;
    }

    pvVirSrc = (VOID *)(videoFrame->virAddr[0]);
    memcpy(pvVirDst, pvVirSrc, u32Width * u32Height * 3 / 2);

    pstVideoFrame->dim.w = videoFrame->frameParam.width;
    pstVideoFrame->dim.h = videoFrame->frameParam.height;

    hd_common_mem_munmap(pvVirDst, u32Width * u32Height * 3 / 2);

    return SAL_SOK;
}

#define BACKTRACE_SIZE 100

/**
 * @function    print_backtrace
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
void print_backtrace()
{
    void *buffer[BACKTRACE_SIZE] = {0};
    int pointer_num = backtrace(buffer, BACKTRACE_SIZE);
    char **string_buffer = backtrace_symbols(buffer, pointer_num);

    if (string_buffer == NULL)
    {
        printf("backtrace_symbols error");
        exit(-1);
    }

    printf("print backtrace begin\n");
    for (int i = 0; i < pointer_num; i++)
    {
        printf("%s\n", string_buffer[i]);
    }

    printf("print backtrace end\n");

    free(string_buffer);

    return;
}

/*******************************************************************************
* 函数名  : disp_ntSendFrame
* 描  述  : 将数据送至vo通道
* 输  入  : - prm        :
*         : - pFrame     :
*         : - s32MilliSec:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntSendFrame(VOID *prm, VOID *pFrame, INT32 s32MilliSec)
{
    HD_RESULT enRet = HD_OK;
    DISP_CHN_COMMON *pDispChn = (DISP_CHN_COMMON *)prm;
    HD_VIDEO_FRAME *pstVideoFrame = (HD_VIDEO_FRAME *)pFrame;
    UINT32 u32Dev = 0;
    UINT32 u32Chn = 0;

    if (NULL == pDispChn || NULL == pstVideoFrame)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    u32Dev = pDispChn->uiDevNo;
    u32Chn = pDispChn->uiChn;

    enRet = vendor_videoout_push_in_buf(g_aOutIds[u32Dev][u32Chn], pstVideoFrame, NULL, 100);
    /* enRet = hd_videoout_push_in_buf(g_aOutIds[u32Dev][u32Chn], pstVideoFrame, NULL, 100); */
    if (HD_OK != enRet)
    {
        DISP_LOGW("dev[%u] chn[%u] hd_videoout_push_in_buf fail:%d\n", u32Dev, u32Chn, enRet);
        DISP_LOGW("dev[%u] chn[%u] will clean buff\n", u32Dev, u32Chn);
        (VOID)disp_ntClearVoBuf(u32Dev, u32Chn, SAL_TRUE);

        enRet = hd_videoout_push_in_buf(g_aOutIds[u32Dev][u32Chn], pstVideoFrame, NULL, 100);
        if (HD_OK != enRet)
        {
            DISP_LOGE("dev[%u] chn[%u] hd_videoout_push_in_buf fail:%d\n", u32Dev, u32Chn, enRet);
            return SAL_SOK;
        }
    }

    return SAL_SOK;
}

/**
 * @function    disp_ntPullOutFrame
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 disp_ntPullOutFrame(VOID *prm, VOID *pFrame, INT32 s32MilliSec)
{
    HD_RESULT enRet = HD_OK;
    DISP_CHN_COMMON *pDispChn = (DISP_CHN_COMMON *)prm;
    HD_VIDEO_FRAME *pstVideoFrame = (HD_VIDEO_FRAME *)pFrame;
    UINT32 u32Dev = 0;
    UINT32 u32Chn = 0;

    if (NULL == pDispChn || NULL == pstVideoFrame)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    u32Dev = pDispChn->uiDevNo;
    u32Chn = pDispChn->uiChn;

    /* 通过vendor_videoout_pull_out_buf接口确保底层没在使用该buffer，否则可能会产生底层使用中的buff被覆盖导致图像撕裂 */
    enRet = vendor_videoout_pull_out_buf(g_aOutIds[u32Dev][u32Chn], pstVideoFrame, s32MilliSec);
    if (HD_OK != enRet)
    {
        DISP_LOGW("dev[%u] chn[%u] vendor_videoout_pull_out_buf fail:%d\n", u32Dev, u32Chn, enRet);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntGetVoChnFrame
* 描  述  : 获取vo输出帧数据
* 输  入  : - pDispChn:
*         : - Ctrl    :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntGetVoChnFrame(UINT32 VoLayer, UINT32 VoChn, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 s32Ret = SAL_SOK;
    HD_RESULT enRet = HD_OK;
    HD_VIDEO_FRAME stVideoFrame;
    HD_VIDEO_FRAME *pstVideoFrame = NULL;
    UINT32 u32Size = 0;
    VOID *pvVirSrc = NULL;
    VOID *pvVirDst = NULL;

    enRet = hd_videoout_pull_out_buf(g_aOutIds[VoLayer][VoChn], &stVideoFrame, 100);
    if (HD_OK != enRet)
    {
        DISP_LOGE("get frame from dev[%u] chn[%u] fail:%d\n", VoLayer, VoChn, enRet);
        return SAL_FAIL;
    }

    u32Size = SAL_align(stVideoFrame.dim.w, 16) * SAL_align(stVideoFrame.dim.h, 2) * 3 / 2;
    pvVirSrc = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, stVideoFrame.phy_addr[0], u32Size);
    if (NULL == pvVirSrc)
    {
        DISP_LOGE("map phy addr[0x%lx] size[%u] fail\n", stVideoFrame.phy_addr[0], u32Size);
        s32Ret = SAL_FAIL;
        goto exit1;
    }

    pstVideoFrame = (HD_VIDEO_FRAME *)pstFrame->uiAppData;
    pvVirDst = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, pstVideoFrame->phy_addr[0], u32Size);
    if (NULL == pvVirDst)
    {
        DISP_LOGE("map phy addr[0x%lx] size[%u] fail\n", stVideoFrame.phy_addr[0], u32Size);
        s32Ret = SAL_FAIL;
        goto exit2;
    }

    memcpy(pvVirDst, pvVirSrc, u32Size);

exit2:
    hd_common_mem_munmap(pvVirSrc, u32Size);

exit1:
    hd_videoout_release_out_buf(g_aOutIds[VoLayer][VoChn], &stVideoFrame);
    return s32Ret;
}

/*******************************************************************************
* 函数名  : disp_ntSetVoInterface
* 描  述  :
* 输  入  : DISP_DEV_COMMON *pDispDev
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntSetVoInterface(DISP_DEV_COMMON *pDispDev)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntGetHdmiEdid
* 描  述  :
* 输  入  : UINT32 u32HdmiId
            UINT8 *pu8Buff
            UINT32 *pu32Len
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntGetHdmiEdid(UINT32 u32HdmiId, UINT8 *pu8Buff, UINT32 *pu32Len)
{
    HD_RESULT enRet = HD_OK;
    HD_VIDEOOUT_EDID_DATA stEdid;

    stEdid.p_video_id = NULL;
    stEdid.video_id_len = 0;
    stEdid.p_edid_data = pu8Buff;
    stEdid.edid_data_len = 256;
    stEdid.interface_id = HD_COMMON_VIDEO_OUT_HDMI;

    if (HD_OK != (enRet = hd_videoout_get(g_aCtrlIds[0], HD_VIDEOOUT_PARAM_EDID_DATA, &stEdid)))
    {
        DISP_LOGE("get edid fail:%d\n", enRet);
        return SAL_FAIL;
    }

    *pu32Len = (pu8Buff[0x7E] + 1) * 128;
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntSetVoChnPriority
* 描  述  : 设置窗口显示优先级
* 输  入  : - uiDev:
*         : - prm  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntSetVoChnPriority(VOID *prm)
{
    HD_RESULT enRet = HD_OK;
    UINT32 u32Dev = 0;
    UINT32 u32Chn = 0;
    HD_VIDEOOUT_WIN_ATTR stWin;
    DISP_CHN_COMMON *pDispChn = (DISP_CHN_COMMON *)prm;

    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    if (pDispChn->uiChn != 0)
    {
        DISP_LOGD("win stitch mode only nend to        stop chn 0, not need to stop chn %u \n", pDispChn->uiChn);
        return SAL_SOK;
    }

    u32Dev = pDispChn->uiDevNo;
    u32Chn = pDispChn->uiChn;

    if (HD_OK != (enRet = hd_videoout_get(g_aOutIds[u32Dev][u32Chn], HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &stWin)))
    {
        DISP_LOGE("dev[%u] chn[%u] set win fail:%d\n", u32Dev, u32Chn, enRet);
        return SAL_FAIL;
    }

    stWin.layer = pDispChn->u32Priority;
    if (HD_OK != (enRet = hd_videoout_set(g_aOutIds[u32Dev][u32Chn], HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &stWin)))
    {
        DISP_LOGE("dev[%u] chn[%u] set win fail:%d\n", u32Dev, u32Chn, enRet);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_free_buffer
* 描  述  : 释放分配的物理内存
* 输  入  : - p_buf:
* 输  出  :
* 返回值  : HD_OK  : 成功
*            : 失败
*******************************************************************************/
static HD_RESULT disp_free_buffer(app_buffer_t *p_buf)
{
    HD_RESULT hd_ret = SAL_SOK;

    if (NULL != p_buf->storage)
    {
        hd_ret = hd_common_mem_munmap(p_buf->storage, p_buf->size);
        if (hd_ret != SAL_SOK)
        {
            DISP_LOGE("hd_common_mem_munmap fail\r\n");
            return hd_ret;
        }

        p_buf->storage = NULL;
    }

    if (HD_COMMON_MEM_VB_INVALID_BLK != p_buf->blk)
    {
        hd_ret = hd_common_mem_release_block(p_buf->blk);
        if (hd_ret != SAL_SOK)
        {
            DISP_LOGE("hd_common_mem_release_block fail\r\n");
            return hd_ret;
        }

        p_buf->blk = HD_COMMON_MEM_VB_INVALID_BLK;
    }

    p_buf->phy_addr = 0;
    return hd_ret;
}

/*******************************************************************************
* 函数名  : disp_ntGetwWitebackFmt
* 描  述  : 视频格式枚举变量进行转化
* 输  入  : - fmt:
* 输  出  : value
* 返回值  : HD_OK  : 成功
*            : 失败
*******************************************************************************/

static HD_RESULT disp_ntGetwWitebackFmt(HD_VIDEO_PXLFMT fmt, int *value)
{

    if (value == SAL_SOK)
    {
        return HD_ERR_SYS;
    }

    switch (fmt)
    {
        case HD_VIDEO_PXLFMT_YUV422:
        case HD_VIDEO_PXLFMT_YUV422_ONE:
            *value = 0;;
            break;
        case HD_VIDEO_PXLFMT_YUV420:
            *value = 1;
            break;
        case HD_VIDEO_PXLFMT_YUV420_NVX3:
            *value = 2;
            break;
        default:
            return HD_ERR_SYS;
            break;
    }

    return HD_OK;
}

/*******************************************************************************
* 函数名  : disp_ntgetDispWbc
* 描  述  : 视频层回写操作
* 输  入  : - VoWbc:
* 输  出  : pstVFrame
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_ntgetDispWbc(UINT32 VoWbc, VO_FRAME_INFO_ST *pstVFrame)
{
    int ddr_id;
    HD_RESULT ret;
    int wb_fmt = 0;
    int out_id = 0;
    VOID *storage;
    UINTPTR phy_addr;
    static char index = 1;
    app_buffer_t wb_vga_buf;
    HD_COMMON_MEM_VB_BLK blk;
    HD_VIDEO_FRAME pout_buffer;
    unsigned int frame_buf_size;
    TDE_HAL_RECT srcRect = {0};
    TDE_HAL_RECT dstRect = {0};
    TDE_HAL_SURFACE srcSurface = {0};
    TDE_HAL_SURFACE dstSurface = {0};
    VIDEO_LIVEVIEW liveview_info = {0};
    HD_COMMON_MEM_POOL_INFO mem_info = {0};
    VENDOR_VIDEOOUT_PUSHTO videoout_wb = {0};
    VENDOR_VIDEOOUT_PUSHTO videoout_get = {0};
    static ALLOC_VB_INFO_S stAllocVbInfo = {0};

    /*获取输出图像的参数，图像格式  宽  高  地址号*/
    if ((ret = hd_videoout_open(g_aCtrlIds[VoWbc], HD_VIDEOOUT_0_CTRL, &liveview_info.videoout_ctrl_path)) != HD_OK) /* open this for device control */
    {
        DISP_LOGE("hd_videoout_open error:%d\n", ret);
        return ret;
    }

    ret = hd_videoout_get(liveview_info.videoout_ctrl_path, HD_VIDEOOUT_PARAM_SYSCAPS, &liveview_info.videoout_syscaps);
    if (ret != SAL_SOK)
    {
        DISP_LOGE("hd_videoout_get0:param_id(%d) video_out_ctrl(%#x) fail\n", HD_VIDEOOUT_PARAM_SYSCAPS, liveview_info.videoout_2nd_ctrl_path);
        return ret;
    }

    ret = hd_videoout_get(liveview_info.videoout_ctrl_path, HD_VIDEOOUT_PARAM_FB_FMT, &liveview_info.videoout_2nd_fb);
    if (ret != SAL_SOK)
    {
        DISP_LOGE("hd_videoout_get1:param_id(%d) video_out_ctrl(%#x) fail\n", HD_VIDEOOUT_PARAM_FB_FMT, liveview_info.videoout_2nd_ctrl_path);
        return ret;
    }

    ret = disp_ntGetwWitebackFmt(liveview_info.videoout_2nd_fb.fmt, &wb_fmt);
    if (ret != SAL_SOK)
    {
        DISP_LOGE("TEST_FMT invalid\n");
        return ret;
    }

    /*获取地址id号*/
    for (out_id = 0; out_id < DDR_ID_MAX; out_id++)
    {
        mem_info.type = HD_COMMON_MEM_ENC_SCL_OUT_POOL;
        mem_info.ddr_id = out_id;
        ret = hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, (VOID *)&mem_info);
        if (ret == SAL_SOK)
        {
            DISP_LOGD("get pool(%d) ddrid(%d) pool_sz(%d)\n", HD_COMMON_MEM_ENC_SCL_OUT_POOL, out_id, mem_info.blk_size);
            break;
        }
        else if (ret == HD_ERR_NG)
        {
            /* DISP_LOGE("no this blk,  type = %d, ddr_id = %d.\n", type, out_id); */
            continue;
        }
        else
        {
            /* DISP_LOGE("other error,  type = %d, ddr_id = %d.\n", type, out_id); */
            continue;
        }
    }

    if (out_id == DDR_ID_MAX)
    {
        DISP_LOGE("dts not config pool(%d) size\n", HD_COMMON_MEM_ENC_SCL_OUT_POOL);
        return SAL_FAIL;
    }

    ddr_id = out_id;

    /*设置对应参数，用于计算存储回显数据所需大小并分配对应内存，将事先分配好的内存送入VO用于存放回显数据*/
    pout_buffer.pxlfmt = liveview_info.videoout_2nd_fb.fmt;;
    pout_buffer.sign = 0;  /* p_vcap_info->sign; */
    pout_buffer.dim.w = liveview_info.videoout_syscaps.output_dim.w;
    pout_buffer.dim.h = liveview_info.videoout_syscaps.output_dim.h;
    pout_buffer.ddr_id = ddr_id;

    /*计算存放回显数据所需内存的大小*/
    frame_buf_size = hd_common_mem_calc_buf_size(&pout_buffer);

    /*分配内存，送入到vo中，硬件会将回写数据写入该内存中*/
    blk = hd_common_mem_get_block(HD_COMMON_MEM_ENC_SCL_OUT_POOL, frame_buf_size, ddr_id);
    if (HD_COMMON_MEM_VB_INVALID_BLK == blk)
    {
        DISP_LOGE("hd_common_mem_get_block fail\r\n");
        return SAL_FAIL;
    }

    phy_addr = hd_common_mem_blk2pa(blk);
    if (SAL_SOK == phy_addr)
    {
        DISP_LOGE("hd_common_mem_blk2pa fail, blk = %ld\r\n", blk);
        hd_common_mem_release_block(blk);
        return SAL_FAIL;
    }

    wb_vga_buf.storage = 0;
    if (ddr_id < 2)
    {
        storage = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE, phy_addr, frame_buf_size);
        if (NULL == storage)
        {
            DISP_LOGE("hd_common_mem_mmap fail, pa = 0x%lX\r\n", phy_addr);
            hd_common_mem_release_block(blk);
            return SAL_FAIL;
        }

        wb_vga_buf.storage = storage;
    }

    wb_vga_buf.blk = blk;
    wb_vga_buf.size = frame_buf_size;
    wb_vga_buf.phy_addr = phy_addr;          /* 将物理地址放入wb_vga_buf */

    /*保存图像信息，并为图像分配内存,与上面分配内存不同，上面的是送给硬件用于保存视频源，该内存用于保存获取的数据，返回给上层应用*/
    if (index == 1)
    {
        index = SAL_SOK;
        /* DISP_LOGI("**************mem_hal_vbAlloc*****************\n"); */
        ret = mem_hal_vbAlloc(pout_buffer.dim.w * pout_buffer.dim.h * 4, "DISP", "disp_nt", NULL, 1, &stAllocVbInfo);
        if (SAL_SOK != ret)
        {
            DISP_LOGE("mem_hal_vbAlloc error! \n");
            disp_free_buffer(&wb_vga_buf);
            index = 1;
            return ret;
        }
    }

    /*设置回写参数*/
    videoout_wb.enabled = 1;
    videoout_wb.wb_width = liveview_info.videoout_syscaps.output_dim.w;
    videoout_wb.wb_height = liveview_info.videoout_syscaps.output_dim.h;
    videoout_wb.oneshut = 0;
    videoout_wb.fmt = wb_fmt;        /* 0: 422, 1: 420, 2: 420SCE */
    videoout_wb.out_pa = wb_vga_buf.phy_addr;
    /*将内存送入vo，硬件上会将一帧源输出图相保存到分配的内存中*/
    ret = vendor_videoout_set(g_aOutIds[VoWbc][0], VENDOR_VIDEOOUT_PARAM_WB_PUSH, &videoout_wb);
    if (ret != SAL_SOK)
    {
        DISP_LOGE("vendor_videoout_set fail\n");
        disp_free_buffer(&wb_vga_buf);
        return SAL_FAIL;
    }

    /*获取回写数据，此处获取出来的数据就是VENDOR_VIDEOOUT_PARAM_WB_PUSH指令送入的内存中的数据，其返回的物理地址和上面送入的是同一块地址*/
    ret = vendor_videoout_set(g_aOutIds[VoWbc][0], VENDOR_VIDEOOUT_PARAM_WB_PULL, &videoout_get);
    if (ret != SAL_SOK)
    {
        DISP_LOGE("VENDOR_VIDEOOUT_PARAM_PULL fail  lasting\n");
        disp_free_buffer(&wb_vga_buf);
        return SAL_FAIL;

    }
    /*应用需要的是BGRA格式，该处传输的yuv420,所以该处代码未使用*/
    #if 0
    /*将获取的数据放入结构体中，以yuv格式输出到上层应用*/
    FILE *fp = NULL; 
    pstVFrame->pVirAddr = stAllocVbInfo.pVirAddr;
    pstVFrame->height = videoout_wb.wb_height;
    pstVFrame->width = videoout_wb.wb_width;
    pstVFrame->stride = videoout_wb.wb_width * 3 / 2;
    pstVFrame->dataFormat = DSP_IMG_DATFMT_YUV420P;
    
    if ((fp = fopen("/mnt/wbc.yuv", "wb")) == NULL)
    {
        printf("file open fail\r\n");
        return SAL_FAIL;
    }
    if (fwrite((char *)(wb_vga_buf.storage), wb_vga_buf.size, 1, fp) != 1)
        printf("file write error\n");
    fclose(fp);
     #endif

    /*nt获取的回写数据为yuv，将其转化为BGRA数据传给应用层*/
    srcSurface.u32Width = videoout_wb.wb_width;
    srcSurface.u32Height = videoout_wb.wb_height;
    srcSurface.u32Stride = videoout_wb.wb_width;
    srcSurface.enColorFmt = SAL_VIDEO_DATFMT_YUV420P;
    srcSurface.PhyAddr = wb_vga_buf.phy_addr;
    srcSurface.vbBlk = wb_vga_buf.blk;
    srcRect.s32Xpos = 0;
    srcRect.s32Ypos = 0;
    srcRect.u32Width = videoout_wb.wb_width;
    srcRect.u32Height = videoout_wb.wb_height;

    dstSurface.u32Width = videoout_wb.wb_width;
    dstSurface.u32Height = videoout_wb.wb_height;
    dstSurface.u32Stride = videoout_wb.wb_width;
    dstSurface.enColorFmt = SAL_VIDEO_DATFMT_BGRA_8888;
    dstSurface.PhyAddr = stAllocVbInfo.u64PhysAddr;
    dstSurface.vbBlk = stAllocVbInfo.u64VbBlk;
    dstRect.s32Xpos = 0;
    dstRect.s32Ypos = 0;
    dstRect.u32Width = videoout_wb.wb_width;
    dstRect.u32Height = videoout_wb.wb_height;

    ret = tde_hal_QuickCopy(&srcSurface, &srcRect, &dstSurface, &dstRect, SAL_FALSE);
    if (ret != SAL_SOK)
    {
        SAL_LOGE("err %#x\n", ret);
        disp_free_buffer(&wb_vga_buf);
        return SAL_FAIL;
    }
    /*将转化成BGRA的数据存入结构体中，传输到上层*/
    pstVFrame->pVirAddr = stAllocVbInfo.pVirAddr;
    pstVFrame->height = videoout_wb.wb_height;
    pstVFrame->width = videoout_wb.wb_width;
    pstVFrame->stride = videoout_wb.wb_width * 4;
    pstVFrame->dataFormat = DSP_IMG_DATFMT_BGRA32;
    /*在函数内直接将回写数据保存成文件，方便测试时排除接口调用错误导致的误判*/
    #if 0
    FILE *fp = NULL; 
    char    sztempTri[128] = {0};
    static INT32 dump_jpegnum[2] = {0};
    
    sprintf(sztempTri, "/mnt/nfs/wbc%d_%d.argb", VoWbc, dump_jpegnum[VoWbc]);
    if ((fp = fopen(sztempTri, "wb")) == NULL)
    {
        printf("file open fail\r\n");
        return SAL_FAIL;
    }
    if (fwrite((char *)(stAllocVbInfo.pVirAddr), stAllocVbInfo.u32Size, 1, fp) != 1)
        printf("file write error\n");
    dump_jpegnum[VoWbc]++;
    if(dump_jpegnum[VoWbc]>100) 
        { 
            dump_jpegnum[VoWbc]=0;
            DISP_LOGE("***********100 picture*******8");
    }
    fclose(fp);
    #endif
    /*释放分配内存*/
    disp_free_buffer(&wb_vga_buf);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_ntenableDispWbc
* 描  述  : nt平台不需要使能wbc,该函数定义为了使应用保持与RK平台一致的调用方法
* 输  入  : VoWbc  *pParam
*         :
* 输  出  :
* 返回值  : SAL_SOK  : 成功
*           : 失败
*******************************************************************************/
static INT32 disp_ntenableDispWbc(UINT32 VoWbc, unsigned int *pParam)
{
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_halRegister
* 描  述  : 注册nt disp显示函数
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
DISP_PLAT_OPS *disp_halRegister(void)
{
    memset(&g_stDispPlatOps, 0, sizeof(DISP_PLAT_OPS));

    g_stDispPlatOps.modInit = disp_ntModInit;
    g_stDispPlatOps.createVoDev = disp_ntCreateDev;
    g_stDispPlatOps.deleteVoDev = disp_ntDeleteDev;
    g_stDispPlatOps.createVoLayer = disp_ntCreateLayer;
    g_stDispPlatOps.setVoChnParam = disp_ntSetChnPos;
    g_stDispPlatOps.setVoLayerCsc = disp_ntSetVideoLayerCsc;
    g_stDispPlatOps.enableVoChn = disp_ntEnableChn;
    g_stDispPlatOps.clearVoChnBuf = disp_ntClearVoBuf;
    g_stDispPlatOps.disableVoChn = disp_ntDisableChn;
    g_stDispPlatOps.startVoChn = disp_ntStartChn,
    g_stDispPlatOps.stopVoChn = disp_ntStopChn,
    g_stDispPlatOps.deinitVoStartingup = disp_ntDeinitStartingup;
    g_stDispPlatOps.putVoFrameInfo = disp_ntPutFrameInfo;
    g_stDispPlatOps.sendVoChnFrame = disp_ntSendFrame;
    g_stDispPlatOps.pullOutFrame = disp_ntPullOutFrame;
    g_stDispPlatOps.getVoChnFrame = disp_ntGetVoChnFrame;
    g_stDispPlatOps.setVoInterface = disp_ntSetVoInterface;
    g_stDispPlatOps.getHdmiEdid = disp_ntGetHdmiEdid;
    g_stDispPlatOps.setVoChnPriority = disp_ntSetVoChnPriority;
    g_stDispPlatOps.getDispWbc = disp_ntgetDispWbc;
    g_stDispPlatOps.enableDispWbc = disp_ntenableDispWbc;

    return &g_stDispPlatOps;
}

