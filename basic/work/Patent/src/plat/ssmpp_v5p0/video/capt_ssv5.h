/**
 * @file   capt_ssv5.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  采集模块平台层头文件
 * @author liuxianying
 * @date   2021/8/4
 * @note
 * @note \n History
   1.日    期: 2021/8/4
     作    者: liuxianying
     修改历史: 创建文件
 */
#ifndef _CAPT_SSV5_H_
#define _CAPT_SSV5_H_

#include <sal.h>
#include <dspcommon.h>
#include <platform_sdk.h>
#include <platform_hal.h>
#include "capbility.h"

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

    ot_isp_sns_obj stSnsObj;     /* sensor对应结构体   */

    combo_dev_attr_t stcomboDevAttr; /*接口控制器属性mipi控制器属性*/
    ot_isp_pub_attr   stIspPubAttr;

    ot_vi_dev_attr  stDevAttr;     /* vi 设备属性 */
    ot_vi_chn_attr  stChnAttr;     /* vi 通道属性 */
    ot_vi_wdr_fusion_grp_attr  stWDRAttr;     /* vi 宽动态属性 */
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
combo_dev_attr_t LVDS_4lane_Fpga_16BIT_720p_NOWDR_ATTR0 =
{
    .devno         = 0,
    .input_mode    = INPUT_MODE_LVDS,      /* input mode */
    .data_rate     = MIPI_DATA_RATE_X1,
    .img_rect      = {0, 0, 1920, 1080},
    .lvds_attr =
    {
        .input_data_type  = DATA_TYPE_RAW_16BIT,
        .wdr_mode         = OT_WDR_MODE_NONE,
        .sync_mode        = LVDS_SYNC_MODE_SOF,
        .vsync_attr       = {LVDS_VSYNC_NORMAL, 0, 0},
        .fid_attr         = {LVDS_FID_NONE, TD_FALSE},
        .data_endian      = LVDS_ENDIAN_BIG,
        .sync_code_endian = LVDS_ENDIAN_BIG,
        .lane_id = {0,  1,  2,  3,  -1, -1, -1, -1},   
        .sync_code = {
            {   
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 0
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            },
            /*   sync_mode is SYNC_MODE_SAV: invalid sav, invalid eav, valid sav, valid eav  */
            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 1
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            },
            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 2
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            },
            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 3
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            }
        }
    }
};

combo_dev_attr_t LVDS_4lane_Fpga_16BIT_720p_NOWDR_ATTR1 =
{
    .devno         = 2,
    .input_mode    = INPUT_MODE_LVDS,      /* input mode */
    .data_rate     = MIPI_DATA_RATE_X1,
    .img_rect      = {0, 0, 1920, 1080},

    .lvds_attr =
    {
        .input_data_type    = DATA_TYPE_RAW_16BIT,
        .wdr_mode         = OT_WDR_MODE_NONE,
        .sync_mode        = LVDS_SYNC_MODE_SOF,
        .vsync_attr       = {LVDS_VSYNC_NORMAL, 0, 0},
        .fid_attr         = {LVDS_FID_NONE, TD_FALSE},
        .data_endian      = LVDS_ENDIAN_BIG,
        .sync_code_endian = LVDS_ENDIAN_BIG,
        .lane_id = {4,  5, 6, 7, -1, -1, -1, -1},
         /* sync_mode is SYNC_MODE_SOF: SOF, EOF, SOL, EOL  
            lvds传输同步码 ，需要和输入芯片设置成一致，R7输入芯片FPGA*/
        .sync_code = {
            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 8
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            },
            /*   sync_mode is SYNC_MODE_SAV: invalid sav, invalid eav, valid sav, valid eav  */
            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 9
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            },

            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 10
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            },

            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 11
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            }
        }
    }
};


ot_vi_dev_attr DEV_ATTR_LVDS_BASE =
{
    /* interface mode */
    OT_VI_INTF_MODE_MIPI, /*VI_MODE_LVDS,  sd3403没有lvds选项，和hisi技术支持沟通lvds合并到mipi模式*/
    /* multiplex mode */
    OT_VI_WORK_MODE_MULTIPLEX_1,
    /* r_mask    g_mask    b_mask*/
    {0xFF000000,    0xFF0000},
    /* progessive or interleaving */
    OT_VI_SCAN_PROGRESSIVE,
    
    { -1, -1, -1, -1},
        
    OT_VI_DATA_SEQ_UYVY,

    /* synchronization information */
    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        OT_VI_VSYNC_PULSE, OT_VI_VSYNC_NEG_LOW, OT_VI_HSYNC_VALID_SIG, OT_VI_HSYNC_NEG_HIGH, OT_VI_VSYNC_VALID_SIG, OT_VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1280,        0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            720,        0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    /* input data type */
    OT_VI_DATA_TYPE_YUV,
    /* bRever */
    TD_FALSE,
    /* DEV CROP */
    {1920, 1080},
    OT_DATA_RATE_X1
};


ot_vi_dev_attr DEV_ATTR_BT1120_BASE =
{
    /* interface mode */
    OT_VI_INTF_MODE_BT1120,
    /* multiplex mode */
    OT_VI_WORK_MODE_MULTIPLEX_1,
    /* r_mask    g_mask    b_mask*/
    {0xFF000000,    0x00FF0000},
    /* progessive or interleaving */
    OT_VI_SCAN_PROGRESSIVE,
    
    { -1, -1, -1, -1},
        
    OT_VI_DATA_SEQ_UVUV,

    /* synchronization information */
    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        OT_VI_VSYNC_PULSE, OT_VI_VSYNC_NEG_LOW, OT_VI_HSYNC_VALID_SIG, OT_VI_HSYNC_NEG_HIGH, OT_VI_VSYNC_VALID_SIG, OT_VI_VSYNC_VALID_NEG_HIGH,

        /*hsync_hfb    hsync_act    hsync_hhb*/
        {
            0,            1920,         0,
            /*vsync0_vhb vsync0_act vsync0_hhb*/
            0,            1080,         0,
            /*vsync1_vhb vsync1_act vsync1_hhb*/
            0,            0,            0
        }
    },
    /* input data type */
    OT_VI_DATA_TYPE_YUV,
    /* bRever */
    TD_FALSE,
    /* DEV CROP */
    {1920, 1080},
    OT_DATA_RATE_X1
};


/* 海思demo板vi pipe的配置 */
ot_vi_pipe_attr PIPE_ATTR_720P_RAW16_420_3DNR_RFR = 
{
    OT_VI_PIPE_BYPASS_NONE,
    TD_TRUE,                  /*是否开启isp TD_FALSE正常开启*/
    {1920, 1080},
    OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    OT_COMPRESS_MODE_NONE,
    OT_DATA_BIT_WIDTH_8,
    OT_VI_BIT_ALIGN_MODE_HIGH,                            // lxy20210803 OT_VI_BIT_ALIGN_MODE_HIGH
    { -1, -1}
};


/* YUV422输入，chn中转换为YUV420 */
ot_vi_pipe_attr PIPE_ATTR_720P_YUV422_3DNR_RFR = 
{
    OT_VI_PIPE_BYPASS_NONE,
    TD_TRUE,                     /*是否开启isp TD_FALSE正常开启*/
    {1920, 1080},
    OT_PIXEL_FORMAT_YVU_SEMIPLANAR_422,
    OT_COMPRESS_MODE_NONE,
    OT_DATA_BIT_WIDTH_8,
    OT_VI_BIT_ALIGN_MODE_HIGH,                            // lxy20210803 OT_VI_BIT_ALIGN_MODE_HIGH
    { -1, -1}
};


/* 海思demo板vi chn的配置 */
ot_vi_chn_attr CHN_ATTR_720P_420_SDR8_LINEAR =
{
    {1920, 1080},
    OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    OT_DYNAMIC_RANGE_SDR8,
    OT_VIDEO_FORMAT_LINEAR,
    OT_COMPRESS_MODE_NONE,
    0,      0,
    1,
    { -1, -1}
};

ot_isp_pub_attr ISP_PUB_ATTR_fpga_60FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    60,
    OT_ISP_BAYER_RGGB,
    OT_WDR_MODE_NONE,
    0,
};
#endif



#endif

