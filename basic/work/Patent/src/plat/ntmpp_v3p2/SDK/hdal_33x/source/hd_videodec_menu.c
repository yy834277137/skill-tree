/**
    @brief Source code of videodec menu.\n
    This file contains the functions which is related to videodec debug menu.

    @file hd_videodec_menu.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_common.h"
#include "hd_type.h"
#include "hd_util.h"
#include "hd_videodec.h"
#include "hd_debug_menu.h"
#include "videodec.h"
#include <string.h>

#define VDODEC_DEVID(self_id)   		(self_id - HD_DAL_VIDEODEC_BASE)
#define VDODEC_INPORTID(self_id)   		(self_id - HD_IN_BASE)
#define VDODEC_OUTPORTID(self_id)   	(self_id - HD_OUT_BASE)

extern HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len);
extern HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len);
extern UINT bd_is_path_start(HD_PATH_ID path_id);
extern int _hd_video_pxlfmt_str(HD_VIDEO_PXLFMT pxlfmt, CHAR *p_str, INT str_len);
extern VDODEC_DEV_PARAM dec_cfg[VDODEC_DEV];
INT dif_dbg_print(INT where, CHAR *buf, const CHAR *fmt, ...);

#define BD_SET_PATH(dev_id, in_id, out_id)  (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

CHAR *codec_string[HD_CODEC_TYPE_MAX] = {"MJPEG", "H264", "H265", "RAW"};
/* videodec_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int videodec_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
{
	int j, k, print_dev_once = 1, len = 0;

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		char bind_src[32] = "-", bind_dest[32] = "-";
		char state_name[32] = "OPEN";
		if (bd_check_exist(path_id) == 0)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEODEC %-2d PATH & BIND -----------------------------\r\n", VDODEC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid      state   bind_src                bind_dest\r\n");
		}
		if (bd_get_next(deviceid, j, bind_dest, sizeof(bind_dest)) == HD_OK) {
			if (bd_is_path_start(path_id)) {
				strcpy(state_name, "START");
			}
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x  %-5s   %-20s    %-20s\r\n",
				     VDODEC_INPORTID(j), VDODEC_OUTPORTID(j), path_id,
				     state_name, bind_src, bind_dest);
	}

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_VIDEODEC_PATH_CONFIG dec_config;
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_videodec_get(path_id, HD_VIDEODEC_PARAM_PATH_CONFIG, &dec_config) != HD_OK)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEODEC %-2d PATH POOL -----------------------------\r\n", VDODEC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid      pool    ddr_id  count   max_count\r\n");
		}
		for (k = 0; k < 4; k++) {
			if (dec_config.data_pool[k].mode == HD_VIDEODEC_POOL_ENABLE) {
				len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x  %-4d    %-4d    %-4.1f    %-4.1f\r\n",
						     VDODEC_INPORTID(j), VDODEC_OUTPORTID(j), path_id,
						     k, dec_config.data_pool[k].ddr_id,
						     (float)dec_config.data_pool[k].counts / 10,
						     (float)dec_config.data_pool[k].max_counts / 10);
			}
		}
	}

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_VIDEODEC_PATH_CONFIG dec_config;
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_videodec_get(path_id, HD_VIDEODEC_PARAM_PATH_CONFIG, &dec_config) != HD_OK)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEODEC %-2d CONFIG -----------------------------\r\n", VDODEC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid      max_width  max_height  frame_rate  codec\r\n");
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x  %-5lu      %-5lu       %-5u       %s\r\n",
				     VDODEC_INPORTID(j), VDODEC_OUTPORTID(j), path_id,
				     dec_config.max_mem.dim.w, dec_config.max_mem.dim.h,
				     dec_config.max_mem.frame_rate, codec_string[dec_config.max_mem.codec_type]);
	}

	print_dev_once = 1;
	for (j = HD_OUT_BASE; j < HD_OUT_MAX; j++) {
		HD_DAL self_id;
		HD_IO out_id;
		INT idx;
		VDODEC_INTL_PARAM *intl_param;
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		if (bd_check_exist(path_id) == 0)
			continue;
		self_id = HD_GET_DEV(path_id);
		out_id = HD_GET_OUT(path_id);
		idx = VDODEC_DEVID(self_id);
		intl_param = &dec_cfg[idx].port[VDODEC_OUTID(out_id)];
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEODEC %-2d VENDOR -----------------------------\r\n", VDODEC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid      sub_yuv_ratio  qp_value  first_out  extra_out\r\n");
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x  %-5u          %-5lu     %-5u\r\n",
				     VDODEC_INPORTID(j), VDODEC_OUTPORTID(j), path_id,
				     intl_param->sub_yuv_ratio, intl_param->qp_value, intl_param->first_out, intl_param->extra_out);
	}

	if (print_dev_once == 0)
		len += dif_dbg_print(where, p_buf + len, "\n\n");
	return len;
}



/* videodec_show_status_p(int where, char *p_buf)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int videodec_show_status_p(int where, char *p_buf, unsigned int buf_len)
{
	unsigned int i, len = 0;

	for (i = HD_DAL_VIDEODEC_BASE; i < HD_DAL_VIDEODEC_MAX; i++) {
		len += videodec_devid_show_status_p(where, p_buf + len, i);
		if (where && (len > (buf_len - HD_DEBUG_MSG_SIZE)))
			break;
	}
	return len;

}

static int hd_videodec_show_status_p(void)
{
	return (videodec_show_status_p(0, NULL, 0));
}

int hd_videodec_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len)
{
	return (videodec_show_status_p(1, p_buf, buf_len));
}

int hd_videodec_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid)
{
	return (videodec_devid_show_status_p(1, p_buf, deviceid));
}

static HD_DBG_MENU videodec_debug_menu[] = {
	{0x01, "dump status",                         hd_videodec_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_videodec_menu_p(void)
{
	return hd_debug_menu_entry_p(videodec_debug_menu, "VIDEODEC");
}

