/**
	@brief Source file of vendor isp ioctl.\n
	This file contains the functions which is related to vendor isp ioctl interface.

	@file vendor_isp_ioctl.c

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
#include <sys/ioctl.h>
#include "hd_type.h"
#include "hdal.h"
//#include "vg_common.h"
#include "vendor_isp_ioctl.h"
#include "vendor_isp_common.h"
#include "vpe_ioctl.h"
#include "di_ioctl.h"
#include "kflow_videoenc_h26x.h"
#include "lcd300/lcd300_fb.h"			
#include "lcd200_v3/lcd200_fb.h"		

/*-----------------------------------------------------------------------------*/
/* External Global Variables                                                   */
/*-----------------------------------------------------------------------------*/
#define VIDEOPROC_DEVICE "/dev/kflow_vpe"
#define VIDEOENC_DEVICE "/dev/videoenc"
#define VIDEODIE_DEVICE "/dev/kflow_dei"
#define VIDEOOUT_DEVICE0 "/dev/fb0"
#define VIDEOOUT_DEVICE1 "/dev/fb3"
#define VIDEOOUT_DEVICE2 "/dev/fb6"

INT isp_vpe_ioctl = 0;
INT isp_enc_ioctl = 0;
INT isp_die_ioctl = 0;
INT isp_lcd0_ioctl = 0;
INT isp_lcd1_ioctl = 0;
INT isp_lcd2_ioctl = 0;

/*-----------------------------------------------------------------------------*/
/* Internal Global Variables                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Internal Function Prototype                                                 */
/*-----------------------------------------------------------------------------*/
extern UINT32 utl_get_vpe_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param);
extern UINT32 utl_get_enc_entity_fd(HD_DAL device_subid, HD_IO in_subid, VOID *param);
extern UINT32 utl_get_di_entity_fd(HD_DAL device_subid, HD_IO out_subid, VOID *param);
extern UINT32 utl_get_lcd_entity_fd(INT32 chip, HD_DAL device_subid, HD_IO in_subid, VOID *param);
extern int  pif_address_ddr_sanity_check(UINTPTR pa, HD_COMMON_MEM_DDR_ID ddrid);
/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_vpe_query_chip_p(HD_PATH_ID path_id, INT *p_chip)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT fd = 0;

	fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	if (fd == 0) {
		VENDOR_ISP_ERROR("vendor_vpe_query_chip fail\n");
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	*p_chip = ENTITY_FD_CHIP(fd);

err_ext:
	return hd_ret;
}

HD_RESULT vendor_enc_query_chip_p(HD_PATH_ID path_id, INT *p_chip)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT fd = 0;

	fd = utl_get_enc_entity_fd(VDOENC_DEVID(dev_id), VDOENC_OUTID(out_id), NULL);

	if (fd == 0) {
		VENDOR_ISP_ERROR("vendor_enc_query_chip fail\n");
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	*p_chip = ENTITY_FD_CHIP(fd);

err_ext:
	return hd_ret;
}

HD_RESULT vendor_venc_set_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	FD_H26XENC_EN_INFO en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_enc_ioctl == 0) {
		isp_enc_ioctl = open(VIDEOENC_DEVICE, O_RDWR);
		if (!isp_enc_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOENC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(FD_H26XENC_EN_INFO));
	en_info.fd = utl_get_enc_entity_fd(VDOENC_DEVID(dev_id), VDOENC_OUTID(out_id), NULL);
	en_info.enable = *param;

	ret = ioctl(isp_enc_ioctl, H26X_ENC_IOC_SET_TMNR_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"H26X_ENC_IOC_SET_TMNR_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_venc_get_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	FD_H26XENC_EN_INFO en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_enc_ioctl == 0) {
		isp_enc_ioctl = open(VIDEOENC_DEVICE, O_RDWR);
		if (!isp_enc_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOENC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(FD_H26XENC_EN_INFO));
	en_info.fd = utl_get_enc_entity_fd(VDOENC_DEVID(dev_id), VDOENC_OUTID(out_id), NULL);

	ret = ioctl(isp_enc_ioctl, H26X_ENC_IOC_GET_TMNR_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"H26X_ENC_IOC_GET_TMNR_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	*param = en_info.enable;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_venc_set_tmnr_param_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_EXT *param)
{
	HD_RESULT hd_ret = HD_OK;
	FD_H26XENC_TMNR_PARAM tmnr;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0, i;

	if (isp_enc_ioctl == 0) {
		isp_enc_ioctl = open(VIDEOENC_DEVICE, O_RDWR);
		if (!isp_enc_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOENC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&tmnr, 0x0, sizeof(FD_H26XENC_TMNR_PARAM));
	tmnr.fd = utl_get_enc_entity_fd(VDOENC_DEVID(dev_id), VDOENC_OUTID(out_id), NULL);
	tmnr.st_tmnr_param.luma_dn_en = param->luma_dn_en;
	tmnr.st_tmnr_param.chroma_dn_en = param->chroma_dn_en;
	tmnr.st_tmnr_param.nr_str_y_3d = param->nr_str_y_3d;
	tmnr.st_tmnr_param.nr_str_y_2d = param->nr_str_y_2d;
	tmnr.st_tmnr_param.nr_str_c_3d = param->nr_str_c_3d;
	tmnr.st_tmnr_param.nr_str_c_2d = param->nr_str_c_2d;
	tmnr.st_tmnr_param.blur_str_y = param->blur_str_y;
	tmnr.st_tmnr_param.center_wzero_y_2d_en = param->center_wzero_y_2d_en;
	tmnr.st_tmnr_param.center_wzero_y_3d_en = param->center_wzero_y_3d_en;
	tmnr.st_tmnr_param.small_vibrat_supp_y_en = param->small_vibrat_supp_y_en;
	tmnr.st_tmnr_param.avoid_residue_th_y = param->avoid_residue_th_y;
	tmnr.st_tmnr_param.avoid_residue_th_c = param->avoid_residue_th_c;
	tmnr.st_tmnr_param.motion_level_th_y_k1 = param->motion_level_th_y_k1;
	tmnr.st_tmnr_param.motion_level_th_y_k2 = param->motion_level_th_y_k2;
	tmnr.st_tmnr_param.motion_level_th2_y_k1 = param->motion_level_th_y_k1;
	tmnr.st_tmnr_param.motion_level_th2_y_k2 = param->motion_level_th_y_k2;
	tmnr.st_tmnr_param.motion_level_th_c_k1 = param->motion_level_th_c_k1;
	tmnr.st_tmnr_param.motion_level_th_c_k2 = param->motion_level_th_c_k2;
	for (i = 0; i < 4; i++) {
		tmnr.st_tmnr_param.lut_y_3d_1_th[i] = param->lut_y_3d_1_th[i];
		tmnr.st_tmnr_param.lut_y_3d_2_th[i] = param->lut_y_3d_2_th[i];
		tmnr.st_tmnr_param.lut_y_2d_th[i] = param->lut_y_2d_th[i];
		tmnr.st_tmnr_param.lut_c_3d_th[i] = param->lut_c_3d_th[i];
		tmnr.st_tmnr_param.lut_c_2d_th[i] = param->lut_c_2d_th[i];
	}
	for (i = 0; i < 8; i++) {
		tmnr.st_tmnr_param.y_base[i] = param->y_base[i];
		tmnr.st_tmnr_param.y_coefa[i] = param->y_coefa[i];
		tmnr.st_tmnr_param.y_coefb[i] = param->y_coefb[i];
		tmnr.st_tmnr_param.y_std[i] = param->y_std[i];
		tmnr.st_tmnr_param.cb_mean[i] = param->cb_mean[i];
		tmnr.st_tmnr_param.cb_std[i] = param->cb_std[i];
		tmnr.st_tmnr_param.cr_mean[i] = param->cr_mean[i];
		tmnr.st_tmnr_param.cr_std[i] = param->cr_std[i];
	}
	tmnr.st_tmnr_param.tmnr_fcs_en = param->tmnr_fcs_en;
	tmnr.st_tmnr_param.tmnr_fcs_str = param->tmnr_fcs_str;
	tmnr.st_tmnr_param.tmnr_fcs_th = param->tmnr_fcs_th;
	tmnr.st_tmnr_param.dithering_enable = param->dithering_en;
	tmnr.st_tmnr_param.dithering_bit_y = param->dithering_bit_y;
	tmnr.st_tmnr_param.dithering_bit_cb = param->dithering_bit_u;
	tmnr.st_tmnr_param.dithering_bit_cr = param->dithering_bit_v;
	tmnr.st_tmnr_param.ref_cmpr_en = 1;
	tmnr.st_tmnr_param.err_compensate = param->err_compensate;

	ret = ioctl(isp_enc_ioctl, H26X_ENC_IOC_SET_TMNR_PARAM, &tmnr);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"H26X_ENC_IOC_SET_TMNR_PARAM\" return %d, fd(%#x)\n", ret, tmnr.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_venc_get_tmnr_param_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_EXT *param)
{
	HD_RESULT hd_ret = HD_OK;
	FD_H26XENC_TMNR_PARAM tmnr;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0, i;

	if (isp_enc_ioctl == 0) {
		isp_enc_ioctl = open(VIDEOENC_DEVICE, O_RDWR);
		if (!isp_enc_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOENC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&tmnr, 0x0, sizeof(FD_H26XENC_TMNR_PARAM));
	tmnr.fd = utl_get_enc_entity_fd(VDOENC_DEVID(dev_id), VDOENC_OUTID(out_id), NULL);

	ret = ioctl(isp_enc_ioctl, H26X_ENC_IOC_GET_TMNR_PARAM, &tmnr);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"H26X_ENC_IOC_GET_TMNR_PARAM\" return %d, fd(%#x)\n", ret, tmnr.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	param->luma_dn_en = tmnr.st_tmnr_param.luma_dn_en;
	param->chroma_dn_en = tmnr.st_tmnr_param.chroma_dn_en;
	param->nr_str_y_3d = tmnr.st_tmnr_param.nr_str_y_3d;
	param->nr_str_y_2d = tmnr.st_tmnr_param.nr_str_y_2d;
	param->nr_str_c_3d = tmnr.st_tmnr_param.nr_str_c_3d;
	param->nr_str_c_2d = tmnr.st_tmnr_param.nr_str_c_2d;
	param->blur_str_y = tmnr.st_tmnr_param.blur_str_y;
	param->center_wzero_y_2d_en = tmnr.st_tmnr_param.center_wzero_y_2d_en;
	param->center_wzero_y_3d_en = tmnr.st_tmnr_param.center_wzero_y_3d_en;
	param->small_vibrat_supp_y_en = tmnr.st_tmnr_param.small_vibrat_supp_y_en;
	param->avoid_residue_th_y = tmnr.st_tmnr_param.avoid_residue_th_y;
	param->avoid_residue_th_c = tmnr.st_tmnr_param.avoid_residue_th_c;
	param->motion_level_th_y_k1 = tmnr.st_tmnr_param.motion_level_th_y_k1;
	param->motion_level_th_y_k2 = tmnr.st_tmnr_param.motion_level_th_y_k2;
	param->motion_level_th_c_k1 = tmnr.st_tmnr_param.motion_level_th_c_k1;
	param->motion_level_th_c_k2 = tmnr.st_tmnr_param.motion_level_th_c_k2;
	for (i = 0; i < 4; i++) {
		param->lut_y_3d_1_th[i] = tmnr.st_tmnr_param.lut_y_3d_1_th[i];
		param->lut_y_3d_2_th[i] = tmnr.st_tmnr_param.lut_y_3d_2_th[i];
		param->lut_y_2d_th[i] = tmnr.st_tmnr_param.lut_y_2d_th[i];
		param->lut_c_3d_th[i] = tmnr.st_tmnr_param.lut_c_3d_th[i];
		param->lut_c_2d_th[i] = tmnr.st_tmnr_param.lut_c_2d_th[i];
	}
	for (i = 0; i < 8; i++) {
		param->y_base[i] = tmnr.st_tmnr_param.y_base[i];
		param->y_coefa[i] = tmnr.st_tmnr_param.y_coefa[i];
		param->y_coefb[i] = tmnr.st_tmnr_param.y_coefb[i];
		param->y_std[i] = tmnr.st_tmnr_param.y_std[i];
		param->cb_mean[i] = tmnr.st_tmnr_param.cb_mean[i];
		param->cb_std[i] = tmnr.st_tmnr_param.cb_std[i];
		param->cr_mean[i] = tmnr.st_tmnr_param.cr_mean[i];
		param->cr_std[i] = tmnr.st_tmnr_param.cr_std[i];
	}
	param->tmnr_fcs_en = tmnr.st_tmnr_param.tmnr_fcs_en;
	param->tmnr_fcs_str = tmnr.st_tmnr_param.tmnr_fcs_str;
	param->tmnr_fcs_th = tmnr.st_tmnr_param.tmnr_fcs_th;
	param->dithering_en = tmnr.st_tmnr_param.dithering_enable;
	param->dithering_bit_y = tmnr.st_tmnr_param.dithering_bit_y;
	param->dithering_bit_u = tmnr.st_tmnr_param.dithering_bit_cb;
	param->dithering_bit_v = tmnr.st_tmnr_param.dithering_bit_cr;
	//param->... = tmnr.st_tmnr_param.ref_cmpr_en;
	param->err_compensate = tmnr.st_tmnr_param.err_compensate;


err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_mrnr_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_en_info_t en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(fd_en_info_t));
	en_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_MRNR_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_MRNR_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	*param = en_info.enable;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_set_mrnr_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_en_info_t en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(fd_en_info_t));
	en_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	en_info.enable = *param;

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_MRNR_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_MRNR_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_mrnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_MRNR *param)
{
	HD_RESULT hd_ret = HD_OK;

	fd_mrnr_param_t mrnr;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1, i, j;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&mrnr, 0x0, sizeof(fd_mrnr_param_t));
	mrnr.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_MRNR_INFO, &mrnr);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_MRNR_INFO\" return %d, fd(%#x)\n", ret, mrnr.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 8; j++) {
			param->t_y_edge_detection[i][j] = mrnr.param.t_y_edge_detection[i][j];
			param->t_y_edge_smoothing[i][j] = mrnr.param.t_y_edge_smoothing[i][j];

		}
	}
	for (i = 0; i < 2; i++) {
		param->nr_strength_y[i] = mrnr.param.nr_strength_y[i];
	}
	param->t_cb_edge_detection = mrnr.param.t_cb_edge_detection;
	param->t_cr_edge_detection = mrnr.param.t_cr_edge_detection;
	param->t_cb_edge_smoothing = mrnr.param.t_cb_edge_smoothing;
	param->t_cr_edge_smoothing = mrnr.param.t_cr_edge_smoothing;
	param->nr_strength_c = mrnr.param.nr_strength_c;


err_ext:

	return hd_ret;
}

HD_RESULT vendor_vpe_set_mrnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_MRNR *param)
{
	HD_RESULT hd_ret = HD_OK;

	fd_mrnr_param_t mrnr;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1, i, j;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&mrnr, 0x0, sizeof(fd_mrnr_param_t));
	mrnr.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 8; j++) {
			mrnr.param.t_y_edge_detection[i][j] = param->t_y_edge_detection[i][j];
			mrnr.param.t_y_edge_smoothing[i][j] = param->t_y_edge_smoothing[i][j];
		}
	}
	for (i = 0; i < 2; i++) {
		mrnr.param.nr_strength_y[i] = param->nr_strength_y[i];
	}
	mrnr.param.t_cb_edge_detection = param->t_cb_edge_detection;
	mrnr.param.t_cr_edge_detection = param->t_cr_edge_detection;
	mrnr.param.t_cb_edge_smoothing = param->t_cb_edge_smoothing;
	mrnr.param.t_cr_edge_smoothing = param->t_cr_edge_smoothing;
	mrnr.param.nr_strength_c = param->nr_strength_c;

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_MRNR_INFO, &mrnr);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_MRNR_INFO\" return %d, fd(%#x)\n", ret, mrnr.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}


err_ext:

	return hd_ret;
}

HD_RESULT vendor_vpe_get_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_en_info_t en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(fd_en_info_t));
	en_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_TMNR_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_TMNR_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	*param = en_info.enable;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_set_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_en_info_t en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}	

	memset(&en_info, 0x0, sizeof(fd_en_info_t));
	en_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	en_info.enable = *param;

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_TMNR_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_TMNR_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_tmnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_EXT *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_tmnr_param_t tmnr;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1, i;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&tmnr, 0x0, sizeof(fd_tmnr_param_t));
	tmnr.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_TMNR_INFO, &tmnr);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_TMNR_INFO\" return %d, fd(%#x)\n", ret, tmnr.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	param->luma_dn_en = tmnr.param.luma_dn_en;
	param->chroma_dn_en = tmnr.param.chroma_dn_en;
	param->nr_str_y_3d = tmnr.param.nr_str_y_3d;
	param->nr_str_y_2d = tmnr.param.nr_str_y_2d;
	param->nr_str_c_3d = tmnr.param.nr_str_c_3d;
	param->nr_str_c_2d = tmnr.param.nr_str_c_2d;
	param->blur_str_y = tmnr.param.blur_str_y;
	param->center_wzero_y_2d_en = tmnr.param.center_wzero_y_2d_en;
	param->center_wzero_y_3d_en = tmnr.param.center_wzero_y_3d_en;
	param->small_vibrat_supp_y_en = tmnr.param.small_vibrat_supp_y_en;
	param->avoid_residue_th_y = tmnr.param.avoid_residue_th_y;
	param->avoid_residue_th_c = tmnr.param.avoid_residue_th_c;
	param->motion_level_th_y_k1 = tmnr.param.motion_level_th_y_k1;
	param->motion_level_th_y_k2 = tmnr.param.motion_level_th_y_k2;
	param->motion_level_th_c_k1 = tmnr.param.motion_level_th_c_k1;
	param->motion_level_th_c_k2 = tmnr.param.motion_level_th_c_k2;
	for (i = 0; i < 4; i++) {
		param->lut_y_3d_1_th[i] = tmnr.param.lut_y_3d_1_th[i];
		param->lut_y_3d_2_th[i] = tmnr.param.lut_y_3d_2_th[i];
		param->lut_y_2d_th[i] = tmnr.param.lut_y_2d_th[i];
		param->lut_c_3d_th[i] = tmnr.param.lut_c_3d_th[i];
		param->lut_c_2d_th[i] = tmnr.param.lut_c_2d_th[i];
	}
	for (i = 0; i < 8; i++) {
		param->y_base[i] = tmnr.param.y_base[i];
		param->y_coefa[i] = tmnr.param.y_coefa[i];
		param->y_coefb[i] = tmnr.param.y_coefb[i];
		param->y_std[i] = tmnr.param.y_std[i];
		param->cb_mean[i] = tmnr.param.cb_mean[i];
		param->cb_std[i] = tmnr.param.cb_std[i];
		param->cr_mean[i] = tmnr.param.cr_mean[i];
		param->cr_std[i] = tmnr.param.cr_std[i];
	}
	param->tmnr_fcs_en = tmnr.param.tmnr_fcs_en;
	param->tmnr_fcs_str = tmnr.param.tmnr_fcs_str;
	param->tmnr_fcs_th = tmnr.param.tmnr_fcs_th;
	param->dithering_en = tmnr.param.dithering_en;
	param->dithering_bit_y = tmnr.param.dithering_bit_y;
	param->dithering_bit_u = tmnr.param.dithering_bit_u;
	param->dithering_bit_v = tmnr.param.dithering_bit_v;
	param->err_compensate = tmnr.param.err_compensate;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_set_tmnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_EXT *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_tmnr_param_t tmnr;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1, i;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&tmnr, 0x0, sizeof(fd_tmnr_param_t));
	tmnr.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	tmnr.param.luma_dn_en = param->luma_dn_en;
	tmnr.param.chroma_dn_en = param->chroma_dn_en;
	tmnr.param.nr_str_y_3d = param->nr_str_y_3d;
	tmnr.param.nr_str_y_2d = param->nr_str_y_2d;
	tmnr.param.nr_str_c_3d = param->nr_str_c_3d;
	tmnr.param.nr_str_c_2d = param->nr_str_c_2d;
	tmnr.param.blur_str_y = param->blur_str_y;
	tmnr.param.center_wzero_y_2d_en = param->center_wzero_y_2d_en;
	tmnr.param.center_wzero_y_3d_en = param->center_wzero_y_3d_en;
	tmnr.param.small_vibrat_supp_y_en = param->small_vibrat_supp_y_en;
	tmnr.param.avoid_residue_th_y = param->avoid_residue_th_y;
	tmnr.param.avoid_residue_th_c = param->avoid_residue_th_c;
	tmnr.param.motion_level_th_y_k1 = param->motion_level_th_y_k1;
	tmnr.param.motion_level_th_y_k2 = param->motion_level_th_y_k2;
	tmnr.param.motion_level_th_c_k1 = param->motion_level_th_c_k1;
	tmnr.param.motion_level_th_c_k2 = param->motion_level_th_c_k2;
	for (i = 0; i < 4; i++) {
		tmnr.param.lut_y_3d_1_th[i] = param->lut_y_3d_1_th[i];
		tmnr.param.lut_y_3d_2_th[i] = param->lut_y_3d_2_th[i];
		tmnr.param.lut_y_2d_th[i] = param->lut_y_2d_th[i];
		tmnr.param.lut_c_3d_th[i] = param->lut_c_3d_th[i];
		tmnr.param.lut_c_2d_th[i] = param->lut_c_2d_th[i];
	}
	for (i = 0; i < 8; i++) {
		tmnr.param.y_base[i] = param->y_base[i];
		tmnr.param.y_coefa[i] = param->y_coefa[i];
		tmnr.param.y_coefb[i] = param->y_coefb[i];
		tmnr.param.y_std[i] = param->y_std[i];
		tmnr.param.cb_mean[i] = param->cb_mean[i];
		tmnr.param.cb_std[i] = param->cb_std[i];
		tmnr.param.cr_mean[i] = param->cr_mean[i];
		tmnr.param.cr_std[i] = param->cr_std[i];
	}
	tmnr.param.tmnr_fcs_en = param->tmnr_fcs_en;
	tmnr.param.tmnr_fcs_str = param->tmnr_fcs_str;
	tmnr.param.tmnr_fcs_th = param->tmnr_fcs_th;
	tmnr.param.dithering_en = param->dithering_en;
	tmnr.param.dithering_bit_y = param->dithering_bit_y;
	tmnr.param.dithering_bit_u = param->dithering_bit_u;
	tmnr.param.dithering_bit_v = param->dithering_bit_v;
	tmnr.param.err_compensate = param->err_compensate;

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_TMNR_INFO, &tmnr);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_TMNR_INFO\" return %d, fd(%#x)\n", ret, tmnr.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_sharp_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_en_info_t en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(fd_en_info_t));
	en_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_SHARPEN_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_SHARPEN_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	*param = en_info.enable;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_set_sharp_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_en_info_t en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(fd_en_info_t));
	en_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	en_info.enable = *param;

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_SHARPEN_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_SHARPEN_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_sharp_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_SHARP *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_sharpen_param_t sharp;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1, i;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&sharp, 0x0, sizeof(fd_sharpen_param_t));
	sharp.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_SHARPEN_INFO, &sharp);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_SHARPEN_INFO\" return %d, fd(%#x)\n", ret, sharp.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	param->edge_weight_src_sel = sharp.param.edge_weight_src_sel;
	param->edge_weight_th = sharp.param.edge_weight_th;
	param->edge_weight_gain = sharp.param.edge_weight_gain;
	param->noise_level = sharp.param.noise_level;
	param->blend_inv_gamma = sharp.param.blend_inv_gamma;
	param->edge_sharp_str1 = sharp.param.edge_sharp_str1;
	param->edge_sharp_str2 = sharp.param.edge_sharp_str2;
	param->flat_sharp_str = sharp.param.flat_sharp_str;
	param->coring_th = sharp.param.coring_th;
	param->bright_halo_clip = sharp.param.bright_halo_clip;
	param->dark_halo_clip = sharp.param.dark_halo_clip;
	for (i = 0; i < 17; i++) {
		param->noise_curve[i] = sharp.param.noise_curve[i];
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_set_sharp_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_SHARP *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_sharpen_param_t sharp;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1, i;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&sharp, 0x0, sizeof(fd_sharpen_param_t));
	sharp.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	sharp.param.edge_weight_src_sel = param->edge_weight_src_sel;
	sharp.param.edge_weight_th = param->edge_weight_th;
	sharp.param.edge_weight_gain = param->edge_weight_gain;
	sharp.param.noise_level = param->noise_level;
	sharp.param.blend_inv_gamma = param->blend_inv_gamma;
	sharp.param.edge_sharp_str1 = param->edge_sharp_str1;
	sharp.param.edge_sharp_str2 = param->edge_sharp_str2;
	sharp.param.flat_sharp_str = param->flat_sharp_str;
	sharp.param.coring_th = param->coring_th;
	sharp.param.bright_halo_clip = param->bright_halo_clip;
	sharp.param.dark_halo_clip = param->dark_halo_clip;
	for (i = 0; i < 17; i++) {
		sharp.param.noise_curve[i] = param->noise_curve[i];
	}

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_SHARPEN_INFO, &sharp);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_SHARPEN_INFO\" return %d, fd(%#x)\n", ret, sharp.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}


err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_tmnr_map_idx_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_MAP_IDX *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_tmnr_map_idx_t map;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&map, 0x0, sizeof(fd_tmnr_map_idx_t));
	map.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_TMNR_MAP_INDEX, &map);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_TMNR_MAP_INDEX\" return %d, fd(%#x)\n", ret, map.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	param->map_en = map.map_en;
	param->map_idx = map.map_idx;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_set_tmnr_map_idx_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_MAP_IDX *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_tmnr_map_idx_t map;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&map, 0x0, sizeof(fd_tmnr_map_idx_t));
	map.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	map.map_en = param->map_en;
	map.map_idx = param->map_idx;

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_TMNR_MAP_INDEX, &map);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_TMNR_MAP_INDEX\" return %d, fd(%#x)\n", ret, map.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_tmnr_sta_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_STA_INFO *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_tmnr_sta_info_t sta_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&sta_info, 0x0, sizeof(fd_tmnr_sta_info_t));
	sta_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_TMNR_STA_INFO, &sta_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_TMNR_STA_INFO\" return %d, fd(%#x)\n", ret, sta_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	param->blk_no = sta_info.blk_no;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_tmnr_sta_data_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_STA_DATA *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_tmnr_sta_data_t sta_data;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&sta_data, 0x0, sizeof(fd_tmnr_sta_data_t));
	sta_data.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_TMNR_STA_DATA, &sta_data);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_TMNR_STA_DATA\" return %d, fd(%#x)\n", ret, sta_data.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	param->timeout_int = sta_data.timeout_int;
	param->p_stabuf = (void *)sta_data.p_stabuf;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_sca_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_SCA_SET *param)
{
	HD_RESULT hd_ret = HD_OK;
	sca_info_set_t sca_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1, i;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}
	memset(&sca_info, 0x0, sizeof(sca_info_set_t));
	sca_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_SCA_INFO, &sca_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_SCA_INFO\" return %d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	param->sca_y_luma_algo_en = sca_info.sca_y_luma_algo_en;
	param->sca_x_luma_algo_en = sca_info.sca_x_luma_algo_en;
	param->sca_y_chroma_algo_en = sca_info.sca_y_chroma_algo_en;
	param->sca_x_chroma_algo_en = sca_info.sca_x_chroma_algo_en;
	param->sca_map_sel = sca_info.sca_map_sel;
	for (i = 0; i < 4; i++) {
		param->sca_1000x_param.sca_ceffH[i] = sca_info.sca_1000x_param.sca_ceffH[i];
		param->sca_1000x_param.sca_ceffV[i] = sca_info.sca_1000x_param.sca_ceffV[i];
		param->sca_1250x_param.sca_ceffH[i] = sca_info.sca_1250x_param.sca_ceffH[i];
		param->sca_1250x_param.sca_ceffV[i] = sca_info.sca_1250x_param.sca_ceffV[i];
		param->sca_1500x_param.sca_ceffH[i] = sca_info.sca_1500x_param.sca_ceffH[i];
		param->sca_1500x_param.sca_ceffV[i] = sca_info.sca_1500x_param.sca_ceffV[i];
		param->sca_1750x_param.sca_ceffH[i] = sca_info.sca_1750x_param.sca_ceffH[i];
		param->sca_1750x_param.sca_ceffV[i] = sca_info.sca_1750x_param.sca_ceffV[i];
		param->sca_2000x_param.sca_ceffH[i] = sca_info.sca_2000x_param.sca_ceffH[i];
		param->sca_2000x_param.sca_ceffV[i] = sca_info.sca_2000x_param.sca_ceffV[i];
		param->sca_2250x_param.sca_ceffH[i] = sca_info.sca_2250x_param.sca_ceffH[i];
		param->sca_2250x_param.sca_ceffV[i] = sca_info.sca_2250x_param.sca_ceffV[i];
		param->sca_2500x_param.sca_ceffH[i] = sca_info.sca_2500x_param.sca_ceffH[i];
		param->sca_2500x_param.sca_ceffV[i] = sca_info.sca_2500x_param.sca_ceffV[i];
		param->sca_2750x_param.sca_ceffH[i] = sca_info.sca_2750x_param.sca_ceffH[i];
		param->sca_2750x_param.sca_ceffV[i] = sca_info.sca_2750x_param.sca_ceffV[i];
		param->sca_3000x_param.sca_ceffH[i] = sca_info.sca_3000x_param.sca_ceffH[i];
		param->sca_3000x_param.sca_ceffV[i] = sca_info.sca_3000x_param.sca_ceffV[i];
		param->sca_3250x_param.sca_ceffH[i] = sca_info.sca_3250x_param.sca_ceffH[i];
		param->sca_3250x_param.sca_ceffV[i] = sca_info.sca_3250x_param.sca_ceffV[i];
		param->sca_3500x_param.sca_ceffH[i] = sca_info.sca_3500x_param.sca_ceffH[i];
		param->sca_3500x_param.sca_ceffV[i] = sca_info.sca_3500x_param.sca_ceffV[i];
		param->sca_3750x_param.sca_ceffH[i] = sca_info.sca_3750x_param.sca_ceffH[i];
		param->sca_3750x_param.sca_ceffV[i] = sca_info.sca_3750x_param.sca_ceffV[i];
		param->sca_4000x_param.sca_ceffH[i] = sca_info.sca_4000x_param.sca_ceffH[i];
		param->sca_4000x_param.sca_ceffV[i] = sca_info.sca_4000x_param.sca_ceffV[i];
		param->sca_5000x_param.sca_ceffH[i] = sca_info.sca_5000x_param.sca_ceffH[i];
		param->sca_5000x_param.sca_ceffV[i] = sca_info.sca_5000x_param.sca_ceffV[i];
		param->sca_6000x_param.sca_ceffH[i] = sca_info.sca_6000x_param.sca_ceffH[i];
		param->sca_6000x_param.sca_ceffV[i] = sca_info.sca_6000x_param.sca_ceffV[i];
		param->sca_7000x_param.sca_ceffH[i] = sca_info.sca_7000x_param.sca_ceffH[i];
		param->sca_7000x_param.sca_ceffV[i] = sca_info.sca_7000x_param.sca_ceffV[i];
		param->sca_8000x_param.sca_ceffH[i] = sca_info.sca_8000x_param.sca_ceffH[i];
		param->sca_8000x_param.sca_ceffV[i] = sca_info.sca_8000x_param.sca_ceffV[i];
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_set_sca_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_SCA_SET *param)
{
	HD_RESULT hd_ret = HD_OK;
	sca_info_set_t sca_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = -1, i;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}
	memset(&sca_info, 0x0, sizeof(sca_info_set_t));
	sca_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	sca_info.sca_y_luma_algo_en = param->sca_y_luma_algo_en;
	sca_info.sca_x_luma_algo_en = param->sca_x_luma_algo_en;
	sca_info.sca_y_chroma_algo_en = param->sca_y_chroma_algo_en;
	sca_info.sca_x_chroma_algo_en = param->sca_x_chroma_algo_en;
	sca_info.sca_map_sel = param->sca_map_sel;
	for (i = 0; i < 4; i++) {
		sca_info.sca_1000x_param.sca_ceffH[i] = param->sca_1000x_param.sca_ceffH[i];
		sca_info.sca_1000x_param.sca_ceffV[i] = param->sca_1000x_param.sca_ceffV[i];
		param->sca_1250x_param.sca_ceffH[i] = sca_info.sca_1250x_param.sca_ceffH[i];
		param->sca_1250x_param.sca_ceffV[i] = sca_info.sca_1250x_param.sca_ceffV[i];
		param->sca_1500x_param.sca_ceffH[i] = sca_info.sca_1500x_param.sca_ceffH[i];
		param->sca_1500x_param.sca_ceffV[i] = sca_info.sca_1500x_param.sca_ceffV[i];
		param->sca_1750x_param.sca_ceffH[i] = sca_info.sca_1750x_param.sca_ceffH[i];
		param->sca_1750x_param.sca_ceffV[i] = sca_info.sca_1750x_param.sca_ceffV[i];
		param->sca_2000x_param.sca_ceffH[i] = sca_info.sca_2000x_param.sca_ceffH[i];
		param->sca_2000x_param.sca_ceffV[i] = sca_info.sca_2000x_param.sca_ceffV[i];
		param->sca_2250x_param.sca_ceffH[i] = sca_info.sca_2250x_param.sca_ceffH[i];
		param->sca_2250x_param.sca_ceffV[i] = sca_info.sca_2250x_param.sca_ceffV[i];
		param->sca_2500x_param.sca_ceffH[i] = sca_info.sca_2500x_param.sca_ceffH[i];
		param->sca_2500x_param.sca_ceffV[i] = sca_info.sca_2500x_param.sca_ceffV[i];
		param->sca_2750x_param.sca_ceffH[i] = sca_info.sca_2750x_param.sca_ceffH[i];
		param->sca_2750x_param.sca_ceffV[i] = sca_info.sca_2750x_param.sca_ceffV[i];
		param->sca_3000x_param.sca_ceffH[i] = sca_info.sca_3000x_param.sca_ceffH[i];
		param->sca_3000x_param.sca_ceffV[i] = sca_info.sca_3000x_param.sca_ceffV[i];
		param->sca_3250x_param.sca_ceffH[i] = sca_info.sca_3250x_param.sca_ceffH[i];
		param->sca_3250x_param.sca_ceffV[i] = sca_info.sca_3250x_param.sca_ceffV[i];
		param->sca_3500x_param.sca_ceffH[i] = sca_info.sca_3500x_param.sca_ceffH[i];
		param->sca_3500x_param.sca_ceffV[i] = sca_info.sca_3500x_param.sca_ceffV[i];
		param->sca_3750x_param.sca_ceffH[i] = sca_info.sca_3750x_param.sca_ceffH[i];
		param->sca_3750x_param.sca_ceffV[i] = sca_info.sca_3750x_param.sca_ceffV[i];
		param->sca_4000x_param.sca_ceffH[i] = sca_info.sca_4000x_param.sca_ceffH[i];
		param->sca_4000x_param.sca_ceffV[i] = sca_info.sca_4000x_param.sca_ceffV[i];
		param->sca_5000x_param.sca_ceffH[i] = sca_info.sca_5000x_param.sca_ceffH[i];
		param->sca_5000x_param.sca_ceffV[i] = sca_info.sca_5000x_param.sca_ceffV[i];
		param->sca_6000x_param.sca_ceffH[i] = sca_info.sca_6000x_param.sca_ceffH[i];
		param->sca_6000x_param.sca_ceffV[i] = sca_info.sca_6000x_param.sca_ceffV[i];
		param->sca_7000x_param.sca_ceffH[i] = sca_info.sca_7000x_param.sca_ceffH[i];
		param->sca_7000x_param.sca_ceffV[i] = sca_info.sca_7000x_param.sca_ceffV[i];
		param->sca_8000x_param.sca_ceffH[i] = sca_info.sca_8000x_param.sca_ceffH[i];
		param->sca_8000x_param.sca_ceffV[i] = sca_info.sca_8000x_param.sca_ceffV[i];
	}

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_SCA_INFO, &sca_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_SCA_INFO\" return %d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_tmnr_motion_buf_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	fd_tmnr_buf_info_t buf_info;
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&buf_info, 0x0, sizeof(fd_tmnr_buf_info_t));
	buf_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_TMNR_MTN_BUF, &buf_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_TMNR_MTN_BUF\" return %d, fd(%#x)\n", hd_ret, buf_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	param->ddr_id = buf_info.ddr_id;
	param->p_start_pa = buf_info.pbuf_addr;
	param->size =  buf_info.pbuf_size;
err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_set_tmnr_motion_buf_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	fd_tmnr_buf_info_t buf_info;
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&buf_info, 0x0, sizeof(fd_tmnr_buf_info_t));
	buf_info.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	buf_info.ddr_id = param->ddr_id;
	buf_info.pbuf_addr = (UINTPTR)param->p_start_pa;
	
	//buf_info.pbuf_addr[1] = (UINTPTR)param->p_start_pa + (param->size / 2);
	buf_info.pbuf_size = param->size;
	if (buf_info.pbuf_size && pif_address_ddr_sanity_check(buf_info.pbuf_addr, buf_info.ddr_id) < 0) {
		VENDOR_ISP_ERROR("%s:Check pa(%#lx) ddrid(%d) error\n", __func__, buf_info.pbuf_addr, buf_info.ddr_id);
		ret = HD_ERR_PARAM;
		goto err_ext;
	}
	//buf_info.one_full_buf = FALSE;

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_TMNR_MTN_BUF, &buf_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_TMNR_MTN_BUF\" return %d, fd(%#x)\n", ret, buf_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_sca_yuv_range_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_YUV_RANGE *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	fd_sca_yuv_range_t range;
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&range, 0x0, sizeof(fd_sca_yuv_range_t));
	range.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_SCA_YUV_RANGE, &range);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_SCA_YUV_RANGE\" return %d, fd(%#x)\n", hd_ret, range.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	switch (range.yuv_range) {
	case VPE_RANGE_BYPASS:
		param->yuv_range = SCA_RANGE_BYPASS;
		break;
	case VPE_RANGE_PC2TV:
		param->yuv_range = SCA_RANGE_PC2TV;
		break;
	case VPE_RANGE_TV2PC:
		param->yuv_range = SCA_RANGE_TV2PC;
		break;
	case VPE_RANGE_DISABLE:
		param->yuv_range = SCA_RANGE_DISABLE;
		break;
	default:
		VENDOR_ISP_ERROR("Not support this yuv_ranage(%d), fd(%#x)\n", range.yuv_range, range.fd);
		break;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_set_sca_yuv_range_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_YUV_RANGE *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	fd_sca_yuv_range_t range;
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&range, 0x0, sizeof(fd_sca_yuv_range_t));
	range.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	switch (param->yuv_range) {
	case SCA_RANGE_BYPASS:
		range.yuv_range = VPE_RANGE_BYPASS;
		break;
	case SCA_RANGE_PC2TV:
		range.yuv_range = VPE_RANGE_PC2TV;
		break;
	case SCA_RANGE_TV2PC:
		range.yuv_range = VPE_RANGE_TV2PC;
		break;
	case SCA_RANGE_DISABLE:
		range.yuv_range = VPE_RANGE_DISABLE;
		break;
	default:
		VENDOR_ISP_ERROR("Not support this yuv_ranage(%d), fd(%#x)\n", param->yuv_range, range.fd);
		break;
	}

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_SCA_YUV_RANGE, &range);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_SCA_YUV_RANGE\" return %d, fd(%#x)\n", hd_ret, range.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_get_edge_smooth_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_EDGE_SMOOTH *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	fd_edgesmooth_param_t edge_smooth;
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&edge_smooth, 0x0, sizeof(fd_edgesmooth_param_t));
	edge_smooth.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_GET_EDGE_SMOOTH_INFO, &edge_smooth);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_EDGE_SMOOTH_INFO\" return %d, fd(%#x)\n", hd_ret, edge_smooth.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	param->edge_smooth_en = edge_smooth.param.edge_smooth_en;
	param->edge_smooth_y_edeng_th_lo = edge_smooth.param.edge_smooth_y_edeng_th_lo;
	param->edge_smooth_y_edeng_th_hi = edge_smooth.param.edge_smooth_y_edeng_th_hi;
	param->edge_smooth_y_ew_lo = edge_smooth.param.edge_smooth_y_ew_lo;
	param->edge_smooth_y_ew_hi = edge_smooth.param.edge_smooth_y_ew_hi;
	param->edge_smooth_y_edi_th = edge_smooth.param.edge_smooth_y_edi_th;
	param->edge_smooth_y_ds_str = edge_smooth.param.edge_smooth_y_ds_str;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_vpe_set_edge_smooth_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_EDGE_SMOOTH *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	fd_edgesmooth_param_t edge_smooth;
	INT ret = -1;

	if (isp_vpe_ioctl == 0) {
		isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		if (!isp_vpe_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEOPROC_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&edge_smooth, 0x0, sizeof(fd_edgesmooth_param_t));
	edge_smooth.fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	edge_smooth.param.edge_smooth_en = param->edge_smooth_en;
	edge_smooth.param.edge_smooth_y_edeng_th_lo = param->edge_smooth_y_edeng_th_lo;
	edge_smooth.param.edge_smooth_y_edeng_th_hi = param->edge_smooth_y_edeng_th_hi;
	edge_smooth.param.edge_smooth_y_ew_lo = param->edge_smooth_y_ew_lo;
	edge_smooth.param.edge_smooth_y_ew_hi = param->edge_smooth_y_ew_hi;
	edge_smooth.param.edge_smooth_y_edi_th = param->edge_smooth_y_edi_th;
	edge_smooth.param.edge_smooth_y_ds_str = param->edge_smooth_y_ds_str;

	ret = ioctl(isp_vpe_ioctl, VPE_IOC_SET_EDGE_SMOOTH_INFO, &edge_smooth);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_EDGE_SMOOTH_INFO\" return %d, fd(%#x)\n", hd_ret, edge_smooth.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_get_die_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DEI_INTERLACE *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_di_die_param_t die_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&die_info, 0x0, sizeof(fd_di_die_param_t));
	die_info.fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_die_ioctl, DI_IOC_GET_DIE_INFO, &die_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"DI_IOC_GET_DIE_INFO\" return %d, fd(%#x)\n", ret, die_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	param->top_motion_en = die_info.param.top_motion_en;
	param->bot_motion_en = die_info.param.bot_motion_en;
	param->auto_th_en = die_info.param.auto_th_en;
	param->strong_md_en = die_info.param.strong_md_en;
	param->mmb_en = die_info.param.mmb_en;
	param->smb_en = die_info.param.smb_en;
	param->emb_en = die_info.param.emb_en;
	param->lmb_en = die_info.param.lmb_en;
	param->pmmb_en = die_info.param.pmmb_en;
	param->corner_detect_en = die_info.param.corner_detect_en;
	param->di_gmm_motion_en = die_info.param.di_gmm_motion_en;
	param->mmb_scene_change_en = die_info.param.mmb_scene_change_en;
	param->mmb_scene_change_th = die_info.param.mmb_scene_change_th;
	param->all_motion = die_info.param.all_motion;
	param->all_static = die_info.param.all_static;
	param->strong_edge = die_info.param.strong_edge;
	param->strong_md_th = die_info.param.strong_md_th;
	param->md_th = die_info.param.md_th;
	param->line_admit = die_info.param.line_admit;
	param->mmb_th = die_info.param.mmb_th;
	param->smb_th = die_info.param.smb_th;
	param->emb_th = die_info.param.emb_th;
	param->lmb_th = die_info.param.lmb_th;
	param->ela_h_th = die_info.param.ela_h_th;
	param->ela_l_th = die_info.param.ela_l_th;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_set_die_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DEI_INTERLACE *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_di_die_param_t die_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&die_info, 0x0, sizeof(fd_di_die_param_t));
	die_info.fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	die_info.param.top_motion_en = param->top_motion_en;
	die_info.param.bot_motion_en = param->bot_motion_en;
	die_info.param.auto_th_en = param->auto_th_en;
	die_info.param.strong_md_en = param->strong_md_en;
	die_info.param.mmb_en = param->mmb_en;
	die_info.param.smb_en = param->smb_en;
	die_info.param.emb_en = param->emb_en;
	die_info.param.lmb_en = param->lmb_en;
	die_info.param.pmmb_en = param->pmmb_en;
	die_info.param.corner_detect_en = param->corner_detect_en;
	die_info.param.di_gmm_motion_en = param->di_gmm_motion_en;
	die_info.param.mmb_scene_change_en = param->mmb_scene_change_en;
	die_info.param.mmb_scene_change_th = param->mmb_scene_change_th;
	die_info.param.all_motion = param->all_motion;
	die_info.param.all_static = param->all_static;
	die_info.param.strong_edge = param->strong_edge;
	die_info.param.strong_md_th = param->strong_md_th;
	die_info.param.md_th = param->md_th;
	die_info.param.line_admit = param->line_admit;
	die_info.param.mmb_th = param->mmb_th;
	die_info.param.smb_th = param->smb_th;
	die_info.param.emb_th = param->emb_th;
	die_info.param.lmb_th = param->lmb_th;
	die_info.param.ela_h_th = param->ela_h_th;
	die_info.param.ela_l_th = param->ela_l_th;

	ret = ioctl(isp_die_ioctl, DI_IOC_SET_DIE_INFO, &die_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"DI_IOC_SET_DIE_INFO\" return %d, fd(%#x)\n", ret, die_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_get_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_di_en_info_t en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(fd_di_en_info_t));
	en_info.fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_die_ioctl, DI_IOC_GET_TMNR_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_GET_TMNR_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	*param = en_info.enable;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_set_tmnr_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_di_en_info_t en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(fd_di_en_info_t));
	en_info.fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	en_info.enable = *param;

	ret = ioctl(isp_die_ioctl, DI_IOC_SET_TMNR_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"VPE_IOC_SET_TMNR_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_get_tmnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DI_TMNR *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_di_tmnr_param_t tmnr_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&tmnr_info, 0x0, sizeof(fd_di_tmnr_param_t));
	tmnr_info.fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_die_ioctl, DI_IOC_GET_TMNR_INFO, &tmnr_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"DI_IOC_GET_TMNR_INFO\" return %d, fd(%#x)\n", ret, tmnr_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	param->tmnr_learn_en = tmnr_info.param.tmnr_learn_en;
	param->y_var = tmnr_info.param.y_var;
	param->cb_var = tmnr_info.param.cb_var;
	param->cr_var = tmnr_info.param.cr_var;
	param->k = tmnr_info.param.k;
	param->auto_k = tmnr_info.param.auto_k;
	param->auto_k_hi = tmnr_info.param.auto_k_hi;
	param->auto_k_lo = tmnr_info.param.auto_k_lo;
	param->trade_thres = tmnr_info.param.trade_thres;
	param->suppress_strength = tmnr_info.param.suppress_strength;
	param->nf = tmnr_info.param.nf;
	param->var_offset = tmnr_info.param.var_offset;
	param->motion_var = tmnr_info.param.motion_var;
	param->motion_th_mult = tmnr_info.param.motion_th_mult;
	param->tmnr_fcs_th = tmnr_info.param.tmnr_fcs_th;
	param->tmnr_fcs_weight = tmnr_info.param.tmnr_fcs_weight;
	param->tmnr_fcs_en = tmnr_info.param.tmnr_fcs_en;
	param->dpr_motion_th = tmnr_info.param.dpr_motion_th;
	param->dpr_cnt_th = tmnr_info.param.dpr_cnt_th;
	param->dpr_mode = tmnr_info.param.dpr_mode;
	param->dpr_en = tmnr_info.param.dpr_en;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_set_tmnr_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DI_TMNR *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_di_tmnr_param_t tmnr_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&tmnr_info, 0x0, sizeof(fd_di_tmnr_param_t));
	tmnr_info.fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	tmnr_info.param.tmnr_learn_en = param->tmnr_learn_en;
	tmnr_info.param.y_var = param->y_var;
	tmnr_info.param.cb_var = param->cb_var;
	tmnr_info.param.cr_var = param->cr_var;
	tmnr_info.param.k = param->k;
	tmnr_info.param.auto_k = param->auto_k;
	tmnr_info.param.auto_k_hi = param->auto_k_hi;
	tmnr_info.param.auto_k_lo = param->auto_k_lo;
	tmnr_info.param.trade_thres = param->trade_thres;
	tmnr_info.param.suppress_strength = param->suppress_strength;
	tmnr_info.param.nf = param->nf;
	tmnr_info.param.var_offset = param->var_offset;
	tmnr_info.param.motion_var = param->motion_var;
	tmnr_info.param.motion_th_mult = param->motion_th_mult;
	tmnr_info.param.tmnr_fcs_th = param->tmnr_fcs_th;
	tmnr_info.param.tmnr_fcs_weight = param->tmnr_fcs_weight;
	tmnr_info.param.tmnr_fcs_en = param->tmnr_fcs_en;
	tmnr_info.param.dpr_motion_th = param->dpr_motion_th;
	tmnr_info.param.dpr_cnt_th = param->dpr_cnt_th;
	tmnr_info.param.dpr_mode = param->dpr_mode;
	tmnr_info.param.dpr_en = param->dpr_en;

	ret = ioctl(isp_die_ioctl, DI_IOC_SET_TMNR_INFO, &tmnr_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"DI_IOC_SET_TMNR_INFO\" return %d, fd(%#x)\n", ret, tmnr_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_get_gmm_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_di_en_info_t en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(fd_di_en_info_t));
	en_info.fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_die_ioctl, DI_IOC_GET_GMM_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"DI_IOC_GET_GMM_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	*param = en_info.enable;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_set_gmm_en_p(HD_PATH_ID path_id, UINT32 *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_di_en_info_t en_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&en_info, 0x0, sizeof(fd_di_en_info_t));
	en_info.fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	en_info.enable = *param;

	ret = ioctl(isp_die_ioctl, DI_IOC_SET_GMM_ENABLE, &en_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"DI_IOC_SET_GMM_ENABLE\" return %d, fd(%#x)\n", ret, en_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_get_gmm_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DI_GMM *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_di_gmm_param_t gmm_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&gmm_info, 0x0, sizeof(fd_di_gmm_param_t));
	gmm_info.fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	ret = ioctl(isp_die_ioctl, DI_IOC_GET_GMM_INFO, &gmm_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"DI_IOC_GET_GMM_INFO\" return %d, fd(%#x)\n", ret, gmm_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	param->gmm_alpha = gmm_info.param.gmm_alpha;
	param->gmm_one_min_alpha = gmm_info.param.gmm_one_min_alpha;
	param->gmm_init_val = gmm_info.param.gmm_init_val;
	param->gmm_tb = gmm_info.param.gmm_tb;
	param->gmm_sigma = gmm_info.param.gmm_sigma;
	param->gmm_tg = gmm_info.param.gmm_tg;
	param->gmm_prune = gmm_info.param.gmm_prune;

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_set_gmm_info_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_DI_GMM *param)
{
	HD_RESULT hd_ret = HD_OK;
	fd_di_gmm_param_t gmm_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}

	memset(&gmm_info, 0x0, sizeof(fd_di_gmm_param_t));
	gmm_info.fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);
	gmm_info.param.gmm_alpha = param->gmm_alpha;
	gmm_info.param.gmm_one_min_alpha = param->gmm_one_min_alpha;
	gmm_info.param.gmm_init_val = param->gmm_init_val;
	gmm_info.param.gmm_tb = param->gmm_tb;
	gmm_info.param.gmm_sigma = param->gmm_sigma;
	gmm_info.param.gmm_tg = param->gmm_tg;
	gmm_info.param.gmm_prune = param->gmm_prune;

	ret = ioctl(isp_die_ioctl, DI_IOC_SET_GMM_INFO, &gmm_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"DI_IOC_SET_GMM_INFO\" return %d, fd(%#x)\n", ret, gmm_info.fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_die_set_init_p(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
#if 0 //should not be init di engine
	di_die_init_info_t init_info;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;
	UINT fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	memset(&init_info, 0x0, sizeof(di_die_init_info_t));
	init_info.st_size = sizeof(di_die_init_info_t);
	init_info.chip_num = ENTITY_FD_CHIP(fd);
	init_info.eng_num = ENTITY_FD_ENGINE(fd);
	init_info.minor_num = ENTITY_FD_MINOR(fd);
	
	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}
	ret = ioctl(isp_die_ioctl, DI_IOC_SET_INIT, &init_info);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"DI_IOC_SET_INIT\" return %d, fd(%#x)\n", ret, fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
#endif
	return hd_ret;
}

HD_RESULT vendor_die_set_uninit_p(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
#if 0 //should not be close di engine
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0;
	UINT fd = utl_get_di_entity_fd(VDOPROC_DEVID(dev_id), VDOPROC_OUTID(out_id), NULL);

	if (isp_die_ioctl == 0) {
		isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		if (!isp_die_ioctl) {
			VENDOR_ISP_ERROR("open %s fail\n", VIDEODIE_DEVICE);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}
	ret = ioctl(isp_die_ioctl, DI_IOC_SET_UNINIT, NULL);
	if (ret < 0) {
		VENDOR_ISP_ERROR("ioctl \"DI_IOC_SET_UNINIT\" return %d, fd(%#x)\n", ret, fd);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

err_ext:
#endif
	return hd_ret;
}

HD_RESULT vendor_lcd_get_iq_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_IQ *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	INT device = VDOOUT_DEVID(dev_id);
	lcd300_drv_iq_t lcd0_iq;
	lcd300_drv_iq_t lcd1ite_iq;
	lcd200_drv_iq_t lcd1_iq;
	INT ret = 0, i;

	if (device == 0) {
		if (isp_lcd0_ioctl == 0) {
			isp_lcd0_ioctl = open(VIDEOOUT_DEVICE0, O_RDWR);
			if (!isp_lcd0_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE0);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		memset(&lcd0_iq, 0x0, sizeof(lcd300_drv_iq_t));
		ret = ioctl(isp_lcd0_ioctl, LCD300_IOC_GET_IQ, &lcd0_iq);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl \"LCD300_IOC_GET_IQ\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
		param->brightness = lcd0_iq.brightness;
		param->ce_en = lcd0_iq.ce.state;
		param->ce_strength = lcd0_iq.ce.strength;
		param->sharp_en = lcd0_iq.sharpness.state;
		param->sharp_hpf0_5x5_gain = lcd0_iq.sharpness.hpf0_5x5_gain;
		param->sharp_hpf1_5x5_gain = lcd0_iq.sharpness.hpf1_5x5_gain;
		param->contrast = lcd0_iq.contrast.contrast;
		param->contrast_mode = lcd0_iq.contrast.contrast_mode;
		param->hue_sat_en = lcd0_iq.hue_sat.hue_sat_enable;
		for (i = 0; i < 6; i++) {
			param->hue[i] = lcd0_iq.hue_sat.hue[i];
			param->saturation[i] = lcd0_iq.hue_sat.saturation[i];
		}
	} else if (device == 1) {
		if (isp_lcd1_ioctl == 0) {
			isp_lcd1_ioctl = open(VIDEOOUT_DEVICE1, O_RDWR);
			if (!isp_lcd1_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE1);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		memset(&lcd1ite_iq, 0x0, sizeof(lcd300_drv_iq_t));
		ret = ioctl(isp_lcd1_ioctl, LCD300_IOC_GET_IQ, &lcd1ite_iq);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl lite \"LCD300_IOC_GET_IQ\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
		param->brightness = lcd1ite_iq.brightness;
		param->ce_en = lcd1ite_iq.ce.state;
		param->ce_strength = lcd1ite_iq.ce.strength;
		param->sharp_en = lcd1ite_iq.sharpness.state;
		param->sharp_hpf0_5x5_gain = lcd1ite_iq.sharpness.hpf0_5x5_gain;
		param->sharp_hpf1_5x5_gain = lcd1ite_iq.sharpness.hpf1_5x5_gain;
		param->contrast = lcd1ite_iq.contrast.contrast;
		param->contrast_mode = lcd1ite_iq.contrast.contrast_mode;
		param->hue_sat_en = lcd1ite_iq.hue_sat.hue_sat_enable;
		for (i = 0; i < 6; i++) {
			param->hue[i] = lcd1ite_iq.hue_sat.hue[i];
			param->saturation[i] = lcd1ite_iq.hue_sat.saturation[i];
		}
	} else if (device == 2) {
		if (isp_lcd2_ioctl == 0) {
			isp_lcd2_ioctl = open(VIDEOOUT_DEVICE2, O_RDWR);
			if (!isp_lcd2_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE2);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		memset(&lcd1_iq, 0x0, sizeof(lcd200_drv_iq_t));
		ret = ioctl(isp_lcd2_ioctl, LCD200_IOC_GET_IQ, &lcd1_iq);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl \"LCD200_IOC_GET_IQ\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
		param->brightness = lcd1_iq.brightness;
		param->sharp_k0 = lcd1_iq.sharpness.k0;
		param->sharp_k1 = lcd1_iq.sharpness.k1;
		param->sharp_threshold0 = lcd1_iq.sharpness.threshold0;
		param->sharp_threshold1 = lcd1_iq.sharpness.threshold1;
		param->contrast_val = lcd1_iq.contrast.contrast;
		param->hue_cos_val = lcd1_iq.hue.cos_val;
		param->hue_sin_val = lcd1_iq.hue.sin_val;
	} else {
		VENDOR_ISP_ERROR("Not support dev id = %#x for LCD_IQ\n", device);
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_lcd_set_iq_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_IQ *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	INT device = VDOOUT_DEVID(dev_id);
	lcd300_drv_iq_t lcd0_iq;
	lcd300_drv_iq_t lcdlite_iq;
	lcd200_drv_iq_t lcd1_iq;
	INT ret = 0, i;

	if (device == 0) {
		if (isp_lcd0_ioctl == 0) {
			isp_lcd0_ioctl = open(VIDEOOUT_DEVICE0, O_RDWR);
			if (!isp_lcd0_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE0);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		memset(&lcd0_iq, 0x0, sizeof(lcd300_drv_iq_t));
		lcd0_iq.brightness =param->brightness;
		lcd0_iq.ce.state =param->ce_en;
		lcd0_iq.ce.strength =param->ce_strength;
		lcd0_iq.sharpness.state =param->sharp_en;
		lcd0_iq.sharpness.hpf0_5x5_gain =param->sharp_hpf0_5x5_gain;
		lcd0_iq.sharpness.hpf1_5x5_gain =param->sharp_hpf1_5x5_gain;
		lcd0_iq.contrast.contrast =param->contrast;
		lcd0_iq.contrast.contrast_mode =param->contrast_mode;
		lcd0_iq.hue_sat.hue_sat_enable = param->hue_sat_en;
		for (i = 0; i < 6; i++) {
			lcd0_iq.hue_sat.hue[i] = param->hue[i];
			lcd0_iq.hue_sat.saturation[i] = param->saturation[i];
		}

		ret = ioctl(isp_lcd0_ioctl, LCD300_IOC_SET_IQ, &lcd0_iq);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl \"LCD300_IOC_SET_IQ\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	} else if (device == 1) {
		if (isp_lcd1_ioctl == 0) {
			isp_lcd1_ioctl = open(VIDEOOUT_DEVICE1, O_RDWR);
			if (!isp_lcd1_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE1);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		memset(&lcdlite_iq, 0x0, sizeof(lcd300_drv_iq_t));
		lcdlite_iq.brightness =param->brightness;
		lcdlite_iq.ce.state =param->ce_en;
		lcdlite_iq.ce.strength =param->ce_strength;
		lcdlite_iq.sharpness.state =param->sharp_en;
		lcdlite_iq.sharpness.hpf0_5x5_gain =param->sharp_hpf0_5x5_gain;
		lcdlite_iq.sharpness.hpf1_5x5_gain =param->sharp_hpf1_5x5_gain;
		lcdlite_iq.contrast.contrast =param->contrast;
		lcdlite_iq.contrast.contrast_mode =param->contrast_mode;
		lcdlite_iq.hue_sat.hue_sat_enable = param->hue_sat_en;
		for (i = 0; i < 6; i++) {
			lcdlite_iq.hue_sat.hue[i] = param->hue[i];
			lcdlite_iq.hue_sat.saturation[i] = param->saturation[i];
		}

		ret = ioctl(isp_lcd1_ioctl, LCD300_IOC_SET_IQ, &lcdlite_iq);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl lite \"LCD300lite_IOC_SET_IQ\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	} else if (device == 2) {
		if (isp_lcd2_ioctl == 0) {
			isp_lcd2_ioctl = open(VIDEOOUT_DEVICE2, O_RDWR);
			if (!isp_lcd2_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE2);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		memset(&lcd1_iq, 0x0, sizeof(lcd200_drv_iq_t));
		lcd1_iq.brightness =param->brightness;
		lcd1_iq.sharpness.k0 =param->sharp_k0;
		lcd1_iq.sharpness.k1 =param->sharp_k1;
		lcd1_iq.sharpness.threshold0 =param->sharp_threshold0;
		lcd1_iq.sharpness.threshold1 =param->sharp_threshold1;
		lcd1_iq.contrast.contrast =param->contrast_val;
		lcd1_iq.hue.cos_val =param->hue_cos_val;
		lcd1_iq.hue.sin_val =param->hue_sin_val;

		ret = ioctl(isp_lcd2_ioctl, LCD200_IOC_SET_IQ, &lcd1_iq);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl \"LCD200_IOC_SET_IQ\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	} else {
		VENDOR_ISP_ERROR("Not support dev id = %#x for LCD_IQ\n", device);
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_lcd_get_hue_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_HUE_SAT *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	INT device = VDOOUT_DEVID(dev_id);
	struct lcd300_hue_sat lcd0_hue;
	INT ret = 0;

	if (device == 0) {
		if (isp_lcd0_ioctl == 0) {
			isp_lcd0_ioctl = open(VIDEOOUT_DEVICE0, O_RDWR);
			if (!isp_lcd0_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE0);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		memset(&lcd0_hue, 0x0, sizeof(struct lcd300_hue_sat));
		ret = ioctl(isp_lcd0_ioctl, LCD300_IOC_GET_HUESAT, &lcd0_hue);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl \"LCD300_IOC_GET_HUESAT\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	} else if (device == 1) {
		if (isp_lcd1_ioctl == 0) {
			isp_lcd1_ioctl = open(VIDEOOUT_DEVICE1, O_RDWR);
			if (!isp_lcd1_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE1);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		memset(&lcd0_hue, 0x0, sizeof(struct lcd300_hue_sat));
		ret = ioctl(isp_lcd1_ioctl, LCD300_IOC_GET_HUESAT, &lcd0_hue);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl lite \"LCD300_IOC_GET_HUESAT\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	} else {
	    hd_ret = HD_ERR_NOT_SUPPORT;
        VENDOR_ISP_ERROR("Only support lcd0 or 1, get hue\n");
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_lcd_set_hue_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_HUE_SAT *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	INT device = VDOOUT_DEVID(dev_id);
	struct lcd300_hue_sat lcd0_hue;
	INT ret = 0, i;
    
	if (device == 0) {
		if (isp_lcd0_ioctl == 0) {
			isp_lcd0_ioctl = open(VIDEOOUT_DEVICE0, O_RDWR);
			if (!isp_lcd0_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE0);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
        lcd0_hue.hue_sat_enable = param->hue_sat_enable;
        for(i = 0; i < 6; i++) {
            if(param->hue_sat[i] > 255) {
                VENDOR_ISP_ERROR("Check hue value error\n");
                hd_ret = HD_ERR_NOT_SUPPORT;
    			goto err_ext;
            }
            lcd0_hue.hue_sat[i] = param->hue_sat[i];
        }
		ret = ioctl(isp_lcd0_ioctl, LCD300_IOC_SET_HUESAT, &lcd0_hue);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl \"LCD300_IOC_SET_HUESAT\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	} else if (device == 1) {
			if (isp_lcd1_ioctl == 0) {
				isp_lcd1_ioctl = open(VIDEOOUT_DEVICE1, O_RDWR);
				if (!isp_lcd1_ioctl) {
					VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE1);
					hd_ret = HD_ERR_SYS;
					goto err_ext;
				}
			}
	        lcd0_hue.hue_sat_enable = param->hue_sat_enable;
	        for(i = 0; i < 6; i++) {
	            if(param->hue_sat[i] > 255) {
	                VENDOR_ISP_ERROR("Check hue value error\n");
	                hd_ret = HD_ERR_NOT_SUPPORT;
	    			goto err_ext;
	            }
	            lcd0_hue.hue_sat[i] = param->hue_sat[i];
	        }
			ret = ioctl(isp_lcd1_ioctl, LCD300_IOC_SET_HUESAT, &lcd0_hue);
			if (ret < 0) {
				VENDOR_ISP_ERROR("ioctl lite \"LCD300_IOC_SET_HUESAT\" return %d\n", ret);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
	} else {
	    hd_ret = HD_ERR_NOT_SUPPORT;
        VENDOR_ISP_ERROR("Only support lcd0 or 1, get hue\n");
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_lcd_get_ygamma_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_YGAMMA *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	INT device = VDOOUT_DEVID(dev_id);
	struct lcd300_ygamma lcd0_ygamma;
	INT ret = 0;

	if (device == 0) {
		if (isp_lcd0_ioctl == 0) {
			isp_lcd0_ioctl = open(VIDEOOUT_DEVICE0, O_RDWR);
			if (!isp_lcd0_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE0);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		memset(&lcd0_ygamma, 0x0, sizeof(struct lcd300_ygamma));
		ret = ioctl(isp_lcd0_ioctl, LCD300_IOC_GET_YGAMMA, &lcd0_ygamma);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl \"LCD300_IOC_GET_YGAMMA\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	} else if (device == 1) {
		if (isp_lcd1_ioctl == 0) {
			isp_lcd1_ioctl = open(VIDEOOUT_DEVICE1, O_RDWR);
			if (!isp_lcd1_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE1);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		memset(&lcd0_ygamma, 0x0, sizeof(struct lcd300_ygamma));
		ret = ioctl(isp_lcd1_ioctl, LCD300_IOC_GET_YGAMMA, &lcd0_ygamma);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl lite \"LCD300_IOC_GET_YGAMMA\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	} else {
	    hd_ret = HD_ERR_NOT_SUPPORT;
        VENDOR_ISP_ERROR("Only support lcd0 or 1, get ygamma\n");
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_lcd_set_ygamma_p(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_LCD_YGAMMA *param)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	INT device = VDOOUT_DEVID(dev_id);
	struct lcd300_ygamma lcd0_ygamma;
	INT ret = 0, i;

	if (device == 0) {
		if (isp_lcd0_ioctl == 0) {
			isp_lcd0_ioctl = open(VIDEOOUT_DEVICE0, O_RDWR);
			if (!isp_lcd0_ioctl) {
				VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE0);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
		}
		lcd0_ygamma.gamma_state = param->gamma_state;
        
        for(i = 0; i < 16; i++) {
            lcd0_ygamma.index[i] = param->index[i];
        }

        for(i = 0; i < 32; i++) {
            if(param->y_gamma[i] > 4095) {
                VENDOR_ISP_ERROR("Check ygamma value error\n");
                hd_ret = HD_ERR_NOT_SUPPORT;
    			goto err_ext;
            }
            lcd0_ygamma.y_gm[i] = param->y_gamma[i];
        }
        ret = ioctl(isp_lcd0_ioctl, LCD300_IOC_SET_YGAMMA, &lcd0_ygamma);
		if (ret < 0) {
			VENDOR_ISP_ERROR("ioctl \"LCD300_IOC_SET_YGAMMA\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	} else if (device == 1) {
			if (isp_lcd1_ioctl == 0) {
				isp_lcd1_ioctl = open(VIDEOOUT_DEVICE1, O_RDWR);
				if (!isp_lcd1_ioctl) {
					VENDOR_ISP_ERROR("open %s fail\n", VIDEOOUT_DEVICE1);
					hd_ret = HD_ERR_SYS;
					goto err_ext;
				}
			}
			lcd0_ygamma.gamma_state = param->gamma_state;

	        for(i = 0; i < 16; i++) {
	            lcd0_ygamma.index[i] = param->index[i];
	        }

	        for(i = 0; i < 32; i++) {
	            if(param->y_gamma[i] > 4095) {
	                VENDOR_ISP_ERROR("Check ygamma value error\n");
	                hd_ret = HD_ERR_NOT_SUPPORT;
	    			goto err_ext;
	            }
	            lcd0_ygamma.y_gm[i] = param->y_gamma[i];
	        }
	        ret = ioctl(isp_lcd1_ioctl, LCD300_IOC_SET_YGAMMA, &lcd0_ygamma);
			if (ret < 0) {
				VENDOR_ISP_ERROR("ioctl lite\"LCD300_IOC_SET_YGAMMA\" return %d\n", ret);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
	} else {
	    hd_ret = HD_ERR_NOT_SUPPORT;
        VENDOR_ISP_ERROR("Only support lcd0 or 1, set ygamma\n");
	}

err_ext:
	return hd_ret;
}

HD_RESULT vendor_isp_ioctl_open_p(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);

	if ((dev_id & 0xff00) == HD_DAL_VIDEOENC_BASE) {
		if (isp_enc_ioctl == 0) {
			isp_enc_ioctl = open(VIDEOENC_DEVICE, O_RDWR);
		}
		if (isp_enc_ioctl < 0) {
			VENDOR_ISP_ERROR("open %s failed.\n", VIDEOENC_DEVICE);
		}
	} else if ((dev_id & 0xff00) >= HD_DAL_VIDEOPROC_BASE && (dev_id & 0xff00) <= HD_DAL_VIDEOPROC_RESERVED_BASE) {
		if (isp_vpe_ioctl == 0) {
			isp_vpe_ioctl = open(VIDEOPROC_DEVICE, O_RDWR);
		}
		if (isp_vpe_ioctl < 0) {
			VENDOR_ISP_ERROR("open %s failed.\n", VIDEOPROC_DEVICE);
		}
		if (isp_die_ioctl == 0) {
			isp_die_ioctl = open(VIDEODIE_DEVICE, O_RDWR);
		}
		if (isp_die_ioctl < 0) {
			VENDOR_ISP_ERROR("open %s failed.\n", VIDEODIE_DEVICE);
		}
	} else if ((dev_id & 0xff00) == HD_DAL_VIDEOOUT_BASE) {
		if (isp_lcd0_ioctl == 0) {
			isp_lcd0_ioctl = open(VIDEOOUT_DEVICE0, O_RDWR);
		}
		if (isp_lcd0_ioctl < 0) {
			VENDOR_ISP_ERROR("open %s failed.\n", VIDEOOUT_DEVICE0);
		}
		if (isp_lcd1_ioctl == 0) {
			isp_lcd1_ioctl = open(VIDEOOUT_DEVICE1, O_RDWR);
		}
		if (isp_lcd1_ioctl < 0) {
			VENDOR_ISP_ERROR("open %s failed.\n", VIDEOOUT_DEVICE1);
		}
		if (isp_lcd2_ioctl == 0) {
			isp_lcd2_ioctl = open(VIDEOOUT_DEVICE2, O_RDWR);
		}
		if (isp_lcd2_ioctl < 0) {
			VENDOR_ISP_ERROR("open %s failed.\n", VIDEOOUT_DEVICE2);
		}
	}
	return hd_ret;
}

HD_RESULT vendor_isp_ioctl_close_p(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL dev_id = HD_GET_DEV(path_id);

	if ((dev_id & 0xff00) == HD_DAL_VIDEOCAP_BASE) {
		if (isp_enc_ioctl) {
			close(isp_enc_ioctl);
			isp_enc_ioctl = 0;
		}
	} else if ((dev_id & 0xff00) >= HD_DAL_VIDEOPROC_BASE && (dev_id & 0xff00) <= HD_DAL_VIDEOPROC_RESERVED_BASE) {
		if (isp_vpe_ioctl) {
			close(isp_vpe_ioctl);
			isp_vpe_ioctl = 0;
		}
		if (isp_die_ioctl) {
			close(isp_die_ioctl);
			isp_die_ioctl = 0;
		}
	} else if ((dev_id & 0xff00) == HD_DAL_VIDEOOUT_BASE) {
		if (isp_lcd0_ioctl) {
			close(isp_lcd0_ioctl);
			isp_lcd0_ioctl = 0;
		}
		if (isp_lcd1_ioctl) {
			close(isp_lcd1_ioctl);
			isp_lcd1_ioctl = 0;
		}
		if (isp_lcd2_ioctl) {
			close(isp_lcd2_ioctl);
			isp_lcd2_ioctl = 0;
		}
	}
	return hd_ret;
}

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
