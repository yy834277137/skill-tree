/*******************************************************************************
 * platform_hal.h
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : heshengyuan <heshengyuan@hikvision.com>
 * Version: V1.0.0  2019年1月4日 Create
 *
 * Description : 对芯片平台封装后，形成硬件抽象层的接口，对外上层代码开发的函数和数据结构
                 会生成部分头文件被公共业务代码使用和调用。这些硬件模块抽象的头文件，统一
                 放到这里来，上层业务代码统一调用 platform_hal.h 即可。

                 注意项1: 全部包含 platform 目录下各个子目录的头文件，肯定是最方便简单的，
                 但是确外部引用了不使用的数据结构，随着项目变化，会变成你最不想遇见的各种
                 凌乱，所以这里只放外部需要的使用的即可，不多放、不少放。
                 
                 主要项2: platform目录对外开发的头文件之间一定一定需要避免交叉引用。
 * Modification: 
 *******************************************************************************/

#ifndef _PLATFORM_HAL_H_
#define _PLATFORM_HAL_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "hal_inc_exter/sys_hal_api.h"
#include "hal_inc_exter/audio_hal_api.h"
#include "hal_inc_exter/disp_hal_api.h"
#include "hal_inc_exter/dup_hal_api.h"
#include "hal_inc_exter/dup_common_api.h"
#include "hal_inc_exter/fb_hal_api.h"
#include "hal_inc_exter/venc_hal_api.h"
#include "hal_inc_exter/vdec_hal_api.h"
#include "hal_inc_exter/osd_hal_api.h"
#include "hal_inc_exter/mem_hal_api.h"
//include "hal_inc_exter/vda_hal_drv.h"
#include "hal_inc_exter/vgs_hal_api.h"
#include "hal_inc_exter/tde_hal.h"
#include "hal_inc_exter/ive_hal.h"
#include "hal_inc_exter/capt_hal_api.h"
#include "hal_inc_exter/svp_dsp_hal_api.h"

#include "hal_inc_exter/hal_comm_neon.h"
#include "hal_inc_exter/DDM_dp.h"
#include "hal_inc_exter/DDM_displayReset.h"
#include "hal_inc_exter/DDM_readinfo.h"
#include "hal_inc_exter/DDM_sound.h"


#include "hal_inc_exter/bt_timing.h"
#include "hal_inc_exter/iic.h"
#include "hal_inc_exter/uart.h"
#include "hal_inc_exter/hardware_hal.h"
#include "hal_inc_exter/cipher_hal.h"
#include "hal_inc_exter/video_inout_base.h"
#include "hal_inc_exter/ai_hal_api.h"



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : plat_hal_init
* 描  述  : palt芯片平台层初始化
* 输  入  : - void
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 plat_hal_init(void);



#endif
