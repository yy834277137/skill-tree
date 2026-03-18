/*******************************************************************************
* disp_hal_drv.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : huangshuxin <huangshuxin@hikvision.com>
* Version: V1.0.0  2019年1月23日 Create
*
* Description :
* Modification:
*******************************************************************************/
#ifndef _DISP_HISI_H_
#define _DISP_HISI_H_

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
    VO_INTF_SYNC_E enVoTiming;
    BT_TIMING_E enUserTiming;
} DISP_TIMING_MAP_S;

INT32 Hdmi_Hal_Init(VOID);



#endif


