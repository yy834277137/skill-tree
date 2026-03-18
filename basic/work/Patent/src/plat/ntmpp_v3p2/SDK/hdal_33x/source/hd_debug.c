/**
	@brief Source file of debug of library.\n
	This file contains the debug menu functions of library.

	@file hd_debug.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "logger.h"
#include "debug.h"
#include "hd_debug_menu.h"
#include "hd_util.h"
#include <string.h>

extern HD_RESULT hd_audiocap_menu_p(void);
extern HD_RESULT hd_audioout_menu_p(void);
extern HD_RESULT hd_audioenc_menu_p(void);
extern HD_RESULT hd_audiodec_menu_p(void);
extern HD_RESULT hd_videocap_menu_p(void);
extern HD_RESULT hd_videoout_menu_p(void);
extern HD_RESULT hd_videoproc_menu_p(void);
extern HD_RESULT hd_videoenc_menu_p(void);
extern HD_RESULT hd_videodec_menu_p(void);
extern HD_RESULT hd_gfx_menu_p(void);
extern HD_RESULT hd_vendor_menu_p(void);
extern HD_RESULT hd_common_menu_p(void);
extern HD_RESULT hd_util_menu_p(void);
extern HD_RESULT hd_debug_menu_p(void);

// register your module here
static HD_DBG_MENU root_menu_p[] = {
	{0x01, "AUDIOCAPTURE",   hd_audiocap_menu_p, TRUE},
	{0x02, "AUDIOOUT",       hd_audioout_menu_p, TRUE},
	{0x03, "AUDIOENC",       hd_audioenc_menu_p, TRUE},
	{0x04, "AUDIODEC",       hd_audiodec_menu_p, TRUE},
	{0x05, "VIDEOCAPTURE",   hd_videocap_menu_p, TRUE},
	{0x06, "VIDEOOUT",       hd_videoout_menu_p, TRUE},
	{0x07, "VIDEOPROCESS",   hd_videoproc_menu_p, TRUE},
	{0x08, "VIDEOENC",       hd_videoenc_menu_p, TRUE},
	{0x09, "VIDEODEC",       hd_videodec_menu_p, TRUE},
	{0x0A, "GFX",            NULL,               TRUE},
	{0x0B, "VENDOR",         NULL,    			 TRUE},
	{0x0C, "COMMON",         hd_common_menu_p,   TRUE},
	{0x0D, "UTIL",           hd_util_menu_p,       TRUE},
	{0x0E, "DEBUG",          hd_debug_menu_p,    TRUE},
	// escape muse be last
	{HD_DEBUG_MENU_ID_LAST,  "", NULL,           FALSE},
};

HD_RESULT hd_debug_init(void)
{
	return HD_OK;
}

HD_RESULT hd_debug_uninit(void)
{
	return HD_OK;
}

/********************************************************************
	DEBUG MENU FUNCTIONS
********************************************************************/
void hd_debug_menu_print_p(HD_DBG_MENU *p_menu, const char *p_title)
{
	DBG_PRINT("\n==============================");
	DBG_PRINT("\n %s", p_title);
	DBG_PRINT("\n------------------------------");

	while (p_menu->menu_id != HD_DEBUG_MENU_ID_LAST) {
		if (p_menu->b_enable) {
			DBG_PRINT("\n %02d : %s", p_menu->menu_id, p_menu->p_name);
		}
		p_menu++;
	}

	DBG_PRINT("\n------------------------------");
	DBG_PRINT("\n %02d : %s", HD_DEBUG_MENU_ID_QUIT, "Quit");
	DBG_PRINT("\n %02d : %s", HD_DEBUG_MENU_ID_RETURN, "Return");
	DBG_PRINT("\n------------------------------\n");
}

int hd_debug_menu_exec_p(int menu_id, HD_DBG_MENU *p_menu)
{
	if (menu_id == HD_DEBUG_MENU_ID_RETURN) {
		return 0; // return 0 for return upper menu
	}

	if (menu_id == HD_DEBUG_MENU_ID_QUIT) {
		return -1; // return -1 to notify upper layer to quit
	}

	while (p_menu->menu_id != HD_DEBUG_MENU_ID_LAST) {
		if (p_menu->menu_id == menu_id && p_menu->b_enable) {
			DBG_PRINT("Run: %02d : %s\r\n", p_menu->menu_id, p_menu->p_name);
			if (p_menu->p_func) {
				return p_menu->p_func();
			} else {
				DBG_PRINT("null function for menu id = %d\n", menu_id);
				return 0; // just skip
			}
		}
		p_menu++;
	}

	DBG_PRINT("cannot find menu id = %d\n", menu_id);
	return 0;
}

int hd_debug_menu_entry_p(HD_DBG_MENU *p_menu, const char *p_title)
{
	int menu_id = 0;

	do {
		hd_debug_menu_print_p(p_menu, p_title);
		menu_id = (int)hd_read_decimal_key_input("");
		if (hd_debug_menu_exec_p(menu_id, p_menu) == -1) {
			return -1; //quit
		}
	} while (menu_id != HD_DEBUG_MENU_ID_RETURN);

	return 0;
}

/********************************************************************
	DEBUG MENU IMPLEMENTATION
********************************************************************/
static int debug_menu_err_enable_p(void)
{
	g_hd_mask_err = (unsigned int) -1;
	return 0;
}

static int debug_menu_err_disable_p(void)
{
	g_hd_mask_err = (unsigned int)0;
	return 0;
}

static int debug_menu_wrn_enable_p(void)
{
	g_hd_mask_wrn = (unsigned int) -1;
	return 0;
}

static int debug_menu_wrn_disable_p(void)
{
	g_hd_mask_wrn = (unsigned int)0;
	return 0;
}

static int debug_menu_ind_enable_p(void)
{
	g_hd_mask_ind = (unsigned int) -1;
	return 0;
}

static int debug_menu_ind_disable_p(void)
{
	g_hd_mask_ind = (unsigned int)0;
	return 0;
}

static int debug_menu_msg_enable_p(void)
{
	g_hd_mask_msg = (unsigned int) -1;
	return 0;
}

static int debug_menu_msg_disable_p(void)
{
	g_hd_mask_msg = (unsigned int)0 | HD_LOG_MASK_DEBUG;
	return 0;
}

static int debug_menu_func_enable_p(void)
{
	g_hd_mask_func = (unsigned int) -1;
	return 0;
}

static int debug_menu_func_disable_p(void)
{
	g_hd_mask_func = (unsigned int)0;
	return 0;
}

static int debug_menu_module_enable_p(unsigned int mask)
{
	g_hd_mask_err |= mask;
	g_hd_mask_wrn |= mask;
	g_hd_mask_ind |= mask;
	g_hd_mask_msg |= mask;
	g_hd_mask_func |= mask;
	return 0;
}

static int debug_menu_module_disable_p(unsigned int mask)
{
	g_hd_mask_err &= ~mask;
	g_hd_mask_wrn &= ~mask;
	g_hd_mask_ind &= ~mask;
	g_hd_mask_msg &= ~mask;
	g_hd_mask_func &= ~mask;
	return 0;
}

static int debug_menu_audiocapture_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_AUDIOCAPTURE);
}

static int debug_menu_audiocapture_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_AUDIOCAPTURE);
}

static int debug_menu_audioout_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_AUDIOOUT);
}

static int debug_menu_audioout_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_AUDIOOUT);
}

static int debug_menu_audioenc_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_AUDIOENC);
}

static int debug_menu_audioenc_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_AUDIOENC);
}

static int debug_menu_audiodec_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_AUDIODEC);
}

static int debug_menu_audiodec_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_AUDIODEC);
}

static HD_RESULT debug_videocapture_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_VIDEOCAPTURE);
}

static HD_RESULT debug_videocapture_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_VIDEOCAPTURE);
}

static int debug_menu_videoout_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_VIDEOOUT);
}

static int debug_menu_videoout_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_VIDEOOUT);
}

static int debug_menu_videoprocess_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_VIDEOPROCESS);
}

static int debug_menu_videoprocess_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_VIDEOPROCESS);
}

static int debug_menu_videoenc_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_VIDEOENC);
}

static int debug_menu_videoenc_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_VIDEOENC);
}

static int debug_menu_videodec_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_VIDEODEC);
}

static int debug_menu_videodec_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_VIDEODEC);
}

static int debug_menu_gfx_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_GFX);
}

static int debug_menu_gfx_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_GFX);
}

static int debug_menu_common_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_COMMON);
}

static int debug_menu_common_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_COMMON);
}

static int debug_menu_util_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_UTIL);
}

static int debug_menu_util_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_UTIL);
}

static int debug_menu_debug_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_UTIL);
}

static int debug_menu_debug_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_UTIL);
}

static HD_DBG_MENU debug_menu_p[] = {
	{0x01, "All ERR mask enable",            debug_menu_err_enable_p,              TRUE},
	{0x02, "All ERR mask disable",           debug_menu_err_disable_p,             TRUE},
	{0x03, "All WRN mask enable",            debug_menu_wrn_enable_p,              TRUE},
	{0x04, "All WRN mask disable",           debug_menu_wrn_disable_p,             TRUE},
	{0x05, "All IND mask enable",            debug_menu_ind_enable_p,              TRUE},
	{0x06, "All IND mask disable",           debug_menu_ind_disable_p,             TRUE},
	{0x07, "All MSG mask enable",            debug_menu_msg_enable_p,              TRUE},
	{0x08, "All MSG mask disable",           debug_menu_msg_disable_p,             TRUE},
	{0x09, "All FUNC mask enable",           debug_menu_func_enable_p,             TRUE},
	{0x0A, "All FUNC mask disable",          debug_menu_func_disable_p,            TRUE},
	{0x0B, "All AUDIOCAPTURE mask enable",   debug_menu_audiocapture_enable_p,     TRUE},
	{0x0C, "All AUDIOCAPTURE mask disable",  debug_menu_audiocapture_disable_p,    TRUE},
	{0x0D, "All AUDIOOUT mask enable",       debug_menu_audioout_enable_p,         TRUE},
	{0x0E, "All AUDIOOUT mask disable",      debug_menu_audioout_disable_p,        TRUE},
	{0x0F, "All AUDIOENC mask enable",       debug_menu_audioenc_enable_p,         TRUE},
	{0x10, "All AUDIOENC mask disable",      debug_menu_audioenc_disable_p,        TRUE},
	{0x11, "All AUDIODEC mask enable",       debug_menu_audiodec_enable_p,         TRUE},
	{0x12, "All AUDIODEC mask disable",      debug_menu_audiodec_disable_p,        TRUE},
	{0x13, "All VIDEOCAPTURE mask enable",   debug_videocapture_enable_p,          TRUE},
	{0x14, "All VIDEOCAPTURE mask disable",  debug_videocapture_disable_p,         TRUE},
	{0x15, "All VIDEOOUT mask enable",       debug_menu_videoout_enable_p,         TRUE},
	{0x16, "All VIDEOOUT mask disable",      debug_menu_videoout_disable_p,        TRUE},
	{0x17, "All VIDEOPROCESS  mask enable",  debug_menu_videoprocess_enable_p,     TRUE},
	{0x18, "All VIDEOPROCESS  mask disable", debug_menu_videoprocess_disable_p,    TRUE},
	{0x19, "All VIDEOENC mask enable",       debug_menu_videoenc_enable_p,         TRUE},
	{0x1A, "All VIDEOENC mask disable",      debug_menu_videoenc_disable_p,        TRUE},
	{0x1B, "All VIDEODEC mask enable",       debug_menu_videodec_enable_p,         TRUE},
	{0x1C, "All VIDEODEC mask disable",      debug_menu_videodec_disable_p,        TRUE},
	{0x1D, "All GFX mask enable",            debug_menu_gfx_enable_p,              TRUE},
	{0x1E, "All GFX mask disable",           debug_menu_gfx_disable_p,             TRUE},
	{0x1F, "All COMMON mask enable",         debug_menu_common_enable_p,           TRUE},
	{0x20, "All COMMON mask disable",        debug_menu_common_disable_p,          TRUE},
	{0x21, "All UTIL mask enable",           debug_menu_util_enable_p,             TRUE},
	{0x22, "All UTIL mask disable",          debug_menu_util_disable_p,            TRUE},
	{0x23, "All DEBUG mask enable",          debug_menu_debug_enable_p,            TRUE},
	{0x24, "All DEBUG mask disable",         debug_menu_debug_disable_p,           TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

HD_RESULT hd_debug_menu_p(void)
{
	return hd_debug_menu_entry_p(debug_menu_p, "DEBUG");
}

HD_RESULT hd_debug_run_menu(void)
{
	hd_debug_menu_entry_p(root_menu_p, "HDAL");
	return HD_OK;
}

/********************************************************************
	DEBUG GET IMPLEMENTATION
********************************************************************/
static HD_RESULT hd_debug_get_err_mask_p(void *p_data)
{
	*(unsigned int *)p_data = g_hd_mask_err;
	return HD_OK;
}

static HD_RESULT hd_debug_get_wrn_mask_p(void *p_data)
{
	*(unsigned int *)p_data = g_hd_mask_wrn;
	return HD_OK;
}

static HD_RESULT hd_debug_get_ind_mask_p(void *p_data)
{
	*(unsigned int *)p_data = g_hd_mask_ind;
	return HD_OK;
}

static HD_RESULT hd_debug_get_msg_mask_p(void *p_data)
{
	*(unsigned int *)p_data = g_hd_mask_msg;
	return HD_OK;
}

static HD_RESULT hd_debug_get_func_mask_p(void *p_data)
{
	*(unsigned int *)p_data = g_hd_mask_func;
	return HD_OK;
}

static HD_DBG_CMD_DESC cmd_get_tbl[] = {
	{HD_DEBUG_PARAM_ERR_MASK, hd_debug_get_err_mask_p},
	{HD_DEBUG_PARAM_WRN_MASK, hd_debug_get_wrn_mask_p},
	{HD_DEBUG_PARAM_IND_MASK, hd_debug_get_ind_mask_p},
	{HD_DEBUG_PARAM_MSG_MASK, hd_debug_get_msg_mask_p},
	{HD_DEBUG_PARAM_FUNC_MASK, hd_debug_get_func_mask_p},
};

HD_RESULT hd_debug_get(HD_DEBUG_PARAM_ID idx, void *p_data)
{
	int i, n = sizeof(cmd_get_tbl) / sizeof(cmd_get_tbl[0]);
	for (i = 0; i < n; i++) {
		if (cmd_get_tbl[i].idx == idx) {
			return cmd_get_tbl[i].p_func(p_data);
		}
	}
	DBG_PRINT("not support id = %d\r\n", idx);
	return HD_ERR_NOT_SUPPORT;
}

/********************************************************************
	DEBUG SET IMPLEMENTATION
********************************************************************/
static HD_RESULT hd_debug_set_err_mask_p(void *p_data)
{
	unsigned int mask = *(unsigned int *)p_data;
	g_hd_mask_err = mask;
	return HD_OK;
}

static HD_RESULT hd_debug_set_wrn_mask_p(void *p_data)
{
	unsigned int mask = *(unsigned int *)p_data;
	g_hd_mask_wrn = mask;
	return HD_OK;
}

static HD_RESULT hd_debug_set_ind_mask_p(void *p_data)
{
	unsigned int mask = *(unsigned int *)p_data;
	g_hd_mask_ind = mask;
	return HD_OK;
}

static HD_RESULT hd_debug_set_msg_mask_p(void *p_data)
{
	unsigned int mask = *(unsigned int *)p_data;
	g_hd_mask_msg = mask;
	return HD_OK;
}

static HD_RESULT hd_debug_set_func_mask_p(void *p_data)
{
	unsigned int mask = *(unsigned int *)p_data;
	g_hd_mask_func = mask;
	return HD_OK;
}

static HD_DBG_CMD_DESC cmd_set_tbl[] = {
	{HD_DEBUG_PARAM_ERR_MASK, hd_debug_set_err_mask_p},
	{HD_DEBUG_PARAM_WRN_MASK, hd_debug_set_wrn_mask_p},
	{HD_DEBUG_PARAM_IND_MASK, hd_debug_set_ind_mask_p},
	{HD_DEBUG_PARAM_MSG_MASK, hd_debug_set_msg_mask_p},
	{HD_DEBUG_PARAM_FUNC_MASK, hd_debug_set_func_mask_p},
};

HD_RESULT hd_debug_set(HD_DEBUG_PARAM_ID idx, void *p_data)
{
	int i, n = sizeof(cmd_set_tbl) / sizeof(cmd_set_tbl[0]);
	for (i = 0; i < n; i++) {
		if (cmd_set_tbl[i].idx == idx) {
			return cmd_set_tbl[i].p_func(p_data);
		}
	}
	DBG_PRINT("not support id = %d\r\n", idx);
	return HD_ERR_NOT_SUPPORT;
}


