#ifndef __LCD200_FB_H__
#define __LCD200_FB_H__

#include <lcd200_v3/lcd200_api.h>

#define LCD200_IOC_MAGIC  'h'

#define LCD200_IOC_GET_EDID        		_IOR(LCD200_IOC_MAGIC, 1, struct lcd200_edid)
/* lcd200_drv_otype_t */
#define LCD200_IOC_SET_EDID        		_IOW(LCD200_IOC_MAGIC, 2, unsigned int)

/* Get frame buffer physical address */
#define LCD200_IOC_GET_FBPADDR     		_IOR(LCD200_IOC_MAGIC, 3, unsigned int)   	/* get frame buffer physical address */

struct lcd200_indim_otype {
	lcd200_drv_indim_t	indim;
	lcd200_drv_otype_t	otype;
};
#define LCD200_IOC_GET_INDIM_OTYPE 		_IOR(LCD200_IOC_MAGIC, 4, struct lcd200_indim_otype)
#define LCD200_IOC_SET_INDIM_OTYPE 		_IOW(LCD200_IOC_MAGIC, 5, struct lcd200_indim_otype)

/* hardware cursor position */
#define LCD200_IOC_SET_IOCURSOR			_IOW(LCD200_IOC_MAGIC, 7, unsigned int)

/* color key */
#define LCD200_IOC_SET_COLOR_KEY 		_IOW(LCD200_IOC_MAGIC, 12, struct lcd200_ckey)

/* input format, enum lcd200_infmt */
#define LCD200_IOC_GET_INFMT			_IOR(LCD200_IOC_MAGIC, 14, unsigned int)	/* enum lcd200_infmt */
#define LCD200_IOC_SET_INFMT			_IOW(LCD200_IOC_MAGIC, 15, unsigned int)	/* enum lcd200_infmt */

/* 0 for only plane0, 1:plane0/1, 2:plane0/1/2 */
#define LCD200_IOC_GET_PLANE_STATE		_IOR(LCD200_IOC_MAGIC, 20, int)
#define LCD200_IOC_SET_PLANE_STATE		_IOW(LCD200_IOC_MAGIC, 21, int)

/* layer mapping */
struct lcd200_layer_mapping {
	int	layerid[3];	/* index is plane */
};
#define LCD200_IOC_GET_LAYER_MAPPING	_IOR(LCD200_IOC_MAGIC, 24, struct lcd200_layer_mapping)
#define LCD200_IOC_SET_LAYER_MAPPING	_IOW(LCD200_IOC_MAGIC, 25, struct lcd200_layer_mapping)

/* blending */
struct lcd200_blending {
	int	blend_h;	/* high, 0 - 255 */
	int	blend_l;	/* low, 0 - 255 */
};
#define LCD200_IOC_GET_BLENDING			_IOR(LCD200_IOC_MAGIC, 28, struct lcd200_blending)
#define LCD200_IOC_SET_BLENDING			_IOW(LCD200_IOC_MAGIC, 29, struct lcd200_blending)
/* palette */
#define LCD200_IOC_SET_PALETTE			_IOW(LCD200_IOC_MAGIC, 30, struct lcd200_palette)
/* system info */
#define LCD200_IOC_GET_INFO				_IOR(LCD200_IOC_MAGIC, 31, struct lcd200_sysinfo)
/* used to select which input source to HDMI/VGA/EXTERNAL .... */
#define LCD200_IOC_SET_DISPDEV			_IOW(LCD200_IOC_MAGIC, 32, int) /* enum lcd200_dispdev */
/* used to on/off lcd200 */
#define LCD200_IOC_SET_CHIPSTATE		_IOW(LCD200_IOC_MAGIC, 33, int)	/* 1 for enabled, 0 for disabled */
/* module parameter */
#define LCD200_IOC_SET_MP_INIT			_IOW(LCD200_IOC_MAGIC, 35, struct lcd200_mp)
#define LCD200_IOC_SET_MP_UNINIT		_IOW(LCD200_IOC_MAGIC, 36, int)
/* module parameter */
#define LCD200_IOC_GET_MP_INIT_DONE     _IOR(LCD200_IOC_MAGIC, 37, int)
#define LCD200_IOC_GET_MP				_IOR(LCD200_IOC_MAGIC, 38, int)
/* brightness... iq */
#define LCD200_IOC_GET_IQ				_IOR(LCD200_IOC_MAGIC, 40, struct lcd200_iq)
#define LCD200_IOC_SET_IQ				_IOW(LCD200_IOC_MAGIC, 41, struct lcd200_iq)


struct lcd200_wbaddr_ex {
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
#define LCD200_IOC_SET_WBADDR_EX      		_IOW(LCD200_IOC_MAGIC, 51, struct lcd200_wbaddr_ex)
#endif /* __LCD200_FB_H__ */