/**
 * @file   venc_hostcmd_proc.h
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

#include <sal.h>

#include "dspcommon.h"
#include "audio_common.h"
#include "venc_tsk_api.h"
#include "jpeg_tsk_api.h"
#include "audio_tsk_api.h"

/*****************************************************************************
                                全局结构体
*****************************************************************************/



/*****************************************************************************
                                函数
*****************************************************************************/

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
INT32 CmdProc_encCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf)
{
    int iRet = SAL_SOK;

    /* AUD_PACK_INFO  stAudPackInfo; */

    SAL_LOGI("Chn: %d cmd: 0x%x !!!\n", chan, cmd);

    switch (cmd)
    {
        /* 指定chan视频源下的主码流 */
        case HOST_CMD_START_ENCODE:
        {
            if (SAL_SOK != (iRet = venc_tsk_start(chan, VENC_STREAM_ID_MAIN)))
            {
                SAL_ERROR("Enc Start Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_STOP_ENCODE:
        {
            if (SAL_SOK != (iRet = venc_tsk_stop(chan, VENC_STREAM_ID_MAIN)))
            {
                SAL_ERROR("Enc Stop Main Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_INSERT_I_FRAME:
        {
            if (SAL_SOK != (iRet = venc_tsk_forceIFrame(chan, VENC_STREAM_ID_MAIN)))
            {
                SAL_ERROR("Enc Force Main Encode Chn %d I Frame Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_SET_ENCODER_PARAM:
        {
            if (SAL_SOK != (iRet = venc_tsk_setPrm(chan, VENC_STREAM_ID_MAIN, pBuf)))
            {
                SAL_ERROR("Enc Set Main Encode Chn %d Param Failed !!!\n", chan);
            }

            /* 告知音频模块哪个设备的哪个通道需要音频数据 */
            iRet = audio_tsk_setPrm(chan, 0, pBuf);
            if (SAL_SOK != iRet)
            {
                SAL_ERROR("audio_tsk_setPrm Failed !!!\n");
            }

            break;
        }

        /* 指定chan视频源下的子码流 */
        case HOST_CMD_START_SUB_ENCODE:
        {
            if (SAL_SOK != (iRet = venc_tsk_start(chan, VENC_STREAM_ID_SUB)))
            {
                SAL_ERROR("Enc Start Sub Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_STOP_SUB_ENCODE:
        {
            if (SAL_SOK != (iRet = venc_tsk_stop(chan, VENC_STREAM_ID_SUB)))
            {
                SAL_ERROR("Enc Stop Sub Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_INSERT_SUB_I_FRAME:
        {
            if (SAL_SOK != (iRet = venc_tsk_forceIFrame(chan, VENC_STREAM_ID_SUB)))
            {
                SAL_ERROR("Enc Force Sub Encode Chn %d I Frame Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_SET_SUB_ENCODER_PARAM:
        {
            if (SAL_SOK != (iRet = venc_tsk_setPrm(chan, VENC_STREAM_ID_SUB, pBuf)))
            {
                SAL_ERROR("Enc Set Main Encode Chn %d Param Failed !!!\n", chan);
            }

            /* 告知音频模块哪个设备的哪个通道需要音频数据 */

            iRet = audio_tsk_setPrm(chan, 1, pBuf);
            if (SAL_SOK != iRet)
            {
                SAL_ERROR("audio_tsk_setPrm Failed !!!\n");
            }

            break;
        }

        /* 指定chan视频源下的thied码流 */
        case HOST_CMD_START_THIRD_ENCODE:
        {
            if (SAL_SOK != (iRet = venc_tsk_start(chan, VENC_STREAM_ID_THRD)))
            {
                SAL_ERROR("Enc Start Sub Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }

        case HOST_CMD_STOP_THIRD_ENCODE:
        {
            if (SAL_SOK != (iRet = venc_tsk_stop(chan, VENC_STREAM_ID_THRD)))
            {
                SAL_ERROR("Enc Stop Sub Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_INSERT_THIRD_I_FRAME:
        {
            if (SAL_SOK != (iRet = venc_tsk_forceIFrame(chan, VENC_STREAM_ID_THRD)))
            {
                SAL_ERROR("Enc Force Sub Encode Chn %d I Frame Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_SET_THIRD_ENCODE_PARAM:
        {
            if (SAL_SOK != (iRet = venc_tsk_setPrm(chan, VENC_STREAM_ID_THRD, pBuf)))
            {
                SAL_ERROR("Enc Set Main Encode Chn %d Param Failed !!!\n", chan);
            }

            /* 告知音频模块哪个设备的哪个通道需要音频数据 */

            iRet = audio_tsk_setPrm(chan, 2, pBuf);
            if (SAL_SOK != iRet)
            {
                SAL_ERROR("audio_tsk_setPrm Failed !!!\n");
            }

            break;
        }

        case HOST_CMD_ENC_DRAW_SWITCH:
        {
            SAL_ERROR("chan %u unsupport set draw !!!\n", chan);
            break;
        }

        /* 抓图的配置，命令发送给抓图的管理者 */
        case HOST_CMD_START_ENC_JPEG:       /* 开启编码抓图功能 */
        {
            if (SAL_SOK != (iRet = jpeg_tsk_start(chan, (JPEG_PARAM *)pBuf)))
            {
                SAL_ERROR("Enc Start Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_STOP_ENC_JPEG:       /* 停止编码抓图功能 */
        {
            if (SAL_SOK != (iRet = jpeg_tsk_stop(chan, (JPEG_PARAM *)pBuf)))
            {
                SAL_ERROR("Enc Stop Encode Chn %d Failed !!!\n", chan);
            }

            break;
        }
        case HOST_CMD_SET_ENC_JPEG_PARAM:      /* 设置编码抓图属性 */
        {
            if (SAL_SOK != (iRet = jpeg_tsk_setPrm(chan, (JPEG_PARAM *)pBuf)))
            {
                SAL_ERROR("Enc Set Encode Chn %d Prm Failed !!!\n", chan);
            }

            break;
        }
        default:
        {
            SAL_ERROR("MODULE_VENC: <0x%x> is nonsupport !!!\n", cmd);
            break;
        }
    }

    return iRet;
}

