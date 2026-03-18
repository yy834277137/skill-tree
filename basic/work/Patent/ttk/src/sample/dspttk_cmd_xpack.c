/**
 * @file    dspttk_cmd_xpack.c
 * @brief
 * @note
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <math.h>
#include <dirent.h>  /* 包含文件夹扫描函数的定义 */
#include <sys/stat.h>
#include <fcntl.h>
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_util.h"
#include "dspttk_fop.h"
#include "dspttk_cmd.h"
#include "dspttk_init.h"
#include "dspttk_devcfg.h"
#include "dspttk_cmd_xpack.h"
#include "dspttk_callback.h"


/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */

static DSPTTK_XPACK_SAVE_PRM gstXpackSaveInfo= {0};


/**
 * @fn      dspttk_get_save_screen_list_info
 * @brief   获取save图个数
 * 
 * @param   [IN] u32Chan XRay通道号
 * 
 * @return  [OUT] save信息结构体 
 */
DSPTTK_XPACK_SAVE_PRM *dspttk_get_save_screen_list_info(UINT32 u32Chan)
{
    return &gstXpackSaveInfo;
}

/**
 * @function   dspttk_xpack_set_segment_attr
 * @brief      设置集中判图分片参数
 * @param[in]  unsigned int u32Chan
 * @param[in]  XPACK_DSP_SEGMENT_OUT *pstPackRst
 * @param[out] None
 * @return     UINT64
 */
UINT64 dspttk_xpack_set_segment_attr(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XPACK *pstCfgXpack = &pstDevCfg->stSettingParam.stXpack;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        if (SAL_SOK != SendCmdToDsp(HOST_CMD_XPACK_SET_SEGMENT, u32Chan, NULL, &pstCfgXpack->stSegmentSet))
        {
            PRT_INFO("error HOST_CMD_XPACK_SET_SEGMENT:u32Chan %d u32Width %u SegSwitch %u type %u set error\n", 
                u32Chan, pstCfgXpack->stSegmentSet.SegmentWidth, 
                pstCfgXpack->stSegmentSet.bSegFlag, 
                pstCfgXpack->stSegmentSet.SegmentDataTpyeTpye);
            sRet = SAL_FAIL;
        }
        else
        {
            PRT_INFO("set [%d] HOST_CMD_XPACK_SET_SEGMENT successful ,u32Width %u SegSwitch %u type %u\n", 
                u32Chan, pstCfgXpack->stSegmentSet.SegmentWidth, 
                pstCfgXpack->stSegmentSet.bSegFlag, 
                pstCfgXpack->stSegmentSet.SegmentDataTpyeTpye);
        }
    }

    return CMD_RET_MIX(HOST_CMD_XPACK_SET_SEGMENT, sRet);
}


/**
 * @fn      dspttk_xpack_set_yuv_offset
 * @brief   设置设置yuv显示偏移量
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参， 
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xpack_set_yuv_offset(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XPACK *pstCfgXpack = &pstDevCfg->stSettingParam.stXpack;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        if (SendCmdToDsp(HOST_CMD_XPACK_SET_YUVSHOW_OFFSET, u32Chan, NULL, (void *)&pstCfgXpack->stXpackYuvOffset) != SAL_SOK)
        {
            PRT_INFO("ERROR chan[%d] HOST_CMD_XPACK_SET_YUVSHOW_OFFSET reset %d offset %d\n",u32Chan,
                    pstCfgXpack->stXpackYuvOffset.reset, pstCfgXpack->stXpackYuvOffset.offset);
            sRet = SAL_FAIL;
        }
        else
        {
            PRT_INFO("set [%d] HOST_CMD_XPACK_SET_YUVSHOW_OFFSET successful  reset %d offset %d\n",u32Chan,
                            pstCfgXpack->stXpackYuvOffset.reset, pstCfgXpack->stXpackYuvOffset.offset);
        }

    }

    return CMD_RET_MIX(HOST_CMD_XPACK_SET_YUVSHOW_OFFSET, sRet);
}


/**
 * @fn      dspttk_xpack_set_jpg
 * @brief   设置JPG抓图参数
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参， 
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xpack_set_jpg(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XPACK *pstCfgXpack = &pstDevCfg->stSettingParam.stXpack;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    
    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        if ( SendCmdToDsp(HOST_CMD_XPACK_SET_JPG, u32Chan, NULL, (void *)&pstCfgXpack->stXpackJpgSet) != SAL_SOK)
        {
            PRT_INFO("ERROR HOST_CMD_XPACK_SET_JPG BackW:%d, BackH:%d, DrawSvaSwitch:%d, bJpgCropSwitch:%u, RatioW:%f, Ratio:%f\n", 
                pstCfgXpack->stXpackJpgSet.u32JpgBackWidth, pstCfgXpack->stXpackJpgSet.u32JpgBackHeight, pstCfgXpack->stXpackJpgSet.bJpgDrawSwitch, pstCfgXpack->stXpackJpgSet.bJpgCropSwitch, 
                pstCfgXpack->stXpackJpgSet.f32WidthResizeRatio, pstCfgXpack->stXpackJpgSet.f32HeightResizeRatio);
            sRet = SAL_FAIL;
        }
        else
        {
            PRT_INFO("set chn[%d] HOST_CMD_XPACK_SET_JPG successful. BackW:%d, BackH:%d, DrawSvaSwitch:%d, bJpgCropSwitch:%u, RatioW:%f, Ratio:%f\n", u32Chan,
                pstCfgXpack->stXpackJpgSet.u32JpgBackWidth, pstCfgXpack->stXpackJpgSet.u32JpgBackHeight, pstCfgXpack->stXpackJpgSet.bJpgDrawSwitch, pstCfgXpack->stXpackJpgSet.bJpgCropSwitch, 
                pstCfgXpack->stXpackJpgSet.f32WidthResizeRatio, pstCfgXpack->stXpackJpgSet.f32HeightResizeRatio);
        }
    }

    return CMD_RET_MIX(HOST_CMD_XPACK_SET_JPG, sRet);
}


/**
 * @fn      dspttk_xpack_set_disp_sync_time
 * @brief   设置同步显示超时时间
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参， 
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xpack_set_disp_sync_time(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XPACK *pstCfgXpack = &pstDevCfg->stSettingParam.stXpack;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    
    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        if ( SendCmdToDsp(HOST_CMD_SET_DISP_SYNC_TIME, u32Chan, (void *)&pstCfgXpack->u32DispSyncTimeSet, NULL) != SAL_SOK)
        {
            PRT_INFO("ERROR HOST_CMD_SET_DISP_SYNC_TIME sync time [%u]", pstCfgXpack->u32DispSyncTimeSet);
            sRet = SAL_FAIL;
        }
        else
        {
            PRT_INFO("set HOST_CMD_SET_DISP_SYNC_TIME sync time [%u] successful.\n", pstCfgXpack->u32DispSyncTimeSet);
        }
    }

    return CMD_RET_MIX(HOST_CMD_SET_DISP_SYNC_TIME, sRet);
}


/**
 * @fn      dspttk_xpack_save_screen
 * @brief   SAVE，保存当前屏幕上的图像数据
 *
 * @param   [IN] u32Chan 无效参数，实际不区分通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xpack_save_screen(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK, i = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    XPACK_SAVE_PRM stSaveParam = {0};
    VOID *pSaveBuf = 0;
    UINT32 u32SaveRes = 0, u32BufSize = 0;
    CHAR sImgFmt[XPACK_SAVE_BUTT][8] = {"nraw", "bmp", "jpg"}; // 图像文件后缀
    CHAR sFileName[256] = {0};
    UINT64 u64CurrentTime = dspttk_get_clock_ms();
    DSPTTK_XPACK_SAVE_PRM *pstSaveInfo = &gstXpackSaveInfo;

    u32SaveRes = pstDevCfg->stInitParam.stXray.xrayWidthMax + pstDevCfg->stInitParam.stDisplay.paddingTop + pstDevCfg->stInitParam.stDisplay.paddingBottom;
    u32SaveRes = (UINT32)(SAL_MAX(pstDevCfg->stInitParam.stXray.resizeFactor, 1.0) * SAL_MAX(pstDevCfg->stInitParam.stXray.resizeFactor, 1.0) *
                          u32SaveRes * pstDevCfg->stInitParam.stXray.xrayHeightMax);
    u32BufSize = u32SaveRes * 6; // 6: 以XRAYLIB_IMG_RAW_CALI_LHZ16格式为最大数据量
    pSaveBuf = malloc(u32BufSize);
    if (NULL == pSaveBuf)
    {
        PRT_INFO("malloc for 'pSaveBuf' failed, size: %u\n", u32BufSize);
        return CMD_RET_MIX(HOST_CMD_XPACK_SAVE, SAL_FAIL);
    }
    for (u32Chan = 0; u32Chan < pstDevCfg->stInitParam.stXray.xrayChanCnt; u32Chan++)
    {
        for (i = 0; i < XPACK_SAVE_BUTT; i++)
        {
            stSaveParam.outType = i;
            stSaveParam.pOutBuff = pSaveBuf;
            stSaveParam.u32BuffSize = u32BufSize;
            stSaveParam.u32DataSize = 0;
            if (SAL_SOK == SendCmdToDsp(HOST_CMD_XPACK_SAVE, u32Chan, NULL, &stSaveParam) && 
                                        stSaveParam.u32DataSize > 0 && stSaveParam.u32DataSize <= u32BufSize)
            {
                if (strlen(pstDevCfg->stSettingParam.stEnv.stOutputDir.saveScreen) > 0)
                {
                    snprintf(sFileName, sizeof(sFileName), "%s/%u/save_no%u_w%u_h%u.%s", pstDevCfg->stSettingParam.stEnv.stOutputDir.saveScreen, 
                             u32Chan, pstSaveInfo->u32SaveScreenCnt, stSaveParam.u32Width, stSaveParam.u32Height, sImgFmt[i]);
                    dspttk_write_file(sFileName, 0, SEEK_SET, stSaveParam.pOutBuff, stSaveParam.u32DataSize);
                }
                else
                {
                    PRT_INFO("the save output directory is NULL\n");
                }
            }
            else
            {
                PRT_INFO("SendCmdToDsp HOST_CMD_XPACK_SAVE failed, outType: %d, BufSize: %u, DataSize %u\n", 
                         stSaveParam.outType, u32BufSize, stSaveParam.u32DataSize);
                sRet = SAL_FAIL;
            }
        }

        /* save成功后计数+1 */
        pstSaveInfo->stSavePicInfo[pstSaveInfo->u32SaveScreenCnt].u32SaveIdx = pstSaveInfo->u32SaveScreenCnt;
        pstSaveInfo->stSavePicInfo[pstSaveInfo->u32SaveScreenCnt].u32Width = stSaveParam.u32Width;
        pstSaveInfo->stSavePicInfo[pstSaveInfo->u32SaveScreenCnt].u32Height = stSaveParam.u32Height;
        pstSaveInfo->stSavePicInfo[pstSaveInfo->u32SaveScreenCnt].u64Time = u64CurrentTime;
        strcpy(pstSaveInfo->stSavePicInfo[pstSaveInfo->u32SaveScreenCnt].cJpgPath ,sFileName);
/*         PRT_INFO("cnt %u idx %u w %u h %u time %llu name %s\n", pstSaveInfo->u32SaveScreenCnt, 
                        pstSaveInfo->stSavePicInfo[pstSaveInfo->u32SaveScreenCnt].u32SaveIdx,
                        pstSaveInfo->stSavePicInfo[pstSaveInfo->u32SaveScreenCnt].u32Width,
                        pstSaveInfo->stSavePicInfo[pstSaveInfo->u32SaveScreenCnt].u32Height,
                        pstSaveInfo->stSavePicInfo[pstSaveInfo->u32SaveScreenCnt].u64Time,
                        pstSaveInfo->stSavePicInfo[pstSaveInfo->u32SaveScreenCnt].cJpgPath); */
    }
    pstSaveInfo->u32SaveScreenCnt++;

    free(pSaveBuf);

    return CMD_RET_MIX(HOST_CMD_XPACK_SAVE, sRet);
}
