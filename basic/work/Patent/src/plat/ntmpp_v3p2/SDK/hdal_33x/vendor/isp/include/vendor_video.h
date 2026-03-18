/**
	@brief Header file of vendor video.
	This file contains the functions which is related to image quality in the chip.

	@file vendor_video.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef	_VENDOR_VIDEO_H_
#define	_VENDOR_VIDEO_H_

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "hd_util.h"

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#define VENDER_VIDEO_MAX_CH_NUM		256
#define VENDER_VIDEO_VERSION		0x030005
/********************************************************************
	MACRO FUNCTION DEFINITIONS
********************************************************************/
#define VENDOR_VIDEOPROC_EXTEND_BASE	      0x2500  ///< for videoproc extend HD_DAL_VIDEOPROC_BASE (0x2400, 0x2500) max count 512
#define VENDOR_VIDEOPROC_CHIP(chip, did)	  (HD_DAL_VIDEOPROC((HD_DAL_VIDEOPROC_CHIP_DEV_COUNT * (chip)) + (did)))
#define VENDOR_VIDEOPROC_CHIP_CTRL(chip, dev_id)  ((VENDOR_VIDEOPROC_CHIP(chip, dev_id) << 16) | HD_CTRL)
#define VENDOR_VIDEOPROC_CHIP_IN(chip, dev_id, in_id)	((VENDOR_VIDEOPROC_CHIP(chip, dev_id) << 16) | ((HD_IN(in_id) & 0x00ff) << 8))
#define VENDOR_VIDEOPROC_CHIP_OUT(chip, dev_id, out_id)	((VENDOR_VIDEOPROC_CHIP(chip, dev_id) << 16) | (HD_OUT(out_id) & 0x00ff))
/********************************************************************
	TYPE DEFINITION
********************************************************************/

/************ di ************/
typedef struct _VENDOR_VIDEO_PARAM_DEI_INTERLACE {
	UINT32 di_en;  				///< DI ON/OFF
	UINT32 top_motion_en;  		///< enable/disable top field motion detection, default=1
	UINT32 bot_motion_en;  		///< enable/disable bottom field motion detection, default=1
	UINT32 auto_th_en;  		///< enable/disable auto md_th, default=1
	UINT32 strong_md_en;  		///< enable/disable strong motion detection, default=1
	UINT32 mmb_en;  			///< enable/disable motion macro block judgement, default=1
	UINT32 smb_en;  			///< enable/disable static macro block judgement, default=1
	UINT32 emb_en;  			///< enable/disable extend macro block judgement, default=1
	UINT32 lmb_en;  			///< enable/disable line macro block judgement, default=1
	UINT32 pmmb_en; 			///< enable/disable pmmb detection, mmb_en should be 1, default=1
	UINT32 corner_detect_en;  	///< enable/disable corner detection, default=1
	UINT32 di_gmm_motion_en;  	///< enable/disable GMM motion result reference, default=1
	UINT32 mmb_scene_change_en; ///< enable/disable mmb scene change detection, default=1
	UINT32 mmb_scene_change_th; ///< mmb scene change detect threshold, 0~2^16-1, default=90
	UINT32 all_motion;  		///< Set all block to motion. Only for debug. Defualt=0
	UINT32 all_static;  		///< Set all block to static. Only for debug. Default=0
	UINT32 strong_edge;  		///< Threshold of strong edge detection. 0~255, default=40
	UINT32 strong_md_th;  		///< Threshold of strong motion detection,0~255, default=40
	UINT32 md_th;  				///< Threshold of pixel motion detection. 0~255, default=8
	UINT32 line_admit;  		///< Connecting pixel count threshold for lmb detection. 0~15, default=6
	UINT32 mmb_th;  			///< Threshold of mmb judgement. 0~255, default=32
	UINT32 smb_th;  			///< Threshold of smb judgement. 0~255, default=10
	UINT32 emb_th;  			///< Threshold of emb judgement. 0~255, default=8
	UINT32 lmb_th;  			///< Threshold of lmb judgement 0~255, default=6
	UINT32 ela_h_th;  			///< edge level average high th. 0~255 (> ela_l_th), default=60
	UINT32 ela_l_th;  			///< edge level average low th. 0~255 (< ela_h_th), default=20
	UINT32 ch0_row1_status_ctrl;     //Boundary control. 0~3. default=0. 
	UINT32 ch1_row1_status_ctrl;     //Boundary control. 0~3. default=0. 
	UINT32 ch0_last_row_status_ctrl; //Boundary control. 0~3. default=0. 
	UINT32 ch1_last_row_status_ctrl; //Boundary control. 0~3. default=0. 	
} VENDOR_VIDEO_PARAM_DEI_INTERLACE;

typedef struct _VENDOR_VIDEO_DI_TMNR {
	UINT32 dn_en;  				///< DN ON/OFF
	UINT32 tmnr_learn_en;  		///< enable temporal strength learning, default=1
	UINT32 y_var;  				///< Luma pixel denoised strength, 0~255, default=20
	UINT32 cb_var;  			///< Cb pixel denoised strength, 0~255, default=12
	UINT32 cr_var;  			///< Cr pixel denoised strength, 0~255, default=12
	UINT32 k;  					///<  Motion detection var multiplier: 1 ~ 8, default=3
	UINT32 auto_k; 				///< Auto adjust K value 0~1, default = 1
	UINT32 auto_k_hi; 			///< Auto adjust K high value. 1~10, default= 5, must > auto_k_hi
	UINT32 auto_k_lo; 			///< Auto adjust K low value.1~10, default=2, must < auto_k_lo
	UINT32 trade_thres;  		///< Manual Y_var blending ration: 0 ~ 128, default = 64
	UINT32 suppress_strength;  	///< Max strength of Crawford filter : 2 ~ 64 (step by 2), default=18
	UINT32 nf;  				///< Var estimator normalize factor: 1 ~ 6, default=5
	UINT32 var_offset;  		///< variance offset limit : 0~15, default=2
	UINT32 motion_var;  		///< motion variance: 1 ~ 20, default=5
	UINT32 motion_th_mult;  	///< block motion ratio threshold: 1 ~ 100, default = 2
	UINT32 tmnr_fcs_th;  		///< false color threshold: 0 ~ 255, default = 10
	UINT32 tmnr_fcs_weight;  	///< false color suppression strength 0 ~ 16, default=8
	UINT32 tmnr_fcs_en;  		///< enable TMNR false color suppression; 0 ~ 1, default=0
	UINT32 dpr_motion_th;  		///< Defect Pixel Removal motion threshold: 0 ~ 255, defualt = 64
	UINT32 dpr_cnt_th;  		///< Defect Pixel Removal connection threshold: 0 ~ 15, default = 1
	UINT32 dpr_mode;  			///< Defect Pixel Removal mode: 0 ~ 1, default = 0
	UINT32 dpr_en;  			///< Defect Pixel Removal enable bit: 0 ~ 1, default = 0
} VENDOR_VIDEO_PARAM_DI_TMNR;

typedef struct _VENDOR_VIDEO_PARAM_DI_GMM {
	UINT32 gmm_en; 				///< enable gmm, default=1
	UINT32 gmm_alpha; 			///< Speed of update: 0 ~ 2^16-1, default=32
	UINT32 gmm_one_min_alpha; 	///< 2^15 - gmm_alpha
	UINT32 gmm_init_val; 		///< Initial value for the mixture of Gaussians.0~255, default=7
	UINT32 gmm_tb; 				///< threshold on the squared Mahalan. dist.0~15, default=9
	UINT32 gmm_sigma; 			///< Initial standard deviation for the newly generated components:0~31, default=9
	UINT32 gmm_tg; 				///< threshold to decide  when a sample is close to the existing components.0~15, default=9
	UINT32 gmm_prune; 			///< prune = -(alpha * CT)*8191/256; CT = 8, default = -8207
} VENDOR_VIDEO_PARAM_DI_GMM;

/************ dn ************/
typedef struct _VENDOR_VIDEO_MRNR {
	UINT32 mrnr_en;  					///< MRNR ON/OFF
	UINT32 t_y_edge_detection[2][8]; 	///< Edge pixel detection threshold of Y{layer1~layer2}, 0~1023
	UINT32 t_cb_edge_detection; 		///< Edge pixel detection threshold of Cb{layer2}, 0~1023
	UINT32 t_cr_edge_detection; 		///< Edge pixel detection threshold of Cr{layer2}, 0~1023
	UINT32 t_y_edge_smoothing[2][8]; 	///< Edge pixel smoothing threshold of Y: {layer1~layer2}, 0~255
	UINT32 t_cb_edge_smoothing; 		///< Edge pixel smoothing threshold of Cb: {layer2}, 0~255
	UINT32 t_cr_edge_smoothing;			///< Edge pixel smoothing threshold of Cr: {layer2}, 0~255
	UINT32 nr_strength_y[2]; 			///< Strength of noise reduction of Y: {layer1~layer2}, 0~15
	UINT32 nr_strength_c; 				///< Strength of noise reduction of CbCr: {layer2}, 0~15
} VENDOR_VIDEO_PARAM_MRNR;

typedef struct _VENDOR_VIDEO_PARAM_TMNR_EXT {
	UINT32 tmnr_en;  		///< 3DNR ON/OFF
	UINT32 luma_dn_en;  	///< Y channel 3DNR ON/OFF
	UINT32 chroma_dn_en;  	///< CbCr channel 3DNR ON/OFF
	UINT32 nr_str_y_3d; 	///< Y channel temporal NR strength, 0~32
	UINT32 nr_str_y_2d; 	///< Y channel spatial NR strength, 0~32
	UINT32 nr_str_c_3d; 	///< CbCr channel temporal NR strength, 0~32
	UINT32 nr_str_c_2d; 	///< CbCr channel spatial NR strength, 0~32
	UINT32 blur_str_y; 		///< Difference Y image blurred strength (0: No blur, 1: low-strength blur, 2: high-strength blur)
	UINT32 center_wzero_y_2d_en; 	///< Apply zero to center weight of Y spatial Bilateral filter
	UINT32 center_wzero_y_3d_en; 	///< Apply zero to center weight of Y temporal Bilateral filter
	UINT32 small_vibrat_supp_y_en; 	///< Y channel small vibration suppression ON/OF
	UINT32 avoid_residue_th_y; 		///< 1~4
	UINT32 avoid_residue_th_c; 		///< 1~4
	UINT32 display_motion_map_en; 	///< Display motion level map ON/OFF
	UINT32 motion_map_channel; 		///< Channel selection for motion level map display (0:Y, 1:Cb, 2:Cr)
	UINT32 motion_level_th_y_k1; 	///< Y channel match level threshold 1 adjustment, 0~32
	UINT32 motion_level_th_y_k2; 	///< Y channel match level threshold 2 adjustment, 0~32 AND K2 > K1
	UINT32 motion_level_th_c_k1; 	///< CbCr channel match level threshold 1 adjustment, 0~32
	UINT32 motion_level_th_c_k2; 	///< CbCr channel match level threshold 2 adjustment, 0~32 AND K2 > K1
	UINT32 lut_y_3d_1_th[4]; 		///< LUT of Y channel 3d_1 filter, 0~127
	UINT32 lut_y_3d_2_th[4]; 		///< LUT of CbCr channel 3d filter, 0~127
	UINT32 lut_y_2d_th[4]; 			///< LUT of Y channel 2d filter, 0~127
	UINT32 lut_c_3d_th[4]; 			///< LUT of CbCr channel 3d filter, 0~127
	UINT32 lut_c_2d_th[4]; 			///< LUT of CbCr channel 2d filter, 0~127
	UINT32 y_base[8]; 				///< Y channel noise model parameter: base noise level, 0 ~16320
	UINT32 y_coefa[8]; 				///< Y channel noise model parameter: slope of line, 0~48, Regard 16 as slope 1.0
	UINT32 y_coefb[8]; 				///< Y channel noise model parameter: intercept of line, 0 ~16320
	UINT32 y_std[8]; 				///< Y channel noise model parameter: noise standard deviation, 0 ~16320
	UINT32 cb_mean[8]; 				///< Cb channel noise model parameter: noise mean, 0 ~6375
	UINT32 cb_std[8]; 				///< Cb channel nosie model parameter: noise standard deviation, 0 ~6375
	UINT32 cr_mean[8]; 				///< Cr channel noise model parameter: noise mean, 0 ~6375
	UINT32 cr_std[8]; 				///< Cr channel nosie model parameter: noise standard deviation, 0 ~6375
	UINT32 tmnr_fcs_en;  	        ///< TMNR False Color Supression enable, **chroma_dn_en shoulde be 1
	UINT32 tmnr_fcs_str;   			///< TMNR_False Color Supression strength
	UINT32 tmnr_fcs_th;         	///< TMNR False Color Supression threshold
	//new in nt98321
	UINT8 dithering_en;   			///< source dithering switch. 0~1. default  0
	UINT8 dithering_bit_y;			///< Y-channel dithering range. 0~3. default 2
	UINT8 dithering_bit_u;			///< U-channel dithering range. 0~3. default 1
	UINT8 dithering_bit_v;			///< V-channel dithering range. 0~3. default 1
	UINT8 err_compensate;			///< err compensation method option. 0~1. default 1
} VENDOR_VIDEO_PARAM_TMNR_EXT;

/************ sharp ************/
typedef struct _VENDOR_VIDEO_PARAM_SHARP {
	UINT32 sharp_en;  			///< Sharpness ON/OFF
	UINT32 edge_weight_src_sel; ///< Select source of edge weight calculation, 0~1
	UINT32 edge_weight_th; 		///< Edge weight coring threshold, 0~255
	UINT32 edge_weight_gain; 	///< Edge weight gain, 0~255
	UINT32 noise_level; 		///< Noise Level, 0~255
	UINT32 noise_curve[17]; 	///< 17 control points of noise modulation curve, 0~255
	UINT32 blend_inv_gamma; 	///< Blending ratio of HPF results, 0~128
	UINT32 edge_sharp_str1; 	///< Sharpen strength1 of edge region, 0~255
	UINT32 edge_sharp_str2; 	///< Sharpen strength2 of edge region, 0~255
	UINT32 flat_sharp_str; 		///< Sharpen strength of flat region,0~255
	UINT32 coring_th; 			///< Coring threshold, 0~255
	UINT32 bright_halo_clip; 	///< Bright halo clip ratio, 0~255
	UINT32 dark_halo_clip; 		///< Dark halo clip ratio, 0~255
} VENDOR_VIDEO_PARAM_SHARP;

//--------------------------------------------------------------------------------------
// Edge Smooth interface
//--------------------------------------------------------------------------------------
typedef struct _VENDOR_VIDEO_PARAM_EDGE_SMOOTH {
	UINT8 edge_smooth_en; 			// Edge smooth enable, ON/OFF
	UINT8 edge_smooth_y_edeng_th_lo;	// luma layer edge energy low threshold, 0~255
	UINT8 edge_smooth_y_edeng_th_hi;	// luma layer edge energy high threshold, 0~255
	UINT8 edge_smooth_y_ew_lo;		// luma layer edge weighting curve low value, 0~32
	UINT8 edge_smooth_y_ew_hi;		// luma layer edge weighting curve high value, 0~32
	UINT8 edge_smooth_y_edi_th;		// luma layer edge mask threshold, 0~63
	UINT8 edge_smooth_y_ds_str;       // luma layer edge directed smoothing filter strength, 0 ~ 7
} VENDOR_VIDEO_PARAM_EDGE_SMOOTH;

#define MAX_MASK_NUM	8
typedef enum _VENDOR_VIDEO_MASK_AREA {
	MASK_INSIDE,
	MASK_OUTSIDE,
	MASK_LINE,
	MASK_MAXTYPE,
	ENUM_DUMMY4WORD(VENDOR_VIDEO_MASK_AREA)
} VENDOR_VIDEO_MASK_AREA;

typedef enum _VENDOR_VIDEO_MASK_BITMAP_TYPE {
	MASK_BITMAP_NONE,
	MASK_BITMAP_HIT_PAL,
	MASK_BITMAP_HIT_MAP,
	MASK_BITMAP_MAX,
	ENUM_DUMMY4WORD(VENDOR_VIDEO_MASK_BITMAP_TYPE)
} VENDOR_VIDEO_MASK_BITMAP_TYPE;

typedef struct _VENDOR_VIDEO_MASK_WIN {
	UINT32 vch; 			///< input channel
	UINT32 mask_idx; 		///< index = priority 0>1>2>3>4>5>6>7
	UINT32 enable; 			///< 1: enable, 0:disable, and ignore other settings
	VENDOR_VIDEO_MASK_AREA mask_area; //0:inside, 1:outside, 2:line
	HD_UPOINT point[4]; 	///< 4 points
} VENDOR_VIDEO_MASK_WIN;

typedef struct _VENDOR_VIDEO_MASK_CONFIG {
	UINT32 vch; 		///< input channel
	UINT32 mask_idx; 	///< index = priority 0>1>2>3>4>5>6>7
	UINT32 mosaic_en; 	///< use original image or mosaic image in mask area
	VENDOR_VIDEO_MASK_BITMAP_TYPE bitmap; ///< bitmap type
	UINT32 alpha; 		///< alpha blending 0~256, only effect at bitmap = 0,1
	UINT32 pal_sel; 	///< palette idx, only effect at bitmap = 0,1
} VENDOR_VIDEO_MASK_CONFIG;

typedef struct _VENDOR_VIDEO_MASK_BITMAP_ADDR {
	UINT32 vch; 		///< input channel
	UINT32 mask_idx; 	///< index = priority 0>1>2>3>4>5>6>7
	UINT32 bitmap_addr; ///< bitmap type
} VENDOR_VIDEO_MASK_BITMAP_ADDR;

typedef struct _VENDOR_VIDEO_MASK_BITMAP_SIZE {
	UINT32 vch; 			///< input channel
	UINT32 bitmap_width; 	///< should 16x
	UINT32 bitmap_height; 	///< bitmap_step_sz = 2^(1+bitmap_step),
							///< bitmap_height = (proc_height+bitmap_step_sz-1)/bitmap_step_sz
} VENDOR_VIDEO_MASK_BITMAP_SIZE;

typedef struct _VENDOR_VIDEO_MASK_BLOCK {
	UINT32 vch; 			///< input channel
	UINT32 mosaic_blkw; 	///< mosaic block w
	UINT32 mosaic_blkh; 	///< mosaic block h
} VENDOR_VIDEO_MASK_BLOCK;

typedef struct _VENDOR_VIDEO_PARAM_MASK {
	VENDOR_VIDEO_MASK_WIN mask_window[MAX_MASK_NUM];
	VENDOR_VIDEO_MASK_CONFIG mask_config[MAX_MASK_NUM];
	VENDOR_VIDEO_MASK_BITMAP_ADDR bitmap_addr[MAX_MASK_NUM];
	VENDOR_VIDEO_MASK_BITMAP_SIZE bitmap_size[MAX_MASK_NUM];
	VENDOR_VIDEO_MASK_BLOCK mosaic_block;
} VENDOR_VIDEO_PARAM_MASK;

/************ lpm ************/
typedef struct _VENDOR_VIDEO_PARAM_LPM {
	UINT8           enable;             ///< low power mode. default: 0, range: 0~2 (0: disable, 1: control by cu16, 2: enable)
} VENDOR_VIDEO_PARAM_LPM;

/************ rnd ************/
typedef struct _VENDOR_VIDEO_PARAM_RND {
	BOOL            enable;             ///< random noise enable. default: 0, range: 0~1 (0: disbale, 1: enable)
	UINT8           range;              ///< noise range for the random noise added to the source. default: 1, range: 1~10
} VENDOR_VIDEO_PARAM_RND;

/************ motion aq ************/
typedef struct _VENDOR_VIDEO_PARAM_MOTION_AQ {
	UINT8           mode;               ///< mode for delta QPs in the motion area. default: 0, range: 0~3 (0: DeltaQP_AQ, 1: DeltaQP_AQ + DeltaQP_Motion, 2: min(DeltaQP_AQ, DeltaQP_Motion), 3: DeltaQP_Motion)
	UINT8           delta_qp_roi_th;    ///< threshold for ROI labeling. default: 3, range: 0~3
										///< if ( Cu16.MotionCount > 2*delta_qp_roi_th ) Cu16.isROI = True; (Note: Cu16.MotionCount is ranged from 0 to 6)
	INT8            delta_qp[6];        ///< delta QPs for motion-based AQ. default: 0, range: -16~15 (-16: disable)
} VENDOR_VIDEO_PARAM_MOTION_AQ;

typedef struct _VENDOR_VIDEO_PARAM_TMNR_STA_DATA {
	UINT32 timeout_int;     ///< Timeout interval (msec)
	UINT32 blk_no;          ///< TMNR statistic data block count
	UINT32 *p_stabuf;       ///< TMNR statistic data buffer
} VENDOR_VIDEO_PARAM_TMNR_STA_DATA;

typedef struct _VENDOR_VIDEO_PARMA_SCA_CTRL {
	INT32 sca_ceffH[4]; 				///< LPF coefficient in horizontal direction
	INT32 sca_ceffV[4]; 				///< LPF coefficient in vertical direction
} VENDOR_VIDEO_PARMA_SCA_CTRL;

typedef struct _VENDOR_VIDEO_PARAM_SCA_SET {
	UINT8 sca_y_luma_algo_en; 			///< Algorithm select for vertical luma scaler
	UINT8 sca_x_luma_algo_en; 			///< Algorithm select for horizontal luma scaler
	UINT8 sca_y_chroma_algo_en; 		///< Algorithm select for vertical chroma scaler
	UINT8 sca_x_chroma_algo_en; 		///< Algorithm select for horizontal chroma scaler
	UINT8 sca_map_sel; 					///< Scaler source mapping format select
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_1000x_param;  ///< scaling parameter for scaling ratio 1.00x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_1250x_param;  ///< scaling parameter for scaling ratio 1.25x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_1500x_param;  ///< scaling parameter for scaling ratio 1.50x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_1750x_param;  ///< scaling parameter for scaling ratio 1.75x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_2000x_param;  ///< scaling parameter for scaling ratio 2.00x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_2250x_param;  ///< scaling parameter for scaling ratio 2.25x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_2500x_param;  ///< scaling parameter for scaling ratio 2.50x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_2750x_param;  ///< scaling parameter for scaling ratio 2.75x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_3000x_param;  ///< scaling parameter for scaling ratio 3.00x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_3250x_param;  ///< scaling parameter for scaling ratio 3.25x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_3500x_param;  ///< scaling parameter for scaling ratio 3.50x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_3750x_param;  ///< scaling parameter for scaling ratio 3.75x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_4000x_param;  ///< scaling parameter for scaling ratio 4.00x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_5000x_param;  ///< scaling parameter for scaling ratio 5.00x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_6000x_param;  ///< scaling parameter for scaling ratio 6.00x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_7000x_param;  ///< scaling parameter for scaling ratio 7.00x
	VENDOR_VIDEO_PARMA_SCA_CTRL sca_8000x_param;  ///< scaling parameter for scaling ratio 8.00x
} VENDOR_VIDEO_PARAM_SCA_SET;

typedef struct _VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF {
	UINTPTR p_start_pa;       	///< TMNR motion buffer start physical address
	UINT32 ddr_id;				///< TMNR motion buffer ddr_id
	UINT32 size;				///< TMNR motion buffer size
} VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF;

typedef enum _VENDOR_VIDEO_SCA_RANGE {
	SCA_RANGE_BYPASS = 0,		///< bypass YUV level transfer
	SCA_RANGE_PC2TV = 1, 		///< transfer to TV level (Y:16~235, C:16~240)
	SCA_RANGE_TV2PC = 2,  		///< transfer to PC level (Y:0~255, C:0~255)
	SCA_RANGE_DISABLE = 3,		///< disable YUV level selection
	SCA_RANGE_MAXTYPE = 4
} VENDOR_VIDEO_SCA_RANGE;

typedef struct _VENDOR_VIDEO_PARAM_SCA_RANGE {
	VENDOR_VIDEO_SCA_RANGE yuv_range;
} VENDOR_VIDEO_PARAM_YUV_RANGE;

/************ lcd iq ************/
typedef struct _VENDOR_VIDEO_PARAM_LCD_IQ {
	/* for Both */
	INT32 brightness;  			///< brightness, -127~127

	/* for LCD300 only */
	UINT32 ce_en; 				///< for LCD300, contrast enhancement ON/OFF
	UINT32 ce_strength; 		///< for LCD300, CE strength, 0~1023
	UINT32 sharp_en; 			///< sharpness ON/OFF
	UINT32 sharp_hpf0_5x5_gain; ///< for LCD300, sharpness high pass filter 0, 0~1023
	UINT32 sharp_hpf1_5x5_gain; ///< for LCD300, sharpness high pass filter 1, 0~1023
	UINT32 contrast; 			///< for LCD300, contrast ratio, 0~255
	UINT32 contrast_mode; 		///< for LCD300, contrast mode: 0:standard, 1:shadow, 2:highlight
	UINT32 hue_sat_en;			///< for LCD300, hue and saturation ON/OFF
	INT32 hue[6];				///< for LCD300, hue, -45~45
	UINT32 saturation[6];		///< for LCD300, saturation, 0~255

	/* for LCD200 only */
	UINT32 sharp_k0; 			///< for LCD200, sharpness k0, 0~15
	UINT32 sharp_k1; 			///< for LCD200, sharpness k1, 0~15
	UINT32 sharp_threshold0; 	///< for LCD200, sharpness threshold 0, 0~255
	UINT32 sharp_threshold1; 	///< for LCD200, sharpness threshold 1, 0~255
	UINT32 contrast_val; 		///< for LCD200, contrast value, 1~31
	INT32 hue_cos_val; 			///< for LCD200, hue cosin value, -32~32
	INT32 hue_sin_val; 			///< for LCD200, hue sin value, -32~32

} VENDOR_VIDEO_PARAM_LCD_IQ;

/************ lcd hue ************/
typedef struct _VENDOR_VIDEO_PARAM_LCD_HUE_SAT {
	INT hue_sat_enable;  	///< 0 for disabled, 1 for enabled
    UINT hue_sat[6];	    ///< 0 ~ 255 
} VENDOR_VIDEO_PARAM_LCD_HUE_SAT;

/************ lcd ygamma ************/
typedef struct _VENDOR_VIDEO_PARAM_LCD_YGAMMA {
	INT	gamma_state;	///< 0 for disabled, 1 for enabled 
	UINT index[16];	///<consists of (index >> 8) & 0x3 | (index & 0x1F) 
	UINT y_gamma[32];	///< 0 ~ 4095 
} VENDOR_VIDEO_PARAM_LCD_YGAMMA;

typedef enum _VENDOR_VIDEO_PARAM_ID {
	VENDOR_VIDEO_DI_GMM,        ///< VENDOR_VIDEO_PARAM_DI_GMM for videoprocess
	VENDOR_VIDEO_DI_TMNR,       ///< VENDOR_VIDEO_PARAM_DI_TMNR for videoprocess
	VENDOR_VIDEO_DI_INTERLACE,  ///< VENDOR_VIDEO_PARAM_DI_INTERLACE for videoprocess
	VENDOR_VIDEO_MASK,			///< VENDOR_VIDEO_PARAM_MASK for videoprocess
	VENDOR_VIDEO_LPM,           ///< VENDOR_VIDEO_PARAM_LPM
	VENDOR_VIDEO_RND,           ///< VENDOR_VIDEO_PARAM_RND
	VENDOR_VIDEO_MOTION_AQ,     ///< VENDOR_VIDEO_PARAM_MOTION_AQ
	VENDOR_VIDEO_DN_2D,			///< VENDOR_VIDEO_PARAM_MRNR for videoprocess 
	VENDOR_VIDEO_SHARP,         ///< VENDOR_VIDEO_PARAM_SHARP for videoprocess
	VENDOR_VIDEO_TMNR_CTRL,     ///< VENDOR_VIDEO_PARAM_TMNR_EXT
	VENDOR_VIDEO_TMNR_STA_DATA, ///< VENDOR_VIDEO_PARAM_TMNR_STA_DATA
	VENDOR_VIDEO_TMNR_MOTION_BUF,  ///< VENDOR_VIDEO_PARAM_TMNR_MOTION_BUF
	VENDOR_VIDEO_SCA,			///< VENDOR_VIDEO_PARAM_SCA_SET
	VENDOR_VIDEO_LCD_IQ,		///< VENDOR_VIDEO_PARAM_LCD_IQ
	VENDOR_VIDEO_YUV_RANGE,		///< VENDOR_VIDEO_PARAM_YUV_RANGE
	VENDOR_VIDEO_LCD_HUE,		///< VENDOR_VIDEO_PARAM_LCD_HUE_SAT
	VENDOR_VIDEO_LCD_YGAMMA,    ///< VENDOR_VIDEO_PARAM_LCD_YGAMMA
	VENDOR_VIDEO_EDGE_SMOOTH,   ///< VENDOR_VIDEO_PARAM_EDGE_SMOOTH
    VENDOR_VIDEO_MAX,
	ENUM_DUMMY4WORD(VENDOR_VIDEO_PARAM_ID)
} VENDOR_VIDEO_PARAM_ID;

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/

HD_RESULT vendor_video_get(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_ID id, void *p_param);
HD_RESULT vendor_video_set(HD_PATH_ID path_id, VENDOR_VIDEO_PARAM_ID id, void *p_param);
HD_RESULT vendor_video_init(HD_PATH_ID path_id);
HD_RESULT vendor_video_uninit(HD_PATH_ID path_id);
#endif

