/**
	@brief Source file of utility of library.\n
	This file contains the utility functions of library.

	@file hd_util.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <sys/ioctl.h>
#include "hd_type.h"
#include "hd_logger.h"
#include "hd_util.h"
#include <string.h>
#include "vpd_ioctl.h"

#define HD_MODULE_NAME HD_UTIL
#define DBG_ERR(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _ERR)("\033[1;31m" fmtstr "\033[0m", ##args)
#define DBG_WRN(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _WRN)("\033[1;33m" fmtstr "\033[0m", ##args)
#define DBG_IND(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _IND)(fmtstr, ##args)
#define DBG_DUMP(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _MSG)(fmtstr, ##args)
#define DBG_FUNC_BEGIN(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("BEGIN: " fmtstr, ##args)
#define DBG_FUNC_END(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("END: " fmtstr, ##args)

extern int vpd_fd;

#define HD_KEYINPUT_SIZE 80
UINT32 hd_read_decimal_key_input(const CHAR *comment)
{
	char    cmd[HD_KEYINPUT_SIZE];
	int     radix = 0;
	char    *ret = NULL;

	DBG_DUMP("%s: ", comment);
	fflush(stdin);
	do {
		ret = fgets(cmd, sizeof(cmd), stdin);
	} while (NULL == ret || cmd[0] == ' ' || cmd[0] == '\n') ;

	if (!strncmp(cmd, "0x", 2)) {
		radix = 16;
	} else {
		radix = 10;
	}

	return (strtoul(cmd, (char **) NULL, radix));
}

UINT32 hd_gettime_ms(VOID)
{
	UINT64 jiffies_us = 0;
	int   ret;

	if (vpd_fd < 0) {
		DBG_ERR("hd_common not init\r\n");
		return 0;
	}
	ret = ioctl(vpd_fd, VPD_GET_JIFFIES_U64_US, &jiffies_us);
	if (ret < 0) {
		return 0;
	}
	return (UINT32)(jiffies_us/1000);
}

UINT64 hd_gettime_us(VOID)
{
	UINT64 jiffies_us = 0;
	int   ret;

	if (vpd_fd < 0) {
		DBG_ERR("hd_common not init\r\n");
		return 0;
	}
	ret = ioctl(vpd_fd, VPD_GET_JIFFIES_U64_US, &jiffies_us);
	if (ret < 0) {
		return 0;
	}
	return jiffies_us;
}

