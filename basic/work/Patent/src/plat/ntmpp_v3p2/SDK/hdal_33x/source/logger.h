/**
	@brief Header file of logger internal of library.\n
	This file contains the logger internal functions of library.

	@file logger.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "hd_debug.h"
#include "hd_logger.h"
#include "debug.h"
/**
 *  Internal macros to make the module's printf macro easier.
 *  Don't use it outside.
 */
extern void hdal_flow_log_p(unsigned int flag, const char *msg_with_format, ...) __attribute__((format(printf, 2, 3)));

#define _HD_MODULE_PRINT_FLOW(flag, module, fmtstr, args...)  do { \
															hdal_flow_log_p(flag, fmtstr, ##args); \
													} while(0)

#define HD_AUDIOCAPTURE_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW(FLOW_AUDIO_FLAG, AUDIOCAPTURE, fmtstr, ##args)
#define HD_AUDIOOUT_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW(FLOW_AUDIO_FLAG, AUDIOOUT, fmtstr, ##args)
#define HD_AUDIOENC_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW(FLOW_AUDIO_FLAG, AUDIOENC, fmtstr, ##args)
#define HD_AUDIODEC_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW(FLOW_AUDIO_FLAG, AUDIODEC, fmtstr, ##args)
#define HD_VIDEOCAP_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW((FLOW_LV_FLAG|FLOW_REC_FLAG|FLOW_CAP_FLAG), VIDEOCAPTURE, fmtstr, ##args)
#define HD_VIDEOOUT_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW((FLOW_LV_FLAG|FLOW_PB_FLAG|FLOW_VOUT_FLAG|FLOW_CLEARWIN_FLAG), VIDEOOUT, fmtstr, ##args)
#define HD_VIDEOPROC_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW((FLOW_LV_FLAG|FLOW_PB_FLAG|FLOW_VPE_FLAG), VIDEOPROCESS, fmtstr, ##args)
#define HD_VIDEOENC_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW((FLOW_ENC_FLAG|FLOW_REC_FLAG), VIDEOENC, fmtstr, ##args)
#define HD_VIDEODEC_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW((FLOW_DEC_FLAG|FLOW_PB_FLAG), VIDEODEC, fmtstr, ##args)
#define HD_GFX_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW(FLOW_GFX_FLAG, GFX, fmtstr, ##args)
#define HD_OSG_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW(FLOW_OSG_FLAG, OSG, fmtstr, ##args)
#define HD_COMMON_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW(FLOW_COMMON_FLAG, COMMON, fmtstr, ##args)

#define HD_TRIGGER_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW(FLOW_TRIGGER_FLAG, USR, fmtstr, ##args)
#endif
