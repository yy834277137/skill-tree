/**
 * @file   disp_ssv5.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  DISP显示hiv5p0抽象层接口
 * @author liuxianying
 * @date   2021/8/4
 * @note
 * @note \n History
   1.日    期: 2021/8/4
     作    者: liuxianying
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <platform_sdk.h>
#include <platform_hal.h>

#include "capbility.h"
#include "disp_ssv5.h"
#include "../../hal/hal_inc_inter/disp_hal_inter.h"



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define DISP_EDID_TOTAL_MAX (512)               /* EDID最大值 */
#define DISP_PPL_FREF       (24)                /* 时钟源固定24M */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

static DISP_PLAT_OPS g_stDispPlatOps;
/* 自定义需要支持的时序，每新增一种分辨率时序都需要增加到该数组中 */
static const DISP_TIMING_MAP_S gastDispTimingMap[] = {
    {1920, 1080, 72, OT_VO_OUT_USER, TIMING_CVT_1920X1080P72_R2},
    {1920, 1080, 60, OT_VO_OUT_1080P60, TIMING_VESA_1920X1080P60},
    {1920, 1080, 50, OT_VO_OUT_1080P50,   TIMING_CVT_1920X1080P50},

    {1680, 1050, 60, OT_VO_OUT_1680x1050_60, TIMING_CVT_1680X1050P60},

    {1280, 1024, 85, OT_VO_OUT_USER, TIMING_VESA_1280X1024P85},
    {1280, 1024, 75, OT_VO_OUT_USER, TIMING_VESA_1280X1024P75},
    {1280, 1024, 72, OT_VO_OUT_USER, TIMING_GTF_1280X1024P72},
    {1280, 1024, 70, OT_VO_OUT_USER, TIMING_GTF_1280X1024P70},
    {1280, 1024, 60, OT_VO_OUT_1280x1024_60, TIMING_VESA_1280X1024P60},

    {1280, 800, 85, OT_VO_OUT_USER, TIMING_VESA_1280X800P85},
    {1280, 800, 75, OT_VO_OUT_USER, TIMING_CVT_1280X800P75},
    {1280, 800, 72, OT_VO_OUT_USER, TIMING_GTF_1280X800P72},
    {1280, 800, 60, OT_VO_OUT_1280x800_60, TIMING_CVT_1280X800P60},

    {1280, 768, 85, OT_VO_OUT_USER, TIMING_VESA_1280X768P85},
    {1280, 768, 75, OT_VO_OUT_USER, TIMING_VESA_1280X768P75},
    {1280, 768, 72, OT_VO_OUT_USER, TIMING_GTF_1280X768P72},
    {1280, 768, 60, OT_VO_OUT_USER, TIMING_CVT_1280X768P60},

    {1280, 720, 72, OT_VO_OUT_USER, TIMING_GTF_1280X720P72},
    {1280, 720, 60, OT_VO_OUT_720P60, TIMING_VESA_1280X720P60},
    {1280, 720, 50, OT_VO_OUT_USER, TIMING_CVT_1280X720P50},

    {1024, 768, 85, OT_VO_OUT_USER, TIMING_VESA_1024X768P85},
    {1024, 768, 75, OT_VO_OUT_USER, TIMING_CVT_1024X768P75},
    {1024, 768, 70, OT_VO_OUT_USER, TIMING_VESA_1024X768P70},
    {1024, 768, 72, OT_VO_OUT_USER, TIMING_CVT_1024X768P72},
    {1024, 768, 60, OT_VO_OUT_1024x768_60, TIMING_VESA_1024X768P60},
};

/* HDMI支持的分辨率时序，参考数据手册 */
static const ot_vo_intf_sync aenHdmiSupportRes[VO_DEV_MAX][OT_VO_OUT_BUTT] = {
    /* DHD0 */
    {
        OT_VO_OUT_1080P24, OT_VO_OUT_1080P25, OT_VO_OUT_1080P30, OT_VO_OUT_720P50,
        OT_VO_OUT_720P60, OT_VO_OUT_1080P50, OT_VO_OUT_1080P60, OT_VO_OUT_576P50,
        OT_VO_OUT_480P60, OT_VO_OUT_800x600_60, OT_VO_OUT_1024x768_60, OT_VO_OUT_1280x1024_60,
        OT_VO_OUT_1366x768_60, OT_VO_OUT_1440x900_60, OT_VO_OUT_1280x800_60, OT_VO_OUT_1600x1200_60,
        OT_VO_OUT_1680x1050_60, OT_VO_OUT_1920x1200_60, OT_VO_OUT_640x480_60, OT_VO_OUT_1920x2160_30,
        OT_VO_OUT_2560x1440_30, OT_VO_OUT_2560x1440_60, OT_VO_OUT_2560x1600_60, OT_VO_OUT_3840x2160_24,
        OT_VO_OUT_3840x2160_25, OT_VO_OUT_3840x2160_30, OT_VO_OUT_3840x2160_50, OT_VO_OUT_3840x2160_60,
        OT_VO_OUT_4096x2160_24, OT_VO_OUT_4096x2160_25, OT_VO_OUT_4096x2160_30, OT_VO_OUT_4096x2160_50,
        OT_VO_OUT_4096x2160_60, OT_VO_OUT_USER, OT_VO_OUT_BUTT,
    },

    /* DHD1 */
    {
        OT_VO_OUT_BUTT,
    },
};

/* MIPI支持的分辨率时序，参考数据手册 */
static const ot_vo_intf_sync aenMipiSupportRes[VO_DEV_MAX][OT_VO_OUT_BUTT] = {
    /* DHD0 */
    {
        OT_VO_OUT_720P50, OT_VO_OUT_720P60, OT_VO_OUT_1080P60, OT_VO_OUT_576P50,
        OT_VO_OUT_1024x768_60, OT_VO_OUT_1280x1024_60, OT_VO_OUT_3840x2160_30,
        OT_VO_OUT_3840x2160_60, OT_VO_OUT_720x1280_60, OT_VO_OUT_1080x1920_60,
        OT_VO_OUT_USER, OT_VO_OUT_BUTT,
    },

    /* DHD1 */
    {
        OT_VO_OUT_576P50,OT_VO_OUT_720P50,OT_VO_OUT_720P60,OT_VO_OUT_1024x768_60,
        OT_VO_OUT_1280x1024_60,OT_VO_OUT_1080P24,OT_VO_OUT_1080P25,OT_VO_OUT_1080P30,
        OT_VO_OUT_1080P60,OT_VO_OUT_720x1280_60,OT_VO_OUT_1080x1920_60, 
        OT_VO_OUT_USER, OT_VO_OUT_BUTT,
    },

       
};


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

extern void Mpi_Tx_Start(const BT_TIMING_S *pstTiming);
extern void Mpi_Tx_Stop(void);
extern INT32 Hdmi_Hal_Stop(VOID);
extern INT32 Hdmi_Hal_SetHdmi(UINT32 uiVoLayer, const BT_TIMING_S *pstTiming);

/**
 * @function    disp_ssv5GetTiming
 * @brief     获取显示设备参数sdk映射信息
 * @param[in] pDispDev 设备参数
 * @param[out] pstDispTiming 映射后信息
 * @return
 */
static const DISP_TIMING_MAP_S *disp_ssv5GetTiming(DISP_DEV_COMMON *pDispDev)
{
    INT32 i = 0;
    const DISP_TIMING_MAP_S *pstDispTiming = NULL;
    UINT32 u32ResNum = sizeof(gastDispTimingMap) / sizeof(gastDispTimingMap[0]);

    for (i = 0; i < u32ResNum; i++)
    {
        pstDispTiming = gastDispTimingMap + i;
        if ((pstDispTiming->u32Width == pDispDev->uiDevWith)
            && (pstDispTiming->u32Height == pDispDev->uiDevHeight)
            && (pstDispTiming->u32Fps == pDispDev->uiDevFps))
        {
            break;
        }
    }

    if ((i >= u32ResNum) || (pstDispTiming->enUserTiming >= TIMING_BUTT))
    {
        DISP_LOGE("unsupport resolution[%uX%u@%u]", pDispDev->uiDevWith, pDispDev->uiDevHeight, pDispDev->uiDevFps);
        return NULL;
    }

    return pstDispTiming;

}

/**
 * @function   disp_ssv5DevBlanking
 * @brief      消隐区设置
 * @param[in]  UINT32 uiDevWith              
 * @param[in]  UINT32 uiDevHeight            
 * @param[in]  ot_vo_pub_attr *pstVoPubAttr  设置消隐区参数前后场信息
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_ssv5DevBlanking(UINT32 uiDevWith, UINT32 uiDevHeight, ot_vo_pub_attr *pstVoPubAttr)
{
    UINT32 u32Addr = 0x12010044;
    UINT32 u32Value = 0;

    if (NULL == pstVoPubAttr)
    {
        DISP_LOGE("!!!\n");

        return SAL_FAIL;
    }

    /* 1. 配置vo 属性 */
    /* pstVoPubAttr->enIntfSync = OT_VO_OUT_USER; */

    pstVoPubAttr->sync_info.syncm = 1;
    pstVoPubAttr->sync_info.iop = 1;
    pstVoPubAttr->sync_info.intfb = 1;


    /* 宽和高为自定义值，写成魔鬼数字 */
    if ((2048 == uiDevWith) && (600 == uiDevHeight))
    {
        /* 1024*600 修改 vo 输出频率到 107 M*/
/*        OT_MPI_SYS_GetReg(u32Addr, &u32Value); */

        DISP_LOGI("u32Addr : %x u32Value : %x \n", u32Addr, u32Value);
/*        OT_MPI_SYS_SetReg(u32Addr, 0xc000); */

        pstVoPubAttr->sync_info.vact = 600;
        pstVoPubAttr->sync_info.vbb = 23;
        pstVoPubAttr->sync_info.vfb = 12;
        pstVoPubAttr->sync_info.hact = 2048;
        pstVoPubAttr->sync_info.hbb = 440;
        pstVoPubAttr->sync_info.hfb = 320;
        pstVoPubAttr->sync_info.hmid= 1;

        DISP_LOGI("Set Sync Info For 1024*600 \n");
    }
    else if ((2560 == uiDevWith)
             && (800 == uiDevHeight))
    {
        pstVoPubAttr->sync_info.vact = 800;
        pstVoPubAttr->sync_info.vbb = 22;
        pstVoPubAttr->sync_info.vfb = 1;

        pstVoPubAttr->sync_info.hact = 2560;
        pstVoPubAttr->sync_info.hbb = 233;
        pstVoPubAttr->sync_info.hfb = 214;
        pstVoPubAttr->sync_info.hmid = 1;

        DISP_LOGI("Set Sync Info For 1280*800 \n");
    }
    else if ((1280 == uiDevWith) && (720 == uiDevHeight))
    {
        /* 1024*600 修改 vo 输出频率到 107 M*/
/*        OT_MPI_SYS_GetReg(u32Addr, &u32Value); */

        DISP_LOGI("u32Addr : %x u32Value : %x \n", u32Addr, u32Value);
/*        OT_MPI_SYS_SetReg(u32Addr, 0xc000); */

        pstVoPubAttr->sync_info.vact = 720;
        pstVoPubAttr->sync_info.vbb = 23;
        pstVoPubAttr->sync_info.vfb = 12;
        pstVoPubAttr->sync_info.hact = 1280;
        pstVoPubAttr->sync_info.hbb = 440;
        pstVoPubAttr->sync_info.hfb = 320;
        pstVoPubAttr->sync_info.hmid = 1;

        DISP_LOGI("Set Sync Info For 1280*720 \n");
    }
    else
    {
        DISP_LOGE("!!!\n");

        /* 1024*600 修改 vo 输出频率到 107 M*/
/*        OT_MPI_SYS_GetReg(u32Addr, &u32Value); */

        DISP_LOGI("u32Addr : %x u32Value : %x \n", u32Addr, u32Value);
/*        OT_MPI_SYS_SetReg(u32Addr, 0xc000); */

        pstVoPubAttr->sync_info.vact = 600;
        pstVoPubAttr->sync_info.vbb = 23;
        pstVoPubAttr->sync_info.vfb = 12;
        pstVoPubAttr->sync_info.hact = 2048;
        pstVoPubAttr->sync_info.hbb = 440;
        pstVoPubAttr->sync_info.hfb = 320;
        pstVoPubAttr->sync_info.hmid = 1;
        return SAL_FAIL;
    }

    pstVoPubAttr->sync_info.bvact = 1;
    pstVoPubAttr->sync_info.bvbb = 1;
    pstVoPubAttr->sync_info.bvfb = 1;

    pstVoPubAttr->sync_info.hpw = 20;
    pstVoPubAttr->sync_info.vpw = 3;

    pstVoPubAttr->sync_info.idv = 0;
    pstVoPubAttr->sync_info.ihs = 1;
    pstVoPubAttr->sync_info.ivs = 1;

    pstVoPubAttr->bg_color = 0x0;
    return SAL_SOK;
}

/**
 * @function   disp_ssv5SetMipiTx
 * @brief      设置mipi控制输出图像
 * @param[in]  DISP_DEV_COMMON *pDispDev     
 * @param[in]  const BT_TIMING_S *pstTiming  mipi控制参数
 * @param[out] None
 * @return     static INT32 SAL_SOK  : 成功
 *                          SAL_FAIL : 失败
 */
static INT32 disp_ssv5SetMipiTx(DISP_DEV_COMMON *pDispDev, const BT_TIMING_S *pstTiming)
{
    UINT32 uiVoLayer = 0;
    INT32 s32Ret = TD_SUCCESS;
    ot_vo_csc stVideoCSC = {0};
    

    if ((pDispDev == NULL) || (NULL == pstTiming))
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    uiVoLayer = pDispDev->szLayer.uiLayerNo;

    Mpi_Tx_Start(pstTiming);

    s32Ret = ss_mpi_vo_get_layer_csc(uiVoLayer, &stVideoCSC);
    if (TD_SUCCESS != s32Ret)
    {
        DISP_LOGE("ss_mpi_vo_get_layer_csc failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }
    DISP_LOGI(" uiVoLayer = %d  csc_matrix= 0x%x \n",uiVoLayer, stVideoCSC.csc_matrix);
#if 0 
    stVideoCSC.csc_matrix = OT_VO_CSC_MATRIX_BT709FULL_TO_RGBFULL;  /*是否支持，OT_VO_CSC_MATRIX_BT709FULL_TO_RGBFULL*/
    if(0 == uiVoLayer)
    {
        s32Ret = ss_mpi_vo_set_layer_csc(uiVoLayer, &stVideoCSC);
        if (TD_SUCCESS != s32Ret)
        {
         DISP_LOGE("ss_mpi_vo_set_layer_csc failed!s32Ret 0x%x \n", s32Ret);
         return SAL_FAIL;
        }
    }
#endif

#if 0
    GRAPHIC_LAYER GraphicLayer = 1;
    ot_vo_csc stCSC = {0};

    s32Ret = ss_mpi_vo_get_layer_csc(GraphicLayer, &stCSC);
    if (TD_SUCCESS != s32Ret)
    {
        DISP_LOGE("ss_mpi_vo_get_layer_csc failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    stCSC.csc_matrix = VO_CSC_MATRIX_IDENTITY;
    s32Ret = ss_mpi_vo_set_layer_csc(GraphicLayer, &stCSC);
    if (TD_SUCCESS != s32Ret)
    {
        DISP_LOGE("OT_MPI_VO_SetVideoLayerCSC failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

#endif
    //Mpi_Tx_SetMipi();   /*新平台不需要设置，mipistart时已经设置mipi配置*/

    return SAL_SOK;
}

/**
 * @function   disp_ssv5SetHDMI
 * @brief      设置hdmi输出
 * @param[in]  DISP_DEV_COMMON *pDispDev     
 * @param[in]  const BT_TIMING_S *pstTiming  hdmi控制器参数
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_ssv5SetHDMI(DISP_DEV_COMMON *pDispDev, const BT_TIMING_S *pstTiming)
{
    UINT32 uiVoLayer = 0;
    INT32 s32Ret = TD_SUCCESS;

    if ((pDispDev == NULL) || (NULL == pstTiming))
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    uiVoLayer = pDispDev->szLayer.uiLayerNo;

    s32Ret = Hdmi_Hal_SetHdmi(uiVoLayer, pstTiming);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("error 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    disp_ssv5SetVoInterface
 * @brief 设置VO设备接口
 * @param[in] pDispDev 设备参数
 * @param[out]
 * @return
 */
static INT32 disp_ssv5SetVoInterface(DISP_DEV_COMMON *pDispDev)
{
    INT32 s32Ret = TD_SUCCESS;
    const DISP_TIMING_MAP_S *pstDispTiming = NULL;
    const BT_TIMING_S *pstBtTiming = NULL;

    if (pDispDev == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    pstDispTiming = disp_ssv5GetTiming(pDispDev);
    if (NULL == pstDispTiming)
    {
        DISP_LOGE("disp_ssv5CreateDev failed!!!\n");
        return SAL_FAIL;
    }

    /* 获取相应时序 */
    pstBtTiming = &(BT_GetTiming(pstDispTiming->enUserTiming)->stTiming);
    if (VO_DEV_MIPI == pDispDev->eVoDev)
    {
        DISP_LOGI("VoDev %d eVoDev %s fps %d \n", pDispDev->uiDevNo, "MIPI_TX", pDispDev->uiDevFps);
        s32Ret = disp_ssv5SetMipiTx(pDispDev, pstBtTiming);
    }
    else if (VO_DEV_HDMI == pDispDev->eVoDev)
    {
        DISP_LOGI("VoDev %d eVoDev %s fps %d \n", pDispDev->uiDevNo, "HDMI", pDispDev->uiDevFps);
        s32Ret = disp_ssv5SetHDMI(pDispDev, pstBtTiming);
    }
    else
    {
        DISP_LOGI("VoDev %d eVoDev %s fps %d \n", pDispDev->uiDevNo, "VO_INTF_BT1120", pDispDev->uiDevFps);
    }

    return s32Ret;
}


/**
 * @function   disp_ssv5PutFrameInfo
 * @brief      将数据帧赋值给显示帧
 * @param[in]  SAL_VideoFrameBuf *videoFrame   SAL帧信息
 * @param[in]  VOID *pFrame                    hisi帧信息
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_ssv5PutFrameInfo(SAL_VideoFrameBuf *videoFrame, VOID *pFrame)
{
    ot_video_frame_info *pstHiFrame = NULL;
    UINT32 uWidth = 0;
    UINT32 uHeight = 0;
    VOID *pStreamBuff = NULL;

    pstHiFrame = (ot_video_frame_info *)pFrame;
    if (pstHiFrame == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    uWidth = videoFrame->frameParam.width;
    uHeight = videoFrame->frameParam.height;
    pStreamBuff = OT_ADDR_U642P(videoFrame->virAddr[0]);

    memcpy(OT_ADDR_U642P(pstHiFrame->video_frame.virt_addr[0]), pStreamBuff, uWidth * uHeight);
    memcpy(OT_ADDR_U642P(pstHiFrame->video_frame.virt_addr[1]), pStreamBuff + uWidth * uHeight, uWidth * uHeight / 2);

    pstHiFrame->video_frame.width = uWidth;
    pstHiFrame->video_frame.height = uHeight;
    pstHiFrame->video_frame.stride[0] = uWidth;

    return SAL_SOK;
}

/**
 * @function   disp_ssv5SendFrame
 * @brief      将数据送至vo通道
 * @param[in]  VOID *prm          
 * @param[in]  VOID *pFrame       帧信息
 * @param[in]  INT32 s32MilliSec  超时时间
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_ssv5SendFrame(VOID *prm, VOID *pFrame, INT32 s32MilliSec)
{
    INT32 s32Ret = TD_SUCCESS;
    UINT32 VoLayer = 0;
    UINT32 VoChn = 0;
    DISP_CHN_COMMON *pDispChn = NULL;
    ot_video_frame_info *pstVideoFrame = NULL;

    pDispChn = (DISP_CHN_COMMON *)prm;
    pstVideoFrame = (ot_video_frame_info *)pFrame;

    if (pstVideoFrame == NULL || pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    VoLayer = pDispChn->uiLayerNo;
    VoChn = pDispChn->uiChn;

    s32Ret = ss_mpi_vo_send_frame(VoLayer, VoChn, pstVideoFrame, s32MilliSec);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("OT_MPI_VO_SendFrame VoLayer %d VoChn %d failed with %#x!\n", VoLayer, VoChn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_ssv5GetFrame
 * @brief      获取视频输出通道帧
 * @param[in]  ot_vo_layer VoLayer             视频层号
 * @param[in]  ot_vo_chn VoChn                 通道号
 * @param[in]  SYSTEM_FRAME_INFO *pstSysFrame  帧信息
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_ssv5GetFrame(ot_vo_layer VoLayer, ot_vo_chn VoChn, SYSTEM_FRAME_INFO *pstSysFrame)
{
    INT32 ret = SAL_SOK;
    ot_video_frame_info *pstFrame = NULL;

    pstFrame = (ot_video_frame_info *)pstSysFrame->uiAppData;

    if (NULL == pstFrame)
    {
        DISP_LOGE("VoLayer %d VoChn %d failed param is null\n", VoLayer, VoChn);
        return SAL_FAIL;
    }

    ret = ss_mpi_vo_get_chn_frame(VoLayer, VoChn, pstFrame, 50);
    if (ret != TD_SUCCESS)
    {
        DISP_LOGE("ss_mpi_vo_get_chn_frame VoLayer %d VoChn %d failed with %#x!\n", VoLayer, VoChn, ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_ssv5RelseseFrame
 * @brief      释放视频输出通道帧
 * @param[in]  ot_vo_layer VoLayer                视频层
 * @param[in]  ot_vo_chn VoChn                    通道号
 * @param[in]  SYSTEM_FRAME_INFO *pstSysFrame  帧信息
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_ssv5RelseseFrame(ot_vo_layer VoLayer, ot_vo_chn VoChn, SYSTEM_FRAME_INFO *pstSysFrame)
{
    INT32 ret = SAL_SOK;
    ot_video_frame_info *pstFrame = NULL;

    pstFrame = (ot_video_frame_info *)pstSysFrame->uiAppData;

    if (NULL == pstFrame)
    {
        DISP_LOGE("VoLayer %d VoChn %d failed param is null\n", VoLayer, VoChn);
        return SAL_FAIL;
    }

    ret = ss_mpi_vo_release_chn_frame(VoLayer, VoChn, pstFrame);
    if (ret != TD_SUCCESS)
    {
        DISP_LOGE("ss_mpi_vo_get_chn_frame VoLayer %d VoChn %d failed with %#x!\n", VoLayer, VoChn, ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_ssv5GetVoChnFrame
 * @brief      获取vo输出帧数据
 * @param[in]  UINT32 VoLayer               视频层
 * @param[in]  UINT32 VoChn                 通道号
 * @param[in]  SYSTEM_FRAME_INFO *pstFrame  数据帧
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_ssv5GetVoChnFrame(UINT32 VoLayer, UINT32 VoChn, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 ret = SAL_SOK;
    ot_video_frame_info stVideoFrame;
    SYSTEM_FRAME_INFO stSrcSysFrame;
    ot_video_frame_info *pstHiFrame = NULL;
    UINT32 uWidth = 0;
    UINT32 uHeight = 0;
    VOID *pStreamBuff = NULL;

    if (NULL == pstFrame)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    stSrcSysFrame.uiAppData = (PhysAddr) & stVideoFrame;
    ret = disp_ssv5GetFrame(VoLayer, VoChn, &stSrcSysFrame);
    if (ret != TD_SUCCESS)
    {
        DISP_LOGE("Disp_hisiGetFrame failed with %#x!\n", ret);
        return SAL_FAIL;
    }

    /*暂时没有使用这个接口,先用memcpy拷贝*/
    /*HAL_bufQuickCopy_v0(&stSrcSysFrame, pstFrame, 0, 0, stVideoFrame.video_frame.u32Width, stVideoFrame.video_frame.u32Height);*/
    pstHiFrame = (ot_video_frame_info *)pstFrame->uiAppData;
    uWidth = stVideoFrame.video_frame.width;
    uHeight = stVideoFrame.video_frame.height;
    pStreamBuff = OT_ADDR_U642P(stVideoFrame.video_frame.virt_addr);
    memcpy(OT_ADDR_U642P(pstHiFrame->video_frame.virt_addr[0]), pStreamBuff, uWidth * uHeight);
    memcpy(OT_ADDR_U642P(pstHiFrame->video_frame.virt_addr[1]), pStreamBuff + uWidth * uHeight, uWidth * uHeight / 2);
    pstHiFrame->video_frame.width = uWidth;
    pstHiFrame->video_frame.height = uHeight;
    pstHiFrame->video_frame.stride[0] = uWidth;


    ret = disp_ssv5RelseseFrame(VoLayer, VoChn, &stSrcSysFrame);
    if (ret != TD_SUCCESS)
    {
        DISP_LOGE("Disp_hisiGetFrame failed with %#x!\n", ret);
        return SAL_FAIL;
    }

    DISP_LOGW("disp_ssv5GetVoChnFrame Unsupported %d!\n", ret);

    return ret;
}

/**
 * @function   disp_ssv5DeinitStartingup
 * @brief      开机时显示反初始化
 * @param[in]  UINT32 uiDev  
 * @param[out] None
 * @return     static INT32
 */
static INT32 disp_ssv5DeinitStartingup(UINT32 uiDev)
{
    INT32 s32Ret = TD_SUCCESS;
    ot_vo_pub_attr stVoPubAttr;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;
    uiLayer = uiDev;
    uiChn = 0;
  
    memset(&stVoPubAttr, 0, sizeof(ot_vo_pub_attr));
    s32Ret = ss_mpi_vo_get_pub_attr(uiDev, &stVoPubAttr);
    if (TD_SUCCESS != s32Ret )
    {
        DISP_LOGE("ss_mpi_vo_get_pub_attr VoDev %d failed with %#x!\n", uiDev, s32Ret);
    }

    stVoPubAttr.intf_sync = OT_VO_OUT_1080P60;
    /*ss928sdk需要在去掉开机画面使能前设置vo_pub_attrs属性，原因是新的sdk无法获取开机uboot下设置的是什么输出类型，这里写死0通道hdmi后续根据产品配置*/
    if(0 == uiDev)
    {
        stVoPubAttr.intf_type = OT_VO_INTF_HDMI;
    }
    else
    {
        stVoPubAttr.intf_type = OT_VO_INTF_MIPI;
    }

    s32Ret = ss_mpi_vo_set_pub_attr(uiDev, &stVoPubAttr);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("ss_mpi_vo_set_pub_attr VoDev %d failed with %#x!\n", uiDev, s32Ret);
    }

    s32Ret = ss_mpi_vo_disable_chn(uiLayer, uiChn);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("Stop %d Layer chn %d Failed s32Ret 0x%x!!!\n", uiLayer, uiChn, s32Ret);
    }

    s32Ret = ss_mpi_vo_disable_video_layer(uiLayer);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("failed with %#x!\n", s32Ret);
    }

    s32Ret = ss_mpi_vo_disable(uiDev);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("failed with %#x!\n", s32Ret);
    }

    return s32Ret;
}

/**
 * @function   disp_ssv5ClearVoBuf
 * @brief      清空指定输出通道的缓存 buffer 数据
 * @param[in]  UINT32 uiVoLayer  视频层号
 * @param[in]  UINT32 uiVoChn    通道号
 * @param[in]  UINT32 bFlag      选择模式
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 disp_ssv5ClearVoBuf(UINT32 uiVoLayer, UINT32 uiVoChn, UINT32 bFlag)
{
    UINT32 s32Ret = SAL_FAIL;

    s32Ret = ss_mpi_vo_clear_chn_buf(uiVoLayer, uiVoChn, bFlag);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("error %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_ssv5DisableChn
 * @brief      禁止vo通道
 * @param[in]  VOID *prm  通道参数
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 disp_ssv5DisableChn(VOID *prm)
{
    UINT32 uiDev = 0;

    UINT32 uiLayer = 0;

    UINT32 uiChn = 0;

    INT32 s32Ret = TD_SUCCESS;

    DISP_CHN_COMMON *pDispChn = NULL;

    pDispChn = (DISP_CHN_COMMON *)prm;
    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    uiDev = pDispChn->uiDevNo;
    uiLayer = pDispChn->uiLayerNo;
    uiChn = pDispChn->uiChn;

    DISP_LOGI("Disable uiDev %d uiLayer %d uiChn %d \n", uiDev, uiLayer, uiChn);

    s32Ret = ss_mpi_vo_disable_chn(uiLayer, uiChn);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("Stop %d Layer chn %d Failed s32Ret 0x%x!!!\n", uiLayer, uiChn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_ssv5SetVideoLayerCsc
 * @brief      设置视频层图像输出效果
 * @param[in]  UINT32 uiVoLayer  视频层
 * @param[in]  void *prm         图像效果参数
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 disp_ssv5SetVideoLayerCsc(UINT32 uiVoLayer, void *prm)
{
    INT32 s32Ret = TD_SUCCESS;
    ot_vo_csc stVoCscSt;
    VIDEO_CSC_S *pstCsc = NULL;

    pstCsc = (VIDEO_CSC_S *)prm;

    if (pstCsc == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    memset(&stVoCscSt, 0, sizeof(ot_vo_csc));

    s32Ret = ss_mpi_vo_get_layer_csc(uiVoLayer, &stVoCscSt);
    if (TD_SUCCESS != s32Ret)
    {
        DISP_LOGE("get video layer[%u] csc fail\n", uiVoLayer);
        return SAL_FAIL;
    }

    stVoCscSt.luma = pstCsc->uiLuma;
    stVoCscSt.contrast = pstCsc->uiContrast;
    stVoCscSt.hue = pstCsc->uiHue;
    stVoCscSt.saturation = pstCsc->uiSatuature;

    s32Ret = ss_mpi_vo_set_layer_csc(uiVoLayer, &stVoCscSt);
    if (TD_SUCCESS != s32Ret)
    {
        DISP_LOGE("OT_MPI_VO_SetVideoLayerCSC Layer %d CSC Failed, %x !!!\n", uiVoLayer, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_ssv5EnableChn
 * @brief      使能vo通道
 * @param[in]  VOID *prm  vo通道信息
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 disp_ssv5EnableChn(VOID *prm)
{
    UINT32 uiDev = 0;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;

    ot_vo_chn_attr stChnAttr = {0}; 
    INT32 s32Ret = TD_SUCCESS;

    DISP_CHN_COMMON *pDispChn = NULL;

    pDispChn = (DISP_CHN_COMMON *)prm;
    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    uiDev = pDispChn->uiDevNo;
    uiLayer = pDispChn->uiLayerNo;
    uiChn = pDispChn->uiChn;

    memset(&stChnAttr, 0, sizeof(ot_vo_chn_attr));

    stChnAttr.rect.x = pDispChn->uiX;
    stChnAttr.rect.y = pDispChn->uiY;
    stChnAttr.rect.width = pDispChn->uiW;
    stChnAttr.rect.height = pDispChn->uiH;
    stChnAttr.priority = 0;
    stChnAttr.deflicker_en = TD_FALSE;

    DISP_LOGI("uiDev %d,VoLayer %d voChn %d x %4d y %4d w %4d h %4d !\n", uiDev, uiLayer, uiChn, stChnAttr.rect.x, stChnAttr.rect.y,
              stChnAttr.rect.width, stChnAttr.rect.height);

    s32Ret = ss_mpi_vo_set_chn_attr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("ss_mpi_vo_set_chn_attr s32Ret 0x%x uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiLayer, uiChn,
                  stChnAttr.rect.x, stChnAttr.rect.y, stChnAttr.rect.width, stChnAttr.rect.height);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vo_enable_chn(uiLayer, uiChn);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("ss_mpi_vo_enable_chn Layer %d  chn %d Failed s32Ret 0x%x !!!\n", uiLayer, uiChn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_ssv5SetChnPos
 * @brief      设置vo参数（放大镜和小窗口）
 * @param[in]  VOID *prm  通道参数
 * @param[out] None
 * @return     static INT32 SAL_SOK   成功
 *                          SAL_FAIL  失败
 */
static INT32 disp_ssv5SetChnPos(VOID *prm)
{
    UINT32 uiDev = 0;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;

    ot_vo_chn_attr stChnAttr;
    INT32 s32Ret = TD_SUCCESS;

    DISP_CHN_COMMON *pDispChn = NULL;

    pDispChn = (DISP_CHN_COMMON *)prm;
    if (pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    uiDev = pDispChn->uiDevNo;
    uiLayer = pDispChn->uiLayerNo;
    uiChn = pDispChn->uiChn;

    memset(&stChnAttr, 0, sizeof(ot_vo_chn_attr));

    s32Ret = ss_mpi_vo_get_chn_attr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("ss_mpi_vo_set_chn_attr s32Ret 0x%x uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiLayer, uiChn,
                  stChnAttr.rect.x, stChnAttr.rect.y, stChnAttr.rect.width, stChnAttr.rect.height);
        return SAL_FAIL;
    }

    stChnAttr.rect.x = SAL_align(pDispChn->uiX, 2);
    stChnAttr.rect.x = SAL_align(pDispChn->uiY, 2);
    stChnAttr.rect.width = pDispChn->uiW;
    stChnAttr.rect.height = pDispChn->uiH;
    stChnAttr.priority = 0;
    stChnAttr.deflicker_en = TD_FALSE;

    DISP_LOGD("uiDev %d,VoLayer %d voChn %d x %4d y %4d w %4d h %4d !\n", uiDev, uiLayer, uiChn, stChnAttr.rect.x, stChnAttr.rect.y,
              stChnAttr.rect.width, stChnAttr.rect.height);

    s32Ret = ss_mpi_vo_set_chn_attr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("ss_mpi_vo_set_chn_attr s32Ret 0x%x uiDev %d,uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiDev, uiLayer, uiChn,
                  stChnAttr.rect.x, stChnAttr.rect.y, stChnAttr.rect.width, stChnAttr.rect.height);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_ssv5DeleteDev
 * @brief      删除显示层
 * @param[in]  VOID *prm  
 * @param[out] None
 * @return     static INT32 SAL_SOK  成功
 *                          SAL_FAIL 失败
 */
static INT32 disp_ssv5DeleteDev(VOID *prm)
{
    UINT32 eVoDev = 0;

    ot_vo_dev VoDev = 0;
    ot_vo_layer VoLayer = 0;

    INT32 s32Ret = TD_SUCCESS;

    DISP_DEV_COMMON *pDispDev = NULL;

    pDispDev = (DISP_DEV_COMMON *)prm;
    if (pDispDev == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    eVoDev = pDispDev->eVoDev;
    VoDev = pDispDev->uiDevNo;
    VoLayer = pDispDev->szLayer.uiLayerNo;

    if (VO_DEV_HDMI == eVoDev)
    {
        Hdmi_Hal_Stop();
    }
    else if(VO_DEV_MIPI == eVoDev)
    {
        Mpi_Tx_Stop();
    }
    

    DISP_LOGI("VoDev = %d VoLayer = %d \n", VoDev, VoLayer);

    s32Ret = ss_mpi_vo_disable_video_layer(VoLayer);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("ss_mpi_vo_disable_video_layer Layer %d failed with 0x%x !\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vo_disable(VoDev);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("ss_mpi_vo_disable VO %d failed with 0x%x !\n", VoDev, s32Ret);
        return SAL_FAIL;
    }

    if (VO_DEV_HDMI == eVoDev)
    {
        s32Ret = Hdmi_Hal_DeInit();
        if (SAL_SOK != s32Ret)
        {
           DISP_LOGE("error !!\n");
           return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function   disp_ssv5CreateLayer
 * @brief      创建视频层
 * @param[in]  VOID *prm  视频层参数
 * @param[out] None
 * @return     INT32 SAL_SOK  成功
 *                   SAL_FAIL 失败
 */
INT32 disp_ssv5CreateLayer(VOID *prm)
{
    UINT32 uiLayerWith = 0;
    UINT32 uiLayerHeight = 0;
    UINT32 uiLayerFps = 0;
    DISP_LAYER_COMMON *pstLayer = NULL;

    ot_vo_layer VoLayer = 0;
    INT32 s32Ret = TD_SUCCESS;

    ot_vo_video_layer_attr stLayerAttr;

    memset(&stLayerAttr, 0, sizeof(ot_vo_video_layer_attr));

    pstLayer = (DISP_LAYER_COMMON *)prm;
    if (pstLayer == NULL)
    {
        DISP_LOGE("error");
        return SAL_FAIL;
    }

    VoLayer = pstLayer->uiLayerNo;
    uiLayerWith = pstLayer->uiLayerWith;
    uiLayerHeight = pstLayer->uiLayerHeight;
    uiLayerFps = pstLayer->uiLayerFps;

    /* 2. 配置layer 属性 */
    stLayerAttr.cluster_mode_en = TD_FALSE;
    stLayerAttr.double_frame_en = TD_FALSE;
    stLayerAttr.pixel_format = OT_PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    /* HDR模式下不支持SDR数据，SDR模式下支持HDR数据显示 */
    stLayerAttr.dst_dynamic_range = OT_DYNAMIC_RANGE_SDR8;

    stLayerAttr.display_rect.x = 0;
    stLayerAttr.display_rect.y = 0;
    stLayerAttr.display_rect.width = uiLayerWith;
    stLayerAttr.display_rect.height = uiLayerHeight;
    stLayerAttr.display_frame_rate = uiLayerFps;

    stLayerAttr.img_size.width = stLayerAttr.display_rect.width;
    stLayerAttr.img_size.height = stLayerAttr.display_rect.height;

    DISP_LOGI("Disp VoLayer %d u32Width %d u32Height %d uiDevFps %d !!!\n", VoLayer, uiLayerWith, uiLayerHeight, uiLayerFps);
    
    stLayerAttr.display_buf_len = 5;  /* 取值[0] [3,15] */

    s32Ret = ss_mpi_vo_set_video_layer_attr(VoLayer, &stLayerAttr);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("OT_MPI_VO_SetVideoLayerAttr VoLayer %d failed with %#x!\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vo_enable_video_layer(VoLayer);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("OT_MPI_VO_EnableVideoLayer Layer %d failed with %#x!\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   disp_halIsSupportIntfSync
 * @brief      对应的接口是否支持对应的时序
 * @param[in]  VO_DEV voDev               
 * @param[in]  VO_INTF_TYPE_E enIntfType  输出接口
 * @param[in]  ot_vo_intf_sync enIntfSync  想要输出的时序
 * @param[out] None
 * @return     static BOOL 支持返回TRUE，否则返回FALSE
 */
static BOOL disp_halIsSupportIntfSync(ot_vo_dev voDev, ot_vo_intf_type enIntfType, ot_vo_intf_sync enIntfSync)
{

    UINT32 i = 0;
    const ot_vo_intf_sync *penSupportIntfSync = NULL;

    if (voDev >= VO_DEV_MAX)
    {
        DISP_LOGE("invalid dev[%d]\n", voDev);
        return SAL_FAIL;
    }

    switch (enIntfType)
    {
        case OT_VO_INTF_HDMI:
            penSupportIntfSync = aenHdmiSupportRes[voDev];
            break;
        case OT_VO_INTF_MIPI:
            penSupportIntfSync = aenMipiSupportRes[voDev];
            break;
        default:
            DISP_LOGE("unsupport intf type[%d]\n", enIntfType);
            return SAL_FALSE;
    }

    for (i = 0; (i < OT_VO_OUT_BUTT) && (*penSupportIntfSync != OT_VO_OUT_BUTT); i++, penSupportIntfSync++)
    {
        if (*penSupportIntfSync == enIntfSync)
        {
            return SAL_TRUE;
        }
    }

    return SAL_FALSE;
}

/**
 * @function   disp_ssv5CreateDev
 * @brief      创建显示设备
 * @param[in]  DISP_DEV_COMMON *pDispDev  设备信息
 * @param[out] None
 * @return     static INT32 SAL_SOK  成功
 *                          SAL_FAIL 失败
 */
static INT32 disp_ssv5CreateDev(DISP_DEV_COMMON *pDispDev)
{
    UINT32 eVoDev = 0;
    UINT32 uiDevWith = 0;
    UINT32 uiDevHeight = 0;
    UINT32 uiDevFps = 0;
    UINT64 u64PixelClock = 0;
    ot_vo_dev VoDev = 0;
    INT32 s32Ret = TD_SUCCESS;
    ot_vo_pub_attr stVoPubAttr;
    ot_vo_user_sync_info stUserInfo = {0};
    ot_vo_intf_sync enIntfSync = 0;
    const DISP_TIMING_MAP_S *pstDispTiming = NULL;
    const BT_TIMING_S *pstBtTiming = NULL;
    UINT64 u64Foutvco = 0.0;

    /*第一个数表示ref_div(PLL 参考时钟分频系数, SD3403V100只能为 1 )， 
    第二个数表示post_div1(PLL 第一级输出分频系数,数值范围： (0,0x7])，
    第三个数表示post_div2(PLL 第二级输出分频系数,数值范围： (0,0x7])
    且post_div1>=post_div2*/
    static const UINT32 au32Mul24[][3] = {
        {1, 7, 7},
        {1, 7, 6},
        {1, 6, 6},
        {1, 6, 5},
        {1, 5, 5},
        {1, 5, 4},
        {1, 4, 4},
        {1, 4, 3},
        {1, 3, 3},
        {1, 3, 2},
        {1, 2, 2},
        {1, 2, 1},
        {1, 1, 1},
    };
    UINT32 u32Num = 0;
    UINT32 i = 0;

    if ((pDispDev == NULL))
    {
        DISP_LOGE("null pointer\n");
        return SAL_FAIL;
    }

    pstDispTiming = disp_ssv5GetTiming(pDispDev);
    if (NULL == pstDispTiming)
    {
        DISP_LOGE("disp_ssv5CreateDev failed!!!\n");
        return SAL_FAIL;
    }

    /* 获取相应时序 */
    pstBtTiming = &(BT_GetTiming(pstDispTiming->enUserTiming)->stTiming);
    enIntfSync = pstDispTiming->enVoTiming;
    VoDev = pDispDev->uiDevNo;
    eVoDev = pDispDev->eVoDev;
    uiDevWith = pDispDev->uiDevWith;
    uiDevHeight = pDispDev->uiDevHeight;
    uiDevFps = pDispDev->uiDevFps;

    memset(&stVoPubAttr, 0, sizeof(ot_vo_pub_attr));
    s32Ret = ss_mpi_vo_get_pub_attr(VoDev, &stVoPubAttr);
    if (TD_SUCCESS != s32Ret )
    {
        DISP_LOGE("ss_mpi_vo_get_pub_attr VoDev %d failed with %#x!\n", VoDev, s32Ret);
        return SAL_FAIL;
    }

    if (VO_DEV_HDMI == eVoDev)
    {
        stVoPubAttr.intf_type = OT_VO_INTF_HDMI;

        s32Ret = Hdmi_Hal_Init();
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }
    }
    else if (VO_DEV_MIPI == eVoDev)
    {
        stVoPubAttr.intf_type = OT_VO_INTF_MIPI;
    }
    else
    {
        stVoPubAttr.intf_type = OT_VO_INTF_BT1120;
        disp_ssv5DevBlanking(uiDevWith, uiDevHeight, &stVoPubAttr);
    }

    /* 先检查是否支持该分辨率时序 */
    if (SAL_TRUE != disp_halIsSupportIntfSync(VoDev, stVoPubAttr.intf_type, enIntfSync))
    {
        /* 检查是否支持用户模式 */
        enIntfSync = OT_VO_OUT_USER;
        if (SAL_TRUE != disp_halIsSupportIntfSync(VoDev, stVoPubAttr.intf_type, enIntfSync))
        {
            DISP_LOGE("disp dev[%d] mode[%u] not support user mode\n", VoDev, eVoDev);
            return SAL_FAIL;
        }
    }

    stVoPubAttr.bg_color = pDispDev->uiBgcolor;
    stVoPubAttr.intf_sync = enIntfSync;

    if (OT_VO_OUT_USER == stVoPubAttr.intf_sync)
    {
        stVoPubAttr.sync_info.syncm = 0;                   /* BT656 */
        stVoPubAttr.sync_info.iop = 1;                    /* 逐行 */
        stVoPubAttr.sync_info.intfb = 1;
        stVoPubAttr.sync_info.vact = pstBtTiming->u32Height;
        stVoPubAttr.sync_info.vbb = pstBtTiming->u32VBackPorch + pstBtTiming->u32VSync;
        stVoPubAttr.sync_info.vfb = pstBtTiming->u32VFrontPorch;
        stVoPubAttr.sync_info.hact = pstBtTiming->u32Width;
        stVoPubAttr.sync_info.hbb = pstBtTiming->u32HBackPorch + pstBtTiming->u32HSync;
        stVoPubAttr.sync_info.hfb = pstBtTiming->u32HFrontPorch;
        stVoPubAttr.sync_info.hmid = 1;
        stVoPubAttr.sync_info.bvact = 0;
        stVoPubAttr.sync_info.bvbb = 0;
        stVoPubAttr.sync_info.bvfb = 0;
        stVoPubAttr.sync_info.hpw = pstBtTiming->u32HSync;
        stVoPubAttr.sync_info.vpw = pstBtTiming->u32VSync;
        stVoPubAttr.sync_info.idv = 0;
        stVoPubAttr.sync_info.ihs = (pstBtTiming->enHSPol == POLARITY_NEG) ? TD_TRUE : TD_FALSE;
        stVoPubAttr.sync_info.ivs = (pstBtTiming->enVSPol == POLARITY_NEG) ? TD_TRUE : TD_FALSE;
        stUserInfo.clk_reverse_en = TD_FALSE;
        stUserInfo.dev_div = 1;                /*H9平台前置分频，预分频都固定为1*/
        stUserInfo.pre_div = 1;
        u64PixelClock = pstBtTiming->u32PixelClock;

        /* stUserInfo.u32PreDiv = 1; */
        /* hdmi需要分频，mipi不需要分频且不支持PLL时钟源只支持LCD和固定时钟源 */
        if (pDispDev->eVoDev == VO_DEV_MIPI)
        {
             /*大于75M使用固定频率时钟源，小于等于75M使用LCD时钟源，如下表示时钟源频率范围*/
             stUserInfo.user_sync_attr.clk_src = OT_VO_CLK_SRC_FIXED;
             if(134400000 < u64PixelClock)
             {
                 stUserInfo.user_sync_attr.fixed_clk = OT_VO_FIXED_CLK_148_5M ;
             }
             else if(134400000 >=  u64PixelClock && u64PixelClock > 108000000)
             {
                 stUserInfo.user_sync_attr.fixed_clk = OT_VO_FIXED_CLK_134_4M;
             }
             else if(108000000 >=  u64PixelClock && u64PixelClock > 75000000)
             {
                 stUserInfo.user_sync_attr.fixed_clk = OT_VO_FIXED_CLK_108M;
             }
             else
             {
                 stUserInfo.user_sync_attr.clk_src = OT_VO_CLK_SRC_LCDMCLK;
                 /*lcd_m_clk_div = (src_clk/1188) * 2^27  单位M*/
                 stUserInfo.user_sync_attr.lcd_m_clk_div = ((u64PixelClock/1188) * 0x01 << 27)/1000000;
                 if(0 >=  stUserInfo.user_sync_attr.lcd_m_clk_div || stUserInfo.user_sync_attr.lcd_m_clk_div > 8473341)
                 {
                     DISP_LOGE("invalid pixel clock:%u\n", pstBtTiming->u32PixelClock);
                     goto err;
                 }
             }

        }
        else if (pDispDev->eVoDev == VO_DEV_HDMI)
        {
            stUserInfo.user_sync_attr.clk_src = OT_VO_CLK_SRC_PLL;

            /* FOUTVCO=24*(u32Fbdiv+u32Frac/2^24)/u32Refdiv要在[800,3200]范围内 */
            /* 以下计算公式理论上能支持像素时钟在[33.33, 3200]之间的任意分辨率 */
            u32Num = sizeof(au32Mul24) / sizeof(au32Mul24[0]);
            /* 先找出一个小于等于3200的一个数 */
            while ((i < u32Num) && ((UINT64)pstBtTiming->u32PixelClock * au32Mul24[i][1] * au32Mul24[i][2] > (UINT64)3200 * 1000000))
            {
                i++;
            }

            if(i >= u32Num)
            {
                DISP_LOGE("invalid pixel clock:%u\n", pstBtTiming->u32PixelClock);
                goto err;
            }
            
            /* 下列结构体的数值根据公式凑数即可 */
            /* 像素时钟=24*(u32Fbdiv+u32Frac/2^24)/u32Refdiv/(u32Postdiv1*u32Postdiv2) */
            /* 将24/u32Refdiv/(u32Postdiv1*u32Postdiv2)=1,u32Fbdiv等于像素时钟的整数位(以MHz为单位)，u32Frac/2^24为小数 */
            u64Foutvco = u64PixelClock *au32Mul24[i][1] * au32Mul24[i][2];
            DISP_LOGI(" invalid pixel clock:%u u64Foutvco = %llu  i =%d\n", pstBtTiming->u32PixelClock, u64Foutvco,  i);

            /* 校验是不是小于等于3200 */
            if (u64Foutvco > (UINT64)3200 * 1000000)
            {
                DISP_LOGE("invalid pixel clock:%u\n", pstBtTiming->u32PixelClock);
                goto err;
            }

            /* 校验是不是大于等于800 */
            if (u64Foutvco < (UINT64)800 * 1000000)
            {
                DISP_LOGE("invalid pixel clock:%u\n", pstBtTiming->u32PixelClock);
                goto err;
            }
            stUserInfo.user_sync_attr.vo_pll.fb_div = (UINT32)((u64Foutvco / DISP_PPL_FREF) / 1000000);
            u64PixelClock = (UINT64)((u64Foutvco / DISP_PPL_FREF) - (stUserInfo.user_sync_attr.vo_pll.fb_div * 1000000));
            stUserInfo.user_sync_attr.vo_pll.frac = (UINT32)(u64PixelClock * (0x01 << DISP_PPL_FREF) / 1000000);
            stUserInfo.user_sync_attr.vo_pll.ref_div =   au32Mul24[i][0];
            stUserInfo.user_sync_attr.vo_pll.post_div1 = au32Mul24[i][1];
            stUserInfo.user_sync_attr.vo_pll.post_div2 = au32Mul24[i][2];
        }

    }

    s32Ret = ss_mpi_vo_set_pub_attr(VoDev, &stVoPubAttr);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("ss_mpi_vo_set_pub_attr VoDev %d failed with %#x!\n", VoDev, s32Ret);
        goto err;
    }

    if (OT_VO_OUT_USER == stVoPubAttr.intf_sync)
    {
       
        
        s32Ret = ss_mpi_vo_set_user_sync_info(VoDev, &stUserInfo);
        if (s32Ret != TD_SUCCESS)
        {
            DISP_LOGE("[VoDev:%d]Set user interface sync info failed with %x.\n", VoDev, s32Ret);
            goto err;
        }

        s32Ret = ss_mpi_vo_set_dev_frame_rate(VoDev, uiDevFps);
        if (TD_SUCCESS != s32Ret  )
        {
            DISP_LOGE("[VoDev:%d]Setss_mpi_vo_set_dev_frame_rate info failed with %x.\n", VoDev, s32Ret);
            goto err;
        }

        
    }

    s32Ret = ss_mpi_vo_enable(VoDev);
    if (s32Ret != TD_SUCCESS)
    {
        DISP_LOGE("OT_MPI_VO_Enable VoDev %d failed with %#x!\n", VoDev, s32Ret);
        goto err;
    }

    return SAL_SOK;
err:
    if (VO_DEV_HDMI == eVoDev)
    {
        s32Ret = Hdmi_Hal_DeInit();
        if (SAL_SOK != s32Ret)
        {
           DISP_LOGE("error !!\n");
           return SAL_FAIL;
        }
    }
    return SAL_FAIL;
}

/**
 * @function    disp_ssv5GetHdmiEdid
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 disp_ssv5GetHdmiEdid(UINT32 u32HdmiId, UINT8 *pu8Buff, UINT32 *pu32Len)
{
    INT32 s32Ret = TD_SUCCESS;
    ot_hdmi_edid stEdid;
    ot_hdmi_id enHdmi = u32HdmiId;

    if (NULL == pu8Buff || pu32Len == NULL || enHdmi >= OT_HDMI_ID_BUTT)
    {
        EDID_LOGE("hdmi[%d] get edid fail:0x%x\n", enHdmi, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_hdmi_force_get_edid(enHdmi, &stEdid);
    if (TD_SUCCESS != s32Ret)
    {
        EDID_LOGE("hdmi[%d] get edid fail:0x%x\n", enHdmi, s32Ret);
        return SAL_FAIL;
    }

    if ((TD_TRUE != stEdid.edid_valid) || (0 == stEdid.edid_len) || (stEdid.edid_len > DISP_EDID_TOTAL_MAX))
    {
        EDID_LOGE("hdmi[%d] edid is invalid[%d] or num[%u] error\n", enHdmi, stEdid.edid_valid, stEdid.edid_len);
        return SAL_FAIL;
    }

    sal_memcpy_s(pu8Buff, DISP_EDID_TOTAL_MAX, stEdid.edid, stEdid.edid_len);

    *pu32Len = stEdid.edid_len;

    return SAL_SOK;

}

/*******************************************************************************
* 函数名  : disp_halRegister
* 描  述  : 注册hisi disp显示函数
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
DISP_PLAT_OPS *disp_halRegister(void)
{

    memset(&g_stDispPlatOps, 0, sizeof(DISP_PLAT_OPS));

    g_stDispPlatOps.createVoDev = disp_ssv5CreateDev;
    g_stDispPlatOps.deleteVoDev = disp_ssv5DeleteDev;
    g_stDispPlatOps.setVoChnParam = disp_ssv5SetChnPos;
    g_stDispPlatOps.setVoLayerCsc = disp_ssv5SetVideoLayerCsc;
    g_stDispPlatOps.enableVoChn = disp_ssv5EnableChn;
    g_stDispPlatOps.clearVoChnBuf = disp_ssv5ClearVoBuf;
    g_stDispPlatOps.disableVoChn = disp_ssv5DisableChn;
    g_stDispPlatOps.deinitVoStartingup = disp_ssv5DeinitStartingup;
    g_stDispPlatOps.putVoFrameInfo = disp_ssv5PutFrameInfo;
    g_stDispPlatOps.sendVoChnFrame = disp_ssv5SendFrame;
    g_stDispPlatOps.getVoChnFrame = disp_ssv5GetVoChnFrame;
    g_stDispPlatOps.setVoInterface = disp_ssv5SetVoInterface;
    g_stDispPlatOps.getHdmiEdid = disp_ssv5GetHdmiEdid;
    g_stDispPlatOps.createVoLayer = disp_ssv5CreateLayer;

    return &g_stDispPlatOps;
}

