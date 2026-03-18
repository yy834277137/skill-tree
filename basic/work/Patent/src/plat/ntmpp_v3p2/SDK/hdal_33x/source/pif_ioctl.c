/*
 *   @file   pif_ioctl.c
 *
 *   @brief  platform interface for driver ioctl.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "cif_list.h"
#include "cif_common.h"
#include "vpd_ioctl.h"
#include "ms_ioctl.h"
#include "gs_ioctl.h"
#include "pif.h"
#include "pif_ioctl.h"
#include "vg_common.h"
#include "kflow_audioio/kflow_audioio.h"

#define MAX_OSG_LOAD_NUM 32

extern int vcap_ioctl;
extern int osg_ioctl;
extern int vpe_ioctl;
extern int ivs_ioctl;
extern int au_ioctl;
extern int frammap_ioctl;
extern int ms_ioctl;
extern int gs_ioctl;

int is_au_init = 0;

extern vpd_sys_info_t platform_sys_Info;
extern pif_group_t *pif_group[GM_MAX_GROUP_NUM];
extern pthread_mutex_t vpd_mutex;
extern unsigned int gmlib_dbg_level;
extern unsigned int gmlib_dbg_mode;
extern char *pif_msg;

extern void gmlib_setting_log(void);
extern int pif_check_bind_in(void *in);

void pif_vcap_init(void)
{
	return;
}

void pif_vcap_release(void)
{
	return;
}

void pif_osg_init(void)
{
	return;
}

void pif_osg_release(void)
{
	return;
}


void pif_vpe_init(void)
{
	return;
}

void pif_vpe_release(void)
{
	return;
}

void pif_au_init(void)
{
	return;
}

void pif_au_release(void)
{
	is_au_init = 0;
	return;
}

void pif_ms_ioctl_init(void)
{
	ms_ioctl = open("/dev/ms", O_RDWR);
	return;
}


void pif_ms_ioctl_release(void)
{
	close(ms_ioctl);
	ms_ioctl = 0;
	return;
}

int pif_audiocap_init(audiocap_init_t *aucap)
{
	return 0;
}

int pif_audioout_init(audioout_init_t *auout)
{
	return 0;
}


int pif_au_livesound(unsigned int acap_dev_id, unsigned int acap_ch, unsigned int aout_dev_id, int on_off)
{
	int ret = 0;
	KFLOW_AUDIOIO_LIVESOUND livesound_info;
	vpd_au_rx_sys_info_t  *p_au_rx_info;
	vpd_au_tx_sys_info_t  *p_au_tx_info;

	if (acap_dev_id >= VPD_MAX_AU_RX_NUM) {
		GMLIB_ERROR_P("Wrong acap_dev_id(%u) max(%d)\n\n", acap_dev_id, VPD_MAX_AU_RX_NUM);
		ret = ERR_FAILED;
		goto exit;
	}
	if (aout_dev_id >= VPD_MAX_AU_TX_NUM) {
		GMLIB_ERROR_P("Wrong aout_dev_id(%u) max(%d)\n\n", aout_dev_id, VPD_MAX_AU_TX_NUM);
		ret = ERR_FAILED;
		goto exit;
	}
	p_au_rx_info = &platform_sys_Info.au_rx_info[acap_dev_id];
	p_au_tx_info = &platform_sys_Info.au_tx_info[aout_dev_id];
	livesound_info.in_chip  = p_au_rx_info->chipid;
	livesound_info.in_dev_no  = p_au_rx_info->dev_no;
	livesound_info.in_port_no = acap_ch;
	livesound_info.out_chip = p_au_tx_info->chipid;
	livesound_info.out_dev_no = p_au_tx_info->dev_no;
	livesound_info.on_off     = on_off;

	GMLIB_L1_P("%s, acap_dev_id(%u) acap_ch(%u) aout_dev_id(%u), on_off(%d)\n",
		   __func__, acap_dev_id, acap_ch, aout_dev_id, on_off);

	if ((ret = ioctl(au_ioctl, KFLOW_AUDIOIO_IOC_LIVESOUND, &livesound_info)) < 0) {
		GMLIB_ERROR_P("ioctl \"KFLOW_AUDIOIO_IOC_LIVESOUND\" fail(%d)\n", ret);
		ret = ERR_FAILED;
	}

exit:
	return ret;
}

int pif_au_get_basic_param(int is_rx, int chip, int dev_no, int *p_sample_rate, int *p_bit_depth, int *p_tracks,
							int *p_data_sample_rate)
{
	int ret = 0;
	KFLOW_AUDIOIO_BASIC_PARAM basic_param;
	basic_param.rx_tx = is_rx ? KFLOW_AUDIOIO_RX_PARAM : KFLOW_AUDIOIO_TX_PARAM;
	basic_param.chip = chip;
	basic_param.dev_no = dev_no;

	if ((ret = ioctl(au_ioctl, KFLOW_AUDIOIO_IOC_GET_BASIC_PARAM, &basic_param)) < 0) {
		GMLIB_ERROR_P("ioctl \"KFLOW_AUDIOIO_IOC_GET_BASIC_PARAM\" fail(%d)\n", ret);
		ret = ERR_FAILED;
		goto exit;
	}
	if (p_sample_rate) {
		*p_sample_rate = basic_param.param.sample_rate;
	}
	if (p_bit_depth) {
		*p_bit_depth = basic_param.param.bit_depth;
	}
	if (p_tracks) {
		*p_tracks = basic_param.param.tracks;
	}
	if (p_data_sample_rate) {
		*p_data_sample_rate = basic_param.param.data_sample_rate;
	}

	//GMLIB_L1_P("%s, %s: dev_no(%d) get value(%d,%d,%d)\n", __func__,
	//			(is_rx ? "RX" : "TX"), dev_no, basic_param.param.sample_rate,
	//			basic_param.param.bit_depth, basic_param.param.tracks);
exit:
	return ret;
}


int pif_au_set_basic_param(int is_rx, int chip, int dev_no, int sample_rate, int bit_depth, int tracks,
							int data_sample_rate)
{
	int ret = 0;
	KFLOW_AUDIOIO_BASIC_PARAM basic_param;
	basic_param.rx_tx = is_rx ? KFLOW_AUDIOIO_RX_PARAM : KFLOW_AUDIOIO_TX_PARAM;
	basic_param.chip = chip;
	basic_param.dev_no = dev_no;
	basic_param.param.sample_rate = sample_rate;
	basic_param.param.bit_depth = bit_depth;
	basic_param.param.tracks = tracks;
	basic_param.param.data_sample_rate = data_sample_rate;

	GMLIB_L1_P("%s, %s: dev_no(%d) value(%d,%d,%d,%d)\n",
		   __func__, (is_rx ? "RX" : "TX"), dev_no, sample_rate, bit_depth, tracks, data_sample_rate);

	if ((ret = ioctl(au_ioctl, KFLOW_AUDIOIO_IOC_SET_BASIC_PARAM, &basic_param)) < 0) {
		GMLIB_ERROR_P("ioctl \"KFLOW_AUDIOIO_IOC_SET_BASIC_PARAM\" fail(%d)\n", ret);
		ret = ERR_FAILED;
	}
	return ret;
}

int pif_au_get_param_by_id(int is_rx, int chip, int dev_no, int feature_id, unsigned int *p_value)
{
	int ret = 0;
	KFLOW_AUDIOIO_ONE_PARAM au_param;
	au_param.rx_tx = is_rx ? KFLOW_AUDIOIO_RX_PARAM : KFLOW_AUDIOIO_TX_PARAM;
	au_param.chip = chip;
	au_param.dev_no = dev_no;
	au_param.param.feature_id = feature_id;

	if ((ret = ioctl(au_ioctl, KFLOW_AUDIOIO_IOC_GET_PARAM_BY_ID, &au_param)) < 0) {
		GMLIB_ERROR_P("ioctl \"KFLOW_AUDIOIO_IOC_GET_PARAM_BY_ID\" fail(%d)\n", ret);
		ret = ERR_FAILED;
	}
	if (p_value) {
		*p_value = au_param.param.value;
	}

	//GMLIB_L1_P("%s, %s: dev_no(%d) id(%d) get value(%u)\n", __func__,
	//			(is_rx ? "RX" : "TX"), dev_no, feature_id, au_param.param.value);
	return ret;
}


int pif_au_set_param_by_id(int is_rx, int chip, int dev_no, int feature_id, unsigned int value)
{
	int ret = 0;
	KFLOW_AUDIOIO_ONE_PARAM au_param;
	au_param.rx_tx = is_rx ? KFLOW_AUDIOIO_RX_PARAM : KFLOW_AUDIOIO_TX_PARAM;
	au_param.chip = chip;
	au_param.dev_no = dev_no;
	au_param.param.feature_id = feature_id;
	au_param.param.value = value;

	GMLIB_L1_P("%s, %s: dev_no(%d) id(%d) value(%u)\n",
		   __func__, (is_rx ? "RX" : "TX"), dev_no, feature_id, value);

	if ((ret = ioctl(au_ioctl, KFLOW_AUDIOIO_IOC_SET_PARAM_BY_ID, &au_param)) < 0) {
		GMLIB_ERROR_P("ioctl \"KFLOW_AUDIOIO_IOC_SET_PARAM_BY_ID\" fail(%d)\n", ret);
		ret = ERR_FAILED;
	}
	return ret;
}


int pif_au_get_init_param_by_id(int is_rx, int chip, int dev_no, int feature_id, unsigned int *p_value)
{
	int ret = 0;
	KFLOW_AUDIOIO_ONE_PARAM au_param;
	au_param.rx_tx = is_rx ? KFLOW_AUDIOIO_RX_PARAM : KFLOW_AUDIOIO_TX_PARAM;
	au_param.chip = chip;
	au_param.dev_no = dev_no;
	au_param.param.feature_id = feature_id;

	if ((ret = ioctl(au_ioctl, KFLOW_AUDIOIO_IOC_GET_INIT_PARAM_BY_ID, &au_param)) < 0) {
		GMLIB_ERROR_P("ioctl \"KFLOW_AUDIOIO_IOC_GET_INIT_PARAM_BY_ID\" fail(%d)\n", ret);
		ret = ERR_FAILED;
	}
	if (p_value) {
		*p_value = au_param.param.value;
	}

	//GMLIB_L1_P("%s, %s: dev_no(%d) id(%d) get value(%u)\n", __func__,
	//			(is_rx ? "RX" : "TX"), dev_no, feature_id, au_param.param.value);
	return ret;
}

int pif_au_set_init_param_by_id(int is_rx, int chip, int dev_no, int feature_id, unsigned int value)
{
	int ret = 0;
	KFLOW_AUDIOIO_ONE_PARAM au_param;
	au_param.rx_tx = is_rx ? KFLOW_AUDIOIO_RX_PARAM : KFLOW_AUDIOIO_TX_PARAM;
	au_param.chip = chip;
	au_param.dev_no = dev_no;
	au_param.param.feature_id = feature_id;
	au_param.param.value = value;

	GMLIB_L1_P("%s, %s: dev_no(%d) id(%d) value(%u)\n",
		   __func__, (is_rx ? "RX" : "TX"), dev_no, feature_id, value);

	if ((ret = ioctl(au_ioctl, KFLOW_AUDIOIO_IOC_SET_INIT_PARAM_BY_ID, &au_param)) < 0) {
		GMLIB_ERROR_P("ioctl \"KFLOW_AUDIOIO_IOC_SET_INIT_PARAM_BY_ID\" fail(%d)\n", ret);
		ret = ERR_FAILED;
	}
	return ret;
}


int pif_au_set_tx_volume(int chip, int dev_no, unsigned int volume)
{
	int ret = 0;
	KFLOW_AUDIOIO_VOLUME volume_param;
	volume_param.rx_tx = KFLOW_AUDIOIO_TX_PARAM;
	volume_param.chip = chip;
	volume_param.dev_no = dev_no;
	volume_param.volume = volume;

	if ((ret = ioctl(au_ioctl, KFLOW_AUDIOIO_IOC_SET_VOLUME, &volume_param)) < 0) {
		GMLIB_ERROR_P("ioctl \"KFLOW_AUDIOIO_IOC_SET_VOLUME\" fail(%d)\n", ret);
		ret = ERR_FAILED;
	}
	GMLIB_L1_P("%s, TX: dev_no(%d) set volume(%d), ret(%d)\n", __func__, dev_no, volume, ret);
	return ret;
}


int pif_au_get_rx_status(int chip, int dev_no, unsigned int *p_queue_depth, unsigned int *p_underrun_count)
{
	int ret = 0;
	KFLOW_AUDIOIO_DEV_STATUS dev_status;
	dev_status.rx_tx = KFLOW_AUDIOIO_RX_PARAM;
	dev_status.chip = chip;
	dev_status.dev_no = dev_no;

	if ((ret = ioctl(au_ioctl, KFLOW_AUDIOIO_IOC_GET_DEV_STATUS, &dev_status)) < 0) {
		GMLIB_ERROR_P("ioctl \"KFLOW_AUDIOIO_IOC_GET_DEV_STATUS\" fail(%d)\n", ret);
		ret = ERR_FAILED;
	}
	if (p_queue_depth) {
		*p_queue_depth = dev_status.rx.queue_depth;
	}
	if (p_underrun_count) {
		*p_underrun_count = dev_status.rx.underrun_count;
	}
	//GMLIB_L1_P("%s, RX: dev_no(%d)\n", __func__, dev_no);
	return ret;
}

int pif_au_get_tx_status(int chip, int dev_no, unsigned int *p_queue_depth, unsigned int *p_underrun_count)
{
	int ret = 0;
	KFLOW_AUDIOIO_DEV_STATUS dev_status;
	dev_status.rx_tx = KFLOW_AUDIOIO_TX_PARAM;
	dev_status.chip = chip;
	dev_status.dev_no = dev_no;

	if ((ret = ioctl(au_ioctl, KFLOW_AUDIOIO_IOC_GET_DEV_STATUS, &dev_status)) < 0) {
		GMLIB_ERROR_P("ioctl \"KFLOW_AUDIOIO_IOC_GET_DEV_STATUS\" fail(%d)\n", ret);
		ret = ERR_FAILED;
	}
	if (p_queue_depth) {
		*p_queue_depth = dev_status.tx.queue_depth;
	}
	if (p_underrun_count) {
		*p_underrun_count = dev_status.tx.underrun_count;
	}
	//GMLIB_L1_P("%s, RX: dev_no(%d)\n", __func__, dev_no);
	return ret;
}


void pif_frammap_init(void)
{
	return;
}

void pif_frammap_release(void)
{
	return;
}

void *pif_alloc_video_buffer_for_hdal(int size, int ddr_id, char pool_name[], int alloc_tag)
{
	int ret;
	void *ret_addr = (void *)HD_COMMON_MEM_VB_INVALID_BLK;
	ms_usr_buffer_info_t usr_buf_info;

	if (alloc_tag != USR_LIB && alloc_tag != USR_USR) {
		GMLIB_ERROR_P("Check alloc_tag(%#x) fail\n", alloc_tag);
		goto exit;
	}
	if (!size) {
		GMLIB_ERROR_P("Check size(%#x) fail\n", size);
		goto exit;
	}
	usr_buf_info.ddr_id = ddr_id;
	if (strlen(pool_name)) {
		strncpy(usr_buf_info.pool_name, pool_name, MAX_POOL_NAME_LEN);
	} else {
		sprintf(usr_buf_info.pool_name, "common_ddr%d", usr_buf_info.ddr_id);
	}
	usr_buf_info.usr_name[0] = '\0';
	usr_buf_info.size = size;
	usr_buf_info.alloc_tag = alloc_tag;

	if (ms_ioctl == 0) {
		pif_ms_ioctl_init();
	}
	if (ioctl(ms_ioctl, USR_MS_ALLOC_BUFFER, &usr_buf_info) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -4) {
			GMLIB_ERROR_P("Not find %s pool to alloc\n", pool_name);
		}
		if (ret == -1) {
			GMLIB_ERROR_P("Pool %s has no enough memory %d\n", pool_name, size);
		}
		GMLIB_ERROR_P("<ioctl \"USR_MS_ALLOC_BUFFER\" fail, error(%d)>\n", ret);
		goto exit;
	}
	ret_addr = (void *)usr_buf_info.addr_pa;
exit:
	return ret_addr;
}

void *pif_alloc_video_buffer_for_hdal_with_name(int size, int ddr_id, char pool_name[], int alloc_tag, char usr_name[])
{
	int ret;
	void *ret_addr = (void *)HD_COMMON_MEM_VB_INVALID_BLK;
	ms_usr_buffer_info_t usr_buf_info;

	if (alloc_tag != USR_LIB && alloc_tag != USR_USR) {
		GMLIB_ERROR_P("Check alloc_tag(%#x) fail\n", alloc_tag);
		goto exit;
	}
	if (!size) {
		GMLIB_ERROR_P("Check size(%#x) fail\n", size);
		goto exit;
	}
	usr_buf_info.ddr_id = ddr_id;
	if (strlen(pool_name)) {
		strncpy(usr_buf_info.pool_name, pool_name, MAX_POOL_NAME_LEN);
	} else {
		sprintf(usr_buf_info.pool_name, "common_ddr%d", usr_buf_info.ddr_id);
	}

	strcpy(usr_buf_info.usr_name, usr_name);	
	usr_buf_info.size = size;
	usr_buf_info.alloc_tag = alloc_tag;

	if (ms_ioctl == 0) {
		pif_ms_ioctl_init();
	}
	if (ioctl(ms_ioctl, USR_MS_ALLOC_BUFFER, &usr_buf_info) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -4) {
			GMLIB_ERROR_P("Not find %s pool to alloc\n", pool_name);
		}
		if (ret == -1) {
			GMLIB_ERROR_P("Pool %s has no enough memory %d\n", pool_name, size);
		}
		GMLIB_ERROR_P("<ioctl \"USR_MS_ALLOC_BUFFER\" fail, error(%d)>\n", ret);
		goto exit;
	}
	ret_addr = (void *)usr_buf_info.addr_pa;
exit:
	return ret_addr;
}


int pif_free_video_buffer_for_hdal(void *pa, int ddr_id, int alloc_tag)
{
	int ret = 0;
	ms_usr_buffer_info_t usr_buf_info;

	if (alloc_tag != USR_LIB && alloc_tag != USR_USR) {
		GMLIB_ERROR_P("Check alloc_tag(%#x) fail\n", alloc_tag);
		goto exit;
	}

	usr_buf_info.ddr_id = ddr_id;
	usr_buf_info.addr_pa = (uintptr_t)pa;
	usr_buf_info.alloc_tag = alloc_tag;

	if (!usr_buf_info.addr_pa) {
		GMLIB_ERROR_P("%s:check usr_buf_info.addr_pa=%p,fail\n", __func__, (void *)usr_buf_info.addr_pa);
		ret = -2;
		goto exit;
	}
	if (ms_ioctl == 0) {
		pif_ms_ioctl_init();
	}
	if (ioctl(ms_ioctl, USR_MS_FREE_BUFFER, &usr_buf_info) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -4) {
			GMLIB_ERROR_P("Not find pool to free\n");
		}
		if (ret == -1) {
			GMLIB_ERROR_P("Addr doesn't belong to pool\n");
		}
		GMLIB_ERROR_P("<ioctl \"USR_MS_FREE_BUFFER\" fail, error(%d)>\n", ret);
		goto exit;
	}
exit:
	return ret;
}


int pif_query_video_buffer_size(uintptr_t phy_addr, unsigned int ddr_id, char pool_name[], unsigned int *p_size)
{
	int ret = 0;
	ms_usr_query_size_t query_size = {0};

	if (phy_addr == 0) {
		GMLIB_ERROR_P("pif_query_video_buffer_size fail, phy_addr is 0.\n");
		ret = -1;
		goto exit;
	}
	if (ddr_id > DDR_ID_MAX) {
		GMLIB_ERROR_P("pif_query_video_buffer_size fail, ddr_id(%u) over max(%u).\n", ddr_id, DDR_ID_MAX);
		ret = -1;
		goto exit;
	}

	query_size.addr_pa = phy_addr;
	query_size.ddr_id = ddr_id;
	strncpy(query_size.pool_name, pool_name, MAX_POOL_NAME_LEN);
	if (ms_ioctl == 0) {
		pif_ms_ioctl_init();
	}
	if (ioctl(ms_ioctl, USR_MS_GET_BUFFER_SZ, &query_size) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -2) {
			/* can not find pool by this pa, skip it */
		} else if (ret < 0) {
			GMLIB_ERROR_P("<ioctl \"USR_MS_GET_BUFFER_SZ\" fail, error(%d)>\n", ret);
		}
		goto exit;
	}
	*p_size = query_size.free_size;

exit:
	return ret;
}


int pif_layout_fixepool_for_hdal(int layout_attr_nr, void *layout_attr, char pool_name[], int del_layout)
{
	int ret = 0;
	ms_user_fixed_pool_layout_t usr_layout_info;
	if (strlen(pool_name)) {
		strncpy(usr_layout_info.pool_name, pool_name, MAX_POOL_NAME_LEN);
	} else {
		GMLIB_ERROR_P("pool name invalid\n");
		return ret;
	}
	usr_layout_info.user_del_layout = del_layout;
	usr_layout_info.layout_attr_nr = layout_attr_nr;
	usr_layout_info.layout_attr = (ms_user_layout_attr_t *)layout_attr;
	if (ms_ioctl == 0) {
		pif_ms_ioctl_init();
	}
	if (ioctl(ms_ioctl, USR_MS_POOL_LAYOUT, &usr_layout_info) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -2) {
			GMLIB_ERROR_P("Not find %s pool to alloc\n", pool_name);
		}
		GMLIB_ERROR_P("<ioctl \"USR_MS_POOL_LAYOUT\" fail, error(%d)>\n", ret);
		goto exit;
	}
exit:
	return ret;
}


int pif_get_pool_usedbuf_info(int *p_used_buf_nr, void *buf_info, char pool_name[])
{
	int ret = 0;
	ms_user_pool_buf_info_t usr_layout_info;

	if (strlen(pool_name)) {
		strncpy(usr_layout_info.pool_name, pool_name, MAX_POOL_NAME_LEN);
	} else {
		GMLIB_ERROR_P("pool name invalid\n");
		return ret;
	}
	usr_layout_info.buf_info_nr = *p_used_buf_nr;
	usr_layout_info.buf_info = (ms_user_buf_info_t *)buf_info;
	if (ms_ioctl == 0) {
		pif_ms_ioctl_init();
	}
	if (ioctl(ms_ioctl, USR_MS_GET_POOL_BUF_INFO, &usr_layout_info) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -2) {
			GMLIB_ERROR_P("Not find %s pool\n", pool_name);
		}
		GMLIB_ERROR_P("<ioctl \"USR_MS_GET_POOL_BUF_INFO\" fail, error(%d)>\n", ret);
		goto exit;
	}

	*p_used_buf_nr = usr_layout_info.buf_info_nr;
exit:
	return ret;
}

int pif_get_left_frame_count_with_next_fd(unsigned int fd, unsigned int next_fd,
		unsigned int *p_left_count, unsigned int *p_done_count, unsigned int *p_dti_buf_cnt)
{
	int ret = 0;
	gs_left_frames_t left_frames = {0};

	left_frames.entity_fd = fd;
	left_frames.next_entity_fd = next_fd;
	left_frames.number_of_frames = *p_left_count;
	left_frames.done_frames = *p_done_count;
	left_frames.dti_buf_cnt = *p_dti_buf_cnt;
	if (ioctl(gs_ioctl, GS_IOC_GET_LEFT_FRAMES, &left_frames) < 0) {
		GMLIB_L1_P("ioctl \"GS_IOC_GET_LEFT_FRAMES\" fail(%d)\n", ret);
		ret = ERR_FAILED;
		goto err_ext;
	}
	if (p_left_count) {
		*p_left_count = left_frames.number_of_frames;
	}
	if (p_done_count) {
		*p_done_count = left_frames.done_frames;
	}
	if (p_dti_buf_cnt) {
		*p_dti_buf_cnt = left_frames.dti_buf_cnt;
	}

err_ext:
	return ret;
}

int pif_get_left_frame_count(unsigned int fd, unsigned int *p_left_count, unsigned int *p_done_count, unsigned int *p_dti_buf_cnt)
{
	return pif_get_left_frame_count_with_next_fd(fd, 0, p_left_count, p_done_count, p_dti_buf_cnt);
}

int pif_get_reserved_ref_frame(unsigned int fd, unsigned int *p_enable)
{
	int ret = 0;
	gs_no_stop_t no_stop;

	no_stop.entity_fd = fd;
	no_stop.enable = 0;
	if (ioctl(gs_ioctl, GS_IOC_GET_NOT_CALL_EM_STOP, &no_stop) < 0) {
		GMLIB_L1_P("ioctl \"GS_IOC_GET_NOT_CALL_EM_STOP\" fail(%d)\n", ret);
		ret = ERR_FAILED;
		goto err_ext;
	}
	*p_enable = no_stop.enable;

err_ext:
	return ret;
}

int pif_set_reserved_ref_frame(unsigned int fd, unsigned int enable)
{
	int ret = 0;
	gs_no_stop_t no_stop;

	no_stop.entity_fd = fd;
	no_stop.enable = enable;
	if (ioctl(gs_ioctl, GS_IOC_SET_NOT_CALL_EM_STOP, &no_stop) < 0) {
		GMLIB_L1_P("ioctl \"GS_IOC_SET_NOT_CALL_EM_STOP\" fail(%d)\n", ret);
		ret = ERR_FAILED;
		goto err_ext;
	}

err_ext:
	return ret;
}

unsigned int pif_get_pool_size(char pool_name[], unsigned int ddr_id)
{
	unsigned int pool_size = 0;
	ms_usr_query_size_t usr_pool_info;
	int ret = 0;

	if (strlen(pool_name)) {
		strncpy(usr_pool_info.pool_name, pool_name, MAX_POOL_NAME_LEN);
	} else {
		GMLIB_ERROR_P("pool name invalid\n");
		return ret;
	}
	if (ddr_id > DDR_ID_MAX) {
		GMLIB_ERROR_P("pif_get_pool_size fail, ddr_id(%u) over max(%u).\n", ddr_id, DDR_ID_MAX);
		ret = -1;
		goto exit;
	}
	if (ms_ioctl == 0) {
		pif_ms_ioctl_init();
	}
	usr_pool_info.ddr_id = ddr_id;
	if (ioctl(ms_ioctl, USR_MS_GET_POOL_SIZE, &usr_pool_info) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		GMLIB_ERROR_P("<ioctl \"USR_MS_GET_POOL_SIZE\" fail, error(%d)>\n", ret);
		goto exit;
	}
	pool_size = usr_pool_info.free_size;
exit:
	return pool_size;
}


int pif_set_usr_init_frame(unsigned int fd, unsigned long addr_pa,
		unsigned int size, unsigned int ddr_id, HD_DIM dim, HD_DIM bg_dim, HD_VIDEO_PXLFMT pxl_fmt)
{
	int ret = 0;
	gs_usr_init_frame_t usr_init_frame;

	usr_init_frame.entity_fd = fd;
	usr_init_frame.addr_va = 0;
	usr_init_frame.addr_pa = addr_pa;
	usr_init_frame.size = size;
	usr_init_frame.ddr_id = ddr_id;
	usr_init_frame.dim.width = dim.w;
	usr_init_frame.dim.height = dim.h;
	usr_init_frame.bg_dim.width = bg_dim.w;
	usr_init_frame.bg_dim.height = bg_dim.h;
	usr_init_frame.format = common_convert_HD_VIDEO_PXLFMT_to_buf_type_value(pxl_fmt);
	if (ioctl(gs_ioctl, GS_IOC_USR_INIT_FRAME, &usr_init_frame) < 0) {
		GMLIB_L1_P("ioctl \"GS_IOC_USR_INIT_FRAME\" fail(%d)\n", ret);
		ret = ERR_FAILED;
		goto err_ext;
	}

err_ext:
	return ret;
}

