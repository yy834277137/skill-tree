/**
    @brief Source code of videoprocess menu.\n
    This file contains the functions which is related to videoprocess debug menu.

    @file hd_videoprocess_menu.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_common.h"
#include "hd_type.h"
#include "hd_util.h"
#include "hd_videoprocess.h"
#include "hd_debug_menu.h"
#include "videoprocess.h"
#include "utl.h"
#include <string.h>

#define VDOPROC_DEVID(self_id)   		(self_id - HD_DAL_VIDEOPROC_BASE)
#define VDOPROC_INPORTID(self_id)   	(self_id - HD_IN_BASE)
#define VDOPROC_OUTPORTID(self_id)   	(self_id - HD_OUT_BASE)

extern HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len);
extern HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len);

//extern HD_RESULT bd_check_exist(HD_PATH_ID path_id);
extern UINT bd_is_path_start(HD_PATH_ID path_id);
extern int _hd_video_pxlfmt_str(HD_VIDEO_PXLFMT pxlfmt, CHAR *p_str, INT str_len);
INT dif_dbg_print(INT where, CHAR *buf, const CHAR *fmt, ...);
extern VDOPROC_INTL_PARAM vpe_cfg[VDOPROC_DEV][VODPROC_OUTPUT_PORT];

#define BD_SET_PATH(dev_id, in_id, out_id)  (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))
//[15:0] ratio numerator (default 0)
#define GET_RATIO_NUMER_VALUE(ratio_prop)  (((ratio_prop) & 0xFFFF0000) >> 16)
//[31:16] sar height (default 0)
#define GET_RATIO_DENOM_VALUE(ratio_prop)  ((ratio_prop) & 0xFFFF)


//path_id=0 to search all
int check_to_print_vpemask(HD_DAL deviceid, HD_PATH_ID path_id)
{
	int i, k;
	HD_VIDEOPROC_VPEMASK_ONEINFO vpemask_info = {0};
	
	if (path_id) {
		for (k = 0; k < HD_VIDEOPROC_MASK_NUM; k++) {
			vpemask_info.index = 1;
			vpemask_info.mask_idx = k;
			if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_VPEMASK_ATTR, &vpemask_info) == HD_OK) {
				if (vpemask_info.index != 0)//enable
					return 1;
			}
		}
	} else {
		for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
			path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
			if (bd_check_exist(path_id) == 0)
				continue;
			for (k = 0; k < HD_VIDEOPROC_MASK_NUM; k++) {
				vpemask_info.index = 1;
				vpemask_info.mask_idx = k;
				if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_VPEMASK_ATTR, &vpemask_info) == HD_OK) {
					if (vpemask_info.index != 0) {//enable 
						return 1;
					}
				}
			}
		}
	}
	return 0;
}
	
	
//path_id=0 to search all
int check_to_print_wk_buf(HD_DAL deviceid, HD_PATH_ID path_id)
{
	int i;
	HD_VIDEOPROC_SCA_BUF_INFO sca_buf_info = {0};
	
	if (path_id) {
		if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_SCA_WK_BUF, &sca_buf_info) == HD_OK) {
			if (sca_buf_info.pbuf_addr)
				return 1;
		}
	} else {
		for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
			path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
			if (bd_check_exist(path_id) == 0)
				continue;
	
			if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_SCA_WK_BUF, &sca_buf_info) == HD_OK) {
				if (sca_buf_info.pbuf_addr)
					return 1;
			}
		}
	}
	return 0;
}

//path_id=0 to search all
int check_to_print_pattern_image(HD_DAL deviceid, HD_PATH_ID path_id)
{
	int i, k;
	HD_VIDEOPROC_PATTERN_IMG pattern_img = {0};
	
	if (path_id) {
		for (k = 1; k <= HD_VIDEOPROC_PATTERN_NUM; k++) {
			pattern_img.index = k;
			if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_PATTERN_IMG, &pattern_img) == HD_OK) {
				if (pattern_img.image.phy_addr[0])
						return 1;
			}
		}
	} else {
		for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
			path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
			if (bd_check_exist(path_id) == 0)
				continue;
			for (k = 1; k <= HD_VIDEOPROC_PATTERN_NUM; k++) {
				pattern_img.index = k;
				if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_PATTERN_IMG, &pattern_img) == HD_OK) {
					if (pattern_img.image.phy_addr[0])
							return 1;
				}
			}
		}
	}
	return 0;
}

//path_id=0 to search all
int check_to_print_pattern_select(HD_DAL deviceid, HD_PATH_ID path_id)
{
	int i;
	HD_VIDEOPROC_PATTERN_SELECT pat_sel = {0};

	if (path_id) {
		pat_sel.index = 1;
		if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_PATTERN_SELECT, &pat_sel) == HD_OK) {
			if (pat_sel.index)
				return 1;
		}		
	} else {
		for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
			path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
			if (bd_check_exist(path_id) == 0)
				continue;
			pat_sel.index = 1;
			if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_PATTERN_SELECT, &pat_sel) == HD_OK) {
				if (pat_sel.index) {
					return 1;
				}
			}
		}
	}
	return 0;
}

/* static int videoproc_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int videoproc_devid_show_status_p(int where, char *p_buf, HD_DAL deviceid)
{
	unsigned int fd;
	int i, k, len = 0, is_exist = 0, is_show;
	char pxlfmt[16] = {0};
	HD_VIDEOPROC_CTRL ctrl = {0};
	HD_VIDEOPROC_DEV_CONFIG vpe_config = {0};
	HD_VIDEOPROC_OUT videoproc_out = {0};
	HD_VIDEOPROC_CROP crop_attr = {0};
	HD_VIDEOPROC_IN videoproc_in = {0};
	HD_VIDEOPROC_PATTERN_IMG pattern_img = {0};
	HD_VIDEOPROC_PATTERN_SELECT pat_sel = {0};
	HD_VIDEOPROC_VPEMASK_ONEINFO vpemask_info = {0};
	HD_VIDEOPROC_SCA_BUF_INFO sca_buf_info = {0};
	HD_PATH_ID path_id;
	VDOPROC_INTL_PARAM *intl_param;
	char bind_src[32] = "-", bind_dest[32] = "-";
	char state_name[32] = "OPEN";

	/* check this device is exist */
	for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
		path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
		if (bd_check_exist(path_id) != 0) {
			is_exist = 1;
			break;
		}
	}
	if (is_exist == 0) {
		goto ext;
	}

	len += dif_dbg_print(where, p_buf + len, "========================= VIDEOPROC %-2d PATH & BIND ================================\r\n", VDOPROC_DEVID(deviceid));
	len += dif_dbg_print(where, p_buf + len, "in   out  pathid     fd         state   bind_src                bind_dest\r\n");

	for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
		path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
		if (bd_check_exist(path_id) == 0)
			continue;

		fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(deviceid), i, NULL);

		if (bd_get_next(deviceid, i + HD_OUT_BASE, bind_dest, sizeof(bind_dest)) == HD_OK) {
			if (bd_is_path_start(path_id)) {
				strcpy(state_name, "START");
			}
		}
		if (bd_get_prev(deviceid, HD_IN_BASE, bind_src, sizeof(bind_src)) == HD_OK) {
			if (bd_is_path_start(path_id)) {
				strcpy(state_name, "START");
			}
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %#x %-5s   %-20s    %-20s\r\n", 0, i, path_id, fd, state_name, bind_src, bind_dest);
	}

	is_show = 0;
	for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
		path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
		if (bd_check_exist(path_id) == 0)
			continue;

		if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_OUT, &videoproc_out) == HD_OK) {
		    if (is_show == 0) {
				len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d OUT FRAME -----------------------------\r\n", VDOPROC_DEVID(deviceid));
				len += dif_dbg_print(where, p_buf + len, "in   out  pathid     x       y       w       h       bg_w    bg_h    pxlfmt\r\n");
				is_show = 1;
			}
			_hd_video_pxlfmt_str(videoproc_out.pxlfmt, pxlfmt, sizeof(pxlfmt));
			len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d    %-4d    %-4d    %-4d    %-4d    %-4d    %-20s\r\n",
						 0, i, path_id, 
					     (int)videoproc_out.rect.x, (int)videoproc_out.rect.y, (int)videoproc_out.rect.w, (int)videoproc_out.rect.h,
					     (int)videoproc_out.bg.w, (int)videoproc_out.bg.h, pxlfmt);
		}
	}

	is_show = 0;
	for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
		path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
		if (bd_check_exist(path_id) == 0)
			continue;

		if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_IN_CROP, &crop_attr) == HD_OK) {
		    if (is_show == 0) {
				len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d IN CROP -----------------------------\r\n", VDOPROC_DEVID(deviceid));
				len += dif_dbg_print(where, p_buf + len, "in   out  pathid     x       y       w       h      mode\r\n");
				is_show = 1;
			}
			len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d    %-4d    %-4d    %-4d   %-4d\r\n",
						 0, i, path_id, 
					     (int)crop_attr.win.rect.x, (int)crop_attr.win.rect.y, (int)crop_attr.win.rect.w, (int)crop_attr.win.rect.h, (int)crop_attr.mode);
		}
	}

	is_show = 0;
	for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
		path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
		if (bd_check_exist(path_id) == 0)
			continue;

		if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_IN_CROP_PSR, &crop_attr) == HD_OK) {
		    if (is_show == 0) {
				len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d IN CROP PSR -----------------------------\r\n", VDOPROC_DEVID(deviceid));
				len += dif_dbg_print(where, p_buf + len, "in   out  pathid     x       y       w       h      mode\r\n");
				is_show = 1;
			}
			len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d    %-4d    %-4d    %-4d   %-4d\r\n",
						 0, i, path_id, 
					     (int)crop_attr.win.rect.x, (int)crop_attr.win.rect.y, (int)crop_attr.win.rect.w, (int)crop_attr.win.rect.h, (int)crop_attr.mode);
		}
	}

	is_show = 0;
	for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
		path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
		if (bd_check_exist(path_id) == 0)
			continue;

		if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_IN, &videoproc_in) == HD_OK) {
		    if (is_show == 0) {
				len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d IN -----------------------------\r\n", VDOPROC_DEVID(deviceid));
				len += dif_dbg_print(where, p_buf + len, "in   out  pathid     bg_w    bg_h    pixfmt\r\n");
				is_show = 1;
			}
			len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d    %-4d    %#x\r\n",
						 0, i, path_id, 
					     (int)videoproc_in.dim.w, (int)videoproc_in.dim.h, (int)videoproc_in.pxlfmt);
		}
	}

	is_show = 0;
	if (check_to_print_pattern_image(deviceid, 0)) {
		len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d PATTERN IMG -----------------------------\r\n", VDOPROC_DEVID(deviceid));
		for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
			path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
			if (bd_check_exist(path_id) == 0)
				continue;
			if (is_show == 0) { // filter first print
				is_show = 1;
			} else {
				len += dif_dbg_print(where, p_buf + len, "-------------------------\r\n");
			}
			if (check_to_print_pattern_image(deviceid, path_id) == 0)
				continue;
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     index   pixfmt      dim_w   dim_h    phy_addr\r\n");
			for (k = 1; k <= HD_VIDEOPROC_PATTERN_NUM; k++) {
				pattern_img.index = k;
				if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_PATTERN_IMG, &pattern_img) == HD_OK) {
					if (pattern_img.image.phy_addr[0] == 0)
						continue;
					len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d    %#08x  %-4d    %-4d     %#lx\r\n", 0, i, path_id,
						(int)pattern_img.index, (int)pattern_img.image.pxlfmt, (int)pattern_img.image.dim.w,
						(int)pattern_img.image.dim.h, pattern_img.image.phy_addr[0]);
				}
			}
		}
	}

	is_show = 0;
	if (check_to_print_pattern_select(deviceid, 0)) {
		len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d PATTERN SELECT -----------------------------\r\n", VDOPROC_DEVID(deviceid));
		for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
			path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
			if (bd_check_exist(path_id) == 0)
				continue;
			if (is_show == 0) { // filter first print
				is_show = 1;
			} else {
				len += dif_dbg_print(where, p_buf + len, "-------------------------\r\n");
			}
			if (check_to_print_pattern_select(deviceid, path_id) == 0)
				continue;
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     index x    y    w    h    color_sel x    y    w    h    img_crop_enable\r\n");
			pat_sel.index = 1;
			if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_PATTERN_SELECT, &pat_sel) == HD_OK) {
				len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d  %-4d %-4d %-4d %-4d %-4d      %-4d %-4d %-4d %-4d %-4d\r\n", 0, i, path_id,
					(int)pat_sel.index, (int)pat_sel.rect.x, (int)pat_sel.rect.y, (int)pat_sel.rect.w,
					(int)pat_sel.rect.h, (int)pat_sel.bg_color_sel, (int)pat_sel.img_crop.x, (int)pat_sel.img_crop.y,
					(int)pat_sel.img_crop.w, (int)pat_sel.img_crop.h, (int)pat_sel.img_crop_enable);
			}
		}
	}

	is_show = 0;	
	if (check_to_print_vpemask(deviceid, 0)) {
		len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d VPEMASK ATTR -----------------------------\r\n", VDOPROC_DEVID(deviceid));
		for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
			path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
			if (bd_check_exist(path_id) == 0)
				continue;
			if (is_show == 0) { // filter first print
				is_show = 1;
			} else {
				len += dif_dbg_print(where, p_buf + len, "-------------------------\r\n");
			}
			if (check_to_print_vpemask(deviceid, path_id) == 0)
				continue;
			len += dif_dbg_print(where, p_buf + len, "in(%d) out(%d) pathid(%#x):\r\n", 0, i, path_id);
			len += dif_dbg_print(where, p_buf + len, "  mask_idx enable(index) mask_area mosaic_en   0         1         2         3\r\n");
			for (k = 0; k < HD_VIDEOPROC_MASK_NUM; k++) {
				vpemask_info.mask_idx = k;
				if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_VPEMASK_ATTR, &vpemask_info) == HD_OK) {
					if (vpemask_info.index == 0) //disable
						continue;
					len += dif_dbg_print(where, p_buf + len, "  %-4d     %-4d          %-4d      %-4d        ",
								 (int)vpemask_info.mask_idx, (int)vpemask_info.index, (int)vpemask_info.mask_area, (int)vpemask_info.mosaic_en);
					len += dif_dbg_print(where, p_buf + len, "%-4dx%-4d %-4dx%-4d %-4dx%-4d %-4dx%-4d\r\n",
								 (int)vpemask_info.point[0].x, (int)vpemask_info.point[0].y, (int)vpemask_info.point[1].x, (int)vpemask_info.point[1].y,
								 (int)vpemask_info.point[2].x, (int)vpemask_info.point[2].y, (int)vpemask_info.point[3].x, (int)vpemask_info.point[3].y);
				}
			}
		}
	}

	is_show = 0;
	if (check_to_print_wk_buf(deviceid, 0)) {
		for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
			path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
			if (bd_check_exist(path_id) == 0)
				continue;
			if (check_to_print_wk_buf(deviceid, path_id) == 0)
				continue;
			if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_SCA_WK_BUF, &sca_buf_info) == HD_OK) {
				if (is_show == 0) {
					len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d SCA WK BUF -----------------------------\r\n", VDOPROC_DEVID(deviceid));
					len += dif_dbg_print(where, p_buf + len, "in   out  pathid     ddr     phy_addr           size\r\n");
					is_show = 1;
				}
				if (sca_buf_info.pbuf_addr == 0)
					continue;
				len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d    %#x         %-4d\r\n",0, i, path_id, 
						     (int)sca_buf_info.ddr_id, (int)sca_buf_info.pbuf_addr, (int)sca_buf_info.pbuf_size);
			}
		}
	}

	path_id = BD_SET_PATH(deviceid, HD_IN_BASE, HD_OUT_BASE);
	if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_CTRL, &ctrl) == HD_OK) {
		len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d CONTROL -----------------------------\r\n", VDOPROC_DEVID(deviceid));
		len += dif_dbg_print(where, p_buf + len, "in   out  pathid     de-interlace\r\n");
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-20s\r\n", 0, 0, path_id, (ctrl.func & HD_VIDEOPROC_FUNC_DI) ? "ON" : "OFF");
	}

	is_show = 0;
	for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
		path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
		if (bd_check_exist(path_id) == 0)
			continue;

		if (hd_videoproc_get(path_id, HD_VIDEOPROC_PARAM_DEV_CONFIG, &vpe_config) == HD_OK) {
		    if (is_show == 0) {
				len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d DEV POOL -----------------------------\r\n", VDOPROC_DEVID(deviceid));
				len += dif_dbg_print(where, p_buf + len, "in   out  pathid     pool    ddr_id  count   max_count\r\n");
				is_show = 1;
			}
			for (k = 0; k < 4; k++) {
				if (vpe_config.data_pool[k].mode == HD_VIDEOPROC_POOL_ENABLE) {
					len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %-4d    %-4d    %-4.1f    %-4.1f\r\n", 
								 0, i, path_id,
							     k, vpe_config.data_pool[k].ddr_id, (float)vpe_config.data_pool[k].counts / 10,
							     (float)vpe_config.data_pool[k].max_counts / 10);
				}
			}
		}
	}

	is_show = 0;
	for (i = 0; i < VODPROC_OUTPUT_PORT; i++) {
		path_id = BD_SET_PATH(deviceid, HD_IN_BASE, i + HD_OUT_BASE);
		if (bd_check_exist(path_id) == 0)
			continue;

		intl_param = &vpe_cfg[VDOPROC_DEVID(deviceid)][i];
	    if (is_show == 0) {
			len += dif_dbg_print(where, p_buf + len, "------------------------- VIDEOPROC %-2d VENDOR -----------------------------\r\n", VDOPROC_DEVID(deviceid));
			len += dif_dbg_print(where, p_buf + len, "in   out  pathid     sub_ratio_thld\r\n");
			is_show = 1;
		}
		len += dif_dbg_print(where, p_buf + len, "%-4d %-4d %#x %lu/%lu\r\n",
					 0, i, path_id,
					GET_RATIO_NUMER_VALUE(intl_param->sub_ratio_thld), GET_RATIO_DENOM_VALUE(intl_param->sub_ratio_thld));
	}

	len += dif_dbg_print(where, p_buf + len, "\n\n");
ext:
	return len;
}

/* videocap_show_status_p(int where, char *p_buf)
 * where
 *		0: print to console (p_buf = NULL)
 *		1: pirnt to buffer  (p_buf = by AP)
 */
static int videoproc_show_status_p(int where, char *p_buf, unsigned int buf_len)
{
	unsigned int i, len = 0;

	for (i = HD_DAL_VIDEOPROC_BASE; i < HD_DAL_VIDEOPROC_MAX; i++) {
		len += videoproc_devid_show_status_p(where, p_buf + len, i);
		if (where && (len > (buf_len - HD_DEBUG_MSG_SIZE)))
			break;
	}
	return len;
}

static int hd_videoproc_show_status_p(void)
{
	return (videoproc_show_status_p(0, NULL, 0));
}

int hd_videoproc_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len)
{
	return (videoproc_show_status_p(1, p_buf, buf_len));
}

int hd_videoproc_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid)
{
	return (videoproc_devid_show_status_p(1, p_buf, deviceid));
}

static HD_DBG_MENU videoproc_debug_menu[] = {
	{0x01, "dump status",                         hd_videoproc_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_videoproc_menu_p(void)
{
	return hd_debug_menu_entry_p(videoproc_debug_menu, "VIDEOPROC");
}

