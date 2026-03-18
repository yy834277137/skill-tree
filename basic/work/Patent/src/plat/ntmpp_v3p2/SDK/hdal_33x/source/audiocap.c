/**
	@brief Source file of audiocap.\n
	This file contains the functions which is related to audio capture in the chip.

	@file audiocap.c

    @ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include <string.h>
#include <sys/ioctl.h>
#include "hdal.h"
#include "vpd_ioctl.h"
#include "trig_ioctl.h"
#include "hdal.h"
#include "audiocap.h"
#include "pif.h"
#include "pif_ioctl.h"
#include "kflow_audioio.h"


extern int pif_alloc_user_fd_for_hdal(vpd_em_type_t type, int chip, int engine, int minor);
//extern unsigned int pif_create_queue_for_hdal(void);
extern uintptr_t pif_create_queue_for_hdal(void);
//extern int pif_destroy_queue_for_hdal(unsigned int queue_handle);
extern int pif_destroy_queue_for_hdal(uintptr_t queue_handle);

extern UINT32 audiocap_max_device_nr;
extern UINT32 audiocap_max_device_in;
extern UINT32 audiocap_max_device_out;
extern int vpd_fd;
extern AUDCAP_INFO audiocap_info[MAX_AUDCAP_DEV];
extern vpd_sys_info_t platform_sys_Info;
AUDCAP_INTERNAL_PARAM audcap_dev_param[MAX_AUDCAP_DEV];
int audioin_align = 4;
static INT audioio_ioctl = -1;
static pthread_mutex_t audiocap_mutex_lock = PTHREAD_MUTEX_INITIALIZER;
extern KFLOW_AUDIOIO_HW_SPEC_INFO audioio_hw_spec_info;
extern INT is_audioio_hw_spec_info_ready;

HD_RESULT audiocap_check_path_id_p(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);

	if ((int) audiocap_max_device_nr == -1) {
		HD_AUDIOCAPTURE_ERR("Error, no audiocap device.\n");
		return HD_ERR_NG;
	}

	if (AUDCAP_DEVID(self_id) > audiocap_max_device_nr || ((int)AUDCAP_DEVID(self_id)) < 0) {
		HD_AUDIOCAPTURE_ERR("Error self_id(%#x)\n", self_id);
		return HD_ERR_NG;
	}
	if (ctrl_id == HD_CTRL) {
		goto ext;
	}

	if (AUDCAP_INID(in_id) > audiocap_max_device_in) {
		HD_AUDIOCAPTURE_ERR("Check input port error, port(%#x) max(%#x)\n", in_id, audiocap_max_device_in);
		return HD_ERR_NG;
	}
	if (AUDCAP_OUTID(out_id) > audiocap_max_device_out) {
		HD_AUDIOCAPTURE_ERR("Check output port error, port(%#x) max(%#x)\n", out_id, audiocap_max_device_out);
		return HD_ERR_NG;
	}
ext:
	return HD_OK;
}


HD_RESULT audiocap_pull_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}

HD_RESULT audiocap_release_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_AUDIO_FRAME *p_audio_frame)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}



INT check_audiocap_params_range(HD_AUDIOCAP_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
	//TODO
	case HD_AUDIOCAP_PARAM_DEVCOUNT:
		break;
	case HD_AUDIOCAP_PARAM_SYSCAPS:
		break;
	case HD_AUDIOCAP_PARAM_SYSINFO:
		break;
	case HD_AUDIOCAP_PARAM_DEV_CONFIG:
		{
			HD_AUDIOCAP_DEV_CONFIG *p_dev_cfg = (HD_AUDIOCAP_DEV_CONFIG *) p_param;
			if (p_dev_cfg->data_pool[0].frame_sample_size % 16) {
				snprintf(p_ret_string, max_str_len,
					 "HD_AUDIOCAP_PARAM_DEV_CONFIG: frame_sample_size(%d) must be 16x alignment.\n",
					 p_dev_cfg->data_pool[0].frame_sample_size);
				goto exit;
			}
		}
		break;
	case HD_AUDIOCAP_PARAM_DRV_CONFIG:
		break;

	case HD_AUDIOCAP_PARAM_IN: {
		HD_AUDIOCAP_IN *p_audcap_in = (HD_AUDIOCAP_IN *) p_param;
		if (p_audcap_in->sample_rate != HD_AUDIO_SR_8000 &&
		    p_audcap_in->sample_rate != HD_AUDIO_SR_11025 &&
		    p_audcap_in->sample_rate != HD_AUDIO_SR_12000 &&
		    p_audcap_in->sample_rate != HD_AUDIO_SR_16000 &&
		    p_audcap_in->sample_rate != HD_AUDIO_SR_22050 &&
		    p_audcap_in->sample_rate != HD_AUDIO_SR_24000 &&
		    p_audcap_in->sample_rate != HD_AUDIO_SR_32000 &&
		    p_audcap_in->sample_rate != HD_AUDIO_SR_44100 &&
		    p_audcap_in->sample_rate != HD_AUDIO_SR_48000) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOCAP_IN: sample_rate(%d) is an invalid value.\n",
				 p_audcap_in->sample_rate);
			goto exit;
		}
		if (p_audcap_in->sample_bit != HD_AUDIO_BIT_WIDTH_16) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOCAP_IN: sample_bit(%d) is an invalid value.\n",
				 p_audcap_in->sample_bit);
			goto exit;
		}
		if (p_audcap_in->mode != HD_AUDIO_SOUND_MODE_MONO &&
		    p_audcap_in->mode != HD_AUDIO_SOUND_MODE_STEREO) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOCAP_IN: mode(%d) is an invalid value.\n",
				 p_audcap_in->mode);
			goto exit;
		}
		/*if (p_audcap_in->frame_sample < 130 || p_audcap_in->frame_sample > 8192) {
			snprintf(p_ret_string, max_str_len,
					"HD_AUDIOCAP_IN: frame_sample(%d) is out-of-range(130~8192).\n",
					p_audcap_in->frame_sample);
			goto exit;
		}*/
		break;
	}

	case HD_AUDIOCAP_PARAM_OUT: {
		HD_AUDIOCAP_OUT *p_audcap_out = (HD_AUDIOCAP_OUT *) p_param;
		if (p_audcap_out->sample_rate != HD_AUDIO_SR_8000 &&
		    p_audcap_out->sample_rate != HD_AUDIO_SR_11025 &&
		    p_audcap_out->sample_rate != HD_AUDIO_SR_12000 &&
		    p_audcap_out->sample_rate != HD_AUDIO_SR_16000 &&
		    p_audcap_out->sample_rate != HD_AUDIO_SR_22050 &&
		    p_audcap_out->sample_rate != HD_AUDIO_SR_24000 &&
		    p_audcap_out->sample_rate != HD_AUDIO_SR_32000 &&
		    p_audcap_out->sample_rate != HD_AUDIO_SR_44100 &&
		    p_audcap_out->sample_rate != HD_AUDIO_SR_48000) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOCAP_OUT: sample_rate(%d) is an invalid value.\n",
				 p_audcap_out->sample_rate);
			goto exit;
		}
		break;
	}

	case HD_AUDIOCAP_PARAM_OUT_AEC: {
		HD_AUDIOCAP_AEC *p_aec = (HD_AUDIOCAP_AEC *) p_param;
		if (p_aec->enabled != TRUE && p_aec->enabled != FALSE) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOCAP_AEC: enabled(%d) is out-of-range(0~1).\n", p_aec->enabled);
			goto exit;
		}
		if (p_aec->enabled == 1) {
			//TODO: check values if it is supported
			snprintf(p_ret_string, max_str_len, "Not support for AEC.\n");
			goto exit;
		}
		break;
	}

	case HD_AUDIOCAP_PARAM_OUT_ANR: {
		HD_AUDIOCAP_ANR *p_anr = (HD_AUDIOCAP_ANR *) p_param;
		if (p_anr->enabled != TRUE && p_anr->enabled != FALSE) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOCAP_ANR: enabled(%d) is out-of-range(0~1).\n", p_anr->enabled);
			goto exit;
		}
		if (p_anr->enabled == 1) {
			//TODO: check values if it is supported
			snprintf(p_ret_string, max_str_len, "Not support for ANR.\n");
			goto exit;
		}
		break;
	}
	case HD_AUDIOCAP_PARAM_VOLUME: {
		//TODO
		break;
	}
	default:
		break;

	}
	return 0;
exit:
	return -1;

}


static VOID fill_default_params(HD_AUDIOCAP_PARAM_ID id, VOID *p_param)
{
	switch (id) {
	case HD_AUDIOCAP_PARAM_DEVCOUNT:
		break;
	case HD_AUDIOCAP_PARAM_SYSCAPS:
		break;
	case HD_AUDIOCAP_PARAM_SYSINFO:
		break;

	case HD_AUDIOCAP_PARAM_DEV_CONFIG: {
		HD_AUDIOCAP_DEV_CONFIG *p_dev_cfg = (HD_AUDIOCAP_DEV_CONFIG *) p_param;
		HD_AUDIOCAP_POOL *p_data_pool = (HD_AUDIOCAP_POOL *) p_dev_cfg->data_pool;
		p_data_pool->ddr_id = 0;
		p_data_pool->frame_sample_size = 320 * 2;
		p_data_pool->counts = HD_AUDIOCAP_SET_COUNT(20, 0);
		p_data_pool->max_counts = HD_AUDIOCAP_SET_COUNT(20, 0);
		p_data_pool->min_counts = 0;
		p_data_pool->mode = 0;    // pool mode, 0: auto, 1:enable, 2:disable
		break;
	}

	case HD_AUDIOCAP_PARAM_DRV_CONFIG:
		break;

	case HD_AUDIOCAP_PARAM_IN: {
		HD_AUDIOCAP_IN *p_audcap_in = (HD_AUDIOCAP_IN *) p_param;
		p_audcap_in->sample_rate = HD_AUDIO_SR_8000;
		p_audcap_in->sample_bit = HD_AUDIO_BIT_WIDTH_16;
		p_audcap_in->mode = HD_AUDIO_SOUND_MODE_MONO;
		p_audcap_in->frame_sample = 1024;
		break;
	}

	case HD_AUDIOCAP_PARAM_OUT: {
		HD_AUDIOCAP_OUT *p_audcap_out = (HD_AUDIOCAP_OUT *) p_param;
		p_audcap_out->sample_rate = HD_AUDIO_SR_8000;
		break;
	}

	case HD_AUDIOCAP_PARAM_OUT_AEC: {
		HD_AUDIOCAP_AEC *p_aec = (HD_AUDIOCAP_AEC *) p_param;
		p_aec->enabled = FALSE;
		p_aec->leak_estimate_enabled = FALSE;
		p_aec->leak_estimate_value = 62;
		p_aec->noise_cancel_level = -22;
		p_aec->echo_cancel_level = -45;
		p_aec->filter_length = 1;
		p_aec->frame_size = 128;
		p_aec->notch_radius = 500;
		break;
	}

	case HD_AUDIOCAP_PARAM_OUT_ANR: {
		HD_AUDIOCAP_ANR *p_anr = (HD_AUDIOCAP_ANR *) p_param;
		p_anr->enabled = FALSE;
		p_anr->suppress_level = 1;
		p_anr->hpf_cut_off_freq = 1;
		p_anr->bias_sensitive = 1;
		break;
	}

	case HD_AUDIOCAP_PARAM_VOLUME: {
		//TODO
		break;
	}

	default:
		break;
	}
}


VOID audiocap_get_capability(HD_DAL self_id, HD_AUDIOCAP_SYSCAPS *p_aucap)
{
	INT  i;
	UINT dev_idx = AUDCAP_DEVID(self_id);
	vpd_au_rx_sys_info_t  *p_au_rx_info;

	p_au_rx_info = &platform_sys_Info.au_rx_info[dev_idx];
	p_aucap->dev_id = dev_idx;
	p_aucap->chip_id = p_au_rx_info->chipid;
	p_aucap->max_in_count = HD_AUDIOCAP_MAX_IN;
	p_aucap->max_out_count = HD_AUDIOCAP_MAX_OUT;
	p_aucap->dev_caps = HD_CAPS_DRVCONFIG
			    | HD_AUDIOCAP_DEVCAPS_LINEIN
			    | HD_AUDIOCAP_DEVCAPS_SSP_CONFIG;

	for (i = 0; i < HD_AUDIOCAP_MAX_IN; i++) {
		p_aucap->in_caps[i] = HD_AUDIO_CAPS_BITS_16 | HD_AUDIO_CAPS_CH_MONO;
		p_aucap->support_in_sr[i] =  HD_AUDIOCAP_SRCAPS_8000 | HD_AUDIOCAP_SRCAPS_16000 |
									HD_AUDIOCAP_SRCAPS_32000 | HD_AUDIOCAP_SRCAPS_48000;
	}
	for (i = 0; i < HD_AUDIOCAP_MAX_OUT; i++) {
		p_aucap->out_caps[i] = HD_AUDIO_CAPS_BITS_16 | HD_AUDIO_CAPS_CH_MONO;
		p_aucap->support_out_sr[i] = HD_AUDIOCAP_SRCAPS_8000 | HD_AUDIOCAP_SRCAPS_16000 |
									HD_AUDIOCAP_SRCAPS_32000 | HD_AUDIOCAP_SRCAPS_48000;
	}

	return;
}

UINT audiocap_get_devcount(VOID)
{
	INT i; 
	UINT dev_count = 0;
	vpd_au_rx_sys_info_t  *p_au_rx_info;

	for (i = 0; i < VPD_MAX_AU_RX_NUM; i++) {
		p_au_rx_info = &platform_sys_Info.au_rx_info[i];
		if (p_au_rx_info->ch < 0) {
			continue;
		}
		dev_count ++;
	}

	return dev_count;
}


HD_RESULT audiocap_clear_buf(INT dev_idx)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_stop_trigger_t stop_trigger_info;
	AUDCAP_INFO_PRIV *p_audcap_priv = &audiocap_info[dev_idx].priv;
	INT ret;

	memset(&stop_trigger_info, 0, sizeof(stop_trigger_info));
	stop_trigger_info.usr_proc_fd.fd = p_audcap_priv->fd;
	stop_trigger_info.mode = USR_PROC_STOP_FD_DOWNSTREAM;
	if ((ret = ioctl(vpd_fd, USR_PROC_STOP_TRIGGER, &stop_trigger_info)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		HD_AUDIOCAPTURE_ERR("<ioctl \"USR_PROC_STOP_TRIGGER\" fail, fd(%#x) error(%d)>\n",
				    stop_trigger_info.usr_proc_fd.fd, ret);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}

HD_RESULT audiocap_module_init(HD_AUDIOCAP_DRV_CONFIG *p_drv_config)
{
	INT i, ret;
	audiocap_init_t aucap;
	HD_RESULT hd_ret = HD_OK;

	memset(&aucap, 0, sizeof(aucap));
	for (i = 0; i < AUDIOCAP_MAX_DEVICE_NUM; i++) {
		aucap.audio_ssp_num[i] = p_drv_config->ssp_config.ssp_num[i];
		aucap.audio_in_enable[i] = p_drv_config->ssp_config.enable[i];
		aucap.audio_ssp_chan[i] = p_drv_config->ssp_config.ssp_chan[i];
		aucap.sample_size[i] = p_drv_config->ssp_config.sample_size[i];
		aucap.sample_rate[i] = p_drv_config->ssp_config.sample_rate[i];
		aucap.ssp_clock[i] = p_drv_config->ssp_config.ssp_clock[i];
		aucap.bit_clock[i] = p_drv_config->ssp_config.bit_clock[i];
		aucap.audio_ssp_master[i] = p_drv_config->ssp_config.ssp_master[i];
		aucap.live_sound_ch[i] = p_drv_config->ssp_config.live_sound_ch[i];
	}

	ret = pif_audiocap_init(&aucap);
	if (ret < 0) {
		hd_ret = HD_ERR_SYS;
		goto ext;
	}
	audioin_align = aucap.in_align;

	ret = pif_env_update();
	if (ret < 0) {
		hd_ret = HD_ERR_SYS;
		goto ext;
	}

ext:
	return hd_ret;
}


HD_RESULT audiocap_init_p(void)
{
	INT ioctl_ret;

	if (audioio_ioctl < 0) {
		audioio_ioctl = open("/dev/kflow_audioio", O_RDWR);
		if (audioio_ioctl == -1) {
			HD_VIDEOENC_ERR("open /dev/kflow_audioio fail\n");
			return HD_ERR_SYS;
		}
	}

	if (is_audioio_hw_spec_info_ready == FALSE) {
		memset(&audioio_hw_spec_info, 0, sizeof(audioio_hw_spec_info));
		ioctl_ret = ioctl(audioio_ioctl, KFLOW_AUDIOIO_IOC_GET_HW_SPEC_INFO, &audioio_hw_spec_info);
		if (ioctl_ret < 0) {
			HD_VIDEODEC_ERR("ioctl \"KFLOW_AUDIOIO_IOC_GET_HW_SPEC_INFO\" return %d\n", ioctl_ret);
			return HD_ERR_SYS;
		}
		is_audioio_hw_spec_info_ready = TRUE;
	}

	pthread_mutex_lock(&audiocap_mutex_lock);
	memset(&audiocap_info, 0, sizeof(audiocap_info));
	memset(&audcap_dev_param, 0, sizeof(audcap_dev_param));
	pthread_mutex_unlock(&audiocap_mutex_lock);

	return HD_OK;
}

HD_RESULT audiocap_uninit_p(void)
{
	if (audioio_ioctl < 0) {
		close(audioio_ioctl);
		audioio_ioctl = -1;
	}
	return HD_OK;
}


HD_RESULT audiocap_init_dev_p(UINT dev)
{
	HD_RESULT hd_ret = HD_OK;
	AUDCAP_INFO_PRIV *p_audcap_priv;
	//UINT queue_handle;
	uintptr_t queue_handle;
	INT fd, port, ioctl_ret;
	UINT i;
	vpd_au_rx_sys_info_t  *p_au_rx_info;
	KFLOW_AUDIOIO_FD_INFO fd_info = {0};

	pthread_mutex_lock(&audiocap_mutex_lock);

	if (audiocap_info[dev].dev_open_count > 0) {
		goto exit;
	}

	p_audcap_priv = &audiocap_info[dev].priv;
	memset(&audiocap_info[dev], 0, sizeof(AUDCAP_INFO));
	memset(&audcap_dev_param[dev], 0, sizeof(AUDCAP_INTERNAL_PARAM));

	p_au_rx_info = &platform_sys_Info.au_rx_info[dev];

	//allocate fd
	fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_AU_GRAB, p_au_rx_info->chipid, p_au_rx_info->dev_no, 0);
	if (fd == 0) {
		HD_AUDIOCAPTURE_ERR("Get fd(%d) failed.\n", fd);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_audcap_priv->fd = fd;

	//create callback queue
	queue_handle = pif_create_queue_for_hdal();
	if (queue_handle == 0) {
		HD_AUDIOCAPTURE_ERR("Create queue(%lu) failed.\n", queue_handle);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_audcap_priv->queue_handle = queue_handle;

	for (i = 0; i < p_au_rx_info->channel_num; i++) {
		p_audcap_priv->port[i].vch = i;
	}

	audcap_dev_param[dev].p_priv_info = p_audcap_priv;

	fill_default_params(HD_AUDIOCAP_PARAM_IN, &audiocap_info[dev].param_in);
	fill_default_params(HD_AUDIOCAP_PARAM_DEV_CONFIG, &audiocap_info[dev].dev_config);
	audcap_dev_param[dev].p_dev_config = &audiocap_info[dev].dev_config;
	for (port = 0; port < MAX_AUDCAP_OUT_PORT; port++) {
		fill_default_params(HD_AUDIOCAP_PARAM_OUT, &audiocap_info[dev].port[port].param_out);
		fill_default_params(HD_AUDIOCAP_PARAM_OUT_AEC, &audiocap_info[dev].port[port].param_aec);
		fill_default_params(HD_AUDIOCAP_PARAM_OUT_ANR, &audiocap_info[dev].port[port].param_anr);
	}

	fd_info.entity_fd = fd;
	ioctl_ret = ioctl(audioio_ioctl, KFLOW_AUDIOIO_IOC_FD_OPEN, &fd_info);
	if (ioctl_ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"KFLOW_AUDIOIO_IOC_FD_OPEN\" return %d\n", ioctl_ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}

exit:
	if (hd_ret == HD_OK) {
		audiocap_info[dev].dev_open_count ++;
	}
	pthread_mutex_unlock(&audiocap_mutex_lock);
	return hd_ret;
}

HD_RESULT audiocap_uninit_dev_p(UINT dev)
{
	HD_RESULT hd_ret = HD_OK;
	AUDCAP_INFO_PRIV *p_audcap_priv;
	INT ret, ioctl_ret;
	KFLOW_AUDIOIO_FD_INFO fd_info = {0};

	p_audcap_priv = &audiocap_info[dev].priv;

	pthread_mutex_lock(&audiocap_mutex_lock);
	audiocap_info[dev].dev_open_count --;
	if (audiocap_info[dev].dev_open_count > 0) {
		goto exit;
	}

	fd_info.entity_fd = p_audcap_priv->fd;
	ioctl_ret = ioctl(audioio_ioctl, KFLOW_AUDIOIO_IOC_FD_CLOSE, &fd_info);
	if (ioctl_ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"KFLOW_AUDIOIO_IOC_FD_CLOSE\" return %d\n", ioctl_ret);
		hd_ret = HD_ERR_SYS;
	}

	if (p_audcap_priv->queue_handle) {
		ret = pif_destroy_queue_for_hdal(p_audcap_priv->queue_handle);
		if (ret < 0) {
			HD_AUDIOCAPTURE_ERR("Destroy queue failed. ret(%d)\n", ret);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
	}

	memset(&audiocap_info[dev], 0, sizeof(AUDCAP_INFO));
	memset(&audcap_dev_param[dev], 0, sizeof(AUDCAP_INTERNAL_PARAM));

exit:
	pthread_mutex_unlock(&audiocap_mutex_lock);

	return hd_ret;
}


AUDCAP_INTERNAL_PARAM *audiocap_get_param_p(HD_DAL self_id)
{
	int dev_idx = AUDCAP_DEVID(self_id);
	if (dev_idx > MAX_AUDCAP_DEV || dev_idx < 0) {
		HD_AUDIOCAPTURE_ERR("Error self_id(%d)\n", self_id);
		goto err_ext;
	}
	return &audcap_dev_param[dev_idx];
err_ext:
	return NULL;
}


HD_RESULT audiocap_get_basic_param(HD_PATH_ID path_id, HD_AUDIOCAP_IN *p_param_in)
{
	HD_RESULT hd_ret = HD_OK;
	INT dev_idx, ret, sample_rate, bit_depth, tracks;
	dev_idx = HD_GET_DEV(path_id) - HD_DAL_AUDIOCAP_BASE;
	vpd_au_rx_sys_info_t  *p_au_rx_info;

	p_au_rx_info = &platform_sys_Info.au_rx_info[dev_idx];

	ret = pif_au_get_basic_param(TRUE, p_au_rx_info->chipid, p_au_rx_info->dev_no, &sample_rate, &bit_depth, &tracks, NULL);
	if (ret == -1) {
		//set param failed
		HD_VIDEOENC_ERR("Get audiocap param failed, ret(%d)\n", ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	} else if (ret == -2) {
		//device doesn't open
		HD_VIDEOENC_ERR("Device doesn't open, ret(%d)\n", ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	} else if (ret == -3) {
		//device is busy
		HD_VIDEOENC_ERR("Device is busy, ret(%d)\n", ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}
	p_param_in->sample_rate = sample_rate;
	p_param_in->sample_bit = bit_depth;
	p_param_in->mode = (tracks == 1) ? HD_AUDIO_SOUND_MODE_MONO : HD_AUDIO_SOUND_MODE_STEREO;

exit:
	return hd_ret;
}


HD_RESULT audiocap_set_basic_param(HD_PATH_ID path_id, HD_AUDIOCAP_IN *p_param_in)
{
	HD_RESULT hd_ret = HD_OK;
	INT dev_idx, ret;
	dev_idx = HD_GET_DEV(path_id) - HD_DAL_AUDIOCAP_BASE;
	vpd_au_rx_sys_info_t  *p_au_rx_info;

	p_au_rx_info = &platform_sys_Info.au_rx_info[dev_idx];

	ret = pif_au_set_basic_param(TRUE, p_au_rx_info->chipid, p_au_rx_info->dev_no,
								p_param_in->sample_rate, p_param_in->sample_bit,
				     p_param_in->mode == HD_AUDIO_SOUND_MODE_MONO ? 1 : 2, p_param_in->sample_rate);
	if (ret == -1) {
		//set param failed
		HD_VIDEOENC_ERR("Set audiocap param failed, ret(%d)\n", ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	} else if (ret == -2) {
		//device doesn't open
		HD_VIDEOENC_ERR("Device doesn't open, ret(%d)\n", ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	} else if (ret == -3) {
		//device is busy
		HD_VIDEOENC_ERR("Device is busy, ret(%d)\n", ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}
exit:
	return hd_ret;
}

