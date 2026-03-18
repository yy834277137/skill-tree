/**
    @brief Source code of videoout menu.\n
    This file contains the functions which is related to videoout debug menu.

    @file hd_videoout_menu.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_common.h"
#include "hd_type.h"
#include "hd_util.h"
#include "hd_videoout.h"
#include "videoout.h"
#include "hd_debug_menu.h"
#include <string.h>

#define VDOOUT_DEVID(self_id)   		(self_id - HD_DAL_VIDEOOUT_BASE)
#define VDOOUT_INPORTID(self_id)   		(self_id - HD_IN_BASE)
#define VDOOUT_OUTPORTID(self_id)   	(self_id - HD_OUT_BASE)
#define BD_SET_CTRL(dev_id)			    (((dev_id) << 16) | HD_CTRL)

extern VDDO_PARAM g_video_out_param[VDDO_MAX];

extern HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len);
extern HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len);
extern UINT bd_check_exist(HD_PATH_ID path_id);
extern UINT bd_is_path_start(HD_PATH_ID path_id);
extern int _hd_video_pxlfmt_str(HD_VIDEO_PXLFMT pxlfmt, CHAR *p_str, INT str_len);
INT dif_dbg_print(INT where, CHAR *buf, const CHAR *fmt, ...);

#define BD_SET_PATH(dev_id, in_id, out_id)  (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

#define BD_SET_CTRL(dev_id)			        (((dev_id) << 16) | HD_CTRL)
#define __to_str(x)  #x
/* static int videoout_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int videoout_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
{
	int j, stamp_idx, print_dev_once = 1, len = 0;
	HD_FB_FMT fb_fmt;
	HD_PATH_ID path_id = BD_SET_PATH(deviceid, HD_IN_BASE, HD_OUT_BASE);
	HD_VIDEOOUT_SYSCAPS lcd_syscaps;

	if (bd_check_exist(path_id) == 0)
		goto ext;
	path_id = HD_VIDEOOUT_CTRL(deviceid - HD_DAL_VIDEOOUT_BASE);
	if (hd_videoout_get(path_id, HD_VIDEOOUT_PARAM_SYSCAPS, &lcd_syscaps) != HD_OK)
		goto ext;
	if (print_dev_once) {
		print_dev_once = 0;
		fb_fmt.fb_id = HD_FB0;
		if (hd_videoout_get(path_id, HD_VIDEOOUT_PARAM_FB_FMT, &fb_fmt) != HD_OK) {
			printf("hd_videoout_get:param_id(%d) videoout_ctrl_path(%#lx) fail\n", HD_VIDEOOUT_PARAM_FB_FMT, path_id);
			goto ext;
		}
		len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOOUT %-2d SYSCAP -----------------------------\r\n", VDOOUT_DEVID(deviceid));
		len += dif_dbg_print(where, p_buf + len, "in_w    in_h    out_w   out_h   fps     scal_up scal_down  lcd_fmt\r\n");
	}
	len += dif_dbg_print(where, p_buf + len, "%-4d    %-4d    %-4d    %-4d    %-4d    %d, %d     %d, %d      %s\r\n",
			     (int)lcd_syscaps.input_dim.w, (int)lcd_syscaps.input_dim.h, (int)lcd_syscaps.output_dim.w, (int)lcd_syscaps.output_dim.h,
			     (int)lcd_syscaps.fps, (int)lcd_syscaps.max_scale_up_w, (int)lcd_syscaps.max_scale_up_h, (int)lcd_syscaps.max_scale_down_w,
			     (int)lcd_syscaps.max_scale_down_h, (lcd_syscaps.out_caps[0] == HD_VIDEO_CAPS_YUV422) ? __to_str(HD_VIDEO_CAPS_YUV420) : \
			     __to_str(HD_VIDEO_PXLFMT_YUV420));

	print_dev_once = 1;
	for (j = HD_IN_BASE; j < HD_IN_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, HD_OUT_BASE);
		char bind_src[32] = "-", bind_dest[32] = "-";
		char state_name[32] = "OPEN";

		if (bd_check_exist(path_id) == 0)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOOUT %-2d PATH & BIND -----------------------------\r\n", VDOOUT_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     state   bind_src                bind_dest\r\n");
		}

		if (bd_get_prev(deviceid, j, bind_src, sizeof(bind_src)) == HD_OK) {
			if (bd_is_path_start(path_id)) {
				strcpy(state_name, "START");
			}
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#-8x %-5s   %-20s    %-20s\r\n", \
				     VDOOUT_INPORTID(j), VDOOUT_DEVID(deviceid), path_id, state_name, bind_src, bind_dest);
	}

	print_dev_once = 1;
	for (j = HD_IN_BASE; j < HD_IN_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, HD_OUT_BASE);
		HD_VIDEOOUT_WIN_ATTR win;
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_videoout_get(path_id, HD_VIDEOOUT_PARAM_IN_WIN_ATTR, &win) != HD_OK)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOOUT %-2d WIN -----------------------------\r\n", VDOOUT_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid        x       y       w       h       visible layer\r\n");
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#-8x    %-4d    %-4d    %-4d    %-4d    %-4d    %-4d\r\n",
				     VDOOUT_INPORTID(j), VDOOUT_DEVID(deviceid), path_id, (int)win.rect.x, (int)win.rect.y, (int)win.rect.w, (int)win.rect.h, (int)win.visible, (int)win.layer);
	}

	print_dev_once = 1;
	for (j = HD_IN_BASE; j < HD_IN_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, HD_OUT_BASE);
		HD_VIDEOOUT_WIN_PSR_ATTR win_psr;
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_videoout_get(path_id, HD_VIDEOOUT_PARAM_IN_WIN_PSR_ATTR, &win_psr) != HD_OK)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOOUT %-2d WIN PSR ---------------------------\r\n", VDOOUT_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid        x       y       w       h       visible\r\n");
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#-8x    %-4d    %-4d    %-4d    %-4d    %-4d   \r\n",
				     VDOOUT_INPORTID(j), VDOOUT_DEVID(deviceid), path_id, (int)win_psr.rect.x, (int)win_psr.rect.y, (int)win_psr.rect.w, (int)win_psr.rect.h, (int)win_psr.visible);
	}

	if (VDOOUT_DEVID(deviceid) < VDDO_MAX) {
		print_dev_once = 1;
		VDDO_PARAM *vdout_param = &g_video_out_param[VDOOUT_DEVID(deviceid)];
		for (stamp_idx= 0; stamp_idx < HD_MAXVO_TOTAL_STAMP_NU; stamp_idx++) {
			HD_VDDO_WIN_PARAM *stamp = &vdout_param->stamp[stamp_idx];
			if (!stamp->win_enable)
				continue;
			if (print_dev_once) {
				print_dev_once = 0;
				len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOOUT %-2d OSG -----------------------------\r\n", VDOOUT_DEVID(deviceid));
				len += dif_dbg_print(where, p_buf + len, "fd         pathid     idx type     w    h    x    y    falpha\r\n");
			}

			len += dif_dbg_print(where, p_buf + len, "%#x %#x %-3d %-8s %-4d %-4d %-4d %-4d %-6d\r\n",
						 vdout_param->osg_fd, stamp->path_id, stamp_idx, videoout_get_osg_type_name(stamp->img.fmt),
						 stamp->img.dim.w, stamp->img.dim.h, stamp->attr.position.x, stamp->attr.position.y,
						 stamp->attr.alpha);
		}
	}

	if (print_dev_once == 0)
		len += dif_dbg_print(where, p_buf + len, "\n\n");

ext:
	return len;
}

/* videoout_show_status_p(int where, char *p_buf)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int videoout_show_status_p(int where, char *p_buf, unsigned int buf_len)
{
	unsigned int i, len = 0;

	for (i = HD_DAL_VIDEOOUT_BASE; i < HD_DAL_VIDEOOUT_MAX; i++) {
		len += videoout_devid_show_status_p(where, p_buf + len, i);
		if (where && (len > (buf_len - HD_DEBUG_MSG_SIZE)))
			break;
	}
	return len;
}

static int hd_videoout_show_status_p(void)
{
	return (videoout_show_status_p(0, NULL, 0));
}

int hd_videoout_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len)
{
	return (videoout_show_status_p(1, p_buf, buf_len));
}

int hd_videoout_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid)
{
	return (videoout_devid_show_status_p(1, p_buf, deviceid));
}

static HD_DBG_MENU videoout_debug_menu[] = {
	{0x01, "dump status",                         hd_videoout_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_videoout_menu_p(void)
{
	return hd_debug_menu_entry_p(videoout_debug_menu, "VIDEOOUT");
}

