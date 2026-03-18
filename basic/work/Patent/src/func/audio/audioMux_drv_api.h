/**
 * @file   audioMux_drv_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---音频封装接口
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取音频封装接口
 */
 
#ifndef _AUDIO_MUX_DRV_API_H_
#define _AUDIO_MUX_DRV_API_H_

/*****************************************************************************
                        头文件
*****************************************************************************/
#include "sal.h"
#include "audio_common.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/*****************************************************************************
                            结构
*****************************************************************************/
typedef struct tagAudStreamPackInfost
{
    UINT32 bNeedAud;
    UINT32 bNeedPsPack;
    UINT32 bNeedRtpPack;
} STREAM_PACK_INFO_S;

typedef struct tagAudChnInfo
{
    STREAM_PACK_INFO_S stStreamPackInfo[ENC_MAX_CHN_NUM];
} CHN_INFO_S;

typedef struct tagAudBitsInfoSt
{
    UINT8 *pBitsDataAddr;               /* 码流地址 */
    UINT32 u32BitsDataLen;              /* 码流长度 */
    UINT64 u64TimeStamp;
    AUDIO_ENC_TYPE enAudioEncType;            /* 编码类型 */
} AUD_BITS_INFO_S;

typedef struct tagAudioPakcInfost
{
    UINT32 u32Chn;
    UINT32 isUsePs;
    UINT32 isUseRtp;
    UINT32 u32PsHandle;
    UINT32 u32RtpHandle;
    UINT32 u32PackBufSize;
    UINT32 u32PackSize;
    UINT8 *pPackOutputBuf;   /* RTP/PS 打包后的码流输出地址, RTP与PS复用 */
} AUDIO_PACK_INFO_S;

/*****************************************************************************
                        函数
*****************************************************************************/

/**
 * @function   audioMux_drv_init
 * @brief    音频封装组件初始化
 * @param[in]  AUDIO_PACK_INFO_S *pstAudPackInfo 封装参数指针
 * @param[out] AUDIO_PACK_INFO_S *pstAudPackInfo 封装句柄
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioMux_drv_init(AUDIO_PACK_INFO_S *pstAudPackInfo);
/**
 * @function   audioMux_drv_rtpProcess
 * @brief    音频RTP封装
 * @param[in]  AUDIO_PACK_INFO_S *pstAudPackInfo 封装参数指针
 * @param[in]  AUD_BITS_INFO_S *pstBitDataInfo 输入码流
 * @param[out] AUDIO_PACK_INFO_S *pstAudPackInfo 封装后的码流
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioMux_drv_rtpProcess(AUDIO_PACK_INFO_S *pstAudPackInfo, AUD_BITS_INFO_S *pstBitDataInfo);
/**
 * @function   audioMux_drv_psProcess
 * @brief    音频PS封装
 * @param[in]  AUDIO_PACK_INFO_S *pstAudPackInfo 封装参数指针
 * @param[in]  AUD_BITS_INFO_S *pstBitDataInfo 输入码流
 * @param[out] AUDIO_PACK_INFO_S *pstAudPackInfo 封装后的码流
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 audioMux_drv_psProcess(AUDIO_PACK_INFO_S *pstAudPackInfo, AUD_BITS_INFO_S *pstBitDataInfo);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_MUX_DRV_H_ */

