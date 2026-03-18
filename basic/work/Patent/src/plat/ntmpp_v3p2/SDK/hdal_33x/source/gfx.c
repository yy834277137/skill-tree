/**
 * @file gfx.c
 * @brief type definition of API.
 * @author PSW
 * @date in the year 2018
 */

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <string.h>
#include <sys/ioctl.h>
#include "hd_type.h"
#include "hdal.h"
#include "gfx.h"
#include "hdal.h"
#include "cif_common.h"
#include "pif.h"
#include "pif_ioctl.h"
//#include "2d_lib/gm2d_gfx.h"
#include "gfx_ioctl.h"
#define SSCA_MAX_SCL_RATIO  16
#define COLOUR1555(val)  (((val) & 0x00007c00) << 17 | ((val) & 0x000003e0) << 14 | ((val) & 0x0000001f) << 11| ((val) & 0x00008000) >> 8)
#define COLOUR8888(val)  (((val) & 0x00ff0000) << 8 | ((val) & 0x0000ff00) << 8 | ((val) & 0x000000ff) << 8 | ((val) & 0xff000000) >> 24)
#define HD_VIDEO_PXLFMT_YUV444ONE_ALPHA  0x51181444 //<HD_VIDEO_PXLFMT_YUV444_ONE+alpha format,p_phy_addr[0]:yuv444_one ,p_phy_addr[1]:alpha  

struct gfx_syscaps gfx_syscap_info = {0};
/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* External Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern void *pif_alloc_video_buffer_for_hdal(int size, int ddr_id, char pool_name[], int alloc_tag);
extern int pif_free_video_buffer_for_hdal(void *pa, int ddr_id, int alloc_tag);
/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them
/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
static UINT8 gfx_is_init = 0;
int flow_gfx_fd = -1;

HD_RESULT gfx_init(void)
{
	HD_RESULT ret = HD_OK;
       
	if (gfx_is_init == 1) {
		HD_GFX_ERR("Error, gfx module is init,doesn't need to init\n");
		ret = HD_ERR_SYS;
		goto out;
	}
	gfx_is_init = 1;

	flow_gfx_fd = open("/dev/kflow_gfx", O_RDWR | O_SYNC);
	if (flow_gfx_fd < 0) {
		HD_GFX_ERR("Error, open /dev/kflow_gfx fail\n");
		ret = HD_ERR_SYS;
		goto out;
	}
       if ((ioctl(flow_gfx_fd, IOCTL_GFX_GET_SYSCAPS, &gfx_syscap_info)) < 0) {
	     HD_GFX_ERR("ioctl \"IOCTL_GFX_GET_SYSCAPS\" fail");
             ret = HD_ERR_SYS;
 	     goto out;
       }
out:
	return ret;
}

HD_RESULT gfx_uninit(void)
{
	HD_RESULT ret = HD_OK;

	if (gfx_is_init != 1) {
		HD_GFX_ERR("Error, gfx module is not init,doesn't need to uninit\n");
		ret = HD_ERR_SYS;
		goto err_exit;
	}
	gfx_is_init = 0;
	close(flow_gfx_fd);
	flow_gfx_fd = -1;
err_exit:
	return ret;
}
//根據fmt 和region 座標計算addr
int get_buf_offset(HD_GFX_IMG_BUF image_buf, unsigned long *addr0, unsigned long *addr1, unsigned long *addr2, HD_IRECT img_region)
{
	unsigned long start_addr;
	HD_IRECT crop_region;
	HD_DIM bg_dim;
	HD_VIDEO_PXLFMT format;

	format = image_buf.format;
	start_addr = image_buf.p_phy_addr[0];
	if (!start_addr) {
		HD_GFX_ERR("Check buf addr fail\n");
		return -1;
	}
	crop_region.x = img_region.x;
	crop_region.y = img_region.y;
	crop_region.w = img_region.w;
	crop_region.h = img_region.h;
	bg_dim.w = image_buf.dim.w;
	bg_dim.h = image_buf.dim.h;
	*addr0 = 0;
	*addr1 = 0;
    *addr2 = 0;
	switch ((UINT32)format) {
	case HD_VIDEO_PXLFMT_ARGB1555:
	case HD_VIDEO_PXLFMT_YUV422_ONE:
	case HD_VIDEO_PXLFMT_YUV422_YUYV:	
		*addr0 = (start_addr + ((crop_region.y * bg_dim.w + crop_region.x) * 2));
		break;
    case HD_VIDEO_PXLFMT_YUV444_ONE:
	   	*addr0 = (start_addr + ((crop_region.y * bg_dim.w + crop_region.x) * 3));
		break;
    case HD_VIDEO_PXLFMT_YUV444ONE_ALPHA:		   	
		*addr0 = (start_addr + ((crop_region.y * bg_dim.w + crop_region.x) * 3));
		*addr1 = image_buf.p_phy_addr[1];
		break; 
	case HD_VIDEO_PXLFMT_ARGB8888:
		*addr0 = (start_addr + ((crop_region.y * bg_dim.w + crop_region.x) * 4));
		break;
	case HD_VIDEO_PXLFMT_YUV420:
		*addr0 = (start_addr + ((crop_region.y * bg_dim.w) + crop_region.x));//y_offset_addr
		*addr1 = (start_addr + ((bg_dim.w * bg_dim.h) + (((crop_region.y * bg_dim.w) / 2 + crop_region.x)))); //uv_offset_addr
		break;
	case HD_VIDEO_PXLFMT_Y8:
		*addr0 = (start_addr + ((crop_region.y * bg_dim.w) + crop_region.x));//y_offset_addr
		break;
    case HD_VIDEO_PXLFMT_RGB888:
		*addr0 = (start_addr + ((crop_region.y * bg_dim.w + crop_region.x) * 3));
		break;
	case HD_VIDEO_PXLFMT_RGB888_PLANAR:
		*addr0 = (start_addr + ((crop_region.y * bg_dim.w + crop_region.x) ));
	   	*addr1 = (start_addr + ((bg_dim.w * bg_dim.h) + (crop_region.y * bg_dim.w + crop_region.x)));
       	*addr2 = (start_addr + (((bg_dim.w * bg_dim.h)*2) + (crop_region.y * bg_dim.w + crop_region.x)));
		break;
	default:
		HD_GFX_ERR("Not support fmt(%#x)\n", format);
		return -1;
	}

	return 0;
}

static int validate_ssca_ratio(int sw, int sh, int dw, int dh)
{
	//max scale ratio is between 1/16x ~ 16x
	if (sw > dw && sw > SSCA_MAX_SCL_RATIO * dw) {
		printf("max ssca scale ratio is %d, src w(%d) h(%d), dst w(%d) h(%d)\n", sw, sh, dw, dh, SSCA_MAX_SCL_RATIO);
		return -1;
	} else if (sw < dw && SSCA_MAX_SCL_RATIO * sw < dw) {
		printf("max ssca scale ratio is %d, src w(%d) h(%d), dst w(%d) h(%d)\n", sw, sh, dw, dh, SSCA_MAX_SCL_RATIO);
		return -1;
	}

	if (sh > dh && sh > SSCA_MAX_SCL_RATIO * dh) {
		printf("max ssca scale ratio is %d, src w(%d) h(%d), dst w(%d) h(%d)\n", sw, sh, dw, dh, SSCA_MAX_SCL_RATIO);
		return -1;
	} else if (sh < dh && SSCA_MAX_SCL_RATIO * sh < dh) {
		printf("max ssca scale ratio is %d, src w(%d) h(%d), dst w(%d) h(%d)\n", sw, sh, dw, dh, SSCA_MAX_SCL_RATIO);
		return -1;
	}

	return 0;
}


static int gfx_get_ssca_fmt(HD_VIDEO_PXLFMT format)
{
	switch (format) {
	case HD_VIDEO_PXLFMT_Y8:
		return SSCA_FLOW_Y8_ONLY;
	case HD_VIDEO_PXLFMT_ARGB1555:
		return SSCA_FLOW_ARGB1555;
	case HD_VIDEO_PXLFMT_ARGB8888:
		return SSCA_FLOW_ARGB8888;
	case HD_VIDEO_PXLFMT_YUV422_ONE:
		return SSCA_FLOW_YUV422_UYVY;
	case HD_VIDEO_PXLFMT_YUV422_YUYV:
		return SSCA_FLOW_YUV422_YUYV;
	case HD_VIDEO_PXLFMT_YUV422_VYUY:
		return SSCA_FLOW_YUV422_VYUY;
	case HD_VIDEO_PXLFMT_YUV420:
		return SSCA_FLOW_YUV420_SP_Y_VU;
	default:
		HD_GFX_ERR("Not support fmt(%#x)\n", format);
		return SSCA_FLOW_FMT_MAX;
	}
}

HD_RESULT gfx_get_lofs_by_fmt(HD_VIDEO_PXLFMT format, int image_w, UINT16 *line_offset_0, UINT16 *line_offset_1)
{
	if (!line_offset_0) {
		HD_GFX_ERR("check line_offset_0 null\n");
		return HD_ERR_NULL_PTR;
	}
	
	if (format == HD_VIDEO_PXLFMT_RGB888_PLANAR || format == HD_VIDEO_PXLFMT_Y8) {
		*line_offset_0 = image_w;  		
	} else if (format == HD_VIDEO_PXLFMT_YUV420) {
		*line_offset_0 = image_w;
		if(line_offset_1)
			*line_offset_1 = image_w;
	} else if (format == HD_VIDEO_PXLFMT_YUV422_ONE || format == HD_VIDEO_PXLFMT_ARGB1555 ||
		format == HD_VIDEO_PXLFMT_RGB565  || format == HD_VIDEO_PXLFMT_YUV422_YUYV) { 
		*line_offset_0 = (image_w * 2);
	} else if (format == HD_VIDEO_PXLFMT_YUV444_ONE || format == HD_VIDEO_PXLFMT_RGB888) {
		*line_offset_0 = (image_w *3);
	} else if (format == HD_VIDEO_PXLFMT_YUV444ONE_ALPHA) {
		*line_offset_0 = (image_w *3);
		if(line_offset_1)
			*line_offset_1 = image_w ;
	} else if (format == HD_VIDEO_PXLFMT_ARGB8888) {
		*line_offset_0 = (image_w * 4);
	}  else {
		HD_GFX_ERR("Not support fmt\n");
		return HD_ERR_NOT_SUPPORT;
	}
	return HD_OK;
}

//[0]:alpha, [1]:data0, [2]:data1, [3]:data2, ex:ARGB 0 ~ 255
void gfx_transfer_color_data(HD_VIDEO_PXLFMT format, UINT8 *color_data, UINT32 color_val)
{
	if (format == HD_VIDEO_PXLFMT_ARGB1555) {
		color_data[0] = (color_val & 0xFF) ? 255 : 0;//transfer alpha 1 to 255 else 0
		color_data[1] = ((color_val >> 8) & 0x1F) << 3;//r
		color_data[2] = ((color_val >> 16) & 0x1F) << 3;//g
		color_data[3] = ((color_val >> 24) & 0x1F) << 3;//b
	} else {
		color_data[0] = (color_val & 0xFF);
		color_data[1] = ((color_val >> 8) & 0xFF);//r
		color_data[2] = ((color_val >> 16) & 0xFF);//g
		color_data[3] = ((color_val >> 24) & 0xFF);//b
	}

}

static enum age_flow_fmt gfx_get_age_fmt(HD_VIDEO_PXLFMT format)
{
	switch ((UINT32)format) {        
    case HD_VIDEO_PXLFMT_RGB888_PLANAR://rgb888_3_plane
        return AGE_FLOW_FMT_R_G_B;                 
    case HD_VIDEO_PXLFMT_RGB888://rgb888_one
        return AGE_FLOW_FMT_RGB;
	case HD_VIDEO_PXLFMT_RGB565:	
		return AGE_FLOW_FMT_RGB_565;	
	case HD_VIDEO_PXLFMT_Y8:
		return AGE_FLOW_FMT_Y;
	case HD_VIDEO_PXLFMT_ARGB1555:
		return AGE_FLOW_FMT_ARGB_1555;
	case HD_VIDEO_PXLFMT_RGBA5551:
		return AGE_FLOW_FMT_RGBA_5551;
	case HD_VIDEO_PXLFMT_ARGB8888:
		return AGE_FLOW_FMT_ARGB;
	case HD_VIDEO_PXLFMT_YUV422:
	case HD_VIDEO_PXLFMT_YUV422_ONE:
		return AGE_FLOW_FMT_UYVY;
	case HD_VIDEO_PXLFMT_YUV420:
		return AGE_FLOW_FMT_Y_VU_420;//for only support nv21 fmt
	case HD_VIDEO_PXLFMT_I8:
		return AGE_FLOW_FMT_PALETTE_8;
	case HD_VIDEO_PXLFMT_YUV444_ONE:
		return AGE_FLOW_FMT_YUV;
    case HD_VIDEO_PXLFMT_YUV444ONE_ALPHA:
		return AGE_FLOW_FMT_YUVA;
	default:
		HD_GFX_ERR("Not support fmt(%#x)\n", format);
		return AGE_FLOW_FMT_MAX;
	}
}

static enum age_flow_flip_rot_mode gfx_get_age_roate_angle(UINT32 angle)
{
	enum age_flow_flip_rot_mode gfx_rotate_angle = AGE_FLOW_ROTATE_0;

	switch (angle) {
	case HD_VIDEO_DIR_ROTATE_90:
		gfx_rotate_angle = AGE_FLOW_ROTATE_90;
		break;
	case HD_VIDEO_DIR_ROTATE_180:
		gfx_rotate_angle = AGE_FLOW_ROTATE_180;
		break;
	case HD_VIDEO_DIR_ROTATE_270:
		gfx_rotate_angle = AGE_FLOW_ROTATE_270;
		break;
	case HD_VIDEO_DIR_MIRRORX:
		gfx_rotate_angle = AGE_FLOW_H_FLIP_ROTATE_180;
		break;
	case HD_VIDEO_DIR_MIRRORY:
		gfx_rotate_angle = AGE_FLOW_H_FLIP_ROTATE_0;
		break;
	case (HD_VIDEO_DIR_MIRRORY| HD_VIDEO_DIR_ROTATE_90):
		gfx_rotate_angle = AGE_FLOW_H_FLIP_ROTATE_90;
		break;
	case (HD_VIDEO_DIR_MIRRORY| HD_VIDEO_DIR_ROTATE_270):
		gfx_rotate_angle = AGE_FLOW_H_FLIP_ROTATE_270;
		break;
	default:
		HD_GFX_ERR("Error, not support angle value(%#x)\n", angle);
		gfx_rotate_angle = AGE_FLOW_FLIP_ROTATE_MAX;
		break;

	}
	return gfx_rotate_angle;
}

HD_RESULT gfx_check_scale_param(HD_GFX_SCALE *p_param)
{
	HD_RESULT ret = HD_OK;

	if (!p_param->dst_img.dim.w || !p_param->dst_img.dim.h) {
		HD_GFX_ERR("%s:check dst dim(%d %d) fail\n", __func__, p_param->dst_img.dim.w, p_param->dst_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (gfx_get_ssca_fmt(p_param->dst_img.format) == SSCA_FLOW_FMT_MAX) {
		HD_GFX_ERR("%s:check dst fmt fail\n", __func__);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->dst_img.p_phy_addr[0] || p_param->dst_img.ddr_id  >= DDR_ID_MAX) {
		HD_GFX_ERR("%s:check dst pa(%#lx) and ddrid(%d) fail\n", __func__, p_param->dst_img.p_phy_addr[0], p_param->dst_img.ddr_id);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->src_img.dim.w || !p_param->src_img.dim.h) {
		HD_GFX_ERR("%s:check src dim(%d %d) fail\n", __func__, p_param->src_img.dim.w, p_param->src_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (gfx_get_ssca_fmt(p_param->src_img.format) == SSCA_FLOW_FMT_MAX) {
		HD_GFX_ERR("%s:check src fmt fail\n", __func__);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->src_img.p_phy_addr[0] || p_param->src_img.ddr_id  >= DDR_ID_MAX) {
		HD_GFX_ERR("%s:check src pa(%#lx) and ddrid(%d) fail\n", __func__, p_param->src_img.p_phy_addr[0], p_param->src_img.ddr_id);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if ((UINT32)(p_param->dst_region.x + p_param->dst_region.w) > p_param->dst_img.dim.w ||
	    (UINT32)(p_param->dst_region.y + p_param->dst_region.h) > p_param->dst_img.dim.h) {
		HD_GFX_ERR("%s:dst regin(%d %d %d %d) > dst_dim(%d %d)\n", __func__, p_param->dst_region.x, p_param->dst_region.y,
			   p_param->dst_region.w, p_param->dst_region.h, p_param->dst_img.dim.w, p_param->dst_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if ((UINT32)(p_param->src_region.x + p_param->src_region.w) > p_param->src_img.dim.w ||
	    (UINT32)(p_param->src_region.y + p_param->src_region.h) > p_param->src_img.dim.h) {
		HD_GFX_ERR("%s:src regin(%d %d %d %d) > src_dim(%d %d)\n", __func__, p_param->src_region.x, p_param->src_region.y,
			   p_param->src_region.w, p_param->src_region.h, p_param->src_img.dim.w, p_param->src_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
err_exit:
	return ret;
}

HD_RESULT gfx_scale_imgs(HD_GFX_SCALE *p_param, UINT8 out_alpha_trans_th, UINT8 num_of_image)
{
    HD_RESULT ret = HD_OK;
    struct ssca_flow_in_info *in_info = NULL;
    struct ssca_flow_out_info *out_info = NULL;
    struct ssca_flow_job_info user_ssca_job_info = {0};
    int ioctl_ret = 0;
    UINT16 i;
    
    if (flow_gfx_fd < 0) {
        HD_GFX_ERR("%s:check flow_gfx_fd(%d) fail\n", __func__, flow_gfx_fd);
        ret = HD_ERR_NG;
        goto err_exit;
    }
    if (!p_param || !num_of_image) {
        HD_GFX_ERR("%s:check p_param=%p num_of_image=%d fail\n", __func__, p_param, num_of_image);
        ret = HD_ERR_NULL_PTR;
        goto err_exit;
    }
    
    for (i = 0; i < num_of_image; i++) {
        if (gfx_check_scale_param(&p_param[i]) != HD_OK) {
            HD_GFX_ERR("%s:check p_param fail\n", __func__);
            ret = HD_ERR_NG;
            goto err_exit;
        }
        if (validate_ssca_ratio(p_param[i].src_region.w, p_param[i].src_region.h, p_param[i].dst_region.w, p_param[i].dst_region.h)) {
            HD_GFX_ERR("validate_ssca_ratio fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }
    }
    in_info = calloc(num_of_image, sizeof(struct ssca_flow_in_info));
    if (!in_info) {
        HD_GFX_ERR("%s:call ssca_flow_in_info fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }
    out_info = calloc(num_of_image, sizeof(struct ssca_flow_out_info));
    if (!out_info) {
        HD_GFX_ERR("%s:call ssca_flow_out_info fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }
    for (i = 0; i < num_of_image; i++) {
        if (get_buf_offset(p_param[i].src_img, &in_info[i].addr[0], &in_info[i].addr[1], &in_info[i].addr[2], p_param[i].src_region) < 0) {
            HD_GFX_ERR("get_buf_offset fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }
        in_info[i].ddr_id = p_param[i].src_img.ddr_id;
        in_info[i].roi.w = p_param[i].src_region.w;
        in_info[i].roi.h = p_param[i].src_region.h;
        ret = gfx_get_lofs_by_fmt(p_param[i].src_img.format, p_param[i].src_img.dim.w, &in_info[i].roi.lofs, NULL);
        if (ret != HD_OK) {
            HD_GFX_ERR("gfx_get_lofs_by_fmt fail\n");
            ret = HD_ERR_DRV;
            goto err_exit;
        }
        in_info[i].fmt = gfx_get_ssca_fmt(p_param[i].src_img.format);
        if (in_info[i].fmt == SSCA_FLOW_FMT_MAX) {
            HD_GFX_ERR("gfx_ssca_fmt_mode fail\n");
            ret = HD_ERR_DRV;
            goto err_exit;
        }
        in_info[i].data_type = SSCA_FLOW_DATA_TYPE_FULL_RANGE;
        in_info[i].alpha_val = 255;
        if (get_buf_offset(p_param[i].dst_img, &out_info[i].addr[0], &out_info[i].addr[1], &out_info[i].addr[2], p_param[i].dst_region) < 0) {
            HD_GFX_ERR("get_buf_offset fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }
        out_info[i].ddr_id = p_param[i].dst_img.ddr_id;
        out_info[i].roi.w = p_param[i].dst_region.w;
        out_info[i].roi.h = p_param[i].dst_region.h;
		ret = gfx_get_lofs_by_fmt(p_param[i].dst_img.format, p_param[i].dst_img.dim.w, &out_info[i].roi.lofs, NULL);
        if (ret != HD_OK) {
            HD_GFX_ERR("gfx_get_lofs_by_fmt fail\n");
            ret = HD_ERR_DRV;
            goto err_exit;
        }
        out_info[i].fmt = gfx_get_ssca_fmt(p_param[i].dst_img.format);
        if (out_info[i].fmt == SSCA_FLOW_FMT_MAX) {
            HD_GFX_ERR("gfx_ssca_fmt_mode fail\n");
            ret = HD_ERR_DRV;
            goto err_exit;
        }
        out_info[i].data_type = SSCA_FLOW_DATA_TYPE_FULL_RANGE;
        out_info[i].alpha_trans_th = out_alpha_trans_th;
    }
    
    user_ssca_job_info.job_cnt = num_of_image;
    user_ssca_job_info.p_in_info = in_info;
    user_ssca_job_info.p_out_info = out_info;
    if ((ioctl_ret = ioctl(flow_gfx_fd, IOCTL_GFX_SCA, &user_ssca_job_info)) < 0) {
        HD_GFX_ERR("IOCTL_GFX_SCA fail, ioctl_ret(%d)\n", ioctl_ret);
        ret = HD_ERR_DRV;
    }
    
err_exit:
    if (in_info) {
        free(in_info);
    }
    if (out_info) {
        free(out_info);
    }
    return ret;
}

HD_RESULT gfx_fill_src2_info(struct age_flow_src2_info *p_in2_info, HD_GFX_IMG_BUF dst_img, HD_IRECT rect, UINT32 color)
{
	HD_RESULT ret = HD_OK;

    p_in2_info->en = 1;
    p_in2_info->pat_en = 1;
    p_in2_info->type = AGE_FLOW_DATA_TYPE_FULL_RANGE;
    p_in2_info->fmt = gfx_get_age_fmt(dst_img.format);
    if (p_in2_info->fmt == AGE_FLOW_FMT_MAX) {
        HD_GFX_ERR("gfx_get_age_fmt fail\n");
        ret = HD_ERR_DRV;
        goto err_exit;
    }
    p_in2_info->addr[0] = 0;
    p_in2_info->addr[1] = 0;
    p_in2_info->ddr_id = 0;
    //alpha_val:only valid at alpha depth = 1 bits condition
    //if alpha = 1, replace alpha = alpha_val[0] else alpha_val[1]
    p_in2_info->alpha_val[0] = 0;
    p_in2_info->alpha_val[1] = 255;
    p_in2_info->roi.w = rect.w;
    p_in2_info->roi.h = rect.h;
    ret = gfx_get_lofs_by_fmt(dst_img.format, dst_img.dim.w, &p_in2_info->roi.lofs[0], &p_in2_info->roi.lofs[1]);
    if (ret != HD_OK) {
        HD_GFX_ERR("gfx_get_lofs_by_fmt fail\n");
        ret = HD_ERR_DRV;
        goto err_exit;
    }
    //p_in2_info->roi.lofs[1] case
    p_in2_info->param.mode = AGE_FLOW_FILL_NORMAL;
    gfx_transfer_color_data(dst_img.format, p_in2_info->param.fill.data, color);
err_exit:
    return ret;
}

HD_RESULT gfx_fill_out_info(struct age_flow_dst_info *p_out_info, HD_GFX_IMG_BUF dst_img, UINT8 dithering_en)
{
    HD_RESULT ret = HD_OK;

     p_out_info->ddr_id = dst_img.ddr_id;
     p_out_info->fmt = gfx_get_age_fmt(dst_img.format);
     if (p_out_info->fmt == AGE_FLOW_FMT_MAX) {
         HD_GFX_ERR("gfx_get_age_fmt fail\n");
         ret = HD_ERR_NG;
         goto err_exit;
      }
      ret = gfx_get_lofs_by_fmt(dst_img.format, dst_img.dim.w, &p_out_info->lofs[0] , &p_out_info->lofs[1]);
      if (ret != HD_OK) {
      	HD_GFX_ERR("gfx_get_lofs_by_fmt fail\n");
        ret = HD_ERR_NG;
        goto err_exit;
      }
      if (dst_img.format == HD_VIDEO_PXLFMT_YUV420) {
           p_out_info->lofs[1] = p_out_info->lofs[0];
		   p_out_info->lofs[2] = 0;
       }  else if (dst_img.format == HD_VIDEO_PXLFMT_RGB888_PLANAR) {
           p_out_info->lofs[1] = p_out_info->lofs[0];
           p_out_info->lofs[2] = p_out_info->lofs[0];
       }  else {
           p_out_info->lofs[1] = 0;
	       p_out_info->lofs[2] = 0;
       }
        p_out_info->type = AGE_FLOW_DATA_TYPE_FULL_RANGE;
        p_out_info->key.key_en = 0;
        p_out_info->dithering_en = dithering_en;
err_exit:
	return ret;
}

HD_RESULT gfx_check_copy_param(HD_GFX_COPY *p_param)
{
	HD_RESULT ret = HD_OK;

	if (!p_param->dst_img.dim.w || !p_param->dst_img.dim.h) {
		HD_GFX_ERR("%s:check dst dim(%d %d) fail\n", __func__, p_param->dst_img.dim.w, p_param->dst_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (gfx_get_age_fmt(p_param->dst_img.format) == AGE_FLOW_FMT_MAX) {
		HD_GFX_ERR("%s:check dst fmt fail\n", __func__);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->dst_img.p_phy_addr[0] || p_param->dst_img.ddr_id  >= DDR_ID_MAX) {
		HD_GFX_ERR("%s:check dst pa(%#lx) and ddrid(%d) fail\n", __func__, p_param->dst_img.p_phy_addr[0], p_param->dst_img.ddr_id);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->src_img.dim.w || !p_param->src_img.dim.h) {
		HD_GFX_ERR("%s:check src dim(%d %d) fail\n", __func__, p_param->src_img.dim.w, p_param->src_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (gfx_get_age_fmt(p_param->src_img.format) == AGE_FLOW_FMT_MAX) {
		HD_GFX_ERR("%s:check src fmt fail\n", __func__);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->src_img.p_phy_addr[0] || p_param->src_img.ddr_id  >= DDR_ID_MAX) {
		HD_GFX_ERR("%s:check src pa(%#lx) and ddrid(%d) fail\n", __func__, p_param->src_img.p_phy_addr[0], p_param->src_img.ddr_id);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if ((UINT32)(p_param->src_region.x + p_param->src_region.w) > p_param->src_img.dim.w ||
	    (UINT32)(p_param->src_region.y + p_param->src_region.h) > p_param->src_img.dim.h) {
		HD_GFX_ERR("%s:check src region(%d %d %d %d) > src dim(%d %d)\n", __func__, p_param->src_region.x, p_param->src_region.y,
			   p_param->src_region.w, p_param->src_region.h, p_param->src_img.dim.w, p_param->src_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if ((UINT32)(p_param->dst_pos.x + p_param->src_region.w) > p_param->dst_img.dim.w ||
	    (UINT32)(p_param->dst_pos.y + p_param->src_region.h) > p_param->dst_img.dim.h) {
		HD_GFX_ERR("%s:check dst pos(%d %d) src region(%d %d) > dst dim(%d %d)\n", __func__, p_param->dst_pos.x, p_param->dst_pos.y,
			   p_param->src_region.w, p_param->src_region.h, p_param->dst_img.dim.w, p_param->dst_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
err_exit:
	return ret;
}

HD_RESULT gfx_copy_imgs(HD_GFX_COPY *p_param, UINT8 num_of_image, UINT8 dithering_en)
{
    HD_RESULT ret = HD_OK;
    struct age_flow_info age_info = {0};
    struct age_flow_src1_info *in1_info = NULL;
    struct age_flow_src2_info *in2_info = NULL;
    HD_IRECT region;
    struct age_flow_blend_param *blend_param = NULL;
    struct age_flow_dst_info *out_info = NULL;
    int ioctl_ret = 0;
    UINT16 i;
    
    if (flow_gfx_fd < 0) {
        HD_GFX_ERR("%s:check flow_gfx_fd(%d) fail\n", __func__, flow_gfx_fd);
        ret = HD_ERR_NG;
        goto err_exit;
    }
    
    if (!p_param || !num_of_image) {
        HD_GFX_ERR("%s:check p_param=%p num_of_image=%d fail\n", __func__, p_param, num_of_image);
        ret = HD_ERR_NULL_PTR;
        goto err_exit;
    }
    
    for (i = 0; i < num_of_image; i++) {
        if (gfx_check_copy_param(&p_param[i]) != HD_OK) {
            HD_GFX_ERR("%s:check p_param fail\n", __func__);
            ret = HD_ERR_NG;
            goto err_exit;
        }
    }
    in1_info = calloc(num_of_image, sizeof(struct age_flow_src1_info));
    if (!in1_info) {
        HD_GFX_ERR("%s:call age_flow_src1_info fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }
    in2_info = calloc(num_of_image, sizeof(struct age_flow_src2_info));
    if (!in2_info) {
        HD_GFX_ERR("%s:call age_flow_src2_info fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }
    blend_param = calloc(num_of_image, sizeof(struct age_flow_blend_param));
    if (!blend_param) {
        HD_GFX_ERR("%s:call age_flow_blend_param fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }
    out_info = calloc(num_of_image, sizeof(struct age_flow_dst_info));
    if (!out_info) {
        HD_GFX_ERR("%s:call age_flow_dst_info fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }    

    for (i = 0 ; i < num_of_image; i++)  {
        in1_info[i].en = 1;
        if (p_param[i].alpha == 1) {
            in2_info[i].en = 1;
            blend_param[i].en = 1;
            blend_param[i].type = AGE_FLOW_BLEND_NONE;
            blend_param[i].src1_alpha.mode = AGE_FLOW_ALPHA_SRC1;
            blend_param[i].src1_alpha.val  = 0;
        } else {
            in2_info[i].en = 0;
            blend_param[i].en = 0;
        }

        in1_info[i].ddr_id = p_param[i].src_img.ddr_id;
        in1_info[i].fmt = gfx_get_age_fmt(p_param[i].src_img.format);
        if (in1_info[i].fmt == AGE_FLOW_FMT_MAX) {
            HD_GFX_ERR("gfx_get_age_fmt fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }
        if (p_param[i].src_img.format == HD_VIDEO_PXLFMT_ARGB1555) {
            in1_info[i].alpha_val[0] = 0;
            in1_info[i].alpha_val[1] = 255;
        }
        if (get_buf_offset(p_param[i].src_img, &in1_info[i].addr[0], &in1_info[i].addr[1], &in1_info[i].addr[2], p_param[i].src_region) < 0) {
            HD_GFX_ERR("get_buf_offset fail\n");
            ret = HD_ERR_NG;
            goto err_exit;

        }        
        ret = gfx_get_lofs_by_fmt(p_param[i].src_img.format, p_param[i].src_img.dim.w, &in1_info[i].roi.lofs[0], &in1_info[i].roi.lofs[1]);
        if (ret != HD_OK) {
            HD_GFX_ERR("gfx_get_lofs_by_fmt fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }

        in1_info[i].roi.w = p_param[i].src_region.w;
        in1_info[i].roi.h = p_param[i].src_region.h;
        in1_info[i].type = AGE_FLOW_DATA_TYPE_FULL_RANGE;
        in1_info[i].op = AGE_FLOW_OP_BYPASS;

        region.x = p_param[i].dst_pos.x;
        region.y = p_param[i].dst_pos.y;
        region.w = p_param[i].dst_img.dim.w;
        region.h = p_param[i].dst_img.dim.h;

        if (in2_info[i].en) {
            in2_info[i].pat_en = 0;
            in2_info[i].ddr_id  = p_param[i].dst_img.ddr_id;
            if (p_param[i].dst_img.format == HD_VIDEO_PXLFMT_ARGB1555) {
                in2_info[i].alpha_val[0] = 0;
                in2_info[i].alpha_val[1] = 255;
            }
            if (get_buf_offset(p_param[i].dst_img, &in2_info[i].addr[0], &in2_info[i].addr[1], &in2_info[i].addr[2], region) < 0) {
                   HD_GFX_ERR("get_buf_offset fail\n");
                   ret = HD_ERR_NG;
                   goto err_exit;
             }
        
            in2_info[i].fmt = gfx_get_age_fmt(p_param[i].dst_img.format);
            if (in2_info[i].fmt == AGE_FLOW_FMT_MAX) {
                HD_GFX_ERR("gfx_get_age_fmt fail\n");
                ret = HD_ERR_NG;
                goto err_exit;
            }
            in2_info[i].roi.w = in1_info[i].roi.w;
            in2_info[i].roi.h = in1_info[i].roi.h;
            in2_info[i].type = AGE_FLOW_DATA_TYPE_FULL_RANGE;
            in2_info[i].param.mode = AGE_FLOW_FILL_NORMAL;
            if (in2_info[i].roi.w < 0 || in2_info[i].roi.h < 0) {
                HD_GFX_ERR("Check  in2_info.roi.w=%d in2_info.roi.h=%d fail\n", in2_info[i].roi.w, in2_info[i].roi.h);
                ret = HD_ERR_NG;
                goto err_exit;
            }
            ret = gfx_get_lofs_by_fmt(p_param[i].dst_img.format, p_param[i].dst_img.dim.w, &in2_info[i].roi.lofs[0], &in2_info[i].roi.lofs[1]);
            if (ret != HD_OK) {
                HD_GFX_ERR("gfx_get_lofs_by_fmt fail\n");
                ret = HD_ERR_NG;
                goto err_exit;
            }
        }
        if (get_buf_offset(p_param[i].dst_img, &out_info[i].addr[0], &out_info[i].addr[1],&out_info[i].addr[2],  region) < 0) {
               HD_GFX_ERR("get_buf_offset fail\n");
               ret = HD_ERR_NG;
               goto err_exit;
       }

       if(gfx_fill_out_info(&out_info[i], p_param[i].dst_img, dithering_en) != HD_OK)  {
              HD_GFX_ERR("gfx_fill_out_info fail\n");
              ret = HD_ERR_NG;
              goto err_exit;
        }
    }
    
    age_info.job_cnt = num_of_image;
    age_info.p_in1_info = in1_info;
    age_info.p_in2_info = in2_info;
    age_info.p_blend_param = blend_param;
    age_info.p_out_info = out_info;
    if ((ioctl_ret = ioctl(flow_gfx_fd, IOCTL_GFX_AGE_COPY, &age_info)) < 0) {
        HD_GFX_ERR("IOCTL_GFX_AGE_COPY fail, ioctl_ret(%d)\n", ioctl_ret);
        ret = HD_ERR_DRV;
    }
    
err_exit:
    if (in1_info) {
        free(in1_info);
    }
    if (in2_info) {
        free(in2_info);
    }
    if (blend_param) {
        free(blend_param);
    }
    if (out_info) {
        free(out_info);
    }
    return ret;
}

HD_RESULT gfx_check_rect_param(HD_GFX_DRAW_RECT *p_param)
{
	HD_RESULT ret = HD_OK;

	if (!p_param->dst_img.dim.w || !p_param->dst_img.dim.h) {
		HD_GFX_ERR("%s:check dim(%d %d) fail\n", __func__, p_param->dst_img.dim.w, p_param->dst_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (gfx_get_age_fmt(p_param->dst_img.format) == AGE_FLOW_FMT_MAX) {
		HD_GFX_ERR("%s:check fmt fail\n", __func__);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->dst_img.p_phy_addr[0] || p_param->dst_img.ddr_id  >= DDR_ID_MAX) {
		HD_GFX_ERR("%s:check pa(%#lx) and ddrid(%d) fail\n", __func__, p_param->dst_img.p_phy_addr[0], p_param->dst_img.ddr_id);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if ((UINT32)(p_param->rect.x + p_param->rect.w) > p_param->dst_img.dim.w || (UINT32)(p_param->rect.y + p_param->rect.h) > p_param->dst_img.dim.h) {
		HD_GFX_ERR("%s:rect(%d %d %d %d) > dst_dim(%d %d)\n", __func__, p_param->rect.x, p_param->rect.y, p_param->rect.w, p_param->rect.h,
			   p_param->dst_img.dim.w, p_param->rect.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
err_exit:
	return ret;
}

HD_RESULT gfx_draw_rects(HD_GFX_DRAW_RECT *p_param, UINT8 num_of_rect)
{
    HD_RESULT ret = HD_OK;
    struct age_flow_info age_info = {0};
    struct age_flow_src1_info in1_info = {0};
    struct age_flow_src2_info *in2_info = NULL;
    struct age_flow_blend_param blend_param = {0};
    struct age_flow_dst_info *out_info = NULL;
    int ioctl_ret = 0;
    UINT16 i;
    HD_IRECT line_rect;
    UINT16 rect_idx;
    UINT16 need_rect_num = 0;
    UINT16 param_idx = 0;
    
    if (flow_gfx_fd < 0) {
        HD_GFX_ERR("%s:check flow_gfx_fd(%d) fail\n", __func__, flow_gfx_fd);
        ret = HD_ERR_NG;
        goto err_exit;
    }
    
    if (!p_param || !num_of_rect) {
        HD_GFX_ERR("%s:check p_param=%p num_of_rect=%d fail\n", __func__, p_param, num_of_rect);
        ret = HD_ERR_NULL_PTR;
        goto err_exit;
    }
    for (i = 0 ; i < num_of_rect; i++)  {//cal need rect number,hollow rect use 4 rect to implement
         if (p_param[i].type == HD_GFX_RECT_HOLLOW) {
            need_rect_num += 4;
        }  else if (p_param[i].type == HD_GFX_RECT_SOLID) {
            need_rect_num += 1;
        } else {
            ret = HD_ERR_PARAM;
            goto err_exit;
         }
    } 
    for (param_idx = 0; param_idx < num_of_rect; param_idx++) {
        if (gfx_check_rect_param(&p_param[param_idx]) != HD_OK) {
            HD_GFX_ERR("%s:check p_param fail\n", __func__);
            ret = HD_ERR_NG;
            goto err_exit;
        } 
    }
    in2_info = calloc(need_rect_num, sizeof(struct age_flow_src2_info));
    if (!in2_info) {
        HD_GFX_ERR("%s:call age_flow_src2_info fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }
    out_info = calloc(need_rect_num, sizeof(struct age_flow_dst_info));
    if (!out_info) {
        HD_GFX_ERR("%s:call age_flow_dst_info fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }    

    //for draw rect=disable src1/blend and use src2 pattern func    
    in1_info.en = 0;    
    blend_param.en = 0;
    rect_idx = 0;
    
    for (param_idx = 0; param_idx < num_of_rect; param_idx++) {
         if (rect_idx >= need_rect_num) {
            HD_GFX_ERR("check age_idx=%d >= need_rect_num=%d fail\n", rect_idx, need_rect_num);
            ret = HD_ERR_NG;
            goto err_exit;
         }                             
        if (p_param[param_idx].type == HD_GFX_RECT_HOLLOW) {
            if ((INT32)p_param[param_idx].thickness >= p_param[param_idx].rect.w || (INT32)p_param[param_idx].thickness >= p_param[param_idx].rect.h) {
                HD_GFX_ERR("Check rectangle thickness(%d) > w(%d) or h(%d)\n", p_param[param_idx].thickness,
                           p_param[param_idx].rect.w, p_param[param_idx].rect.h);
                ret = HD_ERR_NG;
                goto err_exit;
            }
            for (i = 0; i < 4; i++) {//age 沒有空心框,要分次畫框
                if (i == 0) {//top line
                    line_rect.x = p_param[param_idx].rect.x;
                    line_rect.y = p_param[param_idx].rect.y;
                    line_rect.w = p_param[param_idx].rect.w;
                    line_rect.h = p_param[param_idx].thickness;
                } else if (i == 1) {//right line
                    line_rect.x = p_param[param_idx].rect.x + p_param[param_idx].rect.w;
                    line_rect.y = p_param[param_idx].rect.y;
                    line_rect.w = p_param[param_idx].thickness;
                    line_rect.h = p_param[param_idx].rect.h;
                } else if (i == 2) {//bottom line
                    line_rect.x = p_param[param_idx].rect.x;
                    line_rect.y = p_param[param_idx].rect.y + p_param[param_idx].rect.h;
                    line_rect.w = p_param[param_idx].rect.w + p_param[param_idx].thickness;
                    line_rect.h = p_param[param_idx].thickness;
                } else {//left line
                    line_rect.x = p_param[param_idx].rect.x;
                    line_rect.y = p_param[param_idx].rect.y;
                    line_rect.h = p_param[param_idx].rect.h;
                    line_rect.w = p_param[param_idx].thickness;
                }
                ret = gfx_fill_src2_info(&in2_info[rect_idx], p_param[param_idx].dst_img, line_rect, p_param[param_idx].color);
                if (ret != HD_OK) {
                    HD_GFX_ERR("gfx_fill_src2_info fail\n");
                    ret = HD_ERR_NG;
                    goto err_exit;
                }
                if(gfx_fill_out_info(&out_info[rect_idx], p_param[param_idx].dst_img, 0) != HD_OK)  {
                    HD_GFX_ERR("gfx_fill_out_info fail\n");
                    ret = HD_ERR_NG;
                    goto err_exit;
                }
                if (get_buf_offset(p_param[param_idx].dst_img, &out_info[rect_idx].addr[0], &out_info[rect_idx].addr[1], &out_info[rect_idx].addr[2], line_rect) < 0) {
                    HD_GFX_ERR("get_buf_offset fail\n");
                    ret = HD_ERR_NG;
                    goto err_exit;
                }      
                rect_idx ++;
            }
        } else {//HD_GFX_RECT_SOLID case
            ret = gfx_fill_src2_info(&in2_info[rect_idx], p_param[param_idx].dst_img, p_param[param_idx].rect, p_param[param_idx].color);
            if (ret != HD_OK) {
                HD_GFX_ERR("gfx_fill_src2_info fail\n");
                ret = HD_ERR_NG;
                goto err_exit;
            }
            if(gfx_fill_out_info(&out_info[rect_idx], p_param[param_idx].dst_img, 0) != HD_OK)  {
                HD_GFX_ERR("gfx_fill_out_info fail\n");
                ret = HD_ERR_NG;
                goto err_exit;
            }
            if (get_buf_offset(p_param[param_idx].dst_img, &out_info[rect_idx].addr[0], &out_info[rect_idx].addr[1],  &out_info[rect_idx].addr[2], p_param[param_idx].rect) < 0) {
                HD_GFX_ERR("get_data_offset_by_fmt fail\n");
                ret = HD_ERR_NG;
                goto err_exit;
            }       
            rect_idx++;
        }
    }
    
    age_info.job_cnt = need_rect_num;
    age_info.p_in1_info = &in1_info;
    age_info.p_in2_info = in2_info;
    age_info.p_blend_param = &blend_param;
    age_info.p_out_info = out_info;    
     if ((ioctl_ret = ioctl(flow_gfx_fd, IOCTL_GFX_AGE_DRAW_RECT, &age_info)) < 0) {
        HD_GFX_ERR("IOCTL_GFX_AGE_DRAW_RECT fail, ioctl_ret(%d)\n", ioctl_ret);
        ret = HD_ERR_DRV;
     }
     
err_exit:
    if (in2_info) {
        free(in2_info);
    }
    if (out_info) {
        free(out_info);
    }
    return ret;
}

HD_RESULT gfx_check_line_param(HD_GFX_DRAW_LINE *p_param)
{
	HD_RESULT ret = HD_OK;

	//age 只能畫直線
	if ((p_param->end.x != p_param->start.x) && (p_param->end.y != p_param->start.y)) {
		HD_GFX_ERR("gfx draw line only support straight line\n");
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->dst_img.dim.w || !p_param->dst_img.dim.h) {
		HD_GFX_ERR("%s:check dim(%d %d) fail\n", __func__, p_param->dst_img.dim.w, p_param->dst_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (gfx_get_age_fmt(p_param->dst_img.format) == AGE_FLOW_FMT_MAX) {
		HD_GFX_ERR("%s:check fmt fail\n", __func__);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->dst_img.p_phy_addr[0] || p_param->dst_img.ddr_id  >= DDR_ID_MAX) {
		HD_GFX_ERR("%s:check pa(%#lx) and ddrid(%d) fail\n", __func__, p_param->dst_img.p_phy_addr[0], p_param->dst_img.ddr_id);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (p_param->end.x  > (p_param->dst_img.dim.w + p_param->start.x) || p_param->end.y  > (p_param->dst_img.dim.h + p_param->start.y)) {
		HD_GFX_ERR("%s:start(%d %d) end(%d %d) > dst_dim(%d %d)\n", __func__, p_param->start.x, p_param->start.y, p_param->end.x,
			   p_param->end.y, p_param->dst_img.dim.w, p_param->dst_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
err_exit:
	return ret;
}

HD_RESULT gfx_draw_lines(HD_GFX_DRAW_LINE *p_param, UINT8 num_of_line)
{
    HD_RESULT ret = HD_OK;
    struct age_flow_info age_info = {0};
    struct age_flow_src1_info in1_info = {0};
    struct age_flow_src2_info *in2_info = NULL;
    struct age_flow_blend_param blend_param = {0};
    struct age_flow_dst_info *out_info = NULL;
    int ioctl_ret = 0;
    HD_IRECT line_rect;
    UINT16 i;

	if (flow_gfx_fd < 0) {
		HD_GFX_ERR("%s:check flow_gfx_fd(%d) fail\n", __func__, flow_gfx_fd);
		ret = HD_ERR_NG;
		goto err_exit;
	}

    if (!p_param || !num_of_line) {
        HD_GFX_ERR("%s:check p_param=%p num_of_line=%d fail\n", __func__, p_param, num_of_line);
        ret = HD_ERR_NULL_PTR;
        goto err_exit;
    }
    for (i = 0; i < num_of_line; i++) {
        if (gfx_check_line_param(&p_param[i]) != HD_OK) {
            HD_GFX_ERR("%s:check p_param fail\n", __func__);
            ret = HD_ERR_NG;
            goto err_exit;
        }
        if (p_param[i].thickness >= p_param[i].dst_img.dim.w || p_param[i].thickness >= p_param[i].dst_img.dim.h) {
            HD_GFX_ERR("Check line thickness(%d) > w_h(%d %d)\n", p_param[i].thickness, p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h);
            ret = HD_ERR_NG;
            goto err_exit;
        }
    }
    
    in2_info = calloc(num_of_line, sizeof(struct age_flow_src2_info));
    if (!in2_info) {
        HD_GFX_ERR("%s:call age_flow_src2_info fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }
    out_info = calloc(num_of_line, sizeof(struct age_flow_dst_info));
    if (!out_info) {
        HD_GFX_ERR("%s:call age_flow_dst_info fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }
    
    in1_info.en = 0;
    blend_param.en = 0;    

    for (i = 0; i < num_of_line; i++) {
        out_info[i].ddr_id = p_param[i].dst_img.ddr_id;
        out_info[i].fmt = gfx_get_age_fmt(p_param[i].dst_img.format);
        if (out_info[i].fmt == AGE_FLOW_FMT_MAX) {
            HD_GFX_ERR("gfx_get_age_fmt fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }
        in2_info[i].en = 1;
        //age 只能畫直線,不能畫斜線
        line_rect.x = p_param[i].start.x;
        line_rect.y = p_param[i].start.y;
        if (p_param[i].end.x == p_param[i].start.x) {//直線
            line_rect.h = p_param[i].end.y - p_param[i].start.y;
            if (!p_param[i].thickness) {
                line_rect.w = 1;
            } else {
                line_rect.w = p_param[i].thickness;
            }
        } else if (p_param[i].end.y == p_param[i].start.y) {//橫線
            line_rect.w = p_param[i].end.x - p_param[i].start.x;
            if (!p_param[i].thickness) {
                line_rect.h = 1;
            } else {
                line_rect.h = p_param[i].thickness;
            }
        } else {
            HD_GFX_ERR("Check start(%d %d) and end(%d %d) point fail\n", p_param[i].start.x, p_param[i].start.y,
                       p_param[i].end.x, p_param[i].end.y);
            ret = HD_ERR_NG;
            goto err_exit;
        }

        ret = gfx_fill_src2_info(&in2_info[i], p_param[i].dst_img, line_rect, p_param[i].color);
        if (ret != HD_OK) {
            HD_GFX_ERR("gfx_fill_src2_info fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }
        if(gfx_fill_out_info(&out_info[i], p_param[i].dst_img, 0) != HD_OK)  {
                HD_GFX_ERR("gfx_fill_out_info fail\n");
                ret = HD_ERR_NG;
                goto err_exit;
         }
        //根據rect 的座標和大小填output buf的addr
        if (get_buf_offset(p_param[i].dst_img, &out_info[i].addr[0], &out_info[i].addr[1], &out_info[i].addr[2], line_rect) < 0) {
            HD_GFX_ERR("get_data_offset_by_fmt fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }
    }
    
    age_info.p_in1_info = &in1_info;
    age_info.p_in2_info = in2_info;
    age_info.p_blend_param = &blend_param;
    age_info.p_out_info = out_info;
    age_info.job_cnt = num_of_line;
    if ((ioctl_ret = ioctl(flow_gfx_fd, IOCTL_GFX_AGE_DRAW_LINE, &age_info)) < 0) {
        HD_GFX_ERR("IOCTL_GFX_AGE_DRAW_LINE fail, ioctl_ret(%d)\n", ioctl_ret);
        ret = HD_ERR_DRV;
    }
    
err_exit:
    if (in2_info) {
        free(in2_info);
    }
    if (out_info) {
        free(out_info);
    }
    return ret;
}

HD_RESULT gfx_check_rotate_param(HD_GFX_ROTATE *p_param)
{
	HD_RESULT ret = HD_OK;

	if (!p_param->dst_img.dim.w || !p_param->dst_img.dim.h) {
		HD_GFX_ERR("%s:check dst dim(%d %d) fail\n", __func__, p_param->dst_img.dim.w, p_param->dst_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (gfx_get_age_fmt(p_param->dst_img.format) == AGE_FLOW_FMT_MAX) {
		HD_GFX_ERR("%s:check dst fmt fail\n", __func__);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->dst_img.p_phy_addr[0] || p_param->dst_img.ddr_id  >= DDR_ID_MAX) {
		HD_GFX_ERR("%s:check dst pa(%#lx) and ddrid(%d) fail\n", __func__, p_param->dst_img.p_phy_addr[0], p_param->dst_img.ddr_id);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}

	if (!p_param->src_img.dim.w || !p_param->src_img.dim.h) {
		HD_GFX_ERR("%s:check src dim(%d %d) fail\n", __func__, p_param->src_img.dim.w, p_param->src_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (gfx_get_age_fmt(p_param->src_img.format) == AGE_FLOW_FMT_MAX) {
		HD_GFX_ERR("%s:check src fmt fail\n", __func__);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (!p_param->src_img.p_phy_addr[0] || p_param->src_img.ddr_id  >= DDR_ID_MAX) {
		HD_GFX_ERR("%s:check src pa(%#lx) and ddrid(%d) fail\n", __func__, p_param->src_img.p_phy_addr[0], p_param->src_img.ddr_id);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if ((UINT32)(p_param->src_region.x + p_param->src_region.w) > p_param->src_img.dim.w ||
	    (UINT32)(p_param->src_region.y + p_param->src_region.h) > p_param->src_img.dim.h) {
		HD_GFX_ERR("%s:check src region(%d %d %d %d) > src dim(%d %d) fail\n", __func__, p_param->src_region.x,
			   p_param->src_region.y, p_param->src_region.w, p_param->src_region.h, p_param->src_img.dim.w, p_param->src_img.dim.h);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
	if (gfx_get_age_roate_angle(p_param->angle) == AGE_FLOW_FLIP_ROTATE_MAX) {
		HD_GFX_ERR("%s:check angle(%#x) fail\n", __func__, p_param->angle);
		ret = HD_ERR_PARAM;
		goto err_exit;
	}
err_exit:
	return ret;
}

HD_RESULT gfx_roate_imgs(HD_GFX_ROTATE *p_param, UINT8 num_of_image)
{
    HD_RESULT ret = HD_OK;
    struct age_flow_info age_info = {0};
    struct age_flow_src1_info *in1_info = NULL;
    struct age_flow_src2_info in2_info = {0};
    struct age_flow_blend_param blend_param = {0};
    struct age_flow_dst_info *out_info = NULL;
    HD_IRECT rotat_dst_rect;
    int ioctl_ret = 0;
    UINT16 i;
    
    if (flow_gfx_fd < 0) {
        HD_GFX_ERR("%s:check flow_gfx_fd(%d) fail\n", __func__, flow_gfx_fd);
        ret = HD_ERR_NG;
        goto err_exit;
    }
    if (!p_param || !num_of_image) {
        HD_GFX_ERR("%s:check p_param=%p num_of_image=%d fail\n", __func__, p_param, num_of_image);
        ret = HD_ERR_NULL_PTR;
        goto err_exit;
    }
    for (i = 0; i < num_of_image; i++) {
        if (gfx_check_rotate_param(&p_param[i]) != HD_OK) {
            HD_GFX_ERR("%s:check p_param fail\n", __func__);
            ret = HD_ERR_NG;
            goto err_exit;
        }
    }
    
    in1_info = calloc(num_of_image, sizeof(struct age_flow_src1_info));
    if (!in1_info) {
        HD_GFX_ERR("%s:call age_flow_src1_info fail\n", __func__);
        ret = HD_ERR_NOMEM;
        goto err_exit;
    }

   out_info = calloc(num_of_image, sizeof(struct age_flow_dst_info));
   if (!out_info) {
       HD_GFX_ERR("%s:call age_flow_dst_info fail\n", __func__);
       ret = HD_ERR_NOMEM;
       goto err_exit;
   }    
    
    blend_param.en = 0;
    in2_info.en = 0;
    for (i =0 ; i < num_of_image; i++) {
        in1_info[i].en = 1;
        //根據rect 的座標和大小填src buf的addr
        if (get_buf_offset(p_param[i].src_img, &in1_info[i].addr[0], &in1_info[i].addr[1], &in1_info[i].addr[2] ,p_param[i].src_region) < 0) {
            HD_GFX_ERR("get_data_offset_by_fmt fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }
        in1_info[i].ddr_id = p_param[i].src_img.ddr_id;
        in1_info[i].fmt = gfx_get_age_fmt(p_param[i].src_img.format);
        if (in1_info[i].fmt == AGE_FLOW_FMT_MAX) {
            HD_GFX_ERR("gfx_get_age_fmt fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }
        in1_info[i].op = AGE_FLOW_OP_FLIP_ROT;
        in1_info[i].type = AGE_FLOW_DATA_TYPE_FULL_RANGE;
        in1_info[i].param.flip_rot.mode = gfx_get_age_roate_angle(p_param[i].angle);
        if (in1_info[i].param.flip_rot.mode == AGE_FLOW_FLIP_ROTATE_MAX) {
            HD_GFX_ERR("gfx_age_roate_angle fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }
        if (get_buf_offset(p_param[i].src_img, &in1_info[i].addr[0], &in1_info[i].addr[1], &in1_info[i].addr[2], p_param[i].src_region) < 0) {
            HD_GFX_ERR("get_buf_offset fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }

        in1_info[i].roi.w = p_param[i].src_region.w;
        in1_info[i].roi.h = p_param[i].src_region.h;
        ret = gfx_get_lofs_by_fmt(p_param[i].src_img.format, p_param[i].src_img.dim.w, &in1_info[i].roi.lofs[0], &in1_info[i].roi.lofs[1]);
        if (ret != HD_OK) {
            HD_GFX_ERR("gfx_get_lofs_by_fmt fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }

        if (p_param[i].src_img.format == HD_VIDEO_PXLFMT_ARGB1555) {
            in1_info[i].alpha_val[0] = 0;
            in1_info[i].alpha_val[1] = 255;
        }
        out_info[i].ddr_id = p_param[i].dst_img.ddr_id;
        //根據rect 的座標和大小填output buf的addr
        rotat_dst_rect.x = p_param[i].dst_pos.x;
        rotat_dst_rect.y = p_param[i].dst_pos.y;
        rotat_dst_rect.w = p_param[i].dst_img.dim.w;
        rotat_dst_rect.h = p_param[i].dst_img.dim.h;
        if (get_buf_offset(p_param[i].dst_img, &out_info[i].addr[0], &out_info[i].addr[1], &out_info[i].addr[2], rotat_dst_rect) < 0) {
            HD_GFX_ERR("get_data_offset_by_fmt fail\n");
            ret = HD_ERR_NG;
            goto err_exit;
        }

        if(gfx_fill_out_info(&out_info[i], p_param[i].dst_img, 0) != HD_OK)  {
              HD_GFX_ERR("gfx_fill_out_info fail\n");
              ret = HD_ERR_NG;
              goto err_exit;
        }
    }
    
    age_info.job_cnt = num_of_image;
    age_info.p_in1_info = in1_info;
    age_info.p_in2_info = &in2_info;
    age_info.p_blend_param = &blend_param;
    age_info.p_out_info = out_info;
    if ((ioctl_ret = ioctl(flow_gfx_fd, IOCTL_GFX_AGE_ROTATE, &age_info)) < 0) {
        HD_GFX_ERR("IOCTL_GFX_AGE_ROTATE fail, ioctl_ret(%d)\n", ioctl_ret);
        ret = HD_ERR_DRV;
    }
    
err_exit:
    if (in1_info) {
        free(in1_info);
    }
    if (out_info) {
        free(out_info);
    }
    return ret;
}

HD_RESULT gfx_get_syscaps(GFX_SYSCAPS *p_param)
{
        if (!gfx_syscap_info.scl_rate) {
            HD_GFX_ERR("gfx not init\n");
            return HD_ERR_UNINIT;
        }
        p_param->scl_rate = gfx_syscap_info.scl_rate;
        p_param->max_dim.w = gfx_syscap_info.max_w;
        p_param->max_dim.h = gfx_syscap_info.max_h;
        p_param->min_dim.w = gfx_syscap_info.min_w;
        p_param->min_dim.h = gfx_syscap_info.min_h;
        p_param->image_align.w = gfx_syscap_info.align_w;
        p_param->image_align.h = gfx_syscap_info.align_h;
        return HD_OK;
}
/****************************************************/

