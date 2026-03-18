/**
	@brief This sample demonstrates the usage of video process functions.

	@file videoproc_convert.c

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
#include "videoproc_convert.h"
#include "vg_common.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define	DIN_VAR_POOL_NUM			1  // datain max variable pool number
#define	DOUT_POOL_NUM				1  // dataout max pool number
#define VPROC_MAX_COUNT_FOR_VOUT    3
#define VPROC_MAX_COUNT_FOR_VPROC   2
#define VPROC_MAX_COUNT_FOR_OTHER   1
#define VPROC_MIN_COUNT    			0

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
INT convert_hdal_to_vpd_flip_rotate(HD_VIDEO_DIR direction, vpd_rotate_t *rotate, vpd_flip_t *flip);

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
static unsigned int get_vproc_max_counts(unsigned int counts)
{
	unsigned int counts_carry;

	counts_carry = (counts / 10) + (counts % 10 != 0); // unconditional carry

	return counts_carry;
}

HD_RESULT get_vproc_rotation(DIF_VPROC_PARAM *vproc_param, vpd_rotate_t *p_rotate_mode, vpd_flip_t *p_flip_mode)
{
	HD_VIDEO_DIR direction = HD_VIDEO_DIR_NONE;

	if (vproc_param == NULL) {
		GMLIB_ERROR_P("NULL vproc_param\n");
		goto err_exit;
	}
	if (vproc_param->param == NULL) {
		GMLIB_ERROR_P("NULL vproc_param->param\n");
		goto err_exit;
	}
	if (p_flip_mode == NULL) {
		GMLIB_ERROR_P("NULL p_flip_mode\n");
		goto err_exit;
	}
	if (p_rotate_mode == NULL) {
		GMLIB_ERROR_P("NULL p_rotate_mode\n");
		goto err_exit;
	}
	if (vproc_param->param->p_direction != NULL) {
		direction = *vproc_param->param->p_direction;
	} else {
		*p_flip_mode = VPD_FLIP_NONE;
		*p_rotate_mode = VPD_ROTATE_NONE;
	}
	convert_hdal_to_vpd_flip_rotate(direction, p_rotate_mode, p_flip_mode);

	return HD_OK;

err_exit:
	return HD_ERR_NG;

}

HD_RESULT get_vproc_crop_setting(DIF_VPROC_PARAM *vproc_param, vpd_vpe_1_entity_t *vpe_1_entity, INT prev_is_vdec)
{
	HD_URECT *p_crop_rect, *p_pip_crop_rect;
	HD_DIM *p_crop_dim, *p_pip_crop_dim;
	INT *p_crop_enable, *p_pip_crop_enable;

	if (vproc_param == NULL) {
		GMLIB_ERROR_P("NULL vproc_param\n");
		goto err_exit;
	}
	if (vproc_param->param == NULL) {
		GMLIB_ERROR_P("NULL vproc_param\n");
		goto err_exit;
	}
	if (vpe_1_entity == NULL) {
		GMLIB_ERROR_P("NULL vpe_1_entity\n");
		goto err_exit;
	}

	p_crop_rect = vproc_param->param->p_src_rect;
	p_crop_dim = vproc_param->param->p_src_bg_dim;
	p_crop_enable = (INT *)vproc_param->param->p_src_rect_enable;
	p_pip_crop_rect = vproc_param->param->p_psr_crop_rect;
	p_pip_crop_dim = vproc_param->param->p_psr_crop_bg_dim;
	p_pip_crop_enable = (INT *)vproc_param->param->p_psr_crop_enable;

	if (p_crop_rect && p_crop_dim && p_crop_enable && *p_crop_enable) {
		vpe_1_entity->pch_param.crop_bg_dim.w = p_crop_dim->w;
		vpe_1_entity->pch_param.crop_bg_dim.h = p_crop_dim->h;
		vpe_1_entity->pch_param.crop_rect.x = p_crop_rect->x;
		vpe_1_entity->pch_param.crop_rect.y = p_crop_rect->y;
		vpe_1_entity->pch_param.crop_rect.w = p_crop_rect->w;
		vpe_1_entity->pch_param.crop_rect.h = p_crop_rect->h;

		/* If previous unit is vdec, src_rect will be set by in buffer,
		 * If NOT, modify src_rect and src_bg to enable crop function */
		if (!prev_is_vdec) {
			vpe_1_entity->pch_param.src_bg_dim.w = p_crop_dim->w;
			vpe_1_entity->pch_param.src_bg_dim.h = p_crop_dim->h;
			vpe_1_entity->pch_param.src_rect.x = p_crop_rect->x;
			vpe_1_entity->pch_param.src_rect.y = p_crop_rect->y;
			vpe_1_entity->pch_param.src_rect.w = p_crop_rect->w;
			vpe_1_entity->pch_param.src_rect.h = p_crop_rect->h;
		}
	}

	if (p_pip_crop_enable) {
		if (*p_pip_crop_enable && p_pip_crop_rect && p_pip_crop_dim) {
			vpe_1_entity->pch_param.crop_bg_dim2.w = p_pip_crop_dim->w;
			vpe_1_entity->pch_param.crop_bg_dim2.h = p_pip_crop_dim->h;
			vpe_1_entity->pch_param.crop_rect2.x = p_pip_crop_rect->x;
			vpe_1_entity->pch_param.crop_rect2.y = p_pip_crop_rect->y;
			vpe_1_entity->pch_param.crop_rect2.w = p_pip_crop_rect->w;
			vpe_1_entity->pch_param.crop_rect2.h = p_pip_crop_rect->h;
		}
		if ((*p_pip_crop_enable == 0) && p_crop_dim) {
			vpe_1_entity->pch_param.crop_bg_dim2.w = p_crop_dim->w;
			vpe_1_entity->pch_param.crop_bg_dim2.h = p_crop_dim->h;
			vpe_1_entity->pch_param.crop_rect2.x = 0;
			vpe_1_entity->pch_param.crop_rect2.y = 0;
			vpe_1_entity->pch_param.crop_rect2.w = p_crop_dim->w;
			vpe_1_entity->pch_param.crop_rect2.h = p_crop_dim->h;
		}
	}

	GMLIB_L2_P("vproc_crop path_id(%#lx) pip(%d) src_bg(%d,%d) src_rect(%d,%d,%d,%d) crop_bg(%d,%d) crop_rect(%d,%d,%d,%d)\n",
		   dif_get_path_id(&vproc_param->dev),
		   p_crop_enable ? *p_crop_enable : -1,
		   vpe_1_entity->pch_param.src_bg_dim.w, vpe_1_entity->pch_param.src_bg_dim.h,
		   vpe_1_entity->pch_param.src_rect.x, vpe_1_entity->pch_param.src_rect.y,
		   vpe_1_entity->pch_param.src_rect.w, vpe_1_entity->pch_param.src_rect.h,
		   vpe_1_entity->pch_param.crop_bg_dim.w, vpe_1_entity->pch_param.crop_bg_dim.h,
		   vpe_1_entity->pch_param.crop_rect.x, vpe_1_entity->pch_param.crop_rect.y,
		   vpe_1_entity->pch_param.crop_rect.w, vpe_1_entity->pch_param.crop_rect.h);
	GMLIB_L2_P("                         psr(%d) crop_bg2(%d,%d) crop_rect2(%d,%d,%d,%d)\n",
		   p_pip_crop_enable ? *p_pip_crop_enable : -1,
		   vpe_1_entity->pch_param.crop_bg_dim2.w, vpe_1_entity->pch_param.crop_bg_dim2.h,
		   vpe_1_entity->pch_param.crop_rect2.x, vpe_1_entity->pch_param.crop_rect2.y,
		   vpe_1_entity->pch_param.crop_rect2.w, vpe_1_entity->pch_param.crop_rect2.h);

	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT get_vproc_transcode_fps_setting(HD_GROUP *p_group, UINT32 line_idx, INT *p_dec_fps, INT *p_enc_fps)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VENC_PARAM venc;
	DIF_VDEC_PARAM vdec;
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
	if (p_dec_fps == NULL) {
		GMLIB_ERROR_P("NULL p_dec_fps\n");
		goto err_exit;
	}
	if (p_enc_fps == NULL) {
		GMLIB_ERROR_P("NULL p_dec_fps\n");
		goto err_exit;
	}

	*p_dec_fps = *p_enc_fps = 0;
	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) {
			vdec.param = videodec_get_param_p(p_member->p_bind->device.deviceid, p_member->out_subid);
			if (vdec.param && vdec.param->max_mem != NULL) {
				*p_dec_fps = vdec.param->max_mem->max_fps;
			} else {
				GMLIB_ERROR_P("No dec max_fps setting, dec_id(%#x)\n", p_member->p_bind->device.deviceid);
				goto err_exit;
			}
		} else if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
			venc.param = videoenc_get_param_p(p_member->p_bind->device.deviceid);
			if (venc.param == NULL) {
				GMLIB_ERROR_P("transcode, NULL venc.param, enc_id(%#x)\n", p_member->p_bind->device.deviceid);
				goto err_exit;
			}
			if (venc.param->port[p_member->in_subid].p_enc_out_param == NULL) {
				GMLIB_ERROR_P("transcode, NULL venc enc out param, enc_id(%#x)\n", p_member->p_bind->device.deviceid);
				goto err_exit;
			}
			if (venc.param->port[p_member->in_subid].p_enc_out_param->codec_type == HD_CODEC_TYPE_H264 ||
				venc.param->port[p_member->in_subid].p_enc_out_param->codec_type == HD_CODEC_TYPE_H265) {
				if (venc.param->port[p_member->in_subid].p_enc_rc == NULL) {
					GMLIB_ERROR_P("transcode, NULL h26x venc.rc param, enc_id(%#x)\n", p_member->p_bind->device.deviceid);
					goto err_exit;
				}
				goto use_transcode_h26x_fps;
			} else if (venc.param->port[p_member->in_subid].p_enc_out_param->codec_type == HD_CODEC_TYPE_JPEG) {
				if (venc.param->port[p_member->in_subid].p_jpeg_rc == NULL) {
					GMLIB_ERROR_P("transcode, NULL jpeg venc.rc param, enc_id(%#x)\n", p_member->p_bind->device.deviceid);
				}
				goto use_transcode_jpeg_fps;
			} else if (venc.param->port[p_member->in_subid].p_enc_out_param->codec_type == HD_CODEC_TYPE_RAW) {
				if (venc.param->port[p_member->in_subid].p_enc_rc) {
					goto use_transcode_h26x_fps;
				}
				if (venc.param->port[p_member->in_subid].p_jpeg_rc) {
					goto use_transcode_jpeg_fps;
				}
				GMLIB_ERROR_P("transcode, NULL h26x or jpeg venc.rc param, enc_id(%#x)\n", p_member->p_bind->device.deviceid);
				goto err_exit;
			}
use_transcode_h26x_fps:
			p_enc_rc = venc.param->port[p_member->in_subid].p_enc_rc;
			if (p_enc_rc->rc_mode == HD_RC_MODE_CBR) {
				*p_enc_fps = p_enc_rc->cbr.frame_rate_base / p_enc_rc->cbr.frame_rate_incr;
			} else if (p_enc_rc->rc_mode == HD_RC_MODE_VBR) {
				*p_enc_fps = p_enc_rc->vbr.frame_rate_base / p_enc_rc->vbr.frame_rate_incr;
			} else if (p_enc_rc->rc_mode == HD_RC_MODE_FIX_QP) {
				*p_enc_fps = p_enc_rc->fixqp.frame_rate_base / p_enc_rc->fixqp.frame_rate_incr;
			} else if (p_enc_rc->rc_mode == HD_RC_MODE_EVBR) {
				*p_enc_fps = p_enc_rc->evbr.frame_rate_base / p_enc_rc->evbr.frame_rate_incr;
			} else {
				GMLIB_ERROR_P("transcode, No venc.rc mode setting, venc_id(%#x)\n", p_member->p_bind->device.deviceid);
				goto err_exit;
			}
			continue;
use_transcode_jpeg_fps:
			p_jpeg_rc = venc.param->port[p_member->in_subid].p_jpeg_rc;
			*p_enc_fps = p_jpeg_rc->frame_rate_base / p_jpeg_rc->frame_rate_incr;
		}
	}
	if (*p_dec_fps == 0 || *p_enc_fps == 0 || (*p_enc_fps > *p_dec_fps)) {
		GMLIB_ERROR_P("err enc_fps(%d) dec_fps(%d) \n", *p_enc_fps, *p_dec_fps);
		goto err_exit;
	}

	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT get_vproc_fps_setting(HD_GROUP *p_group, UINT32 line_idx, DIF_VPROC_PARAM *vproc_param, INT *p_fps)
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
	if (vproc_param == NULL) {
		GMLIB_ERROR_P("NULL vproc_param\n");
		goto err_exit;
	}
	if (p_fps == NULL) {
		GMLIB_ERROR_P("NULL p_fps\n");
		goto err_exit;
	}

	member_idx = vproc_param->dev.member_idx + 1;
	if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
		GMLIB_ERROR_P("No next unit for vproc\n");
		goto err_exit;
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

exit:
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

/* input p_rect and p_rect2, get the overlap region p_overlap_rect */
static int get_rect_overlap_region(rect_t *p_rect, rect_t *p_rect2, rect_t *p_overlap_rect)
{
	int ret = 0;
	pos_t xy, xy_p;
	pos_t xy2, xy2_p;
	pos_t hole, hole_p;
	rect_t hole_rect = {0};

	if (!p_rect || !p_rect2 || !p_overlap_rect) {
		GMLIB_ERROR_P("Can't get overlap region. rect(%#lx), rect2(%#lx), overlap_rect(%#lx)\n",
				(unsigned long)p_rect, (unsigned long)p_rect2, (unsigned long)p_overlap_rect);
		ret = -1;
		goto exit;
	}

	xy.x = p_rect->x;
	xy.y = p_rect->y;
	xy_p.x = xy.x + p_rect->w;
	xy_p.y = xy.y + p_rect->h;

	xy2.x = p_rect2->x;
	xy2.y = p_rect2->y;
	xy2_p.x = xy2.x + p_rect2->w;
	xy2_p.y = xy2.y + p_rect2->h;

	hole.x = (xy2.x > xy.x) ? xy2.x : xy.x;
	hole.y = (xy2.y > xy.y) ? xy2.y : xy.y;
	hole_p.x = (xy2_p.x > xy_p.x) ? xy_p.x : xy2_p.x;
	hole_p.y = (xy2_p.y > xy_p.y) ? xy_p.y : xy2_p.y;

	hole_rect.x = hole.x;
	hole_rect.y = hole.y;
	hole_rect.w = hole_p.x - hole.x;
	hole_rect.h = hole_p.y - hole.y;

	if (hole_rect.x > 0 && hole_rect.y > 0 && hole_rect.w > 0 && hole_rect.h > 0) {
		*p_overlap_rect = hole_rect;
	} else {
		ret = -1;
	}
exit:
	return ret;
}

INT get_vproc_datain_ddrid(HD_COMMON_MEM_POOL_TYPE pool_type, INT chip_id)
{
	HD_COMMON_MEM_POOL_INFO mem_info = {0};
	int temp_ddr_id = 0;
	int start_ddrid = 0, end_ddrid = 0;

	if (pif_get_ddr_id_by_chip(chip_id, &start_ddrid, &end_ddrid) < 0) {
		return HD_ERR_NOT_SUPPORT;
	}

	mem_info.type = pool_type;
	mem_info.blk_cnt = 0;
	mem_info.blk_size = 0;
	for (temp_ddr_id = start_ddrid; temp_ddr_id <= end_ddrid; temp_ddr_id++) {
		mem_info.ddr_id = temp_ddr_id;
		if (hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, (VOID *)&mem_info) == HD_OK) {
			break;
		}
	}

	if (temp_ddr_id > end_ddrid) {
		printf("dts not config pool_type(%d) size\n", pool_type);
		return 0;
	}
	return temp_ddr_id;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT set_vproc_unit(HD_GROUP *hd_group, INT line_idx, DIF_VPROC_PARAM *vproc_param, INT *len, INT curr_entity_idx)
{
	INT i, size, fps, member_idx, lcd_fps = 0, vcap_fps, is_lcd = 0, is_virtual_lcd = 0;
	INT prev_is_cap = 0, prev_is_dec = 0, prev_is_lcd = 0, next_is_lcd = 0, next_is_enc = 0, first_unit = 0, last_unit = 0;
	INT prev_is_vproc = 0, next_is_vproc = 0, dec_fps, enc_fps;
	UINT frame_rate_base, frame_rate_incr;
	pif_group_t *group;
	vpd_entity_t *vpe, *prev_vpd_entity = NULL;
	vpd_vpe_1_entity_t *vpe_1_entity;
	vpd_rotate_t rotate_mode = VPD_ROTATE_NONE;
	vpd_flip_t flip_mode = VPD_FLIP_NONE;
	INT vpe_fd, vpe_vch, vpe_path, cap_vch = 0, lcd_path, lcd_vch = 0, enc_vch = 0;
	HD_OUT_POOL *p_out_pool;
	HD_DIM in_dim = {0, 0};
	HD_DIM out_dim = {0, 0};
	HD_DIM max_disp_dim = {0, 0};
	HD_VIDEO_PXLFMT in_pxlfmt = HD_VIDEO_PXLFMT_NONE;
	HD_VIDEO_PXLFMT out_pxlfmt = HD_VIDEO_PXLFMT_NONE;
	HD_VIDEO_PXLFMT enc_pxlfmt = HD_VIDEO_PXLFMT_NONE;
	HD_VIDEO_PXLFMT prev_vproc_pxlfmt = HD_VIDEO_PXLFMT_NONE;
	UINT32 vcap_out_subid = 0;
	HD_DIM enc_dim = {0, 0};
	HD_DIM prev_vproc_dim = {0, 0};
	DIF_VPROC_PARAM prev_vproc;
	HD_DIM tmp_in_dim = {0, 0};
	HD_DIM tmp_out_dim = {0, 0};
	INT dup_lcd_vch = -1, dup_disp_width = 0, dup_disp_height = 0;
	HD_VIDEOCAP_OUT cap_out;
	VDODEC_MAXMEM max_mem;
	HD_VIDEOOUT_WIN_ATTR win;
	HD_VIDEOOUT_WIN_PSR_ATTR win_psr;
	HD_MEMBER *p_member;
	HD_PATH_ID path_id;
	VDOPROC_HW_SPEC_INFO hw_spec_info = {0};

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
	if (vproc_param == NULL) {
		GMLIB_ERROR_P("NULL vcap_param\n");
		goto err_exit;
	}
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}
	vpe_vch = vproc_param->dev.device_subid;
	vpe_path = vproc_param->dev.out_subid;

	/* find LCD unit */
	if (dif_get_lcd_ch_path(hd_group, line_idx, &lcd_vch, &lcd_path) == HD_OK) {
		is_lcd = 1;
	}

	/* find previous unit */
	if (vproc_param->dev.member_idx > 0) {
		member_idx = vproc_param->dev.member_idx - 1;
		if (hd_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			GMLIB_ERROR_P("No previous unit for vproc\n");
			goto err_exit;
		}
		p_member = &(hd_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE) {
			cap_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			vcap_out_subid = p_member->out_subid;
			prev_is_cap = 1;
		} else if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) {
			prev_is_dec = 1;
		} else if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			lcd_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			prev_is_lcd = 1;
		} else if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			prev_is_vproc = 1;
			prev_vproc.param = videoproc_get_param_p(p_member->p_bind->device.deviceid, p_member->out_subid);
			if (prev_vproc.param && (prev_vproc.param->p_dst_rect != NULL)) {
				prev_vproc_dim.w = prev_vproc.param->p_dst_rect->w;
				prev_vproc_dim.h = prev_vproc.param->p_dst_rect->h;
			} else {
				GMLIB_ERROR_P("first vproc dim is not set! line_idx(%d)\n", line_idx);
				goto err_exit;
			}
			if (prev_vproc.param && (prev_vproc.param->p_dst_fmt != NULL)) {
				prev_vproc_pxlfmt = *prev_vproc.param->p_dst_fmt;
			} else {
				GMLIB_ERROR_P("first vproc p_dst_fmt is not set! line_idx(%d)\n", line_idx);
				goto err_exit;
			}
		}

	} else {
		first_unit = 1;
	}

	/* find next unit */
	if (vproc_param->dev.member_idx < (HD_MAX_BIND_NUNBER_PER_LINE - 1)) {
		member_idx = vproc_param->dev.member_idx + 1;
		if (hd_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			GMLIB_ERROR_P("No next unit for vproc\n");
			goto err_exit;
		}
		p_member = &(hd_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			lcd_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			next_is_lcd = 1;
		} else if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
			enc_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			next_is_enc = 1;
			if (dif_get_venc_dim_fmt(hd_group, line_idx, &enc_dim, &enc_pxlfmt) != HD_OK) {
				GMLIB_ERROR_P("<venc dim is not set! line_idx(%d) >\n", line_idx);
				goto err_exit;
			}
		} else if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			next_is_vproc = 1;
		}
	} else {
		last_unit = 1;
	}

	vpe_fd = utl_get_vpe_entity_fd(vpe_vch,	vpe_path, (VOID *)0);
	p_out_pool = vproc_param->param->p_out_pool;
	if (p_out_pool == NULL) {
		GMLIB_ERROR_P("<vproc out pool is not set! device_subid(%d) >\n", vpe_vch);
		goto err_exit;
	}

	if (vproc_param->param->p_dst_bg_dim && vproc_param->param->p_dst_fmt) {
		tmp_out_dim.w = vproc_param->param->p_dst_bg_dim->w;
		tmp_out_dim.h = vproc_param->param->p_dst_bg_dim->h;
		out_pxlfmt = *vproc_param->param->p_dst_fmt;
		if (dif_get_align_up_dim(tmp_out_dim, out_pxlfmt, &out_dim) != HD_OK) {
			GMLIB_ERROR_P("<dif_get_align_up_dim fail 1>\n");
			goto err_exit;
		}
	} else if (next_is_enc) {
		tmp_out_dim.w = enc_dim.w;
		tmp_out_dim.h = enc_dim.h;
		out_pxlfmt = enc_pxlfmt;
		if (dif_get_align_up_dim(tmp_out_dim, out_pxlfmt, &out_dim) != HD_OK) {
			GMLIB_ERROR_P("<dif_get_align_up_dim fail 2>\n");
			goto err_exit;
		}
	} else {
		GMLIB_ERROR_P("<vproc out dim and pxlfmt is not set! device_subid(%d) >\n", vpe_vch);
		goto err_exit;
	}

	if (prev_is_cap && (dif_get_vcap_out(hd_group, line_idx, &cap_out) != HD_OK)) {
		GMLIB_ERROR_P("<vcap out param is not set! line_idx(%d) >\n", line_idx);
		goto err_exit;
	}
	if (prev_is_dec && (dif_get_vdec_max_mem(hd_group, line_idx, &max_mem) != HD_OK)) {
		GMLIB_ERROR_P("<vdec max_mem param is not set! line_idx(%d) >\n", line_idx);
		goto err_exit;
	}

	if (prev_is_cap && (platform_sys_Info.cap_info[cap_vch].vcapch < 0)) {
		GMLIB_ERROR_P("<cap: vch(%d) capch(%d) error!>\n", cap_vch, platform_sys_Info.cap_info[cap_vch].vcapch);
		goto err_exit;
	}
	if (next_is_lcd && (platform_sys_Info.lcd_info[lcd_vch].lcdid < 0)) {
		dif_get_lcd_output_type(hd_group, line_idx, &is_virtual_lcd);
		if (is_virtual_lcd == 0) {
			GMLIB_ERROR_P("<lcd: vch(%d) id(%d) error!>\n", lcd_vch, platform_sys_Info.lcd_info[lcd_vch].lcdid);
			goto err_exit;
		}
	}

	if (prev_is_cap) {
		BOOL need_link_di = FALSE, need_link_vpe = FALSE;
		if (dif_get_cap_vpe_link_requirement(vproc_param->param, &cap_out, &need_link_di, &need_link_vpe) != HD_OK) {
			GMLIB_ERROR_P("Get link requirement fail, param(%p)\n", vproc_param->param);
			goto err_exit;
		}

		if (need_link_di) {
			// if in/out the same size --> bind di only
			// if in/out different size --> bind di + vpe
			if (!next_is_lcd && !need_link_vpe) {
				GMLIB_L2_P("cap: vch(%d) capch(%d), Use DI(vpe316) only (Not bind VPE)\n",
					   cap_vch, platform_sys_Info.cap_info[cap_vch].vcapch);
				*len = 0;
				goto not_bind_exit;
			} else {
				prev_vpd_entity = dif_get_vpd_entity(hd_group, line_idx, curr_entity_idx - 1);
				if (prev_vpd_entity == NULL) {
					GMLIB_ERROR_P("Can't find prev entity\n");
					goto err_exit;
				}
				if (prev_vpd_entity->type == VPD_DI_ENTITY_TYPE) {
					if (!prev_vpd_entity->e) {
						GMLIB_ERROR_P("Check VPD_DI_ENTITY_TYPE entity fail\n");
						goto err_exit;
					}
					vpd_di_entity_t *di_entity = (vpd_di_entity_t *)prev_vpd_entity->e;
					in_pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(di_entity->dst_fmt);
					in_dim.w = di_entity->dst_dim.w;
					in_dim.h = di_entity->dst_dim.h;
				} else {
					GMLIB_ERROR_P("Previous DI entity is not added after VCAP.\n");
					goto err_exit;
				}
			}
		} else if (next_is_enc && !need_link_vpe) {
			GMLIB_L2_P("no need to link di and vpe");
			goto not_bind_exit;
		} else {
			in_pxlfmt = cap_out.pxlfmt;
			tmp_in_dim.w = cap_out.dim.w;
			tmp_in_dim.h = cap_out.dim.h;
			if (dif_get_align_up_dim(tmp_in_dim, in_pxlfmt, &in_dim) != HD_OK) {
				GMLIB_ERROR_P("<dif_get_align_up_dim fail 3>\n");
				goto err_exit;
			}
		}
	} else if (prev_is_dec) {
		// find previous vpd_entity (it should be vdec)
		prev_vpd_entity = dif_get_vpd_entity(hd_group, line_idx, curr_entity_idx - 1);
		if (prev_vpd_entity == NULL) {
			GMLIB_ERROR_P("<dif_get_vpd_entity fail >\n");
			goto err_exit;
		}
		in_pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(prev_vpd_entity->pool[0].dst_fmt);
		tmp_in_dim.w = max_mem.max_width;
		tmp_in_dim.h = max_mem.max_height;
		if (dif_get_align_up_dim(tmp_in_dim, in_pxlfmt, &in_dim) != HD_OK) {
			GMLIB_ERROR_P("<dif_get_align_up_dim fail 4>\n");
			goto err_exit;
		}
	} else if (prev_is_vproc) {
		tmp_in_dim.w = prev_vproc_dim.w;
		tmp_in_dim.h = prev_vproc_dim.h;
		in_pxlfmt = prev_vproc_pxlfmt;
		if (dif_get_align_up_dim(tmp_in_dim, in_pxlfmt, &in_dim) != HD_OK) {
			GMLIB_ERROR_P("<dif_get_bg_dim fail, prov_vproc>\n");
			goto err_exit;
		}
	} else if (first_unit) {
		if (next_is_enc) {
			BOOL need_link_vpe = FALSE;
			if (dif_get_vpe_link_requirement(vproc_param->param, &need_link_vpe) != HD_OK) {
				GMLIB_ERROR_P("Get link requirement fail, param(%p)\n", vproc_param->param);
				goto err_exit;
			}
			if (!need_link_vpe) {
				goto not_bind_exit;
			}
		}

		if (vproc_param->param->p_src_bg_dim && vproc_param->param->p_src_fmt) {
			tmp_in_dim.w = vproc_param->param->p_src_bg_dim->w;
			tmp_in_dim.h = vproc_param->param->p_src_bg_dim->h;
			in_pxlfmt = *vproc_param->param->p_src_fmt;
			if (dif_get_align_up_dim(tmp_in_dim, in_pxlfmt, &in_dim) != HD_OK) {
				GMLIB_ERROR_P("<dif_get_align_up_dim fail 5>\n");
				goto err_exit;
			}
		} else {
			GMLIB_ERROR_P("<vproc in param is not set! line_idx(%d) >\n", line_idx);
			goto err_exit;
		}
	} else if (next_is_enc && prev_is_lcd) {//display to encode case
		lcd_fps = pif_cal_disp_tick_fps(lcd_vch);
		tmp_in_dim.w = platform_sys_Info.lcd_info[lcd_vch].fb0_win.width;
		tmp_in_dim.h = platform_sys_Info.lcd_info[lcd_vch].fb0_win.height;
		in_pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(platform_sys_Info.lcd_info[lcd_vch].lcd_fmt);
		if (dif_get_align_up_dim(tmp_in_dim, in_pxlfmt, &in_dim) != HD_OK) {
			GMLIB_ERROR_P("<dif_get_align_up_dim fail 6>\n");
			goto err_exit;
		}
	} else if (last_unit) {
	} else {
		GMLIB_ERROR_P("<vproc: Not support this case! line_idx(%d) >\n", line_idx);
		goto err_exit;
	}
	path_id = dif_get_path_id(&vproc_param->dev);
	if (videoproc_check_is_needed_param_set_p(path_id, in_pxlfmt, out_pxlfmt, &in_dim) != HD_OK) {
		goto err_exit;
	}

	if (get_vproc_fps_setting(hd_group, line_idx, vproc_param, &fps) != HD_OK) {
		GMLIB_ERROR_P("<vproc: Error to set fps!>\n");
		goto err_exit;
	}

	vpe = pif_lookup_entity_buf(VPD_VPE_1_ENTITY_TYPE, vpe_fd, group->encap_buf);
	if (vpe != NULL) {
		*len = 0;
		goto exit;
	}
	*len = pif_set_EntityBuf(VPD_VPE_1_ENTITY_TYPE, &vpe, group->encap_buf);
	if (vpe == NULL) {
		GMLIB_ERROR_P("<vproc: NULL entity buffer!>\n");
		goto err_exit;
	}
	vpe->sn = vpe_fd;
	vpe->ap_bindfd = BD_SET_PATH(vproc_param->dev.device_baseid + vproc_param->dev.device_subid,
				     vproc_param->dev.in_subid + HD_IN_BASE,
				     vproc_param->dev.out_subid + HD_OUT_BASE);

	if (get_vproc_rotation(vproc_param, &rotate_mode, &flip_mode) != HD_OK) {
		GMLIB_ERROR_P("<vcap: get_vproc_rotation fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}

	memset(vpe->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < VPD_MAX_BUFFER_NUM; i++) {
		if (p_out_pool->buf_info[i].enable == HD_BUF_DISABLE) {
			vpe->pool[i].enable = FALSE;
			continue;
		} else if (p_out_pool->buf_info[i].enable == HD_BUF_ENABLE) {
			vpe->pool[i].enable = TRUE;
			vpe->pool[i].ddr_id = p_out_pool->buf_info[i].ddr_id;
			vpe->pool[i].count_decuple = p_out_pool->buf_info[i].counts;
			vpe->pool[i].max_counts = get_vproc_max_counts(p_out_pool->buf_info[i].counts);
			vpe->pool[i].min_counts = VPROC_MIN_COUNT;
			if (next_is_lcd) {
				if (vpe->pool[i].max_counts < VPROC_MAX_COUNT_FOR_VOUT) {
					GMLIB_ERROR_P("set_vproc_pool fail (%d), counts(%lu), max_count(%lu) at least %u for vout\n",
						      i, p_out_pool->buf_info[i].counts, vpe->pool[i].max_counts, VPROC_MAX_COUNT_FOR_VOUT);
					goto err_exit;
				}
			} else {
				if (vpe->pool[i].max_counts < VPROC_MAX_COUNT_FOR_OTHER) {
					GMLIB_ERROR_P("set_vproc_pool fail (%d), counts(%lu), max_count(%lu) at least %u\n",
						      i, p_out_pool->buf_info[i].counts, vpe->pool[i].max_counts, VPROC_MAX_COUNT_FOR_OTHER);
					goto err_exit;
				}
			}
		} else {  //HD_BUF_AUTO
			if (i == 0) {
                //If not set vpe out buf count and default count is different with previous out count, clearwin will lose buf to clear
                GMLIB_PRINT_P("Please set HD_VIDEOPROC_PARAM_DEV_CONFIG first,otherwise videoprocess out buf count=3,if previous out count!=3,clearwin will lose buf to clearwin\n");
				vpe->pool[i].enable = TRUE;
				vpe->pool[i].ddr_id = 0;
				vpe->pool[i].count_decuple = 30;
				if (next_is_lcd) {
					vpe->pool[i].max_counts = VPROC_MAX_COUNT_FOR_VOUT;
				} else if (next_is_vproc) {
					vpe->pool[i].max_counts = VPROC_MAX_COUNT_FOR_VPROC;
				} else {
					vpe->pool[i].max_counts = VPROC_MAX_COUNT_FOR_OTHER;
				}
				vpe->pool[i].min_counts = VPROC_MIN_COUNT;
			} else {
				vpe->pool[i].enable = FALSE;
			}
		}
		size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(out_dim, out_pxlfmt);
		if (next_is_lcd) {
			vpe->pool[i].type = VPD_DISP0_IN_POOL + lcd_vch;
			vpe->pool[i].vch = lcd_vch;
			snprintf(vpe->pool[i].name, GM_POOL_NAME_LEN, "%s", pif_get_pool_name_by_type(vpe->pool[i].type));
		} else if (next_is_enc) {
			vpe->pool[i].type = VPD_ENC_SCL_OUT_POOL;
			vpe->pool[i].vch = enc_vch;
			snprintf(vpe->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x", pif_get_pool_name_by_type(vpe->pool[i].type), vpe_fd);
		} else if (prev_is_dec && next_is_vproc) {
			/* for dec and vpe_multi_path */
			vpe->pool[i].type = VPD_DISP_DEC_OUT_POOL;
			vpe->pool[i].vch = lcd_vch;
			snprintf(vpe->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x", pif_get_pool_name_by_type(vpe->pool[i].type), vpe_fd);
		} else if (prev_is_cap && next_is_vproc && is_lcd) {
			/* for cvbs_lv and enc_substream, vpe_multi_path */
			vpe->pool[i].type = VPD_DISP0_CAP_OUT_POOL + lcd_vch;
			vpe->pool[i].vch = lcd_vch;
			snprintf(vpe->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x", pif_get_pool_name_by_type(vpe->pool[i].type), vpe_fd);
		} else if (prev_is_vproc && next_is_vproc) {
			vpe->pool[i].type = VPD_DISP_DEC_OUT_POOL;
			vpe->pool[i].vch = lcd_vch;
			snprintf(vpe->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x", pif_get_pool_name_by_type(vpe->pool[i].type), vpe_fd);
		} else {
			GMLIB_ERROR_P("<vproc: Not support next unit is not VOUT nor VENC!>\n");
			goto err_exit;
		}
		vpe->pool[i].width = out_dim.w;
		vpe->pool[i].height = out_dim.h;
		if (prev_is_dec && next_is_enc) {  // transcode
			if (get_vproc_transcode_fps_setting(hd_group, line_idx, &dec_fps, &enc_fps) == HD_OK) {
				vpe->pool[i].fps = enc_fps;
				vpe->pool[i].gs_flow_rate = (enc_fps < dec_fps) ? PIF_SET_FPS_RATIO(enc_fps, dec_fps) : PIF_SET_FPS_RATIO(1, 1);
			} else {
				vpe->pool[i].fps = 30;
				vpe->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
				GMLIB_ERROR_P("vproc: get transcode fps err!\n"); // we only show the wanning message
			}
		} else if (prev_is_lcd && next_is_enc) {  // disp_enc
			vpe->pool[i].fps = fps;
			if (dif_get_venc_rc_stuff(hd_group, line_idx, &frame_rate_base, &frame_rate_incr) == HD_OK) {
				// example:
				// 30fps, frame_rate_base = 30, frame_rate_incr = 1, lcd_fps = 30; gs_flow_rate = (30, 1 * 30)
				// 1/2fps, frame_rate_base = 1, frame_rate_incr = 2, lcd_fps = 30; gs_flow_rate = (1, 2 * 30)
				vpe->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(frame_rate_base, frame_rate_incr * lcd_fps);
			} else {
				vpe->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
			}
		} else if (prev_is_cap && next_is_enc) {  // multi_path enc
			/* vcap ---> vpe0_0 +--> venc0
			 *               _1 +--> venc1
			 * NOTE: multi_path, we choose p_cap_out->frc
			 */
			INT denominator = (cap_out.frc & 0x0000FFFF);
			INT numerator   = (cap_out.frc & 0xFFFF0000) >> 16;

			if (denominator == 0) {
				GMLIB_ERROR_P("vcap out param cap_out.frc(%#x) is not correct, device_subid(%d) out_subid(%d)\n",
					      cap_out.frc, cap_vch, (INT)vcap_out_subid);
				vcap_fps = platform_sys_Info.cap_info[cap_vch].fps;
			} else {
				vcap_fps = (numerator * platform_sys_Info.cap_info[cap_vch].fps) / denominator;
				if (vcap_fps < 1 || vcap_fps > platform_sys_Info.cap_info[cap_vch].fps) {
					GMLIB_ERROR_P("vcap out param cap_out.frc(%#x) is not correct, device_subid(%d) out_subid(%d)\n",
						      cap_out.frc, cap_vch, (INT)vcap_out_subid);
					vcap_fps = platform_sys_Info.cap_info[cap_vch].fps;
				}
			}
			if (fps > vcap_fps) {
				GMLIB_ERROR_P("enc fps(%d) must be smaller than cap fps(%d).\n", fps, vcap_fps);
				fps = vcap_fps;
			}

			vpe->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(fps, vcap_fps);
			vpe->pool[i].fps = fps;
		} else if (prev_is_cap && next_is_vproc && is_lcd && lcd_vch != 0) {
			/* vcap ---> vpe0_0 +-->
			 *           vpe0_1 +--> vpe1_0 --> cvbs_lcd
			 * NOTE: multi_path, we choose p_cap_out->frc
			 */
			INT denominator = (cap_out.frc & 0x0000FFFF);
			INT numerator   = (cap_out.frc & 0xFFFF0000) >> 16;

			if (denominator == 0) {
				//GMLIB_ERROR_P("vcap out param cap_out.frc(%#x) is not correct, device_subid(%d) out_subid(%d)\n",
				//			   cap_out.frc, cap_vch, (INT)vcap_out_subid);
				vcap_fps = platform_sys_Info.cap_info[cap_vch].fps;
			} else {
				vcap_fps = (numerator * platform_sys_Info.cap_info[cap_vch].fps) / denominator;
				if (vcap_fps < 1 || vcap_fps > platform_sys_Info.cap_info[cap_vch].fps) {
					GMLIB_ERROR_P("vcap out param cap_out.frc(%#x) is not correct, device_subid(%d) out_subid(%d)\n",
						      cap_out.frc, cap_vch, (INT)vcap_out_subid);
					vcap_fps = platform_sys_Info.cap_info[cap_vch].fps;
				}
			}
			lcd_fps = pif_cal_disp_tick_fps(lcd_vch);
			if (lcd_fps < vcap_fps) {
				vpe->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(lcd_fps, vcap_fps);
			} else {
				vpe->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
			}
			vpe->pool[i].fps = lcd_fps;
		} else {
			vpe->pool[i].fps = fps;
			vpe->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		}
		vpe->pool[i].is_duplicate = 0;
		vpe->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(out_pxlfmt);
		vpe->pool[i].size = size;
		vpe->pool[i].win_size = 0;

		GMLIB_L2_P("set_vproc_pool i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) "
			   "fmt(%#x) dup(%d) gs_flow_rate(%#x)\n",
			   i, vpe->pool[i].enable, vpe->pool[i].name, vpe->pool[i].type, vpe->pool[i].ddr_id, vpe->pool[i].size,
			   vpe->pool[i].count_decuple, vpe->pool[i].max_counts, vpe->pool[i].dst_fmt, vpe->pool[i].is_duplicate,
			   vpe->pool[i].gs_flow_rate);
	}

	if (next_is_lcd && !is_virtual_lcd) {
		dup_lcd_vch = pif_get_duplicate_lcd_vch(group);
		if (dup_lcd_vch > 0) {
			dup_disp_width = platform_sys_Info.lcd_info[dup_lcd_vch].fb0_win.width;
			dup_disp_height = platform_sys_Info.lcd_info[dup_lcd_vch].fb0_win.height;
		}
	}

	hw_spec_info.in_direction = *vproc_param->param->p_direction;
	hw_spec_info.in_src_fmt = in_pxlfmt;
	hw_spec_info.in_dst_fmt = out_pxlfmt;
	if (videoproc_get_vpe_align_p(&hw_spec_info, 1) != HD_OK) {
		GMLIB_ERROR_P("path_id(%#lx) get hardware spec info fail.\n", path_id);
		goto err_exit;
	}

	vpe_1_entity = (vpd_vpe_1_entity_t *)vpe->e;
	vpe_1_entity->pch_param.pch = 0;
	if (!prev_is_dec) { // em_vpe should use buf prop in playback case
		vpe_1_entity->pch_param.src_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(in_pxlfmt);
	}
	vpe_1_entity->pch_param.dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(out_pxlfmt);

	if (next_is_lcd) {
		if (is_virtual_lcd && vproc_param->param->p_dst_bg_dim) {
			max_disp_dim.w = vproc_param->param->p_dst_bg_dim->w;
			max_disp_dim.h = vproc_param->param->p_dst_bg_dim->h;
		} else {
			max_disp_dim.w = platform_sys_Info.lcd_info[lcd_vch].fb0_win.width;
			max_disp_dim.h = platform_sys_Info.lcd_info[lcd_vch].fb0_win.height;
			if (IS_ALIGNX(hw_spec_info.out_dst_bg_dim.w, max_disp_dim.w) == 0 ||
				IS_ALIGNX(hw_spec_info.out_dst_bg_dim.h, max_disp_dim.h) == 0) {
				GMLIB_ERROR_P("LV:Disp dim. doesn't conform alignment. wh(%lu,%lu) align(%u,%u)\n",
							  max_disp_dim.w, max_disp_dim.h, hw_spec_info.out_dst_bg_dim.w, hw_spec_info.out_dst_bg_dim.h);
			}
		}
	}

	/* VCAP will provide src_rect from buffer info, the others is needed to be set. */
	if (prev_is_cap) {
		vpe_1_entity->pch_param.src_bg_dim.w = in_dim.w;
		vpe_1_entity->pch_param.src_bg_dim.h = in_dim.h;

	/* VDEC will provide src_rect from buffer info, no need to set. */
	} else if (prev_is_dec) {
		/* VPE Support 4K output without column mode */
		if (vproc_param->param->p_slice_mode && vproc_param->param->p_slice_mode->enable) {
			vpe_1_entity->pch_param.slice_height = vproc_param->param->p_slice_mode->slice_height;
		}

	/* The others is needed to be set. */
	} else {
		vpe_1_entity->pch_param.src_rect.x = 0;
		vpe_1_entity->pch_param.src_rect.y = 0;
		vpe_1_entity->pch_param.src_rect.w = in_dim.w;
		vpe_1_entity->pch_param.src_rect.h = in_dim.h;
		vpe_1_entity->pch_param.src_bg_dim.w = in_dim.w;
		vpe_1_entity->pch_param.src_bg_dim.h = in_dim.h;
	}

#if 0
	//check vpe output size from proc and input size > 2048
	if (disp_info->vpe_win_rect.w > platform_sys_Info.vpe_out_linebuf_sz || ALIGN64_DOWN(disp_info->cap_dim.w) > 2048) {
		vpe_1_entity->pch_param.src_x_offset = 384;
	}
#endif

	if (get_vproc_crop_setting(vproc_param, vpe_1_entity, prev_is_dec) != HD_OK) {
		GMLIB_ERROR_P("<vprc: get_vproc_crop_setting fail! line_idx(%d)>\n", line_idx);
		goto err_exit;
	}

	vpe_1_entity->pch_param.buf_offset = GMLIB_NULL_VALUE; //disp_info->lv_buf_offset;
	vpe_1_entity->pch_param.buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(in_dim, in_pxlfmt);
	vpe_1_entity->pch_param.rotate_mode = rotate_mode;
	vpe_1_entity->pch_param.flip_mode = flip_mode;

	if (platform_sys_Info.lcd_info[lcd_vch].channel_zero) {
		GMLIB_ERROR_P("Not support only liveview when channel zero set for LCD%d\n", lcd_vch);
		goto err_exit;
	} else if (next_is_lcd) {
		if (dif_get_vout_win_attr(hd_group, line_idx, &win) == HD_OK) {
			if (win.visible == 1) {
				HD_URECT out_rect;
				HD_RESULT ret1;
				ret1 = videoproc_set_vproc_align_down_dim(path_id, in_pxlfmt, out_pxlfmt, win.rect, &out_rect);
				if (ret1 != HD_OK) {
					GMLIB_ERROR_P("videoproc_set_vproc_align_down_dim wrong\n");
					goto err_exit;
				}
				vpe_1_entity->pch_param.dst_rect.x = out_rect.x;
				vpe_1_entity->pch_param.dst_rect.y = out_rect.y;
				vpe_1_entity->pch_param.dst_rect.w = out_rect.w;
				vpe_1_entity->pch_param.dst_rect.h = out_rect.h;
			} else {
				vpe_1_entity->pch_param.dst_rect.x = 0;
				vpe_1_entity->pch_param.dst_rect.y = 0;
				vpe_1_entity->pch_param.dst_rect.w = hw_spec_info.out_dst_min_dim.w;
				vpe_1_entity->pch_param.dst_rect.h = hw_spec_info.out_dst_min_dim.h;
			}
			vpe_1_entity->pch_param.draw_sequence = get_gm_win_layer_from_hdal_layer(win.layer);
		} else {
			GMLIB_ERROR_P("<vprc: dif_get_vout_win_attr fail! line_idx(%d)>\n", line_idx);
			goto err_exit;
		}
		vpe_1_entity->pch_param.dst_bg_dim.w = max_disp_dim.w;
		vpe_1_entity->pch_param.dst_bg_dim.h = max_disp_dim.h;
		if (dup_lcd_vch > 0 && dup_disp_width > 0 && dup_disp_height > 0) {
			vpe_1_entity->pch_param.dup_dst_bg_dim.w = dup_disp_width;
			vpe_1_entity->pch_param.dup_dst_bg_dim.h = dup_disp_height;
			vpe_1_entity->pch_param.dup_src_region = platform_sys_Info.lcd_info[dup_lcd_vch].src_duplicate_region;
		}
		if (dif_get_vout_win_psr_attr(hd_group, line_idx, &win_psr) == HD_OK) {
			if (win_psr.visible) {
				int ret_val = 0;
				vpe_1_entity->pch_param.dst_rect2.x = ALIGNX_DOWN(hw_spec_info.out_dst_rect.x, win_psr.rect.x);
				vpe_1_entity->pch_param.dst_rect2.y = ALIGNX_DOWN(hw_spec_info.out_dst_rect.y, win_psr.rect.y);
				vpe_1_entity->pch_param.dst_rect2.w = ALIGNX_DOWN(hw_spec_info.out_dst_rect.w, win_psr.rect.w);
				vpe_1_entity->pch_param.dst_rect2.h = ALIGNX_DOWN(hw_spec_info.out_dst_rect.h, win_psr.rect.h);
				//calculate vpe_1_entity->pch_param.hole
				ret_val = get_rect_overlap_region(&vpe_1_entity->pch_param.dst_rect,
						&vpe_1_entity->pch_param.dst_rect2, &vpe_1_entity->pch_param.hole);
				if (ret_val < 0) {
					vpe_1_entity->pch_param.hole = vpe_1_entity->pch_param.dst_rect2;
				}
			}
		}
	} else if (next_is_enc) {
		vpe_1_entity->pch_param.dst_rect.x = 0;
		vpe_1_entity->pch_param.dst_rect.y = 0;
		vpe_1_entity->pch_param.dst_rect.w = ALIGNX_DOWN(hw_spec_info.out_dst_rect.w, out_dim.w);
		vpe_1_entity->pch_param.dst_rect.h = ALIGNX_DOWN(hw_spec_info.out_dst_rect.h, out_dim.h);
		vpe_1_entity->pch_param.dst_bg_dim.w = ALIGNX_UP(hw_spec_info.out_dst_bg_dim.w, out_dim.w);
		vpe_1_entity->pch_param.dst_bg_dim.h = ALIGNX_UP(hw_spec_info.out_dst_bg_dim.h, out_dim.h);
	} else if (next_is_vproc) {
		vpe_1_entity->pch_param.dst_rect.x = vproc_param->param->p_dst_rect->x;
		vpe_1_entity->pch_param.dst_rect.y = vproc_param->param->p_dst_rect->y;
		vpe_1_entity->pch_param.dst_rect.w = vproc_param->param->p_dst_rect->w;
		vpe_1_entity->pch_param.dst_rect.h = vproc_param->param->p_dst_rect->h;
		vpe_1_entity->pch_param.dst_bg_dim.w = ALIGNX_UP(hw_spec_info.out_dst_bg_dim.w, out_dim.w);
		vpe_1_entity->pch_param.dst_bg_dim.h = ALIGNX_UP(hw_spec_info.out_dst_bg_dim.h, out_dim.h);
	}

	if (pif_check_dst_property_value(vpe_1_entity->pch_param.dst_rect,
					 vpe_1_entity->pch_param.dst_bg_dim) == -1) {
		GMLIB_ERROR_P("Please update win_attr.rect for new lcd input resolution(%dx%d)!\n",
			      vpe_1_entity->pch_param.dst_bg_dim.w,
			      vpe_1_entity->pch_param.dst_bg_dim.h);
		goto err_exit;
	}
	vpe_1_entity->pch_param.is_combined_job = 1;//default
	//liveview needn't set disp_in buffer count
	if (next_is_lcd) {
		if (win.visible == 1) {
			SET_ENTITY_FEATURE(vpe->feature, VPD_FLAG_ENODE_ENABLE);
		} else {
			SET_ENTITY_FEATURE(vpe->feature, VPD_FLAG_DIR_CB);
		}
		if (!platform_sys_Info.low_latency_mode) {
			SET_ENTITY_FEATURE(vpe->feature, VPD_FLAG_FLOW_BY_TICK | VPD_FLAG_ONE_BY_TICK | VPD_FLAG_GROUP);
		}
		SET_ENTITY_FEATURE(vpe->feature, VPD_FLAG_DONT_CARE_CB_FAIL | VPD_FLAG_OUTBUF_MAX_CNT | VPD_FLAG_NEED_INBUF | VPD_FLAG_HIGH_PRIORITY);
		vpe->group_id = ((DISP_VPE << 24) | ((group->groupidx & 0xff) << 16) | (lcd_vch & 0xffff));
		vpe_1_entity->pch_param.is_combined_job = 0;//lcd n to 1 with pattern cannot be combined.
	} else if (prev_is_dec && next_is_enc) {  // transcode case
		SET_ENTITY_FEATURE(vpe->feature, VPD_FLAG_NON_BLOCK | VPD_FLAG_JOB_SEQUENCE_FIRST | VPD_FLAG_HIGH_PRIORITY);
	} else if (next_is_vproc) { // playback 2 vpe case: first_vpe only do scalling
		SET_ENTITY_FEATURE(vpe->feature, VPD_FLAG_OUTBUF_MAX_CNT | VPD_FLAG_GROUP); // only do scalling
		vpe->group_id = GROUP_FROM_IN_BUF;
	} else if (next_is_enc) {
		SET_ENTITY_FEATURE(vpe->feature, VPD_FLAG_NON_BLOCK | VPD_FLAG_JOB_SEQUENCE_FIRST | VPD_FLAG_GROUP | VPD_FLAG_OUTBUF_MAX_CNT);
		vpe->group_id = GROUP_FROM_IN_BUF;
	} else {
		SET_ENTITY_FEATURE(vpe->feature, VPD_FLAG_JOB_SEQUENCE_FIRST | VPD_FLAG_OUTBUF_MAX_CNT);
	}

	// if dewarp is enabled, vpe can't be combined.
	if (vproc_param->param->dc_enable) {
		vpe_1_entity->pch_param.is_combined_job = 0;
	}

	vpe_1_entity->pch_count = 1;
	vpe_1_entity->engine_nu = 0;  //only pb_vpe use "engine_nu = 1", others 0
	vpe_1_entity->pch_param.sub_ratio_thld = vproc_param->param->sub_ratio_thld;

	GMLIB_L2_P("set_vpe_entity path_id(%#lx) fd(%#x) pch(%d) src_bg(%d,%d) src_rect(%d,%d,%d,%d) crop_bg(%d,%d) crop_rect(%d,%d,%d,%d) src_fmt(%#x)\n",
		   dif_get_path_id(&vproc_param->dev), vpe_fd, vpe_1_entity->pch_param.pch,
		   vpe_1_entity->pch_param.src_bg_dim.w, vpe_1_entity->pch_param.src_bg_dim.h,
		   vpe_1_entity->pch_param.src_rect.x, vpe_1_entity->pch_param.src_rect.y,
		   vpe_1_entity->pch_param.src_rect.w, vpe_1_entity->pch_param.src_rect.h,
		   vpe_1_entity->pch_param.crop_bg_dim.w, vpe_1_entity->pch_param.crop_bg_dim.h,
		   vpe_1_entity->pch_param.crop_rect.x, vpe_1_entity->pch_param.crop_rect.y,
		   vpe_1_entity->pch_param.crop_rect.w, vpe_1_entity->pch_param.crop_rect.h,
		   vpe_1_entity->pch_param.src_fmt);

	GMLIB_L2_P("set_vpe_entity dst_bg(%d,%d) dst_rect(%d,%d,%d,%d) dst_rect2(%d,%d,%d,%d) dst_fmt(%#x) draw_seq(%d) sub_ratio_thld(%d)\n",
		   vpe_1_entity->pch_param.dst_bg_dim.w, vpe_1_entity->pch_param.dst_bg_dim.h,
		   vpe_1_entity->pch_param.dst_rect.x, vpe_1_entity->pch_param.dst_rect.y,
		   vpe_1_entity->pch_param.dst_rect.w, vpe_1_entity->pch_param.dst_rect.h,
		   vpe_1_entity->pch_param.dst_rect2.x, vpe_1_entity->pch_param.dst_rect2.y,
		   vpe_1_entity->pch_param.dst_rect2.w, vpe_1_entity->pch_param.dst_rect2.h,
		   vpe_1_entity->pch_param.dst_fmt,
		   vpe_1_entity->pch_param.draw_sequence,
		   vpe_1_entity->pch_param.sub_ratio_thld);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, vpe);

not_bind_exit:
	return 0;

err_exit:
	return -1;
}


/*
 *  for datain -> multi_vpe, because of multi path_id, we choose a dedicate path_id
 */
static HD_PATH_ID get_vproc_dedicated_path_id(DIF_DEV *dev)
{
	HD_PATH_ID path_id = 0;
	INT32 i, max_out, min_out = -1;

	max_out = get_bind_out_nr(dev->p_bind);
	for (i = 0; i < max_out; i++) {
		if (dev->p_bind->out[i].port_state == HD_PORT_START) {
			min_out = i; // lookup min_out number as a dedicated port.
			break;
		}
	}
	if (min_out == -1) {
		GMLIB_ERROR_P("%s fail! path is not start.\n");
		goto ext;
	}

	path_id = ((((dev->device_baseid | dev->device_subid) & 0xffff) << 16) | \
		(((dev->in_subid + HD_IN_BASE) & 0xff) << 8) | \
		(((min_out + HD_OUT_BASE) & 0xff)));

ext:
	GMLIB_L2_P("%s: path_id(%#x)", path_id);
	return path_id;
}


INT set_vproc_datain_unit(HD_GROUP *hd_group, INT line_idx, DIF_VPROC_PARAM *vproc_param, INT *len)
{
	INT i = 0, fps = 0;
	pif_group_t *group;
	vpd_entity_t *din = NULL;
	vpd_din_entity_t *din_entity;
	INT din_fd, vproc_vch, vproc_path;
	HD_OUT_POOL *p_out_pool;
	HD_DIM src_bg_dim = {0, 0}, src_dim = {0, 0};
	HD_VIDEO_PXLFMT pxlfmt;
	HD_PATH_ID path_id = 0;

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
	if (vproc_param == NULL) {
		GMLIB_ERROR_P("NULL vdec_param\n");
		goto err_exit;
	}

	/* get group */
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	/* get vch, path and fd */
	vproc_vch = vproc_param->dev.device_subid;
	vproc_path = vproc_param->dev.out_subid;

	path_id = get_vproc_dedicated_path_id(&vproc_param->dev);

	din_fd = utl_get_datain_entity_fd(path_id);
	if (din_fd == 0) {
		GMLIB_ERROR_P("<vproc: utl_get_datain_entity_fd fail. line_idx(%d) >\n", line_idx);
		goto err_exit;
	}
	p_out_pool = vproc_param->param->p_out_pool;
	if (p_out_pool == NULL) {
		GMLIB_ERROR_P("<vproc out pool is not set! out_subid(%d) >\n", vproc_path);
		goto err_exit;
	}

	/* get vproc param */
	if (vproc_param->param->p_src_bg_dim == NULL) {
		GMLIB_ERROR_P("<vproc p_src_bg_dim is not set! out_subid(%d) >\n", vproc_path);
		goto err_exit;
	}
	if (vproc_param->param->p_src_fmt == NULL) {
		GMLIB_ERROR_P("<vproc p_src_fmt is not set! out_subid(%d) >\n", vproc_path);
		goto err_exit;
	}
	src_bg_dim.w = vproc_param->param->p_src_bg_dim->w;
	src_bg_dim.h = vproc_param->param->p_src_bg_dim->h;
	src_dim.w = vproc_param->param->p_src_rect ? vproc_param->param->p_src_rect->w : src_bg_dim.w;
	src_dim.h = vproc_param->param->p_src_rect ? vproc_param->param->p_src_rect->h : src_bg_dim.h;
	pxlfmt = *vproc_param->param->p_src_fmt;

	if (get_vproc_fps_setting(hd_group, line_idx, vproc_param, &fps) != HD_OK) {
		GMLIB_ERROR_P("<vproc: Error to set fps!>\n");
		goto err_exit;
	}

	/* lookup entity buf */
	din = pif_lookup_entity_buf(VPD_DIN_ENTITY_TYPE, din_fd, group->encap_buf);
	if (din != NULL) { /* It have been set in encaps buf, just return this buffer */
		*len = 0;
		goto exit;
	}

	/* set entity buf */
	*len = pif_set_EntityBuf(VPD_DIN_ENTITY_TYPE, &din, group->encap_buf);

	/* set param for vpd */
	din->sn = din_fd;
	din->ap_bindfd = BD_SET_PATH(vproc_param->dev.device_baseid + vproc_param->dev.device_subid,
				     vproc_param->dev.in_subid + HD_IN_BASE,
				     vproc_param->dev.out_subid + HD_OUT_BASE);
	memset(din->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < DIN_VAR_POOL_NUM; i++) {
		if (p_out_pool->buf_info[i].enable == HD_BUF_DISABLE) {
			din->pool[i].enable = FALSE;
			continue;
		} else if (p_out_pool->buf_info[i].enable == HD_BUF_ENABLE) {
			din->pool[i].enable = TRUE;
			din->pool[i].ddr_id = get_vproc_datain_ddrid(HD_COMMON_MEM_DISP_DEC_OUT_POOL, COMMON_PCIE_CHIP_RC);
			din->pool[i].count_decuple = p_out_pool->buf_info[i].counts;
			din->pool[i].max_counts = get_vproc_max_counts(p_out_pool->buf_info[i].counts);//p_out_pool->buf_info[i].max_counts / 10;
			din->pool[i].min_counts = VPROC_MIN_COUNT;//p_out_pool->buf_info[i].min_counts / 10;
			din->pool[i].type = VPD_DISP_DEC_OUT_POOL;
			din->pool[i].size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(src_bg_dim, pxlfmt);
			din->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(pxlfmt);
			din->pool[i].is_duplicate = 0;
		} else {  //HD_BUF_AUTO
#if DIN_VAR_POOL_NUM != 1
## something missing, need to check parameters of 2nd din_pool ...
#endif
			din->pool[i].enable = TRUE;
			din->pool[i].ddr_id = 0;
			din->pool[i].count_decuple = 30;
			din->pool[i].max_counts = 3;
			din->pool[i].min_counts = 0;
			din->pool[i].type = VPD_DISP_DEC_OUT_POOL;
			din->pool[i].size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(src_bg_dim, pxlfmt);
			din->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(pxlfmt);
			din->pool[i].is_duplicate = 0;
		}
		din->pool[i].vch = vproc_vch;
		snprintf(din->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x",
			 pif_get_pool_name_by_type(din->pool[i].type), din_fd);
		din->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		din->pool[i].width = src_bg_dim.w;
		din->pool[i].height = ALIGN16_UP(src_bg_dim.h);
		din->pool[i].fps = fps;
		din->pool[i].win_size = 0;

		GMLIB_L2_P("set_vproc_din_pool i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) fmt(%#x) dup(%d)\n",
			   i, din->pool[i].enable, din->pool[i].name, din->pool[i].type, din->pool[i].ddr_id, din->pool[i].size,
			   din->pool[i].count_decuple, din->pool[i].max_counts, din->pool[i].dst_fmt, din->pool[i].is_duplicate);
	}

	/* set param for vpem */
	din_entity = (vpd_din_entity_t *)din->e;
	din_entity->pch = vproc_path;
	din_entity->attr_vch = vproc_vch;
	din_entity->is_audio = 0;
	din_entity->fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(pxlfmt);;
	din_entity->buf_dim.w = src_bg_dim.w;
	din_entity->buf_dim.h = src_bg_dim.h;
	din_entity->src_dim.w = src_dim.w;
	din_entity->src_dim.h = src_dim.h;

	if (pxlfmt == HD_VIDEO_PXLFMT_YUV420_NVX4) {
		din_entity->side_info_ofs = common_calculate_dec_side_info_offset(&src_bg_dim, HD_CODEC_TYPE_H265);
		din_entity->side_info_size = common_calculate_dec_side_info_size(&src_bg_dim, HD_CODEC_TYPE_H265);
	} else {
		din_entity->side_info_ofs = 0;
		din_entity->side_info_size = 0;
	}

	SET_ENTITY_FEATURE(din->feature, VPD_FLAG_OUTBUF_MAX_CNT | VPD_FLAG_STOP_WHEN_APPLY);

	GMLIB_L2_P("set_proc_datain_entity path_id(%#lx) fd(%#x) pch(%d) dst_bg(%d,%d) dst_fmt(%#x)\n",
		   dif_get_path_id(&vproc_param->dev), din_fd, din_entity->attr_vch, din_entity->buf_dim.w, din_entity->buf_dim.h, din_entity->fmt);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, din);

err_exit:
	return -1;
}

INT set_vproc_osg_unit(HD_GROUP *hd_group, INT line_idx, DIF_VPROC_PARAM *vproc_param, INT *len)
{
	INT i, fps;
	pif_group_t *group;
	vpd_entity_t *osg;
	vpd_osg_entity_t *osg_entity;
	INT osg_fd, vproc_vch, vproc_path, lcd_vch, lcd_path;
	HD_DIM dim = {0, 0};
	HD_VIDEO_PXLFMT pxlfmt = 0;
	HD_OUT_POOL out_pool;

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

	vproc_vch = vproc_param->dev.device_subid;
	vproc_path = vproc_param->dev.out_subid;
	dif_get_lcd_ch_path(hd_group, line_idx, &lcd_vch, &lcd_path);
	osg_fd = utl_get_disp_osg_entity_fd(0, lcd_vch, 0, NULL);

	/* get vproc param */
	if (vproc_param->param->p_dst_bg_dim == NULL) {
		GMLIB_ERROR_P("<vproc p_dst_bg_dim is not set! out_subid(%d) >\n", vproc_path);
		goto err_exit;
	}
	if (vproc_param->param->p_dst_fmt == NULL) {
		GMLIB_ERROR_P("<vproc p_dst_fmt is not set! out_subid(%d) >\n", vproc_path);
		goto err_exit;
	}
	dim.w = vproc_param->param->p_dst_bg_dim->w;
	dim.h = vproc_param->param->p_dst_bg_dim->h;
	pxlfmt = *vproc_param->param->p_dst_fmt;

	if (get_vproc_fps_setting(hd_group, line_idx, vproc_param, &fps) != HD_OK) {
		GMLIB_ERROR_P("<vproc: Error to set fps!>\n");
		goto err_exit;
	}

	osg = pif_lookup_entity_buf(VPD_OSG_ENTITY_TYPE, osg_fd, group->encap_buf);
	if (osg != NULL) { /* It have been set in encaps buf, just return this buffer */
		*len = 0;
		goto exit;
	}
	if (dif_get_vproc_out_pool(hd_group, line_idx, &out_pool) != HD_OK) {
		GMLIB_ERROR_P("<vproc out pool is not set! line_idx(%d) >\n", line_idx);
		goto err_exit;
	}

	*len = pif_set_EntityBuf(VPD_OSG_ENTITY_TYPE, &osg, group->encap_buf);
	if (osg == NULL) {
		GMLIB_ERROR_P("<vout: NULL entity buffer!>\n");
		goto err_exit;
	}
	osg->sn = osg_fd;
	osg->ap_bindfd = hd_group->glines[line_idx].linefd;

	memset(osg->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < DOUT_POOL_NUM; i++) {
		if (out_pool.buf_info[i].enable == HD_BUF_DISABLE) {
			osg->pool[i].enable = FALSE;
			continue;
		} else if (out_pool.buf_info[i].enable == HD_BUF_ENABLE) {
			osg->pool[i].enable = TRUE;
			osg->pool[i].ddr_id = out_pool.buf_info[i].ddr_id;
			osg->pool[i].count_decuple = out_pool.buf_info[i].counts;
			osg->pool[i].max_counts = out_pool.buf_info[i].max_counts / 10;
			osg->pool[i].min_counts = out_pool.buf_info[i].min_counts / 10;
		} else {  //HD_BUF_AUTO
			osg->pool[i].enable = TRUE;
			osg->pool[i].ddr_id = 0;
			osg->pool[i].count_decuple = 30;
			osg->pool[i].max_counts = 3;
			osg->pool[i].min_counts = 0;
		}
		osg->pool[i].type = VPD_DISP0_IN_POOL + lcd_vch;
		snprintf(osg->pool[i].name, GM_POOL_NAME_LEN, "%s", pif_get_pool_name_by_type(osg->pool[i].type));
		osg->pool[i].width = dim.w;
		osg->pool[i].height = dim.h;
		osg->pool[i].fps = fps;
		osg->pool[i].vch = vproc_vch;
		osg->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		osg->pool[i].is_duplicate = 1;
		osg->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(pxlfmt);
		osg->pool[i].size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(dim, pxlfmt);;
		osg->pool[i].win_size = 0;

		GMLIB_L2_P("set_vproc_osg_pool i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) fmt(%#x) dup(%d)\n",
				   i, osg->pool[i].enable, osg->pool[i].name, osg->pool[i].type, osg->pool[i].ddr_id, osg->pool[i].size,
				   osg->pool[i].count_decuple, osg->pool[i].max_counts, osg->pool[i].dst_fmt, osg->pool[i].is_duplicate);
	}

	osg_entity = (vpd_osg_entity_t *)osg->e;
	osg_entity->src_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(pxlfmt);
	osg_entity->src_bg_dim.w = dim.w;
	osg_entity->src_bg_dim.h = dim.h;
	osg_entity->attr_vch = 0;
	osg_entity->osg_param_1.direction = 0;

	GMLIB_L2_P("set_vproc_osg_entity path_id(%#lx) fd(%#x) pch(%d) src_bg(%d,%d) src_fmt(%#x)\n",
			dif_get_path_id(&vproc_param->dev), osg_fd, osg_entity->attr_vch,
			osg_entity->src_bg_dim.w, osg_entity->src_bg_dim.h,
			osg_entity->src_fmt);


exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, osg);

err_exit:
	return -1;

}

INT set_vproc_dataout_unit(HD_GROUP *hd_group, INT line_idx, DIF_VPROC_PARAM *vproc_param, INT *len)
{
	INT i;
	unsigned int fps;
	pif_group_t *group;
	vpd_entity_t *vpd_entity = NULL;
	vpd_dout_entity_t *dout_entity;
	INT dataout_fd, vproc_path, lcd_vch, lcd_path;
	HD_DIM dim = {0, 0};
	HD_VIDEO_PXLFMT pxlfmt;

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
	if (vproc_param == NULL) {
		GMLIB_ERROR_P("NULL p_vproc_param\n");
		goto err_exit;
	}
	if (vproc_param->param == NULL) {
		GMLIB_ERROR_P("NULL p_vproc_param->param\n");
		goto err_exit;
	}

	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	vproc_path = vproc_param->dev.out_subid;
	dif_get_lcd_ch_path(hd_group, line_idx, &lcd_vch, &lcd_path);
	dataout_fd = utl_get_dataout_entity_fd(lcd_vch, 0, NULL);

	/* get vproc param */
	if (vproc_param->param->p_dst_bg_dim == NULL) {
		GMLIB_ERROR_P("<vproc p_dst_bg_dim is not set! out_subid(%d) >\n", vproc_path);
		goto err_exit;
	}
	if (vproc_param->param->p_dst_fmt == NULL) {
		GMLIB_ERROR_P("<vproc p_dst_fmt is not set! out_subid(%d) >\n", vproc_path);
		goto err_exit;
	}
	dim.w = vproc_param->param->p_dst_bg_dim->w;
	dim.h = vproc_param->param->p_dst_bg_dim->h;
	pxlfmt = *vproc_param->param->p_dst_fmt;
	pif_get_disp_max_fps(lcd_vch, &fps);

	vpd_entity = pif_lookup_entity_buf(VPD_DOUT_ENTITY_TYPE, dataout_fd, group->encap_buf);
	if (vpd_entity != NULL) { /* It have been set in encaps buf, just return this buffer */
		*len = 0;
		goto exit;
	}
	*len = pif_set_EntityBuf(VPD_DOUT_ENTITY_TYPE, &vpd_entity, group->encap_buf);
	vpd_entity->sn = dataout_fd;
	vpd_entity->ap_bindfd = hd_group->glines[line_idx].linefd;

	memset(vpd_entity->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < DOUT_POOL_NUM; i++) {
		vpd_entity->pool[i].type = VPD_END_POOL;
		snprintf(vpd_entity->pool[i].name, VPD_MAX_STRING_LEN - 1, "%s",
				pif_get_pool_name_by_type(vpd_entity->pool[i].type));

		vpd_entity->pool[i].width = 0;
		vpd_entity->pool[i].height = 0;
		vpd_entity->pool[i].fps = fps;
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
	dout_entity = (vpd_dout_entity_t *)vpd_entity->e;
	dout_entity->is_raw_out = 1;
	dout_entity->raw_out_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(dim, pxlfmt);
	dout_entity->src_frame_rate = fps;
	dout_entity->fps_ratio = PIF_SET_FPS_RATIO(1, 1);
	SET_ENTITY_FEATURE(vpd_entity->feature, VPD_FLAG_REFERENCE_CLOCK);

	GMLIB_L2_P("set_vproc_dataout_entity path_id(%#lx) fd(%#x) raw_out_size(%d) src_frame_rate(%d) fps_ratio(%#x)\n",
			dif_get_path_id(&vproc_param->dev), dataout_fd, dout_entity->raw_out_size,
			dout_entity->src_frame_rate, dout_entity->fps_ratio);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, vpd_entity);

err_exit:
	return -1;

}
/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
