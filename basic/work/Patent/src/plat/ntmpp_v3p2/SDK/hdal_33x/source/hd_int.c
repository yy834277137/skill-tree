/**
        @brief Source code of internal function.\n
        This file contains common internal functions.

        @file hd_int.c

        @ingroup mhdal

        @note Nothing.

        Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_type.h"
#include "hd_videoenc.h"

int _hd_video_pxlfmt_str(HD_VIDEO_PXLFMT pxlfmt, CHAR *p_str, INT str_len)
{
	switch (pxlfmt) {
	case HD_VIDEO_PXLFMT_RAW8:
		snprintf(p_str, str_len, "RAW8");
		break;
	case HD_VIDEO_PXLFMT_RAW10:
		snprintf(p_str, str_len, "RAW10");
		break;
	case HD_VIDEO_PXLFMT_RAW12:
		snprintf(p_str, str_len, "RAW12");
		break;
	case HD_VIDEO_PXLFMT_RAW14:
		snprintf(p_str, str_len, "RAW14");
		break;
	case HD_VIDEO_PXLFMT_RAW16:
		snprintf(p_str, str_len, "RAW16");
		break;
	case HD_VIDEO_PXLFMT_YUV420:
		snprintf(p_str, str_len, "YUV420");
		break;
	case HD_VIDEO_PXLFMT_YUV422:
		snprintf(p_str, str_len, "YUV422");
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX1_H264:
		snprintf(p_str, str_len, "NVX1");
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX1_H265:
		snprintf(p_str, str_len, "NVX1");
		break;
	case HD_VIDEO_PXLFMT_YUV422_ONE:
		snprintf(p_str, str_len, "YUV422_ONE");
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX3:
		snprintf(p_str, str_len, "YUV420_NVX3");
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX4:
		snprintf(p_str, str_len, "YUV420_NVX4");
		break;
	default:
		snprintf(p_str, str_len, "-");
		return (-1);
	}
	return 0;
}

int _hd_video_device_str(HD_DAL device_id, CHAR *p_str, INT str_len)
{
	switch (device_id) {
	case HD_VIDEO_PXLFMT_RAW8:
		snprintf(p_str, str_len, "RAW8");
		break;
	case HD_VIDEO_PXLFMT_RAW10:
		snprintf(p_str, str_len, "RAW10");
		break;
	case HD_VIDEO_PXLFMT_RAW12:
		snprintf(p_str, str_len, "RAW12");
		break;
	case HD_VIDEO_PXLFMT_RAW14:
		snprintf(p_str, str_len, "RAW14");
		break;
	case HD_VIDEO_PXLFMT_RAW16:
		snprintf(p_str, str_len, "RAW16");
		break;
	case HD_VIDEO_PXLFMT_YUV420:
		snprintf(p_str, str_len, "YUV420");
		break;
	case HD_VIDEO_PXLFMT_YUV422:
		snprintf(p_str, str_len, "YUV422");
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX1_H264:
		snprintf(p_str, str_len, "NVX1");
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX1_H265:
		snprintf(p_str, str_len, "NVX1");
		break;
	case HD_VIDEO_PXLFMT_YUV422_ONE:
		snprintf(p_str, str_len, "YUV422_ONE");
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX3:
		snprintf(p_str, str_len, "YUV420_NVX3");
		break;
	default:
		snprintf(p_str, str_len, "-");
		return (-1);
	}
	return 0;
}

int _hd_video_codec_str(HD_VIDEO_CODEC code_type, CHAR *p_str, INT str_len)
{
	if (code_type == HD_CODEC_TYPE_JPEG)
		snprintf(p_str, str_len, "JPEG");
	else if (code_type == HD_CODEC_TYPE_H264)
		snprintf(p_str, str_len, "H264");
	else if (code_type == HD_CODEC_TYPE_H265)
		snprintf(p_str, str_len, "H265");
	else
		snprintf(p_str, str_len, "-");
	return 0;
}

int _hd_video_svc_str(HD_SVC_LAYER_TYPE svc_layer, CHAR *p_str, INT str_len)
{
	if (svc_layer == HD_SVC_LAYER_1X)
		snprintf(p_str, str_len, "svc_1x");
	else if (svc_layer == HD_SVC_LAYER_2X)
		snprintf(p_str, str_len, "svc_2x");
	else if (svc_layer == HD_SVC_LAYER_4X)
		snprintf(p_str, str_len, "svc_4x");
	else
		snprintf(p_str, str_len, "-");
	return 0;
}

int _hd_video_profile_str(HD_VIDEOENC_PROFILE profile, CHAR *p_str, INT str_len)
{
	if (profile == HD_H264E_BASELINE_PROFILE)
		snprintf(p_str, str_len, "BASE");
	else if (profile == HD_H264E_MAIN_PROFILE)
		snprintf(p_str, str_len, "MAIN");
	else if (profile == HD_H264E_HIGH_PROFILE)
		snprintf(p_str, str_len, "HIGH");
	else
		snprintf(p_str, str_len, "-");
	return 0;
}

int _hd_video_entropy_str(HD_VIDEOENC_CODING coding, CHAR *p_str, INT str_len)
{
	if (coding == HD_H264E_CAVLC_CODING)
		snprintf(p_str, str_len, "CAVLC");
	else if (coding == HD_H264E_CABAC_CODING)
		snprintf(p_str, str_len, "CABAC");
	else if (coding == HD_H265E_CABAC_CODING)
		snprintf(p_str, str_len, "CABAC");
	else
		snprintf(p_str, str_len, "-");
	return 0;
}

int _hd_video_rc_mode(HD_VIDEOENC_RC_MODE rc_mode, CHAR *p_str, INT str_len)
{
	if (rc_mode == HD_RC_MODE_CBR)
		snprintf(p_str, str_len, "CBR");
	else if (rc_mode == HD_RC_MODE_VBR)
		snprintf(p_str, str_len, "VBR");
	else if (rc_mode == HD_RC_MODE_FIX_QP)
		snprintf(p_str, str_len, "FIXQ");
	else if (rc_mode == HD_RC_MODE_EVBR)
		snprintf(p_str, str_len, "EVBR");
	else
		snprintf(p_str, str_len, "-");
	return 0;
}

int _hd_audio_codec_str(HD_AUDIO_CODEC code_type, CHAR *p_str, INT str_len)
{
	if (code_type == HD_AUDIO_CODEC_PCM)
		snprintf(p_str, str_len, "PCM");
	else if (code_type == HD_AUDIO_CODEC_AAC)
		snprintf(p_str, str_len, "AAC");
	else if (code_type == HD_AUDIO_CODEC_ADPCM)
		snprintf(p_str, str_len, "ADPCM");
	else if (code_type == HD_AUDIO_CODEC_ULAW)
		snprintf(p_str, str_len, "ULAW");
	else if (code_type == HD_AUDIO_CODEC_ALAW)
		snprintf(p_str, str_len, "ALAW");
	else
		snprintf(p_str, str_len, "-");
	return 0;
}
