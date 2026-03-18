/**
 * @file videoenc.c
 * @brief type definition of API.
 * @author PSW
 * @date in the year 2018
 */

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "hdal.h"
#include "vpd_ioctl.h"
#include "hdal.h"
#include "vg_common.h"
#include "videoenc.h"
#include "cif_common.h"
#include "dif.h"
#include "pif.h"
#include "kflow_videoenc_h26x.h"
#include "osg_ioctl.h"
#include "pif_ioctl.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define GET_DEFAULT_PARAM_FROM_DRIVER  1

#define VIDEOENC_MEM_POOL_NAME    "enc_cap_out_ddr0"
#define MIN_COMPRESSED_RATIO      20
#define ENCODER_RAW_MIN_WIDTH     64
#define ENCODER_RAW_MIN_HEIGHT    64
#define ENCODER_RAW_MAX_WIDTH     3840
#define ENCODER_RAW_MAX_HEIGHT    2176
#define ENCODER_MAX_OSG_NU	      10
extern int osg_ioctl;
/*-----------------------------------------------------------------------------*/
/* External Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern VDOENC_INFO videoenc_info[MAX_VDOENC_DEV];
extern int vpd_fd;
extern vpd_sys_info_t platform_sys_Info;

/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them
/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
VDOENC_INTERNAL_PARAM vdoenc_dev_param[MAX_VDOENC_DEV];
KFLOW_VIDEOENC_HW_SPEC_INFO vdoenc_hw_spec_info;

//FIXME: need to put these functions' declaration somewhere
extern int pif_alloc_user_fd_for_hdal(vpd_em_type_t type, int chip, int engine, int minor);
extern void *pif_alloc_video_buffer_for_hdal(int size, int ddr_id, char pool_name[], int alloc_tag);
extern int pif_free_video_buffer_for_hdal(void *pa, int ddr_id, int alloc_tag);
extern uintptr_t pif_create_queue_for_hdal(void);
extern int pif_destroy_queue_for_hdal(uintptr_t queue_handle);
extern void fill_h26x_param_from_hdal(h26x_param_set  *p_hdal_params, hdal_videoenc_setting_t *h26x_enc_setting, HD_VIDEO_PXLFMT pxl_fmt, dim_t src_dim);
extern HD_FRAME_TYPE get_hdal_frame_type(unsigned int frame_type);
extern HD_SVC_LAYER_TYPE get_hdal_svc_layer_type(unsigned int svc_layer_type);
extern UINT32 utl_get_enc_osg_index(HD_DAL device_subid);
extern int pif_get_left_frame_count(unsigned int fd, unsigned int *p_left_count, unsigned int *p_done_count, unsigned int *p_dti_buf_cnt);
#define MAX_OSG_LOAD_NUM 32
static INT enc_ioctl = -1;

#if !GET_DEFAULT_PARAM_FROM_DRIVER
static INT16 default_AQ_mapping_table[HD_H26XENC_AQ_MAP_TABLE_NUM] = {
	-120, -112, -104, -96, -88, -80, -72, -64, -56, -48, -40, -32, -24, -16, -8,
		7,   15,   23,  31,  39,  47,  55,  63,  71,  79,  87,  95, 103, 111, 119
	};
#endif

UINT16 osg_select[MAX_VDOENC_DEV][HD_VIDEOENC_MAX_OUT] = { {0},  {0} } ;

HD_VIDEO_PXLFMT osg_type_list[VDOENC_OSG_TYPE_NU] = {
	HD_VIDEO_PXLFMT_ARGB1555,
	HD_VIDEO_PXLFMT_ARGB8888,
	HD_VIDEO_PXLFMT_ARGB4444,
	HD_VIDEO_PXLFMT_RGB565,
	HD_VIDEO_PXLFMT_I1,
	HD_VIDEO_PXLFMT_I2,
	HD_VIDEO_PXLFMT_I4
};

char* get_format_name(vpd_buf_type_t vpd_buf_type)
{
	switch(vpd_buf_type) {
	case VPD_BUFTYPE_YUV422:
		return "HD_VIDEO_PXLFMT_YUV422_ONE (YUV422,UYVY)";
	case VPD_BUFTYPE_YUV422_SCE:
		return "HD_VIDEO_PXLFMT_YUV422_NVX3 (YUV422_SCE)";
	case VPD_BUFTYPE_YUV422_MB:
		return "HD_VIDEO_PXLFMT_YUV422_MB               ";
	case VPD_BUFTYPE_YUV420_SP:
		return "HD_VIDEO_PXLFMT_YUV420                  ";
	case VPD_BUFTYPE_YUV420_SCE:
		return "HD_VIDEO_PXLFMT_YUV420_NVX3 (YUV420_SCE)";
	case VPD_BUFTYPE_YUV420_MB:
		return "HD_VIDEO_PXLFMT_YUV420_MB               ";
	case VPD_BUFTYPE_YUV420_16x2:
		return "HD_VIDEO_PXLFMT_YUV420_MB2 (YUV420_16x2)";
	case VPD_BUFTYPE_YUV420_SP8:
		return "HD_VIDEO_PXLFMT_YUV420_W8 (YUV420_SP8)  ";
	case VPD_BUFTYPE_ARGB1555:
		return "HD_VIDEO_PXLFMT_ARGB1555                ";
	case VPD_BUFTYPE_ARGB8888:
		return "HD_VIDEO_PXLFMT_ARGB8888                ";
	case VPD_BUFTYPE_RGB565:
		return "HD_VIDEO_PXLFMT_RGB565                  ";
	case VPD_BUFTYPE_RGB_CV:
		return "HD_VIDEO_PXLFMT_RGB888_PLANAR (RGB_CV)  ";
	case VPD_BUFTYPE_MD:
		return "MD                                      ";
	case VPD_BUFTYPE_DATA:
		return "DATA                                    ";
	default:
		return "Unknown";
	}
}

char* get_codec_name(HD_VIDEO_CODEC codec_type)
{
	switch(codec_type) {
	case HD_CODEC_TYPE_JPEG:
		return "jpeg";
	case HD_CODEC_TYPE_H264:
		return "h264";
	case HD_CODEC_TYPE_H265:
		return "h265";
	case HD_CODEC_TYPE_RAW:
		return "raw";
	default:
		return "Unknown";
	}
}

char* get_osg_type_name(HD_VIDEO_PXLFMT osg_type)
{
	switch(osg_type) {
		case HD_VIDEO_PXLFMT_ARGB1555:
			return "argb1555";
		case HD_VIDEO_PXLFMT_ARGB8888:
			return "argb8888";
		case HD_VIDEO_PXLFMT_ARGB4444:
			return "argb4444";
		case HD_VIDEO_PXLFMT_RGB565:
			return "rgb565";
		case HD_VIDEO_PXLFMT_I1:
			return "1bit_pal";
		case HD_VIDEO_PXLFMT_I2:
			return "2bit_pal";
		case HD_VIDEO_PXLFMT_I4:
			return "4bit_pal";
		default:
			return "unknown";
	}
}

uint8_t get_internal_osg_type_idx(HD_VIDEO_PXLFMT osg_fmt)
{
	switch(osg_fmt) {
		case HD_VIDEO_PXLFMT_ARGB1555:
			return 0;
		case HD_VIDEO_PXLFMT_ARGB8888:
			return 1;
		case HD_VIDEO_PXLFMT_ARGB4444:
			return 2;
		case HD_VIDEO_PXLFMT_RGB565:
			return 3;
		case HD_VIDEO_PXLFMT_I1:
			return 4;
		case HD_VIDEO_PXLFMT_I2:
			return 5;
		case HD_VIDEO_PXLFMT_I4:
			return 6;
		default:
			return 255;
	}
}

uint8_t get_external_osg_type_idx(HD_VIDEO_PXLFMT osg_fmt)
{
	switch (osg_fmt) {
	case HD_VIDEO_PXLFMT_ARGB1555:
		return 0;
	case HD_VIDEO_PXLFMT_ARGB8888:
		return 1;
	case HD_VIDEO_PXLFMT_I1:
		return 2;
	case HD_VIDEO_PXLFMT_I2:
		return 3;
	case HD_VIDEO_PXLFMT_I4:
		return 4;
	default :
		return 255;
	}
}

static INT estimate_max_bs_len(HD_VIDEO_CODEC codec_type, HD_DIM dim, INT is_page_align)
{
	INT max_bs_len = 0;
	switch (codec_type) {
	case HD_CODEC_TYPE_JPEG:
	case HD_CODEC_TYPE_H264:
	case HD_CODEC_TYPE_H265:
		max_bs_len = ((dim.w * dim.h * MIN_COMPRESSED_RATIO * 3) / (2 * 100));
		break;
	case HD_CODEC_TYPE_RAW:
		max_bs_len = (dim.w * dim.h);
		break;
	default:
		break;
	}
	if (is_page_align) {
		int page_size = getpagesize();
		max_bs_len = ((max_bs_len + (page_size - 1)) / page_size) * page_size;
		return max_bs_len;
	} else {
		return max_bs_len;
	}
}

static usr_enc_type_t convert_HD_VIDEO_CODEC_to_usr_enc_type(HD_VIDEO_CODEC type)
{
	switch (type) {
	case HD_CODEC_TYPE_JPEG:
		return VPUSR_ENC_TYPE_JPEG;
	case HD_CODEC_TYPE_H264:
		return VPUSR_ENC_TYPE_H264;
	case HD_CODEC_TYPE_H265:
		return VPUSR_ENC_TYPE_H265;
	default: {
		HD_VIDEOENC_ERR("%s, Not support for this type mapping(%#x)\n", __func__, type);
		return VPUSR_ENC_TYPE_H264;
	}
	}
}

INT verify_param_HD_VIDEOENC_IN_STAMP(HD_VIDEOENC_IN *p_enc_in, HD_VDOENC_STAMP_PARAM *p_enc_img, CHAR *p_ret_string, INT max_str_len)
{
	if ((p_enc_img->img.dim.w != 0) && ((p_enc_img->attr.position.x + p_enc_img->img.dim.w) > p_enc_in->dim.w)) {
		snprintf(p_ret_string, max_str_len,
			 "HD_VIDEOENC_IN_STAMP_IMG: stamp.x(%u)+stamp.w(%u) > enc_in.w(%u)\n",
			 p_enc_img->attr.position.x, p_enc_img->img.dim.w, p_enc_in->dim.w);
		goto exit;
	}

	if ((p_enc_img->img.dim.h != 0) && ((p_enc_img->attr.position.y + p_enc_img->img.dim.h) > p_enc_in->dim.h)) {
		snprintf(p_ret_string, max_str_len,
			 "HD_VIDEOENC_IN_STAMP_IMG: stamp.y(%u)+stamp.h(%u) > enc_in.h(%u)\n",
			 p_enc_img->attr.position.y, p_enc_img->img.dim.h, p_enc_in->dim.h);
		goto exit;
	}

	return 0;
exit:
	return -1;
}

static INT validate_enc_in_dim(UINT32 enc_w, UINT32 enc_h, HD_VIDEO_CODEC  codec_type)
{
	if (codec_type == HD_CODEC_TYPE_H264) {
		if (enc_w < vdoenc_hw_spec_info.h264.min_dim.w || enc_w > vdoenc_hw_spec_info.h264.max_dim.w ||
		    enc_h < vdoenc_hw_spec_info.h264.min_dim.h || enc_h > vdoenc_hw_spec_info.h264.max_dim.h) {
			goto fail_exit;
		}
	} else if (codec_type == HD_CODEC_TYPE_H265) {
		if (enc_w < vdoenc_hw_spec_info.h265.min_dim.w || enc_w > vdoenc_hw_spec_info.h265.max_dim.w ||
		    enc_h < vdoenc_hw_spec_info.h265.min_dim.h || enc_h > vdoenc_hw_spec_info.h265.max_dim.h) {
			goto fail_exit;
		}
	} else if (codec_type == HD_CODEC_TYPE_JPEG) {
		if (enc_w < vdoenc_hw_spec_info.jpeg.min_dim.w || enc_w > vdoenc_hw_spec_info.jpeg.max_dim.w ||
		    enc_h < vdoenc_hw_spec_info.jpeg.min_dim.h || enc_h > vdoenc_hw_spec_info.jpeg.max_dim.h) {
			goto fail_exit;
		}
	}
	return TRUE;

fail_exit:
	return FALSE;
}

static INT validate_enc_in_pxl_fmt(HD_VIDEO_PXLFMT in_pxl_fmt, HD_VIDEO_CODEC  codec_type)
{
	vpd_buf_type_t vpd_buf_type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(in_pxl_fmt);

	if (codec_type == HD_CODEC_TYPE_H264 || codec_type == HD_CODEC_TYPE_H265) {
		if (!BUF_TYPE_IS_SUPPORT(vdoenc_hw_spec_info.h26x.type_support_bit_array, vpd_buf_type)) {
			goto fail_exit;
		}
	} else if (codec_type == HD_CODEC_TYPE_JPEG) {
		if (!BUF_TYPE_IS_SUPPORT(vdoenc_hw_spec_info.jpeg.type_support_bit_array, vpd_buf_type)) {
			goto fail_exit;
		}
	}
	return TRUE;

fail_exit:
	return FALSE;
}

static INT validate_enc_in_dim_alignment(UINT32 enc_w, UINT32 enc_h, HD_VIDEO_PXLFMT pxl_fmt, HD_VIDEO_CODEC  codec_type,
		align_level_dim_t  *p_align_level_val)
{
	align_level_dim_t  align_level;

	if (codec_type == HD_CODEC_TYPE_H264) {
		align_level = (pxl_fmt == HD_VIDEO_PXLFMT_YUV420) ?
			      vdoenc_hw_spec_info.h264.align : vdoenc_hw_spec_info.h264.align_compress;

	} else if (codec_type == HD_CODEC_TYPE_H265) {
		align_level = (pxl_fmt == HD_VIDEO_PXLFMT_YUV420) ?
			      vdoenc_hw_spec_info.h265.align : vdoenc_hw_spec_info.h265.align_compress;

	} else if (codec_type == HD_CODEC_TYPE_JPEG) {
		align_level = vdoenc_hw_spec_info.jpeg.align;

	} else {
		align_level.w = 1;
		align_level.h = 1;
	}
	if (p_align_level_val) {
		*p_align_level_val = align_level;
	}

	if (IS_ALIGNX(align_level.w, enc_w) == 0 || IS_ALIGNX(align_level.h, enc_h) == 0) {
		return FALSE;
	}
	return TRUE;
}

INT verify_param_HD_VIDEOENC_IN(HD_VIDEOENC_IN *p_enc_in, HD_VIDEOENC_OUT *p_enc_out, CHAR *p_ret_string, INT max_str_len)
{
	UINT32 enc_w;
	UINT32 enc_h;
	align_level_dim_t  align_level_val;

	if (HD_VIDEO_DIR_ROTATE_90 == p_enc_in->dir ||
	    HD_VIDEO_DIR_ROTATE_270 == p_enc_in->dir) {
		enc_w = p_enc_in->dim.h;
		enc_h = p_enc_in->dim.w;
	} else {
		enc_w = p_enc_in->dim.w;
		enc_h = p_enc_in->dim.h;
	}

	if (validate_enc_in_dim(enc_w, enc_h, p_enc_out->codec_type) == FALSE) {
		snprintf(p_ret_string, max_str_len,
			 "HD_VIDEOENC_IN: codec_type(%d) dim(%ux%u) is out-of-range\n",
			 p_enc_out->codec_type, enc_w, enc_h);
		goto exit;
	}
	if (validate_enc_in_pxl_fmt(p_enc_in->pxl_fmt, p_enc_out->codec_type) == FALSE) {
		vpd_buf_type_t vpd_buf_type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_enc_in->pxl_fmt);
		snprintf(p_ret_string, max_str_len, "HD_VIDEOENC_IN: codec_type(%s) doesn't support fmt %s",
			 get_codec_name(p_enc_out->codec_type), get_format_name(vpd_buf_type));
		goto exit;
	}
	if (validate_enc_in_dim_alignment(enc_w, enc_h, p_enc_in->pxl_fmt, p_enc_out->codec_type, &align_level_val) == FALSE) {
		snprintf(p_ret_string, max_str_len,
			 "HD_VIDEOENC_IN: codec_type(%d) dim(%ux%u) doesn't conform alignment(w:%u, h:%u)\n",
			 p_enc_out->codec_type, enc_w, enc_h, align_level_val.w, align_level_val.h);
		goto exit;
	}


	return 0;
exit:
	return -1;
}

static INT verify_param_HD_VIDEOENC_OUT(HD_VIDEOENC_OUT *p_enc_out, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_out->codec_type != HD_CODEC_TYPE_JPEG && p_enc_out->codec_type != HD_CODEC_TYPE_H264 &&
	    p_enc_out->codec_type != HD_CODEC_TYPE_H265 && p_enc_out->codec_type != HD_CODEC_TYPE_RAW) {
		snprintf(p_ret_string, max_str_len,
			 "HD_VIDEOENC_OUT: codec_type(%d) is not supported.", p_enc_out->codec_type);
		goto exit;
	}
	if (p_enc_out->codec_type == HD_CODEC_TYPE_JPEG) {
		HD_JPEG_CONFIG  *p_jpeg = &p_enc_out->jpeg;
		/*if (p_jpeg->retstart_interval < 0) {
			snprintf(p_ret_string, max_str_len,
					 "HD_JPEG_CONFIG: retstart_interval(%u) is out-of-range(0~image_size/256).",
					 p_jpeg->retstart_interval);
			goto exit;
		}*/
		if (p_jpeg->image_quality < 1 || p_jpeg->image_quality > 100) {
			snprintf(p_ret_string, max_str_len,
				 "HD_JPEG_CONFIG: image_quality(%d) is out-of-range(1~100).", p_jpeg->image_quality);
			goto exit;
		}
	} else if (p_enc_out->codec_type == HD_CODEC_TYPE_RAW) {
		//
	} else {
		HD_H26X_CONFIG  *p_h26x = &p_enc_out->h26x;
		if (p_h26x->gop_num > 4096) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26X_CONFIG: gop_num(%u) is out-of-range(0~4096).", p_h26x->gop_num);
			goto exit;
		}
		if (p_h26x->chrm_qp_idx < -12 || p_h26x->chrm_qp_idx > 12) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26X_CONFIG: chrm_qp_idx(%d) is out-of-range(-12~12).", p_h26x->chrm_qp_idx);
			goto exit;
		}
		if (p_h26x->sec_chrm_qp_idx < -12 || p_h26x->sec_chrm_qp_idx > 12) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26X_CONFIG: sec_chrm_qp_idx(%d) is out-of-range(-12~12).", p_h26x->sec_chrm_qp_idx);
			goto exit;
		}
		if (p_h26x->ltr_interval > 4095) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26X_CONFIG: ltr_interval(%u) is out-of-range(0~4095).", p_h26x->ltr_interval);
			goto exit;
		}
		if (p_h26x->ltr_interval != 0) {
			if (p_h26x->ltr_pre_ref != 0 && p_h26x->ltr_pre_ref != 1) {
				snprintf(p_ret_string, max_str_len,
					 "HD_H26X_CONFIG: ltr_pre_ref(%d) is out-of-range(0~1).", p_h26x->ltr_pre_ref);
				goto exit;
			}
		}
		if (p_h26x->gray_en != 0 && p_h26x->gray_en != 1) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26X_CONFIG: gray_en(%d) is out-of-range(0~1).", p_h26x->gray_en);
			goto exit;
		}
		if (p_h26x->source_output != 0 && p_h26x->source_output != 1) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26X_CONFIG: source_output(%d) is out-of-range(0~1).", p_h26x->source_output);
			goto exit;
		}

		if (p_enc_out->codec_type == HD_CODEC_TYPE_H264) {
			switch (p_h26x->profile) {
			case HD_H264E_BASELINE_PROFILE:
			case HD_H264E_MAIN_PROFILE:
			case HD_H264E_HIGH_PROFILE:
				break;
			default:
				snprintf(p_ret_string, max_str_len,
					 "HD_VIDEOENC_PROFILE: when H264, h26x.profile(%d) is not support.", p_h26x->profile);
				goto exit;
			}

			switch (p_h26x->level_idc) {
			case HD_H264E_LEVEL_1:
			case HD_H264E_LEVEL_1_1:
			case HD_H264E_LEVEL_1_2:
			case HD_H264E_LEVEL_1_3:
			case HD_H264E_LEVEL_2:
			case HD_H264E_LEVEL_2_1:
			case HD_H264E_LEVEL_2_2:
			case HD_H264E_LEVEL_3:
			case HD_H264E_LEVEL_3_1:
			case HD_H264E_LEVEL_3_2:
			case HD_H264E_LEVEL_4:
			case HD_H264E_LEVEL_4_1:
			case HD_H264E_LEVEL_4_2:
			case HD_H264E_LEVEL_5:
			case HD_H264E_LEVEL_5_1:
				break;
			default:
				snprintf(p_ret_string, max_str_len,
					 "HD_VIDEOENC_LEVEL: when H264, h26x.level_idc(%d) is not support.", p_h26x->level_idc);
				goto exit;
			}

			switch (p_h26x->entropy_mode) {
			case HD_H264E_CAVLC_CODING:
			case HD_H264E_CABAC_CODING:
				break;
			default:
				snprintf(p_ret_string, max_str_len,
					 "HD_VIDEOENC_CODING: when H264, h26x.entropy_mode(%d) is not support.", p_h26x->entropy_mode);
				goto exit;
			}

		} else if (p_enc_out->codec_type == HD_CODEC_TYPE_H265) {
			switch (p_h26x->profile) {
			case HD_H265E_MAIN_PROFILE:
				break;
			default:
				snprintf(p_ret_string, max_str_len,
					 "HD_VIDEOENC_PROFILE: when H265, h26x.profile(%d) is not support.", p_h26x->profile);
				goto exit;
			}

			switch (p_h26x->level_idc) {
			case HD_H265E_LEVEL_1:
			case HD_H265E_LEVEL_2:
			case HD_H265E_LEVEL_2_1:
			case HD_H265E_LEVEL_3:
			case HD_H265E_LEVEL_3_1:
			case HD_H265E_LEVEL_4:
			case HD_H265E_LEVEL_4_1:
			case HD_H265E_LEVEL_5:
			case HD_H265E_LEVEL_5_1:
			case HD_H265E_LEVEL_5_2:
			case HD_H265E_LEVEL_6:
			case HD_H265E_LEVEL_6_1:
			case HD_H265E_LEVEL_6_2:
				break;
			default:
				snprintf(p_ret_string, max_str_len,
					 "HD_VIDEOENC_LEVEL: when H265, h26x.level_idc(%d) is not support.", p_h26x->level_idc);
				goto exit;
			}

			switch (p_h26x->entropy_mode) {
			case HD_H265E_CABAC_CODING:
				break;
			default:
				snprintf(p_ret_string, max_str_len,
					 "HD_VIDEOENC_CODING: when H265, h26x.entropy_mode(%d) is not support.", p_h26x->entropy_mode);
				goto exit;
			}
		}

		if (p_h26x->svc_layer >= HD_SVC_MAX) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26X_CONFIG: svc_layer(%d) is out-of-range(0~%d).", p_h26x->svc_layer, HD_SVC_MAX);
			goto exit;
		}

	}
	return 0;
exit:
	return -1;
}

static INT verify_param_HD_H26XENC_VUI(HD_H26XENC_VUI *p_enc_vui, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_vui->vui_en != 0 && p_enc_vui->vui_en != 1) {
		snprintf(p_ret_string, max_str_len,
			 "HD_H26XENC_VUI: vui_en(%d) is out-of-range(0~1).", p_enc_vui->vui_en);
		goto exit;
	}
	if (p_enc_vui->vui_en == 1) {
		if (p_enc_vui->sar_width > 65535) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VUI: sar_width(%u) is out-of-range(0~65535).", p_enc_vui->sar_width);
			goto exit;
		}
		if (p_enc_vui->sar_height > 65535) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VUI: sar_height(%u) is out-of-range(0~65535).", p_enc_vui->sar_height);
			goto exit;
		}
		if (p_enc_vui->matrix_coef > 255) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VUI: matrix_coef(%d) is out-of-range(0~255).", p_enc_vui->matrix_coef);
			goto exit;
		}
		if (p_enc_vui->transfer_characteristics > 255) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VUI: transfer_characteristics(%d) is out-of-range(0~255).", p_enc_vui->transfer_characteristics);
			goto exit;
		}
		if (p_enc_vui->colour_primaries > 255) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VUI: colour_primaries(%d) is out-of-range(0~255).", p_enc_vui->colour_primaries);
			goto exit;
		}
		if (p_enc_vui->video_format > 7) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VUI: video_format(%d) is out-of-range(0~7).", p_enc_vui->video_format);
			goto exit;
		}
		if (p_enc_vui->color_range > 1) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VUI: color_range(%d) is out-of-range(0~1).", p_enc_vui->color_range);
			goto exit;
		}
		if (p_enc_vui->timing_present_flag != 0 && p_enc_vui->timing_present_flag != 1) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VUI: timing_present_flag(%d) is out-of-range(0~1).", p_enc_vui->timing_present_flag);
			goto exit;
		}
	}
	return 0;
exit:
	return -1;
}

static INT verify_param_HD_H26XENC_DEBLOCK(HD_H26XENC_DEBLOCK *p_enc_deblock, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_deblock->dis_ilf_idc > 2) {
		snprintf(p_ret_string, max_str_len,
			 "HD_H26XENC_DEBLOCK: dis_ilf_idc(%d) is out-of-range(0~2).", p_enc_deblock->dis_ilf_idc);
		goto exit;
	}
	if (p_enc_deblock->db_alpha < -12 || p_enc_deblock->db_alpha > 12) {
		snprintf(p_ret_string, max_str_len,
			 "HD_H26XENC_DEBLOCK: db_alpha(%d) is out-of-range(-12~12).", p_enc_deblock->db_alpha);
		goto exit;
	}
	if (p_enc_deblock->db_beta < -12 || p_enc_deblock->db_beta > 12) {
		snprintf(p_ret_string, max_str_len,
			 "HD_H26XENC_DEBLOCK: db_beta(%d) is out-of-range(-12~12).", p_enc_deblock->db_beta);
		goto exit;
	}
	return 0;
exit:
	return -1;
}

static INT verify_param_HD_H26XENC_RATE_CONTROL(HD_H26XENC_RATE_CONTROL *p_enc_rc, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_rc->rc_mode == HD_RC_MODE_CBR) {
		HD_H26XENC_CBR     *p_cbr = &p_enc_rc->cbr;
		if (p_cbr->bitrate > 128 * 1024 * 1024) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_CBR: bitrate(%u) is out-of-range(0~128Mbit).", p_cbr->bitrate);
			goto exit;
		}
		/* p_cbr->frame_rate_base and p_cbr->frame_rate_incr can be any value.
		 */
		if (p_cbr->init_i_qp > 51 || p_cbr->max_i_qp > 51 || p_cbr->min_i_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_CBR: i_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
				 p_cbr->init_i_qp, p_cbr->max_i_qp, p_cbr->max_i_qp);
			goto exit;
		}
		if (p_cbr->init_p_qp > 51 || p_cbr->max_p_qp > 51 || p_cbr->min_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_CBR: p_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
				 p_cbr->init_p_qp, p_cbr->max_p_qp, p_cbr->max_p_qp);
			goto exit;
		}

		if (p_cbr->static_time > 20) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_CBR: static_time(%u) is out-of-range(0~20).", p_cbr->static_time);
			goto exit;
		}
		if (p_cbr->ip_weight < -100 || p_cbr->ip_weight > 100) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_CBR: ip_weight(%d) is out-of-range(-100~100).", p_cbr->ip_weight);
			goto exit;
		}

	} else if (p_enc_rc->rc_mode == HD_RC_MODE_VBR) {
		HD_H26XENC_VBR     *p_vbr = &p_enc_rc->vbr;
		if (p_vbr->bitrate > 128 * 1024 * 1024) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VBR: bitrate(%u) is out-of-range(0~128Mbit).", p_vbr->bitrate);
			goto exit;
		}
		/* p_vbr->frame_rate_base and p_vbr->frame_rate_incr can be any value.
		 */
		if (p_vbr->init_i_qp > 51 || p_vbr->max_i_qp > 51 || p_vbr->min_i_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VBR: i_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
				 p_vbr->init_i_qp, p_vbr->max_i_qp, p_vbr->max_i_qp);
			goto exit;
		}
		if (p_vbr->init_p_qp > 51 || p_vbr->max_p_qp > 51 || p_vbr->min_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VBR: p_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
				 p_vbr->init_p_qp, p_vbr->max_p_qp, p_vbr->max_p_qp);
			goto exit;
		}

		if (p_vbr->static_time > 20) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VBR: static_time(%u) is out-of-range(0~20).", p_vbr->static_time);
			goto exit;
		}
		if (p_vbr->ip_weight < -100 || p_vbr->ip_weight > 100) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VBR: ip_weight(%d) is out-of-range(-100~100).", p_vbr->ip_weight);
			goto exit;
		}
		if (p_vbr->change_pos > 100) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_VBR: change_pos(%u) is out-of-range(0~100).", p_vbr->change_pos);
			goto exit;
		}

	} else if (p_enc_rc->rc_mode == HD_RC_MODE_FIX_QP) {
		HD_H26XENC_FIXQP     *p_fixqp = &p_enc_rc->fixqp;
		if (p_fixqp->fix_i_qp > 51 || p_fixqp->fix_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_FIXQP: fix_i_qp(%u) fix_p_qp(%u) is out-of-range(0~51).",
				 p_fixqp->fix_i_qp, p_fixqp->fix_p_qp);
			goto exit;
		}

	} else if (p_enc_rc->rc_mode == HD_RC_MODE_EVBR) {
		HD_H26XENC_EVBR    *p_evbr = &p_enc_rc->evbr;
		if (p_evbr->bitrate > 128 * 1024 * 1024) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: bitrate(%u) is out-of-range(0~128Mbit).", p_evbr->bitrate);
			goto exit;
		}
		/* p_evbr->frame_rate_base and p_evbr->frame_rate_incr can be any value.
		 */
		if (p_evbr->init_i_qp > 51 || p_evbr->max_i_qp > 51 || p_evbr->min_i_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: i_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
				 p_evbr->init_i_qp, p_evbr->max_i_qp, p_evbr->max_i_qp);
			goto exit;
		}
		if (p_evbr->init_p_qp > 51 || p_evbr->max_p_qp > 51 || p_evbr->min_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: p_qp(init:%u, min:%u, max:%u) is out-of-range(0~51).",
				 p_evbr->init_p_qp, p_evbr->max_p_qp, p_evbr->max_p_qp);
			goto exit;
		}

		if (p_evbr->static_time > 20) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: static_time(%u) is out-of-range(0~20).", p_evbr->static_time);
			goto exit;
		}
		if (p_evbr->ip_weight < -100 || p_evbr->ip_weight > 100) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: ip_weight(%d) is out-of-range(-100~100).", p_evbr->ip_weight);
			goto exit;
		}

		if (p_evbr->key_p_period > 4096) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: key_p_period(%d) is out-of-range(0~4096).", p_evbr->key_p_period);
			goto exit;
		}
		if (p_evbr->kp_weight < -100 || p_evbr->kp_weight > 100) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: kp_weight(%d) is out-of-range(-100~100).", p_evbr->kp_weight);
			goto exit;
		}
		if (p_evbr->still_frame_cnd < 1 || p_evbr->still_frame_cnd > 4096) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: still_frame_cnd(%d) is out-of-range(1~4096).", p_evbr->still_frame_cnd);
			goto exit;
		}
		if (p_evbr->motion_ratio_thd < 1 || p_evbr->motion_ratio_thd > 100) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: motion_ratio_thd(%d) is out-of-range(1~100).", p_evbr->motion_ratio_thd);
			goto exit;
		}
		if (p_evbr->motion_aq_str < -15 || p_evbr->motion_aq_str > 15) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: motion_aq_str(%d) is out-of-range(-15~15).", p_evbr->motion_aq_str);
			goto exit;
		}

		if (p_evbr->still_i_qp > 51 || p_evbr->still_p_qp > 51 || p_evbr->still_kp_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_EVBR: still_i_qp(%u) still_p_qp(%u) still_kp_qp(%u) is out-of-range(0~51).",
				 p_evbr->still_i_qp, p_evbr->still_p_qp, p_evbr->still_kp_qp);
			goto exit;
		}
	} else {
		snprintf(p_ret_string, max_str_len,
			 "HD_H26XENC_RATE_CONTROL: rc_mode(%d) is not supported.", p_enc_rc->rc_mode);
		goto exit;
	}

	return 0;
exit:
	return -1;
}


static INT verify_param_HD_H26XENC_USR_QP(HD_H26XENC_USR_QP *p_enc_usr_qp, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_usr_qp->enable != 0 && p_enc_usr_qp->enable != 1) {
		snprintf(p_ret_string, max_str_len,
			 "HD_H26XENC_USR_QP: enable(%d) is out-of-range(0~1).", p_enc_usr_qp->enable);
		goto exit;
	}
	if (p_enc_usr_qp->enable == 1) {
		if (p_enc_usr_qp->qp_map_addr == 0) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_USR_QP: qp_map_addr is NULL.");
			goto exit;
		}
	}
	return 0;
exit:
	return -1;
}

static INT verify_param_HD_H26XENC_SLICE_SPLIT(HD_H26XENC_SLICE_SPLIT *p_enc_slice_split, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_slice_split->enable != 0 && p_enc_slice_split->enable != 1) {
		snprintf(p_ret_string, max_str_len,
			 "HD_H26XENC_SLICE_SPLIT: enable(%d) is out-of-range(0~1).", p_enc_slice_split->enable);
		goto exit;
	}
	if (p_enc_slice_split->enable == 1) {
		if ((p_enc_slice_split->slice_row_num < 1)) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_SLICE_SPLIT: slice_row_num(%d) range:(1 ~ num_of_mb/ctu_row).", p_enc_slice_split->slice_row_num);
			goto exit;
		}
	}
	return 0;
exit:
	return -1;
}


static INT verify_param_HD_H26XENC_GDR(HD_H26XENC_GDR *p_enc_gdr, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_gdr->enable != 0 && p_enc_gdr->enable != 1) {
		snprintf(p_ret_string, max_str_len,
			 "HD_H26XENC_GDR: enable(%d) is out-of-range(0~1).", p_enc_gdr->enable);
		goto exit;
	}
	if (p_enc_gdr->enable == 1) {
		/*if (p_enc_gdr->period < 0 || p_enc_gdr->period > 0xFFFFFFFF) {
		    snprintf(p_ret_string, max_str_len,
		            "HD_H26XENC_GDR: period(%d) is out-of-range(0~0xFFFFFFFF).", p_enc_gdr->period);
		    goto exit;
		}*/
		if (p_enc_gdr->number < 1) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_GDR: number(%d) range:(1 ~ num_of_mb/ctu_row).", p_enc_gdr->number);
			goto exit;
		}
	}
	return 0;
exit:
	return -1;
}

static INT verify_param_HD_H26XENC_ROI(HD_H26XENC_ROI  *p_enc_roi, CHAR *p_ret_string, INT max_str_len)
{
	INT i;
	HD_H26XENC_ROI_WIN *p_st_roi;
	/*  if (p_enc_roi->roi_qp_mode < 0 || p_enc_roi->roi_qp_mode > 1) {
	        snprintf(p_ret_string, max_str_len,
	                "HD_H26XENC_ROI: roi_qp_mode(%d) is out-of-range(0~1).", p_enc_roi->roi_qp_mode);
	        goto exit;
	    }*/
	for (i = 0; i < HD_H26XENC_ROI_WIN_COUNT; i++) {
		p_st_roi = &p_enc_roi->st_roi[i];
		if (p_st_roi->enable != 0 && p_st_roi->enable != 1) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_ROI_WIN: enable(%d) is out-of-range(0~1).", p_st_roi->enable);
			goto exit;
		}
		if (p_st_roi->enable == 1) {
			/*if (p_st_roi->rect.x < 0 || p_st_roi->rect.y < 0 || p_st_roi->rect.w < 0 || p_st_roi->rect.h < 0) {
				snprintf(p_ret_string, max_str_len,
						 "HD_H26XENC_ROI_WIN: wrong rect(%d-%d, %dx%d) value.",
						 p_st_roi->rect.x, p_st_roi->rect.y, p_st_roi->rect.w, p_st_roi->rect.h);
				goto exit;
			}*/
			if (p_st_roi->mode > 3) {
				snprintf(p_ret_string, max_str_len,
					 "HD_H26XENC_ROI_WIN: mode(%u) is out-of-range(0~3).", p_st_roi->mode);
				goto exit;
			}
			if (p_st_roi->mode == 3) {
				if (p_st_roi->qp < 0 || p_st_roi->qp > 51) {
					snprintf(p_ret_string, max_str_len,
						 "HD_H26XENC_ROI_WIN: when mode=%u, qp(%d) is out-of-range(0~51).",
						 p_st_roi->mode, p_st_roi->qp);
					goto exit;
				}
			} else {
				if (p_st_roi->qp < -32 || p_st_roi->qp > 31) {
					snprintf(p_ret_string, max_str_len,
						 "HD_H26XENC_ROI_WIN: when mode=%d, qp(%d) is out-of-range(-32~31).",
						 p_st_roi->mode, p_st_roi->qp);
					goto exit;
				}
			}
		}
	}
	return 0;
exit:
	return -1;
}

static INT verify_param_HD_H26XENC_ROW_RC(HD_H26XENC_ROW_RC *p_enc_row_rc, CHAR *p_ret_string, INT max_str_len)
{
	if (p_enc_row_rc->enable != 0 && p_enc_row_rc->enable != 1) {
		snprintf(p_ret_string, max_str_len,
			 "HD_H26XENC_ROW_RC: enable(%d) is out-of-range(0~1).", p_enc_row_rc->enable);
		goto exit;
	}
	if (p_enc_row_rc->enable == 1) {
		if (p_enc_row_rc->i_qp_range > 15) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_ROW_RC: i_qp_range(%u) is out-of-range(0~15).", p_enc_row_rc->i_qp_range);
			goto exit;
		}
		if (p_enc_row_rc->i_qp_step > 15) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_ROW_RC: i_qp_step(%u) is out-of-range(0~15).", p_enc_row_rc->i_qp_step);
			goto exit;
		}
		if (p_enc_row_rc->p_qp_range > 15) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_ROW_RC: p_qp_range(%u) is out-of-range(0~15).", p_enc_row_rc->p_qp_range);
			goto exit;
		}
		if (p_enc_row_rc->p_qp_step > 15) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_ROW_RC: p_qp_step(%u) is out-of-range(0~15).", p_enc_row_rc->p_qp_step);
			goto exit;
		}
		if (p_enc_row_rc->min_i_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_ROW_RC: min_i_qp(%u) is out-of-range(0~51).", p_enc_row_rc->min_i_qp);
			goto exit;
		}
		if (p_enc_row_rc->max_i_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_ROW_RC: max_i_qp(%u) is out-of-range(0~51).", p_enc_row_rc->max_i_qp);
			goto exit;
		}
		if (p_enc_row_rc->min_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_ROW_RC: min_p_qp(%u) is out-of-range(0~51).", p_enc_row_rc->min_p_qp);
			goto exit;
		}
		if (p_enc_row_rc->max_p_qp > 51) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_ROW_RC: max_p_qp(%u) is out-of-range(0~51).", p_enc_row_rc->max_p_qp);
			goto exit;
		}
	}
	return 0;
exit:
	return -1;
}

static INT verify_param_HD_H26XENC_AQ(HD_H26XENC_AQ *p_enc_aq, CHAR *p_ret_string, INT max_str_len)
{
	INT i;
	if (p_enc_aq->enable != 0 && p_enc_aq->enable != 1) {
		snprintf(p_ret_string, max_str_len,
			 "HD_H26XENC_AQ: enable(%d) is out-of-range(0~1).", p_enc_aq->enable);
		goto exit;
	}
	if (p_enc_aq->enable == 1) {
		if (p_enc_aq->i_str < 1 || p_enc_aq->i_str > 8) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_AQ: i_str(%u) is out-of-range(1~8).", p_enc_aq->i_str);
			goto exit;
		}
		if (p_enc_aq->p_str < 1 || p_enc_aq->p_str > 8) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_AQ: p_str(%u) is out-of-range(0~8).", p_enc_aq->p_str);
			goto exit;
		}
		if (p_enc_aq->max_delta_qp < 0 || p_enc_aq->max_delta_qp > 15) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_AQ: max_delta_qp(%u) is out-of-range(0~15).", p_enc_aq->max_delta_qp);
			goto exit;
		}
		if (p_enc_aq->min_delta_qp < -15 || p_enc_aq->min_delta_qp > 0) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_AQ: min_delta_qp(%u) is out-of-range(-15~0).", p_enc_aq->min_delta_qp);
			goto exit;
		}
		if (p_enc_aq->depth > 2) {
			snprintf(p_ret_string, max_str_len,
				 "HD_H26XENC_AQ: depth(%u) is out-of-range(0~2).", p_enc_aq->depth);
			goto exit;
		}
		for (i = 0; i < HD_H26XENC_AQ_MAP_TABLE_NUM; i++) {
			INT16       value = p_enc_aq->thd_table[i];
			if (value < -512 || value > 511) {
				snprintf(p_ret_string, max_str_len,
					 "HD_H26XENC_AQ: p_enc_aq->thd_table[%d](%d) is out-of-range(-512~511).", i, value);
				goto exit;
			}
		}
	}
	return 0;
exit:
	return -1;

}

INT check_params_range(HD_VIDEOENC_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len)
{
	INT ret = 0;
	switch (id) {
	case HD_VIDEOENC_PARAM_PATH_CONFIG:
		break;

	case HD_VIDEOENC_PARAM_IN:
		//verify HD_VIDEOENC_PARAM_IN at hd_videoenc_start
		break;

	case HD_VIDEOENC_PARAM_BUFINFO:
		break;

	case HD_VIDEOENC_PARAM_OUT_ENC_PARAM:
		ret = verify_param_HD_VIDEOENC_OUT((HD_VIDEOENC_OUT *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_VUI:
		ret = verify_param_HD_H26XENC_VUI((HD_H26XENC_VUI *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_DEBLOCK:
		ret = verify_param_HD_H26XENC_DEBLOCK((HD_H26XENC_DEBLOCK *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL:
		ret = verify_param_HD_H26XENC_RATE_CONTROL((HD_H26XENC_RATE_CONTROL *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_USR_QP:
		ret = verify_param_HD_H26XENC_USR_QP((HD_H26XENC_USR_QP *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT:
		ret = verify_param_HD_H26XENC_SLICE_SPLIT((HD_H26XENC_SLICE_SPLIT *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_ENC_GDR:
		ret = verify_param_HD_H26XENC_GDR((HD_H26XENC_GDR *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_ROI:
		ret = verify_param_HD_H26XENC_ROI((HD_H26XENC_ROI *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_ROW_RC:
		ret = verify_param_HD_H26XENC_ROW_RC((HD_H26XENC_ROW_RC *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_AQ:
		ret = verify_param_HD_H26XENC_AQ((HD_H26XENC_AQ *) p_param, p_ret_string, max_str_len);
		break;

	case HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME:
		break;

	default:
		break;
	}
	return ret;
}


INT fill_default_params(UINT32 fd, HD_VIDEOENC_PARAM_ID id, VOID *p_param)
{
#if GET_DEFAULT_PARAM_FROM_DRIVER
	INT ioctl_ret;
	KFLOW_VIDEOENC_H26X_PARAM h26x_param;
#endif

	switch (id) {
	case HD_VIDEOENC_PARAM_PATH_CONFIG:
		break;

	case HD_VIDEOENC_PARAM_IN: {
		HD_VIDEOENC_IN *p_enc_in = (HD_VIDEOENC_IN *) p_param;
		p_enc_in->dim.w = 1920;
		p_enc_in->dim.h = 1080;
		p_enc_in->pxl_fmt = HD_VIDEO_PXLFMT_YUV420;
		p_enc_in->dir = 0;
		break;
	}

//	case HD_VIDEOENC_PARAM_SYSINFO:
	//  break;

	case HD_VIDEOENC_PARAM_BUFINFO:
		break;

	case HD_VIDEOENC_PARAM_OUT_ENC_PARAM: {
		HD_VIDEOENC_OUT *p_enc_out = (HD_VIDEOENC_OUT *) p_param;
		p_enc_out->codec_type = HD_CODEC_TYPE_H264;
		p_enc_out->h26x.gop_num = 60;
		p_enc_out->h26x.chrm_qp_idx = 0;
		p_enc_out->h26x.sec_chrm_qp_idx = 0;
		p_enc_out->h26x.ltr_interval = 0;
		p_enc_out->h26x.ltr_pre_ref = 0;
		p_enc_out->h26x.gray_en = 0;
		p_enc_out->h26x.source_output = 0;
		p_enc_out->h26x.profile = HD_H264E_HIGH_PROFILE;
		p_enc_out->h26x.level_idc = HD_H264E_LEVEL_4_1;
		p_enc_out->h26x.svc_layer = 0;
		p_enc_out->h26x.entropy_mode = 1;
		p_enc_out->jpeg.retstart_interval = 0;
		p_enc_out->jpeg.image_quality = 40;
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_VUI: {
		HD_H26XENC_VUI *p_enc_vui = (HD_H26XENC_VUI *) p_param;
		p_enc_vui->vui_en = 0;
		p_enc_vui->sar_width = 0;
		p_enc_vui->sar_height = 0;
		p_enc_vui->matrix_coef = 2;
		p_enc_vui->transfer_characteristics = 2;
		p_enc_vui->colour_primaries = 2;
		p_enc_vui->video_format = 5;
		p_enc_vui->color_range = 0;
		p_enc_vui->timing_present_flag = 0;
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_DEBLOCK: {
		HD_H26XENC_DEBLOCK *p_enc_deblock = (HD_H26XENC_DEBLOCK *) p_param;
		p_enc_deblock->dis_ilf_idc = 0;
		p_enc_deblock->db_alpha = 0;
		p_enc_deblock->db_beta = 0;
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_RATE_CONTROL: {
		HD_H26XENC_RATE_CONTROL *p_enc_rc = (HD_H26XENC_RATE_CONTROL *) p_param;
		HD_H26XENC_CBR     *p_cbr = &p_enc_rc->cbr;
		p_enc_rc->rc_mode = HD_RC_MODE_CBR;
		p_cbr->bitrate = 2 * 1024 * 1024;
		p_cbr->frame_rate_base = 30;
		p_cbr->frame_rate_incr = 1;
		p_cbr->init_i_qp = 26;
		p_cbr->max_i_qp = 51;
		p_cbr->min_i_qp = 15;
		p_cbr->init_p_qp = 26;
		p_cbr->max_p_qp = 51;
		p_cbr->min_p_qp = 15;
		p_cbr->static_time = 0;
		p_cbr->ip_weight = 0;
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_USR_QP: {
		HD_H26XENC_USR_QP *p_enc_usr_qp = (HD_H26XENC_USR_QP *) p_param;
		p_enc_usr_qp->enable = 0;
		p_enc_usr_qp->qp_map_addr = 0;
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT: {
		HD_H26XENC_SLICE_SPLIT *p_enc_slice_split = (HD_H26XENC_SLICE_SPLIT *) p_param;
		p_enc_slice_split->enable = 0;
		p_enc_slice_split->slice_row_num = 1;
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ENC_GDR: {
		HD_H26XENC_GDR *p_enc_gdr = (HD_H26XENC_GDR *) p_param;
		p_enc_gdr->enable = 0;
		p_enc_gdr->period = 0;
		p_enc_gdr->number = 1;
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ROI: {
		HD_H26XENC_ROI  *p_enc_roi = (HD_H26XENC_ROI *) p_param;
		HD_H26XENC_ROI_WIN  *p_st_roi;
		INT i;
		p_enc_roi->roi_qp_mode = 0;
		for (i = 0; i < HD_H26XENC_ROI_WIN_COUNT; i++) {
			p_st_roi = &p_enc_roi->st_roi[i];
			memset(p_st_roi, 0, sizeof(*p_st_roi));
		}
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_ROW_RC: {
		HD_H26XENC_ROW_RC *p_enc_row_rc = (HD_H26XENC_ROW_RC *) p_param;
#if GET_DEFAULT_PARAM_FROM_DRIVER
		h26x_param.fd = fd;
		h26x_param.param_id = VDOENC_ROW_RC_CONFIG;
		ioctl_ret = ioctl(enc_ioctl, KFLOW_VIDEOENC_IOC_GET_H26X_PARAM, &h26x_param);
		if (ioctl_ret < 0) {
			HD_VIDEOENC_ERR("ioctl \"KFLOW_VIDEOENC_IOC_GET_H26X_PARAM\" return %d\n", ioctl_ret);
			goto ioctl_err_exit;
		}
		p_enc_row_rc->enable = h26x_param.row_rc.enable;
		p_enc_row_rc->i_qp_range = h26x_param.row_rc.i_qp_range;
		p_enc_row_rc->i_qp_step = h26x_param.row_rc.i_qp_step;
		p_enc_row_rc->p_qp_range = h26x_param.row_rc.p_qp_range;
		p_enc_row_rc->p_qp_step = h26x_param.row_rc.p_qp_step;
		p_enc_row_rc->min_i_qp = h26x_param.row_rc.min_i_qp;
		p_enc_row_rc->max_i_qp = h26x_param.row_rc.max_i_qp;
		p_enc_row_rc->min_p_qp = h26x_param.row_rc.min_p_qp;
		p_enc_row_rc->max_p_qp = h26x_param.row_rc.max_p_qp;
#else
		p_enc_row_rc->enable = 1;
		p_enc_row_rc->i_qp_range = 2;
		p_enc_row_rc->i_qp_step = 1;
		p_enc_row_rc->p_qp_range = 4;
		p_enc_row_rc->p_qp_step = 1;
		p_enc_row_rc->min_i_qp = 1;
		p_enc_row_rc->max_i_qp = 51;
		p_enc_row_rc->min_p_qp = 1;
		p_enc_row_rc->max_p_qp = 51;
#endif
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_AQ: {
		HD_H26XENC_AQ *p_enc_aq = (HD_H26XENC_AQ *) p_param;
#if GET_DEFAULT_PARAM_FROM_DRIVER
		INT i;
		h26x_param.fd = fd;
		h26x_param.param_id = VDOENC_AQ_CONFIG;
		ioctl_ret = ioctl(enc_ioctl, KFLOW_VIDEOENC_IOC_GET_H26X_PARAM, &h26x_param);
		if (ioctl_ret < 0) {
			HD_VIDEOENC_ERR("ioctl \"KFLOW_VIDEOENC_IOC_GET_H26X_PARAM\" return %d\n", ioctl_ret);
			goto ioctl_err_exit;
		}
		p_enc_aq->enable = h26x_param.aq.enable;
		p_enc_aq->i_str = h26x_param.aq.i_str;
		p_enc_aq->p_str = h26x_param.aq.p_str;
		p_enc_aq->max_delta_qp = h26x_param.aq.max_delta_qp;
		p_enc_aq->min_delta_qp = h26x_param.aq.min_delta_qp;
		p_enc_aq->depth = h26x_param.aq.depth;
#if HD_H26XENC_AQ_MAP_TABLE_NUM != VG_H26XENC_AQ_MAP_TABLE_COUNT
## error, AQ map table count is not equal.
#endif
		for (i = 0; i < VG_H26XENC_AQ_MAP_TABLE_COUNT; i ++) {
			p_enc_aq->thd_table[i] = h26x_param.aq.thd_table[i];
		}

#else
		p_enc_aq->enable = 0;
		p_enc_aq->i_str = 3;
		p_enc_aq->p_str = 1;
		p_enc_aq->max_delta_qp = 6;
		p_enc_aq->min_delta_qp = -6;
		p_enc_aq->depth = 2;
		memcpy(p_enc_aq->thd_table, default_AQ_mapping_table, sizeof(p_enc_aq->thd_table));
#endif
		break;
	}

	case HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME: {
		HD_H26XENC_REQUEST_IFRAME *p_request_keyframe = (HD_H26XENC_REQUEST_IFRAME *) p_param;
		p_request_keyframe->enable = 0;
		break;
	}
	case HD_VIDEOENC_PARAM_IN_MASK_ATTR: {
		INT i;
		HD_VDOENC_MASK_MOSAIC *p_mask_mosaic = (HD_VDOENC_MASK_MOSAIC *) p_param;
		for (i = 0; i < 8; i++) {
			p_mask_mosaic[i].is_mask = -1;
		}
		break;
	}
	case HD_VIDEOENC_PARAM_IN_STAMP_IMG: {
		INT i;
		HD_OSG_STAMP_IMG *p_stamp = (HD_OSG_STAMP_IMG *) p_param;
		for (i = 0; i < VDOENC_STAMP_NU; i++) {
			p_stamp[i].dim.w = 0;
			p_stamp[i].dim.h = 0;
		}
		break;
	}

	default:
		break;;
	}
	return 0;

#if GET_DEFAULT_PARAM_FROM_DRIVER
ioctl_err_exit:
	return -1;
#endif
}


HD_RESULT videoenc_init_path(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	VDOENC_INFO *p_enc_info;
	VDOENC_INFO_PRIV *p_enc_priv;
	HD_DAL deviceid = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT dev_idx = bd_get_dev_subid(deviceid);
	INT port_idx = BD_GET_OUT_SUBID(out_id);
	INT i, fd, ioctl_ret;
	uintptr_t queue_handle;
	KFLOW_VIDEOENC_FD_INFO fd_info;

	p_enc_info = &videoenc_info[dev_idx];
	p_enc_priv = &p_enc_info->priv[port_idx];

	//allocate h26x_fd and jpeg_fd
	fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_H26X_ENC, dev_idx, 0, port_idx);
	if (fd == 0) {
		HD_VIDEOENC_ERR("%s, get H26X fd(%d) failed.\n", __func__, fd);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_enc_priv->h26x_fd = fd;

	fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_JPEG_ENC, dev_idx, 0, port_idx);
	if (fd == 0) {
		HD_VIDEOENC_ERR("%s, get JPEG fd(%d) failed.\n", __func__, fd);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_enc_priv->jpeg_fd = fd;

	//create callback queue
	queue_handle = pif_create_queue_for_hdal();
	if (queue_handle == 0) {
		HD_VIDEOENC_ERR("%s, create queue(%lu) failed.\n", __func__, (ULONG)queue_handle);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_enc_priv->queue_handle = queue_handle;

	queue_handle = pif_create_queue_for_hdal();
	if (queue_handle == 0) {
		HD_VIDEOENC_ERR("%s, create queue(%lu) failed.\n", __func__, (ULONG)queue_handle);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_enc_priv->osg_queue_handle = queue_handle;
	//open kflow/kdriver
	memset(&fd_info, 0, sizeof(fd_info));
	fd_info.h26x_fd = p_enc_priv->h26x_fd;
	fd_info.jpeg_fd = p_enc_priv->jpeg_fd;
	ioctl_ret = ioctl(enc_ioctl, KFLOW_VIDEOENC_IOC_FD_OPEN, &fd_info);
	if (ioctl_ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"KFLOW_VIDEOENC_IOC_FD_OPEN\" return %d\n", ioctl_ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}

	//init values
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_PATH_CONFIG, &p_enc_info->port[port_idx].enc_path_cfg);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_IN, &p_enc_info->port[port_idx].enc_in_dim);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_ENC_PARAM, &p_enc_info->port[port_idx].enc_out_param);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_VUI, &p_enc_info->port[port_idx].enc_vui);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_DEBLOCK, &p_enc_info->port[port_idx].enc_deblock);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_RATE_CONTROL, &p_enc_info->port[port_idx].enc_rc);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_USR_QP, &p_enc_info->port[port_idx].enc_usr_qp);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_SLICE_SPLIT, &p_enc_info->port[port_idx].enc_slice_split);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_ENC_GDR, &p_enc_info->port[port_idx].enc_gdr);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_ROI, &p_enc_info->port[port_idx].enc_roi);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_ROW_RC, &p_enc_info->port[port_idx].enc_row_rc);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_AQ, &p_enc_info->port[port_idx].enc_aq);
	fill_default_params(p_enc_priv->h26x_fd, HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME, &p_enc_info->port[port_idx].request_keyframe);
	//fill_default_params(HD_VIDEOENC_PARAM_IN_MASK_ATTR, &p_enc_info->port[port_idx].enc_mask_mosaic);
	//fill_default_params(HD_VIDEOENC_PARAM_IN_STAMP_IMG, &p_enc_info->port[port_idx].stamp);

	p_enc_info->port[port_idx].jpeg_rc.vbr_mode = VIDEOENC_JPEG_RC_MODE_FIXQP;
	p_enc_info->port[port_idx].jpeg_rc.base_qp = 40;
	p_enc_info->port[port_idx].jpeg_rc.min_quality = 1;
	p_enc_info->port[port_idx].jpeg_rc.max_quality = 70;
	p_enc_info->port[port_idx].jpeg_rc.bitrate= 32 * 1024 * 1024;
	p_enc_info->port[port_idx].jpeg_rc.frame_rate_base = 30;
	p_enc_info->port[port_idx].jpeg_rc.frame_rate_incr = 1;

	for (i = 0; i < HD_VIDEODEC_MAX_DATA_TYPE; i++) {
		p_enc_info->port[port_idx].enc_path_cfg.data_pool[i].mode = HD_BUF_AUTO;
		videoenc_info[dev_idx].port[port_idx].out_pool.buf_info[i].enable = HD_BUF_AUTO;
	}
	vdoenc_dev_param[dev_idx].port[port_idx].p_jpeg_rc = &p_enc_info->port[port_idx].jpeg_rc;
	vdoenc_dev_param[dev_idx].port[port_idx].p_enc_path_cfg = &p_enc_info->port[port_idx].enc_path_cfg;
	vdoenc_dev_param[dev_idx].port[port_idx].p_out_pool = &videoenc_info[dev_idx].port[port_idx].out_pool;
	vdoenc_dev_param[dev_idx].port[port_idx].get_bs_method = GET_BS_UNKNOWN;

	return HD_OK;

exit:
	memset(p_enc_priv, 0, sizeof(VDOENC_INFO_PRIV));
	return hd_ret;
}

HD_RESULT videoenc_uninit_path(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	VDOENC_INFO *p_enc_info;
	VDOENC_INFO_PRIV *p_enc_priv;
	HD_DAL deviceid = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT dev_idx = bd_get_dev_subid(deviceid);
	INT port_idx = BD_GET_OUT_SUBID(out_id);
	INT ret, ioctl_ret;
	KFLOW_VIDEOENC_FD_INFO fd_info;

	p_enc_info = &videoenc_info[dev_idx];
	p_enc_priv = &p_enc_info->priv[port_idx];

	//close kflow/kdriver
	memset(&fd_info, 0, sizeof(fd_info));
	fd_info.h26x_fd = p_enc_priv->h26x_fd;
	fd_info.jpeg_fd = p_enc_priv->jpeg_fd;
	ioctl_ret = ioctl(enc_ioctl, KFLOW_VIDEOENC_IOC_FD_CLOSE, &fd_info);
	if (ioctl_ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"KFLOW_VIDEOENC_IOC_FD_CLOSE\" return %d\n", ioctl_ret);
		hd_ret = HD_ERR_SYS;
	}
	p_enc_priv->h26x_fd = 0;
	p_enc_priv->jpeg_fd = 0;

	ret = pif_destroy_queue_for_hdal(p_enc_priv->queue_handle);
	if (ret < 0) {
		HD_VIDEOENC_ERR("%s, destroy queue failed. ret(%d)\n", __func__, ret);
		hd_ret = HD_ERR_NG;
	}
	ret = pif_destroy_queue_for_hdal(p_enc_priv->osg_queue_handle);
	if (ret < 0) {
		HD_VIDEOENC_ERR("%s, destroy osg_queue failed. ret(%d)\n", __func__, ret);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}

HD_RESULT videoenc_init(void)
{
	HD_RESULT hd_ret = HD_OK;
	INT ioctl_ret;

	if (enc_ioctl < 0) {
		enc_ioctl = open("/dev/videoenc", O_RDWR);
		if (enc_ioctl == -1) {
			HD_VIDEOENC_ERR("open /dev/videoenc fail\n");
			return HD_ERR_SYS;
		}
	}
	memset(vdoenc_dev_param, 0, sizeof(vdoenc_dev_param));
	memset(videoenc_info, 0, sizeof(videoenc_info));

	ioctl_ret = ioctl(enc_ioctl, KFLOW_VIDEOENC_IOC_GET_HW_SPEC_INFO, &vdoenc_hw_spec_info);
	if (ioctl_ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"KFLOW_VIDEOENC_IOC_GET_HW_SPEC_INFO\" return %d\n", ioctl_ret);
		hd_ret = HD_ERR_SYS;
	}

	return hd_ret;
}

HD_RESULT videoenc_uninit(void)
{
	HD_RESULT hd_ret = HD_OK;
	memset(vdoenc_dev_param, 0, sizeof(vdoenc_dev_param));
	memset(videoenc_info, 0, sizeof(videoenc_info));
	if (enc_ioctl < 0) {
		close(enc_ioctl);
		enc_ioctl = -1;
	}
	return hd_ret;
}



HD_RESULT videoenc_open(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}


HD_RESULT videoenc_close(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_stop_trigger_t stop_trigger_info;
	INT ret;
	UINT32 device_subid, in_subid;
	HD_DAL deviceid = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	device_subid = bd_get_dev_subid(deviceid);
	in_subid = BD_GET_IN_SUBID(in_id);
	VDOENC_INFO *p_enc_info = &videoenc_info[device_subid];
	VDOENC_INFO_PRIV *p_enc_priv = &p_enc_info->priv[in_subid];

	if (p_enc_priv->jpeg_fd != 0) {
		memset(&stop_trigger_info, 0, sizeof(stop_trigger_info));
		stop_trigger_info.usr_proc_fd.fd = p_enc_priv->jpeg_fd;
		stop_trigger_info.mode = USR_PROC_STOP_FD;
		if ((ret = ioctl(vpd_fd, USR_PROC_STOP_TRIGGER, &stop_trigger_info)) < 0) {
			int errsv = errno;
			ret = errsv * -1;
			HD_VIDEOENC_ERR("<ioctl \"USR_PROC_STOP_TRIGGER\" fail, error(%d)>\n", ret);
			hd_ret = HD_ERR_NG;
		}
	}
	if (p_enc_priv->h26x_fd != 0) {
		memset(&stop_trigger_info, 0, sizeof(stop_trigger_info));
		stop_trigger_info.usr_proc_fd.fd = p_enc_priv->h26x_fd;
		stop_trigger_info.mode = USR_PROC_STOP_FD;
		if ((ret = ioctl(vpd_fd, USR_PROC_STOP_TRIGGER, &stop_trigger_info)) < 0) {
			int errsv = errno;
			ret = errsv * -1;
			HD_VIDEOENC_ERR("<ioctl \"USR_PROC_STOP_TRIGGER\" fail, error(%d)>\n", ret);
			hd_ret = HD_ERR_NG;
		}
	}
	return hd_ret;
}


HD_RESULT videoenc_new_in_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT buf_size, ddr_id;
	CHAR pool_name[32];
	VOID *addr_pa;

	buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_video_frame->dim, p_video_frame->pxlfmt);
	if (buf_size == 0) {
		HD_VIDEOENC_ERR("%s, get buf_size(%d) failed.\n", __func__, buf_size);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	ddr_id = p_video_frame->ddr_id;
	sprintf(pool_name, VIDEOENC_MEM_POOL_NAME);
	addr_pa = pif_alloc_video_buffer_for_hdal(buf_size, ddr_id, pool_name, USR_LIB);
	if (addr_pa == NULL) {
		HD_VIDEOENC_ERR("%s, alloca out_buf failed.\n", __func__);
		hd_ret = HD_ERR_NOMEM;
		goto exit;
	}
	p_video_frame->phy_addr[0] = (UINTPTR) addr_pa;

exit:
	return hd_ret;
}

HD_RESULT videoenc_set_osg_for_push_pull(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_in_video_frame, INT32 wait_ms)
{
	usr_proc_trigger_osg_t usr_proc_trigger_osg;
	usr_proc_osg_cb_info_t usr_proc_cb_osg;
	HD_DAL self_id = HD_GET_DEV(path_id);
	INT32 dev_idx = VDOENC_DEVID(self_id);
	VDOENC_INFO *p_enc_info = &videoenc_info[dev_idx];
	HD_IO out_id = HD_GET_OUT(path_id);
	INT outport_idx = VDOENC_OUTID(out_id);
	VDOENC_INFO_PRIV *p_enc_priv = &p_enc_info->priv[outport_idx];
	INT32 ret;
	CHAR pool_name[32];
	int i;

	//check if any stamp should be rendered
	for (i = 0; i < VDOENC_STAMP_NU; i++)
		if (videoenc_info[dev_idx].port[outport_idx].stamp[i].enable)
			break;

	//if no stamp should be rendered, check if any mask should be rendered
	if (i >= VDOENC_STAMP_NU) {
		for (i = 0; i < VDOENC_MASK_NU; i++)
			if (videoenc_info[dev_idx].port[outport_idx].enc_mask_mosaic[i].enable)
				break;

		//neither stamp nor mask should be rendered
		if (i >= VDOENC_MASK_NU)
			return HD_OK;
	}


	usr_proc_trigger_osg.fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_OSG, dev_idx, 0, utl_get_enc_osg_index(outport_idx));
	usr_proc_trigger_osg.in_frame_buffer.ddr_id = p_in_video_frame->ddr_id;
	usr_proc_trigger_osg.in_frame_buffer.addr_pa = p_in_video_frame->phy_addr[0];
	if (pif_address_ddr_sanity_check(usr_proc_trigger_osg.in_frame_buffer.addr_pa, usr_proc_trigger_osg.in_frame_buffer.ddr_id) < 0) {
		HD_VIDEOENC_ERR("%s:Check in_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_proc_trigger_osg.in_frame_buffer.addr_pa, usr_proc_trigger_osg.in_frame_buffer.ddr_id);
		return HD_ERR_PARAM;
	}
	usr_proc_trigger_osg.in_frame_buffer.size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_in_video_frame->dim, p_in_video_frame->pxlfmt);
	pool_name[0] = '\0';
	sprintf(usr_proc_trigger_osg.in_frame_buffer.pool_name, pool_name);
	usr_proc_trigger_osg.in_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_in_video_frame->pxlfmt);
	usr_proc_trigger_osg.in_frame_info.dim.w = p_in_video_frame->dim.w;
	usr_proc_trigger_osg.in_frame_info.dim.h = p_in_video_frame->dim.h;
	usr_proc_trigger_osg.in_frame_info.pathid = p_in_video_frame->reserved[0];
	usr_proc_trigger_osg.direction = 0; //
	usr_proc_trigger_osg.queue_handle = p_enc_priv->osg_queue_handle;
	usr_proc_trigger_osg.p_user_data = (uintptr_t) 0; //pass private data if need
	if ((ret = ioctl(vpd_fd, USR_PROC_TRIGGER_OSG, &usr_proc_trigger_osg)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		HD_VIDEOENC_ERR("<ioctl \"USR_PROC_TRIGGER_OSG\" fail, error(%d)>\n", ret);
		return HD_ERR_SYS;
	}

	usr_proc_cb_osg.queue_handle = p_enc_priv ->osg_queue_handle;
	usr_proc_cb_osg.wait_ms = wait_ms;
	if ((ret = ioctl(vpd_fd, USR_PROC_RECV_DATA_OSG, &usr_proc_cb_osg)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		HD_VIDEOENC_ERR("<ioctl \"USR_PROC_RECV_DATA_OSG\" fail, error(%d)>\n", ret);
		return HD_ERR_SYS;
	}

	return HD_OK;
}


HD_RESULT videoenc_push_in_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_in_video_frame,
			       HD_VIDEOENC_BS *p_usr_out_video_bitstream, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_trigger_adv_encode_t usr_proc_trigger_enc;
	INT ddr_id, ret, buf_size, bs_size, codec_fd;
	CHAR pool_name[32];
	VOID *addr_pa;
	INT dev_idx = VDOENC_DEVID(self_id);
	INT inport_idx = VDOENC_INID(in_id);
	VDOENC_INFO *p_enc_info = &videoenc_info[dev_idx];
	VDOENC_INFO_PRIV *p_enc_priv = &p_enc_info->priv[inport_idx];
	HD_VIDEOENC_IN  *p_enc_in;
	HD_VIDEOENC_OUT *p_enc_param;
	usr_enc_type_t enc_type;
	VDOENC_INTERNAL_PARAM  *p_param = &vdoenc_dev_param[dev_idx];
	UINTPTR  blk = 0;
	CHAR msg_string[256];

	memset(&usr_proc_trigger_enc, 0, sizeof(usr_proc_trigger_enc));

	p_enc_in = &videoenc_info[dev_idx].port[inport_idx].enc_in_dim;
	p_enc_param = &videoenc_info[dev_idx].port[inport_idx].enc_out_param;
	if (in_id < HD_IN_MAX) {
		ret = verify_param_HD_VIDEOENC_IN(p_enc_in, p_enc_param, msg_string, 256);
		if (ret < 0) {
			HD_VIDEOENC_ERR("Wrong value. %s, id=%d\n", msg_string, HD_VIDEOENC_PARAM_IN);
			return HD_ERR_PARAM;
		}
	}

	enc_type = convert_HD_VIDEO_CODEC_to_usr_enc_type(p_enc_param->codec_type);
	codec_fd = (enc_type == VPUSR_ENC_TYPE_JPEG) ? p_enc_priv->jpeg_fd : p_enc_priv->h26x_fd;
	if (codec_fd == 0) {
		HD_VIDEOENC_ERR("%s, codec(type:%d) is not available.\n", __func__, p_enc_param->codec_type);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	if (p_usr_out_video_bitstream) {
		addr_pa = (VOID *) p_usr_out_video_bitstream->video_pack[0].phy_addr;
		ddr_id = p_usr_out_video_bitstream->ddr_id;
		bs_size = p_usr_out_video_bitstream->video_pack[0].size;
		blk = p_usr_out_video_bitstream->blk;
	} else {
		//prepare out-buffer
		bs_size = estimate_max_bs_len(p_enc_param->codec_type, p_in_video_frame->dim, TRUE);
		if (bs_size == 0) {
			HD_VIDEOENC_ERR("%s, estimate bs_len is 0.\n", __func__);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		ddr_id = 0;
		sprintf(pool_name, VIDEOENC_MEM_POOL_NAME);
		addr_pa = pif_alloc_video_buffer_for_hdal(bs_size, ddr_id, pool_name, USR_LIB);
		if (addr_pa == NULL) {
			HD_VIDEOENC_ERR("%s, alloca out_buf failed.\n", __func__);
			hd_ret = HD_ERR_NOMEM;
			goto exit;
		}
	}

	//fill trigger parameters according to user parameters
	buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_in_video_frame->dim, p_in_video_frame->pxlfmt);
	usr_proc_trigger_enc.fd = codec_fd;
	usr_proc_trigger_enc.in_frame_buffer.ddr_id = p_in_video_frame->ddr_id;
	usr_proc_trigger_enc.in_frame_buffer.addr_pa = (UINTPTR) p_in_video_frame->phy_addr[0];
	usr_proc_trigger_enc.in_frame_buffer.size = buf_size;
	if (pif_address_ddr_sanity_check(usr_proc_trigger_enc.in_frame_buffer.addr_pa, usr_proc_trigger_enc.in_frame_buffer.ddr_id) < 0) {
		HD_VIDEOENC_ERR("%s:Check in_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_proc_trigger_enc.in_frame_buffer.addr_pa,
			usr_proc_trigger_enc.in_frame_buffer.ddr_id);
		return HD_ERR_PARAM;
	}
	usr_proc_trigger_enc.max_out_bs_buffer.ddr_id = ddr_id;
	usr_proc_trigger_enc.max_out_bs_buffer.addr_pa = (UINTPTR) addr_pa;
	usr_proc_trigger_enc.max_out_bs_buffer.size = bs_size;
	if (pif_address_ddr_sanity_check(usr_proc_trigger_enc.max_out_bs_buffer.addr_pa, usr_proc_trigger_enc.max_out_bs_buffer.ddr_id) < 0) {
		HD_VIDEOENC_ERR("%s:Check out_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_proc_trigger_enc.max_out_bs_buffer.addr_pa,
			usr_proc_trigger_enc.max_out_bs_buffer.ddr_id);
		return HD_ERR_PARAM;
	}
	usr_proc_trigger_enc.in_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_in_video_frame->pxlfmt);
	usr_proc_trigger_enc.in_frame_info.dim.w = p_in_video_frame->dim.w;
	usr_proc_trigger_enc.in_frame_info.dim.h = p_in_video_frame->dim.h;
	usr_proc_trigger_enc.in_frame_info.pathid = p_in_video_frame->reserved[0];
	usr_proc_trigger_enc.in_frame_info.timestamp = p_in_video_frame->timestamp;

	usr_proc_trigger_enc.enc_type = enc_type;
	usr_proc_trigger_enc.src_framerate = 30;
	usr_proc_trigger_enc.fps_ratio.numerator = 1;
	usr_proc_trigger_enc.fps_ratio.denominator = 1;


	if (enc_type == VPUSR_ENC_TYPE_JPEG) {
		VIDEOENC_JPEG_RATE_CONTROL *p_jpeg_rc = &p_enc_info->port[inport_idx].jpeg_rc;
		usr_jpeg_enc_param_t *p_jpeg = &usr_proc_trigger_enc.jpeg;
		p_jpeg->init_quality = p_enc_param->jpeg.image_quality;

		p_jpeg->rate_control.vbr_mode = p_jpeg_rc->vbr_mode;
		p_jpeg->rate_control.bitrate = p_jpeg_rc->bitrate;
		p_jpeg->rate_control.frame_rate_base = p_jpeg_rc->frame_rate_base;
		p_jpeg->rate_control.frame_rate_incr = p_jpeg_rc->frame_rate_incr;
		if (p_jpeg_rc->vbr_mode == VIDEOENC_JPEG_RC_MODE_FIXQP) {
			p_jpeg->rate_control.base_qp = p_enc_param->jpeg.image_quality;
		} else {
			p_jpeg->rate_control.base_qp = p_jpeg_rc->base_qp;
		}
		p_jpeg->rate_control.max_quality = p_jpeg_rc->max_quality;
		p_jpeg->rate_control.min_quality= p_jpeg_rc->min_quality;

		switch (p_param->port[inport_idx].p_enc_in_param->dir) {
		case HD_VIDEO_DIR_ROTATE_90:
			p_jpeg->rotate = 2;//ROTATE_RIGHT;
			break;
		case HD_VIDEO_DIR_ROTATE_270:
			p_jpeg->rotate = 1;//ROTATE_LEFT;
			break;
		case HD_VIDEO_DIR_ROTATE_180:
			p_jpeg->rotate = 3;//ROTATE_180;
			break;
		default:
			p_jpeg->rotate = 0;//ROTATE_OFF;
			break;
		}
		if (p_param->port[inport_idx].p_enc_in_param->dir == HD_VIDEO_DIR_ROTATE_90 ||
		    p_param->port[inport_idx].p_enc_in_param->dir == HD_VIDEO_DIR_ROTATE_270) {
			p_jpeg->src_dim.w = p_param->port[inport_idx].p_enc_in_param->dim.h;
			p_jpeg->src_dim.h = p_param->port[inport_idx].p_enc_in_param->dim.w;
			usr_proc_trigger_enc.in_frame_info.dim.w = p_in_video_frame->dim.h;
			usr_proc_trigger_enc.in_frame_info.dim.h = p_in_video_frame->dim.w;
		} else {
			p_jpeg->src_dim.w = p_param->port[inport_idx].p_enc_in_param->dim.w;
			p_jpeg->src_dim.h = p_param->port[inport_idx].p_enc_in_param->dim.h;
			usr_proc_trigger_enc.in_frame_info.dim.w = p_in_video_frame->dim.w;
			usr_proc_trigger_enc.in_frame_info.dim.h = p_in_video_frame->dim.h;
		}
	} else {
		h26x_param_set *p_h26x = &usr_proc_trigger_enc.h26x;
		hdal_videoenc_setting_t h26x_enc_setting;
		dim_t src_dim;

		memset(&h26x_enc_setting, 0, sizeof(h26x_enc_setting));
		if (p_param->port[inport_idx].p_enc_in_param) {
			h26x_enc_setting.is_enc_in_param = 1;
			h26x_enc_setting.enc_in_param = *p_param->port[inport_idx].p_enc_in_param;
		}
		if (p_param->port[inport_idx].p_enc_out_param) {
			h26x_enc_setting.is_enc_out_param = 1;
			h26x_enc_setting.enc_out_param = *p_param->port[inport_idx].p_enc_out_param;
		}
		if (p_param->port[inport_idx].p_enc_vui) {
			h26x_enc_setting.is_enc_vui = 1;
			h26x_enc_setting.enc_vui = *p_param->port[inport_idx].p_enc_vui;
		}
		if (p_param->port[inport_idx].p_enc_deblock) {
			h26x_enc_setting.is_enc_deblock = 1;
			h26x_enc_setting.enc_deblock = *p_param->port[inport_idx].p_enc_deblock;
		}
		if (p_param->port[inport_idx].p_enc_rc) {
			h26x_enc_setting.is_enc_rc = 1;
			h26x_enc_setting.enc_rc = *p_param->port[inport_idx].p_enc_rc;
		}
		if (p_param->port[inport_idx].p_enc_usr_qp) {
			h26x_enc_setting.is_enc_usr_qp = 1;
			h26x_enc_setting.enc_usr_qp = *p_param->port[inport_idx].p_enc_usr_qp;
		}
		if (p_param->port[inport_idx].p_enc_slice_split) {
			h26x_enc_setting.is_enc_slice_split = 1;
			h26x_enc_setting.enc_slice_split = *p_param->port[inport_idx].p_enc_slice_split;
		}
		if (p_param->port[inport_idx].p_enc_gdr) {
			h26x_enc_setting.is_enc_gdr = 1;
			h26x_enc_setting.enc_gdr = *p_param->port[inport_idx].p_enc_gdr;
		}
		if (p_param->port[inport_idx].p_enc_roi) {
			h26x_enc_setting.is_enc_roi = 1;
			h26x_enc_setting.enc_roi = *p_param->port[inport_idx].p_enc_roi;
		}
		if (p_param->port[inport_idx].p_enc_row_rc) {
			h26x_enc_setting.is_enc_row_rc = 1;
			h26x_enc_setting.enc_row_rc = *p_param->port[inport_idx].p_enc_row_rc;
		}
		if (p_param->port[inport_idx].p_enc_aq) {
			h26x_enc_setting.is_enc_aq = 1;
			h26x_enc_setting.enc_aq = *p_param->port[inport_idx].p_enc_aq;
		}

		if (p_param->port[inport_idx].p_enc_in_param->dir == HD_VIDEO_DIR_ROTATE_90 ||
		    p_param->port[inport_idx].p_enc_in_param->dir == HD_VIDEO_DIR_ROTATE_270) {
			src_dim.w = p_in_video_frame->dim.h;
			src_dim.h = p_in_video_frame->dim.w;
			usr_proc_trigger_enc.in_frame_info.dim.w = p_in_video_frame->dim.h;
			usr_proc_trigger_enc.in_frame_info.dim.h = p_in_video_frame->dim.w;
		} else {
			src_dim.w = p_in_video_frame->dim.w;
			src_dim.h = p_in_video_frame->dim.h;
			usr_proc_trigger_enc.in_frame_info.dim.w = p_in_video_frame->dim.w;
			usr_proc_trigger_enc.in_frame_info.dim.h = p_in_video_frame->dim.h;
		}
		fill_h26x_param_from_hdal(p_h26x, &h26x_enc_setting, p_in_video_frame->pxlfmt, src_dim);

		if (p_param->port[inport_idx].set_count != p_enc_priv->last_set_count) {
			p_enc_priv->param_ver_count ++;
			p_enc_priv->last_set_count = p_param->port[inport_idx].set_count;
		}
		p_h26x->param_ver = p_enc_priv->param_ver_count;
		p_h26x->update_flag.item.vg_enc_buf_param = 1;
		if (HD_VIDEO_PXLFMT_YUV420_NVX3 /*TYPE_YUV420_SCE*/ == p_in_video_frame->pxlfmt) {
			p_h26x->enc_buf_param.source_y_line_offset = ALIGN16_UP(ALIGN32_UP(src_dim.w) * 3 / 4);
			p_h26x->enc_buf_param.source_c_line_offset = ALIGN16_UP(ALIGN32_UP(src_dim.w) * 3 / 4);
			p_h26x->enc_buf_param.source_chroma_offset = p_h26x->enc_buf_param.source_y_line_offset * src_dim.h;
		} else if (HD_VIDEO_PXLFMT_YUV420 == p_in_video_frame->pxlfmt) {
			p_h26x->enc_buf_param.source_y_line_offset = src_dim.w;
			p_h26x->enc_buf_param.source_c_line_offset = src_dim.w;
			p_h26x->enc_buf_param.source_chroma_offset = p_h26x->enc_buf_param.source_y_line_offset * src_dim.h;
		}
	}

	usr_proc_trigger_enc.queue_handle = p_enc_priv->queue_handle;
	usr_proc_trigger_enc.p_user_data = (void *) blk;

	if ((ret = ioctl(vpd_fd, USR_PROC_TRIGGER_ADV_ENCODE, &usr_proc_trigger_enc)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -2) {
			HD_VIDEOENC_ERR("path(%#x) in_frame_buffer(phy_addr[0]=%p) is still in use."
					"Please 'pull' this buffer before push it.\n",
					p_enc_priv->path_id, (VOID *)p_in_video_frame->phy_addr[0]);
		}

		HD_VIDEOENC_ERR("<ioctl \"USR_PROC_TRIGGER_ADV_ENCODE\" fail, error(%d)>\n", ret);
		hd_ret = HD_ERR_SYS;
	}

exit:
	return hd_ret;
}

HD_RESULT videoenc_release_in_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT ddr_id, ret;
	VOID *addr_pa;

	addr_pa = (VOID *) p_video_frame->phy_addr[0];
	ddr_id = p_video_frame->ddr_id;

	ret = pif_free_video_buffer_for_hdal(addr_pa, ddr_id, USR_LIB);
	if (ret < 0) {
		HD_VIDEOENC_ERR("%s, free buffer(pa:%p) failed.\n", __func__, addr_pa);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}



HD_RESULT videoenc_pull_out_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEOENC_BS *p_video_bitstream, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_adv_enc_cb_info_t usr_proc_enc_cb_info;
	INT ret, pack_count;
	INT dev_idx = VDOENC_DEVID(self_id);
	INT outport_idx = VDOENC_OUTID(out_id);
	VDOENC_INFO *p_enc_info = &videoenc_info[dev_idx];
	VDOENC_INFO_PRIV *p_enc_priv = &p_enc_info->priv[outport_idx];
	HD_VIDEOENC_OUT *p_enc_param = &p_enc_info->port[outport_idx].enc_out_param;
	HD_VIDEO_BS_DATA *p_video_pack;
	HD_VIDEO_CODEC enc_type = p_enc_param->codec_type;

	memset(&usr_proc_enc_cb_info, 0, sizeof(usr_proc_enc_cb_info));
	hd_ret = dif_pull_out_buffer(self_id, out_id, p_video_bitstream, wait_ms);
	if (hd_ret == HD_OK) {
		if (p_video_bitstream->video_pack[0].phy_addr == 0) {
			HD_VIDEOENC_ERR("dif_pull_out_buffer fail\n");
			hd_ret = HD_ERR_NG;
			goto exit;
		}

		if (vdoenc_dev_param[dev_idx].port[outport_idx].get_bs_method == GET_BS_BY_RECV) {
			HD_VIDEOENC_ERR("pull_out operation can't be mixed with recv_list, pathid(%#x)\n",
					p_enc_priv->path_id);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		vdoenc_dev_param[dev_idx].port[outport_idx].get_bs_method = GET_BS_BY_PULL;
	} else if (hd_ret == HD_ERR_NOT_AVAIL) {
		if (p_enc_priv->queue_handle == 0) {
			//HD_VIDEOENC_ERR("%s, output queue is not ready. self_id(%#x) out_id(%#x)\n", __func__, self_id, out_id);
			hd_ret = HD_ERR_NG;
			goto exit;
		}

		usr_proc_enc_cb_info.queue_handle = p_enc_priv->queue_handle;
		usr_proc_enc_cb_info.wait_ms = wait_ms;
		if ((ret = ioctl(vpd_fd, USR_PROC_RECV_DATA_ADV_ENC, &usr_proc_enc_cb_info)) < 0) {
			int errsv = errno * -1;
			if (errsv == -15) {
				hd_ret = HD_ERR_TIMEDOUT;
			} else {
				HD_VIDEOENC_ERR("<ioctl \"USR_PROC_RECV_DATA_ADV_ENC\" fail, error(%d)>\n", ret);
				hd_ret = HD_ERR_SYS;
			}
			goto exit;
		}

		if (usr_proc_enc_cb_info.status == 1) { // 1:JOB_STATUS_FINISH
			hd_ret = HD_OK;
		} else {
			hd_ret = HD_ERR_NG;
			goto exit;
		}

		if (p_video_bitstream) {
			p_video_bitstream->sign = MAKEFOURCC('V', 'S', 'T', 'M');
			p_video_bitstream->p_next = NULL;
			p_video_bitstream->vcodec_format = enc_type;
			p_video_bitstream->pack_num = 1;
			p_video_bitstream->timestamp = 0;
			p_video_bitstream->frame_type = get_hdal_frame_type(usr_proc_enc_cb_info.slice_type);
			p_video_bitstream->svc_layer_type = get_hdal_svc_layer_type(usr_proc_enc_cb_info.svc_layer_type);
			p_video_bitstream->evbr_state = usr_proc_enc_cb_info.evbr_state;

			p_video_bitstream->video_pack[0].pack_type.h264_type = H265_NALU_TYPE_SLICE;
			p_video_bitstream->video_pack[0].phy_addr = usr_proc_enc_cb_info.addr_pa;
			p_video_bitstream->video_pack[0].size = usr_proc_enc_cb_info.bs_len;

			p_video_bitstream->timestamp = usr_proc_enc_cb_info.timestamp;
			p_video_bitstream->blk = (typeof(p_video_bitstream->blk)) usr_proc_enc_cb_info.p_user_data;


			if (enc_type == HD_CODEC_TYPE_JPEG) {
				p_video_pack = &p_video_bitstream->video_pack[0];
				p_video_pack->pack_type.jpeg_type = JPEG_PACK_TYPE_PIC;
				p_video_pack->phy_addr = usr_proc_enc_cb_info.addr_pa + usr_proc_enc_cb_info.bitstream_offset;
				p_video_pack->size = usr_proc_enc_cb_info.bs_len;
				p_video_bitstream->pack_num = 1;
				goto continue_filling;
			}

			//For H264/H265, only 3 possible combinations
			// - bs              (h264/h265)
			// - sps+pps+bs      (h264)
			// - vps+sps+pps_bs  (h265)
			// sequence: VPS -> SPS -> PPS -> slice
			if ((int)usr_proc_enc_cb_info.bs_offset == -1) {
				HD_VIDEOENC_ERR("videoenc:No bs output.\n");
				hd_ret = HD_ERR_NG;
				goto exit;
			} else {
				if (((int)usr_proc_enc_cb_info.sps_offset == -1 && (int)usr_proc_enc_cb_info.pps_offset != -1) ||
				    ((int)usr_proc_enc_cb_info.sps_offset != -1 && (int)usr_proc_enc_cb_info.pps_offset == -1)) {
					HD_VIDEOENC_ERR("videoenc:No sps(%d) and pps(%d) output.\n", usr_proc_enc_cb_info.sps_offset, usr_proc_enc_cb_info.pps_offset);
					hd_ret = HD_ERR_NG;
					goto exit;
				} else {
				}
			}

			pack_count = 0;
			if ((int)usr_proc_enc_cb_info.vps_offset != -1) {
				p_video_pack = &p_video_bitstream->video_pack[pack_count];
				p_video_pack->pack_type.h265_type = H265_NALU_TYPE_VPS;
				p_video_pack->phy_addr = usr_proc_enc_cb_info.addr_pa;
				p_video_pack->size = usr_proc_enc_cb_info.sps_offset - usr_proc_enc_cb_info.vps_offset;
				pack_count ++;
			}
			if ((int)usr_proc_enc_cb_info.sps_offset != -1) {
				p_video_pack = &p_video_bitstream->video_pack[pack_count];
				if (enc_type == HD_CODEC_TYPE_H264) {
					p_video_pack->pack_type.h264_type = H264_NALU_TYPE_SPS;
				} else {
					p_video_pack->pack_type.h265_type = H265_NALU_TYPE_SPS;
				}
				p_video_pack->phy_addr = usr_proc_enc_cb_info.addr_pa + usr_proc_enc_cb_info.sps_offset;
				p_video_pack->size = usr_proc_enc_cb_info.pps_offset - usr_proc_enc_cb_info.sps_offset;
				pack_count ++;
			}
			if ((int)usr_proc_enc_cb_info.pps_offset != -1) {
				p_video_pack = &p_video_bitstream->video_pack[pack_count];
				if (enc_type == HD_CODEC_TYPE_H264) {
					p_video_pack->pack_type.h264_type = H264_NALU_TYPE_PPS;
				} else {
					p_video_pack->pack_type.h265_type = H265_NALU_TYPE_PPS;
				}
				p_video_pack->phy_addr = usr_proc_enc_cb_info.addr_pa + usr_proc_enc_cb_info.pps_offset;
				p_video_pack->size = usr_proc_enc_cb_info.bs_offset - usr_proc_enc_cb_info.pps_offset;
				pack_count ++;
			}
			if ((int)usr_proc_enc_cb_info.bs_offset != -1) {
				p_video_pack = &p_video_bitstream->video_pack[pack_count];
				if (enc_type == HD_CODEC_TYPE_H264) {
					p_video_pack->pack_type.h264_type = H264_NALU_TYPE_SLICE;
				} else {
					p_video_pack->pack_type.h265_type = H265_NALU_TYPE_SLICE;
				}
				p_video_pack->phy_addr = usr_proc_enc_cb_info.addr_pa + usr_proc_enc_cb_info.bs_offset;
				p_video_pack->size = usr_proc_enc_cb_info.bs_len - usr_proc_enc_cb_info.bs_offset;
				pack_count ++;
			}
			p_video_bitstream->pack_num = pack_count;

continue_filling:

			p_video_bitstream->psnr_info.y_mse = usr_proc_enc_cb_info.psnr_y_mse;
			p_video_bitstream->psnr_info.u_mse = usr_proc_enc_cb_info.psnr_u_mse;
			p_video_bitstream->psnr_info.v_mse = usr_proc_enc_cb_info.psnr_v_mse;

			p_video_bitstream->motion_ratio = usr_proc_enc_cb_info.motion_ratio;

			if (p_enc_param->codec_type == HD_CODEC_TYPE_H265) {
				p_video_bitstream->blk_info.h265_info.intra_32x32_cu_num = usr_proc_enc_cb_info.intra_32_cu_num;
				p_video_bitstream->blk_info.h265_info.intra_16x16_cu_num = usr_proc_enc_cb_info.intra_16_cu_num;
				p_video_bitstream->blk_info.h265_info.intra_8x8_cu_num = usr_proc_enc_cb_info.intra_8_cu_num;
				p_video_bitstream->blk_info.h265_info.inter_64x64_cu_num = usr_proc_enc_cb_info.inter_64_cu_num;
				p_video_bitstream->blk_info.h265_info.inter_32x32_cu_num = usr_proc_enc_cb_info.inter_32_cu_num;
				p_video_bitstream->blk_info.h265_info.inter_16x16_cu_num = usr_proc_enc_cb_info.inter_16_cu_num;
				p_video_bitstream->blk_info.h265_info.skip_cu_num = usr_proc_enc_cb_info.skip_cu_num;
				p_video_bitstream->blk_info.h265_info.merge_cu_num = usr_proc_enc_cb_info.merge_cu_num;
			} else if (p_enc_param->codec_type == HD_CODEC_TYPE_H264) {
				p_video_bitstream->blk_info.h264_info.intra_16x16_mb_num = usr_proc_enc_cb_info.intra_16_mb_num;
				p_video_bitstream->blk_info.h264_info.intra_8x8_mb_Num = usr_proc_enc_cb_info.intra_8_mb_num;
				p_video_bitstream->blk_info.h264_info.intra_4x4_mb_num = usr_proc_enc_cb_info.intra_4_mb_num;
				p_video_bitstream->blk_info.h264_info.inter_mb_num = usr_proc_enc_cb_info.inter_mb_num;
				p_video_bitstream->blk_info.h264_info.skip_mb_num = usr_proc_enc_cb_info.skip_mb_num;
			} else if (p_enc_param->codec_type == HD_CODEC_TYPE_JPEG) {

			}
			p_video_bitstream->qp = usr_proc_enc_cb_info.qp;
			p_video_bitstream->ddr_id = usr_proc_enc_cb_info.ddr_id;
			if (pif_address_ddr_sanity_check(usr_proc_enc_cb_info.addr_pa, p_video_bitstream->ddr_id) < 0) {
				HD_VIDEOENC_ERR("%s:Check pull_out pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_proc_enc_cb_info.addr_pa, p_video_bitstream->ddr_id);
				hd_ret = HD_ERR_PARAM;
				goto exit;
			}
		}
	} else {
		HD_VIDEOENC_ERR("pull out buffer error, ret(%#x)\n", hd_ret);
	}
exit:
	return hd_ret;
}


HD_RESULT videoenc_release_out_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEOENC_BS *p_video_bitstream)
{
	HD_RESULT hd_ret = HD_OK;
	INT  ddr_id, ret;
	VOID *addr_pa;

	hd_ret = dif_release_out_buffer(self_id, out_id, p_video_bitstream);
	if (hd_ret == HD_OK) {
		p_video_bitstream->video_pack[0].phy_addr = 0;
	} else if (hd_ret == HD_ERR_NOT_AVAIL) {
		addr_pa = (VOID *) p_video_bitstream->video_pack[0].phy_addr;
		ddr_id = p_video_bitstream->ddr_id;

		ret = pif_free_video_buffer_for_hdal(addr_pa, ddr_id, USR_USR);
		if (ret < 0) {
			HD_VIDEOENC_ERR("%s, free buffer(pa:%p) failed.\n", __func__, addr_pa);
			hd_ret = HD_ERR_NG;
		}
	} else {
		HD_VIDEOENC_ERR("%s, release out buffer failed. ret(%#x)\n", __func__, hd_ret);
	}
	return hd_ret;
}


VDOENC_INTERNAL_PARAM *videoenc_get_param_p(HD_DAL self_id)
{
	if (VDOENC_DEVID(self_id) > MAX_VDOENC_DEV) {
		HD_VIDEOENC_ERR("Error self_id(%d)\n", self_id);
		goto err_ext;
	}
	return &vdoenc_dev_param[VDOENC_DEVID(self_id)];
err_ext:
	return NULL;
}

HD_RESULT videoenc_request_keyframe_p(HD_DAL self_id,  HD_IO out_id)
{
	HD_RESULT hd_ret = HD_OK;
	hd_ret = bd_apply_attr(self_id, out_id, HD_VIDEOENC_PARAM_OUT_REQUEST_IFRAME);
	return hd_ret;
}

HD_RESULT videoenc_set_external_osg_win(HD_PATH_ID path_id, BOOL enable)
{

	HD_RESULT hd_ret = HD_OK;
	struct multi_osg_window_info_t osg_win = {0};
	struct osg_window_info_t *osg_window_info = 0;
	HD_VDOENC_STAMP_PARAM *p_stamp_info;
	INT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = VDOENC_DEVID(self_id);
	INT outport_idx = VDOENC_OUTID(out_id);
	INT osg_idx = VDOENC_STAMP_INID(in_id);
	UINT8 stamp_nu = VDOENC_STAMP_NU;
	INT stamp_idx;
	INT win_idx = 0;

	if (osg_ioctl <= 0) {
		HD_VIDEOENC_ERR("%s: Error, no osg ioctl supported!", __FUNCTION__);
		return HD_ERR_SYS;
	}
	p_stamp_info = &videoenc_info[dev_idx].port[outport_idx].stamp[osg_idx];
	p_stamp_info->enable = enable;
	p_stamp_info->path_id = path_id;

	osg_window_info = (struct osg_window_info_t *)PIF_CALLOC(1, stamp_nu * sizeof(struct osg_window_info_t));
	if (osg_window_info == NULL)
		return HD_ERR_SYS;

	for (stamp_idx = 0; stamp_idx < stamp_nu; stamp_idx++) {
		p_stamp_info = &videoenc_info[dev_idx].port[outport_idx].stamp[stamp_idx];
		if (!p_stamp_info->enable)
			continue;

		osg_window_info[win_idx].osg_idx = stamp_idx;
		osg_window_info[win_idx].enable = p_stamp_info->enable;
		osg_window_info[win_idx].st_disp.fg_alpha = p_stamp_info->attr.alpha;
		osg_window_info[win_idx].st_disp.bg_alpha = 0;
		osg_window_info[win_idx].st_disp.align_type = p_stamp_info->attr.align_type;
		osg_window_info[win_idx].st_disp.pos_x = p_stamp_info->attr.position.x;
		osg_window_info[win_idx].st_disp.pos_y = p_stamp_info->attr.position.y;
		osg_window_info[win_idx].st_grap.type = get_external_osg_type_idx(p_stamp_info->img.fmt);
		osg_window_info[win_idx].st_grap.width = p_stamp_info->img.dim.w;
		osg_window_info[win_idx].st_grap.height = p_stamp_info->img.dim.h;
		osg_window_info[win_idx].st_grap.pa_addr = p_stamp_info->img.p_addr;
		osg_window_info[win_idx].st_grap.source_img_ddr_id = p_stamp_info->img.ddr_id;
		if (pif_address_ddr_sanity_check(osg_window_info[win_idx].st_grap.pa_addr, osg_window_info[win_idx].st_grap.source_img_ddr_id) < 0) {
			HD_VIDEOENC_ERR("%s:Check out_pa(%#lx) ddrid(%d) error\n", __func__, osg_window_info[win_idx].st_grap.pa_addr,
				osg_window_info[win_idx].st_grap.source_img_ddr_id);
			hd_ret = HD_ERR_PARAM;
			goto err_ext;
		}
		if (osg_window_info[win_idx].st_grap.type == 255) {
			HD_VIDEOENC_ERR("path(%d) stamp idx(%d) use unsupport format(%#x)\n", path_id, stamp_idx, p_stamp_info->img.fmt);
		}
		win_idx++;
	}
	if (stamp_nu) {
		osg_win.fd =  pif_alloc_user_fd_for_hdal(VPD_TYPE_OSG, dev_idx, 0, utl_get_enc_osg_index(outport_idx));
		osg_win.num_osg_window = stamp_nu;
		osg_win.osg_window_info = osg_window_info;
		videoenc_info[dev_idx].port[outport_idx].osg_fd = osg_win.fd;

		ret = ioctl(osg_ioctl, IOCTL_OSG_SET_MULTI_WINDOW, &osg_win); //notify fd invalid
		if (ret < 0) {
			HD_VIDEOENC_ERR("ioctl \"IOCTL_OSG_SET_MULTI_WINDOW\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}
err_ext:
	if (osg_window_info)
		PIF_FREE((void *)osg_window_info);
	return hd_ret;

}

HD_RESULT videoenc_set_internal_osg_win(HD_PATH_ID path_id, BOOL enable)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = VDOENC_DEVID(self_id);
	INT outport_idx = VDOENC_OUTID(out_id);
	INT osg_idx = VDOENC_STAMP_INID(in_id);
	KFLOW_VIDEOENC_OSG_WIN enc_osg = {0};
	HD_VDOENC_STAMP_PARAM *p_stamp_info;
	HD_VIDEO_CODEC  codec_type;
	VDOENC_INFO *p_enc_info = &videoenc_info[dev_idx];
	VDOENC_INFO_PRIV *p_enc_priv = &p_enc_info->priv[outport_idx];
	HD_VIDEOENC_OUT *p_enc_param = &p_enc_info->port[outport_idx].enc_out_param;
	unsigned int osg_support_mask;
	HD_RESULT hd_ret = HD_OK;
	int i;
	INT ioctl_ret;

	if (enc_ioctl < 0) {
		HD_VIDEOENC_ERR("%s: Error, enc_ioctl is not init!\n", __FUNCTION__);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	codec_type = p_enc_param->codec_type;

	if (codec_type == HD_CODEC_TYPE_JPEG) {
		osg_support_mask = vdoenc_hw_spec_info.jpeg.osg_support_mask;
		enc_osg.fd = p_enc_priv->jpeg_fd;
		ioctl_ret = ioctl(enc_ioctl, JPEG_ENC_IOC_GET_OSG_WIN, &enc_osg);
	} else {
		osg_support_mask = vdoenc_hw_spec_info.h26x.osg_support_mask;
		enc_osg.fd = p_enc_priv->h26x_fd;
		ioctl_ret = ioctl(enc_ioctl, H26X_ENC_IOC_GET_OSG_WIN, &enc_osg);
	}
	if (ioctl_ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"H26X_ENC_IOC_GET_OSG_WIN\" return %d\n", ioctl_ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	if (osg_support_mask == 0) {
		HD_VIDEOENC_ERR("codec %s not supported osg\n", get_codec_name(codec_type));
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	p_stamp_info = &videoenc_info[dev_idx].port[outport_idx].stamp[osg_idx];
	p_stamp_info->enable = enable;
	p_stamp_info->path_id = path_id;

	enc_osg.fd = (codec_type == HD_CODEC_TYPE_JPEG) ? p_enc_priv->jpeg_fd : p_enc_priv->h26x_fd;
	enc_osg.st_osg_win.enable = enable;
	enc_osg.st_osg_win.osg_idx = osg_idx;
	enc_osg.st_osg_win.ddr_id = p_stamp_info->img.ddr_id;
	enc_osg.st_osg_win.st_gcac.enable = p_stamp_info->attr.gcac_enable;
	enc_osg.st_osg_win.st_gcac.gcac_blk_width = p_stamp_info->attr.gcac_blk_width;
	enc_osg.st_osg_win.st_gcac.gcac_blk_height = p_stamp_info->attr.gcac_blk_height;

	enc_osg.st_osg_win.st_disp.mode = 0;
	enc_osg.st_osg_win.st_disp.bg_alpha = 0;
	enc_osg.st_osg_win.st_disp.fg_alpha = p_stamp_info->attr.alpha;
	enc_osg.st_osg_win.st_disp.pos_x = p_stamp_info->attr.position.x;
	enc_osg.st_osg_win.st_disp.pos_y = p_stamp_info->attr.position.y;
	if (p_stamp_info->enable) {
		enc_osg.st_osg_win.st_grap.type = get_internal_osg_type_idx(p_stamp_info->img.fmt);
		if (!(osg_support_mask & (1<<enc_osg.st_osg_win.st_grap.type))) {
			HD_VIDEOENC_ERR("path(%#x) use unsupport osg type(%#x)\n", path_id, p_stamp_info->img.fmt);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	} else {
		for (i=0; i<VDOENC_OSG_TYPE_NU; i++) {
			if (osg_support_mask & (1<<i)) {
				enc_osg.st_osg_win.st_grap.type = i;
				break;
			}
		}
	}

	enc_osg.st_osg_win.st_grap.width = p_stamp_info->img.dim.w;
	enc_osg.st_osg_win.st_grap.height = p_stamp_info->img.dim.h;
	enc_osg.st_osg_win.st_grap.pa_addr = p_stamp_info->img.p_addr;
	if (p_stamp_info->enable) {
		if (pif_address_ddr_sanity_check(enc_osg.st_osg_win.st_grap.pa_addr, enc_osg.st_osg_win.ddr_id) < 0) {
			HD_VIDEOENC_ERR("%s:Check out_pa(%#lx) ddrid(%d) error\n", __func__, enc_osg.st_osg_win.st_grap.pa_addr,
				enc_osg.st_osg_win.ddr_id);
			hd_ret = HD_ERR_PARAM;
			goto err_ext;
		}
	}

	switch (p_stamp_info->img.fmt) {	//line offset should align to 4 (word alignment)
		case HD_VIDEO_PXLFMT_ARGB1555:
		case HD_VIDEO_PXLFMT_ARGB4444:
		case HD_VIDEO_PXLFMT_RGB565:
			enc_osg.st_osg_win.st_grap.line_offset = ALIGN_CEIL_4(p_stamp_info->img.dim.w * 2);			//2 byte each pixel
			break;
		case HD_VIDEO_PXLFMT_ARGB8888:
			enc_osg.st_osg_win.st_grap.line_offset = p_stamp_info->img.dim.w * 4;						//4 byte each pixel
			break;
		case HD_VIDEO_PXLFMT_I1:
			enc_osg.st_osg_win.st_grap.line_offset = ALIGN_CEIL_32(p_stamp_info->img.dim.w) * 4;		//align to word first, 1 word = 4 byte
			break;
		case HD_VIDEO_PXLFMT_I2:
			enc_osg.st_osg_win.st_grap.line_offset = ALIGN_CEIL_32(p_stamp_info->img.dim.w * 2) * 4;
			break;
		case HD_VIDEO_PXLFMT_I4:
			enc_osg.st_osg_win.st_grap.line_offset = ALIGN_CEIL_32(p_stamp_info->img.dim.w * 4) * 4;
			break;
		default:
			enc_osg.st_osg_win.st_grap.line_offset = ALIGN_CEIL_4(p_stamp_info->img.dim.w * 2);
			break;
	}

	videoenc_info[dev_idx].port[outport_idx].osg_fd = enc_osg.fd;

	if (codec_type == HD_CODEC_TYPE_JPEG) {
		ioctl_ret = ioctl(enc_ioctl, JPEG_ENC_IOC_SET_OSG_WIN, &enc_osg); //notify fd invalid
	} else {
		ioctl_ret = ioctl(enc_ioctl, H26X_ENC_IOC_SET_OSG_WIN, &enc_osg); //notify fd invalid
	}
	if (ioctl_ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"H26X_ENC_IOC_SET_OSG_WIN\" return %d\n", ioctl_ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
err_ext:

	return hd_ret;

}

HD_RESULT videoenc_set_osg_win(HD_PATH_ID path_id, BOOL enable)
{

	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT outport_idx = VDOENC_OUTID(out_id);
	INT dev_idx = VDOENC_DEVID(self_id);

	if (outport_idx >= HD_VIDEOENC_MAX_OUT) {
		HD_VIDEOENC_ERR("Check enc osg out_id fail, out_id(%d)", outport_idx);
		hd_ret = HD_ERR_NOT_SUPPORT;
		return hd_ret;
	}
	if (osg_select[dev_idx][outport_idx]) {
		hd_ret = videoenc_set_external_osg_win(path_id, enable);
	} else {
		hd_ret = videoenc_set_internal_osg_win(path_id, enable);
	}
	return hd_ret;
}

HD_RESULT videoenc_set_extrnal_mask_mosaic(HD_PATH_ID path_id, BOOL enable)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	INT dev_idx = VDOENC_DEVID(self_id);
	INT outport_idx = VDOENC_OUTID(out_id);
	INT mask_idx = VDOENC_MASK_INID(in_id);
	struct multi_osg_mask_mosaic_win_info_t osg_mask_mosaic = {0};
	struct osg_mask_mosaic_win_t *mask_mosaic_win = NULL;
	HD_VDOENC_MASK_MOSAIC *p_enc_mask_mosaic = NULL;
	INT i;
	INT enable_num = VDOENC_MASK_NU;
	INT pos_idx;
	INT drv_idx = 0;
	INT ret = 0;
	unsigned int osg_fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_OSG, dev_idx, 0, utl_get_enc_osg_index(outport_idx));

	if (osg_ioctl <= 0) {
		HD_VIDEOENC_ERR("%s: Error, no osg ioctl supported!", __FUNCTION__);
		return HD_ERR_SYS;
	}

	p_enc_mask_mosaic = videoenc_info[dev_idx].port[outport_idx].enc_mask_mosaic;
	p_enc_mask_mosaic[mask_idx].enable = enable;


	// set IOCTL_OSG_SET_MULTI_MASK
	mask_mosaic_win = (struct osg_mask_mosaic_win_t *)PIF_CALLOC(1, enable_num * sizeof(struct osg_mask_mosaic_win_t));
	if (mask_mosaic_win == NULL) {
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	drv_idx = 0;
	for (i = 0; i < VDOENC_MASK_NU; i++) {
		struct osg_mask_mosaic_win_t *p_cur = &mask_mosaic_win[drv_idx];
		p_cur->mask_idx = drv_idx;
		p_cur->enable = p_enc_mask_mosaic[i].enable;
		if (p_enc_mask_mosaic[i].is_mask) {
			p_cur->mosaic_en = 0;
			p_cur->alpha = p_enc_mask_mosaic[i].mask_cfg.alpha;
			if (p_cur->alpha >= 256) {
				p_cur->alpha = 255;//k_flow_osg only support to 255
			}
			for (pos_idx = 0; pos_idx < 4; pos_idx++) {
				p_cur->st_pos[pos_idx].pos_x = p_enc_mask_mosaic[i].mask_cfg.position[pos_idx].x;
				p_cur->st_pos[pos_idx].pos_y = p_enc_mask_mosaic[i].mask_cfg.position[pos_idx].y;
			}
			if (p_enc_mask_mosaic[i].mask_cfg.type == HD_OSG_MASK_TYPE_SOLID) {
				p_cur->line_hit_opt = 0;
			} else {
				p_cur->line_hit_opt = 1;
			}
			p_cur->pal_sel = p_enc_mask_mosaic[i].mask_cfg.color;
		} else {
			p_cur->mosaic_en = 1;
			p_cur->alpha = p_enc_mask_mosaic[i].mosaic_cfg.alpha;
			if (p_cur->alpha >= 256) {
				p_cur->alpha = 255;
			}
			for (pos_idx = 0; pos_idx < 4; pos_idx++) {
				p_cur->st_pos[pos_idx].pos_x = p_enc_mask_mosaic[i].mosaic_cfg.position[pos_idx].x;
				p_cur->st_pos[pos_idx].pos_y = p_enc_mask_mosaic[i].mosaic_cfg.position[pos_idx].y;
			}
			if (p_enc_mask_mosaic[i].mosaic_cfg.type == HD_OSG_MASK_TYPE_SOLID) {
				p_cur->line_hit_opt = 0;
			} else {
				p_cur->line_hit_opt = 1;
			}
			p_cur->pal_sel = 0;
			if (p_enc_mask_mosaic[i].enable && p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_w != p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_h) {
				HD_VIDEOENC_ERR("Check mosaic blk_w(%d) != blk_h(%d)\n", p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_w,
						p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_h);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
			if (p_enc_mask_mosaic[i].enable && (p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_w != 8) && (p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_w != 16) \
			    && (p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_w != 32) && (p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_w != 64)) {
				HD_VIDEOENC_ERR("Not support mosaic blk(%d) \n", p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_w);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
			mask_mosaic_win[i].mosaic_blk_sz.w = p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_w;
			mask_mosaic_win[i].mosaic_blk_sz.h = p_enc_mask_mosaic[i].mosaic_cfg.mosaic_blk_h;
		}
		p_cur->mask_border_sz = 0;
		p_cur->align_type = OSG_ALIGN_TOP_LEFT;

		drv_idx++;
	}
	if (enable_num) {
		osg_mask_mosaic.fd = osg_fd;
		osg_mask_mosaic.num_osg_mask_mosaic = enable_num;
		osg_mask_mosaic.st_mask_mosaic_win = mask_mosaic_win;
		ret = ioctl(osg_ioctl, IOCTL_OSG_SET_MULTI_MASK, &osg_mask_mosaic); //notify fd invalid
		if (ret < 0) {
			HD_VIDEOENC_ERR("ioctl \"IOCTL_OSG_SET_MULTI_MASK\" return %d\n", ret);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}
err_ext:
	if (mask_mosaic_win)
		PIF_FREE(mask_mosaic_win);
	return hd_ret;

}

HD_RESULT videoenc_set_internal_mask_mosaic(HD_PATH_ID path_id, BOOL enable)
{

	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO in_id = HD_GET_IN(path_id);

	INT dev_idx = VDOENC_DEVID(self_id);
	INT outport_idx = VDOENC_OUTID(out_id);
	INT mask_idx = VDOENC_MASK_INID(in_id);

	FD_H26XENC_MASK_MOSAIC_BLOCK enc_mosaic = {0};
	FD_H26XENC_MASK_WIN enc_mask_win = {0};
	HD_VDOENC_MASK_MOSAIC *p_mask_info;
	INT h26x_fd;
	INT8 i;
	HD_RESULT hd_ret = HD_OK;
	INT ioctl_ret;

	if (mask_idx >= ENCODER_MAX_OSG_NU) {
		return hd_ret;
	}
	p_mask_info = &videoenc_info[dev_idx].port[outport_idx].enc_mask_mosaic[mask_idx];
	p_mask_info->enable = enable;

	h26x_fd = videoenc_info[dev_idx].priv[outport_idx].h26x_fd;

	if (p_mask_info->is_mask == 0) {
		//mosaic
		enc_mosaic.fd = h26x_fd;
		if (p_mask_info->enable == 0) {
			p_mask_info->mosaic_cfg.mosaic_blk_w = 8;
			p_mask_info->mosaic_cfg.mosaic_blk_h = 8;
		}
		enc_mosaic.st_mask_mosaic_block.mosaic_blk_w = p_mask_info->mosaic_cfg.mosaic_blk_w;
		enc_mosaic.st_mask_mosaic_block.mosaic_blk_h = p_mask_info->mosaic_cfg.mosaic_blk_h;
		ioctl_ret = ioctl(enc_ioctl, H26X_ENC_IOC_SET_MASK_MOSAIC_SIZE, &enc_mosaic); //notify fd invalid
		if (ioctl_ret < 0) {
			HD_VIDEOENC_ERR("ioctl \"H26X_ENC_IOC_SET_MASK_MOSAIC_SIZE\" return %d\n", ioctl_ret);
			hd_ret = HD_ERR_SYS;
		}
		enc_mask_win.fd = h26x_fd;
		enc_mask_win.st_mask_win.enable = enable;
		enc_mask_win.st_mask_win.mosaic_en = 1;
		enc_mask_win.st_mask_win.mask_idx = mask_idx;
		if (enable)  {//for user only open but not set and just close
			enc_mask_win.st_mask_win.alpha = p_mask_info->mosaic_cfg.alpha;
			if (enc_mask_win.st_mask_win.alpha >= 256) {
				enc_mask_win.st_mask_win.alpha = 255;//k_flow_osg only support to 255
			}
			enc_mask_win.st_mask_win.alpha = (255 - enc_mask_win.st_mask_win.alpha);//for kdrv_encode mosaic alpha define,0 is max value
			enc_mask_win.st_mask_win.pal_sel = 0;//fix to color 0 ,let all mosaic clolr are same

			switch (p_mask_info->mosaic_cfg.type) {
			case HD_OSG_MASK_TYPE_SOLID:
				enc_mask_win.st_mask_win.line_hit_opt = 0;
				break;
			case HD_OSG_MASK_TYPE_INVERSE:
				enc_mask_win.st_mask_win.line_hit_opt = 1;
				break;
			default:
				HD_VIDEOENC_ERR("unknown mask_cfg.type 0x%X\n", p_mask_info->mosaic_cfg.type);
				break;
			};
			for (i = 0; i < 4; i++) {
				if (p_mask_info->mosaic_cfg.position[i].x % vdoenc_hw_spec_info.h26x.mask_x_align != 0 ||
					p_mask_info->mosaic_cfg.position[i].y % vdoenc_hw_spec_info.h26x.mask_y_align != 0)
					HD_VIDEOENC_ERR("mosaic x(%d) or y(%d) of position %d should be aligned with %d or %d\n",
					p_mask_info->mosaic_cfg.position[i].x, p_mask_info->mosaic_cfg.position[i].y, i,
					vdoenc_hw_spec_info.h26x.mask_x_align, vdoenc_hw_spec_info.h26x.mask_y_align);

				enc_mask_win.st_mask_win.st_pos[i].pos_x = p_mask_info->mosaic_cfg.position[i].x;
				enc_mask_win.st_mask_win.st_pos[i].pos_y = p_mask_info->mosaic_cfg.position[i].y;
			}
		}

		ioctl_ret = ioctl(enc_ioctl, H26X_ENC_IOC_SET_MASK_WIN, &enc_mask_win); //notify fd invalid
		if (ioctl_ret < 0) {
			HD_VIDEOENC_ERR("ioctl \"H26X_ENC_IOC_SET_MASK_WIN\" return %d\n", ioctl_ret);
			hd_ret = HD_ERR_SYS;
		}
	} else if (p_mask_info->is_mask == 1) {
		//normal mask
		enc_mask_win.fd = h26x_fd;
		enc_mask_win.st_mask_win.enable = enable;
		enc_mask_win.st_mask_win.mosaic_en = 0;
		enc_mask_win.st_mask_win.mask_idx = mask_idx;
		if (enable) {
			enc_mask_win.st_mask_win.alpha = p_mask_info->mask_cfg.alpha;
			if (enc_mask_win.st_mask_win.alpha >= 256) {
				enc_mask_win.st_mask_win.alpha = 255;//k_flow_osg only support to 255
			}
			enc_mask_win.st_mask_win.pal_sel = (p_mask_info->mask_cfg.color % 8);//enc mask mask pal only support :0~8

			switch (p_mask_info->mask_cfg.type) {
			case HD_OSG_MASK_TYPE_SOLID:
				enc_mask_win.st_mask_win.line_hit_opt = 0;
				break;
			case HD_OSG_MASK_TYPE_HOLLOW:
				enc_mask_win.st_mask_win.line_hit_opt = 2;
				break;
			case HD_OSG_MASK_TYPE_INVERSE:
				enc_mask_win.st_mask_win.line_hit_opt = 1;
				break;
			default:
				HD_VIDEOENC_ERR("unknown mask_cfg.type 0x%X\n", p_mask_info->mask_cfg.type);
				break;
			};

			for (i = 0; i < 4; i++) {
				if (p_mask_info->mask_cfg.position[i].x % vdoenc_hw_spec_info.h26x.mask_x_align != 0 ||
					p_mask_info->mask_cfg.position[i].y % vdoenc_hw_spec_info.h26x.mask_y_align != 0)
					HD_VIDEOENC_ERR("mask x(%d) or (%d) y of position %d should be aligned with %d or %d\n",
					p_mask_info->mask_cfg.position[i].x, p_mask_info->mask_cfg.position[i].y, i,
					vdoenc_hw_spec_info.h26x.mask_x_align, vdoenc_hw_spec_info.h26x.mask_y_align);

				enc_mask_win.st_mask_win.st_pos[i].pos_x = p_mask_info->mask_cfg.position[i].x;
				enc_mask_win.st_mask_win.st_pos[i].pos_y = p_mask_info->mask_cfg.position[i].y;
			}
		}
		ioctl_ret = ioctl(enc_ioctl, H26X_ENC_IOC_SET_MASK_WIN, &enc_mask_win); //notify fd invalid
		if (ioctl_ret < 0) {
			HD_VIDEOENC_ERR("ioctl \"H26X_ENC_IOC_SET_MASK_WIN\" return %d\n", ioctl_ret);
			hd_ret = HD_ERR_SYS;
		}
	} else {
		HD_VIDEOENC_ERR("Only support mask/mosaic,is_mask=%d", p_mask_info->is_mask);
	}

	return hd_ret;

}

HD_RESULT videoenc_set_mask_mosaic(HD_PATH_ID path_id, BOOL enable)
{

	HD_RESULT hd_ret = HD_OK;
	HD_IO out_id = HD_GET_OUT(path_id);
	INT outport_idx = VDOENC_OUTID(out_id);
	HD_DAL self_id = HD_GET_DEV(path_id);
	INT dev_idx = VDOENC_DEVID(self_id);

	if (outport_idx >= HD_VIDEOENC_MAX_OUT) {
		HD_VIDEOENC_ERR("Check enc osg out_id fail, out_id(%d)", outport_idx);
		hd_ret = HD_ERR_NOT_SUPPORT;
		return hd_ret;
	}
	if (osg_select[dev_idx][outport_idx]) {
		hd_ret = videoenc_set_extrnal_mask_mosaic(path_id, enable);
	} else {
		hd_ret = videoenc_set_internal_mask_mosaic(path_id, enable);
	}
	return hd_ret;
}

HD_RESULT videoenc_get_mask_palette(HD_DAL self_id, HD_IO in_id, HD_OSG_PALETTE_TBL *p_palette_tbl)
{

	FD_H26XENC_MASK_PALETTE_TABLE palette_tbl = {0};

	INT dev_idx = VDOENC_DEVID(self_id);
	INT inport_idx = VDOENC_INID(in_id);
	VDOENC_INFO *p_enc_info = &videoenc_info[dev_idx];
	VDOENC_INFO_PRIV *p_enc_priv = &p_enc_info->priv[inport_idx];

	HD_RESULT hd_ret = HD_OK;
	INT ret;

	if (enc_ioctl < 0) {
		HD_VIDEOENC_ERR("%s: Error, enc_ioctl is not init!\n", __FUNCTION__);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	if (sizeof(palette_tbl.st_mask_pal) != sizeof(HD_OSG_PALETTE_TBL)) {
		HD_VIDEOENC_ERR("%s: Error, palette size not match\n", __FUNCTION__);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	palette_tbl.fd = p_enc_priv->h26x_fd;

	ret = ioctl(enc_ioctl, H26X_ENC_IOC_GET_MASK_PAL, &palette_tbl);
	if (ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"H26X_ENC_IOC_GET_MASK_PAL\" return %d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	memcpy(p_palette_tbl, &palette_tbl.st_mask_pal, sizeof(HD_OSG_PALETTE_TBL));

err_ext:

	return hd_ret;

}

HD_RESULT videoenc_set_mask_palette(HD_DAL self_id, HD_IO in_id, const HD_OSG_PALETTE_TBL *p_palette_tbl)
{

	FD_H26XENC_MASK_PALETTE_TABLE palette_tbl = {0};

	INT dev_idx = VDOENC_DEVID(self_id);
	INT inport_idx = VDOENC_INID(in_id);
	VDOENC_INFO *p_enc_info = &videoenc_info[dev_idx];
	VDOENC_INFO_PRIV *p_enc_priv = &p_enc_info->priv[inport_idx];

	HD_RESULT hd_ret = HD_OK;
	INT ret;

	if (enc_ioctl < 0) {
		HD_VIDEOENC_ERR("%s: Error, enc_ioctl is not init!\n", __FUNCTION__);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	if (sizeof(palette_tbl.st_mask_pal) != sizeof(HD_OSG_PALETTE_TBL)) {
		HD_VIDEOENC_ERR("%s: Error, palette size not match\n", __FUNCTION__);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	palette_tbl.fd = p_enc_priv->h26x_fd;
	memcpy(&palette_tbl.st_mask_pal, p_palette_tbl, sizeof(HD_OSG_PALETTE_TBL));

	ret = ioctl(enc_ioctl, H26X_ENC_IOC_SET_MASK_PAL, &palette_tbl);
	if (ret < 0) {
		HD_VIDEOENC_ERR("ioctl \"H26X_ENC_IOC_SET_MASK_PAL\" return %d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
err_ext:

	return hd_ret;
}
HD_RESULT videoenc_get_status_p(HD_DAL self_id, HD_IO in_id, HD_VIDEOENC_STATUS *p_status)
{
	HD_RESULT hd_ret = HD_OK;
	INT dev_idx = VDOENC_DEVID(self_id);
	INT inport_idx = VDOENC_INID(in_id);
	VDOENC_INFO *p_enc_info = &videoenc_info[dev_idx];
	VDOENC_INFO_PRIV *p_enc_priv = &p_enc_info->priv[inport_idx];
	HD_VIDEO_CODEC codec_type = p_enc_info->port[inport_idx].enc_out_param.codec_type;
	INT fd = codec_type == HD_CODEC_TYPE_JPEG?p_enc_priv->jpeg_fd:p_enc_priv->h26x_fd;
	INT ret;
	usr_proc_queue_status_t status;
	UINT dti_buf_cnt;

	if (is_bind(self_id, inport_idx, 1) == HD_OK) {
		pif_get_left_frame_count(fd,
					 (UINT *) & (p_status->left_frames),
					 (UINT *) & (p_status->done_frames),
					 &dti_buf_cnt);
	} else {
		status.usr_proc_fd.fd = fd;
		status.queue_handle = p_enc_priv->queue_handle;
		status.left_frames = 0;
		status.done_frames = 0;
		if (ioctl(vpd_fd, USR_PROC_QUERY_STATUS, &status) < 0) {
			ret = errno * -1;
			HD_VIDEOENC_ERR("ioctl \"USR_PROC_QUERY_STATUS\" fail(%d)\n", ret);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		p_status->left_frames = status.left_frames;
		p_status->done_frames = status.done_frames;
	}

exit:
	return hd_ret;
}
