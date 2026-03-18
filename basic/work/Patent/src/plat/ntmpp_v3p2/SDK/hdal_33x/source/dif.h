/**
 * @file dif.h
 * @brief type definition of HDAL.
 * @author PSW
 * @date in the year 2018
 */

#ifndef _DIF_H_     /* prevent multiple inclusion of the header file */
#define _DIF_H_

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "audiocap.h"
#include "audiodec.h"
#include "audioenc.h"
#include "audioout.h"
#include "videocap.h"
#include "videodec.h"
#include "videoenc.h"
#include "videoout.h"
#include "videoprocess.h"
#include "gfx.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define DIF_BIT(nr)                       (1UL << (nr))
#define DIF_DEC_NOT_DISPLAY_BIT_0         DIF_BIT(0)
#define DIF_BLACK_BS_BIT_1                DIF_BIT(1)
#define DIF_JPEG_SHIFT_BIT_2              DIF_BIT(2)

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _DIF_DEV {
	BOOL is_bound; 			///< bound state
	HD_DAL device_baseid;	///< device baseid
	HD_DAL device_subid;	///< device subid
	UINT32 in_subid; 		///< device in_subid
	UINT32 out_subid;		///< device out_subid
	HD_BIND_INFO *p_bind;	///< self p_bind
	UINT member_idx;		///< index of this line
} DIF_DEV;

typedef struct _DIF_VCAP_PARAM {
	DIF_DEV dev;
	CAP_INTERNAL_PARAM *param;
} DIF_VCAP_PARAM;

typedef struct _DIF_VPROC_PARAM {
	DIF_DEV dev;
	VDOPROC_INTL_PARAM *param;
} DIF_VPROC_PARAM;

typedef struct _DIF_GFX_PARAM {
	DIF_DEV dev;
	BOOL with_gfx;
	GFX_INTERNAL_PARAM *param;
} DIF_GFX_PARAM;

typedef struct _DIF_VOUT_PARAM {
	DIF_DEV dev;
	VDDO_INTERNAL_PARAM *param;
} DIF_VOUT_PARAM;

typedef struct _DIF_VENC_PARAM {
	DIF_DEV dev;
	VDOENC_INTERNAL_PARAM *param;
} DIF_VENC_PARAM;

typedef struct _DIF_VDEC_PARAM {
	DIF_DEV dev;
	VDODEC_INTL_PARAM *param;
} DIF_VDEC_PARAM;

typedef struct _DIF_ACAP_PARAM {
	DIF_DEV dev;
	AUDCAP_INTERNAL_PARAM *param;
} DIF_ACAP_PARAM;

typedef struct _DIF_AENC_PARAM {
	DIF_DEV dev;
	AUDENC_INTERNAL_PARAM *param;
} DIF_AENC_PARAM;

typedef struct _DIF_ADEC_PARAM {
	DIF_DEV dev;
	AUDDEC_INTERNAL_PARAM *param;
} DIF_ADEC_PARAM;

typedef struct _DIF_AOUT_PARAM {
	DIF_DEV dev;
	AUDOUT_INTERNAL_PARAM *param;
} DIF_AOUT_PARAM;

typedef struct _DIF_VIDEOENC_QP_RATIO {
	UINT8 qp;
	UINT8 ratio;
} DIF_VIDEOENC_QP_RATIO;


/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them
/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
VOID dif_setting_log(unsigned char *msg_cmd);
HD_RESULT dif_bind_set(HD_GROUP *p_group, UINT32 line_idx);
HD_RESULT dif_process_audio_livesound(HD_GROUP *p_group, UINT32 line_idx);
void *dif_get_vpd_entity(HD_GROUP *p_group, UINT32 line_idx, INT entity_idx);
HD_RESULT dif_get_align_up_dim(HD_DIM in_dim, HD_VIDEO_PXLFMT in_pxlfmt, HD_DIM *p_out_dim);
HD_RESULT dif_get_align_down_dim(HD_DIM in_dim, HD_VIDEO_PXLFMT in_pxlfmt, HD_DIM *p_out_dim);

BOOL dif_is_lv(HD_GROUP *p_group, UINT32 line_idx);
BOOL dif_is_lv_changed(HD_GROUP *p_group, UINT32 line_idx);

HD_RESULT dif_audio_poll(HD_AUDIOENC_POLL_LIST *p_poll, UINT32 nr, INT32 wait_ms);
HD_RESULT dif_audio_recv_bs(HD_AUDIOENC_RECV_LIST *p_audioenc_bs, UINT32 nr);
HD_RESULT dif_audio_send_bs(HD_AUDIODEC_SEND_LIST *p_audiodec_bs, UINT32 nr, INT32 wait_ms);

HD_RESULT dif_video_poll(HD_VIDEOENC_POLL_LIST *p_poll, UINT32 nr, INT32 wait_ms);
HD_RESULT check_video_send_bs(HD_VIDEODEC_SEND_LIST *p_videodec_bs, UINT32 nr);
HD_RESULT dif_video_recv_bs(HD_VIDEOENC_RECV_LIST *p_videoenc_bs, UINT32 nr);
HD_RESULT dif_video_send_bs(HD_VIDEODEC_SEND_LIST *p_videodec_bs, UINT32 nr, INT32 wait_ms);
HD_RESULT dif_video_send_yuv(HD_VIDEOPROC_SEND_LIST *p_videoproc_bs, UINT32 nr, INT32 wait_ms);
HD_RESULT dif_apply_attr(HD_DAL dev_id, HD_IO out_id, INT param_id);
HD_RESULT dif_pull_out_buffer(HD_DAL dev_id, HD_IO io_id, VOID *p_out_buffer, INT32 wait_ms);
HD_RESULT dif_release_out_buffer(HD_DAL dev_id, HD_IO io_id, VOID *p_out_buffer);

HD_RESULT dif_get_vcap_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *vcap_vch, INT *vcap_path);
HD_RESULT dif_get_vpe_cascade_level(HD_GROUP *p_group, UINT32 line_idx, INT *cascade_level);
HD_RESULT dif_get_vpe_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *vpe_vch, INT *vpe_path);
HD_RESULT dif_get_venc_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *venc_vch, INT *venc_path);
HD_RESULT dif_get_lcd_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *lcd_vch, INT *lcd_path);
HD_RESULT dif_get_lcd_output_type(HD_GROUP *p_group, UINT32 line_idx, INT *is_virtual);
HD_RESULT dif_get_aout_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *aout_vch, INT *aout_path);
HD_RESULT dif_get_acap_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *acap_vch, INT *acap_path);
HD_RESULT dif_get_next_member_id(HD_GROUP *p_group, UINT32 line_idx, INT this_member_idx,
				 INT *p_next_devid, UINT32 *p_next_in_sub_id, UINT32 *p_next_out_sub_id);

HD_RESULT dif_get_vcap_out(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEOCAP_OUT *p_cap_out);
HD_RESULT dif_get_real_vcap_fps(HD_VIDEOCAP_OUT *p_cap_out, INT cap_vch, INT *p_vcap_fps);
HD_RESULT dif_get_cap_vpe_link_requirement(VDOPROC_INTL_PARAM *vproc_param, HD_VIDEOCAP_OUT *p_cap_out,
		BOOL *p_need_link_di, BOOL *p_need_link_vpe);
HD_RESULT dif_get_vpe_link_requirement(VDOPROC_INTL_PARAM *vproc_param, BOOL *p_need_link_vpe);
HD_RESULT dif_get_vdec_max_mem(HD_GROUP *p_group, UINT32 line_idx, VDODEC_MAXMEM *p_max_mem);
HD_RESULT dif_get_venc_codec_type(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEO_CODEC *p_codec_type);
HD_RESULT dif_get_venc_dim_fmt(HD_GROUP *p_group, UINT32 line_idx, HD_DIM *p_enc_dim, HD_VIDEO_PXLFMT *p_enc_fmt);
HD_RESULT dif_get_venc_rc_stuff(HD_GROUP *p_group, UINT32 line_idx, UINT32 *p_frame_rate_base, UINT32 *p_frame_rate_incr);
HD_RESULT dif_get_venc_rc(HD_GROUP *p_group, UINT32 line_idx, HD_H26XENC_RATE_CONTROL *p_enc_rc);
HD_RESULT dif_get_vout_win_attr(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEOOUT_WIN_ATTR *p_win);
HD_RESULT dif_get_vout_win_psr_attr(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEOOUT_WIN_PSR_ATTR *p_win_psr);
HD_RESULT dif_get_vproc_rotation(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEO_DIR *p_direction);
HD_RESULT dif_get_vproc_out_pxlfmt(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEO_PXLFMT *p_dst_fmt);
HD_RESULT dif_get_vproc_out_pool(HD_GROUP *p_group, UINT32 line_idx, HD_OUT_POOL *p_out_pool);
HD_RESULT dif_get_vproc_out_and_di(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEO_PXLFMT *p_dst_fmt,
				   HD_URECT *p_dst_rect, HD_DIM *p_dst_bg_dim, BOOL *p_di_enable);
HD_RESULT dif_get_pxlfmt_by_codec_type(HD_VIDEO_CODEC codec_type, HD_VIDEO_PXLFMT *p_fmt, UINT32 yuv_idx);
INT dif_is_di_enable(HD_GROUP *p_group, UINT32 line_idx, VDOPROC_INTL_PARAM **pp_vproc_intl_param);
INT dif_get_vcap_fps_for_rec_multi_path(HD_DAL vcap_deviceid, UINT32 vcap_out_subid);

HD_RESULT dif_get_aenc_codec_type(HD_GROUP *p_group, INT line_idx, HD_AUDIO_CODEC *p_codec_type);
HD_RESULT dif_get_adec_codec_type(HD_GROUP *p_group, INT line_idx, HD_AUDIO_CODEC *p_codec_type);
HD_RESULT dif_get_audio_record_bind_num(HD_GROUP *p_group, HD_BIND_INFO *p_bind, INT *bind_num);
HD_RESULT dif_get_adec_in_param(HD_GROUP *p_group, INT line_idx, HD_AUDIODEC_IN *p_dec_in);
HD_RESULT dif_get_aout_vch_setting(INT aout_vch, INT aout_path, INT *p_vch);

HD_FRAME_TYPE get_hdal_frame_type(unsigned int ref_frame);
INT get_gm_win_layer_from_hdal_layer(unsigned int value);
HD_PATH_ID dif_get_path_id(DIF_DEV *dev);
HD_PATH_ID dif_get_path_id_by_binding(HD_BIND_INFO *p_bind, UINT32 in_subid, UINT32 out_subid);
VOID dif_get_dev_string(HD_PATH_ID path_id, CHAR *string, UINT len);
CHAR *dif_return_dev_string(HD_PATH_ID path_id);
INT dif_send_black_bs_to_vdec(HD_PATH_ID dec_pathid, HD_DIM user_dec_max_dim, UINT32 codec_type);
INT dif_send_black_frame_to_vproc(HD_PATH_ID pathid, HD_DIM max_dim, HD_VIDEO_PXLFMT pxlfmt);
HD_RESULT dif_check_flow_mode(HD_DAL dev_id, HD_IO out_id);

#endif //#ifndef _DIF_H_

