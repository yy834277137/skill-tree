/**
 * @file   audio_hal_inter.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---hal层接口封装
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */


#ifndef _AUDIO_HAL_INTER_H_
#define _AUDIO_HAL_INTER_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <sal.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

typedef struct
{
    UINT32 u32Chan;         /*audio channel number*/
    UINT32 u32AudioDevIdx;  /*audio device index*/    
    UINT32 u32SampleRate;   /*audio sampling rate*/
    UINT32 u32BitWidth;
    UINT32 u32SoundMode;
    UINT32 u32FrameRate;
}HAL_AIO_ATTR_S;

typedef struct tagAudioPlatOpsInfo
{    
    INT32 (*aoCreate)(HAL_AIO_ATTR_S *);
    void (*aoDestroy)(HAL_AIO_ATTR_S *);
    INT32 (*aoSendFrame)(HAL_AIO_FRAME_S *);
    INT32 (*aoSetVolume)(UINT32, UINT32);

    INT32 (*aiCreate)(HAL_AIO_ATTR_S *);
    void (*aiDestroy)(HAL_AIO_ATTR_S *);
    INT32 (*aiGetFrame)(HAL_AIO_FRAME_S *);
    INT32 (*aiSetVolume)(UINT32, UINT32);
    INT32 (*aiEnableVqe)(UINT32);
    INT32 (*aiDisableVqe)(UINT32);
    INT32 (*aiSetLoopBack)(UINT32);
    /*tts*/
    UINT32 (*ttsGetSampleRate)(void);/*hik tts 16k, kdxf tts 8k*/
    INT32 (*ttsStart)(void **, UINT32);
    INT32 (*ttsProc)(void *, PUINT8, UINT32, PUINT8, UINT32 *);
    INT32 (*ttsStop)(void**);
}AUDIO_PLAT_OPS_S;

/**
 * @function   audio_hal_register
 * @brief      注册hal层回调函数
 * @param[in]  None
 * @param[out] AUDIO_PLAT_OPS_S * 回调函数结构指针
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audio_hal_register(AUDIO_PLAT_OPS_S * pstAudioPlatOps);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /*_AUDIO_HAL_INTER_H_*/


