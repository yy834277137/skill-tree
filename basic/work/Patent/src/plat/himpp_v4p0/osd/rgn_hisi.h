/**
 * @file   rgn_hisi.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  rgn---OSD处理接口封装
 * @author wangweiqin
 * @date   2017年09月08日 Create
 * @note
 * @note \n History
   1.Date        : 2017年09月08日 Create
     Author      : wangweiqin
     Modification: 新建文件
   2.Date        : 2021/06/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */


#ifndef _RGN_HISI_H_
#define _RGN_HISI_H_

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
} RGN_HISI_S;


#ifdef __cplusplus
 }
#endif /* __cplusplus */
 
 
#endif /*_RGN_HISI_H_*/



