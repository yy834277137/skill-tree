#ifndef _H26XENC_IOCTL_H_
#define _H26XENC_IOCTL_H_

#if defined(WIN32) || !defined(__KERNEL__)
    #include <stdint.h>
#else
    #include <linux/types.h>
#endif
#include "common_def.h"
#include "common_param.h"
#include "kwrap/ioctl.h"

#define OSG_MAX_STEP 8

typedef struct {
	uint32_t fd;
	uint32_t enable;
} FD_H26XENC_EN_INFO;

////////// TMNR //////////
typedef struct {
    uint8_t luma_dn_en;             ///< Y channel TMNR on/off. default: 1, range: 0~1 (0: disable, 1: enable)
    uint8_t chroma_dn_en;           ///<  CbCr channel TMNR on/off. default: 1, range: 0~1 (0: disable, 1: enable)
    uint8_t tmnr_fcs_en;            ///<  TMNR False Color Supression enable. default:0, range: 0~1 (Note: chroma_dn_en shoulde be 1)
    uint8_t nr_str_y_3d;            ///<  Y channel temporal NR strength. default: 4, range: 0~32
    uint8_t nr_str_y_2d;            ///<  Y channel  spatial NR strength. default: 4, range: 0~32
    uint8_t nr_str_c_3d;            ///<  CbCr channel temporal NR strength. default: 4, range: 0~32
    uint8_t nr_str_c_2d;            ///<  CbCr channel  spatial NR strength. default: 4, range: 0~32
    uint8_t blur_str_y;             ///<  Difference Y image blurred strength. default: 1, range 0~2 (0: No blur, 1: low-strength blur, 2: high-strength blur)
    uint8_t center_wzero_y_2d_en;   ///<  Apply zero to center weight of Y  spatial Bilateral filter. default: 1, range 0~1
    uint8_t center_wzero_y_3d_en;   ///<  Apply zero to center weight of Y temporal Bilateral filter. default: 1, range 0~1
    uint8_t small_vibrat_supp_y_en; ///<  Y channel small vibration suppression ON/OFF. default: 0, range 0~1
    uint8_t avoid_residue_th_y;     ///< Y channel temporal absolute difference bigger than this threshold will not be suppressed to zero after NR. default: 1, range 1~4
    uint8_t avoid_residue_th_c;     ///<  CbCr channel temporal absolute difference bigger than this threshold will not be suppressed to zero after NR. default: 1, range 1~4
    uint32_t y_base[8];             ///<  Y channel noise model parameter: base noise level. default: {100,100,100,100,100,100,100,100}, range: 0~255*64
    uint8_t motion_level_th_y_k1;   ///<  Y channel motion level threshold 1 adjustment. default:  8, range: 0~32
    uint8_t motion_level_th_y_k2;   ///<  Y channel motion level threshold 2 adjustment. default: 16, range: 0~32 (Note: K2 > K1)
    uint8_t motion_level_th2_y_k1;  ///<  Y channel motion level threshold 1 adjustment. default:  8, range: 0~32
    uint8_t motion_level_th2_y_k2;  ///<  Y channel motion level threshold 2 adjustment. default: 16, range: 0~32 (Note: K2 > K1)
    uint8_t motion_level_th_c_k1;   ///<  CbCr channel motion level threshold 1 adjustment. default:  8, range: 0~32
    uint8_t motion_level_th_c_k2;   ///<  CbCr channel motion level threshold 2 adjustment. default: 16, range: 0~32 (Note: K2 > K1)
    uint32_t y_coefa[8];            ///<  Y channel noise model parameter: slope of line. default: {8,8,8,8,8,8,8,8}, range: 0~48
    uint32_t y_coefb[8];            ///<  Y channel noise model parameter: intercept of line. default: {100,100,100,80,70,50,50,50}, range: 0~255*64
    uint32_t y_std[8];              ///<  Y channel noise model parameter: noise standard deviation. default: {20,20,20,20,20,20,20,20}, range: 0~255*64
    uint32_t cb_mean[8];            ///<  Cb channel noise model parameter: noise mean. default: {100,100,100,100,100,100,100,100}, range: 0~255*25
    uint32_t cb_std[8];             ///<  Cb channel nosie model parameter: noise standard deviation. default: {20,20,20,20,20,20,20,20}, range 0~255*25
    uint32_t cr_mean[8];            ///<  Cr channel noise model parameter: noise mean. default: {100,100,100,100,100,100,100,100}, range: 0~255*25
    uint32_t cr_std[8];             ///<  Cr channel nosie model parameter: noise standard deviation. default: {20,20,20,20,20,20,20,20}, range 0~255*25
    uint8_t lut_y_3d_1_th[4];       ///<  LUT of Y channel 3D_1 filter. default: {11,33,55,77}, range: 0~127
    uint8_t lut_y_3d_2_th[4];       ///<  LUT of Y channel 3D_2 filter. default: {40,14, 7, 3}, range: 0~127
    uint8_t lut_y_2d_th[4];         ///<  LUT of Y channel 2D   filter. default: {11,33,55,77}, range: 0~127
    uint8_t lut_c_3d_th[4];         ///<  LUT of CbCr channel 3D filter. default: {37,19,11, 7}, range: 0~127
    uint8_t lut_c_2d_th[4];         ///<  LUT of CbCr channel 2D filter. default: {11,33,55,77}, range: 0~127
    uint8_t tmnr_fcs_str;           ///<  TMNR_False Color Supression strength. default: 4, range: 0~15
    uint8_t tmnr_fcs_th;            ///<  TMNR False Color Supression threshold. default: 32, range: 0~255

    uint8_t ref_cmpr_en;            ///<  TMNR reference compression enable. range: 0~1
    uint8_t err_compensate;         ///<  TMNR error compensate. range: 0~1
    uint8_t dithering_enable;       ///<  TMNR source dithering enable. range: 0~1
    uint8_t dithering_bit_y;        ///<  TMNR source dithering y. range: 1~32
    uint8_t dithering_bit_cb;       ///<  TMNR source dithering cb. range: 1~32
    uint8_t dithering_bit_cr;       ///<  TMNR source dithering cr. range: 1~32
} H26XENC_TMNR_PARAM;

typedef struct {
    uint32_t fd;
    H26XENC_TMNR_PARAM st_tmnr_param;
} FD_H26XENC_TMNR_PARAM;

////////// MASK //////////
typedef struct {
	uint32_t pos_x;             ///< [r/w] mask window postion x. range: 0 ~ encode width - 1
	uint32_t pos_y;             ///< [r/w] mask window postion y. range: 0 ~ encode height - 1
} H26XENC_MASK_POS;

typedef struct {
	uint8_t mask_idx;           ///< [r/w] index of mask. default: 0, range: 0~7
	uint8_t enable;             ///< [r/w] enable mask window. default: 0, range: 0~1 (0: disable, 1: enable)
	uint8_t mosaic_en;          ///< [r/w] mask window de-identified method. range: 0~1 (0: orignial, 1: mosaic)
	uint8_t line_hit_opt;       ///< [r/w] mask window line hit operation. 0: and(inside), 1: or(outside), 2: border
	uint32_t alpha;             ///< [r/w] mask window blending alpha. range : 0 ~ 256
	H26XENC_MASK_POS st_pos[4]; ///< [r/w] mask window position configure settings.
	uint8_t pal_sel;
} H26XENC_MASK_WIN;

typedef struct {
    uint32_t fd;
    H26XENC_MASK_WIN st_mask_win;
} FD_H26XENC_MASK_WIN;

typedef struct {
    uint8_t pal_y[8];           ///< [r/w] palette colors y. range: 0 ~ 255
    uint8_t pal_cb[8];          ///< [r/w] palette colors cb. range: 0 ~ 255
    uint8_t pal_cr[8];          ///< [r/w] palette colors cr. range: 0 ~ 255
} H26XENC_MASK_PALETTE_TABLE;

typedef struct {
    uint32_t fd;
    H26XENC_MASK_PALETTE_TABLE st_mask_pal;
} FD_H26XENC_MASK_PALETTE_TABLE;

typedef struct {
	uint8_t mosaic_blk_w;       ///< [r/w] mosaic block width. range: 0 ~ 255
	uint8_t mosaic_blk_h;       ///< [r/w] mosaic block height. range: 0 ~ 255
} H26XENC_MASK_MOSAIC_BLOCK;

typedef struct {
    uint32_t fd;
    H26XENC_MASK_MOSAIC_BLOCK st_mask_mosaic_block;
} FD_H26XENC_MASK_MOSAIC_BLOCK;

////////// OSG //////////
typedef struct {
	uint8_t type;               ///< [r/w] osg graphic type. range: 0~1 (0: argb1555, 1: argb8888)
	uint32_t width;             ///< [r/w] osg graphic width and this field shall be mulitple of 8. range: 0~encode width-1
	uint32_t height;            ///< [r/w] osg grahic height and this field shall be mulitple of 2. range: 0~510
	uintptr_t pa_addr;          ///< [r/w] osg graphic data physical address.
	uint16_t line_offset;		///< [r/w] osg line offset of osg graph, must be multiple of 4
	//uint32_t va_addr;         ///< [r/w] osg graphic data virtual address.
} H26XENC_OSG_GRAP;

typedef struct {
	uint8_t mode;				///< [r/w] osg display mode 0: display OSG, 1: display mask
	uint32_t pos_x;             ///< [r/w] osg display position of x and this field shall be multiple of 2. range: 0~encode width-2
	uint32_t pos_y;             ///< [r/w] osg display position of y and this field shall be multiple of 2. range: 0~encode hieght-2
	uint8_t bg_alpha;           ///< [r/w] osg display foreground trnsparency. range: 0~7
	uint8_t fg_alpha;           ///< [r/w] osg display background trnsparency. range: 0~7
} H26XENC_OSG_DISP;

typedef struct {
	uint8_t enable;             ///< [r/w] osg gcac enable. range: 0~1 (0: disable, 1: enable)
	uint8_t gcac_blk_width;     //< NVR only. unit width of GCAC
	uint8_t gcac_blk_height;    ///< NVR only. unit height of GCAC. Note: OSG dim / (gcac_blk_width* gcac_blk_height) must less than 64
} H26XENC_OSG_GCAC;

typedef struct {
	uint8_t osg_idx;            ///< [r/w] index of osg window. default: 0, range: 0~7
	uint8_t enable;             ///< [r/w] osg window enable bit. range: 0~1 (0: disable, 1: enable)
	uint8_t ddr_id;             ///< [r/w] osg window ddr id.
	H26XENC_OSG_GRAP st_grap;   ///< [r/w] osg window graphic(input) configure settings.
	H26XENC_OSG_DISP st_disp;   ///< [r/w] osg window display(output) configure settings.
	H26XENC_OSG_GCAC st_gcac;   ///< [r/w] osg window gcac configure settings.
} H26XENC_OSG_WIN;

typedef struct {
    uint32_t fd;
    H26XENC_OSG_WIN st_osg_win;
} KFLOW_VIDEOENC_OSG_WIN;

typedef struct {
	uint8_t rgb_to_yuv[3][3];   ///< [r/w] osg color space transformation matrix. range: 0~255
} H26XENC_OSG_RGB2YUV;

typedef struct {
    uint32_t fd;
    H26XENC_OSG_RGB2YUV st_osg_rgb2yuv;
} FD_H26XENC_OSG_RGB2YUV;

////////// MEM POOL //////////
typedef struct {
    unsigned int buf_id;
    unsigned int chip_id;
    uintptr_t addr_pa;
    unsigned int size;
    unsigned int ddr_no;
} H26XENC_REF_BUFFER_INFO;

////////// MODULE //////////
typedef struct {
    unsigned int max_width;
    unsigned int max_height;
    unsigned int max_chn;
} H26X_ENC_MOD_PARAM;

typedef struct {
    unsigned int h26x_fd;
    unsigned int jpeg_fd;
} KFLOW_VIDEOENC_FD_INFO;

typedef struct {
    struct {
    	dim_t min_dim;
    	dim_t max_dim;
    	align_level_dim_t  align;
    	align_level_dim_t  align_compress;
    } h264;

    struct {
    	dim_t min_dim;
    	dim_t max_dim;
    	align_level_dim_t  align;
    	align_level_dim_t  align_compress;
    } h265;

    struct {
    	unsigned int  type_support_bit_array[(MAX_VPD_BUFTYPE_COUNT+31)/32];
    	unsigned int  in_buf_addr_align_level;
    	unsigned int  out_bs_addr_align_level;
    	unsigned int  qp_map_unit;
    	unsigned int  roi_win_max_num;
    	unsigned int  osg_win_max_num;
    	unsigned int  osg_pal_max_num;
    	unsigned int  mot_buf_max_num;
		unsigned int  osg_width_align;
		unsigned int  osg_height_align;
		unsigned int  osg_x_align;
		unsigned int  osg_y_align;
		unsigned int  osg_line_offset_align;
		unsigned int  osg_support_mask;

		unsigned int  mask_win_max_num;
		unsigned int  mask_width_align;
		unsigned int  mask_height_align;
		unsigned int  mask_x_align;
		unsigned int  mask_y_align;
    } h26x;

    struct {
    	unsigned int  type_support_bit_array[(MAX_VPD_BUFTYPE_COUNT+31)/32];
    	dim_t min_dim;
    	dim_t max_dim;
    	align_level_dim_t  align;
    	unsigned int  in_buf_addr_align_level;
    	unsigned int  out_bs_addr_align_level;
		unsigned int  osg_win_max_num;
    	unsigned int  osg_pal_max_num;
		unsigned int  osg_width_align;
		unsigned int  osg_height_align;
		unsigned int  osg_x_align;
		unsigned int  osg_y_align;
		unsigned int  osg_line_offset_align;
		unsigned int  osg_support_mask;
    } jpeg;


    struct {
    	unsigned int  frame_type_support_bit_array[(MAX_VPD_BUFTYPE_COUNT+31)/32];
    	unsigned int  pattern_type_support_bit_array[(MAX_VPD_BUFTYPE_COUNT+31)/32];
    } osg;

} KFLOW_VIDEOENC_HW_SPEC_INFO;


typedef struct {
    UINT32                      fd;
	VG_VDOENC_PARAM_ID          param_id;
	union {
		CHAR                    param_union;
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
	};

} KFLOW_VIDEOENC_H26X_PARAM;

#define KFLOW_VENC_MAGIC        'e'

#define KFLOW_VIDEOENC_IOC_FD_OPEN          _VOS_IOWR(KFLOW_VENC_MAGIC, 100, KFLOW_VIDEOENC_FD_INFO)
#define KFLOW_VIDEOENC_IOC_FD_CLOSE         _VOS_IOWR(KFLOW_VENC_MAGIC, 101, KFLOW_VIDEOENC_FD_INFO)
#define KFLOW_VIDEOENC_IOC_GET_HW_SPEC_INFO _VOS_IOWR(KFLOW_VENC_MAGIC, 102, KFLOW_VIDEOENC_HW_SPEC_INFO)
#define KFLOW_VIDEOENC_IOC_GET_H26X_PARAM   _VOS_IOWR(KFLOW_VENC_MAGIC, 104, KFLOW_VIDEOENC_H26X_PARAM)

#define H26X_ENC_IOC_SET_TMNR_ENABLE        _VOS_IOW(KFLOW_VENC_MAGIC,  110, FD_H26XENC_EN_INFO)
#define H26X_ENC_IOC_GET_TMNR_ENABLE        _VOS_IOWR(KFLOW_VENC_MAGIC, 110, FD_H26XENC_EN_INFO)
#define H26X_ENC_IOC_SET_TMNR_PARAM         _VOS_IOW(KFLOW_VENC_MAGIC,  111, FD_H26XENC_TMNR_PARAM)
#define H26X_ENC_IOC_GET_TMNR_PARAM         _VOS_IOWR(KFLOW_VENC_MAGIC, 111, FD_H26XENC_TMNR_PARAM)
#define H26X_ENC_IOC_SET_MASK_WIN           _VOS_IOW(KFLOW_VENC_MAGIC,  120, FD_H26XENC_MASK_WIN)
#define H26X_ENC_IOC_GET_MASK_WIN           _VOS_IOWR(KFLOW_VENC_MAGIC, 120, FD_H26XENC_MASK_WIN)
#define H26X_ENC_IOC_SET_MASK_PAL           _VOS_IOW(KFLOW_VENC_MAGIC,  121, FD_H26XENC_MASK_PALETTE_TABLE)
#define H26X_ENC_IOC_GET_MASK_PAL           _VOS_IOWR(KFLOW_VENC_MAGIC, 121, FD_H26XENC_MASK_PALETTE_TABLE)
#define H26X_ENC_IOC_SET_MASK_MOSAIC_SIZE   _VOS_IOW(KFLOW_VENC_MAGIC,  122, FD_H26XENC_MASK_MOSAIC_BLOCK)
#define H26X_ENC_IOC_GET_MASK_MOSAIC_SIZE   _VOS_IOWR(KFLOW_VENC_MAGIC, 122, FD_H26XENC_MASK_MOSAIC_BLOCK)
#define H26X_ENC_IOC_SET_OSG_RGB2YUV        _VOS_IOW(KFLOW_VENC_MAGIC,  130, FD_H26XENC_OSG_RGB2YUV)
#define H26X_ENC_IOC_GET_OSG_RGB2YUV        _VOS_IOWR(KFLOW_VENC_MAGIC, 130, FD_H26XENC_OSG_RGB2YUV)
#define H26X_ENC_IOC_SET_OSG_WIN            _VOS_IOW(KFLOW_VENC_MAGIC,  131, KFLOW_VIDEOENC_OSG_WIN)
#define H26X_ENC_IOC_GET_OSG_WIN            _VOS_IOWR(KFLOW_VENC_MAGIC, 131, KFLOW_VIDEOENC_OSG_WIN)
#define H26X_ENC_IOC_SET_BUFFER_INFO        _VOS_IOW(KFLOW_VENC_MAGIC,  140, H26XENC_REF_BUFFER_INFO)
#define H26X_ENC_IOC_GET_BUFFER_INFO        _VOS_IOWR(KFLOW_VENC_MAGIC, 140, H26XENC_REF_BUFFER_INFO)
#define H26X_ENC_IOC_RELEASE_BUFFER         _VOS_IOW(KFLOW_VENC_MAGIC,  141, H26XENC_REF_BUFFER_INFO)

#define JPEG_ENC_IOC_SET_OSG_WIN            _VOS_IOW(KFLOW_VENC_MAGIC,  150, KFLOW_VIDEOENC_OSG_WIN)
#define JPEG_ENC_IOC_GET_OSG_WIN            _VOS_IOWR(KFLOW_VENC_MAGIC, 151, KFLOW_VIDEOENC_OSG_WIN)

#endif
