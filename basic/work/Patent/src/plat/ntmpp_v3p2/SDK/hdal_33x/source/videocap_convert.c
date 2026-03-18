/**
	@brief This sample demonstrates the usage of video process functions.

	@file videocap_convert.c

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
#include "videocap_convert.h"
#include "vg_common.h"
#include "vcap316/vcap_ioctl.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define DEFAULT_MAX_COUNT       2
#define DEFAULT_MIN_COUNT       0
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern unsigned int gmlib_dbg_level;
extern unsigned int gmlib_dbg_mode;
extern vpd_sys_info_t platform_sys_Info;
extern struct vcap_ioc_hw_ability_t videocap_hw_ability;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
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
static unsigned int get_vcap_max_counts(unsigned int counts, unsigned int minus)
{
	unsigned int counts_carry;

	counts_carry = (counts / 10) + (counts % 10 != 0); // unconditional carry, reserved for TMNR
	if (counts_carry > 2) {
		counts_carry -= minus;
	}
	return counts_carry;
}

HD_RESULT get_vcap_flip_mode(HD_GROUP *hd_group, UINT32 line_idx, vpd_flip_t *p_flip_mode, INT *p_is_align)
{
	if (hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_flip_mode == NULL) {
		GMLIB_ERROR_P("NULL p_flip_mode\n");
		goto err_exit;
	}
	if (p_is_align == NULL) {
		GMLIB_ERROR_P("NULL p_is_align\n");
		goto err_exit;
	}
	/* vpe can mirrorX and mirrorY
		if (dif_get_vproc_rotation(hd_group, line_idx, &direction) == HD_OK) {
			switch (direction) {
			case HD_VIDEO_DIR_ROTATE_0:
				*p_flip_mode = VPD_FLIP_NONE;
				*p_is_align = 0;
				break;
			case HD_VIDEO_DIR_ROTATE_90:
				*p_flip_mode = VPD_FLIP_NONE;
				*p_is_align = 1;
				break;
			case HD_VIDEO_DIR_ROTATE_180:
				*p_flip_mode = VPD_FLIP_VERTICAL_AND_HORIZONTAL;
				*p_is_align = 0;
				break;
			case HD_VIDEO_DIR_ROTATE_270:
				*p_flip_mode = VPD_FLIP_NONE;
				*p_is_align = 1;
				break;
			default:
				*p_flip_mode = VPD_FLIP_NONE;
				*p_is_align = 0;
				break;
			}
		} else {
	*/
	*p_flip_mode = VPD_FLIP_NONE;
	*p_is_align = 0;

	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT get_vcap_crop_setting(HD_VIDEOCAP_CROP *p_cap_crop, INT cap_vch, vpd_cap_entity_t *cap_entity)
{
	if (p_cap_crop == NULL) {
		GMLIB_ERROR_P("NULL p_cap_crop\n");
		goto err_exit;
	}
	if (cap_vch >= MAX_CAP_DEV) {
		GMLIB_ERROR_P("Exceed max vcap device count(%d)\n", MAX_CAP_DEV);
		goto err_exit;
	}
	if (cap_entity == NULL) {
		GMLIB_ERROR_P("NULL cap_entity\n");
		goto err_exit;
	}

	if (p_cap_crop && (p_cap_crop->mode == HD_CROP_ON)) {
		if (p_cap_crop->win.coord.w == 0 || p_cap_crop->win.coord.h == 0) {
			// no virtual coordinate, use capture origin w, h
			cap_entity->src_bg_dim.w = platform_sys_Info.cap_info[cap_vch].width;
			cap_entity->src_bg_dim.h = platform_sys_Info.cap_info[cap_vch].height;
		} else {
			cap_entity->src_bg_dim.w = p_cap_crop->win.coord.w;
			cap_entity->src_bg_dim.h = p_cap_crop->win.coord.h;
		}
		cap_entity->crop_rect.x = ALIGN4_DOWN(p_cap_crop->win.rect.x);
		cap_entity->crop_rect.y = ALIGN2_DOWN(p_cap_crop->win.rect.y);
		cap_entity->crop_rect.w = ALIGN4_DOWN(p_cap_crop->win.rect.w);
		cap_entity->crop_rect.h = ALIGN2_DOWN(p_cap_crop->win.rect.h);
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT get_vcap_fps_setting(HD_GROUP *p_group, UINT32 line_idx, INT *p_fps)
{
	INT member_idx, lcd_vch;
	HD_MEMBER *p_member;
	DIF_VENC_PARAM venc;
	HD_H26XENC_RATE_CONTROL *p_enc_rc;
	VIDEOENC_JPEG_RATE_CONTROL *p_jpeg_rc;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_fps == NULL) {
		GMLIB_ERROR_P("NULL p_fps\n");
		goto err_exit;
	}


	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			lcd_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			*p_fps = platform_sys_Info.lcd_info[lcd_vch].fps;
			goto exit;
		} else if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
			venc.param = videoenc_get_param_p(p_member->p_bind->device.deviceid);
			if (venc.param == NULL) {
				GMLIB_ERROR_P("NULL venc.param\n");
				goto err_exit;
			}
			if (venc.param->port[p_member->in_subid].p_enc_out_param == NULL) {
				GMLIB_ERROR_P("NULL venc enc out param\n");
				goto err_exit;
			}

			if (venc.param->port[p_member->in_subid].p_enc_out_param->codec_type == HD_CODEC_TYPE_H264 ||
				venc.param->port[p_member->in_subid].p_enc_out_param->codec_type == HD_CODEC_TYPE_H265) {
				if (venc.param->port[p_member->in_subid].p_enc_rc == NULL) {
					GMLIB_ERROR_P("NULL h26x venc.rc param\n");
					goto err_exit;
				}
				goto use_h26x_fps;
			} else if (venc.param->port[p_member->in_subid].p_enc_out_param->codec_type == HD_CODEC_TYPE_JPEG) {
				if (venc.param->port[p_member->in_subid].p_jpeg_rc == NULL) {
					GMLIB_ERROR_P("NULL jpeg venc.rc param\n");
					goto err_exit;
				}
				goto use_jpeg_fps;
			} else if (venc.param->port[p_member->in_subid].p_enc_out_param->codec_type == HD_CODEC_TYPE_RAW) {
				if (venc.param->port[p_member->in_subid].p_enc_rc != NULL) {
					goto use_h26x_fps;
				}
				if (venc.param->port[p_member->in_subid].p_jpeg_rc != NULL) {	//p_jpeg_rc is never null if venc was opened successfully
					goto use_jpeg_fps;	//if neither h26x nor jpeg rc be set, use jpeg default setting, fps = 30/1
				}
				GMLIB_ERROR_P("NULL h26x or jpeg venc.rc param\n");
				goto err_exit;
			}
use_h26x_fps:
			p_enc_rc = venc.param->port[p_member->in_subid].p_enc_rc;
			if (p_enc_rc->rc_mode == HD_RC_MODE_CBR) {
				*p_fps = p_enc_rc->cbr.frame_rate_base / p_enc_rc->cbr.frame_rate_incr;
			} else if (p_enc_rc->rc_mode == HD_RC_MODE_VBR) {
				*p_fps = p_enc_rc->vbr.frame_rate_base / p_enc_rc->vbr.frame_rate_incr;
			} else if (p_enc_rc->rc_mode == HD_RC_MODE_FIX_QP) {
				*p_fps = p_enc_rc->fixqp.frame_rate_base / p_enc_rc->fixqp.frame_rate_incr;
			} else if (p_enc_rc->rc_mode == HD_RC_MODE_EVBR) {
				*p_fps = p_enc_rc->evbr.frame_rate_base / p_enc_rc->evbr.frame_rate_incr;
			} else {
				GMLIB_ERROR_P("No venc.rc mode setting\n");
				goto err_exit;
			}
			goto exit;

use_jpeg_fps:
			p_jpeg_rc = venc.param->port[p_member->in_subid].p_jpeg_rc;
			*p_fps = p_jpeg_rc->frame_rate_base / p_jpeg_rc->frame_rate_incr;
			goto exit;
		}
	}

exit:
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT get_vcap_dma_number(int is_lcd, int is_enc, HD_VIDEO_CODEC codec_type, int *dma_number)
{
	if (dma_number == NULL) {
		GMLIB_ERROR_P("NULL dma_number\n");
		goto err_exit;
	}

	if (is_lcd) {
		*dma_number = platform_sys_Info.liveview_cap_dma;
		return HD_OK;;
	}
	if (is_enc) {
		if (codec_type == HD_CODEC_TYPE_RAW)
			*dma_number = 1;
		else
			*dma_number = platform_sys_Info.record_cap_dma;
		return HD_OK;;
	}
	*dma_number = 1;

err_exit:
	return HD_ERR_NG;
}
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT set_vcap_unit(HD_GROUP *hd_group, INT line_idx, DIF_VCAP_PARAM *vcap_param, INT *len)
{
	INT i, size = 0, fps = 30, enc_fps = 30, is_rotate_align = 0, is_lcd = 0, is_enc = 0, dma_number, cascade_level = 0;
	INT is_next_enc = 0, is_next_di_enc = 0, member_idx, src_fps, dst_fps, frc_valid, is_virtual_lcd = 0;
	pif_group_t *group;
	vpd_entity_t *cap = NULL;
	vpd_cap_entity_t *cap_entity;
	vpd_flip_t flip_mode = VPD_FLIP_NONE;
	vpd_flip_t vcap_flip_mode = VPD_FLIP_NONE;
	INT cap_fd, cap_vch, cap_path, lcd_vch, lcd_path, enc_vch, enc_path, vpe_vch, vpe_path, flip_valid, drc_valid;
	HD_OUT_POOL *p_out_pool;
	HD_DIM tmp_dim = {0, 0};
	HD_DIM out_dim = {0, 0};
	HD_DIM out_bg_dim = {0, 0};
	HD_DIM crop_align_up_dim = {0, 0};
	HD_DIM crop_align_down_dim = {0, 0};
	HD_VIDEO_PXLFMT out_pxlfmt;
	HD_VIDEOCAP_OUT *p_cap_out;
	HD_VIDEOOUT_WIN_ATTR win;
	HD_VIDEOCAP_CROP *p_cap_crop;
	HD_VIDEO_CODEC codec_type = HD_CODEC_TYPE_H265;
	HD_MEMBER *p_member;
	HD_RESULT flip_ret, drc_ret, frc_ret;
	BOOL tmnr_maybe_en = 0;
	HD_DIM enc_dim = {0, 0};
	HD_VIDEO_PXLFMT enc_pxlfmt = HD_VIDEO_PXLFMT_NONE;
	VDOCAP_TWO_CHANNEL_MODE_PARAM *p_cap_two_ch_param;

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
	if (vcap_param == NULL) {
		GMLIB_ERROR_P("NULL vcap_param\n");
		goto err_exit;
	}
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	cap_vch = vcap_param->dev.device_subid;
	cap_path = vcap_param->dev.out_subid;
	cap_fd = utl_get_cap_entity_fd(cap_vch, cap_path, NULL);
	p_cap_two_ch_param = vcap_param->param->cap_two_ch_param[cap_path];
	p_out_pool = vcap_param->param->p_out_pool[cap_path];
	if (p_out_pool == NULL) {
		GMLIB_ERROR_P("<vcap out pool is not set! out_subid(%d) >\n", cap_path);
		goto err_exit;
	}
	p_cap_out = vcap_param->param->cap_out[cap_path];
	if (p_cap_out == NULL) {
		GMLIB_ERROR_P("<vcap out param is not set! out_subid(%d) >\n", cap_path);
		goto err_exit;
	}

	if (p_cap_two_ch_param) {
		if (p_cap_two_ch_param->enable) {
			if (p_cap_out->dir != HD_VIDEO_DIR_NONE) {
				GMLIB_ERROR_P("not support two channel mode with rotation\n", cap_path);
				goto err_exit;
			}
			if (!p_cap_two_ch_param->is_double && p_cap_two_ch_param->state == 1) {
				p_cap_out->dim.w = p_cap_out->dim.w * 2;
				p_cap_two_ch_param->is_double = TRUE;
			}
		} else {
			if (p_cap_two_ch_param->is_double && p_cap_two_ch_param->state == 2) {
				p_cap_out->dim.w = p_cap_out->dim.w / 2;
				p_cap_two_ch_param->is_double = FALSE;
			}
		}
	}

	member_idx = vcap_param->dev.member_idx + 1;
	if (hd_group->glines[line_idx].member[member_idx].p_bind == NULL) {
		GMLIB_ERROR_P("No next unit for vcap\n");
		goto err_exit;
	}
	p_member = &(hd_group->glines[line_idx].member[member_idx]);
	if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
		is_next_enc = 1;
	}

	if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
		HD_VIDEO_PXLFMT vproc_dst_fmt = HD_VIDEO_PXLFMT_NONE;
		HD_URECT vproc_dst_rect = {0};
		HD_DIM vproc_dst_bg_dim = {0};
		BOOL vproc_di_enable = 0;
		if (dif_get_vproc_out_and_di(hd_group, line_idx, &vproc_dst_fmt, &vproc_dst_rect,
					     &vproc_dst_bg_dim, &vproc_di_enable) == HD_OK) {
			if (vproc_di_enable &&
			    vproc_dst_rect.x == 0 &&
			    vproc_dst_rect.y == 0 &&
			    vproc_dst_rect.w == p_cap_out->dim.w &&
			    vproc_dst_rect.h == p_cap_out->dim.h &&
			    vproc_dst_bg_dim.w == p_cap_out->dim.w &&
			    vproc_dst_bg_dim.h == p_cap_out->dim.h) {

				member_idx = vcap_param->dev.member_idx + 2;
				if (hd_group->glines[line_idx].member[member_idx].p_bind == NULL) {
					GMLIB_ERROR_P("No next unit for vcap\n");
					goto err_exit;
				}
				p_member = &(hd_group->glines[line_idx].member[member_idx]);
				if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
					is_next_di_enc = 1;
				}
			}
		}
	}
	if (dif_get_lcd_ch_path(hd_group, line_idx, &lcd_vch, &lcd_path) == HD_OK) {
		is_lcd = 1;
		tmnr_maybe_en = 1;
		if (dif_get_vout_win_attr(hd_group, line_idx, &win) != HD_OK) {
			GMLIB_ERROR_P("<vcap: dif_get_vout_win_attr fail! line_idx(%d)>\n", line_idx);
			goto err_exit;
		}
		if (dif_get_vpe_cascade_level(hd_group, line_idx, &cascade_level) != HD_OK) {
			GMLIB_ERROR_P("<vcap: dif_get_vpe_cascade_level fail! line_idx(%d)>\n", line_idx);
			goto err_exit;
		}
	} else if (dif_get_venc_ch_path(hd_group, line_idx, &enc_vch, &enc_path) == HD_OK) {
		is_enc = 1;
		if (dif_get_venc_codec_type(hd_group, line_idx, &codec_type) != HD_OK) {
			GMLIB_ERROR_P("<venc codec is not set! line_idx(%d) >\n", line_idx);
			goto err_exit;
		}
		if (dif_get_venc_dim_fmt(hd_group, line_idx, &enc_dim, &enc_pxlfmt) != HD_OK) {
			GMLIB_ERROR_P("<venc dim is not set! line_idx(%d) >\n", line_idx);
			goto err_exit;
		}
	}


	frc_ret = videocap_check_vcap_validfrc(p_cap_out->frc, &frc_valid, &dst_fps, &src_fps);
	if (is_lcd == 1) {
		if ((cascade_level >= 2) &&
		    (lcd_vch != 0) &&
		    (platform_sys_Info.lcd_info[lcd_vch].fb0_win.width <= 960)) {
			/* vcap ---> vpe0_0 ---> vpe1 +--> vout0
			 * NOTE: multi_path, we choose p_cap_out->frc
			 */
			if (frc_ret != HD_OK) {//invalid case
				GMLIB_ERROR_P("vcap out param p_cap_out->frc(%#x) is not correct, device_subid(%d) out_subid(%d)\n",
					      p_cap_out->frc, cap_vch, cap_path);
				GMLIB_ERROR_P("using syscap fps(%d)\n", platform_sys_Info.cap_info[cap_vch].fps);

				fps = platform_sys_Info.cap_info[cap_vch].fps;
			} else {
				fps = (dst_fps * platform_sys_Info.cap_info[cap_vch].fps) / src_fps;
				if (fps < 1 || fps > platform_sys_Info.cap_info[cap_vch].fps) {//invalid case
					GMLIB_ERROR_P("vcap out param p_cap_out->frc(%#x) is not correct, device_subid(%d) out_subid(%d)\n",
						      p_cap_out->frc, cap_vch, cap_path);
					GMLIB_ERROR_P("using syscap fps(%d)\n", platform_sys_Info.cap_info[cap_vch].fps);
					fps = platform_sys_Info.cap_info[cap_vch].fps;
				}
			}
		} else {
			fps = pif_cal_disp_tick_fps(lcd_vch);
		}
	} else {
		if (frc_ret != HD_OK) {//not valid, maybe no set
			GMLIB_WARNING_P("vcap frc: not valid %u, set default to %d\n", dst_fps, VCAP_DEFAULT_FPS);
		}
		/* encode case */
		if (is_next_enc || is_next_di_enc) {
			/* vcap ---> venc */
			if (get_vcap_fps_setting(hd_group, line_idx, &enc_fps) != HD_OK) {
				// if fps is not found, use "p_cap_out->frc" as the default value
				if (frc_ret != HD_OK) {
					GMLIB_ERROR_P("warning:vcap out param p_cap_out->frc(%#x) is not correct, device_subid(%d) out_subid(%d)\n",
						      p_cap_out->frc, cap_vch, cap_path);
					GMLIB_ERROR_P("using min of (enc fps(%d), cap fps(%d))\n",
						      enc_fps, platform_sys_Info.cap_info[cap_vch].fps);
					fps = MIN_VAL(enc_fps, platform_sys_Info.cap_info[cap_vch].fps);

				} else {
					fps = (dst_fps * platform_sys_Info.cap_info[cap_vch].fps) / src_fps;
					if (fps < 1 || fps > platform_sys_Info.cap_info[cap_vch].fps) {//invalid case
						fps = platform_sys_Info.cap_info[cap_vch].fps;
						GMLIB_ERROR_P("warning:vcap out param p_cap_out->frc(%#x) is invalid, device_subid(%d) out_subid(%d)\n",
							      p_cap_out->frc, cap_vch, cap_path);
						GMLIB_ERROR_P("using min of (enc fps(%d), cap fps(%d))\n",
							      enc_fps, platform_sys_Info.cap_info[cap_vch].fps);
						fps = MIN_VAL(enc_fps, platform_sys_Info.cap_info[cap_vch].fps);

					} else {
						fps = MIN_VAL(enc_fps, fps);
					}
				}
			} else {
				if (enc_fps <= platform_sys_Info.cap_info[cap_vch].fps) {//valid ratio
					GMLIB_L1_P("enc fps(%d) < cap fps(%d), maybe need to set frc in HD_VIDEOCAP_PARAM_OUT\n",
						   enc_fps, platform_sys_Info.cap_info[cap_vch].fps);
				}
				fps = MIN_VAL(enc_fps, platform_sys_Info.cap_info[cap_vch].fps);
			}
		} else {
			/* vcap ---> vpe0_0 +--> venc0
			 *               _1 +--> venc1
			 * NOTE: multi_path, we choose p_cap_out->frc
			 */
			if (frc_ret != HD_OK) {//invalid case
				GMLIB_ERROR_P("vcap out param p_cap_out->frc(%#x) is not correct, device_subid(%d) out_subid(%d)\n",
					      p_cap_out->frc, cap_vch, cap_path);
				GMLIB_ERROR_P("using syscap fps(%d)\n", platform_sys_Info.cap_info[cap_vch].fps);

				fps = platform_sys_Info.cap_info[cap_vch].fps;
			} else {
				fps = (dst_fps * platform_sys_Info.cap_info[cap_vch].fps) / src_fps;
				if (fps < 1 || fps > platform_sys_Info.cap_info[cap_vch].fps) {//invalid case
					GMLIB_ERROR_P("vcap out param p_cap_out->frc(%#x) is not correct, device_subid(%d) out_subid(%d)\n",
						      p_cap_out->frc, cap_vch, cap_path);
					GMLIB_ERROR_P("using syscap fps(%d)\n", platform_sys_Info.cap_info[cap_vch].fps);
					fps = platform_sys_Info.cap_info[cap_vch].fps;
				}
			}
		}
	}
	//fps = MIN_VAL(fps, platform_sys_Info.cap_info[cap_vch].fps);

	if (is_lcd && (win.visible == 0)) {
		out_dim.w = platform_sys_Info.cap_info[cap_vch].width / videocap_hw_ability.scaler_h_ratio_max;
		out_dim.h = platform_sys_Info.cap_info[cap_vch].height / videocap_hw_ability.scaler_v_ratio_max;
	} else {
		out_dim.w = p_cap_out->dim.w;
		out_dim.h = p_cap_out->dim.h;
	}
	out_pxlfmt = p_cap_out->pxlfmt;

	cap = pif_lookup_entity_buf(VPD_CAP_ENTITY_TYPE, cap_fd, group->encap_buf);
	if (cap != NULL) {
		*len = 0;
		goto exit;
	}
	if (is_enc && platform_sys_Info.cap_info[cap_vch].vcapch < 0) {
		GMLIB_ERROR_P("<cap: vch(%d) capch(%d) error!>\n", cap_vch, platform_sys_Info.cap_info[cap_vch].vcapch);
		goto err_exit;
	}
	if (is_lcd && platform_sys_Info.lcd_info[lcd_vch].lcdid < 0) {
		dif_get_lcd_output_type(hd_group, line_idx, &is_virtual_lcd);
		if (is_virtual_lcd == 0) {
			GMLIB_ERROR_P("<lcd: vch(%d) id(%d) error!>\n", lcd_vch, platform_sys_Info.lcd_info[lcd_vch].lcdid);
			goto err_exit;
		}
	}

	*len = pif_set_EntityBuf(VPD_CAP_ENTITY_TYPE, &cap, group->encap_buf);
	if (cap == NULL) {
		GMLIB_ERROR_P("<vcap: NULL entity buffer!>\n");
		goto err_exit;
	}
	cap->sn = cap_fd;
	cap->ap_bindfd = BD_SET_PATH(vcap_param->dev.device_baseid + vcap_param->dev.device_subid,
				     vcap_param->dev.in_subid + HD_IN_BASE,
				     vcap_param->dev.out_subid + HD_OUT_BASE);

	if (get_vcap_flip_mode(hd_group, line_idx, &flip_mode, &is_rotate_align) != HD_OK) {
		GMLIB_ERROR_P("<vcap: get_vcap_flip_mode fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}
	//if (get_vcap_dma_number(is_lcd, is_enc, codec_type, &dma_number) != HD_OK) {
	//	GMLIB_ERROR_P("<vcap: get_vcap_dma_number fail! islcd(%d)(%d)(%d)>\n", is_lcd, is_enc, codec_type);
	//	goto err_exit;
	//}
	dma_number = (cap_path % 2) ? 1 : 0;

	memset(cap->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < VPD_MAX_BUFFER_NUM; i++) {
		if (p_out_pool->buf_info[i].enable == HD_BUF_DISABLE) {
			cap->pool[i].enable = FALSE;
			continue;
		} else if (p_out_pool->buf_info[i].enable == HD_BUF_ENABLE) {
			cap->pool[i].enable = TRUE;
			cap->pool[i].ddr_id = p_out_pool->buf_info[i].ddr_id;
			cap->pool[i].count_decuple = p_out_pool->buf_info[i].counts;
			if (dif_is_di_enable(hd_group, line_idx, NULL)) {
				//in DI liveview case, vpe316(di_tmnr) and vpe536(vpe_tmnr) may need one buffer both.
				//so vcap max buffer must minus one.
				tmnr_maybe_en = 1;//di may occupy one buffer
			}
			cap->pool[i].max_counts = get_vcap_max_counts(p_out_pool->buf_info[i].counts, tmnr_maybe_en);
			GMLIB_L1_P("maxcounts[%u][%u] = %u\n", cap_vch, cap_path, cap->pool[i].max_counts);
			cap->pool[i].min_counts = DEFAULT_MIN_COUNT;
			if (cap->pool[i].max_counts < DEFAULT_MAX_COUNT) {
				GMLIB_ERROR_P("set_vcap_pool fail (%d), counts(%lu), max_count(%lu) at least %u !!\n",
					      i, p_out_pool->buf_info[i].counts, cap->pool[i].max_counts, DEFAULT_MAX_COUNT);
				goto err_exit;
			}
		} else { //HD_BUF_AUTO
			if (i == 0) {
				cap->pool[i].enable = TRUE;
				cap->pool[i].ddr_id = 0;
				cap->pool[i].count_decuple = 30;
				cap->pool[i].max_counts = DEFAULT_MAX_COUNT;
				cap->pool[i].min_counts = DEFAULT_MIN_COUNT;
			} else {
				cap->pool[i].enable = FALSE;
			}
		}
		if (is_lcd) {
			if (i == 1) {
				tmp_dim.w = out_dim.w;
				tmp_dim.h = out_dim.h;
				size = common_calculate_md_info_size(&tmp_dim);
				cap->pool[i].dst_fmt = VPD_BUFTYPE_MD;
				cap->pool[i].type = VPD_FLOW_MD_POOL;
			} else {
				if (is_rotate_align) {
					tmp_dim.w = ALIGN16_UP(out_dim.w);
					tmp_dim.h = ALIGN64_UP(out_dim.h);
				} else {
					tmp_dim.w = ALIGN16_UP(out_dim.w);
					tmp_dim.h = ALIGN16_UP(out_dim.h);
				}
				size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(tmp_dim, out_pxlfmt);
				cap->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(out_pxlfmt);

				if ((cascade_level >= 2) &&
				    (lcd_vch != 0) &&
				    (platform_sys_Info.lcd_info[lcd_vch].fb0_win.width <= 960)) {

					/* for cvbs_lv and enc_substream, vpe_multi_path */
					cap->pool[i].type = VPD_ENC_CAP_OUT_POOL;

				} else {
					cap->pool[i].type = VPD_DISP0_CAP_OUT_POOL + lcd_vch;
				}
			}
		} else if (is_enc) {
			if (i == 1) {
				size = common_calculate_md_info_size(&tmp_dim);
				cap->pool[i].dst_fmt = VPD_BUFTYPE_MD;
				cap->pool[i].type = VPD_FLOW_MD_POOL;
			} else {
				tmp_dim.w = ALIGN16_UP(out_dim.w);
				tmp_dim.h = ALIGN16_UP(out_dim.h);
				size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(tmp_dim, out_pxlfmt);
				cap->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(out_pxlfmt);
				cap->pool[i].type = VPD_ENC_CAP_OUT_POOL;
			}

		}
		if (dif_is_di_enable(hd_group, line_idx, NULL)) {
			snprintf(cap->pool[i].name, GM_POOL_NAME_LEN, "%s_di_%#x",
				 pif_get_pool_name_by_type(cap->pool[i].type), cap_fd);
		} else {
			snprintf(cap->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x",
				 pif_get_pool_name_by_type(cap->pool[i].type), cap_fd);
		}
		cap->pool[i].width = ALIGN16_UP(out_dim.w);
		cap->pool[i].height = ALIGN16_UP(out_dim.h);
		cap->pool[i].fps = fps;
		cap->pool[i].vch = cap_vch;
		cap->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		cap->pool[i].is_duplicate = 0;
		cap->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(out_pxlfmt);
		cap->pool[i].size = size;
		cap->pool[i].win_size = 0;

		GMLIB_L2_P("set_vcap_pool i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) fmt(%#x) dup(%d)\n",
			   i, cap->pool[i].enable, cap->pool[i].name, cap->pool[i].type, cap->pool[i].ddr_id,
			   cap->pool[i].size, cap->pool[i].count_decuple, cap->pool[i].max_counts,
			   cap->pool[i].dst_fmt, cap->pool[i].is_duplicate);
	}

	cap_entity = (vpd_cap_entity_t *)cap->e;
	cap_entity->pch = cap_vch;
	cap_entity->cap_vch = cap_vch;
	cap_entity->path = cap_path;
	cap_entity->buf_offset = GMLIB_NULL_VALUE;
	cap_entity->buf_size = cap->pool[0].size;
	cap_entity->frame_rate.target_fps = fps;
	cap_entity->frame_rate.ratio = MAKE_UINT16_UINT16(dst_fps, src_fps);
	if (fps != src_fps) {
		dst_fps = fps;
		src_fps = platform_sys_Info.cap_info[cap_vch].fps;
		if (dst_fps > src_fps) {
			GMLIB_L2_P("warning:vcap out param frame_rate ratio is not correct\n");
			GMLIB_L2_P("dst_fps %u should < src_fps %u.\n", dst_fps, src_fps);
			dst_fps = src_fps;
		}
	}
	drc_ret = videocap_check_vcap_validdrc((int)p_cap_out->depth, &drc_valid);
	if (drc_ret == HD_OK) {
		cap_entity->drc_mode = (int)p_cap_out->depth;
	} else {
		cap_entity->drc_mode = 0;
	}
	flip_ret = videocap_check_vcap_validdir(p_cap_out->dir, &flip_valid, &vcap_flip_mode);
	if (flip_ret == HD_OK) {
		cap_entity->flip_mode = vcap_flip_mode;
	} else {//follow vpe setting
		cap_entity->flip_mode = VPD_FLIP_NONE;
	}
	p_cap_crop = vcap_param->param->cap_crop[cap_path];
	if (p_cap_crop && (get_vcap_crop_setting(p_cap_crop, cap_vch, cap_entity) != HD_OK)) {
		GMLIB_ERROR_P("<vcap: get_vcap_crop_setting fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	} else if (codec_type == HD_CODEC_TYPE_RAW) {
		cap_entity->src_bg_dim.w = ALIGN16_UP(platform_sys_Info.cap_info[cap_vch].width);
		cap_entity->src_bg_dim.h = ALIGN16_UP(platform_sys_Info.cap_info[cap_vch].height);
	} else {
		cap_entity->src_bg_dim.w = platform_sys_Info.cap_info[cap_vch].width;
		cap_entity->src_bg_dim.h = platform_sys_Info.cap_info[cap_vch].height;
	}

	if (dif_get_align_up_dim(out_dim, out_pxlfmt, &out_bg_dim) != HD_OK) {
		GMLIB_ERROR_P("<vcap: dif_get_align_up_dim! line_idx(%d), cap_out_dim(%lu,%lu) pxlfmt(%#x)>\n",
			      line_idx, p_cap_out->dim.w, p_cap_out->dim.h, p_cap_out->pxlfmt);
		goto err_exit;
	} else {
		/* De-interlace need 16 alignment */
		if (dif_is_di_enable(hd_group, line_idx, NULL)) {
			cap_entity->dst_bg_dim.w = ALIGN16_DOWN(out_bg_dim.w);
			cap_entity->dst_bg_dim.h = ALIGN16_DOWN(out_bg_dim.h);
		} else {
			cap_entity->dst_bg_dim.w = out_bg_dim.w;
			cap_entity->dst_bg_dim.h = out_bg_dim.h;
		}
	}

	cap_entity->dst_rect.x = 0;
	cap_entity->dst_rect.y = 0;
	cap_entity->dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(out_pxlfmt);

	/* set dst_dim to next entity when crop enable */
	if (p_cap_crop && (p_cap_crop->mode == HD_CROP_ON)) {
		tmp_dim.w = cap_entity->crop_rect.w;
		tmp_dim.h = cap_entity->crop_rect.h;
		if (cap_entity->crop_rect.w > p_cap_out->dim.w) { //need scale down
			cap_entity->dst_rect.w = p_cap_out->dim.w;
			tmp_dim.w = p_cap_out->dim.w;
		}
		if (cap_entity->crop_rect.h > p_cap_out->dim.h) { //need scale down
			cap_entity->dst_rect.h = p_cap_out->dim.h;
			tmp_dim.h = p_cap_out->dim.h;
		}
	} else {
		tmp_dim.w = out_dim.w;
		tmp_dim.h = out_dim.h;
	}
	if (dif_get_align_down_dim(tmp_dim, out_pxlfmt, &crop_align_down_dim) != HD_OK) {
		GMLIB_ERROR_P("<dif_get_align_down_dim fail >\n");
		goto err_exit;
	}
	if (p_cap_two_ch_param && p_cap_two_ch_param->enable) {
		if (p_cap_crop && (p_cap_crop->mode == HD_CROP_ON)) {
			GMLIB_ERROR_P("not support two channel mode with crop\n", cap_path);
			goto err_exit;
		}
		crop_align_down_dim.w = crop_align_down_dim.w /2;
	}
	/* De-interlace need 16 alignment */
	if (dif_is_di_enable(hd_group, line_idx, NULL)) {
		cap_entity->dst_rect.w = ALIGN16_DOWN(crop_align_down_dim.w);
		cap_entity->dst_rect.h = ALIGN16_DOWN(crop_align_down_dim.h);
	} else {
		cap_entity->dst_rect.w = crop_align_down_dim.w;
		cap_entity->dst_rect.h = crop_align_down_dim.h;
	}

#if 0
	//check vpe output size from proc and input size > 2048,
	if (disp_info->vpe_win_rect.w  > platform_sys_Info.vpe_out_linebuf_sz || ALIGN64_DOWN(cap_entity->dst_rect.w) > 2048) {
		cap_entity->dst_rect.x = 384;
	}

	if (cap_entity->dst_rect.w > 2048) { //for 2096 align to 64=2048 not to do col mode
		cap_entity->dst_rect.w = ALIGN64_DOWN(cap_entity->dst_rect.w);
	}
#endif

	cap_entity->method = VPD_CAP_METHOD_FRAME;
	cap_entity->auto_aspect_ratio = 0;
	cap_entity->dummy_skip = 0; //not to skip dummy under liveview
	cap_entity->frame_retrieve_when_overrun = 0;
	cap_entity->dma_number = dma_number;
	//cap_entity->flip_mode = flip_mode;//move forward
	cap_entity->ext_ctrl.pallete_idx = 0;
	cap_entity->ext_ctrl.is_right_extend = 1;

	/* extend horizontal pixels  [15: 0]extend right pixels
								 [31:16]extend left pixels
	   extend vertical pixels 	 [7: 0]extend bottom lines
								 [15:8]extend top lines	 */
	if (p_cap_crop && (p_cap_crop->mode == HD_CROP_ON)) {
		tmp_dim.w = cap_entity->crop_rect.w;
		tmp_dim.h = cap_entity->crop_rect.h;
		if (cap_entity->crop_rect.w > p_cap_out->dim.w) { //need scale down
			//a speical case : crop = 620, w = 618, dst should align down
			if (p_cap_out->dim.w != crop_align_down_dim.w)
				cap_entity->dst_rect.w = crop_align_down_dim.w;
			else
				cap_entity->dst_rect.w = p_cap_out->dim.w;
			tmp_dim.w = p_cap_out->dim.w;
		}
		if (cap_entity->crop_rect.h > p_cap_out->dim.h) { //need scale down
			//a speical case : crop = 620, w = 618, dst should align down
			if (p_cap_out->dim.h != crop_align_down_dim.h)
				cap_entity->dst_rect.h = crop_align_down_dim.h;
			else
				cap_entity->dst_rect.h = p_cap_out->dim.h;
			tmp_dim.h = p_cap_out->dim.h;
		}
		if (dif_get_align_up_dim(tmp_dim, out_pxlfmt, &crop_align_up_dim) != HD_OK) {
			GMLIB_ERROR_P("<dif_get_bg_dim fail >\n");
			printf("cropzzz manual %u, up%lu, down%lu\n", cap_entity->ext_ctrl.ext_manual_h, crop_align_up_dim.w, crop_align_down_dim.w);
			goto err_exit;
		}
		cap_entity->ext_ctrl.ext_manual_h = crop_align_up_dim.w - crop_align_down_dim.w;
		cap_entity->ext_ctrl.ext_manual_v = crop_align_up_dim.h - crop_align_down_dim.h;

		if (dif_get_vpe_ch_path(hd_group, line_idx, &vpe_vch, &vpe_path) != HD_OK) {//no vpe
			HD_DIM outtemp_dim = {0};
			if (dif_get_align_up_dim(enc_dim, out_pxlfmt, &outtemp_dim) != HD_OK) {
				GMLIB_ERROR_P("<dif_get_outtemp_dim fail >\n");
				goto err_exit;
			}
			if ((cap_entity->dst_rect.w != outtemp_dim.w) ||
			    (cap_entity->dst_rect.h != outtemp_dim.h)) { //need vpe
				GMLIB_ERROR_P("CROP w = %d, h = %d, dst w = %d, h = %d.\n", cap_entity->crop_rect.w, cap_entity->crop_rect.h, outtemp_dim.w, outtemp_dim.h);
				GMLIB_ERROR_P("Vcap CROP needs VPE scale\n");
				goto err_exit;
			}
		}

	} else {
		validate_cap_dim_wh_align(out_dim, out_pxlfmt, &tmp_dim);

		cap_entity->ext_ctrl.ext_manual_h = out_bg_dim.w - tmp_dim.w;
		cap_entity->ext_ctrl.ext_manual_v = out_bg_dim.h - tmp_dim.h;
	}

	if (is_lcd && (win.visible == 0)) {
		SET_ENTITY_FEATURE(cap->feature, VPD_FLAG_DIR_CB);
	} else {
		SET_ENTITY_FEATURE(cap->feature, VPD_FLAG_ENODE_ENABLE);
	}
	/*
	 cap->group_id =  ((DISP_CAP << 24) | ((group->groupidx & 0xff) << 16) |
	 ((platform_sys_Info.cap_info[cap_attrs->basic->cap_vch].chipid  & 0xff) << 8) |
	 (platform_sys_Info.lcd_info[win_attrs->basic->lcd_vch].lcd_vch & 0xff));
	 */

	if (p_cap_two_ch_param && p_cap_two_ch_param->enable) {
#if 0	//is_bind will lock
		if (is_bind(HD_GET_DEV(p_cap_two_ch_param->sec_path_id), VDOCAP_OUTID(HD_GET_OUT(p_cap_two_ch_param->sec_path_id)), 0)) {
			GMLIB_ERROR_P("sec_path_id(%#lx) of two channel mode is bound\n", p_cap_two_ch_param->sec_path_id);
		} else {
			cap_entity->two_ch_mode.enable = 1;
			cap_entity->two_ch_mode.secondary_engine = (p_cap_two_ch_param->sec_side_fd >> 8) & 0xFF;
		}
#endif
		cap_entity->two_ch_mode.enable = 1;
		cap_entity->two_ch_mode.secondary_engine = ENTITY_FD_ENGINE(p_cap_two_ch_param->sec_side_fd);
	} else {
		cap_entity->two_ch_mode.enable = 0;
		cap_entity->two_ch_mode.secondary_engine = 0;
	}

	SET_ENTITY_FEATURE(cap->feature, VPD_FLAG_OUTBUF_MAX_CNT | VPD_FLAG_HIGH_PRIORITY);

	GMLIB_L2_P("set_vcap_entity path_id(%#lx) fd(%#x) pch(%d) src_bg(%d,%d) crop_rect(%d,%d,%d,%d), dst_bg(%d,%d) dst_rect(%d,%d,%d,%d) dst_fmt(%#x) fps(%d) ratio(%#x)\n",
		   dif_get_path_id(&vcap_param->dev), cap_fd, cap_entity->pch,
		   cap_entity->src_bg_dim.w, cap_entity->src_bg_dim.h,
		   cap_entity->crop_rect.x, cap_entity->crop_rect.y,
		   cap_entity->crop_rect.w, cap_entity->crop_rect.h,
		   cap_entity->dst_bg_dim.w, cap_entity->dst_bg_dim.h,
		   cap_entity->dst_rect.x, cap_entity->dst_rect.y,
		   cap_entity->dst_rect.w, cap_entity->dst_rect.h,
		   cap_entity->dst_fmt, cap_entity->frame_rate.target_fps, cap_entity->frame_rate.ratio);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, cap);

err_exit:
	return -1;
}

INT set_di_unit(HD_GROUP *hd_group, INT line_idx, DIF_VCAP_PARAM *vcap_param,
		VDOPROC_INTL_PARAM *p_vproc_intl_param, INT *len)
{
	INT i, size, fps, is_lcd = 0, is_enc = 0, is_virtual_lcd = 0;;
	pif_group_t *group;
	vpd_entity_t *di;
	vpd_di_entity_t *di_entity;
	INT cap_fd, di_fd, vpe_vch, vpe_path, cap_vch, cap_path, lcd_vch, lcd_path, enc_vch, enc_path;
	HD_OUT_POOL *p_out_pool;
	HD_DIM in_dim = {0, 0};
	HD_VIDEO_PXLFMT in_pxlfmt;
	HD_VIDEO_PXLFMT out_pxlfmt;
	HD_VIDEO_CODEC codec_type = HD_CODEC_TYPE_H265;
	HD_DIM enc_dim = {0, 0};
	HD_VIDEO_PXLFMT enc_pxlfmt = HD_VIDEO_PXLFMT_NONE;

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
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}
	if (p_vproc_intl_param == NULL) {
		GMLIB_ERROR_P("NULL vproc param\n");
		goto err_exit;
	}
	cap_vch = vcap_param->dev.device_subid;
	cap_path = vcap_param->dev.out_subid;

	if (dif_get_lcd_ch_path(hd_group, line_idx, &lcd_vch, &lcd_path) == HD_OK) {
		is_lcd = 1;
	} else if (dif_get_venc_ch_path(hd_group, line_idx, &enc_vch, &enc_path) == HD_OK) {
		is_enc = 1;
		if (dif_get_venc_codec_type(hd_group, line_idx, &codec_type) != HD_OK) {
			GMLIB_ERROR_P("<venc codec is not set! line_idx(%d) >\n", line_idx);
			goto err_exit;
		}
		if (dif_get_venc_dim_fmt(hd_group, line_idx, &enc_dim, &enc_pxlfmt) != HD_OK) {
			GMLIB_ERROR_P("<venc dim is not set! line_idx(%d) >\n", line_idx);
			goto err_exit;
		}
	}
	if (dif_get_vpe_ch_path(hd_group, line_idx, &vpe_vch, &vpe_path) != HD_OK) {
		GMLIB_ERROR_P("No videoproc unit bind for deinterlace function\n");
		goto err_exit;
	}
	di_fd = utl_get_di_entity_fd(vpe_vch, vpe_path, (VOID *)0);
	cap_fd = utl_get_cap_entity_fd(cap_vch, cap_path, (VOID *)0);
	p_out_pool = vcap_param->param->p_out_pool[cap_path];
	if (p_out_pool == NULL) {
		GMLIB_ERROR_P("<vcap out pool is not set! device_subid(%d) >\n", cap_path);
		goto err_exit;
	}

	if (vcap_param->param->cap_out[cap_path] == NULL) {
		GMLIB_ERROR_P("<vcap out param is not set! out_subid(%d) >\n", cap_path);
		goto err_exit;
	}

	di = pif_lookup_entity_buf(VPD_DI_ENTITY_TYPE, di_fd, group->encap_buf);
	if (di != NULL) {
		*len = 0;
		goto exit;
	}
	if (platform_sys_Info.cap_info[cap_vch].vcapch < 0) {
		GMLIB_ERROR_P("<cap: vch(%d) capch(%d) error!>\n", cap_vch, platform_sys_Info.cap_info[cap_vch].vcapch);
		goto err_exit;
	}
	if (is_lcd && (platform_sys_Info.lcd_info[lcd_vch].lcdid < 0)) {
		dif_get_lcd_output_type(hd_group, line_idx, &is_virtual_lcd);
		if (is_virtual_lcd == 0) {
			GMLIB_ERROR_P("<lcd: vch(%d) id(%d) error!>\n", lcd_vch, platform_sys_Info.lcd_info[lcd_vch].lcdid);
			goto err_exit;
		}
	}

	in_pxlfmt = vcap_param->param->cap_out[cap_path]->pxlfmt;
	if (in_pxlfmt != HD_VIDEO_PXLFMT_YUV422_ONE) {
		GMLIB_ERROR_P("<cap: vch(%d) capch(%d), di in-pxlfmt only supports YUV422ONE, fmt(%#x)>\n",
			      cap_vch, platform_sys_Info.cap_info[cap_vch].vcapch, in_pxlfmt);
		goto err_exit;
	}
	if (p_vproc_intl_param->p_dst_fmt) {
		out_pxlfmt = *p_vproc_intl_param->p_dst_fmt;
	} else if (is_enc) {
		out_pxlfmt = enc_pxlfmt;
	} else {
		GMLIB_ERROR_P("<cap: vch(%d) capch(%d), get di out-pxlfmt fail>\n",
			      cap_vch, platform_sys_Info.cap_info[cap_vch].vcapch);
		goto err_exit;
	}
	if (out_pxlfmt != HD_VIDEO_PXLFMT_YUV420 && out_pxlfmt != HD_VIDEO_PXLFMT_YUV420_NVX3 &&
	    out_pxlfmt != HD_VIDEO_PXLFMT_YUV422_ONE && out_pxlfmt != HD_VIDEO_PXLFMT_YUV420_MB) {
		GMLIB_ERROR_P("<cap: vch(%d) capch(%d), di out-pxlfmt only supports YUV422_ONE/YUV420/YUV420_NVX3/YUV420_MB, fmt(%#x)>\n",
			      cap_vch, platform_sys_Info.cap_info[cap_vch].vcapch, out_pxlfmt);
		goto err_exit;
	}

	in_dim.w = ALIGN16_DOWN(vcap_param->param->cap_out[cap_path]->dim.w);
	in_dim.h = ALIGN16_DOWN(vcap_param->param->cap_out[cap_path]->dim.h);

	if (is_lcd) {
		fps = platform_sys_Info.lcd_info[lcd_vch].fps;
	} else {
		fps = platform_sys_Info.cap_info[cap_vch].fps;
	}

	*len = pif_set_EntityBuf(VPD_DI_ENTITY_TYPE, &di, group->encap_buf);
	if (di == NULL) {
		GMLIB_ERROR_P("<di: NULL entity buffer!>\n");
		goto err_exit;
	}
	di->sn = di_fd;
	di->ap_bindfd = BD_SET_PATH(vcap_param->dev.device_baseid + vcap_param->dev.device_subid,
				    vcap_param->dev.in_subid + HD_IN_BASE,
				    vcap_param->dev.out_subid + HD_OUT_BASE);
	memset(di->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < VPD_MAX_BUFFER_NUM; i++) {
		if (p_out_pool->buf_info[i].enable == HD_BUF_DISABLE) {
			di->pool[i].enable = FALSE;
			continue;
		} else {
			di->pool[i].enable = TRUE;
		}
		size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(in_dim, out_pxlfmt);
		if (is_lcd) {
			di->pool[i].type = VPD_DISP0_CAP_OUT_POOL + lcd_vch;
		} else if (is_enc) {
			di->pool[i].type = VPD_ENC_CAP_OUT_POOL;
		}
		snprintf(di->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x", pif_get_pool_name_by_type(di->pool[i].type), cap_fd);
		di->pool[i].width = in_dim.w;
		di->pool[i].height = in_dim.h;
		di->pool[i].fps = fps;
		di->pool[i].vch = cap_vch;
		di->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		di->pool[i].is_duplicate = 0;
		di->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(out_pxlfmt);
		di->pool[i].size = size;
		di->pool[i].ddr_id = p_out_pool->buf_info[i].ddr_id;
		di->pool[i].win_size = 0;
		di->pool[i].count_decuple = p_out_pool->buf_info[i].counts;
		di->pool[i].max_counts = get_vcap_max_counts(p_out_pool->buf_info[i].counts, 0);
		di->pool[i].min_counts = DEFAULT_MIN_COUNT;
		if (di->pool[i].max_counts < DEFAULT_MAX_COUNT) {
			GMLIB_ERROR_P("set_di_pool fail (%d), counts(%lu), max_count(%lu) at least %u !!\n",
				      i, p_out_pool->buf_info[i].counts, di->pool[i].max_counts, DEFAULT_MAX_COUNT);
			goto err_exit;
		}
		GMLIB_L2_P("set_di_pool i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) fmt(%#x) dup(%d)\n",
			   i, di->pool[i].enable, di->pool[i].name, di->pool[i].type, di->pool[i].ddr_id, di->pool[i].size,
			   di->pool[i].count_decuple, di->pool[i].max_counts, di->pool[i].dst_fmt, di->pool[i].is_duplicate);
	}

	di_entity = (vpd_di_entity_t *)di->e;
	di_entity->pch = cap_vch;
	di_entity->src_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(in_pxlfmt);
	di_entity->dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(out_pxlfmt);
	di_entity->src_dim.w = in_dim.w;
	di_entity->src_dim.h = in_dim.h;
	di_entity->dst_dim.w = in_dim.w;
	di_entity->dst_dim.h = in_dim.h;
	di_entity->pch_count = 1;
	di_entity->engine_nu = 0;
	di_entity->predict_enable = 0;
	di_entity->pch = 0;
	di_entity->di_mode = VPD_DI_TEMPORAL_SPATIAL;
	di_entity->dn_mode = VPD_DN_TEMPORAL_SPATIAL;

	SET_ENTITY_FEATURE(di->feature, VPD_FLAG_ENODE_ENABLE);
	SET_ENTITY_FEATURE(di->feature, VPD_FLAG_OUTBUF_MAX_CNT);

	GMLIB_L2_P("set_di_entity path_id(%#lx) fd(%#x) pch(%d) src_dim(%d,%d) src_fmt(%#x), dst_dim(%d,%d) dst_fmt(%#x)\n",
		   dif_get_path_id(&vcap_param->dev), di_fd, di_entity->pch,
		   di_entity->src_dim.w, di_entity->src_dim.h,
		   di_entity->src_fmt,
		   di_entity->dst_dim.w, di_entity->dst_dim.h,
		   di_entity->dst_fmt);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, di);

err_exit:
	return -1;
}
