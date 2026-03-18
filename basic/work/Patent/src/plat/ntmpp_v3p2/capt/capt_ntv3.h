/**
 * @file   capt_ntv3.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  采集模块平台层头文件
 * @author liuxianying
 * @date   2022/4/13
 * @note
 * @note \n History
   1.日    期: 2022/4/13
     作    者: liuxianying
     修改历史: 创建文件
 */
#ifndef _CAPT_NTV3_H_
#define _CAPT_NTV3_H_

#include <sal.h>
#include <dspcommon.h>
#include <platform_sdk.h>
#include <platform_hal.h>
#include "hdal_33x/k_driver/include/ssenif_ioctl.h"
#include "hdal_33x/include/vcap_ioctl.h"
#include "capbility.h"



#define NT_AD_PLAT_CHIP_VI_MAX      1//4:31x, 32x  8:33x
#define NT_CAPT_DEV_NUM             1
#define NT_AD_PLAT_VCAP_VI_MAX      4
#define NT_VI_DEV_MAX_CHN_NUM       4


/* 通道 属性 */
typedef struct tagNtCaptChnPrmSt
{
    UINT32 uiPath;
    UINT32 uiChn;               /* 当前通道号 */
    UINT32 uiIspId;             /* 对应的ISP  */
    UINT32 uiSnsId;             /* 对应的sensor  */

    UINT32 uiStarted;           /* 是否已开启 */
    UINT32 viDevEn;             /* 是否已使能videv*/
    UINT32 userPic;             /* 是否已使能用户图片*/
    UINT32 uiIspInited;         /* ISP 是否完成初始化 */
    UINT32         captStatus;    /*采集状态，视频正常0 ，无检测到1，不支持2*/
    void          *MutexHandle;   /* 互斥锁handle */

}NT_CAPT_CHN_PRM;

/* 模块 属性 */
typedef struct tagCaptModPrmSt
{
    UINT32 uiChnCnt;        /* 采集通道个数 */
    /* 采集通道属性 */
    NT_CAPT_CHN_PRM stCaptHalChnPrm[CAPT_CHN_NUM_MAX];
}CAPT_MOD_PRM;

#endif

