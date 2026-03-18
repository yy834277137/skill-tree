/**
	@brief Source file of audiodec.\n
	This file contains the functions which is related to audio decode in the chip.

	@file audiodec.c

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include <string.h>
#include "hdal.h"
#include "vpd_ioctl.h"
#include "trig_ioctl.h"
#include "hdal.h"
#include "audiodec.h"

extern UINT32 audiodec_max_device_nr;
extern UINT32 audiodec_max_device_in;
extern UINT32 audiodec_max_device_out;
extern AUDDEC_INFO audiodec_info[MAX_AUDDEC_DEV];
AUDDEC_INTERNAL_PARAM auddec_dev_param[MAX_AUDDEC_DEV];


HD_RESULT audiodec_check_path_id_p(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);

	if (AUDDEC_DEVID(self_id) > audiodec_max_device_nr || ((int)AUDDEC_DEVID(self_id)) < 0) {
		HD_AUDIODEC_ERR("Error self_id(%#x)\n", self_id);
		return HD_ERR_NG;
	}
	if (AUDDEC_INID(in_id) > audiodec_max_device_in) {
		HD_AUDIODEC_ERR("Check input port error, port(%#x) max(%#x)\n", in_id, audiodec_max_device_in);
		return HD_ERR_NG;
	}
	if (AUDDEC_OUTID(out_id) > audiodec_max_device_out) {
		HD_AUDIODEC_ERR("Check output port error, port(%#x) max(%#x)\n", out_id, audiodec_max_device_out);
		return HD_ERR_NG;
	}
	return HD_OK;
}



INT check_audiodec_params_range(HD_AUDIODEC_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
	case HD_AUDIODEC_PARAM_DEVCOUNT:
		break;
	case HD_AUDIODEC_PARAM_SYSCAPS:
		break;
	case HD_AUDIODEC_PARAM_PATH_CONFIG:
		break;

	case HD_AUDIODEC_PARAM_IN: {
		HD_AUDIODEC_IN *p_auddec_in = (HD_AUDIODEC_IN *) p_param;
		if (p_auddec_in->codec_type != HD_AUDIO_CODEC_PCM &&
		    p_auddec_in->codec_type != HD_AUDIO_CODEC_AAC &&
		    p_auddec_in->codec_type != HD_AUDIO_CODEC_ADPCM &&
		    p_auddec_in->codec_type != HD_AUDIO_CODEC_ULAW &&
		    p_auddec_in->codec_type != HD_AUDIO_CODEC_ALAW) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIODEC_IN: codec_type(%d) is an invalid value.\n",
				 p_auddec_in->codec_type);
			goto exit;
		}
		if (p_auddec_in->sample_rate != HD_AUDIO_SR_8000 &&
		    p_auddec_in->sample_rate != HD_AUDIO_SR_11025 &&
		    p_auddec_in->sample_rate != HD_AUDIO_SR_12000 &&
		    p_auddec_in->sample_rate != HD_AUDIO_SR_16000 &&
		    p_auddec_in->sample_rate != HD_AUDIO_SR_22050 &&
		    p_auddec_in->sample_rate != HD_AUDIO_SR_24000 &&
		    p_auddec_in->sample_rate != HD_AUDIO_SR_32000 &&
		    p_auddec_in->sample_rate != HD_AUDIO_SR_44100 &&
		    p_auddec_in->sample_rate != HD_AUDIO_SR_48000) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIODEC_IN: sample_rate(%d) is an invalid value.\n",
				 p_auddec_in->sample_rate);
			goto exit;
		}
		if (p_auddec_in->sample_bit != HD_AUDIO_BIT_WIDTH_16) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIODEC_IN: sample_bit(%d) is an invalid value.\n",
				 p_auddec_in->sample_bit);
			goto exit;
		}
		if (p_auddec_in->mode != HD_AUDIO_SOUND_MODE_MONO &&
		    p_auddec_in->mode != HD_AUDIO_SOUND_MODE_STEREO) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIODEC_IN: mode(%d) is an invalid value.\n",
				 p_auddec_in->mode);
			goto exit;
		}
		break;
	}
	default:
		break;
	}
	return 0;
exit:
	return -1;

}

static VOID fill_default_params(HD_AUDIODEC_PARAM_ID id, VOID *p_param)
{
	switch (id) {
	case HD_AUDIODEC_PARAM_DEVCOUNT:
		break;

	case HD_AUDIODEC_PARAM_SYSCAPS:
		break;

	case HD_AUDIODEC_PARAM_PATH_CONFIG: {
		HD_AUDIODEC_PATH_CONFIG *p_path_cfg = (HD_AUDIODEC_PATH_CONFIG *) p_param;
		HD_AUDIODEC_POOL *p_data_pool = (HD_AUDIODEC_POOL *) p_path_cfg->data_pool;
		p_data_pool->ddr_id = 0;
		p_data_pool->frame_sample_size = 1024 * 2;
		p_data_pool->counts = HD_AUDIODEC_SET_COUNT(10, 0);
		p_data_pool->max_counts = HD_AUDIODEC_SET_COUNT(10, 0);
		p_data_pool->min_counts = 0;
		p_data_pool->mode = 0;    // pool mode, 0: auto, 1:enable, 2:disable
		break;
	}

	case HD_AUDIODEC_PARAM_IN: {
		HD_AUDIODEC_IN *p_auddec_in = (HD_AUDIODEC_IN *) p_param;
		p_auddec_in->codec_type = HD_AUDIO_CODEC_PCM;
		p_auddec_in->sample_rate = HD_AUDIO_SR_8000;
		p_auddec_in->sample_bit = HD_AUDIO_BIT_WIDTH_16;
		p_auddec_in->mode = HD_AUDIO_SOUND_MODE_MONO;

		break;
	}
	default:
		break;
	}
}

HD_RESULT audiodec_init_p(void)
{
	HD_RESULT hd_ret = HD_OK;
	UINT dev, port;
	memset(audiodec_info, 0, sizeof(audiodec_info));
	memset(auddec_dev_param, 0, sizeof(auddec_dev_param));

	for (dev = 0; dev < MAX_AUDDEC_DEV; dev++) {
		for (port = 0; port < MAX_AUDDEC_PORT; port++) {
			fill_default_params(HD_AUDIODEC_PARAM_IN, &audiodec_info[dev].port[port].dec_in);
			fill_default_params(HD_AUDIODEC_PARAM_PATH_CONFIG, &audiodec_info[dev].port[port].path_config);
			auddec_dev_param[dev].port[port].p_path_config = &audiodec_info[dev].port[port].path_config;
		}
	}

	return hd_ret;
}


HD_RESULT audiodec_uninit_p(void)
{
	HD_RESULT hd_ret = HD_OK;

	memset(audiodec_info, 0, sizeof(audiodec_info));
	memset(auddec_dev_param, 0, sizeof(auddec_dev_param));

	return hd_ret;

}


HD_RESULT audiodec_new_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_BS *p_audio_bitstream)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}

HD_RESULT audiodec_push_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_BS *p_audio_bitstream, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}

HD_RESULT audiodec_release_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_BS *p_audio_bitstream)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}

HD_RESULT audiodec_pull_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}

HD_RESULT audiodec_release_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_FRAME *p_audio_frame)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}


AUDDEC_INTERNAL_PARAM *audiodec_get_param_p(HD_DAL self_id)
{
	int dev_idx = AUDDEC_DEVID(self_id);
	if (dev_idx > MAX_AUDDEC_DEV || dev_idx < 0) {
		HD_AUDIODEC_ERR("Error self_id(%d)\n", self_id);
		goto err_ext;
	}
	return &auddec_dev_param[dev_idx];
err_ext:
	return NULL;

}

