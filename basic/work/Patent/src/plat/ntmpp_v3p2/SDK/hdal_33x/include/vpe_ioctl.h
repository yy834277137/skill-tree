#ifndef _VPE_PARAM_H_
#define _VPE_PARAM_H_

#if defined(WIN32) || !defined(__KERNEL__)
    #include <stdint.h>
#else
    #include <linux/types.h>
#endif
#include "kwrap/ioctl.h"

//--------------------------------------------------------------------------------------
// Global define
//--------------------------------------------------------------------------------------
#define E_OK                  0  ///< Normal completion
#define E_INVALID_CH        (-1) ///< Unknow camera channel
#define E_CH_NOT_RDY        (-2) ///< Camera not ready
#define E_HW_LIMIT          (-3) ///< Exceed HW limitation
#define E_TIMEOUT           (-4) ///< Command timeout
#define E_ERROR            (-99) ///< Other errors

//--------------------------------------------------------------------------------------
// Global control interface
//--------------------------------------------------------------------------------------
typedef struct fd_en_info{
	int32_t fd;
	int32_t enable;
}fd_en_info_t;

//--------------------------------------------------------------------------------------
// PIPE Control interface
//--------------------------------------------------------------------------------------
typedef struct fd_pipe_mode{
	int32_t fd;
	int8_t mode; //Pipe mode. 0: MRNR->TMNR->SHP 1: MRNR->SHP->TMNR 2: SHP->MRNR->TMNR
	              //		   3: SHP->TMNR->MRNR 4: TMNR->MRNR->SHP 5: TMNR->SHP->MRNR
}fd_pipe_mode_t;

//--------------------------------------------------------------------------------------
// SHP interface
//--------------------------------------------------------------------------------------
typedef struct sharpen_param {
    uint8_t  edge_weight_src_sel; // Select source of edge weight calculation, 0~1
    uint8_t  edge_weight_th; // Edge weight coring threshold, 0~255
    uint8_t  edge_weight_gain; // Edge weight gain, 0~255
    uint8_t  noise_level; // Noise Level, 0~255
    uint8_t  noise_curve[17]; // 17 control points of noise modulation curve, 0~255
    uint8_t  blend_inv_gamma; // Blending ratio of HPF results, 0~128
    uint8_t  edge_sharp_str1; // Sharpen strength1 of edge region, 0~255
    uint8_t  edge_sharp_str2; // Sharpen strength2 of edge region, 0~255
    uint8_t  flat_sharp_str; // Sharpen strength of flat region,0~255
    uint8_t  coring_th; // Coring threshold, 0~255
    uint8_t bright_halo_clip; // Bright halo clip ratio, 0~255
    uint8_t dark_halo_clip; // Dark halo clip ratio, 0~255
} sharpen_param_t;

typedef struct fd_sharpen_param{
	int32_t fd;
	sharpen_param_t param;
}fd_sharpen_param_t;

typedef struct fd_sharpen_out{
	int32_t fd;
	int32_t out_sel;  //Sharpen output select. Range 0~1
}fd_sharpen_out_t;

//--------------------------------------------------------------------------------------
// Edge Smooth interface
//--------------------------------------------------------------------------------------
typedef struct edgesmooth_param {
	uint8_t edge_smooth_en; 			// Edge smooth enable, ON/OFF
	uint8_t edge_smooth_y_edeng_th_lo;	// luma layer edge energy low threshold, 0~255
	uint8_t edge_smooth_y_edeng_th_hi;	// luma layer edge energy high threshold, 0~255
	uint8_t edge_smooth_y_ew_lo;		// luma layer edge weighting curve low value, 0~32
	uint8_t edge_smooth_y_ew_hi;		// luma layer edge weighting curve high value, 0~32
	uint8_t edge_smooth_y_edi_th;		// luma layer edge mask threshold, 0~63
	uint8_t edge_smooth_y_ds_str;       // luma layer edge directed smoothing filter strength, 0 ~ 7
} edgesmooth_param_t;

typedef struct fd_edgesmooth_param{
	int32_t fd;
	edgesmooth_param_t param;
}fd_edgesmooth_param_t;

//--------------------------------------------------------------------------------------
// MRNR interface
//--------------------------------------------------------------------------------------
typedef struct mrnr_param {
    uint32_t  t_y_edge_detection[2][8]; // Edge pixel detection threshold for 2-layer, 0~1023
    uint32_t  t_cb_edge_detection; // Edge pixel detection threshold of Cb{layer2}, 0~1023
    uint32_t  t_cr_edge_detection; // Edge pixel detection threshold of Cr{layer2}, 0~1023
    uint8_t  t_y_edge_smoothing[2][8]; // Edge pixel smoothing threshold of Y: {layer1(8 intensity levels)~layer2(8 intensity levels)}, 0~255
    uint8_t  t_cb_edge_smoothing; // Edge pixel smoothing threshold of Cb: {layer2}, 0~255
    uint8_t  t_cr_edge_smoothing; // Edge pixel smoothing threshold of Cr: {layer2}, 0~255
    uint8_t   nr_strength_y[2]; // Strength of noise reduction of Y: {layer1~layer2}, 0~15
    uint8_t   nr_strength_c; // Strength of noise reduction of CbCr: {layer2}, 0~15
} mrnr_param_t;

typedef struct fd_mrnr_param{
	int32_t fd;
	mrnr_param_t param;
}fd_mrnr_param_t;

//--------------------------------------------------------------------------------------
// TMNR interface
//--------------------------------------------------------------------------------------
typedef struct tmnr_param {
    uint8_t   luma_dn_en;  //Y channel 3DNR ON/OFF
    uint8_t   chroma_dn_en;  //CbCr channel 3DNR ON/OFF
    uint8_t   tmnr_fcs_en;  // TMNR False Color Supression enable, **chroma_dn_en shoulde be 1
    uint8_t   nr_str_y_3d; //Y channel temporal NR strength, 0~32
    uint8_t   nr_str_y_2d; //Y channel spatial NR strength, 0~32
    uint8_t   nr_str_c_3d; //CbCr channel temporal NR strength, 0~32
    uint8_t   nr_str_c_2d; //CbCr channel spatial NR strength, 0~32
    uint8_t   blur_str_y; //Difference Y image blurred strength (0: No blur, 1: low-strength blur, 2: high-strength blur)
    uint8_t   center_wzero_y_2d_en; //Apply zero to center weight of Y spatial Bilateral filter
    uint8_t   center_wzero_y_3d_en; //Apply zero to center weight of Y temporal Bilateral filter
    uint8_t   small_vibrat_supp_y_en; //Y channel small vibration suppression ON/OF
    uint8_t   avoid_residue_th_y; // 1~4
    uint8_t   avoid_residue_th_c; // 1~4
    uint32_t  y_base[8]; //Y channel noise model parameter: base noise level, 0 ~16320
    uint32_t  motion_level_th_y_k1; //Y channel match level threshold 1 adjustment, 0~32
    uint32_t  motion_level_th_y_k2; //Y channel match level threshold 2 adjustment, 0~32 AND K2 > K1
    uint32_t  motion_level_th_c_k1; // CbCr channel match level threshold 1 adjustment, 0~32
    uint32_t  motion_level_th_c_k2; // CbCr channel match level threshold 2 adjustment, 0~32 AND K2 > K1
    uint32_t  y_coefa[8]; //Y channel noise model parameter: slope of line, 0~48, Regard 16 as slope 1.0
    uint32_t  y_coefb[8]; //Y channel noise model parameter: intercept of line, 0 ~16320
    uint32_t  y_std[8]; // Y channel noise model parameter: noise standard deviation, 0 ~16320
    uint32_t  cb_mean[8]; // Cb channel noise model parameter: noise mean, 0 ~6375
    uint32_t  cb_std[8]; // Cb channel nosie model parameter: noise standard deviation, 0 ~6375
    uint32_t  cr_mean[8]; // Cr channel noise model parameter: noise mean, 0 ~6375
    uint32_t  cr_std[8]; // Cr channel nosie model parameter: noise standard deviation, 0 ~6375
    uint8_t   lut_y_3d_1_th[4]; // LUT of Y channel 3d_1 filter, 0~127
    uint8_t   lut_y_3d_2_th[4]; // LUT of CbCr channel 3d filter, 0~127
    uint8_t   lut_y_2d_th[4]; // LUT of Y channel 2d filter, 0~127
    uint8_t   lut_c_3d_th[4]; // LUT of CbCr channel 3d filter, 0~127
    uint8_t   lut_c_2d_th[4]; // LUT of CbCr channel 2d filter, 0~127
    uint8_t   tmnr_fcs_str;   // TMNR_False Color Supression strength
    uint8_t   tmnr_fcs_th;         // TMNR False Color Supression threshold

	//new in nt98321
	uint8_t  dithering_en;   // source dithering switch. 0~1. default  0
	uint8_t  dithering_bit_y;// Y-channel dithering range. 0~3. default 2
	uint8_t  dithering_bit_u;// U-channel dithering range. 0~3. default 1
	uint8_t  dithering_bit_v;// V-channel dithering range. 0~3. default 1

	uint8_t err_compensate;// err compensation method option. 0~1. default 1

} tmnr_param_t;

typedef struct fd_tmnr_param{
	int32_t fd;
	tmnr_param_t param;
}fd_tmnr_param_t;

typedef struct fd_tmnr_map_idx{
	int32_t fd;
	int32_t map_en;          //enable/disable motion map output
    int32_t map_idx;         //Select motion map output channel. 0: Y, 1: Cb, 2: Cr
}fd_tmnr_map_idx_t;

typedef struct fd_tmnr_sta_info{
	int32_t fd;
    int32_t blk_no;          //TMNR statistic data block count
}fd_tmnr_sta_info_t;

typedef struct fd_tmnr_sta_data{
	int32_t fd;
    int32_t timeout_int;     //Timeout interval (msec)
	unsigned long *p_stabuf;       //TMNR statistic data buffer
}fd_tmnr_sta_data_t;

typedef struct fd_tmnr_buf_info{
	int32_t fd;
	uint32_t ddr_id;
	unsigned long pbuf_addr;   //TMNR physical address read & write
	uint32_t pbuf_size;   //TMNR mtn buffer (r/w per fd)
}fd_tmnr_buf_info_t;

//--------------------------------------------------------------------------------------
// Scale interface
//--------------------------------------------------------------------------------------
typedef struct sca_param {
    int8_t sca_ceffH[4]; //LPF coefficient in horizontal direction
    int8_t sca_ceffV[4]; //LPF coefficient in vertical direction
}sca_param_t;

typedef struct sca_info_set{
	int32_t fd;
    uint8_t sca_y_luma_algo_en; //Algorithm select for vertical luma scaler
    uint8_t sca_x_luma_algo_en; //Algorithm select for horizontal luma scaler
    uint8_t sca_y_chroma_algo_en; //Algorithm select for vertical chroma scaler
    uint8_t sca_x_chroma_algo_en; //Algorithm select for horizontal chroma scaler
    uint8_t sca_map_sel; //Scaler source mapping format select
    sca_param_t sca_1000x_param;  //scaling parameter for scaling ratio 1.00x idx0
    sca_param_t sca_1250x_param;  //scaling parameter for scaling ratio 1.25x idx1
    sca_param_t sca_1500x_param;  //scaling parameter for scaling ratio 1.50x idx2
    sca_param_t sca_1750x_param;  //scaling parameter for scaling ratio 1.75x idx3
    sca_param_t sca_2000x_param;  //scaling parameter for scaling ratio 2.00x idx4
    sca_param_t sca_2250x_param;  //scaling parameter for scaling ratio 2.25x idx5
    sca_param_t sca_2500x_param;  //scaling parameter for scaling ratio 2.50x idx6
    sca_param_t sca_2750x_param;  //scaling parameter for scaling ratio 2.75x idx7
    sca_param_t sca_3000x_param;  //scaling parameter for scaling ratio 3.00x idx8
    sca_param_t sca_3250x_param;  //scaling parameter for scaling ratio 3.25x idx9
    sca_param_t sca_3500x_param;  //scaling parameter for scaling ratio 3.50x idx10
    sca_param_t sca_3750x_param;  //scaling parameter for scaling ratio 3.75x idx11
    sca_param_t sca_4000x_param;  //scaling parameter for scaling ratio 4.00x idx12
    sca_param_t sca_5000x_param;  //scaling parameter for scaling ratio 5.00x idx13
    sca_param_t sca_6000x_param;  //scaling parameter for scaling ratio 6.00x idx14
    sca_param_t sca_7000x_param;  //scaling parameter for scaling ratio 7.00x idx15
    sca_param_t sca_8000x_param;  //scaling parameter for scaling ratio 8.00x idx16
}sca_info_set_t;

typedef struct fd_sca_buf_info{
	int32_t fd;
	uint32_t ddr_id;
	unsigned long pbuf_addr;   //sca working buffer for scaling up/down over 8x
	uint32_t pbuf_size;
}fd_sca_buf_info_t;


typedef enum {
    VPE_RANGE_BYPASS = 0, // 1 is the same as 0
    VPE_RANGE_PC2TV = 2, // Y:16~235, C:16~240
    VPE_RANGE_TV2PC = 3,
    VPE_RANGE_DISABLE = 4, 	//disable ioctl range selection
    VPE_RANGE_MAXTYPE = 5
} sca_range_t;

typedef struct fd_sca_yuv_range{
	int32_t fd;
	sca_range_t yuv_range;
}fd_sca_yuv_range_t;

//--------------------------------------------------------------------------------------
// palette interface
//--------------------------------------------------------------------------------------
typedef struct palette_config{
	uint8_t idx;
	uint8_t pal_config_y;
	uint8_t pal_config_cb;
	uint8_t pal_config_cr;
}palette_config_t;

//--------------------------------------------------------------------------------------
// mask interface
//--------------------------------------------------------------------------------------
typedef struct mask_point {
	int32_t x; // point (x,y)
	int32_t y; //
}mask_point_t;

typedef enum {
    VPE_MASK_INSIDE = 0,
    VPE_MASK_OUTSIDE = 1,
    VPE_MASK_LINE = 2,
    VPE_MASK_MAXTYPE = 3
} mask_area_t;

typedef enum {
    VPE_MASK_BITMAP_NONE    = 0,
    VPE_MASK_BITMAP_HIT_PAL	= 1,
    VPE_MASK_BITMAP_HIT_MAP = 2,
    VPE_MASK_BITMAP_MAX = 3
} mask_bitmap_type_t;

typedef struct mask_win {
	int32_t fd; // input channel
	int32_t mask_idx;// index = priority 0>1>2>3>4>5>6>7
	int32_t enable; // 1: enable, 0:disable, and ignore other settings
	mask_area_t mask_area; //0:inside, 1:outside, 2:line
	mask_point_t point[4]; // 4 points
}mask_win_t;

typedef struct mask_param {
	int32_t fd; // input channel
	int32_t mask_idx;// index = priority 0>1>2>3>4>5>6>7
	int32_t mosaic_en; // use original image or mosaic image in mask area
	//mask_bitmap_type_t bitmap; // bitmap type
	int32_t alpha; // alpha blending 0~256, only effect at bitmap = 0,1
	int32_t pal_sel; // palette idx, only effect at bitmap = 0,1
}mask_param_t;

typedef struct mask_mosaic_block {
	int32_t fd; // input channel
	int32_t mosaic_blkw; // mosaic block w
	int32_t mosaic_blkh; // mosaic block h
}mask_mosaic_block_t;

//--------------------------------------------------------------------------------------
// pattern image
//--------------------------------------------------------------------------------------
#define MAX_VPE_PATTERN_COUNT 5
typedef struct pat_img_info {
    uint32_t index; //0 ~ (MAX_VPE_PATTERN_COUNT - 1): pattern index

    uint32_t type;       //422/420/...
    uint32_t ddr_id;
	uintptr_t buf_addr;  //mean image buffer physical address(DMA buffer)
	uint32_t buf_size;	//mean image buffer size
    uint16_t width;
    uint16_t height;
}pat_img_info_t;

#define VPE_PATTERN_CROP_ENABLE 0xefef0001
#define VPE_PATTERN_SEL_DISABLE 0xffffffff
typedef struct pat_img_sel_info {
	int32_t fd; // input channel
    uint32_t index; //0 ~ (MAX_VPE_PATTERN_COUNT - 1): pattern index, set VPE_PATTERN_SEL_DIABLE disable pattern selection

	//full image (x:0, y:0, w:100, h:100)
	uint8_t x; //x pos % relative dst image width (0 ~ 100)
	uint8_t y; //y pos % relative dst image height (0 ~ 100)
	uint8_t w; //pat image width % relative dst image width(1 ~ 100)
	uint8_t h; //pat image heihgt % relative dst image height(1 ~ 100)

	uint16_t bg_color_sel;

	uint32_t crop_en; //VPE_PATTERN_CROP_ENABLE:enable, others disable.
	uint16_t crop_x; //x pos at the specified pattern image
	uint16_t crop_y; //y pos at the specified pattern image
	uint16_t crop_w; //width at the specified pattern image
	uint16_t crop_h; //height at the specified pattern image
}pat_img_sel_info_t;

//--------------------------------------------------------------------------------------
// hw information
//--------------------------------------------------------------------------------------

typedef struct vpe_pty_src_info {
	uint16_t min_w;
	uint16_t min_h;

	uint16_t max_w;
	uint16_t max_h;

	uint8_t bg_w;
	uint8_t bg_h;

	uint8_t x;
	uint8_t y;
	uint8_t w;
	uint8_t h;
} vpe_pty_src_info_t;

typedef struct vpe_pty_dst_info {
	uint16_t min_w;
	uint16_t min_h;

	uint16_t max_w;
	uint16_t max_h;

	uint8_t bg_w;
	uint8_t bg_h;

	uint8_t hole_x;
	uint8_t hole_y;
	uint8_t hole_w;
	uint8_t hole_h;

	uint8_t x;
	uint8_t y;
	uint8_t w;
	uint8_t h;
} vpe_pty_dst_info_t;

#define VPE_FEATURE_NONE 	0x00000000
#define VPE_FEATURE_ROT		0x00000001	//enable rot 90/270 degree function
typedef struct vpe_hw_info {
	uint32_t feature;			//enable feature
	uint32_t src_fmt;			//image format TYPE_YUV420_SP/TYPE_YUV422/......
	uint32_t dst_fmt;			//image format TYPE_YUV420_SP/TYPE_YUV422/......

	vpe_pty_src_info_t src;			//src align information
	vpe_pty_dst_info_t dst;			//dst align information
}vpe_hw_info_t;

//--------------------------------------------------------------------------------------
// MD interface
//--------------------------------------------------------------------------------------
#define MD_LV_MAX 4
#define MD_COLUMN_MAX 32
#define MD_VPLANE_WIN_MAX 4

typedef struct md_win_size {
	int32_t fd;
	uint8_t width;
	uint8_t height;
} md_win_size_t;

typedef struct md_buf_addr {
	uint32_t ddr_id;
	void *v_addr;
	unsigned long p_addr;
	int size;
} md_buf_addr_t;

typedef struct md_buf_info {
	int32_t fd;
	uint32_t win_x_num_max;
	uint32_t win_y_num_max;
	md_buf_addr_t evt_buf;
	md_buf_addr_t evt_buf_ep;
	md_buf_addr_t sta_buf;
	md_buf_addr_t lv_buf;
	md_buf_addr_t lv_buf_ep;
} md_buf_info_t;

typedef enum {
	VPE_IOC_MD_STS_TAMPER = 0,
	VPE_IOC_MD_STS_SCENE_CHG,
} VPE_IOC_MD_STS;

typedef struct md_sts_info {
	int32_t fd;			///< in
	VPE_IOC_MD_STS id;	///< in
	uint32_t value;		///< out
} md_sts_info_t;

typedef struct md_region_info {
	uint16_t img_width;
	uint16_t img_height;
	md_win_size_t win_size;
	uint16_t win_width_num;
	uint8_t col_num; ///< col_num=0 => 1 column. col_num=1 => 2 column. etc...
	uint16_t win_x[MD_COLUMN_MAX];
	uint16_t win_y;
	uint8_t win_x_num[MD_COLUMN_MAX];
	uint16_t win_y_num;
	uint16_t win_x_start_num[MD_COLUMN_MAX];
} md_region_info_t;

typedef struct md_evt_info {
	int32_t fd;
	md_region_info_t region;
	md_buf_addr_t evt_buf;
	uint32_t timestamp;
} md_evt_info_t;

typedef enum {
	VPE_IOC_MD_TAMPER_TYPE_EDGE = 0,
	VPE_IOC_MD_TAMPER_TYPE_INTENSITY,
	VPE_IOC_MD_TAMPER_TYPE_MAX
} VPE_IOC_MD_TAMPER_TYPE;

typedef enum {
 	VPE_IOC_MD_PARAM_RST = 0,
	VPE_IOC_MD_PARAM_TYPE,						///< VPE536 MD always uses type 1 (md2_upd_on) update method
	VPE_IOC_MD_PARAM_TIME_PERIOD,
	VPE_IOC_MD_PARAM_DXDY,
	VPE_IOC_MD_PARAM_TBG,
	VPE_IOC_MD_PARAM_SCENE_CHANGE_THRES,
	VPE_IOC_MD_PARAM_TAMPER_TYPE,				///< Tamper operation type. Type: VPE_IOC_MD_TAMPER_TYPE
	VPE_IOC_MD_PARAM_TAMPER_EDGE_TEXT_THRES,
	VPE_IOC_MD_PARAM_TAMPER_EDGE_WIN_THRES,
	VPE_IOC_MD_PARAM_TAMPER_AVG_TEXT_THRES,
	VPE_IOC_MD_PARAM_TAMPER_AVG_WIN_THRES,
	VPE_IOC_MD_PARAM_LVL_ALPHA,
	VPE_IOC_MD_PARAM_LVL_ONE_MIN_ALPHA,
	VPE_IOC_MD_PARAM_LVL_INIT_WEIGHT,
	VPE_IOC_MD_PARAM_LVL_MODEL_UPDATE,
	VPE_IOC_MD_PARAM_LVL_TB,
	VPE_IOC_MD_PARAM_LVL_SIGMA,
	VPE_IOC_MD_PARAM_LVL_TG,
	VPE_IOC_MD_PARAM_LVL_PRUNE,
	VPE_IOC_MD_PARAM_LVL_LUMA_DIFF_THRES,
	VPE_IOC_MD_PARAM_LVL_TEXT_DIFF_THRES,
	VPE_IOC_MD_PARAM_LVL_TEXT_THRES,
	VPE_IOC_MD_PARAM_LVL_TEXT_RATIO_THRES,
	VPE_IOC_MD_PARAM_LVL_GM_MD2_THRES,
	VPE_IOC_MD_PARAM_MAX
} VPE_IOC_MD_PARAM;

typedef struct md_param_info {
	int32_t fd;				///< in
	uint32_t level;			///< in
	VPE_IOC_MD_PARAM id;	///< in
	uint32_t value;			///< in/out
} md_param_info_t;

typedef struct md_lv_vplane_info {
	int32_t fd;
	uint16_t width;
	uint16_t height;
	struct {
		int32_t valid; 			///< indicates whether this window's info needs to be updated. set 0 to skip this window setting
		uint16_t x;
		uint16_t y;
		uint16_t w;
		uint16_t h;
		uint8_t lv;
	} win[MD_VPLANE_WIN_MAX];
} md_lv_vplane_info_t;

//--------------------------------------------------------------------------------------
// DCE interface
//--------------------------------------------------------------------------------------
typedef struct dce_gdc_param {
    int32_t cent_x_s;  //Lens center of x axis (signed)
    int32_t cent_y_s;  //Lens center of y axis (signed)
    uint32_t lens_r;
    uint32_t xdist;  //X input distance factor, for oval shape modeling
    uint32_t ydist;  //Y input distance factor, for oval shape modeling
    uint8_t normfact;  //Radius normalization factor (u1.7)
    uint8_t normbit;  //Radius normalization shift bit
    uint16_t geo_lut[65]; //GDC look-up table
}dce_gdc_param_t;

typedef struct fd_dce_gdc_param {
	int32_t fd;
    dce_gdc_param_t param;
}fd_dce_gdc_param_t;

typedef struct dce_2dlut_param {
    uint8_t lut2d_sz;  //Size selection of 2D look-up table
    uint32_t hfact;  //Horizontal scaling factor for 2DLut scaling up (u0.24)
    uint32_t vfact;  //Vertical scaling factor for 2DLut scaling up (u0.24)
    uint8_t xofs_i;  //2DLUT x offset, integer part
    uint32_t xofs_f;  //2DLUT x offset, fraction part
    uint8_t yofs_i;  //2DLUT y offset, integer part
    uint32_t yofs_f;  //2DLUT xy offset, fraction part
}dce_2dlut_param_t;

typedef struct fd_dce_2dlut_param {
	int32_t fd;
    dce_2dlut_param_t param;
}fd_dce_2dlut_param_t;

typedef struct fd_dce_2dlut_tbl {
	int32_t fd;
    uint32_t lut_tbl[4420];   //2D LUT
}fd_dce_2dlut_tbl_t;

typedef struct dce_fov_param {
    uint8_t fovbound;  //FOV boundary process method selection
    uint16_t boundy;   //Bound value for Y component (u8.2)
    uint16_t boundu;   //Bound value for U component (u8.2)
    uint16_t boundv;   //Bound value for V component (u8.2)
    uint16_t fovgain;  //Scale down factor for FOV preservation (u2.10)
}dce_fov_param_t;

typedef struct fd_dce_fov_param {
	int32_t fd;
    dce_fov_param_t param;
}fd_dce_fov_param_t;

typedef struct fd_dce_mode {
    int32_t fd;
    int32_t dce_mode; // 0:gdc, 1:2d_lut
}fd_dce_mode_t;

//--------------------------------------------------------------------------------------
// DCTG interface
//--------------------------------------------------------------------------------------
typedef struct dctg_lens_param {
    uint8_t mount_type;  //Camera mount type. 0: Ceiling mount, 1: Floor mount
    uint8_t lut2d_sz;  //Size selection of 2D look-up table, should the same with DCE setting.
    uint32_t lens_r;  //Radius of Lens
    uint32_t lens_x_st;  //Lens start x position at a source image
    uint32_t lens_y_st;  //Lens start y position at a source image
}dctg_lens_param_t;

typedef struct dctg_fov_param {
    int32_t theta_st;  //FOV theta start radian (s4.16) Range: -*pi ~ *pi
    int32_t theta_ed;  //FOV theta end radian (s4.16) Range: -*pi ~ *pi
    int32_t phi_st;  //FOV phi start radian (s4.16) Range: -2*pi ~ 2*pi
    int32_t phi_ed;  //FOV phi end radian (s4.16) Range: -2*pi ~ 2*pi
    int32_t rot_z;  //Z-axis rotate radian (s4.16) Range: -2*pi ~ 2*pi
    int32_t rot_y;  //Y-axis rotate radian (s4.16) Range: -2*pi ~ 2*pi
}dctg_fov_param_t;

typedef struct fd_dctg_ra_param {
    int32_t fd;
    uint8_t ra_en;  //Enable mapping radius adjuster (RA)
    uint32_t ra_tab[257]; //radius mapping table
}fd_dctg_ra_param_t;

typedef struct fd_dctg_lens_param {
	int32_t fd;
    dctg_lens_param_t param;
}fd_dctg_lens_param_t;

typedef struct fd_dctg_fov_param {
	int32_t fd;
    dctg_fov_param_t param;
}fd_dctg_fov_param_t;

typedef enum {
    DCTG_MODE_DEWARP_90 = 0, //
    DCTG_MODE_DEWARP_360 = 1, //
    DCTG_MODE_DEWARP_MAXTYPE = 2
} dctg_mode_t;

typedef struct fd_dctg_mode {
    int32_t fd;
    dctg_mode_t dctg_mode;
}fd_dctg_mode_t;

//--------------------------------------------------------------------------------------
// module initial param
//--------------------------------------------------------------------------------------

typedef struct init_info {
	uint32_t st_size;	//structure size
	uint32_t chip_num;
	uint32_t eng_num;
	uint32_t minor_num;
	uint32_t md_lv_buf_num;
	uint32_t act_ch_num;
} init_info_t;

#define VPE_IOC_MAGIC       'v'
#define VPE_IOC_GET_PIPE_MODE                  _VOS_IOWR(VPE_IOC_MAGIC,  100, fd_pipe_mode_t*)
#define VPE_IOC_SET_PIPE_MODE                  _VOS_IOW(VPE_IOC_MAGIC,  100, fd_pipe_mode_t*)
#define VPE_IOC_GET_MRNR_ENABLE                _VOS_IOWR(VPE_IOC_MAGIC,  111, fd_en_info_t*)
#define VPE_IOC_SET_MRNR_ENABLE                _VOS_IOW(VPE_IOC_MAGIC,  111, fd_en_info_t*)
#define VPE_IOC_GET_MRNR_INFO                  _VOS_IOWR(VPE_IOC_MAGIC,  112, fd_mrnr_param_t*)
#define VPE_IOC_SET_MRNR_INFO                  _VOS_IOW(VPE_IOC_MAGIC,  112, fd_mrnr_param_t*)
#define VPE_IOC_GET_TMNR_ENABLE                _VOS_IOWR(VPE_IOC_MAGIC,  121, fd_en_info_t*)
#define VPE_IOC_SET_TMNR_ENABLE                _VOS_IOW(VPE_IOC_MAGIC,  121, fd_en_info_t*)
#define VPE_IOC_GET_TMNR_INFO                  _VOS_IOWR(VPE_IOC_MAGIC,  122, fd_tmnr_param_t*)
#define VPE_IOC_SET_TMNR_INFO                  _VOS_IOW(VPE_IOC_MAGIC,  122, fd_tmnr_param_t*)
#define VPE_IOC_GET_TMNR_MAP_INDEX             _VOS_IOWR(VPE_IOC_MAGIC,  123, fd_tmnr_map_idx_t*)
#define VPE_IOC_SET_TMNR_MAP_INDEX             _VOS_IOW(VPE_IOC_MAGIC,  123, fd_tmnr_map_idx_t*)
#define VPE_IOC_GET_TMNR_STA_INFO              _VOS_IOWR(VPE_IOC_MAGIC,  124, fd_tmnr_sta_info_t*)
#define VPE_IOC_GET_TMNR_STA_DATA              _VOS_IOWR(VPE_IOC_MAGIC,  125, fd_tmnr_sta_data_t*)
#define VPE_IOC_GET_TMNR_MTN_BUF               _VOS_IOWR(VPE_IOC_MAGIC,  126, fd_tmnr_buf_info_t*)
#define VPE_IOC_SET_TMNR_MTN_BUF               _VOS_IOW(VPE_IOC_MAGIC,  126, fd_tmnr_buf_info_t*)
#define VPE_IOC_GET_SHARPEN_ENABLE             _VOS_IOWR(VPE_IOC_MAGIC,  130, fd_en_info_t*)
#define VPE_IOC_SET_SHARPEN_ENABLE             _VOS_IOW(VPE_IOC_MAGIC,  130, fd_en_info_t*)
#define VPE_IOC_GET_SHARPEN_INFO               _VOS_IOWR(VPE_IOC_MAGIC,  131, fd_sharpen_param_t*)
#define VPE_IOC_SET_SHARPEN_INFO               _VOS_IOW(VPE_IOC_MAGIC,  131, fd_sharpen_param_t*)
#define VPE_IOC_GET_SHARPEN_TUNE_OUT           _VOS_IOWR(VPE_IOC_MAGIC,  132, fd_en_info_t*)
#define VPE_IOC_SET_SHARPEN_TUNE_OUT           _VOS_IOW(VPE_IOC_MAGIC,  132, fd_en_info_t*)
#define VPE_IOC_GET_SCA_INFO                   _VOS_IOWR(VPE_IOC_MAGIC,  140, sca_info_set_t*)
#define VPE_IOC_SET_SCA_INFO                   _VOS_IOW(VPE_IOC_MAGIC,  140, sca_info_set_t*)
#define VPE_IOC_GET_EDGE_SMOOTH_INFO           _VOS_IOWR(VPE_IOC_MAGIC,  141, fd_edgesmooth_param_t*)
#define VPE_IOC_SET_EDGE_SMOOTH_INFO           _VOS_IOW(VPE_IOC_MAGIC,  141, fd_edgesmooth_param_t*)

#define VPE_IOC_GET_DCE_ENABLE                 _VOS_IOWR(VPE_IOC_MAGIC,  150, fd_en_info_t*)
#define VPE_IOC_SET_DCE_ENABLE                 _VOS_IOW(VPE_IOC_MAGIC,  150, fd_en_info_t*)
#define VPE_IOC_GET_DCE_MODE                   _VOS_IOWR(VPE_IOC_MAGIC,  151, fd_dce_mode_t*)
#define VPE_IOC_SET_DCE_MODE                   _VOS_IOW(VPE_IOC_MAGIC,  151, fd_dce_mode_t*)
#define VPE_IOC_GET_DCE_GDC_INFO               _VOS_IOWR(VPE_IOC_MAGIC,  152, fd_dce_gdc_param_t*)
#define VPE_IOC_SET_DCE_GDC_INFO               _VOS_IOW(VPE_IOC_MAGIC,  152, fd_dce_gdc_param_t*)
#define VPE_IOC_GET_DCE_2DLUT_INFO             _VOS_IOWR(VPE_IOC_MAGIC,  153, fd_dce_2dlut_param_t*)
#define VPE_IOC_SET_DCE_2DLUT_INFO             _VOS_IOW(VPE_IOC_MAGIC,  153, fd_dce_2dlut_param_t*)
#define VPE_IOC_GET_DCE_2DLUT_TBL              _VOS_IOWR(VPE_IOC_MAGIC,  154, fd_dce_2dlut_tbl_t*)
#define VPE_IOC_SET_DCE_2DLUT_TBL              _VOS_IOW(VPE_IOC_MAGIC,  154, fd_dce_2dlut_tbl_t*)
#define VPE_IOC_GET_DCE_FOV_INFO               _VOS_IOWR(VPE_IOC_MAGIC,  155, fd_dce_fov_param_t*)
#define VPE_IOC_SET_DCE_FOV_INFO               _VOS_IOW(VPE_IOC_MAGIC,  155, fd_dce_fov_param_t*)

#define VPE_IOC_GET_DCTG_ENABLE                _VOS_IOWR(VPE_IOC_MAGIC,  160, fd_en_info_t*)
#define VPE_IOC_SET_DCTG_ENABLE                _VOS_IOW(VPE_IOC_MAGIC,  160, fd_en_info_t*)
#define VPE_IOC_GET_DCTG_LENS_INFO             _VOS_IOWR(VPE_IOC_MAGIC,  161, fd_dctg_lens_param_t*)
#define VPE_IOC_SET_DCTG_LENS_INFO             _VOS_IOW(VPE_IOC_MAGIC,  161, fd_dctg_lens_param_t*)
#define VPE_IOC_GET_DCTG_FOV_INFO              _VOS_IOWR(VPE_IOC_MAGIC,  162, fd_dctg_fov_param_t*)
#define VPE_IOC_SET_DCTG_FOV_INFO              _VOS_IOW(VPE_IOC_MAGIC,  162, fd_dctg_fov_param_t*)
#define VPE_IOC_GET_DCTG_RA_INFO               _VOS_IOWR(VPE_IOC_MAGIC,  163, fd_dctg_ra_param_t*)
#define VPE_IOC_SET_DCTG_RA_INFO               _VOS_IOW(VPE_IOC_MAGIC,  163, fd_dctg_ra_param_t*)
#define VPE_IOC_GET_DCTG_DEWARP_MODE           _VOS_IOWR(VPE_IOC_MAGIC,  164, fd_dctg_mode_t*)
#define VPE_IOC_SET_DCTG_DEWARP_MODE           _VOS_IOW(VPE_IOC_MAGIC,  164, fd_dctg_mode_t*)

#define VPE_IOC_GET_SCA_WK_BUF                 _VOS_IOWR(VPE_IOC_MAGIC,  180, fd_sca_buf_info_t*)
#define VPE_IOC_SET_SCA_WK_BUF                 _VOS_IOW(VPE_IOC_MAGIC,  180, fd_sca_buf_info_t*)
#define VPE_IOC_GET_SCA_YUV_RANGE              _VOS_IOWR(VPE_IOC_MAGIC,  181, fd_sca_yuv_range_t*)
#define VPE_IOC_SET_SCA_YUV_RANGE              _VOS_IOW(VPE_IOC_MAGIC,  181, fd_sca_yuv_range_t*)

#define VPE_IOC_GET_PALETTE_INFO               _VOS_IOWR(VPE_IOC_MAGIC,  200, palette_config_t*)
#define VPE_IOC_SET_PALETTE_INFO               _VOS_IOW(VPE_IOC_MAGIC,  200, palette_config_t*)
#define VPE_IOC_GET_MASK_WIN_INFO     		   _VOS_IOWR(VPE_IOC_MAGIC,  201, mask_win_t*)
#define VPE_IOC_SET_MASK_WIN_INFO              _VOS_IOW(VPE_IOC_MAGIC,  201, mask_win_t*)
#define VPE_IOC_GET_MASK_PARAM_INFO    		   _VOS_IOWR(VPE_IOC_MAGIC,  202, mask_param_t*)
#define VPE_IOC_SET_MASK_PARAM_INFO            _VOS_IOW(VPE_IOC_MAGIC,  202, mask_param_t*)
#define VPE_IOC_GET_MASK_MOSAIC_SIZE   		   _VOS_IOWR(VPE_IOC_MAGIC,  204, mask_mosaic_block_t*)
#define VPE_IOC_SET_MASK_MOSAIC_SIZE           _VOS_IOW(VPE_IOC_MAGIC,  204, mask_mosaic_block_t*)

#define VPE_IOC_GET_PATTERN_IMAGE              _VOS_IOWR(VPE_IOC_MAGIC,  207, pat_img_info_t*)
#define VPE_IOC_SET_PATTERN_IMAGE              _VOS_IOW(VPE_IOC_MAGIC,  207, pat_img_info_t*)
#define VPE_IOC_GET_PATTERN_SEL                _VOS_IOWR(VPE_IOC_MAGIC,  208, pat_img_sel_info_t*)
#define VPE_IOC_SET_PATTERN_SEL                _VOS_IOW(VPE_IOC_MAGIC,  208, pat_img_sel_info_t*)

#define VPE_IOC_GET_MD_ENABLE                  _VOS_IOWR(VPE_IOC_MAGIC,  210, fd_en_info_t*)
#define VPE_IOC_SET_MD_ENABLE                  _VOS_IOW(VPE_IOC_MAGIC,  210, fd_en_info_t*)
#define VPE_IOC_SET_MD_WIN_SIZE                _VOS_IOW(VPE_IOC_MAGIC,  211, md_win_size_t*)
#define VPE_IOC_SET_MD_BUF_INFO                _VOS_IOW(VPE_IOC_MAGIC,  212, md_buf_info_t*)
#define VPE_IOC_GET_MD_EVT_INFO                _VOS_IOWR(VPE_IOC_MAGIC,  213, md_evt_info_t*)
#define VPE_IOC_GET_MD_STS_INFO                _VOS_IOWR(VPE_IOC_MAGIC,  214, md_sts_info_t*)
#define VPE_IOC_GET_MD_PARAM                   _VOS_IOWR(VPE_IOC_MAGIC,  215, md_param_info_t*)
#define VPE_IOC_SET_MD_PARAM                   _VOS_IOW(VPE_IOC_MAGIC,  215, md_param_info_t*)
#define VPE_IOC_GET_MD_LV_VPLANE_INFO          _VOS_IOWR(VPE_IOC_MAGIC,  216, md_lv_vplane_info_t*)
#define VPE_IOC_SET_MD_LV_VPLANE_INFO          _VOS_IOW(VPE_IOC_MAGIC,  216, md_lv_vplane_info_t*)

#define VPE_IOC_SET_INIT_PARAM                 _VOS_IOW(VPE_IOC_MAGIC,  217, init_info_t*)
#define VPE_IOC_SET_UNINIT                     _VOS_IO(VPE_IOC_MAGIC,  218)

#define VPE_IOC_GET_HW_SPEC_INFO			   _VOS_IOWR(VPE_IOC_MAGIC,  219, vpe_hw_info_t)

#define VPE_IOC_SET_MRNR_REL_FD                _VOS_IOW(VPE_IOC_MAGIC,  220, int32_t*)
#define VPE_IOC_SET_TMNR_REL_FD                _VOS_IOW(VPE_IOC_MAGIC,  221, int32_t*)
#define VPE_IOC_SET_SHARPEN_REL_FD             _VOS_IOW(VPE_IOC_MAGIC,  222, int32_t*)
#define VPE_IOC_SET_DCE_REL_FD                 _VOS_IOW(VPE_IOC_MAGIC,  223, int32_t*)
#define VPE_IOC_SET_DCTG_REL_FD                _VOS_IOW(VPE_IOC_MAGIC,  224, int32_t*)
#define VPE_IOC_SET_MASK_REL_FD                _VOS_IOW(VPE_IOC_MAGIC,  225, int32_t*)
#define VPE_IOC_SET_MD_REL_FD                  _VOS_IOW(VPE_IOC_MAGIC,  226, int32_t*)

extern int vpe_module_get(unsigned int cmd, unsigned long arg);
extern int vpe_module_set(unsigned int cmd, unsigned long arg);

extern int vpe_module_init(struct init_info *info);
extern int vpe_module_uninit(void);
#endif
