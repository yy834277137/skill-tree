/**
 * @file   audio_hostcmd_proc_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频模块主机命令处理
 * @author wangweiqin
 * @date   2017年10月12日 Create
 * @note
 * @note \n History
   1.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#ifndef _ADUIO_HOSTCMD_PROC_API_H_
#define _ADUIO_HOSTCMD_PROC_API_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


/**
 * @function   CmdProc_audioCmdProc
 * @brief   DSP 模块audio的处理命令
 * @param[in]  HOST_CMD cmd APP下发DSP的主机命令
 * @param[in]  UINT32 chan 本通道参数是指独立的音频源
 * @param[in]  VOID *pBuf 下发的参数
 * @param[out] None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 CmdProc_audioCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _ADUIO_HOSTCMD_PROC_H_ */

