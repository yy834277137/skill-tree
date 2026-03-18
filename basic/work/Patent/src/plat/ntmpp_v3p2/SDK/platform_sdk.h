
#ifndef _PLATFORM_SDK_H_
#define _PLATFORM_SDK_H_

/* 芯片平台SDK依赖的头文件放在这里，按照需要一项项加进来，platform 目录外的程序是不用保护下面文件的 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "hdal_33x/include/hd_audiocapture.h"
#include "hdal_33x/include/hd_audiodec.h"
#include "hdal_33x/include/hd_audioenc.h"
#include "hdal_33x/include/hd_audioout.h"
#include "hdal_33x/include/hd_common.h"
#include "hdal_33x/include/hd_debug.h"
#include "hdal_33x/include/hd_gfx.h"
#include "hdal_33x/include/hd_logger.h"
#include "hdal_33x/include/hd_type.h"
#include "hdal_33x/include/hd_util.h"
#include "hdal_33x/include/hd_videocapture.h"
#include "hdal_33x/include/hd_videodec.h"
#include "hdal_33x/include/hd_videoenc.h"
#include "hdal_33x/include/hd_videoout.h"
#include "hdal_33x/include/hd_videoprocess.h"
#include "hdal_33x/include/hdal.h"
#include "hdal_33x/include/vendor/vendor_audioio.h"
#include "hdal_33x/include/vendor/vendor_common.h"
#include "hdal_33x/include/vendor/vendor_gfx.h"
#include "hdal_33x/include/vendor/vendor_videocap.h"
#include "hdal_33x/include/vendor/vendor_videodec.h"
#include "hdal_33x/include/vendor/vendor_videoenc.h"
#include "hdal_33x/include/vendor/vendor_videoout.h"
#include "hdal_33x/include/vendor/vendor_videoproc.h"
#include "hdal_33x/include/vendor/vendor_ai.h"
#include "hdal_33x/include/vendor/vendor_ai_cpu/vendor_ai_cpu.h"
#include "hdal_33x/vendor/isp/include/vendor_video.h"
#include "hdal_33x/k_driver/include/nvtmem_if.h"
#include "hdal_33x/k_driver/include/i2c-dev.h"
#include "hdal_33x/source/videoprocess.h"


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define DDR1_ADDR_START 0x100000000


#ifdef __ARM_32BIT_STATE
#define OT_ADDR_P2U64(addr_point)   ((UINT64)(addr_point))
#define OT_ADDR_U642P(addr_u64)     ((void *)(addr_u64))
#else // __ARM_64BIT_STATE
#define OT_ADDR_P2U64(addr_point)   ((UINT64)(addr_point))
#define OT_ADDR_U642P(addr_u64)     ((void *)(addr_u64))
#endif

#ifdef __ARM_32BIT_STATE
#define HI_ADDR_P2U64(addr_point)   ((UINT64)(addr_point))
#define HI_ADDR_U642P(addr_u64)     ((void *)(addr_u64))
#else // __ARM_64BIT_STATE
#define HI_ADDR_P2U64(addr_point)   ((UINT64)(addr_point))
#define HI_ADDR_U642P(addr_u64)     ((void *)(addr_u64))
#endif

#endif