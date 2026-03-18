/**
	@brief Header file of bind of library.\n
	This file contains the bind functions of library.

	@file bind.c

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
#include "hdal.h"
#include "hd_debug.h"
#include "bind.h"
#include "dif.h"
#include "vpd_ioctl.h"
#include "pif.h"
#include "utl.h"
#include "videoenc.h"
#include "videoprocess.h"


/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern unsigned int gmlib_dbg_level;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
extern void gmlib_proc_init(void);
extern void gmlib_proc_close(void);
extern void hdal_setting_log(void);

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

INT32 bd_alloc_nr = 0;
#define BD_MALLOC(a)       ({bd_alloc_nr++; PIF_MALLOC((a));})
#define BD_CALLOC(a, b)    ({bd_alloc_nr++; PIF_CALLOC((a), (b));})
#define BD_FREE(a)         {bd_alloc_nr--; PIF_FREE((a));}
#define BD_GROUP_LIST_NUMBER 10

#define BD_MAX_VAL(a,b)        ((a) > (b) ? (a) : (b))
#define BD_MIN_VAL(a,b)        ((a) < (b) ? (a) : (b))

#define BD_DUMP_ERR_LOG 0	// 1: auto dump log.txt when err happend

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
HD_DEVICE_BASE_INFO all_dbase[HD_MAX_DEVICE_BASEID] = {{0}};
HD_GROUP bgroup[BD_MAX_BIND_GROUP];
HD_LIVEVIEW_INFO lv_collect_info[VPD_MAX_CAPTURE_CHANNEL_NUM][VPD_MAX_CAPTURE_PATH_NUM] = {0};
INT hd_product_type = 0;  /* 0:HD_DVR, 1:HD_NVR */
HD_TRANSCODE_PATH bd_transcode_path;
pthread_mutex_t hdal_mutex;

CHAR *indent_string[HD_MAX_GRAPH_RECURSIVE_LEVEL] = {
	"",
	"  +",
	"    +",
	"      +",
	"        +",
	"          +",
	"            +",
	"              +",
	"                +",
	"                  +"
};
extern vpd_sys_info_t platform_sys_Info;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
CHAR *bd_get_product_type_str(INT product_type)
{
	char *name = NULL;

	switch (product_type) {
	case HD_DVR:
		name = "DVR";
		break;
	case HD_NVR:
		name = "NVR";
		break;
	default:
		name = "Undefined product";
		break;
	}
	return name;
}

HD_DAL bd_get_videoproc_dev_baseid(HD_DAL deviceid)
{
	HD_DAL device_baseid = 0;

	if (((UINT32)(deviceid) & 0x0000FF00) >= HD_DAL_VIDEOPROC_BASE &&
		((UINT32)(deviceid) & 0x0000FF00) <= HD_DAL_VIDEOPROC_RESERVED_BASE) {

		device_baseid = HD_DAL_VIDEOPROC_BASE;
	}
	return device_baseid;
}

HD_DAL bd_get_dev_baseid(HD_DAL deviceid)
{
	HD_DAL device_baseid = 0;

	if ((device_baseid = bd_get_videoproc_dev_baseid(deviceid)) != 0) {
		/* case: it's videoproc, return the device_baseid */
		goto ext;
	}

	switch ((UINT32)(deviceid) & 0x0000FF00) {
	case HD_DAL_VIDEOCAP_BASE:
		device_baseid = HD_DAL_VIDEOCAP_BASE;
		break;
	case HD_DAL_VIDEOOUT_BASE:
		device_baseid = HD_DAL_VIDEOOUT_BASE;
		break;
	case HD_DAL_VIDEOENC_BASE:
		device_baseid = HD_DAL_VIDEOENC_BASE;
		break;
	case HD_DAL_VIDEODEC_BASE:
		device_baseid = HD_DAL_VIDEODEC_BASE;
		break;
	case HD_DAL_AUDIOCAP_BASE:
		device_baseid = HD_DAL_AUDIOCAP_BASE;
		break;
	case HD_DAL_AUDIOOUT_BASE:
		device_baseid = HD_DAL_AUDIOOUT_BASE;
		break;
	case HD_DAL_AUDIOENC_BASE:
		device_baseid = HD_DAL_AUDIOENC_BASE;
		break;
	case HD_DAL_AUDIODEC_BASE:
		device_baseid = HD_DAL_AUDIODEC_BASE;
		break;
	default:
		break;
	}

ext:
	return device_baseid;
}

UINT32 bd_get_dev_subid(HD_DAL deviceid)
{
	return (deviceid - bd_get_dev_baseid(deviceid));
}

UINT32 bd_get_chip_by_device(HD_DAL deviceid)
{
	UINT32 chipid = 0, device_baseid, device_subid;

	device_baseid = bd_get_dev_baseid(deviceid);
	device_subid = bd_get_dev_subid(deviceid);
	switch (device_baseid) {
	case HD_DAL_VIDEOPROC_BASE:
		chipid = device_subid / HD_DAL_VIDEOPROC_CHIP_DEV_COUNT;
		break;
	case HD_DAL_VIDEOCAP_BASE:
		if (device_subid >= VPD_MAX_CAPTURE_CHANNEL_NUM) {
			GMLIB_ERROR_P("ERR: VIDEOCAP divice_subid(%d) fail, deviceid(0x%x)\n", device_subid, deviceid);
			goto ext;
		}
		chipid = platform_sys_Info.cap_info[device_subid].chipid;
		break;
	case HD_DAL_VIDEOOUT_BASE:
		if (device_subid >= VPD_MAX_LCD_NUM) {
			GMLIB_ERROR_P("ERR: VIDEOOUT divice_subid(%d) fail, deviceid(0x%x)\n", device_subid, deviceid);
			goto ext;
		}
		chipid = platform_sys_Info.lcd_info[device_subid].chipid;
		break;
	case HD_DAL_VIDEOENC_BASE:
	case HD_DAL_VIDEODEC_BASE:
	case HD_DAL_AUDIOCAP_BASE:
	case HD_DAL_AUDIOOUT_BASE:
	case HD_DAL_AUDIOENC_BASE:
	case HD_DAL_AUDIODEC_BASE:
		chipid = device_subid;
		break;
	default:
		GMLIB_ERROR_P("internal_err: no chipid, diviceid(%x) !\n", deviceid);
		break;
	}
ext:
	return chipid;
}

UINT32 bd_get_chip_by_bind(HD_BIND_INFO *p_bind)
{
	return bd_get_chip_by_device(p_bind->device.deviceid);
}

HD_DEVICE_BASE_INFO *lookup_device_base(HD_DAL usr_device_baseid)
{
	HD_DEVICE_BASE_INFO *p_dbase = NULL;
	UINT32 idx;

	/*  lookup the device base */
	for (idx = 0; idx < HD_MAX_DEVICE_BASEID; idx ++) {
		if ((all_dbase[idx].in_use == TRUE) && (all_dbase[idx].device_baseid == usr_device_baseid)) {
			p_dbase = &all_dbase[idx];
		}
	}
	return p_dbase;
}

INT32 get_bind_in_nr(HD_BIND_INFO *p_bind)
{
	HD_DEVICE_BASE_INFO *p_dbase = NULL;
	HD_DAL device_baseid;

	device_baseid = bd_get_dev_baseid(p_bind->device.deviceid);
	if (device_baseid == 0) {
		GMLIB_ERROR_P("ERR: deviceid(0x%x)\n", p_bind->device.deviceid);
		goto err_ext;
	}

	p_dbase = lookup_device_base(device_baseid);
	if (p_dbase == NULL) {
		GMLIB_ERROR_P("ERR: get bind_out fail, deviceid(0x%x)\n", p_bind->device.deviceid);
		goto err_ext;
	}
	return p_dbase->ability.max_in;

err_ext:
	return -1;
}

INT32 get_bind_out_nr(HD_BIND_INFO *p_bind)
{
	HD_DEVICE_BASE_INFO *p_dbase = NULL;
	HD_DAL device_baseid;

	device_baseid = bd_get_dev_baseid(p_bind->device.deviceid);
	if (device_baseid == 0) {
		GMLIB_ERROR_P("ERR: deviceid(0x%x)\n", p_bind->device.deviceid);
		goto err_ext;
	}
	p_dbase = lookup_device_base(device_baseid);
	if (p_dbase == NULL) {
		GMLIB_ERROR_P("ERR: get bind_out fail, deviceid(0x%x)\n", p_bind->device.deviceid);
		goto err_ext;
	}
	return p_dbase->ability.max_out;

err_ext:
	return -1;
}

HD_RESULT is_bind_valid(HD_BIND_INFO *p_bind)
{
	HD_DEVICE_BASE_INFO *p_dbase = NULL;
	HD_DAL device_baseid;

	device_baseid = bd_get_dev_baseid(p_bind->device.deviceid);
	if (device_baseid == 0) {
		goto err_ext;
	}
	p_dbase = lookup_device_base(device_baseid);
	if (p_dbase == NULL) {
		goto err_ext;
	}

	return HD_OK;
err_ext:
	return HD_ERR_NG;
}

INT32 is_device_base_exist_p(HD_DAL deviceid)
{
	HD_DAL device_baseid;
	INT32 idx;

	device_baseid = bd_get_dev_baseid(deviceid); // need dbase
	for (idx = 0; idx < HD_MAX_DEVICE_BASEID; idx++) {
		if (all_dbase[idx].in_use == FALSE)
			continue;
		if (all_dbase[idx].device_baseid == device_baseid)
			return idx; //device_base is exist.
	}
	return -1; //device_base is not exist.
}

BOOL is_end_device_p(HD_DAL deviceid)
{
	BOOL ret;

	switch (bd_get_dev_baseid(deviceid)) {
	case HD_DAL_VIDEOOUT_BASE:
	case HD_DAL_AUDIOOUT_BASE:
	case HD_DAL_VIDEOENC_BASE:
	case HD_DAL_AUDIOENC_BASE:
		ret = TRUE;
		break;
	default:
		ret = FALSE;
		break;
	}

	return ret;
}

BOOL is_start_device_p(HD_DAL deviceid)
{
	BOOL ret;

	switch (bd_get_dev_baseid(deviceid)) {
	case HD_DAL_AUDIOCAP_BASE:
	case HD_DAL_VIDEOCAP_BASE:
	case HD_DAL_AUDIODEC_BASE:
	case HD_DAL_VIDEODEC_BASE:
	case HD_DAL_VIDEOPROC_BASE:
		ret = TRUE;
		break;
	default:
		ret = FALSE;
		break;
	}

	return ret;
}

BOOL is_middle_device_p(HD_DAL deviceid)
{
	BOOL ret;

	switch (bd_get_dev_baseid(deviceid)) {
	case HD_DAL_VIDEOPROC_BASE:
		ret = TRUE;
		break;
	default:
		ret = FALSE;
		break;
	}

	return ret;
}

CHAR *get_device_name(HD_DAL deviceid)
{
	CHAR *name = NULL;

	switch (bd_get_dev_baseid(deviceid)) {
	case HD_DAL_VIDEOCAP_BASE:
		name = "VIDEOCAP";
		break;
	case HD_DAL_VIDEOPROC_BASE:
		name = "VIDEOPROC";
		break;
	case HD_DAL_VIDEOOUT_BASE:
		name = "VIDEOOUT";
		break;
	case HD_DAL_VIDEOENC_BASE:
		name = "VIDEOENC";
		break;
	case HD_DAL_VIDEODEC_BASE:
		name = "VIDEODEC";
		break;
	case HD_DAL_AUDIOCAP_BASE:
		name = "AUDIOCAP";
		break;
	case HD_DAL_AUDIOOUT_BASE:
		name = "AUDIOOUT";
		break;
	case HD_DAL_AUDIOENC_BASE:
		name = "AUDIOENC";
		break;
	case HD_DAL_AUDIODEC_BASE:
		name = "AUDIODEC";
		break;
	default:
		GMLIB_ERROR_P("deviceid(%d)\n", deviceid);
		name = "Undefine device";
		break;
	}
	return name;
}

CHAR *get_bind_name(HD_BIND_INFO *p_bind)
{
	return get_device_name(p_bind->device.deviceid);
}

struct list_head *get_next_bind_listhead(HD_BIND_INFO *p_bind, UINT32 out_subid)
{
	struct list_head *listhead = NULL;

	if (out_subid >= (UINT32) get_bind_out_nr(p_bind))
		goto ext;

	if (p_bind->out[out_subid].bnode != NULL)
		listhead = &p_bind->out[out_subid].bnode->dst_listhead;

ext:
	return listhead;
}


struct list_head *get_prev_bind_listhead(HD_BIND_INFO *p_bind, UINT32 in_subid)
{
	struct list_head *listhead = NULL;

	if (in_subid >= (UINT32) get_bind_in_nr(p_bind))
		goto ext;

	if (p_bind->in[in_subid].bnode != NULL)
		listhead = &p_bind->in[in_subid].bnode->src_listhead;

ext:
	return listhead;
}

INT32 get_bind_outsubid(HD_BIND_INFO *p_bind, INT32 in_subid)
{
	INT32 out_nr, in_nr, out_subid = -1, i;
	struct list_head *listhead;

	out_nr = get_bind_out_nr(p_bind);
	in_nr = get_bind_in_nr(p_bind);

	if (in_nr > 1  && out_nr == 1) {
		out_subid = 0;   // N-1 mapping
	} else if (in_nr == 1 && out_nr > 1) {
		/* p_bind: single_in to multi_out */
		for (i = 0; i < out_nr; i++) {
			listhead = get_next_bind_listhead(p_bind, i);
			if (listhead == NULL || list_empty(listhead))
				continue;
			out_subid = i;  // 1-N mapping, lookup first
			break;
		}
	} else {
		out_subid = in_subid;   // N-N mapping
	}
	if (out_subid == -1) {
		GMLIB_L1_P("get_bind_outsubid [warning]: %s no out_subid(%d)!\n", get_bind_name(p_bind), out_subid);
	}
	return out_subid;
}

INT32 get_bind_insubid(HD_BIND_INFO *p_bind, INT32 out_subid)
{
	INT32 out_nr, in_nr, insubid = -1, i;
	struct list_head *listhead;

	in_nr = get_bind_in_nr(p_bind);
	out_nr = get_bind_out_nr(p_bind);
	if (in_nr > 1 && out_nr == 1) {
		/* p_bind: multi_in to single_out */
		for (i = 0; i < in_nr; i++) {
			listhead = get_prev_bind_listhead(p_bind, i);
			if (listhead == NULL || list_empty(listhead))
				continue;

			insubid = i;  // lookup first, N-1 mapping
			break;
		}
	} else if (in_nr == 1 && out_nr > 1) {
		insubid = 0;   	  // 1-N mapping
	} else {
		insubid = out_subid;   // N-N mapping
	}
	if (insubid == -1) {
		GMLIB_L1_P("get_bind_insubid [warning]: %s no in_subid(%d)!\n", get_bind_name(p_bind), insubid);
	}
	return insubid;
}

/*
 * get_next_bind: qurey previous binding info
 * Parameters:
 *    Input:   p_bind  : this binding
 *             out_subid: this binding out subid
 *    Output:  query->in_subid: nex_bind in_subid
 *             query->out_subid : next_bind out_subid (NOTE: first lookup, if 1-N mapping)
 *    Return:  next_bind: next binding, if NULL, the previous binding is not exist.
 */
HD_BIND_INFO *get_next_bind(HD_BIND_INFO *p_bind, UINT32 out_subid, HD_QUERY_INFO *query)
{
	HD_BIND_INFO *p_next_bind = NULL;
	struct list_head *listhead;
	HD_DEV_NODE *dev_node;

	if ((INT32) out_subid == -1) /* no prev bind device */
		goto ext;

	listhead = get_next_bind_listhead(p_bind, out_subid);
	if (listhead == NULL) {
		goto ext;
	}

	list_for_each_entry(dev_node, listhead, list) {
		p_next_bind = (HD_BIND_INFO *)dev_node->p_bind_info;
		if (query) {
			query->in_subid = dev_node->in_subid;
			query->out_subid = get_bind_outsubid(p_next_bind, dev_node->in_subid);
		}
	}

ext:
	return p_next_bind;
}

/*
 * get_prev_bind: qurey previous binding info
 * Parameters:
 *    Input:   p_bind  : this binding
 *             in_subid: this binding in subid
 *    Output:  query->in_subid: prev_bind in_subid
 *             query->out_subid : prev_bind out_subid
 *    Return:  prev_bind: previous binding, if NULL, the previous binding is not exist.
 */
HD_BIND_INFO *get_prev_bind(HD_BIND_INFO *p_bind, UINT32 in_subid, HD_QUERY_INFO *query)
{
	/*
	*                                         src_bind_out(out)
	*                                        /
	*                      bnode.src_listhead
	*
	*                      bnode.dst_listhead
	*                     /                  \
	*  dst_bind_in_other(in)                   dst_bind(in)
	*/
	HD_BIND_INFO *p_prev_bind = NULL;
	struct list_head *listhead;
	HD_DEV_NODE *dev_node;

	if ((INT32) in_subid == -1) /* no prev bind device */
		goto ext;

	listhead = get_prev_bind_listhead(p_bind, in_subid);
	if (listhead == NULL) {
		goto ext;
	}

	list_for_each_entry(dev_node, listhead, list) {
		p_prev_bind = (HD_BIND_INFO *)dev_node->p_bind_info;
		if (query) {
			query->in_subid = get_bind_insubid(p_prev_bind, dev_node->out_subid);
			query->out_subid = dev_node->out_subid;
		}
	}

ext:
	return p_prev_bind;
}

HD_BIND_INFO *get_prev_start_bind(HD_BIND_INFO *p_bind, UINT32 in_subid, HD_QUERY_INFO *query)
{
	/*
	*                                         src_bind_out(out)
	*                                        /
	*                      bnode.src_listhead
	*
	*                      bnode.dst_listhead
	*                     /                  \
	*  dst_bind_in_other(in)                   dst_bind(in)
	*/
	HD_BIND_INFO *p_prev_bind = NULL;
	struct list_head *listhead;
	HD_DEV_NODE *dev_node;
	INT32 in_nr, i;

	if ((INT32) in_subid == -1) /* no prev bind device */
		goto ext;

	listhead = get_prev_bind_listhead(p_bind, in_subid);
	if (listhead == NULL)
		goto ext;

	list_for_each_entry(dev_node, listhead, list) {
		p_prev_bind = (HD_BIND_INFO *)dev_node->p_bind_info;
		if (query) {
			query->out_subid = dev_node->out_subid;
			query->in_subid = -1;
			in_nr = get_bind_in_nr(p_prev_bind);
			for (i = 0; i < in_nr; i++) {
				if (p_prev_bind->in[i].port_state == HD_PORT_START) {
					query->in_subid = i;
					break;
				}
			}
			if ((INT32) query->in_subid == -1) {
				query->in_subid = get_bind_insubid(p_prev_bind, dev_node->out_subid);
			}
		}
	}

ext:
	return p_prev_bind;
}

// NOTE: only for internal use, because of protection
static HD_BIND_INFO *get_graph_start_p(HD_BIND_INFO *p_bind, UINT32 in_subid, HD_QUERY_INFO *query)
{
	HD_BIND_INFO *p_start_bind, *p_prev_bind;

	if (query == NULL) {
		GMLIB_ERROR_P("internal_err: query == NULL,  %s_%d_IN_%d\n", get_bind_name(p_bind),
			      bd_get_dev_subid(p_bind->device.deviceid), in_subid);
		return NULL;
	}

	p_start_bind = p_prev_bind = p_bind;
	query->in_subid = in_subid;
	while ((p_prev_bind = get_prev_bind(p_prev_bind, query->in_subid, query)) != NULL) {
		p_start_bind = p_prev_bind; // the last one is start_bind
	}

	/* check the p_start_bind is start_device */
	if (is_start_device_p(p_start_bind->device.deviceid) == TRUE) {
		return p_start_bind; // lookup ok
	} else {
		return NULL;  // lookup fail
	}
}

HD_BIND_INFO *get_bind(HD_DAL deviceid)
{
	HD_BIND_INFO *p_bind = NULL;
	UINT32 device_baseid, device_subid;
	UINT32 idx;

	device_baseid = bd_get_dev_baseid(deviceid);
	device_subid = bd_get_dev_subid(deviceid);

	for (idx = 0; idx < HD_MAX_DEVICE_BASEID; idx ++) {
		if ((all_dbase[idx].in_use == TRUE) && (all_dbase[idx].device_baseid == device_baseid)) {
			if (device_subid >= all_dbase[idx].ability.max_device_nr)	{
				goto ext;
			}

			p_bind = &all_dbase[idx].p_bind[device_subid];
			return p_bind;  // lookup successs
		}
	}
ext:
	return NULL;  // lookup fail
}

/*
 *  inout = 0; subid assign out_subid of device, query next_bind
 *  inout = 1; subid assign in_subid of device, query prev_bind
 */
HD_RESULT is_bind(HD_DAL deviceid, UINT32 subid, UINT32 inout)
{
	HD_RESULT ret = HD_ERR_NG;
	HD_BIND_INFO *p_bind = get_bind(deviceid);
	HD_BIND_INFO *p_tmp_bind;

	pthread_mutex_lock(&hdal_mutex);
	if (p_bind == NULL) {
		goto ext;
	}

	if (inout == 0) {  // lookup next_bind
		p_tmp_bind = get_next_bind(p_bind, subid, NULL);
		if (p_tmp_bind != NULL) {  // lookup success
			ret =  HD_OK;
			goto ext;
		}
	} else { // lookup prev_bind
		p_tmp_bind = get_prev_bind(p_bind, subid, NULL);
		if (p_tmp_bind != NULL) {  // lookup success
			ret =  HD_OK;
			goto ext;
		}
	}

ext:
	pthread_mutex_unlock(&hdal_mutex);
	return ret;
}

/* check disp_to_enc case */
BOOL is_prev_exist_vout(HD_BIND_INFO *p_end_bind, UINT32 in_subid)
{
	BOOL ret = FALSE;
	HD_BIND_INFO *p_tmp_bind;
	HD_QUERY_INFO query = {0};

	p_tmp_bind = p_end_bind;
	query.in_subid = in_subid;
	while ((p_tmp_bind = get_prev_bind(p_tmp_bind, query.in_subid, &query)) != NULL) {
		if (bd_get_dev_baseid(p_tmp_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			ret = TRUE;
			goto ext;
		}
	}
ext:
	return ret;
}


HD_GROUP *lookup_group(HD_BIND_INFO *p_end_bind, UINT32 in_subid)
{
	UINT32 i, j;
	INT idx;
	HD_GROUP *hd_group = NULL;

	/* 1. lookup transcode of mix_group, because the venc of transcode is different to cap_enc */
	for (idx = 0; idx <= bd_transcode_path.max_in_use_idx; idx++) {
		if (bd_transcode_path.end[idx].p_bind == NULL)
			continue;
		if ((bd_transcode_path.end[idx].p_bind == p_end_bind) &&
		    (bd_transcode_path.end[idx].in_subid == in_subid)) {

			for (i = 0; i < BD_MAX_BIND_GROUP; i++) {
				if (bgroup[i].in_use == TRUE && bgroup[i].is_mix_group == TRUE) {
					hd_group = &bgroup[i];
					goto ext;  // lookup mix_group success
				}
			}
			if (hd_group == NULL) {
				GMLIB_ERROR_P("hd_group lookup in mix_group fail\n");
				goto ext;
			}
		}
	}

	/* 2. lookup general group */
	for (i = 0; i < BD_MAX_BIND_GROUP; i++) {
		if (bgroup[i].in_use != TRUE)
			continue;
		for (j = 0; j < HD_MAX_GROUP_END_DEVICE; j++) {
			if (bgroup[i].p_end_bind[j] == p_end_bind) {
				hd_group = &bgroup[i];   // lookup success
				break;
			}
		}
	}

ext:
	return hd_group;
}

/* this func search next bind port state, return the end_bind and query.in_subid if find 1rt link all start */
void get_next_all_start_endbind(struct list_head *listhead, INT layer, HD_BIND_INFO **p_end_bind, HD_QUERY_INFO *query)
{
	struct list_head *dst_listhead;
	HD_DEV_NODE *dev_node;
	HD_BIND_INFO *p_next_bind = NULL;
	INT32 out_nr, i;

	if (layer >= HD_MAX_GRAPH_RECURSIVE_LEVEL) {
		GMLIB_ERROR_P("%s: layer(%d) overflow\n", __func__, layer);
		goto ext;
	}
	list_for_each_entry(dev_node, listhead, list) {
		p_next_bind = (HD_BIND_INFO *)dev_node->p_bind_info;

		if (is_end_device_p(p_next_bind->device.deviceid) == TRUE) {
			GMLIB_L1_P("  %s %s_%d IN(%d) st(%d)\n", indent_string[layer], get_bind_name(p_next_bind),
				   bd_get_dev_subid(p_next_bind->device.deviceid),
				   (int) dev_node->in_subid,
				   (int) p_next_bind->in[dev_node->in_subid].port_state);

			if (p_next_bind->in[dev_node->in_subid].port_state == HD_PORT_START) {
				query->in_subid = dev_node->in_subid;
				*p_end_bind = p_next_bind;
			}
			continue;
		} else {
			out_nr = get_bind_out_nr(p_next_bind);
			for (i = 0; i < out_nr; i++) {
				dst_listhead = &p_next_bind->out[i].bnode->dst_listhead;
				if (list_empty(dst_listhead))
					continue;

				GMLIB_L1_P("  %s %s_%d port(%d %d) st(%d %d)\n", indent_string[layer], get_bind_name(p_next_bind),
					   bd_get_dev_subid(p_next_bind->device.deviceid),
					   (int) dev_node->in_subid,
					   (int) i,
					   (int) p_next_bind->in[dev_node->in_subid].port_state,
					   (int) p_next_bind->out[i].port_state);
				if ((p_next_bind->in[dev_node->in_subid].port_state == HD_PORT_STOP) ||
				    ((int) p_next_bind->out[i].port_state == HD_PORT_STOP)) {
					continue;
				}
				get_next_all_start_endbind(dst_listhead, layer + 1, p_end_bind, query);
			}
		}
	}

ext:
	return;
}

/* check this path's bindings are already stop, if TRUE return NULL , else return the end_bind */
HD_BIND_INFO *is_link_already_stop(HD_PATH_ID path_id, HD_QUERY_INFO *usr_query)
{
	UINT32 this_devid;
	HD_IO in_id, out_id, in_subid, out_subid;
	HD_BIND_INFO *p_bind, *p_tmp_bind, *p_check_bind, *p_end_bind = NULL;
	struct list_head *listhead;
	HD_QUERY_INFO query = {0};
	CHAR dev_string[64];

	this_devid = HD_GET_DEV(path_id);
	in_id = HD_GET_IN(path_id);
	in_subid = BD_GET_IN_SUBID(in_id);
	out_id = HD_GET_OUT(path_id);
	out_subid = BD_GET_OUT_SUBID(out_id);

	if ((p_bind = get_bind(this_devid)) == NULL) {
		dif_get_dev_string(path_id, dev_string, sizeof(dev_string));
		GMLIB_ERROR_P("%s path_id(%#x) err\n", dev_string, path_id);
		goto ext; /* illegal, return NULL */
	}

	query.in_subid = in_subid;
	query.out_subid = out_subid;
	GMLIB_L1_P("%s ==>\n", __func__);

	p_check_bind = p_tmp_bind = p_bind;
	GMLIB_L1_P("  curr: %s_%d port(%d %d) st(%d %d)\n", get_bind_name(p_tmp_bind),
		   bd_get_dev_subid(p_tmp_bind->device.deviceid),
		   (int) query.in_subid,
		   (int) query.out_subid,
		   (int) p_tmp_bind->in[query.in_subid].port_state,
		   (int) p_tmp_bind->out[query.out_subid].port_state);
	if ((p_tmp_bind->in[query.in_subid].port_state == HD_PORT_STOP) ||
	    (p_tmp_bind->out[query.out_subid].port_state == HD_PORT_STOP)) {
		goto ext; /* exist port_state is STOP, and return NULL */
	}

	/* 1. check prev_device to this_device are already stop */
	while ((p_tmp_bind = get_prev_bind(p_tmp_bind, query.in_subid, &query)) != NULL) {
		GMLIB_L1_P("  prev: %s_%d port(%d %d) st(%d %d)\n", get_bind_name(p_tmp_bind),
			   bd_get_dev_subid(p_tmp_bind->device.deviceid),
			   (int) query.in_subid,
			   (int) query.out_subid,
			   (query.in_subid == (UINT32) - 1) ? 0 : (INT) p_tmp_bind->in[query.in_subid].port_state,
			   (query.out_subid == (UINT32) - 1) ? 0 : (INT) p_tmp_bind->in[query.out_subid].port_state);
		if (query.in_subid == (UINT32) - 1 || query.out_subid == (UINT32) - 1) {
			goto ext; /* exist port_state is not START or no binding, and return NULL */
		}
		if (is_end_device_p(p_tmp_bind->device.deviceid) == TRUE) {
			if (p_tmp_bind->out[query.out_subid].port_state == HD_PORT_STOP) {
				goto ext; /* this is disp_enc case : vcap->vpe->vout-vpe->venc */
			}

			/* get the end, this is disp_enc case : vcap->vpe->vout-vpe->venc */
			p_end_bind = p_tmp_bind;  // p_end_bind = vout
			usr_query->in_subid = query.in_subid;
			goto ext;
		}

		if ((p_tmp_bind->in[query.in_subid].port_state == HD_PORT_STOP) ||
		    (p_tmp_bind->out[query.out_subid].port_state == HD_PORT_STOP)) {
			goto ext; /* exist port_state is not START, and return NULL */
		}
		p_check_bind = p_tmp_bind;
	}

	/* 1.1 check the p_check_bind is start_device */
	if (is_start_device_p(p_check_bind->device.deviceid) != TRUE) {
		GMLIB_L1_P("  not: %s_%d port(%d %d) st(%d %d)\n", get_bind_name(p_check_bind),
			   bd_get_dev_subid(p_check_bind->device.deviceid),
			   (int) query.in_subid,
			   (int) query.out_subid,
			   (int) p_check_bind->in[query.in_subid].port_state,
			   (int) p_check_bind->out[query.out_subid].port_state);

		goto ext; /* it's not start_device, and return NULL */
	}

	/* 2. check p_bind is end_device */
	if (is_end_device_p(p_bind->device.deviceid) == TRUE) {
		//all START
		/* 2.1 new start_device to end_device(p_bind) are all start */
		p_end_bind = p_bind;
		usr_query->in_subid = in_subid;
		goto ext; /* it's end_device */
	}

	if ((listhead = get_next_bind_listhead(p_bind, out_subid)) == NULL) {
		goto ext;
	}
	get_next_all_start_endbind(listhead, 1, &p_end_bind, usr_query);

ext:
	if (p_end_bind == NULL) {
		GMLIB_L1_P("  already STOP:\n");
	} else {
		GMLIB_L1_P("  NO STOP: %s_%d_IN_%d\n", get_bind_name(p_end_bind),
			   bd_get_dev_subid(p_end_bind->device.deviceid), usr_query->in_subid);
	}
	return p_end_bind;
}

/* if there is vout_bind exist, return vout_bind and in_subid
 * NOTE: the function need to set p_bind->query.in_subid
 */
HD_BIND_INFO *is_vout_exist(HD_BIND_INFO *p_bind, UINT32 in_subid, HD_QUERY_INFO *query)
{
	HD_BIND_INFO *p_vout_bind = NULL, *p_tmp_bind;

	if (p_bind == NULL)
		goto ext;

	p_tmp_bind = p_bind;
	query->in_subid = in_subid;
	do {
		if (bd_get_dev_baseid(p_tmp_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			p_vout_bind = p_tmp_bind;
			goto ext;
		}
	} while ((p_tmp_bind = get_prev_bind(p_tmp_bind, query->in_subid, query)) != NULL);

ext:
	return p_vout_bind;
}

HD_RESULT set_multi_transcode(HD_MULTI_PATH *p_multi_path)
{
	INT i, path_idx, is_set = 0;
	HD_MEMBER *end_venc;

	/* only store the videoenc for bd_transcode_path table */
	for (path_idx = 0; path_idx < HD_MAX_MULTI_PATH; path_idx++) {
		end_venc = &p_multi_path->end[path_idx];

		if (end_venc->p_bind == NULL || bd_get_dev_baseid(end_venc->p_bind->device.deviceid) != HD_DAL_VIDEOENC_BASE)
			continue;

		is_set = 0;
		/* 1. look up already setting for videoenc of transcode */
		for (i = 0; i <= bd_transcode_path.max_in_use_idx; i++) {
			if (bd_transcode_path.end[i].p_bind == NULL)
				continue;

			if (bd_transcode_path.end[i].p_bind->device.deviceid == end_venc->p_bind->device.deviceid &&
			    bd_transcode_path.end[i].in_subid == end_venc->in_subid &&
			    bd_transcode_path.end[i].out_subid == end_venc->out_subid) {

				is_set = 1;
				break;
			}
		}

		/* 2. if lookup fail, set new videoenc of transcode */
		if (is_set == 0) {
			for (i = 0; i < HD_MAX_TRANSCODE_PATH_NUMBER; i++) {
				if (bd_transcode_path.end[i].p_bind == NULL) {
					bd_transcode_path.end[i].p_bind = end_venc->p_bind;
					bd_transcode_path.end[i].in_subid = end_venc->in_subid;
					bd_transcode_path.end[i].out_subid = end_venc->out_subid;
					bd_transcode_path.max_in_use_idx = BD_MAX_VAL(bd_transcode_path.max_in_use_idx, i);
					is_set = 1;
					break;
				}
			}
		}

		if (is_set == 0)
			goto err_ext;  // err, bd_transcode_path is not enough
	}
	GMLIB_L1_P("  %s: \n", __func__);
	for (i = 0; i <= bd_transcode_path.max_in_use_idx; i++) {
		if (bd_transcode_path.end[i].p_bind != NULL)
			GMLIB_L1_P("    %s: (%d) diviceid(%#x) (%d %d)\n", __FUNCTION__, i,
				   bd_transcode_path.end[i].p_bind->device.deviceid,
				   bd_transcode_path.end[i].in_subid, bd_transcode_path.end[i].out_subid);
	}

	return HD_OK;

err_ext:
	GMLIB_ERROR_P("set transcode_path info fail, bd_transcode_path[%d] is not enough.\n", HD_MAX_TRANSCODE_PATH_NUMBER);
	return HD_ERR_NG;
}

VOID clear_useless_multi_transcode(VOID)
{
	INT i;
	HD_BIND_INFO *p_end_bind, *p_prev_bind;
	HD_QUERY_INFO query = {0};

	/* look up already setting */
	for (i = 0; i <= bd_transcode_path.max_in_use_idx; i++) {
		if ((p_end_bind = bd_transcode_path.end[i].p_bind) == NULL)
			continue;

		query.in_subid = bd_transcode_path.end[i].in_subid;
		if (p_end_bind->in[query.in_subid].port_state != HD_PORT_START) {
			/* this transcode is stop, we need to clear it */
			bd_transcode_path.end[i].p_bind = NULL;
			continue;
		}

		p_prev_bind = p_end_bind;
		while ((p_prev_bind = get_prev_bind(p_prev_bind, query.in_subid, &query)) != NULL) {
			if ((p_prev_bind->in[query.in_subid].port_state != HD_PORT_START) ||
			    (p_prev_bind->out[query.out_subid].port_state != HD_PORT_START)) {

				/* this transcode is stop, we need to clear it */
				bd_transcode_path.end[i].p_bind = NULL;
				break;
			}
		}
	}
	return;
}

void next_lh_bind_for_multidec(struct list_head *listhead, INT layer, HD_MULTI_PATH *multi_path)
{
	struct list_head *dst_listhead;
	HD_DEV_NODE *dev_node;
	HD_BIND_INFO *p_next_bind = NULL;
	INT32 out_nr, i;

	if (layer >= HD_MAX_GRAPH_RECURSIVE_LEVEL) {
		GMLIB_ERROR_P("%s: layer(%d) overflow\n", __func__, layer);
		goto ext;
	}
	list_for_each_entry(dev_node, listhead, list) {
		p_next_bind = (HD_BIND_INFO *)dev_node->p_bind_info;

		if (bd_get_dev_baseid(p_next_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			for (i = 0; i < HD_MAX_MULTI_PATH; i++) {
				if (multi_path->end[i].p_bind == NULL) {
					multi_path->end[i].p_bind = p_next_bind;
					multi_path->end[i].in_subid = dev_node->in_subid;
					multi_path->end[i].out_subid = 0;
					break;
				}
			}
		}

		if (bd_get_dev_baseid(p_next_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
			for (i = 0; i < HD_MAX_MULTI_PATH; i++) {
				if (multi_path->end[i].p_bind == NULL) {
					multi_path->end[i].p_bind = p_next_bind;
					multi_path->end[i].out_subid = multi_path->end[i].in_subid = dev_node->in_subid;
					break;
				}
			}
		}

		if (is_end_device_p(p_next_bind->device.deviceid) == TRUE) {
			//GMLIB_ERROR_P("%s %s_%d_CHIP_%d_IN_%d_layer(%d)\n", indent_string[layer],
			//		get_bind_name(p_next_bind), bd_get_dev_subid(p_next_bind->device.deviceid),
			//		bd_get_chip_by_bind(p_next_bind), dev_node->in_subid,
			//		layer);
			continue;
		} else {
			out_nr = get_bind_out_nr(p_next_bind);
			for (i = 0; i < out_nr; i++) {
				dst_listhead = &p_next_bind->out[i].bnode->dst_listhead;
				if (list_empty(dst_listhead))
					continue;
				//GMLIB_ERROR_P("%s %s_%d_CHIP_%d_IN_%d_OUT_%d_layer(%d)\n", indent_string[layer],
				//		get_bind_name(p_next_bind), bd_get_dev_subid(p_next_bind->device.deviceid),
				//		bd_get_chip_by_bind(p_next_bind), dev_node->in_subid,
				//		i, layer);
				next_lh_bind_for_multidec(dst_listhead, layer + 1, multi_path);
			}
		}
	}

ext:
	return;
}

void next_lh_bind_for_multi(struct list_head *listhead, INT layer, HD_MULTI_PATH *multi_path)
{
	struct list_head *dst_listhead;
	HD_DEV_NODE *dev_node;
	HD_BIND_INFO *p_next_bind = NULL;
	INT32 out_nr, i;

	if (layer >= HD_MAX_GRAPH_RECURSIVE_LEVEL) {
		GMLIB_ERROR_P("%s: layer(%d) overflow\n", __func__, layer);
		goto ext;
	}
	list_for_each_entry(dev_node, listhead, list) {
		p_next_bind = (HD_BIND_INFO *)dev_node->p_bind_info;

		if (bd_get_dev_baseid(p_next_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE &&
		    p_next_bind->in[dev_node->in_subid].port_state == HD_PORT_START) {
			for (i = 0; i < HD_MAX_MULTI_PATH; i++) {
				if (multi_path->end[i].p_bind == NULL) {
					multi_path->end[i].p_bind = p_next_bind;
					multi_path->end[i].in_subid = dev_node->in_subid;
					multi_path->end[i].out_subid = 0;
					break;
				}
			}
		}

		if (bd_get_dev_baseid(p_next_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE &&
		    p_next_bind->in[dev_node->in_subid].port_state == HD_PORT_START) {
			for (i = 0; i < HD_MAX_MULTI_PATH; i++) {
				if (multi_path->end[i].p_bind == NULL) {
					multi_path->end[i].p_bind = p_next_bind;
					multi_path->end[i].out_subid = multi_path->end[i].in_subid = dev_node->in_subid;
					break;
				}
			}
		}

		if (is_end_device_p(p_next_bind->device.deviceid) == TRUE) {
			//GMLIB_ERROR_P("%s %s_%d_CHIP_%d_IN_%d_layer(%d)\n", indent_string[layer],
			//		get_bind_name(p_next_bind), bd_get_dev_subid(p_next_bind->device.deviceid),
			//		bd_get_chip_by_bind(p_next_bind), dev_node->in_subid,
			//		layer);
			continue;
		} else {
			out_nr = get_bind_out_nr(p_next_bind);
			for (i = 0; i < out_nr; i++) {
				dst_listhead = &p_next_bind->out[i].bnode->dst_listhead;
				if (list_empty(dst_listhead))
					continue;

				if (p_next_bind->out[i].port_state != HD_PORT_START)
					continue;
				//GMLIB_ERROR_P("%s %s_%d_CHIP_%d_IN_%d_OUT_%d_layer(%d)\n", indent_string[layer],
				//		get_bind_name(p_next_bind), bd_get_dev_subid(p_next_bind->device.deviceid),
				//		bd_get_chip_by_bind(p_next_bind), dev_node->in_subid,
				//		i, layer);
				next_lh_bind_for_multi(dst_listhead, layer + 1, multi_path);
			}
		}
	}

ext:
	return;
}

void next_lh_bind_for_link_start(struct list_head *listhead, INT layer, HD_MEMBER end_bind_array[])
{
	struct list_head *dst_listhead;
	HD_DEV_NODE *dev_node;
	HD_BIND_INFO *p_next_bind = NULL;
	INT32 out_nr, i;

	if (layer >= HD_MAX_GRAPH_RECURSIVE_LEVEL) {
		GMLIB_ERROR_P("%s: layer(%d) overflow\n", __func__, layer);
		goto ext;
	}
	list_for_each_entry(dev_node, listhead, list) {
		p_next_bind = (HD_BIND_INFO *)dev_node->p_bind_info;

		if (is_end_device_p(p_next_bind->device.deviceid) == TRUE &&
		    p_next_bind->in[dev_node->in_subid].port_state == HD_PORT_START) {

			for (i = 0; i < HD_MAX_MULTI_PATH; i++) {
				if (end_bind_array[i].p_bind == NULL) {
					end_bind_array[i].p_bind = p_next_bind;
					end_bind_array[i].in_subid = dev_node->in_subid;
					//GMLIB_ERROR_P("%s %s_%d_CHIP_%d_IN_%d_layer(%d) %d\n", indent_string[layer],
					//		get_bind_name(p_next_bind), bd_get_dev_subid(p_next_bind->device.deviceid),
					//		bd_get_chip_by_bind(p_next_bind), dev_node->in_subid,
					//		layer, __LINE__);

					break;
				}
			}
		}

		out_nr = get_bind_out_nr(p_next_bind);
		for (i = 0; i < out_nr; i++) {

			dst_listhead = &p_next_bind->out[i].bnode->dst_listhead;
			if (list_empty(dst_listhead)) {
				continue;
			}
			if (p_next_bind->out[i].port_state != HD_PORT_START)
				continue;
			//GMLIB_ERROR_P("%s %s_%d_CHIP_%d_IN_%d_OUT_%d_layer(%d) %d\n", indent_string[layer],
			//		get_bind_name(p_next_bind), bd_get_dev_subid(p_next_bind->device.deviceid),
			//		bd_get_chip_by_bind(p_next_bind), dev_node->in_subid,
			//		i, layer, __LINE__);
			next_lh_bind_for_link_start(dst_listhead, layer + 1, end_bind_array);
		}
	}

ext:
	return;
}

BOOL check_supported_multi_path(HD_BIND_INFO *p_end_bind, UINT32 in_subid, HD_MULTI_PATH *multi_path)
{
	INT i, end_device_number = 0, is_venc = 0, is_vout[HD_MAX_GROUP_END_DEVICE], vout_total = 0;
	HD_BIND_INFO *p_start_bind;
	struct list_head *listhead;
	UINT32 device_subid;
	HD_QUERY_INFO start_bind_query = {0};

	/* in_subid need to set correctly!! */
	if ((p_start_bind = get_graph_start_p(p_end_bind, in_subid, &start_bind_query)) == NULL) {
		GMLIB_ERROR_P("internal_err: no start bind! %s_%d_%d\n", get_bind_name(p_end_bind),
			      bd_get_dev_subid(p_end_bind->device.deviceid), in_subid);
		goto err_ext;  // error occur
	}

	/* collect the end_bind of pb+lv+transcode */
	/* p_start_bind->query.out_subid come from get_graph_start_p function */
	if ((listhead = get_next_bind_listhead(p_start_bind, start_bind_query.out_subid)) == NULL) {
		GMLIB_ERROR_P("internal_err: no next bind!\n");
		goto err_ext;
	}
	multi_path->front.p_bind = p_start_bind;  // it's videodec
	multi_path->front.out_subid = start_bind_query.out_subid;
	//GMLIB_L1_P("%s%s_%d_%d\n",indent_string[0], get_bind_name(p_start_bind),
	//					bd_get_dev_subid(p_start_bind->device.deviceid), p_start_bind->query.out_subid);

	next_lh_bind_for_multi(listhead, 1, multi_path);
	GMLIB_L1_P("%s front: %s_%d_%d\n", __func__, get_bind_name(p_start_bind),
		   bd_get_dev_subid(p_start_bind->device.deviceid), start_bind_query.out_subid);

	/* check end device */
	memset(is_vout, 0, sizeof(is_vout));
	for (i = 0; i < HD_MAX_MULTI_PATH; i++) {
		if (multi_path->end[i].p_bind == NULL)
			break;

		end_device_number++;
		switch (bd_get_dev_baseid(multi_path->end[i].p_bind->device.deviceid)) {
		case HD_DAL_VIDEOENC_BASE:
			is_venc = 1;
			break;
		case HD_DAL_VIDEOOUT_BASE:
			device_subid = bd_get_dev_subid(multi_path->end[i].p_bind->device.deviceid);
			if (device_subid >= HD_MAX_GROUP_END_DEVICE) {
				GMLIB_ERROR_P("internal_err: %s subid(%x)\n", get_bind_name(multi_path->end[i].p_bind), device_subid);
				break;
			}
			is_vout[device_subid] = 1;
			break;
		default:
			GMLIB_ERROR_P("internal_err: wrong device id (%x)\n", p_start_bind->device.deviceid);
			break;
		}

		GMLIB_L1_P("%s end(%d):%s_%d_CHIP_%d_IN_%d\n", __func__, i, get_bind_name(multi_path->end[i].p_bind),
			   bd_get_dev_subid(multi_path->end[i].p_bind->device.deviceid),
			   bd_get_chip_by_bind(multi_path->end[i].p_bind), multi_path->end[i].in_subid);
	}
	for (i = 0; i < HD_MAX_GROUP_END_DEVICE; i++) {
		if (is_vout[i] == 1) {
			vout_total++;
		}
	}

	if (end_device_number >= 2) { //is multi path
		/* do err checking */
		if (bd_get_dev_baseid(p_start_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE ||
		    bd_get_dev_baseid(p_start_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) {
			if (vout_total >= 2)
				goto err_ext;
			if (vout_total >= 1 && is_venc == 1) {
				if (is_vout[0] == 1) {
					/*
					 *   is_vout[0] == 1, means videoout_0 is set, venc multi with videoout_0, not support.
					 */
					GMLIB_ERROR_P("multi_path: Not support for videoout_0 and videoenc\n");
					goto err_ext;
				}
			}
		} else {
			// do nothing
		}
	}
	return TRUE;

err_ext:
	return FALSE;
}

BOOL is_mix_multi_path(HD_BIND_INFO *p_end_bind, UINT32 in_subid)
{
	INT is_vout = 0;
	HD_BIND_INFO *p_start_bind;
	struct list_head *listhead;
	HD_MULTI_PATH multi_path;
	HD_QUERY_INFO query = {0};

	memset(&multi_path, 0, sizeof(multi_path));

	/* p_end_bind->query.in_subid need to set correctly!! */
	if (is_vout_exist(p_end_bind, in_subid, &query) != NULL) {
		/* videoout exist, set the flag, return TRUE. let all videoout in ONE graph */
		is_vout = 1;
	}

	if ((p_start_bind = get_graph_start_p(p_end_bind, in_subid, &query)) == NULL) {
		GMLIB_ERROR_P("internal_err: no start bind! %s_%d_%d\n", get_bind_name(p_end_bind),
			      bd_get_dev_subid(p_end_bind->device.deviceid), query.in_subid);
		goto err_ext;  // error occur
	}
	if (bd_get_dev_baseid(p_start_bind->device.deviceid) != HD_DAL_VIDEODEC_BASE) {
		goto ext; // it's not transcode
	}

	/* collect the transcode info include end_bind of pb+lv+transcode */
	/* query.out_subid come from get_graph_start_p function */
	if ((listhead = get_next_bind_listhead(p_start_bind, query.out_subid)) == NULL) {
		GMLIB_ERROR_P("internal_err: no next bind!\n");
		goto ext;
	}
	multi_path.front.p_bind = p_start_bind;  // it's videodec
	multi_path.front.in_subid = multi_path.front.out_subid = query.out_subid; // videodec, in/out same
	//GMLIB_ERROR_P("%s%s_%d_%d\n",indent_string[0], get_bind_name(p_start_bind),
	//					bd_get_dev_subid(p_start_bind->device.deviceid), p_start_bind->query.out_subid);

	next_lh_bind_for_multidec(listhead, 1, &multi_path);

	/* store the videoenc in bd_transcode_path table, have to check it when apply */
	if (set_multi_transcode(&multi_path) != HD_OK) {
		goto err_ext;
	}

	return TRUE;

ext:
	if (is_vout == 1)
		return TRUE;
	else
		return FALSE;

err_ext:
	return FALSE;
}

/* check this path's all bindings are start, return the end_bind_array */
VOID is_link_all_start(HD_PATH_ID path_id, HD_QUERY_INFO *usr_query, HD_MEMBER end_bind_array[])
{
	UINT32 this_devid;
	HD_IO in_id, out_id, in_subid, out_subid;
	HD_BIND_INFO *p_bind, *p_tmp_bind, *p_check_bind = NULL;
	struct list_head *listhead;
	INT i, is_end_exist = 0;
	HD_QUERY_INFO query = {0};
	CHAR dev_string[64];

	this_devid = HD_GET_DEV(path_id);
	in_id = HD_GET_IN(path_id);
	in_subid = BD_GET_IN_SUBID(in_id);
	out_id = HD_GET_OUT(path_id);
	out_subid = BD_GET_OUT_SUBID(out_id);

	if ((p_bind = get_bind(this_devid)) == NULL) {
		dif_get_dev_string(path_id, dev_string, sizeof(dev_string));
		GMLIB_ERROR_P("%s path_id(%#x) err\n", dev_string, path_id);
		goto ext; /* illegal, return NULL */
	}
	query.in_subid = in_subid;
	query.out_subid = out_subid;

	GMLIB_L1_P("%s ==>\n", __FUNCTION__);

	/* 1. check start_device to this_device are all start */
	p_tmp_bind = p_bind;
	do {
		GMLIB_L1_P("  prev: %s_%d port(%d %d) st(%d %d)\n", get_bind_name(p_tmp_bind),
			   bd_get_dev_subid(p_tmp_bind->device.deviceid),
			   (int) query.in_subid,
			   (int) query.out_subid,
			   (query.in_subid == (UINT32) - 1) ? 0 : (INT) p_tmp_bind->in[query.in_subid].port_state,
			   (query.out_subid == (UINT32) - 1) ? 0 : (INT) p_tmp_bind->in[query.out_subid].port_state);
		if (query.in_subid == (UINT32) - 1 || query.out_subid == (UINT32) - 1) {
			goto ext; /* exist port_state is not START or no binding, and return NULL */
		}

		if ((p_tmp_bind->in[query.in_subid].port_state != HD_PORT_START) ||
		    (p_tmp_bind->out[query.out_subid].port_state != HD_PORT_START)) {
			goto ext; /* exist port_state is not START, and return NULL */
		}
		p_check_bind = p_tmp_bind;
	} while ((p_tmp_bind = get_prev_bind(p_tmp_bind, query.in_subid, &query)) != NULL);

	/* 1.1 check the p_check_bind is start_device */
	if (is_start_device_p(p_check_bind->device.deviceid) != TRUE) {
		GMLIB_L1_P("  not(S): %s_%d port(%d %d) st(%d %d)\n", get_bind_name(p_check_bind),
			   bd_get_dev_subid(p_check_bind->device.deviceid),
			   (int) query.in_subid,
			   (int) query.out_subid,
			   (int) p_check_bind->in[query.in_subid].port_state,
			   (int) p_check_bind->out[query.out_subid].port_state);
		goto ext; /* it's not start_device, and return NULL */
	}

	/* 2. check this_device to end_device are all start */
	p_tmp_bind = p_check_bind = p_bind;
	query.in_subid = in_subid;
	query.out_subid = out_subid;

	if (is_end_device_p(p_tmp_bind->device.deviceid) == TRUE) {
		end_bind_array[0].p_bind = p_tmp_bind;
		end_bind_array[0].in_subid = in_subid;
		end_bind_array[0].out_subid = out_subid;
	} else {
		if ((listhead = get_next_bind_listhead(p_tmp_bind, query.out_subid)) == NULL) {
			GMLIB_L1_P("  not(E): %s_%d port(%d %d) st(%d %d)\n", get_bind_name(p_tmp_bind),
				   bd_get_dev_subid(p_tmp_bind->device.deviceid),
				   (int) query.in_subid,
				   (int) query.out_subid,
				   (int) p_tmp_bind->in[query.in_subid].port_state,
				   (int) p_tmp_bind->out[query.out_subid].port_state);
			goto ext; /* it's not start_device, and return NULL */
		}
		next_lh_bind_for_link_start(listhead, 1, end_bind_array);
	}
	for (i = 0; i < HD_MAX_MULTI_PATH; i++) {
		if (end_bind_array[i].p_bind != NULL) {
			is_end_exist = 1;
			GMLIB_L1_P("  all_start: %s_%d_IN_%d\n", get_bind_name(end_bind_array[i].p_bind),
				   bd_get_dev_subid(end_bind_array[i].p_bind->device.deviceid),
				   end_bind_array[i].in_subid);
		}
	}
ext:
	if (is_end_exist == 0) {
		GMLIB_L1_P("  not_all_start:\n");
	}
	return;
}

INT check_to_use_same_group(HD_BIND_INFO *p_end_bind, HD_GROUP *hd_group)
{
	UINT32 this_baseid, existed_baseid, prev_baseid = 0, this_device_subid, existed_device_subid;
	HD_BIND_INFO *p_bind, *p_prev_bind;
	INT ret = TRUE;

	this_baseid = bd_get_dev_baseid(p_end_bind->device.deviceid);
	this_device_subid = bd_get_dev_subid(p_end_bind->device.deviceid);
	if (hd_group->p_end_bind[0] == NULL) {
		return FALSE;
	}
	existed_baseid = bd_get_dev_baseid(hd_group->p_end_bind[0]->device.deviceid);
	existed_device_subid = bd_get_dev_subid(hd_group->p_end_bind[0]->device.deviceid);

	//find baseid of previous node, for audioout
	if ((p_bind = get_bind(p_end_bind->device.deviceid)) != NULL) {
		p_prev_bind = get_prev_bind(p_bind, 0, NULL);  // for audioout
		if (p_prev_bind) {
			prev_baseid = bd_get_dev_baseid(p_prev_bind->device.deviceid);
		}
	}

	if ((this_baseid == HD_DAL_VIDEOENC_BASE && existed_baseid == HD_DAL_VIDEOOUT_BASE) ||
	    (this_baseid == HD_DAL_VIDEOOUT_BASE && existed_baseid == HD_DAL_VIDEOENC_BASE) ||
	    (this_baseid == HD_DAL_VIDEOOUT_BASE && existed_baseid == HD_DAL_VIDEOOUT_BASE) || 
	    (this_baseid == HD_DAL_VIDEOENC_BASE && existed_baseid == HD_DAL_VIDEOENC_BASE)) {
		ret = TRUE;  // same group
	} else if ((prev_baseid == HD_DAL_AUDIOCAP_BASE || prev_baseid == HD_DAL_AUDIODEC_BASE) &&
		   (this_baseid == HD_DAL_AUDIOOUT_BASE) &&
		   (existed_baseid == HD_DAL_AUDIOOUT_BASE)) {

		/* 2. audio livesound and audio_playback are not the same group */
		/*    NOTE: audio enc is not */
		if ((this_device_subid != existed_device_subid)) { // for livesound and audio_playback
			ret = FALSE;
		} else {
			ret = TRUE;
		}

	} else {
		ret = FALSE;
	}
	GMLIB_L1_P("%s ==> ret(%d), %s_%d, %s_%d\n", __func__, ret,
		   get_device_name(p_end_bind->device.deviceid), bd_get_dev_subid(p_end_bind->device.deviceid),
		   get_device_name(hd_group->p_end_bind[0]->device.deviceid), bd_get_dev_subid(hd_group->p_end_bind[0]->device.deviceid));
	return ret;
}

HD_GROUP *get_mix_group(HD_BIND_INFO *p_end_bind)
{
	UINT32 i, j, find_flag = 0;
	HD_GROUP *hd_group = NULL;

	for (i = 0; i < BD_MAX_BIND_GROUP; i++) {
		if (bgroup[i].in_use != TRUE || bgroup[i].is_mix_group != TRUE)
			continue;

		hd_group = &bgroup[i]; // don't remove "hd_group = &bgroup[i]" order.

		if (bd_get_dev_baseid(p_end_bind->device.deviceid) != HD_DAL_VIDEOOUT_BASE)
			break;  /* mix: only use videoout, others don't need to set */

		/* store a new p_end_bind(videoout) */
		for (j = 0; j < HD_MAX_GROUP_END_DEVICE; j++) {
			if (hd_group->p_end_bind[j] == p_end_bind) {
				find_flag = 1;
				break;  // already store it
			} else if (hd_group->p_end_bind[j] == NULL) {
				hd_group->p_end_bind[j] = p_end_bind; // store new one
				find_flag = 1;
				break;
			}
		}
		/* error checking */
		if (find_flag == 0) {
			for (j = 0; j < HD_MAX_GROUP_END_DEVICE; j++) {
				GMLIB_ERROR_P("%d: %s_%d \n", j, get_bind_name(hd_group->p_end_bind[j]),
					      bd_get_dev_subid(hd_group->p_end_bind[j]->device.deviceid));
			}
			GMLIB_ERROR_P("end_bind overflow: %s_%d \n", get_bind_name(p_end_bind),
				      bd_get_dev_subid(p_end_bind->device.deviceid));
		}
	}

	/* lookup fail, new one mix_grop */
	if (hd_group == NULL) {
		for (i = 0; i < BD_MAX_BIND_GROUP; i++) {
			if (bgroup[i].in_use != TRUE) {  // it's new one
				memset(&bgroup[i], 0, sizeof(HD_GROUP));
				/* init hd_group */
				hd_group = &bgroup[i];
				hd_group->in_use = TRUE;
				hd_group->is_mix_group = TRUE;
				if (bd_get_dev_baseid(p_end_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
					hd_group->p_end_bind[0] = p_end_bind; /* mix: only use videoout, others don't need to set */
				} else {
					/* others don't need to set ?? */
				}
				break;
			}
		}
	}

	/* err checking */
	if (hd_group == NULL) {
		GMLIB_ERROR_P("internal_err: no new hd_group!\n");

		/* err list info print */
		for (i = 0; i < BD_MAX_BIND_GROUP; i++) {
			GMLIB_ERROR_P("  %d: end_bind list \n", i);
			for (j = 0; j < HD_MAX_GROUP_END_DEVICE; j++) {
				if (bgroup[i].p_end_bind[j] != NULL) {
					GMLIB_ERROR_P("    %d: %s_%d\n", j, get_bind_name(bgroup[i].p_end_bind[j]),
						      bd_get_dev_subid(bgroup[i].p_end_bind[j]->device.deviceid));
				}
			}
		}
	}
	return hd_group;
}

HD_GROUP *get_group(HD_BIND_INFO *p_end_bind)
{
	UINT32 i, j, find_flag = 0;
	HD_GROUP *hd_group = NULL;

	for (i = 0; i < BD_MAX_BIND_GROUP; i++) {
		if (bgroup[i].in_use != TRUE)
			continue;

		if (check_to_use_same_group(p_end_bind, &bgroup[i])) {
			hd_group = &bgroup[i];
			for (j = 0; j < HD_MAX_GROUP_END_DEVICE; j++) {
				if (hd_group->p_end_bind[j] == p_end_bind) {
					find_flag = 1;
					GMLIB_L2_P("%s (hit) idx(%d, %d) (%s_%d)\n", __func__, i, j,
						   get_bind_name(p_end_bind), bd_get_dev_subid(p_end_bind->device.deviceid));
					break;  // already store it
				} else if (hd_group->p_end_bind[j] == NULL) {
					hd_group->p_end_bind[j] = p_end_bind;
					find_flag = 1;
					GMLIB_L2_P("%s (add) idx(%d, %d) (%s_%d)\n", __func__, i, j,
						   get_bind_name(p_end_bind), bd_get_dev_subid(p_end_bind->device.deviceid));
					break;
				}
			}
			if (find_flag == 1) {
				goto chk_hd_group;
			} else {  /* error checking */
				for (j = 0; j < HD_MAX_GROUP_END_DEVICE; j++) {
					GMLIB_ERROR_P("%d: %s_%d \n", j, get_bind_name(hd_group->p_end_bind[j]),
						      bd_get_dev_subid(hd_group->p_end_bind[j]->device.deviceid));
				}
				GMLIB_ERROR_P("end_bind overflow: %s_%d \n", get_bind_name(p_end_bind),
					      bd_get_dev_subid(p_end_bind->device.deviceid));
			}
		} else {
			/* others are different hd_group */
			if (p_end_bind == bgroup[i].p_end_bind[0]) {
				/* for debug */
				for (j = 0; j < HD_MAX_GROUP_END_DEVICE; j++) {
					if (bgroup[i].p_end_bind[j] == NULL)
						continue;
					GMLIB_L2_P("%s (list) idx(%d, %d), (%s_%d)\n", __func__, i, j,
						   get_bind_name(bgroup[i].p_end_bind[j]),
						   bd_get_dev_subid(bgroup[i].p_end_bind[j]->device.deviceid));
				}
				hd_group = &bgroup[i];
				goto chk_hd_group;
			}
		}
	}

chk_hd_group:

	if (hd_group == NULL) { // lookup fail, new one
		for (i = 0; i < BD_MAX_BIND_GROUP; i++) {
			if (bgroup[i].in_use != TRUE) {  // it's new one
				memset(&bgroup[i], 0, sizeof(HD_GROUP));
				/* init hd_group */
				hd_group = &bgroup[i];
				hd_group->in_use = TRUE;
				hd_group->p_end_bind[0] = p_end_bind;
				GMLIB_L2_P("%s (new) hd_group(%p) idx(%d), (%s_%d)\n", __func__, hd_group, i,
					   get_bind_name(p_end_bind), bd_get_dev_subid(p_end_bind->device.deviceid));
				break;
			}
		}
	}

	/* err checking */
	if (hd_group == NULL) {
		GMLIB_ERROR_P("internal_err: no new hd_group!\n");

		/* err list info print */
		for (i = 0; i < BD_MAX_BIND_GROUP; i++) {
			GMLIB_ERROR_P("  %d: end_bind list \n", i);
			for (j = 0; j < HD_MAX_GROUP_END_DEVICE; j++) {
				if (bgroup[i].p_end_bind[j] != NULL) {
					GMLIB_ERROR_P("    %d: %s_%d\n", j, get_bind_name(bgroup[i].p_end_bind[j]),
						      bd_get_dev_subid(bgroup[i].p_end_bind[j]->device.deviceid));
				}
			}
		}
	}

	return hd_group;
}

VOID print_group(HD_GROUP *group)
{
	UINT32 line_idx, member_idx;
	HD_MEMBER *member;

	if (group->in_use != TRUE)
		goto ext;

	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (group->glines[line_idx].state == HD_LINE_INCOMPLETE)
			continue;

		for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {

			member = &group->glines[line_idx].member[member_idx];
			if (member->p_bind == NULL)
				break;
			GMLIB_ERROR_P("%d_%s(%d)_%d s[%d] ", member->in_subid, get_bind_name(member->p_bind),
				      bd_get_dev_subid(member->p_bind->device.deviceid), member->out_subid,
				      group->glines[line_idx].state);
		}
		GMLIB_ERROR_P("\n");
	}

ext:
	return;
}

BOOL is_multiout_p(HD_BIND_INFO *p_bind)
{
	INT32 out_nr;

	out_nr = get_bind_out_nr(p_bind);
	if (out_nr > 1)
		return TRUE;
	else
		return FALSE;
}

HD_BIND_PORT_WAY get_bind_in_out_way_p(HD_BIND_INFO *p_bind)
{
	INT32 in_nr, out_nr;
	HD_BIND_PORT_WAY ret = HD_BIND_N_TO_N_WAY;

	out_nr = get_bind_out_nr(p_bind);
	in_nr = get_bind_in_nr(p_bind);
	if (in_nr == out_nr) {
		ret = HD_BIND_N_TO_N_WAY;
	} else if (in_nr > 1 && out_nr == 1) {
		ret = HD_BIND_N_TO_1_WAY;
	} else if (in_nr == 1 && out_nr > 1) {
		ret = HD_BIND_1_TO_N_WAY;
	} else {
		GMLIB_ERROR_P("%s: internal_err, not support this %d_%d_WAY!\n", get_bind_name(p_bind), in_nr, out_nr);
	}

	return ret;
}

VOID set_bind_in_out_port_state_p(HD_BIND_INFO *p_bind, HD_PATH_ID path_id, HD_PORT_STATE port_state)
{
	HD_IO in_id, out_id;
	UINT32 in_subid, out_subid, i, is_exist_start = 0;
	INT32 port_nr;

	in_id = HD_GET_IN(path_id);
	in_subid = BD_GET_IN_SUBID(in_id);
	out_id = HD_GET_OUT(path_id);
	out_subid = BD_GET_OUT_SUBID(out_id);

	switch (get_bind_in_out_way_p(p_bind)) {
	case HD_BIND_1_TO_N_WAY:
		p_bind->out[out_subid].port_state = port_state;
		/* example case : videocap start/stop */
		/* in_subid = HD_PORT_STOP : all out_subid = HD_PORT_STOP */
		/* in_subid = HD_PORT_START: exist one out_subid = HD_PORT_START */
		port_nr = get_bind_out_nr(p_bind);
		if (port_nr < 0) {
			GMLIB_ERROR_P("%s: internal_err, port_out_nr(%d)!\n", get_bind_name(p_bind), port_nr);
			break;
		}
		for (i = 0; i < (UINT32)port_nr; i++) {
			if (p_bind->out[i].port_state == HD_PORT_START) {
				is_exist_start = 1;
				break;
			}
		}
		if (is_exist_start == 1) {
			p_bind->in[in_subid].port_state = HD_PORT_START;
		} else {
			p_bind->in[in_subid].port_state = HD_PORT_STOP;
		}
		break;
	case HD_BIND_N_TO_1_WAY:
		p_bind->in[in_subid].port_state = port_state;
		/* example case : videoout start/stop */
		/* out_subid = HD_PORT_STOP : all in_subid = HD_PORT_STOP */
		/* out_subid = HD_PORT_START: exist one in_subid = HD_PORT_START */
		port_nr = get_bind_in_nr(p_bind);
		if (port_nr < 0) {
			GMLIB_ERROR_P("%s: internal_err, port_in_nr(%d)!\n", get_bind_name(p_bind), port_nr);
			break;
		}
		for (i = 0; i < (UINT32)port_nr; i++) {
			if (p_bind->in[i].port_state == HD_PORT_START) {
				is_exist_start = 1;
				break;
			}
		}

		if (is_exist_start == 1) {
			p_bind->out[out_subid].port_state = HD_PORT_START;
		} else {
			p_bind->out[out_subid].port_state = HD_PORT_STOP;
		}
		break;
	case HD_BIND_N_TO_N_WAY:
		/* example case : videoenc, videodec and videoproc */
		p_bind->in[in_subid].port_state = port_state;
		p_bind->out[out_subid].port_state = port_state;
		break;
	default:
		GMLIB_ERROR_P("%s: internal_err, not support this port_way(%d)!\n", get_bind_name(p_bind), get_bind_in_out_way_p(p_bind));
		break;
	}
	//printf("%s:%d <%s_%d:(%d %d) (%d %d)!\n>\n",__FUNCTION__,__LINE__, get_bind_name(p_bind), bd_get_dev_subid(p_bind->device.deviceid),
	//							(int)in_subid,(int) out_subid,
	//							p_bind->in[in_subid].port_state, p_bind->out[out_subid].port_state);
	return;
}

HD_RESULT fill_bind_state_p(HD_DAL deviceid, HD_PATH_ID path_id, HD_PORT_STATE port_state)
{
	HD_BIND_INFO *p_bind;
	INT32 out_nr, in_nr, i;
	HD_IO out_id = HD_GET_CTRL(path_id);

	p_bind = get_bind(deviceid);
	if (p_bind == NULL) {
		GMLIB_ERROR_P("%s: internal_err, get p_bind NULL!\n", get_device_name(deviceid));
		goto err_ext;
	}

	out_nr = get_bind_out_nr(p_bind);
	in_nr = get_bind_in_nr(p_bind);
	if (out_id == HD_CTRL) {
		if (out_nr <= 0) {
			GMLIB_ERROR_P("%s: internal_err, out_nr error!\n", get_device_name(deviceid));
			goto err_ext;
		}
		for (i = 0; i < in_nr; i++) {
			p_bind->in[i].port_state = port_state;
		}
		for (i = 0; i < out_nr; i++) {
			p_bind->out[i].port_state = port_state;
		}
	} else {  // check out id
		set_bind_in_out_port_state_p(p_bind, path_id, port_state);
	}
	return HD_OK;

err_ext:
	return HD_ERR_NG;
}

VOID fill_line_state_p(HD_LINE_LIST *graph_lines, UINT32 line_idx, HD_LINE_STATE line_state)
{
	graph_lines[line_idx].state = line_state;
	return;
}

HD_LINE_STATE get_line_state_p(HD_LINE_LIST *graph_lines, UINT32 line_idx)
{
//	graph_lines[line_idx].state = line_state;
	return graph_lines[line_idx].state;
}

VOID fill_graph_members_p(HD_LINE_LIST *graph_lines, HD_MEMBER *member, UINT32 line_idx, UINT32 member_idx)
{
	graph_lines[line_idx].member[member_idx].in_subid = member->in_subid;
	graph_lines[line_idx].member[member_idx].out_subid = member->out_subid;
	graph_lines[line_idx].member[member_idx].p_bind = member->p_bind;
//	graph_lines[line_idx].state = HD_LINE_START_ONGOING;
	return;
}

INT32 get_min_insubid_for_disp_enc(HD_BIND_INFO *p_bind)
{
	INT32 in_nr, i, min_in_subid = -1, all_start;
	struct list_head *listhead;
	HD_BIND_INFO *p_tmp_bind;
	HD_QUERY_INFO query = {0};

	if (bd_get_dev_baseid(p_bind->device.deviceid) != HD_DAL_VIDEOOUT_BASE)
		goto ext;

	in_nr = get_bind_in_nr(p_bind);

	/* p_bind: multi_in to single_out, N-1 mapping */
	for (i = 0; i < in_nr; i++) {
		listhead = get_prev_bind_listhead(p_bind, i);
		if (listhead == NULL || list_empty(listhead))
			continue;
		query.in_subid = i;
		query.out_subid = 0;
		p_tmp_bind = p_bind;
		all_start = 1;
		do {
			GMLIB_L2_P("  disp_enc prev: %s_%d port(%d %d) st(%d %d)\n", get_bind_name(p_tmp_bind),
				   bd_get_dev_subid(p_tmp_bind->device.deviceid),
				   (int) query.in_subid,
				   (int) query.out_subid,
				   (int) p_tmp_bind->in[query.in_subid].port_state,
				   (int) p_tmp_bind->out[query.out_subid].port_state);

			if ((p_tmp_bind->in[query.in_subid].port_state != HD_PORT_START) ||
			    (p_tmp_bind->out[query.out_subid].port_state != HD_PORT_START)) {
				all_start = 0; /* exist port_state is not START, and return NULL */
				break;
			}
		} while ((p_tmp_bind = get_prev_bind(p_tmp_bind, query.in_subid, &query)) != NULL);
		if (all_start == 1) {
			min_in_subid = i;
			GMLIB_L1_P("disp_enc min_in_subid(%d)\n", min_in_subid);
			goto ext;
		}
	}
ext:
	return min_in_subid;
}

HD_RESULT fill_group_one_line_p(HD_GROUP_LINE *one_line, HD_MEMBER *end_member)
{
	INT32 idx = 0, i, j;
	HD_GROUP_LINE lline;
	HD_BIND_INFO *p_prev_bind, *p_last_bind, *p_tmp_bind;
	HD_QUERY_INFO query = {0}, query_tmp = {0};

	p_last_bind = end_member->p_bind;
	query.in_subid = end_member->in_subid;
	query.out_subid = end_member->out_subid;

	if ((INT32)end_member->in_subid == get_min_insubid_for_disp_enc(end_member->p_bind)) {
		p_tmp_bind = p_last_bind;
		query_tmp.in_subid = end_member->in_subid;
		query_tmp.out_subid = end_member->out_subid;
		/* check disp_enc, for enc device is start case. */
		while (p_tmp_bind != NULL) {
			if ((p_tmp_bind = get_next_bind(p_tmp_bind, query_tmp.out_subid, &query_tmp)) == NULL) {
				break;
			}
			if ((p_tmp_bind->in[query_tmp.in_subid].port_state != HD_PORT_START) ||
			    (p_tmp_bind->out[query_tmp.out_subid].port_state != HD_PORT_START)) {

				break;
			}
			if (bd_get_dev_baseid(p_tmp_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {

				/* disp_enc(cap->vpe->vout->vpe->venc): only encapsulate: vpe->venc */
				lline.member[idx].p_bind = p_tmp_bind;
				lline.member[idx].in_subid = query_tmp.in_subid;
				lline.member[idx].out_subid = query_tmp.out_subid;

				while ((p_tmp_bind = get_prev_start_bind(p_tmp_bind, query_tmp.in_subid, &query_tmp)) != NULL) {
					if (bd_get_dev_baseid(p_tmp_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
						idx++; /* add for next idx */
						break;
					}
					idx++;
					lline.member[idx].p_bind = p_tmp_bind;
					lline.member[idx].in_subid = query_tmp.in_subid;
					lline.member[idx].out_subid = query_tmp.out_subid;
				}
				goto next_fill_step; /* complete goto next */
			}
		}
	}

next_fill_step:
	p_prev_bind = p_last_bind;
	lline.member[idx].p_bind = p_prev_bind;
	lline.member[idx].in_subid = query.in_subid;
	lline.member[idx].out_subid = query.out_subid;

	while ((p_prev_bind = get_prev_start_bind(p_prev_bind, query.in_subid, &query)) != NULL) {
		idx++;
		lline.member[idx].p_bind = p_prev_bind;
		lline.member[idx].in_subid = query.in_subid;
		lline.member[idx].out_subid = query.out_subid;
	}
	if (is_start_device_p(lline.member[idx].p_bind->device.deviceid) == TRUE) {
		GMLIB_L1_P("%s ==> line_state(%d)\n", __func__, one_line->state);
		/* 1. fill members  */
		for (i = idx, j = 0; i >= 0; i--, j++) {
			one_line->member[j].p_bind = lline.member[i].p_bind;
			one_line->member[j].in_subid = lline.member[i].in_subid;
			one_line->member[j].out_subid = lline.member[i].out_subid;
			GMLIB_L1_P("  %d:%s_%d (%d %d) (%d %d)\n", (int) j, get_bind_name(one_line->member[j].p_bind),
				   bd_get_dev_subid(one_line->member[j].p_bind->device.deviceid),
				   (int) one_line->member[j].in_subid,
				   (int) one_line->member[j].out_subid,
				   (int) one_line->member[j].p_bind->in[one_line->member[j].in_subid].port_state,
				   (int) one_line->member[j].p_bind->out[one_line->member[j].out_subid].port_state);
		}
		for (j = idx + 1; j < HD_MAX_BIND_NUNBER_PER_LINE; j++) {
			one_line->member[j].p_bind = NULL;  // clear useless member p_bind
		}

		/* 2. set the start end1 end2 member */
		for (i = 0; i <= idx; i++) {
			if (i == 0) {
				one_line->s_idx = 0; // start member
			} else if (is_end_device_p(one_line->member[i].p_bind->device.deviceid)) {
				if (one_line->e1_idx == 0) {
					one_line->e1_idx = i;  // first end member
				} else if (one_line->e2_idx == 0) {
					one_line->e2_idx = i;  // second end member
				} else {
					GMLIB_ERROR_P("internal_err: third end member, NOT implement.\n");
					goto err_ext;
				}
			}
		}
	} else {
		GMLIB_ERROR_P("internal_err, no start_bind(%s:%d)!\n", get_bind_name(end_member->p_bind), end_member->in_subid);
		for (j = 0; j <= idx; j++) {  // show err bind info
			if (lline.member[j].p_bind != NULL) {
				GMLIB_ERROR_P("  %d:%s_%d (%d %d) (%d %d)\n", (int) j, get_bind_name(lline.member[j].p_bind),
					      bd_get_dev_subid(lline.member[j].p_bind->device.deviceid),
					      (int) lline.member[j].in_subid,
					      (int) lline.member[j].out_subid,
					      (int) lline.member[j].p_bind->in[lline.member[j].in_subid].port_state,
					      (int) lline.member[j].p_bind->out[lline.member[j].out_subid].port_state);
			}
		}
		goto err_ext;
	}

	return HD_OK;
err_ext:
	return HD_ERR_NG;
}


HD_LINE_STATE get_group_gline_state_p(HD_BIND_INFO *p_bind, HD_IO out_id)
{
	UINT32 i, line_idx, member_idx;
	HD_MEMBER *member;
	HD_LINE_STATE line_state = HD_LINE_INCOMPLETE;

	for (i = 0; i < BD_MAX_BIND_GROUP; i++) {
		if (bgroup[i].in_use != TRUE)
			continue;
		for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
			if (bgroup[i].glines[line_idx].state == HD_LINE_INCOMPLETE)
				continue;
			for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
				member = &bgroup[i].glines[line_idx].member[member_idx];

				if (member->p_bind == NULL)
					break;

				if ((member->p_bind == p_bind) && (member->out_subid == BD_GET_OUT_SUBID(out_id))) {
					line_state = bgroup[i].glines[line_idx].state;
					goto ext;
				}
			}
		}
	}
ext:
	return line_state;
}

VOID set_group_line_state_p(HD_GROUP *group, UINT32 line_idx, HD_PORT_STATE port_state)
{
	HD_GROUP_LINE *p_glines, *p_prev_glines;
	HD_LINE_STATE prev_line_state, line_state = HD_LINE_STOP;

	p_glines = &group->glines[0];
	p_prev_glines = &group->prev_glines[0];

	prev_line_state = p_prev_glines[line_idx].state;
	if (port_state == HD_PORT_START) {
		switch (prev_line_state) {
		case HD_LINE_INCOMPLETE: /* need gmi: param/bind/apply */
		case HD_LINE_STOP:  /* need gmi: param/bind/apply */
			line_state = HD_LINE_START_ONGOING;
			break;
		case HD_LINE_START: /* gmi: update param/apply, no new obj */
			line_state = HD_LINE_START;
			break;

		case HD_LINE_STOP_ONGOING:
		case HD_LINE_START_ONGOING:
		default:
			line_state = HD_LINE_STOP;
			GMLIB_ERROR_P("internal_err, pre_line_state(%d) line_state(%d) port_state(%d) err!\n",
				      prev_line_state, line_state, port_state);
			break;
		}

	} else {  /* port_state == HD_PORT_STOP */
		switch (prev_line_state) {
		case HD_LINE_INCOMPLETE:
			line_state = HD_LINE_INCOMPLETE;  /* need gmi: unbind/apply */
			break;

		case HD_LINE_START:
			line_state = HD_LINE_STOP_ONGOING;  /* need gmi: unbind/apply */
			break;
		case HD_LINE_STOP:
			line_state = HD_LINE_STOP;	/* gmi: do nothing */
			break;
		case HD_LINE_STOP_ONGOING:
		case HD_LINE_START_ONGOING:
		default:
			line_state = HD_LINE_STOP;
			GMLIB_ERROR_P("internal_err, pre_line_state(%d) line_state(%d) port_state(%d) err!\n",
				      prev_line_state, line_state, port_state);
			break;
		}
	}
	GMLIB_L1_P("%s: line(%d) prev_line_state(%d) port_state(%d) line_state(%d)\n", __func__,
		   line_idx, prev_line_state, port_state, line_state);

	p_glines[line_idx].state = line_state;
	return;
}

VOID set_line_state_p(HD_LINE_LIST *graph_lines, UINT32 line_idx, HD_PORT_STATE port_state)
{
	HD_LINE_STATE prev_line_state, line_state = HD_LINE_STOP;

	prev_line_state = get_line_state_p(graph_lines, line_idx);
	if (port_state == HD_PORT_START) {
		switch (prev_line_state) {
		case HD_LINE_INCOMPLETE: /* need gmi: bind/apply */
		case HD_LINE_STOP:  /* need gmi: bind/apply */
			line_state = HD_LINE_START_ONGOING;
			break;
		case HD_LINE_START: /* gmi: update param and do apply, no new obj */
			line_state = HD_LINE_START;
			break;

		case HD_LINE_STOP_ONGOING:
		case HD_LINE_START_ONGOING:
		default:
			line_state = HD_LINE_STOP;
			GMLIB_ERROR_P("internal_err, pre_line_state(%d) line_state(%d) port_state(%d) err!\n",
				      prev_line_state, line_state, port_state);
			break;
		}

	} else {  /* port_state == HD_PORT_STOP */
		switch (prev_line_state) {
		case HD_LINE_START:
			line_state = HD_LINE_STOP_ONGOING;  /* need gmi: unbind/apply */
			break;
		case HD_LINE_STOP:
			line_state = HD_LINE_STOP;	/* gmi: do nothing */
			break;
		case HD_LINE_STOP_ONGOING:
		case HD_LINE_START_ONGOING:
		case HD_LINE_INCOMPLETE:
		default:
			line_state = HD_LINE_STOP;
			GMLIB_ERROR_P("internal_err, pre_line_state(%d) line_state(%d) port_state(%d) err!\n",
				      prev_line_state, line_state, port_state);
			break;
		}
	}
	fill_line_state_p(graph_lines, line_idx, line_state);
	return;
}

HD_MEMBER *get_videodec_from_hd_group(HD_GROUP *hd_group, UINT32 line_idx)
{
	INT member_idx;
	HD_MEMBER *p_member;
	HD_MEMBER *p_vdec_member = NULL;

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (hd_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			break;
		}
		p_member = &(hd_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) {
			p_vdec_member = p_member;
			goto ext;
		}
	}
ext:
	return p_vdec_member;
}

/*  is_exist_share_dec_tx_block_bs : check dec share path
	if return FALSE, there isn't any dec share send tx block
	if return TRUE, there is other dec share it.
 */
BOOL is_exist_share_dec_tx_block_bs(HD_GROUP *hd_group, UINT32 self_line_idx)
{
	UINT32 line_idx;
	HD_MEMBER *p_vdec_member, *p_self_vdec_member;
	BOOL ret = FALSE;

	if ((p_self_vdec_member = get_videodec_from_hd_group(hd_group, self_line_idx)) == NULL) {
		goto ext; // it's not vdec
	}

	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (hd_group->glines[line_idx].dec_tx_state == HD_DEC_INACTIVE || line_idx == self_line_idx)  {
			continue;
		}
		if ((p_vdec_member = get_videodec_from_hd_group(hd_group, line_idx)) == NULL) {
			continue;  // it's not vdec
		}

		if (p_vdec_member->p_bind->device.deviceid == p_self_vdec_member->p_bind->device.deviceid &&
		    p_vdec_member->out_subid == p_self_vdec_member->out_subid) {

			if (hd_group->glines[line_idx].dec_tx_state == HD_DEC_ALREADY_TX_BLOCK ||
			    hd_group->glines[line_idx].dec_tx_state == HD_DEC_NEW_START) {

				ret = TRUE;
				goto ext;
			}
		}
	}
ext:
	return ret;
}

#if SEND_DEC_BLACK_BS
VOID handle_dec_tx_block(HD_GROUP *hd_group, UINT32 line_idx)
{
	HD_MEMBER *p_vdec_member;
	HD_DAL deviceid;
	UINT32 out_subid;
	HD_PATH_ID dec_pathid;
	VDODEC_INTL_PARAM *p_dec_param;
	HD_DIM user_dec_max_dim;
	HD_DEC_TX_BLACK_STATE prev_glines_tx_state;
	VDODEC_TX_STATE pre_dec_tx_state;
	CHAR dev_string[64];

	if ((p_vdec_member = get_videodec_from_hd_group(hd_group, line_idx)) == NULL) {
		goto ext;  // it's not vdec
	}

	deviceid = p_vdec_member->p_bind->device.deviceid;
	out_subid = p_vdec_member->out_subid;
	dec_pathid = BD_SET_PATH(deviceid, out_subid + HD_OUT_BASE, out_subid + HD_OUT_BASE);
	p_dec_param = videodec_get_param_p(deviceid, out_subid);
	if (p_dec_param == NULL || p_dec_param->max_mem == NULL) {
		dif_get_dev_string(dec_pathid, dev_string, sizeof(dev_string));
		GMLIB_ERROR_P("internal_err, %s path_id(%#x) get dec_param err!\n", dev_string, dec_pathid);
		goto ext;
	}

	pre_dec_tx_state = p_dec_param->tx_state;
	prev_glines_tx_state = hd_group->glines[line_idx].dec_tx_state;
	switch (hd_group->glines[line_idx].dec_tx_state) {
	case HD_DEC_NEW_START:
		if (p_dec_param->tx_state == DEC_TX_NOTHING_DONE) {

			GMLIB_L1_P("  %s:reserved_ref_flag(%d) ddrid(%d)\n", __func__, p_dec_param->reserved_ref_flag,
				   p_dec_param->max_mem->ddr_id);

			if (p_dec_param->max_mem && p_dec_param->max_mem->ddr_id == DDR_ID0) {
				user_dec_max_dim.w = p_dec_param->max_mem->max_width;
				user_dec_max_dim.h = p_dec_param->max_mem->max_height;
				dif_send_black_bs_to_vdec(dec_pathid, user_dec_max_dim, p_dec_param->max_mem->codec_type);
				GMLIB_L1_P("  %s:dec_tx_block (NEW_START)\n", __func__);
			}
			p_dec_param->reserved_ref_flag = 0;
			p_dec_param->tx_state = DEC_TX_BLACK_BS_DONE;
		} else { // p_dec_param->tx_state == DEC_TX_USR_BS_DONE or p_dec_param->tx_state == DEC_TX_BLACK_BS_DONE
			// do_nothing
		}
		hd_group->glines[line_idx].dec_tx_state = HD_DEC_ALREADY_TX_BLOCK;
		break;
	case HD_DEC_GO_STOP:
		if (p_dec_param->tx_state == DEC_TX_BLACK_BS_DONE || p_dec_param->tx_state == DEC_TX_USR_BS_DONE) {
			if (is_exist_share_dec_tx_block_bs(hd_group, line_idx)) {
				// do_nothing
			} else {
				p_dec_param->tx_state = DEC_TX_NOTHING_DONE;
			}
		}
		hd_group->glines[line_idx].dec_tx_state = HD_DEC_INACTIVE;
		break;
	case HD_DEC_ALREADY_TX_BLOCK:
		if (p_dec_param->tx_state == DEC_TX_NOTHING_DONE) {
			if (p_dec_param->max_mem && p_dec_param->max_mem->ddr_id == DDR_ID0) {
				user_dec_max_dim.w = p_dec_param->max_mem->max_width;
				user_dec_max_dim.h = p_dec_param->max_mem->max_height;
				dif_send_black_bs_to_vdec(dec_pathid, user_dec_max_dim, p_dec_param->max_mem->codec_type);
				GMLIB_L1_P("  %s:dec_tx_block done (TX_AGAIN)\n", __func__);
			}
			p_dec_param->tx_state = DEC_TX_BLACK_BS_DONE;
		} else {  //p_dec_param->tx_state == DEC_TX_BLACK_BS_DONE
			// do_nothing
		}
		break;
	case HD_DEC_INACTIVE:
	default:
		// do_nothing
		break;
	}
	GMLIB_L1_P("  %s:line(%d) dev_id(%#x_%d) dec_tx_state(%d->%d) glines_tx_state(%d->%d)\n", __func__,
		   line_idx, deviceid, out_subid, pre_dec_tx_state, p_dec_param->tx_state,
		   prev_glines_tx_state, hd_group->glines[line_idx].dec_tx_state);
ext:
	return;
}
#endif

VOID finish_line_state_p(HD_GROUP *hd_group, UINT32 line_idx)
{
	HD_LINE_STATE prev_line_state;
	BOOL is_new_start = FALSE, is_go_stop = FALSE;
	INT member_idx;
	HD_MEMBER *p_member;
	HD_DAL deviceid;
	UINT32 out_subid;
	VDOPROC_INTL_PARAM *p_vproc_param;
	HD_MEMBER *p_vdec_member;
	VDODEC_INTL_PARAM *p_dec_param;
#if SEND_DEC_BLACK_BS
	HD_DIM user_vproc_max_dim;
	HD_PATH_ID vproc_pathid;
#endif

	prev_line_state = hd_group->glines[line_idx].state;
	switch (prev_line_state) {
	case HD_LINE_STOP_ONGOING:
		hd_group->glines[line_idx].state = HD_LINE_STOP;
		/* NOTE: clear vpdFdinfo[1] = vpd_ch_fd->id = 0,
		 * for next time new apply to create new graph or old graph
		 */
		if (pif_clear_vpd_channel_fd(hd_group->groupfd, line_idx) < 0) {
			GMLIB_ERROR_P("internal_err, clear vpd channel fd err!\n");
		}
		if (get_videodec_from_hd_group(hd_group, line_idx) != NULL) {  // it's videodec line
			hd_group->glines[line_idx].dec_tx_state = HD_DEC_GO_STOP;
		}
		is_go_stop = TRUE;
		break;
	case HD_LINE_START_ONGOING:
		hd_group->glines[line_idx].state = HD_LINE_START;
		if ((p_vdec_member = get_videodec_from_hd_group(hd_group, line_idx)) != NULL) {
			deviceid = p_vdec_member->p_bind->device.deviceid;
			out_subid = p_vdec_member->out_subid;
			p_dec_param = videodec_get_param_p(deviceid, out_subid);
			if (p_dec_param != NULL) {
				if (p_dec_param->tx_state == DEC_TX_BLACK_BS_DONE) {
					p_dec_param->tx_state = DEC_TX_NOTHING_DONE;  // set it, to do tx black bs again,
				} else { // p_dec_param->tx_state == DEC_TX_USR_BS_DONE
					// do_nothing, because of usr have been tx bs.
				}
			}
			hd_group->glines[line_idx].dec_tx_state = HD_DEC_NEW_START;
		}
		is_new_start = TRUE;
		break;
	case HD_LINE_START:
		if ((p_vdec_member = get_videodec_from_hd_group(hd_group, line_idx)) != NULL) {
			deviceid = p_vdec_member->p_bind->device.deviceid;
			out_subid = p_vdec_member->out_subid;
			p_dec_param = videodec_get_param_p(deviceid, out_subid);
			if (p_dec_param != NULL) {
				if (p_dec_param->tx_state == DEC_TX_BLACK_BS_DONE) {
					p_dec_param->tx_state = DEC_TX_NOTHING_DONE;  // set it, to do tx black bs again,
				} else { // p_dec_param->tx_state == DEC_TX_USR_BS_DONE
					// do_nothing, because of usr have been tx bs.
				}
			}
		}
		break;
	case HD_LINE_STOP:
		/* NOTE: clear vpdFdinfo[1] = vpd_ch_fd->id = 0,
		 * for next time new apply to create new graph or old graph
		 */
		if (pif_clear_vpd_channel_fd(hd_group->groupfd, line_idx) < 0) {
			GMLIB_ERROR_P("internal_err, clear vpd channel fd err!\n");
		}
		if (get_videodec_from_hd_group(hd_group, line_idx) != NULL) { // it's videodec line
			hd_group->glines[line_idx].dec_tx_state = HD_DEC_INACTIVE;
		}
		is_go_stop = TRUE;
		break;
	default:
		break;
	}

	/* for vpe send black */
	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (hd_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(hd_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			deviceid = p_member->p_bind->device.deviceid;
			out_subid = p_member->out_subid;
			p_vproc_param = videoproc_get_param_p(deviceid, out_subid);

			if (p_vproc_param) {
				if (is_new_start) {
#if SEND_DEC_BLACK_BS
					if (p_vproc_param->p_src_bg_dim && p_vproc_param->p_src_fmt) {
						vproc_pathid = BD_SET_PATH(deviceid, out_subid + HD_OUT_BASE, out_subid + HD_OUT_BASE);
						user_vproc_max_dim.w = p_vproc_param->p_src_bg_dim->w;
						user_vproc_max_dim.h = p_vproc_param->p_src_bg_dim->h;
						dif_send_black_frame_to_vproc(vproc_pathid, user_vproc_max_dim, *p_vproc_param->p_src_fmt);
					}
#endif
					p_vproc_param->is_black_init = TRUE;
				} else if (is_go_stop) {
					p_vproc_param->is_black_init = FALSE;
				}
			}
		}
	}

	GMLIB_L1_P("  %s:line(%d) line_state(%d->%d)\n", __func__, line_idx, prev_line_state, hd_group->glines[line_idx].state);
	return;
}

UINT bd_check_exist(HD_PATH_ID path_id)
{
	HD_DAL deviceid;
	HD_IO in_id, out_id;
	UINT32 in_subid, out_subid, device_baseid, device_subid, device_nr;
	HD_BIND_INFO *p_bind;
	HD_DEVICE_BASE_INFO *p_dbase;

	deviceid = HD_GET_DEV(path_id);
	in_id = HD_GET_IN(path_id);
	out_id = HD_GET_OUT(path_id);
	p_bind = get_bind(deviceid);
	if (p_bind == NULL)
		return 0;

	device_baseid = bd_get_dev_baseid(deviceid);
	if (device_baseid == 0) {
		return 0;
	}

	device_subid = bd_get_dev_subid(deviceid);
	p_dbase = lookup_device_base(device_baseid);
	if (p_dbase == NULL)
		return 0;

	out_subid = BD_GET_OUT_SUBID(out_id);
	in_subid = BD_GET_IN_SUBID(in_id);

	device_nr = p_dbase->ability.max_device_nr;
	if (device_subid >= device_nr)
		return 0;
	if ((out_subid >= p_dbase->ability.max_out) || (in_subid >= p_dbase->ability.max_in))
		return 0;
	if (p_bind->in == 0)
		return 0;
	if (p_bind->out == 0)
		return 0;
	return (p_bind->out[out_subid].is_opened & p_bind->in[in_subid].is_opened);
}

UINT bd_is_path_start(HD_PATH_ID path_id)
{
	HD_DAL deviceid;
	HD_IO in_id, out_id;
	UINT32 in_subid, out_subid, device_baseid, device_subid, device_nr;
	HD_BIND_INFO *p_bind;
	HD_DEVICE_BASE_INFO *p_dbase;

	deviceid = HD_GET_DEV(path_id);
	in_id = HD_GET_IN(path_id);
	out_id = HD_GET_OUT(path_id);
	p_bind = get_bind(deviceid);
	if (p_bind == NULL)
		return 0;

	device_baseid = bd_get_dev_baseid(deviceid);
	if (device_baseid == 0) {
		return 0;
	}

	device_subid = bd_get_dev_subid(deviceid);
	p_dbase = lookup_device_base(device_baseid);
	if (p_dbase == NULL)
		return 0;

	out_subid = BD_GET_OUT_SUBID(out_id);
	in_subid = BD_GET_IN_SUBID(in_id);

	device_nr = p_dbase->ability.max_device_nr;
	if (device_subid >= device_nr)
		return 0;
	if ((out_subid >= p_dbase->ability.max_out) || (in_subid >= p_dbase->ability.max_in))
		return 0;
	if (p_bind->in == 0)
		return 0;
	if (p_bind->out == 0)
		return 0;
	return (p_bind->out[out_subid].port_state & p_bind->in[in_subid].port_state);
}

HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len)
{
	UINT32 in_subid;
	HD_BIND_INFO *p_bind = get_bind(deviceid);
	HD_BIND_INFO *prev_bind;
	HD_QUERY_INFO query = {0};

	if (p_bind == NULL)
		return HD_ERR_NG;

	in_subid = BD_GET_IN_SUBID(in_id);

	pthread_mutex_lock(&hdal_mutex);
	query.in_subid = in_subid;
	prev_bind = get_prev_bind(p_bind, in_subid, &query);
	pthread_mutex_unlock(&hdal_mutex);
	if (prev_bind == NULL)
		return HD_ERR_NG;

	snprintf(prev_name, prev_len, "%s_%d_OUT_%d", get_device_name(prev_bind->device.deviceid),
		 (int)bd_get_dev_subid(prev_bind->device.deviceid), (int)query.out_subid);
	return HD_OK;
}

HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len)
{
	UINT32 out_subid;
	HD_BIND_INFO *p_bind = get_bind(deviceid);
	HD_BIND_INFO *next_bind;
	HD_QUERY_INFO query = {0};

	if (p_bind == NULL)
		return HD_ERR_NG;

	out_subid = BD_GET_OUT_SUBID(out_id);

	pthread_mutex_lock(&hdal_mutex);
	query.out_subid = out_subid;
	next_bind = get_next_bind(p_bind, out_subid, &query);
	pthread_mutex_unlock(&hdal_mutex);
	if (next_bind == NULL)
		return HD_ERR_NG;

	snprintf(next_name, next_len, "%s_%d_IN_%d", get_device_name(next_bind->device.deviceid),
		 (int)bd_get_dev_subid(next_bind->device.deviceid), (int)query.in_subid);
	return HD_OK;
}

INT is_line_member_all_start(HD_GROUP_LINE *one_line)
{
	INT32 member_idx, out_subid, in_subid, is_all_start = 1;

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (one_line->member[member_idx].p_bind == NULL)
			break;
		out_subid = one_line->member[member_idx].out_subid;
		in_subid = one_line->member[member_idx].in_subid;

		if (one_line->member[member_idx].p_bind->out[out_subid].port_state == HD_PORT_STOP ||
		    one_line->member[member_idx].p_bind->in[in_subid].port_state == HD_PORT_STOP) {
			/* if there is any port_state is STOP, this line we update to STOP */
			one_line->tmp_port_state = HD_PORT_STOP;
			is_all_start = 0;
			break;
		}
	}

	return is_all_start;
}

HD_RESULT set_transcode_path_lines(HD_GROUP *hd_group)
{
	INT32 i, line_idx;
	HD_MEMBER member;
	HD_GROUP_LINE *one_line;
	HD_QUERY_INFO query = {0};

	/* 2. init new glines, encapsulate each lines from binding  */
	for (i = 0; i <= bd_transcode_path.max_in_use_idx; i++) {
		if (bd_transcode_path.end[i].p_bind == NULL)
			continue;

		/* only set transcode case */
		line_idx = (HD_MAX_GRAPH_LINE_PER_CHIP * HD_MAX_GROUP_END_DEVICE) + i;  //
		if (line_idx >= HD_MAX_GRAPH_LINE) {
			GMLIB_ERROR_P("internal_err: graph line overflow(%d, %d). \n", line_idx, HD_MAX_GRAPH_LINE);
			goto ext;
		}
		one_line = &hd_group->glines[line_idx];
		GMLIB_L1_P("%s ==> line_idx(%d)\n", __func__, line_idx);
		member.in_subid = bd_transcode_path.end[i].in_subid;
		member.out_subid = bd_transcode_path.end[i].out_subid;
		member.p_bind = bd_transcode_path.end[i].p_bind;
		if (get_graph_start_p(member.p_bind, member.in_subid, &query) != NULL) {
			if (fill_group_one_line_p(one_line, &member) != HD_OK) {
				GMLIB_ERROR_P("internal_err: fill line err.\n");
				continue;
			}
		}
		one_line->tmp_port_state = (is_line_member_all_start(one_line) == 1) ? HD_PORT_START : HD_PORT_STOP;
		set_group_line_state_p(hd_group, line_idx, one_line->tmp_port_state);
	}

ext:
	return HD_OK;
}

HD_RESULT set_lines(HD_GROUP *group)
{
	INT32 in_nr, i, line_idx, member_idx, group_end_bind_idx, is_zeroch_start = 1;
	HD_MEMBER member;
	HD_GROUP_LINE *one_line;
	UINT base_id, sub_id;
	HD_BIND_INFO *p_end_bind;

	/* 1. backup previous glines */
	memcpy(&group->prev_glines[0], &group->glines[0], sizeof(HD_GROUP_LINE) * HD_MAX_GRAPH_LINE);

	/* clear old data, except for vpd used */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (group->glines[line_idx].state != HD_LINE_START) {
			group->glines[line_idx].type = HD_LINE_NONE;
			group->glines[line_idx].state = HD_LINE_INCOMPLETE;
			memset(group->glines[line_idx].member, 0x0, sizeof(HD_MEMBER) * HD_MAX_BIND_NUNBER_PER_LINE);
		}
		group->glines[line_idx].is_zero_ch_vout = 0;
		group->glines[line_idx].s_idx = 0;
		group->glines[line_idx].e1_idx = 0;
		group->glines[line_idx].e2_idx = 0;
		group->glines[line_idx].tmp_port_state = HD_PORT_STOP;
		group->glines[line_idx].unit_bmp = 0;
	}

	/* 2. init new glines, encapsulate each lines from binding  */
	for (group_end_bind_idx = 0; group_end_bind_idx < HD_MAX_GROUP_END_DEVICE; group_end_bind_idx++) {
		if ((p_end_bind = group->p_end_bind[group_end_bind_idx]) == NULL)
			break;
		in_nr = get_bind_in_nr(p_end_bind);
		for (i = 0; i < in_nr; i++) {
			if (get_prev_bind(p_end_bind, i, NULL) == NULL)  {
				continue;  // no previous bind
			}
			if (bd_get_dev_baseid(p_end_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
				/* check it disp_to_enc */
				if (is_prev_exist_vout(p_end_bind, i) == TRUE) {
					continue;  // not apply it, because it belongs to videoout
				}
			}
			line_idx = group_end_bind_idx * HD_MAX_GRAPH_LINE_PER_CHIP + i;
			if (line_idx >= HD_MAX_GRAPH_LINE) {
				GMLIB_ERROR_P("internal_err: graph line overflow(%d, %d). \n", line_idx, HD_MAX_GRAPH_LINE);
				goto err_ext;
			}
			one_line = &group->glines[line_idx];

			if (is_multiout_p(p_end_bind))
				member.out_subid = i;
			else
				member.out_subid = 0;

			member.in_subid = i;
			member.p_bind = p_end_bind;
			GMLIB_L1_P("%s ==> line_idx(%d) (%s_%d) in_subid(%d)\n", __func__, line_idx,
				   get_bind_name(member.p_bind),
				   bd_get_dev_subid(member.p_bind->device.deviceid),
				   member.in_subid);
			if (fill_group_one_line_p(one_line, &member) != HD_OK) {
				GMLIB_ERROR_P("internal_err: fill line err.\n");
				continue;
			}

			for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
				if (one_line->member[member_idx].p_bind == NULL)
					break;

				base_id = bd_get_dev_baseid(one_line->member[member_idx].p_bind->device.deviceid);
				sub_id = bd_get_dev_subid(one_line->member[member_idx].p_bind->device.deviceid);
				if (base_id == HD_DAL_VIDEOOUT_BASE) {
					if (platform_sys_Info.lcd_info[sub_id].lcdid >= 0 &&
					    platform_sys_Info.lcd_info[sub_id].channel_zero == 1) {
						one_line->is_zero_ch_vout = 1;
					} else {
						one_line->is_zero_ch_vout = 0;
					}
				}
			}
		}

		/* 3. decide line port state */
		for (i = 0; i < in_nr; i++) {
			if (get_prev_bind(p_end_bind, i, NULL) == NULL)  {
				continue;  // no previous bind
			}
			if (bd_get_dev_baseid(p_end_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
				/* check it disp_to_enc */
				if (is_prev_exist_vout(p_end_bind, i) == TRUE)
					continue;  // not apply it, because it belongs to videoout
			}
			line_idx = group_end_bind_idx * HD_MAX_GRAPH_LINE_PER_CHIP + i;
			one_line = &group->glines[line_idx];
			one_line->tmp_port_state = (is_line_member_all_start(one_line) == 1) ? HD_PORT_START : HD_PORT_STOP;

			if (one_line->is_zero_ch_vout == 1 && one_line->tmp_port_state == HD_PORT_STOP) {
				is_zeroch_start = 0;
			}
		}

		/* 4. apply the state,
		 *    For zero channel, if any zero-ch line is STOP, all other zero-ch lines are STOP, too.
		 */
		for (i = 0; i < in_nr; i++) {
			if (get_prev_bind(p_end_bind, i, NULL) == NULL)  {
				continue;  // no previous bind
			}
			if (bd_get_dev_baseid(p_end_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
				/* check disp_to_enc */
				if (is_prev_exist_vout(p_end_bind, i) == TRUE)
					continue;  // not apply it, because it belongs to videoout
			}

			line_idx = group_end_bind_idx * HD_MAX_GRAPH_LINE_PER_CHIP + i;
			one_line = &group->glines[line_idx];

			GMLIB_L1_P("  i(%d) line(%d) tmp_port_state(%d) (%d, %d)\n", i, line_idx,
				   one_line->tmp_port_state, one_line->is_zero_ch_vout, is_zeroch_start);
			if (one_line->is_zero_ch_vout == 1 && is_zeroch_start == 0)
				one_line->tmp_port_state = HD_PORT_STOP;

			set_group_line_state_p(group, line_idx, one_line->tmp_port_state);
		}

	}
	if (group->is_mix_group == TRUE) {
		if (set_transcode_path_lines(group) == HD_ERR_NG) {
			goto err_ext;
		}
	}

	return HD_OK;

err_ext:
	return HD_ERR_NG;
}

HD_RESULT init_bind(HD_DEVICE_BASE_INFO *p_dbase)
{
	UINT32 i, device_nr, in_nr, out_nr, subid;
	HD_BIND_INFO *p_bind = NULL;

	device_nr = p_dbase->ability.max_device_nr;
	in_nr = p_dbase->ability.max_in;
	out_nr = p_dbase->ability.max_out;

	/* 3. alloc max bind buffer, bind_idx = deviceid */
	if ((p_bind = BD_CALLOC(sizeof(HD_BIND_INFO) * device_nr, 1)) == NULL) {
		GMLIB_ERROR_P("dbase(%d) alloc bind buffer size(%d) fail!\n", p_dbase->device_baseid, sizeof(HD_BIND_INFO) * device_nr);
		goto err_ext;
	}

	p_dbase->p_bind = p_bind;

	for (subid = 0; subid < device_nr; subid++) {

		p_bind[subid].device.deviceid = p_dbase->device_baseid + subid;
		if ((p_bind[subid].in = BD_CALLOC(sizeof(HD_DEV_NODE) * in_nr, 1)) == NULL) {
			GMLIB_ERROR_P("%s alloc in DEV_NODE buffer size(%d) fail!\n",
				      get_device_name(p_bind[subid].device.deviceid), (sizeof(HD_DEV_NODE) * in_nr));
			goto err_ext;
		}
		for (i = 0; i < in_nr; i++) {
			p_bind[subid].in[i].p_bind_info = &p_bind[subid];
			p_bind[subid].in[i].out_subid = i;
			p_bind[subid].in[i].is_opened = FALSE;
		}

		if ((p_bind[subid].out = BD_CALLOC(sizeof(HD_DEV_NODE) * out_nr, 1)) == NULL) {
			GMLIB_ERROR_P("%s alloc out DEV_NODE buffer size(%d) fail!\n",
				      get_device_name(p_bind[subid].device.deviceid), sizeof(HD_DEV_NODE) * out_nr);
			goto err_ext;
		}
		// device only alloc out[i].bnode, in[j].bnode come from up layer device
		for (i = 0; i < out_nr; i++) {
			if ((p_bind[subid].out[i].bnode = BD_CALLOC(sizeof(HD_BUF_NODE), 1)) == NULL) {
				GMLIB_ERROR_P("%s alloc BUF_NODE buffer (%d) fail!\n",
					      get_device_name(p_bind[subid].device.deviceid), sizeof(HD_BUF_NODE));
				goto err_ext;
			}
			p_bind[subid].out[i].p_bind_info = &p_bind[subid];
			p_bind[subid].out[i].out_subid = i;
			p_bind[subid].out[i].is_opened = FALSE;
			INIT_LIST_HEAD(&p_bind[subid].out[i].bnode->src_listhead);
			INIT_LIST_HEAD(&p_bind[subid].out[i].bnode->dst_listhead);
		}
	}
	return HD_OK;

err_ext:
	return HD_ERR_NG;
}

VOID uninit_bind(HD_DEVICE_BASE_INFO *p_dbase)
{
	HD_BIND_INFO *p_bind = NULL;
	UINT32 i, device_nr, out_nr, subid;

	device_nr = p_dbase->ability.max_device_nr;
	out_nr = p_dbase->ability.max_out;

	p_bind = p_dbase->p_bind;
	for (subid = 0; subid < device_nr; subid++) {

		if (p_bind[subid].in) {
			BD_FREE(p_bind[subid].in);
			p_bind[subid].in = 0;
		}

		for (i = 0; i < out_nr; i++) {
			if (p_bind[subid].out[i].bnode)
				BD_FREE(p_bind[subid].out[i].bnode);
		}

		if (p_bind[subid].out) {
			BD_FREE(p_bind[subid].out);
			p_bind[subid].out = 0;
		}
	}
	if (p_bind)
		BD_FREE(p_bind);

	return;
}

/**
     p_path_id = 0 -> open fail.
     p_path_id > 0 -> open OK
*/
HD_RESULT bd_device_open(HD_DAL deviceid, HD_IO out_id, HD_IO in_id, HD_PATH_ID *p_path_id)
{
	UINT32 device_baseid, device_subid, out_subid, in_subid, ctrl_id;
	HD_DEVICE_BASE_INFO *p_dbase = NULL;
	HD_BIND_INFO *p_bind;
	HD_RESULT ret = HD_OK;

	pthread_mutex_lock(&hdal_mutex);
	if (p_path_id == NULL) {
		GMLIB_ERROR_P("ERR: p_path_id is null\n");
		ret = HD_ERR_NULL_PTR;
		goto ext;
	}

	*p_path_id = 0; //init it

	device_baseid = bd_get_dev_baseid(deviceid);
	if (device_baseid == 0) {
		GMLIB_ERROR_P("ERR: deviceid(0x%x)\n", deviceid);
		ret = HD_ERR_DEV;
		goto ext;
	}

	device_subid = bd_get_dev_subid(deviceid);
	out_subid = BD_GET_OUT_SUBID(out_id);
	in_subid = BD_GET_IN_SUBID(in_id);
	ctrl_id = HD_GET_CTRL(out_id);
	p_dbase = lookup_device_base(device_baseid);

	GMLIB_L1_P("%s S: %s_%d_IN_%d_OUT_%d\n", __func__, get_device_name(deviceid), device_subid, in_subid, out_subid);

	if (p_dbase == NULL) {
		//for module_init set lcd mp function
		if (ctrl_id == HD_CTRL) {
			*p_path_id = BD_SET_CTRL(deviceid);
			ret = HD_OK;
			goto ext;
		} else {
			GMLIB_ERROR_P("ERR: deviceid(0x%x)\n", deviceid);
			ret = HD_ERR_DEV;
			goto ext;
		}
	}

	if ((in_id == 0) && (ctrl_id == HD_CTRL)) {
		/* 1. check ctrl path open  */
		*p_path_id = BD_SET_CTRL(deviceid);
	} else {
		/* 2. check general in/out open */
		if (in_id < HD_IN_BASE || out_id < HD_OUT_BASE) {
			GMLIB_ERROR_P("ERR: deviceid(%#x) in(%d) out(%d) is not available.\n", deviceid, in_id, out_id);
			ret = HD_ERR_DEV;
			goto ext;
		}
		if (device_subid >= p_dbase->ability.max_device_nr) {
			GMLIB_ERROR_P("ERR: deviceid(0x%x) is not available.\n", deviceid);
			ret = HD_ERR_DEV;
			goto ext;
		}
		if (out_subid >= p_dbase->ability.max_out) {
			GMLIB_ERROR_P("ERR: deviceid(%#x) out(%d) is not available.\n", deviceid, out_id);
			ret = HD_ERR_IO;
			goto ext;
		}
		if (in_subid >= p_dbase->ability.max_in) {
			GMLIB_ERROR_P("ERR: deviceid(%#x) in(%d) is not available.\n", deviceid, in_id);
			ret = HD_ERR_IO;
			goto ext;
		}

		*p_path_id = BD_SET_PATH(deviceid, in_id, out_id);
		if ((p_bind = get_bind(deviceid)) == NULL) {
			GMLIB_ERROR_P("ERR: p_bind == NULL.\n");
			ret = HD_ERR_PATH;
			goto ext;
		}
		if (p_bind->out[out_subid].is_opened == TRUE && p_bind->in[in_subid].is_opened == TRUE) {
			GMLIB_ERROR_P("open err, %s_OUT_%d is already open.\n", get_bind_name(p_bind), out_subid);
			ret = HD_ERR_ALREADY_OPEN;
			goto ext;
		}
		p_bind->out[out_subid].is_opened = TRUE;
		p_bind->in[in_subid].is_opened = TRUE;
	}

ext:
	GMLIB_L1_P("%s E: %s\n", __func__, get_device_name(deviceid));
	pthread_mutex_unlock(&hdal_mutex);
	return ret;
}

HD_RESULT bd_device_close(HD_PATH_ID path_id)
{
	UINT32 device_baseid, device_subid, in_subid, out_subid;
	HD_DAL deviceid = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO out_id = HD_GET_OUT(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	HD_RESULT ret = HD_OK;
	HD_DEVICE_BASE_INFO *p_dbase = NULL;
	HD_BIND_INFO *p_bind, *p_tmp_bind;
	HD_QUERY_INFO query = {0};

	pthread_mutex_lock(&hdal_mutex);
	device_baseid = bd_get_dev_baseid(deviceid);
	if (device_baseid == 0) {
		GMLIB_ERROR_P("ERR: deviceid(0x%x)\n", deviceid);
		ret = HD_ERR_DEV;
		goto ext;
	}

	device_subid = bd_get_dev_subid(deviceid);
	in_subid = BD_GET_IN_SUBID(in_id);
	out_subid = BD_GET_OUT_SUBID(out_id);

	p_dbase = lookup_device_base(device_baseid);
	if (p_dbase == NULL) {
		GMLIB_ERROR_P("ERR: deviceid(0x%x)\n", deviceid);
		ret = HD_ERR_DEV;
		goto ext;
	}
	GMLIB_L1_P("%s S: %s_%d_IN_%d_OUT_%d\n", __func__, get_device_name(deviceid), device_subid, in_subid, out_subid);

	if (ctrl_id == HD_CTRL) {
		/* do nothing  */
	} else {
		/* 2. check general in/out open */
		if (device_subid >= p_dbase->ability.max_device_nr) {
			GMLIB_ERROR_P("ERR: deviceid(0x%x) is not available.\n", deviceid);
			ret = HD_ERR_NOT_AVAIL;
			goto ext;
		}
		if (in_id < HD_IN_BASE || out_id < HD_OUT_BASE) {
			GMLIB_ERROR_P("%s path_id(%#x) error.\n", dif_return_dev_string(path_id), path_id);
			ret = HD_ERR_DEV;
			goto ext;
		}
		if (out_subid >= p_dbase->ability.max_out) {
			GMLIB_ERROR_P("ERR: deviceid(%#x) out(%d) is not available.\n", deviceid, out_id);
			ret = HD_ERR_NOT_AVAIL;
			goto ext;
		}

		p_bind = get_bind(deviceid);
		if (p_bind == NULL) {
			GMLIB_ERROR_P("%s: close err, get p_bind NULL!\n", get_device_name(deviceid));
			ret = HD_ERR_NOT_AVAIL;
			goto ext;
		}

		if (p_bind->out[out_subid].is_opened == FALSE && p_bind->in[in_subid].is_opened == FALSE) {
			GMLIB_ERROR_P("close err, %s_%d_OUT_%d path_id(%#x) is already closed.\n",
				      get_bind_name(p_bind), device_subid, out_subid, path_id);
			ret = HD_ERR_NOT_AVAIL;
			goto ext;
		}
		/* checking binding */
		query.out_subid = out_subid;
		if ((p_tmp_bind = get_next_bind(p_bind, out_subid, &query)) != NULL) {
			GMLIB_ERROR_P("close err, %s_%d_OUT_%d path_id(%#x) is still bind to %s_IN_%d, please stop and unbind.\n",
				      get_bind_name(p_bind), device_subid, out_subid, path_id,
				      get_bind_name(p_tmp_bind), query.in_subid);
			ret = HD_ERR_NOT_AVAIL;
			goto ext;
		}

		query.in_subid = in_subid;
		if ((p_tmp_bind = get_prev_bind(p_bind, in_subid, &query)) != NULL) {
			GMLIB_ERROR_P("close err, %s_%d_IN_%d path_id(%#x) is still bound by %s_%d_OUT_%d, please stop and unbind.\n",
				      get_bind_name(p_bind), bd_get_dev_subid(p_bind->device.deviceid), in_subid, path_id,
				      get_bind_name(p_tmp_bind), bd_get_dev_subid(p_tmp_bind->device.deviceid), query.out_subid);
			ret = HD_ERR_NOT_AVAIL;
			goto ext;
		}

		p_bind->out[out_subid].is_opened = FALSE;
		p_bind->in[in_subid].is_opened = FALSE;

	}

ext:
	GMLIB_L1_P("%s E: %s\n", __func__, get_device_name(deviceid));
	pthread_mutex_unlock(&hdal_mutex);
	return ret;
}

HD_RESULT bd_get_already_open_pathid(HD_DAL deviceid, HD_IO out_id, HD_IO in_id, HD_PATH_ID *p_path_id)
{
	if (p_path_id == NULL) {
		GMLIB_ERROR_P("ERR: p_path_id is null\n");
		return HD_ERR_NULL_PTR;
	}

	if ((in_id == 0) && (HD_GET_CTRL(out_id) == HD_CTRL)) {
		*p_path_id = BD_SET_CTRL(deviceid);
	} else {
		*p_path_id = BD_SET_PATH(deviceid, in_id, out_id);
	}
	return HD_OK;
}

HD_RESULT bd_bind(HD_DAL src_deviceid, HD_IO out_id, HD_DAL dst_deviceid, HD_IO in_id)
{
	HD_BIND_INFO *p_src_bind, *p_dst_bind, *p_tmp_bind;
	HD_BUF_NODE *bnode;
	UINT32 out_subid, in_subid;
	HD_QUERY_INFO query = {0};

	pthread_mutex_lock(&hdal_mutex);
	p_src_bind = get_bind(src_deviceid);
	if (p_src_bind == NULL) {
		GMLIB_ERROR_P("%s: internal_err, get p_src_bind NULL!\n", get_device_name(src_deviceid));
		goto err_ext;
	}
	if (out_id < HD_OUT_BASE) {
		GMLIB_ERROR_P("%s:out_id(%x) is not support!\n", get_bind_name(p_src_bind), out_id);
		goto err_ext;
	}

	p_dst_bind = get_bind(dst_deviceid);
	if (p_dst_bind == NULL) {
		GMLIB_ERROR_P("%s: internal_err, get p_dst_bind NULL!\n", get_device_name(dst_deviceid));
		goto err_ext;
	}
	if (in_id < HD_IN_BASE) {
		GMLIB_ERROR_P("%s:in_id(%x) is not is not support!\n", get_bind_name(p_dst_bind), in_id);
		goto err_ext;
	}

	in_subid = BD_GET_IN_SUBID(in_id);
	if ((INT32) in_subid >= get_bind_in_nr(p_dst_bind)) {
		GMLIB_ERROR_P("%s:in_id(%x) is not is not support!\n", get_bind_name(p_dst_bind), in_id);
		goto err_ext;
	}

	out_subid = BD_GET_OUT_SUBID(out_id);
	if ((INT32) out_subid >= get_bind_out_nr(p_src_bind)) {
		GMLIB_ERROR_P("%s:out_id(%x) is not support!\n", get_bind_name(p_src_bind), out_id);
		goto err_ext;
	}
	GMLIB_L1_P("%s: %s_%d_OUT_%d(%d) -> %s_%d_IN_%d(%d)\n", __func__,
		   get_bind_name(p_src_bind), bd_get_dev_subid(p_src_bind->device.deviceid),
		   out_subid, p_src_bind->out[out_subid].is_opened,
		   get_bind_name(p_dst_bind), bd_get_dev_subid(p_dst_bind->device.deviceid),
		   in_subid, p_dst_bind->in[in_subid].is_opened);
	/* checking open state */
	if ((p_src_bind->out[out_subid].is_opened != TRUE) || (p_dst_bind->in[in_subid].is_opened != TRUE)) {
		GMLIB_ERROR_P("binding err, %s_%d_OUT_%d(%d) or %s_%d_IN_%d(%d) is not open!\n",
			      get_bind_name(p_src_bind), bd_get_dev_subid(p_src_bind->device.deviceid),
			      out_subid, p_src_bind->out[out_subid].is_opened,
			      get_bind_name(p_dst_bind), bd_get_dev_subid(p_dst_bind->device.deviceid),
			      in_subid, p_dst_bind->in[in_subid].is_opened);
#if (BD_DUMP_ERR_LOG == 1)
		system("cat /proc/videograph/dumplog");
#endif
		goto err_ext;
	}

#if 0
	/* checking src already binding */
	if ((p_tmp_bind = get_next_bind(p_src_bind, out_subid, &query)) != NULL) {
		GMLIB_ERROR_P("binding err, %s_OUT_%d is already bind to %s_IN_%d!\n", get_bind_name(p_src_bind), out_subid,
			      get_bind_name(p_tmp_bind), query.in_subid);
		goto err_ext;
	}
#endif

	if ((p_dst_bind->in[in_subid].port_state != HD_PORT_STOP)) {
		GMLIB_ERROR_P("binding err,  %s_%d_IN_%d(%d) is not stopped!\n",
			      get_bind_name(p_dst_bind), bd_get_dev_subid(p_dst_bind->device.deviceid),
			      in_subid, p_dst_bind->in[in_subid].is_opened);
#if (BD_DUMP_ERR_LOG == 1)
		system("cat /proc/videograph/dumplog");
#endif
		goto err_ext;
	}

	/* checking dst already binding */
	if ((p_tmp_bind = get_prev_bind(p_dst_bind, in_subid, &query)) != NULL) {
		GMLIB_ERROR_P("binding err, %s_%d_IN_%d is already bound by %s_%d_OUT_%d!\n",
			      get_bind_name(p_dst_bind), bd_get_dev_subid(p_dst_bind->device.deviceid), in_subid,
			      get_bind_name(p_tmp_bind), bd_get_dev_subid(p_tmp_bind->device.deviceid), query.out_subid);

#if (BD_DUMP_ERR_LOG == 1)
		system("cat /proc/videograph/dumplog");
#endif
		goto err_ext;
	}

	/*
	*                                         src_bind_out(out)
	*                                        /
	*                      bnode.src_listhead
	*
	*                      bnode.dst_listhead
	*                     /                  \
	*  dst_bind_in_other(in)                   dst_bind(in)
	*/
	bnode = p_src_bind->out[out_subid].bnode;
//printf("%s:%d <(%s:0x%x)-->(%s:0x%x) >\n",__FUNCTION__,__LINE__, get_bind_name(p_src_bind), out_subid, get_bind_name(p_dst_bind), in_subid);
//printf("%s:%d <(%s:0x%x)-->(%s:0x%x) bnode(%p)>\n",__FUNCTION__,__LINE__, get_bind_name(p_src_bind), out_subid, get_bind_name(p_dst_bind), in_subid, bnode);


	if (list_empty(&bnode->src_listhead)) {
//printf("%s:%d <(%s:0x%x)-->(%s:0x%x) >\n",__FUNCTION__,__LINE__, get_bind_name(p_src_bind), out_subid, get_bind_name(p_dst_bind), in_subid);
		list_add_tail(&p_src_bind->out[out_subid].list, &bnode->src_listhead);
//		p_src_bind->out[out_subid].p_bind_info = p_src_bind;
	}
	list_add_tail(&p_dst_bind->in[in_subid].list, &bnode->dst_listhead);
	p_dst_bind->in[in_subid].bnode = bnode;  // dst_bind.in.bnode same with src_bind.out.bnode
//printf("%s:%d <(%s:0x%x)-->(%s:0x%x) bnode(%p)>\n",__FUNCTION__,__LINE__, get_bind_name(p_src_bind), out_subid, get_bind_name(p_dst_bind), in_subid, bnode);

	if (is_middle_device_p(p_src_bind->device.deviceid)) {
		p_src_bind->out[out_subid].in_subid = 0;
		p_src_bind->out[out_subid].out_subid = out_subid;
//printf("%s:%d <%s_%d, p_src_bind->out[%d].in_subid(%d)>\n",__FUNCTION__,__LINE__,
//	get_bind_name(p_src_bind), bd_get_dev_subid(p_src_bind->device.deviceid), out_subid, p_src_bind->out[out_subid].in_subid);
	}
	if (is_middle_device_p(p_dst_bind->device.deviceid)) {
		p_dst_bind->in[in_subid].in_subid = in_subid;
		p_dst_bind->in[in_subid].out_subid = -1;
//printf("%s:%d <%s_%d, p_dst_bind->in[%d].out_subid(%d)>\n",__FUNCTION__,__LINE__,
//	get_bind_name(p_dst_bind), bd_get_dev_subid(p_dst_bind->device.deviceid), in_subid, p_dst_bind->in[in_subid].out_subid);
	}
	if (is_start_device_p(p_src_bind->device.deviceid)) {
		p_src_bind->out[out_subid].in_subid = -1;
		p_src_bind->out[out_subid].out_subid = out_subid;
//printf("%s:%d <%s_%d, p_src_bind->out[%d].in_subid(%d) .out_subid(%d)>\n",__FUNCTION__,__LINE__,
//	get_bind_name(p_src_bind), bd_get_dev_subid(p_src_bind->device.deviceid), out_subid, p_src_bind->out[out_subid].in_subid, p_src_bind->out[out_subid].out_subid);
	}
	if (is_end_device_p(p_dst_bind->device.deviceid)) {
		p_dst_bind->in[in_subid].in_subid = in_subid;
		p_dst_bind->in[in_subid].out_subid = -1;
//printf("%s:%d <%s_%d, p_dst_bind->in[%d].in_subid(%d) .out_subid(%d)>\n",__FUNCTION__,__LINE__,
//	get_bind_name(p_dst_bind), bd_get_dev_subid(p_dst_bind->device.deviceid), in_subid, p_dst_bind->in[in_subid].in_subid, p_dst_bind->in[in_subid].out_subid);
	}
//printf("%s:%d <(%s:%d)-->(%s:%d) >\n",__FUNCTION__,__LINE__, get_bind_name(p_src_bind), out_id, get_bind_name(p_dst_bind), in_id);

	pthread_mutex_unlock(&hdal_mutex);
	return HD_OK;

err_ext:
	pthread_mutex_unlock(&hdal_mutex);
	return HD_ERR_NG;
}

HD_RESULT bd_unbind(HD_DAL self_deviceid, HD_IO out_id)
{
	HD_BIND_INFO *p_src_bind, *p_dst_bind;
	UINT32 out_subid, in_subid;
	struct list_head *listhead;
	HD_DEV_NODE *dev_node, *dev_node_n;
	HD_BIND_PORT_WAY port_way;

	pthread_mutex_lock(&hdal_mutex);

	p_src_bind = get_bind(self_deviceid);
	if (p_src_bind == NULL) {
		GMLIB_ERROR_P("%s: internal_err, get p_src_bind NULL!\n", get_device_name(self_deviceid));
		goto err_ext;
	}
	if (out_id < HD_OUT_BASE) {
		GMLIB_ERROR_P("%s:out_id(%x) is not is not support!\n", get_bind_name(p_src_bind), out_id);
		goto err_ext;
	}

	out_subid = BD_GET_OUT_SUBID(out_id);
	if ((INT32) out_subid >= get_bind_out_nr(p_src_bind)) {
		GMLIB_ERROR_P("%s:out_id(%x) is not is not support!\n", get_bind_name(p_src_bind), out_id);
		goto err_ext;
	}
	GMLIB_L1_P("%s: %s_%d_OUT_%d(%d) unbind\n", __func__, get_bind_name(p_src_bind),
		   bd_get_dev_subid(p_src_bind->device.deviceid),
		   out_subid, p_src_bind->out[out_subid].is_opened);

	port_way = get_bind_in_out_way_p(p_src_bind);
	if ((port_way == HD_BIND_1_TO_N_WAY) || (port_way == HD_BIND_N_TO_N_WAY)) {
		if (p_src_bind->out[out_subid].port_state != HD_PORT_STOP) {
			GMLIB_ERROR_P("%s_%d_OUT_%d, Error to unbind, should be stop at first\n",
				      get_bind_name(p_src_bind), bd_get_dev_subid(p_src_bind->device.deviceid),
				      out_subid);
			goto err_ext;  //because it will memory corruption issue.
		}
	}

	/* checking src already binding */
	if (get_next_bind(p_src_bind, out_subid, NULL) == NULL) {
		GMLIB_ERROR_P("unbind err, %s_%d_OUT_%d is already unbind!\n",
			      get_bind_name(p_src_bind),
			      bd_get_dev_subid(p_src_bind->device.deviceid), out_subid);
		goto err_ext;
	}

	/*
	*                                         src_bind_out(out) <-- self_id
	*                                        /
	*                      bnode.src_listhead
	*
	*                      bnode.dst_listhead
	*                     /                  \
	*  dst_bind_in_other(in)                   dst_bind(in)
	*/
	listhead = get_next_bind_listhead(p_src_bind, out_subid);

	if (listhead == NULL)
		goto err_ext;

	list_for_each_entry_safe(dev_node, dev_node_n, listhead, list) {
		in_subid = dev_node->in_subid;
		p_dst_bind = (HD_BIND_INFO *)dev_node->p_bind_info;
		list_del(&p_dst_bind->in[in_subid].list);
		p_dst_bind->in[in_subid].bnode = NULL;  // dst_bind.in.bnode same with src_bind.out.bnode, we don't keep.

		port_way = get_bind_in_out_way_p(p_dst_bind);
		if ((port_way == HD_BIND_N_TO_1_WAY) || (port_way == HD_BIND_N_TO_N_WAY) || (port_way == HD_BIND_1_TO_N_WAY)) {
			if (p_dst_bind->in[in_subid].port_state != HD_PORT_STOP) {
				GMLIB_ERROR_P("%s_%d_OUT_%d unbind %s_%d_IN_%d\n", get_bind_name(p_src_bind),
					   bd_get_dev_subid(p_src_bind->device.deviceid), out_subid,
					   get_bind_name(p_dst_bind), bd_get_dev_subid(p_dst_bind->device.deviceid), in_subid);
				GMLIB_ERROR_P("%s_%d_IN_%d should be stop at first\n",
					      get_bind_name(p_dst_bind), bd_get_dev_subid(p_dst_bind->device.deviceid), in_subid);
#if (BD_DUMP_ERR_LOG == 1)
				system("cat /proc/videograph/dumplog");
#endif

				goto err_ext; //because it will memory corruption issue.
			}
		}
	}
	if (list_empty(&p_src_bind->out[out_subid].bnode->src_listhead)) {
		GMLIB_ERROR_P("unbind: This (%s: %d, %d) no binded!\n", get_bind_name(p_src_bind),
			      bd_get_dev_subid(p_src_bind->device.deviceid), out_subid);
		goto err_ext;
	}

	list_del(&p_src_bind->out[out_subid].list);

	pthread_mutex_unlock(&hdal_mutex);
	return HD_OK;

err_ext:
	pthread_mutex_unlock(&hdal_mutex);
	return HD_ERR_NG;
}

HD_RESULT bd_apply_attr(HD_DAL dev_id, HD_IO out_id, INT param_id)
{
	HD_RESULT ret;

	pthread_mutex_lock(&hdal_mutex);
	ret = dif_apply_attr(dev_id, out_id, param_id);
	pthread_mutex_unlock(&hdal_mutex);

	return ret;
}

HD_RESULT bind_apply_finish_p(HD_GROUP *hd_group)
{
	UINT32 line_idx;
	BOOL is_group_empty = TRUE;

	/* clear useless multi transcode */
	clear_useless_multi_transcode();

	/* set line to START/STOP */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE)
			continue;

		finish_line_state_p(hd_group, line_idx);

		if (hd_group->glines[line_idx].state == HD_LINE_START)
			is_group_empty = FALSE;
	}
#if SEND_DEC_BLACK_BS
	/* handle dec tx block after set glines state, becasue two lines may share the same dec */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE)
			continue;
		handle_dec_tx_block(hd_group, line_idx);
	}
#endif

	if (is_group_empty == TRUE) {
		if (pif_delete_groupfd(hd_group->groupfd) < 0) {
			GMLIB_ERROR_P("%s: internal_err, delete group fd fail!\n", get_bind_name(hd_group->p_end_bind[0]));
			goto err_ext;
		}
		hd_group->groupfd = 0;
	}

	return HD_OK;
err_ext:
	return HD_ERR_NG;
}

static HD_GROUP unbind_hd_group;

HD_RESULT bind_lv_unbind_apply_p(HD_GROUP *hd_group)
{
	HD_RESULT ret = HD_ERR_NG;
	UINT line_idx;
	INT buf_size = 0;
	BOOL is_apply_change = FALSE;
	pif_group_t *p_group;
	void *org_encap_buf = NULL;

	/* keep the original hd_group */
	memcpy(&unbind_hd_group, hd_group, sizeof(unbind_hd_group));

	if (unbind_hd_group.groupfd == 0) {
		GMLIB_ERROR_P("internal_err: unbind_hd_group.groupfd is 0\n");
		goto ext;
	}

	/* init */
	p_group = pif_get_group_by_groupfd(unbind_hd_group.groupfd);
	if (p_group == NULL) {
		GMLIB_ERROR_P("pif_get_group_by_groupfd fail\n");
		goto ext;
	} else {
		buf_size = ((vpd_graph_t *) p_group->encap_buf)->buf_size;
		if ((org_encap_buf = BD_MALLOC(buf_size)) == NULL) {
			GMLIB_ERROR_P("alloc poll memory fail, size(%d).\n", buf_size);
			goto ext;
		}
		memcpy(org_encap_buf, p_group->encap_buf, buf_size);

		p_group->hd_group = &unbind_hd_group;
		if (pif_preset_group(p_group) < 0) {
			GMLIB_ERROR_P("pif_preset_group fail\n");
			goto ext;
		}
		memset(&unbind_hd_group.entity_buf_offs, 0x0, sizeof(INT) * HD_MAX_GRAPH_LINE * HD_MAX_BIND_NUNBER_PER_LINE);
		unbind_hd_group.entity_buf_len = 0;
	}

	/* 1. set link type, main point: take out liveview line  */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {

		if (dif_process_audio_livesound(&unbind_hd_group, line_idx) != HD_OK) {
			GMLIB_ERROR_P("internal_err: dif_process_audio_livesound err.\n");
			goto ext;
		}

		if (unbind_hd_group.glines[line_idx].state == HD_LINE_INCOMPLETE || unbind_hd_group.glines[line_idx].state == HD_LINE_STOP)
			continue;

		if (dif_is_lv(&unbind_hd_group, line_idx) == TRUE) {  /* not liveview */
			is_apply_change = TRUE;
			continue;  // takeout this liveview line, and set is_apply_change TRUE to apply
		}

		if (unbind_hd_group.glines[line_idx].state == HD_LINE_STOP_ONGOING) {
			is_apply_change = TRUE;
			continue;
		}

		/* others we have to set line information */
		is_apply_change = TRUE;
		if (dif_bind_set(&unbind_hd_group, line_idx) != HD_OK) {
			GMLIB_ERROR_P("internal_err: bind set err! need to check unit param.\n");
#if (BD_DUMP_ERR_LOG == 1)
			system("cat /proc/videograph/dumplog");
#endif
			goto ext;
		}
		unbind_hd_group.glines[line_idx].groupidx = unbind_hd_group.groupfd;
	}
	if (is_apply_change == TRUE) {
		GMLIB_L1_P("apply: START liveview unbind groupfd=%p\n", unbind_hd_group.groupfd);
		if (pif_apply(unbind_hd_group.groupfd) < 0)
			goto ext;
		if (bind_apply_finish_p(&unbind_hd_group) != HD_OK)
			goto ext;
		GMLIB_L1_P("apply: END liveview unbind groupfd=%p\n", unbind_hd_group.groupfd);
	}
	if (buf_size == 0) {
		GMLIB_ERROR_P("internal_err: encap_buf size = 0.\n");
		goto ext;
	}

	/* restore encap_buf */
	memcpy(p_group->encap_buf, org_encap_buf, buf_size);
	p_group->hd_group = hd_group;
	ret = HD_OK;

ext:
	if (org_encap_buf) {
		BD_FREE(org_encap_buf);
	}
	return ret;
}

HD_RESULT bind_apply_p(HD_GROUP *hd_group)
{
	UINT line_idx;
	BOOL is_apply_change = FALSE, is_liveview_changed = FALSE;
	pif_group_t *p_group;

	if (hd_group == NULL) {
		GMLIB_ERROR_P("internal_err: hd_group is NULL!\n");
#if (BD_DUMP_ERR_LOG == 1)
		system("cat /proc/videograph/dumplog");
#endif
		goto err_ext;
	}

	if (set_lines(hd_group) != HD_OK) {
		GMLIB_ERROR_P("internal_err: hd_group(%p) set_lines fail\n", hd_group);
#if (BD_DUMP_ERR_LOG == 1)
		system("cat /proc/videograph/dumplog");
#endif
		goto err_ext;
	}

	/* set hd_group fd  */
	if (hd_group->groupfd == 0) {
		hd_group->groupfd = pif_new_groupfd(); // new group fd
		if (hd_group->groupfd == 0) {
			GMLIB_ERROR_P("pif_new_groupfd fail\n");
			goto err_ext;
		}
	}
	p_group = pif_get_group_by_groupfd(hd_group->groupfd);
	if (p_group == NULL) {
		GMLIB_ERROR_P("pif_get_group_by_groupfd fail\n");
		goto err_ext;
	} else {
		p_group->hd_group = hd_group;
		if (pif_preset_group(p_group) < 0) {
			GMLIB_ERROR_P("pif_preset_group fail\n");
			goto err_ext;
		}
		memset(&hd_group->entity_buf_offs, 0x0, sizeof(INT) * HD_MAX_GRAPH_LINE * HD_MAX_BIND_NUNBER_PER_LINE);
		hd_group->entity_buf_len = 0;
	}

	/* 1. set link type  */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {

		if (dif_process_audio_livesound(hd_group, line_idx) != HD_OK) {
			GMLIB_ERROR_P("internal_err: dif_process_audio_livesound err.\n");
			goto err_ext;
		}

		if (hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE || hd_group->glines[line_idx].state == HD_LINE_STOP)
			continue;

		/* 1.2 set link unbind */
		if (hd_group->glines[line_idx].state == HD_LINE_STOP_ONGOING) {
			is_apply_change = TRUE;
			continue;
		}

		/* 1.3 set link bind, set param */
		is_apply_change = TRUE;
		if (dif_bind_set(hd_group, line_idx) != HD_OK) {
			GMLIB_ERROR_P("internal_err: bind set err! need to check unit param.\n");
#if (BD_DUMP_ERR_LOG == 1)
			system("cat /proc/videograph/dumplog");
#endif
			goto err_ext;
		}
		hd_group->glines[line_idx].groupidx = hd_group->groupfd;
		/* check liveview changed */
		if (dif_is_lv_changed(hd_group, line_idx)) {
			is_liveview_changed = TRUE;  /* liveview changed */
		}
	}

	/* 2. apply */
	if (is_apply_change == TRUE) {
		if (is_liveview_changed == TRUE) {
			/* apply a liveview empty hd_group, or take out liveview_line to apply */
			if (bind_lv_unbind_apply_p(hd_group) != HD_OK) {
				GMLIB_ERROR_P("internal_err: liveview unbind apply fail.\n");
				goto err_ext;
			}
		}
		GMLIB_L1_P("%s pif_apply START bd_alloc_nr(%d)\n", __func__, bd_alloc_nr);
		if (pif_apply(hd_group->groupfd) < 0)
			goto err_ext;

		GMLIB_L1_P("%s pif_apply END\n", __func__);
		if (bind_apply_finish_p(hd_group) != HD_OK)
			goto err_ext;
	}

	//print_group(hd_group);

	return HD_OK;

err_ext:
	/* err case: recover the line state */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (hd_group == NULL)
			break;

		if (hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE)
			continue;

		switch (hd_group->glines[line_idx].state) {
		case HD_LINE_STOP_ONGOING:
			hd_group->glines[line_idx].state = HD_LINE_STOP;
			break;
		case HD_LINE_START_ONGOING:
			hd_group->glines[line_idx].state = HD_LINE_START;
			break;
		default:
			break;
		}
	}
	return HD_ERR_NG;
}

HD_RESULT bd_start_list(HD_PATH_ID *path_id, UINT nr)
{
	UINT32 prev_baseid, this_devid, device_baseid;
	HD_IO in_id, out_id, in_subid, out_subid;
	HD_GROUP *group_list[BD_GROUP_LIST_NUMBER], *group = NULL;
	HD_BIND_INFO *p_end_bind, *p_bind_prev;
	HD_MEMBER end_bind_array[HD_MAX_MULTI_PATH];
	UINT32 i, j, k, list_idx;
	BOOL group_is_exist;
	UINT base_id;
	HD_QUERY_INFO end_bind_query = {0};
	HD_QUERY_INFO query = {0};
	HD_DEVICE_BASE_INFO *p_dbase = NULL;

	/* 1. set line start state */
	memset(group_list, 0x0, sizeof(group_list));
	list_idx = 0;

	pthread_mutex_lock(&hdal_mutex);
	for (i = 0; i < nr; i++) {
		this_devid = HD_GET_DEV(path_id[i]);
		out_id = HD_GET_OUT(path_id[i]);
		out_subid = BD_GET_OUT_SUBID(out_id);
		in_id = HD_GET_IN(path_id[i]);
		in_subid = BD_GET_IN_SUBID(in_id);

		if (in_id < HD_IN_BASE || out_id < HD_OUT_BASE) {
			GMLIB_ERROR_P("%s path_id(%#x) error.\n", dif_return_dev_string(path_id[i]), path_id[i]);
			goto err_ext;
		}

		device_baseid = bd_get_dev_baseid(this_devid);
		if (device_baseid == 0) {
			GMLIB_ERROR_P("ERR: deviceid(0x%x)\n", this_devid);
			goto err_ext;
		}
		p_dbase = lookup_device_base(device_baseid);
		if (p_dbase == NULL) {
			GMLIB_ERROR_P("%s path_id(%#x), p_dbase == NULL error.\n", dif_return_dev_string(path_id[i]), path_id[i]);
			goto err_ext;
		}

		if (out_subid >= p_dbase->ability.max_out) {
			continue;
		}
		if (in_subid >= p_dbase->ability.max_in) {
			continue;
		}

		GMLIB_L1_P("%s:%s_%d_IN_%d_OUT_%d i(%d) nr(%d) PORT_START\n", __func__, get_device_name(this_devid),
			   bd_get_dev_subid(this_devid), BD_GET_IN_SUBID(in_id), BD_GET_OUT_SUBID(out_id), i, nr);

		if (fill_bind_state_p(this_devid, path_id[i], HD_PORT_START) == HD_ERR_NG)
			goto err_ext;

		group_is_exist = FALSE;

		/* checking this path bindings are all start, if all start, return end_bind and end_bind_query, other return NULL */
		memset(&end_bind_array, 0, sizeof(end_bind_array));
		is_link_all_start(path_id[i], &end_bind_query, &end_bind_array[0]);

		for (k = 0; k < HD_MAX_MULTI_PATH; k++) {
			if (end_bind_array[k].p_bind == NULL) {
				continue;
			}
			p_end_bind = end_bind_array[k].p_bind;
			end_bind_query.in_subid = end_bind_array[k].in_subid;
			end_bind_query.out_subid = end_bind_array[k].out_subid;

			/* NOTE:
			 *   we need the p_end_bind->query.in_subid, and it come from is_link_all_start function
			 */
			if (hd_product_type == HD_NVR) {
				if (is_mix_multi_path(p_end_bind, end_bind_query.in_subid) == TRUE) {
					/* NOTE: is_mix_multi_path doesn't check port status
					 *  mix_group: lv + pb + transcode + disp_enc
					 */
					if ((group = get_mix_group(p_end_bind)) == NULL) {
						GMLIB_ERROR_P("internal_err: get_mix_group fail, %s_%d \n", get_bind_name(p_end_bind),
							      bd_get_dev_subid(p_end_bind->device.deviceid));
						/* get group fail */
						goto err_ext;
					}
				} else {
					/* handle others: venc, audio,... */
					if ((group = get_group(p_end_bind)) == NULL) {
						/* get group fail */
						GMLIB_ERROR_P("internal_err: get_group fail, %s_%d \n", get_bind_name(p_end_bind),
							      bd_get_dev_subid(p_end_bind->device.deviceid));
						goto err_ext;
					}
				}
			} else if (hd_product_type == HD_DVR) {
				in_subid = end_bind_query.in_subid;
				base_id = bd_get_dev_baseid(p_end_bind->device.deviceid);
				if (base_id == HD_DAL_VIDEOENC_BASE) {
					if ((p_bind_prev = get_prev_bind(p_end_bind, in_subid, NULL)) == NULL) {
						GMLIB_ERROR_P("Invalid bind link. %s_%d_%d \n", get_bind_name(p_end_bind),
							      bd_get_dev_subid(p_end_bind->device.deviceid), in_subid);
						continue;
					}
					prev_baseid = bd_get_dev_baseid(p_bind_prev->device.deviceid);
					if ((prev_baseid == HD_DAL_VIDEOPROC_BASE) &&
					    ((p_bind_prev = get_prev_bind(p_bind_prev, 0, &query)) != NULL)) {

						prev_baseid = bd_get_dev_baseid(p_bind_prev->device.deviceid);
						if (prev_baseid == HD_DAL_VIDEOOUT_BASE) {
							p_end_bind = p_bind_prev;
							end_bind_query.in_subid = query.in_subid;
							end_bind_query.out_subid = query.out_subid;
						}
					}
				}
				if ((group = get_group(p_end_bind)) == NULL) {
					/* get group fail */
					GMLIB_ERROR_P("%s internal_err, get group fail.\n", bd_get_product_type_str(hd_product_type));
					goto err_ext;
				}
			} else {
				GMLIB_ERROR_P("internal_err: get hd_product_type fail(%d)\n", hd_product_type);
			}

			/* It's end device, we need to do apply. */
			for (j = 0; j <= list_idx; j++) {
				if (group_list[j] == group) {
					group_is_exist = TRUE;
					break;
				}
			}
			if (group_is_exist != TRUE) {
				if (list_idx >= BD_GROUP_LIST_NUMBER) {
					GMLIB_ERROR_P("%s: internal_err, start_list_idx overflow!\n", get_device_name(p_end_bind->device.deviceid));
					goto err_ext;
				}
				group_list[list_idx] = group;
				list_idx++;
			}
		}
	}

	for (j = 0; j < list_idx; j++) {
		if (group_list[j] != 0) {
			if (bind_apply_p(group_list[j]) != HD_OK) {
				goto err_ext;
			}
		}
	}
	pthread_mutex_unlock(&hdal_mutex);
	return HD_OK;

err_ext:
	pthread_mutex_unlock(&hdal_mutex);
	return HD_ERR_NG;
}

HD_RESULT bd_stop_list(HD_PATH_ID *path_id, UINT nr)
{
	UINT32 this_deviceid, device_baseid;
	HD_IO in_id, out_id, in_subid, out_subid;
	HD_GROUP *group_list[BD_GROUP_LIST_NUMBER], *group;
	HD_BIND_INFO *p_bind;
	UINT32 i, j, list_idx;
	BOOL group_is_exist;
	HD_QUERY_INFO end_bind_query = {0};
	HD_DEVICE_BASE_INFO *p_dbase = NULL;

	/* 1. set line start state */
	memset(group_list, 0x0, sizeof(group_list));
	list_idx = 0;
	pthread_mutex_lock(&hdal_mutex);
	for (i = 0; i < nr; i++) {
		this_deviceid = HD_GET_DEV(path_id[i]);
		out_id = HD_GET_OUT(path_id[i]);
		out_subid = BD_GET_OUT_SUBID(out_id);
		in_id = HD_GET_IN(path_id[i]);
		in_subid = BD_GET_IN_SUBID(in_id);

		if (in_id < HD_IN_BASE || out_id < HD_OUT_BASE) {
			GMLIB_ERROR_P("%s path_id(%#x) id error.\n", dif_return_dev_string(path_id[i]), path_id[i]);
			goto err_ext;
		}

		device_baseid = bd_get_dev_baseid(this_deviceid);
		if (device_baseid == 0) {
			GMLIB_ERROR_P("ERR: deviceid(0x%x)\n", this_deviceid);
			goto err_ext;
		}
		p_dbase = lookup_device_base(device_baseid);
		if (p_dbase == NULL) {
			GMLIB_ERROR_P("%s path_id(%#x), p_dbase == NULL error.\n", dif_return_dev_string(path_id[i]), path_id[i]);
			goto err_ext;
		}

		if (out_subid >= p_dbase->ability.max_out) {
			continue;
		}
		if (in_subid >= p_dbase->ability.max_in) {
			continue;
		}

		GMLIB_L1_P("%s:%s_%d_IN_%d_OUT_%d i(%d) nr(%d) PORT_STOP\n", __func__, get_device_name(this_deviceid),
			   bd_get_dev_subid(this_deviceid), BD_GET_IN_SUBID(in_id), BD_GET_OUT_SUBID(out_id), i, nr);

		/* check this path's bindings are already stop, if TRUE return NULL , else return the end_bind */
		p_bind = is_link_already_stop(path_id[i], &end_bind_query);

		if (fill_bind_state_p(this_deviceid, path_id[i], HD_PORT_STOP) == HD_ERR_NG) {
			goto err_ext;
		}

		if (p_bind == NULL) {
			continue;
		}

		group_is_exist = FALSE;

		/*   Link: ... -> vout -> (vpe) -> venc   , vout is end-point
		 *   Link: ... -> vout                    , vout is end-point
		 *   Link: ... -> venc                    , venc is end-point
		 */

		/*
		 * 2. get already apply group
		 *    NOTE: the query.in_subid is from func is_link_already_stop
		 */
		group = lookup_group(p_bind, end_bind_query.in_subid);
		if (group == NULL) {
			GMLIB_ERROR_P("hd_group lookup fail(%s)\n", get_bind_name(p_bind));
			continue;  // no any group
		}

		/* check the pre_gline is already stop or incomplete */
		GMLIB_L1_P("  i(%d) group_endbind(%s) nr(%d)\n", i,
			   (group->p_end_bind[0] ? get_bind_name(group->p_end_bind[0]) : "---"), nr);

		/* It's end device, we need to do apply. */
		for (j = 0; j <= list_idx; j++) {
			if (group_list[j] == group) {
				group_is_exist = TRUE;
				break;
			}
		}
		if (group_is_exist != TRUE) {
			if (list_idx >= BD_GROUP_LIST_NUMBER) {
				GMLIB_ERROR_P("%s: internal_err, start_list_idx overflow!\n", get_device_name(p_bind->device.deviceid));
				goto err_ext;
			}
			group_list[list_idx] = group;
			list_idx++;
		}
	}

	for (j = 0; j < list_idx; j++) {
		if (group_list[j] != 0) {
			if (bind_apply_p(group_list[j]) != HD_OK) {
				goto err_ext;
			}
		}
	}
	pthread_mutex_unlock(&hdal_mutex);
	return HD_OK;

err_ext:
	pthread_mutex_unlock(&hdal_mutex);
	return HD_ERR_NG;
}

HD_RESULT audio_poll_list(HD_AUDIOENC_POLL_LIST *p_poll, UINT32 nr, INT32 wait_ms)
{
	HD_RESULT ret;

	ret = dif_audio_poll(p_poll, nr, wait_ms);
	return ret;
}

HD_RESULT audio_recv_list(HD_AUDIOENC_RECV_LIST *p_audioenc_bs, UINT32 nr)
{
	HD_RESULT ret;

	ret = dif_audio_recv_bs(p_audioenc_bs, nr);
	return ret;
}

HD_RESULT audio_send_list(HD_AUDIODEC_SEND_LIST *p_audiodec_bs, UINT32 nr, INT32 wait_ms)
{
	HD_RESULT ret;

	ret = dif_audio_send_bs(p_audiodec_bs, nr, wait_ms);
	return ret;
}

HD_RESULT video_poll_list(HD_VIDEOENC_POLL_LIST *p_poll, UINT32 nr, INT32 wait_ms)
{

	HD_RESULT ret;

	ret = dif_video_poll(p_poll, nr, wait_ms);
	return ret;
}

HD_RESULT video_recv_list(HD_VIDEOENC_RECV_LIST *p_videoenc_bs, UINT32 nr)
{
	HD_RESULT ret;

	ret = dif_video_recv_bs(p_videoenc_bs, nr);
	return ret;
}

HD_RESULT video_send_list(HD_VIDEODEC_SEND_LIST *p_videodec_bs, UINT32 nr, INT32 wait_ms)
{
	HD_RESULT ret;
	ret = dif_video_send_bs(p_videodec_bs, nr, wait_ms);
	return ret;
}

int check_videoout_exist(int id)
{
	UINT32 this_devid = HD_DAL_VIDEOOUT(id);
	HD_BIND_INFO *p_bind = get_bind(this_devid);
	if (p_bind == NULL)
		return 0;
	if (p_bind->out[0].port_state == HD_PORT_START)
		return 1;
	else
		return 0;
}

HD_RESULT init_dbase(HD_DAL usr_device_baseid, UINT32 max_device_subid, UINT32 max_in, UINT max_out)
{
	HD_DEVICE_BASE_INFO *p_dbase = NULL;
	UINT32 idx;

	/* 1. lookup a free device base */
	for (idx = 0; idx < HD_MAX_DEVICE_BASEID; idx ++) {
		if (all_dbase[idx].in_use == FALSE) {
			p_dbase = &all_dbase[idx];
			break;
		}
	}
	if (p_dbase == NULL) {
		GMLIB_ERROR_P("usr_device_baseid(%d): internal_err, new device fail!\n", usr_device_baseid);
		goto err_ext;
	}
	/* 2. init device base */
	p_dbase->in_use = TRUE;
	p_dbase->device_baseid = bd_get_dev_baseid(usr_device_baseid);
	p_dbase->ability.max_device_nr = max_device_subid;
	p_dbase->ability.max_in = max_in;
	p_dbase->ability.max_out = max_out;

	/* 3. init bind base */
	if (init_bind(p_dbase) == HD_ERR_NG)
		goto err_ext;

	return HD_OK;

err_ext:
	return HD_ERR_NG;
}

VOID uninit_dbase(HD_DAL usr_device_baseid)
{
	HD_DEVICE_BASE_INFO *p_dbase = NULL;
	HD_DAL device_baseid;

	/* err checking */
	device_baseid = bd_get_dev_baseid(usr_device_baseid);
	if (device_baseid == 0) {
		GMLIB_ERROR_P("ERR: usr_device_baseid(0x%x)\n", usr_device_baseid);
		goto ext;
	}

	/* 1. lookup a device base */
	p_dbase = lookup_device_base(device_baseid);
	if (p_dbase == NULL) {
		GMLIB_ERROR_P("usr_dbase_id(%#x): internal_err, lookup device fail!\n", usr_device_baseid);
		goto ext;
	}
	/* 2. uninit bind */
	uninit_bind(p_dbase);

	/* 3. set device base default value */
	memset(p_dbase, 0x0, sizeof(HD_DEVICE_BASE_INFO));
ext:
	return;
}

HD_RESULT bd_device_init(HD_DAL usr_device_baseid, UINT32 max_did, UINT32 max_device_in, UINT max_device_out)
{
	HD_RESULT hd_ret = HD_OK;
	INT32 dbase_idx;
	CHAR *device_name;

	if ((dbase_idx = is_device_base_exist_p(usr_device_baseid)) >= 0) {
		device_name = get_device_name(usr_device_baseid);
		GMLIB_ERROR_P("usr_device_baseid(%x): already init for (%s:%d).\n",
			      usr_device_baseid, device_name, dbase_idx);
		hd_ret = HD_ERR_INIT;
		goto err_ext;

	}
	if (init_dbase(usr_device_baseid, max_did, max_device_in, max_device_out) == HD_ERR_NG) {
		GMLIB_ERROR_P("usr_device_baseid(%d): internal_err, device fail!\n", usr_device_baseid);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT bd_device_uninit(HD_DAL usr_device_baseid)
{

	uninit_dbase(usr_device_baseid);
	return HD_OK;
}

/**
 * @brief initializes library, and open resource.
 */
HD_RESULT bd_dal_init(void)
{
	UINT32 mask = -1;

	memset(&bd_transcode_path, 0, sizeof(bd_transcode_path));
	if (pif_init() < 0) {
		printf("pif_init failed!");
		goto err_ext;
	}
	gmlib_proc_init();
	memset(bgroup, 0, sizeof(bgroup));

	/* enable error message print */
	hd_debug_set(HD_DEBUG_PARAM_ERR_MASK, &mask);

	if (pif_get_cap_channel_number() > 0) {
		hd_product_type = HD_DVR;
	} else {
		hd_product_type = HD_DVR; // for no different for DVR/NVR test~
		//hd_product_type = HD_NVR;
	}

	if (pthread_mutex_init(&hdal_mutex, NULL)) {
		printf("bd_dal_init: init hdal_mutex failed!");
		goto err_ext;
	}
	utl_init();

	return HD_OK;
err_ext:
	return HD_ERR_NG;
}


/**
 * @brief de-initializes library, and release resource.
 */
HD_RESULT bd_dal_uninit(void)
{
	pthread_mutex_destroy(&hdal_mutex);
	if (pif_release() < 0) {
		goto err_ext;
	}
	gmlib_proc_close();
	utl_uninit();

	return HD_OK;

err_ext:
	return HD_ERR_NG;
}

