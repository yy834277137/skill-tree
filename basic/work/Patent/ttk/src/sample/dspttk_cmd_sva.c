/**
 * @file    dspttk_cmd_sva.c
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
#include "dspttk_cmd_sva.h"
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


/**
 * @fn      dspttk_sva_set_init_switch
 * @brief   设置SVA算法初始化 打开引擎智能识别
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_INIT
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_init_switch(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_SVA *pstCfgSva = &pstDevCfg->stSettingParam.stSva;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        if (!!pstCfgSva->enSvaInit)
        {
            sRet |= SendCmdToDsp(HOST_CMD_SVA_INIT, u32Chan, NULL, NULL);
        }
        else
        {
            sRet |= SendCmdToDsp(HOST_CMD_SVA_DEINIT, u32Chan, NULL, NULL);
        }
    }

    return CMD_RET_MIX(HOST_CMD_SVA_INIT, sRet);
}

/**
 * @fn      dspttk_sva_dect_switch
 * @brief   设置危险物检测总开关
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_DECT_SWITCH
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_dect_switch(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_OSD *pstCfgOsd = &pstDevCfg->stSettingParam.stOsd;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        sRet |= SendCmdToDsp(HOST_CMD_SVA_DECT_SWITCH, u32Chan, NULL, &pstCfgOsd->enOsdAllDisp);
        PRT_INFO("u32Chan %d HOST_CMD_SVA_DECT_SWITCH\n", u32Chan);
    }

    return CMD_RET_MIX(HOST_CMD_SVA_DECT_SWITCH, sRet);
}

/**
 * @fn      dspttk_sva_set_confidence_switch
 * @brief   设置置信度显示开关
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_SET_EXT_TYPE
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_confidence_switch(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_OSD *pstCfgOsd = &pstDevCfg->stSettingParam.stOsd;

    sRet = SendCmdToDsp(HOST_CMD_SVA_SET_EXT_TYPE, 0, NULL, &pstCfgOsd->enOsdConfDisp);
    if (sRet != SAL_SOK)
    {
        PRT_INFO("HOST_CMD_SVA_SET_EXT_TYPE error.\n");
        sRet = SAL_FAIL;
    }

    return CMD_RET_MIX(HOST_CMD_SVA_SET_EXT_TYPE, sRet);
}

/**
 * @fn      dspttk_sva_set_scale_level
 * @brief   设置报警物osd缩放等级
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_OSD_SCALE_LEAVE
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_scale_level(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_OSD *pstCfgOsd = &pstDevCfg->stSettingParam.stOsd;
    
    sRet = SendCmdToDsp(HOST_CMD_SVA_OSD_SCALE_LEAVE, 0, NULL, &pstCfgOsd->u32ScaleLevel);
    if (sRet != SAL_SOK)
    {
        PRT_INFO("HOST_CMD_SVA_OSD_SCALE_LEAVE error.\n");
        sRet = SAL_FAIL;
    }
    
    return CMD_RET_MIX(HOST_CMD_SVA_OSD_SCALE_LEAVE, sRet);
}

/**
 * @fn      dspttk_sva_set_osd_border_type
 * @brief   设置OSD 叠框类型
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SVA_OSD_BORDER_TYPE
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_osd_border_type(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_OSD *pstCfgOsd = &pstDevCfg->stSettingParam.stOsd;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        sRet |= SendCmdToDsp(HOST_CMD_SVA_OSD_BORDER_TYPE, u32Chan, NULL, &pstCfgOsd->enBorderType);
        if (sRet != SAL_SOK)
        {
            PRT_INFO("u32Chan %d HOST_CMD_SVA_OSD_BORDER_TYPE error.\n", u32Chan);
            sRet = SAL_FAIL;
        }
    }

    return CMD_RET_MIX(HOST_CMD_SVA_OSD_BORDER_TYPE, sRet);
}

/**
 * @fn      dspttk_sva_set_alert_color
 * @brief   设置危险品框颜色
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SET_DISPLINETYPE_PRM
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_alert_color(UINT32 u32Chan)
{    
    INT32 sRet = SAL_SOK;
    UINT32 i = 0,idx = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_OSD *pstCfgOsd = &pstDevCfg->stSettingParam.stOsd;
    SVA_ALERT_COLOR stColor = {0};

    for ( i = 0; i < SVA_XSI_MAX_ALARM_TYPE; i++) /* 修改颜色的违禁品应该和实际注册的保持一致(当前仍未保护AI违禁品) */
    {
        stColor.sva_type[i] = i;
        stColor.sva_color[i] = strtoul(pstCfgOsd->cDispColor, NULL, 16);
        idx ++;
    }
    stColor.sva_cnt = idx;

    sRet = SendCmdToDsp(HOST_CMD_SVA_ALERT_COLOR, 0, NULL, &stColor);
    return CMD_RET_MIX(HOST_CMD_SVA_ALERT_COLOR, sRet);
}

/**
 * @fn      dspttk_sva_set_disp_line_type
 * @brief   设置同名危险品合并
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN] 调用命令字 HOST_CMD_SET_DISPLINETYPE_PRM
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_sva_set_disp_line_type(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_OSD *pstCfgOsd = &pstDevCfg->stSettingParam.stOsd;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    /* 点线和虚线需要设置点和线的距离 此处可以作为灵活配置项使用 */
    if (pstCfgOsd->stDispLinePrm.frametype == DISP_FRAME_TYPE_DOTTEDLINE)
    {
        pstCfgOsd->stDispLinePrm.fulllinelength = 5;
        pstCfgOsd->stDispLinePrm.gaps = 1;
        pstCfgOsd->stDispLinePrm.node = 0;
    }
    else if (pstCfgOsd->stDispLinePrm.frametype == DISP_FRAME_TYPE_DASHDOTLINE)
    {
        pstCfgOsd->stDispLinePrm.fulllinelength = 2;
        pstCfgOsd->stDispLinePrm.gaps = 4;
        pstCfgOsd->stDispLinePrm.node = 1;
    }

    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        sRet |= SendCmdToDsp(HOST_CMD_SET_DISPLINETYPE_PRM, u32Chan, NULL, &pstCfgOsd->stDispLinePrm);
        if (sRet != SAL_SOK)
        {
            PRT_INFO("u32Chan %d HOST_CMD_SET_DISPLINETYPE_PRM error.\n", u32Chan);
            sRet = SAL_FAIL;
        }
    }
    
    return CMD_RET_MIX(HOST_CMD_SET_DISPLINETYPE_PRM, sRet);
}
