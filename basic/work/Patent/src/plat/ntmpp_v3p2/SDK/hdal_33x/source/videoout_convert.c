/**
	@brief This sample demonstrates the usage of video process functions.

	@file videoout_convert.c

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
#include "vg_common.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define	MAX_POOL_NUM	1
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern unsigned int gmlib_dbg_level;
extern unsigned int gmlib_dbg_mode;
extern vpd_sys_info_t platform_sys_Info;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT set_vout_unit(HD_GROUP *hd_group, INT line_idx, DIF_VOUT_PARAM *vout_param, INT *len)
{
	INT i, fps, is_virtual_lcd = 0;
	pif_group_t *group;
	vpd_entity_t *disp;
	vpd_disp_entity_t *disp_entity;
	INT lcd_fd, vpe_vch, vpe_path, lcd_vch, lcd_path;
	HD_DIM in_dim = {0, 0};
	HD_DIM out_dim = {0, 0};
	HD_VIDEO_PXLFMT in_pxlfmt = 0;

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
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	if (dif_get_vpe_ch_path(hd_group, line_idx, &vpe_vch, &vpe_path) != HD_OK) {
		GMLIB_ERROR_P("No vproc unit bind for vout\n");
		goto err_exit;
	}

	lcd_vch = vout_param->dev.device_subid;
	lcd_path = vout_param->dev.in_subid;
	lcd_fd = utl_get_lcd_entity_fd(platform_sys_Info.lcd_info[lcd_vch].chipid, lcd_vch, lcd_path, NULL);

	disp = pif_lookup_entity_buf(VPD_DISP_ENTITY_TYPE, lcd_fd, group->encap_buf);
	if (disp != NULL) { /* It have been set in encaps buf, just return this buffer */
		*len = 0;
		goto exit;
	}
	if (platform_sys_Info.lcd_info[lcd_vch].lcdid < 0) {
		dif_get_lcd_output_type(hd_group, line_idx, &is_virtual_lcd);
		if (is_virtual_lcd == 0) {
			GMLIB_ERROR_P("<lcd: vch(%d) id(%d) error!>\n", lcd_vch, platform_sys_Info.lcd_info[lcd_vch].lcdid);
			goto err_exit;
		} else {
			*len = 0;
			goto exit;
		}
	}
	if (dif_get_vproc_out_pxlfmt(hd_group, line_idx, &in_pxlfmt) != HD_OK) {
		GMLIB_ERROR_P("<vproc out pxlfmt is not set! line_idx(%d) >\n", line_idx);
		goto err_exit;
	}

	in_dim.w = platform_sys_Info.lcd_info[lcd_vch].fb0_win.width;
	in_dim.h = platform_sys_Info.lcd_info[lcd_vch].fb0_win.height;
	out_dim.w = platform_sys_Info.lcd_info[lcd_vch].desk_res.width;
	out_dim.h = platform_sys_Info.lcd_info[lcd_vch].desk_res.height;
	fps = pif_cal_disp_tick_fps(lcd_vch);

	*len = pif_set_EntityBuf(VPD_DISP_ENTITY_TYPE, &disp, group->encap_buf);
	if (disp == NULL) {
		GMLIB_ERROR_P("<vout: NULL entity buffer!>\n");
		goto err_exit;
	}
	disp->sn = lcd_fd;
	disp->ap_bindfd = BD_SET_PATH(vout_param->dev.device_baseid + vout_param->dev.device_subid,
				      vout_param->dev.in_subid + HD_IN_BASE,
				      vout_param->dev.out_subid + HD_OUT_BASE);

	memset(disp->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < MAX_POOL_NUM; i++) {
		disp->pool[i].enable = TRUE;
		disp->pool[i].type = VPD_END_POOL;
		snprintf(disp->pool[i].name, GM_POOL_NAME_LEN, "%s", pif_get_pool_name_by_type(disp->pool[i].type));
		disp->pool[i].width = in_dim.w;
		disp->pool[i].height = in_dim.h;
		disp->pool[i].fps = fps;
		disp->pool[i].vch = lcd_vch;
		disp->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		disp->pool[i].is_duplicate = 0;
		disp->pool[i].dst_fmt = VPD_BUFTYPE_UNKNOWN;
		disp->pool[i].ddr_id = 0;
		disp->pool[i].size = 0;
		disp->pool[i].win_size = 0;
		disp->pool[i].count_decuple = 0;
		disp->pool[i].max_counts = 0;
		disp->pool[i].min_counts = 0;

		GMLIB_L2_P("set_vout_pool i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) fmt(%#x) dup(%d)\n",
			   i, disp->pool[i].enable, disp->pool[i].name, disp->pool[i].type, disp->pool[i].ddr_id, disp->pool[i].size,
			   disp->pool[i].count_decuple, disp->pool[i].max_counts, disp->pool[i].dst_fmt, disp->pool[i].is_duplicate);
	}

	disp_entity = (vpd_disp_entity_t *)disp->e;
	disp_entity->lcd_vch = lcd_vch;
	disp_entity->src_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(in_pxlfmt);
	disp_entity->src_bg_dim.w = in_dim.w;
	disp_entity->src_bg_dim.h = in_dim.h;
	disp_entity->src_rect.x = 0;
	disp_entity->src_rect.y = 0;
	disp_entity->src_rect.w = in_dim.w;
	disp_entity->src_rect.h = in_dim.h;
	disp_entity->disp1_rect.x = 0;
	disp_entity->disp1_rect.y = 0;
	disp_entity->disp1_rect.w = out_dim.w;
	disp_entity->disp1_rect.h = out_dim.h;
	disp_entity->src_frame_rate = pif_cal_disp_tick_fps(lcd_vch);
	disp_entity->feature = 0;
	if (pif_is_liveview(group)) {
		disp_entity->feature |= 1;  //feature: [4:0]latency drop enable,  0(disable)  1(enable)
	}

	//move disp_in buf count to vpe out
	if (platform_sys_Info.low_latency_mode) {
		SET_ENTITY_FEATURE(disp->feature, VPD_FLAG_HIGH_PRIORITY);
	} else {
		SET_ENTITY_FEATURE(disp->feature, VPD_FLAG_HIGH_PRIORITY | VPD_FLAG_REFERENCE_CLOCK);
	}

#if 0 //TODO
	//display_to_enc without resize case,osg must wait lcd done
	if (bindfd->pchinfo.disp.graph_mode == GRAPH_DISP_AND_ENCODE) {
		SET_ENTITY_FEATURE(disp->feature, VPD_FLAG_JOB_SEQUENCE_FIRST);
	}
#endif

	GMLIB_L2_P("set_vout_entity path_id(%#lx) fd(%#x) pch(%d) src_bg(%d,%d) src_rect(%d,%d,%d,%d) src_fmt(%#x) fps(%d), dst_rect(%d,%d,%d,%d)\n",
		   dif_get_path_id(&vout_param->dev), lcd_fd, disp_entity->lcd_vch,
		   disp_entity->src_bg_dim.w, disp_entity->src_bg_dim.h,
		   disp_entity->src_rect.x, disp_entity->src_rect.y,
		   disp_entity->src_rect.w, disp_entity->src_rect.h,
		   disp_entity->src_fmt, disp_entity->src_frame_rate,
		   disp_entity->disp1_rect.x, disp_entity->disp1_rect.y,
		   disp_entity->disp1_rect.w, disp_entity->disp1_rect.h);

exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, disp);

err_exit:
	return -1;

}

INT set_osg_unit(HD_GROUP *hd_group, INT line_idx, DIF_VOUT_PARAM *vout_param, INT *len)
{
	INT i, fps, is_virtual_lcd = 0;
	pif_group_t *group;
	vpd_entity_t *osg;
	vpd_osg_entity_t *osg_entity;
	INT osg_fd, vpe_vch, vpe_path, lcd_vch, lcd_path;
	HD_DIM in_dim = {0, 0};
	HD_VIDEO_PXLFMT in_pxlfmt = 0;
	HD_OUT_POOL out_pool;

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
	group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto err_exit;
	}

	if (dif_get_vpe_ch_path(hd_group, line_idx, &vpe_vch, &vpe_path) != HD_OK) {
		GMLIB_ERROR_P("No vproc unit bind for osg\n");
		goto err_exit;
	}

	lcd_vch = vout_param->dev.device_subid;
	lcd_path = vout_param->dev.in_subid;
	osg_fd = utl_get_disp_osg_entity_fd(0, lcd_vch, lcd_path, NULL);

	osg = pif_lookup_entity_buf(VPD_OSG_ENTITY_TYPE, osg_fd, group->encap_buf);
	if (osg != NULL) { /* It have been set in encaps buf, just return this buffer */
		*len = 0;
		goto exit;
	}
	if (platform_sys_Info.lcd_info[lcd_vch].lcdid < 0) {
		dif_get_lcd_output_type(hd_group, line_idx, &is_virtual_lcd);
		if (is_virtual_lcd == 0) {
			GMLIB_ERROR_P("<lcd: vch(%d) id(%d) error!>\n", lcd_vch, platform_sys_Info.lcd_info[lcd_vch].lcdid);
			goto err_exit;
		}
	}
	if (dif_get_vproc_out_pxlfmt(hd_group, line_idx, &in_pxlfmt) != HD_OK) {
		GMLIB_ERROR_P("<vproc out pxlfmt is not set! line_idx(%d) >\n", line_idx);
		goto err_exit;
	}
	if (dif_get_vproc_out_pool(hd_group, line_idx, &out_pool) != HD_OK) {
		GMLIB_ERROR_P("<vproc out pool is not set! line_idx(%d) >\n", line_idx);
		goto err_exit;
	}

	in_dim.w = platform_sys_Info.lcd_info[lcd_vch].fb0_win.width;
	in_dim.h = platform_sys_Info.lcd_info[lcd_vch].fb0_win.height;
	fps = pif_cal_disp_tick_fps(lcd_vch);

	*len = pif_set_EntityBuf(VPD_OSG_ENTITY_TYPE, &osg, group->encap_buf);
	if (osg == NULL) {
		GMLIB_ERROR_P("<vout: NULL entity buffer!>\n");
		goto err_exit;
	}
	osg->sn = osg_fd;
	osg->ap_bindfd = BD_SET_PATH(vout_param->dev.device_baseid + vout_param->dev.device_subid,
				     vout_param->dev.in_subid + HD_IN_BASE,
				     vout_param->dev.out_subid + HD_OUT_BASE);

	memset(osg->pool, 0x0, sizeof(vpd_pool_t) * VPD_MAX_BUFFER_NUM);
	for (i = 0; i < MAX_POOL_NUM; i++) {
		if (out_pool.buf_info[i].enable == HD_BUF_DISABLE) {
			osg->pool[i].enable = FALSE;
			continue;
		} else if (out_pool.buf_info[i].enable == HD_BUF_ENABLE) {
			osg->pool[i].enable = TRUE;
			osg->pool[i].ddr_id = out_pool.buf_info[i].ddr_id;
			osg->pool[i].count_decuple = out_pool.buf_info[i].counts;
			osg->pool[i].max_counts = out_pool.buf_info[i].max_counts / 10;
			osg->pool[i].min_counts = out_pool.buf_info[i].min_counts / 10;
		} else {  //HD_BUF_AUTO
			osg->pool[i].enable = TRUE;
			osg->pool[i].ddr_id = 0;
			osg->pool[i].count_decuple = 30;
			osg->pool[i].max_counts = 3;
			osg->pool[i].min_counts = 0;
		}
		osg->pool[i].type = VPD_DISP0_IN_POOL + lcd_vch;
		snprintf(osg->pool[i].name, GM_POOL_NAME_LEN, "%s", pif_get_pool_name_by_type(osg->pool[i].type));
		osg->pool[i].width = in_dim.w;
		osg->pool[i].height = in_dim.h;
		osg->pool[i].fps = fps;
		osg->pool[i].vch = lcd_vch;
		osg->pool[i].gs_flow_rate = PIF_SET_FPS_RATIO(1, 1);
		osg->pool[i].is_duplicate = 1;
		osg->pool[i].dst_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(in_pxlfmt);
		osg->pool[i].size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(in_dim, in_pxlfmt);;
		osg->pool[i].win_size = 0;

		GMLIB_L2_P("set_osg_pool i(%d) en(%d) name(%s) type(%d) ddr(%d) size(%d) count(%d) max(%d) fmt(%#x) dup(%d)\n",
			   i, osg->pool[i].enable, osg->pool[i].name, osg->pool[i].type, osg->pool[i].ddr_id, osg->pool[i].size,
			   osg->pool[i].count_decuple, osg->pool[i].max_counts, osg->pool[i].dst_fmt, osg->pool[i].is_duplicate);
	}

	osg_entity = (vpd_osg_entity_t *)osg->e;
	osg_entity->src_fmt = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(in_pxlfmt);
	osg_entity->src_bg_dim.w = in_dim.w;
	osg_entity->src_bg_dim.h = in_dim.h;
	osg_entity->attr_vch = 0;
	osg_entity->osg_param_1.direction = 0;

	GMLIB_L2_P("set_osg_entity path_id(%#lx) fd(%#x) pch(%d) src_bg(%d,%d) src_fmt(%#x)\n",
		   dif_get_path_id(&vout_param->dev), osg_fd, osg_entity->attr_vch,
		   osg_entity->src_bg_dim.w, osg_entity->src_bg_dim.h,
		   osg_entity->src_fmt);


exit:
	return VPD_CALC_ENTITY_OFFSET(group->encap_buf, osg);

err_exit:
	return -1;

}

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
