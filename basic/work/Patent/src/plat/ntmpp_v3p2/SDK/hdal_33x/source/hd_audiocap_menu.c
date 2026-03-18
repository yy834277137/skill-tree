/**
    @brief Source code of audiocapture menu.\n
    This file contains the functions which is related to audiocapture debug menu.

    @file hd_audiocapture_menu.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_common.h"
#include "hd_type.h"
#include "hd_util.h"
#include "hd_audiocapture.h"
#include "hd_debug_menu.h"
#include <string.h>

#define AUDCAP_DEVID(self_id)   		(self_id - HD_DAL_AUDIOCAP_BASE)
#define AUDCAP_INPORTID(self_id)   		(self_id - HD_IN_BASE)
#define AUDCAP_OUTPORTID(self_id)   	(self_id - HD_OUT_BASE)

extern HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len);
extern HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len);
extern HD_RESULT bd_check_exist(HD_PATH_ID path_id);
extern UINT bd_is_path_start(HD_PATH_ID path_id);
extern int _hd_video_pxlfmt_str(HD_VIDEO_PXLFMT pxlfmt, CHAR *p_str, INT str_len);
INT dif_dbg_print(INT where, CHAR *buf, const CHAR *fmt, ...);

#define BD_SET_PATH(dev_id, in_id, out_id)  (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

/* int audiocap_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int audiocap_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
{
	int j, print_dev_once = 1, len = 0;

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, j);
		char bind_src[32] = "-", bind_dest[32] = "-";
		char state_name[32] = "OPEN";

		if (bd_check_exist(path_id) == 0)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- AUDIOCAP %-2d PATH & BIND -----------------------------\r\n", AUDCAP_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     state   bind_src                bind_dest\r\n");
		}

		if (bd_get_next(deviceid, j, bind_dest, sizeof(bind_dest)) == HD_OK) {
			if (bd_is_path_start(path_id)) {
				strcpy(state_name, "START");
			}
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-5s   %-20s    %-20s\r\n",
				     AUDCAP_INPORTID(HD_IN_BASE), AUDCAP_OUTPORTID(j), path_id, state_name, bind_src, bind_dest);
	}

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, j);
		HD_AUDIOCAP_IN audiocap_param;
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_audiocap_get(path_id, HD_AUDIOCAP_PARAM_IN, &audiocap_param) != HD_OK)
			continue;

		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- AUDIOCAP %-2d IN -----------------------------\r\n", AUDCAP_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     rate    bit     samples\r\n");
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d    %-4d    %-4d    %-5s\r\n",
				     AUDCAP_INPORTID(HD_IN_BASE), AUDCAP_OUTPORTID(j), path_id, audiocap_param.sample_rate,
				     audiocap_param.sample_bit, (int)audiocap_param.frame_sample,
				     (audiocap_param.mode == HD_AUDIO_SOUND_MODE_MONO) ? "MONO" : "STEREO");
	}
	if (print_dev_once == 0)
		len += dif_dbg_print(where, p_buf + len, "\n\n");

	return len;
}

/* audiocap_show_status_p(int where, char *p_buf)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int audiocap_show_status_p(int where, char *p_buf)
{
	int i, len = 0;

	for (i = HD_DAL_AUDIOCAP_BASE; i < HD_DAL_AUDIOCAP_MAX; i++) {
		len += audiocap_devid_show_status_p(where, p_buf + len, i);
	}
	return len;
}

static int hd_audiocap_show_status_p(void)
{
	return (audiocap_show_status_p(0, NULL));
}

int hd_audiocap_show_to_proc_status_p(CHAR *p_buf)
{
	return (audiocap_show_status_p(1, p_buf));
}

int hd_audiocap_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid)
{
	return (audiocap_devid_show_status_p(1, p_buf, deviceid));
}

static HD_DBG_MENU audiocap_debug_menu[] = {
	{0x01, "dump status",                         hd_audiocap_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_audiocap_menu_p(void)
{
	return hd_debug_menu_entry_p(audiocap_debug_menu, "AUDIOCAP");
}

