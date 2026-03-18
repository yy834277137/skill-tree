
#ifndef _PLATFORM_SDK_H_
#define _PLATFORM_SDK_H_

/* 芯片平台SDK依赖的头文件放在这里，按照需要一项项加进来，platform 目录外的程序是不用保护下面文件的 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "sdk_inc_ss928v100_sdk010/acl_base.h"
#include "sdk_inc_ss928v100_sdk010/acl_ext.h"
#include "sdk_inc_ss928v100_sdk010/acl.h"
#include "sdk_inc_ss928v100_sdk010/acl_mdl.h"
#include "sdk_inc_ss928v100_sdk010/acl_op.h"
#include "sdk_inc_ss928v100_sdk010/acl_prof.h"
#include "sdk_inc_ss928v100_sdk010/acl_rt.h"
#include "sdk_inc_ss928v100_sdk010/autoconf.h"
#include "sdk_inc_ss928v100_sdk010/ot_aacdec.h"
#include "sdk_inc_ss928v100_sdk010/ot_aacenc.h"
#include "sdk_inc_ss928v100_sdk010/ot_acodec.h"
#include "sdk_inc_ss928v100_sdk010/ot_ao_export.h"
#include "sdk_inc_ss928v100_sdk010/ot_buffer.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_3a.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_adec.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_ae.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_aenc.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_af.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_aio.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_avs.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_avs_lut_generate.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_avs_pos_query.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_awb.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_cipher.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_dis.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_dpu_match.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_dpu_rect.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_dsp.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_fisheye_calibration.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_gdc.h"
#include "sdk_inc_ss928v100_sdk010/ot_common.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_hdmi.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_isp.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_ive.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_klad.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_mau.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_mcf_calibration.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_mcf.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_mcf_vi.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_md.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_motionfusion.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_otp.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_pciv.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_rc.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_region.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_sns.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_svp.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_sys.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_tde.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_vb.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_vdec.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_venc.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_vgs.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_video.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_vi.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_vo.h"
#include "sdk_inc_ss928v100_sdk010/ot_common_vpss.h"
#include "sdk_inc_ss928v100_sdk010/ot_debug.h"
#include "sdk_inc_ss928v100_sdk010/ot_defines.h"
#include "sdk_inc_ss928v100_sdk010/ot_errno.h"
#include "sdk_inc_ss928v100_sdk010/gfbg.h"
#include "sdk_inc_ss928v100_sdk010/ot_ir.h"
#include "sdk_inc_ss928v100_sdk010/ot_isp_bin.h"
#include "sdk_inc_ss928v100_sdk010/ot_isp_debug.h"
#include "sdk_inc_ss928v100_sdk010/ot_isp_define.h"
#include "sdk_inc_ss928v100_sdk010/ot_ivs_md.h"
#include "sdk_inc_ss928v100_sdk010/ot_math.h"
#include "sdk_inc_ss928v100_sdk010/ot_mcc_usrdev.h"
#include "sdk_inc_ss928v100_sdk010/ot_mipi_rx.h"
#include "sdk_inc_ss928v100_sdk010/ot_mipi_tx.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_ae.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_audio.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_avs.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_avs_lut_generate.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_avs_pos_query.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_awb.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_cipher.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_dpu_match.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_dpu_rect.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_dsp.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_fisheye_calibration.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_gdc.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_hdmi.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_isp.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_ive.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_klad.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_mau.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_mcf_calibration.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_mcf.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_mcf_vi.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_motionfusion.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_otp.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_pciv.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_region.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_sys.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_tde.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_vb.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_vdec.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_venc.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_vgs.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_vi.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_vo.h"
#include "sdk_inc_ss928v100_sdk010/ss_mpi_vpss.h"
#include "sdk_inc_ss928v100_sdk010/ot_osal.h"
#include "sdk_inc_ss928v100_sdk010/ot_osal_user.h"
#include "sdk_inc_ss928v100_sdk010/ot_resample.h"
#include "sdk_inc_ss928v100_sdk010/ot_sns_ctrl.h"
#include "sdk_inc_ss928v100_sdk010/ot_spi.h"
#include "sdk_inc_ss928v100_sdk010/ot_ssp.h"
#include "sdk_inc_ss928v100_sdk010/ot_type.h"
#include "sdk_inc_ss928v100_sdk010/ot_vdec_export.h"
#include "sdk_inc_ss928v100_sdk010/ot_vo_export.h"
#include "sdk_inc_ss928v100_sdk010/ot_vqe_register.h"
#include "sdk_inc_ss928v100_sdk010/ot_wtdg.h"
#include "sdk_inc_ss928v100_sdk010/list.h"
#include "sdk_inc_ss928v100_sdk010/osal_ioctl.h"
#include "sdk_inc_ss928v100_sdk010/osal_list.h"
#include "sdk_inc_ss928v100_sdk010/osal_mmz.h"
#include "sdk_inc_ss928v100_sdk010/pwm.h"
#include "sdk_inc_ss928v100_sdk010/securec.h"
#include "sdk_inc_ss928v100_sdk010/securectype.h"
#include "sdk_inc_ss928v100_sdk010/ot_wtdg.h"
#include "sdk_inc_ss928v100_sdk010/i2c-dev.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#ifdef __ARM_32BIT_STATE
#define OT_ADDR_P2U64(addr_point)   ((td_u64)(td_phys_addr_t)(addr_point))
#define OT_ADDR_U642P(addr_u64)     ((void *)(td_phys_addr_t)(addr_u64))
#else // __ARM_64BIT_STATE
#define OT_ADDR_P2U64(addr_point)   ((td_u64)(addr_point))
#define OT_ADDR_U642P(addr_u64)     ((void *)(addr_u64))
#endif

#ifdef __ARM_32BIT_STATE
#define HI_ADDR_P2U64(addr_point)   ((td_u64)(td_phys_addr_t)(addr_point))
#define HI_ADDR_U642P(addr_u64)     ((void *)(td_phys_addr_t)(addr_u64))
#else // __ARM_64BIT_STATE
#define HI_ADDR_P2U64(addr_point)   ((td_u64)(addr_point))
#define HI_ADDR_U642P(addr_u64)     ((void *)(addr_u64))
#endif

#endif