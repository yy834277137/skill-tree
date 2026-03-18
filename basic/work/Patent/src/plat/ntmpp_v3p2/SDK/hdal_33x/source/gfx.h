/**
 * @file gfx.h
 * @brief type definition of API.
 * @author PSW
 * @date in the year 2018
 */

#ifndef _GFX_H_
#define _GFX_H_
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
#define GFX_MAX_DEV  HD_DAL_GFX_COUNT
#define GFX_MAX_PORT  64
/*
#define GFX_MAX_OSG_IMG  16
#define GFX_MAX_WIN  64
#define GFX_MASK_NUM 128
*/
#define GFX_MEM_POOL_NAME    '\0'
extern UINT32 videogfx_max_device_nr;
extern UINT32 videogfx_max_device_in;
extern UINT32 videogfx_max_device_out;
/*
#define GFX_CHECK_SELF_ID(self_id)                                         \
	do {                                                                        \
		if (GFX_DEVID(self_id) > GFX_MAX_DEV || GFX_DEVID(self_id) < 0) { \
			HD_GFX_ERR("Error self_id(%d)\n", (self_id));                  \
			return HD_ERR_NG;                                                   \
		}                                                                       \
	} while (0)

*/
/*-----------------------------------------------------------------------------*/
/* Types Declarations                                                          */
/*-----------------------------------------------------------------------------*/
typedef struct _GFX_PARAM {
	UINT is_init;
} GFX_PORT_PARAM;
typedef struct _GFX_INT_DEV_PARAM {
	GFX_PORT_PARAM *port_cfg[GFX_MAX_PORT];
	HD_OUT_POOL *p_out_pool;
} GFX_INTERNAL_PARAM;

typedef struct _GFX_SYSCAPS {
        UINT scl_rate;                                         ///< [out] scaling rate:ex:1/Nx~Nx
        HD_DIM min_dim;                                 ///< [out] minimum dimension
        HD_DIM max_dim;                                ///< [out] maximum dimension
        HD_DIM image_align;                           ///< [out] dimension alignment
} GFX_SYSCAPS;

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
// DON'T export global variable here. Please use function to access them
/*-----------------------------------------------------------------------------*/
/* Interface Function Prototype                                                */
/*-----------------------------------------------------------------------------*/
HD_RESULT gfx_init(void);
HD_RESULT gfx_uninit(void);
HD_RESULT gfx_scale_imgs(HD_GFX_SCALE *p_param, UINT8 out_alpha_trans_th, UINT8 num_of_image);
HD_RESULT gfx_copy_imgs(HD_GFX_COPY *p_param, UINT8 num_of_image, UINT8 dithering_en);
HD_RESULT gfx_draw_rects(HD_GFX_DRAW_RECT *p_param, UINT8 num_of_rect);
HD_RESULT gfx_draw_lines(HD_GFX_DRAW_LINE *p_param, UINT8 num_of_line);
HD_RESULT gfx_roate_imgs(HD_GFX_ROTATE *p_param, UINT8 num_of_image);
#ifdef  __cplusplus
}
#endif


#endif  /* _GFX_H_ */
