/**
 * @file   rgn_ssv5.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  rgn---OSD处理接口封装
 * @author wangweiqin
 * @date   2017年09月08日 Create
 * @note
 * @note \n History
   1.Date        : 2017年09月08日 Create
     Author      : wangweiqin
     Modification: 新建文件
   2.Date        : 2021/08/09
     Author      : yindongping
     Modification: 组件开发，整理接口
 */


#ifndef _RGN_SSV5_H_
#define _RGN_SSV5_H_

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
    ot_rgn_attr      stRgnAttr;
    ot_rgn_chn_attr  stRgnChnAttr;
    ot_bmp           stBitmap;
    ot_rgn_handle    handle;
    BOOL             bCreated;
    BOOL             bAttached;
} RGN_SSV5_S;


#ifdef __cplusplus
 }
#endif /* __cplusplus */
 
 
#endif /*_RGN_HIV5_H_*/



