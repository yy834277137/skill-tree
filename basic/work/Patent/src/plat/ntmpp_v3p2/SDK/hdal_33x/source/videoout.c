/**
 * @file videoout.c
 * @brief type definition of API.
 * @author PSW
 * @date in the year 2018
 */


/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include "hdal.h"
#include "videoout.h"
#include "vpd_ioctl.h"
#include "cif_common.h"
#include "dif.h"
#include "lcd300/lcd300_fb.h"
#include "kflow_videoout_lcd310_lmt.h"
#include "kflow_videoout_lcd210_lmt.h"
#include "pif.h"
#include "pif_ioctl.h"
#include "osg_ioctl.h"
#include "logger.h"
#include "lcd300/lcd300_fb.h"
#include "lcd200_v3/lcd200_fb.h"
#include "lcd200_v3/lcd200_api.h"
#include "kdrv_videoout/hdmi/kdrv_hdmi_ioctrl.h"
#include "kdrv_videoout/kdrv_tve100_ioctrl.h"
//#endif
/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define LCD310_LITE    1 //"/dev/hdal_lcd300_1"
#define LCD210         2
#define LCD210_1       3

#define SECOND_LCD	   LCD310_LITE
#define THIRD_LCD      LCD210
#define FB_SHIFT		3
/*-----------------------------------------------------------------------------*/
/* External Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
extern VDDO_PARAM g_video_out_param[HD_VIDEOOUT_MAX_DEVICE_ID];
extern int vpd_fd;
HD_RESULT videoout_get_lcd_dim(UINT8 lcd_id, HD_VIDEOOUT_MODE *lcd_mode);
extern UINT32 hd_videoout_max_device_nr;
extern UINT32 hd_videoout_max_device_out;
extern UINT32 hd_videoout_max_device_in;
extern vpd_sys_info_t platform_sys_Info;
#define MAX_OSG_LOAD_NUM 32
#define WAIT_CLRWIN_CNT 5
#define TIMEOUTVAL (100*1000)
extern int osg_ioctl;
extern HD_RESULT check_videoout_exist(int id);
extern unsigned int gmlib_dbg_level;
/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them
/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
extern UINT32 utl_get_disp_osg_index(HD_DAL device_subid);
VDDO_INTERNAL_PARAM lcd_cfg[HD_VIDEOOUT_MAX_DEVICE_ID];
static INT module_init_done = 0;
lcd300_drv_indim_t videoout_get_lcd300_desk_res(HD_VIDEOOUT_INPUT_DIM input_dim);
lcd200_drv_indim_t videoout_get_lcd200_desk_res(HD_VIDEOOUT_INPUT_DIM input_dim);
HD_VIDEOOUT_INPUT_DIM get_lcd300_input_fmt_p(lcd300_drv_indim_t indim);
HD_RESULT videoout_wait_clearwin_done_p(int lcd_vch);
HD_RESULT videoout_fb_open(void);
extern int vpd_fd;
#define LCD_FB_NU   (MAX_VIDEOOUT_CTRL_ID * 3)
static INT32 lcd_fb_fd[LCD_FB_NU];
int mp_init_done[MAX_VIDEOOUT_CTRL_ID] = {0};
/*I2C strtuct and cmd */
#define EDID_I2C_ADDR	(0xA0 >>1)
#define I2C_SLAVE_FORCE       0x0706
#define I2C_RDWR	0x0707	/* Combined R/W transfer (one stop only)*/

//<< Sync with vendor_videoout.h
typedef enum _VENDOR_VIDEOOUT_BT_ID {
	VENDOR_VIDEOOUT_BT1120_1280X720P60 = 0, ///< EXT VIDEO FORMAT IS BT1120_1280X720P60
	VENDOR_VIDEOOUT_BT1120_1280X720P30,      ///< EXT VIDEO FORMAT IS BT1120_1280X720P30
	VENDOR_VIDEOOUT_BT1120_1280X720P25,      ///< EXT VIDEO FORMAT IS BT1120_1280X720P25
	VENDOR_VIDEOOUT_BT1120_1920X1080P60,  ///< EXT VIDEO FORMAT IS BT1120_19200X1080P60
	VENDOR_VIDEOOUT_BT1120_1024X768P60,  	///< EXT VIDEO FORMAT IS BT1120_1024X768P60
	VENDOR_VIDEOOUT_BT1120_1280X1024P60,  	///< EXT VIDEO FORMAT IS BT1120_1280X1024P60
	VENDOR_VIDEOOUT_BT1120_1600X1200P60,  	///< EXT VIDEO FORMAT IS BT1120_1600x1200P60
 	VENDOR_VIDEOOUT_BT1120_37M_1280X720P30,  ///< EXT VIDEO FORMAT IS BT1120_1280X720P30
    VENDOR_VIDEOOUT_BT1120_37M_1280X720P25,  ///< EXT VIDEO FORMAT IS BT1120_1280X720P25
	VENDOR_VIDEOOUT_BT1120_1920X1080P50,    ///< EXT VIDEO FORMAT IS BT1120_19200X1080P50
	VENDOR_VIDEOOUT_BT1120_1440X900P60,	

	VENDOR_VIDEOOUT_BT656_1280X720P30  = 100,     ///< EXT VIDEO FORMAT IS BT656_1280X720P30
	VENDOR_VIDEOOUT_BT656_1280X720P25 ,      ///< EXT VIDEO FORMAT IS BT656_1280X720P25

	VENDOR_VIDEOOUT_BT_NO_CHANGE  = 0xFFFE, ///< EXT VIDEO FORMAT USE CURRENT SETTING
	VENDOR_VIDEOOUT_BT_MAX,
	ENUM_DUMMY4WORD(VENDOR_VIDEOOUT_BT_ID)
} VENDOR_VIDEOOUT_BT_ID;

typedef enum _VENDOR_VIDEOOUT_HDMI_ID {
    VENDOR_VIDEOOUT_HDMI_1440X900P60    = 60,    ///< HDMI VIDEO FORMAT IS 1440X900 & PROGRESSIVE 60FPS,start from HD_VIDEOOUT_HDMI_720X480I240_16X9
    VENDOR_VIDEOOUT_HDMI_1680X1050P60   = 61,    ///< HDMI VIDEO FORMAT IS 1680X1050 & PROGRESSIVE 60FPS
    VENDOR_VIDEOOUT_HDMI_1920X1200P60   = 62,    ///< HDMI VIDEO FORMAT IS 1920X1200 & PROGRESSIVE 60FPS
    VENDOR_VIDEOOUT_HDMI_1280X720P25    = 63,
    VENDOR_VIDEOOUT_HDMI_1280X720P30    = 64,
    VENDOR_VIDEOOUT_HDMI_NO_CHANGE  = 0xFFFE, ///< HDMI VIDEO FORMAT USE CURRENT SETTING
    VENDOR_VIDEOOUT_HDMI_MAX,
    ENUM_DUMMY4WORD(VENDOR_VIDEOOUT_HDMI_ID)
} VENDOR_VIDEOOUT_HDMI_ID;

typedef enum _VENDOR_VIDEOOUT_VGA_ID {
    VENDOR_VIDEOOUT_VGA_1440X900    = 6,    ///< VGA VIDEO FORMAT IS 1440X900,start from HD_VIDEOOUT_VGA_1920X1080
    VENDOR_VIDEOOUT_VGA_1680X1050   = 7,    ///< VGA VIDEO FORMAT IS 1680X1050
    VENDOR_VIDEOOUT_VGA_1920X1200   = 8,    ///< VGA VIDEO FORMAT IS 1920X1200
    VENDOR_VIDEOOUT_VGA_1024X600    = 9,    ///< VGA VIDEO FORMAT IS 1024X600
    VENDOR_VIDEOOUT_VGA_MAX,
    ENUM_DUMMY4WORD(VENDOR_VIDEOOUT_VGA_ID)
} VENDOR_VIDEOOUT_VGA_ID;

typedef enum _VENDOR_VIDEOOUT_INPUT_DIM {
    VENDOR_VIDEOOUT_IN_1440x900 = 14,       ///< IN VIDEO FORMAT IS 1440x900,start from HD_VIDEOOUT_IN_3840x2160
    VENDOR_VIDEOOUT_IN_1680x1050 = 15,      ///< IN VIDEO FORMAT IS 1680x1050
    VENDOR_VIDEOOUT_IN_1920x1200 = 16,      ///< IN VIDEO FORMAT IS 1920x1200
    VENDOR_VIDEOOUT_IN_1024x600 = 17,
    VENDOR_VIDEOOUT_INPUT_DIM_MAX,
    ENUM_DUMMY4WORD(VENDOR_VIDEOOUT_INPUT_DIM)
} VENDOR_VIDEOOUT_INPUT_DIM;

typedef enum _VENDOR_VIDEO_OUT_TYPE {
	VENDOR_VIDEO_OUT_TYPE_NONE = HD_COMMON_VIDEO_OUT_LCD,
	VENDOR_VIDEO_OUT_BT1120,      ///< BT1120
	VENDOR_VIDEO_OUT_BT656,           ///< BT656
	VENDOR_VIDEO_OUT_TYPE_MAX,
	ENUM_DUMMY4WORD(VENDOR_VIDEO_OUT_TYPE)
} VENDOR_VIDEO_OUT_TYPE;
// >>
/******************************/
void videoout_check_which_lcd(INT lcd_id, unsigned int *p_is_lcd300, unsigned int *p_is_lcd200)
{
	if (!p_is_lcd300)
		return;
	if (!p_is_lcd200)
		return;

	*p_is_lcd300 = 0;
	*p_is_lcd200 = 0;

	if (lcd_id == HD_VIDEOOUT_VDDO0)
		*p_is_lcd300 = 1;
	if (lcd_id == HD_VIDEOOUT_VDDO1) {
#if (SECOND_LCD == LCD310_LITE)
	*p_is_lcd300 = 1;
#else
	*p_is_lcd200 = 1;
#endif
	}
	if (lcd_id == HD_VIDEOOUT_VDDO2)
		*p_is_lcd200 = 1;

}

/*1:mean mp init is done,others:0*/
int videoout_check_mp_init_done(int videoout_id)
{
	INT32 lcd_fd = -1;
	int ret = 0;
	int init_done = 0;

	if (mp_init_done[videoout_id]) {
		return mp_init_done[videoout_id];
	}
	switch (videoout_id) {
	case HD_VIDEOOUT_VDDO0:
		if ((lcd_fd = open("/dev/hdal_lcd300_0", O_RDWR)) >= 0) {
			if ((ret = ioctl(lcd_fd, LCD300_IOC_GET_MP_INIT_DONE, &init_done)) < 0) {
				HD_VIDEOOUT_ERR("LCD300_IOC_GET_MP_INIT_DONE fail,ret=%d,mp_init_done=%d\n", ret, init_done);
				mp_init_done[videoout_id] = 0;
			} else {
				mp_init_done[videoout_id] = init_done;
			}
		} else {
			mp_init_done[videoout_id] = 0;//not insert lcd300 ko file,pass check mp_init
		}
		break;
#if (SECOND_LCD == LCD210)
	case HD_VIDEOOUT_VDDO1:
		if ((lcd_fd = open("/dev/hdal_lcd200_0", O_RDWR)) >= 0) {
			if (ioctl(lcd_fd, LCD200_IOC_GET_MP_INIT_DONE, &init_done) < 0) {
				HD_VIDEOOUT_ERR("LCD200_IOC_GET_MP_INIT_DONE fail,ret=%d,mp_init_done=%d\n", ret, init_done);
				mp_init_done[videoout_id] = 0;
			} else {
				mp_init_done[videoout_id] = init_done;
			}
		} else {
			mp_init_done[videoout_id] = 0;//for nor insert lcd200 ko file,pass cehck mp_init
		}
		break;
#else
	case HD_VIDEOOUT_VDDO1:
		if ((lcd_fd = open("/dev/hdal_lcd300_1", O_RDWR)) >= 0) {
			if (ioctl(lcd_fd, LCD300_IOC_GET_MP_INIT_DONE, &init_done) < 0) {
				HD_VIDEOOUT_ERR("LCD300_IOC_GET_MP_INIT_DONE fail,ret=%d,mp_init_done=%d\n", ret, init_done);
				mp_init_done[videoout_id] = 0;
			} else {
				mp_init_done[videoout_id] = init_done;
			}
		} else {
			mp_init_done[videoout_id] = 0;//for nor insert lcd200 ko file,pass cehck mp_init
		}
		break;
#endif
#if (THIRD_LCD == LCD210)
	case HD_VIDEOOUT_VDDO2:
		if ((lcd_fd = open("/dev/hdal_lcd200_0", O_RDWR)) >= 0) {
			if (ioctl(lcd_fd, LCD200_IOC_GET_MP_INIT_DONE, &init_done) < 0) {
				HD_VIDEOOUT_ERR("LCD200_IOC_GET_MP_INIT_DONE fail,ret=%d,mp_init_done=%d\n", ret, init_done);
				mp_init_done[videoout_id] = 0;
			} else {
				mp_init_done[videoout_id] = init_done;
			}
		} else {
			mp_init_done[videoout_id] = 0;//for nor insert lcd200 ko file,pass cehck mp_init
		}
		break;
#endif

	default :
		HD_VIDEOOUT_ERR("Not support lcd id=%d to open\n", videoout_id);
		break;
	}
	if (lcd_fd >= 0) {
		close(lcd_fd);
	}
	return mp_init_done[videoout_id];
}

HD_RESULT videoout_dev_exist(UINT8 lcd_id)
{
	INT32 lcd_fd = 0;
	HD_RESULT ret = HD_OK;

	switch (lcd_id) {
	case HD_VIDEOOUT_VDDO0:
		if (lcd_fb_fd[0] >= 0) {
			break;
		}
		if ((lcd_fd = open("/dev/fb0", O_RDWR)) >= 0) {
			close(lcd_fd);
		} else {
			ret = HD_ERR_SYS;
		}
		break;
	case HD_VIDEOOUT_VDDO1:
		if (lcd_fb_fd[3] >= 0) {
			break;
		}
		if ((lcd_fd = open("/dev/fb3", O_RDWR)) >= 0) {
			close(lcd_fd);
		} else {
			ret = HD_ERR_SYS;
		}
		break;

	case HD_VIDEOOUT_VDDO2:
		if (lcd_fb_fd[6] >= 0) {
			break;
		}
		if ((lcd_fd = open("/dev/fb6", O_RDWR)) >= 0) {
			close(lcd_fd);
		} else {
			ret = HD_ERR_SYS;
		}
		break;
	}
	return ret;
}

HD_RESULT videoout_lcd_ioctl(UINT8 lcd_id, UINT8 fb_id, int ioctl_type, void *param)
{
	INT32 lcd_fd = -1;
	HD_RESULT ret = HD_OK;
	INT fb_idx = 0;
	char ioctl_name[64] = "undefine";

	if (lcd_id >= MAX_VIDEOOUT_CTRL_ID) {
		HD_VIDEOOUT_ERR("Check lcd_id(%d) >= MAX_VIDEOOUT_ID(%d)\n", lcd_id, MAX_VIDEOOUT_CTRL_ID);
		ret = HD_ERR_PARAM;
		goto ext;
	}
	if (videoout_check_mp_init_done(lcd_id) == 0) {
		ret = HD_ERR_ALREADY_START;
		goto ext;
	}
re_try:
	switch (lcd_id) {
	case HD_VIDEOOUT_VDDO0:
	case HD_VIDEOOUT_VDDO1:
	case HD_VIDEOOUT_VDDO2:
		if (fb_id < 3) {
			fb_idx = fb_id + lcd_id * FB_SHIFT;
			if (fb_idx >= LCD_FB_NU) {
				HD_VIDEOOUT_ERR("Not support lcd_id=%d fb_id=%d to open\n", lcd_id, fb_id);
				ret = HD_ERR_PARAM;
				goto ext;
			}
			lcd_fd = lcd_fb_fd[fb_idx];
		} else {
			HD_VIDEOOUT_ERR("Not support lcd_id=%d fb_id=%d to open\n", lcd_id, fb_id);
			ret = HD_ERR_PARAM;
		}
		break;
	default:
		ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("Not support lcd id=%d to open\n", lcd_id);
		goto ext;
	}

	if (lcd_fd < 0) {//for bootup first videoout_init before mp_init
		if ((ret = videoout_fb_open()) != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_fb_open fail\n");
			goto ext;
		}
		goto re_try;
	}

	switch (ioctl_type) {
//<LCD300 class ioctl cmd
	case LCD300_IOC_SET_INDIM_OTYPE:
		if (ioctl(lcd_fd, LCD300_IOC_SET_INDIM_OTYPE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) cannot set LCD300_IOC_SET_INDIM_OTYPE.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_SET_INDIM_OTYPE");
		break;
	case LCD300_IOC_GET_INDIM_OTYPE:

		if (ioctl(lcd_fd, LCD300_IOC_GET_INDIM_OTYPE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) cannot set LCD300_IOC_GET_INDIM_OTYPE.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_GET_INDIM_OTYPE");
		break;
	case LCD300_IOC_SET_DISPDEV:

		if (ioctl(lcd_fd, LCD300_IOC_SET_DISPDEV, param)) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) cannot set LCD300_IOC_SET_DISPDEV.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_SET_DISPDEV");
		break;
	case LCD300_IOC_GET_MP:

		if (ioctl(lcd_fd, LCD300_IOC_GET_MP, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) cannot set LCD300_IOC_GET_MP.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_GET_MP");
		break;
	case LCD300_IOC_GET_PLANE_STATE:

		if (ioctl(lcd_fd, LCD300_IOC_GET_PLANE_STATE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_GET_PLANE_STATE fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_GET_PLANE_STATE");
		break;
	case LCD300_IOC_SET_PLANE_STATE:

		if (ioctl(lcd_fd, LCD300_IOC_SET_PLANE_STATE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_SET_PLANE_STATE fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_SET_PLANE_STATE");
		break;
	case LCD300_IOC_GET_INFMT:

		if (ioctl(lcd_fd, LCD300_IOC_GET_INFMT, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_GET_INFMT fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_GET_INFMT");
		break;
	case LCD300_IOC_SET_INFMT:

		if (ioctl(lcd_fd, LCD300_IOC_SET_INFMT, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_SET_INFMT fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_SET_INFMT");
		break;
	case LCD300_IOC_GET_ALPHA_ARGB1555:

		if (ioctl(lcd_fd, LCD300_IOC_GET_ALPHA_ARGB1555, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_GET_ALPHA_ARGB1555 fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_GET_ALPHA_ARGB1555");
		break;
	case LCD300_IOC_SET_ALPHA_ARGB1555:

		if (ioctl(lcd_fd, LCD300_IOC_SET_ALPHA_ARGB1555, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_SET_ALPHA_ARGB1555 fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_SET_ALPHA_ARGB1555");
		break;
	case LCD300_IOC_GET_BLENDING:

		if (ioctl(lcd_fd, LCD300_IOC_GET_BLENDING, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_GET_BLENDING fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_GET_BLENDING");
		break;
	case LCD300_IOC_SET_BLENDING:

		if (ioctl(lcd_fd, LCD300_IOC_SET_BLENDING, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_SET_BLENDING fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_SET_BLENDING");
		break;
	case LCD300_IOC_SET_COLOR_KEY:

		if (ioctl(lcd_fd, LCD300_IOC_SET_COLOR_KEY, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_SET_COLOR_KEY fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_SET_COLOR_KEY");
		break;
	case LCD300_IOC_GET_CHSCALE:

		if (ioctl(lcd_fd, LCD300_IOC_GET_CHSCALE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_GET_CHSCALE fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_GET_CHSCALE");
		break;
	case LCD300_IOC_SET_CHSCALE:

		if (ioctl(lcd_fd, LCD300_IOC_SET_CHSCALE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_SET_CHSCALE fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_SET_CHSCALE");
		break;
	case LCD300_IOC_SET_VGADAC_STATE:

		if (ioctl(lcd_fd, LCD300_IOC_SET_VGADAC_STATE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_SET_VGADAC_STATE fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD300_IOC_SET_VGADAC_STATE");
		break;
//<LCD200 class ioctl cmd
	case LCD200_IOC_SET_INDIM_OTYPE:

		if (ioctl(lcd_fd, LCD200_IOC_SET_INDIM_OTYPE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) cannot set LCD200_IOC_SET_INDIM_OTYPE.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_SET_INDIM_OTYPE");
		break;
	case LCD200_IOC_GET_INDIM_OTYPE:

		if (ioctl(lcd_fd, LCD200_IOC_GET_INDIM_OTYPE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) cannot set LCD200_IOC_GET_INDIM_OTYPE.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_GET_INDIM_OTYPE");
		break;
	case LCD200_IOC_SET_DISPDEV:

		if (ioctl(lcd_fd, LCD200_IOC_SET_DISPDEV, param)) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) cannot set LCD200_IOC_SET_DISPDEV.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_SET_DISPDEV");
		break;
	case LCD200_IOC_SET_CHIPSTATE:

		if (ioctl(lcd_fd, LCD200_IOC_SET_CHIPSTATE, param)) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) cannot set LCD200_IOC_SET_CHIPSTATE.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_SET_CHIPSTATE");
		break;
	case LCD200_IOC_GET_PLANE_STATE:

		if (ioctl(lcd_fd, LCD200_IOC_GET_PLANE_STATE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD200_IOC_GET_PLANE_STATE fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_GET_PLANE_STATE");
		break;
	case LCD200_IOC_SET_PLANE_STATE:

		if (ioctl(lcd_fd, LCD200_IOC_SET_PLANE_STATE, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d)  LCD200_IOC_SET_PLANE_STATE fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_SET_PLANE_STATE");
		break;
	case LCD200_IOC_GET_INFMT:

		if (ioctl(lcd_fd, LCD200_IOC_GET_INFMT, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d)  LCD200_IOC_GET_INFMT fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_GET_INFMT");
		break;
	case LCD200_IOC_SET_INFMT:

		if (ioctl(lcd_fd, LCD200_IOC_SET_INFMT, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d)  LCD200_IOC_SET_INFMT fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_SET_INFMT");
		break;
	case LCD200_IOC_GET_BLENDING:

		if (ioctl(lcd_fd, LCD200_IOC_GET_BLENDING, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD200_IOC_GET_BLENDING fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_GET_BLENDING");
		break;
	case LCD200_IOC_SET_BLENDING:

		if (ioctl(lcd_fd, LCD200_IOC_SET_BLENDING, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD200_IOC_SET_BLENDING fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_SET_BLENDING");
		break;
	case LCD200_IOC_SET_COLOR_KEY:

		if (ioctl(lcd_fd, LCD200_IOC_SET_COLOR_KEY, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD300_IOC_SET_COLOR_KEY fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_SET_COLOR_KEY");
		break;
	case LCD200_IOC_GET_INFO:

		if (ioctl(lcd_fd, LCD200_IOC_GET_INFO, param) < 0) {
			HD_VIDEOOUT_ERR("LCD_%d Error: fd(%d) LCD200_IOC_GET_INFO fail.\n", lcd_id, lcd_fd);
			ret = HD_ERR_SYS;
		}
		strcpy(ioctl_name, "LCD200_IOC_GET_INFO");
		break;
	default:
		HD_VIDEOOUT_ERR("Not Support lcd_idx(%d) ioctl_type(%d)\n", lcd_id, ioctl_type);
		ret = HD_ERR_NOT_SUPPORT;
		break;
	}
ext:
	GMLIB_L1_P("%s:lcd_id=%d fb_id=%d ioctl_type=%s ret=%d\n", __func__, lcd_id,  fb_id, ioctl_name, ret);
	return ret;
}

static int state_videoout_init = 0;//0:none 1:init
HD_RESULT videoout_fb_open(void)
{
	HD_RESULT ret = HD_OK;
	UINT8 i, j, fb_idx;
	int retry_cnt = 0;
	char dev_name[10];

	for (i = 0; i < MAX_VIDEOOUT_CTRL_ID; i++) {
		if (videoout_check_mp_init_done(i)) {//mp_init done,/dev/fbx should been created
re_open:
			fb_idx = i * FB_SHIFT;
			for (j = 0; j < FB_SHIFT; j++) {
				if (lcd_fb_fd[fb_idx] >= 0) {//有可能是{5,10,-1}的狀況,fb_idx從2開始
					fb_idx++;//跳過已經open 的fb
					continue;
				}
				sprintf(dev_name, "/dev/fb%d", fb_idx);
				lcd_fb_fd[fb_idx] = open(dev_name, O_RDWR);
				if (lcd_fb_fd[fb_idx] < 0) {
					retry_cnt++;
					if (retry_cnt > OPEN_FB_RETRY_CNT) {
						ret = HD_ERR_SYS;
						HD_VIDEOOUT_ERR("Open /dev/fb%d fail, waitting time(%dms)...\n", fb_idx,
								retry_cnt * WAIT_FB_TIME / 1000);
						goto ext;
					}
					usleep(WAIT_FB_TIME);
					goto re_open;
				}
				fb_idx++;
			}
		} else {
			fb_idx = i * FB_SHIFT;
			for (j = 0; j < FB_SHIFT; j++) {
				if (fb_idx < LCD_FB_NU) {
					lcd_fb_fd[fb_idx] = -1;
					fb_idx++;
				}
			}
		}
	}
ext:
	return ret;
}
void videoout_fb_close(void)
{
	int i;
	//close all fbx fd
	for (i = 0; i < LCD_FB_NU; i++) {
		if (lcd_fb_fd[i] >= 0) {
			close(lcd_fb_fd[i]);
			lcd_fb_fd[i] = -1;
		}
	}
}
HD_RESULT videoout_init(void)
{
	UINT dev;
	HD_RESULT ret = HD_OK;
	UINT8 i, j;

	if (!osg_ioctl) {
		HD_VIDEOOUT_ERR("%s: Error, no osg ioctl supported!", __FUNCTION__);
		return HD_ERR_SYS;
	}
	if (state_videoout_init) {
		HD_VIDEOOUT_ERR("%s: double videoout_init!", __FUNCTION__);
		return HD_ERR_SYS;
	}
	state_videoout_init = 1;

	for (dev = 0; dev < HD_VIDEOOUT_MAX_DEVICE_ID ; dev++) {//only lcd0 support new in_buf
		memset(&g_video_out_param[dev].stamp, 0, sizeof(g_video_out_param[dev].stamp));
		g_video_out_param[dev].num_stamp = 0;
		g_video_out_param[dev].max_stamp_idx = 0;
		memset(&g_video_out_param[dev].mask_mosaic, 0, sizeof(g_video_out_param[dev].mask_mosaic));
		g_video_out_param[dev].max_mask_idx = 0;
		g_video_out_param[dev].num_mask = 0;
	}

	memset(lcd_cfg, 0, sizeof(VDDO_INTERNAL_PARAM) * HD_VIDEOOUT_MAX_DEVICE_ID);
	for (i = 0; i < HD_VIDEOOUT_MAX_DEVICE_ID; i++) {
		lcd_cfg[i].p_syscaps = &g_video_out_param[i].syscaps;
		lcd_cfg[i].mode = &g_video_out_param[i].mode;
		for (j = 0; j < HD_FB_MAX; j++) {
			lcd_cfg[i].fb_param[j] = &g_video_out_param[i].fb_param[j];
		}
		for (j = 0; j < HD_VO_MAX_MASK; j++) {
			g_video_out_param[i].mask_mosaic[j].is_mask = -1;
		}
	}
	for (i = 0; i < LCD_FB_NU; i++) {
		lcd_fb_fd[i] = -1;
	}

	if (module_init_done == 0) {
		module_init_done = 1;
		if (0 != pif_set_module_init(&module_init_done)) {
			HD_COMMON_ERR("pif_set_mem_init fail.\n");
			return HD_ERR_NG;
		}
		if (pif_env_update() < 0) {
			HD_VIDEOOUT_ERR("ERR: pif_env_update fail.\n");
			ret = HD_ERR_NG;
		}
	}
	if (videoout_fb_open() != HD_OK) {
		HD_VIDEOOUT_ERR("ERR: videoout_fb_open fail.\n");
		ret = HD_ERR_NG;
	}
	return ret;
}

HD_RESULT videoout_uninit(void)
{
	UINT dev;
	HD_RESULT ret = HD_OK;
	UINT16 i;

	if (state_videoout_init == 0) { //skip if not videoout_init before
		return HD_OK;
	}
	state_videoout_init = 0;

	for (dev = 0; dev < HD_VIDEOOUT_MAX_DEVICE_ID; dev++) {
		//disable all osg mask
		if (g_video_out_param[dev].num_mask) {
			for (i = 0; i < (g_video_out_param[dev].max_mask_idx + 1); i++) {
				g_video_out_param[dev].mask_mosaic[i].enable = FALSE;
			}
			videoout_set_mask_mosaic(g_video_out_param[dev].osg_idx, g_video_out_param[dev].mask_mosaic, \
						 g_video_out_param[dev].max_mask_idx);
			g_video_out_param[dev].max_mask_idx = 0;
			g_video_out_param[dev].num_mask = 0;
			memset(g_video_out_param[dev].mask_mosaic, 0, sizeof(g_video_out_param[dev].mask_mosaic));
		}
		//disable all osg stamp
		if (g_video_out_param[dev].num_stamp) {
			for (i = 0; i < g_video_out_param[dev].max_stamp_idx; i++) {
				g_video_out_param[dev].stamp[i].win_enable = FALSE;
			}
			videoout_set_osg_win(g_video_out_param[dev].osg_idx, g_video_out_param[dev].stamp, \
					     g_video_out_param[dev].max_stamp_idx);
			g_video_out_param[dev].max_stamp_idx = 0;
			g_video_out_param[dev].num_stamp = 0;
			memset(g_video_out_param[dev].stamp, 0, sizeof(g_video_out_param[dev].stamp));
		}
		g_video_out_param[dev].osg_idx = 0;
		g_video_out_param[dev].is_open = 0;
	}
	videoout_fb_close();
	return ret;
}

HD_RESULT videoout_open(UINT8 lcd_id, HD_IO out_id, HD_IO in_id)
{
	HD_RESULT hd_ret = HD_OK;
	VDDO_PARAM *p_lcd_param;
	VDDO_INFO_PRIV *p_lcd_priv;
	//unsigned int queue_handle;
	uintptr_t queue_handle;
	vpd_em_type_t type;
	int in_idx = VO_INID(in_id);
	int fd = 0;

	if (in_idx >= HD_VIDEOOUT_MAX_IN) {
		HD_VIDEOOUT_ERR("Check in_idx(%d) >= HD_VIDEOOUT_MAX_IN(%d).\n", in_idx, HD_VIDEOOUT_MAX_IN);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	if (lcd_id >= HD_VIDEOOUT_MAX_DEVICE_ID) {
		HD_VIDEOOUT_ERR("Check lcd_id(%d) >= HD_VIDEOOUT_MAX_DEVICE_ID(%d).\n", lcd_id, HD_VIDEOOUT_MAX_DEVICE_ID);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_lcd_param = &g_video_out_param[lcd_id];
	if (out_id == HD_CTRL) {
		p_lcd_priv = &p_lcd_param->priv_ctrl;
		if (videoout_dev_exist(lcd_id) != HD_OK) {//for first set dev_config open,all source not ready,just return to config
			goto exit;
		}
	} else {
		if (videoout_dev_exist(lcd_id) != HD_OK) {
			HD_VIDEOOUT_ERR("videoout%d not config can't open,please open ctrl port and set HD_VIDEOOUT_PARAM_DEV_CONFIG.\n", lcd_id);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		p_lcd_priv = &p_lcd_param->priv[in_idx];
	}
	queue_handle = pif_create_queue_for_hdal();
	if (queue_handle == 0) {
		HD_VIDEOOUT_ERR("%s, create queue(%lu) failed.\n", __func__, queue_handle);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	p_lcd_priv->queue_handle = queue_handle;
	if (lcd_id == HD_VIDEOOUT_VDDO0) {
		type = VPD_TYPE_DISPLAY0;
#if (SECOND_LCD == LCD310_LITE)
	} else if (lcd_id == HD_VIDEOOUT_VDDO1) {
		type = VPD_TYPE_DISPLAY1;
#else//LCD210
	} else if (lcd_id == HD_VIDEOOUT_VDDO2) {
		type = VPD_TYPE_DISPLAY3;
#endif
	} else if (lcd_id == HD_VIDEOOUT_VDDO2) {
		type = VPD_TYPE_DISPLAY2;
	} else {
		goto exit;
	}
	fd = pif_alloc_user_fd_for_hdal(type, 0, 0, 0);
	//allocate fd
	if (fd == 0) {
		HD_VIDEOOUT_ERR("%s, alloc fd(%d) failed.\n", __func__, fd);
		return HD_ERR_SYS;
	}
	p_lcd_priv->fd = fd;

exit:
	return hd_ret;
}


HD_RESULT videoout_close(HD_PATH_ID path_id)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_stop_trigger_t stop_trigger_info;
	INT ret;
	HD_DAL self_id = HD_GET_DEV(path_id);
	INT32 dev_idx = VO_DEVID(self_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	VDDO_PARAM *p_lcd_info = &g_video_out_param[dev_idx];
	VDDO_INFO_PRIV *p_lcd_priv = NULL;

	if (ctrl_id == HD_CTRL) {
		p_lcd_priv = &p_lcd_info->priv_ctrl;
	} else {
		p_lcd_priv = &p_lcd_info->priv[VO_INID(in_id)];
	}
	if (p_lcd_priv->fd != 0) {
		memset(&stop_trigger_info, 0, sizeof(stop_trigger_info));
		stop_trigger_info.usr_proc_fd.fd = p_lcd_priv->fd;
		stop_trigger_info.mode = USR_PROC_STOP_FD;
		if ((ret = ioctl(vpd_fd, USR_PROC_STOP_TRIGGER, &stop_trigger_info)) < 0) {
			int errsv = errno;
			ret = errsv * -1;
			HD_VIDEOENC_ERR("<ioctl \"USR_PROC_STOP_TRIGGER\" fail, error(%d)>\n", ret);
			hd_ret = HD_ERR_NG;
		}
	}
	if (p_lcd_priv->queue_handle) {
		ret = pif_destroy_queue_for_hdal(p_lcd_priv->queue_handle);
		if (ret < 0) {
			HD_VIDEOOUT_ERR("%s, destroy queue failed. ret(%d)\n", __func__, ret);
			hd_ret = HD_ERR_NG;
		}
		p_lcd_priv->queue_handle = 0;
		p_lcd_priv->fd = 0;
	}
	return hd_ret;
}
void videoout_set_lcd300_fb(lcd300_drv_mp_t *lcd300_mp, HD_VIDEOOUT_DEVNVR_CONFIG *lcd_cfg)
{
//    lcd300_mp->plane[0].ddr_id = lcd_cfg->plane[0].ddr_id;
	lcd300_mp->plane[0].max_w = lcd_cfg->plane[0].max_w;
	lcd300_mp->plane[0].max_h = lcd_cfg->plane[0].max_h;
	lcd300_mp->plane[0].max_bpp = lcd_cfg->plane[0].max_bpp;
	lcd300_mp->plane[0].gui_rld_enable = lcd_cfg->plane[0].gui_rld_enable;
	lcd300_mp->plane[0].gui_pan_display = 0;
	lcd300_mp->plane[0].buf_paddr = lcd_cfg->plane[0].buf_paddr; /* physical */
	lcd300_mp->plane[0].buf_len = lcd_cfg->plane[0].buf_len;
	lcd300_mp->plane[0].rle_buf_paddr = lcd_cfg->plane[0].rle_buf_paddr;
	lcd300_mp->plane[0].rle_buf_len = lcd_cfg->plane[0].rle_buf_len;
	lcd300_mp->plane[0].ddrid = lcd_cfg->plane[0].ddr_id;
	// lcd300_mp->plane[1].ddr_id = lcd_cfg->plane[1].ddr_id;
	lcd300_mp->plane[1].max_w = lcd_cfg->plane[1].max_w;
	lcd300_mp->plane[1].max_h = lcd_cfg->plane[1].max_h;
	lcd300_mp->plane[1].max_bpp = lcd_cfg->plane[1].max_bpp;
	lcd300_mp->plane[1].gui_rld_enable = lcd_cfg->plane[1].gui_rld_enable;
	lcd300_mp->plane[1].gui_pan_display = 0;
	lcd300_mp->plane[1].buf_paddr = lcd_cfg->plane[1].buf_paddr; /* physical */
	lcd300_mp->plane[1].buf_len = lcd_cfg->plane[1].buf_len;
	lcd300_mp->plane[1].rle_buf_paddr = lcd_cfg->plane[1].rle_buf_paddr;
	lcd300_mp->plane[1].rle_buf_len = lcd_cfg->plane[1].rle_buf_len;
	lcd300_mp->plane[1].rle_buf_ratio = 50; /* 50% */
	lcd300_mp->plane[1].ddrid = lcd_cfg->plane[1].ddr_id;
//    lcd300_mp->plane[].ddr_id = lcd_cfg->plane[2].ddr_id;
	lcd300_mp->plane[2].max_w = lcd_cfg->plane[2].max_w;
	lcd300_mp->plane[2].max_h = lcd_cfg->plane[2].max_h;
	lcd300_mp->plane[2].max_bpp = lcd_cfg->plane[2].max_bpp;
	lcd300_mp->plane[2].gui_rld_enable = lcd_cfg->plane[2].gui_rld_enable;
	lcd300_mp->plane[2].gui_pan_display = 0;
	lcd300_mp->plane[2].buf_paddr = lcd_cfg->plane[2].buf_paddr; /* physical */
	lcd300_mp->plane[2].buf_len = lcd_cfg->plane[2].buf_len;
	lcd300_mp->plane[2].rle_buf_paddr = lcd_cfg->plane[2].rle_buf_paddr;
	lcd300_mp->plane[2].rle_buf_len = lcd_cfg->plane[2].rle_buf_len;
	lcd300_mp->plane[2].ddrid = lcd_cfg->plane[2].ddr_id;
}


void videoout_set_lcd200_fb(lcd200_drv_mp_t *lcd200_mp, HD_VIDEOOUT_DEVNVR_CONFIG *lcd_cfg)
{
	lcd200_mp->plane[0].max_w = lcd_cfg->plane[0].max_w;
	lcd200_mp->plane[0].max_h = lcd_cfg->plane[0].max_h;
	lcd200_mp->plane[0].max_bpp = lcd_cfg->plane[0].max_bpp;
	lcd200_mp->plane[0].gui_pan_display = 0;
	lcd200_mp->plane[0].buf_paddr = lcd_cfg->plane[0].buf_paddr; /* physical */
	lcd200_mp->plane[0].buf_len = lcd_cfg->plane[0].buf_len;

	lcd200_mp->plane[1].max_w = lcd_cfg->plane[1].max_w;
	lcd200_mp->plane[1].max_h = lcd_cfg->plane[1].max_h;
	lcd200_mp->plane[1].max_bpp = lcd_cfg->plane[1].max_bpp;
	lcd200_mp->plane[1].gui_pan_display = 0;
	lcd200_mp->plane[1].buf_paddr = lcd_cfg->plane[1].buf_paddr; /* physical */
	lcd200_mp->plane[1].buf_len = lcd_cfg->plane[1].buf_len;

	lcd200_mp->plane[2].max_w = lcd_cfg->plane[2].max_w;
	lcd200_mp->plane[2].max_h = lcd_cfg->plane[2].max_h;
	lcd200_mp->plane[2].max_bpp = lcd_cfg->plane[2].max_bpp;
	lcd200_mp->plane[2].gui_pan_display = 0;
	lcd200_mp->plane[2].buf_paddr = lcd_cfg->plane[2].buf_paddr; /* physical */
	lcd200_mp->plane[2].buf_len = lcd_cfg->plane[2].buf_len;
}

HD_RESULT videoout_set_lcd300_fb_mode(HD_VIDEOOUT_DEVNVR_CONFIG *lcd_cfg, void *mp)
{
	lcd300_drv_mp_t *lcd300_mp = NULL;
	HD_VIDEOOUT_MODE *lcd_mode;
	HD_RESULT ret = HD_OK;
	lcd300_mp = (lcd300_drv_mp_t *)mp;
	lcd_mode = &lcd_cfg->mode;
	videoout_set_lcd300_fb(lcd300_mp, lcd_cfg);

	if (lcd_mode->output_type == HD_COMMON_VIDEO_OUT_HDMI) {
		switch ((int)lcd_mode->output_mode.hdmi) {
		case HD_VIDEOOUT_HDMI_720X480P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_NTSC : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_720x480x60;
			break;
		case HD_VIDEOOUT_HDMI_720X576P50:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_PAL : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_720x576x50;
			break;
		case CIF_HD_VIDEOOUT_HDMI_3840X2160P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_3840x2160 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_3840x2160x60;
			break;
		case HD_VIDEOOUT_HDMI_3840X2160P30:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_3840x2160 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_3840x2160x30;
			break;
		case HD_VIDEOOUT_HDMI_3840X2160P24:
		case HD_VIDEOOUT_HDMI_3840X2160P25:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_3840x2160 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_3840x2160x25;
			break;
		case HD_VIDEOOUT_HDMI_2560X1440P30://5
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_2560x1440 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_2560x1440x30;
			break;
		case HD_VIDEOOUT_HDMI_2560X1440P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_2560x1440 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_2560x1440x60;
			break;
		case HD_VIDEOOUT_HDMI_1600X1200P60:
		case HD_VIDEOOUT_HDMI_1600X1200P30:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1600x1200 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1600x1200x60;
			break;
		case HD_VIDEOOUT_HDMI_1920X1080P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1920x1080x60;
			break;
		case HD_VIDEOOUT_HDMI_1920X1080P50:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1920x1080x50;
			break;
		case HD_VIDEOOUT_HDMI_1920X1080P30://10
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1920x1080x30;
			break;
		case HD_VIDEOOUT_HDMI_1920X1080P25:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1920x1080x25;
			break;
		case HD_VIDEOOUT_HDMI_1920X1080P24:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1920x1080x24;
			break;
		case HD_VIDEOOUT_HDMI_1280X1024P30:
		case HD_VIDEOOUT_HDMI_1280X1024P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1280x1024 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1280x1024x60;
			break;
		case HD_VIDEOOUT_HDMI_1280X720P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1280x720x60;
			break;
		case HD_VIDEOOUT_HDMI_1280X720P50://15
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1280x720x50;
			break;
		case HD_VIDEOOUT_HDMI_1024X768P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1024x768 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1024x768x60;
			break;
		case HD_VIDEOOUT_HDMI_1440X900P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1440x900 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1440x900x60;
			break;
		case HD_VIDEOOUT_HDMI_1680X1050P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1680x1050 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1680x1050x60;
			break;
		case HD_VIDEOOUT_HDMI_1920X1200P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1200 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_1920x1200x60;
			break;
		default:
			HD_VIDEOOUT_ERR("lcd300 not support hdmi_mode(%d).\n", lcd_mode->output_mode.hdmi);
			ret = HD_ERR_NOT_SUPPORT;
			break;
		}
	} else if (lcd_mode->output_type == HD_COMMON_VIDEO_OUT_VGA) {
		switch ((int)lcd_mode->output_mode.vga) {
		case HD_VIDEOOUT_VGA_1920X1080:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_VGA_1920x1080x60;
			break;

		case HD_VIDEOOUT_VGA_720X480:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_NTSC : \
					    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_HDMI_720x480x60;
			break;
		case HD_VIDEOOUT_VGA_1280X720:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_720P : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_VGA_1280x720x60;
			break;
		case HD_VIDEOOUT_VGA_1024X768:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1024x768 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_VGA_1024x768x60;
			break;
       case VENDOR_VIDEOOUT_VGA_1024X600:
            lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1024x600 : \
                       videoout_get_lcd300_desk_res(lcd_mode->input_dim);
            lcd300_mp->otype = LCD300_OTYPE_VGA_1024x600x60;
            break;
		case HD_VIDEOOUT_VGA_1280X1024:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1280x1024 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_VGA_1280x1024x60;
			break;
		case HD_VIDEOOUT_VGA_1600X1200:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1600x1200 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_VGA_1600x1200x60;
			break;
		case HD_VIDEOOUT_VGA_1440X900:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1440x900 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_VGA_1440x900x60;
			break;
		case HD_VIDEOOUT_VGA_1680X1050:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_1680x1050 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_VGA_1680x1050x60;
			break;
		case HD_VIDEOOUT_VGA_1920X1200:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_1920x1200 : \
					   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_VGA_1920x1200x60;
			break;
		default:
			HD_VIDEOOUT_ERR("lcd300 not support vga_mode(%d).\n", lcd_mode->output_mode.vga);
			ret = HD_ERR_NOT_SUPPORT;
			break;
		}
	} else if (lcd_mode->output_type == (UINT32)VENDOR_VIDEO_OUT_BT1120) {
		switch ((int)(lcd_mode->output_mode.lcd)) {

		case VENDOR_VIDEOOUT_BT1120_1920X1080P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_1920x1080 : \
							   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_BT1120_1920x1080x60;
			break;
        case VENDOR_VIDEOOUT_BT1120_1920X1080P50:
            lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_1920x1080 : \
                               videoout_get_lcd300_desk_res(lcd_mode->input_dim);
            lcd300_mp->otype = LCD300_OTYPE_BT1120_1920x1080x50;
            break;
			case VENDOR_VIDEOOUT_BT1120_1280X720P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_720P : \
							   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_BT1120_1280x720x60;
			break;
		case VENDOR_VIDEOOUT_BT1120_1280X720P30:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_720P : \
							   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_BT1120_1280x720x30;
			break;
		case VENDOR_VIDEOOUT_BT1120_1280X720P25:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_720P : \
							   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_BT1120_1280x720x25;
			break;
        case VENDOR_VIDEOOUT_BT1120_37M_1280X720P30:
            lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_720P : \
                               videoout_get_lcd300_desk_res(lcd_mode->input_dim);
            lcd300_mp->otype = LCD300_OTYPE_BT1120_1280x720x30V2;
            break;
        case VENDOR_VIDEOOUT_BT1120_37M_1280X720P25:
            lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_720P : \
                               videoout_get_lcd300_desk_res(lcd_mode->input_dim);
            lcd300_mp->otype = LCD300_OTYPE_BT1120_1280x720x25V2;
            break;
		case VENDOR_VIDEOOUT_BT1120_1280X1024P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_1280x1024 : \
							   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_BT1120_1280x1024x60;
			break;
		case VENDOR_VIDEOOUT_BT1120_1024X768P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_1024x768 : \
							   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_BT1120_1024x768x60;
			break;
		case VENDOR_VIDEOOUT_BT1120_1600X1200P60:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_1600x1200 : \
							   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_BT1120_1600x1200x60;
			break;
		case VENDOR_VIDEOOUT_BT1120_1440X900P60:
            lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_1440x900 : \
                               videoout_get_lcd300_desk_res(lcd_mode->input_dim);
            lcd300_mp->otype = LCD300_OTYPE_BT1120_1440x900x60;
            break;
		default:
			HD_VIDEOOUT_ERR("lcd300 not support lcd_mode(%d).\n", lcd_mode->output_mode.lcd);
			ret = HD_ERR_NOT_SUPPORT;
			break;
		}
	} else if (lcd_mode->output_type == (UINT32)VENDOR_VIDEO_OUT_BT656) {
		switch ((int)(lcd_mode->output_mode.lcd)) {
		case VENDOR_VIDEOOUT_BT656_1280X720P30:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_720P : \
							   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_BT1120_1280x720x30;//;
			break;
		case VENDOR_VIDEOOUT_BT656_1280X720P25:
			lcd300_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD300_INDIM_720P : \
							   videoout_get_lcd300_desk_res(lcd_mode->input_dim);
			lcd300_mp->otype = LCD300_OTYPE_BT1120_1280x720x25;
			break;
		default:
			HD_VIDEOOUT_ERR("lcd300 not support lcd_mode(%d).\n", lcd_mode->output_mode.lcd);
			ret = HD_ERR_NOT_SUPPORT;
			break;
		}
	}else {
		//HD_VIDEOOUT_ERR("Please modify nvt-top.dtsi to use external HDMI. \n");
		//HD_VIDEOOUT_ERR("lcd300 not support lcd_mode(%d).\n", lcd_mode->output_mode.lcd);
		ret = HD_ERR_NOT_SUPPORT;

	}
	return ret;
}

void videoout_get_lcd300_fb(lcd300_drv_mp_t *lcd300_mp, HD_VIDEOOUT_DEVNVR_CONFIG *lcd_cfg)
{


	lcd_cfg->homology = lcd300_mp->reserved[0];
	lcd_cfg->chip_state = 	lcd300_mp->reserved[1];
	lcd_cfg->plane[0].max_w = lcd300_mp->plane[0].max_w;
	lcd_cfg->plane[0].max_h = lcd300_mp->plane[0].max_h;
	lcd_cfg->plane[0].max_bpp = lcd300_mp->plane[0].max_bpp;
	lcd_cfg->plane[0].gui_rld_enable =  lcd300_mp->plane[0].gui_rld_enable;
	lcd_cfg->plane[0].buf_paddr = lcd300_mp->plane[0].buf_paddr; /* physical */
	lcd_cfg->plane[0].buf_len = lcd300_mp->plane[0].buf_len;
	lcd_cfg->plane[0].rle_buf_paddr = lcd300_mp->plane[0].rle_buf_paddr;
	lcd_cfg->plane[0].rle_buf_len = lcd300_mp->plane[0].rle_buf_len;
	lcd_cfg->plane[0].ddr_id = lcd300_mp->plane[0].ddrid;

	lcd_cfg->plane[1].max_w = lcd300_mp->plane[1].max_w;
	lcd_cfg->plane[1].max_h = lcd300_mp->plane[1].max_h;
	lcd_cfg->plane[1].max_bpp = lcd300_mp->plane[1].max_bpp;
	lcd_cfg->plane[1].gui_rld_enable = lcd300_mp->plane[1].gui_rld_enable;
	lcd_cfg->plane[1].buf_paddr = lcd300_mp->plane[1].buf_paddr; /* physical */
	lcd_cfg->plane[1].buf_len = lcd300_mp->plane[1].buf_len;
	lcd_cfg->plane[1].rle_buf_paddr = lcd300_mp->plane[1].rle_buf_paddr;
	lcd_cfg->plane[1].rle_buf_len = lcd300_mp->plane[1].rle_buf_len;
	lcd_cfg->plane[1].ddr_id = lcd300_mp->plane[1].ddrid;

	lcd_cfg->plane[2].max_w = lcd300_mp->plane[2].max_w;
	lcd_cfg->plane[2].max_h = lcd300_mp->plane[2].max_h;
	lcd_cfg->plane[2].max_bpp = lcd300_mp->plane[2].max_bpp;
	lcd_cfg->plane[2].gui_rld_enable = lcd300_mp->plane[2].gui_rld_enable;
	lcd_cfg->plane[2].buf_paddr = lcd300_mp->plane[2].buf_paddr; /* physical */
	lcd_cfg->plane[2].buf_len = lcd300_mp->plane[2].buf_len;
	lcd_cfg->plane[2].rle_buf_paddr = lcd300_mp->plane[2].rle_buf_paddr;
	lcd_cfg->plane[2].rle_buf_len = lcd300_mp->plane[2].rle_buf_len ;
	lcd_cfg->plane[2].ddr_id = lcd300_mp->plane[2].ddrid;
}

HD_RESULT videoout_get_lcd300_dev_cfg(HD_VIDEOOUT_DEVNVR_CONFIG *lcd_cfg, lcd300_drv_mp_t *lcd300_mp)
{
	HD_VIDEOOUT_MODE *lcd_mode = NULL;
	HD_RESULT ret = HD_OK;

	videoout_get_lcd300_fb(lcd300_mp, lcd_cfg);
	lcd_mode = &lcd_cfg->mode;

	switch (lcd300_mp->otype) {
	case LCD300_OTYPE_CVBS_NTSC:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_720x480;
		lcd_mode->output_mode.cvbs = HD_VIDEOOUT_CVBS_NTSC;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_CVBS;
		break;
	case LCD300_OTYPE_CVBS_PAL:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_720x576;
		lcd_mode->output_mode.cvbs = HD_VIDEOOUT_CVBS_PAL;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_CVBS;
		break;
	case LCD300_OTYPE_VGA_720x480x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_720x480;
		lcd_mode->output_mode.vga = HD_VIDEOOUT_VGA_720X480;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_VGA;
		break;
    case LCD300_OTYPE_VGA_1024x600x60:
        lcd_mode->input_dim = VENDOR_VIDEOOUT_IN_1024x600;
        lcd_mode->output_mode.vga = VENDOR_VIDEOOUT_VGA_1024X600;
        lcd_mode->output_type = HD_COMMON_VIDEO_OUT_VGA;
        break;
	case LCD300_OTYPE_VGA_1024x768x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1024x768;
		lcd_mode->output_mode.vga = HD_VIDEOOUT_VGA_1024X768;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_VGA;
		break;
	case LCD300_OTYPE_VGA_1280x720x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x720;
		lcd_mode->output_mode.vga = HD_VIDEOOUT_VGA_1280X720;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_VGA;
		break;
	case LCD300_OTYPE_VGA_1280x1024x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x1024;
		lcd_mode->output_mode.vga = HD_VIDEOOUT_VGA_1280X1024;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_VGA;
		break;
	case LCD300_OTYPE_VGA_1600x1200x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1600x1200;
		lcd_mode->output_mode.vga = HD_VIDEOOUT_VGA_1600X1200;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_VGA;
		break;
	case LCD300_OTYPE_VGA_1920x1080x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1920x1080;
		lcd_mode->output_mode.vga = HD_VIDEOOUT_VGA_1920X1080;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_VGA;
		break;
	case LCD300_OTYPE_HDMI_720x480x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_720x480;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_720X480P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_720x576x50:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_720x576;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_720X576P50;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_1024x768x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1024x768;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1024X768P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_1280x720x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x720;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1280X720P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_1280x1024x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x1024;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1280X1024P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_1600x1200x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1600x1200;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1600X1200P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_1920x1080x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1920x1080;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1920X1080P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_2560x1440x30:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_2560x1440;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_2560X1440P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_3840x2160x30:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_3840x2160;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_3840X2160P30;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_2560x1440x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_2560x1440;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_2560X1440P30;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_VGA_1440x900x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1440x900;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1440X900P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_VGA_1680x1050x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1680x1050;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1680X1050P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_VGA_1920x1200x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1920x1200;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1920X1200P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_1440x900x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1440x900;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1440X900P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_1680x1050x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1680x1050;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1680X1050P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_HDMI_1920x1200x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1920x1200;
		lcd_mode->output_mode.hdmi = HD_VIDEOOUT_HDMI_1920X1200P60;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	case LCD300_OTYPE_BT1120_1920x1080x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1920x1080;
		lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT1120_1920X1080P60;
		lcd_mode->output_type = VENDOR_VIDEO_OUT_BT1120;
		break;
    case LCD300_OTYPE_BT1120_1920x1080x50:
        lcd_mode->input_dim = HD_VIDEOOUT_IN_1920x1080;
        lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT1120_1920X1080P50;
        lcd_mode->output_type = VENDOR_VIDEO_OUT_BT1120;
        break;
	case LCD300_OTYPE_BT656_1280x720x30:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x720;
		lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT656_1280X720P30;
		lcd_mode->output_type = VENDOR_VIDEO_OUT_BT656;
		break;
       case LCD300_OTYPE_BT656_1280x720x25:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x720;
		lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT656_1280X720P25;
		lcd_mode->output_type = VENDOR_VIDEO_OUT_BT656;
		break;
	case LCD300_OTYPE_BT1120_1280x720x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x720;
		lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT1120_1280X720P60;
		lcd_mode->output_type = VENDOR_VIDEO_OUT_BT1120;
		break;
	case LCD300_OTYPE_BT1120_1280x720x25:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x720;
		lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT1120_1280X720P25;
		lcd_mode->output_type = VENDOR_VIDEO_OUT_BT1120;
		break;
    case LCD300_OTYPE_BT1120_1280x720x25V2:
        lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x720;
        lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT1120_37M_1280X720P25;
        lcd_mode->output_type = VENDOR_VIDEO_OUT_BT1120;
        break;	
    case LCD300_OTYPE_BT1120_1280x720x30V2:
        lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x720;
        lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT1120_37M_1280X720P30;
        lcd_mode->output_type = VENDOR_VIDEO_OUT_BT1120;
        break;
	case LCD300_OTYPE_BT1120_1280x1024x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1280x1024;
		lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT1120_1280X1024P60;
		lcd_mode->output_type = VENDOR_VIDEO_OUT_BT1120;
		break;
	case LCD300_OTYPE_BT1120_1024x768x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1024x768;
		lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT1120_1024X768P60;
		lcd_mode->output_type = VENDOR_VIDEO_OUT_BT1120;
		break;
	case LCD300_OTYPE_BT1120_1600x1200x60:
		lcd_mode->input_dim = HD_VIDEOOUT_IN_1600x1200;
		lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT1120_1600X1200P60;
		lcd_mode->output_type = VENDOR_VIDEO_OUT_BT1120;
		break;
	case LCD300_OTYPE_BT1120_1440x900x60:
        lcd_mode->input_dim = HD_VIDEOOUT_IN_1440x900;
        lcd_mode->output_mode.lcd = VENDOR_VIDEOOUT_BT1120_1440X900P60;
        lcd_mode->output_type = VENDOR_VIDEO_OUT_BT1120;
		break;
	default:
		HD_VIDEOOUT_ERR("lcd300 not support otype(%d).\n", lcd300_mp->otype);
		ret = HD_ERR_NOT_SUPPORT;
		break;
	}

	return ret;
}

HD_RESULT videoout_set_lcd200_fb_mode(int lcd_idx, HD_VIDEOOUT_DEVNVR_CONFIG *lcd_cfg, void *mp)
{
	lcd200_drv_mp_t *lcd200_mp = NULL;
	HD_VIDEOOUT_MODE *lcd_mode;
	HD_RESULT ret = HD_OK;
	lcd200_mp = (lcd200_drv_mp_t *)mp;
	lcd_mode = &lcd_cfg->mode;
	videoout_set_lcd200_fb(lcd200_mp, lcd_cfg);

	if (lcd_mode->output_type == HD_COMMON_VIDEO_OUT_HDMI) {
		switch ((int)lcd_mode->output_mode.hdmi) {
		case HD_VIDEOOUT_HDMI_1920X1080P60:
		case HD_VIDEOOUT_HDMI_1920X1080P30:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1920x1080 : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_HDMI_1920x1080x60;
			break;
		case HD_VIDEOOUT_HDMI_1280X1024P60:
		case HD_VIDEOOUT_HDMI_1280X1024P30:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1280x1024 : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_HDMI_1280x1024x60;
			break;
		case HD_VIDEOOUT_HDMI_1280X720P60:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_720P : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_HDMI_1280x720x60;
			break;
		case HD_VIDEOOUT_HDMI_1024X768P60:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1024x768 : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_HDMI_1024x768x60;
			break;
		case HD_VIDEOOUT_HDMI_1600X1200P60://5
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1600x1200 : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_HDMI_1600x1200x60;
			break;
		case HD_VIDEOOUT_HDMI_720X480P60:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_NTSC : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_HDMI_720x480x60;
			break;
		case HD_VIDEOOUT_HDMI_720X576P50:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_PAL : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_HDMI_720x576x50;
			break;
		default:
			HD_VIDEOOUT_ERR("lcd200 not support hdmi_mode(%d).\n", lcd_mode->output_mode.hdmi);
			ret = HD_ERR_NOT_SUPPORT;
			break;
		}
	} else if (lcd_mode->output_type == HD_COMMON_VIDEO_OUT_VGA) {
		switch ((int)lcd_mode->output_mode.vga) {
		case HD_VIDEOOUT_VGA_1920X1080:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1920x1080 : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_VGA_1920x1080x60;
			break;
		case HD_VIDEOOUT_VGA_1280X1024:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD200_INDIM_1280x1024 : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_VGA_1280x1024x60;
			break;
		case HD_VIDEOOUT_VGA_1280X720:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_720P : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_VGA_1280x720x60;
			break;
		case HD_VIDEOOUT_VGA_1024X768:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1024x768 : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_VGA_1024x768x60;
			break;
        case VENDOR_VIDEOOUT_VGA_1024X600:
            lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1024x600 : \
                       videoout_get_lcd200_desk_res(lcd_mode->input_dim);
            lcd200_mp->otype = LCD200_OTYPE_VGA_1024x600x60;
            break;
		case HD_VIDEOOUT_VGA_1600X1200:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1600x1200 : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_VGA_1600x1200x60;
			break;
		default:
			HD_VIDEOOUT_ERR("lcd200 not support vga_mode(%d).\n", lcd_mode->output_mode.vga);
			ret = HD_ERR_NOT_SUPPORT;
			break;
		}
	} else if (lcd_mode->output_type == HD_COMMON_VIDEO_OUT_CVBS) {
		switch ((int)lcd_mode->output_mode.cvbs) {
		case HD_VIDEOOUT_CVBS_NTSC:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD200_INDIM_NTSC : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_CVBS_NTSC;
			break;
		case HD_VIDEOOUT_CVBS_PAL:
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_PAL : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_CVBS_PAL;
			break;
		default:
			HD_VIDEOOUT_ERR("lcd200 not support cvbs_mode(%d).\n", lcd_mode->output_mode.cvbs);
			ret = HD_ERR_NOT_SUPPORT;
			break;
		}
	} else if (lcd_mode->output_type == HD_COMMON_VIDEO_OUT_LCD) {
		switch ((int)(lcd_mode->output_mode.lcd)) {
		case (HD_VIDEOOUT_LCD_4 + 1):
			lcd200_mp->indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD200_INDIM_1920x1080 : \
					   videoout_get_lcd200_desk_res(lcd_mode->input_dim);
			lcd200_mp->otype = LCD200_OTYPE_1080P;
			break;
		default:
			HD_VIDEOOUT_ERR("lcd200 not support lcd_mode(%d).\n", lcd_mode->output_mode.lcd);
			ret = HD_ERR_NOT_SUPPORT;
			break;
		}
	} else if (lcd_mode->output_type == 0) { /* disable all setting */
		lcd200_mp->indim = lcd200_mp->otype = 0;
	} else {
		HD_VIDEOOUT_ERR("lcd_idx(%d) Not support output_type(%d).\n", lcd_idx, lcd_mode->output_type);
		ret = HD_ERR_NOT_SUPPORT;
	}
	return ret;
}

HD_RESULT videoout_set_videoout_output_lcd300(int homology, unsigned int vout_id)
{
	enum lcd300_dispdev videoout_dispdev = 0;
	HD_RESULT ret = HD_OK;
	int retry_cnt = 0;

	if (homology & (1 << HD_COMMON_VIDEO_OUT_HDMI))  {
		videoout_dispdev |=  HDMI_LCD300_RGB0;
	}
	if (homology & (1 << HD_COMMON_VIDEO_OUT_VGA))  {
		if (vout_id == 1) {
			videoout_dispdev |=  VGA_LCD300_RGB1;
		}
		else
			videoout_dispdev |=  VGA_LCD300_RGB0;
	}
	if (homology & (1 << HD_COMMON_VIDEO_OUT_CVBS))  {
		videoout_dispdev |=  CVBS_LCD200;
	}
	if (homology & (1 << HD_COMMON_VIDEO_OUT_LCD))  {
		videoout_dispdev |=  EXT0_LCD300_BT1120;
	}
	if (homology & (1 << HD_COMMON_VIDEO_OUT_RGB888))  {
		videoout_dispdev |=  EXT0_LCD300_RGB0;
	}
wait:
	if (videoout_check_mp_init_done(vout_id) == 0) {
		retry_cnt ++;
		printf("checkmp vout_id[%d] retry %d!\n", vout_id, retry_cnt);
		if (retry_cnt > 10) {
			ret = HD_ERR_ALREADY_START;
			HD_VIDEOOUT_ERR("wait videoout0 mp_init done timeout(%d ms)\n", (retry_cnt * WAIT_FB_TIME) / 1000);
			goto ext;
		} else {
			usleep(WAIT_FB_TIME);
			goto wait;
		}
	}
	ret = videoout_lcd_ioctl(vout_id, 0, LCD300_IOC_SET_DISPDEV, &videoout_dispdev);
	if (ret != HD_OK) {
		HD_VIDEOOUT_ERR("set ioctl LCD300_IOC_SET_DISPDEV vout_id[%d] fail\n", vout_id);
	}
ext:
	return  ret;
}

HD_RESULT videoout_set_lcd300_vgadac(unsigned int vout_id, unsigned int is_on)
{
	unsigned int on = 0;
	HD_RESULT ret = HD_OK;
	if (is_on)
		on = 1;

	ret = videoout_lcd_ioctl(vout_id, 0, LCD300_IOC_SET_VGADAC_STATE, &on);
	if (ret != HD_OK) {
		HD_VIDEOOUT_ERR("set ioctl LCD300_IOC_SET_VGADAC_STATE vout_id[%d] fail\n", vout_id);
	}

	return  ret;
}

HD_RESULT videoout_set_videoout_output_lcd200(int homology, unsigned int vout_id)
{
	enum lcd200_dispdev videoout2_dispdev = 0;
	HD_RESULT ret = HD_OK;
	int retry_cnt = 0;

	if (homology & (1 << HD_COMMON_VIDEO_OUT_HDMI))  {
		videoout2_dispdev |=  HDMI_LCD200_RGB;
	}
	if (homology & (1 << HD_COMMON_VIDEO_OUT_VGA))  {
		videoout2_dispdev |=  VGA_LCD200_RGB;
	}
	if (homology & (1 << HD_COMMON_VIDEO_OUT_CVBS))  {
		videoout2_dispdev |=  CVBS_LCD200;
	}
	if (homology & (1 << HD_COMMON_VIDEO_OUT_LCD))  {
		videoout2_dispdev |=  EXT0_LCD200_BT1120;
	}
	if (homology & (1 << HD_COMMON_VIDEO_OUT_RGB888))  {
		videoout2_dispdev |=  EXT0_LCD200_RGB;
	}
wait:
	if (videoout_check_mp_init_done(vout_id) == 0) {
		retry_cnt ++;
		if (retry_cnt > 10) {
			ret = HD_ERR_ALREADY_START;
			HD_VIDEOOUT_ERR("wait videoout1 mp_init timeout(%d ms)\n", (retry_cnt * WAIT_FB_TIME) / 1000);
			goto ext;
		} else {
			usleep(WAIT_FB_TIME);
			goto wait;
		}
	}
	ret = videoout_lcd_ioctl(HD_VIDEOOUT_VDDO2, 0, LCD200_IOC_SET_DISPDEV, &videoout2_dispdev);
	if (ret != HD_OK) {
		HD_VIDEOOUT_ERR("set ioctl LCD200_IOC_SET_DISPDEV vout_id[%d] fail\n", vout_id);
	}
ext:
	return  ret;
}

HD_RESULT videoout_check_dev_fb_ready(UINT8 lcd_id)
{
	HD_RESULT ret = HD_OK;
	UINT8 i,  fb_idx;
	int retry_cnt = 0;
	INT32 fb_fd = -1;
	char dev_name[10];

	if (lcd_id >= MAX_VIDEOOUT_CTRL_ID) {
		HD_VIDEOOUT_ERR("Check lcd_id=%d > MAX_VIDEOOUT_CTRL_ID=%d\n", lcd_id, MAX_VIDEOOUT_CTRL_ID);
		ret = HD_ERR_PARAM;
		goto ext;
	}
	fb_idx = lcd_id * FB_SHIFT;//start fd id
	for (i = 0; i < FB_SHIFT; i++) {		
		sprintf(dev_name, "/dev/fb%d", fb_idx);
re_open:		
		fb_fd = open(dev_name, O_RDWR);
		if (fb_fd < 0) {
			retry_cnt++;
			if (retry_cnt > OPEN_FB_RETRY_CNT) {
				ret = HD_ERR_SYS;
				HD_VIDEOOUT_ERR("Open /dev/fb%d fail, waitting time(%dms)...\n", fb_idx, retry_cnt * WAIT_FB_TIME / 1000);
				goto ext;
		}
			usleep(WAIT_FB_TIME);
			goto re_open;
		} else  {
			printf("### OPEN %s OK\n", dev_name);
			close(fb_fd);
			fb_fd = -1;
		}		
		fb_idx++;
	}
ext:
	if (fb_fd >= 0)
		close(fb_fd);
	return ret;
}


HD_RESULT videoout_set_dev_cfg_p(UINT8 lcd_idx, HD_VIDEOOUT_DEV_CONFIG *p_vdo_dev_cfg)
{
	HD_RESULT ret = HD_OK;
	int lcd_fd = 0;
	lcd200_drv_mp_t lcd200_mp;
	lcd300_drv_mp_t lcd300_mp;
	int enable_lcd_200 = 0;

	switch (lcd_idx) {
	case HD_VIDEOOUT_VDDO0:
		lcd_fd = open("/dev/hdal_lcd300_0", O_RDWR);
		if (lcd_fd < 0) {
			HD_VIDEOOUT_ERR("LCD Error: cannot open lcd300 device.\n");
			return HD_ERR_SYS;
		}
		memset(&lcd300_mp, 0, sizeof(lcd300_drv_mp_t));
		if ((ret = videoout_set_lcd300_fb_mode(&p_vdo_dev_cfg->devnvr_cfg, (void *)&lcd300_mp)) != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_set_lcd300_fb_mode fail\n");
			close(lcd_fd);
			return HD_ERR_SYS;
		}
		printf("LCD300_IOC_SET_MP_INIT all -B\n");
		lcd300_mp.reserved[0] = p_vdo_dev_cfg->devnvr_cfg.homology;
		lcd300_mp.reserved[1] = p_vdo_dev_cfg->devnvr_cfg.chip_state;
		if (ioctl(lcd_fd, LCD300_IOC_SET_MP_INIT, &lcd300_mp) < 0) {
			HD_VIDEOOUT_ERR("LCD Error: cannot set LCD300_IOC_SET_MP_INIT.\n");
			close(lcd_fd);
			return HD_ERR_SYS;
		}
		close(lcd_fd);
		printf("LCD300_IOC_SET_MP_INIT all -E\n");
		if (p_vdo_dev_cfg->devnvr_cfg.chip_state) {
			p_vdo_dev_cfg->devnvr_cfg.homology |= (0x1 << p_vdo_dev_cfg->devnvr_cfg.mode.output_type);
			ret = videoout_set_videoout_output_lcd300(p_vdo_dev_cfg->devnvr_cfg.homology, HD_VIDEOOUT_VDDO0);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("LCD Error: set videoout_set_videoout_output_lcd300 fail.\n");
			}
		}
		break;
#if (SECOND_LCD == LCD210)
	case HD_VIDEOOUT_VDDO1:
		lcd_fd = open("/dev/hdal_lcd200_0", O_RDWR);
		if (lcd_fd < 0) {
			HD_VIDEOOUT_ERR("LCD Error: cannot open lcd200 device.\n");
			return HD_ERR_SYS;
		}

		memset(&lcd200_mp, 0, sizeof(lcd200_drv_mp_t));
		if ((ret = videoout_set_lcd200_fb_mode(lcd_idx, &p_vdo_dev_cfg->devnvr_cfg, (void *)&lcd200_mp)) != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_set_lcd200_fb_mode fail\n");
			close(lcd_fd);
			return HD_ERR_SYS;
		}

		if (ioctl(lcd_fd, LCD200_IOC_SET_MP_INIT, &lcd200_mp) < 0) {
			HD_VIDEOOUT_ERR("LCD Error: cannot set LCD200_IOC_SET_MP_INIT.\n");
			close(lcd_fd);
			return HD_ERR_SYS;
		}

		enable_lcd_200 = p_vdo_dev_cfg->devnvr_cfg.chip_state;

		ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_CHIPSTATE, &enable_lcd_200);
		if (ret != HD_OK) {
			close(lcd_fd);
			return HD_ERR_SYS;
		}

		close(lcd_fd);
		if (enable_lcd_200) {
			p_vdo_dev_cfg->devnvr_cfg.homology |= (0x1 << p_vdo_dev_cfg->devnvr_cfg.mode.output_type);
			ret = videoout_set_videoout_output_lcd200(p_vdo_dev_cfg->devnvr_cfg.homology, HD_VIDEOOUT_VDDO1);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("LCD Error: set videoout_set_videoout_output_lcd200 fail.\n");
			}
		}
		break;
#else //SECOND_LCD == LCD310_LITE
	case HD_VIDEOOUT_VDDO1:
		lcd_fd = open("/dev/hdal_lcd300_1", O_RDWR);
		if (lcd_fd < 0) {
			HD_VIDEOOUT_ERR("LCD Error: cannot open lcd300_1 device.\n");
			return HD_ERR_SYS;
		}
		memset(&lcd300_mp, 0, sizeof(lcd300_drv_mp_t));
		if (p_vdo_dev_cfg->devnvr_cfg.chip_state == 0) {//disable
			lcd300_mp.indim = LCD300_INDIM_64x64;
			lcd300_mp.otype = LCD300_OTYPE_64x64;
		}
		else if ((ret = videoout_set_lcd300_fb_mode(&p_vdo_dev_cfg->devnvr_cfg, (void *)&lcd300_mp)) != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_set_lcd300_fb_mode fail\n");
			close(lcd_fd);
			return HD_ERR_SYS;
		}
		printf("LCD300_IOC_SET_MP_INIT lite -B\n");
		lcd300_mp.reserved[0] = p_vdo_dev_cfg->devnvr_cfg.homology;
		lcd300_mp.reserved[1] = p_vdo_dev_cfg->devnvr_cfg.chip_state;
		if (ioctl(lcd_fd, LCD300_IOC_SET_MP_INIT, &lcd300_mp) < 0) {
			HD_VIDEOOUT_ERR("LCD Error: cannot set LCD300_IOC_SET_MP_INIT.\n");
			close(lcd_fd);
			return HD_ERR_SYS;
		}
		close(lcd_fd);
		printf("LCD300_IOC_SET_MP_INIT lite -E\n");
		if (p_vdo_dev_cfg->devnvr_cfg.chip_state) {
			p_vdo_dev_cfg->devnvr_cfg.homology |= (0x1 << p_vdo_dev_cfg->devnvr_cfg.mode.output_type);
			ret = videoout_set_videoout_output_lcd300(p_vdo_dev_cfg->devnvr_cfg.homology, HD_VIDEOOUT_VDDO1);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("LCD Error: set videoout_set_videoout_output_lcd300 fail.\n");
			}
		}
		printf("LCD310_1 OK");
		break;
#endif

	case HD_VIDEOOUT_VDDO2:
#if (THIRD_LCD == LCD210)
		lcd_fd = open("/dev/hdal_lcd200_0", O_RDWR);
#else// (THIRD_LCD == LCD210_1)
		lcd_fd = open("/dev/hdal_lcd200_1", O_RDWR);
#endif
		if (lcd_fd < 0) {
			HD_VIDEOOUT_ERR("LCD Error: cannot open lcd200 device.\n");
			return HD_ERR_SYS;
		}

		memset(&lcd200_mp, 0, sizeof(lcd200_drv_mp_t));
		if ((ret = videoout_set_lcd200_fb_mode(lcd_idx, &p_vdo_dev_cfg->devnvr_cfg, (void *)&lcd200_mp)) != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_set_lcd200_fb_mode fail\n");
			close(lcd_fd);
			return HD_ERR_SYS;
		}

		if (ioctl(lcd_fd, LCD200_IOC_SET_MP_INIT, &lcd200_mp) < 0) {
			HD_VIDEOOUT_ERR("LCD Error: cannot set LCD200_IOC_SET_MP_INIT.\n");
			close(lcd_fd);
			return HD_ERR_SYS;
		}
		close(lcd_fd);
		enable_lcd_200 = p_vdo_dev_cfg->devnvr_cfg.chip_state;	
		if (enable_lcd_200) {
			p_vdo_dev_cfg->devnvr_cfg.homology |= (0x1 << p_vdo_dev_cfg->devnvr_cfg.mode.output_type);
			ret = videoout_set_videoout_output_lcd200(p_vdo_dev_cfg->devnvr_cfg.homology, HD_VIDEOOUT_VDDO2);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("LCD Error: set videoout_set_videoout_output_lcd200 fail.\n");
			}
		}
		break;
	default:
		HD_VIDEOOUT_ERR("Not support lcd_idx(%d)\n", lcd_idx);
		return HD_ERR_NOT_SUPPORT;
	}
	if (videoout_check_dev_fb_ready(lcd_idx) != HD_OK) {
		HD_COMMON_ERR("Check lcd%d fb create  fail.\n", lcd_idx);
		return HD_ERR_NG;
	}
	if (module_init_done == 0) {
		module_init_done = 1;
		if (0 != pif_set_module_init(&module_init_done)) {
			HD_COMMON_ERR("pif_set_mem_init fail.\n");
			return HD_ERR_NG;
		}
	}
	if (pif_env_update() < 0) {
		HD_VIDEOOUT_ERR("ERR: pif_env_update fail.\n");
		ret = HD_ERR_NG;
	}
	return ret;
}

HD_RESULT videoout_set_mode(UINT8 lcd_idx, HD_VIDEOOUT_MODE *src, HD_VIDEOOUT_MODE *dest)
{
	HD_RESULT ret = HD_OK;
	UINT8 need_update = 0;
	switch ((UINT32)src->output_type) {
	case HD_COMMON_VIDEO_OUT_HDMI:
		if (dest->output_mode.hdmi != src->output_mode.hdmi) {
			dest->output_mode.hdmi = src->output_mode.hdmi;
			need_update = 1;
		}
		break;
	case HD_COMMON_VIDEO_OUT_VGA:
		if (dest->output_mode.vga != src->output_mode.vga) {
			dest->output_mode.vga = src->output_mode.vga;
			need_update = 1;
		}
		break;
	case HD_COMMON_VIDEO_OUT_CVBS:
		if (dest->output_mode.cvbs != src->output_mode.cvbs) {
			dest->output_mode.cvbs = src->output_mode.cvbs;
			need_update = 1;
		}
		break;
	case HD_COMMON_VIDEO_OUT_LCD:
	case VENDOR_VIDEO_OUT_BT1120:
	case VENDOR_VIDEO_OUT_BT656:
		if (dest->output_mode.lcd != src->output_mode.lcd) {
			dest->output_mode.lcd = src->output_mode.lcd;
			need_update = 1;
		}
		break;
	default:
		HD_VIDEOOUT_ERR("lcd_idx(%d) Not support output_type(%d)\n", lcd_idx, src->output_type);
		return HD_ERR_NOT_SUPPORT;
	}
	dest->output_type = src->output_type;
	if (dest->input_dim != src->input_dim) {
		dest->input_dim = src->input_dim;
		need_update = 1;
	}

	if (need_update) {
		*dest = *src;
		ret = videoout_set_mode_p(lcd_idx);
	}
	return ret;
}

HD_RESULT videoout_set_fb_enable_p(INT8 fb_id, INT8 lcd_id, FB_PARAM *fb_param)
{
	HD_RESULT ret = HD_OK;
	struct lcd300_plane_state fb_state;
	int lcd200_fb_state = 0;

	if (check_videoout_exist(lcd_id) && fb_id == HD_FB0) {
		HD_VIDEOOUT_ERR("ERROR: You should close videoout working to set fb0 state\n");
	}
	switch (lcd_id) {
	case HD_VIDEOOUT_VDDO0:
#if (SECOND_LCD == LCD310_LITE)
	case HD_VIDEOOUT_VDDO1:
#endif
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_GET_PLANE_STATE, &fb_state);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_GET_PLANE_STATE fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		fb_state.plane_state[fb_id] = fb_param->enable;
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_SET_PLANE_STATE, &fb_state);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_SET_PLANE_STATE fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		break;
#if (THIRD_LCD == LCD210)
	case HD_VIDEOOUT_VDDO2:
#if (SECOND_LCD == LCD210)
	case HD_VIDEOOUT_VDDO1:
#endif
		//lcd200_fb_state:0~1,0:fb0 enable,1:fb0/fb1 enable
		if (fb_id == HD_FB0) {
			printf("lcd%d not support to set fb%d\n", HD_VIDEOOUT_VDDO2, fb_id);
			goto err_exit;
		}  else if ((fb_id == HD_FB1) && (fb_param->enable == 1)) {
			lcd200_fb_state = 1;
		} else if ((fb_id == HD_FB1) && (fb_param->enable == 0)) {
			lcd200_fb_state = 0;
		}  else if ((fb_id == HD_FB2) && (fb_param->enable == 1)) {
			lcd200_fb_state = 1;
		} else if ((fb_id == HD_FB2) && (fb_param->enable == 0)) {
			lcd200_fb_state = 0;
		}  else {
			HD_VIDEOOUT_ERR("lcd%d not support set fb_id=%d,fb_state=%d\n.", HD_VIDEOOUT_VDDO2, fb_id, fb_param->enable);
			ret = HD_ERR_NOT_SUPPORT;
			goto err_exit;
		}

		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD200_IOC_SET_PLANE_STATE, &lcd200_fb_state);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD200_IOC_GET_PLANE_STATE fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		break;
#endif

	default:
		ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("Not support lcd id=%d to open\n", lcd_id);
	}

err_exit:
	return ret;
}


HD_RESULT videoout_get_fb_enable_p(INT8 fb_id, INT8 lcd_id, HD_FB_ENABLE *fb_state)
{
	HD_RESULT ret = HD_OK;
	struct lcd300_plane_state lcd300_fb_state;
	int lcd200_fb_state = 0;
	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);
	if (is_lcd300) {
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_GET_PLANE_STATE, &lcd300_fb_state);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_GET_PLANE_STATE fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		fb_state->enable = lcd300_fb_state.plane_state[fb_id];
	} else if (is_lcd200) {

		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD200_IOC_GET_PLANE_STATE, &lcd200_fb_state);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:lcd_id(%d),LCD200_IOC_GET_PLANE_STATE fail.\n", lcd_id);
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		fb_state->enable = lcd200_fb_state;

	} else {
		ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("Not support lcd id=%d to get_fb_enable_p\n", lcd_id);
	}

err_exit:
	return ret;
}

HD_RESULT videoout_get_hdmi_edid_p(HD_VIDEOOUT_EDID_DATA *p_edid_data)
{
	HD_RESULT hd_ret = HD_OK;
	hdmi20_edid_info_t edid_info;
	hdmi20_vid_info_t hdmi_vid;
	INT hdmi_fd = -1;
	INT i;
	INT ret;
	UINT8 retry_cnt = 0;

re_try :
	hdmi_fd = open("/dev/hdmi20", O_RDWR);
	if (hdmi_fd < 0) {
		retry_cnt++;
		if (retry_cnt > 3) {
			hd_ret = HD_ERR_TIMEDOUT;
			HD_VIDEOOUT_ERR("Open videoout0 /dev/hdmi20 fail, retry %d...\n", retry_cnt);
			goto exit;
		}
		usleep(30 * 1000);
		goto re_try;
	}
	if ((ret = ioctl(hdmi_fd, HDMI_DEV_GET_EDID, &edid_info) < 0)) {
		HD_VIDEOOUT_ERR("ioctl:HDMI_DEV_GET_EDID fail,ret=%d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}
	if (p_edid_data->edid_data_len >= edid_info.u32Edidlength) {
		memcpy(p_edid_data->p_edid_data, edid_info.u8Edid, edid_info.u32Edidlength);
		p_edid_data->edid_data_len = edid_info.u32Edidlength;
	} else {
		HD_VIDEOOUT_ERR("Check user_edid_array_size(%d)<u32Edidlength(%d)\n", p_edid_data->edid_data_len, edid_info.u32Edidlength);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}
	memset(&hdmi_vid.vid, 0, MAX_VID_NU);
	if ((ret = ioctl(hdmi_fd, HDMI_DEV_GET_VID, &hdmi_vid) < 0)) {
		HD_VIDEOOUT_ERR("ioctl:HDMI_DEV_GET_VID fail,ret=%d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}
	if (p_edid_data->video_id_len >= MAX_VID_NU) {
		for (i = 0; i < MAX_VID_NU; i++) {
			if (hdmi_vid.vid[i] == 0)
				break;
			p_edid_data->p_video_id[i] = hdmi_vid.vid[i];
		//memcpy(p_edid_data->p_video_id, hdmi_vid.vid, MAX_VID_NU);
		}
		p_edid_data->video_id_len = i;
	} else {
		HD_VIDEOOUT_ERR("Check user_edid_array_size(%d)<u32Edidlength(%d)\n", p_edid_data->video_id_len, MAX_VID_NU);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}
exit:
	if (hdmi_fd >= 0)  {
		close(hdmi_fd);
	}
	return hd_ret;
}

HD_RESULT videoout_get_vga_edid_p(HD_VIDEOOUT_EDID_DATA *p_edid_data)
{
	INT i2c_fd = -1;
	INT tve_fd = -1;
	UINT8 i2c_addr = EDID_I2C_ADDR;
	struct tve_dev_ioct tve_ioct_i2c_id = {0};
	INT i2c_chan_id = 0;
	INT read_len = 0;
	CHAR i2c_path[64];

	tve_fd = open("/dev/tve100", O_RDWR);
	if (tve_fd < 0) {
		HD_VIDEOOUT_ERR("open /dev/tve100 fail\n");
		return  HD_ERR_SYS;
	}

	tve_ioct_i2c_id.data = -2;
	if(ioctl(tve_fd, VGA_DEV_GET_I2CID, &tve_ioct_i2c_id) < 0) {
		HD_VIDEOOUT_ERR("ioctl VGA_DEV_GET_I2CID(%d) fail\n", tve_ioct_i2c_id.data);
		close(tve_fd);
		return  HD_ERR_SYS;
	}
	if (tve_fd >= 0)  {
		close(tve_fd);
	}
	i2c_chan_id = tve_ioct_i2c_id.data;
	if (i2c_chan_id < 0) {
		HD_VIDEOOUT_ERR("Check i2c_chan_id=%d fail,please check VGA i2c config and HW is correct\n", i2c_chan_id);
		return  HD_ERR_NO_CONFIG;
	}
	sprintf(i2c_path, "/dev/i2c-%d", i2c_chan_id);
	i2c_fd = open(i2c_path, O_RDWR);
	if (i2c_fd < 0) {
		HD_VIDEOOUT_ERR("open %s fail\n", i2c_path);
		return  HD_ERR_FAIL;
	}
	if(ioctl(i2c_fd, I2C_SLAVE_FORCE, i2c_addr) < 0) {
		HD_VIDEOOUT_ERR("ioctl I2C_SLAVE_FORCE fail\n");
		close(i2c_fd);
		return  HD_ERR_FAIL;
	}
	read_len = read(i2c_fd, p_edid_data->p_edid_data, p_edid_data->edid_data_len);
	if (read_len == -1) {
		int errsv = errno;
		printf("Plsase check connect to VGA errsv=%d strerror:%s\n", errsv, strerror(errno));
	} else if(read_len != (INT)p_edid_data->edid_data_len) {
		HD_VIDEOOUT_ERR("Check read size fail, read_len=%d !edid_data_len=%d \n", read_len, p_edid_data->edid_data_len);
		return  HD_ERR_FAIL;
	} else
		p_edid_data->edid_data_len = read_len;
	if (i2c_fd >= 0)  {
		close(i2c_fd);
	}
	return HD_OK;
}

int get_lcd_fmt_by_videofmt(INT8 lcd_id, HD_VIDEO_PXLFMT pxlfmt)
{
	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);
	if (is_lcd300) {
		if ((platform_sys_Info.lcd_info[lcd_id].max_pixel_per_bits < 16) && pxlfmt == HD_VIDEO_PXLFMT_YUV422_ONE) {
			HD_VIDEOOUT_ERR("LCD%d max_pixel_per_bits=%d, not support yuv422(%#x) fmt\n",
					lcd_id, platform_sys_Info.lcd_info[lcd_id].max_pixel_per_bits, pxlfmt);
			return -1;
		}
		switch (pxlfmt) {
		case HD_VIDEO_PXLFMT_YUV422_ONE:
			return LCD300_INFMT_YUV422;
		case HD_VIDEO_PXLFMT_YUV422_NVX3:
			return LCD300_INFMT_YUV422_SCE;
		case HD_VIDEO_PXLFMT_YUV422_PLANAR:
			return LCD300_INFMT_YUV422_SCE;
		case HD_VIDEO_PXLFMT_YUV420:
			return LCD300_INFMT_YUV420;
		case HD_VIDEO_PXLFMT_YUV420_NVX3:
			return LCD300_INFMT_YUV420_SCE;
		case HD_VIDEO_PXLFMT_ARGB8888:
			return LCD300_INFMT_ARGB8888;
		case HD_VIDEO_PXLFMT_ARGB8888_RLE:
			return LCD300_INFMT_ARGB8888_RLE;
		case HD_VIDEO_PXLFMT_RGB888:
			return LCD300_INFMT_RGB888;
		case HD_VIDEO_PXLFMT_RGB888_RLE:
			return LCD300_INFMT_RGB888_RLE;
		case HD_VIDEO_PXLFMT_RGB565:
			return LCD300_INFMT_RGB565;
		case HD_VIDEO_PXLFMT_RGB565_RLE:
			return LCD300_INFMT_RGB565_RLE;
		case HD_VIDEO_PXLFMT_ARGB1555:
			return LCD300_INFMT_ARGB1555;
		case HD_VIDEO_PXLFMT_ARGB1555_RLE:
			return LCD300_INFMT_ARGB1555_RLE;
		case HD_VIDEO_PXLFMT_RGBA5551:
			return LCD300_INFMT_ARGB5551;
		case HD_VIDEO_PXLFMT_RGBA5551_RLE:
			return LCD300_INFMT_ARGB5551_RLE;
		case HD_VIDEO_PXLFMT_I1:
			return LCD300_INFMT_RGB1BPP;
		case HD_VIDEO_PXLFMT_I2:
			return LCD300_INFMT_RGB2BPP;
		case HD_VIDEO_PXLFMT_I4:
			return LCD300_INFMT_RGB4BPP;
		case HD_VIDEO_PXLFMT_I8:
			return LCD300_INFMT_RGB8BPP;
		default:
			HD_VIDEOOUT_ERR("Lcd id(%d),not support pxlfmt 0x%x to set\n", lcd_id, pxlfmt);
			return -1;
		}
	} else if (is_lcd200) {
		switch (pxlfmt) {
		case HD_VIDEO_PXLFMT_YUV422_ONE:
			return LCD200_INFMT_YUV422;
		case HD_VIDEO_PXLFMT_ARGB8888:
			return LCD200_INFMT_ARGB8888;
		case HD_VIDEO_PXLFMT_RGB888:
			return LCD200_INFMT_RGB888;
		case HD_VIDEO_PXLFMT_RGB888_RLE:
			return LCD300_INFMT_RGB888_RLE;
		case HD_VIDEO_PXLFMT_RGB565:
			return LCD200_INFMT_RGB565;
		case HD_VIDEO_PXLFMT_ARGB1555:
			return LCD200_INFMT_ARGB1555;
		case HD_VIDEO_PXLFMT_I1:
			return LCD200_INFMT_RGB1BPP;
		case HD_VIDEO_PXLFMT_I2:
			return LCD200_INFMT_RGB2BPP;
		case HD_VIDEO_PXLFMT_I4:
			return LCD200_INFMT_RGB4BPP;
		case HD_VIDEO_PXLFMT_I8:
			return LCD200_INFMT_RGB8BPP;
		default:
			HD_VIDEOOUT_ERR("Lcd id(%d),not support pxlfmt 0x%x to set\n", lcd_id, pxlfmt);
			return -1;
		}
	} else {
		HD_VIDEOOUT_ERR("Not support lcd id =%d\n", lcd_id);
	}
	return -1;
}

HD_VIDEO_PXLFMT get_videofmt_by_lcd_fmt(INT8 lcd_id, int lcd_fmt)
{
	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);

	if (is_lcd300) {
		switch (lcd_fmt) {
		case LCD300_INFMT_YUV422:
			return HD_VIDEO_PXLFMT_YUV422_ONE;
		case LCD300_INFMT_YUV422_DPCM:
			return HD_VIDEO_PXLFMT_YUV422_NVX3;
		case LCD300_INFMT_YUV422_SCE:
			return HD_VIDEO_PXLFMT_YUV422_PLANAR;
		case LCD300_INFMT_YUV420:
			return HD_VIDEO_PXLFMT_YUV420;
		case LCD300_INFMT_YUV420_SCE:
			return HD_VIDEO_PXLFMT_YUV420_NVX3;
		case LCD300_INFMT_ARGB8888:
			return HD_VIDEO_PXLFMT_ARGB8888;
		case LCD300_INFMT_ARGB8888_RLE:
			return HD_VIDEO_PXLFMT_ARGB8888_RLE;
		case LCD300_INFMT_RGB888:
			return HD_VIDEO_PXLFMT_RGB888;
		case LCD300_INFMT_RGB888_RLE:
			return HD_VIDEO_PXLFMT_RGB888_RLE;
		case LCD300_INFMT_RGB565:
			return HD_VIDEO_PXLFMT_RGB565;
		case LCD300_INFMT_RGB565_RLE:
			return HD_VIDEO_PXLFMT_RGB565_RLE;
		case LCD300_INFMT_ARGB1555:
			return HD_VIDEO_PXLFMT_ARGB1555;
		case LCD300_INFMT_ARGB1555_RLE:
			return HD_VIDEO_PXLFMT_ARGB1555_RLE;
		case LCD300_INFMT_ARGB5551:
			return HD_VIDEO_PXLFMT_RGBA5551;
		case LCD300_INFMT_ARGB5551_RLE:
			return HD_VIDEO_PXLFMT_RGBA5551_RLE;
		case LCD300_INFMT_RGB1BPP:
			return HD_VIDEO_PXLFMT_I1;
		case LCD300_INFMT_RGB2BPP:
			return HD_VIDEO_PXLFMT_I2;
		case LCD300_INFMT_RGB4BPP:
			return HD_VIDEO_PXLFMT_I4;
		case LCD300_INFMT_RGB8BPP:
			return HD_VIDEO_PXLFMT_I8;
		default:
			HD_VIDEOOUT_ERR("Lcd id(%d),not support lcd_fmt 0x%x to set\n", lcd_id, lcd_fmt);
			return HD_VIDEO_PXLFMT_NONE;
		}
	} else if (is_lcd200) {
		switch (lcd_fmt) {
		case LCD200_INFMT_YUV422:
			return HD_VIDEO_PXLFMT_YUV422_ONE;
		case LCD200_INFMT_ARGB8888:
			return HD_VIDEO_PXLFMT_ARGB8888;
		case LCD200_INFMT_RGB888:
			return HD_VIDEO_PXLFMT_RGB888;
		case LCD300_INFMT_RGB888_RLE:
			return HD_VIDEO_PXLFMT_RGB888_RLE;
		case LCD200_INFMT_RGB565:
			return HD_VIDEO_PXLFMT_RGB565;
		case LCD200_INFMT_ARGB1555:
			return HD_VIDEO_PXLFMT_ARGB1555;
		case LCD200_INFMT_RGB1BPP:
			return HD_VIDEO_PXLFMT_I1;
		case LCD200_INFMT_RGB2BPP:
			return HD_VIDEO_PXLFMT_I2;
		case LCD200_INFMT_RGB4BPP:
			return HD_VIDEO_PXLFMT_I4;
		case LCD200_INFMT_RGB8BPP:
			return HD_VIDEO_PXLFMT_I8;
		default:
			HD_VIDEOOUT_ERR("Lcd id(%d),not support lcd_fmt 0x%x to set\n", lcd_id, lcd_fmt);
			return HD_VIDEO_PXLFMT_NONE;
		}
	} else {
		HD_VIDEOOUT_ERR("Not support lcd id =%d\n", lcd_id);
	}
	return -1;
}

static int get_lcd310fmt_flag_by_lcdfmt(int lcdfmt)
{
	switch (lcdfmt) {
	case LCD300_INFMT_YUV422:
		return KFLOW_LCD310_IMG_FMT_YUV422;
	case LCD300_INFMT_YUV422_DPCM:
		return (KFLOW_LCD310_IMG_FMT_YUV422 | KFLOW_LCD310_IMG_FMT_DPCM);
	case LCD300_INFMT_YUV422_SCE:
		return (KFLOW_LCD310_IMG_FMT_YUV422 | KFLOW_LCD310_IMG_FMT_DPCM);
	case LCD300_INFMT_YUV420:
		return KFLOW_LCD310_IMG_FMT_YUV420;
	case LCD300_INFMT_YUV420_DPCM:
		return (KFLOW_LCD310_IMG_FMT_YUV420 | KFLOW_LCD310_IMG_FMT_DPCM);
	case LCD300_INFMT_YUV420_SCE:
		return (KFLOW_LCD310_IMG_FMT_YUV420 | KFLOW_LCD310_IMG_FMT_DPCM);
	case LCD300_INFMT_ARGB8888:
		return KFLOW_LCD310_IMG_FMT_ARGB8888;
	case LCD300_INFMT_ARGB8888_RLE:
		return (KFLOW_LCD310_IMG_FMT_ARGB8888 | KFLOW_LCD310_IMG_FMT_RLD);
	case LCD300_INFMT_RGB888:
		return KFLOW_LCD310_IMG_FMT_RGB888;
	case LCD300_INFMT_RGB888_RLE:
		return (KFLOW_LCD310_IMG_FMT_RGB888 | KFLOW_LCD310_IMG_FMT_RLD);
	case LCD300_INFMT_RGB565:
		return KFLOW_LCD310_IMG_FMT_RGB565;
	case LCD300_INFMT_RGB565_RLE:
		return (KFLOW_LCD310_IMG_FMT_RGB565 | KFLOW_LCD310_IMG_FMT_RLD);
	case LCD300_INFMT_ARGB1555:
		return KFLOW_LCD310_IMG_FMT_ARGB1555;
	case LCD300_INFMT_ARGB1555_RLE:
		return (KFLOW_LCD310_IMG_FMT_ARGB1555 | KFLOW_LCD310_IMG_FMT_RLD);
	case LCD300_INFMT_ARGB5551:
		return KFLOW_LCD310_IMG_FMT_ARGB1555;
	case LCD300_INFMT_ARGB5551_RLE:
		return (KFLOW_LCD310_IMG_FMT_ARGB1555 | KFLOW_LCD310_IMG_FMT_RLD);
	case LCD300_INFMT_RGB1BPP:
		return KFLOW_LCD310_IMG_FMT_1BIT;
	case LCD300_INFMT_RGB2BPP:
		return KFLOW_LCD310_IMG_FMT_2BIT;
	case LCD300_INFMT_RGB4BPP:
		return KFLOW_LCD310_IMG_FMT_4BIT;
	case LCD300_INFMT_RGB8BPP:
		return KFLOW_LCD310_IMG_FMT_8BIT;
	default:
		HD_VIDEOOUT_ERR("Unknown lcdfmt(%d)\n", lcdfmt);
		return -1;
	}
}

static int get_lcd210fmt_flag_by_lcdfmt(int lcdfmt)
{
	switch (lcdfmt) {
	case LCD200_INFMT_YUV422:
		return KFLOW_LCD210_IMG_FMT_YUV422;
	case LCD200_INFMT_ARGB8888:
		return KFLOW_LCD210_IMG_FMT_ARGB8888;
	case LCD200_INFMT_RGB888:
		return KFLOW_LCD210_IMG_FMT_RGB888;
	case LCD200_INFMT_RGB565:
		return KFLOW_LCD210_IMG_FMT_RGB565;
	case LCD200_INFMT_ARGB1555:
		return KFLOW_LCD210_IMG_FMT_ARGB1555;
	case LCD200_INFMT_RGB1BPP:
		return KFLOW_LCD210_IMG_FMT_1BIT;
	case LCD200_INFMT_RGB2BPP:
		return KFLOW_LCD210_IMG_FMT_2BIT;
	case LCD200_INFMT_RGB4BPP:
		return KFLOW_LCD210_IMG_FMT_4BIT;
	case LCD200_INFMT_RGB8BPP:
		return KFLOW_LCD210_IMG_FMT_8BIT;
	default:
		HD_VIDEOOUT_ERR("Unknown lcdfmt(%d)\n", lcdfmt);
		return -1;
	}
}

HD_RESULT videoout_set_fb_fmt(HD_FB_ID fb_id, INT8 lcd_id, HD_VIDEO_PXLFMT fmt)
{
	HD_RESULT ret = HD_OK;
	int lcd_fb_fmt;
	int lcd_fmt_flag = 0;
	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);

	if (is_lcd300) {
		lcd_fb_fmt = get_lcd_fmt_by_videofmt(lcd_id, fmt);
		if (lcd_fb_fmt < 0) {
			HD_VIDEOOUT_ERR("get lcd fmt 0x%x fail.\n", fmt);
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		lcd_fmt_flag = get_lcd310fmt_flag_by_lcdfmt(lcd_fb_fmt);
		if (lcd_fmt_flag < 0) {
			HD_VIDEOOUT_ERR("get lcd fmt flag, lcd_fb_fmt=%d fail.\n", lcd_fb_fmt);
			ret = HD_ERR_NOT_SUPPORT;
			goto err_exit;
		}
		if ((fb_id == 0) && (lcd_fmt_flag & ~(KFLOW_LCD310_PLANE0))) {
			HD_VIDEOOUT_ERR("Check videoout0:fb%d support fmt fail,support_fmt_flag(%#x).\n", fb_id, lcd_fmt_flag);
			ret = HD_ERR_NOT_SUPPORT;
			goto err_exit;
		} else if ((fb_id == 1) && (lcd_fmt_flag & ~(KFLOW_LCD310_PLANE1))) {
			HD_VIDEOOUT_ERR("Check videoout0:fb%d support fmt fail,support_fmt_flag(%#x).\n", fb_id, lcd_fmt_flag);
			ret = HD_ERR_NOT_SUPPORT;
			goto err_exit;
		} else if ((fb_id == 2) && (lcd_fmt_flag & ~(KFLOW_LCD310_PLANE2))) {
			HD_VIDEOOUT_ERR("Check videoout0:fb%d support fmt fail,support_fmt_flag(%#x).\n", fb_id, lcd_fmt_flag);
			ret = HD_ERR_NOT_SUPPORT;
			goto err_exit;
		}
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_SET_INFMT, &lcd_fb_fmt);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_SET_INFMT fail, fb_id=%d lcd_fb_fmt=%d.\n", fb_id, lcd_fb_fmt);
			ret = HD_ERR_SYS;
			goto err_exit;
		}
	} else if (is_lcd200) {

		lcd_fb_fmt = get_lcd_fmt_by_videofmt(lcd_id, fmt);
		if (lcd_fb_fmt < 0) {
			HD_VIDEOOUT_ERR("get lcd fmt 0x%x fail.\n", fmt);
			return HD_ERR_SYS;
		}
		lcd_fmt_flag = get_lcd210fmt_flag_by_lcdfmt(lcd_fb_fmt);
		if (lcd_fmt_flag < 0) {
			HD_VIDEOOUT_ERR("get lcd fmt flag, lcd_fb_fmt=%d fail.\n", lcd_fb_fmt);
			ret = HD_ERR_NOT_SUPPORT;
			goto err_exit;
		}
		if ((fb_id == 0) && (lcd_fmt_flag & ~(KFLOW_LCD210_PLANE0))) {
			HD_VIDEOOUT_ERR("Check videoout1:fb%d support fmt fail,support_fmt_flag(%#x).\n", fb_id, lcd_fmt_flag);
			ret = HD_ERR_NOT_SUPPORT;
			goto err_exit;
		} else if ((fb_id == 1) && (lcd_fmt_flag & ~(KFLOW_LCD210_PLANE1))) {
			HD_VIDEOOUT_ERR("Check videoout1:fb%d support fmt fail,support_fmt_flag(%#x).\n", fb_id, lcd_fmt_flag);
			ret = HD_ERR_NOT_SUPPORT;
			goto err_exit;
		}
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD200_IOC_SET_INFMT, &lcd_fb_fmt);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD200_IOC_SET_INFMT fail, fb_id=%d lcd_fb_fmt=%d.\n", fb_id, lcd_fb_fmt);
			ret = HD_ERR_SYS;
			goto err_exit;
		}
	} else {
		ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("Not support lcd id=%d to open\n", lcd_id);
	}

err_exit:
	return ret;
}

HD_RESULT videoout_get_fb_attr(HD_FB_ID fb_id, INT8 lcd_id, HD_FB_ATTR *p_fb_attrs)
{
	HD_RESULT ret = HD_OK;
	struct lcd300_alpha_argb1555 lcd300_pixel_alpha = {0};
	struct lcd300_blending lcd300_alpha_blend = {0};
	struct lcd200_blending lcd200_alpha_blend = {0};
	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);

	if (is_lcd300) {
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_GET_ALPHA_ARGB1555, &lcd300_pixel_alpha);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_GET_ALPHA_ARGB1555 fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		p_fb_attrs->alpha_1555 = lcd300_pixel_alpha.alpha1_value;

		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_GET_BLENDING, &lcd300_alpha_blend);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_GET_BLENDING fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		p_fb_attrs->alpha_blend = lcd300_alpha_blend.blend[fb_id];
		p_fb_attrs->colorkey_en = g_video_out_param[lcd_id].fb_param[fb_id].colorkey_en;
		p_fb_attrs->r_ckey = g_video_out_param[lcd_id].fb_param[fb_id].r_ckey;
		p_fb_attrs->g_ckey = g_video_out_param[lcd_id].fb_param[fb_id].g_ckey;
		p_fb_attrs->b_ckey = g_video_out_param[lcd_id].fb_param[fb_id].b_ckey;
	} else if (is_lcd200) {

		if (fb_id < 2) {
			memset(&lcd200_alpha_blend, 0, sizeof(struct lcd200_blending));
			ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD200_IOC_GET_BLENDING, &lcd200_alpha_blend);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("ioctl:LCD200_IOC_GET_BLENDING fail.\n");
				ret = HD_ERR_SYS;
				goto err_exit;
			}
			if (fb_id == 0) {
				p_fb_attrs->alpha_blend = lcd200_alpha_blend.blend_l;
			}
			if (fb_id == 1) {
				p_fb_attrs->alpha_blend = lcd200_alpha_blend.blend_h;
			}
			p_fb_attrs->colorkey_en = g_video_out_param[lcd_id].fb_param[fb_id].colorkey_en;
			p_fb_attrs->r_ckey = g_video_out_param[lcd_id].fb_param[fb_id].r_ckey;
			p_fb_attrs->g_ckey = g_video_out_param[lcd_id].fb_param[fb_id].g_ckey;
			p_fb_attrs->b_ckey = g_video_out_param[lcd_id].fb_param[fb_id].b_ckey;
		}

	} else {
		ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("Not support lcd id=%d to open\n", lcd_id);
	}

err_exit:
	return ret;
}

HD_RESULT videoout_set_fb_attr(HD_FB_ID fb_id, INT8 lcd_id, FB_PARAM *p_fb_param)
{
	HD_RESULT ret = HD_OK;
	struct lcd300_ckey lcd300_color_key;
	struct lcd300_alpha_argb1555 lcd300_pixel_alpha;
	struct lcd300_blending lcd300_alpha_blend;
	struct lcd200_ckey lcd200_color_key;
	struct lcd200_blending lcd200_alpha_blend;
	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);

	if (is_lcd300) {
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_GET_ALPHA_ARGB1555, &lcd300_pixel_alpha);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_GET_ALPHA_ARGB1555 fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		lcd300_pixel_alpha.alpha1_value = p_fb_param->alpha_1555;
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_SET_ALPHA_ARGB1555, &lcd300_pixel_alpha);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_SET_ALPHA_ARGB1555 fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}

		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_GET_BLENDING, &lcd300_alpha_blend);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_GET_BLENDING fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		lcd300_alpha_blend.blend[fb_id] = p_fb_param->alpha_blend;
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_SET_BLENDING, &lcd300_alpha_blend);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_SET_BLENDING fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		lcd300_color_key.enable = p_fb_param->colorkey_en;
		lcd300_color_key.fb_idx = fb_id;
		lcd300_color_key.ckey_r = p_fb_param->r_ckey;
		lcd300_color_key.ckey_g = p_fb_param->g_ckey;
		lcd300_color_key.ckey_b = p_fb_param->b_ckey;
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_SET_COLOR_KEY, &lcd300_color_key);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_SET_COLOR_KEY fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
	} else if (is_lcd200) {
		if (fb_id < 2) {
			memset(&lcd200_alpha_blend, 0, sizeof(struct lcd200_blending));
			ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD200_IOC_GET_BLENDING, &lcd200_alpha_blend);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("ioctl:LCD200_IOC_GET_BLENDING fail.\n");
				ret = HD_ERR_SYS;
				goto err_exit;
			}
			if (fb_id == 0) {
				lcd200_alpha_blend.blend_l = p_fb_param->alpha_blend;
			}
			if (fb_id == 1) {
				lcd200_alpha_blend.blend_h = p_fb_param->alpha_blend;
			}
			ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD200_IOC_SET_BLENDING, &lcd200_alpha_blend);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("ioctl:LCD200_IOC_SET_BLENDING fail.\n");
				ret = HD_ERR_SYS;
				goto err_exit;
			}
		}
		if (fb_id != 0) {
			lcd200_color_key.enable = p_fb_param->colorkey_en;
			lcd200_color_key.fb_idx = fb_id;
			lcd200_color_key.ckey_r = p_fb_param->r_ckey;
			lcd200_color_key.ckey_g = p_fb_param->g_ckey;
			lcd200_color_key.ckey_b = p_fb_param->b_ckey;
			ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD200_IOC_SET_COLOR_KEY, &lcd200_color_key);
			if (ret != HD_OK) {
				HD_VIDEOOUT_ERR("ioctl:LCD200_IOC_SET_COLOR_KEY fail.\n");
				ret = HD_ERR_SYS;
				goto err_exit;
			}
		}
	} else {
		ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("Not support lcd id=%d to open\n", lcd_id);
	}

err_exit:
	return ret;
}

HD_RESULT get_lcd_dim(HD_VIDEOOUT_INPUT_DIM lcd_dim, UINT32 *w, UINT32 *h)
{
	HD_RESULT ret = HD_OK;
	switch ((int)lcd_dim) {
	case HD_VIDEOOUT_IN_640x480:
		*w = 640;
		*h = 480;
		break;
	case HD_VIDEOOUT_IN_720x480:
		*w = 720;
		*h = 480;
		break;
	case HD_VIDEOOUT_IN_720x576:
		*w = 720;
		*h = 576;
		break;
    case VENDOR_VIDEOOUT_IN_1024x600:
        *w = 1024;
        *h = 600;
        break;
	case HD_VIDEOOUT_IN_1024x768:
		*w = 1024;
		*h = 768;
		break;
	case HD_VIDEOOUT_IN_800x600:
		*w = 800;
		*h = 600;
		break;
	case HD_VIDEOOUT_IN_1280x720:
		*w = 1280;
		*h = 720;
		break;
	case HD_VIDEOOUT_IN_2560x720:
		*w = 2560;
		*h = 720;
		break;
	case HD_VIDEOOUT_IN_2560x1440:
		*w = 2560;
		*h = 1440;
		break;
	case HD_VIDEOOUT_IN_1280x1024:
		*w = 1280;
		*h = 1024;
		break;
	case HD_VIDEOOUT_IN_1600x1200:
		*w = 1600;
		*h = 1200;
		break;
	case HD_VIDEOOUT_IN_1920x1080:
		*w = 1920;
		*h = 1080;
		break;
	case HD_VIDEOOUT_IN_3840x1080:
		*w = 3840;
		*h = 1080;
		break;
	case HD_VIDEOOUT_IN_3840x2160:
		*w = 3840;
		*h = 2160;
		break;
	case 14://VENDOR_VIDEOOUT_IN_1440x900
		*w = 1440;
		*h = 900;
		break;
	case 15://VENDOR_VIDEOOUT_IN_1680x1050
		*w = 1680;
		*h = 1050;
		break;
	case 16://VENDOR_VIDEOOUT_IN_1920x1200
		*w = 1920;
		*h = 1200;
		break;
	default:
		HD_VIDEOOUT_ERR("Not support lcd dim(%d)\n", lcd_dim);
		*w = 0;
		*h = 0;
		ret = HD_ERR_NOT_SUPPORT;
	}
	return ret;

}
HD_RESULT videoout_check_fb_dim(HD_FB_ID fb_id, INT8 lcd_id, HD_DIM input_dim, HD_URECT output_rect)
{
	struct lcd300_indim_otype lcd_desk;
	HD_VIDEOOUT_INPUT_DIM lcd_desk_dim;
	HD_DIM desk_w_h;
	HD_RESULT ret = HD_OK;
	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);

	if (is_lcd300) {
		if ((input_dim.w > output_rect.w) && ((input_dim.w / output_rect.w) > 4)) {
			HD_VIDEOOUT_ERR("lcd scaling down factor can't over 4,in_w(%d),out_w(%d)\n", input_dim.w, output_rect.w);
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}

		if ((input_dim.h > output_rect.h) && ((input_dim.h / output_rect.h) > 4)) {
			HD_VIDEOOUT_ERR("lcd scaling down factor can't over 4,in_h(%d),out_h(%d)\n", input_dim.h, output_rect.h);
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}

		if ((output_rect.w > input_dim.w) && ((output_rect.w / input_dim.w) > 4)) {
			HD_VIDEOOUT_ERR("lcd scaling up factor can't over 4,out_w(%d),in_w(%d)\n", output_rect.w, input_dim.w);
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}

		if ((output_rect.h > input_dim.h) && ((output_rect.h / input_dim.h) > 4)) {
			HD_VIDEOOUT_ERR("lcd scaling up factor can't over 4,out_h(%d),in_h(%d)\n", output_rect.h, input_dim.h);
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}
		memset(&lcd_desk, 0, sizeof(struct lcd300_indim_otype));
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_GET_INDIM_OTYPE, &lcd_desk);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_GET_INDIM_OTYPE fail.\n");
			ret = HD_ERR_SYS;
			goto exit;
		}
		lcd_desk_dim = get_lcd300_input_fmt_p(lcd_desk.indim);
		if (get_lcd_dim(lcd_desk_dim, &desk_w_h.w, &desk_w_h.h) != HD_OK) {
			HD_VIDEOOUT_ERR("get_lcd_dim fail=%x,lcd_desk_dim\n", lcd_desk_dim);
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}
		if (((output_rect.x + output_rect.w) > desk_w_h.w) || ((output_rect.y + output_rect.h) > desk_w_h.h)) {
			HD_VIDEOOUT_ERR("Check lcd dest boundary fail,out_rect(%d %d %d %d), is over lcd desk region(%dx%d)\n",
					output_rect.x, output_rect.y, output_rect.w, output_rect.h, desk_w_h.w, desk_w_h.h);
			ret = HD_ERR_NOT_SUPPORT;
			goto exit;
		}

	} else {
		HD_VIDEOOUT_ERR("videoout_check_fb_dim lcd_id[%d] not supported.\n", lcd_id);
		ret = HD_ERR_NOT_SUPPORT;
	}
exit:
	return ret;
}

HD_RESULT videoout_set_fb_dim(HD_FB_ID fb_id, INT8 lcd_id, HD_DIM input_dim, HD_URECT output_rect)
{
	HD_RESULT ret = HD_OK;
	struct lcd300_chan_scale lcd_fb_dim;
	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);

	if (is_lcd300) {
		lcd_fb_dim.plane_idx = fb_id;
		lcd_fb_dim.src_width = input_dim.w;
		lcd_fb_dim.src_height = input_dim.h;
		lcd_fb_dim.desk_x = output_rect.x;
		lcd_fb_dim.desk_y = output_rect.y;
		lcd_fb_dim.dst_width = output_rect.w;
		lcd_fb_dim.dst_height = output_rect.h;
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_SET_CHSCALE, &lcd_fb_dim);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_SET_CHSCALE fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
	} else {
		ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("videoout_set_fb_dim lcd_id[%d] not supported.\n", lcd_id);
	}
	if (pif_env_update() < 0) {
		HD_VIDEOOUT_ERR("ERR: pif_env_update fail.\n");
		ret = HD_ERR_NG;
	}
err_exit:
	return ret;
}

HD_RESULT videoout_get_fb_dim(HD_FB_ID fb_id, INT8 lcd_id, HD_DIM *input_dim, HD_URECT *output_rect)
{
	HD_RESULT ret = HD_OK;
	struct lcd300_chan_scale lcd_fb_dim;
	unsigned int is_lcd300 = 0, is_lcd200 = 0;
	struct lcd200_sysinfo lcd200_info;
	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);

	if (is_lcd300) {
		memset(&lcd_fb_dim, 0, sizeof(struct lcd300_chan_scale));
		lcd_fb_dim.plane_idx = fb_id;
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_GET_CHSCALE, &lcd_fb_dim);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_GET_CHSCALE fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		input_dim->w = lcd_fb_dim.src_width;
		input_dim->h = lcd_fb_dim.src_height;
		output_rect->x = lcd_fb_dim.desk_x;
		output_rect->y = lcd_fb_dim.desk_y;
		output_rect->w = lcd_fb_dim.dst_width;
		output_rect->h = lcd_fb_dim.dst_height;
	} else if (is_lcd200) {
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD200_IOC_GET_INFO, &lcd200_info);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD200_IOC_GET_INFO fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		input_dim->w = lcd200_info.idim_xres;
		input_dim->h = lcd200_info.idim_yres;
		output_rect->w = lcd200_info.odim_xres;
		output_rect->h = lcd200_info.odim_yres;
	} else {
		ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("videoout_get_fb_dim lcd_id[%d] not supported.\n", lcd_id);
	}
err_exit:
	return ret;
}

HD_RESULT videoout_get_fb_fmt(HD_FB_ID fb_id, INT8 lcd_id, HD_VIDEO_PXLFMT *fmt)
{
	HD_RESULT ret = HD_OK;
	int lcd_fb_fmt = 0;
	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);

	if (is_lcd300) {
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD300_IOC_GET_INFMT, &lcd_fb_fmt);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD300_IOC_GET_INFMT fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		*fmt = get_videofmt_by_lcd_fmt(lcd_id, lcd_fb_fmt);
		if (*fmt == HD_VIDEO_PXLFMT_NONE) {
			ret = HD_ERR_NOT_SUPPORT;
			goto err_exit;
		}
	} else if (is_lcd200) {
		ret = videoout_lcd_ioctl(lcd_id, fb_id, LCD200_IOC_GET_INFMT, &lcd_fb_fmt);
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("ioctl:LCD200_IOC_GET_INFMT fail.\n");
			ret = HD_ERR_SYS;
			goto err_exit;
		}
		*fmt = get_videofmt_by_lcd_fmt(lcd_id, lcd_fb_fmt);
		if (*fmt == HD_VIDEO_PXLFMT_NONE) {
			ret = HD_ERR_NOT_SUPPORT;
			goto err_exit;
		}
	} else {
		ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("videoout_get_fb_fmt lcd_id[%d] not supported.\n", lcd_id);
	}

err_exit:
	return ret;
}

HD_RESULT videoout_set_win_p(UINT8 lcd_idx, UINT8 win_idx, HD_VIDEOOUT_WIN_ATTR *win_attr)
{
	if (lcd_idx >= HD_VIDEOOUT_MAX_DEVICE_ID || lcd_idx < HD_VIDEOOUT_VDDO0) {
		HD_VIDEOOUT_ERR("Error lcd_idx(%d)\n", lcd_idx);
		return HD_ERR_NG;
	}
	lcd_cfg[lcd_idx].win[win_idx] = win_attr;
	return HD_OK;
}

HD_RESULT videoout_set_win_psr_p(UINT8 lcd_idx, UINT8 win_psr_idx, HD_VIDEOOUT_WIN_PSR_ATTR *win_psr_attr)
{
	if (lcd_idx >= HD_VIDEOOUT_MAX_DEVICE_ID || lcd_idx < HD_VIDEOOUT_VDDO0) {
		HD_VIDEOOUT_ERR("Error lcd_idx(%d)\n", lcd_idx);
		return HD_ERR_NG;
	}
	lcd_cfg[lcd_idx].win_psr[win_psr_idx] = win_psr_attr;
	return HD_OK;
}

HD_VIDEOOUT_INPUT_DIM get_lcd300_input_fmt_p(lcd300_drv_indim_t indim)
{
	HD_VIDEOOUT_INPUT_DIM lcd_dim = HD_VIDEOOUT_IN_AUTO;

	switch (indim) {
	case LCD300_INDIM_NTSC:
		lcd_dim = HD_VIDEOOUT_IN_720x480;
		break;
	case LCD300_INDIM_PAL:
		lcd_dim = HD_VIDEOOUT_IN_720x576;
		break;
	case LCD300_INDIM_640x480:
		lcd_dim = HD_VIDEOOUT_IN_640x480;
		break;
	case LCD300_INDIM_1024x768:
		lcd_dim = HD_VIDEOOUT_IN_1024x768;
		break;
    case LCD300_INDIM_1024x600:
		lcd_dim = VENDOR_VIDEOOUT_IN_1024x600;
        break;
	case LCD300_INDIM_1280x1024:
		lcd_dim = HD_VIDEOOUT_IN_1280x1024;
		break;
	case LCD300_INDIM_720P:
		lcd_dim = HD_VIDEOOUT_IN_1280x720;
		break;
	case LCD300_INDIM_2560x720:
		lcd_dim = HD_VIDEOOUT_IN_2560x720;
		break;
	case LCD300_INDIM_2560x1440:
		lcd_dim = HD_VIDEOOUT_IN_2560x1440;
		break;
	case LCD300_INDIM_1920x1080:
		lcd_dim = HD_VIDEOOUT_IN_1920x1080;
		break;
	case LCD300_INDIM_3840x1080:
		lcd_dim = HD_VIDEOOUT_IN_3840x1080;
		break;
	case LCD300_INDIM_3840x2160:
		lcd_dim = HD_VIDEOOUT_IN_3840x2160;
		break;
	case LCD300_INDIM_1600x1200:
		lcd_dim = HD_VIDEOOUT_IN_1600x1200;
		break;
	case LCD300_INDIM_1440x900:
		lcd_dim = HD_VIDEOOUT_IN_1440x900;
		break;
	case LCD300_INDIM_1680x1050:
		lcd_dim = HD_VIDEOOUT_IN_1680x1050;
		break;
	case LCD300_INDIM_1920x1200:
		lcd_dim = HD_VIDEOOUT_IN_1920x1200;
		break;
	case LCD300_INDIM_64x64:
		lcd_dim = HD_VIDEOOUT_IN_AUTO;
		break;
	default:
		HD_VIDEOOUT_ERR("LCD300 not support indim(%d).\n", indim);
		break;
	}
	return lcd_dim;
}

HD_VIDEOOUT_INPUT_DIM get_lcd200_input_fmt_p(lcd200_drv_indim_t indim)
{
	HD_VIDEOOUT_INPUT_DIM lcd_dim = HD_VIDEOOUT_IN_AUTO;

	switch (indim) {
	case LCD200_INDIM_NTSC:
		lcd_dim = HD_VIDEOOUT_IN_720x480;
		break;
	case LCD200_INDIM_PAL:
		lcd_dim = HD_VIDEOOUT_IN_720x576;
		break;
	case LCD200_INDIM_640x480:
		lcd_dim = HD_VIDEOOUT_IN_640x480;
		break;
	case LCD200_INDIM_1024x768:
		lcd_dim = HD_VIDEOOUT_IN_1024x768;
		break;
    case LCD200_INDIM_1024x600:
        lcd_dim = VENDOR_VIDEOOUT_IN_1024x600;
        break;
	case LCD200_INDIM_1280x1024:
		lcd_dim = HD_VIDEOOUT_IN_1280x1024;
		break;
	case LCD200_INDIM_720P:
		lcd_dim = HD_VIDEOOUT_IN_1280x720;
		break;
	case LCD200_INDIM_2560x1440:
		lcd_dim = HD_VIDEOOUT_IN_2560x1440;
		break;
	case LCD200_INDIM_1920x1080:
		lcd_dim = HD_VIDEOOUT_IN_1920x1080;
		break;
	case LCD200_INDIM_1600x1200:
		lcd_dim = HD_VIDEOOUT_IN_1600x1200;
		break;
	case LCD200_INDIM_64x64:
		lcd_dim = HD_VIDEOOUT_IN_AUTO;
		break;
	default:
		HD_VIDEOOUT_ERR("LCD200 not support indim(%d).\n", indim);
		break;
	}
	return lcd_dim;
}

HD_COMMON_VIDEO_OUT_TYPE get_lcd_output_type_p(lcd300_drv_otype_t otype)
{
	HD_COMMON_VIDEO_OUT_TYPE output_type = HD_COMMON_VIDEO_OUT_HDMI;

	switch (otype) {
	case LCD300_OTYPE_BT656_1280x720x30:
	case LCD300_OTYPE_BT656_1280x720x25:
		output_type = VENDOR_VIDEO_OUT_BT656;
		break;
	case LCD300_OTYPE_BT1120_1280x720x25:
	case LCD300_OTYPE_BT1120_1280x720x30:
    case LCD300_OTYPE_BT1120_1280x720x25V2:
    case LCD300_OTYPE_BT1120_1280x720x30V2:
	case LCD300_OTYPE_BT1120_1280x720x60:
	case LCD300_OTYPE_BT1120_1920x1080x60:
	case LCD300_OTYPE_BT1120_1280x1024x60:
	case LCD300_OTYPE_BT1120_1024x768x60:
	case LCD300_OTYPE_BT1120_1600x1200x60:
	case LCD300_OTYPE_BT1120_1440x900x60:
		output_type = VENDOR_VIDEO_OUT_BT1120;
		break;
	case LCD300_OTYPE_CVBS_NTSC:
	case LCD300_OTYPE_CVBS_PAL:
		output_type = HD_COMMON_VIDEO_OUT_CVBS;
		break;
	case LCD300_OTYPE_VGA_720x480x60:
	case LCD300_OTYPE_VGA_1024x768x60:
	case LCD300_OTYPE_VGA_1024x600x60:
	case LCD300_OTYPE_VGA_1280x720x60:
	case LCD300_OTYPE_VGA_1280x1024x60:
	case LCD300_OTYPE_VGA_1600x1200x60:
	case LCD300_OTYPE_VGA_1920x1080x60:
	case LCD300_OTYPE_VGA_1920x1200x60:
	case LCD300_OTYPE_VGA_1680x1050x60:
	case LCD300_OTYPE_VGA_1440x900x60:
	case LCD300_OTYPE_64x64:
		output_type = HD_COMMON_VIDEO_OUT_VGA;
		break;
	case LCD300_OTYPE_HDMI_720x480x60:
	case LCD300_OTYPE_HDMI_720x576x50:
	case LCD300_OTYPE_HDMI_1024x768x60:
	case LCD300_OTYPE_HDMI_1280x720x60:
	case LCD300_OTYPE_HDMI_1280x720x50:
	case LCD300_OTYPE_HDMI_1280x1024x60:
	case LCD300_OTYPE_HDMI_1600x1200x60:
	case LCD300_OTYPE_HDMI_1920x1080x60:
	case LCD300_OTYPE_HDMI_1920x1080x50:
	case LCD300_OTYPE_HDMI_1920x1080x30:
	case LCD300_OTYPE_HDMI_1920x1080x25:
	case LCD300_OTYPE_HDMI_1920x1080x24:
	case LCD300_OTYPE_HDMI_2560x1440x30:
	case LCD300_OTYPE_HDMI_2560x1440x60:
	case LCD300_OTYPE_HDMI_3840x2160x30:
	case LCD300_OTYPE_HDMI_1920x1200x60:
	case LCD300_OTYPE_HDMI_1680x1050x60:
	case LCD300_OTYPE_HDMI_1440x900x60:
	case LCD300_OTYPE_HDMI_3840x2160x60:
	case LCD300_OTYPE_HDMI_3840x2160x25:
		output_type = HD_COMMON_VIDEO_OUT_HDMI;
		break;
	default:
		HD_VIDEOOUT_ERR("otype(%d) HD_ERR_NOT_SUPPORT.\n", otype);
		break;
	}
	return output_type;
}

UINT get_lcd300_output_id_p(lcd300_drv_otype_t otype)
{
	UINT output_id = HD_COMMON_VIDEO_OUT_HDMI;

	switch (otype) {
	case LCD300_OTYPE_CVBS_NTSC:
		output_id = HD_VIDEOOUT_CVBS_NTSC;
		break;
	case LCD300_OTYPE_CVBS_PAL:
		output_id = HD_VIDEOOUT_CVBS_PAL;
		break;
	case LCD300_OTYPE_BT656_1280x720x30:
		output_id = VENDOR_VIDEOOUT_BT656_1280X720P30;
		break;
	case LCD300_OTYPE_BT656_1280x720x25:
		output_id = VENDOR_VIDEOOUT_BT656_1280X720P25;
		break;
	case LCD300_OTYPE_BT1120_1280x720x60:
		output_id = HD_VIDEOOUT_HDMI_1280X720P60;
		break;
	case LCD300_OTYPE_BT1120_1280x720x25:
		output_id =VENDOR_VIDEOOUT_BT1120_1280X720P25;
		break;
	case LCD300_OTYPE_BT1120_1280x720x30:
		output_id =VENDOR_VIDEOOUT_BT1120_1280X720P30;
		break;
    case LCD300_OTYPE_BT1120_1280x720x25V2:
        output_id =VENDOR_VIDEOOUT_BT1120_37M_1280X720P25;
        break;
    case LCD300_OTYPE_BT1120_1280x720x30V2:
        output_id =VENDOR_VIDEOOUT_BT1120_37M_1280X720P30;
        break;	
	case LCD300_OTYPE_BT1120_1920x1080x60:
		output_id = VENDOR_VIDEOOUT_BT1120_1920X1080P60;
		break;
    case LCD300_OTYPE_BT1120_1920x1080x50:
        output_id = VENDOR_VIDEOOUT_BT1120_1920X1080P50;
        break;
	case LCD300_OTYPE_BT1120_1280x1024x60:
		output_id = VENDOR_VIDEOOUT_BT1120_1280X1024P60;
		break;
	case LCD300_OTYPE_BT1120_1024x768x60:
		output_id = VENDOR_VIDEOOUT_BT1120_1024X768P60;
		break;
	case LCD300_OTYPE_BT1120_1600x1200x60:
		output_id = VENDOR_VIDEOOUT_BT1120_1600X1200P60;
		break;
    case LCD300_OTYPE_BT1120_1440x900x60:
        output_id = VENDOR_VIDEOOUT_BT1120_1440X900P60;
        break;
//VGA ID:#8
	case LCD300_OTYPE_VGA_720x480x60:
		output_id = HD_VIDEOOUT_VGA_720X480;
		break;
    case LCD300_OTYPE_VGA_1024x600x60:
        output_id = VENDOR_VIDEOOUT_VGA_1024X600;
        break;
	case LCD300_OTYPE_VGA_1024x768x60:
		output_id = HD_VIDEOOUT_VGA_1024X768;
		break;
	case LCD300_OTYPE_VGA_1280x720x60:
		output_id = HD_VIDEOOUT_VGA_1280X720;
		break;
	case LCD300_OTYPE_VGA_1280x1024x60:
		output_id = HD_VIDEOOUT_VGA_1280X1024;
		break;
	case LCD300_OTYPE_VGA_1600x1200x60:
		output_id = HD_VIDEOOUT_VGA_1600X1200;
		break;
	case LCD300_OTYPE_VGA_1680x1050x60:
		output_id = HD_VIDEOOUT_VGA_1600X1200;
		break;
	case LCD300_OTYPE_VGA_1920x1080x60:
		output_id = HD_VIDEOOUT_VGA_1920X1080;
		break;
	case LCD300_OTYPE_VGA_1440x900x60:
		output_id = HD_VIDEOOUT_VGA_1440X900;
		break;
	case LCD300_OTYPE_VGA_1920x1200x60:
		output_id = HD_VIDEOOUT_VGA_1920X1200;
		break;
//HDMI ID:#19
	case LCD300_OTYPE_HDMI_720x480x60:
		output_id = HD_VIDEOOUT_HDMI_720X480P60;
		break;
	case LCD300_OTYPE_HDMI_720x576x50:
		output_id = HD_VIDEOOUT_HDMI_720X576P50;
		break;
	case LCD300_OTYPE_HDMI_1280x720x60:
		output_id = HD_VIDEOOUT_HDMI_1280X720P60;
		break;
	case LCD300_OTYPE_HDMI_1280x720x50:
		output_id = HD_VIDEOOUT_HDMI_1280X720P50;
		break;
	case LCD300_OTYPE_HDMI_1280x1024x60:
		output_id = HD_VIDEOOUT_HDMI_1280X1024P30;
		break;
	case LCD300_OTYPE_HDMI_1600x1200x60:
		output_id = HD_VIDEOOUT_HDMI_1600X1200P60;
		break;
	case LCD300_OTYPE_HDMI_1920x1080x60:
		output_id = HD_VIDEOOUT_HDMI_1920X1080P60;
		break;
	case LCD300_OTYPE_HDMI_2560x1440x30:
		output_id = HD_VIDEOOUT_HDMI_2560X1440P30;
		break;
	case LCD300_OTYPE_HDMI_2560x1440x60:
		output_id = HD_VIDEOOUT_HDMI_2560X1440P60;
		break;
	case LCD300_OTYPE_HDMI_3840x2160x30://10
		output_id = HD_VIDEOOUT_HDMI_3840X2160P30;
		break;
	case LCD300_OTYPE_HDMI_1024x768x60:
		output_id = HD_VIDEOOUT_HDMI_1024X768P60;
		break;
	case LCD300_OTYPE_HDMI_1440x900x60:
		output_id = HD_VIDEOOUT_HDMI_1440X900P60;
		break;
	case LCD300_OTYPE_HDMI_1680x1050x60:
		output_id = HD_VIDEOOUT_HDMI_1680X1050P60;
		break;
	case LCD300_OTYPE_HDMI_1920x1200x60:
		output_id = HD_VIDEOOUT_HDMI_1920X1200P60;
		break;
	case LCD300_OTYPE_HDMI_1920x1080x50:
		output_id = HD_VIDEOOUT_HDMI_1920X1080P50;
		break;
	case LCD300_OTYPE_HDMI_1920x1080x25:
		output_id = HD_VIDEOOUT_HDMI_1920X1080P25;
		break;
	case LCD300_OTYPE_HDMI_1920x1080x30:
		output_id = HD_VIDEOOUT_HDMI_1920X1080P30;
		break;
	case LCD300_OTYPE_HDMI_1920x1080x24:
		output_id = HD_VIDEOOUT_HDMI_1920X1080P24;
		break;
	case LCD300_OTYPE_HDMI_3840x2160x25:
		output_id = HD_VIDEOOUT_HDMI_3840X2160P25;
		break;
	case LCD300_OTYPE_HDMI_3840x2160x60:
		output_id = CIF_HD_VIDEOOUT_HDMI_3840X2160P60;
		break;
	case LCD300_OTYPE_64x64:
		output_id = 0;
		break;
	default:
		HD_VIDEOOUT_ERR("LCD300 not support otype(%d).\n", otype);
		break;
	}
	return output_id;
}

UINT get_lcd200_output_id_p(lcd200_drv_otype_t otype)
{
	UINT output_id = HD_COMMON_VIDEO_OUT_HDMI;

	switch (otype) {
	case LCD200_OTYPE_CVBS_NTSC:
		output_id = HD_VIDEOOUT_CVBS_NTSC;
		break;
	case LCD200_OTYPE_CVBS_PAL:
		output_id = HD_VIDEOOUT_CVBS_PAL;
		break;
    case LCD200_OTYPE_VGA_1024x600x60:
        output_id = VENDOR_VIDEOOUT_VGA_1024X600;
        break;
	case LCD200_OTYPE_VGA_1024x768x60:
		output_id = HD_VIDEOOUT_VGA_1024X768;
		break;
	case LCD200_OTYPE_VGA_1280x720x60:
		output_id = HD_VIDEOOUT_VGA_1280X720;
		break;
	case LCD200_OTYPE_VGA_1280x1024x60:
		output_id = HD_VIDEOOUT_VGA_1280X1024;
		break;
	case LCD200_OTYPE_VGA_1600x1200x60:
		output_id = HD_VIDEOOUT_VGA_1600X1200;
		break;
	case LCD200_OTYPE_VGA_1920x1080x60:
		output_id = HD_VIDEOOUT_VGA_1920X1080;
		break;
	case LCD200_OTYPE_HDMI_720x480x60:
		output_id = HD_VIDEOOUT_HDMI_720X480P60;
		break;
	case LCD200_OTYPE_HDMI_720x576x50:
		output_id = HD_VIDEOOUT_HDMI_720X576I50;
		break;
	case LCD200_OTYPE_HDMI_1024x768x60:
		output_id = HD_VIDEOOUT_HDMI_1024X768P60;
		break;
	case LCD200_OTYPE_HDMI_1280x720x60:
		output_id = HD_VIDEOOUT_HDMI_1280X720P60;
		break;
	case LCD200_OTYPE_HDMI_1280x1024x60:
		output_id = HD_VIDEOOUT_HDMI_1280X1024P60;
		break;
	case LCD200_OTYPE_HDMI_1600x1200x60:
		output_id = HD_VIDEOOUT_HDMI_1600X1200P60;
		break;
	case LCD200_OTYPE_HDMI_1920x1080x60:
		output_id = HD_VIDEOOUT_HDMI_1920X1080P60;
		break;
	case LCD200_OTYPE_HDMI_2560x1440x30:
		output_id = HD_VIDEOOUT_HDMI_2560X1440P30;
		break;
	case LCD200_OTYPE_64x64:
		output_id = HD_VIDEOOUT_HDMI_NO_CHANGE;
		break;
	default:
		HD_VIDEOOUT_ERR("LCD200 not support otype(%d).\n", otype);
		break;
	}
	return output_id;
}

HD_RESULT videoout_get_lcd_dim(UINT8 lcd_id, HD_VIDEOOUT_MODE *lcd_mode)
{
	struct lcd300_indim_otype lcd300_indim_otype;
	struct lcd200_indim_otype lcd200_indim_otype;
	HD_RESULT ret = HD_OK;
	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_id, &is_lcd300, &is_lcd200);

	if (is_lcd300) {
		ret = videoout_lcd_ioctl(lcd_id, 0, LCD300_IOC_GET_INDIM_OTYPE, &lcd300_indim_otype);
		if (ret != HD_OK) {
			goto ext;
		}

		lcd_mode->input_dim = get_lcd300_input_fmt_p(lcd300_indim_otype.indim);
		lcd_mode->output_type = get_lcd_output_type_p(lcd300_indim_otype.otype);

		switch ((UINT32)lcd_mode->output_type) {
		case HD_COMMON_VIDEO_OUT_HDMI:
			lcd_mode->output_mode.hdmi = get_lcd300_output_id_p(lcd300_indim_otype.otype);
			break;
		case HD_COMMON_VIDEO_OUT_VGA:
			lcd_mode->output_mode.vga = get_lcd300_output_id_p(lcd300_indim_otype.otype);
			break;
		case HD_COMMON_VIDEO_OUT_CVBS:
			lcd_mode->output_mode.cvbs = get_lcd300_output_id_p(lcd300_indim_otype.otype);
			break;
		case VENDOR_VIDEO_OUT_BT1120:
               case VENDOR_VIDEO_OUT_BT656:
			lcd_mode->output_mode.lcd = get_lcd300_output_id_p(lcd300_indim_otype.otype);
			break;
		default:
			HD_VIDEOOUT_ERR("output_type(%d) HD_ERR_NOT_SUPPORT.\n", lcd_mode->output_type);
			break;
		}
	} else if (is_lcd200) {
		ret = videoout_lcd_ioctl(lcd_id, 0, LCD200_IOC_GET_INDIM_OTYPE, &lcd200_indim_otype);
		if (ret != HD_OK) {
			goto ext;
		}

		lcd_mode->input_dim = get_lcd200_input_fmt_p(lcd200_indim_otype.indim);
		lcd_mode->output_type = get_lcd_output_type_p(lcd200_indim_otype.otype);
		switch (lcd_mode->output_type) {
		case HD_COMMON_VIDEO_OUT_HDMI:
			lcd_mode->output_mode.hdmi = get_lcd200_output_id_p(lcd200_indim_otype.otype);
			break;
		case HD_COMMON_VIDEO_OUT_VGA:
			lcd_mode->output_mode.vga = get_lcd200_output_id_p(lcd200_indim_otype.otype);
			break;
		case HD_COMMON_VIDEO_OUT_CVBS:
			lcd_mode->output_mode.cvbs = get_lcd200_output_id_p(lcd200_indim_otype.otype);
			break;
		default:
			HD_VIDEOOUT_ERR("output_type(%d) HD_ERR_NOT_SUPPORT.\n", lcd_mode->output_type);
			break;
		}
		/* Virtual LCD */
	} else if ((lcd_id == HD_VIDEOOUT_VDDO3) ||
			   (lcd_id == HD_VIDEOOUT_VDDO4) ||
			   (lcd_id == HD_VIDEOOUT_VDDO5)) {
		lcd_mode->input_dim = HD_VIDEOOUT_IN_AUTO;
		lcd_mode->output_type = HD_COMMON_VIDEO_OUT_LCD;
		switch (lcd_id) {
		case HD_VIDEOOUT_VDDO3:
			lcd_mode->output_mode.lcd = HD_VIDEOOUT_LCD_2;
			break;
		case HD_VIDEOOUT_VDDO4:
			lcd_mode->output_mode.lcd = HD_VIDEOOUT_LCD_3;
			break;
		case HD_VIDEOOUT_VDDO5:
			lcd_mode->output_mode.lcd = HD_VIDEOOUT_LCD_4;
			break;
		default:
			HD_VIDEOOUT_ERR("lcd_idx(%d) HD_ERR_NOT_SUPPORT.\n", lcd_id);
			break;
		}
	} else {
		ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("videoout_get_lcd_dim lcd_id[%d] not supported.\n", lcd_id);
	}
ext:
	return ret;
}

lcd300_drv_indim_t videoout_get_lcd300_desk_res(HD_VIDEOOUT_INPUT_DIM input_dim)
{
//<lcd support in_dim_nu =15
	switch ((int)input_dim) {
	case HD_VIDEOOUT_IN_AUTO:
		return LCD300_INDIM_NTSC;
	case HD_VIDEOOUT_IN_640x480:
		return LCD300_INDIM_640x480;
	case HD_VIDEOOUT_IN_720x480:
		return LCD300_INDIM_NTSC;
	case HD_VIDEOOUT_IN_720x576:
		return LCD300_INDIM_PAL;
	case HD_VIDEOOUT_IN_1024x768:
		return LCD300_INDIM_1024x768;
	case HD_VIDEOOUT_IN_1280x720://5
		return LCD300_INDIM_720P;
	case HD_VIDEOOUT_IN_1280x1024:
		return LCD300_INDIM_1280x1024;
	case HD_VIDEOOUT_IN_1600x1200:
		return LCD300_INDIM_1600x1200;
	case HD_VIDEOOUT_IN_2560x720:
		return LCD300_INDIM_2560x720;
	case HD_VIDEOOUT_IN_2560x1440:
		return LCD300_INDIM_2560x1440;
	case HD_VIDEOOUT_IN_1920x1080://10
		return LCD300_INDIM_1920x1080;
	case HD_VIDEOOUT_IN_3840x1080:
		return LCD300_INDIM_3840x1080;
	case HD_VIDEOOUT_IN_3840x2160:
		return LCD300_INDIM_3840x2160;
	case HD_VIDEOOUT_IN_1440x900:
		return LCD300_INDIM_1440x900;
	case HD_VIDEOOUT_IN_1680x1050:
		return LCD300_INDIM_1680x1050;
	case HD_VIDEOOUT_IN_1920x1200:
		return LCD300_INDIM_1920x1200;
	case HD_VIDEOOUT_IN_800x600:
		return LCD300_INDIM_800x600;
    case VENDOR_VIDEOOUT_IN_1024x600:
        return LCD300_INDIM_1024x600;
	default:
		HD_VIDEOOUT_ERR("Not support input dim(%d)\n", input_dim);
		return LCD300_INDIM_MAX;
		break;
	}
	return LCD300_INDIM_MAX;
}

lcd200_drv_indim_t videoout_get_lcd200_desk_res(HD_VIDEOOUT_INPUT_DIM input_dim)
{

	switch (input_dim) {
	case HD_VIDEOOUT_IN_640x480:
		return LCD200_INDIM_640x480;
	case HD_VIDEOOUT_IN_720x480:
		return LCD200_INDIM_NTSC;
	case HD_VIDEOOUT_IN_720x576:
		return LCD200_INDIM_PAL;
    case VENDOR_VIDEOOUT_IN_1024x600:
        return LCD200_INDIM_1024x600;
	case HD_VIDEOOUT_IN_1024x768:
		return LCD200_INDIM_1024x768;
	case HD_VIDEOOUT_IN_1280x720:
		return LCD200_INDIM_720P;
	case HD_VIDEOOUT_IN_1280x1024:
		return LCD200_INDIM_1280x1024;
	case HD_VIDEOOUT_IN_1600x1200:
		return LCD200_INDIM_1600x1200;
	case HD_VIDEOOUT_IN_1920x1080:
		return LCD200_INDIM_1920x1080;
	case HD_VIDEOOUT_IN_2560x1440:
		return LCD200_INDIM_2560x1440;
	default:
		HD_VIDEOOUT_ERR("Not support input dim(%d)\n", input_dim);
		return LCD200_INDIM_MAX;
		break;
	}
	return LCD200_INDIM_MAX;
}

HD_RESULT videoout_set_lcd_dim(UINT8 lcd_idx, HD_VIDEOOUT_MODE *lcd_mode)
{
	struct lcd300_indim_otype indim_otype;
	HD_RESULT ret = HD_OK;

	unsigned int is_lcd300 = 0, is_lcd200 = 0;

	videoout_check_which_lcd(lcd_idx, &is_lcd300, &is_lcd200);

	if (check_videoout_exist(lcd_idx)) {
		HD_VIDEOOUT_ERR("ERROR: You should close all the videoout working to set lcd dim\n");
		return HD_ERR_NG;
	}

	if (is_lcd300) {
		if (HD_COMMON_VIDEO_OUT_HDMI == lcd_mode->output_type) {

			switch ((int)lcd_mode->output_mode.hdmi) {
			case HD_VIDEOOUT_HDMI_720X480P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_NTSC : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_720x480x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_720X576P50:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_PAL : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_720x576x50;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case CIF_HD_VIDEOOUT_HDMI_3840X2160P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_3840x2160 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_3840x2160x60;
                ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
                break;
			case HD_VIDEOOUT_HDMI_3840X2160P25:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_3840x2160 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_3840x2160x25;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_3840X2160P24:
			case HD_VIDEOOUT_HDMI_3840X2160P30:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_3840x2160 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_3840x2160x30;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_2560X1440P30://5
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_2560x1440 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_2560x1440x30;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_2560X1440P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_2560x1440 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_2560x1440x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1600X1200P60:
			case HD_VIDEOOUT_HDMI_1600X1200P30:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1600x1200 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1600x1200x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1920X1080P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1920x1080x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1920X1080P30:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1920x1080x30;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1920X1080P50://10
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1920x1080x50;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1920X1080P25:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1920x1080x25;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1920X1080P24:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1920x1080x24;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1280X1024P30:
			case HD_VIDEOOUT_HDMI_1280X1024P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1280x1024 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1280x1024x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1280X720P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1280x720x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1280X720P50://15
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1280x720x50;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1024X768P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1024x768 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1024x768x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1440X900P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1440x900 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1440x900x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1680X1050P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1680x1050 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1680x1050x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1920X1200P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1200 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_HDMI_1920x1200x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			default:
				HD_VIDEOOUT_ERR("lcd_idx(%d) hdmi(%d) HD_ERR_NOT_SUPPORT.\n", lcd_idx, lcd_mode->output_mode.hdmi);
				ret = HD_ERR_NOT_SUPPORT;
				break;
			}
		} else if (HD_COMMON_VIDEO_OUT_VGA == lcd_mode->output_type) {

			switch ((int)lcd_mode->output_mode.vga) {
			case HD_VIDEOOUT_VGA_720X480:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_NTSC : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_VGA_720x480x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_1280X720:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_VGA_1280x720x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_1280X1024:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1280x1024 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_VGA_1280x1024x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_1920X1080:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_VGA_1920x1080x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
            case VENDOR_VIDEOOUT_VGA_1024X600:
                indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1024x600 : \
                            videoout_get_lcd300_desk_res(lcd_mode->input_dim);
                indim_otype.otype = LCD300_OTYPE_VGA_1024x600x60;
                ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
                break;
			case HD_VIDEOOUT_VGA_1024X768:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1024x768 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_VGA_1024x768x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_1440X900://5
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1440x900 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_VGA_1440x900x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_1680X1050:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1680x1050 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_VGA_1680x1050x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_1600X1200:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1600x1200 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_VGA_1600x1200x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_1920X1200:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1200 : \
						    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_VGA_1920x1200x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			default:
				HD_VIDEOOUT_ERR("lcd_idx(%d) vga(%d) HD_ERR_NOT_SUPPORT.\n", lcd_idx, lcd_mode->output_mode.vga);
				ret = HD_ERR_NOT_SUPPORT;
				break;
			}
		} else if (VENDOR_VIDEO_OUT_BT1120 == (UINT32)lcd_mode->output_type) {
			switch ((int)lcd_mode->output_mode.lcd) {
			case VENDOR_VIDEOOUT_BT1120_1920X1080P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 :
									videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_BT1120_1920x1080x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
            case VENDOR_VIDEOOUT_BT1120_1920X1080P50:
                indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1920x1080 :
                                    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
                indim_otype.otype = LCD300_OTYPE_BT1120_1920x1080x50;
                ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
                break;
			case VENDOR_VIDEOOUT_BT1120_1280X720P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P :
									videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_BT1120_1280x720x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case VENDOR_VIDEOOUT_BT1120_1280X720P30:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P :
									videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_BT1120_1280x720x30;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case VENDOR_VIDEOOUT_BT1120_1280X720P25:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P :
									videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_BT1120_1280x720x25;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
            case VENDOR_VIDEOOUT_BT1120_37M_1280X720P30:
                indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P :
                                    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
                indim_otype.otype = LCD300_OTYPE_BT1120_1280x720x30V2;
                ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
                break;
            case VENDOR_VIDEOOUT_BT1120_37M_1280X720P25:
                indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P :
                                    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
                indim_otype.otype = LCD300_OTYPE_BT1120_1280x720x25V2;
                ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
                break;
			case VENDOR_VIDEOOUT_BT1120_1280X1024P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1280x1024 :
									videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_BT1120_1280x1024x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case VENDOR_VIDEOOUT_BT1120_1024X768P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1024x768 :
									videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_BT1120_1024x768x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;				
			case VENDOR_VIDEOOUT_BT1120_1600X1200P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1600x1200 :
									videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_BT1120_1600x1200x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case VENDOR_VIDEOOUT_BT1120_1440X900P60:
                indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_1440x900 :
                                    videoout_get_lcd300_desk_res(lcd_mode->input_dim);
                indim_otype.otype = LCD300_OTYPE_BT1120_1440x900x60;
                ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break; 
			default:
				HD_VIDEOOUT_ERR("lcd_idx(%d) vga(%d) HD_ERR_NOT_SUPPORT.\n", lcd_idx, lcd_mode->output_mode.lcd);
				ret = HD_ERR_NOT_SUPPORT;
				break;
			}
		}else if ( VENDOR_VIDEO_OUT_BT656 == (UINT32)lcd_mode->output_type) {
			switch ((int)lcd_mode->output_mode.lcd) {
			//<<For special timing case,define @	vendor_videoout.h:VENDOR_VIDEOOUT_BT_ID
			case VENDOR_VIDEOOUT_BT656_1280X720P30:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P : \
									videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_BT656_1280x720x30;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case VENDOR_VIDEOOUT_BT656_1280X720P25:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD300_INDIM_720P :
									videoout_get_lcd300_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD300_OTYPE_BT656_1280x720x25;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD300_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			default:
				HD_VIDEOOUT_ERR("lcd_idx(%d) vga(%d) HD_ERR_NOT_SUPPORT.\n", lcd_idx, lcd_mode->output_mode.lcd);
				ret = HD_ERR_NOT_SUPPORT;
				break;
			}
		}else {
			HD_VIDEOOUT_ERR("lcd_idx(%d) not support output_type(%d).\n", lcd_idx, lcd_mode->output_type);
			ret = HD_ERR_NOT_SUPPORT;
		}
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("check set lcd dim fail, lcd_idx=%d, output_type=%d, input_dim=%d\n", lcd_idx, lcd_mode->output_type,
					lcd_mode->input_dim);
		}
	} else if (is_lcd200) {
		if (HD_COMMON_VIDEO_OUT_HDMI == lcd_mode->output_type) {
			switch (lcd_mode->output_mode.hdmi) {
			case HD_VIDEOOUT_HDMI_720X480P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_NTSC : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_HDMI_720x480x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_720X576P50:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_PAL : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_HDMI_720x576x50;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1920X1080P60:
			case HD_VIDEOOUT_HDMI_1920X1080P30:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1920x1080 : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_HDMI_1920x1080x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1280X1024P30:
			case HD_VIDEOOUT_HDMI_1280X1024P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1280x1024 : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_HDMI_1280x1024x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1280X720P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_720P : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_HDMI_1280x720x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1024X768P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1024x768 : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_HDMI_1024x768x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_1600X1200P60:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1600x1200 : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_HDMI_1600x1200x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_HDMI_2560X1440P30:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_2560x1440 : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_HDMI_2560x1440x30;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			default:
				HD_VIDEOOUT_ERR("lcd_idx(%d) hdmi(%d) HD_ERR_NOT_SUPPORT.\n", lcd_idx, lcd_mode->output_mode.hdmi);
				ret = HD_ERR_NOT_SUPPORT;
				break;
			}
		} else if (HD_COMMON_VIDEO_OUT_VGA == lcd_mode->output_type) {
			switch (lcd_mode->output_mode.vga) {
			case HD_VIDEOOUT_VGA_1920X1080:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1920x1080 : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_VGA_1920x1080x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_1280X1024:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD200_INDIM_1280x1024 : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_VGA_1280x1024x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_1280X720:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_720P : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_VGA_1280x720x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
            case VENDOR_VIDEOOUT_VGA_1024X600:
                indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1024x600 : \
                            videoout_get_lcd200_desk_res(lcd_mode->input_dim);
                indim_otype.otype = LCD200_OTYPE_VGA_1024x600x60;
                ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
                break;
			case HD_VIDEOOUT_VGA_1024X768:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1024x768 : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_VGA_1024x768x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_1600X1200:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_1600x1200 : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_VGA_1600x1200x60;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_VGA_720X480:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_OTYPE_CVBS_NTSC : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_CVBS_NTSC;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			default:
				HD_VIDEOOUT_ERR("lcd_idx(%d) vga(%d) HD_ERR_NOT_SUPPORT.\n", lcd_idx, lcd_mode->output_mode.vga);
				ret = HD_ERR_NOT_SUPPORT;
				break;
			}
		}  else if (HD_COMMON_VIDEO_OUT_CVBS == lcd_mode->output_type) {
			switch (lcd_mode->output_mode.cvbs) {
			case HD_VIDEOOUT_CVBS_NTSC:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ?  LCD200_INDIM_NTSC : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_CVBS_NTSC;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			case HD_VIDEOOUT_CVBS_PAL:
				indim_otype.indim = (lcd_mode->input_dim == HD_VIDEOOUT_IN_AUTO) ? LCD200_INDIM_PAL : \
						    videoout_get_lcd200_desk_res(lcd_mode->input_dim);
				indim_otype.otype = LCD200_OTYPE_CVBS_PAL;
				ret = videoout_lcd_ioctl(lcd_idx, 0, LCD200_IOC_SET_INDIM_OTYPE, &indim_otype);
				break;
			default:
				HD_VIDEOOUT_ERR("lcd_idx(%d) cvbs(%d) HD_ERR_NOT_SUPPORT.\n", lcd_idx, lcd_mode->output_mode.cvbs);
				ret = HD_ERR_NOT_SUPPORT;
				break;
			}
		} else {
			HD_VIDEOOUT_ERR("lcd_idx(%d) not support output_type(%d).\n", lcd_idx, lcd_mode->output_type);
			ret = HD_ERR_NOT_SUPPORT;
		}
		if (ret != HD_OK) {
			HD_VIDEOOUT_ERR("check set lcd dim fail, lcd_idx=%d, output_type=%d, input_dim=%d\n", lcd_idx, lcd_mode->output_type,
					lcd_mode->input_dim);
		}
	} else {
		HD_VIDEOOUT_ERR("videoout_set_lcd_dim lcd_idx(%d) not support, output_type(%d).\n", lcd_idx, lcd_mode->output_type);
		ret = HD_ERR_NOT_SUPPORT;
	}
	if (ret == HD_OK) {
		if (pif_env_update() < 0) {
			HD_VIDEOOUT_ERR("ERR: pif_env_update fail.\n");
			ret = HD_ERR_NG;
		}
	}
	return ret;
}

HD_RESULT videoout_set_mode_p(UINT8 lcd_idx)
{
	HD_VIDEOOUT_MODE *lcd_mode;
	HD_RESULT ret = HD_OK;

	if (lcd_idx >= HD_VIDEOOUT_MAX_DEVICE_ID || lcd_idx < HD_VIDEOOUT_VDDO0) {
		HD_VIDEOOUT_ERR("Error lcd_idx(%d)\n", lcd_idx);
		return HD_ERR_NG;
	}
	if (check_videoout_exist(lcd_idx)) {
		HD_VIDEOOUT_ERR("ERROR: You should close all the videoout working to set lcd mode\n");
		return HD_ERR_NG;
	}
	lcd_mode = lcd_cfg[lcd_idx].mode;

	if (videoout_wait_clearwin_done_p(lcd_idx) != HD_OK) {
		HD_VIDEOOUT_ERR("ERROR: CLEARWIN is ongoing videoout%d should not to set mode\n", lcd_idx);
		return HD_ERR_NG;
	}

	if (lcd_idx >= HD_VIDEOOUT_VDDO0 && lcd_idx < MAX_VIDEOOUT_CTRL_ID) {
		ret = videoout_set_lcd_dim(lcd_idx, lcd_mode);
	} else {
		HD_VIDEOOUT_ERR("Only support lcd %d~%d to set mode, lcd_idx(%d)\n", HD_VIDEOOUT_VDDO0,
				HD_VIDEOOUT_VDDO2, lcd_idx);
		ret = HD_ERR_NOT_SUPPORT;
	}
	return ret;
}

HD_RESULT videoout_update_stamp_cfg(HD_IO out_id, HD_IO in_id, VDDO_PARAM *p_video_out_param, BOOL is_close)
{
	UINT8 i;
	//if stop max idx port, search enable max id as new max stamp idx
	if (GET_STAMP_ID(in_id, out_id) == p_video_out_param->max_stamp_idx) {
		for (i = GET_STAMP_ID(in_id, out_id); i > 0; i--) {
			if (p_video_out_param->stamp[GET_STAMP_ID(in_id, out_id)].win_enable) {
				p_video_out_param->max_stamp_idx = i;
				break;
			}
		}
	}
	if (is_close) {
		if (GET_STAMP_ID(in_id, out_id) >= HD_MAXVO_TOTAL_STAMP_NU) {
			HD_VIDEOOUT_ERR("Check stamp id(%d) fail, max(%d)\n", GET_STAMP_ID(in_id, out_id), HD_MAXVO_TOTAL_STAMP_NU);
			return HD_ERR_OVERRUN;
		}
		memset(&p_video_out_param->stamp[GET_STAMP_ID(in_id, out_id)], 0, sizeof(HD_VDDO_WIN_PARAM));
	}
	return HD_OK;
}

char* videoout_get_osg_type_name(HD_VIDEO_PXLFMT osg_type)
{
	switch(osg_type) {
		case HD_VIDEO_PXLFMT_ARGB1555:
			return "argb1555";
		case HD_VIDEO_PXLFMT_ARGB8888:
			return "argb8888";
		case HD_VIDEO_PXLFMT_ARGB4444:
			return "argb4444";
		case HD_VIDEO_PXLFMT_RGB565:
			return "rgb565";
		case HD_VIDEO_PXLFMT_I1:
			return "1bit_pal";
		case HD_VIDEO_PXLFMT_I2:
			return "2bit_pal";
		case HD_VIDEO_PXLFMT_I4:
			return "4bit_pal";
		default:
			return "unknown";
	}
}

int convert_hdal_videoout_fmt_to_osg_fmt(HD_VIDEO_PXLFMT fmt)
{
	switch (fmt) {
	case HD_VIDEO_PXLFMT_ARGB1555:
		return 0;
	case HD_VIDEO_PXLFMT_ARGB8888:
		return 1;
	case HD_VIDEO_PXLFMT_I1:
		return 2;
	case HD_VIDEO_PXLFMT_I2:
		return 3;
	case HD_VIDEO_PXLFMT_I4:
		return 4;
	default :
		HD_VIDEOOUT_ERR("Not suppot osg src fmt(%#x)\n", fmt);
		return -1;
	}
}

HD_RESULT videoout_set_osg_win(INT osg_idx, HD_VDDO_WIN_PARAM *win_info, UINT16 max_win_idx)
{
	HD_RESULT hd_ret = HD_OK;
	struct multi_osg_window_info_t osg_win = {0};
	struct osg_window_info_t *osg_window_info = NULL;
	INT ret;
	UINT16 i;
	UINT16 win_nu = max_win_idx + 1;
	UINT16 osg_win_idx = 0;
	int osg_fmt = 0;
	int chip = COMMON_PCIE_CHIP_RC;

	if (osg_idx < MAX_VIDEOOUT_CTRL_ID) {
		chip = COMMON_PCIE_CHIP_RC;
	} else {
		chip = osg_idx - MAX_VIDEOOUT_CTRL_ID;
	}
	if (chip < 0) {
		HD_VIDEOOUT_ERR("%s: chip(%d) < 0, osg_idx(%d)!", __FUNCTION__, chip, osg_idx);
		hd_ret = HD_ERR_INV;
		goto err_ext;
	}
	if (osg_ioctl <= 0) {
		HD_VIDEOOUT_ERR("%s: Error, no osg ioctl supported!", __FUNCTION__);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	osg_window_info = (struct osg_window_info_t *)PIF_MALLOC(win_nu * sizeof(struct osg_window_info_t));
	if (!osg_window_info) {
		HD_VIDEOOUT_ERR("malloc fail\n");
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}

	osg_win.fd =  pif_alloc_user_fd_for_hdal(VPD_TYPE_OSG, chip, 0, utl_get_disp_osg_index(osg_idx));
	g_video_out_param[osg_idx].osg_fd = osg_win.fd;
	for (i = 0; i < win_nu; i++) {
		if (!win_info[i].win_enable)
			continue;
		osg_window_info[osg_win_idx].osg_idx = osg_win_idx;
		osg_window_info[osg_win_idx].enable = win_info[i].win_enable;
		osg_window_info[osg_win_idx].st_disp.fg_alpha = win_info[i].attr.alpha;
		osg_window_info[osg_win_idx].st_disp.bg_alpha = 0;
		osg_window_info[osg_win_idx].st_disp.align_type = win_info[i].attr.align_type;
		osg_window_info[osg_win_idx].st_disp.pos_x = win_info[i].attr.position.x;
		osg_window_info[osg_win_idx].st_disp.pos_y = win_info[i].attr.position.y;
		if (win_info[i].win_enable) {
			if ((osg_fmt = convert_hdal_videoout_fmt_to_osg_fmt(win_info[i].img.fmt)) < 0) {
				HD_VIDEOOUT_ERR("convert_hdal_videoout_fmt_to_osg_fmt fail\n");
				hd_ret = HD_ERR_NOT_SUPPORT;
				goto err_ext;
			}
			osg_window_info[osg_win_idx].st_grap.type = osg_fmt;
		} else {
			osg_window_info[osg_win_idx].st_grap.type = 0;
		}
		osg_window_info[osg_win_idx].st_grap.width = win_info[i].img.dim.w;
		osg_window_info[osg_win_idx].st_grap.height = win_info[i].img.dim.h;
		osg_window_info[osg_win_idx].st_grap.pa_addr = win_info[i].img.p_addr;
		osg_window_info[osg_win_idx].st_grap.source_img_ddr_id = win_info[i].img.ddr_id;
		//
		if (win_info[i].attr.colorkey_en == 1) {
			osg_window_info[osg_win_idx].colorkey.enable = 1;
			osg_window_info[osg_win_idx].colorkey.pal_sel = win_info[i].attr.colorkey_val;
		} else {
			osg_window_info[osg_win_idx].colorkey.enable = 0;
			osg_window_info[osg_win_idx].colorkey.pal_sel = 0;
		}
		osg_window_info[osg_win_idx].colorkey.ref_alpha = 0;
		osg_win_idx++;
	}
	osg_win.num_osg_window = osg_win_idx;
	osg_win.osg_window_info = osg_window_info;
	if (osg_win_idx == 0) {
		osg_win.num_osg_window = 1;
		osg_window_info[osg_win_idx].enable = 0;
		osg_win.osg_window_info = osg_window_info;
	}
	if (osg_win.osg_window_info) {
		ret = ioctl(osg_ioctl, IOCTL_OSG_SET_MULTI_WINDOW, &osg_win); //notify fd invalid
		if (ret < 0) {
			HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_SET_MULTI_WINDOW\" return %d,win_nu(%d)\n", ret, win_nu);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}
err_ext:
	if (osg_window_info) {
		PIF_FREE(osg_window_info);
	}
	return hd_ret;
}

HD_RESULT videoout_set_osg_stamp_palette_tbl(UINT32 *pal_tbl, INT tbl_sz)
{
	HD_RESULT hd_ret = HD_OK;
	int ret = 0;
	struct osg_graph_palette_table_t graph_pal_tbl = {0};
	int i = 0;

	if ((tbl_sz < 0) || (tbl_sz > OSG_GRAPH_PALETTE_MAX_NUM)) {
		HD_VIDEOOUT_ERR("%s: Error, tbl_sz(%d) > OSG_GRAPH_PALETTE_MAX_NUM(%d)!", __FUNCTION__, tbl_sz, OSG_GRAPH_PALETTE_MAX_NUM);
		hd_ret = HD_ERR_NOT_SUPPORT;
		goto err_ext;
	}
	if (osg_ioctl <= 0) {
		HD_VIDEOOUT_ERR("%s: Error, no osg ioctl supported!", __FUNCTION__);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	ret = ioctl(osg_ioctl, IOCTL_OSG_GET_GRAPH_PALETTE_TABLE, &graph_pal_tbl); //notify fd invalid
	if (ret < 0) {
		HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_GET_GRAPH_PALETTE_TABLE\" return %d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	for (i = 0 ; i < tbl_sz; i++) {
		graph_pal_tbl.tbl[i].a = (pal_tbl[i] >> 24 & 0xff);
		graph_pal_tbl.tbl[i].r = (pal_tbl[i] >> 16 & 0xff);
		graph_pal_tbl.tbl[i].g = (pal_tbl[i] >> 8 & 0xff);
		graph_pal_tbl.tbl[i].b = (pal_tbl[i] & 0xff);
	}
	ret = ioctl(osg_ioctl, IOCTL_OSG_SET_GRAPH_PALETTE_TABLE, &graph_pal_tbl); //notify fd invalid
	if (ret < 0) {
		HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_SET_GRAPH_PALETTE_TABLE\" return %d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
err_ext:

	return hd_ret;
}

HD_RESULT videoout_set_osg_mask_palette_tbl(UINT32 *pal_tbl, INT tbl_sz)
{
	HD_RESULT hd_ret = HD_OK;
	int ret = 0;
	struct osg_mask_palette_table_t mask_pal_tbl = {0};
	int i = 0;

	if ((tbl_sz < 0) || (tbl_sz > OSG_MASK_PALETTE_MAX_NUM)) {
		HD_VIDEOOUT_ERR("%s: Error, tbl_sz(%d) > OSG_MASK_PALETTE_MAX_NUM(%d)!", __FUNCTION__, tbl_sz, OSG_MASK_PALETTE_MAX_NUM);
		hd_ret = HD_ERR_NOT_SUPPORT;
		goto err_ext;
	}
	if (osg_ioctl <= 0) {
		HD_VIDEOOUT_ERR("%s: Error, no osg ioctl supported!", __FUNCTION__);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	ret = ioctl(osg_ioctl, IOCTL_OSG_GET_MASK_PALETTE_TABLE, &mask_pal_tbl); //notify fd invalid
	if (ret < 0) {
		HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_GET_MASK_PALETTE_TABLE\" return %d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	for (i = 0 ; i < tbl_sz; i++) {
		mask_pal_tbl.tbl[i].y0 = (pal_tbl[i] >> 24 & 0xff);
		mask_pal_tbl.tbl[i].y1 = (pal_tbl[i] >> 16 & 0xff);
		mask_pal_tbl.tbl[i].cb = (pal_tbl[i] >> 8 & 0xff);
		mask_pal_tbl.tbl[i].cr = (pal_tbl[i] & 0xff);
	}
	ret = ioctl(osg_ioctl, IOCTL_OSG_SET_MASK_PALETTE_TABLE, &mask_pal_tbl); //notify fd invalid
	if (ret < 0) {
		HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_SET_MASK_PALETTE_TABLE\" return %d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
err_ext:

	return hd_ret;
}

HD_RESULT videoout_get_osg_stamp_palette_tbl(UINT32 *pal_tbl, INT tbl_sz)
{
	HD_RESULT hd_ret = HD_OK;
	int ret = 0;
	struct osg_graph_palette_table_t graph_pal_tbl = {0};
	int i = 0;

	if ((tbl_sz < 0) || (tbl_sz > OSG_GRAPH_PALETTE_MAX_NUM)) {
		HD_VIDEOOUT_ERR("%s: Error, tbl_sz(%d) > OSG_GRAPH_PALETTE_MAX_NUM(%d)!", __FUNCTION__, tbl_sz, OSG_GRAPH_PALETTE_MAX_NUM);
		hd_ret = HD_ERR_NOT_SUPPORT;
		goto err_ext;
	}
	if (osg_ioctl <= 0) {
		HD_VIDEOOUT_ERR("%s: Error, no osg ioctl supported!", __FUNCTION__);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	ret = ioctl(osg_ioctl, IOCTL_OSG_GET_GRAPH_PALETTE_TABLE, &graph_pal_tbl); //notify fd invalid
	if (ret < 0) {
		HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_GET_GRAPH_PALETTE_TABLE\" return %d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	for (i = 0 ; i < tbl_sz; i++) {
		pal_tbl[i] = ((graph_pal_tbl.tbl[i].a & 0xff) << 24 |
			      (graph_pal_tbl.tbl[i].r & 0xff) << 16 |
			      (graph_pal_tbl.tbl[i].g & 0xff) << 8 |
			      (graph_pal_tbl.tbl[i].b & 0xff)) ;
	}
err_ext:
	return hd_ret;
}

HD_RESULT videoout_get_osg_mask_palette_tbl(UINT32 *pal_tbl, INT tbl_sz)
{
	HD_RESULT hd_ret = HD_OK;
	int ret = 0;
	struct osg_mask_palette_table_t mask_pal_tbl = {0};
	int i = 0;

	if ((tbl_sz < 0) || (tbl_sz > OSG_MASK_PALETTE_MAX_NUM)) {
		HD_VIDEOOUT_ERR("%s: Error, tbl_sz(%d) > OSG_MASK_PALETTE_MAX_NUM(%d)!", __FUNCTION__, tbl_sz, OSG_MASK_PALETTE_MAX_NUM);
		hd_ret = HD_ERR_NOT_SUPPORT;
		goto err_ext;
	}
	if (osg_ioctl <= 0) {
		HD_VIDEOOUT_ERR("%s: Error, no osg ioctl supported!", __FUNCTION__);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	ret = ioctl(osg_ioctl, IOCTL_OSG_GET_MASK_PALETTE_TABLE, &mask_pal_tbl); //notify fd invalid
	if (ret < 0) {
		HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_GET_MASK_PALETTE_TABLE\" return %d\n", ret);
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	for (i = 0 ; i < tbl_sz; i++) {
		mask_pal_tbl.tbl[i].y0 = (pal_tbl[i] >> 24 & 0xff);
		mask_pal_tbl.tbl[i].y1 = (pal_tbl[i] >> 16 & 0xff);
		mask_pal_tbl.tbl[i].cb = (pal_tbl[i] >> 8 & 0xff);
		mask_pal_tbl.tbl[i].cr = (pal_tbl[i] & 0xff);
	}
err_ext:

	return hd_ret;
}

HD_RESULT videoout_update_mask_cfg(HD_IO out_id, HD_IO in_id, VDDO_PARAM *p_video_out_param)
{
	UINT8 i;

	if (GET_MASK_ID(in_id, out_id) == p_video_out_param->max_mask_idx) {
		for (i = GET_MASK_ID(in_id, out_id); i > 0; i--) {
			if (p_video_out_param->mask_mosaic[GET_MASK_ID(in_id, out_id)].enable) {
				p_video_out_param->max_mask_idx = i;
				break;
			}
		}
	}
	//memset(&p_video_out_param->mask_mosaic[in_id], 0, sizeof(HD_VDDO_MASK_MOSAIC));
	return HD_OK;
}

HD_RESULT videoout_set_mask_mosaic(UINT32 osg_idx, HD_VDDO_MASK_MOSAIC *mask_mosaic_param, UINT16 max_mask_idx)
{
	HD_RESULT hd_ret = HD_OK;

	struct multi_osg_mask_mosaic_win_info_t osg_mask_mosaic = {0};
	struct osg_mask_mosaic_win_t *mask_mosaic_win;
	INT16 i;
	UINT16 mask_nu = max_mask_idx + 1;
	UINT16 mask_win_idx = 0;
	INT j;
	INT ret = 0;
	unsigned int osg_fd;
	int chip = COMMON_PCIE_CHIP_RC;

	if (osg_idx < MAX_VIDEOOUT_CTRL_ID) {
		chip = COMMON_PCIE_CHIP_RC;
	} else {
		chip = osg_idx - MAX_VIDEOOUT_CTRL_ID;
	}
	if (chip < 0) {
		HD_VIDEOOUT_ERR("%s: chip(%d) < 0, osg_idx(%d)!", __FUNCTION__, chip, osg_idx);
		hd_ret = HD_ERR_INV;
		return hd_ret;
	}

	osg_fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_OSG, chip, 0, utl_get_disp_osg_index(osg_idx));

	mask_mosaic_win = (struct osg_mask_mosaic_win_t *)PIF_MALLOC(mask_nu * sizeof(struct osg_mask_mosaic_win_t));
	if (!mask_mosaic_win) {
		HD_VIDEOOUT_ERR("malloc fail\n");
		hd_ret = HD_ERR_SYS;
		goto err_ext;
	}
	osg_mask_mosaic.fd = osg_fd;

	for (i = 0; i < mask_nu; i++) {
		if (!mask_mosaic_param[i].enable)
			continue;
		mask_mosaic_win[mask_win_idx].mask_idx = mask_win_idx;
		mask_mosaic_win[mask_win_idx].enable = mask_mosaic_param[i].enable;
		if (mask_mosaic_param[i].is_mask) {
			mask_mosaic_win[mask_win_idx].mosaic_en = 0;
			mask_mosaic_win[mask_win_idx].alpha = mask_mosaic_param[i].mask_cfg.alpha;
			if (mask_mosaic_win[mask_win_idx].alpha >= 256) {
				mask_mosaic_win[mask_win_idx].alpha = 255;
			}
			for (j = 0; j < 4; j++) {
				mask_mosaic_win[mask_win_idx].st_pos[j].pos_x = mask_mosaic_param[i].mask_cfg.position[j].x;
				mask_mosaic_win[mask_win_idx].st_pos[j].pos_y = mask_mosaic_param[i].mask_cfg.position[j].y;
			}
			if (mask_mosaic_param[i].mask_cfg.type == HD_OSG_MASK_TYPE_SOLID) {
				mask_mosaic_win[mask_win_idx].line_hit_opt = 0;
			} else {
				mask_mosaic_win[mask_win_idx].line_hit_opt = 1;
                if (mask_mosaic_param[i].mask_cfg.thickness <= 3) {//0/1/2/3(maapping to 2/4/6/8 pixels)
                    mask_mosaic_win[mask_win_idx].mask_border_sz = mask_mosaic_param[i].mask_cfg.thickness;
                } else {
                    mask_mosaic_win[mask_win_idx].mask_border_sz = 2;
                }
			}           
			mask_mosaic_win[mask_win_idx].pal_sel = mask_mosaic_param[i].mask_cfg.color;
		} else {
			mask_mosaic_win[mask_win_idx].mosaic_en = 1;
			mask_mosaic_win[mask_win_idx].alpha = mask_mosaic_param[i].mosaic_cfg.alpha;
			if (mask_mosaic_win[mask_win_idx].alpha >= 256) {
				mask_mosaic_win[mask_win_idx].alpha = 255;
			}
			for (j = 0; j < 4; j++) {
				mask_mosaic_win[mask_win_idx].st_pos[j].pos_x = mask_mosaic_param[i].mosaic_cfg.position[j].x;
				mask_mosaic_win[mask_win_idx].st_pos[j].pos_y = mask_mosaic_param[i].mosaic_cfg.position[j].y;
			}
			if (mask_mosaic_param[i].mosaic_cfg.type == HD_OSG_MASK_TYPE_SOLID) {
				mask_mosaic_win[mask_win_idx].line_hit_opt = 0;
			} else {
				mask_mosaic_win[mask_win_idx].line_hit_opt = 1;
			}
			mask_mosaic_win[mask_win_idx].pal_sel = 0;
			if (mask_mosaic_param[i].enable && mask_mosaic_param[i].mosaic_cfg.mosaic_blk_w != mask_mosaic_param[i].mosaic_cfg.mosaic_blk_h) {
				HD_VIDEOOUT_ERR("Check mosaic blk_w(%d) != blk_h(%d)\n", mask_mosaic_param[i].mosaic_cfg.mosaic_blk_w,
						mask_mosaic_param[i].mosaic_cfg.mosaic_blk_h);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
			if (mask_mosaic_param[i].enable && (mask_mosaic_param[i].mosaic_cfg.mosaic_blk_w != 8) && (mask_mosaic_param[i].mosaic_cfg.mosaic_blk_w != 16) \
			    && (mask_mosaic_param[i].mosaic_cfg.mosaic_blk_w != 32) && (mask_mosaic_param[i].mosaic_cfg.mosaic_blk_w != 64)) {
				HD_VIDEOOUT_ERR("Not support mosaic blk(%d) \n", mask_mosaic_param[i].mosaic_cfg.mosaic_blk_w);
				hd_ret = HD_ERR_SYS;
				goto err_ext;
			}
			mask_mosaic_win[mask_win_idx].mosaic_blk_sz.w = mask_mosaic_param[i].mosaic_cfg.mosaic_blk_w;
			mask_mosaic_win[mask_win_idx].mosaic_blk_sz.h = mask_mosaic_param[i].mosaic_cfg.mosaic_blk_h;
		}
		mask_mosaic_win[mask_win_idx].mask_border_sz = 2;
		mask_mosaic_win[mask_win_idx].align_type = OSG_ALIGN_TOP_LEFT;
		////
		mask_win_idx++;
	}
	osg_mask_mosaic.num_osg_mask_mosaic = mask_win_idx;
	osg_mask_mosaic.st_mask_mosaic_win = mask_mosaic_win;
	if (mask_win_idx == 0) {
		osg_mask_mosaic.num_osg_mask_mosaic = 1;
		mask_mosaic_win[mask_win_idx].enable = 0;
		osg_mask_mosaic.st_mask_mosaic_win = mask_mosaic_win;
	}
	if (mask_nu) {
		ret = ioctl(osg_ioctl, IOCTL_OSG_SET_MULTI_MASK, &osg_mask_mosaic); //notify fd invalid
		if (ret < 0) {
			HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_SET_MULTI_MASK\" return %d,mask_nu(%d)\n", ret, mask_nu);
			hd_ret = HD_ERR_SYS;
			goto err_ext;
		}
	}
err_ext:
	if (mask_mosaic_win) {
		PIF_FREE(mask_mosaic_win);
	}

	return hd_ret;

}

HD_RESULT videoout_push_in_buf_to_osg(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_in_video_frame, HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	usr_proc_trigger_osg_t usr_proc_trigger_osg;
	INT32 dev_idx = VO_DEVID(self_id);
	VDDO_PARAM *p_lcd_info = &g_video_out_param[dev_idx];
	VDDO_INFO_PRIV *p_lcd_priv = NULL;
	UINT8 osg_idx = g_video_out_param[dev_idx].osg_idx;
	INT32 ret;
	CHAR pool_name[32]; //fill trigger parameters according to user parameters
	int chip = osg_idx - MAX_VIDEOOUT_CTRL_ID;

	if (ctrl_id == HD_CTRL) {
		p_lcd_priv = &p_lcd_info->priv_ctrl;
	} else {
		p_lcd_priv = &p_lcd_info->priv[VO_INID(in_id)];
	}
	if (chip < 0) {
		HD_VIDEOOUT_ERR("%s: chip(%d) < 0, osg_idx(%d)!", __FUNCTION__, chip, osg_idx);
		hd_ret = HD_ERR_INV;
		return hd_ret;
	}
	usr_proc_trigger_osg.fd = pif_alloc_user_fd_for_hdal(VPD_TYPE_OSG, chip, 0, utl_get_disp_osg_index(osg_idx));
	usr_proc_trigger_osg.in_frame_buffer.ddr_id = p_in_video_frame->ddr_id;
	usr_proc_trigger_osg.in_frame_buffer.addr_pa = p_in_video_frame->phy_addr[0];
	if (pif_address_ddr_sanity_check(usr_proc_trigger_osg.in_frame_buffer.addr_pa, usr_proc_trigger_osg.in_frame_buffer.ddr_id) < 0) {
		HD_VIDEOENC_ERR("%s:Check in_pa(%#lx) ddrid(%d) error\n", __func__, usr_proc_trigger_osg.in_frame_buffer.addr_pa,
			usr_proc_trigger_osg.in_frame_buffer.ddr_id);
		return HD_ERR_PARAM;
	}
	usr_proc_trigger_osg.in_frame_buffer.size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_in_video_frame->dim, p_in_video_frame->pxlfmt);
	pool_name[0] = VIDEOOUT_MEM_POOL_NAME;
	sprintf(usr_proc_trigger_osg.in_frame_buffer.pool_name, pool_name);
	usr_proc_trigger_osg.in_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_in_video_frame->pxlfmt);
	usr_proc_trigger_osg.in_frame_info.dim.w = p_in_video_frame->dim.w;
	usr_proc_trigger_osg.in_frame_info.dim.h = p_in_video_frame->dim.h;
	usr_proc_trigger_osg.in_frame_info.pathid = p_in_video_frame->reserved[0];
	usr_proc_trigger_osg.direction = 0; //
	usr_proc_trigger_osg.queue_handle = p_lcd_priv->queue_handle;
	usr_proc_trigger_osg.p_user_data = (void *)(p_user_out_video_frame ? p_user_out_video_frame->blk : 0);
	if ((ret = ioctl(vpd_fd, USR_PROC_TRIGGER_OSG, &usr_proc_trigger_osg)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -2) {
			HD_VIDEOOUT_ERR("path(%#x) in_frame_buffer(phy_addr[0]=%p) is still in use."
					"Please 'pull' this buffer before push it.\n",
					path_id, (VOID *)p_in_video_frame->phy_addr[0]);
		}
		HD_VIDEOOUT_ERR("<ioctl \"USR_PROC_TRIGGER_OSG\" fail, error(%d)>\n", ret);
		hd_ret = HD_ERR_SYS;
	}
	return hd_ret;
}

HD_RESULT videoout_pull_out_buf_from_osg(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_in_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	INT32 ret = 0;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	usr_proc_osg_cb_info_t usr_proc_cb_osg;
	INT32 dev_idx = VO_DEVID(self_id);
	VDDO_PARAM *p_lcd_info = &g_video_out_param[dev_idx];
	VDDO_INFO_PRIV *p_lcd_priv = NULL;

	if (ctrl_id == HD_CTRL) {
		p_lcd_priv = &p_lcd_info->priv_ctrl;
	} else {
		p_lcd_priv = &p_lcd_info->priv[VO_INID(in_id)];
	}
	memset(&usr_proc_cb_osg, 0, sizeof(usr_proc_osg_cb_info_t));
	usr_proc_cb_osg.queue_handle = p_lcd_priv ->queue_handle;
	usr_proc_cb_osg.wait_ms = wait_ms;
	if ((ret = ioctl(vpd_fd, USR_PROC_RECV_DATA_OSG, &usr_proc_cb_osg)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		HD_VIDEOOUT_ERR("<ioctl \"USR_PROC_RECV_DATA_OSG\" fail, error(%d)>\n", ret);
		hd_ret = HD_ERR_SYS;
		goto exit;
	}
	if (usr_proc_cb_osg.status == 1)  { //1:job finished, 0:ongoing, -1:fail
		if (p_in_video_frame) {
			p_in_video_frame->ddr_id = p_in_video_frame->ddr_id;
			p_in_video_frame->phy_addr[0] = usr_proc_cb_osg.addr_pa;
			if (pif_address_ddr_sanity_check(p_in_video_frame->phy_addr[0], p_in_video_frame->ddr_id) < 0) {
				HD_VIDEOOUT_ERR("%s:Check  pull_out pa(%#lx) ddrid(%d) error\n", __func__, p_in_video_frame->phy_addr[0], p_in_video_frame->ddr_id);
				hd_ret = HD_ERR_PARAM;
				goto exit;
			}
			p_in_video_frame->blk = (typeof(p_in_video_frame->blk)) usr_proc_cb_osg.p_user_data;
		}
		hd_ret = HD_OK;
	} else {
		hd_ret = HD_ERR_FAIL;
	}
exit:
	return hd_ret;
}

VDDO_INTERNAL_PARAM *videoout_get_param_p(HD_DAL self_id)
{
	if (VO_DEVID(self_id) > HD_VIDEOOUT_MAX_DEVICE_ID || VO_DEVID(self_id) < HD_VIDEOOUT_VDDO0) {
		HD_VIDEOOUT_ERR("Error self_id(%d)\n", self_id);
		goto err_ext;
	}
	return &lcd_cfg[VO_DEVID(self_id)];
err_ext:
	return NULL;
}

HD_RESULT videoout_get_devcount(HD_DEVCOUNT *p_devcount)
{
	p_devcount->max_dev_count = HD_VIDEOOUT_MAX_DEVICE_ID;;
	return HD_OK;
}

HD_RESULT videoout_wait_clearwin_done_p(int lcd_vch)
{
	int ret;
	int wait_cnt = 0;
	vpd_get_clr_win_t clrwin_st;

wait:
	wait_cnt++;
	clrwin_st.lcd_vch = lcd_vch;
	clrwin_st.clrwin_stat = -1;// 0:not ready 1:ready 2:apply setup

	ret = ioctl(vpd_fd, VPD_GET_CLEAR_STATE, &clrwin_st);
	if (ret < 0) {
		HD_VIDEOOUT_ERR("ioctl \"VPD_GET_CLEAR_STATE\" return %d\n", ret);
		return HD_ERR_NG;
	}

	if (clrwin_st.clrwin_stat != 1 && wait_cnt < WAIT_CLRWIN_CNT) {
		usleep(TIMEOUTVAL);
		HD_VIDEOOUT_FLOW("clrwin_stat=%d,wait_cnt=%d\n", clrwin_st.clrwin_stat, wait_cnt);
		goto wait;
	}
	if (wait_cnt < WAIT_CLRWIN_CNT) {
		return HD_OK;
	} else {
		HD_VIDEOOUT_ERR("wait clearwin %dms timeout,clrwin_stat=%d\n", (WAIT_CLRWIN_CNT * 100), clrwin_st.clrwin_stat);
		return HD_ERR_TIMEDOUT;
	}
}

HD_RESULT videoout_clearwin_p(UINT8 lcd_idx, HD_VIDEOOUT_CLEAR_WIN *clear_win)
{
	vpd_clr_win_t vpd_cw_str;
	HD_RESULT ret = HD_OK;

	vpd_cw_str.in_width = clear_win->input_dim.w;
	vpd_cw_str.in_height = clear_win->input_dim.h;
	if (clear_win->in_fmt == HD_VIDEO_PXLFMT_YUV420) {
		vpd_cw_str.in_fmt = VPD_BUFTYPE_YUV420_SP;
	} else {
		vpd_cw_str.in_fmt = VPD_BUFTYPE_YUV422;
	}
	vpd_cw_str.in_buf = clear_win->buf;
	vpd_cw_str.out_x = clear_win->output_rect.x;
	vpd_cw_str.out_y = clear_win->output_rect.y;
	vpd_cw_str.out_width = clear_win->output_rect.w;
	vpd_cw_str.out_height = clear_win->output_rect.h;

	if (((vpd_cw_str.out_x + vpd_cw_str.out_width) > (int)platform_sys_Info.lcd_info[lcd_idx].desk_res.width) ||
	    ((vpd_cw_str.out_y + vpd_cw_str.out_height) > (int)platform_sys_Info.lcd_info[lcd_idx].desk_res.height)) {
		HD_VIDEOOUT_ERR("clearwin (%d,%d)/(%d,%d) over region (%d,%d)\n",
				vpd_cw_str.out_x, vpd_cw_str.out_y, vpd_cw_str.out_width, vpd_cw_str.out_height,
				platform_sys_Info.lcd_info[lcd_idx].desk_res.width, platform_sys_Info.lcd_info[lcd_idx].desk_res.height);
		return HD_ERR_NG;
	}

	if (((vpd_cw_str.out_x + vpd_cw_str.out_width) > (int)platform_sys_Info.lcd_info[lcd_idx].fb0_win.width) ||
	    ((vpd_cw_str.out_y + vpd_cw_str.out_height) > (int)platform_sys_Info.lcd_info[lcd_idx].fb0_win.height)) {
		HD_VIDEOOUT_ERR("clearwin (%d,%d)/(%d,%d) over fb0_win (%d,%d)\n",
				vpd_cw_str.out_x, vpd_cw_str.out_y, vpd_cw_str.out_width, vpd_cw_str.out_height,
				platform_sys_Info.lcd_info[lcd_idx].fb0_win.width, platform_sys_Info.lcd_info[lcd_idx].fb0_win.height);
		return HD_ERR_NG;
	}

	switch (clear_win->mode) {
	case HD_ACTIVE_BY_APPLY:
		vpd_cw_str.mode = VPD_ACTIVE_BY_APPLY;
		break;
	case HD_ACTIVE_IMMEDIATELY:
		vpd_cw_str.mode = VPD_ACTIVE_IMMEDIATELY;
		break;
	default:
		HD_VIDEOOUT_ERR("clr_win_mode=%d error!\n", clear_win->mode);
		ret = HD_ERR_NOT_SUPPORT;
		goto ext;
	}
	vpd_cw_str.lcd_vch = lcd_idx;
	ret = ioctl(vpd_fd, VPD_CLEAR_WINDOW, &vpd_cw_str);
	if (ret < 0) {
		HD_VIDEOOUT_ERR("ioctl \"VPD_CLEAR_WINDOW\" return %d\n", ret);
		ret = HD_ERR_NG;
	}
ext:
	return ret;
}


HD_RESULT videoout_new_in_buf(HD_DAL self_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT buf_size, ddr_id;
	CHAR pool_name[32];
	VOID *addr_pa;

	buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_video_frame->dim, p_video_frame->pxlfmt);
	if (buf_size == 0) {
		HD_VIDEOOUT_ERR("%s, get buf_size(%d) failed.\n", __func__, buf_size);
		hd_ret = HD_ERR_NG;
		goto exit;
	}

	ddr_id = p_video_frame->ddr_id;
	pool_name[0] = VIDEOOUT_MEM_POOL_NAME;
	addr_pa = pif_alloc_video_buffer_for_hdal(buf_size, ddr_id, pool_name, USR_LIB);
	if (addr_pa == NULL) {
		HD_VIDEOOUT_ERR("%s, alloca out_buf failed.\n", __func__);
		hd_ret = HD_ERR_NOMEM;
		goto exit;
	}
	p_video_frame->phy_addr[0] = (UINTPTR) addr_pa;

exit:
	return hd_ret;
}

HD_RESULT videoout_push_in_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_in_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_trigger_lcd_t usr_proc_trigger_lcd;
	INT32 ret, buf_size;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	INT32 dev_idx = VO_DEVID(self_id);
	VDDO_PARAM *p_lcd_info = &g_video_out_param[dev_idx];
	VDDO_INFO_PRIV *p_lcd_priv = NULL;

	if (ctrl_id == HD_CTRL) {
		p_lcd_priv = &p_lcd_info->priv_ctrl;
	} else {
		p_lcd_priv = &p_lcd_info->priv[VO_INID(in_id)];
	}
	if (p_in_video_frame->pxlfmt != common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(platform_sys_Info.lcd_info[dev_idx].lcd_fmt)) {
		HD_VIDEOOUT_ERR("Check videoout_push_in_buf fmt fail, pushin_fmt(%#x)!=vpd_fmt(%#x)\n", p_in_video_frame->pxlfmt, \
				common_convert_vpd_buffer_type_to_HD_VIDEO_PXLFMT(platform_sys_Info.lcd_info[dev_idx].lcd_fmt));
		return HD_ERR_NOT_ALLOW;
	}
	//fill trigger parameters according to user parameters
	buf_size = common_calculate_buf_size_by_HD_VIDEO_PXLFMT(p_in_video_frame->dim, p_in_video_frame->pxlfmt);
	usr_proc_trigger_lcd.fd = p_lcd_priv->fd;
	usr_proc_trigger_lcd.in_frame_buffer.ddr_id = p_in_video_frame->ddr_id;
	usr_proc_trigger_lcd.in_frame_buffer.addr_pa = p_in_video_frame->phy_addr[0];
	if (pif_address_ddr_sanity_check(usr_proc_trigger_lcd.in_frame_buffer.addr_pa, usr_proc_trigger_lcd.in_frame_buffer.ddr_id) < 0) {
		HD_VIDEOOUT_ERR("%s:Check in_pa(%#lx) ddrid(%d) error\n", __func__, usr_proc_trigger_lcd.in_frame_buffer.addr_pa,  usr_proc_trigger_lcd.in_frame_buffer.ddr_id);
		return HD_ERR_PARAM;
	}
	usr_proc_trigger_lcd.in_frame_buffer.size = buf_size;
	usr_proc_trigger_lcd.in_frame_info.type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_in_video_frame->pxlfmt);
	usr_proc_trigger_lcd.in_frame_info.dim.w = p_in_video_frame->dim.w;
	usr_proc_trigger_lcd.in_frame_info.dim.h = p_in_video_frame->dim.h;

	//TODO: fill values
	usr_proc_trigger_lcd.lcd_param.lcd_ch = dev_idx;
	usr_proc_trigger_lcd.lcd_param.src_fps = platform_sys_Info.lcd_info[dev_idx].fps;
	if ((p_in_video_frame->reserved[1] > 0) &&(p_in_video_frame->reserved[1] <= 60)) {
		usr_proc_trigger_lcd.lcd_param.src_fps = p_in_video_frame->reserved[1];
	}
	usr_proc_trigger_lcd.lcd_param.src_fmt = (p_in_video_frame->pxlfmt == HD_VIDEO_PXLFMT_YUV422_ONE) ? VPD_BUFTYPE_YUV422 : VPD_BUFTYPE_YUV420_SP;
	usr_proc_trigger_lcd.lcd_param.src_xy = 0;
	usr_proc_trigger_lcd.lcd_param.src_dim.w = usr_proc_trigger_lcd.in_frame_info.dim.w;
	usr_proc_trigger_lcd.lcd_param.src_dim.h = usr_proc_trigger_lcd.in_frame_info.dim.h;
	usr_proc_trigger_lcd.lcd_param.src_bg_dim.w = usr_proc_trigger_lcd.in_frame_info.dim.w;
	usr_proc_trigger_lcd.lcd_param.src_bg_dim.h = usr_proc_trigger_lcd.in_frame_info.dim.h;
	usr_proc_trigger_lcd.lcd_param.disp1_dim.w = 0;
	usr_proc_trigger_lcd.lcd_param.disp1_dim.h = 0;
	usr_proc_trigger_lcd.lcd_param.disp1_xy = 0;
	usr_proc_trigger_lcd.lcd_param.feature = 1;
	if (p_in_video_frame->reserved[2] == 1) {
		usr_proc_trigger_lcd.is_cb_put_queue = 1;
	} else {
		usr_proc_trigger_lcd.is_cb_put_queue = 0;
	}

	usr_proc_trigger_lcd.queue_handle = p_lcd_priv->queue_handle;
	usr_proc_trigger_lcd.p_user_data = (void *)123; //pass private data if need
	usr_proc_trigger_lcd.in_frame_info.pathid = p_in_video_frame->reserved[0];
	if ((ret = ioctl(vpd_fd, USR_PROC_TRIGGER_LCD, &usr_proc_trigger_lcd)) < 0) {
		int errsv = errno;
		ret = errsv * -1;
		if (ret == -2) {
			HD_VIDEOOUT_ERR("path(%#x) in_frame_buffer(phy_addr[0]=%p) is still in use."
					"Please 'pull' this buffer before push it.\n",
					path_id, (VOID *)p_in_video_frame->phy_addr[0]);
		}
		HD_VIDEOOUT_ERR("<ioctl \"USR_PROC_TRIGGER_LCD\" fail, error(%d)>\n", ret);
		hd_ret = HD_ERR_SYS;
	}
	return hd_ret;
}

HD_RESULT videoout_release_in_buf(HD_DAL self_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT ddr_id, ret;
	VOID *addr_pa;

	addr_pa = (VOID *) p_video_frame->phy_addr[0];
	ddr_id = p_video_frame->ddr_id;

	ret = pif_free_video_buffer_for_hdal(addr_pa, ddr_id, USR_LIB);
	if (ret < 0) {
		HD_VIDEOOUT_ERR("%s, free buffer(pa:%p) failed.\n", __func__, addr_pa);
		hd_ret = HD_ERR_NG;
	}
	return hd_ret;
}

HD_RESULT videoout_pull_out_buf(HD_PATH_ID path_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms)
{
	HD_RESULT hd_ret = HD_OK;
	usr_proc_lcd_cb_info_t lcd_cb_info;
	HD_DAL self_id = HD_GET_DEV(path_id);
	HD_IO in_id = HD_GET_IN(path_id);
	HD_IO ctrl_id = HD_GET_CTRL(path_id);
	INT32 dev_idx = VO_DEVID(self_id);
	VDDO_PARAM *p_lcd_info = &g_video_out_param[dev_idx];
	VDDO_INFO_PRIV *p_lcd_priv = NULL;
	int ret;

	if (VO_DEVID(self_id) >= HD_VIDEOOUT_MAX_DEVICE_ID || VO_DEVID(self_id) < HD_VIDEOOUT_VDDO0) {
		HD_VIDEOOUT_ERR("Error self_id(%#x)\n", self_id);
		hd_ret = HD_ERR_NG;
		goto exit;
	}
	if (ctrl_id == HD_CTRL) {
		p_lcd_priv = &p_lcd_info->priv_ctrl;
	} else {
		p_lcd_priv = &p_lcd_info->priv[VO_INID(in_id)];
	}
	hd_ret = dif_pull_out_buffer(self_id, in_id, p_video_frame, wait_ms);
	if (hd_ret == HD_OK) {
		if (p_video_frame->phy_addr[0] == 0) {
			HD_VIDEOOUT_ERR("dif_pull_out_buffer fail\n");
			hd_ret = HD_ERR_NG;
			goto exit;
		}
	} else if (hd_ret == HD_ERR_NOT_AVAIL) {
		if (p_lcd_priv->queue_handle == 0) {
			HD_VIDEOOUT_ERR("output queue is not ready. self_id(%#x)\n", self_id);
			hd_ret = HD_ERR_NG;
			goto exit;
		}
		memset(&lcd_cb_info, 0, sizeof(usr_proc_lcd_cb_info_t));
		lcd_cb_info.queue_handle = p_lcd_priv->queue_handle;
		lcd_cb_info.wait_ms = wait_ms;
		if ((ret = ioctl(vpd_fd, USR_PROC_RECV_DATA_LCD, &lcd_cb_info) < 0)) {
			int errsv = errno * -1;
			if (errsv == -15) {
				hd_ret = HD_ERR_TIMEDOUT;
			} else {
				HD_VIDEOOUT_ERR("<ioctl \"USR_PROC_RECV_DATA_LCD\" fail, error(%d)>\n", ret);
				hd_ret = HD_ERR_SYS;
			}
			goto exit;
		}

		if (lcd_cb_info.status == 1)  { //1:job finished, 0:ongoing, -1:fail
			hd_ret = HD_OK;
		} else {
			hd_ret = HD_ERR_FAIL;
		}

		if (p_video_frame) {
			p_video_frame->ddr_id = lcd_cb_info.ddr_id;
			p_video_frame->phy_addr[0] = lcd_cb_info.addr_pa;
			if (pif_address_ddr_sanity_check(p_video_frame->phy_addr[0], p_video_frame->ddr_id ) < 0) {
				HD_VIDEOOUT_ERR("%s:Check  pull_out pa(%#lx) ddrid(%d) error\n", __func__, p_video_frame->phy_addr[0], p_video_frame->ddr_id);
				hd_ret = HD_ERR_PARAM;
				goto exit;
			}
		}
	} else {
		HD_VIDEOOUT_ERR("pull out buffer error, ret(%#x)\n", hd_ret);
		goto exit;
	}

exit:
	return hd_ret;
}

HD_RESULT videoout_release_out_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame)
{
	HD_RESULT hd_ret = HD_OK;
	INT ddr_id, ret;
	VOID *addr_pa;
	HD_IO _in_id = HD_IN(VO_INID(in_id));

	hd_ret = dif_release_out_buffer(self_id, _in_id, p_video_frame);
	if (hd_ret == HD_OK) {
		p_video_frame->phy_addr[0] = 0;
	} else if (hd_ret == HD_ERR_NOT_AVAIL) {
		addr_pa = (VOID *) p_video_frame->phy_addr[0];
		ddr_id = p_video_frame->ddr_id;
		ret = pif_free_video_buffer_for_hdal(addr_pa, ddr_id, USR_USR);
		if (ret < 0) {
			HD_VIDEOOUT_ERR("free buffer phy_addr(%p) failed.\n", addr_pa);
			hd_ret = HD_ERR_NG;
		} else {
			hd_ret = HD_OK;
		}
	} else {
		HD_VIDEOOUT_ERR("release out buffer error, ret(%#x)\n", hd_ret);
		hd_ret = HD_ERR_NG;
	}

	return hd_ret;
}


HD_RESULT videoout_get_syscaps(HD_DAL dev_id, HD_VIDEOOUT_SYSCAPS *p_lcd_syscaps)
{
	HD_RESULT hd_ret = HD_OK;
	UINT32 device_subid;

	device_subid = VO_DEVID(dev_id);
	if (device_subid >= hd_videoout_max_device_nr) {
		HD_VIDEOOUT_ERR("Error dev_id(%d)\n", dev_id);
		hd_ret = HD_ERR_DEV;
		goto exit;
	}

	if (lcd_cfg[device_subid].mode->output_type == HD_COMMON_VIDEO_OUT_LCD) {
		goto exit;
	}

	p_lcd_syscaps->dev_id = dev_id;
	p_lcd_syscaps->chip_id = platform_sys_Info.lcd_info[device_subid].chipid;
	p_lcd_syscaps->max_in_count = hd_videoout_max_device_in;
	p_lcd_syscaps->max_out_count = hd_videoout_max_device_out;
	p_lcd_syscaps->input_dim.w = platform_sys_Info.lcd_info[device_subid].desk_res.width;
	p_lcd_syscaps->input_dim.h = platform_sys_Info.lcd_info[device_subid].desk_res.height;
	p_lcd_syscaps->output_dim.w = platform_sys_Info.lcd_info[device_subid].output_type.width;
	p_lcd_syscaps->output_dim.h = platform_sys_Info.lcd_info[device_subid].output_type.height;
	p_lcd_syscaps->max_out_stamp = 0;
	p_lcd_syscaps->max_out_stamp_ex = HD_MAXVO_TOTAL_STAMP_NU;
	p_lcd_syscaps->max_out_mask = 0;
	p_lcd_syscaps->max_out_mask_ex = HD_MAXVO_TOTAL_MASK_NU;
	p_lcd_syscaps->fps = platform_sys_Info.lcd_info[device_subid].fps;
	if (platform_sys_Info.lcd_info[device_subid].lcd_fmt == VPD_BUFTYPE_YUV422) {
		p_lcd_syscaps->out_caps[0] = HD_VIDEO_CAPS_YUV422;
	} else if (platform_sys_Info.lcd_info[device_subid].lcd_fmt == VPD_BUFTYPE_YUV420_SP) {
		p_lcd_syscaps->out_caps[0] = HD_VIDEO_CAPS_YUV420;
	} else if (platform_sys_Info.lcd_info[device_subid].lcd_fmt == VPD_BUFTYPE_YUV420_SCE) {
		p_lcd_syscaps->out_caps[0] = HD_VIDEO_CAPS_YUV420 | HD_VIDEO_CAPS_COMPRESS;
	} else if (platform_sys_Info.lcd_info[device_subid].lcd_fmt == VPD_BUFTYPE_YUV422_SCE) {
		p_lcd_syscaps->out_caps[0] = HD_VIDEO_CAPS_YUV422 | HD_VIDEO_CAPS_COMPRESS;
	} else {
		HD_VIDEOOUT_ERR("Not support lcd_fmt(%#x)\n", platform_sys_Info.lcd_info[device_subid].lcd_fmt);
	}
	p_lcd_syscaps->max_scale_up_w = 4;
	p_lcd_syscaps->max_scale_up_h = 4;
	p_lcd_syscaps->max_scale_down_w = 8;
	p_lcd_syscaps->max_scale_down_h = 8;
	lcd_cfg[device_subid].mode = &g_video_out_param[device_subid].mode;
	if (videoout_get_lcd_dim(device_subid, lcd_cfg[device_subid].mode) != HD_OK) {
		HD_VIDEOOUT_ERR("videoout_get_lcd_dim fail\n");
		hd_ret = HD_ERR_NOT_SUPPORT;
	}
exit:
	return hd_ret;
}

HD_RESULT videoout_get_dev_cfg_p(HD_DAL self_id, HD_VIDEOOUT_DEV_CONFIG *p_dev_cfg)
{
	HD_RESULT ret = HD_OK;
	int lcd_fd = 0, is_lcd300 = 0, is_lcd200 = 0;
	lcd300_drv_mp_t lcd300_mp;
	lcd200_drv_mp_t lcd200_mp;
	UINT8 lcd_idx = VO_DEVID(self_id);

	if (lcd_idx == HD_VIDEOOUT_VDDO0) {
		lcd_fd = open("/dev/hdal_lcd300_0", O_RDWR);
		is_lcd300 = 1;
	 } else if (lcd_idx == HD_VIDEOOUT_VDDO1) {
#if (SECOND_LCD == LCD310_LITE)
		lcd_fd = open("/dev/hdal_lcd300_1", O_RDWR);
		is_lcd300 = 1;
#endif
	} else if (lcd_idx == HD_VIDEOOUT_VDDO2) {
		lcd_fd = open("/dev/hdal_lcd200_0", O_RDWR);
		is_lcd200 = 1;
		is_lcd300 = 0;
	}
	if (lcd_fd < 0) {
		HD_VIDEOOUT_ERR("LCD Error: cannot open hdal_lcd300_0 device.\n");
		return HD_ERR_SYS;
	}

	if ((lcd_fd) && (is_lcd300)) {

		memset(&lcd300_mp, 0, sizeof(lcd300_drv_mp_t));

		if (ioctl(lcd_fd, LCD300_IOC_GET_MP, &lcd300_mp) < 0) {
			HD_VIDEOOUT_ERR("LCD Error: cannot set LCD300_IOC_SET_MP_INIT.\n");
			close(lcd_fd);
			return HD_ERR_SYS;
		}
		close(lcd_fd);
		if ((ret = videoout_get_lcd300_dev_cfg(&p_dev_cfg->devnvr_cfg, &lcd300_mp)) != HD_OK) {
			HD_VIDEOOUT_ERR("videoout_get_lcd300_dev_cfg fail\n");
			return HD_ERR_SYS;
		}
	} else if (is_lcd200) {
		memset(&lcd200_mp, 0, sizeof(lcd200_drv_mp_t));

		if (ioctl(lcd_fd, LCD200_IOC_GET_MP, &lcd200_mp) < 0) {
			HD_VIDEOOUT_ERR("LCD Error: cannot set LCD200_IOC_SET_MP_INIT.\n");
			close(lcd_fd);
			return HD_ERR_SYS;
		}
		close(lcd_fd);

	} else {
		HD_VIDEOOUT_ERR("Not support lcd_idx(%d)\n", lcd_idx);
		return HD_ERR_NOT_SUPPORT;
	}
	return ret;
}

HD_RESULT videoout_get_win(HD_DAL self_id, HD_IO io_id, HD_VIDEOOUT_WIN_ATTR *p_win_param)
{
	HD_RESULT hd_ret = HD_OK;
	memcpy(p_win_param, &g_video_out_param[VO_DEVID(self_id)].win[VO_INID(io_id)], sizeof(HD_VIDEOOUT_WIN_ATTR));
	return hd_ret;
}

HD_RESULT videoout_get_win_psr(HD_DAL self_id, HD_IO io_id, HD_VIDEOOUT_WIN_PSR_ATTR *p_win_psr_param)
{
	HD_RESULT hd_ret = HD_OK;
	memcpy(p_win_psr_param, &g_video_out_param[VO_DEVID(self_id)].win_psr[VO_INID(io_id)], sizeof(HD_VIDEOOUT_WIN_PSR_ATTR));
	return hd_ret;
}

HD_RESULT videoout_get_mode(HD_DAL self_id, HD_VIDEOOUT_MODE *p_mode_param)
{
	HD_RESULT hd_ret = HD_OK;

	UINT32 device_subid;
	device_subid = VO_DEVID(self_id);

	lcd_cfg[device_subid].mode = &g_video_out_param[device_subid].mode;
	if (videoout_get_lcd_dim(device_subid, lcd_cfg[device_subid].mode) != HD_OK) {
		hd_ret = HD_ERR_NOT_SUPPORT;
		HD_VIDEOOUT_ERR("videoout_get_lcd_dim fail\n");
	} else
		memcpy(p_mode_param, &g_video_out_param[device_subid].mode, sizeof(HD_VIDEOOUT_MODE));

	return hd_ret;
}

HD_RESULT videoout_get_stamp(HD_DAL self_id, HD_IO out_id, HD_IO in_id, HD_OSG_STAMP_ATTR *p_stamp_param)
{
	HD_RESULT hd_ret = HD_OK;
	memcpy(p_stamp_param, &g_video_out_param[VO_DEVID(self_id)].stamp[GET_STAMP_ID(in_id, out_id)], sizeof(HD_OSG_STAMP_ATTR));
	return hd_ret;
}

HD_RESULT videoout_get_mask(HD_DAL self_id, HD_IO out_id, HD_IO in_id, HD_OSG_MASK_ATTR *p_mask_param)
{
	HD_RESULT hd_ret = HD_OK;
	memcpy(p_mask_param, &g_video_out_param[VO_DEVID(self_id)].mask_mosaic[GET_MASK_ID(in_id, out_id)].mask_cfg, sizeof(HD_OSG_MASK_ATTR));
	return hd_ret;
}

INT verify_param_HD_VIDEOOUT_OSG_param(INT lcd_id, HD_VDDO_WIN_PARAM *p_videoout_osg_param,
				       UINT16 max_stamp_idx, HD_VIDEO_FRAME *p_in_video_frame)
{
	KFLOW_OSG_HW_SPEC_INFO osg_spec = {0};
	INT ret = 0;
	INT i = 0;
	UINT16 win_nu = max_stamp_idx + 1;
	HD_VDDO_WIN_PARAM *p_param = NULL;
	HD_DIM dst_dim;
	HD_DIM check_dim;
	HD_DIM align_dim;
	HD_UPOINT dst_posit;
	HD_UPOINT align_point;
	if (lcd_id >= 0 && !p_in_video_frame) { //for flow mode check
		dst_dim.w = platform_sys_Info.lcd_info[lcd_id].fb0_win.width;
		dst_dim.h = platform_sys_Info.lcd_info[lcd_id].fb0_win.height;
		osg_spec.tar_type = platform_sys_Info.lcd_info[lcd_id].lcd_fmt;
	} else if (lcd_id < 0 && p_in_video_frame) {//for user mode chekc
		dst_dim.w = p_in_video_frame->dim.w;
		dst_dim.h = p_in_video_frame->dim.h;
		osg_spec.tar_type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_in_video_frame->pxlfmt);
	} else {
		HD_VIDEOOUT_ERR("Check input param fail\n");
		ret = -1;
		goto err_ext;
	}
	osg_spec.op = OSG_OP_GRAPH;
	osg_spec.src_rot = OSG_ROT_0;//HW support rotate src_img,hdal not support
	ret = ioctl(osg_ioctl, IOCTL_OSG_GET_HW_SPEC_INFO, &osg_spec);
	if (ret < 0) {
		HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_GET_HW_SPEC_INFO\" return %d\n", ret);
		ret = -1;
		goto err_ext;
	}
	VDOEOOUT_CHECK_W_H(dst_dim, osg_spec.tar_max_dim, ret);
	if (ret < 0) {
		HD_VIDEOOUT_ERR("Check dst_dim(%dx%d) > osg tar_max(%dx%d)\n", dst_dim.w, dst_dim.h, osg_spec.tar_max_dim.w, osg_spec.tar_max_dim.h);
		ret = -1;
		goto err_ext;
	}

	for (i = 0; i < win_nu; i++) {
		if (!p_videoout_osg_param[i].win_enable) {
			continue;
		}
		p_param = &p_videoout_osg_param[i];
		VDOEOOUT_CHECK_W_H(p_param->img.dim, osg_spec.src_max_dim, ret);
		if (ret < 0) {
			HD_VIDEOOUT_ERR("Check osg img(%dx%d) > osg src_max(%dx%d)\n", p_param->img.dim.w, p_param->img.dim.h, osg_spec.src_max_dim.w, osg_spec.src_max_dim.h);
			ret = -2;
			goto err_ext;
		}
		VDOEOOUT_CHECK_X(p_param->attr.position.x, p_param->img.dim.w, dst_dim.w, ret);
		if (ret < 0) {
			HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_STAMP_ATTR: stamp.x(%u)+stamp.w(%u) > lcd_%d.w(%d)\n",
					p_param->attr.position.x, p_param->img.dim.w, lcd_id, dst_dim.w);
			ret = -3;
			goto err_ext;
		}
		VDOEOOUT_CHECK_Y(p_param->attr.position.y, p_param->img.dim.h, dst_dim.h, ret);
		if (ret < 0) {
			HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_STAMP_ATTR: stamp.y(%u)+stamp.h(%u) > dst_dim.w(%d)\n",
					p_param->attr.position.y, p_param->img.dim.h, dst_dim.h);
			ret = -3;
			goto err_ext;
		}
		check_dim.w = dst_dim.w;
		check_dim.h = dst_dim.h;
		VDOEOOUT_CHECK_DIM_ALIGN(check_dim, osg_spec.tar_align_lv, ret);
		if (ret < 0) {
			HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_STAMP_IMG: dst_dim(%d,%d) need align to (%d,%d)\n",
					dst_dim.w, dst_dim.h, osg_spec.tar_align_lv.w, osg_spec.tar_align_lv.h);
			ret = -3;
			goto err_ext;
		}
		dst_posit.x = p_param->attr.position.x;
		dst_posit.y = p_param->attr.position.y;
		align_point.x = osg_spec.src_align_lv.x;
		align_point.y = osg_spec.src_align_lv.y;
		VDOEOOUT_CHECK_POSIT_ALIGN(dst_posit, align_point, ret);
		align_dim.w = osg_spec.src_align_lv.w;
		align_dim.h = osg_spec.src_align_lv.h;
		VDOEOOUT_CHECK_DIM_ALIGN(p_param->img.dim, align_dim, ret);
		if (ret < 0) {
			HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_STAMP_ATTR: src_point_to_dst_boundary(%d,%d),src_dim(%d,%d) need align to (%d,%d,%d,%d)\n",
					dst_dim.w - (p_param->attr.position.x + p_param->img.dim.w), \
					dst_dim.h - (p_param->attr.position.y + p_param->img.dim.h), \
					p_param->img.dim.w, p_param->img.dim.h, osg_spec.src_align_lv.x, osg_spec.src_align_lv.y, \
					osg_spec.src_align_lv.w, osg_spec.src_align_lv.h);
			p_param->attr.position.x = ALIGN_FLOOR(p_param->attr.position.x, osg_spec.src_align_lv.x);
			p_param->img.dim.w = ALIGN_FLOOR(p_param->img.dim.w, osg_spec.src_align_lv.x);
			p_param->attr.position.y = ALIGN_FLOOR(p_param->attr.position.y, osg_spec.src_align_lv.y);
			p_param->img.dim.h = ALIGN_FLOOR(p_param->img.dim.h, osg_spec.src_align_lv.y);
		}
	}
err_ext:
	return ret;
}

INT verify_param_HD_VIDEOOUT_MASK_MOSAIC_param(INT lcd_id, HD_VDDO_MASK_MOSAIC *p_videoout_mask_mosaic_param,
		UINT16 max_mask_idx, HD_VIDEO_FRAME *p_in_video_frame)
{
	KFLOW_OSG_HW_SPEC_INFO mask_spec = {0};
	KFLOW_OSG_HW_SPEC_INFO mosaic_spec = {0};
	INT ret = 0;
	INT i = 0;
	INT check_mosaic = 0;
	INT check_mask = 0;
	UINT16 win_nu = max_mask_idx + 1;
	HD_VDDO_MASK_MOSAIC *p_param = NULL;
	HD_DIM dst_dim;
	HD_DIM check_dim;
	HD_DIM align_dim;
	HD_UPOINT dst_posit;
	HD_UPOINT align_point;

	if (lcd_id >= 0 && !p_in_video_frame) { //for flow mode check
		dst_dim.w = platform_sys_Info.lcd_info[lcd_id].fb0_win.width;
		dst_dim.h = platform_sys_Info.lcd_info[lcd_id].fb0_win.height;
		mask_spec.tar_type = platform_sys_Info.lcd_info[lcd_id].lcd_fmt;
		mosaic_spec.tar_type = platform_sys_Info.lcd_info[lcd_id].lcd_fmt;
	} else if (lcd_id < 0 && p_in_video_frame) {//for user mode chekc
		dst_dim.w = p_in_video_frame->dim.w;
		dst_dim.h = p_in_video_frame->dim.h;
		mask_spec.tar_type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_in_video_frame->pxlfmt);
		mosaic_spec.tar_type = common_convert_HD_VIDEO_PXLFMT_to_vpd_buffer_type(p_in_video_frame->pxlfmt);
	} else {
		HD_VIDEOOUT_ERR("Check input param fail\n");
		ret = -1;
		goto err_ext;
	}
	for (i = 0; i < win_nu; i++) {
		if (!check_mask && p_videoout_mask_mosaic_param[i].enable && p_videoout_mask_mosaic_param[i].is_mask) {
			check_mask = 1;
			mask_spec.op = OSG_OP_MASK;
			mask_spec.src_rot = OSG_ROT_0;//HW support rotate src_img,hdal not support
			ret = ioctl(osg_ioctl, IOCTL_OSG_GET_HW_SPEC_INFO, &mask_spec);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_GET_HW_SPEC_INFO\" return %d\n", ret);
				ret = -1;
				goto err_ext;
			}
		} else if (!check_mosaic && p_videoout_mask_mosaic_param[i].enable && !p_videoout_mask_mosaic_param[i].is_mask) {
			check_mosaic = 1;
			p_param = &p_videoout_mask_mosaic_param[i];
			mosaic_spec.src_y = p_param->mosaic_cfg.position[0].y;
			mosaic_spec.src_msc_sz = p_param->mosaic_cfg.mosaic_blk_w;
			mosaic_spec.op = OSG_OP_MOSAIC;
			mosaic_spec.src_rot = OSG_ROT_0;//HW support rotate src_img,hdal not support
			p_param->mosaic_cfg.mosaic_blk_w = p_param->mosaic_cfg.mosaic_blk_h = mosaic_spec.src_msc_sz;
			ret = ioctl(osg_ioctl, IOCTL_OSG_GET_HW_SPEC_INFO, &mosaic_spec);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("ioctl \"IOCTL_OSG_GET_HW_SPEC_INFO\" return %d\n", ret);
				ret = -1;
				goto err_ext;
			}
		}
	}

	if (check_mask) {
		VDOEOOUT_CHECK_W_H(dst_dim, mask_spec.tar_max_dim, ret);
		if (ret < 0) {
			HD_VIDEOOUT_ERR("Check lcd_dim(%dx%d) > osg tar_max(%dx%d)\n", dst_dim.w, dst_dim.h, mask_spec.tar_max_dim.w, mask_spec.tar_max_dim.h);
			ret = -1;
			goto err_ext;
		}
	}
	if (check_mosaic) {
		VDOEOOUT_CHECK_W_H(dst_dim, mosaic_spec.tar_max_dim, ret);
		if (ret < 0) {
			HD_VIDEOOUT_ERR("Check lcd_dim(%dx%d) > osg tar_max(%dx%d)\n", dst_dim.w, dst_dim.h, mosaic_spec.tar_max_dim.w, mosaic_spec.tar_max_dim.h);
			ret = -1;
			goto err_ext;
		}
	}
	for (i = 0; i < win_nu; i++) {
		if (!p_videoout_mask_mosaic_param[i].enable) {
			continue;
		}
		p_param = &p_videoout_mask_mosaic_param[i];
		if (p_param->is_mask) {//do mask spec check
			check_dim.w = p_param->mask_cfg.position[1].x - p_param->mask_cfg.position[0].x;
			check_dim.h = p_param->mask_cfg.position[2].y - p_param->mask_cfg.position[1].y;
			VDOEOOUT_CHECK_W_H(check_dim, mask_spec.src_max_dim, ret);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("Check mask img(%dx%d) > mask src_max(%dx%d)\n", check_dim.w, check_dim.h, \
						mask_spec.src_max_dim.w, mask_spec.src_max_dim.h);
				ret = -2;
				goto err_ext;
			}
			VDOEOOUT_CHECK_X(p_param->mask_cfg.position[2].x, check_dim.w, dst_dim.w, ret);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_MASK_ATTR: mask.x(%u)+mask.w(%u) > lcd_%d.w(%d)\n",
						p_param->mask_cfg.position[2].x, check_dim.w, lcd_id, dst_dim.w);
				ret = -3;
				goto err_ext;
			}
			VDOEOOUT_CHECK_Y(p_param->mask_cfg.position[2].y, check_dim.h, dst_dim.h, ret);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_MASK_ATTR: mask.y(%u)+mask.h(%u) > dst_dim.w(%d)\n",
						p_param->mask_cfg.position[2].y, check_dim.h, dst_dim.h);
				ret = -3;
				goto err_ext;
			}
			VDOEOOUT_CHECK_DIM_ALIGN(dst_dim, mask_spec.tar_align_lv, ret);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_MASK_ATTR: dst_dim(%d,%d) need align to (%d,%d)\n",
						dst_dim.w, dst_dim.h, mask_spec.tar_align_lv.w, mask_spec.tar_align_lv.h);
				ret = -4;
				goto err_ext;
			}
			dst_posit.x = p_param->mask_cfg.position[0].x;
			dst_posit.y = p_param->mask_cfg.position[0].y;
			align_point.x = mask_spec.src_align_lv.x;
			align_point.y = mask_spec.src_align_lv.y;
			VDOEOOUT_CHECK_POSIT_ALIGN(dst_posit, align_point, ret);
			align_dim.w = mask_spec.src_align_lv.w;
			align_dim.h = mask_spec.src_align_lv.h;
			VDOEOOUT_CHECK_DIM_ALIGN(check_dim, align_dim, ret);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_MASK_ATTR: src_point_2_to_dst_boundary(%d,%d),src_dim(%d,%d) need align to (%d,%d,%d,%d)\n",
						dst_dim.w - p_param->mask_cfg.position[2].x, dst_dim.h - p_param->mask_cfg.position[2].y, \
						p_param->mask_cfg.position[1].x - p_param->mask_cfg.position[0].x, \
						p_param->mask_cfg.position[2].y - p_param->mask_cfg.position[1].y, mask_spec.src_align_lv.x, \
						mask_spec.src_align_lv.y, mask_spec.src_align_lv.w, mask_spec.src_align_lv.h);
				p_param->mask_cfg.position[0].x  = ALIGN_FLOOR(p_param->mask_cfg.position[0].x, mask_spec.src_align_lv.x);
				p_param->mask_cfg.position[0].y  = ALIGN_FLOOR(p_param->mask_cfg.position[0].y, mask_spec.src_align_lv.y);
				p_param->mask_cfg.position[1].x += p_param->mask_cfg.position[0].x + ALIGN_FLOOR(p_param->mask_cfg.position[1].x - p_param->mask_cfg.position[0].x, \
								   mask_spec.src_align_lv.w);
				p_param->mask_cfg.position[1].y = ALIGN_FLOOR(p_param->mask_cfg.position[1].y, mask_spec.src_align_lv.y);
				p_param->mask_cfg.position[2].x = p_param->mask_cfg.position[1].x;
				p_param->mask_cfg.position[2].y += p_param->mask_cfg.position[1].y + ALIGN_FLOOR(p_param->mask_cfg.position[2].y - p_param->mask_cfg.position[1].y, \
								   mask_spec.src_align_lv.h);
				p_param->mask_cfg.position[3].x = p_param->mask_cfg.position[0].x;
				p_param->mask_cfg.position[3].y = p_param->mask_cfg.position[2].y;
			}
		} else {//do mosaic spec check
			check_dim.w = p_param->mosaic_cfg.position[1].x - p_param->mosaic_cfg.position[0].x;
			check_dim.h = p_param->mosaic_cfg.position[2].y - p_param->mosaic_cfg.position[1].y;
			VDOEOOUT_CHECK_W_H(check_dim, mosaic_spec.src_max_dim, ret);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("Check mosaic img(%dx%d) > mosaic src_max(%dx%d)\n", check_dim.w, check_dim.h, \
						mosaic_spec.src_max_dim.w, mosaic_spec.src_max_dim.h);
				ret = -2;
				goto err_ext;
			}
			VDOEOOUT_CHECK_X(p_param->mosaic_cfg.position[2].x, check_dim.w, dst_dim.w, ret);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_MOSAIC_ATTR: mosaic.x(%u)+mosaic.w(%u) > lcd_%d.w(%d)\n",
						p_param->mosaic_cfg.position[2].x, check_dim.w, lcd_id, dst_dim.w);
				ret = -3;
				goto err_ext;
			}
			VDOEOOUT_CHECK_Y(p_param->mosaic_cfg.position[2].y, check_dim.h, dst_dim.h, ret);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_MOSAIC_ATTR: mosaic.y(%u)+mosaic.h(%u) > dst_dim.w(%d)\n",
						p_param->mosaic_cfg.position[2].y, check_dim.h, dst_dim.h);
				ret = -3;
				goto err_ext;
			}
			VDOEOOUT_CHECK_DIM_ALIGN(dst_dim, mosaic_spec.tar_align_lv, ret);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_MOSAIC_ATTR: dst_dim(%d,%d) need align to (%d,%d)\n",
						dst_dim.w, dst_dim.h, mosaic_spec.tar_align_lv.w, mosaic_spec.tar_align_lv.h);
				ret = -4;
				goto err_ext;
			}
			dst_posit.x = p_param->mask_cfg.position[0].x;
			dst_posit.y = p_param->mask_cfg.position[0].y;
			align_point.x = mosaic_spec.src_align_lv.x;
			align_point.y = mosaic_spec.src_align_lv.y;
			VDOEOOUT_CHECK_POSIT_ALIGN(dst_posit, align_point, ret);
			align_dim.w = mosaic_spec.src_align_lv.w;
			align_dim.h = mosaic_spec.src_align_lv.h;
			VDOEOOUT_CHECK_DIM_ALIGN(check_dim, align_dim, ret);
			if (ret < 0) {
				HD_VIDEOOUT_ERR("HD_VIDEOOUT_PARAM_OUT_MOSAIC_ATTR: src_point_2_to_dst_boundary(%d,%d),src_dim(%d,%d) need align to (%d,%d,%d,%d)\n",
						dst_dim.w - p_param->mosaic_cfg.position[2].x, dst_dim.h - p_param->mosaic_cfg.position[2].y, \
						p_param->mosaic_cfg.position[1].x - p_param->mosaic_cfg.position[0].x, \
						p_param->mosaic_cfg.position[2].y - p_param->mosaic_cfg.position[1].y, mosaic_spec.src_align_lv.x, \
						mosaic_spec.src_align_lv.y, mosaic_spec.src_align_lv.w, mosaic_spec.src_align_lv.h);
				p_param->mosaic_cfg.position[0].x  = ALIGN_FLOOR(p_param->mosaic_cfg.position[0].x, mosaic_spec.src_align_lv.x);
				p_param->mosaic_cfg.position[0].y  = ALIGN_FLOOR(p_param->mosaic_cfg.position[0].y, mosaic_spec.src_align_lv.y);
				p_param->mosaic_cfg.position[1].x += p_param->mosaic_cfg.position[0].x + \
								     ALIGN_FLOOR(p_param->mosaic_cfg.position[1].x - p_param->mosaic_cfg.position[0].x,
										     mosaic_spec.src_align_lv.w);
				p_param->mosaic_cfg.position[1].y = ALIGN_FLOOR(p_param->mosaic_cfg.position[1].y, mosaic_spec.src_align_lv.y);
				p_param->mosaic_cfg.position[2].x = p_param->mosaic_cfg.position[1].x;
				p_param->mosaic_cfg.position[2].y += p_param->mosaic_cfg.position[1].y + \
								     ALIGN_FLOOR(p_param->mosaic_cfg.position[2].y - p_param->mosaic_cfg.position[1].y, mosaic_spec.src_align_lv.h);
				p_param->mosaic_cfg.position[3].x = p_param->mosaic_cfg.position[0].x;
				p_param->mosaic_cfg.position[3].y = p_param->mosaic_cfg.position[2].y;
			}
		}
	}
err_ext:
	return ret;
}

