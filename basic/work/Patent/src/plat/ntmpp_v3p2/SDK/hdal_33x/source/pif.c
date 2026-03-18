/*
 *   @file   pif.c
 *
 *   @brief  platform interface.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <poll.h>
#include <sys/syscall.h>
#include <malloc.h>
#include "cif_list.h"
#include "cif_common.h"
#include "vpd_ioctl.h"
#include "pif.h"
#include "pif_ioctl.h"
#include "utl.h"
#include "vg_common.h"
#include "trig_ioctl.h"
#include "videodec.h"
#include "videoout.h"
#include "nvt-pcie-lib.h"

#define PIF_MAX_SCALER_PER_CHANNEL  10

#define PIF_RAW_MIN_WIDTH      16
#define PIF_RAW_MIN_HEIGHT     16
#define PIF_MAX_BUFFER_SIZE    4*1024*4024

#define BIT_31 (0x1<<31)
#define PIF_APPLY_FORCE_GRAPH_EXIT BIT_31
#define PIF_PRODUCT_FILENAME    "/tmp/product_info"
#define MAKEBLK(pa, ddrid) ((UINT32)((pa | ((UINT8)ddrid))))
#define PCIE_DEV_NAME "/dev/nvt-pcie-lib"

#define PIF_ASSERT_LCD_VCH(vch) \
	do { \
		if (platform_sys_Info.lcd_info[(vch)].lcdid < 0 || platform_sys_Info.lcd_info[(vch)].active == 0) {\
			GMLIB_ERROR_P("lcd: lcd_vch(%d) invalid!\n", (vch)); \
			return -1; \
		}\
	} while (0)

#define PIF_ASSERT_CAP_VCH(vch) \
	do { \
		if (platform_sys_Info.cap_info[(vch)].vcapch < 0) {\
			GMLIB_ERROR_P("cap: cap_vch(%d) invalid!\n", (vch)); \
			return -1; \
		}\
	} while (0)

#define PIF_CHECK_ATTR_V(param, name) \
	do { \
		if (( (int)param) == (int) GMLIB_NULL_VALUE) {\
			GMLIB_ERROR_P("%s: %s not assigned!\n", name, (#param)); \
			return -1;\
		} \
	} while (0)

#define PIF_CHECK_ENTITY(head_addr) \
	do { \
		unsigned int magic; \
		magic = VPD_GET_ENTITY_MAGIC_VAL((head_addr)); \
		if (magic != VPD_ENTITY_MAGIC_NUM) {\
			GMLIB_ERROR_P("entity magic(%#x, %p) error!\n", magic, head_addr); \
			return 0;\
		} \
	} while (0)

#define PIF_CHECK_ENTITY_OFFSET(addr, offset) \
	do { \
		void *entity_head; \
		unsigned int magic; \
		entity_head = VPD_GET_ENTITY_HEAD_ADDR(addr, offset); \
		magic = VPD_GET_ENTITY_MAGIC_VAL((entity_head)); \
		if (magic != VPD_ENTITY_MAGIC_NUM) { \
			GMLIB_ERROR_P("entity magic(%#x, %p) error!\n", magic, entity_head); \
			return -1; \
		} \
	} while (0)

#define PIF_GET_COUNT(count)	(count ? (count/10) : 0)

typedef struct pifSclRes {
	int active;
	gm_dim_t dim;
} pifSclRes_t;

typedef enum {
	PIF_LOG = 0x1,
	PIF_BUFFER = 0x2
} printWhere_t;

typedef struct {
	usr_entity_type_t type;
	unsigned int fd;
} pif_usr_entity_t;


unsigned int gmlib_dbg_level = 0x0;
unsigned int gmlib_dbg_mode = 0x0; //0:log  1:direct print
unsigned int gmlib_quiet = 0x0; // 0:turn on console print  1: turn off console print
unsigned int display_max_fps[VPD_MAX_LCD_NUM] = {30, 30, 30, 30, 30, 30}; // 60fps or 30fps(default)

extern unsigned int cif_list_alloc_nr;

pthread_mutex_t vpd_mutex;

pthread_t sig_info_dispatcher_id;
struct {
	gm_notify_handler_t fn_notify_handler;
	void *user_data;
	long long last_notify_time, last_notify_time2;
	unsigned int  count;
} pif_notify_info[GM_MAX_GROUP_NUM][MAX_GM_NOTIFY_COUNT];

int max_group_idx = 0;

pif_init_private_t pif_init_data = {0};
int vpd_fd = 0;

char *pif_msg = NULL;
int pif_target_disp_rate[VPD_MAX_LCD_NUM];
pif_group_t *pif_group[GM_MAX_GROUP_NUM];
vpd_sys_info_t platform_sys_Info = {0};
vpd_sys_spec_info_t platform_sys_spec_Info;
pif_cap_info_t pif_cap_collect_info[VPD_MAX_CAPTURE_CHANNEL_NUM][VPD_MAX_CAPTURE_PATH_NUM];
pif_liveview_info_t pif_lv_collect_info[VPD_MAX_CAPTURE_CHANNEL_NUM][VPD_MAX_CAPTURE_PATH_NUM];
vpd_version_t pif_version;
vpd_version_t vpd_version;
static vpd_ddr_range_info_t hdal_ddr_info[VPD_MAX_DDR_NUM];
unsigned int pif_static_time = 0 * 1000000;

vpd_update_entity_t *pif_update_entity_buf = NULL;
int pif_update_entity_buf_len = 1024;

int vcap_ioctl = 0;
int osg_ioctl = 0;
int vpe_ioctl = 0;
int ivs_ioctl = 0;
int au_ioctl = 0;
int frammap_ioctl = 0;
int ms_ioctl = 0;
int devmem_fd = 0;
int gs_ioctl = 0;

extern void gmlib_setting_log(void);
extern void gmlib_flow_log(char *msg_buf, int msg_len);
extern void hdal_flow_enable(unsigned int);
extern void gmlib_flow_enable(unsigned char *);
extern void hdal_show_alloc_statistic(void);

VOID dif_setting_log(unsigned char *msg_cmd);

void pif_debug_snapshot(snapshot_t *snapshot, unsigned int timeout_ms);
void pif_debug_snapshot_result(unsigned int timestamp, unsigned int bs_size);
void pif_debug_clrwin(int lcd_vch, gm_clear_window_t *cw_str);
void pif_debug_bitstream(void *obj, gm_clear_window_t *cw_str);
void pif_debug_adjust_disp(int lcd_vch, int x, int y, int width, int height);
void pif_debug_set_display_rate(int lcd_vch, int display_rate);
int pif_set_for_graph_exit(void);
void pif_set_disp_max_fps(int lcd_vch, unsigned int max_fps);

const char *pif_notify_name[MAX_GM_NOTIFY_COUNT] = {
	"UNKNOWN",
	"PERF_LOG",
	"GMLIB_DBGMODE",
	"GMLIB_DBGLEVEL",
	"SIGNAL_LOSS",
	"SIGNAL_PRESENT",
	"FRAMERATE_CHANGE",
	"HW_CONFIG_CHANGE",
	"TAMPER_ALARM",
	"TAMPER_ALARM_RELEASE"
	"AU_BUF_UNDERRUN"
	"AU_BUF_OVERRUN"
	"HDAL_LOG "
	"HDAL_FLOW"
	"GMLIB_FLOW"
};

char *bs_return_str[] = {
	"SUCCESS",          //GM_SUCCESS               0
	"FD_NOT_EXIST",     //GM_FD_NOT_EXIST         -1
	"BS_BUF_TOO_SMALL", //GM_BS_BUF_TOO_SMALL     -2
	"EXTRA_BUF_TOO_SMALL",//GM_EXTRA_BUF_TOO_SMALL -3
	"TIMEOUT",          // GM_TIMEOUT              -4
};


pif_dbg_alloc_statistic_t pif_dbg_alloc = {0};
pthread_mutex_t pif_dbg_alloc_mutex;
void *pif_malloc(size_t size, const char *func_name)
{
	void *ret_val;
	ret_val = malloc(size);

	pthread_mutex_lock(&pif_dbg_alloc_mutex);
	pif_dbg_alloc.pif_dbg_malloc_cnt ++;
	pif_dbg_alloc.pif_dbg_malloc_size += malloc_usable_size(ret_val);
	pthread_mutex_unlock(&pif_dbg_alloc_mutex);

#if LIB_DEBUG_BUF_ALLOC_PRINT_LOG
	printf(" %s: malloc(%d)=ptr(%p)\n", func_name, size, ret_val);
#endif
	return ret_val;
}

void pif_free(void *ptr, const char *func_name)
{
	int size;
	pthread_mutex_lock(&pif_dbg_alloc_mutex);
	size = malloc_usable_size(ptr);
	pif_dbg_alloc.pif_dbg_free_cnt ++;
	pif_dbg_alloc.pif_dbg_free_size += size;
	pthread_mutex_unlock(&pif_dbg_alloc_mutex);

	free(ptr);
#if LIB_DEBUG_BUF_ALLOC_PRINT_LOG
	printf(" %s: free(%p), size(%d)\n", func_name, ptr, size);
#endif
	return;
}

void *pif_calloc(size_t nmemb, size_t size, const char *func_name)
{
	void *ret_val;

	ret_val = calloc(nmemb, size);
	pthread_mutex_lock(&pif_dbg_alloc_mutex);
	pif_dbg_alloc.pif_dbg_calloc_cnt ++;
	pif_dbg_alloc.pif_dbg_calloc_size += malloc_usable_size(ret_val);
	pthread_mutex_unlock(&pif_dbg_alloc_mutex);

#if LIB_DEBUG_BUF_ALLOC_PRINT_LOG
	printf(" %s: calloc(%d,%d)=ptr(%p)\n", func_name, nmemb, size, ret_val);
#endif
	return ret_val;
}

void *pif_realloc(void *ptr, size_t size, const char *func_name)
{
	int old_size;
	void *ret_val;

	old_size = malloc_usable_size(ptr);
	ret_val = realloc(ptr, size);
	pthread_mutex_lock(&pif_dbg_alloc_mutex);
	pif_dbg_alloc.pif_dbg_realloc_cnt ++;
	pif_dbg_alloc.pif_dbg_realloc_size += (malloc_usable_size(ret_val) - old_size);
	pthread_mutex_unlock(&pif_dbg_alloc_mutex);

#if LIB_DEBUG_BUF_ALLOC_PRINT_LOG
	printf(" %s: realloc(%p,%d)=ptr(%p), old_size(%d)\n", func_name, ptr, size, ret_val, old_size);
#endif
	return ret_val;
}


char *pif_get_pool_name_by_type(vpbuf_pool_type_t pool_type)
{
	char *name = NULL;

	switch (pool_type) {
	case VPD_DISP0_IN_POOL:
		name = "disp0_in";
		break;
	case VPD_DISP1_IN_POOL:
		name = "disp1_in";
		break;
	case VPD_DISP2_IN_POOL:
		name = "disp2_in";
		break;
	case VPD_DISP3_IN_POOL:
		name = "disp3_in";
		break;
	case VPD_DISP4_IN_POOL:
		name = "disp4_in";
		break;
	case VPD_DISP5_IN_POOL:
		name = "disp5_in";
		break;
	case VPD_DISP0_CAP_OUT_POOL:
		name = "disp0_cap_out";
		break;
	case VPD_DISP1_CAP_OUT_POOL:
		name = "disp1_cap_out";
		break;
	case VPD_DISP2_CAP_OUT_POOL:
		name = "disp2_cap_out";
		break;
	case VPD_DISP3_CAP_OUT_POOL:
		name = "disp3_cap_out";
		break;
	case VPD_DISP4_CAP_OUT_POOL:
		name = "disp4_cap_out";
		break;
	case VPD_DISP5_CAP_OUT_POOL:
		name = "disp5_cap_out";
		break;
	case VPD_DISP0_ENC_SCL_OUT_POOL:
		name = "disp0_enc_scl_out";
		break;
	case VPD_DISP1_ENC_SCL_OUT_POOL:
		name = "disp1_enc_scl_out";
		break;
	case VPD_DISP2_ENC_SCL_OUT_POOL:
		name = "disp2_enc_scl_out";
		break;
	case VPD_DISP3_ENC_SCL_OUT_POOL:
		name = "disp3_enc_scl_out";
		break;
	case VPD_DISP4_ENC_SCL_OUT_POOL:
		name = "disp4_enc_scl_out";
		break;
	case VPD_DISP5_ENC_SCL_OUT_POOL:
		name = "disp5_enc_scl_out";
		break;
	case VPD_DISP0_ENC_OUT_POOL:
		name = "disp0_enc_out";
		break;
	case VPD_DISP1_ENC_OUT_POOL:
		name = "disp1_enc_out";
		break;
	case VPD_DISP2_ENC_OUT_POOL:
		name = "disp2_enc_out";
		break;
	case VPD_DISP3_ENC_OUT_POOL:
		name = "disp3_enc_out";
		break;
	case VPD_DISP4_ENC_OUT_POOL:
		name = "disp4_enc_out";
		break;
	case VPD_DISP5_ENC_OUT_POOL:
		name = "disp5_enc_out";
		break;
	case VPD_DISP_DEC_IN_POOL:
		name = "disp_dec_in";
		break;
	case VPD_DISP_DEC_OUT_POOL:
		name = "disp_dec_out";
		break;
	case VPD_DISP_DEC_OUT_RATIO_POOL:
		name = "disp_dec_out_ratio";
		break;
	case VPD_ENC_CAP_OUT_POOL:
		name = "enc_cap_out";
		break;
	case VPD_ENC_SCL_OUT_POOL:
		name = "enc_scl_out";
		break;
	case VPD_ENC_OUT_POOL:
		name = "enc_out";
		break;
	case VPD_AU_ENC_AU_GRAB_OUT_POOL:
		name = "aud_enc";
		break;
	case VPD_AU_DEC_AU_RENDER_IN_POOL:
		name = "aud_dec";
		break;
	case VPD_FLOW_MD_POOL:
		name = "flow_md";
		break;
	case VPD_USER_BLK:
		name = "user";
		break;
#if 0
	case VPD_DISP0_FB_POOL:
		name = "disp0_fb";
		break;
	case VPD_DISP1_FB_POOL:
		name = "disp1_fb";
		break;
	case VPD_DISP2_FB_POOL:
		name = "disp2_fb";
		break;
	case VPD_DISP3_FB_POOL:
		name = "disp3_fb";
		break;
	case VPD_DISP4_FB_POOL:
		name = "disp4_fb";
		break;
	case VPD_DISP5_FB_POOL:
		name = "disp5_fb";
		break;
	case VPD_ENC_REF_POOL:
		name = "enc_ref";
		break;
	case VPD_MB_INFO_POOL:
		name = "mb_info";
		break;
#endif
	case VPD_COMMON_POOL:
		name = "common";
		break;
	case VPD_END_POOL:
		name = "end";
		break;
	default:
		name = "Undefine POOL";
		break;
	}
	return name;
}

char *pif_covert_ret_str(int retval)
{
	char *retstr = NULL;

	switch (retval) {
	case 0:
		retstr = bs_return_str[0];
		break;//GM_SUCCESS
	case -1:
		retstr = bs_return_str[1];
		break; //GM_FD_NOT_EXIST
	case -2:
		retstr = bs_return_str[2];
		break; //GM_BS_BUF_TOO_SMALL
	case -3:
		retstr = bs_return_str[3];
		break; //GM_EXTRA_BUF_TOO_SMALL
	case -4:
		retstr = bs_return_str[4];
		break; //GM_TIMEOUT
	default:
		retstr = bs_return_str[0];
		break; //GM_SUCCESS
	}
	return retstr;
}

int pif_print(int where, char *buf, const char *fmt, ...)
{
	int len = 0;
	va_list ap;
	char line[256];

	if (buf == NULL) {
		GMLIB_ERROR_P("NULL buf\n");
		return 0;
	}

	va_start(ap, fmt);
	len = vsnprintf(line, sizeof(line), fmt, ap);
	va_end(ap);

	if (where & PIF_LOG) {
		GMLIB_L1_P("%s", line);
	}
	if (where & PIF_BUFFER) {
		memcpy(buf, line, len);
	}
	return len;
}

void pif_print_platformInfo(int where)
{
	char inStr[32], outStr[32];
	int i, len = 0;
	static char *pifBuff = NULL;
	FILE *sysfile = NULL;


	if (where & PIF_BUFFER) {
		pifBuff =  PIF_MALLOC(5120);
		if (pifBuff == NULL) {
			GMLIB_ERROR_P("memory allc fail!\n");
			return;
		}
	}
	len += pif_print(where, pifBuff + len, "\n");

	len += pif_print(where, pifBuff + len, "VP_driver: ioctl_version: %d.%d, Date:%s, Time:%s\n",
			 (vpd_version.version & 0x0000ff00) >> 8,
			 vpd_version.version  & 0x000000ff,
			 vpd_version.compiler_date,
			 vpd_version.compiler_time);
	len += pif_print(where, pifBuff + len,  "gm_lib:    ioctl_version: %d.%d, Date:%s, Time:%s\n",
			 (pif_version.version & 0x0000ff00) >> 8,
			 pif_version.version  & 0x000000ff,
			 pif_version.compiler_date,
			 pif_version.compiler_time);

	/* LCD info */
	len += pif_print(where, pifBuff + len, "\n");
	len += pif_print(where, pifBuff + len, "LCD Info:\n");
	len += pif_print(where, pifBuff + len, "------------------------------------------------"
			 "----------\n");
	len += pif_print(where, pifBuff + len, "vch id type cid fps dup input   "
			 "         output\n");
	for (i = 0; i < VPD_MAX_LCD_NUM; i++) {
		if (platform_sys_Info.lcd_info[i].lcdid < 0) {
			continue;
		}
		snprintf(inStr, sizeof(inStr) - 1, "%d,%dx%d",
			 platform_sys_Info.lcd_info[i].fb0_win.id,
			 platform_sys_Info.lcd_info[i].fb0_win.width,
			 platform_sys_Info.lcd_info[i].fb0_win.height);
		snprintf(outStr, sizeof(outStr) - 1, "%d,%dx%d",
			 platform_sys_Info.lcd_info[i].output_type.id,
			 platform_sys_Info.lcd_info[i].output_type.width,
			 platform_sys_Info.lcd_info[i].output_type.height);
		len += pif_print(where, pifBuff + len,
				 "%-3d %-2d %-3d  %-3d %-3d %-3d %-16s %-16s\n",
				 i,
				 platform_sys_Info.lcd_info[i].lcdid,
				 platform_sys_Info.lcd_info[i].lcd_type,
				 platform_sys_Info.lcd_info[i].chipid,
				 platform_sys_Info.lcd_info[i].fps,
				 platform_sys_Info.lcd_info[i].src_duplicate_vch,
				 inStr,
				 outStr);
	}
	/* capture info */
	len += pif_print(where, pifBuff + len, "\n");
	len += pif_print(where, pifBuff + len, "Cap Info:\n");
	len += pif_print(where, pifBuff + len, "------------------------------------------------"
			 "----------\n");
	len += pif_print(where, pifBuff + len, "vch ch cap_ch cid nrP fps scM resolution enginMinor\n");
	for (i = 0; i < VPD_MAX_CAPTURE_CHANNEL_NUM; i++) {
		if (platform_sys_Info.cap_info[i].vcapch < 0) {
			continue;
		}
		snprintf(inStr, sizeof(inStr) - 1, "%dx%d",
			 platform_sys_Info.cap_info[i].width,
			 platform_sys_Info.cap_info[i].height);
		len += pif_print(where, pifBuff + len,
				 "%-3d %-2d %-6d %-3d %-3d %-3d %-3d %-10s 0x%04x\n",
				 i,
				 platform_sys_Info.cap_info[i].ch,
				 platform_sys_Info.cap_info[i].vcapch,
				 platform_sys_Info.cap_info[i].chipid,
				 platform_sys_Info.cap_info[i].num_of_path,
				 platform_sys_Info.cap_info[i].fps,
				 platform_sys_Info.cap_info[i].scan_method,
				 inStr,
				 platform_sys_Info.cap_info[i].engine_minor);
	}

	if (where & PIF_BUFFER) {
		sysfile = fopen(PIF_PRODUCT_FILENAME, "w+");
		if (sysfile == NULL) {
			return;
		}
		fwrite(pifBuff, 1, len, sysfile);
		fclose(sysfile);
		if (pifBuff) {
			PIF_FREE(pifBuff);
		}
	}
}

int pif_check_graphid(int id)
{
	if ((VPD_GET_CHIP_ID(id) >= VPD_MAX_CHIP_NUM) ||
	    (VPD_GET_CPU_ID(id) >= VPD_MAX_CPU_NUM_PER_CHIP) ||
	    (VPD_GET_GRAPH_ID(id) > VPD_MAX_GRAPH_NUM)) {
		return -1;
	}

	return 0;
}

void pif_add_graphList(vpd_graph_list_t *graphList, unsigned int id)
{
	uintptr_t quotient, remainder, graphid;
	int *list;

	graphid = VPD_GET_GRAPH_ID(id);
	GMLIB_L3_P("<graphId(%d)>\n", graphid);

	if (graphid > VPD_MAX_GRAPH_NUM) {
		GMLIB_ERROR_P("graphid(%d) error!! over %d.\n", graphid, VPD_MAX_GRAPH_NUM);
		return;
	}
	if (graphList == NULL) {
		GMLIB_ERROR_P("NULL graphList\n");
		return;
	}

	quotient = graphid / 32;
	remainder = graphid % 32;
	list = &graphList->array[quotient];
	*list |= (1 << remainder);
	return;
}

void pif_reset_graphList(vpd_graph_list_t *graphList)
{
	if (graphList == NULL) {
		GMLIB_ERROR_P("NULL graphList\n");
		return;
	}
	memset(graphList, 0, sizeof(vpd_graph_list_t));
}

void pif_transf_list(list_head_t *head, vpd_graph_list_t *graphList)
{

	list_head_t *pos, *next;
	unsigned int id;

	if (head == NULL) {
		GMLIB_ERROR_P("NULL head\n");
		return;
	}
	if (graphList == NULL) {
		GMLIB_ERROR_P("NULL graphList\n");
		return;
	}

	pif_reset_graphList(graphList);
	common_get_listEachEntry(pos, next, head) {
		if ((id = (unsigned int)pos->member) == 0) {
			continue;
		}
		pif_add_graphList(graphList, id);
	}
	return;
}

int pif_valid_addr(void *addr)
{
	long pagemask = -1, page_size;
	char c;
	void *v_addr;

	if (addr == NULL) {
		GMLIB_ERROR_P("NULL addr\n");
		return 0;
	}

	if (pagemask == -1) {
		page_size = sysconf(_SC_PAGESIZE);
		pagemask = ~(page_size - 1);
	}
	v_addr = (void *)((uintptr_t) addr & pagemask);
	if (mincore((void *)v_addr, 1, (unsigned char *)&c) == -1) {
		GMLIB_PRINT_P("%s:%d <errno =%d>\n", __FUNCTION__, __LINE__, errno);
		return 0;  /* invalid */
	} else {
		return 1;  /* valid */
	}
}

long long current_timestamp(void)
{
	struct timeval te;
	gettimeofday(&te, NULL);
	long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
	return milliseconds;
}

unsigned long current_timestamp32(void)
{
	struct timeval te;
	gettimeofday(&te, NULL);
	unsigned long milliseconds = te.tv_sec * 1000 + te.tv_usec / 1000;
	return milliseconds;
}

void pif_signal_info_execute(vpd_sig_info_t  *vpd_sig_info_data)
{
	int i, j;

	if (vpd_sig_info_data == NULL) {
		GMLIB_ERROR_P("NULL vpd_sig_info_data\n");
		return;
	}

	switch (vpd_sig_info_data->info_type) {
	case INFO_TYPE_HDAL_LOG:
		dif_setting_log(vpd_sig_info_data->hdal_data.msg);
		goto exit;
	case INFO_TYPE_HDAL_FLOW:
		hdal_flow_enable(vpd_sig_info_data->hdal_data.flow);
		goto exit;
	case INFO_TYPE_GMLIB_FLOW:
		gmlib_flow_enable(vpd_sig_info_data->hdal_data.msg);
		goto exit;
	case INFO_TYPE_PERF_LOG:
		if (vpd_sig_info_data->perf_log_info.observe_time > 4000) {
			printf("perf time too large(%d)\n", vpd_sig_info_data->perf_log_info.observe_time);
			goto exit;
		}
		pif_static_time = vpd_sig_info_data->perf_log_info.observe_time * 1000000;
		printf("Set perf update interval: %u sec\n", vpd_sig_info_data->perf_log_info.observe_time);
		goto exit;
	case INFO_TYPE_GMLIB_DBGMODE:
		if (vpd_sig_info_data->gmlib_dbg_mode.dbg_mode == 2) {
			printf("group fd               type  func   count      last_time(exec_time)\n");
			printf("-------------------------------------------------------------------\n");
			for (i = 0; i < GM_MAX_GROUP_NUM; i ++)
				for (j = 0; j < MAX_GM_NOTIFY_COUNT; j++) {
					if (pif_notify_info[i][j].fn_notify_handler)
						printf(" %3d   %20s  %p   %d   %10lld(%lld)\n", i, pif_notify_name[j],
						       pif_notify_info[i][j].fn_notify_handler,
						       pif_notify_info[i][j].count,
						       pif_notify_info[i][j].last_notify_time2,
						       pif_notify_info[i][j].last_notify_time2 - pif_notify_info[i][j].last_notify_time
						      );
				}
			printf("current time: %lld\n", current_timestamp());
			printf("-------------------------------------------------------------------\n");

		} else {
			gmlib_dbg_mode = vpd_sig_info_data->gmlib_dbg_mode.dbg_mode;
		}
		goto exit;
	case INFO_TYPE_GMLIB_DBGLEVEL:

#if 0//LIB_DEBUG_BUF_ALLOC
		hdal_show_alloc_statistic();
#endif
		gmlib_dbg_level = vpd_sig_info_data->gmlib_dbg_level.dbg_level;
		goto exit;

	case INFO_TYPE_SIGNAL_LOSS:
	case INFO_TYPE_SIGNAL_PRESENT:
	case INFO_TYPE_FRAMERATE_CHANGE:
	case INFO_TYPE_HW_CONFIG_CHANGE:
	case INFO_TYPE_TAMPER_ALARM:
	case INFO_TYPE_TAMPER_ALARM_RELEASE:
	case INFO_TYPE_AU_BUF_UNDERRUN:
	case INFO_TYPE_AU_BUF_OVERRUN:
		break;
	default:
		GMLIB_PRINT_P("Unknown sig info type(%d)\n", vpd_sig_info_data->info_type);
		break;
	}
exit:
	return;
}

static void *sig_info_dispatcher_thread(void *arg)
{
	int len;
	struct pollfd fds[1];
	vpd_sig_info_t sig_info;

	fds[0].fd = vpd_fd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	while (vpd_fd != 0) {
		if (poll(fds, 1, 100) <= 0) {
			continue;
		}
		if (fds[0].revents != POLLIN) {
			continue;
		}
		len = read(vpd_fd, &sig_info, sizeof(vpd_sig_info_t));
		if (len != sizeof(vpd_sig_info_t)) {
			GMLIB_PRINT_P("Unknown sig info len(%d)\n", len);
			continue;
		}
		pif_signal_info_execute(&sig_info);
	}
	return 0;
}

int pif_init(void)
{
	vpd_gmlib_dbg_t dbg = {0};
	int versionMinor, versionMajor;
	int wait_loop = 0, ret = 0, len = 0;
	gm_enc_multi_bitstream_t lib_multi_enc_struct;
	vpd_multi_dout_t vpd_multi_enc_struct;
	gm_dec_multi_bitstream_t lib_multi_dec_struct;
	vpd_multi_din_t vpd_multi_dec_struct;

	if ((pif_msg = (char *)PIF_MALLOC(PROC_MSG_SIZE)) == NULL) {
		GMLIB_PRINT_P("Error to allcate gmlib pif message buffer.\n");
		goto ext;
	}
	memset(pif_msg, 0x0, PROC_MSG_SIZE);
	len = snprintf(pif_msg, PROC_MSG_SIZE, "gm_init\n");
	gmlib_flow_log(pif_msg, len);

	pif_init_data.init_nr++;
	if (pif_init_data.init_nr > 1) {
		int wait_count = 0;
		while (pif_init_data.init_complete == 0) {
			wait_count ++ ;
			GMLIB_PRINT_P("wait initial.....(%d).\n", wait_count);
			usleep(100000);
		}
		goto ext;
	}
	pif_init_data.gpfd_nr = 0;

	sscanf(VPD_VERSION_CODE, "%d.%d\n", &versionMajor, &versionMinor);
	if (versionMajor > 256 || versionMinor > 256) {
		GMLIB_PRINT_P("version error! major(%d), minor(%d)\n", versionMajor, versionMinor);
		ret = ERR_VER_OVERFLOW;
		goto err_ext;
	}
	pif_version.version = ((versionMajor & 0x000000ff) << 8) | (versionMinor & 0x000000ff);
	strcpy(pif_version.compiler_date, __DATE__);
	strcpy(pif_version.compiler_time, __TIME__);

	if (pthread_mutex_init(&vpd_mutex, NULL)) {
		perror("pif_init: init vpd_mutex failed!");
		ret = ERR_CREATE_THREAD;
		goto err_ext;
	}

	if (pthread_mutex_init(&pif_dbg_alloc_mutex, NULL)) {
		perror("pif_init: init pif_dbg_alloc_mutex failed!");
		ret = ERR_FAILED;
		goto err_ext;
	}

	if (vpd_fd == 0) {
		vpd_fd = open("/dev/vpd", O_RDWR);
	}
	if (vpd_fd < 0) {
		ret = ERR_FAILED_OPEN_VPD_FILE;
		goto err_ext;
	}

	if ((ioctl(vpd_fd, VPD_GET_GMLIB_DBG_MODE, &dbg)) < 0) {
		perror("ioctl \"VPD_GET_GMLIB_DBG_MODE\" fail");
		ret = ERR_FAILED_OPEN_VPD_FILE;
		goto err_ext;
	}

	ret = pthread_create(&sig_info_dispatcher_id, NULL, sig_info_dispatcher_thread, (void *)0);
	if (ret < 0) {
		ret = ERR_FAILED;
		goto err_ext;
	}

	gmlib_dbg_level = dbg.dbg_level;
	gmlib_dbg_mode = dbg.dbg_mode;
	gmlib_quiet = dbg.dbg_quiet;
	GMLIB_L1_P("<gmlib_dbg_level=%d, gmlib_dbg_mode=%d, gmlib_quiet=%d>\n", gmlib_dbg_level,
		   gmlib_dbg_mode, gmlib_quiet);


	memset(&platform_sys_spec_Info, 0, sizeof(platform_sys_spec_Info));
	if ((ioctl(vpd_fd, VPD_GET_SYS_SPEC_INFO, &platform_sys_spec_Info)) < 0) {
		perror("ioctl \"VPD_GET_SYS_SPEC_INFO\" fail");
		ret = ERR_SYS_NOT_READY;
		goto err_ext;
	}

	//init env info again since driver info won't update after first init at boot
	while (pif_env_update() < 0) {
		/* waiting for vpd driver ready */
		if (++wait_loop > 30) {
			perror("ioctl \"pif_env_update\" fail");
			ret = ERR_SYS_NOT_READY;
			goto err_ext;
		}
		sleep(1);
	}

	memset(hdal_ddr_info, 0, VPD_MAX_DDR_NUM * sizeof(vpd_ddr_range_info_t));
	if (pif_get_mem_init_state() > 0) {
		if ((ioctl(vpd_fd, VPD_GET_HDAL_DDR_INFO, hdal_ddr_info)) < 0) {
			printf("ioctl \"VPD_GET_HDAL_DDR_INFO\" fail");
			ret = ERR_SYS_NOT_READY;
			goto err_ext;
		}
	}
	if ((ioctl(vpd_fd, VPD_GET_PLATFORM_VERSION, &vpd_version)) < 0) {
		perror("ioctl \"VPD_GET_PLATFORM_VERSION\" fail");
		ret = ERR_SYS_NOT_READY;
		goto err_ext;
	}

	if (pif_version.version != vpd_version.version) {
		GMLIB_PRINT_P("Videgoraph version is inconsistent between gmlib and driver!\n");
		GMLIB_PRINT_P("gmlib  version: %d.%d, Date:%s, Time:%s\n",
			      (pif_version.version & 0x0000ff00) >> 8,
			      pif_version.version  & 0x000000ff,
			      pif_version.compiler_date,
			      pif_version.compiler_time);
		GMLIB_PRINT_P("driver version: %d.%d, Date:%s, Time:%s\n",
			      (vpd_version.version & 0x0000ff00) >> 8,
			      vpd_version.version  & 0x000000ff,
			      vpd_version.compiler_date,
			      vpd_version.compiler_time);
		pif_init_data.init_nr = 0;
		ret = ERR_INVALID_LIB_VER;
		goto err_ext;
	}

	memset(pif_cap_collect_info, 0x0, sizeof(pif_cap_collect_info));
	memset(pif_lv_collect_info, 0x0, sizeof(pif_lv_collect_info));
	memset(pif_group, 0, sizeof(pif_group));

	/* check struct size coincidence */
	if (sizeof(vpd_channel_fd_t) > (BD_FD_MAX_PRIVATE_DATA * sizeof(int))) {
		GMLIB_ERROR_P("vpd_channel_fd_t(%zd) is bigger than BD_FD_MAX_PRIVATE_DATA(%d), "
			      "please check \"vpdFdinfo\" of HD_GROUP_LINE struct\n",
			      sizeof(vpd_channel_fd_t), (BD_FD_MAX_PRIVATE_DATA * sizeof(int)));
		ret = ERR_GRAPH_EXIT;
		goto err_ext;
	}

	if (sizeof(gm_pollfd_t) != sizeof(vpd_poll_obj_t)) {
		gm_pollfd_t *poll_fds;

		if (sizeof(gm_ret_event_t) != sizeof(vpd_poll_ret_events_t)) {
			GMLIB_ERROR_P("<vpd_poll_ret_events_t struct size error!>\n");
			ret = ERR_GRAPH_EXIT;
			goto err_ext;
		}
		if (sizeof(poll_fds->fd_private) != sizeof(vpd_channel_fd_t)) {
			GMLIB_ERROR_P("<vpd_channel_fd_t struct size error!>\n");
			ret = ERR_GRAPH_EXIT;
			goto err_ext;
		}
	}
	if (sizeof(lib_multi_enc_struct.enc_private) != sizeof(vpd_get_copy_dout_t)) {
		GMLIB_ERROR_P("lib_multi_enc_struct.enc_private struct size error!(%d, %d)\n",
			      sizeof(lib_multi_enc_struct.enc_private), sizeof(vpd_get_copy_dout_t));
		ret = ERR_GRAPH_EXIT;
		goto err_ext;
	}
	if ((sizeof(lib_multi_enc_struct) - sizeof(lib_multi_enc_struct.enc_private))
	    != sizeof(vpd_multi_enc_struct.user_private)) {
		GMLIB_ERROR_P("<vpd_multi_enc_struct.user_private size error! (%d %d)>\n",
			      (sizeof(lib_multi_enc_struct) - sizeof(lib_multi_enc_struct.enc_private)),
			      sizeof(vpd_multi_enc_struct.user_private));
		ret = ERR_GRAPH_EXIT;
		goto err_ext;
	}
	if (sizeof(lib_multi_dec_struct.dec_private) != sizeof(vpd_put_copy_din_t)) {
		GMLIB_ERROR_P("lib_multi_dec_struct.dec_private struct size error!(%d, %d)\n",
			      sizeof(lib_multi_dec_struct.dec_private), sizeof(vpd_put_copy_din_t));
		ret = ERR_GRAPH_EXIT;
		goto err_ext;
	}
	if ((sizeof(lib_multi_dec_struct) - sizeof(lib_multi_dec_struct.dec_private))
	    != sizeof(vpd_multi_dec_struct.user_private)) {
		GMLIB_ERROR_P("<vpd_multi_dec_struct.user_private size error! (%d %d)>\n",
			      (sizeof(lib_multi_dec_struct) - sizeof(lib_multi_dec_struct.dec_private)),
			      sizeof(vpd_multi_dec_struct.user_private));
		ret = ERR_GRAPH_EXIT;
		goto err_ext;
	}

	pif_print_platformInfo(PIF_LOG | PIF_BUFFER);

	ret = pif_set_for_graph_exit();
	if (ret < 0) {
		GMLIB_PRINT_P("Error \"Abnormal AP exit handling\" fail");
		ret = ERR_GRAPH_EXIT;
		goto err_ext;
	}

	if ((pif_update_entity_buf = PIF_MALLOC(pif_update_entity_buf_len)) == NULL) {
		GMLIB_ERROR_P("alloc pif_update_entity_buf fail!\n");
		ret = -1;
		goto err_ext;
	}
	if (ms_ioctl == 0) {
		pif_ms_ioctl_init();
	}
	if (gs_ioctl == 0) {
		gs_ioctl = open("/dev/gs", O_RDWR);
	}
	if (devmem_fd == 0) {
		devmem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	}

	if (vcap_ioctl == 0) {
		vcap_ioctl = open("/dev/vcap0", O_RDWR);
	}
	if (vcap_ioctl > 0) {
		pif_vcap_init();
	} else {
		GMLIB_L1_P("No support vcap module...\n");
	}

	if (osg_ioctl == 0) {
		osg_ioctl = open("/dev/kflow_osg", O_RDWR);
	}
	if (osg_ioctl > 0) {
		pif_osg_init();
	} else {
		GMLIB_L1_P("No support OSG module ...\n");
	}

	if (vpe_ioctl == 0) {
		vpe_ioctl = open("/dev/kflow_vpe", O_RDWR);
	}
	if (vpe_ioctl > 0) {
		pif_vpe_init();
	} else {
		GMLIB_L1_P("No support VPE(signal loss pattern) module ...\n");
	}

	if (au_ioctl == 0) {
		au_ioctl = open("/dev/kflow_audioio", O_RDWR);
	}
	if (au_ioctl > 0) {
		pif_au_init();
	} else {
		GMLIB_L1_P("No support AUDIO module ...\n");
	}

	memset((void *)pif_target_disp_rate, 0, (sizeof(int) * VPD_MAX_LCD_NUM));

ext:
	pif_init_data.init_complete = 1;
	GMLIB_L3_P("<pif_init_data.init_nr(%d)>\n", pif_init_data.init_nr);
	return 0;
err_ext:
	return ret;
}

pif_group_t *pif_get_group_by_groupfd(int groupfd)
{
	int groupidx = groupfd;
	pif_group_t *group;

	if (groupidx < GM_GROUP_FD_START || groupidx >= GM_MAX_GROUP_NUM) {
		goto err_ext;
	}
	if (((group = pif_group[groupidx]) == NULL) || (group->in_use == 0)) {
		goto err_ext;
	}
	return group;

err_ext:
	return 0;
}

int pif_release(void)
{
	int ret = 0, i, len = 0;

	memset(pif_msg, 0x0, PROC_MSG_SIZE);
	len = snprintf(pif_msg, PROC_MSG_SIZE, "gm_release\n");
	gmlib_flow_log(pif_msg, len);

	pif_init_data.init_nr--;
	GMLIB_L3_P("<pif_release: pif_init_data.init_nr(%d)>\n", pif_init_data.init_nr);
	if (pif_init_data.init_nr > 0) {
		goto ext;
	} else if (pif_init_data.init_nr < 0) {
		ret = ERR_NOT_INIT_PLATFORM;
		GMLIB_ERROR_P("Error gm_release! gm_init not used before.\n");
		goto ext;
	}

	if (pif_init_data.gpfd_nr) {
		ret = ERR_INVALID_OPERATION_GROUPFD_NO_DELETE;
		GMLIB_ERROR_P("%d of groupfd not release!\n", pif_init_data.gpfd_nr);
		goto ext;
	}

	if (pif_update_entity_buf) {
		PIF_FREE(pif_update_entity_buf);
	}
	pthread_mutex_destroy(&vpd_mutex);
	pthread_mutex_destroy(&pif_dbg_alloc_mutex);

	for (i = 0; i <= max_group_idx; i++) {
		if (pif_group[i]) {
			if (pif_group[i]->encap_buf) {
				PIF_FREE(pif_group[i]->encap_buf);
				pif_group[i]->encap_buf = NULL;
			}
			PIF_FREE(pif_group[i]);
			pif_group[i] = NULL;
		}
	}
	max_group_idx = 0;
	if (vpd_fd) {
		close(vpd_fd);
		vpd_fd = 0;
	}
	if (vcap_ioctl) {
		close(vcap_ioctl);
		vcap_ioctl = 0;
		pif_vcap_release();
	}
	if (osg_ioctl) {
		close(osg_ioctl);
		osg_ioctl = 0;
		pif_osg_release();
	}
	if (vpe_ioctl) {
		close(vpe_ioctl);
		vpe_ioctl = 0;
		pif_vpe_release();
	}
	if (au_ioctl) {
		close(au_ioctl);
		au_ioctl = 0;
		pif_au_release();
	}
	if (ms_ioctl) {
		pif_ms_ioctl_release();
		ms_ioctl = 0;
	}
	if (gs_ioctl) {
		close(gs_ioctl);
		gs_ioctl = 0;
	}
	if (devmem_fd) {
		close(devmem_fd);
		devmem_fd = 0;
	}
	if (pif_msg) {
		PIF_FREE(pif_msg);
		pif_msg = 0;
	}
	pthread_join(sig_info_dispatcher_id, NULL);

ext:
	return ret;
}

int pif_get_entityStructSize(vpd_entity_type_t type)
{
	int size = 0;

	switch (type) {
	case VPD_CAP_ENTITY_TYPE:
		size = sizeof(vpd_cap_entity_t);
		break;
	case VPD_DEC_ENTITY_TYPE:
		size = sizeof(vpd_dec_entity_t);
		break;
	case VPD_DI_ENTITY_TYPE:
		size = sizeof(vpd_di_entity_t);
		break;
	case VPD_VPE_1_ENTITY_TYPE:
		size = sizeof(vpd_vpe_1_entity_t);
		break;
	case VPD_DISP_ENTITY_TYPE:
		size = sizeof(vpd_disp_entity_t);
		break;
	case VPD_DIN_ENTITY_TYPE:
		size = sizeof(vpd_din_entity_t);
		break;
	case VPD_DOUT_ENTITY_TYPE:
		size = sizeof(vpd_dout_entity_t);
		break;
	case VPD_H26X_ENC_ENTITY_TYPE:
		size = sizeof(vpd_h26x_enc_entity_t);
		break;
	case VPD_MJPEGE_ENTITY_TYPE:
		size = sizeof(vpd_mjpege_entity_t);
		break;
	case VPD_AU_GRAB_ENTITY_TYPE:
		size = sizeof(vpd_au_grab_entity_t);
		break;
	case VPD_AU_RENDER_ENTITY_TYPE:
		size = sizeof(vpd_au_render_entity_t);
		break;
	case VPD_OSG_ENTITY_TYPE:
		size = sizeof(vpd_osg_entity_t);
		break;
	case VPD_3DNR_ENTITY_TYPE:
		break;
	case VPD_NONE_ENTITY_TYPE:
	case VPD_TOTAL_ENTITY_CNT:
	default:
		GMLIB_ERROR_P("entity type error %d!\n", type);
		return 0;
	}
	return size;
}

void *pif_get_nextEntity(vpd_graph_t *vpd_graph, void *head_addr)
{
	void *next_head_addr;
	unsigned int offs;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("get_next: NULL vpd_graph\n");
		return NULL;
	}
	if (head_addr == NULL) { /* NULL: get first entity offset */
		offs = vpd_graph->entity_offs;
	} else { /* get next entity offset */
		offs = VPD_GET_NEXT_ENTITY_OFFSET_VAL(head_addr);
	}
	if (offs == 0) { /* no next entity */
		return NULL;
	}
	next_head_addr = VPD_GET_ENTITY_HEAD_ADDR(vpd_graph, offs);
	PIF_CHECK_ENTITY(next_head_addr);
	return next_head_addr;
}

int pif_get_entitynr(vpd_graph_t *vpd_graph)
{
	void *head_addr = NULL;
	int total_number = 0;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("get_entitynr: NULL vpd_graph\n");
		return 0;
	}

	while ((head_addr = pif_get_nextEntity(vpd_graph, head_addr)) != NULL) {
		total_number++;
	}
	return total_number;
}

void *pif_lookup_entity_buf(vpd_entity_type_t type, int sn, vpd_graph_t *vpd_graph)
{
	void *head_addr = NULL;
	vpd_entity_t *entity;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("lookup_entity_buf: NULL vpd_graph\n");
		return NULL;
	}

	while ((head_addr = pif_get_nextEntity(vpd_graph, head_addr)) != NULL) {
		entity = VPD_GET_ENTITY_ENTRY_ADDR(head_addr);
		if (entity->type == type && entity->sn == sn) {
			return entity;
		}
	}
	return NULL;
}

void *pif_lookup_first_entity_buf(vpd_entity_type_t type, vpd_graph_t *vpd_graph)
{
	void *head_addr = NULL;
	vpd_entity_t *entity;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("lookup_first_entity_buf: NULL vpd_graph\n");
		return NULL;
	}

	while ((head_addr = pif_get_nextEntity(vpd_graph, head_addr)) != NULL) {
		entity = VPD_GET_ENTITY_ENTRY_ADDR(head_addr);
		if (entity->type == type) {
			return entity;
		}
	}
	return NULL;
}

unsigned int pif_get_entity_encapsbuf_size(vpd_entity_t *entity)
{
	if (entity == NULL) {
		GMLIB_ERROR_P("NULL entity\n");
		return 0;
	}
	return (VPD_ENTITY_HEAD_LEN + sizeof(vpd_entity_t) + pif_get_entityStructSize(entity->type));
}

unsigned int pif_get_total_entity_encapsbuf_size(vpd_graph_t *vpd_graph)
{
	void *head_addr = NULL;
	vpd_entity_t *entity;
	unsigned int len = 0;

	while ((head_addr = pif_get_nextEntity(vpd_graph, head_addr)) != NULL) {
		entity = VPD_GET_ENTITY_ENTRY_ADDR(head_addr);
		len += pif_get_entity_encapsbuf_size(entity);
	}
	return (len);
}

void *pif_get_free_entity_encapsbuf(vpd_graph_t *vpd_graph)
{
	vpd_entity_t *entity;
	void *head_addr = NULL, *free_head_addr;
//++ TODO
#if 1
	unsigned int *offs_addr, entity_len;
	uintptr_t offs_chk;
#else
	unsigned int *offs, entity_len;
	unsigned int offs_chk;
#endif

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("get free buf: NULL vpd_graph\n");
		return 0;
	}

	while ((head_addr = pif_get_nextEntity(vpd_graph, head_addr)) != NULL) {
		offs_addr = VPD_GET_NEXT_ENTITY_OFFSET_ADDR(head_addr);
		if (*offs_addr == 0) {   /* 0: means last one */
			/* set next entity head offset */
			entity = VPD_GET_ENTITY_ENTRY_ADDR(head_addr);
			entity_len = pif_get_entity_encapsbuf_size(entity);

			/* calculate offset */
			offs_chk = (uintptr_t)((head_addr + entity_len) - (uintptr_t)vpd_graph);
			if (offs_chk >= 0x9ffff) {
				GMLIB_ERROR_P("Offset is overflow! offs(%#x) is over 0x9ffff)\n", offs_chk);
				return 0;
			}
			*offs_addr = (unsigned int)offs_chk;

			/* calculate free head addr */
			free_head_addr = (void *)vpd_graph + *offs_addr;

			return free_head_addr;
		}
	}
	GMLIB_ERROR_P("get free buf error!\n");
	return 0;
}

int pif_check_encaps_len_for_entity(vpd_graph_t *graph, vpd_entity_type_t type)
{
	vpd_entity_t *entity;
	void *entity_head = NULL, *free_head_addr = NULL;
	uintptr_t entity_offs;
	void *encaps_start, *encaps_end;
	int total_len, ret = 1;
	unsigned int entity_len;

	if (graph == NULL) {
		GMLIB_ERROR_P("NULL vpd_graph\n");
		return -1;
	}
	if (graph->entity_offs == 0) {  /* first one entity */
		entity_offs = sizeof(vpd_graph_t);
		entity_head = VPD_GET_ENTITY_HEAD_ADDR(graph, entity_offs);
	} else {
		while ((free_head_addr = pif_get_nextEntity(graph, free_head_addr)) != NULL) {
			entity_offs = VPD_GET_NEXT_ENTITY_OFFSET_VAL(free_head_addr);
			if (entity_offs == 0) {   /* 0: means last one */
				entity = VPD_GET_ENTITY_ENTRY_ADDR(free_head_addr);
				entity_len = pif_get_entity_encapsbuf_size(entity);
				/* calculate free head addr */
				entity_head = (void *)graph +
					      (uintptr_t)(free_head_addr + entity_len - (uintptr_t)graph);
				break;
			}
		}
	}
	entity_len = VPD_ENTITY_HEAD_LEN + sizeof(vpd_entity_t) + pif_get_entityStructSize(type);

	encaps_start = graph;
	encaps_end = entity_head + entity_len;
	total_len = encaps_end - encaps_start;
	if (total_len >= graph->buf_size) {
		ret = -1;
	}
	return ret;
}

void pif_init_entity_encapsbuf(void *buf)
{
	VPD_SET_ENTITY_MAGIC(buf);
	VPD_SET_NEXT_ENTITY_OFFSET(buf, 0);
	return;
}

void pif_dump(void *addr, unsigned int len)
{
	unsigned int dumploop;

	for (dumploop = 0; dumploop < (len + 8); dumploop += 16) {
		GMLIB_ERROR_P("<%p: 0x%08x 0x%08x 0x%08x 0x%08x>\n",
			      (void *)(addr + dumploop),
			      *(int *)(addr + dumploop),
			      *(int *)(addr + dumploop + 4),
			      *(int *)(addr + dumploop + 8),
			      *(int *)(addr + dumploop + 12));
	}
}

void pif_dump_entity_buf(vpd_graph_t *vpd_graph)
{
	void *head_addr = NULL;
	vpd_entity_t *entity;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("vpd_graph = NULL\n");
		return;
	}

	GMLIB_ERROR_P("=================================================================================\n");
	GMLIB_ERROR_P("vpd_graph(%p): name(%s) len(%d) buf_size(%d) entity_nr(%d) line_nr(%d) entity_offs(%#x) line_offs(%#x)\n",
		      vpd_graph, vpd_graph->name, vpd_graph->len, vpd_graph->buf_size, vpd_graph->entity_nr,
		      vpd_graph->line_nr, vpd_graph->entity_offs, vpd_graph->line_offs);
	while ((head_addr = pif_get_nextEntity(vpd_graph, head_addr)) != NULL) {
		entity = VPD_GET_ENTITY_ENTRY_ADDR(head_addr);
		GMLIB_ERROR_P("=================== entity ===============================\n");
		pif_dump(entity, sizeof(vpd_entity_t));
		entity->e = (void *)entity + sizeof(vpd_entity_t);
		GMLIB_ERROR_P("=================== entity->e ===============================\n");
		pif_dump(entity->e, 64);
	}
}

void pif_rearrange_entity_buf(vpd_graph_t *vpd_graph)
{
	void *head_addr = NULL;
	vpd_entity_t *entity;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("rearrange_entity_buf: NULL vpd_graph\n");
		return;
	}

	while ((head_addr = pif_get_nextEntity(vpd_graph, head_addr)) != NULL) {
		entity = VPD_GET_ENTITY_ENTRY_ADDR(head_addr);
		entity->e = (void *)entity + sizeof(vpd_entity_t);
	}
}

int pif_set_EntityBuf(vpd_entity_type_t type, vpd_entity_t **entity, vpd_graph_t *vpd_graph)
{
	pif_group_t *group;
	vpd_graph_t *graph;
	unsigned int len = 0;
	void *entity_head;
	int buf_size;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("set_EntityBuf: NULL vpd_graph\n");
		return 0;
	}

	/*       Magic          + offset            + vpd_entity_t         + real_entity(entity->e) */
	/* len = entityMagic(4) + nextEntityOffs(4) + sizeof(vpd_entity_t) + entityStructSize       */
	/* len = 4byte          + 4byte             + sizeof(vpd_entity_t) + pif_get_entityStructSize(type) */

	graph = vpd_graph;
	if (pif_check_encaps_len_for_entity(vpd_graph, type) < 0) { /* check enccps len */
		/* realloc the encap_buf for graph */
		group = vpd_graph->group;
		buf_size = graph->buf_size;
		if (pif_get_entityStructSize(type) > VPD_MAX_BUFFER_LEN) {
			buf_size += ((pif_get_entityStructSize(type) + 0xfff) & 0xfffff000);
		}
		buf_size += VPD_MAX_BUFFER_LEN;
		graph = PIF_REALLOC(group->encap_buf, buf_size);
		if (graph == NULL) {
			GMLIB_ERROR_P("realloc fail, graph = NULL, buf_size(%d)!\n", buf_size);
			len = -1;
			goto ext;
		}

		graph->buf_size = buf_size;
		if (graph != group->encap_buf) {
			pif_rearrange_entity_buf(graph);
		}
		group->encap_buf = graph;
		GMLIB_L1_P("set entity type(%#x) groupidx(%d) encaps_buf change size(%d)!\n", type, group->groupidx, buf_size);
	}

	graph->entity_nr++;
	/* get free entity_head */
	if (graph->entity_offs == 0) {  /* first one entity */
		graph->entity_offs = sizeof(vpd_graph_t);
		entity_head = VPD_GET_ENTITY_HEAD_ADDR(graph, graph->entity_offs);
	} else {
		entity_head = pif_get_free_entity_encapsbuf(graph);
	}
	if (entity_head == NULL) {
		GMLIB_ERROR_P("pif_set_EntityBuf fail, entity_head is NULL\n");
		return 0;
	}
	pif_init_entity_encapsbuf(entity_head);
	len += VPD_ENTITY_HEAD_LEN;  /* 1. add the length of entity_head */
	*entity = (vpd_entity_t *)(entity_head + len);
	len += sizeof(vpd_entity_t);     /* 2. add the length of vpd_entity_t */
	(*entity)->e = (entity_head + len);
	len += pif_get_entityStructSize(type);  /* 3. add the length of entityStructureSize */
	(*entity)->type = type;
	(*entity)->feature = 0;
	(*entity)->extra_buffers = 0;
	(*entity)->group_id = GMLIB_NULL_BYTE;
	GMLIB_L2_P("<type=%d, entity=%p>\n", (*entity)->type, (*entity)->e);
	memset((*entity)->e, GMLIB_NULL_BYTE, pif_get_entityStructSize(type));
ext:
	return len;
}

void *pif_get_nextLine(vpd_graph_t *vpd_graph, void *line)
{
	void *next_line;
	unsigned int offs;
	unsigned int magic;

	if (line == NULL) { /* first line */
		offs = vpd_graph->line_offs;
	} else {          /* get next line offset */
		offs = VPD_GET_NEXT_LINE_OFFSET_VAL(line);
	}

	if (offs == 0) {
		return NULL;
	}
	next_line = VPD_GET_LINE_HEAD_ADDR(vpd_graph, offs);

	magic = VPD_GET_LINE_MAGIC_VAL(next_line);
	if (magic != VPD_LINE_MAGIC_NUM) {
		GMLIB_ERROR_P("line magic(%#x) error!\n", magic);
		return 0;
	}
	return next_line;
}

void *pif_get_free_line_encapsbuf(vpd_graph_t *vpd_graph)
{
	void *line = NULL, *free_line_head = NULL;
	unsigned int offs;
	int line_len;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("get_free_line: NULL vpd_graph\n");
		return 0;
	}

	/* first free line */
	if (vpd_graph->line_offs == 0) {
		vpd_graph->line_offs = sizeof(vpd_graph_t) + pif_get_total_entity_encapsbuf_size(vpd_graph);
		free_line_head = VPD_GET_LINE_HEAD_ADDR(vpd_graph, vpd_graph->line_offs);
		goto ext;
	}

	/* next free line */
	while ((line = pif_get_nextLine(vpd_graph, line)) != NULL) {
		offs = VPD_GET_NEXT_LINE_OFFSET_VAL(line);
		if (offs == 0) {   /* 0: means last one */
			line_len = VPD_LINE_HEAD_LEN + (VPD_LINK_LEN * VPD_GET_LINK_NUM_VAL(line));
			/* calculate next free buffer address */
			free_line_head = (void *)(line + line_len);
			/* set next entity offset */
			VPD_SET_NEXT_LINE_OFFSET(line, ((uintptr_t)free_line_head - (uintptr_t)vpd_graph));
			goto ext;
		}
	}
	GMLIB_ERROR_P("get free line error!\n");
	return 0;
ext:
	return free_line_head;
}

int pif_check_encaps_len_for_graphline(vpd_graph_t *graph, unsigned int link_nr)
{
	void *line = NULL, *free_line_head = NULL;
	unsigned int offs;
	void *encaps_start, *encaps_end;
	int total_len, ret = 1;
	unsigned int line_len;

	if (graph == NULL) {
		GMLIB_ERROR_P("NULL vpd_graph\n");
		return -1;
	}

	if (graph->line_offs == 0) {  /* first one entity */
		offs = sizeof(vpd_graph_t) + pif_get_total_entity_encapsbuf_size(graph);
		free_line_head = VPD_GET_LINE_HEAD_ADDR(graph, offs);
	} else {
		/* next free line */
		while ((line = pif_get_nextLine(graph, line)) != NULL) {
			offs = VPD_GET_NEXT_LINE_OFFSET_VAL(line);
			if (offs == 0) {   /* 0: means last one */
				line_len = VPD_LINE_HEAD_LEN + (VPD_LINK_LEN * VPD_GET_LINK_NUM_VAL(line));
				/* calculate next free buffer address */
				free_line_head = (void *)(line + line_len);
				break;
			}
		}
	}
	if (free_line_head == NULL) {
		GMLIB_ERROR_P("free_line_head = NULL!\n");
		return -1;
	}

	encaps_start = graph;
	encaps_end = free_line_head + VPD_LINE_LEN(link_nr);
	total_len = encaps_end - encaps_start;
	if (total_len >= graph->buf_size) {
		ret = -1;
	}
	return ret;
}



void pif_fillin_line_member(vpd_line_t *vpd_line, void *line)
{
	unsigned int i;

	if (vpd_line == NULL) {
		GMLIB_ERROR_P("NULL vpd_line\n");
		return;
	}
	if (line == NULL) {
		GMLIB_ERROR_P("NULL line\n");
		return;
	}

	vpd_line->line_magic = VPD_GET_LINE_MAGIC_VAL(line);
	vpd_line->next_lineoffs = VPD_GET_NEXT_LINE_OFFSET_VAL(line);
	vpd_line->line_idx = VPD_GET_LINE_IDX_VAL(line);
	vpd_line->link_nr = VPD_GET_LINK_NUM_VAL(line);
	for (i = 0; i < vpd_line->link_nr; i++) {
		vpd_line->link[i].in_offs = VPD_GET_LINK_IN_VAL(line, i);
		vpd_line->link[i].out_offs = VPD_GET_LINK_OUT_VAL(line, i);
	}
}

int pif_get_linenr(vpd_graph_t *vpd_graph)
{
	void *line = NULL;
	int total_number = 0;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("get_linenr: NULL vpd_graph\n");
		return 0;
	}

	while ((line = pif_get_nextLine(vpd_graph, line)) != NULL) {
		total_number++;
	}
	return total_number;
}

int pif_lookup_link(vpd_graph_t *vpd_graph, unsigned int in_offs, unsigned int out_offs)
{
	void *line = NULL;
	vpd_line_t vpd_line;
	unsigned int i;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("lookup_link: NULL vpd_graph\n");
		return 0;
	}

	PIF_CHECK_ENTITY_OFFSET(vpd_graph, in_offs);
	PIF_CHECK_ENTITY_OFFSET(vpd_graph, out_offs);
	while ((line = pif_get_nextLine(vpd_graph, line)) != NULL) {
		pif_fillin_line_member(&vpd_line, line);
		for (i = 0; i < vpd_line.link_nr; i++) {
			PIF_CHECK_ENTITY_OFFSET(vpd_graph, vpd_line.link[i].in_offs);
			PIF_CHECK_ENTITY_OFFSET(vpd_graph, vpd_line.link[i].out_offs);
			if (vpd_line.link[i].in_offs == in_offs && vpd_line.link[i].out_offs == out_offs) {
				return 1;
			}
		}
	}
	return 0;
}

int pif_set_line(vpd_graph_t *vpd_graph, vpd_line_t *vpd_line, void *line_head)
{
	unsigned int len = 0;
	unsigned int idx;

	if (vpd_graph == NULL) {
		GMLIB_ERROR_P("set_line: NULL vpd_graph\n");
		return 0;
	}
	if (vpd_line == NULL) {
		GMLIB_ERROR_P("set_line: NULL vpd_line\n");
		return 0;
	}
	if (line_head == NULL) {
		GMLIB_ERROR_P("set_line: NULL line_head\n");
		return 0;
	}

	vpd_graph->line_nr ++;
	len = VPD_LINE_LEN(vpd_line->link_nr);
	VPD_SET_LINE_MAGIC(line_head);
	VPD_SET_NEXT_LINE_OFFSET(line_head, 0);
	VPD_SET_LINE_IDX_VAL(line_head, vpd_line->line_idx);
	VPD_SET_LINK_NUM_VAL(line_head, vpd_line->link_nr);
	for (idx = 0; idx < vpd_line->link_nr; idx++) {
		VPD_SET_LINK_IN_VAL(line_head, idx, vpd_line->link[idx].in_offs);
		VPD_SET_LINK_OUT_VAL(line_head, idx, vpd_line->link[idx].out_offs);
	}
	GMLIB_L2_P("len(%d) line_idx(%d) link_nr(%d)\n", len, vpd_line->line_idx, vpd_line->link_nr);
	return len;
}

int pif_clear_group_id(pif_group_t *group)
{
	int ret = -1;
	vpd_graph_t *vpd_graph = NULL;

	if ((vpd_graph = group->encap_buf) == NULL) {
		goto ext;
	}
	vpd_graph->id = 0;

	ret = 0;

ext:
	return ret;
}

int pif_preset_group(pif_group_t *group)
{
	vpd_graph_t *vpd_graph = NULL;
	HD_GROUP *p_hd_group = NULL;
	vpd_channel_fd_t *vpd_fd = NULL;
	int graph_id = 0, line_idx;

	if (group == NULL) {
		GMLIB_ERROR_P("preset: NULL group\n");
		goto ext;
	}
	if (group->hd_group == NULL) {
		GMLIB_ERROR_P("preset: NULL group->hd_group\n");
		goto ext;
	}
	p_hd_group = (HD_GROUP *)group->hd_group;

	/* init */
	if ((vpd_graph = group->encap_buf) != NULL) {
		if (vpd_graph->valid & PIF_APPLY_FORCE_GRAPH_EXIT) { // check for force_graph_exit(abnormal graph)
			vpd_graph->valid = 1;
		} else {
			vpd_graph->valid = 0;
		}
		vpd_graph->len = 0;
		vpd_graph->entity_nr = 0;
		vpd_graph->line_nr = 0;
		vpd_graph->entity_offs = 0;
		vpd_graph->line_offs = 0;
	}

	/* new vpd graph buffer */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (p_hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE)  {
			continue;
		}

		/* get graph_id */
		vpd_fd = (vpd_channel_fd_t *) & (p_hd_group->glines[line_idx].vpdFdinfo[0]);
		if (vpd_fd->id != 0) {
			graph_id = vpd_fd->id;
		} else {
			graph_id = VPD_SET_ID_FOR_CHIP_CPU_GRAPH(0, 0, 0);
		}
		vpd_fd->id = graph_id;

		/* alloc memory for encapsulate graph buf */
		if (group->encap_buf == NULL) {
			if ((vpd_graph = PIF_CALLOC(1, VPD_MAX_BUFFER_LEN)) == NULL) {
				GMLIB_ERROR_P("alloc encap_buf fail!\n");
				return -1;
			}
			vpd_graph->buf_size = VPD_MAX_BUFFER_LEN;
			vpd_graph->valid = 1;
			vpd_graph->id = graph_id;
			vpd_graph->group = group;
			vpd_graph->entity_nr = 0;
			group->encap_buf = vpd_graph;
		} else {
			vpd_graph = group->encap_buf;
			vpd_graph->valid = 1;
			if (vpd_graph->id == 0) { //for create a new graph
				vpd_graph->id = graph_id;
			}
		}
	}
	return 0;

ext:
	return -1;
}

int pif_encaps_graph_for_param(pif_group_t *group)
{
	vpd_graph_t *vpd_graph = NULL;
	int len = 0, line_idx, entityidx, buf_size;
	unsigned int in, out;
	vpd_line_t vpd_line;
	void *lineHead;
	HD_GROUP *p_hd_group;

	if (group == NULL) {
		GMLIB_ERROR_P("encaps: NULL group\n");
		goto ext;
	}
	if (group->encap_buf == NULL) {
		GMLIB_ERROR_P("encaps: NULL group->encap_buf\n");
		goto ext;
	}
	if (group->hd_group == NULL) {
		GMLIB_ERROR_P("encaps: NULL group->hd_group\n");
		goto ext;
	}

	p_hd_group = (HD_GROUP *)group->hd_group;
	vpd_graph = (vpd_graph_t *)group->encap_buf;
	len = p_hd_group->entity_buf_len;

	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (p_hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE)  {
			continue;
		}

		vpd_line.line_magic = VPD_LINE_MAGIC_NUM;
		vpd_line.next_lineoffs = 0;
		vpd_line.line_idx = 0;
		vpd_line.link_nr = 0;
		memset(vpd_line.link, 0, sizeof(vpd_line.link));

		/* collect line information */
		for (entityidx = 0; entityidx < (GM_MAX_LINE_ENTITY_NUM - 1); entityidx++) {
			if (p_hd_group->entity_buf_offs[line_idx][entityidx + 1] == 0) {
				break;
			}
			in = p_hd_group->entity_buf_offs[line_idx][entityidx];
			out = p_hd_group->entity_buf_offs[line_idx][entityidx + 1];
			if (pif_lookup_link(vpd_graph, in, out) == 1) { /* already set */
				continue;
			}
			vpd_line.link[(int)vpd_line.link_nr].in_offs = in;
			vpd_line.link[(int)vpd_line.link_nr].out_offs = out;
			vpd_line.link_nr++;
		}

		/* set line information to encapsulate buf, len is line lenght */
		if (vpd_line.link_nr == 0) { /* no link */
			continue;
		}
		/* check encaps buffer */
		if ((pif_check_encaps_len_for_graphline(vpd_graph, vpd_line.link_nr)) < 0) {
			/* realloc the encap_buf for graph */
			buf_size = vpd_graph->buf_size + VPD_MAX_BUFFER_LEN;
			vpd_graph = PIF_REALLOC(group->encap_buf, buf_size);
			if (vpd_graph == NULL) {
				GMLIB_ERROR_P("graph realloc fail\n");
				break;
			}
			if (group->encap_buf != vpd_graph) {
				pif_rearrange_entity_buf(vpd_graph);
			}
			vpd_graph->buf_size = buf_size;
			group->encap_buf = vpd_graph;
			GMLIB_L1_P("encaps graph groupidx(%d) encaps_buf change size(%d)!\n", group->groupidx, buf_size);
		}
		/* &vpd_line.line_idx : get line index  */
		vpd_line.line_idx = line_idx;
		lineHead = pif_get_free_line_encapsbuf(vpd_graph);
		if (lineHead == NULL) {
			GMLIB_ERROR_P("pif_get_free_line_encapsbuf fail\n");
			break;
		}
		len += pif_set_line(vpd_graph, &vpd_line, lineHead);
	}
	len += sizeof(vpd_graph_t);   // totol length
	vpd_graph = group->encap_buf;
	vpd_graph->len = len;
	return 0;

ext:
	return -1;
}

int pif_set_graph_type(pif_group_t *group)
{
	int line_idx, member_idx;
	int is_vcap = 0, is_vdec = 0, is_venc = 0, is_vout = 0;
	int is_acap = 0, is_adec = 0, is_aenc = 0, is_aout = 0;
	HD_GROUP *p_hd_group;
	HD_MEMBER *p_member;
	vpd_graph_t *vpd_graph;

	if (group == NULL) {
		GMLIB_ERROR_P("set type: NULL pif_group\n");
		goto exit;
	}
	p_hd_group = (HD_GROUP *)group->hd_group;
	if (p_hd_group == NULL) {
		GMLIB_ERROR_P("set type: NULL HD_GROUP\n");
		goto exit;
	}
	if ((vpd_graph = group->encap_buf) == NULL) {
		GMLIB_ERROR_P("set type: NULL vpd_graph\n");
		goto exit;
	}

	/* check line type */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (p_hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE)  {
			continue;
		}
		for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
			if (p_hd_group->glines[line_idx].member[member_idx].p_bind == NULL)
				continue;

			p_member = &(p_hd_group->glines[line_idx].member[member_idx]);
			if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE) {
				is_vcap = 1;
			}
			if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) {
				is_vdec = 1;
			}
			if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOENC_BASE) {
				is_venc = 1;
			}
			if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
				is_vout = 1;
			}
			if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_AUDIOCAP_BASE) {
				is_acap = 1;
			}
			if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_AUDIODEC_BASE) {
				is_adec = 1;
			}
			if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_AUDIOENC_BASE) {
				is_aenc = 1;
			}
			if (bd_get_dev_baseid(p_member->p_bind->device.deviceid) == HD_DAL_AUDIOOUT_BASE) {
				is_aout = 1;
			}
		}
	}

	if (is_vcap && is_venc && !is_vout) {
		//vpd_graph->type = GRAPH_ENCODE;
		vpd_graph->type = GRAPH_DISPLAY; // for video same graph
	} else if (is_acap && is_aenc) {
		vpd_graph->type = GRAPH_AU_GRAB;
	} else if (is_acap && is_aout) {
		vpd_graph->type = GRAPH_AU_LIVESOUND;
	} else if (is_adec && is_aout) {
		vpd_graph->type = GRAPH_AU_RENDER;
	} else if (is_vdec && is_venc && !is_vout) {
		//vpd_graph->type = GRAPH_FILE_ENC;
		vpd_graph->type = GRAPH_DISPLAY; // for video same graph
	} else {
		vpd_graph->type = GRAPH_DISPLAY;
	}
	return 0;

exit:
	return -1;
}

int pif_clear_vpd_channel_fd(int groupfd, int line_idx)
{
	HD_GROUP *p_hd_group;
	pif_group_t *group;
	vpd_channel_fd_t *vpd_ch_fd;

	group = pif_get_group_by_groupfd(groupfd);
	if ((group  == NULL) || (group->in_use == 0)) {
		GMLIB_ERROR_P("groupfd(%#x) error!\n", groupfd);
		goto exit;
	}

	p_hd_group = (HD_GROUP *)group->hd_group;

	if (p_hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto exit;
	}
	if (line_idx >= HD_MAX_GRAPH_LINE) {
		GMLIB_ERROR_P("line_idx is over(%d %d)\n", line_idx, HD_MAX_GRAPH_LINE);
		goto exit;
	}

	vpd_ch_fd = (vpd_channel_fd_t *)&p_hd_group->glines[line_idx].vpdFdinfo[0];
	vpd_ch_fd->id = 0;
	vpd_ch_fd->line_idx = 0;

	return 0;

exit:
	return -1;
}

int pif_set_vpd_channel_fd(pif_group_t *group, int graphid)
{
	int line_idx;
	HD_GROUP *p_hd_group;
	vpd_channel_fd_t *vpd_ch_fd;

	if (group == NULL) {
		GMLIB_ERROR_P("NULL pif_group\n");
		goto exit;
	}
	p_hd_group = (HD_GROUP *)group->hd_group;
	if (p_hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto exit;
	}

	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		vpd_ch_fd = (vpd_channel_fd_t *)&p_hd_group->glines[line_idx].vpdFdinfo[0];
		if (p_hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE)  {
			vpd_ch_fd->id = 0;
			vpd_ch_fd->line_idx = 0;
		} else {
			vpd_ch_fd->id = VPD_SET_GRAPH_ID(vpd_ch_fd->id, graphid);
			vpd_ch_fd->line_idx = line_idx;
		}
	}
	return 0;

exit:
	return -1;
}

int pif_query_graph_force_graph_exit(vpd_query_graph_t *graph_graph_exit)
{
	int ret = 0;
	vpd_query_graph_t vpd_graph_graph_exit;

	if (graph_graph_exit == NULL) {
		GMLIB_ERROR_P("NULL graph_graph_exit\n");
		return -1;
	}

	GMLIB_L3_P("<START graph.id(%#x)>\n", graph_graph_exit->id);
	vpd_graph_graph_exit.id = graph_graph_exit->id;
	vpd_graph_graph_exit.g_array = graph_graph_exit->g_array;

	/* ioctl */
	if ((ret = ioctl(vpd_fd, VPD_QUERY_GRAPH, &vpd_graph_graph_exit)) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_PRINT_P("ioctl \"VPD_QUERY_GRAPH\" driver into stop state.\n");
			exit(-1);
		} else {
			GMLIB_PRINT_P("ioctl \"VPD_QUERY_GRAPH\" fail");
			return -1;
		}
	}

	GMLIB_L3_P("<END, graph.id(%#x)>\n", graph_graph_exit->id);
	return ret;
}

int pif_apply_entity(HD_GROUP_LINE *group_line, vpd_entity_t *entity, int entity_body_len)
{
	int ret = 0, buf_len;
	vpd_channel_fd_t *vpd_ch_fd;
	pif_group_t *group;

	if (group_line == NULL) {
		GMLIB_ERROR_P("apply_entity: NULL group_line\n");
		return -1;
	}
	if (entity == NULL) {
		GMLIB_ERROR_P("apply_entity: NULL entity\n");
		return -1;
	}

	group = pif_get_group_by_groupfd(group_line->groupidx);
	if (group == NULL) {
		return -1;
	}

	GMLIB_L2_P("< apply_entity: START fd(%#x) ap_bindfd(%u)>\n", entity->sn, entity->ap_bindfd);
	pthread_mutex_lock(&vpd_mutex);
	if (group->in_use != 0) {
		group->is_set_attribute = 1;
	}

	buf_len = sizeof(vpd_update_entity_t) + entity_body_len;
	if ((buf_len + 8) > pif_update_entity_buf_len) {   // (buf_len + 8) get more memory
		GMLIB_PRINT_P("alloc more memory! require_size(%d) new_size(%d)\n",
			      buf_len, pif_update_entity_buf_len + buf_len);
		PIF_FREE(pif_update_entity_buf);
		pif_update_entity_buf_len = pif_update_entity_buf_len + buf_len;
		if ((pif_update_entity_buf = PIF_MALLOC(pif_update_entity_buf_len)) == NULL) {
			GMLIB_ERROR_P("alloc pif_update_entity_buf fail!\n");
			ret = -1;
			goto ext;
		}
	}

	/*
	  pif_update_entity_buf : packed buffer for cpu_comm
	  --------------------------------------------------------------------
	  Format: {sizeof(vpd_update_entity_t) + entityStructSize             }
	          {vpd_update_entity_t         + entityStruct                 }
	          {len + id + channel_fd + entity + entity_offs + entity_bady}
	*/

	vpd_ch_fd = (vpd_channel_fd_t *) &group_line->vpdFdinfo[0];
	pif_update_entity_buf->magic = VPD_ENTITY_MAGIC_NUM;
	pif_update_entity_buf->id = vpd_ch_fd->id;
	pif_update_entity_buf->len = buf_len;
	memcpy(&pif_update_entity_buf->channel_fd, vpd_ch_fd, sizeof(vpd_channel_fd_t));
	memcpy(&pif_update_entity_buf->entity, entity, sizeof(vpd_entity_t));
	pif_update_entity_buf->entity_offs = sizeof(vpd_update_entity_t);

	/* entity bady point: (void *)pif_update_entity_buf + pif_update_entity_buf->entity_offs  */
	memcpy((void *)((void *)pif_update_entity_buf + pif_update_entity_buf->entity_offs),
	       entity->e, entity_body_len);

	if (ioctl(vpd_fd, VPD_UPDATE_ENTITY_SETTING, pif_update_entity_buf) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_ERROR_P("ioctl \"VPD_UPDATE_ENTITY_SETTING\" driver into stop state(%d).\n", errsv);
			goto ext;
		} else {
			ret = -1 * errsv;
			goto ext;
		}
	}
	group->is_set_attribute = 0;
	ret = 0;

ext:
	pthread_mutex_unlock(&vpd_mutex);
	GMLIB_L2_P("< apply_entity: END fd(%#x) ret(%d)>\n", entity->sn, ret);
	return ret;
}

int pif_set_graph(vpd_graph_t *graph)
{
	int ret = 0;

	if (graph == NULL) {
		GMLIB_ERROR_P("set: NULL vpd_graph!\n");
		ret = -1;
		goto ext;
	}

	GMLIB_L2_P("< set_graph: START len(%d)>\n", graph->len);
	if (graph->type == GRAPH_AU_LIVESOUND) {
		goto ext;  // for livesound, don't need to set/apply graph for ioctl, ret = 0.
	}

	/* ioctl */
	//GMLIB_DUMP_MEM(graph, graph->len);
	graph->len = ALIGN8_UP(graph->len); /* 8byte align for cpucomm DMA */
	if (graph->len > graph->buf_size) {
		GMLIB_ERROR_P("ALIGN8_UP_graph->len(%d) > graph->buf_size(%d) error!\n",
			      ALIGN8_UP(graph->len), graph->buf_size);
		return -1;
	}
	if ((ret = ioctl(vpd_fd, VPD_SET_GRAPH, graph)) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_PRINT_P("ioctl \"VPD_SET_GRAPH\" driver into stop state.\n");
			exit(-1);
		} else {
			GMLIB_PRINT_P("ioctl \"VPD_SET_GRAPH\" fail");
			return -1;
		}
	}
	ret = graph->id;  /* reassing graph->id from ioctl */
	GMLIB_L2_P("< set_graph: END, graph.id(%#x)>\n", graph->id);
ext:
	return ret;
}

int pif_check_graphMember(vpd_graph_list_t *graphList)
{
	int i, ret = 0;

	if (graphList == NULL) {
		GMLIB_ERROR_P("NULL graphList\n");
		return -1;
	}

	for (i = 0; i < VPD_GRAPH_LIST_MAX_ARRAY; i++) {
		if (graphList->array[i] != 0) {
			ret = 1;
			break;
		}
	}
	return ret;
}


int pif_apply_graph(list_head_t *pifHead)
{
	vpd_graph_list_t graphList;
	int ret = 0;

	GMLIB_L2_P("< apply graph pid=%d>\n", getpid());

	if (pifHead == NULL) {
		GMLIB_ERROR_P("NULL pifHead\n");
		goto ext;
	}

	pif_transf_list(pifHead, &graphList);
	if (pif_check_graphMember(&graphList) == 0) {
		goto ext;
	}
	ret = ioctl(vpd_fd, VPD_APPLY, &graphList);

	if (ret < 0) {
		usleep(50000);
		GMLIB_PRINT_P("ioctl \"VPD_APPLY\" fail! (ret=%d) \n", ret);
	}
ext:
	return ret;
}

int pif_apply_empty_version(int graphid)
{
	int ret = 0;
	list_head_t pifHead;
	vpd_graph_t *p_empty_vpd_graph = NULL;

	if ((p_empty_vpd_graph = PIF_CALLOC(1, VPD_MAX_BUFFER_LEN)) == NULL) {
		GMLIB_ERROR_P("pif_apply_empty_version: alloc encap_buf fail!\n");
		ret = -1;
		goto ext;
	}
	memset(p_empty_vpd_graph, 0x0, sizeof(vpd_graph_t));
	init_list(&pifHead);
	p_empty_vpd_graph->valid = 1;
	p_empty_vpd_graph->id = graphid;
	p_empty_vpd_graph->len += sizeof(vpd_graph_t);
	p_empty_vpd_graph->buf_size = VPD_MAX_BUFFER_LEN;
	graphid = pif_set_graph(p_empty_vpd_graph);
	if (graphid == -1) {
		ret = -1;
		goto ext;
	}
	if (add_list(graphid, &pifHead) < 0) {
		ret = -1;
		goto ext;
	}

	if (pif_apply_graph(&pifHead) < 0) {
		free_list(&pifHead);
		ret = -1;
		goto ext;
	}
	free_list(&pifHead);

ext:
	if (p_empty_vpd_graph) {
		PIF_FREE(p_empty_vpd_graph);
	}
	return ret;
}

int pif_apply_force_graph_exit(int groupfd)
{
	int idx, ret = 0;
	vpd_query_graph_t query_graph;
	pif_group_t *group;

	memset(&query_graph, 0, sizeof(vpd_query_graph_t));
	group = pif_get_group_by_groupfd(groupfd);
	if ((group  == NULL) || (group->in_use == 0)) {
		GMLIB_ERROR_P("pif_retrieve_graph_and_apply: groupfd(%#x) error!\n", groupfd);
		ret = -1;
		goto ext;
	}

	query_graph.g_array = (int *)PIF_MALLOC(((VPD_MAX_GRAPH_NUM + 1) * sizeof(int)));
	if (query_graph.g_array == NULL) {
		GMLIB_ERROR_P("alloc query_graph.g_array fail!\n");
		ret = -1;
		goto ext;
	}
	memset((void *)query_graph.g_array, 0xFF, ((VPD_MAX_GRAPH_NUM + 1) * sizeof(int)));
	query_graph.id = 0;
	ret = pif_query_graph_force_graph_exit(&query_graph);
	if (ret != 0) {
		GMLIB_ERROR_P("pif_query_graph_force_graph_exit fail, ret=%d!\n", ret);
		goto ext;
	}

	for (idx = 0 ; idx <= VPD_MAX_GRAPH_NUM; idx++) {
		if (query_graph.g_array[idx] < 0) {
			continue;
		}
		GMLIB_L2_P("Error! Abnormal AP exit under graphid=0x%x\n", query_graph.g_array[idx]);
		GMLIB_PRINT_P("Error! Abnormal AP exit under graphid=0x%x\n", query_graph.g_array[idx]);

		if (pif_apply_empty_version(query_graph.g_array[idx]) < 0) {
			GMLIB_ERROR_P("pif_apply_empty_version fail\n");
			goto ext;
		}
	}

ext:
	if (query_graph.g_array) {
		PIF_FREE(query_graph.g_array);
	}
	return ret;
}

int pif_apply(int groupfd)
{
	list_head_t pifHead;
	int groupidx = (int) groupfd;
	pif_group_t *group;
	vpd_graph_t *vpd_graph = NULL;
	int graphid = 0, ret = 0;

	/* error checking */
	GMLIB_L2_P("< apply: START groupfd=%p>\n", groupfd);
	if ((groupidx <= 0) || (groupidx >= GM_MAX_GROUP_NUM)) {
		GMLIB_ERROR_P("apply: groupfd error(%#x)!\n", groupidx);
		return -1;
	}
	if (pif_init_data.alloc_nr > 1500) {    /* check memory alloc */
		GMLIB_L2_P("apply: groupfd_nr(%d) alloc_nr(%d) cif_list_alloc_nr(%d).\n",
			   pif_init_data.gpfd_nr, pif_init_data.alloc_nr, cif_list_alloc_nr);
	}

	/* check cascade remote video */
	group = pif_get_group_by_groupfd(groupfd);
	if ((group == NULL) || (group->in_use == 0)) {
		GMLIB_ERROR_P("apply: groupfd(%#x) error!\n", groupidx);
		ret = ERR_APPLY_FAIL;
		goto ext;
	}
	if (group->error_state != 0) {
		GMLIB_ERROR_P("apply: There are ERRORs so it cannot APPLY, please EXIT application and check flow.\n");
		ret = ERR_APPLY_FAIL;
		return ret;
	}

	pthread_mutex_lock(&vpd_mutex);
	group->is_set_attribute = 0;
	if (group->encap_buf == NULL) {
		GMLIB_ERROR_P("apply: groupfd(%#x) encap_buf is NULL!\n", groupidx);
		ret = ERR_APPLY_FAIL;
		goto ext;
	}

	/* encaps graph */
	init_list(&pifHead);
	if ((ret = pif_encaps_graph_for_param(group)) < 0) {
		ret = ERR_APPLY_FAIL;
		goto ext;
	}

	/* set graph type */
	if ((ret = pif_set_graph_type(group)) < 0) {
		ret = ERR_APPLY_FAIL;
		goto ext;
	}

	/* get updated vpd graph */
	vpd_graph = group->encap_buf;

	/* get graph id */
	graphid = pif_set_graph(vpd_graph);
	if (graphid == -1) {
		ret = ERR_APPLY_FAIL;
		goto ext;
	}

	/* keep graphid, for application fd */
	if ((ret = add_list(graphid, &pifHead)) < 0) {
		ret = ERR_APPLY_FAIL;
		goto ext;
	}

	/* set new graph id to vpd channel fd */
	if ((ret = pif_set_vpd_channel_fd(group, graphid)) < 0) {
		ret = ERR_APPLY_FAIL;
		goto ext;
	}

	/* apply graph */
	if ((ret = pif_apply_graph(&pifHead)) < 0) {
		ret = ERR_APPLY_FAIL;
	}
	free_list(&pifHead);

ext:
	GMLIB_L2_P("< apply: END groupfd=%p, graphid(%d)>\n", groupfd, graphid);
	pthread_mutex_unlock(&vpd_mutex);
	return ret;
}

int pif_new_groupfd(void)
{
	int groupidx = 0;

	if (!pif_init_data.init_nr) {
		GMLIB_PRINT_P("new_groupfd: lib not init, please init first.\n");
		return 0;
	}
	pif_init_data.gpfd_nr++;

	for (groupidx = GM_GROUP_FD_START; groupidx < GM_MAX_GROUP_NUM; groupidx++) {
		if (pif_group[groupidx] == NULL) {
			if ((pif_group[groupidx] = PIF_CALLOC(1, sizeof(pif_group_t))) == NULL) {
				GMLIB_ERROR_P("alloc groupfd entry(%d) fail!\n", groupidx);
				return 0;
			}
		}
		if (pif_group[groupidx] && pif_group[groupidx]->in_use == 0) {
			pif_group[groupidx]->in_use = 1;
			pif_group[groupidx]->groupidx = groupidx;
			max_group_idx = MAX_VAL(max_group_idx, groupidx);
			GMLIB_L2_P("groupfd=%d, max_group_idx=%d\n", groupidx, max_group_idx);
			return groupidx;
		}
	}
	GMLIB_ERROR_P("new_groupfd: Over the max number of group(%x)!\n", groupidx);
	return 0;
}

int pif_delete_groupfd(int groupfd)
{
	int groupidx = groupfd;
	int ret = 0;
	pif_group_t *group;
	vpd_graph_t *vpd_graph;

	GMLIB_L2_P("groupfd=%#x,\n", groupfd);
	pif_init_data.gpfd_nr--;

	if ((groupidx <= 0) || (groupidx >= GM_MAX_GROUP_NUM)) {
		GMLIB_ERROR_P("gm_delete_groupfd groupfd error(%#x)!\n", groupidx);
		return -1;
	}

	if (((group = pif_group[groupidx]) == NULL) || (group->in_use == 0)) {
		goto ext;
	}

	group->in_use = 0;
	if (group->encap_buf) {
		vpd_graph = group->encap_buf;
		vpd_graph->id = 0;
	}

ext:
	return ret;
}

void pif_print_invalid_for_polled_bs(gm_pollfd_t *poll_fds, int num_fds)
{
	int i;

	GMLIB_PRINT_P("pif_poll: No vaild linefd! num_fds(%d), or apply is ongoing.\n", num_fds);

	if (poll_fds == NULL) {
		GMLIB_ERROR_P("NULL poll_fds\n");
		return;
	}

	/* case: error */
	for (i = 0; i < num_fds; i++) {
		if (poll_fds[i].group_line == NULL) {
			GMLIB_PRINT_P("  Not existed linefd list (maybe already removed):\n");
			break;
		}
	}

	/* case: unbind invalid */
	for (i = 0; i < num_fds; i++) {
		if ((poll_fds[i].group_line != NULL) && (poll_fds[i].group_line->state == HD_LINE_INCOMPLETE)) {
			GMLIB_PRINT_P("  line link already unbind:\n");
			break;
		}
	}
}

int pif_poll(gm_pollfd_t *poll_fds, int num_fds, int timeout_msec)
{
	int i, ret = -1;
	vpd_poll_t pifPoll;
	vpd_channel_fd_t *vpd_ch_fd, *pif_ch_fd;
	int all_not_found = 1;
	int polled_nr;

	if (poll_fds == NULL) {
		GMLIB_ERROR_P("pif_poll: NULL poll_fds\n");
		return -1;
	}

	pifPoll.nr_poll = num_fds;
	pifPoll.timeout_ms = timeout_msec;
	for (i = 0; i < num_fds; i++) {
		if ((poll_fds[i].group_line == NULL) ||
		    (poll_fds[i].group_line->state == HD_LINE_INCOMPLETE))  {
			poll_fds[i].revent.event = 0;
			pif_ch_fd = (vpd_channel_fd_t *) &poll_fds[i].fd_private[0];
			pif_ch_fd->line_idx = 0;
			pif_ch_fd->id = 0;
		} else {

			poll_fds[i].revent.event = 0;
			vpd_ch_fd = (vpd_channel_fd_t *) &poll_fds[i].group_line->vpdFdinfo[0];
			pif_ch_fd = (vpd_channel_fd_t *) &poll_fds[i].fd_private[0];
			pif_ch_fd->line_idx = vpd_ch_fd->line_idx;
			pif_ch_fd->id = vpd_ch_fd->id;
			if (pif_check_graphid(vpd_ch_fd->id) < 0) {
				GMLIB_ERROR_P("num_fds(idx%d): linefd(%#x) error! chip(%d) cpu(%d) graphid(%d)\n",
					      i,
					      poll_fds[i].group_line->linefd,
					      VPD_GET_CHIP_ID(vpd_ch_fd->id),
					      VPD_GET_CPU_ID(vpd_ch_fd->id),
					      VPD_GET_GRAPH_ID(vpd_ch_fd->id));
				return -1;
			}
			all_not_found = 0;
		}
	}



	if (all_not_found) {
		ret = -1;
		pif_print_invalid_for_polled_bs(poll_fds, num_fds);
	} else {
		pifPoll.poll_objs = (vpd_poll_obj_t *)poll_fds;
		if ((ret = ioctl(vpd_fd, VPD_POLL, &pifPoll)) < 0) {
			int errsv = errno;
			if (errsv == abs(VPD_STOP_STATE)) {
				GMLIB_PRINT_P("ioctl \"VPD_POLL\" driver into stop state.\n");
				exit(-1);
			} else {
				ret = errsv * -1;
			}
		}
	}
	polled_nr = 0;
	GMLIB_L3_P("== [Poll_Start: nr(%d)] ==\n", num_fds);
	for (i = 0; i < num_fds; i++) {
		if ((poll_fds[i].group_line != NULL) &&
		    (poll_fds[i].group_line->state != HD_LINE_INCOMPLETE)) {
			if (poll_fds[i].revent.event == GM_POLL_READ)  {
				polled_nr++;
				GMLIB_L3_P("polled_bs(%d): linefd(%#x) len(%d)\n", i, poll_fds[i].group_line->linefd,
					   poll_fds[i].revent.bs_len);
			} else {
				GMLIB_L3_P("polled_bs(%d): linefd(%#x) ***\n", i, poll_fds[i].group_line->linefd);
			}
		}
	}
	GMLIB_L3_P("== [Poll_End: polled_nr(%d)] ==\n", polled_nr);

	return ret;
}

void pif_update_statistic(UINT linefd, BD_STAT_INFO *stat_info, int accum_count)
{
	unsigned int now, diff;
	struct timeval time;

	if (!pif_static_time) {
		return;
	}
	if (stat_info == NULL) {
		GMLIB_ERROR_P("NULL stat_info\n");
		return;
	}

	gettimeofday(&time, NULL);
	now = 1000000 * time.tv_sec + time.tv_usec;
	if (!stat_info->count) {
		stat_info->first_time = now;
	}
	stat_info->count += accum_count;

	diff = now - stat_info->first_time;

	if (diff >= pif_static_time) {
		GMLIB_PRINT_P(" linefd(%#x): %d frames in %u ms\n", linefd, stat_info->count, diff / 1000);
		stat_info->count = 0;
		stat_info->first_time = 0;
	}
}

void pif_prepare_dec_bs(gm_dec_multi_bitstream_t *multi_bs)
{
	vpd_put_copy_din_t *din;
	vpd_channel_fd_t *vpd_fd;

	if (multi_bs == NULL) {
		GMLIB_ERROR_P("NULL multi_bs\n");
		return;
	}
	if ((multi_bs->group_line == NULL) ||
	    (multi_bs->group_line->state == HD_LINE_INCOMPLETE)) {
		return;
	}

	vpd_fd = (vpd_channel_fd_t *) &multi_bs->group_line->vpdFdinfo[0];
	din = (vpd_put_copy_din_t *) &multi_bs->dec_private[0];

	din->channel_fd.id = vpd_fd->id;
	din->channel_fd.line_idx = vpd_fd->line_idx;
	din->channel_fd.entity_fd = 0;
	din->bs_buf = multi_bs->bs_buf;
	din->bs_len = multi_bs->bs_buf_len;
	din->ap_timestamp = multi_bs->ap_timestamp;
	din->user_flag = multi_bs->user_flag;
	din->reserved[1] = multi_bs->reserved[1]; // for timestamp user flag
	if (multi_bs->time_align == TIME_ALIGN_DISABLE) {
		din->time_align_by_user = 1; /* TIME_ALIGN_DISABLE */
	} else if (multi_bs->time_align == TIME_ALIGN_USER) {
		din->time_align_by_user = multi_bs->time_diff; /* TIME_ALIGN_USER */
	} else {
		din->time_align_by_user = 0; /* TIME_ALIGN_ENABLE */
	}
	return;
}

void pif_print_invalid_for_send_bs(gm_dec_multi_bitstream_t *multi_bs, int num_bs)
{
	int i;

	if (multi_bs == NULL) {
		GMLIB_ERROR_P("send_multi_bs: NULL multi_bs\n");
		return;
	}

	/* case: error */
	for (i = 0; i < num_bs; i++) {
		if (multi_bs[i].group_line == NULL) {
			GMLIB_PRINT_P("send_multi_bs: Not existed linefd list (maybe already removed)"
				      ", num_bs(%d) idx(%d)\n", num_bs, i);
			break;
		}
	}

	/* case: unbind invalid */
	for (i = 0; i < num_bs; i++) {
		if ((multi_bs[i].group_line != NULL) &&
		    (multi_bs[i].group_line->state == HD_LINE_INCOMPLETE)) {
			GMLIB_PRINT_P("send_multi_bs: linefd list with already unbind, num_bs(%d) idx(%d)\n", num_bs, i);
			break;
		}
	}

	/* case: bs_buf_len = 0  */
	for (i = 0; i < num_bs; i++) {
		if ((multi_bs[i].group_line != NULL) &&
		    (multi_bs[i].bs_buf_len == 0)) {
			GMLIB_PRINT_P("send_multi_bs: linefd list with bs_buf_len is 0, num_bs(%d), idx(%d)\n", num_bs, i);
			break;
		}
	}
}

int pif_send_multi_bitstreams(gm_dec_multi_bitstream_t *multi_bs, int num_bs, int timeout_msec)
{
	int ret = -1, i;
	vpd_put_copy_multi_bs_t multi;
	vpd_put_copy_din_t *din;
	int valid_nr = 0, groupidx = 0;
	pif_group_t *group;

	if (multi_bs == NULL) {
		GMLIB_ERROR_P("send_multi: NULL multi_bs\n");
		return -1;
	}

	/* 1. prepare bs parameter and check duplicate */
	for (i = 0; i < num_bs; i++) {
		multi_bs[i].retval = -1;
		/* case: not ready */
		if ((multi_bs[i].group_line == NULL) ||
		    (multi_bs[i].group_line->state == HD_LINE_INCOMPLETE) ||
		    (multi_bs[i].bs_buf_len == 0)) {

			din = (vpd_put_copy_din_t *) &multi_bs[i].dec_private[0];
			din->channel_fd.id = 0;
			din->bs_len = 0;
			continue;
		}
		if (groupidx == 0) {
			groupidx = multi_bs[i].group_line->groupidx;
		} else if (groupidx != multi_bs[i].group_line->groupidx) {
			GMLIB_ERROR_P("Cannot send_bitstream with two different groupfd(%d, %d)!\n",
				      groupidx, multi_bs[i].group_line->groupidx);
			ret = -1;
			goto ext;
		}
		if (multi_bs[i].bs_buf_len > PIF_MAX_BUFFER_SIZE) {
			GMLIB_ERROR_P("linefd(%#x) bs_buf_len(%d) over %dKB! \n", (int) multi_bs[i].group_line->linefd,
				      multi_bs[i].bs_buf_len, (PIF_MAX_BUFFER_SIZE / 1024));
			ret = -1;
			goto ext;
		}

		if (multi_bs[i].time_align == TIME_ALIGN_USER &&
		    ((unsigned int) multi_bs[i].time_diff > 10000000)) {

			GMLIB_ERROR_P("linefd(%#x) time_align(TIME_ALIGN_USER), time_diff(%d) overflow. \n",
				      (int) multi_bs[i].group_line->linefd, multi_bs[i].time_diff);
			ret = -1;
			goto ext;
		}
		valid_nr++;
		pif_prepare_dec_bs(&multi_bs[i]);
	}
	if (valid_nr == 0) {
		pif_print_invalid_for_send_bs(multi_bs, num_bs);
		ret = -1;
		goto ext;
	}
	group = pif_get_group_by_groupfd(groupidx);
	if (group == NULL) {
		ret = -1;
		goto ext;
	}

	multi.nr_bs = num_bs;
	multi.timeout_ms = timeout_msec;
	multi.multi_bs = (vpd_multi_din_t *) multi_bs;
	if ((ret = ioctl(vpd_fd, VPD_PUT_COPY_MULTI_DIN, &multi)) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_ERROR_P("ioctl \"VPD_PUT_COPY_MULTI_DIN\" driver into stop state.(%d)\n", errsv);
			ret = -1;
			goto ext;
		} else {
			GMLIB_ERROR_P("ioctl \"VPD_PUT_COPY_MULTI_DIN\" driver failed. errno(%d)\n", errsv);
			ret = -1 * errsv;
			goto ext;
		}
	}

	for (i = 0; i < num_bs; i++) {
		if ((multi_bs[i].group_line != NULL) &&
		    (multi_bs[i].group_line->state != HD_LINE_INCOMPLETE) &&
		    (multi_bs[i].bs_buf_len != 0)) {

			if (ret >= 0) {
				GMLIB_L3_P("send_bs(%d): linefd(%#x) len(%d) buf: %08x %08x %08x %08x\n", i,
					   multi_bs[i].group_line->linefd, multi_bs[i].bs_buf_len,
					   *(int *) &multi_bs[i].bs_buf[0],
					   *(int *) &multi_bs[i].bs_buf[4],
					   *(int *) &multi_bs[i].bs_buf[8],
					   *(int *) &multi_bs[i].bs_buf[12]);
			} else {
				GMLIB_L3_P("send_bs(%d): linefd(%#x) len(%d) %s ***\n", i, multi_bs[i].group_line->linefd,
					   multi_bs[i].bs_buf_len, pif_covert_ret_str(ret));
			}
			pif_update_statistic(multi_bs[i].group_line->linefd, &multi_bs[i].group_line->stat_info, 1);

			// update fail_job_count
			multi_bs[i].reserved[0] = multi.multi_bs[i].din.dinBuf.bs.fail_job_count;
		}
	}

ext:
	return ret;
}

int pif_prepare_enc_bs(gm_enc_multi_bitstream_t *multi_bs)
{
	gm_enc_bitstream_t *bs;
	vpd_get_copy_dout_t *dout;
	vpd_channel_fd_t *vpd_fd;

	if (multi_bs == NULL) {
		GMLIB_ERROR_P("NULL multi_bs\n");
		return -1;
	}

	bs = &multi_bs->bs;
	dout = (vpd_get_copy_dout_t *) &multi_bs->enc_private[0];

	bs->newbs_flag = 0;
	multi_bs->retval = -1;
	/* error checking */
	if (bs->bs_buf == NULL && bs->bs_buf_len != 0) {
		GMLIB_ERROR_P("prepare_enc_bs: bs_buf is NULL, bs_buf_len(%d)\n", bs->bs_buf_len);
		return -1;
	}
	if (bs->extra_buf == NULL && bs->extra_buf_len != 0) {
		GMLIB_ERROR_P("prepare_enc_bs: extra_buf is NULL, but extra_buf_len(%d)\n", bs->extra_buf_len);
		return -1;
	}
	/* case: bind is not ready */
	if ((multi_bs->group_line == NULL) ||
	    (multi_bs->group_line->state == HD_LINE_INCOMPLETE)) {
		dout->channel_fd.id = 0;
		return 0;
	}
	vpd_fd = (vpd_channel_fd_t *) &multi_bs->group_line->vpdFdinfo[0];

	dout->channel_fd.id = vpd_fd->id;
	dout->channel_fd.line_idx = vpd_fd->line_idx;
	dout->bs_buf = bs->bs_buf;
	dout->bs_buf_len = bs->bs_buf_len;
	dout->extra_buf = bs->extra_buf;
	dout->extra_buf_len = bs->extra_buf_len;
	return 0;
}

void pif_collect_enc_bs_info(gm_enc_multi_bitstream_t *multi_bs)
{
	gm_enc_bitstream_t *bs;
	vpd_get_copy_dout_t *dout;
	int i = 0;

	if (multi_bs == NULL) {
		GMLIB_ERROR_P("NULL multi_bs\n");
		return;
	}

	bs = &multi_bs->bs;
	dout = (vpd_get_copy_dout_t *) &multi_bs->enc_private[0];

	/* checking user fd status */
	if ((multi_bs->group_line == NULL) ||
	    (multi_bs->group_line->state == HD_LINE_INCOMPLETE) ||
	    (bs->bs_buf == NULL && bs->extra_buf == NULL)) {
		return;
	}

	/* checking return bs status per channel */
	multi_bs->retval = dout->priv[0];   /* priv[0]: bs status */
	if (multi_bs->retval < 0) {
		bs->bs_len = dout->dout_buf.bs.bs_len;
		bs->extra_len = dout->dout_buf.bs.extra_len;
		return;
	}

	bs->bs_len = dout->dout_buf.bs.bs_len;
	bs->extra_len = dout->dout_buf.bs.extra_len;
	bs->frame_type = dout->dout_buf.bs.frame_type;
	bs->timestamp = dout->dout_buf.bs.timestamp;
	bs->checksum = dout->dout_buf.bs.checksum;
	bs->ref_frame = (int)dout->dout_buf.bs.ref_frame;
	for (i = 0; i < 4; i++) {
		bs->slice_offset[i] = dout->dout_buf.bs.slice_offset[i];
	}
	bs->sps_offset = (int)dout->dout_buf.bs.sps_offset;
	bs->pps_offset = (int)dout->dout_buf.bs.pps_offset;
	bs->vps_offset = (int)dout->dout_buf.bs.vps_offset;
	bs->bs_offset = (int)dout->dout_buf.bs.bs_offset;
	bs->svc_layer_type = (int)dout->dout_buf.bs.svc_layer_type;

	bs->bistream_type = ((int)dout->dout_buf.bs.bistream_type == VPD_ENC_TYPE_H264) ?
			    GM_ENCODE_TYPE_H264 : GM_ENCODE_TYPE_H265;
	bs->intra_16_mb_num = (int)dout->dout_buf.bs.intra_16_mb_num;
	bs->intra_8_mb_num = (int)dout->dout_buf.bs.intra_8_mb_num;
	bs->intra_4_mb_num = (int)dout->dout_buf.bs.intra_4_mb_num;
	bs->inter_mb_num = (int)dout->dout_buf.bs.inter_mb_num;
	bs->skip_mb_num = (int)dout->dout_buf.bs.skip_mb_num;

	bs->intra_32_cu_num = (int)dout->dout_buf.bs.intra_32_cu_num;
	bs->intra_16_cu_num = (int)dout->dout_buf.bs.intra_16_cu_num;
	bs->intra_8_cu_num = (int)dout->dout_buf.bs.intra_8_cu_num;
	bs->inter_64_cu_num = (int)dout->dout_buf.bs.inter_64_cu_num;
	bs->inter_32_cu_num = (int)dout->dout_buf.bs.inter_32_cu_num;
	bs->inter_16_cu_num = (int)dout->dout_buf.bs.inter_16_cu_num;
	bs->skip_cu_num = (int)dout->dout_buf.bs.skip_cu_num;
	bs->merge_cu_num = (int)dout->dout_buf.bs.merge_cu_num;

	bs->psnr_y_mse = (int)dout->dout_buf.bs.psnr_y_mse;
	bs->psnr_u_mse = (int)dout->dout_buf.bs.psnr_u_mse;
	bs->psnr_v_mse = (int)dout->dout_buf.bs.psnr_v_mse;

	bs->qp_value = (int)dout->dout_buf.bs.qp_value;
	bs->evbr_state = (int)dout->dout_buf.bs.evbr_state;
	bs->motion_ratio = dout->dout_buf.bs.motion_ratio;

	if (dout->dout_buf.bs.new_bs) {
		if (dout->dout_buf.bs.new_bs & VPD_FLAG_NEW_BITRATE) {
			bs->newbs_flag |= GM_FLAG_NEW_BITRATE;
		}
		if (dout->dout_buf.bs.new_bs & VPD_FLAG_NEW_FRAME_RATE) {
			bs->newbs_flag |= GM_FLAG_NEW_FRAME_RATE;
		}
		if (dout->dout_buf.bs.new_bs & VPD_FLAG_NEW_GOP) {
			bs->newbs_flag |= GM_FLAG_NEW_GOP;
		}
		if (dout->dout_buf.bs.new_bs & VPD_FLAG_NEW_DIM) {
			bs->newbs_flag |= GM_FLAG_NEW_DIM;
		}
	}
	pif_update_statistic(multi_bs->group_line->linefd, &multi_bs->group_line->stat_info, 1);

	return;
}

int pif_collect_enc_bs_ret_info(gm_enc_multi_bitstream_t *multi_bs, int num_bs)
{
	int ret = 0, i, init_flag = 0;

	if (multi_bs == NULL) {
		GMLIB_ERROR_P("NULL multi_bs\n");
		return -1;
	}

	for (i = 0; i < num_bs; i++) {
		if ((multi_bs[i].group_line == NULL) ||
		    (multi_bs[i].group_line->state == HD_LINE_INCOMPLETE) ||
		    (multi_bs[i].bs.bs_buf == NULL && multi_bs[i].bs.extra_buf == NULL)) {
			continue;
		}

		if (init_flag == 0) {
			ret = multi_bs[i].retval;
			init_flag = 1;
		} else {
			if (ret != multi_bs[i].retval) {
				ret = 0;
				goto ext;
			}
		}
	}
ext:
	return ret;
}

int pif_recv_multi_bitstreams(gm_enc_multi_bitstream_t *multi_bs, int num_bs)
{
	int ret = -1, i, recved_nr;
	vpd_get_copy_multi_bs_t multi;

	if (multi_bs == NULL) {
		GMLIB_ERROR_P("recv_multi: NULL multi_bs\n");
		return -1;
	}

	/* 1. prepare bs parameter */
	for (i = 0; i < num_bs; i++) {
		if (pif_prepare_enc_bs(&multi_bs[i]) < 0) {
			return -1;
		}
	}

	/* 2. getcp bs ioctl */
	multi.nr_bs = num_bs;
	multi.multi_bs = (vpd_multi_dout_t *) multi_bs;
	if (ioctl(vpd_fd, VPD_GET_COPY_MULTI_DOUT, &multi) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_ERROR_P("ioctl \"VPD_GET_COPY_MULTI_DOUT\" driver into stop state(%d).\n", errsv);
			goto ext;
		} else {
			GMLIB_ERROR_P("ioctl \"VPD_GET_COPY_MULTI_DOUT\" driver failed. errno(%d).\n", errsv);
			ret = -1 * errsv;
			goto ext;
		}
	}
	/* 3. collect bs info */
	GMLIB_L3_P("== [Recv_Start: nr(%d)] ==\n", num_bs);
	recved_nr = 0;
	for (i = 0; i < num_bs; i++) {
		pif_collect_enc_bs_info(&multi_bs[i]);
		if ((multi_bs[i].group_line != NULL) &&
		    (multi_bs[i].group_line->state != HD_LINE_INCOMPLETE) &&
		    (multi_bs[i].bs.bs_buf != NULL)) {
			if (multi_bs[i].retval >= 0) {
				GMLIB_L3_P("recv_bs(%d): linefd(%#x) len(%d) buf: %08x %08x %08x %08x\n", i,
					   multi_bs[i].group_line->linefd, multi_bs[i].bs.bs_len,
					   *(int *) &multi_bs[i].bs.bs_buf[0],
					   *(int *) &multi_bs[i].bs.bs_buf[4],
					   *(int *) &multi_bs[i].bs.bs_buf[8],
					   *(int *) &multi_bs[i].bs.bs_buf[12]);
				recved_nr++;
			} else {
				GMLIB_L3_P("recv_bs(%d): linefd(%#x) %s ***\n", i, multi_bs[i].group_line->linefd,
					   pif_covert_ret_str(multi_bs[i].retval));
			}
		}
	}
	GMLIB_L3_P("== [Recv_End: recved_nr(%d)] ==\n", recved_nr);
	ret = pif_collect_enc_bs_ret_info(multi_bs, num_bs);
	if (ret < 0)
		GMLIB_ERROR_P("Collect bs failed, ret(%d), or apply is ongoing.\n", ret);

ext:
	return ret;
}


int pif_request_keyframe(HD_GROUP_LINE *group_line)
{
	int ret = -1;
	vpd_channel_fd_t *vpd_ch_fd;
	pif_group_t *group;

	if (group_line == NULL) {
		GMLIB_ERROR_P("group_line(%p) not exist!\n", group_line);
		return -1;
	}
	if (group_line->state == HD_LINE_INCOMPLETE) {
		GMLIB_ERROR_P("HD_LINE_INCOMPLETE\n");
		return -1;
	}
	group = pif_get_group_by_groupfd(group_line->groupidx);
	if (group == NULL) {
		return -1;
	}

	vpd_ch_fd = (vpd_channel_fd_t *) &group_line->vpdFdinfo[0];
	ret = ioctl(vpd_fd, VPD_REQUEST_KEYFRAME, vpd_ch_fd);
	if (ret < 0) {
		GMLIB_ERROR_P("Error VPD_REQUEST_KEYFRAME return value %d\n", ret);
		ret = -1;
	}
	return ret;
}


int pif_user_get_buffer(pif_get_buf_t get_buf_info,
			gm_buffer_t *frame_buffer,
			gm_video_frame_info_t *frame_info,
			gm_bs_info_t *bs_info)
{
	int ret;
	vpd_vg_buffer_t vpd_vg_buffer;

	if (frame_buffer == NULL) {
		GMLIB_ERROR_P("user_get_buffer: NULL frame_buffer\n");
		return -1;
	}
	if (frame_info == NULL) {
		GMLIB_ERROR_P("user_get_buffer: NULL frame_info\n");
		return -1;
	}

	memset(&vpd_vg_buffer, 0, sizeof(vpd_vg_buffer));
	vpd_vg_buffer.bind_fd = get_buf_info.linefd;
	vpd_vg_buffer.entity_type = get_buf_info.entity_type;
	vpd_vg_buffer.entity_fd = get_buf_info.entity_fd;
	vpd_vg_buffer.post_handling = get_buf_info.post_handling;
	vpd_vg_buffer.timeout_ms = get_buf_info.wait_ms;
	if (vpd_vg_buffer.entity_fd == 0) {
		GMLIB_ERROR_P("user_get_buffer: entity_fd = 0\n");
		ret = -1;
		goto exit;
	}

	if ((ret = ioctl(vpd_fd, VPD_GET_VG_BUFFER, &vpd_vg_buffer)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		GMLIB_ERROR_P("<ioctl \"VPD_GET_VG_BUFFER\" fail, error(%d)>\n", ret);
	} else {
		frame_buffer->ddr_id = vpd_vg_buffer.buffer_info.ddr_id;
		frame_buffer->pa = (void *)vpd_vg_buffer.buffer_info.addr_pa;
		frame_buffer->size = vpd_vg_buffer.buffer_info.size;
		frame_info->format = vpd_vg_buffer.frame_info.type;
		frame_info->dim.width = vpd_vg_buffer.frame_info.dim.w;
		frame_info->dim.height = vpd_vg_buffer.frame_info.dim.h;
		frame_info->bg_dim.width = vpd_vg_buffer.frame_info.bg_dim.w;
		frame_info->bg_dim.height = vpd_vg_buffer.frame_info.bg_dim.h;
		frame_info->timestamp = vpd_vg_buffer.frame_info.timestamp;
		frame_info->ts_user_flag = vpd_vg_buffer.frame_info.ts_user_flag;

		if (bs_info) {
			bs_info->qp = vpd_vg_buffer.enc_info.qp;
			bs_info->slice_type = vpd_vg_buffer.enc_info.slice_type;
			bs_info->evbr_state = vpd_vg_buffer.enc_info.evbr_state;
			bs_info->frame_type = vpd_vg_buffer.enc_info.frame_type;
			bs_info->sps_offset = vpd_vg_buffer.enc_info.sps_offset;
			bs_info->pps_offset = vpd_vg_buffer.enc_info.pps_offset;
			bs_info->vps_offset = vpd_vg_buffer.enc_info.vps_offset;
			bs_info->bs_offset = vpd_vg_buffer.enc_info.bs_offset;
			bs_info->svc_layer_type = vpd_vg_buffer.enc_info.svc_layer_type;
			bs_info->intra_16_mb_num = vpd_vg_buffer.enc_info.intra_16_mb_num;
			bs_info->intra_8_mb_num = vpd_vg_buffer.enc_info.intra_8_mb_num;
			bs_info->intra_4_mb_num = vpd_vg_buffer.enc_info.intra_4_mb_num;
			bs_info->inter_mb_num = vpd_vg_buffer.enc_info.inter_mb_num;
			bs_info->skip_mb_num = vpd_vg_buffer.enc_info.skip_mb_num;
			bs_info->intra_32_cu_num = vpd_vg_buffer.enc_info.intra_32_cu_num;
			bs_info->intra_16_cu_num = vpd_vg_buffer.enc_info.intra_16_cu_num;
			bs_info->intra_8_cu_num = vpd_vg_buffer.enc_info.intra_8_cu_num;
			bs_info->inter_64_cu_num = vpd_vg_buffer.enc_info.inter_64_cu_num;
			bs_info->inter_32_cu_num = vpd_vg_buffer.enc_info.inter_32_cu_num;
			bs_info->inter_16_cu_num = vpd_vg_buffer.enc_info.inter_16_cu_num;
			bs_info->skip_cu_num = vpd_vg_buffer.enc_info.skip_cu_num;
			bs_info->merge_cu_num = vpd_vg_buffer.enc_info.merge_cu_num;
			bs_info->psnr_y_mse = vpd_vg_buffer.enc_info.psnr_y_mse;
			bs_info->psnr_u_mse = vpd_vg_buffer.enc_info.psnr_u_mse;
			bs_info->psnr_v_mse = vpd_vg_buffer.enc_info.psnr_v_mse;
			bs_info->motion_ratio = vpd_vg_buffer.enc_info.motion_ratio;
		}
	}

exit:
	return ret;
}

int pif_user_release_buffer(unsigned int linefd,
			    vpd_entity_type_t entity_type,
			    gm_buffer_t *frame_buffer)
{
	int ret;
	vpd_vg_buffer_t vpd_vg_buffer;

	if (frame_buffer == NULL) {
		GMLIB_ERROR_P("user_release_buffer: NULL frame_buffer\n");
		return -1;
	}

	memset(&vpd_vg_buffer, 0, sizeof(vpd_vg_buffer));
	vpd_vg_buffer.bind_fd = linefd;
	vpd_vg_buffer.entity_type = entity_type;
	vpd_vg_buffer.buffer_info.addr_pa = (uintptr_t)frame_buffer->pa;
	vpd_vg_buffer.buffer_info.ddr_id = frame_buffer->ddr_id;
	if ((ret = ioctl(vpd_fd, VPD_RELEASE_VG_BUFFER, &vpd_vg_buffer)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		GMLIB_ERROR_P("<ioctl \"VPD_RELEASE_VG_BUFFER\" fail, error(%d)>\n", ret);
	}

	return ret;
}

int pif_pull_out_buffer(pif_get_buf_t get_buf_info,
			gm_buffer_t *frame_buffer,
			gm_bs_info_t *bs_info,
			vpd_get_copy_dout_t *dout,
			INT wait_ms)
{
	int ret;
	vpd_pull_t vpd_pull = {0};
	vpd_channel_fd_t *vpd_ch_fd = NULL;

	if (frame_buffer == NULL) {
		GMLIB_ERROR_P("pif_pull_out_buffer: NULL frame_buffer\n");
		return -1;
	}
	if (bs_info == NULL) {
		GMLIB_ERROR_P("pif_pull_out_buffer: NULL bs_info\n");
		return -1;
	}
	if (dout == NULL) {
		GMLIB_ERROR_P("pif_pull_out_buffer: NULL dout\n");
		return -1;
	}

	vpd_ch_fd = (vpd_channel_fd_t *)&get_buf_info.vpdFdinfo[0];
	vpd_pull.channel_fd.id = vpd_ch_fd->id;
	vpd_pull.channel_fd.line_idx = vpd_ch_fd->line_idx;
	vpd_pull.timeout_ms = wait_ms;

	if ((ret = ioctl(vpd_fd, VPD_PULL_OUT_BUFFER, &vpd_pull)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		GMLIB_ERROR_P("<ioctl \"VPD_PULL_OUT_BUFFER\" fail, error(%d)>\n", ret);
	} else {
		frame_buffer->ddr_id = vpd_pull.ddr_id;
		frame_buffer->pa = (void *)vpd_pull.buf_pa;
		frame_buffer->size = vpd_pull.buf_size;

		bs_info->qp = vpd_pull.bs.qp_value;
		bs_info->slice_type = (vpd_pull.bs.bistream_type == VPD_ENC_TYPE_H264) ? H264_NALU_TYPE_SPS : H265_NALU_TYPE_SPS;
		bs_info->evbr_state = vpd_pull.bs.evbr_state;
		bs_info->frame_type = vpd_pull.bs.frame_type;
		bs_info->sps_offset = vpd_pull.bs.sps_offset;
		bs_info->pps_offset = vpd_pull.bs.pps_offset;
		bs_info->vps_offset = vpd_pull.bs.vps_offset;
		bs_info->bs_offset = vpd_pull.bs.bs_offset;
		bs_info->svc_layer_type = vpd_pull.bs.svc_layer_type;
		bs_info->intra_16_mb_num = vpd_pull.bs.intra_16_mb_num;
		bs_info->intra_8_mb_num = vpd_pull.bs.intra_8_mb_num;
		bs_info->intra_4_mb_num = vpd_pull.bs.intra_4_mb_num;
		bs_info->inter_mb_num = vpd_pull.bs.inter_mb_num;
		bs_info->skip_mb_num = vpd_pull.bs.skip_mb_num;
		bs_info->intra_32_cu_num = vpd_pull.bs.intra_32_cu_num;
		bs_info->intra_16_cu_num = vpd_pull.bs.intra_16_cu_num;
		bs_info->intra_8_cu_num = vpd_pull.bs.intra_8_cu_num;
		bs_info->inter_64_cu_num = vpd_pull.bs.inter_64_cu_num;
		bs_info->inter_32_cu_num = vpd_pull.bs.inter_32_cu_num;
		bs_info->inter_16_cu_num = vpd_pull.bs.inter_16_cu_num;
		bs_info->skip_cu_num = vpd_pull.bs.skip_cu_num;
		bs_info->merge_cu_num = vpd_pull.bs.merge_cu_num;
		bs_info->psnr_y_mse = vpd_pull.bs.psnr_y_mse;
		bs_info->psnr_u_mse = vpd_pull.bs.psnr_u_mse;
		bs_info->psnr_v_mse = vpd_pull.bs.psnr_v_mse;
		bs_info->motion_ratio = vpd_pull.bs.motion_ratio;
		bs_info->timestamp = vpd_pull.bs.timestamp;

		memcpy(dout, &vpd_pull.dout, sizeof(vpd_get_copy_dout_t));

	}

	return ret;
}

int pif_release_out_buffer(unsigned int linefd,
			   vpd_entity_type_t entity_type,
			   vpd_get_copy_dout_t *dout)
{
	int ret;

	if (dout == NULL) {
		GMLIB_ERROR_P("pif_release_out_buffer: NULL dout\n");
		return -1;
	}

	if ((ret = ioctl(vpd_fd, VPD_RELEASE_OUT_BUFFER, dout)) < 0) {
		GMLIB_ERROR_P("<ioctl \"VPD_RELEASE_OUT_BUFFER\" fail, error(%d)>\n", ret);
	}

	return ret;
}

void pif_send_log(const char *fmt, ...)
{
	vpd_send_log_t log;
	va_list ap;
	char line[256];
	int ret;

	va_start(ap, fmt);
	log.length = vsnprintf(line, sizeof(line), fmt, ap);
	va_end(ap);

	if (gmlib_dbg_mode == 0) {
		log.str = (unsigned char *)line;
		if ((ret = ioctl(vpd_fd, VPD_SEND_LOGMSG, &log)) < 0) {
			int errsv = errno;
			if (errsv == abs(VPD_STOP_STATE)) {
				//GMLIB_PRINT_P("ioctl \"VPD_SEND_LOGMSG\" driver into stop state.(%d)\n", errsv);
				//exit(-1);
			} else {
				GMLIB_PRINT_P("<ioctl \"VPD_SEND_LOGMSG\" fail(%d)>\n", errsv);
			}
		}
	} else if (gmlib_dbg_mode == 1) {  //1: direct print
		GMLIB_PRINT_P("%s", line);
	}
	return;
}

int pif_send_log_p(unsigned char *str, unsigned int len)
{
	vpd_send_log_t log;
	int ret = 0;

	if (gmlib_dbg_mode == 0) {
		log.str = str;
		log.length = len;
		if ((ret = ioctl(vpd_fd, VPD_SEND_LOGMSG, &log)) < 0) {
			int errsv = errno;
			if (errsv != abs(VPD_STOP_STATE)) {
				GMLIB_PRINT_P("<ioctl \"VPD_SEND_LOGMSG\" fail(%d)>\n", errsv);
			}
		}
	} else if (gmlib_dbg_mode == 1) {  //1: direct print
		printf("%s", str);
	}
	return ret;
}

int pif_clear_window(int lcd_vch, gm_clear_window_t *cw_str)
{
	int ret = 0;
	vpd_clr_win_t vpd_cw_str;

	if (!pif_init_data.init_nr) {
		ret = ERR_NOT_INIT_PLATFORM;
		GMLIB_PRINT_P("gmlib is not init!\n");
		goto ext;
	}
	if (cw_str == NULL) {
		GMLIB_ERROR_P("NULL cw_str\n");
		return -1;
	}

	pif_debug_clrwin(lcd_vch, cw_str);
	GMLIB_L3_P("lcd(%d) inW(%d) inH(%d) inF(%d) inB(%p) x(%d) y(%d) w(%d) h(%d) m(%d)\n", lcd_vch,
		   cw_str->in_w,
		   cw_str->in_h,
		   cw_str->in_fmt,
		   cw_str->buf,
		   cw_str->out_x,
		   cw_str->out_y,
		   cw_str->out_w,
		   cw_str->out_h,
		   cw_str->mode);
	PIF_ASSERT_LCD_VCH(lcd_vch);
	vpd_cw_str.in_width = cw_str->in_w;
	vpd_cw_str.in_height = cw_str->in_h;

	if (cw_str->in_fmt != VPD_BUFTYPE_YUV422) {
		GMLIB_ERROR_P("Error to support clear win format %d\n", cw_str->in_fmt);
		ret = -1;
		goto ext;
	}
	vpd_cw_str.in_fmt = VPD_BUFTYPE_YUV422;
	vpd_cw_str.in_buf = cw_str->buf;
	vpd_cw_str.out_x = cw_str->out_x;
	vpd_cw_str.out_y = cw_str->out_y;
	vpd_cw_str.out_width = cw_str->out_w;
	vpd_cw_str.out_height = cw_str->out_h;

	switch (cw_str->mode) {
	case GM_ACTIVE_BY_APPLY:
		vpd_cw_str.mode = VPD_ACTIVE_BY_APPLY;
		break;
	case GM_ACTIVE_IMMEDIATELY:
		vpd_cw_str.mode = VPD_ACTIVE_IMMEDIATELY;
		break;
	default:
		GMLIB_ERROR_P("clr_win_mode=%d error!\n", cw_str->mode);
		ret = -1;
		goto ext;
	}
	vpd_cw_str.lcd_vch = lcd_vch;
	ret = ioctl(vpd_fd, VPD_CLEAR_WINDOW, &vpd_cw_str);

	if (ret < 0) {
		GMLIB_ERROR_P("ioctl \"VPD_CLEAR_WINDOW\" return %d\n", ret);
		ret = -1;
	}

ext:
	GMLIB_L3_P("<clear_window ret=%d>\n", ret);
	return ret;
}

#define MAX_RAWDATA_SIZE 0x00010000

int pif_check_raw_data_scaling(gm_rect_t src_rect, gm_dim_t *out_dim, unsigned int buf_len)
{
	int min_width, min_height;
	unsigned int fixed_len;
	int ret = 0;

	if (out_dim == NULL) {
		GMLIB_ERROR_P("NULL out_dim\n");
		return -1;
	}

	min_width = ALIGN4_UP(src_rect.width / 8);
	min_height = ALIGN2_UP(src_rect.height / 8);
	if (out_dim->width < min_width) {
		GMLIB_PRINT_P("setting out_dim.width = %d is over vpe scaling down 8x ability, "
			      "fixed width to be %d.\n", out_dim->width, min_width);
		out_dim->width = min_width;
	}
	if (out_dim->height < min_height) {
		GMLIB_PRINT_P("setting out_dim.height = %d is over vpe scaling down 8x ability, "
			      "fixed height to be %d.\n", out_dim->height, min_height);
		out_dim->height = min_height;
	}
	fixed_len = out_dim->width * out_dim->height * 2;
	if (buf_len < fixed_len) {
		GMLIB_PRINT_P("yuv_buf_len = %d is not enough for getting raw_data, "
			      "please extern it to be %d in AP.\n", buf_len, fixed_len);
		ret = -1;
	}

	return ret;
}

int pif_set_for_graph_exit(void)
{
	unsigned int groupfd = 0;
	int ret;

	groupfd = pif_new_groupfd();
	if ((ret = pif_apply_force_graph_exit(groupfd)) < 0) {
		goto err_ext;
	}

err_ext:
	if (groupfd) {
		ret = pif_delete_groupfd(groupfd);
	}

	return ret;
}

int pif_set_display_rate(int lcd_vch, int display_rate)
{
	int ret = 0;

	pif_debug_set_display_rate(lcd_vch, display_rate);

	if (!pif_init_data.init_nr) {
		ret = ERR_NOT_INIT_PLATFORM;
		goto err_ext;
	}

	PIF_ASSERT_LCD_VCH(lcd_vch);
	pif_target_disp_rate[lcd_vch] = display_rate;
	ret = 0;  // GM_SUCCESS

err_ext:
	return ret;
}

int pif_env_update(void)
{
	int ret = 0;
	vpd_env_update_t vpd_env_update_cmdt;

	vpd_env_update_cmdt.which = 0;


	if (vpd_fd == 0) {
		vpd_fd = open("/dev/vpd", O_RDWR);
	}
	if (vpd_fd < 0) {
		ret = ERR_FAILED_OPEN_VPD_FILE;
		goto ext;
	}

	if ((ret = ioctl(vpd_fd, VPD_ENV_UPDATE, &vpd_env_update_cmdt)) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_PRINT_P("ioctl \"VPD_ENV_UPDATE\" driver into stop state.\n");
			exit(-1);
		} else {
			GMLIB_ERROR_P("<ioctl \"VPD_ENV_UPDATE\" fail>\n");
			goto ext;
		}
	}
	if ((ret = ioctl(vpd_fd, VPD_GET_PLATFORM_INFO, &platform_sys_Info)) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_PRINT_P("ioctl \"VPD_GET_PLATFORM_INFO\" driver into stop state.\n");
			exit(-1);
		} else {
			GMLIB_ERROR_P("<ioctl \"VPD_GET_PLATFORM_INFO\" fail>\n");
			goto ext;
		}
	}
	if ((ret = ioctl(vpd_fd, VPD_UPDATE_PLATFORM_INFO, &platform_sys_Info)) < 0) {
		int errsv = errno;

		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_PRINT_P("ioctl \"VPD_UPDATE_PLATFORM_INFO\" driver into stop state.\n");
			exit(-1);
		} else {
			GMLIB_ERROR_P("<ioctl \"VPD_UPDATE_PLATFORM_INFO\" fail>\n");
			goto ext;
		}
	}

ext:
	return ret;
}

void pif_debug_snapshot(snapshot_t *snapshot, unsigned int timeout_ms)
{
	unsigned int len = 0;

	if (snapshot == NULL) {
		GMLIB_ERROR_P("NULL snapshot\n");
		return;
	}

	memset(pif_msg, 0x0, PROC_MSG_SIZE);
	len += snprintf(pif_msg, PROC_MSG_SIZE, "gm_request_snapshot: entity_fd(0x%x) buf(%#lx) buf_len(%d)\n",
			(int)snapshot->entity_fd, (unsigned long)snapshot->bs_buf, snapshot->bs_buf_len);

	len += snprintf(pif_msg + len, PROC_MSG_SIZE,
			"                     bs_dim(%d, %d) quality(%d) timeout_ms(%d)",
			snapshot->bs_width, snapshot->bs_height, snapshot->image_quality, timeout_ms);
	len += sprintf(pif_msg + len, "\n");
	gmlib_flow_log(pif_msg, len);
}

void pif_debug_snapshot_result(unsigned int timestamp, unsigned int bs_size)
{
	unsigned int len = 0;

	memset(pif_msg, 0x0, PROC_MSG_SIZE);
	len += snprintf(pif_msg + len, PROC_MSG_SIZE,
			"                     timestamp(0x%x) bs_size(%d)\n", timestamp, bs_size);
	gmlib_flow_log(pif_msg, len);
}

void pif_debug_clrwin(int lcd_vch, gm_clear_window_t *cw_str)
{
	unsigned int len = 0;

	if (cw_str == NULL) {
		GMLIB_ERROR_P("NULL cw_str\n");
		return;
	}

	memset(pif_msg, 0x0, PROC_MSG_SIZE);
	len += snprintf(pif_msg + len, PROC_MSG_SIZE,
			"gm_clear_window: lcd_vch(%d) wh(%d, %d) fmt(%d) buf(%#lx) xywh(%d,%d,%d,%d) mode(%d)\n",
			lcd_vch, cw_str->in_w, cw_str->in_h, cw_str->in_fmt, (unsigned long)cw_str->buf,
			cw_str->out_x, cw_str->out_y, cw_str->out_w, cw_str->out_h, cw_str->mode);
	gmlib_flow_log(pif_msg, len);
}

void pif_debug_bitstream(void *obj, gm_clear_window_t *cw_str)
{
	unsigned int len = 0;

	if (obj == NULL) {
		GMLIB_ERROR_P("NULL obj\n");
		return;
	}
	if (cw_str == NULL) {
		GMLIB_ERROR_P("NULL cw_str\n");
		return;
	}

	memset(pif_msg, 0x0, PROC_MSG_SIZE);
	len += snprintf(pif_msg + len, PROC_MSG_SIZE,
			"gm_clear_bitstream: group_line(%#lx) wh(%d, %d) fmt(%d) buf(%#lx)\n",
			(unsigned long)obj, cw_str->in_w, cw_str->in_h, cw_str->in_fmt, (unsigned long)cw_str->buf);
	gmlib_flow_log(pif_msg, len);
}

void pif_debug_adjust_disp(int lcd_vch, int x, int y, int width, int height)
{
	unsigned int len = 0;

	memset(pif_msg, 0x0, PROC_MSG_SIZE);
	len += snprintf(pif_msg + len, PROC_MSG_SIZE,
			"gm_adjust_disp: lcd_vch(%d) xywh(%d,%d,%d,%d)\n",
			lcd_vch, x, y, width, height);
	gmlib_flow_log(pif_msg, len);
}

void pif_debug_set_display_rate(int lcd_vch, int display_rate)
{
	unsigned int len = 0;

	memset(pif_msg, 0x0, PROC_MSG_SIZE);
	len += snprintf(pif_msg + len, PROC_MSG_SIZE,
			"gm_set_display_rate: lcd_vch(%d) rate(%d)\n", lcd_vch, display_rate);
	gmlib_flow_log(pif_msg, len);
}

void pif_debug_cal_disp_rate(int cap_fps, int pb_fps, int lcd_fps, int ap_fps, int final_fps)
{
	unsigned int len = 0;

	memset(pif_msg, 0x0, PROC_MSG_SIZE);
	len += snprintf(pif_msg + len, PROC_MSG_SIZE,
			"gm_cal_disp_rate: cap_fps(%d) pb_fps(%d) lcd_fps(%d) AP->target_fps(%d) final_fps(%d)\n",
			cap_fps, pb_fps, lcd_fps, ap_fps, final_fps);
	gmlib_flow_log(pif_msg, len);
}

int pif_get_duplicate_lcd_vch(pif_group_t *group)
{
	INT i, line_idx, member_idx, lcd_vch;
	INT src_lcd_vch = -1, dup_lcd_vch = -1, count = 0;
	HD_GROUP *p_hd_group;

	if (group == NULL) {
		GMLIB_ERROR_P("NULL pif_group\n");
		goto exit;
	}
	p_hd_group = (HD_GROUP *)group->hd_group;
	if (p_hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto exit;
	}

	//find source LCD vch(ex: LCD0, the primary LCD)
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (p_hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE)  {
			continue;
		}
		for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
			if (p_hd_group->glines[line_idx].member[member_idx].p_bind == NULL)
				continue;
			if (bd_get_dev_baseid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid) != HD_DAL_VIDEOOUT_BASE) {
				continue;
			}
			lcd_vch = bd_get_dev_subid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid);
			if (lcd_vch >= VPD_MAX_LCD_NUM) {
				continue;
			}
			src_lcd_vch = platform_sys_Info.lcd_info[lcd_vch].lcd_vch;
			break;
		}
	}

	if (src_lcd_vch == -1) {
		GMLIB_ERROR_P("Source lcd is not found.\n");
		goto exit;
	}

	//find the duplicate LCD vch (ex: LCD1, the secondary LCD)
	for (i = 0; i < VPD_MAX_LCD_NUM; i++) {
		if (platform_sys_Info.lcd_info[i].lcdid == -1) {
			continue;
		}
		if (platform_sys_Info.lcd_info[i].lcd_vch == src_lcd_vch) {
			continue;
		}
		if (platform_sys_Info.lcd_info[i].src_duplicate_vch == src_lcd_vch) {
			dup_lcd_vch = platform_sys_Info.lcd_info[i].lcd_vch;
			count ++;
		}
	}

	if (count > 2) {
		GMLIB_ERROR_P("Not to support >2 duplicate LCD.\n");
		goto exit;
	}

exit:
	return dup_lcd_vch;
}

unsigned int pif_get_timestamp(void)
{
	unsigned int timestamp = 0;
	if ((ioctl(vpd_fd, VPD_GET_TIMESTAMP, &timestamp)) < 0) {
		perror("ioctl \"VPD_GET_TIMESTAMP\" fail!");
	}
	return timestamp;
}

unsigned long pif_get_timestamp2(void)
{
	vpd_get_jiffies_info_t jiffies_info;
	if ((ioctl(vpd_fd, VPD_GET_TIMESTAMP2, &jiffies_info)) < 0) {
		perror("ioctl \"VPD_GET_TIMESTAMP\" fail!");
	}
	return jiffies_info.nvt_jiffies_value;
}

int pif_alloc_user_fd_for_hdal(vpd_em_type_t type, int chip, int engine, int minor)
{
	vpd_usr_proc_fd_t pif_fd = {0};
	unsigned int fd = 0;

	pif_fd.type = type;
	pif_fd.chip = chip;
	pif_fd.engine = engine;
	pif_fd.minor = minor;
	if (ioctl(vpd_fd, VPD_ALLOC_USR_PROC_FD, &pif_fd) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_ERROR_P("ioctl \"VPD_ALLOC_USR_PROC_FD\" driver into stop state(%d).\n", errsv);
			goto ext;
		} else {
			fd = 0;
			goto ext;
		}
	}
	fd = pif_fd.fd;

ext:
	return fd;
}

//unsigned int pif_create_queue_for_hdal(void)
uintptr_t pif_create_queue_for_hdal(void)
{
	usr_proc_queue_info_t queue_info;

	queue_info.queue_handle = 0;
	if (ioctl(vpd_fd, USR_PROC_CREATE_CB_QUEUE, &queue_info) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_ERROR_P("ioctl \"USR_PROC_CREATE_CB_QUEUE\" driver into stop state(%d).\n", errsv);
		}
	}
	return queue_info.queue_handle;
}

//int pif_destroy_queue_for_hdal(unsigned int queue_handle)
int pif_destroy_queue_for_hdal(uintptr_t queue_handle)
{
	int ret = 0;
	usr_proc_queue_info_t queue_info;

	queue_info.queue_handle = queue_handle;
	if (ioctl(vpd_fd, USR_PROC_DESTROY_CB_QUEUE, &queue_info) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_ERROR_P("ioctl \"USR_PROC_DESTROY_CB_QUEUE\" driver into stop state(%d).\n", errsv);
		}
		ret = -1;
	}
	return ret;
}

int pif_set_mem_init(void *mem_init)
{
	int ret;

	if (mem_init == NULL) {
		GMLIB_ERROR_P("NULL mem_init\n");
		return -1;
	}
	ret = ioctl(vpd_fd, VPD_SET_MEM_INIT, mem_init);

	return ret;
}

int pif_set_module_init(void *module_init)
{
	int ret;

	if (module_init == NULL) {
		GMLIB_ERROR_P("NULL module_init\n");
		return -1;
	}
	ret = ioctl(vpd_fd, VPD_MODULE_INIT_DONE, module_init);

	return ret;
}

int pif_clear_usr_blk(void)
{
	int ret = 0;
	int val = 0;

	if (ioctl(vpd_fd, VPD_CLEAR_ALL_BLK, &val) < 0) {
		ret = -errno;
		if (ret == -1) {
			GMLIB_ERROR_P("free all blk fail\n");
		}
		GMLIB_ERROR_P("<ioctl \"VPD_CLEAR_ALL_BLK\" fail, error(%d)>\n", ret);
		goto exit;
	}
exit:
	return ret;
}

int pif_clear_usr_pool_blk(char pool_name[], int ddr_id)
{
	int ret = 0;
	vpd_clr_pool_blk_t clr_pool;

	clr_pool.ddrid = ddr_id;
	if (strlen(pool_name)) {
		strncpy(clr_pool.pool_name, pool_name, MAX_POOL_NAME_LEN);
	} else {
		GMLIB_ERROR_P("Check pool_name fail\n");
		return -1;
	}
	if (vpd_fd == 0) {
		vpd_fd = open("/dev/vpd", O_RDWR);
	}
	if (ioctl(vpd_fd, VPD_CLEAR_BLK_BY_POOL, &clr_pool) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -1) {
			GMLIB_ERROR_P("free pool(%s) ddrid(%d) blk fail\n", clr_pool.pool_name, clr_pool.ddrid);
		}
		GMLIB_ERROR_P("<ioctl \"VPD_CLEAR_BLK_BY_POOL\" fail, error(%d)>\n", ret);
		goto exit;
	}
exit:
	return ret;
}

int pif_get_mem_init_state(void)
{
	int init_state = 0;

    if (!vpd_fd && (vpd_fd = open("/dev/vpd", O_RDWR | O_SYNC)) < 0) {
        HD_COMMON_ERR("open /dev/vpd failed.\n");
        return init_state;
    }
    if (ioctl(vpd_fd, VPD_GET_MEM_INIT_STATE, &init_state) < 0) {
        int errsv = errno;
        if (errsv == abs(VPD_STOP_STATE)) {
            GMLIB_ERROR_P("ioctl \"VPD_GET_MEM_INIT_STATE\" driver into stop state(%d).\n", errsv);
        }
        init_state = -1;
    }

	return init_state;
}

int pif_set_mem_uninit(void)
{
	int ret;

	ret = ioctl(vpd_fd, VPD_SET_MEM_UNINIT, NULL);

	return ret;
}

int pif_get_pool(void *pool_info)
{
    int ret = -1;

    if (!vpd_fd && (vpd_fd = open("/dev/vpd", O_RDWR | O_SYNC)) < 0) {
        HD_COMMON_ERR("open /dev/vpd failed.\n");
        return ret;
    }
    ret = ioctl(vpd_fd, VPD_GET_POOL_INFO, pool_info);
    return ret;
}

int pif_get_usr_va_info(HD_COMMON_MEM_VIRT_INFO *p_vir_info)
{
	int ret = 0;
	vpd_get_usr_va_info_t va_info = {0};

	if (p_vir_info == NULL) {
		GMLIB_ERROR_P("NULL p_vir_info\n");
		goto exit;
	}

	va_info.va = (void *)p_vir_info->va;
	if (vpd_fd == 0) {
		vpd_fd = open("/dev/vpd", O_RDWR);
	}
	if (ioctl(vpd_fd, VPD_GET_USR_VA_INFO, &va_info) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_ERROR_P("ioctl \"VPD_GET_USR_VA_INFO\" driver into stop state(%d).\n", errsv);
		}
		ret = -1;
		goto exit;
	}
	p_vir_info->pa = va_info.pa;
	p_vir_info->cached = va_info.cached;

exit:
	return ret;
}

int pif_get_chip_by_ddr_id(int ddr_id)
{
	int i;
	for (i = 0; i < VPD_MAX_CHIP_NUM; i++) {
		if ((ddr_id >= platform_sys_Info.chip_info[i].start_ddr_no) &&
		    (ddr_id <= platform_sys_Info.chip_info[i].end_ddr_no))
			return i;
	}
	return -1;
}

int pif_set_hdal_max_fps(int lcd_ch, unsigned int max_fps)
{
	int ret = 0;
	vpd_hdal_max_fps_t lcd_fps = {0};


	if (vpd_fd == 0) {
		vpd_fd = open("/dev/vpd", O_RDWR);
	}
	lcd_fps.lcd_ch = lcd_ch;
	lcd_fps.max_fps = max_fps;

	if (ioctl(vpd_fd, VPD_SET_HDAL_MAX_FPS, &lcd_fps) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_ERROR_P("ioctl \"VPD_SET_HDAL_MAX_FPS\" fails(%d).\n", errsv);
		}
		ret = -1;
		goto exit;
	}

exit:
	return ret;
}

int pif_get_hdal_max_fps(int lcd_ch)
{
	int ret = 30;//default = 30
	vpd_hdal_max_fps_t va_info = {0};

	if (vpd_fd == 0) {
		vpd_fd = open("/dev/vpd", O_RDWR);
	}
	if (ioctl(vpd_fd, VPD_GET_HDAL_MAX_FPS, &va_info) < 0) {
		goto exit;
	}
exit:
	return ret;
}

int pif_get_ddr_id_by_chip(int chip, int *p_start_ddr_no, int *p_end_ddr_no)
{
	if (chip >= VPD_MAX_CHIP_NUM)
		return -1;
	if (platform_sys_Info.chip_info[chip].start_ddr_no >= VPD_MAX_DDR_NUM || platform_sys_Info.chip_info[chip].end_ddr_no >= VPD_MAX_DDR_NUM)
		return -1;
	*p_start_ddr_no = platform_sys_Info.chip_info[chip].start_ddr_no;
	*p_end_ddr_no = platform_sys_Info.chip_info[chip].end_ddr_no;
	return 0;
}

char pif_get_chipnu(void)
{
	return platform_sys_Info.chip_num;
}

int pif_is_liveview(pif_group_t *group)
{
	int line_idx, member_idx, is_cap = 0, is_lcd = 0;
	HD_GROUP *p_hd_group;

	if (group == NULL) {
		GMLIB_ERROR_P("NULL pif_group\n");
		goto exit;
	}
	p_hd_group = (HD_GROUP *)group->hd_group;
	if (p_hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto exit;
	}

	/* check line type */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (p_hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE)  {
			continue;
		}
		for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
			if (p_hd_group->glines[line_idx].member[member_idx].p_bind == NULL)
				continue;

			if (bd_get_dev_baseid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE) {
				is_cap = 1;
			} else if (bd_get_dev_baseid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
				is_lcd = 1;
			}
		}
	}

exit:
	return (is_cap & is_lcd);
}

int pif_is_playback(pif_group_t *group)
{
	int line_idx, member_idx, is_dec = 0, is_lcd = 0;
	HD_GROUP *p_hd_group;

	if (group == NULL) {
		GMLIB_ERROR_P("NULL pif_group\n");
		goto exit;
	}
	p_hd_group = (HD_GROUP *)group->hd_group;
	if (p_hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto exit;
	}

	/* check line type */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (p_hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE)  {
			continue;
		}
		for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
			if (p_hd_group->glines[line_idx].member[member_idx].p_bind == NULL)
				continue;

			if (bd_get_dev_baseid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) {
				is_dec = 1;
			} else if (bd_get_dev_baseid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
				is_lcd = 1;
			}
		}
	}

exit:
	return (is_dec & is_lcd);
}

// calculate all of the LCD fps
int pif_cal_disp_max_fps(pif_group_t *group)
{
	int i, line_idx, member_idx, is_liveview = 0, is_playback = 0;
	int max_cap_fps = 0, cap_fps = 0, max_pb_fps = 0, pb_fps = 0;
	int disp_fps = 10, max_disp_fps = 0, lcd_vch;
	HD_GROUP *p_hd_group;
	VDODEC_INTL_PARAM *vdec_param;

	if (group == NULL) {
		GMLIB_ERROR_P("NULL pif_group\n");
		goto err_exit;
	}
	p_hd_group = (HD_GROUP *)group->hd_group;
	if (p_hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}

	/* check line type */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (p_hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE ||
		    p_hd_group->glines[line_idx].state == HD_LINE_STOP ||
		    p_hd_group->glines[line_idx].state == HD_LINE_STOP_ONGOING)  {
			continue;
		}
		for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
			if (p_hd_group->glines[line_idx].member[member_idx].p_bind == NULL)
				continue;

			/* get max_fps of playback */
			if (bd_get_dev_baseid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) {
				vdec_param = videodec_get_param_p(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid,
								  p_hd_group->glines[line_idx].member[member_idx].out_subid);
				if (vdec_param) {
					pb_fps = vdec_param->max_mem->max_fps;
					max_pb_fps = MAX_VAL(max_pb_fps, pb_fps);
				}
				if (max_pb_fps == 0) {
					max_pb_fps = 30;
				}
				is_playback = 1;

				/* get max_fps of vcap */
			} else if (bd_get_dev_baseid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE) {
				for (i = 0; i < CAPTURE_VCH_NUMBER; i++) {
					if (platform_sys_Info.cap_info[i].ch < 0) { //A continuous list(ch) in cap_info. no break.
						continue;
					}
					if (platform_sys_Info.cap_info[i].vcapch == VPD_CASCADE_CH_IDX) {
						continue;
					}
					cap_fps = platform_sys_Info.cap_info[i].fps;
					if (platform_sys_Info.cap_info[i].scan_method == GM_INTERLACED) {
						cap_fps = cap_fps * 2;
					}
					max_cap_fps = MAX_VAL(max_cap_fps, cap_fps);
				}
				is_liveview = 1;
			} else if (bd_get_dev_baseid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid) == HD_DAL_VIDEOOUT_BASE) {
				lcd_vch = bd_get_dev_subid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid);
				max_disp_fps = MAX_VAL(max_disp_fps, platform_sys_Info.lcd_info[lcd_vch].fps);
				GMLIB_L1_P("%s line_idx(%d) lcd_vch(%d) fps(%d) max_disp_fps(%d)\n", __func__,
					   line_idx, lcd_vch, platform_sys_Info.lcd_info[lcd_vch].fps, max_disp_fps);
			} else {
				continue;
			}
		}
	}

	if (is_liveview) {
		disp_fps = MAX_VAL(disp_fps, max_cap_fps);
	}
	if (is_playback) {
		disp_fps = MAX_VAL(disp_fps, max_pb_fps);
	}
	if (max_disp_fps) {
		disp_fps = MIN_VAL(disp_fps, max_disp_fps);
	}
	return disp_fps;

err_exit:
	return 30;
}


int pif_cal_disp_tick_fps(int lcd_vch)
{
	int disp_fps = 30;
	unsigned int hdal_max_fps = 0;


	if (lcd_vch < 0 || lcd_vch >= VPD_MAX_LCD_NUM) {
		printf("%s: error lcd index %d overflow.\n", __func__, lcd_vch);
		return disp_fps;
	}
	pif_get_disp_max_fps(lcd_vch, &hdal_max_fps);
	display_max_fps[lcd_vch] = hdal_max_fps;
	switch (display_max_fps[lcd_vch]) {
	case 60: // 60 fps
		if (platform_sys_Info.lcd_info[lcd_vch].fps == 60) {
			disp_fps = 60;
		} else if (platform_sys_Info.lcd_info[lcd_vch].fps == 50) {
			disp_fps = 50;
		} else if (platform_sys_Info.lcd_info[lcd_vch].fps > 30) {
			disp_fps = 30;
		} else {
			disp_fps = platform_sys_Info.lcd_info[lcd_vch].fps;
		}
		break;

	default: // 30fps
		if (platform_sys_Info.lcd_info[lcd_vch].fps == 60) {
			disp_fps = 30;
		} else if (platform_sys_Info.lcd_info[lcd_vch].fps == 50) {
			disp_fps = 25;
		} else if (platform_sys_Info.lcd_info[lcd_vch].fps > 30) {
			disp_fps = 30;
		} else {
			disp_fps = platform_sys_Info.lcd_info[lcd_vch].fps;
		}
	}

	return disp_fps;
}


int pif_cal_min_disp_fps(pif_group_t *group, int lcd_vch)
{
	int i;
	int line_idx, member_idx, is_liveview = 0, is_playback = 0;
	int max_cap_fps = 0, cap_fps = 0;
	int max_pb_fps = 0, pb_fps = 0;
	int disp_fps = 0, target_disp_fps;
	HD_GROUP *p_hd_group;
	VDODEC_INTL_PARAM *vdec_param;

	if (group == NULL) {
		GMLIB_ERROR_P("NULL pif_group\n");
		goto err_exit;
	}
	p_hd_group = (HD_GROUP *)group->hd_group;
	if (p_hd_group == NULL) {
		GMLIB_ERROR_P("NULL HD_GROUP\n");
		goto err_exit;
	}

	/* check line type */
	for (line_idx = 0; line_idx < HD_MAX_GRAPH_LINE; line_idx++) {
		if (p_hd_group->glines[line_idx].state == HD_LINE_INCOMPLETE ||
		    p_hd_group->glines[line_idx].state == HD_LINE_STOP ||
		    p_hd_group->glines[line_idx].state == HD_LINE_STOP_ONGOING)  {
			continue;
		}
		for (member_idx = 0; member_idx < HD_MAX_BIND_NUNBER_PER_LINE; member_idx++) {
			if (p_hd_group->glines[line_idx].member[member_idx].p_bind == NULL)
				continue;

			/* get max_fps of playback */
			if (bd_get_dev_baseid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid) == HD_DAL_VIDEODEC_BASE) {
				vdec_param = videodec_get_param_p(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid,
								  p_hd_group->glines[line_idx].member[member_idx].out_subid);
				if (vdec_param) {
					pb_fps = vdec_param->max_mem->max_fps;
					max_pb_fps = MAX_VAL(max_pb_fps, pb_fps);
				}
				if (max_pb_fps == 0) {
					max_pb_fps = 30;
				}
				is_playback = 1;

				/* get max_fps of vcap */
			} else if (bd_get_dev_baseid(p_hd_group->glines[line_idx].member[member_idx].p_bind->device.deviceid) == HD_DAL_VIDEOCAP_BASE) {
				for (i = 0; i < CAPTURE_VCH_NUMBER; i++) {
					if (platform_sys_Info.cap_info[i].ch < 0) { //A continuous list(ch) in cap_info. no break.
						continue;
					}
					if (platform_sys_Info.cap_info[i].vcapch == VPD_CASCADE_CH_IDX) {
						continue;
					}
					cap_fps = platform_sys_Info.cap_info[i].fps;
#if 0
// CAP is always set as "frame mode". It doesn't need to fps*2
// Find the keyword: cap_entity->method = VPD_CAP_METHOD_FRAME;
					if (platform_sys_Info.cap_info[i].scan_method == GM_INTERLACED) {
						cap_fps = cap_fps * 2;
					}
#endif
					max_cap_fps = MAX_VAL(max_cap_fps, cap_fps);
				}
				is_liveview = 1;
			} else {
				continue;
			}
		}
	}

	/* get final disp_fps of lcd */
	target_disp_fps = pif_target_disp_rate[lcd_vch];
	if (target_disp_fps) { /* AP set target display rate */
		disp_fps = MIN_VAL(target_disp_fps, platform_sys_Info.lcd_info[lcd_vch].fps);
	} else {
		if (is_liveview) {
			disp_fps = MAX_VAL(disp_fps, max_cap_fps);
		}
		if (is_playback) {
			disp_fps = MAX_VAL(disp_fps, max_pb_fps);
		}
		if (disp_fps < 10) {
			disp_fps = 10;
		}
	}
	disp_fps = MIN_VAL(disp_fps, platform_sys_Info.lcd_info[lcd_vch].fps);

	pif_debug_cal_disp_rate(max_cap_fps, max_pb_fps, platform_sys_Info.lcd_info[lcd_vch].fps,
				target_disp_fps, disp_fps);
	return disp_fps;

err_exit:
	return 30;
}

int pif_check_dst_property_value(rect_t dst_rect, dim_t dst_bg_dim)
{
	if (dst_rect.x + dst_rect.w > dst_bg_dim.w) {
		GMLIB_ERROR_P("Error! vproc dst_x(%d) + dst_w(%d) > dst_bg_w(%d)\n",
			      dst_rect.x, dst_rect.w, dst_bg_dim.w);
		return -1;
	} else if (dst_rect.y + dst_rect.h > dst_bg_dim.h) {
		GMLIB_ERROR_P("Error! vproc dst_y(%d) + dst_h(%d) > dst_bg_h(%d)\n",
			      dst_rect.y, dst_rect.h, dst_bg_dim.h);
		return -1;
	} else {
		return 1;
	}
}

unsigned int pif_get_cap_channel_number(void)
{
	unsigned int i, max_channel_number = 0;

	for (i = 0; i < VPD_MAX_CAPTURE_CHANNEL_NUM; i++) {
		if (platform_sys_Info.cap_info[i].vcapch < 0) { //A continuous list(ch) in cap_info. no break.
			continue;
		}
		max_channel_number++;
	}
	return (max_channel_number);
}

void pif_set_disp_max_fps(int lcd_vch, unsigned int max_fps)
{
	vpd_hdal_max_fps_t  hdal_fps = {0};

	if (lcd_vch < 0 || lcd_vch >= VPD_MAX_LCD_NUM) {
		printf("%s: error lcd index %d overflow.\n", __func__, lcd_vch);
		return;
	}

	/* for virtual LCD */
	if (lcd_vch >= MAX_VIDEOOUT_CTRL_ID) {
		display_max_fps[lcd_vch] = max_fps;
	} else {
		if (max_fps == 60)
			display_max_fps[lcd_vch] = 60;
		else
			display_max_fps[lcd_vch] = 30;
	}
	hdal_fps.lcd_ch = lcd_vch;
	hdal_fps.max_fps = display_max_fps[lcd_vch];
	if ((ioctl(vpd_fd, VPD_SET_HDAL_MAX_FPS, &hdal_fps)) < 0) {
		GMLIB_ERROR_P("ioctl \"VPD_SET_HDAL_MAX_FPS\" fail");
		return;
	}
}

static UINTPTR pif_get_mapped_pa(nvtpcie_chipid_t loc_chipid, nvtpcie_chipid_t tar_chipid, unsigned long tar_pa)
{
	NVTPCIE_IOARG_GET_MAPPED_PA arg = {0};
	int fd = -1;

	fd = open(PCIE_DEV_NAME, O_RDWR);
	if (fd < 0) {
		return (UINTPTR)-1;
	}

	arg.loc_chipid = loc_chipid;
	arg.tar_chipid = tar_chipid;
	arg.tar_pa = tar_pa;

	if (0 != ioctl(fd, NVTPCIE_IOCMD_GET_MAPPED_PA, &arg)) {
		printf("ERR: ioctl(NVTPCIE_IOCMD_GET_MAPPED_PA), errno = %d\r\n", errno);
		close(fd);
		return (UINTPTR)-1;
	}

	close(fd);
	return (UINTPTR)arg.ret_mapped_pa;
}

int pif_address_ddr_sanity_check(UINTPTR pa, HD_COMMON_MEM_DDR_ID ddrid)
{
	static UINTPTR pcie_map_offset = 0;
	
	if (ddrid == DDR_ID_MAX) {
		GMLIB_ERROR_P("Check ddrid error\n");
		return -2;
	}

	if (pcie_map_offset == 0) {
		pcie_map_offset = pif_get_mapped_pa(COMMON_PCIE_CHIP_EP0,COMMON_PCIE_CHIP_RC,  0);
		GMLIB_L1_P("%s:pcie_map_offset=%#lx\n", __func__, (unsigned long)pcie_map_offset);
	}

	if (pa >= pcie_map_offset)
		return 0;
	
	GMLIB_L3_P("pif_address_ddr_sanity_check:pa(%#lx) ddrid(%d)\n",pa, ddrid);
	if (!hdal_ddr_info[ddrid].start_addr || !hdal_ddr_info[ddrid].ddr_size || !hdal_ddr_info[ddrid].end_addr) {
		if (pif_get_mem_init_state() > 0) {
			if ((ioctl(vpd_fd, VPD_GET_HDAL_DDR_INFO, hdal_ddr_info)) < 0) {
				GMLIB_ERROR_P("ioctl \"VPD_GET_HDAL_DDR_INFO\" fail");
			}
		} else {
			GMLIB_ERROR_P("pif_get_mem_init_state fail\n");
		}
	}
	if ( pa < hdal_ddr_info[ddrid].start_addr || pa >= hdal_ddr_info[ddrid].end_addr) {
		GMLIB_ERROR_P("Sanity check error:pa(%#lx) ddrid(%d), hdal ddr%d range(%#lx~%#lx) \n", pa, ddrid, ddrid,
			hdal_ddr_info[ddrid].start_addr, hdal_ddr_info[ddrid].end_addr);
		return -1;
	}
	return 0;
}

int pif_address_sanity_check(UINTPTR pa)
{
	int ddrid;
	int start_ddr_no, end_ddr_no;
	static UINTPTR pcie_map_offset = 0;
	
	if (pif_get_ddr_id_by_chip(COMMON_PCIE_CHIP_RC, &start_ddr_no, &end_ddr_no) < 0) {
		GMLIB_ERROR_P("pif_get_ddr_id_by_chip fail\n");
		return -2;
	}
	if (pcie_map_offset == 0) {
		pcie_map_offset = pif_get_mapped_pa(COMMON_PCIE_CHIP_EP0, COMMON_PCIE_CHIP_RC, 0);
		GMLIB_L1_P("%s:pcie_map_offset=%#lx\n", __func__, (unsigned long)pcie_map_offset);
	}
	if (pa >= pcie_map_offset)
		return 0;
	GMLIB_L3_P("pif_address_sanity_check:pa(%#lx) \n",pa);
	for (ddrid = start_ddr_no; ddrid <= end_ddr_no; ddrid++) {
		if (!hdal_ddr_info[ddrid].start_addr && pif_get_mem_init_state() > 0) {//for not call hd_common_init case
			if ((ioctl(vpd_fd, VPD_GET_HDAL_DDR_INFO, hdal_ddr_info)) < 0) {
				GMLIB_ERROR_P("ioctl \"VPD_GET_HDAL_DDR_INFO\" fail");
				goto exit;
			}
		}
		if (pa >= hdal_ddr_info[ddrid].start_addr && pa < hdal_ddr_info[ddrid].end_addr) {
			return 0;
		}
	}
exit:
	GMLIB_ERROR_P("%s:Check pa(%#lx) error\n", __func__, pa);
	for (ddrid = DDR_ID0; ddrid < DDR_ID_MAX; ddrid++) {
	    if (hdal_ddr_info[ddrid].start_addr)
	        GMLIB_PRINT_P("hdal ddr%d range(%#lx~%#lx)\n", ddrid, hdal_ddr_info[ddrid].start_addr, hdal_ddr_info[ddrid].end_addr);
	}
	return -1;
}

int pif_set_check_trigger_buf(vpd_chk_trigger_buf_t param)
{

       if (param.type < USR_TRIGGER_TYPE_CAP || param.type >=VPD_VPETRIG_MAXNUM) {
            GMLIB_ERROR_P("Check type=%d fail\n", param.type);
            return -1;
       }

       if (param.chk_in_buf != 0 && param.chk_in_buf != 1) {
            GMLIB_ERROR_P("Check chk_in_buf=%d fail\n", param.chk_in_buf);
            return -1;
       }

       if (param.chk_out_buf != 0 && param.chk_out_buf != 1) {
            GMLIB_ERROR_P("Check chk_out_buf=%d fail\n", param.chk_out_buf);
            return -1;
       }

       if ((ioctl(vpd_fd, VPD_SET_CHK_TRI_BUF_INFO, &param)) < 0) {
		GMLIB_ERROR_P("ioctl \"VPD_SET_CHK_TRI_BUF_INFO\" fail");
                return -2;
	}
	return 0;
}

int pif_get_check_trigger_buf(vpd_chk_trigger_buf_t *p_param)
{


       if (p_param->type < USR_TRIGGER_TYPE_CAP || p_param->type >=VPD_VPETRIG_MAXNUM) {
            GMLIB_ERROR_P("Check type=%d fail\n", p_param->type);
            return -1;
       }

       if ((ioctl(vpd_fd, VPD_GET_CHK_TRI_BUF_INFO, p_param)) < 0) {
		GMLIB_ERROR_P("ioctl \"VPD_GET_CHK_TRI_BUF_INFO\" fail");
                return -2;
	}
	return 0;
}

void pif_get_disp_max_fps(int lcd_vch, unsigned int *max_fps)
{
	vpd_hdal_max_fps_t  hdal_fps = {0};
	if (lcd_vch < 0 || lcd_vch >= VPD_MAX_LCD_NUM) {
		printf("%s: error lcd index %d overflow.\n", __func__, lcd_vch);
		return;
	}
	if (vpd_fd == 0) {
		*max_fps = 30;
	}
	hdal_fps.lcd_ch = lcd_vch;
	if ((ioctl(vpd_fd, VPD_GET_HDAL_MAX_FPS, &hdal_fps)) < 0) {
		GMLIB_ERROR_P("ioctl \"VPD_GET_HDAL_MAX_FPS\" fail");
		*max_fps = 30;
	    return;
	}
	if (hdal_fps.max_fps) {
		*max_fps = hdal_fps.max_fps;
	}
}

MEM_TYPE_t pif_get_pool_mem_type(vpbuf_pool_type_t pool_type)
{
	vpd_pool_mem_type_t pool_mem_type = {0};

	pool_mem_type.pool_type = pool_type;
	if ((ioctl(vpd_fd, VPD_GET_POOL_MEM_TYPE, &pool_mem_type)) < 0) {
		GMLIB_ERROR_P("ioctl \"VPD_GET_POOL_MEM_TYPE\" fail");
	    return -1;
	}

	if (pool_mem_type.mem_type == 1) {
		return MEM_VAR_TYPE;
	} else if (pool_mem_type.mem_type == 0) {
		return MEM_FIX_TYPE;
	} else {
		return MEM_UNKNOW_TYPE;
	}
}

HD_COMMON_MEM_VB_BLK pif_get_blk_by_pa(UINT32 pa)
{
    int ddrid;
    int start_ddr_no, end_ddr_no;
    HD_COMMON_MEM_VB_BLK blk = HD_COMMON_MEM_VB_INVALID_BLK;

    if (pif_get_ddr_id_by_chip(COMMON_PCIE_CHIP_RC, &start_ddr_no, &end_ddr_no) < 0) {
        GMLIB_ERROR_P("pif_get_ddr_id_by_chip fail\n");
        return blk;
    }

    for (ddrid = start_ddr_no; ddrid <= end_ddr_no; ddrid++) {
        if (!hdal_ddr_info[ddrid].start_addr && pif_get_mem_init_state() > 0) {//for not call hd_common_init case
            if ((ioctl(vpd_fd, VPD_GET_HDAL_DDR_INFO, hdal_ddr_info)) < 0) {
	        GMLIB_ERROR_P("ioctl \"VPD_GET_HDAL_DDR_INFO\" fail");
                return blk;
	    }
        } else {
            if (pa >= hdal_ddr_info[ddrid].start_addr && pa < hdal_ddr_info[ddrid].end_addr) {
                blk = MAKEBLK(pa, ddrid);
                break;
            }
        }
    }
    return blk;
}

