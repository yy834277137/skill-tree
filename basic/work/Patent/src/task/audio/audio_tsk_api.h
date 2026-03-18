/**
 * @file   audio_tsk_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---音频业务模块
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取码流业务逻辑
 */

#ifndef _AUDIO_TSK_API_H_
#define _AUDIO_TSK_API_H_

/*****************************************************************************
                           头文件
*****************************************************************************/
#include "sal.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/*****************************************************************************
                            宏定义
*****************************************************************************/

/*****************************************************************************
                            结构定义
*****************************************************************************/

/*****************************************************************************
                            函数声明
*****************************************************************************/
/**
 * @function   audio_tsk_init
 * @brief    audio tsk层初始化资源。
 * @param[in]  void
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_init();
/**
 * @function   audio_tsk_deInit
 * @brief    audio tsk层去初始化。
 * @param[in]  void
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_deInit();
/**
 * @function   audio_tsk_ttsStart
 * @brief    合成语音开始
 * @param[in]  UINT32 u32Chn 通道号
 * @param[in]  void *prm 初始化参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_ttsStart(UINT32 u32Chn, void *prm);
/**
 * @function   audio_tsk_ttsProcess
 * @brief    合成语音处理
 * @param[in]  UINT32 u32Chn 音频通道，无效
 * @param[in]  void *prm 输入数据
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_ttsProcess(UINT32 u32Chn, void *prm);
/**
 * @function   audio_tsk_ttsStop
 * @brief    合成语音关闭
 * @param[in]  UINT32 u32Chn 音频通道，无效
 * @param[in]  void *prm 输入参数，无效
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_ttsStop(UINT32 u32Chn, void *prm);
/**
 * @function   audio_tsk_setAoVolume
 * @brief    配置输出音量等级
 * @param[in]  UINT32 u32Chn 通道号
 * @param[in]  void *prm 音量参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_setAoVolume(UINT32 u32Chn, void *prm);
/**
 * @function   audio_tsk_setAiVolume
 * @brief   配置输入音量等级
 * @param[in]  UINT32 u32Chn 通道号
 * @param[in]  void *prm 音量参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_setAiVolume(UINT32 u32Chn, void *prm);
/**
 * @function   audio_tsk_setTalkback
 * @brief  开始/结束  SDK 对讲
 * @param[in]  UINT32 u32Chn 通道号
 * @param[in]  void *prm 对讲参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_setTalkback(UINT32 u32Chn, void *prm);
/**
 * @function   audio_tsk_playStop
 * @brief 停止语音播放
 * @param[in]  UINT32 u32Chn 通道号，无效
 * @param[in]  void *prm 无效
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_playStop(UINT32 u32Chn, void *prm);
/**
 * @function   audio_tsk_playStart
 * @brief 设置播放语音参数/开始播放
 * @param[in]  UINT32 u32Chn 通道号，无效
 * @param[in]  void *pBuf 语音播报参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_playStart(UINT32 u32Chn, void *pBuf);
/**
 * @function   audio_tsk_playStartPCM
 * @brief 设置播放语音参数/开始播放只播放PCM格式文件
 * @param[in]  UINT32 u32Chn 通道号，无效
 * @param[in]  void *pBuf 语音播报参数
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_playStartPCM(UINT32 u32Chn, void *pBuf);

/**
* @function   audio_tsk_decPreviewStop
* @brief 停止音频解码播放
* @param[in]  UINT32 u32VdecChn 解码通道号
* @param[out] None
* @return      int  成功SAL_SOK，失败SAL_FAIL
*/
INT32 audio_tsk_decPreviewStop(UINT32 u32VdecChn);
/**
* @function   audio_tsk_decPreviewStart
* @brief 开启音频解码播放
* @param[in]  UINT32 u32VdecChn 解码通道号
* @param[out] None
* @return      int  成功SAL_SOK，失败SAL_FAIL
*/
INT32 audio_tsk_decPreviewStart(UINT32 u32VdecChn);
/**
 * @function   audio_tsk_decPreviewWriteData
 * @brief IPC/回放 解析到的音频数据写入对应的datapool
 * @param[in]  UINT8 *pAudFrm 音频数据
 * @param[in]  UINT32 u32InLen 音频数据长度
 * @param[in]  UINT32 u32AudType 音频类型
 * @param[in]  UINT8 *pExtAudFrm 音频扩展头
 * @param[in]  UINT32 u32InExtLen 音频头长度
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_decPreviewWriteData(UINT8 *pAudFrm, UINT32 u32InLen, UINT32 u32AudType, UINT8 *pExtAudFrm, UINT32 u32InExtLen);
/**
 * @function   audio_tsk_setPrm
 * @brief    外部只需配置哪个设备(可见光还是红外光)的哪个通道(主码流还是子码流)需要声音数据。
 * @param[in]  UINT32 u32Dev 哪个设备(可见光还是红外光);
 * @param[in]  UINT32 u32Chn 主码流还是子码流.;
 * @param[in]  void *prm 编码参数;
 * @param[out] None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_setPrm(UINT32 u32Dev, UINT32 u32Chn, void *prm);
/**
* @function   		audio_tsk_Save
* @brief			音频编码并保存
* @param[in]  void *prm 无效
* @param[out]	None
* @return		void
*/
void *audio_tsk_Save(void *prm);
/**
* @function		audio_tsk_playSave
* @brief		保存音频文件
* @param[in]  void *pPrm 无效参数
* @param[out] None
* @return	   void
*/
void *audio_tsk_playSave(void *pPrm);
/**
* @function		audio_tsk_soundStart
* @brief		设置播放文件语音参数/开始播放
* @param[in]  UINT32 u32Chn 通道号，无效
* @param[out] None
* @return		int  成功SAL_SOK，失败SAL_FAIL
*/
INT32 audio_tsk_soundStart(UINT32 u32Chn);
/**
 * @function   audio_tsk_encStart
 * @brief 设置开始编码
 * @param[in]  AUDIO_PLAY_PARAM *pstAudioPlayParam 需要编码的音频信息
 * @param[in]  UINT32 *pInLen 音频编码长度（640）
 * @param[out] UINT8 **ppAudFrm 编码后的输出数据
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_tsk_encStart(UINT8 **ppAudFrm, UINT32 *pInLen, AUDIO_PLAY_PARAM *pstAudioPlayParam);
/**
* @function   audio_tsk_InputEnc
* @brief		开启音频输入录音并编码
* @param[in]  UINT32 u32Chn 通道号，无效
* @param[in]  void *pData 音频信号输入
* @return		int  成功SAL_SOK，失败SAL_FAIL
*/
INT32 audio_tsk_InputEnc(UINT32 u32Chn,  void *pData);
/**
* @function   audio_tsk_soundInput
* @brief		开启音频输入录音
* @param[in]  UINT32 u32Chn 通道号
* @return		int  成功SAL_SOK，失败SAL_FAIL
*/
INT32 audio_tsk_soundInput(UINT32 u32Chn);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_TSK_H_ */




