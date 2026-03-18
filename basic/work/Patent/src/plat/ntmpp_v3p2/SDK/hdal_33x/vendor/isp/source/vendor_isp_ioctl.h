/**
	@brief Header file of vendor ioctl isp definition.
	This file contains the functions which is related to ioctl isp definition.

	@file vendor_isp_ioctl.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_ISP_IOCTL_H_
#define _VENDOR_ISP_IOCTL_H_

/********************************************************************
INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "hd_util.h"
#include "vendor_isp_common.h"

/********************************************************************
MACRO CONSTANT DEFINITIONS
********************************************************************/
#define VDOENC_DEVID(self_id)   (self_id - HD_DAL_VIDEOENC_BASE)
#define VDOENC_OUTID(id)        (id - HD_OUT_BASE)
#define VDOPROC_DEVID(self_id)	(self_id - HD_DAL_VIDEOPROC_BASE)
#define VDOPROC_OUTID(id)    	(id - HD_OUT_BASE)
#define VDOOUT_DEVID(self_id)	(self_id - HD_DAL_VIDEOOUT_BASE)

/********************************************************************
MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
TYPE DEFINITION
********************************************************************/


/********************************************************************
EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
/* chip query functions*/
HD_RESULT vendor_vpe_query_chip_p(HD_PATH_ID path_id, INT *p_chip);
HD_RESULT vendor_enc_query_chip_p(HD_PATH_ID path_id, INT *p_chip);

/* venc nr ioctl functions*/
HD_RESULT vendor_venc_set_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_venc_get_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_venc_set_tmnr_param_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_EXT *param);
HD_RESULT vendor_venc_get_tmnr_param_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_EXT *param);

/* vpe nr ioctl functions*/
HD_RESULT vendor_vpe_get_mrnr_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_vpe_set_mrnr_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_vpe_set_mrnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_MRNR *param);
HD_RESULT vendor_vpe_get_mrnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_MRNR *param);
HD_RESULT vendor_vpe_get_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_vpe_set_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_vpe_set_tmnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_EXT *param);
HD_RESULT vendor_vpe_get_tmnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_EXT *param);

/* vpe sharp ioctl functions*/
HD_RESULT vendor_vpe_get_sharp_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_vpe_set_sharp_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_vpe_set_sharp_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_SHARP *param);
HD_RESULT vendor_vpe_get_sharp_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_SHARP *param);

/* vpe tmnr buffer ioctl functions*/
HD_RESULT vendor_vpe_get_tmnr_map_idx_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_MAP_IDX *param);
HD_RESULT vendor_vpe_set_tmnr_map_idx_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_MAP_IDX *param);
HD_RESULT vendor_vpe_get_tmnr_sta_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_STA_INFO *param);
HD_RESULT vendor_vpe_get_tmnr_sta_data_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_STA_DATA *param);

HD_RESULT vendor_vpe_get_tmnr_motion_buf_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF *param);
HD_RESULT vendor_vpe_set_tmnr_motion_buf_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF *param);

/* vpe sca info ioctl functions*/
HD_RESULT vendor_vpe_get_sca_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_SCA_SET *param);
HD_RESULT vendor_vpe_set_sca_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_SCA_SET *param);
HD_RESULT vendor_vpe_get_sca_yuv_range_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_YUV_RANGE *param);
HD_RESULT vendor_vpe_set_sca_yuv_range_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_YUV_RANGE *param);
HD_RESULT vendor_vpe_get_edge_smooth_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_EDGE_SMOOTH *param);
HD_RESULT vendor_vpe_set_edge_smooth_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_EDGE_SMOOTH *param);

/* die ioctl functions*/
HD_RESULT vendor_die_get_die_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DEI_INTERLACE *param);
HD_RESULT vendor_die_set_die_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DEI_INTERLACE *param);
HD_RESULT vendor_die_get_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_die_set_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_die_get_tmnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DI_TMNR *param);
HD_RESULT vendor_die_set_tmnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DI_TMNR *param);
HD_RESULT vendor_die_get_gmm_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_die_set_gmm_en_p(HD_PATH_ID path_id, UINT32 *param);
HD_RESULT vendor_die_get_gmm_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DI_GMM *param);
HD_RESULT vendor_die_set_gmm_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DI_GMM *param);
HD_RESULT vendor_die_set_init_p(HD_PATH_ID path_id);
HD_RESULT vendor_die_set_uninit_p(HD_PATH_ID path_id);

/* lcd ioctl functions*/
HD_RESULT vendor_lcd_get_iq_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_IQ *param);
HD_RESULT vendor_lcd_set_iq_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_IQ *param);
HD_RESULT vendor_lcd_get_hue_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_HUE_SAT *param);
HD_RESULT vendor_lcd_set_hue_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_HUE_SAT *param);
HD_RESULT vendor_lcd_get_ygamma_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_YGAMMA *param);
HD_RESULT vendor_lcd_set_ygamma_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_YGAMMA *param);

HD_RESULT vendor_isp_ioctl_open_p(HD_PATH_ID path_id);
HD_RESULT vendor_isp_ioctl_close_p(HD_PATH_ID path_id);

#endif  /* _VENDOR_IOCTL_H_ */
