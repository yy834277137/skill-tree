/**
    @brief Header file of video output module.\n
    This file contains the functions which is related to video output in the chip.

    @file hd_videoout.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include "hdal.h"
#include "hd_type.h"
#include "vpd_ioctl.h"
#include "pif.h"
#include "dif.h"
#include "videoout.h"
#include "logger.h"
#include "cif_common.h"

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
UINT32 hd_videoout_max_device_nr;
UINT32 hd_videoout_max_device_out;
UINT32 hd_videoout_max_device_in;
VDDO_PARAM g_video_out_param[HD_VIDEOOUT_MAX_DEVICE_ID];
extern VDDO_INTERNAL_PARAM lcd_cfg[HD_VIDEOOUT_MAX_DEVICE_ID];
extern vpd_sys_info_t platform_sys_Info;
#define PAGE_SHIFT                  12
#define PAGE_SIZE                   (1 << PAGE_SHIFT)
#define ALIGN2_DOWN(a)              ALIGN_FLOOR(a, 2)
static int vo_lock_init = 0;
static pthread_rwlock_t videoout_rwlock;
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

HD_RESULT get_lcd_dim_by_value(HD_VIDEOOUT_INPUT_DIM lcd_dim, UINT32 *w, UINT32 *h)
{
	HD_RESULT ret = HD_OK;
	switch ((int)lcd_dim) {
	case HD_VIDEOOUT_IN_AUTO://LCD200_OTYPE_CVBS_NTSC
		*w = 720;
		*h = 576;
		break;
	case HD_VIDEOOUT_IN_640x480:
		*w = 640;
		*h = 480;
		break;
	case HD_VIDEOOUT_IN_720x480:
		*w = 720;
		*h = 480;
		break;
	case HD_VIDEOOUT_IN_720x576:
		*w = 720;
		*h = 576;
		break;
	case HD_VIDEOOUT_IN_1024x768:
		*w = 1024;
		*h = 768;
		break;
	case HD_VIDEOOUT_IN_800x600:
		*w = 800;
		*h = 600;
		break;
	case HD_VIDEOOUT_IN_1280x720:
		*w = 1280;
		*h = 720;
		break;
	case HD_VIDEOOUT_IN_2560x720:
		*w = 2560;
		*h = 720;
		break;
	case HD_VIDEOOUT_IN_2560x1440:
		*w = 2560;
		*h = 1440;
		break;
	case HD_VIDEOOUT_IN_1280x1024:
		*w = 1280;
		*h = 1024;
		break;
	case HD_VIDEOOUT_IN_1600x1200:
		*w = 1600;
		*h = 1200;
		break;
	case HD_VIDEOOUT_IN_1920x1080:
		*w = 1920;
		*h = 1080;
		break;
	case HD_VIDEOOUT_IN_3840x1080:
		*w = 3840;
		*h = 1080;
		break;
	case HD_VIDEOOUT_IN_3840x2160:
		*w = 3840;
		*h = 2160;
		break;
	case 14: //VENDOR_VIDEOOUT_IN_1440x900
		*w = 1440;
		*h = 900;
		break;
	case 15: //VENDOR_VIDEOOUT_IN_1680x1050
		*w = 1680;
		*h = 1050;
		break;
	case 16://VENDOR_VIDEOOUT_IN_1920x1200
		*w = 1920;
		*h = 1200;
		break;
	default:
		HD_VIDEOOUT_ERR("Not support lcd dim(%d)\n", lcd_dim);
		*w = 0;
		*h = 0;
		ret = HD_ERR_NOT_SUPPORT;
	}
	return ret;

}


HD_RESULT hd_videoout_open(HD_IN_ID in_id, HD_OUT_ID out_id, HD_PATH_ID *p_path_id)
{
	HD_DAL in_dev_id = HD_GET_DEV(in_id);
	HD_IO _in_id = HD_GET_IN(in_id);
	HD_DAL self_id = HD_GET_DEV(out_id);
	HD_IO _out_id = HD_GET_OUT(out_id);
	HD_IO ctrl_id = HD_GET_CTRL(out_id);
	HD_RESULT ret = HD_OK;
	VDDO_INFO_PRIV *p_vo_priv = NULL;
	INT lcd_id = 0;
	INT in_idx = 0;

	*p_path_id = 0;
	VIDEOOUT_CHECK_SELF_ID(self_id);
	lcd_id = VO_DEVID(self_id);

	if ((_in_id == 0) && (ctrl_id == HD_CTRL) && (lcd_id >= 0) && (lcd_id < HD_VIDEOOUT_MAX_DEVICE_ID)) {
		/* check and set state */
		p_vo_priv = &g_video_out_param[lcd_id].priv_ctrl;
		if ((p_vo_priv->push_state == HD_PATH_ID_OPEN) ||
		    (p_vo_priv->pull_state == HD_PATH_ID_OPEN)) {
			if ((ret = bd_get_already_open_pathid(self_id, _out_id, _in_id, p_path_id)) != HD_OK) {
				goto ext;
			} else {
				ret = HD_ERR_ALREADY_OPEN;
				goto ext;
			}
		} else if (p_vo_priv->push_state == HD_PATH_ID_ONGOING ||
			   p_vo_priv->pull_state == HD_PATH_ID_ONGOING) {
			HD_VIDEOOUT_ERR("videoout: Fail to open, control port is busy. state(%d,%d)\n",
					p_vo_priv->push_state, p_vo_priv->pull_state);
			ret = HD_ERR_STATE;
			goto ext;
		} else {
			p_vo_priv->push_state = HD_PATH_ID_ONGOING;
			p_vo_priv->pull_state = HD_PATH_ID_ONGOING;
		}

		//open control path
		ret = bd_device_open(self_id, ctrl_id, _in_id, p_path_id);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("bd_device_open fail\n");
			ret = HD_ERR_STATE;
			goto ext;
		}
		ret = videoout_open(lcd_id, ctrl_id, _in_id);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_open fail\n");
			ret = HD_ERR_STATE;
			goto ext;
		}
		p_vo_priv->push_state = HD_PATH_ID_OPEN;
		p_vo_priv->pull_state = HD_PATH_ID_OPEN;
		g_video_out_param[lcd_id].is_open = 1;
		//do nothing
		goto ext;
	}

	if ((_in_id <= HD_IN_MAX) && (lcd_id >= 0) && (lcd_id < HD_VIDEOOUT_MAX_DEVICE_ID)) {
		g_video_out_param[lcd_id].is_open = 1;
	}
	if (bd_get_dev_baseid(self_id) != HD_DAL_VIDEOOUT_BASE) {
		HD_VIDEOOUT_ERR(" The device of path_id(%#x) is not videoout.\n", *p_path_id);
		ret = HD_ERR_DEV;
		goto ext;
	}

	if (in_dev_id != self_id) {
		HD_VIDEOOUT_ERR(" in(%#x) and out(%#x) aren't the same device.\n", _in_id, _out_id);
		ret = HD_ERR_IO;
		goto ext;
	}
	if ((lcd_id < HD_VIDEOOUT_MAX_DEVICE_ID) && (lcd_id >= 0)) {
		if (_in_id <= HD_IN_MAX && _out_id <= HD_OUT_MAX) {
			/* check and set state */
			in_idx = VO_INID(_in_id);
			if (in_idx >= HD_VIDEOOUT_MAX_IN || in_idx < 0) {
				HD_VIDEOOUT_ERR("Check in_idx id(%d) fail, in_id(%#x)\n", in_idx, in_id);
				return HD_ERR_OVERRUN;
			}
			p_vo_priv = &g_video_out_param[lcd_id].priv[in_idx];
			if ((p_vo_priv->push_state == HD_PATH_ID_OPEN) ||
			    (p_vo_priv->pull_state == HD_PATH_ID_OPEN)) {
				if ((ret = bd_get_already_open_pathid(self_id, _out_id, _in_id, p_path_id)) != HD_OK) {
					goto ext;
				} else {
					ret = HD_ERR_ALREADY_OPEN;
					goto ext;
				}
			} else if (p_vo_priv->push_state == HD_PATH_ID_ONGOING ||
				   p_vo_priv->pull_state == HD_PATH_ID_ONGOING) {
				HD_VIDEOOUT_ERR("videoout: Fail to open, path_id(%#x) is busy. state(%d,%d)\n", BD_SET_PATH(self_id, _in_id, _out_id),
						p_vo_priv->push_state, p_vo_priv->pull_state);
				ret = HD_ERR_STATE;
				goto ext;
			} else {
				p_vo_priv->push_state = HD_PATH_ID_ONGOING;
				p_vo_priv->pull_state = HD_PATH_ID_ONGOING;
			}

			ret = bd_device_open(self_id, _out_id, _in_id, p_path_id);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("bd_device_open fail\n");
				ret = HD_ERR_STATE;
				goto ext;
			}
			ret = videoout_open(lcd_id, _out_id, _in_id);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("videoout_open fail\n");
				ret = HD_ERR_STATE;
				goto ext;
			}
			p_vo_priv->push_state = HD_PATH_ID_OPEN;
			p_vo_priv->pull_state = HD_PATH_ID_OPEN;
			HD_VIDEOOUT_FLOW("hd_videoout_open:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u)\n",
					 dif_return_dev_string(*p_path_id), *p_path_id, self_id, _out_id, _in_id);
		} else if (_out_id >= HD_STAMP_EX_BASE && _out_id < HD_MASK_EX_BASE) { //just creat pathid,for osg case
			*p_path_id = BD_SET_PATH(self_id, _in_id, _out_id);
			HD_OSG_FLOW("hd_videoout_open:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u)\n",
				    dif_return_dev_string(*p_path_id), *p_path_id, self_id, _out_id, _in_id);
		}  else if (_out_id >= HD_MASK_EX_BASE) { //just creat pathid,for osg case
			*p_path_id = BD_SET_PATH(self_id, _in_id, _out_id);
			HD_OSG_FLOW("hd_videoout_open:\n    %s path_id(%#x) devid(%#x) out(%u) in(%u)\n",
				    dif_return_dev_string(*p_path_id), *p_path_id, self_id, _out_id, _in_id);
		} else {
			HD_VIDEOOUT_ERR("Not support in_id=0%x out_id=0%x\n", _in_id, _out_id);
			ret = HD_ERR_IO;
		}
	} else if (lcd_id >= 3) {//just creat pathid for virtual videoout id
		*p_path_id = BD_SET_PATH(self_id, _in_id, _out_id);
	}
ext:
	return ret;
}


HD_RESULT hd_videoout_close(HD_PATH_ID path_id)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	VDDO_INFO_PRIV *p_vo_priv = NULL;
	INT lcd_id = 0;
	INT in_idx = 0;

	VIDEOOUT_CHECK_SELF_ID(self_id);
	lcd_id = VO_DEVID(self_id);
	if ((ctrl_id == HD_CTRL) && (lcd_id >= 0) && (lcd_id < HD_VIDEOOUT_MAX_DEVICE_ID)) {
		//do nothing
		/* check and set state */
		p_vo_priv = &g_video_out_param[lcd_id].priv_ctrl;
		if ((p_vo_priv->push_state == HD_PATH_ID_CLOSED) ||
		    (p_vo_priv->pull_state == HD_PATH_ID_CLOSED)) {
			ret = HD_ERR_NOT_OPEN;
			goto ext;
		} else if (p_vo_priv->push_state == HD_PATH_ID_ONGOING ||
			   p_vo_priv->pull_state == HD_PATH_ID_ONGOING) {
			HD_VIDEOOUT_ERR("videoout: Fail to close, control port is busy. state(%d,%d)\n",
					p_vo_priv->push_state, p_vo_priv->pull_state);
			ret = HD_ERR_STATE;
			goto ext;
		} else {
			p_vo_priv->push_state = HD_PATH_ID_CLOSED;
			p_vo_priv->pull_state = HD_PATH_ID_CLOSED;
		}
		ret = videoout_close(path_id);
		if (ret != HD_OK) {
			ret = HD_ERR_SYS;
			HD_VIDEOOUT_ERR("videoout ctrl: Fail to close.\n");
		}
		goto ext;
	}

	if ((lcd_id < HD_VIDEOOUT_MAX_DEVICE_ID) && (lcd_id >= 0)) {
		if (in_id <= HD_IN_MAX && out_id <= HD_OUT_MAX) {
			/* check and set state */
			in_idx = VO_INID(in_id);
			if (in_idx >= HD_VIDEOOUT_MAX_IN) {
				HD_VIDEOOUT_ERR("Check in_idx id(%d) fail, path_id(%#x)\n", in_idx, path_id);
				return HD_ERR_OVERRUN;
			}
			p_vo_priv = &g_video_out_param[lcd_id].priv[in_idx];
			if ((p_vo_priv->push_state == HD_PATH_ID_CLOSED) ||
			    (p_vo_priv->pull_state == HD_PATH_ID_CLOSED)) {
				ret = HD_ERR_NOT_OPEN;
				goto ext;
			} else if (p_vo_priv->push_state == HD_PATH_ID_ONGOING ||
				   p_vo_priv->pull_state == HD_PATH_ID_ONGOING) {
				HD_VIDEOOUT_ERR("Fail to close, path_id(%#x) is busy.  state(%d,%d)\n", path_id,
						p_vo_priv->push_state, p_vo_priv->pull_state);
				ret = HD_ERR_STATE;
				goto ext;
			} else {
				p_vo_priv->push_state = HD_PATH_ID_ONGOING;
				p_vo_priv->pull_state = HD_PATH_ID_ONGOING;
			}
			ret = bd_device_close(path_id);
			if (ret == HD_OK) {
				videoout_close(path_id);
			}
			p_vo_priv->push_state = HD_PATH_ID_CLOSED;
			p_vo_priv->pull_state = HD_PATH_ID_CLOSED;
			HD_VIDEOOUT_FLOW("hd_videoout_close:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		} else if ((out_id > HD_STAMP_EX_BASE) && (VO_STAMP_OUTID(out_id) >= 0 && \
				VO_STAMP_OUTID(out_id) < HD_VO_MAX_OSG_STAMP)) {
			//disable this pathid osg stamp
			out_id = VO_STAMP_OUTID(out_id);
			in_id = VO_INID(in_id);
			if (lcd_id >= HD_VIDEOOUT_MAX_DEVICE_ID) {
				HD_VIDEOOUT_ERR("Check self_id(%d) fail\n", lcd_id);
				goto ext;
			}
			if (GET_STAMP_ID(in_id, out_id) >= HD_MAXVO_TOTAL_STAMP_NU) {
				HD_VIDEOOUT_ERR("Check stamp id(%d) fail, max(%d)\n", GET_STAMP_ID(in_id, out_id), HD_MAXVO_TOTAL_STAMP_NU);
				return HD_ERR_OVERRUN;
			}
			if (g_video_out_param[lcd_id].stamp[GET_STAMP_ID(in_id, out_id)].win_enable) {
				g_video_out_param[lcd_id].stamp[GET_STAMP_ID(in_id, out_id)].win_enable = FALSE;
				videoout_set_osg_win(g_video_out_param[lcd_id].osg_idx, g_video_out_param[lcd_id].stamp, \
						     g_video_out_param[lcd_id].max_stamp_idx);
				videoout_update_stamp_cfg(out_id, in_id, &g_video_out_param[lcd_id], TRUE);
			}
			HD_OSG_FLOW("hd_videoout_close:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		} else if ((out_id >= HD_MASK_EX_BASE) && (VO_MASK_OUTID(out_id) >= 0 && \
				VO_MASK_OUTID(out_id) < HD_VO_MAX_MASK)) {
			//disable this pathid osg mask
			out_id = VO_MASK_OUTID(out_id);
			in_id = VO_INID(in_id);
			if (lcd_id > HD_VIDEOOUT_MAX_DEVICE_ID) {
				HD_VIDEOOUT_ERR("Check self_id(%d) fail\n", lcd_id);
				goto ext;
			}
			if (GET_MASK_ID(in_id, out_id) >= HD_MAXVO_TOTAL_MASK_NU) {
				HD_VIDEOOUT_ERR("Check mask id(%d) fail, max(%d)\n", GET_MASK_ID(in_id, out_id), HD_MAXVO_TOTAL_MASK_NU);
				return HD_ERR_OVERRUN;
			}
			if (g_video_out_param[lcd_id].mask_mosaic[GET_MASK_ID(in_id, out_id)].enable) {
				g_video_out_param[lcd_id].mask_mosaic[GET_MASK_ID(in_id, out_id)].enable = FALSE;
				videoout_set_mask_mosaic(g_video_out_param[lcd_id].osg_idx, g_video_out_param[lcd_id].mask_mosaic, \
							 g_video_out_param[lcd_id].max_mask_idx);
				videoout_update_mask_cfg(out_id, in_id, &g_video_out_param[lcd_id]);
			}
			HD_OSG_FLOW("hd_videoout_close:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		} else {
			HD_VIDEOOUT_ERR("Not support:in_id=0%x,out_id=0x%x,path_id=%#x\n", in_id, out_id, path_id);
			ret = HD_ERR_IO;
		}
	}

ext:
	return ret;
}

HD_RESULT hd_videoout_init(VOID)
{
	HD_RESULT ret;
	HD_VIDEOOUT_MODE *lcd_mode;
	INT i = 0;

	if (hd_videoout_max_device_nr) {
		HD_VIDEOOUT_ERR("video out is initted\n");
		ret = HD_ERR_NG;
		return ret;
	}
	if (!vo_lock_init) {
		vo_lock_init = 1;
		if (pthread_rwlock_init(&videoout_rwlock, NULL)) {
			HD_VIDEOOUT_ERR("init videoout_rwlock fail\n");
			ret = HD_ERR_NG;
			return ret;
		}
	} 
	hd_videoout_max_device_nr = HD_VIDEOOUT_MAX_DEVICE_ID;
	hd_videoout_max_device_out = HD_VIDEOOUT_MAX_OUT;
	hd_videoout_max_device_in = HD_VIDEOOUT_MAX_IN;
	ret = bd_device_init(HD_DAL_VIDEOOUT_BASE, hd_videoout_max_device_nr, hd_videoout_max_device_in, hd_videoout_max_device_out);
	if (ret != HD_OK) {
		HD_VIDEOOUT_ERR("bd_device_init error\n");
	}

	ret = videoout_init();

	if (ret != HD_OK) {
		HD_VIDEOOUT_ERR("videoout_init error\n");
		return ret;
	}
	if (videoout_get_lcd_dim(HD_VIDEOOUT_VDDO0, lcd_cfg[HD_VIDEOOUT_VDDO0].mode) == HD_OK) {
		lcd_mode = &g_video_out_param[HD_VIDEOOUT_VDDO0].mode;
		lcd_mode->output_type = lcd_cfg[HD_VIDEOOUT_VDDO0].mode->output_type;
		lcd_mode->input_dim = lcd_cfg[HD_VIDEOOUT_VDDO0].mode->input_dim;
		lcd_mode->output_mode.hdmi = lcd_cfg[HD_VIDEOOUT_VDDO0].mode->output_mode.hdmi;
	} else {
		lcd_mode = &g_video_out_param[HD_VIDEOOUT_VDDO0].mode;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1920x1080;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1920X1080P30;
	}
	if (videoout_get_lcd_dim(HD_VIDEOOUT_VDDO1, lcd_cfg[HD_VIDEOOUT_VDDO1].mode) == HD_OK) {
		lcd_mode = &g_video_out_param[HD_VIDEOOUT_VDDO1].mode;
		lcd_mode->output_type = lcd_cfg[HD_VIDEOOUT_VDDO1].mode->output_type;
		lcd_mode->input_dim = lcd_cfg[HD_VIDEOOUT_VDDO1].mode->input_dim;
		lcd_mode->output_mode.hdmi = lcd_cfg[HD_VIDEOOUT_VDDO1].mode->output_mode.cvbs;
	} else {
		lcd_mode = &g_video_out_param[HD_VIDEOOUT_VDDO1].mode;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_CVBS;
		lcd_mode->input_dim = HD_VIDEOOUT_IN_720x480;
		lcd_mode->output_mode.cvbs = HD_VIDEOOUT_CVBS_NTSC_SD;
	}

	/* Virtual LCD */
	for (i = MAX_VIDEOOUT_CTRL_ID; i < HD_VIDEOOUT_MAX_DEVICE_ID; i++) {
		videoout_get_lcd_dim(i, lcd_cfg[i].mode);
		lcd_mode = &g_video_out_param[i].mode;
		lcd_mode->output_type = lcd_cfg[i].mode->output_type; // HD_COMMON_VIDEO_OUT_LCD
		lcd_mode->input_dim = lcd_cfg[i].mode->input_dim;     // HD_VIDEOOUT_IN_AUTO
		lcd_mode->output_mode.lcd = lcd_cfg[i].mode->output_mode.lcd;
	}

	HD_VIDEOOUT_FLOW("hd_videoout_init\n");
	return ret;
}

HD_RESULT hd_videoout_uninit(VOID)
{
	HD_RESULT ret = HD_OK;

	hd_videoout_max_device_nr = 0;
	hd_videoout_max_device_out = 0;
	hd_videoout_max_device_in = 0;
	if (vo_lock_init) {
		pthread_rwlock_destroy(&videoout_rwlock);
		vo_lock_init = 0;
	}
	ret = bd_device_uninit(HD_DAL_VIDEOOUT_BASE);
	videoout_uninit();
	HD_VIDEOOUT_FLOW("hd_videoout_uninit\n");
	return ret;
}

HD_RESULT hd_videoout_set(HD_PATH_ID path_id, HD_VIDEOOUT_PARAM_ID id, void *param)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_VIDEOOUT_WIN_ATTR *usr_win;
	HD_VIDEOOUT_WIN_ATTR *win;
	HD_VIDEOOUT_WIN_PSR_ATTR *usr_win_psr;
	HD_VIDEOOUT_WIN_PSR_ATTR *win_psr;
	HD_VIDEOOUT_MODE *mode, *usr_mode;
	HD_VDDO_MASK_MOSAIC *p_mask_mosaic = NULL;
	HD_OSG_MASK_ATTR *p_mask_param;
	HD_OSG_MOSAIC_ATTR *p_mosaic_param = NULL;
	HD_VIDEOOUT_DEV_CONFIG *p_vdo_dev_cfg = NULL;
	HD_VIDEOOUT_DEV_CONFIG *user_dev_cfg = NULL;
	UINT8 in_idx;
	UINT32 lcd_w, lcd_h;
	HD_VIDEOOUT_CLEAR_WIN clear_win;
	HD_FB_FMT *usr_fb_fmt;
	HD_FB_DIM *usr_fb_dim;
	HD_FB_ATTR *usr_fb_attr;
	FB_PARAM *p_fb_param;
	HD_VDDO_WIN_PARAM *p_stamp;
	UINTPTR buf_addr = 0;
	HD_COMMON_MEM_VIRT_INFO usr_va_info;
	INT lcd_id = 0;

#if ENABLE_LOAD_PALETTE_TLB
	HD_VIDEOOUT_PALETTE_TABLE *palette_tlb;
#endif
	HD_RESULT ret = HD_OK;
	INT i;
	//HD_VIDEOOUT_PARAM_DEV_CONFIG setting in lcd driver not ready status,so videoout can't be open
	if (HD_VIDEOOUT_PARAM_DEV_CONFIG != id) {
		if (!hd_videoout_max_device_nr) {
			HD_VIDEOOUT_ERR("video out not init\n");
		}

		VIDEOOUT_CHECK_SELF_ID(self_id);
		VIDEOOUT_CHECK_MODULE_OPEN(self_id);
	}
	lcd_id = VO_DEVID(self_id);
	if (lcd_id >= HD_VIDEOOUT_MAX_DEVICE_ID || lcd_id < 0) {
		HD_VIDEOOUT_ERR("Check lcd_id(%d) fail,pathid(%#x)", lcd_id, path_id);
		return HD_ERR_NG;
	}
	if (id == HD_VIDEOOUT_PARAM_DEV_CONFIG) {
		if (videoout_dev_exist(lcd_id) == HD_OK) {
			HD_VIDEOOUT_ERR("videoout set HD_VIDEOOUT_PARAM_DEV_CONFIG already\n");
			return HD_ERR_NG;
		}
	} else {
		if (videoout_dev_exist(lcd_id) != HD_OK) {
			HD_VIDEOOUT_ERR("videoout need set HD_VIDEOOUT_PARAM_DEV_CONFIG, before set other paramid\n");
			return HD_ERR_NG;
		}
	}
	switch (id) {
	case HD_VIDEOOUT_PARAM_PALETTE_TABLE:
#if ENABLE_LOAD_PALETTE_TLB
		palette_tlb = (HD_VIDEOOUT_PALETTE_TABLE *)param;
		ret = videoout_load_palette_p(lcd_id, palette_tlb);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_load_palette_p fail\n");
		}
#endif
		break;
	case HD_VIDEOOUT_PARAM_DEV_CONFIG:
		p_vdo_dev_cfg = &g_video_out_param[lcd_id].dev_cfg;
		user_dev_cfg = (HD_VIDEOOUT_DEV_CONFIG *)param;
		p_vdo_dev_cfg->devnvr_cfg.homology = user_dev_cfg->devnvr_cfg.homology;
		p_vdo_dev_cfg->devnvr_cfg.chip_state = user_dev_cfg->devnvr_cfg.chip_state;
		p_vdo_dev_cfg->devnvr_cfg.mode = user_dev_cfg->devnvr_cfg.mode;
		g_video_out_param[lcd_id].mode = p_vdo_dev_cfg->devnvr_cfg.mode;
		lcd_cfg[lcd_id].mode = &g_video_out_param[lcd_id].mode;
		p_vdo_dev_cfg->devnvr_cfg.plane[0] = user_dev_cfg->devnvr_cfg.plane[0];
		p_vdo_dev_cfg->devnvr_cfg.plane[1] = user_dev_cfg->devnvr_cfg.plane[1];
		p_vdo_dev_cfg->devnvr_cfg.plane[2] = user_dev_cfg->devnvr_cfg.plane[2];

		ret = videoout_set_dev_cfg_p(lcd_id, p_vdo_dev_cfg);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_set_dev_cfg_p fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_set(DEV_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        homo(%d) state(%d)\n"
				 "        input_dim(%#x) output_type(%#x) lcd(%#x) hdmi(%#x) vga(%#x) cvbs(%#x)\n",
				 dif_return_dev_string(path_id), path_id, user_dev_cfg->devnvr_cfg.homology,
				 user_dev_cfg->devnvr_cfg.chip_state,
				 user_dev_cfg->devnvr_cfg.mode.input_dim,
				 user_dev_cfg->devnvr_cfg.mode.output_type,
				 user_dev_cfg->devnvr_cfg.mode.output_mode.lcd,
				 user_dev_cfg->devnvr_cfg.mode.output_mode.hdmi,
				 user_dev_cfg->devnvr_cfg.mode.output_mode.vga,
				 user_dev_cfg->devnvr_cfg.mode.output_mode.cvbs);
		for (i = 0; i < 3; i++) {
			HD_VIDEOOUT_FLOW("        idx(%d) max_dim(%d,%d) bpp(%d) rld_en(%d) ddr(%d)\n"
					 "        buf_pa(%#lx) buf_len(%u) rle_pa(%#lx) rle_len(%u)\n",
					 i, user_dev_cfg->devnvr_cfg.plane[i].max_w,
					 user_dev_cfg->devnvr_cfg.plane[i].max_h,
					 user_dev_cfg->devnvr_cfg.plane[i].max_bpp,
					 user_dev_cfg->devnvr_cfg.plane[i].gui_rld_enable,
					 user_dev_cfg->devnvr_cfg.plane[i].ddr_id,
					 user_dev_cfg->devnvr_cfg.plane[i].buf_paddr,
					 user_dev_cfg->devnvr_cfg.plane[i].buf_len,
					 user_dev_cfg->devnvr_cfg.plane[i].rle_buf_paddr,
					 user_dev_cfg->devnvr_cfg.plane[i].rle_buf_len);
		}
		break;
	case HD_VIDEOOUT_PARAM_IN_WIN_ATTR:
		VIDEOOUT_CHECK_IN_ID(in_id);

		in_idx = VO_INID(in_id);
		win = &g_video_out_param[lcd_id].win[in_idx];
		usr_win = (HD_VIDEOOUT_WIN_ATTR *)param;

		if (g_video_out_param[lcd_id].mode.output_type == HD_COMMON_VIDEO_OUT_LCD) {
			/* virtual LCD */
		} else {
			get_lcd_dim_by_value(g_video_out_param[lcd_id].mode.input_dim, &lcd_w, &lcd_h);
			if (usr_win->visible && (((usr_win->rect.x + usr_win->rect.w) > lcd_w) || \
									 ((usr_win->rect.y + usr_win->rect.h) > lcd_h))) {
				HD_VIDEOOUT_ERR("%s path_id(%#x), Check lcd_%d input win boundary error(x+w=%d>lcd_w=%d, y+h=%d>lcd_h=%d)\n",
								dif_return_dev_string(path_id), path_id, lcd_id, (usr_win->rect.x + usr_win->rect.w), 
								lcd_w, (usr_win->rect.y + usr_win->rect.h), lcd_h);
				return HD_ERR_LIMIT;
			}
		}
		if (usr_win->visible && ((usr_win->rect.w == 0) || (usr_win->rect.h == 0))) {
			HD_VIDEOOUT_ERR("%s path_id(%#x), Error HD_VIDEOOUT_PARAM_IN_WIN_ATTR %dx%d value\n",
							dif_return_dev_string(path_id), path_id, usr_win->rect.w, usr_win->rect.h);
			return HD_ERR_LIMIT;
		}
		if (usr_win->layer >= HD_WIN_LAYER_MAX) {
			usr_win->layer = 0;
		}
		win->visible = usr_win->visible;
		win->rect.x = usr_win->rect.x;
		win->rect.y = usr_win->rect.y;
		win->rect.w = usr_win->rect.w;
		win->rect.h = usr_win->rect.h;
		win->layer = usr_win->layer;

		ret = videoout_set_win_p(lcd_id, in_idx, win);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_set_win_p fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_set(IN_WIN_ATTR):\n"
				 "    %s path_id(%#x)\n"
				 "        visible(%#x) rect(%u,%u,%u,%u) layer(%#x)\n",
				 dif_return_dev_string(path_id), path_id, usr_win->visible,
				 usr_win->rect.x,
				 usr_win->rect.y,
				 usr_win->rect.w,
				 usr_win->rect.h,
				 usr_win->layer);
		break;
	case HD_VIDEOOUT_PARAM_IN_WIN_PSR_ATTR:
		VIDEOOUT_CHECK_IN_ID(in_id);

		in_idx = VO_INID(in_id);
		win_psr = &g_video_out_param[lcd_id].win_psr[in_idx];
		usr_win_psr = (HD_VIDEOOUT_WIN_PSR_ATTR *)param;

		if (g_video_out_param[lcd_id].mode.output_type == HD_COMMON_VIDEO_OUT_LCD) {
			/* virtual LCD */
		} else {
			get_lcd_dim_by_value(g_video_out_param[lcd_id].mode.input_dim, &lcd_w, &lcd_h);
			if (usr_win_psr->visible && (((usr_win_psr->rect.x + usr_win_psr->rect.w) > lcd_w) || \
										 ((usr_win_psr->rect.y + usr_win_psr->rect.h) > lcd_h))) {
				HD_VIDEOOUT_ERR("Check input win-psr boundary error(x+w = %d, y+h=%d)\n",
								(usr_win_psr->rect.x + usr_win_psr->rect.w),
								(usr_win_psr->rect.y + usr_win_psr->rect.h));
				return HD_ERR_NG;
			}
		}

		win_psr->visible = usr_win_psr->visible;
		win_psr->rect.x = usr_win_psr->rect.x;
		win_psr->rect.y = usr_win_psr->rect.y;
		win_psr->rect.w = usr_win_psr->rect.w;
		win_psr->rect.h = usr_win_psr->rect.h;

		ret = videoout_set_win_psr_p(lcd_id, in_idx, win_psr);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_set_win_psr_p fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_set(IN_WIN_PSR_ATTR):\n"
				 "    %s path_id(%#x)\n"
				 "        visible(%#x) rect(%u,%u,%u,%u)\n",
				 dif_return_dev_string(path_id), path_id, usr_win_psr->visible,
				 usr_win_psr->rect.x,
				 usr_win_psr->rect.y,
				 usr_win_psr->rect.w,
				 usr_win_psr->rect.h);
		break;
	case HD_VIDEOOUT_PARAM_MODE:
		if (ctrl_id != HD_CTRL) {
			HD_VIDEOOUT_ERR("Please use control port to set HD_VIDEOOUT_PARAM_MODE\n");
			ret = HD_ERR_NOT_SUPPORT;
			goto err_ext;
		}
		pthread_rwlock_wrlock(&videoout_rwlock);
		usr_mode = (HD_VIDEOOUT_MODE *)param;
		mode = &g_video_out_param[lcd_id].mode;
		lcd_cfg[lcd_id].mode = &g_video_out_param[lcd_id].mode;
		ret = videoout_set_mode(lcd_id, usr_mode, mode);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_set_mode fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_set(MODE):\n"
				 "    %s path_id(%#x)\n"
				 "        input_dim(%#x) output_type(%#x)\n"
				 "        lcd(%#x) hdmi(%#x) vga(%#x) cvbs(%#x)\n",
				 dif_return_dev_string(path_id), path_id, usr_mode->input_dim,
				 usr_mode->output_type,
				 usr_mode->output_mode.lcd,
				 usr_mode->output_mode.hdmi,
				 usr_mode->output_mode.vga,
				 usr_mode->output_mode.cvbs);
		pthread_rwlock_unlock(&videoout_rwlock);
		break;
	case HD_VIDEOOUT_PARAM_CLEAR_WIN:
		if (param == NULL) {
			ret = HD_ERR_NG;
			goto err_ext;
		}
		//VIDEOOUT_CHECK_OUT_ID(out_id);
		memcpy(&clear_win, param, sizeof(HD_VIDEOOUT_CLEAR_WIN));
		if (clear_win.in_fmt != HD_VIDEO_PXLFMT_YUV422_ONE && clear_win.in_fmt != HD_VIDEO_PXLFMT_YUV420) {
			HD_VIDEOOUT_ERR("Clear win only support in_fmt = HD_VIDEO_PXLFMT_YUV422_ONE and HD_VIDEO_PXLFMT_YUV420\n");
			clear_win.in_fmt = HD_VIDEO_PXLFMT_YUV422_ONE;
		}

		if (clear_win.input_dim.w < 64) {
			ret = HD_ERR_NG;
			HD_VIDEOOUT_ERR("clear_window: minimum input requirement 64x32\n");
			goto err_ext;
		}
		if (clear_win.input_dim.h < 32) {
			ret = HD_ERR_NG;
			HD_VIDEOOUT_ERR("clear_window: minimum input requirement 64x32\n");
			goto err_ext;
		}
		if (clear_win.output_rect.w <= 0) {
			ret = HD_ERR_NG;
			goto err_ext;
		}
		if (clear_win.output_rect.h <= 0) {
			ret = HD_ERR_NG;
			goto err_ext;
		}

		if ((clear_win.output_rect.w / clear_win.input_dim.w) > 8) {
			ret = HD_ERR_NOT_SUPPORT;
			HD_VIDEOOUT_ERR("clear win out_w=%d, in_w=%d over vpe max scling up factor(8)\n", clear_win.output_rect.w,
					clear_win.input_dim.w);
			goto err_ext;
		}

		usr_va_info.cached = 0;
		usr_va_info.va = (void *)clear_win.buf;
		if (pif_get_usr_va_info((HD_COMMON_MEM_VIRT_INFO *)&usr_va_info) != 0) {
			HD_VIDEOOUT_ERR();
			ret = HD_ERR_NG;
			goto err_ext;
		}
		if (pif_address_sanity_check(usr_va_info.pa) < 0) {
			HD_VIDEOOUT_ERR("%s:Check clearwin src va(%#lx) pa(%#lx) error\n", __func__, (UINTPTR)clear_win.buf, usr_va_info.pa);
			ret = HD_ERR_PARAM;
			goto err_ext;
		}
		pthread_rwlock_rdlock(&videoout_rwlock);
		ret = videoout_clearwin_p(lcd_id, &clear_win);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_clearwin_p fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_set(CLEAR_WIN):\n"
				 "    %s path_id(%#x)\n"
				 "        input_dim(%u,%u) pxlfmt(%#x) buf_addr(%p) buf_data(%#x)\n"
				 "        rect(%u,%u,%u,%u) mode(%#x)\n",
				 dif_return_dev_string(path_id), path_id, clear_win.input_dim.w, clear_win.input_dim.h,
				 clear_win.in_fmt, clear_win.buf, *(unsigned int *)clear_win.buf,
				 clear_win.output_rect.x, clear_win.output_rect.y,
				 clear_win.output_rect.w, clear_win.output_rect.h,
				 clear_win.mode);
		pthread_rwlock_unlock(&videoout_rwlock);
		break;
	case HD_VIDEOOUT_PARAM_FB_FMT:
		usr_fb_fmt = (HD_FB_FMT *)param;
		p_fb_param = &g_video_out_param[lcd_id].fb_param[usr_fb_fmt->fb_id];
		p_fb_param->fmt = usr_fb_fmt->fmt;
		ret = videoout_set_fb_fmt(usr_fb_fmt->fb_id, lcd_id, usr_fb_fmt->fmt);
		HD_VIDEOOUT_FLOW("hd_videoout_set(FB_FMT):\n"
				 "    %s path_id(%#x)\n"
				 "        fb_id(%#x) pxlfmt(%#x)\n",
				 dif_return_dev_string(path_id), path_id, usr_fb_fmt->fb_id, usr_fb_fmt->fmt);
		break;
	case HD_VIDEOOUT_PARAM_FB_ATTR:
		usr_fb_attr = (HD_FB_ATTR *)param;
		p_fb_param = &g_video_out_param[lcd_id].fb_param[usr_fb_attr->fb_id];
		p_fb_param->alpha_1555 = usr_fb_attr->alpha_1555;
		p_fb_param->alpha_blend = usr_fb_attr->alpha_blend;
		p_fb_param->colorkey_en = usr_fb_attr->colorkey_en;
		p_fb_param->r_ckey = usr_fb_attr->r_ckey;
		p_fb_param->g_ckey = usr_fb_attr->g_ckey;
		p_fb_param->b_ckey = usr_fb_attr->b_ckey;
		ret = videoout_set_fb_attr(usr_fb_attr->fb_id, lcd_id, p_fb_param);
		HD_VIDEOOUT_FLOW("hd_videoout_set(FB_ATTR):\n"
				 "    %s path_id(%#x) fb_id(%d)\n"
				 "        alpha_blend(%d) alpha_1555(%d)\n"
				 "        colorkey_en(%d) r_ckey(%d) g_ckey(%d) b_ckey(%d)\n",
				 dif_return_dev_string(path_id), path_id, usr_fb_attr->fb_id, p_fb_param->alpha_blend,
				 p_fb_param->alpha_1555, p_fb_param->colorkey_en,
				 p_fb_param->r_ckey, p_fb_param->g_ckey, p_fb_param->b_ckey);
		break;
	case HD_VIDEOOUT_PARAM_FB_DIM:
		usr_fb_dim = (HD_FB_DIM *)param;
		p_fb_param = &g_video_out_param[lcd_id].fb_param[usr_fb_dim->fb_id];
		if (videoout_check_fb_dim(usr_fb_dim->fb_id, lcd_id, usr_fb_dim->input_dim, usr_fb_dim->output_rect) != HD_OK) {
			HD_VIDEOOUT_ERR("hd_videoout_set(FB_DIM): Check error to set %s path_id(0x%x)\n",
					dif_return_dev_string(path_id), (int)path_id);
			ret = HD_ERR_NG;
			goto err_ext;
		}
		p_fb_param->input_dim.w = usr_fb_dim->input_dim.w;
		p_fb_param->input_dim.h = usr_fb_dim->input_dim.h;
		p_fb_param->output_rect.x = usr_fb_dim->output_rect.x;
		p_fb_param->output_rect.y = usr_fb_dim->output_rect.y;
		p_fb_param->output_rect.w = usr_fb_dim->output_rect.w;
		p_fb_param->output_rect.h = usr_fb_dim->output_rect.h;
		if (videoout_set_fb_dim(usr_fb_dim->fb_id, lcd_id, p_fb_param->input_dim, p_fb_param->output_rect) != HD_OK) {
			HD_VIDEOOUT_ERR("hd_videoout_set(FB_DIM): Fail to set path_id(0x%x)\n", (int)path_id);
			ret = HD_ERR_NG;
			goto err_ext;
		}
		HD_VIDEOOUT_FLOW("hd_videoout_set(FB_DIM):\n"
				 "    %s path_id(%#x)\n"
				 "        fb_id(%#x) in_dim(%dx%d) out_rect(%d %d %d %d)\n",
				 dif_return_dev_string(path_id), path_id, usr_fb_dim->fb_id, usr_fb_dim->input_dim.w, usr_fb_dim->input_dim.h,
				 usr_fb_dim->output_rect.x, usr_fb_dim->output_rect.y, usr_fb_dim->output_rect.w,
				 usr_fb_dim->output_rect.h);
		break;
	case HD_VIDEOOUT_PARAM_OUT_STAMP_IMG: {
		out_id = VO_STAMP_OUTID(out_id);
		in_id = VO_INID(in_id);
		if (GET_STAMP_ID(in_id, out_id) >= HD_MAXVO_TOTAL_STAMP_NU) {
			HD_VIDEOOUT_ERR("Check stamp id(%d) fail, max(%d)\n", GET_STAMP_ID(in_id, out_id), HD_MAXVO_TOTAL_STAMP_NU);
			return HD_ERR_OVERRUN;
		}
		g_video_out_param[lcd_id].osg_idx = lcd_id;
		p_stamp = &g_video_out_param[lcd_id].stamp[GET_STAMP_ID(in_id, out_id)];
		buf_addr = ((HD_OSG_STAMP_IMG *)param)->p_addr;
		p_stamp->img.fmt = ((HD_OSG_STAMP_IMG *)param)->fmt;
		p_stamp->img.dim.w = ((HD_OSG_STAMP_IMG *)param)->dim.w;
		p_stamp->img.dim.h = ((HD_OSG_STAMP_IMG *)param)->dim.h;
		p_stamp->img.ddr_id = ((HD_OSG_STAMP_IMG *)param)->ddr_id;
		p_stamp->img.p_addr = buf_addr;
		if (pif_address_ddr_sanity_check(p_stamp->img.p_addr, p_stamp->img.ddr_id) < 0) {
			HD_VIDEOOUT_ERR("%s:Check videoout osg stamp pa(%#lx) ddrid(%d) error\n", __func__, p_stamp->img.p_addr, p_stamp->img.ddr_id);
			ret = HD_ERR_PARAM;
			goto err_ext;
		}
		HD_OSG_FLOW("hd_videoout_set(OUT_STAMP_IMG):\n"
			    "    %s path_id(%#x)\n"
			    "        pxlfmt(%#x) dim(%u,%u) ddr(%u) pa(%p)\n",
			    dif_return_dev_string(path_id), path_id, p_stamp->img.fmt,
			    p_stamp->img.dim.w,
			    p_stamp->img.dim.h,
			    p_stamp->img.ddr_id,
			    (VOID *)p_stamp->img.p_addr);
		break;
	}
	case HD_VIDEOOUT_PARAM_OUT_STAMP_ATTR: {
		out_id = VO_STAMP_OUTID(out_id);
		in_id = VO_INID(in_id);
		if (GET_STAMP_ID(in_id, out_id) > g_video_out_param[lcd_id].max_stamp_idx) {
			g_video_out_param[lcd_id].max_stamp_idx = GET_STAMP_ID(in_id, out_id);
		}
		g_video_out_param[lcd_id].osg_idx = lcd_id;
		p_stamp = &g_video_out_param[lcd_id].stamp[GET_STAMP_ID(in_id, out_id)];

		if (HD_OSG_ALIGN_TYPE_TOP_LEFT != p_stamp->attr.align_type) {
			p_stamp->attr.align_type = HD_OSG_ALIGN_TYPE_TOP_LEFT;
			HD_VIDEOOUT_ERR("osg only suport align_type=%d\n", HD_OSG_ALIGN_TYPE_TOP_LEFT);
		}
		p_stamp->attr.align_type = ((HD_OSG_STAMP_ATTR *)param)->align_type;
		p_stamp->attr.alpha = ((HD_OSG_STAMP_ATTR *)param)->alpha;
		p_stamp->attr.position.x = ((HD_OSG_STAMP_ATTR *)param)->position.x;
		p_stamp->attr.position.y = ((HD_OSG_STAMP_ATTR *)param)->position.y;
		p_stamp->attr.gcac_enable = ((HD_OSG_STAMP_ATTR *)param)->gcac_enable;
		p_stamp->attr.gcac_blk_width = ((HD_OSG_STAMP_ATTR *)param)->gcac_blk_width;
		p_stamp->attr.gcac_blk_height = ((HD_OSG_STAMP_ATTR *)param)->gcac_blk_height;
		HD_OSG_FLOW("hd_videoout_set(OUT_STAMP_ATTR):\n"
			    "    %s path_id(%#x)\n"
			    "        type(%#x) alpha(%u) pos(%u,%u)\n"
			    "        gcac_en(%u) gcac_w(%u) gcac_h(%u)\n",
			    dif_return_dev_string(path_id), path_id, p_stamp->attr.align_type,
			    p_stamp->attr.alpha,
			    p_stamp->attr.position.x,
			    p_stamp->attr.position.y,
			    p_stamp->attr.gcac_enable,
			    p_stamp->attr.gcac_blk_width,
			    p_stamp->attr.gcac_blk_height);
		break;
	}
	case HD_VIDEOOUT_PARAM_OUT_MASK_ATTR: {
		unsigned int lcd_width, lcd_height;
		out_id = VO_MASK_OUTID(out_id);
		in_id = VO_INID(in_id);
		g_video_out_param[lcd_id].osg_idx = lcd_id;
		p_mask_mosaic = &g_video_out_param[lcd_id].mask_mosaic[GET_MASK_ID(in_id, out_id)];
		p_mask_param = (HD_OSG_MASK_ATTR *)param;
		VDOEOOUT_CHECK_H_POSIT(p_mask_param->position[0], p_mask_param->position[1]);
		VDOEOOUT_CHECK_V_POSIT(p_mask_param->position[1], p_mask_param->position[2]);
		VDOEOOUT_CHECK_H_POSIT(p_mask_param->position[3], p_mask_param->position[2]);
		VDOEOOUT_CHECK_V_POSIT(p_mask_param->position[0], p_mask_param->position[3]);
		if ((p_mask_param->position[1].x - p_mask_param->position[0].x) < 8) {
			HD_VIDEOOUT_ERR("mask width must > 8\n");
			return HD_ERR_NOT_SUPPORT;
		}
		if ((p_mask_param->position[2].y - p_mask_param->position[1].y) < 8) {
			HD_VIDEOOUT_ERR("mask height must > 8\n");
			return HD_ERR_NOT_SUPPORT;
		}
		lcd_width = platform_sys_Info.lcd_info[lcd_id].fb0_win.width;
		lcd_height = platform_sys_Info.lcd_info[lcd_id].fb0_win.height;
		if ((lcd_width > 64 && lcd_height > 64) && \
		    ((p_mask_param->position[2].x > lcd_width) || (p_mask_param->position[2].y > lcd_height))) {
			HD_VIDEOOUT_ERR("Check mask x_y(%dx%d) > videoout_dim(%dx%d)\n", p_mask_param->position[2].x, \
					p_mask_param->position[2].y, lcd_width, lcd_height);
			return HD_ERR_NG;
		}
		p_mask_mosaic->is_mask = 1;
		p_mask_mosaic->mask_cfg.alpha = p_mask_param->alpha;
		p_mask_mosaic->mask_cfg.color = p_mask_param->color;
		p_mask_mosaic->mask_cfg.type = p_mask_param->type;
        p_mask_mosaic->mask_cfg.thickness = p_mask_param->thickness;
		p_mask_mosaic->mask_cfg.position[0].x = p_mask_param->position[0].x;
		p_mask_mosaic->mask_cfg.position[0].y = p_mask_param->position[0].y;
		p_mask_mosaic->mask_cfg.position[1].x = p_mask_param->position[1].x;
		p_mask_mosaic->mask_cfg.position[1].y = p_mask_param->position[1].y;
		p_mask_mosaic->mask_cfg.position[2].x = p_mask_param->position[2].x;
		p_mask_mosaic->mask_cfg.position[2].y = p_mask_param->position[2].y;
		p_mask_mosaic->mask_cfg.position[3].x = p_mask_param->position[3].x;
		p_mask_mosaic->mask_cfg.position[3].y = p_mask_param->position[3].y;
		HD_OSG_FLOW("hd_videoout_set(OUT_MASK_ATTR):\n"
			    "    %s path_id(%#x)\n"
			    "        type(%#x) alpha(%u) color(%u) thickness(%d)\n"
			    "        pos0(%u,%u) pos1(%u,%u) pos2(%u,%u) pos3(%u,%u) \n",
			    dif_return_dev_string(path_id), path_id, p_mask_param->type,
			    p_mask_param->alpha, p_mask_param->color,p_mask_param->thickness,
			    p_mask_param->position[0].x, p_mask_param->position[0].y,
			    p_mask_param->position[1].x, p_mask_param->position[1].y,
			    p_mask_param->position[2].x, p_mask_param->position[2].y,
			    p_mask_param->position[3].x, p_mask_param->position[3].y);
		break;
	}
	case HD_VIDEOOUT_PARAM_OUT_MOSAIC_ATTR: {
		unsigned int lcd_width, lcd_height;
		out_id = VO_MASK_OUTID(out_id);
		in_id = VO_INID(in_id);
		g_video_out_param[lcd_id].osg_idx = lcd_id;
		p_mask_mosaic = &g_video_out_param[lcd_id].mask_mosaic[GET_MASK_ID(in_id, out_id)];
		p_mosaic_param = (HD_OSG_MOSAIC_ATTR *)param;
		VDOEOOUT_CHECK_H_POSIT(p_mosaic_param->position[0], p_mosaic_param->position[1]);
		VDOEOOUT_CHECK_V_POSIT(p_mosaic_param->position[1], p_mosaic_param->position[2]);
		VDOEOOUT_CHECK_H_POSIT(p_mosaic_param->position[3], p_mosaic_param->position[2]);
		VDOEOOUT_CHECK_V_POSIT(p_mosaic_param->position[0], p_mosaic_param->position[3]);
		if ((p_mosaic_param->position[1].x - p_mosaic_param->position[0].x) < 8) {
			HD_VIDEOOUT_ERR("mosaic width must > 8\n");
			return HD_ERR_NOT_SUPPORT;
		}
		if ((p_mosaic_param->position[2].y - p_mosaic_param->position[1].y) < 8) {
			HD_VIDEOOUT_ERR("mosaic height must > 8\n");
			return HD_ERR_NOT_SUPPORT;
		}
		lcd_width = platform_sys_Info.lcd_info[lcd_id].fb0_win.width;
		lcd_height = platform_sys_Info.lcd_info[lcd_id].fb0_win.height;
		if ((lcd_width > 64 && lcd_height > 64) && \
		    ((p_mosaic_param->position[2].x > lcd_width) || p_mosaic_param->position[2].y > lcd_height)) {
			HD_VIDEOOUT_ERR("Check mask x_y(%dx%d) > videoout_dim(%dx%d)\n", p_mosaic_param->position[2].x, \
					p_mosaic_param->position[2].y, lcd_width, lcd_height);
			return HD_ERR_NG;
		}
		if ((p_mosaic_param->type != HD_OSG_MASK_TYPE_SOLID) &&
		    (p_mosaic_param->type != HD_OSG_MASK_TYPE_INVERSE)) {
			HD_VIDEOOUT_ERR("mosaic  type 0x%X not supported\n", p_mosaic_param->type);
			return HD_ERR_NOT_SUPPORT;
		}
		p_mask_mosaic->is_mask = 0;
		p_mask_mosaic->mosaic_cfg.alpha = p_mosaic_param->alpha;
		p_mask_mosaic->mosaic_cfg.type = p_mosaic_param->type;
		p_mask_mosaic->mosaic_cfg.mosaic_blk_w = p_mosaic_param->mosaic_blk_w;
		p_mask_mosaic->mosaic_cfg.mosaic_blk_h = p_mosaic_param->mosaic_blk_h;
		p_mask_mosaic->mosaic_cfg.position[0].x = ALIGN2_DOWN(p_mosaic_param->position[0].x);
		p_mask_mosaic->mosaic_cfg.position[0].y = ALIGN2_DOWN(p_mosaic_param->position[0].y);
		p_mask_mosaic->mosaic_cfg.position[1].x = ALIGN2_DOWN(p_mosaic_param->position[1].x);
		p_mask_mosaic->mosaic_cfg.position[1].y = ALIGN2_DOWN(p_mosaic_param->position[1].y);
		p_mask_mosaic->mosaic_cfg.position[2].x = ALIGN2_DOWN(p_mosaic_param->position[2].x);
		p_mask_mosaic->mosaic_cfg.position[2].y = ALIGN2_DOWN(p_mosaic_param->position[2].y);
		p_mask_mosaic->mosaic_cfg.position[3].x = ALIGN2_DOWN(p_mosaic_param->position[3].x);
		p_mask_mosaic->mosaic_cfg.position[3].y = ALIGN2_DOWN(p_mosaic_param->position[3].y);
		HD_OSG_FLOW("hd_videoout_set(OUT_MOSAIC_ATTR):\n"
			    "    %s, path_id(%#x)\n"
			    "        type(%#x) alpha(%u) blk_wh(%u,%u)\n"
			    "        pos0(%u,%u) pos1(%u,%u) pos2(%u,%u) pos3(%u,%u)\n",
			    dif_return_dev_string(path_id), path_id, p_mask_mosaic->mosaic_cfg.type, p_mask_mosaic->mosaic_cfg.alpha,
			    p_mask_mosaic->mosaic_cfg.mosaic_blk_w, p_mask_mosaic->mosaic_cfg.mosaic_blk_h,
			    p_mask_mosaic->mosaic_cfg.position[0].x, p_mask_mosaic->mosaic_cfg.position[0].y,
			    p_mask_mosaic->mosaic_cfg.position[1].x, p_mask_mosaic->mosaic_cfg.position[1].y,
			    p_mask_mosaic->mosaic_cfg.position[2].x, p_mask_mosaic->mosaic_cfg.position[2].y,
			    p_mask_mosaic->mosaic_cfg.position[3].x, p_mask_mosaic->mosaic_cfg.position[3].y);
		break;
	}
	case HD_VIDEOOUT_PARAM_FB_ENABLE: {
		if (ctrl_id != HD_CTRL) {
			HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_FB_ENABLE only support control pathid");
		}
		HD_FB_ENABLE *fb_enable = (HD_FB_ENABLE *)param;
		UINT16 fb_id = fb_enable->fb_id;
		UINT16 fb_status = fb_enable->enable;
		g_video_out_param[lcd_id].fb_param[fb_id].enable = fb_status;
		ret = videoout_set_fb_enable_p(fb_id, lcd_id, &g_video_out_param[lcd_id].fb_param[fb_id]);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_set_fb_enable_p fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_set(FB_ENABLE):\n"
				 "    %s path_id(%#x)\n"
				 "        fb_id(%#x) en(%u)\n",
				 dif_return_dev_string(path_id), path_id, fb_enable->fb_id, fb_enable->enable);
		break;
	}
	case HD_VIDEOOUT_PARAM_OUT_PALETTE_TABLE: {
		HD_PALETTE_TBL *usr_param = (HD_PALETTE_TBL *)param;
		if ((out_id > HD_STAMP_EX_BASE) && (VO_STAMP_OUTID(out_id) >= 0 && VO_STAMP_OUTID(out_id) < HD_VO_MAX_OSG_STAMP)) {
			ret = videoout_set_osg_stamp_palette_tbl(usr_param->p_table, usr_param->table_size);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("videoout_set_osg_stamp_palette_tbl fail\n");
			}
		} else if ((out_id > HD_MASK_EX_BASE) && (VO_MASK_OUTID(out_id) >= 0 && VO_MASK_OUTID(out_id) < HD_VO_MAX_MASK)) {
			ret = videoout_set_osg_mask_palette_tbl(usr_param->p_table, usr_param->table_size);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("videoout_set_osg_mask_palette_tbl fail\n");
			}
		} else {
			HD_VIDEOOUT_ERR("Unsupport path_id(%#x),param_id(%d)\n", path_id, HD_VIDEOOUT_PARAM_OUT_PALETTE_TABLE);
			return HD_ERR_NG;
		}
		HD_VIDEOOUT_FLOW("hd_videoout_set(PALETTE_TABLE):\n"
				 "    %s path_id(%#x)\n"
				 "        table_size(%d) p_table[0](%#x) p_table[%d](%#x))\n",
				 dif_return_dev_string(path_id), path_id, usr_param->table_size, usr_param->p_table[0], usr_param->table_size, \
				 usr_param->p_table[usr_param->table_size - 1]);
		break;
	}
	default:
		HD_VIDEOOUT_ERR("Unsupport set param id(%d)\n", id);
		return HD_ERR_NG;
	}

err_ext:
	return ret;
}

HD_RESULT hd_videoout_get(HD_PATH_ID path_id, HD_VIDEOOUT_PARAM_ID id, void *param)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_DEVCOUNT *p_devcount;
	HD_VIDEOOUT_SYSCAPS *p_syscaps;
	HD_VIDEOOUT_WIN_ATTR *usr_win;
	HD_VIDEOOUT_WIN_PSR_ATTR *usr_win_psr;
	HD_VIDEOOUT_MODE *usr_mode;
	HD_OSG_STAMP_ATTR *p_stamp;
	HD_OSG_MASK_ATTR *p_mask_param;
	HD_VIDEOOUT_DEV_CONFIG *p_dev_cfg;
	HD_FB_DIM *p_fb_dim;
	HD_FB_ATTR *p_fb_attr;
	HD_FB_FMT *p_fb_fmt;
	INT lcd_id = 0;
	INT i;
	HD_FB_ENABLE *p_fb_state;
	HD_VIDEOOUT_EDID_DATA *p_edid_data;

	VIDEOOUT_CHECK_SELF_ID(self_id);
	VIDEOOUT_CHECK_MODULE_OPEN(self_id);
	lcd_id = VO_DEVID(self_id);

	if (lcd_id < 0) {
		HD_VIDEOOUT_ERR("Check lcd_id(%d) fail,pathid(%#x)", lcd_id, path_id);
		return HD_ERR_NG;
	}

	if (lcd_id < HD_VIDEOOUT_MAX_DEVICE_ID && lcd_id >= 0) {
	} else {
		HD_VIDEOOUT_ERR("Check lcd_id(%d) fail,pathid(%#x)", lcd_id, path_id);
		return HD_ERR_NG;
	}

	switch (id) {
	case HD_VIDEOOUT_PARAM_DEV_CONFIG:
		p_dev_cfg = (HD_VIDEOOUT_DEV_CONFIG *)param;
		ret = videoout_get_dev_cfg_p(self_id, p_dev_cfg);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_dev_cfg_p fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_get(DEV_CONFIG):\n"
				 "    %s path_id(%#x)\n"
				 "        homo(%d) state(%d)\n"
				 "        input_dim(%#x) output_type(%#x) lcd(%#x) hdmi(%#x) vga(%#x) cvbs(%#x)\n",
				 dif_return_dev_string(path_id), path_id, p_dev_cfg->devnvr_cfg.homology,
				 p_dev_cfg->devnvr_cfg.chip_state,
				 p_dev_cfg->devnvr_cfg.mode.input_dim,
				 p_dev_cfg->devnvr_cfg.mode.output_type,
				 p_dev_cfg->devnvr_cfg.mode.output_mode.lcd,
				 p_dev_cfg->devnvr_cfg.mode.output_mode.hdmi,
				 p_dev_cfg->devnvr_cfg.mode.output_mode.vga,
				 p_dev_cfg->devnvr_cfg.mode.output_mode.cvbs);
		for (i = 0; i < 3; i++) {
			HD_VIDEOOUT_FLOW("        idx(%d) max_dim(%d,%d) bpp(%d) rld_en(%d) ddr(%d)\n"
					 "        buf_pa(%#lx) buf_len(%u) rle_pa(%#lx) rle_len(%u)\n",
					 i, p_dev_cfg->devnvr_cfg.plane[i].max_w,
					 p_dev_cfg->devnvr_cfg.plane[i].max_h,
					 p_dev_cfg->devnvr_cfg.plane[i].max_bpp,
					 p_dev_cfg->devnvr_cfg.plane[i].gui_rld_enable,
					 p_dev_cfg->devnvr_cfg.plane[i].ddr_id,
					 (ULONG)p_dev_cfg->devnvr_cfg.plane[i].buf_paddr,
					 p_dev_cfg->devnvr_cfg.plane[i].buf_len,
					 (ULONG)p_dev_cfg->devnvr_cfg.plane[i].rle_buf_paddr,
					 p_dev_cfg->devnvr_cfg.plane[i].rle_buf_len);
		}
		break;
	case HD_VIDEOOUT_PARAM_DEVCOUNT:
		p_devcount = (HD_DEVCOUNT *)param;
		ret = videoout_get_devcount((HD_DEVCOUNT *)param);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_devcount fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_get(DEVCOUNT):\n"
				 "    %s path_id(%#x)\n max_dev_count(%u)\n",
				 dif_return_dev_string(path_id), path_id, p_devcount->max_dev_count);
		break;
	case HD_VIDEOOUT_PARAM_SYSCAPS:
		p_syscaps = (HD_VIDEOOUT_SYSCAPS *)param;
		VIDEOOUT_CHECK_CTRL_ID(ctrl_id);
		ret = videoout_get_syscaps(self_id, (HD_VIDEOOUT_SYSCAPS *)param);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_sysinfo fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_get(SYSCAPS):\n"
				 "    %s path_id(%#x)\n"
				 "        output_dim(%u,%u) input_dim(%u,%u) fps(%u)\n"
				 "        max_scale w(u/d:%u,%u) h(u/d:%u,%u)\n"
				 "        out_stamp(%u,%u) out_mask(%u,%u)\n",

				 dif_return_dev_string(path_id), path_id, p_syscaps->output_dim.w, p_syscaps->output_dim.h,
				 p_syscaps->input_dim.w, p_syscaps->input_dim.h,
				 p_syscaps->fps,
				 p_syscaps->max_scale_up_w, p_syscaps->max_scale_up_h,
				 p_syscaps->max_scale_down_w, p_syscaps->max_scale_down_h,
				 p_syscaps->max_out_stamp, p_syscaps->max_out_stamp_ex,
				 p_syscaps->max_out_mask, p_syscaps->max_out_mask_ex);
		break;
	case HD_VIDEOOUT_PARAM_IN_WIN_ATTR:
		usr_win = (HD_VIDEOOUT_WIN_ATTR *)param;
		VIDEOOUT_CHECK_IN_ID(in_id);
		ret = videoout_get_win(self_id, in_id, (HD_VIDEOOUT_WIN_ATTR *)param);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_win fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_get(IN_WIN_ATTR):\n"
				 "    %s path_id(%#x)\n"
				 "        visible(%#x) rect(%u,%u,%u,%u) layer(%#x)\n",
				 dif_return_dev_string(path_id), path_id, usr_win->visible,
				 usr_win->rect.x,
				 usr_win->rect.y,
				 usr_win->rect.w,
				 usr_win->rect.h,
				 usr_win->layer);
		break;
	case HD_VIDEOOUT_PARAM_IN_WIN_PSR_ATTR:
		usr_win_psr = (HD_VIDEOOUT_WIN_PSR_ATTR *)param;
		VIDEOOUT_CHECK_IN_ID(in_id);
		ret = videoout_get_win_psr(self_id, in_id, (HD_VIDEOOUT_WIN_PSR_ATTR *)param);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_win_psr fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_get(IN_WIN_PSR):\n"
				 "    %s path_id(%#x)\n"
				 "        visible(%#x) rect(%u,%u,%u,%u)\n",
				 dif_return_dev_string(path_id), path_id, usr_win_psr->visible,
				 usr_win_psr->rect.x,
				 usr_win_psr->rect.y,
				 usr_win_psr->rect.w,
				 usr_win_psr->rect.h);
		break;
	case HD_VIDEOOUT_PARAM_MODE:
		usr_mode = (HD_VIDEOOUT_MODE *)param;
		if (ctrl_id != HD_CTRL) {
			HD_VIDEOOUT_ERR("Please use control port to get HD_VIDEOOUT_PARAM_MODE\n");
			ret = HD_ERR_NOT_SUPPORT;
			break;
		}
		ret = videoout_get_mode(self_id, (HD_VIDEOOUT_MODE *)param);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_win fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_get(MODE):\n"
				 "    %s path_id(%#x)\n"
				 "        input_dim(%#x) output_type(%#x)\n"
				 "        lcd(%#x) hdmi(%#x) vga(%#x) cvbs(%#x)\n",
				 dif_return_dev_string(path_id), path_id, usr_mode->input_dim,
				 usr_mode->output_type,
				 usr_mode->output_mode.lcd,
				 usr_mode->output_mode.hdmi,
				 usr_mode->output_mode.vga,
				 usr_mode->output_mode.cvbs);
		break;
	case HD_VIDEOOUT_PARAM_OUT_STAMP_ATTR: {
		p_stamp = (HD_OSG_STAMP_ATTR *)param;
		out_id = VO_STAMP_OUTID(out_id);
		in_id = VO_INID(in_id);
		ret = videoout_get_stamp(self_id, out_id, in_id, (HD_OSG_STAMP_ATTR *)param);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_stamp fail\n");
		}
		HD_OSG_FLOW("hd_videoout_get(OUT_STAMP_ATTR):\n"
			    "    %s path_id(%#x)\n"
			    "        type(%#x) alpha(%u) pos(%u,%u)\n"
			    "        gcac_en(%u) gcac_w(%u) gcac_h(%u)\n",
			    dif_return_dev_string(path_id), path_id, p_stamp->align_type,
			    p_stamp->alpha,
			    p_stamp->position.x,
			    p_stamp->position.y,
			    p_stamp->gcac_enable,
			    p_stamp->gcac_blk_width,
			    p_stamp->gcac_blk_height);
		break;
	}
	case HD_VIDEOOUT_PARAM_OUT_MASK_ATTR: {
		p_mask_param = (HD_OSG_MASK_ATTR *)param;
		out_id = VO_MASK_OUTID(in_id);
		in_id = VO_INID(in_id);
		ret = videoout_get_mask(self_id, out_id, in_id, (HD_OSG_MASK_ATTR *)param);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_mask fail\n");
		}
		HD_OSG_FLOW("hd_videoout_get(OUT_MASK_ATTR):\n"
			    "    %s path_id(%#x)\n"
			    "        type(%#x) alpha(%u) color(%u)\n"
			    "        pos0(%u,%u) pos1(%u,%u) pos2(%u,%u) pos3(%u,%u)\n",
			    dif_return_dev_string(path_id), path_id, p_mask_param->type,
			    p_mask_param->alpha, p_mask_param->color,
			    p_mask_param->position[0].x, p_mask_param->position[0].y,
			    p_mask_param->position[1].x, p_mask_param->position[1].y,
			    p_mask_param->position[2].x, p_mask_param->position[2].y,
			    p_mask_param->position[3].x, p_mask_param->position[3].y);
		break;
	}
	case HD_VIDEOOUT_PARAM_OUT_MOSAIC_ATTR: {
		HD_VIDEOOUT_WRN("Not support HD_VIDEOOUT_PARAM_OUT_MOSAIC_ATTR in 8296\n");
		break;
	}
	case HD_VIDEOOUT_PARAM_FB_ATTR:
		p_fb_attr = (HD_FB_ATTR *)param;
		ret = videoout_get_fb_attr(p_fb_attr->fb_id, lcd_id, p_fb_attr);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_fb_attrs fail\n");
		}
		break;
	case HD_VIDEOOUT_PARAM_FB_DIM: {
		p_fb_dim = (HD_FB_DIM *)param;
		ret = videoout_get_fb_dim(p_fb_dim->fb_id, lcd_id, &p_fb_dim->input_dim, &p_fb_dim->output_rect);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_fb_dim fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_get(FB_DIM):\n"
				 "    %s path_id(%#x)\n"
				 "        fb_id(%#x) dim(%dx%d) output_rect(%d %d %d %d)\n",
				 dif_return_dev_string(path_id), path_id, p_fb_dim->fb_id, p_fb_dim->input_dim.w, p_fb_dim->input_dim.h,
				 p_fb_dim->output_rect.x, p_fb_dim->output_rect.y, p_fb_dim->output_rect.w, p_fb_dim->output_rect.h);
		break;
	}
	case HD_VIDEOOUT_PARAM_FB_FMT: {
		p_fb_fmt = (HD_FB_FMT *)param;
		ret = videoout_get_fb_fmt(p_fb_fmt->fb_id, lcd_id, &p_fb_fmt->fmt);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_fb_dim fail\n");
		}
		HD_VIDEOOUT_FLOW("hd_videoout_get(FB_FMT):\n"
				 "    %s path_id(%#x)\n"
				 "        fb_id(%#x) pxlfmt(%#x)\n",
				 dif_return_dev_string(path_id), path_id, p_fb_fmt->fb_id, p_fb_fmt->fmt);
		break;
	}
	case HD_VIDEOOUT_PARAM_OUT_PALETTE_TABLE: {
		HD_PALETTE_TBL *usr_param = (HD_PALETTE_TBL *)param;

		if ((out_id > HD_STAMP_EX_BASE) && (VO_STAMP_OUTID(out_id) >= 0 && VO_STAMP_OUTID(out_id) < HD_VO_MAX_OSG_STAMP)) {
			ret = videoout_get_osg_stamp_palette_tbl(usr_param->p_table, usr_param->table_size);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("videoout_get_osg_stamp_palette_tbl fail\n");
			}
		} else if ((out_id > HD_MASK_EX_BASE) && (VO_MASK_OUTID(out_id) >= 0 && VO_MASK_OUTID(out_id) < HD_VO_MAX_MASK)) {
			ret = videoout_get_osg_mask_palette_tbl(usr_param->p_table, usr_param->table_size);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("videoout_get_osg_mask_palette_tbl fail\n");
			}

		} else {
			HD_VIDEOOUT_ERR("Unsupport path_id(%#x),param_id(%d)\n", path_id, HD_VIDEOOUT_PARAM_OUT_PALETTE_TABLE);
			return HD_ERR_NG;
		}

		HD_VIDEOOUT_FLOW("hd_videoout_get(PALETTE_TABLE):\n"
				 "    %s path_id(%#x)\n"
				 "        table_size(%d) p_table[0](%#x) p_table[%d](%#x))\n",
				 dif_return_dev_string(path_id), path_id, usr_param->table_size, usr_param->p_table[0], usr_param->table_size, \
				 usr_param->p_table[usr_param->table_size - 1]);
		break;
	}
	case HD_VIDEOOUT_PARAM_FB_ENABLE: {
		p_fb_state = (HD_FB_ENABLE *)param;
		ret = videoout_get_fb_enable_p(p_fb_state->fb_id, lcd_id, p_fb_state);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_fb_enable_p fail\n");
			return HD_ERR_NG;
		}
		HD_VIDEOOUT_FLOW("hd_videoout_get(FB_ENABLE):\n"
				 "    %s path_id(%#x)\n"
				 "        fb_id(%#x) en(%u)\n",
				 dif_return_dev_string(path_id), path_id, p_fb_state->fb_id, p_fb_state->enable);
		break;
	}
	case HD_VIDEOOUT_PARAM_EDID_DATA: {
		HD_VIDEOOUT_DEV_CONFIG tmp_dev_cfg;

		p_edid_data = (HD_VIDEOOUT_EDID_DATA *)param;

		if (p_edid_data->edid_data_len && !p_edid_data->p_edid_data) {
			HD_VIDEOOUT_ERR("Check edid_data_len=%d p_edid_data=%p fail\n", p_edid_data->edid_data_len, p_edid_data->p_edid_data);
			return HD_ERR_INV;
		}

		if (p_edid_data->video_id_len && !p_edid_data->p_video_id) {
			HD_VIDEOOUT_ERR("Check video_id_len=%d p_video_id=%p fail\n", p_edid_data->video_id_len, p_edid_data->p_video_id);
			return HD_ERR_INV;
		}

		if (p_edid_data->interface_id != HD_COMMON_VIDEO_OUT_HDMI && p_edid_data->interface_id != HD_COMMON_VIDEO_OUT_VGA) {
			HD_VIDEOOUT_ERR("Only support interface_id=HD_COMMON_VIDEO_OUT_HDMI or HD_COMMON_VIDEO_OUT_VGA\n");
			return HD_ERR_NOT_SUPPORT;
		}

		ret = videoout_get_dev_cfg_p(self_id, &tmp_dev_cfg);

		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_EDID_DATA: call videoout_get_dev_cfg_p fail\n");
			return ret;
		}

		if (tmp_dev_cfg.devnvr_cfg.mode.output_type == HD_COMMON_VIDEO_OUT_TYPE_NONE)  {
			HD_VIDEOOUT_ERR("videoout%d output_type=%d chip_state=%d homology=%d not support to get edid from interface_id=%d\n", 
				lcd_id,  tmp_dev_cfg.devnvr_cfg.mode.output_type, tmp_dev_cfg.devnvr_cfg.chip_state, tmp_dev_cfg.devnvr_cfg.homology, 
				p_edid_data->interface_id);
			return HD_ERR_NOT_SUPPORT;
		}
		//Check videoout "output_type or  homology" == interface_id
		if (tmp_dev_cfg.devnvr_cfg.mode.output_type != p_edid_data->interface_id && !(tmp_dev_cfg.devnvr_cfg.homology & (1 << p_edid_data->interface_id)))  {
			HD_VIDEOOUT_ERR("videoout%d output_type=%d chip_state=%d homology=%d not support to get edid from interface_id=%d\n", 
				lcd_id,  tmp_dev_cfg.devnvr_cfg.mode.output_type, tmp_dev_cfg.devnvr_cfg.chip_state, tmp_dev_cfg.devnvr_cfg.homology, 
				p_edid_data->interface_id);
			return HD_ERR_NOT_SUPPORT;
		}

		if (p_edid_data->interface_id == HD_COMMON_VIDEO_OUT_HDMI) {
			ret = videoout_get_hdmi_edid_p(p_edid_data);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("videoout_get_fb_enable_p fail\n");				
			}			
		} else if (p_edid_data->interface_id == HD_COMMON_VIDEO_OUT_VGA) {
			ret = videoout_get_vga_edid_p(p_edid_data);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("videoout_get_fb_enable_p fail\n");				
			}
		} else {
			HD_VIDEOOUT_ERR("Not support  p_edid_data->interface_id(%#x)\n", p_edid_data->interface_id);
			ret =  HD_ERR_NOT_SUPPORT;
		}

		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("hd_videoout_get(HD_VIDEOOUT_PARAM_EDID_DATA):\n"
				 "    %s path_id(%#x)\n"
				 "        p_video_id(0x%p)  video_id_len(%u)\n"
				 "        p_edid_data(0x%p)  edid_data_len(%u)\n"
				 "        interface_id(%#x)\n",
				 dif_return_dev_string(path_id), path_id, p_edid_data->p_video_id, p_edid_data->video_id_len,
				 p_edid_data->p_edid_data, p_edid_data->edid_data_len, p_edid_data->interface_id);
		}
		HD_VIDEOOUT_FLOW("hd_videoout_get(HD_VIDEOOUT_PARAM_EDID_DATA):\n"
				 "    %s path_id(%#x)\n"
				 "        p_video_id(0x%p)  video_id_len(%u)\n"
				 "        p_edid_data(0x%p)  edid_data_len(%u)\n"
				 "        interface_id(%#x)\n",
				 dif_return_dev_string(path_id), path_id, p_edid_data->p_video_id, p_edid_data->video_id_len,
				 p_edid_data->p_edid_data, p_edid_data->edid_data_len, p_edid_data->interface_id);	
		break;
	}
	default:
		HD_VIDEOOUT_ERR("Unsupport get param id(%d)\n", id);
		return HD_ERR_NG;
	}
	return ret;
}


HD_RESULT hd_videoout_bind(HD_OUT_ID _out_id, HD_IN_ID _dest_in_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);
	HD_DAL dest_id = HD_GET_DEV(_dest_in_id);
	HD_IO in_id = HD_GET_IN(_dest_in_id);

	ret = bd_bind(self_id, out_id, dest_id, in_id);
	HD_VIDEOOUT_FLOW("hd_videoout_bind: %s_%d_OUT_%d -> %s_%d_IN_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(out_id),
		get_device_name(bd_get_dev_baseid(dest_id)), bd_get_dev_subid(dest_id), BD_GET_IN_SUBID(in_id));
	return ret;
}


HD_RESULT hd_videoout_unbind(HD_OUT_ID _out_id)
{
	HD_RESULT ret;
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id = HD_GET_OUT(_out_id);

	ret = bd_unbind(self_id, out_id);
	HD_VIDEOOUT_FLOW("hd_videoout_unbind: %s_%d_OUT_%d\n",
		get_device_name(bd_get_dev_baseid(self_id)), bd_get_dev_subid(self_id), BD_GET_OUT_SUBID(out_id));
	return ret;
}


HD_RESULT hd_videoout_start(HD_PATH_ID path_id)
{
	return hd_videoout_start_list(&path_id, 1);
}

HD_RESULT hd_videoout_start_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret = HD_OK;
	INT check_param_result = 0;
	HD_DAL self_id;
	HD_IO in_id, out_id;
	INT in_idx, out_idx;
	UINT i;
	INT lcd_id = 0;
	CHAR bind_src[32];

	for (i = 0; i < nr; i++) {
		in_id = HD_GET_IN(path_id[i]);
		self_id = HD_GET_DEV(path_id[i]);
		out_id = HD_GET_OUT(path_id[i]);
		if ((in_id <= HD_IN_MAX && out_id <= HD_OUT_MAX)) {
			if (bd_get_prev(self_id, in_id, bind_src, sizeof(bind_src)) != HD_OK) {
				HD_VIDEOOUT_ERR("Check %d path_id(%#x) is not binded\n", i, path_id[i]);
				return HD_ERR_NOT_SUPPORT;
			}
		}
	}

	ret = bd_start_list(path_id, nr);
	if (ret != HD_OK) {
		HD_VIDEOOUT_ERR("start path_id(0x%x),nr(%d) fail\n", *path_id, nr);
		goto ext;
	}
	for (i = 0; i < nr; i++) {
		in_id = HD_GET_IN(path_id[i]);
		out_id = HD_GET_OUT(path_id[i]);
		self_id = HD_GET_DEV(path_id[i]);

		VIDEOOUT_CHECK_SELF_ID(self_id);
		lcd_id = VO_DEVID(self_id);
		if (lcd_id >= HD_VIDEOOUT_MAX_DEVICE_ID || lcd_id < 0) {
			HD_VIDEOOUT_ERR("Check lcd_id(%d) fail,pathid(%#x)\n", lcd_id, path_id[i]);
			return HD_ERR_OVERRUN;
		}
	}

	for (i = 0; i < nr; i++) {
		in_id = HD_GET_IN(path_id[i]);
		out_id = HD_GET_OUT(path_id[i]);
		self_id = HD_GET_DEV(path_id[i]);
		if (in_id <= HD_IN_MAX && out_id <= HD_OUT_MAX) {
			/* do bd_start_list: SO do nothing */
			HD_VIDEOOUT_FLOW("hd_videoout_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
		} else if ((out_id > HD_STAMP_EX_BASE) && (VO_STAMP_OUTID(out_id) >= 0 && VO_STAMP_OUTID(out_id) < HD_VO_MAX_OSG_STAMP)) {
			out_idx = VO_STAMP_OUTID(out_id);
			in_idx = VO_INID(in_id);
			HD_OSG_FLOW("hd_videoout_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
			g_video_out_param[lcd_id].num_stamp ++;
			g_video_out_param[lcd_id].stamp[GET_STAMP_ID(in_idx, out_idx)].win_enable = TRUE;
			g_video_out_param[lcd_id].stamp[GET_STAMP_ID(in_idx, out_idx)].path_id = *path_id;
			if (GET_STAMP_ID(in_idx, out_idx) > g_video_out_param[lcd_id].max_stamp_idx) {
				g_video_out_param[lcd_id].max_stamp_idx = GET_STAMP_ID(in_idx, out_idx);
			}
			if (g_video_out_param[lcd_id].max_stamp_idx >= HD_MAXVO_TOTAL_STAMP_NU) {
				HD_VIDEOOUT_ERR("Check max_stamp_idx=%d fail\n", g_video_out_param[lcd_id].max_stamp_idx);
				return HD_ERR_OVERRUN;
			}
			if (lcd_id < MAX_VIDEOOUT_CTRL_ID) {//only videoout0/1 have dim and fmt to check,and flow mode to check/set param to osg
				check_param_result = verify_param_HD_VIDEOOUT_OSG_param(lcd_id, g_video_out_param[lcd_id].stamp, \
						     g_video_out_param[lcd_id].max_stamp_idx, NULL);
				if (check_param_result < 0) {
					HD_VIDEOOUT_ERR("verify_param_HD_VIDEOOUT_OSG_param fail, ret=%d\n", check_param_result);
					return HD_ERR_PARAM;
				}
				ret = videoout_set_osg_win(g_video_out_param[lcd_id].osg_idx, g_video_out_param[lcd_id].stamp, \
							   g_video_out_param[lcd_id].max_stamp_idx);
				if (ret != HD_OK) {
					HD_VIDEOOUT_ERR("set osg win fail\n");
					return ret;
				}
			}
		} else if ((out_id > HD_MASK_EX_BASE) && \
			   (VO_MASK_OUTID(out_id) >= 0 && VO_MASK_OUTID(out_id) < HD_VO_MAX_MASK)) {
			out_idx = VO_MASK_OUTID(out_id);
			in_idx = VO_INID(in_id);
			if (GET_MASK_ID(in_idx, out_idx) >= HD_MAXVO_TOTAL_MASK_NU) {
				HD_VIDEOOUT_ERR("Check mask idx fail,in_idx(%d),out_idx(%d), mask_idx(%d) > HD_MAXVO_TOTAL_MASK_NU\n", in_idx, out_idx,
						GET_MASK_ID(in_idx, out_idx));
				return HD_ERR_NG;
			}
			HD_OSG_FLOW("hd_videoout_start:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
			g_video_out_param[lcd_id].num_mask ++;
			g_video_out_param[lcd_id].mask_mosaic[GET_MASK_ID(in_idx, out_idx)].enable = TRUE;
			if (GET_MASK_ID(in_idx, out_idx) > g_video_out_param[lcd_id].max_mask_idx) {
				g_video_out_param[lcd_id].max_mask_idx = GET_MASK_ID(in_idx, out_idx);
			}
			if (GET_MASK_ID(in_idx, out_idx) >= HD_MAXVO_TOTAL_MASK_NU) {
				HD_VIDEOOUT_ERR("Check mask id(%d) fail, max(%d)\n", GET_MASK_ID(in_idx, out_idx), HD_MAXVO_TOTAL_MASK_NU);
				return HD_ERR_OVERRUN;
			}
			if (lcd_id < MAX_VIDEOOUT_CTRL_ID) {//only videoout0/1 have dim and fmt to check,and flow mode to check/set param to osg
				check_param_result = verify_param_HD_VIDEOOUT_MASK_MOSAIC_param(lcd_id, g_video_out_param[lcd_id].mask_mosaic,
						     g_video_out_param[lcd_id].max_mask_idx, NULL);
				if (check_param_result < 0) {
					HD_VIDEOOUT_ERR("verify_param_HD_VIDEOOUT_MASK_MOSAIC_param fail,ret=%d\n", check_param_result);
					return HD_ERR_PARAM;
				}
				ret = videoout_set_mask_mosaic(g_video_out_param[lcd_id].osg_idx, g_video_out_param[lcd_id].mask_mosaic, \
							       g_video_out_param[lcd_id].max_mask_idx);
				if (ret != HD_OK) {
					HD_VIDEOOUT_ERR("set osg mask fail\n");
					return ret;
				}
			}
		} else {
			HD_VIDEOENC_ERR("Unsupport set path_id[%d] = %x.\n", i, path_id[i]);
			return HD_ERR_NOT_SUPPORT;
		}
	}

ext:
	return ret;
}


HD_RESULT hd_videoout_stop(HD_PATH_ID path_id)
{
	return hd_videoout_stop_list(&path_id, 1);
}

HD_RESULT hd_videoout_stop_list(HD_PATH_ID *path_id, UINT nr)
{
	HD_RESULT ret = HD_OK;
	HD_DAL self_id;
	HD_IO in_id, out_id;
	UINT i;
	INT lcd_id = 0;

	ret = bd_stop_list(path_id, nr);
	if (ret != HD_OK) {
		HD_VIDEOOUT_ERR("stop path_id(0x%x),nr(%d) fail\n", *path_id, nr);
		goto ext;
	}
	for (i = 0; i < nr; i++) {
		in_id = HD_GET_IN(path_id[i]);
		out_id = HD_GET_OUT(path_id[i]);
		self_id = HD_GET_DEV(path_id[i]);

		VIDEOOUT_CHECK_SELF_ID(self_id);
		lcd_id = VO_DEVID(self_id);
		if (lcd_id >= HD_VIDEOOUT_MAX_DEVICE_ID || lcd_id < 0) {
			HD_VIDEOOUT_ERR("Check lcd_id(%d) fail,pathid(%#x)\n", lcd_id, path_id[i]);
			return HD_ERR_OVERRUN;
		}
		if ((in_id <= HD_IN_MAX) && (out_id <= HD_OUT_MAX)) {
			/* do bd_start_list: SO do nothing */
			HD_VIDEOOUT_FLOW("hd_videoout_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
		} else if ((out_id > HD_STAMP_EX_BASE) && (VO_STAMP_OUTID(out_id) >= 0 && VO_STAMP_OUTID(out_id) < HD_VO_MAX_OSG_STAMP)) {
			out_id = VO_STAMP_OUTID(out_id);
			in_id = VO_INID(in_id);
			HD_OSG_FLOW("hd_videoout_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
			if (GET_STAMP_ID(in_id, out_id) >= HD_MAXVO_TOTAL_STAMP_NU) {
				HD_VIDEOOUT_ERR("Check stamp id(%d) fail, max(%d)\n", GET_STAMP_ID(in_id, out_id), HD_MAXVO_TOTAL_STAMP_NU);
				return HD_ERR_OVERRUN;
			}
			if (g_video_out_param[lcd_id].stamp[GET_STAMP_ID(in_id, out_id)].win_enable) {
				if (g_video_out_param[lcd_id].num_stamp == 0) {
					goto ext;
				}
				g_video_out_param[lcd_id].num_stamp --;
				g_video_out_param[lcd_id].stamp[GET_STAMP_ID(in_id, out_id)].win_enable = FALSE;
				videoout_set_osg_win(g_video_out_param[lcd_id].osg_idx, g_video_out_param[lcd_id].stamp, \
						     g_video_out_param[lcd_id].max_stamp_idx);
				videoout_update_stamp_cfg(out_id, in_id, &g_video_out_param[lcd_id], FALSE);
			}
		} else if ((out_id > HD_MASK_EX_BASE) && (VO_MASK_OUTID(out_id) >= 0 && VO_MASK_OUTID(out_id) < HD_VO_MAX_MASK)) {
			out_id = VO_MASK_OUTID(out_id);
			in_id = VO_INID(in_id);
			HD_OSG_FLOW("hd_videoout_stop:\n    %s path_id(%#x)\n", dif_return_dev_string(path_id[i]), path_id[i]);
			if (GET_MASK_ID(in_id, out_id) >= HD_MAXVO_TOTAL_MASK_NU) {
				HD_VIDEOOUT_ERR("Check mask id(%d) fail, max(%d)\n", GET_MASK_ID(in_id, out_id), HD_MAXVO_TOTAL_MASK_NU);
				return HD_ERR_OVERRUN;
			}
			if (GET_MASK_ID(in_id, out_id) > g_video_out_param[lcd_id].max_mask_idx) {
				g_video_out_param[lcd_id].max_mask_idx = GET_MASK_ID(in_id, out_id);
			}
			if (g_video_out_param[lcd_id].num_mask == 0) {
				goto ext;
			}
			g_video_out_param[lcd_id].num_mask --;
			g_video_out_param[lcd_id].mask_mosaic[GET_MASK_ID(in_id, out_id)].enable = FALSE;
			videoout_set_mask_mosaic(g_video_out_param[lcd_id].osg_idx, g_video_out_param[lcd_id].mask_mosaic, \
						 g_video_out_param[lcd_id].max_mask_idx);
		} else {
			HD_VIDEOOUT_ERR("Unsupport stop in_id=%d,out_id=%d,path_id[%d]=%#x\n", in_id, out_id, i, path_id[i]);
			ret = HD_ERR_IO;
			goto ext;
		}
	}

ext:
	return ret;
}

BOOL videoout_check_addr(UINTPTR address)
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
				printf("INVALID! 0x%x\n", errsv);
				m_valid = FALSE;
			}
		} else {
			//printf("INvalid!start=0x%x\n", (unsigned int)start);
			m_valid = FALSE;
		}
	}
	return m_valid;
}

HD_RESULT hd_videoout_push_in_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_in_video_frame, HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;
	UINT32 lcd_w, lcd_h;
	HD_DAL self_id = HD_GET_DEV(path_id);
	INT inport_idx;
	HD_IO in_id = HD_GET_IN(path_id);
	CHAR bind_src[32];
	VDDO_INFO_PRIV *p_vo_priv = NULL;
	INT lcd_id = 0;
	INT check_param_result = 0;

	VIDEOOUT_CHECK_SELF_ID(self_id);
	lcd_id = VO_DEVID(self_id);
	if (lcd_id >= HD_VIDEOOUT_MAX_DEVICE_ID || lcd_id < 0) {
		HD_VIDEOOUT_ERR("Check lcd_id(%d) fail,pathid(%#x)", lcd_id, path_id);
		return HD_ERR_OVERRUN;
	}
	if (lcd_id < MAX_VIDEOOUT_CTRL_ID) {
		inport_idx = VO_INID(in_id);
		if (inport_idx >= HD_VIDEOOUT_MAX_IN) {
			HD_VIDEOOUT_ERR("Only support push_in to videoout input port\n");
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}
		get_lcd_dim_by_value(g_video_out_param[lcd_id].mode.input_dim, &lcd_w, &lcd_h);
		VIDEOOUT_CHECK_VIDEOOUT_BUF_DIM(p_in_video_frame, lcd_w, lcd_h);
		VIDEOOUT_CHECK_MODULE_OPEN(self_id);
		/* check and set state */
		p_vo_priv = &g_video_out_param[lcd_id].priv[inport_idx];
		if (p_vo_priv->push_state == HD_PATH_ID_CLOSED) {
			ret = HD_ERR_NOT_OPEN;
			goto exit;
		} else if (p_vo_priv->push_state == HD_PATH_ID_ONGOING) {
			ret = HD_ERR_STATE;
			goto exit;
		} else {
			p_vo_priv->push_state = HD_PATH_ID_ONGOING;
		}
		if ((g_video_out_param[lcd_id].push_in_used[inport_idx] == 1) ||
		    (bd_get_prev(self_id, in_id, bind_src, sizeof(bind_src)) == HD_OK)) {
			printf("videoout path(0x%x) already in_used(%d) or in binded src\n",
			       (int)path_id, g_video_out_param[lcd_id].push_in_used[inport_idx]);
			system("echo videoout all > /proc/hdal/setting");
			system("cat /proc/hdal/setting");
			ret = HD_ERR_NOT_SUPPORT;
			p_vo_priv->push_state = HD_PATH_ID_OPEN;
			goto exit;
		}
		g_video_out_param[lcd_id].push_in_used[inport_idx] = 1;
		if (wait_ms < 0 || wait_ms > 10000) {
			HD_VIDEOOUT_ERR("Check the value wait_ms(%d) is out-of-range(0~%d).\n", wait_ms, 10000);
			ret = HD_ERR_NOT_SUPPORT;
			g_video_out_param[lcd_id].push_in_used[inport_idx] = 0;
			p_vo_priv->push_state = HD_PATH_ID_OPEN;
			goto exit;
		}
		if (videoout_check_addr(p_in_video_frame->phy_addr[0] == FALSE)) {
			HD_VIDEOOUT_ERR("Check HD_VIDEO_FRAME phy_addr[0] fail.\n");
			ret = HD_ERR_NOT_SUPPORT;
			g_video_out_param[lcd_id].push_in_used[inport_idx] = 0;
			p_vo_priv->push_state = HD_PATH_ID_OPEN;
			goto exit;
		}
		HD_TRIGGER_FLOW("videoout_push_in_buf:\n");
		HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		HD_TRIGGER_FLOW("       dim(%dx%d) pa(%p) fmt(%#x) fd(%#x)\n", p_in_video_frame->dim.w, p_in_video_frame->dim.h, \
				(VOID *)p_in_video_frame->phy_addr[0], p_in_video_frame->pxlfmt, g_video_out_param[lcd_id].priv[inport_idx].fd);
		p_in_video_frame->reserved[0] = path_id;
		p_in_video_frame->reserved[2] = 0;
		ret = videoout_push_in_buf(path_id, p_in_video_frame, wait_ms);
		g_video_out_param[lcd_id].push_in_used[inport_idx] = 0;
		p_vo_priv->push_state = HD_PATH_ID_OPEN;
	} else {
		if (wait_ms < 0 || wait_ms > 10000) {
			HD_VIDEOOUT_ERR("Check the value wait_ms(%d) is out-of-range(0~%d).\n", wait_ms, 10000);
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}
		g_video_out_param[lcd_id].osg_idx = lcd_id;
		lcd_cfg[lcd_id].osg_idx = &g_video_out_param[lcd_id].osg_idx;
		HD_TRIGGER_FLOW("videoout_push_in_buf_to_osg:\n");
		HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		HD_TRIGGER_FLOW("       dim(%dx%d) pa(%p) fmt(%#x)\n", p_in_video_frame->dim.w, p_in_video_frame->dim.h, \
				(VOID *)p_in_video_frame->phy_addr[0], p_in_video_frame->pxlfmt);
		if (videoout_check_addr(p_in_video_frame->phy_addr[0] == FALSE)) {
			HD_VIDEOOUT_ERR("Check HD_VIDEO_FRAME phy_addr[0] fail.\n");
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}
		//for virtual videoout,user mode to check/set param to osg
		check_param_result = verify_param_HD_VIDEOOUT_OSG_param(-1, g_video_out_param[lcd_id].stamp,
				     g_video_out_param[lcd_id].max_stamp_idx, p_in_video_frame);
		if (check_param_result < 0) {
			HD_VIDEOOUT_ERR("verify_param_HD_VIDEOOUT_OSG_param fail,ret=%d\n", check_param_result);
			return HD_ERR_PARAM;
		}
		check_param_result = verify_param_HD_VIDEOOUT_MASK_MOSAIC_param(-1, g_video_out_param[lcd_id].mask_mosaic,
				     g_video_out_param[lcd_id].max_mask_idx, p_in_video_frame);
		if (check_param_result < 0) {
			HD_VIDEOOUT_ERR("verify_param_HD_VIDEOOUT_MASK_MOSAIC_param fail,ret=%d\n", check_param_result);
			return HD_ERR_PARAM;
		}
		ret = videoout_set_osg_win(g_video_out_param[lcd_id].osg_idx, g_video_out_param[lcd_id].stamp,
					   g_video_out_param[lcd_id].max_stamp_idx);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("set osg win fail\n");
			return ret;
		}
		ret = videoout_set_mask_mosaic(g_video_out_param[lcd_id].osg_idx, g_video_out_param[lcd_id].mask_mosaic, \
					       g_video_out_param[lcd_id].max_mask_idx);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("set osg mask fail\n");
			return ret;
		}
		p_in_video_frame->reserved[0] = path_id;
		ret = videoout_push_in_buf_to_osg(path_id, p_in_video_frame, p_user_out_video_frame, wait_ms);
	}
exit:
	return ret;
}


HD_RESULT hd_videoout_pull_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_RESULT ret;
	VDDO_INFO_PRIV *p_vo_priv = NULL;
	INT lcd_id = 0;
	INT inport_idx;

	VIDEOOUT_CHECK_SELF_ID(self_id);
	lcd_id = VO_DEVID(self_id);
	if (lcd_id >= HD_VIDEOOUT_MAX_DEVICE_ID || lcd_id < 0) {
		HD_VIDEOOUT_ERR("Check lcd_id(%d) fail,pathid(%#x)", lcd_id, path_id);
		return HD_ERR_OVERRUN;
	}

	if (lcd_id < MAX_VIDEOOUT_CTRL_ID) {
		ret = dif_check_flow_mode(self_id, out_id);
		if (ret == HD_ERR_NOT_AVAIL) {
			HD_VIDEOOUT_ERR("Need bind pathid(%#x) to pull out\n", path_id);
			return HD_ERR_NOT_SUPPORT;
		}
		inport_idx = VO_INID(in_id);
		p_vo_priv = &g_video_out_param[lcd_id].priv[inport_idx];
		if (inport_idx >= HD_VIDEOOUT_MAX_IN) {
			HD_VIDEOOUT_ERR("Only support pull_out from videoout input port\n");
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}
		if (wait_ms < 0 || wait_ms > 10000) {
			HD_VIDEOOUT_ERR("Check the value wait_ms(%d) is out-of-range(0~%d).\n", wait_ms, 10000);
			ret = HD_ERR_NOT_SUPPORT;
			p_vo_priv->pull_state = HD_PATH_ID_OPEN;
			goto exit;
		}
		VIDEOOUT_CHECK_MODULE_OPEN(self_id);
		/* check and set state */
		if (p_vo_priv->pull_state == HD_PATH_ID_CLOSED) {
			ret = HD_ERR_NOT_OPEN;
			goto exit;
		} else if (p_vo_priv->pull_state == HD_PATH_ID_ONGOING) {
			ret = HD_ERR_STATE;
			goto exit;
		} else {
			p_vo_priv->pull_state = HD_PATH_ID_ONGOING;
		}
		ret = videoout_pull_out_buf(path_id, out_id, p_video_frame, wait_ms);
		p_vo_priv->pull_state = HD_PATH_ID_OPEN;
		HD_TRIGGER_FLOW("videoout_pull_out_buf:\n");
		HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
		HD_TRIGGER_FLOW("       dim(%dx%d) pa(%p) fmt(%#x)\n", p_video_frame->dim.w, p_video_frame->dim.h, \
				(VOID *)p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
	} else {
		if (g_video_out_param[lcd_id].osg_idx) {
			lcd_cfg[lcd_id].osg_idx = &g_video_out_param[lcd_id].osg_idx;
			ret = videoout_pull_out_buf_from_osg(path_id, p_video_frame, wait_ms);
			HD_TRIGGER_FLOW("videoout_pull_out_buf_from_osg:\n");
			HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
			HD_TRIGGER_FLOW("       dim(%dx%d) pa(%#lx) fmt(%#x)\n", p_video_frame->dim.w, p_video_frame->dim.h, \
							p_video_frame->phy_addr[0], p_video_frame->pxlfmt);
		} else {
			ret = dif_pull_out_buffer(self_id, in_id, p_video_frame, wait_ms);
			if (ret == HD_OK) {
				if (p_video_frame->phy_addr[0] == 0) {
					HD_VIDEOOUT_ERR("dif_pull_out_buffer fail\n");
					ret = HD_ERR_NG;
					goto exit;
				}
			} else if (ret == HD_ERR_NOT_AVAIL) {
				goto exit;
			}
		}
	}
exit:
	return ret;
}

HD_RESULT hd_videoout_release_out_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_RESULT ret;

	VIDEOOUT_CHECK_SELF_ID(self_id);
	VIDEOOUT_CHECK_MODULE_OPEN(self_id);
	HD_TRIGGER_FLOW("hd_videoout_release_out_buf:\n");
	HD_TRIGGER_FLOW("   %s path_id(%#x)\n", dif_return_dev_string(path_id), path_id);
	HD_TRIGGER_FLOW("    pa(%p)\n", (VOID *)p_video_frame->phy_addr[0]);
	ret = videoout_release_out_buf(self_id, in_id, p_video_frame);
	return ret;
}



