/**
	@brief This sample demonstrates the usage of video encode functions.

	@file videoenc_convert.c

	@author K L Chu

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "hdal.h"
#include "vpd_ioctl.h"
#include "bind.h"
#include "logger.h"
#include "cif_common.h"
#include "dif.h"
#include "pif.h"
#include "utl.h"
#include "videoenc_convert.h"
#include "vg_common.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define BS_WIN_COUNT            2

#define ENC_OUT_BASE_SIZE(w, h)  FORMAT_SIZE_YUV420_SCE((w), (h))

#define CONFIG_DEFAULT_COMPRESS	 30
#define CONFIG_MIN_BS_WIDTH      640
#define CONFIG_MIN_BS_HEIGHT     480
#define CONFIG_MIN_BS_SIZE       (ENC_OUT_BASE_SIZE(CONFIG_MIN_BS_WIDTH, CONFIG_MIN_BS_HEIGHT) * CONFIG_DEFAULT_COMPRESS / 100)
#define	MAX_POOL_NUM			 1
#define ENC_OUT_COUNT(count)     (((count) + 9) / 10)    //round up by 10

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern unsigned int gmlib_dbg_level;
extern unsigned int gmlib_dbg_mode;
extern vpd_sys_info_t platform_sys_Info;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
int dif_get_bs_ratio(HD_VIDEO_CODEC codec_type, int qp);

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/

HD_RESULT get_attr_vch_setting(HD_GROUP *p_group, UINT32 line_idx, INT *attr_vch)
{
	INT member_idx;
	HD_MEMBER *p_member;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (attr_vch == NULL) {
		GMLIB_ERROR_P("NULL attr_vch\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if ((bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE) ||
		    (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) ||
		    (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE)) {
			*attr_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			return HD_OK;
		}

	}

err_exit:
	return HD_ERR_NG;
}


static vpd_enc_type_t pc_get_enc_type(HD_VIDEO_CODEC codec_type)
{
	vpd_enc_type_t ret = VPD_ENC_TYPE_H265;

	switch (codec_type) {
	case HD_CODEC_TYPE_JPEG:
		ret = VPD_ENC_TYPE_JPEG;
		break;
	case HD_CODEC_TYPE_H264:
		ret = VPD_ENC_TYPE_H264;
		break;
	case HD_CODEC_TYPE_H265:
		ret = VPD_ENC_TYPE_H265;
		break;
	default:
		GMLIB_ERROR_P("enc_type error(%d). \n", codec_type);
		break;
	}
	return (ret);
}

static int get_videoenc_fps_and_rc_stuff(HD_H26XENC_RATE_CONTROL *p_enc_rc, UINT32 *p_bitrate,
		UINT8 *p_min_qp,  UINT8 *p_max_qp, UINT32 *p_frame_rate_base, UINT32 *p_frame_rate_incr)
{
	UINT32 frame_rate_base = 0, frame_rate_incr = 0, bitrate = 0;
	UINT8 min_qp = 1, max_qp = 51;
	if (p_enc_rc->rc_mode == HD_RC_MODE_CBR) {
		frame_rate_base = p_enc_rc->cbr.frame_rate_base;
		frame_rate_incr = p_enc_rc->cbr.frame_rate_incr;
		bitrate = p_enc_rc->cbr.bitrate;
		min_qp = p_enc_rc->cbr.min_i_qp;
		max_qp = p_enc_rc->cbr.max_i_qp;
	} else if (p_enc_rc->rc_mode == HD_RC_MODE_VBR) {
		frame_rate_base = p_enc_rc->vbr.frame_rate_base;
		frame_rate_incr = p_enc_rc->vbr.frame_rate_incr;
		bitrate = p_enc_rc->vbr.bitrate;
		min_qp = p_enc_rc->vbr.min_i_qp;
		max_qp = p_enc_rc->vbr.max_i_qp;
	} else if (p_enc_rc->rc_mode == HD_RC_MODE_FIX_QP) {
		frame_rate_base = p_enc_rc->fixqp.frame_rate_base;
		frame_rate_incr = p_enc_rc->fixqp.frame_rate_incr;
		min_qp = max_qp = p_enc_rc->fixqp.fix_i_qp;
	} else if (p_enc_rc->rc_mode == HD_RC_MODE_EVBR) {
		frame_rate_base = p_enc_rc->evbr.frame_rate_base;
		frame_rate_incr = p_enc_rc->evbr.frame_rate_incr;
		bitrate = p_enc_rc->evbr.bitrate;
		min_qp = p_enc_rc->evbr.min_i_qp;
		max_qp = p_enc_rc->evbr.max_i_qp;
	}
	if (p_frame_rate_base) {
		*p_frame_rate_base = frame_rate_base;
	}
	if (p_frame_rate_incr) {
		*p_frame_rate_incr = frame_rate_incr;
	}
	if (p_bitrate) {
		*p_bitrate = bitrate;
	}
	if (p_min_qp) {
		*p_min_qp = min_qp;
	}
	if (p_max_qp) {
		*p_max_qp = max_qp;
	}
	if (frame_rate_base == 0 || frame_rate_incr == 0) {
		return 0;
	} else {
		return (frame_rate_base / frame_rate_incr);   // return fps
	}
}

static int get_jpegenc_fps_and_rc_stuff(VDOENC_INTERNAL_PARAM *p_param, int port_chn, UINT32 *p_bitrate,
		UINT8 *p_min_qp,  UINT8 *p_max_qp, UINT32 *p_frame_rate_base, UINT32 *p_frame_rate_incr)
{
	UINT32 frame_rate_base = 0, frame_rate_incr = 0, bitrate = 0;
	UINT8 min_qp = 1, max_qp = 51;
	VIDEOENC_JPEG_RATE_CONTROL *p_jpeg_rc;

	p_jpeg_rc = p_param->port[port_chn].p_jpeg_rc;

	frame_rate_base = p_jpeg_rc->frame_rate_base;
	frame_rate_incr = p_jpeg_rc->frame_rate_incr;
	bitrate = p_jpeg_rc->bitrate;
	if (p_jpeg_rc->vbr_mode == VIDEOENC_JPEG_RC_MODE_FIXQP) {
		min_qp = p_param->port[port_chn].p_enc_out_param->jpeg.image_quality;
		max_qp = p_param->port[port_chn].p_enc_out_param->jpeg.image_quality;
	} else {
		min_qp = p_jpeg_rc->min_quality;
		max_qp = p_jpeg_rc->max_quality;
	}

	if (p_frame_rate_base) {
		*p_frame_rate_base = frame_rate_base;
	}
	if (p_frame_rate_incr) {
		*p_frame_rate_incr = frame_rate_incr;
	}
	if (p_bitrate) {
		*p_bitrate = bitrate;
	}
	if (p_min_qp) {
		*p_min_qp = min_qp;
	}
	if (p_max_qp) {
		*p_max_qp = max_qp;
	}
	if (frame_rate_base == 0 || frame_rate_incr == 0) {
		return 0;
	} else {
		return (frame_rate_base / frame_rate_incr);   // return fps
	}
}


static int calculate_win_size(HD_VIDEO_CODEC codec_type, HD_DIM enc_dim, HD_VIDEO_PXLFMT pxlfmt,
			      UINT8 qp, UINT32 bitrate, int fps, int *p_aver_size)
{
	int win_size = 0, aver_size = 0, min_compress_rate = 0;

	if (codec_type == HD_CODEC_TYPE_RAW) {
		win_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(enc_dim, pxlfmt);
		aver_size = 0;
	} else {
		min_compress_rate = dif_get_bs_ratio(codec_type, qp);
		win_size = (ENC_OUT_BASE_SIZE(enc_dim.w, enc_dim.h) * min_compress_rate) / 100;
		if (win_size < CONFIG_MIN_BS_SIZE)
			win_size = CONFIG_MIN_BS_SIZE;
		aver_size = (bitrate / 8) / fps;
	}
	if (p_aver_size) {
		*p_aver_size = aver_size;
	}
	return win_size;
}


static void set_h26xe_param_from_hdal(h26x_param_set  *p_hdal_params, VDOENC_INTERNAL_PARAM *p_user_param,
				      int port_chn, HD_VIDEO_PXLFMT pxl_fmt, HD_DIM enc_bg_dim)
{
	HD_VIDEO_DIR  dir = HD_VIDEO_DIR_ROTATE_0;
	p_hdal_params->ver = VIDEOENC_PARAM_H_VER;
	p_hdal_params->update_flag.update_mask = 0;

	if (p_user_param->port[port_chn].p_enc_in_param) {
		VG_ENC_BUF_PARAM  *p_enc_buf_param = &p_hdal_params->enc_buf_param;
		HD_VIDEOENC_IN    *p_usr_enc_in_param = p_user_param->port[port_chn].p_enc_in_param;
		p_hdal_params->update_flag.item.vg_enc_buf_param = 1;
		if (HD_VIDEO_PXLFMT_YUV420_NVX3 /*TYPE_YUV420_SCE*/ == pxl_fmt) {
			p_enc_buf_param->source_y_line_offset = ALIGN16_UP(ALIGN32_DOWN(enc_bg_dim.w) * 3 / 4);
			p_enc_buf_param->source_c_line_offset = ALIGN16_UP(ALIGN32_DOWN(enc_bg_dim.w) * 3 / 4);
			p_enc_buf_param->source_chroma_offset = p_enc_buf_param->source_y_line_offset * enc_bg_dim.h;
		} else { // if (HD_VIDEO_PXLFMT_YUV420 == pxl_fmt)
			p_enc_buf_param->source_y_line_offset = enc_bg_dim.w;
			p_enc_buf_param->source_c_line_offset = enc_bg_dim.w;
			p_enc_buf_param->source_chroma_offset = p_enc_buf_param->source_y_line_offset * enc_bg_dim.h;
		}
		GMLIB_L2_P("set h26x offset(%lu,%lu,%lu) fmt(%#x)\n", p_enc_buf_param->source_y_line_offset,
			   p_enc_buf_param->source_c_line_offset, p_enc_buf_param->source_chroma_offset, pxl_fmt);
		dir = p_usr_enc_in_param->dir;
	} else {
		GMLIB_PRINT_P("%s: HD_VIDEOENC_PARAM_IN param is required.\n", __func__);
		return;
	}

	if (p_user_param->port[port_chn].p_enc_out_param) {
		VG_H26XENC_FEATURE *p_vg_param = &p_hdal_params->enc_feature;
		HD_VIDEOENC_OUT *p_usr_enc_out_param = p_user_param->port[port_chn].p_enc_out_param;
		HD_H26X_CONFIG  *p_usr_h26xe_param = &p_usr_enc_out_param->h26x;
		p_hdal_params->codec_type = p_usr_enc_out_param->codec_type;
		p_hdal_params->update_flag.item.vg_h26xenc_feature = 1;
		p_vg_param->gop_num = p_usr_h26xe_param->gop_num;
		p_vg_param->chrm_qp_idx = p_usr_h26xe_param->chrm_qp_idx;
		p_vg_param->sec_chrm_qp_idx = p_usr_h26xe_param->sec_chrm_qp_idx;
		p_vg_param->ltr_interval = p_usr_h26xe_param->ltr_interval;
		p_vg_param->ltr_pre_ref = p_usr_h26xe_param->ltr_pre_ref;
		p_vg_param->gray_en = p_usr_h26xe_param->gray_en;
		switch (dir) {
		case HD_VIDEO_DIR_ROTATE_90:
			p_vg_param->rotate = 2;
			break;
		case HD_VIDEO_DIR_ROTATE_180:
			p_vg_param->rotate = 3;
			break;
		case HD_VIDEO_DIR_ROTATE_270:
			p_vg_param->rotate = 1;
			break;
		case HD_VIDEO_DIR_ROTATE_0:
		default:
			p_vg_param->rotate = 0;
			break;
		}
		p_vg_param->source_output = p_usr_h26xe_param->source_output;
		p_vg_param->profile = p_usr_h26xe_param->profile;
		p_vg_param->level_idc = p_usr_h26xe_param->level_idc;
		p_vg_param->svc_layer = p_usr_h26xe_param->svc_layer;
		p_vg_param->entropy_mode = p_usr_h26xe_param->entropy_mode;
	} else {
		GMLIB_PRINT_P("%s: HD_VIDEOENC_PARAM_OUT_ENC_PARAM param is required.\n", __func__);
		return;
	}

	if (p_user_param->port[port_chn].p_enc_vui) {
		VG_H26XENC_VUI *p_vg_param = &p_hdal_params->vui;
		HD_H26XENC_VUI *p_usr_vui_param = p_user_param->port[port_chn].p_enc_vui;

		p_hdal_params->update_flag.item.vg_h26xenc_vui = 1;
		p_vg_param->vui_en = p_usr_vui_param->vui_en;
		p_vg_param->sar_width = p_usr_vui_param->sar_width;
		p_vg_param->sar_height = p_usr_vui_param->sar_height;
		p_vg_param->matrix_coef = p_usr_vui_param->matrix_coef;
		p_vg_param->transfer_characteristics = p_usr_vui_param->transfer_characteristics;
		p_vg_param->colour_primaries = p_usr_vui_param->colour_primaries;
		p_vg_param->video_format = p_usr_vui_param->video_format;
		p_vg_param->color_range = p_usr_vui_param->color_range;
		p_vg_param->timing_present_flag = p_usr_vui_param->timing_present_flag;
	}

	if (p_user_param->port[port_chn].p_enc_deblock) {
		VG_H26XENC_DEBLOCK *p_vg_param = &p_hdal_params->deblock;
		HD_H26XENC_DEBLOCK *p_usr_deblock_param = p_user_param->port[port_chn].p_enc_deblock;

		p_hdal_params->update_flag.item.vg_h26xenc_deblock = 1;
		p_vg_param->dis_ilf_idc = p_usr_deblock_param->dis_ilf_idc;
		p_vg_param->db_alpha = p_usr_deblock_param->db_alpha;
		p_vg_param->db_beta = p_usr_deblock_param->db_beta;
	}

	if (p_user_param->port[port_chn].p_enc_rc) {
		VG_H26XENC_RATE_CONTROL *p_vg_param = &p_hdal_params->rate_control;
		HD_H26XENC_RATE_CONTROL *p_usr_rc_param = p_user_param->port[port_chn].p_enc_rc;

		p_hdal_params->update_flag.item.vg_h26xenc_rate_control = 1;
		p_vg_param->rc_mode = p_usr_rc_param->rc_mode;
		if (p_usr_rc_param->rc_mode == HD_RC_MODE_CBR) {
			HD_H26XENC_CBR  *p_usr_cbr = &p_usr_rc_param->cbr;
			VG_H26XENC_CBR  *p_vg_cbr = &p_vg_param->rc_param.cbr;
			p_vg_cbr->bitrate = p_usr_cbr->bitrate;
			p_vg_cbr->frame_rate_base = p_usr_cbr->frame_rate_base;
			p_vg_cbr->frame_rate_incr = p_usr_cbr->frame_rate_incr;
			p_vg_cbr->init_i_qp = p_usr_cbr->init_i_qp;
			p_vg_cbr->max_i_qp = p_usr_cbr->max_i_qp;
			p_vg_cbr->min_i_qp = p_usr_cbr->min_i_qp;
			p_vg_cbr->init_p_qp = p_usr_cbr->init_p_qp;
			p_vg_cbr->max_p_qp = p_usr_cbr->max_p_qp;
			p_vg_cbr->min_p_qp = p_usr_cbr->min_p_qp;
			p_vg_cbr->static_time = p_usr_cbr->static_time;
			p_vg_cbr->ip_weight = p_usr_cbr->ip_weight;
		} else if (p_usr_rc_param->rc_mode == HD_RC_MODE_VBR) {
			HD_H26XENC_VBR  *p_usr_vbr = &p_usr_rc_param->vbr;
			VG_H26XENC_VBR  *p_vg_vbr = &p_vg_param->rc_param.vbr;
			p_vg_vbr->bitrate = p_usr_vbr->bitrate;
			p_vg_vbr->frame_rate_base = p_usr_vbr->frame_rate_base;
			p_vg_vbr->frame_rate_incr = p_usr_vbr->frame_rate_incr;
			p_vg_vbr->init_i_qp = p_usr_vbr->init_i_qp;
			p_vg_vbr->max_i_qp = p_usr_vbr->max_i_qp;
			p_vg_vbr->min_i_qp = p_usr_vbr->min_i_qp;
			p_vg_vbr->init_p_qp = p_usr_vbr->init_p_qp;
			p_vg_vbr->max_p_qp = p_usr_vbr->max_p_qp;
			p_vg_vbr->min_p_qp = p_usr_vbr->min_p_qp;
			p_vg_vbr->static_time = p_usr_vbr->static_time;
			p_vg_vbr->ip_weight = p_usr_vbr->ip_weight;
			p_vg_vbr->change_pos = p_usr_vbr->change_pos;
		} else if (p_usr_rc_param->rc_mode == HD_RC_MODE_FIX_QP) {
			HD_H26XENC_FIXQP  *p_usr_fixqp = &p_usr_rc_param->fixqp;
			VG_H26XENC_FIXQP  *p_vg_fixqp = &p_vg_param->rc_param.fixqp;
			p_vg_fixqp->frame_rate_base = p_usr_fixqp->frame_rate_base;
			p_vg_fixqp->frame_rate_incr = p_usr_fixqp->frame_rate_incr;
			p_vg_fixqp->fix_i_qp = p_usr_fixqp->fix_i_qp;
			p_vg_fixqp->fix_p_qp = p_usr_fixqp->fix_p_qp;
		} else { /* if (p_usr_rc_param->rc_mode == HD_RC_MODE_EVBR) */
			HD_H26XENC_EVBR  *p_usr_evbr = &p_usr_rc_param->evbr;
			VG_H26XENC_EVBR  *p_vg_evbr = &p_vg_param->rc_param.evbr;
			p_vg_evbr->bitrate = p_usr_evbr->bitrate;
			p_vg_evbr->frame_rate_base = p_usr_evbr->frame_rate_base;
			p_vg_evbr->frame_rate_incr = p_usr_evbr->frame_rate_incr;
			p_vg_evbr->init_i_qp = p_usr_evbr->init_i_qp;
			p_vg_evbr->max_i_qp = p_usr_evbr->max_i_qp;
			p_vg_evbr->min_i_qp = p_usr_evbr->min_i_qp;
			p_vg_evbr->init_p_qp = p_usr_evbr->init_p_qp;
			p_vg_evbr->max_p_qp = p_usr_evbr->max_p_qp;
			p_vg_evbr->min_p_qp = p_usr_evbr->min_p_qp;
			p_vg_evbr->static_time = p_usr_evbr->static_time;
			p_vg_evbr->ip_weight = p_usr_evbr->ip_weight;
			p_vg_evbr->key_p_period = p_usr_evbr->key_p_period;
			p_vg_evbr->kp_weight = p_usr_evbr->kp_weight;
			p_vg_evbr->still_frame_cnd = p_usr_evbr->still_frame_cnd;
			p_vg_evbr->motion_ratio_thd = p_usr_evbr->motion_ratio_thd;
			p_vg_evbr->motion_aq_str = p_usr_evbr->motion_aq_str;
			p_vg_evbr->still_i_qp = p_usr_evbr->still_i_qp;
			p_vg_evbr->still_p_qp = p_usr_evbr->still_p_qp;
			p_vg_evbr->still_kp_qp = p_usr_evbr->still_kp_qp;
		}
	}

	if (p_user_param->port[port_chn].p_enc_usr_qp) {
		VG_H26XENC_USR_QP *p_vg_param = &p_hdal_params->usr_qp;
		HD_H26XENC_USR_QP *p_usr_qp_param = p_user_param->port[port_chn].p_enc_usr_qp;

		p_hdal_params->update_flag.item.vg_h26xenc_usr_qp = 1;
		p_vg_param->enable = p_usr_qp_param->enable;
		p_vg_param->qp_map_addr = p_usr_qp_param->qp_map_addr;
	}

	if (p_user_param->port[port_chn].p_enc_slice_split) {
		VG_H26XENC_SLICE_SPLIT *p_vg_param = &p_hdal_params->slice_split;
		HD_H26XENC_SLICE_SPLIT *p_usr_slice_split_param = p_user_param->port[port_chn].p_enc_slice_split;

		p_hdal_params->update_flag.item.vg_h26xenc_slice_split = 1;
		p_vg_param->enable = p_usr_slice_split_param->enable;
		p_vg_param->slice_row_num = p_usr_slice_split_param->slice_row_num;
	}

	if (p_user_param->port[port_chn].p_enc_gdr) {
		VG_H26XENC_GDR *p_vg_param = &p_hdal_params->gdr;
		HD_H26XENC_GDR *p_usr_gdr_param = p_user_param->port[port_chn].p_enc_gdr;

		p_hdal_params->update_flag.item.vg_h26xenc_gdr = 1;
		p_vg_param->enable = p_usr_gdr_param->enable;
		p_vg_param->period = p_usr_gdr_param->period;
		p_vg_param->number = p_usr_gdr_param->number;
	}

	if (p_user_param->port[port_chn].p_enc_roi) {
		INT i;
		VG_H26XENC_ROI *p_vg_param = &p_hdal_params->roi;
		HD_H26XENC_ROI *p_usr_roi_param = p_user_param->port[port_chn].p_enc_roi;

		p_hdal_params->update_flag.item.vg_h26xenc_roi = 1;
		for (i = 0; i < VG_H26XENC_ROI_WIN_COUNT; i++) {
			HD_H26XENC_ROI_WIN *p_hdal_st_roi = &p_usr_roi_param->st_roi[i];
			VG_H26XENC_ROI_WIN *p_vg_st_roi = &p_vg_param->st_roi[i];
			p_vg_st_roi->enable = p_hdal_st_roi->enable;
			p_vg_st_roi->coord_X = p_hdal_st_roi->rect.x;
			p_vg_st_roi->coord_Y = p_hdal_st_roi->rect.y;
			p_vg_st_roi->width = p_hdal_st_roi->rect.w;
			p_vg_st_roi->height = p_hdal_st_roi->rect.h;
			p_vg_st_roi->mode = p_hdal_st_roi->mode;
			p_vg_st_roi->qp = p_hdal_st_roi->qp;
		}
	}

	if (p_user_param->port[port_chn].p_enc_row_rc) {
		VG_H26XENC_ROW_RC *p_vg_param = &p_hdal_params->row_rc;
		HD_H26XENC_ROW_RC *p_usr_row_rc_param = p_user_param->port[port_chn].p_enc_row_rc;

		p_hdal_params->update_flag.item.vg_h26xenc_row_rc = 1;
		p_vg_param->enable = p_usr_row_rc_param->enable;
		p_vg_param->i_qp_range = p_usr_row_rc_param->i_qp_range;
		p_vg_param->i_qp_step = p_usr_row_rc_param->i_qp_step;
		p_vg_param->p_qp_range = p_usr_row_rc_param->p_qp_range;
		p_vg_param->p_qp_step = p_usr_row_rc_param->p_qp_step;
		p_vg_param->min_i_qp = p_usr_row_rc_param->min_i_qp;
		p_vg_param->max_i_qp = p_usr_row_rc_param->max_i_qp;
		p_vg_param->min_p_qp = p_usr_row_rc_param->min_p_qp;
		p_vg_param->max_p_qp = p_usr_row_rc_param->max_p_qp;
	}

	if (p_user_param->port[port_chn].p_enc_aq) {
		INT i;
		VG_H26XENC_AQ *p_vg_param = &p_hdal_params->aq;
		HD_H26XENC_AQ *p_usr_aq_param = p_user_param->port[port_chn].p_enc_aq;

		p_hdal_params->update_flag.item.vg_h26xenc_aq = 1;
		p_vg_param->enable = p_usr_aq_param->enable;
		p_vg_param->i_str = p_usr_aq_param->i_str;
		p_vg_param->p_str = p_usr_aq_param->p_str;
		p_vg_param->max_delta_qp = p_usr_aq_param->max_delta_qp;
		p_vg_param->min_delta_qp = p_usr_aq_param->min_delta_qp;
		p_vg_param->depth = p_usr_aq_param->depth;
		for (i = 0; i < VG_H26XENC_AQ_MAP_TABLE_COUNT; i++) {
			p_vg_param->thd_table[i] = p_usr_aq_param->thd_table[i];
		}
	}
}



/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

INT set_enc_osg_unit(HD_GROUP *hd_group, INT line_idx, DIF_VENC_PARAM *p_venc_param, INT *len,
		     INT curr_entity_idx)
{
	int sn, port_chn;
	unsigned int enc_osg_fd;
	pif_group_t *group;
	vpd_entity_t *vpd_entity = NULL, *prev_vpd_entity = NULL;
	vpd_osg_entity_t *osg_entity;
	INT attr_vch = 0;
	VDOENC_INTERNAL_PARAM *p_hdal_vdoenc_param;
	HD_VIDEO_CODEC  codec_type;
	vpd_cap_entity_t *cap_entity;
	vpd_vpe_1_entity_t *vpe_1_entity;
	vpd_di_entity_t *di_entity;

	if (len == NULL) {
		GMLIB_ERROR_P("NULL len\n");
		return 0;
	} else {
		*len = 0;
	}
	if (hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (p_venc_param == NULL) {
		GMLIB_ERROR_P("NULL p_venc_param\n");
		goto err_exit;
	}
	p_hdal_vdoenc_param = p_venc_param->param;
	port_chn = p_venc_param->dev.in_subid;

	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	if (p_hdal_vdoenc_param->port[port_chn].p_enc_in_param == NULL) {
		GMLIB_PRINT_P("%s: HD_VIDEOENC_PARAM_IN param is required.\n", __func__);
		goto err_exit;
	}
	if (p_hdal_vdoenc_param->port[port_chn].p_enc_out_param == NULL) {
		GMLIB_PRINT_P("%s: HD_VIDEOENC_PARAM_OUT_ENC_PARAM param is required.\n", __func__);
		goto exit;
	}

	codec_type = p_hdal_vdoenc_param->port[port_chn].p_enc_out_param->codec_type;
	if (codec_type == HD_CODEC_TYPE_RAW) {
		return 0;
	}

	sn = enc_osg_fd = utl_get_enc_osg_entity_fd(p_venc_param->dev.device_subid, 0, p_venc_param->dev.in_subid,  NULL);
	if (sn == 0) {
		GMLIB_ERROR_P("sn = 0, error\n");
		goto exit;
	}
	if (get_attr_vch_setting(hd_group, line_idx, &attr_vch) != HD_OK) {
		GMLIB_ERROR_P("get_attr_vch_setting fail\n");
		goto exit;
	}

	vpd_entity = pif_lookup_entity_buf(VPD_OSG_ENTITY_TYPE, sn, group->encap_buf);
	if (vpd_entity != NULL) { /* It have been set in encaps buf, just return this buffer */
		*len = 0;
		goto exit;
	}

	*len = pif_set_EntityBuf(VPD_OSG_ENTITY_TYPE, &vpd_entity, group->encap_buf);
	vpd_entity->sn = sn;
	vpd_entity->ap_bindfd = BD_SET_PATH(p_venc_param->dev.device_baseid + p_venc_param->dev.device_subid,
					    p_venc_param->dev.in_subid + HD_IN_BASE,
					    p_venc_param->dev.out_subid + HD_OUT_BASE);

	// NOTE func sequence: dif_get_vpd_entity need keep it after pif_set_EntityBuf, because of realloc in pif_set_EntityBuf!
	// find previous vpd_entity (it could be vcap, vpe316 or vpe536.)
	prev_vpd_entity = dif_get_vpd_entity(hd_group, line_idx, curr_entity_idx - 1);
	if (prev_vpd_entity == NULL) {
		goto exit;
	}

	//osg's out-buffer is the same as the previous one, so just copy it
	memcpy(&vpd_entity->pool, &prev_vpd_entity->pool, sizeof(vpd_entity->pool));

	/* for osg is duplicate pool */
	vpd_entity->pool[0].is_duplicate = 1;
	vpd_entity->pool[0].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
	osg_entity = (vpd_osg_entity_t *)vpd_entity->e;
	if (prev_vpd_entity->type == VPD_CAP_ENTITY_TYPE) {
		if (!prev_vpd_entity->e) {
			printf("Check VPD_CAP_ENTITY_TYPE entity fail\n");
			goto exit;
		}
		cap_entity = (vpd_cap_entity_t *)prev_vpd_entity->e;
		osg_entity->src_bg_dim.w = cap_entity->dst_bg_dim.w;
		osg_entity->src_bg_dim.h = cap_entity->dst_bg_dim.h;
	} else if (prev_vpd_entity->type == VPD_VPE_1_ENTITY_TYPE) {
		if (!prev_vpd_entity->e) {
			printf("Check VPD_VPE_1_ENTITY_TYPE entity fail\n");
			goto exit;
		}
		vpe_1_entity = (vpd_vpe_1_entity_t *)prev_vpd_entity->e;
		osg_entity->src_bg_dim.w = vpe_1_entity->pch_param.dst_bg_dim.w;
		osg_entity->src_bg_dim.h = vpe_1_entity->pch_param.dst_bg_dim.h;
	} else if (prev_vpd_entity->type == VPD_DI_ENTITY_TYPE) {
		if (!prev_vpd_entity->e) {
			printf("Check VPD_DI_ENTITY_TYPE entity fail\n");
			goto exit;
		}
		di_entity = (vpd_di_entity_t *)prev_vpd_entity->e;
		osg_entity->src_bg_dim.w = di_entity->dst_dim.w;
		osg_entity->src_bg_dim.h = di_entity->dst_dim.h;

	} else {
		printf("Not support osg has previous entity(%#x) are cap or vpe case\n", prev_vpd_entity->type);
		goto exit;
	}
	osg_entity->attr_vch = attr_vch;
	osg_entity->src_fmt = vpd_entity->pool[0].dst_fmt;
	osg_entity->osg_param_1.direction = VPD_ROTATE_NONE;
	SET_ENTITY_FEATURE(vpd_entity->feature, VPD_FLAG_JOB_SEQUENCE_LAST);

	GMLIB_L2_P("set_enc_osg_entity path_id(%#lx) fd(%#x) pch(%d) src_bg(%d,%d) src_fmt(%#x)\n",
		   dif_get_path_id(&p_venc_param->dev), enc_osg_fd, osg_entity->attr_vch,
		   osg_entity->src_bg_dim.w, osg_entity->src_bg_dim.h,
		   osg_entity->src_fmt);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, vpd_entity);

err_exit:
	return -1;
}

static BOOL check_do_ratio_by_enc(HD_GROUP *hd_group, INT line_idx, DIF_VENC_PARAM *p_venc_param, INT *p_vcap_fps)
{
	BOOL is_ratio_by_enc = FALSE;
	BOOL need_link_di = FALSE, need_link_vpe = FALSE;
	HD_VIDEOCAP_OUT *p_cap_out = NULL;
	INT cap_vch = 0, member_idx;
	HD_MEMBER *p_member;
	VDOPROC_INTL_PARAM *vproc_param = NULL;
	CAP_INTERNAL_PARAM *vcap_param = NULL;

	if (!p_venc_param || !hd_group || !p_vcap_fps) {
		GMLIB_ERROR_P("Null pointer. group(%p) param(%p) val(%p)\n", hd_group, p_venc_param, p_vcap_fps);
		goto exit;
	}
	if (p_venc_param->dev.member_idx <= 0) {
		GMLIB_ERROR_P("venc: enc's line_idx(%d)'s member_idx(%d) < 0.\n", line_idx, p_venc_param->dev.member_idx);
		goto exit;
	}

	/* !¡Óa¢FDX?U-!O!LaoO|eak!A¢FDNvenc¢FX¢Gg?n!LOca!|V
	 *  1.  vcap -> venc
	 *  2.  vcap -> vproc(¢FDu¢FX¢GgDI) -> venc
	 */

	//!¡Óa?e?@!AA!A?O!¡Ó_?¢FXvcapcIvproc
	member_idx = p_venc_param->dev.member_idx - 1;
	p_member = &(hd_group->glines[line_idx].member[member_idx]);

	if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE) {
		vcap_param = videocap_get_param_p(p_member->p_bind->device.deviceid);
		if (!vcap_param) {
			GMLIB_ERROR_P("venc: get vcap_param fail\n");
			goto exit;
		}
		p_cap_out = vcap_param->cap_out[p_member->out_subid];
		cap_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
		is_ratio_by_enc = TRUE;
	} else if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
		vproc_param = videoproc_get_param_p(p_member->p_bind->device.deviceid, p_member->out_subid);
	}

	//-Y?Ovproc!A!Lo|Ac1?e?Y?O!¡Ó_?¢FXvcap
	if (vproc_param && member_idx != 0) {
		member_idx --;
		p_member = &(hd_group->glines[line_idx].member[member_idx]);

		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE) {
			vcap_param = videocap_get_param_p(p_member->p_bind->device.deviceid);
			if (!vcap_param) {
				GMLIB_ERROR_P("venc: get vcap_param fail\n");
				goto exit;
			}
			p_cap_out = vcap_param->cap_out[p_member->out_subid];
			cap_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			//AE?d3o-Ovproc?O?¢FG?O¢FDu|3¢FX¢GgDI
			if (dif_get_cap_vpe_link_requirement(vproc_param, p_cap_out, &need_link_di, &need_link_vpe) != HD_OK) {
				GMLIB_ERROR_P("venc: get link requirement fail, p(%p,%p)\n", vproc_param, p_cap_out);
				goto exit;
			}
			if (!need_link_vpe) {
				is_ratio_by_enc = TRUE;
			}
		}
	}

	//|paG?O?W-!O!LaoO|eak!A|A¢FDh!Lu!Au1eaovcap fps
	if (is_ratio_by_enc && p_cap_out) {
		if (dif_get_real_vcap_fps(p_cap_out, cap_vch, p_vcap_fps) != HD_OK) {
			GMLIB_ERROR_P("venc: get vcap_fps err!\n");
			*p_vcap_fps = 30;
		}
	}

	GMLIB_L2_P("check enc do ratio(%d): vcap(%p,%d) vproc(%p,%d,%d), vcap_fps(%d)\n",
		   is_ratio_by_enc, vcap_param, cap_vch, vproc_param, need_link_di, need_link_vpe,
		   (p_vcap_fps == NULL) ? -1 : *p_vcap_fps);

exit:
	return is_ratio_by_enc;
}


INT set_venc_unit(HD_GROUP *hd_group, INT line_idx, DIF_VENC_PARAM *p_venc_param, INT *len)
{
	int sn, i, port_chn, fps, win_size, aver_size, aver_count, max_counts;
	unsigned int enc_fd;
	pif_group_t *group;
	vpd_entity_t *vpd_entity = NULL;
	vpd_h26x_enc_entity_t *h26xe_entity = NULL;
	vpd_mjpege_entity_t   *jpege_entity = NULL;
	VDOENC_INTERNAL_PARAM *p_hdal_vdoenc_param;
	HD_VIDEO_CODEC  codec_type;
	HD_DIM enc_dim, enc_bg_dim;
	HD_OUT_POOL *p_out_pool;
	HD_VIDEO_PXLFMT pxlfmt;
	UINT8 min_qp, max_qp;
	UINT32 bitrate;
	HD_VIDEO_DIR dir;
	HD_RESULT hd_ret;
	INT vcap_fps = 30;
	BOOL is_ratio_by_enc = FALSE;
	MEM_TYPE_t mem_type = MEM_UNKNOW_TYPE;

	if (len == NULL) {
		GMLIB_ERROR_P("NULL len\n");
		return 0;
	} else {
		*len = 0;
	}
	if (hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (p_venc_param == NULL) {
		GMLIB_ERROR_P("NULL p_venc_param\n");
		goto err_exit;
	}
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}


	//prepare local variables
	p_hdal_vdoenc_param = p_venc_param->param;
	port_chn = p_venc_param->dev.in_subid;  // The value of in_subid/out_subid are all the same as the channel number.
	if (p_hdal_vdoenc_param->port[port_chn].p_enc_out_param == NULL) {
		GMLIB_PRINT_P("%s: HD_VIDEOENC_PARAM_OUT_ENC_PARAM param is required.\n", __func__);
		goto exit;
	}
	codec_type = p_hdal_vdoenc_param->port[port_chn].p_enc_out_param->codec_type;
	if (codec_type == HD_CODEC_TYPE_RAW) {
		return 0;
	}
	if (p_hdal_vdoenc_param->port[port_chn].p_out_pool == NULL) {
		GMLIB_PRINT_P("%s: HD_VIDEOENC_PARAM_PATH_CONFIG param is required.\n", __func__);
		goto exit;
	}
	if (p_hdal_vdoenc_param->port[port_chn].p_enc_in_param == NULL) {
		GMLIB_PRINT_P("%s: HD_VIDEOENC_PARAM_IN param is required.\n", __func__);
		goto exit;
	}
	if (codec_type == HD_CODEC_TYPE_JPEG) {
		if (p_hdal_vdoenc_param->port[port_chn].p_jpeg_rc == NULL) {
			GMLIB_PRINT_P("%s: VENDOR_JPEGENC_PARAM_OUT_RATE_CONTROL param is required.\n", __func__);
			goto exit;
		}
	} else {
		if (p_hdal_vdoenc_param->port[port_chn].p_enc_rc == NULL) {
			GMLIB_PRINT_P("%s: HD_VIDEOENC_PARAM_OUT_RATE_CONTROL param is required.\n", __func__);
			goto exit;
		}
	}

	sn = enc_fd = utl_get_enc_entity_fd(p_venc_param->dev.device_subid, p_venc_param->dev.in_subid,
					    (VOID *)(UINTPTR)(codec_type == HD_CODEC_TYPE_JPEG ? 1 : 0));
	if (sn == 0) {
		GMLIB_ERROR_P("sn = 0, error\n");
		goto exit;
	}

	p_out_pool = p_hdal_vdoenc_param->port[port_chn].p_out_pool;
	pxlfmt = p_hdal_vdoenc_param->port[port_chn].p_enc_in_param->pxl_fmt;
	dir = p_hdal_vdoenc_param->port[port_chn].p_enc_in_param->dir;
	if (dir == HD_VIDEO_DIR_ROTATE_90 || dir == HD_VIDEO_DIR_ROTATE_270) {
		enc_dim.w = p_hdal_vdoenc_param->port[port_chn].p_enc_in_param->dim.h;
		enc_dim.h = p_hdal_vdoenc_param->port[port_chn].p_enc_in_param->dim.w;
	} else {
		enc_dim = p_hdal_vdoenc_param->port[port_chn].p_enc_in_param->dim;
	}
	hd_ret = dif_get_align_up_dim(enc_dim, pxlfmt, &enc_bg_dim);
	if (hd_ret != HD_OK) {
		GMLIB_PRINT_P("%s: dif_get_align_up_dim is failed. enc_dim(%lu,%lu) pxlfmt(%#x)\n",
			      __func__, enc_dim.w, enc_dim.h, pxlfmt);
		goto exit;
	}

	is_ratio_by_enc = check_do_ratio_by_enc(hd_group, line_idx, p_venc_param, &vcap_fps);
	if (codec_type == HD_CODEC_TYPE_JPEG) {
		fps = get_jpegenc_fps_and_rc_stuff(p_hdal_vdoenc_param, port_chn, &bitrate, &min_qp, &max_qp, NULL, NULL);
		win_size = calculate_win_size(codec_type, enc_dim, pxlfmt, max_qp, bitrate, fps, &aver_size);	//jpeg use max_qp to ge win_size
	} else {
		fps = get_videoenc_fps_and_rc_stuff(p_hdal_vdoenc_param->port[port_chn].p_enc_rc, &bitrate, &min_qp, NULL, NULL, NULL);
		win_size = calculate_win_size(codec_type, enc_dim, pxlfmt, min_qp, bitrate, fps, &aver_size);	//h26x use min_qp to ge win_size
	}
	if (is_ratio_by_enc && fps > vcap_fps) {
		GMLIB_WARNING_P("enc fps(%d) must be smaller than cap fps(%d).\n", fps, vcap_fps);
		fps = vcap_fps;
	}

	//set vpd_entity values
	if (codec_type == HD_CODEC_TYPE_JPEG) {
		vpd_entity = pif_lookup_entity_buf(VPD_MJPEGE_ENTITY_TYPE, sn, group->encap_buf);
		if (vpd_entity != NULL) { /* It have been set in encaps buf, just return this buffer */
			*len = 0;
			goto exit;
		}
		*len = pif_set_EntityBuf(VPD_MJPEGE_ENTITY_TYPE, &vpd_entity, group->encap_buf);
	} else {
		vpd_entity = pif_lookup_entity_buf(VPD_H26X_ENC_ENTITY_TYPE, sn, group->encap_buf);
		if (vpd_entity != NULL) { /* It have been set in encaps buf, just return this buffer */
			*len = 0;
			goto exit;
		}
		*len = pif_set_EntityBuf(VPD_H26X_ENC_ENTITY_TYPE, &vpd_entity, group->encap_buf);
	}

	vpd_entity->sn = sn;
	vpd_entity->ap_bindfd = BD_SET_PATH(p_venc_param->dev.device_baseid + p_venc_param->dev.device_subid,
					    p_venc_param->dev.in_subid + HD_IN_BASE,
					    p_venc_param->dev.out_subid + HD_OUT_BASE);

	memset(vpd_entity->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < VPD_MAX_BUFFER_NUM; i++) {
		vpd_entity->pool[i].type = VPD_ENC_OUT_POOL;
		mem_type = pif_get_pool_mem_type(vpd_entity->pool[i].type);
		if (p_out_pool->buf_info[i].enable == HD_BUF_DISABLE) {
			vpd_entity->pool[i].enable = FALSE;
			continue;
		} else if (p_out_pool->buf_info[i].enable == HD_BUF_ENABLE) {
			max_counts = ENC_OUT_COUNT(p_out_pool->buf_info[i].counts);
			aver_count = max_counts - BS_WIN_COUNT;

			if (aver_count <= 0) {
				max_counts = BS_WIN_COUNT;
				aver_count = 0;
			}

			vpd_entity->pool[i].enable = TRUE;
			vpd_entity->pool[i].ddr_id = p_out_pool->buf_info[i].ddr_id;
			if (mem_type == MEM_FIX_TYPE) {
				vpd_entity->pool[i].size = win_size;
			} else {
				vpd_entity->pool[i].size = (win_size * BS_WIN_COUNT) + (aver_size * aver_count);
			}
			vpd_entity->pool[i].win_size = win_size;
			vpd_entity->pool[i].count_decuple = p_out_pool->buf_info[i].counts;
			vpd_entity->pool[i].max_counts = max_counts;
			vpd_entity->pool[i].min_counts = 0;
		} else { //HD_BUF_AUTO
			if (i == 0) {
				aver_count = 1;
				max_counts = BS_WIN_COUNT + aver_count;

				vpd_entity->pool[i].enable = TRUE;
				vpd_entity->pool[i].ddr_id = 0;
				if (mem_type == MEM_FIX_TYPE) {
					vpd_entity->pool[i].size = win_size;
				} else {
					vpd_entity->pool[i].size = (win_size * BS_WIN_COUNT) + (aver_size * aver_count);
				}
				vpd_entity->pool[i].win_size = win_size;
				vpd_entity->pool[i].count_decuple = max_counts * 10;
				vpd_entity->pool[i].max_counts = max_counts;
				vpd_entity->pool[i].min_counts = 0;
			} else {
				vpd_entity->pool[i].enable = FALSE;
			}
		}

		snprintf(vpd_entity->pool[i].name, VPD_MAX_STRING_LEN - 1, "%s_%#x",
			 pif_get_pool_name_by_type(vpd_entity->pool[i].type), enc_fd);

		GMLIB_L2_P("videoenc path_id(%#lx) pool[%d]: name(%s) size(%d=%d*2+%d*%d) max_cnt(%u), en(%d) fps(%d) set_cnt(%lu) dim(%lu/%lu) br(%lu) min_qp(%u) ratio(%d) mem_type(%d)\n",
			   dif_get_path_id(&p_venc_param->dev), i, vpd_entity->pool[i].name,
			   vpd_entity->pool[i].size, win_size, aver_size, aver_count,
			   vpd_entity->pool[i].max_counts,
			   p_out_pool->buf_info[i].enable, fps, p_out_pool->buf_info[i].counts,
			   enc_dim.w, enc_dim.h, bitrate, min_qp, dif_get_bs_ratio(codec_type, min_qp), mem_type);

		//Need check: rotate case & alignment
		vpd_entity->pool[i].width = ALIGN16_UP(enc_dim.w);
		vpd_entity->pool[i].height = ALIGN16_UP(enc_dim.h);
		vpd_entity->pool[i].fps = fps;
		if (is_ratio_by_enc) {
			vpd_entity->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(fps, vcap_fps);
		} else {
			vpd_entity->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(30, 30);
		}
		vpd_entity->pool[i].vch = port_chn;
		vpd_entity->pool[i].is_duplicate = 0;
		vpd_entity->pool[i].dst_fmt = VPD_BUFTYPE_DATA;
	}
	vpd_entity->max_buffer_size = 0;

	if (codec_type == HD_CODEC_TYPE_JPEG) {
		VIDEOENC_JPEG_RATE_CONTROL *jpeg_rc = p_hdal_vdoenc_param->port[port_chn].p_jpeg_rc;
		jpege_entity = (vpd_mjpege_entity_t *)vpd_entity->e;
		jpege_entity->pch = port_chn;
		jpege_entity->src_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(pxlfmt);
		jpege_entity->src_bg_dim.w = enc_bg_dim.w;
		jpege_entity->src_bg_dim.h = enc_bg_dim.h;
		jpege_entity->src_rect.x = 0;
		jpege_entity->src_rect.y = 0;
		jpege_entity->src_rect.w = enc_dim.w;
		jpege_entity->src_rect.h = enc_dim.h;

		jpege_entity->rc.mode = jpeg_rc->vbr_mode;
		jpege_entity->rc.bitrate = jpeg_rc->bitrate;
		if (jpeg_rc->vbr_mode == VIDEOENC_JPEG_RC_MODE_FIXQP) {
			jpege_entity->rc.base_qp = p_hdal_vdoenc_param->port[port_chn].p_enc_out_param->jpeg.image_quality;
		} else {
			jpege_entity->rc.base_qp = jpeg_rc->base_qp;
		}
		jpege_entity->rc.min_quality = jpeg_rc->min_quality;
		jpege_entity->rc.max_quality = jpeg_rc->max_quality;
		jpege_entity->rc.frame_rate_base = jpeg_rc->frame_rate_base;
		jpege_entity->rc.frame_rate_incr = jpeg_rc->frame_rate_incr;

		switch (dir) {
		case HD_VIDEO_DIR_ROTATE_90:
			jpege_entity->rotate_mode = VPD_ROTATE_RIGHT;
			break;
		case HD_VIDEO_DIR_ROTATE_180:
			jpege_entity->rotate_mode = VPD_ROTATE_180;
			break;
		case HD_VIDEO_DIR_ROTATE_270:
			jpege_entity->rotate_mode = VPD_ROTATE_LEFT;
			break;
		case HD_VIDEO_DIR_ROTATE_0:
		default:
			jpege_entity->rotate_mode = VPD_ROTATE_NONE;
			break;
		}
		jpege_entity->restart_interval =
			p_hdal_vdoenc_param->port[port_chn].p_enc_out_param->jpeg.retstart_interval;
	} else {
		//set h26xe_entity values
		h26xe_entity = (vpd_h26x_enc_entity_t *)vpd_entity->e;
		h26xe_entity->pch = port_chn;
		h26xe_entity->enc_type = pc_get_enc_type(codec_type);

		h26xe_entity->src_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(pxlfmt);
		h26xe_entity->src_bg_dim.w = enc_bg_dim.w;
		h26xe_entity->src_bg_dim.h = enc_bg_dim.h;
		h26xe_entity->src_rect.x = 0;
		h26xe_entity->src_rect.y = 0;
		h26xe_entity->src_rect.w = enc_dim.w;
		h26xe_entity->src_rect.h = enc_dim.h;

		//TODO: how to calculate fps for multi-encoder
		h26xe_entity->src_frame_rate = fps;
		h26xe_entity->fps_ratio = PIF_SET_FPS_RATIO(30, 30);

		set_h26xe_param_from_hdal(&h26xe_entity->hdal_params, p_hdal_vdoenc_param, port_chn, pxlfmt, enc_bg_dim);
	}

	SET_ENTITY_FEATURE(vpd_entity->feature, VPD_FLAG_NON_BLOCK | VPD_FLAG_OUTBUF_MAX_CNT);

	GMLIB_L2_P("set_venc_entity path_id(%#lx) fd(%#x) pch(%d) src_bg(%d,%d) src_rect(%d,%d,%d,%d) src_fmt(%#x) fps(%d)\n",
		   dif_get_path_id(&p_venc_param->dev), enc_fd, h26xe_entity->pch,
		   h26xe_entity->src_bg_dim.w, h26xe_entity->src_bg_dim.h,
		   h26xe_entity->src_rect.x, h26xe_entity->src_rect.y,
		   h26xe_entity->src_rect.w, h26xe_entity->src_rect.h,
		   h26xe_entity->src_fmt, h26xe_entity->src_frame_rate);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, vpd_entity);

err_exit:
	return -1;
}


INT set_dataout_unit(HD_GROUP *hd_group, INT line_idx, DIF_VENC_PARAM *p_venc_param, INT *len)
{
	int sn, i, port_chn, fps;
	unsigned int dataout_fd;
	pif_group_t *group;
	vpd_entity_t *vpd_entity = NULL;
	HD_VIDEO_CODEC  codec_type;
	VDOENC_INTERNAL_PARAM *p_hdal_vdoenc_param;
	vpd_dout_entity_t *dout_entity;
	HD_VIDEO_PXLFMT pxlfmt;
	BOOL is_ratio_by_enc = FALSE;
	INT vcap_fps = 30;

	if (len == NULL) {
		GMLIB_ERROR_P("NULL len\n");
		return 0;
	} else {
		*len = 0;
	}
	if (hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (p_venc_param == NULL) {
		GMLIB_ERROR_P("NULL p_venc_param\n");
		goto err_exit;
	}
	if (p_venc_param->param == NULL) {
		GMLIB_ERROR_P("NULL p_venc_param->param\n");
		goto err_exit;
	}

	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	sn = dataout_fd = utl_get_dataout_entity_fd(p_venc_param->dev.device_subid,
			  p_venc_param->dev.in_subid, NULL);

	vpd_entity = pif_lookup_entity_buf(VPD_DOUT_ENTITY_TYPE, sn, group->encap_buf);
	if (vpd_entity != NULL) { /* It have been set in encaps buf, just return this buffer */
		*len = 0;
		goto exit;
	}
	*len = pif_set_EntityBuf(VPD_DOUT_ENTITY_TYPE, &vpd_entity, group->encap_buf);
	vpd_entity->sn = sn;
	vpd_entity->ap_bindfd = BD_SET_PATH(p_venc_param->dev.device_baseid + p_venc_param->dev.device_subid,
					    p_venc_param->dev.in_subid + HD_IN_BASE,
					    p_venc_param->dev.out_subid + HD_OUT_BASE);

	memset(vpd_entity->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < MAX_POOL_NUM; i++) {
		vpd_entity->pool[i].type = VPD_END_POOL;
		snprintf(vpd_entity->pool[i].name, VPD_MAX_STRING_LEN - 1, "%s",
			 pif_get_pool_name_by_type(vpd_entity->pool[i].type));

		vpd_entity->pool[i].width = 0;
		vpd_entity->pool[i].height = 0;
		vpd_entity->pool[i].fps = 30;
		vpd_entity->pool[i].vch = 0;
		vpd_entity->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		vpd_entity->pool[i].is_duplicate = 0;
		vpd_entity->pool[i].dst_fmt = VPD_BUFTYPE_UNKNOWN;
		vpd_entity->pool[i].ddr_id = 0;
		vpd_entity->pool[i].size = 0;
		vpd_entity->pool[i].win_size = 0;
		vpd_entity->pool[i].count_decuple = 0;
		vpd_entity->pool[i].max_counts = 0;
		vpd_entity->pool[i].min_counts = 0;
		vpd_entity->pool[i].enable = TRUE;
	}
	p_hdal_vdoenc_param = p_venc_param->param;
	port_chn = p_venc_param->dev.in_subid;  // The value of in_subid/out_subid are all the same as the channel number.
	if (p_hdal_vdoenc_param->port[port_chn].p_enc_out_param == NULL) {
		GMLIB_PRINT_P("%s: HD_VIDEOENC_PARAM_OUT_ENC_PARAM param is required.\n", __func__);
		goto exit;
	}
	dout_entity = (vpd_dout_entity_t *)vpd_entity->e;

	codec_type = p_hdal_vdoenc_param->port[port_chn].p_enc_out_param->codec_type;
	if (codec_type == HD_CODEC_TYPE_RAW) {

		dout_entity->is_raw_out = 1;
		pxlfmt = p_hdal_vdoenc_param->port[port_chn].p_enc_in_param->pxl_fmt;
		dout_entity->raw_out_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(
						    p_hdal_vdoenc_param->port[port_chn].p_enc_in_param->dim, pxlfmt);
	}
	if (p_hdal_vdoenc_param->port[port_chn].p_enc_rc) {
		is_ratio_by_enc = check_do_ratio_by_enc(hd_group, line_idx, p_venc_param, &vcap_fps);
		fps = get_videoenc_fps_and_rc_stuff(p_hdal_vdoenc_param->port[port_chn].p_enc_rc, NULL, NULL, NULL, NULL, NULL);
		if (is_ratio_by_enc && fps > vcap_fps) {
			GMLIB_WARNING_P("enc fps(%d) must be smaller than cap fps(%d)\n", fps, vcap_fps);
			fps = vcap_fps;
		}
		dout_entity->src_frame_rate = fps;
	} else {
		dout_entity->src_frame_rate = 30;
	}
	dout_entity->fps_ratio = PIF_SET_FPS_RATIO(1, 1);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, vpd_entity);

err_exit:
	return -1;

}


