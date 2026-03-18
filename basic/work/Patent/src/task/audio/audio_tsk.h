/**
 * @file   audio_tsk.h
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

#ifndef _AUDIO_TSK_H_
#define _AUDIO_TSK_H_

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
#define AUDIO_FRM_IDX_MAX (10)
    
#define AUDIO_FRM_LEN	(160)
#define AUDIO_FRM_CNT	(16)
    
#define AUDIO_AAC_FRM_LEN (160)
    
#define SDK_TALK_BACK_BUF_LEN	(2 * 1024)
#define AUD_TALK_BACK_BUF_LEN	(640 * 4) /* (2*1024) */
    
#define AUD_OUTPUT_BUF_LEN (10 * 1024)
    
#define AUD_ENC_TMP_BUF_LEN (4 * 1024)
    
#define AUDIO_AAC_RESAMPLE_FRM_LEN (512 * 1024)
    
#define ENC_TYPE_NUM (8)
    
#define AUDIO_PACK_MAX			(1)
#define AUDIO_PLAY_BACK_BUF_LEN (640 * 2 * 1024)



/*****************************************************************************
                            结构定义
*****************************************************************************/



/*****************************************************************************
                            函数声明
*****************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _AUDIO_TSK_H_ */



