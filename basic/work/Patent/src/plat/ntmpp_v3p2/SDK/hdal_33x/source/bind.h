/**
 * @file bind.h
 * @brief type definition of HDAL.
 * @author PSW
 * @date in the year 2018
 */

#ifndef _HD_BIND_H_
#define _HD_BIND_H_
#ifdef  __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/

#include "hd_type.h"
#include "hdal.h"
#include "cif_list.h"

/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/

#define HD_DVR 0
#define HD_NVR 1

#define BD_MAX_BIND_GROUP 8	//Original value=16 spends lots of memory (1MB), update to 8

#define HD_MAX_NAME_STRING_LEN 32

#define BD_SET_PATH(dev_id, in_id, out_id)  (((dev_id) << 16) | (((in_id) & 0x00ff) << 8)| ((out_id) & 0x00ff))
#define BD_SET_CTRL(dev_id)			        (((dev_id) << 16) | HD_CTRL)

#define BD_SET_LINE_FD(group, line, s_idx, e_idx)	(((group & 0xff) << 24) | \
													 ((line & 0xff)  << 16) | \
													 ((s_idx & 0xff) << 8) | \
													 ((e_idx & 0xff)))
#define BD_SET_BMP_INDEX(bmp, val)			bmp |= (1 << (val))
#define BD_CLEAR_BMP_INDEX(bmp, val)		bmp &= (~(1 << (val)))
#define BD_IS_BMP_INDEX(bmp, val)			(((bmp) & (1 << (val))) != 0)
#define BD_IS_BMP_EMPTY(bmp)				((bmp) == 0)

#define BD_GET_IN_SUBID(in)  ((in) - HD_IN_BASE)
#define BD_GET_OUT_SUBID(out)  ((out) - HD_OUT_BASE)
#define BD_GET_IN_PORTID(in)  ((in) + HD_IN_BASE)
#define BD_GET_OUT_PORTID(out)  ((out) + HD_OUT_BASE)

#define HD_MAX_MULTI_PATH 8
#define HD_MAX_TRANSCODE_PATH_NUMBER 32

#define HD_MAX_GROUP_END_DEVICE 8
#define HD_MAX_DEVICE_BASEID 16
#define HD_MAX_GRAPH_LINE_PER_CHIP 128
#define HD_MAX_END_BIND_NUMBER 32
#define HD_MAX_BIND_NUNBER_PER_LINE 16 // must same with VPD_MAX_LINE_ENTITY_NUM
#define HD_MAX_GRAPH_LINE (HD_MAX_GRAPH_LINE_PER_CHIP * HD_MAX_GROUP_END_DEVICE + HD_MAX_TRANSCODE_PATH_NUMBER)
#define HD_MAX_GRAPH_RECURSIVE_LEVEL HD_MAX_BIND_NUNBER_PER_LINE

#define HD_MAX_BUF_INFO 4
#define HD_MAX_BUF_SET 256

#define BD_FD_MAX_PRIVATE_DATA	8

/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef enum _HD_PORT_STATE {
	HD_PORT_STOP,
	HD_PORT_START,
	ENUM_DUMMY4WORD(HD_PORT_STATE),
} HD_PORT_STATE;

typedef enum _HD_LINE_TYPE {
	HD_LINE_NONE,
	HD_LINE_VCAP_VENC,	// video encode
	HD_LINE_VDEC_VENC,	// video transcode
	HD_LINE_VCAP_DISP,	// video liveview
	HD_LINE_VDEC_DISP,	// video playback
	HD_LINE_DISP_VENC,	// display encode
	HD_LINE_IVS,
	HD_LINE_ACAP_AENC,	// audio encode
	HD_LINE_ADEC_AOUT,	// audio playback
	HD_LINE_ACAP_AOUT,	// audio livesound
	HD_LINE_VPROC_VOUT,	// YUV playback
	HD_LINE_VPROC_VENC,	// YUV record
	ENUM_DUMMY4WORD(HD_LINE_TYPE),
} HD_LINE_TYPE;

typedef enum _HD_LINE_STATE {
	HD_LINE_INCOMPLETE,
	HD_LINE_START_ONGOING,
	HD_LINE_START,
	HD_LINE_STOP_ONGOING,
	HD_LINE_STOP,
	ENUM_DUMMY4WORD(HD_LINE_STATE),
} HD_LINE_STATE;

typedef enum _HD_DEC_TX_BLACK_STATE {
	HD_DEC_INACTIVE,
	HD_DEC_NEW_START,
	HD_DEC_ALREADY_TX_BLOCK,
	HD_DEC_GO_STOP,
	ENUM_DUMMY4WORD(HD_DEC_TX_BLACK_STATE),
} HD_DEC_TX_BLACK_STATE;

typedef enum _HD_BIND_PORT_WAY {
	HD_BIND_1_TO_N_WAY,
	HD_BIND_N_TO_1_WAY,
	HD_BIND_N_TO_N_WAY,
	ENUM_DUMMY4WORD(HD_BIND_PORT_WAY),
} HD_BIND_PORT_WAY;

typedef enum _HD_BIND_STATE {
	HD_BIND,      ///< bind state
	HD_UNBIND,    ///< unbind
	ENUM_DUMMY4WORD(HD_BIND_STATE),
} HD_BIND_STATE;

typedef enum _HD_PATH_ID_STATE {
	HD_PATH_ID_CLOSED,
	HD_PATH_ID_OPEN,
	HD_PATH_ID_ONGOING,
	ENUM_DUMMY4WORD(HD_PATH_ID_STATE),
} HD_PATH_ID_STATE;

typedef enum _HD_UNIT_BIT {
	HD_BIT_VCAP,	///< 0, video capture
	HD_BIT_VDEC,	///< 1, video decode
	HD_BIT_H26XE,	///< 2, video H26x encode
	HD_BIT_JPGE,	///< 3, video JPG encode
	HD_BIT_VPE,		///< 4, video process engine
	HD_BIT_OSG,		///< 5, video osg
	HD_BIT_DI,		///< 6, video deinterlace engine
	HD_BIT_DATAIN,	///< 7, data in
	HD_BIT_DATAOUT,	///< 8, data out
	HD_BIT_ACAP,	///< 9, audio capture (audio grab)
	HD_BIT_AOUT,	///< 10, audio output (audio render)
	HD_BIT_LCD0,	///< 11, video display 0 (LCD300)
	HD_BIT_LCD1,	///< 12, video display 1 (LCD210_1)
	HD_BIT_LCD2,	///< 13, video display 2 (LCD210_2)
	HD_BIT_LCD3,	///< 14, video display 3 (channel zero)
	HD_BIT_LCD4,	///< 15, video display 4 (reserved)
	HD_BIT_LCD5,	///< 16, video display 5 (reserved)
	HD_BIT_RAWE,	///< 17, video raw encode
	ENUM_DUMMY4WORD(HD_UNIT_BIT),
} HD_UNIT_BIT;

typedef struct _HD_ABILITY {
	UINT32 max_device_nr;
	UINT32 max_out;
	UINT32 max_in;
} HD_ABILITY;

typedef struct _HD_BUF_NODE {
	CHAR pool_name[HD_MAX_NAME_STRING_LEN];
	struct list_head    src_listhead;
	struct list_head    dst_listhead;
} HD_BUF_NODE;

typedef struct _HD_QUERY_INFO {
	UINT32 in_subid; ///< for query purpose, device in port
	UINT32 out_subid;///< for query purpose, device out port
} HD_QUERY_INFO;

typedef struct _HD_DEV_INFO {
//    UINT32 type;
	HD_DAL deviceid;		///< device id
//	CHAR *name;     ///< device name.
} HD_DEV_INFO;

typedef struct _HD_DEV_NODE {
	struct list_head list;
	BOOL is_opened;
	UINT32 in_subid;
	UINT32 out_subid;
	VOID *p_bindfd;		///< gm bind bindfd point, only for end_device.
	VOID *p_object;		///< gm bind object point, start_device use out dnode, end_device use in dnode.
	HD_PORT_STATE port_state;	///< for out_subid state, START/STOP
	HD_BUF_NODE *bnode;  ///< device_out bnonde need to alloc, (device_in bnode = up layer_deivce_out bonde)
	VOID *p_bind_info;
} HD_DEV_NODE;

typedef struct _HD_BIND_INFO {
	HD_DEV_INFO device;
	HD_DEV_NODE *in;   ///< refer to HD_BIND_MAX_XXX_IN_PORT
	HD_DEV_NODE *out;  ///< refer to HD_BIND_MAX_XXX_OUT_PORT
} HD_BIND_INFO;

typedef struct _HD_DEVICE_INFO {
	HD_DAL device_baseid;
	CHAR *name;     		///< device name.
	BOOLEAN in_use;
	HD_ABILITY ability;
	HD_BIND_INFO *p_bind;
} HD_DEVICE_BASE_INFO;

typedef struct _HD_MEMBER {
	UINT32 in_subid;
	UINT32 out_subid;
	HD_BIND_INFO *p_bind;
} HD_MEMBER;

typedef struct _HD_OBJ_CTRL {
	VOID *p_bindfd;
	VOID *p_extra_bindfd;		//for case: display_enc(extra = bind(win, enc))
	VOID *p_in_object;
	VOID *p_out_object;
	VOID *p_extra_object;		//for case: dispaly_enc(cap->in_obj, win->out_obj, enc->ertra_obj)
} HD_OBJ_CTRL;

typedef struct _HD_LINE_LIST {
	HD_LINE_TYPE type;
	HD_LINE_STATE state;
	HD_OBJ_CTRL ctrl;
	HD_MEMBER member[HD_MAX_BIND_NUNBER_PER_LINE];
} HD_LINE_LIST;

typedef struct _BD_STAT_INFO {
	unsigned int count;
	unsigned int first_time;
} BD_STAT_INFO;

typedef struct _HD_GROUP_LINE {
	HD_LINE_TYPE type;
	HD_LINE_STATE state;
	HD_DEC_TX_BLACK_STATE dec_tx_state;
	UINT32 s_idx;	///< start member index
	UINT32 e1_idx;  ///< end 1 member index
	UINT32 e2_idx;  ///< end 2 member index
	HD_MEMBER member[HD_MAX_BIND_NUNBER_PER_LINE];  ///< bind members list
	HD_UNIT_BIT unit_bmp;

	//tmp use
	HD_PORT_STATE tmp_port_state;
	UINT is_zero_ch_vout;

	// vpd use
	UINT linefd;
	INT vpdFdinfo[BD_FD_MAX_PRIVATE_DATA];  //NOTE: size need same with vpd_channel_fd_t
	INT groupidx;
	BD_STAT_INFO stat_info;

} HD_GROUP_LINE;

typedef struct _HD_GROUP {
	BOOL in_use;
	BOOL is_mix_group;  //1: lv+pb+transcode+disp_enc, 0: others
	INT groupfd;
	HD_BIND_INFO *p_end_bind[HD_MAX_GROUP_END_DEVICE];
	HD_GROUP_LINE glines[HD_MAX_GRAPH_LINE];		///< current bind members list
	HD_GROUP_LINE prev_glines[HD_MAX_GRAPH_LINE];   ///< previous bind members list
	INT entity_buf_offs[HD_MAX_GRAPH_LINE][HD_MAX_BIND_NUNBER_PER_LINE];
	INT entity_buf_len;
} HD_GROUP;

typedef struct _HD_MULTI_PATH {
	HD_MEMBER front;
	HD_MEMBER end[HD_MAX_MULTI_PATH];
} HD_MULTI_PATH;

typedef struct _HD_TRANSCODE_PATH {
	INT max_in_use_idx;
	HD_MEMBER end[HD_MAX_TRANSCODE_PATH_NUMBER];
} HD_TRANSCODE_PATH;

typedef enum _HD_BUF_STATE {
	HD_BUF_AUTO = 0,
	HD_BUF_ENABLE = 1,
	HD_BUF_DISABLE = 2,
	ENUM_DUMMY4WORD(HD_BUF_STATE),
} HD_BUF_STATE;

typedef struct _HD_BUF_INFO {
	UINT32	type;
	INT		ddr_id;
	UINT32	counts;
	UINT32	max_counts;
	UINT32	min_counts;
	HD_BUF_STATE enable;
} HD_BUF_INFO;

typedef struct _HD_OUT_POOL {
	HD_BUF_INFO buf_info[HD_MAX_BUF_INFO];
} HD_OUT_POOL;

typedef enum _HD_GROUP_TYPE {
	DISP_CAP = 1,    /* liveview capture */
	DISP_VPE = 2,    /* liveview vpe & playback vpe*/
	CASCADE_VPE = 3, /*liveview with two vpe, and this group doesn't dismiss in fn_apply_start*/
	GROUP_FROM_IN_BUF = 0xF1000000,   // the group entities's in-buffer are the same (encode substream vpe)
	GROUP_TO_OUT_BUF = 0xF2000000,    // the group entities's out-buffer are the same (display vpe)

} HD_GROUP_TYPE;

typedef struct _HD_LV_CONFIG {
	HD_DIM cap_dim;
} HD_LV_CONFIG;

typedef struct _HD_LIVEVIEW_INFO {
	HD_LV_CONFIG cur;
	HD_LV_CONFIG pre;
} HD_LIVEVIEW_INFO;

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them
/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
HD_DAL bd_get_dev_baseid(HD_DAL deviceid);
UINT32 bd_get_dev_subid(HD_DAL deviceid);
UINT32 bd_get_chip_by_device(HD_DAL deviceid);
UINT32 bd_get_chip_by_bind(HD_BIND_INFO *p_bind);
CHAR *get_device_name(HD_DAL deviceid);
CHAR *get_bind_name(HD_BIND_INFO *p_bind);
HD_BIND_INFO *get_bind(HD_DAL deviceid);
HD_RESULT is_bind(HD_DAL deviceid, UINT32 subid, UINT32 inout);
INT32 get_bind_in_nr(HD_BIND_INFO *p_bind);
INT32 get_bind_out_nr(HD_BIND_INFO *p_bind);
BOOL is_end_device_p(HD_DAL deviceid);
BOOL is_start_device_p(HD_DAL deviceid);
BOOL is_middle_device_p(HD_DAL deviceid);
HD_BIND_PORT_WAY get_bind_in_out_way_p(HD_BIND_INFO *p_bind);
HD_RESULT is_bind_valid(HD_BIND_INFO *p_bind);
HD_RESULT bd_apply_attr(HD_DAL dev_id, HD_IO out_id, INT param_id);

HD_RESULT audio_poll_list(HD_AUDIOENC_POLL_LIST *p_poll, UINT32 nr, INT32 wait_ms);
HD_RESULT audio_recv_list(HD_AUDIOENC_RECV_LIST *p_audioenc_bs, UINT32 nr);
HD_RESULT audio_send_list(HD_AUDIODEC_SEND_LIST *p_audiodec_bs, UINT32 nr, INT32 wait_ms);

HD_RESULT video_poll_list(HD_VIDEOENC_POLL_LIST *p_poll, UINT32 nr, INT32 wait_ms);
HD_RESULT video_recv_list(HD_VIDEOENC_RECV_LIST *p_videoenc_bs, UINT32 nr);
HD_RESULT video_send_list(HD_VIDEODEC_SEND_LIST *p_videodec_bs, UINT32 nr, INT32 wait_ms);

HD_RESULT bd_bind(HD_DAL src_deviceid, HD_IO out_id, HD_DAL dst_deviceid, HD_IO in_id);
HD_RESULT bd_unbind(HD_DAL self_deviceid, HD_IO out_id);
HD_RESULT bd_device_open(HD_DAL deviceid, HD_IO out_id, HD_IO in_id, HD_PATH_ID *p_path_id);
HD_RESULT bd_device_close(HD_PATH_ID path_id);
HD_RESULT bd_get_already_open_pathid(HD_DAL deviceid, HD_IO out_id, HD_IO in_id, HD_PATH_ID *p_path_id);

HD_RESULT bd_device_init(HD_DAL usr_device_baseid, UINT32 max_did, UINT32 max_device_in, UINT max_device_out);
HD_RESULT bd_device_uninit(HD_DAL usr_device_baseid);
HD_RESULT bd_dal_init(void);
HD_RESULT bd_dal_uninit(void);
HD_RESULT bd_start_list(HD_PATH_ID *path_id, UINT nr);
HD_RESULT bd_stop_list(HD_PATH_ID *path_id, UINT nr);

HD_RESULT bd_get_next(HD_DAL deviceid, HD_IO out_id, char next_name[], int next_len);
HD_RESULT bd_get_prev(HD_DAL deviceid, HD_IO in_id, char prev_name[], int prev_len);
UINT bd_check_exist(HD_PATH_ID path_id);
UINT bd_is_path_start(HD_PATH_ID path_id);

HD_BIND_INFO *get_next_bind(HD_BIND_INFO *p_bind, UINT32 out_subid, HD_QUERY_INFO *query);
HD_BIND_INFO *get_prev_bind(HD_BIND_INFO *p_bind, UINT32 in_subid, HD_QUERY_INFO *query);

#ifdef  __cplusplus
}
#endif


#endif  /* _HD_BIND_H_ */
