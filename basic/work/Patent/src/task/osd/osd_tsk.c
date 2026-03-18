/**
 * @file   osd_tsk.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  osd 模块 tsk 层
 * @author heshengyuan
 * @date   2014年10月28日 Create
 * @note
 * @note \n History
   1.Date        : 2014年10月28日 Create
     Author      : heshengyuan
     Modification: 新建文件
   2.Date        : 202106/23
     Author      : yindongping
     Modification: 组件开发，整理接口
 */


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <dspcommon.h>
#include "osd_tsk.h"
#include "osd_tsk_api.h"
#include "osd_drv_api.h"
#include "venc_tsk_api.h"
#include "platform_hal.h"
#include "system_prm_api.h"
#include "drawChar.h"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#line __LINE__ "osd_tsk.c"


/* #define SUPPORT_DISP_OSD  / *支持显示osd* / */

typedef struct
{
    BOOL fontType;                   /* 点阵类型，0、1、2、3分别代表8、16、24、32点阵*/
    UINT32 fontLen;                    /*字库长度*/
    UINT8 fontLib[FONT_LIB_SIZE];     /* 字库*/
} FONT_PARAM;


typedef struct
{
    BOOL bReady;                        /* 是否已经初始化 */
    BOOL bFlash;                        /* 是否闪烁 */
    BOOL bAutoLum;                      /* 是否自动调整亮度 */
    BOOL bTranslucent;                  /* 是否半透明 */
    UINT32 flashOn;
    UINT32 flashOff;
    OSD_LINE_S line[OSD_MAX_LINE];      /* 每行OSD的内部参数 */
} OSD_PARAM_S;

typedef struct
{
    UINT32 flgEncOsd;
    UINT32 flgDecOsd;
    UINT32 flgDispOsd;
    UINT32 flgEncLogo;
    UINT32 flgDispLogo;
    UINT32 flgEncPrevOsd[MAX_DISP_CHAN];
    UINT32 osd_region_num;
    UINT32 max_osd_line;
    UINT32 secondCount;
    UINT32 flgAdjustColor;
    PUINT8 stOsdTemp;
    PUINT8 pRgnClear;
    PUINT8 pFontLib[MAX_VENC_CHAN + MAX_VDEC_CHAN + MAX_DISP_CHAN];
    /* OSD_SCALE_BUF_INFO_S osdScaleBuf; / *用不到* / */
    OSD_PARAM_S *pEncOsdParam;
    /* OSD_PARAM_S *pzeroEncOsdParam; */
    OSD_PARAM_S *pDispOsdParam;
    /* OSD_PARAM_S *pEncOsdBackParam; / *用不到* / */
    pthread_mutex_t muProcessPrevOsd;
    pthread_mutex_t muOnSetOsd;
    /* pthread_mutex_t muOnSetLogo; */
    /* pthread_mutex_t muOnSetEncOsd; */
    pthread_mutex_t muOnUsingBuf;
    pthread_mutex_t muRefreshViParam;
} OSD_CTRL_S;

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

static OSD_CTRL_S g_stOsdCtrl;

/* 模块通道数 */
static UINT32 u32EncChnCnt = 0;
static UINT32 u32DispChnCnt = 0;
static UINT32 u32DecChnCnt = 0;
static UINT32 u32EncMask = 0;
static UINT32 u32DispMask = 0;
static UINT32 u32DecMask = 0;
static UINT32 u32EncOffset = 0;
static UINT32 u32DispOffset = 0;
static UINT32 u32DecOffset = 0;

static ENC_OSD_INFO_S g_stEncOsdInfo = {0};

static OSD_CONFIG stDefOsdParam =
{
    0,0                                   /* flash */, SAL_TRUE /*bAutoAdjustLum*/, 240 /*lum*/, SAL_FALSE /*bTranslucent*/,
    {
        {
            16,                         /* char count */
            128,
            1                           /* xScale */,
            1                           /* yScale */,
            {   0x20, 0x20, OSD_YEAR4, 0x2d, OSD_MONTH2, 0x2d, OSD_DAY, 0x20,
                OSD_WEEK3, 0x20, OSD_HOUR12, 0x3a, OSD_MINUTE, 0x3a, OSD_SECOND, OSD_AMPM
            },
        },/*
        {
            18,
            384,
            1,
            1,
            {'L', 'a', 'n', 'g', 'u', 'a', 'g', 'e', ' ', 'm', 'i', 's', 'm', 'a', 't', 'c', 'h', '!'}
        },*/
        {0}, {0},

    }
};

/* ========================================================================== */
/*                          函数声明                                        */
/* ========================================================================== */

void osd_process(UINT32 u32Chan, UINT32 u32Mode);

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function    osd_paramCheck
 * @brief       在视频信号更改或编码制式更改后重新
                检查OSD参数，过虑掉无需字符
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 u32Mode 通道类型
 * @param[out]  None
 * @return      void
 */
static void osd_paramCheck(UINT32 u32Chan, UINT32 u32Mode)
{
    OSD_PARAM_S *pstParam = NULL;
    OSD_LINE_S *pstLine = NULL;
    UINT32 i = 0;
    UINT32 u32ImgW = 0;
    UINT32 u32ImgH = 0;
    INT32 s32Ret = SAL_SOK;

    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;

    /* VIDEO_STANDARD vs  = VS_NON; */

    pthread_mutex_lock(&pstOsdCtrl->muProcessPrevOsd);

    pstParam = pstOsdCtrl->pEncOsdParam + u32Chan;
    if (u32Mode == OSD_ENC_MODE)
    {
        /* 此处先写死 */
        u32ImgW = 1920;
        u32ImgH = 1080;
    }
    else if (u32Mode == OSD_DISP_MODE)
    {
        /* 先写死 */
        u32ImgW = 1024;
        u32ImgH = 600;
    }

    if ((OSD_ENC_MODE != u32Mode) && (OSD_DISP_MODE != u32Mode))
    {
        OSD_LOGE("!!!\n");
        pthread_mutex_unlock(&pstOsdCtrl->muProcessPrevOsd);
        return;
    }

    if (0 == u32ImgH || 0 == u32ImgW)
    {
        OSD_LOGE("!!!\n");
        pthread_mutex_unlock(&pstOsdCtrl->muProcessPrevOsd);
        return;
    }

    /*************************************/
    /*还需根据各种制式对画出的大小进行修改*/
    /*************************************/
    for (i = 0; i < pstOsdCtrl->max_osd_line; i++)
    {
        if (u32Mode == OSD_DISP_MODE && i > 2)
        {
            continue;
        }

        pstLine = pstParam->line + i;
        s32Ret = osd_drv_paramCheck(u32ImgW, u32ImgH, pstLine, u32Mode);
        if (SAL_SOK != s32Ret)
        {
            continue;
        }
    }

#if 0
    if (u32Mode == OSD_ENC_MODE)
    {
        memset(pstOsdCtrl->osdScaleBuf.pOsdScaleBuf, 0x80, pstOsdCtrl->osdScaleBuf.bufLen);
        memcpy(pstOsdCtrl->pEncOsdBackParam + u32Chan, pstParam, sizeof(OSD_PARAM_S));
    }
#endif

    pthread_mutex_unlock(&pstOsdCtrl->muProcessPrevOsd);
    return;
}

/**
 * @function    osd_start
 * @brief       启动OSD
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 u32Mode 通道类型
 * @param[out]  None
 * @return      void
 */
static void osd_start(UINT32 u32Chan, UINT32 u32Mode)
{
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    UINT32 u32ChanOffset = u32EncOffset;

    osd_paramCheck(u32Chan, u32Mode);

    if (u32Mode == OSD_ENC_MODE)
    {
        pstOsdCtrl->flgEncOsd |= 1 << u32Chan;
        u32ChanOffset = u32EncOffset;
    }
    else if (u32Mode == OSD_DISP_MODE)
    {
        pstOsdCtrl->flgDispOsd |= 1 << u32Chan;
        u32ChanOffset = u32DispOffset;
    }

    osd_drv_refreshTag(0xffffffff, u32Chan, u32ChanOffset);
    osd_process(u32Chan, u32Mode);

    OSD_LOGI("viChn %d ok!!\n", u32Chan);
    return;
}

/**
 * @function    osd_deleteLine
 * @brief       检查OSD参数，去掉有区域重叠的行
 * @param[in]   OSD_CONFIG *pstInitPrm osd配置信息
 * @param[in]   UINT32 u32MaxLine OSD最大行数
 * @param[out]  None
 * @return      void
 */
 
/* 2022/01/25 修改注释: 因为先前的逻辑,重叠行将不会显示,导致两个osd字符有重叠行的话,有些osd字符就不会显示出来
                       导致遗漏; 先注释掉,后续多次验证功能正常,没有其他问题的时候后再删除osd_deleteLine函数 */
#if 0
static void osd_deleteLine(OSD_CONFIG *pstInitPrm, UINT32 u32MaxLine)
{
    UINT32 y[OSD_MAX_LINE];
    UINT32 i = 0;
    UINT32 k = 0;
    UINT32 h = 32;

    y[0] = pstInitPrm->line[0].top >> 16;

    for (i = 1; i < u32MaxLine; i++)
    {
        y[i] = pstInitPrm->line[i].top >> 16;
        if (pstInitPrm->line[i].charCnt == 0)
        {
            continue;
        }

        for (k = 0; k < i; k++)
        {
            if (pstInitPrm->line[k].charCnt && (((y[i] >= y[k]) && (y[i] < y[k] + h)) || ((y[i] + h > y[k]) && (y[i] <= y[k]))))
            {
                pstInitPrm->line[i].charCnt = 0;
                continue;
            }
        }
    }

    return;
}
#endif

/**
 * @function    osd_checkClearFlg
 * @brief       检测是否需要clear osd
 * @param[in]   UINT32 u32EncChan 通道号
 * @param[in]   UINT32 u32DispChan 显示通道号
 * @param[in]   UINT32 u32Mode 通道类型
 * @param[out]  UINT32 *pLineCnt 需要clear的OSD行数
 * @return      BOOL SAL_FALSE/SAL_TRUE
 */
static BOOL osd_checkClearFlg(UINT32 u32EncChan, UINT32 u32DispChan, UINT32 u32Mode, UINT32 *pLineCnt)
{
    BOOL beNeedClear = SAL_FALSE;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    UINT32 u32Line = 0;

    if (u32Mode == ENC_PREV_OSD)
    {
        beNeedClear = (pstOsdCtrl->flgEncPrevOsd[u32DispChan] & (1 << u32EncChan)) && (pstOsdCtrl->flgEncOsd & (1 << u32EncChan));
        u32Line = pstOsdCtrl->max_osd_line;
    }
    else if (u32Mode == ENC_PREV_LOGO)
    {
        beNeedClear = SAL_TRUE;
        u32Line = 1;
    }
    else if (u32Mode == DISP_PREV_OSD)
    {
        beNeedClear = pstOsdCtrl->flgDispOsd & (1 << u32DispChan);
        u32Line = pstOsdCtrl->max_osd_line;
    }
    else
    {
        beNeedClear = pstOsdCtrl->flgDispLogo & (1 << u32DispChan);
        u32Line = 1;
    }

    *pLineCnt = u32Line;

    return beNeedClear;
}

/**
 * @function    osd_setParam
 * @brief      设置OSD
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 u32Mode 通道类型
 * @param[in]   UINT32 bStart 开始标记
 * @param[in]   OSD_CONFIG *pstInitPrm OSD配置信息
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 osd_setParam(UINT32 u32Chan, UINT32 u32Mode, UINT32 bStart, OSD_CONFIG *pstInitPrm)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 x = 0;
    UINT32 y = 0;
    INT32 s32Ret = SAL_SOK;
    UINT32 u32Lum = 0;
    UINT32 u32ScaleH = 0;
    UINT32 u32ScaleV = 0;
    UINT32 u32EncChn = 0;
    UINT32 u32ChnOffset = u32EncOffset;
    OSD_PARAM_S *pstParam = NULL;
    OSD_RGN_S *pstRegion = NULL;
    PUINT8 pLib = NULL;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;

#ifdef SUPPORT_DISP_OSD
    BOOL beNeedClear = SAL_FALSE;
    UINT32 u32LineCnt = 0;
    OSD_LINE_S *pstLine = NULL;
#endif

    if (NULL == pstInitPrm)
    {
        OSD_LOGE("pstInitPrm is NULL\n");
        return SAL_FAIL;
    }
    
    /* 2022/01/25 修改注释: 因为先前的逻辑,重叠行将不会显示,导致两个osd字符有重叠行的话,有些osd字符就不会显示出来
                            导致遗漏; 先注释掉,后续多次验证功能正常,没有其他问题的时候后再删除osd_deleteLine函数 */

    /* 检查各行OSD是否有重叠区域，重叠行将不会显示 */
    //osd_deleteLine(pstInitPrm, pstOsdCtrl->max_osd_line);

    if (u32Mode == OSD_ENC_MODE)
    {
        u32ChnOffset = u32EncOffset;
#ifdef SUPPORT_DISP_OSD
        pthread_mutex_lock(&pstOsdCtrl->muProcessPrevOsd);
        beNeedClear = osd_checkClearFlg(u32Chan, 0, ENC_PREV_OSD, &u32LineCnt);
        if (SAL_TRUE == beNeedClear)
        {
            for (i = 0; i < u32LineCnt; i++)
            {
                pstLine = &pstOsdCtrl->pDispOsdParam[0].line[i];
                osd_drv_clearDisp(u32Chan, 0, pstLine, u32Mode);
            }
        }

        beNeedClear = osd_checkClearFlg(u32Chan, 2, ENC_PREV_OSD, &u32LineCnt);
        if (SAL_TRUE == beNeedClear)
        {
            for (i = 0; i < u32LineCnt; i++)
            {
                pstLine = &pstOsdCtrl->pDispOsdParam[2].line[i];
                osd_drv_clearDisp(u32Chan, 2, pstLine, u32Mode);
            }
        }

        pthread_mutex_unlock(&pstOsdCtrl->muProcessPrevOsd);
#endif

        for (i = 0; i < pstOsdCtrl->max_osd_line; i++)
        {
            for (j = 0; j < VENC_DEV_CHN_NUM; j++)
            {
                s32Ret = venc_tsk_getHalChan(u32Chan, j, &u32EncChn);
                if (SAL_SOK != s32Ret)
                {
                    OSD_LOGW("u32Chan:%d,stream:%d\n", u32Chan, j);
                    continue;
                }

                pstRegion = &g_stEncOsdInfo.stOsdRegion[VENC_DEV_CHN_NUM * u32Chan + j];
                s32Ret = osd_drv_destroy(u32EncChn, i, pstRegion);
                if (SAL_SOK != s32Ret)
                {
                    OSD_LOGE("u32Chan:%d,stream:%d\n", u32Chan, j);
                    continue;
                }
            }
        }

        pstParam = pstOsdCtrl->pEncOsdParam + u32Chan;
        pstOsdCtrl->flgEncOsd &= ~(1u << u32Chan);
    }
    else if (u32Mode == OSD_DISP_MODE)
    {
        pstParam = pstOsdCtrl->pDispOsdParam + u32Chan;
        pstOsdCtrl->flgDispOsd &= ~(1u << u32Chan);
        u32ChnOffset = u32DispOffset;
    }
    else
    {
        u32ChnOffset = u32EncOffset;
        pstParam = pstOsdCtrl->pEncOsdParam + u32Chan;
        OSD_LOGE("chan:%d mode:%d\n", u32Chan, u32Mode);
    }

    u32Lum = pstInitPrm->bAutoAdjustLum ? 0xf0 : pstInitPrm->lum & 0xff;
    u32Lum = u32Lum | (u32Lum << 8) | (u32Lum << 16) | (u32Lum << 24);
    pstParam->bReady = SAL_FALSE;
    pstParam->bAutoLum = SAL_FALSE;
    pstParam->bFlash = SAL_FALSE;
    pstParam->bTranslucent = SAL_FALSE;

    if (pstInitPrm->bAutoAdjustLum)
    {
        pstParam->bAutoLum = SAL_TRUE;
    }

    if (pstInitPrm->bTranslucent)
    {
        pstParam->bTranslucent = SAL_TRUE;
    }

    if (pstInitPrm->flash)
    {
        pstParam->bFlash = SAL_TRUE;
        pstParam->flashOn = (pstInitPrm->flash >> 8) & 0xff;
        pstParam->flashOff = pstInitPrm->flash & 0xff;
    }

    /* 清掉所有使用的特殊字符 */
    osd_drv_clearTag(u32Chan, u32ChnOffset);

    pLib = pstOsdCtrl->pFontLib[u32Chan + u32ChnOffset];

    /* 初始化字符 */
    for (i = 0; i < pstOsdCtrl->max_osd_line; i++)
    {
        if (u32Mode == OSD_DISP_MODE && i > 2)
        {
            break;
        }

        x = pstInitPrm->line[i].top & 0xFFFF;
        y = pstInitPrm->line[i].top >> 16;
        OSD_LOGI("u32Chan %d,i %d, xy[%d,%d],charCnt %d\n", u32Chan, i, x, y, pstInitPrm->line[i].charCnt);

        x = SAL_align(x, 8);
        y = SAL_align(y, 8);

        /* 根据主机设置的宽高比例选择算法 */
        u32ScaleH = pstInitPrm->line[i].hScale;
        u32ScaleV = pstInitPrm->line[i].vScale;
        s32Ret = osd_drv_setLineParam(&pstParam->line[i], u32ScaleV, u32ScaleH);
        if (SAL_SOK != s32Ret)
        {
            OSD_LOGE("invalid in init osd char u32Chan:%d,line:%d,char:%d\n", u32Chan, i, j);
            continue;
        }

        for (j = 0; j < pstInitPrm->line[i].charCnt; j++)
        {
            s32Ret = osd_drv_charInit(u32Chan, u32ChnOffset, pLib, pstParam->line + i, pstInitPrm->line[i].code[j], &x, y);
            if (s32Ret)
            {
                OSD_LOGE("invalid in init osd char u32Chan:%d,line:%d,char:%d\n", u32Chan, i, j);
                continue;
            }
        }
    }

    pstParam->bReady = SAL_TRUE;

    if (pstInitPrm->bStart)
    {
        osd_start(u32Chan, u32Mode);
    }

    return s32Ret;
}

/**
 * @function    osd_memAlloc
 * @brief       申请OSD组件所需内存
 * @param[in]   None
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
static INT32 osd_memAlloc(void)
{
    UINT32 u32Size = 0;
    INT32 s32Ret = SAL_FAIL;
    UINT8 *p = NULL;
    UINT8 *pvTemp = NULL;;
    ALLOC_VB_INFO_S stVbInfo = {0};

    UINT8 *ppTemp = NULL;;
    /* UINT8     *pTemp  = NULL;; */
    INT32 i = 0;
    INT32 j = 0;
    /* INT8      *pstrZone      = NULL; */
    CHAR *pStrMmb = "osd";
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    OSD_PARAM_S *pstEncOsdParam = NULL;
    OSD_PARAM_S *pstDispOsdParam = NULL;
    UINT16 *pu16Tmp = NULL;
    UINT16 u16BackColor = OSD_BACK_COLOR;

    u32Size = sizeof(OSD_PARAM_S) * (u32EncChnCnt + u32DecChnCnt + u32DispChnCnt);

    p = (PUINT8)SAL_memMalloc(u32Size, "osd", "tsk");
    if (NULL == p)
    {
        OSD_LOGE("\n");
        return SAL_FAIL;
    }

    memset(p, 0, u32Size);
    pstEncOsdParam = (OSD_PARAM_S *)p;
    pstDispOsdParam = pstEncOsdParam + u32DispOffset;
    pstOsdCtrl->pEncOsdParam = pstEncOsdParam;
    pstOsdCtrl->pDispOsdParam = pstDispOsdParam;

#if 0
    p = (PUINT8)SAL_memMalloc(sizeof(OSD_PARAM_S) * u32EncChnCnt, "osd", "tsk");
    if (NULL == p)
    {
        OSD_LOGE("!!!\n");

        return SAL_FAIL;
    }

    memset(p, 0, sizeof(OSD_PARAM_S) * u32EncChnCnt);
    pstOsdCtrl->pEncOsdBackParam = (OSD_PARAM_S *)p;
#endif

    /* 待优化 */
    u32Size = (OSD_4CIF_IMG_SIZE + OSD_CIF_IMG_SIZE + OSD_QCIF_IMG_SIZE)
              * pstOsdCtrl->max_osd_line * u32EncChnCnt + (1024 * 200 * 2);
    /* pstOsdCtrl->osdScaleBuf.bufLen = OSD_4CIF_IMG_SIZE; */
    /* u32Size += pstOsdCtrl->osdScaleBuf.bufLen; */

    /*下面调用有问题*/
    s32Ret = mem_hal_vbAlloc(u32Size, pStrMmb, pStrMmb, NULL, SAL_FALSE, &stVbInfo);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("\n");
        return SAL_FAIL;
    }
    ppTemp = (UINT8 *)stVbInfo.u64PhysAddr;
    pvTemp = stVbInfo.pVirAddr;

    pu16Tmp = (UINT16 *)pvTemp;
    for (i = 0; i < u32Size; i += 2)
    {
        *pu16Tmp++ = u16BackColor;
    }

    /* pstOsdCtrl->osdScaleBuf.pOsdScaleBuf = pvTemp; */
    /* pstOsdCtrl->osdScaleBuf.osdScaleBufPhyAddr = (PhysAddr)(ppTemp); */
    /* pvTemp += pstOsdCtrl->osdScaleBuf.bufLen; */
    /* ppTemp += pstOsdCtrl->osdScaleBuf.bufLen; */

    for (i = 0; i < u32EncChnCnt; i++)
    {
        for (j = 0; j < pstOsdCtrl->max_osd_line; j++)
        {
            pstEncOsdParam[i].line[j].p4CifCharImg = pvTemp;
            pvTemp += OSD_4CIF_IMG_SIZE;
            pstEncOsdParam[i].line[j].pCifCharImg = pvTemp;
            pvTemp += OSD_CIF_IMG_SIZE;
            pstEncOsdParam[i].line[j].pQCifCharImg = pvTemp;
            pvTemp += OSD_QCIF_IMG_SIZE;

            pstEncOsdParam[i].line[j].p4CifCharImgP = ppTemp;
            ppTemp += OSD_4CIF_IMG_SIZE;
            pstEncOsdParam[i].line[j].pCifCharImgP = ppTemp;
            ppTemp += OSD_CIF_IMG_SIZE;
            pstEncOsdParam[i].line[j].pQCifCharImgP = ppTemp;
            ppTemp += OSD_QCIF_IMG_SIZE;
        }
    }

    pstOsdCtrl->stOsdTemp = ppTemp; /*(1024 * 200 * 2)*/

    /*disp chan*/
    if(u32DispChnCnt)
    {
        u32Size = OSD_DISP_IMG_SIZE * pstOsdCtrl->max_osd_line * u32DispChnCnt;
        memset(&stVbInfo, 0, sizeof(ALLOC_VB_INFO_S));
        s32Ret = mem_hal_vbAlloc(u32Size, pStrMmb, pStrMmb, NULL, SAL_FALSE,  &stVbInfo);
        if (SAL_SOK != s32Ret)
        {
            OSD_LOGE("\n");
            return SAL_FAIL;
        }
    
        ppTemp = (UINT8 *)stVbInfo.u64PhysAddr;
        pvTemp = stVbInfo.pVirAddr;
    
        pu16Tmp = (UINT16 *)pvTemp;
        for (i = 0; i < u32Size; i += 2)
        {
            *pu16Tmp++ = u16BackColor;
        }
    
        for (i = 0; i < u32DispChnCnt; i++)
        {
            for (j = 0; j < pstOsdCtrl->max_osd_line; j++)
            {
                pstDispOsdParam[i].line[j].p4CifCharImg = pvTemp;
                pstDispOsdParam[i].line[j].pCifCharImg = pvTemp;
                pstDispOsdParam[i].line[j].pQCifCharImg = pvTemp;
    
                pstDispOsdParam[i].line[j].p4CifCharImgP = ppTemp;
                pstDispOsdParam[i].line[j].pCifCharImgP = ppTemp;
                pstDispOsdParam[i].line[j].pQCifCharImgP = ppTemp;
            }
    
            pvTemp += OSD_DISP_IMG_SIZE;
            ppTemp += OSD_DISP_IMG_SIZE;
        }
    }
    return SAL_SOK;
}

/**
 * @function    osd_process
 * @brief       OSD显示设置，分别在编码、解码、预览时调用
                用于产生字符大小、位置等参数，并用于计算
                填充字符背景颜色值{黑还是白}
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 u32Mode 通道类型
 * @param[out]  None
 * @return      void
 */
void osd_process(UINT32 u32Chan, UINT32 u32Mode)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 i = 0;
    UINT32 j = 0;
    OSD_PARAM_S *pstParam = NULL;
    OSD_LINE_S *pstLine = NULL;

    BOOL bLineChange = 0;
    UINT32 u32OsdRegChn = 0;
    UINT32 u32EncChn = 0;
    UINT32 u32Resolution = 0;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    OSD_RGN_S *pstRegion = NULL;

    if (OSD_DEC_MODE == u32Mode)
    {
        return;
    }

    if (u32Mode == OSD_ENC_MODE)
    {
        pstParam = pstOsdCtrl->pEncOsdParam + u32Chan;
    }
    else if (u32Mode == OSD_DISP_MODE)
    {
        pstParam = pstOsdCtrl->pDispOsdParam + u32Chan;
    }
    else
    {
        pstParam = pstOsdCtrl->pEncOsdParam + u32Chan;
    }

    for (i = 0; i < pstOsdCtrl->max_osd_line; i++)
    {
        pstLine = pstParam->line + i;
        if (pstLine->validCharCnt == 0)
        {
            continue;
        }

        bLineChange = SAL_FALSE;
        /*编码闪烁*/
        if (u32Mode == OSD_ENC_MODE)
        {
            if (pstParam->bFlash)
            {
                if (pstOsdCtrl->secondCount % (pstParam->flashOn + pstParam->flashOff) >= pstParam->flashOn)
                {
                    for (j = 0; j < VENC_DEV_CHN_NUM; j++)
                    {
                        s32Ret = venc_tsk_getHalChan(u32Chan, j, &u32EncChn);
                        if (SAL_FAIL == s32Ret)
                        {
                            OSD_LOGE("get enc chnfail, videv %d stream %d\n", u32Chan, j);
                            continue;
                        }

                        u32OsdRegChn = VENC_DEV_CHN_NUM * u32Chan + j;
                        pstRegion = &g_stEncOsdInfo.stOsdRegion[u32OsdRegChn];
                        s32Ret = osd_drv_destroy(u32EncChn, i, pstRegion);
                        if (SAL_FAIL == s32Ret)
                        {
                            OSD_LOGE("destroy region fail, videv %d stream %d\n", u32Chan, j);
                            continue;
                        }
                    }

                    continue;
                }
            }
        }

        s32Ret = osd_drv_draw(pstLine, u32Mode, &bLineChange);
        if (SAL_FAIL == s32Ret)
        {
            OSD_LOGE("draw osd fail\n");
            continue;
        }

        if (bLineChange)
        {
            if (u32Mode == OSD_ENC_MODE)
            {
                pthread_mutex_lock(&pstOsdCtrl->muOnUsingBuf);
                for (j = 0; j < VENC_DEV_CHN_NUM; j++)
                {
                    s32Ret = venc_tsk_getHalChan(u32Chan, j, &u32EncChn);
                    if (SAL_FAIL == s32Ret)
                    {
                        /*OSD_LOGE("get enc chn fail, videv %d stream %d\n", u32Chan, j);*/
                        continue;
                    }

                    s32Ret = venc_tsk_getResolution(u32Chan, j, &u32Resolution);
                    if (SAL_FAIL == s32Ret)
                    {
                        OSD_LOGE("get enc u32Resolution, videv %d stream %d\n", u32Chan, j);
                        continue;
                    }

                    u32OsdRegChn = VENC_DEV_CHN_NUM * u32Chan + j;
                    pstRegion = &g_stEncOsdInfo.stOsdRegion[u32OsdRegChn];
                    s32Ret = osd_drv_process(u32EncChn, u32Resolution, i, pstRegion, &pstParam->line[i], pstParam->bTranslucent);
                    if (SAL_FAIL == s32Ret)
                    {
                        OSD_LOGE("videv %d stream %d\n", u32Chan, j);
                        continue;
                    }
                }

                pthread_mutex_unlock(&pstOsdCtrl->muOnUsingBuf);
            }
        }
    }

    return;
}

/**
 * @function    osd_dispProcess
 * @brief       画OSD、LOGO函数
 * @param[in]   BOOL bAll 更新所有字符
 * @param[in]   void *prm osd区域参数
 * @param[out]  None
 * @return      void
 */
void osd_dispProcess(BOOL bAll, void *prm)
{
    INT32 i = 0;
    INT32 j = 0;
    INT32 k = 0;

    OSD_LINE_S *pstLine;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;

    pthread_mutex_lock(&pstOsdCtrl->muRefreshViParam);
    pthread_mutex_lock(&pstOsdCtrl->muProcessPrevOsd);

    for (i = 0; i < u32EncChnCnt; i++)
    {
        for (k = 0; k < MAX_DISP_CHAN; k++)
        {
            if (pstOsdCtrl->flgEncPrevOsd[k] & (1 << i))
            {
                for (j = 0; j < pstOsdCtrl->max_osd_line; j++)
                {
                    pstLine = &pstOsdCtrl->pEncOsdParam[i].line[j];
                    if ((pstOsdCtrl->pEncOsdParam[i].bFlash && pstOsdCtrl->secondCount % (pstOsdCtrl->pEncOsdParam[i].flashOn + pstOsdCtrl->pEncOsdParam[i].flashOff)
                         >= pstOsdCtrl->pEncOsdParam[i].flashOn) && (pstLine->validCharCnt > 0))
                    {
                        osd_drv_clearDisp(i, k, pstLine, ENC_PREV_OSD);
                    }
                    else if ((pstLine->validCharCnt > 0) && bAll)
                    {
                        osd_drv_disp(i, k, pstLine, ENC_PREV_OSD, prm, pstOsdCtrl->stOsdTemp);
                    }
                }
            }
        }

        for (j = 0; j < pstOsdCtrl->max_osd_line; j++)
        {
            if (prm == NULL)
            {
                /* pVi->bUpdateOsd[j] = 0; */
            }
        }
    }

    for (i = 0; i < u32EncChnCnt; i++)
    {
        if (pstOsdCtrl->flgEncLogo & (1 << i))
        {
            for (k = 0; k < MAX_DISP_CHAN; k++)
            {
            #if 0
                if (voParam[k].flgEncPrevLogo & (1 << i))
                {
                    /*unsupport*/
                    osd_drv_disp(i, k, NULL, ENC_PREV_LOGO, prm, pstOsdCtrl->stOsdTemp);
                }

            #endif
            }
        }
    }

    for (i = 0; i < u32DispChnCnt; i++)
    {
        if (pstOsdCtrl->flgDispOsd & (1 << i))
        {
            for (j = 0; j < pstOsdCtrl->max_osd_line; j++)
            {
                if (j > 2)
                {
                    continue;
                }

                pstLine = &pstOsdCtrl->pDispOsdParam[i].line[j];
                if (pstLine->validCharCnt > 0)
                {
                    osd_drv_disp(0, i, pstLine, DISP_PREV_OSD, prm, pstOsdCtrl->stOsdTemp);
                }
            }
        }
    }

    for (i = 0; i < u32DispChnCnt; i++)
    {
        if (pstOsdCtrl->flgDispLogo & (1 << i))
        {
        #if 0
            pVoParam = voParam + i;
            for (j = 0; j < MAX_LOGO_NUM_PER_CHAN; j++)
            {
                pLogoCfg = pVoParam->dispLogoCfg + j;
                if (pLogoCfg->bEnable)
                {
                    PrevImgProc(0, i, j, DISP_PREV_LOGO, pRect);
                }
            }

        #endif
        }
    }

    pthread_mutex_unlock(&pstOsdCtrl->muProcessPrevOsd);
    pthread_mutex_unlock(&pstOsdCtrl->muRefreshViParam);
}

/**
 * @function    osd_updateFontLib
 * @brief       处理主机的更新字库命令
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 u32ChanMode 通道类型
 * @param[in]   FONT_PARAM *pstParam 点阵字库参数
 * @param[out]  None
 * @return      void
 */
void osd_updateFontLib(UINT32 u32Chan, UINT32 u32ChanMode, FONT_PARAM *pstParam)
{
    PUINT8 pLib = NULL;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;

    if (u32ChanMode == OSD_ENC_MODE)
    {
        pLib = pstOsdCtrl->pFontLib[u32Chan];
    }
    else if (u32ChanMode == OSD_DEC_MODE)
    {
        pLib = pstOsdCtrl->pFontLib[u32EncChnCnt + u32Chan];
    }
    else if (u32ChanMode == OSD_DISP_MODE)
    {
        pLib = pstOsdCtrl->pFontLib[u32EncChnCnt + u32DecChnCnt + u32Chan];
    }
    else
    {
        pLib = pstOsdCtrl->pFontLib[u32Chan];
    }

    memset(pLib, 0, (FONT_LIB_SIZE));
    memcpy(pLib, pstParam->fontLib, pstParam->fontLen);

    return;
}

/**
 * @function    osd_tsk_getColorThread
 * @brief       更新OSD颜色线程
 * @param[in]   void *prm inv
 * @param[out]  None
 * @return      void *
 */
void *osd_tsk_getColorThread(void *prm)
{
    INT32 i = 0;
    INT32 j = 0;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    /* VI_PIPE ViPipe = 0; */
    PUINT8 pLumaAddr = NULL;
    UINT32 u32ImgW = 0;
    UINT32 u32ImgH = 0;
    UINT32 u32Stride = 0;

    SAL_SET_THR_NAME();

    while (1)
    {
        if (0 == pstOsdCtrl->flgAdjustColor)
        {
            usleep(500 * 1000);
            continue;
        }

        for (i = 0; i < MAX_VENC_CHAN; i++)
        {
            /* 取采集通道与编码通道同通道 */
            if ((pstOsdCtrl->flgEncOsd & (1 << i)) == 0)
            {
                usleep(50 * 1000);
                continue;
            }

            pthread_mutex_lock(&pstOsdCtrl->muRefreshViParam);

            /* ViPipe = 4 * i; */

            /*TODO:get vi frm*/

            for (j = 0; j < pstOsdCtrl->max_osd_line; j++)
            {
                osd_drv_getColor(&pstOsdCtrl->pEncOsdParam->line[j], pLumaAddr, u32ImgW, u32ImgH, u32Stride);
            }

            /*TODO:put vi frm*/

            pthread_mutex_unlock(&pstOsdCtrl->muRefreshViParam);
        }

        sleep(1);
    }

    return NULL;
}

/**
 * @function    osd_tsk_refreshThread
 * @brief       OSD刷新线程
 * @param[in]   None
 * @param[out]  None
 * @return      void *
 */
static void *osd_tsk_refreshThread()
{
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    INT32 i = 0;
    UINT32 u32FlgEnc = 0;

    SAL_SET_THR_NAME();

    while (1)
    {
        for (i = 0; i < u32EncChnCnt; i++)
        {
            if ((pstOsdCtrl->flgEncOsd & (1 << i)) != 0)
            {
                break;
            }
        }

        if (i == u32EncChnCnt)
        {
            usleep(500 * 1000);
            continue;
        }

        pthread_mutex_lock(&pstOsdCtrl->muOnSetOsd);
        /* pthread_mutex_lock(&pstOsdCtrl->muOnSetLogo); */
        osd_drv_refreshDateTime(&u32FlgEnc, u32EncMask);

        for (i = 0; i < u32EncChnCnt; i++)
        {
            if (pstOsdCtrl->flgEncOsd & (1 << i))
            {
                osd_drv_refreshTag(u32FlgEnc, i, u32EncOffset);
                osd_process(i, OSD_ENC_MODE);
            }
        }

        /* pthread_mutex_unlock(&pstOsdCtrl->muOnSetLogo); */
        pthread_mutex_unlock(&pstOsdCtrl->muOnSetOsd);
        usleep(100 * 1000);
    }

    return NULL;
}

/**
 * @function    osd_tsk_init
 * @brief       初始化OSD字符，给编码、解码、预览
 * @param[in]   None
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_init(void)
{
    INT32 i = 0;
    INT32 j = 0;
    UINT32 k = 0;
    UINT32 isLattice = SAL_FALSE;
    INT32 s32Ret = SAL_SOK;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    OSD_RGN_S *pstRegion = NULL;
    DSPINITPARA *pstDspInitParm = SystemPrm_getDspInitPara();
    CAPB_OSD *pstCapbOsd = capb_get_osd();
    SAL_ThrHndl pid;

    memset(&g_stOsdCtrl, 0, sizeof(OSD_CTRL_S));
    u32EncChnCnt = pstDspInitParm->encChanCnt;
    u32DispChnCnt = pstDspInitParm->stVoInitInfoSt.uiVoCnt;
    u32DecChnCnt = 0; /*pstDspInitParm->decChanCnt; unsupport*/

    /*enc|dec|disp|...*/
    u32EncOffset = 0;
    u32DecOffset = u32EncChnCnt;
    u32DispOffset = u32EncChnCnt + u32DecChnCnt;

    for (i = 0; i < u32EncChnCnt; i++)
    {
        u32EncMask |= (1 << i);
    }

    for (i = 0; i < u32DecChnCnt; i++)
    {
        u32DecMask |= (1 << (i + u32DecOffset));
    }

    for (i = 0; i < u32DispChnCnt; i++)
    {
        u32DispMask |= (1 << (i + u32DispOffset));
    }

    memset(&g_stEncOsdInfo, 0, sizeof(ENC_OSD_INFO_S));
    g_stEncOsdInfo.uiEncChnCnt = u32EncChnCnt;
    if (OSD_MAX_CHAN < u32EncChnCnt)
    {
        OSD_LOGE("enc chan num overflow %d\n", u32EncChnCnt);
        return SAL_FAIL;
    }

    memset(pstOsdCtrl, 0, sizeof(OSD_CTRL_S));
    pstOsdCtrl->osd_region_num = MAX_OVERLAY_REGION_PER_CHAN;
    pstOsdCtrl->max_osd_line = MAX_OVERLAY_REGION_PER_CHAN;

    s32Ret = osd_memAlloc();
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("mem alloc fail\n");
        return SAL_FAIL;
    }

    if (pstCapbOsd && OSD_FONT_LATTICE == pstCapbOsd->enFontType)
    {
        isLattice = SAL_TRUE;
    }
    else
    {
        isLattice = SAL_FALSE;
    }

    s32Ret = osd_drv_Init(&pstDspInitParm->stFontLibInfo.stHzLib,
                          &pstDspInitParm->stFontLibInfo.stAscLib,
                          pstDspInitParm->stFontLibInfo.stEncHzLib.pVirAddr,
                          pstDspInitParm->stFontLibInfo.stEncAscLib.pVirAddr,
                          isLattice);
    if (SAL_SOK != s32Ret)
    {
        OSD_LOGE("\n");
        return SAL_FAIL;
    }

    k = 0;
    for (i = 0; i < g_stEncOsdInfo.uiEncChnCnt; i++)
    {
        for (j = 0; j < VENC_DEV_CHN_NUM; j++)
        {
            pstRegion = &g_stEncOsdInfo.stOsdRegion[VENC_DEV_CHN_NUM * i + j];
            s32Ret = osd_drv_regionCreate(k, pstOsdCtrl->osd_region_num, pstRegion);
            if (SAL_SOK != s32Ret)
            {
                OSD_LOGE("\n");
                return SAL_FAIL;
            }

            k += pstOsdCtrl->osd_region_num;
        }
    }

    pthread_mutex_init(&pstOsdCtrl->muOnUsingBuf, NULL);
    /* pthread_mutex_init(&pstOsdCtrl->muOnSetEncOsd, NULL); */
    pthread_mutex_init(&pstOsdCtrl->muOnSetOsd, NULL);
    pthread_mutex_init(&pstOsdCtrl->muProcessPrevOsd, NULL);
    /* pthread_mutex_init(&pstOsdCtrl->muOnSetLogo, NULL); */
    pthread_mutex_init(&pstOsdCtrl->muRefreshViParam, NULL);

    SAL_thrCreate(&pid, osd_tsk_getColorThread, SAL_THR_PRI_DEFAULT, 0, NULL);
    SAL_thrCreate(&pid, osd_tsk_refreshThread, SAL_THR_PRI_DEFAULT, 0, NULL);

    /* 违禁品名称相关的OSD初始化 */
    if (SAL_SOK != osd_func_alertNameBlockInit(SVA_ALERT_NAME_LEN, ISM_XDT_TYPE_MAX))
    {
        SYS_LOGE("alert name osd init fail !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != osd_func_asciiBlockInit())
    {
        SYS_LOGE("ASCII osd init fail !!!\n");
        return SAL_FAIL;
    }

    /* 数字相关的OSD初始化 */
    if (SAL_SOK != osd_func_numBlockInit())
    {
        SYS_LOGE("num osd init fail !!!\n");
        return SAL_FAIL;
    }

    OSD_LOGI("finish\n");

    return SAL_SOK;
}

/**
 * @function    osd_tsk_setEncPrm
 * @brief       主机命令 HOST_CMD_SET_ENC_OSD 设置编码视频的OSD
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 *pParam 使能标记
 * @param[in]   UINT32 *pBuf OSD参数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_setEncPrm(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf)
{
    UINT32 s32Ret = SAL_SOK;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;

    pthread_mutex_lock(&pstOsdCtrl->muOnSetOsd);

    s32Ret = osd_setParam(u32Chan, OSD_ENC_MODE, 0, (OSD_CONFIG *)pBuf);

    pthread_mutex_unlock(&pstOsdCtrl->muOnSetOsd);
    OSD_LOGI("viChan %d set ok\n", u32Chan);
    return s32Ret;
}

/**
 * @function    osd_tsk_startEncProc
 * @brief      主机命令 HOST_CMD_START_ENC_OSD 启动编码OSD
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 *pParam 使能标记
 * @param[in]   UINT32 *pBuf OSD参数，inv
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_startEncProc(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf)
{
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    OSD_PARAM_S *pstOsdParam = pstOsdCtrl->pEncOsdParam + u32Chan;

    if (NULL == pstOsdParam)
    {
        OSD_LOGE("pstOsdParam %d is null\n", u32Chan);
        return SAL_FAIL;
    }

    if (pstOsdParam->bReady == SAL_FALSE)
    {
        OSD_LOGE("not ready\n");
        return SAL_FAIL;
    }

    if ((pstOsdCtrl->flgEncOsd & (1 << u32Chan)) == 0)
    {
        pstOsdCtrl->flgEncOsd |= (1 << u32Chan);
    }

    pthread_mutex_lock(&pstOsdCtrl->muOnSetOsd);
    osd_start(u32Chan, OSD_ENC_MODE);
    pthread_mutex_unlock(&pstOsdCtrl->muOnSetOsd);

    OSD_LOGI("viChn %d ok!!\n", u32Chan);
    return SAL_SOK;
}

/**
 * @function    osd_tsk_stopEncProc
 * @brief       主机命令 HOST_CMD_STOP_ENC_OSD 关闭编码OSD
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 *pParam 使能标记
 * @param[in]   UINT32 *pBuf OSD参数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_stopEncProc(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf)
{
    INT32 s32Ret = SAL_SOK;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;

    pthread_mutex_lock(&pstOsdCtrl->muOnSetOsd);

    s32Ret = osd_setParam(u32Chan, OSD_ENC_MODE, 0, (OSD_CONFIG *)pBuf);

    pthread_mutex_unlock(&pstOsdCtrl->muOnSetOsd);

    return s32Ret;
}

/**
 * @function    osd_tsk_setDispPrm
 * @brief       主机命令 HOST_CMD_SET_ENC_DEFAULT_OSD 设置为DSP端配置的默认OSD格式
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 *pParam 使能标记
 * @param[in]   UINT32 *pBuf OSD参数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_setEncDefPrm(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf)
{
    INT32 s32Ret = SAL_SOK;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;

    pthread_mutex_lock(&pstOsdCtrl->muOnSetOsd);
    s32Ret = osd_setParam(u32Chan, OSD_ENC_MODE, *pParam, &stDefOsdParam);
    pthread_mutex_unlock(&pstOsdCtrl->muOnSetOsd);

    return s32Ret;
}

/**
 * @function    osd_tsk_setDispPrm
 * @brief       主机命令 HOST_CMD_SET_DISP_OSD 设置显示的OSD
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 *pParam 使能标记
 * @param[in]   UINT32 *pBuf OSD参数
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_setDispPrm(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf)
{
    INT32 s32Ret = SAL_SOK;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;

    pthread_mutex_lock(&pstOsdCtrl->muOnSetOsd);

    s32Ret = osd_setParam(u32Chan, OSD_DISP_MODE, *pParam, (OSD_CONFIG *)pBuf);

    pthread_mutex_unlock(&pstOsdCtrl->muOnSetOsd);

#ifdef SUPPORT_DISP_OSD
    osd_dispProcess(0, NULL);
#endif
    return s32Ret;
}

/**
 * @function    osd_tsk_startDispProc
 * @brief       主机命令 HOST_CMD_START_DISP_OSD  启动VGA、HDMI、CVBS的OSD
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 *pParam INV prm
 * @param[in]   UINT32 *pBuf INV prm
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_startDispProc(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf)
{
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    OSD_PARAM_S *pstOsdParam = pstOsdCtrl->pDispOsdParam + u32Chan;

    if (pstOsdParam->bReady == SAL_FALSE)
    {
        return SAL_FAIL;
    }

    pthread_mutex_lock(&pstOsdCtrl->muOnSetOsd);
    osd_start(u32Chan, OSD_DISP_MODE);
    pthread_mutex_unlock(&pstOsdCtrl->muOnSetOsd);
    
#ifdef SUPPORT_DISP_OSD
    osd_dispProcess(0, NULL);
#endif

    return SAL_SOK;
}

/**
 * @function    osd_tsk_stopDispOsd
 * @brief       主机命令 HOST_CMD_STOP_DISP_OSD 停止OSD显示
 * @param[in]   UINT32 u32Chan 通道号
 * @param[in]   UINT32 *pParam INV prm
 * @param[in]   UINT32 *pBuf INV prm
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_stopDispOsd(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf)
{
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    OSD_LINE_S *pstLine = NULL;
    BOOL beNeedClear = SAL_FALSE;
    UINT32 u32LineCnt = 0;
    UINT32 i = 0;

    pthread_mutex_lock(&pstOsdCtrl->muProcessPrevOsd);
    beNeedClear = osd_checkClearFlg(0, u32Chan, ENC_PREV_OSD, &u32LineCnt);
    if (SAL_TRUE == beNeedClear)
    {
        for (i = 0; i < u32LineCnt; i++)
        {
            pstLine = &pstOsdCtrl->pDispOsdParam[0].line[i];
            osd_drv_clearDisp(0, u32Chan, pstLine, DISP_PREV_OSD);
        }
    }

    pthread_mutex_unlock(&pstOsdCtrl->muProcessPrevOsd);

    pstOsdCtrl->flgDispOsd &= ~(1 << u32Chan);
    return SAL_SOK;
}

/**
 * @function    osd_tsk_updateEncFontLib
 * @brief       处理主机的更新编码通道字库命令
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 *pParam INV prm
 * @param[in]   UINT32 *pBuf 字库
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_updateEncFontLib(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf)
{
    /* PDSPINITPARA pDspInitParam = dspParam.pDspInitParam; */
    /* if (pDspInitParam->NetOsdType & OSD_TYPE_DYNAMIC) */
    {
        osd_updateFontLib(u32Chan, OSD_ENC_MODE, (FONT_PARAM *)pBuf);
    }

    return SAL_SOK;
}

/**
 * @function    osd_tsk_updateDecFontLib
 * @brief       处理主机的更新解码通道字库命令
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 *pParam INV prm
 * @param[in]   UINT32 *pBuf 字库
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_updateDecFontLib(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf)
{
    /* PDSPINITPARA pDspInitParam = dspParam.pDspInitParam; */
    /* if (pDspInitParam->NetOsdType & OSD_TYPE_DYNAMIC) */
    {
        osd_updateFontLib(u32Chan, OSD_DEC_MODE, (FONT_PARAM *)pBuf);
    }

    return SAL_SOK;
}

/**
 * @function    osd_tsk_updateDispFontLib
 * @brief       处理主机的更新显示通道字库命令
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 *pParam INV prm
 * @param[in]   UINT32 *pBuf 字库
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_updateDispFontLib(UINT32 u32Chan, UINT32 *pParam, UINT32 *pBuf)
{
    /* PDSPINITPARA pDspInitParam = dspParam.pDspInitParam; */
    /* if(pDspInitParam->NetOsdType & OSD_TYPE_DYNAMIC) */
    {
        osd_updateFontLib(u32Chan, OSD_DISP_MODE, (FONT_PARAM *)pBuf);
    }

    return SAL_SOK;
}

/**
 * @function    osd_tsk_hangEncProc
 * @brief       停止编码OSD显示
 * @param[in]   UINT32 u32Chan vi通道
 * @param[in]   UINT32 u32StreamId 码流id
 * @param[in]   UINT32 u32VenChnId venc hal通道id
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_hangEncProc(UINT32 u32Chan, UINT32 u32StreamId, UINT32 u32VenChnId)
{
    INT32 s32Ret = SAL_SOK;
    INT32 i = 0;
    UINT32 u32OsdRegChn = VENC_DEV_CHN_NUM * u32Chan + u32StreamId;
    OSD_RGN_S *pstRegion = NULL;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    OSD_PARAM_S *pstOsdParam = NULL;

    if (NULL == pstOsdCtrl->pEncOsdParam)
    {
        OSD_LOGE("pEncOsdParam is null\n");
        return SAL_FAIL;
    }

    pstOsdParam = pstOsdCtrl->pEncOsdParam + u32Chan;

    if (pstOsdParam->bReady == SAL_FALSE)
    {
        OSD_LOGE("not ready\n");
        return SAL_FAIL;
    }

    pthread_mutex_lock(&pstOsdCtrl->muOnUsingBuf);
    pstRegion = &g_stEncOsdInfo.stOsdRegion[u32OsdRegChn];

    for (i = 0; i < pstOsdCtrl->osd_region_num; i++)
    {
        s32Ret = osd_drv_destroy(u32VenChnId, i, pstRegion);
        if (SAL_SOK != s32Ret)
        {
            OSD_LOGE("!!!\n");
            continue;
        }
    }

    pthread_mutex_unlock(&pstOsdCtrl->muOnUsingBuf);
    OSD_LOGI("viChan %d,VeGroup = %d\n", u32Chan, u32StreamId);

    return SAL_SOK;
}

/**
 * @function    osd_tsk_restart
 * @brief       重新启动OSD
 * @param[in]   UINT32 u32ViChan vi通道
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
void osd_tsk_restart(UINT32 u32ViChan)
{
    UINT32 u32Chan = u32ViChan;
    OSD_CTRL_S *pstOsdCtrl = &g_stOsdCtrl;
    OSD_PARAM_S *pstOsdParam = NULL;

    if (NULL == pstOsdCtrl->pEncOsdParam)
    {
        OSD_LOGE("pEncOsdParam is null\n");
        return;
    }

    pstOsdParam = pstOsdCtrl->pEncOsdParam + u32Chan;
    if (pstOsdParam->bReady == SAL_FALSE)
    {
        OSD_LOGW("not ready\n");
        return;
    }

    if ((pstOsdCtrl->flgEncOsd & (1 << u32Chan)) == 0)
    {
        pstOsdCtrl->flgEncOsd |= (1 << u32Chan);
    }

    pthread_mutex_lock(&pstOsdCtrl->muOnSetOsd);
    osd_start(u32Chan, OSD_ENC_MODE);
    pthread_mutex_unlock(&pstOsdCtrl->muOnSetOsd);
    return;
}

/**
 * @function    osd_tsk_startDST
 * @brief       osd启用夏令时
 * @param[in]   UINT32 u32ViChan vi通道
 * @param[out]  None
 * @return      int  成功SAL_SOK，失败SAL_FAIL
 */
INT32 osd_tsk_startDST(UINT32 u32Chan,  UINT32 *pBuf)
{

    if (NULL == pBuf)
    {
        OSD_LOGE("pParam or pBuf is null\n");
        return SAL_FAIL;
    }
    
    SAL_setDSTDateTime((OSD_DST_PRAM_S *)pBuf);

    return SAL_SOK;
}


