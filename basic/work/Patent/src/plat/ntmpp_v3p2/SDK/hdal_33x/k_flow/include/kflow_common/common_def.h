#ifndef __VPCOMMON_H__
#define __VPCOMMON_H__


/**************************************************************************************************/
/* common definition */
/**************************************************************************************************/

typedef struct {
	unsigned int x;
	unsigned int y;
} pos_t;

typedef struct {
	unsigned int w;
	unsigned int h;
} dim_t, align_level_dim_t;
#define IS_ZERO_DIM(p_dim)     ((p_dim)->w == 0 || (p_dim)->h == 0)

typedef struct {
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
} rect_t, align_level_rect_t;
#define IS_ZERO_RECT(p_rect)    ((p_rect)->w == 0 || (p_rect)->h == 0)


typedef enum {
	VPD_BUFTYPE_UNKNOWN     = 0x00,
	VPD_BUFTYPE_YUV422      = 0x20,  // --> VPD_YUV422
	VPD_BUFTYPE_YUV422_SCE  = 0x21,  // --> VPD_YUV422_DPCM
	VPD_BUFTYPE_YUV422_MB   = 0x22,  // --> VPD_YUV422_MB
	VPD_BUFTYPE_YUV420_SP   = 0x10,  // --> VPD_YUV420
	VPD_BUFTYPE_YUV420_SCE  = 0x11,  // --> VPD_YUV420_DPCM
	VPD_BUFTYPE_YUV420_MB   = 0x12,  // --> VPD_YUV420_MB
	VPD_BUFTYPE_YUV420_16x2 = 0x13,  // new
	VPD_BUFTYPE_YUV420_SP8  = 0x14,  // new
	VPD_BUFTYPE_YUV420_LLC_8x4  = 0x15,  // --> VPD_YUV420_LLC_8x4
	VPD_BUFTYPE_YUV420_8x4  = 0x16,  // --> VPD_YUV420_8x4 (h26x true mode)
	VPD_BUFTYPE_ARGB1555    = 0x31,  // --> VPD_RGB1555
	VPD_BUFTYPE_ARGB8888    = 0x32,  // --> VPD_RGB8888
	VPD_BUFTYPE_RGB565      = 0x33,  // --> VPD_RGB565
	VPD_BUFTYPE_RGB_CV      = 0x34,  // new
	VPD_BUFTYPE_MD      	= 0x35,  // --> for motion detection
	VPD_BUFTYPE_DATA        = 0x90,  // --> VPD_BITSTREAM & any other type
	MAX_VPD_BUFTYPE_COUNT   = 0x99,  // --> max type counts
} vpd_buf_type_t;





typedef struct {
	int is_dpcm : 4;
	int is_mb   : 4;
} vpd_buf_type_ex_t;

typedef enum {
	VPD_VPE_BACKGROUND = 0,
	VPD_VPE_PIP_LAYER = 1
} vpd_vpe_feature_t;

typedef enum {
	VPD_DI_OFF = 0,
	VPD_DI_TEMPORAL = 1,
	VPD_DI_SPATIAL = 2,
	VPD_DI_TEMPORAL_SPATIAL = 3,
} vpd_di_mode_t;

typedef enum {
	VPD_DN_OFF = 0,
	VPD_DN_TEMPORAL = 1,
	VPD_DN_SPATIAL = 2,
	VPD_DN_TEMPORAL_SPATIAL = 3,
} vpd_dn_mode_t;

typedef enum {
	VPD_SHARPNESS_OFF = 0,
	VPD_SHARPNESS_ON = 1,
} vpd_sharpness_mode_t;

typedef enum {
	VPD_VRC_CBR,    ///< Constant Bitrate
	VPD_VRC_VBR,    ///< Variable Bitrate
	VPD_VRC_FIXQP,  ///< Fixed QP
	VPD_VRC_EVBR,   ///< Enhanced Variable Bitrate
} vpd_vrc_mode_t;  /* video codec rate control mode */

typedef enum {
	VPD_JPEG_CBR = 0,    ///< Constant Bitrate
	VPD_JPEG_VBR,        ///< Variable Bitrate
	VPD_JPEG_FIXQP,      ///< Fixed QP
} vpd_jpeg_rc_mode_t;  /* video codec rate control mode */

typedef enum {
	VPD_SLICE_TYPE_P_FRAME = 0,
	VPD_SLICE_TYPE_B_FRAME = 1,
	VPD_SLICE_TYPE_I_FRAME = 2,
	VPD_SLICE_TYPE_NP_FRAME = 3,
	VPD_SLICE_TYPE_KP_FRAME = 4,
	VPD_SLICE_TYPE_AUDIO_BS = 5,
	VPD_SLICE_TYPE_RAW = 6,
	VPD_SLICE_TYPE_SKIP = 10,
} vpd_slice_type_t;  /* video codec rate control mode */


typedef enum {
	VPD_CAP_METHOD_FRAME = 0,   // 'even/odd interlacing frame' or 'whole progressive frame'
	VPD_CAP_METHOD_2FRAME,      // 'half-even follows half-odd frame'
	VPD_CAP_METHOD_FIELD,
	VPD_CAP_METHOD_2FIELD
} vpd_cap_method_t;

typedef enum {
	VPD_FLIP_NONE = 0,
	VPD_FLIP_VERTICAL,
	VPD_FLIP_HORIZONTAL,
	VPD_FLIP_VERTICAL_AND_HORIZONTAL
} vpd_flip_t;

typedef enum {
	VPD_ROTATE_NONE = 0,
	VPD_ROTATE_LEFT,
	VPD_ROTATE_RIGHT,
	VPD_ROTATE_180,         //rotate 180 degree
} vpd_rotate_t;

typedef enum {
	VPD_AU_NONE = 0,
	VPD_AU_PCM,
	VPD_AU_ACC,
	VPD_AU_ADPCM,
	VPD_AU_G711_ALAW,
	VPD_AU_G711_ULAW,
	MAX_VPD_AU_TYPE_COUNT
} vpd_au_enc_type_t;


typedef enum {
	VPD_VUI_TV_RANGE = 0,
	VPD_VUI_FULL_RANGE
} vpd_vui_format_range_t;



typedef enum {
	VPD_ENC_TYPE_JPEG = 0,
	VPD_ENC_TYPE_H264,
	VPD_ENC_TYPE_H265,
} vpd_enc_type_t;

typedef enum {
	VPD_DEC_TYPE_JPEG = 0,
	VPD_DEC_TYPE_H264,
	VPD_DEC_TYPE_H265,
} vpd_dec_type_t;


#define MAX_POOL_NAME_LEN     31
typedef struct {
	int ddr_id;
	uintptr_t addr_pa;
	uintptr_t addr_va;
	int size;
	uintptr_t addr_pa2;
	uintptr_t addr_va2;
	int size2;
    char pool_name[MAX_POOL_NAME_LEN + 1];
} usr_buffer_info_t;


typedef struct {
	vpd_buf_type_t type;
	dim_t dim;
	dim_t bg_dim;
	unsigned int poc_info;
	unsigned int src_fmt;
	unsigned long long timestamp;
	unsigned int sub_yuv_ratio; // sub yuv ratio by setting user
	unsigned int pathid;
	unsigned int h26x_out_mode; // h26x output mode (fmt_1st | fmt_2nd | fmt_3rd | fmt_4th)
	unsigned int ts_user_flag;  // timestamp user flag
} usr_video_frame_info_t;

typedef struct {
	unsigned int slice_type;
	unsigned int qp;
	unsigned int evbr_state;
	unsigned int frame_type;

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

	unsigned int motion_ratio;

} usr_enc_info;

//Operate 'bit array' by these macros, the bit array MUST BE defined as 'unsigned int'
#define BIT_ARRAY_SET(array, index) \
	do { \
		if (((unsigned int)index) > MAX_VPD_BUFTYPE_COUNT) \
			break; \
		array[((index)/32)] |= (1 << ((index)%32));  \
	} while(0)

#define BIT_ARRAY_CLEAR(array, index)   \
	do { \
		if (((unsigned int)index) > MAX_VPD_BUFTYPE_COUNT) \
			break; \
		array[((index)/32)] &= ~(1 << ((index)%32)); \
	} while(0)

#define BIT_ARRAY_IS_SET(array, index) \
	( (((unsigned int)index) < MAX_VPD_BUFTYPE_COUNT) && array[((index)/32)] & (1 << ((index)%32)) )

#define BUF_TYPE_SET_SUPPORT      BIT_ARRAY_SET
#define BUF_TYPE_SET_NOT_SUPPORT  BIT_ARRAY_CLEAR
#define BUF_TYPE_IS_SUPPORT       BIT_ARRAY_IS_SET



#endif
