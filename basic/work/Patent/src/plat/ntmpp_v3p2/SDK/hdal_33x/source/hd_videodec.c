/**
	@brief Source file of video decode.\n
	This file contains the functions which is related to video decode in the chip.

	@file hd_videodec.c

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
#include "videodec.h"
#include "logger.h"
#include "vg_common.h"
#include "kflow_videodec.h"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define MSG_STR_LEN  256

/*-----------------------------------------------------------------------------*/
/* External Global Variables                                                   */
/*-----------------------------------------------------------------------------*/
unsigned long current_timestamp32(void);
extern KFLOW_VIDEODEC_HW_SPEC_INFO vdodec_hw_spec_info;

/*-----------------------------------------------------------------------------*/
/* Internal Global Variables                                                   */
/*-----------------------------------------------------------------------------*/
UINT32 videodec_max_device_nr;
UINT32 videodec_max_device_in;
UINT32 videodec_max_device_out;
VDODEC_PARAM vdodec_param[VDODEC_DEV];

/*-----------------------------------------------------------------------------*/
/* External Function Prototype                                                 */
/*-----------------------------------------------------------------------------*/
extern HD_RESULT _hd_common_mem_cache_sync_dma_from_device(void *virt_addr, unsigned int size);

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT hd_videodec_bind(HD_OUT_ID out_id, HD_IN_ID dest_in_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(out_id);
	HD_IO _out_id = HD_GET_OUT(out_id);
	HD_DAL dest_id = HD_GET_DEV(dest_in_id);
	HD_IO _in_id = HD_GET_IN(dest_in_id);

	ret = bd_bind(self_id, _out_id, dest_id, _in_id);
	HD_VIDEODEC_FLOW("hd_videodec_bind: %s_%d_OUT_%d -> %s_%d_IN_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(_out_id),
		get_device_name(bd_get_dev_baseid(dest_id)), bd_get_dev_subid(dest_id), BD_GET_IN_SUBID(_in_id));
	return ret;
}

HD_RESULT hd_videodec_unbind(HD_OUT_ID out_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(out_id);
	HD_IO _out_id = HD_GET_OUT(out_id);

	ret = bd_unbind(self_id, _out_id);
	HD_VIDEODEC_FLOW("hd_videodec_unbind: %s_%d_OUT_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(_out_id));
	return ret;
}

HD_RESULT hd_videodec_open(HD_IN_ID in_id, HD_OUT_ID out_id, HD_PATH_ID *p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(in_id);
	HD_IO _in_id = HD_GET_IN(in_id);
	HD_DAL self_id = HD_GET_DEV(out_id);
	HD_IO _out_id = HD_GET_OUT(out_id);
	HD_RESULT ret;
	INT dev_idx = VDODEC_DEVID(self_id);
	INT port_idx = BD_GET_OUT_SUBID(_out_id);

	if (p_path_id == NULL) {
		HD_VIDEODEC_ERR(" The p_path_id is NULL.\n");
		return HD_ERR_PARAM;
	}

	if (bd_get_dev_baseid(self_id) != HD_DAL_VIDEODEC_BASE) {
		HD_VIDEODEC_ERR(" The device of path_id(%#x) is not videodec.\n", GET_VDEC_PATHID(self_id, _in_id, _out_id));
		ret = HD_ERR_DEV;
		goto ext;
	}

	if (in_dev_id != self_id) {
		HD_VIDEODEC_ERR(" in(%#x) and out(%#x) aren't the same device.\n", in_id, out_id);
		ret = HD_ERR_IO;
		goto ext;
	}

	/* check and set state */
	if ((vdodec_param[dev_idx].port[port_idx].priv.push_state == HD_PATH_ID_OPEN) ||
	    (vdodec_param[dev_idx].port[port_idx].priv.pull_state == HD_PATH_ID_OPEN)) {
		if ((ret = bd_get_already_open_pathid(self_id, _out_id, _in_id, p_path_id)) != HD_OK) {
			HD_VIDEODEC_ERR("Check open_pathid fail, ret(%d).\n", ret);
			goto state_exit;
		} else {
			HD_VIDEODEC_ERR("The path_id(%#x) has already open.\n", *p_path_id);
			ret = HD_ERR_ALREADY_OPEN;
			goto state_exit;
		}
	} else if ((vdodec_param[dev_idx].port[port_idx].priv.push_state == HD_PATH_ID_ONGOING) ||
		   (vdodec_param[dev_idx].port[port_idx].priv.pull_state == HD_PATH_ID_ONGOING)) {
		HD_VIDEODEC_ERR("The state of path_id(%#x) is invalid, push_state(%d) pull_state(%d).\n",
				*p_path_id,
				vdodec_param[dev_idx].port[port_idx].priv.push_state,
				vdodec_param[dev_idx].port[port_idx].priv.pull_state);
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		vdodec_param[dev_idx].port[port_idx].priv.push_state = HD_PATH_ID_ONGOING;
		vdodec_param[dev_idx].port[port_idx].priv.pull_state = HD_PATH_ID_ONGOING;
	}

	if (HD_GET_CTRL(out_id) != HD_CTRL) {
		if (_in_id != _out_id) {
			HD_VIDEODEC_ERR("hd_videodec open fail, in_id(%d) out_id(%d) need to be same. Please correct it.\n",
					_in_id, _out_id);
			ret = HD_ERR_PARAM;
			goto ext;
		}
	}

	ret = bd_device_open(self_id, _out_id, _in_id, p_path_id);
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("bd_device_open fail\n");
		goto ext;
	}

	ret = videodec_init_path_p(*p_path_id);
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("videodec_init_path fail\n");
		goto ext;
	}

	HD_VIDEODEC_FLOW("hd_videodec_open:\n    %s path_id(%#x) fd(%#x) devid(%#x) out(%u) in(%u)\n",
			 dif_return_dev_string(*p_path_id), *p_path_id,
			 vdodec_param[dev_idx].port[port_idx].priv.fd,
			 self_id, _out_id, _in_id);
	vdodec_param[dev_idx].port[port_idx].priv.push_state = HD_PATH_ID_OPEN;
	vdodec_param[dev_idx].port[port_idx].priv.pull_state = HD_PATH_ID_OPEN;
	return ret;

ext:
	vdodec_param[dev_idx].port[port_idx].priv.push_state = HD_PATH_ID_CLOSED;
	vdodec_param[dev_idx].port[port_idx].priv.pull_state = HD_PATH_ID_CLOSED;
	return ret;

state_exit:
	return ret;
}

HD_RESULT hd_videodec_start(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	VDODEC_MAXMEM *vdodec_maxmem;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = VDODEC_DEVID(self_id);
	INT inport_idx = VDODEC_INID(in_id);

	ret = videodec_check_path_id_p(path_id);
	if (ret != HD_OK) {
		goto ext;
	}

	vdodec_maxmem = &vdodec_param[dev_idx].port[inport_idx].max_mem;

	ret = verify_param_PATH_CONFIG(vdodec_maxmem);
	if (ret < 0) {
		HD_VIDEODEC_ERR("Wrong value. id=%d\n", HD_VIDEODEC_PARAM_PATH_CONFIG);
		return HD_ERR_PARAM;
	}

	ret = bd_start_list(&path_id, 1);

ext:
	HD_VIDEODEC_FLOW("hd_videodec_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_videodec_start_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;
	HD_PATH_ID this_path_id;
	VDODEC_MAXMEM *vdodec_maxmem;
	HD_DAL self_id;
	HD_IO in_id;
	INT dev_idx, inport_idx;

	if (path_id == NULL) {
		HD_VIDEODEC_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}

	for (i = 0; i < nr; i++) {
		this_path_id = path_id[i];
		ret = videodec_check_path_id_p(this_path_id);
		HD_VIDEODEC_FLOW("hd_videodec_start:\n    %s path_id(%#x)\n", dif_return_dev_string(this_path_id), this_path_id);
		if (ret != HD_OK) {
			goto ext;
		}

		// check path config param
		self_id = HD_GET_DEV(this_path_id);
		in_id = HD_GET_IN(this_path_id);
		dev_idx = VDODEC_DEVID(self_id);
		inport_idx = VDODEC_INID(in_id);
		vdodec_maxmem = &vdodec_param[dev_idx].port[inport_idx].max_mem;
		ret = verify_param_PATH_CONFIG(vdodec_maxmem);
		if (ret < 0) {
			HD_VIDEODEC_ERR("Wrong value. id=%d\n", HD_VIDEODEC_PARAM_PATH_CONFIG);
			return HD_ERR_PARAM;
		}
	}

	ret = bd_start_list(path_id, nr);

ext:
	return ret;
}

HD_RESULT hd_videodec_stop(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	ret = videodec_check_path_id_p(path_id);
	if (ret != HD_OK)
		goto ext;
	ret = bd_stop_list(&path_id, 1);
ext:
	HD_VIDEODEC_FLOW("hd_videodec_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_videodec_stop_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;

	for (i = 0; i < nr; i++) {
		ret = videodec_check_path_id_p(path_id[i]);
		HD_VIDEODEC_FLOW("hd_videodec_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
		if (ret != HD_OK)
			goto ext;
	}

	ret = bd_stop_list(path_id, nr);

ext:
	return ret;
}

HD_RESULT hd_videodec_close(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = VDODEC_DEVID(self_id);
	INT port_idx = BD_GET_IN_SUBID(in_id);

	/* check and set state */
	if ((vdodec_param[dev_idx].port[port_idx].priv.push_state == HD_PATH_ID_CLOSED) ||
	    (vdodec_param[dev_idx].port[port_idx].priv.pull_state == HD_PATH_ID_CLOSED)) {
		HD_VIDEODEC_ERR("The path_id(%#x) has already closed.\n", path_id);
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	} else if ((vdodec_param[dev_idx].port[port_idx].priv.push_state == HD_PATH_ID_ONGOING) ||
		   (vdodec_param[dev_idx].port[port_idx].priv.pull_state == HD_PATH_ID_ONGOING)) {
		HD_VIDEODEC_ERR("The state of path_id(%#x) is invalid, push_state(%d) pull_state(%d).\n",
				path_id,
				vdodec_param[dev_idx].port[port_idx].priv.push_state,
				vdodec_param[dev_idx].port[port_idx].priv.pull_state);
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		vdodec_param[dev_idx].port[port_idx].priv.push_state = HD_PATH_ID_ONGOING;
		vdodec_param[dev_idx].port[port_idx].priv.pull_state = HD_PATH_ID_ONGOING;
	}

	ret = videodec_check_path_id_p(path_id);
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("Check path_id(%#x) fail, ret(%d).\n", path_id, ret);
		goto ext;
	}

	ret = videodec_close(path_id);
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("videodec_close fail(%d)\n", ret);
		goto ext;
	}

	ret = bd_device_close(path_id);
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("bd_device_close fail(%d)\n", ret);
		goto ext;
	}

	ret = videodec_uninit_path_p(path_id);
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("videodec_uninit_path_p fail(%d)\n", ret);
		goto ext;
	}

	HD_VIDEODEC_FLOW("hd_videodec_close:\n    %s path_id(%#x) fd(%#x)\n",
			 dif_return_dev_string(path_id), path_id, vdodec_param[dev_idx].port[port_idx].priv.fd);
	vdodec_param[dev_idx].port[port_idx].priv.push_state = HD_PATH_ID_CLOSED;
	vdodec_param[dev_idx].port[port_idx].priv.pull_state = HD_PATH_ID_CLOSED;
	return ret;

ext:
	vdodec_param[dev_idx].port[port_idx].priv.push_state = HD_PATH_ID_OPEN;
	vdodec_param[dev_idx].port[port_idx].priv.pull_state = HD_PATH_ID_OPEN;
	return ret;

state_exit:
	return ret;
}

HD_RESULT hd_videodec_get(HD_PATH_ID path_id, HD_VIDEODEC_PARAM_ID id, VOID *p_param)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 idx, dev;
	INT i;
	HD_VIDEODEC_PATH_CONFIG *p_config;
	HD_VIDEODEC_STATUS *p_status;

	ret = videodec_check_path_id_p(path_id);
	if (ret != HD_OK)
		goto exit;

	if (p_param == NULL) {
		HD_VIDEODEC_ERR("NULL p_param!\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	dev = VDODEC_DEVID(self_id);
	idx = VDODEC_OUTID(out_id);

	switch (id) {
	case HD_VIDEODEC_PARAM_DEVCOUNT: {
		HD_DEVCOUNT *p_dev_count = (HD_DEVCOUNT *) p_param;
		p_dev_count->max_dev_count = 1;
		HD_VIDEODEC_FLOW("hd_videodec_get(DEVCOUNT):\n"
				 "    %s path_id(%#x)\n"
				 "        max_count(%u)\n", dif_return_dev_string(path_id), path_id, p_dev_count->max_dev_count);
		break;
	}
	case HD_VIDEODEC_PARAM_SYSCAPS: {
		HD_VIDEODEC_SYSCAPS *p_syscaps = (HD_VIDEODEC_SYSCAPS *)p_param;
		p_syscaps->dev_id = 0;
		p_syscaps->chip = 0;
		p_syscaps->max_in_count = videodec_max_device_in;
		p_syscaps->max_out_count = videodec_max_device_out;
		p_syscaps->dev_caps = HD_CAPS_PATHCONFIG
				      | HD_CAPS_LISTFUNC;
		for (i = 0; i < HD_VIDEODEC_MAX_IN; i++) {
			p_syscaps->in_caps[i] = HD_VIDEOENC_CAPS_JPEG
						| HD_VIDEOENC_CAPS_H264
						| HD_VIDEOENC_CAPS_H265;
		}
		for (i = 0; i < HD_VIDEODEC_MAX_OUT; i++) {
			p_syscaps->out_caps[i] = HD_VIDEO_CAPS_YUV420;
		}
		p_syscaps->max_dim.w = vdodec_hw_spec_info.h265.max_dim.w;
		p_syscaps->max_dim.h = vdodec_hw_spec_info.h265.max_dim.h;
		break;
	}
	case HD_VIDEODEC_PARAM_PATH_CONFIG:
		p_config = (HD_VIDEODEC_PATH_CONFIG *)p_param;
		memset(p_config, 0x0, sizeof(HD_VIDEODEC_PATH_CONFIG));
		p_config->max_mem.dim.w = vdodec_param[dev].port[idx].max_mem.max_width;
		p_config->max_mem.dim.h = vdodec_param[dev].port[idx].max_mem.max_height;
		p_config->max_mem.frame_rate = vdodec_param[dev].port[idx].max_mem.max_fps;
		p_config->max_mem.max_bitrate = vdodec_param[dev].port[idx].max_mem.max_bitrate;
		p_config->max_mem.bs_counts = vdodec_param[dev].port[idx].max_mem.bs_counts;
		p_config->max_mem.codec_type = vdodec_param[dev].port[idx].max_mem.codec_type;
		p_config->max_mem.max_ref_num = vdodec_param[dev].port[idx].max_mem.max_ref_num;
		p_config->max_mem.ddr_id = vdodec_param[dev].port[idx].max_mem.ddr_id;
		p_config->max_mem.max_bs_size = vdodec_param[dev].port[idx].max_mem.max_bs_size;
		HD_VIDEODEC_FLOW("hd_videodec_get(PATH_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        max_dim(%u,%u) max_fps(%u) max_br(%u) bs_count(%u) codec(%#x) ref_num(%u) ddr(%d) max_bs_size(%u)\n",
				 dif_return_dev_string(path_id), path_id, p_config->max_mem.dim.w,
				 p_config->max_mem.dim.h,
				 p_config->max_mem.frame_rate,
				 p_config->max_mem.max_bitrate,
				 p_config->max_mem.bs_counts,
				 p_config->max_mem.codec_type,
				 p_config->max_mem.max_ref_num,
				 p_config->max_mem.ddr_id,
				 p_config->max_mem.max_bs_size);

		for (i = 0; i < HD_VIDEODEC_MAX_DATA_TYPE; i++) {
			p_config->data_pool[i].ddr_id = vdodec_param[dev].port[idx].out_pool.buf_info[i].ddr_id;
			p_config->data_pool[i].counts = vdodec_param[dev].port[idx].out_pool.buf_info[i].counts;
			p_config->data_pool[i].max_counts = vdodec_param[dev].port[idx].out_pool.buf_info[i].max_counts;
			p_config->data_pool[i].min_counts = vdodec_param[dev].port[idx].out_pool.buf_info[i].min_counts;
			p_config->data_pool[i].mode = vdodec_param[dev].port[idx].out_pool.buf_info[i].enable;
			HD_VIDEODEC_FLOW("        idx(%d) ddr(%u) count(%u) max/min(%u,%u) mode(%#x)\n",
					 i, p_config->data_pool[i].ddr_id,
					 p_config->data_pool[i].counts,
					 p_config->data_pool[i].max_counts,
					 p_config->data_pool[i].min_counts,
					 p_config->data_pool[i].mode);
		}
		break;
	case HD_VIDEODEC_PARAM_STATUS:
		p_status = (HD_VIDEODEC_STATUS *)p_param;
		memset(p_status, 0x0, sizeof(HD_VIDEODEC_STATUS));
		videodec_get_status_p(self_id, out_id, &(vdodec_param[dev].port[idx].status));
		p_status->left_frames = vdodec_param[dev].port[idx].status.left_frames;
		p_status->reserved_ref_frame = vdodec_param[dev].port[idx].status.reserved_ref_frame;
		p_status->done_frames = vdodec_param[dev].port[idx].status.done_frames;
		p_status->reserved[0] = vdodec_param[dev].port[idx].status.dti_buf_cnt;
		HD_VIDEODEC_FLOW("hd_videodec_get(STATUS):\n"
				 "    %s path_id(%#x) fd(%#x)\n"
				 "        left_frames(%u) reserved_ref_frame(%u) done_frames(%u) dti_buf_cnt(%u)\n",
				 dif_return_dev_string(path_id), path_id,
				 vdodec_param[dev].port[idx].priv.fd,
				 p_status->left_frames,
				 p_status->reserved_ref_frame,
				 p_status->done_frames,
				 p_status->reserved[0]);
		break;
	default:
		HD_VIDEODEC_ERR("Not support to set param id(%d)\n", id);
		ret = HD_ERR_NG;
		goto exit;
	}

exit:
	return ret;
}

HD_RESULT hd_videodec_set(HD_PATH_ID path_id, HD_VIDEODEC_PARAM_ID id, VOID *p_param)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 idx, dev;
	INT i;
	HD_VIDEODEC_PATH_CONFIG *p_config;
	HD_VIDEODEC_STATUS *p_status;
	VDODEC_INTL_PARAM *p_dec_param;

	ret = videodec_check_path_id_p(path_id);
	if (ret != HD_OK)
		goto exit;

	if (p_param == NULL) {
		HD_VIDEODEC_ERR("NULL p_param!\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	dev = VDODEC_DEVID(self_id);
	idx = VDODEC_OUTID(out_id);

	switch (id) {
	case HD_VIDEODEC_PARAM_SYSCAPS:
		HD_VIDEODEC_ERR("Not support to set system information.\n");
		ret = HD_ERR_NG;
		goto exit;
	case HD_VIDEODEC_PARAM_PATH_CONFIG:
		p_config = (HD_VIDEODEC_PATH_CONFIG *)p_param;
		if (validate_dec_in_dim(p_config->max_mem.dim, p_config->max_mem.codec_type) == FALSE) {
			ret = HD_ERR_LIMIT;
			goto exit;
		}
		vdodec_param[dev].port[idx].max_mem.max_width = p_config->max_mem.dim.w;
		vdodec_param[dev].port[idx].max_mem.max_height = p_config->max_mem.dim.h;
		vdodec_param[dev].port[idx].max_mem.max_fps = p_config->max_mem.frame_rate;
		vdodec_param[dev].port[idx].max_mem.max_bitrate = p_config->max_mem.max_bitrate;
		vdodec_param[dev].port[idx].max_mem.bs_counts = p_config->max_mem.bs_counts;
		vdodec_param[dev].port[idx].max_mem.codec_type = p_config->max_mem.codec_type;
		if (p_config->max_mem.max_ref_num < 1) {
			HD_VIDEODEC_ERR("Error to set max_ref_num = %d, fixed to 1.\n", p_config->max_mem.max_ref_num);
			vdodec_param[dev].port[idx].max_mem.max_ref_num = 1;
		} else if (p_config->max_mem.max_ref_num > 16) {
			HD_VIDEODEC_ERR("Error to set max_ref_num = %d, fixed to 1.\n", p_config->max_mem.max_ref_num);
			vdodec_param[dev].port[idx].max_mem.max_ref_num = 1;
		} else {
			vdodec_param[dev].port[idx].max_mem.max_ref_num = p_config->max_mem.max_ref_num;
		}
		vdodec_param[dev].port[idx].max_mem.ddr_id = p_config->max_mem.ddr_id;
		vdodec_param[dev].port[idx].max_mem.max_bs_size = p_config->max_mem.max_bs_size;

		HD_VIDEODEC_FLOW("hd_videodec_set(PATH_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        max_dim(%u,%u) max_fps(%u) max_br(%u) bs_count(%u) codec(%#x) ref_num(%u) ddr(%d) max_bs_size(%u)\n",
				 dif_return_dev_string(path_id), path_id, p_config->max_mem.dim.w,
				 p_config->max_mem.dim.h,
				 p_config->max_mem.frame_rate,
				 p_config->max_mem.max_bitrate,
				 p_config->max_mem.bs_counts,
				 p_config->max_mem.codec_type,
				 p_config->max_mem.max_ref_num,
				 p_config->max_mem.ddr_id,
				 p_config->max_mem.max_bs_size);

		for (i = 0; i < HD_VIDEODEC_MAX_DATA_TYPE; i++) {
			vdodec_param[dev].port[idx].out_pool.buf_info[i].ddr_id = p_config->data_pool[i].ddr_id;
			vdodec_param[dev].port[idx].out_pool.buf_info[i].counts = p_config->data_pool[i].counts;
#if 0 // use counts and max_ref_cnt to calculate in videodec_convert
			vdodec_param[dev].port[idx].out_pool.buf_info[i].max_counts = p_config->data_pool[i].max_counts;
			vdodec_param[dev].port[idx].out_pool.buf_info[i].min_counts = p_config->data_pool[i].min_counts;
#endif
			vdodec_param[dev].port[idx].out_pool.buf_info[i].enable = p_config->data_pool[i].mode;
			HD_VIDEODEC_FLOW("        idx(%d) ddr(%u) count(%u) max/min(%u,%u) mode(%#x)\n",
					 i, p_config->data_pool[i].ddr_id,
					 p_config->data_pool[i].counts,
					 p_config->data_pool[i].max_counts,
					 p_config->data_pool[i].min_counts,
					 p_config->data_pool[i].mode);
		}

		ret = videodec_set_param_p(self_id, out_id);
		if (ret != HD_OK) {
			HD_VIDEODEC_ERR("set parameter error\n");
		}
		break;
	case HD_VIDEODEC_PARAM_STATUS:
		p_status = (HD_VIDEODEC_STATUS *)p_param;
		vdodec_param[dev].port[idx].status.reserved_ref_frame = p_status->reserved_ref_frame;
		p_dec_param = videodec_get_param_p(self_id, idx);
		if (p_dec_param == NULL) {
			HD_VIDEODEC_ERR("internal_err, get dec_param err! deviceid(%#x) idx(%d)\n", self_id, idx);
			goto exit;
		}
		p_dec_param->reserved_ref_flag = p_status->reserved_ref_frame;
		videodec_set_status_p(self_id, out_id, &(vdodec_param[dev].port[idx].status));
		HD_VIDEODEC_FLOW("hd_videodec_set(STATUS):\n"
				 "    %s path_id(%#x)\n"
				 "        reserved_ref_frame(%u) reserved_ref_flag(%u)\n",
				 dif_return_dev_string(path_id), path_id, p_status->reserved_ref_frame, p_dec_param->reserved_ref_flag);
		break;
	default:
		HD_VIDEODEC_ERR("Not support to set param id(%d)\n", id);
		ret = HD_ERR_NG;
		goto exit;
	}

exit:
	return ret;
}

static char bind_src_name[64] = "-";
HD_RESULT hd_videodec_push_in_buf(HD_PATH_ID path_id, HD_VIDEODEC_BS *p_video_bitstream,
				  HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	char *sign = NULL;
	INT dev_idx = VDODEC_DEVID(self_id);
	INT inport_idx = VDODEC_INID(in_id);
	HD_VIDEODEC_SEND_LIST send_list = {0};
	VOID *send_list_va = 0;

	/* check and set state */
	if (vdodec_param[dev_idx].port[inport_idx].priv.push_state == HD_PATH_ID_CLOSED) {
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	} else if (vdodec_param[dev_idx].port[inport_idx].priv.push_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		vdodec_param[dev_idx].port[inport_idx].priv.push_state = HD_PATH_ID_ONGOING;
	}

	ret = videodec_check_path_id_p(path_id);
	if (ret != HD_OK) {
		goto exit;
	}
	if (p_video_bitstream == NULL) {
		ret = HD_ERR_PARAM;
		HD_VIDEODEC_ERR("p_video_bitstream is NULL\n");
		goto exit;
	}

	ret = dif_check_flow_mode(self_id, out_id);
	if (ret == HD_ERR_ALREADY_BIND) {
		send_list_va = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_NONCACHE,
						  p_video_bitstream->phy_addr,
						  p_video_bitstream->size);
		if (send_list_va == NULL) {
			goto exit;
		}
		send_list.path_id = path_id;
		send_list.user_bs.p_bs_buf = (CHAR *)send_list_va;
		send_list.user_bs.bs_buf_size = p_video_bitstream->size;
		send_list.user_bs.time_align = HD_VIDEODEC_TIME_ALIGN_DISABLE;
		send_list.user_bs.time_diff = 0;
		if ((ret = hd_videodec_send_list(&send_list, 1, wait_ms)) != HD_OK) {
			goto exit;
		}
	} else if (ret == HD_ERR_NOT_AVAIL) {
		if (p_user_out_video_frame == NULL) {
			ret = HD_ERR_PARAM;
			HD_VIDEODEC_ERR("p_user_out_video_frame is NULL\n");
			goto exit;
		}

		if (vdodec_param[dev_idx].port[inport_idx].priv.fd == 0) {
			ret = HD_ERR_NOT_OPEN;
			goto exit;
		}

		if ((vdodec_param[dev_idx].port[inport_idx].push_in_used == 1) ||
		    (bd_get_prev(self_id, in_id, bind_src_name, sizeof(bind_src_name)) == HD_OK)) {
			HD_VIDEODEC_ERR("Videodec path(0x%x) is already used(%d). in_id(%d) src(%s)\n",
					(int)path_id, vdodec_param[dev_idx].port[inport_idx].push_in_used, (int)in_id, bind_src_name);
			system("echo videodec all > /proc/hdal/setting");
			system("cat /proc/hdal/setting");
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}

		vdodec_param[dev_idx].port[inport_idx].push_in_used = 1;

		HD_TRIGGER_FLOW("hd_videodec_push_in_buf:\n");
		HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		HD_TRIGGER_FLOW("       in_buf_info:ddrid(%d) pa(%p) buf_sz(%d)\n",
				p_video_bitstream->ddr_id, (VOID *)p_video_bitstream->phy_addr, p_video_bitstream->size);
		if (p_user_out_video_frame->sign) {
			sign = (char *)&p_user_out_video_frame->sign;
			HD_TRIGGER_FLOW("       out_buf_info:sign(%C%C%C%C) ddrid(%d) pa(%p)\n", sign[0],
					sign[1], sign[2], sign[3], p_user_out_video_frame->ddr_id,
					(VOID *)p_user_out_video_frame->phy_addr[0]);
		} else {
			HD_TRIGGER_FLOW("       out_buf_info:ddrid(%d) pa(%p)\n",
					p_user_out_video_frame->ddr_id, (VOID *)p_user_out_video_frame->phy_addr[0]);
		}
		p_user_out_video_frame->reserved[0] = path_id;
		ret = videodec_push_in_buf_p(self_id, out_id, p_video_bitstream, p_user_out_video_frame, wait_ms);
		if (ret == HD_OK) {
			vdodec_param[dev_idx].port[inport_idx].push_in_used = 0;
		}
	} else {
		HD_VIDEODEC_ERR("hd_videodec_push_in_buf fail. ret(%#x)\n", ret);
		goto exit;
	}

exit:
	if (send_list_va) {
		if (hd_common_mem_munmap(send_list_va, p_video_bitstream->size) != HD_OK) {
			HD_VIDEODEC_ERR("munmap fail.\n");
		}
	}
	vdodec_param[dev_idx].port[inport_idx].priv.push_state = HD_PATH_ID_OPEN;
	return ret;

state_exit:
	return ret;
}


HD_RESULT hd_videodec_pull_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	char *sign = NULL;
	INT dev_idx = VDODEC_DEVID(self_id);
	INT outport_idx = VDODEC_OUTID(in_id);

	/* check and set state */
	if (vdodec_param[dev_idx].port[outport_idx].priv.pull_state == HD_PATH_ID_CLOSED) {
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	} else if (vdodec_param[dev_idx].port[outport_idx].priv.pull_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		vdodec_param[dev_idx].port[outport_idx].priv.pull_state = HD_PATH_ID_ONGOING;
	}

	ret = videodec_check_path_id_p(path_id);
	if (ret != HD_OK)
		goto exit;

	if (p_video_frame == NULL) {
		ret = HD_ERR_PARAM;
		goto exit;
	}
	if (vdodec_param[dev_idx].port[outport_idx].priv.fd == 0) {
		ret = HD_ERR_NOT_OPEN;
		goto exit;
	}

	ret = videodec_pull_out_buf_p(self_id, out_id, p_video_frame, wait_ms);
	HD_TRIGGER_FLOW("hd_videodec_pull_out_buf:\n");
	HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	if (p_video_frame->sign) {
		sign = (char *)&p_video_frame->sign;
		HD_TRIGGER_FLOW("       out_buf_info:sign(%C%C%C%C) dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", \
				sign[0], sign[1], sign[2], sign[3], p_video_frame->dim.w, p_video_frame->dim.h, p_video_frame->ddr_id, \
				(VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
	} else {
		HD_TRIGGER_FLOW("       out_buf_info:dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", \
				p_video_frame->dim.w, p_video_frame->dim.h, p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0], \
				p_video_frame->pxlfmt);
	}

exit:
	vdodec_param[dev_idx].port[outport_idx].priv.pull_state = HD_PATH_ID_OPEN;
	return ret;

state_exit:
	return ret;
}

HD_RESULT hd_videodec_release_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);

	ret = videodec_check_path_id_p(path_id);
	if (ret != HD_OK)
		goto exit;

	if (p_video_frame == NULL) {
		ret = HD_ERR_PARAM;
		goto exit;
	}
	HD_TRIGGER_FLOW("hd_videodec_release_out_buf:\n");
	HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	HD_TRIGGER_FLOW("       ddrid(%d) pa(%p)\n", p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0]);
	ret = videodec_release_out_buf_p(self_id, out_id, p_video_frame);

exit:
	return ret;
}

/*
  HD_VIDEODEC_SEND_LIST, user_flag
  user_flag bit0: user callback control, 0(normal, put to next) 1(free after callback)
  NOTE: refer to em property ID ID_FLOW_CONTROL
 */
#define HD_VIDEODEC_USER_FLAG_MASK 0xfffffffe // user_flag bit0

HD_RESULT hd_videodec_send_list(HD_VIDEODEC_SEND_LIST *p_videodec_bs, UINT32 nr, INT32 wait_ms)
{
	UINT i;
	HD_RESULT ret = HD_OK;
	HD_PATH_ID path_id;
	HD_DAL self_id;
	HD_IO out_id;
	UINT32 idx;
	VDODEC_INTL_PARAM *p_dec_param;

	for (i = 0; i < nr; i++) {
		path_id = p_videodec_bs[i].path_id;
		if (path_id == 0)
			continue;
		ret = videodec_check_path_id_p(path_id);
		if (ret != HD_OK) {
			return HD_ERR_PATH;
		}
		self_id = HD_GET_DEV(path_id);
		out_id = HD_GET_OUT(path_id);
		idx = VDODEC_OUTID(out_id);

		// user_flag : assigned from user AP, available only bit0.
		// user_flag bit0: user callback control, 0(normal, put to next) 1(free after callback)
		p_videodec_bs[i].user_bs.user_flag &= ~HD_VIDEODEC_USER_FLAG_MASK;

		// user_flag : assigned by hdal lib
		// user_flag bit2: jpeg shift flow control, 0(no shift checking) 1(do shift checking)
		p_dec_param = videodec_get_param_p(self_id, idx);
		if (p_dec_param->max_mem->codec_type == HD_CODEC_TYPE_JPEG) {
			p_videodec_bs[i].user_bs.user_flag |= DIF_JPEG_SHIFT_BIT_2;
		}
	}

#if SEND_DEC_BLACK_BS
try_send: {
		UINT dev_id;
		VDODEC_INTL_PARAM *p_dev_cfg;
		UINT32 out_subid;
		unsigned long start_time = current_timestamp32();

		// wait until black bitstream is sent for all channels
		for (i = 0; i < nr; i++) {
			path_id = p_videodec_bs[i].path_id;
			if (path_id == 0)
				continue;

			dev_id = HD_GET_DEV(path_id);
			out_subid = VDODEC_OUTID(HD_GET_OUT(path_id));
			p_dev_cfg = videodec_get_param_p(dev_id, out_subid);
			if (p_dev_cfg->tx_state == DEC_TX_NOTHING_DONE) {
				HD_VIDEODEC_FLOW("%s:dev_id(%#x_%d) tx_state(%d)\n", __func__, dev_id, out_subid, p_dev_cfg->tx_state);
				if (TIME_DIFF(current_timestamp32(), start_time) >= wait_ms) {
					/* do the error checking */
					ret = check_video_send_bs(p_videodec_bs, nr);
					if (ret != HD_OK) {
						return ret;
					} else {
						HD_VIDEODEC_ERR("wait DEC_TX_BLACK_BS_DONE fail!\n");
						return HD_ERR_TIMEDOUT;
					}
				} else {
					usleep(1000);
					goto try_send;
				}
			}
			p_dev_cfg->tx_state = DEC_TX_USR_BS_DONE;
		}
	}
#endif

	return video_send_list(p_videodec_bs, nr, wait_ms);
}

HD_RESULT hd_videodec_init(VOID)
{
	HD_RESULT ret;

	memset(&vdodec_param, 0x0, sizeof(VDODEC_PARAM) * VDODEC_DEV);
	videodec_max_device_nr = VDODEC_DEV;
	videodec_max_device_in = VDODEC_INPUT_PORT;
	videodec_max_device_out = VDODEC_OUTPUT_PORT;
	ret = bd_device_init(HD_DAL_VIDEODEC_BASE, videodec_max_device_nr, videodec_max_device_in, videodec_max_device_out);
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("device_init error\n");
		goto exit;
	}
	ret = videodec_init_p();
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("videodec_init error\n");
	}

exit:
	HD_VIDEODEC_FLOW("hd_videodec_init\n");
	return ret;
}

HD_RESULT hd_videodec_uninit(VOID)
{
	HD_RESULT ret;

	ret = bd_device_uninit(HD_DAL_VIDEODEC_BASE);
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("bd_device_uninit error\n");
		goto exit;
	}
	ret = videodec_uninit_p();
	if (ret != HD_OK) {
		HD_VIDEODEC_ERR("videodec_uninit error\n");
	}

exit:
	HD_VIDEODEC_FLOW("hd_videodec_uninit\n");
	return ret;
}

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
