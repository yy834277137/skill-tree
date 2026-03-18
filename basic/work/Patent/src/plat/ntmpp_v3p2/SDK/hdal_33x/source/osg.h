/**
 * @file osg.h
 * @brief type definition of API.
 * @author PSW
 * @date in the year 2018
 */

#ifndef _OSG_H_
#define _OSG_H_
#ifdef  __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "hd_type.h"
#include "bind.h"
/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                        */
/*-----------------------------------------------------------------------------*/
#define OSG_MAX_DEV  HD_DAL_OSG_COUNT
#define OSG_MAX_PORT  64
#define OSG_MAX_IMG  16
#define OSG_MAX_WIN  64
#define OSG_MASK_NUM 128
#define OSG_DEVID(self_id)     (self_id - HD_DAL_OSG_BASE)
#define OSG_INID(id)    (id - HD_IN_BASE)
#define OSG_OUTID(id)    (id - HD_OUT_BASE)
#define OSG_MAX_DEV   			HD_DAL_OSG_COUNT
#define OSG_MEM_POOL_NAME    '\0'
extern UINT32 osg_max_device_nr;
extern UINT32 osg_max_device_in;
extern UINT32 osg_max_device_out;
#define OSG_CHECK_SELF_ID(self_id)                                         \
	do {                                                                        \
		if (OSG_DEVID(self_id) > 1 || OSG_DEVID(self_id) < 0) { \
			HD_OSG_ERR("Error self_id(%d)\n", (self_id));                  \
			return HD_ERR_NG;                                                   \
		}                                                                       \
	} while (0)

#define OSG_CHECK_OUT_ID(io_id)                                                    \
	do {                                                                               \
        if ((io_id) >= HD_OUT_BASE && (io_id) < (HD_OUT_BASE + OSG_MAX_PORT)) { \
		} else {                                                                       \
			HD_OSG_ERR("Error io_id(%d), only support ouput port\n", (io_id));                             \
			return HD_ERR_NG;                                                          \
		}                                                                              \
	} while (0)

#define OSG_CHECK_CTRL_ID(io_id)                                                    \
	do {                                                                               \
        if (io_id == HD_CTRL) { \
		} else {                                                                       \
			HD_OSG_ERR("Error io_id(%d), only support ctrl port\n", (io_id));                             \
			return HD_ERR_NG;                                                          \
		}                                                                              \
	} while (0)

#define OSG_CHECK_RGN_NU(rgn_nu)                                                    \
	do {                                                                               \
        if ((rgn_nu >= 0) && (rgn_nu < OSG_MAX_WIN)) { \
		} else {                                                                       \
			HD_OSG_ERR("Error rgn_id(%d), MAX_RGN_NU(%d)\n", (rgn_nu), OSG_MAX_WIN);                             \
			return HD_ERR_NG;                                                          \
		}                                                                              \
	} while (0)
/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct {
	UINT osg_fd;
	//UINT queue_handle;
	uintptr_t queue_handle;
} OSG_INFO_PRIV;

typedef struct _OSG_PARAM {
	HD_DAL bind_devid;
	HD_IO bind_ioid;
	UINT is_init;
	HD_OSG_IMG_BMP image_info[OSG_MAX_IMG];
	HD_OSG_STAMP img_win[OSG_MAX_WIN];
	HD_OSG_MASK mask_rgn[OSG_MAX_WIN];
	OSG_INFO_PRIV priv;
} OSG_PORT_PARAM;

typedef struct _OSG_DEV_PARAM {
	OSG_PORT_PARAM port_cfg[OSG_MAX_PORT];
} OSG_DEV_PARAM;

typedef struct _OSG_INT_DEV_PARAM {
	OSG_PORT_PARAM *port_cfg[OSG_MAX_PORT];
} OSG_INTERNAL_PARAM;
/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them
/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
HD_RESULT osg_init(void);
HD_RESULT osg_uninit(void);
HD_RESULT osg_bind(HD_DAL self_id, HD_IO out_id, HD_DAL dest_id, HD_IO in_id);
HD_RESULT osg_unbind(HD_DAL self_id, HD_IO out_id);
HD_RESULT osg_open(HD_DAL self_id, HD_IO out_id);
HD_RESULT osg_close(HD_DAL self_id, HD_IO out_id);
HD_RESULT osg_load_image(HD_OSG_IMG_BMP *img_bmp);
HD_RESULT osg_set_region(OSG_PORT_PARAM *port_cfg, UINT8 rgn_nu);
HD_RESULT osg_set_mask_to_osg(HD_DAL dest_id, HD_IO in_id, OSG_PORT_PARAM *port_cfg, UINT8 mask_nu);
HD_RESULT osg_set_palette_tbl(HD_OSG_PALETTE_TABLE *pal_tbl);
OSG_INTERNAL_PARAM *osg_get_param_p(HD_DAL self_id);
HD_RESULT osg_new_in_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_video_frame);
HD_RESULT osg_push_in_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_in_video_frame, INT32 wait_ms);
HD_RESULT osg_pull_out_buf(HD_DAL self_id, HD_IO in_id, HD_VIDEO_FRAME *p_in_video_frame, INT32 wait_ms);
HD_RESULT osg_release_out_buf(HD_DAL self_id, HD_IO out_id, HD_VIDEO_FRAME *p_video_frame);
#ifdef  __cplusplus
}
#endif


#endif  /* _GFX_H_ */
