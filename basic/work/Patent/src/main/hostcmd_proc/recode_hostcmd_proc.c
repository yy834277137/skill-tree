/*******************************************************************************
* vdec_hostcmd_proc.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wutao <wutao19@hikvision.com.cn>
* Version: V1.0.0  2019年9月28日 Create
*
* Description : 解码模块主机命令处理模块
* Modification:
*******************************************************************************/

#include <sal.h>

#include "dspcommon.h"
#include "recode_tsk_api.h"
#include "recode_hostcmd_proc.h"


/*****************************************************************************
                                全局结构体
*****************************************************************************/

/*****************************************************************************
                                函数
*****************************************************************************/

/*****************************************************************************
   函 数 名  : CmdProc_recodeCmdProc
   功能描述  : DSP 模块vdec的处理命令
   输入参数  : 无
   输出参数  : 无
   返 回 值  : 无
   调用函数  :
   被调函数  :

   修改历史      :
   1.日    期   : 2017年10月14日
    作    者   : wwq
    修改内容   : 新生成函数
*****************************************************************************/
INT32 CmdProc_recodeCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf)
{
    int iRet = SAL_SOK;

    RECODE_LOGI("Chn: %d cmd: 0x%x !!!\n", chan, cmd);

    switch (cmd)
    {
        case HOST_CMD_RECODE_START:
        {
            if (SAL_SOK != (iRet = recode_tskStart(chan, NULL, pBuf)))
            {
                RECODE_LOGE("Enc Start Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }

        case HOST_CMD_RECODE_STOP:
        {
            if (SAL_SOK != (iRet = recode_tskStop(chan, NULL, pBuf)))
            {
                RECODE_LOGE("Enc Start Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }

        case HOST_CMD_RECODE_SET:
        {
            if (SAL_SOK != (iRet = recode_tskSetPrm(chan, NULL, pBuf)))
            {
                RECODE_LOGE("Enc Start Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }

        case HOST_CMD_RECODE_AUD_PACK:
        {
            if (SAL_SOK != (iRet = recode_tskSetAudPack(chan, NULL, pBuf)))
            {
                RECODE_LOGE("Enc Start Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }


        default:
        {
            RECODE_LOGE("MODULE_VENC: <0x%x> is nonsupport !!!\n", cmd);
            break;
        }
    }

    return iRet;
}

