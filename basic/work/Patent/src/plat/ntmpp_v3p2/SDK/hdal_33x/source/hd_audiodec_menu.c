/**
    @brief Source code of audiodec menu.\n
    This file contains the functions which is related to audiodec debug menu.

    @file hd_audiodec_menu.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_common.h"
#include "hd_type.h"
#include "hd_util.h"
#include "hd_audiodec.h"
#include "hd_debug_menu.h"
#include <string.h>

#define AUDDEC_DEVID(self_id)   		(self_id - HD_DAL_AUDIODEC_BASE)
#define AUDDEC_INPORTID(self_id)   		(self_id - HD_IN_BASE)
#define AUDDEC_OUTPORTID(self_id)   	(self_id - HD_OUT_BASE)

extern HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len);
extern HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len);
extern HD_RESULT bd_check_exist(HD_PATH_ID path_id);
INT dif_dbg_print(INT where, CHAR *buf, const CHAR *fmt, ...);

#define BD_SET_PATH(dev_id, in_id, out_id)  (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

/* int audiodec_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int audiodec_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
{
	int j, print_dev_once = 1, len = 0;

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_AUDIODEC_IN audiodec_param;
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_audiodec_get(path_id, HD_AUDIODEC_PARAM_IN, &audiodec_param) != HD_OK)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- AUDIODEC %-2d IN -----------------------------\r\n", AUDDEC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     rate    bit     samples\r\n");
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d    %-4d    %-5s\r\n",
				     AUDDEC_INPORTID(j), AUDDEC_OUTPORTID(j), path_id,
				     audiodec_param.sample_rate, audiodec_param.sample_bit,
				     (audiodec_param.mode == HD_AUDIO_SOUND_MODE_MONO) ? "MONO" : "STEREO");
	}
	if (print_dev_once == 0)
		len += dif_dbg_print(where, p_buf + len, "\n\n");
	return len;
}

/* audiodec_show_status_p(int where, char *p_buf)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int audiodec_show_status_p(int where, char *p_buf, unsigned int buf_len)
{
	unsigned int i, len = 0;

	for (i = HD_DAL_AUDIODEC_BASE; i < HD_DAL_AUDIODEC_MAX; i++) {
		len += audiodec_devid_show_status_p(where, p_buf + len, i);
		if (where && (len > (buf_len - HD_DEBUG_MSG_SIZE)))
			break;
	}
	return len;
}

static int hd_audiodec_show_status_p(void)
{
	return (audiodec_show_status_p(0, NULL, 0));
}

int hd_audiodec_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len)
{
	return (audiodec_show_status_p(1, p_buf, buf_len));
}

int hd_audiodec_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid)
{
	return (audiodec_devid_show_status_p(1, p_buf, deviceid));
}

static HD_DBG_MENU audiodec_debug_menu[] = {
	{0x01, "dump status",                         hd_audiodec_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_audiodec_menu_p(void)
{
	return hd_debug_menu_entry_p(audiodec_debug_menu, "AUDIODEC");
}

