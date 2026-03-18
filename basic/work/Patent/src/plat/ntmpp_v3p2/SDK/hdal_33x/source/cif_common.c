/*
 *   @file   cif_common.c
 *
 *   @brief  Customer interface common function.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include "cif_list.h"
#include "hd_type.h"
#include "vpd_ioctl.h"
#include "pif.h"
#include "cif_common.h"
#include "vg_common.h"

extern unsigned int gmlib_dbg_level;

static void common_add_list(list_head_t *new, list_head_t *prev, list_head_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static void common_del_list(list_head_t *prev, list_head_t *next)
{
	next->prev = prev;
	prev->next = next;
}

void common_add_listHeadEntry(list_head_t *newEntry, list_head_t *head)
{
	common_add_list(newEntry, head, head->next);
}

void common_add_listTailEntry(list_head_t *newEntry, list_head_t *head)
{
	common_add_list(newEntry, head->prev, head);
}

void common_del_listEntry(list_head_t *entry)
{
	common_del_list(entry->prev, entry->next);
}


////////////////////////////////////////////////////////////////////////////////
// align level
////////////////////////////////////////////////////////////////////////////////

/* ## General information about alignment ##
 *       UYVY:  x(4x)  y(2x)   w(4x)   h(2x)
 *      SM420:  x(8x)  y(2x)   w(8x)   h(2x)
 *      MB422:  x(16x) y(16x)  w(16x)  h(16x)
 *      MB420:  x(16x) y(16x)  w(16x)  h(16x)
 *
 *  DPCM+UYVY:  x(16x) y(2x)   w(16x)  h(2x)
 * DPCM+SM420:  x(16x) y(2x)   w(16x)  h(2x)
 * DPCM+MB422:  x(16x) y(16x)  w(16x)  h(16x)
 * DPCM+MB420:  x(16x) y(16x)  w(16x)  h(16x)
 */
void common_get_cap_align_level(vpd_buf_type_t buf_type, align_level_t *level)
{
	int level_w, level_h;

	switch (buf_type) {
	case VPD_BUFTYPE_YUV422:
		level_w = 4;
		level_h = 2;
		break;
	case VPD_BUFTYPE_YUV422_SCE:
		level_w = 16;
		level_h = 2;
		break;
	case VPD_BUFTYPE_YUV422_MB:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_YUV420_SP:
		level_w = 8;
		level_h = 2;
		break;
	case VPD_BUFTYPE_YUV420_SCE:
		level_w = FORMAT_ALIGN_VAL_YUV420_SCE_W;
		level_h = FORMAT_ALIGN_VAL_YUV420_SCE_H;
		break;
	case VPD_BUFTYPE_YUV420_MB:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_YUV420_16x2:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_YUV420_SP8:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_ARGB1555:
	case VPD_BUFTYPE_ARGB8888:
	case VPD_BUFTYPE_RGB565:
	case VPD_BUFTYPE_RGB_CV:
	case VPD_BUFTYPE_DATA:
	default:
		level_w = 1;
		level_h = 1;
		break;
	}

	level->w = level_w;
	level->h = level_h;
}

/* ## General information about alignment ##
 *       UYVY:  x(4x)  y(2x)   w(4x)   h(2x)
 *      SM420:  x(8x)  y(2x)   w(8x)   h(2x)
 *      MB422:  x(16x) y(16x)  w(16x)  h(16x)
 *      MB420:  x(16x) y(16x)  w(16x)  h(16x)
 *
 *  DPCM+UYVY:  x(16x) y(2x)   w(16x)  h(2x)
 * DPCM+SM420:  x(16x) y(2x)   w(16x)  h(2x)
 * DPCM+MB422:  x(16x) y(16x)  w(16x)  h(16x)
 * DPCM+MB420:  x(16x) y(16x)  w(16x)  h(16x)
 */
void common_get_vpe_align_level(vpd_buf_type_t buf_type, align_level_t *level)
{
	int level_w, level_h;

	switch (buf_type) {
	case VPD_BUFTYPE_YUV422:
		level_w = 4;
		level_h = 2;
		break;
	case VPD_BUFTYPE_YUV422_SCE:
		level_w = 16;
		level_h = 2;
		break;
	case VPD_BUFTYPE_YUV422_MB:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_YUV420_SP:
		level_w = 8;
		level_h = 2;
		break;
	case VPD_BUFTYPE_YUV420_SCE:
		level_w = FORMAT_ALIGN_VAL_YUV420_SCE_W;
		level_h = FORMAT_ALIGN_VAL_YUV420_SCE_H;
		break;
	case VPD_BUFTYPE_YUV420_MB:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_YUV420_16x2:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_YUV420_SP8:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_ARGB1555:
	case VPD_BUFTYPE_ARGB8888:
	case VPD_BUFTYPE_RGB565:
	case VPD_BUFTYPE_RGB_CV:
	case VPD_BUFTYPE_DATA:
	default:
		level_w = 1;
		level_h = 1;
		break;
	}

	level->w = level_w;
	level->h = level_h;
}

/* ## General information about alignment ##
 *       UYVY:  x(4x)  y(2x)   w(4x)   h(2x)
 *      SM420:  x(8x)  y(2x)   w(8x)   h(2x)
 *      MB422:  x(16x) y(16x)  w(16x)  h(16x)
 *      MB420:  x(16x) y(16x)  w(16x)  h(16x)
 *
 *  DPCM+UYVY:  x(16x) y(2x)   w(16x)  h(2x)
 * DPCM+SM420:  x(16x) y(2x)   w(16x)  h(2x)
 * DPCM+MB422:  x(16x) y(16x)  w(16x)  h(16x)
 * DPCM+MB420:  x(16x) y(16x)  w(16x)  h(16x)
 */
void common_get_enc_align_level(vpd_buf_type_t buf_type, align_level_t *level)
{
	int level_w, level_h;

	switch (buf_type) {
	case VPD_BUFTYPE_YUV422:
		level_w = 4;
		level_h = 2;
		break;
	case VPD_BUFTYPE_YUV422_SCE:
		level_w = 16;
		level_h = 2;
		break;
	case VPD_BUFTYPE_YUV422_MB:
		level_w = 16;
		level_h = 16;
		break;

	case VPD_BUFTYPE_YUV420_SP:
		level_w = 8;
		level_h = 2;
		break;
	case VPD_BUFTYPE_YUV420_SCE:
		level_w = FORMAT_ALIGN_VAL_YUV420_SCE_W;
		level_h = FORMAT_ALIGN_VAL_YUV420_SCE_H;
		break;
	case VPD_BUFTYPE_YUV420_MB:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_YUV420_16x2:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_YUV420_SP8:
		level_w = 16;
		level_h = 16;
		break;
	case VPD_BUFTYPE_ARGB1555:
	case VPD_BUFTYPE_ARGB8888:
	case VPD_BUFTYPE_RGB565:
	case VPD_BUFTYPE_RGB_CV:
	case VPD_BUFTYPE_DATA:
	default:
		level_w = 1;
		level_h = 1;
		break;
	}

	level->w = level_w;
	level->h = level_h;
}

////////////////////////////////////////////////////////////////////////////////
// type conversion
////////////////////////////////////////////////////////////////////////////////

vpd_buf_type_t common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(HD_VIDEO_PXLFMT pxlfmt)
{
	vpd_buf_type_t buffer_type = VPD_BUFTYPE_UNKNOWN;

	if (CIF_VIDEO_PXLFMT_YUV420_MB3 == pxlfmt) {
		return VPD_BUFTYPE_YUV420_8x4;
	}

	switch (pxlfmt) {
	case HD_VIDEO_PXLFMT_YUV422_ONE:
		buffer_type = VPD_BUFTYPE_YUV422;
		break;
	case HD_VIDEO_PXLFMT_YUV422_NVX3:
		buffer_type = VPD_BUFTYPE_YUV422_SCE;
		break;
	/*
	case HD_VIDEO_PXLFMT_YUV422:
		buffer_type = VPD_BUFTYPE_YUV422;
		break;
	case GM_FORMAT_YUV422_MB:
		buffer_type = VPD_BUFTYPE_YUV422_MB;
		break;*/
	case HD_VIDEO_PXLFMT_YUV420:
		buffer_type = VPD_BUFTYPE_YUV420_SP;
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX3:
		buffer_type = VPD_BUFTYPE_YUV420_SCE;
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX4:
		buffer_type = VPD_BUFTYPE_YUV420_LLC_8x4;
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB:
		buffer_type = VPD_BUFTYPE_YUV420_MB;
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB2:
		buffer_type = VPD_BUFTYPE_YUV420_16x2;
		break;
	case HD_VIDEO_PXLFMT_YUV420_W8:
		buffer_type = VPD_BUFTYPE_YUV420_SP8;
		break;

	case HD_VIDEO_PXLFMT_ARGB1555:
		buffer_type = VPD_BUFTYPE_ARGB1555;
		break;
	case HD_VIDEO_PXLFMT_ARGB8888:
		buffer_type = VPD_BUFTYPE_ARGB8888;
		break;
	case HD_VIDEO_PXLFMT_RGB565:
		buffer_type = VPD_BUFTYPE_RGB565;
		break;
	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		buffer_type = VPD_BUFTYPE_RGB_CV;
		break;
	/*	case GM_FORMAT_DATA:
			buffer_type = VPD_BUFTYPE_DATA;
			break;*/
	default:
		GMLIB_ERROR_P("%s, Not support for this format mapping(%#x)\n", __func__, pxlfmt);
		buffer_type = VPD_BUFTYPE_UNKNOWN;
		break;
	}
	return buffer_type;
}


unsigned int common_convert_HD_VIDEO_PXLFMT_to_buf_type_value(HD_VIDEO_PXLFMT pxlfmt)
{
	unsigned int buf_type = 0x00;//TYPE_UNKNOWN;

	switch (pxlfmt) {
	case HD_VIDEO_PXLFMT_YUV422_ONE:
		buf_type = 0x22;//TYPE_YUV422;
		break;
	case HD_VIDEO_PXLFMT_YUV422_NVX3:
		buf_type = 0x21;//TYPE_YUV422_SCE;
		break;
	case HD_VIDEO_PXLFMT_YUV420:
		buf_type = 0x10;//TYPE_YUV420_SP;
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX3:
		buf_type = 0x11;//TYPE_YUV420_SCE;
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX4:
		buf_type = 0x15;//TYPE_YUV420_LLC_8x4;
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB:
		buf_type = 0x12;//TYPE_YUV420_MB;
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB2:
		buf_type = 0x13;//TYPE_YUV420_16x2;
		break;
	case HD_VIDEO_PXLFMT_YUV420_W8:
		buf_type = 0x14;//TYPE_YUV420_SP8;
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB3:
		buf_type = 0x16;//TYPE_YUV420_8x4;
		break;
	case HD_VIDEO_PXLFMT_ARGB1555:
		buf_type = 0x31;//TYPE_RGB1555;
		break;
	case HD_VIDEO_PXLFMT_ARGB8888:
		buf_type = 0x32;//TYPE_RGB8888;
		break;
	case HD_VIDEO_PXLFMT_RGB565:
		buf_type = 0x33;//TYPE_RGB565;
		break;
	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		buf_type = 0x34;//TYPE_RGB_CV;
		break;
	default:
		break;
	}
	return buf_type;
}

HD_VIDEO_PXLFMT common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(vpd_buf_type_t pxlfmt)
{
	HD_VIDEO_PXLFMT buffer_type = HD_VIDEO_PXLFMT_NONE;

	switch (pxlfmt) {
	case VPD_BUFTYPE_YUV422:
		buffer_type = HD_VIDEO_PXLFMT_YUV422_ONE;
		break;
	case VPD_BUFTYPE_YUV422_SCE:
		buffer_type = HD_VIDEO_PXLFMT_YUV422_NVX3;
		break;
	case VPD_BUFTYPE_YUV420_SP:
		buffer_type = HD_VIDEO_PXLFMT_YUV420;
		break;
	case VPD_BUFTYPE_YUV420_SCE:
		buffer_type = HD_VIDEO_PXLFMT_YUV420_NVX3;
		break;
	case VPD_BUFTYPE_YUV420_LLC_8x4:
		buffer_type = HD_VIDEO_PXLFMT_YUV420_NVX4;
		break;
	case VPD_BUFTYPE_YUV420_MB:
		buffer_type = HD_VIDEO_PXLFMT_YUV420_MB;
		break;
	case VPD_BUFTYPE_YUV420_16x2:
		buffer_type = HD_VIDEO_PXLFMT_YUV420_MB2;
		break;
	case VPD_BUFTYPE_YUV420_SP8:
		buffer_type = HD_VIDEO_PXLFMT_YUV420_W8;
		break;
	case VPD_BUFTYPE_ARGB1555:
		buffer_type = HD_VIDEO_PXLFMT_ARGB1555;
		break;
	case VPD_BUFTYPE_ARGB8888:
		buffer_type = HD_VIDEO_PXLFMT_ARGB8888;
		break;
	case VPD_BUFTYPE_RGB565:
		buffer_type = HD_VIDEO_PXLFMT_RGB565;
		break;
	case VPD_BUFTYPE_RGB_CV:
		buffer_type = HD_VIDEO_PXLFMT_RGB888_PLANAR;
		break;
	case VPD_BUFTYPE_YUV420_8x4:
		buffer_type = HD_VIDEO_PXLFMT_YUV420_MB3;
		break;
	default:
		GMLIB_ERROR_P("%s, Not support for this vpd format mapping(%#x)\n", __func__, pxlfmt);
		buffer_type = HD_VIDEO_PXLFMT_NONE;
		break;
	}
	return buffer_type;
}

vpbuf_pool_type_t HD_COMMON_MEM_POOL_TYPE_to_vpd_type(HD_COMMON_MEM_POOL_TYPE type)
{
	vpbuf_pool_type_t vpd_type;

	switch (type) {
	case HD_COMMON_MEM_COMMON_POOL:
		vpd_type = VPD_COMMON_POOL;
		break;
	case HD_COMMON_MEM_DISP0_IN_POOL:
		vpd_type = VPD_DISP0_IN_POOL;
		break;
	case HD_COMMON_MEM_DISP1_IN_POOL:
		vpd_type = VPD_DISP1_IN_POOL;
		break;
	case HD_COMMON_MEM_DISP2_IN_POOL:
		vpd_type = VPD_DISP2_IN_POOL;
		break;
	case HD_COMMON_MEM_DISP3_IN_POOL:
		vpd_type = VPD_DISP3_IN_POOL;
		break;
	case HD_COMMON_MEM_DISP4_IN_POOL:
		vpd_type = VPD_DISP4_IN_POOL;
		break;
	case HD_COMMON_MEM_DISP5_IN_POOL:
		vpd_type = VPD_DISP5_IN_POOL;
		break;
	case HD_COMMON_MEM_DISP0_CAP_OUT_POOL:
		vpd_type = VPD_DISP0_CAP_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP1_CAP_OUT_POOL:
		vpd_type = VPD_DISP1_CAP_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP2_CAP_OUT_POOL:
		vpd_type = VPD_DISP2_CAP_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP3_CAP_OUT_POOL:
		vpd_type = VPD_DISP3_CAP_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP4_CAP_OUT_POOL:
		vpd_type = VPD_DISP4_CAP_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP5_CAP_OUT_POOL:
		vpd_type = VPD_DISP5_CAP_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP0_ENC_SCL_OUT_POOL:
		vpd_type = VPD_DISP0_ENC_SCL_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP1_ENC_SCL_OUT_POOL:
		vpd_type = VPD_DISP1_ENC_SCL_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP2_ENC_SCL_OUT_POOL:
		vpd_type = VPD_DISP2_ENC_SCL_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP3_ENC_SCL_OUT_POOL:
		vpd_type = VPD_DISP3_ENC_SCL_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP4_ENC_SCL_OUT_POOL:
		vpd_type = VPD_DISP4_ENC_SCL_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP5_ENC_SCL_OUT_POOL:
		vpd_type = VPD_DISP5_ENC_SCL_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP0_ENC_OUT_POOL:
		vpd_type = VPD_DISP0_ENC_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP1_ENC_OUT_POOL:
		vpd_type = VPD_DISP1_ENC_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP2_ENC_OUT_POOL:
		vpd_type = VPD_DISP2_ENC_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP3_ENC_OUT_POOL:
		vpd_type = VPD_DISP3_ENC_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP4_ENC_OUT_POOL:
		vpd_type = VPD_DISP4_ENC_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP5_ENC_OUT_POOL:
		vpd_type = VPD_DISP5_ENC_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP_DEC_IN_POOL:
		vpd_type = VPD_DISP_DEC_IN_POOL;
		break;
	case HD_COMMON_MEM_DISP_DEC_OUT_POOL:
		vpd_type = VPD_DISP_DEC_OUT_POOL;
		break;
	case HD_COMMON_MEM_DISP_DEC_OUT_RATIO_POOL:
		vpd_type = VPD_DISP_DEC_OUT_RATIO_POOL;
		break;
	case HD_COMMON_MEM_ENC_CAP_OUT_POOL:
		vpd_type = VPD_ENC_CAP_OUT_POOL;
		break;
	case HD_COMMON_MEM_ENC_SCL_OUT_POOL:
		vpd_type = VPD_ENC_SCL_OUT_POOL;
		break;
	case HD_COMMON_MEM_ENC_OUT_POOL:
		vpd_type = VPD_ENC_OUT_POOL;
		break;
	case HD_COMMON_MEM_AU_ENC_AU_GRAB_OUT_POOL:
		vpd_type = VPD_AU_ENC_AU_GRAB_OUT_POOL;
		break;
	case HD_COMMON_MEM_AU_DEC_AU_RENDER_IN_POOL:
		vpd_type = VPD_AU_DEC_AU_RENDER_IN_POOL;
		break;
	case HD_COMMON_MEM_FLOW_MD_POOL:
		vpd_type = VPD_FLOW_MD_POOL;
		break;
	case HD_COMMON_MEM_USER_BLK:
		vpd_type = VPD_USER_BLK;
		break;
#if 0
	case HD_COMMON_MEM_DISP0_FB_POOL:
		vpd_type = VPD_DISP0_FB_POOL;
		break;
	case HD_COMMON_MEM_DISP1_FB_POOL:
		vpd_type = VPD_DISP1_FB_POOL;
		break;
	case HD_COMMON_MEM_DISP2_FB_POOL:
		vpd_type = VPD_DISP2_FB_POOL;
		break;
	case HD_COMMON_MEM_DISP3_FB_POOL:
		vpd_type = VPD_DISP3_FB_POOL;
		break;
	case HD_COMMON_MEM_DISP4_FB_POOL:
		vpd_type = VPD_DISP4_FB_POOL;
		break;
	case HD_COMMON_MEM_DISP5_FB_POOL:
		vpd_type = VPD_DISP5_FB_POOL;
		break;
	case HD_COMMON_MEM_ENC_REF_POOL:
		vpd_type = VPD_ENC_REF_POOL;
		break;
#endif
	default:
		vpd_type = VPD_END_POOL;
		break;
	}
	return vpd_type;
}

vpd_dec_type_t common_convert_hd_codec_type_to_vpd_buffer_type(int type)
{
	vpd_dec_type_t vpd_type = -1;
	switch (type) {
	case HD_CODEC_TYPE_JPEG:
		vpd_type = VPD_DEC_TYPE_JPEG;
		break;
	case HD_CODEC_TYPE_H264:
		vpd_type = VPD_DEC_TYPE_H264;
		break;
	case HD_CODEC_TYPE_H265:
		vpd_type = VPD_DEC_TYPE_H265;
		break;
	}
	return vpd_type;
}

char* common_HD_VIDEO_PXLFMT_str(HD_VIDEO_PXLFMT pxlfmt)
{
	switch(pxlfmt) {
	case HD_VIDEO_PXLFMT_NONE: ///< don't care
		return "HD_VIDEO_PXLFMT_NONE";
	/* 0 = osd icon index format */
	case HD_VIDEO_PXLFMT_I1_ICON: ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
		return "HD_VIDEO_PXLFMT_I1_ICON";
	case HD_VIDEO_PXLFMT_I2_ICON: ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
		return "HD_VIDEO_PXLFMT_I2_ICON";
	case HD_VIDEO_PXLFMT_I4_ICON: ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
		return "HD_VIDEO_PXLFMT_I4_ICON";
	case HD_VIDEO_PXLFMT_I8_ICON: ///< 1 plane, pixel=INDEX(w,h), w/o padding bits
		return "HD_VIDEO_PXLFMT_I8_ICON";
	/* 1 = osd index format */
	case HD_VIDEO_PXLFMT_I1: ///< 1 plane, pixel=INDEX(w,h)
		return "HD_VIDEO_PXLFMT_I1";
	case HD_VIDEO_PXLFMT_I2: ///< 1 plane, pixel=INDEX(w,h)
		return "HD_VIDEO_PXLFMT_I2";
	case HD_VIDEO_PXLFMT_I4: ///< 1 plane, pixel=INDEX(w,h)
		return "HD_VIDEO_PXLFMT_I4";
	case HD_VIDEO_PXLFMT_I8: ///< 1 plane, pixel=INDEX(w,h)
		return "HD_VIDEO_PXLFMT_I8";
	/* 2 = osd argb format */
	case HD_VIDEO_PXLFMT_RGB888_PLANAR: ///< 3 plane, pixel=R(w,h), G(w,h), B(w,h)
		return "HD_VIDEO_PXLFMT_RGB888_PLANAR";
	case HD_VIDEO_PXLFMT_RGB888: ///< 1 plane, pixel=RGB(3w,h)
		return "HD_VIDEO_PXLFMT_RGB888";
	case HD_VIDEO_PXLFMT_RGB565: ///< 1 plane, pixel=RGB(2w,h)
		return "HD_VIDEO_PXLFMT_RGB565";
	case HD_VIDEO_PXLFMT_RGBA5551: ///< 1 plane, pixel=ARGB(2w,h)
		return "HD_VIDEO_PXLFMT_RGBA5551";
	case HD_VIDEO_PXLFMT_ARGB1555: ///< 1 plane, pixel=ARGB(2w,h)
		return "HD_VIDEO_PXLFMT_ARGB1555";
	case HD_VIDEO_PXLFMT_ARGB4444: ///< 1 plane, pixel=ARGB(2w,h)
		return "HD_VIDEO_PXLFMT_ARGB4444";
	case HD_VIDEO_PXLFMT_A4: ///< 1 plane, pixel=A(w,h)
		return "HD_VIDEO_PXLFMT_A4";
	case HD_VIDEO_PXLFMT_ARGB4565: ///< 2 plane, pixel=A4(w,h),RGB565(2w,h)
		return "HD_VIDEO_PXLFMT_ARGB4565";
	case HD_VIDEO_PXLFMT_A8: ///< 1 plane, pixel=A(w,h)
		return "HD_VIDEO_PXLFMT_A8";
	case HD_VIDEO_PXLFMT_ARGB8565: ///< 2 plane, pixel=A8(w,h),RGB565(2w,h)
		return "HD_VIDEO_PXLFMT_ARGB8565";
	case HD_VIDEO_PXLFMT_ARGB8888: ///< 1 plane, pixel=ARGB(4w,h)
		return "HD_VIDEO_PXLFMT_ARGB8888";
	/* 3 = osd compressed argb format */
	case HD_VIDEO_PXLFMT_RGB888_RLE: ///< novatek-rgb-compress
		return "HD_VIDEO_PXLFMT_RGB888_RLE";
	case HD_VIDEO_PXLFMT_RGB565_RLE: ///< novatek-rgb-compress
		return "HD_VIDEO_PXLFMT_RGB565_RLE";
	case HD_VIDEO_PXLFMT_RGBA5551_RLE: ///< novatek-rgb-compress
		return "HD_VIDEO_PXLFMT_RGBA5551_RLE";
	case HD_VIDEO_PXLFMT_ARGB1555_RLE: ///< novatek-rgb-compress
		return "HD_VIDEO_PXLFMT_ARGB1555_RLE";
	case HD_VIDEO_PXLFMT_ARGB8888_RLE: ///< novatek-rgb-compress
		return "HD_VIDEO_PXLFMT_ARGB8888_RLE";
	/* 4 = video raw format */
	case HD_VIDEO_PXLFMT_RAW8: ///< 1 plane, pixel=RAW(w,h) x 8bits
		return "HD_VIDEO_PXLFMT_RAW8";
	case HD_VIDEO_PXLFMT_RAW10: ///< 1 plane, pixel=RAW(w,h) x 10bits
		return "HD_VIDEO_PXLFMT_RAW10";
	case HD_VIDEO_PXLFMT_RAW12: ///< 1 plane, pixel=RAW(w,h) x 12bits
		return "HD_VIDEO_PXLFMT_RAW12";
	case HD_VIDEO_PXLFMT_RAW14: ///< 1 plane, pixel=RAW(w,h) x 14bits
		return "HD_VIDEO_PXLFMT_RAW14";
	case HD_VIDEO_PXLFMT_RAW16: ///< 1 plane, pixel=RAW(w,h) x 16bits
		return "HD_VIDEO_PXLFMT_RAW16";
	case HD_VIDEO_PXLFMT_RAW8_SHDR2: ///< 2 plane, pixel=RAW(w,h) x 8bits, for two-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW8_SHDR2";
	case HD_VIDEO_PXLFMT_RAW10_SHDR2: ///< 2 plane, pixel=RAW(w,h) x 10bits, for two-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW10_SHDR2";
	case HD_VIDEO_PXLFMT_RAW12_SHDR2: ///< 2 plane, pixel=RAW(w,h) x 12bits, for two-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW12_SHDR2";
	case HD_VIDEO_PXLFMT_RAW14_SHDR2: ///< 2 plane, pixel=RAW(w,h) x 14bits, for two-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW14_SHDR2";
	case HD_VIDEO_PXLFMT_RAW16_SHDR2: ///< 2 plane, pixel=RAW(w,h) x 16bits, for two-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW16_SHDR2";
	case HD_VIDEO_PXLFMT_RAW8_SHDR3: ///< 3 plane, pixel=RAW(w,h) x 8bits, for three-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW8_SHDR3";
	case HD_VIDEO_PXLFMT_RAW10_SHDR3: ///< 3 plane, pixel=RAW(w,h) x 10bits, for three-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW10_SHDR3";
	case HD_VIDEO_PXLFMT_RAW12_SHDR3: ///< 3 plane, pixel=RAW(w,h) x 12bits, for three-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW12_SHDR3";
	case HD_VIDEO_PXLFMT_RAW14_SHDR3: ///< 3 plane, pixel=RAW(w,h) x 14bits, for three-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW14_SHDR3";
	case HD_VIDEO_PXLFMT_RAW16_SHDR3: ///< 3 plane, pixel=RAW(w,h) x 16bits, for three-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW16_SHDR3";
	case HD_VIDEO_PXLFMT_RAW8_SHDR4: ///< 4 plane, pixel=RAW(w,h) x 8bits, for four-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW8_SHDR4";
	case HD_VIDEO_PXLFMT_RAW10_SHDR4: ///< 4 plane, pixel=RAW(w,h) x 10bits, for four-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW10_SHDR4";
	case HD_VIDEO_PXLFMT_RAW12_SHDR4: ///< 4 plane, pixel=RAW(w,h) x 12bits, for four-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW12_SHDR4";
	case HD_VIDEO_PXLFMT_RAW14_SHDR4: ///< 4 plane, pixel=RAW(w,h) x 14bits, for four-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW14_SHDR4";
	case HD_VIDEO_PXLFMT_RAW16_SHDR4: ///< 4 plane, pixel=RAW(w,h) x 16bits, for four-frame SHDR
		return "HD_VIDEO_PXLFMT_RAW16_SHDR4";
	/* 5 = video yuv format */
	case HD_VIDEO_PXLFMT_Y8: ///< 1 plane, pixel=Y(w,h)
		return "HD_VIDEO_PXLFMT_Y8 (YUV400)";
	case HD_VIDEO_PXLFMT_YUV420_PLANAR: ///< 3 plane, pixel=Y(w,h), U(w/2,h/2), and V(w/2,h/2)
		return "HD_VIDEO_PXLFMT_YUV420_PLANAR";
	case HD_VIDEO_PXLFMT_YUV420: ///< 2 plane, pixel=Y(w,h), UV(w/2,h/2), semi-planer format with U1V1
		return "HD_VIDEO_PXLFMT_YUV420";
	case HD_VIDEO_PXLFMT_YUV422_PLANAR: ///< 3 plane, pixel=Y(w,h), U(w/2,h), and V(w/2,h)
		return "HD_VIDEO_PXLFMT_YUV422_PLANAR";
	case HD_VIDEO_PXLFMT_YUV422: ///< 2 plane, pixel=Y(w,h), UV(w/2,h), semi-planer format with U1V1
		return "HD_VIDEO_PXLFMT_YUV422";
	case HD_VIDEO_PXLFMT_YUV422_ONE: ///< 1 plane, pixel=UYVY(w,h), packed format with Y2U1V1
		return "HD_VIDEO_PXLFMT_YUV422_ONE (UYVY)";
	case HD_VIDEO_PXLFMT_YUV422_VYUY: ///< 1 plane, pixel=VYUY(w,h), packed format with Y2U1V1
		return "HD_VIDEO_PXLFMT_YUV422_VYUY";
	case HD_VIDEO_PXLFMT_YUV422_YUYV: ///< 1 plane, pixel=YUYV(w,h), packed format with Y2U1V1
		return "HD_VIDEO_PXLFMT_YUV422_YUYV";
	case HD_VIDEO_PXLFMT_YUV422_YVYU: ///< 1 plane, pixel=YVYU(w,h), packed format with Y2U1V1
		return "HD_VIDEO_PXLFMT_YUV422_YVYU";
	case HD_VIDEO_PXLFMT_YUV444_PLANAR: ///< 3 plane, pixel=Y(w,h), U(w,h), and V(w,h)
		return "HD_VIDEO_PXLFMT_YUV444_PLANAR";
	case HD_VIDEO_PXLFMT_YUV444: ///< 2 plane, pixel=Y(w,h), UV(w,h), semi-planer format with U1V1
		return "HD_VIDEO_PXLFMT_YUV444";
	case HD_VIDEO_PXLFMT_YUV444_ONE: ///< 1 plane, pixel=YUV(w,h), packed format with Y1U1V1
		return "HD_VIDEO_PXLFMT_YUV444_ONE";
	case HD_VIDEO_PXLFMT_YUV420_W8: ///< 2 plane, pixel=Y(w,h), UV(w/2,h/2), semi-planer format with U8V8
		return "HD_VIDEO_PXLFMT_YUV420_W8";
	case HD_VIDEO_PXLFMT_YUV420_MB: ///< 1 plane, 16x16 macro block format
		return "HD_VIDEO_PXLFMT_YUV420_MB";
	case HD_VIDEO_PXLFMT_YUV420_MB2: ///< 1 plane, 16x2 macro block format
		return "HD_VIDEO_PXLFMT_YUV420_MB2";
	case HD_VIDEO_PXLFMT_YUV420_MB3: ///< 1 plane, 1 plane, pixel=Y(w,h), UV(w/2,h/2), semi-planer format 8x4
		return "HD_VIDEO_PXLFMT_YUV420_MB3";
	/* 6 = video yuv compress format */
	case HD_VIDEO_PXLFMT_YUV420_NVX1_H264: ///< novatek-yuv-compress-1 of YUV420 for h264
		return "HD_VIDEO_PXLFMT_YUV420_NVX1_H264";
	case HD_VIDEO_PXLFMT_YUV420_NVX1_H265: ///< novatek-yuv-compress-1 of YUV420 for h265
		return "HD_VIDEO_PXLFMT_YUV420_NVX1_H265";
	case HD_VIDEO_PXLFMT_YUV420_NVX2: ///< novatek-yuv-compress-2 of YUV420
		return "HD_VIDEO_PXLFMT_YUV420_NVX2";
	case HD_VIDEO_PXLFMT_YUV420_NVX3: ///< novatek-yuv-compress-3 of YUV420
		return "HD_VIDEO_PXLFMT_YUV420_NVX3";
	case HD_VIDEO_PXLFMT_YUV420_NVX4: ///< novatek-yuv-compress-4 of YUV420 for h264/h265 LLC_8x4
		return "HD_VIDEO_PXLFMT_YUV420_NVX4";
	case HD_VIDEO_PXLFMT_YUV422_NVX3: ///< novatek-yuv-compress-3 of YUV422
		return "HD_VIDEO_PXLFMT_YUV422_NVX3";
	case HD_VIDEO_PXLFMT_YUV420_NVX5: ///< novatek-yuv-compress-5 of YUV420 for VPE
		return "HD_VIDEO_PXLFMT_YUV420_NVX5";
	/* f = video raw compress format */
	case HD_VIDEO_PXLFMT_NRX8: ///< novatek-raw-compress-1 of RAW8
		return "HD_VIDEO_PXLFMT_NRX8";
	case HD_VIDEO_PXLFMT_NRX10: ///< novatek-raw-compress-1 of RAW10
		return "HD_VIDEO_PXLFMT_NRX10";
	case HD_VIDEO_PXLFMT_NRX12: ///< novatek-raw-compress-1 of RAW12
		return "HD_VIDEO_PXLFMT_NRX12";
	case HD_VIDEO_PXLFMT_NRX14: ///< novatek-raw-compress-1 of RAW14
		return "HD_VIDEO_PXLFMT_NRX14";
	case HD_VIDEO_PXLFMT_NRX16: ///< novatek-raw-compress-1 of RAW16
		return "HD_VIDEO_PXLFMT_NRX16";
	case HD_VIDEO_PXLFMT_NRX8_SHDR2: ///< novatek-raw-compress-1 of RAW8
		return "HD_VIDEO_PXLFMT_NRX8_SHDR2";
	case HD_VIDEO_PXLFMT_NRX10_SHDR2: ///< novatek-raw-compress-1 of RAW10
		return "HD_VIDEO_PXLFMT_NRX10_SHDR2";
	case HD_VIDEO_PXLFMT_NRX12_SHDR2: ///< novatek-raw-compress-1 of RAW12
		return "HD_VIDEO_PXLFMT_NRX12_SHDR2";
	case HD_VIDEO_PXLFMT_NRX14_SHDR2: ///< novatek-raw-compress-1 of RAW14
		return "HD_VIDEO_PXLFMT_NRX14_SHDR2";
	case HD_VIDEO_PXLFMT_NRX16_SHDR2: ///< novatek-raw-compress-1 of RAW16
		return "HD_VIDEO_PXLFMT_NRX16_SHDR2";
	case HD_VIDEO_PXLFMT_NRX8_SHDR3: ///< novatek-raw-compress-1 of RAW8
		return "HD_VIDEO_PXLFMT_NRX8_SHDR3";
	case HD_VIDEO_PXLFMT_NRX10_SHDR3: ///< novatek-raw-compress-1 of RAW10
		return "HD_VIDEO_PXLFMT_NRX10_SHDR3";
	case HD_VIDEO_PXLFMT_NRX12_SHDR3: ///< novatek-raw-compress-1 of RAW12
		return "HD_VIDEO_PXLFMT_NRX12_SHDR3";
	case HD_VIDEO_PXLFMT_NRX14_SHDR3: ///< novatek-raw-compress-1 of RAW14
		return "HD_VIDEO_PXLFMT_NRX14_SHDR3";
	case HD_VIDEO_PXLFMT_NRX16_SHDR3: ///< novatek-raw-compress-1 of RAW16
		return "HD_VIDEO_PXLFMT_NRX16_SHDR3";
	case HD_VIDEO_PXLFMT_NRX8_SHDR4: ///< novatek-raw-compress-1 of RAW8
		return "HD_VIDEO_PXLFMT_NRX8_SHDR4";
	case HD_VIDEO_PXLFMT_NRX10_SHDR4: ///< novatek-raw-compress-1 of RAW10
		return "HD_VIDEO_PXLFMT_NRX10_SHDR4";
	case HD_VIDEO_PXLFMT_NRX12_SHDR4: ///< novatek-raw-compress-1 of RAW12
		return "HD_VIDEO_PXLFMT_NRX12_SHDR4";
	case HD_VIDEO_PXLFMT_NRX14_SHDR4: ///< novatek-raw-compress-1 of RAW14
		return "HD_VIDEO_PXLFMT_NRX14_SHDR4";
	case HD_VIDEO_PXLFMT_NRX16_SHDR4: ///< novatek-raw-compress-1 of RAW16
		return "HD_VIDEO_PXLFMT_NRX16_SHDR4";
	default:
		return "unknown format";
	}
}

int common_calculate_frame_buf_size(dim_t *dim, vpd_buf_type_t type)
{
	int  size = 0, w = dim->w, h = dim->h;

	switch (type) {
	case VPD_BUFTYPE_YUV422:
		//size = w * h * 2;
		size = FORMAT_SIZE_YUV422(w, h);
		break;

	case VPD_BUFTYPE_YUV422_SCE:
		size = FORMAT_SIZE_YUV422_SCE(w, h);
		break;
	case VPD_BUFTYPE_YUV422_MB:
		size = FORMAT_SIZE_YUV422_MB(w, h);
		break;

	case VPD_BUFTYPE_YUV420_SP:
		//size = w * h * 3 / 2;
		size = FORMAT_SIZE_YUV420_SP(w, h);
		break;
	case VPD_BUFTYPE_YUV420_SCE:
		size = FORMAT_SIZE_YUV420_SCE(w, h);
		break;
	case VPD_BUFTYPE_YUV420_MB:
		//size = w * h * 3 / 2;
		size = FORMAT_SIZE_YUV420_MB(w, h);
		break;

	case VPD_BUFTYPE_YUV420_16x2:
		//size = ALIGN64_UP(w) * ALIGN64_UP(h) * 3 / 2;
		size = FORMAT_SIZE_YUV420_16X2(w, h);
		break;
	case VPD_BUFTYPE_YUV420_SP8:
		//size = w * h * 3 / 2
		size = FORMAT_SIZE_YUV420_SP8(w, h);
		break;
	case VPD_BUFTYPE_ARGB1555:
		//size = w * h * 2;
		size = FORMAT_SIZE_ARGB1555(w, h);
		break;
	case VPD_BUFTYPE_ARGB8888:
		//size = w * h * 4;
		size = FORMAT_SIZE_ARGB8888(w, h);
		break;
	case VPD_BUFTYPE_RGB565:
		//size = w * h * 2;
		size = FORMAT_SIZE_RGB565(w, h);
		break;
	case VPD_BUFTYPE_RGB_CV:
		//size = w * h * 2;
		size = FORMAT_SIZE_RGB_CV(w, h);
		break;
	case VPD_BUFTYPE_YUV420_8x4:
		size = FORMAT_SIZE_YUV420_SP8(w, h);
		break;
	case VPD_BUFTYPE_DATA:
	default:
		GMLIB_ERROR_P("%s, not implement type(%d)\n", __func__, type);
	}
	if (type == 0) {
		GMLIB_ERROR_P("%s, Can't simulate on 829x. type(%d)\n", __func__, type);
		exit(0);
	}

	return size;
}

unsigned int common_calculate_buf_size_by_HD_VIDEO_PXLFMT(HD_DIM dim, HD_VIDEO_PXLFMT pxlfmt)
{
	int  buf_size = 0, w = dim.w, h = dim.h;

	if (pxlfmt == CIF_VIDEO_PXLFMT_YUV420_MB3) {
		return FORMAT_SIZE_YUV420_SP8(w, h);
	}

	switch (pxlfmt) {
	case HD_VIDEO_PXLFMT_YUV420:
	case HD_VIDEO_PXLFMT_YUV420_PLANAR:
		buf_size = FORMAT_SIZE_YUV420_SP(w, h);
		break;
	case HD_VIDEO_PXLFMT_YUV422:
	case HD_VIDEO_PXLFMT_YUV422_PLANAR:
	case HD_VIDEO_PXLFMT_YUV422_ONE:
	case HD_VIDEO_PXLFMT_ARGB1555:
		buf_size = FORMAT_SIZE_YUV422(w, h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_W8:    ///< 2 plane, pixel=Y(w,h), UV(w/2,h/2), semi-planer format with U8V8 (using by 313)
		buf_size = FORMAT_SIZE_YUV420_SP8(w, h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB:    ///< 1 plane, 16x16 macro block format (using by 313)
		buf_size = FORMAT_SIZE_YUV420_MB(w, h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB2:   ///< 1 plane, 16x2 macro block format (using by 313)
		buf_size = FORMAT_SIZE_YUV420_16X2(w, h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX3:
		buf_size = FORMAT_SIZE_YUV420_SCE(w, h);
		break;
	case HD_VIDEO_PXLFMT_YUV422_NVX3:
		buf_size = FORMAT_SIZE_YUV422_SCE(w, h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX4:
		buf_size = FORMAT_SIZE_YUV420_LLC_8x4(w, h);
		buf_size += common_calculate_dec_side_info_size(&dim, HD_CODEC_TYPE_H265);
		buf_size += common_calculate_dec_mb_info_size(&dim, HD_CODEC_TYPE_H265);
		break;
	case HD_VIDEO_PXLFMT_ARGB8888:
		buf_size = FORMAT_SIZE_ARGB8888(w, h);
		break;

	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		buf_size = FORMAT_SIZE_RGB_CV(w, h);
		break;

	default:
		GMLIB_ERROR_P("%s, not implement pxlfmt(%#x)\n", __func__, pxlfmt);
	}
	return buf_size;
}


int common_calculate_md_info_size(HD_DIM *dim)
{
	int mb_size, mb_w_cnt, mb_h_cnt, size;
	mb_size = (dim->w <= 1920 && dim->h <= 1080) ? 16 : 32;
	if ((mb_w_cnt = dim->w / mb_size) < 256)
		mb_w_cnt = 256;
	mb_h_cnt = dim->h / mb_size;
	size = ALIGN8_UP(mb_w_cnt * mb_h_cnt * 2) / 8;
	return size;
}

int common_calculate_dec_mb_info_size(HD_DIM *dim, HD_VIDEO_CODEC codec)
{
	int  buf_size = 0;

	switch (codec) {
	case HD_CODEC_TYPE_JPEG:
		buf_size = 0;
		break;
	case HD_CODEC_TYPE_H264:
	case HD_CODEC_TYPE_H265:
		buf_size = ALIGN64_UP(dim->w) * ALIGN64_UP(dim->h) / 16;
		break;
	default:
		GMLIB_ERROR_P("%s, not implement codec(%d)\n", __func__, codec);
		break;
	}
	return buf_size;
}

/* for video decode YUV420_LLC_8x4 side info */
int common_calculate_dec_side_info_size(HD_DIM *dim, HD_VIDEO_CODEC codec)
{
	int  buf_size = 0;

	switch (codec) {
	case HD_CODEC_TYPE_JPEG:
		buf_size = 0;
		break;
	case HD_CODEC_TYPE_H264:
	case HD_CODEC_TYPE_H265:
		buf_size = ALIGN4K_UP(ALIGN128_UP(ALIGN64_UP(dim->w) / 4) * (ALIGN64_UP(dim->h) / 8) * 3 / 2);
		break;
	default:
		GMLIB_ERROR_P("%s, not implement codec(%d)\n", __func__, codec);
		break;
	}
	return buf_size;
}

/* for video decode YUV420_LLC_8x4 side info */
int common_calculate_dec_side_info_offset(HD_DIM *dim, HD_VIDEO_CODEC codec)
{
	int  buf_size = 0;

	switch (codec) {
	case HD_CODEC_TYPE_JPEG:
		buf_size = 0;
		break;
	case HD_CODEC_TYPE_H264:
	case HD_CODEC_TYPE_H265:
		// side_info offset will equal to size of YUV420_NVX4
		buf_size = FORMAT_SIZE_YUV420_LLC_8x4(dim->w, dim->h);
		break;
	default:
		GMLIB_ERROR_P("%s, not implement codec(%d)\n", __func__, codec);
		break;
	}
	return buf_size;
}
