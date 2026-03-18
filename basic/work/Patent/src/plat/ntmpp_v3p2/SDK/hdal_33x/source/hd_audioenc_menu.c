/**
    @brief Source code of audioenc menu.\n
    This file contains the functions which is related to audioenc debug menu.

    @file hd_audioenc_menu.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_common.h"
#include "hd_type.h"
#include "hd_util.h"
#include "hd_audioenc.h"
#include "hd_debug_menu.h"
#include <string.h>

#define AUDENC_DEVID(self_id)   		(self_id - HD_DAL_AUDIOENC_BASE)
#define AUDENC_INPORTID(self_id)   		(self_id - HD_IN_BASE)
#define AUDENC_OUTPORTID(self_id)   	(self_id - HD_OUT_BASE)

extern HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len);
extern HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len);

extern HD_RESULT bd_check_exist(HD_PATH_ID path_id);
extern int _hd_audio_codec_str(HD_AUDIO_CODEC code_type, CHAR *p_str, INT str_len);
INT dif_dbg_print(INT where, CHAR *buf, const CHAR *fmt, ...);

#define BD_SET_PATH(dev_id, in_id, out_id)  (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

/* audioenc_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int audioenc_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
{
	int j, print_dev_once = 1, len = 0;
	char codec_type[16];

	print_dev_once = 1;
	for (j = HD_IN_BASE; j < HD_IN_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		HD_AUDIOENC_OUT audioenc_param;
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_audioenc_get(path_id, HD_AUDIOENC_PARAM_OUT, &audioenc_param) != HD_OK)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- AUDIOENC %-2d OUT -----------------------------\r\n", AUDENC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid      codec\r\n");
		}
		_hd_audio_codec_str(audioenc_param.codec_type, codec_type, sizeof(codec_type));
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x  %-20s\r\n",
				     AUDENC_INPORTID(j), AUDENC_INPORTID(j), path_id, codec_type);
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
static int audioenc_show_status_p(int where, char *p_buf, unsigned int buf_len)
{
	unsigned int i, len = 0;

	for (i = HD_DAL_AUDIOENC_BASE; i < HD_DAL_AUDIOENC_MAX; i++) {
		len += audioenc_devid_show_status_p(where, p_buf + len, i);
		if (where && (len > (buf_len - HD_DEBUG_MSG_SIZE)))
			break;
	}
	return len;
}

static int hd_audioenc_show_status_p(void)
{
	return (audioenc_show_status_p(0, NULL, 0));
}

int hd_audioenc_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len)
{
	return (audioenc_show_status_p(1, p_buf, buf_len));
}

int hd_audioenc_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid)
{
	return (audioenc_devid_show_status_p(1, p_buf, deviceid));
}

static HD_DBG_MENU audioenc_debug_menu[] = {
	{0x01, "dump status",                         hd_audioenc_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_audioenc_menu_p(void)
{
	return hd_debug_menu_entry_p(audioenc_debug_menu, "AUDIOENC");
}

