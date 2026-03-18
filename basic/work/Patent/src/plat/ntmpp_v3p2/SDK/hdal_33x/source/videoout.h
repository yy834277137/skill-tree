/**
 * @file videoout.h
 * @brief type definition of API.
 * @author PSW
 * @date in the year 2018
 */

#ifndef _VIDEOOUT_H_
#define _VIDEOOUT_H_
#ifdef  __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "hd_videoout.h"
#include "bind.h"
#include "trig_ioctl.h"
#include "vpd_ioctl.h"
/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define ENABLE_LOAD_PALETTE_TLB 0
#define VDDO_MAX   5  //original is HD_DAL_VIDEOOUT_COUNT(16), it spends lots of memory (2.8MB), 31x only max 3 physical videoout,id:3~4 for set RC/EP osg virtual lcd
#define VO_IN_STAMP_MAX_NU 16///<This value limt every in_id max stamp number,ex:in_id=2,stamp_id=2*16~3*16-1
#define VO_IN_MASK_MAX_NU HD_VO_MAX_MASK
#define VO_DEVID(self_id)     (self_id - HD_DAL_VIDEOOUT_BASE)
#define VO_INID(id)     (id - HD_IN_BASE)
#define VO_OUTID(id)    (id - HD_OUT_BASE)
#define VO_STAMP_OUTID(id)     (id - HD_STAMP_EX_BASE - HD_OUT_BASE)
#define VO_MASK_OUTID(id)     (id - HD_MASK_EX_BASE - HD_OUT_BASE)
#define GET_STAMP_ID(in_id, out_id)     ((in_id * VO_IN_STAMP_MAX_NU) + out_id)
#define GET_MASK_ID(in_id, out_id)     ((in_id * VO_IN_MASK_MAX_NU) + out_id)
#define HD_VO_MAX_OSG_STAMP 64 ///< each win max osg stamp num
#define HD_VO_MAX_MASK  16 ///< max mask NUM
#define HD_MAXVO_TOTAL_STAMP_NU (HD_VO_MAX_OSG_STAMP * HD_VIDEOOUT_MAX_IN)
#define HD_MAXVO_TOTAL_MASK_NU (HD_VO_MAX_MASK * HD_VIDEOOUT_MAX_IN)
#define VIDEOOUT_MEM_POOL_NAME    '\0'
#define HD_VIDEOOUT_MAX_DEVICE_ID 6
#define HD_VIDEOOUT_VDDO0      0  ///< Indicate vddo device(LCD300)
#define HD_VIDEOOUT_VDDO1      1  ///< Indicate vddo device(LCD300-lite)
#define HD_VIDEOOUT_VDDO2      2  ///< Indicate vddo device(LCD210)
#define HD_VIDEOOUT_VDDO3      3  ///< Indicate vddo device(virtual LCD)
#define HD_VIDEOOUT_VDDO4      4  ///< Indicate vddo device(virtual LCD)
#define HD_VIDEOOUT_VDDO5      5  ///< Indicate vddo device(virtual LCD)
#define MAX_VIDEOOUT_CTRL_ID    HD_VIDEOOUT_VDDO3  ///< Indicate MAX videoout control id,to get dim/fmt
#define OPEN_FB_RETRY_CNT   500//
#define WAIT_FB_TIME        (20*1000)//20ms
#define VIDEOOUT_CHECK_MODULE_OPEN(self_id)                                         \
	do {                                                                        \
		if (g_video_out_param[VO_DEVID(self_id)].is_open == 0) { \
			HD_VIDEOOUT_ERR("Error videoout_%d doesn,t open\n", VO_DEVID(self_id));\
			return HD_ERR_NG;                                                   \
		}                                                                       \
	} while (0)
#define VIDEOOUT_CHECK_SELF_ID(self_id)                                         \
	do {                                                                        \
		if (VO_DEVID(self_id) > HD_VIDEOOUT_VDDO5 || VO_DEVID(self_id) < HD_VIDEOOUT_VDDO0) { \
			HD_VIDEOOUT_ERR("Error self_id(0x%x)\n", (self_id));                  \
			return HD_ERR_NG;                                                   \
		}                                                                       \
	} while (0)

#define VIDEOOUT_CHECK_IO_ID(io_id)                                                    \
	do {                                                                               \
		if ((io_id) >= HD_IN_BASE && (io_id) < (HD_IN_BASE + HD_VIDEOOUT_MAX_IN)) {          \
		} else if ((io_id) >= HD_OUT_BASE && (io_id) < (HD_OUT_BASE + HD_VIDEOOUT_MAX_OUT)) { \
		} else {                                                                       \
			HD_VIDEOOUT_ERR("Error io_id(%d)\n", (io_id));                             \
			return HD_ERR_NG;                                                          \
		}                                                                              \
	} while (0)

#define VIDEOOUT_CHECK_OUT_ID(io_id)                                                    \
	do {                                                                               \
		if ((io_id) >= HD_OUT_BASE && (io_id) < (HD_OUT_BASE + HD_VIDEOOUT_MAX_OUT)) { \
		} else {                                                                       \
			HD_VIDEOOUT_ERR("Error io_id(%d), only support ouput port\n", (io_id));                             \
			return HD_ERR_NG;                                                          \
		}                                                                              \
	} while (0)

#define VIDEOOUT_CHECK_IN_ID(io_id)                                                    \
	do {                                                                               \
		if ((io_id) >= HD_IN_BASE && (io_id) < (HD_IN_BASE + HD_VIDEOOUT_MAX_IN)) {          \
		} else {                                                                       \
			HD_VIDEOOUT_ERR("Error io_id(%d), only support input port\n", (io_id));                             \
			return HD_ERR_NG;                                                          \
		}                                                                              \
	} while (0)

#define VIDEOOUT_CHECK_CTRL_ID(io_id)                                                    \
	do {                                                                               \
		if (io_id == HD_CTRL) { \
		} else {                                                                       \
			HD_VIDEOOUT_ERR("Error io_id(%d), only support ctrl port\n", (io_id));                             \
			return HD_ERR_NG;                                                          \
		}                                                                              \
	} while (0)

#define VIDEOOUT_CHECK_VIDEOOUT_BUF_DIM(p_video_frame, lcd_w, lcd_h)     \
	do {                                                   \
		if (p_video_frame == NULL) {                       \
			ret = HD_ERR_PARAM;                      \
			goto exit;                                     \
		}                                                  \
		if (p_video_frame->dim.w != lcd_w || p_video_frame->dim.h != lcd_h) {         \
			HD_VIDEOOUT_ERR("Check HD_VIDEO_FRAME dim(w=%d,h=%d) error, lcd input dim(w=%d h=%d)\n", \
							p_video_frame->dim.w, p_video_frame->dim.h, lcd_w, lcd_h);    \
			ret = HD_ERR_NOT_SUPPORT;                                           \
			goto exit;                                                        \
		}                                                                     \
	} while (0)
#define VDOEOOUT_CHECK_H_POSIT(p1, p2)                                         \
	do {                                                                        \
		if ((p1.x > p2.x) || (p1.y != p2.y)) { \
			HD_VIDEOOUT_ERR("Error H direction position\n");                  \
			return HD_ERR_NG;                                                   \
		}                                                                       \
	} while (0)
#define VDOEOOUT_CHECK_V_POSIT(p1, p2)                                         \
	do {                                                                        \
		if ((p1.x != p2.x) || (p1.y > p2.y)) { \
			HD_VIDEOOUT_ERR("Error V direction position\n");                  \
			return HD_ERR_NG;                                                   \
		}                                                                       \
	} while (0)
#define VDOEOOUT_CHECK_W_H(src, dst, ret)                                         \
	do {                                                                        \
		if ((src.w > dst.w) || (src.h > dst.h)) {   \
			HD_VIDEOOUT_ERR("Error src dim > dst dim\n");                       \
			ret = -1;                                                           \
		}                                                                       \
		else ret = 0;                                                                \
	} while (0)
#define VDOEOOUT_CHECK_X(x, src_w, dst_w, ret)                                         \
	do {                                                                        \
		if (!src_w && ((x + src_w) > dst_w)) {  \
			HD_VIDEOOUT_ERR("Error (x + src_w) > dst_w\n");                  \
			ret = -1;                                                           \
		}                                                                       \
		else ret = 0;                                                                \
	} while (0)
#define VDOEOOUT_CHECK_Y(y, src_h, dst_h, ret)                                         \
	do {                                                                        \
		if (!src_h && ((y + src_h) > dst_h)) {  \
			HD_VIDEOOUT_ERR("Error (y + src_h) > dst_h\n");                  \
			ret = -1;                                                           \
		}                                                                       \
		else ret = 0;                                                                \
	} while (0)
#define VDOEOOUT_CHECK_DIM_ALIGN(check_dim, align_dim, ret)                                         \
	do {                                                                        \
		if (((align_dim.w > 0) && (align_dim.w != 1) && (check_dim.w % align_dim.w)) \
			|| ((align_dim.h > 0) && (align_dim.h != 1) && (check_dim.h % align_dim.h))) {   \
			HD_VIDEOOUT_ERR("Error Check dim align\n");                  \
			ret = -1;                                                           \
		}                                                                       \
		else ret = 0;                                                                \
	} while (0)
#define VDOEOOUT_CHECK_POSIT_ALIGN(check_posit, align_posit, ret)                                         \
	do {                                                                        \
		if (((align_posit.x > 0) && (align_posit.x != 1) && (check_posit.x % align_posit.x)) \
			|| ((align_posit.y > 0) && (align_posit.y != 1) && (check_posit.y % align_posit.y))) {   \
			HD_VIDEOOUT_ERR("Error Check position align\n");                  \
			ret = -1;                                                           \
		}                                                                       \
		else ret = 0;                                                                \
	} while (0)
/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _FB_PARAM {
	UINT8  enable;
	HD_VIDEO_PXLFMT fmt;
	UINT8  alpha_blend;
	UINT8  alpha_1555;
	BOOL    colorkey_en;   ///< 0:disable/1:enable
	UINT16  r_ckey;        ///< rgb565 must be expended to rgb888
	UINT16  g_ckey;        ///< rgb565 must be expended to rgb888
	UINT16  b_ckey;        ///< rgb565 must be expended to rgb888
	UINT8 fb_to_plane_nu;
	HD_DIM input_dim;
	HD_URECT  output_rect;
} FB_PARAM;

typedef struct {
	INT fd;
	//unsigned int queue_handle;
	uintptr_t queue_handle;
	HD_PATH_ID_STATE push_state;
	HD_PATH_ID_STATE pull_state;
} VDDO_INFO_PRIV;

typedef struct _HD_VDDO_WIN_PARAM {
	BOOL win_enable;
	HD_OSG_STAMP_IMG img;
	HD_OSG_STAMP_ATTR attr;
	HD_PATH_ID path_id;
} HD_VDDO_WIN_PARAM;

typedef struct _HD_VDDO_MASK_MOSAIC {
	INT8 is_mask;///< keep this region is mask(0) or mosaic(1)s. -1 is unused
	BOOL enable;
	union {
		HD_OSG_MASK_ATTR  mask_cfg;
		HD_OSG_MOSAIC_ATTR  mosaic_cfg;
	};
} HD_VDDO_MASK_MOSAIC;

typedef struct _VDDO_PARAM {
	BOOL is_open;
	HD_VIDEOOUT_SYSCAPS syscaps;
	HD_VIDEOOUT_DEV_CONFIG dev_cfg;
	HD_VIDEOOUT_WIN_ATTR  win[HD_VIDEOOUT_MAX_IN];
	HD_VIDEOOUT_WIN_PSR_ATTR  win_psr[HD_VIDEOOUT_MAX_IN];
	HD_VIDEOOUT_MODE mode;
	FB_PARAM fb_param[HD_FB_MAX];
	HD_VDDO_MASK_MOSAIC mask_mosaic[HD_MAXVO_TOTAL_MASK_NU];
	VDDO_INFO_PRIV priv[HD_VIDEOOUT_MAX_IN];
	VDDO_INFO_PRIV priv_ctrl;
	UINT16 osg_idx;
	HD_VDDO_WIN_PARAM stamp[HD_MAXVO_TOTAL_STAMP_NU];
	UINT16 max_stamp_idx;
	UINT16 num_stamp;
	UINT16 max_mask_idx;
	UINT16 num_mask;
	HD_OUT_POOL out_pool;   ///< for pool info setting
	HD_VIDEO_PXLFMT push_in_pxlfmt;
	char push_in_used[HD_VIDEOOUT_MAX_IN];
	INT osg_fd;
} VDDO_PARAM;

typedef struct _VDDO_INTERNAL_PARAM {
	HD_VIDEOOUT_SYSCAPS *p_syscaps;
	HD_VIDEOOUT_DEV_CONFIG *p_dev_cfg;
	HD_VIDEOOUT_WIN_ATTR  *win[HD_VIDEOOUT_MAX_IN];
	HD_VIDEOOUT_WIN_PSR_ATTR  *win_psr[HD_VIDEOOUT_MAX_IN];
	HD_VIDEOOUT_MODE *mode;
	FB_PARAM *fb_param[HD_FB_MAX];
	UINT linefd;
	UINT16 *osg_idx;
	HD_OUT_POOL *p_out_pool;
} VDDO_INTERNAL_PARAM;
/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them
/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
VDDO_INTERNAL_PARAM *videoout_get_param_p(HD_DAL self_id);
HD_RESULT videoout_init(void);
HD_RESULT videoout_uninit(void);
HD_RESULT videoout_set_dev_cfg_p(UINT8 lcd_idx, HD_VIDEOOUT_DEV_CONFIG *p_vdo_dev_cfg);
HD_RESULT videoout_set_mode(UINT8 lcd_idx, HD_VIDEOOUT_MODE *src, HD_VIDEOOUT_MODE *dest);
HD_RESULT videoout_set_win_p(UINT8 lcd_idx, UINT8 win_idx, HD_VIDEOOUT_WIN_ATTR *win_attr);
HD_RESULT videoout_set_win_psr_p(UINT8 lcd_idx, UINT8 win_idx, HD_VIDEOOUT_WIN_PSR_ATTR *win_psr_attr);
HD_RESULT videoout_clearwin_p(UINT8 lcd_idx, HD_VIDEOOUT_CLEAR_WIN *clear_win);
HD_RESULT videoout_set_mode_p(UINT8 lcd_idx);
#if ENABLE_LOAD_PALETTE_TLB
HD_RESULT videoout_load_palette_p(UINT8 lcd_idx, HD_VIDEOOUT_PALETTE_TABLE *palette_tlb);
#endif
extern int pif_alloc_user_fd_for_hdal(vpd_em_type_t type, int chip, int engine, int minor);
extern void *pif_alloc_video_buffer_for_hdal(int size, int ddr_id, char pool_name[], int alloc_tag);
extern int pif_free_video_buffer_for_hdal(void *pa, int ddr_id, int alloc_tag);
//extern unsigned int pif_create_queue_for_hdal(void);
extern uintptr_t pif_create_queue_for_hdal(void);
//extern int pif_destroy_queue_for_hdal(unsigned int queue_handle);
extern int pif_destroy_queue_for_hdal(uintptr_t queue_handle);
extern int pif_env_update(void);
HD_RESULT videoout_new_in_buf(HD_DAL self_id, HD_VIDEO_FRAME *p_video_frame);
HD_RESULT videoout_push_in_buf(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_in_video_frame, INT32 wait_ms);
HD_RESULT videoout_release_in_buf(HD_DAL self_id, HD_VIDEO_FRAME *p_video_frame);
HD_RESULT videoout_pull_out_buf(HD_PATH_ID path_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame, INT32 wait_ms);
HD_RESULT videoout_release_out_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame);
HD_RESULT videoout_push_in_buf_to_osg(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_in_video_frame, HD_VIDEO_FRAME *p_user_out_video_frame, INT32 wait_ms);
HD_RESULT videoout_pull_out_buf_from_osg(HD_PATH_ID path_id, HD_VIDEO_FRAME *p_in_video_frame, INT32 wait_ms);
HD_RESULT videoout_get_syscaps(HD_DAL dev_id, HD_VIDEOOUT_SYSCAPS *p_cap_syscaps);
HD_RESULT videoout_get_win(HD_DAL self_id, HD_IO io_id, HD_VIDEOOUT_WIN_ATTR *p_win_param);
HD_RESULT videoout_get_win_psr(HD_DAL self_id, HD_IO io_id, HD_VIDEOOUT_WIN_PSR_ATTR *p_win_psr_param);
HD_RESULT videoout_get_mode(HD_DAL self_id, HD_VIDEOOUT_MODE *p_mode_param);
HD_RESULT videoout_get_stamp(HD_DAL self_id, HD_IO out_id, HD_IO in_id, HD_OSG_STAMP_ATTR *p_stamp_param);
HD_RESULT videoout_get_mask(HD_DAL self_id, HD_IO out_id, HD_IO in_id, HD_OSG_MASK_ATTR *p_mask_param);
HD_RESULT videoout_get_devcount(HD_DEVCOUNT *p_devcount);
HD_RESULT videoout_set_osg_win(INT osg_idx, HD_VDDO_WIN_PARAM *win_info, UINT16 max_win_idx);
HD_RESULT videoout_update_stamp_cfg(HD_IO out_id, HD_IO in_id, VDDO_PARAM *p_video_out_param, BOOL is_close);
HD_RESULT videoout_set_mask_mosaic(UINT32 osg_idx, HD_VDDO_MASK_MOSAIC *mask_mosaic_param, UINT16 max_mask_idx);
HD_RESULT videoout_update_mask_cfg(HD_IO out_id, HD_IO in_id, VDDO_PARAM *p_video_out_param);
HD_RESULT videoout_set_fb_fmt(HD_FB_ID fb_id, INT8 lcd_id, HD_VIDEO_PXLFMT fmt);
HD_RESULT videoout_get_fb_fmt(HD_FB_ID fb_id, INT8 lcd_id, HD_VIDEO_PXLFMT *fmt);
HD_RESULT videoout_set_fb_attr(HD_FB_ID fb_id, INT8 lcd_id, FB_PARAM *p_fb_param);
HD_RESULT videoout_get_fb_attr(HD_FB_ID fb_id, INT8 lcd_id, HD_FB_ATTR *p_fb_attrs);
HD_RESULT videoout_set_fb_dim(HD_FB_ID fb_id, INT8 lcd_id, HD_DIM input_dim, HD_URECT output_rect);
HD_RESULT videoout_get_fb_dim(HD_FB_ID fb_id, INT8 lcd_id, HD_DIM *input_dim, HD_URECT *output_rect);
HD_RESULT videoout_set_fb_enable_p(INT8 fb_id, INT8 lcd_id, FB_PARAM *fb_param);
HD_RESULT videoout_get_fb_enable_p(INT8 fb_id, INT8 lcd_id, HD_FB_ENABLE *fb_state);
HD_RESULT videoout_get_hdmi_edid_p(HD_VIDEOOUT_EDID_DATA *p_edid_data);
HD_RESULT videoout_get_vga_edid_p(HD_VIDEOOUT_EDID_DATA *p_edid_data);
HD_RESULT videoout_check_fb_dim(HD_FB_ID fb_id, INT8 lcd_id, HD_DIM input_dim, HD_URECT output_rect);
HD_RESULT videoout_get_dev_cfg_p(HD_DAL self_id, HD_VIDEOOUT_DEV_CONFIG *p_dev_cfg);
HD_RESULT videoout_set_osg_stamp_palette_tbl(UINT32 *pal_tbl, INT tbl_sz);
HD_RESULT videoout_get_osg_stamp_palette_tbl(UINT32 *pal_tbl, INT tbl_sz);
HD_RESULT videoout_set_osg_mask_palette_tbl(UINT32 *pal_tbl, INT tbl_sz);
HD_RESULT videoout_get_osg_mask_palette_tbl(UINT32 *pal_tbl, INT tbl_sz);
INT verify_param_HD_VIDEOOUT_OSG_param(INT lcd_id, HD_VDDO_WIN_PARAM *p_videoout_osg_param, UINT16 max_stamp_idx, HD_VIDEO_FRAME *p_in_video_frame);
INT verify_param_HD_VIDEOOUT_MASK_MOSAIC_param(INT lcd_id, HD_VDDO_MASK_MOSAIC *p_videoout_mask_mosaic_param, UINT16 max_mask_idx, HD_VIDEO_FRAME *p_in_video_frame);
HD_RESULT videoout_open(UINT8 lcd_id, HD_IO out_id, HD_IO in_id);
HD_RESULT videoout_close(HD_PATH_ID path_id);
HD_RESULT videoout_dev_exist(UINT8 lcd_id);
HD_RESULT videoout_get_lcd_dim(UINT8 lcd_idx, HD_VIDEOOUT_MODE *lcd_mode);
HD_RESULT videoout_set_videoout_output_lcd300(int homology, unsigned int vout_id);
HD_RESULT videoout_set_lcd300_vgadac(unsigned int vout_id, unsigned int is_on);
int videoout_check_mp_init_done(int videoout_id);
char* videoout_get_osg_type_name(HD_VIDEO_PXLFMT osg_type);
#ifdef  __cplusplus
}
#endif


#endif  /* _VIDEOOUT_H_ */
