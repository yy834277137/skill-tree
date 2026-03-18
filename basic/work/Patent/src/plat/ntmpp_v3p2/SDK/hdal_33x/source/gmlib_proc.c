/*
 *   @file   gmlib_proc.c
 *
 *   @brief  gmlib_proc function for /proc system.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <stdarg.h>

#include "cif_list.h"
#include "cif_common.h"
#include "vpd_ioctl.h"
#include "pif.h"
#include "utl.h"
#include "debug.h"
#include "log.h"

#define GMLIB_PROC_VERSION "v0.2"

/* gmlib_proc_bind, need to sync the value in vg_log.c */
/*
  |--SETTING_MSG_SIZE--|--FLOW_MSG_SIZE--|--ERR_MSG_SIZE--|--HDAL_SETTING_MSG_SIZE--|--HDAL_FLOW_MSG_SIZE--|
                        <-- MSG_LENGTH_SIZE                                          <-- MSG_LENGTH_SIZE
                         <--MSG_OFFSET_SIZE                                           <--MSG_OFFSET_SIZE
                                          <-- MSG_LENGTH_SIZE
                                           <--MSG_OFFSET_SIZE

 */
#define MSG_LENGTH_SIZE       8
#define MSG_OFFSET_SIZE       8

#define SETTING_MSG_SIZE      (96 * 1024)
#define FLOW_MSG_SIZE         (128 * 1024)
#define ERR_MSG_SIZE          (32 * 1024)
#define HDAL_SETTING_MSG_SIZE (96 * 1024)
#define HDAL_FLOW_MSG_SIZE    (128 * 1024)
#define MMAP_MSG_LEN          (SETTING_MSG_SIZE + FLOW_MSG_SIZE + ERR_MSG_SIZE + HDAL_SETTING_MSG_SIZE + HDAL_FLOW_MSG_SIZE)

#define SETTING_MSG_OFFSET(x)         (x)
#define FLOW_MSG_OFFSET(x)            (SETTING_MSG_OFFSET(x) + SETTING_MSG_SIZE)
#define ERR_MSG_OFFSET(x)             (FLOW_MSG_OFFSET(x) + FLOW_MSG_SIZE)
#define HDAL_SETTING_MSG_OFFSET(x)    (ERR_MSG_OFFSET(x) + ERR_MSG_SIZE)
#define HDAL_FLOW_MSG_OFFSET(x)       (HDAL_SETTING_MSG_OFFSET(x) + HDAL_SETTING_MSG_SIZE)



char *buffer_mapping = 0;
int log_fd = 0;
char *proc_msg = 0;

extern vpd_sys_info_t platform_sys_Info;
extern pif_group_t *pif_group[GM_MAX_GROUP_NUM];
extern unsigned int gmlib_dbg_level;
pthread_mutex_t log_mutex;

char *hdal_get_log_addr(void)
{
	char *msg = NULL;

	if (!buffer_mapping) {
		return NULL;
	}
	msg = buffer_mapping + SETTING_MSG_SIZE + FLOW_MSG_SIZE + ERR_MSG_SIZE;
	return msg;
}

int hdal_get_log_buffer_len(void)
{
	return HDAL_SETTING_MSG_SIZE;
}

unsigned int gmlib_flow_dbgmode = 0;
void gmlib_flow_enable(unsigned char *cmd_str)
{
	if (*cmd_str == '1')
		gmlib_flow_dbgmode = 1;
	else
		gmlib_flow_dbgmode = 0;
}

void gmlib_flow_log(char *msg_buf, int msg_len)
{
	unsigned int *flow_msg_len, *flow_msg_offset;
	unsigned int len = 0;
	char *flow_msg;
	struct timeval tv;
	unsigned int curr_ms;
	unsigned int msg_buf_size = FLOW_MSG_SIZE - MSG_LENGTH_SIZE - MSG_OFFSET_SIZE;

	if (proc_msg == 0)
		return;
	if (buffer_mapping == 0)
		return;
	if (gmlib_flow_dbgmode != 1)
		return;
	if (msg_len > PROC_MSG_SIZE)
		return;

	pthread_mutex_lock(&log_mutex); //protect proc_msg & msg_len & msg_offset

	memset(proc_msg, 0x0, PROC_MSG_SIZE);
	gettimeofday(&tv, NULL);
	curr_ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	len = sprintf(proc_msg, "[%04x] ", curr_ms & 0xffff);
	len += sprintf(proc_msg + len, "%s", (char *)msg_buf);

	flow_msg = FLOW_MSG_OFFSET(buffer_mapping) + MSG_LENGTH_SIZE + MSG_OFFSET_SIZE;
	flow_msg_len = (unsigned int *)FLOW_MSG_OFFSET(buffer_mapping);
	flow_msg_offset = (unsigned int *)(FLOW_MSG_OFFSET(buffer_mapping) + MSG_LENGTH_SIZE);

	/* print sequence:
		1.msg_offset=0: print msg_offset~msg_len
		1.msg_offset>0: print msg_offset~(msg_len-msg_offset), print 0~msg_offset
	 */
	if (*flow_msg_offset == 0) {
		if ((*flow_msg_len + len) < msg_buf_size) {
			/*
			<- msg_offset
			  <- msg_len (original)  <- msg_buf_size
			x-------x----------------------x
			  |<--len-->|<- msg_len (new)
			 */
			*flow_msg_len += sprintf(flow_msg + *flow_msg_len, "%s", proc_msg);
		} else {
			/*
			<- msg_offset(original)        <- msg_buf_size
			                    <- msg_len (original)
			x-------------------------x----x
			                    |<--len-->|
			|<--len-->|<- msg_offset(new)
			 */
			*flow_msg_offset = sprintf(flow_msg, "%s", proc_msg);
		}
	} else {
		/* buffer is enough */
		if ((*flow_msg_offset + len) < msg_buf_size) {
			/*
			             <- msg_offset(original)
			                    <- msg_len (original)
			                         <- msg_buf_size
			x------------------x------x----x
			             |<--len-->|
			                       <- msg_offset(new), msg_len(new)
			 */
			if ((*flow_msg_offset + len) > *flow_msg_len)
				*flow_msg_len = *flow_msg_offset + len;
			*flow_msg_offset += sprintf(flow_msg + *flow_msg_offset, "%s", proc_msg);
		} else {
			/*
			                    <- msg_offset(original)
			                      <- msg_len (original)
			                          <- msg_buf_size
			x-------------------------x-x---x
			                    |<--len-->|
			                    <- msg_len(new)
			|<--len-->|<- msg_offset(new)
			 */
			*flow_msg_len = *flow_msg_offset;
			*flow_msg_offset = sprintf(flow_msg, "%s", proc_msg);
		}
	}
	pthread_mutex_unlock(&log_mutex);
}

void gmlib_err_log(const char *fmt, ...)
{
	va_list ap;
	unsigned int *err_msg_len, *err_msg_offset;
	unsigned int len;
	char *err_msg;
	struct timeval tv;
	unsigned int curr_ms;
	unsigned int msg_buf_size = ERR_MSG_SIZE - MSG_LENGTH_SIZE - MSG_OFFSET_SIZE;

	if (proc_msg == 0)
		return;
	if (buffer_mapping == 0)
		return;

	pthread_mutex_lock(&log_mutex); //protect proc_msg & msg_len & msg_offset
	memset(proc_msg, 0x0, PROC_MSG_SIZE);
	gettimeofday(&tv, NULL);
	curr_ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	len = sprintf(proc_msg, "[%04x] ", curr_ms & 0xffff);

	va_start(ap, fmt);
	len += vsnprintf(proc_msg + len, PROC_MSG_SIZE, fmt, ap);
	va_end(ap);

	err_msg = ERR_MSG_OFFSET(buffer_mapping) + MSG_LENGTH_SIZE + MSG_OFFSET_SIZE;
	err_msg_len = (unsigned int *)ERR_MSG_OFFSET(buffer_mapping);
	err_msg_offset = (unsigned int *)(ERR_MSG_OFFSET(buffer_mapping) + MSG_LENGTH_SIZE);

	/* print sequence:
		1.msg_offset=0: print msg_offset~msg_len
		1.msg_offset>0: print msg_offset~(msg_len-msg_offset), print 0~msg_offset
	 */
	if (*err_msg_offset == 0) {
		if ((*err_msg_len + len) < msg_buf_size) {
			/*
			<- msg_offset
			  <- msg_len (original)  <- msg_buf_size
			x-------x----------------------x
			  |<--len-->|<- msg_len (new)
			 */
			*err_msg_len += sprintf(err_msg + *err_msg_len, "%s", proc_msg);
		} else {
			/*
			<- msg_offset(original)        <- msg_buf_size
			                    <- msg_len (original)
			x-------------------------x----x
			                    |<--len-->|
			|<--len-->|<- msg_offset(new)
			 */
			*err_msg_offset = sprintf(err_msg, "%s", proc_msg);
		}
	} else {
		if ((*err_msg_offset + len) < msg_buf_size) {
			/*
			             <- msg_offset(original)
			                    <- msg_len (original)
			                         <- msg_buf_size
			x------------------x------x----x
			             |<--len-->|
			                       <- msg_offset(new), msg_len(new)
			 */
			if ((*err_msg_offset + len) > *err_msg_len)
				*err_msg_len = *err_msg_offset + len;
			*err_msg_offset += sprintf(err_msg + *err_msg_offset, "%s", proc_msg);
		} else {
			/*
			                    <- msg_offset(original)
			                      <- msg_len (original)
			                          <- msg_buf_size
			x-------------------------x-x---x
			                    |<--len-->|
			                    <- msg_len(new)
			|<--len-->|<- msg_offset(new)
			 */
			*err_msg_len = *err_msg_offset;
			*err_msg_offset = sprintf(err_msg, "%s", proc_msg);
		}
	}
	pthread_mutex_unlock(&log_mutex);
	return;
}

unsigned int hdal_flow_dbgmode = 1;
void hdal_flow_enable(unsigned int cmd_value)
{
	hdal_flow_dbgmode = cmd_value;
}

void hdal_flow_log_p(unsigned int flag, const char *fmt, ...)
{
	va_list ap;
	unsigned int *flow_msg_len, *flow_msg_offset;
	unsigned int len = 0;
	unsigned int msg_buf_size = HDAL_FLOW_MSG_SIZE - MSG_LENGTH_SIZE - MSG_OFFSET_SIZE;
	char *flow_msg;

	if (proc_msg == 0)
		return;
	if (buffer_mapping == 0)
		return;
	if ((hdal_flow_dbgmode & flag) == 0)
		return;
	pthread_mutex_lock(&log_mutex); //protect proc_msg & msg_len & msg_offset
	memset(proc_msg, 0x0, PROC_MSG_SIZE);
	va_start(ap, fmt);
	len = vsnprintf(proc_msg, PROC_MSG_SIZE, fmt, ap);
	va_end(ap);

	if (gmlib_dbg_level >= GMLIB_DBG_LEVEL_1) {
		pif_send_log("(%d):%s", (int)syscall(SYS_gettid), proc_msg);
	}

	flow_msg = HDAL_FLOW_MSG_OFFSET(buffer_mapping) + MSG_LENGTH_SIZE + MSG_OFFSET_SIZE;
	flow_msg_len = (unsigned int *)HDAL_FLOW_MSG_OFFSET(buffer_mapping);
	flow_msg_offset = (unsigned int *)(HDAL_FLOW_MSG_OFFSET(buffer_mapping) + MSG_LENGTH_SIZE);

	/* print sequence:
		1.msg_offset=0: print msg_offset~msg_len
		1.msg_offset>0: print msg_offset~(msg_len-msg_offset), print 0~msg_offset
	 */
	if (*flow_msg_offset == 0) {
		/*
		<- msg_offset
		  <- msg_len (original)  <- msg_size
		x-------x----------------------x
		  |<--len-->|<- msg_len (new)
		 */
		if ((*flow_msg_len + len) < msg_buf_size) {
			*flow_msg_len += sprintf(flow_msg + *flow_msg_len, "%s", proc_msg);
		} else {
			/*
			<- msg_offset(original)        <- msg_size
			                    <- msg_len (original)
			x-------------------------x----x
			                    |<--len-->|
			|<--len-->|<- msg_offset(new)
			 */
			*flow_msg_offset = sprintf(flow_msg, "%s", proc_msg);
		}
	} else {
		if ((*flow_msg_offset + len) < msg_buf_size) {
			/*
			             <- msg_offset(original)
			                    <- msg_len (original)
			                         <- msg_size
			x------------------x------x----x
			             |<--len-->|
			                       <- msg_offset(new), msg_len(new)
			 */
			if ((*flow_msg_offset + len) > *flow_msg_len)
				*flow_msg_len = *flow_msg_offset + len;
			*flow_msg_offset += sprintf(flow_msg + *flow_msg_offset, "%s", proc_msg);
		} else {
			/*
			                    <- msg_offset(original)
			                      <- msg_len (original)
			                          <- msg_size
			x-------------------------x-x---x
			                    |<--len-->|
			                    <- msg_len(new)
			|<--len-->|<- msg_offset(new)
			 */
			*flow_msg_len = *flow_msg_offset;
			*flow_msg_offset = sprintf(flow_msg, "%s", proc_msg);
		}
	}
	pthread_mutex_unlock(&log_mutex);
	return;
}

void hdal_show_alloc_statistic(void)
{
	int total_count, total_size;
	extern pif_dbg_alloc_statistic_t pif_dbg_alloc;

	printf("Allocate count: malloc(%d)+calloc(%d)+realloc(%d)=%d\n",
	       pif_dbg_alloc.pif_dbg_malloc_cnt, pif_dbg_alloc.pif_dbg_calloc_cnt, pif_dbg_alloc.pif_dbg_realloc_cnt,
	       (total_count = pif_dbg_alloc.pif_dbg_malloc_cnt + pif_dbg_alloc.pif_dbg_calloc_cnt + pif_dbg_alloc.pif_dbg_realloc_cnt));
	printf("Allocate size: malloc(%d)+calloc(%d)+realloc(%d)=%d\n",
	       pif_dbg_alloc.pif_dbg_malloc_size, pif_dbg_alloc.pif_dbg_calloc_size, pif_dbg_alloc.pif_dbg_realloc_size,
	       (total_size = pif_dbg_alloc.pif_dbg_malloc_size + pif_dbg_alloc.pif_dbg_calloc_size + pif_dbg_alloc.pif_dbg_realloc_size));

	printf("Free count : free=%d\n", pif_dbg_alloc.pif_dbg_free_cnt);
	printf("Free size : free=%d\n", pif_dbg_alloc.pif_dbg_free_size);
	printf("----------------------------\n");
	printf("Diff count : diff=%d\n", total_count - pif_dbg_alloc.pif_dbg_free_cnt);
	printf("Diff size : diff=%d\n", total_size - pif_dbg_alloc.pif_dbg_free_size);
	printf("----------------------------\n");
}

void printlog(const char *fmt, ...)
{
	va_list ap;
	char msg[PROC_MSG_SIZE];
	INT ioctl_ret;

	memset(msg, 0x0, PROC_MSG_SIZE);
	va_start(ap, fmt);
	vsnprintf(msg, PROC_MSG_SIZE, fmt, ap);
	va_end(ap);

	if (log_fd == 0) {
		log_fd = open("/dev/log_vg", O_RDWR);
	}
	ioctl_ret = ioctl(log_fd, IOCTL_PRINTM, msg);
	if (ioctl_ret < 0) {
		printf("ioctl \"IOCTL_PRINTM\" return %d\n", ioctl_ret);
	}
}

void gmlib_proc_init(void)
{
	if (log_fd == 0) {
		log_fd = open("/dev/log_vg", O_RDWR);
	}
	if (!log_fd) {
		printf("Error to open /dev/log_vg\n");
	}

	if (pthread_mutex_init(&log_mutex, NULL)) {
		printf("log_mutex failed!");
		return;
	}
	if (!buffer_mapping) {
		buffer_mapping = (char *)mmap(0, MMAP_MSG_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, log_fd, 0);
	}
	if (!buffer_mapping) {
		printf("Error to mmap /dev/log_vg\n");
		return;
	}
	if ((proc_msg = (char *)PIF_MALLOC(PROC_MSG_SIZE)) == NULL) {
		printf("Error to allcate gmlib proc message buffer.\n");
		return;
	}
}

void gmlib_proc_close(void)
{
	if (log_fd) {
		close(log_fd);
		log_fd = 0;
	}
	if (buffer_mapping) {
		munmap(buffer_mapping, MMAP_MSG_LEN);
		buffer_mapping = 0;
	}
	if (proc_msg) {
		PIF_FREE(proc_msg);
		proc_msg = 0;
	}
	pthread_mutex_destroy(&log_mutex);
}
