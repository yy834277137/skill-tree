/*
 *   @file   common_param.h
 *
 *   @brief  Common parameters are defined in this file
 *
 *   Header file for common parameters.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#ifndef __COMMON_PARAM_H__
#define __COMMON_PARAM_H__

#define VIDEOENC_PARAM_H_VER    0x20200120

#include "kwrap/cpu.h"   // this file in VOS include path

typedef struct {
	UINT32  w;      ///< Rectangle width
	UINT32  h;      ///< Rectangle height
} VG_USIZE;


typedef enum {
	VG_CODEC_TYPE_JPEG = 0,
	VG_CODEC_TYPE_H264,
	VG_CODEC_TYPE_H265,
} VG_CODEC_TYPE;


typedef enum {
	//H264 profile
	VG_VDOENC_H264E_BASELINE_PROFILE = 66,
	VG_VDOENC_H264E_MAIN_PROFILE = 77,
	VG_VDOENC_H264E_HIGH_PROFILE = 100,

	//H265 profile
	VG_VDOENC_H265E_MAIN_PROFILE = 1
} VG_VDOENC_PROFILE;

typedef enum {
	//H264 level
	VG_VDOENC_H264E_LEVEL_1 = 10,
	VG_VDOENC_H264E_LEVEL_1_1 = 11,
	VG_VDOENC_H264E_LEVEL_1_2 = 12,
	VG_VDOENC_H264E_LEVEL_1_3 = 13,
	VG_VDOENC_H264E_LEVEL_2 = 20,
	VG_VDOENC_H264E_LEVEL_2_1 = 21,
	VG_VDOENC_H264E_LEVEL_2_2 = 22,
	VG_VDOENC_H264E_LEVEL_3 = 30,
	VG_VDOENC_H264E_LEVEL_3_1 = 31,
	VG_VDOENC_H264E_LEVEL_3_2 = 32,
	VG_VDOENC_H264E_LEVEL_4 = 40,
	VG_VDOENC_H264E_LEVEL_4_1 = 41,
	VG_VDOENC_H264E_LEVEL_4_2 = 42,
	VG_VDOENC_H264E_LEVEL_5 = 50,
	VG_VDOENC_H264E_LEVEL_5_1 = 51,

	//H265 level
	VG_VDOENC_H265E_LEVEL_1 = 30,
	VG_VDOENC_H265E_LEVEL_2 = 60,
	VG_VDOENC_H265E_LEVEL_2_1 = 63,
	VG_VDOENC_H265E_LEVEL_3 = 90,
	VG_VDOENC_H265E_LEVEL_3_1 = 93,
	VG_VDOENC_H265E_LEVEL_4 = 120,
	VG_VDOENC_H265E_LEVEL_4_1 = 123,
	VG_VDOENC_H265E_LEVEL_5 = 150,
	VG_VDOENC_H265E_LEVEL_5_1 = 153,
	VG_VDOENC_H265E_LEVEL_5_2 = 156,
	VG_VDOENC_H265E_LEVEL_6 = 180,
	VG_VDOENC_H265E_LEVEL_6_1 = 183,
	VG_VDOENC_H265E_LEVEL_6_2 = 186

} VG_VDOENC_LEVEL;

typedef enum {
	//H264 coding
	VG_VDOENC_H264E_CAVLC_CODING = 0,
	VG_VDOENC_H264E_CABAC_CODING = 1,

	//H265 coding
	VG_VDOENC_H265E_CABAC_CODING = 1
} VG_VDOENC_CODING;

typedef enum {
	VG_VDOENC_SVC_DISABLE = 0,
	VG_VDOENC_SVC_2X = 1,
	VG_VDOENC_SVC_4X = 2
} VG_VDOENC_SVC_LAYER;

typedef enum {
	VG_VDOENC_RC_MODE_CBR = 1,
	VG_VDOENC_RC_MODE_VBR = 2,
	VG_VDOENC_RC_MODE_FIX_QP = 3,
	VG_VDOENC_RC_MODE_EVBR = 4
} VG_VDOENC_RC_MODE;

typedef enum {
	VG_JPEGENC_RC_MODE_CBR = 0,
	VG_JPEGENC_RC_MODE_VBR = 1,
	VG_JPEGENC_RC_MODE_FIX_QP = 2,
} VG_JPEGENC_RC_MODE;

typedef struct {
	UINT32 max_channels;    ///< [r] max channels of video encoder
	VG_USIZE max_dim;     ///< [r] max dimension of video encoder
} VG_PARAM_SYSINFO;


/************ encode feature ************/
typedef struct {
#if 0
	VG_CODEC_TYPE codec_type;     ///< [w] codec type
	UINT32 encode_width;            ///< [w] encode width.
	UINT32 encode_height;           ///< [w] encode height.
	VG_PIX_FMT source_format;     ///< [w] source format
#endif
	UINT32 source_y_line_offset;    ///< [w] source line offset (luma)
	UINT32 source_c_line_offset;    ///< [w] source line offset (chroma)
	UINT32 source_chroma_offset;    ///< [w] source chroma offset
} VG_ENC_BUF_PARAM;

typedef struct {
	UINT32 gop_num;                 ///< [r/w] I frame period. range: 0~4096 (0: only one I frame)
	INT8 chrm_qp_idx;               ///< [r/w] chroma qp offset.        default: 0, range: -12~12
	INT8 sec_chrm_qp_idx;           ///< [r/w] second chroma QP offset. default: 0, range: -12~12
	UINT32 ltr_interval;            ///< [r/w] long-term reference frame interval. default: 0, range: 0~4095 (0: disable)
	BOOL ltr_pre_ref;               ///< [r/w] long-term reference setting. default: 0, range: 0~1 (0: all long-term reference to IDR frame, 1: reference latest long-term reference frame)
	BOOL gray_en;                   ///< [r/w] encode color to gray. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT8 rotate;                   ///< [r/w] rotate source image. default: 0, range: 0~3 (0: disable, 1: 90 degrees, 2: 270 degrees, 3: 180 degrees)
	BOOL source_output;             ///< [r/w] Source output. default: 0, range: 0~1 (0: disable, 1: enable)
	VG_VDOENC_PROFILE profile;    ///< [r/w] Profile IDC. default(H.264): VDOENC_H264E_HIGH_PROFILE, default(H.265): VDOENC_H265E_MAIN_PROFILE
	VG_VDOENC_LEVEL level_idc;    ///< [r/w] Level IDC. default(H.264): VDOENC_H264E_LEVEL_4_1, default(H.265): VDOENC_H265E_LEVEL_5_0
	VG_VDOENC_SVC_LAYER svc_layer;///< [r/w] SVC Layer. default: 0, range: 0~2 (0: disable, 1: 2x, 2: 4x)
	VG_VDOENC_CODING entropy_mode;///< [r/w] Entropy coding method. default: 1, range(H.264): 0~1, range(H.265): 1 (0: CAVLC, 1: CABAC)
} VG_H26XENC_FEATURE;

/************ vui ************/
typedef struct {
	UINT8 vui_en;                   ///< [r/w] enable vui. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT32 sar_width;               ///< [r/w] Horizontal size of the sample aspect ratio. default: 0, range: 0~65535
	UINT32 sar_height;              ///< [r/w] Vertical size of the sample aspect rat. default: 0, range: 0~65535
	UINT8 matrix_coef;              ///< [r/w] Matrix coefficients are used to derive the luma and Chroma signals from green, blue, and red primaries. default: 2, range: 0~255
	UINT8 transfer_characteristics; ///< [r/w] The opto-electronic transfers characteristic of the source pictures. default: 2, range: 0~255
	UINT8 colour_primaries;         ///< [r/w] Chromaticity coordinates the source primaries. default: 2, range: 0~255
	UINT8 video_format;             ///< [r/w] Indicate the representation of pictures. default: 5, range: 0~7
	UINT8 color_range;              ///< [r/w] Indicate the black level and range of the luma and Chroma signals. default: 0, range: 0~1 (0: Not full range, 1: Full range)
	UINT8 timing_present_flag;  ///< [r/w] timing info present flag. default: 0, range: 0~1 (0: disable, 1: enable)
} VG_H26XENC_VUI;

/************ deblock ************/
typedef struct {
	UINT8 dis_ilf_idc;      ///< [r/w] Disable loop filter in slice header. default: 0, range: 0~2 (0: Filter, 1: No Filter, 2: Slice Mode)
	INT8 db_alpha;          ///< [r/w] Alpha & C0 offset. default: 0, range: -12~12
	INT8 db_beta;           ///< [r/w] Beta offset.       default: 0, range: -12~12
} VG_H26XENC_DEBLOCK;

/************ rc ************/
typedef struct {
	UINT32 bitrate;           ///< [r/w] Bit rate (bits per second)
	UINT32 frame_rate_base;   ///< [r/w] Frame rate
	UINT32 frame_rate_incr;   ///< [r/w] Frame rate = uiFrameRateBase / uiFrameRateIncr
	UINT8 init_i_qp;          ///< [r/w] Rate control's init I qp.        default: 26, range: 0~51
	UINT8 max_i_qp;           ///< [r/w] Rate control's max I qp.         default: 51, range: 0~51
	UINT8 min_i_qp;           ///< [r/w] Rate control's min I qp.         default:  1, range: 0~51
	UINT8 init_p_qp;          ///< [r/w] Rate control's init P qp.        default: 26, range: 0~51
	UINT8 max_p_qp;           ///< [r/w] Rate control's max P qp.         default: 51, range: 0~51
	UINT8 min_p_qp;           ///< [r/w] Rate control's min P qp.         default:  1, range: 0~51
	UINT32 static_time;       ///< [r/w] Rate control's static time.      default:  0, range: 0~20
	INT32 ip_weight;          ///< [r/w] Rate control's I/P frame weight. default:  0, range: -100~100
} VG_H26XENC_CBR;

typedef struct {
	UINT32 bitrate;           ///< [r/w] Bit rate (bits per second)
	UINT32 frame_rate_base;   ///< [r/w] Frame rate
	UINT32 frame_rate_incr;   ///< [r/w] Frame rate = uiFrameRateBase / uiFrameRateIncr
	UINT8 init_i_qp;          ///< [r/w] Rate control's init I qp.        default: 26, range: 0~51
	UINT8 max_i_qp;           ///< [r/w] Rate control's max I qp.         default: 51, range: 0~51
	UINT8 min_i_qp;           ///< [r/w] Rate control's min I qp.         default:  1, range: 0~51
	UINT8 init_p_qp;          ///< [r/w] Rate control's init P qp.        default: 26, range: 0~51
	UINT8 max_p_qp;           ///< [r/w] Rate control's max P qp.         default: 51, range: 0~51
	UINT8 min_p_qp;           ///< [r/w] Rate control's min P qp.         default:  1, range: 0~51
	UINT32 static_time;       ///< [r/w] Rate control's static time.      default:  0, range: 0~20
	INT32 ip_weight;          ///< [r/w] Rate control's I/P frame weight. default:  0, range: -100~100
	UINT32 change_pos;        ///< [r/w] Early limit bitate.              default:  0, range: 0~100 (0: disable)
} VG_H26XENC_VBR;

typedef struct {
	UINT32 frame_rate_base;   ///< [r/w] Frame rate
	UINT32 frame_rate_incr;   ///< [r/w] Frame rate = uiFrameRateBase / uiFrameRateIncr
	UINT8 fix_i_qp;           ///< [r/w] Fix qp of I frame. default: 26, range: 0~51
	UINT8 fix_p_qp;           ///< [r/w] Fix qp of P frame. default: 26, range: 0~51
} VG_H26XENC_FIXQP;

typedef struct {
	UINT32 bitrate;             ///< [r/w] Bit rate (bits per second)
	UINT32 frame_rate_base;     ///< [r/w] Frame rate
	UINT32 frame_rate_incr;     ///< [r/w] Frame rate = uiFrameRateBase / uiFrameRateIncr
	UINT8 init_i_qp;            ///< [r/w] Rate control's init I qp.        default: 26, range: 0~51
	UINT8 max_i_qp;             ///< [r/w] Rate control's max I qp.         default: 51, range: 0~51
	UINT8 min_i_qp;             ///< [r/w] Rate control's min I qp.         default:  1, range: 0~51
	UINT8 init_p_qp;            ///< [r/w] Rate control's init P qp.        default: 26, range: 0~51
	UINT8 max_p_qp;             ///< [r/w] Rate control's max P qp.         default: 51, range: 0~51
	UINT8 min_p_qp;             ///< [r/w] Rate control's min P qp.         default:  1, range: 0~51
	UINT32 static_time;         ///< [r/w] Rate control's static time.      default:  0, range: 0~20
	INT32 ip_weight;            ///< [r/w] Rate control's I/P frame weight. default:  0, range: -100~100
	UINT32 key_p_period;        ///< [r/w] Key P frame interval.  default: frame rate*2, range: 0~4096
	INT32 kp_weight;            ///< [r/w] Rate control's KP/P frame weight. default: 0, range: -100~100
	UINT32 still_frame_cnd;     ///< [r/w] Condition of still environment of EVBR. default: 100, range: 1~4096
	UINT32 motion_ratio_thd;    ///< [r/w] Threshold of motion ratio to decide motion frame and still frame. default: 30, range: 1~100
	INT32 motion_aq_str;        ///< [r/w] Motion aq strength for smart ROI. default: -6, range: -15~15
	UINT32 still_i_qp;          ///< [r/w] Still mode qp of I frame.        default: 28, range: 0~51
	UINT32 still_p_qp;          ///< [r/w] Still mode qp of P frame.        default: 30, range: 0~51
	UINT32 still_kp_qp;         ///< [r/w] Still mode qp of key P frame.    default: 36, range: 0~51
} VG_H26XENC_EVBR;

typedef struct {
	VG_VDOENC_RC_MODE rc_mode; ///< [r/w] rate control mode. default: 1, range: 1~4 (1: CBR, 2: VBR, 3: FixQP, 4: EVBR)
	union {
		VG_H26XENC_CBR cbr;        ///< [r/w] parameter of rate control mode CBR
		VG_H26XENC_VBR vbr;        ///< [r/w] parameter of rate control mode VBR
		VG_H26XENC_FIXQP fixqp;    ///< [r/w] parameter of rate control mode FixQP
		VG_H26XENC_EVBR evbr;      ///< [r/w] parameter of rate control mode EVBR
	} rc_param;
} VG_H26XENC_RATE_CONTROL;

typedef struct {
	UINT32 bitrate;	                ///< [r/w] bit rate
	UINT32 frame_rate_base;         ///< [r/w] frame rate
	UINT32 frame_rate_incr;         ///< [r/w] frame rate = frame_rate_base / frame_rate_incr
	UINT32 base_qp;                 ///< [r/w] base qp
	UINT32 min_quality;             ///< [r/w] min quality
	UINT32 max_quality;             ///< [r/w] max quality
	VG_JPEGENC_RC_MODE vbr_mode;                ///< [r/w] 0: CBR, 1: VBR
} VG_JPEGENC_RATE_CONTROL;

/********* user-defined QP map *********/
typedef struct {
	BOOL enable;            ///< [r/w] enable user qp. default: 0, range: 0~1 (0: disable, 1: enable)
	uintptr_t qp_map_addr;     ///< [r/w] buffer address of user qp map. two byte per cu16
							///<       bit[0:5] qp value (default: 0; if qp mode is 3 then qp value means fixed qp [range: 0~51], otherwise qp value means delta qp [range: -32~31])
							///<       bit[6:7] qp mode (default: 0; 0: delta qp, 1: reserved, 2: delta qp [disable AQ], 3: fixed qp)
							///<       bit[8:9] fro mode (default: 0; 0: normal, 1: reserved, 2: disable FRO, 3: reserved)
							///<       bit[12] lpm mode (default: 0; 0: disable, 1: enable)
							///<       bit[13] roi label (default: 0; 0: false, 1: true)
} VG_H26XENC_USR_QP;

/************ multi slice ************/
typedef struct {
	UINT32 enable;          ///< [r/w] enable multiple slice. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT32 slice_row_num;   ///< [r/w] number of macroblock/ctu rows occupied by a slice, range: 1 ~ number of macroblock/ctu row
} VG_H26XENC_SLICE_SPLIT;

/************ mask ************/
typedef struct {
	UINT16 pos_x;              ///< [r/w] mask window postion x. range: 0 ~ encode width - 1
	UINT16 pos_y;              ///< [r/w] mask window postion y. range: 0 ~ encode height - 1
} VG_H26XENC_MASK_POS;

typedef struct {
	UINT8 index;            ///< [r/w] index of mask. default: 0, range: 0~7
	BOOL enable;            ///< [r/w] enable mask window. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT8 did;              ///< [r/w] mask window de-identified method. range: 0~1 (0: orignial, 1: mosaic)
	UINT8 line_hit_opt;     ///< [r/w] mask window line hit operation. 0: and, 1: or, 2: border
	UINT16 alpha;           ///< [r/w] mask window blending alpha. range : 0 ~ 256
	VG_H26XENC_MASK_POS st_pos[4];    ///< [r/w] mask window position configure settings.
	UINT8 pal_y;            ///< [r/w] palette colors y. range: 0 ~ 255
	UINT8 pal_cb;           ///< [r/w] palette colors cb. range: 0 ~ 255
	UINT8 pal_cr;           ///< [r/w] palette colors cr. range: 0 ~ 255
} VG_H26XENC_MASK_WIN;

typedef struct {
	UINT8 mosaic_blk_w;       ///< [r/w] mosaic block width. range: 0 ~ 255
	UINT8 mosaic_blk_h;       ///< [r/w] mosaic block height. range: 0 ~ 255
} VG_H26XENC_MASK_INIT;

/************ gdr ************/
typedef struct {
	BOOL enable;            ///< [r/w] enable gdr. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT32 period;          ///< [r/w] intra refresh period. default: 0, range: 0~0xFFFFFFFF (0: always refresh, others: intra refresh frame period)
	UINT32 number;          ///< [r/w] intra refresh row number. default: 1, range: 1 ~ number of macroblock/ctu row
} VG_H26XENC_GDR;

/************ roi ************/
#define VG_H26XENC_ROI_WIN_COUNT 10
typedef struct {
	BOOL enable;             ///< [r/w] enable roi qp. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT16 coord_X;          ///< [r/w] coordinate x of roi. range: 0~encode width -1
	UINT16 coord_Y;          ///< [r/w] coordinate y of roi. range: 0~encode height-1
	UINT16 width;            ///< [r/w]  width of roi. range: 0~encode width -1
	UINT16 height;           ///< [r/w] height of roi. range: 0~encode height-1
	UINT8 mode;              ///< [r/w] qp mode. default: 0, range: 0~3 (0: delta qp, 1: reserved, 2: delta qp [disable AQ], 3: fixed qp)
	INT8 qp;                 ///< [r/w] qp value. default: 0; if qp mode is 3 then qp value means fixed qp (range: 0~51), otherwise qp value means delta qp (range: -32~31)
} VG_H26XENC_ROI_WIN;

typedef struct {
	VG_H26XENC_ROI_WIN st_roi[VG_H26XENC_ROI_WIN_COUNT]; ///< [r/w] roi window settings. ROIs can be overlaid, and the priority of the ROIs is based on index number, index 0 is highest priority and index 9 is lowest.
} VG_H26XENC_ROI;

/************ row rc ************/
typedef struct {
	BOOL enable;             ///< [r/w] enable row rc. default: 1, range: 0~1 (0: disable, 1: enable)
	UINT8 i_qp_range;   ///< [r/w] qp range of I frame for row-level rata control. default: 2, range: 0~15
	UINT8 i_qp_step;    ///< [r/w] qp step  of I frame for row-level rata control. default: 1, range: 0~15
	UINT8 p_qp_range;   ///< [r/w] qp range of P frame for row-level rata control. default: 4, range: 0~15
	UINT8 p_qp_step;    ///< [r/w] qp step  of P frame for row-level rata control. default: 1, range: 0~15
	UINT8 min_i_qp;      ///< [r/w] min qp of I frame for row-level rata control. default:  1, range: 0~51
	UINT8 max_i_qp;      ///< [r/w] max qp of I frame for row-level rata control. default: 51, range: 0~51
	UINT8 min_p_qp;      ///< [r/w] min qp of P frame for row-level rata control. default:  1, range: 0~51
	UINT8 max_p_qp;      ///< [r/w] max qp of P frame for row-level rata control. default: 51, range: 0~51
} VG_H26XENC_ROW_RC;

/************ aq ************/
#define VG_H26XENC_AQ_MAP_TABLE_COUNT 30
typedef struct {
	BOOL enable;              ///< [r/w] AQ enable. default: 0, range: 0~1 (0: disable, 1: enable)
	UINT8 i_str;              ///< [r/w] aq strength of I frame. default: 3, range: 1~8
	UINT8 p_str;              ///< [r/w] aq strength of P frame. default: 1, range: 1~8
	UINT8 max_delta_qp;       ///< [r/w] max delta qp of aq. default: -6, range: -15~0
	UINT8 min_delta_qp;       ///< [r/w] min delta qp of aq. default:  6, range:  0~15
	UINT8 depth; ///< [r/w] AQ depth. default: 2, range(H.264): 2, range(H.265): 0~2 (0: cu64, 1: cu32, 2: cu16)
	INT16 thd_table[VG_H26XENC_AQ_MAP_TABLE_COUNT]; ///< [r/w] non-linear AQ mapping table. range: -512~511, default: {-120,-112,-104, -96, -88, -80, -72, -64, -56, -48, -40, -32, -24, -16, -8, 7, 15, 23, 31, 39,47, 55, 63, 71, 79, 87, 95, 103, 111, 119}
						 ///<       for ( dqp = -15; dqp < 15; dqp++ ) if ( Cu.RelativeTextureComplexity <= thd_table[dqp+15] ) break; Cu.DeltaQP_AQ = dqp;
} VG_H26XENC_AQ;

/************ lpm ************/
typedef struct {
	UINT8 enable; ///< [r/w] low power mode. default: 0, range: 0~2 (0: disable, 1: control by cu16, 2: enable)
} VG_H26XENC_LPM;

/************ rnd ************/
typedef struct {
	BOOL enable;            ///< [r/w] random noise enable. default: 0, range: 0~1 (0: disbale, 1: enable)
	UINT8 range;  ///< [r/w] noise range for the random noise added to the source. default: 1, range: 1~10
} VG_H26XENC_RND;

/************ osg ************/
typedef struct {
	UINT8 type;               ///< [r/w] osg graphic type. range: 0~1 (0: argb1555, 1: argb8888)
	UINT16 width; ///< [r/w] osg graphic width and this field shall be mulitple of 8. range: 0~encode width-1
	UINT16 height;        ///< [r/w] osg grahic height and this field shall be mulitple of 2. range: 0~510
	uintptr_t pa_addr;            ///< [r/w] osg graphic data physical address.
	uintptr_t va_addr;            ///< [r/w] osg graphic data virtual address.
} VG_H26XENC_OSG_GRAP;

typedef struct {
	UINT16 x_str; ///< [r/w] osg display position of x and this field shall be multiple of 2. range: 0~encode width-2
	UINT16 y_str; ///< [r/w] osg display position of y and this field shall be multiple of 2. range: 0~encode hieght-2
	UINT8 bg_alpha;           ///< [r/w] osg display foreground trnsparency. range: 0~7
	UINT8 fg_alpha;           ///< [r/w] osg display background trnsparency. range: 0~7
} VG_H26XENC_OSG_DISP;

typedef struct {
	BOOL enable;             ///< [r/w] osg gcac enable. range: 0~1 (0: disable, 1: enable)
	UINT8 org_color_lv; ///< [r/w] osg gcac orginal color judgment level. range: 0~7 (0: 100%, 1: more than 87.5%, 2: more than 75% ... 7: more than 12.5%)
	UINT8 inv_color_lv; ///< [r/w] osg gcac complementary color judgment level. range: 0~7 (0: 100%, 1: more than 87.5%, 2: more than 75% ... 7: more than 12.5%)
} VG_H26XENC_OSG_GCAC;

typedef struct {
	UINT8 index; ///< [r/w] index of osg window. default: 0, range: 0~9
	BOOL enable;             ///< [r/w] osg window enable bit. range: 0~1 (0: disable, 1: enable)
	VG_H26XENC_OSG_GRAP st_grap;      ///< [r/w] osg window graphic(input) configure settings.
	VG_H26XENC_OSG_DISP st_disp;      ///< [r/w] osg window display(output) configure settings.
	VG_H26XENC_OSG_GCAC st_gcac;      ///< [r/w] osg window gcac configure settings.
} VG_H26XENC_OSG_WIN;

typedef struct {
	UINT8 rgb_to_yuv[3][3];   ///< [r/w] osg color space transformation matrix. range: 0~255
} VG_H26XENC_OSG_INIT;

/************ motion aq ************/
typedef struct {
	UINT8 mode; ///< [r/w] mode for delta QPs in the motion area. default: 0, range: 0~3 (0: DeltaQP_AQ, 1: DeltaQP_AQ + DeltaQP_Motion, 2: min(DeltaQP_AQ, DeltaQP_Motion), 3: DeltaQP_Motion)
	UINT8 delta_qp_roi_th;    ///< [r/w] threshold for ROI labeling. default: 3, range: 0~3
							  ///<       if ( Cu16.MotionCount > 2*delta_qp_roi_th ) Cu16.isROI = True; (Note: Cu16.MotionCount is ranged from 0 to 6)
	INT8 delta_qp[6];   ///< [r/w] delta QPs for motion-based AQ. default: 0, range: -16~15 (-16: disable)
} VG_H26XENC_MOTION_AQ;

/************ tmnr ************/
typedef struct {
	BOOL enable;          ///< [r/w] Y channel TMNR on/off. default: 1, range: 0~1 (0: disable, 1: enable)
	UINT8 avoid_residue_th; ///< [r/w] Y channel temporal absolute difference bigger than this threshold will not be suppressed to zero after NR. default: 1, range 1~4
	UINT8 nr_str_3d;          ///< [r/w] Y channel temporal NR strength. default: 4, range: 0~32
	UINT8 nr_str_2d;          ///< [r/w] Y channel  spatial NR strength. default: 4, range: 0~32
	BOOL small_vibrat_supp_en; ///< [r/w] Y channel small vibration suppression ON/OFF. default: 0, range 0~1
	BOOL center_wzero_2d_en; ///< [r/w] Apply zero to center weight of Y  spatial Bilateral filter. default: 1, range 0~1
	BOOL center_wzero_3d_en; ///< [r/w] Apply zero to center weight of Y temporal Bilateral filter. default: 1, range 0~1
	UINT8 blur_str; ///< [r/w] Difference Y image blurred strength. default: 1, range 0~2 (0: No blur, 1: low-strength blur, 2: high-strength blur)
	UINT8 lut_2d_th[4];      ///< [r/w] LUT of Y channel 2D   filter. default: {11,33,55,77}, range: 0~127
	UINT8 lut_3d_1_th[4];    ///< [r/w] LUT of Y channel 3D_1 filter. default: {11,33,55,77}, range: 0~127
	UINT8 lut_3d_2_th[4];    ///< [r/w] LUT of Y channel 3D_2 filter. default: {40,14, 7, 3}, range: 0~127
	UINT8 motion_level_th_k1; ///< [r/w] Y channel motion level threshold 1 adjustment. default:  8, range: 0~32
	UINT8 motion_level_th_k2; ///< [r/w] Y channel motion level threshold 2 adjustment. default: 16, range: 0~32 (Note: K2 > K1)
	UINT8 motion_level_th2_k1; ///< [r/w] Y channel motion level threshold 1 adjustment. default:  8, range: 0~32
	UINT8 motion_level_th2_k2; ///< [r/w] Y channel motion level threshold 2 adjustment. default: 16, range: 0~32 (Note: K2 > K1)
	UINT16 base[8]; ///< [r/w] Y channel noise model parameter: base noise level. default: {100,100,100,100,100,100,100,100}, range: 0~255*64
	UINT16 std[8]; ///< [r/w] Y channel noise model parameter: noise standard deviation. default: {20,20,20,20,20,20,20,20}, range: 0~255*64
	UINT8 coefa[8]; ///< [r/w] Y channel noise model parameter: slope of line. default: {8,8,8,8,8,8,8,8}, range: 0~48
	UINT16 coefb[8]; ///< [r/w] Y channel noise model parameter: intercept of line. default: {100,100,100,80,70,50,50,50}, range: 0~255*64
} VG_H26XENC_TMNR_LUMA;

typedef struct {
	BOOL enable;       ///< [r/w] CbCr channel TMNR on/off. default: 1, range: 0~1 (0: disable, 1: enable)
	UINT8 avoid_residue_th; ///< [r/w] CbCr channel temporal absolute difference bigger than this threshold will not be suppressed to zero after NR. default: 1, range 1~4
	UINT8 nr_str_3d;          ///< [r/w] CbCr channel temporal NR strength. default: 4, range: 0~32
	UINT8 nr_str_2d;          ///< [r/w] CbCr channel  spatial NR strength. default: 4, range: 0~32
	UINT8 lut_2d_th[4];     ///< [r/w] LUT of CbCr channel 2D filter. default: {11,33,55,77}, range: 0~127
	UINT8 lut_3d_th[4];     ///< [r/w] LUT of CbCr channel 3D filter. default: {37,19,11, 7}, range: 0~127
	UINT8 motion_level_th_k1; ///< [r/w] CbCr channel motion level threshold 1 adjustment. default:  8, range: 0~32
	UINT8 motion_level_th_k2; ///< [r/w] CbCr channel motion level threshold 2 adjustment. default: 16, range: 0~32 (Note: K2 > K1)
	UINT16 cb_mean[8]; ///< [r/w] Cb channel noise model parameter: noise mean. default: {100,100,100,100,100,100,100,100}, range: 0~255*25
	UINT16 cr_mean[8]; ///< [r/w] Cr channel noise model parameter: noise mean. default: {100,100,100,100,100,100,100,100}, range: 0~255*25
	UINT16 cb_std[8]; ///< [r/w] Cb channel nosie model parameter: noise standard deviation. default: {20,20,20,20,20,20,20,20}, range 0~255*25
	UINT16 cr_std[8]; ///< [r/w] Cr channel nosie model parameter: noise standard deviation. default: {20,20,20,20,20,20,20,20}, range 0~255*25
} VG_H26XENC_TMNR_CHRM;

typedef struct {
	BOOL enable;             ///< [r/w] tmnr on/off. default: 1, range: 0~1 (0: disable, 1: enable)
	UINT8 tmnr_fcs_en;          ///< [r/w] TMNR False Color Supression enable. default:0, range: 0~1 (Note: chroma_dn_en shoulde be 1)
	UINT8 tmnr_fcs_str;         ///< [r/w] TMNR_False Color Supression strength. default: 4, range: 0~15
	UINT8 tmnr_fcs_th;          ///< [r/w] TMNR False Color Supression threshold. default: 32, range: 0~255
	VG_H26XENC_TMNR_LUMA st_luma;     ///< [r/w] tmnr y parameter
	VG_H26XENC_TMNR_CHRM st_chrm;     ///< [r/w] tmnr cbcr parameter
} VG_H26XENC_TMNR;

typedef struct {
	UINT32 retstart_interval;  ///< [r/w] JPEG restart interval. default: 0, range: 0~image size/256
	BOOL image_quality;        ///< [r/w] JPEG image quality. default: 50, range 1~100
} VG_JPEG_CONFIG;

typedef struct {
	//(Input) Fill the following arguments
	VG_USIZE dim;                     ///< [w] dimension of encode size
	VG_VDOENC_SVC_LAYER svc_layer;    ///< [w] svc layer (0: disable, 1: 2x, 2: 4x)
	UINT32 ltr_enable;                  ///< [w] ltr enable

	//(Output) The buf_size is returned
	UINT32 buf_size;                    ///< [r] size of encode internal buffer
} VG_H26XENC_INTERNAL_BUF_SIZE;



typedef enum {
	VDOENC_ENC_PARAM,              ///< [w],   use structure VG_ENCPARAM
	VDOENC_H26X_FEATURE,           ///< [r/w], use structure VG_H26XENC_FEATURE
	VDOENC_VUI_PARAM,              ///< [r/w], use structure VG_H26XENC_VUI
	VDOENC_DEBLOCK_PARAM,          ///< [r/w], use structure VG_H26XENC_DEBLOCK
	VDOENC_RATE_CONTROL,           ///< [r/w], use structure VG_H26XENC_RATE_CONTROL
	VDOENC_USR_QP_CONFIG,          ///< [r/w], use structure VG_H26XENC_USR_QP
	VDOENC_SLICE_SPLIT_CONFIG,     ///< [r/w], use structure VG_H26XENC_SLICE_SPLIT
	VDOENC_MASK_WIN_CONFIG,        ///< [r/w], use structure VG_H26XENC_MASK_WIN
	VDOENC_MASK_INIT_CONFIG,       ///< [r/w], use structure VG_H26XENC_MASK_INIT
	VDOENC_ENC_GDR_CONFIG,         ///< [r/w], use structure VG_H26XENC_GDR
	VDOENC_ROI_CONFIG,             ///< [r/w], use structure VG_H26XENC_ROI
	VDOENC_ROW_RC_CONFIG,          ///< [r/w], use structure VG_H26XENC_ROW_RC
	VDOENC_AQ_CONFIG,              ///< [r/w], use structure VG_H26XENC_AQ
	VDOENC_LPM_CONFIG,             ///< [r/w], use structure VG_H26XENC_LPM
	VDOENC_RND_CONFIG,             ///< [r/w], use structure VG_H26XENC_RND
	VDOENC_OSG_WIN_CONFIG,         ///< [r/w], use structure VG_H26XENC_OSG_WIN
	VDOENC_OSG_INIT_CONFIG,        ///< [r/w], use structure VG_H26XENC_OSG_INIT
	VDOENC_MOTION_AQ_CONFIG,       ///< [r/w], use structure VG_H26XENC_MOTION_AQ
	VDOENC_TMNR_CONFIG,            ///< [r/w], use structure VG_H26XENC_TMNR
	MAX_VDOENC_PARAM_COUNT
} VG_VDOENC_PARAM_ID;

typedef struct {
	unsigned int ver;   // VIDEOENC_PARAM_H_VER value, driver should check it before using this structure

	union {
		unsigned int update_mask;
		struct {
			int vg_enc_buf_param        : 1;
			int vg_h26xenc_feature      : 1;
			int vg_h26xenc_vui          : 1;
			int vg_h26xenc_deblock      : 1;
			int vg_h26xenc_rate_control : 1;
			int vg_h26xenc_usr_qp       : 1;
			int vg_h26xenc_slice_split  : 1;
			int vg_h26xenc_mask_win     : 1;
			int vg_h26xenc_mask_init    : 1;
			int vg_h26xenc_gdr          : 1;
			int vg_h26xenc_roi          : 1;
			int vg_h26xenc_row_rc       : 1;
			int vg_h26xenc_aq           : 1;
			int vg_h26xenc_lpm          : 1;
			int vg_h26xenc_rnd          : 1;
			int vg_h26xenc_osg_win      : 1;
			int vg_h26xenc_osg_init     : 1;
			int vg_h26xenc_motion_aq    : 1;
			int vg_h26xenc_tmnr         : 1;
			int reserved                : 13;
		} item;
	} update_flag;

	VG_CODEC_TYPE codec_type;
	int low_latency_mode;

	VG_ENC_BUF_PARAM        enc_buf_param;
	VG_H26XENC_FEATURE      enc_feature;
	VG_H26XENC_VUI          vui;
	VG_H26XENC_DEBLOCK      deblock;
	VG_H26XENC_RATE_CONTROL rate_control;
	VG_H26XENC_USR_QP       usr_qp;
	VG_H26XENC_SLICE_SPLIT  slice_split;
	VG_H26XENC_GDR          gdr;
	VG_H26XENC_ROI          roi;
	VG_H26XENC_ROW_RC       row_rc;
	VG_H26XENC_AQ           aq;
	VG_H26XENC_LPM          lpm;
	VG_H26XENC_RND          rnd;
	VG_H26XENC_MOTION_AQ    motion_aq;

	unsigned int param_ver; //hdal uses it internally, encoder driver don't see this value

} h26x_param_set;


#endif
