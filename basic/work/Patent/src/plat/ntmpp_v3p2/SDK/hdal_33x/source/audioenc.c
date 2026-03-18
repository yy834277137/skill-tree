/**
	@brief Source file of audioenc.\n
	This file contains the functions which is related to audio encode in the chip.

	@file audioenc.c

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include <string.h>
#include "hdal.h"
#include "vpd_ioctl.h"
#include "trig_ioctl.h"
#include "hdal.h"
#include "audioenc.h"

extern UINT32 audioenc_max_device_nr;
extern UINT32 audioenc_max_device_in;
extern UINT32 audioenc_max_device_out;
extern AUDENC_INFO audioenc_info[MAX_AUDENC_DEV];
AUDENC_INTERNAL_PARAM audenc_dev_param[MAX_AUDENC_DEV];

HD_RESULT audioenc_check_path_id_p(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);

	if (AUDENC_DEVID(self_id) > audioenc_max_device_nr || ((int)AUDENC_DEVID(self_id)) < 0) {
		HD_AUDIOENC_ERR("Error self_id(%#x)\n", self_id);
		return HD_ERR_NG;
	}
	if (AUDENC_INID(in_id) > audioenc_max_device_in) {
		HD_AUDIOENC_ERR("Check input port error, port(%#x) max(%#x)\n", in_id, audioenc_max_device_in);
		return HD_ERR_NG;
	}
	if (AUDENC_OUTID(out_id) > audioenc_max_device_out) {
		HD_AUDIOENC_ERR("Check output port error, port(%#x) max(%#x)\n", out_id, audioenc_max_device_out);
		return HD_ERR_NG;
	}
	return HD_OK;
}


INT check_audioenc_params_range(HD_AUDIOENC_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
	//TODO
	case HD_AUDIOENC_PARAM_DEVCOUNT:
		break;
	case HD_AUDIOENC_PARAM_SYSCAPS:
		break;
	case HD_AUDIOENC_PARAM_BUFINFO:
		break;
	case HD_AUDIOENC_PARAM_PATH_CONFIG:
		break;
	case HD_AUDIOENC_PARAM_IN:
		break;

	case HD_AUDIOENC_PARAM_OUT: {
		HD_AUDIOENC_OUT *p_audenc_out = (HD_AUDIOENC_OUT *) p_param;
		if (p_audenc_out->codec_type != HD_AUDIO_CODEC_PCM &&
		    p_audenc_out->codec_type != HD_AUDIO_CODEC_AAC &&
		    p_audenc_out->codec_type != HD_AUDIO_CODEC_ADPCM &&
		    p_audenc_out->codec_type != HD_AUDIO_CODEC_ULAW &&
		    p_audenc_out->codec_type != HD_AUDIO_CODEC_ALAW) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOENC_OUT: codec_type(%d) is an invalid value.\n",
				 p_audenc_out->codec_type);
			goto exit;
		}
		if (p_audenc_out->codec_type == HD_AUDIO_CODEC_AAC) {
			/*if (p_audenc_out->bitrate < 1) {
				snprintf(p_ret_string, max_str_len,
						"HD_AUDIOENC_OUT: bitrate(%lu) is out-of-range.\n",
						p_audenc_out->bitrate);
				goto exit;
			}*/
		}
		break;
	}
	default:
		goto exit;
	}
	return 0;
exit:
	return -1;

}

static VOID fill_default_params(HD_AUDIOENC_PARAM_ID id, VOID *p_param)
{
	switch (id) {
	//TODO
	case HD_AUDIOENC_PARAM_DEVCOUNT:
		break;
	case HD_AUDIOENC_PARAM_SYSCAPS:
		break;
	case HD_AUDIOENC_PARAM_PATH_CONFIG:
		break;
	case HD_AUDIOENC_PARAM_IN:
		break;
	case HD_AUDIOENC_PARAM_OUT: {
		HD_AUDIOENC_OUT *p_audenc_out = (HD_AUDIOENC_OUT *) p_param;
		p_audenc_out->codec_type = HD_AUDIO_CODEC_PCM;
		//p_audenc_out->bitrate = 32000;
		break;
	}
	default:
		break;
	}
}


HD_RESULT audioenc_init_p(void)
{
	HD_RESULT hd_ret = HD_OK;
	UINT dev, port;

	memset(audioenc_info, 0, sizeof(audioenc_info));
	memset(audenc_dev_param, 0, sizeof(audenc_dev_param));

	for (dev = 0; dev < MAX_AUDENC_DEV; dev++) {
		for (port = 0; port < MAX_AUDENC_PORT; port++) {
			fill_default_params(HD_AUDIOENC_PARAM_OUT, &audioenc_info[dev].port[port].enc_out);
		}
	}

	return hd_ret;
}

HD_RESULT audioenc_uninit_p(void)
{
	HD_RESULT hd_ret = HD_OK;

	memset(audioenc_info, 0, sizeof(audioenc_info));
	memset(audenc_dev_param, 0, sizeof(audenc_dev_param));

	return hd_ret;
}


HD_RESULT audioenc_new_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}

HD_RESULT audioenc_push_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}

HD_RESULT audioenc_release_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}

HD_RESULT audioenc_pull_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_BS *p_audio_bitstream, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}

HD_RESULT audioenc_release_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_BS *p_audio_bitstream)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}



AUDENC_INTERNAL_PARAM *audioenc_get_param_p(HD_DAL self_id)
{
	int dev_idx = AUDENC_DEVID(self_id);
	if (dev_idx > MAX_AUDENC_DEV || dev_idx < 0) {
		HD_AUDIOENC_ERR("Error self_id(%d)\n", self_id);
		goto err_ext;
	}
	return &audenc_dev_param[dev_idx];
err_ext:
	return NULL;

}
