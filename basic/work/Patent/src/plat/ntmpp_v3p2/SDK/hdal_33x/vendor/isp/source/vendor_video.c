/**
	@brief Source file of vendor video.\n
	This file contains the functions which is related to image quality in the chip.

	@file vendor_video.c

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
#include <pthread.h>
#include "hd_type.h"
#include "hdal.h"
#include "vendor_video.h"
#include "vendor_isp_common.h"
#include "vendor_isp_ioctl.h"
#include "videoprocess.h"

/*-----------------------------------------------------------------------------*/
/* External Global Variables                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Internal Global Variables                                                   */
/*-----------------------------------------------------------------------------*/
VENDOR_VIDEO_PARAM vendor_video_param[VENDER_VIDEO_MAX_CH_NUM];
INT video_init_done = 0;
pthread_mutex_t vendor_video_mutex = PTHREAD_MUTEX_INITIALIZER;
extern VDOPROC_PARAM *vdoproc_param[VDOPROC_DEV][VODPROC_OUTPUT_PORT];

/*-----------------------------------------------------------------------------*/
/* Internal Function Prototype                                                 */
/*-----------------------------------------------------------------------------*/
#define IS_ALIGN32(x)       ((x) == (((x) >> 5) << 5))

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT vendor_video_get_idx(HD_PATH_ID path_id)
{
	INT idx = -1, i, is_find = 0, is_add = 0;

	for (i = 0; i < VENDER_VIDEO_MAX_CH_NUM; i++) {
		if (vendor_video_param[i].path_id == path_id) {
			idx = i;
			is_find = 1;
			goto exit;
		}
	}

	for (i = 0; i < VENDER_VIDEO_MAX_CH_NUM; i++) {
		if (vendor_video_param[i].path_id == 0) {
			vendor_video_param[i].path_id = path_id;
			idx = i;
			is_add = 1;
			goto exit;
		}
	}

exit:
	if ((is_find == 0) && (is_add == 0)) {
		VENDOR_ISP_ERROR("vendor_video_get_idx: idx exceed. path_id(%#x)\n", path_id);
	}

	return idx;
}

HD_RESULT vendor_video_get(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_ID id, void* p_param)
{
	HD_RESULT ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	INT idx;
	VENDOR_VIDEO_PARAM_DI_GMM *p_di_gmm;
	VENDOR_VIDEO_PARAM_DI_TMNR *p_di_tmnr;
	VENDOR_VIDEO_PARAM_DEI_INTERLACE *p_di_interlace;
	VENDOR_VIDEO_PARAM_MASK *p_mask;
	VENDOR_VIDEO_PARAM_LPM *p_lpm;
	VENDOR_VIDEO_PARAM_RND *p_rnd;
	VENDOR_VIDEO_PARAM_MOTION_AQ *p_aq;
	VENDOR_VIDEO_PARAM_MRNR *p_mrnr;
	VENDOR_VIDEO_PARAM_SHARP *p_sharp;
	VENDOR_VIDEO_PARAM_TMNR_EXT *p_tmnr_ctrl;
	VENDOR_VIDEO_PARAM_TMNR_STA_DATA *p_tmnr_sta_data;
	VENDOR_VIDEO_PARAM_SCA_SET *p_sca_set;
	VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF *p_tmnr_motion_buf;
	VENDOR_VIDEO_PARAM_LCD_IQ *p_lcd_iq;
	VENDOR_VIDEO_PARAM_YUV_RANGE *p_yuv_range;
	VENDOR_VIDEO_PARAM_EDGE_SMOOTH *p_edge_smooth;
    VENDOR_VIDEO_PARAM_LCD_HUE_SAT *p_lcd_hue;
    VENDOR_VIDEO_PARAM_LCD_YGAMMA *p_lcd_ygamma;

    if ((idx = vendor_video_get_idx(path_id)) < 0) {
    	ret = HD_ERR_NOT_AVAIL;
		goto err_exit;
    }
    
	switch (id) {
	case VENDOR_VIDEO_DI_GMM:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_di_gmm = (VENDOR_VIDEO_PARAM_DI_GMM *)p_param;
		memset(p_di_gmm, 0x0, sizeof(VENDOR_VIDEO_PARAM_DI_GMM));
		ret = vendor_die_get_gmm_en_p(path_id, &(vendor_video_param[idx].di_gmm.gmm_en));
		if (ret != HD_OK)
			goto err_exit;
		ret = vendor_die_get_gmm_info_p(path_id, &(vendor_video_param[idx].di_gmm));
		if (ret != HD_OK)
			goto err_exit;
		VENDOR_ISP_FLOW("vendor_video_get(VENDOR_VIDEO_DI_GMM):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        di_gmm.gmm_en(%d)\n", vendor_video_param[idx].di_gmm.gmm_en);
		memcpy(p_di_gmm, &vendor_video_param[idx].di_gmm, sizeof(VENDOR_VIDEO_PARAM_DI_GMM));
		break;
	case VENDOR_VIDEO_DI_TMNR:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_di_tmnr = (VENDOR_VIDEO_PARAM_DI_TMNR *)p_param;
		memset(p_di_tmnr, 0x0, sizeof(VENDOR_VIDEO_PARAM_DI_TMNR));
		ret = vendor_die_get_tmnr_en_p(path_id, &(vendor_video_param[idx].di_tmnr.dn_en));
		if (ret != HD_OK)
			goto err_exit;
		ret = vendor_die_get_tmnr_info_p(path_id, &(vendor_video_param[idx].di_tmnr));
		if (ret != HD_OK)
			goto err_exit;
		VENDOR_ISP_FLOW("vendor_video_get(VENDOR_VIDEO_DI_TMNR):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        dn_en(%d)\n", vendor_video_param[idx].di_tmnr.dn_en);
		memcpy(p_di_tmnr, &vendor_video_param[idx].di_tmnr, sizeof(VENDOR_VIDEO_PARAM_DI_TMNR));
		break;
	case VENDOR_VIDEO_DI_INTERLACE:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_di_interlace = (VENDOR_VIDEO_PARAM_DEI_INTERLACE *)p_param;
		memset(p_di_interlace, 0x0, sizeof(VENDOR_VIDEO_PARAM_DEI_INTERLACE));
		//vendor_die_get_die_en_p(path_id, &(vendor_video_param[idx].di_interlace.di_en));
		ret = vendor_die_get_die_info_p(path_id, &(vendor_video_param[idx].di_interlace));
		if (ret != HD_OK)
			goto err_exit;
		VENDOR_ISP_FLOW("vendor_video_get(VENDOR_VIDEO_DI_INTERLACE):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        di_en(%d)\n", vendor_video_param[idx].di_interlace.di_en);
		memcpy(p_di_interlace, &vendor_video_param[idx].di_interlace, sizeof(VENDOR_VIDEO_PARAM_DEI_INTERLACE));
		break;
	case VENDOR_VIDEO_MASK:
		p_mask = (VENDOR_VIDEO_PARAM_MASK *)p_param;
		memset(p_mask, 0x0, sizeof(VENDOR_VIDEO_PARAM_MASK));
		memcpy(p_mask, &vendor_video_param[idx].mask, sizeof(VENDOR_VIDEO_PARAM_MASK));
		break;
	case VENDOR_VIDEO_LPM:
		p_lpm = (VENDOR_VIDEO_PARAM_LPM *)p_param;
		memset(p_lpm, 0x0, sizeof(VENDOR_VIDEO_PARAM_LPM));
		memcpy(p_lpm, &vendor_video_param[idx].lpm, sizeof(VENDOR_VIDEO_PARAM_LPM));
		break;
	case VENDOR_VIDEO_RND:
		p_rnd = (VENDOR_VIDEO_PARAM_RND *)p_param;
		memset(p_rnd, 0x0, sizeof(VENDOR_VIDEO_PARAM_RND));
		memcpy(p_rnd, &vendor_video_param[idx].rnd, sizeof(VENDOR_VIDEO_PARAM_RND));
		break;
	case VENDOR_VIDEO_MOTION_AQ:
		p_aq = (VENDOR_VIDEO_PARAM_MOTION_AQ *)p_param;
		memset(p_aq, 0x0, sizeof(VENDOR_VIDEO_PARAM_MOTION_AQ));
		memcpy(p_aq, &vendor_video_param[idx].aq, sizeof(VENDOR_VIDEO_PARAM_MOTION_AQ));
		break;
	case VENDOR_VIDEO_DN_2D:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_mrnr = (VENDOR_VIDEO_PARAM_MRNR *)p_param;
		memset(p_param, 0x0, sizeof(VENDOR_VIDEO_PARAM_MRNR));
		ret = vendor_vpe_get_mrnr_en_p(path_id, &(vendor_video_param[idx].vpe_mrnr.mrnr_en));
		if (ret != HD_OK)
			goto err_exit;
		ret = vendor_vpe_get_mrnr_info_p(path_id, &(vendor_video_param[idx].vpe_mrnr));
		if (ret != HD_OK)
			goto err_exit;
		VENDOR_ISP_FLOW("vendor_video_get(VENDOR_VIDEO_DN_2D):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        vpe_mrnr_en(%d)\n", vendor_video_param[idx].vpe_mrnr.mrnr_en);
		memcpy(p_mrnr, &vendor_video_param[idx].vpe_mrnr, sizeof(VENDOR_VIDEO_PARAM_MRNR));
		break;
	case VENDOR_VIDEO_SHARP:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_sharp = (VENDOR_VIDEO_PARAM_SHARP *)p_param;
		memset(p_sharp, 0x0, sizeof(VENDOR_VIDEO_PARAM_SHARP));
		ret = vendor_vpe_get_sharp_en_p(path_id, &(vendor_video_param[idx].sharp.sharp_en));
		if (ret != HD_OK)
			goto err_exit;
		ret = vendor_vpe_get_sharp_info_p(path_id, &(vendor_video_param[idx].sharp));
		if (ret != HD_OK)
			goto err_exit;
		VENDOR_ISP_FLOW("vendor_video_get(VENDOR_VIDEO_SHARP):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        sharp_en(%d)\n", vendor_video_param[idx].sharp.sharp_en);
		memcpy(p_sharp, &vendor_video_param[idx].sharp, sizeof(VENDOR_VIDEO_PARAM_SHARP));
		break;
	case VENDOR_VIDEO_TMNR_CTRL:
		p_tmnr_ctrl = (VENDOR_VIDEO_PARAM_TMNR_EXT *)p_param;
		memset(p_param, 0x0, sizeof(VENDOR_VIDEO_PARAM_TMNR_EXT));
		if ((dev_id & 0xff00) == HD_DAL_VIDEOENC_BASE) {
			ret = vendor_venc_get_tmnr_en_p(path_id, &(vendor_video_param[idx].enc_dn.tmnr_en));
			if (ret != HD_OK)
				goto err_exit;
			ret = vendor_venc_get_tmnr_param_p(path_id, &(vendor_video_param[idx].enc_dn));
			if (ret != HD_OK)
				goto err_exit;
			VENDOR_ISP_FLOW("vendor_video_get(VENDOR_VIDEO_TMNR_CTRL):\n");
			VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
			VENDOR_ISP_FLOW("        enc_tmnr_en(%d)\n", vendor_video_param[idx].enc_dn.tmnr_en);
			memcpy(p_tmnr_ctrl, &vendor_video_param[idx].enc_dn, sizeof(VENDOR_VIDEO_PARAM_TMNR_EXT));
		} else if ((dev_id & 0xff00) >= HD_DAL_VIDEOPROC_BASE && (dev_id & 0xff00) <= HD_DAL_VIDEOPROC_RESERVED_BASE) {
			ret = vendor_vpe_get_tmnr_en_p(path_id, &(vendor_video_param[idx].vpe_dn.tmnr_en));
			if (ret != HD_OK)
				goto err_exit;
			ret = vendor_vpe_get_tmnr_info_p(path_id, &(vendor_video_param[idx].vpe_dn));
			if (ret != HD_OK)
				goto err_exit;
			VENDOR_ISP_FLOW("vendor_video_get(VENDOR_VIDEO_TMNR_CTRL):\n");
			VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
			VENDOR_ISP_FLOW("        vpe_tmnr_en(%d)\n", vendor_video_param[idx].vpe_dn.tmnr_en);
			memcpy(p_tmnr_ctrl, &vendor_video_param[idx].vpe_dn, sizeof(VENDOR_VIDEO_PARAM_TMNR_EXT));
		}
		break;
	case VENDOR_VIDEO_TMNR_STA_DATA:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_tmnr_sta_data = (VENDOR_VIDEO_PARAM_TMNR_STA_DATA *)p_param;
		memset(p_tmnr_sta_data, 0x0, sizeof(VENDOR_VIDEO_PARAM_TMNR_STA_DATA));
		ret = vendor_vpe_get_tmnr_sta_data_p(path_id, &vendor_video_param[idx].tmnr_sta_data);
		if (ret != HD_OK)
			goto err_exit;
		memcpy(p_tmnr_sta_data, &vendor_video_param[idx].tmnr_sta_data, sizeof(VENDOR_VIDEO_PARAM_TMNR_STA_DATA));
		break;
	case VENDOR_VIDEO_SCA:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_sca_set = (VENDOR_VIDEO_PARAM_SCA_SET *)p_param;
		ret = vendor_vpe_get_sca_info_p(path_id, &(vendor_video_param[idx].sca_set));
		if (ret != HD_OK)
			goto err_exit;
		memcpy(p_sca_set, &vendor_video_param[idx].sca_set, sizeof(VENDOR_VIDEO_PARAM_SCA_SET));
		break;
	case VENDOR_VIDEO_TMNR_MOTION_BUF:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_tmnr_motion_buf = (VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF *)p_param;
		ret = vendor_vpe_get_tmnr_motion_buf_p(path_id, &(vendor_video_param[idx].tmnr_motion_buf));
		if (ret != HD_OK)
			goto err_exit;
		memcpy(p_tmnr_motion_buf, &vendor_video_param[idx].tmnr_motion_buf, sizeof(VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF));
		break;
	case VENDOR_VIDEO_LCD_IQ:
		if ((dev_id & 0xff00) != HD_DAL_VIDEOOUT_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_lcd_iq = (VENDOR_VIDEO_PARAM_LCD_IQ *)p_param;
		ret = vendor_lcd_get_iq_p(path_id, &(vendor_video_param[idx].lcd_iq));
		if (ret != HD_OK)
			goto err_exit;
		memcpy(p_lcd_iq, &vendor_video_param[idx].lcd_iq, sizeof(VENDOR_VIDEO_PARAM_LCD_IQ));
		break;
    case VENDOR_VIDEO_LCD_HUE:
		if ((dev_id & 0xff00) != HD_DAL_VIDEOOUT_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_lcd_hue = (VENDOR_VIDEO_PARAM_LCD_HUE_SAT*)p_param;
		ret = vendor_lcd_get_hue_p(path_id, &(vendor_video_param[idx].lcd_hue));
		if (ret != HD_OK)
			goto err_exit;
		memcpy(p_lcd_hue, &vendor_video_param[idx].lcd_hue, sizeof(VENDOR_VIDEO_PARAM_LCD_HUE_SAT));
		break;  
    case VENDOR_VIDEO_LCD_YGAMMA:
		if ((dev_id & 0xff00) != HD_DAL_VIDEOOUT_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_lcd_ygamma = (VENDOR_VIDEO_PARAM_LCD_YGAMMA*)p_param;
		ret = vendor_lcd_get_ygamma_p(path_id, &(vendor_video_param[idx].lcd_ygamma));
		if (ret != HD_OK)
			goto err_exit;
		memcpy(p_lcd_ygamma, &vendor_video_param[idx].lcd_ygamma, sizeof(VENDOR_VIDEO_PARAM_LCD_YGAMMA));
		break;
	case VENDOR_VIDEO_YUV_RANGE:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_yuv_range = (VENDOR_VIDEO_PARAM_YUV_RANGE *)p_param;
		ret = vendor_vpe_get_sca_yuv_range_p(path_id, &(vendor_video_param[idx].yuv_range));
		if (ret != HD_OK)
			goto err_exit;
		memcpy(p_yuv_range, &vendor_video_param[idx].yuv_range, sizeof(VENDOR_VIDEO_PARAM_YUV_RANGE));
		break;
	case VENDOR_VIDEO_EDGE_SMOOTH:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_edge_smooth = (VENDOR_VIDEO_PARAM_EDGE_SMOOTH *)p_param;
		ret = vendor_vpe_get_edge_smooth_p(path_id, &(vendor_video_param[idx].edge_smooth));
		if (ret != HD_OK)
			goto err_exit;
		memcpy(p_edge_smooth, &vendor_video_param[idx].edge_smooth, sizeof(VENDOR_VIDEO_PARAM_EDGE_SMOOTH));
		break;
	default:
		VENDOR_ISP_ERROR("Not support to get param id(%d)\n", id);
		ret = HD_ERR_NG;
		goto exit;
	}

exit:
	return ret;

err_exit:
	VENDOR_ISP_ERROR("vendor_video: get parameter id(%d) error\n", id);
	return ret;
}


HD_RESULT vendor_video_set(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_ID id, void *p_param)
{
	HD_RESULT ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	VDOPROC_PARAM *vproc_param = NULL;
	INT idx;
	VENDOR_VIDEO_PARAM_DI_GMM *p_di_gmm;
	VENDOR_VIDEO_PARAM_DI_TMNR *p_di_tmnr;
	VENDOR_VIDEO_PARAM_DEI_INTERLACE *p_di_interlace;
	VENDOR_VIDEO_PARAM_MASK *p_mask;
	VENDOR_VIDEO_PARAM_LPM *p_lpm;
	VENDOR_VIDEO_PARAM_RND *p_rnd;
	VENDOR_VIDEO_PARAM_MOTION_AQ *p_aq;
	VENDOR_VIDEO_PARAM_MRNR *p_mrnr;
	VENDOR_VIDEO_PARAM_SHARP *p_sharp;
	VENDOR_VIDEO_PARAM_TMNR_EXT *p_tmnr_ctrl;
	VENDOR_VIDEO_PARAM_SCA_SET *p_sca_set;
	VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF *p_tmnr_motion_buf;
	VENDOR_VIDEO_PARAM_LCD_IQ *p_lcd_iq;
	VENDOR_VIDEO_PARAM_YUV_RANGE *p_yuv_range;
	VENDOR_VIDEO_PARAM_EDGE_SMOOTH *p_edge_smooth;
    VENDOR_VIDEO_PARAM_LCD_HUE_SAT *p_lcd_hue;
    VENDOR_VIDEO_PARAM_LCD_YGAMMA *p_lcd_ygamma;

    if ((idx = vendor_video_get_idx(path_id)) < 0) {
    	ret = HD_ERR_NOT_AVAIL;
		goto err_exit;
    }
    
	pthread_mutex_lock(&vendor_video_mutex);

	switch (id) {

	case VENDOR_VIDEO_DI_GMM:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_di_gmm = (VENDOR_VIDEO_PARAM_DI_GMM *)p_param;
		memcpy(&vendor_video_param[idx].di_gmm, p_di_gmm, sizeof(VENDOR_VIDEO_PARAM_DI_GMM));
		VENDOR_ISP_FLOW("vendor_video_set(VENDOR_VIDEO_DI_GMM):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        di_gmm.gmm_en(%d)\n", vendor_video_param[idx].di_gmm.gmm_en);
		ret = vendor_die_set_gmm_en_p(path_id, &(vendor_video_param[idx].di_gmm.gmm_en));
		if (ret != HD_OK)
			goto err_exit;
		ret = vendor_die_set_gmm_info_p(path_id, &(vendor_video_param[idx].di_gmm));
		if (ret != HD_OK)
			goto err_exit;
		break;
	case VENDOR_VIDEO_DI_TMNR:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_di_tmnr = (VENDOR_VIDEO_PARAM_DI_TMNR *)p_param;
		memcpy(&vendor_video_param[idx].di_tmnr, p_di_tmnr, sizeof(VENDOR_VIDEO_PARAM_DI_TMNR));
		VENDOR_ISP_FLOW("vendor_video_set(VENDOR_VIDEO_DI_TMNR):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        dn_en(%d)\n", vendor_video_param[idx].di_tmnr.dn_en);
		ret = vendor_die_set_tmnr_en_p(path_id, &(vendor_video_param[idx].di_tmnr.dn_en));
		if (ret != HD_OK)
			goto err_exit;
		ret = vendor_die_set_tmnr_info_p(path_id, &(vendor_video_param[idx].di_tmnr));
		if (ret != HD_OK)
			goto err_exit;
		break;
	case VENDOR_VIDEO_DI_INTERLACE:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_di_interlace = (VENDOR_VIDEO_PARAM_DEI_INTERLACE *)p_param;
		memcpy(&vendor_video_param[idx].di_interlace, p_di_interlace, sizeof(VENDOR_VIDEO_PARAM_DEI_INTERLACE));
		//vendor_die_set_die_en_p(path_id, &(vendor_video_param[idx].di_interlace.di_en));
		VENDOR_ISP_FLOW("vendor_video_set(VENDOR_VIDEO_DI_INTERLACE):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        di_en(%d)\n", vendor_video_param[idx].di_interlace.di_en);
		ret = vendor_die_set_die_info_p(path_id, &(vendor_video_param[idx].di_interlace));
		if (ret != HD_OK)
			goto err_exit;
		break;
	case VENDOR_VIDEO_MASK:
		p_mask = (VENDOR_VIDEO_PARAM_MASK *)p_param;
		memcpy(&vendor_video_param[idx].mask, p_mask, sizeof(VENDOR_VIDEO_PARAM_MASK));
		break;
	case VENDOR_VIDEO_LPM:
		p_lpm = (VENDOR_VIDEO_PARAM_LPM *)p_param;
		memcpy(&vendor_video_param[idx].lpm, p_lpm, sizeof(VENDOR_VIDEO_PARAM_LPM));
		break;
	case VENDOR_VIDEO_RND:
		p_rnd = (VENDOR_VIDEO_PARAM_RND *)p_param;
		memcpy(&vendor_video_param[idx].rnd, p_rnd, sizeof(VENDOR_VIDEO_PARAM_RND));
		break;
	case VENDOR_VIDEO_MOTION_AQ:
		p_aq = (VENDOR_VIDEO_PARAM_MOTION_AQ *)p_param;
		memcpy(&vendor_video_param[idx].aq, p_aq, sizeof(VENDOR_VIDEO_PARAM_MOTION_AQ));
		break;
	case VENDOR_VIDEO_DN_2D:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_mrnr = (VENDOR_VIDEO_PARAM_MRNR *)p_param;
		memcpy(&vendor_video_param[idx].vpe_mrnr, p_mrnr, sizeof(VENDOR_VIDEO_PARAM_MRNR));
		VENDOR_ISP_FLOW("vendor_video_set(VENDOR_VIDEO_DN_2D):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        vpe_mrnr_en(%d)\n", vendor_video_param[idx].vpe_mrnr.mrnr_en);
		ret = vendor_vpe_set_mrnr_en_p(path_id, &(vendor_video_param[idx].vpe_mrnr.mrnr_en));
		if (ret != HD_OK)
			goto err_exit;
		ret = vendor_vpe_set_mrnr_info_p(path_id, &(vendor_video_param[idx].vpe_mrnr));
		if (ret != HD_OK)
			goto err_exit;
		break;
	case VENDOR_VIDEO_SHARP:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_sharp = (VENDOR_VIDEO_PARAM_SHARP *)p_param;
		memcpy(&vendor_video_param[idx].sharp, p_sharp, sizeof(VENDOR_VIDEO_PARAM_SHARP));
		VENDOR_ISP_FLOW("vendor_video_set(VENDOR_VIDEO_SHARP):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        sharp_en(%d)\n", vendor_video_param[idx].sharp.sharp_en);
		ret = vendor_vpe_set_sharp_en_p(path_id, &(vendor_video_param[idx].sharp.sharp_en));
		if (ret != HD_OK)
			goto err_exit;
		ret = vendor_vpe_set_sharp_info_p(path_id, &(vendor_video_param[idx].sharp));
		if (ret != HD_OK)
			goto err_exit;
		break;
	case VENDOR_VIDEO_TMNR_CTRL:
		p_tmnr_ctrl = (VENDOR_VIDEO_PARAM_TMNR_EXT *)p_param;
		if ((dev_id & 0xff00) == HD_DAL_VIDEOENC_BASE) {
			memcpy(&vendor_video_param[idx].enc_dn, p_tmnr_ctrl, sizeof(VENDOR_VIDEO_PARAM_TMNR_EXT));
    		VENDOR_ISP_FLOW("vendor_video_set(VENDOR_VIDEO_TMNR_CTRL):\n");
    		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
    		VENDOR_ISP_FLOW("        enc_tmnr_en(%d)\n", vendor_video_param[idx].enc_dn.tmnr_en);
			ret = vendor_venc_set_tmnr_en_p(path_id, &(vendor_video_param[idx].enc_dn.tmnr_en));
			if (ret != HD_OK)
				goto err_exit;
			ret = vendor_venc_set_tmnr_param_p(path_id, &(vendor_video_param[idx].enc_dn));
			if (ret != HD_OK)
				goto err_exit;
		} else if ((dev_id & 0xff00) >= HD_DAL_VIDEOPROC_BASE && (dev_id & 0xff00) <= HD_DAL_VIDEOPROC_RESERVED_BASE) {
			memcpy(&vendor_video_param[idx].vpe_dn, p_tmnr_ctrl, sizeof(VENDOR_VIDEO_PARAM_TMNR_EXT));
    		VENDOR_ISP_FLOW("vendor_video_set(VENDOR_VIDEO_TMNR_CTRL):\n");
    		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
    		VENDOR_ISP_FLOW("        vpe_tmnr_en(%d)\n", vendor_video_param[idx].vpe_dn.tmnr_en);

			/* check in crop width when tmnr enable */
			vproc_param = vdoproc_param[VDOPROC_DEVID(dev_id)][VDOPROC_OUTID(out_id)];
			if (vproc_param == NULL) {
				VENDOR_ISP_ERROR("%s: path_id(%#x) set fail, it maybe no open.\n", __func__, path_id);
				ret = HD_ERR_NOT_OPEN;
				goto err_exit;
			}
			if (p_tmnr_ctrl->tmnr_en && (!IS_ALIGN32(vproc_param->src_rect.w) || !IS_ALIGN32(vproc_param->psr_crop_rect.w)) &&
				((vproc_param->src_fmt == HD_VIDEO_PXLFMT_YUV420_NVX3) || (vproc_param->src_fmt == HD_VIDEO_PXLFMT_YUV422_NVX3))) {
				VENDOR_ISP_ERROR("%s: path_id(%#x) HD_VIDEOPROC_CROP.win.rect.w(%u, %u) need 32 align with NVX3 pxlfmt when TMNR enable.\n",
						__func__, path_id, vproc_param->src_rect.w, vproc_param->psr_crop_rect.w);
				ret = HD_ERR_INV;
				goto err_exit;
			}
			vproc_param->tmnr_en = p_tmnr_ctrl->tmnr_en;
			ret = vendor_vpe_set_tmnr_en_p(path_id, &(vendor_video_param[idx].vpe_dn.tmnr_en));
			if (ret != HD_OK)
				goto err_exit;
			ret = vendor_vpe_set_tmnr_info_p(path_id, &(vendor_video_param[idx].vpe_dn));
			if (ret != HD_OK)
				goto err_exit;
		}
		break;
	case VENDOR_VIDEO_SCA:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_sca_set = (VENDOR_VIDEO_PARAM_SCA_SET *)p_param;
		memcpy(&vendor_video_param[idx].sca_set, p_sca_set, sizeof(VENDOR_VIDEO_PARAM_SCA_SET));
		ret = vendor_vpe_set_sca_info_p(path_id, &(vendor_video_param[idx].sca_set));
		if (ret != HD_OK)
			goto err_exit;
		break;
	case VENDOR_VIDEO_TMNR_MOTION_BUF:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to set param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_tmnr_motion_buf = (VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF *)p_param;
		memcpy(&vendor_video_param[idx].tmnr_motion_buf, p_tmnr_motion_buf, sizeof(VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF));
		VENDOR_ISP_FLOW("vendor_video_set(VENDOR_VIDEO_TMNR_MOTION_BUF):\n");
		VENDOR_ISP_FLOW("    path_id(%#x)\n", path_id);
		VENDOR_ISP_FLOW("        ddr_id(%d) p_start_pa(%#lx) size(%d)\n", vendor_video_param[idx].tmnr_motion_buf.ddr_id, \
            vendor_video_param[idx].tmnr_motion_buf.p_start_pa, vendor_video_param[idx].tmnr_motion_buf.size);
		ret = vendor_vpe_set_tmnr_motion_buf_p(path_id, &(vendor_video_param[idx].tmnr_motion_buf));
		if (ret != HD_OK)
			goto err_exit;
		break;
	case VENDOR_VIDEO_LCD_IQ:
		if ((dev_id & 0xff00) != HD_DAL_VIDEOOUT_BASE) {
			VENDOR_ISP_ERROR("Not support to set param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_lcd_iq = (VENDOR_VIDEO_PARAM_LCD_IQ *)p_param;
		memcpy(&vendor_video_param[idx].lcd_iq, p_lcd_iq, sizeof(VENDOR_VIDEO_PARAM_LCD_IQ));
		ret = vendor_lcd_set_iq_p(path_id, &(vendor_video_param[idx].lcd_iq));
		if (ret != HD_OK)
			goto err_exit;
		break;
   case VENDOR_VIDEO_LCD_HUE:
		if ((dev_id & 0xff00) != HD_DAL_VIDEOOUT_BASE) {
			VENDOR_ISP_ERROR("Not support to set param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_lcd_hue = (VENDOR_VIDEO_PARAM_LCD_HUE_SAT *)p_param;
		memcpy(&vendor_video_param[idx].lcd_hue, p_lcd_hue, sizeof(VENDOR_VIDEO_PARAM_LCD_HUE_SAT));
		ret = vendor_lcd_set_hue_p(path_id, &(vendor_video_param[idx].lcd_hue));
		if (ret != HD_OK)
			goto err_exit;
		break;
     case VENDOR_VIDEO_LCD_YGAMMA:
		if ((dev_id & 0xff00) != HD_DAL_VIDEOOUT_BASE) {
			VENDOR_ISP_ERROR("Not support to set param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_lcd_ygamma = (VENDOR_VIDEO_PARAM_LCD_YGAMMA *)p_param;
		memcpy(&vendor_video_param[idx].lcd_ygamma, p_lcd_ygamma, sizeof(VENDOR_VIDEO_PARAM_LCD_YGAMMA));
		ret = vendor_lcd_set_ygamma_p(path_id, &(vendor_video_param[idx].lcd_ygamma));
		if (ret != HD_OK)
			goto err_exit;
		break;  
	case VENDOR_VIDEO_YUV_RANGE:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_yuv_range = (VENDOR_VIDEO_PARAM_YUV_RANGE *)p_param;
		memcpy(&vendor_video_param[idx].yuv_range, p_yuv_range, sizeof(VENDOR_VIDEO_PARAM_YUV_RANGE));
		ret = vendor_vpe_set_sca_yuv_range_p(path_id, &(vendor_video_param[idx].yuv_range));
		if (ret != HD_OK)
			goto err_exit;
		break;
	case VENDOR_VIDEO_EDGE_SMOOTH:
		if ((dev_id & 0xff00) < HD_DAL_VIDEOPROC_BASE || (dev_id & 0xff00) > HD_DAL_VIDEOPROC_RESERVED_BASE) {
			VENDOR_ISP_ERROR("Not support to get param id(%d) for this path_id(%#x)\n", id, path_id);
			goto exit;
		}
		p_edge_smooth = (VENDOR_VIDEO_PARAM_EDGE_SMOOTH *)p_param;
		memcpy(&vendor_video_param[idx].edge_smooth, p_edge_smooth, sizeof(VENDOR_VIDEO_PARAM_EDGE_SMOOTH));
		ret = vendor_vpe_set_edge_smooth_p(path_id, &(vendor_video_param[idx].edge_smooth));
		if (ret != HD_OK)
			goto err_exit;
		break;
	default:
		VENDOR_ISP_ERROR("Not support to set param id(%d)\n", id);
		ret = HD_ERR_NG;
		goto exit;
	}

exit:
	pthread_mutex_unlock(&vendor_video_mutex);
	return ret;

err_exit:
	pthread_mutex_unlock(&vendor_video_mutex);
	VENDOR_ISP_ERROR("vendor_video: set parameter id(%d) error\n", id);
	return ret;
}

HD_RESULT vendor_video_init(HD_PATH_ID path_id)
{
	HD_RESULT ret = HD_OK;
	UINT vendor_video_version = VENDER_VIDEO_VERSION;
	INT idx;
	
	if (video_init_done == 0) {
		VENDOR_ISP_FLOW("VENDOR: Video Version: v%x.%x.%x\n",
				(vendor_video_version & 0xF00000) >> 20,
				(vendor_video_version & 0x0FFFF0) >> 4,
				(vendor_video_version & 0x00000F));
		video_init_done = 1;
		memset(&vendor_video_param, 0x0, sizeof(VENDOR_VIDEO_PARAM) * VENDER_VIDEO_MAX_CH_NUM);
	}

	pthread_mutex_lock(&vendor_video_mutex);
	if ((idx = vendor_video_get_idx(path_id)) < 0) {
		ret = HD_ERR_NOT_AVAIL;
		goto exit;
	}
	memset(&vendor_video_param[idx], 0x0, sizeof(VENDOR_VIDEO_PARAM));

	ret = vendor_isp_ioctl_open_p(path_id);
	if (ret != HD_OK) {
		VENDOR_ISP_ERROR("vendor_video: open ioctl fail, path_id(%#x) error\n", path_id);
		goto exit;
	}
	ret = vendor_die_set_init_p(path_id);
	if (ret != HD_OK) {
		VENDOR_ISP_ERROR("vendor_video: init die fail, path_id(%#x) error\n", path_id);
		goto exit;
	}

exit:
	pthread_mutex_unlock(&vendor_video_mutex);
	return ret;
}

HD_RESULT vendor_video_uninit(HD_PATH_ID path_id)
{
	HD_RESULT ret = HD_OK;

	pthread_mutex_lock(&vendor_video_mutex);
	VENDOR_ISP_FLOW("vendor_video_uninit(%#x):\n", (int)path_id);
	ret = vendor_die_set_uninit_p(path_id);
	if (ret != HD_OK) {
		VENDOR_ISP_ERROR("vendor_video: close ioctl fail, path_id(%#x) error\n", path_id);
		goto exit;
	}
	ret = vendor_isp_ioctl_close_p(path_id);
	if (ret != HD_OK) {
		VENDOR_ISP_ERROR("vendor_video: uninit die fail, path_id(%#x) error\n", path_id);
		goto exit;
	}

exit:
	pthread_mutex_unlock(&vendor_video_mutex);
	return ret;
}
/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
