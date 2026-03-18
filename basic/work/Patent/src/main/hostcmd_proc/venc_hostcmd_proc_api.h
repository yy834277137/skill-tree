/**
 * @file   venc_hostcmd_proc_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  编码模块主机命令处理
 * @author wangweiqin
 * @date   2017年10月12日 Create
 * @note
 * @note \n History
   1.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#ifndef _VENC_HOSTCMD_PROC_API_H_
#define _VENC_HOSTCMD_PROC_API_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


/**
 * @function   CmdProc_encCmdProc
 * @brief   DSP 模块编码处理命令
 * @param[in]  HOST_CMD cmd APP下发DSP的主机命令
 * @param[in]  UINT32 chan 本通道参数是指独立的视频源(每个视频设备下可以有多路不同分辨率的码流，
                           独立通过主机命令和本通道结合来表示)
 * @param[in]  VOID *pBuf 下发的参数
 * @param[out] None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */

INT32 CmdProc_encCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _VENC_HOSTCMD_PROC_H_ */

