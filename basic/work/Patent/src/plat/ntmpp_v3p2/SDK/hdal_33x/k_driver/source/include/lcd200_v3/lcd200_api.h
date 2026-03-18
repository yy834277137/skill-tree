#ifndef __LCD200_API_H__
#define __LCD200_API_H__

/* desk_res */
typedef enum lcd200_indim {
	LCD200_INDIM_NTSC = 0,
	LCD200_INDIM_PAL,
	LCD200_INDIM_640x480,
	LCD200_INDIM_1024x768,
	LCD200_INDIM_1280x1024,
	LCD200_INDIM_720P,
	LCD200_INDIM_1600x1200,
	LCD200_INDIM_1920x1080,
	LCD200_INDIM_2560x1440,
	LCD200_INDIM_64x64,
	LCD200_INDIM_1024x600,
	LCD200_INDIM_MAX,
	LCD200_INDIM_DUMMY = 0x10000000,
} lcd200_drv_indim_t;

typedef enum lcd200_otype {
	LCD200_OTYPE_CVBS = 0,
  	LCD200_OTYPE_CVBS_NTSC = LCD200_OTYPE_CVBS,
  	LCD200_OTYPE_CVBS_PAL,
	LCD200_OTYPE_VGA_1024x768x60,
	LCD200_OTYPE_VGA_1280x720x60,
	LCD200_OTYPE_VGA_1280x1024x60,
	LCD200_OTYPE_VGA_1600x1200x60,
	LCD200_OTYPE_VGA_1920x1080x60,
	LCD200_OTYPE_HDMI_720x480x60,
	LCD200_OTYPE_HDMI_720x576x50,
	LCD200_OTYPE_HDMI_1024x768x60,
	LCD200_OTYPE_HDMI_1280x720x60,
	LCD200_OTYPE_HDMI_1280x1024x60,
	LCD200_OTYPE_HDMI_1600x1200x60,
	LCD200_OTYPE_HDMI_1920x1080x60,
	LCD200_OTYPE_HDMI_2560x1440x30,
	LCD200_OTYPE_1080P,
	LCD200_OTYPE_64x64,
	LCD200_OTYPE_VGA_1024x600x60,
	LCD200_OTYPE_MAX,
	LCD200_OTYPE_DUMMY = 0x10000000,
} lcd200_drv_otype_t;

/*
 * @This function is used to both desk_res and output_type
 *
 * @function lcd200_drv_set_deskres_otype(int chip_idx, lcd200_drv_indim_t in_dim, lcd200_drv_otype_t otype);
 * @param: chip_idx.
 *		   in_dim: input dim
 *		   otype: output_type.
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_deskres_otype(int chip_idx, lcd200_drv_indim_t in_dim, lcd200_drv_otype_t otype);
int lcd200_drv_get_deskres_otype(int chip_idx, lcd200_drv_indim_t *in_dim, lcd200_drv_otype_t *otype);

/*
 * @This function is used to set desk resoultion
 *
 * @function lcd200_drv_set_deskres(int chip_idx, lcd200_indim_t in_dim);
 * @param: chip_idx.
 *		   in_dim: input dim
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_deskres(int chip_idx, lcd200_drv_indim_t in_dim);
int lcd200_drv_get_deskres(int chip_idx, lcd200_drv_indim_t *in_dim);

/*
 * @This function is used to set output type
 *
 * @function lcd200_drv_set_otype(int chip_idx, lcd200_indim_t in_dim);
 * @param: chip_idx.
 *		   otype: output type
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_otype(int chip_idx, lcd200_drv_otype_t otype);
int lcd200_drv_get_otype(int chip_idx, lcd200_drv_otype_t *otype);

typedef enum lcd200_infmt {
	LCD200_INFMT_YUV422,
	LCD200_INFMT_ARGB8888,
	LCD200_INFMT_RGB888,
	LCD200_INFMT_RGB565,
	LCD200_INFMT_ARGB1555,
	LCD200_INFMT_RGB1BPP,
	LCD200_INFMT_RGB2BPP,
	LCD200_INFMT_RGB4BPP,
	LCD200_INFMT_RGB8BPP,
	LCD200_INFMT_MAX,
	LCD200_INFMT_DUMMY = 0x10000000,
} lcd200_drv_infmt_t;

/*
 * @This function is used to set input format
 *
 * @function lcd200_drv_set_infmt(int chip_idx, int plane, lcd200_infmt_t infmt);
 * @param: chip_idx.
 *		   plane: which plane id
 *		   lcd200_infmt_t: input format
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_infmt(int chip_idx, int plane, lcd200_drv_infmt_t infmt);
int lcd200_drv_get_infmt(int chip_idx, int plane, lcd200_drv_infmt_t *infmt);

/*
 * @This function is used to get plane buffer address.
 *
 * @function lcd200_drv_get_plane_paddr(int chip_idx, int plane, unsigned int *paddr, unsigned int *vaddr);
 * @param: chip_idx.
 *		   plane: 0~2
 *		   paddr: physical address
 * @return 0 on success, <0 on error
 */
int lcd200_drv_get_plane_paddr(int chip_idx, int plane, uintptr_t *paddr, unsigned int *vaddr);
int lcd200_drv_set_plane_paddr(int chip_idx, int plane, uintptr_t paddr);

/* color key for LCD200 */
typedef struct lcd200_ckey {
    int fb_idx; /* INDEX */
    int enable; /* 0 for disabled, 1 for enabled */
    int ckey_r; /* 8bits R */
    int ckey_g; /* 8bits G */
    int ckey_b; /* 8bits B */
} lcd200_drv_ckey_t;

/*
 * @This function is used to set color key of LCD210
 *
 * @function int lcd200_drv_set_ckey(lcd200_drv_ckey_t *ckey);
 * @param: ckey. all parameters are necessary.
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_ckey(int chip_idx, lcd200_drv_ckey_t *ckey);

/*
 * @This function is used to configure every channel status
 *
 * @function int lcd200_drv_set_chanstate(int chip_idx, unsigned int pip_mode);
 * @param: pip_mode: PIP:0-Disable 1-Single PIP Win 2-Double PIP Win
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_chanstate(int chip_idx, unsigned int pip_mode);
int lcd200_drv_get_chanstate(int chip_idx, unsigned int *pip_mode);

/*
 * @This function is used to layer mapping for individual channels. The index is channel id.
 *
 * @function int lcd200_drv_set_layermapping(int layer0_id, int layer1_id, int layer2_id);
 * @param: layer0_id, layer1_id, int layer2_id. The value is 0/1/2
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_layermapping(int chip_idx, int layer0_id, int layer1_id, int layer2_id);
int lcd200_drv_get_layermapping(int chip_idx, int *layer0_id, int *layer1_id, int *layer2_id);

/*
 * @This function is used to set blending for high priority and low priority
 *
 * @function int lcd200_drv_set_blend(unsigned int blend_h, unsigned int blend_l);
 * @param: int blend_h, int blend_l, range 0-255
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_blend(int chip_idx, unsigned int blend_h, unsigned int blend_l);
int lcd200_drv_get_blend(int chip_idx, unsigned int *blend_h, unsigned int *blend_l);

/*
 * @This function is used to set 0/1 alpha value for individual channels. The index is channel id.
 *
 * @function int lcd200_drv_set_argb1555(int chan, int alpha0_val, int alpha1_val);
 * @param: chan: channel id
 * @param: alpha0_val/alpha1_val: alpha value 0-255
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_argb1555(int chip_idx, int chan, int alpha0_val, int alpha1_val);
int lcd200_drv_get_argb1555(int chip_idx, int chan, int *alpha0_val, int *alpha1_val);

typedef struct lcd200_palette {
	/* index
	 * LCD210: 1 ~ 255. 0 for transparent and internal use.
	 */
    int  index;
    int  rgb565;	/* RGB565, R[15:11], G[10:5], B[4:0] */
} lcd200_drv_palette_t;

/*
 * @This function is used to set palette ram.
 *
 * @function int lcd200_drv_set_palette(int chip_idx, lcd200_drv_palette_t *palette);
 * @param: palette: max value is 255
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_palette(int chip_idx, lcd200_drv_palette_t *palette);

typedef struct lcd200_sysinfo {
	int	fps;
	int	idim_xres;	///< input dim xres
	int idim_yres;	///< input dim yres
	int odim_xres;	///< output dim xres
	int odim_yres;	///< output dim xres
} lcd200_drv_sysinfo_t;

/*
 * @This function is used to get dimension info from lcd210
 *
 * @function int lcd200_drv_get_sysinfo(lcd200_drv_sysinfo_t *sysinfo);
 * @param: output to sysinfo
 * @return 0 on success, <0 on error
 */
int lcd200_drv_get_sysinfo(int chip_idx, lcd200_drv_sysinfo_t *sysinfo);

typedef enum lcd200_dispdev {
	/* HDMI */
	HDMI_LCD200_RGB = 0x1,
	/* VGA */
	VGA_LCD200_RGB = 0x10,
	/* External pinmux0 */
	EXT0_LCD200_RGB = 0x100,
	EXT0_LCD200_BT1120 = 0x200,
	/* External pinmux1 */
	EXT1_LCD200_RGB0 = 0x1000,
	EXT1_LCD200_BT1120 = 0x2000,
	CVBS_LCD200 = 0x10000,
	DISPDEV_LCD200_DUMMY = 0x10000000,
} lcd200_drv_dispdev_t;

#define HDMI_LCD200_MASK	(HDMI_LCD200_RGB)
#define	VGA_LCD200_MASK		(VGA_LCD200_RGB)
#define EXT0_LCD200_MASK	(EXT0_LCD200_RGB | EXT0_LCD200_BT1120)
#define EXT1_LCD200_MASK	(EXT1_LCD200_RGB0 | EXT1_LCD200_BT1120)
#define CVBS_LCD200_MASK	(CVBS_LCD200)
/*
 * @This function is used to set the input source to HDMI/VGA/EXTERNAL
 *
 * @function int lcd200_drv_set_dispdev(int chip_idx, lcd200_dispdev_t vout);
 * @param: chip_idx, dispdev
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_dispdev(int chip_idx, unsigned int dispdev_bmap); /* lcd200_drv_dispdev_t */

/*
 * @This function is used to set source frame rate
 *
 * @function int lcd200_drv_set_srcfps(int chip_idx, int fps);
 * @param: chip_idx, fps (frame per second)
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_srcfps(int chip_idx, int fps);

#define LCD200_EDID_TABLE_MAX	20

typedef struct lcd200_edid {
	lcd200_drv_otype_t	otype[LCD200_EDID_TABLE_MAX];
	int valid_num;
} lcd200_drv_edid_t;

/*
 * @This function is used to get edid table
 *
 * @function int lcd200_drv_get_edid(int chip_idx, lcd200_drv_edid_t *edid_table);
 * @param: chip_idx, edid_table
 * @return 0 on success, <0 on error
 */
int lcd200_drv_get_edid(int chip_idx, lcd200_drv_edid_t *edid_table);

typedef struct lcd200_mp {
	enum lcd200_indim	indim;
	enum lcd200_otype	otype;
	int	fb0_fb1_buf_share;	/* 0 for fb0/1 allocates different buffers. 1 for share */
	int homology;			/* only suitable to lcd210. When it is set, it will not register to videograph */
	struct {
		int	max_w; 	/* each plane must be identical */
		int	max_h;	/* each plane must be identical */
		int max_bpp;
		int	gui_pan_display;	/* not supported yet */
		uintptr_t buf_paddr;
		unsigned int buf_len;
		int	reserved[5];
	} plane[3];
	int	reserved[5];
} lcd200_drv_mp_t;	/* module parameters */

/*
 * @This function is used to int moudule parameter
 *
 * @function int lcd200_drv_set_mp_init(int chip_idx, lcd200_drv_mp_t *mp, int mp_len);
 * @param: chip_idx, mp, mp_len
 * @return 0 on success, <0 on error
 */
int lcd200_drv_set_mp_init(int chip_idx, lcd200_drv_mp_t *mp, int mp_len);
int lcd200_drv_set_mp_uninit(int chip_idx);

typedef struct lcd200_iq {
	int	brightness;		/* -127~127 */
	struct {
		unsigned int	k0;	/* 0-15 */
		unsigned int	k1;	/* 0-15 */
		unsigned int	threshold0;	/* 0-255 */
		unsigned int	threshold1;	/* 0-255 */
	} sharpness;
	struct {
		unsigned int contrast;	/* 1-31 */
	} contrast;
	struct {
		int cos_val;	/* -32 ~ 32. Cos@ = (HueCosValue / 32). @ is the rotating degreee from -180 ~ 180 */
		int	sin_val;	/* -32 ~ 32. Sin@ = (HueSinValue / 32). @ is the rotating degreee from -180 ~ 180 */
	} hue;
} lcd200_drv_iq_t;

/*
 * @This function is used to get/set iq table
 *
 * @function int lcd200_drv_get_iq(int chip_idx, lcd200_drv_iq_t *iq);
 * @param: chip_idx, iq
 * @return 0 on success, <0 on error
 */
int lcd200_drv_get_iq(int chip_idx, lcd200_drv_iq_t *iq);
int lcd200_drv_set_iq(int chip_idx, lcd200_drv_iq_t *iq);

#endif /* __LCD200_API_H__ */