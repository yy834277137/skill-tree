/**
	@brief Header file of audio out module.\n
	This file contains the functions which is related to video output in the chip.

	@file audioout.c

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
#include <pthread.h>
#include "hd_type.h"
#include "hdal.h"
#include "bind.h"
#include "dif.h"
#include "audioout.h"
#include "logger.h"
#include "kflow_audioio.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

#define AUDIOOUT_CHECK_PATH_ID(path_id)                                            \
	do {                                                                           \
		if (audioout_check_path_id_p(path_id) != HD_OK) {                          \
			HD_AUDIOOUT_ERR("Error self_id(%d)\n", (self_id));                 \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define AUDIOOUT_CHECK_OUT_ID(_out_id)                                             \
	do {                                                                           \
		HD_DAL self_id = HD_GET_DEV(_out_id);                                      \
		HD_IO out_id = HD_GET_OUT(_out_id);                                        \
		if (AUDOUT_DEVID(self_id) > MAX_AUDOUT_DEV || AUDOUT_DEVID(self_id) < 0) { \
			HD_AUDIOOUT_ERR("Error self_id(%#x) from out_id(%#x)\n", (self_id), (_out_id)); \
			return HD_ERR_NG;                                                      \
		}                                                                          \
		if ((out_id) >= HD_OUT_BASE && (out_id) < (HD_OUT_BASE + MAX_AUDOUT_OUT_PORT)) {        \
		} else {                                                                   \
			HD_AUDIOOUT_ERR("Error out_port(%#x) from out_id(%#x)\n", (out_id), (_out_id)); \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define AUDIOOUT_CHECK_P_PARAM(p_param)                                            \
	do {                                                                           \
		if ((p_param) == NULL) {                                                   \
			HD_AUDIOOUT_ERR("Error p_param(%p)\n", (p_param));                 \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define AUDIOOUT_CHECK_HD_AUDIO_FRAME_SIZE(p_audio_frame)                                     \
	do {                                                                                      \
		if (p_audio_frame == NULL) {                                                          \
			ret = HD_ERR_PARAM;                                                               \
			goto exit;                                                                        \
		}                                                                                     \
		if (p_audio_frame->size == 0) {                                                                      \
			HD_AUDIOOUT_ERR("Check HD_AUDIO_FRAME size(%d) is zero.\n", p_audio_frame->size);\
			ret = HD_ERR_NOT_SUPPORT;                                                           \
			goto exit;                                                                        \
		}                                                                                     \
	} while (0)
/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern AUDOUT_INTERNAL_PARAM audout_dev_param[MAX_AUDOUT_DEV];
extern int pif_env_update(void);
extern KFLOW_AUDIOIO_HW_SPEC_INFO audioio_hw_spec_info;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
UINT32 audioout_max_device_nr = 0;
UINT32 audioout_max_device_in;
UINT32 audioout_max_device_out;
AUDOUT_INFO audioout_info[MAX_AUDOUT_DEV] = {0};
pthread_mutex_t audioout_dev_lock[MAX_AUDOUT_DEV] = {0};

INT audioout_is_init = 0;

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
HD_RESULT hd_audioout_init(VOID)
{
	HD_RESULT ret;
	UINT dev_count, i;
	int ret_val;

	if (audioout_is_init == 1) {
		HD_AUDIOOUT_ERR("hd_audioout_init error, already init.\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	audioout_max_device_nr = audioout_get_devcount();
	audioout_max_device_in = 1;
	audioout_max_device_out = 1;

	if (audioout_max_device_nr == 0) {
		dev_count = 3;
	} else {
		dev_count = audioout_max_device_nr;
	}
	for (i = 0; i < audioout_max_device_nr; i++) {
		if ((ret_val = pthread_mutex_init(&audioout_dev_lock[i], NULL)) != 0) {
			HD_AUDIOOUT_ERR("Init device mutex failed, ret(%d) idx(%u) count(%#x)\n",
					ret_val, i, audioout_max_device_nr);
			ret = HD_ERR_NG;
			goto exit;
		}
	}

	ret = audioout_init_p();
	if (ret != HD_OK) {
		goto exit;
	}

	ret = bd_device_init(HD_DAL_AUDIOOUT_BASE, dev_count, audioout_max_device_in, audioout_max_device_out);
	if (ret != HD_OK) {
		goto exit;
	}
	audioout_is_init = 1;
exit:
	HD_AUDIOOUT_FLOW("hd_audioout_init\n");
	return ret;
}

HD_RESULT hd_audioout_uninit(VOID)
{
	UINT i;
	HD_RESULT ret;
	int ret_val;

	if (audioout_is_init != 1) {
		HD_AUDIOOUT_ERR("hd_audioout_uninit error, not init first.\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	ret = bd_device_uninit(HD_DAL_AUDIOOUT_BASE);

	for (i = 0; i < audioout_max_device_nr; i++) {
		if ((ret_val = pthread_mutex_destroy(&audioout_dev_lock[i])) != 0) {
			HD_AUDIOOUT_ERR("Destroy device mutex failed, ret(%d,%s) idx(%u) count(%#x)\n",
					ret_val, strerror(errno), i, audioout_max_device_nr);
			ret = HD_ERR_NG;
			goto exit;
		}
	}

	ret = audioout_uninit_p();
	if (ret != HD_OK) {
		goto exit;
	}

	audioout_is_init = 0;
exit:
	HD_AUDIOOUT_FLOW("hd_audioout_uninit\n");
	return ret;
}

HD_RESULT hd_audioout_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID *p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_IO ctrl_id = HD_GET_CTRL(_out_id);
	UINT dev_idx = AUDOUT_DEVID(self_id);
	HD_RESULT ret;

	if (p_path_id == NULL) {
		HD_AUDIOOUT_ERR(" The p_path_id is NULL.\n");
		return HD_ERR_PARAM;
	}

	if (bd_get_dev_baseid(self_id) != HD_DAL_AUDIOOUT_BASE) {
		HD_AUDIOOUT_ERR(" The device of path_id(%#x) is not audioout.\n", *p_path_id);
		ret = HD_ERR_DEV;
		goto ext;
	}

	if ((_in_id == 0) && (ctrl_id == HD_CTRL)) {
		//open control path
		ret = bd_device_open(self_id, ctrl_id, _in_id, p_path_id);
		//do nothing
		goto ext;
	}

	if (audioout_max_device_nr == 0) {
		audioout_max_device_nr = audioout_get_devcount();
		if (audioout_max_device_nr == 0) {
			HD_AUDIOOUT_ERR("hd_audioout_open error, no audio device.\n");
			ret = HD_ERR_NG;
			goto ext;
		}
	}

	if (in_dev_id != self_id) {
		HD_AUDIOOUT_ERR(" in(%#x) and out(%#x) aren't the same device.\n", _in_id, _out_id);
		ret = HD_ERR_IO;
		goto ext;
	}

	/* check and set state */
	if (audioout_info[dev_idx].priv.path_id_state == HD_PATH_ID_OPEN) {
		if ((ret = bd_get_already_open_pathid(self_id, _out_id, _in_id, p_path_id)) != HD_OK) {
			goto state_exit;
		} else {
			ret = HD_ERR_ALREADY_OPEN;
			goto state_exit;
		}
	} else if (audioout_info[dev_idx].priv.path_id_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		audioout_info[dev_idx].priv.path_id_state = HD_PATH_ID_ONGOING;
	}

	pthread_mutex_lock(&audioout_dev_lock[dev_idx]);

	ret = bd_device_open(self_id, out_id, in_id, p_path_id);
	if (ret != HD_OK) {
		pthread_mutex_unlock(&audioout_dev_lock[dev_idx]);
		goto ext;
	}
	ret = audioout_init_dev_p(dev_idx);
	if (ret != HD_OK) {
		pthread_mutex_unlock(&audioout_dev_lock[dev_idx]);
		goto ext;
	}
	pthread_mutex_unlock(&audioout_dev_lock[dev_idx]);

	HD_AUDIOOUT_FLOW("hd_audioout_open:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u)\n",
			 dif_return_dev_string(*p_path_id), *p_path_id, self_id, out_id, in_id);
	audioout_info[dev_idx].priv.path_id_state = HD_PATH_ID_OPEN;
	return ret;

ext:
	audioout_info[dev_idx].priv.path_id_state = HD_PATH_ID_CLOSED;
	return ret;

state_exit:
	return ret;
}

HD_RESULT hd_audioout_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	INT dev_idx = AUDOUT_DEVID(self_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT ret;

	/* check and set state */
	if (audioout_info[dev_idx].priv.path_id_state == HD_PATH_ID_CLOSED) {
		//HD_AUDIOOUT_ERR("Fail to close, path_id(%#lx) is not opened.\n", path_id);
		//ret = HD_ERR_NOT_OPEN;
		goto uninit;
	} else if (audioout_info[dev_idx].priv.path_id_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		audioout_info[dev_idx].priv.path_id_state = HD_PATH_ID_ONGOING;
	}

uninit:
	AUDIOOUT_CHECK_PATH_ID(path_id);
	if (bd_get_dev_baseid(self_id) != HD_DAL_AUDIOOUT_BASE) {
		HD_AUDIOOUT_ERR(" The device of path_id(%#x) is not audioout.\n", path_id);
		ret = HD_ERR_DEV;
		goto ext;
	}

	pthread_mutex_lock(&audioout_dev_lock[dev_idx]);
	if (ctrl_id != HD_CTRL) {
		ret = audioout_uninit_dev_p(dev_idx);
		if (ret != HD_OK) {
			pthread_mutex_unlock(&audioout_dev_lock[dev_idx]);
			goto ext;
		}
	}
	ret = bd_device_close(path_id);
	if (ret != HD_OK) {
		pthread_mutex_unlock(&audioout_dev_lock[dev_idx]);
		goto ext;
	}
	pthread_mutex_unlock(&audioout_dev_lock[dev_idx]);

	HD_AUDIOOUT_FLOW("hd_audioout_close:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	audioout_info[dev_idx].priv.path_id_state = HD_PATH_ID_CLOSED;
	return ret;

ext:
	audioout_info[dev_idx].priv.path_id_state = HD_PATH_ID_OPEN;
	return ret;

state_exit:
	return ret;
}

HD_RESULT hd_audioout_start(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);

	AUDIOOUT_CHECK_PATH_ID(path_id);
	ret = bd_start_list(&path_id, 1);
	HD_AUDIOOUT_FLOW("hd_audioout_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_audioout_stop(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);

	AUDIOOUT_CHECK_PATH_ID(path_id);
	ret = bd_stop_list(&path_id, 1);
	HD_AUDIOOUT_FLOW("hd_audioout_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_audioout_start_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;
	if (path_id == NULL) {
		HD_AUDIOOUT_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}
	for (i = 0; i < nr; i++) {
		ret = audioout_check_path_id_p(path_id[i]);
		if (ret != HD_OK)
			goto ext;
	}
	ret = bd_start_list(path_id, nr);
ext:
	return ret;
}

HD_RESULT hd_audioout_stop_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;
	if (path_id == NULL) {
		HD_AUDIOOUT_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}
	for (i = 0; i < nr; i++) {
		ret = audioout_check_path_id_p(path_id[i]);
		if (ret != HD_OK)
			goto ext;
	}

	ret = bd_stop_list(path_id, nr);

ext:
	return ret;
}

HD_RESULT hd_audioout_set(HD_PATH_ID path_id, HD_AUDIOOUT_PARAM_ID id, VOID *p_param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = AUDOUT_DEVID(self_id);
	INT inport_idx, ret;
	AUDOUT_INFO *p_audout_info;
	CHAR msg_string[256];

	//check parameters
	AUDIOOUT_CHECK_PATH_ID(path_id);
	if (id != HD_AUDIOOUT_PARAM_CLEAR_BUF) {
		AUDIOOUT_CHECK_P_PARAM(p_param);
		ret = check_audioout_params_range(id, p_param, msg_string, 256);
		if (ret < 0) {
			HD_AUDIOOUT_ERR("Wrong value. %s\n", msg_string);
			return HD_ERR_PARAM;
		}
	}

	//store user values
	p_audout_info = &audioout_info[dev_idx];
	inport_idx = AUDOUT_INID(in_id);
	switch (id) {
	case HD_AUDIOOUT_PARAM_DEVCOUNT:
		HD_AUDIOOUT_ERR("audioout doesn't support set opertion for HD_AUDIOOUT_PARAM_DEVCOUNT param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIOOUT_PARAM_SYSCAPS:
		HD_AUDIOOUT_ERR("audioout doesn't support set opertion for HD_AUDIOOUT_PARAM_SYSCAPS param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIOOUT_PARAM_SYSINFO:
		HD_AUDIOOUT_ERR("audioout doesn't support set opertion for HD_AUDIOOUT_PARAM_SYSINFO param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIOOUT_PARAM_DEV_CONFIG:
		memcpy(&p_audout_info->dev_config, p_param, sizeof(HD_AUDIOOUT_DEV_CONFIG));
		HD_AUDIOOUT_FLOW("hd_audioout_set(DEV_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        in(sr:%#x) out(mode:%#x, sr:%#x, sb:%#x)\n"
				 "        frame_sample(%u) frame_num(%u)\n",
				 dif_return_dev_string(path_id), path_id, p_audout_info->dev_config.in_max.sample_rate,
				 p_audout_info->dev_config.out_max.mode,
				 p_audout_info->dev_config.out_max.sample_rate,
				 p_audout_info->dev_config.out_max.sample_bit,
				 p_audout_info->dev_config.frame_sample_max,
				 p_audout_info->dev_config.frame_num_max);
		break;

	case HD_AUDIOOUT_PARAM_DRV_CONFIG:
		// parameter is configurated from kdriver

		break;

	case HD_AUDIOOUT_PARAM_IN:
		memcpy(&p_audout_info->param_in[inport_idx], p_param, sizeof(HD_AUDIOOUT_IN));
		HD_AUDIOOUT_FLOW("hd_audioout_set(IN):\n"
				 "    %s path_id(%#x)\n"
				 "        sample_rate(%#x)\n",
				 dif_return_dev_string(path_id), path_id, p_audout_info->param_in[inport_idx].sample_rate);
		hd_ret = audioout_set_basic_param(path_id, p_param, NULL);
		if (hd_ret != HD_OK) {
			hd_ret = audioout_get_basic_param(path_id, p_param, NULL);
			if (hd_ret == HD_OK) {
				memcpy(&p_audout_info->param_in, p_param, sizeof(HD_AUDIOOUT_IN));
			}
		}
		audout_dev_param[dev_idx].p_param_in[inport_idx] = &p_audout_info->param_in[inport_idx];
		break;

	case HD_AUDIOOUT_PARAM_OUT:
		memcpy(&p_audout_info->param_out, p_param, sizeof(HD_AUDIOOUT_OUT));
		HD_AUDIOOUT_FLOW("hd_audioout_set(OUT):\n"
				 "    %s path_id(%#x)\n"
				 "        mode(%#x) sample_rate(%#x) sample_bit(%#x)\n",
				 dif_return_dev_string(path_id), path_id, p_audout_info->param_out.mode, p_audout_info->param_out.sample_rate,
				 p_audout_info->param_out.sample_bit);
		hd_ret = audioout_set_basic_param(path_id, NULL, p_param);
		if (hd_ret != HD_OK) {
			hd_ret = audioout_get_basic_param(path_id, NULL, p_param);
			if (hd_ret == HD_OK) {
				memcpy(&p_audout_info->param_out, p_param, sizeof(HD_AUDIOOUT_OUT));
			}
		}
		audout_dev_param[dev_idx].p_param_out = &p_audout_info->param_out;
		break;

	case HD_AUDIOOUT_PARAM_VOLUME:
		if (((HD_AUDIOOUT_VOLUME *) p_param)->volume <= 100) {
			memcpy(&p_audout_info->param_volume, p_param, sizeof(HD_AUDIOOUT_VOLUME));
			audout_dev_param[dev_idx].p_param_volume = &p_audout_info->param_volume;
			audioout_set_param(path_id, (HD_AUDIOOUT_VOLUME *) p_param);
			HD_AUDIOOUT_FLOW("hd_audioout_set(VOLUME):\n"
					 "    %s path_id(%#x)\n"
					 "        volume(%u)\n",
					 dif_return_dev_string(path_id), path_id, p_audout_info->param_volume.volume);
		} else {
			HD_AUDIOOUT_ERR("volume(%u) is an invalid value.\n", ((HD_AUDIOOUT_VOLUME *) p_param)->volume);
			return HD_ERR_PARAM;
		}
		break;

	case HD_AUDIOOUT_PARAM_CLEAR_BUF:
		hd_ret = audioout_clear_buf(dev_idx);
		break;

	default:
		break;
	}

	return hd_ret;
}

HD_RESULT hd_audioout_get(HD_PATH_ID path_id, HD_AUDIOOUT_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = AUDOUT_DEVID(self_id);
	INT inport_idx;
	AUDOUT_INFO *p_audout_info;

	//check parameters
	AUDIOOUT_CHECK_PATH_ID(path_id);
	AUDIOOUT_CHECK_P_PARAM(p_param);

	//store user values
	p_audout_info = &audioout_info[dev_idx];
	inport_idx = AUDOUT_OUTID(in_id);
	switch (id) {
	case HD_AUDIOOUT_PARAM_DEVCOUNT: {
		HD_DEVCOUNT *p_devcount = (HD_DEVCOUNT *) p_param;
		p_devcount->max_dev_count = audioout_get_devcount();
		break;
	}

	case HD_AUDIOOUT_PARAM_SYSCAPS: {
		HD_AUDIOOUT_SYSCAPS *p_auout = (HD_AUDIOOUT_SYSCAPS *) p_param;
		audioout_get_capability(self_id, p_auout);
		break;
	}

	case HD_AUDIOOUT_PARAM_SYSINFO:
		//TODO
		break;

	case HD_AUDIOOUT_PARAM_DEV_CONFIG:
		memcpy(p_param, &p_audout_info->dev_config, sizeof(HD_AUDIOOUT_DEV_CONFIG));
		break;

	case HD_AUDIOOUT_PARAM_DRV_CONFIG:
		memcpy(p_param, &p_audout_info->drv_config, sizeof(HD_AUDIOOUT_DRV_CONFIG));
		break;

	case HD_AUDIOOUT_PARAM_IN:
		memcpy(p_param, &p_audout_info->param_in[inport_idx], sizeof(HD_AUDIOOUT_IN));
		break;

	case HD_AUDIOOUT_PARAM_OUT:
		memcpy(p_param, &p_audout_info->param_out, sizeof(HD_AUDIOOUT_OUT));
		break;

	case HD_AUDIOOUT_PARAM_VOLUME:
		memcpy(p_param, &p_audout_info->param_volume, sizeof(HD_AUDIOOUT_VOLUME));
		break;

	case HD_AUDIOOUT_PARAM_CLEAR_BUF:
		HD_AUDIOOUT_ERR("audioout doesn't support get opertion for HD_AUDIOOUT_PARAM_CLEAR_BUF param.\n");
		return HD_ERR_PARAM;

	default:
		break;
	}

	return HD_OK;
}


HD_RESULT hd_audioout_push_in_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = AUDOUT_DEVID(self_id);

	/* check and set state */
	if (audioout_info[dev_idx].priv.path_id_state == HD_PATH_ID_CLOSED) {
		ret = HD_ERR_NOT_OPEN;
		goto state_exit;
	} else if (audioout_info[dev_idx].priv.path_id_state == HD_PATH_ID_ONGOING) {
		ret = HD_ERR_STATE;
		goto state_exit;
	} else {
		audioout_info[dev_idx].priv.path_id_state = HD_PATH_ID_ONGOING;
	}

	AUDIOOUT_CHECK_PATH_ID(path_id);
	AUDIOOUT_CHECK_HD_AUDIO_FRAME_SIZE(p_audio_frame);
	if (wait_ms < 0 || wait_ms > 10000) {
		HD_AUDIOOUT_ERR("Check the value wait_ms(%d) is out-of-range(0~%d).\n", wait_ms, 10000);
		ret = HD_ERR_NOT_SUPPORT;
		goto exit;
	}

	if ((p_audio_frame->phy_addr[0] == 0) ||
		((p_audio_frame->phy_addr[0] % audioio_hw_spec_info.in_align) != 0)) {
		HD_AUDIOOUT_ERR("Check HD_AUDIO_FRAME phy_addr[0] is NULL or not %d-bytes alignment.\n", audioio_hw_spec_info.in_align);
		ret = HD_ERR_NOT_SUPPORT;
		goto exit;
	}

	pthread_mutex_lock(&audioout_dev_lock[dev_idx]);
	ret = audioout_push_in_buf_p(self_id, in_id, p_audio_frame, wait_ms);
	pthread_mutex_unlock(&audioout_dev_lock[dev_idx]);

exit:
	audioout_info[dev_idx].priv.path_id_state = HD_PATH_ID_OPEN;
	return ret;

state_exit:
	return ret;
}


