/**
    @brief Source code of videocapture menu.\n
    This file contains the functions which is related to videocapture debug menu.

    @file hd_videocapture_menu.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_common.h"
#include "hd_type.h"
#include "hd_util.h"
#include "hd_debug_menu.h"
#include <string.h>

static int hd_util_show_status_p(void)
{
	printf("------------------------- UTIL ------------------------------------------------\r\n");
	printf("current time (ms)\r\n");
	printf("%-10ld\r\n",	hd_gettime_ms());

	return 0;
}

static HD_DBG_MENU util_debug_menu[] = {
	{0x01, "dump status", hd_util_show_status_p,              TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_util_menu_p(void)
{
	return hd_debug_menu_entry_p(util_debug_menu, "UTIL");
}

