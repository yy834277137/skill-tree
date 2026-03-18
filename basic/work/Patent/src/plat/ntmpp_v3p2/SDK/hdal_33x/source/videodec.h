/**
	@brief Header file of video deocde internal of library.\n
	This file contains the video deocde internal functions of library.

	@file videodec.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VIDEODEC_H_
#define _VIDEODEC_H_

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
#include "hd_videodec.h"

/*-----------------------------------------------------------------------------*/
/* External Constant Definitions                                               */
/*-----------------------------------------------------------------------------*/

#define VDODEC_DEV   			4    ///< Indicate max out dev
#define VDODEC_INPUT_PORT		96  ///< Indicate max input port value
#define VDODEC_OUTPUT_PORT		96  ///< Indicate max output port value
#define VDODEC_DEVID(self_id)	(self_id - HD_DAL_VIDEODEC_BASE)
#define VDODEC_INID(id)     	(id - HD_IN_BASE)
#define VDODEC_OUTID(id)    	(id - HD_OUT_BASE)

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _HD_VDODEC_PARAM {
	HD_VIDEODEC_SYSCAPS sysinfo[VDODEC_DEV];
	HD_VIDEODEC_PATH_CONFIG config[VDODEC_DEV];
} HD_VDODEC_PARAM;

/*-----------------------------------------------------------------------------*/
/* Internal Constant Definitions                                               */
/*-----------------------------------------------------------------------------*/
#define GET_VDEC_PATHID(self_id, in_id, out_id)	(HD_PATH_ID)((self_id & 0xffff) << 16) | ((in_id & 0xff) << 8) | (out_id & 0xff)

/*-----------------------------------------------------------------------------*/
/* Internal Types Declarations                                                 */
/*-----------------------------------------------------------------------------*/
typedef struct _VDODEC_PRIV {
	INT32 fd;
	//UINT32 queue_handle;
	uintptr_t queue_handle;
	HD_PATH_ID_STATE push_state;
	HD_PATH_ID_STATE pull_state;
} VDODEC_PRIV;

typedef struct _VDODEC_MAXMEM {
	UINT32 max_width;  		///< for video data, specify its max width
	UINT32 max_height; 		///< for video data, specify its max height
	UINT32 max_fps;    		///< for video data, specify its max framerate
	UINT32 max_bitrate;		///< for video data, specify its max bitrate
	UINT32 bs_counts;		///< for video data, specify its bitstream buffer count
	UINT32 codec_type;		///< for video data, specify its codec type
	UINT32 max_ref_num;		///< for video data, specify its max reference number
	UINT32 ddr_id;			///< DDR ID for datain out pool
	UINT32 max_bs_size;     ///< Window size for datain out pool(variable length), 0 means AUTO
} VDODEC_MAXMEM;

typedef struct _VDODEC_STATUS {
	UINT32 left_frames;			///< number of frames to be decoded
	UINT32 reserved_ref_frame;	///< reserved reference frame when unbind unit
	UINT32 done_frames;		   	///< number of decoded frames
	UINT32 dti_buf_cnt;		   	///< datain available buffer count
} VDODEC_STATUS;

typedef struct _VDODEC_PARAM {
	struct {
		VDODEC_MAXMEM max_mem;	///< for max memory setting
		HD_OUT_POOL out_pool;	///< for pool info setting
		VDODEC_STATUS status;	///< number of frames to be decoded
		VDODEC_PRIV priv;  		///< internal private data, do not use it
		CHAR        push_in_used; ///< for checking push_in is used
	} port[VDODEC_OUTPUT_PORT];
} VDODEC_PARAM;

typedef enum _VDODEC_TX_STATE {
	DEC_TX_NOTHING_DONE,
	DEC_TX_BLACK_BS_DONE,
	DEC_TX_USR_BS_DONE,
	ENUM_DUMMY4WORD(VDODEC_TX_STATE),
} VDODEC_TX_STATE;

typedef struct _VDODEC_INTL_PARAM {
	VDODEC_MAXMEM *max_mem;		///< for max memory setting
	HD_OUT_POOL *p_out_pool;	///< pointer of out_pool
	VDODEC_STATUS *p_status;	///< pointer of status
	UINT linefd;
	VDODEC_TX_STATE tx_state;
	UINT32 reserved_ref_flag;   ///< handle_dec_tx_block refer it, (0: don't tx, 1: tx)
	UINT32 sub_yuv_ratio;       ///< sub-yuv ratio setting by user
	UINT32 qp_value;            ///< qp configuration by user
	UINT32 first_out;           ///< decode first output mode (compress / uncompress)
	UINT32 extra_out;           ///< decode extra output mode (compress / uncompress)
	UINT32 status_next_path_id; ///< next pathid. ex: dec->vpe->enc, the value is vpe's pathid

} VDODEC_INTL_PARAM;

typedef struct _VDODEC_DEV_PARAM {
	VDODEC_INTL_PARAM port[VDODEC_OUTPUT_PORT];
} VDODEC_DEV_PARAM;

typedef struct _VDODEC_H265_TILE_BUF {
	UINTPTR addr_pa;             ///< physical address of H265 tile buffer
	UINT32  buf_size;            ///< size of H265 tile buffer
	INT32   ddr_id;              ///< ddr id of H265 tile buffer
} VDODEC_H265_TILE_BUF;


typedef struct _VDODEC_CTRL_PARAM {
	VDODEC_H265_TILE_BUF h26x_tile_buf;
} VDODEC_CTRL_PARAM;

/*-----------------------------------------------------------------------------*/
/* External Function Prototype                                                 */
/*-----------------------------------------------------------------------------*/
INT validate_dec_in_dim(HD_DIM dim, HD_VIDEO_CODEC codec_type);
INT verify_param_PATH_CONFIG(VDODEC_MAXMEM *p_maxmem);
HD_RESULT videodec_new_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_VIDEODEC_BS *p_video_bitstream);
HD_RESULT videodec_push_in_buf_p(HD_DAL self_id, HD_IO in_id, HD_VIDEODEC_BS *p_video_bitstream,
				 HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms);
HD_RESULT videodec_release_in_buf_p(HD_DAL self_id, HD_IO out_id, HD_VIDEODEC_BS *p_video_bitstream);
HD_RESULT videodec_pull_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms);
HD_RESULT videodec_release_out_buf_p(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame);
HD_RESULT videodec_get_status_p(HD_DAL self_id, HD_IO io_id, VDODEC_STATUS *p_status);
HD_RESULT videodec_set_status_p(HD_DAL self_id, HD_IO io_id, VDODEC_STATUS *p_status);
VDODEC_INTL_PARAM *videodec_get_param_p(HD_DAL self_id, HD_IO sub_id);
HD_RESULT videodec_set_param_p(HD_DAL self_id, HD_IO io_id);
HD_RESULT videodec_check_path_id_p(HD_PATH_ID path_id);
HD_RESULT videodec_init_path_p(HD_PATH_ID path_id);
HD_RESULT videodec_uninit_path_p(HD_PATH_ID path_id);
HD_RESULT videodec_init_p(void);
HD_RESULT videodec_uninit_p(void);
HD_RESULT videodec_close(HD_PATH_ID path_id);

#ifdef  __cplusplus
}
#endif

#endif
