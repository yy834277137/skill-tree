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
#include <assert.h>
#include "hd_type.h"
#include "hd_debug.h"
#include "hd_logger.h"
#include "hdal.h"
#include "audiocap.h"
#include "audioenc.h"
#include "audiodec.h"
#include "audioout.h"
#include "videoout.h"
#include "videoenc.h"
#include "videodec.h"
#include "videoprocess.h"
#include "gfx.h"
#include "videocap.h"
#include "bind.h"
#include "vpd_ioctl.h"
#include "trig_ioctl.h"
#include "pif.h"
#include "pif_ioctl.h"
#include "utl.h"
#include "dif.h"
#include "cif_common.h"
#include "vg_common.h"
#include "videocap_convert.h"
#include "videoproc_convert.h"
#include "videoout_convert.h"
#include "videoenc_convert.h"
#include "videodec_convert.h"
#include "audiocap_convert.h"
#include "audioout_convert.h"
#include "audioenc_convert.h"
#include "audiodec_convert.h"
#if SEND_DEC_BLACK_BS
#include "constant_data.h"
#endif
#include "kflow_videodec.h"
#include "kflow_audioio.h"

extern UINT16 osg_select[MAX_VDOENC_DEV][HD_VIDEOENC_MAX_OUT] ;
extern pthread_mutex_t hdal_mutex;
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern HD_GROUP bgroup[BD_MAX_BIND_GROUP];
extern vpd_sys_info_t platform_sys_Info;
extern vpd_sys_spec_info_t platform_sys_spec_Info;
extern unsigned int gmlib_dbg_level;
extern unsigned int gmlib_dbg_mode;
extern HD_LIVEVIEW_INFO lv_collect_info[VPD_MAX_CAPTURE_CHANNEL_NUM][VPD_MAX_CAPTURE_PATH_NUM];
extern KFLOW_VIDEODEC_HW_SPEC_INFO vdodec_hw_spec_info;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
char *hdal_get_log_addr(void);
int hdal_get_log_buffer_len(void);

int hd_videocap_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len);
int hd_videoproc_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len);
int hd_videoout_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len);
int hd_videoenc_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len);
int hd_videodec_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len);
int hd_audiocap_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len);
int hd_audiodec_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len);
int hd_audioenc_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len);
int hd_audioout_show_to_proc_status_p(CHAR *p_buf, unsigned int buf_len);

int hd_videocap_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid);
int hd_videoproc_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid);
int hd_videoout_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid);
int hd_videoenc_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid);
int hd_videodec_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid);
int hd_audiocap_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid);
int hd_audiodec_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid);
int hd_audioenc_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid);
int hd_audioout_devid_show_to_proc_status_p(CHAR *p_buf, HD_DAL deviceid);

struct list_head *get_next_bind_listhead(HD_BIND_INFO *p_bind, UINT32 out_subid);

HD_RESULT _hd_common_mem_cache_sync_dma_to_device(void *virt_addr, unsigned int size);

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/
INT32 dif_alloc_nr = 0;
#define DIF_MALLOC(a)       ({dif_alloc_nr++; PIF_MALLOC((a));})
#define DIF_CALLOC(a, b)    ({dif_alloc_nr++; PIF_CALLOC((a), (b));})
#define DIF_FREE(a)         {dif_alloc_nr--; PIF_FREE((a));}

#define	MAX_LIST_NUM			128

//#define DEFAULT_DISP_FMT	HD_VIDEO_PXLFMT_YUV422_ONE 	//HD_VIDEO_PXLFMT_YUV420, HD_VIDEO_PXLFMT_YUV422_ONE

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

int dif_get_bs_ratio(HD_VIDEO_CODEC codec_type, int qp)
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

/* where
 *		0: print to console
 *		1: pirnt to buffer
 */

INT dif_dbg_print(INT where, CHAR *buf, const CHAR *fmt, ...)
{
	int len = 0;
	va_list ap;
	char line[512];

	va_start(ap, fmt);
	len = vsnprintf(line, sizeof(line), fmt, ap);
	va_end(ap);

	if (where == 1) {
		memcpy(buf, line, len);
		buf[len + 1] = 0;
	} else {
		printf("%s", line);
	}
	return len;
}

CHAR *dif_indent_string[HD_MAX_GRAPH_RECURSIVE_LEVEL] = {
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

UINT32 dif_print_next_lh_bind(INT where, CHAR *buf, struct list_head *listhead, UINT32 layer)
{
	struct list_head *dst_listhead;
	HD_DEV_NODE *dev_node;
	HD_BIND_INFO *p_next_bind = NULL;
	INT32 out_nr, i, out_subid;
	INT len = 0;
	char state_str[10];

	if (layer >= HD_MAX_GRAPH_RECURSIVE_LEVEL) {
		goto ext;
	}
	list_for_each_entry(dev_node, listhead, list) {
		p_next_bind = (HD_BIND_INFO *)dev_node->p_bind_info;

		if (is_end_device_p(p_next_bind->device.deviceid) == TRUE) {


			switch (get_bind_in_out_way_p(p_next_bind)) {
			case HD_BIND_N_TO_1_WAY:
				out_subid = 0;
				break;
			case HD_BIND_N_TO_N_WAY:
				out_subid = dev_node->in_subid;
				break;
			case HD_BIND_1_TO_N_WAY:
			default:
				out_subid = -1;
				break;
			}

			strcpy(state_str, "STOP");
			if (p_next_bind->in[dev_node->in_subid].port_state == HD_PORT_START) {
				if (out_subid != -1) {
					if (p_next_bind->out[out_subid].port_state == HD_PORT_START) {
						strcpy(state_str, "START");
					}
				} else {
					strcpy(state_str, "----");
				}
			}


			len += dif_dbg_print(where, buf + len, "%s %s_%d_CHIP_%d_IN_%d (%#x:%s)\n", dif_indent_string[layer],
					     get_bind_name(p_next_bind), bd_get_dev_subid(p_next_bind->device.deviceid),
					     bd_get_chip_by_bind(p_next_bind), dev_node->in_subid,
					     dif_get_path_id_by_binding(p_next_bind, dev_node->in_subid, out_subid), state_str);
			out_nr = get_bind_out_nr(p_next_bind);
			for (i = 0; i < out_nr; i++) {
				dst_listhead = &p_next_bind->out[i].bnode->dst_listhead;
				if (list_empty(dst_listhead))
					continue;
				len += dif_print_next_lh_bind(where, buf + len, dst_listhead, layer + 1);
			}
		} else {
			out_nr = get_bind_out_nr(p_next_bind);
			for (i = 0; i < out_nr; i++) {
				dst_listhead = &p_next_bind->out[i].bnode->dst_listhead;
				if (list_empty(dst_listhead))
					continue;

				strcpy(state_str, "STOP");
				if (p_next_bind->in[dev_node->in_subid].port_state == HD_PORT_START) {
					if (p_next_bind->out[i].port_state == HD_PORT_START) {
						strcpy(state_str, "START");
					}
				}

				len += dif_dbg_print(where, buf + len, "%s %s_%d_CHIP_%d_IN_%d_OUT_%d (%#x:%s)\n", dif_indent_string[layer],
						     get_bind_name(p_next_bind), bd_get_dev_subid(p_next_bind->device.deviceid),
						     bd_get_chip_by_bind(p_next_bind), dev_node->in_subid, i,
						     dif_get_path_id_by_binding(p_next_bind, dev_node->in_subid, i), state_str);

				len += dif_print_next_lh_bind(where, buf + len, dst_listhead, layer + 1);
			}
		}
	}

ext:
	return len;
}

INT dif_print_bind(INT where, CHAR *buf, HD_BIND_INFO *p_bind, UINT32 out_subid)
{
	UINT32 in_subid = 0;
	struct list_head *listhead;
	INT len = 0;
	char state_name[10] = "STOP";

	listhead = get_next_bind_listhead(p_bind, out_subid);

	if (listhead == NULL || list_empty(listhead))
		goto ext;

	if (p_bind->out[out_subid].port_state == HD_PORT_START)
		strcpy(state_name, "START");

	switch (get_bind_in_out_way_p(p_bind)) {
	case HD_BIND_N_TO_N_WAY:
		in_subid = out_subid;
		break;
	case HD_BIND_1_TO_N_WAY:
		in_subid = 0;
		break;
	case HD_BIND_N_TO_1_WAY:
	default:
		in_subid = -1;
	}

	len += dif_dbg_print(where, buf + len, "%s%s_%d_CHIP_%d_OUT_%d (%#x:%s)\n", dif_indent_string[0],
			     get_bind_name(p_bind), bd_get_dev_subid(p_bind->device.deviceid), bd_get_chip_by_bind(p_bind),
			     out_subid, dif_get_path_id_by_binding(p_bind, in_subid, out_subid), state_name);

	len += dif_print_next_lh_bind(where, buf + len, listhead, 1);

ext:
	return len;
}

static INT dif_hdal_graph_p(CHAR *buf)
{
	HD_DAL deviceid;
	HD_BIND_INFO *p_bind;
	INT32 out_nr, i;
	INT len = 0, prev_len;

	len += dif_dbg_print(1, buf + len, "-------------------------------------\n");

	/* videocap */
	for (deviceid = HD_DAL_VIDEOCAP_BASE; deviceid < HD_DAL_VIDEOCAP_MAX; deviceid++) {
		p_bind = get_bind(deviceid);
		if (p_bind == NULL)
			continue;
		if (is_bind_valid(p_bind) != HD_OK)
			continue;
		out_nr = get_bind_out_nr(p_bind);
		prev_len = len;
		for (i = 0; i < out_nr; i++) {
			len += dif_print_bind(1, buf + len, p_bind, i);
		}
		if (prev_len != len)
			len += dif_dbg_print(1, buf + len, "-------------------------------------\n");
	}

	/* videodec */
	for (deviceid = HD_DAL_VIDEODEC_BASE; deviceid < HD_DAL_VIDEODEC_MAX; deviceid++) {
		p_bind = get_bind(deviceid);
		if (p_bind == NULL)
			continue;
		if (is_bind_valid(p_bind) != HD_OK)
			continue;
		out_nr = get_bind_out_nr(p_bind);
		prev_len = len;
		for (i = 0; i < out_nr; i++) {
			len += dif_print_bind(1, buf + len, p_bind, i);
		}
		if (prev_len != len)
			len += dif_dbg_print(1, buf + len, "-------------------------------------\n");
	}

	/* audiocap */
	for (deviceid = HD_DAL_AUDIOCAP_BASE; deviceid < HD_DAL_AUDIOCAP_MAX; deviceid++) {
		p_bind = get_bind(deviceid);
		if (p_bind == NULL)
			continue;
		if (is_bind_valid(p_bind) != HD_OK)
			continue;
		out_nr = get_bind_out_nr(p_bind);
		prev_len = len;
		for (i = 0; i < out_nr; i++) {
			len += dif_print_bind(1, buf + len, p_bind, i);
		}
		if (prev_len != len)
			len += dif_dbg_print(1, buf + len, "-------------------------------------\n");
	}

	/* audioenc */
	for (deviceid = HD_DAL_AUDIODEC_BASE; deviceid < HD_DAL_AUDIODEC_MAX; deviceid++) {
		p_bind = get_bind(deviceid);
		if (p_bind == NULL)
			continue;
		if (is_bind_valid(p_bind) != HD_OK)
			continue;
		out_nr = get_bind_out_nr(p_bind);
		prev_len = len;
		for (i = 0; i < out_nr; i++) {
			len += dif_print_bind(1, buf + len, p_bind, i);
		}
		if (prev_len != len)
			len += dif_dbg_print(1, buf + len, "-------------------------------------\n");
	}

	return len;
}

VOID dif_setting_log(unsigned char *msg_cmd)
{
	unsigned int len = 0, param = 0, buf_len;
	char *msg;
	char cmd_type[32], cmd_param[32];

	msg = hdal_get_log_addr();
	buf_len = hdal_get_log_buffer_len();
	len += dif_dbg_print(1, msg + len, "hdal command: %s", msg_cmd);
	memset(cmd_type, 0, sizeof(cmd_type));
	memset(cmd_param, 0, sizeof(cmd_param));

	sscanf((char *) msg_cmd, "%s %s\n", cmd_type, cmd_param);

	if (strcasecmp(cmd_type, "videocap") == 0) {
		if (strcasecmp(cmd_param, "all") == 0) {
			len += hd_videocap_show_to_proc_status_p(msg + len, buf_len);
		} else {
			param = atoi(cmd_param);
			len += hd_videocap_devid_show_to_proc_status_p(msg + len, param + HD_DAL_VIDEOCAP_BASE);
		}
	} else if (strcasecmp(cmd_type, "videoproc") == 0) {
		if (strcasecmp(cmd_param, "all") == 0) {
			len += hd_videoproc_show_to_proc_status_p(msg + len, buf_len);
		} else {
			param = atoi(cmd_param);
			len += hd_videoproc_devid_show_to_proc_status_p(msg + len, param + HD_DAL_VIDEOPROC_BASE);
		}
	} else if (strcasecmp(cmd_type, "videoout") == 0) {
		if (strcasecmp(cmd_param, "all") == 0) {
			len += hd_videoout_show_to_proc_status_p(msg + len, buf_len);
		} else {
			param = atoi(cmd_param);
			len += hd_videoout_devid_show_to_proc_status_p(msg + len, param + HD_DAL_VIDEOOUT_BASE);
		}
	} else if (strcasecmp(cmd_type, "videodec") == 0) {
		if (strcasecmp(cmd_param, "all") == 0) {
			len += hd_videodec_show_to_proc_status_p(msg + len, buf_len);
		} else {
			param = atoi(cmd_param);
			len += hd_videodec_devid_show_to_proc_status_p(msg + len, param + HD_DAL_VIDEODEC_BASE);
		}
	} else if (strcasecmp(cmd_type, "videoenc") == 0) {
		if (strcasecmp(cmd_param, "all") == 0) {
			len += hd_videoenc_show_to_proc_status_p(msg + len, buf_len);
		} else {
			param = atoi(cmd_param);
			len += hd_videoenc_devid_show_to_proc_status_p(msg + len, param + HD_DAL_VIDEOENC_BASE);
		}
	} else if (strcasecmp(cmd_type, "audiocap") == 0) {
		if (strcasecmp(cmd_param, "all") == 0) {
			len += hd_audiocap_show_to_proc_status_p(msg + len, buf_len);
		} else {
			param = atoi(cmd_param);
			len += hd_audiocap_devid_show_to_proc_status_p(msg + len, param + HD_DAL_AUDIOCAP_BASE);
		}
	} else if (strcasecmp(cmd_type, "audioenc") == 0) {
		if (strcasecmp(cmd_param, "all") == 0) {
			len += hd_audioenc_show_to_proc_status_p(msg + len, buf_len);
		} else {
			param = atoi(cmd_param);
			len += hd_audioenc_devid_show_to_proc_status_p(msg + len, param + HD_DAL_AUDIOENC_BASE);
		}
	} else if (strcasecmp(cmd_type, "audiodec") == 0) {
		if (strcasecmp(cmd_param, "all") == 0) {
			len += hd_audiodec_show_to_proc_status_p(msg + len, buf_len);
		} else {
			param = atoi(cmd_param);
			len += hd_audiodec_devid_show_to_proc_status_p(msg + len, param + HD_DAL_AUDIODEC_BASE);
		}
	} else if (strcasecmp(cmd_type, "audioout") == 0) {
		if (strcasecmp(cmd_param, "all") == 0) {
			len += hd_audioout_show_to_proc_status_p(msg + len, buf_len);
		} else {
			param = atoi(cmd_param);
			len += hd_audioout_devid_show_to_proc_status_p(msg + len, param + HD_DAL_AUDIOOUT_BASE);
		}
	} else if (strcasecmp(cmd_type, "graph") == 0) {
		len += dif_hdal_graph_p(msg + len);
	}
	dif_dbg_print(1, msg + len, "=== finished ===\n");

	return;
}

int get_sample_rate_from_HD_AUDIO_SR_p(HD_AUDIO_SR sample_rate)
{
	return sample_rate;
}

int get_sample_size_from_HD_AUDIO_BIT_WIDTH_p(HD_AUDIO_BIT_WIDTH sample_bit)
{
	return sample_bit;
}

int get_channel_type_from_HD_AUDIO_SOUND_MODE_p(HD_AUDIO_SOUND_MODE mode)
{
	if (mode == HD_AUDIO_SOUND_MODE_MONO) {
		return 1;
	} else if (mode == HD_AUDIO_SOUND_MODE_STEREO) {
		return 2;
	} else {
		GMLIB_ERROR_P("HD_AUDIO_SOUND_MODE mode(%d) is not supported.\n", mode);
		return 0;
	}
}

int get_audio_encode_type_from_HD_AUDIO_CODEC_p(HD_AUDIO_CODEC codec_type)
{
	switch (codec_type) {
	case HD_AUDIO_CODEC_PCM:
		return GM_PCM;
	case HD_AUDIO_CODEC_AAC:
		return GM_AAC;
	case HD_AUDIO_CODEC_ADPCM:
		return GM_ADPCM;
	case HD_AUDIO_CODEC_ULAW:
		return GM_G711_ALAW;
	case HD_AUDIO_CODEC_ALAW:
		return GM_G711_ULAW;
	default:
		GMLIB_ERROR_P("HD_AUDIO_CODEC type(%d) is not supported.\n", codec_type);
		return 0;
	}
}

BOOL dif_is_lv(HD_GROUP *hd_group, UINT32 line_idx)
{
	INT lcd_vch, lcd_path, vcap_vch, vcap_path;

	if (dif_get_lcd_ch_path(hd_group, line_idx, &lcd_vch, &lcd_path) != HD_OK) {  /* no lcd */
		goto ext;
	}
	if (dif_get_vcap_ch_path(hd_group, line_idx, &vcap_vch, &vcap_path) != HD_OK) {  /* no vcap */
		goto ext;
	}

	return TRUE;

ext:
	return FALSE;
}

HD_LIVEVIEW_INFO *dif_get_lv_collect_info(UINT32 cap_dev_subid, UINT32 out_subid)
{
	if (cap_dev_subid >= VPD_MAX_CAPTURE_CHANNEL_NUM || out_subid >= VPD_MAX_CAPTURE_PATH_NUM) {
		GMLIB_ERROR_P("Cap err! cap_dev_subid(%d) out_subid(%d).\n", cap_dev_subid, out_subid);
		return NULL; // err!
	}

	return &lv_collect_info[cap_dev_subid][out_subid];
}

BOOL dif_is_lv_changed(HD_GROUP *hd_group, UINT32 line_idx)
{
	BOOL ret = FALSE;
	INT member_idx;
	HD_MEMBER *p_member;
	HD_VIDEOCAP_OUT cap_out;
	HD_LIVEVIEW_INFO *lv_info;
	HD_LINE_STATE pre_line_state, cur_line_state;

	if (hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto ext;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto ext;
	}

	if (dif_is_lv(hd_group, line_idx) == FALSE) {  /* not liveview */
		goto ext;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (hd_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}

		p_member = &(hd_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) != HD_DAL_VIDEOCAP_BASE) {
			continue;
		}

		if (dif_get_vcap_out(hd_group, line_idx, &cap_out) != HD_OK) {
			GMLIB_ERROR_P("vcap out param is not set! line_idx(%d) devicedid(%d) out_subid(%d)\n",
				      line_idx, p_member->p_bind->device.deviceid, p_member->out_subid);
			goto ext;
		}
		lv_info = dif_get_lv_collect_info(bd_get_dev_subid(p_member->p_bind->device.deviceid), p_member->out_subid);
		if (lv_info == NULL) {
			GMLIB_ERROR_P("lv_collect err! cap_dev_subid(%d) out_subid(%d).\n",
				      p_member->p_bind->device.deviceid, p_member->out_subid);
			goto ext;
		}

		/* backup previous liveview info */
		memcpy(&lv_info->pre, &lv_info->cur, sizeof(HD_LV_CONFIG));

		/* keep current liveview info */
		lv_info->cur.cap_dim.w = cap_out.dim.w;
		lv_info->cur.cap_dim.h = cap_out.dim.h;
		pre_line_state = hd_group->prev_glines[line_idx].state;
		cur_line_state = hd_group->glines[line_idx].state;

		GMLIB_L1_P("%s: %d pre_line_state(%d)(%dx%d) cur_line_state(%d)(%dx%d)\n", __func__, line_idx,
			   pre_line_state, lv_info->pre.cap_dim.w, lv_info->pre.cap_dim.h,
			   cur_line_state, lv_info->cur.cap_dim.w, lv_info->cur.cap_dim.h);

		if ((pre_line_state == HD_LINE_START || pre_line_state == HD_LINE_START_ONGOING) &&
		    (cur_line_state == HD_LINE_START || cur_line_state == HD_LINE_START_ONGOING)) {
			/* start(or onging) --> start(or onging),  check liveview changed */

			if (lv_info->cur.cap_dim.w != lv_info->pre.cap_dim.w ||
			    lv_info->cur.cap_dim.h != lv_info->pre.cap_dim.h) {

				ret = TRUE; // liveview changed
				goto ext;
			}
		}
	}

ext:

	return ret;
}

HD_FRAME_TYPE get_hdal_frame_type(unsigned int ref_frame)
{
	if (ref_frame == 0)           /* value: PROP_SLICE_TYPE_P = 0 */
		return HD_FRAME_TYPE_P;
	else if (ref_frame == 2)      /* value: PROP_SLICE_TYPE_I = 2 */
		return HD_FRAME_TYPE_I;
	else                          /* value: PROP_SLICE_TYPE_KP = 4 */
		return HD_FRAME_TYPE_KP;
}

INT get_gm_win_layer_from_hdal_layer(unsigned int value)
{
	if (value == HD_LAYER1)
		return 0;
	else if (value == HD_LAYER2)
		return 1;
	else if (value == 2 /*VENDOR_WIN_LAYER3*/)
		return 2;
	else if (value == 3 /*VENDOR_WIN_LAYER4*/)
		return 3;
	else if (value == 4 /*VENDOR_WIN_LAYER5*/)
		return 4;
	else if (value == 5 /*VENDOR_WIN_LAYER6*/)
		return 5;
	else if (value == 6 /*VENDOR_WIN_LAYER7*/)
		return 6;
	else if (value == 7 /*VENDOR_WIN_LAYER8*/)
		return 7;
	else
		return 0;
}


HD_SVC_LAYER_TYPE get_hdal_svc_layer_type(unsigned int svc_layer_type)
{
	return svc_layer_type;
}

HD_GROUP_LINE *get_group_line_p(HD_BIND_INFO *p_bind, UINT32 *p_in_subid, UINT32 *p_out_subid)
{
	UINT32 i, line_idx, member_idx;
	HD_GROUP_LINE *one_line = NULL;

	for (i = 0; i < BD_MAX_BIND_GROUP; i++) {
		if (bgroup[i].in_use != TRUE) {
			continue;
		}
		for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
			if (bgroup[i].glines[line_idx].state == HD_LINE_INCOMPLETE ||
			    bgroup[i].glines[line_idx].state == HD_LINE_STOP)  {
				continue;
			}

			for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
				if (bgroup[i].glines[line_idx].member[member_idx].p_bind == p_bind) {
					if (p_in_subid && bgroup[i].glines[line_idx].member[member_idx].in_subid == *p_in_subid) {
						one_line = &bgroup[i].glines[line_idx];
						goto ext;
					} else if (p_out_subid && bgroup[i].glines[line_idx].member[member_idx].out_subid == *p_out_subid) {
						one_line = &bgroup[i].glines[line_idx];
						goto ext;
					}
				}
			}
		}
	}

ext:
	return one_line;
}


INT dif_is_di_enable(HD_GROUP *p_group, UINT32 line_idx, VDOPROC_INTL_PARAM **pp_vproc_param)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VPROC_PARAM vproc;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}

	if (pp_vproc_param) {
		*pp_vproc_param = NULL;
	}
	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			vproc.param = videoproc_get_param_p(p_member->p_bind->device.deviceid, p_member->out_subid);
			if (vproc.param && (vproc.param->p_di_enable != NULL) && (*(vproc.param->p_di_enable))) {
				if (pp_vproc_param) {
					*pp_vproc_param = vproc.param;
				}
				return TRUE;
			}
		}
	}

err_exit:
	return FALSE;
}

/*
 * type                                    src_bg(w,h)  src(x,y,w,h)  dst_bg(w,h)  dst(x,y,w,h) rt_src_bg(w,h)
 * HD_VIDEO_PXLFMT_YUV422_ONE (YUV422,UYVY)  ( 8, 2)   ( 2, 2, 8, 2)    ( 8, 2)   ( 2, 2, 2, 2)   ( 2, 8)
 * HD_VIDEO_PXLFMT_YUV420                    (16, 2)   ( 2, 2, 8, 2)    (16, 2)   ( 4, 2, 4, 2)   ( 2,16)
 * HD_VIDEO_PXLFMT_YUV420_W8                 (16, 2)   ( 2, 2, 8, 2)    (16, 2)   ( 4, 2, 4, 2)   ( 2,16)
 * HD_VIDEO_PXLFMT_YUV422_MB                 (16,16)   ( 2, 2, 8, 2)    (16,16)   (16,16,16,16)   (16,16)
 * HD_VIDEO_PXLFMT_YUV420_MB                 (16,16)   ( 2, 2, 8, 2)    (16,16)   (16,16,16,16)   (16,16)
 * HD_VIDEO_PXLFMT_YUV420_MB2 (YUV420_16x2)  (16, 4)   ( 2, 2, 8, 2)    ( 1, 1)   ( 1, 1, 1, 1)   ( 4,16)
 * HD_VIDEO_PXLFMT_YUV422_NVX3 (YUV422_SCE)  (32, 2)   ( 2, 2, 8, 2)    (32, 2)   (32, 2,32, 2)   ( 2,32)
 * HD_VIDEO_PXLFMT_YUV420_NVX3 (YUV420_SCE)  (32, 2)   ( 2, 2, 8, 2)    (32, 2)   (32, 2,32, 2)   (16,64)
 * HD_VIDEO_PXLFMT_ARGB1555                  ( 1, 1)   ( 1, 1, 1, 1)    (16, 1)   (16, 1,16, 1)   (16,64)
 */
HD_RESULT dif_get_align_up_dim(HD_DIM in_dim, HD_VIDEO_PXLFMT in_pxlfmt, HD_DIM *p_out_dim)
{
	if (p_out_dim == NULL) {
		GMLIB_ERROR_P("NULL p_out_dim\n");
		goto err_exit;
	}

	switch (in_pxlfmt) {
	case HD_VIDEO_PXLFMT_YUV422_ONE:
		p_out_dim->w = ALIGN8_UP(in_dim.w);
		p_out_dim->h = ALIGN2_UP(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_YUV420:
	case HD_VIDEO_PXLFMT_YUV420_W8:
		p_out_dim->w = ALIGN16_UP(in_dim.w);
		p_out_dim->h = ALIGN2_UP(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB:
		p_out_dim->w = ALIGN16_UP(in_dim.w);
		p_out_dim->h = ALIGN16_UP(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB2:
		p_out_dim->w = ALIGN16_UP(in_dim.w);
		p_out_dim->h = ALIGN4_UP(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB3:
		p_out_dim->w = ALIGN64_UP(in_dim.w);
		p_out_dim->h = ALIGN16_UP(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_YUV422_NVX3:
	case HD_VIDEO_PXLFMT_YUV420_NVX3:
		p_out_dim->w = ALIGN32_UP(in_dim.w);
		p_out_dim->h = ALIGN2_UP(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_NVX4:
		p_out_dim->w = ALIGN128_UP(in_dim.w);
		p_out_dim->h = ALIGN16_UP(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_ARGB1555:
		p_out_dim->w = in_dim.w;
		p_out_dim->h = in_dim.h;
		break;
	default:
		GMLIB_ERROR_P("Not support for input pxlfmt(%#x)\n", in_pxlfmt);
		goto err_exit;
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_align_down_dim(HD_DIM in_dim, HD_VIDEO_PXLFMT in_pxlfmt, HD_DIM *p_out_dim)
{
	if (p_out_dim == NULL) {
		GMLIB_ERROR_P("NULL p_out_dim\n");
		goto err_exit;
	}

	switch (in_pxlfmt) {
	case HD_VIDEO_PXLFMT_YUV422_ONE:
		p_out_dim->w = ALIGN8_DOWN(in_dim.w);
		p_out_dim->h = ALIGN2_DOWN(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_YUV420:
	case HD_VIDEO_PXLFMT_YUV420_W8:
		p_out_dim->w = ALIGN16_DOWN(in_dim.w);
		p_out_dim->h = ALIGN2_DOWN(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB:
		p_out_dim->w = ALIGN16_DOWN(in_dim.w);
		p_out_dim->h = ALIGN16_DOWN(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_YUV420_MB2:
		p_out_dim->w = ALIGN16_DOWN(in_dim.w);
		p_out_dim->h = ALIGN4_DOWN(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_YUV422_NVX3:
	case HD_VIDEO_PXLFMT_YUV420_NVX3:
		p_out_dim->w = ALIGN32_DOWN(in_dim.w);
		p_out_dim->h = ALIGN2_DOWN(in_dim.h);
		break;
	case HD_VIDEO_PXLFMT_ARGB1555:
		p_out_dim->w = in_dim.w;
		p_out_dim->h = in_dim.h;
		break;
	default:
		GMLIB_ERROR_P("Not support for input pxlfmt(%#x)\n", in_pxlfmt);
		goto err_exit;
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

/**
	Get pxlfmt by codec type

	@param codec_type: codec type.
	@param *p_fmt: output pixel format.
	@param yuv_idx: yuv type index. (0 for main_yuv, 1 for sub_yuv)
*/
HD_RESULT dif_get_pxlfmt_by_codec_type(HD_VIDEO_CODEC codec_type, HD_VIDEO_PXLFMT *p_fmt, UINT32 yuv_idx)
{
	if (p_fmt == NULL) {
		GMLIB_ERROR_P("NULL p_fmt\n");
		goto err_exit;
	}

#if 0
	if (codec_type == HD_CODEC_TYPE_H265) {
		*p_fmt = (yuv_idx == 0) ? HD_VIDEO_PXLFMT_YUV420_MB2 : HD_VIDEO_PXLFMT_YUV420;
	} else if (codec_type == HD_CODEC_TYPE_H264) {
		*p_fmt = (yuv_idx == 0) ? HD_VIDEO_PXLFMT_YUV420 : HD_VIDEO_PXLFMT_YUV420;
	} else if (codec_type == HD_CODEC_TYPE_JPEG) {
		*p_fmt = (yuv_idx == 0) ? HD_VIDEO_PXLFMT_YUV420_W8 : HD_VIDEO_PXLFMT_YUV420_W8;
	} else {
		GMLIB_ERROR_P("Unknown codec_type(0x%x) yuv_idx(%u)\n", codec_type, yuv_idx);
		goto err_exit;
	}
#else
	if (codec_type == HD_CODEC_TYPE_H265 || codec_type == HD_CODEC_TYPE_H264) {
		if (yuv_idx == 0) {
			*p_fmt = HD_VIDEO_PXLFMT_YUV420_NVX4; // first write: LLC_8x4_blockbase
		} else {
			if (0) { // [636 TODO]
				*p_fmt = HD_VIDEO_PXLFMT_YUV420; // extra write: YUV420(true mode)
			} else {
				*p_fmt = HD_VIDEO_PXLFMT_YUV420_NVX3; // extra write: YUV_420SCE
			}
		}
	} else if (codec_type == HD_CODEC_TYPE_JPEG) {
		*p_fmt = (yuv_idx == 0) ? HD_VIDEO_PXLFMT_YUV420 : HD_VIDEO_PXLFMT_YUV420;
	} else {
		GMLIB_ERROR_P("Unknown codec_type(0x%x) yuv_idx(%u)\n", codec_type, yuv_idx);
		goto err_exit;
	}
#endif

	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_venc_codec_type(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEO_CODEC *p_codec_type)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VENC_PARAM venc;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_codec_type == NULL) {
		GMLIB_ERROR_P("NULL p_codec_type\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
			venc.param = videoenc_get_param_p(p_member->p_bind->device.deviceid);
			if (venc.param && venc.param->port[p_member->in_subid].p_enc_out_param != NULL) {
				*p_codec_type = venc.param->port[p_member->in_subid].p_enc_out_param->codec_type;
				return HD_OK;
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_venc_rc(HD_GROUP *p_group, UINT32 line_idx, HD_H26XENC_RATE_CONTROL *p_enc_rc)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VENC_PARAM venc;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
			venc.param = videoenc_get_param_p(p_member->p_bind->device.deviceid);
			if (venc.param && venc.param->port[p_member->in_subid].p_enc_rc != NULL) {
				memcpy(p_enc_rc, venc.param->port[p_member->in_subid].p_enc_rc, sizeof(HD_H26XENC_RATE_CONTROL));
				return HD_OK;
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_venc_dim_fmt(HD_GROUP *p_group, UINT32 line_idx, HD_DIM *p_enc_dim, HD_VIDEO_PXLFMT *p_enc_fmt)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VENC_PARAM venc;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
			venc.param = videoenc_get_param_p(p_member->p_bind->device.deviceid);
			if (venc.param && venc.param->port[p_member->in_subid].p_enc_in_param != NULL) {
				p_enc_dim->w = venc.param->port[p_member->in_subid].p_enc_in_param->dim.w;
				p_enc_dim->h = venc.param->port[p_member->in_subid].p_enc_in_param->dim.h;
				*p_enc_fmt = venc.param->port[p_member->in_subid].p_enc_in_param->pxl_fmt;
				return HD_OK;
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_venc_rc_stuff(HD_GROUP *p_group, UINT32 line_idx, UINT *p_frame_rate_base, UINT *p_frame_rate_incr)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VENC_PARAM venc;
	HD_VIDEO_CODEC codec_type = HD_CODEC_TYPE_JPEG;
	UINT frame_rate_base = 0, frame_rate_incr = 0;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
			venc.param = videoenc_get_param_p(p_member->p_bind->device.deviceid);
			if (venc.param && venc.param->port[p_member->in_subid].p_enc_out_param != NULL) {
				codec_type = venc.param->port[p_member->in_subid].p_enc_out_param->codec_type;
			}
			if (codec_type == HD_CODEC_TYPE_JPEG) {
				if (venc.param->port[p_member->in_subid].p_jpeg_rc == NULL) {
					goto err_exit;
				}
				frame_rate_base = venc.param->port[p_member->in_subid].p_jpeg_rc->frame_rate_base;
				frame_rate_incr = venc.param->port[p_member->in_subid].p_jpeg_rc->frame_rate_incr;
			} else {
				if (venc.param->port[p_member->in_subid].p_enc_rc == NULL) {
					goto err_exit;
				}
				switch(venc.param->port[p_member->in_subid].p_enc_rc->rc_mode) {
				case HD_RC_MODE_CBR:
					frame_rate_base = venc.param->port[p_member->in_subid].p_enc_rc->cbr.frame_rate_base;
					frame_rate_incr = venc.param->port[p_member->in_subid].p_enc_rc->cbr.frame_rate_incr;
					break;
				case HD_RC_MODE_VBR:
					frame_rate_base = venc.param->port[p_member->in_subid].p_enc_rc->vbr.frame_rate_base;
					frame_rate_incr = venc.param->port[p_member->in_subid].p_enc_rc->vbr.frame_rate_incr;
					break;
				case HD_RC_MODE_FIX_QP:
					frame_rate_base = venc.param->port[p_member->in_subid].p_enc_rc->fixqp.frame_rate_base;
					frame_rate_incr = venc.param->port[p_member->in_subid].p_enc_rc->fixqp.frame_rate_incr;
					break;
				case HD_RC_MODE_EVBR:
					frame_rate_base = venc.param->port[p_member->in_subid].p_enc_rc->evbr.frame_rate_base;
					frame_rate_incr = venc.param->port[p_member->in_subid].p_enc_rc->evbr.frame_rate_incr;
					break;
				default:
					goto err_exit;
				}
			}
		}
	}

	if (frame_rate_incr == 0 || frame_rate_base == 0) {  // err checking
		goto err_exit;
	}

	if (p_frame_rate_base && p_frame_rate_incr) {
		*p_frame_rate_base = frame_rate_base;
		*p_frame_rate_incr = frame_rate_incr;
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_vproc_rotation(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEO_DIR *p_direction)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VPROC_PARAM vproc;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_direction == NULL) {
		GMLIB_ERROR_P("NULL p_direction\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			vproc.param = videoproc_get_param_p(p_member->p_bind->device.deviceid, p_member->out_subid);
			if (vproc.param && vproc.param->p_direction != NULL) {
				*p_direction = *vproc.param->p_direction;
				return HD_OK;
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_vproc_out_pxlfmt(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEO_PXLFMT *p_dst_fmt)
{
	INT member_idx, is_set = 0;
	HD_MEMBER *p_member;
	DIF_VPROC_PARAM vproc;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_dst_fmt == NULL) {
		GMLIB_ERROR_P("NULL p_dst_fmt\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			break;
		}
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			vproc.param = videoproc_get_param_p(p_member->p_bind->device.deviceid, p_member->out_subid);
			/* for vout with vpe_multi + vpe case: 2_level vpe, we need to use 2nd vpe dst_fmt */
			if (vproc.param && vproc.param->p_dst_fmt != NULL) {
				is_set = 1;
				*p_dst_fmt = *vproc.param->p_dst_fmt;
			}
		}
	}
	if (is_set == 1) {
		return HD_OK;
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_vproc_out_pool(HD_GROUP *p_group, UINT32 line_idx, HD_OUT_POOL *p_out_pool)
{
	INT member_idx, is_set = 0;
	HD_MEMBER *p_member;
	DIF_VPROC_PARAM vproc;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_out_pool == NULL) {
		GMLIB_ERROR_P("NULL p_out_pool\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			break;
		}
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			vproc.param = videoproc_get_param_p(p_member->p_bind->device.deviceid, p_member->out_subid);
			if (vproc.param && vproc.param->p_out_pool != NULL) {
				memcpy(p_out_pool, vproc.param->p_out_pool, sizeof(HD_OUT_POOL));
				is_set = 1;
			}
		}
	}
	if (is_set == 1) {
		return HD_OK;
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_vproc_out_and_di(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEO_PXLFMT *p_dst_fmt,
				   HD_URECT *p_dst_rect, HD_DIM *p_dst_bg_dim, BOOL *p_di_enable)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VPROC_PARAM vproc;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			vproc.param = videoproc_get_param_p(p_member->p_bind->device.deviceid, p_member->out_subid);
			if (vproc.param) {
				if (p_dst_fmt && vproc.param->p_dst_fmt) {
					memcpy(p_dst_fmt, vproc.param->p_dst_fmt, sizeof(HD_VIDEO_PXLFMT));
				}
				if (p_dst_rect && vproc.param->p_dst_rect) {
					memcpy(p_dst_rect, vproc.param->p_dst_rect, sizeof(HD_URECT));
				}
				if (p_dst_bg_dim && vproc.param->p_dst_bg_dim) {
					memcpy(p_dst_bg_dim, vproc.param->p_dst_bg_dim, sizeof(HD_DIM));
				}
				if (p_di_enable && vproc.param->p_di_enable) {
					memcpy(p_di_enable, vproc.param->p_di_enable, sizeof(BOOL));
				}
				return HD_OK;
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}


HD_RESULT dif_get_vcap_out(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEOCAP_OUT *p_cap_out)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VCAP_PARAM vcap;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_cap_out == NULL) {
		GMLIB_ERROR_P("NULL p_cap_out\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE) {
			vcap.param = videocap_get_param_p(p_member->p_bind->device.deviceid);
			if (vcap.param && vcap.param->cap_out[p_member->out_subid] != NULL) {
				memcpy(p_cap_out, vcap.param->cap_out[p_member->out_subid], sizeof(HD_VIDEOCAP_OUT));
				return HD_OK;
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_vdec_max_mem(HD_GROUP *p_group, UINT32 line_idx, VDODEC_MAXMEM *p_max_mem)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VDEC_PARAM vdec;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_max_mem == NULL) {
		GMLIB_ERROR_P("NULL p_max_mem\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) {
			vdec.param = videodec_get_param_p(p_member->p_bind->device.deviceid, p_member->out_subid);
			if (vdec.param && vdec.param->max_mem != NULL) {
				memcpy(p_max_mem, vdec.param->max_mem, sizeof(HD_VIDEOCAP_OUT));
				return HD_OK;
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_vout_win_attr(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEOOUT_WIN_ATTR *p_win)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VOUT_PARAM vout;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_win == NULL) {
		GMLIB_ERROR_P("NULL p_win\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			vout.param = videoout_get_param_p(p_member->p_bind->device.deviceid);
			if (vout.param && vout.param->win[p_member->in_subid] != NULL) {
				memcpy(p_win, vout.param->win[p_member->in_subid], sizeof(HD_VIDEOOUT_WIN_ATTR));
				return HD_OK;
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_vout_win_psr_attr(HD_GROUP *p_group, UINT32 line_idx, HD_VIDEOOUT_WIN_PSR_ATTR *p_win_psr)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VOUT_PARAM vout;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_win_psr == NULL) {
		GMLIB_ERROR_P("NULL p_win_psr\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			vout.param = videoout_get_param_p(p_member->p_bind->device.deviceid);
			if (vout.param && vout.param->win_psr[p_member->in_subid] != NULL) {
				memcpy(p_win_psr, vout.param->win_psr[p_member->in_subid], sizeof(HD_VIDEOOUT_WIN_PSR_ATTR));
				return HD_OK;
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_aenc_codec_type(HD_GROUP *p_group, INT line_idx, HD_AUDIO_CODEC *p_codec_type)
{
	INT member_idx;
	HD_MEMBER *p_member;
	AUDENC_INTERNAL_PARAM *p_aenc_param;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_codec_type == NULL) {
		GMLIB_ERROR_P("NULL p_codec_type\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_AUDIOENC_BASE) {
			p_aenc_param = audioenc_get_param_p(p_member->p_bind->device.deviceid);
			if (p_aenc_param == NULL) {
				GMLIB_ERROR_P("NULL aenc_param\n");
				goto err_exit;
			}
			*p_codec_type = p_aenc_param->port[p_member->out_subid].p_enc_out->codec_type;
			return HD_OK;
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_adec_codec_type(HD_GROUP *p_group, INT line_idx, HD_AUDIO_CODEC *p_codec_type)
{
	INT member_idx;
	HD_MEMBER *p_member;
	AUDDEC_INTERNAL_PARAM *p_adec_param;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_codec_type == NULL) {
		GMLIB_ERROR_P("NULL p_codec_type\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_AUDIODEC_BASE) {
			p_adec_param = audiodec_get_param_p(p_member->p_bind->device.deviceid);
			if (p_adec_param == NULL) {
				GMLIB_ERROR_P("NULL aenc_param\n");
				goto err_exit;
			}
			*p_codec_type = p_adec_param->port[p_member->out_subid].p_dec_in->codec_type;
			return HD_OK;
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_adec_in_param(HD_GROUP *p_group, INT line_idx, HD_AUDIODEC_IN *p_dec_in)
{
	INT member_idx;
	HD_MEMBER *p_member;
	AUDDEC_INTERNAL_PARAM *p_adec_param;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (p_dec_in == NULL) {
		GMLIB_ERROR_P("NULL p_dec_in\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_AUDIODEC_BASE) {
			p_adec_param = audiodec_get_param_p(p_member->p_bind->device.deviceid);
			if (p_adec_param && p_adec_param->port[p_member->out_subid].p_dec_in != NULL) {
				memcpy(p_dec_in, p_adec_param->port[p_member->out_subid].p_dec_in, sizeof(HD_AUDIODEC_IN));
				return HD_OK;
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}


// this func search next bind venc, to get max fps of venc
void get_next_all_venc_fps(struct list_head *listhead, INT layer, INT *fps)
{
	struct list_head *dst_listhead;
	HD_DEV_NODE *dev_node;
	HD_BIND_INFO *p_next_bind = NULL;
	INT32 out_nr, i, enc_fps;
	DIF_VENC_PARAM venc;
	HD_H26XENC_RATE_CONTROL *p_enc_rc;

	if (layer >= HD_MAX_GRAPH_RECURSIVE_LEVEL) {
		GMLIB_ERROR_P("%s: layer(%d) overflow\n", __func__, layer);
		goto ext;
	}
	list_for_each_entry(dev_node, listhead, list) {
		p_next_bind = (HD_BIND_INFO *)dev_node->p_bind_info;

		if ((bd_get_dev_baseid(p_next_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) &&
		    (p_next_bind->in[dev_node->in_subid].port_state == HD_PORT_START)) {

			venc.param = videoenc_get_param_p(p_next_bind->device.deviceid);
			if (venc.param && venc.param->port[dev_node->in_subid].p_enc_rc) {
				p_enc_rc = venc.param->port[dev_node->in_subid].p_enc_rc;
				if (p_enc_rc->rc_mode == HD_RC_MODE_CBR) {
					enc_fps = p_enc_rc->cbr.frame_rate_base / p_enc_rc->cbr.frame_rate_incr;
				} else if (p_enc_rc->rc_mode == HD_RC_MODE_VBR) {
					enc_fps = p_enc_rc->vbr.frame_rate_base / p_enc_rc->vbr.frame_rate_incr;
				} else if (p_enc_rc->rc_mode == HD_RC_MODE_FIX_QP) {
					enc_fps = p_enc_rc->fixqp.frame_rate_base / p_enc_rc->fixqp.frame_rate_incr;
				} else if (p_enc_rc->rc_mode == HD_RC_MODE_EVBR) {
					enc_fps = p_enc_rc->evbr.frame_rate_base / p_enc_rc->evbr.frame_rate_incr;
				} else {
					GMLIB_ERROR_P("No venc.rc mode setting, venc_id(%#x)\n", p_next_bind->device.deviceid);
					goto ext;
				}
				*fps = MAX_VAL(*fps, enc_fps);
				GMLIB_L1_P("  %s==> *max_fps(%d) enc_fps(%d)\n", dif_indent_string[layer], *fps, enc_fps);
			}
		}

		if (is_end_device_p(p_next_bind->device.deviceid) == TRUE) {
			GMLIB_L1_P("  %s %s_%d IN(%d) st(%d)\n", dif_indent_string[layer], get_bind_name(p_next_bind),
				   bd_get_dev_subid(p_next_bind->device.deviceid),
				   (int) dev_node->in_subid,
				   (int) p_next_bind->in[dev_node->in_subid].port_state);
			continue;
		} else {
			out_nr = get_bind_out_nr(p_next_bind);
			for (i = 0; i < out_nr; i++) {
				dst_listhead = &p_next_bind->out[i].bnode->dst_listhead;
				if (list_empty(dst_listhead))
					continue;

				if (p_next_bind->out[i].port_state != HD_PORT_START)
					continue;

				GMLIB_L1_P("  %s %s_%d port(%d %d) st(%d %d)\n", dif_indent_string[layer], get_bind_name(p_next_bind),
					   bd_get_dev_subid(p_next_bind->device.deviceid),
					   (int) dev_node->in_subid,
					   (int) i,
					   (int) p_next_bind->in[dev_node->in_subid].port_state,
					   (int) p_next_bind->out[i].port_state);
				get_next_all_venc_fps(dst_listhead, layer + 1, fps);
			}
		}
	}

ext:
	return;
}

// this func search next bind venc, to get max fps of venc
INT dif_get_vcap_fps_for_rec_multi_path(HD_DAL vcap_deviceid, UINT32 vcap_out_subid)
{
	HD_BIND_INFO *p_bind;
	struct list_head *listhead;
	INT vcap_fps = -1;

	if (bd_get_dev_baseid(vcap_deviceid) != HD_DAL_VIDEOCAP_BASE) {
		goto ext; /* illegal, return -1 */
	}
	if ((p_bind = get_bind(vcap_deviceid)) == NULL) {
		GMLIB_ERROR_P("%s: deviceid(%#x) err \n", __func__, vcap_deviceid);
		goto ext; /* illegal, return -1 */
	}
	if ((listhead = get_next_bind_listhead(p_bind, vcap_out_subid)) == NULL) {
		goto ext;
	}
	GMLIB_L1_P("get vcap fps:\n");
	get_next_all_venc_fps(listhead, 1, &vcap_fps);

ext:
	return vcap_fps;
}

HD_RESULT dif_get_vcap_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *vcap_vch, INT *vcap_path)
{
	INT member_idx;
	HD_MEMBER *p_member;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (vcap_vch == NULL) {
		GMLIB_ERROR_P("NULL vcap_vch\n");
		goto err_exit;
	}
	if (vcap_path == NULL) {
		GMLIB_ERROR_P("NULL vcap_path\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE) {
			*vcap_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			*vcap_path = p_member->out_subid;
			return HD_OK;
		}
	}

err_exit:
	return HD_ERR_NG;
}


HD_RESULT dif_get_real_vcap_fps(HD_VIDEOCAP_OUT *p_cap_out, INT cap_vch, INT *p_vcap_fps)
{
	/* vcap ---> vpe0_0 +--> venc0
	 *               _1 +--> venc1
	 * NOTE: multi_path, we choose p_cap_out->frc
	 */
	INT denominator = (p_cap_out->frc & 0x0000FFFF);
	INT numerator   = (p_cap_out->frc & 0xFFFF0000) >> 16;
	INT vcap_fps = 0;

	if (denominator == 0) {
		vcap_fps = platform_sys_Info.cap_info[cap_vch].fps;
	} else {
		vcap_fps = (numerator * platform_sys_Info.cap_info[cap_vch].fps) / denominator;
		if (vcap_fps < 1 || vcap_fps > platform_sys_Info.cap_info[cap_vch].fps) {
			vcap_fps = platform_sys_Info.cap_info[cap_vch].fps;
		}
	}
	if (p_vcap_fps) {
		*p_vcap_fps = vcap_fps;
	}
	return HD_OK;
}


HD_RESULT dif_get_cap_vpe_link_requirement(VDOPROC_INTL_PARAM *vproc_param, HD_VIDEOCAP_OUT *p_cap_out,
		BOOL *p_need_link_di, BOOL *p_need_link_vpe)
{
	if (!vproc_param || !p_cap_out) {
		return HD_ERR_NG;
	}

	// if in/out the same size --> bind di only
	// if in/out different size --> bind di + vpe
	if (p_need_link_di) {
		*p_need_link_di = (vproc_param->p_di_enable != NULL) && (*(vproc_param->p_di_enable));
	}
	if (p_need_link_vpe) {
		if (vproc_param->use_vpe_func) {
			*p_need_link_vpe = TRUE;
		} else {
			*p_need_link_vpe = (vproc_param->p_dst_rect != NULL) &&
							(vproc_param->p_dst_bg_dim != NULL) &&
							(vproc_param->p_dst_fmt != NULL) &&
							!(vproc_param->p_dst_rect->x == 0 &&
					     	vproc_param->p_dst_rect->y == 0 &&
					     	vproc_param->p_dst_rect->w == p_cap_out->dim.w &&
					     	vproc_param->p_dst_rect->h == p_cap_out->dim.h &&
					     	vproc_param->p_dst_bg_dim->w == p_cap_out->dim.w &&
					     	vproc_param->p_dst_bg_dim->h == p_cap_out->dim.h &&
					     	*vproc_param->p_dst_fmt == p_cap_out->pxlfmt &&
					     	*vproc_param->p_direction == HD_VIDEO_DIR_NONE) &&
					     	!(vproc_param->p_dst_rect->x == 0 &&
					     	vproc_param->p_dst_rect->y == 0 &&
					     	vproc_param->p_dst_rect->w == 0 &&
					     	vproc_param->p_dst_rect->h == 0 &&
					     	vproc_param->p_dst_bg_dim->w == 0 &&
					     	vproc_param->p_dst_bg_dim->h == 0);
		}
	}
	return HD_OK;

}

HD_RESULT dif_get_vpe_link_requirement(VDOPROC_INTL_PARAM *vproc_param, BOOL *p_need_link_vpe)
{
	if (!vproc_param) {
		return HD_ERR_NG;
	}

	// if in/out different size ,format or use other vpe function--> bind vpe
	if (p_need_link_vpe) {
		if (vproc_param->use_vpe_func) {
			*p_need_link_vpe = TRUE;
		} else {
			*p_need_link_vpe = (vproc_param->p_dst_rect != NULL) &&
							(vproc_param->p_dst_bg_dim != NULL) &&
							(vproc_param->p_dst_fmt != NULL) &&
							!(vproc_param->p_dst_rect->x == 0 &&
					     	vproc_param->p_dst_rect->y == 0 &&
					     	vproc_param->p_dst_rect->w == vproc_param->p_src_bg_dim->w &&
					     	vproc_param->p_dst_rect->h == vproc_param->p_src_bg_dim->h &&
					     	vproc_param->p_dst_bg_dim->w == vproc_param->p_src_bg_dim->w &&
					     	vproc_param->p_dst_bg_dim->h == vproc_param->p_src_bg_dim->h &&
					     	*vproc_param->p_dst_fmt == *vproc_param->p_src_fmt &&
					     	*vproc_param->p_direction == HD_VIDEO_DIR_NONE) &&
					     	!(vproc_param->p_dst_rect->x == 0 &&
					     	vproc_param->p_dst_rect->y == 0 &&
					     	vproc_param->p_dst_rect->w == 0 &&
					     	vproc_param->p_dst_rect->h == 0 &&
					     	vproc_param->p_dst_bg_dim->w == 0 &&
					     	vproc_param->p_dst_bg_dim->h == 0);
		}
	}
	return HD_OK;

}

HD_RESULT dif_get_venc_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *venc_vch, INT *venc_path)
{
	INT member_idx;
	HD_MEMBER *p_member;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (venc_vch == NULL) {
		GMLIB_ERROR_P("NULL venc_vch\n");
		goto err_exit;
	}
	if (venc_path == NULL) {
		GMLIB_ERROR_P("NULL venc_path\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
			*venc_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			*venc_path = p_member->in_subid;
			return HD_OK;
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_lcd_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *lcd_vch, INT *lcd_path)
{
	INT member_idx;
	HD_MEMBER *p_member;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (lcd_vch == NULL) {
		GMLIB_ERROR_P("NULL lcd_vch\n");
		goto err_exit;
	}
	if (lcd_path == NULL) {
		GMLIB_ERROR_P("NULL lcd_path\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			*lcd_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			*lcd_path = p_member->in_subid;
			return HD_OK;
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_lcd_output_type(HD_GROUP *p_group, UINT32 line_idx, INT *is_virtual)
{
	INT member_idx;
	HD_MEMBER *p_member;
	VDDO_INTERNAL_PARAM *p_vout_param;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (is_virtual == NULL) {
		GMLIB_ERROR_P("NULL is_virtual\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			p_vout_param = videoout_get_param_p(p_member->p_bind->device.deviceid);
			if (p_vout_param && p_vout_param->mode && p_vout_param->mode->output_type == HD_COMMON_VIDEO_OUT_LCD) {
				*is_virtual = 1;
			} else {
				*is_virtual = 0;
			}
			return HD_OK;
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_vpe_cascade_level(HD_GROUP *p_group, UINT32 line_idx, INT *cascade_level)
{
	INT member_idx, count = 0;
	HD_MEMBER *p_member;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (cascade_level == NULL) {
		GMLIB_ERROR_P("NULL cascade_level\n");
		goto err_exit;
	}
	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			count++;
		} else if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			break;
		}
	}

	*cascade_level = count;
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_vpe_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *vpe_vch, INT *vpe_path)
{
	INT member_idx;
	HD_MEMBER *p_member;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (vpe_vch == NULL) {
		GMLIB_ERROR_P("NULL vpe_vch\n");
		goto err_exit;
	}
	if (vpe_path == NULL) {
		GMLIB_ERROR_P("NULL vpe_path\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			*vpe_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			*vpe_path = p_member->in_subid;
			return HD_OK;
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_acap_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *acap_dev, INT *acap_ch)
{
	INT member_idx;
	HD_MEMBER *p_member;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (acap_dev == NULL) {
		GMLIB_ERROR_P("NULL acap_vch\n");
		goto err_exit;
	}
	if (acap_ch == NULL) {
		GMLIB_ERROR_P("NULL acap_path\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_AUDIOCAP_BASE) {
			*acap_dev = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			*acap_ch = p_member->out_subid;
			return HD_OK;
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_next_member_id(HD_GROUP *p_group, UINT32 line_idx, INT this_member_idx,
				 INT *p_next_devid, UINT32 *p_next_in_sub_id, UINT32 *p_next_out_sub_id)
{
	INT next_member_idx = this_member_idx + 1;
	HD_MEMBER *p_member;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (next_member_idx >= HD_MAX_BIND_NUNBER_PER_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}

	if (p_group->glines[line_idx].member[next_member_idx].p_bind == NULL) {
		GMLIB_ERROR_P("Next module is NULL\n");
		goto err_exit;
	}
	if (!p_next_devid || !p_next_in_sub_id || !p_next_out_sub_id) {
		GMLIB_ERROR_P("p_next_pathid is NULL\n");
		goto err_exit;
	}

	p_member = &(p_group->glines[line_idx].member[next_member_idx]);
	*p_next_devid = p_member->p_bind->device.deviceid;
	*p_next_in_sub_id = p_member->in_subid;
	*p_next_out_sub_id = p_member->out_subid;

	return HD_OK;

err_exit:
	return HD_ERR_NG;
}


HD_RESULT dif_get_aout_ch_path(HD_GROUP *p_group, UINT32 line_idx, INT *aout_vch, INT *aout_path)
{
	INT member_idx;
	HD_MEMBER *p_member;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}
	if (aout_vch == NULL) {
		GMLIB_ERROR_P("NULL aout_vch\n");
		goto err_exit;
	}
	if (aout_path == NULL) {
		GMLIB_ERROR_P("NULL aout_path\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
			continue;
		}
		p_member = &(p_group->glines[line_idx].member[member_idx]);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_AUDIOOUT_BASE) {
			*aout_vch = bd_get_dev_subid(p_member->p_bind->device.deviceid);
			*aout_path = p_member->in_subid;
			return HD_OK;
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_audio_record_bind_num(HD_GROUP *p_group, HD_BIND_INFO *p_bind, INT *bind_num)
{
	INT line_idx, member_idx;
	HD_MEMBER *p_member, *p_next_meber;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (p_bind == NULL) {
		GMLIB_ERROR_P("NULL p_bind\n");
		goto err_exit;
	}
	if (bind_num == NULL) {
		GMLIB_ERROR_P("NULL bind_num\n");
		goto err_exit;
	}

	*bind_num = 0;
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (p_group->glines[line_idx].state == HD_LINE_INCOMPLETE)  {
			continue;
		}

		for (member_idx = 0; member_idx < (HD_MAX_BIND_NUNBER_PER_LINE - 1); member_idx++) {
			if (p_group->glines[line_idx].member[member_idx].p_bind == NULL) {
				continue;
			}
			p_member = &(p_group->glines[line_idx].member[member_idx]);
			if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) != HD_DAL_AUDIOCAP_BASE) {
				continue;
			}
			if (p_member->p_bind == p_bind) {
				p_next_meber = &(p_group->glines[line_idx].member[member_idx + 1]);
				if (p_next_meber->p_bind == NULL) {
					continue;
				}
				if (bd_get_dev_baseid(p_next_meber->p_bind->device.deviceid) == HD_DAL_AUDIOENC_BASE) {
					*bind_num = *bind_num + 1;
				}
			}
		}
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_PATH_ID dif_get_path_id(DIF_DEV *dev)
{
	return ((((dev->device_baseid | dev->device_subid) & 0xffff) << 16) | \
		(((dev->in_subid + HD_IN_BASE) & 0xff) << 8) | \
		(((dev->out_subid + HD_OUT_BASE) & 0xff)));
}

HD_PATH_ID dif_get_path_id_by_binding(HD_BIND_INFO *p_bind, UINT32 in_subid, UINT32 out_subid)
{
	return (((p_bind->device.deviceid & 0xffff) << 16) | \
		(((in_subid + HD_IN_BASE) & 0xff) << 8) | \
		(((out_subid + HD_OUT_BASE) & 0xff)));
}

VOID dif_get_dev_string(HD_PATH_ID path_id, CHAR *string, UINT len)
{
	HD_IO in_subid, out_subid;
	UINT32 devid;

	if (len <= 0) {
		GMLIB_ERROR_P("string len = 0.\n");
		goto ext;
	}

	if (string == NULL) {
		GMLIB_ERROR_P("string point is NULL.\n");
		goto ext;
	}

	devid = HD_GET_DEV(path_id);
	in_subid = BD_GET_IN_SUBID(HD_GET_IN(path_id));
	out_subid = BD_GET_OUT_SUBID(HD_GET_OUT(path_id));

	snprintf(string, len - 1, "%s_%d_CHIP_%d_IN_%d_OUT_%d",
		 get_device_name(devid),
		 bd_get_dev_subid(devid),
		 bd_get_chip_by_device(devid),
		 in_subid, out_subid);
ext:
	return;
}

CHAR *dif_return_dev_string(HD_PATH_ID path_id)
{
	static CHAR dev_string[64];

	dif_get_dev_string(path_id, dev_string, sizeof(dev_string));

	return dev_string;
}

HD_RESULT check_in_out_unit_parm_p(VOID *p_in_param, DIF_DEV *p_in_dev, VOID *p_out_param, DIF_DEV *p_out_dev)
{
	INT is_check_in_dim = 0, is_check_in_fmt = 0, is_check_out_dim = 0, is_check_out_fmt = 0;
	HD_DIM in_dim = {0}, out_dim = {0};
	HD_VIDEO_PXLFMT in_fmt = HD_VIDEO_PXLFMT_NONE, out_fmt = HD_VIDEO_PXLFMT_NONE;
	CAP_INTERNAL_PARAM *vcap_param;
	VDODEC_INTL_PARAM *vdec_param;
	VDOENC_INTERNAL_PARAM *venc_param;
	VDOPROC_INTL_PARAM *vproc_param;
	INT lcd_vch = 0, is_check_max = 0;
	HD_VIDEO_DIR dir;
	CHAR in_dev_string[64], out_dev_string[64];

	if (p_in_param == NULL) {
		GMLIB_ERROR_P("NULL p_in_param\n");
		goto err_exit;
	}
	if (p_in_dev == NULL) {
		GMLIB_ERROR_P("NULL p_in_dev\n");
		goto err_exit;
	}
	if (p_out_param == NULL) {
		GMLIB_ERROR_P("NULL p_out_param\n");
		goto err_exit;
	}
	if (p_out_dev == NULL) {
		GMLIB_ERROR_P("NULL p_out_dev\n");
		goto err_exit;
	}

	/* Get input param */
	switch (p_in_dev->device_baseid) {
	case HD_DAL_VIDEOENC_BASE:
		venc_param = (VDOENC_INTERNAL_PARAM *)p_in_param;
		if (venc_param->port[p_in_dev->in_subid].p_enc_in_param == NULL) {
			GMLIB_ERROR_P("Check in unit error: venc in param is not set.\n");
			goto err_exit;
		}
		dir = venc_param->port[p_in_dev->in_subid].p_enc_in_param->dir;
		if (HD_VIDEO_DIR_ROTATE_90 == dir || HD_VIDEO_DIR_ROTATE_270 == dir) {
			in_dim.w = venc_param->port[p_in_dev->in_subid].p_enc_in_param->dim.h;
			in_dim.h = venc_param->port[p_in_dev->in_subid].p_enc_in_param->dim.w;
		} else {
			in_dim.w = venc_param->port[p_in_dev->in_subid].p_enc_in_param->dim.w;
			in_dim.h = venc_param->port[p_in_dev->in_subid].p_enc_in_param->dim.h;
		}
		in_fmt = venc_param->port[p_in_dev->in_subid].p_enc_in_param->pxl_fmt;
		is_check_in_dim = is_check_in_fmt = 1;
		break;

	case HD_DAL_VIDEOOUT_BASE:
		lcd_vch = p_in_dev->device_subid;
		if (lcd_vch >= VPD_MAX_LCD_NUM) {
			GMLIB_ERROR_P("Check in unit error: vout device_subid(%d) > max(%d).\n", lcd_vch, VPD_MAX_LCD_NUM);
			goto err_exit;
		}
		if (platform_sys_Info.lcd_info[lcd_vch].lcdid >= 0) {
			in_dim.w = platform_sys_Info.lcd_info[lcd_vch].fb0_win.width;
			in_dim.h = platform_sys_Info.lcd_info[lcd_vch].fb0_win.height;
			in_fmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(platform_sys_Info.lcd_info[lcd_vch].lcd_fmt);
			is_check_in_dim = is_check_in_fmt = 1;
		}
		break;

	case HD_DAL_VIDEOPROC_BASE:
		vproc_param = (VDOPROC_INTL_PARAM *)p_in_param;
		if (vproc_param->p_src_bg_dim) {
			in_dim.w = vproc_param->p_src_bg_dim->w;
			in_dim.h = vproc_param->p_src_bg_dim->h;
			is_check_in_dim = 1;
		}
		/* No need to check input pxlfmt for vproc */
		break;

	case HD_DAL_AUDIOCAP_BASE:
	case HD_DAL_AUDIODEC_BASE:
	case HD_DAL_AUDIOENC_BASE:
	case HD_DAL_AUDIOOUT_BASE:
	case HD_DAL_VIDEOCAP_BASE:
	case HD_DAL_VIDEODEC_BASE:
		is_check_in_dim = is_check_in_fmt = 0;
		break;

	default:
		GMLIB_ERROR_P("not support this in device(%#x)\n", p_in_dev->device_subid);
		goto err_exit;
	}

	/* Get output param */
	switch (p_out_dev->device_baseid) {
	case HD_DAL_VIDEOCAP_BASE:
		vcap_param = (CAP_INTERNAL_PARAM *)p_out_param;
		if (vcap_param->cap_out[p_out_dev->out_subid] == NULL) {
			GMLIB_ERROR_P("Check out unit error: vcap out param is not set.\n");
			goto err_exit;
		}
		out_dim.w = vcap_param->cap_out[p_out_dev->out_subid]->dim.w;
		out_dim.h = vcap_param->cap_out[p_out_dev->out_subid]->dim.h;
		out_fmt = vcap_param->cap_out[p_out_dev->out_subid]->pxlfmt;
		is_check_out_dim = is_check_out_fmt = 1;
		break;

	case HD_DAL_VIDEODEC_BASE:
		vdec_param = (VDODEC_INTL_PARAM *)p_out_param;
		if (vdec_param->max_mem == NULL) {
			GMLIB_ERROR_P("Check out unit error: vdec max_mem param is not set.\n");
			goto err_exit;
		}
		out_dim.w = vdec_param->max_mem->max_width;
		out_dim.h = vdec_param->max_mem->max_height;
		if (dif_get_pxlfmt_by_codec_type(vdec_param->max_mem->codec_type, &out_fmt, 0) != HD_OK) {
			GMLIB_ERROR_P("<dif_get_pxlfmt_by_codec_type fail >\n");
			goto err_exit;
		}
		is_check_out_dim = is_check_out_fmt = 1;
		is_check_max = 1;
		break;

	case HD_DAL_VIDEOPROC_BASE:
		vproc_param = (VDOPROC_INTL_PARAM *)p_out_param;
		if (vproc_param->p_dst_bg_dim) {
			out_dim.w = vproc_param->p_dst_bg_dim->w;
			out_dim.h = vproc_param->p_dst_bg_dim->h;
			is_check_out_dim = 1;
		} else {
			GMLIB_ERROR_P("Check in unit error: vproc out param is not set.\n");
			goto err_exit;
		}
		if (vproc_param->p_dst_fmt) {
			out_fmt = *vproc_param->p_dst_fmt;
			is_check_out_fmt = 1;
		} else {
			GMLIB_ERROR_P("Check in unit error: vproc out pxlfmt is not set.\n");
			goto err_exit;
		}
		break;

	case HD_DAL_AUDIOCAP_BASE:
	case HD_DAL_AUDIODEC_BASE:
	case HD_DAL_AUDIOENC_BASE:
	case HD_DAL_AUDIOOUT_BASE:
	case HD_DAL_VIDEOENC_BASE:
	case HD_DAL_VIDEOOUT_BASE:
		is_check_out_dim = is_check_out_fmt = 0;
		break;
	default:
		GMLIB_ERROR_P("not support this in device(%#x)\n", p_in_dev->device_subid);
		goto err_exit;
	}
	if (is_check_in_dim && is_check_out_dim) {
		if (is_check_max) {
			if ((in_dim.w > out_dim.w) || (in_dim.h > out_dim.h)) {
				dif_get_dev_string(dif_get_path_id(p_in_dev), in_dev_string, sizeof(in_dev_string));
				dif_get_dev_string(dif_get_path_id(p_out_dev), out_dev_string, sizeof(out_dev_string));
				GMLIB_ERROR_P("check_in_out_unit_parm fail. IN(%s, %#x) dim(%u,%u) > OUT(%s, %#x) dim(%u,%u)\n",
					      in_dev_string, dif_get_path_id(p_in_dev), in_dim.w, in_dim.h,
					      out_dev_string, dif_get_path_id(p_out_dev), out_dim.w, out_dim.h);
				goto err_exit;
			}
		} else {
			if ((in_dim.w != out_dim.w) || (in_dim.h != out_dim.h)) {
				dif_get_dev_string(dif_get_path_id(p_in_dev), in_dev_string, sizeof(in_dev_string));
				dif_get_dev_string(dif_get_path_id(p_out_dev), out_dev_string, sizeof(out_dev_string));
				GMLIB_ERROR_P("check_in_out_unit_parm fail. IN(%s, %#x) dim(%u,%u) != OUT(%s, %#x) dim(%u,%u)\n",
					      in_dev_string, dif_get_path_id(p_in_dev), in_dim.w, in_dim.h,
					      out_dev_string, dif_get_path_id(p_out_dev), out_dim.w, out_dim.h);
				goto err_exit;
			}
		}
	}
	if (is_check_in_fmt && is_check_out_fmt) {
		if (in_fmt != out_fmt) {
			dif_get_dev_string(dif_get_path_id(p_in_dev), in_dev_string, sizeof(in_dev_string));
			dif_get_dev_string(dif_get_path_id(p_out_dev), out_dev_string, sizeof(out_dev_string));
			GMLIB_ERROR_P("check_in_out_unit_parm fail. IN(%s, %#x) fmt(%#x) != OUT(%s, %#x) fmt(%#x)\n",
				      in_dev_string, dif_get_path_id(p_in_dev), in_fmt,
				      out_dev_string, dif_get_path_id(p_out_dev), out_fmt);
			goto err_exit;
		}
	}

	GMLIB_L3_P("OK IN(%#x) param(%p) check(%d,%d) OUT(%#x) param(%p) check(%d,%d)\n",
		   dif_get_path_id(p_in_dev), p_in_param, is_check_in_dim, is_check_in_fmt,
		   dif_get_path_id(p_out_dev), p_out_param, is_check_out_dim, is_check_out_fmt);
	return HD_OK;

err_exit:
	GMLIB_L3_P("NG IN(%p) param(%p) check(%d,%d) OUT(%p) param(%p) check(%d,%d)\n",
		   p_in_dev, p_in_param, is_check_in_dim, is_check_in_fmt,
		   p_out_dev, p_out_param, is_check_out_dim, is_check_out_fmt);
	return HD_ERR_NG;
}

inline VOID dif_set_dev(HD_MEMBER *p_member, DIF_DEV *p_dev, UINT member_idx)
{
	p_dev->device_baseid = bd_get_dev_baseid(p_member->p_bind->device.deviceid);
	p_dev->device_subid = bd_get_dev_subid(p_member->p_bind->device.deviceid);
	p_dev->in_subid = p_member->in_subid;
	p_dev->out_subid = p_member->out_subid;
	p_dev->p_bind = p_member->p_bind;
	p_dev->member_idx = member_idx;
	p_dev->is_bound = TRUE;
	return;
}

void *dif_get_vpd_entity(HD_GROUP *p_group, UINT32 line_idx, INT entity_idx)
{
	void *prev_vpd_entity = NULL;
	pif_group_t *group;
	INT offset;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Wrong graph line,line(%u > %d)\n", line_idx, HD_MAX_GRAPH_LINE);
		goto exit;
	}
	if (entity_idx < 0 || entity_idx >= HD_MAX_BIND_NUNBER_PER_LINE) {
		GMLIB_ERROR_P("Wrong entity_idx(%d) range(0~%d)\n", entity_idx, HD_MAX_BIND_NUNBER_PER_LINE);
		goto exit;
	}
	group = pif_get_group_by_groupfd(p_group->groupfd);
	if (group == NULL) {
		GMLIB_ERROR_P("NULL group\n");
		goto exit;
	}
	offset = p_group->entity_buf_offs[line_idx][entity_idx];
	prev_vpd_entity = group->encap_buf + offset + VPD_ENTITY_HEAD_LEN;
exit:
	return prev_vpd_entity;
}

HD_RESULT dif_bind_set(HD_GROUP *p_group, UINT32 line_idx)
{
	INT member_idx, entity_idx = 0, len = 0;
	INT vcap_offs = 0, vdec_offs = 0, venc_offs = 0, vout_offs = 0, vproc_offs = 0;
	INT acap_offs = 0, adec_offs = 0, aenc_offs = 0, aout_offs = 0, di_offs = 0;
	INT osg_offs = 0, datain_offs = 0, dataout_offs = 0, enc_osg_offs = 0;
	INT is_virtual_lcd = 0;
	HD_GROUP_LINE *one_line;
	DIF_VCAP_PARAM vcap;
	DIF_VDEC_PARAM vdec;
	DIF_VENC_PARAM venc;
	DIF_VOUT_PARAM vout;
	DIF_VPROC_PARAM vproc, vproc_out; // vproc: for current setting, vproc_out: keep param for next vproc out
	DIF_ACAP_PARAM acap;
	DIF_ADEC_PARAM adec;
	DIF_AENC_PARAM aenc;
	DIF_AOUT_PARAM aout;
	VOID *p_out_param = NULL;
	DIF_DEV *p_out_dev = NULL;
	HD_VIDEO_CODEC codec_type = HD_CODEC_TYPE_H265;
	VDOPROC_INTL_PARAM *p_vproc_param;
	BOOL need_link_vpe = FALSE;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}

	GMLIB_L1_P("%s: dif_alloc_nr(%d)\n", __func__, dif_alloc_nr);
	one_line = &p_group->glines[line_idx];
	one_line->linefd = BD_SET_LINE_FD((INT)p_group->groupfd, line_idx, one_line->s_idx, one_line->e1_idx);
	GMLIB_L2_P("dif_bind_set p_group(%p) groupfd(%p) line_idx(%u) linefd(%#x) buf_len(%d)\n", p_group, p_group->groupfd, \
		   line_idx, one_line->linefd, p_group->entity_buf_len);

	/* get device param */
	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {

		if (one_line->member[member_idx].p_bind == NULL) {
			continue;
		}
		switch (bd_get_dev_baseid(one_line->member[member_idx].p_bind->device.deviceid)) {
		case HD_DAL_VIDEOCAP_BASE:
			vcap.param = videocap_get_param_p(one_line->member[member_idx].p_bind->device.deviceid);
			dif_set_dev(&one_line->member[member_idx], &vcap.dev, member_idx);

			if ((member_idx != 0) && (check_in_out_unit_parm_p(vcap.param, &vcap.dev, p_out_param, p_out_dev) != HD_OK)) {
				goto err_exit;
			}
			p_out_param = vcap.param;
			p_out_dev = &vcap.dev;

			vcap_offs = set_vcap_unit(p_group, line_idx, &vcap, &len);
			if (vcap_offs > 0) {
				p_group->entity_buf_offs[line_idx][entity_idx++] = vcap_offs;
				p_group->entity_buf_len += len;
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_VCAP);
			} else if (vcap_offs < 0) {
				GMLIB_ERROR_P("set_vcap_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_vcap_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, vcap_offs, entity_idx);

			if (dif_is_di_enable(p_group, line_idx, &p_vproc_param)) {
				if (p_vproc_param == NULL) {
					GMLIB_ERROR_P("set_di_unit fail! line_idx(%u) member_idx(%d), no vproc info\n", line_idx, member_idx);
					goto err_exit;
				}
				di_offs = set_di_unit(p_group, line_idx, &vcap, p_vproc_param, &len);
				if (di_offs > 0) {
					p_group->entity_buf_offs[line_idx][entity_idx++] = di_offs;
					p_group->entity_buf_len += len;
					BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_DI);
				} else if (di_offs < 0) {
					GMLIB_ERROR_P("set_di_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
					goto err_exit;
				}
				GMLIB_L2_P("set_di_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, di_offs, entity_idx);
			}
			break;

		case HD_DAL_VIDEODEC_BASE:
			vdec.param = videodec_get_param_p(one_line->member[member_idx].p_bind->device.deviceid,
							  one_line->member[member_idx].out_subid);
			dif_set_dev(&one_line->member[member_idx], &vdec.dev, member_idx);

			if ((member_idx != 0) && (check_in_out_unit_parm_p(vdec.param, &vdec.dev, p_out_param, p_out_dev) != HD_OK)) {
				goto err_exit;
			}
			p_out_param = vdec.param;
			p_out_dev = &vdec.dev;


			datain_offs = set_datain_unit(p_group, line_idx, &vdec, &len);
			if (datain_offs > 0) {
				p_group->entity_buf_offs[line_idx][entity_idx++] = datain_offs;
				p_group->entity_buf_len += len;
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_DATAIN);
			} else if (datain_offs < 0) {
				GMLIB_ERROR_P("set_datain_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_datain_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, datain_offs, entity_idx);

			vdec_offs = set_vdec_unit(p_group, line_idx, &vdec, &len);
			if (vdec_offs > 0) {
				p_group->entity_buf_offs[line_idx][entity_idx++] = vdec_offs;
				p_group->entity_buf_len += len;
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_VDEC);
			} else if (vdec_offs < 0) {
				GMLIB_ERROR_P("set_vdec_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_vdec_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, vdec_offs, entity_idx);
			break;

		case HD_DAL_VIDEOENC_BASE:
			venc.param = videoenc_get_param_p(one_line->member[member_idx].p_bind->device.deviceid);
			dif_set_dev(&one_line->member[member_idx], &venc.dev, member_idx);

			if (dif_get_venc_codec_type(p_group, line_idx, &codec_type) != HD_OK) {
				GMLIB_L2_P("dif_get_venc_codec_type fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			if (codec_type != HD_CODEC_TYPE_RAW) {
				if (p_out_dev->device_baseid == HD_DAL_VIDEOPROC_BASE && !need_link_vpe) {
					GMLIB_L2_P("previous device is vpe and there is no need to link vpe\n");
				} else {
					if ((member_idx != 0) && (check_in_out_unit_parm_p(venc.param, &venc.dev, p_out_param, p_out_dev) != HD_OK)) {
						goto err_exit;
					}
				}
				p_out_param = venc.param;
				p_out_dev = &venc.dev;
				if (osg_select[p_out_dev->device_subid][venc.dev.out_subid]) {
					enc_osg_offs = set_enc_osg_unit(p_group, line_idx, &venc, &len, entity_idx);
					if (enc_osg_offs > 0) {
						p_group->entity_buf_offs[line_idx][entity_idx++] = enc_osg_offs;
						p_group->entity_buf_len += len;
					} else if (enc_osg_offs < 0) {
						GMLIB_ERROR_P("set_enc_osg_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
						goto err_exit;
					}
					GMLIB_L2_P("set_enc_osg_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, enc_osg_offs, entity_idx);
				}
				venc_offs = set_venc_unit(p_group, line_idx, &venc, &len);
				if (venc_offs > 0) {
					p_group->entity_buf_offs[line_idx][entity_idx++] = venc_offs;
					p_group->entity_buf_len += len;
					BD_SET_BMP_INDEX(one_line->unit_bmp,
							 (codec_type == HD_CODEC_TYPE_JPEG ? HD_BIT_JPGE : HD_BIT_H26XE));
				} else if (venc_offs < 0) {
					GMLIB_ERROR_P("set_venc_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
					goto err_exit;
				}
				GMLIB_L2_P("set_venc_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, venc_offs, entity_idx);
			} else {
				//set flag for raw encode
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_RAWE);
			}
			dataout_offs = set_dataout_unit(p_group, line_idx, &venc, &len);
			if (dataout_offs > 0) {
				p_group->entity_buf_offs[line_idx][entity_idx++] = dataout_offs;
				p_group->entity_buf_len += len;
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_DATAOUT);
			} else if (dataout_offs < 0) {
				GMLIB_ERROR_P("set_dataout_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_dataout_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, dataout_offs, entity_idx);
			break;

		case HD_DAL_VIDEOOUT_BASE:
			vout.param = videoout_get_param_p(one_line->member[member_idx].p_bind->device.deviceid);
			dif_set_dev(&one_line->member[member_idx], &vout.dev, member_idx);

			if ((member_idx != 0) && (check_in_out_unit_parm_p(vout.param, &vout.dev, p_out_param, p_out_dev) != HD_OK)) {
				goto err_exit;
			}
			p_out_param = vout.param;
			p_out_dev = &vout.dev;

			dif_get_lcd_output_type(p_group, line_idx, &is_virtual_lcd);
			if (is_virtual_lcd) {
				break;
			}

			osg_offs = set_osg_unit(p_group, line_idx, &vout, &len);
			if (osg_offs > 0) {
				p_group->entity_buf_offs[line_idx][entity_idx++] = osg_offs;
				p_group->entity_buf_len += len;
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_OSG);
			} else if (osg_offs < 0) {
				GMLIB_ERROR_P("set_osg_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_osg_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, osg_offs, entity_idx);

			vout_offs = set_vout_unit(p_group, line_idx, &vout, &len);
			if (vout_offs > 0) {
				p_group->entity_buf_offs[line_idx][entity_idx++] = vout_offs;
				p_group->entity_buf_len += len;
				if (vout.dev.device_subid <= VPD_MAX_LCD_NUM) {
					BD_SET_BMP_INDEX(one_line->unit_bmp, (HD_BIT_LCD0 + vout.dev.device_subid));
				}
			} else if (vout_offs < 0) {
				GMLIB_ERROR_P("set_vout_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_vout_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, vout_offs, entity_idx);
			break;

		case HD_DAL_VIDEOPROC_BASE:
			vproc.param = videoproc_get_param_p(one_line->member[member_idx].p_bind->device.deviceid,
							    one_line->member[member_idx].out_subid);
			dif_set_dev(&one_line->member[member_idx], &vproc.dev, member_idx);

			if ((member_idx != 0) && (check_in_out_unit_parm_p(vproc.param, &vproc.dev, p_out_param, p_out_dev) != HD_OK)) {
				goto err_exit;
			}
			//check vpe prev entity is videoout,to bind vpe to osg that before vidoout
			/*
			* ->*osg_0_0_0
			*   ->lcd0vg_0_0_0
			*    >vpe536lib_0_0_4
			*/
			if ((member_idx != 0) && (bd_get_dev_baseid(one_line->member[member_idx - 1].p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE)) {
				//if (p_out_dev->device_baseid == HD_DAL_VIDEOOUT_BASE) {
				osg_offs = set_osg_unit(p_group, line_idx, &vout, &len);
				if (osg_offs > 0) {
					p_group->entity_buf_offs[line_idx][entity_idx++] = osg_offs;
					p_group->entity_buf_len += len;
					BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_OSG);
				} else if (osg_offs < 0) {
					GMLIB_ERROR_P("set_osg_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
					goto err_exit;
				}
				GMLIB_L2_P("set_osg_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, osg_offs, entity_idx);
			}
			/* vproc_out for keep next vproc out for 2 vpe connected. */
			memcpy(&vproc_out, &vproc, sizeof(vproc_out));
			p_out_param = vproc_out.param;
			p_out_dev = &vproc_out.dev;

			/* check vpe is first entity, then bind datain to vpe */
			if (member_idx == 0) {
				datain_offs = set_vproc_datain_unit(p_group, line_idx, &vproc, &len);
				if (datain_offs > 0) {
					p_group->entity_buf_offs[line_idx][entity_idx++] = datain_offs;
					p_group->entity_buf_len += len;
					BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_DATAIN);
				} else if (datain_offs < 0) {
					GMLIB_ERROR_P("set_datain_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
					goto err_exit;
				}
				GMLIB_L2_P("set_datain_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, datain_offs, entity_idx);
			}
			need_link_vpe = FALSE;
			vproc_offs = set_vproc_unit(p_group, line_idx, &vproc, &len, entity_idx);
			if (vproc_offs > 0) {
				need_link_vpe = TRUE;
				p_group->entity_buf_offs[line_idx][entity_idx++] = vproc_offs;
				p_group->entity_buf_len += len;
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_VPE);
			} else if (vproc_offs < 0) {
				GMLIB_ERROR_P("set_vproc_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_vproc_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, vproc_offs, entity_idx);

			dif_get_lcd_output_type(p_group, line_idx, &is_virtual_lcd);
			if (is_virtual_lcd) {

				osg_offs = set_vproc_osg_unit(p_group, line_idx, &vproc, &len);
				if (osg_offs > 0) {
					p_group->entity_buf_offs[line_idx][entity_idx++] = osg_offs;
					p_group->entity_buf_len += len;
					BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_OSG);
				} else if (osg_offs < 0) {
					GMLIB_ERROR_P("set_osg_unit fail! line_idx(%lu) member_idx(%d)\n", line_idx, member_idx);
					goto err_exit;
				}
				GMLIB_L2_P("set_osg_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, osg_offs, entity_idx);

				dataout_offs = set_vproc_dataout_unit(p_group, line_idx, &vproc, &len);
				if (dataout_offs > 0) {
					p_group->entity_buf_offs[line_idx][entity_idx++] = dataout_offs;
					p_group->entity_buf_len += len;
					BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_DATAOUT);
				} else if (dataout_offs < 0) {
					GMLIB_ERROR_P("set_dataout_unit fail! line_idx(%lu) member_idx(%d)\n", line_idx, member_idx);
					goto err_exit;
				}
				GMLIB_L2_P("set_dataout_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, dataout_offs, entity_idx);
			}
			break;

		case HD_DAL_AUDIOCAP_BASE:
			acap.param = audiocap_get_param_p(one_line->member[member_idx].p_bind->device.deviceid);
			dif_set_dev(&one_line->member[member_idx], &acap.dev, member_idx);

			if ((member_idx != 0) && (check_in_out_unit_parm_p(acap.param, &acap.dev, p_out_param, p_out_dev) != HD_OK)) {
				goto err_exit;
			}
			p_out_param = acap.param;
			p_out_dev = &acap.dev;

			acap_offs = set_acap_unit(p_group, line_idx, &acap, &len, member_idx);
			if (acap_offs > 0) {
				p_group->entity_buf_offs[line_idx][entity_idx++] = acap_offs;
				p_group->entity_buf_len += len;
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_ACAP);
			} else if (acap_offs < 0) {
				GMLIB_ERROR_P("acap_offs fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_acap_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, acap_offs, entity_idx);
			break;

		case HD_DAL_AUDIODEC_BASE:
			adec.param = audiodec_get_param_p(one_line->member[member_idx].p_bind->device.deviceid);
			dif_set_dev(&one_line->member[member_idx], &adec.dev, member_idx);

			if ((member_idx != 0) && (check_in_out_unit_parm_p(adec.param, &adec.dev, p_out_param, p_out_dev) != HD_OK)) {
				goto err_exit;
			}
			p_out_param = adec.param;
			p_out_dev = &adec.dev;

			adec_offs = set_adec_unit(p_group, line_idx, &adec, &len);
			if (adec_offs > 0) {
				p_group->entity_buf_offs[line_idx][entity_idx++] = adec_offs;
				p_group->entity_buf_len += len;
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_DATAIN);
			} else if (adec_offs < 0) {
				GMLIB_ERROR_P("set_adec_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_adec_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, adec_offs, entity_idx);
			break;

		case HD_DAL_AUDIOENC_BASE:
			aenc.param = audioenc_get_param_p(one_line->member[member_idx].p_bind->device.deviceid);
			dif_set_dev(&one_line->member[member_idx], &aenc.dev, member_idx);

			if ((member_idx != 0) && (check_in_out_unit_parm_p(aenc.param, &aenc.dev, p_out_param, p_out_dev) != HD_OK)) {
				goto err_exit;
			}
			p_out_param = aenc.param;
			p_out_dev = &aenc.dev;

			aenc_offs = set_aenc_unit(p_group, line_idx, &aenc, &len);
			if (aenc_offs > 0) {
				p_group->entity_buf_offs[line_idx][entity_idx++] = aenc_offs;
				p_group->entity_buf_len += len;
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_DATAOUT);
			} else if (aenc_offs < 0) {
				GMLIB_ERROR_P("set_aenc_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_aenc_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, aenc_offs, entity_idx);
			break;

		case HD_DAL_AUDIOOUT_BASE:
			aout.param = audioout_get_param_p(one_line->member[member_idx].p_bind->device.deviceid);
			dif_set_dev(&one_line->member[member_idx], &aout.dev, member_idx);

			if ((member_idx != 0) && (check_in_out_unit_parm_p(aout.param, &aout.dev, p_out_param, p_out_dev) != HD_OK)) {
				goto err_exit;
			}
			p_out_param = aout.param;
			p_out_dev = &aout.dev;

			aout_offs = set_aout_unit(p_group, line_idx, &aout, &len);
			if (aout_offs > 0) {
				p_group->entity_buf_offs[line_idx][entity_idx++] = aout_offs;
				p_group->entity_buf_len += len;
				BD_SET_BMP_INDEX(one_line->unit_bmp, HD_BIT_AOUT);
			} else if (aout_offs < 0) {
				GMLIB_ERROR_P("set_aout_unit fail! line_idx(%u) member_idx(%d)\n", line_idx, member_idx);
				goto err_exit;
			}
			GMLIB_L2_P("set_aout_unit len(%d) buf_len(%d) offs(%d) entity_idx(%d)\n", len, p_group->entity_buf_len, aout_offs, entity_idx);
			break;

		default:
			GMLIB_ERROR_P("not support deviceid(%d)\n", one_line->member[member_idx].p_bind->device.deviceid);
			break;
		}
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_process_audio_livesound(HD_GROUP *p_group, UINT32 line_idx)
{
	INT member_idx;
	INT acap_dev = -1, acap_ch = -1, aout_dev = -1;
	HD_GROUP_LINE *one_line;

	if (p_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("Exceed max graph line\n");
		goto err_exit;
	}

	one_line = &p_group->glines[line_idx];
	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {

		if (one_line->member[member_idx].p_bind == NULL) {
			continue;
		}
		switch (bd_get_dev_baseid(one_line->member[member_idx].p_bind->device.deviceid)) {
		case HD_DAL_AUDIOCAP_BASE:
			acap_dev = bd_get_dev_subid(one_line->member[member_idx].p_bind->device.deviceid);
			acap_ch = one_line->member[member_idx].out_subid;
			break;

		case HD_DAL_AUDIOOUT_BASE:
			aout_dev = bd_get_dev_subid(one_line->member[member_idx].p_bind->device.deviceid);
			break;

		default:
			break;
		}
	}

	/* livesound: no real grash, just call ioctl api */
	if (acap_dev != -1 && acap_ch != -1 && aout_dev != -1) {
		if ((one_line->state == HD_LINE_START_ONGOING) || (one_line->state == HD_LINE_START)) {
			// turn on livesound
			if (pif_au_livesound(acap_dev, acap_ch, aout_dev, 1) != 0) {
				GMLIB_ERROR_P("turn on livesound fail! line_idx(%u)\n", line_idx);
				goto err_exit;
			}
		} else if ((one_line->state == HD_LINE_STOP_ONGOING) || (one_line->state == HD_LINE_STOP)) {
			// turn off livesound
			if (pif_au_livesound(acap_dev, acap_ch, aout_dev, 0) != 0) {
				GMLIB_ERROR_P("turn off livesound fail! line_idx(%u)\n", line_idx);
				goto err_exit;
			}
		} else {
			GMLIB_ERROR_P("not support line state for livesound! line_idx(%u)\n", line_idx);
		}
	}
	return HD_OK;

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_check_duplicate_pathid(HD_PATH_ID *check_array, INT nr, HD_PATH_ID path_id)
{
	INT i;

	for (i = 0; i < nr; i++) {
		if (check_array[i] == path_id) {
			return HD_ERR_NG;
		}
	}

	for (i = 0; i < nr; i++) {
		if (check_array[i] == 0) {
			check_array[i] = path_id;
			return HD_OK;
		}
	}

	GMLIB_ERROR_P("dif_check_duplicate_pathid: exceed error! nr = %d\n", nr);
	return HD_ERR_NG;
}

HD_RESULT dif_audio_poll(HD_AUDIOENC_POLL_LIST *p_poll, UINT32 nr, INT32 wait_ms)
{
	UINT32 i, port_subid;
	gm_pollfd_t *gmpoll_fds;
	HD_BIND_INFO *p_bind = NULL;
	HD_IO out_id;
	HD_DAL deviceid;
	HD_RESULT ret = HD_OK;
	INT poll_ret;
	HD_PATH_ID check_duplicate[MAX_LIST_NUM];
	CHAR dev_string[64];

	if ((gmpoll_fds = DIF_CALLOC(sizeof(gm_pollfd_t) * nr, 1)) == NULL) {
		GMLIB_ERROR_P("alloc audio poll memory fail, size(%d).\n", sizeof(gm_pollfd_t) * nr);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	if (nr > MAX_LIST_NUM) {
		GMLIB_ERROR_P("dif_audio_poll: Exceed nr(%u) MAX_LIST_NUM(%d)\n", nr, MAX_LIST_NUM);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	memset(check_duplicate, 0x0, sizeof(HD_PATH_ID) * MAX_LIST_NUM);

	for (i = 0; i < nr; i++) {
		deviceid = HD_GET_DEV(p_poll[i].path_id);
		out_id = HD_GET_OUT(p_poll[i].path_id);
		if ((deviceid == 0) || ((p_bind = get_bind(deviceid)) == NULL)) {
			gmpoll_fds[i].group_line = NULL;
			continue;
		}
		if (dif_check_duplicate_pathid(check_duplicate, nr, p_poll[i].path_id) != HD_OK) {
			dif_get_dev_string(p_poll[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("dif_audio_poll: %s path_id(%#x) is duplicate.\n", dev_string, p_poll[i].path_id);
			ret = HD_ERR_NG;
			goto ext;
		}

		// for video_dec/video_enc/audio_dec/audio_enc, in/out port is 1-1-mapping
		port_subid = BD_GET_OUT_SUBID(out_id);
		gmpoll_fds[i].group_line = get_group_line_p(p_bind, NULL, &port_subid);
		gmpoll_fds[i].event = GM_POLL_READ;
		GMLIB_L3_P("%d_%d: path_id(%#x)\n", nr, i, p_poll[i].path_id);
	}

	/* poll */
	poll_ret = pif_poll(gmpoll_fds, nr, wait_ms);
	if (poll_ret == GM_TIMEOUT) {
		ret = HD_ERR_TIMEDOUT;
		goto ext;
	} else if (poll_ret < 0) {
		ret = HD_ERR_SYS;
		goto ext;
	}
	for (i = 0; i < nr; i++) {
		if (gmpoll_fds[i].revent.event != GM_POLL_READ) {
			p_poll[i].revent.event = FALSE;
			p_poll[i].revent.bs_size = 0;
			GMLIB_L3_P("%d_%d: path_id(%#x) not ready\n", nr, i, p_poll[i].path_id);
			continue;
		}
		p_poll[i].revent.event = TRUE;
		p_poll[i].revent.bs_size = gmpoll_fds[i].revent.bs_len;
		GMLIB_L3_P("%d_%d: path_id(%#x) ready\n", nr, i, p_poll[i].path_id);
	}

ext:
	if (gmpoll_fds != NULL)
		DIF_FREE(gmpoll_fds);
	return ret;
}

HD_RESULT dif_audio_recv_bs(HD_AUDIOENC_RECV_LIST *p_audioenc_bs, UINT32 nr)
{
	UINT32 i, port_subid;
	gm_enc_multi_bitstream_t *gmmulti_bs;
	HD_BIND_INFO *p_bind = NULL;
	HD_IO out_id;
	HD_DAL deviceid;
	HD_RESULT ret = HD_OK;
	INT recv_ret;
	HD_PATH_ID check_duplicate[MAX_LIST_NUM];
	CHAR dev_string[64];

	if ((gmmulti_bs = DIF_CALLOC(sizeof(gm_enc_multi_bitstream_t) * nr, 1)) == NULL) {
		GMLIB_ERROR_P("alloc audio recv bs memory fail, size(%d).\n",
			      sizeof(gm_enc_multi_bitstream_t) * nr);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	if (nr > MAX_LIST_NUM) {
		GMLIB_ERROR_P("dif_audio_recv_bs: Exceed nr(%u) MAX_LIST_NUM(%d)\n", nr, MAX_LIST_NUM);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	memset(check_duplicate, 0x0, sizeof(HD_PATH_ID) * MAX_LIST_NUM);

	for (i = 0; i < nr; i++) {
		deviceid = HD_GET_DEV(p_audioenc_bs[i].path_id);
		out_id = HD_GET_OUT(p_audioenc_bs[i].path_id);
		GMLIB_L3_P("%d_%d: path_id(%#x) buf(%#lx) buf_size(%d)\n", nr, i, p_audioenc_bs[i].path_id,
				(ULONG)p_audioenc_bs[i].user_bs.p_user_buf, p_audioenc_bs[i].user_bs.user_buf_size);
		if ((deviceid == 0) || (p_audioenc_bs[i].user_bs.p_user_buf == NULL) ||
		    ((p_bind = get_bind(deviceid)) == NULL)) {

			gmmulti_bs[i].group_line = NULL;
			gmmulti_bs[i].bs.bs_buf = NULL;
			gmmulti_bs[i].bs.bs_buf_len = 0;
			continue;
		}
		if (dif_check_duplicate_pathid(check_duplicate, nr, p_audioenc_bs[i].path_id) != HD_OK) {
			dif_get_dev_string(p_audioenc_bs[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("dif_audio_recv_bs: %s path_id(%#x) is duplicate.\n", dev_string, p_audioenc_bs[i].path_id);
			ret = HD_ERR_NG;
			goto ext;
		}

		// for audio_dec and audio_enc, in/out port is 1-1-mapping
		port_subid = BD_GET_OUT_SUBID(out_id);
		gmmulti_bs[i].group_line = get_group_line_p(p_bind, NULL, &port_subid);;
		gmmulti_bs[i].bs.bs_buf = p_audioenc_bs[i].user_bs.p_user_buf;         // set buffer point
		gmmulti_bs[i].bs.bs_buf_len = p_audioenc_bs[i].user_bs.user_buf_size;  // set buffer size
		gmmulti_bs[i].bs.extra_buf = 0;  // not to recevie MV data
		gmmulti_bs[i].bs.extra_buf_len = 0;  // not to recevie MV data
	}

	/* recv */
	if ((recv_ret = pif_recv_multi_bitstreams(gmmulti_bs, nr)) < 0) {
		ret = HD_ERR_NG;
		goto ext;
	}

	for (i = 0; i < nr; i++) {
		p_audioenc_bs[i].retval = gmmulti_bs[i].retval;
		if ((gmmulti_bs[i].retval < 0) && gmmulti_bs[i].group_line) {
			p_audioenc_bs[i].user_bs.size = gmmulti_bs[i].bs.bs_len;
			continue;
		} else {
			p_audioenc_bs[i].user_bs.size = gmmulti_bs[i].bs.bs_len;
			p_audioenc_bs[i].user_bs.p_user_buf = gmmulti_bs[i].bs.bs_buf;
			p_audioenc_bs[i].user_bs.timestamp = gmmulti_bs[i].bs.timestamp;
			p_audioenc_bs[i].user_bs.newbs_flag = gmmulti_bs[i].bs.newbs_flag;

			if (gmlib_dbg_level >= GMLIB_DBG_LEVEL_1 && gmmulti_bs[i].bs.bs_buf != NULL) {
				KFLOW_AUDIOIO_BS_FLOW_TIME *p_bs_flow_time = (KFLOW_AUDIOIO_BS_FLOW_TIME *)gmmulti_bs[i].bs.bs_buf;
				if (p_bs_flow_time->magic == KFLOW_AU_FLOW_TIME_MAGIC) {
					p_bs_flow_time->rx.lib_recv_time = pif_get_timestamp2();
				}
			}
		}
	}

ext:
	if (gmmulti_bs != NULL)
		DIF_FREE(gmmulti_bs);
	return ret;
}

HD_RESULT dif_audio_send_bs(HD_AUDIODEC_SEND_LIST *p_audiodec_bs, UINT32 nr, INT32 wait_ms)
{
	UINT32 i, port_subid;
	gm_dec_multi_bitstream_t *gmmulti_bs;
	HD_BIND_INFO *p_bind = NULL;
	HD_RESULT ret = HD_OK;
	INT send_ret;
	HD_IO out_id;
	HD_DAL deviceid;
	HD_PATH_ID check_duplicate[MAX_LIST_NUM];
	CHAR dev_string[64];

	if ((gmmulti_bs = DIF_CALLOC(sizeof(gm_dec_multi_bitstream_t) * nr, 1)) == NULL) {
		GMLIB_ERROR_P("alloc audio send bs memory fail, size(%d).\n",
			      sizeof(gm_dec_multi_bitstream_t) * nr);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	if (nr > MAX_LIST_NUM) {
		GMLIB_ERROR_P("dif_audio_send_bs: Exceed nr(%u) MAX_LIST_NUM(%d)\n", nr, MAX_LIST_NUM);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	memset(check_duplicate, 0x0, sizeof(HD_PATH_ID) * MAX_LIST_NUM);

	for (i = 0; i < nr; i++) {
		deviceid = HD_GET_DEV(p_audiodec_bs[i].path_id);
		out_id = HD_GET_OUT(p_audiodec_bs[i].path_id);
		if ((deviceid == 0) || (p_audiodec_bs[i].user_bs.p_user_buf == NULL) ||
		    ((p_bind = get_bind(deviceid)) == NULL)) {

			gmmulti_bs[i].group_line = NULL;
			gmmulti_bs[i].bs_buf = NULL;
			gmmulti_bs[i].bs_buf_len = 0;
			continue;
		}
		if (dif_check_duplicate_pathid(check_duplicate, nr, p_audiodec_bs[i].path_id) != HD_OK) {
			dif_get_dev_string(p_audiodec_bs[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("dif_audio_send_bs: %s path_id(%#x) is duplicate.\n", dev_string, p_audiodec_bs[i].path_id);
			ret = HD_ERR_NG;
			goto ext;
		}

		// for video_dec and video_enc, in/out port is 1-1-mapping
		port_subid = BD_GET_OUT_SUBID(out_id);
		gmmulti_bs[i].group_line = get_group_line_p(p_bind, NULL, &port_subid);
		gmmulti_bs[i].bs_buf = p_audiodec_bs[i].user_bs.p_user_buf;      // set buffer point
		gmmulti_bs[i].bs_buf_len = p_audiodec_bs[i].user_bs.user_buf_size;

		if (gmlib_dbg_level >= GMLIB_DBG_LEVEL_1) {
			KFLOW_AUDIOIO_BS_FLOW_TIME *p_bs_flow_time = (KFLOW_AUDIOIO_BS_FLOW_TIME *)gmmulti_bs[i].bs_buf;
			if (p_bs_flow_time->magic == KFLOW_AU_FLOW_TIME_MAGIC) {
				p_bs_flow_time->tx.lib_send_time = pif_get_timestamp2();
			}
		}
	}
	/* send */
	if ((send_ret = pif_send_multi_bitstreams(gmmulti_bs, nr, wait_ms)) < 0) {
		if (send_ret == -4) {
			ret = HD_ERR_TIMEDOUT;
		} else {
			ret = HD_ERR_NG;
		}
		goto ext;
	}

ext:
	if (gmmulti_bs != NULL)
		DIF_FREE(gmmulti_bs);
	return ret;
}

HD_RESULT dif_video_poll(HD_VIDEOENC_POLL_LIST *p_poll, UINT32 nr, INT32 wait_ms)
{
	UINT32 i, port_subid;
	gm_pollfd_t *gmpoll_fds;
	HD_BIND_INFO *p_bind = NULL;
	HD_IO out_id;
	HD_DAL deviceid;
	HD_RESULT ret = HD_OK;
	INT poll_ret;
	HD_PATH_ID check_duplicate[MAX_LIST_NUM];
	CHAR dev_string[64];

	if ((gmpoll_fds = DIF_CALLOC(sizeof(gm_pollfd_t) * nr, 1)) == NULL) {
		GMLIB_ERROR_P("alloc poll memory fail, size(%d).\n", sizeof(gm_pollfd_t) * nr);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	if (nr > MAX_LIST_NUM) {
		GMLIB_ERROR_P("dif_video_poll: Exceed nr(%u) MAX_LIST_NUM(%d)\n", nr, MAX_LIST_NUM);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	memset(check_duplicate, 0x0, sizeof(HD_PATH_ID) * MAX_LIST_NUM);

	for (i = 0; i < nr; i++) {
		deviceid = HD_GET_DEV(p_poll[i].path_id);
		out_id = HD_GET_OUT(p_poll[i].path_id);

		if ((deviceid == 0) || ((p_bind = get_bind(deviceid)) == NULL)) {
			gmpoll_fds[i].group_line = NULL;
			continue;
		}
		if (dif_check_duplicate_pathid(check_duplicate, nr, p_poll[i].path_id) != HD_OK) {
			dif_get_dev_string(p_poll[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("dif_video_poll: %s path_id(%#x) is duplicate.\n", dev_string, p_poll[i].path_id);
			ret = HD_ERR_NG;
			goto ext;
		}
		// for video_dec and video_enc, in/out port is 1-1-mapping
		port_subid = BD_GET_OUT_SUBID(out_id);
		gmpoll_fds[i].group_line = get_group_line_p(p_bind, NULL, &port_subid);
		gmpoll_fds[i].event = GM_POLL_READ;
		GMLIB_L3_P("%d_%d: path_id(%#x)\n", nr, i, p_poll[i].path_id);
	}

	/* poll */
	poll_ret = pif_poll(gmpoll_fds, nr, wait_ms);
	if (poll_ret == GM_TIMEOUT) {
		ret = HD_ERR_TIMEDOUT;
		goto ext;
	} else if (poll_ret < 0) {
		ret = HD_ERR_SYS;
		goto ext;
	}
	for (i = 0; i < nr; i++) {
		if (gmpoll_fds[i].revent.event != GM_POLL_READ) {
			p_poll[i].revent.event = FALSE;
			p_poll[i].revent.bs_size = 0;
			GMLIB_L3_P("%d_%d: path_id(%#x) not ready\n", nr, i, p_poll[i].path_id);
			continue;
		}
		GMLIB_L3_P("%d_%d: path_id(%#x) ready\n", nr, i, p_poll[i].path_id);
		p_poll[i].revent.event = TRUE;
		p_poll[i].revent.bs_size = gmpoll_fds[i].revent.bs_len;
		if (gmpoll_fds[i].revent.keyframe == 1) {
			p_poll[i].revent.frame_type = HD_FRAME_TYPE_I;
		} else {
			p_poll[i].revent.frame_type = HD_FRAME_TYPE_P;
		}
	}

ext:
	if (gmpoll_fds != NULL)
		DIF_FREE(gmpoll_fds);
	return ret;
}

HD_RESULT dif_video_recv_bs(HD_VIDEOENC_RECV_LIST *p_videoenc_bs, UINT32 nr)
{
	UINT32 i, j, port_subid, pack_count = 0;
	gm_enc_multi_bitstream_t *gmmulti_bs;
	HD_BIND_INFO *p_bind = NULL;
	HD_IO out_id;
	HD_DAL deviceid;
	HD_RESULT ret = HD_OK;
	INT recv_ret;
	gm_encode_type_t enc_type;
	HD_PATH_ID check_duplicate[MAX_LIST_NUM];
	CHAR dev_string[64];
	VDOENC_INTERNAL_PARAM *p_dev_cfg;

	if ((gmmulti_bs = DIF_CALLOC(sizeof(gm_enc_multi_bitstream_t) * nr, 1)) == NULL) {
		GMLIB_ERROR_P("alloc video recv bs memory fail, size(%d).\n",
			      sizeof(gm_enc_multi_bitstream_t) * nr);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	if (nr > MAX_LIST_NUM) {
		GMLIB_ERROR_P("dif_video_recv_bs: Exceed nr(%u) MAX_LIST_NUM(%d)\n", nr, MAX_LIST_NUM);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	memset(check_duplicate, 0x0, sizeof(HD_PATH_ID) * MAX_LIST_NUM);

	for (i = 0; i < nr; i++) {
		deviceid = HD_GET_DEV(p_videoenc_bs[i].path_id);
		out_id = HD_GET_OUT(p_videoenc_bs[i].path_id);
		GMLIB_L3_P("%d_%d: path_id(%#x) buf(%#lx) buf_size(%d)\n", nr, i, p_videoenc_bs[i].path_id,
					(ULONG)p_videoenc_bs[i].user_bs.p_user_buf, p_videoenc_bs[i].user_bs.user_buf_size);
		if ((deviceid == 0) || (p_videoenc_bs[i].user_bs.p_user_buf == NULL) ||
		    ((p_bind = get_bind(deviceid)) == NULL)) {
			gmmulti_bs[i].group_line = NULL;
			gmmulti_bs[i].bs.bs_buf = NULL;
			gmmulti_bs[i].bs.bs_buf_len = 0;
			continue;
		}
		if (dif_check_duplicate_pathid(check_duplicate, nr, p_videoenc_bs[i].path_id) != HD_OK) {
			dif_get_dev_string(p_videoenc_bs[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("dif_video_recv_bs: %s path_id(%#x) is duplicate.\n", dev_string, p_videoenc_bs[i].path_id);
			ret = HD_ERR_NG;
			goto ext;
		}
		port_subid = BD_GET_OUT_SUBID(out_id);
		p_dev_cfg = videoenc_get_param_p(HD_GET_DEV(p_videoenc_bs[i].path_id));
		if (p_dev_cfg->port[port_subid].get_bs_method == GET_BS_BY_PULL) {
			GMLIB_ERROR_P("recv_list operation can't be mixed with pull_out, pathid(%#x)\n", p_videoenc_bs[i].path_id);
			ret = HD_ERR_NG;
			goto ext;
		}
		p_dev_cfg->port[port_subid].get_bs_method = GET_BS_BY_RECV;
		// for video_dec and video_enc, in/out port is 1-1-mapping
		gmmulti_bs[i].group_line = get_group_line_p(p_bind, NULL, &port_subid);
		gmmulti_bs[i].bs.bs_buf = p_videoenc_bs[i].user_bs.p_user_buf;      // set buffer point
		gmmulti_bs[i].bs.bs_buf_len = p_videoenc_bs[i].user_bs.user_buf_size;  // set buffer length
		gmmulti_bs[i].bs.extra_buf = 0;  // not to recevie MV data
		gmmulti_bs[i].bs.extra_buf_len = 0;  // not to recevie MV data
	}

	/* recv */
	if ((recv_ret = pif_recv_multi_bitstreams(gmmulti_bs, nr)) < 0) {
		ret = HD_ERR_NG;
		goto ext;
	}

	for (i = 0; i < nr; i++) {
		p_videoenc_bs[i].retval = gmmulti_bs[i].retval;
		if ((gmmulti_bs[i].retval < 0) && gmmulti_bs[i].group_line) {
			p_videoenc_bs[i].user_bs.user_buf_size = gmmulti_bs[i].bs.bs_len;
			continue;
		} else {
			HD_VIDEOENC_USER_BS_DATA *p_video_pack;
			HD_VIDEOENC_USER_BS *p_user_bs = &p_videoenc_bs[i].user_bs;
			gm_enc_bitstream_t *p_bs = &gmmulti_bs[i].bs;
			enc_type = p_bs->bistream_type;
			//assert(enc_type != GM_ENCODE_TYPE_JPEG);

			p_user_bs->sign = MAKEFOURCC('V', 'S', 'T', 'M');
			p_user_bs->p_next = NULL;
			p_user_bs->timestamp = p_bs->timestamp;
			p_user_bs->frame_type = get_hdal_frame_type(p_bs->frame_type);
			p_user_bs->svc_layer_type = p_bs->svc_layer_type;

			//Only 3 possible combinations
			// - bs              (h264/h265)
			// - sps+pps+bs      (h264)
			// - vps+sps+pps_bs  (h265)
			// sequence: VPS -> SPS -> PPS -> slice
			if (p_bs->bs_offset == -1) {
				GMLIB_ERROR_P("No bs output.\n");
				ret = HD_ERR_NG;
				goto ext;
			} else {
				if ((p_bs->sps_offset == -1 && p_bs->pps_offset != -1) ||
				    (p_bs->sps_offset != -1 && p_bs->pps_offset == -1)) {
					GMLIB_ERROR_P("No sps(%d) and pps(%d) output.\n", p_bs->sps_offset, p_bs->pps_offset);
					ret = HD_ERR_NG;
					goto ext;
				} else {
				}
			}

			pack_count = 0;
			if (p_bs->vps_offset != -1) {
				p_video_pack = &p_user_bs->video_pack[pack_count];
				p_video_pack->pack_type.h265_type = H265_NALU_TYPE_VPS;
				p_video_pack->user_buf_addr = (UINTPTR) p_bs->bs_buf;
				p_video_pack->size = p_bs->sps_offset - p_bs->vps_offset;
				pack_count ++;
			}
			if (p_bs->sps_offset != -1) {
				p_video_pack = &p_user_bs->video_pack[pack_count];
				if (enc_type == GM_ENCODE_TYPE_H264)
					p_video_pack->pack_type.h264_type = H264_NALU_TYPE_SPS;
				else
					p_video_pack->pack_type.h265_type = H265_NALU_TYPE_SPS;
				p_video_pack->user_buf_addr = (UINTPTR) p_bs->bs_buf + p_bs->sps_offset;
				p_video_pack->size = p_bs->pps_offset - p_bs->sps_offset;
				pack_count ++;
			}
			if (p_bs->pps_offset != -1) {
				p_video_pack = &p_user_bs->video_pack[pack_count];
				if (enc_type == GM_ENCODE_TYPE_H264)
					p_video_pack->pack_type.h264_type = H264_NALU_TYPE_PPS;
				else
					p_video_pack->pack_type.h265_type = H265_NALU_TYPE_PPS;
				p_video_pack->user_buf_addr = (UINTPTR) p_bs->bs_buf + p_bs->pps_offset;
				p_video_pack->size = p_bs->bs_offset - p_bs->pps_offset;
				pack_count ++;
			}
			if (p_bs->bs_offset != -1) {
				p_video_pack = &p_user_bs->video_pack[pack_count];
				if (enc_type == GM_ENCODE_TYPE_H264) {
					if (p_user_bs->frame_type == HD_FRAME_TYPE_I) {
						p_video_pack->pack_type.h264_type = H264_NALU_TYPE_IDR;
					} else {
						p_video_pack->pack_type.h264_type = H264_NALU_TYPE_SLICE;
					}
				} else {
					if (p_user_bs->frame_type == HD_FRAME_TYPE_I) {
						p_video_pack->pack_type.h265_type = H265_NALU_TYPE_IDR;
					} else {
						p_video_pack->pack_type.h265_type = H265_NALU_TYPE_SLICE;
					}
				}
				p_video_pack->user_buf_addr = (UINTPTR) p_bs->bs_buf + p_bs->bs_offset;
				p_video_pack->size = p_bs->bs_len - p_bs->bs_offset;
				pack_count ++;
			}
			p_user_bs->pack_num = pack_count;

			p_user_bs->newbs_flag = p_bs->newbs_flag;
			for (j = 0; j < 4; j++) {
				p_user_bs->slice_offset[j] = p_bs->slice_offset[j];
			}
			if (enc_type == GM_ENCODE_TYPE_H264) {
				p_user_bs->vcodec_format = HD_CODEC_TYPE_H264;
				p_user_bs->blk_info.h264_info.intra_16x16_mb_num = p_bs->intra_16_mb_num;
				p_user_bs->blk_info.h264_info.intra_8x8_mb_Num = p_bs->intra_8_mb_num;
				p_user_bs->blk_info.h264_info.intra_4x4_mb_num = p_bs->intra_4_mb_num;
				p_user_bs->blk_info.h264_info.inter_mb_num = p_bs->inter_mb_num;
				p_user_bs->blk_info.h264_info.skip_mb_num = p_bs->skip_mb_num;
			} else if (enc_type == GM_ENCODE_TYPE_H265) {
				p_user_bs->vcodec_format = HD_CODEC_TYPE_H265;

				p_user_bs->blk_info.h265_info.intra_32x32_cu_num = p_bs->intra_32_cu_num;
				p_user_bs->blk_info.h265_info.intra_16x16_cu_num = p_bs->intra_16_cu_num;
				p_user_bs->blk_info.h265_info.intra_8x8_cu_num = p_bs->intra_8_cu_num;
				p_user_bs->blk_info.h265_info.inter_64x64_cu_num = p_bs->inter_64_cu_num;
				p_user_bs->blk_info.h265_info.inter_32x32_cu_num = p_bs->inter_32_cu_num;
				p_user_bs->blk_info.h265_info.inter_16x16_cu_num = p_bs->inter_16_cu_num;
				p_user_bs->blk_info.h265_info.skip_cu_num = p_bs->skip_cu_num;
				p_user_bs->blk_info.h265_info.merge_cu_num = p_bs->merge_cu_num;
			} else {
				p_user_bs->vcodec_format = HD_CODEC_TYPE_JPEG;
			}
			p_user_bs->psnr_info.y_mse = p_bs->psnr_y_mse;
			p_user_bs->psnr_info.u_mse = p_bs->psnr_u_mse;
			p_user_bs->psnr_info.v_mse = p_bs->psnr_v_mse;
			p_user_bs->qp = p_bs->qp_value;
			p_user_bs->evbr_state = (p_bs->evbr_state == 0) ? HD_EVBR_STATE_MOTION : HD_EVBR_STATE_STILL;
			p_user_bs->motion_ratio = p_bs->motion_ratio;
		}
	}

ext:
	if (gmmulti_bs != NULL)
		DIF_FREE(gmmulti_bs);
	return ret;
}

HD_RESULT check_video_send_bs(HD_VIDEODEC_SEND_LIST *p_videodec_bs, UINT32 nr)
{
	UINT32 i, port_subid, valid_nr = 0;
	HD_BIND_INFO *p_bind = NULL;
	HD_RESULT ret = HD_OK;
	HD_IO out_id;
	HD_DAL deviceid;
	HD_PATH_ID check_duplicate[MAX_LIST_NUM];
	CHAR dev_string[64];

	if (nr > MAX_LIST_NUM) {
		GMLIB_ERROR_P("dif_video_send_bs: Exceed nr(%u) MAX_LIST_NUM(%d)\n", nr, MAX_LIST_NUM);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	memset(check_duplicate, 0x0, sizeof(HD_PATH_ID) * MAX_LIST_NUM);
	for (i = 0; i < nr; i++) {
		deviceid = HD_GET_DEV(p_videodec_bs[i].path_id);
		out_id = HD_GET_OUT(p_videodec_bs[i].path_id);
		if ((deviceid == 0) || (p_videodec_bs[i].user_bs.p_bs_buf == NULL) ||
		    ((p_bind = get_bind(deviceid)) == NULL)) {

			continue;
		}
		if (dif_check_duplicate_pathid(check_duplicate, nr, p_videodec_bs[i].path_id) != HD_OK) {
			dif_get_dev_string(p_videodec_bs[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("dif_video_send_bs: %s path_id(%#x) is duplicate.\n", dev_string, p_videodec_bs[i].path_id);
			ret = HD_ERR_NG;
			goto ext;
		}
		port_subid = BD_GET_OUT_SUBID(out_id);
		if (get_group_line_p(p_bind, NULL, &port_subid) == NULL) {
			dif_get_dev_string(p_videodec_bs[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("dif_video_send_bs: %d %s path_id(%#x) is not ready. or it is invalid.\n",
				      i, dev_string, p_videodec_bs[i].path_id);
			ret = HD_ERR_PATH;
			goto ext;
		}
		valid_nr++;
	}
	if (valid_nr == 0) {
		GMLIB_ERROR_P("send bs fail, path_id list:\n");
		for (i = 0; i < nr; i++) {
			dif_get_dev_string(p_videodec_bs[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("  dif_video_send_bs: %s path_id(%#x) bs_buf_size(%d) p_bs_buf(%p)\n",
				      dev_string,
				      p_videodec_bs[i].path_id,
				      p_videodec_bs[i].user_bs.bs_buf_size,
				      p_videodec_bs[i].user_bs.p_bs_buf);
		}
		ret = HD_ERR_NG;
		goto ext;
	}

ext:
	return ret;
}

HD_RESULT dif_video_send_bs(HD_VIDEODEC_SEND_LIST *p_videodec_bs, UINT32 nr, INT32 wait_ms)
{
	UINT32 i, port_subid;
	gm_dec_multi_bitstream_t *gmmulti_bs;
	HD_BIND_INFO *p_bind = NULL;
	HD_RESULT ret = HD_OK;
	INT send_ret;
	HD_IO out_id;
	HD_DAL deviceid;
	HD_PATH_ID check_duplicate[MAX_LIST_NUM];
	CHAR dev_string[64];

	if ((gmmulti_bs = DIF_CALLOC(sizeof(gm_dec_multi_bitstream_t) * nr, 1)) == NULL) {
		GMLIB_ERROR_P("alloc video send bs memory fail, size(%d).\n", sizeof(gm_dec_multi_bitstream_t) * nr);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	if (nr > MAX_LIST_NUM) {
		GMLIB_ERROR_P("dif_video_send_bs: Exceed nr(%u) MAX_LIST_NUM(%d)\n", nr, MAX_LIST_NUM);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	memset(check_duplicate, 0x0, sizeof(HD_PATH_ID) * MAX_LIST_NUM);

	for (i = 0; i < nr; i++) {
		deviceid = HD_GET_DEV(p_videodec_bs[i].path_id);
		out_id = HD_GET_OUT(p_videodec_bs[i].path_id);

		if ((deviceid == 0) || (p_videodec_bs[i].user_bs.p_bs_buf == NULL) ||
		    ((p_bind = get_bind(deviceid)) == NULL)) {

			gmmulti_bs[i].group_line = NULL;
			gmmulti_bs[i].bs_buf = NULL;
			gmmulti_bs[i].bs_buf_len = 0;
			continue;
		}
		if (dif_check_duplicate_pathid(check_duplicate, nr, p_videodec_bs[i].path_id) != HD_OK) {
			dif_get_dev_string(p_videodec_bs[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("dif_video_send_bs: %s path_id(%#x) is duplicate.\n", dev_string, p_videodec_bs[i].path_id);
			ret = HD_ERR_NG;
			goto ext;
		}

		// for video_dec and video_enc, in/out port is 1-1-mapping
		port_subid = BD_GET_OUT_SUBID(out_id);
		gmmulti_bs[i].group_line = get_group_line_p(p_bind, NULL, &port_subid);
		if (gmmulti_bs[i].group_line == NULL) {
			dif_get_dev_string(p_videodec_bs[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("dif_video_send_bs: %d %s path_id(%#x) is not ready. or it is invalid.\n",
				      i, dev_string, p_videodec_bs[i].path_id);
			ret = HD_ERR_PATH;
			goto ext;
		}
		gmmulti_bs[i].bs_buf = p_videodec_bs[i].user_bs.p_bs_buf;      // set buffer point
		gmmulti_bs[i].bs_buf_len = p_videodec_bs[i].user_bs.bs_buf_size;
		gmmulti_bs[i].time_align = p_videodec_bs[i].user_bs.time_align;
		gmmulti_bs[i].time_diff = p_videodec_bs[i].user_bs.time_diff;
		gmmulti_bs[i].ap_timestamp = p_videodec_bs[i].user_bs.timestamp;
		gmmulti_bs[i].user_flag = p_videodec_bs[i].user_bs.user_flag;
		gmmulti_bs[i].reserved[1] = p_videodec_bs[i].user_bs.reserved[1]; // timestamp user flag
	}
	/* send */
	if ((send_ret = pif_send_multi_bitstreams(gmmulti_bs, nr, wait_ms)) < 0) {
		if (send_ret == -4) {
			ret = HD_ERR_TIMEDOUT;
		} else {
			GMLIB_L2_P("%s: send_bs fail(%d), HD_ERR_NG\n", __func__, send_ret);
			for (i = 0; i < nr; i++) {
				dif_get_dev_string(p_videodec_bs[i].path_id, dev_string, sizeof(dev_string));
				GMLIB_L2_P("  idx(%d) %s path_id(%#x) buf(%p) buf_size(%d), \n", i, dev_string, p_videodec_bs[i].path_id,
					   p_videodec_bs[i].user_bs.p_bs_buf, p_videodec_bs[i].user_bs.bs_buf_size);
			}
			ret = HD_ERR_NG;
		}
		goto ext;
	}

	for (i = 0; i < nr; i++) {
		p_videodec_bs[i].user_bs.reserved[0] = gmmulti_bs[i].reserved[0];
		GMLIB_L2_P("%s: i(%u) reserved[0] = %u, reserved[1] = %u\n", __func__, i, p_videodec_bs[i].user_bs.reserved[0], p_videodec_bs[i].user_bs.reserved[1]);
	}

ext:
	if (gmmulti_bs != NULL)
		DIF_FREE(gmmulti_bs);
	return ret;
}

HD_RESULT dif_video_send_yuv(HD_VIDEOPROC_SEND_LIST *p_videoproc_bs, UINT32 nr, INT32 wait_ms)
{
	UINT32 i, port_subid;
	gm_dec_multi_bitstream_t *gmmulti_bs;
	HD_BIND_INFO *p_bind = NULL;
	HD_RESULT ret = HD_OK;
	INT send_ret;
	HD_IO out_id;
	HD_DAL deviceid;
	HD_PATH_ID check_duplicate[MAX_LIST_NUM];
	CHAR dev_string[64];

	if ((gmmulti_bs = DIF_CALLOC(sizeof(gm_dec_multi_bitstream_t) * nr, 1)) == NULL) {
		GMLIB_ERROR_P("alloc video send bs memory fail, size(%d).\n", sizeof(gm_dec_multi_bitstream_t) * nr);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	if (nr > MAX_LIST_NUM) {
		GMLIB_ERROR_P("dif_video_send_bs: Exceed nr(%u) MAX_LIST_NUM(%d)\n", nr, MAX_LIST_NUM);
		ret = HD_ERR_NOMEM;
		goto ext;
	}
	memset(check_duplicate, 0x0, sizeof(HD_PATH_ID) * MAX_LIST_NUM);

	for (i = 0; i < nr; i++) {
		deviceid = HD_GET_DEV(p_videoproc_bs[i].path_id);
		out_id = HD_GET_OUT(p_videoproc_bs[i].path_id);

		if ((deviceid == 0) || (p_videoproc_bs[i].user_bs.p_bs_buf == NULL) ||
		    ((p_bind = get_bind(deviceid)) == NULL)) {

			gmmulti_bs[i].group_line = NULL;
			gmmulti_bs[i].bs_buf = NULL;
			gmmulti_bs[i].bs_buf_len = 0;
			continue;
		}
		if (dif_check_duplicate_pathid(check_duplicate, nr, p_videoproc_bs[i].path_id) != HD_OK) {
			dif_get_dev_string(p_videoproc_bs[i].path_id, dev_string, sizeof(dev_string));
			GMLIB_ERROR_P("dif_video_send_bs: %s path_id(%#x) is duplicate.\n", dev_string, p_videoproc_bs[i].path_id);
			ret = HD_ERR_NG;
			goto ext;
		}

		// for video_dec and video_enc, in/out port is 1-1-mapping
		port_subid = BD_GET_OUT_SUBID(out_id);
		gmmulti_bs[i].group_line = get_group_line_p(p_bind, NULL, &port_subid);
		gmmulti_bs[i].bs_buf = p_videoproc_bs[i].user_bs.p_bs_buf;      // set buffer point
		gmmulti_bs[i].bs_buf_len = p_videoproc_bs[i].user_bs.bs_buf_size;
		gmmulti_bs[i].time_align = p_videoproc_bs[i].user_bs.time_align;
		gmmulti_bs[i].time_diff = p_videoproc_bs[i].user_bs.time_diff;
		gmmulti_bs[i].ap_timestamp = p_videoproc_bs[i].user_bs.timestamp;
		gmmulti_bs[i].user_flag = p_videoproc_bs[i].user_bs.user_flag;
	}
	/* send */
	if ((send_ret = pif_send_multi_bitstreams(gmmulti_bs, nr, wait_ms)) < 0) {
		if (send_ret == -4) {
			ret = HD_ERR_TIMEDOUT;
		} else {
			GMLIB_L2_P("%s: send_bs fail(%d), HD_ERR_NG\n", __func__, send_ret);
			for (i = 0; i < nr; i++) {
				dif_get_dev_string(p_videoproc_bs[i].path_id, dev_string, sizeof(dev_string));
				GMLIB_L2_P("  idx(%d) %s path_id(%#x) buf(%p) buf_size(%d), \n", i, dev_string, p_videoproc_bs[i].path_id,
					   p_videoproc_bs[i].user_bs.p_bs_buf, p_videoproc_bs[i].user_bs.bs_buf_size);
			}
			ret = HD_ERR_NG;
		}
		goto ext;
	}

ext:
	if (gmmulti_bs != NULL)
		DIF_FREE(gmmulti_bs);
	return ret;
}

HD_RESULT apply_cap_crop(HD_DAL dev_id, HD_IO out_id)
{
	CAP_INTERNAL_PARAM *cap_dev_cfg;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	HD_VIDEOCAP_CROP *p_cap_crop;
	HD_BIND_INFO *p_bind = NULL;
	pif_group_t *group;
	vpd_entity_t *entity = NULL;
	vpd_cap_entity_t *cap_entity = NULL;
	INT entity_fd, entity_body_len, err_code = 0;
	UINT32 out_subid = VDOCAP_OUTID(out_id);
	HD_VIDEOCAP_OUT *p_cap_out;
	HD_DIM tmp_dim = {0, 0};
	HD_DIM crop_align_up_dim = {0, 0};
	HD_DIM crop_align_down_dim = {0, 0};

	/* check devices number and is open */
	cap_dev_cfg = videocap_get_param_p(dev_id);
	if (!cap_dev_cfg) {
		GMLIB_ERROR_P("cap_%d not bind\n", VDOCAP_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_SUPPORT;
		goto ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_ERROR_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NG;
		goto ext;
	}

	/* no flow case, set it OK and return */
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == NULL) {
		hd_ret = HD_OK;
		goto ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		hd_ret = HD_OK;
		goto ext;
	}
	group = pif_get_group_by_groupfd(group_line->groupidx);
	if (group == NULL) {
		hd_ret = HD_OK;
		goto ext;
	}

	/* find vpd entity by pathid */
	entity_fd = utl_get_cap_entity_fd(VDOCAP_DEVID(dev_id), out_subid, NULL);
	entity = pif_lookup_entity_buf(VPD_CAP_ENTITY_TYPE, entity_fd, group->encap_buf);
	if (entity == NULL) {
		GMLIB_L1_P("cannot find entity by fd(%#x), dev_id(%d) out_id(%d)\n", entity_fd, VDOCAP_DEVID(dev_id), out_subid);
		hd_ret = HD_ERR_TERM;
		goto ext;
	}

	p_cap_out = cap_dev_cfg->cap_out[VDOCAP_OUTID(out_id)];
	if (p_cap_out == NULL) {
		GMLIB_ERROR_P("<vcap out param is not set! dev_id(%d) >\n", dev_id);
		goto ext;
	}
	p_cap_crop = cap_dev_cfg->cap_crop[VDOCAP_OUTID(out_id)];
	cap_entity = (vpd_cap_entity_t *)entity->e;

	if (p_cap_crop && (get_vcap_crop_setting(p_cap_crop, out_subid, cap_entity) != HD_OK)) {
		GMLIB_ERROR_P("get_vcap_crop_setting fail! dev_id(%d)\n", dev_id);
		hd_ret = HD_ERR_NG;
		goto ext;
	}

	/* set dst_dim to next entity when crop enable */
	if (p_cap_crop && (p_cap_crop->mode == HD_CROP_ON)) {
		tmp_dim.w = cap_entity->crop_rect.w;
		tmp_dim.h = cap_entity->crop_rect.h;
		if (cap_entity->crop_rect.w > p_cap_out->dim.w) { //need scale down
			cap_entity->dst_rect.w = p_cap_out->dim.w;
			tmp_dim.w = p_cap_out->dim.w;
		}
		if (cap_entity->crop_rect.h > p_cap_out->dim.h) { //need scale down
			cap_entity->dst_rect.h = p_cap_out->dim.h;
			tmp_dim.h = p_cap_out->dim.h;
		}
	} else {
		tmp_dim.w = p_cap_out->dim.w;
		tmp_dim.h = p_cap_out->dim.h;
	}
	if (dif_get_align_down_dim(tmp_dim, p_cap_out->pxlfmt, &crop_align_down_dim) != HD_OK) {
		GMLIB_ERROR_P("<dif_get_align_down_dim fail >\n");
		goto ext;
	}

	/* dst_rect is final output dim */
	cap_entity->dst_rect.w = crop_align_down_dim.w;
	cap_entity->dst_rect.h = crop_align_down_dim.h;

	/* extend horizontal pixels  [15: 0]extend right pixels
								 [31:16]extend left pixels
	   extend vertical pixels 	 [7: 0]extend bottom lines
								 [15:8]extend top lines	 */
	if (dif_get_align_up_dim(tmp_dim, p_cap_out->pxlfmt, &crop_align_up_dim) != HD_OK) {
		GMLIB_ERROR_P("<dif_get_align_up_dim fail >\n");
		goto ext;
	}
	/* set ext_manual by final output dim */
	cap_entity->ext_ctrl.ext_manual_h = crop_align_up_dim.w - cap_entity->dst_rect.w;
	cap_entity->ext_ctrl.ext_manual_v = crop_align_up_dim.h - cap_entity->dst_rect.h;

	entity_body_len = sizeof(vpd_cap_entity_t);
	if ((err_code = pif_apply_entity(group_line, entity, entity_body_len)) < 0) {
		GMLIB_ERROR_P("pif_apply_crop fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto ext;
	}

ext:
	return hd_ret;
}

HD_RESULT apply_vpe_crop(HD_DAL dev_id, HD_IO out_id, INT param_id)
{
	DIF_VPROC_PARAM vproc;
	HD_RESULT hd_ret = HD_OK;
	HD_GROUP_LINE *group_line;
	HD_BIND_INFO *p_bind = NULL, *prev_bind = NULL;
	pif_group_t *group;
	vpd_entity_t *entity = NULL;
	vpd_vpe_1_entity_t *vpe_1_entity = NULL;
	INT entity_fd, entity_body_len, err_code, prev_is_vdec = 0;
	UINT32 out_subid = VDOPROC_OUTID(out_id);
	UINT32 in_subid = VDOPROC_INID(param_id);

	/* check devices number and is open */
	vproc.param = videoproc_get_param_p(dev_id, out_subid);
	if (!vproc.param) {
		GMLIB_ERROR_P("vpe_%d cfg not found\n", VDOPROC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_SUPPORT;
		goto ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_ERROR_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NG;
		goto ext;
	}

	/* no flow case, set it OK and return */
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		hd_ret = HD_OK;
		goto ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		hd_ret = HD_OK;
		goto ext;
	}
	group = pif_get_group_by_groupfd(group_line->groupidx);
	if (group == NULL) {
		hd_ret = HD_OK;
		goto ext;
	}
	prev_bind = get_prev_bind(p_bind, in_subid, NULL);
	if (prev_bind == NULL) {
		hd_ret = HD_OK;
		goto ext;
	}
	if (bd_get_dev_baseid(prev_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) {
		prev_is_vdec = 1;
	} else {
		prev_is_vdec = 0;
	}

	/* find vpd entity by pathid */
	entity_fd = utl_get_vpe_entity_fd(VDOPROC_DEVID(dev_id), out_subid, NULL);
	entity = pif_lookup_entity_buf(VPD_VPE_1_ENTITY_TYPE, entity_fd, group->encap_buf);
	if (entity == NULL) {
		GMLIB_L1_P("cannot find entity by fd(%#x), dev_id(%d)\n", entity_fd, VDOPROC_DEVID(dev_id));
		hd_ret = HD_ERR_TERM;
		goto ext;
	}

	vpe_1_entity = (vpd_vpe_1_entity_t *)entity->e;
	if (get_vproc_crop_setting(&vproc, vpe_1_entity, prev_is_vdec) != HD_OK) {
		GMLIB_ERROR_P("<get_vproc_crop_setting fail! dev_id(%d)\n", dev_id);
		hd_ret = HD_ERR_NG;
		goto ext;
	}

	entity_body_len = sizeof(vpd_vpe_1_entity_t);
	if ((err_code = pif_apply_entity(group_line, entity, entity_body_len)) < 0) {
		GMLIB_ERROR_P("pif_apply_crop fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto ext;
	}

ext:
	return hd_ret;
}

HD_RESULT apply_dec_sub_yuv(HD_DAL dev_id, HD_IO out_id)
{
	DIF_VDEC_PARAM vdec;
	HD_RESULT hd_ret = HD_OK;
	HD_GROUP_LINE *group_line;
	HD_BIND_INFO *p_bind = NULL;
	pif_group_t *group;
	vpd_entity_t *entity = NULL;
	vpd_dec_entity_t *dec_entity = NULL;
	INT entity_fd, entity_body_len, err_code;
	UINT32 out_subid = VDODEC_OUTID(out_id);

	/* check devices number and is open */
	vdec.param = videodec_get_param_p(dev_id, out_subid);
	if (!vdec.param) {
		GMLIB_ERROR_P("dec_%d cfg not found\n", VDODEC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_SUPPORT;
		goto ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_ERROR_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NG;
		goto ext;
	}

	/* no flow case, set it OK and return */
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		hd_ret = HD_OK;
		goto ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		hd_ret = HD_OK;
		goto ext;
	}
	group = pif_get_group_by_groupfd(group_line->groupidx);
	if (group == NULL) {
		hd_ret = HD_OK;
		goto ext;
	}

	/* find vpd entity by pathid */
	entity_fd = utl_get_dec_entity_fd(VDODEC_DEVID(dev_id), out_subid, NULL);
	entity = pif_lookup_entity_buf(VPD_DEC_ENTITY_TYPE, entity_fd, group->encap_buf);
	if (entity == NULL) {
		GMLIB_L1_P("cannot find entity by fd(%#x), dev_id(%d)\n", entity_fd, VDODEC_DEVID(dev_id));
		hd_ret = HD_ERR_TERM;
		goto ext;
	}

	dec_entity = (vpd_dec_entity_t *)entity->e;
	if (set_vdec_sub_yuv_setting(&vdec, dec_entity) != HD_OK) {
		GMLIB_ERROR_P("<get_vdec_sub_yuv_setting fail! dev_id(%d)\n", dev_id);
		hd_ret = HD_ERR_NG;
		goto ext;
	}

	entity_body_len = sizeof(vpd_dec_entity_t);
	if ((err_code = pif_apply_entity(group_line, entity, entity_body_len)) < 0) {
		GMLIB_ERROR_P("pif_apply_sub_yuv fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto ext;
	}

ext:
	return hd_ret;
}

HD_RESULT apply_enc_keyframe(HD_DAL dev_id, HD_IO out_id)
{
	VDOENC_INTERNAL_PARAM *p_dev_cfg;
	HD_RESULT hd_ret = HD_OK;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	INT err_code = 0;
	HD_H26XENC_REQUEST_IFRAME *p_request_keyframe;
	UINT32 outport_id = VDOENC_OUTID(out_id);

	/* check devices number and is open */
	p_dev_cfg = videoenc_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_ERROR_P("enc_%d cfg not found\n", VDOENC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_SUPPORT;
		goto ext;
	}
	p_request_keyframe = p_dev_cfg->port[outport_id].p_request_keyframe;
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_ERROR_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NG;
		goto ext;
	}

	/* no flow case, set it OK and return */
	group_line = get_group_line_p(p_bind, NULL, &outport_id);
	if (group_line == 0) {
		hd_ret = HD_OK;
		goto ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		GMLIB_ERROR_P("HD_LINE_INCOMPLETE\n");
		hd_ret = HD_OK;
		goto ext;
	}

	/* do pif_request_keyframe */
	if (p_request_keyframe->enable) {
		if ((err_code = pif_request_keyframe(group_line)) < 0) {
			GMLIB_ERROR_P("pif_request_keyframe fail, err_code=%x\n", err_code);
			hd_ret = HD_ERR_NG;
			goto ext;
		}
		p_request_keyframe->enable = 0;
	}

ext:
	return hd_ret;

}

HD_RESULT dif_apply_attr(HD_DAL dev_id, HD_IO out_id, INT param_id)
{

	HD_RESULT ret = HD_OK;

	switch (bd_get_dev_baseid(dev_id)) {
	case HD_DAL_VIDEOCAP_BASE:
		ret = apply_cap_crop(dev_id, out_id);
		break;
	case HD_DAL_VIDEOPROC_BASE:
		ret = apply_vpe_crop(dev_id, out_id, param_id);
		break;
	case HD_DAL_VIDEOENC_BASE:
		ret = apply_enc_keyframe(dev_id, out_id);
		break;
	case HD_DAL_VIDEODEC_BASE:
		ret = apply_dec_sub_yuv(dev_id, out_id);
		break;
	case HD_DAL_VIDEOOUT_BASE:
	case HD_DAL_AUDIODEC_BASE:
	case HD_DAL_AUDIOENC_BASE:
	case HD_DAL_AUDIOOUT_BASE:
	case HD_DAL_AUDIOCAP_BASE:
	default:
		GMLIB_ERROR_P("not support deviceid(%d)\n", dev_id);
		break;
	}

	return ret;
}

HD_RESULT vdec_pull_out(HD_DAL dev_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	VDODEC_INTL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	UINT32 out_subid = VDODEC_OUTID(out_id);
	gm_buffer_t frame_buffer = {0};
	gm_video_frame_info_t frame_info = {0};
	pif_get_buf_t get_buf_info = {0};
	HD_DIM out_dim = {0, 0};
	HD_DIM out_bg_dim = {0, 0}; // align by pxlfmt
	HD_VIDEO_PXLFMT out_pxlfmt = HD_VIDEO_PXLFMT_NONE;
	DIF_VDEC_PARAM vdec;

	p_dev_cfg = videodec_get_param_p(dev_id, out_subid);

	if (!p_dev_cfg) {
		GMLIB_L1_P("dec_%d cfg not found\n", out_subid);
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state != HD_LINE_START) {
		GMLIB_L2_P("flow mode is not available, go to user trigger\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->linefd = group_line->linefd;
	get_buf_info.linefd = group_line->linefd;
	get_buf_info.post_handling = 1;
	get_buf_info.wait_ms = wait_ms;

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_VDEC)) {
		get_buf_info.entity_type = VPD_DEC_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_dec_entity_fd(bd_get_dev_subid(dev_id), out_subid, NULL);
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_user_get_buffer(get_buf_info, &frame_buffer, &frame_info, NULL)) < 0) {
		GMLIB_ERROR_P("pif_user_get_buffer fail, err_code=%x\n", err_code);
		if (err_code == -15) {
			hd_ret = HD_ERR_TIMEDOUT;
		} else {
			hd_ret = HD_ERR_SYS;
		}
		goto err_ext;
	}
	out_dim.w = (UINT32)frame_info.dim.width;
	out_dim.h = (UINT32)frame_info.dim.height;
	out_bg_dim.w = (UINT32)frame_info.bg_dim.width;
	out_bg_dim.h = (UINT32)frame_info.bg_dim.height;
	out_pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(frame_info.format);
#if 0
	// align by codec
	vdec.param = videodec_get_param_p(dev_id, out_subid);
	if (vdec.param->max_mem->codec_type == HD_CODEC_TYPE_H265) {
		codec_bg_dim.w = ALIGN_CEIL(out_bg_dim.w, vdodec_hw_spec_info.h265.first_align.w);
		codec_bg_dim.h = ALIGN_CEIL(out_bg_dim.h, vdodec_hw_spec_info.h265.first_align.h);
	} else if (vdec.param->max_mem->codec_type == HD_CODEC_TYPE_H264) {
		codec_bg_dim.w = ALIGN_CEIL(out_bg_dim.w, vdodec_hw_spec_info.h264.first_align.w);
		codec_bg_dim.h = ALIGN_CEIL(out_bg_dim.h, vdodec_hw_spec_info.h264.first_align.h);
	} else if (vdec.param->max_mem->codec_type == HD_CODEC_TYPE_JPEG) {
		codec_bg_dim.w = ALIGN_CEIL(out_bg_dim.w, vdodec_hw_spec_info.jpeg.align.w);
		codec_bg_dim.h = ALIGN_CEIL(out_bg_dim.h, vdodec_hw_spec_info.jpeg.align.h);
	} else {
		HD_VIDEODEC_ERR("invalid codec_type: 0x%x. dev_id(%#x) out_id(%#x) out_subid(%#x)\n",
			vdec.param->max_mem->codec_type, dev_id, out_id, out_subid);
	}
#endif

	GMLIB_L2_P("vdec_pull_out: ddr(%d) pa(%p) timestamp(%llu) out_dim(%u,%u) out_bg_dim(%u,%u) pxlfmt(%#x)\n",
		   frame_buffer.ddr_id, frame_buffer.pa, frame_info.timestamp,
		   out_dim.w, out_dim.h, out_bg_dim.w, out_bg_dim.h, out_pxlfmt);

	if (p_video_frame) {
		vdec.param = videodec_get_param_p(dev_id, out_subid);
		p_video_frame->ddr_id = frame_buffer.ddr_id;
		p_video_frame->phy_addr[0] = (UINTPTR)frame_buffer.pa;
		// foreground dimension
		p_video_frame->dim.w = out_dim.w;
		p_video_frame->dim.h = out_dim.h;
		// background dimension
		p_video_frame->reserved[0] = out_bg_dim.w;
		p_video_frame->reserved[1] = out_bg_dim.h;
		// timestamp user flag
		p_video_frame->reserved[4] = (UINT32)frame_info.ts_user_flag;
		p_video_frame->pxlfmt = out_pxlfmt;
		p_video_frame->timestamp = (UINT64)frame_info.timestamp;
		if (vdec.param->max_mem->codec_type == HD_CODEC_TYPE_H265) {
			p_video_frame->sign = MAKEFOURCC('2', '6', '5', 'D');
		} else if (vdec.param->max_mem->codec_type == HD_CODEC_TYPE_H264) {
			p_video_frame->sign = MAKEFOURCC('2', '6', '4', 'D');
		} else if (vdec.param->max_mem->codec_type == HD_CODEC_TYPE_JPEG) {
			p_video_frame->sign = MAKEFOURCC('J', 'P', 'G', 'D');
		} else {
			HD_VIDEODEC_ERR("invalid codec_type: 0x%x. dev_id(%#x) out_id(%#x) out_subid(%#x)\n",
				vdec.param->max_mem->codec_type, dev_id, out_id, out_subid);
		}
	}

err_ext:
	return hd_ret;
}


HD_RESULT vcap_pull_out(HD_DAL dev_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	CAP_INTERNAL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	gm_buffer_t frame_buffer = {0};
	gm_video_frame_info_t frame_info = {0};
	pif_get_buf_t get_buf_info = {0};
	UINT32 out_subid = VDOCAP_OUTID(out_id);
	HD_DIM out_dim = {0, 0};
	HD_DIM out_bg_dim = {0, 0};
	HD_VIDEO_PXLFMT out_pxlfmt = HD_VIDEO_PXLFMT_NONE;

	p_dev_cfg = videocap_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_L1_P("cap_%d cfg not found\n", VDOCAP_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state != HD_LINE_START) {
		GMLIB_L2_P("flow mode is not available, go to user trigger\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->linefd[VDOCAP_OUTID(out_id)] = group_line->linefd;
	get_buf_info.linefd = group_line->linefd;
	get_buf_info.post_handling = 1;
	get_buf_info.wait_ms = wait_ms;

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_VCAP)) {
		get_buf_info.entity_type = VPD_CAP_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_cap_entity_fd(bd_get_dev_subid(dev_id), out_subid, NULL);
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_user_get_buffer(get_buf_info, &frame_buffer, &frame_info, NULL)) < 0) {
		GMLIB_ERROR_P("pif_user_get_buffer fail, err_code=%x\n", err_code);
		if (err_code == -15) {
			hd_ret = HD_ERR_TIMEDOUT;
		} else {
			hd_ret = HD_ERR_SYS;
		}
		goto err_ext;
	}

	out_dim.w = (UINT32)frame_info.dim.width;
	out_dim.h = (UINT32)frame_info.dim.height;
	out_pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(frame_info.format);
	if (dif_get_align_up_dim(out_dim, out_pxlfmt, &out_bg_dim) != HD_OK) {
		GMLIB_ERROR_P("vcap_pull_out: dif_get_align_up_dim fail\n");
		goto err_ext;
	}
	GMLIB_L2_P("vcap_pull_out: ddr(%d) pa(%p) timestamp(%llu) out_dim(%u,%u) out_bg_dim(%u,%u) pxlfmt(%#x)\n",
		   frame_buffer.ddr_id, frame_buffer.pa, frame_info.timestamp,
		   out_dim.w, out_dim.h, out_bg_dim.w, out_bg_dim.h, out_pxlfmt);

	if (p_video_frame) {
		p_video_frame->ddr_id = frame_buffer.ddr_id;
		p_video_frame->phy_addr[0] = (UINTPTR)frame_buffer.pa;
		p_video_frame->dim.w = out_bg_dim.w;
		p_video_frame->dim.h = out_bg_dim.h;
		p_video_frame->pxlfmt = out_pxlfmt;
		p_video_frame->timestamp = (UINT64)frame_info.timestamp;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vout_pull_out(HD_DAL dev_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	VDDO_INTERNAL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL, *prev_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	UINT32 in_subid = VO_INID(in_id);
	gm_buffer_t frame_buffer = {0};
	gm_video_frame_info_t frame_info = {0};
	pif_get_buf_t get_buf_info = {0};
	HD_DIM out_dim = {0, 0};
	HD_DIM out_bg_dim = {0, 0};
	HD_VIDEO_PXLFMT out_pxlfmt = HD_VIDEO_PXLFMT_NONE;
	gm_bs_info_t bs_info = {0};
	vpd_pull_out_bs *p_pull_out_bs = NULL;
	DIF_VPROC_PARAM vproc = {0};
	HD_QUERY_INFO vproc_subid = {0};
	HD_VIDEO_PXLFMT vproc_dst_fmt = HD_VIDEO_PXLFMT_NONE;
	HD_DIM vproc_dst_bg_dim = {0};

	p_dev_cfg = videoout_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_L1_P("vout_%d cfg not found\n", VO_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, &in_subid, NULL);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state != HD_LINE_START) {
		GMLIB_L2_P("flow mode is not available, go to user trigger\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->linefd = group_line->linefd;
	get_buf_info.linefd = group_line->linefd;
	get_buf_info.post_handling = 0;
	get_buf_info.wait_ms = wait_ms;

	if (VO_DEVID(dev_id) < MAX_VIDEOOUT_CTRL_ID) {
		if (BD_IS_BMP_INDEX(group_line->unit_bmp, (HD_BIT_LCD0 + bd_get_dev_subid(dev_id)))) {
			get_buf_info.entity_type = VPD_DISP_ENTITY_TYPE;
			get_buf_info.entity_fd = utl_get_lcd_entity_fd(0, bd_get_dev_subid(dev_id), in_subid, NULL);
		} else {
			GMLIB_L1_P("this unit driver is not exist\n");
			hd_ret = HD_ERR_NOT_AVAIL;
			goto err_ext;
		}

		if ((err_code = pif_user_get_buffer(get_buf_info, &frame_buffer, &frame_info, NULL)) < 0) {
			GMLIB_ERROR_P("pif_user_get_buffer fail, err_code=%x\n", err_code);
			if (err_code == -15) {
				hd_ret = HD_ERR_TIMEDOUT;
			} else {
				hd_ret = HD_ERR_SYS;
			}
			goto err_ext;
		}

		out_dim.w = (UINT32)frame_info.dim.width;
		out_dim.h = (UINT32)frame_info.dim.height;
		out_pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(frame_info.format);
		if (dif_get_align_up_dim(out_dim, out_pxlfmt, &out_bg_dim) != HD_OK) {
			GMLIB_ERROR_P("vout_pull_out: dif_get_align_up_dim fail\n");
			goto err_ext;
		}
		GMLIB_L2_P("vout_pull_out: ddr(%d) pa(%p) timestamp(%llu) out_dim(%u,%u) out_bg_dim(%u,%u) pxlfmt(%#x)\n",
			   frame_buffer.ddr_id, frame_buffer.pa, frame_info.timestamp,
			   out_dim.w, out_dim.h, out_bg_dim.w, out_bg_dim.h, out_pxlfmt);

		if (p_video_frame) {
			p_video_frame->ddr_id = frame_buffer.ddr_id;
			p_video_frame->phy_addr[0] = (UINTPTR)frame_buffer.pa;
			p_video_frame->dim.w = out_bg_dim.w;
			p_video_frame->dim.h = out_bg_dim.h;
			p_video_frame->pxlfmt = out_pxlfmt;
			p_video_frame->timestamp = (UINT64)frame_info.timestamp;
			}

	/* virtual LCD */
	} else {
		if ((p_pull_out_bs = DIF_MALLOC(sizeof(vpd_pull_out_bs))) == NULL) {
			GMLIB_L1_P("alloc pull out memory fail, size(%ld).\n", sizeof(vpd_pull_out_bs));
			hd_ret = HD_ERR_NOMEM;
			goto err_ext;
		}

		prev_bind = get_prev_bind(p_bind, in_subid, &vproc_subid);
		if (prev_bind == NULL) {
			hd_ret = HD_ERR_NOT_AVAIL;
			goto err_ext;
		}
		if (bd_get_dev_baseid(prev_bind->device.deviceid) == HD_DAL_VIDEOPROC_BASE) {
			vproc.param = videoproc_get_param_p(prev_bind->device.deviceid, vproc_subid.out_subid);
			if (vproc.param) {
				if (vproc.param->p_dst_fmt) {
					memcpy(&vproc_dst_fmt, vproc.param->p_dst_fmt, sizeof(HD_VIDEO_PXLFMT));
				}
				if (vproc.param->p_dst_bg_dim) {
					memcpy(&vproc_dst_bg_dim, vproc.param->p_dst_bg_dim, sizeof(HD_DIM));
				}
			} else {
				hd_ret = HD_ERR_NOT_AVAIL;
				goto err_ext;
			}
		}

		p_pull_out_bs->sign = (unsigned int)MAKEFOURCC('P','U','B','S');
		p_pull_out_bs->p_next = NULL;

		memcpy(&get_buf_info.vpdFdinfo[0], &group_line->vpdFdinfo[0], sizeof(vpd_channel_fd_t));
		if ((err_code = pif_pull_out_buffer(get_buf_info, &frame_buffer, &bs_info, &p_pull_out_bs->dout, wait_ms)) < 0) {
			GMLIB_ERROR_P("pif_pull_out_buffer fail, err_code=%x\n", err_code);
			hd_ret = HD_ERR_NG;
			goto err_ext;
		}

		GMLIB_L2_P("vout_pull_out: ddr(%d) pa(%p) timestamp(%llu) p_next(%p) dout(%p)  out_dim(%lu,%lu) out_bg_dim(%lu,%lu) pxlfmt(%#x)\n",
					frame_buffer.ddr_id, frame_buffer.pa, bs_info.timestamp, p_pull_out_bs, &p_pull_out_bs->dout,
					vproc_dst_bg_dim.w, vproc_dst_bg_dim.h, vproc_dst_fmt);

		if (p_video_frame) {
			p_video_frame->ddr_id = frame_buffer.ddr_id;
			p_video_frame->phy_addr[0] = (UINTPTR)frame_buffer.pa;
			p_video_frame->dim.w = vproc_dst_bg_dim.w;
			p_video_frame->dim.h = vproc_dst_bg_dim.h;
			p_video_frame->pxlfmt = vproc_dst_fmt;
			p_video_frame->timestamp = (UINT64)bs_info.timestamp;
			p_video_frame->p_next = (HD_METADATA *)p_pull_out_bs;
		}
	}
	return hd_ret;

err_ext:
	if (p_pull_out_bs) {
		DIF_FREE(p_pull_out_bs);
	}
	return hd_ret;
}

HD_RESULT vproc_pull_out(HD_DAL dev_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	VDOPROC_INTL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	UINT32 out_subid = VDOPROC_OUTID(out_id);
	gm_buffer_t frame_buffer = {0};
	gm_video_frame_info_t frame_info = {0};
	pif_get_buf_t get_buf_info = {0};
	HD_DIM out_dim = {0, 0};
	HD_DIM out_bg_dim = {0, 0};
	HD_VIDEO_PXLFMT out_pxlfmt = HD_VIDEO_PXLFMT_NONE;

	p_dev_cfg = videoproc_get_param_p(dev_id, out_subid);
	if (!p_dev_cfg) {
		GMLIB_L1_P("vproc_%d cfg not found\n", VDOPROC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state != HD_LINE_START) {
		GMLIB_L2_P("flow mode is not available, go to user trigger\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->linefd = group_line->linefd;
	get_buf_info.linefd = group_line->linefd;
	get_buf_info.post_handling = 1;
	get_buf_info.wait_ms = wait_ms;

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_VPE)) {
		get_buf_info.entity_type = VPD_VPE_1_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_vpe_entity_fd(bd_get_dev_subid(dev_id), out_subid, NULL);
	} else if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_DI)) {
		get_buf_info.entity_type = VPD_DI_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_di_entity_fd(bd_get_dev_subid(dev_id), out_subid, NULL);
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_user_get_buffer(get_buf_info, &frame_buffer, &frame_info, NULL)) < 0) {
		GMLIB_ERROR_P("pif_user_get_buffer fail, err_code=%x\n", err_code);
		if (err_code == -15) {
			hd_ret = HD_ERR_TIMEDOUT;
		} else {
			hd_ret = HD_ERR_SYS;
		}
		goto err_ext;
	}

	out_dim.w = (UINT32)frame_info.dim.width;
	out_dim.h = (UINT32)frame_info.dim.height;
	out_pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(frame_info.format);
	if (dif_get_align_up_dim(out_dim, out_pxlfmt, &out_bg_dim) != HD_OK) {
		GMLIB_ERROR_P("vout_pull_out: dif_get_align_up_dim fail\n");
		goto err_ext;
	}
	GMLIB_L2_P("vproc_pull_out: ddr(%d) pa(%p) timestamp(%llu) out_dim(%u,%u) out_bg_dim(%u,%u) pxlfmt(%#x)\n",
		   frame_buffer.ddr_id, frame_buffer.pa, frame_info.timestamp,
		   out_dim.w, out_dim.h, out_bg_dim.w, out_bg_dim.h, out_pxlfmt);

	if (p_video_frame) {
		p_video_frame->ddr_id = frame_buffer.ddr_id;
		p_video_frame->phy_addr[0] = (UINTPTR)frame_buffer.pa;
		p_video_frame->dim.w = out_bg_dim.w;
		p_video_frame->dim.h = out_bg_dim.h;
		p_video_frame->pxlfmt = out_pxlfmt;
		p_video_frame->timestamp = (UINT64)frame_info.timestamp;
	}

err_ext:
	return hd_ret;
}

HD_RESULT fill_videoenc_bs_info(HD_VIDEO_CODEC enc_type,
				gm_buffer_t *p_frame_buffer,
				gm_bs_info_t *p_bs_info,
				HD_VIDEOENC_BS *p_video_bitstream)
{
	HD_RESULT hd_ret = HD_OK;
	INT pack_count = 0;
	HD_VIDEO_BS_DATA *p_video_pack;

	if (p_bs_info == NULL) {
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	if (p_video_bitstream == NULL) {
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	p_video_bitstream->sign = MAKEFOURCC('V', 'S', 'T', 'M');
	p_video_bitstream->p_next = NULL;
	p_video_bitstream->vcodec_format = enc_type;
	p_video_bitstream->pack_num = 1;
	p_video_bitstream->frame_type = get_hdal_frame_type(p_bs_info->frame_type);
	p_video_bitstream->svc_layer_type = get_hdal_svc_layer_type(p_bs_info->svc_layer_type);
	p_video_bitstream->evbr_state = p_bs_info->evbr_state;
	p_video_bitstream->video_pack[0].pack_type.h264_type = H265_NALU_TYPE_SLICE;
	p_video_bitstream->video_pack[0].phy_addr = (UINTPTR)p_frame_buffer->pa;
	p_video_bitstream->video_pack[0].size = p_frame_buffer->size;
	p_video_bitstream->qp = p_bs_info->qp;
	p_video_bitstream->psnr_info.y_mse = p_bs_info->psnr_y_mse;
	p_video_bitstream->psnr_info.u_mse = p_bs_info->psnr_u_mse;
	p_video_bitstream->psnr_info.v_mse = p_bs_info->psnr_v_mse;
	p_video_bitstream->motion_ratio = p_bs_info->motion_ratio;
	p_video_bitstream->timestamp = (UINT64)p_bs_info->timestamp;

	if (enc_type == HD_CODEC_TYPE_JPEG) {
		p_video_pack = &p_video_bitstream->video_pack[0];
		p_video_pack->pack_type.jpeg_type = JPEG_PACK_TYPE_PIC;
		p_video_pack->phy_addr = (UINTPTR)p_frame_buffer->pa;
		p_video_pack->size = p_frame_buffer->size;
		p_video_bitstream->pack_num = 1;
	} else if ((enc_type == HD_CODEC_TYPE_H265) || (enc_type == HD_CODEC_TYPE_H264)) {
		//For H264/H265, only 3 possible combinations
		// - bs              (h264/h265)
		// - sps+pps+bs      (h264)
		// - vps+sps+pps_bs  (h265)
		// sequence: VPS -> SPS -> PPS -> slice
		if ((int)p_bs_info->bs_offset == -1) {
			HD_VIDEOENC_ERR("videoenc:No bs output.\n");
			hd_ret = HD_ERR_NG;
			goto exit;
		} else {
			if (((int)p_bs_info->sps_offset == -1 && (int)p_bs_info->pps_offset != -1) ||
			    ((int)p_bs_info->sps_offset != -1 && (int)p_bs_info->pps_offset == -1)) {
				HD_VIDEOENC_ERR("videoenc:No sps(%d) and pps(%d) output.\n", p_bs_info->sps_offset, p_bs_info->pps_offset);
				hd_ret = HD_ERR_NG;
				goto exit;
			} else {
			}
		}

		if ((int)p_bs_info->vps_offset != -1) {
			p_video_pack = &p_video_bitstream->video_pack[pack_count];
			p_video_pack->pack_type.h265_type = H265_NALU_TYPE_VPS;
			p_video_pack->phy_addr = (UINTPTR)p_frame_buffer->pa;
			p_video_pack->size = p_bs_info->sps_offset - p_bs_info->vps_offset;
			pack_count++;
		}
		if ((int)p_bs_info->sps_offset != -1) {
			p_video_pack = &p_video_bitstream->video_pack[pack_count];
			if (enc_type == HD_CODEC_TYPE_H264) {
				p_video_pack->pack_type.h264_type = H264_NALU_TYPE_SPS;
			} else {
				p_video_pack->pack_type.h265_type = H265_NALU_TYPE_SPS;
			}
			p_video_pack->phy_addr = (UINTPTR)p_frame_buffer->pa + p_bs_info->sps_offset;
			p_video_pack->size = p_bs_info->pps_offset - p_bs_info->sps_offset;
			pack_count++;
		}
		if ((int)p_bs_info->pps_offset != -1) {
			p_video_pack = &p_video_bitstream->video_pack[pack_count];
			if (enc_type == HD_CODEC_TYPE_H264) {
				p_video_pack->pack_type.h264_type = H264_NALU_TYPE_PPS;
			} else {
				p_video_pack->pack_type.h265_type = H265_NALU_TYPE_PPS;
			}
			p_video_pack->phy_addr = (UINTPTR)p_frame_buffer->pa + p_bs_info->pps_offset;
			p_video_pack->size = p_bs_info->bs_offset - p_bs_info->pps_offset;
			pack_count++;
		}
		if ((int)p_bs_info->bs_offset != -1) {
			p_video_pack = &p_video_bitstream->video_pack[pack_count];
			if (enc_type == HD_CODEC_TYPE_H264) {
				if (p_video_bitstream->frame_type == HD_FRAME_TYPE_I) {
					p_video_pack->pack_type.h264_type = H264_NALU_TYPE_IDR;
				} else {
					p_video_pack->pack_type.h264_type = H264_NALU_TYPE_SLICE;
				}
			} else {
				if (p_video_bitstream->frame_type == HD_FRAME_TYPE_I) {
					p_video_pack->pack_type.h265_type = H265_NALU_TYPE_IDR;
				} else {
					p_video_pack->pack_type.h265_type = H265_NALU_TYPE_SLICE;
				}
			}
			p_video_pack->phy_addr = (UINTPTR)p_frame_buffer->pa + p_bs_info->bs_offset;
			p_video_pack->size = p_frame_buffer->size - p_bs_info->bs_offset;
			pack_count++;
		}
		p_video_bitstream->pack_num = pack_count;

		if (enc_type == HD_CODEC_TYPE_H265) {
			p_video_bitstream->blk_info.h265_info.intra_32x32_cu_num = p_bs_info->intra_32_cu_num;
			p_video_bitstream->blk_info.h265_info.intra_16x16_cu_num = p_bs_info->intra_16_cu_num;
			p_video_bitstream->blk_info.h265_info.intra_8x8_cu_num = p_bs_info->intra_8_cu_num;
			p_video_bitstream->blk_info.h265_info.inter_64x64_cu_num = p_bs_info->inter_64_cu_num;
			p_video_bitstream->blk_info.h265_info.inter_32x32_cu_num = p_bs_info->inter_32_cu_num;
			p_video_bitstream->blk_info.h265_info.inter_16x16_cu_num = p_bs_info->inter_16_cu_num;
			p_video_bitstream->blk_info.h265_info.skip_cu_num = p_bs_info->skip_cu_num;
			p_video_bitstream->blk_info.h265_info.merge_cu_num = p_bs_info->merge_cu_num;
		} else if (enc_type == HD_CODEC_TYPE_H264) {
			p_video_bitstream->blk_info.h264_info.intra_16x16_mb_num = p_bs_info->intra_16_mb_num;
			p_video_bitstream->blk_info.h264_info.intra_8x8_mb_Num = p_bs_info->intra_8_mb_num;
			p_video_bitstream->blk_info.h264_info.intra_4x4_mb_num = p_bs_info->intra_4_mb_num;
			p_video_bitstream->blk_info.h264_info.inter_mb_num = p_bs_info->inter_mb_num;
			p_video_bitstream->blk_info.h264_info.skip_mb_num = p_bs_info->skip_mb_num;
		}
	}

exit:
	return hd_ret;
}

HD_RESULT venc_pull_out(HD_DAL dev_id, HD_IO in_id, HD_VIDEOENC_BS *p_video_bitstream, INT32 wait_ms)
{
	VDOENC_INTERNAL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	UINT32 in_subid = VDOENC_INID(in_id);
	gm_buffer_t frame_buffer = {0};
	gm_video_frame_info_t frame_info = {0};
	gm_bs_info_t bs_info = {0};
	pif_get_buf_t get_buf_info = {0};
	HD_VIDEO_CODEC enc_type;
	VOID *param = NULL;
	vpd_pull_out_bs *p_pull_out_bitstream = NULL;

	p_dev_cfg = videoenc_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_L1_P("venc_%d cfg not found\n", VDOENC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((p_pull_out_bitstream = DIF_MALLOC(sizeof(vpd_pull_out_bs))) == NULL) {
		GMLIB_L1_P("alloc pull out memory fail, size(%d).\n", sizeof(vpd_pull_out_bs));
		hd_ret = HD_ERR_NOMEM;
		goto err_ext;
	}

	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, &in_subid, NULL);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state != HD_LINE_START) {
		GMLIB_L2_P("flow mode is not available, go to user trigger\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->port[in_subid].linefd = group_line->linefd;
	get_buf_info.linefd = group_line->linefd;
	get_buf_info.post_handling = 1;
	memcpy(&get_buf_info.vpdFdinfo[0], &group_line->vpdFdinfo[0], sizeof(vpd_channel_fd_t));
	enc_type = p_dev_cfg->port[in_subid].p_enc_out_param->codec_type;

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_H26XE)) {
		param = (VOID *)0;
		get_buf_info.entity_type = VPD_H26X_ENC_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_enc_entity_fd(bd_get_dev_subid(dev_id), in_subid, param);
	} else if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_JPGE)) {
		param = (VOID *)1;
		get_buf_info.entity_type = VPD_MJPEGE_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_enc_entity_fd(bd_get_dev_subid(dev_id), in_subid, param);
	} else if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_RAWE)) {
		get_buf_info.entity_type = VPD_DOUT_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_dataout_entity_fd(bd_get_dev_subid(dev_id), in_subid, NULL);
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_pull_out_buffer(get_buf_info, &frame_buffer, &bs_info, &p_pull_out_bitstream->dout, wait_ms)) < 0) {
		GMLIB_ERROR_P("pif_pull_out_buffer fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

	GMLIB_L2_P("venc_pull_out: ddr(%d) pa(%p) size(%d) timestamp(%llu)\n",
		   frame_buffer.ddr_id, frame_buffer.pa, frame_buffer.size, frame_info.timestamp);

	if (p_video_bitstream) {
		p_video_bitstream->timestamp = (UINT64)frame_info.timestamp;
		hd_ret = fill_videoenc_bs_info(enc_type, &frame_buffer, &bs_info, p_video_bitstream);
		if (hd_ret != HD_OK) {
			GMLIB_ERROR_P("fill_videoenc_bs_info fail, hd_ret=%x\n", hd_ret);
			goto err_ext;
		}

		p_pull_out_bitstream->sign = (unsigned int)MAKEFOURCC('P', 'U', 'B', 'S');
		p_pull_out_bitstream->p_next = NULL;
		p_video_bitstream->p_next = (HD_METADATA *)p_pull_out_bitstream;
	}

	return HD_OK;

err_ext:
	if (p_pull_out_bitstream)
		DIF_FREE(p_pull_out_bitstream);

	return hd_ret;
}

HD_RESULT aenc_pull_out(HD_DAL dev_id, HD_IO in_id, HD_AUDIO_BS *p_audio_bitstream, INT32 wait_ms)
{
	AUDENC_INTERNAL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	UINT32 in_subid = AUDENC_INID(in_id);
	gm_buffer_t frame_buffer = {0};
	gm_video_frame_info_t frame_info = {0};
	pif_get_buf_t get_buf_info = {0};
	gm_bs_info_t bs_info = {0};
	vpd_pull_out_bs *p_pull_out_bitstream = NULL;

	p_dev_cfg = audioenc_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_L1_P("aenc_%d cfg not found\n", AUDENC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((p_pull_out_bitstream = DIF_MALLOC(sizeof(vpd_pull_out_bs))) == NULL) {
		GMLIB_L1_P("alloc pull out memory fail, size(%d).\n", sizeof(vpd_pull_out_bs));
		hd_ret = HD_ERR_NOMEM;
		goto err_ext;
	}

	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, &in_subid, NULL);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state != HD_LINE_START) {
		GMLIB_L2_P("flow mode is not available, go to user trigger\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->port[in_subid].linefd = group_line->linefd;
	get_buf_info.linefd = group_line->linefd;
	get_buf_info.post_handling = 1;
	memcpy(&get_buf_info.vpdFdinfo[0], &group_line->vpdFdinfo[0], sizeof(vpd_channel_fd_t));

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_ACAP)) {
		get_buf_info.entity_type = VPD_AU_GRAB_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_au_grab_entity_fd(0, bd_get_dev_subid(dev_id), in_subid, NULL);
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_pull_out_buffer(get_buf_info, &frame_buffer, &bs_info, &p_pull_out_bitstream->dout, wait_ms)) < 0) {
		GMLIB_ERROR_P("pif_pull_out_buffer fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

	GMLIB_L2_P("aenc_pull_out: ddr(%d) pa(%p) size(%d) timestamp(%llu)\n",
		   frame_buffer.ddr_id, frame_buffer.pa, frame_buffer.size, frame_info.timestamp);

	if (p_audio_bitstream) {
		p_audio_bitstream->phy_addr = (UINTPTR)frame_buffer.pa;
		p_audio_bitstream->size = (UINT32)frame_buffer.size;
		p_audio_bitstream->ddr_id = (UINT32)frame_buffer.ddr_id;
		p_audio_bitstream->timestamp = (UINT64)frame_info.timestamp;
		p_audio_bitstream->acodec_format = p_dev_cfg->port[in_subid].p_enc_out->codec_type;

		p_pull_out_bitstream->sign = (unsigned int)MAKEFOURCC('P', 'U', 'B', 'S');
		p_pull_out_bitstream->p_next = NULL;
		p_audio_bitstream->p_next = (HD_METADATA *)p_pull_out_bitstream;
	}

	return HD_OK;

err_ext:
	if (p_pull_out_bitstream)
		DIF_FREE(p_pull_out_bitstream);

	return hd_ret;
}

HD_RESULT acap_pull_out(HD_DAL dev_id, HD_IO out_id, HD_AUDIO_FRAME *p_audio_bitstream, INT32 wait_ms)
{
	AUDCAP_INTERNAL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	UINT32 out_subid = AUDCAP_OUTID(out_id);
	gm_buffer_t frame_buffer = {0};
	gm_video_frame_info_t frame_info = {0};
	pif_get_buf_t get_buf_info = {0};
	gm_bs_info_t bs_info = {0};
	vpd_pull_out_bs *p_pull_out_bitstream = NULL;

	p_dev_cfg = audiocap_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_L1_P("aenc_%d cfg not found\n", AUDCAP_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((p_pull_out_bitstream = DIF_MALLOC(sizeof(vpd_pull_out_bs))) == NULL) {
		GMLIB_L1_P("alloc pull out memory fail, size(%d).\n", sizeof(vpd_pull_out_bs));
		hd_ret = HD_ERR_NOMEM;
		goto err_ext;
	}

	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state != HD_LINE_START) {
		GMLIB_L2_P("flow mode is not available, go to user trigger\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	p_dev_cfg->port[AUDCAP_OUTID(out_id)].linefd = group_line->linefd;
	get_buf_info.linefd = group_line->linefd;
	get_buf_info.post_handling = 1;
	memcpy(&get_buf_info.vpdFdinfo[0], &group_line->vpdFdinfo[0], sizeof(vpd_channel_fd_t));

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_ACAP)) {
		get_buf_info.entity_type = VPD_AU_GRAB_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_au_grab_entity_fd(0, bd_get_dev_subid(dev_id), out_subid, NULL);
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_pull_out_buffer(get_buf_info, &frame_buffer, &bs_info, &p_pull_out_bitstream->dout, wait_ms)) < 0) {
		GMLIB_ERROR_P("pif_pull_out_buffer fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

	GMLIB_L2_P("acap_pull_out: ddr(%d) pa(%p) size(%d) timestamp(%llu)\n",
		   frame_buffer.ddr_id, frame_buffer.pa, frame_buffer.size, frame_info.timestamp);

	if (p_audio_bitstream) {
		p_audio_bitstream->phy_addr[0] = (UINTPTR)frame_buffer.pa;
		p_audio_bitstream->size = (UINT32)frame_buffer.size;
		p_audio_bitstream->ddr_id = (UINT32)frame_buffer.ddr_id;
		p_audio_bitstream->timestamp = (UINT64)frame_info.timestamp;

		p_pull_out_bitstream->sign = (unsigned int)MAKEFOURCC('P', 'U', 'B', 'S');
		p_pull_out_bitstream->p_next = NULL;
		p_audio_bitstream->p_next = (HD_METADATA *)p_pull_out_bitstream;
	}

	return HD_OK;

err_ext:
	if (p_pull_out_bitstream)
		DIF_FREE(p_pull_out_bitstream);

	return hd_ret;
}

HD_RESULT dif_pull_out_buffer(HD_DAL dev_id, HD_IO io_id, VOID *p_out_buffer, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;

	if (p_out_buffer == NULL) {
		GMLIB_ERROR_P("NULL p_out_buffer deviceid(%d)\n", dev_id);
		ret = HD_ERR_NOT_AVAIL;
		goto exit;
	}

	switch (bd_get_dev_baseid(dev_id)) {
	case HD_DAL_VIDEODEC_BASE:
		ret = vdec_pull_out(dev_id, io_id, (HD_VIDEO_FRAME *)p_out_buffer, wait_ms);
		break;
	case HD_DAL_VIDEOCAP_BASE:
		ret = vcap_pull_out(dev_id, io_id, (HD_VIDEO_FRAME *)p_out_buffer, wait_ms);
		break;
	case HD_DAL_VIDEOOUT_BASE:
		ret = vout_pull_out(dev_id, io_id, (HD_VIDEO_FRAME *)p_out_buffer, wait_ms);
		break;
	case HD_DAL_VIDEOPROC_BASE:
		ret = vproc_pull_out(dev_id, io_id, (HD_VIDEO_FRAME *)p_out_buffer, wait_ms);
		break;
	case HD_DAL_VIDEOENC_BASE:
		ret = venc_pull_out(dev_id, io_id, (HD_VIDEOENC_BS *)p_out_buffer, wait_ms);
		break;
	case HD_DAL_AUDIOENC_BASE:
		ret = aenc_pull_out(dev_id, io_id, (HD_AUDIO_BS *)p_out_buffer, wait_ms);
		break;
	case HD_DAL_AUDIOCAP_BASE:
		ret = acap_pull_out(dev_id, io_id, (HD_AUDIO_FRAME *)p_out_buffer, wait_ms);
		break;
	case HD_DAL_AUDIODEC_BASE:
		GMLIB_ERROR_P("need implement deviceid(%d)\n", dev_id);
		break;
	case HD_DAL_AUDIOOUT_BASE:
	default:
		GMLIB_ERROR_P("not support deviceid(%d)\n", dev_id);
		break;
	}

exit:
	return ret;
}

HD_RESULT vdec_release_out(HD_DAL dev_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame)
{
	VDODEC_INTL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	UINT32 out_subid = VDODEC_OUTID(out_id);
	gm_buffer_t frame_buffer;
	vpd_entity_type_t entity_type;

	p_dev_cfg = videodec_get_param_p(dev_id, out_subid);

	if (!p_dev_cfg) {
		GMLIB_L1_P("vdec_%d cfg not found\n", VDODEC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		GMLIB_L1_P("HD_LINE_INCOMPLETE\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->linefd = group_line->linefd;
	frame_buffer.ddr_id = p_video_frame->ddr_id;
	frame_buffer.pa = (void *)p_video_frame->phy_addr[0];

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_VDEC)) {
		entity_type = VPD_DEC_ENTITY_TYPE;
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_user_release_buffer(group_line->linefd, entity_type, &frame_buffer)) < 0) {
		GMLIB_ERROR_P("pif_user_release_buffer fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vcap_release_out(HD_DAL dev_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame)
{
	CAP_INTERNAL_PARAM *p_dev_cfg;
	HD_RESULT hd_ret = HD_OK;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	INT err_code = 0;
	UINT32 out_subid = VDOCAP_OUTID(out_id);
	gm_buffer_t frame_buffer;
	vpd_entity_type_t entity_type;

	p_dev_cfg = videocap_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_L1_P("vcap_%d cfg not found\n", VDOCAP_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		GMLIB_L1_P("HD_LINE_INCOMPLETE\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->linefd[out_subid] = group_line->linefd;

	frame_buffer.ddr_id = p_video_frame->ddr_id;
	frame_buffer.pa = (void *)p_video_frame->phy_addr[0];

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_VCAP)) {
		entity_type = VPD_CAP_ENTITY_TYPE;
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_user_release_buffer(group_line->linefd, entity_type, &frame_buffer)) < 0) {
		GMLIB_ERROR_P("pif_user_release_buffer fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT vout_release_out(HD_DAL dev_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame)
{
	VDDO_INTERNAL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	UINT32 in_subid = VO_INID(in_id);
	gm_buffer_t frame_buffer;
	vpd_entity_type_t entity_type;
	vpd_pull_out_bs *p_pull_out_bs = NULL;

	p_dev_cfg = videoout_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_L1_P("vout_%d cfg not found\n", VO_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, &in_subid, NULL);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		GMLIB_L1_P("HD_LINE_INCOMPLETE\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->linefd = group_line->linefd;

	frame_buffer.ddr_id = p_video_frame->ddr_id;
	frame_buffer.pa = (void *)p_video_frame->phy_addr[0];

	if (VO_DEVID(dev_id) < MAX_VIDEOOUT_CTRL_ID) {
		if (BD_IS_BMP_INDEX(group_line->unit_bmp, (HD_BIT_LCD0 + bd_get_dev_subid(dev_id)))) {
			entity_type = VPD_DISP_ENTITY_TYPE;
		} else {
			GMLIB_L1_P("this unit driver is not exist\n");
			hd_ret = HD_ERR_NOT_AVAIL;
			goto err_ext;
		}

		if ((err_code = pif_user_release_buffer(group_line->linefd, entity_type, &frame_buffer)) < 0) {
			GMLIB_ERROR_P("pif_user_release_buffer fail, err_code=%x\n", err_code);
			hd_ret = HD_ERR_NG;
			goto err_ext;
		}
	} else {
		entity_type = VPD_DOUT_ENTITY_TYPE;
		p_pull_out_bs = (vpd_pull_out_bs *)p_video_frame->p_next;
		if (p_pull_out_bs == NULL) {
			goto err_ext;
		}
		if ((err_code = pif_release_out_buffer(group_line->linefd, entity_type, &p_pull_out_bs->dout)) < 0) {
			GMLIB_ERROR_P("pif_release_out_buffer fail, err_code=%x\n", err_code);
			hd_ret = HD_ERR_NG;
			goto err_ext;
		}
		GMLIB_L2_P("vout_release_out: ddr(%d) pa(%p) p_next(%p) dout(%p)\n",
					frame_buffer.ddr_id, frame_buffer.pa, p_pull_out_bs, &p_pull_out_bs->dout);
	}

err_ext:
	if (p_pull_out_bs) {
		DIF_FREE(p_pull_out_bs);
	}
	return hd_ret;
}

HD_RESULT vproc_release_out(HD_DAL dev_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame)
{
	VDOPROC_INTL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	UINT32 out_subid = VDOPROC_OUTID(out_id);
	gm_buffer_t frame_buffer;
	vpd_entity_type_t entity_type;

	p_dev_cfg = videoproc_get_param_p(dev_id, out_subid);
	if (!p_dev_cfg) {
		GMLIB_L1_P("vproc_%d cfg not found\n", VDOPROC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		GMLIB_L1_P("HD_LINE_INCOMPLETE\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->linefd = group_line->linefd;

	frame_buffer.ddr_id = p_video_frame->ddr_id;
	frame_buffer.pa = (void *)p_video_frame->phy_addr[0];

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_VPE)) {
		entity_type = VPD_VPE_1_ENTITY_TYPE;
	} else if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_DI)) {
		entity_type = VPD_DI_ENTITY_TYPE;
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_user_release_buffer(group_line->linefd, entity_type, &frame_buffer)) < 0) {
		GMLIB_ERROR_P("pif_user_release_buffer fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT venc_release_out(HD_DAL dev_id, HD_IO in_id, HD_VIDEOENC_BS *p_video_bitstream)
{
	VDOENC_INTERNAL_PARAM *p_dev_cfg;
	HD_RESULT hd_ret = HD_OK;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	INT err_code = 0;
	UINT32 in_subid = VDOENC_INID(in_id);
	vpd_entity_type_t entity_type;
	vpd_pull_out_bs *p_pull_out_bs;

	if (!p_video_bitstream || !p_video_bitstream->p_next || p_video_bitstream->p_next->sign != MAKEFOURCC('P', 'U', 'B', 'S')) {
		GMLIB_L1_P("pull out stream data not avaliable\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_pull_out_bs = (vpd_pull_out_bs *) p_video_bitstream->p_next;

	p_dev_cfg = videoenc_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_L1_P("venc_%d cfg not found\n", VDOENC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &in_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		GMLIB_L1_P("HD_LINE_INCOMPLETE\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->port[in_subid].linefd = group_line->linefd;

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_H26XE)) {
		entity_type = VPD_H26X_ENC_ENTITY_TYPE;
	} else if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_JPGE)) {
		entity_type = VPD_MJPEGE_ENTITY_TYPE;
	} else if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_RAWE)) {
		entity_type = VPD_DOUT_ENTITY_TYPE;
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_release_out_buffer(group_line->linefd, entity_type, (vpd_get_copy_dout_t *)&p_pull_out_bs->dout)) < 0) {
		GMLIB_ERROR_P("pif_release_out_buffer fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

	// free allocated memory
	DIF_FREE(p_pull_out_bs);

err_ext:
	return hd_ret;
}

HD_RESULT aenc_release_out(HD_DAL dev_id, HD_IO in_id, HD_AUDIO_BS *p_audio_bitstream)
{
	AUDENC_INTERNAL_PARAM *p_dev_cfg;
	HD_RESULT hd_ret = HD_OK;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	INT err_code = 0;
	UINT32 in_subid = AUDENC_INID(in_id);
	vpd_pull_out_bs *p_pull_out_bs;

	if (!p_audio_bitstream || !p_audio_bitstream->p_next || p_audio_bitstream->p_next->sign != MAKEFOURCC('P', 'U', 'B', 'S')) {
		GMLIB_L1_P("pull out stream data not avaliable\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_pull_out_bs = (vpd_pull_out_bs *) p_audio_bitstream->p_next;

	p_dev_cfg = audioenc_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_L1_P("aenc_%d cfg not found\n", AUDENC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &in_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		GMLIB_L1_P("HD_LINE_INCOMPLETE\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->port[in_subid].linefd = group_line->linefd;

	if ((err_code = pif_release_out_buffer(group_line->linefd, VPD_AU_GRAB_ENTITY_TYPE, (vpd_get_copy_dout_t *)&p_pull_out_bs->dout)) < 0) {
		GMLIB_ERROR_P("pif_release_out_buffer fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

	// free allocated memory
	DIF_FREE(p_pull_out_bs);

err_ext:
	return hd_ret;
}

HD_RESULT acap_release_out(HD_DAL dev_id, HD_IO out_id, HD_AUDIO_FRAME *p_audio_bitstream)
{
	AUDCAP_INTERNAL_PARAM *p_dev_cfg;
	HD_RESULT hd_ret = HD_OK;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	INT err_code = 0;
	UINT32 out_subid = AUDCAP_OUTID(out_id);
	vpd_pull_out_bs *p_pull_out_bs;

	if (!p_audio_bitstream || !p_audio_bitstream->p_next || p_audio_bitstream->p_next->sign != MAKEFOURCC('P', 'U', 'B', 'S')) {
		GMLIB_L1_P("pull out stream data not avaliable\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_pull_out_bs = (vpd_pull_out_bs *) p_audio_bitstream->p_next;

	p_dev_cfg = audiocap_get_param_p(dev_id);
	if (!p_dev_cfg) {
		GMLIB_L1_P("aenc_%d cfg not found\n", AUDENC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		GMLIB_L1_P("HD_LINE_INCOMPLETE\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->port[out_subid].linefd = group_line->linefd;

	if ((err_code = pif_release_out_buffer(group_line->linefd, VPD_AU_GRAB_ENTITY_TYPE, (vpd_get_copy_dout_t *)&p_pull_out_bs->dout)) < 0) {
		GMLIB_ERROR_P("pif_release_out_buffer fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

	// free allocated memory
	DIF_FREE(p_pull_out_bs);

err_ext:
	return hd_ret;
}

HD_RESULT dif_release_out_buffer(HD_DAL dev_id, HD_IO io_id, VOID *p_out_buffer)
{
	HD_RESULT ret = HD_OK;

	if (p_out_buffer == NULL) {
		GMLIB_ERROR_P("NULL p_out_buffer deviceid(%d)\n", dev_id);
		ret = HD_ERR_NOT_AVAIL;
		goto exit;
	}

	switch (bd_get_dev_baseid(dev_id)) {
	case HD_DAL_VIDEODEC_BASE:
		ret = vdec_release_out(dev_id, io_id, (HD_VIDEO_FRAME *)p_out_buffer);
		break;
	case HD_DAL_VIDEOCAP_BASE:
		ret = vcap_release_out(dev_id, io_id, (HD_VIDEO_FRAME *)p_out_buffer);
		break;
	case HD_DAL_VIDEOOUT_BASE:
		ret = vout_release_out(dev_id, io_id, (HD_VIDEO_FRAME *)p_out_buffer);
		break;
	case HD_DAL_VIDEOPROC_BASE:
		ret = vproc_release_out(dev_id, io_id, (HD_VIDEO_FRAME *)p_out_buffer);
		break;
	case HD_DAL_VIDEOENC_BASE:
		ret = venc_release_out(dev_id, io_id, (HD_VIDEOENC_BS *)p_out_buffer);
		break;
	case HD_DAL_AUDIOENC_BASE:
		ret = aenc_release_out(dev_id, io_id, (HD_AUDIO_BS *)p_out_buffer);
		break;
	case HD_DAL_AUDIOCAP_BASE:
		ret = acap_release_out(dev_id, io_id, (HD_AUDIO_FRAME *)p_out_buffer);
		break;
	case HD_DAL_AUDIODEC_BASE:
		GMLIB_ERROR_P("need implement deviceid(%d)\n", dev_id);
		break;
	case HD_DAL_AUDIOOUT_BASE:
	default:
		GMLIB_ERROR_P("not support deviceid(%d)\n", dev_id);
		break;
	}

exit:
	return ret;
}

void fill_h26x_param_from_hdal(h26x_param_set  *p_hdal_params, hdal_videoenc_setting_t *h26x_enc_setting,
			       HD_VIDEO_PXLFMT pxl_fmt, dim_t src_dim)
{
	p_hdal_params->ver = VIDEOENC_PARAM_H_VER;

	if (!h26x_enc_setting->is_enc_in_param || !h26x_enc_setting->is_enc_out_param) {
		GMLIB_PRINT_P("%s: enc_in(%d) enc_out(%d) params are required.\n", __func__,
			      h26x_enc_setting->is_enc_in_param, h26x_enc_setting->is_enc_out_param);
		return;
	}

	p_hdal_params->update_flag.update_mask = 0;

	if (h26x_enc_setting->is_enc_in_param) {
		VG_ENC_BUF_PARAM  *p_enc_buf_param = &p_hdal_params->enc_buf_param;

		p_hdal_params->update_flag.item.vg_enc_buf_param = 1;
		if (HD_VIDEO_PXLFMT_YUV420_NVX3 /*TYPE_YUV420_SCE*/ == pxl_fmt) {
			p_enc_buf_param->source_y_line_offset = ALIGN16_UP(ALIGN32_DOWN(src_dim.w) * 3 / 4);
			p_enc_buf_param->source_c_line_offset = ALIGN16_UP(ALIGN32_DOWN(src_dim.w) * 3 / 4);
			p_enc_buf_param->source_chroma_offset = p_enc_buf_param->source_y_line_offset * src_dim.h;
		} else { // if (HD_VIDEO_PXLFMT_YUV420 == pxl_fmt)
			p_enc_buf_param->source_y_line_offset = src_dim.w;
			p_enc_buf_param->source_c_line_offset = src_dim.w;
			p_enc_buf_param->source_chroma_offset = p_enc_buf_param->source_y_line_offset * src_dim.h;
		}
		GMLIB_L1_P("set h26x offset(%u,%u,%u) fmt(%#x)\n", p_enc_buf_param->source_y_line_offset,
			   p_enc_buf_param->source_c_line_offset, p_enc_buf_param->source_chroma_offset, pxl_fmt);
	}

	if (h26x_enc_setting->is_enc_out_param) {
		HD_H26X_CONFIG *p_hdal_param = &h26x_enc_setting->enc_out_param.h26x;
		VG_H26XENC_FEATURE *p_vg_param = &p_hdal_params->enc_feature;
		HD_VIDEO_DIR  dir = h26x_enc_setting->enc_in_param.dir;

		p_hdal_params->codec_type = h26x_enc_setting->enc_out_param.codec_type;

		p_hdal_params->update_flag.item.vg_h26xenc_feature = 1;
		p_vg_param->gop_num = p_hdal_param->gop_num;
		p_vg_param->chrm_qp_idx = p_hdal_param->chrm_qp_idx;
		p_vg_param->sec_chrm_qp_idx = p_hdal_param->sec_chrm_qp_idx;
		p_vg_param->ltr_interval = p_hdal_param->ltr_interval;
		p_vg_param->ltr_pre_ref = p_hdal_param->ltr_pre_ref;
		p_vg_param->gray_en = p_hdal_param->gray_en;
		switch (dir) {
		case HD_VIDEO_DIR_ROTATE_90:
			p_vg_param->rotate = 2;
			break;
		case HD_VIDEO_DIR_ROTATE_180:
			p_vg_param->rotate = 3;
			break;
		case HD_VIDEO_DIR_ROTATE_270:
			p_vg_param->rotate = 1;
			break;
		case HD_VIDEO_DIR_ROTATE_0:
		default:
			p_vg_param->rotate = 0;
			break;
		}
		p_vg_param->source_output = p_hdal_param->source_output;
		p_vg_param->profile = p_hdal_param->profile;
		p_vg_param->level_idc = p_hdal_param->level_idc;
		p_vg_param->svc_layer = p_hdal_param->svc_layer;
		p_vg_param->entropy_mode = p_hdal_param->entropy_mode;
	}

	if (h26x_enc_setting->is_enc_vui) {
		HD_H26XENC_VUI *p_hdal_param = &h26x_enc_setting->enc_vui;
		VG_H26XENC_VUI *p_vg_param = &p_hdal_params->vui;

		p_hdal_params->update_flag.item.vg_h26xenc_vui = 1;
		p_vg_param->vui_en = p_hdal_param->vui_en;
		p_vg_param->sar_width = p_hdal_param->sar_width;
		p_vg_param->sar_height = p_hdal_param->sar_height;
		p_vg_param->matrix_coef = p_hdal_param->matrix_coef;
		p_vg_param->transfer_characteristics = p_hdal_param->transfer_characteristics;
		p_vg_param->colour_primaries = p_hdal_param->colour_primaries;
		p_vg_param->video_format = p_hdal_param->video_format;
		p_vg_param->color_range = p_hdal_param->color_range;
		p_vg_param->timing_present_flag = p_hdal_param->timing_present_flag;
	}

	if (h26x_enc_setting->is_enc_deblock) {
		HD_H26XENC_DEBLOCK *p_hdal_param = &h26x_enc_setting->enc_deblock;
		VG_H26XENC_DEBLOCK *p_vg_param = &p_hdal_params->deblock;

		p_hdal_params->update_flag.item.vg_h26xenc_deblock = 1;
		p_vg_param->dis_ilf_idc = p_hdal_param->dis_ilf_idc;
		p_vg_param->db_alpha = p_hdal_param->db_alpha;
		p_vg_param->db_beta = p_hdal_param->db_beta;
	}

	if (h26x_enc_setting->is_enc_rc) {
		HD_H26XENC_RATE_CONTROL *p_hdal_param = &h26x_enc_setting->enc_rc;
		VG_H26XENC_RATE_CONTROL *p_vg_param = &p_hdal_params->rate_control;

		p_hdal_params->update_flag.item.vg_h26xenc_rate_control = 1;
		p_vg_param->rc_mode = p_hdal_param->rc_mode;
		if (p_hdal_param->rc_mode == HD_RC_MODE_CBR) {
			HD_H26XENC_CBR  *p_hdal_cbr = &p_hdal_param->cbr;
			VG_H26XENC_CBR  *p_vg_cbr = &p_vg_param->rc_param.cbr;
			p_vg_cbr->bitrate = p_hdal_cbr->bitrate;
			p_vg_cbr->frame_rate_base = p_hdal_cbr->frame_rate_base;
			p_vg_cbr->frame_rate_incr = p_hdal_cbr->frame_rate_incr;
			p_vg_cbr->init_i_qp = p_hdal_cbr->init_i_qp;
			p_vg_cbr->max_i_qp = p_hdal_cbr->max_i_qp;
			p_vg_cbr->min_i_qp = p_hdal_cbr->min_i_qp;
			p_vg_cbr->init_p_qp = p_hdal_cbr->init_p_qp;
			p_vg_cbr->max_p_qp = p_hdal_cbr->max_p_qp;
			p_vg_cbr->min_p_qp = p_hdal_cbr->min_p_qp;
			p_vg_cbr->static_time = p_hdal_cbr->static_time;
			p_vg_cbr->ip_weight = p_hdal_cbr->ip_weight;
		} else if (p_hdal_param->rc_mode == HD_RC_MODE_VBR) {
			HD_H26XENC_VBR  *p_hdal_vbr = &p_hdal_param->vbr;
			VG_H26XENC_VBR  *p_vg_vbr = &p_vg_param->rc_param.vbr;
			p_vg_vbr->bitrate = p_hdal_vbr->bitrate;
			p_vg_vbr->frame_rate_base = p_hdal_vbr->frame_rate_base;
			p_vg_vbr->frame_rate_incr = p_hdal_vbr->frame_rate_incr;
			p_vg_vbr->init_i_qp = p_hdal_vbr->init_i_qp;
			p_vg_vbr->max_i_qp = p_hdal_vbr->max_i_qp;
			p_vg_vbr->min_i_qp = p_hdal_vbr->min_i_qp;
			p_vg_vbr->init_p_qp = p_hdal_vbr->init_p_qp;
			p_vg_vbr->max_p_qp = p_hdal_vbr->max_p_qp;
			p_vg_vbr->min_p_qp = p_hdal_vbr->min_p_qp;
			p_vg_vbr->static_time = p_hdal_vbr->static_time;
			p_vg_vbr->ip_weight = p_hdal_vbr->ip_weight;
			p_vg_vbr->change_pos = p_hdal_vbr->change_pos;
		} else if (p_hdal_param->rc_mode == HD_RC_MODE_FIX_QP) {
			HD_H26XENC_FIXQP  *p_hdal_fixqp = &p_hdal_param->fixqp;
			VG_H26XENC_FIXQP  *p_vg_fixqp = &p_vg_param->rc_param.fixqp;
			p_vg_fixqp->frame_rate_base = p_hdal_fixqp->frame_rate_base;
			p_vg_fixqp->frame_rate_incr = p_hdal_fixqp->frame_rate_incr;
			p_vg_fixqp->fix_i_qp = p_hdal_fixqp->fix_i_qp;
			p_vg_fixqp->fix_p_qp = p_hdal_fixqp->fix_p_qp;
		} else { /* if (p_hdal_param->rc_mode == HD_RC_MODE_EVBR) */
			HD_H26XENC_EVBR  *p_hdal_evbr = &p_hdal_param->evbr;
			VG_H26XENC_EVBR  *p_vg_evbr = &p_vg_param->rc_param.evbr;
			p_vg_evbr->bitrate = p_hdal_evbr->bitrate;
			p_vg_evbr->frame_rate_base = p_hdal_evbr->frame_rate_base;
			p_vg_evbr->frame_rate_incr = p_hdal_evbr->frame_rate_incr;
			p_vg_evbr->init_i_qp = p_hdal_evbr->init_i_qp;
			p_vg_evbr->max_i_qp = p_hdal_evbr->max_i_qp;
			p_vg_evbr->min_i_qp = p_hdal_evbr->min_i_qp;
			p_vg_evbr->init_p_qp = p_hdal_evbr->init_p_qp;
			p_vg_evbr->max_p_qp = p_hdal_evbr->max_p_qp;
			p_vg_evbr->min_p_qp = p_hdal_evbr->min_p_qp;
			p_vg_evbr->static_time = p_hdal_evbr->static_time;
			p_vg_evbr->ip_weight = p_hdal_evbr->ip_weight;
			p_vg_evbr->key_p_period = p_hdal_evbr->key_p_period;
			p_vg_evbr->kp_weight = p_hdal_evbr->kp_weight;
			p_vg_evbr->still_frame_cnd = p_hdal_evbr->still_frame_cnd;
			p_vg_evbr->motion_ratio_thd = p_hdal_evbr->motion_ratio_thd;
			p_vg_evbr->motion_aq_str = p_hdal_evbr->motion_aq_str;
			p_vg_evbr->still_i_qp = p_hdal_evbr->still_i_qp;
			p_vg_evbr->still_p_qp = p_hdal_evbr->still_p_qp;
			p_vg_evbr->still_kp_qp = p_hdal_evbr->still_kp_qp;
		}
	}

	if (h26x_enc_setting->is_enc_usr_qp) {
		HD_H26XENC_USR_QP *p_hdal_param = &h26x_enc_setting->enc_usr_qp;
		VG_H26XENC_USR_QP *p_vg_param = &p_hdal_params->usr_qp;

		p_hdal_params->update_flag.item.vg_h26xenc_usr_qp = 1;
		p_vg_param->enable = p_hdal_param->enable;
		p_vg_param->qp_map_addr = p_hdal_param->qp_map_addr;
	}

	if (h26x_enc_setting->is_enc_slice_split) {
		HD_H26XENC_SLICE_SPLIT *p_hdal_param = &h26x_enc_setting->enc_slice_split;
		VG_H26XENC_SLICE_SPLIT *p_vg_param = &p_hdal_params->slice_split;

		p_hdal_params->update_flag.item.vg_h26xenc_slice_split = 1;
		p_vg_param->enable = p_hdal_param->enable;
		p_vg_param->slice_row_num = p_hdal_param->slice_row_num;
	}

	if (h26x_enc_setting->is_enc_gdr) {
		HD_H26XENC_GDR *p_hdal_param = &h26x_enc_setting->enc_gdr;
		VG_H26XENC_GDR *p_vg_param = &p_hdal_params->gdr;

		p_hdal_params->update_flag.item.vg_h26xenc_gdr = 1;
		p_vg_param->enable = p_hdal_param->enable;
		p_vg_param->period = p_hdal_param->period;
		p_vg_param->number = p_hdal_param->number;
	}

	if (h26x_enc_setting->is_enc_roi) {
		INT i;
		HD_H26XENC_ROI *p_hdal_param = &h26x_enc_setting->enc_roi;
		VG_H26XENC_ROI *p_vg_param = &p_hdal_params->roi;

		p_hdal_params->update_flag.item.vg_h26xenc_roi = 1;
		for (i = 0; i < VG_H26XENC_ROI_WIN_COUNT; i++) {
			HD_H26XENC_ROI_WIN *p_hdal_st_roi = &p_hdal_param->st_roi[i];
			VG_H26XENC_ROI_WIN *p_vg_st_roi = &p_vg_param->st_roi[i];
			p_vg_st_roi->enable = p_hdal_st_roi->enable;
			p_vg_st_roi->coord_X = p_hdal_st_roi->rect.x;
			p_vg_st_roi->coord_Y = p_hdal_st_roi->rect.y;
			p_vg_st_roi->width = p_hdal_st_roi->rect.w;
			p_vg_st_roi->height = p_hdal_st_roi->rect.h;
			p_vg_st_roi->mode = p_hdal_st_roi->mode;
			p_vg_st_roi->qp = p_hdal_st_roi->qp;

		}
	}

	if (h26x_enc_setting->is_enc_row_rc) {
		HD_H26XENC_ROW_RC *p_hdal_param = &h26x_enc_setting->enc_row_rc;
		VG_H26XENC_ROW_RC *p_vg_param = &p_hdal_params->row_rc;

		p_hdal_params->update_flag.item.vg_h26xenc_row_rc = 1;
		p_vg_param->enable = p_hdal_param->enable;
		p_vg_param->i_qp_range = p_hdal_param->i_qp_range;
		p_vg_param->i_qp_step = p_hdal_param->i_qp_step;
		p_vg_param->p_qp_range = p_hdal_param->p_qp_range;
		p_vg_param->p_qp_step = p_hdal_param->p_qp_step;
		p_vg_param->min_i_qp = p_hdal_param->min_i_qp;
		p_vg_param->max_i_qp = p_hdal_param->max_i_qp;
		p_vg_param->min_p_qp = p_hdal_param->min_p_qp;
		p_vg_param->max_p_qp = p_hdal_param->max_p_qp;
	}

	if (h26x_enc_setting->is_enc_aq) {
		INT i;
		HD_H26XENC_AQ *p_hdal_param = &h26x_enc_setting->enc_aq;
		VG_H26XENC_AQ *p_vg_param = &p_hdal_params->aq;

		p_hdal_params->update_flag.item.vg_h26xenc_aq = 1;
		p_vg_param->enable = p_hdal_param->enable;
		p_vg_param->i_str = p_hdal_param->i_str;
		p_vg_param->p_str = p_hdal_param->p_str;
		p_vg_param->max_delta_qp = p_hdal_param->max_delta_qp;
		p_vg_param->min_delta_qp = p_hdal_param->min_delta_qp;
		p_vg_param->depth = p_hdal_param->depth;
		for (i = 0; i < VG_H26XENC_AQ_MAP_TABLE_COUNT; i++) {
			p_vg_param->thd_table[i] = p_hdal_param->thd_table[i];
		}
	}
}

#if SEND_DEC_BLACK_BS
INT dif_send_black_bs_to_vdec(HD_PATH_ID dec_pathid, HD_DIM user_dec_max_dim, UINT32 codec_type)
{
	HD_RESULT hd_ret = HD_OK;
	HD_VIDEODEC_SEND_LIST video_bs[1];
	const_bitstream_t *p_bs = NULL;

	if (codec_type == HD_CODEC_TYPE_H264) {
		if (user_dec_max_dim.w >= 640 && user_dec_max_dim.h >= 480) {
			p_bs = &const_bs_640x480_h264;
		} else if (user_dec_max_dim.w >= 352 && user_dec_max_dim.h >= 240) {
			p_bs = &const_bs_352x240_h264;
		} else if (user_dec_max_dim.w >= 80 && user_dec_max_dim.h >= 96) {
			p_bs = &const_bs_80x96_h264;
		}
	} else if (codec_type == HD_CODEC_TYPE_H265) {
		if (user_dec_max_dim.w >= 640 && user_dec_max_dim.h >= 480) {
			p_bs = &const_bs_640x480_h265;
		} else if (user_dec_max_dim.w >= 352 && user_dec_max_dim.h >= 240) {
			p_bs = &const_bs_352x240_h265;
		} else if (user_dec_max_dim.w >= 80 && user_dec_max_dim.h >= 96) {
			p_bs = &const_bs_80x96_h265;
		}
	} else {
		if (user_dec_max_dim.w >= 640 && user_dec_max_dim.h >= 480) {
			p_bs = &const_bs_640x480_jpeg;
		} else if (user_dec_max_dim.w >= 352 && user_dec_max_dim.h >= 240) {
			p_bs = &const_bs_352x240_jpeg;
		} else if (user_dec_max_dim.w >= 80 && user_dec_max_dim.h >= 96) {
			p_bs = &const_bs_80x96_jpeg;
		}
	}
	if (p_bs == NULL) {
		GMLIB_ERROR_P("Skip sending black frame, codec_type(%d) wh(%u-%u)\n",
			      codec_type, user_dec_max_dim.w, user_dec_max_dim.h);
		goto skip_send_bs;
	}

	memset(video_bs, 0, sizeof(video_bs));  /* clear all video_bs */
	video_bs[0].path_id = dec_pathid;
	video_bs[0].user_bs.p_bs_buf = p_bs->p_data;
	video_bs[0].user_bs.bs_buf_size = p_bs->size;
	// user_flag bit1: dec black send bitstream control
	//                 0(normal bitstream), 1(black bitstream, DEC do not keep it as reference frame)
	video_bs[0].user_bs.user_flag = DIF_BLACK_BS_BIT_1();
	hd_ret = dif_video_send_bs(video_bs, 1, 1000);
	if (hd_ret != HD_OK) {
		GMLIB_ERROR_P("Send black bs failed, ret(%d)\n", hd_ret);
	}
	return 0;

skip_send_bs:
	return 1;
}

INT dif_send_black_frame_to_vproc(HD_PATH_ID pathid, HD_DIM max_dim, HD_VIDEO_PXLFMT pxlfmt)
{
	HD_RESULT hd_ret = HD_OK;
	HD_VIDEODEC_SEND_LIST video_bs = {0};
	HD_VIDEO_FRAME frame_buffer = {0};
	UINT32 frame_buf_size = 0;
	HD_COMMON_MEM_DDR_ID ddr_id = DDR_ID0;
	HD_COMMON_MEM_VB_BLK frame_blk = HD_COMMON_MEM_VB_INVALID_BLK;
	UINT64 pool = HD_COMMON_MEM_DISP_DEC_OUT_POOL;
	CHAR *frame_buffer_va = NULL, *p_data = NULL;
	UINT i;
	CHAR dev_string[64];

	dif_get_dev_string(pathid, dev_string, sizeof(dev_string));
	if ((pxlfmt != HD_VIDEO_PXLFMT_YUV420) && (pxlfmt != HD_VIDEO_PXLFMT_YUV422_ONE)) {
		dif_get_dev_string(pathid, dev_string, sizeof(dev_string));
		GMLIB_ERROR_P(" %s Unsupported pxlfmt, please check HD_VIDEOPROC_PARAM_IN, path_id(%#x) pxlfmt(%#x)\n", dev_string, pathid, pxlfmt);
		goto err_exit;
	}

	frame_buffer.sign   = MAKEFOURCC('V', 'F', 'R', 'M');
	frame_buffer.dim.w  = max_dim.w;
	frame_buffer.dim.h  = max_dim.h;
	frame_buffer.pxlfmt  = pxlfmt;
	frame_buffer.ddr_id = ddr_id;
	frame_buf_size = hd_common_mem_calc_buf_size(&frame_buffer);

	frame_blk = hd_common_mem_get_block(pool, frame_buf_size, ddr_id);
	if (HD_COMMON_MEM_VB_INVALID_BLK == frame_blk) {
		goto err_exit;
	}
	frame_buffer.phy_addr[0] = hd_common_mem_blk2pa(frame_blk);
	if (frame_buffer.phy_addr[0] == 0) {
		hd_common_mem_release_block(frame_blk);
		goto err_exit;
	}
	frame_buffer_va = (CHAR *)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, frame_buffer.phy_addr[0], frame_buf_size);
	if (!frame_buffer_va) {
		GMLIB_ERROR_P(" %s path_id(%#x) frame_buffer_va NULL\n", dev_string, pathid);
		goto err_exit;
	}

	p_data = frame_buffer_va;
	if (pxlfmt == HD_VIDEO_PXLFMT_YUV422_ONE) {
		for (i = 0; i < (frame_buf_size / 2); i += 2) {
			p_data[0] = 0x80;
			p_data[1] = 0x10;
			p_data[2] = 0x80;
			p_data[3] = 0x10;
			p_data += 4;
		}
	} else if (pxlfmt == HD_VIDEO_PXLFMT_YUV420) {
		for (i = 0; i < (frame_buf_size * 2 / 3); i++) {
			p_data[i] = 0x10;
		}
		for (i = (frame_buf_size * 2 / 3); i < frame_buf_size; i++) {
			p_data[i] = 0x80;
		}
	}
	if (_hd_common_mem_cache_sync_dma_to_device(frame_buffer_va, frame_buf_size) != HD_OK) {
		goto err_exit;
	}

	video_bs.path_id = pathid;
	video_bs.user_bs.p_bs_buf = frame_buffer_va;
	video_bs.user_bs.bs_buf_size = frame_buf_size;
	hd_ret = dif_video_send_bs(&video_bs, 1, 1000);
	if (hd_ret != HD_OK) {
		GMLIB_ERROR_P(" %s path_id(%#x) Send black frame failed, ret(%d)\n", dev_string, pathid, hd_ret);
	}
	if (hd_common_mem_munmap(frame_buffer_va, frame_buf_size) != HD_OK) {
		goto err_exit;
	}
	hd_common_mem_release_block(frame_blk);
	return 0;

err_exit:
	if (frame_buffer_va) {
		if (hd_common_mem_munmap(frame_buffer_va, frame_buf_size) != HD_OK) {
			GMLIB_ERROR_P(" %s path_id(%#x) hd_common_mem_munmap fail\n", dev_string, pathid);
		}
	}
	if (HD_COMMON_MEM_VB_INVALID_BLK != frame_blk) {
		hd_common_mem_release_block(frame_blk);
	}
	return 1;
}
#endif

HD_RESULT dif_check_flow_mode(HD_DAL dev_id, HD_IO out_id)
{
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	UINT32 out_subid = out_id - HD_OUT_BASE;

	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("Error get_bind fail\n");
		goto ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		goto ext;
	}
	if (group_line->state != HD_LINE_START) {
		GMLIB_L2_P("flow mode is not available, go to user trigger\n");
		goto ext;
	}
	return HD_ERR_ALREADY_BIND;

ext:
	return HD_ERR_NOT_AVAIL;
}

HD_RESULT dif_get_vout_win_psr_attr_by_groupline(HD_GROUP_LINE *p_group_line, HD_VIDEOOUT_WIN_PSR_ATTR *p_win_psr)
{
	INT member_idx;
	HD_MEMBER *p_member;
	DIF_VOUT_PARAM vout;

	if (p_group_line == NULL) {
		printf("NULL HD_GROUP\n");
		goto err_exit;
	}
	if (p_win_psr == NULL) {
		printf("NULL p_win_psr\n");
		goto err_exit;
	}

	for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
		if (p_group_line->member[member_idx].p_bind == NULL) {
			printf("[m=%d] null\n", member_idx);
			continue;
		}
		p_member = &(p_group_line->member[member_idx]);
		//printf("yes  member baseid = 0x%x\n", p_member->p_bind->device.deviceid);
		if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
			vout.param = videoout_get_param_p(p_member->p_bind->device.deviceid);
			if (vout.param && vout.param->win_psr[p_member->in_subid] != NULL) {
				memcpy(p_win_psr, vout.param->win_psr[p_member->in_subid], sizeof(HD_VIDEOOUT_WIN_PSR_ATTR));
				return HD_OK;
			} else {
				memset(p_win_psr, 0, sizeof(HD_VIDEOOUT_WIN_PSR_ATTR));
				return HD_ERR_FAIL;//has vout, but no psr
			}
		}
	}

err_exit:
	return HD_ERR_NG;
}

HD_RESULT dif_get_next_out_wsr(HD_DAL dev_id, HD_IO out_id, HD_VIDEOOUT_WIN_PSR_ATTR *p_win_psr)
{
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	UINT32 out_subid = out_id - HD_OUT_BASE;
	HD_RESULT dif_ret;
	//HD_VIDEOOUT_WIN_PSR_ATTR win_psr;

	if ((p_bind = get_bind(dev_id)) == NULL) {
		printf("Error get_bind fail\n");
		goto ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		printf("No group_line out_sub=%d\n", out_subid);
		goto ext;
	}
	if (group_line->state != HD_LINE_START) {
		printf("flow mode is not available, go to user trigger\n");
		goto ext;
	}

	dif_ret = dif_get_vout_win_psr_attr_by_groupline(group_line, p_win_psr);
	if (dif_ret == HD_OK) {
		return HD_OK;
	} else if (dif_ret == HD_ERR_FAIL) {//has vout, no psr
		return HD_OK;
	}
ext:
	return HD_ERR_NG;
}
HD_RESULT vproc_pull_in(HD_DAL dev_id, HD_IO in_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	VDOPROC_INTL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	//UINT32 in_subid = VDOPROC_INID(in_id);
	UINT32 out_subid = VDOPROC_OUTID(out_id);
	gm_buffer_t frame_buffer = {0};
	gm_video_frame_info_t frame_info = {0};
	pif_get_buf_t get_buf_info = {0};
	HD_DIM out_dim = {0, 0};
	HD_DIM out_bg_dim = {0, 0};
	HD_VIDEO_PXLFMT out_pxlfmt = HD_VIDEO_PXLFMT_NONE;

	p_dev_cfg = videoproc_get_param_p(dev_id, out_subid);
	if (!p_dev_cfg) {
		GMLIB_L1_P("vproc_%d cfg not found\n", VDOPROC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("Error get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state != HD_LINE_START) {
		GMLIB_L2_P("flow mode is not available, go to user trigger\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->linefd = group_line->linefd;
	get_buf_info.linefd = group_line->linefd;
	get_buf_info.post_handling = 0;
	get_buf_info.wait_ms = wait_ms;

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_VPE)) {
		get_buf_info.entity_type = VPD_VPE_1_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_vpe_entity_fd(bd_get_dev_subid(dev_id), out_subid, NULL);
	} else if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_DI)) {
		get_buf_info.entity_type = VPD_DI_ENTITY_TYPE;
		get_buf_info.entity_fd = utl_get_di_entity_fd(bd_get_dev_subid(dev_id), out_subid, NULL);
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_user_get_buffer(get_buf_info, &frame_buffer, &frame_info, NULL)) < 0) {
		GMLIB_ERROR_P("pif_user_get_buffer fail, err_code=%x\n", err_code);
		if (err_code == -15) {
			hd_ret = HD_ERR_TIMEDOUT;
		} else {
			hd_ret = HD_ERR_SYS;
		}
		goto err_ext;
	}

	out_dim.w = (UINT32)frame_info.dim.width;
	out_dim.h = (UINT32)frame_info.dim.height;
	out_pxlfmt = common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(frame_info.format);
	if (dif_get_align_up_dim(out_dim, out_pxlfmt, &out_bg_dim) != HD_OK) {
		GMLIB_ERROR_P("vout_pull_out: dif_get_align_up_dim fail\n");
		goto err_ext;
	}
	GMLIB_L2_P("vproc_pull_in: ddr(%d) pa(%p) timestamp(%llu) out_dim(%u,%u) out_bg_dim(%u,%u) pxlfmt(%#x)\n",
		   frame_buffer.ddr_id, frame_buffer.pa, frame_info.timestamp,
		   out_dim.w, out_dim.h, out_bg_dim.w, out_bg_dim.h, out_pxlfmt);

	if (p_video_frame) {
		p_video_frame->ddr_id = frame_buffer.ddr_id;
		p_video_frame->phy_addr[0] = (UINTPTR)frame_buffer.pa;
		// foreground dimension
		p_video_frame->dim.w = out_dim.w;
		p_video_frame->dim.h = out_dim.h;
		// background dimension
		p_video_frame->reserved[0] = out_bg_dim.w;
		p_video_frame->reserved[1] = out_bg_dim.h;
		// timestamp user flag
		p_video_frame->reserved[4] = (UINT32)frame_info.ts_user_flag;
		p_video_frame->pxlfmt = out_pxlfmt;
		p_video_frame->timestamp = (UINT64)frame_info.timestamp;
	}

err_ext:
	return hd_ret;
}

HD_RESULT dif_pull_in_buffer(HD_DAL dev_id, HD_IO in_id, HD_IO out_id, VOID *p_out_buffer, INT32 wait_ms)
{
	HD_RESULT ret = HD_OK;

	if (p_out_buffer == NULL) {
		GMLIB_ERROR_P("NULL p_out_buffer deviceid(%d)\n", dev_id);
		ret = HD_ERR_NOT_AVAIL;
		goto exit;
	}

	switch (bd_get_dev_baseid(dev_id)) {
	case HD_DAL_VIDEOPROC_BASE:
		ret = vproc_pull_in(dev_id, in_id, out_id, (HD_VIDEO_FRAME *)p_out_buffer, wait_ms);
		break;
	case HD_DAL_VIDEODEC_BASE:
	case HD_DAL_VIDEOCAP_BASE:
	case HD_DAL_VIDEOOUT_BASE:
	case HD_DAL_VIDEOENC_BASE:
	case HD_DAL_AUDIOENC_BASE:
	case HD_DAL_AUDIODEC_BASE:
	case HD_DAL_AUDIOCAP_BASE:
	case HD_DAL_AUDIOOUT_BASE:
	default:
		GMLIB_ERROR_P("not support deviceid(%d)\n", dev_id);
		break;
	}

exit:
	return ret;
}

HD_RESULT vproc_release_in(HD_DAL dev_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame)
{
	VDOPROC_INTL_PARAM *p_dev_cfg;
	HD_BIND_INFO *p_bind = NULL;
	HD_GROUP_LINE *group_line;
	HD_RESULT hd_ret = HD_OK;
	INT err_code = 0;
	UINT32 out_subid = VDOPROC_OUTID(out_id);
	gm_buffer_t frame_buffer;
	vpd_entity_type_t entity_type;

	p_dev_cfg = videoproc_get_param_p(dev_id, out_subid);
	if (!p_dev_cfg) {
		GMLIB_L1_P("vproc_%d cfg not found\n", VDOPROC_DEVID(dev_id));
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if ((p_bind = get_bind(dev_id)) == NULL) {
		GMLIB_L1_P("get_bind fail\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	group_line = get_group_line_p(p_bind, NULL, &out_subid);
	if (group_line == 0) {
		GMLIB_L1_P("No group_line\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		GMLIB_L1_P("HD_LINE_INCOMPLETE\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}
	p_dev_cfg->linefd = group_line->linefd;

	frame_buffer.ddr_id = p_video_frame->ddr_id;
	frame_buffer.pa = (void *)p_video_frame->phy_addr[0];

	if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_VPE)) {
		entity_type = VPD_VPE_1_ENTITY_TYPE;
	} else if (BD_IS_BMP_INDEX(group_line->unit_bmp, HD_BIT_DI)) {
		entity_type = VPD_DI_ENTITY_TYPE;
	} else {
		GMLIB_L1_P("this unit driver is not exist\n");
		hd_ret = HD_ERR_NOT_AVAIL;
		goto err_ext;
	}

	if ((err_code = pif_user_release_buffer(group_line->linefd, entity_type, &frame_buffer)) < 0) {
		GMLIB_ERROR_P("pif_user_release_buffer fail, err_code=%x\n", err_code);
		hd_ret = HD_ERR_NG;
		goto err_ext;
	}

err_ext:
	return hd_ret;
}

HD_RESULT dif_release_in_buffer(HD_DAL dev_id, HD_IO io_id, VOID *p_in_buffer)
{
	HD_RESULT ret = HD_OK;

	if (p_in_buffer == NULL) {
		GMLIB_ERROR_P("NULL p_out_buffer deviceid(%d)\n", dev_id);
		ret = HD_ERR_NOT_AVAIL;
		goto exit;
	}

	switch (bd_get_dev_baseid(dev_id)) {
	case HD_DAL_VIDEOPROC_BASE:
		ret = vproc_release_in(dev_id, io_id, (HD_VIDEO_FRAME *)p_in_buffer);
		break;
	case HD_DAL_VIDEODEC_BASE:
	case HD_DAL_VIDEOCAP_BASE:
	case HD_DAL_VIDEOOUT_BASE:
	case HD_DAL_VIDEOENC_BASE:
	case HD_DAL_AUDIOENC_BASE:
	case HD_DAL_AUDIODEC_BASE:
	case HD_DAL_AUDIOCAP_BASE:
	case HD_DAL_AUDIOOUT_BASE:
	default:
		GMLIB_ERROR_P("not support deviceid(%d)\n", dev_id);
		break;
	}

exit:
	return ret;
}
