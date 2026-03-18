/**
	@brief Header file of audio capture module.\n
	This file contains the functions which is related to video output in the chip.

	@file audioenc.c

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
#include "audiocap.h"
#include "logger.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

#define AUDIOCAP_CHECK_PATH_ID(path_id)                                            \
	do {                                                                           \
		if (audiocap_check_path_id_p(path_id) != HD_OK) {                          \
			HD_AUDIOCAPTURE_ERR("Error self_id(%d)\n", (self_id));                 \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define AUDIOCAP_CHECK_OUT_ID(_out_id)                                             \
	do {                                                                           \
		HD_DAL self_id = HD_GET_DEV(_out_id);                                      \
		HD_IO out_id = HD_GET_OUT(_out_id);                                        \
		if (AUDCAP_DEVID(self_id) > MAX_AUDCAP_DEV || ((int)AUDCAP_DEVID(self_id)) < 0) { \
			HD_AUDIOCAPTURE_ERR("Error self_id(%#x) from out_id(%#x)\n", (self_id), (_out_id)); \
			return HD_ERR_NG;                                                      \
		}                                                                          \
		if ((out_id) >= HD_OUT_BASE && (out_id) < (HD_OUT_BASE + MAX_AUDCAP_OUT_PORT)) {        \
		} else {                                                                   \
			HD_AUDIOCAPTURE_ERR("Error out_port(%#x) from out_id(%#x)\n", (out_id), (_out_id)); \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define AUDIOCAP_CHECK_P_PARAM(p_param)                                            \
	do {                                                                           \
		if ((p_param) == NULL) {                                                   \
			HD_AUDIOCAPTURE_ERR("Error p_param(%p)\n", (p_param));                 \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern AUDCAP_INTERNAL_PARAM audcap_dev_param[MAX_AUDCAP_DEV];
extern int pif_env_update(void);

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
UINT32 audiocap_max_device_nr = 0;
UINT32 audiocap_max_device_in;
UINT32 audiocap_max_device_out;
AUDCAP_INFO audiocap_info[MAX_AUDCAP_DEV];
INT audiocap_is_init = 0;

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
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/


HD_RESULT hd_audiocap_init(VOID)
{
	HD_RESULT ret;
	UINT dev_count;

	if (audiocap_is_init == 1) {
		HD_AUDIOCAPTURE_ERR("hd_audiocap_init error, already init.\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	ret = audiocap_init_p();
	if (ret != HD_OK) {
		goto exit;
	}

	audiocap_max_device_nr = audiocap_get_devcount();
	audiocap_max_device_in = MAX_AUDCAP_IN_PORT;
	audiocap_max_device_out = MAX_AUDCAP_OUT_PORT;

	if (audiocap_max_device_nr == 0) {
		dev_count = 2;
	} else {
		dev_count = audiocap_max_device_nr;
	}
	ret = bd_device_init(HD_DAL_AUDIOCAP_BASE, dev_count, audiocap_max_device_in, audiocap_max_device_out);
	if (ret != HD_OK) {
		goto exit;
	}
	audiocap_is_init = 1;
exit:
	HD_AUDIOCAPTURE_FLOW("hd_audiocap_init\n");
	return ret;
}

HD_RESULT hd_audiocap_uninit(VOID)
{
	HD_RESULT ret;

	if (audiocap_is_init != 1) {
		HD_AUDIOCAPTURE_ERR("hd_audiocap_uninit error, not init first.\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	ret = bd_device_uninit(HD_DAL_AUDIOCAP_BASE);

	ret = audiocap_uninit_p();
	if (ret != HD_OK) {
		goto exit;
	}
	audiocap_is_init = 0;

exit:
	HD_AUDIOCAPTURE_FLOW("hd_audiocap_uninit\n");
	return ret;
}


HD_RESULT hd_audiocap_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID *p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	UINT dev_idx = AUDCAP_DEVID(self_id);
	HD_AUDIOCAP_SYSCAPS aucap = {0};
	HD_RESULT ret;

	if (p_path_id == NULL) {
		HD_AUDIOCAPTURE_ERR(" The p_path_id is NULL.\n");
		return HD_ERR_PARAM;
	}

	if (bd_get_dev_baseid(self_id) != HD_DAL_AUDIOCAP_BASE) {
		HD_AUDIOCAPTURE_ERR(" The device of path_id(%#x) is not audiocapture.\n", *p_path_id);
		ret = HD_ERR_DEV;
		goto ext;
	}

	if ((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		//open control path
		ret = bd_device_open(self_id, ctrl_id, _in_id, p_path_id);
		//do nothing
		goto ext;
	}
	if (audiocap_max_device_nr == 0) {
		audiocap_max_device_nr = audiocap_get_devcount();
		if (audiocap_max_device_nr == 0) {
			HD_AUDIOCAPTURE_ERR("hd_audiocap_open error, no audio device.\n");
			ret = HD_ERR_NG;
			goto ext;
		}
	}

	if (in_dev_id != self_id) {
		HD_AUDIOCAPTURE_ERR(" in(%#x) and out(%#x) aren't the same device.\n", _in_id, _out_id);
		ret = HD_ERR_IO;
		goto ext;
	}

	audiocap_get_capability(self_id, &aucap);
	if (out_id > aucap.max_out_count) {
		HD_AUDIOCAPTURE_ERR(" out(%#x) is out of range.\n", _out_id);
		ret = HD_ERR_IO;
		goto ext;
	}

	ret = bd_device_open(self_id, out_id, in_id, p_path_id);
	if (ret != HD_OK) {
		goto ext;
	}
	ret = audiocap_init_dev_p(dev_idx);
	if (ret != HD_OK) {
		goto ext;
	}
ext:
	HD_AUDIOCAPTURE_FLOW("hd_audiocap_open:\n    %s path_id(%#x) self(%#x) out(%u) in(%u)\n",
			     dif_return_dev_string(*p_path_id), *p_path_id, self_id, _out_id, _in_id);
	return ret;
}

HD_RESULT hd_audiocap_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	INT dev_idx = AUDCAP_DEVID(self_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT ret;

	AUDIOCAP_CHECK_PATH_ID(path_id);
	if (ctrl_id != HD_CTRL) {
		ret = audiocap_uninit_dev_p(dev_idx);
		if (ret != HD_OK) {
			goto ext;
		}
	}
	ret = bd_device_close(path_id);
	if (ret != HD_OK) {
		goto ext;
	}

ext:
	HD_AUDIOCAPTURE_FLOW("hd_audiocap_close:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;

}

HD_RESULT hd_audiocap_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);
	UINT dev_idx = AUDCAP_DEVID(self_id);
	AUDCAP_INFO_PRIV *p_audcap_priv;
	INT outport_idx = AUDCAP_OUTID(out_id);

	AUDIOCAP_CHECK_OUT_ID(_out_id);
	p_audcap_priv = &audiocap_info[dev_idx].priv;
	ret = bd_bind(self_id, out_id, dest_id, in_id);
	if (ret == HD_OK) {
		p_audcap_priv->port[outport_idx].state = AUDCAP_STATE_BOUND;
	}
	HD_AUDIOCAPTURE_FLOW("hd_audiocap_bind: %s_%d_OUT_%d -> %s_%d_IN_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(out_id),
		get_device_name(bd_get_dev_baseid(dest_id)), bd_get_dev_subid(dest_id), BD_GET_IN_SUBID(in_id));
	return ret;
}


HD_RESULT hd_audiocap_unbind(HD_OUT_ID _out_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	INT dev_idx = AUDCAP_DEVID(self_id);
	INT outport_idx = AUDCAP_OUTID(out_id);
	AUDCAP_INFO_PRIV *p_audcap_priv;

	AUDIOCAP_CHECK_OUT_ID(_out_id);
	p_audcap_priv = &audiocap_info[dev_idx].priv;

	ret = bd_unbind(self_id, out_id);
	if (ret == HD_OK) {
		p_audcap_priv->port[outport_idx].state = AUDCAP_STATE_UNBOUND;
	}
	HD_AUDIOCAPTURE_FLOW("hd_audiocap_unbind: %s_%d_OUT_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(out_id));
	return ret;
}

HD_RESULT hd_audiocap_start(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);

	AUDIOCAP_CHECK_PATH_ID(path_id);
	ret = bd_start_list(&path_id, 1);
	HD_AUDIOCAPTURE_FLOW("hd_audiocap_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_audiocap_stop(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);

	AUDIOCAP_CHECK_PATH_ID(path_id);
	ret = bd_stop_list(&path_id, 1);
	HD_AUDIOCAPTURE_FLOW("hd_audiocap_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_audiocap_start_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;

	if (path_id == NULL) {
		HD_AUDIOCAPTURE_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}

	for (i = 0; i < nr; i++) {
		ret = audiocap_check_path_id_p(path_id[i]);
		if (ret != HD_OK)
			goto ext;
	}
	ret = bd_start_list(path_id, nr);
ext:
	return ret;
}

HD_RESULT hd_audiocap_stop_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;

	if (path_id == NULL) {
		HD_AUDIOCAPTURE_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}

	for (i = 0; i < nr; i++) {
		ret = audiocap_check_path_id_p(path_id[i]);
		if (ret != HD_OK)
			goto ext;
	}

	ret = bd_stop_list(path_id, nr);

ext:
	return ret;
}

HD_RESULT hd_audiocap_set(HD_PATH_ID path_id, HD_AUDIOCAP_PARAM_ID id, VOID *p_param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT dev_idx = AUDCAP_DEVID(self_id);
	INT outport_idx, ret, i;
	AUDCAP_INFO *p_audcap_info;
	CHAR msg_string[256];

	//check parameters
	AUDIOCAP_CHECK_PATH_ID(path_id);
	if (id != HD_AUDIOCAP_PARAM_CLEAR_BUF) {
		AUDIOCAP_CHECK_P_PARAM(p_param);
		ret = check_audiocap_params_range(id, p_param, msg_string, 256);
		if (ret < 0) {
			HD_AUDIOCAPTURE_ERR("Wrong value. %s\n", msg_string);
			return HD_ERR_PARAM;
		}
	}

	//store user values
	p_audcap_info = &audiocap_info[dev_idx];
	outport_idx = AUDCAP_OUTID(out_id);
	switch (id) {
	case HD_AUDIOCAP_PARAM_DEVCOUNT:
		HD_AUDIOCAPTURE_ERR("audiocap doesn't support set opertion for HD_AUDIOCAP_PARAM_DEVCOUNT param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIOCAP_PARAM_SYSCAPS:
		HD_AUDIOCAPTURE_ERR("audiocap doesn't support set opertion for HD_AUDIOCAP_PARAM_SYSCAPS param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIOCAP_PARAM_SYSINFO:
		HD_AUDIOCAPTURE_ERR("audiocap doesn't support set opertion for HD_AUDIOCAP_PARAM_SYSINFO param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIOCAP_PARAM_DEV_CONFIG:
		memcpy(&p_audcap_info->dev_config, p_param, sizeof(HD_AUDIOCAP_DEV_CONFIG));
		audcap_dev_param[dev_idx].p_dev_config = &p_audcap_info->dev_config;
		HD_AUDIOCAPTURE_FLOW("hd_audiocap_set(DEV_CONFIG):\n"
				     "    %s path_id(%#x)\n"
				     "        in(mode:%#x, sr:%#x, sb:%#x, fs:%u) out(sr:%#x)\n"
				     "        aec en(%d) leak(%d,%d) nc(%d) ec(%d) fl(%d) fs(%d) nr(%d)\n"
				     "        anr en(%d) sp(%d) hpf(%d) bias(%d) frame(%u)\n",
				     dif_return_dev_string(path_id), path_id, p_audcap_info->dev_config.in_max.mode,
				     p_audcap_info->dev_config.in_max.sample_rate,
				     p_audcap_info->dev_config.in_max.sample_bit,
				     p_audcap_info->dev_config.in_max.frame_sample,
				     p_audcap_info->dev_config.out_max.sample_rate,
				     p_audcap_info->dev_config.aec_max.enabled,
				     p_audcap_info->dev_config.aec_max.leak_estimate_enabled,
				     p_audcap_info->dev_config.aec_max.leak_estimate_value,
				     p_audcap_info->dev_config.aec_max.noise_cancel_level,
				     p_audcap_info->dev_config.aec_max.echo_cancel_level,
				     p_audcap_info->dev_config.aec_max.filter_length,
				     p_audcap_info->dev_config.aec_max.frame_size,
				     p_audcap_info->dev_config.aec_max.notch_radius,
				     p_audcap_info->dev_config.anr_max.enabled,
				     p_audcap_info->dev_config.anr_max.suppress_level,
				     p_audcap_info->dev_config.anr_max.hpf_cut_off_freq,
				     p_audcap_info->dev_config.anr_max.bias_sensitive,
				     p_audcap_info->dev_config.frame_num_max);
		for (i = 0; i < HD_AUDIOCAP_MAX_DATA_TYPE; i++) {
			p_audcap_info->dev_config.data_pool[i].max_counts = p_audcap_info->dev_config.data_pool[i].counts;
			p_audcap_info->dev_config.data_pool[i].min_counts = 0;
			HD_AUDIOCAPTURE_FLOW("    idx(%d) ddr(%u) count(%u) max/min(%u,%u) mode(%#x)\n",
					     i, p_audcap_info->dev_config.data_pool[i].ddr_id,
					     p_audcap_info->dev_config.data_pool[i].counts,
					     p_audcap_info->dev_config.data_pool[i].max_counts,
					     p_audcap_info->dev_config.data_pool[i].min_counts,
					     p_audcap_info->dev_config.data_pool[i].mode);
		}
		break;
	case HD_AUDIOCAP_PARAM_DRV_CONFIG:
		// parameter is configurated from kdriver
		break;
	case HD_AUDIOCAP_PARAM_IN:
		memcpy(&p_audcap_info->param_in, p_param, sizeof(HD_AUDIOCAP_IN));
		HD_AUDIOCAPTURE_FLOW("hd_audiocap_set(IN):\n"
				     "    %s path_id(%#x)\n"
				     "        mode(%#x) sample_rate(%#x) sample_bit(%#x) frame_sample(%u)\n",
				     dif_return_dev_string(path_id), path_id, p_audcap_info->param_in.mode, p_audcap_info->param_in.sample_rate,
				     p_audcap_info->param_in.sample_bit, p_audcap_info->param_in.frame_sample);
		hd_ret = audiocap_set_basic_param(path_id, p_param);
		if (hd_ret != HD_OK) {
			hd_ret = audiocap_get_basic_param(path_id, p_param);
			if (hd_ret == HD_OK) {
				memcpy(&p_audcap_info->param_in, p_param, sizeof(HD_AUDIOCAP_IN));
			}
		}
		audcap_dev_param[dev_idx].p_param_in = &p_audcap_info->param_in;
		break;

	case HD_AUDIOCAP_PARAM_OUT:
		memcpy(&p_audcap_info->port[outport_idx].param_out, p_param, sizeof(HD_AUDIOCAP_OUT));
		audcap_dev_param[dev_idx].port[outport_idx].p_param_out = &p_audcap_info->port[outport_idx].param_out;
		HD_AUDIOCAPTURE_FLOW("hd_audiocap_set(OUT):\n"
				     "    %s path_id(%#x)\n"
				     "        sample_rate(%#x)\n",
				     dif_return_dev_string(path_id), path_id, p_audcap_info->port[outport_idx].param_out.sample_rate);
		break;

	case HD_AUDIOCAP_PARAM_OUT_AEC:
		memcpy(&p_audcap_info->port[outport_idx].param_aec, p_param, sizeof(HD_AUDIOCAP_AEC));
		audcap_dev_param[dev_idx].port[outport_idx].p_param_aec = &p_audcap_info->port[outport_idx].param_aec;
		break;

	case HD_AUDIOCAP_PARAM_OUT_ANR:
		memcpy(&p_audcap_info->port[outport_idx].param_anr, p_param, sizeof(HD_AUDIOCAP_ANR));
		audcap_dev_param[dev_idx].port[outport_idx].p_param_anr = &p_audcap_info->port[outport_idx].param_anr;
		break;

	case HD_AUDIOCAP_PARAM_VOLUME:
		memcpy(&p_audcap_info->param_volume, p_param, sizeof(HD_AUDIOCAP_VOLUME));
		audcap_dev_param[dev_idx].p_param_volume = &p_audcap_info->param_volume;
		HD_AUDIOCAPTURE_FLOW("hd_audiocap_set(VOLUME):\n"
				     "    %s path_id(%#x)\n"
				     "        volume(%u)\n",
				     dif_return_dev_string(path_id), path_id, p_audcap_info->param_volume.volume);
		break;

	case HD_AUDIOCAP_PARAM_CLEAR_BUF:
		hd_ret = audiocap_clear_buf(dev_idx);
		break;

	default:
		break;
	}

	return hd_ret;
}

HD_RESULT hd_audiocap_get(HD_PATH_ID path_id, HD_AUDIOCAP_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT dev_idx = AUDCAP_DEVID(self_id);
	INT outport_idx, i;
	AUDCAP_INFO *p_audcap_info;

	//check parameters
	AUDIOCAP_CHECK_PATH_ID(path_id);
	AUDIOCAP_CHECK_P_PARAM(p_param);

	//retrieve user values
	p_audcap_info = &audiocap_info[dev_idx];
	outport_idx = AUDCAP_OUTID(out_id);
	switch (id) {
	case HD_AUDIOCAP_PARAM_DEVCOUNT: {
		HD_DEVCOUNT *p_devcount = (HD_DEVCOUNT *) p_param;
		p_devcount->max_dev_count = audiocap_get_devcount();
		break;
	}
	case HD_AUDIOCAP_PARAM_SYSCAPS: {
		HD_AUDIOCAP_SYSCAPS *p_aucap = (HD_AUDIOCAP_SYSCAPS *) p_param;
		audiocap_get_capability(self_id, p_aucap);
		break;
	}
	case HD_AUDIOCAP_PARAM_SYSINFO:
		//TODO
		break;

	case HD_AUDIOCAP_PARAM_DEV_CONFIG:
		memcpy(p_param, &p_audcap_info->dev_config, sizeof(HD_AUDIOCAP_DEV_CONFIG));
		HD_AUDIOCAPTURE_FLOW("hd_audiocap_get(DEV_CONFIG):\n"
				     "    %s path_id(%#x)\n"
				     "        in(mode:%#x, sr:%#x, sb:%#x, fs:%u) out(sr:%#x)"
				     "        aec en(%d) leak(%d,%d) nc(%d) ec(%d) fl(%d) fs(%d) nr(%d)"
				     "        anr en(%d) sp(%d) hpf(%d) bias(%d) frame(%u)\n",
				     dif_return_dev_string(path_id), path_id, p_audcap_info->dev_config.in_max.mode,
				     p_audcap_info->dev_config.in_max.sample_rate,
				     p_audcap_info->dev_config.in_max.sample_bit,
				     p_audcap_info->dev_config.in_max.frame_sample,
				     p_audcap_info->dev_config.out_max.sample_rate,
				     p_audcap_info->dev_config.aec_max.enabled,
				     p_audcap_info->dev_config.aec_max.leak_estimate_enabled,
				     p_audcap_info->dev_config.aec_max.leak_estimate_value,
				     p_audcap_info->dev_config.aec_max.noise_cancel_level,
				     p_audcap_info->dev_config.aec_max.echo_cancel_level,
				     p_audcap_info->dev_config.aec_max.filter_length,
				     p_audcap_info->dev_config.aec_max.frame_size,
				     p_audcap_info->dev_config.aec_max.notch_radius,
				     p_audcap_info->dev_config.anr_max.enabled,
				     p_audcap_info->dev_config.anr_max.suppress_level,
				     p_audcap_info->dev_config.anr_max.hpf_cut_off_freq,
				     p_audcap_info->dev_config.anr_max.bias_sensitive,
				     p_audcap_info->dev_config.frame_num_max);
		for (i = 0; i < HD_AUDIOCAP_MAX_DATA_TYPE; i++) {
			HD_AUDIOCAPTURE_FLOW("        idx(%d) ddr(%u) count(%u) max/min(%u,%u) mode(%#x)\n",
					     i, p_audcap_info->dev_config.data_pool[i].ddr_id,
					     p_audcap_info->dev_config.data_pool[i].counts,
					     p_audcap_info->dev_config.data_pool[i].max_counts,
					     p_audcap_info->dev_config.data_pool[i].min_counts,
					     p_audcap_info->dev_config.data_pool[i].mode);
		}

		break;

	case HD_AUDIOCAP_PARAM_DRV_CONFIG:
		memcpy(p_param, &p_audcap_info->drv_config, sizeof(HD_AUDIOCAP_DRV_CONFIG));
		HD_AUDIOCAPTURE_FLOW("hd_audiocap_get(DRV_CONFIG):\n"
				     "    %s path_id(%#x)\n"
				     "        mono(%#x)\n",
				     dif_return_dev_string(path_id), path_id, p_audcap_info->drv_config.mono);
		for (i = 0; i < AUDIOCAP_MAX_DEVICE_NUM; i++) {
			HD_AUDIOCAPTURE_FLOW("        idx(%d) en(%d) num(%d) ch(%d) ss(%d) sr(%d) ssp_clk(%d) bit_clk(%d) mst(%d) ls_ch(%d)\n",
					     i, p_audcap_info->drv_config.ssp_config.enable[i],
					     p_audcap_info->drv_config.ssp_config.ssp_num[i],
					     p_audcap_info->drv_config.ssp_config.ssp_chan[i],
					     p_audcap_info->drv_config.ssp_config.sample_size[i],
					     p_audcap_info->drv_config.ssp_config.sample_rate[i],
					     p_audcap_info->drv_config.ssp_config.ssp_clock[i],
					     p_audcap_info->drv_config.ssp_config.bit_clock[i],
					     p_audcap_info->drv_config.ssp_config.ssp_master[i],
					     p_audcap_info->drv_config.ssp_config.live_sound_ch[i]);
		}
		break;

	case HD_AUDIOCAP_PARAM_IN:
		memcpy(p_param, &p_audcap_info->param_in, sizeof(HD_AUDIOCAP_IN));
		HD_AUDIOCAPTURE_FLOW("hd_audiocap_get(IN):\n"
				     "    %s path_id(%#x)\n"
				     "        mode(%#x) sample_rate(%#x) sample_bit(%#x) frame_sample(%u)\n",
				     dif_return_dev_string(path_id), path_id, p_audcap_info->param_in.mode, p_audcap_info->param_in.sample_rate,
				     p_audcap_info->param_in.sample_bit, p_audcap_info->param_in.frame_sample);
		break;

	case HD_AUDIOCAP_PARAM_OUT:
		memcpy(p_param, &p_audcap_info->port[outport_idx].param_out, sizeof(HD_AUDIOCAP_OUT));
		HD_AUDIOCAPTURE_FLOW("hd_audiocap_get(OUT):\n"
				     "    %s path_id(%#x)\n"
				     "        sample_rate(%#x)\n",
				     dif_return_dev_string(path_id), path_id, p_audcap_info->port[outport_idx].param_out.sample_rate);
		break;

	case HD_AUDIOCAP_PARAM_OUT_AEC:
		memcpy(p_param, &p_audcap_info->port[outport_idx].param_aec, sizeof(HD_AUDIOCAP_AEC));
		break;

	case HD_AUDIOCAP_PARAM_OUT_ANR:
		memcpy(p_param, &p_audcap_info->port[outport_idx].param_anr, sizeof(HD_AUDIOCAP_ANR));
		break;

	case HD_AUDIOCAP_PARAM_VOLUME:
		memcpy(p_param, &p_audcap_info->param_volume, sizeof(HD_AUDIOCAP_VOLUME));
		HD_AUDIOCAPTURE_FLOW("hd_audiocap_get(VOLUME):\n"
				     "    %s path_id(%#x)\n"
				     "        volume(%u)\n",
				     dif_return_dev_string(path_id), path_id, p_audcap_info->param_volume.volume);
		break;

	case HD_AUDIOCAP_PARAM_CLEAR_BUF:
		HD_AUDIOCAPTURE_ERR("audiocap doesn't support get opertion for HD_AUDIOCAP_PARAM_CLEAR_BUF param.\n");
		return HD_ERR_PARAM;

	default:
		break;
	}

	return HD_OK;
}

HD_RESULT hd_audiocap_pull_out_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms)
{
	HD_AUDIOCAPTURE_ERR("Not support for %s, path_id(%#x)\n", __func__, path_id);
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audiocap_release_out_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME *p_audio_frame)
{
	HD_AUDIOCAPTURE_ERR("Not support for %s, path_id(%#x)\n", __func__, path_id);
	return HD_ERR_NOT_SUPPORT;
}




