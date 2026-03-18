/**
 * @file videoout.c
 * @brief type definition of API.
 * @author PSW
 * @date in the year 2018
 */

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include "hdal.h"
#include "vpd_ioctl.h"
#include "trig_ioctl.h"
#include "videocap.h"
#include "videoout.h"
#include "hdal.h"
#include "cif_common.h"
#include "vg_common.h"
#include "dif.h"
#include "vcap316/vcap_ioctl.h"
#include "pif.h"
#include "utl.h"
#include "pif_ioctl.h"


/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define CAP_CHIP_OFFSET      24
#define CAP_CHIP_BITMAP      0xFF000000
#define CAP_ENGINE_OFFSET    8
#define CAP_ENGINE_BITMAP    0x00FFFF00
#define CAP_CHANNEL_OFFSET   0
#define CAP_CHANNEL_BITMAP   0x000000FF

#define CAP_DEV_ID_CHIP(id)      ((id & CAP_CHIP_BITMAP) >> CAP_CHIP_OFFSET)
#define CAP_DEV_ID_ENGINE(id)    ((id & CAP_ENGINE_BITMAP) >> CAP_ENGINE_OFFSET)
#define CAP_DEV_ID_CHANNEL(id)   ((id & CAP_CHANNEL_BITMAP) >> CAP_CHANNEL_OFFSET)
#define CAP_DEV_ID(chip, engine, channel) \
	(((chip) << CAP_CHIP_OFFSET) | ((engine) << CAP_ENGINE_OFFSET) | ((channel) & CAP_CHANNEL_BITMAP))
#define PAGE_SHIFT                  12
#define PAGE_SIZE                   (1 << PAGE_SHIFT)
#define DRC_MODE_MAX                3
/*-----------------------------------------------------------------------------*/
/* External Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern UINT32 videocap_max_device_nr;
extern UINT32 videocap_max_device_out;
extern UINT32 videocap_max_device_in;
extern vpd_sys_info_t platform_sys_Info;
extern HD_CAP_INFO g_video_cap_info[MAX_CAP_DEV];

/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them
/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
CAP_INTERNAL_PARAM cap_dev_cfg[MAX_CAP_DEV];
HD_VIDEO_FRAME     cap_push_in = {0};
static INT vcap_ioctl = 0;

struct vcap_ioc_hw_limit_t videocap_hw_spec_info;
struct vcap_ioc_hw_ability_t videocap_hw_ability;
//FIXME: need to put this function declaration somewhere
extern int pif_alloc_user_fd_for_hdal(vpd_em_type_t type, int chip, int engine, int minor);
extern void *pif_alloc_video_buffer_for_hdal(int size, int ddr_id, char pool_name[], int alloc_tag);
extern int pif_free_video_buffer_for_hdal(void *pa, int ddr_id, int alloc_tag);
//extern unsigned int pif_create_queue_for_hdal(void);
extern uintptr_t pif_create_queue_for_hdal(void);
//extern int pif_destroy_queue_for_hdal(unsigned int queue_handle);
extern int pif_destroy_queue_for_hdal(uintptr_t queue_handle);
extern int vpd_fd;
HD_RESULT videocap_check_vcap_validtype(vpd_buf_type_t vpd_buf_type, INT *valid);
vpd_flip_t videocap_get_flip_param(HD_VIDEO_DIR dir);

static UINT32 get_cap_dev_number_p(void)
{
	UINT32 i, max_dev_number = 0;

	for (i = 0; i < MAX_CAP_DEV; i++) {
		if (platform_sys_Info.cap_info[i].vcapch < 0) { //A continuous list(ch) in cap_info. no break.
			continue;
		}
		max_dev_number = i;
	}
	return (max_dev_number);
}

static UINT32 get_cap_max_in_count_p(void)
{
	return 1;
}

static UINT32 get_cap_max_out_count_p(void)
{
	UINT32 i, max_out_count = 0;

	for (i = 0; i < MAX_CAP_DEV; i++) {
		if (platform_sys_Info.cap_info[i].vcapch >= 0) { //A continuous list(ch) in cap_info. no break.
			max_out_count = platform_sys_Info.cap_info[i].num_of_path;
			break;
		}
	}
	return max_out_count;
}

static INT validate_cap_out_dim(UINT32 out_w, UINT32 out_h, HD_VIDEO_PXLFMT fmt)
{
	UINT32 w_min, h_min, w_max, h_max;
	if (fmt == HD_VIDEO_PXLFMT_YUV420_NVX3) {
		w_min = videocap_hw_spec_info.dma.yuv_sce_w_min[0];
		w_max = videocap_hw_spec_info.dma.yuv_sce_w_max[0];
		h_min = videocap_hw_spec_info.dma.yuv_sce_h_min[0];
		h_max = videocap_hw_spec_info.dma.yuv_sce_h_max[0];
	} else if ((fmt == HD_VIDEO_PXLFMT_YUV422) ||
		   (fmt == HD_VIDEO_PXLFMT_YUV422_ONE)) {
		w_min = videocap_hw_spec_info.dma.yuv_w_min[1];
		w_max = videocap_hw_spec_info.dma.yuv_w_max[1];
		h_min = videocap_hw_spec_info.dma.yuv_h_min[1];
		h_max = videocap_hw_spec_info.dma.yuv_h_max[1];
	} else if (fmt == HD_VIDEO_PXLFMT_YUV422_NVX3) {
		w_min = videocap_hw_spec_info.dma.yuv_sce_w_min[1];
		w_max = videocap_hw_spec_info.dma.yuv_sce_w_max[1];
		h_min = videocap_hw_spec_info.dma.yuv_sce_h_min[1];
		h_max = videocap_hw_spec_info.dma.yuv_sce_h_max[1];
	} else if (fmt == HD_VIDEO_PXLFMT_YUV420_MB) {
		w_min = videocap_hw_spec_info.dma.yuv_mb_w_min[0];
		w_max = videocap_hw_spec_info.dma.yuv_mb_w_max[0];
		h_min = videocap_hw_spec_info.dma.yuv_mb_h_min[0];
		h_max = videocap_hw_spec_info.dma.yuv_mb_h_max[0];
	} else {// if (fmt == HD_VIDEO_PXLFMT_YUV420) {
		w_min = videocap_hw_spec_info.dma.yuv_w_min[0];
		w_max = videocap_hw_spec_info.dma.yuv_w_max[0];
		h_min = videocap_hw_spec_info.dma.yuv_h_min[0];
		h_max = videocap_hw_spec_info.dma.yuv_h_max[0];
	}
	if (out_w < w_min || out_w > w_max ||
	    out_h < h_min || out_h > h_max) {
		goto fail_exit;
	}

	return TRUE;

fail_exit:
	HD_VIDEOCAPTURE_ERR("VCAP out error: w=%d, w_min=%d, w_max=%d, h=%d, h_min=%d, h_max=%d.\n", out_w, w_min, w_max, out_h, h_min, h_max);

	return FALSE;
}

static INT validate_cap_crop_dim(UINT32 out_w, UINT32 out_h)
{
	UINT32 w_min, h_min, w_max, h_max;

	w_min = videocap_hw_spec_info.sc.w_min;
	w_max = videocap_hw_spec_info.sc.w_max;
	h_min = videocap_hw_spec_info.sc.h_min;
	h_max = videocap_hw_spec_info.sc.h_max;

	if (out_w < w_min || out_w > w_max ||
	    out_h < h_min || out_h > h_max) {
		goto fail_exit;
	}

	return TRUE;

fail_exit:
	HD_VIDEOCAPTURE_ERR("VCAP crop error: w=%d, w_min=%d, w_max=%d, h=%d, h_min=%d, h_max=%d.\n", out_w, w_min, w_max, out_h, h_min, h_max);

	return FALSE;
}

HD_RESULT validate_cap_dim_wh_align(HD_DIM in_dim, HD_VIDEO_PXLFMT in_pxlfmt, HD_DIM *p_out_dim)
{
	return validate_cap_dim_bg_align_dir(in_dim, in_pxlfmt, p_out_dim, 0);
}

HD_RESULT validate_cap_dim_bg_align_dir(HD_DIM in_dim, HD_VIDEO_PXLFMT in_pxlfmt, HD_DIM *p_out_dim, unsigned int is_up)
{
	UINT32 align_w, align_h;
	if (p_out_dim == NULL) {
		GMLIB_ERROR_P("NULL p_out_dim\n");
		goto err_exit;
	}

	switch (in_pxlfmt) {
	case HD_VIDEO_PXLFMT_YUV422_ONE:
		align_w = videocap_hw_spec_info.dma.yuv_w_align[1];
		align_h = videocap_hw_spec_info.dma.yuv_h_align[1];
		break;
	case HD_VIDEO_PXLFMT_YUV420:
		align_w = videocap_hw_spec_info.dma.yuv_w_align[0];
		align_h = videocap_hw_spec_info.dma.yuv_h_align[0];
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX3:
		align_w = videocap_hw_spec_info.dma.yuv_sce_w_align[0];
		align_h = videocap_hw_spec_info.dma.yuv_sce_h_align[0];
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB:
		align_w = videocap_hw_spec_info.dma.yuv_mb_w_align[0];
		align_h = videocap_hw_spec_info.dma.yuv_mb_h_align[0];
		break;
	case HD_VIDEO_PXLFMT_YUV422_NVX3:
		align_w = videocap_hw_spec_info.dma.yuv_sce_w_align[1];
		align_h = videocap_hw_spec_info.dma.yuv_sce_h_align[1];
		break;
	default:
		GMLIB_ERROR_P("Not support for input pxlfmt(%#x)\n", in_pxlfmt);
		goto err_exit;
	}
	if (is_up) {
		p_out_dim->w = ALIGN_CEIL(in_dim.w, align_w);
		p_out_dim->h = ALIGN_CEIL(in_dim.h, align_h);
	} else {
		p_out_dim->w = ALIGN_FLOOR(in_dim.w, align_w);
		p_out_dim->h = ALIGN_FLOOR(in_dim.h, align_h);
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT validate_cap_dim_wh_align_odd(HD_DIM in_dim, HD_VIDEO_PXLFMT in_pxlfmt, HD_DIM *p_out_dim, unsigned int is_up)
{
	UINT32 align_w, align_h;
	if (p_out_dim == NULL) {
		GMLIB_ERROR_P("NULL p_out_dim\n");
		goto err_exit;
	}

	switch (in_pxlfmt) {
	case HD_VIDEO_PXLFMT_YUV422_ONE:
	case HD_VIDEO_PXLFMT_YUV422_NVX3:
		align_w = videocap_hw_spec_info.dma.yuv_w_align[1];
		align_h = videocap_hw_spec_info.dma.yuv_h_align[1];
		break;
	case HD_VIDEO_PXLFMT_YUV420:
	case HD_VIDEO_PXLFMT_YUV420_NVX3:
	case HD_VIDEO_PXLFMT_YUV420_MB:
		align_w = videocap_hw_spec_info.dma.yuv_w_align[0];
		align_h = videocap_hw_spec_info.dma.yuv_h_align[0];
		break;
	default:
		GMLIB_ERROR_P("Not support for input pxlfmt(%#x)\n", in_pxlfmt);
		goto err_exit;
	}
	if (is_up) {
		p_out_dim->w = ALIGN_CEIL(in_dim.w, align_w);
		p_out_dim->h = ALIGN_CEIL(in_dim.h, align_h);
	} else {
		p_out_dim->w = ALIGN_FLOOR(in_dim.w, align_w);
		p_out_dim->h = ALIGN_FLOOR(in_dim.h, align_h);
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT videocap_vi_trans_src(HD_VIDEOCAP_VI_SRC hd_src, VCAP_IOC_VI_SRC_T *vi_src)
{
	HD_RESULT ret = HD_ERR_SYS;
	unsigned int valid = 0;
	if (hd_src <= HD_VIDEOCAP_VI_SRC_XCAP7) {
		*vi_src = hd_src;
		return HD_OK;
	}
	if ((hd_src >= HD_VIDEOCAP_VI_SRC_CSI0)&&(hd_src <= HD_VIDEOCAP_VI_SRC_CSI7)) {
		*vi_src = hd_src + VCAP_IOC_VI_SRC_CSI0 - HD_VIDEOCAP_VI_SRC_CSI0;
		printf("videocap_vi_trans_src in %d, out %d\n", hd_src, *vi_src);
		return HD_OK;
	}
	//other case
	switch (hd_src) {
	case HD_VIDEOCAP_VI_SRC_XCAP0_1:
		valid = 1;
		*vi_src = VCAP_IOC_VI_SRC_XCAP0_1;
		break;
	case HD_VIDEOCAP_VI_SRC_XCAP2_3:
		valid = 1;
		*vi_src = VCAP_IOC_VI_SRC_XCAP2_3;
		break;
	case HD_VIDEOCAP_VI_SRC_XCAP4_5:
		valid = 1;
		*vi_src = VCAP_IOC_VI_SRC_XCAP4_5;
		break;
	case HD_VIDEOCAP_VI_SRC_XCAP6_7:
		valid = 1;
		*vi_src = VCAP_IOC_VI_SRC_XCAP6_7;
		break;
	default:
		break;
	}
	if (valid) {
		return HD_OK;
	}
	//other wrong
	*vi_src = VCAP_IOC_VI_SRC_MAX;
	printf("hd_src %d wrong \n", hd_src);
	return ret;
}

HD_RESULT videocap_vi_trans_fmt(HD_VIDEOCAP_VI_FMT hd_fmt, VCAP_IOC_VI_FMT_T *vi_fmt)
{
	HD_RESULT ret = HD_ERR_SYS;
	unsigned int valid = 0;
	switch (hd_fmt) {
		case HD_VIDEOCAP_VI_FMT_BT656:
			valid = 1;
			*vi_fmt = VCAP_IOC_VI_FMT_BT656;
			break;
		case HD_VIDEOCAP_VI_FMT_BT1120:
			valid = 1;
			*vi_fmt = VCAP_IOC_VI_FMT_BT1120;
			break;
		case HD_VIDEOCAP_VI_FMT_BT656DH:
		default:
			valid = 0;
			*vi_fmt = 0;
			break;
		case HD_VIDEOCAP_VI_FMT_CSI_16BIT:
			valid = 1;
			*vi_fmt = VCAP_IOC_VI_FMT_CSI;
			break;

	}
	if (valid) {
			return HD_OK;
	}
	printf("hd_fmt %d wrong \n", hd_fmt);
	return ret;
}

HD_RESULT videocap_vi_register(HD_VIDEOCAP_VI *p_vcap_vi)
{
	INT ioc_ret, i;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_vi_t vcap_vi;
	VCAP_IOC_VI_SRC_T vi_src;
	VCAP_IOC_VI_FMT_T vi_fmt;

	/* deregister all vcap vi to force clear previous setting */
	vcap_vi.version = VCAP_IOC_VERSION;
	vcap_vi.chip    = p_vcap_vi->chip;       ///< GM8220 support 8   VI for each chip, GM8296 support 4   VI for each chip
	vcap_vi.vcap    = p_vcap_vi->vcap;       ///< GM8220 support 2 VCAP for each chip, GM8296 support 1 VCAP for each chip
	vcap_vi.vi      = p_vcap_vi->vi;         ///< GM8220 and GM8296 support 4 VI for each VCAP

	ret =  videocap_vi_trans_src(p_vcap_vi->global.src, &vi_src);
	vcap_vi.global.src = vi_src;
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("videocap_vi_register: p_vcap_vi->global.src wrong type %u.\n", p_vcap_vi->global.src);
		goto ext;
	}
	ret = videocap_vi_trans_fmt(p_vcap_vi->global.format, &vi_fmt);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("videocap_vi_register: p_vcap_vi->global.src wrong fmt %u.\n", p_vcap_vi->global.format);
		goto ext;
	}
	vcap_vi.global.format = vi_fmt;
	vcap_vi.global.tdm     = p_vcap_vi->global.tdm;
	vcap_vi.global.id_extract     = p_vcap_vi->global.id_extract;
	vcap_vi.global.latch_edge     = p_vcap_vi->global.latch_edge;

	for (i = 0; i < VCAP_IOC_VI_VPORT_MAX; i++) {
		vcap_vi.vport[i].clk_inv   = p_vcap_vi->vport[i].clk_inv;
		vcap_vi.vport[i].clk_dly   = p_vcap_vi->vport[i].clk_dly;
		vcap_vi.vport[i].clk_pdly  = p_vcap_vi->vport[i].clk_pdly;
		vcap_vi.vport[i].clk_pin   = p_vcap_vi->vport[i].clk_pin;
		vcap_vi.vport[i].data_swap = p_vcap_vi->vport[i].data_swap;
	}

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_REGISTER, &vcap_vi);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VI_REGISTER ioctl...error\n");
		HD_VIDEOCAPTURE_ERR("chip: %d, vcap: %d, vi: %d, src: %d, fmt: %d \n", p_vcap_vi->chip, p_vcap_vi->vcap, p_vcap_vi->vi, p_vcap_vi->global.src, p_vcap_vi->global.format);
		HD_VIDEOCAPTURE_ERR("vcap src: %d, format: %d, tdm: %d, extract: %d, latch_edge: %d\n", vcap_vi.global.src, vcap_vi.global.format, vcap_vi.global.tdm, vcap_vi.global.id_extract, vcap_vi.global.latch_edge);
		ret = HD_ERR_SYS;
		goto ext;
	}

ext:
	return ret;
}

HD_RESULT videocap_vi_deregister(HD_VIDEOCAP_VI_ID *p_vcap_vi_id)
{
	INT ioc_ret;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_vi_id_t vcap_vi_id;

	/* deregister all vcap vi to force clear previous setting */
	vcap_vi_id.version = VCAP_IOC_VERSION;
	vcap_vi_id.chip    = p_vcap_vi_id->chip;       ///< GM8220 support 8   VI for each chip, GM8296 support 4   VI for each chip
	vcap_vi_id.vcap    = p_vcap_vi_id->vcap;       ///< GM8220 support 2 VCAP for each chip, GM8296 support 1 VCAP for each chip
	vcap_vi_id.vi      = p_vcap_vi_id->vi;         ///< GM8220 and GM8296 support 4 VI for each VCAP

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_DEREGISTER, &vcap_vi_id);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VI_DEREGISTER ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}

ext:
	return ret;
}

HD_RESULT videocap_host_uninit(HD_VIDEOCAP_HOST_ID *p_vcap_host_id)
{
	INT ioc_ret;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_host_id_t vcap_host_id;

	/* uninit vcap host to force clear previous setting */
	vcap_host_id.version = VCAP_IOC_VERSION;
	vcap_host_id.host    = p_vcap_host_id->host;
	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_HOST_UNINIT, &vcap_host_id);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_HOST_UNINIT ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}

ext:
	return ret;
}

HD_RESULT videocap_host_init(HD_VIDEOCAP_HOST *p_vcap_host)
{
	INT ioc_ret;
	UINT32 i;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_host_t vcap_host;

	/* vcap host init, to specify vcap system vi usage and prepare requirement memory */
	memset(&vcap_host, 0, sizeof(vcap_host));
	vcap_host.version         = VCAP_IOC_VERSION;
	vcap_host.host            = p_vcap_host->host;
	vcap_host.nr_of_vi        = p_vcap_host->nr_of_vi;

	for (i = 0; i < p_vcap_host->nr_of_vi; i++) {
		if (i >= VCAP_IOC_VI_MAX)
			break;
		vcap_host.vi[i].chip = p_vcap_host->vi[i].chip;   ///< GM8220 support 8   VI for each chip, GM8296 support 4   VI for each chip
		vcap_host.vi[i].vcap = p_vcap_host->vi[i].vcap;   ///< GM8220 support 2 VCAP for each chip, GM8296 support 1 VCAP for each chip
		vcap_host.vi[i].vi   = p_vcap_host->vi[i].vi;   ///< GM8220 and GM8296 support 4 VI for each VCAP
		vcap_host.vi[i].mode = p_vcap_host->vi[i].mode;   ///< GM8220 and GM8296 support 4 VI for each VCAP
	}

	vcap_host.md.enable       = p_vcap_host->md.enable;
	vcap_host.md.mb_x_num_max = p_vcap_host->md.mb_x_num_max;
	vcap_host.md.mb_y_num_max = p_vcap_host->md.mb_y_num_max;
	vcap_host.md.buf_src      = p_vcap_host->md.buf_src;

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_HOST_INIT, &vcap_host);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_HOST_INIT ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}

ext:
	return ret;
}

HD_RESULT videocap_vi_vport_get_param(HD_VIDEOCAP_VI_VPORT *p_vi_vport)
{
	INT ioc_ret;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_vi_vport_param_t vi_vport = {0};

	/* VCH VPORT */
	vi_vport.version = VCAP_IOC_VERSION;
	vi_vport.chip    = p_vi_vport->chip;
	vi_vport.vcap    = p_vi_vport->vcap;
	vi_vport.vi      = p_vi_vport->vi;
	vi_vport.vport   = p_vi_vport->vport;
	vi_vport.pid     = p_vi_vport->pid;

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_VPORT_GET_PARAM, &vi_vport);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VI_VPORT_GET_PARAM ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}
	p_vi_vport->value = vi_vport.value;
ext:
	return ret;
}

HD_RESULT videocap_vi_vport_set_param(HD_VIDEOCAP_VI_VPORT *p_vi_vport)
{
	INT ioc_ret;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_vi_vport_param_t vi_vport;

	/* VCH VPORT */
	vi_vport.version = VCAP_IOC_VERSION;
	vi_vport.chip    = p_vi_vport->chip;
	vi_vport.vcap    = p_vi_vport->vcap;
	vi_vport.vi      = p_vi_vport->vi;
	vi_vport.vport   = p_vi_vport->vport;
	vi_vport.pid     = p_vi_vport->pid;
	vi_vport.value   = p_vi_vport->value;

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_VPORT_SET_PARAM, &vi_vport);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VI_VPORT_SET_PARAM ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}

ext:
	return ret;
}

HD_RESULT videocap_vi_ch_get_param(HD_VIDEOCAP_VI_CH_PARAM *p_ch_param)
{
	INT ioc_ret;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_vi_ch_param_t ch_param = {0};

	/* VCH PARAM */
	ch_param.version = VCAP_IOC_VERSION;
	ch_param.chip    = p_ch_param->chip;
	ch_param.vcap    = p_ch_param->vcap;
	ch_param.vi      = p_ch_param->vi;
	ch_param.ch      = p_ch_param->ch;
	ch_param.pid     = p_ch_param->pid;

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_CH_GET_PARAM, &ch_param);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VI_CH_SET_PARAM ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}
	p_ch_param->value = ch_param.value;
ext:
	return ret;
}

HD_RESULT videocap_vi_ch_set_param(HD_VIDEOCAP_VI_CH_PARAM *p_ch_param)
{
	INT ioc_ret;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_vi_ch_param_t ch_param;

	/* VCH PARAM */
	ch_param.version = VCAP_IOC_VERSION;
	ch_param.chip    = p_ch_param->chip;
	ch_param.vcap    = p_ch_param->vcap;
	ch_param.vi      = p_ch_param->vi;
	ch_param.ch      = p_ch_param->ch;
	ch_param.value   = p_ch_param->value;
	ch_param.pid     = p_ch_param->pid;

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_CH_SET_PARAM, &ch_param);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VI_CH_SET_PARAM ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}

ext:
	return ret;
}

HD_RESULT videocap_vi_ch_get_norm(HD_VIDEOCAP_VI_CH_NORM *p_ch_norm)
{
	INT ioc_ret;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_vi_ch_norm_t ch_norm;

	/* VCH PARAM */
	memset(&ch_norm, 0, sizeof(ch_norm));
	ch_norm.version = VCAP_IOC_VERSION;
	ch_norm.chip = p_ch_norm->chip;
	ch_norm.vcap = p_ch_norm->vcap;
	ch_norm.vi = p_ch_norm->vi;
	ch_norm.ch = p_ch_norm->ch;

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_CH_GET_NORM, &ch_norm);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VI_CH_SET_NORM ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}
	p_ch_norm->cap_width = ch_norm.cap_width;
	p_ch_norm->cap_height = ch_norm.cap_height;
	p_ch_norm->org_width = ch_norm.org_width;
	p_ch_norm->org_height = ch_norm.org_height;
	p_ch_norm->fps = ch_norm.fps;
	p_ch_norm->format = ch_norm.format;
	p_ch_norm->prog = ch_norm.prog;
	p_ch_norm->data_rate = ch_norm.data_rate;
	p_ch_norm->data_latch = ch_norm.data_latch;
	p_ch_norm->horiz_dup = ch_norm.horiz_dup;
	p_ch_norm->reserved[0] = ch_norm.data_src;

ext:
	return ret;
}

HD_RESULT videocap_get_vi(HD_VIDEOCAP_VI *p_vcap_vi)
{
	INT ioc_ret, i;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_vi_t vcap_vi;

	/* VCAP VI */
	memset(&vcap_vi, 0, sizeof(vcap_vi));
	vcap_vi.version = VCAP_IOC_VERSION;
	vcap_vi.chip = p_vcap_vi->chip;
	vcap_vi.vcap = p_vcap_vi->vcap;
	vcap_vi.vi = p_vcap_vi->vi;

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_GET_INFO, &vcap_vi);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VI_GET_INFO ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}
	p_vcap_vi->global.src            = vcap_vi.global.src;
	p_vcap_vi->global.format         = vcap_vi.global.format;
	p_vcap_vi->global.tdm            = vcap_vi.global.tdm;
	p_vcap_vi->global.id_extract     = vcap_vi.global.id_extract;
	p_vcap_vi->global.latch_edge     = vcap_vi.global.latch_edge;
	for (i = 0; i < VCAP_IOC_VI_VPORT_MAX; i++) {
		p_vcap_vi->vport[i].clk_inv   = vcap_vi.vport[i].clk_inv;
		p_vcap_vi->vport[i].clk_dly   = vcap_vi.vport[i].clk_dly;
		p_vcap_vi->vport[i].clk_pdly  = vcap_vi.vport[i].clk_pdly;
		p_vcap_vi->vport[i].clk_pin   = vcap_vi.vport[i].clk_pin;
		p_vcap_vi->vport[i].data_swap = vcap_vi.vport[i].data_swap;
	}

ext:
	return ret;
}

HD_RESULT videocap_vi_ch_set_norm(HD_VIDEOCAP_VI_CH_NORM *p_ch_norm)
{
	INT ioc_ret;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_vi_ch_norm_t ch_norm = {0};


	ch_norm.version = VCAP_IOC_VERSION;
	ch_norm.chip = p_ch_norm->chip;
	ch_norm.vcap = p_ch_norm->vcap;
	ch_norm.vi = p_ch_norm->vi;
	ch_norm.ch = p_ch_norm->ch;
	//HD_VIDEOCAP_VI_CH_NORM has no data_src, so need to get ch_norm.data_src
	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_CH_GET_NORM, &ch_norm);

	/* VCH PARAM */
	ch_norm.version = VCAP_IOC_VERSION;
	ch_norm.chip = p_ch_norm->chip;
	ch_norm.vcap = p_ch_norm->vcap;
	ch_norm.vi = p_ch_norm->vi;
	ch_norm.ch = p_ch_norm->ch;
	ch_norm.cap_width = p_ch_norm->cap_width;
	ch_norm.cap_height = p_ch_norm->cap_height;
	ch_norm.org_width = p_ch_norm->org_width;
	ch_norm.org_height = p_ch_norm->org_height;
	ch_norm.fps = p_ch_norm->fps;
	ch_norm.format = p_ch_norm->format;
	ch_norm.prog = p_ch_norm->prog;
	ch_norm.data_rate = p_ch_norm->data_rate;
	ch_norm.data_latch = p_ch_norm->data_latch;
	ch_norm.horiz_dup = p_ch_norm->horiz_dup;

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_CH_SET_NORM, &ch_norm);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VI_CH_SET_NORM ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}

ext:
	return ret;
}

HD_RESULT videocap_vi_ch_set_norm3(HD_VIDEOCAP_VI_CH_NORM3 *p_ch_norm)
{
	INT ioc_ret;
	HD_RESULT ret = HD_OK;
	struct vcap_ioc_vi_ch_norm_t ch_norm = {0};


	ch_norm.version = VCAP_IOC_VERSION;
	ch_norm.chip = p_ch_norm->chip;
	ch_norm.vcap = p_ch_norm->vcap;
	ch_norm.vi = p_ch_norm->vi;
	ch_norm.ch = p_ch_norm->ch;
	//HD_VIDEOCAP_VI_CH_NORM has no data_src, so need to get ch_norm.data_src
	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_CH_GET_NORM, &ch_norm);

	/* VCH PARAM */
	ch_norm.version = VCAP_IOC_VERSION;
	ch_norm.chip = p_ch_norm->chip;
	ch_norm.vcap = p_ch_norm->vcap;
	ch_norm.vi = p_ch_norm->vi;
	ch_norm.ch = p_ch_norm->ch;
	ch_norm.cap_width = p_ch_norm->cap_width;
	ch_norm.cap_height = p_ch_norm->cap_height;
	ch_norm.org_width = p_ch_norm->org_width;
	ch_norm.org_height = p_ch_norm->org_height;
	ch_norm.fps = p_ch_norm->fps;
	ch_norm.format = p_ch_norm->format;
	ch_norm.prog = p_ch_norm->prog;
	ch_norm.data_rate = p_ch_norm->data_rate;
	ch_norm.data_latch = p_ch_norm->data_latch;
	ch_norm.horiz_dup = p_ch_norm->horiz_dup;
	ch_norm.data_src = p_ch_norm->data_src;

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_VI_CH_SET_NORM, &ch_norm);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VI_CH_SET_NORM ioctl...error\n");
		ret = HD_ERR_SYS;
		goto ext;
	}

ext:
	return ret;
}

HD_RESULT videocap_get_devcount(HD_DEVCOUNT *p_devcount)
{
	p_devcount->max_dev_count = get_cap_dev_number_p();
	return HD_OK;
}

HD_RESULT videocap_get_syscaps(HD_DAL dev_id, HD_VIDEOCAP_SYSCAPS *p_cap_syscaps)
{
	HD_RESULT hd_ret = HD_OK;
	UINT32 device_subid, i;

	device_subid = VDOCAP_DEVID(dev_id);
	if (device_subid > videocap_max_device_nr) {
		HD_VIDEOCAPTURE_ERR("Error dev_id(%d)\n", dev_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}
	if (platform_sys_Info.cap_info[device_subid].vcapch < 0) {
		HD_VIDEOCAPTURE_ERR("Error, dev_id(%d) is invalid device id.\n", dev_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}

	p_cap_syscaps->dev_id = dev_id;
	p_cap_syscaps->chip_id = platform_sys_Info.cap_info[device_subid].chipid;
	p_cap_syscaps->max_in_count = videocap_max_device_in;
	p_cap_syscaps->max_out_count = videocap_max_device_out;
	p_cap_syscaps->dev_caps = 0; //TODO
	p_cap_syscaps->in_caps[0] = 0; //TODO
	for (i = 0; i < videocap_max_device_out; i++) {
		p_cap_syscaps->out_caps[i] = 0;
		p_cap_syscaps->out_caps[i] |= HD_VIDEO_CAPS_YUV420;
		p_cap_syscaps->out_caps[i] |= HD_VIDEO_CAPS_YUV422;
		p_cap_syscaps->out_caps[i] |= HD_VIDEO_CAPS_SCALE_UP;
		p_cap_syscaps->out_caps[i] |= HD_VIDEO_CAPS_SCALE_DOWN;
		p_cap_syscaps->out_caps[i] |= HD_VIDEO_CAPS_CROP_WIN;
		p_cap_syscaps->out_caps[i] |= HD_VIDEO_CAPS_FRC_DOWN;
	}
	p_cap_syscaps->max_dim.w = platform_sys_Info.cap_info[device_subid].width;
	p_cap_syscaps->max_dim.h = platform_sys_Info.cap_info[device_subid].height;
	p_cap_syscaps->max_frame_rate = platform_sys_Info.cap_info[device_subid].fps;
	p_cap_syscaps->max_w_scaleup = platform_sys_Info.cap_info[device_subid].ability.scl_h_max_sz;
exit:
	return hd_ret;
}

HD_RESULT videocap_get_sysinfo(HD_DAL dev_id, HD_VIDEOCAP_SYSINFO *p_sysinfo)
{
	HD_RESULT hd_ret = HD_OK;
	UINT32 device_subid, i;

	device_subid = VDOCAP_DEVID(dev_id);
	if (device_subid > videocap_max_device_nr) {
		HD_VIDEOCAPTURE_ERR("Error dev_id(%d)\n", dev_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}
	if (platform_sys_Info.cap_info[device_subid].vcapch < 0) {
		HD_VIDEOCAPTURE_ERR("Error, dev_id(%d) is invalid device id.\n", dev_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}

	p_sysinfo->dev_id = dev_id;
	for (i = 0; i < videocap_max_device_out; i++)
		p_sysinfo->cur_fps[i] = platform_sys_Info.cap_info[device_subid].fps;  //TODO

exit:
	return hd_ret;
}

HD_RESULT videocap_get_param_out(HD_DAL dev_id, HD_IO out_id, HD_VIDEOCAP_OUT *p_cap_out)
{
	HD_VIDEOCAP_OUT *inter_cap_out;
	UINT32 device_subid = VDOCAP_DEVID(dev_id);

	inter_cap_out = &g_video_cap_info[device_subid].cap_out[VDOCAP_OUTID(out_id)];
	p_cap_out->dim.w = inter_cap_out->dim.w;
	p_cap_out->dim.h = inter_cap_out->dim.h;
	p_cap_out->frc = inter_cap_out->frc;
	p_cap_out->pxlfmt = inter_cap_out->pxlfmt;
	p_cap_out->depth = inter_cap_out->depth;
	p_cap_out->dir = inter_cap_out->dir;
	return HD_OK;
}

HD_RESULT videocap_get_param_out_crop(HD_DAL dev_id, HD_IO out_id, HD_VIDEOCAP_CROP *p_cap_out_crop)
{
	HD_VIDEOCAP_CROP *param;
	UINT32 device_subid = VDOCAP_DEVID(dev_id);

	param = &g_video_cap_info[device_subid].cap_crop[VDOCAP_OUTID(out_id)];
	p_cap_out_crop->mode = param->mode;
	p_cap_out_crop->win.rect.x = param->win.rect.x;
	p_cap_out_crop->win.rect.y = param->win.rect.y;
	p_cap_out_crop->win.rect.w = param->win.rect.w;
	p_cap_out_crop->win.rect.h = param->win.rect.h;
	p_cap_out_crop->win.coord.w = param->win.coord.w;
	p_cap_out_crop->win.coord.h = param->win.coord.h;
	return HD_OK;
}

HD_RESULT videocap_set_crop(HD_DAL dev_id, HD_IO out_id, HD_VIDEOCAP_CROP *param)
{
	HD_RESULT hd_ret = HD_OK;
	hd_ret = bd_apply_attr(dev_id, out_id, HD_VIDEOCAP_PARAM_IN_CROP);
	return hd_ret;
}

HD_RESULT videocap_set_out(HD_DAL dev_id, HD_IO out_id, HD_VIDEOCAP_OUT *param)
{
	HD_RESULT hd_ret = HD_OK;

	return hd_ret;
}

HD_RESULT videocap_set_md_stat(HD_DAL dev_id, HD_VIDEOCAP_MD_STATUS *param)
{
	return HD_OK;
}

HD_RESULT videocap_user_proc_init(HD_PATH_ID path_id)
{
	INT fd, i;
	UINT engine, minor;
	// UINT queue_handle;
	uintptr_t queue_handle;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 device_subid = VDOCAP_DEVID(self_id);
	UINT32 out_subid = VDOCAP_OUTID(out_id);

	if (platform_sys_Info.cap_info[device_subid].vcapch < 0) {
		printf("vcapch invalid %d. pathid(0x%x)\n", device_subid, path_id);
		goto err_ext;
	}
	if (out_id > MAX_CAP_PORT) {//ctrl or mask path
		return HD_OK;
	}

	engine = ENTITY_FD_ENGINE(platform_sys_Info.cap_info[device_subid].engine_minor);
	minor = ENTITY_FD_MINOR(platform_sys_Info.cap_info[device_subid].engine_minor);
	fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_CAPTURE, platform_sys_Info.cap_info[device_subid].chipid, engine, minor + out_subid);
	if (fd == 0) {
		printf("get fd failed. device_subid(%d) out_subid(%d)\n", device_subid, out_subid);
		goto err_ext;
	}
	g_video_cap_info[device_subid].priv[out_subid].fd = fd;
	g_video_cap_info[device_subid].priv[out_subid].push_in_used = 0;

	queue_handle = pif_create_queue_for_hdal();
	if (queue_handle == 0) {
		HD_VIDEOCAPTURE_ERR("create queue_handle failed.\n");
		goto err_ext;
	}
	g_video_cap_info[device_subid].priv[out_subid].queue_handle = queue_handle;

	for (i = 0; i < HD_VP_MAX_DATA_TYPE; i++) {
		g_video_cap_info[device_subid].out_pool[out_subid].buf_info[i].enable = HD_BUF_AUTO;
	}
	cap_dev_cfg[device_subid].p_out_pool[out_subid] = &g_video_cap_info[device_subid].out_pool[out_subid];

	return HD_OK;

err_ext:
	return HD_ERR_NG;
}

HD_RESULT videocap_user_proc_uninit(HD_PATH_ID path_id)
{
	INT  ret;
	//UINT queue_handle;
	uintptr_t queue_handle;
	//usr_proc_stop_trigger_t stop_trigger_info;
	HD_RESULT hd_ret = HD_ERR_NG;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	UINT32 device_subid = VDOCAP_DEVID(self_id);
	UINT32 out_subid = VDOCAP_OUTID(out_id);

	if (platform_sys_Info.cap_info[device_subid].vcapch < 0) {
		printf("vcapch invalid %d. pathid(0x%x)\n", device_subid, path_id);
		goto err_ext;
	}
	if (out_id > MAX_CAP_PORT) {//ctrl or mask path
		return HD_OK;
	}

	if	(g_video_cap_info[device_subid].priv[out_subid].fd == 0) {
		//uninit already
		printf("get fd failed. device_subid(%d) out_subid(%d)\n", device_subid, out_subid);
		goto err_ext;
	}

	/* clear pull queue */
	queue_handle = g_video_cap_info[device_subid].priv[out_subid].queue_handle;
	if (queue_handle) {
		ret = pif_destroy_queue_for_hdal(queue_handle);
		if (ret < 0) {
			HD_VIDEOCAPTURE_ERR("Destroy queue failed. ret(%d)\n", ret);
			hd_ret = HD_ERR_NG;
			goto err_ext;
		}
	}
	g_video_cap_info[device_subid].priv[out_subid].fd = 0;

	return HD_OK;

err_ext:
	return hd_ret;
}

HD_RESULT videocap_uninit_path_p(HD_PATH_ID path_id)
{
	INT ret;
	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT device_subid, out_subid;
	usr_proc_stop_trigger_t stop_trigger_info;

	device_subid = VDOCAP_DEVID(self_id);
	out_subid = VDOCAP_OUTID(out_id);
	if ((UINT32)device_subid > videocap_max_device_nr || device_subid < 0) {
		HD_VIDEOCAPTURE_ERR("Error self_id(%d)\n", self_id);
		hd_ret = HD_ERR_DEV;
		goto err_ext;
	}
	if ((UINT32)out_subid >= videocap_max_device_out || out_subid < 0) {
		HD_VIDEOCAPTURE_ERR("Error out_id(%d)\n", out_id);
		hd_ret = HD_ERR_DEV;
		goto err_ext;
	}

	/* call driver stop */
	memset(&stop_trigger_info, 0, sizeof(stop_trigger_info));
	stop_trigger_info.usr_proc_fd.fd = g_video_cap_info[device_subid].priv[out_subid].fd;
	stop_trigger_info.mode = USR_PROC_STOP_FD;
	if ((ret = ioctl(vpd_fd, USR_PROC_STOP_TRIGGER, &stop_trigger_info)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		HD_VIDEOENC_ERR("<ioctl \"USR_PROC_STOP_TRIGGER\" fail, error(%d)>\n", ret);
		hd_ret = HD_ERR_NG;
	}
#if 0 //move to p_videocap_user_proc_uninit
	g_video_cap_info[device_subid].priv[out_subid].fd = 0;

	/* clear pull queue */
	queue_handle = g_video_cap_info[device_subid].priv[out_subid].queue_handle;
	if (queue_handle) {
		ret = pif_destroy_queue_for_hdal(queue_handle);
		if (ret < 0) {
			HD_VIDEOCAPTURE_ERR("Destroy queue failed. ret(%d)\n", ret);
			hd_ret = HD_ERR_NG;
			goto err_ext;
		}
	}
#endif
err_ext:
	return hd_ret;
}

HD_RESULT videocap_open(HD_IN_ID _in_id, HD_OUT_ID _out_id, HD_PATH_ID *p_path_id)
{
	static BOOL is_cap_env_update = 0;
	UINT32 device_subid, out_subid;
	HD_IO in_id = HD_GET_IN(_in_id);
	HD_DAL self_id = HD_GET_DEV(_out_id);
	HD_IO out_id;
	HD_RESULT ret;
	if (_out_id == HD_CTRL) {
		*p_path_id = VCAP_CTRL_FAKE_PATHID;
		return HD_OK;
	}

	device_subid = bd_get_dev_subid(self_id);
	if (is_cap_env_update == 0) {
		if (pif_env_update() < 0) {
			GMLIB_ERROR_P("ERR: deviceid(0x%x) is not available.\n", self_id);
			ret = HD_ERR_NOT_AVAIL;
			goto ext;
		}
		is_cap_env_update = 1;
		videocap_max_device_nr = get_cap_dev_number_p();
		videocap_max_device_out = get_cap_max_out_count_p();
		videocap_max_device_in = get_cap_max_in_count_p();
	}

	if (device_subid > videocap_max_device_nr) {
		GMLIB_ERROR_P("ERR: deviceid(0x%x) is not available, max_device_num(%d)\n", self_id, videocap_max_device_nr);
		ret = HD_ERR_NOT_AVAIL;
		goto ext;
	}

	out_id = HD_GET_OUT(_out_id);
	if (out_id >= (HD_MASK_BASE + 1) && out_id <= (HD_MASK_MAX + 1)) { //just creat pathid,for mask case
		if ((VDOCAP_MASKID(out_id) + 1) >= HD_VO_MAX_MASK) {
			GMLIB_ERROR_P("vcap mask(%d) is not available. max(%d)\n", VDOCAP_MASKID(out_id), HD_VO_MAX_MASK);
			ret = HD_ERR_NOT_AVAIL;
			goto ext;
		}
		*p_path_id = 0;
		*p_path_id = BD_SET_PATH(self_id, in_id, out_id);
		ret = HD_OK;
	} else {
		if (platform_sys_Info.cap_info[device_subid].vcapch < 0) {
			HD_VIDEOCAPTURE_ERR("Error, dev_id(%d) is invalid device id.\n", device_subid);
			ret = HD_ERR_DEV;
			goto ext;
		}

		if (HD_GET_CTRL(_out_id) == HD_CTRL) {
			out_id = HD_GET_CTRL(_out_id);
			in_id = 0;
		} else {
			out_id = HD_GET_OUT(_out_id);
			out_subid = BD_GET_OUT_SUBID(out_id);
			if (out_subid >= videocap_max_device_out) {
				GMLIB_ERROR_P("ERR: deviceid(%#x) out(%d) is not available. max_device_out(%d)\n",
					      self_id, out_id, videocap_max_device_out);

				ret = HD_ERR_NOT_AVAIL;
				goto ext;
			}
		}

		ret = bd_device_open(self_id, out_id, in_id, p_path_id);
	}

ext:
	return ret;
}

HD_RESULT videocap_close(HD_PATH_ID path_id)
{
	HD_RESULT ret;

	if (path_id == 	VCAP_CTRL_FAKE_PATHID) {
		return HD_OK;
	}

	ret = videocap_uninit_path_p(path_id);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("videocap_uninit_path_p error\n");
		goto err_ext;
	}
	ret = bd_device_close(path_id);

err_ext:
	return ret;
}

HD_RESULT videocap_close_fakepath(HD_PATH_ID path_id)
{
	HD_RESULT ret;
	HD_IO out_id;

	out_id = HD_GET_OUT(path_id);

	if (path_id == 	VCAP_CTRL_FAKE_PATHID) {
		return HD_OK;
	}
	if (out_id >= (HD_MASK_BASE + 1) && out_id <= (HD_MASK_MAX + 1)) { //just creat pathid,for mask case

		videocap_close_mask(path_id);
		return HD_OK;
	}

	ret = bd_device_close(path_id);
	return ret;
}

void videocap_ioctl_init(void)
{
	if (vcap_ioctl == 0) {
		vcap_ioctl = open("/dev/vcap0", O_RDWR);
		if (vcap_ioctl == -1) {
			HD_VIDEOCAPTURE_ERR("open videocap ioctl error\n");
		}
	}
}

void videocap_ioctl_uninit(void)
{
	if (vcap_ioctl) {
		close(vcap_ioctl);
		vcap_ioctl = 0;
	}
}


HD_RESULT videocap_init(void)
{
	HD_RESULT ret;

	memset(cap_dev_cfg, 0, sizeof(cap_dev_cfg));
	memset(g_video_cap_info, 0, sizeof(g_video_cap_info));

	videocap_max_device_nr = get_cap_dev_number_p();
	videocap_max_device_out = get_cap_max_out_count_p();
	videocap_max_device_in = get_cap_max_in_count_p();
	ret = bd_device_init(HD_DAL_VIDEOCAP_BASE, MAX_CAP_DEV, 1, MAX_CAP_PORT);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("bd_device_init error\n");
		goto err_ext;
	}

	videocap_ioctl_init();
	videocap_hw_spec_info.version = VCAP_IOC_VERSION;
	ret = ioctl(vcap_ioctl, VCAP_IOC_HW_LIMIT, &videocap_hw_spec_info);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("ioctl \"VCAP_IOC_HW_LIMIT\" return %d\n", ret);
		goto err_ext;
	}

	videocap_hw_ability.version = VCAP_IOC_VERSION;
	ret = ioctl(vcap_ioctl, VCAP_IOC_HW_ABILITY, &videocap_hw_ability);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("ioctl \"VCAP_IOC_HW_ABILITY\" return %d\n", ret);
		goto err_ext;
	}
	//ret = p_videocap_user_proc_init();
	//if (ret != HD_OK) {
	//	printf("p_videocap_user_proc_init fail!\n");
	//	goto err_ext;
	//}
	return HD_OK;

err_ext:
	return HD_ERR_NG;
}

HD_RESULT videocap_uninit(void)
{
	HD_RESULT hd_ret;
	//p_videocap_user_proc_uninit();
	videocap_ioctl_uninit();
	hd_ret = bd_device_uninit(HD_DAL_VIDEOCAP_BASE);

	return hd_ret;
}

HD_RESULT videocap_push_in_buf_pair(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK, check_ret = HD_OK;
	usr_proc_trigger_capture_t usr_proc_trigger_cap;
	UINT32 device_subid = VDOCAP_DEVID(self_id);
	INT ret, fd, is_support, valid_wh;
	uintptr_t queue_handle;
	HD_VIDEOCAP_OUT *cap_out;
	HD_DIM dim, out_dim, tmp_dim;
	vpd_buf_type_t vpd_buf_type;
	HD_VIDEOCAP_CROP *param;
	CHAR pool_name[20] = {};
	UINT estimate_size = 0, query_size = 0;
	HD_PATH_ID path_id = GET_VDEC_PATHID(self_id, 1, out_id);
	HD_VIDEO_PXLFMT outfmt;


	if (VDOCAP_DEVID(self_id) > videocap_max_device_nr) {
		HD_VIDEOCAPTURE_ERR("Error self_id(%d)\n", self_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}
	if (VDOCAP_OUTID(out_id) >= videocap_max_device_out) {
		HD_VIDEOCAPTURE_ERR("Error out_id(%d)\n", out_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}

	if (platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].vcapch < 0) {
		HD_VIDEOCAPTURE_ERR("Error, self_id(%d) is invalid device id.\n", self_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}

	if (p_video_frame == NULL) {
		HD_VIDEOCAPTURE_ERR("p_video_frame NULL\n");
		goto exit;
	}
	if (dif_check_flow_mode(self_id, out_id) == HD_ERR_ALREADY_BIND) {
		//HD_VIDEOCAPTURE_ERR("flow mode!\n");
		if (p_video_frame->reserved[6] != VCAP_PUSH_PULL_GIVE_BUF) {
			hd_ret = HD_ERR_ALREADY_BIND;
			goto exit;
		}
	}
	if (p_video_frame->phy_addr[0] == 0) {
		HD_VIDEOCAPTURE_ERR("phy_addr NULL\n");
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	cap_push_in.phy_addr[0] = p_video_frame->phy_addr[0];
	cap_push_in.reserved[0] = self_id;
	cap_push_in.reserved[1] = out_id;

	//trigger mode
	fd = g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].fd;
	queue_handle = g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].queue_handle;
	cap_out = cap_dev_cfg[device_subid].cap_out[VDOCAP_OUTID(out_id)];
	if (cap_out != NULL) {
		dim.w = cap_out->dim.w;
		dim.h = cap_out->dim.h;
		outfmt = cap_out->pxlfmt;
	} else {
		dim.w = p_video_frame->dim.w;
		dim.h = p_video_frame->dim.h;
		outfmt = p_video_frame->pxlfmt;
	}
	valid_wh = validate_cap_out_dim(dim.w, dim.h, outfmt);
	if (valid_wh != TRUE) {
		printf("videocap_push_in_buf_pair vcapout invalid. w = %u, h = %u, outfmt = 0x%x.\n", dim.w, dim.h, outfmt);
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	validate_cap_dim_wh_align_odd(dim, outfmt, &out_dim, 0);
	validate_cap_dim_bg_align_dir(out_dim, outfmt, &tmp_dim, 1);
	//Fill trigger parameters according to user parameters
	if (p_video_frame->phy_addr[0] == 0) {
		if ((cap_push_in.reserved[0] == self_id) && (cap_push_in.reserved[1] == out_id)
		    && (cap_push_in.phy_addr[0])) {
			p_video_frame->phy_addr[0] = cap_push_in.phy_addr[0];
		} else {
			printf("Error out_id(%d)\n", out_id);
			HD_VIDEOCAPTURE_ERR("Error out_id(%d)\n", out_id);
			hd_ret = HD_ERR_PARAM;
			goto exit;
		}
	}

	estimate_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(tmp_dim, p_video_frame->pxlfmt);
	ret = pif_query_video_buffer_size(p_video_frame->phy_addr[0],
					  p_video_frame->ddr_id,
					  pool_name, &query_size);
	if (ret == -2) {
		/* can not find pool by this pa, skip it */
		hd_ret = HD_OK;
	} else if (ret < 0) {
		HD_VIDEOCAPTURE_ERR("query_video_buffer_size fail\n");
		hd_ret = HD_ERR_NOBUF;
		goto exit;
	} else {
		if (query_size < estimate_size) {
			HD_VIDEOCAPTURE_ERR("path_id(%#x) p_user_out_video_frame too small, buf size(%u) < set size(%u) by wh(%u,%u) pxlfmt(%#x)\n",
					    path_id, query_size, estimate_size, tmp_dim.w, tmp_dim.h, p_video_frame->pxlfmt);
			hd_ret = HD_ERR_NOBUF;
			goto exit;
		}
	}

	//check the output type if capture can support it
	vpd_buf_type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_video_frame->pxlfmt);
	check_ret = videocap_check_vcap_validtype(vpd_buf_type, &is_support);
	if (check_ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("videocap_check_vcap_validtype check error\n");
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	if (!is_support) {
		HD_VIDEOCAPTURE_ERR("videocap doesn't support for pxlfmt(%#x), please 'cat /proc/videograph/sys_spec' for capability\n",
				    p_video_frame->pxlfmt);
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}

	memset(&usr_proc_trigger_cap, 0, sizeof(usr_proc_trigger_cap));
	usr_proc_trigger_cap.fd = fd;
	usr_proc_trigger_cap.out_frame_buffer.ddr_id = p_video_frame->ddr_id;
	usr_proc_trigger_cap.out_frame_buffer.addr_pa = p_video_frame->phy_addr[0];
	if (pif_address_ddr_sanity_check(usr_proc_trigger_cap.out_frame_buffer.addr_pa, usr_proc_trigger_cap.out_frame_buffer.ddr_id) < 0) {
		HD_VIDEOCAPTURE_ERR("%s:Check out_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_proc_trigger_cap.out_frame_buffer.addr_pa,
			usr_proc_trigger_cap.out_frame_buffer.ddr_id);
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	usr_proc_trigger_cap.out_frame_buffer.size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(tmp_dim, p_video_frame->pxlfmt);  //TODO
	usr_proc_trigger_cap.out_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_video_frame->pxlfmt);
	usr_proc_trigger_cap.out_frame_info.dim.w = out_dim.w;
	usr_proc_trigger_cap.out_frame_info.dim.h = out_dim.h;
	usr_proc_trigger_cap.out_frame_info.pathid = p_video_frame->reserved[0];
	usr_proc_trigger_cap.src_bg_dim.w = platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].width;
	usr_proc_trigger_cap.src_bg_dim.h = platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].height;
	usr_proc_trigger_cap.info.ch = VDOCAP_DEVID(self_id);
	usr_proc_trigger_cap.info.path = VDOCAP_OUTID(out_id);
	usr_proc_trigger_cap.info.frame_rate = 30;
	cap_out = &g_video_cap_info[device_subid].cap_out[VDOCAP_OUTID(out_id)];
	usr_proc_trigger_cap.flip_mode = videocap_get_flip_param(cap_out->dir);

	param = &g_video_cap_info[device_subid].cap_crop[VDOCAP_OUTID(out_id)];
	if (param->mode == HD_CROP_ON) {
		usr_proc_trigger_cap.scale.enable = 1;

		usr_proc_trigger_cap.scale.src_crop.x  = param->win.rect.x;
		usr_proc_trigger_cap.scale.src_crop.y  = param->win.rect.y;
		usr_proc_trigger_cap.scale.src_crop.w  = param->win.rect.w;
		usr_proc_trigger_cap.scale.src_crop.h  = param->win.rect.h;
		usr_proc_trigger_cap.scale.dst_rect.x  = 0;
		usr_proc_trigger_cap.scale.dst_rect.y  = 0;
		usr_proc_trigger_cap.scale.dst_rect.w  = out_dim.w;
		usr_proc_trigger_cap.scale.dst_rect.h  = out_dim.h;
	} else {
		usr_proc_trigger_cap.scale.enable = 0;
	}
	usr_proc_trigger_cap.queue_handle = queue_handle;

	if (fd == 0) {
		printf(" push in fails fd = %d ", fd);
		printf(" device_sub = %d\n", device_subid);
		printf(" out_id = %d\n", out_id);
		goto exit;
	}
	usr_proc_trigger_cap.ext_manual_v = tmp_dim.h - out_dim.h;//verical
	usr_proc_trigger_cap.ext_manual_h = tmp_dim.w - out_dim.w;//horizontal
	if (p_video_frame->reserved[4] < DRC_MODE_MAX) {
		usr_proc_trigger_cap.drc_mode = p_video_frame->reserved[4];
	} else {
		usr_proc_trigger_cap.drc_mode = 0;
	}
	if ((ret = ioctl(vpd_fd, USR_PROC_TRIGGER_CAPTURE, &usr_proc_trigger_cap)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		HD_VIDEOCAPTURE_ERR("<ioctl \"USR_PROC_TRIGGER_CAPTURE\" fail, error(%d)>\n", ret);
		goto exit;
	}
	if (usr_proc_trigger_cap.queue_handle != 0) {
		queue_handle = usr_proc_trigger_cap.queue_handle;
	}
	p_video_frame->dim.w = tmp_dim.w;
	p_video_frame->dim.h = tmp_dim.h;
exit:
	return hd_ret;
}

HD_RESULT videocap_pull_out_buf_pair(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK, check_ret = HD_OK;
	usr_proc_cap_cb_info_t usr_proc_cap_cb_info = {0};
	UINT32 device_subid = VDOCAP_DEVID(self_id);
	INT ret, is_support, fd;
	uintptr_t queue_handle;
	vpd_buf_type_t vpd_buf_type;

	if (VDOCAP_DEVID(self_id) > videocap_max_device_nr) {
		HD_VIDEOCAPTURE_ERR("Error self_id(%d)\n", self_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}
	if (VDOCAP_OUTID(out_id) >= videocap_max_device_out) {
		HD_VIDEOCAPTURE_ERR("Error out_id(%d)\n", out_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}

	if (platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].vcapch < 0) {
		HD_VIDEOCAPTURE_ERR("Error, self_id(%d) is invalid device id.\n", self_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}

	if (p_video_frame == NULL) {
		HD_VIDEOCAPTURE_ERR("p_video_frame NULL\n");
		goto exit;
	}
	if (p_video_frame->reserved[6] != VCAP_PUSH_PULL_GIVE_BUF) {
		hd_ret = dif_pull_out_buffer(self_id, out_id, p_video_frame, wait_ms);
		if (hd_ret == HD_OK) {
			if (p_video_frame->phy_addr[0] == 0) {
				HD_VIDEOCAPTURE_ERR("dif_pull_out_buffer fail\n");
				hd_ret = HD_ERR_NG;
				goto exit;
			} else {//hd_ret HD_OK
				goto exit;
			}
		}
	}
	if ((p_video_frame->reserved[6] == VCAP_PUSH_PULL_GIVE_BUF) || (hd_ret == HD_ERR_NOT_AVAIL)) {
		queue_handle = g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].queue_handle;

		//Fill trigger parameters according to user parameters
		if (p_video_frame->phy_addr[0] == 0) {
			if ((cap_push_in.reserved[0] == self_id) && (cap_push_in.reserved[1] == out_id)
			    && (cap_push_in.phy_addr[0])) {
				p_video_frame->phy_addr[0] = cap_push_in.phy_addr[0];
			} else {
				printf("Error out_id(%d)\n", out_id);
				HD_VIDEOCAPTURE_ERR("Error out_id(%d)\n", out_id);
				hd_ret = HD_ERR_PARAM;
				goto exit;
			}
		}

		//check the output type if capture can support it
		vpd_buf_type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_video_frame->pxlfmt);
		check_ret = videocap_check_vcap_validtype(vpd_buf_type, &is_support);
		if (check_ret != HD_OK) {
			HD_VIDEOCAPTURE_ERR("videocap_check_vcap_validtype check error\n");
			hd_ret = HD_ERR_PARAM;
			goto exit;
		}
		if (!is_support) {
			printf("%s, videocap doesn't support for pxlfmt(%#x), please 'cat /proc/videograph/sys_spec' for capability\n",
			       __func__, p_video_frame->pxlfmt);
			hd_ret = HD_ERR_PARAM;
			goto exit;
		}
		fd = g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].fd;
		usr_proc_cap_cb_info.fd = fd;
		usr_proc_cap_cb_info.queue_handle = queue_handle;
		usr_proc_cap_cb_info.wait_ms = wait_ms;
		if (usr_proc_cap_cb_info.fd == 0) {
			HD_VIDEOCAPTURE_ERR(" pull out fails fd = %d ", fd);
			HD_VIDEOCAPTURE_ERR(" device_sub = %d\n", device_subid);
			HD_VIDEOCAPTURE_ERR(" out_id = %d\n", out_id);
			goto exit;
		}

		if ((ret = ioctl(vpd_fd, USR_PROC_RECV_DATA_CAPTURE, &usr_proc_cap_cb_info)) < 0) {
			int errsv = errno;
			ret = errsv * -1;
			HD_VIDEOCAPTURE_ERR("<ioctl \"USR_PROC_RECV_DATA_CAPTURE\" fail, error(%d)>\n", ret);
			if (ret == -15) {
				hd_ret = HD_ERR_TIMEDOUT;
			} else {
				hd_ret = HD_ERR_SYS;
			}
			goto exit;
		}
		p_video_frame->phy_addr[0] = usr_proc_cap_cb_info.addr_pa;

		p_video_frame->timestamp = usr_proc_cap_cb_info.timestamp * 1000;
		if (usr_proc_cap_cb_info.status == 1)  { //1:job finished, 0:ongoing, -1:fail
			hd_ret = HD_OK;
		} else if (usr_proc_cap_cb_info.status == 0) {
			HD_VIDEOCAPTURE_ERR("status ongoing \n");
			hd_ret = HD_ERR_TIMEDOUT;
		} else {
			hd_ret = HD_ERR_FAIL;
		}
	} else {
		HD_VIDEOCAPTURE_ERR("pull out buffer error, ret(%#x)\n", hd_ret);
		goto exit;
	}


exit:
	if (hd_ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("pull out buffer EXIT, ret(%#x)\n", hd_ret);
	}
	return hd_ret;
}

HD_RESULT videocap_push_in_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_out_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	if (VDOCAP_DEVID(self_id) > videocap_max_device_nr) {
		HD_VIDEOCAPTURE_ERR("Error self_id(%d)\n", self_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}
	if (VDOCAP_OUTID(out_id) >= videocap_max_device_out) {
		HD_VIDEOCAPTURE_ERR("Error out_id(%d)\n", out_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}
	if (p_out_video_frame == NULL) {
		HD_VIDEOCAPTURE_ERR("p_video_frame NULL\n");
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	if (p_out_video_frame->phy_addr[0] == 0) {
		HD_VIDEOCAPTURE_ERR("phy_addr NULL\n");
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	if (p_out_video_frame->phy_addr[0]) {
		cap_push_in.phy_addr[0] = p_out_video_frame->phy_addr[0];
		cap_push_in.reserved[0] = self_id;
		cap_push_in.reserved[1] = out_id;
	}
exit:
	return hd_ret;


}

HD_RESULT videocap_pull_out_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT ret;
	ret = videocap_push_in_buf_pair(self_id, out_id, p_video_frame, wait_ms);
	if ((ret != HD_ERR_ALREADY_BIND) && (ret != HD_OK)) {
		HD_VIDEOCAPTURE_ERR("videocap_pull_out_buf_pair fail\n");
		goto ext;
	}

	ret = videocap_pull_out_buf_pair(self_id, out_id, p_video_frame, wait_ms);
	if (ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("videocap_pull_out_buf_pair fail\n");
	}
ext:
	return ret;
}

HD_RESULT videocap_release_out_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame)
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
			HD_VIDEOCAPTURE_ERR("%s, free buffer(pa:%p) failed.\n", __func__, addr_pa);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		hd_ret = HD_OK;
	} else {
		HD_VIDEOCAPTURE_ERR("release out buffer error, ret(%#x)\n", hd_ret);
		hd_ret = HD_ERR_NG;
	}
exit:
	return hd_ret;

}

static int check_mask_path_id(HD_PATH_ID path_id)
{
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id) - 1;
	HD_IO in_id = HD_GET_IN(path_id);

	if (bd_get_dev_baseid(self_id) != HD_DAL_VIDEOCAP_BASE) {
		HD_VIDEOCAPTURE_ERR(" The device of path_id(%#x) is not videocapture.\n", path_id);
		return -1;
	}

	if (VDOCAP_DEVID(self_id) > MAX_CAP_DEV) {
		HD_VIDEOCAPTURE_ERR("Error self_id(%d)\n", (self_id));
		return -1;
	}

	if (out_id < HD_MASK_BASE || out_id > HD_MASK_MAX) {
		HD_VIDEOCAPTURE_ERR("mask attr can't be applied to out_id(%d) which is not mask\n", out_id);
		return -1;
	}

	if (VDOCAP_INID(in_id) >= MAX_CAP_PORT) {
		HD_VIDEOCAPTURE_ERR("mask in_id(%d) is not available. max(%d)\n", VDOCAP_INID(in_id), MAX_CAP_PORT);
		return -1;
	}

	if (VDOCAP_MASKID(out_id) >= HD_VO_MAX_MASK) {
		HD_VIDEOCAPTURE_ERR("vcap mask(%d) is not available. max(%d)\n", VDOCAP_MASKID(out_id), HD_VO_MAX_MASK);
		return -1;
	}

	return 0;
}

static int get_mask_index(HD_PATH_ID path_id, char *chip, char *cap, char *vi, char *ch)
{
	INT ret = 0;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_IN(path_id);
	UINT32 device_subid = VDOCAP_DEVID(dev_id);
	struct vcap_ioc_vg_fd_t fd_info;

	memset(&fd_info, 0x0, sizeof(struct vcap_ioc_vg_fd_t));
	fd_info.version = VCAP_IOC_VERSION;
	fd_info.fd = utl_get_cap_entity_fd(device_subid, VDOCAP_INID(out_id), NULL);
	ret = ioctl(vcap_ioctl, VCAP_IOC_VG_FD_CONVERT, &fd_info); //notify fd invalid
	if (ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VG_FD_CONVERT(%x) fail\n", path_id);
		return HD_ERR_SYS;
	}

	if (chip)
		*chip = fd_info.chip;
	if (cap)
		*cap = fd_info.vcap;
	if (vi)
		*vi = fd_info.vi;
	if (ch)
		*ch = fd_info.ch;
	return 0;
}

HD_RESULT videocap_set_mask(HD_PATH_ID path_id, HD_OSG_MASK_ATTR *param)
{
	INT ioc_ret;
	HD_DAL dev_id  = HD_GET_DEV(path_id);
	UINT32 device_subid = VDOCAP_DEVID(dev_id);
	HD_IO mask_id = VDOCAP_MASKID(HD_GET_OUT(path_id) - 1);
	HD_IO out_id  = VDOCAP_INID(HD_GET_IN(path_id));
	struct vcap_ioc_mask_ctrl_t ctrl;
	struct vcap_ioc_mask_win_t win;
	char chip, cap, vi, ch;
	UINT32 i;

	if (!param) {
		HD_VIDEOCAPTURE_ERR("param is NULL\n");
		return HD_ERR_NG;
	}

	if (check_mask_path_id(path_id) < 0) {
		HD_VIDEOCAPTURE_ERR("vcap invalid path_id(%d)\n", (int)path_id);
		return HD_ERR_NG;
	}

	if (get_mask_index(path_id, &chip, &cap, &vi, &ch)) {
		HD_VIDEOCAPTURE_ERR("vcap get_mask_index(%d) fail\n", (int)path_id);
		return HD_ERR_NG;
	}

	memset(&win, 0, sizeof(struct vcap_ioc_mask_win_t));
	win.x_start = param->position[0].x;
	if (win.x_start & 1) {//x is 2 aligned
		HD_VIDEOCAPTURE_ERR("top left x(%d) must be 2 aligned\n", win.x_start);
		return HD_ERR_NG;
	}

	win.y_start = param->position[0].y;
	if (param->position[1].x <= param->position[0].x) {
		HD_VIDEOCAPTURE_ERR("left x(%d) >= right x(%d)\n", (int)param->position[0].x, (int)param->position[1].x);
		return HD_ERR_NG;
	} else
		win.width = (param->position[1].x - param->position[0].x);
	if (param->position[3].y <= param->position[0].y) {
		HD_VIDEOCAPTURE_ERR("top y(%d) >= bottom y(%d)\n", (int)param->position[0].y, (int)param->position[3].y);
		return HD_ERR_NG;
	} else
		win.height = (param->position[3].y - param->position[0].y);

	win.bg_width = platform_sys_Info.cap_info[device_subid].width;
	if (win.bg_width & 3) {//width is 4 aligned
		HD_VIDEOCAPTURE_ERR("width(%d) must be 4 aligned\n", win.bg_width);
		return HD_ERR_NG;
	}
	win.bg_height = platform_sys_Info.cap_info[device_subid].height;
	if (param->color > 7) {
		HD_VIDEOCAPTURE_ERR("invalid color(%d)\n", (int)param->color);
		return HD_ERR_NG;
	} else
		win.color = param->color;

	memset(&ctrl, 0, sizeof(struct vcap_ioc_mask_ctrl_t));
	ctrl.version = VCAP_IOC_VERSION;
	ctrl.chip    = chip;
	ctrl.vcap    = cap;
	ctrl.vi      = vi;
	ctrl.ch      = ch;
	ctrl.win_idx = mask_id;

	for (i = 0; i < platform_sys_Info.cap_info[device_subid].num_of_path; i++) {
		ctrl.path = i;
		ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_MASK_CTRL, &ctrl);
		if (ioc_ret < 0) {
			HD_VIDEOCAPTURE_ERR("VCAP_IOC_MASK_CTRL ioctl...error\n");
			return HD_ERR_SYS;
		}
	}

	win.version = VCAP_IOC_VERSION;
	win.chip    = chip;
	win.vcap    = cap;
	win.vi      = vi;
	win.ch      = ch;
	win.win_idx = mask_id;
	if (param->alpha == 0)
		win.alpha = VCAP_IOC_MASK_ALPHA_0;
	else if (param->alpha < 65)
		win.alpha = VCAP_IOC_MASK_ALPHA_25;
	else if (param->alpha < 96)
		win.alpha = VCAP_IOC_MASK_ALPHA_37_5;
	else if (param->alpha < 128)
		win.alpha = VCAP_IOC_MASK_ALPHA_50;
	else if (param->alpha < 160)
		win.alpha = VCAP_IOC_MASK_ALPHA_62_5;
	else if (param->alpha < 192)
		win.alpha = VCAP_IOC_MASK_ALPHA_75;
	else if (param->alpha < 224)
		win.alpha = VCAP_IOC_MASK_ALPHA_87_5;
	else
		win.alpha = VCAP_IOC_MASK_ALPHA_100;

	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_MASK_SET_WIN, &win);
	if (ioc_ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_MASK_SET_WIN ioctl...error\n");
		return HD_ERR_SYS;
	}

	memset(&ctrl, 0, sizeof(struct vcap_ioc_mask_ctrl_t));
	ctrl.version = VCAP_IOC_VERSION;
	ctrl.chip    = chip;
	ctrl.vcap    = cap;
	ctrl.vi      = vi;
	ctrl.ch      = ch;
	ctrl.win_idx = mask_id;
	ctrl.win_enb = 1;
	ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_MASK_CTRL, &ctrl);
	for (i = 0; i < platform_sys_Info.cap_info[device_subid].num_of_path; i++) {
		ctrl.path = i;
		ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_MASK_CTRL, &ctrl);
		if (ioc_ret < 0) {
			HD_VIDEOCAPTURE_ERR("VCAP_IOC_MASK_CTRL ioctl...error\n");
			return HD_ERR_SYS;
		}
	}

	memcpy(&g_video_cap_info[device_subid].mask[out_id][mask_id],
	       param, sizeof(HD_OSG_MASK_ATTR));

	return HD_OK;
}

HD_RESULT videocap_get_mask(HD_PATH_ID path_id, HD_OSG_MASK_ATTR *param)
{
	HD_DAL dev_id  = HD_GET_DEV(path_id);
	UINT32 device_subid = VDOCAP_DEVID(dev_id);
	HD_IO  mask_id = VDOCAP_MASKID(HD_GET_OUT(path_id) - 1);
	HD_IO  out_id  = VDOCAP_INID(HD_GET_IN(path_id));

	if (!param) {
		HD_VIDEOCAPTURE_ERR("param is NULL\n");
		return HD_ERR_NG;
	}

	if (check_mask_path_id(path_id) < 0) {
		HD_VIDEOCAPTURE_ERR("vcap invalid path_id(%d)\n", (int)path_id);
		return HD_ERR_NG;
	}

	memcpy(param, &g_video_cap_info[device_subid].mask[out_id][mask_id], sizeof(HD_OSG_MASK_ATTR));
	return HD_OK;
}

HD_RESULT videocap_start_mask(HD_PATH_ID path_id)
{
	UINT32 i;
	INT ioc_ret;
	HD_IO mask_id = VDOCAP_MASKID(HD_GET_OUT(path_id) - 1);
	char chip, cap, vi, ch;
	struct vcap_ioc_mask_ctrl_t ctrl;
	HD_DAL dev_id  = HD_GET_DEV(path_id);
	UINT32 device_subid = VDOCAP_DEVID(dev_id);

	if (check_mask_path_id(path_id) < 0) {
		HD_VIDEOCAPTURE_ERR("vcap invalid path_id(%d)\n", (int)path_id);
		return HD_ERR_NG;
	}

	if (get_mask_index(path_id, &chip, &cap, &vi, &ch)) {
		HD_VIDEOCAPTURE_ERR("vcap get_mask_index(%d) fail\n", (int)path_id);
		return HD_ERR_NG;
	}

	memset(&ctrl, 0, sizeof(struct vcap_ioc_mask_ctrl_t));
	ctrl.version = VCAP_IOC_VERSION;
	ctrl.chip    = chip;
	ctrl.vcap    = cap;
	ctrl.vi      = vi;
	ctrl.ch      = ch;
	ctrl.win_idx = mask_id;
	ctrl.win_enb = 1;

	for (i = 0; i < platform_sys_Info.cap_info[device_subid].num_of_path; i++) {
		ctrl.path = i;
		ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_MASK_CTRL, &ctrl);
		if (ioc_ret < 0) {
			HD_VIDEOCAPTURE_ERR("VCAP_IOC_MASK_CTRL ioctl...error\n");
			return HD_ERR_SYS;
		}
	}
	return HD_OK;
}

HD_RESULT videocap_stop_mask(HD_PATH_ID path_id)
{
	INT ioc_ret;
	HD_IO mask_id = VDOCAP_MASKID(HD_GET_OUT(path_id) - 1);
	char chip, cap, vi, ch;
	struct vcap_ioc_mask_ctrl_t ctrl;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	UINT32 i, device_subid = VDOCAP_DEVID(dev_id);


	if (check_mask_path_id(path_id) < 0) {
		HD_VIDEOCAPTURE_ERR("vcap invalid path_id(%d)\n", (int)path_id);
		return HD_ERR_NG;
	}

	if (get_mask_index(path_id, &chip, &cap, &vi, &ch)) {
		HD_VIDEOCAPTURE_ERR("vcap get_mask_index(%d) fail\n", (int)path_id);
		return HD_ERR_NG;
	}

	memset(&ctrl, 0, sizeof(struct vcap_ioc_mask_ctrl_t));
	ctrl.version = VCAP_IOC_VERSION;
	ctrl.chip    = chip;
	ctrl.vcap    = cap;
	ctrl.vi      = vi;
	ctrl.ch      = ch;
	ctrl.win_idx = mask_id;

	for (i = 0; i < platform_sys_Info.cap_info[device_subid].num_of_path; i++) {
		ctrl.path = i;
		ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_MASK_CTRL, &ctrl);
		if (ioc_ret < 0) {
			HD_VIDEOCAPTURE_ERR("VCAP_IOC_MASK_CTRL ioctl...error\n");
			return HD_ERR_SYS;
		}
	}
	return HD_OK;
}

HD_RESULT videocap_close_mask(HD_PATH_ID path_id)
{
	INT ioc_ret, i;
	HD_DAL dev_id  = HD_GET_DEV(path_id);
	UINT32 device_subid = VDOCAP_DEVID(dev_id);
	HD_IO mask_id = VDOCAP_MASKID(HD_GET_OUT(path_id) - 1);
	HD_IO out_id  = VDOCAP_INID(HD_GET_IN(path_id));
	char chip, cap, vi, ch;
	struct vcap_ioc_mask_ctrl_t ctrl;

	if (check_mask_path_id(path_id) < 0) {
		HD_VIDEOCAPTURE_ERR("vcap invalid path_id(%d)\n", (int)path_id);
		return HD_ERR_NG;
	}

	if (get_mask_index(path_id, &chip, &cap, &vi, &ch)) {
		HD_VIDEOCAPTURE_ERR("vcap get_mask_index(%d) fail\n", (int)path_id);
		return HD_ERR_NG;
	}

	memset(&ctrl, 0, sizeof(struct vcap_ioc_mask_ctrl_t));
	ctrl.version = VCAP_IOC_VERSION;
	ctrl.chip    = chip;
	ctrl.vcap    = cap;
	ctrl.vi      = vi;
	ctrl.ch      = ch;
	ctrl.path    = out_id;
	ctrl.win_idx = mask_id;

	for (i = 0; i < platform_sys_Info.cap_info[device_subid].num_of_path; i++) {
		ctrl.path = i;
		ioc_ret = ioctl(vcap_ioctl, VCAP_IOC_MASK_CTRL, &ctrl);
		if (ioc_ret < 0) {
			HD_VIDEOCAPTURE_ERR("VCAP_IOC_MASK_CTRL ioctl...error\n");
			return HD_ERR_SYS;
		}
	}

	memset(&g_video_cap_info[device_subid].mask[out_id][mask_id],
	       0, sizeof(HD_OSG_MASK_ATTR));

	return HD_OK;
}

HD_RESULT videocap_set_mask_palette(HD_PATH_ID path_id, HD_OSG_PALETTE_TBL *palette)
{
	HD_DAL dev_id = HD_GET_DEV(path_id);
	UINT32 device_subid = VDOCAP_DEVID(dev_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	INT ret = 0, i;
	struct vcap_ioc_vg_fd_t fd_info;
	struct vcap_ioc_palette_t vcap_palette;

	if (!palette) {
		HD_VIDEOCAPTURE_ERR("palette is NULL\n");
		return HD_ERR_NG;
	}

	memset(&fd_info, 0x0, sizeof(struct vcap_ioc_vg_fd_t));
	fd_info.version = VCAP_IOC_VERSION;
	fd_info.fd = utl_get_cap_entity_fd(device_subid, VDOCAP_INID(out_id), NULL);
	ret = ioctl(vcap_ioctl, VCAP_IOC_VG_FD_CONVERT, &fd_info); //notify fd invalid
	if (ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VG_FD_CONVERT(%x) fail\n", path_id);
		return HD_ERR_SYS;
	}

	for (i = 0 ; i < 8 ; ++i) {
		memset(&vcap_palette, 0, sizeof(struct vcap_ioc_palette_t));
		vcap_palette.version = VCAP_IOC_VERSION;
		vcap_palette.chip    = fd_info.chip;
		vcap_palette.vcap    = fd_info.vcap;
		vcap_palette.p_id    = i;
		vcap_palette.color_yuv = ((palette->pal_y[i]     & 0xFF)       |
					  ((palette->pal_cb[i] & 0xFF) << 8) |
					  ((palette->pal_cr[i] & 0xFF) << 16));
		ret = ioctl(vcap_ioctl, VCAP_IOC_PALETTE_SET_COLOR, &vcap_palette);
		if (ret < 0) {
			HD_VIDEOCAPTURE_ERR("VCAP_IOC_MASK_CTRL ioctl...error\n");
			return HD_ERR_SYS;
		}
	}

	return HD_OK;
}

HD_RESULT videocap_get_mask_palette(HD_PATH_ID path_id, HD_OSG_PALETTE_TBL *palette)
{
	INT ret = 0, i;
	HD_DAL dev_id = HD_GET_DEV(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	struct vcap_ioc_vg_fd_t fd_info;
	struct vcap_ioc_palette_t vcap_palette;
	UINT32 device_subid = VDOCAP_DEVID(dev_id);

	if (!palette) {
		HD_VIDEOCAPTURE_ERR("palette is NULL\n");
		return HD_ERR_NG;
	}

	memset(&fd_info, 0x0, sizeof(struct vcap_ioc_vg_fd_t));
	fd_info.version = VCAP_IOC_VERSION;
	fd_info.fd = utl_get_cap_entity_fd(device_subid, VDOCAP_INID(out_id), NULL);
	ret = ioctl(vcap_ioctl, VCAP_IOC_VG_FD_CONVERT, &fd_info); //notify fd invalid
	if (ret < 0) {
		HD_VIDEOCAPTURE_ERR("VCAP_IOC_VG_FD_CONVERT(%x) fail\n", path_id);
		return HD_ERR_SYS;
	}

	for (i = 0 ; i < 8 ; ++i) {
		memset(&vcap_palette, 0, sizeof(struct vcap_ioc_palette_t));
		vcap_palette.version = VCAP_IOC_VERSION;
		vcap_palette.chip    = fd_info.chip;
		vcap_palette.vcap    = fd_info.vcap;
		vcap_palette.p_id    = i;
		ret = ioctl(vcap_ioctl, VCAP_IOC_PALETTE_GET_COLOR, &vcap_palette);
		if (ret < 0) {
			HD_VIDEOCAPTURE_ERR("VCAP_IOC_MASK_CTRL ioctl...error\n");
			return HD_ERR_SYS;
		}

		palette->pal_y[i]  = (vcap_palette.color_yuv & 0xFF);
		palette->pal_cb[i] = ((vcap_palette.color_yuv & 0xFF00) >> 8);
		palette->pal_cr[i] = ((vcap_palette.color_yuv & 0xFF0000) >> 16);
	}

	return HD_OK;
}

CAP_INTERNAL_PARAM *videocap_get_param_p(HD_DAL self_id)
{
	if (VDOCAP_DEVID(self_id) > MAX_CAP_DEV) {
		HD_VIDEOCAPTURE_ERR("Error self_id(%d)\n", self_id);
		goto err_ext;
	}
	return &cap_dev_cfg[VDOCAP_DEVID(self_id)];
err_ext:
	return NULL;
}

vpd_flip_t videocap_get_flip_param(HD_VIDEO_DIR dir)
{
	vpd_flip_t def = VPD_FLIP_NONE;
	if (dir == HD_VIDEO_DIR_MIRRORX)
		def = VPD_FLIP_HORIZONTAL;
	else if (dir == HD_VIDEO_DIR_MIRRORY)
		def = VPD_FLIP_VERTICAL;
	else if (dir == HD_VIDEO_DIR_MIRRORXY)
		def = VPD_FLIP_VERTICAL_AND_HORIZONTAL;
	return def;
}

HD_RESULT videocap_check_out_param(HD_VIDEOCAP_OUT *vcap_out_param)
{
	HD_RESULT result = HD_OK;
	INT check;

	check = validate_cap_out_dim(vcap_out_param->dim.w, vcap_out_param->dim.h, vcap_out_param->pxlfmt);
	if (check == FALSE) {
		printf("validate_cap_out_dim fail\n");
		result = HD_ERR_NG;
		return result;
	}
	return result;
}

HD_RESULT videocap_check_crop_param(HD_VIDEOCAP_CROP *vcap_crop_param)
{
	HD_RESULT result = HD_OK;
	INT check;
	if (!vcap_crop_param) {
		result = HD_ERR_NG;
		goto err_exit;
	}
	if (vcap_crop_param->mode == HD_CROP_OFF) {
		result = HD_OK;
		return result;
	}
	check = validate_cap_crop_dim(vcap_crop_param->win.rect.w, vcap_crop_param->win.rect.h);
	if (check == FALSE) {
		result = HD_ERR_NG;
		goto err_exit;
	}
	return result;
err_exit:
	GMLIB_ERROR_P("ERR: videocap_check_crop_param err 0x%x\n", result);
	return result;
}

HD_RESULT videocap_check_vcap_validtype(vpd_buf_type_t vpd_buf_type, INT *valid)
{
	unsigned int trans_type;

	switch (vpd_buf_type) {
	case VPD_BUFTYPE_YUV422:
		trans_type = VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV422;
		break;
	case VPD_BUFTYPE_YUV420_SP:
		trans_type = VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV420;
		break;
	case VPD_BUFTYPE_YUV422_SCE:
		trans_type = VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV422_SCE;
		break;
	case VPD_BUFTYPE_YUV420_SCE:
		trans_type = VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV420_SCE;
		break;
	case VPD_BUFTYPE_YUV422_MB:
		trans_type = VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV422_MB;
		break;
	case VPD_BUFTYPE_YUV420_MB:
		trans_type = VCAP_IOC_DMA_OUT_FMT_ABILITY_BITMASK_YUV420_MB;
		break;
	default:
		trans_type = 0;
		break;
	}
	if (trans_type == 0) {
		GMLIB_ERROR_P("ERR: invalid vpd_buf_type 0x%x\n", vpd_buf_type);
		goto err;
	}
	if (videocap_hw_ability.dma_out_fmt_ability & trans_type) {
		*valid = 1;
	} else {
		*valid = 0;
		GMLIB_ERROR_P("ERR: valid vcap dst fmt 0x%x, wanted 0x%x\n", videocap_hw_ability.dma_out_fmt_ability, vpd_buf_type);
	}
	return HD_OK;
err:
	*valid = 0;
	return HD_OK;
}

HD_RESULT videocap_check_vcap_validdir(unsigned int dir, INT *valid, vpd_flip_t *flip_type)
{
	HD_RESULT result;
	if (!valid) {
		result = HD_ERR_NG;
		goto err_exit;
	}
	if (!flip_type) {
		*valid = 0;
		result = HD_ERR_NG;
		goto err_exit;
	}
	*valid = 1;
	switch (dir) {
	case HD_VIDEO_DIR_NONE:
		*flip_type = VPD_FLIP_NONE;
		break;
	case HD_VIDEO_DIR_MIRRORX:
		*flip_type = VPD_FLIP_HORIZONTAL;
		break;
	case HD_VIDEO_DIR_MIRRORY:
		*flip_type = VPD_FLIP_VERTICAL;
		break;
	case HD_VIDEO_DIR_MIRRORXY:
		*flip_type = VPD_FLIP_VERTICAL_AND_HORIZONTAL;
		break;
	default:
		*valid = 0;
		break;
	}
	return HD_OK;
err_exit:
	printf("videocap_check_vcap_validdir NG\n");
	return result;
}

HD_RESULT videocap_check_vcap_validdrc(unsigned int drc, INT *valid)
{
	HD_RESULT result = HD_ERR_NG;
	if (!valid) {
		goto err_exit;
	}
	*valid = 1;
	if (drc < 0x8) {
		return HD_OK;
	} else {
		goto err_exit;
	}
err_exit:
	return result;
}

HD_RESULT videocap_check_vcap_validfrc(unsigned int frc, INT *valid, INT *dst, INT *src)
{
	HD_RESULT result = HD_ERR_NG;
	INT dst_fps, src_fps;
	src_fps = (frc & 0x0000FFFF);
	dst_fps = (frc & 0xFFFF0000) >> 16;

	if ((!valid) || (!dst) || (!src)) {
		goto err_exit;
	}
	*valid = 1;
	if ((dst_fps == 0) || (src_fps == 0)) {
		goto default_v;
	}
	if ((dst_fps > VCAP_MAX_INFPS) || (src_fps > VCAP_MAX_INFPS) || (dst_fps > src_fps)) {
		goto default_v;
	}
	*dst = dst_fps;
	*src = src_fps;
	return HD_OK;
default_v:
	*dst = VCAP_DEFAULT_FPS;
	*src = VCAP_DEFAULT_FPS;
	//GMLIB_ERROR_P("dst = %u, %u  \n", *dst, *src);
	return result;
err_exit:
	GMLIB_ERROR_P("frc = 0x%x,  ", frc);
	GMLIB_ERROR_P("videocap_check_vcap_validfrc NG\n");
	return result;
}

HD_RESULT videocap_check_vcap_validfrc_fps(unsigned int value)
{
	HD_RESULT result = HD_ERR_NG;
	unsigned int dst_fps, src_fps;
	src_fps = (value & 0x0000FFFF);
	dst_fps = (value & 0xFFFF0000) >> 16;

	if ((dst_fps == 0) || (src_fps == 0)) {
		goto err_exit;
	}
	if ((dst_fps > 120) || (src_fps > 120) || (dst_fps > src_fps)) {
		goto err_exit;
	}

	return HD_OK;
err_exit:
	return result;
}

HD_RESULT videocap_push_in_buf_direct(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK, check_ret = HD_OK;
	usr_proc_trigger_capture_t usr_proc_trigger_cap;
	UINT32 device_subid = VDOCAP_DEVID(self_id);
	INT ret, fd, is_support, valid_wh;
	uintptr_t queue_handle;
	HD_DIM dim, out_dim, out_bg_dim, src_dim, src_align_dim;
	vpd_buf_type_t vpd_buf_type;
	HD_VIDEOCAP_CROP *param;
	UINT estimate_size = 0, query_size = 0;
	HD_PATH_ID path_id = GET_VDEC_PATHID(self_id, 1, out_id);
	HD_VIDEO_PXLFMT pxlfmt;
	CHAR pool_name[20] = {};
	HD_VIDEOCAP_OUT *cap_out;


	if (VDOCAP_DEVID(self_id) > videocap_max_device_nr) {
		HD_VIDEOCAPTURE_ERR("Error self_id(%d)\n", self_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}
	if (VDOCAP_OUTID(out_id) >= videocap_max_device_out) {
		HD_VIDEOCAPTURE_ERR("Error out_id(%d)\n", out_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}

	if (p_video_frame == NULL) {
		HD_VIDEOCAPTURE_ERR("p_video_frame NULL\n");
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	if (p_video_frame->phy_addr[0] == 0) {
		HD_VIDEOCAPTURE_ERR("phy_addr NULL\n");
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	//trigger mode
	pxlfmt = p_video_frame->pxlfmt;
	fd = g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].fd;
	queue_handle = g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].queue_handle;
	dim.w = p_video_frame->dim.w;
	dim.h = p_video_frame->dim.h;
	valid_wh = validate_cap_out_dim(dim.w, dim.h, pxlfmt);
	if (valid_wh != TRUE) {
		printf("videocap_push_in_buf_direct vcapout invalid. w = %u, h = %u, outfmt = 0x%x.\n", dim.w, dim.h, pxlfmt);
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}

	validate_cap_dim_wh_align_odd(dim, pxlfmt, &out_dim, 0);
	validate_cap_dim_bg_align_dir(dim, pxlfmt, &out_bg_dim, 1);

	if (p_video_frame->reserved[1] == VCAP_PUSHPULL_SIZE_MAGIC) {
		query_size = p_video_frame->reserved[0];
	} else {
		ret = pif_query_video_buffer_size(p_video_frame->phy_addr[0],
						  p_video_frame->ddr_id,
						  pool_name, &query_size);
		if (ret == -2) {
			/* can not find pool by this pa, skip it */
			hd_ret = HD_OK;
		} else if (ret < 0) {
			HD_VIDEOCAPTURE_ERR("query_video_buffer_size fail\n");
			hd_ret = HD_ERR_NOBUF;
			goto exit;
		}
	}
	estimate_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(out_bg_dim, p_video_frame->pxlfmt);
	if (query_size == 0) {
		HD_VIDEOCAPTURE_ERR("path_id(%#x) query_size zero\n", path_id);
		printf("path_id(%#x) Fail buffer address %#lx\n", path_id, p_video_frame->phy_addr[0]);
		//system("cat /proc/videograph/ms/fixed/user_blk_ddr0");
		hd_ret = HD_ERR_NOT_FOUND;
		goto exit;
	}
	if (query_size < estimate_size) {
		HD_VIDEOCAPTURE_ERR("path_id(%#x) p_user_out_video_frame too small, buf size(%u) < set size(%u) by wh(%u,%u) pxlfmt(%#x)\n",
				    path_id, query_size, estimate_size, out_bg_dim.w, out_bg_dim.h, p_video_frame->pxlfmt);
		if ((dim.w != out_bg_dim.w) || (dim.h != out_bg_dim.h)) {
			printf("set dim w = %u %u, out_dim %u, %u\n", dim.w, dim.h, out_dim.w, out_dim.h);
			printf("real dimbg_w = %u %u\n", out_bg_dim.w, out_bg_dim.h);
			//printf("real w = %u, h = %u (set w = %u, h = %u)\n", out_dim.w, out_dim.h, dim.w, dim.h);
		}

		hd_ret = HD_ERR_NOBUF;
		goto exit;
	}

	//check the output type if capture can support it
	vpd_buf_type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_video_frame->pxlfmt);
	check_ret = videocap_check_vcap_validtype(vpd_buf_type, &is_support);
	if (check_ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("videocap_check_vcap_validtype check error\n");
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	if (!is_support) {
		HD_VIDEOCAPTURE_ERR("videocap doesn't support for pxlfmt(%#x), please 'cat /proc/videograph/sys_spec' for capability\n",
				    p_video_frame->pxlfmt);
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}

	memset(&usr_proc_trigger_cap, 0, sizeof(usr_proc_trigger_cap));
	usr_proc_trigger_cap.fd = fd;
	usr_proc_trigger_cap.out_frame_buffer.ddr_id = p_video_frame->ddr_id;
	usr_proc_trigger_cap.out_frame_buffer.addr_pa = p_video_frame->phy_addr[0];
	if (pif_address_ddr_sanity_check(usr_proc_trigger_cap.out_frame_buffer.addr_pa, usr_proc_trigger_cap.out_frame_buffer.ddr_id) < 0) {
		HD_VIDEOCAPTURE_ERR("%s:Check out_pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)usr_proc_trigger_cap.out_frame_buffer.addr_pa,
			usr_proc_trigger_cap.out_frame_buffer.ddr_id);
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	usr_proc_trigger_cap.out_frame_buffer.size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(out_bg_dim, p_video_frame->pxlfmt);  //TODO
	usr_proc_trigger_cap.out_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_video_frame->pxlfmt);
	usr_proc_trigger_cap.out_frame_info.dim.w = dim.w;
	usr_proc_trigger_cap.out_frame_info.dim.h = dim.h;
	src_dim.w = platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].width;
	src_dim.h = platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].height;
	validate_cap_dim_bg_align_dir(src_dim, pxlfmt, &src_align_dim, 1);
	cap_out = &g_video_cap_info[device_subid].cap_out[VDOCAP_OUTID(out_id)];
	usr_proc_trigger_cap.flip_mode = videocap_get_flip_param(cap_out->dir);
	usr_proc_trigger_cap.out_frame_info.pathid = p_video_frame->reserved[3];
	// 1080 is a special number for vcap
	// vcap height should align to 16, but 1080 is ok.
	// to avoid height from AP (src_align_dim.h) is wrong, 1080 is changed here (vcap sysinfo is 1080, not 1088)
	if (src_align_dim.h == 1088) {
		if (src_dim.h >= 1080)//32x vcap max h = 1080
			usr_proc_trigger_cap.src_bg_dim.h = 1080;
		else
			usr_proc_trigger_cap.src_bg_dim.h = src_dim.h;

	} else {
		usr_proc_trigger_cap.src_bg_dim.h = src_align_dim.h;//platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].height;
	}

	usr_proc_trigger_cap.src_bg_dim.w = src_align_dim.w;//platform_sys_Info.cap_info[VDOCAP_DEVID(self_id)].width;
	usr_proc_trigger_cap.dst_bg_dim.w = out_bg_dim.w;
	usr_proc_trigger_cap.dst_bg_dim.h = out_bg_dim.h;
	usr_proc_trigger_cap.info.ch = VDOCAP_DEVID(self_id);
	usr_proc_trigger_cap.info.path = VDOCAP_OUTID(out_id);
	if (videocap_check_vcap_validfrc_fps(p_video_frame->reserved[6]) == HD_OK) {
		usr_proc_trigger_cap.info.frame_rate = p_video_frame->reserved[6] ;
	} else {
		usr_proc_trigger_cap.info.frame_rate = 30;
	}
	param = &g_video_cap_info[device_subid].cap_crop[VDOCAP_OUTID(out_id)];
	if (param->mode == HD_CROP_ON) {
		usr_proc_trigger_cap.scale.enable = 1;

		usr_proc_trigger_cap.scale.src_crop.x  = param->win.rect.x;
		usr_proc_trigger_cap.scale.src_crop.y  = param->win.rect.y;
		usr_proc_trigger_cap.scale.src_crop.w  = param->win.rect.w;
		usr_proc_trigger_cap.scale.src_crop.h  = param->win.rect.h;
		usr_proc_trigger_cap.scale.dst_rect.x  = 0;
		usr_proc_trigger_cap.scale.dst_rect.y  = 0;
		usr_proc_trigger_cap.scale.dst_rect.w  = out_dim.w;
		usr_proc_trigger_cap.scale.dst_rect.h  = out_dim.h;
	} else {
		usr_proc_trigger_cap.scale.enable = 0;
	}
	usr_proc_trigger_cap.queue_handle = queue_handle;
	usr_proc_trigger_cap.p_user_data = (void *) p_video_frame->blk;
	usr_proc_trigger_cap.ext_manual_v = out_bg_dim.h - out_dim.h;//verical
	usr_proc_trigger_cap.ext_manual_h = out_bg_dim.w - out_dim.w;//horizontal
	if (p_video_frame->reserved[4] < DRC_MODE_MAX) {
		usr_proc_trigger_cap.drc_mode = p_video_frame->reserved[4];
	} else {
		usr_proc_trigger_cap.drc_mode = 0;
	}

	if ((ret = ioctl(vpd_fd, USR_PROC_TRIGGER_CAPTURE, &usr_proc_trigger_cap)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		hd_ret = ret;
		HD_VIDEOCAPTURE_ERR("<ioctl \"USR_PROC_TRIGGER_CAPTURE\" fail, error(%d)>\n", ret);
		goto exit;
	}
	//if ((dim.w != out_dim.w)||(dim.h != out_dim.h)) {
	//printf("real w = %u, h = %u (set w = %u, h = %u)\n", out_dim.w, out_dim.h, dim.w, dim.h);
	p_video_frame->dim.w = out_dim.w;
	p_video_frame->dim.h = out_dim.h;
	p_video_frame->pw[0] = out_bg_dim.w;
	p_video_frame->ph[0] = out_bg_dim.h;
	//}

exit:
	return hd_ret;
}

HD_RESULT videocap_pull_out_buf_direct(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_cap_cb_info_t usr_proc_cap_cb_info = {0};
	UINT32 device_subid = VDOCAP_DEVID(self_id);
	INT ret;
	uintptr_t queue_handle;


	if (VDOCAP_DEVID(self_id) > videocap_max_device_nr) {
		HD_VIDEOCAPTURE_ERR("Error self_id(%d)\n", self_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}
	if (VDOCAP_OUTID(out_id) >= videocap_max_device_out) {
		HD_VIDEOCAPTURE_ERR("Error out_id(%d),(%d)\n", out_id, videocap_max_device_out);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}

	if (p_video_frame == NULL) {
		HD_VIDEOCAPTURE_ERR("p_video_frame NULL\n");
		hd_ret = HD_ERR_PARAM;
		goto exit;
	}
	queue_handle = g_video_cap_info[device_subid].priv[VDOCAP_OUTID(out_id)].queue_handle;

	usr_proc_cap_cb_info.queue_handle = queue_handle;
	usr_proc_cap_cb_info.wait_ms = wait_ms;
	if ((ret = ioctl(vpd_fd, USR_PROC_RECV_DATA_CAPTURE, &usr_proc_cap_cb_info)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		HD_VIDEOCAPTURE_ERR("<ioctl \"USR_PROC_RECV_DATA_CAPTURE\" fail, error(%d)(ms=%d)>\n", ret, wait_ms);
		if (ret == -15) {
			hd_ret = HD_ERR_TIMEDOUT;
		} else {
			hd_ret = HD_ERR_SYS;
		}
		goto exit;
	}
	p_video_frame->phy_addr[0] = usr_proc_cap_cb_info.addr_pa;

	p_video_frame->timestamp = usr_proc_cap_cb_info.timestamp * 1000;
	p_video_frame->blk = (typeof(p_video_frame->blk)) usr_proc_cap_cb_info.p_user_data;
	if (usr_proc_cap_cb_info.status == 1)  { //1:job finished, 0:ongoing, -1:fail
		p_video_frame->dim.w = usr_proc_cap_cb_info.out_w;
		p_video_frame->dim.h = usr_proc_cap_cb_info.out_h;
		p_video_frame->pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(usr_proc_cap_cb_info.out_fmt);
		hd_ret = HD_OK;
	} else if (usr_proc_cap_cb_info.status == 0) {
		HD_VIDEOCAPTURE_ERR("status ongoing \n");
		hd_ret = HD_ERR_TIMEDOUT;
	} else {
		hd_ret = HD_ERR_FAIL;
	}


exit:
	if (hd_ret != HD_OK) {
		HD_VIDEOCAPTURE_ERR("pull out buffer EXIT, ret(%#x)\n", hd_ret);
	}
	return hd_ret;
}

