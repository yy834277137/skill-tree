/**
    @brief Source code of audioout menu.\n
    This file contains the functions which is related to audioout debug menu.

    @file hd_audioout_menu.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_common.h"
#include "hd_type.h"
#include "hd_util.h"
#include "hd_audioout.h"
#include "hd_debug_menu.h"
#include <string.h>

#define AUDOUT_DEVID(self_id)   		(self_id - HD_DAL_AUDIOOUT_BASE)
#define AUDOUT_INPORTID(self_id)   		(self_id - HD_IN_BASE)
#define AUDOUT_OUTPORTID(self_id)   	(self_id - HD_OUT_BASE)

extern HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len);
extern HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len);
extern HD_RESULT bd_check_exist(HD_PATH_ID path_id);
extern UINT bd_is_path_start(HD_PATH_ID path_id);
extern int _hd_video_pxlfmt_str(HD_VIDEO_PXLFMT pxlfmt, CHAR *p_str, INT str_len);
INT dif_dbg_print(INT where, CHAR *buf, const CHAR *fmt, ...);

#define BD_SET_PATH(dev_id, in_id, out_id)  (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

/* audioout_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int audioout_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
{
	int j, print_dev_once, len = 0;

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, j);
		char bind_src[32] = "-", bind_dest[32] = "-";
		char state_name[32] = "OPEN";

		if (bd_check_exist(path_id) == 0)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- AUDIOOUT %-2d PATH & BIND -----------------------------\r\n", AUDOUT_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     state   bind_src                bind_dest\r\n");
		}

		if (bd_get_prev(deviceid, j, bind_src, sizeof(bind_src)) == HD_OK) {
			if (bd_is_path_start(path_id)) {
				strcpy(state_name, "START");
			}
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-5s   %-20s    %-20s\r\n",
				     AUDOUT_INPORTID(HD_IN_BASE), AUDOUT_OUTPORTID(j), path_id, state_name, bind_src, bind_dest);
	}

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, j);
		HD_AUDIOOUT_IN audioout_param_in;
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_audioout_get(path_id, HD_AUDIOOUT_PARAM_IN, &audioout_param_in) != HD_OK)
			continue;

		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- AUDIOOUT %-2d IN -----------------------------\r\n", AUDOUT_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     rate\r\n");
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d \r\n",
				     AUDOUT_OUTPORTID(HD_IN_BASE), AUDOUT_OUTPORTID(j), path_id, audioout_param_in.sample_rate);
	}
	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, j);
		HD_AUDIOOUT_OUT audioout_param_out;
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_audioout_get(path_id, HD_AUDIOOUT_PARAM_OUT, &audioout_param_out) != HD_OK)
			continue;

		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- AUDIOOUT %-2d OUT -----------------------------\r\n", AUDOUT_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     rate   bit     mode\r\n");
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d   %-4d    %-4d\r\n",
				     AUDOUT_OUTPORTID(HD_IN_BASE), AUDOUT_OUTPORTID(j), path_id,
				     audioout_param_out.sample_rate, audioout_param_out.sample_bit, audioout_param_out.mode);
	}

	if (print_dev_once == 0)
		len += dif_dbg_print(where, p_buf + len, "\n\n");

	return len;
}

/* audioout_show_status_p(int where, char *p_buf)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int audioout_show_status_p(int where, char *p_buf, unsigned int buf_len)
{
	unsigned int i, len = 0;

	for (i = HD_DAL_AUDIOOUT_BASE; i < HD_DAL_AUDIOOUT_MAX; i++) {
		len += audioout_devid_show_status_p(where, p_buf + len, i);
		if (where && (len > (buf_len - HD_DEBUG_MSG_SIZE)))
			break;
	}
	return len;
}

static int hd_audioout_show_status_p(void)
{
	return (audioout_show_status_p(0, NULL, 0));
}

int hd_audioout_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len)
{
	return (audioout_show_status_p(1, p_buf, buf_len));
}

int hd_audioout_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid)
{
	return (audioout_devid_show_status_p(1, p_buf, deviceid));
}

static HD_DBG_MENU audioout_debug_menu[] = {
	{0x01, "dump status",                         hd_audioout_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_audioout_menu_p(void)
{
	return hd_debug_menu_entry_p(audioout_debug_menu, "AUDIOOUT");
}

