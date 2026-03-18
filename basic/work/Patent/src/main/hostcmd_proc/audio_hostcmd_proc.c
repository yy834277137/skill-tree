/**
 * @file   audio_hostcmd_proc.c
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


#include <sal.h>

#include "audio_tsk_api.h"

/*****************************************************************************
                                函数
*****************************************************************************/


/**
 * @function   CmdProc_audioCmdProc
 * @brief   DSP 模块audio的处理命令
 * @param[in]  HOST_CMD cmd APP下发DSP的主机命令
 * @param[in]  UINT32 chan 本通道参数是指独立的音频源
 * @param[in]  VOID *pBuf 下发的参数
 * @param[out] None
 * @return      INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 CmdProc_audioCmdProc(HOST_CMD cmd, UINT32 chan, VOID *pBuf)
{
    int iRet = SAL_FAIL;

    /* AUDIO_VOL_PRM_S *pstAudioVol = NULL; */

    SAL_LOGI("Chn: %d cmd: %x !!!\n", chan, cmd);

    switch (cmd)
    {
        case HOST_CMD_MODULE_AUDIO:     /* 音频模块的能力级 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_SET_AUDIO_LOOPBACK:      /* 设置音频采集回环 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_SET_DEC_PLAY:      /* 设置音频解码播放 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_SET_TALKBACK:       /* 设置语音对讲功能属性 */
        {
            iRet = audio_tsk_setTalkback(chan, pBuf);
            break;
        }
        case HOST_CMD_START_AUDIO_PLAY:       /* 开启解码语音播报 */
        {
            //iRet = audio_tsk_playStart(chan, pBuf);
            iRet = audio_tsk_playStartPCM(chan, pBuf);/* 只播放PCM格式音频文件*/
            break;
        }
        case HOST_CMD_STOP_AUDIO_PLAY:       /* 停止语音播报 */
        {
            iRet = audio_tsk_playStop(chan, pBuf);
            break;
        }
        case HOST_CMD_START_AUTO_ANSWER:      /* 开启自动应答功能 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_STOP_AUTO_ANSWER:      /* 停止自动应答功能 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_START_AUDIO_RECORD:      /* 启动语音录制功能 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_STOP_AUDIO_RECORD:       /* 停止语音录制功能 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_START_AUDIO_OUTPUT:       /* 启动语音输出 */
        {
            //调试使用iRet = audio_tsk_soundStart(chan);
            break;
        }
        case HOST_CMD_STOP_AUDIO_OUTPUT:       /* 停止语音输出 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_START_AUDIO_INPUT:       /* 启动语音输入并编码 */
        {
            iRet = audio_tsk_InputEnc(chan, pBuf);
            break;
        }
        case HOST_CMD_STOP_AUDIO_INPUT:       /* 停止语音输入 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_SET_AUDIO_ALARM_LEVEL:       /* 设置声音超限报警阈值 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_SET_AI_VOLUME:       /* 设置音频输入音量 */
        {
            /* pstAudioVol = (AUDIO_VOL_PRM_S *)pBuf; */
            /* iRet = audio_tsk_setAiVolume(pstAudioVol->uiChn, pstAudioVol); */
            iRet = audio_tsk_setAiVolume(0, pBuf);
            break;
        }
        case HOST_CMD_SET_AO_VOLUME:      /* 设置音频输出音量 */
        {
            /* pstAudioVol = (AUDIO_VOL_PRM_S *)pBuf; */
            /* iRet = audio_tsk_setAoVolume(pstAudioVol->uiChn, pstAudioVol); */
            iRet = audio_tsk_setAoVolume(0, pBuf);
            break;
        }
        case HOST_CMD_SET_EAR_PIECE:      /* 设置听筒免提切换 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_START_SOUND_RECORD:       /* 开启音频输入录音 */
        {
            iRet = audio_tsk_soundInput(chan);
            break;
        }
        case HOST_CMD_STOP_SOUND_RECORD:      /* 停止音频输入录音 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_SET_MANAGE_TALKTO_RESIDENT:      /* 启动网络设备与模拟室内机对讲功能 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_SET_DOOR_TALKTO_RESIDENT:       /* 门口机与模拟室内机对讲功能 */
        {
            SAL_ERROR("CMD <%x> is ERROR !!!\n", cmd);
            break;
        }
        case HOST_CMD_SET_AUDIO_ENC_TYPE:      /* 设置音频编码类型 */
        {
            break;
        }
        case HOST_CMD_SET_AUDIO_TTS_START:
        {
            iRet = audio_tsk_ttsStart(0, pBuf);
            break;
        }
        case HOST_CMD_SET_AUDIO_TTS_HANDLE:
        {
            iRet = audio_tsk_ttsProcess(0, pBuf);
            break;
        }
        case HOST_CMD_SET_AUDIO_TTS_STOP:
        {
            iRet = audio_tsk_ttsStop(0, pBuf);
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

