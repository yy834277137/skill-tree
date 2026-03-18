/**
	@brief video capture module.\n
	This file contains the functions which is related to video capture in the chip.

	@file videocapture.c

	@ingroup hdal

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
#include "hd_type.h"
#include "bind.h"
#include "dif.h"
#include "videocap.h"
#include "logger.h"
#include "vcap316/vcap_ioctl.h"
#include "common_def.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct {
	unsigned int pull_get;
} HD_VIDEOCAP_INOUT_CTRL;

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern CAP_INTERNAL_PARAM cap_dev_cfg[MAX_CAP_DEV];
extern vpd_sys_info_t platform_sys_Info;
extern struct vcap_ioc_hw_ability_t videocap_hw_ability;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
extern int pif_env_update(void);
/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
UINT32 videocap_max_device_nr;
UINT32 videocap_max_device_out;
UINT32 videocap_max_device_in;
HD_CAP_INFO g_video_cap_info[MAX_CAP_DEV];
HD_VIDEOCAP_INOUT_CTRL g_video_cap_inout_ctrl[MAX_CAP_DEV] = {0};
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT hd_videocap_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);

	ret = bd_bind(self_id, out_id, dest_id, in_id);
	HD_VIDEOCAP_FLOW("hd_videocap_bind: %s_%d_OUT_%d -> %s_%d_IN_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(out_id),
		get_device_name(bd_get_dev_baseid(dest_id)), bd_get_dev_subid(dest_id), BD_GET_IN_SUBID(in_id));
	return ret;
}

HD_RESULT hd_videocap_unbind(HD_OUT_ID _out_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);

	ret = bd_unbind(self_id, out_id);
	HD_VIDEOCAP_FLOW("hd_videocap_unbind: %s_%d_OUT_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(out_id));
	return ret;
}

HD_RESULT hd_videocap_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID *p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	HD_RESULT ret;
	VDOCAP_PRIV *p_cap_priv;
	UINT32 dev_idx = VDOCAP_DEVID(self_id);

	if (bd_get_dev_baseid(self_id) != HD_DAL_VIDEOCAP_BASE) {
		HD_VIDEOCAPTURE_ERR(" The device(%#x) of path_id(%#x) is not videocapture.\n", self_id, *p_path_id);
		ret = HD_ERR_DEV;
		goto ext;
	}

	if ((ctrl_id != HD_CTRL) && (in_dev_id != self_id)) {
		HD_VIDEOCAPTURE_ERR(" in(%#x) and out(%#x) aren't the same device.\n", _in_id, _out_id);
		ret = HD_ERR_IO;
		goto ext;
	} else if (ctrl_id == HD_CTRL) {
		ret = videocap_open(_in_id, ctrl_id, p_path_id);
		HD_VIDEOCAP_FLOW("hd_videocap_openctrl:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u)\n",
				 dif_return_dev_string(*p_path_id), *p_path_id, self_id, HD_GET_OUT(_out_id), HD_GET_IN(_in_id));
		return HD_OK;
	} else if (HD_GET_OUT(_out_id) > MAX_CAP_PORT) {
		ret = videocap_open(_in_id, _out_id, p_path_id);
		HD_VIDEOCAP_FLOW("hd_videocap_openss:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u)\n",
				 dif_return_dev_string(*p_path_id), *p_path_id, self_id, HD_GET_OUT(_out_id), HD_GET_IN(_in_id));
		return ret;
	}
	/* check and set state */

	p_cap_priv = &g_video_cap_info[dev_idx].priv[VDOCAP_OUTID(HD_GET_OUT(_out_id))];
	if ((p_cap_priv->push_state == HD_PATH_ID_OPEN) ||
	    (p_cap_priv->pull_state == HD_PATH_ID_OPEN)) {
		if ((ret = bd_get_already_open_pathid(self_id, HD_GET_OUT(_out_id), HD_GET_IN(_in_id), p_path_id)) != HD_OK) {
			goto state_exit;
		} else {
			ret = HD_ERR_ALREADY_OPEN;
			goto state_exit;
		}
	} else if (p_cap_priv->push_state == HD_PATH_ID_ONGOING ||
		   p_cap_priv->pull_state == HD_PATH_ID_ONGOING) {
		//HD_VIDEOCAPTURE_ERR(" in(%#x) and out(%#x) busy.\n", _in_id, _out_id);
		//HD_VIDEOCAPTURE_ERR(" out_id(%#x) is busy. state(%d,%d)\n",_out_id, p_cap_priv->push_state, p_cap_priv->pull_state);
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		p_cap_priv->push_state = HD_PATH_ID_ONGOING;
		p_cap_priv->pull_state = HD_PATH_ID_ONGOING;
	}
	ret = videocap_open(_in_id, _out_id, p_path_id);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR(" in(%#x) and out(%#x) open fails.\n", _in_id, _out_id);
		goto ext;
	}
	p_cap_priv->push_state = HD_PATH_ID_OPEN;
	p_cap_priv->pull_state = HD_PATH_ID_OPEN;
	if (p_cap_priv->path_init_count == 0) {
		ret = videocap_user_proc_init(*p_path_id);
		if (ret != HD_OK) {
			goto state_exit;
		}
		p_cap_priv->path_init_count++;
	}
	HD_VIDEOCAP_FLOW("hd_videocap_open:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u)\n",
			 dif_return_dev_string(*p_path_id), *p_path_id, self_id, HD_GET_OUT(_out_id), HD_GET_IN(_in_id));
	return ret;
ext:
	p_cap_priv = &g_video_cap_info[dev_idx].priv[VDOCAP_OUTID(HD_GET_OUT(_out_id))];
	if (p_cap_priv) {
		p_cap_priv->push_state = HD_PATH_ID_CLOSED;
		p_cap_priv->pull_state = HD_PATH_ID_CLOSED;
	}
	return ret;
state_exit:
	return ret;
}

HD_RESULT hd_videocap_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_RESULT ret;
	INT out_idx = VDOCAP_OUTID(HD_GET_OUT(path_id));
	VDOCAP_PRIV *p_cap_priv;
	UINT32 dev_idx = VDOCAP_DEVID(self_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);

	if (bd_get_dev_baseid(self_id) != HD_DAL_VIDEOCAP_BASE) {
		HD_VIDEOCAPTURE_ERR(" The device of path_id(%#x) is not videocapture.\n", path_id);
		ret = HD_ERR_DEV;
		goto state_exit;
	}
	if (ctrl_id == HD_CTRL) {
		ret = videocap_close_fakepath(path_id);
		HD_VIDEOCAP_FLOW("hd_videocap_CLOSE:\n    %s path_id(%#x) \n",
				 dif_return_dev_string(path_id), path_id);
		return HD_OK;
	} else if (out_idx >= MAX_CAP_PORT) {
		ret = videocap_close_fakepath(path_id);
		HD_VIDEOCAP_FLOW("hd_videocap_CLOSE:\n    %s path_id(%#x) \n",
				 dif_return_dev_string(path_id), path_id);
		return ret;
	}
	/* check and set state */
	p_cap_priv = &g_video_cap_info[dev_idx].priv[out_idx];
	if ((p_cap_priv->push_state == HD_PATH_ID_CLOSED) ||
	    (p_cap_priv->pull_state == HD_PATH_ID_CLOSED)) {
		ret = HD_ERR_NOT_OPEN;
		printf("Fail to close, path_id(%#x) is closed.(%d, %d)  state(%d,%d)\n", path_id, dev_idx, out_idx,
		       p_cap_priv->push_state, p_cap_priv->pull_state);
		goto state_exit;
	} else if (p_cap_priv->push_state == HD_PATH_ID_ONGOING ||
		   p_cap_priv->pull_state == HD_PATH_ID_ONGOING) {
		printf("Fail to close, path_id(%#x) is busy.  state(%d,%d)\n", path_id,
		       p_cap_priv->push_state, p_cap_priv->pull_state);
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		p_cap_priv->push_state = HD_PATH_ID_ONGOING;
		p_cap_priv->pull_state = HD_PATH_ID_ONGOING;
	}

	if (HD_GET_OUT(path_id) >= (HD_MASK_BASE + 1) && HD_GET_OUT(path_id) <= (HD_MASK_MAX + 1)) //mask
		ret = videocap_close_mask(path_id);
	else
		ret = videocap_close(path_id);

	if (ret != HD_OK) {
		goto exit;
	}
	p_cap_priv->push_state = HD_PATH_ID_CLOSED;
	p_cap_priv->pull_state = HD_PATH_ID_CLOSED;
	p_cap_priv->path_init_count --;
	if (p_cap_priv->path_init_count == 0) {
		videocap_user_proc_uninit(path_id);
	}

	HD_VIDEOCAP_FLOW("hd_videocap_CLOSE:\n    %s path_id(%#x) \n",
			 dif_return_dev_string(path_id), path_id);

	return ret;

exit:
	p_cap_priv = &g_video_cap_info[dev_idx].priv[out_idx];
	if (p_cap_priv) {
		p_cap_priv->push_state = HD_PATH_ID_OPEN;
		p_cap_priv->pull_state = HD_PATH_ID_OPEN;
	}
	return ret;
state_exit:
	return ret;
}

HD_RESULT hd_videocap_init(VOID)
{
	HD_RESULT ret;

	if ((ret = videocap_init()) != HD_OK) {
		HD_VIDEOCAPTURE_ERR("videocap_init error\n");
	}
	HD_VIDEOCAP_FLOW("hd_videocap_init\n");
	return ret;
}

HD_RESULT hd_videocap_uninit(VOID)
{
	HD_RESULT ret;

	ret = videocap_uninit();
	HD_VIDEOCAP_FLOW("hd_videocap_uninit\n");
	return ret;
}

HD_RESULT hd_videocap_start(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	if (path_id == VCAP_CTRL_FAKE_PATHID) {
		HD_VIDEOCAP_FLOW("hd_videocap_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		return HD_OK;
	}

	if (HD_GET_OUT(path_id) >= (HD_MASK_BASE + 1) && HD_GET_OUT(path_id) <= (HD_MASK_MAX + 1)) //mask
		ret = videocap_start_mask(path_id);
	else
		ret = bd_start_list(&path_id, 1);
	HD_VIDEOCAP_FLOW("hd_videocap_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_videocap_start_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;
	if (*path_id == VCAP_CTRL_FAKE_PATHID) {
		HD_VIDEOCAP_FLOW("hd_videocap_startlist:\n    path_id(%#x)\n", *path_id);
		return HD_OK;
	}

	ret = bd_start_list(path_id, nr);
	for (i = 0; i < nr; i++) {
		HD_VIDEOCAP_FLOW("hd_videocap_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
	}
	return ret;
}

HD_RESULT hd_videocap_stop(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	if (path_id == VCAP_CTRL_FAKE_PATHID) {
		HD_VIDEOCAP_FLOW("hd_videocap_stop:\n   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		return HD_OK;
	}

	if (HD_GET_OUT(path_id) >= (HD_MASK_BASE + 1) && HD_GET_OUT(path_id) <= (HD_MASK_MAX + 1)) //mask
		ret = videocap_stop_mask(path_id);
	else
		ret = bd_stop_list(&path_id, 1);
	HD_VIDEOCAP_FLOW("hd_videocap_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_videocap_stop_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;
	if (*path_id == VCAP_CTRL_FAKE_PATHID) {
		HD_VIDEOCAP_FLOW("hd_videocap_stop_list:\n    path_id(%#x)\n", *path_id);
		return HD_OK;
	}

	ret = bd_stop_list(path_id, nr);
	for (i = 0; i < nr; i++) {
		HD_VIDEOCAP_FLOW("hd_videocap_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
	}
	return ret;
}

HD_RESULT hd_videocap_drv_get(HD_VIDEOCAP_DRV_PARAM_ID id, VOID *p_param)
{
	HD_RESULT ret = HD_OK;

	videocap_ioctl_init();
	switch (id) {
	case HD_VIDEOCAP_DRV_PARAM_VI_VPORT:
		ret = videocap_vi_vport_get_param((HD_VIDEOCAP_VI_VPORT *)p_param);
		break;
	case HD_VIDEOCAP_DRV_PARAM_VI_CH_PARAM:
		ret = videocap_vi_ch_get_param((HD_VIDEOCAP_VI_CH_PARAM *) p_param);
		break;
	case HD_VIDEOCAP_DRV_PARAM_VI_CH_NORM:
		ret = videocap_vi_ch_get_norm((HD_VIDEOCAP_VI_CH_NORM *)p_param);
		break;
	case HD_VIDEOCAP_DRV_PARAM_GET_VI:
		ret = videocap_get_vi((HD_VIDEOCAP_VI *)p_param);
		break;
	default:
		HD_VIDEOCAPTURE_ERR("Unsupport set param id(%d)\n", id);
		ret = HD_ERR_NOT_SUPPORT;
	}

	return ret;
}

HD_RESULT hd_videocap_drv_set(HD_VIDEOCAP_DRV_PARAM_ID id, VOID *p_param)
{
	HD_RESULT ret = HD_OK;

	videocap_ioctl_init();
	switch (id) {
	case HD_VIDEOCAP_DRV_PARAM_DEREGISTER_VI:
		ret = videocap_vi_deregister((HD_VIDEOCAP_VI_ID *)p_param);
		break;
	case HD_VIDEOCAP_DRV_PARAM_UNINIT_HOST:
		ret = videocap_host_uninit((HD_VIDEOCAP_HOST_ID *)p_param);
		break;
	case HD_VIDEOCAP_DRV_PARAM_INIT_HOST:
		ret = videocap_host_init((HD_VIDEOCAP_HOST *)p_param);
		break;
	case HD_VIDEOCAP_DRV_PARAM_REGISTER_VI:
		ret = videocap_vi_register((HD_VIDEOCAP_VI *)p_param);
		break;
	case HD_VIDEOCAP_DRV_PARAM_VI_VPORT:
		ret = videocap_vi_vport_set_param((HD_VIDEOCAP_VI_VPORT *)p_param);
		break;
	case HD_VIDEOCAP_DRV_PARAM_VI_CH_PARAM:
		ret = videocap_vi_ch_set_param((HD_VIDEOCAP_VI_CH_PARAM *)p_param);
		break;
	case HD_VIDEOCAP_DRV_PARAM_VI_CH_NORM:
		ret = videocap_vi_ch_set_norm((HD_VIDEOCAP_VI_CH_NORM *)p_param);
		if (pif_env_update() < 0) {
			HD_VIDEOCAPTURE_ERR("param id(%d): pif_env_update fail\n", id);
		}
		break;
	case HD_VIDEOCAP_DRV_PARAM_VI_CH_NORM3:
		ret = videocap_vi_ch_set_norm3((HD_VIDEOCAP_VI_CH_NORM3 *)p_param);
		if (pif_env_update() < 0) {
			HD_VIDEOCAPTURE_ERR("param id(%d): pif_env_update fail\n", id);
		}
		break;
	default:
		HD_VIDEOCAPTURE_ERR("Unsupport set param id(%d)\n", id);
		ret = HD_ERR_NOT_SUPPORT;
	}

	return ret;
}

HD_RESULT hd_videocap_get(HD_PATH_ID path_id, HD_VIDEOCAP_PARAM_ID id, VOID *p_param)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_DEVCOUNT *p_devcount;
	HD_VIDEOCAP_SYSCAPS *p_syscaps;
	HD_VIDEOCAP_SYSINFO *p_sysinfo;
	HD_VIDEOCAP_OUT *p_out;
	HD_VIDEOCAP_CROP *p_crop;
	HD_VIDEOCAP_PATH_CONFIG *p_config;
	HD_OUT_POOL *p_out_pool;
	INT i, ret_update;
	if (path_id == VCAP_CTRL_FAKE_PATHID) {
		HD_VIDEOCAP_FLOW("hd_videocap_get():\n"
				 "    %s path_id(%#x)\n"
				 "    id(%u)    not supported now\n", dif_return_dev_string(path_id), path_id, id);
		return HD_OK;
	}
	ret_update = pif_env_update();
	if (ret_update < 0) {
		ret = HD_ERR_DEV;
		goto ext;
	}

	switch (id) {
	case HD_VIDEOCAP_PARAM_DEVCOUNT:
		VIDEOCAP_CHECK_SELF_ID(self_id);
		p_devcount = (HD_DEVCOUNT *)p_param;
		ret = videocap_get_devcount(p_devcount);
		HD_VIDEOCAP_FLOW("hd_videocap_get(DEVCOUNT):\n"
				 "    %s path_id(%#x)\n"
				 "        max_count(%u)\n", dif_return_dev_string(path_id), path_id, p_devcount->max_dev_count);
		break;
	case HD_VIDEOCAP_PARAM_SYSCAPS:
		VIDEOCAP_CHECK_SELF_ID(self_id);
		p_syscaps = (HD_VIDEOCAP_SYSCAPS *)p_param;
		ret = videocap_get_syscaps(self_id, p_syscaps);
		HD_VIDEOCAP_FLOW("hd_videocap_get(SYSCAPS):\n"
				 "    %s path_id(%#x)\n"
				 "        max_dim(%u,%u) max_fps(%u) max_scale(%u) pxlfmt(%#x)\n",
				 dif_return_dev_string(path_id), path_id, p_syscaps->max_dim.w, p_syscaps->max_dim.h,
				 p_syscaps->max_frame_rate, p_syscaps->max_w_scaleup, p_syscaps->pxlfmt);
		break;
	case HD_VIDEOCAP_PARAM_SYSINFO:
		VIDEOCAP_CHECK_SELF_ID(self_id);
		p_sysinfo = (HD_VIDEOCAP_SYSINFO *)p_param;
		ret = videocap_get_sysinfo(self_id, p_sysinfo);
		HD_VIDEOCAP_FLOW("hd_videocap_get(SYSINFO):\n"
				 "    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		for (i = 0; i < HD_VIDEOCAP_MAX_OUT; i++) {
			HD_VIDEOCAP_FLOW("        idx(%d) cur_fps(%u)\n", i, p_sysinfo->cur_fps[i]);
		}
		break;
	case HD_VIDEOCAP_PARAM_OUT:
		VIDEOCAP_CHECK_SELF_ID(self_id);
		VIDEOCAP_CHECK_OUT_ID(out_id);
		p_out = (HD_VIDEOCAP_OUT *)p_param;
		ret = videocap_get_param_out(self_id, out_id, p_out);
		HD_VIDEOCAP_FLOW("hd_videocap_get(OUT):\n"
				 "    %s path_id(%#x)\n"
				 "        dim(%u,%u) pxlfmt(%#x) frc(%#x) dir(%#x) drc(%d)\n",
				 dif_return_dev_string(path_id), path_id, p_out->dim.w, p_out->dim.h, p_out->pxlfmt, p_out->frc, p_out->dir, p_out->depth);
		break;
	case HD_VIDEOCAP_PARAM_IN_CROP:
		VIDEOCAP_CHECK_SELF_ID(self_id);
		VIDEOCAP_CHECK_OUT_ID(out_id);
		p_crop = (HD_VIDEOCAP_CROP *)p_param;
		ret = videocap_get_param_out_crop(self_id, out_id, p_crop);
		HD_VIDEOCAP_FLOW("hd_videocap_get(IN_CROP):\n"
				 "    %s path_id(%#x)\n"
				 "        mode(%d) rect(%u,%u,%u,%u) bg(%u,%u)\n",
				 dif_return_dev_string(path_id), path_id, p_crop->mode, p_crop->win.rect.x, p_crop->win.rect.y,
				 p_crop->win.rect.w, p_crop->win.rect.h,
				 p_crop->win.coord.w, p_crop->win.coord.h);
		break;
	case HD_VIDEOCAP_PARAM_PATH_CONFIG:
		p_config = (HD_VIDEOCAP_PATH_CONFIG *)p_param;
		p_out_pool = &g_video_cap_info[VDOCAP_DEVID(self_id)].out_pool[VDOCAP_OUTID(out_id)];
		memset(p_config, 0x0, sizeof(HD_VIDEOCAP_PATH_CONFIG));
		HD_VIDEOCAP_FLOW("hd_videocap_get(DEV_CONFIG):\n"
				 "    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		for (i = 0; i < HD_VP_MAX_DATA_TYPE; i++) {
			p_config->data_pool[i].ddr_id = p_out_pool->buf_info[i].ddr_id;
			p_config->data_pool[i].counts = p_out_pool->buf_info[i].counts;
			p_config->data_pool[i].max_counts = p_out_pool->buf_info[i].max_counts;
			p_config->data_pool[i].min_counts = p_out_pool->buf_info[i].min_counts;
			p_config->data_pool[i].mode = p_out_pool->buf_info[i].enable;
			HD_VIDEOCAP_FLOW("        idx(%d) ddr(%u) count(%u) max/min(%u,%u) mode(%#x)\n",
					 i, p_config->data_pool[i].ddr_id,
					 p_config->data_pool[i].counts,
					 p_config->data_pool[i].max_counts,
					 p_config->data_pool[i].min_counts,
					 p_config->data_pool[i].mode);
		}
		break;
	case HD_VIDEOCAP_PARAM_OUT_MASK_ATTR:
		ret = videocap_get_mask(path_id, p_param);
		break;
	case HD_VIDEOCAP_PARAM_IN_PALETTE_TABLE:
		ret = videocap_get_mask_palette(path_id, p_param);
		break;
	default:
		HD_VIDEOCAPTURE_ERR("Unsupport set param id(%d)\n", id);
		ret = HD_ERR_PARAM;
		goto ext;
	}

ext:
	return ret;
}

HD_RESULT hd_videocap_set(HD_PATH_ID path_id, HD_VIDEOCAP_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_VIDEOCAP_OUT *cap_out;
	HD_VIDEOCAP_CROP *cap_crop;
	HD_VIDEOCAP_MD_STATUS *cap_md_stat;
	HD_OUT_POOL *p_out_pool;
	HD_VIDEOCAP_PATH_CONFIG *p_config;
	INT i;
	HD_RESULT ret = HD_OK;

	VIDEOCAP_CHECK_SELF_ID(self_id);
	if (path_id == VCAP_CTRL_FAKE_PATHID) {
		HD_VIDEOCAP_FLOW("hd_videocap_set():\n"
				 "    %s path_id(%#x)\n"
				 "    id(%u)    not supported now\n", dif_return_dev_string(path_id), path_id, id);
		return HD_OK;
	}

	switch (id) {
	case HD_VIDEOCAP_PARAM_OUT:
		VIDEOCAP_CHECK_OUT_ID(out_id);
		cap_out = &g_video_cap_info[VDOCAP_DEVID(self_id)].cap_out[VDOCAP_OUTID(out_id)];
		cap_out->dim.w = ((HD_VIDEOCAP_OUT *)p_param)->dim.w;
		cap_out->dim.h = ((HD_VIDEOCAP_OUT *)p_param)->dim.h;
		cap_out->frc = ((HD_VIDEOCAP_OUT *)p_param)->frc;
		cap_out->pxlfmt = ((HD_VIDEOCAP_OUT *)p_param)->pxlfmt;
		cap_out->depth = ((HD_VIDEOCAP_OUT *)p_param)->depth;
		cap_out->dir = ((HD_VIDEOCAP_OUT *)p_param)->dir;
		if (cap_out->dim.w * videocap_hw_ability.scaler_h_ratio_max < platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].width ||
		    cap_out->dim.h * videocap_hw_ability.scaler_v_ratio_max < platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].height) {
			ret = HD_ERR_NG;
			HD_VIDEOCAPTURE_ERR(" %s path_id(%#x): \n"
						"  Over capture's scaling capability."
					    "user w/h(%u,%u) cap_in w/h(%u,%u) scaling-down capability(w:%d, h:%d)\n",
					    dif_return_dev_string(path_id), path_id,
					    cap_out->dim.w, cap_out->dim.h,
					    platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].width,
					    platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].height,
					    videocap_hw_ability.scaler_h_ratio_max,
					    videocap_hw_ability.scaler_v_ratio_max);
			goto ext;
		}
		cap_dev_cfg[VDOCAP_DEVID(self_id)].cap_out[VDOCAP_OUTID(out_id)] = cap_out;
		ret = videocap_check_out_param(cap_out);
		if (ret != HD_OK) {
			ret = HD_ERR_NG;
			goto ext;
		}
		videocap_set_out(self_id, out_id, cap_out);
		HD_VIDEOCAP_FLOW("hd_videocap_set(OUT):\n"
				 "    %s path_id(%#x)\n"
				 "       dim(%u,%u) pxlfmt(%#x) frc(%#x) dir(%#x)\n",
				 dif_return_dev_string(path_id), path_id, cap_out->dim.w, cap_out->dim.h,
				 cap_out->pxlfmt, cap_out->frc, cap_out->dir);
		break;
	case HD_VIDEOCAP_PARAM_PATH_CONFIG:
		p_config = (HD_VIDEOCAP_PATH_CONFIG *)p_param;
		p_out_pool = &g_video_cap_info[VDOCAP_DEVID(self_id)].out_pool[VDOCAP_OUTID(out_id)];
		HD_VIDEOCAP_FLOW("hd_videocap_set(DEV_CONFIG):\n"
				 "    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		for (i = 0; i < HD_VP_MAX_DATA_TYPE; i++) {
			p_out_pool->buf_info[i].ddr_id = p_config->data_pool[i].ddr_id;
			p_out_pool->buf_info[i].counts = p_config->data_pool[i].counts;
			p_out_pool->buf_info[i].max_counts = p_config->data_pool[i].max_counts;
			p_out_pool->buf_info[i].min_counts = p_config->data_pool[i].min_counts;
			p_out_pool->buf_info[i].enable = p_config->data_pool[i].mode;
			HD_VIDEOCAP_FLOW("        idx(%d) ddr(%u) count(%u) max/min(%u,%u) mode(%#x)\n",
					 i, p_config->data_pool[i].ddr_id,
					 p_config->data_pool[i].counts,
					 p_config->data_pool[i].max_counts,
					 p_config->data_pool[i].min_counts,
					 p_config->data_pool[i].mode);
		}
		cap_dev_cfg[VDOCAP_DEVID(self_id)].p_out_pool[VDOCAP_OUTID(out_id)] = p_out_pool;
		break;
	case HD_VIDEOCAP_PARAM_IN_CROP:
		VIDEOCAP_CHECK_OUT_ID(out_id);
		cap_crop = &g_video_cap_info[VDOCAP_DEVID(self_id)].cap_crop[VDOCAP_OUTID(out_id)];
		if ((((HD_VIDEOCAP_CROP *)p_param)->mode != HD_CROP_ON) &&
		    (((HD_VIDEOCAP_CROP *)p_param)->mode !=  HD_CROP_OFF)) {

			ret = HD_ERR_NOT_SUPPORT;
			HD_VIDEOCAPTURE_ERR("Unsupport crop mode(%d)\n", ((HD_VIDEOCAP_CROP *)p_param)->mode);
			goto ext;
		}

		cap_crop->mode = ((HD_VIDEOCAP_CROP *)p_param)->mode;
		cap_crop->win.coord.w = ((HD_VIDEOCAP_CROP *)p_param)->win.coord.w;
		cap_crop->win.coord.h = ((HD_VIDEOCAP_CROP *)p_param)->win.coord.h;
		cap_crop->win.rect.x = ((HD_VIDEOCAP_CROP *)p_param)->win.rect.x;
		cap_crop->win.rect.y = ((HD_VIDEOCAP_CROP *)p_param)->win.rect.y;
		cap_crop->win.rect.w = ((HD_VIDEOCAP_CROP *)p_param)->win.rect.w;
		cap_crop->win.rect.h = ((HD_VIDEOCAP_CROP *)p_param)->win.rect.h;
		cap_dev_cfg[VDOCAP_DEVID(self_id)].cap_crop[VDOCAP_OUTID(out_id)] = cap_crop;
		ret = videocap_check_crop_param(cap_crop);
		if (ret != HD_OK) {
			ret = HD_ERR_NG;
			goto ext;
		}
		videocap_set_crop(self_id, out_id, cap_crop);
		HD_VIDEOCAP_FLOW("hd_videocap_set(IN_CROP):\n"
				 "    %s path_id(%#x)\n"
				 "        mode(%d) rect(%u,%u,%u,%u)(%u,%u)\n",
				 dif_return_dev_string(path_id), path_id, cap_crop->mode, cap_crop->win.rect.x, cap_crop->win.rect.y,
				 cap_crop->win.rect.w, cap_crop->win.rect.h, cap_crop->win.coord.w, cap_crop->win.coord.h);
		break;
	case HD_VIDEOCAP_PARAM_OUT_MD_STATUS:
		VIDEOCAP_CHECK_CTRL_ID(ctrl_id);
		cap_dev_cfg[VDOCAP_DEVID(self_id)].cap_md = &g_video_cap_info[VDOCAP_DEVID(self_id)].cap_md;
		cap_md_stat = &g_video_cap_info[VDOCAP_DEVID(self_id)].cap_md.md_stat;
		memcpy(cap_md_stat, p_param, sizeof(HD_VIDEOCAP_MD_STATUS));
		videocap_set_md_stat(self_id, cap_md_stat);
		HD_VIDEOCAP_FLOW("hd_videocap_set(OUT_MD_STATUS):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d)\n",
				 dif_return_dev_string(path_id), path_id, cap_md_stat->enabled);
		break;
	case HD_VIDEOCAP_PARAM_OUT_MASK_ATTR:
		HD_VIDEOCAP_FLOW("hd_videocap_set(OUT_MASK_ATTR):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d)\n",
				 dif_return_dev_string(path_id), path_id, 0);
		ret = videocap_set_mask(path_id, p_param);
		break;
	case HD_VIDEOCAP_PARAM_IN_PALETTE_TABLE:
		HD_VIDEOCAP_FLOW("hd_videocap_set(IN_PALETTE_TABLE):\n"
				 "    %s path_id(%#x)\n"
				 "        en(%d)\n",
				 dif_return_dev_string(path_id), path_id, 0);
		ret = videocap_set_mask_palette(path_id, p_param);
		break;
	default:
		HD_VIDEOCAPTURE_ERR("Unsupport set param id(%d)\n", id);
		HD_VIDEOCAPTURE_ERR("path_id(%#x))\n", path_id);
		return HD_ERR_NG;
	}
ext:
	return ret;
}

static char bind_src_name[64] = "-";
HD_RESULT hd_videocap_push_in_buf_pair(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 device_subid = VDOCAP_DEVID(self_id);
	HD_IO in_id = HD_GET_OUT(path_id);
	UINT32 dev_idx = VDOCAP_DEVID(self_id);
	VDOCAP_PRIV *p_cap_priv;

	if (out_id >= MAX_CAP_PORT) {
		HD_VIDEOCAPTURE_ERR("hd_videocap_push_in_buf_pair error:\n    path_id(%#x)\n", path_id);
		return HD_ERR_PARAM;
	}

	if (p_video_frame == NULL) {
		ret = HD_ERR_PARAM;
		HD_VIDEOCAPTURE_ERR("hd_videocap_push_in_buf_pair fail. p_video_frame NULL.\n");
		goto state_exit;
	}
	if (p_video_frame->phy_addr[0] != 0) {
		p_video_frame->reserved[6] = VCAP_PUSH_PULL_GIVE_BUF;
	} else {
		HD_VIDEOCAPTURE_ERR("hd_videocap_push_in_buf_pair fail. p_video_frame->phy_addr[0] zero.\n");
		HD_VIDEOCAPTURE_ERR("if no buffer, please use hd_videocap_pull_out_buf()\n");
		goto state_exit;
	}

	/* check and set state */
	dev_idx = VDOCAP_DEVID(self_id);

	p_cap_priv = &g_video_cap_info[dev_idx].priv[VDOCAP_OUTID(out_id)];

	if (p_cap_priv->push_state == HD_PATH_ID_CLOSED) {
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	} else if (p_cap_priv->push_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		p_cap_priv->push_state = HD_PATH_ID_ONGOING;
	}

	if ((g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].push_in_used == 1) ||
	    (bd_get_next(self_id, in_id, bind_src_name, sizeof(bind_src_name)) == HD_OK)) {
		printf("Already used %d in_id(%d) path(0x%x) src(%s)\n",
		       g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].push_in_used, (int)in_id, (int)path_id, bind_src_name);
		system("echo videocap all > /proc/hdal/setting");
		system("cat /proc/hdal/setting");
		ret = HD_ERR_NOT_SUPPORT;
		goto exit;
	}
	g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].push_in_used = 1;
	p_video_frame->reserved[0] = path_id;
	ret = videocap_push_in_buf_pair(self_id, out_id, p_video_frame, wait_ms);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("hd_videocap_push_in_buf_pair fail");
		HD_VIDEOCAPTURE_ERR("   path_id(%#x)\n", path_id);
		HD_VIDEOCAPTURE_ERR("       dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", p_video_frame->dim.w, \
				    p_video_frame->dim.h, p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
	}
	g_video_cap_info[dev_idx].priv[VDOCAP_OUTID(out_id)].push_in_used = 0;
	HD_VIDEOCAP_FLOW("hd_videocap_push_in_buf_pair:\n");
	HD_VIDEOCAP_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	HD_VIDEOCAP_FLOW("       dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", p_video_frame->dim.w, \
			 p_video_frame->dim.h, p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
exit:
	if (p_cap_priv) {
		p_cap_priv->push_state = HD_PATH_ID_OPEN;
	}
	return ret;
state_exit:
	return ret;
}
HD_RESULT hd_videocap_pull_out_buf_pair(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 dev_idx = VDOCAP_DEVID(self_id);
	VDOCAP_PRIV *p_cap_priv;
	if (out_id >= MAX_CAP_PORT) {
		HD_VIDEOCAPTURE_ERR("hd_videocap_push_in_buf_pair error:\n    path_id(%#x)\n", path_id);
		return HD_ERR_PARAM;
	}

	if (p_video_frame == NULL) {
		ret = HD_ERR_PARAM;
		printf("p_video_frame null\n");
		goto state_exit;
	}
	if (p_video_frame->phy_addr[0] != 0) {
		p_video_frame->reserved[6] = VCAP_PUSH_PULL_GIVE_BUF;
	} else {
		HD_VIDEOCAPTURE_ERR("hd_videocap_pull_out_buf_pair fail. p_video_frame->phy_addr[0] zero.\n");
		HD_VIDEOCAPTURE_ERR("need match the value in hd_videocap_push_in_buf_pair\n");
		goto state_exit;
	}

	/* check and set state */
	dev_idx = VDOCAP_DEVID(self_id);

	p_cap_priv = &g_video_cap_info[dev_idx].priv[VDOCAP_OUTID(out_id)];
	if (p_cap_priv->pull_state == HD_PATH_ID_CLOSED) {
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	} else if (p_cap_priv->pull_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		p_cap_priv->pull_state = HD_PATH_ID_ONGOING;
	}

	ret = videocap_pull_out_buf_pair(self_id, out_id, p_video_frame, wait_ms);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("hd_videocap_pull_out_buf_pair fail 0x%x\n", ret);
		HD_VIDEOCAPTURE_ERR("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		HD_VIDEOCAPTURE_ERR("       dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", p_video_frame->dim.w, \
				    p_video_frame->dim.h, p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
	}
	HD_VIDEOCAP_FLOW("hd_videocap_pull_out_buf_pair:\n");
	HD_VIDEOCAP_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	HD_VIDEOCAP_FLOW("       dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", p_video_frame->dim.w, \
			 p_video_frame->dim.h, p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
	p_cap_priv->pull_state = HD_PATH_ID_OPEN;
	return ret;

state_exit:
	return ret;
}

HD_RESULT hd_videocap_push_in_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 device_subid = VDOCAP_DEVID(self_id);
	HD_IO in_id = HD_GET_OUT(path_id);

	if (out_id >= MAX_CAP_PORT) {
		HD_VIDEOCAPTURE_ERR("hd_videocap_push_in_buf_pair error:\n    path_id(%#x)\n", path_id);
		return HD_ERR_PARAM;
	}

	if (p_video_frame == NULL) {
		ret = HD_ERR_PARAM;
		goto exit;
	}
	if ((g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].push_in_used == 1) ||
	    (bd_get_next(self_id, in_id, bind_src_name, sizeof(bind_src_name)) == HD_OK)) {
		printf("Already used %d in_id(%d) path(0x%x) src(%s)\n",
		       g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].push_in_used, (int)in_id, (int)path_id, bind_src_name);
		system("echo videocap all > /proc/hdal/setting");
		system("cat /proc/hdal/setting");
		ret = HD_ERR_NOT_SUPPORT;
		goto exit;
	}
	g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].push_in_used = 1;
	p_video_frame->reserved[0] = path_id;
	ret = videocap_push_in_buf(self_id, out_id, p_video_frame, wait_ms);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("hd_videocap_pull_out_buf fail");
		HD_VIDEOCAPTURE_ERR("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		HD_VIDEOCAPTURE_ERR("       dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", p_video_frame->dim.w, \
				    p_video_frame->dim.h, p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
	}
	g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].push_in_used = 0;
	HD_VIDEOCAP_FLOW("hd_videocap_push_in_buf:\n");
	HD_VIDEOCAP_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	HD_VIDEOCAP_FLOW("       dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", p_video_frame->dim.w, \
			 p_video_frame->dim.h, p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
exit:
	return ret;
}


HD_RESULT hd_videocap_pull_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 device_subid = VDOCAP_DEVID(self_id);
	VDOCAP_PRIV *p_cap_priv;
	HD_VIDEOCAP_INOUT_CTRL *p_ctrl;
	p_ctrl = &g_video_cap_inout_ctrl[device_subid];

	/* check and set state */
	if (out_id >= MAX_CAP_PORT) {
		HD_VIDEOCAPTURE_ERR("hd_videocap_push_in_buf_pair error:\n    path_id(%#x)\n", path_id);
		return HD_ERR_PARAM;
	}

	p_cap_priv = &g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)];
	if (p_cap_priv->pull_state == HD_PATH_ID_CLOSED) {
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	} else if (p_cap_priv->pull_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		p_cap_priv->pull_state = HD_PATH_ID_ONGOING;
	}

	if (p_video_frame == NULL) {
		ret = HD_ERR_PARAM;
		goto state_exit;
	}
	p_video_frame->reserved[6] = 0;
	ret = videocap_pull_out_buf(self_id, out_id, p_video_frame, wait_ms);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("hd_videocap_pull_out_buf fail");
		HD_VIDEOCAPTURE_ERR("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		HD_VIDEOCAPTURE_ERR("       dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", p_video_frame->dim.w, \
				    p_video_frame->dim.h, p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
	}
	HD_VIDEOCAP_FLOW("hd_videocap_pull_out_buf:\n");
	HD_VIDEOCAP_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	HD_VIDEOCAP_FLOW("       dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", p_video_frame->dim.w, \
			 p_video_frame->dim.h, p_video_frame->ddr_id, (VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
	p_cap_priv->pull_state = HD_PATH_ID_OPEN;
	p_ctrl->pull_get = 1;
	return ret;
state_exit:
	return ret;
}

HD_RESULT hd_videocap_release_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);

	if (out_id >= MAX_CAP_PORT) {
		HD_VIDEOCAPTURE_ERR("hd_videocap_push_in_buf_pair error:\n    path_id(%#x)\n", path_id);
		return HD_ERR_PARAM;
	}

	if (p_video_frame == NULL) {
		ret = HD_ERR_PARAM;
		goto exit;
	}
	HD_VIDEOCAP_FLOW("hd_videocap_release_out_buf:\n");
	HD_VIDEOCAP_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	HD_VIDEOCAP_FLOW("       dim(%dx%d) ddrid(%d) pa(%p) fmt(%#x)\n", \
			 p_video_frame->dim.w, p_video_frame->dim.h, p_video_frame->ddr_id, \
			 (VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
	ret = videocap_release_out_buf(self_id, out_id, p_video_frame);


exit:
	return ret;
}


