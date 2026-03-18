/**
 * @file:   ppm_tsk.c
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  人包关联应用交互层源文件
 * @author: sunzelin
 * @date    2020/12/28
 * @note:
 * @note \n History:
   1.日    期: 2020/12/28
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "ppm_tsk.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/**
 * @function:   Ppm_TskSetVprocHandle
 * @brief:      设置vproc通道句柄
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_TskSetVprocHandle(UINT32 chan, UINT32 u32OutChn, VOID *pDupChnHandle)
{
    if (SAL_SOK != Ppm_DrvSetVprocHandle(chan, u32OutChn, pDupChnHandle))
    {
        SAL_ERROR("set vproc handle failed! chan %d, outChn %d \n", chan, u32OutChn);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   Ppm_tskShowCompileVersion
 * @brief      打印智能编译时间
 * @param[in]  VOID  
 * @param[out] None
 * @return     static VOID
 */
static VOID Ppm_tskShowCompileVersion(VOID)
{
    PPM_LOGW("Module: ppm, Build Time: %s %s\n", __DATE__, __TIME__);
	return;
}

/**
 * @function:   Ppm_TskInit
 * @brief:      任务层初始化接口
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_TskInit(VOID)
{
    if (SAL_SOK != Ppm_DrvInit())
    {
        SAL_ERROR("%s failed! \n", __func__);
        return SAL_FAIL;
    }

	/* 打印人包智能模块组件编译时间 */
	Ppm_tskShowCompileVersion();

    return SAL_SOK;
}

/**
 * @function:   CmdProc_ppmCmdProc
 * @brief:      人包关联交互命令处理函数
 * @param[in]:  HOST_CMD cmd
 * @param[in]:  UINT32 chan
 * @param[in]:  void *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 CmdProc_ppmCmdProc(HOST_CMD cmd, UINT32 chan, void *prm)
{
    int iRet = SAL_FAIL;

    switch (cmd)
    {
        case HOST_CMD_PPM_MODULE_INIT:
        {
            iRet = Ppm_DrvModuleInit();
            break;
        }
        case HOST_CMD_PPM_MODULE_DEINIT:
        {
            iRet = Ppm_DrvModuleDeinit();
            break;
        }
        case HOST_CMD_PPM_ENABLE_CHANNEL:
        {
            iRet = Ppm_DrvEnableChannel(chan);
            break;
        }
        case HOST_CMD_PPM_DISABLE_CHANNEL:
        {
            iRet = Ppm_DrvDisableChannel(chan);
            break;
        }
        case HOST_CMD_PPM_SET_PKG_CAPT_REGION:
        {
            iRet = Ppm_DrvSetPkgCapRegion(chan, prm);
            break;
        }
        case HOST_CMD_PPM_SET_FACE_DET_REGION:
        {
            iRet = Ppm_DrvSetFaceRoi(chan, prm);
            break;
        }
        case HOST_CMD_PPM_SET_CAPT_AB_REGION:
        {
            iRet = Ppm_DrvSetCapABRegion(chan, prm);
            break;
        }
        case HOST_CMD_PPM_SET_XSI_MATCH_PRM:
        {
            iRet = Ppm_DrvSetXsiMatchPrm(chan, prm);
            break;
        }
        case HOST_CMD_PPM_SET_DETECT_SENSITY:
        {
            iRet = Ppm_DrvSetDetSensity(chan, prm);
            break;
        }
        case HOST_CMD_PPM_SET_FACE_MATCH_THRS:
        {
            iRet = Ppm_DrvSetFaceMatchThresh(chan, prm);
            break;
        }
        case HOST_CMD_PPM_SET_MATCH_MATRIX_PRM:
        {
            iRet = Ppm_DrvSetMatchMatrixPrm(chan, prm);
            break;
        }
        case HOST_CMD_PPM_SET_FACE_SCORE_FILTER:
        {
            iRet = Ppm_DrvSetFaceScoreFilter(chan, prm);
            break;
        }
        case HOST_CMD_PPM_SET_CALIB_PRM:
        {
            iRet = Ppm_DrvSetCalibPrm(chan, prm);
            break;
        }
        case HOST_CMD_PPM_SET_FACE_CALIB_PRM:
        {
            iRet = Ppm_DrvSetFaceCalibPrm(chan, prm);
            break;
        }
        case HOST_CMD_PPM_GENERATE_ALG_CFG_FILE:
        {
            iRet = Ppm_DrvGenerateAlgCfgFile(prm);
            break;
        }
        case HOST_CMD_PPM_GET_VERSION:
        {
            iRet = Ppm_DrvGetVersion(prm);
            break;
        }
        case HOST_CMD_PPM_SET_SET_CONFIDENCE_TH:
        {
            iRet = Ppm_DrvSetConfTh(chan, prm);
            break;
        }
        case HOST_CMD_PPM_SET_POS_SWITCH:
        {
            iRet = Ppm_DrvSetPosSwitch(prm);
            break;
        }
        default:
        {
            BA_LOGE("CMD <%x> is ERROR !!!\n", cmd);
            iRet = SAL_FAIL;
            break;
        }
    }

    return iRet;
}

