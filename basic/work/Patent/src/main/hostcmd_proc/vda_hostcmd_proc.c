/*******************************************************************************
* vdec_hostcmd_proc.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wutao <wutao19@hikvision.com.cn>
* Version: V1.0.0  2019年7月12日 Create
*
* Description : 移动侦测模块主机命令处理模块
* Modification:
*******************************************************************************/

#include <sal.h>
#include "dspcommon.h"
//#include "vda_tsk.h"
#include "vda_hostcmd_proc.h"

/*****************************************************************************
                                全局结构体
*****************************************************************************/

/*****************************************************************************
                                函数
*****************************************************************************/

/*****************************************************************************
 函 数 名  : CmdProc_vdaCmdProc
 功能描述  : DSP 模块vda的处理命令
 输入参数  : 无
 输出参数  : 无
 返 回 值      :成功SAL_SOK
                            失败SAL_FALSE
*****************************************************************************/
INT32 CmdProc_vdaCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf)
{
    int iRet     = SAL_SOK;

    SAL_INFO("Chn: %d cmd: 0x%x !!!\n", chan, cmd);

#if 0
    switch(cmd)
    {
        case HOST_CMD_START_MD:
        {                                   
            if (SAL_SOK != (iRet = Host_VdaMdChnStart(chan)))
            {
                SAL_ERROR("Start Chn %d Failed !!!\n", chan);
            }
            break;
        }

        case HOST_CMD_STOP_MD:
        {                                   
            if (SAL_SOK != (iRet = Host_VdaMdChnStop(chan)))
            {
                SAL_ERROR("Stop  Chn %d Failed !!!\n", chan);
            }
            break;
        }
        
        case HOST_CMD_SET_MD:
        {                                   
            if (SAL_SOK != (iRet = Host_VdaMdChnSet(chan, pBuf)))
            {
                SAL_ERROR("Set Chn %d Failed !!!\n", chan);
            }
            break;
        }
        
       
        default:
        {
            SAL_ERROR("MODULE_VDA: <0x%x> is nonsupport !!!\n", cmd);
            break;
        }
    }
#endif

    return iRet;
}

