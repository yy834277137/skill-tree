/**
 * @file   xsp_native.c
 * @brief  封装自研XSP成像处理接口
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ========================================================================== */
#include "xsp_native.h"
#include "libXRay.h"
#include "libXRay_def.h"
#include "xray_preproc.h"
#include "system_prm_api.h"

#line __LINE__ "xsp_native.c"

/* ========================================================================== */
/*                           Macros & Typedefs                                */
/* ========================================================================== */
#define RIN_NATIVE_PIPELINE_OBJ_NUM_MAX             4   // 流水线处理时最多缓存的待处理条带数


/* ========================================================================== */
/*                         Structure Declarations                             */
/* ========================================================================== */
typedef struct
{
    BOOL b_sus_enable; /* 可疑物开关 */
} NATIVE_PRO_ATTR;

/* 实时预览流水线处理对象属性 */
typedef struct
{
    INT32 s32CurPipeSeg;            /* 当前处于的流水段，默认值为-1，插入队列后初始值为0，每处理完一段流水该值+1 */
    VOID *pPipelineParam;           /* 流水线参数，指向算法结构体XRAYLIB_PIPELINE_PARAM_MODEXX，不同流水线模式参数不同 */
    XSP_DEBUG_ALG stDebug;          /* 从task层下发的调试参数 */
    VOID *pPassthData;              /* 从task层下发的透传参数，异步处理完后原样返回给task层 */
    BOOL bPseudo;                   /* 是否伪空白，TRUE-伪空白 */
    XSP_XRAW_TIPED st_xraw_tiped;   /* 带有tip图像的归一化xraw数据 */
} PIPELINE_OBJECT_ATTR;

/* 各通道实时预览流水线参数，每个算法句柄Handle对应一个 */
typedef struct
{
    XRAYLIB_PIPELINE_MODE enPipelineMode; /* 流水线模式 */
    VOID *pBgacHandle; /* 背景自动校正算法句柄，XRay预处理 */
    DSA_LIST *lstPipeline; /* 流水线处理队列，队列中每个节点称为对象，最多处理对象个数为RIN_NATIVE_PIPELINE_OBJ_NUM_MAX */
    PIPELINE_OBJECT_ATTR stPipelineObj[RIN_NATIVE_PIPELINE_OBJ_NUM_MAX]; /* 流水线处理对象的属性 */
    UINT32 u32PassthDataSize; /* 流水线处理透传数据长度，单位：字节 */
    void (*rtproc_pipeline_cb)(void *p_handle, XSP_RTPROC_OUT *pstRtProcOut); /* 流水线处理回调函数 */
} RIN_NATIVE_RT_PIPELINE;

/* 流水线处理级数 */
typedef enum
{
    PIPELINE_STAGE_0 = 0,           /* 算法第一级处理流水 */
    PIPELINE_STAGE_1 = 1,           /* 算法第二级处理流水 */
    PIPELINE_STAGE_2 = 2,           /* 算法第三级处理流水 */
} PIPELINE_STAGE_NUM;

/* ========================================================================== */
/*                              Global Variables                              */
/* ========================================================================== */
/* 算法全局参数*/
static NATIVE_PRO_ATTR g_native_pro_attr = {0};
static XSP_HANDLE_ATTR g_handle_attr[XSP_HANDLE_NUM_MAX] = {0}; /* 数组方式有溢出风险，后续改为链表方式可动态添加 */
static RIN_NATIVE_RT_PIPELINE g_rt_pipeline[XSP_HANDLE_NUM_MAX] = {0};


/* ========================================================================== */
/*                        Function Declarations                               */
/* ========================================================================== */

static SAL_STATUS rin_native_2xraylibimg(XIMAGE_DATA_ST *pstSrcXimg, XRAYLIB_IMAGE *pstDstXrayImg, const CHAR *_func_, const UINT32 _line_)
{
    INT32 i = 0;
    CAPB_XRAY_IN *pstCapbXrayIn = capb_get_xrayin();

    switch (pstSrcXimg->enImgFmt)
    {
    case DSP_IMG_DATFMT_YUV420SP_UV:
    case DSP_IMG_DATFMT_YUV420SP_VU:
        pstDstXrayImg->format = XRAYLIB_IMG_YUV_NV21;
        break;
    case DSP_IMG_DATFMT_BGR24:
        pstDstXrayImg->format = XRAYLIB_IMG_RGB_ARGB_C3;
        break;
    case DSP_IMG_DATFMT_BGRA32:
        pstDstXrayImg->format = XRAYLIB_IMG_RGB_ARGB_C4;
        break;
    case DSP_IMG_DATFMT_BGRP:
        pstDstXrayImg->format = XRAYLIB_IMG_RGB_ARGB_P3;
        break;
    case DSP_IMG_DATFMT_SP16:
        pstDstXrayImg->format = (XRAY_ENERGY_DUAL == pstCapbXrayIn->xray_energy_num) ? XRAYLIB_IMG_RAW_MERGE : XRAYLIB_IMG_RAW_L;
        break;
    case DSP_IMG_DATFMT_LHP:
        pstDstXrayImg->format = (XRAY_ENERGY_DUAL == pstCapbXrayIn->xray_energy_num) ? XRAYLIB_IMG_RAW_LH : XRAYLIB_IMG_RAW_L;
        break;
    case DSP_IMG_DATFMT_LHZP:
        pstDstXrayImg->format = (XRAY_ENERGY_DUAL == pstCapbXrayIn->xray_energy_num) ? XRAYLIB_IMG_RAW_LHZ8 : XRAYLIB_IMG_RAW_L;
        break;
    case DSP_IMG_DATFMT_LHZ16P:
        pstDstXrayImg->format = (XRAY_ENERGY_DUAL == pstCapbXrayIn->xray_energy_num) ? XRAYLIB_IMG_RAW_LHZ16 : XRAYLIB_IMG_RAW_L;
        break;
    default:
        SAL_LOGE("Not supported image format[0x%x], Func:%s|%u\n", pstSrcXimg->enImgFmt, _func_, _line_);
        return SAL_FAIL;
    }

    pstDstXrayImg->width = pstSrcXimg->u32Width;
    pstDstXrayImg->height = pstSrcXimg->u32Height;
    for (i = 0; i < XIMAGE_PLANE_MAX; i++)
    {
        pstDstXrayImg->stride[i] = pstSrcXimg->u32Stride[i];
        pstDstXrayImg->pData[i] = pstSrcXimg->pPlaneVir[i];
    }

    return SAL_SOK;
}


static SAL_STATUS rin_native_2ximg(XRAYLIB_IMAGE *pstSrcXrayImg, XIMAGE_DATA_ST *pstDstXimg, const CHAR *_func_, const UINT32 _line_)
{
    INT32 i = 0;

    switch (pstSrcXrayImg->format)
    {
    case XRAYLIB_IMG_RAW_L:
        pstDstXimg->enImgFmt = DSP_IMG_DATFMT_SP16;
        break;
    case XRAYLIB_IMG_RAW_LH:
        pstDstXimg->enImgFmt = DSP_IMG_DATFMT_LHP;
        break;
    case XRAYLIB_IMG_RAW_LHZ8:
        pstDstXimg->enImgFmt = DSP_IMG_DATFMT_LHZP;
        break;
    case XRAYLIB_IMG_RAW_LHZ16:
        pstDstXimg->enImgFmt = DSP_IMG_DATFMT_LHZ16P;
        break;
    default:
        SAL_LOGE("Not supported image format[0x%x], Func:%s|%u\n", pstSrcXrayImg->format, _func_, _line_);
        return SAL_FAIL;
    }

    pstDstXimg->u32Width = pstSrcXrayImg->width;
    pstDstXimg->u32Height = pstSrcXrayImg->height;
    for (i = 0; i < XIMAGE_PLANE_MAX; i++)
    {
        pstDstXimg->u32Stride[i] = pstSrcXrayImg->stride[i];
        pstDstXimg->pPlaneVir[i] = pstSrcXrayImg->pData[i];
    }

    return SAL_SOK;
}


/**
 * @fn      get_node_by_pipeseg
 * @brief   在处理队列中查询流水线处理段为s32PipelineSeg的节点
 * 
 * @param   [IN] lst 流水线处理队列
 * @param   [IN] s32PipelineSeg 流水线处理段，第N段流水的值为N
 * 
 * @return  DSA_NODE* 处理队列中的节点
 */
static DSA_NODE *get_node_by_pipeseg(DSA_LIST *lst, const INT32 s32PipelineSeg)
{
    DSA_NODE *pNode = NULL;

    if (DSA_LstGetCount(lst) > 0)
    {
        pNode = DSA_LstGetHead(lst);
        while (NULL != pNode)
        {
            if (s32PipelineSeg == ((PIPELINE_OBJECT_ATTR *)pNode->pAdData)->s32CurPipeSeg)
            {
                return pNode;
            }
            else
            {
                pNode = pNode->next;
            }
        }
    }

    return NULL;
}


/**
 * @fn      rin_native_rt_pipeline_out
 * @brief   根据不同模式，封装实时预览流水线处理输出，并回调给task层
 * 
 * @param   [IN] u32HandleIdx 算法通道号（句柄Handle索引）
 * @param   [IN] pstPipelineObj 流水线处理对象属性，enResult为SAL_SOK时有效
 * @param   [IN] enResult 处理结果，SAL_SOK：成功，其他：失败
 */
static void rin_native_rt_pipeline_out(UINT32 u32HandleIdx, PIPELINE_OBJECT_ATTR *pstPipelineObj, SAL_STATUS enResult)
{
    UINT32 i = 0;
    XSP_RTPROC_OUT stRtProcOut = {0};
    XRAYLIB_PIPELINE_PARAM_MODE3A *pPipelineParam3a = NULL;
    XRAYLIB_PIPELINE_PARAM_MODE2A *pPipelineParam2a = NULL;
    CAPB_XSP *pstCapbXsp = capb_get_xsp();
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();

    stRtProcOut.enResult = enResult;
    stRtProcOut.pPassthData = pstPipelineObj->pPassthData;

    if (SAL_SOK == enResult)
    {
        switch (g_rt_pipeline[u32HandleIdx].enPipelineMode)
        {
        case XRAYLIB_PIPELINE_MODE2A:
            pPipelineParam2a = (XRAYLIB_PIPELINE_PARAM_MODE2A *)pstPipelineObj->pPipelineParam;

            stRtProcOut.stDispOut.enImgFmt = pstCapbXsp->enDispFmt;
            stRtProcOut.stDispOut.u32Width = pPipelineParam2a->idx1.out_disp.width;
            stRtProcOut.stDispOut.u32Height = pPipelineParam2a->idx1.out_disp.height;
            stRtProcOut.stDispOut.u32Stride[0] = pPipelineParam2a->idx1.out_disp.stride[0];
            stRtProcOut.stDispOut.pPlaneVir[0] = pPipelineParam2a->idx1.out_disp.pData[0];
            if (DSP_IMG_DATFMT_YUV420SP_VU == pstCapbXsp->enDispFmt)
            {
                stRtProcOut.stDispOut.pPlaneVir[1] = pPipelineParam2a->idx1.out_disp.pData[1];
            }

            stRtProcOut.stNrawOut.enImgFmt = (XRAY_ENERGY_SINGLE == pstCapbXray->xray_energy_num) ? DSP_IMG_DATFMT_SP16 : DSP_IMG_DATFMT_LHZP;
            stRtProcOut.stBlendOut.u32Width     = stRtProcOut.stNrawOut.u32Width     = pPipelineParam2a->idx1.out_calilhz.width;
            stRtProcOut.stBlendOut.u32Height    = stRtProcOut.stNrawOut.u32Height    = pPipelineParam2a->idx1.out_calilhz.height;
            stRtProcOut.stBlendOut.u32Stride[0] = stRtProcOut.stNrawOut.u32Stride[0] = pPipelineParam2a->idx1.out_calilhz.width;
            stRtProcOut.stNrawOut.pPlaneVir[0] = pPipelineParam2a->idx1.out_calilhz.pData[0];
            stRtProcOut.stNrawOut.pPlaneVir[1] = pPipelineParam2a->idx1.out_calilhz.pData[1];
            stRtProcOut.stNrawOut.pPlaneVir[2] = pPipelineParam2a->idx1.out_calilhz.pData[2];

            stRtProcOut.stBlendOut.enImgFmt = DSP_IMG_DATFMT_SP16;
            stRtProcOut.stBlendOut.pPlaneVir[0] = pPipelineParam2a->idx1.out_merge.pData[0];

            stRtProcOut.stAiYuvOut.enImgFmt = DSP_IMG_DATFMT_YUV420SP_VU;
            stRtProcOut.stAiYuvOut.u32Width = pPipelineParam2a->idx1.out_ai.width;
            stRtProcOut.stAiYuvOut.u32Height = pPipelineParam2a->idx1.out_ai.height;
            stRtProcOut.stAiYuvOut.u32Stride[0] = pPipelineParam2a->idx1.out_ai.stride[0];
            stRtProcOut.stAiYuvOut.pPlaneVir[0] = pPipelineParam2a->idx1.out_ai.pData[0];
            stRtProcOut.stAiYuvOut.pPlaneVir[1] = pPipelineParam2a->idx1.out_ai.pData[1];

            stRtProcOut.st_debug.u64TimeProcPipe0S = pstPipelineObj->stDebug.u64TimeProcPipe0S;
            stRtProcOut.st_debug.u64TimeProcPipe0E = pstPipelineObj->stDebug.u64TimeProcPipe0E;
            stRtProcOut.st_debug.u64TimeProcPipe1S = pstPipelineObj->stDebug.u64TimeProcPipe1S;
            stRtProcOut.st_debug.u64TimeProcPipe1E = pstPipelineObj->stDebug.u64TimeProcPipe1E;
            stRtProcOut.st_debug.u64TimeProcPipe2S = pstPipelineObj->stDebug.u64TimeProcPipe2S;
            stRtProcOut.st_debug.u64TimeProcPipe2E = pstPipelineObj->stDebug.u64TimeProcPipe2E;
            stRtProcOut.st_debug.u32SliceNo = pstPipelineObj->stDebug.u32SliceNo;
            stRtProcOut.st_package_indentify.blank_prob = xray_bgac_get_slice_prob(g_rt_pipeline[u32HandleIdx].pBgacHandle, stRtProcOut.st_debug.u32SliceNo);
            if (stRtProcOut.st_package_indentify.blank_prob < 0)
            {
                stRtProcOut.st_package_indentify.blank_prob = ~(stRtProcOut.st_package_indentify.blank_prob - 1);
            }
            else
            {
                stRtProcOut.st_package_indentify.blank_prob = 0;
            }
            /* 包裹分割结果 */
            stRtProcOut.st_package_indentify.package_num = SAL_MIN(pPipelineParam2a->idx1.packpos.nPackNum, XSP_PACK_IN1_SLICE_NUM_MAX);
            for (i = 0; i < stRtProcOut.st_package_indentify.package_num; i++)
            {
                stRtProcOut.st_package_indentify.package_rect[i].uiX = pPipelineParam2a->idx1.packpos.stPackPos[i].yStart;
                stRtProcOut.st_package_indentify.package_rect[i].uiY = pPipelineParam2a->idx1.packpos.stPackPos[i].xStart;
                stRtProcOut.st_package_indentify.package_rect[i].uiWidth = 
                    SAL_SUB_SAFE(pPipelineParam2a->idx1.packpos.stPackPos[i].yEnd, pPipelineParam2a->idx1.packpos.stPackPos[i].yStart);
                stRtProcOut.st_package_indentify.package_rect[i].uiHeight = 
                    SAL_SUB_SAFE(pPipelineParam2a->idx1.packpos.stPackPos[i].xEnd, pPipelineParam2a->idx1.packpos.stPackPos[i].xStart);
            }
            break;

        case XRAYLIB_PIPELINE_MODE3A:
        case XRAYLIB_PIPELINE_MODE3B:
            pPipelineParam3a = (XRAYLIB_PIPELINE_PARAM_MODE3A *)pstPipelineObj->pPipelineParam;

            stRtProcOut.stDispOut.enImgFmt = pstCapbXsp->enDispFmt;
            stRtProcOut.stDispOut.u32Width = pPipelineParam3a->idx2.out_disp.width;
            stRtProcOut.stDispOut.u32Height = pPipelineParam3a->idx2.out_disp.height;
            stRtProcOut.stDispOut.u32Stride[0] = pPipelineParam3a->idx2.out_disp.stride[0];
            stRtProcOut.stDispOut.pPlaneVir[0] = pPipelineParam3a->idx2.out_disp.pData[0];

            stRtProcOut.stNrawOut.enImgFmt = (XRAY_ENERGY_SINGLE == pstCapbXray->xray_energy_num) ? DSP_IMG_DATFMT_SP16 : DSP_IMG_DATFMT_LHZP;
            stRtProcOut.stBlendOut.u32Width     = stRtProcOut.stNrawOut.u32Width     = pPipelineParam3a->idx2.out_calilhz.width;
            stRtProcOut.stBlendOut.u32Height    = stRtProcOut.stNrawOut.u32Height    = pPipelineParam3a->idx2.out_calilhz.height;
            stRtProcOut.stBlendOut.u32Stride[0] = stRtProcOut.stNrawOut.u32Stride[0] = pPipelineParam3a->idx2.out_calilhz.width;
            stRtProcOut.stNrawOut.pPlaneVir[0] = pPipelineParam3a->idx2.out_calilhz.pData[0];
            stRtProcOut.stNrawOut.pPlaneVir[1] = pPipelineParam3a->idx2.out_calilhz.pData[1];
            stRtProcOut.stNrawOut.pPlaneVir[2] = pPipelineParam3a->idx2.out_calilhz.pData[2];

            stRtProcOut.stBlendOut.enImgFmt = DSP_IMG_DATFMT_SP16;
            stRtProcOut.stBlendOut.pPlaneVir[0] = pPipelineParam3a->idx2.out_merge.pData[0];

            stRtProcOut.stAiYuvOut.enImgFmt = DSP_IMG_DATFMT_YUV420SP_VU;
            stRtProcOut.stAiYuvOut.u32Width = pPipelineParam3a->idx2.out_ai.width;
            stRtProcOut.stAiYuvOut.u32Height = pPipelineParam3a->idx2.out_ai.height;
            stRtProcOut.stAiYuvOut.u32Stride[0] = pPipelineParam3a->idx2.out_ai.stride[0];
            stRtProcOut.stAiYuvOut.u32Stride[1] = pPipelineParam3a->idx2.out_ai.stride[1];
            stRtProcOut.stAiYuvOut.pPlaneVir[0] = pPipelineParam3a->idx2.out_ai.pData[0];
            stRtProcOut.stAiYuvOut.pPlaneVir[1] = pPipelineParam3a->idx2.out_ai.pData[1];

            stRtProcOut.st_debug.u64TimeProcPipe0S = pstPipelineObj->stDebug.u64TimeProcPipe0S;
            stRtProcOut.st_debug.u64TimeProcPipe0E = pstPipelineObj->stDebug.u64TimeProcPipe0E;
            stRtProcOut.st_debug.u64TimeProcPipe1S = pstPipelineObj->stDebug.u64TimeProcPipe1S;
            stRtProcOut.st_debug.u64TimeProcPipe1E = pstPipelineObj->stDebug.u64TimeProcPipe1E;
            stRtProcOut.st_debug.u64TimeProcPipe2S = pstPipelineObj->stDebug.u64TimeProcPipe2S;
            stRtProcOut.st_debug.u64TimeProcPipe2E = pstPipelineObj->stDebug.u64TimeProcPipe2E;

            stRtProcOut.st_package_indentify.blank_prob = xray_bgac_get_slice_prob(g_rt_pipeline[u32HandleIdx].pBgacHandle, stRtProcOut.st_debug.u32SliceNo);
            if (stRtProcOut.st_package_indentify.blank_prob < 0)
            {
                stRtProcOut.st_package_indentify.blank_prob = ~(stRtProcOut.st_package_indentify.blank_prob - 1);
            }
            else
            {
                stRtProcOut.st_package_indentify.blank_prob = 0;
            }

            /* 包裹分割结果 */
            stRtProcOut.st_package_indentify.package_num = SAL_MIN(pPipelineParam3a->idx2.packpos.nPackNum, XSP_PACK_IN1_SLICE_NUM_MAX);
            for (i = 0; i < stRtProcOut.st_package_indentify.package_num; i++)
            {
                stRtProcOut.st_package_indentify.package_rect[i].uiX = pPipelineParam3a->idx2.packpos.stPackPos[i].yStart;
                stRtProcOut.st_package_indentify.package_rect[i].uiY = pPipelineParam3a->idx2.packpos.stPackPos[i].xStart;
                stRtProcOut.st_package_indentify.package_rect[i].uiWidth = 
                    SAL_SUB_SAFE(pPipelineParam3a->idx2.packpos.stPackPos[i].yEnd, pPipelineParam3a->idx2.packpos.stPackPos[i].yStart);
                stRtProcOut.st_package_indentify.package_rect[i].uiHeight = 
                    SAL_SUB_SAFE(pPipelineParam3a->idx2.packpos.stPackPos[i].xEnd, pPipelineParam3a->idx2.packpos.stPackPos[i].xStart);
            }

            break;

        default:
            break;
        }

        stRtProcOut.st_xraw_tiped.tiped_pos = pstPipelineObj->st_xraw_tiped.tiped_pos;
        stRtProcOut.st_xraw_tiped.en_tiped_status = pstPipelineObj->st_xraw_tiped.en_tiped_status;
    }

    g_rt_pipeline[u32HandleIdx].rtproc_pipeline_cb(g_handle_attr[u32HandleIdx].p_handle, &stRtProcOut);

    return;
}

/**
 * @fn      rin_native_rt_pipeline_mode2a_0
 * @brief   实时预览流水线处理线程，2A模式（总计2段流水），第0段流水
 * 
 * @param   [IN] args 算法通道（Handle）实时预览流水线参数，结构体RIN_NATIVE_RT_PIPELINE
 * 
 * @return  void* 无
 */
static void *rin_native_rt_pipeline_mode2a_0(void *args)
{
    INT32 sRet = 0;
    CHAR sTaskName[SAL_THR_NAME_LEN_MAX] = {0};
    CHAR dumpName[128] = {0};
    // CHAR dumpname[128] = {0};
    UINT32 u32HandleIdx = 0;
    UINT16 *pu16FullCorr = NULL;
    UINT16 *pu16FullCorrBefore = NULL;
    UINT16 *pu16FullCorrAfter = NULL;
    DSA_LIST *pLstPipeline = NULL;
    DSA_NODE *pNode = NULL;
    RIN_NATIVE_RT_PIPELINE *p_rt_pipeline = (RIN_NATIVE_RT_PIPELINE *)args;
    PIPELINE_OBJECT_ATTR *pstPipelineObj = NULL;
    XRAYLIB_PIPELINE_PARAM_MODE2A *pPipelineParam2a = NULL;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XIMAGE_DATA_ST stAutoCorr = {0};
    UINT32 u32CorrBeforeSize = 0, u32CorrAfterSize = 0;

    if (p_rt_pipeline == NULL)
    {
        XSP_LOGE("the p_rt_pipeline is NULL\n");
        return NULL;
    }
    for (u32HandleIdx = 0; u32HandleIdx < XSP_HANDLE_NUM_MAX; u32HandleIdx++)
    {
        if (p_rt_pipeline == g_rt_pipeline + u32HandleIdx)
        {
            break;
        }
    }
    if (u32HandleIdx == XSP_HANDLE_NUM_MAX)
    {
        XSP_LOGE("cannot find matched handle\n");
        return NULL;
    }
    pLstPipeline = p_rt_pipeline->lstPipeline;

    pu16FullCorr = (UINT16 *)SAL_memMalloc(pstCapbXray->xraw_width_max * pstCapbXray->xray_energy_num * sizeof(UINT16), "xsp", "full-corr");
    if (NULL == pu16FullCorr)
    {
        XSP_LOGE("SAL_memMalloc for 'pu16FullCorr' failed\n");
        return NULL;
    }

    pu16FullCorrBefore = (UINT16 *)SAL_memMalloc(pstCapbXray->xraw_width_max * pstCapbXray->xray_energy_num * sizeof(UINT16), "xsp", "full-corr");
    if (NULL == pu16FullCorrBefore)
    {
        XSP_LOGE("SAL_memMalloc for 'pu16FullCorrBefore' failed\n");
        return NULL;
    }

    pu16FullCorrAfter = (UINT16 *)SAL_memMalloc(pstCapbXray->xraw_width_max * pstCapbXray->xray_energy_num * sizeof(UINT16), "xsp", "full-corr");
    if (NULL == pu16FullCorrAfter)
    {
        XSP_LOGE("SAL_memMalloc for 'pu16FullCorrAfter' failed\n");
        return NULL;
    }



    /* 修改系统默认线程名称 */
    snprintf(sTaskName, sizeof(sTaskName), "rtpipe-2a0-%u", u32HandleIdx);
    prctl(PR_SET_NAME, (unsigned long)sTaskName);

    while (1)
    {
        SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (NULL == (pNode = get_node_by_pipeseg(pLstPipeline, 0)))
        {
            SAL_CondWait(&pLstPipeline->sync, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }
        SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);

        pstPipelineObj = (PIPELINE_OBJECT_ATTR *)pNode->pAdData;
        pPipelineParam2a = (XRAYLIB_PIPELINE_PARAM_MODE2A *)pstPipelineObj->pPipelineParam;

        if (pstPipelineObj->stDebug.bDumpEnable)
        {
            snprintf(dumpName, sizeof(dumpName), "%s/p1-in_ch%u_n%u_t%llu_w%u_h%u_s%u.xraw", 
                     pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                     pPipelineParam2a->idx0.in_xraw.width, pPipelineParam2a->idx0.in_xraw.height, pPipelineParam2a->idx0.in_xraw.stride[0]);
            SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam2a->idx0.in_xraw.pData[0],
                            XRAW_LE_SIZE(pPipelineParam2a->idx0.in_xraw.width, pPipelineParam2a->idx0.in_xraw.height));
            SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam2a->idx0.in_xraw.pData[1],
                            XRAW_HE_SIZE(pPipelineParam2a->idx0.in_xraw.width, pPipelineParam2a->idx0.in_xraw.height));
        }

        rin_native_2ximg(&pPipelineParam2a->idx0.in_xraw, &stAutoCorr, __FUNCTION__, __LINE__);
        if (SAL_SOK == xray_bg_auto_corr(p_rt_pipeline->pBgacHandle, &stAutoCorr, pstPipelineObj->bPseudo,
                                         pstPipelineObj->stDebug.u32SliceNo, pu16FullCorr))
        {
            
            // memset(dumpname, 0, sizeof(dumpname));
            // snprintf(dumpname, sizeof(dumpname), "/mnt/dumpCorr/Chan%d_Updated.txt", u32HandleIdx);
            // memset(dumpName, 0, sizeof(dumpName));
            // snprintf(dumpName, sizeof(dumpName), "Chan %d, SliceNo %d SAL_SOK\n", u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo);
            // SAL_WriteToFile(dumpname, 0, SEEK_END, dumpName, sizeof(dumpName));



            if (u32HandleIdx == 0)
            {
                
                rin_native_get_correction_data(g_handle_attr[u32HandleIdx].p_handle, XSP_CORR_FULLLOAD, pu16FullCorrBefore, &u32CorrBeforeSize);
                if (0 != memcmp(pu16FullCorrBefore, pu16FullCorr, u32CorrBeforeSize))
                {
                    // XSP_LOGE("SliceNo %d: Before AutoFull Template is Equal To Current Setting Template.\n", pstPipelineObj->stDebug.u32SliceNo);
                    // snprintf(dumpName, sizeof(dumpName), "/mnt/dumpCorr/trunk/before/before_corrfull_sliceNo%d_width%d.raw", pstPipelineObj->stDebug.u32SliceNo, u32CorrBeforeSize);

                    // SAL_WriteToFile(dumpName, 0, SEEK_SET, pu16FullCorrBefore, u32CorrBeforeSize);

                    // snprintf(dumpName, sizeof(dumpName), "/mnt/dumpCorr/trunk/current/current_corrfull_sliceNo%d_width%d.raw", pstPipelineObj->stDebug.u32SliceNo, u32CorrBeforeSize);

                    // SAL_WriteToFile(dumpName, 0, SEEK_SET, pu16FullCorr, u32CorrBeforeSize);
                }

                rin_native_set_correction_data(g_handle_attr[u32HandleIdx].p_handle, XSP_CORR_AUTOFULL, pu16FullCorr, stAutoCorr.u32Width, 1);

                rin_native_get_correction_data(g_handle_attr[u32HandleIdx].p_handle, XSP_CORR_FULLLOAD, pu16FullCorrAfter, &u32CorrAfterSize);


                if (0 == memcmp(pu16FullCorrAfter, pu16FullCorr, u32CorrBeforeSize))
                {
                    // XSP_LOGE("SliceNo %d: After Setted AutoFull Template is Not Equal Current Algorithm Template.\n", pstPipelineObj->stDebug.u32SliceNo);
                    // snprintf(dumpName, sizeof(dumpName), "/mnt/dumpCorr/trunk/after/after_corrfull_sliceNo%d_width%d.raw", pstPipelineObj->stDebug.u32SliceNo, u32CorrAfterSize);

                    // SAL_WriteToFile(dumpName, 0, SEEK_SET, pu16FullCorrAfter, u32CorrAfterSize);
                }

                // snprintf(dumpName, sizeof(dumpName), "/mnt/dumpCorr/package/Chan%dsliceNo%d_width%d_height%d.xraw", 
                //         u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, pPipelineParam2a->idx0.in_xraw.width, pPipelineParam2a->idx0.in_xraw.height);
                // SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam2a->idx0.in_xraw.pData[0],
                //                 XRAW_LE_SIZE(pPipelineParam2a->idx0.in_xraw.width, pPipelineParam2a->idx0.in_xraw.height));
                // SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam2a->idx0.in_xraw.pData[1],
                //                 XRAW_HE_SIZE(pPipelineParam2a->idx0.in_xraw.width, pPipelineParam2a->idx0.in_xraw.height));
            }
        }
        else
        {
            // memset(dumpname, 0, sizeof(dumpname));
            // snprintf(dumpname, sizeof(dumpname), "/mnt/dumpCorr/Chan%d_NotUpdated.txt", u32HandleIdx);
            // memset(dumpName, 0, sizeof(dumpName));
            // snprintf(dumpName, sizeof(dumpName), "Chan %d, SliceNo %d SAL_FAIL\n", u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo);
            // SAL_WriteToFile(dumpname, 0, SEEK_END, dumpName, sizeof(dumpName));
        }

        pstPipelineObj->stDebug.u64TimeProcPipe0S = sal_get_tickcnt();
        sRet = libXRay_PipelineProcess(g_handle_attr[u32HandleIdx].p_handle, pstPipelineObj->pPipelineParam, PIPELINE_STAGE_0);
        pstPipelineObj->stDebug.u64TimeProcPipe0E = sal_get_tickcnt();
        if (sRet == XRAY_LIB_OK)
        {
            // 保存上一个条带的透传数据，注：只有流水段1才会缓存条带
            SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            if (0 == pPipelineParam2a->idx0.out_calilhz.width || 0 == pPipelineParam2a->idx0.out_calilhz.height)
            {
                rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_EINTR);
                DSA_LstDelete(pLstPipeline, pNode); /* 算法缓存一个条带 */
            }
            else
            {
                pstPipelineObj->s32CurPipeSeg++;
                if (pstPipelineObj->stDebug.bDumpEnable)
                {
                    snprintf(dumpName, sizeof(dumpName), "%s/p1-out_ch%u_n%u_t%llu_w%u_h%u_s%u.calilh", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam2a->idx0.out_calilhz_ori.width, pPipelineParam2a->idx0.out_calilhz_ori.height, 
                             pPipelineParam2a->idx0.out_calilhz_ori.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam2a->idx0.out_calilhz_ori.pData[0],
                                    XRAW_LE_SIZE(pPipelineParam2a->idx0.out_calilhz_ori.width, pPipelineParam2a->idx0.out_calilhz_ori.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam2a->idx0.out_calilhz_ori.pData[1],
                                    XRAW_HE_SIZE(pPipelineParam2a->idx0.out_calilhz_ori.width, pPipelineParam2a->idx0.out_calilhz_ori.height));
                    snprintf(dumpName, sizeof(dumpName), "%s/p1-out_ch%u_n%u_t%llu_w%u_h%u_s%u.calilhz", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam2a->idx0.out_calilhz.width, pPipelineParam2a->idx0.out_calilhz.height, 
                             pPipelineParam2a->idx0.out_calilhz.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam2a->idx0.out_calilhz.pData[0],
                                    XRAW_LE_SIZE(pPipelineParam2a->idx0.out_calilhz.width, pPipelineParam2a->idx0.out_calilhz.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam2a->idx0.out_calilhz.pData[1],
                                    XRAW_HE_SIZE(pPipelineParam2a->idx0.out_calilhz.width, pPipelineParam2a->idx0.out_calilhz.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam2a->idx0.out_calilhz.pData[2],
                                    XRAW_Z_SIZE(pPipelineParam2a->idx0.out_calilhz.width, pPipelineParam2a->idx0.out_calilhz.height));
                }
            }
            SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pLstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
        else
        {
            XSP_LOGW("chan %u, libXRay_PipelineProcess failed, sRet: 0x%x\n", u32HandleIdx, sRet);
            SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_FAIL);
            DSA_LstDelete(pLstPipeline, pNode);
            SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pLstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
    }
    return NULL;
}

/**
 * @fn      rin_native_rt_pipeline_mode2a_1
 * @brief   实时预览流水线处理线程，2A模式（总计2段流水），第1段流水
 * 
 * @param   [IN] args 算法通道（Handle）实时预览流水线参数，结构体RIN_NATIVE_RT_PIPELINE
 * 
 * @return  void* 无
 */
static void *rin_native_rt_pipeline_mode2a_1(void *args)
{
    INT32 sRet = 0;
    CHAR sTaskName[SAL_THR_NAME_LEN_MAX] = {0};
    CHAR dumpName[128] = {0};
    UINT32 u32HandleIdx = 0;
    DSA_LIST *pLstPipeline = NULL;
    DSA_NODE *pNode = NULL;
    RIN_NATIVE_RT_PIPELINE *p_rt_pipeline = (RIN_NATIVE_RT_PIPELINE *)args;
    PIPELINE_OBJECT_ATTR *pstPipelineObj = NULL;
    XRAYLIB_PIPELINE_PARAM_MODE2A *pPipelineParam2a = NULL;

    if (p_rt_pipeline == NULL)
    {
        XSP_LOGE("the p_rt_pipeline is NULL\n");
        return NULL;
    }
    for (u32HandleIdx = 0; u32HandleIdx < XSP_HANDLE_NUM_MAX; u32HandleIdx++)
    {
        if (p_rt_pipeline == g_rt_pipeline + u32HandleIdx)
        {
            break;
        }
    }
    if (u32HandleIdx == XSP_HANDLE_NUM_MAX)
    {
        XSP_LOGE("cannot find matched handle\n");
        return NULL;
    }
    pLstPipeline = p_rt_pipeline->lstPipeline;

    /* 修改系统默认线程名称 */
    snprintf(sTaskName, sizeof(sTaskName), "rtpipe-2a1-%u", u32HandleIdx);
    prctl(PR_SET_NAME, (unsigned long)sTaskName);

    while (1)
    {
        SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (NULL == (pNode = get_node_by_pipeseg(pLstPipeline, 1)))
        {
            SAL_CondWait(&pLstPipeline->sync, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }
        SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);

        pstPipelineObj = (PIPELINE_OBJECT_ATTR *)pNode->pAdData;
        pPipelineParam2a = (XRAYLIB_PIPELINE_PARAM_MODE2A *)pstPipelineObj->pPipelineParam;
        memcpy(&pPipelineParam2a->idx1.in_calilhz, &pPipelineParam2a->idx0.out_calilhz, sizeof(XRAYLIB_IMAGE));
        memcpy(&pPipelineParam2a->idx1.in_calilhz_ori, &pPipelineParam2a->idx0.out_calilhz_ori, sizeof(XRAYLIB_IMAGE));

        pstPipelineObj->stDebug.u64TimeProcPipe2S = sal_get_tickcnt();
        sRet = libXRay_PipelineProcess(g_handle_attr[u32HandleIdx].p_handle, pstPipelineObj->pPipelineParam, PIPELINE_STAGE_1);
        pstPipelineObj->stDebug.u64TimeProcPipe2E = sal_get_tickcnt();
        XSP_LOGD("\nout_calilhz: fmt[%d], w[%u], h[%u], s[%u]\n"
                 "out_merge: fmt[%d], w[%u], h[%u], s[%u]\n"
                 "out_disp: fmt[%d], w[%u], h[%u], s[%u]\n"
                 "out_ai: fmt[%d], w[%u], h[%u], s[%u]\n", 
                 pPipelineParam2a->idx1.out_calilhz.format, pPipelineParam2a->idx1.out_calilhz.width, pPipelineParam2a->idx1.out_calilhz.height, pPipelineParam2a->idx1.out_calilhz.stride[0], 
                 pPipelineParam2a->idx1.out_merge.format, pPipelineParam2a->idx1.out_merge.width, pPipelineParam2a->idx1.out_merge.height, pPipelineParam2a->idx1.out_merge.stride[0], 
                 pPipelineParam2a->idx1.out_disp.format, pPipelineParam2a->idx1.out_disp.width, pPipelineParam2a->idx1.out_disp.height, pPipelineParam2a->idx1.out_disp.stride[0], 
                 pPipelineParam2a->idx1.out_ai.format, pPipelineParam2a->idx1.out_ai.width, pPipelineParam2a->idx1.out_ai.height, pPipelineParam2a->idx1.out_ai.stride[0]);
        /* tip注入结果 */
        if (XSP_TIPED_NONE == pstPipelineObj->st_xraw_tiped.en_tiped_status)
        {
            if (XRAYLIB_TIP_WORKING == pPipelineParam2a->idx1.TipStatus)
            {
                pstPipelineObj->st_xraw_tiped.tiped_pos =  pPipelineParam2a->idx1.TipParam.nTipedPosW;
                pstPipelineObj->st_xraw_tiped.en_tiped_status = XSP_TIPED_PROCESSING;
            }
            else if (XRAYLIB_TIP_END ==  pPipelineParam2a->idx1.TipStatus)
            {
                pstPipelineObj->st_xraw_tiped.tiped_pos = -1; /*无效*/
                pstPipelineObj->st_xraw_tiped.en_tiped_status = XSP_TIPED_SUCESS;
            }
            else if (XRAYLIB_TIP_NONE ==  pPipelineParam2a->idx1.TipStatus)
            {
                pstPipelineObj->st_xraw_tiped.tiped_pos = -1; /*无效*/
                pstPipelineObj->st_xraw_tiped.en_tiped_status = XSP_TIPED_NONE;
            }
            else
            {
                pstPipelineObj->st_xraw_tiped.tiped_pos = -1; /*无效*/
                pstPipelineObj->st_xraw_tiped.en_tiped_status = XSP_TIPED_FAIL;
                XSP_LOGW("tip: en_tiped_status %d, 0x%x!\n",  pstPipelineObj->st_xraw_tiped.en_tiped_status, pPipelineParam2a->idx1.TipStatus);
            }
        }
        if (sRet == XRAY_LIB_OK)
        {
            pstPipelineObj->s32CurPipeSeg++;
            SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            if (pPipelineParam2a->idx1.out_calilhz.width > 0 && pPipelineParam2a->idx1.out_calilhz.height > 0)
            {
                rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_SOK);
                if (pstPipelineObj->stDebug.bDumpEnable)
                {
                    snprintf(dumpName, sizeof(dumpName), "%s/p2-out_ch%u_n%u_t%llu_w%u_h%u_s%u.calilhz", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam2a->idx1.out_calilhz.width, pPipelineParam2a->idx1.out_calilhz.height, 
                             pPipelineParam2a->idx1.out_calilhz.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam2a->idx1.out_calilhz.pData[0],
                                    XRAW_LE_SIZE(pPipelineParam2a->idx1.out_calilhz.width, pPipelineParam2a->idx1.out_calilhz.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam2a->idx1.out_calilhz.pData[1],
                                    XRAW_HE_SIZE(pPipelineParam2a->idx1.out_calilhz.width, pPipelineParam2a->idx1.out_calilhz.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam2a->idx1.out_calilhz.pData[2],
                                    XRAW_Z_SIZE(pPipelineParam2a->idx1.out_calilhz.width, pPipelineParam2a->idx1.out_calilhz.height));
                    snprintf(dumpName, sizeof(dumpName), "%s/p2-out_ch%u_n%u_t%llu_w%u_h%u_s%u.merge", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam2a->idx1.out_merge.width, pPipelineParam2a->idx1.out_merge.height, 
                             pPipelineParam2a->idx1.out_merge.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam2a->idx1.out_merge.pData[0],
                                    pPipelineParam2a->idx1.out_merge.width * pPipelineParam2a->idx1.out_merge.height * sizeof(UINT32));
                    snprintf(dumpName, sizeof(dumpName), "%s/p2-out_ch%u_n%u_t%llu_w%u_h%u_s%u.argb", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam2a->idx1.out_disp.width, pPipelineParam2a->idx1.out_disp.height, 
                             pPipelineParam2a->idx1.out_disp.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam2a->idx1.out_disp.pData[0],
                                    pPipelineParam2a->idx1.out_disp.height * pPipelineParam2a->idx1.out_disp.stride[0] * sizeof(UINT32));
                    snprintf(dumpName, sizeof(dumpName), "%s/p2-out_ch%u_n%u_t%llu_w%u_h%u_s%u-%u.nv21", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam2a->idx1.out_ai.width, pPipelineParam2a->idx1.out_ai.height, 
                             pPipelineParam2a->idx1.out_ai.stride[0], pPipelineParam2a->idx1.out_ai.stride[1]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam2a->idx1.out_ai.pData[0],
                                    pPipelineParam2a->idx1.out_ai.height * pPipelineParam2a->idx1.out_ai.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam2a->idx1.out_ai.pData[1],
                                    pPipelineParam2a->idx1.out_ai.height * pPipelineParam2a->idx1.out_ai.stride[0] / 2);
                    if (XRAYLIB_TIP_NONE != pPipelineParam2a->idx1.TipStatus)
                    {
                        snprintf(dumpName, sizeof(dumpName), "%s/p2-out_tip_ch%u_n%u_t%llu_w%u_h%u_s%u.calilhz", 
                                pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                                pPipelineParam2a->idx1.out_calilhz.width, pPipelineParam2a->idx1.out_calilhz.height, 
                                pPipelineParam2a->idx1.out_calilhz.stride[0]);
                        SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam2a->idx1.TipParam.pTipedCaliLowData, 
                                        XRAW_LE_SIZE(pPipelineParam2a->idx1.out_calilhz.stride[0], pPipelineParam2a->idx1.out_calilhz.height));
                        SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam2a->idx1.TipParam.pTipedCaliHighData, 
                                        XRAW_HE_SIZE(pPipelineParam2a->idx1.out_calilhz.stride[1], pPipelineParam2a->idx1.out_calilhz.height));
                        SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam2a->idx1.TipParam.pTipedZData, 
                                        XRAW_Z_SIZE(pPipelineParam2a->idx1.out_calilhz.stride[2], pPipelineParam2a->idx1.out_calilhz.height));
                    }
                }
            }
            else
            {
                rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_FAIL);
                XSP_LOGW("chan %u, libXRay_PipelineProcess failed, out width(%u) & height(%u)\n", u32HandleIdx, 
                         pPipelineParam2a->idx1.out_calilhz.width, pPipelineParam2a->idx1.out_calilhz.height);
            }
            DSA_LstDelete(pLstPipeline, pNode);
            SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pLstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
        else
        {
            XSP_LOGW("chan %u, libXRay_PipelineProcess failed, sRet: 0x%x\n", u32HandleIdx, sRet);
            SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_FAIL);
            DSA_LstDelete(pLstPipeline, pNode);
            SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pLstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
    }
    return NULL;
}


/**
 * @fn      rin_native_rt_pipeline_mode3ab_0
 * @brief   实时预览流水线处理线程，3A模式（总计3段流水），第0段流水
 * 
 * @param   [IN] args 算法通道（Handle）实时预览流水线参数，结构体RIN_NATIVE_RT_PIPELINE
 * 
 * @return  void* 无
 */
static void *rin_native_rt_pipeline_mode3ab_0(void *args)
{
    INT32 sRet = 0;
    CHAR sTaskName[SAL_THR_NAME_LEN_MAX] = {0};
    CHAR dumpName[128] = {0};
    UINT32 u32HandleIdx = 0;
    UINT16 *pu16FullCorr = NULL;
    UINT16 *pu16FullCorrBefore = NULL;
    UINT16 *pu16FullCorrAfter = NULL;
    DSA_LIST *pLstPipeline = NULL;
    DSA_NODE *pNode = NULL;
    RIN_NATIVE_RT_PIPELINE *p_rt_pipeline = (RIN_NATIVE_RT_PIPELINE *)args;
    PIPELINE_OBJECT_ATTR *pstPipelineObj = NULL;
    XRAYLIB_PIPELINE_PARAM_MODE3A *pPipelineParam3a = NULL;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XIMAGE_DATA_ST stAutoCorr = {0};
    UINT32 u32CorrBeforeSize = 0, u32CorrAfterSize = 0;

    if (p_rt_pipeline == NULL)
    {
        XSP_LOGE("the p_rt_pipeline is NULL\n");
        return NULL;
    }
    for (u32HandleIdx = 0; u32HandleIdx < XSP_HANDLE_NUM_MAX; u32HandleIdx++)
    {
        if (p_rt_pipeline == g_rt_pipeline + u32HandleIdx)
        {
            break;
        }
    }
    if (u32HandleIdx == XSP_HANDLE_NUM_MAX)
    {
        XSP_LOGE("cannot find matched handle\n");
        return NULL;
    }
    pLstPipeline = p_rt_pipeline->lstPipeline;

    pu16FullCorr = (UINT16 *)SAL_memMalloc(pstCapbXray->xraw_width_max * pstCapbXray->xray_energy_num * sizeof(UINT16), "xsp", "full-corr");
    if (NULL == pu16FullCorr)
    {
        XSP_LOGE("SAL_memMalloc for 'pu16FullCorr' failed\n");
        return NULL;
    }

    pu16FullCorrBefore = (UINT16 *)SAL_memMalloc(pstCapbXray->xraw_width_max * pstCapbXray->xray_energy_num * sizeof(UINT16), "xsp", "full-corr");
    if (NULL == pu16FullCorrBefore)
    {
        XSP_LOGE("SAL_memMalloc for 'pu16FullCorrBefore' failed\n");
        return NULL;
    }

    pu16FullCorrAfter = (UINT16 *)SAL_memMalloc(pstCapbXray->xraw_width_max * pstCapbXray->xray_energy_num * sizeof(UINT16), "xsp", "full-corr");
    if (NULL == pu16FullCorrAfter)
    {
        XSP_LOGE("SAL_memMalloc for 'pu16FullCorrAfter' failed\n");
        return NULL;
    }

    /* 修改系统默认线程名称 */
    snprintf(sTaskName, sizeof(sTaskName), "rtpipe-3a0-%u", u32HandleIdx);
    prctl(PR_SET_NAME, (unsigned long)sTaskName);

    while (1)
    {
        SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (NULL == (pNode = get_node_by_pipeseg(pLstPipeline, 0)))
        {
            SAL_CondWait(&pLstPipeline->sync, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }
        SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);

        pstPipelineObj = (PIPELINE_OBJECT_ATTR *)pNode->pAdData;
        pPipelineParam3a = (XRAYLIB_PIPELINE_PARAM_MODE3A *)pstPipelineObj->pPipelineParam;

        if (pstPipelineObj->stDebug.bDumpEnable)
        {
            snprintf(dumpName, sizeof(dumpName), "%s/p1-in_ch%u_n%u_t%llu_w%u_h%u_s%u.xraw", 
                     pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                     pPipelineParam3a->idx0.in_xraw.width, pPipelineParam3a->idx0.in_xraw.height, pPipelineParam3a->idx0.in_xraw.stride[0]);
            SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam3a->idx0.in_xraw.pData[0],
                            XRAW_LE_SIZE(pPipelineParam3a->idx0.in_xraw.width, pPipelineParam3a->idx0.in_xraw.height));
            SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx0.in_xraw.pData[1],
                            XRAW_HE_SIZE(pPipelineParam3a->idx0.in_xraw.width, pPipelineParam3a->idx0.in_xraw.height));
        }

        rin_native_2ximg(&pPipelineParam3a->idx0.in_xraw, &stAutoCorr, __FUNCTION__, __LINE__);
        if (SAL_SOK == xray_bg_auto_corr(p_rt_pipeline->pBgacHandle, &stAutoCorr, pstPipelineObj->bPseudo,
                                         pstPipelineObj->stDebug.u32SliceNo, pu16FullCorr))
        {
            rin_native_get_correction_data(g_handle_attr[u32HandleIdx].p_handle, XSP_CORR_FULLLOAD, pu16FullCorrBefore, &u32CorrBeforeSize);

            if (0 == memcmp(pu16FullCorrBefore, pu16FullCorr, u32CorrBeforeSize))
            {
                XSP_LOGE("SliceNo %d: Before AutoFull Template is Not Equal Current Setting Template.\n", pstPipelineObj->stDebug.u32SliceNo);
            }

            rin_native_set_correction_data(g_handle_attr[u32HandleIdx].p_handle, XSP_CORR_AUTOFULL, pu16FullCorr, stAutoCorr.u32Width, 1);

            rin_native_get_correction_data(g_handle_attr[u32HandleIdx].p_handle, XSP_CORR_FULLLOAD, pu16FullCorrAfter, &u32CorrAfterSize);

            if (0 != memcmp(pu16FullCorrBefore, pu16FullCorr, u32CorrBeforeSize))
            {
                XSP_LOGE("SliceNo %d: After Setted AutoFull Template is Not Equal Current Algorithm Template.\n", pstPipelineObj->stDebug.u32SliceNo);
            }
        }

        pstPipelineObj->stDebug.u64TimeProcPipe0S = sal_get_tickcnt();
        sRet = libXRay_PipelineProcess(g_handle_attr[u32HandleIdx].p_handle, pstPipelineObj->pPipelineParam, PIPELINE_STAGE_0);
        pstPipelineObj->stDebug.u64TimeProcPipe0E = sal_get_tickcnt();
        if (sRet == XRAY_LIB_OK)
        {
            // 保存上一个条带的透传数据，注：只有流水段1才会缓存条带
            SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            if (0 == pPipelineParam3a->idx0.out_calilhz.width || 0 == pPipelineParam3a->idx0.out_calilhz.height)
            {
                rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_EINTR);
                DSA_LstDelete(pLstPipeline, pNode); /* 算法缓存一个条带 */
            }
            else
            {
                pstPipelineObj->s32CurPipeSeg++;
                if (pstPipelineObj->stDebug.bDumpEnable)
                {
                    snprintf(dumpName, sizeof(dumpName), "%s/p1-out_ch%u_n%u_t%llu_w%u_h%u_s%u.calilh", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam3a->idx0.out_calilhz_ori.width, pPipelineParam3a->idx0.out_calilhz_ori.height, 
                             pPipelineParam3a->idx0.out_calilhz_ori.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam3a->idx0.out_calilhz_ori.pData[0],
                                    XRAW_LE_SIZE(pPipelineParam3a->idx0.out_calilhz_ori.width, pPipelineParam3a->idx0.out_calilhz_ori.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx0.out_calilhz_ori.pData[1],
                                    XRAW_HE_SIZE(pPipelineParam3a->idx0.out_calilhz_ori.width, pPipelineParam3a->idx0.out_calilhz_ori.height));
                    snprintf(dumpName, sizeof(dumpName), "%s/p1-out_ch%u_n%u_t%llu_w%u_h%u_s%u.calilhz", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam3a->idx0.out_calilhz.width, pPipelineParam3a->idx0.out_calilhz.height, 
                             pPipelineParam3a->idx0.out_calilhz.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam3a->idx0.out_calilhz.pData[0],
                                    XRAW_LE_SIZE(pPipelineParam3a->idx0.out_calilhz.width, pPipelineParam3a->idx0.out_calilhz.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx0.out_calilhz.pData[1],
                                    XRAW_HE_SIZE(pPipelineParam3a->idx0.out_calilhz.width, pPipelineParam3a->idx0.out_calilhz.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx0.out_calilhz.pData[2],
                                    XRAW_ORI_Z_SIZE(pPipelineParam3a->idx0.out_calilhz.width, pPipelineParam3a->idx0.out_calilhz.height));
                }
            }
            SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pLstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
        else
        {
            XSP_LOGW("chan %u, libXRay_PipelineProcess failed, sRet: 0x%x\n", u32HandleIdx, sRet);
            SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_FAIL);
            DSA_LstDelete(pLstPipeline, pNode);
            SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pLstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
    }

    return NULL;
}


/**
 * @fn      rin_native_rt_pipeline_mode3ab_1
 * @brief   实时预览流水线处理线程，3A模式（总计3段流水），第1段流水
 * 
 * @param   [IN] args 算法通道（Handle）实时预览流水线参数，结构体RIN_NATIVE_RT_PIPELINE
 * 
 * @return  void* 无
 */
static void *rin_native_rt_pipeline_mode3ab_1(void *args)
{
    INT32 sRet = 0;
    CHAR sTaskName[SAL_THR_NAME_LEN_MAX] = {0};
    CHAR dumpName[128] = {0};
    UINT32 u32HandleIdx = 0;
    DSA_LIST *pLstPipeline = NULL;
    DSA_NODE *pNode = NULL;
    RIN_NATIVE_RT_PIPELINE *p_rt_pipeline = (RIN_NATIVE_RT_PIPELINE *)args;
    PIPELINE_OBJECT_ATTR *pstPipelineObj = NULL;
    XRAYLIB_PIPELINE_PARAM_MODE3A *pPipelineParam3a = NULL;

    if (p_rt_pipeline == NULL)
    {
        XSP_LOGE("the p_rt_pipeline is NULL\n");
        return NULL;
    }
    for (u32HandleIdx = 0; u32HandleIdx < XSP_HANDLE_NUM_MAX; u32HandleIdx++)
    {
        if (p_rt_pipeline == g_rt_pipeline + u32HandleIdx)
        {
            break;
        }
    }
    if (u32HandleIdx == XSP_HANDLE_NUM_MAX)
    {
        XSP_LOGE("cannot find matched handle\n");
        return NULL;
    }
    pLstPipeline = p_rt_pipeline->lstPipeline;

    /* 修改系统默认线程名称 */
    snprintf(sTaskName, sizeof(sTaskName), "rtpipe-3a1-%u", u32HandleIdx);
    prctl(PR_SET_NAME, (unsigned long)sTaskName);

    while (1)
    {
        SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (NULL == (pNode = get_node_by_pipeseg(pLstPipeline, 1)))
        {
            SAL_CondWait(&pLstPipeline->sync, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }
        SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);

        pstPipelineObj = (PIPELINE_OBJECT_ATTR *)pNode->pAdData;
        pPipelineParam3a = (XRAYLIB_PIPELINE_PARAM_MODE3A *)pstPipelineObj->pPipelineParam;
        memcpy(&pPipelineParam3a->idx1.in_calilhz, &pPipelineParam3a->idx0.out_calilhz, sizeof(XRAYLIB_IMAGE));

        pstPipelineObj->stDebug.u64TimeProcPipe1S = sal_get_tickcnt();
        sRet = libXRay_PipelineProcess(g_handle_attr[u32HandleIdx].p_handle, pstPipelineObj->pPipelineParam, PIPELINE_STAGE_1);
        pstPipelineObj->stDebug.u64TimeProcPipe1E = sal_get_tickcnt();
        if (sRet == XRAY_LIB_OK)
        {
            SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            if (0 == pPipelineParam3a->idx1.out_calilhz.width || 0 == pPipelineParam3a->idx1.out_calilhz.height)
            {
                rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_FAIL);
                XSP_LOGW("chan %u, libXRay_PipelineProcess failed, out width(%u) & height(%u)\n", u32HandleIdx, 
                         pPipelineParam3a->idx1.out_calilhz.width, pPipelineParam3a->idx1.out_calilhz.height);
                DSA_LstDelete(pLstPipeline, pNode); /* 非法数据，直接删除 */
            }
            else
            {
                pstPipelineObj->s32CurPipeSeg++;
                if (pstPipelineObj->stDebug.bDumpEnable)
                {
                    snprintf(dumpName, sizeof(dumpName), "%s/p2-out_ch%u_n%u_t%llu_w%u_h%u_s%u.calilhz", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam3a->idx1.out_calilhz.width, pPipelineParam3a->idx1.out_calilhz.height, 
                             pPipelineParam3a->idx1.out_calilhz.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam3a->idx1.out_calilhz.pData[0],
                                    XRAW_LE_SIZE(pPipelineParam3a->idx1.out_calilhz.width, pPipelineParam3a->idx1.out_calilhz.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx1.out_calilhz.pData[1],
                                    XRAW_HE_SIZE(pPipelineParam3a->idx1.out_calilhz.width, pPipelineParam3a->idx1.out_calilhz.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx1.out_calilhz.pData[2],
                                    XRAW_Z_SIZE(pPipelineParam3a->idx1.out_calilhz.width, pPipelineParam3a->idx1.out_calilhz.height));
                }
            }
            SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pLstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
        else
        {
            XSP_LOGW("chan %u, libXRay_PipelineProcess failed, sRet: 0x%x\n", u32HandleIdx, sRet);
            SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_FAIL);
            DSA_LstDelete(pLstPipeline, pNode);
            SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pLstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
    }

    return NULL;
}


/**
 * @fn      rin_native_rt_pipeline_mode3ab_2
 * @brief   实时预览流水线处理线程，3A模式（总计3段流水），第2段流水
 * 
 * @param   [IN] args 算法通道（Handle）实时预览流水线参数，结构体RIN_NATIVE_RT_PIPELINE
 * 
 * @return  void* 无
 */
static void *rin_native_rt_pipeline_mode3ab_2(void *args)
{
    INT32 sRet = 0;
    CHAR sTaskName[SAL_THR_NAME_LEN_MAX] = {0};
    CHAR dumpName[128] = {0};
    UINT32 u32HandleIdx = 0;
    DSA_LIST *pLstPipeline = NULL;
    DSA_NODE *pNode = NULL;
    RIN_NATIVE_RT_PIPELINE *p_rt_pipeline = (RIN_NATIVE_RT_PIPELINE *)args;
    PIPELINE_OBJECT_ATTR *pstPipelineObj = NULL;
    XRAYLIB_PIPELINE_PARAM_MODE3A *pPipelineParam3a = NULL;

    if (p_rt_pipeline == NULL)
    {
        XSP_LOGE("the p_rt_pipeline is NULL\n");
        return NULL;
    }
    for (u32HandleIdx = 0; u32HandleIdx < XSP_HANDLE_NUM_MAX; u32HandleIdx++)
    {
        if (p_rt_pipeline == g_rt_pipeline + u32HandleIdx)
        {
            break;
        }
    }
    if (u32HandleIdx == XSP_HANDLE_NUM_MAX)
    {
        XSP_LOGE("cannot find matched handle\n");
        return NULL;
    }
    pLstPipeline = p_rt_pipeline->lstPipeline;

    /* 修改系统默认线程名称 */
    snprintf(sTaskName, sizeof(sTaskName), "rtpipe-3a2-%u", u32HandleIdx);
    prctl(PR_SET_NAME, (unsigned long)sTaskName);

    while (1)
    {
        SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        while (NULL == (pNode = get_node_by_pipeseg(pLstPipeline, 2)))
        {
            SAL_CondWait(&pLstPipeline->sync, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
        }
        SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);

        pstPipelineObj = (PIPELINE_OBJECT_ATTR *)pNode->pAdData;
        pPipelineParam3a = (XRAYLIB_PIPELINE_PARAM_MODE3A *)pstPipelineObj->pPipelineParam;
        memcpy(&pPipelineParam3a->idx2.in_calilhz, &pPipelineParam3a->idx1.out_calilhz, sizeof(XRAYLIB_IMAGE));
        memcpy(&pPipelineParam3a->idx2.in_calilhz_ori, &pPipelineParam3a->idx0.out_calilhz_ori, sizeof(XRAYLIB_IMAGE));

        pstPipelineObj->stDebug.u64TimeProcPipe2S = sal_get_tickcnt();
        sRet = libXRay_PipelineProcess(g_handle_attr[u32HandleIdx].p_handle, pstPipelineObj->pPipelineParam, PIPELINE_STAGE_2);
        pstPipelineObj->stDebug.u64TimeProcPipe2E = sal_get_tickcnt();
        
        XSP_LOGD("\nout_calilhz: fmt[%d], w[%u], h[%u], s[%u]\n"
                 "out_merge: fmt[%d], w[%u], h[%u], s[%u]\n"
                 "out_disp: fmt[%d], w[%u], h[%u], s[%u]\n"
                 "out_ai: fmt[%d], w[%u], h[%u], s[%u]\n", 
                 pPipelineParam3a->idx2.out_calilhz.format, pPipelineParam3a->idx2.out_calilhz.width, pPipelineParam3a->idx2.out_calilhz.height, pPipelineParam3a->idx2.out_calilhz.stride[0], 
                 pPipelineParam3a->idx2.out_merge.format, pPipelineParam3a->idx2.out_merge.width, pPipelineParam3a->idx2.out_merge.height, pPipelineParam3a->idx2.out_merge.stride[0], 
                 pPipelineParam3a->idx2.out_disp.format, pPipelineParam3a->idx2.out_disp.width, pPipelineParam3a->idx2.out_disp.height, pPipelineParam3a->idx2.out_disp.stride[0], 
                 pPipelineParam3a->idx2.out_ai.format, pPipelineParam3a->idx2.out_ai.width, pPipelineParam3a->idx2.out_ai.height, pPipelineParam3a->idx2.out_ai.stride[0]
                 );
        /* tip注入结果 */
        if (XSP_TIPED_NONE == pstPipelineObj->st_xraw_tiped.en_tiped_status)
        {
            if (XRAYLIB_TIP_WORKING == pPipelineParam3a->idx2.TipStatus)
            {
                pstPipelineObj->st_xraw_tiped.tiped_pos =  pPipelineParam3a->idx2.TipParam.nTipedPosW;
                pstPipelineObj->st_xraw_tiped.en_tiped_status = XSP_TIPED_PROCESSING;
            }
            else if (XRAYLIB_TIP_END ==  pPipelineParam3a->idx2.TipStatus)
            {
                pstPipelineObj->st_xraw_tiped.tiped_pos = -1; /*无效*/
                pstPipelineObj->st_xraw_tiped.en_tiped_status = XSP_TIPED_SUCESS;
                XSP_LOGI("tip: en_tiped_status %d, 0x%x!\n",  pstPipelineObj->st_xraw_tiped.en_tiped_status, pPipelineParam3a->idx2.TipStatus);
            }
            else if (XRAYLIB_TIP_NONE ==  pPipelineParam3a->idx2.TipStatus)
            {
                pstPipelineObj->st_xraw_tiped.tiped_pos = -1; /*无效*/
                pstPipelineObj->st_xraw_tiped.en_tiped_status = XSP_TIPED_NONE;
            }
            else
            {
                pstPipelineObj->st_xraw_tiped.tiped_pos = -1; /*无效*/
                pstPipelineObj->st_xraw_tiped.en_tiped_status = XSP_TIPED_FAIL;
                XSP_LOGW("tip: en_tiped_status %d, 0x%x!\n",  pstPipelineObj->st_xraw_tiped.en_tiped_status, pPipelineParam3a->idx2.TipStatus);
            }
        }
        if (sRet == XRAY_LIB_OK)
        {
            pstPipelineObj->s32CurPipeSeg++;
            SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            if (pPipelineParam3a->idx2.out_calilhz.width > 0 && pPipelineParam3a->idx2.out_calilhz.height > 0)
            {
                rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_SOK);
                if (pstPipelineObj->stDebug.bDumpEnable)
                {
                    snprintf(dumpName, sizeof(dumpName), "%s/p3-out_ch%u_n%u_t%llu_w%u_h%u_s%u.calilhz", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam3a->idx2.out_calilhz.width, pPipelineParam3a->idx2.out_calilhz.height, 
                             pPipelineParam3a->idx2.out_calilhz.stride[0]);

                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam3a->idx2.out_calilhz.pData[0],
                                    XRAW_LE_SIZE(pPipelineParam3a->idx2.out_calilhz.width, pPipelineParam3a->idx2.out_calilhz.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx2.out_calilhz.pData[1],
                                    XRAW_HE_SIZE(pPipelineParam3a->idx2.out_calilhz.width, pPipelineParam3a->idx2.out_calilhz.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx2.out_calilhz.pData[2],
                                    XRAW_Z_SIZE(pPipelineParam3a->idx2.out_calilhz.width, pPipelineParam3a->idx2.out_calilhz.height));

                    snprintf(dumpName, sizeof(dumpName), "%s/p3-out-ori_ch%u_n%u_t%llu_w%u_h%u_s%u.calilhz", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam3a->idx2.out_calilhz_ori.width, pPipelineParam3a->idx2.out_calilhz_ori.height, 
                             pPipelineParam3a->idx2.out_calilhz_ori.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam3a->idx2.out_calilhz_ori.pData[0],
                                    XRAW_LE_SIZE(pPipelineParam3a->idx2.out_calilhz_ori.width, pPipelineParam3a->idx2.out_calilhz_ori.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx2.out_calilhz_ori.pData[1],
                                    XRAW_HE_SIZE(pPipelineParam3a->idx2.out_calilhz_ori.width, pPipelineParam3a->idx2.out_calilhz_ori.height));
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx2.out_calilhz_ori.pData[2],
                                    XRAW_Z_SIZE(pPipelineParam3a->idx2.out_calilhz_ori.width, pPipelineParam3a->idx2.out_calilhz_ori.height));

                    snprintf(dumpName, sizeof(dumpName), "%s/p3-out_ch%u_n%u_t%llu_w%u_h%u_s%u.merge", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam3a->idx2.out_merge.width, pPipelineParam3a->idx2.out_merge.height, 
                             pPipelineParam3a->idx2.out_merge.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam3a->idx2.out_merge.pData[0],
                                    pPipelineParam3a->idx2.out_merge.width * pPipelineParam3a->idx2.out_merge.height * sizeof(UINT32));
                    snprintf(dumpName, sizeof(dumpName), "%s/p3-out_ch%u_n%u_t%llu_w%u_h%u_s%u.argb", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam3a->idx2.out_disp.width, pPipelineParam3a->idx2.out_disp.height, 
                             pPipelineParam3a->idx2.out_disp.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam3a->idx2.out_disp.pData[0],
                                    pPipelineParam3a->idx2.out_disp.height * pPipelineParam3a->idx2.out_disp.stride[0] * sizeof(UINT32));
                    snprintf(dumpName, sizeof(dumpName), "%s/p3-out_ch%u_n%u_t%llu_w%u_h%u_s%u-%u.nv21", 
                             pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                             pPipelineParam3a->idx2.out_ai.width, pPipelineParam3a->idx2.out_ai.height, 
                             pPipelineParam3a->idx2.out_ai.stride[0], pPipelineParam3a->idx2.out_ai.stride[1]);
                    SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam3a->idx2.out_ai.pData[0],
                                    pPipelineParam3a->idx2.out_ai.height * pPipelineParam3a->idx2.out_ai.stride[0]);
                    SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx2.out_ai.pData[1],
                                    pPipelineParam3a->idx2.out_ai.height * pPipelineParam3a->idx2.out_ai.stride[0] / 2);

                    if (XRAYLIB_TIP_NONE != pPipelineParam3a->idx2.TipStatus)
                    {
                        snprintf(dumpName, sizeof(dumpName), "%s/p3-out_tip_ch%u_n%u_t%llu_w%u_h%u_s%u.calilhz", 
                                pstPipelineObj->stDebug.chDumpDir, u32HandleIdx, pstPipelineObj->stDebug.u32SliceNo, sal_get_tickcnt(),
                                pPipelineParam3a->idx2.out_calilhz.width, pPipelineParam3a->idx2.out_calilhz.height, 
                                pPipelineParam3a->idx2.out_calilhz.stride[0]);
                        SAL_WriteToFile(dumpName, 0, SEEK_SET, pPipelineParam3a->idx2.TipParam.pTipedCaliLowData, 
                                        XRAW_LE_SIZE(pPipelineParam3a->idx2.out_calilhz.stride[0], pPipelineParam3a->idx2.out_calilhz.height));
                        SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx2.TipParam.pTipedCaliHighData, 
                                        XRAW_HE_SIZE(pPipelineParam3a->idx2.out_calilhz.stride[1], pPipelineParam3a->idx2.out_calilhz.height));
                        SAL_WriteToFile(dumpName, 0, SEEK_END, pPipelineParam3a->idx2.TipParam.pTipedZData, 
                                        XRAW_Z_SIZE(pPipelineParam3a->idx2.out_calilhz.stride[2], pPipelineParam3a->idx2.out_calilhz.height));
                    }
                }
            }
            else
            {
                rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_FAIL);
                XSP_LOGW("chan %u, libXRay_PipelineProcess failed, out width(%u) & height(%u)\n", u32HandleIdx, 
                         pPipelineParam3a->idx2.out_calilhz.width, pPipelineParam3a->idx2.out_calilhz.height);
            }
            DSA_LstDelete(pLstPipeline, pNode);
            SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pLstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
        else
        {
            XSP_LOGW("chan %u, libXRay_PipelineProcess failed, sRet: 0x%x\n", u32HandleIdx, sRet);
            SAL_mutexTmLock(&pLstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
            rin_native_rt_pipeline_out(u32HandleIdx, pstPipelineObj, SAL_FAIL);
            DSA_LstDelete(pLstPipeline, pNode);
            SAL_mutexTmUnlock(&pLstPipeline->sync.mid, __FUNCTION__, __LINE__);
            SAL_CondSignal(&pLstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
        }
    }

    return NULL;
}


/**
 * @fn      rin_natvie_rt_pipeline_init
 * @brief   实时预览流水线处理初始化
 * 
 * @param   [IN] u32HandleIdx 算法句柄索引，范围[0, XSP_HANDLE_NUM_MAX-1]
 * @param   [IN] pstDevAttr 算法需要的设备属性
 * @param   [IN] enPipelineMode 流水线模式
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，其他：初始化失败
 */
static SAL_STATUS rin_natvie_rt_pipeline_init(UINT32 u32HandleIdx, XRAY_DEV_ATTR *pstDevAttr, XRAYLIB_PIPELINE_MODE enPipelineMode)
{
    UINT32 i = 0, u32BufHeight = 0;
    UINT32 u32StackSize = 1024 * 1024; /* 线程栈，1MB */
    SAL_STATUS sRet = SAL_FAIL;
    DSP_IMG_DATFMT enFmt = DSP_IMG_DATFMT_UNKNOWN;
    XIMAGE_DATA_ST stTmpImage = {0};
    DSA_NODE *pNode = NULL;
    SAL_ThrHndl stThrHndl = {0};
    XRAYLIB_PIPELINE_PARAM_MODE3A *pPipelineParam3a = NULL;
    // XRAYLIB_PIPELINE_PARAM_MODE3B *pPipelineParam3b = NULL;
    XRAYLIB_PIPELINE_PARAM_MODE2A *pPipelineParam2a = NULL;
    CAPB_XRAY_IN *pstCapbXrayin = capb_get_xrayin();

    if (NULL == pstDevAttr->rtproc_pipeline_cb)
    {
        XSP_LOGE("chan %u, the rtproc_pipeline_cb is NULL\n", u32HandleIdx);
        return SAL_FAIL;
    }

    g_rt_pipeline[u32HandleIdx].enPipelineMode = enPipelineMode;
    g_rt_pipeline[u32HandleIdx].lstPipeline = DSA_LstInit(RIN_NATIVE_PIPELINE_OBJ_NUM_MAX);
    if (NULL == g_rt_pipeline[u32HandleIdx].lstPipeline)
    {
        XSP_LOGE("chan %u, init 'lstPipeline' failed\n", u32HandleIdx);
        return SAL_FAIL;
    }

    SAL_mutexTmLock(&g_rt_pipeline[u32HandleIdx].lstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__);
    for (i = 0; i < RIN_NATIVE_PIPELINE_OBJ_NUM_MAX; i++)
    {
        if (NULL != (pNode = DSA_LstGetIdleNode(g_rt_pipeline[u32HandleIdx].lstPipeline)))
        {
            pNode->pAdData = g_rt_pipeline[u32HandleIdx].stPipelineObj + i;
            DSA_LstPush(g_rt_pipeline[u32HandleIdx].lstPipeline, pNode);
        }
    }

    for (i = 0; i < RIN_NATIVE_PIPELINE_OBJ_NUM_MAX; i++)
    {
        DSA_LstPop(g_rt_pipeline[u32HandleIdx].lstPipeline);
    }
    SAL_mutexTmUnlock(&g_rt_pipeline[u32HandleIdx].lstPipeline->sync.mid, __FUNCTION__, __LINE__);

    g_rt_pipeline[u32HandleIdx].rtproc_pipeline_cb = pstDevAttr->rtproc_pipeline_cb;
    g_rt_pipeline[u32HandleIdx].u32PassthDataSize = pstDevAttr->passth_data_size;
    if (pstDevAttr->passth_data_size > 0)
    {
        for (i = 0; i < RIN_NATIVE_PIPELINE_OBJ_NUM_MAX; i++)
        {
            g_rt_pipeline[u32HandleIdx].stPipelineObj[i].pPassthData = (void *)SAL_memMalloc(pstDevAttr->passth_data_size, "xsp", "handle");
            if (NULL == g_rt_pipeline[u32HandleIdx].stPipelineObj[i].pPassthData)
            {
                XSP_LOGE("SAL_memMalloc for 'pPassthData' failed, size: %u\n", pstDevAttr->passth_data_size);
                return SAL_FAIL;
            }
        }
    }

    u32BufHeight = pstDevAttr->xray_slice_height_max + XRAY_LIB_MAX_FILTER_KERNEL_LENGTH * 2;

    switch (enPipelineMode)
    {
    case XRAYLIB_PIPELINE_MODE2A:
        for (i = 0; i < RIN_NATIVE_PIPELINE_OBJ_NUM_MAX; i++)
        {
            g_rt_pipeline[u32HandleIdx].stPipelineObj[i].pPipelineParam = (void *)SAL_memMalloc(sizeof(XRAYLIB_PIPELINE_PARAM_MODE2A), "xsp", "handle");
            if (NULL == g_rt_pipeline[u32HandleIdx].stPipelineObj[i].pPipelineParam)
            {
                XSP_LOGE("SAL_memMalloc for 'pPipelineParam' failed, size: %lu\n", sizeof(XRAYLIB_PIPELINE_PARAM_MODE2A));
                return SAL_FAIL;
            }

            pPipelineParam2a = (XRAYLIB_PIPELINE_PARAM_MODE2A *)g_rt_pipeline[u32HandleIdx].stPipelineObj[i].pPipelineParam;

            /************************ idx0 out ************************/
            enFmt = (XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num) ? DSP_IMG_DATFMT_LHZP : DSP_IMG_DATFMT_SP16;
            stTmpImage.stMbAttr.bCached = SAL_TRUE;
            if (SAL_SOK != ximg_create(pstDevAttr->xray_width_max, u32BufHeight, enFmt, "xsp-idx0-out_calilhz", NULL, &stTmpImage))
            {
                XSP_LOGE("ximg_create for 'idx0.out_calilhz' failed, w:%u, h:%u, fmt:0x%x\n", pstDevAttr->xray_width_max, u32BufHeight, enFmt);
                return SAL_FAIL;
            }
            rin_native_2xraylibimg(&stTmpImage, &pPipelineParam2a->idx0.out_calilhz, __FUNCTION__, __LINE__);

            memset(&stTmpImage, 0, sizeof(stTmpImage));
            enFmt = (XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num) ? DSP_IMG_DATFMT_LHZ16P : DSP_IMG_DATFMT_SP16;
            stTmpImage.stMbAttr.bCached = SAL_TRUE;
            if (SAL_SOK != ximg_create(pstDevAttr->xray_width_max, u32BufHeight, enFmt, "xsp-idx0-out_calilhz_ori", NULL, &stTmpImage))
            {
                XSP_LOGE("ximg_create for 'idx0.out_calilhz_ori' failed, w:%u, h:%u, fmt:0x%x\n", pstDevAttr->xray_width_max, u32BufHeight, enFmt);
                return SAL_FAIL;
            }
            rin_native_2xraylibimg(&stTmpImage, &pPipelineParam2a->idx0.out_calilhz_ori, __FUNCTION__, __LINE__);

            /************************ idx1 in ************************/
            memcpy(&pPipelineParam2a->idx1.in_calilhz, &pPipelineParam2a->idx0.out_calilhz, sizeof(XRAYLIB_IMAGE));
        }

        sRet = SAL_thrCreate(&stThrHndl, rin_native_rt_pipeline_mode2a_0, SAL_THR_PRI_DEFAULT, u32StackSize, g_rt_pipeline + u32HandleIdx);
        if (SAL_SOK != sRet)
        {
            XSP_LOGE("chan %u, SAL_thrCreate 'rin_native_rt_pipeline_mode2a_0' failed\n", u32HandleIdx);
            return SAL_FAIL;
        }

        sRet = SAL_thrCreate(&stThrHndl, rin_native_rt_pipeline_mode2a_1, SAL_THR_PRI_DEFAULT, u32StackSize, g_rt_pipeline + u32HandleIdx);
        if (SAL_SOK != sRet)
        {
            XSP_LOGE("chan %u, SAL_thrCreate 'rin_native_rt_pipeline_mode2a_1' failed\n", u32HandleIdx);
            return SAL_FAIL;
        }

        break;

    /* 
      TODO：目前3A和3B使用的参数结构体相同，故可以均使用3A的逻辑，后续若算法接口稳定了可以将MODE3A/B/C统一相同的参数类型 
     */
    case XRAYLIB_PIPELINE_MODE3A:
    case XRAYLIB_PIPELINE_MODE3B:
        for (i = 0; i < RIN_NATIVE_PIPELINE_OBJ_NUM_MAX; i++)
        {
            g_rt_pipeline[u32HandleIdx].stPipelineObj[i].pPipelineParam = (void *)SAL_memMalloc(sizeof(XRAYLIB_PIPELINE_PARAM_MODE3A), "xsp", "handle");
            if (NULL == g_rt_pipeline[u32HandleIdx].stPipelineObj[i].pPipelineParam)
            {
                XSP_LOGE("SAL_memMalloc for 'pPipelineParam' failed, size: %lu\n", sizeof(XRAYLIB_PIPELINE_PARAM_MODE3A));
                return SAL_FAIL;
            }

            pPipelineParam3a = (XRAYLIB_PIPELINE_PARAM_MODE3A *)g_rt_pipeline[u32HandleIdx].stPipelineObj[i].pPipelineParam;
            
            /************************ idx0 out ************************/
            enFmt = (XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num) ? DSP_IMG_DATFMT_LHZP : DSP_IMG_DATFMT_SP16;
            stTmpImage.stMbAttr.bCached = SAL_TRUE;
            if (SAL_SOK != ximg_create(pstDevAttr->xray_width_max * XRAY_LIB_MAX_RESIZE_FACTOR, 
                                       u32BufHeight * XRAY_LIB_MAX_RESIZE_FACTOR, 
                                       enFmt, "xsp-idx0-out_calilhz", NULL, &stTmpImage))
            {
                XSP_LOGE("ximg_create for 'idx0.out_calilhz' failed, w:%u, h:%u, fmt:0x%x\n", 
                         pstDevAttr->xray_width_max  * XRAY_LIB_MAX_RESIZE_FACTOR, u32BufHeight * XRAY_LIB_MAX_RESIZE_FACTOR, enFmt);
                return SAL_FAIL;
            }
            rin_native_2xraylibimg(&stTmpImage, &pPipelineParam3a->idx0.out_calilhz, __FUNCTION__, __LINE__);

            memset(&stTmpImage, 0, sizeof(stTmpImage));
            enFmt = (XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num) ? DSP_IMG_DATFMT_LHZ16P : DSP_IMG_DATFMT_SP16;
            stTmpImage.stMbAttr.bCached = SAL_TRUE;
            if (SAL_SOK != ximg_create(pstDevAttr->xray_width_max, u32BufHeight, enFmt, "xsp-idx0-out_calilhz_ori", NULL, &stTmpImage))
            {
                XSP_LOGE("ximg_create for 'idx0.out_calilhz_ori' failed, w:%u, h:%u, fmt:0x%x\n", pstDevAttr->xray_width_max, u32BufHeight, enFmt);
                return SAL_FAIL;
            }
            rin_native_2xraylibimg(&stTmpImage, &pPipelineParam3a->idx0.out_calilhz_ori, __FUNCTION__, __LINE__);

            /************************ idx1 in/out ************************/
            memcpy(&pPipelineParam3a->idx1.in_calilhz, &pPipelineParam3a->idx0.out_calilhz, sizeof(XRAYLIB_IMAGE));

            memset(&stTmpImage, 0, sizeof(stTmpImage));
            enFmt = (XRAY_ENERGY_DUAL == pstCapbXrayin->xray_energy_num) ? DSP_IMG_DATFMT_LHZP : DSP_IMG_DATFMT_SP16;
            stTmpImage.stMbAttr.bCached = SAL_TRUE;
            if (SAL_SOK != ximg_create(pstDevAttr->xray_width_max * XRAY_LIB_MAX_RESIZE_FACTOR, // 由于算法超分系数仅影响最后一级流水输出，中间ai层仍然是2倍输出
                                       u32BufHeight * XRAY_LIB_MAX_RESIZE_FACTOR, 
                                       enFmt, "xsp-idx1-out_calilhz", NULL, &stTmpImage))
            {
                XSP_LOGE("ximg_create for 'idx1.out_calilhz' failed, w:%u, h:%u, fmt:0x%x\n", 
                         pstDevAttr->xray_width_max * XRAY_LIB_MAX_RESIZE_FACTOR, u32BufHeight * XRAY_LIB_MAX_RESIZE_FACTOR, enFmt);
                return SAL_FAIL;
            }
            rin_native_2xraylibimg(&stTmpImage, &pPipelineParam3a->idx1.out_calilhz, __FUNCTION__, __LINE__);

            /************************ idx2 in ************************/
            memcpy(&pPipelineParam3a->idx2.in_calilhz, &pPipelineParam3a->idx1.out_calilhz, sizeof(XRAYLIB_IMAGE));
            memcpy(&pPipelineParam3a->idx2.in_calilhz_ori, &pPipelineParam3a->idx0.out_calilhz_ori, sizeof(XRAYLIB_IMAGE));
        }

        sRet = SAL_thrCreate(&stThrHndl, rin_native_rt_pipeline_mode3ab_0, SAL_THR_PRI_DEFAULT, u32StackSize, g_rt_pipeline + u32HandleIdx);
        if (SAL_SOK != sRet)
        {
            XSP_LOGE("chan %u, SAL_thrCreate 'rin_native_rt_pipeline_mode3ab_0' failed\n", u32HandleIdx);
            return SAL_FAIL;
        }

        sRet = SAL_thrCreate(&stThrHndl, rin_native_rt_pipeline_mode3ab_1, SAL_THR_PRI_DEFAULT, u32StackSize, g_rt_pipeline + u32HandleIdx);
        if (SAL_SOK != sRet)
        {
            XSP_LOGE("chan %u, SAL_thrCreate 'rin_native_rt_pipeline_mode3ab_1' failed\n", u32HandleIdx);
            return SAL_FAIL;
        }

        sRet = SAL_thrCreate(&stThrHndl, rin_native_rt_pipeline_mode3ab_2, SAL_THR_PRI_DEFAULT, u32StackSize, g_rt_pipeline + u32HandleIdx);
        if (SAL_SOK != sRet)
        {
            XSP_LOGE("chan %u, SAL_thrCreate 'rin_native_rt_pipeline_mode3ab_2' failed\n", u32HandleIdx);
            return SAL_FAIL;
        }
        break;

    default:
        return SAL_FAIL;
    }

    g_rt_pipeline[u32HandleIdx].pBgacHandle = xray_bgac_init();
    if (NULL == g_rt_pipeline[u32HandleIdx].pBgacHandle)
    {
        XSP_LOGE("chan %u, xray_preproc_init failed\n", u32HandleIdx);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

static SAL_STATUS rin_native_set_default_params(void *p_handle, XSP_HANDLE_TYPE handle_type, XRAY_DEV_ATTR *p_dev_attr)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_ENERGY_MODE energy_mode = XRAYLIB_ENERGY_DUAL;

    XRAYLIB_CONFIG_IMAGE st_config_img = {0};
    XRAYLIB_CONFIG_PARAM st_config_param = {0};
    CAPB_XSP *pstCapbXsp = capb_get_xsp();

    if (XSP_HANDLE_RT_PB == handle_type)
    {
        /* 设置输出的缩放系数，对NRAW、YUV、RGB、融合灰度图均起效 */
        /* 设置宽的缩放系数 */
        st_config_param.key = XRAYLIB_PARAM_WIDTH_RESIZE_FACTOR;
        st_config_param.denominatorValue = 100;
        st_config_param.numeratorValue = (int)(pstCapbXsp->resize_width_factor * st_config_param.denominatorValue);
        hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGE("libXRay_SetConfig 'XRAYLIB_PARAM_WIDTH_RESIZE_FACTOR' failed, numerator: %d, denominator: %d, hr: 0x%x\n",
                     st_config_param.numeratorValue, st_config_param.denominatorValue, hr);
            return SAL_FAIL;
        }

        /* 设置高的缩放系数 */
        st_config_param.key = XRAYLIB_PARAM_HEIGHT_RESIZE_FACTOR;
        st_config_param.denominatorValue = 100;
        st_config_param.numeratorValue = (int)(pstCapbXsp->resize_height_factor * st_config_param.denominatorValue);
        hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGE("libXRay_SetConfig 'XRAYLIB_PARAM_HEIGHT_RESIZE_FACTOR' failed, numerator: %d, denominator: %d, hr: 0x%x\n",
                     st_config_param.numeratorValue, st_config_param.denominatorValue, hr);
            return SAL_FAIL;
        }
    }

    /* 配置能量模式 */
    switch (p_dev_attr->xray_energy_num)
    {
    case XRAY_ENERGY_SINGLE:
        energy_mode = XRAYLIB_ENERGY_LOW;
        break;

    case XRAY_ENERGY_DUAL:
        energy_mode = XRAYLIB_ENERGY_DUAL;
        break;

    default:
        XSP_LOGE("xray_energy_num is error with %d.\n", p_dev_attr->xray_energy_num);
        return SAL_FAIL;
    }
    st_config_img.key = XRAYLIB_ENERGY;
    st_config_img.value = &energy_mode;
    hr = libXRay_SetConfig(p_handle, &st_config_img, XRAYLIB_SETCONFIG_IMAGEORDER);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    /* 剂量波动修正 */
    st_config_param.key = XRAYLIB_PARAM_XRAYDOSE_CALI;
    st_config_param.denominatorValue = 1;
    if (XSP_VANGLE_PRIMARY == p_dev_attr->visual_angle)
    {
        /* 主视角有传送带，开启剂量波动修正*/
        st_config_param.numeratorValue = 3;
    }
    else /* 辅视角 */
    {
        st_config_param.numeratorValue = 0;
    }
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig 'XRAYLIB_PARAM_XRAYDOSE_CALI' failed, value: %d, hr: 0x%x\n",
                 st_config_param.numeratorValue, hr);
        return SAL_FAIL;
    }

    /* 平铺校正 */
    st_config_param.key = XRAYLIB_PARAM_FLATDETMETRIC_CALI;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = (ISM_SC4233XX == p_dev_attr->ism_dev_type) ? XRAYLIB_ON : XRAYLIB_OFF; // 仅4233支持平铺校正
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig 'XRAYLIB_PARAM_FLATDETMETRIC_CALI' failed, value: %d, hr: 0x%x\n",
                 st_config_param.numeratorValue, hr);
        return SAL_FAIL;
    }

    /* 降噪模式 */
    st_config_param.key = XRAYLIB_PARAM_DENOISING_MODE;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = (ISM_SC140100XX == p_dev_attr->ism_dev_type) ? XRAYLIB_DENOISING_MODE1 : XRAYLIB_DENOISING_MODE0;
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig 'XRAYLIB_PARAM_DENOISING_MODE' failed, value: %d, hr: 0x%x\n",
                 st_config_param.numeratorValue, hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      rin_native_lib_init
 * @brief   自研算法初始化
 * 
 * @param   [IN] handle_type 句柄类型，实时回拉、转存
 * @param   [IN] p_dev_attr XRay探测器属性
 *  
 * @warning 输入的XRay探测器属性中的宽高均是原始的，不受是否超分影响 
 *  
 * @return  void* 算法句柄
 */
void *rin_native_lib_init(XSP_HANDLE_TYPE handle_type, XRAY_DEV_ATTR *p_dev_attr)
{
    void *p_handle = NULL;
    UINT32 i = 0, idx = 0;
    float fSize = 0;
    XRAY_DEVICEINFO stXrayDevInfo = {0};
    XRAYLIB_PIPELINE_MODE enPipelineMode = XRAYLIB_PIPELINE_MODE3A;
    XRAY_LIB_MEM_TAB mem_tab[XRAY_LIB_MTAB_NUM] = {0};
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_XRY_VERSION stXspVer = {0};
    XRAYLIB_AI_PARAM stAIParam = {0};
    DSPINITPARA *pstDspInitPrm = SystemPrm_getDspInitPara();
    CHAR *p_const_file_path = NULL;
    CHAR *p_change_file_path = NULL;

    /* 查找空闲的handle */
    for (i = 0; i < XSP_HANDLE_NUM_MAX; i++)
    {
        if (NULL == g_handle_attr[i].p_handle)
        {
            idx = i;
            break;
        }
    }

    if (XSP_HANDLE_NUM_MAX == i)
    {
        XSP_LOGE("there's no idle handle exist!!\n");
        return NULL;
    }

    if (NULL == p_dev_attr)
    {
        XSP_LOGE("p_dev_attr is null!\n");
        return NULL;
    }

    /* 图像宽高及视角信息 */
    p_const_file_path = "./xsp/rin/libXRayPublicParam";
    if (XSP_HANDLE_RT_PB == handle_type)
    {
        if (XSP_VANGLE_PRIMARY == p_dev_attr->visual_angle)
        {
            p_change_file_path = "./xsp/rin/ParaFile0";
        }
        else if (XSP_VANGLE_SECONDARY == p_dev_attr->visual_angle)
        {
            p_change_file_path = "./xsp/rin/ParaFile1";
        }
        else
        {
            XSP_LOGE("the 'visual_angle' is invalid: %d\n", p_dev_attr->visual_angle);
            return NULL;
        }
    }
    else if (XSP_HANDLE_TRANS == handle_type) /* 转存不区分视角，均按主视角处理 */
    {
        p_change_file_path = "./xsp/rin/ParaFileTrans0";
    }

    /* 获取算法版本*/
    stXspVer = libXRay_GetVersion();
    XSP_LOGI("XSP native version: V%d.%d.%d, Build %d-%d-%d\n", stXspVer.major, stXspVer.minor,
             stXspVer.revision, stXspVer.year, stXspVer.month, stXspVer.day);

    /* 计算算法库所需内存大小 */
    hr = libXRay_GetMemSize(p_dev_attr->xray_width_max, p_dev_attr->xray_slice_height_max, p_dev_attr->xray_slice_resize, mem_tab);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_GetMemSize err, hr=0x%x.\n", hr);
        goto EXIT;
    }

    /* 分配内存 */
    for (i = 0; i < XRAY_LIB_MTAB_NUM; i++)
    {
        XSP_LOGW("i_%d, mem size %lu \n", i, mem_tab[i].size);
        mem_tab[i].base = (void *)SAL_memMalloc(mem_tab[i].size, "xsp", "algoHandle");
        if (NULL == mem_tab[i].base)
        {
            XSP_LOGE("Xsp_halMalloc, size:%lu.\n", mem_tab[i].size);
            goto EXIT;
        }

        fSize += mem_tab[i].size;
    }

    fSize /= (1024.0f * 1024.0f);

    /* 流水线选择 */
    if (XSP_LIB_RAYIN == p_dev_attr->enXspLibSrc)
    {
        enPipelineMode = XRAYLIB_PIPELINE_MODE2A;
    }
    else
    {
        enPipelineMode = XRAYLIB_PIPELINE_MODE3A;
    }

    /* 设备类型 */
    stXrayDevInfo.xraylib_devicetype = p_dev_attr->ism_dev_type;
    
    /* X-Ray探测板供应商或传输板*/
    stXrayDevInfo.xraylib_detectortype = p_dev_attr->xray_det_vendor;
    /*暂不支持的探测板型号*/
    if(p_dev_attr->xray_det_vendor == XRAYDV_HAMAMATSU||p_dev_attr->xray_det_vendor == XRAYDV_TYW||p_dev_attr->xray_det_vendor == XRAYDV_VIDETECT)
    {
        XSP_LOGE("xray_det_vendor is error with %d.\n", p_dev_attr->xray_det_vendor);
        return NULL;
    }
    
    /*射线源型号*/
    stXrayDevInfo.xraylib_sourcetype = p_dev_attr->enXraySource;

    /*指定图像能量模式：高能、低能、双能*/
    switch(p_dev_attr->xray_energy_num)
    {
        case XRAY_ENERGY_SINGLE:
            stXrayDevInfo.xraylib_energymode = XRAYLIB_ENERGY_LOW;
            break;
        case XRAY_ENERGY_DUAL:
            stXrayDevInfo.xraylib_energymode = XRAYLIB_ENERGY_DUAL;
            break;
        default:
            XSP_LOGE("xray_energy_num is error with %d.\n", p_dev_attr->xray_energy_num);
            return NULL;
    }

    XSP_LOGW("Rayin XSP: device_type 0x%x/0x%x, xrayType 0x%x detect %d, width %d, height %d, slice_height_max%d, resize_factor %f, energy num %d, chan cnt %d, visual_angle %d,  mem %.2fMB\n",
             stXrayDevInfo.xraylib_devicetype, p_dev_attr->ism_dev_type, p_dev_attr->enXraySource, p_dev_attr->xray_det_vendor, p_dev_attr->xray_width_max,
             p_dev_attr->xray_height_max, p_dev_attr->xray_slice_height_max, p_dev_attr->xray_slice_resize, p_dev_attr->xray_energy_num, p_dev_attr->xray_in_chan_cnt, p_dev_attr->visual_angle, fSize);

    /* 视角*/
    if (XSP_HANDLE_RT_PB == handle_type)
    {
        if(1 == p_dev_attr->xray_in_chan_cnt)
        {
            stXrayDevInfo.xraylib_visualNo = XRAYLIB_VISUAL_SINGLE;          //算法需要区分单双视角来进行文件配置
        }
        else
        {
            stXrayDevInfo.xraylib_visualNo = (p_dev_attr->visual_angle == XSP_VANGLE_PRIMARY) ? XRAYLIB_VISUAL_MAIN : XRAYLIB_VISUAL_AUX;
        }
    }
    else if (XSP_HANDLE_TRANS == handle_type) /* 转存不区分视角，均按主视角处理 */
    {
        if (1 == p_dev_attr->xray_in_chan_cnt)
        {
            stXrayDevInfo.xraylib_visualNo = XRAYLIB_VISUAL_SINGLE;
        }
        else
        {
            stXrayDevInfo.xraylib_visualNo = XRAYLIB_VISUAL_MAIN;
        }

    }

    /* AI参数, 仅实时需要，转存关闭AI初始化*/
    stAIParam.nUseAI = SAL_FALSE;
    stAIParam.nChannel = idx;
#if defined X86
    stAIParam.systemPlat = SYS_X86_PLAT;
#elif defined RK3588
    stAIParam.systemPlat = SYS_RK_PLAT;
#elif defined NT98336
    stAIParam.systemPlat = SYS_NT_PLAT;
#endif

    if (XSP_HANDLE_RT_PB == handle_type)
    {
        if (XSP_LIB_RAYIN_AI == p_dev_attr->enXspLibSrc)
        {
#if defined X86
            stAIParam.systemPlat = SYS_X86_PLAT;
            stAIParam.systemProcessType = X86_INTEL_GPU;
            stAIParam.nUseAI = SAL_TRUE;
#elif defined RK3588
            if (XSP_VANGLE_PRIMARY == p_dev_attr->visual_angle)
            {
                stAIParam.nChannel = 0;
                stAIParam.rayinSchedulerParam.aiSrNpuCore = SYSTEM_NPU_CORE_0;
                stAIParam.rayinSchedulerParam.aiSegNpuCore = SYSTEM_NPU_CORE_2;
            }
            else if(XSP_VANGLE_SECONDARY == p_dev_attr->visual_angle)
            {
                stAIParam.nChannel = 1;
                stAIParam.rayinSchedulerParam.aiSrNpuCore = SYSTEM_NPU_CORE_1;
                stAIParam.rayinSchedulerParam.aiSegNpuCore = SYSTEM_NPU_CORE_2;
            }
            stAIParam.systemPlat = SYS_RK_PLAT;
            stAIParam.systemProcessType = ARM_HARD_WARE_CORE;
            stAIParam.systemProcessPrecision = PROCESS_FP16;    // PROCESS_FP32
            stAIParam.plat_mode = 0;                         // 使用自研AI
            stAIParam.nUseAI = SAL_TRUE;
            
            
            if (0 == stAIParam.plat_mode)
            {
                XSP_LOGE("Using Rayin Model.aiSrcNpuCore %d, aiSegNpuCore %d\n\n\n", stAIParam.rayinSchedulerParam.aiSrNpuCore, stAIParam.rayinSchedulerParam.aiSegNpuCore);
            }
#else
            XSP_LOGE("XSP lib src[%d] is only supported for plat x86 and rk!\n", p_dev_attr->enXspLibSrc);
            return NULL;
#endif
        }
        else if (XSP_LIB_RAYIN_AI_EXT == p_dev_attr->enXspLibSrc)
        {
#if defined X86
            stAIParam.systemPlat = SYS_X86_PLAT;
            stAIParam.systemProcessType = X86_NVIDIA_GPU;
            stAIParam.nUseAI = SAL_TRUE;
#else
            XSP_LOGE("XSP lib src[%d] is only supported for plat x86!\n", p_dev_attr->enXspLibSrc);
            return NULL;
#endif
        }
        else if (XSP_LIB_RAYIN_AI_OTH == p_dev_attr->enXspLibSrc)
        {
#if defined  RK3588
            stAIParam.systemPlat = SYS_RK_PLAT;
            stAIParam.systemProcessType = ARM_HARD_WARE_CORE;
            stAIParam.systemProcessPrecision = PROCESS_FP16;    // PROCESS_FP32
            stAIParam.nChannel = 0;
            stAIParam.plat_mode = 1;                         // 使用研究院AI
            stAIParam.scheduler_param.dev_num = 3;                                 // RK3588使用以下配置
            stAIParam.scheduler_param.dev_data[0].dev_type = DEV_ID0;    // 使用第一颗硬核
            stAIParam.scheduler_param.dev_data[0].dev_id = idx % 2;
            stAIParam.scheduler_param.dev_data[1].dev_type = DEV_ID1;    // 使用一颗ARM
            stAIParam.scheduler_param.dev_data[1].dev_id = 0;
            stAIParam.scheduler_param.dev_data[2].dev_type = DEV_ID4;    // 使用一颗IPU，yuv输入
            stAIParam.scheduler_param.dev_data[2].dev_id = 0;
            stAIParam.nUseAI = SAL_TRUE;
#else
            XSP_LOGE("XSP lib src[%d] is only supported for plat rk!\n", p_dev_attr->enXspLibSrc);
            return NULL;
#endif
        }
        else
        {
            stAIParam.nUseAI = SAL_FALSE;
        }
    }    

    /* 创建句柄 */
    hr = libXRay_Create(p_dev_attr->xray_width_max, p_dev_attr->xray_slice_height_max, p_dev_attr->xray_slice_resize, \
                    stAIParam, p_const_file_path, p_change_file_path, mem_tab, &p_handle, enPipelineMode, &stXrayDevInfo);
    /* 返回给应用算法初始化状态 */
    pstDspInitPrm->stSysStatus.bXspInitAbnormally = (hr == XRAY_LIB_OK) ? SAL_FALSE : SAL_TRUE;
    pstDspInitPrm->stSysStatus.u32XspErrno = hr;
    if (hr != XRAY_LIB_OK || NULL == p_handle)
    {
        XSP_LOGE("libXRay_Create failed, hr: 0x%x, p_handle: %p\n", hr, p_handle);
        p_handle = NULL;
        goto EXIT;
    }

    /* 流水线初始化，仅实时和回拉需要，转存无需 */
    if (XSP_HANDLE_RT_PB == handle_type)
    {
        if (SAL_SOK != rin_natvie_rt_pipeline_init(idx, p_dev_attr, enPipelineMode))
        {
            XSP_LOGE("chan %u, rin_natvie_rt_pipeline_init failed, mode: 0x%x\n", idx, enPipelineMode);
            p_handle = NULL;
            goto EXIT;
        }
    }

    g_handle_attr[idx].width_max = p_dev_attr->xray_width_max;
    g_handle_attr[idx].height_max = p_dev_attr->xray_height_max;
    g_handle_attr[idx].p_handle = p_handle;
    g_handle_attr[idx].handle_type = handle_type;

    /* 算法默认参数配置*/
    if (SAL_SOK != rin_native_set_default_params(p_handle, handle_type, p_dev_attr))
    {
        XSP_LOGE("chan %u, rin_native_set_default_params failed\n", idx);
        p_handle = NULL;
        goto EXIT;
    }

EXIT:
    return p_handle;
}

/******************************************************************
 * @function:   rin_native_lib_reload_zTable
 * @brief:      重新载入Z值表
 * @param[in]:  void *p_handle   算法句柄
 * @param[out]: None
 * @return:     SAL_STATUS 
 *******************************************************************/
SAL_STATUS rin_native_lib_reload_zTable(void *p_handle, XRAY_DEV_ATTR *p_dev_attr)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAY_DEVICEINFO stXrayDevInfo = {0};

    if (NULL == p_dev_attr)
    {
        XSP_LOGE("p_dev_attr is null!\n");
        return SAL_FAIL;
    }

    /* 设备类型 */
    stXrayDevInfo.xraylib_devicetype = p_dev_attr->ism_dev_type;

    /*暂不支持的探测板型号*/
    if(p_dev_attr->xray_det_vendor == XRAYDV_HAMAMATSU||p_dev_attr->xray_det_vendor == XRAYDV_TYW||p_dev_attr->xray_det_vendor == XRAYDV_VIDETECT)
    {
        XSP_LOGE("xray_det_vendor is error with %d.\n", p_dev_attr->xray_det_vendor);
        return SAL_FAIL;
    }
    /* X-Ray探测板供应商*/
    stXrayDevInfo.xraylib_detectortype = p_dev_attr->xray_det_vendor;

    /*射线源型号*/
    stXrayDevInfo.xraylib_sourcetype = p_dev_attr->enXraySource;

    /*指定图像能量模式：高能、低能、双能*/
    switch(p_dev_attr->xray_energy_num)
    {
        case XRAY_ENERGY_SINGLE:
            stXrayDevInfo.xraylib_energymode = XRAYLIB_ENERGY_LOW;
            break;
        case XRAY_ENERGY_DUAL:
            stXrayDevInfo.xraylib_energymode = XRAYLIB_ENERGY_DUAL;
            break;
        default:
            XSP_LOGE("xray_energy_num is error with %d.\n", p_dev_attr->xray_energy_num);
            return SAL_FAIL;
    }

    XSP_LOGW("ReloadProfile: device_type 0x%x/0x%x, xrayType 0x%x detect %d,  energy num %d, chan cnt %d, visual_angle %d\n",
             stXrayDevInfo.xraylib_devicetype, p_dev_attr->ism_dev_type, p_dev_attr->enXraySource, p_dev_attr->xray_det_vendor, p_dev_attr->xray_energy_num, p_dev_attr->xray_in_chan_cnt, p_dev_attr->visual_angle);

    /* 视角*/
    if(1 == p_dev_attr->xray_in_chan_cnt)
    {
        stXrayDevInfo.xraylib_visualNo = XRAYLIB_VISUAL_SINGLE;          //算法需要区分单双视角来进行文件配置
    }
    else
    {
        stXrayDevInfo.xraylib_visualNo = (p_dev_attr->visual_angle == XSP_VANGLE_PRIMARY) ? XRAYLIB_VISUAL_MAIN : XRAYLIB_VISUAL_AUX;
    }

    hr = libXRay_ReloadProfile(p_handle, &stXrayDevInfo);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_ReloadZTable failed, errno: 0x%08x, \n", hr            );
        return SAL_FAIL;
    }
    else
    {
        XSP_LOGW("libXRay_ReloadZTable succeed\n");
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_get_handle_idx
 * @brief:      获取当前通道号
 * @param[in]:  void *p_handle   算法句柄
 * @param[out]: None
 * @return:     S32              通道号
 *******************************************************************/
static S32 rin_native_get_handle_idx(void *p_handle)
{
    S32 i = 0;

    for (i = 0; i < XSP_HANDLE_NUM_MAX; i++)
    {
        if (p_handle == g_handle_attr[i].p_handle)
        {
            break;
        }
    }

    if (i < XSP_HANDLE_NUM_MAX)
    {
        return i;
    }
    else
    {
        return -1;
    }
}

/******************************************************************
 * @function:   rin_native_get_version
 * @brief:      获取算法版本号
 * @param[in]:  void *p_handle   算法句柄
 * @param[out]: None
 * @return:     CHAR *           版本号字符串
 *******************************************************************/
CHAR *rin_native_get_version(void *p_handle)
{
    static CHAR lib_ver_str[128] = {0};
    XRAYLIB_XRY_VERSION stXspVer = {0};

    if (0 == strlen(lib_ver_str))
    {
        stXspVer = libXRay_GetVersion();
        XSP_LOGI("XSP[Ver:%d.%d.%d,Build:%d-%d-%d].\n", stXspVer.major, stXspVer.minor,
                 stXspVer.revision, stXspVer.year, stXspVer.month, stXspVer.day);

        snprintf((char *)lib_ver_str, 128, "XSP[version:%d.%d.%d,build:%d-%d-%d]\n",
                 stXspVer.major, stXspVer.minor, stXspVer.revision, stXspVer.year, stXspVer.month, stXspVer.day);
    }

    return lib_ver_str;
}

/**
 * @function    rin_native_set_akey
 * @brief
 * @param[in]   void *p_handle  算法句柄
 * @param[in]   S32 s32OptNum   命令行参数数量
 * @param[in]   S32 key         算法参数的key
 * @param[in]   S32 value1      写入的参数1（参数改变相关参数的分子）
 * @param[in]   S32 value2      写入的参数2（参数改变相关参数的分母）
 * @param[out]  None
 * @return      成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS rin_native_set_akey(void *p_handle, S32 s32OptNum, S32 key, S32 value1, S32 value2)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};
    XRAYLIB_CONFIG_PARAM st_config_param = {0};
    S32 s32ImageValue = 0;
    XRAYLIB_CONFIG_IMAGE_KEY imageKey = XRAYLIB_CONFIG_IMAGE_KEY_MIN_VALUE;

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle is NULL\n");
        return SAL_FAIL;
    }

    if (4 == s32OptNum)         // 设置图像处理相关对应的参数
    {
        /* 判断key是否符合图像处理参数的范围 */
        imageKey = (XRAYLIB_CONFIG_IMAGE_KEY)key;
        s32ImageValue = value1;
        st_config_image.key = imageKey;
        st_config_image.value = &s32ImageValue;

        XSP_LOGW("libXRay_SetConfig key: 0x%08x, value: %d\n", key, *((S32 *)st_config_image.value));
        hr = libXRay_SetConfig(p_handle, &st_config_image, XRAYLIB_SETCONFIG_IMAGEORDER);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGE("libXRay_SetConfig failed, errno: 0x%08x, key: 0x%08x, value: %d\n", hr,
                     st_config_param.key, *((S32 *)st_config_image.value));
            return SAL_FAIL;
        }
    }
    else if (5 == s32OptNum)    // 设置参数改变相关对应的参数
    {
        st_config_param.key = key;
        st_config_param.denominatorValue = (0 == value2) ? 1 : value2; /* 若分母为0，强制其为1 */
        st_config_param.numeratorValue = value1;

        XSP_LOGW("libXRay_SetConfig key: 0x%08x, numerator: %d, denominator: %d\n", key, value1, value2);
        hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGE("libXRay_SetConfig failed, errno: 0x%08x, key: 0x%08x, numerator: %d, denominator: %d\n", hr,
                     st_config_param.key, st_config_param.numeratorValue, st_config_param.denominatorValue);
            return SAL_FAIL;
        }
    }
    else        // 命令参数无效情况
    {
        XSP_LOGE("Invalid dspdebug parameter.\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    rin_native_get_akey
 * @brief
 * @param[in]   void *p_handle  算法句柄
 * @param[in]   S32 key         算法参数的key
 * @param[out]  S32 *pImageVal  图像处理相关参数的返回值
 * @param[out]  S32 *pVal1      参数改变相关的分子值
 * @param[out]  S32 *pVal2      参数改变相关的分母值
 * @return      成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS rin_native_get_akey(void *p_handle, S32 key, S32 *pImageVal, S32 *pVal1, S32 *pVal2)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};
    XRAYLIB_CONFIG_PARAM st_config_param = {0};
    XRAYLIB_SETCONFIG_CHANNEL setConfigChan = XRAYLIB_SETCONFIG_IMAGEORDER;

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle is NULL\n");
        return SAL_FAIL;
    }

    if (NULL == pVal1)
    {
        XSP_LOGE("the pVal1 is NULL\n");
        return SAL_FAIL;
    }

    if (NULL == pVal2)
    {
        XSP_LOGE("the pVal2 is NULL\n");
        return SAL_FAIL;
    }

    /* 当key键对应有图像处理相关的参数时,获取图像处理相关参数 */
    if (key >= XRAYLIB_CONFIG_IMAGE_KEY_MIN_VALUE && key <= XRAYLIB_CONFIG_IMAGE_KEY_MAX_VALUE)
    {
        setConfigChan = XRAYLIB_SETCONFIG_IMAGEORDER;
        st_config_image.key = key;
        st_config_image.value = pImageVal;

        hr = libXRay_GetConfig(p_handle, &st_config_image, setConfigChan);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGE("libXRay_SetConfig failed, errno: 0x%08x, key: 0x%08x, value: %d\n",
                     hr, st_config_image.key, *((S32 *)st_config_image.value));
            return SAL_FAIL;
        }
    }

    /* 获取改变参数相关标识值 */
    setConfigChan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    st_config_param.key = key;
    hr = libXRay_GetConfig(p_handle, &st_config_param, setConfigChan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig failed, errno: 0x%08x, key: 0x%08x, numerator: %d, denominator: %d\n",
                 hr, st_config_param.key, st_config_param.numeratorValue, st_config_param.denominatorValue);
        return SAL_FAIL;
    }

    *pVal1 = st_config_param.numeratorValue;
    *pVal2 = st_config_param.denominatorValue;

    return SAL_SOK;
}

/**
 * @function    rin_native_get_all_key
 * @brief
 * @param[in]   void *p_handle
 * @param[out]  None
 * @return      成功SAL_SOK，失败SAL_FAIL
 */
SAL_STATUS rin_native_get_all_key(void *p_handle)
{
    struct XRAYLIb_KEY_VALUE
    {
        UINT32 type;
        UINT32 key;
        INT32 value1;
        INT32 value2;
    } stKeyValue[200] = {0};

    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL setConfigChan = XRAYLIB_SETCONFIG_IMAGEORDER;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};
    XRAYLIB_CONFIG_PARAM st_config_param = {0};
    INT32 s32ErrVal = -1;
    UINT32 countKey = 0;
    INT32 val1 = 0;
    UINT32 i = 0;
    CHAR *pVersionStr = NULL;
    
    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle is NULL\n");
        return SAL_FAIL;
    }

    st_config_image.value = &val1;
    /* 获取图像处理标识值*/
    setConfigChan = XRAYLIB_SETCONFIG_IMAGEORDER;
    for (i = XRAYLIB_CONFIG_IMAGE_KEY_MIN_VALUE; i <= XRAYLIB_CONFIG_IMAGE_KEY_MAX_VALUE; i++)
    {
        st_config_image.key = i;
        hr = libXRay_GetConfig(p_handle, &st_config_image, setConfigChan);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGW("libXRay_GetConfig failed, errno: 0x%08x, key: 0x%x, value: %d\n",
                     hr, st_config_image.key, *((S32 *)st_config_image.value));
            st_config_image.value = &s32ErrVal;
        }

        stKeyValue[countKey].type = setConfigChan;
        stKeyValue[countKey].key = st_config_image.key;
        stKeyValue[countKey].value1 = *((INT32 *)st_config_image.value);
        stKeyValue[countKey].value2 = 1;

        countKey++;
        val1 = 0;
    }

    /* 获取改变参数标识值 */
    setConfigChan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    for (i = XRAYLIB_PARAM_WHITENUP; i <= XRAYLIB_PARAM_IMAGEWIDTH; i++)
    {
        st_config_param.key = (XRAYLIB_CONFIG_PARAM_KEY)i;
        hr = libXRay_GetConfig(p_handle, &st_config_param, setConfigChan);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGW("libXRay_GetConfig failed, errno: 0x%08x, key: 0x%x, value1: %d, value2: %d\n",
                    hr, st_config_param.key, st_config_param.numeratorValue, st_config_param.denominatorValue);
            st_config_param.numeratorValue = s32ErrVal;
            st_config_param.denominatorValue = 1;
        }

        stKeyValue[countKey].type = setConfigChan;
        stKeyValue[countKey].key = st_config_param.key;
        stKeyValue[countKey].value1 = st_config_param.numeratorValue;
        stKeyValue[countKey].value2 = st_config_param.denominatorValue;

        countKey++;
    }
    for (i = XRAYLIB_PARAM_DUAL_FILTERKERNEL_LENGTH; i <= XRAYLIB_PARAM_SIGMA_NOISES; i++)
    {
        st_config_param.key = (XRAYLIB_CONFIG_PARAM_KEY)i;
        hr = libXRay_GetConfig(p_handle, &st_config_param, setConfigChan);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGW("libXRay_GetConfig failed, errno: 0x%08x, key: 0x%x, value1: %d, value2: %d\n",
                    hr, st_config_param.key, st_config_param.numeratorValue, st_config_param.denominatorValue);
            st_config_param.numeratorValue = s32ErrVal;
            st_config_param.denominatorValue = 1;
        }

        stKeyValue[countKey].type = setConfigChan;
        stKeyValue[countKey].key = st_config_param.key;
        stKeyValue[countKey].value1 = st_config_param.numeratorValue;
        stKeyValue[countKey].value2 = st_config_param.denominatorValue;

        countKey++;
    }

    for (i = XRAYLIB_PARAM_ROTATE; i <= XRAYLIB_PARAM_SPEEDGEAR; i++)
    {
        st_config_param.key = (XRAYLIB_CONFIG_PARAM_KEY)i;
        hr = libXRay_GetConfig(p_handle, &st_config_param, setConfigChan);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGW("libXRay_GetConfig failed, errno: 0x%08x, key: 0x%x, value1: %d, value2: %d\n",
                    hr, st_config_param.key, st_config_param.numeratorValue, st_config_param.denominatorValue);
            st_config_param.numeratorValue = s32ErrVal;
            st_config_param.denominatorValue = 1;
        }

        stKeyValue[countKey].type = setConfigChan;
        stKeyValue[countKey].key = st_config_param.key;
        stKeyValue[countKey].value1 = st_config_param.numeratorValue;
        stKeyValue[countKey].value2 = st_config_param.denominatorValue;

        countKey++;
    }

    /* 输出所有结果 */
    pVersionStr = rin_native_get_version(p_handle);
    SAL_print("xsp version is : %s\n", pVersionStr);

    if (!(strlen(pVersionStr) > 0))
    {
        XSP_LOGE("Get the XSP lib version error\n");
        return SAL_FAIL;
    }
    SAL_print("type \t key       \t value1 \t value2 \t\n");
    for(int i = 0; i < countKey; i++)
    {
        SAL_print("%-4d \t 0x%08x \t %-8d \t %-8d\n", 
                   stKeyValue[i].type,
                   stKeyValue[i].key, 
                   stKeyValue[i].value1, 
                   stKeyValue[i].value2);
    }

    return SAL_SOK;
}


/**
 * @fn      rin_native_set_blank_region
 * @brief   设置XRAW数据左右置白参数
 * 
 * @param   [IN] p_handle 算法句柄，rin_native_lib_init()返回值
 * @param   [IN] top_margin XRAW数据左置白区域
 * @param   [IN] bottom_margin XRAW数据右置白区域
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS rin_native_set_blank_region(void *p_handle, U32 top_margin, U32 bottom_margin)
{
    INT32 idx = 0;
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    idx = rin_native_get_handle_idx(p_handle);
    if (idx < 0)
    {
        XSP_LOGE("the p_handle[%p] is invalid\n", p_handle);
        return SAL_FAIL;
    }

    XSP_LOGI("chan %d, set blank region: left %u, right %u\n", idx, top_margin, bottom_margin);
    xray_bgac_update_lrblank(g_rt_pipeline[idx].pBgacHandle, top_margin, bottom_margin);

    /*上方置白*/
    config_param.key = XRAYLIB_PARAM_WHITENUP;
    config_param.numeratorValue = top_margin;
    config_param.denominatorValue = 1;
    hr = libXRay_SetConfig(p_handle, &config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (XRAY_LIB_OK != hr)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_WHITENUP failed, errno: 0x%x\n", hr);
        return SAL_FAIL;
    }

    /*下方置白*/
    config_param.key = XRAYLIB_PARAM_WHITENDOWN;
    config_param.numeratorValue = bottom_margin;
    config_param.denominatorValue = 1;
    hr = libXRay_SetConfig(p_handle, &config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (XRAY_LIB_OK != hr)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_WHITENDOWN failed, errno: 0x%x\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/******************************************************************
 * @function:   hka_xsps_get_correction_data
 * @brief:      获取本底、满载等校正数据
 * @param       p_handle[IN]        图像处理通道对应的句柄
 * @param       corr_type[IN]       校正数据类型，参考枚举XSP_CORRECT_TYPE
 * @param       p_corr_data[OUT]    校正数据Buffer，可为NULL，为NULL时仅输出校正数据长度，用于申请Buffer
 * @param       p_corr_size[OUT]    校正数据长度，单位：Byte
 * @param[out]: None
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_get_correction_data(void *p_handle, XSP_CORRECT_TYPE corr_type, U16 *p_corr_data, U32 *p_corr_size)
{
    S32 idx = 0;
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CALI_TABLE st_cali_table = {0};
    CAPB_XRAY_IN *pst_capb_xrayin = NULL;

    if (NULL == p_handle || NULL == p_corr_size)
    {
        XSP_LOGE("the p_handle[%p] OR p_corr_size[%p] is NULL\n", p_handle, p_corr_size);
        return SAL_FAIL;
    }

    pst_capb_xrayin = capb_get_xrayin();
    if (NULL == pst_capb_xrayin)
    {
        XSP_LOGE("p_xray_in is error with cap %p.\n", pst_capb_xrayin);
        return SAL_FAIL;
    }

    idx = rin_native_get_handle_idx(p_handle);
    if (idx < 0)
    {
        XSP_LOGE("the p_handle[%p] is invalid\n", p_handle);
        return SAL_FAIL;
    }

    XSP_LOGT("the corr_type %d, energy num %d\n", corr_type, pst_capb_xrayin->xray_energy_num);
    *p_corr_size = g_handle_attr[idx].width_max * pst_capb_xrayin->xray_energy_num * sizeof(U16); /*两个字节*/
    if (NULL != p_corr_data)
    {
        switch (pst_capb_xrayin->xray_energy_num)
        {
            case XRAY_ENERGY_DUAL:
                st_cali_table.EnergyMode = XRAYLIB_ENERGY_DUAL;
                break;

            case XRAY_ENERGY_SINGLE:
                st_cali_table.EnergyMode = XRAYLIB_ENERGY_LOW;
                break;

            default:
                XSP_LOGE("energy num %d is error!\n", pst_capb_xrayin->xray_energy_num);
                return SAL_FAIL;
        }

        switch (corr_type)
        {
            case XSP_CORR_EMPTYLOAD:
                st_cali_table.UpdateMode = XRAYLIB_UPDATEZERO;
                st_cali_table.pCaliTableAirLow = NULL;
                st_cali_table.pCaliTableAirHigh = NULL;
                if (XRAYLIB_ENERGY_DUAL == st_cali_table.EnergyMode)
                {
                    st_cali_table.pCaliTableBackgroundLow = p_corr_data;
                    st_cali_table.pCaliTableBackgroundHigh = p_corr_data + g_handle_attr[idx].width_max;
                }
                else
                {
                    st_cali_table.pCaliTableBackgroundLow = p_corr_data;
                    st_cali_table.pCaliTableBackgroundHigh = NULL;
                }

                break;

            case XSP_CORR_FULLLOAD:
                st_cali_table.UpdateMode = XRAYLIB_UPDATEFULL;
                st_cali_table.pCaliTableBackgroundLow = NULL;
                st_cali_table.pCaliTableBackgroundHigh = NULL;
                if (XRAYLIB_ENERGY_DUAL == st_cali_table.EnergyMode)
                {
                    st_cali_table.pCaliTableAirLow = p_corr_data;
                    st_cali_table.pCaliTableAirHigh = p_corr_data + g_handle_attr[idx].width_max;
                }
                else
                {
                    st_cali_table.pCaliTableAirLow = p_corr_data;
                    st_cali_table.pCaliTableAirHigh = NULL;
                }

                break;

            default:
                XSP_LOGE("corr_type %d is error\n", corr_type);
                return SAL_FAIL;
        }

        st_cali_table.nCaliTableHeight = 1;
        st_cali_table.nCaliWidth = g_handle_attr[idx].width_max;
        hr = libXRay_GetCaliTable(p_handle, &st_cali_table);
        if (XRAY_LIB_OK != hr)
        {
            XSP_LOGE("libXRay_GetConfig is error with hr = 0x%x!", hr);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_correction_data
 * @brief:      设置矫正数据
 * @param[in]:  void *p_handle      算法句柄
 * @param[in]:  S32 corr_type       通道号
 * @param[in]:  U16 *p_corr_data    矫正数据
 * @param[in]:  U32 corr_raw_width  xraw宽度
 * @param[in]:  U32 corr_raw_height xraw高度
 * @param[out]: None
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_correction_data(void *p_handle, XSP_CORRECT_TYPE corr_type, U16 *p_corr_data, U32 corr_raw_width, U32 corr_raw_height)
{
    INT32 idx = 0;
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_IMAGE stXrayLibImg = {0};
    XRAYLIB_UPDATE_ZEROANDFULL_MODE update_mode = XRAYLIB_UPDATEZERO;
    CAPB_XRAY_IN *pstCapbXray = capb_get_xrayin();
    XIMAGE_DATA_ST stFullCorr = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    idx = rin_native_get_handle_idx(p_handle);
    if (idx < 0)
    {
        XSP_LOGE("the p_handle[%p] is invalid\n", p_handle);
        return SAL_FAIL;
    }

    /* 更新计算 */
    stXrayLibImg.width = corr_raw_width;
    stXrayLibImg.height = corr_raw_height;
    stXrayLibImg.pData[0] = p_corr_data;
    if (XRAY_ENERGY_DUAL == pstCapbXray->xray_energy_num)
    {
        stXrayLibImg.pData[1] = p_corr_data + corr_raw_width * corr_raw_height;
    }

    switch (corr_type)
    {
    case XSP_CORR_EMPTYLOAD:
        update_mode = XRAYLIB_UPDATEZERO;
        break;

    case XSP_CORR_FULLLOAD:
        update_mode = XRAYLIB_UPDATEFULL;
        break;

    case XSP_CORR_AUTOFULL:
        update_mode = XRAYLIB_UPDATEFULL;
        break;

    default:
        XSP_LOGE("unsupport this correction type: %d\n", corr_type);
        return SAL_FAIL;
    }

    if (XSP_CORR_AUTOFULL == corr_type)
    {
        hr = libXRay_UpdateZeroAndFullMem(p_handle, &stXrayLibImg, update_mode);
    }
    else
    {
        hr = libXRay_UpdateZeroAndFull(p_handle, &stXrayLibImg, update_mode);
        if (XSP_CORR_FULLLOAD == corr_type)
        {
            stFullCorr.u32Width = corr_raw_width;
            stFullCorr.u32Height = corr_raw_height;
            stFullCorr.u32Stride[0] = corr_raw_width;
            stFullCorr.pPlaneVir[0] = stXrayLibImg.pData[0];
            if (XRAY_ENERGY_DUAL == pstCapbXray->xray_energy_num)
            {
                stFullCorr.enImgFmt = DSP_IMG_DATFMT_LHP;
                stFullCorr.u32Stride[1] = corr_raw_width;
                stFullCorr.pPlaneVir[1] = stXrayLibImg.pData[1];
            }
            else
            {
                stFullCorr.enImgFmt = DSP_IMG_DATFMT_SP16;
            }
            xray_bgac_update_maunal_fullcorr(g_rt_pipeline[idx].pBgacHandle, &stFullCorr);
        }
    }
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_UpdateZeroAndFull failed, hr: 0x%x, corr_type: %d, update_mode: %d\n", hr, corr_type, update_mode);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_disp_color
 * @brief:      设置显示参数
 * @param[in]:  void *p_handle   算法句柄
 * @param[in]:  U32 type         类型
 * @param[out]: None
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_disp_color(void *p_handle, U32 type)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_COLOR_MODE color_mode = 0;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    /*显示*/
    switch (type)
    {
        case XSP_DISP_COLOR:
            color_mode = XRAYLIB_DUALCOLOR;
            break;

        case XSP_DISP_GRAY:
            color_mode = XRAYLIB_GRAY;
            break;

        case XSP_DISP_ORGANIC:
            color_mode = XRAYLIB_ORGANIC;
            break;

        case XSP_DISP_ORGANIC_PLUS:
            color_mode = XRAYLIB_ORGANIC_PLUS;
            break;

        case XSP_DISP_INORGANIC:
            color_mode = XRAYLIB_INORGANIC;
            break;

        case XSP_DISP_INORGANIC_PLUS:
            color_mode = XRAYLIB_INORGANIC_PLUS;
            break;

        case XSP_DISP_ORGANIC_ENHANCE:
            color_mode = XRAYLIB_Z789;
            break;

        case XSP_DISP_ORGANIC_ENHANCE_7:
            color_mode = XRAYLIB_Z7;
            break;

        case XSP_DISP_ORGANIC_ENHANCE_8:
            color_mode = XRAYLIB_Z8;
            break;

        case XSP_DISP_ORGANIC_ENHANCE_9:
            color_mode = XRAYLIB_Z9;
            break;

        default:
            /* color_mode = XRAYLIB_DUALCOLOR; */
            XSP_LOGE("the disp color type [%d]!\n", type);
            return SAL_FAIL;
    }

    XSP_LOGI("color_mode %d!\n", color_mode);

    /*配置颜色模式*/
    en_config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_COLOR;
    st_config_image.value = &color_mode;
    hr = libXRay_SetConfig(p_handle, &st_config_image, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig disp is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_rotate
 * @brief:      设置旋转类型
 * @param[in]:  void *p_handle      算法句柄
 * @param[in]:  U32 type            类型
 * @param[out]: None
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_rotate(void *p_handle, U32 type, XRAY_PROC_TYPE enType)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    /*类型*/
    switch (enType)
    {
        case XRAY_TYPE_NORMAL:
            switch (type)
            {
                case ROTATE_MODE_90:
                    st_config_param.numeratorValue = XRAYLIB_MOVE_RIGHT;
                    break;

                case ROTATE_MODE_270:
                    st_config_param.numeratorValue = XRAYLIB_MOVE_LEFT;
                    break;

                default:
                    XSP_LOGE("set_rotate type [%d] entype [%d]!\n", type, enType);
                    return SAL_FAIL;
            }
            break;
        case XRAY_TYPE_PLAYBACK:
        case XRAY_TYPE_TRANSFER:
            switch (type)
            {
                case ROTATE_MODE_90:
                    st_config_param.numeratorValue = XRAYLIB_ENTIRE_MOVE_RIGHT;
                    break;

                case ROTATE_MODE_270:
                    st_config_param.numeratorValue = XRAYLIB_ENTIRE_MOVE_LEFT;
                    break;

                default:
                    XSP_LOGE("set_rotate type [%d] entype [%d]!\n", type ,enType);
                    return SAL_FAIL;
            }
            break;

        default:
            XSP_LOGE("set_rotate type [%d] entype [%d]!\n", type ,enType);
        return SAL_FAIL;
    }

    /*配置旋转*/
    st_config_param.denominatorValue = 1;
    st_config_param.key = XRAYLIB_PARAM_ROTATE;
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig rotate is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_get_rotate
 * @brief:      获取旋转类型
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 *p_type         类型
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_get_rotate(void *p_handle, U32 *p_type)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    st_config_param.denominatorValue = 1;
    st_config_param.key = XRAYLIB_PARAM_ROTATE;
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    hr = libXRay_GetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig rotate is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    switch (st_config_param.numeratorValue)
    {
        case XRAYLIB_MOVE_RIGHT:
            *p_type = ROTATE_MODE_90;
            break;

        case XRAYLIB_MOVE_LEFT:
            *p_type = ROTATE_MODE_270;
            break;

        default:
            /* *p_type = ROTATE_MODE_90; */
            XSP_LOGE("the ratate type is error with [%d]!\n", st_config_param.numeratorValue);
            return SAL_FAIL;
    }

    XSP_LOGT("get_rotate numeratorValue %d, type:%d!\n", st_config_param.numeratorValue, *p_type);

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_mirror
 * @brief:      设置镜像类型
 * @param[in]:  void *p_handle      算法句柄
 * @param[in]:  U32 type            类型
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_mirror(void *p_handle, U32 type)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    U32 rotate_type = 0;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    (void)rin_native_get_rotate(p_handle, &rotate_type);

    if (ROTATE_MODE_270 == rotate_type)
    {
        switch (type)
        {
            case MIRROR_MODE_VERTICAL:
                st_config_param.numeratorValue = XRAYLIB_MIRROR_NONE;
                break;

            case MIRROR_MODE_NONE:
                st_config_param.numeratorValue = XRAYLIB_MIRROR_UPDOWN;
                break;

            default:
                /* st_config_param.numeratorValue = XRAYLIB_MIRROR_NONE; */
                XSP_LOGE("the ratate type is error with [%d]!\n", rotate_type);
                return SAL_FAIL;
        }
    }
    else /* if(ROTATE_MODE_90 == rotate_type) */
    {
        /*向左向右*/
        switch (type)
        {
            case MIRROR_MODE_NONE:
                st_config_param.numeratorValue = XRAYLIB_MIRROR_NONE;
                break;

            case MIRROR_MODE_VERTICAL:
                st_config_param.numeratorValue = XRAYLIB_MIRROR_UPDOWN;
                break;

            default:
                /* st_config_param.numeratorValue = XRAYLIB_MIRROR_NONE; */
                XSP_LOGE("the ratate type is error with [%d]!\n", rotate_type);
                return SAL_FAIL;
        }
    }

    XSP_LOGT("set_mirror type:%d, rotate_type [%d], numeratorValue %d!\n", type, rotate_type, st_config_param.numeratorValue);

    /*配置镜像*/
    st_config_param.denominatorValue = 1;
    st_config_param.key = XRAYLIB_PARAM_MIRROR;
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig rotate is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_get_mirror
 * @brief:      获取镜像类型
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 *p_type         类型
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_get_mirror(void *p_handle, U32 *p_type)
{
    U32 rotate_type = 0;
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};
    XRAYLIB_SETCONFIG_CHANNEL config_chan = 0;

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    st_config_param.key = XRAYLIB_PARAM_MIRROR;
    st_config_param.denominatorValue = 1;
    config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    hr = libXRay_GetConfig(p_handle, &st_config_param, config_chan);
    if (XRAY_LIB_OK != hr)
    {
        XSP_LOGE("libXRay_GetConfig is error with hr = 0x%x!", hr);
        return SAL_FAIL;
    }

    (void)rin_native_get_rotate(p_handle, &rotate_type);

    if (ROTATE_MODE_270 == rotate_type)
    {
        switch (st_config_param.numeratorValue)
        {
            case XRAYLIB_MIRROR_NONE:
                *p_type = MIRROR_MODE_VERTICAL;
                break;

            case XRAYLIB_MIRROR_UPDOWN:
                *p_type = MIRROR_MODE_NONE;
                break;

            default:
                /* *p_type = MIRROR_MODE_NONE; */
                XSP_LOGE("the ratate type is error with [%d]!\n", rotate_type);
                return SAL_FAIL;
        }
    }
    else if (ROTATE_MODE_90 == rotate_type)
    {
        switch (st_config_param.numeratorValue)
        {
            case XRAYLIB_MIRROR_NONE:
                *p_type = MIRROR_MODE_NONE;
                break;

            case XRAYLIB_MIRROR_UPDOWN:
                *p_type = MIRROR_MODE_VERTICAL;
                break;

            default:
                /* *p_type = MIRROR_MODE_NONE; */
                XSP_LOGE("the ratate type is error with [%d]!\n", rotate_type);
                return SAL_FAIL;
        }
    }
    else
    {
        XSP_LOGE("rin_native_get_rotate rotate type  is error with  %d!", rotate_type);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_pseudo_color
 * @brief:      设置伪彩类型(单能)
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 type            类型
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_pseudo_color(void *p_handle, U32 type)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};
    XRAYLIB_SINGLECOLORTABLE_SEL single_color = XRAYLIB_SINGLECOLORTABLE_SEL1;

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    switch (type)
    {
        case XSP_PSEUDO_COLOR_0:
            single_color = XRAYLIB_SINGLECOLORTABLE_SEL1;
            break;

        case XSP_PSEUDO_COLOR_1:
            single_color = XRAYLIB_SINGLECOLORTABLE_SEL2;
            break;

        default:
            XSP_LOGE("pseudo_color type  is error with  %d!", type);
            return SAL_FAIL;
    }

    config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_SINGLECOLORTABLE;
    st_config_image.value = &single_color;
    hr = libXRay_SetConfig(p_handle, &st_config_image, config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig pseudo_color is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_luminance
 * @brief:      设置亮度类型(单能)
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: S32 type            类型
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_luminance(void *p_handle, S32 type)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    if (type > XRAY_LIB_BRIGHTNESSLEVEL)
    {
        XSP_LOGE("luminance type  is error with  %d!", type);
        return SAL_FAIL;
    }

    config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_BRIGHTNESS;
    st_config_image.value = &type;
    hr = libXRay_SetConfig(p_handle, &st_config_image, config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig set_luminance is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/******************************************************************
 * @function:   rin_native_set_default_enhance
 * @brief:      设置默认增强模式
 * @param[in]:  void *p_handle  算法句柄
 * @param[in]:  U16 defStyle    默认增强方法
 * @param[in]:  S32 value       默认的增强等级,上层下发[0, 100],传给算法[-10, 60]
 * @return:     SAL_STATUS      SAL_SOK 成功
 *                              其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_default_enhance(void *p_handle, XSP_DEFAULT_STATE defStyle, S32 value)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    if (XSP_DEFAULT_STATE_CLOSE == defStyle || XSP_DEFAULT_STATE_MODE0 == defStyle
        || XSP_DEFAULT_STATE_MODE1 == defStyle || XSP_DEFAULT_STATE_MODE2 == defStyle)
    {
        st_config_param.numeratorValue = defStyle;
    }
    else
    {
        XSP_LOGE("Set Default mode %d is error!", defStyle);
        return SAL_FAIL;
    }

    XSP_LOGI("Set default_enhance defStyle %d, value %d\n!", defStyle, value);

    /*默认增强模式*/
    st_config_param.denominatorValue = 1;
    st_config_param.key = XRAYLIB_PARAM_DEFAUL_ENHANCE;
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    /* 分子映射关系：[0, 50] -> [-10, 10], [50, 100] -> [10, 60] 增强等级范围为[-10, 60]*/
    st_config_param.key = XRAYLIB_PARAM_DEFAULTENHANCE_INTENSITY;
    st_config_param.denominatorValue = (value < 50) ? 5 : 2;
    value = (value < 50) ? (SAL_CLIP(value, 0, 50) * 2 - 50) : (SAL_CLIP(value, 50, 100) * 2 - 80);
    st_config_param.numeratorValue = value;
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_anti
 * @brief:      设置反色
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 enable          使能
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_anti(void *p_handle, U32 enable)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_INVERSE_MODE anti_mode;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    XSP_LOGI("SetConfig Anti %d\n", enable);

    if (enable)
    {
        anti_mode = XRAYLIB_INVERSE_INVERSE;
    }
    else
    {
        anti_mode = XRAYLIB_INVERSE_NONE;
    }

    /*反色显示*/
    en_config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_INVERSE;
    st_config_image.value = &anti_mode;
    hr = libXRay_SetConfig(p_handle, &st_config_image, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/*******************************************************************
 * @fn      rin_native_get_anti
 * @brief   获取反色状态
 * @param[IN]   p_handle 图像处理通道对应的句柄
 * @param[OUT]  S32 *s32GetAnti 反色状态
 * @return  SAL_STATUS SAL_SOK-设置成功，SAL_FAIL-设置失败
 *******************************************************************/
SAL_STATUS rin_native_get_anti(void *p_handle,S32 *s32GetAnti)
{   
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};
    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    if (NULL == s32GetAnti)
    {
        XSP_LOGE("Null s32GetAnti pointer!\n");
        return SAL_FAIL;
    }
    st_config_image.key = XRAYLIB_INVERSE;
    st_config_image.value = s32GetAnti;
    hr = libXRay_GetConfig(p_handle, &st_config_image, XRAYLIB_SETCONFIG_IMAGEORDER);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_GetConfig for anti failed, gotten anti value:%d.\n", *s32GetAnti);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/******************************************************************
 * @function:   rin_native_set_lp
 * @brief:      设置低穿
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 enable          使能
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_lp(void *p_handle, U32 enable)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_PENETRATION_MODE lp_mode;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    XSP_LOGI("SetConfig lp %d\n", enable);

    if (enable)
    {
        lp_mode = XRAYLIB_PENETRATION_LOWPENE;
    }
    else
    {
        lp_mode = XRAYLIB_PENETRATION_NORMAL;
    }

    /*低穿*/
    en_config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_PENETRATION;
    st_config_image.value = &lp_mode;
    hr = libXRay_SetConfig(p_handle, &st_config_image, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_hp
 * @brief:      设置高穿
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 enable          使能
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_hp(void *p_handle, U32 enable)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_PENETRATION_MODE hp_mode;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    XSP_LOGI("SetConfig hp %d\n", enable);

    if (enable)
    {
        hp_mode = XRAYLIB_PENETRATION_HIGHPENE;
    }
    else
    {
        hp_mode = XRAYLIB_PENETRATION_NORMAL;
    }

    /*配置高穿*/
    en_config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_PENETRATION;
    st_config_image.value = &hp_mode;
    hr = libXRay_SetConfig(p_handle, &st_config_image, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_le
 * @brief:      设置局增
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 enable          使能
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_le(void *p_handle, U32 enable)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_ENHANCE_MODE le_mode = XRAYLIB_ENHANCE_NORAML;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    XSP_LOGI("SetConfig le %d\n", enable);

    if (enable)
    {
        le_mode = XRAYLIB_SPECIAL_LOCALENHANCE;
    }
    else
    {
        le_mode = XRAYLIB_ENHANCE_NORAML;
    }

    /*局部增强*/
    en_config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_ENHANCE;
    st_config_image.value = &le_mode;
    hr = libXRay_SetConfig(p_handle, &st_config_image, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_SPECIAL_LOCALENHANCE is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_ue
 * @brief:      设置超增
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 enable          使能
 * @param[out]: U32 value           等级
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_ue(void *p_handle, U32 enable, U32 value)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_ENHANCE_MODE ue_mode = 0;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    XSP_LOGI("SetConfig ue %d, value %d\n", enable, value);

    if (enable)
    {
        ue_mode = XRAYLIB_ENHANCE_SUPER;
    }
    else
    {
        ue_mode = XRAYLIB_PENETRATION_NORMAL;
    }

    /*超级增强*/
    en_config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_ENHANCE;
    st_config_image.value = &ue_mode;
    hr = libXRay_SetConfig(p_handle, &st_config_image, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_ENHANCE_SUPER is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_ee
 * @brief:      设置边增
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 enable          使能
 * @param[out]: U32 value           等级
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_ee(void *p_handle, BOOL bEnable, U32 value)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_ENHANCE_MODE ee_mode;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};
    XRAYLIB_CONFIG_PARAM st_config_param = {0};
    int level = (int)value;

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    XSP_LOGI("SetConfig ee %d, value %d\n", bEnable, value);

    if (bEnable)
    {
        ee_mode = XRAYLIB_ENHANCE_EDGE1;
    }
    else
    {
        ee_mode = XRAYLIB_PENETRATION_NORMAL;
    }

    /*边缘增强*/
    en_config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_ENHANCE;
    st_config_image.value = &ee_mode;
    hr = libXRay_SetConfig(p_handle, &st_config_image, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    /* 分子映射关系：[0,100]->[-100,100]，分母固定为10，EE增强等级范围为[-10, 10] */
    level = (SAL_MIN(level, 100) - 50) * 2;
    XSP_LOGI("SetConfig ee, enable %d, value %d, level %d\n", bEnable, value, level);

    st_config_param.key = XRAYLIB_PARAM_EDGE1ENHANCE_INTENSITY;
    st_config_param.denominatorValue = 10;
    st_config_param.numeratorValue = level;
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_absor
 * @brief:      设置可变吸收率
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 enable          使能
 * @param[out]: U32 value           等级，[0, XRAY_LIB_GRAYLEVEL-1]
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_absor(void *p_handle, U32 enable, U32 value)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};
    INT32 level = 0;

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    XSP_LOGI("SetConfig ue %d, value %d.\n", enable, value);

    if (enable)
    {
        level = value;
    }
    else
    {
        level = -1;
    }

    /* 可变吸收率 */
    en_config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_GRAYLEVEL;
    st_config_image.value = &level;
    hr = libXRay_SetConfig(p_handle, &st_config_image, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_alert
 * @brief:      设置难穿透识别
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 enable          使能
 * @param[out]: U32 sensitivity     灵敏度
 * @param[out]: U32 color           颜色
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_unpen_alert(void *p_handle, U32 enable, U32 sensitivity, U32 color)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAY_LIB_ONOFF enEnble = XRAYLIB_OFF;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    if (enable > SAL_TRUE)
    {
        XSP_LOGE("the unpen enable[%d] is error!\n", enable);
        return SAL_FAIL;
    }

    if (sensitivity >= XSP_SENSITIVITY_NUM)
    {
        XSP_LOGE("unpen sensitivity[%d] is error!\n", sensitivity);
        return SAL_FAIL;
    }

    XSP_LOGI("SetConfig unpen enable %d, sensitivity %d\n", enable, sensitivity);

    if (enable)
    {
        enEnble = XRAYLIB_ON;
    }
    else
    {
        enEnble = XRAYLIB_OFF;
    }

    /*难穿透*/
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    st_config_param.key = XRAYLIB_PARAM_LOWPENETRATION_PROMPT;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = enEnble;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_LOWPENETRATION_PROMPT error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    /*难穿透等级*/
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    st_config_param.key = XRAYLIB_PARAM_LOWPENETRATION_SENSTIVTY;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = sensitivity;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_LOWPENETRATION_SENSTIVTY error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_sus_alert
 * @brief:      设置可疑物识别
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 enable          使能
 * @param[out]: U32 sensitivity     灵敏度
 * @param[out]: U32 color           颜色
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_sus_alert(void *p_handle, U32 enable, U32 sensitivity, U32 color)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    if (enable > SAL_TRUE)
    {
        XSP_LOGE("the sus enable[%d] is error!\n", enable);
        return SAL_FAIL;
    }

    if (sensitivity >= XSP_SENSITIVITY_NUM)
    {
        XSP_LOGE("the sus sensitivity[%d] is error!\n", sensitivity);
        return SAL_FAIL;
    }

    XSP_LOGI("the sus enable[%d], sensitivity[%d], color[%d]!\n", enable, sensitivity, color);

    g_native_pro_attr.b_sus_enable = enable;

    /*可疑物识别开关*/
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    st_config_param.key = XRAYLIB_PARAM_CONCERDMATERIAL_PROMPT;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = enable; /*可疑物识别开关*/
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_CONCERDMATERIAL_PROMPT error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    /*可疑物识别*/
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    st_config_param.key = XRAYLIB_PARAM_CONCERDMATERIAL_SENSTIVTY;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = sensitivity; /*等级正好与可疑物识别等级对应*/
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_CONCERDMATERIAL_SENSTIVTY error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @fn      rin_native_set_bg
 * @brief   设置背景参数
 * @param   p_handle[IN] 图像处理通道对应的句柄
 * @param   thr[IN]      背景阈值， 参数[0,65535]
 * @param   fullupdate_ratio[IN] 矫正数据更新的比例[0, 100],0表示不更新，100表示以前的数据去掉，全部更新
 * @return  SAL_STATUS   SAL_SOK-设置成功，SAL_FAIL-设置失败
 *******************************************************************/
SAL_STATUS rin_native_set_bg(void *p_handle, U32 thr, U32 fullupdate_ratio)
{
    U32 ratio = 0;
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};
    CAPB_XRAY_IN *p_xray_in = capb_get_xrayin();

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    if (thr > 65535)
    {
        XSP_LOGE("the bg thr[%d] is error!\n", thr);
    }

    if (fullupdate_ratio > 100)
    {
        XSP_LOGE("the bg fullupdate_ratio[%d] is error!\n", fullupdate_ratio);
    }

    ratio = fullupdate_ratio;

    XSP_LOGI("the bg thr %d, fullupdate_ratio %d, ratio %d!\n", thr, fullupdate_ratio, ratio);

    /*背景阈值*/
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    st_config_param.key = XRAYLIB_PARAM_BACKGROUNDTHRESHOLED;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = thr;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    if(XRAY_ENERGY_DUAL == p_xray_in->xray_energy_num)
    {
        /* 双能分类灰度上限(双能分类灰度上限和背景噪声阈值配置统一) */
        st_config_param.key = XRAYLIB_PARAM_DUALDISTINGUISH_GRAYUP;
        hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
        if (XRAY_LIB_OK != hr)
        {
            XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
            return SAL_FAIL;
        }
    }

    /*更新比例，[0, 100]->[0.0, 1.0]*/
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    st_config_param.key = XRAYLIB_PARAM_RTUPDATEAIRRATIO;
    st_config_param.denominatorValue = 100;
    st_config_param.numeratorValue = ratio;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @fn      rin_native_set_color_table
 * @brief   设置成像颜色配置表
 * @param   p_handle[IN] 图像处理通道对应的句柄
 * @param   type[IN]     成像颜色配置表类型， 参数[0,3]
 * @return  SAL_STATUS   SAL_SOK-设置成功，SAL_FAIL-设置失败
 *******************************************************************/
SAL_STATUS rin_native_set_color_table(void *p_handle, U32 type)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};
    XRAYLIB_DUALCOLORTABLE_SEL dual_color = XRAYLIB_DUALCOLORTABLE_SEL1;

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    switch (type)
    {
        case XSP_COLOR_TABLE_0:
            dual_color = XRAYLIB_DUALCOLORTABLE_SEL1;
            break;

        case XSP_COLOR_TABLE_1:
            dual_color = XRAYLIB_DUALCOLORTABLE_SEL2;
            break;

        case XSP_COLOR_TABLE_2:
            dual_color = XRAYLIB_DUALCOLORTABLE_SEL3;
            break;

        case XSP_COLOR_TABLE_3:
            dual_color = XRAYLIB_DUALCOLORTABLE_SEL4;
            break;

        case XSP_COLOR_TABLE_4:
            dual_color = XRAYLIB_DUALCOLORTABLE_SEL5;
            break;

        case XSP_COLOR_TABLE_5:
            dual_color = XRAYLIB_DUALCOLORTABLE_SEL6;
            break;

        case XSP_COLOR_TABLE_6:
            dual_color = XRAYLIB_DUALCOLORTABLE_SEL7;
            break;
        
        case XSP_COLOR_TABLE_7:
            dual_color = XRAYLIB_DUALCOLORTABLE_SEL8;
            break;

        default:
            XSP_LOGE("color table type[%d] error!\n", type);
            return SAL_FAIL;
    }

    XSP_LOGI("SetConfig color_table dual_color %d\n", dual_color);

    config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_DUALCOLORTABLE;
    st_config_image.value = &dual_color;
    hr = libXRay_SetConfig(p_handle, &st_config_image, config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig color table is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @fn      rin_native_set_colors_imaging
 * @brief   设置切换三色显示和六色显示
 * @param   p_handle[IN] 图像处理通道对应的句柄
 * @param   type[IN]     成像颜色三色六色显示枚举， 参数[0,1]
 * @return  SAL_STATUS   SAL_SOK-设置成功，SAL_FAIL-设置失败
 *******************************************************************/
SAL_STATUS rin_native_set_colors_imaging(void *p_handle, U32 type)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL config_chan = 0;
    XRAYLIB_CONFIG_IMAGE st_config_image = {0};
    XRAYLIB_COLORSIMAGING_SEL colors_imaging = XRAYLIB_COLORSIMAGING_3S;

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    if (type == XSP_COLORS_IMAGING_6S)
    {
        colors_imaging = XRAYLIB_COLORSIMAGING_6S;
    }
    else
    {
        colors_imaging = XRAYLIB_COLORSIMAGING_3S;
        if (type != XSP_COLORS_IMAGING_3S)
        {
            XSP_LOGW("SetConfig colors_imaging type[%d] err! Use default 3S\n", type);
        }
    }

    XSP_LOGI("SetConfig colors_imaging colors_imaging[%d]\n", colors_imaging);

    config_chan = XRAYLIB_SETCONFIG_IMAGEORDER;
    st_config_image.key = XRAYLIB_COLORSIMAGING;
    st_config_image.value = &colors_imaging;
    hr = libXRay_SetConfig(p_handle, &st_config_image, config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig colors_imaging is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_set_deformity_correction
 * @brief:      设置畸变矫正
 * @param[in]:  void *p_handle      算法句柄
 * @param[out]: U32 enable          使能
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_set_deformity_correction(void *p_handle, U32 enable)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    if (enable > SAL_TRUE)
    {
        XSP_LOGE("the deformity correction enable[%d] is error!\n", enable);
        return SAL_FAIL;
    }

    XSP_LOGI("SetConfig deformity_correction enable %d\n", enable);

    if (SAL_TRUE == enable)
    {
        st_config_param.numeratorValue = XRAYLIB_ON;
    }
    else
    {
        st_config_param.numeratorValue = XRAYLIB_OFF;
    }

    /*畸变矫正*/
    st_config_param.denominatorValue = 1;
    st_config_param.key = XRAYLIB_PARAM_GEOMETRIC_CALI;
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @fn      rin_native_set_dose_correct
 * @brief   剂量波动修正
 * @param   p_handle[IN] 图像处理通道对应的句柄
 * @param   U32 value    等级
 * @return  SAL_STATUS   SAL_SOK-设置成功，SAL_FAIL-设置失败
 *******************************************************************/
SAL_STATUS rin_native_set_dose_correct(void *p_handle, U32 value)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    if (value > 100)
    {
        XSP_LOGE("the param:%d is invalid!Level:0~100\n", value);
        return SAL_FAIL;
    }

    XSP_LOGI("SetConfig Correct Level:%d\n", value);
    /*剂量波动修正*/
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = value;
    st_config_param.key = XRAYLIB_PARAM_XRAYDOSE_CALI;
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @fn      rin_native_set_color_adjust
 * @brief   颜色调节
 * @param   p_handle[IN] 图像处理通道对应的句柄
 * @param   U32 value    等级
 * @return  SAL_STATUS   SAL_SOK-设置成功，SAL_FAIL-设置失败
 *******************************************************************/
SAL_STATUS rin_native_set_color_adjust(void *p_handle, U32 value)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_SETCONFIG_CHANNEL en_config_chan = 0;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};
    int level = (int)value;

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    /* 分子映射关系：[0,100]->[-100,100]，分母固定为10，范围为[-10, 10] */
    level = (SAL_MIN(level, 100) - 50) * 2;
    XSP_LOGI("SetConfig color adjust, value %d, level %d\n", value, level);

    st_config_param.key = XRAYLIB_PARAM_MATERIAL_ADJUST;
    st_config_param.denominatorValue = 10;
    st_config_param.numeratorValue = level;
    en_config_chan = XRAYLIB_SETCONFIG_PARAMCHANGE;
    hr = libXRay_SetConfig(p_handle, &st_config_param, en_config_chan);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig is error hr = 0x%x.\n", hr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      rin_native_set_belt_speed
 * @brief   设置传送带速度
 *
 * @param   [IN] p_handle 算法实例句柄，仅支持实时和回拉处理类型
 *
 * @return  XSP_SPEEDUP_MODE 加速模式
 */
SAL_STATUS rin_native_set_belt_speed(void *p_handle, FLOAT32 f32BeltSpeed,UINT32 line_per_slice, XRAY_PROC_SPEED enSpeed)
{
    XRAYLIB_CONFIG_PARAM st_config_param = {0};
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAY_SPEEDGEAR enSpeedGear = 0;

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return XSP_SPEEDUP_NONE;
    }

    XSP_LOGI("set belt speed:%f m/s line_per_slice %d \n", f32BeltSpeed, line_per_slice);

    st_config_param.key = XRAYLIB_PARAM_SPEED;
    st_config_param.denominatorValue = 1000;
    st_config_param.numeratorValue = (INT32)(f32BeltSpeed * 1000);
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig 'XRAYLIB_PARAM_SPEED' failed, value: %d, hr: 0x%x\n",
                 st_config_param.numeratorValue, hr);
        return SAL_FAIL;
    }

    st_config_param.key = XRAYLIB_PARAM_RT_HEIGHT;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = (INT32)line_per_slice;
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig 'XRAYLIB_PARAM_RT_HEIGHT' failed, value: %d, hr: 0x%x\n",
                 st_config_param.numeratorValue, hr);
        return SAL_FAIL;
    }

    enSpeedGear = enSpeed;  
    st_config_param.key = XRAYLIB_PARAM_SPEEDGEAR;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = (INT32) enSpeedGear;
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig 'XRAYLIB_PARAM_SPEEDGEAR' failed, value: %d, hr: 0x%x\n",
                 st_config_param.numeratorValue, hr);
        // return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    rin_native_set_noise_reduction
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS rin_native_set_noise_reduction(void *p_handle, U32 level)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    st_config_param.key = XRAYLIB_PARAM_DENOISING_INTENSITY;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = SAL_CLIP(level, 0, 100) / 20;
    st_config_param.numeratorValue = SAL_CLIP(st_config_param.numeratorValue, XRAYLIB_DENOISING_MIN_INTENSITY, XRAYLIB_DENOISING_MAX_INTENSITY);
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_DENOISING_MODE failed, errno: %d, value: %d\n", hr, st_config_param.numeratorValue);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      rin_native_set_contrast
 * @brief   复用为融合灰度图OFFSET
 * 
 * @param   [IN] p_handle 算法实例句柄
 * @param   [IN] level OFFSET等级，范围[0, 100]，映射到算法库的[0, 10000]
 * 
 * @return  SAL_STATUS SAL_SOK：成功，SAL_FAIL：失败
 */
SAL_STATUS rin_native_set_contrast(void *p_handle, U32 level)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    st_config_param.key = XRAYLIB_PARAM_COMPOSITIVE_RATIO;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = SAL_CLIP(level, 0, 100) * 100;
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_MERGE_BASELINE failed, errno: %d, value: %d\n", hr, st_config_param.numeratorValue);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function    rin_native_set_gamma
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS rin_native_set_gamma(void *p_handle, U32 mode_switch, U32 value)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }
    
    /* gamma设置开关 ，只有打开时才需要设置gamma参数 0-关闭 1-打开*/
    st_config_param.key = XRAYLIB_PARAM_GAMMA_MODE;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = (1 == mode_switch) ? XRAYLIB_GAMMA_OPEN : XRAYLIB_GAMMA_CLOSE;
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_GAMMA_MODE failed, errno: %d mode %d , value: %d\n", hr, mode_switch, st_config_param.numeratorValue);
        return SAL_FAIL;
    }
    if(mode_switch == 1)
    {
        /* 分子映射关系：[0-100]->[10,30]，分母固定为10，增强等级范围为[1, 3] */
        value = SAL_CLIP(value, 0, 100) / 5 + 10;
        st_config_param.key = XRAYLIB_PARAM_GAMMA_INTENSITY;
        st_config_param.denominatorValue = 10;
        st_config_param.numeratorValue = value;
        hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_GAMMA_VALUE failed, errno: %d, mode %d value: %d\n", hr, mode_switch, st_config_param.numeratorValue);
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}


/**
 * @function    rin_native_set_aixsp_rate
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS rin_native_set_aixsp_rate(void *p_handle, U32 value)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }
    /* 分子映射关系：[0,100]->[0,10]，分母固定为10，增强等级范围为[0, 1] */

    st_config_param.key = XRAYLIB_PARAM_SIGMA_NOISES;
    st_config_param.denominatorValue = 10;
    st_config_param.numeratorValue = SAL_CLIP(value, 0, 100) / 10;
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_SIGMA_NOISES failed, errno: %d, value: %d\n", hr, st_config_param.numeratorValue);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @function    rin_native_set_stretchratio
 * @brief       设置拉伸比例
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS rin_native_set_stretch_ratio(void *p_handle, U32 stretch_ratio)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    st_config_param.key = XRAYLIB_PARAM_STRETCHRATIO;
    st_config_param.denominatorValue = 100;
    st_config_param.numeratorValue = SAL_CLIP(stretch_ratio, 80, 100);
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_STRETCHRATIO failed, errno: %d, value: %d\n", hr, st_config_param.numeratorValue);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    rin_native_set_le_th_range
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS rin_native_set_le_th_range(void *p_handle, U32 lower_limit, U32 upper_limit)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    if (lower_limit > upper_limit)
    {
        XSP_LOGE("the lower_limit[%u] is bigger than upper_limit[%u]\n", lower_limit, upper_limit);
        return SAL_FAIL;
    }

    st_config_param.key = XRAYLIB_PARAM_SPECIALENHANCE_THRESHOLDDOWN;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = SAL_CLIP(lower_limit, 0, 65535);
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_SPECIALENHANCE_THRESHOLDDOWN failed, errno: %d, value: %d\n", hr, st_config_param.numeratorValue);
        return SAL_FAIL;
    }

    st_config_param.key = XRAYLIB_PARAM_SPECIALENHANCE_THRESHOLDUP;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = SAL_CLIP(upper_limit, 0, 65535);
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_SPECIALENHANCE_THRESHOLDUP failed, errno: %d, value: %d\n", hr, st_config_param.numeratorValue);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    rin_native_set_dt_gray_range
 * @brief   设置双能分类灰度下限 (双能分类灰度上限与背景噪声阈值统一, 此处不处理)
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS rin_native_set_dt_gray_range(void *p_handle, U32 lower_limit, U32 upper_limit)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    st_config_param.key = XRAYLIB_PARAM_DUALDISTINGUISH_GRAYDOWN;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = SAL_CLIP(lower_limit, 0, 65535);
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_DUALDISTINGUISH_GRAYDOWN failed, errno: %d, value: %d\n", hr, st_config_param.numeratorValue);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    rin_native_set_bkg_color
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
SAL_STATUS rin_native_set_bkg_color(void *p_handle, U32 u32BkgColor)
{
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    st_config_param.key = XRAYLIB_PARAM_BACKGROUNDCOLOR;
    st_config_param.numeratorValue = u32BkgColor;
    st_config_param.denominatorValue = 1;
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_BACKGROUNDCOLOR failed, handle:%p, errno:0x%x, value:0x%x\n", p_handle, hr, st_config_param.numeratorValue);
        return SAL_FAIL;
    }
    return SAL_SOK;
}


/**
 * @fn      rin_native_set_coldhot_threshold
 * @brief   设置冷热源参数阈值，现复用为设置背景自动校正灵敏度
 * 
 * @param   [IN] p_handle 算法句柄
 * @param   [IN] level 冷热源参数阈值，范围[0, 800]，默认值为100
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功；SAL_FAIL：失败
 */
SAL_STATUS rin_native_set_coldhot_threshold(void *p_handle, U32 level)
{
    INT32 idx = 0;
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_CONFIG_PARAM st_config_param = {0};

    if (NULL == p_handle)
    {
        XSP_LOGE("the p_handle[%p] is invalid!\n", p_handle);
        return SAL_FAIL;
    }

    idx = rin_native_get_handle_idx(p_handle);
    if (idx < 0)
    {
        XSP_LOGE("the p_handle[%p] is invalid\n", p_handle);
        return SAL_FAIL;
    }

    // DSP内部背景自动校正，默认值100，对应其灵敏度为50
    XSP_LOGI("chan %d, coldhot threshold %u, bgac sensitivity %u\n", idx, level, SAL_MIN(level, 200) / 2);
    xray_bgac_set_sensitivity(g_rt_pipeline[idx].pBgacHandle, SAL_MIN(level, 200) / 2);

    st_config_param.key = XRAYLIB_PARAM_COLDHOTTHRESHOLED;
    st_config_param.denominatorValue = 1;
    st_config_param.numeratorValue = 0; // 关闭XSP算法库内部的冷热源校正
    hr = libXRay_SetConfig(p_handle, &st_config_param, XRAYLIB_SETCONFIG_PARAMCHANGE);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGE("libXRay_SetConfig XRAYLIB_PARAM_COLDHOTTHRESHOLED failed, errno: %d, value: %d\n", hr, st_config_param.numeratorValue);
        return SAL_FAIL;
    }

    return SAL_SOK;   
}

/******************************************************************
 * @fn      rin_native_get_zdata_version
 * @brief   获取成像算法原子序数数据版本
 * @param   p_handle[IN] 图像处理通道对应的句柄
 * @param   type[IN]     成像算法原子序数数据类型
 * @return  SAL_STATUS   SAL_SOK-设置成功，SAL_FAIL-设置失败
 *******************************************************************/
SAL_STATUS rin_native_get_zdata_version(void *p_handle, U32 *type)
{
    if ((NULL == p_handle) || (NULL == type))
    {
        XSP_LOGE("the handle[%p], type[%p] is invalid\n", p_handle, type);
        return SAL_FAIL;
    }

    *type = XSP_ZDATA_VERSION_V1;
    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_rtpreview_process
 * @brief:      实时过包处理流程，包括成像处理，包裹检测，难穿透识别，可疑物识别，tip注入等
 * @param[in]:  void *p_handle                                             算法句柄
 * @param[in]:  XSP_RTPROC_IN st_rtproc_in 实时过包的原始数据
 * @param[out]: XSP_RTPROC_OUT *pst_rtproc_out 实时过包的归一化数据，YUV数据和识别结果
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_rtpreview_process(void *p_handle, XSP_RTPROC_IN st_rtproc_in, XSP_RTPROC_OUT *pst_rtproc_out)
{
    S32 idx = 0;
    DSA_NODE *pNode = NULL;
    PIPELINE_OBJECT_ATTR *pstPipelineObj = NULL;

    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_IMAGE stTipImg = {0};
    XRAYLIB_PIPELINE_PARAM_MODE3A *pPipelineParam3a = NULL;
    XRAYLIB_PIPELINE_PARAM_MODE2A *pPipelineParam2a = NULL;

    XIMAGE_DATA_ST *pstXrawIn = NULL;               /*归一化后的xray数据输出*/
    XIMAGE_DATA_ST *pstDispOut = NULL;              /*输出的显示YUV数据*/
    XIMAGE_DATA_ST *pstAiYuvOut = NULL;             /*输出的AI YUV数据*/
    XIMAGE_DATA_ST *pstBlendOut = NULL;             /*输出的Blend数据*/
    XIMAGE_DATA_ST *pstNrawOut = NULL;              /*输出的归一化数据*/
    XIMAGE_DATA_ST *pstOriNrawOut = NULL;           /*未超分的输出归一化数据*/
    XSP_TIP_INJECT *pst_tip_inject = NULL;          /*TIP注入参数*/
    XSP_XRAW_TIPED *pst_xraw_tiped = NULL;          /*带有tip图像的归一化xraw数据输出*/

    CHECK_PTR_NULL(p_handle, SAL_FAIL);
    CHECK_PTR_NULL(pst_rtproc_out, SAL_FAIL);

    pstXrawIn = &st_rtproc_in.stXrawIn;
    pst_tip_inject = &st_rtproc_in.st_tip_inject;
    pstDispOut = &pst_rtproc_out->stDispOut;
    pstAiYuvOut = &pst_rtproc_out->stAiYuvOut;
    pstBlendOut = &pst_rtproc_out->stBlendOut;
    pstNrawOut = &pst_rtproc_out->stNrawOut;
    pstOriNrawOut = &pst_rtproc_out->stOriNrawOut;
    pst_xraw_tiped = &pst_rtproc_out->st_xraw_tiped;

    CHECK_PTR_NULL(pstXrawIn->pPlaneVir[0], SAL_FAIL);
    CHECK_PTR_NULL(pstBlendOut->pPlaneVir[0], SAL_FAIL);
    CHECK_PTR_NULL(pstNrawOut->pPlaneVir[0], SAL_FAIL);
    if (DSP_IMG_DATFMT_LHZP == pstNrawOut->enImgFmt)
    {
        CHECK_PTR_NULL(pstNrawOut->pPlaneVir[1], SAL_FAIL);
        CHECK_PTR_NULL(pstNrawOut->pPlaneVir[2], SAL_FAIL);
    }
    CHECK_PTR_NULL(pstOriNrawOut->pPlaneVir[0], SAL_FAIL);
    if (DSP_IMG_DATFMT_LHZ16P == pstOriNrawOut->enImgFmt)
    {
        CHECK_PTR_NULL(pstOriNrawOut->pPlaneVir[1], SAL_FAIL);
        CHECK_PTR_NULL(pstOriNrawOut->pPlaneVir[2], SAL_FAIL);
    }
    CHECK_PTR_NULL(pstAiYuvOut->pPlaneVir[0], SAL_FAIL);
    if (DSP_IMG_DATFMT_YUV420SP_VU == pstAiYuvOut->enImgFmt)
    {
        CHECK_PTR_NULL(pstAiYuvOut->pPlaneVir[1], SAL_FAIL);
    }
    CHECK_PTR_NULL(pstDispOut->pPlaneVir[0], SAL_FAIL);
    if (DSP_IMG_DATFMT_YUV420SP_VU == pstDispOut->enImgFmt)
    {
        CHECK_PTR_NULL(pstDispOut->pPlaneVir[1], SAL_FAIL);
    }

    idx = rin_native_get_handle_idx(p_handle);
    if (idx < 0)
    {
        XSP_LOGE("the p_handle[%p] is invalid\n", p_handle);
        return SAL_FAIL;
    }

    /*TIP注入*/
    pst_xraw_tiped->en_tiped_status = XSP_TIPED_NONE;
    if (SAL_TRUE == pst_tip_inject->b_enable)
    {
        stTipImg.width = pst_tip_inject->tip_width;
        stTipImg.height = pst_tip_inject->tip_height;
        stTipImg.pData[0] = pst_tip_inject->p_tip_normalized;
        if (DSP_IMG_DATFMT_LHZP == pstNrawOut->enImgFmt)
        {
            stTipImg.pData[1] = (U16 *)(XRAW_HE_OFFSET(pst_tip_inject->p_tip_normalized, pst_tip_inject->tip_width, pst_tip_inject->tip_height));
            stTipImg.pData[2] = (U08 *)(XRAW_Z_OFFSET(pst_tip_inject->p_tip_normalized, pst_tip_inject->tip_width, pst_tip_inject->tip_height));
        }

        hr = libXRay_SetTIPImage(p_handle, &stTipImg);
        if (hr != XRAY_LIB_OK)
        {
            XSP_LOGW("chan %d, libXRay_SetTIPImage failed, errno: 0x%x\n", idx, hr);
            pst_xraw_tiped->en_tiped_status = XSP_TIPED_FAIL;
        }
    }

    if (SAL_SOK != SAL_mutexTmLock(&g_rt_pipeline[idx].lstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
    {
        XSP_LOGE("chan %d, SAL_mutexTmLock failed\n", idx);
        return SAL_FAIL;
    }
    pNode = DSA_LstGetIdleNode(g_rt_pipeline[idx].lstPipeline);
    if (NULL != pNode)
    {
        pstPipelineObj = (PIPELINE_OBJECT_ATTR *)pNode->pAdData;
        pstPipelineObj->s32CurPipeSeg = -1;
        DSA_LstPush(g_rt_pipeline[idx].lstPipeline, pNode);
    }
    else
    {
        CHAR debuginfo[32] = {0};
        pNode = DSA_LstGetHead(g_rt_pipeline[idx].lstPipeline);
        while (pNode != NULL)
        {
            snprintf(debuginfo+strlen(debuginfo), sizeof(debuginfo)-strlen(debuginfo), "%d ", ((PIPELINE_OBJECT_ATTR *)pNode->pAdData)->s32CurPipeSeg);
            pNode = pNode->next;
        }
        XSP_LOGE("chan %d, DSA_LstGetIdleNode from 'lstPipeline' failed, cnt: %u, pipe: %s\n", 
                 idx, DSA_LstGetCount(g_rt_pipeline[idx].lstPipeline), debuginfo);
        SAL_mutexTmUnlock(&g_rt_pipeline[idx].lstPipeline->sync.mid, __FUNCTION__, __LINE__);
        return SAL_FAIL;
    }
    SAL_mutexTmUnlock(&g_rt_pipeline[idx].lstPipeline->sync.mid, __FUNCTION__, __LINE__);

    /***************************** 输入输出参数 *****************************/
    memcpy(pstPipelineObj->pPassthData, st_rtproc_in.pPassthData, g_rt_pipeline[idx].u32PassthDataSize); // 透传参数
    memcpy(&pstPipelineObj->stDebug, &st_rtproc_in.st_debug, sizeof(XSP_DEBUG_ALG));
    pstPipelineObj->bPseudo = st_rtproc_in.bPseudo;
    switch (g_rt_pipeline[idx].enPipelineMode)
    {
    case XRAYLIB_PIPELINE_MODE2A:
        pPipelineParam2a = (XRAYLIB_PIPELINE_PARAM_MODE2A *)pstPipelineObj->pPipelineParam;
        /*TIP状态标记*/
        pstPipelineObj->st_xraw_tiped.en_tiped_status = pst_xraw_tiped->en_tiped_status;
        // 输入参数
        rin_native_2xraylibimg(pstXrawIn, &pPipelineParam2a->idx0.in_xraw, __FUNCTION__, __LINE__);
        // 输出Buffer
        rin_native_2xraylibimg(pstOriNrawOut, &pPipelineParam2a->idx1.out_calilhz_ori, __FUNCTION__, __LINE__);
        rin_native_2xraylibimg(pstNrawOut, &pPipelineParam2a->idx1.out_calilhz, __FUNCTION__, __LINE__);
        rin_native_2xraylibimg(pstBlendOut, &pPipelineParam2a->idx1.out_merge, __FUNCTION__, __LINE__);
        rin_native_2xraylibimg(pstDispOut, &pPipelineParam2a->idx1.out_disp, __FUNCTION__, __LINE__);
        rin_native_2xraylibimg(pstAiYuvOut, &pPipelineParam2a->idx1.out_ai, __FUNCTION__, __LINE__);
        pPipelineParam2a->idx1.TipStatus = XRAYLIB_TIP_NONE;

        /*TIP输出归一化buffer*/
        pPipelineParam2a->idx1.TipParam.pTipedCaliLowData = (U16 *)(XRAW_LE_OFFSET(pst_xraw_tiped->p_xraw_normalized, pstNrawOut->u32Width, pstNrawOut->u32Height));
        /*单能：低能；双能:低能高能，原子序数*/
        if (DSP_IMG_DATFMT_LHZP == pstNrawOut->enImgFmt)
        {
            pPipelineParam2a->idx1.TipParam.pTipedCaliHighData = (U16 *)(XRAW_HE_OFFSET(pst_xraw_tiped->p_xraw_normalized, \
                                                                 pstNrawOut->u32Width, pstNrawOut->u32Height));
            pPipelineParam2a->idx1.TipParam.pTipedZData = (U08 *)(XRAW_Z_OFFSET(pst_xraw_tiped->p_xraw_normalized,\
                                                            pstNrawOut->u32Width, pstNrawOut->u32Height));
        }
        else
        {
            pPipelineParam2a->idx1.TipParam.pTipedCaliHighData = NULL;
            pPipelineParam2a->idx1.TipParam.pTipedZData = NULL;
        }
        break;

    case XRAYLIB_PIPELINE_MODE3A:
    case XRAYLIB_PIPELINE_MODE3B:
        pPipelineParam3a = (XRAYLIB_PIPELINE_PARAM_MODE3A *)pstPipelineObj->pPipelineParam;
        /*TIP状态标记*/
        pstPipelineObj->st_xraw_tiped.en_tiped_status = pst_xraw_tiped->en_tiped_status;
        // 输入参数
        rin_native_2xraylibimg(pstXrawIn, &pPipelineParam3a->idx0.in_xraw, __FUNCTION__, __LINE__);
        // 输出Buffer
        rin_native_2xraylibimg(pstOriNrawOut, &pPipelineParam3a->idx2.out_calilhz_ori, __FUNCTION__, __LINE__);
        rin_native_2xraylibimg(pstNrawOut, &pPipelineParam3a->idx2.out_calilhz, __FUNCTION__, __LINE__);
        rin_native_2xraylibimg(pstBlendOut, &pPipelineParam3a->idx2.out_merge, __FUNCTION__, __LINE__);
        rin_native_2xraylibimg(pstDispOut, &pPipelineParam3a->idx2.out_disp, __FUNCTION__, __LINE__);
        rin_native_2xraylibimg(pstAiYuvOut, &pPipelineParam3a->idx2.out_ai, __FUNCTION__, __LINE__);

        pPipelineParam3a->idx2.TipStatus = XRAYLIB_TIP_NONE;

        /*TIP输出归一化buffer*/
        pPipelineParam3a->idx2.TipParam.pTipedCaliLowData = (U16 *)(XRAW_LE_OFFSET(pst_xraw_tiped->p_xraw_normalized,\
                                                pstNrawOut->u32Width, pstNrawOut->u32Height));
        /*单能：低能；双能:低能高能，原子序数*/
        if (DSP_IMG_DATFMT_LHZP == pstNrawOut->enImgFmt)
        {
            pPipelineParam3a->idx2.TipParam.pTipedCaliHighData = (U16 *)(XRAW_HE_OFFSET(pst_xraw_tiped->p_xraw_normalized,\
                                                        pstNrawOut->u32Width, pstNrawOut->u32Height));
            pPipelineParam3a->idx2.TipParam.pTipedZData = (U08 *)(XRAW_Z_OFFSET(pst_xraw_tiped->p_xraw_normalized, \
                                                        pstNrawOut->u32Width, pstNrawOut->u32Height));
        }
        else
        {
            pPipelineParam3a->idx2.TipParam.pTipedCaliHighData = NULL;
            pPipelineParam3a->idx2.TipParam.pTipedZData = NULL;
        }
        break;
    
    default:
        break;
    }

    if (SAL_SOK != SAL_mutexTmLock(&g_rt_pipeline[idx].lstPipeline->sync.mid, SAL_TIMEOUT_FOREVER, __FUNCTION__, __LINE__))
    {
        XSP_LOGE("chan %d, SAL_mutexTmLock failed\n", idx);
        return SAL_FAIL;
    }
    pstPipelineObj->s32CurPipeSeg++;
    SAL_CondSignal(&g_rt_pipeline[idx].lstPipeline->sync, SAL_COND_ST_BROADCAST, __FUNCTION__, __LINE__);
    SAL_mutexTmUnlock(&g_rt_pipeline[idx].lstPipeline->sync.mid, __FUNCTION__, __LINE__);

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_indentify_process
 * @brief:      实时过包的识别处理流程，包括难穿透识别，可疑物识别等
 * @param[in]:  void *p_handle                           算法句柄
 * @param[in]:  XSP_XRAW_NORMALIZED st_xraw_normalized 归一化数据
 * @param[out]: XSP_IDENTIFY_S *p_sus_indentify 输出可疑物结果
 * @param[out]: XSP_IDENTIFY_S *p_unpen_indentify 输出难穿透结果
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_indentify_process(void *p_handle, XSP_IDT_IN *pstIdtIn, XSP_IDENTIFY_S **pstXspIdtResult)
{
    S32 idx = 0;
    INT32 i = 0;
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_IMAGE stXrayLibImg = {0};
    XSP_IDENTIFY_S *pstXspSusIdt = NULL;
    XSP_IDENTIFY_S *pstXspUnpenIdt = NULL;
    XSP_IDENTIFY_S *pstXspExplosiveIdt = NULL;
    CAPB_XRAY_IN *pst_capb_xrayin = capb_get_xrayin();
    XRAYLIB_CONCERED_AREA concerd_material = {0};
    XRAYLIB_CONCERED_AREA low_penetration = {0};
    XRAYLIB_CONCERED_AREA stExplosiveArea = {0};
    XRAYLIB_CONCERED_AREA stDrugArea = {0};

    CHECK_PTR_NULL(p_handle, SAL_FAIL);
    CHECK_PTR_NULL(pstXspIdtResult, SAL_FAIL);
    idx = rin_native_get_handle_idx(p_handle);
    if (idx < 0)
    {
        XSP_LOGE("the p_handle[%p] is invalid\n", p_handle);
        return SAL_FAIL;
    }
    CHECK_PTR_NULL(pst_capb_xrayin, SAL_FAIL);
    CHECK_PTR_NULL(pstIdtIn->pPlaneVir[0], SAL_FAIL);
    if (XRAY_ENERGY_DUAL == pst_capb_xrayin->xray_energy_num)
    {
        CHECK_PTR_NULL(pstIdtIn->pPlaneVir[1], SAL_FAIL);
        CHECK_PTR_NULL(pstIdtIn->pPlaneVir[2], SAL_FAIL);
    }

    if (NULL != pstXspIdtResult[0])
    {
        pstXspSusIdt = pstXspIdtResult[0];
    }
    if (NULL != pstXspIdtResult[1])
    {
        pstXspUnpenIdt = pstXspIdtResult[1];
    }
    if (NULL != pstXspIdtResult[2])
    {
        pstXspExplosiveIdt = pstXspIdtResult[2];
    }

    rin_native_2xraylibimg(pstIdtIn, &stXrayLibImg, __FUNCTION__, __LINE__);

    /**
     * 大矩形表示包裹数据，内存存放顺序为从上到下，从左到右；
     * 小矩形表示算法识别的框体，top、bottom、left和right是算法返回的坐标表示
     * top是识别框下沿与包裹顶部的距离，bottom是识别框上沿与包裹顶部的距离
     *     ___________________________________________
     *    |   ^      ^                                |
     *    |   |top   | bottom                         |
     *    |   |      |        ________________        |
     *    |   |              |                |       |
     *    |   |              |                |       |
     *    |   |              |                |       |
     *    |   |              |                |       |
     *    |   |              |                |       |
     *    |   |              |________________|       |
     *    |                                           |
     *    |------------------>left                    |
     *    |----------------------------------->right  |
     *    |___________________________________________|
     */

    hr = libXRay_ConcernedArea(p_handle, &stXrayLibImg, &concerd_material, &low_penetration, &stExplosiveArea, &stDrugArea);
    if (hr == XRAY_LIB_OK)
    {
        /*有机物识别的数据转换*/
        pstXspSusIdt->num = SAL_MIN(concerd_material.nNum, XSP_IDENTIFY_NUM);
        for (i = 0; i < pstXspSusIdt->num; i++)
        {
            pstXspSusIdt->rect[i].uiX = concerd_material.CMRect[i].nLeft;
            pstXspSusIdt->rect[i].uiY = concerd_material.CMRect[i].nBottom;
            pstXspSusIdt->rect[i].uiWidth = SAL_SUB_SAFE(concerd_material.CMRect[i].nRight, concerd_material.CMRect[i].nLeft);
            pstXspSusIdt->rect[i].uiHeight = SAL_SUB_SAFE(concerd_material.CMRect[i].nTop, concerd_material.CMRect[i].nBottom);
        }

        /*难穿透识别的数据转换*/
        pstXspIdtResult[1]->num = SAL_MIN(low_penetration.nNum, XSP_IDENTIFY_NUM);
        for (i = 0; i < pstXspUnpenIdt->num; i++)
        {
            pstXspUnpenIdt->rect[i].uiX = low_penetration.CMRect[i].nLeft;
            pstXspUnpenIdt->rect[i].uiY = low_penetration.CMRect[i].nBottom;
            pstXspUnpenIdt->rect[i].uiWidth = SAL_SUB_SAFE(low_penetration.CMRect[i].nRight, low_penetration.CMRect[i].nLeft);
            pstXspUnpenIdt->rect[i].uiHeight = SAL_SUB_SAFE(low_penetration.CMRect[i].nTop, low_penetration.CMRect[i].nBottom);
        }

        /* 疑似爆炸物识别的数据转换 */
        pstXspExplosiveIdt->num = SAL_MIN(stExplosiveArea.nNum, XSP_IDENTIFY_NUM);
        for (i = 0; i < pstXspExplosiveIdt->num; i++)
        {
            pstXspExplosiveIdt->rect[i].uiX = stExplosiveArea.CMRect[i].nLeft;
            pstXspExplosiveIdt->rect[i].uiY = stExplosiveArea.CMRect[i].nBottom;
            pstXspExplosiveIdt->rect[i].uiWidth = SAL_SUB_SAFE(stExplosiveArea.CMRect[i].nRight, stExplosiveArea.CMRect[i].nLeft);
            pstXspExplosiveIdt->rect[i].uiHeight = SAL_SUB_SAFE(stExplosiveArea.CMRect[i].nTop, stExplosiveArea.CMRect[i].nBottom);
        }
    }
    else
    {
        XSP_LOGW("chan %d, libXRay_ConcernedArea failed, hr: 0x%x\n", idx, hr);
        pstXspSusIdt->num = 0;
        pstXspUnpenIdt->num = 0;
        pstXspExplosiveIdt->num = 0;
    }

    /*调试打印*/
    XSP_LOGD("material num %d -> sus num %d, low_penetration num %d -> unpen num %d, stExplosiveArea num %d -> pstXspExplosiveIdt num %d.\n",
             concerd_material.nNum,
             pstXspSusIdt->num,
             low_penetration.nNum,
             pstXspUnpenIdt->num, 
             stExplosiveArea.nNum, 
             pstXspExplosiveIdt->num);
    for (i = 0; i < pstXspSusIdt->num; i++)
    {
        XSP_LOGT("pack w %d, h %d, sus_indentify num %d, id %d: [%d, %d, %d, %d]->[%d, %d, %d, %d].\n",
                 stXrayLibImg.width,
                 stXrayLibImg.height,
                 pstXspSusIdt->num,
                 i,
                 concerd_material.CMRect[i].nLeft,
                 concerd_material.CMRect[i].nRight,
                 concerd_material.CMRect[i].nTop,
                 concerd_material.CMRect[i].nBottom,
                 pstXspSusIdt->rect[i].uiX,
                 pstXspSusIdt->rect[i].uiY,
                 pstXspSusIdt->rect[i].uiWidth,
                 pstXspSusIdt->rect[i].uiHeight);
    }

    for (i = 0; i < pstXspUnpenIdt->num; i++)
    {
        XSP_LOGT("pack w %d, h %d, unpen_indentify num %d, id %d:[%d, %d, %d, %d]->[%d, %d, %d, %d].\n",
                 stXrayLibImg.width,
                 stXrayLibImg.height,
                 pstXspUnpenIdt->num,
                 i,
                 low_penetration.CMRect[i].nLeft,
                 low_penetration.CMRect[i].nRight,
                 low_penetration.CMRect[i].nTop,
                 low_penetration.CMRect[i].nBottom,
                 pstXspUnpenIdt->rect[i].uiX,
                 pstXspUnpenIdt->rect[i].uiY,
                 pstXspUnpenIdt->rect[i].uiWidth,
                 pstXspUnpenIdt->rect[i].uiHeight);
    }

    for (i = 0; i < pstXspExplosiveIdt->num; i++)
    {
        XSP_LOGT("pack w %d, h %d, explosive_indentify num %d, id %d:[%d, %d, %d, %d]->[%d, %d, %d, %d].\n",
                 stXrayLibImg.width,
                 stXrayLibImg.height,
                 pstXspExplosiveIdt->num,
                 i,
                 stExplosiveArea.CMRect[i].nLeft,
                 stExplosiveArea.CMRect[i].nRight,
                 stExplosiveArea.CMRect[i].nTop,
                 stExplosiveArea.CMRect[i].nBottom,
                 pstXspExplosiveIdt->rect[i].uiX,
                 pstXspExplosiveIdt->rect[i].uiY,
                 pstXspExplosiveIdt->rect[i].uiWidth,
                 pstXspExplosiveIdt->rect[i].uiHeight);
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_realtime_process
 * @brief:      回拉处理流程
 * @param[in]:  void *p_handle                                            算法句柄
 * @param[in]:  XSP_XRAW_PLAYBACK_NORMALIZED st_xraw_playback_normalized  回拉的归一化参数和数据
 * @param[out]: XSP_YUV *pst_yuv                                          回拉的YUV数据
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_playback_process(void *p_handle, XSP_PBPROC_IN *pst_pbproc_in, XSP_PBPROC_OUT *pstPbProcOut)
{
    S32 idx = 0;
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_IMAGE stXrayLibRaw = {0};
    XRAYLIB_IMAGE stXrayLibMerge = {0};
    XRAYLIB_IMAGE stXrayLibDisp = {0};
    XIMAGE_DATA_ST *pstBlendIn = NULL;
    XIMAGE_DATA_ST *pstNrawIn = NULL;
    XRAYLIB_IMG_CHANNEL img_chan = 0;

    CHECK_PTR_NULL(p_handle, SAL_FAIL);
    CHECK_PTR_NULL(pst_pbproc_in, SAL_FAIL);
    CHECK_PTR_NULL(pstPbProcOut, SAL_FAIL);

    idx = rin_native_get_handle_idx(p_handle);
    if (idx < 0)
    {
        XSP_LOGE("the p_handle[%p] is invalid\n", p_handle);
        return SAL_FAIL;
    }

    pstNrawIn = &pst_pbproc_in->stNrawIn;
    pstBlendIn = &pst_pbproc_in->stBlendIn;

    CHECK_PTR_NULL(pstBlendIn->pPlaneVir[0], SAL_FAIL);
    CHECK_PTR_NULL(pstNrawIn->pPlaneVir[0], SAL_FAIL);
    if (DSP_IMG_DATFMT_LHZP == pstNrawIn->enImgFmt)
    {
        CHECK_PTR_NULL(pstNrawIn->pPlaneVir[1], SAL_FAIL);
        CHECK_PTR_NULL(pstNrawIn->pPlaneVir[2], SAL_FAIL);
    }
    CHECK_PTR_NULL(pstPbProcOut->pPlaneVir[0], SAL_FAIL);
    if (pstPbProcOut->enImgFmt & DSP_IMG_DATFMT_YUV420_MASK)
    {
        CHECK_PTR_NULL(pstPbProcOut->pPlaneVir[1], SAL_FAIL);
    }

    rin_native_2xraylibimg(pstNrawIn, &stXrayLibRaw, __FUNCTION__, __LINE__);
    rin_native_2xraylibimg(pstBlendIn, &stXrayLibMerge, __FUNCTION__, __LINE__);
    rin_native_2xraylibimg(pstPbProcOut, &stXrayLibDisp, __FUNCTION__, __LINE__);

    /* p_gray_fuse作为输入buffer数据有效，则使用融合灰度图 */
    if (pst_pbproc_in->bUseBlend)
    {
        img_chan = XRAYLIB_IMG_CHANNEL_MERGE;
    }
    else
    {
        img_chan = XRAYLIB_IMG_CHANNEL_HIGHLOW;
    }

    hr = libXRay_GetImage(p_handle, &stXrayLibRaw, &stXrayLibMerge, &stXrayLibDisp, img_chan, pst_pbproc_in->neighbour_top, pst_pbproc_in->neighbour_bottom);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGW("chan %d, libXRay_GetImage failed, hr: 0x%x, w: %d, h: %d\n",
                 rin_native_get_handle_idx(p_handle), hr, stXrayLibDisp.width, stXrayLibDisp.height);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/******************************************************************
 * @function:   rin_native_transform_process
 * @brief:      转存处理流程
 * @param[in]:  void *p_handle                          算法句柄
 * @param[in]:  XSP_XRAW_NORMALIZED st_xraw_normalized  转存的归一化数据
 * @param[out]: XSP_YUV *pst_yuv                        转存的YUV数据
 * @return:     SAL_STATUS  SAL_SOK 成功
 *                          其它      失败
 *******************************************************************/
SAL_STATUS rin_native_transform_process(void *p_handle, XSP_TRANS_IN *pstTransIn, XSP_TRANS_OUT *pstTransOut)
{
    S32 idx = 0;
    XRAY_LIB_HRESULT hr = XRAY_LIB_OK;
    XRAYLIB_IMAGE stXrayLibRaw = {0};
    XRAYLIB_IMAGE stXrayLibMerge = {0};
    XRAYLIB_IMAGE stXrayLibDisp = {0};
    XIMAGE_DATA_ST *pstNrawIn = NULL;
    XRAYLIB_IMG_CHANNEL img_chan = 0;

    CHECK_PTR_NULL(p_handle, SAL_FAIL);
    CHECK_PTR_NULL(pstTransIn, SAL_FAIL);
    CHECK_PTR_NULL(pstTransOut, SAL_FAIL);

    idx = rin_native_get_handle_idx(p_handle);
    if (idx < 0)
    {
        XSP_LOGE("the p_handle[%p] is invalid\n", p_handle);
        return SAL_FAIL;
    }

    pstNrawIn = &pstTransIn->stNrawIn;
    CHECK_PTR_NULL(pstNrawIn->pPlaneVir[0], SAL_FAIL);
    if (DSP_IMG_DATFMT_LHZP == pstNrawIn->enImgFmt)
    {
        CHECK_PTR_NULL(pstNrawIn->pPlaneVir[1], SAL_FAIL);
        CHECK_PTR_NULL(pstNrawIn->pPlaneVir[2], SAL_FAIL);
    }
    CHECK_PTR_NULL(pstTransOut->pPlaneVir[0], SAL_FAIL);
    if (DSP_IMG_DATFMT_YUV420SP_VU == pstTransOut->enImgFmt)
    {
        CHECK_PTR_NULL(pstTransOut->pPlaneVir[1], SAL_FAIL);
    }

    rin_native_2xraylibimg(pstNrawIn, &stXrayLibRaw, __FUNCTION__, __LINE__);
    rin_native_2xraylibimg(&pstTransIn->stBlendIn, &stXrayLibMerge, __FUNCTION__, __LINE__);
    rin_native_2xraylibimg(pstTransOut, &stXrayLibDisp, __FUNCTION__, __LINE__);
    
    img_chan = XRAYLIB_IMG_CHANNEL_HIGHLOW; /* 转存处理使用高低能数据，不使用融合灰度图 */

    hr = libXRay_GetImage(p_handle, &stXrayLibRaw, &stXrayLibMerge, &stXrayLibDisp, img_chan, 0, 0);
    if (hr != XRAY_LIB_OK)
    {
        XSP_LOGW("chan %d, libXRay_GetImage failed, hr: 0x%x, w: %d, h: %d\n",
                 rin_native_get_handle_idx(p_handle), hr, stXrayLibDisp.width, stXrayLibDisp.height);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

