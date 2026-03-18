/**
 * @file   xray_hostcmd_proc.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief  X ray数据下载解析预处理模块命令交互头文件
 * @author liwenbin
 * @date   2019/10/19
 * @note :
 * @note \n History:
   1.Date        : 2019/10/19
     Author      : liwenbin
     Modification: Created file
 */

#ifndef _XRAY_HOSTCMD_PROC_H_
#define _XRAY_HOSTCMD_PROC_H_

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */



/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */



/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */



/* ========================================================================== */
/*                          Function Declarations                             */
/* ========================================================================== */

/**
 * @function:   CmdProc_XrayCmdProc
 * @brief:      X ray数据下载解析预处理模块命令处理
 * @param[in]:  HOST_CMD cmd  命令号
 * @param[in]:  UINT32 chan   通道
 * @param[in]:  VOID *pBuf    命令参数缓冲
 * @param[out]: None
 * @return:     INT32         成功SAL_SOK，失败SAL_FALSE
 */
INT32 CmdProc_XrayCmdProc(HOST_CMD cmd, UINT32 chan, UINT32 *pParam, VOID *pBuf);



#endif /* end of #ifndef _XRAY_HOSTCMD_PROC_H_*/

