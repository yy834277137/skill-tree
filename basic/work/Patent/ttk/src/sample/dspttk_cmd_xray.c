/**
 * @file    dspttk_cmd_xray.c
 * @brief
 * @note
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include <stdlib.h>
#include <string.h>
#include <sys/prctl.h>
#include <math.h>
#include <dirent.h>  /* 包含文件夹扫描函数的定义 */
#include <sys/stat.h>
#include <fcntl.h>
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_util.h"
#include "dspttk_fop.h"
#include "dspttk_cmd.h"
#include "dspttk_init.h"
#include "dspttk_devcfg.h"
#include "dspttk_cmd_xray.h"
#include "dspttk_cmd_xpack.h"
#include "dspttk_callback.h"
#include "dspttk_xraw_header.h"


/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
#define FILE_PATH_STRLEN_MAX            256             /* 文件路径最大长度，单位：字节 */

#define POS(w, x, y) ((y) * w + (x))            // 计算指定像素点在图像中的位置

/* XRAW文件属性 */
typedef struct
{
    CHAR sFileName[256];        // 文件名，带路径
    XRAY_PROC_TYPE enType;      // 数据类型，仅支持本底、满载和过包数据
    XRAY_ENERGY_NUM enEnergy;   // XRAW能量数
    UINT32 u32Width;
    UINT32 u32Height;
    UINT32 u32Offset;           // 数据在文件中的偏移量
    UINT16 *pu16XRaw;           // XRAW数据内存地址，直接是数据，无文件头
    UINT32 u32Size;             // XRAW数据长度，单位：字节
} XRAW_FILE_ATTR;

/* 模拟采传发送数据结构体 */
typedef struct 
{
    void *pVSyncTimer;               // 模拟VSync触发的定时器
    pthread_t xrayPid;               // 模拟过包线程identifiers
    sem_t semXrayRt;                 // 模拟过包线程信号量
    UINT32 u32XrayRtColNo;           // 模拟发送的条带号
}XRAY_RAW_DATA_INFO;


/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
static XRAY_RAW_DATA_INFO gstRawDataInfo[MAX_XRAY_CHAN] = {0};
static PLAYBACK_SLICE_RANGE gstPbSliceRange[MAX_XRAY_CHAN] = {0};
static XRAY_PACKAGE_INFO gstTransInfo[MAX_XRAY_CHAN] = {0};



PLAYBACK_SLICE_RANGE *dspttk_get_gpbslice_prm(UINT32 u32Chan)
{
    return &gstPbSliceRange[u32Chan];
}


/**
 * @function:   dspttk_xray_simulate_func
 * @brief:      定时器执行函数
 * @param[in]:  INT32 chan  通道
 * @param[out]: None
 * @return:     void
 */
void dspttk_xray_simulate_func(INT32 u32chan)
{
    XRAY_RAW_DATA_INFO *pstRawDataInfo = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    if (u32chan >= pstShareParam->xrayChanCnt)
    {
        PRT_INFO("u32chan is invalid [%d]\n", u32chan);
        return;
    }
    pstRawDataInfo = &gstRawDataInfo[u32chan];

    if (DSPTTK_STATUS_RTPREVIEW == gstGlobalStatus.enXrayPs)
    {
        dspttk_sem_give(&pstRawDataInfo->semXrayRt, __FUNCTION__, __LINE__);
    }

    return;
}


/*****************************************************************************
   函 数 名  : nomalization_to_gray
   功能描述  : 归一化包裹数据转灰度图
   输入参数  :   包裹数据
   输出参数  :  
   返 回 值  : sok fail
   调用函数  :
   被调函数  :
*****************************************************************************/
static int dspttk_nraw_to_gray(float *pfSrcNraw, UINT8 *pu8DstGray, UINT32 u32Width, UINT32 u32Height)
{
    float result = 0;
    float offset = 15;
    UINT32 pixel_pos = 0, u32PixelY = 0;
    float *pfSrcNrawStep  = NULL;
    UINT8 *pu8DstGrayStep = NULL;

    if (pfSrcNraw == NULL || pu8DstGray == NULL || u32Width == 0 || u32Height == 0)
    {
        PRT_INFO("error pfSrcNraw [%p] pu8DstGray [%p]  w [%d] h [%d]\n", pfSrcNraw, pu8DstGray, u32Width, u32Height);
        return SAL_FAIL;
    }

    pfSrcNrawStep = pfSrcNraw;
    pu8DstGrayStep = pu8DstGray;

    for(u32PixelY = 0; u32PixelY < u32Height * 2; u32PixelY++)
    {
        for(pixel_pos = 0; pixel_pos < u32Width; pixel_pos++)
        {

            result = *pfSrcNrawStep * 255 + offset;                    /*归一化的值转灰度图  offset提高图像亮度*/
            if(result > 200)
            {
                *pu8DstGrayStep = 255;
            }
            else
            {
                *pu8DstGrayStep = ((unsigned char)result);
            }
            pu8DstGrayStep++;
            pfSrcNrawStep++;
        }
    }

    return SAL_SOK;
    
}

/**
 * @fn      dspttk_trans_xraw_2_nraw
 * @brief   生成将本地满载过包数据转换成归一化nraw数据
 *
 * @param   [IN] u32Chan 通道号
 * @param   [IN] u32Width u32Height      过包数据宽高
 * @param   [IN] UINT16 *pu16Full, char *pu16Zero, char *pu16Xraw 本地满载过包数据
 * @param   [OUT] UINT16 *pfNrawData  归一化输出的数据地址
 *
 * @return  SAL_STATUS   成功 SAL_SOK 失败 SAL_FAIL
 */
SAL_STATUS dspttk_trans_xraw_2_nraw(UINT32 u32Chan, \
                                                UINT32 u32Width, UINT32 u32Height, \
                                                UINT16 *pu16Full, UINT16 *pu16Zero, \
                                                UINT16 *pu16Xraw, float *pfNrawDst)
{
    UINT32 u32PixelPos = 0, u32PixelY = 0;
    UINT16 *pu16FullStart = NULL, *pu16ZeroStep = NULL, *pu16NrawSrcStep = NULL;
    float *pfNrawDstStep = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    DSPTTK_CHECK_CHAN(u32Chan, pstShareParam->xrayChanCnt, SAL_FAIL);

    if (pu16Zero == NULL || pu16Full == NULL || pu16Xraw == NULL || u32Width == 0 || u32Height == 0)
    {
        PRT_INFO("error zero [%p] full [%p] xscan [%p] w [%d] h [%d]\n", pu16Zero, pu16Full, pu16Xraw, u32Width, u32Height);
        return SAL_FAIL;
    }

    pfNrawDstStep = pfNrawDst;

    pu16NrawSrcStep = pu16Xraw;
    for (u32PixelY = 0; u32PixelY < u32Height * 2; u32PixelY++) /* 遍历每一列 */
    {
        pu16FullStart = pu16Full;
        pu16ZeroStep = pu16Zero;
        for (u32PixelPos = 0; u32PixelPos < u32Width; u32PixelPos++) /* 遍历每一行 */
        {
            if(u32PixelPos > 5 && (0xFFFF == *pu16NrawSrcStep || 0xFFFF == *pu16FullStart))        /*计算归一化的值*/
            {
                *pfNrawDstStep = ((float)(*(pu16NrawSrcStep - 5) - *(pu16ZeroStep-5))) / ((float)(*(pu16FullStart - 5) - *(pu16ZeroStep - 5)));
            }
            else
            {
                *pfNrawDstStep = ((float)(*pu16NrawSrcStep-*pu16ZeroStep)) / ((float)(*pu16FullStart-*pu16ZeroStep));

            }
            if(*pfNrawDstStep < 0)
            {
                *pfNrawDstStep = 0;
            }

            pfNrawDstStep++;
            pu16NrawSrcStep++;
            pu16ZeroStep++;
            pu16FullStart++;
        }
    }


    return SAL_SOK;
}


#if 0
/**
 * @fn      dspttk_make_liner_packageSign
 * @brief   生成预览条带信息头中的包裹标记
 *
 * @param   [IN] u32Chan 通道号
 * @param   [IN] 含隐含入参 XRAY_NRAW_INFO 本地满载过包数据
 * @param   [OUT] 含隐含输出  XRAY_NRAW_INFO 归一化输出数据
 *
 * @return  SAL_STATUS   成功 SAL_SOK 失败 SAL_FAIL
 */
SAL_STATUS dspttk_make_liner_packageSign(UINT32 u32Chan)
{
    SAL_STATUS s32Ret = SAL_SOK;
    UINT32 u32PixelPos = 0, i = 0, u32LinerPixelSum = 0, u32AveragePixel = 0, u32LinerLens = 0;
    UINT16 *pu16GrayStep = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    DSPTTK_CHECK_CHAN(u32Chan, pstShareParam->xrayChanCnt, SAL_FAIL);

    XRAY_RAW_DATA_INFO *pstRawDataInfo = &gstRawDataInfo[u32Chan];
    XRAY_PREVIEW *pstXrayPreviewHeader = &pstRawDataInfo->stXrayElementHeader.xrayProcParam.stXrayPreview;
    pu16GrayStep = (UINT16 *)pstRawDataInfo->stNrawInfo.pu8GrayData;
    u32LinerLens = pstRawDataInfo->stNrawInfo.u32NrawWidth;
    /* 重置为0 */
    for(int j = 0; j < 8; j++)
    {
        pstXrayPreviewHeader->packageSign[j] = 0;
    }
    /* 通过灰度值判断该行是否含包裹 */
    for ( i = 0; i < pstRawDataInfo->stNrawInfo.u32NrawHeight; i++)
    {
        for(u32PixelPos = 0; u32PixelPos < u32LinerLens; u32PixelPos++)
        {
            if(u32PixelPos < 5 || u32PixelPos > SAL_SUB_SAFE(u32LinerLens, 5))
            {
                *pu16GrayStep = 0XFFFF;
            }
            u32LinerPixelSum += *pu16GrayStep;
            pu16GrayStep++;
        }
        u32AveragePixel = SAL_DIV_SAFE(u32LinerPixelSum, u32LinerLens);

       /* 大于63000将该行行标记为空白反之为包裹 */
        if (u32AveragePixel < 0xF618)
        {
            pstXrayPreviewHeader->packageSign[i / 32] |= (1u << i % 32);
        }
        else
        {
            pstXrayPreviewHeader->packageSign[i / 32] &= ~(1u << i % 32);
        }
        u32LinerPixelSum = 0; /* 每一行重新计算像素总和 */
    }

    for (i = 0; i < pstRawDataInfo->stNrawInfo.u32NrawHeight / 32; i++)
    {
        if(pstXrayPreviewHeader->packageSign[i] == 0)
        {
            pstRawDataInfo->stNrawInfo.enSliceCont = XSP_SLICE_BLANK;
        }
        else
        {
            pstRawDataInfo->stNrawInfo.enSliceCont = XSP_SLICE_PACKAGE;
        }
    }
    if (pstRawDataInfo->stNrawInfo.enSliceCont == XSP_SLICE_PACKAGE && 
        pstRawDataInfo->stNrawInfo.enSliceTagBefor == XSP_SLICE_BLANK && pstCfgXray->stTipSet.stTipData.uiEnable) /* 记为包裹的开始 */
    {
        if (dspttk_sem_give(&pstCfgXray->stTipSet.semSetTip[u32Chan], __FUNCTION__, __LINE__) == SAL_FAIL)
        {
            PRT_INFO("dspttk_xray_simulate_func failed %p\n", &pstCfgXray->stTipSet.semSetTip[u32Chan]);
            return SAL_FAIL;
        }
    }

    pstRawDataInfo->stNrawInfo.enSliceTagBefor = pstRawDataInfo->stNrawInfo.enSliceCont;
    #if 0 /* 将硬件分割的包裹标记全部置为1 */
    pstXrayPreviewHeader->packageSign[0] = 0xFFFFFFFF;
    pstXrayPreviewHeader->packageSign[1] = 0xFFFFFFFF;
    pstXrayPreviewHeader->packageSign[2] = 0xFFFFFFFF;
    pstXrayPreviewHeader->packageSign[3] = 0xFFFFFFFF;
    #endif

    return s32Ret;
}


/**
 * @fn      dspttk_make_xray_hw_divided_header
 * @brief   生成硬件分割时的条带信息头
 *
 * @param   [IN] u32Chan 通道号
 * @param   [IN] 含隐含入参 XRAY_PREVIEW stXrayPreview
 *
 * @return  SAL_STATUS   成功 SAL_SOK 失败 SAL_FAIL
 */
SAL_STATUS dspttk_make_xray_hw_divided_header(UINT32 u32Chan)
{
    SAL_STATUS sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XRAY_RAW_DATA_INFO *pstRawDataInfo = &gstRawDataInfo[u32Chan];
    XRAY_NRAW_INFO *pstNrawInfo = &pstRawDataInfo->stNrawInfo;

    DSPTTK_CHECK_CHAN(u32Chan, pstShareParam->xrayChanCnt, SAL_FAIL);

    /* 将原始raw数据转化为归一化nraw数据 */
    sRet = dspttk_trans_xraw_2_nraw(u32Chan, pstNrawInfo->u32NrawWidth, pstNrawInfo->u32NrawHeight,\
                                           pstNrawInfo->pu16FullLinerData, pstNrawInfo->pu16ZeroLinerData,\
                                           pstNrawInfo->pu16XrawData, pstNrawInfo->pfNrawData);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("trans xraw 2 nraw failed. chan[%d] Nw[%d] Nh[%d] full[%p] zero[%p] xraw[%p] nraw[%p].\n",\
                                            u32Chan, pstNrawInfo->u32NrawWidth, pstNrawInfo->u32NrawHeight,\
                                            pstNrawInfo->pu16FullLinerData, pstNrawInfo->pu16ZeroLinerData,\
                                            pstNrawInfo->pu16XrawData, pstNrawInfo->pfNrawData);
    }
    sRet = dspttk_nraw_to_gray(pstNrawInfo->pfNrawData, pstNrawInfo->pu8GrayData, pstNrawInfo->u32NrawWidth, pstNrawInfo->u32NrawHeight);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("trans xraw 2 nraw failed. chan[%d].\n",u32Chan);
    }

    /* 标记每一行的包裹信息 */
    sRet = dspttk_make_liner_packageSign(u32Chan);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("make liner package sign failed. ret[%d] chan[%d]\n", sRet, u32Chan);
        return SAL_FAIL;
    }

    return SAL_SOK;
}
#endif


/*****************************************************************************
   函 数 名  : caculate_average_data
   功能描述  : 计算校正数据平均值
   输入参数  :   raw文件信息 
   输出参数  :  
   返 回 值  : sok fail
   调用函数  :
   被调函数  :
*****************************************************************************/
SAL_STATUS dspttk_calculate_average_data(UINT16 *u16AverDst, UINT16 *u16Src, UINT32 u32Width, UINT32 u32Height)
{
    UINT32 u32HeightIdx = 0;
    UINT32 u32PixelsPos = 0;
    UINT16 *pu16CurrData = NULL;
    UINT16 *pu16AverageData = NULL;

    if (u32Width == 0 || u32Height == 0 || u16AverDst == NULL || u16Src == NULL)
    {
        PRT_INFO("w[%d] h[%d] u16AverDst[%p] u16Src[%p]\n", u32Width, u32Height, u16AverDst, u16Src);
        return SAL_FAIL;
    }

    pu16CurrData = u16Src;
    pu16AverageData = u16AverDst;
    for(u32HeightIdx = 0; u32HeightIdx < u32Height; u32HeightIdx++)
    {
        pu16AverageData = u16AverDst;
        for(u32PixelsPos = 0; u32PixelsPos < u32Width; u32PixelsPos++)
        {
            if(u32HeightIdx == 0)
            {
                *pu16AverageData = *pu16CurrData;
            }
            
            if(*pu16CurrData > *pu16AverageData)
            {
                *pu16AverageData = *pu16AverageData + (*pu16CurrData - *pu16AverageData) / 2;         /*计算每一行相加的平均值*/
            }
            else
            {
                *pu16AverageData = *pu16CurrData + (*pu16AverageData - *pu16CurrData) / 2;
            }
            pu16CurrData++;
            pu16AverageData++;

        }
    }

    return SAL_SOK;
}


#if 0
/**
 * @fn      dspttk_xray_make_rt_header
 * @brief   生成信息头
 *
 * @param   [IN] u32Chan   通道号
 * @param   [IN] u32Width   宽度
 * @param   [IN] u32Height 高度
 *
 * @return  SAL_STATUS   成功 SAL_SOK 失败 SAL_FAIL
 */
SAL_STATUS dspttk_xray_make_rt_header(UINT32 u32Chan, UINT32 u32Width, UINT32 u32Height)
{
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    DSPTTK_SETTING_SVA *pstCfgSva = &pstDevCfg->stSettingParam.stSva;
    DSPTTK_CHECK_CHAN(u32Chan, pstShareParam->xrayChanCnt, SAL_FAIL);

    if (u32Width == 0 || u32Height == 0)
    {
        PRT_INFO("w[%d] h[%d] \n", u32Width, u32Height);
        return SAL_FAIL;
    }

    XRAY_RAW_DATA_INFO *pstRawDataInfo = &gstRawDataInfo[u32Chan];
    XRAY_NRAW_INFO *pstNrawInfo = &pstRawDataInfo->stNrawInfo;
    XRAY_ELEMENT *pstXrayEle = &pstRawDataInfo->stXrayElementHeader;

    pstXrayEle->chan = u32Chan;
    pstXrayEle->magic = XRAY_ELEMENT_MAGIC;
    pstXrayEle->time = dspttk_get_clock_ms();
    pstXrayEle->width = u32Width;
    pstXrayEle->height = u32Height;
    pstXrayEle->dataLen = u32Width * u32Height * RAW_BYTE(pstShareParam->dspCapbPar.xray_energy_num);
    /* 做其他头信息 */
    pstXrayEle->xrayProcParam.stXrayPreview.direction = pstCfgXray->enDispDir;
    if (pstXrayEle->type == XRAY_TYPE_NORMAL || pstXrayEle->type == XRAY_TYPE_PSEUDO_BLANK)
    {
        pstXrayEle->xrayProcParam.stXrayPreview.columnNo = ++pstRawDataInfo->u32XrayRtColNo;
    }
    
    pstXrayEle->xrayProcParam.stXrayPreview.speed = pstCfgXray->stSpeedParam.enSpeedUsr;
    /* 硬件分割模拟采传的包裹标记 */
    if (pstNrawInfo->pu16XrawData == NULL)
    {
        PRT_INFO("Nraw src ptr is %p\n", pstNrawInfo->pu16XrawData);
        return SAL_FAIL;
    }

    if (pstNrawInfo->pu16FullLinerData == NULL)
    {
        pstNrawInfo->pu16FullLinerData = (UINT16 *)malloc(pstNrawInfo->u32NrawWidth * sizeof(UINT16));
    }
    if (pstNrawInfo->pu16ZeroLinerData == NULL)
    {
        pstNrawInfo->pu16ZeroLinerData = (UINT16 *)malloc(pstNrawInfo->u32NrawWidth * sizeof(UINT16));
    }
    if (pstCfgXray->stPackagePrm.enDivMethod == XSP_PACK_DIVIDE_HW_SIGNAL || !!pstCfgSva->enSvaPackConse || !!pstCfgXray->stTipSet.stTipData.uiEnable)
    {
        if (pstNrawInfo->pu8GrayData == NULL)
        {
            pstNrawInfo->pu8GrayData = (UINT8 *)malloc(pstNrawInfo->u32NrawWidth * pstNrawInfo->u32NrawHeight * RAW_BYTE(pstShareParam->dspCapbPar.xray_energy_num));
        }
        if (pstNrawInfo->pfNrawData == NULL)
        {
            pstNrawInfo->pfNrawData = (float *)malloc(pstNrawInfo->u32NrawWidth * pstNrawInfo->u32NrawWidth * \
                                                            RAW_BYTE(pstShareParam->dspCapbPar.xray_energy_num));  /* 此处的归一化数据只有高低能 */
        }
    }

    switch (pstXrayEle->type)
    {
    case XRAY_TYPE_NORMAL:
        if (pstCfgXray->stPackagePrm.enDivMethod == XSP_PACK_DIVIDE_HW_SIGNAL || !!pstCfgSva->enSvaPackConse || !!pstCfgXray->stTipSet.stTipData.uiEnable) /* 硬件分割会做包裹信息头 或 开启sva连续连续包裹测试会做识别 */
        {
            if (dspttk_make_xray_hw_divided_header(u32Chan) != SAL_SOK)
            {
                PRT_INFO("dspttk_make_xray_hw_divided_header failed col[%d].\n", pstXrayEle->xrayProcParam.stXrayPreview.columnNo);
            }
        }
        break;
    case XRAY_TYPE_PSEUDO_BLANK:    
        break;
    case XRAY_TYPE_CORREC_FULL:
        dspttk_calculate_average_data(pstNrawInfo->pu16FullLinerData, pstNrawInfo->pu16XrawData, \
                                                pstNrawInfo->u32NrawWidth, pstNrawInfo->u32NrawHeight);
        break;
    case XRAY_TYPE_CORREC_ZERO:
        dspttk_calculate_average_data(pstNrawInfo->pu16ZeroLinerData, pstNrawInfo->pu16XrawData, \
                                                pstNrawInfo->u32NrawWidth, pstNrawInfo->u32NrawHeight);
        break;
    default:
        break;
    }


    if (XRAY_DIRECTION_RIGHT == pstCfgXray->enDispDir)
    {
        gstGlobalStatus.stRtSliceStatus[u32Chan].u64SliceNoL2R = pstXrayEle->xrayProcParam.stXrayPreview.columnNo;
    }
    else
    {
        gstGlobalStatus.stRtSliceStatus[u32Chan].u64SliceNoR2L = pstXrayEle->xrayProcParam.stXrayPreview.columnNo;
    }

    return SAL_SOK;
}
#endif


SAL_STATUS dspttk_xraw_reconstruct(UINT16 *p16DstXraw, UINT32 u32DstW, UINT32 u32DstEnergy, 
                                   UINT16 *p16SrcXraw, UINT32 u32SrcW, UINT32 u32SrcEnergy, 
                                   UINT32 u32Height, XRAY_PROC_TYPE *enType, UINT32 u32SliceNo)
{
    UINT32 i = 0, j = 0, k = 0, m = 0;
    UINT32 u32PixelNumL = 0, u32PixelNumR = 0;
    UINT32 u32PixelFilled = 0;
    UINT32 u32PixelPos[u32DstW]; // 线性插值左像素的位置，右像素+1
    UINT32 u32Weight[u32DstW]; // 线性插值右像素的权重，左像素u32DstW - u32Weight
    UINT16 *pu16SrcLine = p16SrcXraw, *pu16DstLine = p16DstXraw;

    if (NULL == p16DstXraw || NULL == p16SrcXraw || NULL == enType)
    {
        PRT_INFO("the p16DstXraw(%p) OR p16SrcXraw(%p) OR enType(%p) is NULL\n", p16DstXraw, p16SrcXraw, enType);
        return SAL_FAIL;
    }
    if (u32DstEnergy == 0 || u32SrcEnergy == 0 || u32DstEnergy > u32SrcEnergy) // 不允许单能变双能
    {
        PRT_INFO("the DstEnergy(%u) OR SrcEnergy(%u) is invalid\n", u32DstEnergy, u32SrcEnergy);
        return SAL_FAIL;
    }

    // 先判定过包数据是否为填充的伪空白数据
    if (XRAY_TYPE_NORMAL == *enType)
    {
        for (i = 0; i < u32Height * u32SrcEnergy; i++)
        {
            pu16SrcLine = p16SrcXraw + u32SrcW * i;
            for (j = 0; j < u32SrcW; j++)
            {
                if (0xFFFF == pu16SrcLine[j])
                {
                    k++;
                }
                else
                {
                    break;
                }
            }

            if (k == u32SrcW) // 一整行全是0xFFFF，说明有填充的伪空白数据，则视该段数据XRAY_TYPE_PSEUDO_BLANK类型
            {
                *enType = XRAY_TYPE_PSEUDO_BLANK;
                return SAL_SOK;
            }
            else
            {
                k = 0;
            }
        }
    }

    if (u32DstW > u32SrcW) // 填充
    {
        // 本底像素值填充0，满载和过包像素值填充0xFFFF
        u32PixelFilled = (XRAY_TYPE_CORREC_ZERO == *enType) ? 0 : 0xFFFF;
        for (i = 0; i < u32Height; i++)
        {
            u32PixelNumL = ((u32DstW - u32SrcW) / 64 / 2) * 64;
            u32PixelNumR = u32DstW - u32SrcW - u32PixelNumL;
            for (m = 0; m < u32DstEnergy; m++)
            {
                pu16SrcLine = p16SrcXraw + u32SrcW * i * u32SrcEnergy + u32SrcW * m;
                pu16DstLine = p16DstXraw + u32DstW * i * u32DstEnergy + u32DstW * m;
                for (k = 0; k < u32PixelNumL; k++) // 左边u32PixelNumL个像素填充u32PixelFilled
                {
                    pu16DstLine[k] = u32PixelFilled;
                }
                for (j = 0, k = u32PixelNumL; j < u32SrcW; j++, k++) // 中间u32SrcW个像素复制
                {
                    pu16DstLine[k] = pu16SrcLine[j];
                }
                for (j = 0, k = u32PixelNumL + u32SrcW; j < u32PixelNumR; j++, k++) // 右边u32PixelNumR个像素填充u32PixelFilled
                {
                    pu16DstLine[k] = u32PixelFilled;
                }
            }
        }
    }
    else if (u32DstW < u32SrcW) // 缩放
    {
        for (j = 0; j < u32DstW; j++)
        {
            u32PixelPos[j] = u32SrcW * j / u32DstW;
            u32Weight[j] = u32SrcW * j % u32DstW;
        }
        if (u32PixelPos[u32DstW-1] == u32SrcW - 1)
        {
            u32PixelPos[u32DstW-1] = u32SrcW - 2;
            u32Weight[u32DstW-1] = u32DstW;
        }

        for (i = 0; i < u32Height; i++)
        {
            for (m = 0; m < u32DstEnergy; m++)
            {
                pu16SrcLine = p16SrcXraw + u32SrcW * i * u32SrcEnergy + u32SrcW * m;
                pu16DstLine = p16DstXraw + u32DstW * i * u32DstEnergy + u32DstW * m;
                for (j = 0; j < u32DstW; j++)
                {
                    pu16DstLine[j] = ((u32DstW - u32Weight[j]) * pu16SrcLine[u32PixelPos[j]] + u32Weight[j] * pu16SrcLine[u32PixelPos[j]+1]) / u32DstW;
                }
            }
        }
    }
    else // 直接复制
    {
        for (i = 0; i < u32Height; i++)
        {
            for (m = 0; m < u32DstEnergy; m++)
            {
                pu16SrcLine = p16SrcXraw + u32SrcW * i * u32SrcEnergy + u32SrcW * m;
                pu16DstLine = p16DstXraw + u32DstW * i * u32DstEnergy + u32DstW * m;
                memcpy(pu16DstLine, pu16SrcLine, u32SrcW * sizeof(UINT16));
            }
        }
    }

    return SAL_SOK;
}


/**
 * @fn:   dspttk_xray_write_rt_share_buf
 * @brief:  将本底满载数据写入实时过包的share buf
 * 
 * @param[IN]:      UINT32 u32Chan                      通道号
 * @param[IN/OUT]:  UINT8 *pu8RrdRawBuf                输入buf用于文件读取
 * @param[IN]:      XRAY_PROC_TYPE enRawType            输入数据类型
 * @param[IN]:      含隐含入参  pstRawDataPath            存储源数据的路径
 * @param[IN]:      含隐含入参  pstXrayShare              共享缓冲区用于数据交换
 * @param[IN]:      含隐含入参  pstDevCfg                 存储配置文件
 * 
 * @param[out]: None
 * @return:     SAL_STATUS   成功 SAL_SOK 失败 SAL_FAIL
 */
SAL_STATUS dspttk_xray_write_rt_share_buf(UINT32 u32Chan, XRAW_FILE_ATTR *pstXRawAttr)
{
    INT32 s32Ret = SAL_SOK;
    XRAY_SPEED_PARAM_ST *pstXraySpeed = {0};
    UINT32 u32ElemSize = 0; // 写入share buffer一帧的数据量，包括XRAY_ELEMENT的长度
    UINT32 u32ShBufResLen = 0; // 从writeIdx开始到buffer底部的剩余长度
    UINT32 u32ElemW = 0, u32ElemH = 0, u32OffsetH = 0;
    UINT8 *pu8DstBuf = NULL;
    XRAY_RAW_DATA_INFO *pstRawDataInfo = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XRAY_SHARE_BUF *pstShareBufRt = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    XRAY_ELEMENT *pstXrayElem = NULL;
    XRAY_PROC_TYPE enProcType = XRAY_TYPE_NORMAL;

    DSPTTK_CHECK_CHAN(u32Chan, pstShareParam->xrayChanCnt, SAL_FAIL);
    pstShareBufRt = &pstShareParam->xrayShareBuf[u32Chan];
    pstRawDataInfo = &gstRawDataInfo[u32Chan];

    if (XRAY_TYPE_NORMAL == pstXRawAttr->enType) // 实时过包，每个包裹文件时，清空信号量
    {
        while (dspttk_sem_get_value(&pstRawDataInfo->semXrayRt) > 0)
        {
            dspttk_sem_take(&pstRawDataInfo->semXrayRt, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }
    }

    PRT_INFO("fetch xraw file %s, type %d, energy %d, width %u, height %u, offset %u, size %u\n", pstXRawAttr->sFileName, pstXRawAttr->enType, 
             pstXRawAttr->enEnergy, pstXRawAttr->u32Width, pstXRawAttr->u32Height, pstXRawAttr->u32Offset, pstXRawAttr->u32Size);
    while (u32OffsetH < pstXRawAttr->u32Height)
    {
        if (XRAY_TYPE_NORMAL == pstXRawAttr->enType)
        {
            dspttk_sem_take(&pstRawDataInfo->semXrayRt, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

            pstXraySpeed = &pstShareParam->dspCapbPar.xray_speed[pstCfgXray->stSpeedParam.enFormUsr][pstCfgXray->stSpeedParam.enSpeedUsr];
            if (pstXraySpeed->slice_height > 0 && pstXraySpeed->integral_time > 10)
            {
                if (u32OffsetH + pstXraySpeed->slice_height <= pstXRawAttr->u32Height) // 发送一个条带
                {
                    u32ElemH = pstXraySpeed->slice_height;
                }
                else // 数据量不足，直接跳出
                {
                    break;
                }
            }
            else // 速度参数异常，重复尝试
            {
                dspttk_msleep(1000);
                continue;
            }
        }
        else if (XRAY_TYPE_CORREC_FULL == pstXRawAttr->enType || XRAY_TYPE_CORREC_ZERO == pstXRawAttr->enType || 
                 XRAY_TYPE_AUTO_CORR_FULL == pstXRawAttr->enType)
        {
            u32ElemH = pstXRawAttr->u32Height; // 一次性发送所有的校正数据
        }

        u32ElemW = pstShareParam->dspCapbPar.xray_width_max;
        u32ElemSize = DSPTTK_XRAW_SIZE(u32ElemW, u32ElemH, pstShareParam->dspCapbPar.xray_energy_num) + sizeof(XRAY_ELEMENT);
        u32ShBufResLen = pstShareBufRt->bufLen - pstShareBufRt->writeIdx;
        if (u32ElemSize > u32ShBufResLen) // 无法一次性写入，申请临时内存
        {
            pu8DstBuf = (UINT8 *)malloc(u32ElemSize);
            if (NULL == pu8DstBuf)
            {
                PRT_INFO("malloc for 'pu8DstBuf' failed, buffer size: %u\n", u32ElemSize);
                break;
            }
        }
        else
        {
            pu8DstBuf = (UINT8 *)pstShareBufRt->pVirtAddr + pstShareBufRt->writeIdx;
        }

        enProcType = pstXRawAttr->enType;
        s32Ret = dspttk_xraw_reconstruct((UINT16 *)(pu8DstBuf + sizeof(XRAY_ELEMENT)), u32ElemW, pstShareParam->dspCapbPar.xray_energy_num,
                     pstXRawAttr->pu16XRaw+(pstXRawAttr->u32Width*u32OffsetH*pstXRawAttr->enEnergy), pstXRawAttr->u32Width, pstXRawAttr->enEnergy,
                     u32ElemH, &enProcType, pstRawDataInfo->u32XrayRtColNo+1);
        if (SAL_SOK != s32Ret)
        {
            PRT_INFO("xray chan %u, dspttk_xraw_reconstruct failed\n", u32Chan);
            if (u32ElemSize > u32ShBufResLen)
            {
                free(pu8DstBuf);
            }
            break;
        }

        pstXrayElem = (XRAY_ELEMENT *)pu8DstBuf;
        pstXrayElem->magic = XRAY_ELEMENT_MAGIC;
        pstXrayElem->chan = u32Chan;
        pstXrayElem->type = enProcType;
        pstXrayElem->time = dspttk_get_clock_ms();
        pstXrayElem->width = u32ElemW;
        pstXrayElem->height = u32ElemH;
        pstXrayElem->dataLen = DSPTTK_XRAW_SIZE(u32ElemW, u32ElemH, pstShareParam->dspCapbPar.xray_energy_num);

        memset(&pstXrayElem->xrayProcParam.stXrayPreview, 0, sizeof(XRAY_PREVIEW));
        pstXrayElem->xrayProcParam.stXrayPreview.direction = pstCfgXray->enDispDir;
        pstXrayElem->xrayProcParam.stXrayPreview.columnNo = ++pstRawDataInfo->u32XrayRtColNo;
        pstXrayElem->xrayProcParam.stXrayPreview.speed = pstCfgXray->stSpeedParam.enSpeedUsr;
        if (XRAY_DIRECTION_RIGHT == pstCfgXray->enDispDir) // TODO: 这里是什么用意
        {
            gstGlobalStatus.stRtSliceStatus[u32Chan].u64SliceNoL2R = pstXrayElem->xrayProcParam.stXrayPreview.columnNo;
        }
        else
        {
            gstGlobalStatus.stRtSliceStatus[u32Chan].u64SliceNoR2L = pstXrayElem->xrayProcParam.stXrayPreview.columnNo;
        }

        if (u32ElemSize > u32ShBufResLen) // 有申请临时内存，将临时内存数据放入share buffer中，并释放
        {
            memcpy(pstShareBufRt->pVirtAddr + pstShareBufRt->writeIdx, pu8DstBuf, u32ShBufResLen);
            memcpy(pstShareBufRt->pVirtAddr, pu8DstBuf + u32ShBufResLen, u32ElemSize - u32ShBufResLen);
            free(pu8DstBuf);
        }
        pstShareBufRt->writeIdx = (pstShareBufRt->writeIdx + u32ElemSize) % pstShareBufRt->bufLen; // 更新share buffer的写索引
        u32OffsetH += u32ElemH;
    }

    return SAL_SOK;
}



static INT32 dspttk_xray_parse_xraw_with_hdr(CHAR *pXRawPath, XRAW_FILE_ATTR *pastXRawAttr[])
{
    UINT32 i = 0, j = 0, k = 0;
    UINT32 u32ReadSize = 0, u32Width = 0, u32Height = 0, u32Offset = 0;
    const UINT32 u32ZeroFullXRawCnt = 2;
    long fileSize = 0, idx = -1;
    VOID *pXRawBuffer = NULL;
    PAL_XRAW_HEADER_V0 *pstHeaderV0 = NULL;
    PAL_XRAW_HEADER_V1 *pstHeaderV1 = NULL;
    PAL_XRAW_DT enXRawDt = PAL_XRAW_DT_UNDEF;
    PAL_XRAY_ENERGY_NUM enXRayEnergy = PAL_XRAY_ENERGY_UNDEF;
    UINT8 au8PartingLine[] = {0x52, 0x41, 0x59, 0x49, 0x4E}; // RAYIN
    UINT16 *pu16XRawLine = NULL, *pu16CorrParting = NULL;
    XRAW_FILE_ATTR *pstXRawAttr = pastXRawAttr[0];

    if (NULL == pXRawPath || strlen(pXRawPath) == 0)
    {
        return -1;
    }
    if (NULL == pastXRawAttr)
    {
        return -1;
    }

    fileSize = dspttk_get_file_size(pXRawPath);
    if (fileSize <= 0)
    {
        return -1;
    }

    u32ReadSize = SAL_MAX(sizeof(PAL_XRAW_HEADER_V0), sizeof(PAL_XRAW_HEADER_V1));
    pXRawBuffer = malloc(u32ReadSize);
    if (NULL == pXRawBuffer)
    {
        PRT_INFO("malloc for 'pXRawBuffer' failed, buffer size %u\n", u32ReadSize);
        return -1;
    }

    if (SAL_SOK == dspttk_read_file(pXRawPath, 0, pXRawBuffer, &u32ReadSize))
    {
        if (u32ReadSize >= sizeof(PAL_XRAW_HEADER_V0))
        {
            j = SAL_offsetof(PAL_XRAW_HEADER_V0, stDevInfo); // stDevInfo前的数据均为0，则判定为V0版本
            for (i = 0; i < j; i++)
            {
                if (*((UINT8 *)pXRawBuffer + i) != 0)
                {
                    break;
                }
            }

            if (i == j) // 遍历结束，stDevInfo前的数据均为0，确定为V0版本
            {
                pstHeaderV0 = (PAL_XRAW_HEADER_V0 *)pXRawBuffer;
                enXRawDt = pstHeaderV0->stXrawData.enDataType;
                enXRayEnergy = pstHeaderV0->stDevInfo.stXsensorInfo.enXrayEnergyNum;
                u32Width = pstHeaderV0->stDevInfo.stXsensorInfo.u16LinePixelNum;
                u32Offset = sizeof(PAL_XRAW_HEADER_V0);
            }
            else // 继续判断是否V1版本
            {
                if (u32ReadSize >= sizeof(PAL_XRAW_HEADER_V1))
                {
                    pstHeaderV1 = (PAL_XRAW_HEADER_V1 *)pXRawBuffer;
                    //if (PAL_RAW_EXPORT_MAGIC == pstHeaderV1->magic && pstHeaderV1->version > 0) XRAW文件头赋值不严格，会通不过，这里先注释掉
                    {
                        enXRawDt = pstHeaderV1->stXrawData.enDataType;
                        enXRayEnergy = pstHeaderV1->stDevInfo.stXsensorInfo.enXrayEnergyNum;
                        u32Width = pstHeaderV1->stDevInfo.stXsensorInfo.u16LinePixelNum;
                        u32Offset = sizeof(PAL_XRAW_HEADER_V1);
                    }
                }
                else
                {
                    PRT_INFO("unrecognized the xraw file header: %s, ReadSize: %u\n", pXRawPath, u32ReadSize);
                    goto EXIT;
                }
            }

            if (0 == u32Width || enXRawDt == PAL_XRAW_DT_UNDEF || enXRawDt > PAL_XRAW_DT_PACKAGE_SCANED || 
                enXRayEnergy == PAL_XRAY_ENERGY_UNDEF || enXRayEnergy > PAL_XRAY_ENERGY_DUAL)
            {
                PRT_INFO("u32Offset %u, i %u, j %u, the Width(%u) OR XRawDt(%d) OR XRayEnergy(%d) is invalid\n", 
                         u32Offset, i, j, u32Width, enXRawDt, enXRayEnergy);
                goto EXIT;
            }

            fileSize = dspttk_get_file_size(pXRawPath);
            if (fileSize <= u32Offset)
            {
                PRT_INFO("the xraw file size(%ld) is invalid\n", fileSize);
                goto EXIT;
            }

            PRT_INFO("XRaw File %s, DataType %d, Width %u, Size %ld\n", pXRawPath, enXRawDt, u32Width, fileSize);
            if (PAL_XRAW_DT_PACKAGE_SCANED == enXRawDt)
            {
                memset(pstXRawAttr, 0, sizeof(XRAW_FILE_ATTR));
                strcpy(pstXRawAttr->sFileName, pXRawPath);
                pstXRawAttr->enType = XRAY_TYPE_NORMAL;
                pstXRawAttr->enEnergy = (PAL_XRAY_ENERGY_DUAL == enXRayEnergy) ? XRAY_ENERGY_DUAL : XRAY_ENERGY_SINGLE;
                pstXRawAttr->u32Width = u32Width;
                pstXRawAttr->u32Offset = u32Offset;
                pstXRawAttr->u32Size = fileSize - u32Offset;
                pstXRawAttr->u32Height = (PAL_XRAY_ENERGY_DUAL == enXRayEnergy) ? pstXRawAttr->u32Size/u32Width/4 : pstXRawAttr->u32Size/u32Width/2;
                idx++;
            }
            else
            {
                pXRawBuffer = realloc(pXRawBuffer, fileSize);
                if (NULL == pXRawBuffer)
                {
                    PRT_INFO("realloc for 'pXRawBuffer' failed, buffer size: %ld\n", fileSize);
                    goto EXIT;
                }

                u32ReadSize = (UINT32)fileSize;
                if (SAL_SOK == dspttk_read_file(pXRawPath, 0, pXRawBuffer, &u32ReadSize) && u32ReadSize == fileSize) // 读取整个文件
                {
                    // 遍历行数据，检测“RAYIN 0000000000 RAYIN”本底与“RAYIN FFFFFFFFFF RAYIN”满载标识
                    u32Width *= ((PAL_XRAY_ENERGY_DUAL == enXRayEnergy) ? 2 : 1); // 双能，有低能和高能，宽×2
                    u32Height = (u32ReadSize - u32Offset) / u32Width / sizeof(UINT16);
                    pu16XRawLine = (UINT16 *)((UINT8 *)pXRawBuffer + u32Offset);
                    for (k = 0; k < u32Height; k++)
                    {
                        if (0 == memcmp(pu16XRawLine, au8PartingLine, sizeof(au8PartingLine)))
                        {
                            if (idx >= 0) // 已有本底和满载标识，结束上一段本底和满载
                            {
                                pstXRawAttr->u32Size = DSPTTK_XRAW_SIZE(pstXRawAttr->u32Width, pstXRawAttr->u32Height, pstXRawAttr->enEnergy);
                                pstXRawAttr->pu16XRaw = (UINT16 *)malloc(pstXRawAttr->u32Size);
                                if (NULL != pstXRawAttr->pu16XRaw)
                                {
                                    memcpy(pstXRawAttr->pu16XRaw, (UINT8 *)pXRawBuffer + pstXRawAttr->u32Offset, pstXRawAttr->u32Size);
                                }
                                else
                                {
                                    PRT_INFO("malloc for 'pu16XRaw' failed, idx: %ld, buffer size: %u\n", idx, pstXRawAttr->u32Size);
                                    idx--; // 发生错误时，直接跳出
                                    break;
                                }
                            }

                            j = u32Width - (sizeof(au8PartingLine) + 1); // 前后对称，6个像素，12个字节
                            pu16CorrParting = pu16XRawLine + SAL_CEIL(sizeof(au8PartingLine) + 1, 2); // 跳过前3个像素，6个字节
                            for (i = 0; i < j; i++)
                            {
                                if (0 != pu16CorrParting[i] && 0xFFFF != pu16CorrParting[i])
                                {
                                    break;
                                }
                            }
                            if (i == j) // 遍历结束，每个像素均是0或者FFFF
                            {
                                if (idx >= 0) // 已有本底和满载标识，检测是否有重复的本底和满载
                                {
                                    // 这里检测重复的逻辑实际是有问题的，只检测了上一段，未检测所有的，但因为最多只有2段，所以也能正常运行
                                    if ((0 == pu16CorrParting[0] && XRAY_TYPE_CORREC_ZERO == pstXRawAttr->enType) ||
                                        (0xFFFF == pu16CorrParting[0] && XRAY_TYPE_CORREC_FULL == pstXRawAttr->enType))
                                    {
                                        PRT_INFO("reduplicated correct data, parting 0x%x, previous type %d\n", 
                                                 pu16CorrParting[0], pstXRawAttr->enType); // 重复的校正数据
                                        pu16XRawLine = pu16XRawLine + u32Width;
                                        continue;
                                    }
                                }
                                if (idx + 1 < u32ZeroFullXRawCnt) // 最多支持u32ZeroFullXRawCnt段，不要越界
                                {
                                    idx++;
                                    pstXRawAttr += idx;
                                    memset(pstXRawAttr, 0, sizeof(XRAW_FILE_ATTR));
                                    strcpy(pstXRawAttr->sFileName, pXRawPath);
                                    if (0 == pu16CorrParting[0])
                                    {
                                        pstXRawAttr->enType = XRAY_TYPE_CORREC_ZERO;
                                    }
                                    else
                                    {
                                        if (PAL_XRAW_DT_AUTO_CORR == enXRawDt || PAL_XRAW_DT_AUTO_UPDATE == enXRawDt)
                                        {
                                            pstXRawAttr->enType = XRAY_TYPE_AUTO_CORR_FULL;
                                        }
                                        else
                                        {
                                            pstXRawAttr->enType = XRAY_TYPE_CORREC_FULL;
                                        }
                                    }
                                    pstXRawAttr->u32Width = u32Width / ((PAL_XRAY_ENERGY_DUAL == enXRayEnergy) ? 2 : 1); // 双能，前面×2了，这里除回去
                                    pstXRawAttr->enEnergy = (PAL_XRAY_ENERGY_DUAL == enXRayEnergy) ? XRAY_ENERGY_DUAL : XRAY_ENERGY_SINGLE;
                                    pstXRawAttr->u32Offset = (VOID *)(pu16XRawLine + u32Width) - pXRawBuffer; // +u32Width: 跳过本底满载分割线
                                }
                                else // 多段本底满载，XRaw文件可能有问题，直接结束
                                {
                                    PRT_INFO("reduplicated correct data, parting 0x%x\n", pu16CorrParting[0]); // 重复的校正数据
                                    break;
                                }
                            }
                        }
                        else
                        {
                            if (idx >= 0) // 已有本底和满载标识
                            {
                                pstXRawAttr->u32Height++; // 双能分低能与高能，实际高度只有一半
                            }
                        }
                        pu16XRawLine += u32Width;
                    }

                    if (k == u32Height && idx >= 0 && 0 == pstXRawAttr->u32Size) // 遍历所有的行，自然结束
                    {
                        pstXRawAttr->u32Size = DSPTTK_XRAW_SIZE(pstXRawAttr->u32Width, pstXRawAttr->u32Height, pstXRawAttr->enEnergy);
                        pstXRawAttr->pu16XRaw = (UINT16 *)malloc(pstXRawAttr->u32Size);
                        if (NULL != pstXRawAttr->pu16XRaw)
                        {
                            memcpy(pstXRawAttr->pu16XRaw, (UINT8 *)pXRawBuffer + pstXRawAttr->u32Offset, pstXRawAttr->u32Size);
                        }
                        else
                        {
                            PRT_INFO("malloc for 'pu16XRaw' failed, idx: %ld, buffer size: %u\n", idx, pstXRawAttr->u32Size);
                            idx--;
                        }
                    }
                }
                else
                {
                    PRT_INFO("dspttk_read_file '%s' failed, read size %u, expect %ld\n", pXRawPath, u32ReadSize, fileSize);
                }
            }
        }
        else
        {
            PRT_INFO("dspttk_read_file '%s' failed, read size %u, expect %zu\n", pXRawPath, u32ReadSize, sizeof(PAL_XRAW_HEADER_V0));
        }
    }
    else
    {
        PRT_INFO("dspttk_read_file '%s' failed, read size %u\n", pXRawPath, u32ReadSize);
    }

EXIT:
    if (NULL != pXRawBuffer)
    {
        free(pXRawBuffer);
    }

    return (INT32)(idx+1);
}


VOID dspttk_xsensor_send_xray_data_rt(UINT32 u32Chan)
{
    INT32 s32Ret = SAL_SOK, i = 0, j = 0, k = 0;
    CHAR sTaskName[16] = {0};
    INT32 s32DirNum = 0, s32FileNum = 0;
    UINT32 u32SliceSendInterval = 1000000; // 条带发送间隔，单位us
    UINT32 u32Width = 0, u32Offset = 0, u32ReadSize = 0;
    INT32 s32FullIdx = -1, s32ZeroIdx = -1; // 本底满载的文件索引，有效值从0开始
    long fileSize = 0;
    CHAR sXRawDate[16] = {0}; // XRAW文件的日期
    CHAR **ppXrawDirName = NULL, **ppXrawFileName = NULL;
    CHAR *pBaseName = NULL; // 纯文件名，无路径
    XRAY_RAW_DATA_INFO *pstRawDataInfo = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENV_INPUT *pstInputDir = &pstDevCfg->stSettingParam.stEnv.stInputDir;
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XRAY_SPEED_PARAM_ST *pstXraySpeed = NULL;
    XRAW_FILE_ATTR **ppXRawAttr = NULL;

    if (u32Chan < pstShareParam->xrayChanCnt)
    {
        pstRawDataInfo = &gstRawDataInfo[u32Chan];
    }
    else
    {
        return;
    }

    snprintf(sTaskName, sizeof(sTaskName), "xsensor-send-%u", u32Chan);
    dspttk_pthread_set_name(sTaskName);

    s32Ret = dspttk_sem_init(&pstRawDataInfo->semXrayRt, 0);
    if (SAL_SOK != s32Ret)
    {
        goto EXIT;
    }

    // 创建XSensor的发送定时器，通过信号量间接驱动该线程发送过包数据
    pstXraySpeed = &pstShareParam->dspCapbPar.xray_speed[pstCfgXray->stSpeedParam.enFormUsr][pstCfgXray->stSpeedParam.enSpeedUsr];
    if (pstXraySpeed->slice_height > 0 && pstXraySpeed->integral_time > 10)
    {
        u32SliceSendInterval = (pstXraySpeed->integral_time - 10) * pstXraySpeed->slice_height; // 因定时器的影响，XRay数据的写入频率较读取速率稍快
    }
    pstRawDataInfo->pVSyncTimer = dspttk_timer_create(u32SliceSendInterval, dspttk_xray_simulate_func, u32Chan);
    if (NULL == pstRawDataInfo->pVSyncTimer)
    {
        PRT_INFO("xray chan %u, dspttk_timer_create failed, SliceSendInterval: %u\n", u32Chan, u32SliceSendInterval);
        goto EXIT;
    }

    // TODO: semSetTip从配置文件结构体中移出来，另外函数结束时需释放
    s32Ret = dspttk_sem_init(&pstDevCfg->stSettingParam.stXray.stTipSet.semSetTip[u32Chan], 0);
    if (SAL_SOK != s32Ret)
    {
        goto EXIT;
    }

    while (1)
    {
        // 每次循环均重新检索rtXraw目录下的子目录，允许动态添加
        s32DirNum = dspttk_get_dir_name_list(pstInputDir->rtXraw, &ppXrawDirName, 2); // 最多支持3级目录，2是已经去除了当前目录
        if (s32DirNum <= 0)
        {
            PRT_INFO("dspttk_get_dir_name_list from '%s' failed, DirNum: %d\n", pstInputDir->rtXraw, s32DirNum);
            break;
        }

        /* 遍历rtXraw目录下所有子目录中的本底、满载和过包数据 */
        for (i = 0; i < s32DirNum; i++)
        {
            s32FileNum = dspttk_get_file_name_list(ppXrawDirName[i], &ppXrawFileName);
            if (s32FileNum >= 2) // 至少需2个文件：1个或2个校正数据文件，1个过包数据文件
            {
                s32FullIdx = -1;
                s32ZeroIdx = -1;
                k = 0;
                // s32FileNum+3：本底满载分2个文件属性，但可能是同一个文件，而且与zero_xxx、full_xxx重复
                ppXRawAttr = (XRAW_FILE_ATTR **)dspttk_array_pointers_alloc(s32FileNum+3, sizeof(XRAW_FILE_ATTR));
                if (NULL == ppXRawAttr)
                {
                    PRT_INFO("dspttk_array_pointers_alloc failed, ArrCnt %d, ElemSize: %zu\n", s32FileNum+3, sizeof(XRAW_FILE_ATTR));
                    free(ppXrawFileName);
                    continue;
                }

                // 遍历当前目录内所有文件，查找文件名符合规则的
                for (j = 0; j < s32FileNum; j++)
                {
                    pBaseName = basename(ppXrawFileName[j]); // 此处有风险，basename可能会修改ppXrawFileName[j]中的字符串
                    if (NULL != pBaseName)
                    {
                        if (sscanf(pBaseName, "zero_w%u_o%u.xraw", &u32Width, &u32Offset) > 0 || // 本底
                            sscanf(pBaseName, "full_w%u_o%u.xraw", &u32Width, &u32Offset) > 0 || // 满载
                            sscanf(pBaseName, "xscan_w%u_o%u.xraw", &u32Width, &u32Offset) > 0) // 过包
                        {
                            memset(ppXRawAttr[k], 0, sizeof(XRAW_FILE_ATTR));

                            strcpy(ppXRawAttr[k]->sFileName, ppXrawFileName[j]);
                            ppXRawAttr[k]->u32Width = u32Width;
                            ppXRawAttr[k]->u32Offset = u32Offset;
                            ppXRawAttr[k]->enEnergy = XRAY_ENERGY_DUAL; // 默认为双能数据
                            if (pBaseName == strstr(pBaseName, "zero_")) // 以zero开头，本底
                            {
                                if (s32ZeroIdx == -1) // 每个文件夹内只允许使用1组本底满载，其他的校正数据会自动忽略
                                {
                                    ppXRawAttr[k]->enType = XRAY_TYPE_CORREC_ZERO;
                                }
                                else
                                {
                                    continue;
                                }
                            }
                            else if (pBaseName == strstr(pBaseName, "full_")) // 以full开头，满载
                            {
                                if (s32FullIdx == -1) // 每个文件夹内只允许使用1组本底满载，其他的校正数据会自动忽略
                                {
                                    ppXRawAttr[k]->enType = XRAY_TYPE_CORREC_FULL;
                                }
                                else
                                {
                                    continue;
                                }
                            }
                            else // 以scan开头，过包数据
                            {
                                ppXRawAttr[k]->enType = XRAY_TYPE_NORMAL;
                            }

                            fileSize = dspttk_get_file_size(ppXrawFileName[j]);
                            if (fileSize > u32Offset)
                            {
                                ppXRawAttr[k]->u32Size = fileSize - u32Offset;
                                ppXRawAttr[k]->u32Height = ppXRawAttr[k]->u32Size / ppXRawAttr[k]->u32Width / 4; // 默认为双能数据
                                /**
                                 * 本底满载数据较小，预先读取，过包数据较大，在写入RT Preview Share Buffer前依次读取
                                 */
                                if (XRAY_TYPE_CORREC_ZERO == ppXRawAttr[k]->enType || XRAY_TYPE_CORREC_FULL == ppXRawAttr[k]->enType)
                                {
                                    ppXRawAttr[k]->pu16XRaw = (UINT16 *)malloc(ppXRawAttr[k]->u32Size);
                                    if (NULL == ppXRawAttr[k]->pu16XRaw)
                                    {
                                        PRT_INFO("malloc for 'pu16XRaw' failed, idx %d, buffer size: %u\n", k, ppXRawAttr[k]->u32Size);
                                        continue;
                                    }

                                    if (SAL_SOK == dspttk_read_file(ppXrawFileName[j], (long)u32Offset, ppXRawAttr[k]->pu16XRaw, &ppXRawAttr[k]->u32Size) &&
                                        ppXRawAttr[k]->u32Size == fileSize - u32Offset)
                                    {
                                        if (XRAY_TYPE_CORREC_ZERO == ppXRawAttr[k]->enType)
                                        {
                                            s32ZeroIdx = k++;
                                        }
                                        else
                                        {
                                            s32FullIdx = k++;
                                        }
                                    }
                                    else
                                    {
                                        PRT_INFO("dspttk_read_file '%s' failed, idx %d, read size %u, expect %ld, file size %ld, offset %u\n", 
                                                 ppXrawFileName[j], j, ppXRawAttr[k]->u32Size, fileSize - u32Offset, fileSize, u32Offset);
                                        free(ppXRawAttr[k]->pu16XRaw);
                                        ppXRawAttr[k]->pu16XRaw = NULL;
                                    }
                                }
                                else
                                {
                                    k++;
                                }
                            }
                            else
                            {
                                PRT_INFO("the file size is invalid, idx %d, file %s, size %ld, offset %u\n", j, ppXrawFileName[j], fileSize, u32Offset);
                            }
                        }
                        else if (sscanf(pBaseName, "ch0%*[12]_%14[0-9].xraw", sXRawDate) > 0) // 从设备中导出的带有文件头的XRAW数据，ch01或ch02开头
                        {
                            // 解析XRaw头
                            s32Ret = dspttk_xray_parse_xraw_with_hdr(ppXrawFileName[j], ppXRawAttr + k);
                            if (1 == s32Ret) // 过包数据、尚无相同校正数据类型的本底满载
                            {
                                if ((XRAY_TYPE_NORMAL == ppXRawAttr[k]->enType) ||
                                    (ppXRawAttr[k]->enType == XRAY_TYPE_CORREC_ZERO && s32ZeroIdx == -1) ||
                                    (ppXRawAttr[k]->enType == XRAY_TYPE_CORREC_FULL && s32FullIdx == -1) ||
                                    (ppXRawAttr[k]->enType == XRAY_TYPE_AUTO_CORR_FULL && s32FullIdx == -1))
                                {
                                    if (XRAY_TYPE_CORREC_ZERO == ppXRawAttr[k]->enType)
                                    {
                                        s32ZeroIdx = k;
                                    }
                                    else if (XRAY_TYPE_CORREC_FULL == ppXRawAttr[k]->enType || XRAY_TYPE_AUTO_CORR_FULL == ppXRawAttr[k]->enType)
                                    {
                                        s32FullIdx = k;
                                    }
                                    k++;
                                }
                            }
                            else if (2 == s32Ret) // 尚无任何校正数据，2组校正数据一组为本底，另一组为满载
                            {
                                if (s32ZeroIdx == -1 && s32FullIdx == -1)
                                {
                                    if ((ppXRawAttr[k]->enType == XRAY_TYPE_CORREC_ZERO) && 
                                        (ppXRawAttr[k+1]->enType == XRAY_TYPE_CORREC_FULL || ppXRawAttr[k+1]->enType == XRAY_TYPE_AUTO_CORR_FULL))
                                    {
                                        s32ZeroIdx = k++; // 本底校正数据在前
                                        s32FullIdx = k++; // 满载校正数据在后
                                    }
                                    else if ((ppXRawAttr[k]->enType == XRAY_TYPE_CORREC_FULL || ppXRawAttr[k]->enType == XRAY_TYPE_AUTO_CORR_FULL) && 
                                             (ppXRawAttr[k+1]->enType == XRAY_TYPE_CORREC_ZERO))
                                    {
                                        s32FullIdx = k++; // 本底校正数据在前
                                        s32ZeroIdx = k++; // 满载校正数据在后
                                    }
                                }
                                else
                                {
                                    PRT_INFO("got ZERO and FULL correction data already, ignore this file: %d, %s\n", j, ppXrawFileName[j]);
                                }
                            }
                            else
                            {
                                PRT_INFO("dspttk_xray_parse_xraw_with_hdr failed, idx: %d, file: %s, return: %d\n", j, ppXrawFileName[j], s32Ret);
                            }
                        }
                        else
                        {
                            PRT_INFO("ignore this file: %s, idx: %d\n", pBaseName, j);
                        }
                    }
                    else
                    {
                        PRT_INFO("basename() return NULL, idx: %d, XrawFileName: %s\n", j, ppXrawFileName[j]);
                    }
                }

                // 检查该目录内文件类型是否完整：至少3个文件，仅有1个本底、1个满载
                if (k >= 3 && s32ZeroIdx >= 0 && s32FullIdx >= 0)
                {
                    // 先将本底和满载写入share buffer
                    s32Ret = dspttk_xray_write_rt_share_buf(u32Chan, ppXRawAttr[s32ZeroIdx]);
                    free(ppXRawAttr[s32ZeroIdx]->pu16XRaw);
                    if (SAL_SOK != s32Ret)
                    {
                        PRT_INFO("dspttk_xray_write_rt_share_buf failed, Zero Idx %d, File %s\n", s32ZeroIdx, ppXRawAttr[s32ZeroIdx]->sFileName);
                    }

                    ppXRawAttr[s32FullIdx]->enType = XRAY_TYPE_CORREC_FULL; // 为了启用包裹检测，强制写为手动校正数据
                    s32Ret = dspttk_xray_write_rt_share_buf(u32Chan, ppXRawAttr[s32FullIdx]);
                    free(ppXRawAttr[s32FullIdx]->pu16XRaw);
                    if (SAL_SOK != s32Ret)
                    {
                        PRT_INFO("dspttk_xray_write_rt_share_buf failed, Full Idx %d, File %s\n", s32FullIdx, ppXRawAttr[s32FullIdx]->sFileName);
                    }

                    // TODO: 此处有问题，InputDelayTime应定义为INT型，要支持负数，先注释掉
                    #if 0
                    if (u32Chan == 1)
                    {
                        dspttk_msleep(pstDevCfg->stSettingParam.stXray.u32InputDelayTime);
                        PRT_INFO("chan 1 InputDelayTime is %u ms\n", pstDevCfg->stSettingParam.stXray.u32InputDelayTime);
                    }
                    #endif

                    // 然后依次写入过包数据
                    for (j = 0; j < k; j++)
                    {
                        if (XRAY_TYPE_NORMAL == ppXRawAttr[j]->enType)
                        {
                            ppXRawAttr[j]->pu16XRaw = (UINT16 *)malloc(ppXRawAttr[j]->u32Size);
                            if (NULL == ppXRawAttr[j]->pu16XRaw)
                            {
                                PRT_INFO("malloc for 'pu16XRaw' failed, idx %d, buffer size: %u\n", j, ppXRawAttr[j]->u32Size);
                                continue;
                            }

                            u32ReadSize = ppXRawAttr[j]->u32Size;
                            if (SAL_SOK == dspttk_read_file(ppXRawAttr[j]->sFileName, (long)ppXRawAttr[j]->u32Offset, ppXRawAttr[j]->pu16XRaw, &u32ReadSize) &&
                                u32ReadSize == ppXRawAttr[j]->u32Size)
                            {
                                s32Ret = dspttk_xray_write_rt_share_buf(u32Chan, ppXRawAttr[j]);
                                if (SAL_SOK != s32Ret)
                                {
                                    PRT_INFO("dspttk_xray_write_rt_share_buf failed, Package Idx %d, File %s\n", j, ppXRawAttr[j]->sFileName);
                                }
                            }
                            else
                            {
                                PRT_INFO("dspttk_read_file '%s' failed, idx %d, read size %u, expect %ld, file size %ld, offset %u\n", 
                                         ppXRawAttr[j]->sFileName, j, ppXRawAttr[k]->u32Size, fileSize - u32Offset, fileSize, u32Offset);
                            }

                            free(ppXRawAttr[j]->pu16XRaw);
                            ppXRawAttr[j]->pu16XRaw = NULL;
                        }
                    }
                }

                free(ppXRawAttr);
            }

            if (s32FileNum > 0)
            {
                free(ppXrawFileName);
                ppXrawFileName = NULL;
            }
        }

        if (s32DirNum > 0)
        {
            free(ppXrawDirName);
            ppXrawDirName = NULL;
        }

        /* 遍历完所有文件夹是否重新遍历 */
        if (!pstDevCfg->stSettingParam.stEnv.bInputDirRtLoop)
        {
            break;
        }
    }

EXIT:
    // 销毁定时器
    if (NULL != pstRawDataInfo->pVSyncTimer)
    {
        dspttk_timer_destroy(pstRawDataInfo->pVSyncTimer);
        pstRawDataInfo->pVSyncTimer = NULL;
    }

    if (SAL_SOK != dspttk_sem_destroy(&pstRawDataInfo->semXrayRt))
    {
        PRT_INFO("dspttk_sem_destroy is failed\n");
    }

    return;
}


/**
 * @fn      dspttk_xray_rtpreview_change_speed
 * @brief   切换实时过包速度
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 * @param   [IN] XRAY_SPEED_PARAM 隐含入参，速度参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xray_rtpreview_change_speed(UINT32 u32Chan)
{
    UINT64 s64Ret = SAL_FAIL;
    INT32 sRet = SAL_SOK;
    UINT32 i = 0, u32SliceSendInterval = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XRAY_SPEED_PARAM_ST *pstXraySpeed = NULL;

    if (gstGlobalStatus.enXrayPs == DSPTTK_STATUS_RTPREVIEW)
    {
        s64Ret = dspttk_xray_all_status_stop(0); // TODO: 这边为什么要停止预览，再重新开预览？
        if (SAL_FAIL == CMD_RET_OF(s64Ret))
        {
            sRet = SAL_FAIL;
            PRT_INFO("set dspttk_xray_all_status_stop failed\n");
        }

        sRet |= SendCmdToDsp(HOST_CMD_XRAY_CHANGE_SPEED, 0, NULL, &pstCfgXray->stSpeedParam);
        s64Ret = dspttk_xray_rtpreview_start(0);
        if (SAL_FAIL == CMD_RET_OF(s64Ret))
        {
            sRet = SAL_FAIL;
            PRT_INFO("set dspttk_xray_rtpreview_start failed %lld\n", s64Ret);
        }
    }
    else
    {
        PRT_INFO("speed %d form %d\n", pstCfgXray->stSpeedParam.enSpeedUsr, pstCfgXray->stSpeedParam.enFormUsr);
        sRet |= SendCmdToDsp(HOST_CMD_XRAY_CHANGE_SPEED, 0, NULL, &pstCfgXray->stSpeedParam);
    }

    pstXraySpeed = &pstShareParam->dspCapbPar.xray_speed[pstCfgXray->stSpeedParam.enFormUsr][pstCfgXray->stSpeedParam.enSpeedUsr];
    if (pstXraySpeed->slice_height > 0 && pstXraySpeed->integral_time > 10)
    {
        u32SliceSendInterval = (pstXraySpeed->integral_time - 10) * pstXraySpeed->slice_height; // 因定时器的影响，XRay数据的写入频率较读取速率稍快
        for (i = 0; i < pstShareParam->xrayChanCnt; i++)
        {
            if (NULL != gstRawDataInfo[i].pVSyncTimer)
            {
                sRet = dspttk_timer_set_time(gstRawDataInfo[i].pVSyncTimer, u32SliceSendInterval);
            }
        }
    }

    return CMD_RET_MIX(HOST_CMD_XRAY_CHANGE_SPEED, sRet);
}


/**
 * @fn      dspttk_xray_playback_stop
 * @brief   关闭回拉功能
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
UINT64 dspttk_xray_playback_stop(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK ;

    if (gstGlobalStatus.enXrayPs == DSPTTK_STATUS_PLAYBACK)
    {
        sRet = SendCmdToDsp(HOST_CMD_XRAY_PLAYBACK_STOP, 0, NULL, NULL);
        if (SAL_SOK == sRet)
        {
            gstGlobalStatus.enXrayPs = DSPTTK_STATUS_NONE;
        }
    }

    return CMD_RET_MIX(HOST_CMD_XRAY_PLAYBACK_STOP, sRet);
}


/**
 * @fn      dspttk_xray_rtpreview_start
 * @brief   开启实时预览
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xray_rtpreview_start(UINT32 u32Chan)
{
    UINT32 i = 0;
    INT32 sRet = SAL_SOK ;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    // TODO: 该接口中状态未用锁保护，可能存在问题！！！
    // 若当前在回拉模式，自动结束回拉
    if (gstGlobalStatus.enXrayPs == DSPTTK_STATUS_PLAYBACK)
    {
        sRet = SendCmdToDsp(HOST_CMD_XRAY_PLAYBACK_STOP, 0, NULL, NULL); // 停止回拉不区分通道号
        if (SAL_SOK == sRet)
        {
            gstGlobalStatus.enXrayPs = DSPTTK_STATUS_NONE;
        }
        else // 停止回拉失败直接返回错误
        {
            return CMD_RET_MIX(HOST_CMD_XRAY_PLAYBACK_STOP, sRet);
        }
    }

    if (gstGlobalStatus.enXrayPs == DSPTTK_STATUS_NONE)
    {
        for (i = 0; i < pstShareParam->xrayChanCnt; i++)
        {
            sRet = SendCmdToDsp(HOST_CMD_XRAY_INPUT_START, i, NULL, NULL);
            if (sRet != SAL_SOK)
            {
                PRT_INFO("xray chan %u, HOST_CMD_XRAY_INPUT_START(0x%x) failed\n", i, HOST_CMD_XRAY_INPUT_START);
                return CMD_RET_MIX(HOST_CMD_XRAY_INPUT_START, sRet);
            }
        }
        gstGlobalStatus.enXrayPs = DSPTTK_STATUS_RTPREVIEW;
    }
    else if (gstGlobalStatus.enXrayPs == DSPTTK_STATUS_RTPREVIEW_STOP)
    {
        gstGlobalStatus.enXrayPs = DSPTTK_STATUS_RTPREVIEW;
        return CMD_RET_MIX(HOST_CMD_XRAY_INPUT_START, SAL_SOK);
    }
    else
    {
        PRT_INFO("current status is already in %d\n", gstGlobalStatus.enXrayPs);
        return CMD_RET_MIX(HOST_CMD_XRAY_INPUT_START, SAL_FAIL);
    }

    /* 创建采传模拟发送数据线程，只创建一次，运行过程中不主动销毁 */
    for (i = 0; i < pstShareParam->xrayChanCnt; i++)
    {
        if (gstRawDataInfo[i].xrayPid == 0)
        {
            sRet = dspttk_pthread_spawn(&gstRawDataInfo[i].xrayPid, DSPTTK_PTHREAD_SCHED_RR, 70, 0x100000, dspttk_xsensor_send_xray_data_rt, 1, i);
            if (sRet != SAL_SOK)
            {
                PRT_INFO("xray chan %u, dspttk_pthread_spawn 'dspttk_xsensor_send_xray_data_rt' failed\n", i);
                gstGlobalStatus.enXrayPs = DSPTTK_STATUS_NONE; /* 如果失败则状态复位 */
                return CMD_RET_MIX(HOST_CMD_XRAY_INPUT_START, SAL_FAIL);
            }
        }
    }

    return CMD_RET_MIX(HOST_CMD_XRAY_INPUT_START, sRet);
}


/**
 * @fn      dspttk_xray_rtpreview_stop
 * @brief   过包状态重置
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
UINT64 dspttk_xray_rtpreview_stop(UINT32 u32Chan)
{
    UINT32 i = 0;
    INT32 sRet = SAL_SOK ;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();

    /* 先切换状态确保只reset一次 */
    dspttk_mutex_lock(&gstGlobalStatus.mutexStatus, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    gstGlobalStatus.enXrayPs = DSPTTK_STATUS_NONE;
    dspttk_mutex_unlock(&gstGlobalStatus.mutexStatus, __FUNCTION__, __LINE__);
    /* 检测状态并发送过包指令 */
    for (i = 0; i < pstShareParam->xrayChanCnt; i++)
    {
        sRet |= SendCmdToDsp(HOST_CMD_XRAY_INPUT_STOP, i, NULL, NULL);
        if (sRet != SAL_SOK)
        {
            PRT_INFO("chan[%d] sRet %d HOST_CMD_XRAY_INPUT_STOP failed \n", i, sRet);
        }
    }
    /* 若停止采集失败则将状态复位 */
    if (SAL_SOK != sRet)
    {
        PRT_INFO("set dspttk_xray_rtpreview_reset failed\n");
        return CMD_RET_MIX(HOST_CMD_XRAY_INPUT_STOP, sRet);
    }
    /* 若停止采集失败则将状态复位 */
    if (SAL_SOK != sRet)
    {
        PRT_INFO("set dspttk_xray_rtpreview_reset failed\n");
        return CMD_RET_MIX(HOST_CMD_XRAY_INPUT_STOP, sRet);
    }

    return CMD_RET_MIX(HOST_CMD_XRAY_INPUT_STOP, sRet);
}


/**
 * @fn      dspttk_xray_all_status_stop
 * @brief   关闭实时预览
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xray_all_status_stop(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK ;
    UINT32 i = 0;
    UINT64 s64Ret = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_GLOBAL_STATUS *pstGlobalStatus = &gstGlobalStatus;

    /* 检测状态并发送过包指令 */
    if (pstGlobalStatus->enXrayPs == DSPTTK_STATUS_PLAYBACK)
    {
        s64Ret = dspttk_xray_playback_stop(0);
        if (SAL_FAIL == CMD_RET_OF(s64Ret))
        {
            sRet = SAL_FAIL;
            PRT_INFO("set dspttk_xray_rtpreview_start failed\n");
        }
        for (i = 0; i < pstShareParam->xrayChanCnt; i++)
        {
            sRet |= SendCmdToDsp(HOST_CMD_XRAY_INPUT_START, i, NULL, NULL);
            if (sRet == SAL_FAIL )
            {
                PRT_INFO("ERROR START RT FAILED chan %d threadid %ld.\n", i, gstRawDataInfo[i].xrayPid);
            }
            pstGlobalStatus->enXrayPs = DSPTTK_STATUS_RTPREVIEW_STOP;
        }
        /* 释放整包回拉的内存 */
        if (pstGlobalStatus->stPbInfo.stPbPack.pu8PbSpliceDataSrc != NULL)
        {
            free(pstGlobalStatus->stPbInfo.stPbPack.pu8PbSpliceDataSrc);
            pstGlobalStatus->stPbInfo.stPbPack.pu8PbSpliceDataSrc = NULL;
        }
        if (pstGlobalStatus->stPbInfo.stPbPack.pu8PbSpliceDataDst != NULL)
        {
            free(pstGlobalStatus->stPbInfo.stPbPack.pu8PbSpliceDataDst);
            pstGlobalStatus->stPbInfo.stPbPack.pu8PbSpliceDataDst = NULL;
        }
        if (pstGlobalStatus->stPbInfo.stPbSave.pu8SaveDataSrc != NULL)
        {
            free(pstGlobalStatus->stPbInfo.stPbSave.pu8SaveDataSrc);
            pstGlobalStatus->stPbInfo.stPbSave.pu8SaveDataSrc = NULL;
        }
        /* 将回拉状态置为条带回拉 */
    }
    else if (pstGlobalStatus->enXrayPs == DSPTTK_STATUS_RTPREVIEW)
    {
        /* 将状态置为none停止信号量发送数据 */
        pstGlobalStatus->enXrayPs = DSPTTK_STATUS_RTPREVIEW_STOP; /* 只停止发送数据但是不停止采集功能 */
    }

    pstGlobalStatus->stPbInfo.enPbStatus = DSPTTK_STATUS_PB_SLICE;

    return CMD_RET_MIX(HOST_CMD_XRAY_INPUT_STOP, sRet);
}


/**
 * @fn      dspttk_xray_playback_slice_data
 * @brief   条带回拉模式，读取条带数据与包裹信息，并写入指定Buffer
 *
 * @param   [OUT] pu8PbFrameBuf 写入条带数据与包裹信息的Buffer
 * @param   [IN] pstSlice 数据库中的条带数据
 * @param   [IN] u32SliceNum 条带个数
 * @param   [IN] pstPackage 数据库中的包裹信息
 * @param   [IN] pu32PackageNum 包裹个数
 * @param   [IN] bSliceNb 是否是邻域条带
 * @param   [IN] bForward 是否是正向回拉
 * 
 * @return  UINT32 写入Buffer的数据量，单位：字节
 */
static UINT32 dspttk_xray_playback_slice_data(UINT8 *pu8PbFrameBuf, DB_TABLE_SLICE *pstSlice, UINT32 u32SliceNum, 
                                              DB_TABLE_PACKAGE *pstPackage, UINT32 *pu32PackageNum, BOOL bSliceNb, BOOL bForward)
{
    SAL_STATUS sRet = 0;
    UINT32 i = 0, j = 0, u32ReadSize = 0;
    UINT32 u32PackageNum = 0, u32PackageRstCnt = 0;
    UINT8 *pu8Offset = NULL, *pu8SliceNraw = NULL;
    XRAY_PB_SLICE_HEADER *pstSliceHdr = NULL;
    XRAY_PB_PACKAGE_RESULT *pstPackageRst = NULL;
    XRAY_NRAW_SPLIT_PARAM stSplitParam = {0};

    if (NULL == pu8PbFrameBuf || NULL == pstSlice)
    {
        PRT_INFO("pu8PbFrameBuf(%p) OR pstSlice(%p) is NULL\n", pu8PbFrameBuf, pstSlice);
        return 0;
    }
    if (0 == u32SliceNum)
    {
        PRT_INFO("the u32SliceNum is ZERO\n");
        return 0;
    }
    if (NULL != pu32PackageNum && *pu32PackageNum > 0)
    {
        u32PackageNum = *pu32PackageNum;
        if (NULL == pstPackage)
        {
            PRT_INFO("the u32PackageNum(%u) is non-zero, but pstPackage is NULL\n", u32PackageNum);
            return 0;
        }
    }

    pu8Offset = pu8PbFrameBuf;
    for (i = 0; i < u32SliceNum; i++)
    {
        pstSliceHdr = (XRAY_PB_SLICE_HEADER *)pu8Offset;
        pstSliceHdr->u64ColNo = pstSlice[i].u64No;
        pstSliceHdr->u32Size = pstSlice[i].u32Size;
        pstSliceHdr->u32Width = pstSlice[i].u32Width;
        pstSliceHdr->u32HeightIn = pstSlice[i].u32Height; // 目前所有条件均不会出现只有半段数据的情况，因此u32HeightIn和u32HeightTotal
        pstSliceHdr->u32HeightTotal = pstSlice[i].u32Height;
        pstSliceHdr->enSliceCont = bSliceNb ? XSP_SLICE_NEIGHBOUR : pstSlice[i].enContent; // 区分条带是邻域条带还是显示条带
        pstSliceHdr->enPackTag = XSP_PACKAGE_NONE;
        pstSliceHdr->bAppendPackageResult = SAL_FALSE;
        for (j = 0; j < u32PackageNum; j++)
        {
            if (pstSliceHdr->u64ColNo == pstPackage[j].u64SliceStart)
            {
                pstSliceHdr->enPackTag = XSP_PACKAGE_START;
                pstSliceHdr->bAppendPackageResult = SAL_TRUE;
                break;
            }
            else if (pstSliceHdr->u64ColNo == pstPackage[j].u64SliceEnd)
            {
                pstSliceHdr->enPackTag = XSP_PACKAGE_END;
                pstSliceHdr->bAppendPackageResult = SAL_TRUE;
                break;
            }
            else if (pstSliceHdr->u64ColNo > pstPackage[j].u64SliceStart && pstSliceHdr->u64ColNo < pstPackage[j].u64SliceEnd)
            {
                pstSliceHdr->enPackTag = XSP_PACKAGE_MIDDLE;
                break;
            }
        }

        if (XSP_SLICE_BLANK == pstSliceHdr->enSliceCont && XSP_PACKAGE_NONE == pstSliceHdr->enPackTag)
        {
            pstSliceHdr->u32Size = 0; // 空白条带不读取回调的条带数据，DSP内部自动生成
        }
        pu8Offset += sizeof(XRAY_PB_SLICE_HEADER);

        /***************** 读取包裹信息 *****************/
        if (pstSliceHdr->bAppendPackageResult)
        {
            u32PackageRstCnt++;
            pstPackageRst = (XRAY_PB_PACKAGE_RESULT *)pu8Offset;
            pstPackageRst->u32PackageId = (UINT32)pstPackage[j].u64Id;
            pstPackageRst->u32PackageWidth = pstPackage[j].u32Width;
            pstPackageRst->u32PackageHeight = pstPackage[j].u32Height;

            // 读取包裹的智能信息
            u32ReadSize = sizeof(XPACK_SVA_RESULT_OUT);
            sRet = dspttk_read_file(pstPackage[j].sInfoPath, SAL_offsetof(XPACK_RESULT_ST, stPackSavPrm), 
                                    (UINT8 *)&pstPackageRst->stSvaInfo + SAL_offsetof(SVA_DSP_OUT, target_num), &u32ReadSize);
            if (SAL_SOK != sRet || u32ReadSize != sizeof(XPACK_SVA_RESULT_OUT))
            {
                PRT_INFO("dspttk_read_file %s failed, sRet: %d, ReadSize: %u, expect: %zu\n", 
                         pstPackage[j].sInfoPath, sRet, u32ReadSize, sizeof(XPACK_SVA_RESULT_OUT));
                pstPackageRst->stSvaInfo.target_num = 0; // 读取失败时，危险品数量强制为0
            }

            // 读取包裹的成像信息
            u32ReadSize = sizeof(XSP_PACK_INFO);
            sRet = dspttk_read_file(pstPackage[j].sInfoPath, SAL_offsetof(XPACK_RESULT_ST, igmPrm), &pstPackageRst->stProPrm, &u32ReadSize);
            if (SAL_SOK != sRet || u32ReadSize != sizeof(XSP_PACK_INFO))
            {
                PRT_INFO("dspttk_read_file %s failed, sRet: %d, ReadSize: %u, expect: %zu\n", 
                         pstPackage[j].sInfoPath, sRet, u32ReadSize, sizeof(XSP_PACK_INFO));
                memset(&pstPackageRst->stProPrm, 0, sizeof(XSP_PACK_INFO)); // 读取失败时，清空成像信息
            }

            pu8Offset += sizeof(XRAY_PB_PACKAGE_RESULT);
        }
/*         if (i == 0 || i == u32SliceNum -1)
        {
            PRT_INFO("%s %s no.%llu en %d tag %d\n", i == 0 ? "Start" : "End", 
                        pstSlice[i].sPath, pstSliceHdr->u64ColNo, pstSliceHdr->enSliceCont, pstSliceHdr->enPackTag);
        } */

        /***************** 读取条带数据 *****************/
        if (pstSliceHdr->u32Size > 0)
        {
            u32ReadSize = pstSliceHdr->u32Size;
            if (bForward) // 正向回拉时NRaw数据倒着读
            {
                pu8SliceNraw = (UINT8 *)malloc(pstSliceHdr->u32Size);
                if (NULL != pu8SliceNraw)
                {
                    sRet = dspttk_read_file(pstSlice[i].sPath, 0, pu8SliceNraw, &u32ReadSize);
                    if (SAL_SOK == sRet && u32ReadSize == pstSliceHdr->u32Size)
                    {
                        stSplitParam.u32SrcWidth = pstSliceHdr->u32Width;
                        stSplitParam.u32SrcHeight = pstSliceHdr->u32HeightTotal;
                        stSplitParam.pSrcBuf = pu8SliceNraw;
                        stSplitParam.u32StartLine = pstSliceHdr->u32HeightTotal - 1;
                        stSplitParam.u32EndLine = pstSliceHdr->u32HeightTotal - pstSliceHdr->u32HeightIn;
                        stSplitParam.pDestBuf = pu8Offset;
                        if (0 != SendCmdToDsp(HOST_CMD_XRAY_SPLIT_NRAW, 0, NULL, &stSplitParam))
                        {
                            PRT_INFO("SendCmdToDsp HOST_CMD_XRAY_SPLIT_NRAW(0x%x) failed\n", HOST_CMD_XRAY_SPLIT_NRAW);
                        }
                    }
                    else
                    {
                        PRT_INFO("dspttk_read_file %s failed, sRet: %d, ReadSize: %u, expect: %u\n", 
                                 pstSlice[i].sPath, sRet, u32ReadSize, pstSliceHdr->u32Size);
                    }
                    free(pu8SliceNraw);
                }
            }
            else
            {
                sRet = dspttk_read_file(pstSlice[i].sPath, 0, pu8Offset, &u32ReadSize);
                if (SAL_SOK != sRet || u32ReadSize != pstSliceHdr->u32Size)
                {
                    PRT_INFO("dspttk_read_file %s failed, sRet: %d, ReadSize: %u, expect: %u\n", 
                             pstSlice[i].sPath, sRet, u32ReadSize, pstSliceHdr->u32Size);
                }
            }

            pu8Offset += pstSliceHdr->u32Size; // 读取失败时，不做任何操作，Buffer保留
        }
     /*   PRT_INFO("No %llu size %u w %u h %u to_h %u cont %d\n", pstSliceHdr->u64ColNo, pstSliceHdr->u32Size, pstSliceHdr->u32Width, 
                        pstSliceHdr->u32HeightIn, pstSliceHdr->u32HeightTotal, pstSliceHdr->enSliceCont);*/
    }

    // 回拉帧中仅有1个包裹，且无开始和结束，重置回拉帧中包裹数为0
    if (1 == u32PackageNum && 0 == u32PackageRstCnt)
    {
        *pu32PackageNum = 0;
    }

    return pu8Offset - pu8PbFrameBuf;
}


/**
 * @fn      dspttk_xray_playback_slice_get_head
 * @brief   条带回拉模式，获取当前屏幕上的头条带号（条带号最大）
 * 
 * @param   [IN] u32Chan XRay通道号
 * 
 * @return  UINT64 非0：获取成功，尾条带号；0：获取失败
 */
static UINT64 dspttk_xray_playback_slice_get_head(UINT32 u32Chan)
{
    UINT64 u64SliceNo = 0;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_NODE *pNode = NULL;

    if (u32Chan >= pstShareParam->xrayChanCnt)
    {
        PRT_INFO("the u32Chan(%u) is invalid, range: [0, %u]\n", u32Chan, pstShareParam->xrayChanCnt);
        return 0;
    }

    dspttk_mutex_lock(&gstPbSliceRange[u32Chan].lstPbSlice->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    pNode = dspttk_lst_get_head(gstPbSliceRange[u32Chan].lstPbSlice);
    if (NULL != pNode)
    {
        u64SliceNo = *(UINT64 *)pNode->pAdData;
    }
    dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].lstPbSlice->sync.mid, __FUNCTION__, __LINE__);

    return u64SliceNo;
}


/**
 * @fn      dspttk_xray_playback_slice_get_tail
 * @brief   条带回拉模式，获取当前屏幕上的尾条带号（条带号最小）
 * 
 * @param   [IN] u32Chan XRay通道号
 * 
 * @return  UINT64 非0：获取成功，尾条带号；0：获取失败
 */
static UINT64 dspttk_xray_playback_slice_get_tail(UINT32 u32Chan)
{
    UINT64 u64SliceNo = 0;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_NODE *pNode = NULL;

    if (u32Chan >= pstShareParam->xrayChanCnt)
    {
        PRT_INFO("the u32Chan(%u) is invalid, range: [0, %u]\n", u32Chan, pstShareParam->xrayChanCnt);
        return 0;
    }

    dspttk_mutex_lock(&gstPbSliceRange[u32Chan].lstPbSlice->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    pNode = dspttk_lst_get_tail(gstPbSliceRange[u32Chan].lstPbSlice);
    if (NULL != pNode)
    {
        u64SliceNo = *(UINT64 *)pNode->pAdData;
    }
    dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].lstPbSlice->sync.mid, __FUNCTION__, __LINE__);

    return u64SliceNo;
}


/**
 * @fn      dspttk_xray_playback_slice_build_frame
 * @brief   条带回拉模式，根据回拉方向与回拉列数，检索条带与包裹信息，整合成回拉帧后写入share buffer
 * 
 * @param   [IN] u32Chan 无效参数，实际不使用
 * @param   [IN] enPbDir 回拉方向
 * @param   [IN] u32ColNum 回拉的列数，初次回拉为一整屏，实际回拉的列数不会超过该值
 *
 * @return  VOID 无
 */
static VOID dspttk_xray_playback_slice_build_frame(UINT32 u32Chan, XRAY_PROC_DIRECTION enPbDir, UINT32 u32ColNum)
{
    SAL_STATUS sRet = 0;
    UINT32 i = 0;
    BOOL bPbForward = SAL_FALSE; // 是否为正向回拉（回拉方向与实时过包方向相反）
    UINT32 u32ShareBufRes = 0, u32PbFrameBufSize = 0;
    UINT32 u32ShareBufDir = 0;/* ShareBuf 剩余可用len */
    UINT32 u32SliceNum = 0, u32NbTopNum = 1, u32NbBottomNum = 1, u32PackageNum = 0;
    UINT32 u32PbFrameHeight = 0;
    UINT64 u64SliceHead = 0, u64SliceTail = 0;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_NODE *pNode = NULL;
    UINT8 *pu8PbFrameBuf = NULL, *pu8Offset = NULL;
    XRAY_SHARE_BUF *pstShareBuf = NULL;
    XRAY_ELEMENT *pstXRayElem = NULL;
    DB_TABLE_PACKAGE *pstPackage = NULL;
    DB_TABLE_SLICE *pstSlice = NULL, *pstNbTop = NULL, *pstNbBottom = NULL;
    DSPTTK_XRAY_RT_SLICE_STATUS *pstRtSliceStatus = NULL;

    if (u32Chan >= pstShareParam->xrayChanCnt)
    {
        PRT_INFO("the u32Chan(%u) is invalid, range: [0, %u]\n", u32Chan, pstShareParam->xrayChanCnt);
        return;
    }

    if (0 == u32ColNum)
    {
        PRT_INFO("chan %u, the u32ColNum(%u) is invalid\n", u32Chan, u32ColNum);
        return;
    }

    pstRtSliceStatus = &gstGlobalStatus.stRtSliceStatus[u32Chan];
    pstShareBuf = &pstShareParam->xrayPbBuf[u32Chan];

    // 首次回拉仅支持正向回拉
    if (0 == dspttk_lst_get_count(gstPbSliceRange[u32Chan].lstPbSlice) && enPbDir == pstRtSliceStatus->enDir)
    {
        PRT_INFO("chan %u, playback direction(%d) must be opposite to rt-preview(%d)\n", u32Chan, enPbDir, pstRtSliceStatus->enDir);
        return;
    }
    dspttk_mutex_lock(&gstPbSliceRange[u32Chan].mutexStatus, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

    // 根据最后一个条带的高度计算需要的条带数
    u32SliceNum = SAL_DIV_SAFE(u32ColNum, pstRtSliceStatus->u32SliceHeight);
    if (0 == dspttk_lst_get_count(gstPbSliceRange[u32Chan].lstPbSlice)) // 初次进入条带回拉
    {
        u64SliceTail = pstRtSliceStatus->u64SliceNo + 1;
        gstPbSliceRange[u32Chan].u32StartCol = gstPbSliceRange[u32Chan].u32EndCol = pstRtSliceStatus->u32SliceHeight;
    }
    else
    {
        u64SliceHead = dspttk_xray_playback_slice_get_head(u32Chan);
        u64SliceTail = dspttk_xray_playback_slice_get_tail(u32Chan);
    }

    if (enPbDir == gstGlobalStatus.stRtSliceStatus[u32Chan].enDir) // 回拉方向与实时过包方向相同，反向回拉，从HeadNo顺序查找，即条带号大于HeadNo的
    {
        bPbForward = SAL_FALSE;
        sRet = dspttk_query_slice_assigned_n(u32Chan, &pstSlice, &u32SliceNum, u64SliceHead, SAL_FALSE);
        if (sRet != SAL_SOK || u32SliceNum == 0)
        {
            PRT_INFO("chan %u, reverse playback, dspttk_query_slice_assigned_n failed, sRet: %d, u32SliceNum: %u, Range: [0x%llx, 0x%llx]\n", 
                     u32Chan, sRet, u32SliceNum, u64SliceHead, u64SliceTail);
            dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].mutexStatus, __FUNCTION__, __LINE__);
            goto EXIT;
        }
    }
    else // 回拉方向与实时过包方向相反，正向回拉，从TailNo逆序查找，即条带号小于TailNo的
    {
        bPbForward = SAL_TRUE;
        sRet = dspttk_query_slice_assigned_n(u32Chan, &pstSlice, &u32SliceNum, u64SliceTail, SAL_TRUE);
        if (sRet != SAL_SOK || u32SliceNum == 0)
        {
            PRT_INFO("chan %u, forward playback, dspttk_query_slice_assigned_n failed, sRet: %d, u32SliceNum: %u, Range: [0x%llx, 0x%llx]\n", 
                     u32Chan, sRet, u32SliceNum, u64SliceHead, u64SliceTail);
            dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].mutexStatus, __FUNCTION__, __LINE__);
            goto EXIT;
        }

        // 正向回拉时，删除与当前已回拉条带高度不同的条带，仅支持回拉条带高度完全相同的情况
        for (i = 0; i < u32SliceNum; i++)
        {
            if (pstSlice[i].u32Height != pstRtSliceStatus->u32SliceHeight)
            {
                PRT_INFO("chan %u, this slice height(%u) is different with lastest(%u), drop it(%u/%u)\n", 
                         u32Chan, pstSlice[i].u32Height, pstRtSliceStatus->u32SliceHeight, i, u32SliceNum);
                u32SliceNum = i;
                break;
            }
        }
    }
    // PRT_INFO("search start %llu end %llu head %llu tail %llu\n", pstSlice[0].u64No,  pstSlice[u32SliceNum-1].u64No, u64SliceHead, u64SliceTail);
    // 查询回拉帧中条带关联的所有包裹
    sRet = dspttk_query_package_by_sliceno(u32Chan, &pstPackage, &u32PackageNum, pstSlice[0].u64No, pstSlice[u32SliceNum-1].u64No);
    if (sRet != SAL_SOK)
    {
        PRT_INFO("chan %u, dspttk_query_package_by_sliceno failed, SliceNoStart: 0x%llx, SliceNoEnd: 0x%llx\n", 
                 u32Chan, pstSlice[0].u64No, pstSlice[u32SliceNum-1].u64No);
        dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].mutexStatus, __FUNCTION__, __LINE__);
        goto EXIT;
    }

    // 查询回拉帧中起始条带的前条带作为上邻域，结束条带的后条带作为下邻域，这里邻域仅使用1个条带
    if (!bPbForward) // 反向回拉，条带号从小到大排列
    {
        sRet = dspttk_query_slice_assigned_n(u32Chan, &pstNbTop, &u32NbTopNum, pstSlice[0].u64No, SAL_TRUE); // 上邻域条带号小于回拉帧中起始条带号
        if (sRet != SAL_SOK)
        {
            u32NbTopNum = 0;
        }
        sRet = dspttk_query_slice_assigned_n(u32Chan, &pstNbBottom, &u32NbBottomNum, pstSlice[u32SliceNum-1].u64No, SAL_FALSE); // 下邻域条带号大于回拉帧中结束条带号
        if (sRet != SAL_SOK)
        {
            u32NbBottomNum = 0;
        }
    }
    else // 正向回拉，条带号从大到小排列
    {
        sRet = dspttk_query_slice_assigned_n(u32Chan, &pstNbTop, &u32NbTopNum, pstSlice[0].u64No, SAL_FALSE); // 上邻域条带号大于回拉帧中起始条带号
        if (sRet != SAL_SOK)
        {
            u32NbTopNum = 0;
        }
        sRet = dspttk_query_slice_assigned_n(u32Chan, &pstNbBottom, &u32NbBottomNum, pstSlice[u32SliceNum-1].u64No, SAL_TRUE); // 下邻域条带号小于回拉帧中结束条带号
        if (sRet != SAL_SOK)
        {
            u32NbBottomNum = 0;
        }
    }

    u32NbTopNum = 0;
    u32NbBottomNum = 0;

    /* 首次回拉将整屏数据push到队列里但是不发送数据 */
    if (dspttk_lst_get_count(gstPbSliceRange[u32Chan].lstPbSlice) == 0)
    {
        for (i = 0; i < u32SliceNum; i++)
        {
            pNode = dspttk_lst_get_idle_node(gstPbSliceRange[u32Chan].lstPbSlice);
            if (NULL != pNode)
            {
                *(UINT64 *)pNode->pAdData = pstSlice[i].u64No;
                dspttk_lst_push(gstPbSliceRange[u32Chan].lstPbSlice, pNode);
            }
        }
        dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].mutexStatus, __FUNCTION__, __LINE__);
        goto EXIT;
    }

    // 计算写入share buffer的回拉数据量
    u32PbFrameBufSize = sizeof(XRAY_ELEMENT) + sizeof(XRAY_PB_PACKAGE_RESULT) * 2 * u32PackageNum; // ×2：包裹起始条带和结束条带均携带XRAY_PB_PACKAGE_RESULT
    for (i = 0; i < u32SliceNum; i++)
    {
        u32PbFrameBufSize += sizeof(XRAY_PB_SLICE_HEADER) + pstSlice[i].u32Size;
        u32PbFrameHeight += pstSlice[i].u32Height;
    }
    for (i = 0; i < u32NbTopNum; i++)
    {
        u32PbFrameBufSize += sizeof(XRAY_PB_SLICE_HEADER) + pstNbTop[i].u32Size;
    }
    for (i = 0; i < u32NbBottomNum; i++)
    {
        u32PbFrameBufSize += sizeof(XRAY_PB_SLICE_HEADER) + pstNbBottom[i].u32Size;
    }
    if (u32PbFrameHeight > pstShareParam->dspCapbPar.xray_height_max) // 单个回拉帧中的条带总高度不能超过能力集中的xray_height_max
    {
        PRT_INFO("chan %u, u32PbFrameHeight(%u) is bigger than xray_height_max(%u)\n", u32Chan, u32PbFrameHeight, pstShareParam->dspCapbPar.xray_height_max);
        dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].mutexStatus, __FUNCTION__, __LINE__);
        goto EXIT;
    }

    // 计算share buffer的剩余容量
    if (pstShareBuf->readIdx <= pstShareBuf->writeIdx) // 可写Buffer段有回头
    {
        u32ShareBufDir = pstShareBuf->bufLen - pstShareBuf->writeIdx;
        u32ShareBufRes = u32ShareBufDir + pstShareBuf->readIdx;
    }
    else
    {
        u32ShareBufDir = pstShareBuf->readIdx - pstShareBuf->writeIdx;
        u32ShareBufRes = u32ShareBufDir;
    }
    if (u32ShareBufRes < u32PbFrameBufSize) // share buffer的剩余容量不足，忽略该次回拉
    {
        PRT_INFO("chan %u, insufficient free space to put this pb frame, need: %u, current: %u\n", u32Chan, u32PbFrameBufSize, u32ShareBufRes);
        dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].mutexStatus, __FUNCTION__, __LINE__);
        goto EXIT;
    }
    if (u32ShareBufDir < u32PbFrameBufSize) // 无法一次性写入，申请临时内存
    {
        pu8PbFrameBuf = (UINT8 *)malloc(u32PbFrameBufSize);
        if (NULL == pu8PbFrameBuf)
        {
            PRT_INFO("chan %u, insufficient free space to put this pb frame, need: %u, current: %u\n", u32Chan, u32PbFrameBufSize, u32ShareBufRes);
            dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].mutexStatus, __FUNCTION__, __LINE__);
            goto EXIT;
        }
    }
    else
    {
        pu8PbFrameBuf = pstShareBuf->pVirtAddr + pstShareBuf->writeIdx;
    }
    /***************** 填充XRAY_ELEMENT（除了dataLen） *****************/
    pstXRayElem = (XRAY_ELEMENT *)pu8PbFrameBuf;
    pstXRayElem->magic = XRAY_ELEMENT_MAGIC;
    pstXRayElem->chan = u32Chan;
    pstXRayElem->type = XRAY_TYPE_PLAYBACK;
    pstXRayElem->time = (UINT64)dspttk_get_clock_ms();
    pstXRayElem->width = pstSlice[0].u32Width;
    pstXRayElem->height = u32PbFrameHeight;
    pstXRayElem->xrayProcParam.stXrayPlayback.direction = enPbDir;
    pstXRayElem->xrayProcParam.stXrayPlayback.uiWorkMode = XRAY_PB_SLICE;
    pstXRayElem->xrayProcParam.stXrayPlayback.uiSliceNum = u32SliceNum + u32NbTopNum + u32NbBottomNum;

    /***************** 填充回拉帧数据 *****************/

    pu8Offset = pu8PbFrameBuf + sizeof(XRAY_ELEMENT);
    if (u32NbTopNum > 0)
    {
        pu8Offset += dspttk_xray_playback_slice_data(pu8Offset, pstNbTop, u32NbTopNum, NULL, NULL, SAL_TRUE, bPbForward); // 上邻域
    }
    pu8Offset += dspttk_xray_playback_slice_data(pu8Offset, pstSlice, u32SliceNum, pstPackage, &u32PackageNum, SAL_FALSE, bPbForward); // 显示条带
    if (u32NbBottomNum > 0)
    {
        pu8Offset += dspttk_xray_playback_slice_data(pu8Offset, pstNbBottom, u32NbBottomNum, NULL, NULL, SAL_TRUE, bPbForward); // 下邻域
    }
    pstXRayElem->xrayProcParam.stXrayPlayback.uiPackageNum = u32PackageNum;
    pstXRayElem->dataLen = pu8Offset - pu8PbFrameBuf - sizeof(XRAY_ELEMENT);
    if (u32ShareBufDir < u32PbFrameBufSize) // 将临时内存数据放入share buffer中，并释放pu8PbFrameBuf
    {
        PRT_INFO("share %u frame %u len %u pu8Offset %p\n", u32ShareBufDir, u32PbFrameBufSize, pstXRayElem->dataLen, pu8Offset);
        memcpy(pstShareBuf->pVirtAddr + pstShareBuf->writeIdx, pu8PbFrameBuf, u32ShareBufDir);
        memcpy(pstShareBuf->pVirtAddr, pu8PbFrameBuf + u32ShareBufDir, u32PbFrameBufSize - u32ShareBufDir);
        free(pu8PbFrameBuf);
    }

    dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].mutexStatus, __FUNCTION__, __LINE__);

    /***************** 更新share buffer的写索引 *****************/
    pstShareBuf->writeIdx = (pstShareBuf->writeIdx + sizeof(XRAY_ELEMENT) + pstXRayElem->dataLen) % pstShareBuf->bufLen;
    dspttk_mutex_lock(&gstPbSliceRange[u32Chan].lstPbSlice->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);

    /***************** 更新当前屏幕上的条带范围 *****************/
    if (!bPbForward) // 反向回拉，HeadNo和TailNo均递增
    {
        for (i = 0; i < u32SliceNum; i++)
        {
            // 从尾开始删除节点
            pNode = dspttk_lst_get_tail(gstPbSliceRange[u32Chan].lstPbSlice);
            if (NULL != pNode)
            {
                dspttk_lst_delete(gstPbSliceRange[u32Chan].lstPbSlice, pNode);
            }

            // 从头开始插入节点
            pNode = dspttk_lst_get_idle_node(gstPbSliceRange[u32Chan].lstPbSlice);
            if (NULL != pNode)
            {
                *(UINT64 *)pNode->pAdData = pstSlice[i].u64No;
                dspttk_lst_inst(gstPbSliceRange[u32Chan].lstPbSlice, NULL, pNode);
            }
        }
    }
    else // 正向回拉，HeadNo和TailNo均递减
    {
        // 非初次进入条带回拉，先从头开始删除u32SliceNum个节点
        if (dspttk_lst_get_count(gstPbSliceRange[u32Chan].lstPbSlice) > 0)
        {
            for (i = 0; i < u32SliceNum; i++)
            {
                dspttk_lst_pop(gstPbSliceRange[u32Chan].lstPbSlice);
            }
        }

        for (i = 0; i < u32SliceNum; i++)
        {
            pNode = dspttk_lst_get_idle_node(gstPbSliceRange[u32Chan].lstPbSlice);
            if (NULL != pNode)
            {
                *(UINT64 *)pNode->pAdData = pstSlice[i].u64No;
                dspttk_lst_push(gstPbSliceRange[u32Chan].lstPbSlice, pNode);
            }
        }
    }
    dspttk_mutex_unlock(&gstPbSliceRange[u32Chan].lstPbSlice->sync.mid, __FUNCTION__, __LINE__);

EXIT:
    if (NULL != pstSlice)
    {
        dspttk_query_slice_free(pstSlice);
    }
    if (NULL != pstNbTop)
    {
        dspttk_query_slice_free(pstNbTop);
    }
    if (NULL != pstNbBottom)
    {
        dspttk_query_slice_free(pstNbBottom);
    }
    if (NULL != pstPackage)
    {
        dspttk_query_package_free(pstPackage);
    }

    return;
}

/**
 * @fn      dspttk_xray_pb_write_frame2share_buf
 * @brief   左向条带回拉
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
SAL_STATUS dspttk_xray_pb_write_frame2share_buf(UINT32 u32Chan, UINT32 u32Width, UINT32 u32Height, CHAR *cSrc, \
                                              DSPTTK_XRAY_PB_STATUS_E enPbStatus, XRAY_PROC_DIRECTION enDir)
{
    UINT32 u32WriteIdx = 0, u32ReadIdx = 0, u32FrameSize = 0, u32ShareTotalLen = 0, u32ShareFreeSize = 0;
    XRAY_ELEMENT stXRayElem = {0};
    XRAY_SHARE_BUF *pstShareBuf = NULL;
    UINT32 u32FrameHeadLens = 0;  /* element head 偏移量  XRAY_ELEMENT + XRAY_PB_SLICE_HEADER + (XRAY_PB_PACKAGE_RESULT) */
    UINT8 *pu8ShareSrcData = NULL,/* 加入头的源数据 */ *pu8ShareDstAddr = NULL; /* 写入share-buf的目的地址 */
    UINT32 u32ShareAvailableSizeR = 0, u32ShareAvailableSizeL = 0;  /* share-buf中未解析的数据 */
    XRAY_PB_SLICE_HEADER stPbSliceHeader = {0};
    XRAY_PB_PACKAGE_RESULT stPbPackResult = {0};
    DSPTTK_XRAY_PB_INFO *pstPbInfo = &gstGlobalStatus.stPbInfo;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XRAY_ENERGY_NUM enEnergy = pstShareParam->dspCapbPar.xray_energy_num;

    if (cSrc == NULL || u32Chan >= pstShareParam->xrayChanCnt)
    {
        PRT_INFO("cSrc %p chan %d\n", cSrc, u32Chan);
        return SAL_FAIL;
    }
    pstShareBuf = &pstShareParam->xrayPbBuf[u32Chan];
    /***************** 填充XRAY_ELEMENT（除了dataLen） *****************/
    stXRayElem.chan = u32Chan;
    stXRayElem.magic = XRAY_ELEMENT_MAGIC;
    stXRayElem.type = XRAY_TYPE_PLAYBACK;
    stXRayElem.time = (UINT32)dspttk_get_clock_ms();
    stXRayElem.width = u32Width;
    stXRayElem.height = u32Height;
    stXRayElem.xrayProcParam.stXrayPlayback.uiSliceNum = 1;
    stXRayElem.xrayProcParam.stXrayPlayback.uiPackageNum = 1;
    stXRayElem.xrayProcParam.stXrayPlayback.direction = enDir;
    stXRayElem.xrayProcParam.stXrayPlayback.uiWorkMode = XRAY_PB_FRAME;
    stXRayElem.dataLen = NRAW_BYTE_SIZE(stXRayElem.width, stXRayElem.height, enEnergy);
    u32FrameHeadLens = sizeof(XRAY_ELEMENT);
    /* 填充回拉头 宽高信息与XRAY_ELEMENT 相同 */
    stPbSliceHeader.u32Size = stXRayElem.dataLen;
    stPbSliceHeader.u32Width = stXRayElem.width;
    stPbSliceHeader.u32HeightIn = stXRayElem.height;
    stPbSliceHeader.u32HeightTotal = stPbSliceHeader.u32HeightIn;
    stPbSliceHeader.enSliceCont = XSP_SLICE_PACKAGE;
    stPbSliceHeader.bAppendPackageResult = 1;
    u32FrameHeadLens += sizeof(XRAY_PB_SLICE_HEADER);

    /* 写入包裹信息头 */
    if (1 == stPbSliceHeader.bAppendPackageResult)
    {
        if(enPbStatus == DSPTTK_STATUS_PB_PACKAGE)
        {
            memcpy(&(stPbPackResult.stProPrm), &pstPbInfo->stPbPack.stXspInfo, sizeof(XSP_PACK_INFO));
            stPbPackResult.u32PackageId = (UINT32)pstPbInfo->stPbPack.u64CurPbPackageId;
            stPbPackResult.u32PackageId = (stPbPackResult.u32PackageId == 0) ? 1 : stPbPackResult.u32PackageId;

            /* 设置危险品信息 */
            memcpy(&(stPbPackResult.stSvaInfo), &pstPbInfo->stPbPack.stSvaInfo, sizeof(pstPbInfo->stPbPack.stSvaInfo));
            stPbPackResult.u32PackageWidth= stXRayElem.width;
            stPbPackResult.u32PackageHeight = stXRayElem.height;
        }
        else /* DSPTTK_STATUS_PB_SAVE */
        {
            memset(&stPbPackResult, 0x0, sizeof(stPbPackResult));
            stPbPackResult.u32PackageId = (pstPbInfo->stPbSave.u32SaveNo == 0) ? 1 : pstPbInfo->stPbSave.u32SaveNo;
            stPbPackResult.stProPrm.stTransferInfo.uiNoramlDirection = enDir;
        }
        u32FrameHeadLens += sizeof(XRAY_PB_PACKAGE_RESULT);
    }

    u32FrameSize = u32FrameHeadLens + stXRayElem.dataLen;
    if (pu8ShareSrcData == NULL)
    {
        pu8ShareSrcData = (UINT8 *)malloc(u32FrameSize + 128);
        if (pu8ShareSrcData == NULL)
        {
            PRT_INFO("error pu8ShareSrcData %p\n", pu8ShareSrcData);
            return SAL_FAIL;
        }
    }
    memcpy(pu8ShareSrcData , &stXRayElem, sizeof(XRAY_ELEMENT));
    memcpy(pu8ShareSrcData + sizeof(XRAY_ELEMENT), &stPbSliceHeader, sizeof(XRAY_PB_SLICE_HEADER));
    if (1 == stPbSliceHeader.bAppendPackageResult)
    {
        memcpy(pu8ShareSrcData + sizeof(XRAY_ELEMENT) + sizeof(XRAY_PB_SLICE_HEADER), &stPbPackResult, sizeof(XRAY_PB_PACKAGE_RESULT));
    }
    memcpy(pu8ShareSrcData + u32FrameHeadLens, cSrc, NRAW_BYTE_SIZE(u32Width, u32Height, enEnergy));
    #if 0
        char name[128] = {0};
        snprintf(name, 128, "/mnt/dump/pb_w%d_h%d_len%d_%d_%d.bin", stXRayElem.width, stXRayElem.height, stXRayElem.dataLen, u32FrameHeadLens, enDir);
        dspttk_write_file(name, 0, SEEK_SET, pu8ShareSrcData, u32FrameSize);
    #endif
WAIT:
    /* 向share-buf中写数据 */
    u32WriteIdx = pstShareBuf->writeIdx;
    u32ReadIdx = pstShareBuf->readIdx;
    pu8ShareDstAddr = (UINT8 *)pstShareBuf->pVirtAddr;
    if (u32WriteIdx > u32ReadIdx)
    {
        u32ShareFreeSize = pstShareBuf->bufLen - (u32WriteIdx - u32ReadIdx);
    }
    else if (u32WriteIdx < u32ReadIdx)
    {
        u32ShareFreeSize = (u32ReadIdx - u32WriteIdx);
    }

    if ((u32ShareFreeSize > 0) && (u32ShareFreeSize < pstShareBuf->bufLen / 2))
    {
        dspttk_msleep(2);
        goto WAIT;
    }

    u32ShareTotalLen = pstShareBuf->bufLen;
    u32ShareAvailableSizeR = u32ShareTotalLen - u32WriteIdx;

    if (u32FrameSize > u32ShareAvailableSizeR)
    {
        memcpy((char *)(pu8ShareDstAddr + u32WriteIdx), pu8ShareSrcData, u32ShareAvailableSizeR);
        u32ShareAvailableSizeL = u32FrameSize - u32ShareAvailableSizeR;
        memcpy((char *)pu8ShareDstAddr, pu8ShareSrcData + u32ShareAvailableSizeR, u32ShareAvailableSizeL);
    }
    else
    {
        memcpy((char *)(pu8ShareDstAddr + u32WriteIdx), pu8ShareSrcData, u32FrameSize);
    }

    pstShareBuf->writeIdx = (pstShareBuf->writeIdx + u32FrameSize) % pstShareBuf->bufLen;

    if (pu8ShareSrcData != NULL)
    {
        free(pu8ShareSrcData);
        pu8ShareSrcData = NULL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_xray_build_pb_package_frame
 * @brief   将单独的条带拼接成一个回拉帧
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
SAL_STATUS dspttk_xray_build_pb_package_frame(UINT32 u32Chan, UINT64 u64CurPbPackageId, XRAY_NRAW_SPLICE_PARAM *pstSpliceParam)
{
    SAL_STATUS sRet = SAL_FAIL;
    UINT32 i = 0, u32SliceNum = 0, u32ReadSize = 0, u32ReadOffset = 0;
    DSPTTK_XRAY_PB_INFO *pstPbInfo = &gstGlobalStatus.stPbInfo;
    DB_TABLE_PACKAGE stPackage = {0};
    DB_TABLE_SLICE *pstSlice = NULL;

    if (u64CurPbPackageId == 0)
    {
        PRT_INFO("u64CurPbPackageId error\n");
        return SAL_FAIL;
    }

    // 查询指定包裹ID对应的条带范围
    sRet = dspttk_query_package_by_packageid(u32Chan, &stPackage, u64CurPbPackageId);
    if (sRet != SAL_SOK)
    {
        PRT_INFO("chan %u, dspttk_query_package_by_packageid failed\n", u32Chan);
        goto EXIT;
    }

    // 查询指定条带范围内所有的条带数据及其信息
    sRet = dspttk_query_slice_by_range(u32Chan, &pstSlice, &u32SliceNum, stPackage.u64SliceStart, stPackage.u64SliceEnd);
    if (sRet != SAL_SOK || u32SliceNum == 0)
    {
        PRT_INFO("chan %u, dspttk_query_slice_by_range failed, sRet: %d, u32SliceNum: %u, slice range: [%llu, %llu]\n", 
                 u32Chan, sRet, u32SliceNum, stPackage.u64SliceStart, stPackage.u64SliceEnd);
        sRet = SAL_FAIL;
        goto EXIT;
    }

    /***************** 拼接条带数据成整包NRaw *****************/
    // 读取各条带的信息，申请Buffer等
    pstSpliceParam->u32SliceOrder = 0;
    pstSpliceParam->u32SrcWidth = pstSlice[0].u32Width;
    pstSpliceParam->u32SliceHeight = pstSlice[0].u32Height;

    for (i = 0; i < u32SliceNum; i++)
    {
        pstSpliceParam->u32SrcHeightTotal += pstSlice[i].u32Height;
        pstSpliceParam->u32BufSize += pstSlice[i].u32Size;
    }

    if (pstPbInfo->stPbPack.pu8PbSpliceDataSrc == NULL || pstPbInfo->stPbPack.pu8PbSpliceDataDst == NULL)
    {
        PRT_INFO("src %p dst %p\n", pstSpliceParam->pSrcBuf, pstSpliceParam->pDestBuf);
        sRet = SAL_FAIL;
        goto EXIT;
    }
    else
    {
        pstSpliceParam->pSrcBuf = pstPbInfo->stPbPack.pu8PbSpliceDataSrc;
        pstSpliceParam->pDestBuf = pstPbInfo->stPbPack.pu8PbSpliceDataDst;
    }

    /***************** 读取各条带的NRaw数据并拼接成整包NRaw *****************/
    for (i = 0; i < u32SliceNum; i++)
    {
        u32ReadSize = pstSlice[i].u32Size;
        sRet = dspttk_read_file(pstSlice[i].sPath, 0, pstSpliceParam->pSrcBuf+u32ReadOffset, &u32ReadSize);
        if (SAL_SOK != sRet || u32ReadSize != pstSlice[i].u32Size)
        {
            PRT_INFO("dspttk_read_file %s failed, sRet: %d, ReadSize: %u, expect: %u\n", 
                     pstSlice[i].sPath, sRet, u32ReadSize, pstSlice[i].u32Size);
            goto EXIT;
        }
        u32ReadOffset += pstSlice[i].u32Size;
    }
    if (0 != SendCmdToDsp(HOST_CMD_XRAY_SPLICE_NRAW, 0, NULL, pstSpliceParam))
    {
        PRT_INFO("SendCmdToDsp HOST_CMD_XRAY_SPLICE_NRAW(0x%x) failed num %u Id %llu TolH %u.\n",
                             HOST_CMD_XRAY_SPLICE_NRAW, u32SliceNum, u64CurPbPackageId, pstSpliceParam->u32SrcHeightTotal);
        goto EXIT;
    }

    if (pstSpliceParam->u32SrcWidth != stPackage.u32Width || pstSpliceParam->u32SrcHeightTotal != stPackage.u32Height)
    {
        PRT_INFO("Package info(%u X %u) is unmatched slices info(%u X %u)\n", 
                 stPackage.u32Width, stPackage.u32Height, pstSpliceParam->u32SrcWidth, pstSpliceParam->u32SrcHeightTotal);
        goto EXIT;
    }

    /***************** 读取包裹信息 *****************/
    // 读取包裹的智能信息
    u32ReadSize = sizeof(XPACK_SVA_RESULT_OUT);
    sRet = dspttk_read_file(stPackage.sInfoPath, SAL_offsetof(XPACK_RESULT_ST, stPackSavPrm), 
                            (UINT8 *)&pstPbInfo->stPbPack.stSvaInfo + SAL_offsetof(SVA_DSP_OUT, target_num), &u32ReadSize);
    if (SAL_SOK != sRet || u32ReadSize != sizeof(XPACK_SVA_RESULT_OUT))
    {
        PRT_INFO("dspttk_read_file %s failed, sRet: %d, ReadSize: %u, expect: %zu\n", 
                 stPackage.sInfoPath, sRet, u32ReadSize, sizeof(XPACK_SVA_RESULT_OUT));
        goto EXIT;
    }

    // 读取包裹的成像信息
    u32ReadSize = sizeof(XSP_PACK_INFO);
    sRet = dspttk_read_file(stPackage.sInfoPath, SAL_offsetof(XPACK_RESULT_ST, igmPrm), &pstPbInfo->stPbPack.stXspInfo, &u32ReadSize);
    if (SAL_SOK != sRet || u32ReadSize != sizeof(XSP_PACK_INFO))
    {
        PRT_INFO("dspttk_read_file %s failed, sRet: %d, ReadSize: %u, expect: %zu\n", 
                 stPackage.sInfoPath, sRet, u32ReadSize, sizeof(XSP_PACK_INFO));
        goto EXIT;
    }
    sRet = SAL_SOK;

EXIT:
    if (NULL != pstSlice)
    {
        dspttk_query_slice_free(pstSlice);
    }

    return sRet;
}


/**
 * @fn      dspttk_xray_pb_save_pic_write
 * @brief   获取save数据并向share-buf写数据
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
SAL_STATUS dspttk_xray_pb_save_pic_write(UINT32 u32Chan, XRAY_PROC_DIRECTION enDir)
{
    SAL_STATUS sRet = SAL_SOK ;
    INT32 s32SaveNo = 0;
    DSPTTK_XRAY_PB_SAVE *pstPbSave = &gstGlobalStatus.stPbInfo.stPbSave;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_ENV_OUTPUT *pstEnvOutputDir = &pstDevCfg->stSettingParam.stEnv.stOutputDir;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    CHAR szNrawPath[256] = {0};
    DSPTTK_XPACK_SAVE_PRM *pstXpackSaveInfo = NULL;

    pstXpackSaveInfo = dspttk_get_save_screen_list_info(0);
    if (pstPbSave->pu8SaveDataSrc == NULL)
    {
        PRT_INFO("pu8SaveDataSrc is %p\n", pstPbSave->pu8SaveDataSrc);
        return SAL_FAIL;
    }

    s32SaveNo = pstPbSave->u32SaveNo;
    /* save cnt异常检测 */
    if (s32SaveNo >= pstXpackSaveInfo->u32SaveScreenCnt)
    {
        PRT_INFO("warning save no %u > max cnt %u\n", pstPbSave->u32SaveNo, pstXpackSaveInfo->u32SaveScreenCnt);
        s32SaveNo = pstXpackSaveInfo->u32SaveScreenCnt - 1;
    }
    else if (s32SaveNo < 0)
    {
        PRT_INFO("warning save no %d < 0\n", s32SaveNo);
        s32SaveNo = 0; 
    }
    else
    {
        PRT_INFO("save No.%u [0, %u]\n", s32SaveNo, pstXpackSaveInfo->u32SaveScreenCnt - 1);
    }

    pstPbSave->u32Width = (UINT32)(pstDevCfg->stInitParam.stXray.xrayWidthMax * pstDevCfg->stInitParam.stXray.resizeFactor);
    snprintf(pstPbSave->cNrawName, 128, "save_no%u_w%u_h%u.nraw", s32SaveNo, pstPbSave->u32Width, pstPbSave->u32Height);
    snprintf(szNrawPath, sizeof(szNrawPath), "%s/0/%s",pstEnvOutputDir->saveScreen , pstPbSave->cNrawName);
    pstPbSave->u32Size = NRAW_BYTE_SIZE(pstPbSave->u32Width, pstPbSave->u32Height, pstShareParam->dspCapbPar.xray_energy_num);
    /* 数据读取 */
    if(SAL_SOK != dspttk_read_file(szNrawPath, 0, (void *)pstPbSave->pu8SaveDataSrc, &pstPbSave->u32Size))
    {
        PRT_INFO("read file failed path[%s] src[%p]  size[%u] w[%u] h[%u]\n", szNrawPath, pstPbSave->pu8SaveDataSrc, 
                                                            pstPbSave->u32Size, pstPbSave->u32Width, pstPbSave->u32Height);
        return SAL_FAIL;
    }

    /* share-buf数据写入 */
    sRet = dspttk_xray_pb_write_frame2share_buf(0, pstPbSave->u32Width, pstPbSave->u32Height, \
                                                (char *)pstPbSave->pu8SaveDataSrc, DSPTTK_STATUS_PB_SAVE, \
                                                 gstGlobalStatus.stRtSliceStatus[0].enDir);
    if (sRet != SAL_SOK)
    {
        PRT_INFO("dspttk_xray_pb_write_frame2share_buf\n");
    }
    else
    {
        if (pstCfgXray->enDispDir == enDir)
        {
            if (pstPbSave->u32SaveNo < pstXpackSaveInfo->u32SaveScreenCnt - 1)
            {
                pstPbSave->u32SaveNo++;
            }
        }
        else
        {
            if (pstPbSave->u32SaveNo != 0)
            {
                pstPbSave->u32SaveNo--;
            }
            
        }
    }
    return SAL_SOK;
}


/**
 * @fn      dspttk_xray_pb_one_package_write
 * @brief   拼接回拉数据并向share-buf写数据
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
SAL_STATUS dspttk_xray_pb_one_package_write(UINT32 u32Chan, XRAY_PROC_DIRECTION enDir)
{
    SAL_STATUS sRet = SAL_SOK ;
    UINT64 *pu64FirstPackageId = NULL;
    UINT64 u64CurPbPackageId = 0, u64CurTransId = 0;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XRAY_NRAW_SPLICE_PARAM *pstSpliceParam[MAX_XRAY_CHAN] = {NULL, NULL};
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    DSPTTK_XRAY_PB_STATUS_E enPbStatus = gstGlobalStatus.stPbInfo.enPbStatus;
    DSPTTK_XRAY_PB_PACKAGE *pstPbPack = &gstGlobalStatus.stPbInfo.stPbPack;

    pu64FirstPackageId = dspttk_get_first_trans_package_id(u32Chan);
    u64CurTransId = dspttk_get_current_trans_package_id(u32Chan);
    /* 包裹id异常检测 */
    if (pstPbPack->u64CurPbPackageId > u64CurTransId)
    {
        PRT_INFO("warning curId %lld > newId %llu\n", pstPbPack->u64CurPbPackageId, u64CurTransId);
        pstPbPack->u64CurPbPackageId = u64CurTransId;
    }
    else if (pstPbPack->u64CurPbPackageId < *pu64FirstPackageId)
    {
        PRT_INFO("warning curId %lld < firstId %llu\n", pstPbPack->u64CurPbPackageId, *pu64FirstPackageId);
        pstPbPack->u64CurPbPackageId = *pu64FirstPackageId; 
    }
    else
    {
        PRT_INFO("curId %llu [%llu, %llu]\n", pstPbPack->u64CurPbPackageId, u64CurTransId, *pu64FirstPackageId);
    }
    u64CurPbPackageId = pstPbPack->u64CurPbPackageId;
    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        pstSpliceParam[u32Chan] = (XRAY_NRAW_SPLICE_PARAM *)malloc(sizeof(XRAY_NRAW_SPLICE_PARAM));
        if (pstSpliceParam[u32Chan] == NULL)
        {
            PRT_INFO("Chan %d error pstSpliceParam %p\n", u32Chan, pstSpliceParam[u32Chan]);
            return SAL_FAIL;
        }
        memset(pstSpliceParam[u32Chan], 0, sizeof(XRAY_NRAW_SPLICE_PARAM));
        /* 拼接图像帧数据 */
        sRet = dspttk_xray_build_pb_package_frame(u32Chan, u64CurPbPackageId, pstSpliceParam[u32Chan]);
        if (sRet != SAL_SOK)
        {
            PRT_INFO("dspttk_xray_build_pb_package_frame\n");
            goto EXIT;
        }
        /* share-buf数据写入 */
        sRet = dspttk_xray_pb_write_frame2share_buf(u32Chan, pstSpliceParam[u32Chan]->u32SrcWidth, \
                                                        pstSpliceParam[u32Chan]->u32SrcHeightTotal, \
                                                        (CHAR *)pstSpliceParam[u32Chan]->pDestBuf, enPbStatus, enDir);
        if (sRet != SAL_SOK)
        {
            PRT_INFO("dspttk_xray_pb_write_frame2share_buf\n");
            goto EXIT;
        }
    }

    if (pstCfgXray->enDispDir == enDir)
    {
        if (pstPbPack->u64CurPbPackageId < u64CurTransId)
        {
            pstPbPack->u64CurPbPackageId++;
        }
    }
    else
    {
        if (pstPbPack->u64CurPbPackageId > *pu64FirstPackageId)
        {
            pstPbPack->u64CurPbPackageId--;
        }
    }

EXIT:
    for (u32Chan = 0; u32Chan < pstShareParam->xrayChanCnt; u32Chan++)
    {
        if (pstSpliceParam[u32Chan] != NULL)
        {
            free(pstSpliceParam[u32Chan]);
            pstSpliceParam[u32Chan] = NULL;
        }
    }

    return sRet;

}

/**
 * @fn      dspttk_xray_playback_slice_one_shot
 * @brief   单次触发条带回拉 
 * @note    初次条带回拉时，回拉方向只允许与过包方向相反，即正向回拉 
 * 
 * @param   [IN] u32Chan 无效参数，实际不使用
 * @param   [IN] enPbDir 回拉方向
 * 
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
static UINT64 dspttk_xray_playback_slice_one_shot(UINT32 u32Chan, XRAY_PROC_DIRECTION enPbDir)
{
    INT32 sRet = SAL_SOK ;
    UINT32 i = 0, j = 0, u32Cnt = 0;
    DSPTTK_NODE *pNode = NULL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XRAY_PB_PARAM stPbParam = {
        .enPbType = XRAY_PB_SLICE,
        .enPbSpeed = XRAY_PB_SPEED_NORMAL,
    };

    if (DSPTTK_STATUS_NONE == gstGlobalStatus.enXrayPs)
    {
        for (i = 0; i < pstShareParam->xrayChanCnt; i++)
        {
            sRet |= SendCmdToDsp(HOST_CMD_XRAY_PLAYBACK_START, i, NULL, &stPbParam);
            if (NULL == gstPbSliceRange[i].lstPbSlice) // 若条带链表未初始化，则初始化
            {
                gstPbSliceRange[i].lstPbSlice = dspttk_lst_init(DSPTTK_PBSLICE_CNT_ONSCREEN);
                if (NULL != gstPbSliceRange[i].lstPbSlice)
                {
                    for (j = 0; j < DSPTTK_PBSLICE_CNT_ONSCREEN; j++)
                    {
                        if (NULL != (pNode = dspttk_lst_get_idle_node(gstPbSliceRange[i].lstPbSlice)))
                        {
                            pNode->pAdData = gstPbSliceRange[i].u64SliceNo + j;
                            dspttk_lst_push(gstPbSliceRange[i].lstPbSlice, pNode);
                        }
                    }
                    for (j = 0; j < DSPTTK_PBSLICE_CNT_ONSCREEN; j++)
                    {
                        dspttk_lst_pop(gstPbSliceRange[i].lstPbSlice);
                    }
                }
                else
                {
                    PRT_INFO("chan %u, init 'lstPbSlice' failed\n", i);
                    return CMD_RET_MIX(HOST_CMD_XRAY_PLAYBACK_START, SAL_FAIL);
                }
            }

            // 清空链表中的所有节点，数据清0
            u32Cnt = dspttk_lst_get_count(gstPbSliceRange[i].lstPbSlice);
            for (j = 0; j < u32Cnt; j++)
            {
                dspttk_lst_pop(gstPbSliceRange[i].lstPbSlice);
            }
            memset(gstPbSliceRange[i].u64SliceNo, 0, sizeof(gstPbSliceRange[i].u64SliceNo));
            gstPbSliceRange[i].u32StartCol = 0;
            gstPbSliceRange[i].u32EndCol = 0;
        }
        if (SAL_SOK == sRet)
        {
            dspttk_mutex_lock(&gstGlobalStatus.mutexStatus, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            gstGlobalStatus.enXrayPs = DSPTTK_STATUS_PLAYBACK;
            dspttk_mutex_unlock(&gstGlobalStatus.mutexStatus, __FUNCTION__, __LINE__);        }
    }

    if (DSPTTK_STATUS_PLAYBACK == gstGlobalStatus.enXrayPs)
    {
        for (i = 0; i < pstShareParam->xrayChanCnt; i++)
        {
            if (0 == dspttk_lst_get_count(gstPbSliceRange[i].lstPbSlice)) // 首次回拉
            {
                dspttk_xray_playback_slice_build_frame(i, enPbDir, pstShareParam->dspCapbPar.xray_height_max);
            }
            else
            {
                dspttk_xray_playback_slice_build_frame(i, enPbDir, 384); // 固定每次回拉384列
            }
        }
    }

    return CMD_RET_MIX(HOST_CMD_XRAY_PLAYBACK_START, sRet);
}

/**
 * @fn      dspttk_xray_pb_package_start
 * @brief   开始包裹回拉 内存申请 调用回拉命令字
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
UINT64 dspttk_xray_pb_package_start(UINT32 u32Chan)
{
    SAL_STATUS sRet = SAL_SOK;
    UINT32 i = 0, u32BufferWidth = 0, u32BufferHeight = 0;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_XRAY_PB_INFO *pstPbInfo = &gstGlobalStatus.stPbInfo;

    XRAY_PB_PARAM stPbParam = {
        .enPbType = XRAY_PB_FRAME,
        .enPbSpeed = XRAY_PB_SPEED_NORMAL,
    };

    u32BufferWidth = (UINT32)(pstShareParam->dspCapbPar.xray_width_max * pstShareParam->dspCapbPar.xsp_resize_factor.resize_width_factor);
    u32BufferHeight = (UINT32)(pstShareParam->dspCapbPar.xray_height_max * pstShareParam->dspCapbPar.xsp_resize_factor.resize_height_factor);
    if (DSPTTK_STATUS_NONE == gstGlobalStatus.enXrayPs)
    {
        for (i = 0; i < pstShareParam->xrayChanCnt; i++)
        {
            sRet |= SendCmdToDsp(HOST_CMD_XRAY_PLAYBACK_START, i, NULL, &stPbParam);
        }
        if (sRet == SAL_SOK)
        {
            gstGlobalStatus.enXrayPs = DSPTTK_STATUS_PLAYBACK;
        }
        else
        {
            PRT_INFO("error ret[%d] HOST_CMD_XRAY_PLAYBACK_START enXrayPs %d\n", sRet, gstGlobalStatus.enXrayPs);
        }

        /* 申请回拉拼接内存  每次回拉过程只申请一次 */
        if (NULL == pstPbInfo->stPbPack.pu8PbSpliceDataSrc)
        {
            pstPbInfo->stPbPack.pu8PbSpliceDataSrc = (UINT8 *)malloc(NRAW_BYTE_SIZE(u32BufferWidth, u32BufferHeight, pstShareParam->dspCapbPar.xray_energy_num));
            if (NULL == pstPbInfo->stPbPack.pu8PbSpliceDataSrc)
            {
                PRT_INFO("NULL SRC\n");
            }
        }
        if (NULL == pstPbInfo->stPbPack.pu8PbSpliceDataDst)
        {
            pstPbInfo->stPbPack.pu8PbSpliceDataDst = (UINT8 *)malloc(NRAW_BYTE_SIZE(u32BufferWidth, u32BufferHeight, pstShareParam->dspCapbPar.xray_energy_num));
            if (NULL == pstPbInfo->stPbPack.pu8PbSpliceDataDst)
            {
                PRT_INFO("NULL DST\n");
            }
        }
        /* 申请save */
        if (NULL == pstPbInfo->stPbSave.pu8SaveDataSrc)
        {
            pstPbInfo->stPbSave.pu8SaveDataSrc = (UINT8 *)malloc(NRAW_BYTE_SIZE(u32BufferWidth, u32BufferHeight, pstShareParam->dspCapbPar.xray_energy_num));
            if (NULL == pstPbInfo->stPbSave.pu8SaveDataSrc)
            {
                PRT_INFO("errro pu8SaveDataSrc is null\n");
            }
        }
    }

    return CMD_RET_MIX(HOST_CMD_XRAY_PLAYBACK_START, sRet);
}


/**
 * @fn      dspttk_xray_playback_left
 * @brief   左向回拉
 *
 * @param   [IN] u32Chan 无效参数，实际不使用
 *
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
UINT64 dspttk_xray_playback_left(UINT32 u32Chan)
{
    UINT64 s64Ret = SAL_SOK;
    SAL_STATUS sRet = SAL_SOK;
    DSPTTK_XRAY_PROC_STATUS_E enXrayPs = gstGlobalStatus.enXrayPs;
    DSPTTK_XRAY_PB_STATUS_E enPbStatus = gstGlobalStatus.stPbInfo.enPbStatus;

    if (enXrayPs == DSPTTK_STATUS_RTPREVIEW || enXrayPs == DSPTTK_STATUS_RTPREVIEW_STOP)
    {
        if (SAL_FAIL == CMD_RET_OF(dspttk_xray_rtpreview_stop(u32Chan)))
        {
            PRT_INFO("set dspttk_xray_rtpreview_stop failed\n");
            return s64Ret;
        }
    }

    if (enPbStatus == DSPTTK_STATUS_PB_PACKAGE)
    {
        if (CMD_RET_OF(dspttk_xray_pb_package_start(u32Chan)) != SAL_SOK)
        {
            PRT_INFO("left pb_package_start failed\n");
            return s64Ret;
        }
        if (SAL_SOK != dspttk_xray_pb_one_package_write(u32Chan, XRAY_DIRECTION_LEFT)  )
        {
            PRT_INFO("left dspttk_xray_pb_one_package_write failed\n");
            return CMD_RET_MIX(HOST_CMD_XRAY_PLAYBACK_START, sRet);
        }
    }
    else if (enPbStatus == DSPTTK_STATUS_PB_SAVE)
    {
        if (CMD_RET_OF(dspttk_xray_pb_package_start(u32Chan)) != SAL_SOK)
        {
            PRT_INFO("left pb_package_start failed\n");
            return s64Ret;
        }
        if (SAL_SOK != dspttk_xray_pb_save_pic_write(u32Chan, XRAY_DIRECTION_LEFT))
        {
            PRT_INFO("left pb_package_start failed\n");
            return s64Ret;
        }
    }
    else if (enPbStatus == DSPTTK_STATUS_PB_SLICE)
    {
        s64Ret = dspttk_xray_playback_slice_one_shot(u32Chan, XRAY_DIRECTION_LEFT);
    }
    else
    {
        PRT_INFO("jump this status.\n");
    }

    return CMD_RET_MIX(s64Ret, sRet);
}


/**
 * @fn      dspttk_xray_playback_right
 * @brief   右向回拉
 * 
 * @param   [IN] u32Chan 无效参数，实际不使用
 * 
 * @return  UINT64 高32位表示命令号, 低32位表示错误号
 */
UINT64 dspttk_xray_playback_right(UINT32 u32Chan)
{
    UINT64 s64Ret = SAL_SOK;
    SAL_STATUS sRet = SAL_SOK;

    DSPTTK_XRAY_PROC_STATUS_E enXrayPs = gstGlobalStatus.enXrayPs;
    DSPTTK_XRAY_PB_STATUS_E enPbStatus = gstGlobalStatus.stPbInfo.enPbStatus;

    if (enXrayPs == DSPTTK_STATUS_RTPREVIEW || enXrayPs == DSPTTK_STATUS_RTPREVIEW_STOP)
    {
        if (SAL_FAIL == CMD_RET_OF(dspttk_xray_rtpreview_stop(u32Chan)))
        {
            PRT_INFO("set dspttk_xray_rtpreview_stop failed\n");
            return s64Ret;
        }
    }

    if (enPbStatus == DSPTTK_STATUS_PB_PACKAGE)
    {
        if (CMD_RET_OF(dspttk_xray_pb_package_start(u32Chan)) != SAL_SOK)
        {
            PRT_INFO("left pb_package_start failed\n");
            return s64Ret;
        }
        if (SAL_SOK != dspttk_xray_pb_one_package_write(u32Chan, XRAY_DIRECTION_RIGHT)  )
        {
            PRT_INFO("left dspttk_xray_pb_one_package_write failed\n");
            return CMD_RET_MIX(HOST_CMD_XRAY_PLAYBACK_START, sRet);
        }
    }
    else if (enPbStatus == DSPTTK_STATUS_PB_SAVE)
    {
        if (CMD_RET_OF(dspttk_xray_pb_package_start(u32Chan)) != SAL_SOK)
        {
            PRT_INFO("left pb_package_start failed\n");
            return s64Ret;
        }
        if (SAL_SOK != dspttk_xray_pb_save_pic_write(u32Chan, XRAY_DIRECTION_RIGHT))
        {
            PRT_INFO("left pb_package_start failed\n");
            return s64Ret;
        }
    }
    else if (enPbStatus == DSPTTK_STATUS_PB_SLICE)
    {
        s64Ret = dspttk_xray_playback_slice_one_shot(u32Chan, XRAY_DIRECTION_RIGHT);
    }
    else
    {
        PRT_INFO("jump this status.\n");
    }

    return CMD_RET_MIX(s64Ret, sRet);
}


/**
 * @fn      dspttk_xray_trans_simply
 * @brief   NRaw数据的便捷转存，无智能与图像增强，仅转换成指定格式的默认图像 
 * @warning 该接口需配合dspttk_xray_trans_free()一起使用，释放图片内存
 *
 * @param   [OUT] pImgBuf 存放图片数据的内存，使用完成后需调用dspttk_xray_trans_free()释放
 * @param   [IN] enImgType 图片格式
 * @param   [IN] pNRawBuf 需转存的NRaw数据
 * @param   [IN] u32NRawSize NRaw数据大小
 * @param   [IN] u32NRawWidth NRaw数据宽
 * @param   [IN] u32NRawHeight NRaw数据高
 *
 * @return  INT32 >0：图片数据大小，单位：字节；其他：转存失败
 */
INT32 dspttk_xray_trans_simply(VOID **pImgBuf, XRAY_TRANS_TYPE enImgType, VOID *pNRawBuf, UINT32 u32NRawSize, UINT32 u32NRawWidth, UINT32 u32NRawHeight)
{
    XRAY_TRANS_GET_BUFFER_SIZR_PARAM stTransBufAttr = {0};
    XRAY_TRANS_PROC_PARAM stTransParam = {0};

    if (NULL == pNRawBuf || NULL == pImgBuf)
    {
        PRT_INFO("pNRawBuf(%p) OR pImgBuf(%p) is NULL\n", pNRawBuf, pImgBuf);
        return -1;
    }

    if (0 == u32NRawSize || 0 == u32NRawWidth || 0 == u32NRawHeight || enImgType >= XRAY_TRANS_BUTT)
    {
        PRT_INFO("u32NRawSize(%u) OR u32NRawWidth(%u) OR u32NRawHeight(%u) OR enImgType(%d) is invalid\n", 
                 u32NRawSize, u32NRawWidth, u32NRawHeight, enImgType);
        return -1;
    }

    // 先获取转存后的图片需要的内存大小
    stTransBufAttr.enImgType = enImgType;
    stTransBufAttr.u32PackageWidth = u32NRawWidth;
    stTransBufAttr.u32PackageHeight = u32NRawHeight;
    if (0 == SendCmdToDsp(HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE, 0, NULL, &stTransBufAttr) && stTransBufAttr.u32ImgBufSize > 0)
    {
        // 申请相应大小的内存，并开始转存
        stTransParam.u32PackageWidth = u32NRawWidth;
        stTransParam.u32PackageHeight = u32NRawHeight;
        stTransParam.pu8RawBuf = pNRawBuf;
        stTransParam.u32RawSize = u32NRawSize;
        stTransParam.enImgType = enImgType;
        // stTransParam.enXspProcType = XSP_PROC_NONE;
        stTransParam.u32ImgBufSize = stTransBufAttr.u32ImgBufSize;
        stTransParam.pu8ImgBuf = (UINT8 *)malloc(stTransBufAttr.u32ImgBufSize);
        if (NULL == stTransParam.pu8ImgBuf)
        {
            PRT_INFO("malloc for 'pu8ImgBuf' failed, buffer size: %u\n", stTransBufAttr.u32ImgBufSize);
            return -1;
        }

        if (0 == SendCmdToDsp(HOST_CMD_XRAY_TRANS, 0, NULL, &stTransParam))
        {
            // 这里要做一个检查：判断命令HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE输出的预估图片大小是否小于了实际输出的图片大小，若存在，则必然有内存越界
            if (stTransParam.u32ImgDataSize > stTransBufAttr.u32ImgBufSize)
            {
                PRT_INFO("Memory out of bounds occured!! ImgDataSize(%u) is bigger than ImgBufSize(%u)\n", 
                         stTransParam.u32ImgDataSize, stTransBufAttr.u32ImgBufSize);
                free(stTransParam.pu8ImgBuf);
                return -1;
            }

            *pImgBuf = stTransParam.pu8ImgBuf;
            return stTransParam.u32ImgDataSize;
        }
        else
        {
            PRT_INFO("SendCmdToDsp HOST_CMD_XRAY_TRANS(0x%x) failed\n", HOST_CMD_XRAY_TRANS);
            free(stTransParam.pu8ImgBuf);
            return -1;
        }
    }
    else
    {
        PRT_INFO("SendCmdToDsp HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE(0x%x) failed\n", HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE);
        return -1;
    }
}


/**
 * @fn      dspttk_xray_trans_free
 * @brief   释放图片数据内存，配合dspttk_xray_trans_simply()一起使用
 * 
 * @param   [IN] pImgBuf 图片数据内存地址
 * 
 * @return  VOID 无
 */
VOID dspttk_xray_trans_free(VOID *pImgBuf)
{
    if (NULL != pImgBuf)
    {
        free(pImgBuf);
    }

    return;
}


/**
 * @fn      dspttk_get_global_trans_prm
 * @brief   获取转存全局结构体
 * 
 * @param   [IN] u32Chan XRay通道号
 * 
 * @return  [OUT] &gstTransInfo 全局结构体地址
 */
XRAY_PACKAGE_INFO *dspttk_get_global_trans_prm(UINT32 u32Chan)
{
    return &gstTransInfo[u32Chan];
}


/**
 * @fn      dspttk_get_current_trans_package_id
 * @brief   获取转存最新的包裹id
 * 
 * @param   [IN] u32Chan XRay通道号
 * 
 * @return  [OUT] u64PackageId 包裹ID号
 */
UINT64 dspttk_get_current_trans_package_id(UINT32 u32Chan)
{
    return gstTransInfo[u32Chan].u64CurPackageId;
}

/**
 * @fn      dspttk_get_first_trans_package_id
 * @brief   获取转存第一个包裹id
 * 
 * @param   [IN] u32Chan XRay通道号
 * 
 * @return  [OUT] u64FirstPackageId 包裹ID号
 */
UINT64 *dspttk_get_first_trans_package_id(UINT32 u32Chan)
{
    return &gstTransInfo[u32Chan].u64FirstPackageId;
}

/**
 * @fn      dspttk_xray_trans_package
 * @brief   转存包裹图像，转存的类型由Web页面选定，图像增强模式随机
 * 
 * @param   [IN] u32Chan XRay通道号
 * @param   [IN] u64PackageId 包裹ID号
 * 
 * @return  VOID 无
 */
VOID dspttk_xray_trans_package(UINT32 u32Chan)
{
    SAL_STATUS sRet = 0;
    UINT32 i = 0, j = 0;
    UINT32 u32SliceNum = 0;
    UINT64 u64PackageId = 0;
    DSPTTK_NODE *pNode = NULL;
    UINT8 *pu8SplSrcBuf = NULL;
    XRAY_PACKAGE_INFO *pstPackPrm = dspttk_get_global_trans_prm(u32Chan);
    UINT32 u32ReadSize = 0, u32ReadOffset = 0, u32Bufsize = 0;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    DSPTTK_SETTING_ENV *pstCfgEnv = &pstDevCfg->stSettingParam.stEnv;
    XRAY_TRANS_GET_BUFFER_SIZR_PARAM stTransBufAttr = {0};
    XRAY_TRANS_PROC_PARAM *pstTransParam = NULL; // 大结构体，使用malloc的方式申请堆内存
    DB_TABLE_PACKAGE stPackage = {0};
    DB_TABLE_SLICE *pstSlice = NULL;
    XRAY_NRAW_SPLICE_PARAM stSpliceParam = {0};
    CHAR aszImgPostfix[XRAY_TRANS_BUTT][8] = {"bmp", "jpg", "gif", "png", "tiff"};
    CHAR szImgFileName[256] = {0};
    // 转存包裹的图像增强模式，从下面数组中随机选择一种
    struct {
        CHAR sName[32];
        UINT32 u32XspProcType;
        XSP_RT_PARAMS stXspProcParam;
    } astXspProc[] = {
        {"default", 0x0, {{0, {0, 0, 0}}}},
        {"disp-color", 0x1, {.stDefaultStyle = {XSP_DISP_COLOR}}},
        {"disp-color", 0x1, {.stDefaultStyle = {XSP_DISP_GRAY}}},
        {"disp-color", 0x1, {.stDefaultStyle = {XSP_DISP_ORGANIC}}},
        {"disp-color", 0x1, {.stDefaultStyle = {XSP_DISP_ORGANIC_PLUS}}},
        {"disp-color", 0x1, {.stDefaultStyle = {XSP_DISP_INORGANIC}}},
        {"disp-color", 0x1, {.stDefaultStyle = {XSP_DISP_INORGANIC_PLUS}}},
        {"disp-color", 0x1, {.stDefaultStyle = {XSP_DISP_ORGANIC_ENHANCE}}},
        {"disp-color", 0x1, {.stDefaultStyle = {XSP_DISP_ORGANIC_ENHANCE_7}}},
        {"disp-color", 0x1, {.stDefaultStyle = {XSP_DISP_ORGANIC_ENHANCE_8}}},
        {"disp-color", 0x1, {.stDefaultStyle = {XSP_DISP_ORGANIC_ENHANCE_9}}},
        {"anti", 0x2, {.stAntiColor = {SAL_TRUE, 100}}},
        {"ee", 0x4, {.stEe = {SAL_TRUE, 50}}},
        {"ue", 0x8, {.stUe = {SAL_TRUE, 50}}},
        {"le", 0x10, {.stLe = {SAL_TRUE, 50}}},
        {"lp", 0x20, {.stLp = {SAL_TRUE, 50}}},
        {"hp", 0x40, {.stHp = {SAL_TRUE, 50}}},
        {"absor-0", 0x80, {.stVarAbsor = {SAL_TRUE, 0}}},
        {"absor-32", 0x80, {.stVarAbsor = {SAL_TRUE, 32}}},
        {"absor-63", 0x80, {.stVarAbsor = {SAL_TRUE, 63}}},
        {"lumi-25", 0x100, {.stLum = {SAL_TRUE, 25}}},
        {"lumi-50", 0x100, {.stLum = {SAL_TRUE, 50}}},
        {"pesudo-0", 0x200, {.stPseudoColor = {.pseudo_color = XSP_PSEUDO_COLOR_0}}},
        {"pesudo-1", 0x200, {.stPseudoColor = {.pseudo_color = XSP_PSEUDO_COLOR_1}}},
        {"color-table-0", 0x1000, {.stColorTable = {.enConfigueTable = XSP_COLOR_TABLE_0}}},
        {"color-table-1", 0x1000, {.stColorTable = {.enConfigueTable = XSP_COLOR_TABLE_1}}},
        {"color-table-2", 0x1000, {.stColorTable = {.enConfigueTable = XSP_COLOR_TABLE_2}}},
        {"color-table-3", 0x1000, {.stColorTable = {.enConfigueTable = XSP_COLOR_TABLE_3}}},
    };
    if (u32Chan >= pstShareParam->xrayChanCnt)
    {
        PRT_INFO("the u32Chan(%u) is invalid, range: [0, %u]\n", u32Chan, pstShareParam->xrayChanCnt);
        return;
    }

    pstTransParam = (XRAY_TRANS_PROC_PARAM *)malloc(sizeof(XRAY_TRANS_PROC_PARAM));
    if (NULL == pstTransParam)
    {
        PRT_INFO("chan %u, malloc for 'pstTransParam' failed, buffer size: %zu\n", u32Chan, sizeof(XRAY_TRANS_PROC_PARAM));
        return;
    }
    u32Bufsize = (UINT32)(pstDevCfg->stInitParam.stXray.xrayResW * pstDevCfg->stInitParam.stXray.packageLenMax * pstDevCfg->stInitParam.stXray.resizeFactor * 10);
    stSpliceParam.pSrcBuf = (UINT8 *)malloc(u32Bufsize);
    stSpliceParam.pDestBuf = (UINT8 *)malloc(u32Bufsize);
    if (NULL == stSpliceParam.pSrcBuf || NULL == stSpliceParam.pDestBuf || u32Bufsize == 0)
    {
        PRT_INFO("malloc for pSrcBuf(%p) OR pDestBuf(%p) failed, buffer size: %u\n", 
                stSpliceParam.pSrcBuf, stSpliceParam.pDestBuf, stSpliceParam.u32BufSize);
        return;
    }

    while (1)
    {
        dspttk_mutex_lock(&pstPackPrm->lstTranPack->sync.mid, 100, __FUNCTION__, __LINE__);
        dspttk_sem_take(&pstPackPrm->semEnTrans, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        pNode = dspttk_lst_get_head(pstPackPrm->lstTranPack);
        if (pNode != NULL)
        {
            u64PackageId = gstTransInfo[u32Chan].u64CurPackageId = *(UINT64 *)pNode->pAdData;
        }
        dspttk_mutex_unlock(&pstPackPrm->lstTranPack->sync.mid, __FUNCTION__, __LINE__);

        // 查询指定包裹ID对应的条带范围
        sRet = dspttk_query_package_by_packageid(u32Chan, &stPackage, u64PackageId);
        if (sRet != SAL_SOK)
        {
            continue;
        }

        // 查询指定条带范围内所有的条带数据及其信息
        sRet = dspttk_query_slice_by_range(u32Chan, &pstSlice, &u32SliceNum, stPackage.u64SliceStart, stPackage.u64SliceEnd);
        if (sRet != SAL_SOK || u32SliceNum == 0)
        {
            PRT_INFO("chan %u, dspttk_query_slice_by_range failed, sRet: %d, u32SliceNum: %u, slice range: [%llu, %llu]\n", 
                    u32Chan, sRet, u32SliceNum, stPackage.u64SliceStart, stPackage.u64SliceEnd);
            continue;
        }

        /***************** 拼接条带数据成整包NRaw *****************/
        // 读取各条带的信息，申请Buffer等
        u32ReadOffset = stSpliceParam.u32SliceOrder = stSpliceParam.u32BufSize = stSpliceParam.u32SrcHeightTotal = 0;
        stSpliceParam.u32SrcWidth = pstSlice[0].u32Width;
        stSpliceParam.u32SliceHeight = pstSlice[0].u32Height;
        for (i = 0; i < u32SliceNum; i++)
        {
            stSpliceParam.u32SrcHeightTotal += pstSlice[i].u32Height;
            stSpliceParam.u32BufSize += pstSlice[i].u32Size;
            // PRT_INFO("ht %u h %u\n", stSpliceParam.u32SrcHeightTotal, pstSlice[i].u32Height);
        }
        if (u32Bufsize < stSpliceParam.u32BufSize)
        {
            stSpliceParam.pSrcBuf = (UINT8 *)realloc(stSpliceParam.pSrcBuf, (stSpliceParam.u32BufSize - u32Bufsize) * sizeof(UINT8));
            stSpliceParam.pDestBuf = (UINT8 *)realloc(stSpliceParam.pDestBuf, (stSpliceParam.u32BufSize - u32Bufsize) * sizeof(UINT8));
        }
        /***************** 读取各条带的NRaw数据并拼接成整包NRaw *****************/
        pu8SplSrcBuf = stSpliceParam.pSrcBuf;
        for (i = 0; i < u32SliceNum; i++)
        {
            u32ReadSize = pstSlice[i].u32Size;
            sRet = dspttk_read_file(pstSlice[i].sPath, 0, pu8SplSrcBuf+u32ReadOffset, &u32ReadSize);
            if (SAL_SOK != sRet || u32ReadSize != pstSlice[i].u32Size)
            {
                PRT_INFO("dspttk_read_file %s failed, sRet: %d, ReadSize: %u, expect: %u\n", 
                        pstSlice[i].sPath, sRet, u32ReadSize, pstSlice[i].u32Size);
                continue;
            }

            u32ReadOffset += pstSlice[i].u32Size;
            // PRT_INFO("u32ReadOffset %d pstSlice[%d].u32Size %d\n", u32ReadOffset, i,  pstSlice[i].u32Size);
        }
        if (0 != SendCmdToDsp(HOST_CMD_XRAY_SPLICE_NRAW, 0, NULL, &stSpliceParam))
        {
            PRT_INFO("SendCmdToDsp HOST_CMD_XRAY_SPLICE_NRAW(0x%x) failed\n", HOST_CMD_XRAY_SPLICE_NRAW);
            continue;
        }

        if (stSpliceParam.u32SrcWidth != stPackage.u32Width || stSpliceParam.u32SrcHeightTotal != stPackage.u32Height)
        {
            PRT_INFO("Package info(%u X %u) is unmatched slices info(%u X %u) sliceNum %d\n", 
                    stPackage.u32Width, stPackage.u32Height, stSpliceParam.u32SrcWidth, stSpliceParam.u32SrcHeightTotal, u32SliceNum);
            continue;
        }

        /***************** 读取包裹信息 *****************/
        // 读取包裹的智能信息
        u32ReadSize = sizeof(XPACK_SVA_RESULT_OUT);
        sRet = dspttk_read_file(stPackage.sInfoPath, SAL_offsetof(XPACK_RESULT_ST, stPackSavPrm), 
                                (UINT8 *)&pstTransParam->stSvaInfo + SAL_offsetof(SVA_DSP_OUT, target_num), &u32ReadSize);
        if (SAL_SOK != sRet || u32ReadSize != sizeof(XPACK_SVA_RESULT_OUT))
        {
            PRT_INFO("dspttk_read_file %s failed, sRet: %d, ReadSize: %u, expect: %zu\n", 
                    stPackage.sInfoPath, sRet, u32ReadSize, sizeof(XPACK_SVA_RESULT_OUT));
            continue;
        }

        // 读取包裹的成像信息
        u32ReadSize = sizeof(XSP_PACK_INFO);
        sRet = dspttk_read_file(stPackage.sInfoPath, SAL_offsetof(XPACK_RESULT_ST, igmPrm), &pstTransParam->stXspInfo, &u32ReadSize);
        if (SAL_SOK != sRet || u32ReadSize != sizeof(XSP_PACK_INFO))
        {
            PRT_INFO("dspttk_read_file %s failed, sRet: %d, ReadSize: %u, expect: %zu\n", 
                    stPackage.sInfoPath, sRet, u32ReadSize, sizeof(XSP_PACK_INFO));
            continue;
        }
        pstTransParam->u32PackageWidth = stSpliceParam.u32SrcWidth;
        pstTransParam->u32PackageHeight = stSpliceParam.u32SrcHeightTotal;
        pstTransParam->pu8RawBuf = stSpliceParam.pDestBuf;
        pstTransParam->u32RawSize = stSpliceParam.u32BufSize;

        for (i = 0; i < XRAY_TRANS_BUTT; i++)
        {
            if (pstCfgXray->bTransTypeEn[i])
            {
                stTransBufAttr.enImgType = i;
                stTransBufAttr.u32PackageWidth = stSpliceParam.u32SrcWidth;
                stTransBufAttr.u32PackageHeight = stSpliceParam.u32SrcHeightTotal;
                if (0 == SendCmdToDsp(HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE, 0, NULL, &stTransBufAttr) && stTransBufAttr.u32ImgBufSize > 0)
                {
                    pstTransParam->enImgType = i;
                    // 随机一种图像增强
                    j = dspttk_get_random(0, SAL_arraySize(astXspProc)-1);
                    pstTransParam->unXspProcType.u32XspProcType = astXspProc[j].u32XspProcType;
                    /* 单能设置伪彩色双能设置颜色表 */
                    if ((pstTransParam->unXspProcType.u32XspProcType == 0x200) && 
                        pstShareParam->dspCapbPar.xray_energy_num == XRAY_ENERGY_DUAL)
                    {
                        pstTransParam->unXspProcType.u32XspProcType= 0x1000;
                    }
                    else if ((pstTransParam->unXspProcType.u32XspProcType == 0x1000) && 
                            pstShareParam->dspCapbPar.xray_energy_num == XRAY_ENERGY_SINGLE)
                    {
                        pstTransParam->unXspProcType.u32XspProcType= 0x200;
                    }
                    memset(&pstTransParam->stXspProcParam, 0x0, sizeof(XSP_RT_PARAMS));
                    memcpy(&pstTransParam->stXspProcParam, &astXspProc[j].stXspProcParam, sizeof(XSP_RT_PARAMS));

                    pstTransParam->u32ImgBufSize = stTransBufAttr.u32ImgBufSize;
                    pstTransParam->pu8ImgBuf = (UINT8 *)malloc(stTransBufAttr.u32ImgBufSize);
                    if (NULL == pstTransParam->pu8ImgBuf)
                    {
                        PRT_INFO("malloc for 'pu8ImgBuf' failed, buffer size: %u\n", stTransBufAttr.u32ImgBufSize);
                        continue;
                    }

                    if (0 == SendCmdToDsp(HOST_CMD_XRAY_TRANS, 0, NULL, pstTransParam))
                    {
                        if (pstTransParam->u32ImgDataSize <= stTransBufAttr.u32ImgBufSize)
                        {
                            // 图片的命名规格为：chan_packageId_width_height_sliceRange_
                            snprintf(szImgFileName, sizeof(szImgFileName), "%s/%u/%llu_w%u_h%u_sl%llu-%llu_t%llu_sva%u.%s", 
                                    pstCfgEnv->stOutputDir.packageTrans, u32Chan, stPackage.u64Id, stPackage.u32Width, stPackage.u32Height, 
                                    stPackage.u64SliceStart, stPackage.u64SliceEnd, SAL_SUB_SAFE(stPackage.u64TimeCb, stPackage.u64TimeEnd), 
                                    pstTransParam->stSvaInfo.target_num, aszImgPostfix[i]);
                            sRet = dspttk_write_file(szImgFileName, 0, SEEK_SET, pstTransParam->pu8ImgBuf, pstTransParam->u32ImgDataSize);
                            PRT_INFO("dspttk_write_file %s\n", szImgFileName);
                        }
                        else
                        {
                            PRT_INFO("Memory out of bounds occured!! ImgDataSize(%u) is bigger than ImgBufSize(%u)\n", 
                                    pstTransParam->u32ImgDataSize, stTransBufAttr.u32ImgBufSize);
                        }
                    }
                    else
                    {
                        PRT_INFO("SendCmdToDsp HOST_CMD_XRAY_TRANS(0x%x) failed\n", HOST_CMD_XRAY_TRANS);
                    }
                    free(pstTransParam->pu8ImgBuf);
                }
                else
                {
                    PRT_INFO("SendCmdToDsp HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE(0x%x) failed\n", HOST_CMD_XRAY_GET_TRANS_BUFFER_SIZE);
                }
            }
        }

        dspttk_mutex_lock(&pstPackPrm->lstTranPack->sync.mid, 100, __FUNCTION__, __LINE__);
        dspttk_lst_delete(pstPackPrm->lstTranPack, pNode);
        dspttk_mutex_unlock(&pstPackPrm->lstTranPack->sync.mid, __FUNCTION__, __LINE__);
        if (NULL != pstSlice)
        {
            dspttk_query_slice_free(pstSlice);
        }
    }

    return;
}


/**
 * @fn      dspttk_xray_rtpreview_fillin_blank
 * @brief   设置实时预览置白参数
 *
 * @param   [IN] u32Chan 通道号 范围[0, pstShareParam->xrayChanCnt-1]
 * @param   [IN]  隐含入参，XRAY_PARAM 置白参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xray_rtpreview_fillin_blank(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    if (u32Chan < pstShareParam->xrayChanCnt)
    {
        sRet = SendCmdToDsp(HOST_CMD_XRAY_SET_PARAM, u32Chan, NULL, &pstCfgXray->stFillinBlank[u32Chan]);
    }

    return CMD_RET_MIX(HOST_CMD_XRAY_SET_PARAM, sRet);
}


/**
 * @fn      dspttk_rtpreview_enhanced_scan
 * @brief   设置强扫
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ENHANCED_SCAN_PARAM 强扫设置的参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_rtpreview_enhanced_scan(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_ENHANCED_SCAN, 0, NULL, &pstCfgXray->stEnhancedScan);

    return CMD_RET_MIX(HOST_CMD_XSP_SET_ENHANCED_SCAN, sRet);
}


/**
 * @fn      dspttk_xsp_vertical_mirror
 * @brief   设置垂直（上下）镜像
 *
 * @param   [IN] u32Chan 通道号 范围[0, pstShareParam->xrayChanCnt-1]
 * @param   [IN]  隐含入参，XSP_VERTICAL_MIRROR_PARAM 垂直镜像参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_vertical_mirror(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    if (u32Chan < pstShareParam->xrayChanCnt)
    {
        sRet = SendCmdToDsp(HOST_CMD_XSP_SET_MIRROR, u32Chan, NULL, &pstCfgXray->stVMirror[u32Chan]);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_MIRROR, sRet);
}


/**
 * @fn      dspttk_xsp_color_table
 * @brief   设置颜色表（双能使用）
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_COLOR_TABLE_T 颜色表参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_color_table(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    // pstCfgXray->stColorTable.enConfigueTable = XSP_COLOR_TABLE_1;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_COLOR_TABLE, 0, NULL, &pstCfgXray->stColorTable.enConfigueTable);

    return CMD_RET_MIX(HOST_CMD_XSP_SET_COLOR_TABLE, sRet);
}


/**
 * @fn      dspttk_xsp_pseudo_color
 * @brief   设置伪彩（仅单能使用）
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_PSEUDO_COLOR_T 伪彩参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_pseudo_color(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    pstCfgXray->stPseudoColor.pseudo_color = !pstCfgXray->stPseudoColor.pseudo_color;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_PSEUDO_COLOR, 0, NULL, &pstCfgXray->stPseudoColor.pseudo_color);
    return CMD_RET_MIX(HOST_CMD_XSP_SET_PSEUDO_COLOR, sRet);
}


/**
 * @fn      dspttk_xsp_default_style
 * @brief   设置默认风格
 *
 * @param   [IN] u32Chan 通道号 不区分通道号 
 * @param   [IN]  隐含入参，XSP_DEFAULT_STYLE 默认风格参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_default_style(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    // pstCfgXray->stXspStyle.xsp_style = XSP_DEFAULT_STYLE_1;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_DEFAULT, 0, NULL, &pstCfgXray->stXspStyle.xsp_style);

    return CMD_RET_MIX(HOST_CMD_XSP_SET_DEFAULT, sRet);
}


/**
 * @fn      dspttk_xsp_set_alert_unpen
 * @brief   设置难穿透等并设置灵敏度
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ALERT_TYPE 默认可疑物参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_alert_unpen(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_UNPEN, 0, NULL, &pstCfgXray->stXspUnpen);

    return CMD_RET_MIX(HOST_CMD_XSP_SET_UNPEN, sRet);
}

/**
 * @fn      dspttk_xsp_set_alert_sus
 * @brief   设置可疑有机物并设置灵敏度
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ALERT_TYPE 默认可疑物参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_alert_sus(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_SUS_ALERT, 0, NULL, &pstCfgXray->stSusAlert);

    return CMD_RET_MIX(HOST_CMD_XSP_SET_SUS_ALERT, sRet);
}


/**
 * @fn      dspttk_xsp_set_tip
 * @brief   设置Tip插入
 * @brief   只有包裹时才会插入，且在包裹的起始条带开始插入，
 *          如果在文件夹中放入多个tip数据，会循环遍历文件夹下所有文件
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参 XSP_TIP_DATA_ST Tip参数  
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_tip(UINT32 u32Chan)
{
    #if 0
    INT32 sRet = SAL_FAIL, i = 0;
    UINT32 u32FileNum = 0, j = 0, u32FileSizeMax = 0, u32RawWidth = 0, u32RawHeight = 0, u32FileSize = 0;
    UINT8 *pTipDataBuf = NULL;
    UINT8 *pTipDataRiseBuf = NULL;   /*超分后的tip buf*/
    CHAR *pCutNameStr = NULL;
    CHAR *pRawTipFileName= NULL;
    CHAR *pRawTipBuf = NULL;
    FLOAT32 f32ResizeFactorH = 0, f32ResizeFactorW = 0;
    UINT64 *pu64TipDirBuf = NULL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XRAY_NRAW_RESIZE_PARAM stNrawResize = {0};
    DISP_SVA_SWITCH stSvaDispSwitch = {0};

    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    DSPTTK_SETTING_ENV *pstCfgEnv = &pstDevCfg->stSettingParam.stEnv;

    f32ResizeFactorH = pstShareParam->dspCapbPar.xsp_resize_factor.resize_height_factor;
    f32ResizeFactorW = pstShareParam->dspCapbPar.xsp_resize_factor.resize_width_factor;

    /* 关闭tip时关掉智能 */
    for (i = 0; i < pstShareParam->xrayChanCnt; i++)
    {
        stSvaDispSwitch.uidevchn = i;
        stSvaDispSwitch.dispSvaSwitch = (pstCfgXray->stTipSet.stTipData.uiEnable == SAL_FALSE) ?  SAL_TRUE : SAL_FALSE;
        stSvaDispSwitch.chn = i;

        sRet = SendCmdToDsp(HOST_CMD_SET_AI_DISP_SWITCH, 0, NULL, &stSvaDispSwitch);
        if (sRet == SAL_FAIL)
        {
            PRT_INFO("chan [%d] HOST_CMD_SET_AI_DISP_SWITCH failed uiChn %d bSwitch %d DispChn %d\n", 
                        i, stSvaDispSwitch.uidevchn, stSvaDispSwitch.dispSvaSwitch, stSvaDispSwitch.chn);
        }
    }

    if(pstCfgXray->stTipSet.stTipData.uiEnable == SAL_FALSE)
    {
        sRet = SendCmdToDsp(HOST_CMD_XSP_SET_TIP, 0, NULL, &pstCfgXray->stTipSet);
        if (sRet == SAL_SOK)
        {
            PRT_INFO("cancel insert successful with tip.\n");
            return SAL_SOK;
        }
        else
        {
            PRT_INFO("cancel insert failed with tip.\n");
            return SAL_FAIL;
        }
    }

    u32FileNum = dspttk_get_file_num(pstCfgEnv->stInputDir.tipNraw);
    if (u32FileNum == 0)
    {
        PRT_INFO("get file num is %d.pls send data to [datain/xraw/tip/]\n", u32FileNum);
        goto EXIT;
    }
    /* 获取文件夹路径 通过动态数组申请*/
    pu64TipDirBuf = (UINT64 *)malloc(u32FileNum * (FILE_PATH_STRLEN_MAX + sizeof(UINT64*)));
    if (pu64TipDirBuf == NULL)
    {
        PRT_INFO("pu64TipDirBuf malloc failed.\n");
        goto EXIT;
    }

    for (i = 0; i < u32FileNum; i++)
    {
        /* 通过以下操作实现动态数组的分配的存储
        * pu64DirBuf内部结构分为头+数据段 每个头（UINT64类型）存储着唯一的数据段的首地址 数据段位（CHAR）存放对应的路径 e.x. [datain/xraw/1/full_w1024_o0.raw]
        *   |   |   |   |   |   |-------------------|---------------------|-------------------------|-----------------/....
        * \---------数据头------/ \--------------------------------数据段---------------------------------------------
        */
        *(pu64TipDirBuf + i) = (UINT64)((UINT8* )pu64TipDirBuf + sizeof(UINT64*) * u32FileNum + FILE_PATH_STRLEN_MAX  * i);
    }

    sRet = dspttk_get_file_name_list(pstCfgEnv->stInputDir.tipNraw, (char **)pu64TipDirBuf, &u32FileNum, &u32FileSizeMax);
    if (SAL_FAIL == sRet || u32FileSizeMax == 0)
    {
        PRT_INFO("dspttk_get_dir_name_list error num %d dir %s FileSizeMax %d.\n", u32FileNum, pstCfgEnv->stInputDir.tipNraw, u32FileSizeMax);
    }

    pTipDataBuf = (UINT8 *)malloc(u32FileSizeMax * sizeof(CHAR));
    if (pTipDataBuf == NULL)
    {
        PRT_INFO("pTipBuff malloc failed %p.\n", pTipDataBuf);
        goto EXIT;
    }

    if (f32ResizeFactorH != 1.0 || f32ResizeFactorW != 1.0) /* 超分时TIP NRAW 数据需要适配到超分后尺寸 */
    {
        pTipDataRiseBuf = (UINT8 *)malloc((float)u32FileSizeMax * f32ResizeFactorH * f32ResizeFactorW);
        if (pTipDataRiseBuf == NULL)
        {
            PRT_INFO("pTipDataRiseBuf malloc failed %p.\n", pTipDataRiseBuf);
            goto EXIT;
        }
    }

    for (i = 0; i < u32FileNum; i++)
    {
        for (j = 0; j < pstShareParam->xrayChanCnt; j++)
        {
            pRawTipFileName = (char *)(*(pu64TipDirBuf + i)); //取出buf中的文件名q
            u32FileSize = (INT32)dspttk_get_file_size(pRawTipFileName);
            if (u32FileSize <= 0)
            {
                PRT_INFO("get %s file size failed! \n", pRawTipFileName);
                return SAL_FAIL;
            }
            if (SAL_FAIL == dspttk_read_file(pRawTipFileName, 0, pTipDataBuf, &u32FileSize))
            {
                PRT_INFO("dspttk_read_file read failed %p size %d tipfile[%d] %p.\n", pTipDataBuf, u32FileSize, i, pRawTipFileName);
                goto EXIT;
            }

            pCutNameStr = strstr(pRawTipFileName, "_w");  // 将字符串截取到“_w”出现的位置
            sRet = sscanf(pCutNameStr, "%*[^w]w%u[^_]", &u32RawWidth);
            sRet |= sscanf(pCutNameStr, "%*[^h]h%u[^.]", &u32RawHeight);
            if(sRet == SAL_FAIL)
            {
                PRT_INFO("w %d h %d str %s \n", u32RawWidth, u32RawHeight, pCutNameStr);
                goto EXIT;
            }

            if (f32ResizeFactorH != 1.0 || f32ResizeFactorW != 1.0) /* 超分时TIP NRAW 数据需要适配到超分后尺寸 */
            {
                stNrawResize.pSrcBuf = pTipDataBuf;
                stNrawResize.u32SrcW = u32RawWidth;
                stNrawResize.u32SrcH = u32RawHeight;
                stNrawResize.pDestBuf = pTipDataRiseBuf;
                stNrawResize.u32DstW = f32ResizeFactorW * u32RawWidth;
                stNrawResize.u32DstH = f32ResizeFactorH * u32RawHeight;
                sRet = SendCmdToDsp(HOST_CMD_XRAY_RESIZE_NRAW, 0, NULL, &stNrawResize);
                if (sRet != SAL_SOK)
                {
                    PRT_INFO("xraw_resize failed raw_w %d raw_h %d factor_w %f factor_h %f.\n", u32RawWidth, u32RawHeight, f32ResizeFactorW, f32ResizeFactorH);
                    goto EXIT;
                }
                pstCfgXray->stTipSet.stTipData.stTipNormalized[j].pXrayBuf = (unsigned short *)pTipDataRiseBuf;
                pstCfgXray->stTipSet.stTipData.stTipNormalized[j].uiBufLen = u32FileSize * (UINT32)(f32ResizeFactorW * f32ResizeFactorH);
            }
            else /* 非超分情况下直接使用原始数据 */
            {
                pstCfgXray->stTipSet.stTipData.stTipNormalized[j].pXrayBuf = (unsigned short *)pTipDataBuf;
                pstCfgXray->stTipSet.stTipData.stTipNormalized[j].uiBufLen = u32FileSize;
            }

            pstCfgXray->stTipSet.stTipData.stTipNormalized[j].uiXrayW = (UINT32)((float)u32RawWidth * f32ResizeFactorW);
            pstCfgXray->stTipSet.stTipData.stTipNormalized[j].uiXrayH = (UINT32)((float)u32RawHeight * f32ResizeFactorH);
        }
        sRet = dspttk_sem_take(&pstCfgXray->stTipSet.semSetTip[u32Chan], SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        DSPTTK_CHECK_RET(sRet, SAL_FAIL);
        sRet = SendCmdToDsp(HOST_CMD_XSP_SET_TIP, 0, NULL, &pstCfgXray->stTipSet.stTipData);
        if(sRet == SAL_FAIL)
        {
            PRT_INFO("set tip failed chan %d enable %d time %llu scan %d xray-buf %p buf-len %d w %d h %d\n", \
                        u32Chan, pstCfgXray->stTipSet.stTipData.uiEnable, \
                        pstCfgXray->stTipSet.stTipData.uiTime, pstCfgXray->stTipSet.stTipData.uiPackScan, \
                        pstCfgXray->stTipSet.stTipData.stTipNormalized->pXrayBuf, \
                        pstCfgXray->stTipSet.stTipData.stTipNormalized->uiBufLen, \
                        pstCfgXray->stTipSet.stTipData.stTipNormalized[u32Chan].uiXrayW, \
                        pstCfgXray->stTipSet.stTipData.stTipNormalized[u32Chan].uiXrayH);
        }

        /* tip停止则跳出循环 */
        if(pstCfgXray->stTipSet.stTipData.uiEnable == SAL_FALSE)
        {
            break;
        }

        // dspttk_msleep(4000);
        i = (i == u32FileNum - 1) ? -1 : i;

    }

EXIT:
    if (pTipDataBuf != NULL)
    {
        free(pTipDataBuf);
    }

    if (pu64TipDirBuf != NULL)
    {
        free(pu64TipDirBuf);
    }

    if (pRawTipBuf != NULL)
    {
        free(pRawTipBuf);
    }
    
    return CMD_RET_MIX(HOST_CMD_XSP_SET_TIP, sRet);
    #else
    return CMD_RET_MIX(HOST_CMD_XSP_SET_TIP, SAL_FAIL);
    #endif
}


/**
 * @fn      dspttk_xsp_set_bkg
 * @brief   设置背景阈值
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_BKG_PARAM 背景阈值参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_bkg(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_BKG, 0, NULL, &pstCfgXray->stBkg);

    return CMD_RET_MIX(HOST_CMD_XSP_SET_BKG, sRet);
}


/**
 * @fn      dspttk_xsp_set_default_enhance
 * @brief   设置默认增强系数
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ORIGINAL_MODE_PARAM 设置默认增强系数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_default_enhance(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    // pstCfgXray->stDefaultEnhance.uiLevel = 50;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_ORIGINAL, 0, NULL, &pstCfgXray->stDefaultEnhance);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_ORIGINAL failed level[%d]\n", pstCfgXray->stDefaultEnhance.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_ORIGINAL, sRet);
}


/**
 * @fn      dspttk_xsp_set_contrast
 * @brief   设置对比度
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_CONTRAST_PARAM 设置对比度参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_contrast(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_CONTRAST, 0, NULL, &pstCfgXray->stContrast);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_CONTRAST failed level[%d]\n", pstCfgXray->stContrast.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_CONTRAST, sRet);
}


/**
 * @fn      dspttk_xsp_set_deformity_correction
 * @brief   设置畸变校正
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DEFORMITY_CORRECTION 畸变校正参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_deformity_correction(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_DEFORMITY_CORRECTION, 0, NULL, &pstCfgXray->stDeformityCorrection);
    PRT_INFO("SET HOST_CMD_XSP_SET_DEFORMITY_CORRECTION is %s prm[%d]\n", sRet == SAL_FAIL ? "failed" : "successful", pstCfgXray->stColorImage.enColorsImaging);

    return CMD_RET_MIX(HOST_CMD_XSP_SET_DEFORMITY_CORRECTION, sRet);
}


/**
 * @fn      dspttk_xsp_set_imaging_param
 * @brief   设置三色六色显示
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DEFORMITY_CORRECTION 畸变校正参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_imaging_param(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_COLORS_IMAGING, 0, NULL, &pstCfgXray->stColorImage);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_COLORS_IMAGING failed level[%d]\n", pstCfgXray->stColorImage.enColorsImaging);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_DEFORMITY_CORRECTION, sRet);
}


/**
 * @fn      dspttk_xsp_set_color_adjust
 * @brief   设置冷暖色调调节
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_COLOR_ADJUST_PARAM 冷暖色调调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_color_adjust(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    sRet = SendCmdToDsp(HOST_CMD_XSP_COLOR_ADJUST, 0, NULL, &pstCfgXray->stColorAdjust);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_COLOR_ADJUST failed level[%d]\n", pstCfgXray->stColorAdjust.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_COLOR_ADJUST, sRet);
}


/**
 * @fn      dspttk_xsp_set_anti
 * @brief   设置反色
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ANTI_COLOR_PARAM 反色调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_anti(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    pstXspPicPrm->stAntiColor.bEnable = !pstXspPicPrm->stAntiColor.bEnable;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_ANTI, 0, NULL, &pstXspPicPrm->stAntiColor);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_ANTI failed level[%d]\n", pstXspPicPrm->stAntiColor.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_ANTI, sRet);
}


/**
 * @fn      dspttk_xsp_set_z789
 * @brief   设置原子序数Z789
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DISP_MODE 原子序数Z789 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_z789(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    if (pstXspPicPrm->stDispMode.disp_mode < XSP_DISP_ORGANIC_ENHANCE || pstXspPicPrm->stDispMode.disp_mode > XSP_DISP_ORGANIC_ENHANCE_9)
    {
        pstXspPicPrm->stDispMode.disp_mode = XSP_DISP_ORGANIC_ENHANCE;
    }

    pstXspPicPrm->stDispMode.disp_mode = (pstXspPicPrm->stDispMode.disp_mode + 1) % 10;
    if (pstXspPicPrm->stDispMode.disp_mode == 0) 
    {
        pstXspPicPrm->stDispMode.disp_mode = XSP_DISP_ORGANIC_ENHANCE;
    }

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_DISP, 0, NULL, &pstXspPicPrm->stDispMode.disp_mode);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_DISP failed level[%d]\n", pstXspPicPrm->stDispMode.disp_mode);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_DISP, sRet);
}


/**
 * @fn      dspttk_xsp_set_lp_or_hp
 * @brief   设置高穿低穿
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_HP_PARAM XSP_LP_PARAM 高穿低穿 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_lp_or_hp(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    BOOL bPenetrate = SAL_FALSE;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;
    bPenetrate = !bPenetrate;

    if (!bPenetrate)
    {
        pstXspPicPrm->stHp.bEnable = 1;
        pstXspPicPrm->stHp.uiLevel = 100;
        sRet = SendCmdToDsp(HOST_CMD_XSP_SET_HP, 0, NULL, &pstXspPicPrm->stHp);
        pstXspPicPrm->stLp.bEnable = 0;
        pstXspPicPrm->stLp.uiLevel = 0;
        sRet |= SendCmdToDsp(HOST_CMD_XSP_SET_LP, 0, NULL, &pstXspPicPrm->stLp);
        if (sRet == SAL_FAIL)
        {
            PRT_INFO("HOST_CMD_XSP_SET_LP[%d] or HOST_CMD_XSP_SET_HP failed level[%d]\n", pstXspPicPrm->stLp.uiLevel, pstXspPicPrm->stHp.uiLevel);
        }
    }
    else
    {
        pstXspPicPrm->stHp.bEnable = 0;
        pstXspPicPrm->stHp.uiLevel = 0;
        sRet = SendCmdToDsp(HOST_CMD_XSP_SET_HP, 0, NULL, &pstXspPicPrm->stHp);
        pstXspPicPrm->stLp.bEnable = 1;
        pstXspPicPrm->stLp.uiLevel = 100;
        sRet |= SendCmdToDsp(HOST_CMD_XSP_SET_LP, 0, NULL, &pstXspPicPrm->stLp);
        if (sRet == SAL_FAIL)
        {
            PRT_INFO("HOST_CMD_XSP_SET_LP[%d] or HOST_CMD_XSP_SET_HP failed level[%d]\n", pstXspPicPrm->stLp.uiLevel, pstXspPicPrm->stHp.uiLevel);
        }
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_HP, sRet);
}


/**
 * @fn      dspttk_xsp_set_gray_or_color
 * @brief   设置彩色或者黑白
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DISP_MODE 彩色/黑彩 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_gray_or_color(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    pstXspPicPrm->stDispMode.disp_mode = !pstXspPicPrm->stDispMode.disp_mode;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_DISP, 0, NULL, &pstXspPicPrm->stDispMode);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_DISP failed level[%d]\n", pstXspPicPrm->stDispMode.disp_mode);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_DISP, sRet);
}


/**
 * @fn      dspttk_xsp_set_organic
 * @brief   设置有机物剔除
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DISP_MODE 有机物剔除 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_organic(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;

    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    pstXspPicPrm->stDispMode.disp_mode = (pstXspPicPrm->stDispMode.disp_mode == XSP_DISP_ORGANIC) ? XSP_DISP_COLOR : XSP_DISP_ORGANIC;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_DISP, 0, NULL, &pstXspPicPrm->stDispMode);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_DISP failed level[%d]\n", pstXspPicPrm->stDispMode.disp_mode);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_DISP, sRet);
}


/**
 * @fn      dspttk_xsp_set_default_mode
 * @brief   设置恢复默认
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_default_mode(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL, i = 0;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    struct {
        HOST_CMD enCmd;
        XSP_ENABLE_A_LEVEL stEnAndLevel;
    } astXspProc[] = {
        {HOST_CMD_XSP_SET_ANTI, {SAL_FALSE, 0}},
        {HOST_CMD_XSP_SET_EE,{SAL_FALSE, 0}},
        {HOST_CMD_XSP_SET_UE, {SAL_FALSE, 0}},
        {HOST_CMD_XSP_SET_LE, {SAL_FALSE, 0}},
        {HOST_CMD_XSP_SET_LP, {SAL_FALSE, 0}},
        {HOST_CMD_XSP_SET_HP, {SAL_FALSE, 0}},
        {HOST_CMD_XSP_SET_ABSOR, {SAL_TRUE, 32}},
        {HOST_CMD_XSP_SET_LUMINANCE, {SAL_TRUE, 50}},
    };

    for (i = 0; i < SAL_arraySize(astXspProc); i++)
    {
        sRet = SendCmdToDsp(astXspProc[i].enCmd, 0, NULL, &astXspProc[i].stEnAndLevel);
    }
    pstXspPicPrm->stDefaultStyle.xsp_style = XSP_DEFAULT_STYLE_0;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_DEFAULT, 0, NULL, &pstXspPicPrm->stDefaultStyle.xsp_style);
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_PSEUDO_COLOR, 0, NULL, &pstXspPicPrm->stPseudoColor.pseudo_color);
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_COLOR_TABLE, 0, NULL, &pstXspPicPrm->stColorTable.enConfigueTable);

    return CMD_RET_MIX(HOST_CMD_XSP_SET_DEFAULT, sRet);
}


/**
 * @fn      dspttk_xsp_set_inorganic
 * @brief   设置无机物剔除
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_DISP_MODE 有机物剔除 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_inorganic(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    pstXspPicPrm->stDispMode.disp_mode = (pstXspPicPrm->stDispMode.disp_mode == XSP_DISP_INORGANIC) ? 
                                                                XSP_DISP_COLOR : XSP_DISP_INORGANIC;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_DISP, 0, NULL, &pstXspPicPrm->stDispMode);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_LE failed level[%d]\n", pstXspPicPrm->stLe.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_DISP, sRet);
}


/**
 * @fn      dspttk_xsp_set_le
 * @brief   设置局增
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_LE_PARAM 局增 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_le(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    pstXspPicPrm->stLe.bEnable = !pstXspPicPrm->stLe.bEnable;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_LE, 0, NULL, &pstXspPicPrm->stLe);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_LE failed level[%d]\n", pstXspPicPrm->stLe.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_LE, sRet);
}


/**
 * @fn      dspttk_xsp_set_absorptance_add
 * @brief   设置吸收率增加
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_VAR_ABSOR_PARAM 吸收率 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_absorptance_add(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    /* 首次设置需要从中间值开始 */
    pstXspPicPrm->stVarAbsor.uiLevel = (pstXspPicPrm->stVarAbsor.uiLevel == 0) ?
                                        30 : pstXspPicPrm->stVarAbsor.uiLevel;
    if (pstXspPicPrm->stVarAbsor.uiLevel < 60) /* 可变吸收率范围(0-64) */
    {
        pstXspPicPrm->stVarAbsor.bEnable = SAL_TRUE;
        pstXspPicPrm->stVarAbsor.uiLevel += 3;
    }
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_ABSOR, 0, NULL, &pstXspPicPrm->stVarAbsor);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_ABSOR failed level[%d]\n", pstXspPicPrm->stVarAbsor.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_ABSOR, sRet);
}


/**
 * @fn      dspttk_xsp_set_absorptance_sub
 * @brief   设置吸收率减少
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_VAR_ABSOR_PARAM 吸收率 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_absorptance_sub(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    /* 首次设置需要从中间值开始 */
    pstXspPicPrm->stVarAbsor.uiLevel = (pstXspPicPrm->stVarAbsor.uiLevel == 0) ?
                                        30 : pstXspPicPrm->stVarAbsor.uiLevel;
    if (pstXspPicPrm->stVarAbsor.uiLevel > 3) /* 可变吸收率范围(0-64) */
    {
        pstXspPicPrm->stVarAbsor.bEnable = SAL_TRUE;
        pstXspPicPrm->stVarAbsor.uiLevel -= 3;
    }
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_ABSOR, 0, NULL, &pstXspPicPrm->stVarAbsor);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_ABSOR failed level[%d]\n", pstXspPicPrm->stVarAbsor.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_ABSOR, sRet);
}


/**
 * @fn      dspttk_xsp_set_ee
 * @brief   设置边增
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_EE_PARAM 边增 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_ee(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    pstXspPicPrm->stEe.bEnable = !pstXspPicPrm->stUe.bEnable;
    pstXspPicPrm->stEe.uiLevel = pstXspPicPrm->stUe.bEnable == SAL_TRUE ? 100 : 0;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_EE, 0, NULL, &pstXspPicPrm->stEe);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_EE failed level[%d]\n", pstXspPicPrm->stEe.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_EE, sRet);
}


/**
 * @fn      dspttk_xsp_set_ue
 * @brief   设置超增
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_UE_PARAM 超增 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_ue(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    pstXspPicPrm->stUe.bEnable = !pstXspPicPrm->stUe.bEnable;
    pstXspPicPrm->stUe.uiLevel = pstXspPicPrm->stUe.bEnable == SAL_TRUE ? 100 : 0;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_UE, 0, NULL, &pstXspPicPrm->stUe);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_UE failed level[%d]\n", pstXspPicPrm->stUe.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_UE, sRet);
}


/**
 * @fn      dspttk_xsp_set_lum_add
 * @brief   设置亮度增加
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_LUMINANCE_PARAM 亮度参数 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_lum_add(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    pstXspPicPrm->stLum.bEnable = SAL_TRUE;
    if (pstXspPicPrm->stLum.uiLevel < 100)
    {
        pstXspPicPrm->stLum.uiLevel += 10;
    }
    else
    {
        pstXspPicPrm->stLum.uiLevel = 100;
    }
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_LUMINANCE, 0, NULL, &pstXspPicPrm->stLum);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_LUMINANCE failed level[%d]\n", pstXspPicPrm->stLum.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_LUMINANCE, sRet);
}


/**
 * @fn      dspttk_xsp_set_lum_sub
 * @brief   设置亮度减少
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_LUMINANCE_PARAM 亮度参数 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_lum_sub(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    pstXspPicPrm->stLum.bEnable = SAL_TRUE;
    pstXspPicPrm->stLum.uiLevel = SAL_SUB_SAFE(pstXspPicPrm->stLum.uiLevel, 10);
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_LUMINANCE, 0, NULL, &pstXspPicPrm->stLum);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("HOST_CMD_XSP_SET_LUMINANCE failed level[%d]\n", pstXspPicPrm->stLum.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_LUMINANCE, sRet);
}


/**
 * @fn      dspttk_xsp_set_original_mode
 * @brief   设置原始显示
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_ORIGINAL_MODE_PARAM 原始显示 调节参数
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_original_mode(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPINITPARA *pstShareParam = dspttk_get_share_param();
    XSP_RT_PARAMS *pstXspPicPrm = &pstShareParam->stXspPicPrm;

    pstXspPicPrm->stOriginalMode.bEnable = !pstXspPicPrm->stOriginalMode.bEnable;
    pstXspPicPrm->stOriginalMode.uiLevel = pstXspPicPrm->stOriginalMode.bEnable == SAL_TRUE ? 100 : 0;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_ORIGINAL, 0, NULL, &pstXspPicPrm->stOriginalMode);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("set HOST_CMD_XSP_SET_ORIGINAL error level[%d] en[%d].\n", 
                    pstXspPicPrm->stOriginalMode.uiLevel, pstXspPicPrm->stOriginalMode.bEnable);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_ORIGINAL, sRet);
}


/**
 * @fn      dspttk_xsp_set_rm_blank_slice
 * @brief   设置空白条带移除
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，XSP_SET_RM_BLANK_SLICE 包裹后的空白条带控制结构体
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_rm_blank_slice(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_BLANK_SLICE, 0, NULL, &pstCfgXray->stRmBlankSlice);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("set HOST_CMD_XSP_SET_BLANK_SLICE error en[%d] level[%d].\n", pstCfgXray->stRmBlankSlice.bEnable, pstCfgXray->stRmBlankSlice.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_BLANK_SLICE, sRet);
}


/**
 * @fn      dspttk_xsp_set_package_divide_method
 * @brief   设置包裹分割方式
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，HOST_CMD_XSP_PACKAGE_DIVIDE 包包裹分割方式结构体
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_package_divide_method(UINT32 u32Chan)
{
    INT32 sRet = SAL_FAIL;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    sRet = SendCmdToDsp(HOST_CMD_XSP_PACKAGE_DIVIDE, 0, NULL, &pstCfgXray->stPackagePrm);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("set HOST_CMD_XSP_PACKAGE_DIVIDE error method [%d].\n", pstCfgXray->stPackagePrm.enDivMethod);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_PACKAGE_DIVIDE, sRet);
}


/**
 * @fn      dspttk_xsp_set_stretch
 * @brief   设置拉伸模式，依靠算法内部实现，
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，对应结构体 XSP_STRETCH_MODE_PARAM 
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_stretch(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    pstCfgXray->stretchParam.bEnable = SAL_TRUE;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_STRETCH, u32Chan, NULL, (void *)&pstCfgXray->stretchParam);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("set HOST_CMD_XSP_SET_STRETCH error en[%d] level[%d].\n", pstCfgXray->stretchParam.bEnable, pstCfgXray->stretchParam.uiLevel);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_STRETCH, sRet);
}


/**
 * @fn      dspttk_xsp_set_bkg_color
 * @brief   设置显示图像背景色
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，结构体XSP_BKG_COLOR
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_bkg_color(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    UINT32 u32Argb = 0, u32Yuv = 0;
    char cColorStr[16] = {0};
    XSP_BKG_COLOR stXspBkgColor = {0};
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;
    stXspBkgColor.enDataFmt = pstCfgXray->u32BkgColorFormat ? DSP_IMG_DATFMT_YUV420SP_VU : DSP_IMG_DATFMT_BGR24;
    if (pstCfgXray->cBkgColorValue[0] == '#') 
    {
        strcpy(cColorStr, "ff");     // 先添加 ff 前缀
        strcat(cColorStr, &pstCfgXray->cBkgColorValue[1]);  // 再拼接剩余部分
    }
    else
    {
        strcpy(cColorStr, "ff");     // 先添加 FF 前缀
        strcat(cColorStr, &pstCfgXray->cBkgColorValue[0]);  // 再拼接剩余部分
    }
    u32Argb = strtoul(cColorStr, NULL, 16);
    if (stXspBkgColor.enDataFmt == DSP_IMG_DATFMT_YUV420SP_VU)
    {
        unsigned char r = (u32Argb >> 16) & 0xFF;
        unsigned char g = (u32Argb >> 8) & 0xFF;
        unsigned char b = u32Argb & 0xFF;

        unsigned char y = SAL_CLIP((unsigned char)(0.299 * r + 0.587 * g + 0.114 * b), 16, 235);
        unsigned char u = SAL_CLIP((unsigned char)(-0.169 * r - 0.331 * g + 0.5 * b + 128), 16, 235);
        unsigned char v = SAL_CLIP((unsigned char)(0.5 * r - 0.419 * g - 0.081 * b + 128), 16, 235);
        u32Yuv = 0xFF000000 | (y << 16) | (u << 8) | v;
        stXspBkgColor.uiBkgValue = u32Yuv;
    }
    else
    {
        stXspBkgColor.uiBkgValue = u32Argb;
    }

    PRT_INFO("fmt %s uiBkgValue %x\n", pstCfgXray->u32BkgColorFormat ? "YUV":"ARGB", stXspBkgColor.uiBkgValue);
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_BKGCOLOR, 0, NULL, (void *)&stXspBkgColor);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("set HOST_CMD_XSP_SET_BKGCOLOR error fmt[%d].\n", stXspBkgColor.enDataFmt);
    }

    return CMD_RET_MIX(HOST_CMD_XSP_SET_BKGCOLOR, sRet);
}


/**
 * @fn      dspttk_xsp_set_gamma
 * @brief   设置gamma参数 此参数会影响亮度
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，结构体 XSP_GAMMA_PARAM
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_gamma(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    pstCfgXray->stXspGamma.bEnable = SAL_TRUE;
    // pstCfgXray->stXspGamma.uiLevel = 50;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_GAMMA, u32Chan, NULL, (void *)&pstCfgXray->stXspGamma);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("set HOST_CMD_XSP_SET_GAMMA error en[%d] level[%d].\n", pstCfgXray->stXspGamma.bEnable, pstCfgXray->stXspGamma.uiLevel);
    }
    
    return CMD_RET_MIX(HOST_CMD_XSP_SET_GAMMA, sRet);
}


/**
 * @fn      dspttk_xsp_set_aixsp_rate
 * @brief   设置AIXSP和传统XSP权重
 *
 * @param   [IN] u32Chan 通道号 不区分通道号
 * @param   [IN]  隐含入参，结构体XSP_AIXSP_RATE_PARAM
 *
 * @return  UINT64 高32位表示命令号，低32位表示错误号
 */
UINT64 dspttk_xsp_set_aixsp_rate(UINT32 u32Chan)
{
    INT32 sRet = SAL_SOK;
    DSPTTK_DEVCFG *pstDevCfg = dspttk_devcfg_get();
    DSPTTK_SETTING_XRAY *pstCfgXray = &pstDevCfg->stSettingParam.stXray;

    pstCfgXray->stXspAixsprate.bEnable = SAL_TRUE;
    // pstCfgXray->stXspAixsprate.uiLevel = 100;
    sRet = SendCmdToDsp(HOST_CMD_XSP_SET_AIXSP_RATE, u32Chan, NULL, (void *)&pstCfgXray->stXspAixsprate);
    if (sRet == SAL_FAIL)
    {
        PRT_INFO("set HOST_CMD_XSP_SET_AIXSP_RATE error en[%d] level[%d].\n", 
                    pstCfgXray->stXspAixsprate.bEnable, pstCfgXray->stXspAixsprate.uiLevel);
    }
    
    return CMD_RET_MIX(HOST_CMD_XSP_SET_AIXSP_RATE, sRet);
}