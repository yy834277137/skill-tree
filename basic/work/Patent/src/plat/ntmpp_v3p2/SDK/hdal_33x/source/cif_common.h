/*
 *   @file   cif_common.h
 *
 *   @brief  The customer interface.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#ifndef __CIF_COMMON_H__      /* prevent multiple inclusion of the header file */
#define __CIF_COMMON_H__

#include "hd_type.h"
#include "hdal.h"
#include "vpd_ioctl.h"
#include "trig_ioctl.h"

#define CIF_VIDEO_PXLFMT_YUV420_MB3      0x510cd420//8x4
#define CIF_HD_VIDEOOUT_HDMI_3840X2160P60      101 //HD_VIDEOOUT_HDMI_3840X2160P60

typedef dim_t align_level_t;
void common_get_cap_align_level(vpd_buf_type_t buf_type, align_level_t *level);
void common_get_vpe_align_level(vpd_buf_type_t buf_type, align_level_t *level);
void common_get_enc_align_level(vpd_buf_type_t buf_type, align_level_t *level);
int common_calculate_frame_buf_size(dim_t *dim, vpd_buf_type_t type);
unsigned int common_calculate_buf_size_by_HD_VIDEO_PXLFMT(HD_DIM dim, HD_VIDEO_PXLFMT pxlfmt);
int common_calculate_md_info_size(HD_DIM *dim);
int common_calculate_dec_mb_info_size(HD_DIM *dim, HD_VIDEO_CODEC codec);
int common_calculate_dec_side_info_size(HD_DIM *dim, HD_VIDEO_CODEC codec);
int common_calculate_dec_side_info_offset(HD_DIM *dim, HD_VIDEO_CODEC codec);
vpd_buf_type_t common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(HD_VIDEO_PXLFMT pxlfmt);
HD_VIDEO_PXLFMT common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(vpd_buf_type_t pxlfmt);
vpbuf_pool_type_t HD_COMMON_MEM_POOL_TYPE_to_vpd_type(HD_COMMON_MEM_POOL_TYPE type);
vpd_dec_type_t common_convert_hd_codec_type_to_vpd_buffer_type(int type);
char* common_HD_VIDEO_PXLFMT_str(HD_VIDEO_PXLFMT pxlfmt);
unsigned int common_convert_HD_VIDEO_PXLFMT_to_buf_type_value(HD_VIDEO_PXLFMT pxlfmt);

#endif //#ifndef __CIF_COMMON_H__

