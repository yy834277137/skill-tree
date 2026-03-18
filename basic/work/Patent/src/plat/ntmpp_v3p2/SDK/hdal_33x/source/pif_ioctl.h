/*
 *   @file   pif_ioctl.h.c
 *
 *   @brief  platform interface for driver ioctl header file.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#ifndef __PIF_IOCTL_H__
#define __PIF_IOCTL_H__

#define AUDIO_MAX_DEVICE_NUM 4

typedef struct {
	int audio_ssp_num[AUDIO_MAX_DEVICE_NUM];
	int audio_in_enable[AUDIO_MAX_DEVICE_NUM];
	int audio_ssp_chan[AUDIO_MAX_DEVICE_NUM];
	int sample_size[AUDIO_MAX_DEVICE_NUM];
	int sample_rate[AUDIO_MAX_DEVICE_NUM];
	int ssp_clock[AUDIO_MAX_DEVICE_NUM];
	int bit_clock[AUDIO_MAX_DEVICE_NUM];
	int audio_ssp_master[AUDIO_MAX_DEVICE_NUM];
	int live_sound_ch[AUDIO_MAX_DEVICE_NUM];
	int in_align;
} audiocap_init_t;

typedef struct {
	int audio_out_enable[AUDIO_MAX_DEVICE_NUM];
	int resample_ratio[AUDIO_MAX_DEVICE_NUM];
	int playback_chmap[AUDIO_MAX_DEVICE_NUM];
	int out_align;
} audioout_init_t;

typedef struct {
	audiocap_init_t aucap;
	audioout_init_t auout;
} audio_init_t;

//<duplicate with enum MS_TAG in ms_ioctl.h
enum USR_TAG {
	USR_GS = (1 << 0),
	USR_JPG = (1 << 1),
	USR_USR = (1 << 2),
	USR_PUSH = (1 << 3),
	USR_DUMP = (1 << 4),
	USR_LIB = (1 << 5),
	USR_TRIG = (1 << 6),
	USR_EM = (1 << 7),
};


void pif_vcap_init(void);
void pif_vcap_release(void);
void pif_osg_init(void);
void pif_osg_release(void);
void pif_vpe_init(void);
void pif_vpe_release(void);
void pif_au_init(void);
void pif_au_release(void);
int pif_audiocap_init(audiocap_init_t *aucap);
int pif_audioout_init(audioout_init_t *auout);
void pif_frammap_init(void);
void pif_frammap_release(void);
void pif_ms_ioctl_init(void);
void pif_ms_ioctl_release(void);
int pif_query_video_buffer_size(uintptr_t phy_addr, unsigned int ddr_id, char pool_name[], unsigned int *p_size);
int pif_layout_fixepool_for_hdal(int layout_attr_nr, void *layout_attr, char pool_name[], int del_layout);
int pif_get_pool_usedbuf_info(int *p_used_buf_nr, void *buf_info, char pool_name[]);
int pif_get_left_frame_count(unsigned int fd, unsigned int *p_left_count, unsigned int *p_done_count, unsigned int *p_dti_buf_cnt);
int pif_get_reserved_ref_frame(unsigned int fd, unsigned int *p_enable);
int pif_set_reserved_ref_frame(unsigned int fd, unsigned int enable);
int pif_au_livesound(unsigned int acap_dev_id, unsigned int acap_ch, unsigned int aout_dev_id, int on_off);
int pif_au_get_basic_param(int is_rx, int chip, int dev_no, int *p_sample_rate, int *p_bit_depth, int *p_tracks, int *p_data_sample_rate);
int pif_au_set_basic_param(int is_rx, int chip, int dev_no, int sample_rate, int bit_depth, int tracks, int data_sample_rate);
int pif_au_get_param_by_id(int is_rx, int chip, int dev_no, int feature_id, unsigned int *p_value);
int pif_au_set_param_by_id(int is_rx, int chip, int dev_no, int feature_id, unsigned int value);
int pif_au_get_init_param_by_id(int is_rx, int chip, int dev_no, int feature_id, unsigned int *p_value);
int pif_au_set_init_param_by_id(int is_rx, int chip, int dev_no, int feature_id, unsigned int value);
int pif_au_set_tx_volume(int chip, int dev_no, unsigned int volume);
int pif_au_get_rx_status(int chip, int dev_no, unsigned int *p_queue_depth, unsigned int *p_underrun_count);
int pif_au_get_tx_status(int chip, int dev_no, unsigned int *p_queue_depth, unsigned int *p_underrun_count);
unsigned int pif_get_pool_size(char pool_name[], unsigned int ddr_id);
int pif_set_usr_init_frame(unsigned int fd, unsigned long addr_pa,
		unsigned int size, unsigned int ddr_id, HD_DIM dim, HD_DIM bg_dim, HD_VIDEO_PXLFMT pxl_fmt);

#endif /* PIF_IOCTL_H_ */
