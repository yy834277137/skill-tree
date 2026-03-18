/**
    @brief Source code of videoenc menu.\n
    This file contains the functions which is related to videoenc debug menu.

    @file hd_videoenc_menu.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_common.h"
#include "hd_type.h"
#include "hd_util.h"
#include "hd_videoenc.h"
#include "videoenc.h"
#include "hd_debug_menu.h"
#include <string.h>

#define VDOENC_DEVID(self_id)   		(self_id - HD_DAL_VIDEOENC_BASE)
#define VDOENC_INPORTID(self_id)   		(self_id - HD_IN_BASE)
#define VDOENC_OUTPORTID(self_id)   	(self_id - HD_OUT_BASE)

extern VDOENC_INFO videoenc_info[MAX_VDOENC_DEV];

extern HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len);
extern HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len);

extern UINT bd_is_path_start(HD_PATH_ID path_id);
extern int _hd_video_pxlfmt_str(HD_VIDEO_PXLFMT pxlfmt, CHAR *p_str, INT str_len);

#define BD_SET_PATH(dev_id, in_id, out_id)  (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))

extern int _hd_video_codec_str(HD_VIDEO_CODEC code_type, CHAR *p_str, INT str_len);
extern int _hd_video_svc_str(HD_SVC_LAYER_TYPE svc_layer, CHAR *p_str, INT str_len);
extern int _hd_video_profile_str(HD_VIDEOENC_PROFILE profile, CHAR *p_str, INT str_len);
extern int _hd_video_entropy_str(HD_VIDEOENC_CODING coding, CHAR *p_str, INT str_len);
extern int _hd_video_rc_mode(HD_VIDEOENC_RC_MODE rc_mode, CHAR *p_str, INT str_len);
INT dif_dbg_print(INT where, CHAR *buf, const CHAR *fmt, ...);

static char *pool_mode_str[4] = {"AUTO", "ENABLE", "DISABLE", ""};

/* videoenc_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int videoenc_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
{
	int j, stamp_idx, print_dev_once = 1, print_fd_line = 0, len = 0, port;
	char pxlfmt[16], codec_type[16], svc_layer[16], profile[16], entropy_mode[16], rc_mode[16];
	int ddr_id = 0, counts = 0, pool_mode = 3;
	VDOENC_INTERNAL_PARAM *p_param;

	for (j = HD_IN_BASE; j < HD_IN_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		char bind_src[32] = "-", bind_dest[32] = "-";
		char state_name[32] = "OPEN";

		if (bd_check_exist(path_id) == 0)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOENC %-2d PATH & BIND -----------------------------\r\n", VDOENC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     state   bind_src                bind_dest\r\n");
		}
		if (bd_get_prev(deviceid, j, bind_src, sizeof(bind_src)) == HD_OK) {
			if (bd_is_path_start(path_id)) {
				strcpy(state_name, "START");
			}
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-5s   %-20s    %-20s\r\n",
				     VDOENC_INPORTID(j), VDOENC_OUTPORTID(j), path_id, state_name, bind_src, bind_dest);
	}
	print_dev_once = 1;
	for (j = HD_IN_BASE; j < HD_IN_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		HD_VIDEOENC_IN video_in_param;
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_videoenc_get(path_id, HD_VIDEOENC_PARAM_IN, &video_in_param) != HD_OK)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOENC %-2d IN FRAME -----------------------------\r\n", VDOENC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     w       h       pxlfmt\r\n");
		}
		_hd_video_pxlfmt_str(video_in_param.pxl_fmt, pxlfmt, sizeof(pxlfmt));
		if ((video_in_param.dim.w != 0) && (video_in_param.dim.h != 0)) {
			len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4lu    %-4lu    %-20s\r\n",
					     VDOENC_INPORTID(j), VDOENC_INPORTID(j), path_id,
					     video_in_param.dim.w, video_in_param.dim.h, pxlfmt);
		}
	}


	print_dev_once = 1;
	for (j = HD_IN_BASE; j < HD_IN_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		HD_VIDEOENC_OUT enc_param;
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_videoenc_get(path_id, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &enc_param) != HD_OK)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOENC %-2d OUT FRAME -----------------------------\r\n", VDOENC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     codec   gop     svc     profile level    entropy\r\n");
		}

		_hd_video_codec_str(enc_param.codec_type, codec_type, sizeof(codec_type));
		_hd_video_svc_str(enc_param.h26x.svc_layer, svc_layer, sizeof(svc_layer));
		_hd_video_profile_str(enc_param.h26x.profile, profile, sizeof(profile));
		_hd_video_entropy_str(enc_param.h26x.entropy_mode, entropy_mode, sizeof(entropy_mode));

		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-5s %4d      %-5s  %4d    %-5s    %-5s\r\n",
				     VDOENC_INPORTID(j), VDOENC_INPORTID(j), path_id,
				     codec_type, enc_param.h26x.gop_num,  svc_layer, enc_param.h26x.level_idc, profile, entropy_mode);
	}


	print_dev_once = 1;
	for (j = HD_IN_BASE; j < HD_IN_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		HD_VIDEOENC_OUT enc_param;
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_videoenc_get(path_id, HD_VIDEOENC_PARAM_IN, &enc_param) != HD_OK)
			continue;

		p_param = videoenc_get_param_p(deviceid);
		port = j - 1;
		if (port < 0 || port >= HD_VIDEOENC_MAX_IN) {
			break;
		}
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOENC %-2d POOL -----------------------------\r\n", VDOENC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     ddr  count  mode\r\n");
		}
		if (p_param && p_param->port[port].p_enc_path_cfg) {
			ddr_id = p_param->port[port].p_enc_path_cfg->data_pool[0].ddr_id;
			counts = p_param->port[port].p_enc_path_cfg->data_pool[0].counts;
			pool_mode = p_param->port[port].p_enc_path_cfg->data_pool[0].mode;
			if (pool_mode < 0 || pool_mode >= 2)
				pool_mode = 3;
		}

		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %d    %2.1f    %s\r\n",
				     VDOENC_INPORTID(j), VDOENC_INPORTID(j), path_id,
				     ddr_id, ((float)counts) / 10, pool_mode_str[pool_mode]);
	}

	print_dev_once = 1;
	for (j = HD_IN_BASE; j < HD_IN_MAX; j++) {
		HD_PATH_ID path_id = BD_SET_PATH(deviceid, j, j);
		HD_H26XENC_RATE_CONTROL rc_param;
		if (bd_check_exist(path_id) == 0)
			continue;
		if (hd_videoenc_get(path_id, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &rc_param) != HD_OK)
			continue;
		if (print_dev_once) {
			print_dev_once = 0;
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOENC %-2d RC -----------------------------\r\n", VDOENC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     mode    base    incr    bitrate\r\n");
		}
		_hd_video_rc_mode(rc_param.rc_mode, rc_mode, sizeof(rc_mode));

		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-5s   %04d    %04d    %04dKbps\r\n",
				     VDOENC_INPORTID(j), VDOENC_INPORTID(j), path_id, rc_mode,
				     (int)rc_param.cbr.frame_rate_base, (int)rc_param.cbr.frame_rate_incr, (int)rc_param.cbr.bitrate / 1024);
	}

	if (VDOENC_DEVID(deviceid) < MAX_VDOENC_DEV) {
		print_dev_once = 1;
		for (j = 0; j < HD_VIDEOENC_MAX_IN; j++) {
			print_fd_line = 0;
			VDOENC_PORT *port = &videoenc_info[VDOENC_DEVID(deviceid)].port[j];
			for (stamp_idx= 0; stamp_idx < VDOENC_STAMP_NU; stamp_idx++) {
				HD_VDOENC_STAMP_PARAM *stamp = &port->stamp[stamp_idx];
				//if (bd_check_exist(stamp->path_id) == 0)
				//	continue;
				if (!stamp->enable)
					continue;
				if (print_dev_once) {
					print_dev_once = 0;
					len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOENC %-2d OSG -----------------------------\r\n", VDOENC_DEVID(deviceid));
					len += dif_dbg_print(where, p_buf + len, "fd         pathid     idx type     w    h    x    y    falpha\r\n");
				}
				print_fd_line = 1;

				len += dif_dbg_print(where, p_buf + len, "%#x %#x %-3d %-8s %-4d %-4d %-4d %-4d %-6d\r\n",
						     port->osg_fd, stamp->path_id, stamp_idx, get_osg_type_name(stamp->img.fmt),
						     stamp->img.dim.w, stamp->img.dim.h, stamp->attr.position.x, stamp->attr.position.y,
						     stamp->attr.alpha);
			}
			if (print_fd_line) {
				len += dif_dbg_print(where, p_buf + len, "-------------------------------------------------------------\r\n");
			}
		}
	}

	if (print_dev_once == 0)
		len += dif_dbg_print(where, p_buf + len, "\n\n");
	return len;
}

/* videoenc_show_status_p(int where, char *p_buf)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int videoenc_show_status_p(int where, char *p_buf, unsigned int buf_len)
{
	unsigned int i, len = 0;

	for (i = HD_DAL_VIDEOENC_BASE; i < HD_DAL_VIDEOENC_MAX; i++) {
		len += videoenc_devid_show_status_p(where, p_buf + len, i);
		if (where && (len > (buf_len - HD_DEBUG_MSG_SIZE)))
			break;
	}

	return len;
}

static int hd_videoenc_show_status_p(void)
{
	return (videoenc_show_status_p(0, NULL, 0));
}

int hd_videoenc_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len)
{
	return (videoenc_show_status_p(1, p_buf, buf_len));
}

int hd_videoenc_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid)
{
	return (videoenc_devid_show_status_p(1, p_buf, deviceid));
}

static HD_DBG_MENU videoenc_debug_menu[] = {
	{0x01, "dump status",                         hd_videoenc_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_videoenc_menu_p(void)
{
	return hd_debug_menu_entry_p(videoenc_debug_menu, "VIDEOENC");
}

