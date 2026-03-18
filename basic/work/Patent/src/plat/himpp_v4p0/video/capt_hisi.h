/**
 * @file   capt_hisi.h
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

#ifndef _CAPT_HISI_H_
#define _CAPT_HISI_H_

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

    ISP_SNS_OBJ_S stSnsObj;     /* sensor对应结构体   */

    combo_dev_attr_t stcomboDevAttr;
    ISP_PUB_ATTR_S   stIspPubAttr;

    VI_DEV_ATTR_S  stDevAttr;     /* vi 设备属性 */
    VI_CHN_ATTR_S  stChnAttr;     /* vi 通道属性 */
    VI_WDR_ATTR_S  stWDRAttr;     /* vi 宽动态属性 */
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
        .wdr_mode         = HI_WDR_MODE_NONE,
        .sync_mode        = LVDS_SYNC_MODE_SOF,
        .vsync_attr       = {LVDS_VSYNC_NORMAL, 0, 0},
        .fid_attr         = {LVDS_FID_NONE, HI_FALSE},
        .data_endian      = LVDS_ENDIAN_BIG,
        .sync_code_endian = LVDS_ENDIAN_BIG,
#ifdef HI3519A_SPC020    
        .lane_id = {0,  1,  2,  3,  -1, -1, -1, -1, -1, -1, -1, -1},
#else
        .lane_id = {0,  1,  2,  3,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
#endif        
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
        .wdr_mode         = HI_WDR_MODE_NONE,
        .sync_mode        = LVDS_SYNC_MODE_SOF,
        .vsync_attr       = {LVDS_VSYNC_NORMAL, 0, 0},
        .fid_attr         = {LVDS_FID_NONE, HI_FALSE},
        .data_endian      = LVDS_ENDIAN_BIG,
        .sync_code_endian = LVDS_ENDIAN_BIG,
#ifdef HI3519A_SPC020        
        .lane_id = {4,  5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1},
#else
        .lane_id = {4,  5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
#endif       
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

combo_dev_attr_t LVDS_4lane_Fpga_16BIT_720p_NOWDR_ATTR2 =
{
    .devno         = 6,
    .input_mode    = INPUT_MODE_LVDS,      /* input mode */
    .data_rate     = MIPI_DATA_RATE_X1,
    .img_rect      = {0, 0, 1920, 1080},

    .lvds_attr =
    {
        .input_data_type  = DATA_TYPE_RAW_16BIT,
        .wdr_mode         = HI_WDR_MODE_NONE,
        .sync_mode        = LVDS_SYNC_MODE_SOF,
        .vsync_attr       = {LVDS_VSYNC_NORMAL, 0, 0},
        .fid_attr         = {LVDS_FID_NONE, HI_FALSE},
        .data_endian      = LVDS_ENDIAN_BIG,
        .sync_code_endian = LVDS_ENDIAN_BIG,
#ifdef HI3519A_SPC020
        .lane_id = {8, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1, -1},
#else
        .lane_id = {12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
#endif

        .sync_code = {
            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 12
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            },   
            /*   sync_mode is SYNC_MODE_SAV: invalid sav, invalid eav, valid sav, valid eav  
                SOF, EOF, SOL, EOL
            */
            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 13
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            },

            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 14
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            },

            {
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},      // lane 15
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d},
                {0xABAB, 0xB6B6, 0x8080, 0x9d9d}
            }
        }
    }
};

VI_DEV_ATTR_S DEV_ATTR_LVDS_BASE =
{
    /* interface mode */
    VI_MODE_LVDS,
    /* multiplex mode */
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFF000000,    0xFF0000},
    /* progessive or interleaving */
    VI_SCAN_PROGRESSIVE,
    
    { -1, -1, -1, -1},
        
    VI_DATA_SEQ_YUYV,

    /* synchronization information */
    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

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
    VI_DATA_TYPE_YUV,
    /* bRever */
    HI_FALSE,
    /* DEV CROP */
    {1920, 1080},
    {
        {
            {1920, 1080},
            //HI_FALSE,

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1
};


VI_DEV_ATTR_S DEV_ATTR_BT1120_BASE =
{
    /* interface mode */
    VI_MODE_BT1120_STANDARD,
    /* multiplex mode */
    VI_WORK_MODE_1Multiplex,
    /* r_mask    g_mask    b_mask*/
    {0xFF000000,    0x00FF0000},
    /* progessive or interleaving */
    VI_SCAN_PROGRESSIVE,
    
    { -1, -1, -1, -1},
        
    VI_DATA_SEQ_UVUV,

    /* synchronization information */
    {
        /*port_vsync   port_vsync_neg     port_hsync        port_hsync_neg        */
        VI_VSYNC_PULSE, VI_VSYNC_NEG_LOW, VI_HSYNC_VALID_SINGNAL, VI_HSYNC_NEG_HIGH, VI_VSYNC_VALID_SINGAL, VI_VSYNC_VALID_NEG_HIGH,

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
    VI_DATA_TYPE_YUV,
    /* bRever */
    HI_FALSE,
    /* DEV CROP */
    {1920, 1080},
    {
        {
            {1920, 1080},
            //HI_FALSE,

        },
        {
            VI_REPHASE_MODE_NONE,
            VI_REPHASE_MODE_NONE
        }
    },
    {
        WDR_MODE_NONE,
        1080
    },
    DATA_RATE_X1
};


/* 海思demo板vi pipe的配置 */
VI_PIPE_ATTR_S PIPE_ATTR_720P_RAW16_420_3DNR_RFR =
{
    VI_PIPE_BYPASS_NONE, HI_FALSE,HI_TRUE,
    1920, 1080,
    PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_8,
    HI_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    HI_FALSE,
    { -1, -1}
};

/* YUV422输入，chn中转换为YUV420 */
VI_PIPE_ATTR_S PIPE_ATTR_720P_YUV422_3DNR_RFR =
{
    VI_PIPE_BYPASS_NONE, HI_FALSE,HI_TRUE,
    1920, 1080,
    PIXEL_FORMAT_YVU_SEMIPLANAR_422,
    COMPRESS_MODE_NONE,
    DATA_BITWIDTH_8,
    HI_FALSE,
    {
        PIXEL_FORMAT_YVU_SEMIPLANAR_420,
        DATA_BITWIDTH_8,
        VI_NR_REF_FROM_RFR,
        COMPRESS_MODE_NONE
    },
    HI_FALSE,
    { -1, -1}
};


/* 海思demo板vi chn的配置 */
VI_CHN_ATTR_S CHN_ATTR_720P_420_SDR8_LINEAR =
{
    {1920, 1080},
    PIXEL_FORMAT_YVU_SEMIPLANAR_420,
    DYNAMIC_RANGE_SDR8,
    VIDEO_FORMAT_LINEAR,
    COMPRESS_MODE_NONE,
    0,      0,
    1,
    { -1, -1}
};

ISP_PUB_ATTR_S ISP_PUB_ATTR_fpga_60FPS =
{
    {0, 0, 1920, 1080},
    {1920, 1080},
    60,
    BAYER_RGGB,
    WDR_MODE_NONE,
    0,
};
#endif



#endif

