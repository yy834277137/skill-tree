/*******************************************************************************
* vdec_hostcmd_proc.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2017年10月12日 Create
*
* Description : 解码模块主机命令处理模块
* Modification:
*******************************************************************************/

#include <sal.h>

#include "dspcommon.h"
#include "vdec_tsk_api.h"

#include "vdec_hostcmd_proc.h"

/*****************************************************************************
                                全局结构体
*****************************************************************************/

/*****************************************************************************
                                函数
*****************************************************************************/

/*****************************************************************************
   函 数 名  : CmdProc_vdecCmdProc
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
INT32 CmdProc_vdecCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf)
{
    int iRet = SAL_SOK;

    VDEC_LOGI("Chn: %d cmd: 0x%x !!!\n", chan, cmd);

    switch (cmd)
    {
        case HOST_CMD_DEC_START:
        {
            if (SAL_SOK != (iRet = vdec_tsk_Start(chan, pBuf)))
            {
                VDEC_LOGE("Enc Start Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }

        case HOST_CMD_DEC_STOP:
        {
            if (SAL_SOK != (iRet = vdec_tsk_Stop(chan, pBuf)))
            {
                VDEC_LOGE("Enc Start Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }

        case HOST_CMD_DEC_PAUSE:
        {
            if (SAL_SOK != (iRet = vdec_tsk_Pause(chan, pBuf)))
            {
                VDEC_LOGE("Enc Start Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_DEC_ATTR:
        {
            if (SAL_SOK != (iRet = vdec_tsk_SetPrm(chan, pBuf)))
            {
                VDEC_LOGE("Enc Start Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_DEC_SYNC:
        {
            if (SAL_SOK != (iRet = vdec_tsk_SetSynPrm(chan, pBuf)))
            {
                VDEC_LOGE("Enc Start Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_DEC_RESET:
        {
            if (SAL_SOK != (iRet = vdec_tsk_ReSet(chan)))
            {
                VDEC_LOGE("Host_DecReSet Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_DEC_AUD_ENABLE:
        {
            if (SAL_SOK != (iRet = vdec_tsk_AudioEnable(chan, pBuf)))
            {
                VDEC_LOGE("Dec Start Aud play Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_DEC_JPG_TO_BMP:
		{
			if (SAL_SOK != (iRet = vdec_tsk_JpgToBmp(pBuf)))
            {
                VDEC_LOGE("Dec Jpg to Bmp Chn %d Failed !!!\n", chan);
            }
            break;
		}
        default:
        {
            VDEC_LOGE("MODULE_VENC: <0x%x> is nonsupport !!!\n", cmd);
            break;
        }
    }

    return iRet;
}

