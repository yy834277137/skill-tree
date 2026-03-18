/**
	@brief Header file of audio encode module.\n
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
#include "hd_logger.h"
#include "hdal.h"
#include "bind.h"
#include "dif.h"
#include "audioenc.h"
#include "logger.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define AUDIOENC_CHECK_PATH_ID(path_id)                                            \
	do {                                                                           \
		if (audioenc_check_path_id_p(path_id) != HD_OK) {                          \
			HD_AUDIOENC_ERR("Error self_id(%d)\n", (self_id));                 \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define AUDIOENC_CHECK_OUT_ID(_out_id)                                             \
	do {                                                                           \
		HD_DAL self_id = HD_GET_DEV(_out_id);                                      \
		HD_IO out_id = HD_GET_OUT(_out_id);                                        \
		if (AUDENC_DEVID(self_id) > MAX_AUDENC_DEV || AUDENC_DEVID(self_id) < 0) { \
			HD_AUDIOENC_ERR("Error self_id(%#x) from out_id(%#x)\n", (self_id), (_out_id)); \
			return HD_ERR_NG;                                                      \
		}                                                                          \
		if ((out_id) >= HD_OUT_BASE && (out_id) < (HD_OUT_BASE + MAX_AUDENC_PORT)) {        \
		} else {                                                                   \
			HD_AUDIOENC_ERR("Error out_port(%#x) from out_id(%#x)\n", (out_id), (_out_id)); \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)

#define AUDIOENC_CHECK_P_PARAM(p_param)                                            \
	do {                                                                           \
		if ((p_param) == NULL) {                                                   \
			HD_AUDIOENC_ERR("Error p_param(%p)\n", (p_param));                 \
			return HD_ERR_NG;                                                      \
		}                                                                          \
	} while (0)


/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern AUDENC_INTERNAL_PARAM audenc_dev_param[MAX_AUDENC_DEV];
extern int audioin_align;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
UINT32 audioenc_max_device_nr;
UINT32 audioenc_max_device_in;
UINT32 audioenc_max_device_out;
AUDENC_INFO audioenc_info[MAX_AUDENC_DEV];

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

HD_RESULT hd_audioenc_init(VOID)
{
	HD_RESULT ret;

	audioenc_max_device_nr = 1;
	audioenc_max_device_in = MAX_AUDENC_PORT;
	audioenc_max_device_out = MAX_AUDENC_PORT;
	audioenc_init_p();
	ret = bd_device_init(HD_DAL_AUDIOENC_BASE, audioenc_max_device_nr, audioenc_max_device_in, audioenc_max_device_out);
	HD_AUDIOENC_FLOW("hd_audioenc_init\n");
	return ret;
}

HD_RESULT hd_audioenc_uninit(VOID)
{
	HD_RESULT ret;

	ret = bd_device_uninit(HD_DAL_AUDIOENC_BASE);

	audioenc_uninit_p();
	HD_AUDIOENC_FLOW("hd_audioenc_uninit\n");
	return ret;
}

HD_RESULT hd_audioenc_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID *p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(_in_id);
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_RESULT ret;

	if (p_path_id == NULL) {
		HD_AUDIOENC_ERR(" The p_path_id is NULL.\n");
		return HD_ERR_PARAM;
	}

	if (bd_get_dev_baseid(self_id) != HD_DAL_AUDIOENC_BASE) {
		HD_AUDIOENC_ERR(" The device of path_id(%#x) is not audioenc.\n", *p_path_id);
		ret = HD_ERR_DEV;
		goto ext;
	}

	if (in_dev_id != self_id) {
		HD_AUDIOENC_ERR(" in(%#x) and out(%#x) aren't the same device.\n", _in_id, _out_id);
		ret = HD_ERR_IO;
		goto ext;
	}

	ret = bd_device_open(self_id, out_id, in_id, p_path_id);

ext:
	HD_AUDIOENC_FLOW("hd_audioenc_open:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u)\n",
			 dif_return_dev_string(*p_path_id), *p_path_id, self_id, out_id, in_id);
	return ret;
}

HD_RESULT hd_audioenc_close(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_RESULT ret;

	AUDIOENC_CHECK_PATH_ID(path_id);
	ret = bd_device_close(path_id);
	HD_AUDIOENC_FLOW("hd_audioenc_close:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_audioenc_bind(HD_OUT_ID out_id, HD_IN_ID dest_in_id)
{
	HD_AUDIOENC_ERR("audioenc doesn't bind to any other module. out_id(%#x) dest_in_id(%#x)\n", out_id, dest_in_id);
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audioenc_unbind(HD_OUT_ID out_id)
{
	HD_AUDIOENC_ERR("audioenc doesn't unbind from any other module. out_id(%#x)\n", out_id);
	return HD_OK;
}

HD_RESULT hd_audioenc_start(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);

	AUDIOENC_CHECK_PATH_ID(path_id);
	ret = bd_start_list(&path_id, 1);
	HD_AUDIOENC_FLOW("hd_audioenc_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_audioenc_stop(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);

	AUDIOENC_CHECK_PATH_ID(path_id);
	ret = bd_stop_list(&path_id, 1);
	HD_AUDIOENC_FLOW("hd_audioenc_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	return ret;
}

HD_RESULT hd_audioenc_start_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;

	if (path_id == NULL) {
		HD_AUDIOENC_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}
	for (i = 0; i < nr; i++) {
		ret = audioenc_check_path_id_p(path_id[i]);
		if (ret != HD_OK)
			goto ext;
	}
	ret = bd_start_list(path_id, nr);
ext:
	return ret;
}

HD_RESULT hd_audioenc_stop_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret;
	UINT i;

	if (path_id == NULL) {
		HD_AUDIOENC_ERR(" The path_id is NULL.\n");
		ret = HD_ERR_PARAM;
		goto ext;
	}
	for (i = 0; i < nr; i++) {
		ret = audioenc_check_path_id_p(path_id[i]);
		if (ret != HD_OK)
			goto ext;
	}

	ret = bd_stop_list(path_id, nr);

ext:
	return ret;
}


HD_RESULT hd_audioenc_set(HD_PATH_ID path_id, HD_AUDIOENC_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT dev_idx = AUDENC_DEVID(self_id);
	INT outport_idx, ret;
	AUDENC_INFO *p_audenc_info;
	CHAR msg_string[256] = {0};

	//check parameters
	AUDIOENC_CHECK_PATH_ID(path_id);
	AUDIOENC_CHECK_P_PARAM(p_param);

	ret = check_audioenc_params_range(id, p_param, msg_string, 256);
	if (ret < 0) {
		HD_AUDIOENC_ERR("Wrong value. %s\n", msg_string);
		return HD_ERR_PARAM;
	}

	//store user values
	p_audenc_info = &audioenc_info[dev_idx];
	outport_idx = AUDENC_OUTID(out_id);
	switch (id) {
	case HD_AUDIOENC_PARAM_DEVCOUNT:
		HD_AUDIOENC_ERR("audioenc doesn't support set opertion for HD_AUDIOENC_PARAM_DEVCOUNT param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIOENC_PARAM_SYSCAPS:
		HD_AUDIOENC_ERR("audioenc doesn't support set opertion for HD_AUDIOENC_PARAM_SYSCAPS param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIOENC_PARAM_BUFINFO:
		HD_AUDIOENC_ERR("audioenc doesn't support set/get opertion for HD_AUDIOENC_PARAM_BUFINFO param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIOENC_PARAM_PATH_CONFIG:
		memcpy(&p_audenc_info->port[outport_idx].path_config, p_param, sizeof(HD_AUDIOENC_PATH_CONFIG));
		audenc_dev_param[dev_idx].port[outport_idx].p_path_config = &p_audenc_info->port[outport_idx].path_config;
		HD_AUDIOENC_FLOW("hd_audioenc_set(PATH_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        codec(%#x) sample_rate(%#x) sample_bit(%#x) mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id,  p_audenc_info->port[outport_idx].path_config.max_mem.codec_type,
				 p_audenc_info->port[outport_idx].path_config.max_mem.sample_rate,
				 p_audenc_info->port[outport_idx].path_config.max_mem.sample_bit,
				 p_audenc_info->port[outport_idx].path_config.max_mem.mode);
		break;

	case HD_AUDIOENC_PARAM_IN:
		memcpy(&p_audenc_info->port[outport_idx].enc_in, p_param, sizeof(HD_AUDIOENC_IN));
		audenc_dev_param[dev_idx].port[outport_idx].p_enc_in = &p_audenc_info->port[outport_idx].enc_in;
		HD_AUDIOENC_FLOW("hd_audioenc_set(IN):\n"
				 "    %s path_id(%#x)\n"
				 "        sample_rate(%#x) sample_bit(%#x) mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id, p_audenc_info->port[outport_idx].enc_in.sample_rate,
				 p_audenc_info->port[outport_idx].enc_in.sample_bit,
				 p_audenc_info->port[outport_idx].enc_in.mode);
		break;

	case HD_AUDIOENC_PARAM_OUT:
		memcpy(&p_audenc_info->port[outport_idx].enc_out, p_param, sizeof(HD_AUDIOENC_OUT));
		audenc_dev_param[dev_idx].port[outport_idx].p_enc_out = &p_audenc_info->port[outport_idx].enc_out;
		HD_AUDIOENC_FLOW("hd_audioenc_set(OUT):\n"
				 "    %s path_id(%#x)\n"
				 "        codec(%#x) aac_adts(%u)\n",
				 dif_return_dev_string(path_id), path_id, p_audenc_info->port[outport_idx].enc_out.codec_type,
				 p_audenc_info->port[outport_idx].enc_out.aac_adts);
		break;
	default:
		break;
	}

	return HD_OK;
}


HD_RESULT hd_audioenc_get(HD_PATH_ID path_id, HD_AUDIOENC_PARAM_ID id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT dev_idx = AUDENC_DEVID(self_id);
	INT outport_idx;
	AUDENC_INFO *p_audenc_info;

	//check parameters
	AUDIOENC_CHECK_PATH_ID(path_id);
	AUDIOENC_CHECK_P_PARAM(p_param);

	//retrieve user values
	p_audenc_info = &audioenc_info[dev_idx];
	outport_idx = AUDENC_OUTID(out_id);
	switch (id) {
	case HD_AUDIOENC_PARAM_DEVCOUNT: {
		HD_DEVCOUNT *p_devcount = (HD_DEVCOUNT *) p_param;
		p_devcount->max_dev_count = 1;
		break;
	}

	case HD_AUDIOENC_PARAM_SYSCAPS: {
		UINT32 i;
		HD_AUDIOENC_SYSCAPS *p_aucap = (HD_AUDIOENC_SYSCAPS *) p_param;
		p_aucap->dev_id = self_id;
		p_aucap->chip_id = 0;
		p_aucap->max_in_count = audioenc_max_device_in;
		p_aucap->max_out_count = audioenc_max_device_out;
		p_aucap->dev_caps = HD_CAPS_LISTFUNC;
		for (i = 0; i < HD_AUDIOENC_MAX_IN; i ++) {
			p_aucap->in_caps[i] = HD_AUDIO_CAPS_BITS_16
					      | HD_AUDIO_CAPS_CH_MONO;
		}
		for (i = 0; i < HD_AUDIOENC_MAX_OUT; i ++) {
			p_aucap->out_caps[i] = HD_AUDIOENC_CAPS_PCM
					       | HD_AUDIOENC_CAPS_ULAW
					       | HD_AUDIOENC_CAPS_ALAW;
		}
		break;
	}

	case HD_AUDIOENC_PARAM_BUFINFO:
		HD_AUDIOENC_ERR("audioenc doesn't support set/get opertion for HD_AUDIOENC_PARAM_BUFINFO param.\n");
		return HD_ERR_PARAM;

	case HD_AUDIOENC_PARAM_PATH_CONFIG:
		memcpy(p_param, &p_audenc_info->port[outport_idx].path_config, sizeof(HD_AUDIOENC_PATH_CONFIG));
		HD_AUDIOENC_FLOW("hd_audioenc_get(PATH_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        codec(%#x) sample_rate(%#x) sample_bit(%#x) mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id,  p_audenc_info->port[outport_idx].path_config.max_mem.codec_type,
				 p_audenc_info->port[outport_idx].path_config.max_mem.sample_rate,
				 p_audenc_info->port[outport_idx].path_config.max_mem.sample_bit,
				 p_audenc_info->port[outport_idx].path_config.max_mem.mode);
		break;

	case HD_AUDIOENC_PARAM_IN:
		memcpy(p_param, &p_audenc_info->port[outport_idx].enc_in, sizeof(HD_AUDIOENC_IN));
		HD_AUDIOENC_FLOW("hd_audioenc_get(IN):\n"
				 "    %s path_id(%#x)\n"
				 "        sample_rate(%#x) sample_bit(%#x) mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id, p_audenc_info->port[outport_idx].enc_in.sample_rate,
				 p_audenc_info->port[outport_idx].enc_in.sample_bit,
				 p_audenc_info->port[outport_idx].enc_in.mode);
		break;

	case HD_AUDIOENC_PARAM_OUT:
		memcpy(p_param, &p_audenc_info->port[outport_idx].enc_out, sizeof(HD_AUDIOENC_OUT));
		HD_AUDIOENC_FLOW("hd_audioenc_get(OUT):\n"
				 "    %s path_id(%#x)\n"
				 "        codec(%#x) aac_adts(%u)\n",
				 dif_return_dev_string(path_id), path_id, p_audenc_info->port[outport_idx].enc_out.codec_type,
				 p_audenc_info->port[outport_idx].enc_out.aac_adts);
		break;
	default:
		break;
	}

	return HD_OK;
}


HD_RESULT hd_audioenc_push_in_buf(HD_PATH_ID path_id, HD_AUDIO_FRAME *p_in_audio_frame, HD_AUDIO_BS *p_user_out_audio_bitstream, INT32 wait_ms)
{
	HD_AUDIOENC_ERR("Not support for %s, path_id(%#x)\n", __func__, path_id);
	return HD_ERR_NOT_SUPPORT;
}

HD_RESULT hd_audioenc_pull_out_buf(HD_PATH_ID path_id, HD_AUDIO_BS *p_audio_bitstream, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);

	hd_ret = dif_pull_out_buffer(self_id, out_id, p_audio_bitstream, wait_ms);
	if (hd_ret == HD_OK) {
		if (p_audio_bitstream->phy_addr == 0) {
			HD_VIDEOENC_ERR("dif_pull_out_buffer fail\n");
			return HD_ERR_NG;
		}
	} else if (hd_ret == HD_ERR_NOT_AVAIL) {
		HD_AUDIOENC_ERR("Not support for %s, path_id(%#x)\n", __func__, path_id);
		return HD_ERR_NOT_SUPPORT;
	} else {
		HD_AUDIOENC_ERR("%s: hd_audioenc_pull_out_buf fail, path_id(%#x)\n", __func__, path_id);
		return HD_ERR_NG;
	}

	return HD_OK;
}

HD_RESULT hd_audioenc_release_out_buf(HD_PATH_ID path_id, HD_AUDIO_BS *p_audio_bitstream)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);

	hd_ret = dif_release_out_buffer(self_id, out_id, p_audio_bitstream);
	if (hd_ret == HD_OK) {
		p_audio_bitstream->phy_addr = 0;
	} else if (hd_ret == HD_ERR_NOT_AVAIL) {
		HD_AUDIOENC_ERR("Not support for %s, path_id(%#x)\n", __func__, path_id);
		return HD_ERR_NOT_SUPPORT;
	} else {
		HD_AUDIOENC_ERR("%s: hd_audioenc_release_out_buf fail, path_id(%#x)\n", __func__, path_id);
		return HD_ERR_NG;
	}

	return HD_OK;
}

HD_RESULT hd_audioenc_poll_list(HD_AUDIOENC_POLL_LIST *p_poll, UINT32 nr, INT32 wait_ms)
{
	if (p_poll == NULL) {
		HD_AUDIOENC_ERR(" The p_poll is NULL.\n");
		return HD_ERR_PARAM;
	}
	return audio_poll_list(p_poll, nr, wait_ms);
}

HD_RESULT hd_audioenc_recv_list(HD_AUDIOENC_RECV_LIST *p_audioenc_bs, UINT32 nr)
{
	UINT i;

	if (p_audioenc_bs == NULL) {
		HD_AUDIOENC_ERR(" The p_audioenc_bs is NULL.\n");
		return HD_ERR_PARAM;
	}

	for (i = 0; i < nr; i++) {
		if ((((unsigned long)p_audioenc_bs[i].user_bs.p_user_buf) % audioin_align) != 0) {
			HD_AUDIOENC_ERR("The p_audioenc_bs address is not %d-bytes alignment.\n", audioin_align);
			return HD_ERR_PARAM;
		}
	}

	return audio_recv_list(p_audioenc_bs, nr);
}


