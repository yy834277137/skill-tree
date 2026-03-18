/*******************************************************************************
* vda_drv.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : wutao19 <wutao19@hikvision.com.cn>
* Version: V1.0.0  2019年7月10日 Create
*
* Description : 硬件平台移动侦测模块
* Modification:
*******************************************************************************/
#ifndef _VDA_DRV_H_
#define _VDA_DRV_H_

/*****************************************************************************
                                头文件
*****************************************************************************/
#include "sal.h"
#include "platform_sdk.h"
#include <sys/ioctl.h>

#include "dspcommon.h"
#include "platform_hal.h"

/*****************************************************************************
                                宏定义
*****************************************************************************/

#define DRV_MAX_VDA_CHN_NUM     (2)

#define DRV_MAX_MD_WIDTH     (704)
#define DRV_MAX_MD_HEIGHT    (576)


/*****************************************************************************
                                结构体定义
*****************************************************************************/
/* 模块 属性 */
typedef struct tagVdaDrvChnPrmSt
{
    UINT32 uiChn;                /* 通道号 */
    UINT32 uiLevel;              /* 灵敏度等级 */
}VDA_DRV_CHN_PRM;


/* Jpeg Dev 信息 普通抓图的业务信息 */
typedef struct tagVdaDrvDevInfoSt
{

    UINT32  isCreate;
    UINT32  isStart;            /* 是否开始*/
    void   *ChnHandle;         /* 通道创建的句柄*/
    void   *DupHandle;
    UINT32  width;
    UINT32  height;
	UINT32  uiChn;                /* 通道号 */
    UINT32  uiLevel;              /* 灵敏度等级 */
	UINT32  firstFrame;
    SYSTEM_FRAME_INFO stSysFrame; 
    SAL_ThrHndl hndl;            /* 处理线程 */  
    void *MutexHandle;           /* 锁和信号量的handle */
} VDA_DRV_DEV_INFO;


/* 模块 属性 */
typedef struct tagVdaModDrvPrmSt
{
    UINT32 uiChnNum;                /* 已创建的通道个数 */
    VDA_DRV_DEV_INFO stVdaDrvChnPrm[DRV_MAX_VDA_CHN_NUM]; /* 每个通道的信息 */
}VDA_MOD_DRV_PRM;


    

/*****************************************************************************
                            函数声明
*****************************************************************************/
/*****************************************************************************
 函 数 名  : Vda_halMdChnSetLevel
 功能描述  : 移动侦测 参数
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdChnSet(UINT32 uiChn, MD_CONFIG *pCfgPrm);


/*****************************************************************************
 函 数 名  : Vda_halMdChnStop
 功能描述  : 移动侦测通道结束
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdChnStop(UINT32 uiChn);

/*****************************************************************************
 函 数 名  : Vda_halMdChnStart
 功能描述  : 移动侦测通道开始
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdChnStart(UINT32 uiChn);


/*****************************************************************************
 函 数 名  : Vda_halMdInit
 功能描述  : 移动侦测模块初始化
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdInit(void);


/*****************************************************************************
 函 数 名  : Vda_halMdChnCreate
 功能描述  : 移动侦测通道创建
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
INT32 Vda_DrvMdChnCreateMode(UINT32 uiChn,void *DupHandle);




#endif /* _VDA_HAL_DRV_H_*/
