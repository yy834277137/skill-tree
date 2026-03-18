/**
	@brief Header file of debug internal of library.\n
	This file contains the debug internal functions of library.

	@file debug.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "hd_debug.h"
#include "hd_logger.h"
#define FLOW_DISABLE_BIT       (0)
#define FLOW_ERR_BIT           (1)
#define FLOW_ALL_BIT           (2)


#define FLOW_CAP_BIT           (6)
#define FLOW_VPE_BIT           (7)
#define FLOW_VOUT_BIT          (8)
#define FLOW_DEC_BIT           (9)
#define FLOW_ENC_BIT           (10)
#define FLOW_AUDIO_BIT         (11)

#define FLOW_LV_BIT            (16)
#define FLOW_REC_BIT           (17)
#define FLOW_PB_BIT            (18)
#define FLOW_CLEARWIN_BIT      (19)
#define FLOW_NR_BIT            (20)
#define FLOW_MD_BIT            (21)
#define FLOW_CROP_BIT          (22)
#define FLOW_TRIGGER_BIT       (23)
#define FLOW_GFX_BIT           (24)
#define FLOW_OSG_BIT           (25)
#define FLOW_COMMON_BIT        (26)
#define FLOW_VENDOR_BIT        (27)

#define FLOW_NONCHKMEM_BIT      (28)


#define FLOW_DISABLE_FLAG      (0x1 << FLOW_DISABLE_BIT)
#define FLOW_ERR_FLAG          (0x1 << FLOW_ERR_BIT)
#define FLOW_ALL_FLAG          (0x1 << FLOW_ALL_BIT)

#define FLOW_CAP_FLAG          (0x1 << FLOW_CAP_BIT)
#define FLOW_VPE_FLAG          (0x1 << FLOW_VPE_BIT)
#define FLOW_VOUT_FLAG         (0x1 << FLOW_VOUT_BIT)
#define FLOW_DEC_FLAG          (0x1 << FLOW_DEC_BIT)
#define FLOW_ENC_FLAG          (0x1 << FLOW_ENC_BIT)
#define FLOW_AUDIO_FLAG        (0x1 << FLOW_AUDIO_BIT)

#define FLOW_LV_FLAG           (0x1 << FLOW_LV_BIT)
#define FLOW_REC_FLAG          (0x1 << FLOW_REC_BIT)
#define FLOW_PB_FLAG           (0x1 << FLOW_PB_BIT)
#define FLOW_CLEARWIN_FLAG     (0x1 << FLOW_CLEARWIN_BIT)
#define FLOW_NR_FLAG           (0x1 << FLOW_NR_BIT)
#define FLOW_MD_FLAG           (0x1 << FLOW_MD_BIT)
#define FLOW_CROP_FLAG         (0x1 << FLOW_CROP_BIT)
#define FLOW_TRIGGER_FLAG      (0x1 << FLOW_TRIGGER_BIT)
#define FLOW_GFX_FLAG          (0x1 << FLOW_GFX_BIT)
#define FLOW_OSG_FLAG          (0x1 << FLOW_OSG_BIT)
#define FLOW_COMMON_FLAG       (0x1 << FLOW_COMMON_BIT)
#define FLOW_VENDOR_FLAG       (0x1 << FLOW_VENDOR_BIT)
#define FLOW_NONCHKMEM_FLAG    (0x1 << FLOW_NONCHKMEM_BIT)


#define HD_MODULE_NAME HD_DEBUG
#define DBG_ERR(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _ERR)("\033[1;31m" fmtstr "\033[0m", ##args)
#define DBG_WRN(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _WRN)("\033[1;33m" fmtstr "\033[0m", ##args)
#define DBG_IND(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _IND)(fmtstr, ##args)
#define DBG_DUMP(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _MSG)(fmtstr, ##args)
#define DBG_FUNC_BEGIN(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("BEGIN: " fmtstr, ##args)
#define DBG_FUNC_END(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("END: " fmtstr, ##args)
#define DBG_PRINT(fmtstr, args...) \
	do { \
		extern unsigned int hdal_flow_dbgmode; \
		if (hdal_flow_dbgmode & FLOW_DISABLE_FLAG) \
			break; \
		printf(fmtstr, ##args); \
	} while(0)

typedef struct _HD_DBG_CMD_DESC {
	HD_DEBUG_PARAM_ID idx;
	HD_RESULT(*p_func)(void *p_data);
} HD_DBG_CMD_DESC;

#endif
