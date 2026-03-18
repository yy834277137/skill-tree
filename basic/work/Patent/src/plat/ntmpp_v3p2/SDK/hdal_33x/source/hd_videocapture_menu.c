/**
    @brief Source code of videocapture menu.\n
    This file contains the functions which is related to videocapture debug menu.

    @file hd_videocapture_menu.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_common.h"
#include "hd_type.h"
#include "hd_util.h"
#include "hd_videocapture.h"
#include "hd_debug_menu.h"
#include <string.h>

#define VDOCAP_DEVID(self_id)   		(self_id - HD_DAL_VIDEOCAP_BASE)
#define VDOCAP_INPORTID(self_id)   		(self_id - HD_IN_BASE)
#define VDOCAP_OUTPORTID(self_id)   	(self_id - HD_OUT_BASE)

extern HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len);
extern HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len);
extern HD_RESULT bd_check_exist(HD_PATH_ID path_id);
extern UINT bd_is_path_start(HD_PATH_ID path_id);
extern int _hd_video_pxlfmt_str(HD_VIDEO_PXLFMT pxlfmt, CHAR *p_str, INT str_len);
INT dif_dbg_print(INT where, CHAR *buf, const CHAR *fmt, ...);

#define BD_SET_PATH(dev_id, in_id, out_id)  (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

/* static int videocap_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int videocap_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
{
	int j, k, print_dev_once = 1, len = 0;
	char pxlfmt[16];
	HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, HD_OUT_BASE);
	HD_VIDEOCAP_SYSCAPS cap_syscaps;

	if (bd_check_exist(path_id) == 0)
		goto ext;
	path_id = HD_VIDEOCAP_CTRL(deviceid - HD_DAL_VIDEOCAP_BASE);
	if (hd_videocap_get(path_id, HD_VIDEOCAP_PARAM_SYSCAPS, &cap_syscaps) != HD_OK)
		goto ext;
	if (print_dev_once) {
		print_dev_once = 0;
		len += dif_dbg_print(where, p_buf + len, "========================== VIDEOCAP %-2d SYSCAP ==========================\r\n", VDOCAP_DEVID(deviceid));

		len += dif_dbg_print(where, p_buf + len, "w       h       fps     scaling\r\n");
	}

	len += dif_dbg_print(where, p_buf + len, "%-4d    %-4d    %-4d    %-4d\r\n", (int)cap_syscaps.max_dim.w,
			     (int)cap_syscaps.max_dim.h, (int)cap_syscaps.max_frame_rate, (int)cap_syscaps.max_w_scaleup);
	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, j);
		char bind_src[32] = "-", bind_dest[32] = "-";
		char state_name[32] = "OPEN";

		if (bd_check_exist(path_id) == 0) {
			continue;
		}
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOCAP %-2d PATH & BIND -----------------------------\r\n", VDOCAP_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     state   bind_src                bind_dest\r\n");
		}

		if (bd_get_next(deviceid, j, bind_dest, sizeof(bind_dest)) == HD_OK) {
			if (bd_is_path_start(path_id)) {
				strcpy(state_name, "START");
			}
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-5s   %-20s    %-20s\r\n",
				     VDOCAP_INPORTID(HD_IN_BASE), VDOCAP_OUTPORTID(j), path_id, state_name, bind_src, bind_dest);
	}

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, j);
		HD_VIDEOCAP_OUT cap_out;
		if (bd_check_exist(path_id) == 0) {
			continue;
		}
		if (hd_videocap_get(path_id, HD_VIDEOCAP_PARAM_OUT, &cap_out) != HD_OK) {
			continue;
		}
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOCAP %-2d OUT FRAME -----------------------------\r\n", VDOCAP_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     w       h       pxlfmt               frc\r\n");
		}
		_hd_video_pxlfmt_str(cap_out.pxlfmt, pxlfmt, sizeof(pxlfmt));
		if ((cap_out.dim.w != 0) && (cap_out.dim.h != 0))
			len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4lu    %-4lu    %-20s %#x\r\n",
					     VDOCAP_INPORTID(HD_IN_BASE), VDOCAP_OUTPORTID(j), path_id, cap_out.dim.w, cap_out.dim.h, pxlfmt, cap_out.frc);
	}

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_VIDEOCAP_PATH_CONFIG cap_config;
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, j);
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_videocap_get(path_id, HD_VIDEOCAP_PARAM_PATH_CONFIG, &cap_config) != HD_OK)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOCAP %-2d PATH POOL -----------------------------\r\n", VDOCAP_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     pool    ddr_id  count   max_count\r\n");
		}
		for (k = 0; k < 4; k++) {
			if (cap_config.data_pool[k].mode == HD_VIDEOCAP_POOL_ENABLE) {
				len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d    %-4d    %-4.1f    %-4.1f\r\n",
						     VDOCAP_INPORTID(HD_IN_BASE), VDOCAP_OUTPORTID(j), path_id, k,
						     cap_config.data_pool[k].ddr_id, (float)cap_config.data_pool[k].counts / 10,
						     (float)cap_config.data_pool[k].max_counts / 10);
			}
		}
	}

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_VIDEOCAP_CROP crop_attr;
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, j);
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_videocap_get(path_id, HD_VIDEOCAP_PARAM_IN_CROP, &crop_attr) != HD_OK)
			continue;
		if (crop_attr.mode != HD_CROP_ON)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOCAP %-2d IN CROP -----------------------------\r\n", VDOCAP_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     mode    x       y       w       h      coord.w   coord.h\r\n");
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x    %-4s    %-4d    %-4d    %-4d    %-4d   %-4d      %-4d\r\n",
				     VDOCAP_INPORTID(HD_IN_BASE), VDOCAP_OUTPORTID(j), path_id, "ON",
				     (int)crop_attr.win.rect.x, (int)crop_attr.win.rect.y, (int)crop_attr.win.rect.w, (int)crop_attr.win.rect.h,
				     (int)crop_attr.win.coord.w, (int)crop_attr.win.coord.h);
	}
	if (print_dev_once == 0)
		len += dif_dbg_print(where, p_buf + len, "\n\n");

ext:
	return len;
}

/* videocap_show_status_p(int where, char *p_buf)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int videocap_show_status_p(int where, char *p_buf, unsigned int buf_len)
{
	HD_DAL deviceid;
	unsigned int len = 0;

	for (deviceid = HD_DAL_VIDEOCAP_BASE; deviceid < HD_DAL_VIDEOCAP_MAX; deviceid++) {
		len += videocap_devid_show_status_p(where, p_buf + len, deviceid);
		if (where && (len > (buf_len - HD_DEBUG_MSG_SIZE)))
			break;
	}
	return len;
}

static int hd_videocap_show_status_p(void)
{
	return (videocap_show_status_p(0, NULL, 0));
}

int hd_videocap_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len)
{
	return (videocap_show_status_p(1, p_buf, buf_len));
}

int hd_videocap_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid)
{
	return (videocap_devid_show_status_p(1, p_buf, deviceid));
}

static HD_DBG_MENU videocap_debug_menu[] = {
	{0x01, "dump status",                         hd_videocap_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_videocap_menu_p(void)
{
	return hd_debug_menu_entry_p(videocap_debug_menu, "VIDEOCAP");
}

