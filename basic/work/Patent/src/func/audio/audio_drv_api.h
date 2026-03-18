/**
 * @file   audio_drv_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---fun层接口调用
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建audio_drv.h文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */

#ifndef _AUDIO_DRV_API_H_
#define _AUDIO_DRV_API_H_

/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal.h"
#include "sal_audio.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/*****************************************************************************
                         宏定义
*****************************************************************************/

/*****************************************************************************
                         结构定义
*****************************************************************************/
typedef struct tagInternalInfoSt
{
    UINT8 *pDataBuf;
    UINT32 u32BufSize;
    UINT32 u32RestDataLen;  /* 送播数据中的剩余长度 */
} AUDIO_BUF_PRM_S; 

/*****************************************************************************
                            函数声明
*****************************************************************************/
/**
 * @function   audio_drv_setAudioTalkBack
 * @brief      开始/结束 SDK 对讲
 * @param[in]  UINT32 u32AudioChan 音频通道
 * @param[in]  void *pBuf 对讲使能参数
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_setAudioTalkBack(UINT32 u32AudioChan, void *pBuf);
/**
 * @function   audio_drv_setAiVolume
 * @brief      配置输入音量等级
 * @param[in]  UINT32 u32AudioChan 音频通道，当前无效
 * @param[in]  void *pstAudioVol 音频信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_setAiVolume(UINT32 u32AudioChan, void *pstAudioVol);
/**
 * @function   audio_drv_aiGetFrame
 * @brief      获取一帧音频数据
 * @param[in]  SAL_AudioFrameBuf **pstAudFrmBuf 音频采集到的PCM数据
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_aiGetFrame(SAL_AudioFrameBuf **pstAudFrmBuf);

/**
 * @function   audio_drv_setAoVolume
 * @brief      配置输出音量等级
 * @param[in]  UINT32 u32AudioChan 音频通道，当前无效
 * @param[in]  void *pstAudioVol 音频信息
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_setAoVolume(UINT32 u32AudioChan, void *pstAudioVol);
/**
 * @function   audio_drv_aoSendFrame
 * @brief      播放一帧音频数据
 * @param[in]  UINT8 *pu8AudioDat 待输出音频数据
 * @param[in]  UINT32 len 音频数据长度
 * @param[in]  AUDIO_BUF_PRM_S *pstAoBufPrm 音频输出中转buf
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_aoSendFrame(UINT8 *pu8AudioDat, UINT32 len, AUDIO_BUF_PRM_S *pstAoBufPrm);
/**
 * @function   audio_drv_init
 * @brief      音频模块销毁
 * @param[in]  void
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_deInit(void);
/**
 * @function   audio_drv_init
 * @brief      audio drv层初始化资源
 * @param[in]  void
 * @param[out] None
 * @return     INT32  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_drv_init(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_DRV_H_ */


