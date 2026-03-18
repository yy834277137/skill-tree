/*******************************************************************************
* ba_tsk.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2019年9月5日 Create
*
* Description :
* Modification:
*******************************************************************************/

/* ========================================================================== */
/*                          头文件区									   */
/* ========================================================================== */
#include "ba_tsk.h"
#include "ba_drv.h"

/* ========================================================================== */
/*                          宏定义区									   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构区									   */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数定义区									   */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : CmdProc_baCmdProc
* 描  述  :
* 输  入  : - cmd :
*         : - chan:
*         : - prm :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 CmdProc_baCmdProc(HOST_CMD cmd, UINT32 chan, void *prm)
{
    int iRet = SAL_FAIL;

    /* INT32 i = 0; */

    switch (cmd)
    {
        case HOST_CMD_BA_INIT: /* 开启BA */
        {
            iRet = Ba_DrvModuleInit(chan, prm);
            break;
        }
        case HOST_CMD_BA_DEINIT: /* 退出BA */
        {
            iRet = Ba_DrvModuleDeInit(chan);
            break;
        }
        case HOST_CMD_SET_BA_REGION: /* 设置检测区域，暂不开放 */
        {
            iRet = Ba_DrvSetDetectRegion(chan, prm);
            break;
        }
        case HOST_CMD_SET_BA_DET_LEVEL: /* 设置行为分析检测级别，暂不开放 */
        {
            iRet = Ba_DrvSetDetectLevel(chan, prm);
            break;
        }
        case HOST_CMD_SET_BA_CAPTURE: /*设置抓拍开关*/
        {
            /* iRet = Ba_DrvCaptSwitch(chan, prm); */
            break;
        }
        case HOST_CMD_GET_BA_VERSION:
        {
            iRet = Ba_DrvGetVersion(chan, prm);/*获取版本号*/
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

/**
 * @function   Ba_tskShowCompileVersion
 * @brief      打印智能编译时间
 * @param[in]  VOID  
 * @param[out] None
 * @return     static VOID
 */
static VOID Ba_tskShowCompileVersion(VOID)
{
    BA_LOGW("Module: ba, Build Time: %s %s\n", __DATE__, __TIME__);
	return;
}

/*******************************************************************************
* 函数名  : Ba_tskInit
* 描  述  :
* 输  入  : - void:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*			 SAL_FAIL : 失败
*******************************************************************************/
INT32 Ba_tskInit(void)
{
    INT32 s32Ret = SAL_FAIL;

    s32Ret = Ba_DrvInit();
    if (s32Ret != SAL_SOK)
    {
        BA_LOGE("error\n");
        return SAL_FAIL;
    }

	/* 打印版本编译时间 */
	Ba_tskShowCompileVersion();
    return SAL_SOK;
}

