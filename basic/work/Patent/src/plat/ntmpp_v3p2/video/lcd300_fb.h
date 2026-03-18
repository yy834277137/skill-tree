#ifndef __LCD300_FB_H__
#define __LCD300_FB_H__

#include "lcd300_api.h"

#define LCD300_IOC_MAGIC  'g'

#define LCD300_IOC_GET_EDID        		_IOR(LCD300_IOC_MAGIC, 1, struct lcd300_edid)
/* lcd300_drv_otype_t */
#define LCD300_IOC_SET_EDID        		_IOW(LCD300_IOC_MAGIC, 2, unsigned int)

/* Get frame buffer physical address */
#define LCD300_IOC_GET_FBPADDR     		_IOR(LCD300_IOC_MAGIC, 3, unsigned int)   	/* get frame buffer physical address */

struct lcd300_indim_otype {
	lcd300_drv_indim_t	indim;
	lcd300_drv_otype_t	otype;
};
#define LCD300_IOC_GET_INDIM_OTYPE 		_IOR(LCD300_IOC_MAGIC, 4, struct lcd300_indim_otype)
#define LCD300_IOC_SET_INDIM_OTYPE 		_IOW(LCD300_IOC_MAGIC, 5, struct lcd300_indim_otype)

/* hardware cursor position */
#define LCD300_IOC_SET_IOCURSOR			_IOW(LCD300_IOC_MAGIC, 7, unsigned int)
/* GUI */
#define LCD300_IOC_RLE_FORCE_REFRESH   	_IOW(LCD300_IOC_MAGIC, 8, unsigned int)   	/* refresh the gui immediately */
#define LCD300_IOC_RLE_SET_INTVAL      	_IOW(LCD300_IOC_MAGIC, 9, unsigned int)   	/* set refresh interval */
#define LCD300_IOC_RLE_GET_INTVAL      	_IOR(LCD300_IOC_MAGIC, 10, unsigned int)   	/* get refresh interval */
/* color key */
#define LCD300_IOC_SET_COLOR_KEY 		_IOW(LCD300_IOC_MAGIC, 12, struct lcd300_ckey)

/* input format, enum lcd300_infmt */
#define LCD300_IOC_GET_INFMT			_IOR(LCD300_IOC_MAGIC, 14, unsigned int)	/* enum lcd300_infmt */
#define LCD300_IOC_SET_INFMT			_IOW(LCD300_IOC_MAGIC, 15, unsigned int)	/* enum lcd300_infmt */

/* alpha value for argb1555 */
struct lcd300_alpha_argb1555 {
	int	alpha0_value;
	int alpha1_value;
};
#define LCD300_IOC_GET_ALPHA_ARGB1555	_IOR(LCD300_IOC_MAGIC, 16, struct lcd300_alpha_argb1555)
#define LCD300_IOC_SET_ALPHA_ARGB1555	_IOR(LCD300_IOC_MAGIC, 17, struct lcd300_alpha_argb1555)

/* twin monitor */
#define LCD300_IOC_SET_TWIN_MONITOR		_IOW(LCD300_IOC_MAGIC, 18, struct lcd300_twin_monitor)

/* plane state enabled/disabled */
struct lcd300_plane_state {
	int	plane_state[3];	/* 0 for disabled, 1 for enabled */
};
#define LCD300_IOC_GET_PLANE_STATE		_IOR(LCD300_IOC_MAGIC, 20, struct lcd300_plane_state)
#define LCD300_IOC_SET_PLANE_STATE		_IOW(LCD300_IOC_MAGIC, 21, struct lcd300_plane_state)

/* writeback buffer. Only YUV422 is supported. */
struct lcd300_write_back {
	int	width;
	int	height;
	int oneshut; /* 0: free-run, 1: one-shut */
	int sel;     /* 0: ch0~ch2+cursor, 1: ch0~ch2, 2: ch0 */
	int fmt;     /* 0: 422, 1: 420, 2: 420SCE */
	int	state;	/* 0:disabled, 1:enabled */
	uintptr_t paddr;
};
#define LCD300_IOC_SET_WRITEBACK		_IOW(LCD300_IOC_MAGIC, 22, struct lcd300_write_back)

/* layer mapping */
struct lcd300_layer_mapping {
	int	layerid[3];	/* index is plane */
};
#define LCD300_IOC_GET_LAYER_MAPPING	_IOR(LCD300_IOC_MAGIC, 24, struct lcd300_layer_mapping)
#define LCD300_IOC_SET_LAYER_MAPPING	_IOW(LCD300_IOC_MAGIC, 25, struct lcd300_layer_mapping)

/* blending */
struct lcd300_blending {
	int	blend[3];	/* 0 - 255 */
};
#define LCD300_IOC_GET_BLENDING			_IOR(LCD300_IOC_MAGIC, 28, struct lcd300_blending)
#define LCD300_IOC_SET_BLENDING			_IOW(LCD300_IOC_MAGIC, 29, struct lcd300_blending)
/* palette */
#define LCD300_IOC_SET_PALETTE			_IOW(LCD300_IOC_MAGIC, 30, struct lcd300_palette)
/* system info */
#define LCD300_IOC_GET_INFO				_IOR(LCD300_IOC_MAGIC, 31, struct lcd300_sysinfo)
/* used to select which input source to HDMI/VGA/EXTERNAL .... */
#define LCD300_IOC_SET_DISPDEV			_IOW(LCD300_IOC_MAGIC, 32, int) /* enum lcd300_dispdev */
/* module parameter */
#define LCD300_IOC_SET_MP_INIT			_IOW(LCD300_IOC_MAGIC, 35, struct lcd300_mp)
#define LCD300_IOC_SET_MP_UNINIT		_IOW(LCD300_IOC_MAGIC, 36, int)
/* module parameter */
#define LCD300_IOC_GET_MP				_IOR(LCD300_IOC_MAGIC, 37, struct lcd300_mp)

/* module parameter */
#define LCD300_IOC_GET_MP_INIT_DONE     _IOR(LCD300_IOC_MAGIC, 38, int)

struct lcd300_chan_scale {
	int	plane_idx;	/* 0 ~ 2 */
	int	src_width;
	int	src_height;
	int	dst_width;
	int	dst_height;
	int	desk_x;		/* x pixel poistion to desk_res */
	int	desk_y;		/* y lines poistion to desk_res */
};

#define LCD300_IOC_GET_CHSCALE			_IOWR(LCD300_IOC_MAGIC, 38, struct lcd300_chan_scale)
#define LCD300_IOC_SET_CHSCALE			_IOW(LCD300_IOC_MAGIC, 39, struct lcd300_chan_scale)

/* brightness... iq */
#define LCD300_IOC_GET_IQ				_IOR(LCD300_IOC_MAGIC, 40, struct lcd300_iq)
#define LCD300_IOC_SET_IQ				_IOW(LCD300_IOC_MAGIC, 41, struct lcd300_iq)

/* FHD path write back */
struct lcd300_fhd_wb {
  	int enable;
  	int wb_lx; /* write-back x start pixel (two-pixel alignment), Left x, for SDO1 */
    int wb_ty; /* write-back y start line, Top Y, for SDO1, range: 0 ~ 12 */
    int wb_rx; /* write-back x start pixel (two-pixel alignment), Right x, for SDO1 */
    int wb_by; /* write-back y start line, Bottom Y, for SDO1, range: 0 ~ 12 */
    int wb_width; /* outer size, the inner size is SDO1 destination size */
    int wb_height; /* outer size, the inner size is SDO1 destination size */
    uintptr_t wb_ba; /* physical base address */
};


#define MAX_EDID_NU 256
typedef struct edid_info{
	unsigned int is_valid[MAX_EDID_NU];
    unsigned int w[MAX_EDID_NU];
    unsigned int h[MAX_EDID_NU];
	unsigned char refresh_rate[MAX_EDID_NU];
	unsigned char is_progress[MAX_EDID_NU];
	unsigned int aspect_rate[MAX_EDID_NU];
	unsigned int	bEdidValid;
    unsigned int u32Edidlength;
    unsigned char u8Edid[512];
} edid_info_t;

#define LCD300_IOC_GET_FHD_WB 	_IOR(LCD300_IOC_MAGIC, 42, struct lcd300_fhd_wb)
#define LCD300_IOC_SET_FHD_WB 	_IOW(LCD300_IOC_MAGIC, 43, struct lcd300_fhd_wb)

struct lcd300_hue_sat {
	int hue_sat_enable;	/* 0 for disabled, 1 for enabled */
	unsigned int hue_sat[6];	/* 0 ~ 255 */
};
#define LCD300_IOC_GET_HUESAT 	_IOR(LCD300_IOC_MAGIC, 44, struct lcd300_hue_sat)
#define LCD300_IOC_SET_HUESAT 	_IOW(LCD300_IOC_MAGIC, 45, struct lcd300_hue_sat)

struct lcd300_ygamma {
	int	gamma_state;	/* 0 for disabled, 1 for enabled */
	unsigned int index[16];	/* consists of (index >> 8) & 0x3 | (index & 0x1F) */
	unsigned int y_gm[32];	/* 0 ~ 4095 */
};
#define LCD300_IOC_GET_YGAMMA 	_IOR(LCD300_IOC_MAGIC, 46, struct lcd300_ygamma)
#define LCD300_IOC_SET_YGAMMA 	_IOW(LCD300_IOC_MAGIC, 47, struct lcd300_ygamma)

/* set VGA DAC power state. 1 for enable, 0 for disabled.
 */
#define LCD300_IOC_SET_VGADAC_STATE		_IOW(LCD300_IOC_MAGIC, 48, int)

/* show device edid capability
*/
#define LCD300_IOC_GET_DEVICE_CAPABILITY        		_IOR(LCD300_IOC_MAGIC, 49, struct edid_info)

struct lcd300_wbaddr_ex {
	int enable;
  	int wb_lx; /* write-back x start pixel (two-pixel alignment), Left x, for SDO1 */
    int wb_ty; /* write-back y start line, Top Y, for SDO1, range: 0 ~ 12 */
    int wb_rx; /* write-back x start pixel (two-pixel alignment), Right x, for SDO1 */
    int wb_by; /* write-back y start line, Bottom Y, for SDO1, range: 0 ~ 12 */
    int wb_width; /* outer size, the inner size is SDO1 destination size */
    int wb_height; /* outer size, the inner size is SDO1 destination size */
	uintptr_t wb_ba[5];	/* physical base address, max is 5 piece of buffers */
	unsigned int piece_buffer_sz;
	unsigned int num_of_piece;
	uintptr_t share_mem_ba;	/* must be 64 bytes alignment */
	unsigned int share_mem_sz;	/* must be 128 bytes alignment */
};

#define LCD300_IOC_SET_WBADDR_EX      		_IOW(LCD300_IOC_MAGIC, 50, struct lcd300_wbaddr_ex)
#define LCD300_IOC_GET_WBADDR_EX      		_IOR(LCD300_IOC_MAGIC, 51, struct lcd300_wbaddr_ex)


/* writeback push/pull
*/
#define LCD300_IOC_PUSH_WB              _IOW(LCD300_IOC_MAGIC, 52, struct lcd300_write_back)
#define LCD300_IOC_PULL_WB              _IOW(LCD300_IOC_MAGIC, 53, struct lcd300_write_back)

#endif /* __LCD300_FB_H__ */