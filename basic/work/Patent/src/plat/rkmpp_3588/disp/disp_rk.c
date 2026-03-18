/**
 * @file   disp_rk.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  RK3588显示平台层
 * @author liuxianying
 * @date   2022/5/19
 * @note
 * @note \n History
   1.日    期: 2022/5/19
     作    者: liuxianying
     修改历史: 创建文件
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <platform_sdk.h>
#include <platform_hal.h>

#include "capbility.h"
#include "disp_rk.h"
#include "../../hal/hal_inc_inter/disp_hal_inter.h"
#include "fmtConvert_rk.h"


#include "stdio.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#line __LINE__ "disp_rk.c"

#define disp_HAL_GET_HZ_OFFSET(x) ((94 * (((x) >> 8) - 0xa0 - 1) + (((x) & 0xff) - 0xa0 - 1)) * 32)
#define DISP_EDID_TOTAL_MAX (512)               /* EDID最大值 */
#define disp_VGS_CHN_START	6

#define DISP_CHECK_RET(ret, ret_success, str) \
    { \
        if (ret != ret_success) \
        { \
            DISP_LOGE("%s, ret: 0x%x \n", str, ret); \
            return SAL_FAIL; \
        } \
    }

static INT8 wbc_enable_judge = 0; /*wbc使能标志参数，防止关闭回写操作时抓取图片*/

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

static DISP_PLAT_OPS g_stDispPlatOps;
/* 自定义需要支持的时序，每新增一种分辨率时序都需要增加到该数组中 */
static const DISP_TIMING_MAP_S gastDispTimingMap[] = {
    {1920, 1200, 50, VO_OUTPUT_USER,           TIMING_CVT_1920X1200P50},

    {1920, 1080, 120, VO_OUTPUT_1080P120,      TIMING_CVT_1920X1080P120_RB},
    {1920, 1080, 75,  VO_OUTPUT_USER,          TIMING_CVT_1920X1080P75},

    {1920, 1080, 72,  VO_OUTPUT_USER,          TIMING_CVT_1920X1080P72_R2},
    {1920, 1080, 60,  VO_OUTPUT_1080P60,       TIMING_VESA_1920X1080P60},
    {1920, 1080, 50,  VO_OUTPUT_1080P50,       TIMING_CVT_1920X1080P50},
    {1920, 1080, 90,  VO_OUTPUT_USER,          TIMING_VESA_1920X1080P90},
    {1920, 1080, 100,  VO_OUTPUT_USER,         TIMING_VESA_1920X1080P100},

    {1680, 1050, 60,  VO_OUTPUT_USER,          TIMING_CVT_1680X1050P60},

    {1280, 1024, 85,  VO_OUTPUT_USER,          TIMING_VESA_1280X1024P85},
    {1280, 1024, 75,  VO_OUTPUT_USER,          TIMING_VESA_1280X1024P75},
    {1280, 1024, 72,  VO_OUTPUT_USER,          TIMING_GTF_1280X1024P72},
    {1280, 1024, 70,  VO_OUTPUT_USER,          TIMING_GTF_1280X1024P70},
    {1280, 1024, 60,  VO_OUTPUT_1280x1024_60,  TIMING_VESA_1280X1024P60},

    {1280, 800, 85,  VO_OUTPUT_USER,           TIMING_VESA_1280X800P85},
    {1280, 800, 75,  VO_OUTPUT_USER,           TIMING_CVT_1280X800P75},
    {1280, 800, 72,  VO_OUTPUT_USER,           TIMING_GTF_1280X800P72},
    {1280, 800, 60,  VO_OUTPUT_1280x800_60,    TIMING_CVT_1280X800P60},

    {1280, 768, 85,  VO_OUTPUT_USER,           TIMING_VESA_1280X768P85},
    {1280, 768, 75,  VO_OUTPUT_USER,           TIMING_VESA_1280X768P75},
    {1280, 768, 72,  VO_OUTPUT_USER,           TIMING_GTF_1280X768P72},
    {1280, 768, 60,  VO_OUTPUT_USER,           TIMING_CVT_1280X768P60},

    {1280, 720, 72,  VO_OUTPUT_USER,           TIMING_GTF_1280X720P72},
        
    /* RK芯片特殊，要求输出时序的水平参数:htotal、hactive、hbp、hsync、hfp 都4对齐做约束 
        VESA标准的1280X720P60,hfp为110没有4对齐,所以RK平台改用CVT标准的;
    */
    {1280, 720, 60,  VO_OUTPUT_720P60,         TIMING_CVT_1280X720P60},
    {1280, 720, 50,  VO_OUTPUT_USER,           TIMING_CVT_1280X720P50},

    {1024, 768, 85,  VO_OUTPUT_USER,           TIMING_VESA_1024X768P85},
    {1024, 768, 75,  VO_OUTPUT_USER,           TIMING_VESA_1024X768P75},
    {1024, 768, 70,  VO_OUTPUT_USER,           TIMING_VESA_1024X768P70},
    {1024, 768, 72,  VO_OUTPUT_USER,           TIMING_CVT_1024X768P72},
    {1024, 768, 60,  VO_OUTPUT_1024x768_60,    TIMING_VESA_1024X768P60},
};

/* HDMI支持的分辨率时序，参考数据手册 */
static const VO_INTF_SYNC_E aenHdmiSupportRes[VO_DEV_MAX][VO_OUTPUT_BUTT] = {
    /* DHD0 */
    {
        VO_OUTPUT_1080P24, VO_OUTPUT_1080P25, VO_OUTPUT_1080P30, VO_OUTPUT_720P50,
        VO_OUTPUT_720P60, VO_OUTPUT_1080P50, VO_OUTPUT_1080P60, VO_OUTPUT_1080P120,
        VO_OUTPUT_576P50, VO_OUTPUT_480P60, VO_OUTPUT_800x600_60, VO_OUTPUT_1024x768_60,
        VO_OUTPUT_1280x1024_60, VO_OUTPUT_1366x768_60, VO_OUTPUT_1440x900_60, VO_OUTPUT_1280x800_60,
        VO_OUTPUT_1600x1200_60, VO_OUTPUT_1680x1050_60, VO_OUTPUT_1920x1200_60, VO_OUTPUT_640x480_60,
        VO_OUTPUT_1920x2160_30, VO_OUTPUT_2560x1440_30, VO_OUTPUT_2560x1440_60, VO_OUTPUT_2560x1600_60,
        VO_OUTPUT_3840x2160_24, VO_OUTPUT_3840x2160_25, VO_OUTPUT_3840x2160_30, VO_OUTPUT_3840x2160_50,
        VO_OUTPUT_3840x2160_60, VO_OUTPUT_4096x2160_24, VO_OUTPUT_4096x2160_25, VO_OUTPUT_4096x2160_30,
        VO_OUTPUT_4096x2160_50, VO_OUTPUT_4096x2160_60, VO_OUTPUT_USER, VO_OUTPUT_BUTT,
    },

    /* DHD1 */
    {
        VO_OUTPUT_1080P24, VO_OUTPUT_1080P25, VO_OUTPUT_1080P30, VO_OUTPUT_720P50,
        VO_OUTPUT_720P60, VO_OUTPUT_1080P50, VO_OUTPUT_1080P60, VO_OUTPUT_1080P120,
        VO_OUTPUT_576P50, VO_OUTPUT_480P60, VO_OUTPUT_800x600_60, VO_OUTPUT_1024x768_60,
        VO_OUTPUT_1280x1024_60, VO_OUTPUT_1366x768_60, VO_OUTPUT_1440x900_60, VO_OUTPUT_1280x800_60,
        VO_OUTPUT_1600x1200_60, VO_OUTPUT_1680x1050_60, VO_OUTPUT_1920x1200_60, VO_OUTPUT_640x480_60,
        VO_OUTPUT_1920x2160_30, VO_OUTPUT_2560x1440_30, VO_OUTPUT_2560x1440_60, VO_OUTPUT_2560x1600_60,
        VO_OUTPUT_3840x2160_24, VO_OUTPUT_3840x2160_25, VO_OUTPUT_3840x2160_30, VO_OUTPUT_3840x2160_50,
        VO_OUTPUT_3840x2160_60, VO_OUTPUT_4096x2160_24, VO_OUTPUT_4096x2160_25, VO_OUTPUT_4096x2160_30,
        VO_OUTPUT_4096x2160_50, VO_OUTPUT_4096x2160_60, VO_OUTPUT_USER, VO_OUTPUT_BUTT,
    },

    /* DHD2 */
    {
        VO_OUTPUT_1080P24, VO_OUTPUT_1080P25, VO_OUTPUT_1080P30, VO_OUTPUT_720P50,
        VO_OUTPUT_720P60, VO_OUTPUT_1080P50, VO_OUTPUT_1080P60, VO_OUTPUT_1080P120,
        VO_OUTPUT_576P50, VO_OUTPUT_480P60, VO_OUTPUT_800x600_60, VO_OUTPUT_1024x768_60,
        VO_OUTPUT_1280x1024_60, VO_OUTPUT_1366x768_60, VO_OUTPUT_1440x900_60, VO_OUTPUT_1280x800_60,
        VO_OUTPUT_1600x1200_60, VO_OUTPUT_1680x1050_60, VO_OUTPUT_1920x1200_60, VO_OUTPUT_640x480_60,
        VO_OUTPUT_1920x2160_30, VO_OUTPUT_2560x1440_30, VO_OUTPUT_2560x1440_60, VO_OUTPUT_2560x1600_60,
        VO_OUTPUT_3840x2160_24, VO_OUTPUT_3840x2160_25, VO_OUTPUT_3840x2160_30, VO_OUTPUT_3840x2160_50,
        VO_OUTPUT_3840x2160_60, VO_OUTPUT_4096x2160_24, VO_OUTPUT_4096x2160_25, VO_OUTPUT_4096x2160_30,
        VO_OUTPUT_4096x2160_50, VO_OUTPUT_4096x2160_60, VO_OUTPUT_USER, VO_OUTPUT_BUTT,
    },

    /* DHD3 */
    {
        VO_OUTPUT_BUTT,
    }
};

/* MIPI支持的分辨率时序，参考数据手册 */
static const VO_INTF_SYNC_E aenMipiSupportRes[VO_DEV_MAX][VO_OUTPUT_BUTT] = {
    /* DHD0 */
    {
        VO_OUTPUT_BUTT,
    },
    /* DHD1 */
    {
        VO_OUTPUT_BUTT,
    },

    /* DHD2 */
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

    /* DHD3 */
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
    }
};


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/* extern void Mpi_Tx_Start(const BT_TIMING_S *pstTiming); */
/* extern int Mpi_Tx_SetMipi(void); */

/*******************************************************************************
* 函数名  : disp_rkGetDevNo
* 描  述  : 对上层传下来的设备号进行转换
* 输  入  : - uiDevWith   :
*         : - uiDevHeight :
*         : - pstVoPubAttr:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static UINT32 disp_rkGetDevNo(UINT32 u32DevNo)
{
    UINT32 u32BoardType;

    u32BoardType = HARDWARE_GetBoardType();

    /* DB_TS3719_V1_0板子使用的设备号是2和3的mipi输出 */
    if (DB_TS3719_V1_0 == u32BoardType || DB_RS20046_V1_0 == u32BoardType)
    {

        return u32DevNo + 2;

    }

    return u32DevNo;
}

/**
 * @function    disp_rkGetTiming
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static const DISP_TIMING_MAP_S *disp_rkGetTiming(DISP_DEV_COMMON *pDispDev)
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

/*******************************************************************************
* 函数名  : disp_rkDevBlanking
* 描  述  : 消隐区设置
* 输  入  : - uiDevWith   :
*         : - uiDevHeight :
*         : - pstVoPubAttr:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkDevBlanking(UINT32 uiDevWith, UINT32 uiDevHeight, VO_PUB_ATTR_S *pstVoPubAttr)
{
    RK_U32 u32Addr = 0x12010044;
    RK_U32 u32Value = 0;

    if (NULL == pstVoPubAttr)
    {
        DISP_LOGE("!!!\n");

        return SAL_FAIL;
    }

    /* 1. 配置vo 属性 */
    /* pstVoPubAttr->enIntfSync = VO_OUTPUT_USER; */

    pstVoPubAttr->stSyncInfo.bSynm = 1;
    pstVoPubAttr->stSyncInfo.bIop = 1;


    /* 宽和高为自定义值，写成魔鬼数字 */
    if ((2048 == uiDevWith) && (600 == uiDevHeight))
    {
        /* 1024*600 修改 vo 输出频率到 107 M*/
/*        RK_MPI_SYS_GetReg(u32Addr, &u32Value); */

        DISP_LOGI("u32Addr : %x u32Value : %x \n", u32Addr, u32Value);
/*        RK_MPI_SYS_SetReg(u32Addr, 0xc000); */

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
/*        RK_MPI_SYS_GetReg(u32Addr, &u32Value); */

        DISP_LOGI("u32Addr : %x u32Value : %x \n", u32Addr, u32Value);
/*        RK_MPI_SYS_SetReg(u32Addr, 0xc000); */

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
        /* 1024*600 修改 vo 输出频率到 107 M*/
/*        RK_MPI_SYS_GetReg(u32Addr, &u32Value); */

        DISP_LOGI("u32Addr : %x u32Value : %x \n", u32Addr, u32Value);
/*        RK_MPI_SYS_SetReg(u32Addr, 0xc000); */

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
    RK_S32 s32Ret = SAL_SOK;
    VO_CSC_S stVideoCSC = {0};

    if ((pDispDev == NULL) || (NULL == pstTiming))
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    uiVoLayer = pDispDev->szLayer.uiLayerNo;

    /* Mpi_Tx_Start(pstTiming); */

    s32Ret = RK_MPI_VO_GetPostProcessParam(uiVoLayer, &stVideoCSC);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("RK_MPI_VO_GetVideoLayerCSC failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    stVideoCSC.enCscMatrix = VO_CSC_MATRIX_BT709_TO_RGB_PC;
    s32Ret = RK_MPI_VO_SetPostProcessParam(uiVoLayer, &stVideoCSC);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("RK_MPI_VO_SetVideoLayerCSC failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

#if 0
    GRAPHIC_LAYER GraphicLayer = 1;
    VO_CSC_S stCSC = {0};

    s32Ret = RK_MPI_VO_GetGraphicLayerCSC(GraphicLayer, &stCSC);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("RK_MPI_VO_GetVideoLayerCSC failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

    stCSC.enCscMatrix = VO_CSC_MATRIX_IDENTITY;
    s32Ret = RK_MPI_VO_SetGraphicLayerCSC(GraphicLayer, &stCSC);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("RK_MPI_VO_SetVideoLayerCSC failed!s32Ret 0x%x \n", s32Ret);
        return SAL_FAIL;
    }

#endif
    /* Mpi_Tx_SetMipi(); */

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

    return SAL_SOK;
}

/**
 * @function    disp_rkSetVoInterface
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 disp_rkSetVoInterface(DISP_DEV_COMMON *pDispDev)
{
    INT32 s32Ret = SAL_SOK;
    const DISP_TIMING_MAP_S *pstDispTiming = NULL;
    const BT_TIMING_S *pstBtTiming = NULL;

    if (pDispDev == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    pstDispTiming = disp_rkGetTiming(pDispDev);
    if (NULL == pstDispTiming)
    {
        DISP_LOGE("disp_rkCreateDev failed!!!\n");
        return SAL_FAIL;
    }

    /* 获取相应时序 */
    pstBtTiming = &(BT_GetTiming(pstDispTiming->enUserTiming)->stTiming);
    if ((VO_DEV_MIPI == pDispDev->eVoDev) || (VO_DEV_MIPI_1 == pDispDev->eVoDev))
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
* 函数名  : disp_rkSendFrame
* 描  述  : 将数据帧赋值给显示帧
* 输  入  : - prm        :
*         : - pFrame     :
*         : - s32MilliSec:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkPutFrameInfo(SAL_VideoFrameBuf *videoFrame, VOID *pFrame)
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
    pStreamBuff = (void *)(videoFrame->virAddr[0]);

    memcpy((void *)(pstHiFrame->stVFrame.pVirAddr[0]), pStreamBuff, uWidth * uHeight * 3);
    pstHiFrame->stVFrame.u32Width = uWidth;
    pstHiFrame->stVFrame.u32Height = uHeight;

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_rkSendFrame
* 描  述  : 将数据送至vo通道
* 输  入  : - prm        :
*         : - pFrame     :
*         : - s32MilliSec:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkSendFrame(VOID *prm, VOID *pFrame, INT32 s32MilliSec)
{
    RK_S32 s32Ret = SAL_SOK;
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

    s32Ret = RK_MPI_VO_SendFrame(VoLayer, VoChn, pstVideoFrame, s32MilliSec);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SendFrame VoLayer %d VoChn %d failed with %#x!\n", VoLayer, VoChn, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_rkGetFrame
* 描  述  : 获取视频输出通道帧
* 输  入  : - pDispChn      :
*         : - pstSystemFrame:
*         : - uiRotate      :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkGetFrame(VO_LAYER VoLayer, VO_CHN VoChn, SYSTEM_FRAME_INFO *pstSysFrame)
{
    INT32 ret = SAL_SOK;
    VIDEO_FRAME_INFO_S *pstFrame = NULL;

    pstFrame = (VIDEO_FRAME_INFO_S *)pstSysFrame->uiAppData;

    if (NULL == pstFrame)
    {
        DISP_LOGE("VoLayer %d VoChn %d failed param is null\n", VoLayer, VoChn);
        return SAL_FAIL;
    }

    ret = RK_MPI_VO_GetChnFrame(VoLayer, VoChn, pstFrame, 50);
    if (ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_GetChnFrame VoLayer %d VoChn %d failed with %#x!\n", VoLayer, VoChn, ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_rkRelseseFrame
* 描  述  : 释放视频输出通道帧
* 输  入  : - pDispChn      :
*         : - pstSystemFrame:
*         : - uiRotate      :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkRelseseFrame(VO_LAYER VoLayer, VO_CHN VoChn, SYSTEM_FRAME_INFO *pstSysFrame)
{
    INT32 ret = SAL_SOK;
    VIDEO_FRAME_INFO_S *pstFrame = NULL;

    pstFrame = (VIDEO_FRAME_INFO_S *)pstSysFrame->uiAppData;

    if (NULL == pstFrame)
    {
        DISP_LOGE("VoLayer %d VoChn %d failed param is null\n", VoLayer, VoChn);
        return SAL_FAIL;
    }

    ret = RK_MPI_VO_ReleaseChnFrame(VoLayer, VoChn, pstFrame);
    if (ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_GetChnFrame VoLayer %d VoChn %d failed with %#x!\n", VoLayer, VoChn, ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_rkGetVoChnFrame
* 描  述  : 获取vo输出帧数据
* 输  入  : - pDispChn:
*         : - Ctrl    :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkGetVoChnFrame(UINT32 VoLayer, UINT32 VoChn, SYSTEM_FRAME_INFO *pstFrame)
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
    ret = disp_rkGetFrame(VoLayer, VoChn, &stSrcSysFrame);
    if (ret != SAL_SOK)
    {
        DISP_LOGE("Disp_rkGetFrame failed with %#x!\n", ret);
        return SAL_FAIL;
    }

    /*暂时没有使用这个接口,先用memcpy拷贝*/
    /*HAL_bufQuickCopy_v0(&stSrcSysFrame, pstFrame, 0, 0, stVideoFrame.stVFrame.u32Width, stVideoFrame.stVFrame.u32Height);*/
    pstHiFrame = (VIDEO_FRAME_INFO_S *)pstFrame->uiAppData;
    uWidth = stVideoFrame.stVFrame.u32Width;
    uHeight = stVideoFrame.stVFrame.u32Height;
    pStreamBuff = (void *)(stVideoFrame.stVFrame.pVirAddr);
    memcpy((void *)(pstHiFrame->stVFrame.pVirAddr[0]), pStreamBuff, uWidth * uHeight);
    memcpy((void *)(pstHiFrame->stVFrame.pVirAddr[1]), pStreamBuff + uWidth * uHeight, uWidth * uHeight / 2);
    pstHiFrame->stVFrame.u32Width = uWidth;
    pstHiFrame->stVFrame.u32Height = uHeight;



    ret = disp_rkRelseseFrame(VoLayer, VoChn, &stSrcSysFrame);
    if (ret != SAL_SOK)
    {
        DISP_LOGE("Disp_rkGetFrame failed with %#x!\n", ret);
        return SAL_FAIL;
    }

    DISP_LOGW("disp_rkGetVoChnFrame Unsupported %d!\n", ret);

    return ret;
}

/*******************************************************************************
* 函数名  : Disp_rkDeinitStartingup
* 描  述  : 开机时显示反初始化
* 输  入  : - uiDev  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkDeinitStartingup(UINT32 uiDev)
{
    RK_S32 s32Ret = SAL_SOK;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;

    uiLayer = uiDev;
    uiChn = 0;

    s32Ret = RK_MPI_VO_DisableChn(uiLayer, uiChn);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGW("Stop %d Layer chn %d Failed s32Ret 0x%x!!!\n", uiLayer, uiChn, s32Ret);
    }

    s32Ret = RK_MPI_VO_DisableLayer(uiLayer);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGW("failed with %#x!\n", s32Ret);
    }

    s32Ret = RK_MPI_VO_Disable(uiDev);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGW("failed with %#x!\n", s32Ret);
    }

    return s32Ret;
}

/*******************************************************************************
* 函数名  : disp_rkClearVoBuf
* 描  述  : 清空指定输出通道的缓存 buffer 数据
* 输  入  : - uiLayerNo: Vo设备号
*         : - uiVoChn  : Vo通道号
          : - bFlag    : 选择模式
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkClearVoBuf(UINT32 uiVoLayer, UINT32 uiVoChn, UINT32 bFlag)
{
    UINT32 s32Ret = SAL_FAIL;
    RK_BOOL bClrAll = bFlag;

    s32Ret = RK_MPI_VO_ClearChnBuffer(uiVoLayer, uiVoChn, bClrAll);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("error %#x\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_rkDisableChn
* 描  述  : 禁止vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkDisableChn(VOID *prm)
{
    UINT32 uiDev = 0;

    UINT32 uiLayer = 0;

    UINT32 uiChn = 0;

    RK_S32 s32Ret = SAL_SOK;

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

    s32Ret = RK_MPI_VO_DisableChn(uiLayer, uiChn);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("Stop %d Layer chn %d Failed s32Ret 0x%x!!!\n", uiLayer, uiChn, s32Ret);
        return SAL_FAIL;
    }

    stBorder.bBorderEn = 0;
    stBorder.stBorder.u32LeftWidth = 2;
    stBorder.stBorder.u32RightWidth = 2;
    stBorder.stBorder.u32TopWidth = 2;
    stBorder.stBorder.u32BottomWidth = 2;
    s32Ret = RK_MPI_VO_SetChnBorder(uiLayer, uiChn, &stBorder);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("RK_MPI_VO_SetChnBorder error s32Ret 0x%x!! bBorderEn %d u32Color %d LineW %d\n", s32Ret, stBorder.bBorderEn, stBorder.stBorder.u32Color,
                  stBorder.stBorder.u32LeftWidth);
        /* return SAL_FAIL; */
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_rkSetVideoLayerCsc
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
static INT32 disp_rkSetVideoLayerCsc(UINT32 uiVoLayer, void *prm)
{
    INT32 s32Ret = SAL_SOK;
    VO_CSC_S stVoCscSt;
    VIDEO_CSC_S *pstCsc = NULL;


    pstCsc = (VIDEO_CSC_S *)prm;

    if (pstCsc == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    memset(&stVoCscSt, 0, sizeof(VO_CSC_S));

    s32Ret = RK_MPI_VO_GetPostProcessParam(uiVoLayer, &stVoCscSt);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("get video layer[%u] csc fail\n", uiVoLayer);
        return SAL_FAIL;
    }

    stVoCscSt.u32Luma = pstCsc->uiLuma;
    stVoCscSt.u32Contrast = pstCsc->uiContrast;
    stVoCscSt.u32Hue = pstCsc->uiHue;
    stVoCscSt.u32Satuature = pstCsc->uiSatuature;

    s32Ret = RK_MPI_VO_SetPostProcessParam(uiVoLayer, &stVoCscSt);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("RK_MPI_VO_SetVideoLayerCSC Layer %d CSC Failed, %x !!!\n", uiVoLayer, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_rkEnableChn
* 描  述  : 使能vo
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkEnableChn(VOID *prm)
{
    UINT32 uiDev = 0;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;
    VO_BORDER_S stBorder = {0};
    VO_CHN_ATTR_S stChnAttr;
    /* VO_CHN_PARAM_S stChnParam; */
    INT32 s32Ret = SAL_SOK;

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
    stChnAttr.bDeflicker = pDispChn->uiScaleAlgo;

    DISP_LOGI("uiDev %d,VoLayer %d voChn %d x %4d y %4d w %4d h %4d uiScaleAlgo:%d!\n", uiDev, uiLayer, uiChn, stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y,
              stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height, stChnAttr.bDeflicker);

    s32Ret = RK_MPI_VO_SetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SetChnAttr s32Ret 0x%x uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VO_EnableChn(uiLayer, uiChn);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_EnableChn Layer %d  chn %d Failed s32Ret 0x%x !!!\n", uiLayer, uiChn, s32Ret);
        return SAL_FAIL;
    }

#if 0
    /* 通道参数*/
    memset(&stChnParam, 0, sizeof(VO_CHN_PARAM_S));
    stChnParam.stAspectRatio.enMode = ASPECT_RATIO_NONE;
    stChnParam.stAspectRatio.stVideoRect.s32X = pDispChn->uiX;
    stChnParam.stAspectRatio.stVideoRect.s32Y = pDispChn->uiY;
    stChnParam.stAspectRatio.stVideoRect.u32Width = pDispChn->uiW;
    stChnParam.stAspectRatio.stVideoRect.u32Height = pDispChn->uiH;
    stChnParam.stAspectRatio.u32BgColor = 0xefefef;
    s32Ret = RK_MPI_VO_SetChnParam(uiLayer, uiChn, &stChnParam);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_EnableChn Layer %d  chn %d Failed s32Ret 0x%x !!!\n", uiLayer, uiChn, s32Ret);
        return SAL_FAIL;
    }

#endif

    /* 通道边框*/
    stBorder.bBorderEn = pDispChn->bDispBorder;
    stBorder.stBorder.u32Color = pDispChn->BorDerColor;
    stBorder.stBorder.u32LeftWidth = pDispChn->BorDerLineW;
    stBorder.stBorder.u32RightWidth = pDispChn->BorDerLineW;
    stBorder.stBorder.u32TopWidth = pDispChn->BorDerLineW;
    stBorder.stBorder.u32BottomWidth = pDispChn->BorDerLineW;
    if (stBorder.bBorderEn)
    {
        s32Ret = RK_MPI_VO_SetChnBorder(uiLayer, uiChn, &stBorder);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("RK_MPI_VO_SetChnBorder error s32Ret 0x%x!! bBorderEn %d u32Color %d LineW %d\n", s32Ret, stBorder.bBorderEn, stBorder.stBorder.u32Color,
                      stBorder.stBorder.u32LeftWidth);
            /* return SAL_FAIL; */
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_rkSetChnPos
* 描  述  : 设置vo参数（放大镜和小窗口）
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkSetChnPos(VOID *prm)
{
    UINT32 uiDev = 0;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;
    VO_BORDER_S stBorder = {0};

    VO_CHN_ATTR_S stChnAttr;
    INT32 s32Ret = SAL_SOK;

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

    s32Ret = RK_MPI_VO_GetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SetChnAttr s32Ret 0x%x uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }

    stChnAttr.stRect.s32X = SAL_align(pDispChn->uiX, 2);
    stChnAttr.stRect.s32Y = SAL_align(pDispChn->uiY, 2);
    stChnAttr.stRect.u32Width = pDispChn->uiW;
    stChnAttr.stRect.u32Height = pDispChn->uiH;
    stChnAttr.u32Priority = 0;

    DISP_LOGD("uiDev %d,VoLayer %d voChn %d x %4d y %4d w %4d h %4d !\n", uiDev, uiLayer, uiChn, stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y,
              stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);

    s32Ret = RK_MPI_VO_SetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SetChnAttr s32Ret 0x%x uiDev %d,uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiDev, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }

    RK_MPI_VO_GetChnBorder(uiLayer, uiChn, &stBorder);

    if ((stBorder.bBorderEn != pDispChn->bDispBorder)
        || (stBorder.stBorder.u32Color != pDispChn->BorDerColor)
        || (stBorder.stBorder.u32LeftWidth != pDispChn->BorDerLineW))
    {
        stBorder.bBorderEn = pDispChn->bDispBorder;
        stBorder.stBorder.u32Color = pDispChn->BorDerColor;
        stBorder.stBorder.u32LeftWidth = pDispChn->BorDerLineW;
        stBorder.stBorder.u32RightWidth = pDispChn->BorDerLineW;
        stBorder.stBorder.u32TopWidth = pDispChn->BorDerLineW;
        stBorder.stBorder.u32BottomWidth = pDispChn->BorDerLineW;
        s32Ret = RK_MPI_VO_SetChnBorder(uiLayer, uiChn, &stBorder);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("RK_MPI_VO_SetChnBorder error s32Ret 0x%x!! bBorderEn %d u32Color 0x%x LineW %d\n", s32Ret, stBorder.bBorderEn, stBorder.stBorder.u32Color,
                      stBorder.stBorder.u32LeftWidth);
            /* return SAL_FAIL; */
        }
    }

    return SAL_SOK;
}

/*测试代码，暂时未启用*/
#if 0
static void Sample_VO_GetVsyncEvent(RK_VOID *pPrivateData)
{
    /*
       RK_S32 s32Ret = RK_SUCCESS;
       RK_U32 *pSyncType = (RK_U32 *) pPrivateData;
     */
    unsigned int uiResult = 0;
    static UINT32 last = 0;
    struct timeval tv = {0, 0};

    gettimeofday(&tv, NULL);

    DISP_LOGE("Sample_VO_GetVsyncEvent is coming\n");
    uiResult = (unsigned int)((tv.tv_sec * 100000) + tv.tv_usec / 10);
    DISP_LOGE("Sample_VO_RegVsyncCallback  uiResult is %d \n ", uiResult);
    DISP_LOGE("Sample_VO_RegVsyncCallback  last is %d \n ", last);
    last = uiResult - last;
    DISP_LOGE("Sample_VO_RegVsyncCallback time is %d  \n", last);
    last = uiResult;


}

static RK_U32 s32PrivateData = VO_INTF_HDMI;

/**
 * @function    Sample_VO_RegVsyncCallback
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static RK_S32 Sample_VO_RegVsyncCallback(RK_U32 eSyncType, VO_DEV VoDev)
{
    RK_S32 s32Ret = RK_SUCCESS;
    RK_VO_CALLBACK_FUNC_S stCallbackFunc;

    s32PrivateData = eSyncType;
    stCallbackFunc.pfnEventCallback = Sample_VO_GetVsyncEvent;
    stCallbackFunc.pPrivateData = &s32PrivateData;
    s32Ret = RK_MPI_VO_RegVsyncCallbackFunc(VoDev, &stCallbackFunc);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("Sample_VO_RegVsyncCallback failed 0x%x\n", s32Ret);
        return RK_FAILURE;
    }

    DISP_LOGE("Sample_VO_RegVsyncCallback success 0x%x\n", s32Ret);
    return RK_SUCCESS;
}

/**
 * @function    Sample_VO_UnRegVsyncCallback
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static RK_S32 Sample_VO_UnRegVsyncCallback(RK_U32 eSyncType, VO_DEV VoDev)
{
    RK_S32 s32Ret = RK_SUCCESS;
    RK_VO_CALLBACK_FUNC_S stCallbackFunc;

    s32PrivateData = eSyncType;
    stCallbackFunc.pfnEventCallback = Sample_VO_GetVsyncEvent;
    stCallbackFunc.pPrivateData = &s32PrivateData;
    s32Ret = RK_MPI_VO_UnRegVsyncCallbackFunc(VoDev, &stCallbackFunc);
    if (s32Ret != RK_SUCCESS)
    {
        DISP_LOGE("Sample_VO_UnRegVsyncCallback failed 0x%x\n", s32Ret);
        return RK_FAILURE;
    }

    DISP_LOGE("Sample_VO_UnRegVsyncCallback success 0x%x\n", s32Ret);
    return RK_SUCCESS;
}

#endif

/*******************************************************************************
* 函数名  : disp_rkDeleteDev
* 描  述  : 删除显示层
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkDeleteDev(VOID *prm)
{

    VO_DEV VoDev = 0;
    VO_LAYER VoLayer = 0;

    RK_S32 s32Ret = SAL_SOK;

    DISP_DEV_COMMON *pDispDev = NULL;

    pDispDev = (DISP_DEV_COMMON *)prm;
    if (pDispDev == NULL)
    {
        DISP_LOGE("error\n");
        return SAL_FAIL;
    }

    /* VoDev = pDispDev->uiDevNo; */
    VoDev = disp_rkGetDevNo(pDispDev->uiDevNo);
    VoLayer = pDispDev->szLayer.uiLayerNo;

    DISP_LOGI("VoDev = %d VoLayer = %d \n", VoDev, VoLayer);

    s32Ret = RK_MPI_VO_DisableLayer(VoLayer);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_DisableVideoLayer Layer %d failed with 0x%x !\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VO_Disable(VoDev);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_Disable VO %d failed with 0x%x !\n", VoDev, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : Disp_rkCreateLayer
* 描  述  : 创建视频层
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_rkCreateLayer(VOID *prm)
{
    UINT32 uiLayerWith = 0;
    UINT32 uiLayerHeight = 0;
    UINT32 uiLayerFps = 0;
    DISP_LAYER_COMMON *pstLayer = NULL;
    PIXEL_FORMAT_E enPixelFmt;
    VO_LAYER VoLayer = 0;
    RK_S32 s32Ret = SAL_SOK;
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
    /*  stLayerAttr.bClusterMode = RK_FALSE; */
    stLayerAttr.bBypassFrame = RK_FALSE; //TRUE时，图层只有一路数据的时候，图层直通模式，bypass兼容拼接模式，不满足条件可以自动切换到拼接；先置为false，true时测不到从左到右包裹时有竖条的问题
    s32Ret = fmtConvert_rk_sal2rk(pstLayer->enInputSalPixelFmt, &enPixelFmt);
    DISP_CHECK_RET(s32Ret, SAL_SOK, "pixFmtConvert fail");
    stLayerAttr.enPixFormat = enPixelFmt;

    /* HDR模式下不支持SDR数据，SDR模式下支持HDR数据显示 */
    stLayerAttr.enDstDynamicRange = DYNAMIC_RANGE_SDR8;

    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;
    stLayerAttr.stDispRect.u32Width = uiLayerWith;
    stLayerAttr.stDispRect.u32Height = uiLayerHeight;
    stLayerAttr.u32DispFrmRt = uiLayerFps;
    stLayerAttr.enCompressMode = COMPRESS_AFBC_16x16; //需要在/tmp/gpu_debug下抓gpu合成前后的图像时需要改为非压缩模式

    stLayerAttr.stImageSize.u32Width = stLayerAttr.stDispRect.u32Width;
    stLayerAttr.stImageSize.u32Height = stLayerAttr.stDispRect.u32Height;

    DISP_LOGI("Disp VoLayer %d u32Width %d u32Height %d uiDevFps %d !!!\n", VoLayer, uiLayerWith, uiLayerHeight, uiLayerFps);

    s32Ret = RK_MPI_VO_SetLayerDispBufLen(VoLayer, 5); /* 取值[0] [3,15] */
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SetDisplayBufLen VoLayer %d failed with %#x!\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VO_SetLayerAttr(VoLayer, &stLayerAttr);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SetVideoLayerAttr VoLayer %d failed with %#x!\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VO_EnableLayer(VoLayer);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_EnableVideoLayer Layer %d failed with %#x!\n", VoLayer, s32Ret);
        return SAL_FAIL;
    }

    /*Sample_VO_RegVsyncCallback(VO_INTF_HDMI, 0);
       printf("Sample_VO_RegVsyncCallback OK\n");*/
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
        case VO_INTF_HDMI1:
            penSupportIntfSync = aenHdmiSupportRes[voDev];
            break;
        case VO_INTF_MIPI:
        case VO_INTF_MIPI1:
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
* 函数名  : disp_rkCreateDev
* 描  述  : 创建显示
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_rkCreateDev(DISP_DEV_COMMON *pDispDev)
{
    UINT32 eVoDev = 0;
    UINT32 uiDevWith = 0;
    UINT32 uiDevHeight = 0;
    UINT32 uiDevFps = 0;
    UINT64 u64PixelClock = 0;
    VO_DEV VoDev = 0;
    RK_S32 s32Ret = SAL_SOK;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_USER_INTFSYNC_INFO_S stUserInfo = {0};
    VO_INTF_SYNC_E enIntfSync = 0;
    const DISP_TIMING_MAP_S *pstDispTiming = NULL;
    const BT_TIMING_S *pstBtTiming = NULL;
    VO_HDMI_PARAM_S stHdmiParam;
    UINT32 u32Num = 0;
    UINT32 i = 0;
    /* 3个数相乘等于24，第一个数逐渐递增，第二个数大于第三个数 */
    static const UINT32 au32Mul24[][3] = {
        {1, 6, 4},
        {2, 4, 3},
        {3, 4, 2},
        {4, 3, 2},
        {6, 2, 2},
        {8, 3, 1},
        {12, 2, 1},
        {24, 1, 1},
    };


    if ((pDispDev == NULL))
    {
        DISP_LOGE("null pointer\n");
        return SAL_FAIL;
    }

    pstDispTiming = disp_rkGetTiming(pDispDev);
    if (NULL == pstDispTiming)
    {
        DISP_LOGE("disp_rkCreateDev failed!!!\n");
        return SAL_FAIL;
    }

    /* 获取相应时序 */
    pstBtTiming = &(BT_GetTiming(pstDispTiming->enUserTiming)->stTiming);
    enIntfSync = pstDispTiming->enVoTiming;
    /* VoDev = pDispDev->uiDevNo; */
    VoDev = disp_rkGetDevNo(pDispDev->uiDevNo);
    eVoDev = pDispDev->eVoDev;
    uiDevWith = pDispDev->uiDevWith;
    uiDevHeight = pDispDev->uiDevHeight;
    uiDevFps = pDispDev->uiDevFps;

    memset(&stVoPubAttr, 0, sizeof(VO_PUB_ATTR_S));
    memset(&stHdmiParam, 0, sizeof(VO_HDMI_PARAM_S));

    /* HDMI接口属性*/
    stHdmiParam.enHdmiMode = VO_HDMI_MODE_DVI;
    stHdmiParam.enColorFmt = VO_HDMI_COLOR_FORMAT_RGB;
    stHdmiParam.enQuantRange = VO_HDMI_QUANT_RANGE_FULL;

    if (VO_DEV_HDMI == eVoDev)
    {
        stVoPubAttr.enIntfType = VO_INTF_HDMI;
        s32Ret = RK_MPI_VO_SetHdmiParam(VO_INTF_HDMI, 0, &stHdmiParam);
        DISP_CHECK_RET(s32Ret, RK_SUCCESS, "RK_MPI_VO_SetHdmiParam fail");
    }
    else if (VO_DEV_HDMI_1 == eVoDev)
    {
        stVoPubAttr.enIntfType = VO_INTF_HDMI1;
        s32Ret = RK_MPI_VO_SetHdmiParam(VO_INTF_HDMI1, 1, &stHdmiParam);
        DISP_CHECK_RET(s32Ret, RK_SUCCESS, "RK_MPI_VO_SetHdmiParam fail");
    }
    else if (VO_DEV_MIPI == eVoDev)
    {
        stVoPubAttr.enIntfType = VO_INTF_MIPI;
    }
    else if (VO_DEV_MIPI_1 == eVoDev)
    {
        stVoPubAttr.enIntfType = VO_INTF_MIPI1;
    }
    else
    {
        stVoPubAttr.enIntfType = VO_INTF_BT1120;
        disp_rkDevBlanking(uiDevWith, uiDevHeight, &stVoPubAttr);
    }

#ifdef DSP_ISA
    /* rk分析仪默认走user模式 */
    enIntfSync = VO_OUTPUT_USER;

#else
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

#endif
    stVoPubAttr.u32BgColor = pDispDev->uiBgcolor;
    stVoPubAttr.enIntfSync = enIntfSync;

    if (VO_OUTPUT_USER == stVoPubAttr.enIntfSync)
    {
        /* stUserInfo.u32PreDiv = 1; */
        /* hdmi需要分频，mipi不需要分频 */
        if ((VO_DEV_MIPI == pDispDev->eVoDev) || (VO_DEV_MIPI_1 == pDispDev->eVoDev))
        {
            stUserInfo.u32DevDiv = 1;
        }
        else if ((pDispDev->eVoDev == VO_DEV_HDMI) || (pDispDev->eVoDev == VO_DEV_HDMI_1))
        {
            stUserInfo.u32DevDiv = 2;
        }

        stVoPubAttr.stSyncInfo.bSynm = 0;
        stVoPubAttr.stSyncInfo.bIop = 1;                    /* 逐行 */

        stVoPubAttr.stSyncInfo.u16Vact = pstBtTiming->u32Height;
        stVoPubAttr.stSyncInfo.u16Vbb = pstBtTiming->u32VBackPorch;
        stVoPubAttr.stSyncInfo.u16Vfb = pstBtTiming->u32VFrontPorch;
        stVoPubAttr.stSyncInfo.u16Vpw = pstBtTiming->u32VSync;

        stVoPubAttr.stSyncInfo.u16Hact = pstBtTiming->u32Width;
        stVoPubAttr.stSyncInfo.u16Hbb = pstBtTiming->u32HBackPorch;
        stVoPubAttr.stSyncInfo.u16Hfb = pstBtTiming->u32HFrontPorch;
        stVoPubAttr.stSyncInfo.u16Hpw = pstBtTiming->u32HSync;

        stVoPubAttr.stSyncInfo.bIdv = 0;
        stVoPubAttr.stSyncInfo.bIhs = (pstBtTiming->enHSPol == POLARITY_POS) ? RK_TRUE : RK_FALSE;
        stVoPubAttr.stSyncInfo.bIvs = (pstBtTiming->enVSPol == POLARITY_POS) ? RK_TRUE : RK_FALSE;

        stVoPubAttr.stSyncInfo.u16FrameRate = pstBtTiming->u32Fps;
        stVoPubAttr.stSyncInfo.u32PixClock = pstBtTiming->u32PixelClock / 1000;

        stVoPubAttr.stSyncInfo.u16Hmid = 0;
        stVoPubAttr.stSyncInfo.u16Bvact = 0;
        stVoPubAttr.stSyncInfo.u16Bvbb = 0;
        stVoPubAttr.stSyncInfo.u16Bvfb = 0;

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
        u32Num = sizeof(au32Mul24) / sizeof(au32Mul24[0]);

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

        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Refdiv = au32Mul24[i][0];
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv1 = au32Mul24[i][1];
        stUserInfo.stUserIntfSyncAttr.stUserSyncPll.u32Postdiv2 = au32Mul24[i][2];
    }

    DISP_LOGI("VoDev %d, stVoPubAttr.enIntfSync %d, u32BgColor:0x%x.\n", VoDev, stVoPubAttr.enIntfSync, stVoPubAttr.u32BgColor);
    s32Ret = RK_MPI_VO_SetPubAttr(VoDev, &stVoPubAttr);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SetPubAttr VoDev %d failed with %#x!\n", VoDev, s32Ret);
        return SAL_FAIL;
    }

    /* 绑定voDev和图层*/
    s32Ret = RK_MPI_VO_BindLayer((VO_LAYER)pDispDev->szLayer.uiLayerNo, VoDev, VO_LAYER_MODE_VIDEO);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_BindLayer VoDev %d failed with %#x!\n", VoDev, s32Ret);
        return SAL_FAIL;
    }

#ifdef RK_DISP
    if (VO_OUTPUT_USER == stVoPubAttr.enIntfSync)
    {
        s32Ret = RK_MPI_VO_SetUserIntfSyncInfo(VoDev, &stUserInfo);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("[VoDev:%d]Set user interface sync info failed with %x.\n", VoDev, s32Ret);
            return SAL_FAIL;
        }

        RK_MPI_VO_SetDevFrameRate(VoDev, uiDevFps);
    }

#endif

    DISP_LOGI("[VoDev:%d]Set user interface sync info u32DevDiv = %x uiDevFps= %d.\n", VoDev, stUserInfo.u32DevDiv, uiDevFps);

    s32Ret = RK_MPI_VO_Enable(VoDev);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_Enable VoDev %d failed with %#x!\n", VoDev, s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    disp_rkGetHdmiEdid
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static INT32 disp_rkGetHdmiEdid(UINT32 u32HdmiId, UINT8 *pu8Buff, UINT32 *pu32Len)
{
    RK_S32 s32Ret = SAL_SOK;
    VO_SINK_CAPABILITY_S stSinkCap;
    VO_EDID_S stEdid;
    RK_U32 enHdmi = u32HdmiId;

    if (NULL == pu8Buff || pu32Len == NULL)
    {
        EDID_LOGE("hdmi[%d] get edid fail:0x%x\n", enHdmi, s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VO_GetSinkCapability(enHdmi, 0, &stSinkCap);
    if (s32Ret != RK_SUCCESS)
    {
        EDID_LOGE("fail RK_MPI_VO_GetSinkCapability, 0x%x", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_VO_Get_Edid(enHdmi, 0, &stEdid);
    if (SAL_SOK != s32Ret)
    {
        EDID_LOGE("hdmi[%d] get edid fail:0x%x\n", enHdmi, s32Ret);
        return SAL_FAIL;
    }

    if ((RK_TRUE != stEdid.bEdidValid) || (0 == stEdid.u32Edidlength) || (stEdid.u32Edidlength > DISP_EDID_TOTAL_MAX))
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
static INT32 disp_rksetVoChnPriority(VOID *prm)
{
    UINT32 uiDev = 0;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;
    VO_CHN_ATTR_S stChnAttr;
    INT32 s32Ret = SAL_SOK;

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
    s32Ret = RK_MPI_VO_GetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SetChnAttr s32Ret 0x%x uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }

    stChnAttr.u32Priority = pDispChn->u32Priority;
    s32Ret = RK_MPI_VO_SetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SetChnAttr s32Ret 0x%x uiDev %d,uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiDev, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }

    DISP_LOGD("dev %d chn %d u32Priority %d \n", uiLayer, uiChn, stChnAttr.u32Priority);
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_rkenableDispWbc
* 描  述  : 视频层回写使能操作
* 输  入  : VoWbc  *pParam
*         :
* 输  出  :
* 返回值  : RK_SUCCESS  : 成功
*           : 失败
*******************************************************************************/
static INT32 disp_rkenableDispWbc(UINT32 VoWbc, unsigned int *pParam)
{
    VO_WBC_SOURCE_S stWbcSource;
    VO_WBC_ATTR_S stWbcAttr;
    RK_S32 s32Ret = RK_SUCCESS;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;

    stWbcSource.enSourceType = VO_WBC_SOURCE_DEV;
    stWbcSource.u32SourceId = 0;
    if (*pParam == RK_TRUE)
    {
        RK_MPI_VO_SetWbcSource(VoWbc, &stWbcSource);
        /*设置视频回写属性*/
        s32Ret = RK_MPI_VO_GetLayerAttr(VoWbc, &stLayerAttr);
        if (s32Ret != RK_SUCCESS)
        {
            RK_LOGE("[%s] Get Layer Attr failed\n", __func__);
            return RK_FAILURE;
        }

        stWbcAttr.stTargetSize.u32Width = stLayerAttr.stImageSize.u32Width;
        stWbcAttr.stTargetSize.u32Height = stLayerAttr.stImageSize.u32Height;
        stWbcAttr.enPixelFormat = RK_FMT_BGRA8888;
        stWbcAttr.u32FrameRate = 25;
        stWbcAttr.enCompressMode = COMPRESS_MODE_NONE;
        s32Ret = RK_MPI_VO_SetWbcAttr(VoWbc, &stWbcAttr);
        if (s32Ret != RK_SUCCESS)
        {
            DISP_LOGE("RK_MPI_VO_SetWbcAttr erro! \n");
            return s32Ret;
        }

        /*回写使能*/
        wbc_enable_judge = RK_TRUE;
        s32Ret = RK_MPI_VO_EnableWbc(VoWbc);
        if (s32Ret != RK_SUCCESS)
        {
            wbc_enable_judge = RK_FALSE;
            DISP_LOGE("RK_MPI_VO_EnableWbc  erro! \n");
            return s32Ret;
        }
    }
    else
    {
        /*回写失能*/
        wbc_enable_judge = RK_FALSE;
        s32Ret = RK_MPI_VO_DisableWbc(VoWbc);
        if (s32Ret != RK_SUCCESS)
        {
            wbc_enable_judge = RK_TRUE;
            DISP_LOGE("RK_MPI_VO_DisableWbc error");
            return s32Ret;
        }
    }

    return RK_SUCCESS;

}

/*******************************************************************************
* 函数名  : disp_rkgetDispWbc
* 描  述  : 视频层回写操作
* 输  入  : VoWbc
*         :
* 输  出  : pstVFrame
* 返回值  : RK_SUCCESS  : 成功
*           : 失败
*******************************************************************************/
static INT32 disp_rkgetDispWbc(UINT32 VoWbc, VO_FRAME_INFO_ST *pstVFrame)
{
    VIDEO_FRAME_INFO_S stVFrame;
    static char index = 1;
    static ALLOC_VB_INFO_S stAllocVbInfo = {0};
    INT32 width = 0;
    INT32 height = 0;
    RK_S32 s32Ret = RK_SUCCESS;

    if (wbc_enable_judge == RK_TRUE)
    {
        /*抓取回显图像*/
        s32Ret = RK_MPI_VO_GetWbcFrame(VoWbc, &stVFrame, -1);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("RK_MPI_VO_GetWbcFrame error! \n");
            return s32Ret;
        }

        /*保存图像信息，并位图像分配内存*/
        height = stVFrame.stVFrame.u32Height;
        width = stVFrame.stVFrame.u32Width;
        if (index == RK_TRUE)
        {
            index = RK_FALSE;
            /* DISP_LOGI("**************mem_hal_vbAlloc*****************\n"); */
            s32Ret = mem_hal_vbAlloc(height * width * 4, "DISP", "disp_rk", NULL, 1, &stAllocVbInfo);
            if (SAL_SOK != s32Ret)
            {
                DISP_LOGE("mem_hal_vbAlloc error! \n");
                index = RK_TRUE;
                return s32Ret;
            }
        }

        memcpy(stAllocVbInfo.pVirAddr, stVFrame.stVFrame.pVirAddr[0], height * width * 4);
        pstVFrame->pVirAddr = stAllocVbInfo.pVirAddr;
        pstVFrame->height = height;
        pstVFrame->width = width;
        pstVFrame->stride = width * 4;
        pstVFrame->dataFormat = DSP_IMG_DATFMT_BGRA32;

        /*释放抓取帧*/
        s32Ret = RK_MPI_VO_ReleaseWbcFrame(VoWbc, &stVFrame);
        if (s32Ret != RK_SUCCESS)
        {
            DISP_LOGE("RK_MPI_VO_ReleaseWbcFrame error  %d\n", s32Ret);
        }
    }
    else
    {
        DISP_LOGE("********WBC is disable*********\n");
        return SAL_FAIL;
    }

    return RK_SUCCESS;

}


/**
 * @function    disp_rkScalingAlgorithm
 * @brief   设置rk新算法接口
 * @param[in]    prm设置参数
 * @param[out]
 * @return SAL_SOK  : 成功
 *         SAL_FAIL : 失败
 */
static INT32 disp_rkScalingAlgorithm(VOID *prm)
{
    UINT32 uiDev = 0;
    UINT32 uiLayer = 0;
    UINT32 uiChn = 0;
    VO_CHN_ATTR_S stChnAttr;
    INT32 s32Ret = SAL_SOK;

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
    s32Ret = RK_MPI_VO_GetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SetChnAttr s32Ret 0x%x uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }

    stChnAttr.bDeflicker = pDispChn->uiScaleAlgo;

    DISP_LOGI("uiDev %d,VoLayer %d voChn %d x %4d y %4d w %4d h %4d uiScaleAlgo:%d!\n", uiDev, uiLayer, uiChn, stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y,
              stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height, stChnAttr.bDeflicker);

    s32Ret = RK_MPI_VO_SetChnAttr(uiLayer, uiChn, &stChnAttr);
    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("RK_MPI_VO_SetChnAttr s32Ret 0x%x uiVoLayer %d i %d x %d y %d w %d h %d !\n", s32Ret, uiLayer, uiChn,
                  stChnAttr.stRect.s32X, stChnAttr.stRect.s32Y, stChnAttr.stRect.u32Width, stChnAttr.stRect.u32Height);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    disp_rkCommonPrm
 * @brief   显示通用设置接口
 * @param[in]    enType 设置类型
 *               prm设置参数
 * @param[out]
 * @return SAL_SOK  : 成功
 *         SAL_FAIL : 失败
 */
static INT32 disp_rkCommonPrm(DISP_CFG_TYPE_E enType, VOID *prm)
{
    INT32 s32Ret = 0;

    if (enType <= DISP_CFG_NONE || enType >= DISP_CFG_BUTT)
    {
        DUP_LOGE("DISP_CFG_TYPE_E is %d, illgal \n", enType);
        return SAL_FAIL;
    }

    switch(enType)
    {
        case DISP_SCALING_ALGORITHM_CFG:
            s32Ret = disp_rkScalingAlgorithm(prm);
            break;
        default:
            DUP_LOGE("enType is invalid:%d\n", enType);
            return SAL_FAIL;
    }

    return s32Ret;
}


/*******************************************************************************
* 函数名  : disp_halRegister
* 描  述  : 注册rk disp显示函数
* 输  入  : - prm:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
DISP_PLAT_OPS *disp_halRegister(void)
{

    memset(&g_stDispPlatOps, 0, sizeof(DISP_PLAT_OPS));

    g_stDispPlatOps.createVoDev = disp_rkCreateDev;
    g_stDispPlatOps.deleteVoDev = disp_rkDeleteDev;
    g_stDispPlatOps.setVoChnParam = disp_rkSetChnPos;
    g_stDispPlatOps.setVoLayerCsc = disp_rkSetVideoLayerCsc;
    g_stDispPlatOps.enableVoChn = disp_rkEnableChn;
    g_stDispPlatOps.clearVoChnBuf = disp_rkClearVoBuf;
    g_stDispPlatOps.disableVoChn = disp_rkDisableChn;
    g_stDispPlatOps.deinitVoStartingup = disp_rkDeinitStartingup;
    g_stDispPlatOps.putVoFrameInfo = disp_rkPutFrameInfo;
    g_stDispPlatOps.sendVoChnFrame = disp_rkSendFrame;
    g_stDispPlatOps.getVoChnFrame = disp_rkGetVoChnFrame;
    g_stDispPlatOps.setVoInterface = disp_rkSetVoInterface;
    g_stDispPlatOps.getHdmiEdid = disp_rkGetHdmiEdid;
    g_stDispPlatOps.createVoLayer = disp_rkCreateLayer;
    g_stDispPlatOps.setVoChnPriority = disp_rksetVoChnPriority;
    g_stDispPlatOps.getDispWbc = disp_rkgetDispWbc;
    g_stDispPlatOps.enableDispWbc = disp_rkenableDispWbc;
    g_stDispPlatOps.setVoCommonPrm = disp_rkCommonPrm;

    /* 系统起来时先清除一下默认绑定关系 */
    RK_MPI_VO_ClearLayersBinding();
    return &g_stDispPlatOps;
}

