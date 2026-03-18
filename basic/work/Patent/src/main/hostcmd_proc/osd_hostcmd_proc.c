/**
 * @file   osd_hostcmd_proc.c
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

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "osd_tsk_api.h"

/**
 * @function   CmdProc_osdCmdProc
 * @brief   DSP 模块 osd 的处理命令
 * @param[in]  HOST_CMD cmd APP下发DSP的主机命令
 * @param[in]  UINT32 chan 本通道参数是指独立的音频源
 * @param[in]  VOID *pBuf 下发的参数
 * @param[out] None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 CmdProc_osdCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf)
{
    int iRet     = SAL_SOK;

    //return SAL_SOK;
    OSD_LOGI("Chn: %d cmd: 0x%x !!!\n", chan, cmd);

    switch(cmd)
    {
        case HOST_CMD_MODULE_OSD:  /* OSD模块的能力级 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_SET_ENC_OSD:    /* 设置编码通道OSD属性 */
        {
            iRet = osd_tsk_setEncPrm(chan, NULL, pBuf);
            break;
        }
        case HOST_CMD_SET_ENC_DEFAULT_OSD:   /* 设置编码通道默认OSD属性 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_START_ENC_OSD:    /* 开启编码通道OSD功能 */
        {
            iRet = osd_tsk_startEncProc(chan, NULL, pBuf);
            break;
        }
        case HOST_CMD_STOP_ENC_OSD:      /* 停止编码通道OSD功能 */
        {
            iRet = osd_tsk_stopEncProc(chan, NULL, pBuf);
            break;
        }
        case HOST_CMD_SET_DST_OSD:      /* OSD模块启用夏令时 */
        {
            iRet = osd_tsk_startDST(chan, pBuf);
            break;
        }
        case HOST_CMD_SET_DISP_OSD:      /* 设置显示通道OSD功能 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_START_DISP_OSD:     /* 开启显示通道OSD功能 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_STOP_DISP_OSD:      /* 停止显示通道OSD功能 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_SET_ENC_LOGO:      /* 设置编码通道LOGO属性 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_START_ENC_LOGO:       /* 开启编码通道LOGO功能 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_STOP_ENC_LOGO :       /* 停止编码通道LOGO功能 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_SET_DISP_LOGO:        /* 设置显示通道LOGO属性 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_START_DISP_LOGO :      /* 开启显示通道LOGO功能 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_STOP_DISP_LOGO:       /* 停止显示通道LOGO功能 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        case HOST_CMD_UPDATE_FONT_LIB:       /* 更新OSD 字库 */
        {
            OSD_LOGE("unsupport\n");
            break;
        }
        default:
        {
            OSD_LOGE("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
    }

    return iRet;
}


