/*******************************************************************************
 * fld_hostcmd_proc.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : huangshuxin <huangshuxin@hikvision.com>
 * Version: V1.0.0  2019年5月23日 Create
 *
 * Description : 
 * Modification: 
 *******************************************************************************/



/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <sal.h>

#include "dspcommon.h"
#include "fld_hostcmd_proc.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

INT32 CmdProc_fldCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf)
{
    int iRet     = SAL_FAIL;

    SAL_INFO("Chn: %d cmd: %x !!!\n", chan, cmd);

    switch(cmd)
    {
        case HOST_CMD_SET_SOLID_PARAM:
        {
            iRet = SAL_SOK;
            break;
        }
        case HOST_CMD_GET_SOLID_PARAM:
        {
            iRet = SAL_SOK;
            break;
        }        
        default:
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
    }

    return iRet;
}





