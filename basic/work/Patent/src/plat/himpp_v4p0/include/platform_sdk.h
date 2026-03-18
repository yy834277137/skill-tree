
#ifndef _PLATFORM_SDK_H_
#define _PLATFORM_SDK_H_

/* 芯片平台SDK依赖的头文件放在这里，按照需要一项项加进来，platform 目录外的程序是不用保护下面文件的 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "sdk_inc_hi3559a_sdk030/hi_common.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_sys.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_vb.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_isp.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_vi.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_vo.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_venc.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_video.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_vpss.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_region.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_adec.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_aenc.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_ai.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_ao.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_aio.h"
#include "sdk_inc_hi3559a_sdk030/hi_defines.h"
#include "sdk_inc_hi3559a_sdk030/hi_dsp.h"
#include "sdk_inc_hi3559a_sdk030/hi_tde_type.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_ive.h"
#include "sdk_inc_hi3559a_sdk030/hi_ae_comm.h"
#include "sdk_inc_hi3559a_sdk030/hi_af_comm.h"
#include "sdk_inc_hi3559a_sdk030/hi_buffer.h"
#include "sdk_inc_hi3559a_sdk030/hi_unf_cipher.h"
#include "sdk_inc_hi3559a_sdk030/hi_types.h"
#include "sdk_inc_hi3559a_sdk030/ivs_md.h"
#include "sdk_inc_hi3559a_sdk030/mpi_ive.h"
#include "sdk_inc_hi3559a_sdk030/mpi_audio.h"
#include "sdk_inc_hi3559a_sdk030/mpi_sys.h"
#include "sdk_inc_hi3559a_sdk030/mpi_vb.h"
#include "sdk_inc_hi3559a_sdk030/mpi_vi.h"
#include "sdk_inc_hi3559a_sdk030/mpi_vo.h"
#include "sdk_inc_hi3559a_sdk030/mpi_venc.h"
#include "sdk_inc_hi3559a_sdk030/mpi_vpss.h"
#include "sdk_inc_hi3559a_sdk030/mpi_region.h"
#include "sdk_inc_hi3559a_sdk030/mpi_vdec.h"
#include "sdk_inc_hi3559a_sdk030/mpi_dsp.h"
#include "sdk_inc_hi3559a_sdk030/mpi_vgs.h"
#include "sdk_inc_hi3559a_sdk030/hi_tde_api.h"
#include "sdk_inc_hi3559a_sdk030/hi_type.h"

#include "sdk_inc_hi3559a_sdk030/mpi_isp.h"
#include "sdk_inc_hi3559a_sdk030/mpi_ae.h"
#include "sdk_inc_hi3559a_sdk030/mpi_awb.h"

#include "sdk_inc_hi3559a_sdk030/hi_sns_ctrl.h"
#include "sdk_inc_hi3559a_sdk030/hi_mipi.h"
#include "sdk_inc_hi3559a_sdk030/hi_mipi_tx.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_vgs.h"
#include "sdk_inc_hi3559a_sdk030/hifb.h"

#include "sdk_inc_hi3559a_sdk030/hi_md.h"
#include "sdk_inc_hi3559a_sdk030/hi_math.h"

#include "sdk_inc_hi3559a_sdk030/hi_osal.h"
#include "sdk_inc_hi3559a_sdk030/osal_list.h"
#include "sdk_inc_hi3559a_sdk030/osal_mmz.h"

#include "sdk_inc_hi3559a_sdk030/acodec.h"
#include "sdk_inc_hi3559a_sdk030/mpi_hdmi.h"
#include "sdk_inc_hi3559a_sdk030/hi_comm_hdmi.h"
//#include "sdk_inc_hi3559a_sdk030/DDM_reset.h"
#include "sdk_inc_hi3559a_sdk030/i2c-dev.h"



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#ifdef __ARM_32BIT_STATE
#define HI_ADDR_P2U64(addr_point)   ((HI_U64)(HI_PHYS_ADDR_T)(addr_point))
#define HI_ADDR_U642P(addr_u64)     ((void *)(HI_PHYS_ADDR_T)(addr_u64))
#else // __ARM_64BIT_STATE
#define HI_ADDR_P2U64(addr_point)   ((HI_U64)(addr_point))
#define HI_ADDR_U642P(addr_u64)     ((void *)(addr_u64))
#endif


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

#endif

