/**
 * @file   osd_hostcmd_proc_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  osd模块主机命令处理
 * @author heshengyuan
 * @date   2018年9月30日 Create
 * @note
 * @note \n History
   1.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#ifndef _OSD_HOST_CMD_PROC_API_H_
#define _OSD_HOST_CMD_PROC_API_H_
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* ========================================================================== */
/*                          函数声明                                        */
/* ========================================================================== */

/**
 * @function   CmdProc_osdCmdProc
 * @brief   DSP 模块 osd 的处理命令
 * @param[in]  HOST_CMD cmd APP下发DSP的主机命令
 * @param[in]  UINT32 chan 本通道参数是指独立的音频源
 * @param[in]  VOID *pBuf 下发的参数
 * @param[out] None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 CmdProc_osdCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*_OSD_HOST_CMD_PROC_H_*/

