/*******************************************************************************
 * drv_osd.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : liuxianying <liuxianying@hikvision.com>
 * Version: V1.0.0  2021年07月03日 Create
 *
 * Description :
 * Modification:
 *******************************************************************************/

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include <sal.h>
#include "osd_drv_api.h"
#include "drawChar.h"
#include "freetype.h"
#include "disp_tsk_inter.h"
#include "vgs_drv_api.h"
#include "system_plat_common.h"
#include "system_prm_api.h"



static UINT32 gau32VarOsdChn[DISP_HAL_MAX_DEV_NUM][DISP_HAL_MAX_CHN_NUM] = {0};
static DISP_SVA_OUT_CTL g_SvaPosInfo[DISP_HAL_MAX_DEV_NUM] = {0};
static DISP_SVA_RECT_F g_svaRect[DISP_HAL_MAX_DEV_NUM][DISP_HAL_MAX_CHN_NUM][SVA_XSI_MAX_ALARM_NUM] = {0};
static VGS_DRAW_AI_PRM *gpastDispLinePrm[DISP_HAL_MAX_DEV_NUM][DISP_HAL_MAX_CHN_NUM] = {NULL};
static VGS_ADD_OSD_PRM *gpastDispOsdPrm[DISP_HAL_MAX_DEV_NUM][DISP_HAL_MAX_CHN_NUM] = {NULL};
static BOOL gbOsdTimeEnable = SAL_FALSE;
static UINT32 gu32OsdTime = 0;

#define REGION_CPY_TO_DSP(dsp_region, osd_region) \
    { \
        dsp_region->u64PhyAddr = osd_region->stAddr.u64PhyAddr; \
        dsp_region->u32Stride  = osd_region->u32Stride; \
        dsp_region->u32Width   = osd_region->u32Width; \
        dsp_region->u32Height  = osd_region->u32Height; \
    }


extern DISP_CHN_COMMON *disp_tskGetVoChn(UINT32 uiDev, UINT32 uiChn);
extern DISP_DEV_COMMON *disp_tskGetVoDev(UINT32 uiDev);
extern UINT64 disp_tskGetTimeMilli(void);
extern UINT32 disp_tskGetVoChnCnt(UINT32 uiDev);




/*******************************************************************************
* 函数名  : disp_svpDspfillLatRegion
* 描  述  : 获取DSP画线的类型
* 输  入  : DISPLINE_PRM *pstSrc : ARM端
* 输  出  : SVP_DSP_LINE_TYPE_S *pstDst : DSP端
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
VOID disp_svpDspfillLatRegion(OSD_REGION_S *pstNameRgn, OSD_REGION_S *pstNumRgn, SVP_DSP_OSD_S *pstOsdLat)
{
    UINT32 h, w;
    UINT32 u32NameByte, u32NameLeft, u32NameLeftTmp;
    UINT32 u32NumByte, u32NumLeft;
    UINT8 *pu8Name, *pu8Num, *pu8Dst, *pu8LatTmp, *pu8NumTmp;
    UINT8 u8Tmp;

    if (NULL == pstNumRgn)
    {
        pstOsdLat->u32LatPitch = SAL_align((pstNameRgn->u32Width + 0x07) >> 3, 8);
        pstOsdLat->stOsdRect.u32Width  = pstNameRgn->u32Width;
        pstOsdLat->stOsdRect.u32Height =  pstNameRgn->u32Height;

        u32NameByte = (pstNameRgn->u32Width + 0x07) >> 3;
        pu8Dst  = pstOsdLat->au8Lattice;
        pu8Name = pstNameRgn->stAddr.pu8VirAddr;

        for (h = 0; h < pstNameRgn->u32Height; h++)
        {
            memcpy(pu8Dst, pu8Name, u32NameByte);
            pu8Name += pstNameRgn->u32Stride;
            pu8Dst  += pstOsdLat->u32LatPitch;
        }

        return;
    }

    pstOsdLat->u32LatPitch = SAL_align((pstNameRgn->u32Width + pstNumRgn->u32Width + 0x07) >> 3, 8);
    pstOsdLat->stOsdRect.u32Width  = pstNameRgn->u32Width + pstNumRgn->u32Width;
    pstOsdLat->stOsdRect.u32Height =  pstNameRgn->u32Height;

    u32NameByte    = (pstNameRgn->u32Width + 0x07) >> 3;
    u32NameLeft    = pstNameRgn->u32Width & 0x07;
    u32NameLeftTmp = 8 - u32NameLeft;
    u32NumByte     = (pstNumRgn->u32Width + u32NameLeft + 0x07) >> 3;
    u32NumLeft     = (pstNumRgn->u32Width + u32NameLeft) & 0x07;

    pu8Name = pstNameRgn->stAddr.pu8VirAddr;
    pu8Num  = pstNumRgn->stAddr.pu8VirAddr;
    pu8Dst  = pstOsdLat->au8Lattice;

    if (0 == u32NameLeft)
    {
        for (h = 0; h < pstNameRgn->u32Height; h++)
        {
            memcpy(pu8Dst, pu8Name, u32NameByte);
            memcpy(pu8Dst + u32NameByte, pu8Num, u32NumByte);
            pu8Name += pstNameRgn->u32Stride;
            pu8Num  += pstNumRgn->u32Stride;
            pu8Dst  += pstOsdLat->u32LatPitch;
        }
    }
    else
    {
        for (h = 0; h < pstNameRgn->u32Height; h++)
        {
            memcpy(pu8Dst, pu8Name, u32NameByte);

            pu8LatTmp = pu8Dst + u32NameByte - 1;
            pu8NumTmp = pu8Num;
            u8Tmp     = *(pu8Name + u32NameByte - 1);
            for (w = 0; w < u32NumByte; w++)
            {
                *pu8LatTmp++ = (u8Tmp | (*pu8NumTmp >> u32NameLeftTmp));
                u8Tmp = *pu8NumTmp++ << u32NameLeft;
            }

            if (u32NumLeft > 0)
            {
                if (u32NumLeft > u32NameLeftTmp)
                {
                    *pu8LatTmp = (u8Tmp | (*pu8NumTmp >> u32NameLeftTmp));
                }
                else
                {
                    *pu8LatTmp = u8Tmp;
                }
            }

            pu8Name += pstNameRgn->u32Stride;
            pu8Num  += pstNumRgn->u32Stride;
            pu8Dst  += pstOsdLat->u32LatPitch;
        }
    }
}


/*******************************************************************************
* 函数名  : disp_osdPstOutSort
* 描  述  : 非叠框模式智能信息排序
* 输  入  : - pstOut:
*         : - pSvaRect:
*         : - uiPicW:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_osdPstOutSort(SVA_PROCESS_IN *pstIn, SVA_PROCESS_OUT *pstOut, DISP_SVA_RECT_F *pSvaRect, UINT32 width)
{
    INT32 i = 0;
    INT32 j = 0;
    INT32 alert_num = 0; /* 智能信息个数 */
    SVA_TARGET tmpTarget = {0};
    DISP_SVA_RECT_F tmpRect = {0};

    FLOAT32 f32TmpX1 = 0.0f;   /*第1个矩形框X坐标*/
    FLOAT32 f32TmpX2 = 0.0f;   /*第2个矩形框X坐标*/
    FLOAT32 f32TmpW1 = 0.0f;   /*第1个矩形宽*/
    FLOAT32 f32TmpW2 = 0.0f;   /*第2个矩形宽*/
    FLOAT32 f32PicW  = 0.0f;   /* 图像宽 */
    FLOAT32 f32EndX1 = 0.0f;
    FLOAT32 f32EndX2 = 0.0f;

    if (NULL == pstIn || NULL == pstOut || NULL == pSvaRect)
    {
        DISP_LOGE("pstOut NULL err\n");
        return SAL_FAIL;
    }
    
    f32PicW = (FLOAT32)width;    /* 图像宽 */
    alert_num = pstOut->target_num;

    for (i = 0; i < alert_num - 1; i++)
    {
        for (j = 0; j < alert_num - 1 - i; j++)
        {
            f32EndX1 = pSvaRect[j].f32EndX;
            f32EndX2 = pSvaRect[j +1 ].f32EndX;
        
            /*采用算法吐出的浮点值计算矩形框X坐标进行排序，防止精度丢失导致画框排序抖动*/
            f32TmpX1 = (FLOAT32)pSvaRect[j].x + pSvaRect[j].f32StartXFract;
            f32TmpX2 = (FLOAT32)pSvaRect[j+1].x + pSvaRect[j+1].f32StartXFract;

            f32TmpW1 = f32EndX1 - f32TmpX1;
            f32TmpW2 = f32EndX2 - f32TmpX2;
            if (SVA_OSD_LINE_POINT_TYPE == pstIn->drawType)
            {
                /*计算出目标6x6小框的X坐标进行排序*/
                f32TmpX1 = ((f32TmpX1 +  f32TmpW1 / 2) >= f32PicW ? f32PicW - f32TmpX1 : f32TmpW1 / 2) + f32TmpX1;
                f32TmpX2 = ((f32TmpX2 +  f32TmpW2 / 2) >= f32PicW ? f32PicW - f32TmpX2 : f32TmpW2 / 2) + f32TmpX2;
            }

            /*按照目标顶点X坐标进行排序*/
            if (f32TmpX1 > f32TmpX2)
            {
                memcpy(&tmpTarget, &pstOut->target[j], sizeof(SVA_TARGET));
                memcpy(&pstOut->target[j], &pstOut->target[j + 1], sizeof(SVA_TARGET));
                memcpy(&pstOut->target[j + 1], &tmpTarget, sizeof(SVA_TARGET));

                memcpy(&tmpRect, &pSvaRect[j], sizeof(DISP_SVA_RECT_F));
                memcpy(&pSvaRect[j], &pSvaRect[j + 1], sizeof(DISP_SVA_RECT_F));
                memcpy(&pSvaRect[j + 1], &tmpRect, sizeof(DISP_SVA_RECT_F));
            }
        }
    }

    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : disp_osdPstOutFloattoINT
* 描  述  : 获取智能信息的检测区域
* 输  入  : - pDispChn:
*         : - pstFrameInfo:
*         : - pstOut        :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 disp_osdPstOutFloattoINT(DISP_CHN_COMMON *pDispChn, SAL_VideoFrameBuf *pVideoFrame, SVA_PROCESS_OUT *pstOut)
{
    UINT32 voChn = 0;
    UINT32 vodev = 0;
    UINT32 uiPicW = 0;
    UINT32 uiPicH = 0;
    INT32 alert_num = 0; /* 智能信息个数 */
    INT32 i = 0;
    DISP_SVA_RECT_F *pSvaRect = NULL;
    BOOL bUseFract = capb_get_line()->bUseFract;
    FLOAT32 f32StartX = 0.0;
    FLOAT32 f32EndX = 0.0;

    if (pDispChn == NULL)
    {
        DISP_LOGE("pDispChn NULL err\n");
        return SAL_FAIL;
    }

    if (pVideoFrame == NULL)
    {
        DISP_LOGE("pstFrameInfo NULL err\n");
        return SAL_FAIL;
    }

    if (pstOut == NULL)
    {
        DISP_LOGE("pstOut NULL err\n");
        return SAL_FAIL;
    }

    vodev = pDispChn->uiDevNo;
    voChn = pDispChn->uiChn;
    uiPicW = pVideoFrame->frameParam.width;
    uiPicH = pVideoFrame->frameParam.height;
    alert_num = pstOut->target_num;
    pSvaRect = g_svaRect[vodev][voChn]; /* 智能信息浮点型转整形存储结构体 */

    for (i = 0; i < alert_num; i++)
    {
        f32StartX = pstOut->target[i].rect.x * (float)uiPicW;
        
        /*画框使用整型，会导致精度丢失*/
        pSvaRect[i].x = (UINT32)f32StartX;
        pSvaRect[i].y = pstOut->target[i].rect.y * uiPicH;
        f32EndX = (pstOut->target[i].rect.x + pstOut->target[i].rect.width) * (float)uiPicW;
        pSvaRect[i].width = (UINT32)(f32EndX - f32StartX);
        pSvaRect[i].height = pstOut->target[i].rect.height * uiPicH;
        pSvaRect[i].f32EndX = f32EndX;

        pSvaRect[i].y      = SAL_align(pSvaRect[i].y, 2);
        pSvaRect[i].height = SAL_align(pSvaRect[i].height, 2);

        if (SAL_TRUE == bUseFract)
        {
            pSvaRect[i].f32StartXFract = f32StartX - (UINT32)f32StartX;
            pSvaRect[i].f32EndXFract   = f32EndX - (UINT32)f32EndX;
        }
        else
        {
            pSvaRect[i].x     = SAL_align(pSvaRect[i].x, 2);
            pSvaRect[i].width = SAL_align(pSvaRect[i].width, 2);
            pSvaRect[i].f32StartXFract = -1;
            pSvaRect[i].f32EndXFract   = -1;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_osdPstOutcorrect
* 描  述  : 当前叠框坐标矫正
* 输  入  : - pDispChn:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_osdPstOutcorrect(DISP_OSD_COORDINATE_CORRECT *osd_correct, 
                                          DISP_OSD_CORRECT_PRM_S *pstOsdPrm, 
                                          DISP_SVA_RECT_F *pstSvaRect,
                                          SVA_PROCESS_OUT *pstOut,
                                          SVA_TARGET *pstTarInfo,
                                          DISP_TAR_CNT_INFO *pstPkgCntInfo)
{
    FLOAT32 f32TmpWidth = 0;
    UINT32 uiTmpH = 0;
    UINT32 uiPicW = 0;    /* 图像宽 */
    UINT32 uiPicH = 0;    /* 图像高 */
    UINT32 yOffset = 0;
    UINT32 uiTmp = 0;
    UINT32 uiBorGap = 8;      /* OSD列间隔 */
    FLOAT32 f32Tmp = 0.0f;
    FLOAT32 f32X = 0.0f;     /*目标物矩形顶点X坐标， 仅用于OSD画框排列*/
    FLOAT32 f32W = 0.0f;     /*目标物矩形宽度，仅用于OSD画框排列*/
    INT32 aioffsetH = 0;
    SVA_BORDER_TYPE_E enBorderType; /* osd画线类型 */

    if ((NULL == osd_correct)|| (NULL == pstOsdPrm)
        || (NULL == pstSvaRect) || (NULL == pstOut)
        || (NULL == pstTarInfo))
    {
        DISP_LOGE("ptr null! %p, %p, %p, %p, %p \n",
                    osd_correct, pstOsdPrm, pstSvaRect, pstOut, pstTarInfo);
        return SAL_FAIL;
    }

    /* 避免重复计算，另一个视角目标直接返回 */
    if (SAL_TRUE == pstTarInfo->bAnotherViewTar
     && (SVA_OSD_TYPE_CROSS_NO_RECT_TYPE != pstOsdPrm->enBorderType && SVA_OSD_TYPE_CROSS_RECT_TYPE != pstOsdPrm->enBorderType))
    {
        pstOsdPrm->u32X = pstOsdPrm->u32Y = 0;
        return SAL_SOK;
    }

    uiPicW       = osd_correct->uiPicW;    /* 图像宽 */
    uiPicH       = osd_correct->uiPicH;    /* 图像高 */
    yOffset      = pstOsdPrm->u32Height + 2;
    aioffsetH    = osd_correct->uiAiOffsetH;
    enBorderType = pstOsdPrm->enBorderType; /* osd画线类型 */

    f32X = (FLOAT32)pstSvaRect->x + pstSvaRect->f32StartXFract;
    /* 对目标物进行框选 */
    if (SVA_OSD_LINE_POINT_TYPE == enBorderType)
    {
        f32TmpWidth = pstSvaRect->f32EndX - f32X;

        f32TmpWidth = ((FLOAT32)pstSvaRect->x + f32TmpWidth / 2) >= (FLOAT32)uiPicW ? (FLOAT32)uiPicW - (FLOAT32)pstSvaRect->x : f32TmpWidth / 2;

        uiTmpH = (pstSvaRect->y + pstSvaRect->height / 2) >= uiPicH ? uiPicH - pstSvaRect->y : pstSvaRect->height / 2;
        //uiTmpW = (uiTmpW + 1) / 2 * 2;
        uiTmpH = (uiTmpH + 1) / 2 * 2;

        //pstSvaRect->x += (uiTmpW - 2);
        pstSvaRect->y += (uiTmpH - 2);
        pstSvaRect->x  = (UINT32)(f32X + f32TmpWidth);
        pstSvaRect->width  = 6;
        pstSvaRect->height = 6;
        pstSvaRect->f32StartXFract = (f32X + f32TmpWidth) - (UINT32)(f32X + f32TmpWidth);
        pstSvaRect->f32EndXFract   = (f32X + f32TmpWidth) - (UINT32)(f32X + f32TmpWidth);
        
        f32W = (FLOAT32)pstSvaRect->width + pstSvaRect->f32EndXFract;
        f32X = ((f32X +  f32W / 2) >= (FLOAT32)uiPicW ? (FLOAT32)uiPicW - f32X : f32W / 2) + f32X;
    }
    
    pstOsdPrm->u32X = pstSvaRect->x;
    pstOsdPrm->u32Y = pstSvaRect->y > yOffset ? (pstSvaRect->y - yOffset) : (pstSvaRect->y + 8);

    if ((SVA_OSD_LINE_RECT_TYPE == enBorderType) 
        || (SVA_OSD_LINE_POINT_TYPE == enBorderType)
        || (SVA_OSD_TYPE_NO_LINE_TYPE == enBorderType))
    {
        f32Tmp = f32X;
        /* 当下一个目标的左上角x坐标大于当前目标OSD右侧区域的x坐标时，重新进行从上到下OSD叠加过程 */
       if (f32Tmp > osd_correct->f32UpRightX)
        {
            osd_correct->uiTmpOsdY = osd_correct->uiUpStartY;
            osd_correct->f32UpRightX = 0.0f;
        }

        if (SAL_FALSE == osd_correct->bOsdPstOutFlag)
        {
            /*
            ** 目标叠加OSD，当前策略:
            ** 画面上方从纵坐标为yOffset开始，最大叠加到整个画面高度的13%
            ** 画面下方从整个画面高度的87%开始，若超过画面高度则进行覆盖
            */
            if (osd_correct->uiTmpOsdY + pstOsdPrm->u32Height >= uiPicH * 13 / 100)
            {
                osd_correct->uiTmpOsdY = osd_correct->uiTmpOsdY >= uiPicH * 87 / 100 ? osd_correct->uiTmpOsdY : SAL_alignDown(uiPicH * 87 / 100, 2);
                if (osd_correct->uiTmpOsdY + pstOsdPrm->u32Height > uiPicH)
                {
                    osd_correct->uiTmpOsdY = uiPicH - yOffset;
                }

                osd_correct->uiUpOsdFlag = 0;
                pstOsdPrm->u32Y = osd_correct->uiTmpOsdY > aioffsetH ? osd_correct->uiTmpOsdY - aioffsetH : osd_correct->uiTmpOsdY;  /* 记录OSD坐标用于VGS画框 */
            }
            else
            {
                osd_correct->uiUpOsdFlag = 1;
                pstOsdPrm->u32Y = osd_correct->uiTmpOsdY + aioffsetH < uiPicH ? osd_correct->uiTmpOsdY + aioffsetH : osd_correct->uiTmpOsdY;  /* 记录OSD坐标用于VGS画框 */
            }
        }
        else
        {
            /*
            ** 目标叠加OSD，当前策略:
            ** 画面上方从纵坐标为yOffset开始，包裹上方最多叠10个危险品
            ** 画面下方从第11个危险开始叠，若超过画面高度则进行覆盖
            */
            if (osd_correct->uiTmpOsdY + pstOsdPrm->u32Height > osd_correct->uiUpStartY+osd_correct->uiUpTarNum * pstOsdPrm->u32Height)
            {
                SVA_LOGD("111:chan %d, uiTmpOsdY %d, %d + %d*%d =  sum %d, sourceH %d, sum + sourceH =  %d \n", osd_correct->uiChan, osd_correct->uiTmpOsdY, 
                    osd_correct->uiUpStartY, osd_correct->uiUpTarNum, pstOsdPrm->u32Height,
                    (osd_correct->uiUpStartY + osd_correct->uiUpTarNum * pstOsdPrm->u32Height),
                    osd_correct->uiSourceH,  (osd_correct->uiUpStartY + osd_correct->uiUpTarNum * pstOsdPrm->u32Height + osd_correct->uiSourceH));
                osd_correct->uiTmpOsdY = osd_correct->uiTmpOsdY >= (osd_correct->uiUpStartY + osd_correct->uiUpTarNum * pstOsdPrm->u32Height + osd_correct->uiSourceH) ? \
                                                                osd_correct->uiTmpOsdY : SAL_alignDown((osd_correct->uiUpStartY+osd_correct->uiUpTarNum *pstOsdPrm->u32Height + osd_correct->uiSourceH), 2);
                if (osd_correct->uiTmpOsdY + pstOsdPrm->u32Height > 1024)
                {
                    osd_correct->uiTmpOsdY = 1024 - yOffset;
                }

                osd_correct->uiUpOsdFlag = 0;
                pstOsdPrm->u32Y = osd_correct->uiTmpOsdY > aioffsetH ? osd_correct->uiTmpOsdY - aioffsetH : osd_correct->uiTmpOsdY;  /* 记录OSD坐标用于VGS画框 */
            }
            else
            {
                osd_correct->uiUpOsdFlag = 1;
                pstOsdPrm->u32Y = osd_correct->uiTmpOsdY + aioffsetH < uiPicH ? osd_correct->uiTmpOsdY + aioffsetH : osd_correct->uiTmpOsdY;  /* 记录OSD坐标用于VGS画框 */
            }
        }
        
        osd_correct->uiTmpOsdY += pstOsdPrm->u32Height;      /* 保留上一个目标的叠框纵坐标位置，避免重叠 */
        osd_correct->f32LastRightX = f32X + (FLOAT32)pstOsdPrm->u32Width + 14.0f;    /* 保留上一个目标OSD的右横坐标 */
        if (1 == osd_correct->uiUpOsdFlag)
        {
            osd_correct->f32UpRightX = osd_correct->f32LastRightX > osd_correct->f32UpRightX ? osd_correct->f32LastRightX : osd_correct->f32UpRightX;
        }
    }
    else if (((SVA_OSD_TYPE_CROSS_NO_RECT_TYPE == enBorderType) || (SVA_OSD_TYPE_CROSS_RECT_TYPE == enBorderType))
              && (pstTarInfo->bOsdShow && NULL != pstPkgCntInfo && ((pstPkgCntInfo->astViewTarCnt[0].uiTarCnt[pstTarInfo->uiPkgId][pstTarInfo->merge_type] + pstPkgCntInfo->astViewTarCnt[1].uiTarCnt[pstTarInfo->uiPkgId][pstTarInfo->merge_type]) > 0)))
    {
        if (osd_correct->uiLastY <= osd_correct->uiUpStartY + pstOsdPrm->u32Height)
        {
            osd_correct->uiLastY = osd_correct->uiPkgUpY;
            osd_correct->uiMaxOsdLen = osd_correct->uiTmpOsdLen + uiBorGap;
        }
    
        /* 是否向下排列 */
        if (SAL_TRUE != osd_correct->bDownArranged)
        {
            osd_correct->bDownArranged = (osd_correct->uiLastY <= osd_correct->uiUpStartY + pstOsdPrm->u32Height);
        }
    
        /* 根据上一个OSD纵坐标Y计算当前行OSD的Y坐标 */
        pstOsdPrm->u32X = osd_correct->uiPkgLeftX + osd_correct->uiMaxOsdLen;
        pstOsdPrm->u32Y = osd_correct->bDownArranged ? osd_correct->uiLastY : osd_correct->uiLastY-pstOsdPrm->u32Height;
    
        /* 记录最长的OSD长度和下一次OSD叠加的Y坐标 */
        uiTmp = pstOsdPrm->u32Width + osd_correct->uiMaxOsdLen;
        osd_correct->uiTmpOsdLen = uiTmp > osd_correct->uiTmpOsdLen ? uiTmp : osd_correct->uiTmpOsdLen;
        osd_correct->uiLastY = (SAL_TRUE != osd_correct->bDownArranged) \
                               ? (osd_correct->uiLastY - pstOsdPrm->u32Height) \
                               : (osd_correct->uiLastY + pstOsdPrm->u32Height);
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_osdClearSvaResult
* 描  述  : 清除智能处理结果
* 输  入  : - chan    通道号
* 输  出  : -无
* 返回值  : 成功:SAL_SOK
*         : 失败:SAL_FAIL
*******************************************************************************/
INT32 disp_osdClearSvaResult(UINT32 chan, DISP_CLEAR_SVA_TYPE *clearprm)
{
    DISP_SVA_OUT_CTL *pSvaResault = NULL;
    SVA_PROCESS_OUT *pSvaOut = NULL;
    INT32 i = 0;

    if (chan > 1)
    {
        DISP_LOGE("srcChn %d error\n", chan);
        return SAL_FAIL;
    }

    if (clearprm == NULL)
    {
        DISP_LOGE("chn %d NULL err\n", chan);
        return SAL_FAIL;
    }

    pSvaResault = &g_SvaPosInfo[chan];
    for (i = 0; i < DISP_HAL_XDATA_CNT; i++)
    {
        if (pSvaResault->svaOut[i].TipOrgShow == TIP_TYPE)
        {
            if (clearprm->tip == 0)
            {
                continue;
            }
        }
        else if (pSvaResault->svaOut[i].TipOrgShow == ORGNAIC_TYPE)
        {
            if (clearprm->orgnaic == 0)
            {
                continue;
            }
        }
        else if (pSvaResault->svaOut[i].TipOrgShow == SVA_TYPE)
        {
            if (clearprm->svaclear == 0)
            {
                continue;
            }
        }

        pSvaResault->svaOut[i].bhave = 0;

        pSvaOut = &pSvaResault->svaOut[i].svaPrm;
        pSvaOut->target_num = 0;
        pSvaResault->svaOut[i].needDelayShowTime = 0;
        pSvaResault->svaOut[i].TipOrgShow = SVA_TYPE;
        pSvaResault->total[SVA_TYPE] = 0;
        pSvaResault->total[ORGNAIC_TYPE] = 0;
        pSvaResault->total[TIP_TYPE] = 0;

    }

    pSvaResault->svaCnt = 0;
    /* pSvaResault->reaultCnt = 0; */

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_osdFlickCheck
* 描  述  : 智能信息闪烁时更新基准时间
* 输  入  : - pDispChn:
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
void disp_osdFlickCheck(DISP_CHN_COMMON *pDispChn)
{
    UINT64 flicktime = 0;
    DISPFLICKER_PRM *pstAiFlick = NULL;
    UINT32 i = 0;

    if (pDispChn == NULL)
    {
        DISP_LOGE("null err\n");
        return;
    }

    /* 智能信息闪烁 */
    for (i = 0; i < MAX_ARTICLE_TYPE; i++)
    {
        if (pDispChn->stInModule.uiAiFlag == SAL_FALSE)
        {
            pDispChn->aiflicker[i].currentsystime = 0;
        }

        if ((pDispChn->aiflicker[i].timedisappear > 0) && (pDispChn->stInModule.uiAiFlag == SAL_TRUE))
        {
            pstAiFlick = NULL;
            if (pDispChn->aiflicker[i].currentsystime == 0)
            {
                pDispChn->aiflicker[i].currentsystime = disp_tskGetTimeMilli();
            }

            pstAiFlick = &pDispChn->aiflicker[i];
            if (pstAiFlick == NULL)
            {
                DISP_LOGE("type %d NULL err\n", i);
                continue;
            }

            flicktime = disp_tskGetTimeMilli();

            /* 按照1s 60帧计算 当当前帧是消失帧且下一帧最多是消失帧的最后一帧且闪烁次数超过1000次，更新基准时间 */
            if (((flicktime - pstAiFlick->currentsystime) % (pstAiFlick->timedisappear + pstAiFlick->timedisplay) <= 16)
                && ((flicktime - pstAiFlick->currentsystime) / (pstAiFlick->timedisappear + pstAiFlick->timedisplay) >= 1000))
            {
                pDispChn->aiflicker[i].currentsystime = disp_tskGetTimeMilli();
            }

            if ((flicktime - pstAiFlick->currentsystime) % (pstAiFlick->timedisappear + pstAiFlick->timedisplay) < pstAiFlick->timedisplay)
            {
                if (i == SVA_TYPE)
                {
                    pDispChn->stInModule.uiAiFlagDisappear = 0;
                }
                else if (i == ORGNAIC_TYPE)
                {
                    pDispChn->stInModule.uiorgnaciFlagDisappear = 0;
                }
                else if (i == TIP_TYPE)
                {
                    pDispChn->stInModule.uitipFlagDisappear = 0;
                }
            }
            else
            {
                if (i == SVA_TYPE)
                {
                    pDispChn->stInModule.uiAiFlagDisappear = 1;
                }
                else if (i == ORGNAIC_TYPE)
                {
                    pDispChn->stInModule.uiorgnaciFlagDisappear = 1;
                }
                else if (i == TIP_TYPE)
                {
                    pDispChn->stInModule.uitipFlagDisappear = 1;
                }
            }
        }
        else
        {
            if (i == SVA_TYPE)
            {
                pDispChn->stInModule.uiAiFlagDisappear = 0;
            }
            else if (i == ORGNAIC_TYPE)
            {
                pDispChn->stInModule.uiorgnaciFlagDisappear = 0;
            }
            else if (i == TIP_TYPE)
            {
                pDispChn->stInModule.uitipFlagDisappear = 0;
            }
        }
    }
}


/*******************************************************************************
* 函数名  : disp_osdSetDispFlickPrm
* 描  述  : 设置智能信息闪烁时间
* 输  入  : - uiDev:
*         : - prm  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_osdSetDispFlickPrm(UINT32 uiDev, VOID *prm)
{
    DISPFLICKER_PRM *pstAiFlick = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_DEV_COMMON *pDispDev = NULL;
    ARTICLE_TYPE type = SVA_TYPE;

    pDispDev = disp_tskGetVoDev(uiDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("uiDev:%d NULL error \n", uiDev);
        return SAL_FAIL;
    }

    pstAiFlick = (DISPFLICKER_PRM *)prm;
    if (pstAiFlick == NULL)
    {
        DISP_LOGE("pstAiFlick NULL error \n");
        return SAL_FAIL;
    }

    if (pstAiFlick->vochn >= disp_tskGetVoChnCnt(uiDev))
    {
        DISP_LOGE("vochn:%d error \n", pstAiFlick->vochn);
        return SAL_FAIL;
    }

    pDispChn = disp_tskGetVoChn(uiDev, pstAiFlick->vochn);
    if (pDispChn == NULL)
    {
        DISP_LOGE("chn:%d pDispChn NULL error !\n", pstAiFlick->vochn);
        return SAL_FAIL;
    }

    type = pstAiFlick->type;

    if (pstAiFlick->timedisplay == 0)
    {
        DISP_LOGE("chn:%d Flick Prm err\n", pstAiFlick->vochn);
        return SAL_FAIL;
    }

    if (type >= MAX_ARTICLE_TYPE)
    {
        DISP_LOGE("chn %d type %d err\n", uiDev, type);
        return SAL_FAIL;
    }

    pDispChn->aiflicker[type].vochn = pstAiFlick->vochn;
    pDispChn->aiflicker[type].timedisplay = pstAiFlick->timedisplay;
    pDispChn->aiflicker[type].timedisappear = pstAiFlick->timedisappear;
    if (pDispChn->uiEnable == SAL_FALSE)
    {
        pDispChn->aiflicker[type].currentsystime = 0;
        DISP_LOGW("chn:%d uiEnable:%d err\n", pstAiFlick->vochn, pDispChn->uiEnable);
    }
    else
    {
        pDispChn->aiflicker[type].currentsystime = disp_tskGetTimeMilli();

    }

    DISP_LOGI("uidev %d, vochn %d type %d set Flick prm success\n", uiDev, pstAiFlick->vochn, type);

    return SAL_SOK;

}


/*******************************************************************************
* 函数名  : Disp_drvSetAiLineTypePrm
* 描  述  : 设置智能信息画线类型（虚线、实线、点画线及具体参数）
* 输  入  : - uiDev:
*         : - prm  :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_osdSetLineTypePrm(UINT32 uiDev, VOID *prm)
{
    DISPLINE_PRM *pstAiLineType = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_DEV_COMMON *pDispDev = NULL;
    ARTICLE_TYPE type = SVA_TYPE;

    pDispDev = disp_tskGetVoDev(uiDev);
    if (pDispDev == NULL)
    {
        DISP_LOGE("uiDev:%d NULL error \n", uiDev);
        return SAL_FAIL;
    }

    pstAiLineType = (DISPLINE_PRM *)prm;
    if (pstAiLineType == NULL)
    {
        DISP_LOGE("pstAiLineType prm NULL error \n");
        return SAL_FAIL;
    }

    if (pstAiLineType->vochn >= disp_tskGetVoChnCnt(uiDev))
    {
        DISP_LOGE("vochn:%d error \n", pstAiLineType->vochn);
        return SAL_FAIL;
    }

    pDispChn = disp_tskGetVoChn(uiDev, pstAiLineType->vochn);
    if (pDispChn == NULL)
    {
        DISP_LOGE("chn:%d pDispChn NULL error !\n", pstAiLineType->vochn);
        return SAL_FAIL;
    }

    if (pstAiLineType->frametype >= DISP_FRAME_TYPE_MAX)
    {
        DISP_LOGE("chn:%d Prm frametype err\n", pstAiLineType->vochn);
        return SAL_FAIL;
    }

    type = pstAiLineType->type;
    if (type >= MAX_ARTICLE_TYPE)
    {
        DISP_LOGE("chn %d type %d err\n", uiDev, type);
        return SAL_FAIL;
    }

    if (pstAiLineType->frametype == DISP_FRAME_TYPE_DOTTEDLINE)
    {
        if ((pstAiLineType->fulllinelength == 0) || (pstAiLineType->gaps == 0) || (pstAiLineType->node != 0))
        {
            DISP_LOGE("chn:%d Prm length err\n", pstAiLineType->vochn);
            return SAL_FAIL;
        }
    }

    if (pstAiLineType->frametype == DISP_FRAME_TYPE_DASHDOTLINE)
    {
        if ((pstAiLineType->fulllinelength == 0) || (pstAiLineType->gaps == 0) || (pstAiLineType->node == 0))
        {
            DISP_LOGE("chn:%d Prm length err\n", pstAiLineType->vochn);
            return SAL_FAIL;
        }
    }

    if ((pstAiLineType->fulllinelength >= pDispChn->uiW) || (pstAiLineType->fulllinelength >= pDispChn->uiH)
        || (pstAiLineType->gaps >= pDispChn->uiW) || (pstAiLineType->gaps >= pDispChn->uiH))
    {
        DISP_LOGW("chn:%d Prm fulllength %d gaps %d err\n", pstAiLineType->vochn, pstAiLineType->fulllinelength, pstAiLineType->gaps);
    }

    pstAiLineType->fulllinelength = SAL_align(pstAiLineType->fulllinelength, 2);
    pstAiLineType->gaps = SAL_align(pstAiLineType->gaps, 2);

    if (type == SVA_TYPE)
    {
        pDispChn->articlelinetype.ailine.vochn = pstAiLineType->vochn;
        pDispChn->articlelinetype.ailine.frametype = pstAiLineType->frametype;
        pDispChn->articlelinetype.ailine.fulllinelength = pstAiLineType->fulllinelength;
        pDispChn->articlelinetype.ailine.gaps = pstAiLineType->gaps;
        pDispChn->articlelinetype.ailine.node = pstAiLineType->node;
    }
    else if (type == ORGNAIC_TYPE)
    {
        pDispChn->articlelinetype.orgnaicline.vochn = pstAiLineType->vochn;
        pDispChn->articlelinetype.orgnaicline.frametype = pstAiLineType->frametype;
        pDispChn->articlelinetype.orgnaicline.fulllinelength = pstAiLineType->fulllinelength;
        pDispChn->articlelinetype.orgnaicline.gaps = pstAiLineType->gaps;
        pDispChn->articlelinetype.orgnaicline.node = pstAiLineType->node;
    }
    else if (type == TIP_TYPE)
    {
        pDispChn->articlelinetype.tipline.vochn = pstAiLineType->vochn;
        pDispChn->articlelinetype.tipline.frametype = pstAiLineType->frametype;
        pDispChn->articlelinetype.tipline.fulllinelength = pstAiLineType->fulllinelength;
        pDispChn->articlelinetype.tipline.gaps = pstAiLineType->gaps;
        pDispChn->articlelinetype.tipline.node = pstAiLineType->node;
    }
    else
    {
        DISP_LOGW("chn %d type %d err\n", pstAiLineType->vochn, type);
    }

    DISP_LOGI("uidev %d, vochn %d type %d set Line prm success(for analysis)\n", uiDev, pstAiLineType->vochn, type);
    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_osdSvaPrmChange
* 描  述  : 智能信息颜色、名字修正
* 输  入  : - chan    通道号
* 输  出  : -无
* 返回值  : 成功:SAL_SOK
*         : 失败:SAL_FAIL
*******************************************************************************/
INT32 disp_osdSvaPrmChange(INT32 chan, SVA_PROCESS_IN *pstIn, INT32 colorFlag, INT32 nameflg)
{
    INT32 i = 0, j = 0;
    DISP_SVA_OUT_CTL *pSvaResault = NULL;
    SVA_PROCESS_OUT *pSvaOut = NULL;
    UINT32 type = 0;

    if (chan > 1)
    {
        DISP_LOGE("srcChn %d error\n", chan);
        return SAL_FAIL;
    }

    if (pstIn == NULL)
    {
        DISP_LOGE("chan %d pstIn == NULL err\n", chan);
        return SAL_FAIL;
    }

    pSvaResault = &g_SvaPosInfo[chan];

    for (i = 0; i < DISP_HAL_XDATA_CNT; i++)
    {
        if (pSvaResault->svaOut[i].bhave == 1)
        {
            pSvaOut = &pSvaResault->svaOut[i].svaPrm;
            for (j = 0; j < pSvaOut->target_num; j++)
            {
                type = pSvaOut->target[j].type;

				if (type >= SVA_MAX_ALARM_TYPE || !pstIn->alert[type].bInit)
				{
					DISP_LOGW("invalid type! type %d \n", type);
					continue;
				}
				
				if (colorFlag)
				{
					pSvaOut->target[j].color = pstIn->alert[type].sva_color;
				}
				
				if (nameflg)
				{
					sal_memcpy_s(pSvaOut->target[j].sva_name, SVA_ALERT_NAME_LEN, pstIn->alert[type].sva_name, SVA_ALERT_NAME_LEN);
				}
            }
        }
    }

    return SAL_SOK;
}



/*******************************************************************************
* 函数名  : disp_osdEnableVoAiFlag
* 描  述  :dev对应所有vo通道的osd进行使能
* 输  入  : - uiVoDev:
*         : - uiModId   :
*         : - prm       :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_osdEnableVoAiFlag(UINT32 uiVoDev, VOID *prm)
{
    UINT32 i = 0;
    UINT32 uiVoChn = 0;
    UINT32 uiChnNum = 0;
    UINT32 uiEnable = 0;
    UINT32 uiVoMaxCnt = 0;

    DSP_DISP_OSD_PARAM *pDispRgn = NULL;
    DISP_CHN_COMMON *pDispChn = NULL;

    if ((NULL == prm) || (DISP_MAX_DEV_NUM <= uiVoDev))
    {
        DISP_LOGE("ptr == NULL or vodev[%d] invalid!\n", uiVoDev);
        return SAL_FAIL;
    }

    /* 参数类型转换 */
    pDispRgn = (DSP_DISP_OSD_PARAM *)prm;
    uiChnNum = pDispRgn->uiChnNum;
    uiVoMaxCnt = disp_tskGetVoChnCnt(uiVoDev);

    /* 为避免同源参数影响，配置OSD开关前对使能参数进行初始化 */
    for (i = 0; i < uiVoMaxCnt; i++)
    {
        pDispChn = disp_tskGetVoChn(uiVoDev, i);
        if (pDispChn == NULL)
        {
            DISP_LOGE("error\n");
            return SAL_FAIL;
        }

        pDispChn->stInModule.uiAiFlag = SAL_FALSE;
    }

    /* 实现指定uiDev下各通道的osd显示开关 */
    for (i = 0; i < uiChnNum; i++)
    {
        uiVoChn = pDispRgn->uiVoChn[i];
        uiEnable = pDispRgn->uiEnable[i];

        pDispChn = disp_tskGetVoChn(uiVoDev, i);
        if (pDispChn == NULL)
        {
            DISP_LOGE("error\n");
            return SAL_FAIL;
        }

        pDispChn->stInModule.uiAiFlag = uiEnable;
        DISP_LOGI("uiChnNum %d voDev %d uiVoChn %d uiEnable %d \n", uiChnNum, uiVoDev, uiVoChn, uiEnable);
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_osdSetVoAiSwitch
* 描  述  : 设置是否智能显示
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_osdSetVoAiSwitch(UINT32 vodev, void *prm)
{
    INT32 s32Ret = SAL_SOK;
    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_SVA_SWITCH *ai_switch = (DISP_SVA_SWITCH *)prm;

    if ((NULL == prm) || (DISP_MAX_DEV_NUM <= vodev))
    {
        DISP_LOGE("ptr == NULL or vodev[%d] invalid!\n", vodev);
        return SAL_FAIL;
    }

    pDispChn = disp_tskGetVoChn(vodev, ai_switch->chn);
    if (pDispChn == NULL)
    {
        DISP_LOGE("vodev %d chn %d  pDispChn null err\n", vodev, ai_switch->chn);
        return SAL_FAIL;
    }

    if (pDispChn->stInModule.g_AiDispSwitch == ai_switch->dispSvaSwitch)
    {
        DISP_LOGW("Switch is Setted The same status %d !\n",pDispChn->stInModule.g_AiDispSwitch);
    }
    else
    {
        pDispChn->stInModule.g_AiDispSwitch = ai_switch->dispSvaSwitch;
        DISP_LOGI("Sva Disp status is Setted as %d\n", pDispChn->stInModule.g_AiDispSwitch);
    }

    DISP_LOGI("Ai vodev %d chan %d vichn %d statu %d switch set success\n",vodev,ai_switch->chn,ai_switch->uidevchn,ai_switch->dispSvaSwitch);
    return s32Ret;
}

/**
* @function:   Sva_HalInitCocrtInfo
* @brief:      填充十字框四角位置参数
* @param[in]:  VGS_DRAW_LINE_PRM *pstLineArr
* @param[in]:  DISPLINE_PRM *pstLineType
* @param[in]:  UINT32 uiX
* @param[in]:  UINT32 uiY
* @param[in]:  UINT32 uiW
* @param[in]:  UINT32 uiH
* @param[in]:  UINT32 uiThick
* @param[in]:  UINT32 uiColor
* @return:     INT32
*/
static INT32 disp_osdCalCrossLoc(VGS_DRAW_LINE_PRM *pstLineArr,
                          DISPLINE_PRM *pstLineType,
                          UINT32 uiX, UINT32 uiY, UINT32 uiW, UINT32 uiH,
                          UINT32 uiThick, UINT32 uiColor)
{
    UINT32 uiCurLineNum = 0;
    UINT32 uiLineLen = 10;

    VGS_DRAW_PRM_S *pstLinePrm = NULL;

    if (!pstLineArr || !pstLineType)
    {
        DISP_LOGE("ptr null! %p, %p \n", pstLineArr, pstLineType);
        return SAL_FAIL;
    }

    uiCurLineNum = pstLineArr->uiLineNum;

    /* left-up */
    pstLinePrm = &pstLineArr->stVgsLinePrm[uiCurLineNum];	
    pstLinePrm->stStartPoint.s32X = uiX;
    pstLinePrm->stStartPoint.s32Y = uiY;
    pstLinePrm->stEndPoint.s32X = uiX;
    pstLinePrm->stEndPoint.s32Y = uiY + uiLineLen + uiThick;
    pstLinePrm->u32Thick = uiThick;
    pstLinePrm->u32Color = uiColor;
    pstLineArr->af32XFract[uiCurLineNum] = 0.0f;
    memcpy(&pstLineArr->linetype[uiCurLineNum++], pstLineType, sizeof(DISPLINE_PRM));

    pstLinePrm = &pstLineArr->stVgsLinePrm[uiCurLineNum];	
    pstLinePrm->stStartPoint.s32X = uiX;
    pstLinePrm->stStartPoint.s32Y = uiY;
    pstLinePrm->stEndPoint.s32X = uiX + uiLineLen + uiThick;
    pstLinePrm->stEndPoint.s32Y = uiY;
    pstLinePrm->u32Thick = uiThick;
    pstLinePrm->u32Color = uiColor;
    pstLineArr->af32XFract[uiCurLineNum] = 0.0f;
    memcpy(&pstLineArr->linetype[uiCurLineNum++], pstLineType, sizeof(DISPLINE_PRM));

    /* left-down */
    pstLinePrm = &pstLineArr->stVgsLinePrm[uiCurLineNum];	
    pstLinePrm->stStartPoint.s32X = uiX;
    pstLinePrm->stStartPoint.s32Y = uiY + uiH - uiLineLen;
    pstLinePrm->stEndPoint.s32X = uiX;
    pstLinePrm->stEndPoint.s32Y = uiY + uiH + uiThick;
    pstLinePrm->u32Thick = uiThick;
    pstLinePrm->u32Color = uiColor;
    pstLineArr->af32XFract[uiCurLineNum] = 0.0f;
    memcpy(&pstLineArr->linetype[uiCurLineNum++], pstLineType, sizeof(DISPLINE_PRM));

    pstLinePrm = &pstLineArr->stVgsLinePrm[uiCurLineNum];	
    pstLinePrm->stStartPoint.s32X = uiX;
    pstLinePrm->stStartPoint.s32Y = uiY + uiH;
    pstLinePrm->stEndPoint.s32X = uiX + uiLineLen + uiThick;
    pstLinePrm->stEndPoint.s32Y = uiY + uiH;
    pstLinePrm->u32Thick = uiThick;
    pstLinePrm->u32Color = uiColor;
    pstLineArr->af32XFract[uiCurLineNum] = 0.0f;
    memcpy(&pstLineArr->linetype[uiCurLineNum++], pstLineType, sizeof(DISPLINE_PRM));

    /* right-up */
    pstLinePrm = &pstLineArr->stVgsLinePrm[uiCurLineNum];	
    pstLinePrm->stStartPoint.s32X = uiX + uiW - uiLineLen;
    pstLinePrm->stStartPoint.s32Y = uiY;
    pstLinePrm->stEndPoint.s32X = uiX + uiW;
    pstLinePrm->stEndPoint.s32Y = uiY;
    pstLinePrm->u32Thick = uiThick;
    pstLinePrm->u32Color = uiColor;
    pstLineArr->af32XFract[uiCurLineNum] = 0.0f;
    memcpy(&pstLineArr->linetype[uiCurLineNum++], pstLineType, sizeof(DISPLINE_PRM));

    pstLinePrm = &pstLineArr->stVgsLinePrm[uiCurLineNum];	
    pstLinePrm->stStartPoint.s32X = uiX + uiW;
    pstLinePrm->stStartPoint.s32Y = uiY;
    pstLinePrm->stEndPoint.s32X = uiX + uiW;
    pstLinePrm->stEndPoint.s32Y = uiY + uiLineLen + uiThick;
    pstLinePrm->u32Thick = uiThick;
    pstLinePrm->u32Color = uiColor;
    pstLineArr->af32XFract[uiCurLineNum] = 0.0f;
    memcpy(&pstLineArr->linetype[uiCurLineNum++], pstLineType, sizeof(DISPLINE_PRM));

    /* right-down */
    pstLinePrm = &pstLineArr->stVgsLinePrm[uiCurLineNum];	
    pstLinePrm->stStartPoint.s32X = uiX + uiW - uiLineLen;
    pstLinePrm->stStartPoint.s32Y = uiY + uiH;
    pstLinePrm->stEndPoint.s32X = uiX + uiW + uiThick;
    pstLinePrm->stEndPoint.s32Y = uiY + uiH;
    pstLinePrm->u32Thick = uiThick;
    pstLinePrm->u32Color = uiColor;
    pstLineArr->af32XFract[uiCurLineNum] = 0.0f;
    memcpy(&pstLineArr->linetype[uiCurLineNum++], pstLineType, sizeof(DISPLINE_PRM));

    pstLinePrm = &pstLineArr->stVgsLinePrm[uiCurLineNum];	
    pstLinePrm->stStartPoint.s32X = uiX + uiW;
    pstLinePrm->stStartPoint.s32Y = uiY + uiH - uiLineLen;
    pstLinePrm->stEndPoint.s32X = uiX + uiW;
    pstLinePrm->stEndPoint.s32Y = uiY + uiH + uiThick;
    pstLinePrm->u32Thick = uiThick;
    pstLinePrm->u32Color = uiColor;
    pstLineArr->af32XFract[uiCurLineNum] = 0.0f;
    memcpy(&pstLineArr->linetype[uiCurLineNum++], pstLineType, sizeof(DISPLINE_PRM));

    pstLineArr->uiLineNum = uiCurLineNum;

    return SAL_SOK;
}
                
/**
* @function:   Sva_HalInitCocrtInfo
* @brief:      初始化矫正参数，按照画面中所有的包裹进行初始化 
* @param[in]:  DISP_CHN_COMMON *pDispChn
* @param[in]:  UINT32 uiPicW
* @param[in]:  UINT32 uiPicH
* @param[in]:  SVA_PROCESS_IN *pstIn
* @param[in]:  SVA_PROCESS_OUT *pstOut
* @param[in]:  DISP_OSD_COORDINATE_CORRECT *pCorctPtrArr
* @param[in]:  UINT32 uiCnt
* @return:     INT32
*/
INT32 disp_osdInitCocrtInfo(DISP_CHN_COMMON *pDispChn,
                          UINT32 uiPicW, 
                          UINT32 uiPicH, 
                          SVA_PROCESS_IN *pstIn, 
                          SVA_PROCESS_OUT *pstOut, 
                          DISP_OSD_COORDINATE_CORRECT *pCorctPtrArr)
{
    UINT32 i = 0;
    UINT32 uiPkgX = 0;
    UINT32 uiPkgY = 0;

    DISP_OSD_COORDINATE_CORRECT *pstCorctInfo = NULL;

    if (NULL == pstIn 
        || NULL == pstOut 
        || NULL == pCorctPtrArr)
    {
        DISP_LOGE("ptr null! %p, %p, %p \n", pstIn, pstOut, pCorctPtrArr);
        return SAL_FAIL;
    }

    if (SVA_OSD_TYPE_CROSS_NO_RECT_TYPE == pstIn->drawType
        || SVA_OSD_TYPE_CROSS_RECT_TYPE == pstIn->drawType)
    {
        for(i = 0; i <  pstOut->packbagAlert.OriginalPackageNum; i++)
        {
            uiPkgX = SAL_align((UINT32)(pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.x * (float)uiPicW), 2);
            uiPkgY = SAL_align((UINT32)(pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.y * (float)uiPicH), 2);

            pstCorctInfo = pCorctPtrArr + (i % SVA_MAX_PACKAGE_BUF_NUM);

            pstCorctInfo->uiUpOsdFlag = 1;
            pstCorctInfo->uiUpStartY = 2;       /* 最上面空余两个像素 */
            pstCorctInfo->uiTmpOsdY = 2;
            pstCorctInfo->uiAiOffsetH = 0;
            pstCorctInfo->uiAiCropH = (NULL != pDispChn) ? pDispChn->enlargeprm.uiCropH : 0;

            pstCorctInfo->uiPkgLeftX = uiPkgX;
            pstCorctInfo->uiPkgUpY = uiPkgY;
            pstCorctInfo->uiLastY = (pstCorctInfo->uiPkgUpY > 4 ? pstCorctInfo->uiPkgUpY - 4 : pstCorctInfo->uiPkgUpY);
            pstCorctInfo->uiMaxOsdLen = 0;
            pstCorctInfo->bDownArranged = SAL_FALSE;
        }
    }
    else
    {
        /* 非包裹框选类型，只需要初始化一个矫正参数即可 */
        pstCorctInfo = pCorctPtrArr;

        pstCorctInfo->uiUpOsdFlag = 1;
        pstCorctInfo->uiUpStartY = 2;       /* 最上面空余两个像素 */
        pstCorctInfo->uiTmpOsdY = 2;
        pstCorctInfo->uiAiOffsetH = 0;
        pstCorctInfo->uiAiCropH = (NULL != pDispChn) ? pDispChn->enlargeprm.uiCropH : 0;

        pstCorctInfo->uiPkgLeftX = uiPkgX;
        pstCorctInfo->uiPkgUpY = uiPkgY;
        pstCorctInfo->uiLastY = pstCorctInfo->uiUpStartY;   /* 默认从画面上部往下进行排列 */
        pstCorctInfo->uiMaxOsdLen = 0;
        pstCorctInfo->bDownArranged = SAL_FALSE;
    }

    return SAL_SOK;
}

/**
 * @function:   disp_osdGetRealOut
 * @brief:      获取真实输出
 * @param[in]:  SVA_PROCESS_IN *pstIn
 * @param[out]: SVA_PROCESS_OUT *pstOut
 * @return:     INT32
 */
INT32 disp_osdGetRealOut(UINT32 chan, SVA_PROCESS_IN *pstIn, SVA_PROCESS_OUT *pstOut, DISP_TAR_CNT_INFO *pstPkgCntInfo)
{
    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 k = 0;
    UINT32 uiPkgIdx = 0;

    float fPkgX = 0;
    float fPkgY = 0;
    float fPkgW = 0;
    float fPkgH = 0;
    float fMidX = 0;
    float fMidY = 0;

    UINT32 uiExistIdxCnt[SVA_MAX_PACKAGE_BUF_NUM] = {0};                           /* 当前已出现过的目标类别个数 */
    UINT32 auiExistIdx[SVA_MAX_PACKAGE_BUF_NUM][SVA_MAX_ALARM_TYPE] = {0};         /* 当前已出现过的目标类别 */
    UINT32 auiExistTarCnt[SVA_MAX_PACKAGE_BUF_NUM][SVA_MAX_ALARM_TYPE][DISP_SVA_DEV_MAX] = {0};      /* 当前已出现过的目标个数, 用于进行告警个数过滤 */
    UINT32 auiExistTarIdx[SVA_MAX_PACKAGE_BUF_NUM][SVA_MAX_ALARM_NUM] = {0};       /* 当前已出现过的目标索引下标 */
    UINT32 uiMainViewChn = 0;
    
    uiMainViewChn= Sva_DrvGetSyncMainChan();
    if(uiMainViewChn >= DISP_SVA_DEV_MAX)
    {
        DISP_LOGE("Get main view chn failed %u\n", uiMainViewChn);
        return SAL_FAIL;
    }

    if (chan >= DISP_SVA_DEV_MAX || NULL == pstIn || NULL == pstOut)
    {
        DISP_LOGE("chan err %u or ptr err! %p, %p or \n", chan, pstIn, pstOut);
        return SAL_FAIL;
    }

	/* 智能模块和显示公用此接口，智能内部暂不需要使用计数，此入参可能为空 */
	if (!pstPkgCntInfo)
	{
		return SAL_SOK;
	}

    /* 清空上一次保留的重复OSD个数 */
    memset(pstPkgCntInfo, 0x00, sizeof(DISP_TAR_CNT_INFO));

    for (i = 0; i < pstOut->target_num; i++)
    {
        fMidX = pstOut->target[i].rect.x + pstOut->target[i].rect.width / 2.0;
        fMidY = pstOut->target[i].rect.y + pstOut->target[i].rect.height / 2.0;

        for(k = 0; k< pstOut->packbagAlert.OriginalPackageNum; k++)
        {
            fPkgX = pstOut->packbagAlert.OriginalPackageLoc[k].PackageRect.x;
            fPkgY = pstOut->packbagAlert.OriginalPackageLoc[k].PackageRect.y;
            fPkgW = pstOut->packbagAlert.OriginalPackageLoc[k].PackageRect.width;
            fPkgH = pstOut->packbagAlert.OriginalPackageLoc[k].PackageRect.height;

            if ((fMidX <= fPkgX || fMidX >= fPkgX + fPkgW)
                || (fMidY <= fPkgY || fMidY >= fPkgY + fPkgH))
            {
                continue;
            }
            break;
        }

        uiPkgIdx = k;
        
        pstOut->target[i].bRectShow = SAL_TRUE;
        pstOut->target[i].uiPkgId = k;              /* 记录目标包含在那个包裹中 */

        for (j = 0; j < uiExistIdxCnt[uiPkgIdx]; j++)
        {
            if (auiExistIdx[uiPkgIdx][j] == pstOut->target[i].merge_type)
            {
                break;
            }
        }

        if (j < uiExistIdxCnt[uiPkgIdx])
        {
            /* 主辅视角违禁品数目分类 */
            if (chan == uiMainViewChn)
            {
                /* 主通道显示 */
                if(SAL_TRUE == pstOut->target[i].bAnotherViewTar)
                {
                    auiExistTarCnt[uiPkgIdx][auiExistIdx[uiPkgIdx][j]][1 - uiMainViewChn]++;
                }
                else
                {
                    auiExistTarCnt[uiPkgIdx][auiExistIdx[uiPkgIdx][j]][uiMainViewChn]++;
                }
            }
            else
            {
                /* 辅通道显示 */
                if(SAL_TRUE == pstOut->target[i].bAnotherViewTar)
                {
                    auiExistTarCnt[uiPkgIdx][auiExistIdx[uiPkgIdx][j]][uiMainViewChn]++;
                }
                else
                {
                    auiExistTarCnt[uiPkgIdx][auiExistIdx[uiPkgIdx][j]][1 - uiMainViewChn]++;
                }
            }
            continue;
        }

		pstOut->target[i].bOsdShow = SAL_TRUE;
        /* 主辅视角违禁品数目分类 */
        if (chan == uiMainViewChn)
        {
            if(SAL_TRUE == pstOut->target[i].bAnotherViewTar)
            {
                auiExistTarCnt[uiPkgIdx][pstOut->target[i].merge_type][1 - uiMainViewChn]++;
            }
            else
            {
                auiExistTarCnt[uiPkgIdx][pstOut->target[i].merge_type][uiMainViewChn]++;
            }
        }
        else
        {
            if(SAL_TRUE == pstOut->target[i].bAnotherViewTar)
            {
                auiExistTarCnt[uiPkgIdx][pstOut->target[i].merge_type][uiMainViewChn]++;
            }
            else
            {
                auiExistTarCnt[uiPkgIdx][pstOut->target[i].merge_type][1 - uiMainViewChn]++;
            }
        }
        auiExistIdx[uiPkgIdx][uiExistIdxCnt[uiPkgIdx]] = pstOut->target[i].merge_type;
        auiExistTarIdx[uiPkgIdx][uiExistIdxCnt[uiPkgIdx]++] = i;

        DISP_LOGD("sorting: i %d, uiPkgIdx %d, auiExistTarCnt %d, auiExistIdx %d, auiExistTarIdx %d, uiExistIdxCnt %d, merge_type %d \n",
                  i, uiPkgIdx, auiExistTarCnt[uiPkgIdx][uiExistIdxCnt[uiPkgIdx] - 1][uiMainViewChn] + auiExistTarCnt[uiPkgIdx][uiExistIdxCnt[uiPkgIdx] - 1][1 - uiMainViewChn],
                  auiExistIdx[uiPkgIdx][uiExistIdxCnt[uiPkgIdx] - 1],
                  auiExistTarIdx[uiPkgIdx][uiExistIdxCnt[uiPkgIdx] - 1],
                  uiExistIdxCnt[uiPkgIdx],
                  pstOut->target[i].merge_type);
    }

    for(k = 0; k < pstOut->packbagAlert.OriginalPackageNum; k++)
    {
        DISP_LOGD("k %d, uiExistIdxCnt %d \n", k, uiExistIdxCnt[k]);
        for(i = 0; i < uiExistIdxCnt[k]; i++)
        {
            if ((auiExistTarCnt[k][auiExistIdx[k][i]][uiMainViewChn] + auiExistTarCnt[k][auiExistIdx[k][i]][1 - uiMainViewChn]) < pstIn->alert[auiExistIdx[k][i]].sva_alert_tar_cnt)
            {
                continue;
            }

            DISP_LOGD("save: pkgId %d, auiExistTarCnt %d %d, x %f \n", 
                      k, auiExistTarCnt[k][i][uiMainViewChn], auiExistTarCnt[k][i][1 - uiMainViewChn],
                      pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.x);

			pstPkgCntInfo->astViewTarCnt[uiMainViewChn].uiTarCnt[k][auiExistIdx[k][i]] = auiExistTarCnt[k][auiExistIdx[k][i]][uiMainViewChn];
			pstPkgCntInfo->astViewTarCnt[1 - uiMainViewChn].uiTarCnt[k][auiExistIdx[k][i]] = auiExistTarCnt[k][auiExistIdx[k][i]][1 - uiMainViewChn];
            //pstOut->target[auiExistTarIdx[k % SVA_MAX_PACKAGE_BUF_NUM][i]].rect.x = pstOut->stSvaPkgBufInfo.stPkgLocInfo[k % SVA_MAX_PACKAGE_BUF_NUM].x;
        }
    }

    return SAL_SOK;
}


/**
 * @function:   disp_osdDbgDrawPkgRect
 * @brief:      十字框选类型框选
 * @param[in]:  SVA_PROCESS_IN *pstIn
 * @param[out]: SVA_PROCESS_OUT *pstOut
 * @return:     INT32
 */
VOID disp_osdDbgDrawPkgRect(UINT32 uiPicW, UINT32 uiPicH, SVA_PROCESS_OUT *pstOut, VGS_DRAW_LINE_PRM *pstCpuLineArray, DISPLINE_PRM *pstLineType)
{
    UINT32 i = 0;
    UINT32 u32PkgX = 0;
    UINT32 u32PkgY = 0;
    UINT32 u32PkgW = 0;
    UINT32 u32PkgH = 0;

    for(i = 0; i < pstOut->packbagAlert.OriginalPackageNum; i++)
    {
        DISP_LOGD("disp: get i %d, [%f, %f] [%f, %f] \n", i % SVA_MAX_PACKAGE_BUF_NUM,
                  pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.x,
                  pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.y,
                  pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.width,
                  pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.height);

        u32PkgX = SAL_align((UINT32)(pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.x * (float)uiPicW), 2);
        u32PkgY = SAL_align((UINT32)(pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.y * (float)uiPicH), 2);
        u32PkgW = SAL_align((UINT32)(pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.width * (float)uiPicW), 2);
        u32PkgH = SAL_align((UINT32)(pstOut->packbagAlert.OriginalPackageLoc[i].PackageRect.height * (float)uiPicH), 2);

        if(u32PkgW == 0 || u32PkgH == 0)
        {
            DISP_LOGE("disp failed! w %d, h %d \n", u32PkgW, u32PkgH);
            continue;
        }
        (VOID)disp_osdCalCrossLoc(pstCpuLineArray,
                                  pstLineType,
                                  u32PkgX, u32PkgY, u32PkgW, u32PkgH,
                                  2, 0x000000);
#if 0
        pstCpuRect = pstCpuRectArray->astRect + pstCpuRectArray->u32RectNum++;
        pstCpuRect->s32X = ALIGN_UP((UINT32)(pstOut->stSvaPkgBufInfo.stPkgLocInfo[i % SVA_MAX_PACKAGE_BUF_NUM].x * (float)uiPicW), 2);
        pstCpuRect->s32Y = ALIGN_UP((UINT32)(pstOut->stSvaPkgBufInfo.stPkgLocInfo[i % SVA_MAX_PACKAGE_BUF_NUM].y * (float)uiPicH), 2);
        pstCpuRect->u32Width = ALIGN_DOWN((UINT32)(pstOut->stSvaPkgBufInfo.stPkgLocInfo[i % SVA_MAX_PACKAGE_BUF_NUM].width * (float)uiPicW), 2);
        pstCpuRect->u32Height = ALIGN_DOWN((UINT32)(pstOut->stSvaPkgBufInfo.stPkgLocInfo[i % SVA_MAX_PACKAGE_BUF_NUM].height * (float)uiPicH), 2);
        pstCpuRect->f32StartXFract = -1;//pSvaRect[j].f32StartXFract;
        pstCpuRect->f32EndXFract = -1;//pSvaRect[j].f32EndXFract;
        pstCpuRect->u32Color = 0x000000;
        pstCpuRect->u32Thick = 4;
        memcpy(&pstCpuRect->stLinePrm, pstLineType, sizeof(*pstLineType));
#endif
    }

    return;
}

/**
 * @function   disp_osdSetDispLineWidthPrm
 * @brief      设置画框线宽,为兼容安检机直接修改能力集的
               线宽
 * @param[in]  UINT32 uiDev  
 * @param[in]  VOID *prm     
 * @param[out] None
 * @return     INT32
 */
INT32 disp_osdSetDispLineWidthPrm(UINT32 uiDev, VOID *prm)
{
    UINT32 *u32LineWidth = (UINT32 *)prm;
    UINT32  u32OldLineWidth =  capb_get_line()->u32DispLineWidth;

    /* 因为画线用cpu在yuv420上所以画线最少2像素。当前只支持2和4像素其他未测试 */
    /* 2022/08/10: rk平台显示画线采用的rgb数据,最低支持1像素,放开此处2像素的限制,需要应用根据不同平台来做限制 */
    if(*u32LineWidth == 2  || *u32LineWidth == 4 || *u32LineWidth == 1)
    {
        if(*u32LineWidth != u32OldLineWidth)
        {
            capb_get_line()->u32DispLineWidth = *u32LineWidth;
        }
    }
    else
    {
        DISP_LOGW("unsuported lineWidth = %d !!!\n",*u32LineWidth);
        return SAL_SOK;
    }
   
    DISP_LOGI("display LineWidth change; oldLineWidth = %d lineWidth = %d !!!\n",u32OldLineWidth, *u32LineWidth);

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : disp_osdAiDrawFrame
* 描  述  : 
* 输  入  : - prm           :
*         : - pstSystemFrame:
*         : - pstOut        :
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_osdAiDrawFrame(VOID *prm, SYSTEM_FRAME_INFO *pstSystemFrame, SVA_PROCESS_OUT *pstOut, SVA_PROCESS_IN *pstIn, DISP_TAR_CNT_INFO *pstPkgCntInfo)
{
    /* pstOut 是根据窗口传递的智能信息 */
    INT32 s32DspRet = SAL_FAIL;
    INT32 s32Ret = SAL_SOK;
    UINT32 VoLayer = 0;
    UINT32 VoChn = 0;
    UINT32 voDev = 0;
    UINT32 viChn = 0; /* 采集通道 */
    UINT32 uiPicW = 0;    /* 图像宽 */
    UINT32 uiPicH = 0;    /* 图像高 */
    INT32 alert_num = 0;
    UINT32 direction = 0;
    UINT32 j = 0;
    UINT32 u32LatSize = 0;
    UINT32 u32FontSize = 0;
    UINT32 u32PkgId = 0;
    
    UINT32 uiConfidence = 0;
    UINT32 scaleLevel = 0;
    UINT32 uiColor = 0;
    UINT32 reliability = 0;  /* 0~100 */
    UINT32 uiSubTypeIdx = 0;
    UINT32 indx = 0;
    UINT32 u32MergeType = 0;

    SVA_BORDER_TYPE_E drawType = SVA_OSD_NORMAL_TYPE;

    UINT32  fGAlpha;
    UINT32  BgAlpha;
    UINT32  numfGAlpha;
    UINT32  numBgAlpha;

    DISP_CHN_COMMON *pDispChn = NULL;
    DISP_SVA_RECT_F *pSvaRect = NULL;
    VGS_ARTICLE_LINE_TYPE *articlelinetype = NULL;
    DISPLINE_PRM *pstLineType = NULL;
    
    DISP_OSD_COORDINATE_CORRECT *pstOsd_corct = NULL;
    DISP_OSD_COORDINATE_CORRECT osd_correct[SVA_MAX_PACKAGE_BUF_NUM] = {0};
    DISP_OSD_CORRECT_PRM_S stOsdCorrectPrm;

    /* 能力集相关 */
    CAPB_OSD *pstCapbOsd       = capb_get_osd();
    CAPB_LINE *pstCapbLine     = capb_get_line();
    DRAW_MOD_E enLineMode      = pstCapbLine->enDrawMod;
    DRAW_MOD_E enOsdMode       = pstCapbOsd->enDrawMod;
    UINT32 u32Thick            = pstCapbLine->u32DispLineWidth;
    OSD_FONT_TYPE_E enFontType = pstCapbOsd->enFontType;

    /* OSD颜色相关变量定义 */
    BOOL bBgEnable = SAL_FALSE;
    UINT32 u32OsdColor24   = 0;
    UINT32 u32OsdColor1555 = 0;
    UINT32 u32BgColor24    = 0;
    UINT32 u32BgColor1555  = 0;

    /* 索引值 */
    UINT32 u32LineNum   = 0;        // 线数

    /* 使用CPU画框和线数据结构 */
    VGS_DRAW_AI_PRM *pstCpuLinePrm     = NULL;
    VGS_DRAW_LINE_PRM *pstCpuLineArray = NULL;
    VGS_RECT_ARRAY_S *pstCpuRectArray  = NULL;
    VGS_DRAW_RECT_S *pstCpuRect        = NULL;
    RECT_EXCEED_TYPE_E enExceedType    = pstCapbLine->enRectType;

    /* 使用VGS画OSD数据结构 */
    VGS_ADD_OSD_PRM *pstVgsOsdPrm = NULL;
    VGS_OSD_PRM *pstOsd           = NULL;
    VGS_OSD_PRM *pstNumOsd        = NULL;
    OSD_REGION_S *pstOsdLatRgn    = NULL;
    OSD_REGION_S *pstOsdFontRgn   = NULL;
    OSD_REGION_S *pstNumLatRgn    = NULL;
    OSD_REGION_S *pstNumFontRgn   = NULL;
    OSD_SET_PRM_S *pstOsdPrm      = NULL;
    UINT32 u32NameLen = 0;
    UINT32 u32NumLen  = 0;

    /* DSP核画框和OSD使用数据结构 */
    SVP_DSP_TASK_S stDspLineOsdTask;
    SVP_DSP_LINE_OSD_PARAM_S *pstLineOsdParam = NULL;
    SVP_DSP_LINE_ARRAY_S *pstLineArray        = NULL;
    SVP_DSP_RECT_ARRAY_S *pstRectArray        = NULL;
    SVP_DSP_DRAW_RECT_PRM_S *pstRect          = NULL;
    SVP_DSP_OSD_ARRAY_S *pstOsdArray          = NULL;
    SVP_DSP_OSD_S *pstOsdParam                = NULL;
    SAL_VideoFrameBuf salVideoFrame = {0};

    UINT64 u64StartTime = 0;
    UINT64 u64EndTime   = 0;
    UINT64 u64GapTime   = 0;

    UINT32 uiMainViewChn = 0;

    if (NULL == prm)
    {
        DISP_LOGE("pDispChn Prm Is Null !!!\n");
        return SAL_FAIL;
    }

    if (NULL == pstSystemFrame)
    {
        DISP_LOGE("Prm Is Null !!!\n");
        return SAL_FAIL;
    }

    uiMainViewChn= Sva_DrvGetSyncMainChan();
    if(uiMainViewChn >= DISP_SVA_DEV_MAX)
    {
        DISP_LOGE("Get main view chn failed %u\n", uiMainViewChn);
    }

    memset(&salVideoFrame, 0, sizeof(SAL_VideoFrameBuf));
    s32Ret = sys_hal_getVideoFrameInfo( pstSystemFrame, &salVideoFrame);
    if(SAL_SOK != s32Ret)
    {
        DISP_LOGW("sys_hal_getVideoFrameInfo failed !\n");
    }

    pDispChn = (DISP_CHN_COMMON *)prm;

    VoLayer = pDispChn->uiLayerNo; /* 视频层号 */
    VoChn = pDispChn->uiChn;    /* 显示通道号 */
    voDev = pDispChn->uiDevNo;  /* 显示设备 */
    viChn = pDispChn->stInModule.uiChn;

    uiPicW = salVideoFrame.frameParam.width;
    uiPicH = salVideoFrame.frameParam.height;

    articlelinetype = &pDispChn->articlelinetype;
    if (articlelinetype == NULL)
    {
        DISP_LOGE("chn %d null err\n", pDispChn->stInModule.uiChn);
        return SAL_FAIL;
    }

    pSvaRect = g_svaRect[voDev][VoChn]; /* 智能信息浮点型转整形存储结构体 */
        /* 分析仪 */
    s32Ret = disp_osdPstOutFloattoINT(pDispChn, &salVideoFrame, pstOut);

    if (s32Ret != SAL_SOK)
    {
        DISP_LOGE("chn %d direction %d\n", viChn, direction);
        return SAL_FAIL;
    }
    
    /* 外部配置本地化 */
    uiConfidence = pstIn->stTargetPrm.enOsdExtType;
    scaleLevel = pstIn->stTargetPrm.scaleLevel;
    drawType   = pstIn->drawType;

    if (SVA_OSD_NORMAL_TYPE != drawType)
    {
        s32Ret = disp_osdPstOutSort(pstIn, pstOut, pSvaRect, uiPicW);
        if (s32Ret != SAL_SOK)
        {
            DISP_LOGE("chn %d pstOutSort failed\n", viChn);
            return SAL_FAIL;
        }
    }

    /* 十字叠框类型需要对重复目标进行过滤 */
    if (SVA_OSD_TYPE_CROSS_NO_RECT_TYPE == drawType
        || SVA_OSD_TYPE_CROSS_RECT_TYPE == drawType)
    {
        (VOID)disp_osdGetRealOut(viChn, pstIn, pstOut, pstPkgCntInfo);
        DISP_LOGD("222:alert_num %d pstOut->target_num[%d] \n",alert_num, pstOut->target_num);
    }

    (VOID)disp_osdInitCocrtInfo(pDispChn, uiPicW, uiPicH, pstIn, pstOut, &osd_correct[0]);


    alert_num = pstOut->target_num;
    if (alert_num >= SVA_XSI_MAX_ALARM_NUM)
    {
        DISP_LOGD("chn %d target_num %d\n", VoChn, pstOut->target_num);
        alert_num = SVA_XSI_MAX_ALARM_NUM;
    }

    /********************************************************************
                    画线
    ********************************************************************/
    if (((alert_num > 0) && (!pDispChn->enlargeprm.uiDisappear))
        || (alert_num == 0 && pstOut->packbagAlert.OriginalPackageNum !=0 
            &&(SVA_OSD_TYPE_CROSS_NO_RECT_TYPE == drawType || SVA_OSD_TYPE_CROSS_RECT_TYPE == drawType)))
    {
        /* 初始化CPU画框参数 */
        if (DRAW_MOD_CPU == enLineMode)
        {
            pstCpuLinePrm = gpastDispLinePrm[VoLayer][VoChn];
            if (NULL == pstCpuLinePrm)
            {
                DISP_LOGE("please alloc memory first\n");
                return SAL_FAIL;
            }

            pstCpuLineArray = &pstCpuLinePrm->VgsDrawVLine;
            pstCpuLineArray->uiLineNum    = 0;
            
            pstCpuRectArray = &pstCpuLinePrm->VgsDrawRect;
            pstCpuRectArray->u32RectNum   = 0;
            pstCpuRectArray->enExceedType = enExceedType;
        }

#if 0	/* 增加画包裹前沿的调试功能 */
		{
			(VOID)Disp_halDebugDrawPkg(pstOut, gpastDispLinePrm[VoLayer][VoChn], uiPicW, uiPicH, articlelinetype);
		}
#endif

        /* 初始化用VGS画OSD */
        if (DRAW_MOD_VGS == enOsdMode)
        {
            pstNumFontRgn = osd_func_getVarFontRegionSet(gau32VarOsdChn[VoLayer][VoChn], alert_num);
            pstVgsOsdPrm = gpastDispOsdPrm[VoLayer][VoChn];
            if ((NULL == pstVgsOsdPrm) || (NULL == pstNumFontRgn))
            {
                DISP_LOGE("please alloc memory first\n");
                return SAL_FAIL;
            }
        
            pstVgsOsdPrm->uiOsdNum    = 0;
        }

        /* 初始化DSP画框，OSD参数 */
        if ((DRAW_MOD_DSP == enLineMode) || (DRAW_MOD_DSP == enOsdMode))
        {
            (VOID)svpdsp_func_getBuff(SVP_DSP_CMD_LINE_OSD_USER, &stDspLineOsdTask, SVP_DSP_BUFF_MODE_ONCE);
            pstLineOsdParam = (SVP_DSP_LINE_OSD_PARAM_S *)stDspLineOsdTask.pstTaskBuff->pvVirAddr;
            
            pstLineArray = &pstLineOsdParam->stLineArray;
            pstLineArray->u32LineNum = 0;

            pstRectArray = &pstLineOsdParam->stRectArray;
            pstRectArray->u32RectNum  = 0;
            pstRectArray->u32RectType = (UINT32)enExceedType;

            pstOsdArray = &pstLineOsdParam->stOsdArray;
            pstOsdArray->u32OsdNum = 0;

            (VOID)svpdsp_func_getVideoFrame(&salVideoFrame , &pstLineOsdParam->stVideoFrame);
        }
        
        //osd_correct->uiUpOsdFlag = 1;
        //osd_correct->uiUpStartY  = 2;        // 最上面空余两行
        //osd_correct->uiTmpOsdY   = 2;
        //osd_correct->uiAiOffsetH = pDispChn->enlargeprm.uioffsetH;
        //osd_correct->uiAiCropH = pDispChn->enlargeprm.uiCropH;

        (VOID)osd_func_readStart();

        for (j = 0; j < alert_num; j++)
        {
            if (j >= DISP_OSD_MAX_OBJ_NUM)
            {
                DISP_LOGE("chn %d alert_num %d\n", viChn, alert_num);
                break;
            }

            indx        = pstOut->target[j].type;               /* 目标类型 */
            u32PkgId    = pstOut->target[j].uiPkgId;            /* 对应的包裹ID */
            reliability = pstOut->target[j].visual_confidence;  /* osd 置信度 */
            uiSubTypeIdx = pstOut->target[j].u32SubTypeIdx;     /* 小类种类 */
			u32MergeType = pstOut->target[j].merge_type;        /* 大类合并的标识类别，仅包裹模式使用 */
				
            if (NULL == (pstOsdPrm = osd_func_getOsdPrm(OSD_BLOCK_IDX_STRING, indx)))
            {
                DISP_LOGW("get osd fail param, idx[%u]\n", indx);
                continue;
            }
            uiColor = pstOsdPrm->u32Color;
            u32OsdColor24 = uiColor;
            SAL_RGB24ToARGB1555(uiColor, &u32OsdColor1555, 1);
            
            if (indx == ISM_TIP_TYPE)
            {
                scaleLevel      = pDispChn->orgPrm.tipScaleLevel;
                uiConfidence    = SAL_FALSE;
                drawType        = pDispChn->orgPrm.tipDrawType;
                pstLineType     = &articlelinetype->tipline;
                u32OsdColor24   = OSD_COLOR24_RED;
                u32OsdColor1555 = OSD_COLOR_RED;
                fGAlpha = 255;
                BgAlpha = 255;
                numfGAlpha = 0;
                numBgAlpha = 0;
            }
            else if (indx == ISM_ORGNAIC_TYPE)
            {
                uiConfidence   = SAL_FALSE;
                scaleLevel     = pDispChn->orgPrm.orgScaleLevel;
                drawType       = pDispChn->orgPrm.orgDrawType;
                pstLineType    = &articlelinetype->orgnaicline;
                u32BgColor24   = OSD_COLOR24_BACK;
                u32BgColor1555 = OSD_BACK_COLOR;
                fGAlpha = 0;
                BgAlpha = 255;
                numfGAlpha = 0;
                numBgAlpha = 0;
            }
            else
            {
                uiConfidence   = pstIn->stTargetPrm.enOsdExtType;
                scaleLevel     = pstIn->stTargetPrm.scaleLevel;
                drawType       = pstIn->drawType;
                pstLineType    = &articlelinetype->ailine;
                u32BgColor24   = OSD_COLOR24_BACK;
                u32BgColor1555 = OSD_BACK_COLOR;
                if (SVA_OSD_LINE_RECT_TYPE == drawType || SVA_OSD_LINE_POINT_TYPE == drawType || SVA_OSD_TYPE_NO_LINE_TYPE == drawType)
                {
                    fGAlpha = 255;
                    BgAlpha = 0;
                    numfGAlpha = 255;
                    numBgAlpha = 0;
                }
                else
                {
                    fGAlpha = 255;
                    BgAlpha = 0;
                    numfGAlpha = 255;
                    numBgAlpha = 0;
                }
                
                /* osd缩放等级设置 */
                if ((scaleLevel > 1) && (pDispChn->stChnPrm.uiRatio < 1.0))
                {
                    if (pDispChn->stChnPrm.uiRatio < 0.15)
                    {
                        scaleLevel = 1;
                    }
                    else if (pDispChn->stChnPrm.uiRatio < 0.35)
                    {
                        scaleLevel = scaleLevel - 1;
                    }
                }

                if (0 == scaleLevel)
                {
                    scaleLevel = 1;
                }
            }
            
            if (scaleLevel == 0)
            {
                scaleLevel = 1;
            }

            /* truetype点阵大小和字体大小保持一致，hzk16点阵大小固定为16 */
            if (OSD_FONT_TRUETYPE == enFontType)
            {
                u32LatSize = pstCapbOsd->TrueTypeSize[SAL_MIN(scaleLevel, SAL_arraySize(pstCapbOsd->TrueTypeSize)) - 1];
                u32FontSize = u32LatSize;
            }
            else
            {
                u32LatSize  = 16;
                u32FontSize = scaleLevel * 16;
            }

            if (SVA_OSD_LINE_RECT_TYPE == drawType || SVA_OSD_LINE_POINT_TYPE == drawType || SVA_OSD_TYPE_NO_LINE_TYPE == drawType)
            {
                bBgEnable = SAL_TRUE;

                u32BgColor24  = u32OsdColor24;
                u32OsdColor24 = OSD_COLOR24_WHITE;

                u32BgColor1555  = u32OsdColor1555;
                u32OsdColor1555 = OSD_COLOR_WHITE;
            }
            else
            {
            	u32BgColor1555 = OSD_BACK_COLOR;
                bBgEnable = SAL_FALSE;
            }

            /* 矫正数据防止数据异常 */
            reliability = (reliability > 99) ? (99) : (reliability);
                        
            if (pSvaRect[j].width && pSvaRect[j].height)
            {
                /* 数据校验防止画出图 */
                if ( pSvaRect[j].x >= uiPicW || pSvaRect[j].y >= uiPicH)
                {
                    continue;
                }

                /* 违禁品名称OSD的点阵 */
                if (NULL == (pstOsdLatRgn = osd_func_getLatRegion(OSD_BLOCK_IDX_STRING, indx, u32LatSize)))
                {
                    DISP_LOGW("get alert name osd lattice fail, idx[%u], lat[%u]\n", indx, u32LatSize);
                    continue;
                }

                /* 违禁品名称的OSD */
                if (DRAW_MOD_VGS == enOsdMode)
                {
                    if (NULL == (pstOsdFontRgn = osd_func_getFontRegion(OSD_BLOCK_IDX_STRING, indx, u32LatSize, u32FontSize, bBgEnable)))
                    {
                        DISP_LOGW("get alert name osd fail, idx[%u], lat[%u] font[%u]\n", indx, u32LatSize, u32FontSize);
                        continue;
                    }

                    u32NameLen = pstOsdFontRgn->u32Width;
                }
                else
                {
                    u32NameLen = pstOsdLatRgn->u32Width * ((OSD_FONT_TRUETYPE == enFontType) ? 1 : scaleLevel);
                }

                u32NumLen = 0;
                if (((SVA_OSD_TYPE_CROSS_NO_RECT_TYPE != drawType)
                      && (SVA_OSD_TYPE_CROSS_RECT_TYPE != drawType))
                      && (uiConfidence == SAL_TRUE))
                {
                    if(SVA_NO_SUB_TYPE_IDX == uiSubTypeIdx)
                    {
                        pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_NUM_PAREN, reliability, uiSubTypeIdx, u32LatSize); 
                    }
                    else
                    {
                        pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_DOUBLE_NUM_PAREN_DOT, reliability, uiSubTypeIdx, u32LatSize);
                    }

                    if (NULL == pstNumLatRgn)
                    {
                        DISP_LOGW("get num osd fail, num[%u], lat[%u]\n", reliability, u32LatSize);
                        continue;
                    }

                    if (DRAW_MOD_VGS == enOsdMode)
                    {
                        if (SAL_SOK != osd_func_FillFont(pstNumLatRgn, u32LatSize, pstNumFontRgn, u32FontSize, u32OsdColor1555, u32BgColor1555))
                        {
                            DISP_LOGW("fill osd font fail, lat[%u] font[%u]\n", reliability, u32LatSize);
                            continue;
                        }

                        u32NumLen = pstNumFontRgn->u32Width;
                    }
                    else
                    {
                        u32NumLen = pstNumLatRgn->u32Width * ((OSD_FONT_TRUETYPE == enFontType) ? 1 : scaleLevel);
                    }
                }

                /* 获取复用无括号数字block的个数osd字块 */
                if ((SVA_OSD_TYPE_CROSS_NO_RECT_TYPE == drawType) || (SVA_OSD_TYPE_CROSS_RECT_TYPE == drawType))
                {
                    if (SVA_PROC_MODE_DUAL_CORRELATION == Sva_DrvGetAlgMode())
                    {
                        pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_DOUBLE_NUM_STRING_PAREN_MUL, pstPkgCntInfo->astViewTarCnt[uiMainViewChn].uiTarCnt[u32PkgId][u32MergeType],
                                        pstPkgCntInfo->astViewTarCnt[1 - uiMainViewChn].uiTarCnt[u32PkgId][u32MergeType], u32LatSize);
                    }
                    else
                    {
                        pstNumLatRgn = osd_func_getNumLatRegion(OSD_BLOCK_IDX_NUM_MUL, 
                                        pstPkgCntInfo->astViewTarCnt[uiMainViewChn].uiTarCnt[u32PkgId][u32MergeType] + pstPkgCntInfo->astViewTarCnt[1 - uiMainViewChn].uiTarCnt[u32PkgId][u32MergeType],
                                        0, u32LatSize);
                    }
                    if (NULL == pstNumLatRgn)
                    {
                        DISP_LOGW("get num osd fail, num[%u], lat[%u]\n", reliability, u32LatSize);
                        continue;
                    }

                    if (DRAW_MOD_VGS == enOsdMode)
                    {
                        if (SAL_SOK != osd_func_FillFont(pstNumLatRgn, u32LatSize, pstNumFontRgn, u32FontSize, u32OsdColor1555, u32BgColor1555))
                        {
                            DISP_LOGW("fill osd font fail, lat[%u] font[%u]\n", reliability, u32LatSize);
                            continue;
                        }

                        u32NumLen = pstNumFontRgn->u32Width;
                    }
                    else
                    {
                        u32NumLen = pstNumLatRgn->u32Width * ((OSD_FONT_TRUETYPE == enFontType) ? 1 : scaleLevel);
                    }
                }

                /* 根据画框和OSD方式重新计算框和OSD的坐标 */
                stOsdCorrectPrm.u32Width     = u32NameLen + u32NumLen;
                stOsdCorrectPrm.u32Height    = pstOsdLatRgn->u32Height * ((OSD_FONT_TRUETYPE == enFontType) ? 1 : scaleLevel);
                stOsdCorrectPrm.enBorderType = drawType;
                
                pstOsd_corct = ((SVA_OSD_TYPE_CROSS_NO_RECT_TYPE == drawType) || (SVA_OSD_TYPE_CROSS_RECT_TYPE == drawType)) \
                               ? &osd_correct[u32PkgId] \
                               : &osd_correct[0];
                pstOsd_corct->uiPicW = uiPicW;
                pstOsd_corct->uiPicH = uiPicH;
                pstOsd_corct->uiSourceW = uiPicW;
                pstOsd_corct->uiSourceH = uiPicH;
                pstOsd_corct->uiUpTarNum = 10;
                pstOsd_corct->bOsdPstOutFlag = SAL_FALSE;
                s32Ret = disp_osdPstOutcorrect(pstOsd_corct, &stOsdCorrectPrm, \
                                                pSvaRect + j, pstOut, &pstOut->target[j], pstPkgCntInfo);
                
                if (s32Ret != SAL_SOK)
                {
                    DISP_LOGE("chn %d j %d PstOutcorrect fail\n", viChn, j);
                    continue;
                }
                
                if ((stOsdCorrectPrm.u32X >= uiPicW) || (stOsdCorrectPrm.u32Y >= uiPicH))
                {
                    DISP_LOGD("invalid correct prm x*y[%d, %d], Pic W*h [%d, %d] \n", 
                               stOsdCorrectPrm.u32X, stOsdCorrectPrm.u32Y, uiPicW, uiPicH);
                    continue;
                }

                /* 框和线使用同一模块画 */
                if (DRAW_MOD_CPU == enLineMode)
                {
                    if ((SVA_OSD_NORMAL_TYPE == drawType || SVA_OSD_LINE_RECT_TYPE == drawType
                        || SVA_OSD_LINE_POINT_TYPE == drawType || SVA_OSD_TYPE_NO_LINE_TYPE == drawType
                        || ((SVA_OSD_TYPE_CROSS_RECT_TYPE == drawType) && pstOut->target[j].bRectShow))
                        && (SAL_TRUE != pstOut->target[j].bAnotherViewTar))
                    {
                        /* 矩形 */
                        pstCpuRect = pstCpuRectArray->astRect + pstCpuRectArray->u32RectNum++;

                        pstCpuRect->s32X           = pSvaRect[j].x;
                        pstCpuRect->s32Y           = pSvaRect[j].y;
                        pstCpuRect->u32Width       = pSvaRect[j].width;
                        pstCpuRect->u32Height      = pSvaRect[j].height;
                        pstCpuRect->f32StartXFract = pSvaRect[j].f32StartXFract;
                        pstCpuRect->f32EndXFract   = pSvaRect[j].f32EndXFract;
                        pstCpuRect->u32Color       = uiColor;
                        pstCpuRect->u32Thick       = u32Thick;
                        memcpy(&pstCpuRect->stLinePrm, pstLineType, sizeof(*pstLineType));

                        /* 竖线 */
                        if ((SVA_OSD_LINE_RECT_TYPE == drawType || SVA_OSD_LINE_POINT_TYPE == drawType || indx == ISM_TIP_TYPE))
                        {
                            u32LineNum = pstCpuLineArray->uiLineNum;
                            if (pstOsd_corct->uiUpOsdFlag)
                            {
                                if (stOsdCorrectPrm.u32Y + stOsdCorrectPrm.u32Height < pSvaRect[j].y)
                                {
                                    /* OSD在框的上方 */
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].stStartPoint.s32X = pSvaRect[j].x;
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].stStartPoint.s32Y = stOsdCorrectPrm.u32Y + stOsdCorrectPrm.u32Height;
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].stEndPoint.s32X   = pSvaRect[j].x;
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].stEndPoint.s32Y   = pSvaRect[j].y;
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].u32Thick          = u32Thick;
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].u32Color          = uiColor;
                                    pstCpuLineArray->af32XFract[u32LineNum] = pSvaRect[j].f32StartXFract;
                                    memcpy(pstCpuLineArray->linetype + u32LineNum, pstLineType, sizeof(*pstLineType));

                                    pstCpuLineArray->uiLineNum++;
                                }
                            }
                            else
                            {
                                if (stOsdCorrectPrm.u32Y > pSvaRect[j].y + pSvaRect[j].height)
                                {
                                    /* OSD在框的下方 */
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].stStartPoint.s32X = pSvaRect[j].x;
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].stStartPoint.s32Y = pSvaRect[j].y + pSvaRect[j].height;
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].stEndPoint.s32X   = pSvaRect[j].x;
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].stEndPoint.s32Y   = stOsdCorrectPrm.u32Y;
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].u32Thick          = u32Thick;
                                    pstCpuLineArray->stVgsLinePrm[u32LineNum].u32Color          = uiColor;
                                    pstCpuLineArray->af32XFract[u32LineNum] = pSvaRect[j].f32StartXFract;
                                    memcpy(pstCpuLineArray->linetype + u32LineNum, pstLineType, sizeof(*pstLineType));

                                    pstCpuLineArray->uiLineNum++;
                                }
                            }
                        }
                    }
                }
                else if (DRAW_MOD_DSP == enLineMode)
                {
                    if ((SVA_OSD_NORMAL_TYPE == drawType
                        || SVA_OSD_LINE_RECT_TYPE == drawType
                        || SVA_OSD_LINE_POINT_TYPE == drawType
                        || SVA_OSD_TYPE_NO_LINE_TYPE == drawType
                        || ((SVA_OSD_TYPE_CROSS_RECT_TYPE == drawType) && pstOut->target[j].bRectShow))
                        && (SAL_TRUE != pstOut->target[j].bAnotherViewTar))
                    {
                        /* 矩形 */
                        pstRect = pstRectArray->astRect + pstRectArray->u32RectNum++;
                        pstRect->stRectPrm.u32X      = pSvaRect[j].x;
                        pstRect->stRectPrm.u32Y      = pSvaRect[j].y;
                        pstRect->stRectPrm.u32Width  = pSvaRect[j].width;
                        pstRect->stRectPrm.u32Height = pSvaRect[j].height;
                        pstRect->u32Color       = uiColor;
                        pstRect->u32Thick       = u32Thick;
                        pstRect->f32StartXFract = pSvaRect[j].f32StartXFract;
                        pstRect->f32EndXFract   = pSvaRect[j].f32EndXFract;
                        (VOID)svpdsp_func_getLineType(pstLineType, &pstRect->stLineType);

                        /* 竖线 */
                        if ((SVA_OSD_LINE_RECT_TYPE == drawType || SVA_OSD_LINE_POINT_TYPE == drawType || indx == ISM_TIP_TYPE))
                        {
                            u32LineNum = pstLineArray->u32LineNum;
                            if (pstOsd_corct->uiUpOsdFlag)
                            {
                                if (stOsdCorrectPrm.u32Y + stOsdCorrectPrm.u32Height < pSvaRect[j].y)
                                {
                                    /* OSD在框的上方 */
                                    pstLineArray->astVgsLinePrm[u32LineNum].stStartPoint.s32X = pSvaRect[j].x;
                                    pstLineArray->astVgsLinePrm[u32LineNum].stStartPoint.s32Y = stOsdCorrectPrm.u32Y + stOsdCorrectPrm.u32Height;
                                    pstLineArray->astVgsLinePrm[u32LineNum].stEndPoint.s32X   = pSvaRect[j].x;
                                    pstLineArray->astVgsLinePrm[u32LineNum].stEndPoint.s32Y   = pSvaRect[j].y;
                                    pstLineArray->astVgsLinePrm[u32LineNum].u32Color          = uiColor;
                                    pstLineArray->astVgsLinePrm[u32LineNum].u32Thick          = u32Thick;
                                    pstLineArray->af32XFract[u32LineNum] = pSvaRect[j].f32StartXFract;
                                    (VOID)svpdsp_func_getLineType(pstLineType, pstLineArray->astLineType + u32LineNum);
                                    pstLineArray->u32LineNum++;
                                }
                            }
                            else
                            {
                                if (stOsdCorrectPrm.u32Y > pSvaRect[j].y + pSvaRect[j].height)
                                {
                                    /* OSD在框的下方 */
                                    pstLineArray->astVgsLinePrm[u32LineNum].stStartPoint.s32X = pSvaRect[j].x;
                                    pstLineArray->astVgsLinePrm[u32LineNum].stStartPoint.s32Y = pSvaRect[j].y + pSvaRect[j].height;
                                    pstLineArray->astVgsLinePrm[u32LineNum].stEndPoint.s32X   = pSvaRect[j].x;
                                    pstLineArray->astVgsLinePrm[u32LineNum].stEndPoint.s32Y   = stOsdCorrectPrm.u32Y;
                                    pstLineArray->astVgsLinePrm[u32LineNum].u32Color          = uiColor;
                                    pstLineArray->astVgsLinePrm[u32LineNum].u32Thick          = u32Thick;
                                    pstLineArray->af32XFract[u32LineNum] = pSvaRect[j].f32StartXFract;
                                    (VOID)svpdsp_func_getLineType(pstLineType, pstLineArray->astLineType + u32LineNum);
                                    pstLineArray->u32LineNum++;
                                }
                            }
                        }
                    }
                }

                if (((SVA_OSD_TYPE_CROSS_NO_RECT_TYPE == drawType || SVA_OSD_TYPE_CROSS_RECT_TYPE == drawType)
					   && ((!pstOut->target[j].bOsdShow) || (pstPkgCntInfo->astViewTarCnt[uiMainViewChn].uiTarCnt[u32PkgId][u32MergeType] + pstPkgCntInfo->astViewTarCnt[1 - uiMainViewChn].uiTarCnt[u32PkgId][u32MergeType] <= 0)))
					 || ((SVA_OSD_TYPE_CROSS_NO_RECT_TYPE != drawType && SVA_OSD_TYPE_CROSS_RECT_TYPE != drawType) 
					   && (SAL_TRUE == pstOut->target[j].bAnotherViewTar)))
                {
                    DISP_LOGD("i %d, bOsdShow %d,u32PkgId %d,drawType %d,  uiTarCnt %d \n", 
						       j, pstOut->target[j].bOsdShow,u32PkgId, drawType, pstPkgCntInfo->astViewTarCnt[uiMainViewChn].uiTarCnt[u32PkgId][u32MergeType] + pstPkgCntInfo->astViewTarCnt[1 - uiMainViewChn].uiTarCnt[u32PkgId][u32MergeType]);
                    continue;
                }

                if (DRAW_MOD_DSP == enOsdMode)
                {
                    /* 使用DSP画OSD */
                    pstOsdParam = pstOsdArray->astOsdParam + pstOsdArray->u32OsdNum++;
                    pstOsdParam->stOsdRect.u32X  = stOsdCorrectPrm.u32X;
                    pstOsdParam->stOsdRect.u32Y  = stOsdCorrectPrm.u32Y;
                    pstOsdParam->u32OsdColor = u32OsdColor24;
                    pstOsdParam->u32BgColor  = u32BgColor24;
                    pstOsdParam->u32BgEnable = (SAL_TRUE == bBgEnable) ? 1 : 0;
                    (VOID)disp_svpDspfillLatRegion(pstOsdLatRgn, pstNumLatRgn, pstOsdParam);
                }
                else if (DRAW_MOD_VGS == enOsdMode)
                {
                    /* 汉字OSD */
                    pstOsd = pstVgsOsdPrm->stVgsOsdPrm + pstVgsOsdPrm->uiOsdNum++;
                    pstOsd->u64PhyAddr = pstOsdFontRgn->stAddr.u64PhyAddr;					
					pstOsd->pVirAddr = pstOsdFontRgn->stAddr.pu8VirAddr;				
                    pstOsd->enPixelFmt = SAL_VIDEO_DATFMT_ARGB16_1555;
                    pstOsd->s32X       = stOsdCorrectPrm.u32X;
                    pstOsd->s32Y       = stOsdCorrectPrm.u32Y;
                    pstOsd->u32Width   = pstOsdFontRgn->u32Width;
                    pstOsd->u32Height  = pstOsdFontRgn->u32Height;
                    pstOsd->u32BgColor = u32BgColor1555;
                    pstOsd->u32BgAlpha = BgAlpha;
                    pstOsd->u32FgAlpha = fGAlpha;
                    pstOsd->u32Stride  = pstOsdFontRgn->u32Stride;

					
                    
                    /* 数字OSD */
                    if ((uiConfidence == SAL_TRUE || SVA_OSD_TYPE_CROSS_NO_RECT_TYPE == drawType || SVA_OSD_TYPE_CROSS_RECT_TYPE == drawType)
                        && (NULL != pstNumFontRgn))
                    {
                        pstNumOsd = pstVgsOsdPrm->stVgsOsdPrm + pstVgsOsdPrm->uiOsdNum++;
                        pstNumOsd->u64PhyAddr = pstNumFontRgn->stAddr.u64PhyAddr;					
						pstNumOsd->pVirAddr = pstNumFontRgn->stAddr.pu8VirAddr;
                        pstNumOsd->enPixelFmt = SAL_VIDEO_DATFMT_ARGB16_1555;
                        pstNumOsd->s32X       = pstOsd->s32X + pstOsd->u32Width;
                        pstNumOsd->s32Y       = pstOsd->s32Y;
                        pstNumOsd->u32Width   = pstNumFontRgn->u32Width;
                        pstNumOsd->u32Height  = pstNumFontRgn->u32Height;
                        pstNumOsd->u32BgColor = u32BgColor1555;
                        pstNumOsd->u32BgAlpha = numBgAlpha;
                        pstNumOsd->u32FgAlpha = numfGAlpha;
                        pstNumOsd->u32Stride  = pstNumFontRgn->u32Stride;
                        pstNumFontRgn++;
                    }
                }
            }
        }

        u64StartTime = SAL_getCurUs();

        if (DRAW_MOD_CPU == enLineMode)
        {
            pstLineType    = &articlelinetype->ailine;
            /* 十字框选类型框选, 画出所有包裹的位置 */
            if (SVA_OSD_TYPE_CROSS_NO_RECT_TYPE == drawType 
                || SVA_OSD_TYPE_CROSS_RECT_TYPE == drawType)
            {
                (VOID)disp_osdDbgDrawPkgRect(uiPicW, uiPicH, pstOut, pstCpuLineArray, pstLineType);
            }
            vgs_func_drawRectArraySoft(&salVideoFrame, pstCpuRectArray, SAL_TRUE, SAL_FALSE);
            vgs_func_drawLineSoft(&salVideoFrame, pstCpuLineArray, SAL_TRUE, SAL_FALSE);
            mem_hal_mmzFlushCache(salVideoFrame.vbBlk);
        }

        if (DRAW_MOD_VGS == enOsdMode)
        {
            vgs_hal_drawLineOSDArray(pstSystemFrame, pstVgsOsdPrm, NULL);
        }

        if ((DRAW_MOD_DSP == enOsdMode) || (DRAW_MOD_DSP == enLineMode))
        {
            if (SAL_SOK != (s32DspRet = svpdsp_func_runTask(&stDspLineOsdTask)))
            {
                DISP_LOGE("Dsp run draw line and osd task fail, volayer:%u, chn:%u, viChn:%u\n", VoLayer, VoChn, viChn);
            }

            if (SAL_SOK == s32DspRet)
            {
                svpdsp_func_waitTaskFinish(&stDspLineOsdTask);
            }
        }
        
        u64EndTime = SAL_getCurUs();
        u64GapTime = u64EndTime - u64StartTime;
        if ((SAL_TRUE == gbOsdTimeEnable) && (u64GapTime > (UINT64)gu32OsdTime * 1000))
        {
            DISP_LOGE("dsp run cost %llu us\n", u64GapTime);
        }

        (VOID)osd_func_readEnd();


    }

    return SAL_SOK;
}


/******************************************************************
   Function:   Disp_HalDrawOsdUseVgsInit
   Description:初始化智能画线或者画框
   Input:
   Output:
   Return:   OK or ERR Information
 *******************************************************************/
INT32 disp_osdDrawInit(UINT32 u32Dev, UINT32 u32Chn, DRAW_MOD_E enLineMode, DRAW_MOD_E enOsdMode)
{
    INT32 s32Ret = SAL_SOK;
    OSD_VAR_BLOCK_S stBlock;

    if (DRAW_MOD_VGS == enOsdMode)
    {
        stBlock.u32StringLenMax = strlen("(  %)");
        stBlock.u32LatSizeMax   = 98;   // 按照大号字体来申请内存
        stBlock.u32FontSizeMax  = 98;
        stBlock.u32LatNum       = 0;
        stBlock.u32FontNum      = 128;
    
        s32Ret = osd_func_getFreeVarBlock(&stBlock, &gau32VarOsdChn[u32Dev][u32Chn]);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("disp get free osd block fail\n");
            return SAL_FAIL;
        }

        gpastDispOsdPrm[u32Dev][u32Chn] = SAL_memMalloc(sizeof(VGS_ADD_OSD_PRM), "disp_tsk_inter", "disp_osdDraw");
        if (NULL == gpastDispOsdPrm[u32Dev][u32Chn])
        {
            SAL_ERROR("disp malloc fail\n");
            return SAL_FAIL;
        }
    }

    if (DRAW_MOD_CPU == enLineMode)
    {
        s32Ret = vgs_func_drawLineSoftInit(&gpastDispLinePrm[u32Dev][u32Chn]);
        if (SAL_SOK != s32Ret)
        {
            DISP_LOGE("disp dev[%u] chn[%u] init line fail\n", u32Dev, u32Chn);
            if (NULL != gpastDispOsdPrm[u32Dev][u32Chn])
            {
                SAL_memfree(gpastDispOsdPrm[u32Dev][u32Chn], "disp_tsk_inter", "disp_osdDraw");
            }
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : disp_osdTest
* 描  述  : 
* 输  入  : 
* 输  出  : 
* 返回值  : SAL_SOK : 成功
*         SAL_FAIL : 失败
*******************************************************************************/
INT32 disp_osdTest(const VOID *pvArg1, const VOID *pvArg2, const VOID *pvArg3, const VOID *pvArg4)
{
    SAL_UNUSED(pvArg1);
    SAL_UNUSED(pvArg2);
    SAL_UNUSED(pvArg3);
    SAL_UNUSED(pvArg4);

    gbOsdTimeEnable = (0 == *((INT32 *)pvArg1)) ? SAL_FALSE : SAL_TRUE;
    gu32OsdTime = (UINT32)(*((INT32 *)pvArg2));
    DISP_LOGI("disp set osd test enable[%u] time[%u]\n", gbOsdTimeEnable, gu32OsdTime);

    return SAL_SOK;
}

