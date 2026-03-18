/**
 * @file   fb_rk3588.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  图形层hiv4p抽象层接口
 * @author yeyanzhong
 * @date   2022.3.15
 * @note
 * @note \n History
   1.日    期: 2022.3.15
     作    者: yeyanzhong
     修改历史: 创建文件
 */

#ifndef _FB_RK3588_H_
#define _FB_RK3588_H_

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <sal.h>
#include <platform_sdk.h>

#include <dspcommon.h>

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#define BMP_HEAD_LEN		(54)
#define BMP_HEAD_INFO_LEN	(40)


/* app上层下发的fb号,原先是基于海思平台来的 */
#define GRAPHICS_LAYER_G0	0 
#define GRAPHICS_LAYER_G1	1
#define GRAPHICS_LAYER_G3	2

#define VO_LAYER_CLUSTER0         0  /* cluster作为视频层 */
#define VO_LAYER_CLUSTER1         1
#define VO_LAYER_CLUSTER2         2
#define VO_LAYER_CLUSTER3         3
#define VO_LAYER_ESMART0          4  /* 约定作为UI图形层0 */
#define VO_LAYER_ESMART1          5  /* 约定作为UI图形层1 */
#define VO_LAYER_ESMART2          6  /* 约定作为鼠标层，绑定UI图形层0*/
#define VO_LAYER_ESMART3          7  /* 约定作为鼠标层，绑定UI图形层1 */

#define MOUSE_WIDTH			(32)
#define MOUSE_HEIGHT		(32)



#endif


