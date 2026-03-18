/**
    @brief Source file of video process.\n
    This file contains the functions which is related to video process in the chip.

    @file videoprocess.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "hdal.h"
#include "vpd_ioctl.h"
#include "trig_ioctl.h"
#include "hdal.h"
#include "videoprocess.h"
#include "cif_common.h"
#include "dif.h"
#include "di_ioctl.h"
#include "vpe_ioctl.h"
#include "vg_common.h"
#include "pif_ioctl.h"
#include "pif.h"
#include "utl.h"

#define ALIGN8_UP(x)        ((((x) + 7) >> 3) << 3)
#define PAGE_SHIFT          12
#define PAGE_SIZE           (1 << PAGE_SHIFT)
#define VIDEOPROC_DEVICE 	"/dev/kflow_vpe"
#define VPEMASK_MOSAIC_BLK  8

extern int vpd_fd;
extern vpd_sys_info_t platform_sys_Info;
extern UINT32 videoproc_max_device_nr;
extern UINT32 videoproc_max_device_in;
extern UINT32 videoproc_max_device_out;
extern VDOPROC_PARAM *vdoproc_param[VDOPROC_DEV][VODPROC_OUTPUT_PORT];
VDOPROC_INTL_PARAM vpe_cfg[VDOPROC_DEV][VODPROC_OUTPUT_PORT];
extern int vpe_ioctl;
extern int pif_alloc_user_fd_for_hdal(vpd_em_type_t type, int chip, int engine, int minor);
extern void *pif_alloc_video_buffer_for_hdal(int size, int ddr_id, char pool_name[], int alloc_tag);
extern int pif_free_video_buffer_for_hdal(void *pa, int ddr_id, int alloc_tag);
//extern unsigned int pif_create_queue_for_hdal(void);
extern uintptr_t pif_create_queue_for_hdal(void);
//extern int pif_destroy_queue_for_hdal(unsigned int queue_handle);
extern int pif_destroy_queue_for_hdal(uintptr_t queue_handle);
extern UINT32 utl_get_vpe_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param);
INT convert_hdal_to_vpd_flip_rotate(HD_VIDEO_DIR direction, vpd_rotate_t *rotate, vpd_flip_t *flip)
{
	if ((direction & ~HD_VIDEO_DIR_ROTATE_MASK) == HD_VIDEO_DIR_MIRRORX) {
		*flip = VPD_FLIP_HORIZONTAL;
	} else if ((direction & ~HD_VIDEO_DIR_ROTATE_MASK) == HD_VIDEO_DIR_MIRRORY) {
		*flip = VPD_FLIP_VERTICAL;
	} else if ((direction & ~HD_VIDEO_DIR_ROTATE_MASK) == HD_VIDEO_DIR_MIRRORXY) {
		*flip = VPD_FLIP_VERTICAL_AND_HORIZONTAL;
	} else {
		*flip = VPD_FLIP_NONE;
	}
	if ((direction & HD_VIDEO_DIR_ROTATE_MASK) ==  HD_VIDEO_DIR_ROTATE_90) {
		*rotate = VPD_ROTATE_RIGHT;
	} else if ((direction & HD_VIDEO_DIR_ROTATE_MASK) == HD_VIDEO_DIR_ROTATE_180) {
		*flip = VPD_FLIP_VERTICAL_AND_HORIZONTAL;
		*rotate = VPD_ROTATE_NONE;
	} else if ((direction & HD_VIDEO_DIR_ROTATE_MASK) == HD_VIDEO_DIR_ROTATE_270) {
		*rotate = VPD_ROTATE_LEFT;
	} else {
		*rotate = VPD_ROTATE_NONE;
	}

	if (*rotate == VPD_ROTATE_NONE && *flip == VPD_FLIP_NONE) {
		return 0;
	} else {
		return 1;
	}
}

INT convert_hdal_to_vpd_di_dn_sharp(BOOL di_enable, BOOL dn_enable, BOOL sharp_enable,
				    vpd_di_mode_t *di_mode, vpd_dn_mode_t *dn_mode, vpd_sharpness_mode_t *sharp_mode)
{
	if (di_enable) {
		*di_mode = VPD_DI_TEMPORAL_SPATIAL;
	} else {
		*di_mode = VPD_DI_OFF;
	}
	if (di_enable) {
		*dn_mode = VPD_DN_TEMPORAL_SPATIAL;
	} else {
		*dn_mode = VPD_DN_OFF;
	}
	if (sharp_enable) {
		*sharp_mode = VPD_SHARPNESS_ON;
	} else {
		*sharp_mode = VPD_SHARPNESS_OFF;
	}

	if (*di_mode == VPD_DI_OFF && *dn_mode == VPD_DN_OFF && *sharp_mode == VPD_SHARPNESS_OFF) {
		return 0;
	} else {
		return 1;
	}
}

BOOL videoproc_check_addr(UINTPTR address)
{
	BOOL m_valid = FALSE;
	unsigned char vector[2];
	int ret2;
	UINTPTR start = 0;
	//check
	if (address) {
		start = (address / PAGE_SIZE) * PAGE_SIZE;
		if (start) {
			ret2 = mincore((void *)start, 0x8, &vector[0]);
			if (ret2 == 0) { //0 means ok
				m_valid = TRUE;
			} else {
				int errsv = errno;
				HD_VIDEOPROC_ERR("INVALID! errsv(%#x), addr(%p)\n", errsv, (VOID *)address);
				m_valid = FALSE;
			}
		} else {
			//HD_VIDEOPROC_ERR("INvalid!start=0x%x\n", (UINT)start);
			m_valid = FALSE;
		}
	}
	return m_valid;
}


HD_RESULT videoproc_set_vproc_align_down_dim(HD_PATH_ID path_id, HD_VIDEO_PXLFMT in_fmt, HD_VIDEO_PXLFMT out_fmt, HD_URECT in_rect, HD_URECT *p_out_rect)
{
	VDOPROC_PARAM *vproc_param;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT idx = VDOPROC_DEVID(self_id);
	VDOPROC_HW_SPEC_INFO hw_spec_info = {0};

	if (p_out_rect == NULL) {
		HD_VIDEOPROC_ERR("set_vproc_align_down_dim, p_out_rect NULL\n");
		goto err_exit;
	}

	if ((in_fmt < HD_VIDEO_PXLFMT_I1_ICON) || (in_fmt > HD_VIDEO_PXLFMT_NRX16_SHDR4)) {
		HD_VIDEOPROC_ERR("path_id(%#x) set wrong input pxlfmt(%#x), please check HD_VIDEOPROC_PARAM_IN.\n", path_id, in_fmt);
		goto err_exit;
	}
	if ((out_fmt < HD_VIDEO_PXLFMT_I1_ICON) || (out_fmt > HD_VIDEO_PXLFMT_NRX16_SHDR4)) {
		HD_VIDEOPROC_ERR("path_id(%#x) set wrong output pxlfmt(%#x), please check HD_VIDEOPROC_PARAM_IN.\n", path_id, out_fmt);
		goto err_exit;
	}

	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if (vproc_param == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		goto err_exit;
	}

	if ((vproc_param->direction != HD_VIDEO_DIR_NONE) &&
	    (vproc_param->direction != HD_VIDEO_DIR_MIRRORX) &&
	    (vproc_param->direction != HD_VIDEO_DIR_MIRRORY) &&
	    (vproc_param->direction != HD_VIDEO_DIR_MIRRORXY) &&
	    (vproc_param->direction != HD_VIDEO_DIR_ROTATE_0) &&
	    (vproc_param->direction != HD_VIDEO_DIR_ROTATE_90) &&
	    (vproc_param->direction != HD_VIDEO_DIR_ROTATE_180) &&
	    (vproc_param->direction != HD_VIDEO_DIR_ROTATE_270)) {
		HD_VIDEOPROC_ERR("path_id(%#x) set wrong output direction(%#x), please check HD_VIDEOPROC_PARAM_OUT.\n", path_id, vproc_param->direction);
		goto err_exit;
	}

	hw_spec_info.in_direction = vproc_param->direction;
	hw_spec_info.in_src_fmt = in_fmt;
	hw_spec_info.in_dst_fmt = out_fmt;
	if (videoproc_get_vpe_align_p(&hw_spec_info, 1) != HD_OK) {
		HD_VIDEOPROC_ERR("path_id(%#x) get hardware spec info fail.\n", path_id);
		goto err_exit;
	}
	p_out_rect->x = ALIGN_FLOOR(in_rect.x, hw_spec_info.out_dst_rect.x);
	p_out_rect->y = ALIGN_FLOOR(in_rect.y, hw_spec_info.out_dst_rect.y);
	p_out_rect->w = ALIGN_FLOOR(in_rect.w, hw_spec_info.out_dst_rect.w);
	p_out_rect->h = ALIGN_FLOOR(in_rect.h, hw_spec_info.out_dst_rect.h);
	return HD_OK;
err_exit:
	return HD_ERR_SYS;
}

HD_RESULT videoproc_check_is_needed_param_set_p(HD_PATH_ID path_id, HD_VIDEO_PXLFMT in_fmt, HD_VIDEO_PXLFMT out_fmt, HD_DIM *p_src_bg)
{
	VDOPROC_PARAM *vproc_param;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT idx = VDOPROC_DEVID(self_id);
	VDOPROC_HW_SPEC_INFO hw_spec_info = {0};

	if ((in_fmt < HD_VIDEO_PXLFMT_I1_ICON) || (in_fmt > HD_VIDEO_PXLFMT_NRX16_SHDR4)) {
		HD_VIDEOPROC_ERR("path_id(%#x) set wrong input pxlfmt(%#x), please check HD_VIDEOPROC_PARAM_IN.\n", path_id, in_fmt);
		goto err_exit;
	}
	if ((out_fmt < HD_VIDEO_PXLFMT_I1_ICON) || (out_fmt > HD_VIDEO_PXLFMT_NRX16_SHDR4)) {
		HD_VIDEOPROC_ERR("path_id(%#x) set wrong output pxlfmt(%#x), please check HD_VIDEOPROC_PARAM_IN.\n", path_id, out_fmt);
		goto err_exit;
	}

	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if (vproc_param == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		goto err_exit;
	}

	if ((vproc_param->direction != HD_VIDEO_DIR_NONE) &&
	    (vproc_param->direction != HD_VIDEO_DIR_MIRRORX) &&
	    (vproc_param->direction != HD_VIDEO_DIR_MIRRORY) &&
	    (vproc_param->direction != HD_VIDEO_DIR_MIRRORXY) &&
	    (vproc_param->direction != HD_VIDEO_DIR_ROTATE_0) &&
	    (vproc_param->direction != HD_VIDEO_DIR_ROTATE_90) &&
	    (vproc_param->direction != HD_VIDEO_DIR_ROTATE_180) &&
	    (vproc_param->direction != HD_VIDEO_DIR_ROTATE_270)) {
		HD_VIDEOPROC_ERR("path_id(%#x) set wrong output direction(%#x), please check HD_VIDEOPROC_PARAM_OUT.\n", path_id, vproc_param->direction);
		goto err_exit;
	}

	hw_spec_info.in_direction = vproc_param->direction;
	hw_spec_info.in_src_fmt = in_fmt;
	hw_spec_info.in_dst_fmt = out_fmt;
	if (videoproc_get_vpe_align_p(&hw_spec_info, 1) != HD_OK) {
		HD_VIDEOPROC_ERR("path_id(%#x) get hardware spec info fail.\n", path_id);
		goto err_exit;
	}

	/* p_src_bg == NULL --> trigger mode
	 * p_src_bg != NULL --> flow mode    */
	if (p_src_bg == NULL) {
		/* check in dimension */
		if ((vproc_param->src_bg_dim.w < hw_spec_info.out_src_min_dim.w) ||
		    (vproc_param->src_bg_dim.w > hw_spec_info.out_src_max_dim.w)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.coord.w(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_bg_dim.w, hw_spec_info.out_src_min_dim.w, hw_spec_info.out_src_max_dim.w);
			goto err_exit;
		}
		if ((vproc_param->src_bg_dim.h < hw_spec_info.out_src_min_dim.h) ||
		    (vproc_param->src_bg_dim.h > hw_spec_info.out_src_max_dim.h)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.coord.h(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_bg_dim.h, hw_spec_info.out_src_min_dim.h, hw_spec_info.out_src_max_dim.h);
			goto err_exit;
		}

		/* check alignment */
		if (IS_ALIGNX(hw_spec_info.out_src_bg_dim.w, vproc_param->src_bg_dim.w) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.coord.w(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_bg_dim.w, hw_spec_info.out_src_bg_dim.w);
			goto err_exit;
		}
		if (IS_ALIGNX(hw_spec_info.out_src_bg_dim.h, vproc_param->src_bg_dim.h) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.coord.h(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_bg_dim.h, hw_spec_info.out_src_bg_dim.h);
			goto err_exit;
		}
	} else {
		/* check in dimension */
		if ((p_src_bg->w < hw_spec_info.out_src_min_dim.w) ||
		    (p_src_bg->w > hw_spec_info.out_src_max_dim.w)) {
			HD_VIDEOPROC_ERR("path_id(%#x) input dim.w(%u) is needed between %u ~ %u, please check previous unit output setting.\n",
					 path_id, vproc_param->src_bg_dim.w, hw_spec_info.out_src_min_dim.w, hw_spec_info.out_src_max_dim.w);
			goto err_exit;
		}
		if ((p_src_bg->h < hw_spec_info.out_src_min_dim.h) ||
		    (p_src_bg->h > hw_spec_info.out_src_max_dim.h)) {
			HD_VIDEOPROC_ERR("path_id(%#x) input dim.h(%u) is needed between %u ~ %u, please check previous unit output setting.\n",
					 path_id, p_src_bg->h, hw_spec_info.out_src_min_dim.h, hw_spec_info.out_src_max_dim.h);
			goto err_exit;
		}

		/* check alignment */
		if (IS_ALIGNX(hw_spec_info.out_src_bg_dim.w, p_src_bg->w) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) input dim.w(%u) is needed align to %u, please check previous unit output setting.\n",
					 path_id, p_src_bg->w, hw_spec_info.out_src_bg_dim.w);
			goto err_exit;
		}
		if (IS_ALIGNX(hw_spec_info.out_src_bg_dim.h, p_src_bg->h) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) input dim.h(%u) is needed align to %u, please check previous unit output setting.\n",
					 path_id, p_src_bg->h, hw_spec_info.out_src_bg_dim.h);
			goto err_exit;
		}
	}

	/* check out dimension */
	if ((vproc_param->dst_bg_dim.w < hw_spec_info.out_dst_min_dim.w) ||
	    (vproc_param->dst_bg_dim.w > hw_spec_info.out_dst_max_dim.w)) {
		HD_VIDEOPROC_ERR("path_id(%#x) dim.w(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_OUT.\n",
				 path_id, vproc_param->dst_bg_dim.w, hw_spec_info.out_dst_min_dim.w, hw_spec_info.out_dst_max_dim.w);
		goto err_exit;
	}
	if ((vproc_param->dst_bg_dim.h < hw_spec_info.out_dst_min_dim.h) ||
	    (vproc_param->dst_bg_dim.h > hw_spec_info.out_dst_max_dim.h)) {
		HD_VIDEOPROC_ERR("path_id(%#x) dim.h(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_OUT.\n",
				 path_id, vproc_param->dst_bg_dim.h, hw_spec_info.out_dst_min_dim.h, hw_spec_info.out_dst_max_dim.h);
		goto err_exit;
	}

	/* check alignment */
	if (IS_ALIGNX(hw_spec_info.out_dst_bg_dim.w, vproc_param->dst_bg_dim.w) == 0) {
		HD_VIDEOPROC_ERR("path_id(%#x) dim.w(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_OUT.\n",
				 path_id, vproc_param->dst_bg_dim.w, hw_spec_info.out_dst_bg_dim.w);
		goto err_exit;
	}
	if (IS_ALIGNX(hw_spec_info.out_dst_bg_dim.h, vproc_param->dst_bg_dim.h) == 0) {
		HD_VIDEOPROC_ERR("path_id(%#x) dim.h(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_OUT.\n",
				 path_id, vproc_param->dst_bg_dim.h, hw_spec_info.out_dst_bg_dim.h);
		goto err_exit;
	}

	/* check in crop */
	if (vproc_param->src_rect_enable == TRUE) {
		if ((vproc_param->src_bg_dim.w < hw_spec_info.out_src_min_dim.w) ||
		    (vproc_param->src_bg_dim.w > hw_spec_info.out_src_max_dim.w)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.coord.w(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_bg_dim.w, hw_spec_info.out_src_min_dim.w, hw_spec_info.out_src_max_dim.w);
			goto err_exit;
		}
		if ((vproc_param->src_bg_dim.h < hw_spec_info.out_src_min_dim.h) ||
		    (vproc_param->src_bg_dim.h > hw_spec_info.out_src_max_dim.h)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.coord.h(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_bg_dim.h, hw_spec_info.out_src_min_dim.h, hw_spec_info.out_src_max_dim.h);
			goto err_exit;
		}
		if ((vproc_param->src_rect.w < hw_spec_info.out_src_min_dim.w) ||
		    (vproc_param->src_rect.w > vproc_param->src_bg_dim.w)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.w(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_rect.w, hw_spec_info.out_src_min_dim.w, vproc_param->src_bg_dim.w);
			goto err_exit;
		}
		if ((vproc_param->src_rect.h < hw_spec_info.out_src_min_dim.h) ||
		    (vproc_param->src_rect.h > vproc_param->src_bg_dim.h)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.h(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_rect.h, hw_spec_info.out_src_min_dim.h, vproc_param->src_bg_dim.h);
			goto err_exit;
		}

		/* check alignment */
		if (IS_ALIGNX(hw_spec_info.out_src_rect.x, vproc_param->src_rect.x) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.x(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_rect.x, hw_spec_info.out_src_rect.x);
			goto err_exit;
		}
		if (IS_ALIGNX(hw_spec_info.out_src_rect.y, vproc_param->src_rect.y) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.y(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_rect.y, hw_spec_info.out_src_rect.y);
			goto err_exit;
		}
		if (IS_ALIGNX(hw_spec_info.out_src_rect.w, vproc_param->src_rect.w) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.w(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_rect.w, hw_spec_info.out_src_rect.w);
			goto err_exit;
		}
		if (IS_ALIGNX(hw_spec_info.out_src_rect.h, vproc_param->src_rect.h) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.h(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_rect.h, hw_spec_info.out_src_rect.h);
			goto err_exit;
		}
	}

	/* check in crop psr */
	if (vproc_param->psr_crop_enable == TRUE) {
		if ((vproc_param->psr_crop_bg_dim.w < hw_spec_info.out_src_min_dim.w) ||
		    (vproc_param->psr_crop_bg_dim.w > hw_spec_info.out_src_max_dim.w)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.coord.w(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vproc_param->psr_crop_bg_dim.w, hw_spec_info.out_src_min_dim.w, hw_spec_info.out_src_max_dim.w);
			goto err_exit;
		}
		if ((vproc_param->psr_crop_bg_dim.h < hw_spec_info.out_src_min_dim.h) ||
		    (vproc_param->psr_crop_bg_dim.h > hw_spec_info.out_src_max_dim.h)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.coord.h(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vproc_param->psr_crop_bg_dim.h, hw_spec_info.out_src_min_dim.h, hw_spec_info.out_src_max_dim.h);
			goto err_exit;
		}
		if ((vproc_param->psr_crop_rect.w < hw_spec_info.out_src_min_dim.w) ||
		    (vproc_param->psr_crop_rect.w > vproc_param->psr_crop_bg_dim.w)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.w(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vproc_param->psr_crop_rect.w, hw_spec_info.out_src_min_dim.w, vproc_param->psr_crop_bg_dim.w);
			goto err_exit;
		}
		if ((vproc_param->psr_crop_rect.h < hw_spec_info.out_src_min_dim.h) ||
		    (vproc_param->psr_crop_rect.h > vproc_param->psr_crop_bg_dim.h)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.h(%u) is needed between %u ~ %u, please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vproc_param->psr_crop_rect.h, hw_spec_info.out_src_min_dim.h, vproc_param->psr_crop_bg_dim.h);
			goto err_exit;
		}

		/* check alignment */
		if (IS_ALIGNX(hw_spec_info.out_dst_hole.x, vproc_param->psr_crop_rect.x) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.x(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vproc_param->psr_crop_rect.x, hw_spec_info.out_dst_hole.x);
			goto err_exit;
		}
		if (IS_ALIGNX(hw_spec_info.out_dst_hole.y, vproc_param->psr_crop_rect.y) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.y(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vproc_param->psr_crop_rect.y, hw_spec_info.out_dst_hole.y);
			goto err_exit;
		}
		if (IS_ALIGNX(hw_spec_info.out_dst_hole.w, vproc_param->psr_crop_rect.w) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.w(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vproc_param->psr_crop_rect.w, hw_spec_info.out_dst_hole.w);
			goto err_exit;
		}
		if (IS_ALIGNX(hw_spec_info.out_dst_hole.h, vproc_param->psr_crop_rect.h) == 0) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.h(%u) is needed align to %u, please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vproc_param->psr_crop_rect.h, hw_spec_info.out_dst_hole.h);
			goto err_exit;
		}
	}
	/* check in crop width when tmnr enable */
	if ((vproc_param->tmnr_en) && ((vproc_param->src_fmt == HD_VIDEO_PXLFMT_YUV420_NVX3) || (vproc_param->src_fmt == HD_VIDEO_PXLFMT_YUV422_NVX3))) {
		if ((vproc_param->src_rect.w > 0) && (IS_ALIGNX(32, vproc_param->src_rect.w) == 0)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.w(%u) need 32 align with NVX3 pxlfmt when TMNR enable, please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vproc_param->src_rect.w);
			goto err_exit;
		}
		if ((vproc_param->psr_crop_rect.w > 0) && (IS_ALIGNX(32, vproc_param->psr_crop_rect.w) == 0)) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.w(%u) need 32 align with NVX3 pxlfmt when TMNR enable, please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vproc_param->psr_crop_rect.w);
			goto err_exit;
		}
	}

	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT videoproc_get_vpe_align_p(VDOPROC_HW_SPEC_INFO *p_hw_spec_info, INT is_vpe)
{
	HD_RESULT hd_ret = HD_OK;
	INT ioctl_ret;
	vpe_hw_info_t vdoproc_vpe_hw_spec_info = {0};
	di_hw_info_t vdoproc_di_hw_spec_info = {0};

	if (!vpe_ioctl) {
		vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
	}

	if (is_vpe) {
		if ((p_hw_spec_info->in_direction == HD_VIDEO_DIR_ROTATE_90) ||
		    (p_hw_spec_info->in_direction == HD_VIDEO_DIR_ROTATE_270)) {
			vdoproc_vpe_hw_spec_info.feature = VPE_FEATURE_ROT;
		} else {
			vdoproc_vpe_hw_spec_info.feature = VPE_FEATURE_NONE;
		}
		vdoproc_vpe_hw_spec_info.src_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_hw_spec_info->in_src_fmt);
		vdoproc_vpe_hw_spec_info.dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_hw_spec_info->in_dst_fmt);

		ioctl_ret = ioctl(vpe_ioctl, VPE_IOC_GET_HW_SPEC_INFO, &vdoproc_vpe_hw_spec_info);
		if (ioctl_ret < 0) {
			HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_GET_HW_SPEC_INFO\" return %d\n", ioctl_ret);
			hd_ret = HD_ERR_SYS;
		}

		p_hw_spec_info->out_src_fmt_is_valid = vdoproc_vpe_hw_spec_info.vaild_src_fmt;
		p_hw_spec_info->out_dst_fmt_is_valid = vdoproc_vpe_hw_spec_info.vaild_dst_fmt;

		p_hw_spec_info->out_src_rect.x = vdoproc_vpe_hw_spec_info.src.x;
		p_hw_spec_info->out_src_rect.y = vdoproc_vpe_hw_spec_info.src.y;
		p_hw_spec_info->out_src_rect.w = vdoproc_vpe_hw_spec_info.src.w;
		p_hw_spec_info->out_src_rect.h = vdoproc_vpe_hw_spec_info.src.h;
		p_hw_spec_info->out_src_min_dim.w = vdoproc_vpe_hw_spec_info.src.min_w;
		p_hw_spec_info->out_src_min_dim.h = vdoproc_vpe_hw_spec_info.src.min_h;
		p_hw_spec_info->out_src_max_dim.w = vdoproc_vpe_hw_spec_info.src.max_w;
		p_hw_spec_info->out_src_max_dim.h = vdoproc_vpe_hw_spec_info.src.max_h;
		p_hw_spec_info->out_src_bg_dim.w = vdoproc_vpe_hw_spec_info.src.bg_w;
		p_hw_spec_info->out_src_bg_dim.h = vdoproc_vpe_hw_spec_info.src.bg_h;

		p_hw_spec_info->out_dst_rect.x = vdoproc_vpe_hw_spec_info.dst.x;
		p_hw_spec_info->out_dst_rect.y = vdoproc_vpe_hw_spec_info.dst.y;
		p_hw_spec_info->out_dst_rect.w = vdoproc_vpe_hw_spec_info.dst.w;
		p_hw_spec_info->out_dst_rect.h = vdoproc_vpe_hw_spec_info.dst.h;
		p_hw_spec_info->out_dst_hole.x = vdoproc_vpe_hw_spec_info.dst.hole_x;
		p_hw_spec_info->out_dst_hole.y = vdoproc_vpe_hw_spec_info.dst.hole_y;
		p_hw_spec_info->out_dst_hole.w = vdoproc_vpe_hw_spec_info.dst.hole_w;
		p_hw_spec_info->out_dst_hole.h = vdoproc_vpe_hw_spec_info.dst.hole_h;
		p_hw_spec_info->out_dst_min_dim.w = vdoproc_vpe_hw_spec_info.dst.min_w;
		p_hw_spec_info->out_dst_min_dim.h = vdoproc_vpe_hw_spec_info.dst.min_h;
		p_hw_spec_info->out_dst_max_dim.w = vdoproc_vpe_hw_spec_info.dst.max_w;
		p_hw_spec_info->out_dst_max_dim.h = vdoproc_vpe_hw_spec_info.dst.max_h;
		p_hw_spec_info->out_dst_bg_dim.w = vdoproc_vpe_hw_spec_info.dst.bg_w;
		p_hw_spec_info->out_dst_bg_dim.h = vdoproc_vpe_hw_spec_info.dst.bg_h;

		HD_VIDEOPROC_MSG("VPE feature(%u) src fmt(%#x) rect(%u,%u,%u,%u) min(%u,%u) max(%u,%u) bg(%u,%u), "
				 "dst fmt(%#x) rect(%u,%u,%u,%u) hole(%u,%u,%u,%u) min(%u,%u) max(%u,%u) bg(%u,%u)",
				 vdoproc_vpe_hw_spec_info.feature, p_hw_spec_info->in_src_fmt,
				 p_hw_spec_info->out_src_rect.x, p_hw_spec_info->out_src_rect.y,
				 p_hw_spec_info->out_src_rect.w, p_hw_spec_info->out_src_rect.h,
				 p_hw_spec_info->out_src_min_dim.w, p_hw_spec_info->out_src_min_dim.h,
				 p_hw_spec_info->out_src_max_dim.w, p_hw_spec_info->out_src_max_dim.h,
				 p_hw_spec_info->out_src_bg_dim.w, p_hw_spec_info->out_src_bg_dim.h,
				 p_hw_spec_info->in_dst_fmt,
				 p_hw_spec_info->out_dst_rect.x, p_hw_spec_info->out_dst_rect.y,
				 p_hw_spec_info->out_dst_rect.w, p_hw_spec_info->out_dst_rect.h,
				 p_hw_spec_info->out_dst_hole.x, p_hw_spec_info->out_dst_hole.y,
				 p_hw_spec_info->out_dst_hole.w, p_hw_spec_info->out_dst_hole.h,
				 p_hw_spec_info->out_dst_min_dim.w, p_hw_spec_info->out_dst_min_dim.h,
				 p_hw_spec_info->out_dst_max_dim.w, p_hw_spec_info->out_dst_max_dim.h,
				 p_hw_spec_info->out_dst_bg_dim.w, p_hw_spec_info->out_dst_bg_dim.h);
	} else {
		if (p_hw_spec_info->is_interlace) {
			vdoproc_di_hw_spec_info.feature = DI_FEATURE_INTER;
		} else {
			vdoproc_di_hw_spec_info.feature = DI_FEATURE_PRO;
		}
		vdoproc_di_hw_spec_info.src_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_hw_spec_info->in_src_fmt);
		vdoproc_di_hw_spec_info.dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_hw_spec_info->in_dst_fmt);

		ioctl_ret = ioctl(vpe_ioctl, DI_IOC_GET_HW_SPEC_INFO, &vdoproc_di_hw_spec_info);
		if (ioctl_ret < 0) {
			HD_VIDEOPROC_ERR("ioctl \"DI_IOC_GET_HW_SPEC_INFO\" return %d\n", ioctl_ret);
			hd_ret = HD_ERR_SYS;
		}

		p_hw_spec_info->out_src_fmt_is_valid = vdoproc_di_hw_spec_info.vaild_src_fmt;
		p_hw_spec_info->out_dst_fmt_is_valid = vdoproc_di_hw_spec_info.vaild_dst_fmt;

		p_hw_spec_info->out_src_rect.x = vdoproc_di_hw_spec_info.src.x;
		p_hw_spec_info->out_src_rect.y = vdoproc_di_hw_spec_info.src.y;
		p_hw_spec_info->out_src_rect.w = vdoproc_di_hw_spec_info.src.w;
		p_hw_spec_info->out_src_rect.h = vdoproc_di_hw_spec_info.src.h;
		p_hw_spec_info->out_src_min_dim.w = vdoproc_di_hw_spec_info.src.min_w;
		p_hw_spec_info->out_src_min_dim.h = vdoproc_di_hw_spec_info.src.min_h;
		p_hw_spec_info->out_src_max_dim.w = vdoproc_di_hw_spec_info.src.max_w;
		p_hw_spec_info->out_src_max_dim.h = vdoproc_di_hw_spec_info.src.max_h;
		p_hw_spec_info->out_src_bg_dim.w = vdoproc_di_hw_spec_info.src.bg_w;
		p_hw_spec_info->out_src_bg_dim.h = vdoproc_di_hw_spec_info.src.bg_h;

		p_hw_spec_info->out_dst_rect.x = vdoproc_di_hw_spec_info.dst.x;
		p_hw_spec_info->out_dst_rect.y = vdoproc_di_hw_spec_info.dst.y;
		p_hw_spec_info->out_dst_rect.w = vdoproc_di_hw_spec_info.dst.w;
		p_hw_spec_info->out_dst_rect.h = vdoproc_di_hw_spec_info.dst.h;
		p_hw_spec_info->out_dst_min_dim.w = vdoproc_di_hw_spec_info.dst.min_w;
		p_hw_spec_info->out_dst_min_dim.h = vdoproc_di_hw_spec_info.dst.min_h;
		p_hw_spec_info->out_dst_max_dim.w = vdoproc_di_hw_spec_info.dst.max_w;
		p_hw_spec_info->out_dst_max_dim.h = vdoproc_di_hw_spec_info.dst.max_h;
		p_hw_spec_info->out_dst_bg_dim.w = vdoproc_di_hw_spec_info.dst.bg_w;
		p_hw_spec_info->out_dst_bg_dim.h = vdoproc_di_hw_spec_info.dst.bg_h;

		HD_VIDEOPROC_MSG("DI feature(%u) src fmt(%#x) rect(%u,%u,%u,%u) min(%u,%u) max(%u,%u) bg(%u,%u), "
				 "dst fmt(%#x) rect(%u,%u,%u,%u) min(%u,%u) max(%u,%u) bg(%u,%u)",
				 vdoproc_vpe_hw_spec_info.feature, p_hw_spec_info->in_src_fmt,
				 p_hw_spec_info->out_src_rect.x, p_hw_spec_info->out_src_rect.y,
				 p_hw_spec_info->out_src_rect.w, p_hw_spec_info->out_src_rect.h,
				 p_hw_spec_info->out_src_min_dim.w, p_hw_spec_info->out_src_min_dim.h,
				 p_hw_spec_info->out_src_max_dim.w, p_hw_spec_info->out_src_max_dim.h,
				 p_hw_spec_info->out_src_bg_dim.w, p_hw_spec_info->out_src_bg_dim.h,
				 p_hw_spec_info->in_dst_fmt,
				 p_hw_spec_info->out_dst_rect.x, p_hw_spec_info->out_dst_rect.y,
				 p_hw_spec_info->out_dst_rect.w, p_hw_spec_info->out_dst_rect.h,
				 p_hw_spec_info->out_dst_min_dim.w, p_hw_spec_info->out_dst_min_dim.h,
				 p_hw_spec_info->out_dst_max_dim.w, p_hw_spec_info->out_dst_max_dim.h,
				 p_hw_spec_info->out_dst_bg_dim.w, p_hw_spec_info->out_dst_bg_dim.h);
	}

	if (p_hw_spec_info->out_src_fmt_is_valid) {
		if ((p_hw_spec_info->out_src_rect.x == 0) ||
		    (p_hw_spec_info->out_src_rect.y == 0) ||
		    (p_hw_spec_info->out_src_rect.w == 0) ||
		    (p_hw_spec_info->out_src_rect.h == 0) ||
		    (p_hw_spec_info->out_src_min_dim.w == 0) ||
		    (p_hw_spec_info->out_src_min_dim.h == 0) ||
		    (p_hw_spec_info->out_src_max_dim.w == 0) ||
		    (p_hw_spec_info->out_src_max_dim.h == 0) ||
		    (p_hw_spec_info->out_src_bg_dim.w == 0) ||
		    (p_hw_spec_info->out_src_bg_dim.h == 0)) {
			hd_ret = HD_ERR_SYS;
		}
	}

	if (p_hw_spec_info->out_dst_fmt_is_valid) {
		if ((p_hw_spec_info->out_dst_rect.x == 0) ||
		    (p_hw_spec_info->out_dst_rect.y == 0) ||
		    (p_hw_spec_info->out_dst_rect.w == 0) ||
		    (p_hw_spec_info->out_dst_rect.h == 0) ||
		    (p_hw_spec_info->out_dst_min_dim.w == 0) ||
		    (p_hw_spec_info->out_dst_min_dim.h == 0) ||
		    (p_hw_spec_info->out_dst_max_dim.w == 0) ||
		    (p_hw_spec_info->out_dst_max_dim.h == 0) ||
		    (p_hw_spec_info->out_dst_bg_dim.w == 0) ||
		    (p_hw_spec_info->out_dst_bg_dim.h == 0)) {
			hd_ret = HD_ERR_SYS;
		}
	}

	return hd_ret;
}

HD_RESULT videoproc_new_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT buf_size, ddr_id;
	CHAR pool_name[20] = {};
	HD_COMMON_MEM_VB_BLK blk = 0;

	buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_video_frame->dim, p_video_frame->pxlfmt);
	if (buf_size == 0) {
		HD_VIDEOPROC_ERR("get buf_size(%d) failed.\n", buf_size);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	ddr_id = p_video_frame->ddr_id;
	blk = (HD_COMMON_MEM_VB_BLK)pif_alloc_video_buffer_for_hdal(buf_size, ddr_id, pool_name, USR_LIB);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		HD_VIDEOPROC_ERR("allocate in_buf failed.\n");
		hd_ret = HD_ERR_NOMEM;
		goto exit;
	}
	p_video_frame->phy_addr[0] = hd_common_mem_blk2pa(blk);

exit:
	return hd_ret;
}

HD_RESULT videoproc_push_in_buf_p(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame,
				  HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	CHAR pool_name[20] = {};
	HD_COMMON_MEM_VB_BLK blk = 0;
	INT ddr_id, i;
	INT errsv, cnt = 0, ret = 0;
	HD_DIM dim;
	usr_proc_trigger_vpe_t usr_vpe[VPD_VPETRIG_MAXNUM] = {0};
	UINT32 out_w[VPD_VPETRIG_MAXNUM] = {0}, out_h[VPD_VPETRIG_MAXNUM] = {0};
	UINTPTR addr_all_pa[VPD_VPETRIG_MAXNUM] = {0};
	UINT estimate_size = 0, query_size = 0;
	HD_VIDEO_PXLFMT dst_fmt = HD_VIDEO_PXLFMT_NONE;
	VDOPROC_PARAM *vproc_param;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT idx = VDOPROC_DEVID(self_id);
	UINTPTR out_blk = 0;

	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if (vproc_param == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	if (p_video_frame->ddr_id > DDR_ID_MAX) {
		HD_VIDEOPROC_ERR("ddr_id=%d exceeds max %d\n", p_video_frame->ddr_id, DDR_ID_MAX);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	dst_fmt = vproc_param->dst_fmt;
	dim.w = vproc_param->dst_bg_dim.w;
	dim.h = vproc_param->dst_bg_dim.h;
	estimate_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(dim, dst_fmt);
	if (estimate_size == 0) {
		HD_VIDEOPROC_ERR("Estimate buffer size is 0. path_id(%#x) wh(%u,%u) fmt(%#x)\n",
				 path_id, dim.w, dim.h, dst_fmt);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	cnt = 0;
	if (p_user_out_video_frame) {
		out_blk = p_user_out_video_frame->blk;
		while (p_user_out_video_frame) {
			if (cnt >= VPD_VPETRIG_MAXNUM) {
				HD_VIDEOPROC_ERR("path_id(%#x) exceeds max %d.\n", path_id, VPD_VPETRIG_MAXNUM);
				p_user_out_video_frame->p_next = NULL;
				break;
			}
			ret = pif_query_video_buffer_size(p_user_out_video_frame->phy_addr[0],
							  p_user_out_video_frame->ddr_id,
							  pool_name, &query_size);
			if (ret == -2) {
				/* can not find pool by this pa, skip it */
				hd_ret = HD_OK;
			} else if (ret < 0) {
				HD_VIDEOPROC_ERR("query_video_buffer_size fail\n");
				hd_ret = HD_ERR_NOBUF;
				goto exit;
			} else {
				if (query_size < estimate_size) {
					HD_VIDEOPROC_ERR("path_id(%#x) p_user_out_video_frame too small, buf size(%u) < set size(%u) by wh(%u,%u) pxlfmt(%#x)\n",
							 path_id, query_size, estimate_size, dim.w, dim.h, dst_fmt);
					hd_ret = HD_ERR_NOBUF;
					goto exit;
				}
			}

			ddr_id = p_user_out_video_frame->ddr_id;
			out_w[cnt] = p_user_out_video_frame->dim.w;
			out_h[cnt] = p_user_out_video_frame->dim.h;
			addr_all_pa[cnt] = p_user_out_video_frame->phy_addr[0];
			cnt++;

			if ((p_user_out_video_frame->p_next)
			    && (p_user_out_video_frame->ddr_id < DDR_ID_MAX)
			    && (videoproc_check_addr((UINTPTR)p_user_out_video_frame->p_next) == TRUE)) {
				p_user_out_video_frame = (HD_VIDEO_FRAME *)(p_user_out_video_frame->p_next);
			} else {
				break;
			}
		}
	} else {
		ddr_id = DDR_ID0;
		blk = (HD_COMMON_MEM_VB_BLK)pif_alloc_video_buffer_for_hdal(estimate_size, ddr_id, pool_name, USR_LIB);
		if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
			HD_VIDEOPROC_ERR("path_id(%#x) allocate out_buf failed. size(%d) ddr(%d) pool(%s)\n",
					 path_id, estimate_size, ddr_id, pool_name);
			hd_ret = HD_ERR_NOMEM;
			goto exit;
		}
		addr_all_pa[cnt] = hd_common_mem_blk2pa(blk);
		cnt = 1;
	}

	memset(usr_vpe, 0x0, sizeof(usr_proc_trigger_vpe_t) * VPD_VPETRIG_MAXNUM);
	for (i = 0; i < cnt; i++) {
		if ((vproc_param->di_enable) && (vproc_param->priv.di_fd)) {
			usr_vpe[i].fd = vproc_param->priv.di_fd;
		} else {
			if (vproc_param->priv.fd) {
				usr_vpe[i].fd = vproc_param->priv.fd;
			} else {
				break;
			}
		}
		usr_vpe[i].in_frame_buffer.ddr_id = p_video_frame->ddr_id;
		usr_vpe[i].in_frame_buffer.addr_pa = p_video_frame->phy_addr[0];
		if (pif_address_ddr_sanity_check(usr_vpe[i].in_frame_buffer.addr_pa, usr_vpe[i].in_frame_buffer.ddr_id) < 0) {
			HD_VIDEOPROC_ERR("%s:Check in_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_vpe[i].in_frame_buffer.addr_pa, usr_vpe[i].in_frame_buffer.ddr_id);
			ret = HD_ERR_PARAM;
			goto exit;
		}
		usr_vpe[i].in_frame_buffer.size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(
				p_video_frame->dim, p_video_frame->pxlfmt);
		sprintf(usr_vpe[i].in_frame_buffer.pool_name, pool_name);

		usr_vpe[i].in_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(
							p_video_frame->pxlfmt);
		if (vproc_param->src_bg_dim.w && vproc_param->src_bg_dim.h) {
			usr_vpe[i].in_frame_info.dim.w = vproc_param->src_bg_dim.w;
			usr_vpe[i].in_frame_info.dim.h = vproc_param->src_bg_dim.h;
		} else {
			usr_vpe[i].in_frame_info.dim.w = vproc_param->dst_bg_dim.w;
			usr_vpe[i].in_frame_info.dim.h = vproc_param->dst_bg_dim.h;
		}
		usr_vpe[i].in_frame_info.pathid = p_video_frame->reserved[0];
		usr_vpe[i].out_frame_buffer.ddr_id = ddr_id;
		usr_vpe[i].out_frame_buffer.addr_pa = addr_all_pa[i];
		if (pif_address_ddr_sanity_check(usr_vpe[i].out_frame_buffer.addr_pa, usr_vpe[i].out_frame_buffer.ddr_id) < 0) {
			HD_VIDEOPROC_ERR("%s:Check out_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_vpe[i].out_frame_buffer.addr_pa, usr_vpe[i].out_frame_buffer.ddr_id);
			ret = HD_ERR_PARAM;
			goto exit;
		}
		usr_vpe[i].out_frame_buffer.size = estimate_size;
		usr_vpe[i].out_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(dst_fmt);
		usr_vpe[i].out_frame_info.dim.w = vproc_param->dst_bg_dim.w;
		usr_vpe[i].out_frame_info.dim.h = vproc_param->dst_bg_dim.h;

		usr_vpe[i].scale.src_rect.x = vproc_param->src_rect.x;
		usr_vpe[i].scale.src_rect.y = vproc_param->src_rect.y;
		usr_vpe[i].scale.src_rect.w = vproc_param->src_rect.w;
		usr_vpe[i].scale.src_rect.h = vproc_param->src_rect.h;

		usr_vpe[i].scale.dst_rect.x = vproc_param->dst_rect.x;
		usr_vpe[i].scale.dst_rect.y = vproc_param->dst_rect.y;

		if ((vproc_param->dst_rect.w == 0) || (vproc_param->dst_rect.h == 0)) {
			usr_vpe[i].scale.dst_rect.w = out_w[i];
			usr_vpe[i].scale.dst_rect.h = out_h[i];
		} else {
			usr_vpe[i].scale.dst_rect.w = vproc_param->dst_rect.w;
			usr_vpe[i].scale.dst_rect.h = vproc_param->dst_rect.h;
		}

		if (vproc_param->slice_mode.enable) {
			usr_vpe[i].slice_height = vproc_param->slice_mode.slice_height;
		} else {
			usr_vpe[i].slice_height = VPLIB_NULL_VALUE;
		}

		usr_vpe[i].vpe_num = cnt;
		usr_vpe[i].transform.enable = convert_hdal_to_vpd_flip_rotate(vproc_param->direction,
					      &usr_vpe[i].transform.rotate_mode,
					      &usr_vpe[i].transform.flip_mode);
		usr_vpe[i].quality.enable = convert_hdal_to_vpd_di_dn_sharp(vproc_param->di_enable,
					    vproc_param->dn_enable,
					    vproc_param->sharp_enable,
					    &usr_vpe[i].quality.di_mode,
					    &usr_vpe[i].quality.dn_mode,
					    &usr_vpe[i].quality.shrp_mode);

		usr_vpe[i].queue_handle = vproc_param->priv.queue_handle;
		usr_vpe[i].p_user_data = (void *)out_blk;
		usr_vpe[i].uv_swap = vproc_param->uv_swap;
		usr_vpe[i].side_offset = common_calculate_dec_side_info_offset(&vproc_param->src_bg_dim, HD_CODEC_TYPE_H265);
		usr_vpe[i].side_size = common_calculate_dec_side_info_size(&vproc_param->src_bg_dim, HD_CODEC_TYPE_H265);
	}

	ret = ioctl(vpd_fd, USR_PROC_TRIGGER_VPE, usr_vpe);
	if (ret < 0) {
		errsv = errno * -1;
		if (errsv == -2) {
			HD_VIDEOPROC_ERR("path(%#x) in_frame_buffer(phy_addr[0]=%p) is still in use."
					 "Please 'pull' this buffer before push it.\n",
					 path_id, (VOID *)p_video_frame->phy_addr[0]);
		}

		HD_VIDEOPROC_ERR("<ioctl \"USR_PROC_TRIGGER_VPE\" fail, error(%d)>\n", errsv);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}

exit:
	return hd_ret;
}

HD_RESULT videoproc_release_in_buf_p(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT ddr_id, ret;
	VOID *addr_pa;

	addr_pa = (VOID *) p_video_frame->phy_addr[0];
	ddr_id = p_video_frame->ddr_id;

	ret = pif_free_video_buffer_for_hdal(addr_pa, ddr_id, USR_LIB);
	if (ret < 0) {
		HD_VIDEOPROC_ERR("free buffer(pa:%p) failed.\n", addr_pa);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}

HD_RESULT videoproc_pull_out_buf_p(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_vpe_cb_info_t cb_info = {0};
	INT errsv;
	VDOPROC_PARAM *vproc_param;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT idx = VDOPROC_DEVID(self_id);

	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if (vproc_param == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	if (vproc_param->priv.queue_handle == 0) {
		HD_VIDEOPROC_ERR("output queue is not ready. self_id(%#x) out_id(%#x)\n", self_id, out_id);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	hd_ret = dif_pull_out_buffer(self_id, out_id, p_video_frame, wait_ms);
	if (hd_ret == HD_OK) {
		if (p_video_frame->phy_addr[0] == 0) {
			HD_VIDEOPROC_ERR("dif_pull_out_buffer fail\n");
			hd_ret = HD_ERR_NG;
			goto exit;
		}
	} else if (hd_ret == HD_ERR_NOT_AVAIL) {
		cb_info.queue_handle = vproc_param->priv.queue_handle;
		cb_info.wait_ms = wait_ms;
		if (ioctl(vpd_fd, USR_PROC_RECV_DATA_VPE, &cb_info) < 0) {
			errsv = errno * -1;
			HD_VIDEOPROC_ERR("<ioctl \"USR_PROC_RECV_DATA_VPE\" fail, error(%d)>\n", errsv);
			if (errsv == -15) {
				hd_ret = HD_ERR_TIMEDOUT;
			} else {
				hd_ret = HD_ERR_SYS;
			}
			goto exit;
		}
		if (cb_info.status == 1)  { //1:job finished, 0:ongoing, -1:fail
			hd_ret = HD_OK;
		} else {
			hd_ret = HD_ERR_FAIL;
		}
		if (p_video_frame) {
			p_video_frame->ddr_id = cb_info.ddr_id;
			p_video_frame->phy_addr[0] = cb_info.addr_pa;
			if (pif_address_ddr_sanity_check(p_video_frame->phy_addr[0], p_video_frame->ddr_id) < 0) {
				HD_VIDEOPROC_ERR("%s:Check  pull_out pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)p_video_frame->phy_addr[0], p_video_frame->ddr_id);
				hd_ret = HD_ERR_PARAM;
				goto exit;
			}
			p_video_frame->dim.w = vproc_param->dst_bg_dim.w;
			p_video_frame->dim.h = vproc_param->dst_bg_dim.h;
			p_video_frame->pxlfmt = vproc_param->dst_fmt;
			p_video_frame->blk = (typeof(p_video_frame->blk)) cb_info.p_user_data;
		}
	} else {
		HD_VIDEOPROC_ERR("pull out buffer error, ret(%#x)\n", hd_ret);
	}

exit:
	return hd_ret;
}

HD_RESULT videoproc_release_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT ddr_id, ret;
	VOID *addr_pa;

	hd_ret = dif_release_out_buffer(self_id, out_id, p_video_frame);
	if (hd_ret == HD_OK) {
		p_video_frame->phy_addr[0] = 0;
	} else if (hd_ret == HD_ERR_NOT_AVAIL) {
		addr_pa = (VOID *) p_video_frame->phy_addr[0];
		ddr_id = p_video_frame->ddr_id;

		ret = pif_free_video_buffer_for_hdal(addr_pa, ddr_id, USR_USR);
		if (ret < 0) {
			HD_VIDEOPROC_ERR("release buffer phy_addr(%p) failed.\n", addr_pa);
			hd_ret = HD_ERR_NG;
		}
	} else {
		HD_VIDEOPROC_ERR("release out buffer failed. ret(%#x)\n", hd_ret);
	}

	return hd_ret;
}

VDOPROC_INTL_PARAM *videoproc_get_param_p(HD_DAL self_id, UINT32 out_subid)
{
	UINT32 idx;
	idx = VDOPROC_DEVID(self_id);

	if (out_subid >= videoproc_max_device_out) {
		HD_VIDEOPROC_ERR("Check output port error,devid(%#x) out_subid(%d) max(%d)\n", self_id, out_subid, videoproc_max_device_out);
		return NULL;
	}
	return &vpe_cfg[idx][out_subid];
}

HD_RESULT videoproc_set_param_p(HD_DAL self_id, UINT32 out_subid, HD_VIDEOPROC_PARAM_ID id)
{
	UINT32 idx;

	idx = VDOPROC_DEVID(self_id);

	if (out_subid >= videoproc_max_device_out) {
		HD_VIDEOPROC_ERR("Check output port error,devid(%#x) out_subid(%d) max(%d)\n", self_id, out_subid, videoproc_max_device_out);
		return HD_ERR_NG;
	}
	if (vdoproc_param[idx][out_subid] == NULL) {
		HD_VIDEOPROC_ERR("self_id(%#x) out_subid(%d) err, it maybe no open.\n", self_id, out_subid);
		return HD_ERR_NG;
	}
	vpe_cfg[idx][out_subid].use_vpe_func = vdoproc_param[idx][out_subid]->use_vpe_func;

	switch (id) {
	case HD_VIDEOPROC_PARAM_SYSINFO:
		break;
	case HD_VIDEOPROC_PARAM_IN_CROP:
		vpe_cfg[idx][out_subid].p_src_rect_enable = &vdoproc_param[idx][out_subid]->src_rect_enable;
		vpe_cfg[idx][out_subid].p_src_rect = &vdoproc_param[idx][out_subid]->src_rect;
		vpe_cfg[idx][out_subid].p_src_bg_dim = &vdoproc_param[idx][out_subid]->src_bg_dim;
		break;
	case HD_VIDEOPROC_PARAM_IN_CROP_PSR:
		vpe_cfg[idx][out_subid].p_psr_crop_enable = &vdoproc_param[idx][out_subid]->psr_crop_enable;
		vpe_cfg[idx][out_subid].p_psr_crop_rect = &vdoproc_param[idx][out_subid]->psr_crop_rect;
		vpe_cfg[idx][out_subid].p_psr_crop_bg_dim = &vdoproc_param[idx][out_subid]->psr_crop_bg_dim;
		break;
	case HD_VIDEOPROC_PARAM_CTRL:
		vpe_cfg[idx][out_subid].p_di_enable = &vdoproc_param[idx][out_subid]->di_enable;
		vpe_cfg[idx][out_subid].p_dn_enable = &vdoproc_param[idx][out_subid]->dn_enable;
		vpe_cfg[idx][out_subid].p_sharp_enable = &vdoproc_param[idx][out_subid]->sharp_enable;
		break;
	case HD_VIDEOPROC_PARAM_OUT:
		vpe_cfg[idx][out_subid].p_dst_rect = &vdoproc_param[idx][out_subid]->dst_rect;
		vpe_cfg[idx][out_subid].p_dst_bg_dim = &vdoproc_param[idx][out_subid]->dst_bg_dim;
		vpe_cfg[idx][out_subid].p_dst_fmt = &vdoproc_param[idx][out_subid]->dst_fmt;
		vpe_cfg[idx][out_subid].p_direction = &vdoproc_param[idx][out_subid]->direction;
		break;
	case HD_VIDEOPROC_PARAM_IN:
		vpe_cfg[idx][out_subid].p_src_bg_dim = &vdoproc_param[idx][out_subid]->src_bg_dim;
		vpe_cfg[idx][out_subid].p_src_fmt = &vdoproc_param[idx][out_subid]->src_fmt;
		break;
	case HD_VIDEOPROC_PARAM_DEV_CONFIG:
		vpe_cfg[idx][out_subid].p_out_pool = &vdoproc_param[idx][out_subid]->out_pool;
		break;
	default:
		break;
	}

	return HD_OK;
}

HD_RESULT videoproc_set_crop_p(HD_DAL self_id,  HD_IO out_id, INT param_id)
{
	HD_RESULT hd_ret = HD_OK;
	hd_ret = bd_apply_attr(self_id, out_id, param_id);
	return hd_ret;
}

HD_RESULT videoproc_set_pattern_p(HD_DAL self_id,  HD_IO in_id, HD_VIDEOPROC_PATTERN_IMG *p_pattern)
{
	HD_RESULT hd_ret = HD_OK;
	pat_img_info_t pattern_info;
	int ret = 0;

	if (!vpe_ioctl) {
		vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
	}
	if (!p_pattern) {
		HD_VIDEOPROC_ERR("Check p_pattern fail\n");
		return HD_ERR_ABORT;
	}
	pattern_info.index = p_pattern->index;
	pattern_info.buf_addr = p_pattern->image.phy_addr[0];
	pattern_info.width = p_pattern->image.dim.w;
	pattern_info.height = p_pattern->image.dim.h;
	//pattern_info.type = p_pattern->image.pxlfmt;
	pattern_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_pattern->image.pxlfmt);//TYPE_YUV420_SCE;
	//HD_VIDEOPROC_ERR(" pattype=0x%x\n", pattern_info.type );
	pattern_info.ddr_id = p_pattern->image.ddr_id;
	pattern_info.buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_pattern->image.dim, p_pattern->image.pxlfmt);
	if ((ret = ioctl(vpe_ioctl, VPE_IOC_SET_PATTERN_IMAGE, &pattern_info)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_SET_PATTERN_IMAGE\" fail, ret=%d", ret);
		hd_ret = HD_ERR_NG;
	}

	return hd_ret;
}

HD_RESULT videoproc_get_pattern_p(HD_DAL self_id,  HD_IO in_id, HD_VIDEOPROC_PATTERN_IMG *p_pattern)
{
	HD_RESULT hd_ret = HD_OK;
	pat_img_info_t pattern_info;
	int ret = 0;

	if (!vpe_ioctl) {
		vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
	}
	if (!p_pattern) {
		HD_VIDEOPROC_ERR("Check p_pattern fail\n");
		return HD_ERR_ABORT;
	}
	pattern_info.index = p_pattern->index;
	if ((ret = ioctl(vpe_ioctl, VPE_IOC_GET_PATTERN_IMAGE, &pattern_info)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_GET_PATTERN_IMAGE\" fail, ret=%d", ret);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_pattern->index = (uint32_t)pattern_info.index;
	p_pattern->image.phy_addr[0] = (uint32_t)pattern_info.buf_addr;
	p_pattern->image.dim.w = pattern_info.width;
	p_pattern->image.dim.h = pattern_info.height;
	p_pattern->image.pxlfmt = pattern_info.type ? common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(pattern_info.type) : 0;
	p_pattern->image.ddr_id = pattern_info.ddr_id;

exit:
	return hd_ret;
}

HD_RESULT videoproc_enable_pattern_p(HD_DAL self_id,  HD_IO out_id, HD_VIDEOPROC_PATTERN_SELECT *p_pat_sel, HD_URECT *p_dst_rect)
{
	HD_RESULT hd_ret = HD_OK;
	pat_img_sel_info_t vpe_select_pattern = {0};
	int ret = 0;
	VDOPROC_HW_SPEC_INFO hw_spec_info = {0};
	UINT32 out_w, out_h;
	INT idx = VDOPROC_DEVID(self_id);
	VDOPROC_PARAM *vproc_param;

	if (!vpe_ioctl) {
		vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
	}
	vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
	if ((vproc_param == NULL) ||
	    (vproc_param->src_fmt == HD_VIDEO_PXLFMT_NONE) ||
	    (vproc_param->dst_fmt == HD_VIDEO_PXLFMT_NONE)) {//not set
		hw_spec_info.in_direction = 0;
		hw_spec_info.in_src_fmt = HD_VIDEO_PXLFMT_YUV420_NVX3;
		hw_spec_info.in_dst_fmt = HD_VIDEO_PXLFMT_YUV420_NVX3;
	} else {
		hw_spec_info.in_direction = vproc_param->direction;
		hw_spec_info.in_src_fmt = vproc_param->src_fmt;
		hw_spec_info.in_dst_fmt = vproc_param->dst_fmt;
	}
	if (videoproc_get_vpe_align_p(&hw_spec_info, 1) != HD_OK) {
		HD_VIDEOPROC_ERR("self_id(%u) get hardware spec info fail.\n", self_id);
		hd_ret = HD_ERR_DRV;
		goto err_exit;
	}

	vpe_select_pattern.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(self_id), VDOPROC_OUTID(out_id), NULL);

	if (!p_pat_sel) {
		vpe_select_pattern.x = 0;//p_pat_sel->rect.x;
		vpe_select_pattern.y = 0;//p_pat_sel->rect.y;
		vpe_select_pattern.w = 100;//p_pat_sel->rect.w;
		vpe_select_pattern.h = 100;//p_pat_sel->rect.h;
		//vpe_select_pattern.bg_color_sel = p_pat_sel->bg_color_sel;
		vpe_select_pattern.index = VPE_PATTERN_SEL_DISABLE;
	} else {
		vpe_select_pattern.x = p_pat_sel->rect.x;//percentage 0~100
		vpe_select_pattern.y = p_pat_sel->rect.y;//percentage 0~100
		vpe_select_pattern.w = p_pat_sel->rect.w;//percentage 0~100
		vpe_select_pattern.h = p_pat_sel->rect.h;//percentage 0~100
		out_w = (p_pat_sel->rect.w * p_dst_rect->w) / 100;
		out_h = (p_pat_sel->rect.h * p_dst_rect->h) / 100;
		if ((out_w < hw_spec_info.out_dst_min_dim.w) ||
		    (out_h < hw_spec_info.out_dst_min_dim.h)) {
			HD_VIDEOPROC_ERR("self_id(%u) set pattern fails.\n", self_id);
			HD_VIDEOPROC_ERR("percent w=%d(%%), h=%d(%%), w=%d, h=%d. \n", p_pat_sel->rect.w, p_pat_sel->rect.h, p_dst_rect->w, p_dst_rect->h);
			HD_VIDEOPROC_ERR("out_w=%d, h=%d, min w=%d, h=%d. \n", out_w, out_h, hw_spec_info.out_dst_min_dim.w, hw_spec_info.out_dst_min_dim.h);
			hd_ret = HD_ERR_NG;
			goto err_exit;
		}
		if ((p_pat_sel->rect.x) && (p_pat_sel->rect.x + p_pat_sel->rect.w > 100)) {
			HD_VIDEOPROC_ERR("self_id(%u) set pattern fails.\n", self_id);
			HD_VIDEOPROC_ERR("percent x=%d(%%), y=%d(%%), w=%d(%%), h=%d(%%). \n", p_pat_sel->rect.x, p_pat_sel->rect.y, p_pat_sel->rect.w, p_pat_sel->rect.h);
			hd_ret = HD_ERR_NG;
			goto err_exit;

		}
		if ((p_pat_sel->rect.y) && (p_pat_sel->rect.y + p_pat_sel->rect.h > 100)) {
			HD_VIDEOPROC_ERR("self_id(%u) set pattern fails.\n", self_id);
			HD_VIDEOPROC_ERR("percent x=%d(%%), y=%d(%%), w=%d(%%), h=%d(%%). \n", p_pat_sel->rect.x, p_pat_sel->rect.y, p_pat_sel->rect.w, p_pat_sel->rect.h);
			hd_ret = HD_ERR_NG;
			goto err_exit;

		}
		vpe_select_pattern.crop_en = p_pat_sel->img_crop_enable;;
		vpe_select_pattern.crop_x = p_pat_sel->img_crop.x;
		vpe_select_pattern.crop_y = p_pat_sel->img_crop.y;
		vpe_select_pattern.crop_w = p_pat_sel->img_crop.w;
		vpe_select_pattern.crop_h = p_pat_sel->img_crop.h;
		vpe_select_pattern.bg_color_sel = p_pat_sel->bg_color_sel;
		vpe_select_pattern.index = p_pat_sel->index;
	}
	if ((ret = ioctl(vpe_ioctl, VPE_IOC_SET_PATTERN_SEL, &vpe_select_pattern)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_SET_PATTERN_SEL\" fail, ret=%d", ret);
		hd_ret = HD_ERR_DRV;
	}
err_exit:
	return hd_ret;
}

HD_RESULT videoproc_get_pattern_sel_p(HD_DAL self_id,  HD_IO out_id, HD_VIDEOPROC_PATTERN_SELECT *p_pat_sel)
{
	HD_RESULT hd_ret = HD_OK;
	pat_img_sel_info_t vpe_select_pattern = {0};
	int ret = 0;

	if (!vpe_ioctl) {
		vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
	}
	if (!p_pat_sel) {
		HD_VIDEOPROC_ERR("Check p_pat_sel fail\n");
		return HD_ERR_ABORT;
	}
	vpe_select_pattern.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(self_id), VDOPROC_OUTID(out_id), NULL);
	vpe_select_pattern.index = p_pat_sel->index;
	if ((ret = ioctl(vpe_ioctl, VPE_IOC_GET_PATTERN_SEL, &vpe_select_pattern)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_GET_PATTERN_SEL\" fail, ret=%d", ret);
		hd_ret = HD_ERR_DRV;
		goto exit;
	}
	if (vpe_select_pattern.index == VPE_PATTERN_SEL_DISABLE)
		p_pat_sel->index = 0;//disbale
	else
		p_pat_sel->index = vpe_select_pattern.index+1;//return value is array_index, need to transfer to 1~MAX
	p_pat_sel->bg_color_sel = vpe_select_pattern.bg_color_sel;
	p_pat_sel->rect.x = vpe_select_pattern.x;
	p_pat_sel->rect.y = vpe_select_pattern.y;
	p_pat_sel->rect.w = vpe_select_pattern.w;
	p_pat_sel->rect.h = vpe_select_pattern.h;
	p_pat_sel->img_crop_enable = vpe_select_pattern.crop_en;
	p_pat_sel->img_crop.x = vpe_select_pattern.crop_x;
	p_pat_sel->img_crop.y = vpe_select_pattern.crop_y;
	p_pat_sel->img_crop.w = vpe_select_pattern.crop_w;
	p_pat_sel->img_crop.h = vpe_select_pattern.crop_h;

exit:
	return hd_ret;
}

HD_RESULT videoproc_set_vpemask_p(HD_DAL self_id,  HD_IO out_id, HD_VIDEOPROC_VPEMASK_ONEINFO *p_pattern)
{
	HD_RESULT hd_ret = HD_OK;
	mask_win_t pattern_info = {0};
	mask_param_t pattern_para = {0};
	int ret = 0, j;
	mask_mosaic_block_t mask_mosa_size;

	if (!vpe_ioctl) {
		vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
	}
	if (!p_pattern) {
		HD_VIDEOPROC_ERR("Check p_pattern fail\n");
		return HD_ERR_ABORT;
	}
	pattern_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(self_id), VDOPROC_OUTID(out_id), NULL);
	pattern_info.mask_idx = p_pattern->mask_idx;
	if (p_pattern->index) {
		pattern_info.enable = 1; // 1: enable, 0:disable, and ignore other settings
	} else {
		pattern_info.enable = 0; // 1: enable, 0:disable, and ignore other settings
	}
	pattern_info.mask_area = p_pattern->mask_area; //0:inside, 1:outside, 2:line
	for (j = 0; j < 4; j++) {
		pattern_info.point[j].x = p_pattern->point[j].x; // 4 points
		pattern_info.point[j].y = p_pattern->point[j].y; // 4 points
	}
	if ((ret = ioctl(vpe_ioctl, VPE_IOC_SET_MASK_WIN_INFO, &pattern_info)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_SET_MASK_WIN_INFO\" fail, ret=%d\n", ret);
		hd_ret = HD_ERR_NG;
	}
	pattern_para.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(self_id), VDOPROC_OUTID(out_id), NULL);
	pattern_para.mask_idx = p_pattern->mask_idx;
	pattern_para.mosaic_en = p_pattern->mosaic_en; // alpha> 100, seen. use original image or mosaic image in mask area
	//pattern_para.bitmap = VPE_MASK_BITMAP_NONE; // bitmap type
	pattern_para.alpha = p_pattern->alpha; // alpha blending 0~256, only effect at bitmap = 0 or 1
	pattern_para.pal_sel = 0; // palette idx 0:black, 1:white, only effect at bitmap = 0 or 1,
	if ((ret = ioctl(vpe_ioctl, VPE_IOC_SET_MASK_PARAM_INFO, &pattern_para)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_SET_MASK_PARAM_INFO\" fail, ret=%d\n", ret);
		hd_ret = HD_ERR_NG;
	}

	mask_mosa_size.fd = pattern_para.fd;
	mask_mosa_size.mosaic_blkw = VPEMASK_MOSAIC_BLK;
	mask_mosa_size.mosaic_blkh = VPEMASK_MOSAIC_BLK;
	if ((ret = ioctl(vpe_ioctl, VPE_IOC_SET_MASK_MOSAIC_SIZE, &mask_mosa_size)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_SET_MASK_PARAM_INFO\" fail, ret=%d\n", ret);
		hd_ret = HD_ERR_NG;
	}

	return hd_ret;
}

HD_RESULT videoproc_get_vpemask_p(HD_DAL self_id,  HD_IO out_id, HD_VIDEOPROC_VPEMASK_ONEINFO *p_pattern)
{
	HD_RESULT hd_ret = HD_OK;
	mask_win_t pattern_info = {0};
	mask_param_t pattern_para = {0};
	int ret = 0, j;

	if (!vpe_ioctl) {
		vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
	}
	if (!p_pattern) {
		HD_VIDEOPROC_ERR("Check p_pattern fail\n");
		return HD_ERR_ABORT;
	}
	pattern_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(self_id), VDOPROC_OUTID(out_id), NULL);
	pattern_info.mask_idx = p_pattern->mask_idx;
	if ((ret = ioctl(vpe_ioctl, VPE_IOC_GET_MASK_WIN_INFO, &pattern_info)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_SET_MASK_WIN_INFO\" fail, ret=%d\n", ret);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_pattern->mask_area = pattern_info.mask_area;
	p_pattern->index = pattern_info.enable;
	for (j = 0; j < 4; j++) {
		p_pattern->point[j].x = pattern_info.point[j].x;
		p_pattern->point[j].y = pattern_info.point[j].y;
	}

	pattern_para.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(self_id), VDOPROC_OUTID(out_id), NULL);
	pattern_para.mask_idx = p_pattern->mask_idx;
	if ((ret = ioctl(vpe_ioctl, VPE_IOC_GET_MASK_PARAM_INFO, &pattern_para)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_SET_MASK_PARAM_INFO\" fail, ret=%d\n", ret);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_pattern->mosaic_en = pattern_para.mosaic_en;
	p_pattern->alpha = pattern_para.alpha;

exit:
	return hd_ret;
}

HD_RESULT videoproc_set_sca_buf_info(HD_DAL self_id,  HD_IO out_id, HD_VIDEOPROC_SCA_BUF_INFO *sca_buf_info)
{
	HD_RESULT hd_ret = HD_OK;
	fd_sca_buf_info_t vpe_sca_buf_param = {0};
	int ret = 0;

	if (!vpe_ioctl) {
		vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
	}
	if (!sca_buf_info) {
		HD_VIDEOPROC_ERR("Check sca_buf_info fail\n");
		return HD_ERR_ABORT;
	}
	vpe_sca_buf_param.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(self_id), VDOPROC_OUTID(out_id), NULL);
	vpe_sca_buf_param.ddr_id = sca_buf_info->ddr_id;
	vpe_sca_buf_param.pbuf_addr = sca_buf_info->pbuf_addr;
	vpe_sca_buf_param.pbuf_size = sca_buf_info->pbuf_size;

	if ((ret = ioctl(vpe_ioctl, VPE_IOC_SET_SCA_WK_BUF, &vpe_sca_buf_param)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_SET_SCA_WK_BUF\" fail, ret=%d", ret);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}

HD_RESULT videoproc_get_sca_buf_info(HD_DAL self_id,  HD_IO out_id, HD_VIDEOPROC_SCA_BUF_INFO *sca_buf_info)
{
	HD_RESULT hd_ret = HD_OK;
	fd_sca_buf_info_t vpe_sca_buf_param = {0};
	int ret = 0;

	if (!vpe_ioctl) {
		vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
	}
	if (!sca_buf_info) {
		HD_VIDEOPROC_ERR("Check sca_buf_info fail\n");
		return HD_ERR_ABORT;
	}
	vpe_sca_buf_param.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(self_id), VDOPROC_OUTID(out_id), NULL);
	if ((ret = ioctl(vpe_ioctl, VPE_IOC_GET_SCA_WK_BUF, &vpe_sca_buf_param)) < 0) {
		HD_VIDEOPROC_ERR("ioctl \"VPE_IOC_GET_SCA_WK_BUF\" fail, ret=%d", ret);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	sca_buf_info->ddr_id = vpe_sca_buf_param.ddr_id;
	sca_buf_info->pbuf_addr = vpe_sca_buf_param.pbuf_addr;
	sca_buf_info->pbuf_size = vpe_sca_buf_param.pbuf_size;

exit:
	return hd_ret;
}

HD_RESULT videoproc_check_path_id_p(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);

	if (VDOPROC_DEVID(self_id) > videoproc_max_device_nr) {
		HD_VIDEOPROC_ERR("Error self_id(%#x)\n", self_id);
		return HD_ERR_NG;
	}
	if (HD_GET_CTRL(path_id) != HD_CTRL) {
		if (VDOPROC_INID(in_id) >= videoproc_max_device_in) {
			HD_VIDEOPROC_ERR("Check input port error, port(%#x) max(%#x)\n", in_id, videoproc_max_device_in);
			return HD_ERR_NG;
		}
		if (VDOPROC_OUTID(out_id) >= videoproc_max_device_out) {
			HD_VIDEOPROC_ERR("Check output port error, port(%#x) max(%#x)\n", out_id, videoproc_max_device_out);
			return HD_ERR_NG;
		}
	}
	return HD_OK;
}

HD_RESULT videoproc_init_path_p(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	INT fd = 0, minor, idx, i;
	UINT32 chip;
	uintptr_t queue_handle;
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 out_subid = VDOPROC_OUTID(out_id);

	idx = VDOPROC_DEVID(self_id);
	if (out_subid >= videoproc_max_device_out) {
		HD_VIDEOPROC_ERR("Check path_id(%#x) error, out_subid(%d) max(%d)\n", path_id, out_subid, videoproc_max_device_out);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	if (vdoproc_param[idx][out_subid] == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	/* VPE engine */
	chip = idx / HD_DAL_VIDEOPROC_CHIP_DEV_COUNT;
	minor = utl_get_vpe_minor(idx, out_subid);
	if (minor < 0) {
		HD_VIDEOPROC_ERR("get videoproc minor(%d) failed, please check if the driver inserted successfully.\n", minor);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	//++ vpe_lite eng --> 1
	//fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_VPE, chip, 1, minor);
	fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_VPE, chip, 0, minor);

	if (fd == 0) {
		HD_VIDEOPROC_ERR("get videoproc fd(%#x) failed, please check if the driver inserted successfully.\n", fd);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	vdoproc_param[idx][out_subid]->priv.fd = fd;

	minor = idx % HD_DAL_VIDEOPROC_CHIP_DEV_COUNT;
	fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_DI, chip, 0, minor);
	vdoproc_param[idx][out_subid]->priv.di_fd = fd;

	queue_handle = pif_create_queue_for_hdal();
	if (queue_handle == 0) {
		HD_VIDEOPROC_ERR("create queue_handle failed.\n");
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	vdoproc_param[idx][out_subid]->priv.queue_handle = queue_handle;

	for (i = 0; i < HD_VP_MAX_DATA_TYPE; i++) {
		vdoproc_param[idx][out_subid]->out_pool.buf_info[i].enable = HD_BUF_AUTO;
	}
	vpe_cfg[idx][out_subid].p_out_pool = &vdoproc_param[idx][out_subid]->out_pool;

	/* fill default value */
	{
		UINT32 sub_thld_numer = VDOPROC_SUB_RATIO_THLD_NUMER;
		UINT32 sub_thld_denom = VDOPROC_SUB_RATIO_THLD_DENOM;
		vpe_cfg[idx][out_subid].sub_ratio_thld = ((sub_thld_numer & 0xFFFF) << 16) | (sub_thld_denom & 0xFFFF);

		vdoproc_param[idx][out_subid]->slice_mode.slice_height = 256;
		vdoproc_param[idx][out_subid]->slice_mode.enable = 0;
	}

exit:
	return hd_ret;
}

HD_RESULT videoproc_uninit_path_p(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	INT idx, ret;
	usr_proc_stop_trigger_t stop_trigger_info;
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 out_subid = VDOPROC_OUTID(out_id);

	idx = VDOPROC_DEVID(self_id);
	if (out_subid >= videoproc_max_device_out) {
		HD_VIDEOPROC_ERR("Check path_id(%#x) error, out_subid(%d) max(%d)\n", path_id, out_subid, videoproc_max_device_out);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	if (vdoproc_param[idx][out_subid] == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	/* call driver stop */
	memset(&stop_trigger_info, 0, sizeof(stop_trigger_info));
	if ((vdoproc_param[idx][out_subid]->priv.fd)) {
		stop_trigger_info.usr_proc_fd.fd = vdoproc_param[idx][out_subid]->priv.fd;
		stop_trigger_info.mode = USR_PROC_STOP_FD;
		ret = ioctl(vpd_fd, USR_PROC_STOP_TRIGGER, &stop_trigger_info);
		if (ret < 0) {
			int errsv = errno;
			ret = errsv * -1;
			HD_VIDEOENC_ERR("<ioctl \"USR_PROC_STOP_TRIGGER\" fail, error(%d)>\n", ret);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		vdoproc_param[idx][out_subid]->priv.fd = 0;
	}

	if ((vdoproc_param[idx][out_subid]->di_enable) && (vdoproc_param[idx][out_subid]->priv.di_fd)) {
		stop_trigger_info.usr_proc_fd.fd = vdoproc_param[idx][out_subid]->priv.di_fd;
		stop_trigger_info.mode = USR_PROC_STOP_FD;
		ret = ioctl(vpd_fd, USR_PROC_STOP_TRIGGER, &stop_trigger_info);
		if (ret < 0) {
			int errsv = errno;
			ret = errsv * -1;
			HD_VIDEOENC_ERR("<ioctl \"USR_PROC_STOP_TRIGGER\" fail, error(%d)>\n", ret);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		vdoproc_param[idx][out_subid]->priv.di_fd = 0;
	}

	/* clear pull queue */
	ret = pif_destroy_queue_for_hdal(vdoproc_param[idx][out_subid]->priv.queue_handle);
	if (ret < 0) {
		HD_VIDEOPROC_ERR("destroy queue failed. ret(%d)\n", ret);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

exit:
	return hd_ret;
}

HD_RESULT videoproc_init_p(void)
{
	HD_RESULT hd_ret = HD_OK;

	memset(vpe_cfg, 0, sizeof(vpe_cfg));

	return hd_ret;
}

HD_RESULT videoproc_uninit_p(void)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}


HD_RESULT videoproc_close_vpemask(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 out_subid = VDOPROC_OUTID(out_id);
	HD_VIDEOPROC_VPEMASK_ONEINFO  vpeinfo = {0};
	UINT32 idx, i;

	idx = VDOPROC_DEVID(self_id);
	if (out_subid >= videoproc_max_device_out) {
		HD_VIDEOPROC_ERR("Check path_id(%#x) error, out_subid(%d) max(%d)\n", path_id, out_subid, videoproc_max_device_out);
		return HD_ERR_NG;
	}
	if (vdoproc_param[idx][out_subid] == NULL) {
		HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", path_id);
		return HD_ERR_NG;
	}

	/* No need to disable mask if it's not enable */
	if (vdoproc_param[idx][out_subid]->vpeinfo.index == 0) {
		return HD_OK;
	}

	for (i = 0; i < 8; i++) {
		vpeinfo.mask_idx = i;
		vpeinfo.index = 0;
	}

	memcpy((void *)&vdoproc_param[idx][out_subid]->vpeinfo, (void *)&vpeinfo, sizeof(HD_VIDEOPROC_VPEMASK_ONEINFO));
	videoproc_set_vpemask_p(self_id, out_id, &vdoproc_param[idx][out_subid]->vpeinfo);

	return HD_OK;
}

HD_RESULT videoproc_videoproc_close_uv_swap(HD_PATH_ID path_id)
{
	HD_RESULT ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT fd = 0;
	fd_sca_uv_swap_t sca_uv_swap = {0};	
	
	fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	if (fd == 0) {
		HD_VIDEOPROC_ERR("utl_get_vpe_entity_fd fail\n");
		ret = HD_ERR_NOT_AVAIL;
		goto exit;
	}
	if (!vpe_ioctl) {
		vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
	}
	if (vpe_ioctl < 0) {
		HD_VIDEOPROC_ERR("open %s  fail\n", VIDEOPROC_DEVICE);
		ret = HD_ERR_NOT_AVAIL;
		goto exit;
	}
	
	sca_uv_swap.fd  = fd;
	sca_uv_swap.uv_swap = 0;
	if (ioctl(vpe_ioctl, VPE_IOC_SET_SCA_UV_SWAP, &sca_uv_swap) < 0) {
		HD_VIDEOPROC_ERR("VPE_IOC_SET_SCA_UV_SWAP ioctl failed\n");
		ret = HD_ERR_NG;
		goto exit;
	}
exit:
	return ret;
}

HD_RESULT videoproc_check_rotate_and_crop(VDOPROC_PARAM *vdoproc_param)
{
	HD_RESULT result = HD_OK;
	UINT32 rotate = 0, align_w = 32, align_h = 8;
	if (!vdoproc_param) {
		return HD_ERR_NG;
	}
	switch (vdoproc_param->src_fmt) {
	case HD_VIDEO_PXLFMT_YUV422_ONE:
		align_w = 8;
		align_h = 8;
		break;
	case HD_VIDEO_PXLFMT_YUV420:
		align_w = 16;
		align_h = 8;
		break;
	default:
		break;
	}
	if ((vdoproc_param->direction == HD_VIDEO_DIR_ROTATE_270) ||
	    (vdoproc_param->direction == HD_VIDEO_DIR_ROTATE_90)) {
		rotate = 1;
	}
	if ((vdoproc_param->src_rect_enable) && (rotate)) { // vcap crop on
		if (vdoproc_param->src_rect.w % align_w) {
			HD_VIDEOPROC_ERR("in crop+rotate w should %d alignment. w(%d)\n", align_w, vdoproc_param->src_rect.w);
			result = HD_ERR_NG;
		}
		if (vdoproc_param->src_rect.h % align_h) {
			HD_VIDEOPROC_ERR("in crop+rotate h should %d alignment. h(%d)\n", align_h, vdoproc_param->src_rect.h);
			result = HD_ERR_NG;
		}
	}
	if (rotate) {
		if (vdoproc_param->src_bg_dim.w % align_w) {
			HD_VIDEOPROC_ERR(" rotate w should %d alignment. w(%d)\n", align_w, vdoproc_param->src_bg_dim.w);
			result = HD_ERR_NG;
		}
		if (vdoproc_param->src_bg_dim.h % align_h) {
			HD_VIDEOPROC_ERR(" rotate h should %d alignment. h(%d)\n", align_h, vdoproc_param->src_bg_dim.h);
			result = HD_ERR_NG;
		}
	}
	return result;
}

HD_RESULT videoproc_check_pip_and_psr(HD_PATH_ID path_id, VDOPROC_PARAM *vdoproc_param, HD_VIDEOPROC_CROP *p_crop)
{
	HD_RESULT result = HD_OK;

	if (p_crop == NULL) {
		return result;
	}
	if (p_crop->mode != HD_CROP_ON) {
		return result;
	}
	if (vdoproc_param->src_rect_enable) {
		if (vdoproc_param->src_rect.x + vdoproc_param->src_rect.w > vdoproc_param->src_bg_dim.w) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.x(%u) + win.rect.w(%u) > win.coord.w(%u), please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vdoproc_param->src_rect.x, vdoproc_param->src_rect.w, vdoproc_param->src_bg_dim.w);
			result = HD_ERR_NG;
		}
		if (vdoproc_param->src_rect.y + vdoproc_param->src_rect.h > vdoproc_param->src_bg_dim.h) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.y(%u) + win.rect.h(%u) > win.coord.h(%u), please check HD_VIDEOPROC_PARAM_IN_CROP.\n",
					 path_id, vdoproc_param->src_rect.y, vdoproc_param->src_rect.h, vdoproc_param->src_bg_dim.h);
			result = HD_ERR_NG;
		}
	}

	if (vdoproc_param->psr_crop_enable) {
		if (vdoproc_param->psr_crop_rect.x + vdoproc_param->psr_crop_rect.w > vdoproc_param->psr_crop_bg_dim.w) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.x(%u) + win.rect.w(%u) > win.coord.w(%u), please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vdoproc_param->psr_crop_rect.x, vdoproc_param->psr_crop_rect.w, vdoproc_param->psr_crop_bg_dim.w);
			result = HD_ERR_NG;
		}
		if (vdoproc_param->psr_crop_rect.y + vdoproc_param->psr_crop_rect.h > vdoproc_param->psr_crop_bg_dim.h) {
			HD_VIDEOPROC_ERR("path_id(%#x) win.rect.y(%u) + win.rect.h(%u) > win.coord.h(%u), please check HD_VIDEOPROC_PARAM_IN_CROP_PSR.\n",
					 path_id, vdoproc_param->psr_crop_rect.y, vdoproc_param->psr_crop_rect.h, vdoproc_param->psr_crop_bg_dim.h);
			result = HD_ERR_NG;
		}
	}
	return result;
}

HD_RESULT videoproc_check_pattern(HD_PATH_ID path_id, VDOPROC_PARAM *vdoproc_param)
{
	UINT32 out_w, dst_w;
	HD_RESULT ret = HD_OK;
	if (vdoproc_param == 0) {
		goto exit;
	}
	if (vdoproc_param->dst_rect.w == 0) {
		goto exit;
	}
	if (vdoproc_param->pattern.pattern_select.index == VPE_PATTERN_SEL_DISABLE) {
		goto exit;
	}
	if (vdoproc_param->pattern.pattern_select.rect.w == 0) {
		goto exit;
	}
	dst_w = vdoproc_param->dst_rect.w;
	out_w = vdoproc_param->pattern.pattern_select.rect.w * dst_w / 100;
	if ((dst_w > 2048) && (out_w <= 2048)) {
		HD_VIDEOPROC_ERR(" HD_VIDEOPROC_PARAM_PATTERN_SELECT:exceed HW limit, dst_w = %u, out_w = %u\n", dst_w, out_w);
		return HD_ERR_SYS;
	}

exit:
	return ret;
}

HD_RESULT videoproc_push_list_p(HD_PATH_ID *p_path_id, HD_VIDEO_FRAME *p_in_video_frame,
				HD_VIDEO_FRAME *p_out_video_frame, UINT32 num, INT32 wait_ms, HD_DIM *p_llc_dim)
{
	HD_RESULT hd_ret = HD_OK;
	CHAR pool_name[20] = {};
	INT32 i = 0, idx = 0, ret = 0;
	HD_DIM out_dim = {0};
	HD_VIDEO_PXLFMT out_fmt = HD_VIDEO_PXLFMT_NONE;
	UINT estimate_size = 0, query_size = 0;
	VDOPROC_PARAM *vproc_param = NULL;
	HD_DAL self_id = 0;
	HD_IO out_id = 0;
	HD_VIDEO_FRAME *p_tmp_in_video_frame = NULL;
	HD_DIM in_dim[VPD_VPETRIG_MAXNUM] = {0};
	UINTPTR in_pa[VPD_VPETRIG_MAXNUM] = {0};
	UINT32 in_ddr_id[VPD_VPETRIG_MAXNUM] = {0};
	HD_VIDEO_PXLFMT in_fmt[VPD_VPETRIG_MAXNUM] = {0};
	UINT32 side_offset[VPD_VPETRIG_MAXNUM] = {0};
	UINT32 side_size[VPD_VPETRIG_MAXNUM] = {0};
	INT errsv = 0;
	static usr_proc_trigger_vpe_t usr_vpe[VPD_VPETRIG_MAXNUM] = {0};

	/* check NULL param */
	if (p_path_id == NULL) {
		HD_VIDEOPROC_ERR("NULL p_path_id\n");
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	if (p_in_video_frame == NULL) {
		HD_VIDEOPROC_ERR("NULL p_in_video_frame\n");
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	if (p_out_video_frame == NULL) {
		HD_VIDEOPROC_ERR("NULL p_out_video_frame\n");
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	if (num == 0) {
		HD_VIDEOPROC_ERR("push list num(%u) need > 0\n", num);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	if (num > VPD_VPETRIG_MAXNUM) {
		HD_VIDEOPROC_ERR("push list num(%u) exceeds MAX(%u)\n", num, VPD_VPETRIG_MAXNUM);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	/* check pathid and param */
	for (i = 0; i < (INT32)num; i++) {
		ret = videoproc_check_path_id_p(p_path_id[i]);
		if (ret != HD_OK) {
			goto exit;
		}
		self_id = HD_GET_DEV(p_path_id[i]);
		out_id = HD_GET_OUT(p_path_id[i]);
		idx = VDOPROC_DEVID(self_id);
		vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
		if (vproc_param == NULL) {
			HD_VIDEOPROC_ERR("path_id(%#x) err, it maybe no open.\n", p_path_id[i]);
			hd_ret = HD_ERR_NG;
			goto exit;
		}

		if (i == 0) {
			out_fmt = vproc_param->dst_fmt;
			out_dim.w = ALIGN16_UP(vproc_param->dst_bg_dim.w);
			out_dim.h = ALIGN16_UP(vproc_param->dst_bg_dim.h);
			estimate_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(out_dim, out_fmt);
			if (estimate_size == 0) {
				HD_VIDEOPROC_ERR("Estimate buffer size is 0. path_id(%#x) wh(%u,%u) fmt(%#x)\n",
						 p_path_id[i], out_dim.w, out_dim.h, out_fmt);
				hd_ret = HD_ERR_NG;
				goto exit;
			}
		} else {
			if ((out_dim.w != ALIGN16_UP(vproc_param->dst_bg_dim.w)) ||
			    (out_dim.h != ALIGN16_UP(vproc_param->dst_bg_dim.h)) ||
			    (out_fmt != vproc_param->dst_fmt)) {
				HD_VIDEOPROC_ERR("path_id(%#x) HD_VIDEOPROC_PARAM_OUT wh(%u,%u) pxlfmt(%#x) must same with FIRST pathid's setting wh(%u,%u) pxlfmt(%#x)\n",
						 p_path_id[i], vproc_param->dst_bg_dim.w, vproc_param->dst_bg_dim.h, vproc_param->dst_fmt, out_dim.w, out_dim.h, out_fmt);
				hd_ret = HD_ERR_NG;
				goto exit;
			}
		}
	}

	/* check input video frame */
	if (p_in_video_frame->ddr_id > DDR_ID_MAX) {
		HD_VIDEOPROC_ERR("input video frame error, ddr_id=%d exceeds max %d\n", p_in_video_frame->ddr_id, DDR_ID_MAX);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_tmp_in_video_frame = p_in_video_frame;
	i = 0;
	while (p_tmp_in_video_frame) {
		in_dim[i].w = p_tmp_in_video_frame->dim.w;
		in_dim[i].h = p_tmp_in_video_frame->dim.h;
		in_pa[i] = p_tmp_in_video_frame->phy_addr[0];
		in_ddr_id[i] = p_tmp_in_video_frame->ddr_id;
		in_fmt[i] = p_tmp_in_video_frame->pxlfmt;
		side_offset[i] = common_calculate_dec_side_info_offset(&p_llc_dim[i], HD_CODEC_TYPE_H265);
		side_size[i] = common_calculate_dec_side_info_size(&p_llc_dim[i], HD_CODEC_TYPE_H265);
		i++;
		if ((p_tmp_in_video_frame->p_next)
		    && (p_tmp_in_video_frame->ddr_id < DDR_ID_MAX)
		    && (videoproc_check_addr((UINTPTR)p_tmp_in_video_frame->p_next) == TRUE)) {
			p_tmp_in_video_frame = (HD_VIDEO_FRAME *)(p_tmp_in_video_frame->p_next);
		} else {
			break;
		}
	}
	if (i != (INT32)num) {
		HD_VIDEOPROC_ERR("p_in_video_frame number(%d) != num(%u), please check\n", i, num);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	/* check output video frame */
	if (p_out_video_frame->ddr_id > DDR_ID_MAX) {
		HD_VIDEOPROC_ERR("output video frame error, ddr_id=%d exceeds max %d\n", p_out_video_frame->ddr_id, DDR_ID_MAX);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	ret = pif_query_video_buffer_size(p_out_video_frame->phy_addr[0], p_out_video_frame->ddr_id, pool_name, &query_size);
	if (ret == -2) {
		/* can not find pool by this pa, skip it */
		hd_ret = HD_OK;
	} else if (ret < 0) {
		HD_VIDEOPROC_ERR("query_video_buffer_size fail\n");
		hd_ret = HD_ERR_NOBUF;
		goto exit;
	} else {
		if (query_size < estimate_size) {
			HD_VIDEOPROC_ERR("path_id(%#x) output video frame too small, buf size(%u) < set size(%u) by wh(%u,%u) pxlfmt(%#x)\n",
					 p_path_id[0], query_size, estimate_size, out_dim.w, out_dim.h, out_fmt);
			hd_ret = HD_ERR_NOBUF;
			goto exit;
		}
	}
	if ((p_out_video_frame->p_next)
	    && (videoproc_check_addr((UINTPTR)p_out_video_frame->p_next) == TRUE)) {
		HD_VIDEOPROC_ERR("ONLY ONE output video frame for push list, please check.\n");
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	for (i = 0; i < (INT32)num; i++) {
		self_id = HD_GET_DEV(p_path_id[i]);
		out_id = HD_GET_OUT(p_path_id[i]);
		idx = VDOPROC_DEVID(self_id);
		vproc_param = vdoproc_param[idx][VDOPROC_OUTID(out_id)];
		if ((vproc_param->di_enable) && (vproc_param->priv.di_fd)) {
			usr_vpe[i].fd = vproc_param->priv.di_fd;
		} else if (vproc_param->priv.fd) {
			usr_vpe[i].fd = vproc_param->priv.fd;
		} else {
			HD_VIDEOPROC_ERR("No vaild handle for pathid(%#x), please open first.\n", p_path_id[i]);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		usr_vpe[i].in_frame_buffer.ddr_id = in_ddr_id[i];
		usr_vpe[i].in_frame_buffer.addr_pa = in_pa[i];
		if (pif_address_ddr_sanity_check(usr_vpe[i].in_frame_buffer.addr_pa, usr_vpe[i].in_frame_buffer.ddr_id) < 0) {
			HD_VIDEOPROC_ERR("%s:Check in_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_vpe[i].in_frame_buffer.addr_pa, usr_vpe[i].in_frame_buffer.ddr_id);
			hd_ret = HD_ERR_PARAM;
			goto exit;
		}
		usr_vpe[i].in_frame_buffer.size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(in_dim[i], in_fmt[i]);
		sprintf(usr_vpe[i].in_frame_buffer.pool_name, pool_name);
		usr_vpe[i].in_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(in_fmt[i]);
		usr_vpe[i].in_frame_info.dim.w = vproc_param->src_bg_dim.w;
		usr_vpe[i].in_frame_info.dim.h = vproc_param->src_bg_dim.h;
		usr_vpe[i].in_frame_info.pathid = p_path_id[i];

		usr_vpe[i].out_frame_buffer.ddr_id = p_out_video_frame->ddr_id;
		usr_vpe[i].out_frame_buffer.addr_pa = p_out_video_frame->phy_addr[0];
		if (pif_address_ddr_sanity_check(usr_vpe[i].out_frame_buffer.addr_pa, usr_vpe[i].out_frame_buffer.ddr_id ) < 0) {
			HD_VIDEOPROC_ERR("%s:Check out_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_vpe[i].out_frame_buffer.addr_pa, usr_vpe[i].out_frame_buffer.ddr_id);
			hd_ret = HD_ERR_PARAM;
			goto exit;
		}
		usr_vpe[i].out_frame_buffer.size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(out_dim, out_fmt);
		usr_vpe[i].out_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(out_fmt);
		usr_vpe[i].out_frame_info.dim.w = vproc_param->dst_bg_dim.w;
		usr_vpe[i].out_frame_info.dim.h = vproc_param->dst_bg_dim.h;

		usr_vpe[i].scale.src_rect.x = vproc_param->src_rect.x;
		usr_vpe[i].scale.src_rect.y = vproc_param->src_rect.y;
		usr_vpe[i].scale.src_rect.w = vproc_param->src_rect.w;
		usr_vpe[i].scale.src_rect.h = vproc_param->src_rect.h;

		usr_vpe[i].scale.dst_rect.x = vproc_param->dst_rect.x;
		usr_vpe[i].scale.dst_rect.y = vproc_param->dst_rect.y;
		usr_vpe[i].scale.dst_rect.w = vproc_param->dst_rect.w;
		usr_vpe[i].scale.dst_rect.h = vproc_param->dst_rect.h;

		usr_vpe[i].vpe_num = num;
		usr_vpe[i].transform.enable = convert_hdal_to_vpd_flip_rotate(vproc_param->direction,
					      &usr_vpe[i].transform.rotate_mode,
					      &usr_vpe[i].transform.flip_mode);
		usr_vpe[i].quality.enable = convert_hdal_to_vpd_di_dn_sharp(vproc_param->di_enable,
					    vproc_param->dn_enable,
					    vproc_param->sharp_enable,
					    &usr_vpe[i].quality.di_mode,
					    &usr_vpe[i].quality.dn_mode,
					    &usr_vpe[i].quality.shrp_mode);

		usr_vpe[i].queue_handle = vproc_param->priv.queue_handle;
		usr_vpe[i].p_user_data = NULL;
		usr_vpe[i].uv_swap = vproc_param->uv_swap;
		usr_vpe[i].side_offset = side_offset[i];
		usr_vpe[i].side_size = side_size[i];
	}

	ret = ioctl(vpd_fd, USR_PROC_PUSH_LIST_VPE, usr_vpe);
	if (ret < 0) {
		errsv = errno * -1;
		if (errsv == -2) {
			HD_VIDEOPROC_ERR("in_frame_buffer is still in use, please 'pull' this buffer before push it.\n");
		}

		HD_VIDEOPROC_ERR("<ioctl \"USR_PROC_PUSH_LIST_VPE\" fail, error(%d)>\n", errsv);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}

exit:
	return hd_ret;
}
