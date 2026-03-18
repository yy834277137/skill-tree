/*******************************************************************************
* sva_tsk.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : huangshuxin <huangshuxin@hikvision.com>
* Version: V1.0.0  2019年2月14日 Create
*
* Description :
* Modification:
*******************************************************************************/



/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "sva_tsk.h"
#include "link_drv_api.h"

#include "system_prm_api.h"

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
* 函数名  : CmdProc_svaCmdProc
* 描  述  : 命令字接口
* 输  入  : - cmd : 命令字
*         : - chan: 通道号
*         : - prm : 参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 CmdProc_svaCmdProc(HOST_CMD cmd, UINT32 chan, void *prm)
{
    int iRet = SAL_FAIL;

    switch (cmd)
    {
        case HOST_CMD_MODULE_SVA: /* 预留 */
        {
            break;
        }
        case HOST_CMD_SVA_GET_VERSION:
        {
            iRet = Sva_DrvGetVersion(prm); /*获取版本号*/
            break;
        }
        case HOST_CMD_SVA_INIT: /* 算法初始化*/
        {
            iRet = Sva_DrvModuleInit(chan);
            break;
        }
        case HOST_CMD_SVA_DEINIT: /*  算法去初始化*/
        {
            iRet = Sva_DrvModuleDeInit(chan);
            break;
        }
        case HOST_CMD_SVA_SET_DETECT_REGION: /*设置智能检测区域*/
        {
            iRet = Sva_DrvSetDetectRegion(chan, prm);
            break;
        }
        case HOST_CMD_SVA_ALERT_SWITCH:  /* 设置危险物检测开关，单个检测物 */
        {
            iRet = Sva_DrvSetAlertSwitch(chan, prm);
            break;
        }
        case HOST_CMD_SVA_ALERT_COLOR:   /* 危险物颜色设置 */
        {
            iRet = Sva_DrvSetAlertColor(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_CONF:   /* 危险物检测置信度设置 */
        {
            iRet = Sva_DrvSetAlertConf(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_DIRETION: /* 设置传送带方向 */
        {
            iRet = Sva_DrvSetDirection(chan, prm);
            break;
        }
        case HOST_CMD_SVA_UPDATE_MODEL: /*模型升级，暂时保留*/
        {
			iRet = Sva_DrvUpgradeModel(chan, prm);
            break;
        }
        case HOST_CMD_SVA_DECT_SWITCH: /*危险物检测总开关*/
        {
            iRet = Sva_DrvSetDectSwitch(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_CHN_SIZE: /*设置安检机通道大小,目前无效*/
        {
            iRet = Sva_DrvSetChnSize(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_EXT_TYPE: /*设置置信度显示开关*/
        {
            iRet = Sva_DrvSetOsdExtType(chan, prm);
            break;
        }
        case HOST_CMD_SVA_ALERT_NAME: /*设置报警物名称*/
        {
            iRet = Sva_DrvSetAlertName(chan, prm);
            break;
        }
        case HOST_CMD_SVA_OSD_SCALE_LEAVE: /*设置报警物信息OSD字体大小*/
        {
            iRet = Sva_DrvSetScaleLevel(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SYNC_MAIN_CHAN: /*设置双通道采集主通道通道号*/
        {
            iRet = Sva_DrvSetSyncMainChan(chan, prm);
            break;
        }
        case HOST_CMD_SVA_PROC_MODE: /*设置处理模式*/
        {
            iRet = Sva_DrvChgProcMode(prm);
            break;
        }
        case HOST_CMD_SVA_OSD_BORDER_TYPE: /*设置OSD 叠框类型*/
        {
            iRet = Sva_DrvSetOsdBorderType(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_AI_MODEL_ID: /*设置AI 模型ID*/
        {
            iRet = Sva_DrvSetModelId(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_AI_MODEL_NAME: /*设置AI模型名称*/
        {
            iRet = Sva_DrvSetModelName(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_AI_MODEL_STATUS: /*设置AI 模型启用开关*/
        {
            iRet = Sva_DrvSetModelDetectStatus(chan, prm);
            break;
        }
        case HOST_CMD_SVA_GET_AI_MODEL_STATUS: /*获取AI 模型开关状态*/
        {
            iRet = Sva_DrvGetModelDetectStatus(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_AI_MODEL_ALERT_MAP: /*设置AI 模型目标映射*/
        {
            iRet = Sva_DrvSetAiTargetMap(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_SAMPLE_COLLECT_SWITCH: /*设置素材采集开关(调试使用)*/
        {
            iRet = Sva_DrvDbgSampleSwitch(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_INPUT_GAP_NUM: /*设置帧处理间隔,目前只支持全帧处理和隔帧处理*/
        {
            iRet = Sva_DrvSetInputGapNum(prm);
            break;
        }
        case HOST_CMD_SVA_INPUT_AUTO_TEST_PIC: /* 送入BMP图片用于算法智能测试使用 */
        {
            iRet = Sva_DrvInputAutoTestPic(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_PKG_SPLIT_SENSITY: /* 设置包裹分割灵敏度 */
        {
            iRet = Sva_DrvSetPkgSplitSensity(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_PKG_SPLIT_FILTER:/* 设置包裹分割过滤阈值 */
        {
            iRet = Sva_DrvSetPkgSplitFilter(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_ZOOM_INOUT_PRM: /* 设置放大缩小配置参数 */
        {
            iRet = Sva_DrvSetZoomInOutVal(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_JPEGPOS_SWITCH: /* 设置增加Jpeg图片pos信息开关 */
        {
            iRet = Sva_DrvSetJpegPosSwitch(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_ALERT_TAR_CNT: /* 设置危险品告警个数阈值 */
        {
            iRet = Sva_DrvSetAlertTarNumThresh(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_ENG_MODEL: /* 设置引擎模式 */
        {
            iRet = Sva_DrvSetEngModel(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_SIZE_FILTER: /* 设置目标尺寸过滤参数 */
        {
            iRet = Sva_DrvSetTarFilterCfg(prm);
            break;
        }
        case HOST_CMD_SVA_SET_SPRAY_FILTER_SWITCH: /* 设置喷灌过滤开关 */
        {
            iRet = Sva_DrvSetSprayFilterSwitch(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_INS_PROC_CNT: /* 设置每秒裁剪张数(成都定制, 保留) */
        {
            iRet = Sva_DrvSetInsCropCnt(prm);
            break;
        }
        case HOST_CMD_SVA_SET_PKG_BORDER_PIXEL: /* 设置包裹边沿预留像素(成都定制, 保留) */
        {
            iRet = Sva_DrvSetPkgBorGapPixel(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_SPLIT_MODE: /* 设置集中判图模式开关 */
        {
            iRet = Sva_DrvSetSplitMode(prm);
            break;
        }
        case HOST_CMD_SVA_SET_PKG_COMB_THRESH: /* 设置包裹是否需要融合的像素阈值，[-50,50] */
        {
            iRet = Sva_DrvSetPkgCombPixelThresh(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_FORCE_SPLIT_PRM: /* 设置包裹强制分割参数，暂未开放 */
        {
            iRet = Sva_DrvSetForceSplitSwitch(chan, prm);
            break;
        }
        case HOST_CMD_SVA_SET_PKG_COMBINE_FLAG: /* 设置包裹融合开关 */
        {
            iRet = Sva_DrvSetPkgCombineFlag(chan, prm);
            break;
        }
        case HOST_CMD_SVA_INIT_USB_CMD: /* 分析仪使用xtrans，支持对USB从片设备进行拉起上电操作 */
        {
            iRet = Sva_DrvResetTrans(chan, prm);
            break;
        }

		case HOST_CMD_SVA_GET_CONF: /*默认置信度获取命令*/
        {
            iRet = Sva_DrvGetDefConf(chan, prm);
            break;
        }
		default:
        {
            SVA_LOGE("CMD <%x> is ERROR !!!\n", cmd);
            iRet = SAL_FAIL;
            break;
        }
    }

    return iRet;
}

/**
 * @function:   Sva_tskSetMirrorChnPrm
 * @brief:      设置镜像通道参数
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_tskSetMirrorChnPrm(UINT32 chan, void *prm)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = Sva_DrvSetMirrorChnPrm(chan, prm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Set Mirror Chn Param Failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_tskSetScaleChnPrm
 * @brief:      设置缩放通道参数
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Sva_tskSetScaleChnPrm(UINT32 chan, void *prm)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = Sva_DrvSetScaleChnPrm(chan, prm);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Set Mirror Chn Param Failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   Sva_tskGetDupHandle
 * @brief:      获取dup handle
 * @param[in]:  UINT32 chan
 * @param[in]:  VOID **ppDupHandle
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Sva_tskGetDupHandle(UINT32 chan, VOID **ppDupHandle)
{
    DSPINITPARA *pstInit = NULL;
    INST_INFO_S *pstInstInfo = NULL;
    VOID *pNodeHandle = NULL;

    CHAR acInstName[NAME_LEN] = {0};

    pstInit = SystemPrm_getDspInitPara();
    if (NULL == pstInit)
    {
        SVA_LOGE("pstInit == null! \n");
        return SAL_FAIL;
    }

    if (DOUBLE_CHIP_SLAVE_TYPE == pstInit->deviceType)
    {
        SVA_LOGW("slave chip no need get dup handle! \n");
        goto exit;
    }

    snprintf(acInstName, NAME_LEN, "%s_%d", "DUP_FRONT", chan);
    pstInstInfo = link_drvGetInst(acInstName);
    if (NULL == pstInstInfo)
    {
        SYS_LOGE("pstInstInfo == null! \n");
        return SAL_FAIL;
    }

    pNodeHandle = link_drvGetHandleFromNode(pstInstInfo, "out_sva");
    if (NULL == pNodeHandle)
    {
        SYS_LOGE("pNodeHandle == null! \n");
        return SAL_FAIL;
    }

exit:
    *ppDupHandle = pNodeHandle;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Sva_tskDevInit
* 描  述  : 任务设备初始化
* 输  入  : - chan: 通道
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_tskDevInit(UINT32 chan)
{
    INT32 s32Ret = SAL_FAIL;
    VOID *pDupHandle = NULL;

#ifndef DSP_ISM
    s32Ret = Sva_tskGetDupHandle(chan, (VOID **)&pDupHandle);
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("get dup handle failed! chan %d \n", chan);
        return SAL_FAIL;
    }
#endif

    s32Ret = Sva_DrvDevInit(pDupHandle);
	if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("sva dev init failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   Sva_tskShowCompileVersion
 * @brief      打印智能编译时间
 * @param[in]  VOID  
 * @param[out] None
 * @return     static VOID
 */
static VOID Sva_tskShowCompileVersion(VOID)
{
    SVA_LOGW("Module: sva, Build Time: %s %s\n", __DATE__, __TIME__);
	return;
}

/*******************************************************************************
* 函数名  : Sva_tskInit
* 描  述  : 任务模块初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Sva_tskInit(void)
{
    INT32 s32Ret = SAL_FAIL;
	
    s32Ret = Sva_DrvInit();
    if (s32Ret != SAL_SOK)
    {
        SVA_LOGE("sva drv init failed! \n");
        return SAL_FAIL;
    }

	/* 打印智能组件编译时间 */
	Sva_tskShowCompileVersion();
    return SAL_SOK;
}

