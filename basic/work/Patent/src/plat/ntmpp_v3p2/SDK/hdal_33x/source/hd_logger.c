/**
	@brief Source file of logger of library.\n
	This file contains the logger functions of library.

	@file hd_logger.c

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
#include "hd_type.h"
#include "hd_logger.h"
#include "logger.h"

#define PROC_MSG_SIZE     	4096

unsigned int g_hd_mask_err = (unsigned int)0;
unsigned int g_hd_mask_wrn = (unsigned int)0;
unsigned int g_hd_mask_ind = (unsigned int)0;
unsigned int g_hd_mask_msg = (unsigned int)0;
unsigned int g_hd_mask_func = (unsigned int)0;

extern char *pif_msg;
extern unsigned int hdal_flow_dbgmode;

void pif_send_log_p(unsigned char *str, unsigned int len);

void hd_printf(const char *fmt, ...)
{
	va_list ap;
	unsigned int length;

	if (hdal_flow_dbgmode == 0) {  // 0:no message/no log,     1:enable lib message,    2:log flow
		return;
	}
	if (pif_msg == NULL) {
		return;
	}

	memset(pif_msg, 0x0, PROC_MSG_SIZE);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	length = vsnprintf(pif_msg, PROC_MSG_SIZE, fmt, ap);
	pif_send_log_p((unsigned char *)pif_msg, length);
	va_end(ap);
}
