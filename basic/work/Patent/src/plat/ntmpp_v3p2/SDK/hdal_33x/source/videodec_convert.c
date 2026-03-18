/**
	@brief This sample demonstrates the usage of video decode functions.

	@file videodec_convert.c

	@author K L Chu

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
#include "hdal.h"
#include "vpd_ioctl.h"
#include "bind.h"
#include "logger.h"
#include "cif_common.h"
#include "dif.h"
#include "pif.h"
#include "utl.h"
#include "videodec_convert.h"
#include "vg_common.h"
#include "kflow_videodec.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define BS_WIN_COUNT            2

#define CONFIG_DEFAULT_QP   	15	// min qp value
#define CONFIG_DEFAULT_COMPRESS	30	// min compress rate
#define CONFIG_DEFAULT_JPG_QP 	85	// min qp value

#define CONFIG_MIN_BS_WIDTH	    640
#define CONFIG_MIN_BS_HEIGHT    480
#define CONFIG_MIN_BS_SIZE		(CONFIG_MIN_BS_WIDTH * CONFIG_MIN_BS_HEIGHT * 1.5 * CONFIG_DEFAULT_COMPRESS / 100)
#define	DIN_VAR_POOL_NUM		1  // datain max variable pool number

#define DEFAULT_MAX_COUNT       2  // 1(dec dispatch job) + 1(dec basic ref & vpe repeat)
#define DEFAULT_MIN_COUNT       0

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
VDEC_CVT_CONFIG vdec_convert_config[VDODEC_OUTPUT_PORT] = {0};

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern unsigned int gmlib_dbg_level;
extern unsigned int gmlib_dbg_mode;
extern vpd_sys_info_t platform_sys_Info;
extern vpd_sys_spec_info_t platform_sys_spec_Info;
extern KFLOW_VIDEODEC_HW_SPEC_INFO vdodec_hw_spec_info;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/
static unsigned int _get_max_counts(unsigned int counts, unsigned int max_ref_counts)
{
	unsigned int counts_carry, max_counts;

	counts_carry = (counts / 10) + (counts % 10 != 0); // unconditional carry

	if (counts_carry < max_ref_counts) {
		max_counts = 0; // invalid case
	} else {
		max_counts = counts_carry - max_ref_counts + 1; // +1 stands for vpe repeat and dec basic reference will use the same buffer
	}

	return max_counts;
}

static HD_RESULT _get_sub_yuv_dim(HD_DIM dim, HD_DIM sub_align, UINT32 sub_ratio, HD_DIM *p_sub_dim)
{
	UINT32 tmp_ratio;

	if (p_sub_dim == NULL) {
		GMLIB_ERROR_P("p_sub_dim NULL\n");
		return HD_ERR_NULL_PTR;
	}

	switch (sub_ratio) {
	case 0:                          // didn't set
	case VDEC_CVT_SUB_RATIO_OFF:     // turn off sub-yuv output
		tmp_ratio = SUB_RATIO_DEFAULT_VALUE;
		break;

	case VDEC_CVT_SUB_RATIO_2x:      // 1/2
	case VDEC_CVT_SUB_RATIO_3x:      // 1/3
	case VDEC_CVT_SUB_RATIO_4x:      // 1/4
		tmp_ratio = sub_ratio;
		break;

	default:
		GMLIB_ERROR_P("invalid value, sub_yuv_ratio(%lu)\n", sub_ratio);
		return HD_ERR_NOT_SUPPORT;
	}
	p_sub_dim->w = ALIGN_CEIL(dim.w / tmp_ratio, sub_align.w);
	p_sub_dim->h = ALIGN_CEIL(dim.h / tmp_ratio, sub_align.h);

	return HD_OK;
}

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
int get_bs_ratio(HD_VIDEO_CODEC codec_type, int qp)
{
	unsigned int i;

	if (codec_type == HD_CODEC_TYPE_H264) {
		for (i = 0; i < (sizeof(platform_sys_spec_Info.h264_qp_ratio_table) / sizeof(DIF_VIDEOENC_QP_RATIO)); i++) {
			if (qp >= platform_sys_spec_Info.h264_qp_ratio_table[i].qp) {
				return platform_sys_spec_Info.h264_qp_ratio_table[i].ratio;
			}
		}
	} else if (codec_type == HD_CODEC_TYPE_H265) {
		for (i = 0; i < (sizeof(platform_sys_spec_Info.h265_qp_ratio_table) / sizeof(DIF_VIDEOENC_QP_RATIO)); i++) {
			if (qp >= platform_sys_spec_Info.h265_qp_ratio_table[i].qp) {
				return platform_sys_spec_Info.h265_qp_ratio_table[i].ratio;
			}
		}
	} else if (codec_type == HD_CODEC_TYPE_JPEG) {
		for (i = 0; i < (sizeof(platform_sys_spec_Info.jpg_qp_ratio_table) / sizeof(DIF_VIDEOENC_QP_RATIO)); i++) {
			if (qp >= platform_sys_spec_Info.jpg_qp_ratio_table[i].qp) {
				return platform_sys_spec_Info.jpg_qp_ratio_table[i].ratio;
			}
		}
	} else if (codec_type == HD_CODEC_TYPE_RAW) {
		return 100;
	}

	return 30;
}

int get_align_by_codec(HD_VIDEO_CODEC codec_type, HD_DIM *main_align, HD_DIM *sub_align)
{
	int ret = 0;

	if (main_align == NULL) {
		GMLIB_ERROR_P("main_align is NULL\n");
		ret = -1;
		goto exit;
	}

	if (sub_align == NULL) {
		GMLIB_ERROR_P("sub_align is NULL\n");
		ret = -1;
		goto exit;
	}

	if (codec_type == HD_CODEC_TYPE_H264) {
		main_align->w = vdodec_hw_spec_info.h264.first_align.w;
		main_align->h = vdodec_hw_spec_info.h264.first_align.h;
		sub_align->w = vdodec_hw_spec_info.h264.extra_align.w;
		sub_align->h = vdodec_hw_spec_info.h264.extra_align.h;
	} else if (codec_type == HD_CODEC_TYPE_H265) {
		main_align->w = vdodec_hw_spec_info.h265.first_align.w;
		main_align->h = vdodec_hw_spec_info.h265.first_align.h;
		sub_align->w = vdodec_hw_spec_info.h265.extra_align.w;
		sub_align->h = vdodec_hw_spec_info.h265.extra_align.h;
	} else if (codec_type == HD_CODEC_TYPE_JPEG) {
		main_align->w = vdodec_hw_spec_info.jpeg.align.w;
		main_align->h = vdodec_hw_spec_info.jpeg.align.h;
		sub_align->w = 1;
		sub_align->h = 1;
	} else {
		GMLIB_ERROR_P("unknown codec_type(%d)\n", codec_type);
		ret = -1;
		goto exit;
	}

	if (main_align->w == 0 || main_align->h == 0 ||
	    sub_align->w == 0 || sub_align->h == 0) {
		GMLIB_ERROR_P("get align dim error main(%dx%d) sub(%dx%d)\n",
			      main_align->w, main_align->h, sub_align->w, sub_align->h);
		ret = -1;
		goto exit;
	}

exit:
	return ret;
}

INT set_datain_unit(HD_GROUP *hd_group, INT line_idx, DIF_VDEC_PARAM *vdec_param, INT *len)
{
	INT i = 0, min_compress_rate = 0, fps = 0, max_counts = 0, aver_count = 0;
	UINT32 bitrate = 0, win_size = 0, aver_size = 0;
	pif_group_t *group;
	vpd_entity_t *din = NULL;
	vpd_din_entity_t *din_entity;
	INT din_fd, vdec_vch, vdec_path, dec_qp;
	HD_OUT_POOL *p_out_pool;
	HD_DIM dim = {0, 0};
	VDODEC_MAXMEM *p_vdec_maxmem;
	HD_PATH_ID path_id = 0;

	if (len == NULL) {
		GMLIB_ERROR_P("NULL len\n");
		return 0;
	} else {
		*len = 0;
	}
	if (hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (vdec_param == NULL) {
		GMLIB_ERROR_P("NULL vdec_param\n");
		goto err_exit;
	}

	/* get group */
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	/* get vch, path and fd */
	vdec_vch = vdec_param->dev.device_subid;
	vdec_path = vdec_param->dev.out_subid;
	path_id = dif_get_path_id(&vdec_param->dev);
	din_fd = utl_get_datain_entity_fd(path_id);
	if (din_fd == 0) {
		GMLIB_ERROR_P("<vdec: utl_get_datain_entity_fd fail. line_idx(%d) >\n", line_idx);
		goto err_exit;
	}
	p_out_pool = vdec_param->param->p_out_pool;
	if (p_out_pool == NULL) {
		GMLIB_ERROR_P("<vdec out pool is not set! out_subid(%d) >\n", vdec_path);
		goto err_exit;
	}

	/* get max_mem */
	p_vdec_maxmem = vdec_param->param->max_mem;
	if (p_vdec_maxmem == NULL) {
		GMLIB_ERROR_P("<vdec maxmem param is not set! out_subid(%d) >\n", vdec_path);
		goto err_exit;
	}
	dim.w = ALIGN16_UP(p_vdec_maxmem->max_width);
	dim.h = ALIGN16_UP(p_vdec_maxmem->max_height);

	/* set fps, bitrate, etc */
	fps = p_vdec_maxmem->max_fps;
	if (fps <= 0) {
		fps = 25;
	}
	bitrate = p_vdec_maxmem->max_bitrate;
	if (bitrate == 0) {
		bitrate = 3 * 1024 * 1024; // default: 3 MB
	}

	if (p_vdec_maxmem->codec_type == HD_CODEC_TYPE_JPEG) {
		dec_qp = (vdec_param->param->qp_value != 0) ? vdec_param->param->qp_value : CONFIG_DEFAULT_JPG_QP;
	} else {
		dec_qp = (vdec_param->param->qp_value != 0) ? vdec_param->param->qp_value : CONFIG_DEFAULT_QP;
	}

	GMLIB_L2_P("set_datain_unit: codec_type(%lu), dec_qp(%lu), bitrate(%lu), fps(%lu)\n",
		   p_vdec_maxmem->codec_type, vdec_param->param->qp_value, bitrate, fps);

	min_compress_rate = get_bs_ratio(p_vdec_maxmem->codec_type, dec_qp); //fixme for api assign value

	/* calc buf size */
	if (p_vdec_maxmem->max_bs_size == 0) {
		win_size = (common_calculate_buf_size_by_HD_VIDEO_PXLFMT(dim, HD_VIDEO_PXLFMT_YUV420) * min_compress_rate) / 100;
	} else {
		win_size = p_vdec_maxmem->max_bs_size;
	}
	if (win_size < CONFIG_MIN_BS_SIZE)
		win_size = CONFIG_MIN_BS_SIZE;
	aver_size = (bitrate / 8) / fps;

	/* lookup entity buf */
	din = pif_lookup_entity_buf(VPD_DIN_ENTITY_TYPE, din_fd, group->encap_buf);
	if (din != NULL) { /* It have been set in encaps buf, just return this buffer */
		*len = 0;
		goto exit;
	}

	/* set entity buf */
	*len = pif_set_EntityBuf(VPD_DIN_ENTITY_TYPE, &din, group->encap_buf);

	/* set param for vpd */
	din->sn = din_fd;
	din->ap_bindfd = BD_SET_PATH(vdec_param->dev.device_baseid + vdec_param->dev.device_subid,
				     vdec_param->dev.out_subid + HD_OUT_BASE,
				     vdec_param->dev.out_subid + HD_OUT_BASE);
	memset(din->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < DIN_VAR_POOL_NUM; i++) {
		din->pool[i].enable = TRUE;
		din->pool[i].type = VPD_DISP_DEC_IN_POOL;
		snprintf(din->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x",
			 pif_get_pool_name_by_type(din->pool[i].type), din_fd);
		din->pool[i].width = ALIGN16_UP(p_vdec_maxmem->max_width);
		din->pool[i].height = ALIGN16_UP(p_vdec_maxmem->max_height);
		if (p_vdec_maxmem->max_fps == (UINT)GMLIB_NULL_VALUE || !p_vdec_maxmem->max_fps) {
			din->pool[i].fps = 30;
		} else {
			din->pool[i].fps = p_vdec_maxmem->max_fps;
		}

		max_counts = p_vdec_maxmem->bs_counts;
		aver_count = max_counts - BS_WIN_COUNT;

		if (aver_count <= 0) {
			max_counts = BS_WIN_COUNT;
			aver_count = 0;
		}

		din->pool[i].vch = vdec_vch;
		din->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		din->pool[i].is_duplicate = 0;
		din->pool[i].dst_fmt = VPD_BUFTYPE_DATA;
		din->pool[i].ddr_id = p_vdec_maxmem->ddr_id;
		din->pool[i].size = (win_size * BS_WIN_COUNT) + (aver_size * aver_count);
		din->pool[i].win_size = win_size;
		din->pool[i].count_decuple = max_counts * 10;
		din->pool[i].max_counts = max_counts;
		din->pool[i].min_counts = 0;

		GMLIB_L2_P("set_datain_pool: i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) fmt(%#x) dup(%d)\n",
			   i, din->pool[i].enable, din->pool[i].name, din->pool[i].type, din->pool[i].ddr_id, din->pool[i].size,
			   din->pool[i].count_decuple, din->pool[i].max_counts, din->pool[i].dst_fmt, din->pool[i].is_duplicate);
		GMLIB_L2_P("                 win_size(%lu), aver_size(%lu), aver_count(%lu)\n", win_size, aver_size, aver_count);
	}

	/* set param for vpem */
	din_entity = (vpd_din_entity_t *)din->e;
	din_entity->pch = vdec_path;
	din_entity->attr_vch = vdec_vch;
	din_entity->is_audio = 0;
	din_entity->fmt = VPD_BUFTYPE_DATA;
	din_entity->buf_dim.w = ALIGN16_UP(p_vdec_maxmem->max_width);
	din_entity->buf_dim.h = ALIGN16_UP(p_vdec_maxmem->max_height);
	din_entity->src_dim.w = din_entity->buf_dim.w;
	din_entity->src_dim.h = din_entity->buf_dim.h;

	SET_ENTITY_FEATURE(din->feature, VPD_FLAG_OUTBUF_MAX_CNT | VPD_FLAG_STOP_WHEN_APPLY);

	GMLIB_L2_P("set_datain_entity: path_id(%#lx) fd(%#x) pch(%d) dst_bg(%d,%d) dst_fmt(%#x)\n",
		   dif_get_path_id(&vdec_param->dev), din_fd, din_entity->attr_vch, din_entity->buf_dim.w, din_entity->buf_dim.h, din_entity->fmt);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, din);

err_exit:
	return -1;
}

int calculate_pool_size(INT pool_index, HD_VIDEO_CODEC codec_type, HD_DIM dim)
{
	HD_VIDEO_PXLFMT pxlfmt;
	INT pool_size = 0;
	HD_RESULT ret_val;

	// get pxlfmt
	ret_val = dif_get_pxlfmt_by_codec_type(codec_type, &pxlfmt, pool_index);
	if (ret_val != HD_OK) {
		GMLIB_ERROR_P("get pxlfmt fail(%d)\n", ret_val);
		return -1;
	}

	// calc size by pxlfmt
	pool_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(dim, pxlfmt);

	GMLIB_L2_P("%s: idx: %d, codec: %d, dim: %dx%d, pool_size: %d(including yuv, side_info and mb_info)\n",
		   __func__, pool_index, codec_type, dim.w, dim.h, pool_size);

	return pool_size;
}

//caculate h264/h265/jpeg size and choose the larger one
int calculate_max_pool_size(INT pool_index, HD_VIDEO_CODEC codec_type, HD_DIM dim)
{
#if 0
	HD_VIDEO_PXLFMT pxlfmt_h264, pxlfmt_h265;
	INT size_h264 = 0, size_h265 = 0, mb_info_size_h264, mb_info_size_h265;
	HD_RESULT ret_val;

	//h264 size
	ret_val = dif_get_pxlfmt_by_codec_type(HD_CODEC_TYPE_H264, &pxlfmt_h264, pool_index);
	if (ret_val != HD_OK) {
		pxlfmt_h264 = HD_VIDEO_PXLFMT_YUV420;
	}
	mb_info_size_h264 = common_calculate_dec_mb_info_size(&dim, HD_CODEC_TYPE_H264);
	size_h264 = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(dim, pxlfmt_h264) + mb_info_size_h264;

	//h265 size
	ret_val = dif_get_pxlfmt_by_codec_type(HD_CODEC_TYPE_H265, &pxlfmt_h265, pool_index);
	if (ret_val != HD_OK) {
		pxlfmt_h265 = HD_VIDEO_PXLFMT_YUV420_MB2;
	}
	if (pool_index == 0) {
		mb_info_size_h265 = common_calculate_dec_mb_info_size(&dim, HD_CODEC_TYPE_H265);
	} else {
		mb_info_size_h265 = 0;
	}
	size_h265 = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(dim, pxlfmt_h265) + mb_info_size_h265;

	GMLIB_L2_P("calculate_max_pool_size idx(%d) codec(%d) dim(%dx%d) size(%d,%d)\n",
		   pool_index, codec_type, dim.w, dim.h, size_h264, size_h265);

	return ((size_h264 > size_h265) ? size_h264 : size_h265);
#else
	INT size_h264 = 0, size_h265 = 0, size_jpeg = 0;
	INT max_size;

	size_h264 = calculate_pool_size(pool_index, HD_CODEC_TYPE_H264, dim);
	size_h265 = calculate_pool_size(pool_index, HD_CODEC_TYPE_H265, dim);
	size_jpeg = calculate_pool_size(pool_index, HD_CODEC_TYPE_JPEG, dim);

	max_size = size_h264;
	if (max_size < size_h265) {
		max_size = size_h265;
	}
	if (max_size < size_jpeg) {
		max_size = size_jpeg;
	}

	GMLIB_L2_P("calculate_max_pool_size idx(%d) codec(%d) dim(%dx%d) size(%d,%d,%d)\n",
		   pool_index, codec_type, dim.w, dim.h, size_h264, size_h265, size_jpeg);

	return max_size;
#endif
}

INT set_vdec_unit(HD_GROUP *hd_group, INT line_idx, DIF_VDEC_PARAM *vdec_param, INT *len)
{
	INT i, size = 0, size2, dec_qp;
	UINT32 min_compress_rate, win_size = 0, yuv_idx = 0;
	pif_group_t *group;
	vpd_entity_t *dec = NULL;
	vpd_dec_entity_t *dec_entity;
	INT vdec_fd, vdec_vch, vdec_path;
	HD_OUT_POOL *p_out_pool;
	HD_DIM dim = {0, 0};
	HD_DIM sub_dim = {0, 0};
	HD_DIM main_align = {0, 0};
	HD_DIM sub_align = {0, 0};
	VDODEC_MAXMEM *p_vdec_maxmem;
	HD_VIDEO_PXLFMT pxlfmt, pxlfmt2;

	if (len == NULL) {
		GMLIB_ERROR_P("NULL len\n");
		return 0;
	} else {
		*len = 0;
	}
	if (hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (vdec_param == NULL) {
		GMLIB_ERROR_P("NULL vdec_param\n");
		goto err_exit;
	}

	/* get group */
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	/* get vch, path and fd */
	vdec_vch = vdec_param->dev.device_subid;
	vdec_path = vdec_param->dev.out_subid;
	vdec_fd = utl_get_dec_entity_fd(vdec_vch, vdec_path, NULL);
	p_out_pool = vdec_param->param->p_out_pool;
	if (p_out_pool == NULL) {
		GMLIB_ERROR_P("<vdec out pool is not set! out_subid(%d) >\n", vdec_path);
		goto err_exit;
	}

	/* get max_mem */
	p_vdec_maxmem = vdec_param->param->max_mem;
	if (p_vdec_maxmem == NULL) {
		GMLIB_ERROR_P("<vdec maxmem param is not set! out_subid(%d) >\n", vdec_path);
		goto err_exit;
	}

	/* get dim */
	get_align_by_codec(p_vdec_maxmem->codec_type, &main_align, &sub_align);
	dim.w = ALIGN_CEIL(p_vdec_maxmem->max_width, main_align.w);
	dim.h = ALIGN_CEIL(p_vdec_maxmem->max_height, main_align.h);
	if (_get_sub_yuv_dim(dim, sub_align, vdec_param->param->sub_yuv_ratio, &sub_dim) != HD_OK) {
		GMLIB_ERROR_P("get sub yuv dim fail\n");
		goto err_exit;
	}

	/* get pxlfmt */
	yuv_idx = 0;
	if (dif_get_pxlfmt_by_codec_type(p_vdec_maxmem->codec_type, &pxlfmt, yuv_idx) != HD_OK) { // main_yuv
		GMLIB_ERROR_P("<get pxlfmt fail >\n");
		goto err_exit;
	}
	yuv_idx = 1;
	if (dif_get_pxlfmt_by_codec_type(p_vdec_maxmem->codec_type, &pxlfmt2, yuv_idx) != HD_OK) { // sub_yuv
		GMLIB_ERROR_P("<get pxlfmt2 fail >\n");
		goto err_exit;
	}

	if (1) {
		size = calculate_max_pool_size(0, p_vdec_maxmem->codec_type, dim);
		size2 = calculate_max_pool_size(1, p_vdec_maxmem->codec_type, sub_dim);
	} else {
		INT mb_info_size = common_calculate_dec_mb_info_size(&dim, p_vdec_maxmem->codec_type);
		size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(dim, pxlfmt) + mb_info_size;
		size2 = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(sub_dim, pxlfmt2);
	}

	if (p_vdec_maxmem->codec_type == HD_CODEC_TYPE_JPEG) {
		dec_qp = (vdec_param->param->qp_value != 0) ? vdec_param->param->qp_value : CONFIG_DEFAULT_JPG_QP;
	} else {
		dec_qp = (vdec_param->param->qp_value != 0) ? vdec_param->param->qp_value : CONFIG_DEFAULT_QP;
	}

	GMLIB_L2_P("set_vdec_unit: codec_type(%lu), dec_qp(%lu)\n",
		   p_vdec_maxmem->codec_type, vdec_param->param->qp_value);

	min_compress_rate = get_bs_ratio(p_vdec_maxmem->codec_type, dec_qp); //fixme for api assign value
	if (p_vdec_maxmem->max_bs_size == 0) {
		win_size = (common_calculate_buf_size_by_HD_VIDEO_PXLFMT(dim, HD_VIDEO_PXLFMT_YUV420) * min_compress_rate) / 100;
	} else {
		win_size = p_vdec_maxmem->max_bs_size;
	}
	if (win_size < CONFIG_MIN_BS_SIZE)
		win_size = CONFIG_MIN_BS_SIZE;

	/* lookup entity buf */
	dec = pif_lookup_entity_buf(VPD_DEC_ENTITY_TYPE, vdec_fd, group->encap_buf);
	if (dec != NULL) { /* It have been set in encaps buf, just return this buffer */
		*len = 0;
		goto exit;
	}

	/* set entity buf */
	*len = pif_set_EntityBuf(VPD_DEC_ENTITY_TYPE, &dec, group->encap_buf);

	GMLIB_L2_P("set_vdec_pool dim(%dx%d) sub_dim(%dx%d) size(%d) size2(%d)\n",
		   dim.w, dim.h, sub_dim.w, sub_dim.h, size, size2);

	/* set param for vpd */
	dec->sn = vdec_fd;
	dec->ap_bindfd = BD_SET_PATH(vdec_param->dev.device_baseid + vdec_param->dev.device_subid,
				     vdec_param->dev.out_subid + HD_OUT_BASE,
				     vdec_param->dev.out_subid + HD_OUT_BASE);
	memset(dec->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < VPD_MAX_BUFFER_NUM; i++) {
		if (p_out_pool->buf_info[i].enable == HD_BUF_DISABLE) {
			dec->pool[i].enable = FALSE;
			continue;
		} else if (p_out_pool->buf_info[i].enable == HD_BUF_ENABLE) {
			dec->pool[i].enable = TRUE;
			dec->pool[i].ddr_id = p_out_pool->buf_info[i].ddr_id;
			dec->pool[i].count_decuple = p_out_pool->buf_info[i].counts;
			dec->pool[i].max_counts = _get_max_counts(p_out_pool->buf_info[i].counts, p_vdec_maxmem->max_ref_num);
			if (dec->pool[i].max_counts < DEFAULT_MAX_COUNT) {
				GMLIB_ERROR_P("max_counts(< %u) is not enough! i(%d), counts(%d), max_ref_num(%d) => max_counts(%u)\n",
					      DEFAULT_MAX_COUNT, i,
					      p_out_pool->buf_info[i].counts,
					      p_vdec_maxmem->max_ref_num,
					      dec->pool[i].max_counts);
				goto err_exit;
			}
			dec->pool[i].min_counts = DEFAULT_MIN_COUNT;
		} else { //HD_BUF_AUTO
			if (i == 0) {
				dec->pool[i].enable = TRUE;
				dec->pool[i].ddr_id = 0;
				dec->pool[i].count_decuple = 15;
				dec->pool[i].max_counts = DEFAULT_MAX_COUNT;
				dec->pool[i].min_counts = DEFAULT_MIN_COUNT;
			} else if (i == 1) {
				dec->pool[i].enable = TRUE;
				dec->pool[i].ddr_id = 0;
				dec->pool[i].count_decuple = 15;
				dec->pool[i].max_counts = DEFAULT_MAX_COUNT;
				dec->pool[i].min_counts = DEFAULT_MIN_COUNT;
			} else {
				dec->pool[i].enable = FALSE;
			}
		}

		if (i == 0) {
			dec->pool[i].type = VPD_DISP_DEC_OUT_POOL;
			snprintf(dec->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x",
				 pif_get_pool_name_by_type(dec->pool[i].type), vdec_fd);
			dec->pool[i].size = size;
			dec->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(pxlfmt);
		} else if (i == 1) {
			dec->pool[i].type = VPD_DISP_DEC_OUT_RATIO_POOL;
			snprintf(dec->pool[i].name, GM_POOL_NAME_LEN, "%s_%#x",
				 pif_get_pool_name_by_type(dec->pool[i].type), vdec_fd);
			dec->pool[i].size = size2;
			dec->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(pxlfmt2);
		}

		dec->pool[i].width = ALIGN16_UP(p_vdec_maxmem->max_width);
		dec->pool[i].height = ALIGN16_UP(p_vdec_maxmem->max_height);
		if (p_vdec_maxmem->max_fps == (UINT)GMLIB_NULL_VALUE || !p_vdec_maxmem->max_fps) {
			dec->pool[i].fps = 30;
		} else {
			dec->pool[i].fps = p_vdec_maxmem->max_fps;
		}
		dec->pool[i].vch = vdec_vch;
		dec->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		dec->pool[i].is_duplicate = 0;
		dec->pool[i].win_size = 0;

		GMLIB_L2_P("set_vdec_pool i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) fmt(%#x) dup(%d)\n",
			   i, dec->pool[i].enable, dec->pool[i].name, dec->pool[i].type, dec->pool[i].ddr_id, dec->pool[i].size,
			   dec->pool[i].count_decuple, dec->pool[i].max_counts, dec->pool[i].dst_fmt, dec->pool[i].is_duplicate);
	}

	/* set param for vpem */
	dec_entity = (vpd_dec_entity_t *)dec->e;
	dec_entity->pch = vdec_path;
	dec_entity->dst_bg_dim.w = ALIGN16_UP(p_vdec_maxmem->max_width);
	dec_entity->dst_bg_dim.h = ALIGN16_UP(p_vdec_maxmem->max_height);
	dec_entity->dst_pos.x = dec_entity->dst_pos.y = 0;
	dec_entity->dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(pxlfmt);
	dec_entity->yuv_width_threshold = 0; // 0: any resolution will output sub-yuv
	dec_entity->max_ref_num = p_vdec_maxmem->max_ref_num;
	dec_entity->codec_type = p_vdec_maxmem->codec_type;
	dec_entity->first_out = vdec_param->param->first_out;
	dec_entity->extra_out = vdec_param->param->extra_out;
	if (dec->pool[1].enable) {
		dec_entity->sub_yuv_ratio = vdec_param->param->sub_yuv_ratio;
	} else {
		dec_entity->sub_yuv_ratio = 0;
	}
	dec_entity->attr_vch = vdec_vch;

	/* update vdec convert config */
	vdec_convert_config[vdec_path].sub_yuv_ratio = dec_entity->sub_yuv_ratio;

	SET_ENTITY_FEATURE(dec->feature, VPD_FLAG_OUTBUF_MAX_CNT); //Need prevent datain->dec flooding

	GMLIB_L2_P("set_vdec_entity path_id(%#lx) fd(%#x) pch(%d) dst_xy(%d,%d) dst_bg(%d,%d) dst_fmt(%#x)\n",
		   dif_get_path_id(&vdec_param->dev), vdec_fd, dec_entity->pch,
		   dec_entity->dst_pos.x, dec_entity->dst_pos.y,
		   dec_entity->dst_bg_dim.w, dec_entity->dst_bg_dim.h,
		   dec_entity->dst_fmt);
	GMLIB_L2_P("                yuv_width_threshold(%u), max_ref_num(%u), codec_type(%u), sub_yuv_ratio(%u)\n",
		   dec_entity->yuv_width_threshold, dec_entity->max_ref_num,
		   dec_entity->codec_type, dec_entity->sub_yuv_ratio);
	GMLIB_L2_P("                first_out(%u), extra_out(%u)\n",
		   dec_entity->first_out, dec_entity->extra_out);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, dec);

err_exit:
	return -1;
}

HD_RESULT set_vdec_sub_yuv_setting(DIF_VDEC_PARAM *vdec_param, vpd_dec_entity_t *dec_entity)
{
	if (vdec_param == NULL) {
		GMLIB_ERROR_P("NULL vdec_param\n");
		goto err_exit;
	}
	if (vdec_param->param == NULL) {
		GMLIB_ERROR_P("NULL vdec_param\n");
		goto err_exit;
	}
	if (dec_entity == NULL) {
		GMLIB_ERROR_P("NULL dec_entity\n");
		goto err_exit;
	}

	dec_entity->sub_yuv_ratio = vdec_param->param->sub_yuv_ratio;

	GMLIB_L2_P("%s: path_id(%#lx) sub_yuv_ratio(%u)\n",
		   __func__, dif_get_path_id(&vdec_param->dev), dec_entity->sub_yuv_ratio);

	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

VDEC_CVT_CONFIG *get_vdec_convert_config(HD_PATH_ID path_id)
{
	HD_RESULT ret = HD_OK;
	INT vdec_path;

	ret = videodec_check_path_id_p(path_id);
	if (ret != HD_OK) {
		GMLIB_ERROR_P("check path_id(0x%lx) fail\n", path_id);
		return NULL;
	}

	vdec_path = VDODEC_OUTID(HD_GET_OUT(path_id));

	return &vdec_convert_config[vdec_path];
}

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
