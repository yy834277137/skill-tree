/*******************************************************************************
* xsp_tsk.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : zhangxunhua <zhangxunhua@hikvision.com>
* Version: V1.0.0  2019年10月18日 Create
*
* Description :
* Modification:
*******************************************************************************/


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "xsp_tsk.h"
#include "xsp_drv.h"

#line __LINE__ "xsp_tsk.c"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : CmdProc_xspCmdProc
* 描  述  :
* 输  入  : - cmd :
*         : - chan:
*         : - prm :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 CmdProc_xspCmdProc(HOST_CMD cmd, UINT32 chan, void *prm)
{
    int iRet = SAL_SOK;

    XSP_LOGI("chan[%d], cmd: 0x%x\n", chan, cmd);

/*******************************************************
 *    1) 模式(默认)设置:
 *       默认模式0：增强模式0
 *       默认模式1：增强模式1
 *       默认模式2：增强模式2
 *    2) 原始模式设置：
 *       开启：开启原始模式，所有功能关闭，即恢复默认；
 *       关闭：关闭原始模式，其它功能不变；
 * 研究院算法：
 *    3) 低穿/高穿/局增 关系互斥；
 * 自研算法：
 *    4) 低穿/高穿互斥；
 *    5) 边增/超增互斥；
 *    互斥的关系，最多只能一个功能开启
 ********************************************************/

    switch (cmd)
    {
    case HOST_CMD_MODULE_XSP: /* XSP图像处理模块 */
        break;

    case HOST_CMD_MODULE_XSP_INIT: /* XSP图像处理模块初始化 */
        XSP_LOGI("The HOST_CMD_MODULE_XSP_INIT is not support\n");
        break;

    case HOST_CMD_MODULE_XSP_DEINIT: /* XSP图像处理模块去初始化*/
        XSP_LOGI("The HOST_CMD_MODULE_XSP_DEINIT is not support\n");
        break;

    case HOST_CMD_XSP_SET_DEFAULT: /* 过包模式*/
        XSP_LOGI("The HOST_CMD_XSP_SET_DEFAULT\n");
        iRet = Xsp_DrvSetMode(chan, prm);
        break;

    case HOST_CMD_XSP_SET_ORIGINAL:   /* 设置原始模式*/
        XSP_LOGI("The HOST_CMD_XSP_SET_ORIGINAL\n");
        iRet = Xsp_DrvSetOriginal(chan, prm);
        break;

    case HOST_CMD_XSP_SET_DISP:   /* 设置显示模式*/
        XSP_LOGI("The HOST_CMD_XSP_SET_CG_MODE\n");
        iRet = Xsp_DrvSetDisp(chan, prm);
        break;

    case HOST_CMD_XSP_SET_ANTI:  /* 反色显示设置*/
        XSP_LOGI("The HOST_CMD_XSP_SET_ANTI_COLOR\n");
        iRet = Xsp_DrvSetAnti(chan, prm);
        break;

    case HOST_CMD_XSP_SET_EE:   /* 边缘增强设置*/
        XSP_LOGI("The HOST_CMD_XSP_SET_EDGE_ENHANCE\n");
        iRet = Xsp_DrvSetEe(chan, prm);
        break;

    case HOST_CMD_XSP_SET_UE:  /* 超级增强设置*/
        XSP_LOGI("The HOST_CMD_XSP_SET_SUPER_ENHANCE.\n");
        iRet = Xsp_DrvSetUe(chan, prm);
        break;

    case HOST_CMD_XSP_SET_ABSOR:   /* 可变吸收率设置*/
        XSP_LOGI("HOST_CMD_XSP_SET_ABSOR\n");
        iRet = Xsp_DrvSetAbsor(chan, prm);
        break;

    case HOST_CMD_XSP_SET_UNPEN:    /* 难穿透物质识别设置*/
        XSP_LOGI("HOST_CMD_XSP_SET_UNPEN\n");
        iRet = Xsp_DrvSetUnpen(chan, prm);
        break;

    case HOST_CMD_XSP_GET_VERSION: /* 获取版本号 */
        XSP_LOGI("HOST_CMD_XSP_GET_VERSION\n");
        iRet = Xsp_DrvGetVersion(chan, prm);
        break;

    case HOST_CMD_XSP_SET_LP:           /*低穿*/
        XSP_LOGI("HOST_CMD_XSP_SET_LP\n");
        iRet = Xsp_DrvSetLp(chan, prm);
        break;

    case HOST_CMD_XSP_SET_HP:           /*高穿*/
        XSP_LOGI("HOST_CMD_XSP_SET_HP\n");
        iRet = Xsp_DrvSetHp(chan, prm);
        break;

    case HOST_CMD_XSP_SET_LE:           /*局增*/
        XSP_LOGI("HOST_CMD_XSP_SET_LE\n");
        iRet = Xsp_DrvSetLe(chan, prm);
        break;

    case HOST_CMD_XSP_SET_LUMINANCE:   /* 设置亮度*/
        XSP_LOGI("HOST_CMD_XSP_SET_LUMINANCE\n");
        iRet = Xsp_DrvSetLum(chan, prm);
        break;

    case HOST_CMD_XSP_SET_PSEUDO_COLOR: /* 设置伪彩*/
        XSP_LOGI("HOST_CMD_XSP_SET_PSEUDO_COLOR\n");
        iRet = Xsp_DrvSetPseudoColor(chan, prm);
        break;

    case HOST_CMD_XSP_SET_MIRROR:     /* 设置镜像*/
        XSP_LOGI("HOST_CMD_XSP_SET_MIRROR\n");
        iRet = Xsp_DrvSetMirror(chan, prm);
        break;

    case HOST_CMD_XSP_SET_SUS_ALERT: /* 设置可疑物报警*/
        XSP_LOGI("HOST_CMD_XSP_SET_SUS_ALERT\n");
        iRet = Xsp_DrvSetSusAlert(chan, prm);
        break;

    case HOST_CMD_XSP_GET_TIP_RAW:   /* 获取tip xraw数据*/
        XSP_LOGI("HOST_CMD_XSP_GET_TIP_RAW\n");
        iRet = Xsp_DrvGetTipRaw(chan, prm);
        break;

    case HOST_CMD_XSP_SET_TIP:        /* 设置tip*/
        XSP_LOGI("HOST_CMD_XSP_SET_TIP\n");
        iRet = Xsp_DrvSetTipPrm(chan, prm);
        break;

    case HOST_CMD_XSP_SET_ENHANCED_SCAN:  /* 设置强扫*/
        XSP_LOGI("HOST_CMD_XSP_SET_ENHANCED_SCAN\n");
        iRet = Xsp_DrvSetEnhancedScan(chan, prm);
        break;

    case HOST_CMD_XSP_SET_BKG:  /* 设置背景检测参数 */
        XSP_LOGI("HOST_CMD_XSP_SET_BKG\n");
        iRet = Xsp_DrvSetBkgParam(chan, prm);
        break;

    case HOST_CMD_XSP_SET_DEFORMITY_CORRECTION:  /* 设置畸变矫正 */
        XSP_LOGI("HOST_CMD_XSP_SET_DEFORMITY_CORRECTION\n");
        iRet = Xsp_DrvSetDeformityCorrection(chan, prm);
        break;

    case HOST_CMD_XSP_SET_COLOR_TABLE:  /* 设置颜色表 */
        XSP_LOGI("HOST_CMD_XSP_SET_COLOR_TABLE\n");
        iRet = Xsp_DrvSetColorTable(chan, prm);
        break;

    case HOST_CMD_XSP_SET_COLORS_IMAGING: /* 设置三色六色显示 */
        XSP_LOGI("HOST_CMD_XSP_SET_COLORS_IMAGING\n");
        iRet = Xsp_DrvSetColorsImaging(chan, prm);
        break;

    case HOST_CMD_XSP_COLOR_ADJUST:  /* 颜色调节 */
        XSP_LOGI("HOST_CMD_XSP_COLOR_ADJUST\n");
        iRet = Xsp_DrvSetColorAdjust(chan, prm);
        break;

    case HOST_CMD_XSP_NOISE_REDUCE: /* 降噪 */
        iRet = Xsp_DrvSetNoiseReduce(chan, ((XSP_NOISE_REDUCE_PARAM *)prm)->uiLevel);
        break;

    case HOST_CMD_XSP_SET_CONTRAST: /* 对比度 */
        iRet = Xsp_DrvSetContrast(chan, ((XSP_CONTRAST_PARAM *)prm)->uiLevel);
        break;

    case HOST_CMD_XSP_SET_GAMMA:  /* 显示GAMMA参数 */
        iRet = Xsp_DrvSetGamma(chan, prm);
        break;

    case HOST_CMD_XSP_SET_AIXSP_RATE:  /* AIXSP权重参数 */
        iRet = Xsp_DrvSetAixspRate(chan, ((XSP_AIXSP_RATE_PARAM *)prm)->uiLevel);
        break;

    case HOST_CMD_XSP_SET_LE_TH: /* 局增阈值 */
        iRet = Xsp_DrvSetLeThRange(chan, ((XSP_LE_TH_RANGE *)prm)->u32LowerLimit, ((XSP_LE_TH_RANGE *)prm)->u32UpperLimit);
        break;

    case HOST_CMD_XSP_SET_DT_GRAY: /* 双能分辨灰度 */
        iRet = Xsp_DrvSetDtGrayRange(chan, ((XSP_DT_GRAY_RANGE *)prm)->u32LowerLimit, ((XSP_DT_GRAY_RANGE *)prm)->u32UpperLimit);
        break;

    case HOST_CMD_XSP_SET_SPEEDUP_MODE: /* XSP算法加速模式 */
        break;

    case HOST_CMD_XSP_PACKAGE_DIVIDE: /* 包裹分割参数 */
        iRet = Xsp_DrvSetPackageDivide(chan, (XSP_PACKAGE_DIVIDE_PARAM *)prm);
        break;

    case HOST_CMD_XSP_SET_BLANK_SLICE:        /* 设置空白条带用于显示*/
        XSP_LOGI("HOST_CMD_XSP_SET_BLANK_SLICE\n");
        iRet = Xsp_DrvSetBlankSlice(chan, (XSP_SET_RM_BLANK_SLICE *)prm);
        break;

    case HOST_CMD_XSP_DOSE_CORRECT:  /* 剂量波动修正 */
        iRet = Xsp_DrvSetDoseCorrect(chan, prm);
        break;
    
    case HOST_CMD_XSP_SET_BKGCOLOR:  /* 显示图像背景色 */
        iRet = Xsp_DrvSetBkgColor(chan, prm);
        break;

    case HOST_CMD_XSP_SET_COLDHOT_THRESHOLD: /* 设置冷热源阈值 */
        iRet = Xsp_DrvSetColdHotThreshold(chan, prm);
        break;

    case HOST_CMD_XSP_SET_STRETCH: /* 包裹拉伸参数 */
        iRet = Xsp_DrvSetStretch(chan,(XSP_STRETCH_MODE_PARAM *)prm);
        break;

    case HOST_CMD_XSP_RELOAD_ZTABLE: /* 重新加载Z值表 */
        iRet = Xsp_DrvReloadZTable(chan, prm);
        break;

    default:
        SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
        iRet = SAL_FAIL;
        break;
    }

    return iRet;
}

#if 0

/*******************************************************************************
* 函数名  : Xsp_tskDevInit
* 描  述  : xsp tsk初始化
* 输  入  : - pDupHandle:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Xsp_tskDevInit(void *pDupHandle)
{
    INT32 s32Ret = SAL_FAIL;

    if (pDupHandle == NULL)
    {
        SAL_ERROR("error\n");
        return SAL_FAIL;
    }

    /* s32Ret = Sva_DrvDevInit(pDupHandle); */
    if (s32Ret != SAL_SOK)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}

#endif

/*******************************************************************************
* 函数名  : Xsp_tskInit
* 描  述  :
* 输  入  : - void:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Xsp_tskInit(void)
{
    #if 0
    INT32 s32Ret = SAL_FAIL;


    s32Ret = Xsp_DrvInit();
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("Xsp_tskInit error\n");
        return SAL_FAIL;
    }

    #endif
    return SAL_SOK;
}

/******************************************************************
 * @function:   Xsp_tskDeInit
 * @brief:      xsp模块去初始化
 * @param[in]:  NONE
 * @param[out]: NONE
 * @return:     SAL_SOK 成功, SAL_FAIL 失败
 *******************************************************************/
INT32 Xsp_tskDeInit(void)
{
    #if 0
    INT32 s32Ret = SAL_FAIL;

    /* s32Ret = Xsp_DrvInit(); */
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("Xsp_tskDeInit is not finished\n");
        return SAL_FAIL;
    }

    #endif

    return SAL_SOK;
}