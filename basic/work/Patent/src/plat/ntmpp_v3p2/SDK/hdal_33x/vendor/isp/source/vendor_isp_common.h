/**
	@brief Header file of vendor isp common definition.
	This file contains the functions which is related to common definition.

	@file vendor_isp_common.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_ISP_COMMON_H_
#define _VENDOR_ISP_COMMON_H_

/********************************************************************
INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "hd_util.h"
#include "vendor_ad.h"
#include "vendor_video.h"

/********************************************************************
MACRO CONSTANT DEFINITIONS
********************************************************************/
#define VENDOR_OUTID(id)    	(id - HD_OUT_BASE)

#define EM_CHIP_OFFSET      20
#define EM_CHIP_BITMAP      0x00F00000

#define EM_ENGINE_OFFSET    12
#define EM_ENGINE_BITMAP    0x000FF000

#define EM_MINOR_OFFSET     0
#define EM_MINOR_BITMAP     0x00000FFF

#define ENTITY_FD_CHIP(fd)      ((fd & EM_CHIP_BITMAP) >> EM_CHIP_OFFSET)
#define ENTITY_FD_ENGINE(fd)    ((fd & EM_ENGINE_BITMAP) >> EM_ENGINE_OFFSET)
#define ENTITY_FD_MINOR(fd)     ((fd & EM_MINOR_BITMAP) >> EM_MINOR_OFFSET)

#define FLOW_NR_BIT           (20)
#define FLOW_NR_FLAG          (0x1 << FLOW_NR_BIT)
extern void hdal_flow_log_p(unsigned int flag, const char *msg_with_format, ...) __attribute__((format(printf, 2, 3)));

#define _HD_MODULE_PRINT_FLOW(flag, module, fmtstr, args...)  do { \
															hdal_flow_log_p(flag, fmtstr, ##args); \
													} while(0)
#define VENDOR_ISP_FLOW(fmtstr, args...) _HD_MODULE_PRINT_FLOW(FLOW_NR_FLAG, NR, fmtstr, ##args)
#define VENDOR_ISP_ERROR(fmtstr, args...) _HD_MODULE_PRINT(DEBUG, ERR, fmtstr, ##args)

/********************************************************************
MACRO FUNCTION DEFINITIONS
********************************************************************/

/********************************************************************
TYPE DEFINITION
********************************************************************/

typedef struct _VENDOR_VIDEO_PARAM {
	HD_PATH_ID	path_id;
	VENDOR_VIDEO_PARAM_DI_GMM di_gmm;		///< for VPE316
	VENDOR_VIDEO_PARAM_DI_TMNR di_tmnr; 	///< for VPE316
	VENDOR_VIDEO_PARAM_DEI_INTERLACE di_interlace; ///< for VPE316
	VENDOR_VIDEO_PARAM_MRNR vpe_mrnr; 		///< for VPE536
	VENDOR_VIDEO_PARAM_TMNR_EXT vpe_dn;	///< for VPE536
	VENDOR_VIDEO_PARAM_TMNR_EXT enc_dn;  	///< for ENCODER
	VENDOR_VIDEO_PARAM_SHARP sharp;
	VENDOR_VIDEO_PARAM_MASK mask;
	VENDOR_VIDEO_PARAM_LPM lpm;
	VENDOR_VIDEO_PARAM_RND rnd;
	VENDOR_VIDEO_PARAM_MOTION_AQ aq;
	VENDOR_VIDEO_PARAM_TMNR_STA_DATA tmnr_sta_data;
	VENDOR_VIDEO_PARAM_SCA_SET sca_set;
	VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF tmnr_motion_buf;
	VENDOR_VIDEO_PARAM_LCD_IQ lcd_iq;
	VENDOR_VIDEO_PARAM_YUV_RANGE yuv_range;
	VENDOR_VIDEO_PARAM_EDGE_SMOOTH edge_smooth;
    VENDOR_VIDEO_PARAM_LCD_HUE_SAT lcd_hue;
    VENDOR_VIDEO_PARAM_LCD_YGAMMA lcd_ygamma;
} VENDOR_VIDEO_PARAM;

typedef struct _VENDOR_VIDEO_PARAM_TMNR_MAP_IDX {
	UINT32 map_en;          //enable/disable motion map output
	UINT32 map_idx;         //Select motion map output channel. 0: Y, 1: Cb, 2: Cr
} VENDOR_VIDEO_PARAM_TMNR_MAP_IDX;

typedef struct _VENDOR_VIDEO_PARAM_TMNR_STA_INFO {
	UINT32 blk_no;          //TMNR statistic data block count
} VENDOR_VIDEO_PARAM_TMNR_STA_INFO;

typedef struct _VENDOR_VIDEO_PARAM_TMNR_MTN_BUF{
	UINT32 ddr_id;
	UINTPTR pbuf_addr[2];   //TMNR physical address read & write
	UINT32 pbuf_size;   //TMNR mtn buffer (r/w per fd)
	UINT32 one_full_buf;
} VENDOR_VIDEO_PARAM_TMNR_MTN_BUF;

/********************************************************************
EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

#endif  /* _VENDOR_ISP_COMMON_H_ */
