/**
 * @file   rgn_rk.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  rgn---OSD处理接口封装
 * @author wangzhenya5
 * @date   2022年03月31日 Create
 * @note
 */


#ifndef _RGN_RK_H_
#define _RGN_RK_H_

#include <sal.h>
#include <platform_hal.h>
#include "hal_inc_inter/osd_hal_inter.h"


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* OSD 管理信息 */
typedef struct
{
    RGN_ATTR_S      stRgnAttr;
    RGN_CHN_ATTR_S  stRgnChnAttr;
    BITMAP_S        stBitmap;
    RGN_HANDLE      handle;
    BOOL            bCreated;
    BOOL            bAttached;
} RGN_RK_S;


#ifdef __cplusplus
 }
#endif /* __cplusplus */
 
 
#endif /*_RGN_HISI_H_*/



