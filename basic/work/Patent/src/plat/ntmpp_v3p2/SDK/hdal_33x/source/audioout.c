/**
	@brief Source file of audioout.\n
	This file contains the functions which is related to audio output in the chip.

	@file audioout.c

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
#include "audioout.h"
#include "pif.h"
#include "pif_ioctl.h"
#include "kflow_audioio.h"

#define AUDIOOUT_MEM_POOL_NAME    "au_dec_ddr0"

extern int pif_alloc_user_fd_for_hdal(vpd_em_type_t type, int chip, int engine, int minor);
//extern unsigned int pif_create_queue_for_hdal(void);
extern uintptr_t pif_create_queue_for_hdal(void);
//extern int pif_destroy_queue_for_hdal(unsigned int queue_handle);
extern int pif_destroy_queue_for_hdal(uintptr_t queue_handle);
extern void *pif_alloc_video_buffer_for_hdal(int size, int ddr_id, char pool_name[], int alloc_tag);
extern int pif_free_video_buffer_for_hdal(void *pa, int ddr_id, int alloc_tag);

extern int get_sample_rate_from_HD_AUDIO_SR_p(HD_AUDIO_SR sample_rate);
extern int get_sample_size_from_HD_AUDIO_BIT_WIDTH_p(HD_AUDIO_BIT_WIDTH sample_bit);
extern int get_channel_type_from_HD_AUDIO_SOUND_MODE_p(HD_AUDIO_SOUND_MODE mode);
extern int get_audio_encode_type_from_HD_AUDIO_CODEC_p(HD_AUDIO_CODEC codec_type);

extern UINT32 audioout_max_device_nr;
extern UINT32 audioout_max_device_in;
extern UINT32 audioout_max_device_out;
extern int vpd_fd;
extern AUDOUT_INFO audioout_info[MAX_AUDOUT_DEV];
extern vpd_sys_info_t platform_sys_Info;
AUDOUT_INTERNAL_PARAM audout_dev_param[MAX_AUDOUT_DEV] = {0};
static INT audioio_ioctl = -1;
KFLOW_AUDIOIO_HW_SPEC_INFO audioio_hw_spec_info;
INT is_audioio_hw_spec_info_ready = FALSE;


HD_RESULT audioout_check_path_id_p(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);

	if ((INT) audioout_max_device_nr == -1) {
		HD_AUDIOOUT_ERR("Error, no audioout device.\n");
		return HD_ERR_NG;
	}
	if (ctrl_id == HD_CTRL) {
		goto ext;
	}
	if (AUDOUT_DEVID(self_id) > audioout_max_device_nr || ((int)AUDOUT_DEVID(self_id)) < 0) {
		HD_AUDIOOUT_ERR("Error self_id(%#x)\n", self_id);
		return HD_ERR_NG;
	}
	if (AUDOUT_INID(in_id) > audioout_max_device_in) {
		HD_AUDIOOUT_ERR("Check input port error, port(%#x) max(%#x)\n", in_id, audioout_max_device_in);
		return HD_ERR_NG;
	}
	if (AUDOUT_OUTID(out_id) > audioout_max_device_out) {
		HD_AUDIOOUT_ERR("Check output port error, port(%#x) max(%#x)\n", out_id, audioout_max_device_out);
		return HD_ERR_NG;
	}
ext:
	return HD_OK;
}



INT check_audioout_params_range(HD_AUDIOOUT_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len)
{
	switch (id) {
	//TODO
	case HD_AUDIOOUT_PARAM_DEVCOUNT:
		break;
	case HD_AUDIOOUT_PARAM_SYSCAPS:
		break;
	case HD_AUDIOOUT_PARAM_SYSINFO:
		break;
	case HD_AUDIOOUT_PARAM_DEV_CONFIG:
		break;
	case HD_AUDIOOUT_PARAM_DRV_CONFIG:
		break;

	case HD_AUDIOOUT_PARAM_OUT: {
		HD_AUDIOOUT_OUT *p_audout_out = (HD_AUDIOOUT_OUT *) p_param;
		if (p_audout_out->sample_rate != HD_AUDIO_SR_8000 &&
		    p_audout_out->sample_rate != HD_AUDIO_SR_11025 &&
		    p_audout_out->sample_rate != HD_AUDIO_SR_12000 &&
		    p_audout_out->sample_rate != HD_AUDIO_SR_16000 &&
		    p_audout_out->sample_rate != HD_AUDIO_SR_22050 &&
		    p_audout_out->sample_rate != HD_AUDIO_SR_24000 &&
		    p_audout_out->sample_rate != HD_AUDIO_SR_32000 &&
		    p_audout_out->sample_rate != HD_AUDIO_SR_44100 &&
		    p_audout_out->sample_rate != HD_AUDIO_SR_48000) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOOUT_OUT: sample_rate(%d) is an invalid value.\n",
				 p_audout_out->sample_rate);
			goto exit;
		}
		if (p_audout_out->sample_bit != HD_AUDIO_BIT_WIDTH_16) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOOUT_OUT: sample_bit(%d) is an invalid value.\n",
				 p_audout_out->sample_bit);
			goto exit;
		}
		if (p_audout_out->mode != HD_AUDIO_SOUND_MODE_MONO &&
		    p_audout_out->mode != HD_AUDIO_SOUND_MODE_STEREO) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOOUT_OUT: mode(%d) is an invalid value.\n",
				 p_audout_out->mode);
			goto exit;
		}
		break;
	}


	case HD_AUDIOOUT_PARAM_IN: {
		HD_AUDIOOUT_IN *p_audout_in = (HD_AUDIOOUT_IN *) p_param;
		if (p_audout_in->sample_rate != HD_AUDIO_SR_8000 &&
		    p_audout_in->sample_rate != HD_AUDIO_SR_11025 &&
		    p_audout_in->sample_rate != HD_AUDIO_SR_12000 &&
		    p_audout_in->sample_rate != HD_AUDIO_SR_16000 &&
		    p_audout_in->sample_rate != HD_AUDIO_SR_22050 &&
		    p_audout_in->sample_rate != HD_AUDIO_SR_24000 &&
		    p_audout_in->sample_rate != HD_AUDIO_SR_32000 &&
		    p_audout_in->sample_rate != HD_AUDIO_SR_44100 &&
		    p_audout_in->sample_rate != HD_AUDIO_SR_48000) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOOUT_IN: sample_rate(%d) is an invalid value.\n",
				 p_audout_in->sample_rate);
			goto exit;
		}
		break;
	}
	case HD_AUDIOOUT_PARAM_VOLUME: {
		HD_AUDIOOUT_VOLUME *p_audout_volume = (HD_AUDIOOUT_VOLUME *) p_param;
		if (p_audout_volume->volume > 100) {
			snprintf(p_ret_string, max_str_len,
				 "HD_AUDIOOUT_VOLUME: volume(%u) is an invalid value.\n",
				 p_audout_volume->volume);
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


static VOID fill_default_params(HD_AUDIOOUT_PARAM_ID id, VOID *p_param)
{
	switch (id) {
	//TODO
	case HD_AUDIOOUT_PARAM_DEVCOUNT:
		break;
	case HD_AUDIOOUT_PARAM_SYSCAPS:
		break;
	case HD_AUDIOOUT_PARAM_SYSINFO:
		break;
	case HD_AUDIOOUT_PARAM_DEV_CONFIG:
		break;
	case HD_AUDIOOUT_PARAM_DRV_CONFIG:
		break;

	case HD_AUDIOOUT_PARAM_OUT: {
		HD_AUDIOOUT_OUT *p_audout_out = (HD_AUDIOOUT_OUT *) p_param;
		p_audout_out->sample_rate = HD_AUDIO_SR_8000;
		p_audout_out->sample_bit = HD_AUDIO_BIT_WIDTH_16;
		p_audout_out->mode = HD_AUDIO_SOUND_MODE_MONO;
		break;
	}

	case HD_AUDIOOUT_PARAM_IN: {
		HD_AUDIOOUT_IN *p_audout_in = (HD_AUDIOOUT_IN *) p_param;
		p_audout_in->sample_rate = HD_AUDIO_SR_8000;
		break;
	}

	case HD_AUDIOOUT_PARAM_VOLUME: {
		HD_AUDIOOUT_VOLUME *p_audout_volume = (HD_AUDIOOUT_VOLUME *) p_param;
		p_audout_volume->volume = 50;
		break;
	}

	default:
		break;
	}
}


VOID audioout_get_capability(HD_DAL self_id, HD_AUDIOOUT_SYSCAPS *p_auout)
{
	INT  i;
	UINT dev_idx = AUDOUT_DEVID(self_id);
	vpd_au_tx_sys_info_t  *p_au_tx_info;

	p_au_tx_info = &platform_sys_Info.au_tx_info[dev_idx];

	p_auout->dev_id = dev_idx;
	p_auout->chip_id = p_au_tx_info->chipid;
	p_auout->max_in_count = HD_AUDIOOUT_MAX_IN;
	p_auout->max_out_count = HD_AUDIOOUT_MAX_OUT;
	p_auout->dev_caps = HD_CAPS_DRVCONFIG
						| HD_AUDIOOUT_DEVCAPS_LINEOUT
						| HD_AUDIOOUT_DEVCAPS_SSP_CONFIG
						| HD_AUDIOOUT_DEVCAPS_CLEAR_BUF
						| HD_AUDIOOUT_DEVCAPS_VOLUME;

	for (i = 0; i < HD_AUDIOOUT_MAX_IN; i++) {
		p_auout->in_caps[i] = HD_AUDIO_CAPS_BITS_16 | HD_AUDIO_CAPS_CH_MONO;
		p_auout->support_in_sr[i] =  HD_AUDIOOUT_SRCAPS_8000 | HD_AUDIOOUT_SRCAPS_16000 |
									HD_AUDIOOUT_SRCAPS_32000 | HD_AUDIOOUT_SRCAPS_48000;
	}
	for (i = 0; i < HD_AUDIOOUT_MAX_OUT; i++) {
		p_auout->out_caps[i] = HD_AUDIO_CAPS_BITS_16 | HD_AUDIO_CAPS_CH_MONO;
		p_auout->support_out_sr[i] = HD_AUDIOOUT_SRCAPS_8000 | HD_AUDIOOUT_SRCAPS_16000 |
									HD_AUDIOOUT_SRCAPS_32000 | HD_AUDIOOUT_SRCAPS_48000;
	}
}


UINT audioout_get_devcount(VOID)
{
	INT i;
	UINT dev_count = 0;
	vpd_au_tx_sys_info_t  *p_au_tx_info;

	for (i = 0; i < VPD_MAX_AU_TX_NUM; i++) {
		p_au_tx_info = &platform_sys_Info.au_tx_info[i];
		if (p_au_tx_info->ch < 0) {
			continue;
		}
		dev_count ++;
	}

	return dev_count;
}


HD_RESULT audioout_clear_buf(INT dev_idx)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_stop_trigger_t stop_trigger_info;
	AUDOUT_INFO_PRIV *p_audout_priv = &audioout_info[dev_idx].priv;
	INT ret;

	memset(&stop_trigger_info, 0, sizeof(stop_trigger_info));
	stop_trigger_info.usr_proc_fd.fd = p_audout_priv->fd;
	stop_trigger_info.mode = USR_PROC_STOP_FD_BUF_CLEAR;
	if ((ret = ioctl(vpd_fd, USR_PROC_STOP_TRIGGER, &stop_trigger_info)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		HD_AUDIOOUT_ERR("<ioctl \"USR_PROC_STOP_TRIGGER\" fail, fd(%#x) error(%d)>\n",
				stop_trigger_info.usr_proc_fd.fd, ret);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}


HD_RESULT audioout_module_init(HD_AUDIOOUT_DRV_CONFIG *p_drv_config)
{
	INT i, ret;
	audioout_init_t auout;
	HD_RESULT hd_ret = HD_OK;

	memset(&auout, 0, sizeof(auout));
	for (i = 0; i < AUDIOOUT_MAX_DEVICE_NUM; i++) {
		auout.audio_out_enable[i] = p_drv_config->ssp_config.enable[i];
		auout.resample_ratio[i] = p_drv_config->ssp_config.resample_ratio[i];
		auout.playback_chmap[i] = p_drv_config->ssp_config.playback_chmap[i];
	}

	ret = pif_audioout_init(&auout);
	if (ret < 0) {
		hd_ret = HD_ERR_SYS;
		goto ext;
	}

	ret = pif_env_update();
	if (ret < 0) {
		hd_ret = HD_ERR_SYS;
		goto ext;
	}

ext:
	return hd_ret;
}


HD_RESULT audioout_init_p(void)
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

	return HD_OK;
}

HD_RESULT audioout_uninit_p(void)
{
	if (audioio_ioctl < 0) {
		close(audioio_ioctl);
		audioio_ioctl = -1;
	}
	return HD_OK;
}


HD_RESULT audioout_init_dev_p(UINT dev)
{
	HD_RESULT hd_ret = HD_OK;
	AUDOUT_INFO_PRIV *p_audout_priv;
	uintptr_t queue_handle;
	INT fd, port, ioctl_ret;
	UINT i;
	vpd_au_tx_sys_info_t  *p_au_tx_info;
	KFLOW_AUDIOIO_FD_INFO fd_info = {0};

	//memset(&audioout_info[dev], 0, sizeof(AUDOUT_INFO));
	//memset(&audout_dev_param[dev], 0, sizeof(AUDOUT_INTERNAL_PARAM));

	p_audout_priv = &audioout_info[dev].priv;
	p_au_tx_info = &platform_sys_Info.au_tx_info[dev];

	//allocate fd
	fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_AU_RENDER, p_au_tx_info->chipid, p_au_tx_info->dev_no, 0);
	if (fd == 0) {
		HD_AUDIOOUT_ERR("Get VPD_TYPE_AU_RENDER fd(%d) failed. dev(%d)\n", fd, dev);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_audout_priv->fd = fd;

	//create callback queue
	queue_handle = pif_create_queue_for_hdal();
	if (queue_handle == 0) {
		HD_AUDIOOUT_ERR("Create queue(%lu) failed.\n", queue_handle);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_audout_priv->queue_handle = queue_handle;

	for (i = 0; i < p_au_tx_info->channel_num; i++) {
		p_audout_priv->port[i].vch = i;
	}
	audout_dev_param[dev].p_priv_info = p_audout_priv;

	for (port = 0; port < MAX_AUDOUT_PORT; port++) {
		fill_default_params(HD_AUDIOOUT_PARAM_IN, &audioout_info[dev].param_in[port]);
	}
	fill_default_params(HD_AUDIOOUT_PARAM_OUT, &audioout_info[dev].param_out);
	fill_default_params(HD_AUDIOOUT_PARAM_VOLUME, &audioout_info[dev].param_volume);

	fd_info.entity_fd = fd;
	ioctl_ret = ioctl(audioio_ioctl, KFLOW_AUDIOIO_IOC_FD_OPEN, &fd_info);
	if (ioctl_ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"KFLOW_AUDIOIO_IOC_FD_OPEN\" return %d\n", ioctl_ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}

exit:
	return hd_ret;
}

HD_RESULT audioout_uninit_dev_p(UINT dev)
{
	HD_RESULT hd_ret = HD_OK;
	AUDOUT_INFO_PRIV *p_audout_priv;
	INT ret, ioctl_ret;
	KFLOW_AUDIOIO_FD_INFO fd_info = {0};

	p_audout_priv = &audioout_info[dev].priv;

	fd_info.entity_fd = p_audout_priv->fd;
	ioctl_ret = ioctl(audioio_ioctl, KFLOW_AUDIOIO_IOC_FD_CLOSE, &fd_info);
	if (ioctl_ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"KFLOW_AUDIOIO_IOC_FD_CLOSE\" return %d\n", ioctl_ret);
		hd_ret = HD_ERR_SYS;
	}

	if (p_audout_priv->queue_handle) {
		ret = pif_destroy_queue_for_hdal(p_audout_priv->queue_handle);
		if (ret < 0) {
			HD_AUDIOOUT_ERR("Destroy queue failed. ret(%d)\n", ret);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
	}
	memset(&audioout_info[dev], 0, sizeof(AUDOUT_INFO));
	memset(&audout_dev_param[dev], 0, sizeof(AUDOUT_INTERNAL_PARAM));

exit:
	return hd_ret;
}


HD_RESULT audioout_new_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT	buf_size, ddr_id;
	CHAR pool_name[32];
	VOID *addr_pa;

	buf_size = p_audio_frame->size;
	ddr_id = p_audio_frame->ddr_id;
	sprintf(pool_name, AUDIOOUT_MEM_POOL_NAME);
	addr_pa = pif_alloc_video_buffer_for_hdal(buf_size, ddr_id, pool_name, USR_LIB);
	if (addr_pa == NULL) {
		HD_AUDIOOUT_ERR("%s, alloca out_buf failed.\n", __func__);
		hd_ret = HD_ERR_NOMEM;
		goto exit;
	}
	p_audio_frame->phy_addr[0] = (UINTPTR) addr_pa;

exit:
	return hd_ret;
}

HD_RESULT audioout_push_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_trigger_au_render_t usr_trigger_au_render;
	INT dev_idx = AUDOUT_DEVID(self_id);
	AUDOUT_INFO *p_audout_info = &audioout_info[dev_idx];
	AUDOUT_INFO_PRIV *p_audout_priv = &p_audout_info->priv;
	usr_au_render_param_t *p_au_render_param;
	INT ret;

	memset(&usr_trigger_au_render, 0, sizeof(usr_trigger_au_render));
	usr_trigger_au_render.fd = p_audout_priv->fd;
	usr_trigger_au_render.in_frame_buffer.ddr_id = p_audio_frame->ddr_id;
	usr_trigger_au_render.in_frame_buffer.addr_pa = p_audio_frame->phy_addr[0];
	if (pif_address_ddr_sanity_check(usr_trigger_au_render.in_frame_buffer.addr_pa, usr_trigger_au_render.in_frame_buffer.ddr_id) < 0) {
		HD_AUDIOOUT_ERR("%s:Check in_pa(%#lx) ddrid(%d) error\n", __func__, usr_trigger_au_render.in_frame_buffer.addr_pa, 
			usr_trigger_au_render.in_frame_buffer.ddr_id);
		return HD_ERR_PARAM;
	}
	usr_trigger_au_render.in_frame_buffer.size = p_audio_frame->size;
	sprintf(usr_trigger_au_render.in_frame_buffer.pool_name, AUDIOOUT_MEM_POOL_NAME);

	p_au_render_param = &usr_trigger_au_render.au_render_param;
	p_au_render_param->channel_bmp = (1 << dev_idx);
	p_au_render_param->sample_rate = get_sample_rate_from_HD_AUDIO_SR_p(p_audio_frame->sample_rate);
	p_au_render_param->bit_width = get_sample_size_from_HD_AUDIO_BIT_WIDTH_p(p_audio_frame->bit_width);
	p_au_render_param->mode = get_channel_type_from_HD_AUDIO_SOUND_MODE_p(p_audio_frame->sound_mode);
	p_au_render_param->codec_type = GM_PCM;

	usr_trigger_au_render.queue_handle = p_audout_priv->queue_handle;
	usr_trigger_au_render.p_user_data = (void *)234; //pass private data if need
	usr_trigger_au_render.is_cb_put_queue = FALSE;

	if ((ret = ioctl(vpd_fd, USR_PROC_TRIGGER_AU_RENDER, &usr_trigger_au_render)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -2) {
			HD_AUDIOOUT_ERR("in_frame_buffer(phy_addr[0]=%p) is still in use."
					"Please 'pull' this buffer before push it.\n",
					(VOID *)p_audio_frame->phy_addr[0]);
		}
		HD_VIDEOENC_ERR("<ioctl \"USR_PROC_TRIGGER_AU_RENDER\" fail, error(%d)>\n", ret);
	}

	return hd_ret;
}

HD_RESULT audioout_release_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_AUDIO_FRAME *p_audio_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT ddr_id, ret;
	VOID *addr_pa;

	addr_pa = (VOID *) p_audio_frame->phy_addr[0];
	ddr_id = p_audio_frame->ddr_id;
	ret = pif_free_video_buffer_for_hdal(addr_pa, ddr_id, USR_LIB);
	if (ret < 0) {
		HD_AUDIOOUT_ERR("%s, free buffer(pa:%p) failed.\n", __func__, addr_pa);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}


AUDOUT_INTERNAL_PARAM *audioout_get_param_p(HD_DAL self_id)
{
	int dev_idx = AUDOUT_DEVID(self_id);
	if (dev_idx > MAX_AUDOUT_DEV || dev_idx < 0) {
		HD_AUDIOOUT_ERR("Error self_id(%d)\n", self_id);
		goto err_ext;
	}
	return &audout_dev_param[dev_idx];
err_ext:
	return NULL;
}


HD_RESULT audioout_get_basic_param(HD_PATH_ID path_id, HD_AUDIOOUT_IN *p_param_in, HD_AUDIOOUT_OUT *p_param_out)
{
	HD_RESULT hd_ret = HD_OK;
	INT dev_idx, ret, sample_rate, bit_depth, tracks, data_sample_rate;
	dev_idx = HD_GET_DEV(path_id) - HD_DAL_AUDIOOUT_BASE;
	vpd_au_tx_sys_info_t  *p_au_tx_info;

	p_au_tx_info = &platform_sys_Info.au_tx_info[dev_idx];

	ret = pif_au_get_basic_param(FALSE, p_au_tx_info->chipid, p_au_tx_info->dev_no,
								&sample_rate, &bit_depth, &tracks, &data_sample_rate);
	if (ret == -1) {
		//set param failed
		HD_VIDEOENC_ERR("Get audioout param failed, ret(%d)\n", ret);
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
	if (p_param_in) {
		p_param_in->sample_rate = data_sample_rate;
	}
	if (p_param_out) {
		p_param_out->sample_rate = sample_rate;
		p_param_out->sample_bit = bit_depth;
		p_param_out->mode = (tracks == 1) ? HD_AUDIO_SOUND_MODE_MONO : HD_AUDIO_SOUND_MODE_STEREO;
	}

exit:
	return hd_ret;
}


HD_RESULT audioout_set_basic_param(HD_PATH_ID path_id, HD_AUDIOOUT_IN *p_param_in, HD_AUDIOOUT_OUT *p_param_out)
{
	HD_RESULT hd_ret = HD_OK;
	INT dev_idx, ret, sample_rate = 0, bit_depth = 0, tracks = 0, data_sample_rate = 0;
	dev_idx = HD_GET_DEV(path_id) - HD_DAL_AUDIOOUT_BASE;
	vpd_au_tx_sys_info_t  *p_au_tx_info;

	if (p_param_in) {
		data_sample_rate = p_param_in->sample_rate;
	}
	if (p_param_out) {
		sample_rate = p_param_out->sample_rate;
		bit_depth = p_param_out->sample_bit;
		tracks = p_param_out->mode == HD_AUDIO_SOUND_MODE_MONO ? 1 : 2;
	}
	p_au_tx_info = &platform_sys_Info.au_tx_info[dev_idx];

	ret = pif_au_set_basic_param(FALSE, p_au_tx_info->chipid, p_au_tx_info->dev_no,
								sample_rate, bit_depth, tracks, data_sample_rate);
	if (ret == -1) {
		//set param failed
		HD_VIDEOENC_ERR("Set audioout param failed, ret(%d)\n", ret);
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

HD_RESULT audioout_set_param(HD_PATH_ID path_id, HD_AUDIOOUT_VOLUME *p_volume)
{
	HD_RESULT hd_ret = HD_OK;
	INT dev_idx, ret;
	dev_idx = HD_GET_DEV(path_id) - HD_DAL_AUDIOOUT_BASE;
	vpd_au_tx_sys_info_t  *p_au_tx_info;

	p_au_tx_info = &platform_sys_Info.au_tx_info[dev_idx];

	ret = pif_au_set_tx_volume(p_au_tx_info->chipid, p_au_tx_info->dev_no, p_volume->volume);
	if (ret == -1) {
		HD_VIDEOENC_ERR("Set audioout volume failed, ret(%d)\n", ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}

exit:
	return hd_ret;
}

