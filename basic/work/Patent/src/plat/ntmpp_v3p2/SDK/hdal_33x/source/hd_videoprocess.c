/**
	@brief Source file of video process.\n
	This file contains the functions which is related to video process in the chip.

	@file hd_videoprocess.c

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
#include "hd_type.h"
#include "hdal.h"
#include "bind.h"
#include "dif.h"
#include "videoprocess.h"
#include "logger.h"
#include "cif_common.h"
#include "pif.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
unsigned long current_timestamp32(void);

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
UINT32 videoproc_max_device_nr;
UINT32 videoproc_max_device_in;
UINT32 videoproc_max_device_out;
VDOPROC_PARAM *vdoproc_param[VDOPROC_DEV][VODPROC_OUTPUT_PORT] = {{NULL}};

INT vdo_proc_alloc_nr = 0;
#define VDO_PROC_MALLOC(a)       ({vdo_proc_alloc_nr++; PIF_MALLOC((a));})
#define VDO_PROC_CALLOC(a, b)    ({vdo_proc_alloc_nr++; PIF_CALLOC((a), (b));})
#define VDO_PROC_FREE(a)         {vdo_proc_alloc_nr--; PIF_FREE((a));}

#define ALIGN64_DOWN(x)     (((x) >> 6) << 6)
#define ALIGN32_DOWN(x)     (((x) >> 5) << 5)
#define ALIGN16_DOWN(x)     (((x) >> 4) << 4)
#define ALIGN8_DOWN(x)      (((x) >> 3) << 3)
#define ALIGN4_DOWN(x)      (((x) >> 2) << 2)
#define ALIGN2_DOWN(x)      (((x) >> 1) << 1)

#define ALIGN64_UP(x)       ((((x) + 63) >> 6) << 6)
#define ALIGN32_UP(x)       ((((x) + 31) >> 5) << 5)
#define ALIGN16_UP(x)       ((((x) + 15) >> 4) << 4)
#define ALIGN8_UP(x)        ((((x) + 7) >> 3) << 3)
#define ALIGN4_UP(x)        ((((x) + 3) >> 2) << 2)
#define ALIGN2_UP(x)        ((((x) + 1) >> 1) << 1)

#define TIME_DIFF(new_val, old_val)     ((int)(new_val) - (int)(old_val))

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT hd_videoproc_bind(HD_OUT_ID out_id, HD_IN_ID dest_in_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(out_id);
	HD_IO _out_id = HD_GET_OUT(out_id);
	HD_DAL dest_id = HD_GET_DEV(dest_in_id);
	HD_IO _in_id = HD_GET_IN(dest_in_id);

	ret = bd_bind(self_id, _out_id, dest_id, _in_id);
	HD_VIDEOPROC_FLOW("hd_videoproc_bind: %s_%d_OUT_%d -> %s_%d_IN_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(_out_id),
		get_device_name(bd_get_dev_baseid(dest_id)), bd_get_dev_subid(dest_id), BD_GET_IN_SUBID(_in_id));

	return ret;
}

HD_RESULT hd_videoproc_unbind(HD_OUT_ID out_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(out_id);
	HD_IO _out_id = HD_GET_OUT(out_id);

	ret = bd_unbind(self_id, _out_id);
	HD_VIDEOPROC_FLOW("hd_videoproc_unbind: %s_%d_OUT_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(_out_id));
	return ret;
}

HD_RESULT hd_videoproc_open(HD_IN_ID in_id, HD_OUT_ID out_id, HD_PATH_ID *p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(in_id);
	HD_IO _in_id = HD_GET_IN(in_id);
	HD_DAL self_id = HD_GET_DEV(out_id);
	HD_IO _out_id = HD_GET_OUT(out_id);
	HD_RESULT ret;
	INT	idx = VDOPROC_DEVID(self_id);
	VDOPROC_PARAM *vproc_param;

	if (bd_get_dev_baseid(self_id) != HD_DAL_VIDEOPROC_BASE) {
		HD_VIDEOPROC_ERR(" The device of path_id(%#x) is not videoproc.\n", GET_VPROC_PATHID(self_id, _in_id, _out_id));
		ret = HD_ERR_DEV;
		goto state_exit;
	}

	if (in_dev_id != self_id) {
		HD_VIDEOPROC_ERR(" in(%#x) and out(%#x) aren't the same device.\n", in_id, out_id);
		ret = HD_ERR_IO;
		goto state_exit;
	}
	if (vdoproc_param[idx][VDOPROC_OUTID(_out_id)] == NULL) {
		if ((vdoproc_param[idx][VDOPROC_OUTID(_out_id)] = VDO_PROC_CALLOC(sizeof(VDOPROC_PARAM), 1)) == NULL) {
			HD_VIDEOPROC_ERR(" vdoproc_param[%d][%d] alloc memory size(%zd) fail!\n",
					 idx, VDOPROC_OUTID(_out_id), sizeof(VDOPROC_PARAM));
			ret = HD_ERR_NULL_PTR;
			goto state_exit;
		}
	}
	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(_out_id)];
	vproc_param->use_vpe_func = 0;

	/* check and set state */
	if ((vproc_param->priv.push_state == HD_PATH_ID_OPEN) ||
	    (vproc_param->priv.pull_state == HD_PATH_ID_OPEN)) {
		if ((ret = bd_get_already_open_pathid(self_id, _out_id, _in_id, p_path_id)) != HD_OK) {
			goto state_exit;
		} else {
			ret = HD_ERR_ALREADY_OPEN;
			goto state_exit;
		}
	} else if ((vproc_param->priv.push_state == HD_PATH_ID_ONGOING) ||
		   (vproc_param->priv.pull_state == HD_PATH_ID_ONGOING)) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		vproc_param->priv.push_state = HD_PATH_ID_ONGOING;
		vproc_param->priv.pull_state = HD_PATH_ID_ONGOING;
	}

	ret = bd_device_open(self_id, _out_id, _in_id, p_path_id);
	if (ret != HD_OK) {
		HD_VIDEOPROC_ERR("bd_device_open fail\n");
		goto ext;
	}

	ret = videoproc_init_path_p(*p_path_id);
	if (ret != HD_OK) {
		HD_VIDEOPROC_ERR("videoproc_init_path fail, %s path_id(%#x)\n", dif_return_dev_string(*p_path_id), *p_path_id);
		goto ext;
	}

	HD_VIDEOPROC_FLOW("hd_videoproc_open:\n    %s path_id(%#x) fd(%#x) dei_fd(%#x) alloc_nr(%d)\n",
			  dif_return_dev_string(*p_path_id), *p_path_id, vproc_param->priv.fd, vproc_param->priv.di_fd, vdo_proc_alloc_nr);
	vproc_param->priv.push_state = HD_PATH_ID_OPEN;
	vproc_param->priv.pull_state = HD_PATH_ID_OPEN;
	return ret;

ext:
	vproc_param->priv.push_state = HD_PATH_ID_CLOSED;
	vproc_param->priv.pull_state = HD_PATH_ID_CLOSED;
	return ret;

state_exit:
	return ret;
}

HD_RESULT hd_videoproc_start(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	INT idx;
	HD_DAL self_id = HD_GET_DEV(path_id);
	VDOPROC_PARAM *vproc_param;
	HD_IO out_id;

	ret = videoproc_check_path_id_p(path_id);
	if (ret != HD_OK) {
		goto ext;
	}
	idx = VDOPROC_DEVID(self_id);
	out_id = HD_GET_OUT(path_id);
	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if (vproc_param == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) start fail, it maybe no open.\n", path_id);
		ret = HD_ERR_NOT_OPEN;
		goto ext;
	}
	vproc_param->src_rect_enable = vproc_param->src_rect_enable_at_start;
	vproc_param->psr_crop_enable = vproc_param->psr_crop_enable_at_start;

	ret = bd_start_list(&path_id, 1);

ext:
	HD_VIDEOPROC_FLOW("hd_videoproc_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_videoproc_start_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;
	INT idx;
	HD_DAL self_id;
	VDOPROC_PARAM *vproc_param;
	HD_IO out_id;

	for (i = 0; i < nr; i++) {
		ret = videoproc_check_path_id_p(path_id[i]);
		if (ret != HD_OK) {
			goto ext;
		}
		HD_VIDEOPROC_FLOW("hd_videoproc_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
		ret = videoproc_check_path_id_p(path_id[i]);
		if (ret != HD_OK)
			goto ext;

		self_id = HD_GET_DEV(path_id[i]);
		idx = VDOPROC_DEVID(self_id);
		out_id = HD_GET_OUT(path_id[i]);
		vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
		if (vproc_param == NULL) {
			HD_VIDEOPROC_ERR("path_id(%#x) start fail, it maybe no open.\n", path_id[i]);
			ret = HD_ERR_NOT_OPEN;
			goto ext;
		}
		vproc_param->src_rect_enable = vproc_param->src_rect_enable_at_start;
		vproc_param->psr_crop_enable = vproc_param->psr_crop_enable_at_start;
	}

	ret = bd_start_list(path_id, nr);

ext:
	return ret;
}

HD_RESULT hd_videoproc_stop(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	ret = videoproc_check_path_id_p(path_id);
	if (ret != HD_OK) {
		goto ext;
	}
	ret = bd_stop_list(&path_id, 1);
ext:
	HD_VIDEOPROC_FLOW("hd_videoproc_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_videoproc_stop_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;

	for (i = 0; i < nr; i++) {
		ret = videoproc_check_path_id_p(path_id[i]);
		if (ret != HD_OK) {
			goto ext;
		}
	}

	ret = bd_stop_list(path_id, nr);

ext:
	return ret;
}

HD_RESULT hd_videoproc_close(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	INT idx;
	HD_DAL self_id = HD_GET_DEV(path_id);
	VDOPROC_PARAM *vproc_param;
	HD_IO out_id;

	idx = VDOPROC_DEVID(self_id);
	out_id = HD_GET_OUT(path_id);
	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if (vproc_param == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) close fail, it maybe no open.\n", path_id);
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	}
	vproc_param->use_vpe_func = 0;

	/* check and set state */
	if ((vproc_param->priv.push_state == HD_PATH_ID_CLOSED) ||
	    (vproc_param->priv.pull_state == HD_PATH_ID_CLOSED)) {
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	} else if ((vproc_param->priv.push_state == HD_PATH_ID_ONGOING) ||
		   (vproc_param->priv.pull_state == HD_PATH_ID_ONGOING)) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		vproc_param->priv.push_state = HD_PATH_ID_ONGOING;
		vproc_param->priv.pull_state = HD_PATH_ID_ONGOING;
	}


	ret = videoproc_check_path_id_p(path_id);
	if (ret != HD_OK)
		goto ext;
	ret = videoproc_close_vpemask(path_id);
	if (ret != HD_OK)
		goto ext;
	ret = videoproc_videoproc_close_uv_swap(path_id);
	if (ret != HD_OK)
		goto ext;
	ret = bd_device_close(path_id);
	if (ret != HD_OK) {
		HD_VIDEOPROC_ERR("bd_device_close fail\n");
		goto ext;
	}

	ret = videoproc_uninit_path_p(path_id);
	if (ret != HD_OK) {
		HD_VIDEOPROC_ERR("videoproc_uninit_path_p fail\n");
		goto ext;
	}

	HD_VIDEOPROC_FLOW("hd_videoproc_close:\n    %s path_id(%#x) alloc_nr(%d)\n",
			  dif_return_dev_string(path_id), path_id, vdo_proc_alloc_nr);
	vproc_param->priv.push_state = HD_PATH_ID_CLOSED;
	vproc_param->priv.pull_state = HD_PATH_ID_CLOSED;
	return ret;

ext:
	vproc_param->priv.push_state = HD_PATH_ID_OPEN;
	vproc_param->priv.pull_state = HD_PATH_ID_OPEN;
	return ret;

state_exit:
	return ret;
}

HD_RESULT hd_videoproc_get(HD_PATH_ID path_id, HD_VIDEOPROC_PARAM_ID id, VOID *p_param)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	UINT32 idx;
	INT i;
	HD_VIDEOPROC_SYSCAPS *p_syscaps;
	HD_VIDEOPROC_CROP *p_crop;
	HD_VIDEOPROC_CTRL *p_ctrl;
	HD_VIDEOPROC_OUT *p_out;
	HD_VIDEOPROC_DEV_CONFIG *p_config;
	HD_VIDEOPROC_IN *p_in;
	VDOPROC_PARAM *vproc_param;
	HD_IO out_id;
	HD_VIDEOPROC_PATTERN_IMG *p_pattern;
	HD_VIDEOPROC_PATTERN_SELECT *p_pat_sel;
	HD_VIDEOPROC_VPEMASK_ONEINFO *pvpeinfo;
	HD_VIDEOPROC_SCA_BUF_INFO *sca_buf_info;
	INT16 pattern_idx = 0;
	INT16 array_index = 0;

	ret = videoproc_check_path_id_p(path_id);
	if (ret != HD_OK)
		goto exit;

	if (p_param == NULL) {
		HD_VIDEOPROC_ERR("NULL p_param!\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	idx = VDOPROC_DEVID(self_id);
	out_id = HD_GET_OUT(path_id);
	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if (vproc_param == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		ret = HD_ERR_NG;
		goto exit;
	}

	switch (id) {
	case HD_VIDEOPROC_PARAM_SYSCAPS:
		p_syscaps = (HD_VIDEOPROC_SYSCAPS *)p_param;
		memset(p_syscaps, 0x0, sizeof(HD_VIDEOPROC_SYSCAPS));
		p_syscaps->max_w_scaleup_ratio = 8;
		p_syscaps->max_w_scaledown_ratio = 8;
		p_syscaps->max_h_scaleup_ratio = 8;
		p_syscaps->max_h_scaledown_ratio = 8;
		HD_VIDEOPROC_FLOW("hd_videoproc_get(SYSCAPS):\n"
				  "    %s path_id(%#x)\n"
				  "        max_scale w(u/d:%u,%u) h(u/d:%u,%u)\n"
				  "        in_stamp(%u,%u) in_mask(%u,%u)\n",
				  dif_return_dev_string(path_id), path_id,
				  p_syscaps->max_w_scaleup_ratio, p_syscaps->max_w_scaledown_ratio,
				  p_syscaps->max_h_scaleup_ratio, p_syscaps->max_h_scaledown_ratio,
				  p_syscaps->max_in_stamp, p_syscaps->max_in_stamp_ex,
				  p_syscaps->max_in_mask, p_syscaps->max_in_mask_ex);
		break;
	case HD_VIDEOPROC_PARAM_IN_CROP:
		p_crop = (HD_VIDEOPROC_CROP *)p_param;
		memset(p_crop, 0x0, sizeof(HD_VIDEOPROC_CROP));
		p_crop->mode = vproc_param->src_rect_enable_at_start ? HD_CROP_ON : HD_CROP_OFF;
		p_crop->win.rect.x = vproc_param->src_rect.x;
		p_crop->win.rect.y = vproc_param->src_rect.y;
		p_crop->win.rect.w = vproc_param->src_rect.w;
		p_crop->win.rect.h = vproc_param->src_rect.h;
		p_crop->win.coord.w = vproc_param->src_bg_dim.w;
		p_crop->win.coord.h = vproc_param->src_bg_dim.h;
		HD_VIDEOPROC_FLOW("hd_videoproc_get(IN_CROP):\n"
				  "    %s path_id(%#x)\n"
				  "        mode(%d) rect(%u,%u,%u,%u) bg(%u,%u)\n",
				  dif_return_dev_string(path_id), path_id, p_crop->mode, p_crop->win.rect.x, p_crop->win.rect.y,
				  p_crop->win.rect.w, p_crop->win.rect.h,
				  p_crop->win.coord.w, p_crop->win.coord.h);
		break;
	case HD_VIDEOPROC_PARAM_IN_CROP_PSR:
		p_crop = (HD_VIDEOPROC_CROP *)p_param;
		memset(p_crop, 0x0, sizeof(HD_VIDEOPROC_CROP));
		p_crop->mode = vproc_param->psr_crop_enable_at_start ? HD_CROP_ON : HD_CROP_OFF;
		p_crop->win.rect.x = vproc_param->psr_crop_rect.x;
		p_crop->win.rect.y = vproc_param->psr_crop_rect.y;
		p_crop->win.rect.w = vproc_param->psr_crop_rect.w;
		p_crop->win.rect.h = vproc_param->psr_crop_rect.h;
		p_crop->win.coord.w = vproc_param->psr_crop_bg_dim.w;
		p_crop->win.coord.h = vproc_param->psr_crop_bg_dim.h;
		HD_VIDEOPROC_FLOW("hd_videoproc_get(IN_CROP_PSR):\n"
				  "    %s path_id(%#x)\n"
				  "        mode(%d) rect(%u,%u,%u,%u) bg(%u,%u)\n",
				  dif_return_dev_string(path_id), path_id, p_crop->mode, p_crop->win.rect.x, p_crop->win.rect.y,
				  p_crop->win.rect.w, p_crop->win.rect.h,
				  p_crop->win.coord.w, p_crop->win.coord.h);
		break;
	case HD_VIDEOPROC_PARAM_CTRL:
		p_ctrl = (HD_VIDEOPROC_CTRL *)p_param;
		memset(p_ctrl, 0x0, sizeof(HD_VIDEOPROC_CTRL));
		MK_BIT(vproc_param->di_enable, p_ctrl->func, HD_VIDEOPROC_FUNC_DI);
		MK_BIT(vproc_param->dn_enable, p_ctrl->func, HD_VIDEOPROC_FUNC_3DNR);
		MK_BIT(vproc_param->sharp_enable, p_ctrl->func, HD_VIDEOPROC_FUNC_SHARP);
		HD_VIDEOPROC_FLOW("hd_videoproc_get(CTRL):\n"
				  "    %s path_id(%#x)\n"
				  "        func(%#x)\n", dif_return_dev_string(path_id), path_id, p_ctrl->func);
		break;
	case HD_VIDEOPROC_PARAM_OUT:
		p_out = (HD_VIDEOPROC_OUT *)p_param;
		memset(p_out, 0x0, sizeof(HD_VIDEOPROC_OUT));
		p_out->rect.x = vproc_param->dst_rect.x;
		p_out->rect.y = vproc_param->dst_rect.y;
		p_out->rect.w = vproc_param->dst_rect.w;
		p_out->rect.h = vproc_param->dst_rect.h;
		p_out->bg.w = vproc_param->dst_bg_dim.w;
		p_out->bg.h = vproc_param->dst_bg_dim.h;
		p_out->pxlfmt = vproc_param->dst_fmt;
		p_out->dir = vproc_param->direction;
		HD_VIDEOPROC_FLOW("hd_videoproc_get(OUT):\n"
				  "    %s path_id(%#x)\n"
				  "        rect(%u,%u,%u,%u) bg(%u,%u) pxlfmt(%#x) dir(%#x)\n",
				  dif_return_dev_string(path_id), path_id,
				  p_out->rect.x, p_out->rect.y,
				  p_out->rect.w, p_out->rect.h,
				  p_out->bg.w, p_out->bg.h,
				  p_out->pxlfmt, p_out->dir);
		break;
	case HD_VIDEOPROC_PARAM_DEV_CONFIG:
		p_config = (HD_VIDEOPROC_DEV_CONFIG *)p_param;
		memset(p_config, 0x0, sizeof(HD_VIDEOPROC_DEV_CONFIG));
		HD_VIDEOPROC_FLOW("hd_videoproc_get(DEV_CONFIG):\n"
				  "    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		for (i = 0; i < HD_VP_MAX_DATA_TYPE; i++) {
			p_config->data_pool[i].ddr_id = vproc_param->out_pool.buf_info[i].ddr_id;
			p_config->data_pool[i].counts = vproc_param->out_pool.buf_info[i].counts;
			p_config->data_pool[i].max_counts = vproc_param->out_pool.buf_info[i].max_counts;
			p_config->data_pool[i].min_counts = vproc_param->out_pool.buf_info[i].min_counts;
			p_config->data_pool[i].mode = vproc_param->out_pool.buf_info[i].enable;
			HD_VIDEOPROC_FLOW("        idx(%d) ddr(%u) count(%u) max/min(%u,%u) mode(%#x)\n",
					  i, p_config->data_pool[i].ddr_id,
					  p_config->data_pool[i].counts,
					  p_config->data_pool[i].max_counts,
					  p_config->data_pool[i].min_counts,
					  p_config->data_pool[i].mode);
		}
		break;
	case HD_VIDEOPROC_PARAM_IN:
		p_in = (HD_VIDEOPROC_IN *)p_param;
		memset(p_in, 0x0, sizeof(HD_VIDEOPROC_IN));
		p_in->dim.w = vproc_param->src_bg_dim.w;
		p_in->dim.h = vproc_param->src_bg_dim.h;
		p_in->pxlfmt = vproc_param->src_fmt;
		HD_VIDEOPROC_FLOW("hd_videoproc_get(IN):\n"
				  "    %s path_id(%#x)\n"
				  "        bg(%u,%u) pxlfmt(%#x)\n",
				  dif_return_dev_string(path_id), path_id, p_in->dim.w, p_in->dim.h, p_in->pxlfmt);
		break;
	case HD_VIDEOPROC_PARAM_PATTERN_IMG: {
		p_pattern = (HD_VIDEOPROC_PATTERN_IMG *)p_param;
		pattern_idx = p_pattern->index;
		if ((pattern_idx < 1) || (pattern_idx > HD_VIDEOPROC_PATTERN_NUM)) {
			HD_VIDEOPROC_ERR(" HD_VIDEOPROC_PARAM_PATTERN_IMG: Only support 1~%d pattern index for get\n", HD_VIDEOPROC_PATTERN_NUM);
			ret = HD_ERR_NG;
			goto exit;
		} else {
			array_index = pattern_idx - 1;
		}
		vproc_param->pattern.pattern_info[array_index].index = array_index;
		ret = videoproc_get_pattern_p(self_id, in_id, &vproc_param->pattern.pattern_info[array_index]);
		if (ret != HD_OK) {
			goto exit;
		}
		p_pattern->index = vproc_param->pattern.pattern_info[array_index].index;
		p_pattern->image.pxlfmt = vproc_param->pattern.pattern_info[array_index].image.pxlfmt;
		p_pattern->image.phy_addr[0] = vproc_param->pattern.pattern_info[array_index].image.phy_addr[0];
		p_pattern->image.dim.w = vproc_param->pattern.pattern_info[array_index].image.dim.w;
		p_pattern->image.dim.h = vproc_param->pattern.pattern_info[array_index].image.dim.h;
		HD_VIDEOPROC_FLOW("hd_videoproc_get(PATTERN_IMG):\n"
				  "    %s path_id(%#x)\n"
				  "        idx(%u) pxlfmt(%#x) pa(%p) wh(%u,%u)\n",
				  dif_return_dev_string(path_id), path_id, p_pattern->index, p_pattern->image.pxlfmt,
				  (VOID *)p_pattern->image.phy_addr[0], p_pattern->image.dim.w, p_pattern->image.dim.h);
		break;
	}
	case HD_VIDEOPROC_PARAM_PATTERN_SELECT:
		p_pat_sel = (HD_VIDEOPROC_PATTERN_SELECT *)p_param;

		pattern_idx = p_pat_sel->index;
		if ((pattern_idx < 1) || (pattern_idx > HD_VIDEOPROC_PATTERN_NUM)) {
			HD_VIDEOPROC_ERR(" HD_VIDEOPROC_PARAM_PATTERN_SELECT:Only support 1~%d pattern index for get\n", HD_VIDEOPROC_PATTERN_NUM);
			ret = HD_ERR_NG;
			goto exit;
		} else {
			array_index = pattern_idx - 1;
		}
		vproc_param->pattern.pattern_select.index = array_index;
		ret = videoproc_get_pattern_sel_p(self_id, out_id, &vproc_param->pattern.pattern_select);
		if (ret != HD_OK) {
			goto exit;
		}
		p_pat_sel->index = vproc_param->pattern.pattern_select.index;
		p_pat_sel->bg_color_sel = vproc_param->pattern.pattern_select.bg_color_sel;
		p_pat_sel->rect.x = vproc_param->pattern.pattern_select.rect.x;
		p_pat_sel->rect.y = vproc_param->pattern.pattern_select.rect.y;
		p_pat_sel->rect.w = vproc_param->pattern.pattern_select.rect.w;
		p_pat_sel->rect.h = vproc_param->pattern.pattern_select.rect.h;
		p_pat_sel->img_crop_enable = vproc_param->pattern.pattern_select.img_crop_enable;
		p_pat_sel->img_crop.x = vproc_param->pattern.pattern_select.img_crop.x;
		p_pat_sel->img_crop.y = vproc_param->pattern.pattern_select.img_crop.y;
		p_pat_sel->img_crop.w = vproc_param->pattern.pattern_select.img_crop.w;
		p_pat_sel->img_crop.h = vproc_param->pattern.pattern_select.img_crop.h;
		HD_VIDEOPROC_FLOW("hd_videoproc_get(PATTERN_SELECT):\n"
				  "    %s path_id(%#x)\n"
				  "        idx(%u) color(%#x) rect(%u,%u,%u,%u) crop_enable(%x) crop_img(%d %d %d %d)\n",
				  dif_return_dev_string(path_id), path_id,
				  p_pat_sel->index,
				  p_pat_sel->bg_color_sel,
				  p_pat_sel->rect.x,
				  p_pat_sel->rect.y,
				  p_pat_sel->rect.w,
				  p_pat_sel->rect.h,
				  p_pat_sel->img_crop_enable,
				  p_pat_sel->img_crop.x,
				  p_pat_sel->img_crop.y,
				  p_pat_sel->img_crop.w,
				  p_pat_sel->img_crop.h);
		break;
	case HD_VIDEOPROC_PARAM_VPEMASK_ATTR:
		pvpeinfo = (HD_VIDEOPROC_VPEMASK_ONEINFO *)p_param;
		pattern_idx = pvpeinfo->mask_idx;
		if ((pattern_idx < 0) || (pattern_idx > HD_VIDEOPROC_MASK_NUM)) {
			HD_VIDEOPROC_ERR(" HD_VIDEOPROC_PARAM_VPEMASK_ATTR: Only support 0~%d mask index for get\n", HD_VIDEOPROC_MASK_NUM);
			ret = HD_ERR_NG;
			goto exit;
		}
		vproc_param->vpeinfo.mask_idx = pattern_idx;
		ret = videoproc_get_vpemask_p(self_id, out_id, &vproc_param->vpeinfo);
		if (ret != HD_OK) {
			goto exit;
		}
		pvpeinfo->mask_idx = vproc_param->vpeinfo.mask_idx;
		pvpeinfo->mask_area = vproc_param->vpeinfo.mask_area;
		pvpeinfo->point[0].x = vproc_param->vpeinfo.point[0].x;
		pvpeinfo->point[0].y = vproc_param->vpeinfo.point[0].y;
		pvpeinfo->point[1].x = vproc_param->vpeinfo.point[1].x;
		pvpeinfo->point[1].y = vproc_param->vpeinfo.point[1].y;
		pvpeinfo->point[2].x = vproc_param->vpeinfo.point[2].x;
		pvpeinfo->point[2].y = vproc_param->vpeinfo.point[2].y;
		pvpeinfo->point[3].x = vproc_param->vpeinfo.point[3].x;
		pvpeinfo->point[3].y = vproc_param->vpeinfo.point[3].y;
		pvpeinfo->mosaic_en  = vproc_param->vpeinfo.mosaic_en;
		pvpeinfo->alpha      = vproc_param->vpeinfo.alpha;
		pvpeinfo->index      = vproc_param->vpeinfo.index;
		HD_VIDEOPROC_FLOW("hd_videoproc_get(VPEMASK_ATTR):\n"
				  "    %s path_id(%#x)\n"
				  "        idx(%u) mask(%u,%#x) p0(%d,%d) p1(%d,%d) "
				  "        p2(%d,%d) p3(%d,%d) mosaic_en(%u) alpha(%u)\n",
				  dif_return_dev_string(path_id), path_id,
				  pvpeinfo->index,
				  pvpeinfo->mask_idx, pvpeinfo->mask_area,
				  pvpeinfo->point[0].x, pvpeinfo->point[0].y,
				  pvpeinfo->point[1].x, pvpeinfo->point[1].y,
				  pvpeinfo->point[2].x, pvpeinfo->point[2].y,
				  pvpeinfo->point[3].x, pvpeinfo->point[3].y,
				  pvpeinfo->mosaic_en, pvpeinfo->alpha);
		break;
	case HD_VIDEOPROC_PARAM_SCA_WK_BUF:
		sca_buf_info = (HD_VIDEOPROC_SCA_BUF_INFO *)p_param;
		ret = videoproc_get_sca_buf_info(self_id, out_id, sca_buf_info);
		if (ret != HD_OK) {
			HD_VIDEOPROC_ERR("HD_VIDEOPROC_PARAM_SCA_WK_BUF fail\n");
			goto exit;
		}
		HD_VIDEOPROC_FLOW("hd_videoproc_get(SCA_WK_BUF):\n"
				  "    %s path_id(%#x)\n"
				  "        ddr(%u) pa(%p) size(%u)\n",
				  dif_return_dev_string(path_id), path_id, sca_buf_info->ddr_id,
				  (VOID *)sca_buf_info->pbuf_addr,
				  sca_buf_info->pbuf_size);
		break;
	default:
		HD_VIDEOPROC_ERR("Not support to get param id(%d)\n", id);
		ret = HD_ERR_NG;
		goto exit;
	}

exit:
	return ret;
}

HD_RESULT hd_videoproc_set(HD_PATH_ID path_id, HD_VIDEOPROC_PARAM_ID id, VOID *p_param)
{
	HD_RESULT ret = HD_OK, vpe_ret = 0;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	UINT32 idx = 0;
	INT i = 0;
	HD_VIDEOPROC_CROP *p_crop = NULL;
	HD_VIDEOPROC_CTRL *p_ctrl = NULL;
	HD_VIDEOPROC_OUT *p_out = NULL;
	HD_VIDEOPROC_PATTERN_IMG *p_pattern = NULL;
	HD_VIDEOPROC_PATTERN_SELECT *p_pat_sel = NULL;
	HD_VIDEOPROC_DEV_CONFIG *p_config = NULL;
	HD_VIDEOPROC_VPEMASK_ONEINFO *pvpeinfo = NULL;
	HD_VIDEOPROC_SCA_BUF_INFO *sca_buf_info = NULL;
	HD_VIDEOPROC_IN *p_in = NULL;
	INT16 pattern_idx = 0;
	INT16 array_index = 0;
	INT is_apply_attr = 1;
	INT32 align1 = 0;
	VDOPROC_PARAM *vproc_param = NULL, *vproc_param_tmp = NULL;
	HD_IO out_id = 0;
	HD_URECT *p_dst_rect = NULL;
	UINT check_ctrl = 0xffffffff;

	ret = videoproc_check_path_id_p(path_id);
	if (ret != HD_OK) {
		goto exit;
	}

	if (p_param == NULL) {
		HD_VIDEOPROC_ERR("NULL p_param!\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	idx = VDOPROC_DEVID(self_id);
	out_id = HD_GET_OUT(path_id);
	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if (vproc_param == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		ret = HD_ERR_NG;
		goto exit;
	}

	switch (id) {
	case HD_VIDEOPROC_PARAM_SYSCAPS:
		HD_VIDEOPROC_ERR("Not support to set system information.\n");
		ret = HD_ERR_NG;
		goto exit;
	case HD_VIDEOPROC_PARAM_IN_CROP:
		p_crop = (HD_VIDEOPROC_CROP *)p_param;
		if ((p_crop->mode == HD_CROP_OFF) || (vproc_param->src_rect_enable != p_crop->mode)) {
			is_apply_attr = 0;
		}
		vproc_param->use_vpe_func = 1;
		vproc_param->src_rect_enable_at_start = (p_crop->mode == HD_CROP_ON) ? TRUE : FALSE;
		if ((p_crop->win.coord.w != 0) && (p_crop->win.coord.h != 0)) {
			vproc_param->src_rect.x = p_crop->win.rect.x;
			vproc_param->src_rect.y = p_crop->win.rect.y;
			vproc_param->src_rect.w = p_crop->win.rect.w;
			vproc_param->src_rect.h = p_crop->win.rect.h;
			align1 = ALIGN8_DOWN(p_crop->win.rect.w);
			if (align1 != p_crop->win.rect.w) {
				HD_VIDEOPROC_ERR("crop w (%d) must align to 8.\n", p_crop->win.rect.w);
				if (align1) {
					vproc_param->src_rect.w = align1;
				} else {
					vproc_param->src_rect.w = 8;
				}
			}
			align1 = ALIGN2_DOWN(p_crop->win.rect.h);
			if (align1 != p_crop->win.rect.h) {
				HD_VIDEOPROC_ERR("crop h (%d) must align to 2.\n", p_crop->win.rect.h);
				if (align1) {
					vproc_param->src_rect.h = align1;
				} else {
					vproc_param->src_rect.h = 2;
				}
			}

			vproc_param->src_bg_dim.w = p_crop->win.coord.w;
			vproc_param->src_bg_dim.h = p_crop->win.coord.h;
			HD_VIDEOPROC_MSG("IN_CROP idx(%u) xywh(%u,%u,%u,%u) bg_wh(%u,%u)\n", idx,
					 p_crop->win.rect.x, p_crop->win.rect.y, vproc_param->src_rect.w,
					 vproc_param->src_rect.h, p_crop->win.coord.w, p_crop->win.coord.h);
			HD_VIDEOPROC_FLOW("hd_videoproc_set(IN_CROP):\n"
					  "    %s path_id(%#x)\n"
					  "        mode(%d) rect(%u,%u,%u,%u) bg(%u,%u)\n",
					  dif_return_dev_string(path_id), path_id, p_crop->mode, p_crop->win.rect.x, p_crop->win.rect.y,
					  p_crop->win.rect.w, p_crop->win.rect.h,
					  p_crop->win.coord.w, p_crop->win.coord.h);
			vproc_param_tmp = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
			vproc_param_tmp->src_rect_enable = vproc_param->src_rect_enable;
			vproc_param_tmp->src_rect = vproc_param->src_rect;
			vproc_param_tmp->src_bg_dim = vproc_param->src_bg_dim;
		} else {
			HD_VIDEOPROC_ERR("When hd_videoproc_set id(%d), the win.coord dim must be set. \n", id);
			ret = HD_ERR_NG;
			goto exit;

		}
		if (videoproc_check_rotate_and_crop(vproc_param) != HD_OK) {
			ret = HD_ERR_NG;
			goto exit;
		}
		if (videoproc_check_pip_and_psr(path_id, vproc_param, p_crop) != HD_OK) {
			ret = HD_ERR_NG;
			goto exit;
		}
		if (videoproc_check_pattern(path_id, vproc_param) != HD_OK) {
			ret = HD_ERR_NG;
			goto exit;
		}

		break;
	case HD_VIDEOPROC_PARAM_IN_CROP_PSR:
		p_crop = (HD_VIDEOPROC_CROP *)p_param;
		if ((p_crop->mode == HD_CROP_OFF) || (vproc_param->psr_crop_enable != p_crop->mode)) {
			is_apply_attr = 0;
		}
		vproc_param->use_vpe_func = 1;
		vproc_param->psr_crop_enable_at_start = (p_crop->mode == HD_CROP_ON) ? TRUE : FALSE;
		if ((p_crop->win.coord.w != 0) && (p_crop->win.coord.h != 0)) {
			vproc_param->psr_crop_rect.x = p_crop->win.rect.x;
			vproc_param->psr_crop_rect.y = p_crop->win.rect.y;
			vproc_param->psr_crop_rect.w = p_crop->win.rect.w;
			vproc_param->psr_crop_rect.h = p_crop->win.rect.h;
			align1 = ALIGN8_DOWN(p_crop->win.rect.w);
			if (align1 != p_crop->win.rect.w) {
				HD_VIDEOPROC_ERR("crop w (%d) must align to 8.\n", p_crop->win.rect.w);
				if (align1) {
					vproc_param->psr_crop_rect.w = align1;
				} else {
					vproc_param->psr_crop_rect.w = 8;
				}
			}
			align1 = ALIGN2_DOWN(p_crop->win.rect.h);
			if (align1 != p_crop->win.rect.h) {
				HD_VIDEOPROC_ERR("crop h (%d) must align to 2.\n", p_crop->win.rect.h);
				if (align1) {
					vproc_param->psr_crop_rect.h = align1;
				} else {
					vproc_param->psr_crop_rect.h = 2;
				}
			}
			vproc_param->psr_crop_bg_dim.w = p_crop->win.coord.w;
			vproc_param->psr_crop_bg_dim.h = p_crop->win.coord.h;
			HD_VIDEOPROC_MSG("IN_CROP_PSR idx(%u) xywh(%u,%u,%u,%u) bg_wh(%u,%u)\n", idx,
					 p_crop->win.rect.x, p_crop->win.rect.y, vproc_param->psr_crop_rect.w,
					 vproc_param->psr_crop_rect.h, p_crop->win.coord.w, p_crop->win.coord.h);
			HD_VIDEOPROC_FLOW("hd_videoproc_set(IN_CROP_PSR):\n"
					  "    %s path_id(%#x)\n"
					  "        mode(%d) rect(%u,%u,%u,%u) bg(%u,%u)\n",
					  dif_return_dev_string(path_id), path_id, p_crop->mode, p_crop->win.rect.x, p_crop->win.rect.y,
					  p_crop->win.rect.w, p_crop->win.rect.h,
					  p_crop->win.coord.w, p_crop->win.coord.h);
		} else {
			HD_VIDEOPROC_ERR("When hd_videoproc_set id(%d), the win.coord dim must be set. \n", id);
			ret = HD_ERR_NG;
			goto exit;

		}
		if (videoproc_check_rotate_and_crop(vproc_param) != HD_OK) {
			ret = HD_ERR_NG;
			goto exit;
		}
		if (videoproc_check_pip_and_psr(path_id, vproc_param, p_crop) != HD_OK) {
			ret = HD_ERR_NG;
			goto exit;
		}
		break;
	case HD_VIDEOPROC_PARAM_CTRL:
		p_ctrl = (HD_VIDEOPROC_CTRL *)p_param;
		CLR_BIT(check_ctrl, HD_VIDEOPROC_FUNC_DI);
		CLR_BIT(check_ctrl, HD_VIDEOPROC_FUNC_3DNR);
		CLR_BIT(check_ctrl, HD_VIDEOPROC_FUNC_SHARP);
		if (p_ctrl->func & check_ctrl) {
			HD_VIDEOPROC_ERR("Only support HD_VIDEOPROC_FUNC_DI for HD_VIDEOPROC_CTRL\n");
			ret = HD_ERR_NG;
			goto exit;
		}
		vproc_param->di_enable = IS_BIT(p_ctrl->func, HD_VIDEOPROC_FUNC_DI);
		vproc_param->dn_enable = IS_BIT(p_ctrl->func, HD_VIDEOPROC_FUNC_3DNR);
		vproc_param->sharp_enable = IS_BIT(p_ctrl->func, HD_VIDEOPROC_FUNC_SHARP);
		HD_VIDEOPROC_MSG("CTRL idx(%u) di(%u) dn(%u) sharp(%u)\n", idx,
				 (UINT32)IS_BIT(p_ctrl->func, HD_VIDEOPROC_FUNC_DI),
				 (UINT32)IS_BIT(p_ctrl->func, HD_VIDEOPROC_FUNC_3DNR),
				 (UINT32)IS_BIT(p_ctrl->func, HD_VIDEOPROC_FUNC_SHARP));
		HD_VIDEOPROC_FLOW("hd_videoproc_set(CTRL):\n"
				  "    %s path_id(%#x)\n"
				  "        func(%#x)\n", dif_return_dev_string(path_id), path_id, p_ctrl->func);
		break;
	case HD_VIDEOPROC_PARAM_OUT:
		p_out = (HD_VIDEOPROC_OUT *)p_param;
		vproc_param->dst_rect.x = p_out->rect.x;
		vproc_param->dst_rect.y = p_out->rect.y;
		vproc_param->dst_rect.w = p_out->rect.w;
		vproc_param->dst_rect.h = p_out->rect.h;
		vproc_param->dst_bg_dim.w = p_out->bg.w;
		vproc_param->dst_bg_dim.h = p_out->bg.h;
		vproc_param->dst_fmt = p_out->pxlfmt;
		vproc_param->direction = p_out->dir;
		HD_VIDEOPROC_MSG("OUT idx(%u) xywh(%u,%u,%u,%u) bg(%u,%u) dst_fmt(%#x) dir(%#x)\n", idx,
				 p_out->rect.x, p_out->rect.y, p_out->rect.w, p_out->rect.h,
				 p_out->bg.w, p_out->bg.h, p_out->pxlfmt, p_out->dir);
		HD_VIDEOPROC_FLOW("hd_videoproc_set(OUT):\n"
				  "    %s path_id(%#x)\n"
				  "        rect(%u,%u,%u,%u) bg(%u,%u) pxlfmt(%#x) dir(%#x)\n",
				  dif_return_dev_string(path_id), path_id, p_out->rect.x, p_out->rect.y,
				  p_out->rect.w, p_out->rect.h,
				  p_out->bg.w, p_out->bg.h,
				  p_out->pxlfmt, p_out->dir);

		if (videoproc_check_rotate_and_crop(vproc_param) != HD_OK) {
			ret = HD_ERR_NG;
			goto exit;
		}
		break;
	case HD_VIDEOPROC_PARAM_PATTERN_IMG: {
		p_pattern = (HD_VIDEOPROC_PATTERN_IMG *)p_param;
		vproc_param->use_vpe_func = 1;
		pattern_idx = p_pattern->index;
		if ((pattern_idx < 1) || (pattern_idx > HD_VIDEOPROC_PATTERN_NUM)) {
			HD_VIDEOPROC_ERR(" HD_VIDEOPROC_PARAM_PATTERN_IMG:Only support 1~%d pattern index\n", HD_VIDEOPROC_PATTERN_NUM);
			ret = HD_ERR_NG;
			goto exit;
		} else {
			array_index = pattern_idx - 1;
		}
		vproc_param->pattern.pattern_info[array_index].image.pxlfmt = p_pattern->image.pxlfmt;
		vproc_param->pattern.pattern_info[array_index].image.phy_addr[0] = p_pattern->image.phy_addr[0];
		vproc_param->pattern.pattern_info[array_index].image.ddr_id = p_pattern->image.ddr_id;
		if (pif_address_ddr_sanity_check(vproc_param->pattern.pattern_info[array_index].image.phy_addr[0], vproc_param->pattern.pattern_info[array_index].image.ddr_id) < 0) {
			HD_VIDEOPROC_ERR("%s:Check vpe pattern pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)vproc_param->pattern.pattern_info[array_index].image.phy_addr[0],
				vproc_param->pattern.pattern_info[array_index].image.ddr_id);
			ret = HD_ERR_PARAM;
			goto exit;
		}
		vproc_param->pattern.pattern_info[array_index].image.dim.w = p_pattern->image.dim.w;
		vproc_param->pattern.pattern_info[array_index].image.dim.h = p_pattern->image.dim.h;
		vproc_param->pattern.pattern_info[array_index].index = array_index;
		videoproc_set_pattern_p(self_id, in_id, &vproc_param->pattern.pattern_info[array_index]);
		HD_VIDEOPROC_FLOW("hd_videoproc_set(PATTERN_IMG):\n"
				  "    %s path_id(%#x)\n"
				  "        idx(%u) pxlfmt(%#x) pa(%p) wh(%u,%u)\n",
				  dif_return_dev_string(path_id), path_id, p_pattern->index, p_pattern->image.pxlfmt,
				  (VOID *)p_pattern->image.phy_addr[0], p_pattern->image.dim.w, p_pattern->image.dim.h);
		break;
	}
	case HD_VIDEOPROC_PARAM_PATTERN_SELECT:
		p_pat_sel = (HD_VIDEOPROC_PATTERN_SELECT *)p_param;
		p_dst_rect = (HD_URECT *) & (vproc_param->dst_rect);
		vproc_param->use_vpe_func = 1;
		pattern_idx = p_pat_sel->index;
		if ((pattern_idx < 0) || (pattern_idx > HD_VIDEOPROC_PATTERN_NUM)) {
			HD_VIDEOPROC_ERR(" HD_VIDEOPROC_PARAM_PATTERN_SELECT:Only support 0~%d pattern index,0:disabled\n", HD_VIDEOPROC_PATTERN_NUM);
			ret = HD_ERR_NG;
			goto exit;
		} else {
			array_index = pattern_idx - 1;
		}
		if (pattern_idx > 0) {
			vproc_param->pattern.pattern_select.bg_color_sel = p_pat_sel->bg_color_sel;
			vproc_param->pattern.pattern_select.rect.x = p_pat_sel->rect.x;
			vproc_param->pattern.pattern_select.rect.y = p_pat_sel->rect.y;
			vproc_param->pattern.pattern_select.rect.w = p_pat_sel->rect.w;
			vproc_param->pattern.pattern_select.rect.h = p_pat_sel->rect.h;
			vproc_param->pattern.pattern_select.index = array_index;
			if (p_pat_sel->img_crop_enable == HD_VIDEOPROC_PATTERN_CROP_ENABLE) {
				vproc_param->pattern.pattern_select.img_crop_enable = p_pat_sel->img_crop_enable;
				vproc_param->pattern.pattern_select.img_crop.x = p_pat_sel->img_crop.x;
				vproc_param->pattern.pattern_select.img_crop.y = p_pat_sel->img_crop.y;
				vproc_param->pattern.pattern_select.img_crop.w = p_pat_sel->img_crop.w;
				vproc_param->pattern.pattern_select.img_crop.h = p_pat_sel->img_crop.h;
			} else {
				vproc_param->pattern.pattern_select.img_crop_enable = 0;
				vproc_param->pattern.pattern_select.img_crop.x = 0;
				vproc_param->pattern.pattern_select.img_crop.y = 0;
				vproc_param->pattern.pattern_select.img_crop.w = vproc_param->pattern.pattern_info[array_index].image.dim.w;
				vproc_param->pattern.pattern_select.img_crop.h = vproc_param->pattern.pattern_info[array_index].image.dim.h;
			}
			if (videoproc_check_pattern(path_id, vproc_param) != HD_OK) {
				ret = HD_ERR_NG;
				goto exit;
			}
			ret = videoproc_enable_pattern_p(self_id, out_id, &vproc_param->pattern.pattern_select, p_dst_rect);
		} else {
			ret = videoproc_enable_pattern_p(self_id, out_id, NULL, p_dst_rect);
			vproc_param->pattern.pattern_select.index = HD_VIDEOPROC_PATTERN_DISABLE;
		}
		HD_VIDEOPROC_FLOW("hd_videoproc_set(PATTERN_SELECT):\n"
				  "    %s path_id(%#x)\n"
				  "        idx(%u) color(%#x) rect(%u,%u,%u,%u) crop_enable(%x) crop_img(%d %d %d %d)\n",
				  dif_return_dev_string(path_id), path_id, p_pat_sel->index, p_pat_sel->bg_color_sel,
				  p_pat_sel->rect.x, p_pat_sel->rect.y, p_pat_sel->rect.w, p_pat_sel->rect.h,
				  p_pat_sel->img_crop_enable,
				  vproc_param->pattern.pattern_select.img_crop.x,
				  vproc_param->pattern.pattern_select.img_crop.y,
				  vproc_param->pattern.pattern_select.img_crop.w,
				  vproc_param->pattern.pattern_select.img_crop.h);
		if (ret != HD_OK) {
			goto exit;
		}
		break;
	case HD_VIDEOPROC_PARAM_VPEMASK_ATTR:
		pvpeinfo = (HD_VIDEOPROC_VPEMASK_ONEINFO *)p_param;
		memcpy((void *)&vproc_param->vpeinfo, (void *)pvpeinfo, sizeof(HD_VIDEOPROC_VPEMASK_ONEINFO));
		vproc_param->use_vpe_func = 1;
		vpe_ret = videoproc_set_vpemask_p(self_id, out_id, &vproc_param->vpeinfo);
		HD_VIDEOPROC_FLOW("hd_videoproc_set(VPEMASK_ATTR):\n"
				  "    %s path_id(%#x)\n"
				  "        idx(%u) mask(%u,%#x) p0(%d,%d) p1(%d,%d) "
				  "        p2(%d,%d) p3(%d,%d) mosaic_en(%u) alpha(%u)\n",
				  dif_return_dev_string(path_id), path_id, pvpeinfo->index, pvpeinfo->mask_idx, pvpeinfo->mask_area,
				  pvpeinfo->point[0].x, pvpeinfo->point[0].y,
				  pvpeinfo->point[1].x, pvpeinfo->point[1].y,
				  pvpeinfo->point[2].x, pvpeinfo->point[2].y,
				  pvpeinfo->point[3].x, pvpeinfo->point[3].y,
				  pvpeinfo->mosaic_en, pvpeinfo->alpha);
		if (vpe_ret != HD_OK) {
			HD_VIDEOPROC_ERR("HD_VIDEOPROC_PARAM_VPEMASK_ATTR fail\n");
			HD_VIDEOPROC_ERR("hd_videoproc_set(VPEMASK_ATTR):\n"
					 "    %s path_id(%#x)\n"
					 "        idx(%u) mask(%u,%#x) p0(%d,%d) p1(%d,%d) "
					 "        p2(%d,%d) p3(%d,%d) mosaic_en(%u) alpha(%u)\n",
					 dif_return_dev_string(path_id), path_id, pvpeinfo->index, pvpeinfo->mask_idx, pvpeinfo->mask_area,
					 pvpeinfo->point[0].x, pvpeinfo->point[0].y,
					 pvpeinfo->point[1].x, pvpeinfo->point[1].y,
					 pvpeinfo->point[2].x, pvpeinfo->point[2].y,
					 pvpeinfo->point[3].x, pvpeinfo->point[3].y,
					 pvpeinfo->mosaic_en, pvpeinfo->alpha);

			ret = HD_ERR_NG;
			goto exit;
		}
		break;
	case HD_VIDEOPROC_PARAM_DEV_CONFIG:
		p_config = (HD_VIDEOPROC_DEV_CONFIG *)p_param;
		HD_VIDEOPROC_FLOW("hd_videoproc_set(DEV_CONFIG):\n"
				  "    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		for (i = 0; i < HD_VP_MAX_DATA_TYPE; i++) {
			vproc_param->out_pool.buf_info[i].ddr_id = p_config->data_pool[i].ddr_id;
			vproc_param->out_pool.buf_info[i].counts = p_config->data_pool[i].counts;
			vproc_param->out_pool.buf_info[i].max_counts = p_config->data_pool[i].max_counts;
			vproc_param->out_pool.buf_info[i].min_counts = p_config->data_pool[i].min_counts;
			vproc_param->out_pool.buf_info[i].enable = p_config->data_pool[i].mode;
			HD_VIDEOPROC_FLOW("        idx(%d) ddr(%u) count(%u) max/min(%u,%u) mode(%#x)\n",
					  i, p_config->data_pool[i].ddr_id,
					  p_config->data_pool[i].counts,
					  p_config->data_pool[i].max_counts,
					  p_config->data_pool[i].min_counts,
					  p_config->data_pool[i].mode);
		}
		break;
	case HD_VIDEOPROC_PARAM_SCA_WK_BUF:
		sca_buf_info = (HD_VIDEOPROC_SCA_BUF_INFO *)p_param;
        if (sca_buf_info->pbuf_addr !=  (UINTPTR)-1 && pif_address_ddr_sanity_check(sca_buf_info->pbuf_addr, sca_buf_info->ddr_id) < 0) {
			HD_VIDEOPROC_ERR("%s:Check vpe sca_wk_buf pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)sca_buf_info->pbuf_addr, sca_buf_info->ddr_id);
			ret = HD_ERR_PARAM;
			goto exit;
		}
		ret = videoproc_set_sca_buf_info(self_id, out_id, sca_buf_info);
		if (ret != HD_OK) {
			HD_VIDEOPROC_ERR("HD_VIDEOPROC_PARAM_SCA_WK_BUF fail\n");
			goto exit;
		}
		HD_VIDEOPROC_FLOW("hd_videoproc_set(SCA_WK_BUF):\n"
				  "    %s path_id(%#x)\n"
				  "        ddr(%u) pa(%p) size(%u)\n",
				  dif_return_dev_string(path_id), path_id, sca_buf_info->ddr_id,
				  (VOID *)sca_buf_info->pbuf_addr,
				  sca_buf_info->pbuf_size);
		break;
	case HD_VIDEOPROC_PARAM_IN:
		p_in = (HD_VIDEOPROC_IN *)p_param;
		vproc_param->src_bg_dim.w = p_in->dim.w;
		vproc_param->src_bg_dim.h = p_in->dim.h;
		vproc_param->src_fmt = p_in->pxlfmt;
		HD_VIDEOPROC_MSG("IN idx(%u) bg(%u,%u) dst_fmt(%#x)\n", idx,
				 p_in->dim.w, p_in->dim.h, p_in->pxlfmt);
		HD_VIDEOPROC_FLOW("hd_videoproc_set(IN):\n"
				  "    %s path_id(%#x)\n"
				  "        bg(%u,%u) pxlfmt(%#x)\n",
				  dif_return_dev_string(path_id), path_id, p_in->dim.w, p_in->dim.h, p_in->pxlfmt);
		break;
	default:
		HD_VIDEOPROC_ERR("Not support to set param id(%d)\n", id);
		ret = HD_ERR_NG;
		goto exit;
	}

	ret = videoproc_set_param_p(self_id, VDOPROC_OUTID(out_id), id);
	if (ret != HD_OK) {
		HD_VIDEOPROC_ERR("set parameter error\n");
	}
	if (id == HD_VIDEOPROC_PARAM_IN_CROP || id == HD_VIDEOPROC_PARAM_IN_CROP_PSR) {
		if (is_apply_attr == 1) {
			videoproc_set_crop_p(self_id, out_id, in_id); //use in_id to find previous p_bind
		}
	}

exit:
	return ret;
}


HD_RESULT hd_videoproc_push_in_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame,
				   HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT idx = VDOPROC_DEVID(self_id);
	CHAR bind_src_name[64] = "-";
	HD_VIDEO_FRAME *p_next_video_frame = NULL;
	HD_VIDEOPROC_SEND_LIST send_list = {0};
	VOID *send_list_va = 0;
	UINT32 frame_size = 0;
	VDOPROC_PARAM *vproc_param;
	HD_IO out_id = HD_GET_OUT(path_id);

	if (VDOPROC_OUTID(out_id) >= videoproc_max_device_out) {
		HD_VIDEOPROC_ERR("Check output port error, port(%#x) max(%#x)\n", VDOPROC_OUTID(out_id), videoproc_max_device_out);
		ret = HD_ERR_NG;
		goto state_exit;
	}

	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if (vproc_param == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		ret = HD_ERR_NG;
		goto state_exit;
	}

	/* check and set state */
	if (vproc_param->priv.push_state == HD_PATH_ID_CLOSED) {
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	} else if (vproc_param->priv.push_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		vproc_param->priv.push_state = HD_PATH_ID_ONGOING;
	}

	ret = videoproc_check_path_id_p(path_id);
	if (ret != HD_OK) {
		goto exit;
	}
	if (p_video_frame == NULL) {
		ret = HD_ERR_PARAM;
		goto exit;
	}

	vproc_param->src_fmt = p_video_frame->pxlfmt;
	ret = videoproc_check_is_needed_param_set_p(path_id, vproc_param->src_fmt, vproc_param->dst_fmt, NULL);
	if (ret != HD_OK) {
		goto exit;
	}

	ret = dif_check_flow_mode(self_id, out_id);
	if (ret == HD_ERR_ALREADY_BIND) {
		frame_size = hd_common_mem_calc_buf_size((void *)p_video_frame);
		send_list_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE,
						  p_video_frame->phy_addr[0],
						  frame_size);
		if (send_list_va == NULL) {
			goto exit;
		}
		send_list.path_id = path_id;
		send_list.user_bs.p_bs_buf = (CHAR *)send_list_va;
		send_list.user_bs.bs_buf_size = frame_size;
		send_list.user_bs.time_align = HD_VIDEODEC_TIME_ALIGN_DISABLE;
		send_list.user_bs.time_diff = 0;
		if ((ret = hd_videoproc_send_list(&send_list, 1, wait_ms)) != HD_OK) {
			goto exit;
		}
	} else if (ret == HD_ERR_NOT_AVAIL) {
		if ((vproc_param->di_enable) && (vproc_param->priv.di_fd == 0)) {
			ret = HD_ERR_NOT_OPEN;
			goto exit;
		} else {
			if (vproc_param->priv.fd == 0) {
				ret = HD_ERR_NOT_OPEN;
				goto exit;
			}
		}

		if ((vproc_param->push_in_used == 1) ||
		    (bd_get_prev(self_id, in_id, bind_src_name, sizeof(bind_src_name)) == HD_OK)) {
			HD_VIDEOPROC_ERR("videoproc path(0x%x) is already used(%d). dev_id(%d) src(%s)\n",
					 (int)path_id, vproc_param->push_in_used, (int)self_id, bind_src_name);
			system("echo videoproc all > /proc/hdal/setting");
			system("cat /proc/hdal/setting");
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}
		vproc_param->push_in_used = 1;

		HD_TRIGGER_FLOW("hd_videoproc_push_in_buf:\n");
		HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		HD_TRIGGER_FLOW("       in_buf_info: ddrid(%d) dim(%dx%d) fmt(%#x) pa(%p)\n",
				p_video_frame->ddr_id, p_video_frame->dim.w, p_video_frame->dim.h,
				p_video_frame->pxlfmt, (VOID *)p_video_frame->phy_addr[0]);
		if (p_user_out_video_frame) {
			HD_TRIGGER_FLOW("       out_buf_info: ddrid(%d) dim(%dx%d) pa(%p) p_next(%p)\n",
					p_user_out_video_frame->ddr_id,	p_user_out_video_frame->dim.w,
					p_user_out_video_frame->dim.h,	(VOID *)p_user_out_video_frame->phy_addr[0],
					p_user_out_video_frame->p_next);


			p_next_video_frame = (HD_VIDEO_FRAME *)(p_user_out_video_frame->p_next);
			while ((p_next_video_frame) &&
			       (p_next_video_frame->ddr_id < DDR_ID_MAX) &&
			       (videoproc_check_addr((UINTPTR)p_next_video_frame) == TRUE)) {
				HD_TRIGGER_FLOW("           ->p_next: ddrid(%d) dim(%dx%d) pa(%p)\n",
						p_next_video_frame->ddr_id,	p_next_video_frame->dim.w,
						p_next_video_frame->dim.h, (VOID *)p_next_video_frame->phy_addr[0]);
				p_next_video_frame = (HD_VIDEO_FRAME *)(p_next_video_frame->p_next);
			}
		}
		p_video_frame->reserved[0] = path_id;
		ret = videoproc_push_in_buf_p(path_id, p_video_frame, p_user_out_video_frame, wait_ms);
		if (ret == HD_OK) {
			vproc_param->push_in_used = 0;
		}
	} else {
		HD_VIDEOPROC_ERR("hd_videoproc_push_in_buf fail. ret(%#x)\n", ret);
		goto exit;
	}

exit:
	vproc_param->priv.push_state = HD_PATH_ID_OPEN;
	if (send_list_va) {
		if (hd_common_mem_munmap(send_list_va, frame_size) != HD_OK) {
			HD_VIDEOPROC_ERR("munmap fail.\n");
		}
	}

	return ret;

state_exit:
	return ret;
}

HD_RESULT hd_videoproc_pull_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_VIDEO_FRAME *p_next_video_frame = NULL;
	INT idx = VDOPROC_DEVID(self_id);
	VDOPROC_PARAM *vproc_param;

	if (VDOPROC_OUTID(out_id) >= videoproc_max_device_out) {
		HD_VIDEOPROC_ERR("Check output port error, port(%#x) max(%#x)\n", VDOPROC_OUTID(out_id), videoproc_max_device_out);
		ret = HD_ERR_NG;
		goto state_exit;
	}

	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if (vproc_param == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		ret = HD_ERR_NG;
		goto state_exit;
	}

	/* check and set state */
	if (vproc_param->priv.pull_state == HD_PATH_ID_CLOSED) {
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	} else if (vproc_param->priv.pull_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		vproc_param->priv.pull_state = HD_PATH_ID_ONGOING;
	}

	ret = videoproc_check_path_id_p(path_id);
	if (ret != HD_OK) {
		goto exit;
	}
	if (p_video_frame == NULL) {
		ret = HD_ERR_PARAM;
		goto exit;
	}
	if ((vproc_param->di_enable) && (vproc_param->priv.di_fd == 0)) {
		ret = HD_ERR_NOT_OPEN;
		goto exit;
	} else {
		if (vproc_param->priv.fd == 0) {
			ret = HD_ERR_NOT_OPEN;
			goto exit;
		}
	}

	ret = videoproc_pull_out_buf_p(path_id, p_video_frame, wait_ms);
	HD_TRIGGER_FLOW("hd_videoproc_pull_out_buf:\n");
	HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	HD_TRIGGER_FLOW("       out_buf_info: ddrid(%d) dim(%dx%d) pa(%p) p_next(%p)\n",
			p_video_frame->ddr_id, p_video_frame->dim.w,
			p_video_frame->dim.h, (VOID *)p_video_frame->phy_addr[0],
			p_video_frame->p_next);


	p_next_video_frame = (HD_VIDEO_FRAME *)(p_video_frame->p_next);
	while ((p_next_video_frame) &&
	       (videoproc_check_addr((UINTPTR)p_next_video_frame) == TRUE) &&
	       (p_next_video_frame->ddr_id < DDR_ID_MAX)) {
		HD_TRIGGER_FLOW("           ->p_next: ddrid(%d) dim(%dx%d) pa(%p)\n",
				p_next_video_frame->ddr_id,	p_next_video_frame->dim.w,
				p_next_video_frame->dim.h, (VOID *)p_next_video_frame->phy_addr[0]);
		p_next_video_frame = (HD_VIDEO_FRAME *)(p_next_video_frame->p_next);
	}


exit:
	vproc_param->priv.pull_state = HD_PATH_ID_OPEN;
	return ret;

state_exit:
	return ret;
}

HD_RESULT hd_videoproc_release_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);

	ret = videoproc_check_path_id_p(path_id);
	if (ret != HD_OK) {
		goto exit;
	}
	if (p_video_frame == NULL) {
		ret = HD_ERR_PARAM;
		goto exit;
	}
	HD_TRIGGER_FLOW("hd_videoproc_release_out_buf:\n");
	HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	HD_TRIGGER_FLOW("       ddrid(%d) pa(%p)\n", p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0]);
	ret = videoproc_release_out_buf_p(self_id, out_id, p_video_frame);

exit:
	return ret;
}

HD_RESULT hd_videoproc_poll_list(HD_VIDEOPROC_POLL_LIST *p_poll, UINT32 num, INT32 wait_ms)
{
	if (p_poll == NULL) {
		HD_VIDEOPROC_ERR(" The p_poll is NULL.\n");
		return HD_ERR_PARAM;
	}
	return video_poll_list((HD_VIDEOENC_POLL_LIST *)p_poll, num, wait_ms);
}

HD_RESULT hd_videoproc_send_list(HD_VIDEOPROC_SEND_LIST *p_videoproc_bs, UINT32 nr, INT32 wait_ms)
{
	if (p_videoproc_bs == NULL) {
		HD_VIDEOPROC_ERR(" The p_videoproc_bs is NULL.\n");
		return HD_ERR_PARAM;
	}

#if 0
	UINT i, dev_id;
	VDOPROC_INTL_PARAM *p_dev_cfg;
	HD_PATH_ID path_id;
	UINT32 out_subid;
	UINT32 start_time = current_timestamp32();

try_send:
	// wait until black bitstream is sent for all channels
	for (i = 0; i < nr; i++) {
		path_id = p_videoproc_bs[i].path_id;
		if (path_id == 0)
			continue;

		dev_id = HD_GET_DEV(path_id);
		out_subid = VDOPROC_OUTID(HD_GET_OUT(path_id));
		p_dev_cfg = videoproc_get_param_p(dev_id, out_subid);
		if (p_dev_cfg && p_dev_cfg->is_black_init == FALSE) {
			if (TIME_DIFF(current_timestamp32(), start_time) >= wait_ms) {
				return HD_ERR_TIMEDOUT;
			} else {
				usleep(1000);
				goto try_send;
			}
		}
	}
#endif
	return dif_video_send_yuv(p_videoproc_bs, nr, wait_ms);
}

HD_RESULT hd_videoproc_init(VOID)
{
	HD_RESULT ret;

	videoproc_max_device_nr = VDOPROC_DEV;
	videoproc_max_device_in = VODPROC_INPUT_PORT;
	videoproc_max_device_out = VODPROC_OUTPUT_PORT;

	ret = bd_device_init(HD_DAL_VIDEOPROC_BASE, videoproc_max_device_nr, videoproc_max_device_in, videoproc_max_device_out);
	if (ret != HD_OK) {
		HD_VIDEOPROC_ERR("bd_device_init VIDEOPROC error\n");
		goto exit;
	}
	ret = videoproc_init_p();
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("videoproc_init error\n");
	}

exit:
	HD_VIDEOPROC_FLOW("hd_videoproc_init\n");
	return ret;
}

HD_RESULT hd_videoproc_uninit(VOID)
{
	HD_RESULT ret;
	INT dev_idx, outport_idx;

	ret = bd_device_uninit(HD_DAL_VIDEOPROC_BASE);
	if (ret != HD_OK) {
		HD_VIDEOPROC_ERR("bd_device_uninit VIDEOPROC error\n");
		goto exit;
	}

	ret = videoproc_uninit_p();
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("videoproc_uninit error\n");
	}
	/* release vdoproc_param source */
	for (dev_idx = 0; dev_idx < VDOPROC_DEV; dev_idx++) {
		for (outport_idx = 0; outport_idx < VODPROC_OUTPUT_PORT; outport_idx++) {
			if (vdoproc_param[dev_idx][outport_idx] != NULL) {
				VDO_PROC_FREE(vdoproc_param[dev_idx][outport_idx]);
				vdoproc_param[dev_idx][outport_idx] = NULL;
			}
		}
	}

exit:
	HD_VIDEOPROC_FLOW("hd_videoproc_uninit: alloc_nr(%d)\n", vdo_proc_alloc_nr);
	return ret;
}


/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
