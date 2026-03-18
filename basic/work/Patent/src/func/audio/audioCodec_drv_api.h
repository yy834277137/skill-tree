/**
 * @file   audioCodec_drv_api.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  音频组件---编解码接口
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

#ifndef _AUDIO_CODEC_DRV_API_H_
#define _AUDIO_CODEC_DRV_API_H_

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

#define AAC_ENC_SRC_FRM_LEN (2048)

#define AUDIO_CODEC_NEED_MORE_DATA	(1)


/*****************************************************************************
                            结构
*****************************************************************************/
/*  实现音频编解码的回调函数，每种编解码类型对应一组 ENC_OPERATION 结构体。*/
typedef struct
{    
    UINT32 bNeedHndl;
    void *hndl;
    
    int (*Init)(void **);
    int (*PreProc)(AUDIO_INTERFACE_INFO_S *);
    int (*Proc)(void *, AUDIO_INTERFACE_INFO_S *);
    int (*DeInit)(void **);
} CODEC_OPERATION_S;

/*****************************************************************************
                            函数
*****************************************************************************/
/**
 * @function   audioCodec_drv_encRegister
 * @brief    注册音频编码函数
 * @param[in]  UINT32 u32ArraySize 数组大小
 * @param[in]  CODEC_OPERATION_S *pstOperate 编码相关函数指针数组
 * @param[out] None
 * @return     void
 */
void audioCodec_drv_encRegister(UINT32 u32ArraySize,  CODEC_OPERATION_S *pstOperate);
/**
 * @function   audioCodec_drv_decRegister
 * @brief    注册音频解码函数
 * @param[in]  UINT32 u32ArraySize 数组大小
 * @param[in]  CODEC_OPERATION_S *pstOperate 解码相关函数指针数组
 * @param[out] None
 * @return     void
 */
void audioCodec_drv_decRegister(UINT32 u32ArraySize, CODEC_OPERATION_S *pstOperate);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_CODEC_DRV_H_ */



