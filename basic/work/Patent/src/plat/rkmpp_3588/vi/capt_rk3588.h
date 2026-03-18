/**
 * @file   capt_hisi.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  采集模块平台层头文件
 * @author fangzuopeng
 * @date   2022/7/1
 * @note
 * @note \n History
   1.日    期: 2022/7/1
     作    者: fangzuopeng
     修改历史: 创建文件
 */

#ifndef _CAPT_HISI_H_
#define _CAPT_HISI_H_

#include <sal.h>
#include <dspcommon.h>
#include <platform_sdk.h>
#include <platform_hal.h>
#include "capbility.h"
#include "v4l2-subdev.h"
#include "v4l2-mediabus.h"



/*  硬件mipi驱动配置：
    i2c7 -> csi2_dphy0 -> mipi2_csi2 -> rkcif_mipi_lvds2 <-> media0(v4l-subdev5) <-> video0
    i2c2 -> csi2_dphy3 -> mipi4_csi2 -> rkcif_mipi_lvds4 <-> media1(v4l-subdev2) <-> video11*/

#define INPUT_V4L_SUBDEV0      "/dev/v4l-subdev2"
#define INPUT_V4L_SUBDEV1      "/dev/v4l-subdev5"

#define INPUT_VIDEO_DEV0        "/dev/video11"
#define INPUT_VIDEO_DEV1        "/dev/video0"


/* 通道 属性 */
typedef struct tagCaptHalChnPrmSt
{
    UINT32 uiChn;               /* 当前通道号 */
    UINT32 uiIspId;             /* 对应的ISP  */
    UINT32 uiSnsId;             /* 对应的sensor  */

    UINT32 uiStarted;           /* 是否已开启 */
    UINT32 viDevEn;             /* 是否已使能videv*/
    UINT32 userPic;             /* 是否已使能用户图片*/

    UINT32 uiIspInited;         /* ISP 是否完成初始化 */
    SAL_ThrHndl IspPid;         /* ISP 处理线程       */

    VI_DEV_ATTR_S  stDevAttr;     /* vi 设备属性 */
    VI_PIPE_ATTR_S stPipeAttr;   /* vi 图层属性 */
    VI_CHN_ATTR_S  stChnAttr;     /* vi 通道属性 */
    
    struct v4l2_subdev_format stV412SubdevFormat;       /*mipi 配置*/
    struct v4l2_subdev_selection stV412SubdevCrop;      /*裁剪配置*/

    UINT32         captStatus;    /*采集状态，视频正常0 ，无检测到1，不支持2*/
    void          *MutexHandle;   /* 互斥锁handle */

}CAPT_HAL_CHN_PRM;

/* 模块 属性 */
typedef struct tagCaptModPrmSt
{
    UINT32 uiChnCnt;        /* 采集通道个数 */
    /* 采集通道属性 */
    CAPT_HAL_CHN_PRM stCaptHalChnPrm[CAPT_CHN_NUM_MAX];
}CAPT_MOD_PRM;


#define NEW_LIUXY_VI_ATTR   /*多产品型号不同输入源可配置*/
#ifdef NEW_LIUXY_VI_ATTR


VI_DEV_ATTR_S DEV_ATTR_MIPI_BASE =
{
    /* interface mode */
    VI_MODE_MIPI,
    /* multiplex mode */
    VI_WORK_MODE_2Multiplex,
    
    VI_DATA_SEQ_BUTT,

    VI_DATA_TYPE_RGB,
    
    {1920, 1080},

    DATA_RATE_X1,

};



/* rkdemo板vi pipe的配置 */
VI_PIPE_ATTR_S PIPE_ATTR_1080P_RGB888=
{
    RK_FALSE,
    1920, 1080,
    RK_FMT_RGB888,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_8,
    {60,60}
};



/* rkdemo板vi chn的配置 */
VI_CHN_ATTR_S CHN_ATTR_1080P_RGB888_SDR8_LINEAR =
{
    {1920, 1080},
    RK_FMT_RGB888,
    DYNAMIC_RANGE_SDR8,
    VIDEO_FORMAT_LINEAR,
    COMPRESS_MODE_NONE,
    0,      0,
    1,                          /*需要get时必须设置深度，且小于buf_count */
    { -1, -1},
    VI_ALLOC_BUF_TYPE_INTERNAL,
};

VI_ISP_OPT_S ISP_OPT_1080P_VIDEO0 =
{
    5, 
    0,
    VI_V4L2_CAPTURE_TYPE_VIDEO_CAPTURE,
    VI_V4L2_MEMORY_TYPE_MMAP, 
    INPUT_VIDEO_DEV0,
    RK_TRUE,
    {1920,1080}
};

VI_ISP_OPT_S ISP_OPT_1080P_VIDEO1 =
{
    5, 
    0,
    VI_V4L2_CAPTURE_TYPE_VIDEO_CAPTURE,
    VI_V4L2_MEMORY_TYPE_MMAP, 
    INPUT_VIDEO_DEV1,
    RK_TRUE,
    {1920,1080}
};


#endif



#endif

