/**
	@brief Header file of video deocde internal of library.\n
	This file contains the video deocde internal functions of library.

	@file videodec.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VIDEOCAP_H_
#define _VIDEOCAP_H_

#ifdef  __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "bind.h"
#include "common_def.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define MAX_CAP_PORT            8  ///< Indicate max input port value
#define MAX_CAP_DEV             HD_DAL_VIDEOCAP_COUNT   ///< Indicate max enc dev
#define VDOCAP_DEVID(self_id)   (self_id - HD_DAL_VIDEOCAP_BASE)
#define VDOCAP_INID(id)         (id - HD_IN_BASE)
#define VDOCAP_OUTID(id)        (id - HD_OUT_BASE)
#define VDOCAP_MASKID(id)       (id - HD_MASK_BASE)

#define VIDEOCAP_CHECK_SELF_ID(self_id)                                         \
	do {                                                                        \
		if (VDOCAP_DEVID(self_id) > MAX_CAP_DEV) { \
			HD_VIDEOCAPTURE_ERR("Error self_id(%d)\n", (self_id));                  \
			return HD_ERR_NG;                                                   \
		}                                                                       \
	} while (0)

#define VIDEOCAP_CHECK_OUT_ID(io_id)                                                    \
	do {                                                                               \
		if ((io_id) >= HD_OUT_BASE && (io_id) < (HD_OUT_BASE + MAX_CAP_PORT)) { \
		} else {                                                                       \
			HD_VIDEOCAPTURE_ERR("Error io_id(%d), only support ouput port\n", (io_id));                             \
			return HD_ERR_NG;                                                          \
		}                                                                              \
	} while (0)

#define VIDEOCAP_CHECK_IN_ID(io_id)                                                    \
	do {                                                                               \
		if ((io_id) >= HD_IN_BASE && (io_id) < (HD_IN_BASE + MAX_CAP_PORT)) { \
		} else {                                                                       \
			HD_VIDEOCAPTURE_ERR("Error io_id(%d), only support input port\n", (io_id));                             \
			return HD_ERR_NG;                                                          \
		}                                                                              \
	} while (0)
#define VIDEOCAP_CHECK_CTRL_ID(io_id)                                                    \
	do {                                                                               \
		if (io_id == HD_CTRL) { \
		} else {                                                                       \
			HD_VIDEOCAPTURE_ERR("Error io_id(%d), only support ctrl port\n", (io_id));                             \
			return HD_ERR_NG;                                                          \
		}                                                                              \
	} while (0)

#define VCAP_MAX_INFPS 		1000
#define VCAP_DEFAULT_FPS 	30
#define VCAP_PUSH_PULL_GIVE_BUF 0x47
#define VCAP_CTRL_FAKE_PATHID  0x200001FF
#define VCAP_PUSHPULL_SIZE_MAGIC  0xF1F1F1F1

/*-----------------------------------------------------------------------------*/
/* External Constant Definitions                                               */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/

typedef struct _HD_MD_PARAM {
	HD_VIDEOCAP_MD_STATUS md_stat;
} HD_MD_PARAM;


typedef struct _VDOCAP_PRIV {
	INT32 fd;
	//UINT32 queue_handle;
	uintptr_t queue_handle;
	CHAR push_in_used;
	HD_PATH_ID_STATE push_state;
	HD_PATH_ID_STATE pull_state;
	INT  path_init_count;
} VDOCAP_PRIV;

typedef struct _VDOCAP_TWO_CHANNEL_MODE_PARAM {
	UINT8 enable;
	INT32 sec_side_fd;
	HD_PATH_ID sec_path_id;
	BOOL is_double; //width is double or not
	INT state; //0: do nothing, 1: turn on, 2: turn off
} VDOCAP_TWO_CHANNEL_MODE_PARAM;

typedef struct _HD_CAP_INFO {
	HD_VIDEOCAP_CROP cap_crop[MAX_CAP_PORT];
	HD_VIDEOCAP_OUT cap_out[MAX_CAP_PORT];
	HD_MD_PARAM cap_md;
	HD_OUT_POOL out_pool[MAX_CAP_PORT];	///< for pool info setting
	//private data
	VDOCAP_PRIV priv[MAX_CAP_PORT];
	HD_OSG_MASK_ATTR mask[MAX_CAP_PORT][HD_VIDEOCAP_MAX_MASK];
	VDOCAP_TWO_CHANNEL_MODE_PARAM cap_two_ch_param[MAX_CAP_PORT];
} HD_CAP_INFO;

typedef struct _CAP_INTERNAL_PARAM {
	HD_VIDEOCAP_CROP *cap_crop[MAX_CAP_PORT];
	HD_VIDEOCAP_OUT *cap_out[MAX_CAP_PORT];
	HD_MD_PARAM *cap_md;
	UINT linefd[MAX_CAP_PORT];
	HD_OUT_POOL *p_out_pool[MAX_CAP_PORT];
	VDOCAP_TWO_CHANNEL_MODE_PARAM *cap_two_ch_param[MAX_CAP_PORT];
} CAP_INTERNAL_PARAM;

typedef struct _CAP_HW_LIMIT_INFO {
	UINT32 w_align;             ///< width  alignment value
	UINT32 h_align;             ///< height alignment value
	UINT32 w_min;               ///< width  minimum   value
	UINT32 h_min;               ///< height minimum   value
	UINT32 w_max;               ///< width  maximun   value
	UINT32 h_max;               ///< height maximun   value
} CAP_HW_LIMIT_INFO;
/*-----------------------------------------------------------------------------*/
/* Internal Constant Definitions                                               */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Internal Types Declarations                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* External Function Prototype                                                 */
/*-----------------------------------------------------------------------------*/
HD_RESULT videocap_push_in_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_out_video_frame, INT32 wait_ms);
HD_RESULT videocap_pull_out_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms);
HD_RESULT videocap_release_out_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame);
HD_RESULT videocap_push_in_buf_pair(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_out_video_frame, INT32 wait_ms);
HD_RESULT videocap_pull_out_buf_pair(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms);

void videocap_ioctl_init(void);
void videocap_ioctl_uninit(void);

HD_RESULT videocap_vi_register(HD_VIDEOCAP_VI *p_vcap_vi);
HD_RESULT videocap_vi_deregister(HD_VIDEOCAP_VI_ID *p_vcap_vi_id);
HD_RESULT videocap_host_init(HD_VIDEOCAP_HOST *p_vcap_host);
HD_RESULT videocap_host_uninit(HD_VIDEOCAP_HOST_ID *p_vcap_host_id);
HD_RESULT videocap_vi_vport_get_param(HD_VIDEOCAP_VI_VPORT *p_vi_vport);
HD_RESULT videocap_vi_vport_set_param(HD_VIDEOCAP_VI_VPORT *p_vi_vport);
HD_RESULT videocap_vi_ch_get_param(HD_VIDEOCAP_VI_CH_PARAM *p_ch_param);
HD_RESULT videocap_vi_ch_set_param(HD_VIDEOCAP_VI_CH_PARAM *p_ch_param);
HD_RESULT videocap_vi_ch_get_norm(HD_VIDEOCAP_VI_CH_NORM *p_ch_norm);
HD_RESULT videocap_vi_ch_set_norm(HD_VIDEOCAP_VI_CH_NORM *p_ch_norm);
HD_RESULT videocap_get_vi(HD_VIDEOCAP_VI *p_vcap_vi);

HD_RESULT videocap_set_crop(HD_DAL dev_id, HD_IO out_id, HD_VIDEOCAP_CROP *param);
HD_RESULT videocap_set_out(HD_DAL dev_id, HD_IO out_id, HD_VIDEOCAP_OUT *param);
HD_RESULT videocap_get_devcount(HD_DEVCOUNT *p_devcount);
HD_RESULT videocap_get_syscaps(HD_DAL dev_id, HD_VIDEOCAP_SYSCAPS *p_cap_syscaps);
HD_RESULT videocap_get_sysinfo(HD_DAL dev_id, HD_VIDEOCAP_SYSINFO *p_sysinfo);
HD_RESULT videocap_get_param_out(HD_DAL dev_id, HD_IO out_id, HD_VIDEOCAP_OUT *p_cap_out);
HD_RESULT videocap_get_param_out_crop(HD_DAL dev_id, HD_IO out_id, HD_VIDEOCAP_CROP *p_cap_out_crop);
HD_RESULT videocap_set_mask(HD_PATH_ID path_id, HD_OSG_MASK_ATTR *param);
HD_RESULT videocap_get_mask(HD_PATH_ID path_id, HD_OSG_MASK_ATTR *param);
HD_RESULT videocap_set_mask_palette(HD_PATH_ID path_id, HD_OSG_PALETTE_TBL *palette);
HD_RESULT videocap_get_mask_palette(HD_PATH_ID path_id, HD_OSG_PALETTE_TBL *palette);

HD_RESULT videocap_set_md_stat(HD_DAL dev_id, HD_VIDEOCAP_MD_STATUS *param);

HD_RESULT videocap_uninit_path_p(HD_PATH_ID path_id);
HD_RESULT videocap_init(void);
HD_RESULT videocap_uninit(void);
HD_RESULT videocap_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID *p_path_id);
HD_RESULT videocap_close(HD_PATH_ID path_id);
HD_RESULT videocap_start_mask(HD_PATH_ID path_id);
HD_RESULT videocap_stop_mask(HD_PATH_ID path_id);
HD_RESULT videocap_close_mask(HD_PATH_ID path_id);
HD_RESULT videocap_close_fakepath(HD_PATH_ID path_id);

CAP_INTERNAL_PARAM *videocap_get_param_p(HD_DAL self_id);
HD_RESULT videocap_check_out_param(HD_VIDEOCAP_OUT *vcap_out_param);
HD_RESULT videocap_check_crop_param(HD_VIDEOCAP_CROP *vcap_crop_param);
HD_RESULT validate_cap_dim_wh_align(HD_DIM in_dim, HD_VIDEO_PXLFMT in_pxlfmt, HD_DIM *p_out_dim);
HD_RESULT validate_cap_dim_bg_align_dir(HD_DIM in_dim, HD_VIDEO_PXLFMT in_pxlfmt, HD_DIM *p_out_dim, unsigned int is_up);
HD_RESULT videocap_check_vcap_validdir(unsigned int dir, INT *valid, vpd_flip_t *flip_type);
HD_RESULT videocap_check_vcap_validdrc(unsigned int drc, INT *valid);
HD_RESULT videocap_check_vcap_validfrc(unsigned int frc, INT *valid, INT *dst, INT *src);
HD_RESULT videocap_user_proc_init(HD_PATH_ID path_id);
HD_RESULT videocap_user_proc_uninit(HD_PATH_ID path_id);
HD_RESULT validate_cap_dim_wh_align_odd(HD_DIM in_dim, HD_VIDEO_PXLFMT in_pxlfmt, HD_DIM *p_out_dim, unsigned int is_up);
HD_RESULT videocap_vi_ch_set_norm3(HD_VIDEOCAP_VI_CH_NORM3 *p_ch_norm);

#ifdef  __cplusplus
}
#endif

#endif
