/**
    @brief Source file of graphic function.\n
    This file contains the functions which is related to video graphic function in the chip.

    @file hd_gfx.c

    @ingroup mhdal

    @note Nothing.

    Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include "hdal.h"
#include "gfx.h"
#include "logger.h"
#include "vpd_ioctl.h"
#include "pif.h"
/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
UINT32 videogfx_max_device_nr;
UINT32 videogfx_max_device_in;
UINT32 videogfx_max_device_out;
static int lock_init = 0;
static pthread_mutex_t gfx_lock;
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/

HD_RESULT hd_gfx_init(VOID)
{
    HD_RESULT ret = HD_OK;

    if (!lock_init) {
        lock_init = 1;
        if (pthread_mutex_init(&gfx_lock, NULL)) {
            HD_GFX_ERR("gfx_lock:pthread_mutex_init fail\n");
            goto err_exit;
        }
    } else {
        HD_GFX_ERR("hd_gfx_init twice\n");
        goto err_exit;
    }

    videogfx_max_device_nr = GFX_MAX_DEV;
    videogfx_max_device_in = 8;
    videogfx_max_device_out = 8;
    ret = gfx_init();
	if (ret != HD_OK)
		return ret;
    if (pif_env_update() < 0) {
	    HD_GFX_ERR("pif_env_update fail\n");
        ret = HD_ERR_INIT;
    }
err_exit:
    HD_GFX_FLOW("hd_gfx_init\n");
    return ret;
}

HD_RESULT hd_gfx_uninit(VOID)
{
    HD_RESULT ret = HD_OK;


    videogfx_max_device_nr = 0;
    videogfx_max_device_in = 0;
    videogfx_max_device_out = 0;
    gfx_uninit();

    if (lock_init) {
        pthread_mutex_destroy(&gfx_lock);
        lock_init = 0;
    } else {
        HD_GFX_ERR("hd_gfx_uninit twice\n");
    }

    HD_GFX_FLOW("hd_gfx_uninit\n");
    return ret;
}

HD_RESULT hd_gfx_crop_img(HD_GFX_IMG_BUF src_buf_img, HD_IRECT src_region, HD_GFX_IMG_BUF dst_buf_img)
{
    HD_RESULT ret = HD_OK;

    HD_GFX_ERR("hd_gfx_crop_img not support\n");
    return ret;
}

HD_RESULT hd_gfx_memset(HD_GFX_DRAW_RECT *p_param)
{
    HD_RESULT ret = HD_OK;

    HD_GFX_ERR("hd_gfx_memset not support\n");

    return ret;
}

HD_RESULT _hd_gfx_scale(HD_GFX_SCALE *p_param, UINT8 out_alpha_trans_th, UINT8 num_of_image)
{
    HD_RESULT ret = HD_OK;
    UINT16 i;

    if (!lock_init) {
        HD_GFX_ERR("gfx is not init\n");
        ret = HD_ERR_INIT;
        goto err_exit;
    }
    if (!p_param) {
        HD_GFX_ERR("check p_param fail\n");
        ret = HD_ERR_ABORT;
        goto err_exit;
    }
    for (i = 0; i < num_of_image; i++) {
        if (pif_address_ddr_sanity_check(p_param[i].src_img.p_phy_addr[0], p_param[i].src_img.ddr_id) < 0) {
            HD_GFX_ERR("%s:Check src pa(%#lx) ddrid(%d) error\n", __func__, p_param[i].src_img.p_phy_addr[0], p_param[i].src_img.ddr_id);
            ret = HD_ERR_PARAM;
            goto err_exit;
        }
        if (pif_address_ddr_sanity_check(p_param[i].dst_img.p_phy_addr[0], p_param[i].dst_img.ddr_id) < 0) {
            HD_GFX_ERR("%s:Check dst pa(%#lx) ddrid(%d) error\n", __func__, p_param[i].dst_img.p_phy_addr[0], p_param[i].dst_img.ddr_id);
            ret = HD_ERR_PARAM;
            goto err_exit;
        }
    }

    pthread_mutex_lock(&gfx_lock);

    ret = gfx_scale_imgs(p_param, out_alpha_trans_th, num_of_image);
    if (ret != HD_OK) {
        ret = HD_ERR_NG;
        HD_GFX_ERR("gfx_set_scale_img fail\n");
        for (i = 0; i < num_of_image; i++) {
            HD_GFX_ERR("   src_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) src_rect(%d,%d,%d,%d)\n"
                       "   dst_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) dst_rect(%d,%d,%d,%d) quality(%#x)\n",
                       p_param[i].src_img.ddr_id, p_param[i].src_img.p_phy_addr[0],
                       p_param[i].src_img.dim.w, p_param[i].src_img.dim.h, p_param[i].src_img.format,
                       p_param[i].src_region.x, p_param[i].src_region.y, p_param[i].src_region.w, p_param[i].src_region.h,
                       p_param[i].dst_img.ddr_id, p_param[i].dst_img.p_phy_addr[0],
                       p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h, p_param[i].dst_img.format,
                       p_param[i].dst_region.x, p_param[i].dst_region.y, p_param[i].dst_region.w, p_param[i].dst_region.h,
                       p_param[i].quality);
        }
    } else {
        HD_GFX_FLOW("hd_gfx_scale:\n");
        for (i = 0; i < num_of_image; i++) {
            HD_GFX_FLOW("   src_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) src_rect(%d,%d,%d,%d)\n"
                        "   dst_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) dst_rect(%d,%d,%d,%d) quality(%#x)\n",
                        p_param[i].src_img.ddr_id, p_param[i].src_img.p_phy_addr[0],
                        p_param[i].src_img.dim.w, p_param[i].src_img.dim.h, p_param[i].src_img.format,
                        p_param[i].src_region.x, p_param[i].src_region.y, p_param[i].src_region.w, p_param[i].src_region.h,
                        p_param[i].dst_img.ddr_id, p_param[i].dst_img.p_phy_addr[0],
                        p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h, p_param[i].dst_img.format,
                        p_param[i].dst_region.x, p_param[i].dst_region.y, p_param[i].dst_region.w, p_param[i].dst_region.h,
                        p_param[i].quality);
        }
    }
    pthread_mutex_unlock(&gfx_lock);
err_exit:
    return ret;
}
HD_RESULT hd_gfx_scale(HD_GFX_SCALE *p_param)
{
    return _hd_gfx_scale(p_param, 128, 1);
}

HD_RESULT __hd_gfx_copy(HD_GFX_COPY *p_param, UINT8 num_of_image, UINT8 dithering_en)
{
    HD_RESULT ret = HD_OK;
    UINT16 i;

    if (!lock_init) {
        HD_GFX_ERR("gfx is not init\n");
        ret = HD_ERR_INIT;
        goto err_exit;
    }
    if (!p_param) {
        HD_GFX_ERR("check p_param fail\n");
        ret = HD_ERR_ABORT;
        goto err_exit;
    }

    for (i = 0; i < num_of_image; i++) {
        if (pif_address_ddr_sanity_check(p_param[i].src_img.p_phy_addr[0], p_param[i].src_img.ddr_id) < 0) {
            HD_GFX_ERR("%s:Check src pa(%#lx) ddrid(%d) error\n", __func__, p_param[i].src_img.p_phy_addr[0], p_param[i].src_img.ddr_id);
            ret = HD_ERR_PARAM;
            goto err_exit;
        }
        if (pif_address_ddr_sanity_check(p_param[i].dst_img.p_phy_addr[0], p_param[i].dst_img.ddr_id) < 0) {
            HD_GFX_ERR("%s:Check dst pa(%#lx) ddrid(%d) error\n", __func__, p_param[i].dst_img.p_phy_addr[0], p_param[i].dst_img.ddr_id);
            ret = HD_ERR_PARAM;
            goto err_exit;
        }
    }

    pthread_mutex_lock(&gfx_lock);
    ret = gfx_copy_imgs(p_param, num_of_image, dithering_en);
    if (ret != HD_OK) {
        ret = HD_ERR_NG;
        HD_GFX_ERR("gfx_copy_img fail\n");
        for (i = 0; i < num_of_image; i++) {
            HD_GFX_ERR("   src_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) src_rect(%d,%d,%d,%d)\n"
                       "   dst_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) dst_pos(%d,%d) colorkey(%#x) alpha(%#x)\n",
                       p_param[i].src_img.ddr_id, p_param[i].src_img.p_phy_addr[0],
                       p_param[i].src_img.dim.w, p_param[i].src_img.dim.h, p_param[i].src_img.format,
                       p_param[i].src_region.x, p_param[i].src_region.y, p_param[i].src_region.w, p_param[i].src_region.h,
                       p_param[i].dst_img.ddr_id, p_param[i].dst_img.p_phy_addr[0],
                       p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h, p_param[i].dst_img.format,
                       p_param[i].dst_pos.x, p_param[i].dst_pos.y, p_param[i].colorkey, p_param[i].alpha);
        }
    } else {
        HD_GFX_FLOW("hd_gfx_copy:\n");
        for (i = 0; i < num_of_image; i++) {
            HD_GFX_FLOW("   src_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) src_rect(%d,%d,%d,%d)\n"
                        "   dst_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) dst_pos(%d,%d) colorkey(%#x) alpha(%#x)\n",
                        p_param[i].src_img.ddr_id, p_param[i].src_img.p_phy_addr[0],
                        p_param[i].src_img.dim.w, p_param[i].src_img.dim.h, p_param[i].src_img.format,
                        p_param[i].src_region.x, p_param[i].src_region.y, p_param[i].src_region.w, p_param[i].src_region.h,
                        p_param[i].dst_img.ddr_id, p_param[i].dst_img.p_phy_addr[0],
                        p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h, p_param[i].dst_img.format,
                        p_param[i].dst_pos.x, p_param[i].dst_pos.y, p_param[i].colorkey, p_param[i].alpha);
        }
    }
    pthread_mutex_unlock(&gfx_lock);
err_exit:
    return ret;
}

HD_RESULT hd_gfx_copy(HD_GFX_COPY *p_param)
{
    return __hd_gfx_copy(p_param, 1, 0);
}

HD_RESULT __gfx_common_dmacopy(HD_COMMON_MEM_DDR_ID src_ddr, UINTPTR src_phy_addr,
			    HD_COMMON_MEM_DDR_ID dst_ddr, UINTPTR dst_phy_addr,
			    UINT32 len)
{
    HD_GFX_COPY gfx_data_copy;

    gfx_data_copy.alpha = 0;
    gfx_data_copy.colorkey = 0;
    gfx_data_copy.dst_pos.x = 0;
    gfx_data_copy.dst_pos.y = 0;
    gfx_data_copy.src_region.x = 0;
    gfx_data_copy.src_region.y = 0;
    gfx_data_copy.src_region.w = 2;
    gfx_data_copy.src_region.h = len / 4; 
    gfx_data_copy.src_img.ddr_id = src_ddr;
    gfx_data_copy.src_img.dim.w = 2;
    gfx_data_copy.src_img.dim.h = len / 4;
    gfx_data_copy.src_img.format = HD_VIDEO_PXLFMT_ARGB1555;
    gfx_data_copy.src_img.p_phy_addr[0] = src_phy_addr;
    gfx_data_copy.dst_img.dim.w = 2;
    gfx_data_copy.dst_img.dim.h = len / 4; 
    gfx_data_copy.dst_img.format = HD_VIDEO_PXLFMT_ARGB1555;
    gfx_data_copy.dst_img.ddr_id = dst_ddr;
    gfx_data_copy.dst_img.p_phy_addr[0] = dst_phy_addr;
    return __hd_gfx_copy(&gfx_data_copy, 1, 0);
}

HD_RESULT __hd_gfx_draw_rects(HD_GFX_DRAW_RECT *p_param, UINT8 num_of_rect)
{
    HD_RESULT ret = HD_OK;
    UINT16 i;

    if (!lock_init) {
        HD_GFX_ERR("gfx is not init\n");
        ret = HD_ERR_INIT;
        goto err_exit;
    }

    if (!p_param) {
        HD_GFX_ERR("check p_param fail\n");
        ret = HD_ERR_ABORT;
        goto err_exit;
    }
    for (i = 0; i < num_of_rect; i++) {
        if (!p_param[i].rect.h || !p_param[i].rect.w) {
            HD_GFX_ERR("check rect fail\n");
            ret = HD_ERR_ABORT;
            goto err_exit;
        }
        if (pif_address_ddr_sanity_check(p_param[i].dst_img.p_phy_addr[0], p_param[i].dst_img.ddr_id) < 0) {
            HD_GFX_ERR("%s:Check dst pa(%#lx) ddrid(%d) error\n", __func__, p_param[i].dst_img.p_phy_addr[0], p_param[i].dst_img.ddr_id);
            ret = HD_ERR_PARAM;
            goto err_exit;
        }
    }

    pthread_mutex_lock(&gfx_lock);
    ret = gfx_draw_rects(p_param, num_of_rect);
    if (ret != HD_OK) {
        ret = HD_ERR_NG;
        HD_GFX_ERR("gfx_draw_rect fail\n");
        for (i = 0; i < num_of_rect; i++) {
            HD_GFX_ERR("   rect_%d:dst_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) rect(%d,%d,%d,%d) color(%#x) type(%#x) thickness(%d)\n",
                       i, p_param[i].dst_img.ddr_id, p_param[i].dst_img.p_phy_addr[0],
                       p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h, p_param[i].dst_img.format,
                       p_param[i].rect.x, p_param[i].rect.y, p_param[i].rect.w, p_param[i].rect.h,
                       p_param[i].color, p_param[i].type, p_param[i].thickness);
        }
    } else {
        HD_GFX_FLOW("hd_gfx_draw_rect:\n");
        for (i = 0; i < num_of_rect; i++) {
            HD_GFX_FLOW("   rect_%d:dst_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) rect(%d,%d,%d,%d) color(%#x) type(%#x) thickness(%d)\n",
                        i, p_param[i].dst_img.ddr_id, p_param[i].dst_img.p_phy_addr[0],
                        p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h, p_param[i].dst_img.format,
                        p_param[i].rect.x, p_param[i].rect.y, p_param[i].rect.w, p_param[i].rect.h,
                        p_param[i].color, p_param[i].type, p_param[i].thickness);
        }
    }
    pthread_mutex_unlock(&gfx_lock);
err_exit:
    return ret;
}

HD_RESULT hd_gfx_draw_rect(HD_GFX_DRAW_RECT *p_param)
{
    return __hd_gfx_draw_rects(p_param, 1);
}

HD_RESULT __hd_gfx_draw_lines(HD_GFX_DRAW_LINE *p_param, UINT8 num_of_line)
{
    HD_RESULT ret = HD_OK;
    UINT16 i;

    if (!lock_init) {
        HD_GFX_ERR("gfx is not init\n");
        ret = HD_ERR_INIT;
        goto err_exit;
    }

    if (!p_param) {
        HD_GFX_ERR("check p_param fail\n");
        ret = HD_ERR_ABORT;
        goto err_exit;
    }
    for (i = 0; i < num_of_line; i++) {
        if ((p_param[i].start.x == p_param[i].end.x) && (p_param[i].start.y == p_param[i].end.y)) {
            HD_GFX_ERR("check line fail\n");
            ret = HD_ERR_ABORT;
            goto err_exit;
        }
        if (pif_address_ddr_sanity_check(p_param[i].dst_img.p_phy_addr[0], p_param[i].dst_img.ddr_id) < 0) {
            HD_GFX_ERR("%s:Check dst pa(%#lx) ddrid(%d) error\n", __func__, p_param[i].dst_img.p_phy_addr[0], p_param[i].dst_img.ddr_id);
            ret = HD_ERR_PARAM;
            goto err_exit;
        }
    }

    pthread_mutex_lock(&gfx_lock);
    ret = gfx_draw_lines(p_param, num_of_line);
    if (ret != HD_OK) {
        ret = HD_ERR_NG;
        HD_GFX_ERR("hd_gfx_draw_line: fail\n");
        for (i = 0; i < num_of_line; i++) {
            HD_GFX_ERR("   line_%d:dst_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) start(%d,%d) end(%d,%d) color(%#x) thickness(%d)\n",
                       i, p_param[i].dst_img.ddr_id, p_param[i].dst_img.p_phy_addr[0],
                       p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h, p_param[i].dst_img.format,
                       p_param[i].start.x, p_param[i].start.y, p_param[i].end.x, p_param[i].end.y,
                       p_param[i].color, p_param[i].thickness);
        }
    } else {
        HD_GFX_FLOW("hd_gfx_draw_line:\n");
        for (i = 0; i < num_of_line; i++) {
            HD_GFX_FLOW("   line_%d:dst_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) start(%d,%d) end(%d,%d) color(%#x) thickness(%d)\n",
                        i, p_param[i].dst_img.ddr_id, p_param[i].dst_img.p_phy_addr[0],
                        p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h, p_param[i].dst_img.format,
                        p_param[i].start.x, p_param[i].start.y, p_param[i].end.x, p_param[i].end.y,
                        p_param[i].color, p_param[i].thickness);
        }
    }
    pthread_mutex_unlock(&gfx_lock);
err_exit:
    return ret;
}

HD_RESULT hd_gfx_draw_line(HD_GFX_DRAW_LINE *p_param)
{
    return __hd_gfx_draw_lines(p_param, 1);
}

HD_RESULT __hd_gfx_rotate(HD_GFX_ROTATE *p_param, UINT8 num_of_image)
{
    HD_RESULT ret = HD_OK;
    UINT16 i;

    if (!lock_init) {
        HD_GFX_ERR("gfx is not init\n");
        ret = HD_ERR_INIT;
        goto err_exit;
    }
    if (!p_param) {
        HD_GFX_ERR("check p_param fail\n");
        ret = HD_ERR_ABORT;
        goto err_exit;
    }
    for (i = 0; i < num_of_image; i++) {
        if ((p_param[i].dst_pos.x > (int)p_param[i].dst_img.dim.w) || (p_param[i].dst_pos.y > (int)p_param[i].dst_img.dim.h)) {
            HD_GFX_ERR("check rotate dst fail\n");
            ret = HD_ERR_ABORT;
            goto err_exit;
        }

        if (pif_address_ddr_sanity_check(p_param[i].src_img.p_phy_addr[0], p_param[i].src_img.ddr_id) < 0) {
            HD_GFX_ERR("%s:Check src pa(%#lx) ddrid(%d) error\n", __func__, p_param[i].src_img.p_phy_addr[0],  p_param[i].src_img.ddr_id);
            ret = HD_ERR_PARAM;
            goto err_exit;
        }
        if (pif_address_ddr_sanity_check(p_param[i].dst_img.p_phy_addr[0], p_param[i].dst_img.ddr_id) < 0) {
            HD_GFX_ERR("%s:Check dst pa(%#lx) ddrid(%d) error\n", __func__, p_param[i].dst_img.p_phy_addr[0], p_param[i].dst_img.ddr_id);
            ret = HD_ERR_PARAM;
            goto err_exit;
        }
    }
    pthread_mutex_lock(&gfx_lock);
    ret = gfx_roate_imgs(p_param, num_of_image);
    if (ret != HD_OK) {
        HD_GFX_ERR("hd_gfx_rotate:\n");
        for (i = 0; i < num_of_image; i++) {
            HD_GFX_ERR("   src_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) src_rect(%d,%d,%d,%d)\n"
                       "   dst_buf(ddr%d, pa:%#lx) dim(%d,%d) angle(%#x)\n",
                       p_param[i].src_img.ddr_id, p_param[i].src_img.p_phy_addr[0],
                       p_param[i].src_img.dim.w, p_param[i].src_img.dim.h, p_param[i].src_img.format,
                       p_param[i].src_region.x, p_param[i].src_region.y, p_param[i].src_region.w, p_param[i].src_region.h,
                       p_param[i].dst_img.ddr_id, p_param[i].dst_img.p_phy_addr[0],
                       p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h, p_param[i].angle);
        }
    } else {
        HD_GFX_FLOW("hd_gfx_rotate:\n");
        for (i = 0; i < num_of_image; i++) {
            HD_GFX_FLOW("   src_buf(ddr%d, pa:%#lx) dim(%d,%d) pxlfmt(%#x) src_rect(%d,%d,%d,%d)\n"
                        "   dst_buf(ddr%d, pa:%#lx) dim(%d,%d) angle(%#x)\n",
                        p_param[i].src_img.ddr_id, p_param[i].src_img.p_phy_addr[0],
                        p_param[i].src_img.dim.w, p_param[i].src_img.dim.h, p_param[i].src_img.format,
                        p_param[i].src_region.x, p_param[i].src_region.y, p_param[i].src_region.w, p_param[i].src_region.h,
                        p_param[i].dst_img.ddr_id, p_param[i].dst_img.p_phy_addr[0],
                        p_param[i].dst_img.dim.w, p_param[i].dst_img.dim.h, p_param[i].angle);
        }
    }
    pthread_mutex_unlock(&gfx_lock);
err_exit:
    return ret;
}

HD_RESULT hd_gfx_rotate(HD_GFX_ROTATE *p_param)
{
    return __hd_gfx_rotate(p_param, 1);
}


