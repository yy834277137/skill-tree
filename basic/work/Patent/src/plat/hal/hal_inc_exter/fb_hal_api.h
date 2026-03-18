/*******************************************************************************
* fb_hal_drv.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wangweiqin <wangweiqin@hikvision.com.cn>
* Version: V1.0.0  2017年09月08日 Create
*
* Description : 硬件平台 fb 模块
* Modification:
*******************************************************************************/
#ifndef _FB_HAL_H_
#define _FB_HAL_H_


#include "sal.h"

#include <linux/fb.h>
#include <sys/mman.h>



/*****************************************************************************
                            宏定义
*****************************************************************************/





/*****************************************************************************
                            结构体定义
*****************************************************************************/
/* 图形界面信息 */
typedef struct tagDispHalFbAttrSt
{
    INT8   *cAddr;   /* FB 地址   */
    UINT32  uiSize;  /* FB 的大小 */

    UINT32 uiW;      /* 内存宽   */
    UINT32 uiH;      /* 内存高   */
    UINT32 uiStride; /* 内存跨度 */
}DISP_HAL_FB_ATTR_ST;

/* 图形界面信息 */
typedef struct tagFbHalInitAttrSt
{
    UINT32 uiW;      /* 内存宽   */
    UINT32 uiH;      /* 内存高   */
}FB_INIT_ATTR_ST;


/* bmp image color palette 4*256=1024 bytes */
typedef struct tagRGBQUAD
{
    char   rgbBlue;
    char   rgbGreen;
    char   rgbRed;
    char   rgbReserved;
} RGBQUAD;

/*******************************************************************************
* 函数名  : fb_hal_drv_ChangeMouseDevChn
* 描  述  : 设置鼠标层对用FB通道的绑定关系
* 输  入  : - uiBindChn
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 fb_hal_setMouseFbChn(UINT32 uiBindChn);

/*******************************************************************************
* 函数名  : fb_hal_drv_ChangeMouseDevChn
* 描  述  : 刷新FB显示内容
* 输  入  : - uiBindChn
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 fb_hal_refreshFb(UINT32 uiChn, SCREEN_CTRL *pstHalFbAttr);

/**
 * @function   fb_hal_setPos
 * @brief      设置位置
 * @param[in]  UINT32 uiChn     设备号 
 * @param[in]  POS_ATTR *pstPos 位置信息
 * @param[out] None
 * @return     INT32  SAL_SOK   成功
 *                    SAL_FAIL  失败
 */
INT32 fb_hal_setPos(UINT32 uiChn, POS_ATTR *pstPos);

/*******************************************************************************
* 函数名  : fb_hal_snapFb
* 描  述  : 抓取FB显示数据
* 输  入  : - uiChn:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 fb_hal_snapFb(UINT32 uiChn);

/*******************************************************************************
* 函数名  : fb_hal_releaseFb
* 描  述  : 释放fb缓存
* 输  入  : - uiChn:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 fb_hal_releaseFb(UINT32 uiChn);

/*******************************************************************************
* 函数名  : fb_hal_getFrameBuffer
* 描  述  : 获取fb缓存地址和app保持
* 输  入  : - uiChn       :对应设备号
*       : - pstHalFbAttr:显示参数
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 fb_hal_getFrameBuffer(UINT32 uiChn, SCREEN_CTRL *pstHalFbAttr,FB_INIT_ATTR_ST *pstFbPrm);

/*******************************************************************************
* 函数名  : fb_hal_checkFbIllegal
* 描  述  : 校验设备号是否超出设备最大值
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 fb_hal_checkFbIllegal(UINT32 chan);


/*******************************************************************************
* 函数名  : fb_hal_Init
* 描  述  : 初始化fb结构体
* 输  入  : - chan:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 fb_hal_Init(void);




#endif /* _FB_HAL_DRV_H_ */


