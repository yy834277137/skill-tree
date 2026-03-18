#ifndef __LCD300_API_H__
#define __LCD300_API_H__

/* desk_res */
typedef enum lcd300_indim {
	LCD300_INDIM_NTSC = 0,
	LCD300_INDIM_PAL,
	LCD300_INDIM_640x480,
	LCD300_INDIM_1024x768,
	LCD300_INDIM_1280x1024,
	LCD300_INDIM_720P,
	LCD300_INDIM_1600x1200,
	LCD300_INDIM_2560x720,
	LCD300_INDIM_1920x1080,
	LCD300_INDIM_2560x1440,
	LCD300_INDIM_3840x1080,
	LCD300_INDIM_3840x2160,
	LCD300_INDIM_1440x900,
	LCD300_INDIM_1680x1050,
	LCD300_INDIM_1920x1200,
	LCD300_INDIM_800x600,
	LCD300_INDIM_64x64,
	LCD300_INDIM_MAX,
	LCD300_INDIM_DUMMY = 0x10000000,
} lcd300_drv_indim_t;

typedef enum lcd300_otype {
	LCD300_OTYPE_CVBS = 0,
	LCD300_OTYPE_CVBS_NTSC = LCD300_OTYPE_CVBS,
	LCD300_OTYPE_CVBS_PAL,
	LCD300_OTYPE_VGA_720x480x60,
	LCD300_OTYPE_VGA_800x600x60,
	LCD300_OTYPE_VGA_1024x768x60,
	LCD300_OTYPE_VGA_1280x720x60,
	LCD300_OTYPE_VGA_1280x1024x60,
	LCD300_OTYPE_VGA_1600x1200x60,
	LCD300_OTYPE_VGA_1920x1080x60,
	LCD300_OTYPE_HDMI_720x480x60,
	LCD300_OTYPE_HDMI_720x576x50, //10
	LCD300_OTYPE_HDMI_1024x768x60,
	LCD300_OTYPE_HDMI_1280x720x60,
	LCD300_OTYPE_HDMI_1280x1024x60,
	LCD300_OTYPE_HDMI_1600x1200x60,
	LCD300_OTYPE_HDMI_1920x1080x60,
	LCD300_OTYPE_HDMI_2560x1440x30,	/* 119Mhz */
	LCD300_OTYPE_HDMI_3840x2160x30,
	LCD300_OTYPE_HDMI_2560x1440x60, /* 241.5Mhz */
	LCD300_OTYPE_VGA_1440x900x60,
	LCD300_OTYPE_VGA_1680x1050x60,//20
	LCD300_OTYPE_VGA_1920x1200x60,
	LCD300_OTYPE_HDMI_1440x900x60,
	LCD300_OTYPE_HDMI_1680x1050x60,
	LCD300_OTYPE_HDMI_1920x1200x60,
	LCD300_OTYPE_1080P,
	LCD300_OTYPE_HDMI_1920x1080x50,
	LCD300_OTYPE_HDMI_3840x2160x25,
	LCD300_OTYPE_HDMI_1280x720x50,
	LCD300_OTYPE_HDMI_1920x1080x24,
	LCD300_OTYPE_HDMI_1920x1080x25,//30
	LCD300_OTYPE_HDMI_1920x1080x30,
	LCD300_OTYPE_HDMI_800x600x60,
	LCD300_OTYPE_BT656_1280x720x30,
	LCD300_OTYPE_BT656_1280x720x25,
	LCD300_OTYPE_BT1120_1280x720x60,
	LCD300_OTYPE_BT1120_1280x720x25,
	LCD300_OTYPE_BT1120_1280x720x30,
	LCD300_OTYPE_BT1120_1920x1080x60,
	LCD300_OTYPE_BT1120_1024x768x60,
	LCD300_OTYPE_BT1120_1280x1024x60,//40
	LCD300_OTYPE_HDMI_3840x2160x60,
	LCD300_OTYPE_64x64,
	LCD300_OTYPE_MAX,
	LCD300_OTYPE_DUMMY = 0x10000000,
} lcd300_drv_otype_t;

/*
 * @This function is used to both desk_res and output_type
 *
 * @function lcd300_drv_set_deskres_otype(int chip_idx, lcd300_drv_indim_t in_dim, lcd300_drv_otype_t otype);
 * @param: chip_idx.
 *		   in_dim: input dim
 *		   otype: output_type.
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_deskres_otype(int chip_idx, lcd300_drv_indim_t in_dim, lcd300_drv_otype_t otype);
int lcd300_drv_get_deskres_otype(int chip_idx, lcd300_drv_indim_t *in_dim, lcd300_drv_otype_t *otype);

/*
 * @This function is used to set desk resoultion
 *
 * @function lcd300_drv_set_deskres(int chip_idx, lcd300_indim_t in_dim);
 * @param: chip_idx.
 *		   in_dim: input dim
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_deskres(int chip_idx, lcd300_drv_indim_t in_dim);
int lcd300_drv_get_deskres(int chip_idx, lcd300_drv_indim_t *in_dim);

/*
 * @This function is used to set output type
 *
 * @function lcd300_drv_set_otype(int chip_idx, lcd300_indim_t in_dim);
 * @param: chip_idx.
 *		   otype: output type
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_otype(int chip_idx, lcd300_drv_otype_t otype);
int lcd300_drv_get_otype(int chip_idx, lcd300_drv_otype_t *otype);

/*
 * @This function is used to set write back.
 *
 * @function lcd300_drv_set_writeback(int chip_idx, int width, int height);
 * @param: chip_idx.
 *		   width/height in pixel. width or heigh must be 16 pixel alignment.
 *         oneshut one shut mode or freerun.
 *         sel select 0: ch0~ch2+cursor, 1: ch0~ch2, 2: ch0 only.
 *         fmt select 0: 422, 1: 420, 2: 420SCE.
 *		   state = 0 means disabled, 1 for enabled.
 *		   wb_paddr: the address the LCD310 would like to output image
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_writeback(int chip_idx, int width, int height, int oneshut, int sel, int fmt, int state, uintptr_t wb_paddr);
int lcd300_drv_get_writeback(int chip_idx, int *width, int *height, int *oneshut, int *sel, int *fmt, int *state);

typedef enum lcd300_infmt {
	LCD300_INFMT_YUV422 = 0,
	LCD300_INFMT_YUV422_DPCM,	/* deprecated */
	LCD300_INFMT_YUV422_SCE,
	LCD300_INFMT_YUV420,
	LCD300_INFMT_YUV420_DPCM,	/* deprecated */
	LCD300_INFMT_YUV420_SCE,
	LCD300_INFMT_ARGB8888,
	LCD300_INFMT_ARGB8888_RLE,
	LCD300_INFMT_RGB888,
	LCD300_INFMT_RGB888_RLE,
	LCD300_INFMT_RGB565,
	LCD300_INFMT_RGB565_RLE,
	LCD300_INFMT_ARGB1555,
	LCD300_INFMT_ARGB1555_RLE,
	LCD300_INFMT_ARGB5551,
	LCD300_INFMT_ARGB5551_RLE,
	LCD300_INFMT_RGB1BPP,
	LCD300_INFMT_RGB2BPP,
	LCD300_INFMT_RGB4BPP,
	LCD300_INFMT_RGB8BPP,
	LCD300_INFMT_MAX,
	LCD300_INFMT_DUMMY = 0x10000000,
} lcd300_drv_infmt_t;

/*
 * @This function is used to set input format
 *
 * @function lcd300_drv_set_infmt(int chip_idx, int plane, lcd300_infmt_t infmt);
 * @param: chip_idx.
 *		   plane: which plane id
 *		   lcd300_infmt_t: input format
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_infmt(int chip_idx, int plane, lcd300_drv_infmt_t infmt);
int lcd300_drv_get_infmt(int chip_idx, int plane, lcd300_drv_infmt_t *infmt);

/*
 * @This function is used to get plane buffer address which allocates from driver itself.
 *
 * @function lcd300_drv_get_plane_paddr(int chip_idx, int plane, unsigned int *paddr, unsigned int *vaddr);
 * @param: chip_idx.
 *		   plane: 0~2
 *		   paddr: physical address
 * @return 0 on success, <0 on error
 */
int lcd300_drv_get_plane_paddr(int chip_idx, int plane, uintptr_t *paddr, unsigned int *vaddr);

/* color key for LCD300 */
typedef struct lcd300_ckey {
    int fb_idx; /* INDEX */
    int enable; /* 0 for disabled, 1 for enabled */
    int ckey_r; /* 8bits R */
    int ckey_g; /* 8bits G */
    int ckey_b; /* 8bits B */
} lcd300_drv_ckey_t;

/*
 * @This function is used to set color key of LCD310
 *
 * @function lcd300_drv_set_ckey(int chip_idx, lcd300_drv_ckey_t *ckey);
 * @param: ckey. all parameters are necessary.
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_ckey(int chip_idx, lcd300_drv_ckey_t *ckey);

/*
 * @This function is used to configure every channel status
 *
 * @function lcd300_drv_set_chanstate(int chip_idx, int chan0_state, int chan1_state, int chan2_state);
 * @param: chan0_state, chan1_state, int chan2_state. 0 for disabled, 1 for enabled.
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_chanstate(int chip_idx, int chan0_state, int chan1_state, int chan2_state);
int lcd300_drv_get_chanstate(int chip_idx, int *chan0_state, int *chan1_state, int *chan2_state);

/*
 * @This function is used to layer mapping for individual planes. The index is plane id.
 *
 * @function lcd300_drv_set_layermapping(int chip_idx, int layer0_id, int layer1_id, int layer2_id);
 * @param: layer0_id, layer1_id, layer2_id. The value is 0/1/2
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_layermapping(int chip_idx, int layer0_id, int layer1_id, int layer2_id);
int lcd300_drv_get_layermapping(int chip_idx, int *layer0_id, int *layer1_id, int *layer2_id);

/*
 * @This function is used to set blending for individual channels. The index is channel id.
 *
 * @function lcd300_drv_set_blend(int chip_idx, int ch0_blend, int ch1_blend, int ch2_blend);
 * @param: ch0_blend, ch1_blend, int ch2_blend, range 0-255
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_blend(int chip_idx, int ch0_blend, int ch1_blend, int ch2_blend);
int lcd300_drv_get_blend(int chip_idx, int *ch0_blend, int *ch1_blend, int *ch2_blend);

/*
 * @This function is used to set 0/1 alpha value for individual plane. The index is plane id.
 *
 * @function lcd300_drv_set_argb1555(int chip_idx, int chan, int alpha0_val, int alpha1_val);
 * @param: chan: channel id
 * @param: alpha0_val/alpha1_val: alpha value 0-255
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_argb1555(int chip_idx, int plane, int alpha0_val, int alpha1_val);
int lcd300_drv_get_argb1555(int chip_idx, int plane, int *alpha0_val, int *alpha1_val);

typedef struct lcd300_palette {
	/* pal_idx
	 * LCD310: 0 ~ 255, default entry content is white,yellow,cyan,green,magenta,red,blue,black
	 */
    int  index;
    int  rgb565;	/* RGB565, R[15:11], G[10:5], B[4:0] */
} lcd300_drv_palette_t;

/*
 * @This function is used to set palette ram.
 *
 * @function lcd300_drv_set_palette(int chip_idx, lcd300_drv_palette_t *palette);
 * @param: index: palette idx. max value is 255
 * @param: rgb565: 16bits for rgb
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_palette(int chip_idx, lcd300_drv_palette_t *palette);

typedef struct lcd300_sysinfo {
	int	fps;		///< hardware fps
	int	idim_xres;	///< input dim xres
	int idim_yres;	///< input dim yres
	int odim_xres;	///< output dim xres
	int odim_yres;	///< output dim xres
} lcd300_drv_sysinfo_t;

/*
 * @This function is used to get sysinfo of LCD310
 *
 * @function int lcd300_drv_get_sysinfo(lcd300_drv_sysinfo_t *sysinfo);
 * @param: output to sysinfo
 * @return 0 on success, <0 on error
 */
int lcd300_drv_get_sysinfo(int chip_idx, lcd300_drv_sysinfo_t *sysinfo);

typedef enum lcd300_dispdev {
	/* HDMI */
	HDMI_LCD300_RGB0 = 0x1,	/* LCD300 has two rgb source */
	HDMI_LCD300_RGB1 = 0x2,	/* dual screen use */

	/* VGA */
	VGA_LCD300_RGB0 = 0x10,
	VGA_LCD300_RGB1 = 0x20,
	/* External pinmux0 */
	EXT0_LCD300_RGB0 = 0x100,
	EXT0_LCD300_RGB1 = 0x200,	/* dual screen use */
	EXT0_LCD300_BT1120 = 0x400,
	/* External pinmux1 */
	EXT1_LCD300_RGB0 = 0x1000,
	EXT1_LCD300_RGB1 = 0x2000,	/* dual screen use */
	EXT1_LCD300_BT1120 = 0x4000,
	DISPDEV_DUMMY = 0x10000000,
} lcd300_drv_dispdev_t;

#define HDMI_LCD300_MASK	(HDMI_LCD300_RGB0 | HDMI_LCD300_RGB1)
#define	VGA_LCD300_MASK		(VGA_LCD300_RGB0 | VGA_LCD300_RGB1)
#define EXT0_LCD300_MASK	(EXT0_LCD300_RGB0 | EXT0_LCD300_RGB1 | EXT0_LCD300_BT1120)
#define EXT1_LCD300_MASK	(EXT1_LCD300_RGB0 | EXT1_LCD300_RGB1 | EXT1_LCD300_BT1120)

/*
 * @This function is used to set the input source to HDMI/VGA/EXTERNAL
 *
 * @function int lcd300_drv_set_dispdev(int chip_idx, lcd300_dispdev_t vout);
 * @param: chip_idx, dispdev
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_dispdev(int chip_idx, unsigned int dispdev_bmap); /* lcd300_drv_dispdev_t */

/*
 * @This function is used to set VGA DAC power state. 0 for power-down, 1 for power-up.
 *
 * @function int lcd300_drv_set_vgadac_state(int chip_idx, int dac_state);
 * @param: chip_idx, dac_state
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_vgadac_state(int chip_idx, int dac_state);

/*
 * @This function is used to set source frame rate
 *
 * @function int lcd300_drv_set_srcfps(int chip_idx, int fps);
 * @param: chip_idx, fps (frame per second)
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_srcfps(int chip_idx, int fps);

typedef struct lcd300_twin_monitor {
	int	enable;			///< 0:disabled, 1:twin display output
	int bt1120_mirror_left_right;	///< When Twin minitor function is enabled, BT1120 interface can mirror Left side or Right side */
} lcd300_drv_twin_monitor_t;
/*
 * @This function is used to set twin monitor. When users use twin monitor, the desk width must be two multiple of output resolution.
 *	For example: the output resolution is 1920x1080, then desk resolution must be 3840x1080. And LCD300 outputs from both
 *				 RGB0 and RGB1 path. Besides, BT1120 interface also can output image. I can choose RGB0 or 1 as the mirror source.
 * @function int lcd300_drv_set_twin_monitor(int chip_idx, lcd300_drv_twin_monitor_t twin_monitor);
 * @param: chip_idx, twin_monitor
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_twin_monitor(int chip_idx, lcd300_drv_twin_monitor_t *twin_monitor);

#define LCD300_EDID_TABLE_MAX	20

typedef struct lcd300_edid {
	lcd300_drv_otype_t	otype[LCD300_EDID_TABLE_MAX];
	int valid_num;
} lcd300_drv_edid_t;

/*
 * @This function is used to get edid table
 *
 * @function int lcd300_drv_get_edid(int chip_idx, lcd300_drv_edid_t *edid_table);
 * @param: chip_idx, edid_table
 * @return 0 on success, <0 on error
 */
int lcd300_drv_get_edid(int chip_idx, lcd300_drv_edid_t *edid_table);

typedef struct lcd300_iq {
	int	brightness;		/* -127~127 */
	struct {
		int	state;		/* 0 for disabled, 1 for enabled */
		int	strength;	/* strength 0~1023 */
	} ce;	/* contrast enhancement */
	struct {
		int	state;
		int	hpf0_5x5_gain;	/* 0~1023 */
		int hpf1_5x5_gain; 	/* 0~1023 */
	} sharpness;
	struct {
		unsigned int contrast;	/* 0-255 */
		unsigned int contrast_mode;	/* 0: standard, 1:shadow 2:highlight */
	} contrast;
	struct {
		unsigned int hue_sat_enable;
		unsigned int saturation[6];
    	int hue[6];
	} hue_sat;
} lcd300_drv_iq_t;

/*
 * @This function is used to get/set iq table
 *
 * @function int lcd300_drv_get_iq(int chip_idx, lcd300_drv_iq_t *iq);
 * @param: chip_idx, iq
 * @return 0 on success, <0 on error
 */
int lcd300_drv_get_iq(int chip_idx, lcd300_drv_iq_t *iq);
int lcd300_drv_set_iq(int chip_idx, lcd300_drv_iq_t *iq);

typedef struct lcd300_mp {
	enum lcd300_indim	indim;
	enum lcd300_otype	otype;
	struct {
		int	max_w;	/* 16 alignment */
		int	max_h;
		int max_bpp;
		int gui_rld_enable;
		int	gui_pan_display;	/* not supported yet */
		uintptr_t buf_paddr;
		unsigned int buf_len;
		uintptr_t rle_buf_paddr;	/* only valid for gui compress */
		unsigned int rle_buf_len;	/* only valid for gui compress */
		int	rle_buf_ratio;
		/* NEW */
		int	src_width;	/* 16 alignment */
		int	src_height;	/* 16 alignment */
		int ddrid;
		int	reserved[2];
	} plane[3];
	int	reserved[5];
} lcd300_drv_mp_t;	/* module parameters */

/*
 * @This function is used to int moudule parameter
 *
 * @function int lcd300_drv_set_mp_init(int chip_idx, lcd300_drv_mp_t *mp, int mp_len);
 * @param: chip_idx, mp, mp_len
 * @return 0 on success, <0 on error
 */
int lcd300_drv_set_mp_init(int chip_idx, lcd300_drv_mp_t *mp, int mp_len);
int lcd300_drv_set_mp_uninit(int chip_idx);

#endif /* __LCD300_API_H__ */
