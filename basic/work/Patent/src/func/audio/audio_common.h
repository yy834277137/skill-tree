/**
 * @file   audio_common.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---通用结构定义
 * @author yangzhifu
 * @date   2018年12月10日 Create
 * @note
 * @note \n History
   1.Date        : 2018年12月10日 Create
     Author      : yangzhifu
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，audio_drv中提取编解码接口
 */

#ifndef _AUDIO_COMMON_H_
#define _AUDIO_COMMON_H_

#include "sal.h"

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#define DEV_MAX_NUM        (2)
#define ENC_MAX_CHN_NUM    (3)

#define DEV_VISIBLE_LIGHT  (0)
#define DEV_INFRARED_LIGHT (1)

#define MAIN_STREAM_CHN    (0)
#define SUB_STREAM_CHN     (1)
#define THIRD_STREAM_CHN   (2)

#define AO_FRAME_LEN		(640)
#define WAV_FRAME_LEN		(640)

typedef struct 
{
   unsigned int     Channel;
   //unsigned int     bEnable;
   
   int              enType;   
   unsigned char    *pAudData;
   unsigned int     AudLen;

}AUD_DATA_EXCHANGE;

typedef struct 
{
   unsigned int     enStreamType; 
     
}AUD_CHN_INFO;
    
typedef struct 
{
    unsigned int   bEnable;
    AUD_CHN_INFO   AudChnInfo[ENC_MAX_CHN_NUM]; //0:main , 1:sub    
}AUD_INFO;
    
typedef struct
{ 
    AUD_INFO   AudInfo[DEV_MAX_NUM];      
}AUD_PACK_INFO;

typedef struct tagInterfaceInfoSt
{
    UINT32 bClrTmp;
    INT32 framenum;
    AUDIO_ENC_TYPE enDstType;       /* 音频输出类型，输入*/
    UINT32 reSample;
    UINT32 srcSampRate;
    
    UINT8 *pInputBuf;               /* 外部传入 */
    UINT32 uiInputLen;              /* 外部传入 */

    UINT8 *pOutputBuf;              /* 输出数据缓存 */
    UINT32 uiOutputBufLen;
    UINT32 uiOutputLen;             /* 输出 */

    UINT8 *pInTmpBuf;              /* 输入数据缓存 */
    UINT32 uiInTmpBufLen;
    UINT32 uiInTmpLen;             /* 待处理数据长度 */
} AUDIO_INTERFACE_INFO_S;  /* config this strcture when to call enc/dec  driver. */


/*****************************************************************************
                            函数声明
*****************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*_AUDIO_COMMON_H_*/







