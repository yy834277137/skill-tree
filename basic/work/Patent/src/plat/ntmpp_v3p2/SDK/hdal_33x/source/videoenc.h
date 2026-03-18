/**
 * @file videoenc.h
 * @brief type definition of HDAL.
 * @author PSW
 * @date in the year 2018
 */

#ifndef _VIDEO_ENC_H_
#define _VIDEO_ENC_H_
#ifdef  __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "bind.h"
#include "trig_ioctl.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define MAX_VDOENC_DEV          4   ///< Indicate max enc dev
#define VDOENC_DEVID(self_id)   (self_id - HD_DAL_VIDEOENC_BASE)
#define VDOENC_INID(id)         (id - HD_IN_BASE)
#define VDOENC_OUTID(id)        (id - HD_OUT_BASE)
#define VDOENC_STAMP_INID(id)   (id - HD_STAMP_BASE - HD_IN_BASE)
#define VDOENC_MASK_INID(id)    (id - HD_MASK_BASE - HD_IN_BASE)
#define VDOENC_STAMP_NU         16
#define VDOENC_MASK_NU          16
#define VDOENC_OSG_TYPE_NU      7
#define VDOENC_CHECK_H_POSIT(p1, p2)                                         \
	do {                                                                        \
		if ((p1.x > p2.x) || (p1.y != p2.y)) { \
			HD_VIDEOENC_ERR("Error H direction position\n");                  \
			return HD_ERR_NG;                                                   \
		}                                                                       \
	} while (0)
#define VDOENC_CHECK_V_POSIT(p1, p2)                                         \
	do {                                                                        \
		if ((p1.x != p2.x) || (p1.y > p2.y)) { \
			HD_VIDEOENC_ERR("Error V direction position\n");                  \
			return HD_ERR_NG;                                                   \
		}                                                                       \
	} while (0)

#define GET_VENC_PATHID(self_id, in_id, out_id) \
	(HD_PATH_ID)((self_id & 0xffff) << 16) | ((in_id & 0xff) << 8) | (out_id & 0xff)

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
#define GET_BS_UNKNOWN   0
#define GET_BS_BY_RECV   1
#define GET_BS_BY_PULL   2

typedef struct _VDOENC_INFO_PRIV {
	INT h26x_fd;
	INT jpeg_fd;
	uintptr_t queue_handle;
	uintptr_t osg_queue_handle;
	UINT param_ver_count;
	UINT last_set_count;
	INT  path_init_count;
	HD_PATH_ID  path_id;

	HD_PATH_ID_STATE push_state;
	HD_PATH_ID_STATE pull_state;
} VDOENC_INFO_PRIV;

typedef struct _VDOENC_STAMP_PARAM {
	HD_OSG_STAMP_IMG img;
	HD_OSG_STAMP_ATTR attr;
	HD_PATH_ID path_id;
	BOOL enable;
} HD_VDOENC_STAMP_PARAM;

typedef struct _HD_VDOENC_MASK_MOSAIC {
	INT8 is_mask;///< keep this region is mask(0) or mosaic(1)s. -1 is unused
	BOOL enable;
	union {
		HD_OSG_MASK_ATTR  mask_cfg;
		HD_OSG_MOSAIC_ATTR  mosaic_cfg;
	};
} HD_VDOENC_MASK_MOSAIC;

typedef enum _VIDEOENC_JPEG_RC_MODE {
	VIDEOENC_JPEG_RC_MODE_CBR = 0,     ///< Constant Bitrate
	VIDEOENC_JPEG_RC_MODE_VBR,         ///< Variable Bitrate
	VIDEOENC_JPEG_RC_MODE_FIXQP,       ///< fix quality
	VIDEOENC_JPEG_RC_MAX,
	ENUM_DUMMY4WORD(VIDEOENC_JPEG_RC_MODE)
} VIDEOENC_JPEG_RC_MODE;

typedef struct _VIDEOENC_JPEG_RATE_CONTROL {
	VIDEOENC_JPEG_RC_MODE vbr_mode;         ///< 0: CBR, 1: VBR
	UINT32 base_qp;                      ///< base qp
	UINT32 min_quality;                  ///< min quality
	UINT32 max_quality;                  ///< max quality
	UINT32 bitrate;	                 ///< bit rate
	UINT32 frame_rate_base;              ///< frame rate
	UINT32 frame_rate_incr;              ///< frame rate = frame_rate_base / frame_rate_incr
} VIDEOENC_JPEG_RATE_CONTROL;

typedef struct _VDOENC_PORT {
	HD_VIDEOENC_PATH_CONFIG enc_path_cfg;
	HD_VIDEOENC_IN          enc_in_dim;
	HD_VIDEOENC_OUT         enc_out_param;
	HD_H26XENC_VUI          enc_vui;
	HD_H26XENC_DEBLOCK      enc_deblock;
	HD_H26XENC_RATE_CONTROL enc_rc;
	HD_H26XENC_USR_QP       enc_usr_qp;
	HD_H26XENC_SLICE_SPLIT  enc_slice_split;
	HD_H26XENC_GDR          enc_gdr;
	HD_H26XENC_ROI          enc_roi;
	HD_H26XENC_ROW_RC       enc_row_rc;
	HD_H26XENC_AQ           enc_aq;
	HD_H26XENC_REQUEST_IFRAME request_keyframe;
	HD_VDOENC_MASK_MOSAIC   enc_mask_mosaic[VDOENC_MASK_NU];
	HD_VDOENC_STAMP_PARAM   stamp[VDOENC_STAMP_NU];
	HD_OSG_PALETTE_TBL		enc_osg_palette_tbl;
	HD_VIDEOENC_STATUS		enc_status;
	HD_OUT_POOL out_pool;	///< for pool info setting
	VIDEOENC_JPEG_RATE_CONTROL jpeg_rc;
	CHAR        push_in_used;
	INT osg_fd;
} VDOENC_PORT;
typedef struct _VDOENC_INFO {
	VDOENC_PORT port[HD_VIDEOENC_MAX_IN];
	//private data
	VDOENC_INFO_PRIV priv[HD_VIDEOENC_MAX_IN];
} VDOENC_INFO;

typedef struct _VDOENC_INTERNAL_PARAM {
	struct {
		UINT                    set_count;    //if user set any param, this value add 1
		HD_VIDEOENC_PATH_CONFIG *p_enc_path_cfg;
		HD_VIDEOENC_IN          *p_enc_in_param;
		HD_VIDEOENC_OUT         *p_enc_out_param;
		HD_H26XENC_VUI          *p_enc_vui;
		HD_H26XENC_DEBLOCK      *p_enc_deblock;
		HD_H26XENC_RATE_CONTROL *p_enc_rc;
		HD_H26XENC_USR_QP       *p_enc_usr_qp;
		HD_H26XENC_SLICE_SPLIT  *p_enc_slice_split;
		HD_H26XENC_GDR          *p_enc_gdr;
		HD_H26XENC_ROI          *p_enc_roi;
		HD_H26XENC_ROW_RC       *p_enc_row_rc;
		HD_H26XENC_AQ           *p_enc_aq;
		HD_H26XENC_REQUEST_IFRAME *p_request_keyframe;
		UINT16 *osg_idx;
		UINT linefd;
		//HD_VIDEOENC_IN *enc_in_dim;
		//HD_VIDEOENC_PARAM   *enc_cfg;
		//HD_H26XENC_RATE_CONTROL *enc_rc;
	    HD_OUT_POOL *p_out_pool;
		VIDEOENC_JPEG_RATE_CONTROL *p_jpeg_rc;
		INT get_bs_method;  // use GET_BS_UNKNOWN, GET_BS_BY_RECV, GET_BS_BY_PULL

	} port[HD_VIDEOENC_MAX_IN];
} VDOENC_INTERNAL_PARAM;
/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them

/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
HD_RESULT videoenc_open(HD_PATH_ID path_id);
HD_RESULT videoenc_close(HD_PATH_ID path_id);
HD_RESULT videoenc_init(void);
HD_RESULT videoenc_uninit(void);
HD_RESULT videoenc_new_in_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame);
HD_RESULT videoenc_push_in_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame, HD_VIDEOENC_BS *p_usr_out_video_bitstream, INT32 wait_ms);
HD_RESULT videoenc_release_in_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame);
HD_RESULT videoenc_pull_out_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEOENC_BS *p_video_bitstream, INT32 wait_ms);
HD_RESULT videoenc_release_out_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEOENC_BS *p_video_bitstream);
INT check_params_range(HD_VIDEOENC_PARAM_ID id, VOID *p_param, CHAR *p_ret_string, INT max_str_len);
HD_RESULT videoenc_init_path(HD_PATH_ID path_id);
HD_RESULT videoenc_uninit_path(HD_PATH_ID path_id);
INT verify_param_HD_VIDEOENC_IN(HD_VIDEOENC_IN *p_enc_in, HD_VIDEOENC_OUT *p_enc_out, CHAR *p_ret_string, INT max_str_len);
INT verify_param_HD_VIDEOENC_IN_STAMP(HD_VIDEOENC_IN *p_enc_in, HD_VDOENC_STAMP_PARAM *p_enc_img, CHAR *p_ret_string, INT max_str_len);

VDOENC_INTERNAL_PARAM *videoenc_get_param_p(HD_DAL self_id);
HD_RESULT videoenc_request_keyframe_p(HD_DAL self_id,  HD_IO out_id);
HD_RESULT videoenc_set_osg_win(HD_PATH_ID path_id, BOOL enable);
HD_RESULT videoenc_set_mask_mosaic(HD_PATH_ID path_id, BOOL enable);
HD_RESULT videoenc_get_mask_palette(HD_DAL self_id, HD_IO in_id, HD_OSG_PALETTE_TBL *p_palette_tbl);
HD_RESULT videoenc_set_mask_palette(HD_DAL self_id, HD_IO out_id, const HD_OSG_PALETTE_TBL *p_palette_tbl);
HD_RESULT videoenc_get_status_p(HD_DAL self_id, HD_IO io_id, HD_VIDEOENC_STATUS *p_status);
HD_RESULT videoenc_set_osg_for_push_pull(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_in_video_frame, INT32 wait_ms);
char* get_format_name(vpd_buf_type_t vpd_buf_type);
char* get_osg_type_name(HD_VIDEO_PXLFMT osg_type);
#ifdef  __cplusplus
}
#endif


#endif  /* _VIDEO_ENC_H_ */
