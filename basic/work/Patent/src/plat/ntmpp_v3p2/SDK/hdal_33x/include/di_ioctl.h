#ifndef _DI_IOCTL_H_
#define _DI_IOCTL_H_

#if defined(WIN32) || !defined(__KERNEL__)
	#include <stdint.h>
#else
	#include <linux/types.h>
#endif
#include "dei_ioctl.h"
/*
typedef struct fd_di_en_info {
	int32_t fd;
	int32_t enable;
}fd_di_en_info_t;
*/
#define fd_di_en_info_t fd_dei_en_info_t
/*
typedef struct di_gmm_param {
	uint32_t gmm_alpha; //Speed of update: 0 ~ 2^16-1, default=32
	uint32_t gmm_one_min_alpha; //2^15 - gmm_alpha
	uint32_t gmm_init_val; //Initial value for the mixture of Gaussians.0~255, default=7
	uint32_t gmm_tb; //threshold on the squared Mahalan. dist.0~15, default=9
	uint32_t gmm_sigma; //Initial standard deviation for the newly generated components:0~31, default=9
	uint32_t gmm_tg; //threshold to decide  when a sample is close to the existing components.0~15, default=9
	int32_t	 gmm_prune; //prune = -(alpha * CT)*8191/256; CT = 8, default = -8207
}di_gmm_param_t;
*/
#define di_gmm_param_t dei_gmm_param_t
/*
typedef struct fd_di_gmm_param {
	int32_t fd;
	di_gmm_param_t param;
}fd_di_gmm_param_t;
*/
#define fd_di_gmm_param_t fd_dei_gmm_param_t
/*
typedef struct di_tmnr_param {
	uint8_t tmnr_learn_en;  //enable temporal strength learning, default=1
	uint8_t y_var;  //Luma pixel denoised strength, 0~255, default=20
	uint8_t cb_var;  //Cb pixel denoised strength, 0~255, default=12
	uint8_t cr_var;  //Cr pixel denoised strength, 0~255, default=12
	uint8_t k;  // Motion detection var multiplier: 1 ~ 8, default=3
	uint8_t auto_k; //Auto adjust K value 0~1, default = 1
	uint8_t auto_k_hi; //Auto adjust K high value. 1~10, default= 5, must > auto_k_hi
	uint8_t auto_k_lo; //Auto adjust K low value.1~10, default=2, must < auto_k_lo
	uint8_t trade_thres;  //Manual Y_var blending ration: 0 ~ 128, default = 64
	uint8_t suppress_strength;  //Max strength of Crawford filter : 2 ~ 64 (step by 2), default=18
	uint8_t nf;  //Var estimator normalize factor: 1 ~ 6, default=5
	uint8_t var_offset;  //variance offset limit : 0~15, default=2
	uint8_t motion_var;  //motion variance: 1 ~ 20, default=5
	uint8_t motion_th_mult;  //block motion ratio threshold: 1 ~ 100, default = 2
	uint8_t tmnr_fcs_th;  //false color threshold: 0 ~ 255, default = 10
	uint8_t tmnr_fcs_weight;  //false color suppression strength 0 ~ 16, default=8
	uint8_t tmnr_fcs_en;  // enable TMNR false color suppression; 0 ~ 1, default=0
	uint8_t dpr_motion_th;  //Defect Pixel Removal motion threshold: 0 ~ 255, defualt = 64
	uint8_t dpr_cnt_th;  //Defect Pixel Removal connection threshold: 0 ~ 15, default = 1
	uint8_t dpr_mode;  //Defect Pixel Removal mode: 0 ~ 1, default = 0
	uint8_t dpr_en;  //Defect Pixel Removal enable bit: 0 ~ 1, default = 0
}di_tmnr_param_t;
*/
#define di_tmnr_param_t dei_tmnr_param_t
/*
typedef struct fd_di_tmnr_param {
	int32_t fd;
	di_tmnr_param_t param;
}fd_di_tmnr_param_t;
*/
#define fd_di_tmnr_param_t fd_dei_tmnr_param_t
/*
typedef struct di_die_param {
	uint32_t top_motion_en;  //enable/disable top field motion detection, default=1
	uint32_t bot_motion_en;  //enable/disable bottom field motion detection, default=1
	uint32_t auto_th_en;  //enable/disable auto md_th, default=1
	uint32_t strong_md_en;  //enable/disable strong motion detection, default=1
	uint32_t mmb_en;  //enable/disable motion macro block judgement, default=1
	uint32_t smb_en;  //enable/disable static macro block judgement, default=1
	uint32_t emb_en;  //enable/disable extend macro block judgement, default=1
	uint32_t lmb_en;  //enable/disable line macro block judgement, default=1
	uint32_t pmmb_en; //enable/disable pmmb detection, mmb_en should be 1, default=1
	uint32_t corner_detect_en;  //enable/disable corner detection, default=1
	uint32_t di_gmm_motion_en;  //enable/disable GMM motion result reference, default=1
	uint32_t mmb_scene_change_en;  //enable/disable mmb scene change detection, default=1
	uint32_t mmb_scene_change_th;  //mmb scene change detect threshold, 0~2^16-1, default=90
	uint32_t all_motion;  //Set all block to motion. Only for debug. Defualt=0
	uint32_t all_static;  //Set all block to static. Only for debug. Default=0
	uint32_t strong_edge;  //Threshold of strong edge detection. 0~255, default=40
	uint32_t strong_md_th;  //Threshold of strong motion detection,0~255, default=40
	uint32_t md_th;  //Threshold of pixel motion detection. 0~255, default=8
	uint32_t line_admit;  //Connecting pixel count threshold for lmb detection. 0~15, default=6
	uint32_t mmb_th;  //Threshold of mmb judgement. 0~255, default=32
	uint32_t smb_th;  //Threshold of smb judgement. 0~255, default=10
	uint32_t emb_th;  //Threshold of emb judgement. 0~255, default=8
	uint32_t lmb_th;  //Threshold of lmb judgement 0~255, default=6
	uint32_t ela_h_th;  //edge level average high th. 0~255 (> ela_l_th), default=60
	uint32_t ela_l_th;  //edge level average low th. 0~255 (< ela_h_th), default=20

	//new in NT98321.
	uint32_t ch0_row1_status_ctrl;     //Boundary control. 0~3. default=0.
	uint32_t ch1_row1_status_ctrl;     //Boundary control. 0~3. default=0.
	uint32_t ch0_last_row_status_ctrl; //Boundary control. 0~3. default=0.
	uint32_t ch1_last_row_status_ctrl; //Boundary control. 0~3. default=0.
} di_die_param_t;
*/
#define di_die_param_t dei_die_param_t
/*
typedef struct fd_di_die_param {
	int32_t fd;
	di_die_param_t param;
}fd_di_die_param_t;
*/
#define fd_di_die_param_t fd_dei_die_param_t

//--------------------------------------------------------------------------------------
// hw information
//--------------------------------------------------------------------------------------
/*
typedef struct di_pty_src_info {
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
} di_pty_src_info_t;
*/
#define di_pty_src_info_t dei_pty_src_info_t
/*
typedef struct di_pty_dst_info {
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
} di_pty_dst_info_t;
*/
#define di_pty_dst_info_t dei_pty_dst_info_t

#define DI_FEATURE_PRO 	DEI_FEATURE_PRO//	0x00000000	//input progressive mode
#define DI_FEATURE_INTER	DEI_FEATURE_INTER//0x00000001	//input interlace mode
/*
typedef struct di_hw_info {
	uint32_t feature;			//feature
	uint32_t src_fmt;			//image format TYPE_YUV420_SP/TYPE_YUV422/......
	uint32_t dst_fmt;			//image format TYPE_YUV420_SP/TYPE_YUV422/......

	di_pty_src_info_t src;			//src align information
	di_pty_dst_info_t dst;			//dst align information
} di_hw_info_t;
*/
#define di_hw_info_t dei_hw_info_t
//--------------------------------------------------------------------------------------
// module initial param
//--------------------------------------------------------------------------------------
/*
typedef struct di_die_init_info {
	uint32_t st_size;       //structure size
	uint32_t chip_num;
	uint32_t eng_num;
	uint32_t minor_num;
	uint32_t act_ch_num;
} di_die_init_info_t;
*/
#define di_die_init_info_t dei_init_info_t

#define DI_IOC_MAGIC	                DEI_IOC_MAGIC//   'x'
#define DI_IOC_GET_GMM_ENABLE			DEI_IOC_GET_GMM_ENABLE//		_IOWR(DI_IOC_MAGIC,  100, fd_di_en_info_t*)
#define DI_IOC_SET_GMM_ENABLE			DEI_IOC_SET_GMM_ENABLE//		_IOW(DI_IOC_MAGIC,  100, fd_di_en_info_t*)
#define DI_IOC_GET_GMM_INFO				DEI_IOC_GET_GMM_INFO//		_IOWR(DI_IOC_MAGIC,  101, fd_di_gmm_param_t*)
#define DI_IOC_SET_GMM_INFO				DEI_IOC_SET_GMM_INFO//		_IOW(DI_IOC_MAGIC,  101, fd_di_gmm_param_t*)
#define DI_IOC_GET_TMNR_ENABLE			DEI_IOC_GET_TMNR_ENABLE//		_IOWR(DI_IOC_MAGIC,  102, fd_di_en_info_t*)
#define DI_IOC_SET_TMNR_ENABLE			DEI_IOC_SET_TMNR_ENABLE//		_IOW(DI_IOC_MAGIC,  102, fd_di_en_info_t*)
#define DI_IOC_GET_TMNR_INFO			DEI_IOC_GET_TMNR_INFO//		_IOWR(DI_IOC_MAGIC,  103, fd_di_tmnr_param_t*)
#define DI_IOC_SET_TMNR_INFO			DEI_IOC_SET_TMNR_INFO//		_IOW(DI_IOC_MAGIC,  103, fd_di_tmnr_param_t*)
#define DI_IOC_GET_DIE_INFO				DEI_IOC_GET_DIE_INFO//		_IOWR(DI_IOC_MAGIC,  104, fd_di_die_param_t*)
#define DI_IOC_SET_DIE_INFO				DEI_IOC_SET_DIE_INFO//		_IOW(DI_IOC_MAGIC,  104, fd_di_die_param_t*)

#define DI_IOC_SET_INIT					DEI_IOC_SET_INIT//		_IOW(DI_IOC_MAGIC,  217, di_die_init_info_t*)
#define DI_IOC_SET_UNINIT				DEI_IOC_SET_UNINIT//		_IO(DI_IOC_MAGIC,  218)
#define DI_IOC_GET_HW_SPEC_INFO			DEI_IOC_GET_HW_SPEC_INFO//		_IOWR(DI_IOC_MAGIC,  219, di_hw_info_t)

//extern int di_module_param_cfg(unsigned int cmd, unsigned long arg);
//extern int di_module_init(di_die_init_info_t *info);
//extern int di_module_uninit(void);
#define di_module_param_cfg dei_module_param_cfg
#define di_module_init dei_module_init
#define di_module_uninit dei_module_uninit
#endif //_DI_IOCTL_H_
