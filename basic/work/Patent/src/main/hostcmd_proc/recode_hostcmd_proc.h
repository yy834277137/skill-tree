/*******************************************************************************
  *recode_hostcmd_proc.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wutao <wutao19@hikvision.com.cn>
* Version: V1.0.0  2019年9月28日 Create
*
* Description : 解码模块主机命令处理模块
* Modification:
*******************************************************************************/

#ifndef _RECODE_HOSTCMD_PROC_H_
#define _RECODE_HOSTCMD_PROC_H_

#include "libdemux.h"
#include "libmux.h"
#include "system_prm_api.h"
#include "PSMuxLib.h"



INT32 CmdProc_recodeCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf);



#endif /* _VDEC_HOSTCMD_PROC_H_ */

