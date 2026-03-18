/*******************************************************************************
* ba_out.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2021年7月7日 Create
*
* Description :
* Modification:
*******************************************************************************/
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "ba_out.h"

/*******************************************************************************
* 函数名  : Ba_tskInit
* 描  述  : 模块tsk层初始化
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 Ba_tskInit(void)
{
    return SAL_SOK;
}


ATTRIBUTE_WEAK VOID Ba_DrvSetDumpCnt(UINT32 uiCnt)
{
    return;
}

/*******************************************************************************
* 函数名  : CmdProc_baCmdProc
* 描  述  : 应用层命令字交互接口
* 输  入  : cmd : 数据分发句柄
*         : chan: 通道号
*         : prm : 参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
ATTRIBUTE_WEAK INT32 CmdProc_baCmdProc(HOST_CMD cmd, UINT32 chan, void *prm)
{
    return SAL_SOK;
}
