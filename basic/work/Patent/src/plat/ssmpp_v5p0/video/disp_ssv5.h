/**
 * @file   disp_ssv5.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  显示模块平台层头文件
 * @author liuxianying
 * @date   2021/8/4
 * @note
 * @note \n History
   1.日    期: 2021/8/4
     作    者: liuxianying
     修改历史: 创建文件
 */
#ifndef _DISP_SSV5_H_
#define _DISP_SSV5_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <sal.h>
#include <platform_sdk.h>
#include <dspcommon.h>
#include "platform_hal.h"



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


typedef struct tagDispTimingMap
{
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Fps;
    ot_vo_intf_sync enVoTiming;
    BT_TIMING_E enUserTiming;
} DISP_TIMING_MAP_S;

INT32 Hdmi_Hal_Init(VOID);
INT32 Hdmi_Hal_DeInit(VOID);



#endif


