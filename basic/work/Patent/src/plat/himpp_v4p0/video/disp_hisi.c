/*******************************************************************************
* disp_hal_drv.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : huangshuxin <huangshuxin@hikvision.com>
* Version: V1.0.0  2019年2月1日 Create
*
* Description :
* Modification:
*******************************************************************************/



/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <platform_sdk.h>
#include <platform_hal.h>

#include "capbility.h"
#include "disp_hisi.h"
#include "../../hal/hal_inc_inter/disp_hal_inter.h"




/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define disp_HAL_GET_HZ_OFFSET(x) ((94 * (((x) >> 8) - 0xa0 - 1) + (((x) & 0xff) - 0xa0 - 1)) * 32)
#define DISP_EDID_TOTAL_MAX         (512)       // EDID最大值
#define disp_VGS_CHN_START 6



/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

static DISP_PLAT_OPS g_stDispPlatOps;
/* 自定义需要支持的时序，每新增一种分辨率时序都需要增加到该数组中 */
static const DISP_TIMING_MAP_S gastDispTimingMap[] = {
    {1920,  1080,   72,     VO_OUTPUT_USER,             TIMING_CVT_1920X1080P72_R2},
    {1920,  1080,   60,     VO_OUTPUT_1080P60,          TIMING_VESA_1920X1080P60},
    {1920,  1080,   50,     VO_OUTPUT_USER,             TIMING_CVT_1920X1080P50},

    {1680,  1050,   60,     VO_OUTPUT_USER,             TIMING_CVT_1680X1050P60},

    {1280,  1024,   85,     VO_OUTPUT_USER,             TIMING_VESA_1280X1024P85},
    {1280,  1024,   75,     VO_OUTPUT_USER,             TIMING_VESA_1280X1024P75},
    {1280,  1024,   72,     VO_OUTPUT_USER,             TIMING_GTF_1280X1024P72},
    {1280,  1024,   70,     VO_OUTPUT_USER,             TIMING_GTF_1280X1024P70},
    {1280,  1024,   60,     VO_OUTPUT_1280x1024_60,     TIMING_VESA_1280X1024P60},

    {1280,  800,    85,     VO_OUTPUT_USER,             TIMING_VESA_1280X800P85},
    {1280,  800,    75,     VO_OUTPUT_USER,             TIMING_CVT_1280X800P75},
    {1280,  800,    72,     VO_OUTPUT_USER,             TIMING_GTF_1280X800P72},
    {1280,  800,    60,     VO_OUTPUT_1280x800_60,      TIMING_CVT_1280X800P60},

    {1280,  768,    85,     VO_OUTPUT_USER,             TIMING_VESA_1280X768P85},
    {1280,  768,    75,     VO_OUTPUT_USER,             TIMING_VESA_1280X768P75},
    {1280,  768,    72,     VO_OUTPUT_USER,             TIMING_GTF_1280X768P72},
    {1280,  768,    60,     VO_OUTPUT_USER,             TIMING_CVT_1280X768P60},
        
    {1280,  720,    72,     VO_OUTPUT_USER,             TIMING_GTF_1280X720P72},
    {1280,  720,    60,     VO_OUTPUT_720P60,           TIMING_VESA_1280X720P60},
    {1280,  720,    50,     VO_OUTPUT_USER,             TIMING_CVT_1280X720P50},
    
    {1024,  768,    85,     VO_OUTPUT_USER,             TIMING_VESA_1024X768P85},
    {1024,  768,    75,     VO_OUTPUT_USER,             TIMING_VESA_1024X768P75},
    {1024,  768,    70,     VO_OUTPUT_USER,             TIMING_VESA_1024X768P70},
    {1024,  768,    72,     VO_OUTPUT_USER,             TIMING_CVT_1024X768P72},
    {1024,  768,    60,     VO_OUTPUT_1024x768_60,      TIMING_VESA_1024X768P60},
};

/* HDMI支持的分辨率时序，参考数据手册 */
static const VO_INTF_SYNC_E aenHdmiSupportRes[VO_DEV_MAX][VO_OUTPUT_BUTT] = {
    /* DHD0 */
    {
        VO_OUTPUT_1080P24, VO_OUTPUT_1080P25, VO_OUTPUT_1080P30, VO_OUTPUT_720P50,
        VO_OUTPUT_720P60, VO_OUTPUT_1080P50, VO_OUTPUT_1080P60, VO_OUTPUT_576P50,
        VO_OUTPUT_480P60, VO_OUTPUT_800x600_60, VO_OUTPUT_1024x768_60, VO_OUTPUT_1280x1024_60,
        VO_OUTPUT_1366x768_60, VO_OUTPUT_1440x900_60, VO_OUTPUT_1280x800_60, VO_OUTPUT_1600x1200_60,
        VO_OUTPUT_1680x1050_60, VO_OUTPUT_1920x1200_60, VO_OUTPUT_640x480_60, VO_OUTPUT_1920x2160_30,
        VO_OUTPUT_2560x1440_30, VO_OUTPUT_2560x1440_60, VO_OUTPUT_2560x1600_60, VO_OUTPUT_3840x2160_24,
        VO_OUTPUT_3840x2160_25, VO_OUTPUT_3840x2160_30, VO_OUTPUT_3840x2160_50, VO_OUTPUT_3840x2160_60,
        VO_OUTPUT_4096x2160_24, VO_OUTPUT_4096x2160_25, VO_OUTPUT_4096x2160_30, VO_OUTPUT_4096x2160_50,
        VO_OUTPUT_4096x2160_60, VO_OUTPUT_USER, VO_OUTPUT_BUTT,
    },

    /* DHD1 */
    {
        VO_OUTPUT_BUTT,
    },
};

/* MIPI支持的分辨率时序，参考数据手册 */
static const VO_INTF_SYNC_E aenMipiSupportRes[VO_DEV_MAX][VO_OUTPUT_BUTT] = {
    /* DHD0 */
    {
        VO_OUTPUT_720P50, VO_OUTPUT_720P60, VO_OUTPUT_1080P60, VO_OUTPUT_576P50, 
        VO_OUTPUT_1024x768_60, VO_OUTPUT_1280x1024_60, VO_OUTPUT_3840x2160_30,
        VO_OUTPUT_3840x2160_60, VO_OUTPUT_720x1280_60, VO_OUTPUT_1080x1920_60,
        VO_OUTPUT_USER, VO_OUTPUT_BUTT,
    },

    /* DHD1 */
    {   
        VO_OUTPUT_720P50, VO_OUTPUT_720P60, VO_OUTPUT_1080P60, VO_OUTPUT_576P50, 
        VO_OUTPUT_1024x768_60, VO_OUTPUT_1280x1024_60, VO_OUTPUT_720x1280_60, 
        VO_OUTPUT_1080x1920_60, VO_OUTPUT_USER, VO_OUTPUT_BUTT,
    },
}; 


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

extern void Mpi_Tx_Start(const BT_TIMING_S *pstTiming);
extern int Mpi_Tx_SetMipi(void);
extern INT32 Hdmi_Hal_Stop(VOID);
extern INT32 Hdmi_Hal_SetHdmi(UINT32 uiVoLayer, const BT_TIMING_S *pstTiming);

static const DISP_TIMING_MAP_S  *disp_hisiGetTiming(DISP_DEV_COMMON *pDispDev)
{
    INT32 i = 0;
    const DISP_TIMING_MAP_S *pstDispTiming = NULL;
    UINT32 u32ResNum = sizeof(gastDispTimingMap)/sizeof(gastDispTimingMap[0]);

    for (i = 0; i < u32ResNum; i++)
    {
        pstDispTiming = gastDispTimingMap + i;
        if ((pstDispTiming->u32Width == pDispDev->uiDevWith) && 
            (pstDispTiming->u32Height == pDispDev->uiDevHeight) && 
            (pstDispTiming->u32Fps == pDispDev->uiDevFps))
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


/*******************************************************************************
* 函数名  : disp_hisiDevBlanking
* 描  述  : 消隐区设置
* 输  入  : - uiDevWith   :
*         : - uiDevHeight :
*         : - pstVoPubAttr:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiDevBlanking(UINT32 uiDevWith, UINT32 uiDevHeight, VO_PUB_ATTR_S *pstVoPubAttr)
{
    HI_U32 u32Addr = 0x12010044;
    HI_U32 u32Value = 0;

    if (NULL == pstVoPubAttr)
    {
        DISP_LOGE("!!!\n");

        return SAL_FAIL;
    }

    /* 1. 配置vo 属性 */
    /* pstVoPubAttr->enIntfSync = VO_OUTPUT_USER; */

    pstVoPubAttr->stSyncInfo.bSynm = 1;
    pstVoPubAttr->stSyncInfo.bIop = 1;
    pstVoPubAttr->stSyncInfo.u8Intfb = 1;


    /* 宽和高为自定义值，写成魔鬼数字 */
    if ((2048 == uiDevWith) && (600 == uiDevHeight))
    {
        /* 1024*600 修改 vo 输出频率到 107 M*/
/*        HI_MPI_SYS_GetReg(u32Addr, &u32Value); */

        DISP_LOGI("u32Addr : %x u32Value : %x \n", u32Addr, u32Value);
/*        HI_MPI_SYS_SetReg(u32Addr, 0xc000); */

        pstVoPubAttr->stSyncInfo.u16Vact = 600;
        pstVoPubAttr->stSyncInfo.u16Vbb = 23;
        pstVoPubAttr->stSyncInfo.u16Vfb = 12;
        pstVoPubAttr->stSyncInfo.u16Hact = 2048;
        pstVoPubAttr->stSyncInfo.u16Hbb = 440;
        pstVoPubAttr->stSyncInfo.u16Hfb = 320;
        pstVoPubAttr->stSyncInfo.u16Hmid = 1;

        DISP_LOGI("Set Sync Info For 1024*600 \n");
    }
    else if ((2560 == uiDevWith)
             && (800 == uiDevHeight))
    {
        pstVoPubAttr->stSyncInfo.u16Vact = 800;
        pstVoPubAttr->stSyncInfo.u16Vbb = 22;
        pstVoPubAttr->stSyncInfo.u16Vfb = 1;

        pstVoPubAttr->stSyncInfo.u16Hact = 2560;
        pstVoPubAttr->stSyncInfo.u16Hbb = 233;
        pstVoPubAttr->stSyncInfo.u16Hfb = 214;
        pstVoPubAttr->stSyncInfo.u16Hmid = 1;

        DISP_LOGI("Set Sync Info For 1280*800 \n");
    }
    else if ((1280 == uiDevWith) && (720 == uiDevHeight))
    {
        /* 1024*600 修改 vo 输出频率到 107 M*/
/*        HI_MPI_SYS_GetReg(u32Addr, &u32Value); */

        DISP_LOGI("u32Addr : %x u32Value : %x \n", u32Addr, u32Value);
/*        HI_MPI_SYS_SetReg(u32Addr, 0xc000); */

        pstVoPubAttr->stSyncInfo.u16Vact = 720;
        pstVoPubAttr->stSyncInfo.u16Vbb = 23;
        pstVoPubAttr->stSyncInfo.u16Vfb = 12;
        pstVoPubAttr->stSyncInfo.u16Hact = 1280;
        pstVoPubAttr->stSyncInfo.u16Hbb = 440;
        pstVoPubAttr->stSyncInfo.u16Hfb = 320;
        pstVoPubAttr->stSyncInfo.u16Hmid = 1;

        DISP_LOGI("Set Sync Info For 1280*720 \n");
    }
    else
    {
        DISP_LOGE("!!!\n");

        /* 1024*600 修改 vo 输出频率到 107 M*/
/*        HI_MPI_SYS_GetReg(u32Addr, &u32Value); */

        DISP_LOGI("u32Addr : %x u32Value : %x \n", u32Addr, u32Value);
/*        HI_MPI_SYS_SetReg(u32Addr, 0xc000); */

        pstVoPubAttr->stSyncInfo.u16Vact = 600;
        pstVoPubAttr->stSyncInfo.u16Vbb = 23;
        pstVoPubAttr->stSyncInfo.u16Vfb = 12;
        pstVoPubAttr->stSyncInfo.u16Hact = 2048;
        pstVoPubAttr->stSyncInfo.u16Hbb = 440;
        pstVoPubAttr->stSyncInfo.u16Hfb = 320;
        pstVoPubAttr->stSyncInfo.u16Hmid = 1;
        return SAL_FAIL;
    }

    pstVoPubAttr->stSyncInfo.u16Bvact = 1;
    pstVoPubAttr->stSyncInfo.u16Bvbb = 1;
    pstVoPubAttr->stSyncInfo.u16Bvfb = 1;

    pstVoPubAttr->stSyncInfo.u16Hpw = 20;
    pstVoPubAttr->stSyncInfo.u16Vpw = 3;

    pstVoPubAttr->stSyncInfo.bIdv = 0;
    pstVoPubAttr->stSyncInfo.bIhs = 1;
    pstVoPubAttr->stSyncInfo.bIvs = 1;

    pstVoPubAttr->u32BgColor = 0x0;
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_halSetMipiTx
* 描  述  : 设置mipi输出图像
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_halSetMipiTx(DISP_DEV_COMMON *pDispDev, const BT_TIMING_S *pstTiming)
{
    UINT32 uiVoLayer = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    VO_CSC_S stVideoCSC = {0};

    if ((pDispDev == NULL) || (NULL == pstTiming))
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    uiVoLayer = pDispDev->szLayer.uiLayerNo;

    Mpi_Tx_Start(pstTiming);

    s32Ret = HI_MPI_VO_GetVideoLayerCSC(uiVoLayer, &stVideoCSC);
    if (HI_SUCCESS != s32Ret)
    {
        DISP_LOGE("HI_MPI_VO_GetVideoLayerCSC failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    stVideoCSC.enCscMatrix = VO_CSC_MATRIX_BT709_TO_RGB_PC;
    s32Ret = HI_MPI_VO_SetVideoLayerCSC(uiVoLayer, &stVideoCSC);
    if (HI_SUCCESS != s32Ret)
    {
        DISP_LOGE("HI_MPI_VO_SetVideoLayerCSC failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

#if 0
    GRAPHIC_LAYER GraphicLayer = 1;
    VO_CSC_S stCSC = {0};

    s32Ret = HI_MPI_VO_GetGraphicLayerCSC(GraphicLayer, &stCSC);
    if (HI_SUCCESS != s32Ret)
    {
        DISP_LOGE("HI_MPI_VO_GetVideoLayerCSC failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    stCSC.enCscMatrix = VO_CSC_MATRIX_IDENTITY;
    s32Ret = HI_MPI_VO_SetGraphicLayerCSC(GraphicLayer, &stCSC);
    if (HI_SUCCESS != s32Ret)
    {
        DISP_LOGE("HI_MPI_VO_SetVideoLayerCSC failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

#endif
    Mpi_Tx_SetMipi();

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_halSetHDMI
* 描  述  : 设置hdmi输出
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_halSetHDMI(DISP_DEV_COMMON *pDispDev, const BT_TIMING_S *pstTiming)
{
    UINT32 uiVoLayer = 0;
    INT32 s32Ret = HI_SUCCESS;

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

static INT32 disp_hisiSetVoInterface(DISP_DEV_COMMON *pDispDev)
{
    INT32 s32Ret = HI_SUCCESS;
    const DISP_TIMING_MAP_S *pstDispTiming = NULL;
    const BT_TIMING_S *pstBtTiming = NULL;

    if (pDispDev == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    pstDispTiming = disp_hisiGetTiming(pDispDev);
    if(NULL == pstDispTiming)
    {
        DISP_LOGE("disp_hisiCreateDev failed!!!\n");
        return SAL_FAIL;
    }

    /* 获取相应时序 */
    pstBtTiming = &(BT_GetTiming(pstDispTiming->enUserTiming)->stTiming);
    if (VO_DEV_MIPI == pDispDev->eVoDev)
    {
        DISP_LOGI("VoDev %d eVoDev %s fps %d \n", pDispDev->uiDevNo, "MIPI_TX", pDispDev->uiDevFps);
        s32Ret = disp_halSetMipiTx(pDispDev, pstBtTiming);
    }
    else if (VO_DEV_HDMI == pDispDev->eVoDev)
    {
        DISP_LOGI("VoDev %d eVoDev %s fps %d \n", pDispDev->uiDevNo, "HDMI", pDispDev->uiDevFps);
        s32Ret = disp_halSetHDMI(pDispDev, pstBtTiming);
    }
    else
    {
        DISP_LOGI("VoDev %d eVoDev %s fps %d \n", pDispDev->uiDevNo, "VO_INTF_BT1120", pDispDev->uiDevFps);
    }

    return s32Ret;
}


/*******************************************************************************
* 函数名  : disp_hisiSendFrame
* 描  述  : 将数据帧赋值给显示帧
* 输  入  : - prm        :
*         : - pFrame     :
*         : - s32MilliSec:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiPutFrameInfo(SAL_VideoFrameBuf *videoFrame, VOID *pFrame)
{
    VIDEO_FRAME_INFO_S *pstHiFrame = NULL;
    UINT32 uWidth = 0;
    UINT32 uHeight = 0;
    VOID *pStreamBuff = NULL;

    pstHiFrame = (VIDEO_FRAME_INFO_S *)pFrame;
    if (pstHiFrame == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    uWidth = videoFrame->frameParam.width;
    uHeight = videoFrame->frameParam.height;
    pStreamBuff = HI_ADDR_U642P(videoFrame->virAddr[0]);

    memcpy(HI_ADDR_U642P(pstHiFrame->stVFrame.u64VirAddr[0]), pStreamBuff, uWidth * uHeight);
    memcpy(HI_ADDR_U642P(pstHiFrame->stVFrame.u64VirAddr[1]), pStreamBuff + uWidth * uHeight, uWidth * uHeight / 2);

    pstHiFrame->stVFrame.u32Width = uWidth;
    pstHiFrame->stVFrame.u32Height = uHeight;
    pstHiFrame->stVFrame.u32Stride[0] = uWidth;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_hisiSendFrame
* 描  述  : 将数据送至vo通道
* 输  入  : - prm        :
*         : - pFrame     :
*         : - s32MilliSec:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiSendFrame(VOID *prm, VOID *pFrame, INT32 s32MilliSec)
{
    HI_S32 s32Ret = HI_SUCCESS;
    UINT32 VoLayer = 0;
    UINT32 VoChn = 0;
    DISP_CHN_COMMON *pDispChn = NULL;
    VIDEO_FRAME_INFO_S *pstVideoFrame = NULL;

    pDispChn = (DISP_CHN_COMMON *)prm;
    pstVideoFrame = (VIDEO_FRAME_INFO_S *)pFrame;

    if (pstVideoFrame == NULL || pDispChn == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    VoLayer = pDispChn->uiLayerNo;
    VoChn = pDispChn->uiChn;

    s32Ret = HI_MPI_VO_SendFrame(VoLayer, VoChn, pstVideoFrame, s32MilliSec);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_SendFrame VoLayer %d VoChn %d failed with %#x!\n", VoLayer, VoChn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_hisiGetFrame
* 描  述  : 获取视频输出通道帧
* 输  入  : - pDispChn      :
*         : - pstSystemFrame:
*         : - uiRotate      :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiGetFrame(VO_LAYER VoLayer, VO_CHN VoChn, SYSTEM_FRAME_INFO *pstSysFrame)
{
    INT32 ret = SAL_SOK;
    VIDEO_FRAME_INFO_S *pstFrame = NULL;

    pstFrame = (VIDEO_FRAME_INFO_S *)pstSysFrame->uiAppData;

    if (NULL == pstFrame)
    {
        DISP_LOGE("VoLayer %d VoChn %d failed param is null\n", VoLayer, VoChn);
        return SAL_FAIL;
    }

    ret = HI_MPI_VO_GetChnFrame(VoLayer, VoChn, pstFrame, 50);
    if (ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_GetChnFrame VoLayer %d VoChn %d failed with %#x!\n", VoLayer, VoChn, ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_hisiRelseseFrame
* 描  述  : 释放视频输出通道帧
* 输  入  : - pDispChn      :
*         : - pstSystemFrame:
*         : - uiRotate      :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiRelseseFrame(VO_LAYER VoLayer, VO_CHN VoChn, SYSTEM_FRAME_INFO *pstSysFrame)
{
    INT32 ret = SAL_SOK;
    VIDEO_FRAME_INFO_S *pstFrame = NULL;

    pstFrame = (VIDEO_FRAME_INFO_S *)pstSysFrame->uiAppData;

    if (NULL == pstFrame)
    {
        DISP_LOGE("VoLayer %d VoChn %d failed param is null\n", VoLayer, VoChn);
        return SAL_FAIL;
    }

    ret = HI_MPI_VO_ReleaseChnFrame(VoLayer, VoChn, pstFrame);
    if (ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_GetChnFrame VoLayer %d VoChn %d failed with %#x!\n", VoLayer, VoChn, ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_hisiGetVoChnFrame
* 描  述  : 获取vo输出帧数据
* 输  入  : - pDispChn:
*         : - Ctrl    :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiGetVoChnFrame(UINT32 VoLayer, UINT32 VoChn, SYSTEM_FRAME_INFO *pstFrame)
{
    INT32 ret = SAL_SOK;
    VIDEO_FRAME_INFO_S stVideoFrame;
    SYSTEM_FRAME_INFO stSrcSysFrame;
    VIDEO_FRAME_INFO_S *pstHiFrame = NULL;
    UINT32 uWidth = 0;
    UINT32 uHeight = 0;
    VOID *pStreamBuff = NULL;

    if (NULL == pstFrame)
    {
        DISP_LOGE("is null !\n");
        return SAL_FAIL;
    }

    stSrcSysFrame.uiAppData = (PhysAddr) & stVideoFrame;
    ret = disp_hisiGetFrame(VoLayer, VoChn, &stSrcSysFrame);
    if (ret != HI_SUCCESS)
    {
        DISP_LOGE("Disp_hisiGetFrame failed with %#x!\n", ret);
        return SAL_FAIL;
    }

     /*暂时没有使用这个接口,先用memcpy拷贝*/
    /*HAL_bufQuickCopy_v0(&stSrcSysFrame, pstFrame, 0, 0, stVideoFrame.stVFrame.u32Width, stVideoFrame.stVFrame.u32Height);*/
    pstHiFrame = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    uWidth = stVideoFrame.stVFrame.u32Width;
    uHeight = stVideoFrame.stVFrame.u32Height;
    pStreamBuff = HI_ADDR_U642P(stVideoFrame.stVFrame.u64VirAddr);
    memcpy(HI_ADDR_U642P(pstHiFrame->stVFrame.u64VirAddr[0]), pStreamBuff, uWidth * uHeight);
    memcpy(HI_ADDR_U642P(pstHiFrame->stVFrame.u64VirAddr[1]), pStreamBuff + uWidth * uHeight, uWidth * uHeight / 2);
    pstHiFrame->stVFrame.u32Width = uWidth;
    pstHiFrame->stVFrame.u32Height = uHeight;
    pstHiFrame->stVFrame.u32Stride[0] = uWidth;


    ret = disp_hisiRelseseFrame(VoLayer, VoChn, &stSrcSysFrame);
    if (ret != HI_SUCCESS)
    {
        DISP_LOGE("Disp_hisiGetFrame failed with %#x!\n", ret);
        return SAL_FAIL;
    }

    DISP_LOGW("disp_hisiGetVoChnFrame Unsupported %d!\n", ret);

    return ret;
}


/*******************************************************************************
* 函数名  : Disp_hisiDeinitStartingup
* 描  述  : 开机时显示反初始化
* 输  入  : - uiDev  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiDeinitStartingup(UINT32 uiDev)
{
    HI_S32 s32Ret = HI_SUCCESS;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;

    uiLayer = uiDev;
    uiChn = 0;

    s32Ret = HI_MPI_VO_DisableChn(uiLayer, uiChn);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("Stop %d Layer chn %d Failed s32Ret 0x%x!!!\n", uiLayer, uiChn, s32Ret);
    }

    s32Ret = HI_MPI_VO_DisableVideoLayer(uiLayer);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("failed with %#x!\n", s32Ret);
    }

    s32Ret = HI_MPI_VO_Disable(uiDev);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("failed with %#x!\n", s32Ret);
    }

    return s32Ret;
}

/*******************************************************************************
* 函数名  : disp_hisiClearVoBuf
* 描  述  : 清空指定输出通道的缓存 buffer 数据
* 输  入  : - uiLayerNo: Vo设备号
*         : - uiVoChn  : Vo通道号
          : - bFlag    : 选择模式
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiClearVoBuf(UINT32 uiVoLayer, UINT32 uiVoChn, UINT32 bFlag)
{
    UINT32 s32Ret = SAL_FAIL;

    s32Ret = HI_MPI_VO_ClearChnBuf(uiVoLayer, uiVoChn, bFlag);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("error %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_hisiDisableChn
* 描  述  : 禁止vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiDisableChn(VOID *prm)
{
    UINT32 uiDev = 0;

    UINT32 uiLayer = 0;

    UINT32 uiChn = 0;

    HI_S32 s32Ret = HI_SUCCESS;

    DISP_CHN_COMMON *pDispChn = NULL;

    VO_BORDER_S stBorder = {0};

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

    s32Ret = HI_MPI_VO_DisableChn(uiLayer, uiChn);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("Stop %d Layer chn %d Failed s32Ret 0x%x!!!\n", uiLayer, uiChn, s32Ret);
        return SAL_FAIL;
    }

    stBorder.bBorderEn = 0;
    stBorder.stBorder.u32LeftWidth = 2;
    stBorder.stBorder.u32RightWidth = 2;
    stBorder.stBorder.u32TopWidth = 2;
    stBorder.stBorder.u32BottomWidth = 2;
    s32Ret = HI_MPI_VO_SetChnBorder(uiLayer, uiChn, &stBorder);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("HI_MPI_VO_SetChnBorder error s32Ret 0x%x!! bBorderEn %d u32Color %d LineW %d\n",s32Ret,stBorder.bBorderEn,stBorder.stBorder.u32Color,
            stBorder.stBorder.u32LeftWidth);
        //return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_hisiSetVideoLayerCsc
* 描  述  : 设置视频层图像输出效果
* 输  入  : - uiChn:
*         : - Lua  :
*         : - Con  :
*         : - Hue  :
*         : - Sat  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiSetVideoLayerCsc(UINT32 uiVoLayer, void *prm)
{
    INT32 s32Ret = HI_SUCCESS;
    VO_CSC_S stVoCscSt;
    VIDEO_CSC_S *pstCsc = NULL;

    pstCsc = (VIDEO_CSC_S *)prm;

    if (pstCsc == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    memset(&stVoCscSt, 0, sizeof(VO_CSC_S));

    s32Ret = HI_MPI_VO_GetVideoLayerCSC(uiVoLayer, &stVoCscSt);
    if (HI_SUCCESS != s32Ret)
    {
        DISP_LOGE("get video layer[%u] csc fail\n", uiVoLayer);
        return SAL_FAIL;
    }

    stVoCscSt.u32Luma = pstCsc->uiLuma;
    stVoCscSt.u32Contrast = pstCsc->uiContrast;
    stVoCscSt.u32Hue = pstCsc->uiHue;
    stVoCscSt.u32Satuature = pstCsc->uiSatuature;

    s32Ret = HI_MPI_VO_SetVideoLayerCSC(uiVoLayer, &stVoCscSt);
    if (HI_SUCCESS != s32Ret)
    {
        DISP_LOGE("HI_MPI_VO_SetVideoLayerCSC Layer %d CSC Failed, %x !!!\n", uiVoLayer, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : Disp_hisiEnableChn
* 描  述  : 使能vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiEnableChn(VOID *prm)
{
    UINT32 uiDev = 0;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;
    VO_BORDER_S stBorder = {0};

    VO_CHN_ATTR_S stChnAttr;
    INT32 s32Ret = HI_SUCCESS;

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

    memset(&stChnAttr, 0, sizeof(VO_CHN_ATTR_S));

    stChnAttr.stRect.s32X = pDispChn->uiX;
    stChnAttr.stRect.s32Y = pDispChn->uiY;
    stChnAttr.stRect.u32Width = pDispChn->uiW;
    stChnAttr.stRect.u32Height = pDispChn->uiH;
    stChnAttr.u32Priority = 0;
    stChnAttr.bDeflicker = HI_FALSE;

    DISP_LOGI("uiDev %d,VoLayer %d voChn %d x %4d y %4d w %4d h %4d !\n", uiDev, uiLayer, uiChn, stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y,
             stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);

    s32Ret = HI_MPI_VO_SetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_SetChnAttr s32Ret 0x%x uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VO_EnableChn(uiLayer, uiChn);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("HI_MPI_VO_EnableChn Layer %d  chn %d Failed s32Ret 0x%x !!!\n", uiLayer, uiChn, s32Ret);
        return SAL_FAIL;
    }

    stBorder.bBorderEn = pDispChn->bDispBorder;
    stBorder.stBorder.u32Color = pDispChn->BorDerColor;    
    stBorder.stBorder.u32LeftWidth = pDispChn->BorDerLineW;
    stBorder.stBorder.u32RightWidth = pDispChn->BorDerLineW;
    stBorder.stBorder.u32TopWidth = pDispChn->BorDerLineW;
    stBorder.stBorder.u32BottomWidth = pDispChn->BorDerLineW;
    if (stBorder.bBorderEn)
    {
        s32Ret = HI_MPI_VO_SetChnBorder(uiLayer, uiChn, &stBorder);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("HI_MPI_VO_SetChnBorder error s32Ret 0x%x!! bBorderEn %d u32Color %d LineW %d\n",s32Ret,stBorder.bBorderEn,stBorder.stBorder.u32Color,
                stBorder.stBorder.u32LeftWidth);
            //return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_hisiSetChnPos
* 描  述  : 设置vo参数（放大镜和小窗口）
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiSetChnPos(VOID *prm)
{
    UINT32 uiDev = 0;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;
    VO_BORDER_S stBorder = {0};

    VO_CHN_ATTR_S stChnAttr;
    INT32 s32Ret = HI_SUCCESS;

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

    memset(&stChnAttr, 0, sizeof(VO_CHN_ATTR_S));

    s32Ret = HI_MPI_VO_GetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_SetChnAttr s32Ret 0x%x uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }

    stChnAttr.stRect.s32X = SAL_align(pDispChn->uiX, 2);
    stChnAttr.stRect.s32Y = SAL_align(pDispChn->uiY, 2);
    stChnAttr.stRect.u32Width = pDispChn->uiW;
    stChnAttr.stRect.u32Height = pDispChn->uiH;
    stChnAttr.u32Priority = 0;
    stChnAttr.bDeflicker = HI_FALSE;

    DISP_LOGD("uiDev %d,VoLayer %d voChn %d x %4d y %4d w %4d h %4d !\n", uiDev, uiLayer, uiChn, stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y,
              stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);

    s32Ret = HI_MPI_VO_SetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_SetChnAttr s32Ret 0x%x uiDev %d,uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiDev, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }

    HI_MPI_VO_GetChnBorder(uiLayer,uiChn,&stBorder);

    if ((stBorder.bBorderEn != pDispChn->bDispBorder) || 
        (stBorder.stBorder.u32Color != pDispChn->BorDerColor) || 
        (stBorder.stBorder.u32LeftWidth != pDispChn->BorDerLineW))
    {
        stBorder.bBorderEn = pDispChn->bDispBorder;
        stBorder.stBorder.u32Color = pDispChn->BorDerColor;    
        stBorder.stBorder.u32LeftWidth = pDispChn->BorDerLineW;
        stBorder.stBorder.u32RightWidth = pDispChn->BorDerLineW;
        stBorder.stBorder.u32TopWidth = pDispChn->BorDerLineW;
        stBorder.stBorder.u32BottomWidth = pDispChn->BorDerLineW;
        s32Ret = HI_MPI_VO_SetChnBorder(uiLayer, uiChn, &stBorder);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("HI_MPI_VO_SetChnBorder error s32Ret 0x%x!! bBorderEn %d u32Color 0x%x LineW %d\n",s32Ret,stBorder.bBorderEn,stBorder.stBorder.u32Color,
                stBorder.stBorder.u32LeftWidth);
            //return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_hisiDeleteDev
* 描  述  : 删除显示层
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiDeleteDev(VOID *prm)
{
    UINT32 eVoDev = 0;

    VO_DEV VoDev = 0;
    VO_LAYER VoLayer = 0;

    HI_S32 s32Ret = HI_SUCCESS;

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

    DISP_LOGI("VoDev = %d VoLayer = %d \n", VoDev, VoLayer);

    s32Ret = HI_MPI_VO_DisableVideoLayer(VoLayer);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_DisableVideoLayer Layer %d failed with 0x%x !\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VO_Disable(VoDev);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_Disable VO %d failed with 0x%x !\n", VoDev, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_hisiCreateLayer
* 描  述  : 创建视频层
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_hisiCreateLayer(VOID *prm)
{
    UINT32 uiLayerWith = 0;
    UINT32 uiLayerHeight = 0;
    UINT32 uiLayerFps = 0;
    DISP_LAYER_COMMON *pstLayer = NULL;

    VO_LAYER VoLayer = 0;
    HI_S32 s32Ret = HI_SUCCESS;

    VO_VIDEO_LAYER_ATTR_S stLayerAttr;

    memset(&stLayerAttr, 0, sizeof(VO_VIDEO_LAYER_ATTR_S));

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
    stLayerAttr.bClusterMode = HI_FALSE;
    stLayerAttr.bDoubleFrame = HI_FALSE;
    stLayerAttr.enPixFormat = PIXEL_FORMAT_YVU_SEMIPLANAR_420;

    /* HDR模式下不支持SDR数据，SDR模式下支持HDR数据显示 */
    stLayerAttr.enDstDynamicRange = DYNAMIC_RANGE_SDR8;

    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;
    stLayerAttr.stDispRect.u32Width = uiLayerWith;
    stLayerAttr.stDispRect.u32Height = uiLayerHeight;
    stLayerAttr.u32DispFrmRt = uiLayerFps;

    stLayerAttr.stImageSize.u32Width = stLayerAttr.stDispRect.u32Width;
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;

    DISP_LOGI("Disp VoLayer %d u32Width %d u32Height %d uiDevFps %d !!!\n", VoLayer, uiLayerWith, uiLayerHeight, uiLayerFps);

    s32Ret = HI_MPI_VO_SetDisplayBufLen(VoLayer, 5); /* 取值[0] [3,15] */
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_SetDisplayBufLen VoLayer %d failed with %#x!\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VO_SetVideoLayerAttr(VoLayer, &stLayerAttr);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_SetVideoLayerAttr VoLayer %d failed with %#x!\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VO_EnableVideoLayer(VoLayer);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_EnableVideoLayer Layer %d failed with %#x!\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_halIsSupportIntfSync
* 描  述  : 对应的接口是否支持对应的时序
* 输  入  : VO_DEV voDev : 通道号
          VO_INTF_TYPE_E enIntfType : 输出接口
          VO_INTF_SYNC_E enIntfSync : 想要输出的时序
* 输  出  : BOOL : 支持返回TRUE，否则返回FALSE
* 返回值  : BOOL : 支持返回TRUE，否则返回FALSE
*******************************************************************************/
static BOOL disp_halIsSupportIntfSync(VO_DEV voDev, VO_INTF_TYPE_E enIntfType, VO_INTF_SYNC_E enIntfSync)
{

    UINT32 i = 0;
    const VO_INTF_SYNC_E *penSupportIntfSync = NULL;

    if (voDev >= VO_DEV_MAX)
    {
        DISP_LOGE("invalid dev[%d]\n", voDev);
        return SAL_FAIL;
    }

    switch (enIntfType)
    {
        case VO_INTF_HDMI:
            penSupportIntfSync = aenHdmiSupportRes[voDev];
            break;
        case VO_INTF_MIPI:
            penSupportIntfSync = aenMipiSupportRes[voDev];
            break;
        default:
            DISP_LOGE("unsupport intf type[%d]\n", enIntfType);
            return SAL_FALSE;
    }

    for (i = 0; (i < VO_OUTPUT_BUTT) && (*penSupportIntfSync != VO_OUTPUT_BUTT); i++, penSupportIntfSync++)
    {
        if (*penSupportIntfSync == enIntfSync)
        {
            return SAL_TRUE;
        }
    }

    return SAL_FALSE;
}


/*******************************************************************************
* 函数名  : disp_hisiCreateDev
* 描  述  : 创建显示
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisiCreateDev(DISP_DEV_COMMON *pDispDev)
{
    UINT32 eVoDev = 0;
    UINT32 uiDevWith = 0;
    UINT32 uiDevHeight = 0;
    UINT32 uiDevFps = 0;
    UINT64 u64PixelClock = 0;
    VO_DEV VoDev = 0;
    HI_S32 s32Ret = HI_SUCCESS;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_USER_INTFSYNC_INFO_S stUserInfo = {0};
    VO_INTF_SYNC_E enIntfSync = 0;
    const DISP_TIMING_MAP_S *pstDispTiming = NULL;
    const BT_TIMING_S *pstBtTiming = NULL;

    /* 3个数相乘等于24，第一个数逐渐递增，第二个数大于第三个数 */
    static const UINT32 au32Mul24[][3] = {
        {1,     6,      4},
        {2,     4,      3},
        {3,     4,      2},
        {4,     3,      2},
        {6,     2,      2},
        {8,     3,      1},
        {12,    2,      1},
        {24,    1,      1},
    };
    UINT32 u32Num = 0;
    UINT32 i = 0;

    if ((pDispDev == NULL))
    {
        DISP_LOGE("null pointer\n");
        return SAL_FAIL;
    }

    pstDispTiming = disp_hisiGetTiming(pDispDev);
    if(NULL == pstDispTiming)
    {
        DISP_LOGE("disp_hisiCreateDev failed!!!\n");
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

    memset(&stVoPubAttr, 0, sizeof(VO_PUB_ATTR_S));

    if (VO_DEV_HDMI == eVoDev)
    {
        stVoPubAttr.enIntfType = VO_INTF_HDMI;

        s32Ret = Hdmi_Hal_Init();
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("error !!\n");
            return SAL_FAIL;
        }
        
    }
    else if (VO_DEV_MIPI == eVoDev)
    {
        stVoPubAttr.enIntfType = VO_INTF_MIPI;
    }
    else
    {
        stVoPubAttr.enIntfType = VO_INTF_BT1120;
        disp_hisiDevBlanking(uiDevWith, uiDevHeight, &stVoPubAttr);
    }

    /* 先检查是否支持该分辨率时序 */
    if (SAL_TRUE != disp_halIsSupportIntfSync(VoDev, stVoPubAttr.enIntfType, enIntfSync))
    {
        /* 检查是否支持用户模式 */
        enIntfSync = VO_OUTPUT_USER;
        if (SAL_TRUE != disp_halIsSupportIntfSync(VoDev, stVoPubAttr.enIntfType, enIntfSync))
        {
            DISP_LOGE("disp dev[%d] mode[%u] not support user mode\n", VoDev, eVoDev);
            return SAL_FAIL;
        }
    }

    stVoPubAttr.u32BgColor = pDispDev->uiBgcolor;
    stVoPubAttr.enIntfSync = enIntfSync;
    
    if (VO_OUTPUT_USER == stVoPubAttr.enIntfSync)
    {
        //stUserInfo.u32PreDiv = 1;
        //hdmi需要分频，mipi不需要分频
        if (pDispDev->eVoDev == VO_DEV_MIPI)
        {
            stUserInfo.u32DevDiv = 1;
        }
        else if (pDispDev->eVoDev == VO_DEV_HDMI)
        {
            stUserInfo.u32DevDiv = 2;
        }

        stVoPubAttr.stSyncInfo.bSynm    = 0;                /* BT656 */
        stVoPubAttr.stSyncInfo.bIop     = 1;                /* 逐行 */
        stVoPubAttr.stSyncInfo.u8Intfb  = 1;
        stVoPubAttr.stSyncInfo.u16Vact  = pstBtTiming->u32Height;
        stVoPubAttr.stSyncInfo.u16Vbb   = pstBtTiming->u32VBackPorch + pstBtTiming->u32VSync;
        stVoPubAttr.stSyncInfo.u16Vfb   = pstBtTiming->u32VFrontPorch;
        stVoPubAttr.stSyncInfo.u16Hact  = pstBtTiming->u32Width;
        stVoPubAttr.stSyncInfo.u16Hbb   = pstBtTiming->u32HBackPorch + pstBtTiming->u32HSync;
        stVoPubAttr.stSyncInfo.u16Hfb   = pstBtTiming->u32HFrontPorch;
        stVoPubAttr.stSyncInfo.u16Hmid  = 1;
        stVoPubAttr.stSyncInfo.u16Bvact = 0;
        stVoPubAttr.stSyncInfo.u16Bvbb  = 0;
        stVoPubAttr.stSyncInfo.u16Bvfb  = 0;
        stVoPubAttr.stSyncInfo.u16Hpw   = pstBtTiming->u32HSync;
        stVoPubAttr.stSyncInfo.u16Vpw   = pstBtTiming->u32VSync;
        stVoPubAttr.stSyncInfo.bIdv     = 0;
        stVoPubAttr.stSyncInfo.bIhs     = (pstBtTiming->enHSPol == POLARITY_NEG) ? HI_TRUE : HI_FALSE;
        stVoPubAttr.stSyncInfo.bIvs     = (pstBtTiming->enVSPol == POLARITY_NEG) ? HI_TRUE : HI_FALSE;
        
        /* 下列结构体的数值根据公式凑数即可 */
        /* 像素时钟=24*(u32Fbdiv+u32Frac/2^24)/u32Refdiv/(u32Postdiv1*u32Postdiv2) */
        /* 将24/u32Refdiv/(u32Postdiv1*u32Postdiv2)=1,u32Fbdiv等于像素时钟的整数位(以MHz为单位)，u32Frac/2^24为小数 */
        u64PixelClock = pstBtTiming->u32PixelClock;
        
        stUserInfo.stUserIntfSyncAttr.enClkSource = VO_CLK_SOURCE_PLL;
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Fbdiv = (UINT32)(u64PixelClock / 1000000);

        u64PixelClock -= ((u64PixelClock / 1000000) * 1000000);
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Frac = (UINT32)(u64PixelClock * (0x01 << 24) / 1000000);

        /* FOUTVCO=24*(u32Fbdiv+u32Frac/2^24)/u32Refdiv要在[800,3200]范围内 */
        /* 以下计算公式理论上能支持像素时钟在[33.33, 3200]之间的任意分辨率 */
        u32Num = sizeof(au32Mul24)/sizeof(au32Mul24[0]);

        /* 先找出一个小于等于3200的一个数 */
        while ((i < u32Num) && ((UINT64)pstBtTiming->u32PixelClock * au32Mul24[i][1] * au32Mul24[i][2] > (UINT64)3200 * 1000000))
        {
            i++;
        }

        /* 校验是不是小于等于3200 */
        if ((UINT64)pstBtTiming->u32PixelClock * au32Mul24[i][1] * au32Mul24[i][2] > (UINT64)3200 * 1000000)
        {
            DISP_LOGE("invalid pixel clock:%u\n", pstBtTiming->u32PixelClock);
            return SAL_FAIL;
        }

        /* 校验是不是大于等于800 */
        if ((UINT64)pstBtTiming->u32PixelClock * au32Mul24[i][1] * au32Mul24[i][2] < (UINT64)800 * 1000000)
        {
            DISP_LOGE("invalid pixel clock:%u\n", pstBtTiming->u32PixelClock);
            return SAL_FAIL;
        }
        
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Refdiv   = au32Mul24[i][0];
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv1 = au32Mul24[i][1];
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv2 = au32Mul24[i][2];
    }
    
    s32Ret = HI_MPI_VO_SetPubAttr(VoDev, &stVoPubAttr);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_SetPubAttr VoDev %d failed with %#x!\n", VoDev, s32Ret);
        return SAL_FAIL;
    }

    if (VO_OUTPUT_USER == stVoPubAttr.enIntfSync)
    {
        s32Ret = HI_MPI_VO_SetUserIntfSyncInfo(VoDev, &stUserInfo);
        if (s32Ret != HI_SUCCESS)
        {
            DISP_LOGE("[VoDev:%d]Set user interface sync info failed with %x.\n",VoDev,s32Ret);
            return SAL_FAIL;
        }
        
        HI_MPI_VO_SetDevFrameRate(VoDev, uiDevFps);
    }

    s32Ret = HI_MPI_VO_Enable(VoDev);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_Enable VoDev %d failed with %#x!\n", VoDev, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static INT32 disp_hisiGetHdmiEdid(UINT32 u32HdmiId, UINT8 *pu8Buff, UINT32 *pu32Len)
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_HDMI_EDID_S stEdid;
    HI_HDMI_ID_E  enHdmi = u32HdmiId;

    if(NULL == pu8Buff || pu32Len == NULL || enHdmi >= HI_HDMI_ID_BUTT)
    {
        EDID_LOGE("hdmi[%d] get edid fail:0x%x\n", enHdmi, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_HDMI_Force_GetEDID(enHdmi, &stEdid);
    if (HI_SUCCESS != s32Ret)
    {
        EDID_LOGE("hdmi[%d] get edid fail:0x%x\n", enHdmi, s32Ret);
        return SAL_FAIL;
    }

    if ((HI_TRUE != stEdid.bEdidValid) || (0 == stEdid.u32Edidlength) || (stEdid.u32Edidlength > DISP_EDID_TOTAL_MAX))
    {
        EDID_LOGE("hdmi[%d] edid is invalid[%d] or num[%u] error\n", enHdmi, stEdid.bEdidValid, stEdid.u32Edidlength);
        return SAL_FAIL;
    }

    sal_memcpy_s(pu8Buff, DISP_EDID_TOTAL_MAX, stEdid.u8Edid, stEdid.u32Edidlength);

    *pu32Len = stEdid.u32Edidlength;

    return SAL_SOK;
    
}

/*******************************************************************************
* 函数名  : Disp_halSetVoChnPriority
* 描  述  : 设置窗口显示优先级
* 输  入  : - uiDev:
*         : - prm  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_hisisetVoChnPriority(VOID *prm)
{
    UINT32 uiDev = 0;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;
    VO_CHN_ATTR_S stChnAttr;
    INT32 s32Ret = HI_SUCCESS;

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

    memset(&stChnAttr, 0, sizeof(VO_CHN_ATTR_S));
    s32Ret = HI_MPI_VO_GetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_SetChnAttr s32Ret 0x%x uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }
    
    stChnAttr.u32Priority = pDispChn->u32Priority;
    s32Ret = HI_MPI_VO_SetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != HI_SUCCESS)
    {
        DISP_LOGE("HI_MPI_VO_SetChnAttr s32Ret 0x%x uiDev %d,uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiDev, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }
    DISP_LOGD("dev %d chn %d u32Priority %d \n",uiLayer,uiChn,stChnAttr.u32Priority);
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

    g_stDispPlatOps.createVoDev =       disp_hisiCreateDev;
    g_stDispPlatOps.deleteVoDev =       disp_hisiDeleteDev;
    g_stDispPlatOps.setVoChnParam =     disp_hisiSetChnPos;
    g_stDispPlatOps.setVoLayerCsc =     disp_hisiSetVideoLayerCsc;
    g_stDispPlatOps.enableVoChn =       disp_hisiEnableChn;
    g_stDispPlatOps.clearVoChnBuf =     disp_hisiClearVoBuf;
    g_stDispPlatOps.disableVoChn  =     disp_hisiDisableChn;
    g_stDispPlatOps.deinitVoStartingup = disp_hisiDeinitStartingup;
    g_stDispPlatOps.putVoFrameInfo =    disp_hisiPutFrameInfo;
    g_stDispPlatOps.sendVoChnFrame =    disp_hisiSendFrame;
    g_stDispPlatOps.getVoChnFrame =     disp_hisiGetVoChnFrame;
    g_stDispPlatOps.setVoInterface =    disp_hisiSetVoInterface;
    g_stDispPlatOps.getHdmiEdid   =     disp_hisiGetHdmiEdid;
    g_stDispPlatOps.createVoLayer =     disp_hisiCreateLayer;
    g_stDispPlatOps.setVoChnPriority =  disp_hisisetVoChnPriority;
    
    return &g_stDispPlatOps;
}

