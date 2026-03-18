/**
    @brief Header file of bind of library.\n
    This file contains the bind functions of library.

    @file hd_common.c

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
#include <sys/mman.h>
#include <pthread.h>
#include "hd_type.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <comm/dma_util.h>
#include <comm/nvtmem_if.h>

#include "hdal.h"
#include "vpd_ioctl.h"
#include "pif.h"
#include "utl.h"
#include "kflow_videoenc_h26x.h"
#include "kflow_videodec.h"
#include "cif_common.h"
#include "hd_debug_menu.h"
#include "hd_version.h"
#include "logger.h"
#include "log.h"
#include "pif_ioctl.h"

/******for allo from ms*****/
static int vpd_fd = 0;
static int nvtmem_fd = -1;
struct map_info {
	void *user_va;
	void *va;
	UINTPTR  pa;
	unsigned int map_size;
};

typedef struct {
	uintptr_t	start_addr;
	uintptr_t   end_addr;
} memory_range_info_t;

struct map_info  mmap_tbl[HD_COMMON_MEM_MAX_MMAP_NUM] = {0};
extern void *pif_alloc_video_buffer_for_hdal(int size, int ddr_id, char pool_name[], int alloc_tag);
extern int pif_free_video_buffer_for_hdal(void *pa, int ddr_id, int alloc_tag);
extern void *pif_alloc_video_buffer_for_hdal_with_name(int size, int ddr_id, char pool_name[], int alloc_tag, char usr_name[]);

pthread_mutex_t mmap_mutex;

/***************************/
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define MAKEBLK(pa, ddrid) ((UINTPTR)pa) | (((UINTPTR)ddrid) & (0xFF))
#define GET_PA(blk) (((blk) >> 8) << 8)  // (blk&0xFFFFFF00)
#define GET_DDRID(blk) ((UINT8)(blk&0x000000FF))
#define CHECK_ALIGN_256(x)      (((x) & 0xFF) ? 0 : 1)

#define HD_COMMON_NON_INIT   0
#define HD_COMMON_INIT_1ST   1
#define HD_COMMON_INIT_2ND   2
#define HD_MEM_SHARED_POOL_NUM           4
#define GET_SHAREPOOL_ID(val,idx)	(idx < HD_MEM_SHARED_POOL_NUM ?(val >> (idx * 8)) & 0xFF : VPD_END_POOL)
#define USE_NVTMEM_MMAP	1
#define USE_NVTMEM_CACHE_FLUSH	1
#define MAX_KER_MBLK_NU	32
#define KERNEL_SYS_DEBUG_PATH	"/sys/kernel/debug/memblock/memory"
#define HKVS_PATH "/proc/hkvs"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern unsigned int hdal_flow_dbgmode;

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
HD_RESULT bd_dal_init(void);
HD_RESULT bd_dal_uninit(void);

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static HD_COMMON_MEM_INIT_CONFIG hdl_mem_init;
static UINT enc_ref_buf_id = 0;
static UINT common_init_cnt = 0;
static unsigned int ask_vpd_hdal_status = 0;

/* refer to structure HD_COMMON_MEM_MEM_TYPE */
static CHAR *mem_type_str[] = {  /* refer to 'HD_COMMON_MEM_MEM_TYPE' */
	"CACHE",
	"NONCACHE",
	"ERR_MEM_TYPE",
};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT hd_common_share_init(unsigned int value)
{
	vpd_get_hdal_module_info_t send_info = {0};
	int ret, close_vpd = 0;

	if (vpd_fd <= 0) {
		//now vpd closed
		if (value == 0) {
			close_vpd = 1;//close vpd, after reset
		}
		vpd_fd = open("/dev/vpd", O_RDWR | O_SYNC);
		if (vpd_fd < 0) {
			ret = ERR_FAILED_OPEN_VPD_FILE;
			printf("ERR_FAILED_OPEN_VPD_FILE");
			return HD_ERR_SYS;
		}
	}
	send_info.cmd_type = SHM_SETCMD_WANT_CHECK;
	send_info.set_value = value;

	ret = ioctl(vpd_fd, VPD_SHM_SET_HDAL_PID, &send_info);
	if (ret < 0) {
		int errsv = abs(errno);
		GMLIB_PRINT_P("<ioctl \"VPD_SHM_SET_HDAL_PID11\" fail(%d)>\n", errsv);
		GMLIB_PRINT_P("ret11 = 0x%x, errsv=0x%x\n", ret, errsv);
	}
	HD_COMMON_FLOW("hd_common_set_checkvpd:\n");
	HD_COMMON_FLOW("    value(%u) pid(%d)\n", value, (int)syscall(SYS_gettid));
	if (close_vpd) {
		close(vpd_fd);
		vpd_fd = -1;
	}
	return HD_OK;
}

HD_RESULT hd_common_get_checkvpd(unsigned int *pvalue)
{
	vpd_get_hdal_module_info_t send_info = {0};
	int ret;
	if (vpd_fd == 0) {
		*pvalue = 0;
		HD_COMMON_FLOW("hd_common_get_checkvpd:\n");
		HD_COMMON_FLOW("    value(=0) pid(%d)\n", (int)syscall(SYS_gettid));
		return HD_OK;
	}
	if ((ret = ioctl(vpd_fd, VPD_SHM_GET_HDAL_PID, &send_info)) < 0) {
		int errsv = errno;
		if (errsv != abs(VPD_STOP_STATE)) {
			GMLIB_PRINT_P("<ioctl \"VPD_SHM_GET_HDAL_PID22\" fail(%d)>\n", errsv);
		}
	}
	*pvalue = send_info.want_check;
	HD_COMMON_FLOW("hd_common_get_checkvpd:\n");
	HD_COMMON_FLOW("    value(%u) pid(%d)\n", *pvalue, (int)syscall(SYS_gettid));

	return HD_OK;
}



int hd_common_add_pid(void)
{
	int ret = 0;

	vpd_get_hdal_module_info_t send_info = {0};
	pid_t now_pid = 0;

	if (vpd_fd == 0) {
		vpd_fd = open("/dev/vpd", O_RDWR | O_SYNC);
	}
	if (vpd_fd < 0) {
		ret = ERR_FAILED_OPEN_VPD_FILE;
		goto ext;
	}
	now_pid = getpid();
	send_info.cmd_type = SHM_SETCMD_COMMON_OPEN;
	send_info.set_value = 1;
	send_info.go_pid[0] = (unsigned int)now_pid;

	if ((ret = ioctl(vpd_fd, VPD_SHM_SET_HDAL_PID, &send_info)) < 0) {
		int errsv = errno;
		if (errsv != abs(VPD_STOP_STATE)) {
			GMLIB_PRINT_P("<ioctl \"VPD_SHM_SET_HDAL_PID33\" fail(%d)>\n", errsv);
		}
	}

	HD_COMMON_FLOW("hd_common_add_pid:\n");
	HD_COMMON_FLOW("    pid(%d)\n", now_pid);

ext:
	return ret;
}

int hd_common_del_pid(void)
{
	int ret = 0;

	vpd_get_hdal_module_info_t send_info = {0};
	pid_t now_pid = 0;

	if (vpd_fd == 0) {
		ret = ERR_FAILED_OPEN_VPD_FILE;
		printf("hd_common_del_pid: vpd_fd NOT open!!\n");
		goto ext;
	}

	now_pid = getpid();
	send_info.cmd_type = SHM_SETCMD_COMMON_OPEN;
	send_info.set_value = 0;
	send_info.go_pid[0] = now_pid;

	if ((ret = ioctl(vpd_fd, VPD_SHM_SET_HDAL_PID, &send_info)) < 0) {
		int errsv = errno;
		if (errsv != abs(VPD_STOP_STATE)) {
			GMLIB_PRINT_P("<ioctl \"VPD_SHM_SET_HDAL_PID22\" fail(%d)>\n", errsv);
		}
	}
	HD_COMMON_FLOW("hd_common_del_pid:\n");
	HD_COMMON_FLOW("    pid(%d)\n", now_pid);

ext:
	return ret;
}

int hd_common_check_pid(unsigned int cmd, unsigned int *value, unsigned int ask_vpd)
{
	int ret = -1;
	unsigned int check_vpd = 0;
	vpd_get_hdal_module_info_t hdal_info = {0};

	if (vpd_fd == 0) {
		*value = 0;
		ret = 0;
		goto ext;
	}

	if (vpd_fd < 0) {
		ret = ERR_FAILED_OPEN_VPD_FILE;
		goto ext;
	}
	hd_common_get_checkvpd(&check_vpd);
	if (check_vpd == 0) {
		*value = 0;
		return 0;
	}

	if ((ret = ioctl(vpd_fd, VPD_SHM_GET_HDAL_PID, &hdal_info)) < 0) {
		int errsv = errno;
		if (errsv == abs(VPD_STOP_STATE)) {
			GMLIB_PRINT_P("ioctl \"VPD_UPDATE_PLATFORM_INFO\" driver into stop state.\n");
			exit(-1);
		} else {
			GMLIB_ERROR_P("<ioctl \"VPD_UPDATE_PLATFORM_INFO\" fail>\n");
			goto ext;
		}
	}

	if (hdal_info.status & SHM_SET_STATUS_COMMON) {
		*value = 1;
	} else {
		*value = 0;
	}

	return 0;
ext:
	return ret;
}

HD_RESULT hd_common_init(UINT32 sys_config_type)
{
	static int log_fd = -1;
	int chk_ret = HD_COMMON_NON_INIT;
	unsigned int hdal_version = HDAL_VERSION;
	unsigned int impl_version = HDAL_IMPL_VERSION;
	unsigned int common_open = 0;

	common_init_cnt++;
	if (common_init_cnt > 1) {
		goto ext;
	}

	hd_common_check_pid(0, &common_open, ask_vpd_hdal_status);
	hd_common_add_pid();
	if (common_open == 0) {
		chk_ret = HD_COMMON_INIT_1ST;
	} else {
		chk_ret = HD_COMMON_INIT_2ND;
	}

	if (chk_ret == HD_COMMON_INIT_2ND) {
		return HD_OK;
	}
	printf("HDAL IMPL Version: v%x.%02x.%03x\r\n",
		      (impl_version & 0xF00000) >> 20,
		      (impl_version & 0x0FF000) >> 12,
		      (impl_version & 0x000FFF));

	if ((log_fd = open("/dev/log_vg", O_RDWR | O_SYNC)) < 0) {
		HD_COMMON_ERR("open /dev/log_vg failed.\n");
		goto err_ext;
	}
	if (ioctl(log_fd, IOCTL_SET_HDAL_VERSION, &hdal_version) < 0) {
		HD_COMMON_ERR("Set HDAL Version failed.\n");
		return HD_ERR_NG;
	}

	if (ioctl(log_fd, IOCTL_SET_IMPL_VERSION, &impl_version) < 0) {
		HD_COMMON_ERR("Set HDAL Version failed.\n");
		return HD_ERR_NG;
	}

	if ((ioctl(log_fd, IOCTL_GET_HDAL_DBGMODE, &hdal_flow_dbgmode)) < 0) {
		HD_COMMON_ERR("Get HDAL DBGMODE failed.\n");
		return HD_ERR_NG;
	}

	if ((vpd_fd = open("/dev/vpd", O_RDWR | O_SYNC)) < 0) {
		HD_COMMON_ERR("open /dev/vpd failed.\n");
		goto err_ext;
	}

	if ((nvtmem_fd = open("/dev/nvtmem0", O_RDWR)) < 0) {
		HD_COMMON_ERR("open /dev/nvtmem0 failed.\n");
		goto err_ext;
	}
	memset(mmap_tbl, 0, sizeof(mmap_tbl));
	if (pthread_mutex_init(&mmap_mutex, NULL)) {
		HD_COMMON_ERR("%s:%d\n", __FUNCTION__, __LINE__);
		goto err_ext;
	}
	if (bd_dal_init() != HD_OK) {
		HD_COMMON_ERR("%s:%d\n", __FUNCTION__, __LINE__);
		goto err_ext;
	}

	if (log_fd) {
		close(log_fd);
	}

ext:
	HD_COMMON_FLOW("hd_common_init:\n");
	HD_COMMON_FLOW("    count(%d) pid(%d)\n", common_init_cnt, (int)syscall(SYS_gettid));
	return HD_OK;

err_ext:
	return HD_ERR_NG;
}

HD_RESULT hd_common_uninit(void)
{

	unsigned int common_open = 0;

	common_init_cnt--;

	hd_common_del_pid();
	hd_common_check_pid(0, &common_open, ask_vpd_hdal_status);
	if (common_open) {
		return HD_OK;
	}

	HD_COMMON_FLOW("hd_common_uninit:\n");
	HD_COMMON_FLOW("    count(%d) pid(%d)\n", common_init_cnt, (int)syscall(SYS_gettid));
	if (common_init_cnt > 0) {
		goto ext;
	}

	if (vpd_fd != -1) {
		close(vpd_fd);
		vpd_fd = -1;
	}
	if (nvtmem_fd != -1) {
		close(nvtmem_fd);
		nvtmem_fd = -1;
	}
	if (bd_dal_uninit() != HD_OK) {
		HD_COMMON_ERR("%s:%d\n", __FUNCTION__, __LINE__);
		goto err_ext;
	}
	pthread_mutex_destroy(&mmap_mutex);
	HD_COMMON_FLOW("hd_common_uninit\n");

#if 0//LIB_DEBUG_BUF_ALLOC
	{
		extern void hdal_show_alloc_statistic(void);
		hdal_show_alloc_statistic();
	}
#endif

ext:
	return HD_OK;

err_ext:
	return HD_ERR_NG;
}

static vpbuf_pool_type_t hd_type_to_vpd_type(HD_COMMON_MEM_POOL_TYPE type)
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
	default:
		if (type > 0) {
			vpd_type = type;
		} else {
			vpd_type = VPD_END_POOL;
		}
		break;
	}
	return vpd_type;
}

static HD_COMMON_MEM_POOL_TYPE vpd_type_to_hd_type(vpbuf_pool_type_t vpd_type)
{
	HD_COMMON_MEM_POOL_TYPE hd_type;

	switch (vpd_type) {
	case VPD_COMMON_POOL:
		hd_type = HD_COMMON_MEM_COMMON_POOL;
		break;
	case VPD_DISP0_IN_POOL:
		hd_type = HD_COMMON_MEM_DISP0_IN_POOL;
		break;
	case VPD_DISP1_IN_POOL:
		hd_type = HD_COMMON_MEM_DISP1_IN_POOL;
		break;
	case VPD_DISP2_IN_POOL:
		hd_type = HD_COMMON_MEM_DISP2_IN_POOL;
		break;
	case VPD_DISP3_IN_POOL:
		hd_type = HD_COMMON_MEM_DISP3_IN_POOL;
		break;
	case VPD_DISP4_IN_POOL:
		hd_type = HD_COMMON_MEM_DISP4_IN_POOL;
		break;
	case VPD_DISP5_IN_POOL:
		hd_type = HD_COMMON_MEM_DISP5_IN_POOL;
		break;
	case VPD_DISP0_CAP_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP0_CAP_OUT_POOL;
		break;
	case VPD_DISP1_CAP_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP1_CAP_OUT_POOL;
		break;
	case VPD_DISP2_CAP_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP2_CAP_OUT_POOL;
		break;
	case VPD_DISP3_CAP_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP3_CAP_OUT_POOL;
		break;
	case VPD_DISP4_CAP_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP4_CAP_OUT_POOL;
		break;
	case VPD_DISP5_CAP_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP5_CAP_OUT_POOL;
		break;
	case VPD_DISP0_ENC_SCL_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP0_ENC_SCL_OUT_POOL;
		break;
	case VPD_DISP1_ENC_SCL_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP1_ENC_SCL_OUT_POOL;
		break;
	case VPD_DISP2_ENC_SCL_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP2_ENC_SCL_OUT_POOL;
		break;
	case VPD_DISP3_ENC_SCL_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP3_ENC_SCL_OUT_POOL;
		break;
	case VPD_DISP4_ENC_SCL_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP4_ENC_SCL_OUT_POOL;
		break;
	case VPD_DISP5_ENC_SCL_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP5_ENC_SCL_OUT_POOL;
		break;
	case VPD_DISP0_ENC_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP0_ENC_OUT_POOL;
		break;
	case VPD_DISP1_ENC_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP1_ENC_OUT_POOL;
		break;
	case VPD_DISP2_ENC_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP2_ENC_OUT_POOL;
		break;
	case VPD_DISP3_ENC_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP3_ENC_OUT_POOL;
		break;
	case VPD_DISP4_ENC_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP4_ENC_OUT_POOL;
		break;
	case VPD_DISP5_ENC_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP5_ENC_OUT_POOL;
		break;
	case VPD_DISP_DEC_IN_POOL:
		hd_type = HD_COMMON_MEM_DISP_DEC_IN_POOL;
		break;
	case VPD_DISP_DEC_OUT_POOL:
		hd_type = HD_COMMON_MEM_DISP_DEC_OUT_POOL;
		break;
	case VPD_DISP_DEC_OUT_RATIO_POOL:
		hd_type = HD_COMMON_MEM_DISP_DEC_OUT_RATIO_POOL;
		break;
	case VPD_ENC_CAP_OUT_POOL:
		hd_type = HD_COMMON_MEM_ENC_CAP_OUT_POOL;
		break;
	case VPD_ENC_SCL_OUT_POOL:
		hd_type = HD_COMMON_MEM_ENC_SCL_OUT_POOL;
		break;
	case VPD_ENC_OUT_POOL:
		hd_type = HD_COMMON_MEM_ENC_OUT_POOL;
		break;
	case VPD_AU_ENC_AU_GRAB_OUT_POOL:
		hd_type = HD_COMMON_MEM_AU_ENC_AU_GRAB_OUT_POOL;
		break;
	case VPD_AU_DEC_AU_RENDER_IN_POOL:
		hd_type = HD_COMMON_MEM_AU_DEC_AU_RENDER_IN_POOL;
		break;
	case VPD_FLOW_MD_POOL:
		hd_type = HD_COMMON_MEM_FLOW_MD_POOL;
		break;
	case VPD_USER_BLK:
		hd_type = HD_COMMON_MEM_USER_BLK;
		break;
	default:
		hd_type = 0;
		break;
	}
	return hd_type;
}

/*parse sys/kernel/debug/memblock/memory memory range*/
HD_RESULT __hd_get_kernel_memory(memory_range_info_t *kernel_memory_info, unsigned int *nr_of_kernel_memory_info)
{
    unsigned int i = 0;
    char line[256];    
    FILE *fp;
    uintptr_t start_addr, end_addr;
    
    if (!kernel_memory_info || *nr_of_kernel_memory_info == 0) {
        return HD_ERR_NULL_PTR;
     }
    fp = fopen("/sys/kernel/debug/memblock/memory", "r");
    if (!fp) {
	printf("open /sys/kernel/debug/memblock/memory fail,errno(%d) strerror(%s)\n", errno, strerror(errno));
	return HD_ERR_FAIL;
    }
    while (fgets(line, sizeof(line), fp) != 0) {
        if (strstr(line, ":")) {
            char *token, *strptr;        
            
	    token = strstr(line, ":");
	    token++;
	    start_addr = strtoull(token, &strptr, 16);
             token = strstr(strptr, "..");
             token+= 2;
             end_addr = strtoull(token, NULL, 16);
             kernel_memory_info[i] .start_addr = start_addr;
             kernel_memory_info[i] .end_addr = end_addr;
             i++;
            if (i >= *nr_of_kernel_memory_info) {
            	HD_COMMON_ERR("memblock nr=%d >= nr_of_kernel_memory_info=%d\n" ,i, *nr_of_kernel_memory_info);
                fclose(fp);
                return HD_ERR_RESOURCE;
            }            
        }
    }
    fclose(fp);
    *nr_of_kernel_memory_info = i;
    return HD_OK;
}

HD_RESULT check_hdal_memory_with_kernel_memory(memory_range_info_t *kernel_memory_info, unsigned int nr_of_memory_info, 
    vpd_ddr_range_info_t hdal_ddr_info)
{
   unsigned int i;
   
   for (i = 0; i < nr_of_memory_info; i++) {
        if (hdal_ddr_info.start_addr >= kernel_memory_info[i].start_addr && hdal_ddr_info.start_addr <= kernel_memory_info[i].end_addr) { 
            return HD_ERR_NOT_ALLOW;
        } else if (hdal_ddr_info.end_addr >= kernel_memory_info[i].start_addr && hdal_ddr_info.end_addr <= kernel_memory_info[i].end_addr) {
            return HD_ERR_NOT_ALLOW;
        }
    }
   return HD_OK;
}

int chk_proc_exist(char *path_name) {
	if (access(path_name, F_OK)) {
		return 0;
	   } else {	
		return 1;	
	}
}
	
HD_RESULT hd_common_mem_init(HD_COMMON_MEM_INIT_CONFIG *p_mem_config)
{
	unsigned int i, j;
	unsigned long sys_hdal_size[DDR_ID_MAX] = {0};
	unsigned long total_size[DDR_ID_MAX] = {0};
	int ret;
	vpd_mem_init_t mem_init;
	struct nvtmem_hdal_base sys_hdal = {0};
	unsigned int vpd_num = 0;
	unsigned int ddr_id, size, ddr_nr;
	int enc_pool_fd = 0;
	int dec_ioctl_fd = 0;
	int vpe_ioctl_fd = 0;
	int sys_fd = 0;
	HD_RESULT hd_ret = HD_OK;
   	memory_range_info_t kernel_memory[MAX_KER_MBLK_NU];
    unsigned int nr_of_array = MAX_KER_MBLK_NU;
   	int start_ddr_no, end_ddr_no, ddrid;
	char total_chip_nr, chipid;
	
	if (pif_get_mem_init_state() > 0) {
		HD_COMMON_ERR("HDAL memory already init.\n");
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	sys_fd = open("/dev/nvtmem0", O_RDWR);
	if (sys_fd < 0) {
		HD_COMMON_ERR("Error: cannot open /dev/nvtmem0 device.\n");
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	if (ioctl(sys_fd, NVTMEM_GET_DTS_HDAL_BASE, &sys_hdal) < 0) {
		HD_COMMON_ERR(" ioctl: NVTMEM_GET_DTS_HDAL_BASE error\n");
		hd_ret = HD_ERR_DRV;
		goto err_ext;
	}
	memset(&mem_init, 0, sizeof(mem_init));    	
	for (ddr_nr = 0; ddr_nr < MAX_DDR_NR; ddr_nr++) {
		if (sys_hdal.size[ddr_nr] != 0) {
			ddr_id = sys_hdal.ddr_id[ddr_nr];
			sys_hdal_size[ddr_id] = sys_hdal.size[ddr_nr];
			mem_init.hdal_ddr_info[ddr_nr].ddr_id = ddr_nr;
			mem_init.hdal_ddr_info[ddr_nr].start_addr = sys_hdal.base[ddr_nr];
			mem_init.hdal_ddr_info[ddr_nr].ddr_size = sys_hdal.size[ddr_nr];
			mem_init.hdal_ddr_info[ddr_nr].end_addr = sys_hdal.base[ddr_nr] + sys_hdal.size[ddr_nr];		
		}
	}
	//*proc/hdal/flow disable nonchkmem and proc/hkvs not exist to check hdal & kernel memoy
	if ((hdal_flow_dbgmode & FLOW_NONCHKMEM_FLAG) == 0 && chk_proc_exist(KERNEL_SYS_DEBUG_PATH) == 1 
		&&  chk_proc_exist(HKVS_PATH) == 0) {
		if ( __hd_get_kernel_memory(kernel_memory, &nr_of_array) != HD_OK) {
	        		HD_COMMON_ERR("Error: __hd_get_kernel_memory fail\n.\n");
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		} else {
		    for (i = 0; i < nr_of_array; i++) {
		        HD_COMMON_FLOW("kernel sys memory %d range:%#lx~%#lx\n", i, kernel_memory[i].start_addr, kernel_memory[i].end_addr);
		    }
	    }
	   	 total_chip_nr = pif_get_chipnu();
		 for (chipid = 0; chipid < total_chip_nr; chipid++) {
		 	if ( chipid != COMMON_PCIE_CHIP_RC)
				continue;
			if (pif_get_ddr_id_by_chip(chipid, &start_ddr_no, &end_ddr_no) < 0)  {
				GMLIB_ERROR_P("pif_get_ddr_id_by_chip fail\n");        	
				continue;
			}	
			for (ddrid = start_ddr_no; ddrid < end_ddr_no; ddrid ++) {//check all hdal ddr memory with kernel
				if (check_hdal_memory_with_kernel_memory(kernel_memory, nr_of_array, mem_init.hdal_ddr_info[ddrid]) != HD_OK)  {
					HD_COMMON_ERR("check hdal memory :ddr%d start_addr=%#lx end_addr=%#lx overlap with kernel memory\n:", 
						ddrid, mem_init.hdal_ddr_info[ddrid].start_addr, mem_init.hdal_ddr_info[ddrid].end_addr);
					system("cat /sys/kernel/debug/memblock/memory");
					hd_ret = HD_ERR_SYS;        
					return hd_ret;
				}
			}
	     }
	} 
	enc_pool_fd = open("/dev/videoenc", O_RDWR);
	if (enc_pool_fd < 0) {
		HD_COMMON_ERR("enc dev not found\r\n");
		//return HD_ERR_NG;
	}

	dec_ioctl_fd = open("/dev/vdodec_ioctl", O_RDWR);
	if (dec_ioctl_fd < 0) {
		HD_COMMON_ERR("dec device(/dev/vdodec_ioctl) is not found\r\n");
		//return HD_ERR_NG;
	}

	vpe_ioctl_fd = open("/dev/kflow_vpe", O_RDWR);
	if (vpe_ioctl_fd < 0) {
		HD_COMMON_ERR("dec device(/dev/kflow_vpe) is not found\r\n");
		//return HD_ERR_NG;
	}

	memset(&hdl_mem_init, 0, sizeof(hdl_mem_init));
	memcpy(&hdl_mem_init, p_mem_config, sizeof(hdl_mem_init));
	for (i = 0; i < HD_COMMON_MEM_MAX_POOL_NUM; i++) {
		ddr_id = p_mem_config->pool_info[i].ddr_id;
		size = p_mem_config->pool_info[i].blk_size * p_mem_config->pool_info[i].blk_cnt;
		if (i >= VPD_MAX_POOL_NUM) {
			HD_COMMON_ERR("vpd pool limit %d\r\n", VPD_MAX_POOL_NUM);
			hd_ret = HD_ERR_LIMIT;
			goto err_ext;
		}
		if (size == 0) {
			continue;
		}
		if (ddr_id >= VPD_MAX_DDR_NUM) {
			HD_COMMON_ERR("vpd ddr support max %d\r\n", VPD_MAX_DDR_NUM);
			hd_ret = HD_ERR_PARAM;
			goto err_ext;
		}
		if (pif_get_chip_by_ddr_id(ddr_id) < 0) {
			HD_COMMON_ERR("type(%d): ddr_id(%d) is not exist\n", p_mem_config->pool_info[i].type, ddr_id);
			usleep(10000);
			hd_ret = HD_ERR_PARAM;
			goto err_ext;
		}
		total_size[ddr_id] += size;
		if (total_size[ddr_id] > sys_hdal_size[ddr_id]) {
			HD_COMMON_ERR("hdal memory DDR%d size %ldKB is over config %ldKB\n", ddr_id,
				      (unsigned long)(total_size[ddr_id] / 1024),
				      (unsigned long)(sys_hdal_size[ddr_id] / 1024));
			hd_ret = HD_ERR_LIMIT;
			goto err_ext;
		}
		if (vpd_num >= VPD_MAX_POOL_NUM) {
			HD_COMMON_ERR("vpd pool exceed %d\r\n", VPD_MAX_POOL_NUM);
			hd_ret = HD_ERR_NG;
			goto err_ext;
		}
		mem_init.pool_info[vpd_num].type = hd_type_to_vpd_type(p_mem_config->pool_info[i].type);
		mem_init.pool_info[vpd_num].hdal_type = p_mem_config->pool_info[i].type; //keep hdal library pool type for print message.

		if (HD_COMMON_MEM_ENC_REF_POOL == p_mem_config->pool_info[i].type) {
			if (enc_pool_fd >= 0) {
				H26XENC_REF_BUFFER_INFO   buf_info;
				HD_COMMON_MEM_POOL_INFO   *p_pool_info = &p_mem_config->pool_info[i];
				for (j = 0; j < p_mem_config->pool_info[i].blk_cnt; j++) {
					buf_info.buf_id = ++enc_ref_buf_id;
					buf_info.chip_id = pif_get_chip_by_ddr_id(p_pool_info->ddr_id);
					buf_info.addr_pa = p_pool_info->start_addr + p_pool_info->blk_size * j;
					buf_info.size = p_pool_info->blk_size;
					buf_info.ddr_no = p_pool_info->ddr_id;
					ret = ioctl(enc_pool_fd, H26X_ENC_IOC_SET_BUFFER_INFO, &buf_info);
					if (ret < 0) {
						HD_COMMON_ERR("Insert ref-buffer for encoder failed. now_count(%u)\r\n", j);
						hd_ret = HD_ERR_NG;
						goto exit;
					}
				}
			}
			continue;
		}

		if (HD_COMMON_MEM_DEC_TILE_POOL == p_mem_config->pool_info[i].type) {
			if (dec_ioctl_fd >= 0) {
				VDODEC_BUFFER_INFO buf_info;
				HD_COMMON_MEM_POOL_INFO *p_pool_info = &p_mem_config->pool_info[i];
				for (j = 0; j < p_mem_config->pool_info[i].blk_cnt; j++) {
					buf_info.chip_id = pif_get_chip_by_ddr_id(p_pool_info->ddr_id);
					if ((int)buf_info.chip_id < 0) {
						HD_COMMON_ERR("Error dec tile pool address 0x%x\r\n", (int)p_pool_info->start_addr);
						hd_ret =  HD_ERR_NG;
						goto exit;
					}
					buf_info.addr_pa = p_pool_info->start_addr + p_pool_info->blk_size * j;
					buf_info.size = p_pool_info->blk_size;
					buf_info.ddr_no = p_pool_info->ddr_id;
					ret = ioctl(dec_ioctl_fd, VDODEC_IOCTL_SET_H265D_CABAC_BUF, &buf_info);
					if (ret < 0) {
						HD_COMMON_ERR("Insert tile buffer for decoder failed. now_count(%u)\r\n", j);
						hd_ret = HD_ERR_NG;
						goto exit;
					}
				}
			}
			continue;
		}

#if 0  /* set buffer to driver by using vendor api */
		if (HD_COMMON_MEM_GLOBAL_MD_POOL == p_mem_config->pool_info[i].type) {

			HD_COMMON_MEM_POOL_INFO   *p_pool_info = &p_mem_config->pool_info[i];
			struct vcap_ioc_vg_fd_t fd_info;
			struct vcap_ioc_md_buf_t cap_md_buf;
			md_buf_info_t vpe_md_buf;

			for (j = 0; j < p_mem_config->pool_info[i].blk_cnt; j++) {

				/*
				 * for DVR_2MP_4CH:  4CH 1920x1080 HDMI(1080p)+CVBS
				 * MB_X_NUM_MAX = 256;
				 * MB_Y_NUM_MAX = 256
				 * MB_GAU_LEN = 256;
				 * EVT_LEN = 2;
				 * LV_LEN = 2;
				 * mb_size = 16;
				 * mb_x_num_max = max_width/mb_size = 1920/16 = 120;
				 * mb_y_num_max = max_height/mb_size = 1088/16 = 68;
				 * sta buf size = mb_x_num_max*mb_y_num_max*MB_GAU_LEN (bits)
				 *              = 120*68*256/8 (bytes)
				 * event buf size = MB_X_NUM_MAX*mb_y_num_max*EVT_LEN (bits)
				 *                = 256*68*2/8 (bytes)
				 * level buf size = MB_X_NUM_MAX*mb_y_num_max*LV_LEN (bits)
				 *                = 256*68*2/8 (bytes)
				 * */
				if (vcap_ioctl_fd && (j < 8)) {
					fd_info.version = VCAP_IOC_VERSION;
					fd_info.fd = ENTITY_FD(VPD_TYPE_CAPTURE, 0, j * 2, 0);
					ret = ioctl(vcap_ioctl_fd, VCAP_IOC_VG_FD_CONVERT, &fd_info);
					if (ret < 0) {
						HD_COMMON_ERR("ioctl \"VCAP_IOC_VG_FD_CONVERT\" fd=%x return %d\n", fd_info.fd, ret);
					}

					cap_md_buf.version = VCAP_IOC_VERSION;
					cap_md_buf.chip = fd_info.chip;
					cap_md_buf.vcap = fd_info.vcap;
					cap_md_buf.vi = fd_info.vi;
					cap_md_buf.ch = fd_info.ch;
					cap_md_buf.mb_x_num_max = 120;
					cap_md_buf.mb_y_num_max = 68;
					cap_md_buf.sta.ddr_id = p_pool_info->ddr_id;
					cap_md_buf.sta.paddr = p_pool_info->start_addr + p_pool_info->blk_size * j;
					cap_md_buf.sta.size = ALIGN_CEIL_16(120 * 68 * 256 / 8);
					cap_md_buf.event.ddr_id = p_pool_info->ddr_id;
					cap_md_buf.event.paddr = cap_md_buf.sta.paddr + cap_md_buf.sta.size;
					cap_md_buf.event.size =  ALIGN_CEIL_16(256 * 68 * 2 / 8);
					cap_md_buf.level.ddr_id = p_pool_info->ddr_id;
					cap_md_buf.level.paddr = cap_md_buf.event.paddr + cap_md_buf.event.size;
					cap_md_buf.level.size = ALIGN_CEIL_16(256 * 68 * 2 / 8);

					ret = ioctl(vcap_ioctl_fd, VCAP_IOC_MD_BUFFER_ASSIGN, &cap_md_buf); //notify fd invalid
					if (ret < 0) {
						HD_COMMON_ERR("ioctl \"VCAP_IOC_MD_BUFFER_ASSIGN\" return %d\n", ret);
					}
				}

				if (vpe_ioctl_fd) {
					vpe_md_buf.fd = ENTITY_FD(VPD_TYPE_VPE, 0, 0, j);
					vpe_md_buf.win_x_num_max = 120;
					vpe_md_buf.win_y_num_max = 68;
					vpe_md_buf.sta_buf.ddr_id = p_pool_info->ddr_id;
					vpe_md_buf.sta_buf.p_addr = p_pool_info->start_addr + p_pool_info->blk_size * j;
					vpe_md_buf.sta_buf.size =  ALIGN_CEIL_16(120 * 68 * 256 / 8);
					vpe_md_buf.evt_buf.ddr_id = p_pool_info->ddr_id;
					vpe_md_buf.evt_buf.p_addr = vpe_md_buf.sta_buf.p_addr + vpe_md_buf.sta_buf.size;
					vpe_md_buf.evt_buf.size = ALIGN_CEIL_16(256 * 68 * 2 / 8);

					ret = ioctl(vpe_ioctl_fd, VPE_IOC_SET_MD_BUF_INFO, &vpe_md_buf);
					if (ret < 0) {
						HD_COMMON_ERR("ioctl \"VPE_IOC_SET_MD_BUF_INFO\" return %d\n", ret);
					}
				}
			}
			continue;
		}
#endif
		if (VPD_END_POOL == mem_init.pool_info[i].type) {
			HD_COMMON_ERR("pool type (%u) not support!\r\n", p_mem_config->pool_info[i].type);
			continue;
		}

		if ((p_mem_config->pool_info[i].type == HD_COMMON_MEM_GLOBAL_MD_POOL) ||
		    (p_mem_config->pool_info[i].type == HD_COMMON_MEM_TMNR_MOTION_POOL) ||
		    (p_mem_config->pool_info[i].type == HD_COMMON_MEM_DSP_POOL) ||
		    (p_mem_config->pool_info[i].type == HD_COMMON_MEM_DISP0_FB_POOL) ||
		    (p_mem_config->pool_info[i].type == HD_COMMON_MEM_DISP1_FB_POOL) ||
		    (p_mem_config->pool_info[i].type == HD_COMMON_MEM_DISP2_FB_POOL) ||
		    (p_mem_config->pool_info[i].type == HD_COMMON_MEM_DISP3_FB_POOL) ||
		    (p_mem_config->pool_info[i].type == HD_COMMON_MEM_DISP4_FB_POOL) ||
		    (p_mem_config->pool_info[i].type == HD_COMMON_MEM_DISP5_FB_POOL)) {//not create ms pool
			mem_init.pool_info[vpd_num].add_to_ms = 0;
		} else {
			mem_init.pool_info[vpd_num].add_to_ms = 1;
		}
		mem_init.pool_info[vpd_num].size = size;
		mem_init.pool_info[vpd_num].ddr_id = ddr_id;
		mem_init.pool_info[vpd_num].start_addr = p_mem_config->pool_info[i].start_addr;
		mem_init.pool_info[vpd_num].blk_size = p_mem_config->pool_info[i].blk_size;
		mem_init.pool_info[vpd_num].blk_cnt = p_mem_config->pool_info[i].blk_cnt;
		if (!CHECK_ALIGN_256(mem_init.pool_info[vpd_num].start_addr) || !CHECK_ALIGN_256(size)) {
			HD_COMMON_ERR("pool address and size should be 256 aligned!r\n");
			hd_ret = HD_ERR_NOT_AVAIL;
			goto exit;
		}
		for (j = 0; j < VPD_SHARED_POOL_NUM; j++) {
			mem_init.pool_info[vpd_num].shared_pool[j] = hd_type_to_vpd_type(GET_SHAREPOOL_ID(p_mem_config->pool_info[i].shared_pool[0], j));
		}
		vpd_num++;
	}

exit:
	if (0 != pif_set_mem_init(&mem_init)) {
		HD_COMMON_ERR("pif_set_mem_init fail.\n");
		hd_ret = HD_ERR_NG;
	}

err_ext:
	if (vpe_ioctl_fd >= 0) {
		close(vpe_ioctl_fd);
	}
	if (enc_pool_fd >= 0) {
		close(enc_pool_fd);
	}
	if (dec_ioctl_fd >= 0) {
		close(dec_ioctl_fd);
	}
	if (sys_fd >= 0) {
		close(sys_fd);
	}

	return hd_ret;
}

HD_RESULT hd_common_mem_get(HD_COMMON_MEM_PARAM_ID id, VOID *p_param)
{
	HD_RESULT ret = HD_OK;

	if (p_param == NULL) {
		GMLIB_ERROR_P("NULL p_param\n");
		ret = HD_ERR_NG;
		goto exit;
	}

	switch (id) {
	case HD_COMMON_MEM_PARAM_INIT_CONFIG:
		memcpy((VOID *)p_param, (VOID *)&hdl_mem_init, sizeof(hdl_mem_init));
		break;
	case HD_COMMON_MEM_PARAM_POOL_CONFIG: {
		HD_COMMON_MEM_POOL_INFO *pinfo = (HD_COMMON_MEM_POOL_INFO *)p_param;
		vpd_pool_size_info_t pool_size;
		pool_size.ddr_id = pinfo->ddr_id;
		pool_size.type = hd_type_to_vpd_type(pinfo->type);

		if (pif_get_pool((void *)&pool_size) != 0) {
			ret = HD_ERR_NG;
		} else {
			pinfo->blk_cnt = pool_size.blk_cnt;
			pinfo->blk_size = pool_size.blk_size;
			pinfo->start_addr = pool_size.start_addr;
			pinfo->shared_pool[0] = vpd_type_to_hd_type(pool_size.shared_pool[0]);
			ret = HD_OK;
		}
		break;
	}
	case HD_COMMON_MEM_PARAM_VIRT_INFO: {
		if (pif_get_usr_va_info((HD_COMMON_MEM_VIRT_INFO *)p_param) != 0) {
			ret = HD_ERR_NG;
		}
		break;
	}
	default:
		HD_COMMON_ERR("Unsupport mem param id(%d)\n", id);
		ret = HD_ERR_PARAM;
		break;
	}

exit:
	return ret;
}

HD_RESULT hd_common_mem_uninit(void)
{
	if (0 != pif_set_mem_uninit()) {
		return HD_ERR_NG;
	} else {
		return HD_OK;
	}
}
#if USE_NVTMEM_MMAP
void *hd_mmap_pa_to_va(HD_COMMON_MEM_MEM_TYPE mem_type, UINTPTR pa, unsigned int size, struct map_info *va_map_info)
{
	unsigned int page_size;
	unsigned long map_size;
	UINTPTR aligned_pa;
	void *mmap_va;
	alloc_type_t alloc_type;
	int ret = 0;

	if (pa == 0 || size == 0) {
		HD_COMMON_ERR("Invalid param pa(%#lx) size(%d)\n", (ULONG)pa, size);
		return NULL;
	}
	page_size = getpagesize();
	aligned_pa = (pa / page_size) * page_size;
	map_size = (size + pa - aligned_pa);

	if (mem_type == HD_COMMON_MEM_MEM_TYPE_NONCACHE) {
		alloc_type  = ALLOC_BUFFERABLE | ALLOC_MAP_ONLY;
	} else {
		alloc_type  = ALLOC_CACHEABLE | ALLOC_MAP_ONLY;
	}
	if (nvtmem_fd < 0) {
		if ((nvtmem_fd = open("/dev/nvtmem0", O_RDWR)) < 0) {
			HD_COMMON_ERR("open /dev/nvtmem0 failed.\n");
			return NULL;
		}
	}
	ret = ioctl(nvtmem_fd, NVTMEM_ALLOC_TYPE, &alloc_type);
	if (ret == -1) {
		printf("IOCTL NVTMEM_ALLOC_TYPE fail\n");
		return NULL;
	}
	mmap_va = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, nvtmem_fd, (unsigned long)aligned_pa);
	if (mmap_va == MAP_FAILED || mmap_va == 0) {
		HD_COMMON_ERR("mmap pa to va fail, errno(%d): %s.\n", errno, strerror(errno));
		return NULL;
	}
	if (va_map_info) {
		va_map_info->map_size = map_size;
		va_map_info->va = mmap_va;
		va_map_info->pa = pa;
		va_map_info->user_va = (mmap_va + (pa - aligned_pa));
		return va_map_info->user_va;
	} else {
		munmap(mmap_va, map_size);
		return NULL;
	}
}
#else
void *hd_mmap_pa_to_va(HD_COMMON_MEM_MEM_TYPE mem_type, UINTPTR pa, unsigned int size, struct map_info *va_map_info)
{
	unsigned int page_size;
	unsigned long map_size;
	UINTPTR aligned_pa;
	void *mmap_va;

	if (pa == 0 || size == 0) {
		HD_COMMON_ERR("Invalid param pa(%#lx) size(%d)\n", (ULONG)pa, size);
		return NULL;
	}
	page_size = getpagesize();
	aligned_pa = (pa / page_size) * page_size;
	map_size = (size + pa - aligned_pa);

	if (mem_type == HD_COMMON_MEM_MEM_TYPE_NONCACHE) {
		aligned_pa = aligned_pa | NONCACHE_FLAG;
	}
	if (vpd_fd <= 0) {
		if ((vpd_fd = open("/dev/vpd", O_RDWR | O_SYNC)) < 0) {
			HD_COMMON_ERR("open /dev/vpd failed.\n");
			return NULL;
		}
	}
	mmap_va = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, vpd_fd, (unsigned long)aligned_pa);
	if (mem_type == HD_COMMON_MEM_MEM_TYPE_NONCACHE) {
		aligned_pa = aligned_pa & (~NONCACHE_FLAG);
	}
	if (mmap_va == MAP_FAILED || mmap_va == 0) {
		HD_COMMON_ERR("mmap pa to va fail, errno(%d): %s.\n", errno, strerror(errno));
		return NULL;
	}
	if (va_map_info) {
		va_map_info->map_size = map_size;
		va_map_info->va = mmap_va;
		va_map_info->pa = pa;
		va_map_info->user_va = (mmap_va + (pa - aligned_pa));
		return va_map_info->user_va;
	} else {
		munmap(mmap_va, map_size);
		return NULL;
	}
}
#endif
INT get_pool_name_by_type(HD_COMMON_MEM_POOL_TYPE type, CHAR *p_out_name, UINT32 out_size, HD_COMMON_MEM_DDR_ID ddr_id)
{
	CHAR *p_name = NULL;
	CHAR pool_name[32];
	if (HD_COMMON_MEM_COMMON_POOL == type) {
		p_out_name[0] = '\0'; //to get from free pool with empty string
		return 0;
	}

	switch (type) {
	case HD_COMMON_MEM_DISP0_IN_POOL:
		p_name = "disp0_in";
		break;
	case HD_COMMON_MEM_DISP1_IN_POOL:
		p_name = "disp1_in";
		break;
	case HD_COMMON_MEM_DISP2_IN_POOL:
		p_name = "disp2_in";
		break;
	case HD_COMMON_MEM_DISP3_IN_POOL:
		p_name = "disp3_in";
		break;
	case HD_COMMON_MEM_DISP4_IN_POOL:
		p_name = "disp4_in";
		break;
	case HD_COMMON_MEM_DISP5_IN_POOL:
		p_name = "disp5_in";
		break;
	case HD_COMMON_MEM_DISP0_CAP_OUT_POOL:
		p_name = "disp0_cap_out";
		break;
	case HD_COMMON_MEM_DISP1_CAP_OUT_POOL:
		p_name = "disp1_cap_out";
		break;
	case HD_COMMON_MEM_DISP2_CAP_OUT_POOL:
		p_name = "disp2_cap_out";
		break;
	case HD_COMMON_MEM_DISP3_CAP_OUT_POOL:
		p_name = "disp3_cap_out";
		break;
	case HD_COMMON_MEM_DISP4_CAP_OUT_POOL:
		p_name = "disp4_cap_out";
		break;
	case HD_COMMON_MEM_DISP5_CAP_OUT_POOL:
		p_name = "disp5_cap_out";
		break;
	case HD_COMMON_MEM_DISP0_ENC_SCL_OUT_POOL:
		p_name = "disp0_enc_scl_out";
		break;
	case HD_COMMON_MEM_DISP1_ENC_SCL_OUT_POOL:
		p_name = "disp1_enc_scl_out";
		break;
	case HD_COMMON_MEM_DISP2_ENC_SCL_OUT_POOL:
		p_name = "disp2_enc_scl_out";
		break;
	case HD_COMMON_MEM_DISP3_ENC_SCL_OUT_POOL:
		p_name = "disp3_enc_scl_out";
		break;
	case HD_COMMON_MEM_DISP4_ENC_SCL_OUT_POOL:
		p_name = "disp4_enc_scl_out";
		break;
	case HD_COMMON_MEM_DISP5_ENC_SCL_OUT_POOL:
		p_name = "disp5_enc_scl_out";
		break;
	case HD_COMMON_MEM_DISP0_ENC_OUT_POOL:
		p_name = "disp0_enc_out";
		break;
	case HD_COMMON_MEM_DISP1_ENC_OUT_POOL:
		p_name = "disp1_enc_out";
		break;
	case HD_COMMON_MEM_DISP2_ENC_OUT_POOL:
		p_name = "disp2_enc_out";
		break;
	case HD_COMMON_MEM_DISP3_ENC_OUT_POOL:
		p_name = "disp3_enc_out";
		break;
	case HD_COMMON_MEM_DISP4_ENC_OUT_POOL:
		p_name = "disp4_enc_out";
		break;
	case HD_COMMON_MEM_DISP5_ENC_OUT_POOL:
		p_name = "disp5_enc_out";
		break;
	case HD_COMMON_MEM_DISP_DEC_IN_POOL:
		p_name = "disp_dec_in";
		break;
	case HD_COMMON_MEM_DISP_DEC_OUT_POOL:
		p_name = "disp_dec_out";
		break;
	case HD_COMMON_MEM_DISP_DEC_OUT_RATIO_POOL:
		p_name = "disp_dec_out_ratio";
		break;
	case HD_COMMON_MEM_ENC_CAP_OUT_POOL:
		p_name = "enc_cap_out";
		break;
	case HD_COMMON_MEM_ENC_SCL_OUT_POOL:
		p_name = "enc_scl_out";
		break;
	case HD_COMMON_MEM_ENC_OUT_POOL:
		p_name = "enc_out";
		break;
	case HD_COMMON_MEM_AU_ENC_AU_GRAB_OUT_POOL:
		p_name = "au_enc_au_grab_out";
		break;
	case HD_COMMON_MEM_AU_DEC_AU_RENDER_IN_POOL:
		p_name = "au_dec_au_render_in";
		break;
	case HD_COMMON_MEM_FLOW_MD_POOL:
		p_name = "flow_md";
		break;
	case HD_COMMON_MEM_USER_BLK:
		p_name = "user_blk";
		break;
	case HD_COMMON_MEM_OSG_POOL:
		p_name = "osg";
		break;
	case HD_COMMON_MEM_CNN_POOL:
		p_name = "cnn";
		break;
	default:
		if (type < HD_COMMON_MEM_MAX_POOL_NUM) {
			p_name = pool_name;
			sprintf(p_name, "Pool_type_%d", type);
		} else {
			HD_COMMON_ERR("Not support pool type(%d), get blk\n", type);
			return -1;
		}
	}

	snprintf(p_out_name, out_size, "%s_ddr%d", p_name, ddr_id);

	return 0;
}

HD_COMMON_MEM_VB_BLK hd_common_mem_get_block(HD_COMMON_MEM_POOL_TYPE pool_type, UINT32 blk_size, HD_COMMON_MEM_DDR_ID ddr)
{
	INT ddr_id;
	CHAR pool_name[32];
	HD_COMMON_MEM_VB_BLK blk = HD_COMMON_MEM_VB_INVALID_BLK;

	if (blk_size == 0 || ddr >= DDR_ID_MAX) {
		HD_COMMON_ERR("%s, get blk_size(%d),ddr(%d) failed.\n", __func__, blk_size, ddr);
		return blk;
	}

	ddr_id = ddr;

	if (get_pool_name_by_type(pool_type, pool_name, sizeof(pool_name), ddr) < 0) {
		HD_COMMON_ERR("%s, get blk_size(%d),ddr(%d) failed.\n", __func__, blk_size, ddr);
		return HD_COMMON_MEM_VB_INVALID_BLK;
	}

	blk = (HD_COMMON_MEM_VB_BLK)pif_alloc_video_buffer_for_hdal(blk_size, ddr_id, pool_name, USR_LIB);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		HD_COMMON_ERR("%s, get blk_size(%d),ddr(%d) failed.\n", __func__, blk_size, ddr);
		return blk;
	}
	if (blk & 0x000000FF) {
		HD_COMMON_ERR("blk not align to 256 ,blk=%p\n", (VOID *) blk);
	} else {
		blk = MAKEBLK(blk, ddr_id);
	}
	HD_COMMON_FLOW("hd_common_mem_get_block:\n");
	HD_COMMON_FLOW("    pool_type(0x%x) blk_size(%d) ddrid(%d) pa(%p)\n", pool_type, blk_size, \
		       ddr, (VOID *)blk);
	return blk;
}

HD_COMMON_MEM_VB_BLK __hd_common_mem_get_block_with_name(HD_COMMON_MEM_POOL_TYPE pool_type, UINT32 blk_size, HD_COMMON_MEM_DDR_ID ddr, 
	CHAR *usr_name)
{
	INT ddr_id;
	CHAR pool_name[32];
	HD_COMMON_MEM_VB_BLK blk = HD_COMMON_MEM_VB_INVALID_BLK;

	if (blk_size == 0 || ddr >= DDR_ID_MAX) {
		HD_COMMON_ERR("%s, get blk_size(%d),ddr(%d) failed.\n", __func__, blk_size, ddr);
		return blk;
	}

	ddr_id = ddr;

	if (get_pool_name_by_type(pool_type, pool_name, sizeof(pool_name), ddr) < 0) {
		HD_COMMON_ERR("%s, get blk_size(%d),ddr(%d) failed.\n", __func__, blk_size, ddr);
		return HD_COMMON_MEM_VB_INVALID_BLK;
	}

	blk = (HD_COMMON_MEM_VB_BLK)pif_alloc_video_buffer_for_hdal_with_name(blk_size, ddr_id, pool_name, USR_LIB, usr_name);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		HD_COMMON_ERR("%s, get blk_size(%d),ddr(%d) failed.\n", __func__, blk_size, ddr);
		return blk;
	}
	if (blk & 0x000000FF) {
		HD_COMMON_ERR("blk not align to 256 ,blk=%p\n", (VOID *) blk);
	} else {
		blk = MAKEBLK(blk, ddr_id);
	}
	HD_COMMON_FLOW("__hd_common_mem_get_block:\n");
	HD_COMMON_FLOW("    pool_type(0x%x) blk_size(%d) ddrid(%d) pa(%p) name(%s)\n", pool_type, blk_size, \
		       ddr, (VOID *)blk, usr_name);
	return blk;
}


UINTPTR hd_common_mem_blk2pa(HD_COMMON_MEM_VB_BLK blk)
{
	HD_COMMON_FLOW("hd_common_mem_blk2pa:\n");
	HD_COMMON_FLOW("    blk(%p) pa(%p)\n", (VOID *)blk, (VOID *)GET_PA(blk));

	return (UINTPTR)GET_PA(blk);
}

HD_RESULT hd_common_mem_release_block(HD_COMMON_MEM_VB_BLK blk)
{
	HD_COMMON_MEM_DDR_ID ddr;
	INT ret;
	ddr = GET_DDRID(blk);
	blk = GET_PA(blk);

	HD_COMMON_FLOW("hd_common_mem_release_block:\n");
	HD_COMMON_FLOW("   pa(%p) ddr(%d)\n", (VOID *)blk, ddr);
	ret = pif_free_video_buffer_for_hdal((void *)blk, ddr, USR_LIB);
	if (ret < 0) {
		HD_COMMON_ERR("%s, free blk(%p) failed.\n", __func__, (VOID *) blk);
		return HD_ERR_NG;
	}
	return HD_OK;
}

void *hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE mem_type, UINTPTR phy_addr, UINT32 size)
{
	void *mmap_dest = NULL;
	struct map_info *mmap;
	INT i = 0, j;
	
	if (pif_address_sanity_check(phy_addr) < 0) {
		HD_COMMON_ERR("%s:Check mmap pa(%#lx) error\n", __func__, (ULONG)phy_addr);
		return mmap_dest;
	}

	pthread_mutex_lock(&mmap_mutex);
	if (mem_type >= HD_COMMON_MEM_MEM_TYPE_MAX) {
		HD_COMMON_ERR("mmap mem_type error\n");
		goto mmap_exit;
	}
	while (i < HD_COMMON_MEM_MAX_MMAP_NUM && mmap_tbl[i].va) {
		i++;
	}
	if (i >= HD_COMMON_MEM_MAX_MMAP_NUM) {
		HD_COMMON_ERR("mmap tbl run out:\n");
		for (j = 0; j < HD_COMMON_MEM_MAX_MMAP_NUM; j++) {
			HD_COMMON_ERR("    %d: pa(%#lx) va(%#lx) user_va(%#lx) map_size(%d)\n", j,
							(ULONG)mmap_tbl[j].pa, (ULONG)mmap_tbl[j].va, (ULONG)mmap_tbl[j].user_va,
							mmap_tbl[j].map_size);
		}
		goto mmap_exit;
	}
	mmap = &mmap_tbl[i];
	mmap_dest = hd_mmap_pa_to_va(mem_type, phy_addr, size, mmap);
	HD_COMMON_FLOW("hd_common_mem_mmap:\n");
	HD_COMMON_FLOW("    idx(%d) pa(%p) va(%p) %s\n", i, (VOID *)phy_addr, mmap_dest, mem_type_str[mem_type]);
mmap_exit:
	pthread_mutex_unlock(&mmap_mutex);
	return mmap_dest;
}

HD_RESULT hd_common_mem_munmap(void *virt_addr, unsigned int size)
{
	INT i = 0;
	HD_RESULT ret = HD_OK;

	pthread_mutex_lock(&mmap_mutex);
	while (i < HD_COMMON_MEM_MAX_MMAP_NUM && mmap_tbl[i].user_va != virt_addr) {
		i ++;
	}
	if (i >= HD_COMMON_MEM_MAX_MMAP_NUM) {
		HD_COMMON_ERR("Not find buf(%p) in mmap table\n", virt_addr);
		ret = HD_ERR_NG;
		goto munmap_exit;
	}
	munmap(mmap_tbl[i].va, mmap_tbl[i].map_size);
	mmap_tbl[i].va = 0;

munmap_exit:
	HD_COMMON_FLOW("hd_common_mem_munmap:\n");
	HD_COMMON_FLOW("    idx(%d) va(%p)\n", i, virt_addr);
	pthread_mutex_unlock(&mmap_mutex);
	return ret;
}

typedef struct {
	unsigned int size;
	unsigned int nr;
	unsigned int range;
} HD_COMMON_MEM_LAYOUT_ATTR;

typedef struct {
	int type;
	int chk_in_buf;
	int chk_out_buf;
} HD_COMMON_CHK_TRI_BUF;
//TODO!@Add vendor api
/*hd_common_pool_layout need stop module and free pool buf to layout
 *layout_attr:layout_attr.size need in order and from small to large
*/
HD_RESULT hd_common_pool_layout(HD_COMMON_MEM_POOL_TYPE pool_type, int attr_nr, HD_COMMON_MEM_LAYOUT_ATTR *layout_attr, int del_layout)
{
	char pool_name[MAX_POOL_NAME_LEN + 1];
	int i;
	unsigned int pre_size = 0;

	HD_COMMON_FLOW("hd_common_pool_layout:\n");
	HD_COMMON_FLOW("   pool_type(%#x) attr_nr(%d)\n", pool_type, attr_nr);
	for (i = 0; i < attr_nr; i++) {
		if (!layout_attr[i].nr || !layout_attr[i].size) {
			HD_COMMON_ERR("Check layout_attr[%d].range=%d,layout_attr[%d].size=%d fail\n", i, layout_attr[i].range,
				      i, layout_attr[i].size);
			return HD_ERR_NG;
		}
		if (layout_attr[i].range > layout_attr[i].size / 3) {
			HD_COMMON_ERR("layout_attr[%d].range(%d) > layout_attr[%d].size/3(%d)\n", i, layout_attr[i].range, i, layout_attr[i].size / 3);
			return HD_ERR_NG;
		}
		if (pre_size >= layout_attr[i].size) {
			HD_COMMON_ERR("layout_attr[%d].size(%d) >= layout_attr[%d].size(%d)\n", i - 1, pre_size,
				      i, layout_attr[i].size);
			return HD_ERR_NG;
		}
		pre_size = layout_attr[i].size;
		HD_COMMON_FLOW("   layout_attr[%d]:size(%d) range(%d) nr(%d)\n", i, layout_attr[i].size, layout_attr[i].range, layout_attr[i].nr);
	}
	if (get_pool_name_by_type(pool_type, pool_name, sizeof(pool_name), 0) < 0) {
		HD_COMMON_ERR("get_pool_name_by_type,pool_type(%#x) failed.\n", pool_type);
		return HD_ERR_NG;
	}

	if (pif_layout_fixepool_for_hdal(attr_nr, layout_attr, pool_name, del_layout) < 0) {
		HD_COMMON_ERR("pif_layout_fixepool_for_hdal failed.\n");
		return HD_ERR_NG;
	}
	return HD_OK;
}


//TODO!@Add vendor api
/*hd_common_get_pool_used_buf to pool in-used buf info
 *used_buf_nr:-1 to get in-used buf nr, > 0 to get in-used buf pa and size
*/
HD_RESULT hd_common_get_pool_usedbuf_info(HD_COMMON_MEM_POOL_TYPE pool_type, int *p_used_buf_nr, HD_BUFINFO *buf_info)
{
	char pool_name[MAX_POOL_NAME_LEN + 1];
	int i;

	if (!p_used_buf_nr) {
		HD_COMMON_ERR("Check p_used_buf_nr(%p) fail\n", p_used_buf_nr);
		return HD_ERR_NG;
	}
	if ((*p_used_buf_nr > 0) && !buf_info) {
		HD_COMMON_ERR("Check buf_info(%p) fail\n", buf_info);
		return HD_ERR_NG;
	}
	HD_COMMON_FLOW("hd_common_pool_layout:\n");
	HD_COMMON_FLOW("   pool_type(%#x) used_buf_nr(%d)\n", pool_type, *p_used_buf_nr);
	if (get_pool_name_by_type(pool_type, pool_name, sizeof(pool_name), 0) < 0) {
		HD_COMMON_ERR("get_pool_name_by_type,pool_type(%#x) failed.\n", pool_type);
		return HD_ERR_NG;
	}
	if (pif_get_pool_usedbuf_info(p_used_buf_nr, buf_info, pool_name) < 0) {
		HD_COMMON_ERR("pif_layout_fixepool_for_hdal failed.\n");
		return HD_ERR_NG;
	}
	if (*p_used_buf_nr && buf_info) {
		for (i = 0; i < *p_used_buf_nr; i++) {
			HD_COMMON_FLOW("   buf[%d]:phy_addr(%p) buf_size(%d)\n", i, (VOID *)buf_info[i].phy_addr, buf_info[i].buf_size);
		}
	}
	return HD_OK;
}
#if USE_NVTMEM_CACHE_FLUSH
HD_RESULT hd_common_mem_cache_sync(void *virt_addr, unsigned int size, HD_COMMON_MEM_DMA_DIR dir)
{
	int ret;
	struct nvtmem_vaddr_info saddr;

	if (nvtmem_fd < 0) {
		nvtmem_fd = open("/dev/nvtmem0", O_RDWR);
		if (nvtmem_fd < 0) {
			return HD_ERR_SYS;
		}
	}
	if (virt_addr == NULL || size == 0) {
		HD_COMMON_ERR("hd_common_mem_flush_cache parameters err, virt_addr(%p) size(%d)\n", virt_addr, size);
		return HD_ERR_SYS;
	}
	saddr.vaddr = (uintptr_t)virt_addr;
	saddr.length = size;	
	if (dir == HD_COMMON_MEM_DMA_BIDIRECTIONAL) {
		saddr.dir = DMA_DIR_BIDIRECTIONAL;  		
	} else if (dir == HD_COMMON_MEM_DMA_TO_DEVICE) {
		saddr.dir = DMA_DIR_TO_DEVICE;
	} else if (dir == HD_COMMON_MEM_DMA_FROM_DEVICE) {
		saddr.dir = DMA_DIR_FROM_DEVICE;  	
	} else {
		HD_COMMON_ERR("_hd_common_mem_cache_sync parameters err, dir(%#x)\n", dir);
		return HD_ERR_SYS;
	}
	HD_COMMON_FLOW("hd_common_mem_flush_cache:\n");
	HD_COMMON_FLOW("    virt_addr(%#lx) size(%ld) dir(%#x)\n", (ULONG) saddr.vaddr, saddr.length, saddr.dir);
	ret = ioctl(nvtmem_fd, NVTMEM_MEMORY_SYNC, &saddr);
	if (ret < 0) {
		printf("IOCTL NVTMEM_MEMORY_SYNC fail func=%s, line=%d \r\n", __FUNCTION__, __LINE__);
		return HD_ERR_SYS;
	}	
	return HD_OK;
}
#else
HD_RESULT hd_common_mem_cache_sync(void *virt_addr, unsigned int size, HD_COMMON_MEM_DMA_DIR dir)
{
	HD_RESULT ret;
	vpd_flush_data_t vpd_flush;

	if (vpd_fd <= 0) {
		return HD_ERR_SYS;
	}
	if (virt_addr == NULL || size == 0) {
		HD_COMMON_ERR("hd_common_mem_cache_sync parameters err, virt_addr(%p) size(%d)\n", virt_addr, size);
		return HD_ERR_SYS;
	}
	if (dir == HD_COMMON_MEM_DMA_BIDIRECTIONAL) {
		vpd_flush.dir = VPD_FLUSH_DIR_BIDIRECTIONAL;
	} else if (dir == HD_COMMON_MEM_DMA_TO_DEVICE) {
		vpd_flush.dir = VPD_FLUSH_DIR_DMA_TO_DEVICE;
	} else if (dir == HD_COMMON_MEM_DMA_FROM_DEVICE) {
		vpd_flush.dir = VPD_FLUSH_DIR_DMA_FROM_DEVICE;
	} else {
		HD_COMMON_ERR("_hd_common_mem_cache_sync parameters err, dir(%#x)\n", dir);
		return HD_ERR_SYS;
	}
	vpd_flush.addr_va = (uintptr_t)virt_addr;
	vpd_flush.size = size;
	HD_COMMON_FLOW("_hd_common_mem_cache_sync:\n");
	HD_COMMON_FLOW("    virt_addr(%p) size(%d) dir(%#x)\n", (void *)vpd_flush.addr_va, vpd_flush.size, vpd_flush.dir);
	if (vpd_fd == 0) {
		vpd_fd = open("/dev/vpd", O_RDWR);
	}
	ret = ioctl(vpd_fd, VPD_FLUSH, &vpd_flush);
	if (ret < 0) {
		return HD_ERR_SYS;
	}
	return HD_OK;
}
#endif
HD_RESULT hd_common_clear_all_blk(void)
{
	HD_RESULT ret = HD_OK;

	HD_COMMON_FLOW("hd_common_clear_all_blk:\n");
	if (pif_clear_usr_blk() < 0) {
	}
	if (ret < 0) {
		ret = HD_ERR_SYS;
	}
	return ret;
}

HD_RESULT hd_common_clear_pool_blk(HD_COMMON_MEM_POOL_TYPE pool_type, INT ddrid)
{
	HD_RESULT ret = HD_OK;
	char pool_name[MAX_POOL_NAME_LEN + 1];

	if (ddrid >= DDR_ID_MAX) {
		HD_COMMON_ERR("Check ddrid(%d) fail\n", ddrid);
		return HD_ERR_RESOURCE;
	}
	if (get_pool_name_by_type(pool_type, pool_name, sizeof(pool_name), ddrid) < 0) {
		HD_COMMON_ERR("get_pool_name_by_type,pool_type(%#x) ddrid(%d) failed.\n", pool_type, ddrid);
		return HD_ERR_NG;
	}
	HD_COMMON_FLOW("hd_common_clear_pool_blk:\n");
	if (pif_clear_usr_pool_blk(pool_name, ddrid) < 0) {
		ret = HD_ERR_SYS;
	}
	return ret;
}

HD_RESULT hd_common_get_pool_size(HD_COMMON_MEM_POOL_TYPE pool_type, UINT32 ddrid, UINT32 *pool_sz)
{
	HD_RESULT ret = HD_OK;
	char pool_name[MAX_POOL_NAME_LEN + 1];

	*pool_sz = 0;
	if (ddrid >= DDR_ID_MAX) {
		HD_COMMON_ERR("Check ddrid(%d) fail\n", ddrid);
		return HD_ERR_RESOURCE;
	}
	if (get_pool_name_by_type(pool_type, pool_name, sizeof(pool_name), ddrid) < 0) {
		HD_COMMON_ERR("get_pool_name_by_type,pool_type(%#x) failed.\n", pool_type);
		return HD_ERR_NG;
	}
	HD_COMMON_FLOW("hd_common_get_pool_size:\n");
	*pool_sz = pif_get_pool_size(pool_name, ddrid);
	return ret;
}

HD_RESULT hd_common_set_chk_trigger_buf(HD_COMMON_CHK_TRI_BUF param)
{   
       int ret;
       vpd_chk_trigger_buf_t set_chk_tri_buf;

       set_chk_tri_buf.type = param.type;
       set_chk_tri_buf.chk_in_buf = param.chk_in_buf;
       set_chk_tri_buf.chk_out_buf = param.chk_out_buf;
       ret = pif_set_check_trigger_buf(set_chk_tri_buf);
       
       if (ret < 0) {
            return HD_ERR_NG;
       }
       return HD_OK;
}

HD_RESULT hd_common_get_chk_trigger_buf(HD_COMMON_CHK_TRI_BUF *p_param)
{   
       int ret;
       vpd_chk_trigger_buf_t get_chk_tri_buf;

       get_chk_tri_buf.type = p_param->type;
       ret = pif_get_check_trigger_buf(&get_chk_tri_buf);
       
       if (ret < 0) {
            return HD_ERR_NG;
       }
       p_param->chk_in_buf =  get_chk_tri_buf.chk_in_buf;
       p_param->chk_out_buf =  get_chk_tri_buf.chk_out_buf;
       return HD_OK;
}

HD_RESULT _hd_common_mem_cache_sync_bidirectional(void *virt_addr, unsigned int size)
{
	return hd_common_mem_cache_sync(virt_addr, size, HD_COMMON_MEM_DMA_BIDIRECTIONAL);
}
HD_RESULT _hd_common_mem_cache_sync_dma_to_device(void *virt_addr, unsigned int size)
{
	return hd_common_mem_cache_sync(virt_addr, size, HD_COMMON_MEM_DMA_TO_DEVICE);
}
HD_RESULT _hd_common_mem_cache_sync_dma_from_device(void *virt_addr, unsigned int size)
{
	return hd_common_mem_cache_sync(virt_addr, size, HD_COMMON_MEM_DMA_FROM_DEVICE);
}

HD_RESULT _hd_common_mem_clear_all_blk(void)
{
	return hd_common_clear_all_blk();
}

HD_RESULT _hd_common_clear_pool_blk(HD_COMMON_MEM_POOL_TYPE pool_type, INT ddrid)
{
	return hd_common_clear_pool_blk(pool_type, ddrid);
}
HD_RESULT _hd_common_get_pool_size(HD_COMMON_MEM_POOL_TYPE pool_type, INT ddrid, UINT32 *pool_sz)
{
	return hd_common_get_pool_size(pool_type, ddrid, pool_sz);
}
HD_RESULT _hd_common_mem_get(VOID *p_param)
{
	return hd_common_mem_get(HD_COMMON_MEM_PARAM_POOL_CONFIG, p_param);
}
HD_RESULT _hd_common_pool_layout(HD_COMMON_MEM_POOL_TYPE pool_type, int attr_nr, HD_COMMON_MEM_LAYOUT_ATTR *layout_attr, int del_layout)
{
	return hd_common_pool_layout(pool_type, attr_nr, layout_attr, del_layout);
}
HD_RESULT _hd_common_get_pool_usedbuf_info(HD_COMMON_MEM_POOL_TYPE pool_type, int *p_used_buf_nr, HD_BUFINFO *buf_info)
{
	return hd_common_get_pool_usedbuf_info(pool_type, p_used_buf_nr, buf_info);
}

HD_RESULT _hd_common_share_init(unsigned int value)
{
	return hd_common_share_init(value);
}

HD_RESULT _hd_common_set_chk_trigger_buf(HD_COMMON_CHK_TRI_BUF param)
{
	return hd_common_set_chk_trigger_buf(param);
}

HD_RESULT _hd_common_get_chk_trigger_buf(HD_COMMON_CHK_TRI_BUF *p_param)
{
	return hd_common_get_chk_trigger_buf(p_param);
}
#if USE_NVTMEM_CACHE_FLUSH
HD_RESULT hd_common_mem_flush_cache(void *virt_addr, unsigned int size)
{
	int ret;
	struct nvtmem_vaddr_info saddr;

	if (nvtmem_fd < 0) {
		nvtmem_fd = open("/dev/nvtmem0", O_RDWR);
		if (nvtmem_fd < 0) {
			return HD_ERR_SYS;
		}
	}
	if (virt_addr == NULL || size == 0) {
		HD_COMMON_ERR("hd_common_mem_flush_cache parameters err, virt_addr(%p) size(%d)\n", virt_addr, size);
		return HD_ERR_SYS;
	}
	saddr.vaddr = (uintptr_t)virt_addr;
	saddr.length = size;
	saddr.dir = DMA_DIR_BIDIRECTIONAL;  		// 1: DMA_TO_DEVICE, it means clean operation. 2:DMA_FROM_DEVICE, it means invalidate operation.

	HD_COMMON_FLOW("hd_common_mem_flush_cache:\n");
	HD_COMMON_FLOW("    virt_addr(%#lx) size(%ld) dir(%#x)\n", (ULONG)saddr.vaddr, saddr.length, saddr.dir);
	
	ret = ioctl(nvtmem_fd, NVTMEM_MEMORY_SYNC, &saddr);
	if (ret < 0) {
		printf("IOCTL fail func=%s, line=%d \r\n", __FUNCTION__, __LINE__);
		return HD_ERR_SYS;
	}
	return HD_OK;
}
#else
HD_RESULT hd_common_mem_flush_cache(void *virt_addr, unsigned int size)
{
	HD_RESULT ret;
	vpd_flush_data_t vpd_flush;

	if (vpd_fd <= 0) {
		return HD_ERR_SYS;
	}
	if (virt_addr == NULL || size == 0) {
		HD_COMMON_ERR("hd_common_mem_flush_cache parameters err, virt_addr(%p) size(%d)\n", virt_addr, size);
		return HD_ERR_SYS;
	}
	vpd_flush.dir = VPD_FLUSH_DIR_BIDIRECTIONAL;
	vpd_flush.addr_va = (uintptr_t)virt_addr;
	vpd_flush.size = size;
	HD_COMMON_FLOW("hd_common_mem_flush_cache:\n");
	HD_COMMON_FLOW("    virt_addr(%p) size(%d) dir(%#x)\n", (void *)vpd_flush.addr_va, vpd_flush.size, vpd_flush.dir);
	if (vpd_fd == 0) {
		vpd_fd = open("/dev/vpd", O_RDWR);
	}
	ret = ioctl(vpd_fd, VPD_FLUSH, &vpd_flush);
	if (ret < 0) {
		return HD_ERR_SYS;
	}
	return HD_OK;
}
#endif
HD_RESULT hd_common_mem_alloc(CHAR *name, UINTPTR *phy_addr, void **virt_addr, UINT32 size, HD_COMMON_MEM_DDR_ID ddr)
  {
      CHAR pool_name[64];
      int ddr_id = ddr;
      int start_ddr_no, end_ddr_no;
      HD_COMMON_MEM_VB_BLK blk = HD_COMMON_MEM_VB_INVALID_BLK;
  
      if (pif_get_ddr_id_by_chip(COMMON_PCIE_CHIP_RC, &start_ddr_no, &end_ddr_no) < 0)  {
          GMLIB_ERROR_P("pif_get_ddr_id_by_chip fail\n");
          return HD_ERR_FAIL;
      }
          
      if (size == 0 || ddr_id < start_ddr_no || ddr_id > end_ddr_no)  {
          HD_COMMON_ERR("%s, check size(%d),ddr(%d) failed.\n", __func__, size, ddr_id);
          return HD_ERR_INV;
      }

      if (get_pool_name_by_type(HD_COMMON_MEM_USER_BLK, pool_name, sizeof(pool_name), ddr_id) < 0) {
           HD_COMMON_ERR("%s, get size(%d),ddr(%d) failed.\n", __func__, size, ddr_id);
           return HD_ERR_NOT_FOUND;
      }
  
      blk = (HD_COMMON_MEM_VB_BLK)pif_alloc_video_buffer_for_hdal(size, ddr_id, pool_name, USR_LIB);
      if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
          HD_COMMON_ERR("%s, get blk_size(%d),ddr(%d) failed.\n", __func__, size, ddr_id);
          return HD_ERR_FAIL;
      }

      if (blk & 0x000000FF) {
          HD_COMMON_ERR("blk not align to 256 ,blk=%lx\n",  (ULONG)blk);
      }  else  {
          *phy_addr = hd_common_mem_blk2pa(blk);               
          *virt_addr = hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, *phy_addr, size);
      }
      
      HD_COMMON_FLOW("hd_common_mem_alloc:\n");
      HD_COMMON_FLOW("    name(%s) pool_type(0x%x) pa(%#lx) va(0x%p) size(%d) ddrid(%d) \n", !name ? "NULL" : name,
      HD_COMMON_MEM_USER_BLK, (ULONG)*phy_addr, *virt_addr, size, ddr_id);
      return HD_OK;    
  }


HD_RESULT hd_common_dmacopy(HD_COMMON_MEM_DDR_ID src_ddr, UINTPTR src_phy_addr,
			    HD_COMMON_MEM_DDR_ID dst_ddr, UINTPTR dst_phy_addr,
			    UINT32 len)
{
	dma_util_t dma_util;
	int dma_fd;
	int ret;


	if (pif_address_ddr_sanity_check(src_phy_addr, src_ddr) < 0) {
		HD_COMMON_ERR("%s:Check src pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)src_phy_addr, src_ddr);
		ret = HD_ERR_PARAM;
	}
	if (pif_address_ddr_sanity_check(dst_phy_addr, dst_ddr) < 0) {
		HD_COMMON_ERR("%s:Check dst pa(%#lx) ddrid(%d) error\n", __func__, (ULONG)dst_phy_addr, dst_ddr);
		ret = HD_ERR_PARAM;
	}	
	if ((dma_fd = open("/dev/dma_util",  O_WRONLY)) < 0) {
		HD_COMMON_ERR("open /dev/dma_util failed.\n");
		return HD_ERR_SYS;
	}

	dma_util.src_ddr_id = src_ddr;
	dma_util.src_addr = src_phy_addr;
	dma_util.dst_ddr_id = dst_ddr;
	dma_util.dst_addr = dst_phy_addr;
	dma_util.size = len;
	HD_COMMON_FLOW("hd_common_dmacopy:\n");
	HD_COMMON_FLOW("    src_ddr(%d) src_phy_addr(%p) dst_ddr(%d) dst_phy_addr(%p) len(%d)\n", \
		       src_ddr, (VOID *)src_phy_addr, dst_ddr, (VOID *)dst_phy_addr, len);

	ret = ioctl(dma_fd, DMA_UTIL_DO_DMA_MEMCPY, &dma_util);
	if (0 != ret) {
		HD_COMMON_ERR("Fail to dmacopy from %p(ddr%d) to %p(ddr%d) len(%d)\n",
			      (VOID *)dma_util.src_addr, dma_util.src_ddr_id,
			      (VOID *)dma_util.dst_addr, dma_util.dst_ddr_id,
			      dma_util.size);
		close(dma_fd);
		return HD_ERR_SYS;
	}

	close(dma_fd);
	return HD_OK;
}

HD_RESULT hd_common_mem_free(UINTPTR phy_addr, void *virt_addr)
{
    HD_COMMON_MEM_VB_BLK blk = HD_COMMON_MEM_VB_INVALID_BLK;
    
    if (phy_addr == 0 || virt_addr ==  NULL) {
        HD_COMMON_ERR("%s, check phy_addr(%#lx),virt_addr(%p) failed.\n", __func__, (ULONG)phy_addr, virt_addr);
        return HD_ERR_INV;
    }
    if (hd_common_mem_munmap(virt_addr, 0) != HD_OK)  {
        HD_COMMON_ERR("%s, hd_common_mem_munmap virt_addr(%p) failed.\n", __func__, virt_addr);
    }
    blk = pif_get_blk_by_pa(phy_addr);
    if (blk != HD_COMMON_MEM_VB_INVALID_BLK) {
        if (hd_common_mem_release_block(blk)  != HD_OK) {                
           return HD_ERR_FAIL;
        }
    } else { 
        HD_COMMON_ERR("Not find blk by pa(%#lx)\n", (ULONG)phy_addr);
    }
    return HD_OK;    
}


/**
 * used to get the number of chips.
 * return: the number of chips.
 */
INT hd_common_pcie_get_chip_num(VOID)
{
	/* 321 ONLY one chip */
	HD_COMMON_ERR("Not support %s\n", __func__);
	return 1;
}

UINT32 hd_common_mem_calc_buf_size(void *p_hd_data)
{
	HD_VIDEO_FRAME *p_frame = (HD_VIDEO_FRAME *)p_hd_data;
	int buf_size = 0;

	if (p_frame) {
		switch (p_frame->sign) {
		case MAKEFOURCC('2', '6', '5', 'D'):
			buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_frame->dim, HD_VIDEO_PXLFMT_YUV420_NVX4);
			//buf_size += common_calculate_dec_side_info_size(&(p_frame->dim), HD_CODEC_TYPE_H265);
			//buf_size += common_calculate_dec_mb_info_size(&(p_frame->dim), HD_CODEC_TYPE_H265);
			if (p_frame->ddr_id == DDR_ID2) {
				buf_size = buf_size + p_frame->dim.w * p_frame->dim.h; // add bitstream size for ep dec
			}
			//HD_COMMON_ERR("265 buf_size(%d) \n", buf_size);
			break;

		case MAKEFOURCC('2', '6', '4', 'D'):
			buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_frame->dim, HD_VIDEO_PXLFMT_YUV420_NVX4);
			//buf_size += common_calculate_dec_side_info_size(&(p_frame->dim), HD_CODEC_TYPE_H264);
			//buf_size += common_calculate_dec_mb_info_size(&(p_frame->dim), HD_CODEC_TYPE_H264);
			if (p_frame->ddr_id == DDR_ID2) {
				buf_size = buf_size + p_frame->dim.w * p_frame->dim.h; // add bitstream size for ep dec
			}
			//HD_COMMON_ERR("264 buf_size(%d) \n", buf_size);
			break;

		case MAKEFOURCC('J', 'P', 'G', 'D'):
			buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_frame->dim, HD_VIDEO_PXLFMT_YUV420_W8);
			if (p_frame->ddr_id == DDR_ID2) {
				buf_size = buf_size + p_frame->dim.w * p_frame->dim.h; // add bitstream size for ep dec
			}
			break;

		default:
			if (p_frame->pxlfmt) {
				buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_frame->dim, p_frame->pxlfmt);
				if (p_frame->pxlfmt == HD_VIDEO_PXLFMT_YUV420_NVX4) {
					// YUV420_NVX4 need to consider side_info and mb_info,
					// user did not specify codec_type, we use H265 as defulat here.
					//buf_size += common_calculate_dec_side_info_size(&(p_frame->dim), HD_CODEC_TYPE_H265);
					//buf_size += common_calculate_dec_mb_info_size(&(p_frame->dim), HD_CODEC_TYPE_H265);
					if (p_frame->ddr_id == DDR_ID2) {
						buf_size = buf_size + p_frame->dim.w * p_frame->dim.h; // add bitstream size for ep dec
					}
				}
			} else {
				buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_frame->dim, HD_VIDEO_PXLFMT_YUV422);
				if (p_frame->ddr_id == DDR_ID2) {
					buf_size = buf_size + p_frame->dim.w * p_frame->dim.h; // add bitstream size for ep dec
				}
			}
			//HD_COMMON_ERR("default: use 264 buf_size(%d) \n", buf_size);
		}
	}
	buf_size = (buf_size + 15) / 16 * 16;

	return buf_size;
}

/********************************************************************
    UTIL MENU IMPLEMENTATION
********************************************************************/
HD_RESULT util_menu_dump_log(void)
{
	return system("/bin/busybox cat /proc/videograph/dumplog");
}

HD_RESULT util_menu_set_storage(void)
{
	return system("echo 0 > /proc/videograph/mode");
}

HD_RESULT util_menu_set_console(void)
{
	return system("echo 1 > /proc/videograph/mode");
}

HD_RESULT util_menu_set_direct_print(void)
{
	return system("echo 2 > /proc/videograph/mode");
}

HD_RESULT util_menu_set_statistic_of_display_print(void)
{
	return system("cat /proc/videograph/statistic");
}

HD_RESULT util_menu_set_statistic_of_encode_print(void)
{
	return system("echo 10 > /proc/videograph/perf");
}

HD_RESULT util_menu_turn_off_statistic_of_encode_print(void)
{
	return system("echo 0 > /proc/videograph/perf");
}

HD_RESULT util_menu_dump_videograph(void)
{
	return system("cat /proc/videograph/graph");
}

static HD_DBG_MENU util_menu_p[] = {
	{0x01, "Start to dump log",         util_menu_dump_log,              TRUE},
	{0x02, "Set log to storage",        util_menu_set_storage,           TRUE},
	{0x03, "Set log to console",        util_menu_set_console,           TRUE},
	{0x04, "Set log to direct print",   util_menu_set_direct_print,      TRUE},
	{0x05, "dump the statistic of display per 10s", util_menu_set_statistic_of_display_print,      TRUE},
	{0x06, "dump the statistic of encode per 10s",  util_menu_set_statistic_of_encode_print,      TRUE},
	{0x07, "turn off the statistic of encode",  util_menu_turn_off_statistic_of_encode_print,      TRUE},
	{0x08, "dump the videograph",       util_menu_dump_videograph,       TRUE},
	// escape muse be last
	{ -1,   "",               NULL,               FALSE},
};

static HD_RESULT hd_util_menu_p(void)
{
	return hd_debug_menu_entry_p(util_menu_p, "UTIL");
}

/********************************************************************
    VPD MENU IMPLEMENTATION
********************************************************************/

HD_RESULT vpd_menu_p_set_level_0_p(void)
{
	return system("echo 0 > /proc/videograph/vpd/dbglevel");
}

HD_RESULT vpd_menu_p_set_level_1_p(void)
{
	return system("echo 1 > /proc/videograph/vpd/dbglevel");
}

HD_RESULT vpd_menu_p_set_level_2_p(void)
{
	return system("echo 2 > /proc/videograph/vpd/dbglevel");
}

HD_RESULT vpd_menu_p_set_level_3_p(void)
{
	return system("echo 3 > /proc/videograph/vpd/dbglevel");
}

static HD_DBG_MENU vpd_menu_p[] = {
	{0x01, "Set Debug Level 0",         vpd_menu_p_set_level_0_p,              TRUE},
	{0x02, "Set Debug Level 1",         vpd_menu_p_set_level_1_p,              TRUE},
	{0x03, "Set Debug Level 2",         vpd_menu_p_set_level_2_p,              TRUE},
	{0x04, "Set Debug Level 3",         vpd_menu_p_set_level_3_p,              TRUE},
	// escape muse be last
	{ -1,   "",               NULL,               FALSE},
};

HD_RESULT hd_vpd_menu_p(void)
{
	return hd_debug_menu_entry_p(vpd_menu_p, "VPD");
}

/********************************************************************
    USR MENU IMPLEMENTATION
********************************************************************/
HD_RESULT usr_menu_p_set_level_0_p(void)
{
	return system("echo 0 > /proc/videograph/usr/dbglevel");
}

HD_RESULT usr_menu_p_set_level_1_p(void)
{
	return system("echo 1 > /proc/videograph/usr/dbglevel");
}

HD_RESULT usr_menu_p_set_level_2_p(void)
{
	return system("echo 2 > /proc/videograph/usr/dbglevel");
}

HD_RESULT usr_menu_p_set_level_3_p(void)
{
	return system("echo 3 > /proc/videograph/usr/dbglevel");
}

static HD_DBG_MENU usr_menu_p[] = {
	{0x01, "Set Debug Level 0",         usr_menu_p_set_level_0_p,              TRUE},
	{0x02, "Set Debug Level 1",         usr_menu_p_set_level_1_p,              TRUE},
	{0x03, "Set Debug Level 2",         usr_menu_p_set_level_2_p,              TRUE},
	{0x04, "Set Debug Level 3",         usr_menu_p_set_level_3_p,              TRUE},
	// escape muse be last
	{ -1,   "",               NULL,               FALSE},
};

HD_RESULT hd_usr_menu_p(void)
{
	return hd_debug_menu_entry_p(usr_menu_p, "USR");
}

/********************************************************************
    EM MENU IMPLEMENTATION
********************************************************************/
HD_RESULT em_menu_p_set_level_0_p(void)
{
	return system("echo 0 > /proc/videograph/em/dbglevel");
}

HD_RESULT em_menu_p_set_level_1_p(void)
{
	return system("echo 1 > /proc/videograph/em/dbglevel");
}

HD_RESULT em_menu_p_set_level_2_p(void)
{
	return system("echo 2 > /proc/videograph/em/dbglevel");
}

HD_RESULT em_menu_p_set_level_3_p(void)
{
	return system("echo 3 > /proc/videograph/em/dbglevel");
}

static HD_DBG_MENU em_menu_p[] = {
	{0x01, "Set Debug Level 0",         em_menu_p_set_level_0_p,              TRUE},
	{0x02, "Set Debug Level 1",         em_menu_p_set_level_1_p,              TRUE},
	{0x03, "Set Debug Level 2",         em_menu_p_set_level_2_p,              TRUE},
	{0x04, "Set Debug Level 3",         em_menu_p_set_level_3_p,              TRUE},
	// escape muse be last
	{ -1,   "",               NULL,               FALSE},
};

HD_RESULT hd_em_menu_p(void)
{
	return hd_debug_menu_entry_p(em_menu_p, "EM");
}

/********************************************************************
    MS MENU IMPLEMENTATION
********************************************************************/
HD_RESULT ms_menu_p_set_level_0_p(void)
{
	return system("echo 0 > /proc/videograph/ms/dbglevel");
}

HD_RESULT ms_menu_p_set_level_1_p(void)
{
	return system("echo 1 > /proc/videograph/ms/dbglevel");
}

HD_RESULT ms_menu_p_set_level_2_p(void)
{
	return system("echo 2 > /proc/videograph/ms/dbglevel");
}

HD_RESULT ms_menu_p_set_level_3_p(void)
{
	return system("echo 3 > /proc/videograph/ms/dbglevel");
}

static HD_DBG_MENU ms_menu_p[] = {
	{0x01, "Set Debug Level 0",         ms_menu_p_set_level_0_p,              TRUE},
	{0x02, "Set Debug Level 1",         ms_menu_p_set_level_1_p,              TRUE},
	{0x03, "Set Debug Level 2",         ms_menu_p_set_level_2_p,              TRUE},
	{0x04, "Set Debug Level 3",         ms_menu_p_set_level_3_p,              TRUE},
	// escape muse be last
	{ -1,   "",               NULL,               FALSE},
};

HD_RESULT hd_ms_menu_p(void)
{
	return hd_debug_menu_entry_p(ms_menu_p, "MS");
}

/********************************************************************
    GS MENU IMPLEMENTATION
********************************************************************/
HD_RESULT gs_menu_p_set_level_0_p(void)
{
	return system("echo 0 > /proc/videograph/gs/dbglevel");
}

HD_RESULT gs_menu_p_set_level_1_p(void)
{
	return system("echo 1 > /proc/videograph/gs/dbglevel");
}

HD_RESULT gs_menu_p_set_level_2_p(void)
{
	return system("echo 2 > /proc/videograph/gs/dbglevel");
}

HD_RESULT gs_menu_p_set_level_3_p(void)
{
	return system("echo 3 > /proc/videograph/gs/dbglevel");
}

static HD_DBG_MENU gs_menu_p[] = {
	{0x01, "Set Debug Level 0",         gs_menu_p_set_level_0_p,              TRUE},
	{0x02, "Set Debug Level 1",         gs_menu_p_set_level_1_p,              TRUE},
	{0x03, "Set Debug Level 2",         gs_menu_p_set_level_2_p,              TRUE},
	{0x04, "Set Debug Level 3",         gs_menu_p_set_level_3_p,              TRUE},
	// escape muse be last
	{ -1,   "",               NULL,               FALSE},
};

HD_RESULT hd_gs_menu_p(void)
{
	return hd_debug_menu_entry_p(gs_menu_p, "GS");
}

/********************************************************************
    BUFFER MENU IMPLEMENTATION
********************************************************************/
HD_RESULT buffer_menu_p_dump_au_dec_pool_p(void)
{
	return system("cat /proc/videograph/buffer/au_dec");
}

HD_RESULT buffer_menu_p_dump_au_enc_pool_p(void)
{
	return system("cat /proc/videograph/buffer/au_enc");
}

HD_RESULT buffer_menu_p_dump_common_pool_p(void)
{
	return system("cat /proc/videograph/buffer/common");
}

HD_RESULT buffer_menu_p_dump_disp0_cap_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp0_cap_out");
}

HD_RESULT buffer_menu_p_dump_disp0_enc_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp0_enc_out");
}

HD_RESULT buffer_menu_p_dump_disp0_enc_scl_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp0_enc_scl_out");
}

HD_RESULT buffer_menu_p_dump_disp0_in_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp0_in");
}

HD_RESULT buffer_menu_p_dump_disp1_cap_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp1_cap_out");
}

HD_RESULT buffer_menu_p_dump_disp1_enc_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp1_enc_out");
}

HD_RESULT buffer_menu_p_dump_disp1_enc_scl_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp1_enc_scl_out");
}

HD_RESULT buffer_menu_p_dump_disp1_in_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp1_in");
}

HD_RESULT buffer_menu_p_dump_disp2_cap_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp2_cap_out");
}

HD_RESULT buffer_menu_p_dump_disp2_enc_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp2_enc_out");
}

HD_RESULT buffer_menu_p_dump_disp2_enc_scl_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp2_enc_scl_out");
}

HD_RESULT buffer_menu_p_dump_disp2_in_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp2_in");
}

HD_RESULT buffer_menu_p_dump_disp3_cap_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp3_cap_out");
}

HD_RESULT buffer_menu_p_dump_disp3_enc_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp3_enc_out");
}

HD_RESULT buffer_menu_p_dump_disp3_enc_scl_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp3_enc_scl_out");
}

HD_RESULT buffer_menu_p_dump_disp3_in_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp3_in");
}

HD_RESULT buffer_menu_p_dump_disp_dec_in_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp_dec_in");
}

HD_RESULT buffer_menu_p_dump_disp_dec_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp_dec_out");
}

HD_RESULT buffer_menu_p_dump_disp_dec_out_ratio_pool_p(void)
{
	return system("cat /proc/videograph/buffer/disp_dec_out_ratio");
}

HD_RESULT buffer_menu_p_dump_enc_cap_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/enc_cap_out");
}

HD_RESULT buffer_menu_p_dump_enc_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/enc_out");
}

HD_RESULT buffer_menu_p_dump_enc_scl_out_pool_p(void)
{
	return system("cat /proc/videograph/buffer/enc_scl_out");
}

HD_RESULT buffer_menu_p_dump_flow_md_pool_p(void)
{
	return system("cat /proc/videograph/buffer/flow_md");
}

HD_RESULT buffer_menu_p_dump_user_blk_p(void)
{
	return system("cat /proc/videograph/buffer/user_blk");
}

static HD_DBG_MENU pool_menu_p[] = {
	{0x01, "Dump au dec pool",          buffer_menu_p_dump_au_dec_pool_p,              TRUE},
	{0x02, "Dump au enc pool",          buffer_menu_p_dump_au_enc_pool_p,              TRUE},
	{0x03, "Dump common pool",          buffer_menu_p_dump_common_pool_p,              TRUE},
	{0x04, "Dump disp0_cap_out pool",   buffer_menu_p_dump_disp0_cap_out_pool_p,       TRUE},
	{0x05, "Dump disp0_enc_out pool",   buffer_menu_p_dump_disp0_enc_out_pool_p,       TRUE},
	{0x06, "Dump disp0_enc_scl_out pool",   buffer_menu_p_dump_disp0_enc_scl_out_pool_p,   TRUE},
	{0x07, "Dump disp0_in pool",        buffer_menu_p_dump_disp0_in_pool_p,            TRUE},
	{0x08, "Dump disp1_cap_out pool",   buffer_menu_p_dump_disp1_cap_out_pool_p,       TRUE},
	{0x09, "Dump disp1_enc_out pool",   buffer_menu_p_dump_disp1_enc_out_pool_p,       TRUE},
	{0x0A, "Dump disp1_enc_scl_out pool",   buffer_menu_p_dump_disp1_enc_scl_out_pool_p,   TRUE},
	{0x0B, "Dump disp1_in pool",        buffer_menu_p_dump_disp1_in_pool_p,            TRUE},
	{0x0C, "Dump disp2_cap_out pool",   buffer_menu_p_dump_disp2_cap_out_pool_p,       TRUE},
	{0x0D, "Dump disp2_enc_out pool",   buffer_menu_p_dump_disp2_enc_out_pool_p,       TRUE},
	{0x0E, "Dump disp2_enc_scl_out pool",   buffer_menu_p_dump_disp2_enc_scl_out_pool_p,   TRUE},
	{0x0F, "Dump disp2_in pool",        buffer_menu_p_dump_disp2_in_pool_p,            TRUE},
	{0x10, "Dump disp3_cap_out pool",   buffer_menu_p_dump_disp3_cap_out_pool_p,       TRUE},
	{0x11, "Dump disp3_enc_out pool",   buffer_menu_p_dump_disp3_enc_out_pool_p,       TRUE},
	{0x12, "Dump disp3_enc_scl_out pool",   buffer_menu_p_dump_disp3_enc_scl_out_pool_p,   TRUE},
	{0x13, "Dump disp3_in pool",        buffer_menu_p_dump_disp3_in_pool_p,            TRUE},
	{0x14, "Dump disp_dec_in pool",     buffer_menu_p_dump_disp_dec_in_pool_p,         TRUE},
	{0x15, "Dump disp_dec_out pool",    buffer_menu_p_dump_disp_dec_out_pool_p,        TRUE},
	{0x16, "Dump disp_dec_out_ratio pool",  buffer_menu_p_dump_disp_dec_out_ratio_pool_p,  TRUE},
	{0x17, "Dump enc_cap_out pool",     buffer_menu_p_dump_enc_cap_out_pool_p,         TRUE},
	{0x18, "Dump enc_out pool",         buffer_menu_p_dump_enc_out_pool_p,             TRUE},
	{0x19, "Dump enc_scl_out pool",     buffer_menu_p_dump_enc_scl_out_pool_p,         TRUE},
	{0x1A, "Dump flow_md pool",         buffer_menu_p_dump_flow_md_pool_p,             TRUE},
	{0x1B, "Dump user blk",             buffer_menu_p_dump_user_blk_p,                 TRUE},
	// escape muse be last
	{ -1,   "",               NULL,               FALSE},
};

HD_RESULT hd_pool_menu_p(void)
{
	return hd_debug_menu_entry_p(pool_menu_p, "BUFFER");
}

static int hd_common_version_p(void)
{
	HD_COMMON_MSG("HDAL: Version: v%x.%02x.%03x\r\n",
		      (HDAL_VERSION & 0xF00000) >> 20,
		      (HDAL_VERSION & 0x0FF000) >> 12,
		      (HDAL_VERSION & 0x000FFF));
	return 0;
}

/********************************************************************
    HD_COMMON MENU IMPLEMENTATION
********************************************************************/
// register your module here
static HD_DBG_MENU common_menu_p[] = {
	{0x01, "VERSION",        hd_common_version_p,  TRUE},
	{0x02, "UTIL",           hd_util_menu_p,       TRUE},
	{0x03, "VPD",            hd_vpd_menu_p,        TRUE},
	{0x04, "USR",            hd_usr_menu_p,        TRUE},
	{0x05, "EM",             hd_em_menu_p,         TRUE},
	{0x06, "MS",             hd_ms_menu_p,         TRUE},
	{0x07, "GS",             hd_gs_menu_p,         TRUE},
	{0x08, "POOL",           hd_pool_menu_p,       TRUE},
	// escape muse be last
	{HD_DEBUG_MENU_ID_LAST,  "", NULL,             FALSE},
};

HD_RESULT hd_common_menu_p(void)
{
	return hd_debug_menu_entry_p(common_menu_p, "COMMON");
}

