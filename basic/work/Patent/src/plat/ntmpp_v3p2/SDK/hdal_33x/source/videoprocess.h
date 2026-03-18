/**
	@brief Header file of video process internal of library.\n
	This file contains the video process internal functions of library.

	@file videoprocess.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VIDEOPROCESS_H_
#define _VIDEOPROCESS_H_

#ifdef  __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "bind.h"
#include "hd_debug.h"
#include "hd_logger.h"
#include "hd_videoprocess.h"
//#include "vendor_video.h"

/*-----------------------------------------------------------------------------*/
/* External Constant Definitions                                               */
/*-----------------------------------------------------------------------------*/
#define VDOPROC_DEV   				1024	///< first 512 for VPE, last 512 for VPE Lite
#define VODPROC_INPUT_PORT			1  		///< Indicate max input port value
#define VODPROC_OUTPUT_PORT			12 		///< Indicate max output port value
#define VDOPROC_DEVID(self_id)		(self_id - HD_DAL_VIDEOPROC_BASE)
#define VDOPROC_INID(id)     		(id - HD_IN_BASE)
#define VDOPROC_OUTID(id)    		(id - HD_OUT_BASE)
#define HD_VIDEOPROC_PATTERN_NUM	5  ///< Indicate max pattern number
#define HD_VIDEOPROC_MASK_NUM		8  ///< Indicate max mask number

// sub-yuv ratio threshold default
#define VDOPROC_SUB_RATIO_THLD_NUMER   2
#define VDOPROC_SUB_RATIO_THLD_DENOM   3

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _HD_VDOPROC_PARAM {
	HD_VIDEOPROC_SYSINFO sysinfo[HD_DAL_VIDEOPROC_COUNT];
	HD_VIDEOPROC_IN in[HD_DAL_VIDEOPROC_COUNT];
	HD_VIDEOPROC_OUT out[HD_DAL_VIDEOPROC_COUNT];
	HD_VIDEOPROC_CTRL ctrl[HD_DAL_VIDEOPROC_COUNT];
} HD_VDOPROC_PARAM;

/*-----------------------------------------------------------------------------*/
/* Internal Constant Definitions                                               */
/*-----------------------------------------------------------------------------*/
#define SET_BIT(bmp, bit)		do{(bmp) |= (bit);} while(0)
#define CLR_BIT(bmp, bit)    	do{(bmp) &= (~(bit));} while(0)
#define IS_BIT(bmp, bit)        ((bmp) & (bit))
#define MK_BIT(cond, bmp, bit)	if (cond) {SET_BIT(bmp, bit);} else {CLR_BIT(bmp, bit);}
#define GET_VPROC_PATHID(self_id, in_id, out_id)	(HD_PATH_ID)((self_id & 0xffff) << 16) | ((in_id & 0xff) << 8) | (out_id & 0xff)

/*-----------------------------------------------------------------------------*/
/* Internal Types Declarations                                                 */
/*-----------------------------------------------------------------------------*/
typedef struct _VDOPROC_PRIV {
	INT32 fd;
	INT32 di_fd;
	//UINT32 queue_handle;
	uintptr_t queue_handle;
	HD_PATH_ID_STATE push_state;
	HD_PATH_ID_STATE pull_state;
} VDOPROC_PRIV;

typedef struct _VDOPROC_PATTERN {
	HD_VIDEOPROC_PATTERN_IMG pattern_info[HD_VIDEOPROC_PATTERN_NUM];
	HD_VIDEOPROC_PATTERN_SELECT pattern_select;
} VDOPROC_PATTERN;

typedef struct _VDOPROC_VPE1TO4_INFO {
	UINT32 out_num; //from 1 to 4
	HD_URECT dst_rect[4];		///< source croping parameters
} VDOPROC_VPE1TO4_INFO;

typedef struct _VDOPROC_SLICE_MODE {
	BOOL enable;
	unsigned int slice_height;	//VPE support 4K output without column mode
} VDOPROC_SLICE_MODE;

typedef struct _VDOPROC_PARAM {
	HD_URECT src_rect;		///< source croping parameters
	HD_URECT dst_rect;		///< destination croping parameters
	HD_URECT psr_crop_rect;	///< psr croping parameters
	HD_VIDEO_PXLFMT src_fmt;///< source pixel format

	HD_DIM src_bg_dim;	///< source backgroud dimension
	HD_DIM dst_bg_dim;	///< destination backgroud dimension
	HD_DIM psr_crop_bg_dim;	///< psr backgroud dimension
	HD_VIDEO_PXLFMT dst_fmt;///< destination pixel format
	HD_VIDEO_DIR direction;	///< flip and rotate setting
	BOOL di_enable;			///< 0(disabled), 1(enbaled)
	BOOL dn_enable;  		///< < 0(disabled), 1(enbaled)
	BOOL sharp_enable;  	///< 0(disabled), 1(enabled)
	BOOL src_rect_enable;  		///< 0(disabled), 1(enbaled)
	BOOL psr_crop_enable;  		///< 0(disabled), 1(enbaled)
	VDOPROC_PATTERN pattern;
	HD_OUT_POOL out_pool;	///< for pool info setting
	VDOPROC_PRIV priv;  ///< internal private data, do not use it
	HD_VIDEOPROC_VPEMASK_ONEINFO vpeinfo;
	VDOPROC_VPE1TO4_INFO vpe1to4_info;
	VDOPROC_SLICE_MODE slice_mode;
	CHAR push_in_used;
	BOOL uv_swap;           ///< uv swap for yuv from decoder (1: enable, 0: disable)
	BOOL tmnr_en;           ///< vendor tmnr function for checking crop wh(1: enable, 0: disable)
	BOOL use_vpe_func;		///< check if using other vpe functoin
	BOOL src_rect_enable_at_start;	///< 0(disabled), 1(enbaled) for next start
	BOOL psr_crop_enable_at_start;	///< 0(disabled), 1(enbaled) for next start
} VDOPROC_PARAM;

typedef struct _VDOPROC_INTL_PARAM {
	HD_URECT *p_src_rect;		///< pointer of src_rect
	HD_URECT *p_dst_rect;		///< pointer of dst_rect
	HD_URECT *p_psr_crop_rect;	///< pointer of psr_crop_rect
	HD_DIM *p_src_bg_dim;		///< pointer of src_bg_dim
	HD_DIM *p_dst_bg_dim;		///< pointer of dst_bg_dim
	HD_DIM *p_psr_crop_bg_dim;	///< pointer of psr_crop_bg_dim
	HD_VIDEO_PXLFMT *p_src_fmt; ///< pointer of dst_fmt
	HD_VIDEO_PXLFMT *p_dst_fmt; ///< pointer of dst_fmt
	HD_VIDEO_DIR *p_direction;	///< pointer of direction
	VDOPROC_SLICE_MODE *p_slice_mode; ///< pointer of slice mode
	BOOL *p_di_enable;			///< pointer of di_enable
	BOOL *p_dn_enable;  		///< pointer of dn_enable
	BOOL *p_sharp_enable;  		///< pointer of sharp_enable
	BOOL *p_src_rect_enable;  		///< pointer of src_rect_enable
	BOOL *p_psr_crop_enable;  		///< pointer of psr_crop_enable
	UINT linefd;
	HD_OUT_POOL *p_out_pool;
	BOOL is_black_init;
	UINT32 sub_ratio_thld;       ///< user sub-yuv ratio threshold
	BOOL dc_enable;              ///< dewarp enable
	BOOL use_vpe_func;		///< check if using other vpe functoin
} VDOPROC_INTL_PARAM;

typedef struct _VDOPROC_HW_SPEC_INFO {
	/* set input */
	HD_VIDEO_DIR in_direction;	///< direction for input
	INT is_interlace;			///< interlace for input
	HD_VIDEO_PXLFMT in_src_fmt; ///< src_fmt for input
	HD_VIDEO_PXLFMT in_dst_fmt; ///< dst_fmt for input

	/* get output */
	UINT8 out_src_fmt_is_valid;	///< out_src_fmt_is_valid for ouput
	UINT8 out_dst_fmt_is_valid;	///< out_dst_fmt_is_valid for ouput
	HD_URECT out_src_rect;		///< src_rect for output
	HD_DIM out_src_min_dim;		///< out_src_min_dim for output
	HD_DIM out_src_max_dim;		///< out_src_max_dim for output
	HD_DIM out_src_bg_dim;		///< out_src_bg_dim for output
	HD_URECT out_dst_rect;		///< dst_rect for output
	HD_URECT out_dst_hole;		///< dst_hole for output
	HD_DIM out_dst_min_dim;		///< out_dst_min_dim for output
	HD_DIM out_dst_max_dim;		///< out_dst_max_dim for output
	HD_DIM out_dst_bg_dim;		///< out_dst_bg_dim for output
} VDOPROC_HW_SPEC_INFO;

/*-----------------------------------------------------------------------------*/
/* External Function Prototype                                                 */
/*-----------------------------------------------------------------------------*/
HD_RESULT videoproc_check_is_needed_param_set_p(HD_PATH_ID path_id, HD_VIDEO_PXLFMT in_fmt, HD_VIDEO_PXLFMT out_fmt, HD_DIM *p_src_bg);
HD_RESULT videoproc_new_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame);
HD_RESULT videoproc_push_in_buf_p(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame,
				  HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms);
HD_RESULT videoproc_release_in_buf_p(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame);
HD_RESULT videoproc_pull_out_buf_p(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms);
HD_RESULT videoproc_release_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame);
VDOPROC_INTL_PARAM *videoproc_get_param_p(HD_DAL self_id, UINT32 out_subid);
HD_RESULT videoproc_set_param_p(HD_DAL self_id, UINT32 out_subid, HD_VIDEOPROC_PARAM_ID id);
HD_RESULT videoproc_set_crop_p(HD_DAL self_id, HD_IO out_id, INT param_id);
HD_RESULT videoproc_set_pattern_p(HD_DAL self_id,  HD_IO in_id, HD_VIDEOPROC_PATTERN_IMG *p_pattern);
HD_RESULT videoproc_get_pattern_p(HD_DAL self_id,  HD_IO in_id, HD_VIDEOPROC_PATTERN_IMG *p_pattern);
HD_RESULT videoproc_enable_pattern_p(HD_DAL self_id,  HD_IO out_id, HD_VIDEOPROC_PATTERN_SELECT *p_pat_sel, HD_URECT *p_dst_rect);
HD_RESULT videoproc_get_pattern_sel_p(HD_DAL self_id,  HD_IO out_id, HD_VIDEOPROC_PATTERN_SELECT *p_pat_sel);
HD_RESULT videoproc_set_sca_buf_info(HD_DAL self_id,  HD_IO out_id, HD_VIDEOPROC_SCA_BUF_INFO *p_pat_sel);
HD_RESULT videoproc_get_sca_buf_info(HD_DAL self_id,  HD_IO out_id, HD_VIDEOPROC_SCA_BUF_INFO *p_pat_sel);
HD_RESULT videoproc_check_path_id_p(HD_PATH_ID path_id);
HD_RESULT videoproc_get_vpe_align_p(VDOPROC_HW_SPEC_INFO *p_hw_spec_info, INT is_vpe);
HD_RESULT videoproc_init_path_p(HD_PATH_ID path_id);
HD_RESULT videoproc_uninit_path_p(HD_PATH_ID path_id);
HD_RESULT videoproc_init_p(void);
HD_RESULT videoproc_uninit_p(void);
HD_RESULT videoproc_set_vpemask_p(HD_DAL self_id,  HD_IO in_id, HD_VIDEOPROC_VPEMASK_ONEINFO *p_pat_sel);
HD_RESULT videoproc_get_vpemask_p(HD_DAL self_id,  HD_IO in_id, HD_VIDEOPROC_VPEMASK_ONEINFO *p_pat_sel);
HD_RESULT videoproc_close_vpemask(HD_PATH_ID path_id);
HD_RESULT videoproc_check_rotate_and_crop(VDOPROC_PARAM *vdoproc_param);
HD_RESULT videoproc_check_pip_and_psr(HD_PATH_ID path_id, VDOPROC_PARAM *vdoproc_param, HD_VIDEOPROC_CROP *p_crop);
HD_RESULT videoproc_set_vproc_align_down_dim(HD_PATH_ID path_id, HD_VIDEO_PXLFMT in_fmt, HD_VIDEO_PXLFMT out_fmt, HD_URECT in_rect, HD_URECT *p_out_rect);
HD_RESULT videoproc_check_pattern(HD_PATH_ID path_id, VDOPROC_PARAM *vdoproc_param);
HD_RESULT videoproc_push_list_p(HD_PATH_ID *p_path_id, HD_VIDEO_FRAME *p_in_video_frame,
				HD_VIDEO_FRAME *p_out_video_frame, UINT32 num, INT32 wait_ms, HD_DIM *p_llc_dim);
HD_RESULT videoproc_videoproc_close_uv_swap(HD_PATH_ID path_id);


BOOL videoproc_check_addr(UINTPTR address);

#ifdef  __cplusplus
}
#endif
#endif
