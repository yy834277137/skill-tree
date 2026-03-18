/*
 *   @file   pif.h
 *
 *   @brief  platform interface header file.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#ifndef __PIF_H__     /* prevent multiple inclusion of the header file */
#define __PIF_H__
#include <pthread.h>
#include "error.h"
#include "bind.h"

/* General difinition */
#define GM_MAX_CHIP_NUM                         4  // must same with VPD_MAX_CHIP_NUM
#define GM_MAX_GROUP_NUM                        136  // must same with VPD_MAX_GROUP_NUM
#define GM_GROUP_FD_START                       1  // groupfd start from 1, 0 for null group use

#define GM_MAX_BIND_NUM_PER_GROUP               64  // must same with VPD_MAX_BIND_NUM_PER_GROUP
#define GM_MAX_DUP_LINE_ENTITY_NUM              6   // must same with VPD_MAX_DUP_LINE_ENTITY_NUM
#define GM_MAX_LINE_ENTITY_NUM                  16  // must same with VPD_MAX_LINE_ENTITY_NUM

#define GM_MAX_ATTR_DATA 10

#define FILE_VCH_NUMBER         128
#define CAPTURE_VCH_NUMBER      128
#define LCD_VCH_NUMBER          6
#define AUDIO_GRAB_VCH_NUMBER   32
#define AUDIO_RENDER_VCH_NUMBER 32

#define DISPLAY_MAX_FPS			30 // 60fps or 30fps(default)

#define GMLIB_DBG_LEVEL_1      1  // for user dbg
#define GMLIB_DBG_LEVEL_2      2  // config parameter/flow
#define GMLIB_DBG_LEVEL_3      3  // internal debug

/* need sync with em/common.h  & gm_lib/src/pif.h  & ioctl/vplib.h  */
#define GMLIB_NULL_BYTE     0xFE
#define GMLIB_NULL_SHORT    0xFEFE
#define GMLIB_NULL_VALUE    ((GMLIB_NULL_BYTE << 24) | (GMLIB_NULL_BYTE << 16) | \
							 (GMLIB_NULL_BYTE << 8) | (GMLIB_NULL_BYTE))

#define GMLIB_DEFAULT_WIN_X             0
#define GMLIB_DEFAULT_WIN_Y             0
#define GMLIB_DEFAULT_WIN_WIDTH         64  //minimum requirement for HW
#define GMLIB_DEFAULT_WIN_HEIGHT        64  //minimum requirement for HW
#define GMLIB_DUPLICATE_CASCADE_CAP_DST_WIDTH           640
#define GMLIB_DUPLICATE_CASCADE_CAP_DST_HEIGHT_NTSC     480
#define GMLIB_DUPLICATE_CASCADE_CAP_DST_HEIGHT_PAL      576
#define GMLIB_YCrYCb_to_CrCbY(x)  ((((x) & 0xff00) >> 8) | (((x) & 0xff) << 8) | ((x) & 0xff0000))

void *pif_malloc(size_t size, const char *func);
void pif_free(void *ptr, const char *func);
void *pif_calloc(size_t nmemb, size_t size, const char *func);
void *pif_realloc(void *ptr, size_t size, const char *func);


#define LIB_DEBUG_BUF_ALLOC              0
#define LIB_DEBUG_BUF_ALLOC_PRINT_LOG    0
#if LIB_DEBUG_BUF_ALLOC
#define PIF_MALLOC(size)         pif_malloc((size), __func__)
#define PIF_CALLOC(nmemb,size)   pif_calloc((nmemb), (size), __func__)
#define PIF_FREE(ptr)            pif_free((ptr), __func__)
#define PIF_REALLOC(prt,size)    pif_realloc((prt), (size), __func__)
#else
#define PIF_MALLOC(size)         malloc((size))
#define PIF_CALLOC(nmemb,size)   calloc((nmemb), (size))
#define PIF_FREE(ptr)            free((ptr))
#define PIF_REALLOC(prt,size)    realloc((prt), (size))
#endif

typedef struct {
	int pif_dbg_malloc_cnt;
	int pif_dbg_calloc_cnt;
	int pif_dbg_realloc_cnt;
	int pif_dbg_free_cnt;
	int pif_dbg_malloc_size;
	int pif_dbg_calloc_size;
	int pif_dbg_realloc_size;
	int pif_dbg_free_size;
} pif_dbg_alloc_statistic_t;


/* return value definition */
#define GM_SUCCESS               0
#define GM_FD_NOT_EXIST         -1
#define GM_BS_BUF_TOO_SMALL     -2
#define GM_EXTRA_BUF_TOO_SMALL  -3
#define GM_TIMEOUT              -4
#define GM_DUPLICATE_FD         -5

#define	GM_POOL_NUM			4
#define	GM_POOL_NAME_LEN	64

#define GM_FRAME_FARE_BASE_CLK  600

#define PROC_MSG_SIZE     	4096

extern void gmlib_err_log(const char *fmt, ...);

#define GMLIB_L1_P(fmt, args...) \
	do {\
		if (gmlib_dbg_level >= GMLIB_DBG_LEVEL_1) \
			pif_send_log("(%d):" fmt,(int)syscall(SYS_gettid), ## args);\
	} while(0)

#define GMLIB_L2_P(fmt, args...) \
	do {\
		if (gmlib_dbg_level >= GMLIB_DBG_LEVEL_2) \
			pif_send_log(fmt, ## args);\
	} while(0)

#define GMLIB_L3_P(fmt, args...) \
	do {\
		if (gmlib_dbg_level >= GMLIB_DBG_LEVEL_3) \
			pif_send_log("%s:%d " fmt,__FUNCTION__,__LINE__, ## args);\
	} while(0)

#define GMLIB_WARNING_P(fmt, args...) \
	do {\
		pif_send_log("%s:%d " fmt,__FUNCTION__,__LINE__, ## args);\
	} while(0)

#define GMLIB_ERROR_P(fmt, args...) \
	do {\
		extern unsigned int gmlib_quiet;\
		if (gmlib_quiet == 0) {\
			printf("%s:" fmt, __func__, ##args); fflush(stdout);\
		}\
		pif_send_log("(%d):" fmt,(int)syscall(SYS_gettid), ## args);\
		gmlib_err_log("%s:%d " fmt,__FUNCTION__,__LINE__, ## args);\
	} while(0)

#define GMLIB_PRINT_P(fmt, args...) \
	do {\
		extern unsigned int gmlib_quiet;\
		if (gmlib_quiet == 0)\
			printf(fmt, ## args); fflush(stdout);\
	} while(0)

#define GMLIB_DUMP_DATA(prefix, bindfd, buf, len) \
	do { \
		if (gmlib_dbg_level >= GMLIB_DBG_LEVEL_2) { \
			GMLIB_L2_P(prefix"bindfd(%#x) len(%d) buf: %02x%02x%02x%02x %02x%02x%02x%02x " \
					   "%02x%02x%02x%02x %02x%02x%02x%02x\n", bindfd, len, \
					   buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],\
					   buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]); \
		} \
	} while(0)

#define GMLIB_DUMP_MEM(addr, len) \
	do { \
		int dumploop; \
		if (gmlib_dbg_level >= GMLIB_DBG_LEVEL_2) \
			for (dumploop = 0; dumploop < (len); dumploop += 16) { \
				GMLIB_L2_P("<0x%08x: 0x%08x 0x%08x 0x%08x 0x%08x>\n", \
						   (void *)((void *)(addr) + dumploop), \
						   *(int *)((void *)(addr) + dumploop), \
						   *(int *)((void *)(addr) + dumploop + 4), \
						   *(int *)((void *)(addr) + dumploop + 8), \
						   *(int *)((void *)(addr) + dumploop + 12)); \
			} \
	} while(0)


#define PIF_SET_DWORD(msb_16, lsb_16) \
	(((unsigned int)(msb_16)<<16) & 0xffff0000) | ((unsigned int) (lsb_16) & 0x0000ffff)
#define PIF_SET_DIM(width, height) PIF_SET_DWORD(width, height)
#define PIF_SET_XY(x, y) PIF_SET_DWORD(x, y)
#define PIF_SET_FPS_RATIO(numerator, denominator) PIF_SET_DWORD(numerator, denominator)

#define PIF_INT_MSB(value) (((unsigned int)(value)>>16) & 0x0000FFFF)
#define PIF_INT_LSB(value) (((unsigned int)(value)) & 0x0000FFFF)

#define PIF_PARAM_WIDTH(value)       PIF_INT_MSB(value)
#define PIF_PARAM_HEIGHT(value)      PIF_INT_LSB(value)
#define PIF_PARAM_X(value)           PIF_INT_MSB(value)
#define PIF_PARAM_Y(value)           PIF_INT_LSB(value)

typedef struct {
	unsigned int linefd;
	unsigned int entity_fd;
	vpd_entity_type_t entity_type;
	int vpdFdinfo[BD_FD_MAX_PRIVATE_DATA];  //NOTE: size need same with vpd_channel_fd_t
	unsigned int post_handling;
	int wait_ms;
} pif_get_buf_t;

typedef struct {
	unsigned int count;
	unsigned int first_time;
} pif_statistic_info_t;

typedef enum {
	GRAPH_DISP_ONLY = 1,
	GRAPH_ZERO_CH_ONLY = 2,
	GRAPH_DISP_AND_ENCODE = 3,
} graph_mode_t;

typedef struct {
	graph_mode_t graph_mode;
	//for lv
	int          lv_buf_size;           //real size of frame in memory
	int          lv_buf_offset;         //accumulate size

	dim_t        cap_dim;
	rect_t       cap_crop_rect;         //if cap does NOT crop, set this value to all 0
	dim_t        cap_bg_dim;
	vpd_cap_method_t cap_method;

	//for playback
	dim_t        pb_file_dim;
	unsigned int src_attr_vch;          //cap-vch or file-vch
	int          pb_max_fps;

	dim_t        vpe_crop_dim;
	dim_t        vpe_crop_dim2;
	rect_t       vpe_crop_rect;
	rect_t       vpe_win_rect;
	rect_t       vpe_crop_rect2;
	rect_t       vpe_sub_win_rect;
	unsigned int lv_vpe_draw_sequence;

	int          is_ch_visible;
	vpd_buf_type_t    cap_dst_fmt;
	vpd_buf_type_t    vpe_dst_fmt;
	vpd_di_mode_t     di_mode;
	vpd_dn_mode_t     dn_mode;
	vpd_sharpness_mode_t shrp_mode;
	unsigned int spnr;
	unsigned int tmnr;
	unsigned int sharpness;
	unsigned int false_color;

	//rotation attr
	int         rotate_enabled;
	int         rotate_mode;

	//aspect ratio attr
	int         aspect_enabled;
	int         palette_idx;

	//for disp_enc or zero-ch
	dim_t        lvenc_disp_bg_dim;
	rect_t       lvenc_disp_rect;
	dim_t        lvenc_enc_bg_dim;
	rect_t       lvenc_enc_rect;
	int          lvenc_src_fps;
	int          lvenc_is_need_vpe;
} disp_info_t;

typedef struct {
	int fincr;
	int fbase;     // framerate = fbase/fincr, need base on GM_FRAME_FARE_BASE_CLK
	int is_cap_target_fps_mode;
	dim_t dim;              //max enc dim in all streams
	int is3dnr;

	dim_t             cap_bg_dim;
	rect_t            cap_rect;
	rect_t            cap_crop_rect;
	int               enc_buf_size;           //real size of frame in memory

	vpd_buf_type_t    cap_dst_fmt;
	vpd_buf_type_t    enc_src_fmt;
	int               is_internal_enc_3di;
	vpd_di_mode_t     di_mode;
	vpd_dn_mode_t     dn_mode;
	vpd_sharpness_mode_t shrp_mode;
	unsigned int spnr;
	unsigned int tmnr;
	unsigned int sharpness;
	int               vpe_stream_counts;
#define VPE_IN_EQUAL_OUT        0xAA
#define VPE_IN_NOT_EQUAL_OUT    0xAB
	unsigned char     vpe_in_out;   //IVS_VPE: [0xAA]equal, [0xAB]not equal
} rec_cap_info_t;

typedef struct {
	rec_cap_info_t  cur;
	rec_cap_info_t  pre;
} pif_cap_info_t;

typedef struct {
	dim_t             cap_dim;
	dim_t             win_dim;
} lv_info_t;

typedef struct {
	lv_info_t  cur;
	lv_info_t  pre;
} pif_liveview_info_t;

typedef enum {
	PIP_NONE = 0,
	PIP_SCL_PRIORITY,   // set priority to gs and do scaling in sequence
	PIP_MODE1,          // do pip/pap by scaler support
	PIP_MODE2,          // do pip by cascading capture link
} pip_mode_t;


typedef struct {
	int in_use;
	int groupidx;
	int error_state;
	unsigned int enc_start_node_offset;

	/* following for encapsulate info */
	void *encap_buf;    //structure is vpd_graph_t
	int sub_group_count;
	int is_set_attribute;
	void *hd_group;
} pif_group_t;

typedef struct {
	int init_nr;
	unsigned int gpfd_nr;
	unsigned int alloc_nr;
	unsigned int init_complete;
} pif_init_private_t;

/* Dimention definition */
typedef struct gm_dim {
	int width;
	int height;
} gm_dim_t;

/* Rectancle definition */
typedef struct gm_rect {
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
} gm_rect_t;

typedef enum {
	GM_PCM = 1,
	GM_AAC,
	GM_ADPCM,
	GM_G711_ALAW,
	GM_G711_ULAW
} gm_audio_encode_type_t;

/***********************************
 *        Notify Structure
 ***********************************/
typedef enum {
	GM_NOTIFY_UNKNOWN = 0,
	GM_NOTIFY_PERF_LOG,
	GM_NOTIFY_GMLIB_DBGMODE,
	GM_NOTIFY_GMLIB_DBGLEVEL,
	GM_NOTIFY_SIGNAL_LOSS,
	GM_NOTIFY_SIGNAL_PRESENT,
	GM_NOTIFY_FRAMERATE_CHANGE,
	GM_NOTIFY_HW_CONFIG_CHANGE,
	GM_NOTIFY_TAMPER_ALARM,
	GM_NOTIFY_TAMPER_ALARM_RELEASE,
	GM_NOTIFY_AUDIO_BUFFER_UNDERRUN,
	GM_NOTIFY_AUDIO_BUFFER_OVERRUN,
	GM_NOTIFY_HDAL_LOG,
	GM_NOTIFY_HDAL_FLOW,
	GM_NOTIFY_GMLIB_FLOW,
	MAX_GM_NOTIFY_COUNT,
} gm_notify_t;

/*!
 * @fn int gm_notify_handler_t(void *groupfd, int entity_fd, gm_notify_t notify, void *data)
 * @brief callback function for notify
 * @param groupfd The fd of group
 * @param entity_fd the eneity which sends the notification
 * @param notify the type of this notification
 * @param data the private data from registered function
 * @return return none
 */
typedef void (*gm_notify_handler_t)(void *groupfd, int entity_fd, gm_notify_t notify, void *data);



/***********************************
 *        System Structure
 ***********************************/
typedef struct {
	int valid;
	unsigned int chipid;
	int reserved[8];         ///< Reserved words
} gm_file_sys_info_t;

typedef enum {
	GM_INTERLACED,
	GM_PROGRESSIVE,
} gm_video_scan_method_t;

typedef enum {
	GM_ENCODE_TYPE_JPEG = 0,
	GM_ENCODE_TYPE_H264 = 1,
	GM_ENCODE_TYPE_H265 = 2,
} gm_encode_type_t;

typedef enum {
	GM_I_FRAME,
	GM_P_FRAME,
	GM_NP_FRAME,    //Non-Reference P-frame (no other frames refer to this)
	GM_KP_FRAME,    //Key P-frame
	GM_SKIP_FRAME,  //An unchanged P-frame (This frame is a duplicate of previous one,
	//  Ths slice length is very small)
} gm_slice_type_t;

typedef struct {
	int valid;
	unsigned int number_of_path;
	gm_video_scan_method_t scan_method;
	int framerate;
	gm_dim_t dim;
	unsigned int chipid;
	int is_present;
	int scl_h_max_sz;
	int reserved[7];         ///< Reserved words
} gm_cap_sys_info_t;

typedef struct {
	int valid;
	int framerate;
	gm_dim_t dim;
	unsigned int chipid;
	unsigned int is_channel_zero;
	int reserved[8];         ///< Reserved words
} gm_lcd_sys_info_t;

typedef enum {
	GM_MONO = 1,
	GM_STEREO,
} gm_audio_channel_type_t;

typedef struct {
	int valid;
	gm_audio_channel_type_t channel_type;
	int sample_rate;  /* 8000, 16000, 32000, 44100... */
	int sample_size;  /* bit number per sample, 8bit, 16bit... */
	unsigned int chipid;
	int reserved[5];         ///< Reserved words
} gm_audio_grab_sys_info_t;

typedef struct {
	int valid;
	gm_audio_channel_type_t channel_type;
	int sample_rate;  /* 8000, 16000, 32000, 44100... */
	int sample_size;  /* bit number per sample, 8bit, 16bit... */
	unsigned int chipid;
	int reserved[5];         ///< Reserved words
} gm_audio_render_sys_info_t;


/***********************************
 *       Poll Structure
 ***********************************/
#define GM_POLL_READ            1
#define GM_POLL_WRITE           2

typedef struct {
	unsigned int event;
	unsigned int bs_len;
	unsigned int extra_len;
	unsigned int keyframe;  // 1: keyframe, 0: non-keyframe
} gm_ret_event_t;

/******************** only for reference and to get the __vpd_channel_fd_t size *****************/
typedef struct {
	int line_idx;   // mapping to line index of graph
	int id;         //id=chipid(bit24_31)+cpuid(bit16_23)+graphid(bit-0_15)
	uintptr_t gs_ver_id;    // gs verId handler point;
	int entity_fd;
	int reserve[1]; // NOTE: vpdChannelFD total len = GM_FD_MAX_PRIVATE_DATA
} __vpd_channel_fd_t; // NOTE: relative to gm_pollfd_t

#define GM_FD_MAX_PRIVATE_DATA sizeof(__vpd_channel_fd_t)
/******************** only for reference and to get the __vpd_channel_fd_t size *****************/
typedef struct {
	char fd_private[GM_FD_MAX_PRIVATE_DATA] __attribute__((aligned(8)));  ///< Library internal data, don't use it!
	HD_GROUP_LINE *group_line;    ///< The return value of gm_bind
	unsigned int event;     ///< The poll event ID,
	gm_ret_event_t revent;  ///< The returned event value
} __attribute__((packed, aligned(8))) gm_pollfd_t;  // size layout, have to same with vpd_poll_obj_t

/***********************************
 *  Encode/Decode Bitstream Structure
 ***********************************/
#define GM_FLAG_NEW_FRAME_RATE  (1 << 3) ///< Indicate the bitstream of new frame rate setting
#define GM_FLAG_NEW_GOP         (1 << 4) ///< Indicate the bitstream of new GOP setting
#define GM_FLAG_NEW_DIM         (1 << 6) ///< Indicate the bitstream of new resolution setting
#define GM_FLAG_NEW_BITRATE     (1 << 7) ///< Indicate the bitstream of new bitrate setting

typedef struct gm_enc_bitstream {
	/* provide by application */
	unsigned long long timestamp;   ///< Encode bitstream timestamp
	char *bs_buf;             ///< Bitstream buffer pointer
	unsigned int bs_buf_len;  ///< AP provide bs_buf max length
	char *extra_buf;          ///< NULL indicate no needed extra-data
	unsigned int extra_buf_len;  ///< NULL indicate no needed extra-data

	/* value return by gm_recv_bitstream() */
	unsigned int bs_len;      ///< Bitstream length
	unsigned int extra_len;   ///< Extra data length
	unsigned int frame_type;  ///< old name is 'keyframe', its from "slice_type" property
	unsigned int newbs_flag;  ///< Flag notification of new seting, such as GM_FLAG_NEW_BITRATE
	unsigned int checksum;             ///< Checksum value
	int ref_frame;            ///< 1: reference frame, CANNOT skip, 0: not reference frame, can skip,
	unsigned int slice_offset[4];      ///< multi-slice offset 1~3 (first offset is 0)
	int sps_offset;
	int pps_offset;
	int vps_offset;
	int bs_offset;

	int svc_layer_type;
	unsigned int qp_value;
	unsigned int evbr_state;

	unsigned int bistream_type;

	//for h264 output
	unsigned int intra_16_mb_num;
	unsigned int intra_8_mb_num;
	unsigned int intra_4_mb_num;
	unsigned int inter_mb_num;
	unsigned int skip_mb_num;

	//for h265 output
	unsigned int intra_32_cu_num;
	unsigned int intra_16_cu_num;
	unsigned int intra_8_cu_num;
	unsigned int inter_64_cu_num;
	unsigned int inter_32_cu_num;
	unsigned int inter_16_cu_num;
	unsigned int skip_cu_num;
	unsigned int merge_cu_num;

	//for psnr output
	unsigned int psnr_y_mse;
	unsigned int psnr_u_mse;
	unsigned int psnr_v_mse;

	unsigned int motion_ratio;
	int reserved[5];          ///< Reserved words
} gm_enc_bitstream_t;

/******************** only for reference and to get the __vpd_get_copy_dout_t *****************/
typedef struct {
	uintptr_t extra_va;  //put "audio 2nd bitstream" or "video mv data"
	unsigned int extra_len;
	uintptr_t bs_va;     // kernel virtual address of this bs
	unsigned int bs_len;
	unsigned int new_bs;    // new bitstream flag
	unsigned int bistream_type;
	unsigned int frame_type;
	unsigned int ref_frame;
	uintptr_t reserved;
	unsigned int checksum;
	unsigned long long timestamp;// user's bs timestamp (microsecond) must be set to this field
	unsigned int slice_offset[4]; //multi-slice offset 1~3 (first offset is 0)
	unsigned int sps_offset;
	unsigned int pps_offset;
	unsigned int vps_offset;
	unsigned int bs_offset;
	unsigned int svc_layer_type;
	unsigned int intra_16_mb_num;
	unsigned int intra_8_mb_num;
	unsigned int intra_4_mb_num;
	unsigned int inter_mb_num;
	unsigned int skip_mb_num;
	unsigned int intra_32_cu_num;
	unsigned int intra_16_cu_num;
	unsigned int intra_8_cu_num;
	unsigned int inter_64_cu_num;
	unsigned int inter_32_cu_num;
	unsigned int inter_16_cu_num;
	unsigned int skip_cu_num;
	unsigned int merge_cu_num;
	unsigned int psnr_y_mse;
	unsigned int psnr_u_mse;
	unsigned int psnr_v_mse;
	unsigned int qp_value;
	unsigned int evbr_state;
	unsigned int motion_ratio;
} __vpd_dout_bs_t; // Must be same as vpd_dout_bs_t

typedef struct {
	/* for dataout users to check and set */
	__vpd_dout_bs_t bs;
	/* internal use */
	void    *priv; /* internal use */
	unsigned int reserved[1];
} __vpd_dout_buf_t; // Must be same as vpd_dout_buf_t

typedef struct {
	__vpd_dout_buf_t dout_buf;
	__vpd_channel_fd_t channel_fd;
	unsigned int bs_buf_len;    // length of bs_buf, for check buf length whether it is enough. set the value by AP.
	char *bs_buf;               // buffer point of bitstream, set by AP
	unsigned int extra_buf_len; // length of mv_buf(video)/2nd_bs(audio), for check buf length whether it is enough. set the value by AP.
	char *extra_buf;            // extra_buffer point of bitstream, set by AP
	unsigned int reserved1[1];
	/* internal use */
	int    priv[2]; /* internal use */
	unsigned int reserved[4];
} __attribute__((packed, aligned(8))) __vpd_get_copy_dout_t;  // Must be same as vpd_get_copy_dout_t

#define GM_ENC_MAX_PRIVATE_DATA  sizeof(__vpd_get_copy_dout_t)
/******************** only for reference and to get the __vpd_get_copy_dout_t *****************/

typedef struct gm_enc_multi_bitstream {
	gm_enc_bitstream_t bs;
	HD_GROUP_LINE *group_line;
	int retval;  ///< less than 0: recv bistream fail.
	int reserved[7];         ///< Reserved words

	char enc_private[GM_ENC_MAX_PRIVATE_DATA] __attribute__((aligned(8))); ///< Library internal data, don't use it!
} gm_enc_multi_bitstream_t;

typedef enum {
	TIME_ALIGN_ENABLE  = 0xFEFE01FE,
	TIME_ALIGN_DISABLE = 0xFEFE07FE,
	TIME_ALIGN_USER    = 0xFEFE09FE
} time_align_t;

/******************** only for reference and to get the __vpd_put_copy_din_t *****************/
typedef struct {
	uintptr_t va;        // kernel virtual address of this bs
	uintptr_t pa;        // physical address of this bs
	unsigned int size;      // max allowable length of bs to be copied
	unsigned int used_size; // in the end, how much memory used. should be set by user
	unsigned int user_flag; // in the end, how much memory used. should be set by user
	unsigned int fail_job_count; // entity fail job count which is query from em
	unsigned int ts_user_flag; // timestamp user flag
	// NOTE: Be careful, this long long timestamp must 4-bytes alignment. for other toolchain.
	unsigned long long timestamp __attribute__((packed, aligned(4)));
	unsigned long long ap_timestamp __attribute__((packed, aligned(4)));
} __vpd_din_bs_t;

typedef struct {
	/* for datain users to check and set */
	__vpd_din_bs_t bs;
	/* internal use */
	void    *priv;
} __vpd_din_buf_t;

typedef struct {
	__vpd_channel_fd_t channel_fd;
	unsigned int bs_len;    // length of bitstream from AP
	char *bs_buf;           // buffer point of bitstream from AP
	unsigned int time_align_by_user; // time align value in micro-second, 0 means disable
	int timeout_ms;         // timeout in ms, -1 means no poll
	__vpd_din_buf_t dinBuf;   // data_in handle
	unsigned long long ap_timestamp;
	unsigned int user_flag;
	unsigned int reserved[7];
} __attribute__((packed, aligned(8))) __vpd_put_copy_din_t;

#define GM_DEC_MAX_PRIVATE_DATA  sizeof(__vpd_put_copy_din_t)
/******************** only for reference and to get the __vpd_put_copy_din_t *****************/
/* NOTE: need 8bytes align for (* vpdDinBs_t)->timestamp */
typedef struct gm_dec_multi_bitstream {
	HD_GROUP_LINE *group_line;
	char *bs_buf;
	unsigned int bs_buf_len;
	int retval;  ///< less than 0: send bistream fail.
	/* time_align:
	    TIME_ALIGN_ENABLE(default): playback time align by LCD period (ex. 60HZ is 33333us)
	    TIME_ALIGN_DISABLE: play timestamp by gm_send_multi_bitstreams called
	    TIME_ALIGN_USER: start to play at previous play point + time_diff(us)
	 */
	time_align_t time_align;
	unsigned int time_diff; ///< time_diff(us): playback interval time by micro-second
	unsigned long long ap_timestamp;
	unsigned int user_flag; ///< bit0: user callback control, 0(normal, put to next) 1(free after callback)
	///< bit1: dec send black bitstream control
	///<       0(normal bitstream), 1(black bitstream, DEC do not keep it as reference frame)

	int reserved[2];        ///< Reserved words
	                        ///< [0] for decode job fail counts
	                        ///< [1] for ap timestamp user flag

	char dec_private[GM_DEC_MAX_PRIVATE_DATA] __attribute__((aligned(8)));   ///< Library internal(vpd_put_copy_din_t) data, don't use it!
} __attribute__((packed, aligned(8))) gm_dec_multi_bitstream_t;

/***********************************
 *        Clear Window Structure
 ***********************************/
typedef enum {
	GM_ACTIVE_BY_APPLY,
	GM_ACTIVE_IMMEDIATELY,
} gm_clear_window_mode_t;

typedef struct {
	int in_w;   ///< minimun input dim: 64x32
	int in_h;   ///< minimun input dim: 64x32
	vpd_buf_type_t in_fmt;
	unsigned char *buf;
	int out_x;
	int out_y;
	int out_w;
	int out_h;
	gm_clear_window_mode_t  mode;
	int reserved[5];
} gm_clear_window_t;

/***********************************
 *        Snapshot Structure
 ***********************************/
typedef struct snapshot { /* encode snapshot JPEG */
	unsigned int entity_fd;
	int image_quality;  ///< The value of JPEG quality from 1(worst) ~ 100(best)
	char *bs_buf;
	unsigned int bs_buf_len; ///< User given parepred bs_buf length, gm_lib returns actual length
	int bs_width;   ///< bitstream width, support range 128 ~ 720
	int bs_height;  ///< bitstream height, support range 96 ~ 576
	unsigned int timestamp;   ///< bitstream timestamp
	int reserved1[5];
} snapshot_t;

/***********************************
 * user interface structure
 ***********************************/


typedef struct {
	vpd_buf_type_t format;
	gm_dim_t dim;
	gm_dim_t bg_dim;
	unsigned long long timestamp;
	unsigned int ts_user_flag;
} gm_video_frame_info_t;

typedef struct {
	void *pa;
	int size;
	int ddr_id;
	char pool_name[GM_POOL_NAME_LEN];//please fill the buffer name of /proc/videograph/ms/fixed or '\0' NULL str,ex:disp_dec_out..
} gm_buffer_t;

typedef struct {
	unsigned int qp;
	unsigned int evbr_state;
	unsigned int frame_type;
	unsigned int slice_type;  //i,p,kp,...

	unsigned int sps_offset;
	unsigned int pps_offset;
	unsigned int vps_offset;
	unsigned int bs_offset;
	unsigned int svc_layer_type; //sps,pps,vps,slice,...

	unsigned int intra_16_mb_num;
	unsigned int intra_8_mb_num;
	unsigned int intra_4_mb_num;
	unsigned int inter_mb_num;
	unsigned int skip_mb_num;

	unsigned int intra_32_cu_num;
	unsigned int intra_16_cu_num;
	unsigned int intra_8_cu_num;
	unsigned int inter_64_cu_num;
	unsigned int inter_32_cu_num;
	unsigned int inter_16_cu_num;
	unsigned int skip_cu_num;
	unsigned int merge_cu_num;

	unsigned int psnr_y_mse;
	unsigned int psnr_u_mse;
	unsigned int psnr_v_mse;

	unsigned int motion_ratio;
	unsigned long long timestamp;
} gm_bs_info_t;


#define GM_LCD0      0  ///< Indicate lcd_vch value
#define GM_LCD1      1  ///< Indicate lcd_vch value
#define GM_LCD2      2  ///< Indicate lcd_vch value
#define GM_LCD3      3  ///< Indicate lcd_vch value
#define GM_LCD4      4  ///< Indicate lcd_vch value
#define GM_LCD5      5  ///< Indicate lcd_vch value

typedef struct {
	int                      is_enc_in_param;
	HD_VIDEOENC_IN           enc_in_param;

	int                      is_enc_out_param;
	HD_VIDEOENC_OUT          enc_out_param;

	int                      is_enc_vui;
	HD_H26XENC_VUI           enc_vui;

	int                      is_enc_deblock;
	HD_H26XENC_DEBLOCK       enc_deblock;

	int                      is_enc_rc;
	HD_H26XENC_RATE_CONTROL  enc_rc;

	int                      is_enc_usr_qp;
	HD_H26XENC_USR_QP        enc_usr_qp;

	int                      is_enc_slice_split;
	HD_H26XENC_SLICE_SPLIT   enc_slice_split;

	int                      is_enc_gdr;
	HD_H26XENC_GDR           enc_gdr;

	int                      is_enc_roi;
	HD_H26XENC_ROI           enc_roi;

	int                      is_enc_row_rc;
	HD_H26XENC_ROW_RC        enc_row_rc;

	int                      is_enc_aq;
	HD_H26XENC_AQ            enc_aq;
} hdal_videoenc_setting_t;


/***********************************
 *        OSG Structure
 ***********************************/
typedef struct {
	unsigned int osg_image_idx;      ///< identified image index for osg
	int exist;                       ///< indicated non_exist:0, exist:1
	char *buf;                       ///< image buffer pointer
	unsigned int buf_len;            ///< image buffer length
	vpd_buf_type_t buf_type;            ///< image buffer type
	gm_dim_t dim;                    ///< the dimension of image
	int reserved[5];                 ///< Reserved words
} gm_osg_image_t;

typedef enum {
	GM_ALIGN_TOP_LEFT = 0,
	GM_ALIGN_TOP_CENTER,
	GM_ALIGN_TOP_RIGHT,
	GM_ALIGN_BOTTOM_LEFT,
	GM_ALIGN_BOTTOM_CENTER,
	GM_ALIGN_BOTTOM_RIGHT,
	GM_ALIGN_CENTER,
} gm_align_type_t;

typedef enum {
	GM_ALPHA_0 = 0, ///< alpha 0%
	GM_ALPHA_25,    ///< alpha 25%
	GM_ALPHA_37_5,  ///< alpha 37.5%
	GM_ALPHA_50,    ///< alpha 50%
	GM_ALPHA_62_5,  ///< alpha 62.5%
	GM_ALPHA_75,    ///< alpha 75%
	GM_ALPHA_87_5,  ///< alpha 87.5%
	GM_ALPHA_100    ///< alpha 100%
} gm_alpha_t;

#define MAX_OSG_WINDOW_NUM 64   ///< per encode object
typedef struct {
	int win_idx;    ///< Range: 0(upper layer) ~ 63(lower)
	///< Do unbind->apply will disable all settings
	int enabled;    ///< 0:enable, 1:disable
	int osg_img_idx;///< range:0 ~ 1023
	int x;          ///< range:0 ~ 4095 according to align_type.
	int y;          ///< range:0 ~ 4095 according to align_type.
	gm_alpha_t alpha;    ///< Only RGB1555 supported
	gm_align_type_t align_type;
	int reserved[5];
} gm_osg_window_t;

typedef enum {
	MEM_VAR_TYPE = 0,
	MEM_FIX_TYPE,
	MEM_UNKNOW_TYPE
} MEM_TYPE_t;

/***********************************
 *        MASK Structure
 ***********************************/
#define MAX_CAP_MASK_NUM 16   ///< per cap_vch
#define MAX_ENC_MASK_NUM 64   ///< per encode object
typedef struct {
	int win_idx;    ///< Capture object per channel (shared by all paths): 0(upper layer) ~ 15(lower)
	///< Encoder object per channel: 0(upper layer) ~ 63(lower)
	///< Do unbind->apply will disable all settings
	int enabled;    ///< 1:enable, 0:disable
	gm_dim_t    virtual_bg_dim; ///< user-defined background plane.
	gm_rect_t   virtual_rect;   ///< user-defined area based on the virtual_bg_dim.
	gm_alpha_t alpha;
	int palette_idx;    ///< Capture object support: 0 ~ 7
	///< Encoder object support: 0 ~ 15
	int reserved[6];
} gm_mask_t;


/***********************************
 *        Palette Structure
 ***********************************/
#define GM_MAX_PALETTE_IDX_NUM  16
typedef struct {
	int palette_table[GM_MAX_PALETTE_IDX_NUM]; ///< palette table, capture_mask: index 0 ~ 7, osg: index 0 ~ 15
} gm_palette_table_t;



char *pif_get_pool_name_by_type(vpbuf_pool_type_t pool_type);
int pif_valid_addr(void *addr);
int pif_init(void);
pif_group_t *pif_get_group_by_groupfd(int groupfd);
int pif_clear_group_id(pif_group_t *group);
int pif_preset_group(pif_group_t *group);
int pif_release(void);
int pif_apply(int groupfd);
int pif_apply_entity(HD_GROUP_LINE *group_line, vpd_entity_t *entity, int entity_body_len);
int pif_new_groupfd(void);
int pif_delete_groupfd(int groupfd);
int pif_poll(gm_pollfd_t *poll_fds, int num_fds, int timeout_msec);
int pif_recv_multi_bitstreams(gm_enc_multi_bitstream_t *multi_bs, int num_bs);
int pif_send_multi_bitstreams(gm_dec_multi_bitstream_t *multi_bs, int num_bs, int timeout_ms);
int pif_request_keyframe(HD_GROUP_LINE *group_line);

void pif_send_log(const char *fmt, ...);
int pif_clear_window(int lcd_vch, gm_clear_window_t *cw_str);
int pif_set_display_rate(int lcd_vch, int display_rate);
int pif_env_update(void);
int pif_check_dst_property_value(rect_t dst_rect, dim_t dst_bg_dim);
int pif_clear_vpd_channel_fd(int groupfd, int line_idx);
void pif_debug_cal_disp_rate(int cap_fps, int pb_fps, int lcd_fps, int ap_fps, int final_fps);

unsigned int pif_get_timestamp(void);
unsigned long pif_get_timestamp2(void);
int pif_get_duplicate_lcd_vch(pif_group_t *group);

int pif_user_get_buffer(pif_get_buf_t get_buf_info,
			gm_buffer_t *frame_buffer,
			gm_video_frame_info_t *frame_info,
			gm_bs_info_t *bs_info);

int pif_user_release_buffer(unsigned int linefd,
			    vpd_entity_type_t entity_type,
			    gm_buffer_t *frame_buffer);

int pif_pull_out_buffer(pif_get_buf_t get_buf_info,
			gm_buffer_t *frame_buffer,
			gm_bs_info_t *bs_info,
			vpd_get_copy_dout_t *dout,
			INT wait_ms);
int pif_release_out_buffer(unsigned int linefd,
			   vpd_entity_type_t entity_type,
			   vpd_get_copy_dout_t *dout);

int pif_set_mem_init(void *mem_init);
int pif_get_mem_init_state(void);
int pif_set_module_init(void *module_init);
int pif_clear_usr_blk(void);
int pif_clear_usr_pool_blk(char pool_name[], int ddr_id);
int pif_set_mem_uninit(void);
int pif_get_pool(void *pool_info);
int pif_get_usr_va_info(HD_COMMON_MEM_VIRT_INFO *p_vir_info);
int pif_get_chip_by_ddr_id(int ddr_id);
int pif_get_ddr_id_by_chip(int chip, int *p_start_ddr_no, int *p_end_ddr_no);
char pif_get_chipnu(void);
int pif_is_liveview(pif_group_t *group);
int pif_is_playback(pif_group_t *group);
int pif_cal_min_disp_fps(pif_group_t *group, int lcd_vch);
int pif_cal_disp_tick_fps(int lcd_vch);
int pif_cal_disp_max_fps(pif_group_t *group);
unsigned int pif_get_cap_channel_number(void);

void *pif_lookup_entity_buf(vpd_entity_type_t type, int sn, vpd_graph_t *vpd_graph);
int pif_set_EntityBuf(vpd_entity_type_t type, vpd_entity_t **entity, vpd_graph_t *vpd_graph);
int pif_set_line(vpd_graph_t *vpd_graph, vpd_line_t *vpd_line, void *line_buf);
int pif_address_ddr_sanity_check(UINTPTR pa, HD_COMMON_MEM_DDR_ID ddrid);
int pif_address_sanity_check(UINTPTR pa);
int pif_set_check_trigger_buf(vpd_chk_trigger_buf_t param);
int pif_get_check_trigger_buf(vpd_chk_trigger_buf_t *p_param);
void pif_get_disp_max_fps(int lcd_vch, unsigned int *max_fps);  // 60fps or 30fps(default)
MEM_TYPE_t pif_get_pool_mem_type(vpbuf_pool_type_t pool_type);
HD_COMMON_MEM_VB_BLK pif_get_blk_by_pa(UINT32 pa);
#endif //#ifndef __PIF_H__

