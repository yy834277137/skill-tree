/*******************************************************************************
* face_drv.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : zongkai5 <zongkai5@hikvision.com>
* Version: V2.0.0  2021年10月30日 Create
*
* Description :
* Modification:
*******************************************************************************/


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include "sal.h"
#include "face_drv.h"
#include "dup_tsk.h"

/* ========================================================================== */
/*                             宏定义区                                       */
/* ========================================================================== */
#define FACE_DRV_CHECK_CHAN(chan, value)	{if (chan > FACE_MAX_CHAN) {FACE_LOGE("Input FACE Chan[%d] is Invalid!\n", chan); return (value); }}
#define FACE_DRV_CHECK_PRM(ptr, value)		{if (!ptr) {FACE_LOGE("Ptr (The address is empty or Value is 0 )\n"); return (value); }}

#define RESULT_PATH_FD_CMP		"./face/output/rcg_result.txt"
#define RESULT_ROOT				"./face/output/feature/"
#define CAP_FACE_PIC_ROOT		"/home/config/face/"
#define GET_FRAME_DATA_FAIL_CNT 120

#define FACE_DRV_CHECK_RET(ret, loop, str) \
    { \
        if (ret) \
        { \
            FACE_LOGE("%s, ret: 0x%x \n", str, ret); \
            goto loop; \
        } \
    }

/* ========================================================================== */
/*                             数据结构区                                     */
/* ========================================================================== */
static FACE_COMMON g_face_common = {0};

#ifdef DEBUG_FACE_LOG
static FILE *g_fp_res = NULL;
#endif
/* ========================================================================== */
/*                             函数定义区                                     */
/* ========================================================================== */

/**
 * @function    Face_DrvGetDev
 * @brief         获取全局变量
 * @param[in] chan: 通道号
 * @param[out]
 * @return
 */
FACE_DEV_PRM *Face_DrvGetDev(UINT32 chan)
{
    FACE_DRV_CHECK_CHAN(chan, NULL);

    /* 暂时只有一个人脸处理通道 */
    return &g_face_common.stFace_dev[0];   /* &g_face_common.stFace_dev[chan]; */
}

/**
 * @function    Face_DrvStartCap
 * @brief         开启抓拍命令
 * @param[in]  chan - 通道号
 * @param[out] NULL
 * @return    SAL_SOK
 */
INT32 Face_DrvStartCap(UINT32 chan)
{
    FACE_DEV_PRM *pstFace_dev = NULL;

    /* 入参有效性判断 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstFace_dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_dev, SAL_FAIL);

    /* 通道未初始化，直接返回失败 ,先初始化通道*/
    if (SAL_TRUE != pstFace_dev->uiChnStatus)
    {
        FACE_LOGE("chan %d is not started yet! Pls Check!\n", chan);
        return SAL_FAIL;
    }

    if (pstFace_dev->bUpFrmThStart == SAL_TRUE)
    {
        FACE_LOGW("Cap status already Setted!!\n");
        return SAL_SOK;
    }

    pstFace_dev->bUpFrmThStart = SAL_TRUE;

    FACE_LOGI("Face Cap start!\n");
    return SAL_SOK;
}

/**
 * @function    Face_DrvStopCap
 * @brief         关闭抓拍命令
 * @param[in]  chan
 * @param[out] NULL
 * @return    SAL_SOK
 */
INT32 Face_DrvStopCap(UINT32 chan)
{
    FACE_DEV_PRM *pstFace_dev = NULL;

    /* 入参有效性判断 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstFace_dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_dev, SAL_FAIL);

    /* 通道未初始化，直接返回失败 ,先初始化通道*/
    if (SAL_TRUE != pstFace_dev->uiChnStatus)
    {
        FACE_LOGE("chan %d is not started yet! Pls Check!\n", chan);
        return SAL_FAIL;
    }

    if (pstFace_dev->bUpFrmThStart == SAL_FALSE)
    {
        FACE_LOGW("Cap status already Stopped!!\n");
        return SAL_SOK;
    }

    pstFace_dev->bUpFrmThStart = SAL_FALSE;
    FACE_LOGI("Face Cap close!\n");
    return SAL_SOK;
}

/**
 * @function    Face_DrvGetVersion
 * @brief         获取人脸版本信息
 * @param[in]  *pParam 版本参数
 * @param[out] *pParam 版本参数
 * @return  SAL_SOK
 */
INT32 Face_DrvGetVersion(void *pParam)
{
    INT32 s32Ret = SAL_FAIL;

    FACE_COMMON_PARAM *pstComPrm = NULL;
    FACE_VERSION_DATA *pstAppVerInfo = NULL;

    FACE_DRV_CHECK_PRM(pParam, SAL_FAIL);

    pstComPrm = Face_HalGetComPrm();

    /* 判断引擎是否初始化 */
    if (SAL_TRUE != pstComPrm->bInit)
    {
        FACE_LOGE("Face Engine is not initialized! return fail!! \n");
        return SAL_FAIL;
    }
    
    pstAppVerInfo = &pstComPrm->stVersionInfo.stAppVerInfo;

    s32Ret = snprintf(pParam, FACE_MAX_VERESION_LENGTH, "AppVersion: %d_%d_%d %d_%d_%d",
                      pstAppVerInfo->nMajorVersion, pstAppVerInfo->nSubVersion, pstAppVerInfo->nRevisVersion,
                      pstAppVerInfo->nVersionYear, pstAppVerInfo->nVersionMonth, pstAppVerInfo->nVersionDay);
    if (s32Ret < 0)
    {
        FACE_LOGE("Get Version Failed!\n");
        return SAL_FAIL;
    }

    FACE_LOGI("AppVersion: %d.%d.%d buildTime:%d_%d_%d\n",
              pstAppVerInfo->nMajorVersion, pstAppVerInfo->nSubVersion, pstAppVerInfo->nRevisVersion,
              pstAppVerInfo->nVersionYear, pstAppVerInfo->nVersionMonth, pstAppVerInfo->nVersionDay);

    return SAL_SOK;
}

/**
 * @function    Face_DrvGetFeatureVersionRst
 * @brief         用于特征数据版本获取接口
 * @param[in]  pFeatData
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_DrvGetFeatureVersionRst(void *pFaceCmpData)
{
    INT32 s32Ret = SAL_SOK;

    FACE_COMPARE_CHECK_DATA *pstFaceCmpData = NULL;
    FACE_COMMON_PARAM *pstComPrm = NULL;

    /* checker */
    FACE_DRV_CHECK_RET(NULL == pFaceCmpData, err, "pFaceCmpData NULL!");

    pstFaceCmpData = (FACE_COMPARE_CHECK_DATA *)pFaceCmpData;

    FACE_LOGI("from app: start face feature version check! \n");

    pstComPrm = Face_HalGetComPrm();
    FACE_DRV_CHECK_RET(NULL == pstComPrm, err, "pstComPrm NULL!");

    /* 判断引擎是否初始化 */
    if (SAL_TRUE != pstComPrm->bInit)
    {
        FACE_LOGE("Face Engine is not initialized! return fail!! \n");
        goto err;
    }

    if (NULL == pstComPrm->pExternCompare)
    {
        FACE_LOGE("extern compare lib handle err! %p \n", pstComPrm->pExternCompare);
        goto err;
    }

    s32Ret = SAE_FACE_DFR_Compare_Extern_CheckFeature(pstComPrm->pExternCompare,
                                                      (UINT8 *)pstFaceCmpData->stTestFace,
                                                      FACE_FEATURE_LENGTH);
    FACE_DRV_CHECK_RET(s32Ret, err, "extern complib check feature failed!");

    pstFaceCmpData->bCheckRst = SAL_TRUE;
    return SAL_SOK;

err:
	if (NULL != pstFaceCmpData)
	{
    	pstFaceCmpData->bCheckRst = SAL_FALSE;
	}
    return SAL_FAIL;
}

/**
 * @function    Face_DrvClearDataBase
 * @brief         清空本地数据库
 * @param[in]  NULL
 * @param[out] NULL
 * @return         SAL_SOK
 */
UINT32 Face_DrvClearDataBase(void)
{
    UINT32 i = 0;
    FACE_MODEL_DATA_BASE *pstDataBase = NULL;

    pstDataBase = Face_HalGetDataBase();

    pstDataBase->uiModelCnt = 0;
    for (i = 0; i < pstDataBase->uiMaxModelCnt; i++)
    {
        memset((char *)pstDataBase->pFeatureData[i], 0, FACE_TEATURE_HEADER_LENGTH + FACE_FEATURE_LENGTH);
    }

    FACE_LOGI("Clear Local Data Base Success!\n");

    return SAL_SOK;
}

/**
 * @function    Face_DrvUpdateDataBase
 * @brief         更新本地数据库信息
 * @param[in]  chan  : 通道号
 * @param[in]  pParam: 更新数据库的参数
 * @param[out] NULL
 * @return  SAL_SOK
 */
INT32 Face_DrvUpdateDataBase(UINT32 chan, void *pParam)
{
	INT32 s32Ret = 0; 
	UINT32 i = 0;
    UINT32 uiObjCnt = 0;

    FACE_DSP_DATABASE_PARAM *pstObjData = NULL;
    FACE_DSP_UPDATE_PARAM *pstUpdateParam = NULL;
    FACE_MODEL_DATA_BASE *pstDataBase = NULL;

	FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    /* 入参有效性判断 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pParam, SAL_FAIL);

    FACE_LOGI("Face_DrvUpdateDataBase! chan %d \n", chan);

    pstUpdateParam = (FACE_DSP_UPDATE_PARAM *)pParam;
    uiObjCnt = pstUpdateParam->uiModelCnt;

    pstFaceHalComm = Face_HalGetComPrm();

    /* 判断引擎是否初始化 */
    if (SAL_TRUE != pstFaceHalComm->bInit)
    {
        FACE_LOGE("Face Engine is not initialized! return fail!! \n");
        return SAL_FAIL;
    }

    pstDataBase = Face_HalGetDataBase();

    FACE_LOGI("chan %d CurModelCnt %d MaxModelCnt %d \n",
              chan, pstDataBase->uiModelCnt, pstDataBase->uiMaxModelCnt);

    Face_DrvClearDataBase();

    for (i = 0; i < uiObjCnt && i < MAX_FACE_NUM; i++)
    {
        pstObjData = &pstUpdateParam->stFeatData[i];
		
		/* 校验特征数据是否满足当前extern compare版本 */
		s32Ret = SAE_FACE_DFR_Compare_Extern_CheckFeature(pstFaceHalComm->pExternCompare,
														  &pstObjData->Featdata[0], FACE_FEATURE_LENGTH);
		if (SAL_SOK != s32Ret)
		{
			FACE_LOGE("SAE_FACE_DFR_Compare_Extern_CheckFeature failed! s32Ret:%d",s32Ret);
			return SAL_FAIL;
		}
		
        /* 数据库中数据由数据头(包含人脸ID信息)和特征数据组成 */
        memcpy(pstDataBase->pFeatureData[i], &pstObjData->uiFaceId, FACE_TEATURE_HEADER_LENGTH);               /* 拷贝数据头 */
        memcpy((char *)(pstDataBase->pFeatureData[i]) + FACE_TEATURE_HEADER_LENGTH, &pstObjData->Featdata[0], FACE_FEATURE_LENGTH); /* 拷贝特征数据 */
		pstDataBase->uiModelCnt++;

#if 0  /* 保存人脸建模数据 */
		Face_HalDumpFaceFeature((char *)(pstDataBase->pFeatureData[i]) + FACE_TEATURE_HEADER_LENGTH, 272, "/home/config/update_dump", i);
#endif

        FACE_LOGI("i %d faceId %d\n", i, pstObjData->uiFaceId);
    }

    FACE_LOGI("Face Module: chan %d Update DataBase End! \n", chan);
    return SAL_SOK;
}

/**
 * @function    Face_DrvUpdateCompLib
 * @brief         更新人脸比对库
 * @param[in] chan  : 通道号
 * @param[in]  pParam: 更新数据
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_DrvUpdateCompLib(UINT32 chan, void *pParam)
{
    UINT32 s32Ret = SAL_FAIL;

    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pParam, SAL_FAIL);

    s32Ret = Face_HalUpdateCompareLib();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Face Module: chan %d Update Compare Lib Fail!\n", chan);
        return SAL_FAIL;
    }

    FACE_LOGI("Face Module: chan %d Update Compare Lib OK!\n", chan);
    return SAL_SOK;
}

#ifdef DEBUG_FACE_LOG

/**
 * @function    Face_DrvFDQualityDebugTest
 * @brief         人脸FD质量输出结果测试接口
 * @param[in]  算法输出数据
 * @param[out] NULL
 * @return SALSOK
 */
static VOID Face_DrvFDQualityDebugTest(VOID *pstOutput)
{
    SAE_FACE_OUT_DATA_DFR_QLTY_T *pstDFRQualityOut = NULL;

    s32Ret = Face_HalGetComPrm()->stIcfFuncP.IcfGetPackageStatus(pstOutput);
    if (SAL_SOK != s32Ret)
    {
        goto exit;
    }

    /* FD Quality结果 */
    pstDFRQualityOut = (SAE_FACE_OUT_DATA_DFR_QLTY_T *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput,
                                                                                                       ICF_ANA_DFR_QUALITY_DATA);
    FACE_DRV_CHECK_RET(NULL == pstDFRQualityOut, exit, "pDFRQualityOut == null!");

    FACE_LOGI("module sts: %d, face_0 %d \n", pstDFRQualityOut->module_sts, pstDFRQualityOut->quality_face_num[0]);

    for (int j = 0; j < pstDFRQualityOut->quality_face_num[0]; j++)
    {
        fprintf(g_fp_res, "quality out: %d\n", fd_qty_out->quality_out[j].face_num);
        for (int i = 0; i < fd_qty_out->quality_out[j].face_num; i++)
        {
            /* 打印有效信息, 结构体中其他成员无效 */
            fprintf(g_fp_res, "    score: %f: pitch: %f yaw: %f clearity: %f invisible: %d cls: %d illumination: %d\n",
                    fd_qty_out->quality_out[j].face_info[i].face_score,        /* 人脸评分 */
                    fd_qty_out->quality_out[j].face_info[i].pose_pitch,
                    fd_qty_out->quality_out[j].face_info[i].pose_yaw,
                    fd_qty_out->quality_out[j].face_info[i].clearity_score,
                    fd_qty_out->quality_out[j].face_info[i].invisible_cls,     /* 遮挡 */
                    fd_qty_out->quality_out[j].face_info[i].face_cls,          /* 人脸非人脸   老版本还是3分类，测试的时候特殊处理 */
                    fd_qty_out->quality_out[j].face_info[i].illumination_cls); /* 光照类别 */
        }
    }

exit:
    return;
}

/**
 * @function    Face_DrvTarckDebugTest
 * @brief         人脸算法跟踪输出节点测试接口
 * @param[in]  算法输出结果数据
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvTarckDebugTest(void *pstAlgDataPacket)
{
    FRE_FD_TRK_OUT_T *trk_out = (FRE_FD_TRK_OUT_T *)ICF_GetDataPtrFromPkg(pstAlgDataPacket, ICF_ANA_FDTRK_DATA);

    ICF_CHECK_ERROR(NULL == trk_out, -1);

    for (int j = 0; j < trk_out->image_num; j++)
    {
        fprintf(g_fp_res, "track out: %d\n", trk_out->track_out[j].face_num);
        for (int n = 0; n < trk_out->track_out[j].face_num; n++)
        {
            fprintf(g_fp_res, "    ID: %d rect: %f %f %f %f conf: %f\n",
                    trk_out->track_out[j].face_info[n].id,
                    trk_out->track_out[j].face_info[n].rect.x,
                    trk_out->track_out[j].face_info[n].rect.y,
                    trk_out->track_out[j].face_info[n].rect.width,
                    trk_out->track_out[j].face_info[n].rect.height,
                    trk_out->track_out[j].face_info[n].confidence);
        }

        fprintf(g_fp_res, "\n");
        fflush(g_fp_res);
    }

    return SAL_SOK;
}

/**
 * @function    Face_DrvFDDetectDebugTest
 * @brief         人脸FD检测输出测试接口
 * @param[in]  算法FD回调结果
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvFDDetectDebugTest(void *pstAlgDataPacket)
{
    FRE_DET_OUT_T *det_out = (FRE_DET_OUT_T *)ICF_GetDataPtrFromPkg(pstAlgDataPacket, ICF_ANA_FDDET_DATA);

    ICF_CHECK_ERROR(NULL == det_out, -1);

    for (int j = 0; j < det_out->image_num; j++)
    {
        fprintf(g_fp_res, "detect out: %d\n", det_out->detect_out[j].face_num);
        for (int i = 0; i < det_out->detect_out[j].face_num; i++)
        {
            fprintf(g_fp_res, "    rect: %f %f %f %f conf: %f\n",
                    det_out->detect_out[j].face_info[i].rect.x,
                    det_out->detect_out[j].face_info[i].rect.y,
                    det_out->detect_out[j].face_info[i].rect.width,
                    det_out->detect_out[j].face_info[i].rect.height,
                    det_out->detect_out[j].face_info[i].confidence);
        }

        fprintf(g_fp_res, "\n");
    }

    fflush(g_fp_res);

    return SAL_SOK;

}

/**
 * @function    Face_DrvSelectDebugTest
 * @brief         人脸抓拍选帧结果测试接口
 * @param[in] 回调输出算法结果 pstTestName:写文件名称
 * @param[out] NULL
 * @return  SAL_SOK
 */
static INT32 Face_DrvSelectDebugTest(void *pstAlgDataPacket, char *pstTestName)
{
    FRE_SEL_FRAME_OUT_T *sel_out = (FRE_SEL_FRAME_OUT_T *)ICF_GetDataPtrFromPkg(pstAlgDataPacket, ICF_ANA_FDSEL_DATA);

    ICF_CHECK_ERROR(NULL == sel_out, -1);

    for (int n = 0; n < sel_out->image_num; n++)
    {
        static int frame_num = 0;               /* 为了计数, 静态变量不是强需求 */
        char filename[1024] = { 0 };
#if 1
        /* 开启保存抠图 */
        sprintf(filename,
                "%s%s%s%d_%dx%d%s", "./face/output/",
                pstTestName,
                "_crop_",
                frame_num,
                sel_out->sel_out[n].crop_image.yuv_data.image_w,
                sel_out->sel_out[n].crop_image.yuv_data.image_h,
                ".nv21");
        FILE *fp = fopen(filename, "wb");

        fwrite(sel_out->sel_out[n].crop_image.yuv_data.y,
               sel_out->sel_out[n].crop_image.yuv_data.image_w
               * sel_out->sel_out[n].crop_image.yuv_data.image_h,
               1,
               fp);
        fwrite(sel_out->sel_out[n].crop_image.yuv_data.u,
               sel_out->sel_out[n].crop_image.yuv_data.image_w
               * sel_out->sel_out[n].crop_image.yuv_data.image_h
               * 1 / 2,
               1,
               fp);

        sprintf(filename,
                "%s%s%s%d_%dx%d%s", "./face/output/",
                pstTestName,
                "_src_",
                frame_num,
                sel_out->sel_out[n].proc_info.image_info.yuv_data.image_w,
                sel_out->sel_out[n].proc_info.image_info.yuv_data.image_h,
                ".nv21");

        FILE *fp1 = fopen(filename, "wb");
        fwrite(sel_out->sel_out[n].proc_info.image_info.yuv_data.y,
               sel_out->sel_out[n].proc_info.image_info.yuv_data.image_w
               * sel_out->sel_out[n].proc_info.image_info.yuv_data.image_h,
               1,
               fp1);
        fwrite(sel_out->sel_out[n].proc_info.image_info.yuv_data.u,
               sel_out->sel_out[n].proc_info.image_info.yuv_data.image_w
               * sel_out->sel_out[n].proc_info.image_info.yuv_data.image_h
               * 1 / 2,
               1,
               fp1);
#else
        /* 开启保存抠图 */
        sprintf(filename,
                "%s%s%s%d_%dx%d%s",
                "./../../data/output/",
                pstTestName,
                "_crop_",
                frame_num,
                sel_out->sel_out[n].crop_image.yuv_data.image_w,
                sel_out->sel_out[n].crop_image.yuv_data.image_h,
                ".bgrg");
        FILE *fp = fopen(filename, "wb");

        fwrite(sel_out->sel_out[n].crop_image.yuv_data.b,
               3 * sel_out->sel_out[n].crop_image.yuv_data.image_w
               * sel_out->sel_out[n].crop_image.yuv_data.image_h,
               1,
               fp);
        fwrite(sel_out->sel_out[n].crop_image.yuv_data.y,
               1 * sel_out->sel_out[n].crop_image.yuv_data.image_w
               * sel_out->sel_out[n].crop_image.yuv_data.image_h,
               1,
               fp);

        sprintf(filename,
                "%s%s%s%d_%dx%d%s",
                "./../../data/output/",
                pstTestName,
                "_src_",
                frame_num,
                sel_out->sel_out[n].proc_info.image_info.yuv_data.image_w,
                sel_out->sel_out[n].proc_info.image_info.yuv_data.image_h,
                ".bgrg");

        FILE *fp1 = fopen(filename, "wb");
        fwrite(sel_out->sel_out[n].proc_info.image_info.yuv_data.b,
               3 * sel_out->sel_out[n].proc_info.image_info.yuv_data.image_w
               * sel_out->sel_out[n].proc_info.image_info.yuv_data.image_h,
               1,
               fp1);
        fwrite(sel_out->sel_out[n].proc_info.image_info.yuv_data.y,
               1 * sel_out->sel_out[n].proc_info.image_info.yuv_data.image_w
               * sel_out->sel_out[n].proc_info.image_info.yuv_data.image_h,
               1,
               fp1);
#endif
        fclose(fp);
        fclose(fp1);
        frame_num++;
    }

    return SAL_SOK;
}

/**
 * @function    Face_DrvAttributeDebugTest
 * @brief         人脸属性输出测试接口
 * @param[in]  算法输出结果
 * @param[out] NULL
 * @return  SAL_SOK
 */
static INT32 Face_DrvAttributeDebugTest(void *pstAlgDataPacket)
{
    FRE_ATTRIBUTE_OUT_T *attr = (FRE_ATTRIBUTE_OUT_T *)ICF_GetDataPtrFromPkg(pstAlgDataPacket, ICF_ANA_DFRATT_DATA);

    ICF_CHECK_ERROR(NULL == attr, -1);

    fprintf(g_fp_res, "attribute image num: %d\n", attr->image_num);
    for (int j = 0; j < attr->image_num; j++)
    {
        /* Attribute结果 */
        fprintf(g_fp_res, "    ret: %8x, glass: %d %f, hat:%d %f, mask:%d %f, beard:%d %f, gender:%d %f, race:%d %f, nation:%d %f, express:%d %f, age:%f %f\n",
                attr->attribute_sts[j],
                attr->attribute_out[j].face_info.glass.predict_label,
                attr->attribute_out[j].face_info.glass.confidence[attr->attribute_out[j].face_info.glass.predict_label],
                attr->attribute_out[j].face_info.hat.predict_label,
                attr->attribute_out[j].face_info.hat.confidence[attr->attribute_out[j].face_info.hat.predict_label],
                attr->attribute_out[j].face_info.mask.predict_label,
                attr->attribute_out[j].face_info.mask.confidence[attr->attribute_out[j].face_info.mask.predict_label],
                attr->attribute_out[j].face_info.beard.predict_label,
                attr->attribute_out[j].face_info.beard.confidence[attr->attribute_out[j].face_info.beard.predict_label],
                attr->attribute_out[j].face_info.gender.predict_label,
                attr->attribute_out[j].face_info.gender.confidence[attr->attribute_out[j].face_info.gender.predict_label],
                attr->attribute_out[j].face_info.race.predict_label,
                attr->attribute_out[j].face_info.race.confidence[attr->attribute_out[j].face_info.race.predict_label],
                attr->attribute_out[j].face_info.nation.predict_label,
                attr->attribute_out[j].face_info.nation.confidence[attr->attribute_out[j].face_info.nation.predict_label],
                attr->attribute_out[j].face_info.express.predict_label,
                attr->attribute_out[j].face_info.express.confidence[attr->attribute_out[j].face_info.express.predict_label],
                attr->attribute_out[j].face_info.age.predict_label,
                attr->attribute_out[j].face_info.age.confidence[(int)attr->attribute_out[j].face_info.age.predict_label]);
    }

    return SAL_SOK;
}

/**
 * @function    Face_DrvFeatureDebugTest
 * @brief         人脸特征数据输出测试接口
 * @param[in]   算法回调结果输出
 * @param[out] NULL
 * @return  SAL_SOK
 */
static INT32 Face_DrvFeatureDebugTest(void *pstAlgDataPacket)
{
    FRE_FEATURE_OUT_T *feature = (FRE_FEATURE_OUT_T *)ICF_GetDataPtrFromPkg(pstAlgDataPacket, ICF_ANA_DFRFEA_DATA);

    ICF_CHECK_ERROR(NULL == feature, -1);

    for (int j = 0; j < feature->image_num; j++)
    {
        if (0 != feature->feat_sts[j])
        {
            continue;
        }

        /* Feature结果 */
        char feat_temp[300] = { 0 };
        static int proc_num = 0;
        sprintf(feat_temp, "%s%s%d%s", RESULT_ROOT, "rcg_feature", proc_num + 1, ".bin");

        FILE *fp_feat = fopen(feat_temp, "wb");
        fwrite(feature->feat_out[j].face_info.feat_data, sizeof(char), feature->feat_out[j].face_info.feat_len, fp_feat);
        fclose(fp_feat);

        proc_num++;
    }

    return SAL_SOK;
}

/**
 * @function    Face_DrvLandMarkDebugTest
 * @brief         人脸关键点输出测试接口
 * @param[in]  算法回调结果数据
 * @param[out] NULL
 * @return  SAL_SOK
 */
static int Face_DrvLandMarkDebugTest(void *pstAlgDataPacket)
{
    FRE_LANDMARK_OUT_T *lms_list = (FRE_LANDMARK_OUT_T *)ICF_GetDataPtrFromPkg(pstAlgDataPacket, ICF_ANA_DFRLAN_DATA);

    ICF_CHECK_ERROR(NULL == lms_list, -1);

    fprintf(g_fp_res, "landmark image num: %d\n", lms_list->image_num);
    for (int j = 0; j < lms_list->image_num; j++)
    {
        /* Landmark结果 */
        fprintf(g_fp_res, "    ret: %8x,pts:%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n",
                lms_list->landmark_sts[j],
                lms_list->landmark_out[j].face_info.landmark[0].x,
                lms_list->landmark_out[j].face_info.landmark[0].y,
                lms_list->landmark_out[j].face_info.landmark[1].x,
                lms_list->landmark_out[j].face_info.landmark[1].y,
                lms_list->landmark_out[j].face_info.landmark[2].x,
                lms_list->landmark_out[j].face_info.landmark[2].y,
                lms_list->landmark_out[j].face_info.landmark[3].x,
                lms_list->landmark_out[j].face_info.landmark[3].y,
                lms_list->landmark_out[j].face_info.landmark[4].x,
                lms_list->landmark_out[j].face_info.landmark[4].y);
        fflush(g_fp_res);
    }

    return 0;

}

#endif

/**
 * @function    Face_DrvGetQueBuf
 * @brief         获取空闲队列buf
 * @param[in]  DSA_QueHndl *pstBufQue-队列
 * @param[in]  void **ppstYuvFrame-队列数据
 * @param[out]
 * @return SAL_SOK
 */
static INT32 Face_DrvGetQueBuf(DSA_QueHndl *pstBufQue, void **ppstYuvFrame, UINT32 uiTimeOut)
{
    FACE_DRV_CHECK_PRM(pstBufQue, SAL_FAIL);

    DSA_QueGet(pstBufQue, (void **)ppstYuvFrame, uiTimeOut);


    return SAL_SOK;

}

/**
 * @function    Face_DrvPutQueBuf
 * @brief         回收人脸抓拍缓存buf
 * @param[in]  DSA_QueHndl *pstBufQue-队列
 * @param[in]  void **ppstYuvFrame-队列数据
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvPutQueBuf(DSA_QueHndl *pstBufQue, void *pstYuvFrame, UINT32 uiTimeOut)
{
    FACE_DRV_CHECK_PRM(pstBufQue, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pstYuvFrame, SAL_FAIL);
    DSA_QuePut(pstBufQue, (void *)pstYuvFrame, SAL_TIMEOUT_NONE);   /* 默认回收队列数据即刻返回 */

    return SAL_SOK;

}

/**
 * @function    Face_DrvGetOutputResult0
 * @brief         算法返回建模数据的回调接口,该接口为图片模式回调接口
 * @param[in]      nDataType: 数据类型
 * @param[in]      pstOutput: 输出参数
 * @param[in]      nSize    : 大小
 * @param[in]      pUsr     : 用户数据(暂未使用)
 * @param[out]
 * @return  SAL_SOK
 */
INT32 Face_DrvGetOutputResult0(int nNodeID,
                               int nCallBackType,
                               void *pstOutput,
                               unsigned int nSize,
                               void *pUsr,
                               int nUserSize)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    UINT32 u32DetectDataType = 0;
    UINT32 u32FeatureDataType = 0;

    ICF_MEDIA_INFO_V2 *pstMediaInfo = NULL;
    SAE_FACE_OUT_DATA_DFR_DET_T *pstDetectOut = NULL;
    SAE_FACE_OUT_DATA_DFR_FEATURE_T *pstFeatureOut = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    FACE_PIC_REGISTER_OUT_S *pstPicProcOut = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_RET(NULL == pstFaceHalComm, err, "pstFaceHalComm == null!");

    /* 获取图片注册业务线的全局参数 */
    pstFaceAnaBufTab = Face_HalGetAnaDataTab(FACE_PICTURE_MODE);
    FACE_DRV_CHECK_RET(NULL == pstFaceAnaBufTab, err, "pstFaceAnaBufTab == null!");

    pstPicProcOut = &pstFaceAnaBufTab->uFaceProcOut.stPicProcOut;

    FACE_DRV_CHECK_RET(NULL == pstOutput, err, "pstOutput == null!");
    s32Ret = pstFaceHalComm->stIcfFuncP.IcfGetPackageStatus(pstOutput);
    FACE_DRV_CHECK_RET(s32Ret, err, "ICF_Package_GetState failed!");

    if (nNodeID == SAE_FACE_NID_DET_FEATURE_1_POST)
    {
        u32DetectDataType = ICF_ANA_DFR_DETECT_DATA;
        u32FeatureDataType = ICF_ANA_DFR_FEATURE_DATA;
    }
    else if (nNodeID == SAE_FACE_NID_DET_FEATURE_2_POST)
    {
        u32DetectDataType = ICF_ANA_DFR_DETECT2_DATA;
        u32FeatureDataType = ICF_ANA_DFR_FEATURE2_DATA;
    }

    pstMediaInfo = (ICF_MEDIA_INFO_V2 *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput, ICF_ANA_MEDIA_DATA);
    FACE_DRV_CHECK_RET(NULL == pstMediaInfo, err, "pstMediaInfo == null!");

    /* 输出人脸检测的结果，此处仅输出可见光的人脸检测结果 */
    {
        pstDetectOut = (SAE_FACE_OUT_DATA_DFR_DET_T *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput,
                                                                                                      u32DetectDataType);
        FACE_DRV_CHECK_RET(NULL == pstDetectOut, err, "pstDetectOut == null!");

        if (SAL_SOK != pstDetectOut->module_sts)
        {
            FACE_LOGE("detect err, sts: %x \n", pstDetectOut->module_sts);
            goto err;
        }

        FACE_LOGI("face get result !! vl face sts(%x), detect_out_num(%d)\n",
                  pstDetectOut->img_sts[0], pstDetectOut->detect_face_num[0]);

        for (i = 0; i < pstDetectOut->detect_face_num[0]; i++)
        {
            FACE_LOGI("i %d, id %d, conf %f, [%f, %f] [%f, %f] \n",
                      i, pstDetectOut->detect_out[0][i].id, pstDetectOut->detect_out[0][i].confidence,
                      pstDetectOut->detect_out[0][i].rect.x,
                      pstDetectOut->detect_out[0][i].rect.y,
                      pstDetectOut->detect_out[0][i].rect.width,
                      pstDetectOut->detect_out[0][i].rect.height);
        }
    }

    /* 输出人脸建模结果 */
    {
        pstFeatureOut = (SAE_FACE_OUT_DATA_DFR_FEATURE_T *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput,
                                                                                                           u32FeatureDataType);
        FACE_DRV_CHECK_RET(NULL == pstFeatureOut, err, "pstFeatureOut == null!");

        if (SAL_SOK != pstFeatureOut->module_sts)
        {
            FACE_LOGE("feature err, sts: %x \n", pstFeatureOut->module_sts);
            goto err;
        }

        FACE_LOGI("get feature face num %d, feature data length %d \n",
                  pstFeatureOut->feat_face_num, pstFeatureOut->feat_len);

        /* szl_face_todo: 暂定找到的第一个人脸建模结果送入后处理，待进一步确认是否会有多个人脸结果返回 */
        for (i = 0; i < pstFeatureOut->feat_face_num; i++)
        {
            if (SAL_SOK != pstFeatureOut->face_sts[i])
            {
                FACE_LOGW("invalid face, skip! \n");
                continue;
            }

            FACE_LOGI("found feature! \n");
            break;
        }

        if (i >= pstFeatureOut->feat_face_num)
        {
            FACE_LOGE("not found valid feature data! \n");
            goto err;
        }

        pstPicProcOut->enProcSts = FACE_REGISTER_SUCCESS;
        pstPicProcOut->u32FeatDataLen = pstFeatureOut->feat_len;
        sal_memcpy_s(pstPicProcOut->acFeaureData, FACE_FEATURE_LENGTH,
                     pstFeatureOut->feat_data[i], pstFeatureOut->feat_len);
    }

    goto exit;

err:
    pstPicProcOut->enProcSts = FACE_REGISTER_FAIL;

exit:
    sem_post(&pstFaceAnaBufTab->sem);
    return SAL_SOK;
}

/**
 * @function   Face_DrvGetFaceRectFromLoginSelect
 * @brief      从引擎选帧结果中获取人脸目标区域
 * @param[in]  SAE_FACE_OUT_DATA_SELECT_FRAME_T *pstSelFrameInfo
 * @param[in]  FACE_VIDEO_LOGIN_SELECT_QUE_DATA_S *pstVideoCapSelQueBuf
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_DrvGetFaceRectFromLoginSelect(SAE_FACE_OUT_DATA_SELECT_FRAME_T *pstSelFrameInfo,
                                                FACE_VIDEO_LOGIN_SELECT_QUE_DATA_S *pstVideoLoginSelQueBuf)
{
    UINT32 i = 0;

    FACE_DRV_CHECK_RET(NULL == pstSelFrameInfo, err, "pstOutput == null!");
    FACE_DRV_CHECK_RET(NULL == pstVideoLoginSelQueBuf, err, "pstOutput == null!");

    /* 拷贝人脸目标区域信息 */
    pstVideoLoginSelQueBuf->u32FaceNum = pstSelFrameInfo->select_face_num;
    FACE_LOGI("get face rect num %d,  from select frame! \n", pstVideoLoginSelQueBuf->u32FaceNum);

    for (i = 0; i < pstVideoLoginSelQueBuf->u32FaceNum && i < FACE_MAX_SELECT_FACE_NUM_IN_ONE_FRAME; i++)
    {
        pstVideoLoginSelQueBuf->stFaceRec[i].x = pstSelFrameInfo->select_face_info[i].rect_src.x;
        pstVideoLoginSelQueBuf->stFaceRec[i].y = pstSelFrameInfo->select_face_info[i].rect_src.y;
        pstVideoLoginSelQueBuf->stFaceRec[i].width = pstSelFrameInfo->select_face_info[i].rect_src.width;
        pstVideoLoginSelQueBuf->stFaceRec[i].height = pstSelFrameInfo->select_face_info[i].rect_src.height;

        FACE_LOGI("i %d, get face rect [%f, %f], [%f, %f] \n",
                  i, pstVideoLoginSelQueBuf->stFaceRec[i].x,
                  pstVideoLoginSelQueBuf->stFaceRec[i].y,
                  pstVideoLoginSelQueBuf->stFaceRec[i].width,
                  pstVideoLoginSelQueBuf->stFaceRec[i].height);
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Face_DrvGetBackImgFromLoginSelect
 * @brief      从引擎结果中获取选帧背景图
 * @param[in]  SAE_FACE_OUT_DATA_SELECT_FRAME_T *pstSelFrameInfo
 * @param[in]  FACE_VIDEO_LOGIN_SELECT_QUE_DATA_S *pstVideoCapSelQueBuf
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_DrvGetBackImgFromLoginSelect(SAE_FACE_OUT_DATA_SELECT_FRAME_T *pstSelFrameInfo,
                                               FACE_VIDEO_LOGIN_SELECT_QUE_DATA_S *pstVideoLoginSelQueBuf)
{
    UINT32 u32ImgSize = 0;

    SAE_FACE_SELECT_FRAME_INFO_T *pstSelectFrame = NULL; /* 帧数据 */

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    FACE_DRV_CHECK_RET(NULL == pstSelFrameInfo, err, "pstOutput == null!");
    FACE_DRV_CHECK_RET(NULL == pstVideoLoginSelQueBuf, err, "pstOutput == null!");

    if (pstSelFrameInfo->select_face_num <= 0 || pstSelFrameInfo->select_face_num > 1)
    {
        FACE_LOGW("select frame num > 1, %d \n", pstSelFrameInfo->select_face_num);
    }

    /* 当前仅取第一张，因在配置选帧策略时，默认每个人脸id仅返回一张选帧结果 */
    pstSelectFrame = &pstSelFrameInfo->select_face_info[0];

    FACE_LOGI("get background img w %d, h %d, format %d \n",
              pstSelectFrame->image_info_src.yuv_data.image_w,
              pstSelectFrame->image_info_src.yuv_data.image_h,
              pstSelectFrame->image_info_src.yuv_data.format);

    u32ImgSize = pstSelectFrame->image_info_src.yuv_data.image_w \
                 * pstSelectFrame->image_info_src.yuv_data.image_h * 3 / 2;

    /* 拷贝图像 */
    {
        if (SAL_SOK != sys_hal_getVideoFrameInfo(&pstVideoLoginSelQueBuf->stFrame, &stVideoFrmBuf))
        {
			FACE_LOGE("get video frame info failed! \n");
			goto err;
		}
        memcpy((VOID *)stVideoFrmBuf.virAddr[0], pstSelectFrame->image_info_src.yuv_data.y, u32ImgSize);
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Face_DrvGetOutputResult1
 * @brief         人脸登录选帧回调函数
 * @param[in]
 * @param[out]
 * @return    SAL_SOK
 */
INT32 Face_DrvGetOutputResult1(int nNodeID,
                               int nCallBackType,
                               void *pstOutput,
                               unsigned int nSize,
                               void *pUsr,
                               int nUserSize)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    FACE_DEV_PRM *pstFaceDev = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    ICF_MEDIA_INFO_V2 *pstMediaInfo = NULL;
    SAE_FACE_IN_DATA_INPUT_T *pstFaceInDataInfo = NULL;
    SAE_FACE_OUT_DATA_SELECT_FRAME_T *pstSelectFrame = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    FACE_VIDEO_LOGIN_SELECT_QUE_DATA_S *pstVideoLoginSelQueBuf = NULL;

    FACE_DRV_CHECK_RET(NULL == pstOutput, err, "pstOutput == null!");

    pstFaceDev = Face_DrvGetDev(0);
    FACE_DRV_CHECK_RET(NULL == pstFaceDev, err, "pstFaceDev == null!");

    pstFullQue = pstFaceDev->stFaceVideoLoginSelQueInfo.stFaceSelQuePrm.pstFullQue;
    pstEmptQue = pstFaceDev->stFaceVideoLoginSelQueInfo.stFaceSelQuePrm.pstEmptQue;

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_RET(NULL == pstFaceHalComm, err, "pstFaceHalComm == null!");

    pstFaceAnaBufTab = Face_HalGetAnaDataTab(FACE_VIDEO_LOGIN_MODE);
    FACE_DRV_CHECK_RET(NULL == pstFaceAnaBufTab, err, "pstFaceAnaBufTab == null!");

    s32Ret = pstFaceHalComm->stIcfFuncP.IcfGetPackageStatus(pstOutput);
    if (SAL_SOK != s32Ret && ICF_ALGP_DETECT_NO_FACE != s32Ret && ICF_ALGP_TRACK_NO_FACE != s32Ret)
    {
        FACE_LOGE("ret: %8x\n", s32Ret);
        goto err;
    }

    pstMediaInfo = (ICF_MEDIA_INFO_V2 *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput,
                                                                                        ICF_ANA_MEDIA_DATA);
    FACE_DRV_CHECK_RET(NULL == pstMediaInfo, err, "pstMediaInfo == null!");

    pstFaceInDataInfo = (SAE_FACE_IN_DATA_INPUT_T *)pstMediaInfo->stInputInfo.stBlobData[0].pData;
    FACE_DRV_CHECK_RET(NULL == pstFaceInDataInfo, err, "pstFaceInDataInfo == null!");

#ifdef DEBUG_FACE_LOG
    fprintf(g_fp_res, "frame num: %d\n\n", pstFaceInDataInfo->data_info[0].frame_num);

    /* FD检测结果 */
    s32Ret = Face_DrvFDDetectDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFDDetectDebugTest failed!");

    /* Track结果 */
    s32Ret = Face_DrvTarckDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvTarckDebugTest failed!");

    /* FDQuality结果 */
    s32Ret = Face_DrvFDQualityDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFDQualityDebugTest failed!");

    /* Select选帧结果 */
    s32Ret = Face_DrvSelectDebugTest(pstOutput, "cap");
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvSelectDebugTest failed!");
#endif

#ifdef DEBUG_FACE_LOG
    /*关键点结果*/
    s32Ret = Face_DrvLandMarkDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvLandMarkDebugTest failed!");

    /* 属性结果输出 */
    s32Ret = Face_DrvAttributeDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvAttributeDebugTest failed!");

    /* 建模结果输出 */
    s32Ret = Face_DrvFeatureDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFeatureDebugTest failed!");
#endif

    /* 基于引擎返回的选帧结果，获取背景图像数据和人脸区域送入下一节点队列 */
    {
        pstSelectFrame = (SAE_FACE_OUT_DATA_SELECT_FRAME_T *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput, ICF_ANA_SELECT_FRAME_DATA);
        FACE_DRV_CHECK_RET(NULL == pstSelectFrame, err, "IcfGetPackageDataPtr failed!");

        if (SAL_SOK != pstSelectFrame->select_frame_sts)
        {
            FACE_LOGE("login: select frame err! sts: %x \n", pstSelectFrame->select_frame_sts);
            goto err;
        }

        if (pstSelectFrame->select_face_num >= 1)
        {
            FACE_LOGI("login: get select frame num %d \n", pstSelectFrame->select_face_num);
            FACE_LOGI("login: i %d, id %d, conf %f, [%f, %f] [%f, %f] \n",
                      i, pstSelectFrame->select_face_info[0].id,
                      pstSelectFrame->select_face_info[0].confidence,
                      pstSelectFrame->select_face_info[0].rect_src.x,
                      pstSelectFrame->select_face_info[0].rect_src.y,
                      pstSelectFrame->select_face_info[0].rect_src.width,
                      pstSelectFrame->select_face_info[0].rect_src.height);

            s32Ret = Face_DrvGetQueBuf(pstEmptQue, (VOID **)&pstVideoLoginSelQueBuf, SAL_TIMEOUT_NONE);
            if (s32Ret != SAL_SOK || NULL == pstVideoLoginSelQueBuf)
            {
                FACE_LOGE("login: Get Que buff Err!\n");
                goto err;
            }

            /* 拷贝选帧图像，将背景图和人脸抠图信息送入下一个graph3节点队列，用于进一步获取人脸属性和建模结果 */
            s32Ret = Face_DrvGetBackImgFromLoginSelect(pstSelectFrame, pstVideoLoginSelQueBuf);
            FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvGetBackImgFromEngineSelect failed!");

            s32Ret = Face_DrvGetFaceRectFromLoginSelect(pstSelectFrame, pstVideoLoginSelQueBuf);
            FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvGetBackImgFromEngineSelect failed!");

            /* 入队列 */
            Face_DrvPutQueBuf(pstFullQue, (VOID *)pstVideoLoginSelQueBuf, SAL_TIMEOUT_NONE);
        }
        else
        {
            FACE_LOGD("login: errrr! get select face num %d \n", pstSelectFrame->select_face_num);
            goto err;
        }
    }

    if (pstSelectFrame)
    {
        SAE_FACE_Release_Cur_Sel_Frame(pstSelectFrame);
    }

    return SAL_SOK;/*此处信号量不加一，是因为后期处理时会将信号量加一*/

err:

    if (pstSelectFrame)
    {
        SAE_FACE_Release_Cur_Sel_Frame(pstSelectFrame);
    }

	if (NULL != pstFaceAnaBufTab)
	{
	    pstFaceAnaBufTab->uFaceProcOut.stVideoLoginProcOut.enProcSts = FACE_REGISTER_FAIL;
	    sem_post(&pstFaceAnaBufTab->sem);/*应用将会等待处理结果，所以即使失败，此处信号量会加一*/
	}
    return SAL_FAIL;

}

/**
 * @function    Face_DrvGetOutputResult2
 * @brief         人脸登录业务建模回调处理函数
 * @param[in]
 * @param[out]
 * @return  SAL_SOK
 */
INT32 Face_DrvGetOutputResult2(int nNodeID,
                               int nCallBackType,
                               void *pstOutput,
                               unsigned int nSize,
                               void *pUsr,
                               int nUserSize)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    ICF_MEDIA_INFO_V2 *pstMediaInfo = NULL;
    SAE_FACE_IN_DATA_INPUT_T *pstFaceInDataInfo = NULL;
    SAE_FACE_OUT_DATA_DFR_FEATURE_T *pstFeatureOut = NULL;
    FACE_VIDEO_LOGIN_OUT_S *pstVideoLoginOut = NULL;

    FACE_DRV_CHECK_RET(NULL == pstOutput, err, "pstOutput == null!");

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_RET(NULL == pstFaceHalComm, err, "pstFaceHalComm == null!");

    pstFaceAnaBufTab = Face_HalGetAnaDataTab(FACE_VIDEO_LOGIN_MODE);
    FACE_DRV_CHECK_RET(NULL == pstFaceAnaBufTab, err, "pstFaceAnaBufTab == null!");

    pstVideoLoginOut = &pstFaceAnaBufTab->uFaceProcOut.stVideoLoginProcOut;

    s32Ret = pstFaceHalComm->stIcfFuncP.IcfGetPackageStatus(pstOutput);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ret: %8x\n", s32Ret);
        goto err;
    }

    pstMediaInfo = (ICF_MEDIA_INFO_V2 *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput,
                                                                                        ICF_ANA_MEDIA_DATA);
    FACE_DRV_CHECK_RET(NULL == pstMediaInfo, err, "pstMediaInfo == null!");

    pstFaceInDataInfo = (SAE_FACE_IN_DATA_INPUT_T *)pstMediaInfo->stInputInfo.stBlobData[0].pData;
    FACE_DRV_CHECK_RET(NULL == pstFaceInDataInfo, err, "pstFaceInDataInfo == null!");

    FACE_LOGI("Get LogIn Face data! frame_num(%d) \n", (UINT32)pstMediaInfo->stInputInfo.stBlobData[0].nFrameNum);

#ifdef DEBUG_FACE_LOG
    /* FD检测结果 */
    s32Ret = Face_DrvFDDetectDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFDDetectDebugTest failed!!");

    /* Track结果 */
    s32Ret = Face_DrvTarckDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvTarckDebugTest failed!!");

    /* FDQuality结果 */
    s32Ret = Face_DrvFDQualityDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFDQualityDebugTest failed!!");

    /* Select选帧结果 */
    s32Ret = Face_DrvSelectDebugTest(pstOutput, "rcg");
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvSelectDebugTest failed!!");
#endif

#ifdef DEBUG_FACE_LOG
    /*关键点结果*/
    s32Ret = Face_DrvLandMarkDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvLandMarkDebugTest failed!!");

    /* 属性结果输出 */
    s32Ret = Face_DrvAttributeDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvAttributeDebugTest failed!!");

    /* 建模结果输出 */
    s32Ret = Face_DrvFeatureDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFeatureDebugTest failed!!");
#endif

    /* 获取人脸建模结果 */
    {
        pstFeatureOut = (SAE_FACE_OUT_DATA_DFR_FEATURE_T *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput,
                                                                                                           ICF_ANA_DFR_FEATURE_DATA);
        FACE_DRV_CHECK_RET(NULL == pstFeatureOut, err, "pstFeatureOut == null!");

        if (SAL_SOK != pstFeatureOut->module_sts)
        {
            FACE_LOGE("login: get feature failed! ret: %8x\n",pstFeatureOut->module_sts);
            goto err;
        }

        FACE_LOGI("login: Face Feature Num(%d), feature len %d \n",
                  pstFeatureOut->feat_face_num, pstFeatureOut->feat_len);

        for (i = 0; i < pstFeatureOut->feat_face_num; i++)
        {
            if (SAL_SOK != pstFeatureOut->face_sts[i])
            {
                FACE_LOGW("login: err face feature! skip! i %d, ret:%8x \n", i,pstFeatureOut->face_sts[i]);
                continue;
            }

            /*更新结构体中的feature数据*/
            pstVideoLoginOut->enProcSts = FACE_REGISTER_SUCCESS;

            pstVideoLoginOut->u32FeatDataLen = pstFeatureOut->feat_len;
            sal_memcpy_s(pstVideoLoginOut->acFeaureData, FACE_FEATURE_LENGTH,
                         pstFeatureOut->feat_data[i], pstFeatureOut->feat_len);
            break;
        }

        if (pstFeatureOut->feat_face_num && i >= pstFeatureOut->feat_face_num)
        {
            FACE_LOGW("login: not found valid face feature! \n");
            goto err;
        }
    }

    goto exit;

err:
	if (NULL != pstVideoLoginOut)
	{
    	pstVideoLoginOut->enProcSts = FACE_REGISTER_FAIL;
	}

exit:
	if (NULL != pstFaceAnaBufTab)
	{
    	sem_post(&pstFaceAnaBufTab->sem);
	}
    return SAL_SOK;
}

/**
 * @function   Face_DrvGetFaceRectFromCapSelect
 * @brief      从引擎选帧结果中获取人脸目标区域
 * @param[in]  SAE_FACE_OUT_DATA_SELECT_FRAME_T *pstSelFrameInfo
 * @param[in]  FACE_VIDEO_CAP_SELECT_QUE_DATA_S *pstVideoCapSelQueBuf
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_DrvGetFaceRectFromCapSelect(SAE_FACE_OUT_DATA_SELECT_FRAME_T *pstSelFrameInfo,
                                              FACE_VIDEO_CAP_SELECT_QUE_DATA_S *pstVideoCapSelQueBuf)
{
    UINT32 i = 0;

    FACE_DRV_CHECK_RET(NULL == pstSelFrameInfo, err, "pstOutput == null!");
    FACE_DRV_CHECK_RET(NULL == pstVideoCapSelQueBuf, err, "pstOutput == null!");

    /* 拷贝人脸目标区域信息 */
    pstVideoCapSelQueBuf->u32FaceNum = pstSelFrameInfo->select_face_num;
    FACE_LOGI("get face rect num %d,  from select frame! \n", pstVideoCapSelQueBuf->u32FaceNum);

	/* szl_face_todo: 增加人脸抠图拷贝，用于后续对人脸抠图进行建模 */
    for (i = 0; i < pstVideoCapSelQueBuf->u32FaceNum; i++)
    {
        pstVideoCapSelQueBuf->stFaceRec[i].x = pstSelFrameInfo->select_face_info[i].rect_src.x;
        pstVideoCapSelQueBuf->stFaceRec[i].y = pstSelFrameInfo->select_face_info[i].rect_src.y;
        pstVideoCapSelQueBuf->stFaceRec[i].width = pstSelFrameInfo->select_face_info[i].rect_src.width;
        pstVideoCapSelQueBuf->stFaceRec[i].height = pstSelFrameInfo->select_face_info[i].rect_src.height;

        FACE_LOGI("i %d, get face rect [%f, %f], [%f, %f] \n",
                  i, pstVideoCapSelQueBuf->stFaceRec[i].x,
                  pstVideoCapSelQueBuf->stFaceRec[i].y,
                  pstVideoCapSelQueBuf->stFaceRec[i].width,
                  pstVideoCapSelQueBuf->stFaceRec[i].height);
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function   Face_DrvGetBackImgFromCapSelect
 * @brief      从引擎结果中获取选帧背景图
 * @param[in]  SAE_FACE_OUT_DATA_SELECT_FRAME_T *pstSelFrameInfo
 * @param[in]  FACE_VIDEO_CAP_SELECT_QUE_DATA_S *pstVideoCapSelQueBuf
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_DrvGetBackImgFromCapSelect(SAE_FACE_OUT_DATA_SELECT_FRAME_T *pstSelFrameInfo,
                                             FACE_VIDEO_CAP_SELECT_QUE_DATA_S *pstVideoCapSelQueBuf)
{
    UINT32 u32ImgSize = 0;

    SAE_FACE_SELECT_FRAME_INFO_T *pstSelectFrame = NULL; /* 帧数据 */

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    FACE_DRV_CHECK_RET(NULL == pstSelFrameInfo, err, "pstOutput == null!");
    FACE_DRV_CHECK_RET(NULL == pstVideoCapSelQueBuf, err, "pstOutput == null!");

    if (pstSelFrameInfo->select_face_num <= 0 || pstSelFrameInfo->select_face_num > 1)
    {
        FACE_LOGW("select frame num > 1, %d \n", pstSelFrameInfo->select_face_num);
    }

    /* 当前仅取第一张，因在配置选帧策略时，默认每个人脸id仅返回一张选帧结果 */
    pstSelectFrame = &pstSelFrameInfo->select_face_info[0];

    FACE_LOGI("get background img w %d, h %d, format %d \n",
              pstSelectFrame->image_info_src.yuv_data.image_w,
              pstSelectFrame->image_info_src.yuv_data.image_h,
              pstSelectFrame->image_info_src.yuv_data.format);

    u32ImgSize = pstSelectFrame->image_info_src.yuv_data.image_w \
                 * pstSelectFrame->image_info_src.yuv_data.image_h * 3 / 2;

    /* 拷贝图像 */
    {
        if (SAL_SOK != sys_hal_getVideoFrameInfo(&pstVideoCapSelQueBuf->stFrame, &stVideoFrmBuf))
        {
			FACE_LOGE("get video frame info failed! \n");
			goto err;
		}
        memcpy((VOID *)stVideoFrmBuf.virAddr[0], pstSelectFrame->image_info_src.yuv_data.y, u32ImgSize);
    }

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Face_DrvGetOutputResult3
 * @brief         算法抓拍回调函数，返回抓拍人脸大小图数据
 * @param[in]
 * @param[out]
 * @return    SAL_SOK
 */
INT32 Face_DrvGetOutputResult3(int nNodeID,
                               int nCallBackType,
                               void *pstOutput,
                               unsigned int nSize,
                               void *pUsr,
                               int nUserSize)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    FACE_DEV_PRM *pstFaceDev = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    ICF_MEDIA_INFO_V2 *pstMediaInfo = NULL;
    SAE_FACE_IN_DATA_INPUT_T *pstFaceInDataInfo = NULL;
    SAE_FACE_OUT_DATA_SELECT_FRAME_T *pstSelectFrame = NULL;
    DSA_QueHndl *pstFullQue = NULL;
    DSA_QueHndl *pstEmptQue = NULL;
    FACE_VIDEO_CAP_SELECT_QUE_DATA_S *pstVideoCapSelQueBuf = NULL;

    FACE_DRV_CHECK_RET(NULL == pstOutput, err, "pstOutput == null!");

    pstFaceDev = Face_DrvGetDev(0);
    FACE_DRV_CHECK_RET(NULL == pstFaceDev, err, "pstFaceDev == null!");

    pstFullQue = pstFaceDev->stFaceVideoCapSelQueInfo.stFaceSelQuePrm.pstFullQue;
    pstEmptQue = pstFaceDev->stFaceVideoCapSelQueInfo.stFaceSelQuePrm.pstEmptQue;

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_RET(NULL == pstFaceHalComm, err, "pstFaceHalComm == null!");

    pstFaceAnaBufTab = Face_HalGetAnaDataTab(FACE_VIDEO_CAP_MODE);
    FACE_DRV_CHECK_RET(NULL == pstFaceAnaBufTab, err, "pstFaceAnaBufTab == null!");

    /* 当前选帧业务中检测人脸数为0时会报错，此处进行校验 */
    s32Ret = pstFaceHalComm->stIcfFuncP.IcfGetPackageStatus(pstOutput);
    if (SAL_SOK != s32Ret && ICF_ALGP_DETECT_NO_FACE != s32Ret)
    {
        FACE_LOGD("ret: %8x\n", s32Ret);
        goto err;
    }

    pstMediaInfo = (ICF_MEDIA_INFO_V2 *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput,
                                                                                        ICF_ANA_MEDIA_DATA);
    FACE_DRV_CHECK_RET(NULL == pstMediaInfo, err, "pstMediaInfo == null!");

    pstFaceInDataInfo = (SAE_FACE_IN_DATA_INPUT_T *)pstMediaInfo->stInputInfo.stBlobData[0].pData;
    FACE_DRV_CHECK_RET(NULL == pstFaceInDataInfo, err, "pstFaceInDataInfo == null!");

    FACE_LOGD("============= cap: get select frameNum %d \n", (UINT32)pstMediaInfo->stInputInfo.stBlobData[0].nFrameNum);

#ifdef DEBUG_FACE_LOG
    fprintf(g_fp_res, "frame num: %d\n\n", pstFaceInDataInfo->data_info[0].frame_num);

    /* FD检测结果 */
    s32Ret = Face_DrvFDDetectDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFDDetectDebugTest failed!");

    /* Track结果 */
    s32Ret = Face_DrvTarckDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvTarckDebugTest failed!");

    /* FDQuality结果 */
    s32Ret = Face_DrvFDQualityDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFDQualityDebugTest failed!");

    /* Select选帧结果 */
    s32Ret = Face_DrvSelectDebugTest(pstOutput, "cap");
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvSelectDebugTest failed!");
#endif

#ifdef DEBUG_FACE_LOG
    /*关键点结果*/
    s32Ret = Face_DrvLandMarkDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvLandMarkDebugTest failed!");

    /* 属性结果输出 */
    s32Ret = Face_DrvAttributeDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvAttributeDebugTest failed!");

    /* 建模结果输出 */
    s32Ret = Face_DrvFeatureDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFeatureDebugTest failed!");
#endif

    /* 基于引擎返回的选帧结果，获取背景图像数据和人脸区域送入下一节点队列 */
    {
        pstSelectFrame = (SAE_FACE_OUT_DATA_SELECT_FRAME_T *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput, ICF_ANA_SELECT_FRAME_DATA);
        FACE_DRV_CHECK_RET(NULL == pstSelectFrame, err, "IcfGetPackageDataPtr failed!");

        if (SAL_SOK != pstSelectFrame->select_frame_sts)
        {
            FACE_LOGE("cap: select frame err! sts: %x \n", pstSelectFrame->select_frame_sts);
            goto err;
        }



        if (pstSelectFrame->select_face_num >= 1)
        {
            FACE_LOGW("cap: get select face num %d \n", pstSelectFrame->select_face_num);
            FACE_LOGI("cap: i %d, id %d, conf %f, [%f, %f] [%f, %f] \n",
                      i, pstSelectFrame->select_face_info[0].id,
                      pstSelectFrame->select_face_info[0].confidence,
                      pstSelectFrame->select_face_info[0].rect_src.x,
                      pstSelectFrame->select_face_info[0].rect_src.y,
                      pstSelectFrame->select_face_info[0].rect_src.width,
                      pstSelectFrame->select_face_info[0].rect_src.height);

            s32Ret = Face_DrvGetQueBuf(pstEmptQue, (VOID **)&pstVideoCapSelQueBuf, SAL_TIMEOUT_NONE);
            if (s32Ret != SAL_SOK || NULL == pstVideoCapSelQueBuf)
            {
                FACE_LOGE("cap: Get Que buff Err!\n");
                goto err;
            }

            /* 拷贝选帧图像，将背景图和人脸抠图信息送入下一个graph3节点队列，用于进一步获取人脸属性和建模结果 */
            s32Ret = Face_DrvGetBackImgFromCapSelect(pstSelectFrame, pstVideoCapSelQueBuf);
            FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvGetBackImgFromEngineSelect failed!");

            s32Ret = Face_DrvGetFaceRectFromCapSelect(pstSelectFrame, pstVideoCapSelQueBuf);
            FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvGetBackImgFromEngineSelect failed!");

            /* 入队列 */
            if (1)
            {
                Face_DrvPutQueBuf(pstFullQue, (VOID *)pstVideoCapSelQueBuf, SAL_TIMEOUT_NONE);
            }
            else /* debug: 屏蔽后级处理 */
            {
                Face_DrvPutQueBuf(pstEmptQue, (VOID *)pstVideoCapSelQueBuf, SAL_TIMEOUT_NONE);
            }
        }
        else
        {
            FACE_LOGD("cap: errrr! get select face num %d \n", pstSelectFrame->select_face_num);
        }
    }

    if (pstSelectFrame)
    {
        SAE_FACE_Release_Cur_Sel_Frame(pstSelectFrame);
    }

    return SAL_SOK;
err:
    if (pstSelectFrame)
    {
        SAE_FACE_Release_Cur_Sel_Frame(pstSelectFrame);
    }

    return SAL_FAIL;
}

/**
 * @function    Face_DrvGetOutputResult4
 * @brief         算法返回人脸属性和建模数据的回调接口，用于人脸抓拍业务线
 * @param[in]
 * @param[out]
 * @return  SAL_SOK
 */
INT32 Face_DrvGetOutputResult4(int nNodeID,
                               int nCallBackType,
                               void *pstOutput,
                               unsigned int nSize,
                               void *pUsr,
                               int nUserSize)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
	UINT32 u32Cnt = 0;

    FACE_COMMON_PARAM *pstFaceHalComm = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    ICF_MEDIA_INFO_V2 *pstMediaInfo = NULL;
    SAE_FACE_IN_DATA_INPUT_T *pstFaceInDataInfo = NULL;
    SAE_FACE_OUT_DATA_DFR_FEATURE_T *pstFeatureOut = NULL;
    FACE_VIDEO_CAPTURE_OUT_S *pstVideoCapOut = NULL;

    FACE_DRV_CHECK_RET(NULL == pstOutput, err, "pstOutput == null!");

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_RET(NULL == pstFaceHalComm, err, "pstFaceHalComm == null!");

    pstFaceAnaBufTab = Face_HalGetAnaDataTab(FACE_VIDEO_CAP_MODE);
    FACE_DRV_CHECK_RET(NULL == pstFaceAnaBufTab, err, "pstFaceAnaBufTab == null!");

    pstVideoCapOut = &pstFaceAnaBufTab->uFaceProcOut.stVideoCapProcOut;

    s32Ret = pstFaceHalComm->stIcfFuncP.IcfGetPackageStatus(pstOutput);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("ret: %8x\n", s32Ret);
        goto err;
    }

    pstMediaInfo = (ICF_MEDIA_INFO_V2 *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput,
                                                                                        ICF_ANA_MEDIA_DATA);
    FACE_DRV_CHECK_RET(NULL == pstMediaInfo, err, "pstMediaInfo == null!");

    pstFaceInDataInfo = (SAE_FACE_IN_DATA_INPUT_T *)pstMediaInfo->stInputInfo.stBlobData[0].pData;
    FACE_DRV_CHECK_RET(NULL == pstFaceInDataInfo, err, "pstFaceInDataInfo == null!");

    FACE_LOGI("Get cap Face data! frame_num(%d) \n", pstFaceInDataInfo->data_info[0].frame_num);

#ifdef DEBUG_FACE_LOG
    /* FD检测结果 */
    s32Ret = Face_DrvFDDetectDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFDDetectDebugTest failed!!");

    /* Track结果 */
    s32Ret = Face_DrvTarckDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvTarckDebugTest failed!!");

    /* FDQuality结果 */
    s32Ret = Face_DrvFDQualityDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFDQualityDebugTest failed!!");

    /* Select选帧结果 */
    s32Ret = Face_DrvSelectDebugTest(pstOutput, "rcg");
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvSelectDebugTest failed!!");
#endif

#ifdef DEBUG_FACE_LOG
    /*关键点结果*/
    s32Ret = Face_DrvLandMarkDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvLandMarkDebugTest failed!!");

    /* 属性结果输出 */
    s32Ret = Face_DrvAttributeDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvAttributeDebugTest failed!!");

    /* 建模结果输出 */
    s32Ret = Face_DrvFeatureDebugTest(pstOutput);
    FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "Face_DrvFeatureDebugTest failed!!");
#endif

    /* 获取人脸建模结果 */
    {
        pstFeatureOut = (SAE_FACE_OUT_DATA_DFR_FEATURE_T *)pstFaceHalComm->stIcfFuncP.IcfGetPackageDataPtr(pstOutput,
                                                                                                          ICF_ANA_DFR_FEATURE_DATA);
        FACE_DRV_CHECK_RET(NULL == pstFeatureOut, err, "pstFeatureOut == null!");

        if (SAL_SOK != pstFeatureOut->module_sts)
        {
            FACE_LOGE("get feature failed! ret: %8x\n",pstFeatureOut->module_sts);
            goto err;
        }

        FACE_LOGI("Face Feature Num(%d), feature len %d \n",
                  pstFeatureOut->feat_face_num, pstFeatureOut->feat_len);

        for (u32Cnt = 0, i = 0; i < pstFeatureOut->feat_face_num && i < MAX_CAP_FACE_NUM; i++)
        {
        	/* 异常人脸信息过滤 */
            if (SAL_SOK != pstFeatureOut->face_sts[i])
            {
                FACE_LOGW("err face feature! skip! i %d, ret: %8x \n", i, pstFeatureOut->face_sts[i]);
                continue;
            }

            /*更新结构体中的feature数据*/
            pstVideoCapOut->enProcSts = FACE_REGISTER_SUCCESS;

            pstVideoCapOut->au32FeatDataLen[i] = pstFeatureOut->feat_len;
            sal_memcpy_s((UINT8 *)pstVideoCapOut->acFeaureData[i], FACE_FEATURE_LENGTH,
                         pstFeatureOut->feat_data[i], pstFeatureOut->feat_len);
			
			++u32Cnt;
        }

		/* 保存人脸个数 */
		pstVideoCapOut->u32FaceNum = u32Cnt;
		FACE_LOGW("pstVideoCapOut->u32FaceNum(%d), feature len %d \n",
                  pstVideoCapOut->u32FaceNum, pstFeatureOut->feat_len);
    }
    goto exit;

err:
	if (NULL != pstVideoCapOut)
	{
    	pstVideoCapOut->enProcSts = FACE_REGISTER_FAIL;
	}

exit:
	if (NULL != pstFaceAnaBufTab)
	{
    	sem_post(&pstFaceAnaBufTab->sem);
	}
    return SAL_SOK;
}

/**
 * @function    Face_DrvJudgeFeatData
 * @brief         判断待注册人脸是否已注册
 * @param[in]  chan  : 通道号
 * @param[in]  pParam: 特征数据
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_DrvJudgeFeatData(UINT32 chan, void *pParam)
{
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pParam, SAL_FAIL);

    UINT32 s32Ret = SAL_FAIL;
    float fSim = 0.9f;

    UINT8 *FeatureData = NULL;
    FACE_MODEL_DATA_BASE *pstFaceDataBase = NULL;
    FACE_DSP_DATABASE_PARAM *pstFaceDataInfo = NULL;
    FACE_DSP_DATABASE_PARAM stFaceDataInfo = {0};

    pstFaceDataInfo = (FACE_DSP_DATABASE_PARAM *)pParam;
    FeatureData = (UINT8 *)&pstFaceDataInfo->Featdata[0];

    pstFaceDataBase = Face_HalGetDataBase();
    if (0 == pstFaceDataBase->uiModelCnt)
    {
        return SAL_SOK;
    }

    /* 对待注册人脸一一比对数据库中已有人脸 */
    memcpy(&stFaceDataInfo.Featdata[0], FeatureData, FACE_FEATURE_LENGTH);
    s32Ret = Face_HalCompare(chan, &stFaceDataInfo, fSim, 1);
    if (SAL_SOK != s32Ret)
    {
        if(FACE_REGISTER_REPEAT == stFaceDataInfo.bFlag)
        {
			/* 数据库中已存在注册人脸，返回失败通知应用为重复注册 */
			pstFaceDataInfo->uiRepeatId = stFaceDataInfo.uiRepeatId;
			pstFaceDataInfo->bFlag = FACE_REGISTER_REPEAT;
			FACE_LOGW("chan %d Face Comping Found the Existing FaceID %d!\n", chan, stFaceDataInfo.uiRepeatId);
		}
		if(FACE_REGISTER_FAIL == stFaceDataInfo.bFlag)
		{
			/*已获取人脸特征，但是注册失败，建议检测人脸库中人脸特征版本是否正确*/
			pstFaceDataInfo->bFlag = FACE_REGISTER_FAIL;
			FACE_LOGW("chan %d Face Register Failed!\n", chan);
		}
		
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Face_DrvJpeg2Yuv
 * @brief         人脸图片解码(Jpeg To Nv21)
 * @param[in]  chan  : 通道号
 * @param[in]  pParam: 人脸注册相关信息
 * @param[in]  pFrame  : 人脸注册相关信息
 * @param[out] pOut: 注册成功后返回特征数据
 * @return SAL_SOK
 */
UINT32 Face_DrvJpeg2Yuv(UINT32 chan, void *pParam, void *pFrame, void *pOut)
{
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pParam, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pFrame, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pOut, SAL_FAIL);

#if 1  /* 测试耗时使用，后续删除 */
    UINT64 time_get_jpge_start = 0;
    UINT64 time_get_jpge_end = 0;
    /*   UINT64 time_scale_chn_start = 0; */
    /*   UINT64 time_scale_chn_end = 0; */
    /*   UINT64 time_send_jpeg_start = 0; */
    /*   UINT64 time_send_jpeg_end = 0; */
    /*   UINT64 time_get_yuv_start = 0; */
/*    UINT64 time_get_yuv_end = 0; */
    UINT64 time_get_memsize_start = 0;
    UINT64 time_get_memsize_end = 0;
    UINT64 time_get_alloc_start = 0;
    UINT64 time_get_alloc_end = 0;
    UINT64 time_get_create_start = 0;
    UINT64 time_get_create_end = 0;
    UINT64 time_jdec_proc_start = 0;
    UINT64 time_jdec_proc_end = 0;
    UINT64 time_jdec_convrt_start = 0;
    UINT64 time_jdec_convrt_end = 0;
    UINT64 time_2nv21_start = 0;
    UINT64 time_2nv21_end = 0;
#endif

    UINT32 i = 0;
    UINT32 bufLen = 0;
    UINT32 decWidth = 0;
    UINT32 decHeight = 0;
    UINT32 pitch = 0;
    UINT32 uiJpegLen = 0;
    UINT32 uiVdecChn = 0;
    UINT32 uiWidth = 0;
    UINT32 uiHeight = 0;
    UINT32 uiProgressiveMode = 0;
    PhysAddr uiPhyAddr = 0;

    UINT32 s32Ret = SAL_FAIL;
/*    UINT32 bHwJDec = SAL_FALSE; */
    UINT32 bSoftJDec = SAL_TRUE;

    FACE_YUV_FRAME_EX yuvFrame = {0};
    HKAJPGD_ABILITY stAbility = {0};
    HKAJPGD_STREAM stJpegData = {0};
    HKAJPGD_OUTPUT outParam = {0};
    HKA_MEM_TAB memTab[HKA_MEM_TAB_NUM] = {0};
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    PUINT8 pSrcY = NULL;
    PUINT8 pSrcV = NULL;
    PUINT8 pSrcU = NULL;
    FACE_REG_RESULT_S *pstOutResult = NULL;
    FACE_DSP_REGISTER_JPG_PARAM *pstJpegInfo = NULL;
    FACE_DEV_PRM *pstFace_Dev = NULL;
    FACE_JDEC_BUF_INFO *pstJdecBufInfo = NULL;
    FACE_JDEC_BUF *pstJdecBufData = NULL;
    SYSTEM_FRAME_INFO *pstSysFrameInfo = NULL;

    void *pVirAddr = NULL;
    void *pConvertData[3] = {NULL};
    void *pOutBuf1 = NULL;          /* 解码的后的图像存储 */
    void *pOutBuf2 = NULL;          /* 格式转换后的图像 */

    pstFace_Dev = Face_DrvGetDev(chan);

    uiVdecChn = pstFace_Dev->uiJpegVdecChn;
    pstJpegInfo = (FACE_DSP_REGISTER_JPG_PARAM *)pParam;
    pstSysFrameInfo = (SYSTEM_FRAME_INFO *)pFrame;
    pstOutResult = (FACE_REG_RESULT_S *)pOut;

    uiJpegLen = pstJpegInfo->uiLength;
    uiPhyAddr = pstFace_Dev->stFaceYuvMem.uiPhyAddr;
    pVirAddr = pstFace_Dev->stFaceYuvMem.pVirAddr;
    pstOutResult->eErrCode = FACE_REG_ERR_MODE_NUM;

    FACE_LOGI("Drv Jpg Addr %p vdecChn %d \n", pstJpegInfo->pAddr, uiVdecChn);

    /* 获取jpeg解码缓存，目前只申请一个，只有一个人脸通道 */
    pstJdecBufInfo = Face_HalGetJdecBuf();
    if (NULL == pstJdecBufInfo)
    {
        FACE_LOGE("Get Jdec Buf Failed! chan %d\n", chan);
        return SAL_FAIL;
    }

    if (pstJpegInfo->uiLength > MAX_JPEG_SIZE)
    {
        FACE_LOGE("Jpeg size %d > Max Size %d, chan %d \n", pstJpegInfo->uiLength, MAX_JPEG_SIZE, chan);
        return SAL_FAIL;
    }

    pstJdecBufData = &pstJdecBufInfo->stBufData[0];
    memset((void *)pstJdecBufData->VirAddr, 0, pstJdecBufData->uiBufSize);

    /* 拷贝用户态数据到Jdec Buf */
    memcpy((void *)pstJdecBufData->VirAddr, pstJpegInfo->pAddr, pstJpegInfo->uiLength);

    /* 对获取的解码图片信息进行判断，硬解还是软解 */
    time_get_jpge_start = Face_HalgetTimeMilli();
    stJpegData.stream_buf = (UINT8 *)pstJdecBufData->VirAddr;
    stJpegData.stream_len = uiJpegLen;

    s32Ret = HKAJPGD_GetImageInfo(&stJpegData, &stAbility.image_info);
    if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
    {
        FACE_LOGE("chan %d HKAJPGD_GetImageInfo ERR! s32Ret:0x%x\n", chan, s32Ret);
        return SAL_FAIL;
    }

    time_get_jpge_end = Face_HalgetTimeMilli();

    uiWidth = stAbility.image_info.img_size.width;
    uiHeight = stAbility.image_info.img_size.height;
    uiProgressiveMode = stAbility.image_info.progressive_mode;

    if (uiWidth > 1920 || uiHeight > 1080)
    {
        pstOutResult->eErrCode = FACE_RESOLUTION_OVER_SIZE;
        FACE_LOGW("invalid picture resolution > 1080P! w %d, h %d \n", uiWidth, uiHeight);
        return SAL_FAIL;
    }

    FACE_LOGI("ability: progressive_mode %d num_components %d w %d h %d pix_format %d \n",
              stAbility.image_info.progressive_mode,
              stAbility.image_info.num_components,
              stAbility.image_info.img_size.width,
              stAbility.image_info.img_size.height,
              stAbility.image_info.pix_format);

    FACE_LOGI("Get Jpeg cost %llu ms, chan %d w %d h %d mode %d \n",
              time_get_jpge_end - time_get_jpge_start, chan, uiWidth, uiHeight, uiProgressiveMode);

    /* if (uiProgressiveMode) */
    {
        /*       bHwJDec = SAL_FALSE; */
        bSoftJDec = SAL_TRUE;
    }

#if 0
    /*现在逻辑默认只用软件解码，不用硬件*/
    if (bHwJDec)
    {
        pstOutResult->eJdecMode = FACE_JDEC_HW_MODE;

        FACE_LOGI("HW Jdec Entering! chan %d \n", chan);

        time_scale_chn_start = Face_HalgetTimeMilli();

        /* 更新解码输出通道宽高 */

        if (uiWidth < FACE_LOGIN_IMG_WIDTH
            && uiHeight < FACE_LOGIN_IMG_HEIGHT)
        {
            uiWidth = ALIGN_UP(uiWidth, 2);
            uiHeight = ALIGN_UP(uiHeight, 2);
            s32Ret = Vdec_halScaleJpegChn(uiVdecChn, 0, uiWidth, uiHeight);
            if (SAL_SOK != s32Ret)
            {
                FACE_LOGE("chan %d VdecChn %d Update Jpeg Output Chn Failed!\n", chan, uiVdecChn);
                return SAL_FAIL;
            }
        }

        time_scale_chn_end = Face_HalgetTimeMilli();
        FACE_LOGI("szl_dbg: chan %d VdecChn %d Update Jpeg Output Chn OK, cost %llu ms! w %d h %d\n ",
                  chan, uiVdecChn, time_scale_chn_end - time_scale_chn_start, uiWidth, uiHeight);


        time_send_jpeg_start = Face_HalgetTimeMilli();
        /* jpg数据送解码 */
        s32Ret = Vdec_halDSendJpegFrame(uiVdecChn, (void *)pstJpegInfo, pstJpegInfo->uiLength);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("chan %d VdecChn %d Send Frame Failed!\n", chan, uiVdecChn);
            return SAL_FAIL;
        }

        time_send_jpeg_end = Face_HalgetTimeMilli();
        FACE_LOGI("szl_dbg: chan %d VdecChn %d Send Frame OK, cost %llu ms!\n ",
                  chan, uiVdecChn, time_send_jpeg_end - time_send_jpeg_start);


        time_get_yuv_start = Face_HalgetTimeMilli();

        /* 获取解码数据 */
        s32Ret = Vdec_halDGetJpegYUVFrame(uiVdecChn, 2, pstFrameInfo);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("chan %d VdecChn %d Get Dec Yuv Failed!\n", chan, uiVdecChn);
            bSoftJDec = SAL_TRUE;   /* 硬解失败不返回，启用软解 */
        }

        time_get_yuv_end = Face_HalgetTimeMilli();
        FACE_LOGI("Get HW Jdec Frame cost %llu ms, chan %d \n", time_get_yuv_end - time_get_yuv_start, chan);
    }

#endif
    if (bSoftJDec)
    {
        FACE_LOGI("SOFT Jdec Entering! chan %d \n", chan);

        pstOutResult->eJdecMode = FACE_JDEC_SOFT_MODE;
        time_get_memsize_start = Face_HalgetTimeMilli();

        /* 获取所需资源数量 */

        FACE_LOGI("sizeof(ability): %d, sizeof(mem_tab): %d, mem_tab array loop: %d \n",
                  (UINT32)sizeof(HKAJPGD_ABILITY), (UINT32)sizeof(HKA_MEM_TAB) * HKA_MEM_TAB_NUM, HKA_MEM_TAB_NUM);

        FACE_LOGI("before getmemsz, ability: progressive_mode %d num_components %d w %d h %d pix_format %d \n",
                  stAbility.image_info.progressive_mode,
                  stAbility.image_info.num_components,
                  stAbility.image_info.img_size.width,
                  stAbility.image_info.img_size.height,
                  stAbility.image_info.pix_format);

        s32Ret = HKAJPGD_GetMemSize(&stAbility, memTab);
        if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
        {
            FACE_LOGE("Err ret:%#x\n", s32Ret);
            return SAL_FAIL;
        }

        FACE_LOGI("111after get mem size---ability: progressive_mode %d num_components %d w %d h %d pix_format %d, quality %d, watermark %d \n",
                  stAbility.image_info.progressive_mode,
                  stAbility.image_info.num_components,
                  stAbility.image_info.img_size.width,
                  stAbility.image_info.img_size.height,
                  stAbility.image_info.pix_format,
                  stAbility.image_info.quality,
                  stAbility.watermark_enable);

        for (i = 0; i < HKA_MEM_TAB_NUM; i++)
        {
            FACE_LOGI("memTab: i %d size %d alignment %d \n", i,
                      (UINT32)memTab[i].size,
                      (UINT32)memTab[i].alignment);
        }

        time_get_memsize_end = Face_HalgetTimeMilli();
        FACE_LOGI("Get MemSize cost %llu ms, chan %d \n", time_get_memsize_end - time_get_memsize_start, chan);

        /* 分配库所需资源 */
        time_get_alloc_start = Face_HalgetTimeMilli();
        s32Ret = Face_HalAllocMemTab(&memTab[0], HKA_MEM_TAB_NUM);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("Err ret:%#x\n", s32Ret);
            return SAL_FAIL;
        }

        time_get_alloc_end = Face_HalgetTimeMilli();
        FACE_LOGI("Alloc mem cost %llu ms, chan %d \n", time_get_alloc_end - time_get_alloc_start, chan);

        /* 创建库实例 */
        void *handle = NULL;

        time_get_create_start = Face_HalgetTimeMilli();

        for (i = 0; i < HKA_MEM_TAB_NUM; i++)
        {
            FACE_LOGI("memTab: i %d size %u alignment %d base %p \n", i,
                      (UINT32)memTab[i].size,
                      (UINT32)memTab[i].alignment,
                      memTab[i].base);
        }

        FACE_LOGI("ability: progressive_mode %d num_components %d w %d h %d pix_format %d, quality %d, watermark %d \n",
                  stAbility.image_info.progressive_mode,
                  stAbility.image_info.num_components,
                  stAbility.image_info.img_size.width,
                  stAbility.image_info.img_size.height,
                  stAbility.image_info.pix_format,
                  stAbility.image_info.quality,
                  stAbility.watermark_enable);

        s32Ret = HKAJPGD_Create(&stAbility, memTab, &handle);
        if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
        {
            Face_HalFreeMemTab(&memTab[0], HKA_MEM_TAB_NUM);
            FACE_LOGE("Err ret:%#x\n", s32Ret);
            return SAL_FAIL;
        }

        time_get_create_end = Face_HalgetTimeMilli();
        FACE_LOGI("create handle cost %llu ms, chan %d \n", time_get_create_end - time_get_create_start, chan);

        bufLen = ((uiHeight / 32) + 1) * 32 * ((uiWidth / 32) + 1) * 32;

        /* 申请jpg解码的后的缓存 4表示最大用到的存储的分量 */
        pOutBuf1 = Face_HalMemAlloc(bufLen * 4, 128);
        if (NULL == pOutBuf1)
        {
            Face_HalFreeMemTab(&memTab[0], HKA_MEM_TAB_NUM);
            FACE_LOGE("Mem Alloc Failed! \n");
            return SAL_FAIL;
        }

        outParam.image_out.data[0] = pOutBuf1;
        outParam.image_out.data[1] = outParam.image_out.data[0] + bufLen;
        outParam.image_out.data[2] = outParam.image_out.data[1] + bufLen;
        outParam.image_out.data[3] = outParam.image_out.data[2] + bufLen;

        /* process img_out的数据块 */
        time_jdec_proc_start = Face_HalgetTimeMilli();
        if (NULL == handle)
        {
            FACE_LOGE("handle Err NULL !!\n");
        }

        s32Ret = HKAJPGD_Process(handle, (HKA_VOID *)(&stJpegData), sizeof(HKAJPGD_STREAM), (HKA_VOID *)(&outParam), sizeof(HKAJPGD_OUTPUT));
        if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
        {
            Face_HalMemFree(pOutBuf1);
            Face_HalFreeMemTab(&memTab[0], HKA_MEM_TAB_NUM);
            FACE_LOGE("Err ret:%#x\n", s32Ret);
            return SAL_FAIL;
        }
        else
        {
            time_jdec_proc_end = Face_HalgetTimeMilli();
            FACE_LOGI("Jdec Proc Success! pix_format %d, cost %llu ms \n", stAbility.image_info.pix_format, time_jdec_proc_end - time_jdec_proc_start);

            /* jpg解码完后有多种格式，需要换成0x22111100处理 即为I420格式 */
            if (0x22111100 != stAbility.image_info.pix_format)
            {
                /* 申请格式转换后的缓存 I420格式 */
                bufLen = ((uiHeight / 32) + 1) * 32 * ((uiWidth / 32) + 1) * 32;
                pOutBuf2 = Face_HalMemAlloc(bufLen * 3 / 2, 128);
                if (NULL == pOutBuf2)
                {
                    Face_HalMemFree(pOutBuf1);
                    Face_HalFreeMemTab(&memTab[0], HKA_MEM_TAB_NUM);
                    FACE_LOGE("Mem Alloc Failed! \n");
                    return SAL_FAIL;
                }

                pConvertData[0] = pOutBuf2;                     /* Y分量 */
                pConvertData[1] = pConvertData[0] + bufLen;     /* U分量 */
                pConvertData[2] = pConvertData[1] + bufLen / 4; /* V分量 */

                /* 申请地址用着格式转化用的 */
                time_jdec_convrt_start = Face_HalgetTimeMilli();
                s32Ret = HKAJPGD_FormatConvert(handle, (void *)&outParam, (void **)pConvertData);
                if (HIK_VIDEO_DEC_LIB_S_OK != s32Ret)
                {
                    Face_HalMemFree(pOutBuf1);
                    Face_HalMemFree(pOutBuf2);
                    Face_HalFreeMemTab(&memTab[0], HKA_MEM_TAB_NUM);
                    FACE_LOGE("Err ret:%#x\n", s32Ret);
                    return SAL_FAIL;
                }

                time_jdec_convrt_end = Face_HalgetTimeMilli();
                FACE_LOGI("Jdec convert format cost %llu ms \n", time_jdec_convrt_end - time_jdec_convrt_start);

                pSrcY = pConvertData[0]; /* Y分量 */
                pSrcU = pConvertData[1]; /* U分量 */
                pSrcV = pConvertData[2]; /* V分量 */
            }
            else
            {
                pSrcY = outParam.image_out.data[0]; /* Y分量 */
                pSrcU = outParam.image_out.data[1]; /* U分量 */
                pSrcV = outParam.image_out.data[2]; /* V分量 */
            }

            /* 解码后的yuv图像宽和高都是2对齐，outParam.image_out.width和height是原图的大小 */
            decWidth = SAL_alignDown(outParam.image_out.width, 2u);
            decHeight = SAL_alignDown(outParam.image_out.height, 4u);

            /* 将yv12拷贝成nv21格式 */
            pitch = SAL_alignDown(decWidth, 16u);  /* 后面缩放模块要求pitch16对齐 格式转换后的图像要求16对齐，用优化的函数 宽和高要求是一样 */

            FACE_LOGI("align w %d h %d pitch %d \n", decWidth, decHeight, pitch);
            bufLen = pitch * decHeight * 3 / 2;

            /* 输出帧信息 */
            stVideoFrmBuf.frameParam.width = pitch;
            stVideoFrmBuf.frameParam.height = decHeight;
            stVideoFrmBuf.stride[0] = pitch;
            stVideoFrmBuf.stride[1] = pitch;
            stVideoFrmBuf.phyAddr[0] = (PhysAddr)uiPhyAddr;
            stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + pitch * stVideoFrmBuf.frameParam.height;
            stVideoFrmBuf.virAddr[0] = (PhysAddr)pVirAddr;
            stVideoFrmBuf.virAddr[1] = stVideoFrmBuf.virAddr[0] + stVideoFrmBuf.frameParam.height;
            stVideoFrmBuf.pts = 0;
            stVideoFrmBuf.frameNum = 2;
            stVideoFrmBuf.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;
            stVideoFrmBuf.poolId = pstFace_Dev->stFaceYuvMem.u32PoolId;
            stVideoFrmBuf.vbBlk = pstFace_Dev->stFaceYuvMem.u64VbBlk;

            (VOID)sys_hal_buildVideoFrame(&stVideoFrmBuf, pstSysFrameInfo);

            memset(&yuvFrame, 0x0, sizeof(yuvFrame));
            yuvFrame.frameMode = PROGRESSIVE_FRAME_MODE;
            yuvFrame.yTopAddr = pSrcY;
            yuvFrame.uTopAddr = pSrcU;
            yuvFrame.vTopAddr = pSrcV;
            yuvFrame.width = decWidth;
            yuvFrame.height = decHeight;
            yuvFrame.pitchY = outParam.image_out.step[0];
            yuvFrame.pitchUv = outParam.image_out.step[1]; /* uv的值一样大 */

            /* 海思平台的格式要求 优化的函数时候16字节对齐，宽和pitch一致的情况 */
            time_2nv21_start = Face_HalgetTimeMilli();
            s32Ret = Face_HalI420ToNv21(&yuvFrame, pitch, pitch, \
                                        (PUINT8)pVirAddr, (PUINT8)((PUINT8)pVirAddr + pitch * decHeight));
            if (SAL_SOK != s32Ret)
            {
                Face_HalFreeMemTab(&memTab[0], HKA_MEM_TAB_NUM);
                Face_HalMemFree(pOutBuf2);
                Face_HalMemFree(pOutBuf1);
                FACE_LOGE("Err: 0x%x!\n", s32Ret);
                return SAL_FAIL;
            }

            time_2nv21_end = Face_HalgetTimeMilli();
            FACE_LOGI("I420 to NV21 cost %llu ms \n", time_2nv21_end - time_2nv21_start);
        }

        /* MMZ内存释放 */
        if (NULL != pOutBuf1)
        {
            Face_HalMemFree(pOutBuf1);
            pOutBuf1 = NULL;
        }

        if (NULL != pOutBuf2)
        {
            Face_HalMemFree(pOutBuf2);
            pOutBuf2 = NULL;
        }

        Face_HalFreeMemTab(&memTab[0], HKA_MEM_TAB_NUM);
    }

    /* END */

    FACE_LOGI("Jpeg 2 Yuv End! \n");
    return SAL_SOK;
}

/**
 * @function   Face_DrvGetFrameMem
 * @brief      申请帧内存
 * @param[in]  UINT32 imgW
 * @param[in]  UINT32 imgH
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrameInfo
 * @param[out] None
 * @return     INT32
 */
INT32 Face_DrvGetFrameMem(UINT32 imgW, UINT32 imgH, SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 u32LumaSize = 0;
    UINT32 u32BlkSize = 0;

    ALLOC_VB_INFO_S stVbInfo = {0};
    SAL_VideoFrameBuf salVideoInfo = {0};

    if (0x00 == pstSystemFrameInfo->uiAppData)
    {
        s32Ret = sys_hal_allocVideoFrameInfoSt(pstSystemFrameInfo);
        if (SAL_FAIL == s32Ret)
        {
            DISP_LOGE("sys_hal_allocVideoFrameInfoSt failed size %d !\n", u32BlkSize);
            return SAL_FAIL;
        }
    }

    u32BlkSize = imgW * imgH * 3 / 2;

    s32Ret = mem_hal_vbAlloc(u32BlkSize, "FACE", "FRM_BUF", NULL, SAL_FALSE, &stVbInfo);
    if (SAL_FAIL == s32Ret)
    {
        DISP_LOGE("mem_hal_vbAlloc failed size %u !\n", u32BlkSize);
        return SAL_FAIL;
    }

    u32LumaSize = (imgW * imgH);

    salVideoInfo.poolId = stVbInfo.u32PoolId;
    salVideoInfo.frameParam.width = imgW;
    salVideoInfo.frameParam.height = imgH;
    salVideoInfo.frameParam.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;
    salVideoInfo.phyAddr[0] = stVbInfo.u64PhysAddr;
    salVideoInfo.phyAddr[1] = salVideoInfo.phyAddr[0] + u32LumaSize;
    salVideoInfo.phyAddr[2] = salVideoInfo.phyAddr[1];

    salVideoInfo.virAddr[0] = (PhysAddr)(stVbInfo.pVirAddr);
    salVideoInfo.virAddr[1] = salVideoInfo.virAddr[0] + u32LumaSize;
    salVideoInfo.virAddr[2] = salVideoInfo.virAddr[1];

    salVideoInfo.stride[0] = salVideoInfo.frameParam.width;
    salVideoInfo.stride[1] = salVideoInfo.frameParam.width;
    salVideoInfo.stride[2] = salVideoInfo.frameParam.width;
    salVideoInfo.vbBlk = (PhysAddr)stVbInfo.u64VbBlk;

    if (SAL_SOK != sys_hal_buildVideoFrame(&salVideoInfo, pstSystemFrameInfo))
    {
		FACE_LOGE("build video frame failed! \n");
		return SAL_FAIL;
	}

    pstSystemFrameInfo->uiDataAddr = (PhysAddr)salVideoInfo.virAddr[0];
    pstSystemFrameInfo->uiDataWidth = imgW;
    pstSystemFrameInfo->uiDataHeight = imgH;

    return SAL_SOK;
}

/**
 * @function   Face_DrvPutFrameMem
 * @brief      释放帧内存
 * @param[in]  SYSTEM_FRAME_INFO *pstSystemFrameInfo
 * @param[out] None
 * @return     INT32
 */
INT32 Face_DrvPutFrameMem(SYSTEM_FRAME_INFO *pstSystemFrameInfo)
{
    INT32 s32Ret = SAL_SOK;

    SAL_VideoFrameBuf salVideoInfo = {0};

    s32Ret = sys_hal_getVideoFrameInfo(pstSystemFrameInfo, &salVideoInfo);
	if (SAL_SOK != s32Ret)
	{
		FACE_LOGE("get video frame info failed! \n");
		return SAL_FAIL;
	}

    s32Ret = mem_hal_vbFree((Ptr)salVideoInfo.virAddr[0], "FACE", "FRM_BUF", salVideoInfo.bufLen, salVideoInfo.vbBlk, salVideoInfo.poolId);
    if (SAL_SOK != s32Ret)
    {
        DISP_LOGE("mem_hal_vbFree failed %p\n", (Ptr)salVideoInfo.virAddr[0]);
        return SAL_FAIL;
    }

    s32Ret = sys_hal_rleaseVideoFrameInfoSt(pstSystemFrameInfo);
	if (SAL_SOK != s32Ret)
	{
		FACE_LOGE("release video frame info failed! \n");
		return SAL_FAIL;
	}

    return SAL_SOK;
}

/**
 * @function    Face_DrvUserRegister
 * @brief         人脸注册
 * @param[in]    chan  : 通道号
 * @param[in]    pParam: 人脸注册相关信息
 * @param[out]  pParam: 注册成功后返回特征数据
 * @return  SAL_SOK
 */
INT32 Face_DrvUserRegister(UINT32 chan, void *pParam)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    UINT32 j = 0;
    UINT32 uiWidth = 0;
    UINT32 uiHeight = 0;
    UINT32 uiFaceCnt = 0;
    UINT32 uiFailCnt = 0;
    UINT32 uiPitch = 0;
    UINT32 uiFreeBufId = 0;

#if 1  /* 测试耗时使用，后续删除 */
    UINT64 time_start = 0;
    UINT64 time_before_input = 0;
    UINT64 time_result = 0;
    UINT64 time_judge_start = 0;
    UINT64 time_judge_mid = 0;
    UINT64 time_judge_end = 0;
    UINT64 time_reg_end_cpy = 0;
    UINT64 time_end = 0;
#endif

    void *pHandle = NULL;
    FACE_DEV_PRM *pstFace_Dev = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    ICF_INPUT_DATA_V2 *pstFaceAnaBuf = NULL;
    FACE_DSP_REGISTER_JPG_PARAM *pstJpegInfo = NULL;
    FACE_DSP_REGISTER_PARAM *pstFaceRegParam = NULL;
    FACE_PIC_REGISTER_OUT_S *pstPicProcOut = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;
    SAE_FACE_IN_DATA_INPUT_T *pstFaceInDataInfo = {0};

    FACE_REG_RESULT_S stRegResult = {0};
    FACE_DSP_DATABASE_PARAM stFaceDataBase = {0};
    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    /* 入参有效性判断 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pParam, SAL_FAIL);

    time_start = Face_HalgetTimeMilli();
    FACE_LOGI("Face_DrvUserRegister ! chan %d\n", chan);

    /* 接收应用下发相关参数 */
    pstFaceRegParam = (FACE_DSP_REGISTER_PARAM *)pParam;

    pstFace_Dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_Dev, SAL_FAIL);

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_PRM(pstFaceHalComm, SAL_FAIL);

    FACE_LOGI("chan %d RegCnt %d\n", chan, pstFaceRegParam->uiRegCnt);
    for (j = 0; j < pstFaceRegParam->uiRegCnt; j++)
    {
        FACE_LOGI("Reg Input: i %d length %d width %d height %d \n ", j,
                  pstFaceRegParam->stJpegData[j].uiLength,
                  pstFaceRegParam->stJpegData[j].uiWidth,
                  pstFaceRegParam->stJpegData[j].uiHeight);
    }

    if (SAL_TRUE != pstFace_Dev->uiChnStatus)
    {
        FACE_LOGE("chan %d is not started yet! Pls Check!\n", chan);
        return SAL_FAIL;
    }

    uiFaceCnt = pstFaceRegParam->uiRegCnt;

    /* 循环处理 */
    for (i = 0; i < uiFaceCnt; i++)
    {
        pstJpegInfo = &pstFaceRegParam->stJpegData[i];

        s32Ret = Face_DrvJpeg2Yuv(chan, pstJpegInfo, &pstFace_Dev->stRegisterFrameInfo, &stRegResult);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("Convert Jpeg to Yuv Failed! chan %d idx %d\n", chan, i);

            if (FACE_RESOLUTION_OVER_SIZE == stRegResult.eErrCode)
            {
                pstFaceRegParam->stFeatData[i].bFlag = FACE_REGISTER_OVERSIZE;
            }

            uiFailCnt++;
            continue;
        }

        if (stRegResult.eJdecMode >= FACE_JDEC_MODE_NUM)
        {
            FACE_LOGE("Invalid Jdec Mode %d, continue! chan %d i %d \n", stRegResult.eJdecMode, chan, i);
            continue;
        }

        if (SAL_SOK != sys_hal_getVideoFrameInfo(&pstFace_Dev->stRegisterFrameInfo, &stVideoFrmBuf))
        {
            FACE_LOGE("alloc video frame failed! chan %d \n", chan);
            return SAL_FAIL;
        }

        uiWidth = stVideoFrmBuf.stride[0];
        uiHeight = stVideoFrmBuf.frameParam.height;

		/* 图片未进行64对齐，可能会导致人脸算法无法检测到人脸或者报错，此处进行64对齐 */
		if (stVideoFrmBuf.frameParam.width % 64 || stVideoFrmBuf.frameParam.height % 64)
		{
			if (pstFace_Dev->stRegTmpFrameInfo.uiAppData == 0x00)
			{
				s32Ret = Face_DrvGetFrameMem(FACE_REGISTER_MAX_WIDTH, FACE_REGISTER_MAX_HEIGHT, &pstFace_Dev->stRegTmpFrameInfo);
				if (SAL_SOK != s32Ret)
				{
					FACE_LOGE("get frame mem failed! w %d, h %d \n", FACE_REGISTER_MAX_WIDTH, FACE_REGISTER_MAX_HEIGHT);
					return SAL_FAIL;
				}
			}

	        UINT32 uiRawWidth = uiWidth;
	        UINT32 uiRawHeight = uiHeight;

			uiWidth = SAL_align(stVideoFrmBuf.frameParam.width, 64) > FACE_REGISTER_MAX_WIDTH ? FACE_REGISTER_MAX_WIDTH : SAL_align(stVideoFrmBuf.frameParam.width, 64);
			uiHeight = SAL_align(stVideoFrmBuf.frameParam.height, 64) > FACE_REGISTER_MAX_HEIGHT ? FACE_REGISTER_MAX_HEIGHT : SAL_align(stVideoFrmBuf.frameParam.height, 64);

			/*申请新内存，用于临时存储64对齐后的图片*/
	        if (SAL_SOK != sys_hal_getVideoFrameInfo(&pstFace_Dev->stRegTmpFrameInfo, &stVideoFrmBuf))
	        {
	            FACE_LOGE("alloc video frame failed! chan %d \n", chan);
	            return SAL_FAIL;
	        }
			stVideoFrmBuf.frameParam.width = uiWidth;
			stVideoFrmBuf.frameParam.height = uiHeight;
			stVideoFrmBuf.stride[0] = stVideoFrmBuf.stride[1] = stVideoFrmBuf.stride[2] = uiWidth;
			
			stVideoFrmBuf.virAddr[1] = stVideoFrmBuf.virAddr[0] + uiWidth * uiHeight;
			stVideoFrmBuf.virAddr[2] = stVideoFrmBuf.virAddr[1];
			stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + uiWidth * uiHeight;
			stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];

	        if (SAL_SOK != sys_hal_buildVideoFrame(&stVideoFrmBuf, &pstFace_Dev->stRegTmpFrameInfo))
	        {
	            FACE_LOGE("build video frame failed! chan %d \n", chan);
	            return SAL_FAIL;
	        }

			/*此处修改是将原始jpg解码yuv图像，按照原始宽高填充到向上64对齐的大的背景图上，即存在一定的边沿填充*/
			s32Ret = Ia_TdeQuickCopy(&pstFace_Dev->stRegisterFrameInfo, 
				                     &pstFace_Dev->stRegTmpFrameInfo, 
				                     0, 0, uiRawWidth, uiRawHeight, SAL_FALSE);
			if (SAL_SOK != s32Ret)
			{
				FACE_LOGE("Ia_TdeQuickCopy failed! w %d, h %d \n", uiWidth, uiHeight);
				return SAL_FAIL;
			}
		
	        if (SAL_SOK != sys_hal_getVideoFrameInfo(&pstFace_Dev->stRegTmpFrameInfo, &stVideoFrmBuf))
	        {
	            FACE_LOGE("alloc video frame failed! chan %d \n", chan);
	            return SAL_FAIL;
	        }
			
			/*将帧结构体还原，避免影响下一次拷贝*/

			stVideoFrmBuf.frameParam.width = FACE_REGISTER_MAX_WIDTH;
			stVideoFrmBuf.frameParam.height = FACE_REGISTER_MAX_HEIGHT;
			stVideoFrmBuf.stride[0] = stVideoFrmBuf.stride[1] = stVideoFrmBuf.stride[2] = FACE_REGISTER_MAX_WIDTH;

			stVideoFrmBuf.virAddr[1] = stVideoFrmBuf.virAddr[0] + FACE_REGISTER_MAX_WIDTH * FACE_REGISTER_MAX_HEIGHT;
			stVideoFrmBuf.virAddr[2] = stVideoFrmBuf.virAddr[1];
			stVideoFrmBuf.phyAddr[1] = stVideoFrmBuf.phyAddr[0] + FACE_REGISTER_MAX_WIDTH * FACE_REGISTER_MAX_HEIGHT;
			stVideoFrmBuf.phyAddr[2] = stVideoFrmBuf.phyAddr[1];

	        if (SAL_SOK != sys_hal_buildVideoFrame(&stVideoFrmBuf, &pstFace_Dev->stRegTmpFrameInfo))
	        {
	            FACE_LOGE("build video frame failed! chan %d \n", chan);
	            return SAL_FAIL;
	        }
		}

        uiPitch = uiWidth;

        FACE_LOGI("chan %d i %d w %d h %d stride %d phy %llu vir %p\n",
                  chan, i, uiWidth, uiHeight, uiPitch, stVideoFrmBuf.phyAddr[0], (VOID *)stVideoFrmBuf.virAddr[0]);

        /* 将格式转换后的图片数据传入引擎接口 */
        if (SAL_SOK != Face_HalGetAnaFreeBuf(FACE_PICTURE_MODE, &uiFreeBufId))
        {
            FACE_LOGE("Get Free Buf Failed! Chan %d\n", chan);
            uiFailCnt++;
            continue;
        }

        pstFaceAnaBufTab = Face_HalGetAnaDataTab(FACE_PICTURE_MODE);
        pstFaceAnaBuf = &pstFaceAnaBufTab->stFaceBufData[uiFreeBufId];
        pstFaceInDataInfo = &pstFaceAnaBufTab->stFaceData[uiFreeBufId];
        /* *pstFaceAnaBuf->pUseFlag[uiFreeBufId] = 1; */

        pstFaceAnaBuf->nBlobNum = 1;
        pstFaceAnaBuf->stBlobData[0].nFrameNum = pstFaceAnaBufTab->uiFrameNum;

        /* 将转换后的NV21数据填充到缓存Buf，并送给人脸引擎 */
        memcpy((VOID *)pstFaceInDataInfo->data_info[0].yuv_data.y, (void *)stVideoFrmBuf.virAddr[0], uiHeight * uiWidth * 3 / 2);

        pstFaceInDataInfo->data_info[0].yuv_data.image_w = uiWidth;
        pstFaceInDataInfo->data_info[0].yuv_data.image_h = uiHeight;
        pstFaceInDataInfo->data_info[0].yuv_data.pitch_y = uiWidth;
        pstFaceInDataInfo->data_info[0].yuv_data.pitch_uv = uiWidth;
        /* pstFaceInDataInfo->data_info[0].yuv_data.y = (unsigned char *)source_buff->met_tab.pBase; */
        pstFaceInDataInfo->data_info[0].yuv_data.u = (unsigned char *)pstFaceInDataInfo->data_info[0].yuv_data.y + uiWidth * uiHeight;
        pstFaceInDataInfo->data_info[0].yuv_data.v = (unsigned char *)pstFaceInDataInfo->data_info[0].yuv_data.y + uiWidth * uiHeight;

        FACE_LOGW("uiWidth(%d) uiHeight(%d)\n", uiWidth, uiHeight);

#if 0  /* 调试接口 */
        Face_HalDumpNv21((CHAR *)pstFaceInDataInfo->data_info[0].yuv_data.y,
                         uiWidth * uiHeight * 3 / 2,
                         "./jpgd",
                         pstFaceAnaBufTab->uiFrameNum,
                         uiWidth, uiHeight);
#endif

        pHandle = Face_HalGetICFHandle(FACE_PICTURE_MODE, 0);
        if (NULL == pHandle)
        {
            FACE_LOGE("Get Vcae Handle Failed! Mode %d\n ", FACE_PICTURE_MODE);

            *(pstFaceAnaBuf->pUseFlag[0]) = SAL_FALSE;  /* 将人脸分析缓存使用标志位置为False */
            uiFailCnt++;
            continue;
        }

        pstPicProcOut = &pstFaceAnaBufTab->uFaceProcOut.stPicProcOut;

        /* 清空全局缓存，避免旧数据残留 */
        sal_memset_s(pstPicProcOut, sizeof(FACE_PIC_REGISTER_OUT_S), 0x00, sizeof(FACE_PIC_REGISTER_OUT_S));

        time_before_input = Face_HalgetTimeMilli();

        FACE_LOGI("before input cost %llu ms framenum(%llu) format(%d) y_addr(%p) u_arrd(%p) v_addr(%p) uiFreeBufId(%d)\n",
                  time_before_input - time_start, \
                  pstFaceAnaBuf->stBlobData[0].nFrameNum, pstFaceInDataInfo->data_info[0].yuv_data.format, \
                  pstFaceInDataInfo->data_info[0].yuv_data.y, \
                  pstFaceInDataInfo->data_info[0].yuv_data.u, \
                  pstFaceInDataInfo->data_info[0].yuv_data.v, uiFreeBufId);

        s32Ret = pstFaceHalComm->stIcfFuncP.IcfInputData(pHandle, pstFaceAnaBuf, sizeof(ICF_INPUT_DATA_V2));
        while (SAL_SOK != s32Ret)
        {
            FACE_LOGE("Input data error 0x%x\n", s32Ret);
            s32Ret = pstFaceHalComm->stIcfFuncP.IcfInputData(pHandle, pstFaceAnaBuf, sizeof(ICF_INPUT_DATA_V2));
            usleep(10000);
        }

        pstFaceAnaBufTab->uiFrameNum++;   /* 帧序号累加 */

        /* 等待算法结果回调 */
        sem_wait(&pstFaceAnaBufTab->sem);

        time_result = Face_HalgetTimeMilli();

        FACE_LOGI("Get Result !!!! i %d Mode %d cost %llu ms\n", i, FACE_PICTURE_MODE, time_result - time_before_input);

        SAL_clear(&stFaceDataBase);

        time_judge_start = Face_HalgetTimeMilli();

        /* 返回状态0为成功返回 */
        if (FACE_REGISTER_FAIL == pstPicProcOut->enProcSts)
        {
            uiFailCnt++;
            FACE_LOGE("i %d Get Output Feature Data Failed!\n", i);
        }
        else
        {
            memcpy(&stFaceDataBase.Featdata[0], pstPicProcOut->acFeaureData, FACE_FEATURE_LENGTH);

            time_judge_mid = Face_HalgetTimeMilli();
            if (SAL_SOK != Face_DrvJudgeFeatData(chan, &stFaceDataBase))
            {
				if(FACE_REGISTER_REPEAT == stFaceDataBase.bFlag)/* 数据库中已有特征数据 */
				{
                    FACE_LOGI("Face Existed already! chan %d\n", chan);
				}
				if(FACE_REGISTER_FAIL == stFaceDataBase.bFlag) 
				{
					FACE_LOGE("chan %d Face Register Failed!\n", chan);					
				}
            }
            else
            {
                stFaceDataBase.bFlag = FACE_REGISTER_SUCCESS;    /* 标记为建模成功 */
            }

            time_judge_end = Face_HalgetTimeMilli();

            FACE_LOGI("i %d Len %d mid cost %llu ms, cost %llu ms\n",
                      i, pstPicProcOut->u32FeatDataLen,
                      time_judge_end - time_judge_mid, time_judge_end - time_judge_start);
        }

		pstFaceRegParam->stFeatData[i].dataLen = FACE_FEATURE_LENGTH;
        memcpy(&pstFaceRegParam->stFeatData[i], &stFaceDataBase, sizeof(FACE_DSP_DATABASE_PARAM));

#if 0  /* 保存人脸建模数据 */
        Face_HalDumpFaceFeature((CHAR *)stFaceDataBase.Featdata, 272, "/home/config/register_dump", pstFaceAnaBufTab->uiFrameNum);
#endif

        time_reg_end_cpy = Face_HalgetTimeMilli();
        FACE_LOGI("chan %d Build Model i %d end, cost %llu ms ! bSuccess %d \n", chan, i, time_reg_end_cpy - time_judge_start, pstFaceRegParam->stFeatData[i].bFlag);
    }

    time_end = Face_HalgetTimeMilli();

    FACE_LOGI("Face Module: chan %d User Register End! uiFaceCnt %d, Fail %d times! last gap cost %llu ms, total cost %llu ms\n",
              chan, uiFaceCnt, uiFailCnt, time_end - time_reg_end_cpy, time_end - time_start);
    return SAL_SOK;
}

/**
 * @function    Face_DrvUserLogin
 * @brief         人脸登录
 * @param[in]  chan  : 通道号
 * @param[in]  pParam  : 人脸登录使用的参数
 * @param[out] pParam: 登录成功后返回人脸ID
 * @return SAL_SOK
 */
INT32 Face_DrvUserLogin(UINT32 chan, void *pParam)
{
    /* 变量定义 */
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    UINT32 uiVdecChn = 0;
    float fSim = 0.0;
    static UINT32 uiFailCnt = 0;
    static UINT32 uiLastStatus = 0;   /* 0表示上一次获取解码帧失败，1表示成功 */
    UINT32 uiErrTMout = 0;
	UINT32 u32TimeOut = 50;
    UINT64 time_frame_start = 0;
    UINT64 time_frame_end = 0;

#if 1  /* 测试耗时使用，后续删除 */
    UINT64 time_start = 0;
    UINT64 time_proc_start = 0;
    UINT64 time_proc_err_end = 0;
    UINT64 time_proc_ok_end = 0;
    UINT64 time_end = 0;
#endif


    FACE_DEV_PRM *pstFace_Dev = NULL;
    FACE_DSP_LOGIN_PARAM *pstLoginParam = NULL;
    FACE_DSP_LOGIN_OUTPUT_PARAM *pstLoginOutputParam = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    /* 入参有效性判断 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pParam, SAL_FAIL);

    time_start = Face_HalgetTimeMilli();
    FACE_LOGI("Face_DrvUserLogin ! chan %d\n", chan);

    pstLoginParam = (FACE_DSP_LOGIN_PARAM *)pParam;

    /* 接收应用传入解码通道号 */
    pstFace_Dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_Dev, SAL_FAIL);

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_PRM(pstFaceHalComm, SAL_FAIL);

    /* 通道未初始化，直接返回失败 */
    if (SAL_TRUE != pstFace_Dev->uiChnStatus)
    {
        FACE_LOGE("chan %d is not started yet! Pls Check!\n", chan);
        return SAL_FAIL;
    }

    pstLoginOutputParam = (FACE_DSP_LOGIN_OUTPUT_PARAM *)&pstLoginParam->stOutputData;

    /* 人脸登录解码通道号，用于获取人脸nv21数据 */
    uiVdecChn = pstFace_Dev->uiFrameFaceLogVdecChn;

    /* 人脸登录比对阈值 */
    fSim = pstFace_Dev->stIn.faceLoginThreshold;

    FACE_LOGI("login: chan %d VdecChn %d sim %.4f \n", chan, uiVdecChn, fSim);
	
#if 0	
	void *pHandle = NULL;

    /* 清空人脸登录选帧缓存 */
    pHandle = Face_HalGetICFHandle(FACE_VIDEO_LOGIN_MODE, 0);


    /* 人脸登录业务存在不确定帧间隔，需要在每次选帧前增加历史帧清空操作 */
    {
        ICF_CONFIG_PARAM_V2 configParam = {0};
        SAE_FACE_CFG_SELECT_FRAME_PARAM_T stSelectFrameParam = {0};

        /* 置位清空标记 */
        stSelectFrameParam.reset_flag = 1;

        configParam.nNodeID	= SAE_FACE_NID_TRACK_SELECT_1_SELECT_FRAME;
        configParam.nKey = SAE_FACE_CFG_FD_SELECT_FRAME_CLEAN;
        configParam.pConfigData = &stSelectFrameParam;
        configParam.nConfSize = sizeof(SAE_FACE_CFG_SELECT_FRAME_PARAM_T);

        s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetConfig(pHandle,
                                                        &configParam,
                                                        sizeof(ICF_CONFIG_PARAM_V2));
        FACE_DRV_CHECK_RET(s32Ret, err, "ICF_SetConfig SAE_FACE_CFG_FD_SELECT_FRAME_SETTING failed!");
    }
#endif

    for (i = 0; i < u32TimeOut; i++)
    {
        time_frame_start = Face_HalgetTimeMilli();
        do
        {
            s32Ret = Face_HalGetFrame(uiVdecChn, &pstFace_Dev->stLoginFrameInfo);
            if (SAL_SOK == s32Ret)
            {
                uiFailCnt = 0;
                uiLastStatus = 1;
                break;
            }
            else
            {
                /* 控制刷屏打印，连续失败120次打印一次 */
                if (0 == uiLastStatus)
                {
                    uiFailCnt++;
                    uiLastStatus = 0;
                    if (5 == uiFailCnt)
                    {
                        uiFailCnt = 0;
                        uiErrTMout = 1;
                        FACE_LOGE("Vdec %d Get Frame fail for 5 times! Pls Check if IPC is down or the VdecChan is stopped!\n", uiVdecChn);
                        return SAL_FAIL;
                    }
                }
                else
                {
                    uiLastStatus = 0;
                    FACE_LOGE("Vdec %d Get Frame fail!\n", uiVdecChn);
                    continue;
                }

                usleep(10 * 1000);
            }
        }
        while (1);

        time_frame_end = Face_HalgetTimeMilli();

        FACE_LOGI("Login: Get Frame cost %llu ms, ret %d\n", time_frame_end - time_frame_start, s32Ret);

        if (1 == uiErrTMout)
        {
            FACE_LOGE("Pls Check if IPC is DOWNNNNNNNN! chan %d\n", chan);
            return SAL_FAIL;
        }

        time_proc_start = Face_HalgetTimeMilli();

        s32Ret = Face_HalLoginProc(chan, fSim, &pstFace_Dev->stLoginFrameInfo, pstLoginOutputParam);
        if (SAL_SOK != s32Ret)
        {
            time_proc_err_end = Face_HalgetTimeMilli();
            FACE_LOGW("Login Proc Failed! chan %d i %d cost %llu ms\n", chan, i, time_proc_err_end - time_proc_start);
            continue;
        }
        else
        {
            time_proc_ok_end = Face_HalgetTimeMilli();
            FACE_LOGI("Login Proc Success! chan %d i %d cost %llu ms\n", chan, i, time_proc_ok_end - time_proc_start);
            break;
        }
    }

    time_end = Face_HalgetTimeMilli();

    if (u32TimeOut == i)
    {
        FACE_LOGE("Login Proc Failed for 50 times! chan %d last gap %llu ms, cost %llu ms\n", chan, time_end - time_proc_start, time_end - time_start);
        return SAL_FAIL;
    }

    FACE_LOGI("Face Module: chan %d User Login End! Face id %d, last gap %llu ms, cost total %llu ms\n",
              chan, pstLoginOutputParam->uiFaceId, time_end - time_proc_start, time_end - time_start);

    return SAL_SOK;
}

/**
 * @function    Face_DrvCompareFeatureData
 * @brief         人脸特征数据比对接口
 * @param[in]   chan-人脸通道
 * @param[in]    void *pParam  -输入结构体
 * @param[out]  NULL
 * @return   SAL_SOK
 */
INT32 Face_DrvCompareFeatureData(UINT32 chan, void *pParam)
{
    INT32 s32Ret = SAL_FAIL;

    float fScore = 0;

    FACE_COMPARE_DATA *pstFaceCmpData = NULL;
    FACE_COMMON_PARAM *pstComPrm = NULL;

    SAE_FACE_IN_DATA_FEATCMP_1V1_T stFeat_1v1 = {0};

    /* 入参有效性检验 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pParam, SAL_FAIL);

    pstFaceCmpData = (FACE_COMPARE_DATA *)pParam;

    pstComPrm = Face_HalGetComPrm();
    if (NULL == pstComPrm || pstComPrm->bInit != SAL_TRUE)
    {
        FACE_LOGE("Init Face module First!!!!!\n");
        return SAL_FAIL;
    }

    if (NULL == pstComPrm->pExternCompare)
    {
        FACE_LOGE("extern face complib err! handle null! \n");
        return SAL_FAIL;
    }

    stFeat_1v1.feat1 = pstFaceCmpData->stTestFace;
    stFeat_1v1.feat2 = pstFaceCmpData->stLibFace;
    stFeat_1v1.feat_len = FACE_FEATURE_LENGTH;
    s32Ret = SAE_FACE_DFR_Compare_Extern_1v1(pstComPrm->pExternCompare,
                                             &stFeat_1v1,
                                             sizeof(SAE_FACE_IN_DATA_FEATCMP_1V1_T),
                                             &fScore);
    FACE_DRV_CHECK_RET(s32Ret, err, "1v1 compare failed!");

    pstFaceCmpData->fScore = fScore;

    FACE_LOGI("SAE_FACE_DFR_Compare_Extern_1v1, get fscore :%f\n", fScore);
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Face_DrvSetConfig
 * @brief         人脸参数配置接口
 * @param[in]  chan  : 通道号
 * @param[in]  pParam: 配置参数
 * @param[out] NULL
 * @return  SAL_SOK
 */
INT32 Face_DrvSetConfig(UINT32 chan, void *pParam)
{
    INT32 s32Ret = SAL_FAIL;

    void *pHandle = NULL;
    FACE_DEV_PRM *pstFace_Dev = NULL;
    FACE_DSP_CONFIG_DATA *pstCfgData = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    /*  FRE_CFG_DET_CONF_MINTHRSH_T uiFaceCapConfig = { 0 }; */
    /*  FRE_CFG_TRK_RULE_LIST_T uiFaceRectConfig = { 0 }; */
    ICF_CONFIG_PARAM_V2 configParam = {0};
    SAE_FACE_CFG_TRACK_RULE_T stRuleList = {0};
    SAE_FACE_CFG_DET_CONF_THRESH_T stDetConfThresh = {0};

    FACE_DSP_CONFIG_TYPE_E enType = 0;
    FACE_DSP_CONFIG_CHANNEL_TYPE_E chanType = 0; /*确定参数类型 0-登录通道参数配置 1-抓拍通道参数配置*/

    /* 入参有效性检验 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pParam, SAL_FAIL);

    /* 入参本地化 */
    pstCfgData = (FACE_DSP_CONFIG_DATA *)pParam;

    pstFace_Dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_Dev, SAL_FAIL);

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_PRM(pstFaceHalComm, SAL_FAIL);

    /* 判断引擎是否初始化 */
    if (SAL_TRUE != pstFaceHalComm->bInit)
    {
        FACE_LOGE("Face Engine is not initialized! return fail!! \n");
        return SAL_FAIL;
    }

    enType = (FACE_DSP_CONFIG_TYPE_E)pstCfgData->enType;  /* 获取配置类型 */
    chanType = (FACE_DSP_CONFIG_CHANNEL_TYPE_E)pstCfgData->chanType;

    FACE_LOGI("Set FaceConfig !!!Chan (%d) Type(%d)\n", chanType, enType);

    /*人脸登录参数配置*/
    if (chanType == FACE_LOGIN_CHAN)
    {
        switch (enType)
        {
            case FACE_DETECT_THRESHOLD:     /* szl_face_todo: 后续需要整理到外部流程，人脸登录比对阈值配置 */
            {
                FACE_LOGI("from app: get det thresh %f for login line! \n", pstCfgData->stCfgParam.fFdThreshold);

                /* checker */
                if (pstCfgData->stCfgParam.fFdThreshold < fabs(1e-6) || pstCfgData->stCfgParam.fFdThreshold > 1.0f)
                {
                    FACE_LOGE("invalid fd thresh %f for login line \n", pstCfgData->stCfgParam.fFdThreshold);
                    return SAL_FAIL;
                }

                pstFace_Dev->stIn.faceLoginThreshold = pstCfgData->stCfgParam.fFdThreshold;
                FACE_LOGI("chan %d Set Face login Threshold %.4f End!\n", chan, pstFace_Dev->stIn.faceLoginThreshold);
                break;
            }
            case FACE_DETECT_REGION:    /*人脸登录区域配置*/
            {
#if 0  /* 当前人脸登录使用graph3业务线，该业务线不支持fd_detect_rule_list配置，后续如有需要可以切换到graph1 */
                node_id = FRE_NID_RCG_TRK;
                pHandle = Face_HalGetICFHandle(FACE_VIDEO_LOGIN_MODE);
                FACE_DRV_CHECK_PRM(pHandle, SAL_FAIL);

                uiFaceRectConfig.rule_list.rule_num	= 1;
                uiFaceRectConfig.rule_list.rule[0].enable = 1;
                uiFaceRectConfig.rule_list.rule[0].polygon.vertex_num = 4;
                uiFaceRectConfig.rule_list.rule[0].polygon.point[0].x = pstCfgData->stCfgParam.stRegion.x;
                uiFaceRectConfig.rule_list.rule[0].polygon.point[0].y = pstCfgData->stCfgParam.stRegion.y;
                uiFaceRectConfig.rule_list.rule[0].polygon.point[1].x = pstCfgData->stCfgParam.stRegion.x + pstCfgData->stCfgParam.stRegion.width;
                uiFaceRectConfig.rule_list.rule[0].polygon.point[1].y = pstCfgData->stCfgParam.stRegion.y;
                uiFaceRectConfig.rule_list.rule[0].polygon.point[2].x = pstCfgData->stCfgParam.stRegion.x + pstCfgData->stCfgParam.stRegion.width;
                uiFaceRectConfig.rule_list.rule[0].polygon.point[2].y = pstCfgData->stCfgParam.stRegion.y + pstCfgData->stCfgParam.stRegion.height;
                uiFaceRectConfig.rule_list.rule[0].polygon.point[3].x = pstCfgData->stCfgParam.stRegion.x;
                uiFaceRectConfig.rule_list.rule[0].polygon.point[3].y = pstCfgData->stCfgParam.stRegion.y + pstCfgData->stCfgParam.stRegion.height;
                uiFaceRectConfig.rule_list.rule[0].rule_type = REGION_RULE;
                uiFaceRectConfig.rule_list.rule[0].ID = 1;
                s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetConfig(pHandle, node_id, FRE_CFG_TRK_RULE_LIST, &uiFaceRectConfig, sizeof(FRE_CFG_TRK_RULE_LIST_T));
                ICF_CHECK_ERROR(SAL_SOK != s32Ret, s32Ret);
                FACE_LOGI("FRE_CFG_TRK_RULE_LIST: %d\n", uiFaceRectConfig.rule_list.rule[0].ID);

                uiFaceRectConfig.rule_list.rule[0].ID = 0;
                s32Ret = pstFaceHalComm->stIcfFuncP.IcfGetConfig(pHandle, node_id, FRE_CFG_TRK_RULE_LIST, &uiFaceRectConfig, sizeof(FRE_CFG_TRK_RULE_LIST_T));
                ICF_CHECK_ERROR(SAL_SOK != s32Ret, s32Ret);
                FACE_LOGI("FRE_CFG_TRK_RULE_LIST: %d\n", uiFaceRectConfig.rule_list.rule[0].ID);

                uiFaceRectConfig.rule_list.rule[0].ID = 1;
                s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetConfig(pHandle, node_id, FRE_CFG_TRK_RULE_LIST_IR, &uiFaceRectConfig, sizeof(FRE_CFG_TRK_RULE_LIST_T));
                ICF_CHECK_ERROR(SAL_SOK != s32Ret, s32Ret);
                FACE_LOGI("FRE_CFG_TRK_RULE_LIST_IR: %d\n", uiFaceRectConfig.rule_list.rule[0].ID);

                uiFaceRectConfig.rule_list.rule[0].ID = 0;
                s32Ret = pstFaceHalComm->stIcfFuncP.IcfGetConfig(pHandle, node_id, FRE_CFG_TRK_RULE_LIST_IR, &uiFaceRectConfig, sizeof(FRE_CFG_TRK_RULE_LIST_T));
                ICF_CHECK_ERROR(SAL_SOK != s32Ret, s32Ret);
                FACE_LOGI("FRE_CFG_TRK_RULE_LIST_IR: %d\n", uiFaceRectConfig.rule_list.rule[0].ID);
#endif
                if (pstCfgData->stCfgParam.stRegion.x < 0 ||pstCfgData->stCfgParam.stRegion.x > 1
                    || pstCfgData->stCfgParam.stRegion.y < 0||pstCfgData->stCfgParam.stRegion.y > 1
                    || pstCfgData->stCfgParam.stRegion.width < 0 ||pstCfgData->stCfgParam.stRegion.width > 1
                    || pstCfgData->stCfgParam.stRegion.height < 0||pstCfgData->stCfgParam.stRegion.height > 1
                    || pstCfgData->stCfgParam.stRegion.x + pstCfgData->stCfgParam.stRegion.width > 1
                    || pstCfgData->stCfgParam.stRegion.y + pstCfgData->stCfgParam.stRegion.height > 1)
                {
                    FACE_LOGE("Invalid Prm, Pls Check! x %.4f y %.4f w %.4f h %.4f x+w %.4f y+h %.4f\n",
                              pstCfgData->stCfgParam.stRegion.x,
                              pstCfgData->stCfgParam.stRegion.y,
                              pstCfgData->stCfgParam.stRegion.width,
                              pstCfgData->stCfgParam.stRegion.height,
                              pstCfgData->stCfgParam.stRegion.x + pstCfgData->stCfgParam.stRegion.width,
                              pstCfgData->stCfgParam.stRegion.y + pstCfgData->stCfgParam.stRegion.height);
                    return SAL_FAIL;
                }

                FACE_LOGI("login line: Set region x(%f) y(%f) wid(%f) hei(%f)\n",
                          pstCfgData->stCfgParam.stRegion.x, pstCfgData->stCfgParam.stRegion.y,
                          pstCfgData->stCfgParam.stRegion.width, pstCfgData->stCfgParam.stRegion.height);
                break;
            }
            default:
            {
                FACE_LOGE("Chan %d Get Invalid Config chan %d Type %d, Pls Check!\n", chan, chanType, enType);
                return SAL_FAIL;
            }
        }

        FACE_LOGI("Face Log Param Set Success!!!\n");
    }
    /*人脸抓拍参数配置*/
    else if (chanType == FACE_CAP_CHAN)
    {
        switch (enType)
        {
            case FACE_DETECT_THRESHOLD:    /*人脸抓拍阈值配置*/
            {
                FACE_LOGI("from app: get det thresh %f for videoCap line! \n", pstCfgData->stCfgParam.fFdThreshold);

                /* checker */
                if (pstCfgData->stCfgParam.fFdThreshold < fabs(1e-6) || pstCfgData->stCfgParam.fFdThreshold > 1.0f)
                {
                    FACE_LOGE("invalid fd thresh %f for videoCap line \n", pstCfgData->stCfgParam.fFdThreshold);
                    return SAL_FAIL;
                }

                pHandle = Face_HalGetICFHandle(FACE_VIDEO_CAP_MODE, 0);
                FACE_DRV_CHECK_PRM(pHandle, SAL_FAIL);

                stDetConfThresh.det_conf_thresh = pstCfgData->stCfgParam.fFdThreshold;

                memset(&configParam, 0, sizeof(configParam));

                configParam.nNodeID	= SAE_FACE_NID_TRACK_SELECT_DFR_DETECT;
                configParam.nKey = SAE_FACE_CFG_DET_CONF_THRESH;
                configParam.pConfigData	= &stDetConfThresh;
                configParam.nConfSize = sizeof(SAE_FACE_CFG_DET_CONF_THRESH_T);

                s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetConfig(pstFaceHalComm->astProcLineHandle[FACE_VIDEO_CAP_MODE].stIcfCreateHandle[0].pChannelHandle,
                                                                 &configParam,
                                                                 sizeof(ICF_CONFIG_PARAM_V2));
                FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "set det conf thresh failed!");

                memset(&stDetConfThresh, 0x00, sizeof(SAE_FACE_CFG_DET_CONF_THRESH_T));
                s32Ret = pstFaceHalComm->stIcfFuncP.IcfGetConfig(pstFaceHalComm->astProcLineHandle[FACE_VIDEO_CAP_MODE].stIcfCreateHandle[0].pChannelHandle,
                                                                 &configParam,
                                                                 sizeof(ICF_CONFIG_PARAM_V2));
                FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "get det conf thresh failed!");

                FACE_LOGI("Set SAE_FACE_CFG_DET_CONF_THRESH_T: %f\n", stDetConfThresh.det_conf_thresh);
                break;
            }
            case FACE_DETECT_REGION:   /*人脸抓拍区域配置*/
            {
                FACE_LOGI("from app: get face Cap x(%f) y(%f) wid(%f) hei(%f) for videoCap line! \n",
                          pstCfgData->stCfgParam.stRegion.x, pstCfgData->stCfgParam.stRegion.y,
                          pstCfgData->stCfgParam.stRegion.width, pstCfgData->stCfgParam.stRegion.height);

                pHandle = Face_HalGetICFHandle(FACE_VIDEO_CAP_MODE, 0);
                FACE_DRV_CHECK_PRM(pHandle, SAL_FAIL);

                if (pstCfgData->stCfgParam.stRegion.x < 0 ||pstCfgData->stCfgParam.stRegion.x > 1
                    || pstCfgData->stCfgParam.stRegion.y < 0||pstCfgData->stCfgParam.stRegion.y > 1
                    || pstCfgData->stCfgParam.stRegion.width < 0 ||pstCfgData->stCfgParam.stRegion.width > 1
                    || pstCfgData->stCfgParam.stRegion.height < 0||pstCfgData->stCfgParam.stRegion.height > 1
                    || pstCfgData->stCfgParam.stRegion.x + pstCfgData->stCfgParam.stRegion.width > 1
                    || pstCfgData->stCfgParam.stRegion.y + pstCfgData->stCfgParam.stRegion.height > 1)
                {
                    FACE_LOGE("Invalid Prm, Pls Check! x %.4f y %.4f w %.4f h %.4f x+w %.4f y+h %.4f\n",
                              pstCfgData->stCfgParam.stRegion.x,
                              pstCfgData->stCfgParam.stRegion.y,
                              pstCfgData->stCfgParam.stRegion.width,
                              pstCfgData->stCfgParam.stRegion.height,
                              pstCfgData->stCfgParam.stRegion.x + pstCfgData->stCfgParam.stRegion.width,
                              pstCfgData->stCfgParam.stRegion.y + pstCfgData->stCfgParam.stRegion.height);
                    return SAL_FAIL;
                }

                stRuleList.rule_list.rule_num = 1;
                stRuleList.rule_list.rule[0].enable = 1;
                stRuleList.rule_list.rule[0].polygon.vertex_num = 4;
                stRuleList.rule_list.rule[0].polygon.point[0].x = pstCfgData->stCfgParam.stRegion.x;
                stRuleList.rule_list.rule[0].polygon.point[0].y = pstCfgData->stCfgParam.stRegion.y;
                stRuleList.rule_list.rule[0].polygon.point[1].x = pstCfgData->stCfgParam.stRegion.x + pstCfgData->stCfgParam.stRegion.width;
                stRuleList.rule_list.rule[0].polygon.point[1].y = pstCfgData->stCfgParam.stRegion.y;
                stRuleList.rule_list.rule[0].polygon.point[2].x = pstCfgData->stCfgParam.stRegion.x + pstCfgData->stCfgParam.stRegion.width;
                stRuleList.rule_list.rule[0].polygon.point[2].y = pstCfgData->stCfgParam.stRegion.y + pstCfgData->stCfgParam.stRegion.height;
                stRuleList.rule_list.rule[0].polygon.point[3].x = pstCfgData->stCfgParam.stRegion.x;
                stRuleList.rule_list.rule[0].polygon.point[3].y = pstCfgData->stCfgParam.stRegion.y + pstCfgData->stCfgParam.stRegion.height;
                stRuleList.rule_list.rule[0].rule_type = 1; /* region */
                stRuleList.rule_list.rule[0].ID = 1;

                memset(&configParam, 0, sizeof(configParam));

                configParam.nNodeID	= SAE_FACE_NID_TRACK_SELECT_FD_TRACK;
                configParam.nKey = SAE_FACE_CFG_FD_TRK_RULE_LIST;
                configParam.pConfigData	= &stRuleList;
                configParam.nConfSize = sizeof(SAE_FACE_CFG_TRACK_RULE_T);

                s32Ret = pstFaceHalComm->stIcfFuncP.IcfSetConfig(pstFaceHalComm->astProcLineHandle[FACE_VIDEO_CAP_MODE].stIcfCreateHandle[0].pChannelHandle,
                                                                 &configParam,
                                                                 sizeof(ICF_CONFIG_PARAM_V2));
                FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "set rule list failed!");

                memset(&stRuleList, 0x00, sizeof(SAE_FACE_CFG_TRACK_RULE_T));
                s32Ret = pstFaceHalComm->stIcfFuncP.IcfGetConfig(pstFaceHalComm->astProcLineHandle[FACE_VIDEO_CAP_MODE].stIcfCreateHandle[0].pChannelHandle,
                                                                 &configParam,
                                                                 sizeof(ICF_CONFIG_PARAM_V2));
                FACE_DRV_CHECK_RET(SAL_SOK != s32Ret, err, "get rule list failed!");

                FACE_LOGI("videoCap line---set rule list: vertex_num %d, rule_type %d, id %d, lu[%f, %f], ru[%f, %f], rd[%f, %f], ld[%f, %f] \n",
                          stRuleList.rule_list.rule[0].polygon.vertex_num, stRuleList.rule_list.rule[0].rule_type,
                          stRuleList.rule_list.rule[0].ID,
                          stRuleList.rule_list.rule[0].polygon.point[0].x, stRuleList.rule_list.rule[0].polygon.point[0].y,
                          stRuleList.rule_list.rule[0].polygon.point[1].x, stRuleList.rule_list.rule[0].polygon.point[1].y,
                          stRuleList.rule_list.rule[0].polygon.point[2].x, stRuleList.rule_list.rule[0].polygon.point[2].y,
                          stRuleList.rule_list.rule[0].polygon.point[3].x, stRuleList.rule_list.rule[0].polygon.point[3].y);
                break;
            }
            default:
            {
                FACE_LOGE("Chan %d Get Invalid Config chan %d Type %d, Pls Check!\n", chan, chanType, enType);
                return SAL_FAIL;
            }
        }

        FACE_LOGI("Face Cap Param Set Success!!!\n");
    }
    else
    {
        FACE_LOGE("Face Set config prm Err!\n");
        return SAL_FAIL;
    }

    FACE_LOGI("Face Module: Set Config Param End! chan %d chanType %d  type %d \n", chan, chanType, enType);
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function    Face_DrvDeInitQue
 * @brief         去初始化队列参数
 * @param[in]  FACE_QUEUE_PRM *pstQuePrm 抓拍队列参数
 * @param[out] NULL
 * @return  SAL_SOK
 */
static INT32 Face_DrvDeInitQue(FACE_QUEUE_PRM *pstQuePrm)
{
    INT32 s32Ret = SAL_SOK;

    FACE_DRV_CHECK_PRM(pstQuePrm, SAL_FAIL);

    if (NULL != pstQuePrm->pstEmptQue)
    {
        s32Ret = DSA_QueDelete(pstQuePrm->pstEmptQue);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("error !!!\n");
            return SAL_FAIL;
        }
    }

    (VOID)Face_HalFree(pstQuePrm->pstEmptQue);

    if (NULL != pstQuePrm->pstFullQue)
    {
        s32Ret = DSA_QueDelete(pstQuePrm->pstFullQue);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("error !!!\n");
            return SAL_FAIL;
        }
    }

    (VOID)Face_HalFree(pstQuePrm->pstFullQue);

    FACE_LOGI("deinit Que end! \n");
    return SAL_SOK;

}

/**
 * @function    Face_DrvInitQue
 * @brief         初始化队列参数
 * @param[in]  *pstQuePrm-队列参数
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvInitQue(FACE_QUEUE_PRM *pstQuePrm)
{
    FACE_DRV_CHECK_PRM(pstQuePrm, SAL_FAIL);

    if (pstQuePrm->enType >= MAX_QUEUE_TYPE
        || 0 == pstQuePrm->uiBufCnt
        || 0 == pstQuePrm->uiBufSize)
    {
        FACE_LOGE("Invalid input args: type %d, buf_cnt %d, buf_size %d \n",
                  pstQuePrm->enType, pstQuePrm->uiBufCnt, pstQuePrm->uiBufSize);
        return SAL_FAIL;
    }

    if (SAL_SOK != Face_HalMalloc((VOID **)&pstQuePrm->pstEmptQue, sizeof(DSA_QueHndl)))
    {
        FACE_LOGE("malloc failed !!!\n");
        (VOID)Face_DrvDeInitQue(pstQuePrm);
        return SAL_FAIL;
    }

    if (SAL_SOK != DSA_QueCreate(pstQuePrm->pstEmptQue, pstQuePrm->uiBufCnt))
    {
        FACE_LOGE("error !!!\n");
        (VOID)Face_DrvDeInitQue(pstQuePrm);
        return SAL_FAIL;
    }

    if (DOUBLE_QUEUE_TYPE == pstQuePrm->enType)
    {
        if (SAL_SOK != Face_HalMalloc((VOID **)&pstQuePrm->pstFullQue, sizeof(DSA_QueHndl)))
        {
            FACE_LOGE("malloc failed !!!\n");
            (VOID)Face_DrvDeInitQue(pstQuePrm);
            return SAL_FAIL;
        }

        if (SAL_SOK != DSA_QueCreate(pstQuePrm->pstFullQue, pstQuePrm->uiBufCnt))
        {
            FACE_LOGE("error !!!\n");
            (VOID)Face_DrvDeInitQue(pstQuePrm);
            return SAL_FAIL;
        }
    }

    FACE_LOGI("init Que end! type %d, cnt %d, size %d \n", pstQuePrm->enType, pstQuePrm->uiBufCnt, pstQuePrm->uiBufSize);
    return SAL_SOK;
}

/**
 * @function    Face_DrvDeInitVideoCapAttrFeatQue
 * @brief         人脸抓拍业务线节点二队列去初始化
 * @param[in]  chan- 通道号
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvDeInitVideoCapAttrFeatQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    FACE_DEV_PRM *pstFace_dev = NULL;
    FACE_YUV_QUE_PRM *pstFaceYuvQueInfo = NULL;
    FACE_QUEUE_PRM *pstFaceYuvQuePrm = NULL;
    FACE_FRAME_INFO_ST *pstFaceYuvFrame = NULL;

    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstFace_dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_dev, SAL_FAIL);

    pstFaceYuvQueInfo = &pstFace_dev->stFaceYuvQueInfo;

    pstFaceYuvQuePrm = &pstFaceYuvQueInfo->pstFaceYuvQuePrm;

    for (i = 0; i < pstFaceYuvQuePrm->uiBufCnt; i++)
    {
        pstFaceYuvFrame = pstFaceYuvQueInfo->pstFaceYuvFrame[i];
        if (NULL == pstFaceYuvFrame)
        {
            FACE_LOGI("Yuv Frame has been deinitialized! continue. i %d \n", i);
            continue;
        }

        s32Ret = Face_DrvPutFrameMem(&pstFaceYuvFrame->stFrame);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Disp_halGetFrameMem error !!!\n");
        }

        Face_HalFree(pstFaceYuvQueInfo->pstFaceYuvFrame[i]);
    }

    s32Ret = Face_DrvDeInitQue(pstFaceYuvQuePrm);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Init Que Failed! \n");
    }

    FACE_LOGI("deinit Que Yuv Frm End! chan %d \n", chan);
    return SAL_SOK;
}

/**
 * @function    Face_DrvDeInitVideoCapSelQue
 * @brief         人脸抓拍业务线节点一队列去初始化
 * @param[in]  chan- 通道号
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvDeInitVideoCapSelQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    FACE_DEV_PRM *pstFace_dev = NULL;
    FACE_VIDEO_CAP_SELECT_QUE_PRM_S *pstFaceSelQueInfo = NULL;
    FACE_QUEUE_PRM *pstFaceSelQuePrm = NULL;

    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstFace_dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_dev, SAL_FAIL);

    pstFaceSelQueInfo = &pstFace_dev->stFaceVideoCapSelQueInfo;

    pstFaceSelQuePrm = &pstFaceSelQueInfo->stFaceSelQuePrm;

    for (i = 0; i < pstFaceSelQuePrm->uiBufCnt; i++)
    {
        s32Ret = Face_DrvPutFrameMem(&pstFaceSelQueInfo->stFaceSelInfo[i].stFrame);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Disp_halGetFrameMem error !!!\n");
        }
    }

    s32Ret = Face_DrvDeInitQue(pstFaceSelQuePrm);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Init Que Failed! \n");
    }

    FACE_LOGI("deinit Que Yuv Frm End! chan %d \n", chan);
    return SAL_SOK;
}

/**
 * @function    Face_DrvInitVideoCapSelectQue
 * @brief         初始化人脸抓拍第一个节点队列，用于获取选帧图像数据和人脸抠图结果
 * @param[in]  chan -通道号
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvInitVideoCapSelectQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    FACE_DEV_PRM *pstFace_dev = NULL;
    FACE_VIDEO_CAP_SELECT_QUE_PRM_S *pstFaceVideoCapSelQueInfo = NULL;
    FACE_QUEUE_PRM *pstFaceSelQuePrm = NULL;

    /* 参数合法性检测 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstFace_dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_dev, SAL_FAIL);

    /*获取全局人脸抓拍缓存帧数据*/
    pstFaceVideoCapSelQueInfo = &pstFace_dev->stFaceVideoCapSelQueInfo;
    pstFaceSelQuePrm = &pstFaceVideoCapSelQueInfo->stFaceSelQuePrm;

    pstFaceSelQuePrm->enType = DOUBLE_QUEUE_TYPE; /* 双队列 */
    pstFaceSelQuePrm->uiBufCnt = FACE_MAX_VIDEO_CAP_SELECT_QUE_DEPTH; /* 最大缓存个数 */
    pstFaceSelQuePrm->uiBufSize = sizeof(FACE_VIDEO_CAP_SELECT_QUE_DATA_S);

    s32Ret = Face_DrvInitQue(pstFaceSelQuePrm);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init queue failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    for (i = 0; i < pstFaceSelQuePrm->uiBufCnt; i++)
    {
        s32Ret = Face_DrvGetFrameMem(FACE_CAP_IMG_WIDTH,
                                     FACE_CAP_IMG_HEIGHT,
                                     &pstFaceVideoCapSelQueInfo->stFaceSelInfo[i].stFrame);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Err!\n");
            Face_DrvDeInitVideoCapSelQue(chan);
        }

        s32Ret = DSA_QuePut(pstFaceSelQuePrm->pstEmptQue,
                            (void *)&pstFaceVideoCapSelQueInfo->stFaceSelInfo[i],
                            SAL_TIMEOUT_NONE);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Que Put Err!\n");
            Face_DrvDeInitVideoCapSelQue(chan);
            return SAL_FAIL;
        }
    }

    FACE_LOGI("Init Que Yuv Frm End! chan %d, type %d, cnt %d, size %d \n",
              chan, pstFaceSelQuePrm->enType, pstFaceSelQuePrm->uiBufCnt, pstFaceSelQuePrm->uiBufSize);
    return SAL_SOK;

}

/**
 * @function    Face_DrvInitVideoCapAttrFeatQue
 * @brief         初始化人脸抓拍第二个节点队列，用于获取人脸属性和建模结果
 * @param[in]  chan -通道号
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvInitVideoCapAttrFeatQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    FACE_DEV_PRM *pstFace_dev = NULL;
    FACE_YUV_QUE_PRM *pstFaceYuvQueInfo = NULL;
    FACE_QUEUE_PRM *pstFaceYuvQuePrm = NULL;
    FACE_FRAME_INFO_ST *pstFaceYuvFrame = NULL;

    /* 参数合法性检测 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstFace_dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_dev, SAL_FAIL);

    /*获取全局人脸抓拍缓存帧数据*/
    pstFaceYuvQueInfo = &pstFace_dev->stFaceYuvQueInfo;
    pstFaceYuvQuePrm = &pstFaceYuvQueInfo->pstFaceYuvQuePrm;

    pstFaceYuvQuePrm->enType = DOUBLE_QUEUE_TYPE; /* 双队列 */
    pstFaceYuvQuePrm->uiBufCnt = FACE_MAX_YUV_BUF_NUM; /* 最大缓存个数 */
    pstFaceYuvQuePrm->uiBufSize = sizeof(FACE_FRAME_INFO_ST);

    s32Ret = Face_DrvInitQue(pstFaceYuvQuePrm);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init queue failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    for (i = 0; i < pstFaceYuvQuePrm->uiBufCnt; i++)
    {
        s32Ret = Face_HalMalloc((VOID **)&pstFaceYuvQueInfo->pstFaceYuvFrame[i], sizeof(FACE_FRAME_INFO_ST));
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Face Malloc Err!\n");
            Face_DrvDeInitVideoCapAttrFeatQue(chan);
            return SAL_FAIL;
        }

        pstFaceYuvFrame = pstFaceYuvQueInfo->pstFaceYuvFrame[i];

        s32Ret = Face_DrvGetFrameMem(FACE_CAP_IMG_WIDTH, FACE_CAP_IMG_HEIGHT, &pstFaceYuvFrame->stFrame);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Err!\n");
            Face_DrvDeInitVideoCapAttrFeatQue(chan);
        }

        s32Ret = DSA_QuePut(pstFaceYuvQuePrm->pstEmptQue, (void *)pstFaceYuvFrame, SAL_TIMEOUT_NONE);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Que Put Err!\n");
            Face_DrvDeInitVideoCapAttrFeatQue(chan);
            return SAL_FAIL;
        }
    }

    FACE_LOGI("Init Que Yuv Frm End! chan %d, type %d, cnt %d, size %d \n",
              chan, pstFaceYuvQuePrm->enType, pstFaceYuvQuePrm->uiBufCnt, pstFaceYuvQuePrm->uiBufSize);
    return SAL_SOK;

}

/**
 * @function   Face_DrvInitPrmQue
 * @brief      初始化配置参数队列
 * @param[in]  UINT32 chan
 * @param[out] None
 * @return     static INT32
 */
static INT32 Face_DrvInitPrmQue(UINT32 chan)
{
    /* 变量定义 */
    UINT32 i = 0;

    FACE_PROCESS_IN *pstIn = NULL;
    FACE_DEV_PRM *pstFace_Dev = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    /* 入参有效性检验 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstFace_Dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_Dev, SAL_FAIL);

    /* 初始化队列资源 */
    pstPrmFullQue = &pstFace_Dev->stPrmFullQue;
    pstPrmEmptQue = &pstFace_Dev->stPrmEmptQue;

    if (SAL_SOK != DSA_QueCreate(pstPrmFullQue, FACE_MAX_PRM_BUFFER))
    {
        FACE_LOGE("Create Prm Full Que Error !!!\n");
        return SAL_FAIL;
    }

    if (SAL_SOK != DSA_QueCreate(pstPrmEmptQue, FACE_MAX_PRM_BUFFER))
    {
        FACE_LOGE("Create Prm Empty Que Error !!!\n");
        return SAL_FAIL;
    }

    /* 填充默认值 */
    pstFace_Dev->stIn.faceLoginThreshold = 0.9f;         /*人脸登录 阈值 */
    pstFace_Dev->stIn.faceLoginRect.x = 0.01f;
    pstFace_Dev->stIn.faceLoginRect.y = 0.01f;
    pstFace_Dev->stIn.faceLoginRect.width = 0.9f;
    pstFace_Dev->stIn.faceLoginRect.height = 0.9f;

    pstFace_Dev->stIn.faceCapThreashold = 0.9f;         /* 人脸抓拍阈值 */
    pstFace_Dev->stIn.faceCapDetRect.x = 0.01f;
    pstFace_Dev->stIn.faceCapDetRect.y = 0.01f;
    pstFace_Dev->stIn.faceCapDetRect.width = 0.9f;
    pstFace_Dev->stIn.faceCapDetRect.height = 0.9f;

    for (i = 0; i < FACE_MAX_PRM_BUFFER; i++)
    {
        pstIn = (FACE_PROCESS_IN *)SAL_memZalloc(sizeof(FACE_PROCESS_IN), "FACE", "Face Prm");
        if (NULL == pstIn)
        {
            FACE_LOGE("alloc mmz for emtpy que failed!\n");
            return SAL_FAIL;
        }

        SAL_clear(pstIn);

        pstIn->faceLoginThreshold = pstFace_Dev->stIn.faceLoginThreshold;
        pstIn->faceLoginRect.x = pstFace_Dev->stIn.faceLoginRect.x;
        pstIn->faceLoginRect.y = pstFace_Dev->stIn.faceLoginRect.y;
        pstIn->faceLoginRect.width = pstFace_Dev->stIn.faceLoginRect.width;
        pstIn->faceLoginRect.height = pstFace_Dev->stIn.faceLoginRect.height;

        pstIn->faceCapThreashold = pstFace_Dev->stIn.faceCapThreashold;
        pstIn->faceCapDetRect.x = pstFace_Dev->stIn.faceCapDetRect.x;
        pstIn->faceCapDetRect.y = pstFace_Dev->stIn.faceCapDetRect.y;
        pstIn->faceCapDetRect.width = pstFace_Dev->stIn.faceCapDetRect.width;
        pstIn->faceCapDetRect.height = pstFace_Dev->stIn.faceCapDetRect.height;

        /* 默认参数放到队列中 */
        if (SAL_SOK != DSA_QuePut(pstPrmEmptQue, (void *)pstIn, SAL_TIMEOUT_NONE))
        {
            FACE_LOGE("error !!!\n");
            return SAL_FAIL;
        }
    }

    DSA_QueGet(pstPrmEmptQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
    DSA_QuePut(pstPrmFullQue, (void *)pstIn, SAL_TIMEOUT_NONE);

    return SAL_SOK;
}

/**
 * @function    Face_DrvDeInitVideoLoginSelQue
 * @brief         人脸登录业务线节点一队列去初始化
 * @param[in]  chan- 通道号
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvDeInitVideoLoginSelQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
    FACE_DEV_PRM *pstFace_dev = NULL;
    FACE_VIDEO_LOGIN_SELECT_QUE_PRM_S *pstFaceSelQueInfo = NULL;
    FACE_QUEUE_PRM *pstFaceSelQuePrm = NULL;

    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstFace_dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_dev, SAL_FAIL);

    pstFaceSelQueInfo = &pstFace_dev->stFaceVideoLoginSelQueInfo;

    pstFaceSelQuePrm = &pstFaceSelQueInfo->stFaceSelQuePrm;

    for (i = 0; i < pstFaceSelQuePrm->uiBufCnt; i++)
    {
        s32Ret = Face_DrvPutFrameMem(&pstFaceSelQueInfo->stFaceSelInfo[i].stFrame);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Disp_halGetFrameMem error !!!\n");
        }
    }

    s32Ret = Face_DrvDeInitQue(pstFaceSelQuePrm);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Init Que Failed! \n");
    }

    FACE_LOGI("deinit Que Yuv Frm End! chan %d \n", chan);
    return SAL_SOK;
}

/**
 * @function    Face_DrvInitVideoLoginSelectQue
 * @brief         初始化人脸登录第一个节点队列，用于获取选帧图像数据和人脸抠图结果
 * @param[in]  chan -通道号
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvInitVideoLoginSelectQue(UINT32 chan)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;

    FACE_DEV_PRM *pstFace_dev = NULL;
    FACE_VIDEO_LOGIN_SELECT_QUE_PRM_S *pstFaceVideoLoginSelQueInfo = NULL;
    FACE_QUEUE_PRM *pstFaceSelQuePrm = NULL;

    /* 参数合法性检测 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);

    pstFace_dev = Face_DrvGetDev(chan);
    FACE_DRV_CHECK_PRM(pstFace_dev, SAL_FAIL);

    /*获取全局人脸抓拍缓存帧数据*/
    pstFaceVideoLoginSelQueInfo = &pstFace_dev->stFaceVideoLoginSelQueInfo;
    pstFaceSelQuePrm = &pstFaceVideoLoginSelQueInfo->stFaceSelQuePrm;

    pstFaceSelQuePrm->enType = DOUBLE_QUEUE_TYPE; /* 双队列 */
    pstFaceSelQuePrm->uiBufCnt = FACE_MAX_VIDEO_CAP_SELECT_QUE_DEPTH; /* 最大缓存个数 */
    pstFaceSelQuePrm->uiBufSize = sizeof(FACE_VIDEO_LOGIN_SELECT_QUE_DATA_S);

    s32Ret = Face_DrvInitQue(pstFaceSelQuePrm);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("init queue failed! chan %d \n", chan);
        return SAL_FAIL;
    }

    for (i = 0; i < pstFaceSelQuePrm->uiBufCnt; i++)
    {
        s32Ret = Face_DrvGetFrameMem(FACE_CAP_IMG_WIDTH,
                                     FACE_CAP_IMG_HEIGHT,
                                     &pstFaceVideoLoginSelQueInfo->stFaceSelInfo[i].stFrame);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Err!\n");
            Face_DrvDeInitVideoLoginSelQue(chan);
        }

        s32Ret = DSA_QuePut(pstFaceSelQuePrm->pstEmptQue,
                            (void *)&pstFaceVideoLoginSelQueInfo->stFaceSelInfo[i],
                            SAL_TIMEOUT_NONE);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Que Put Err!\n");
            Face_DrvDeInitVideoLoginSelQue(chan);
            return SAL_FAIL;
        }
    }

    FACE_LOGI("Init Que Yuv Frm End! chan %d, type %d, cnt %d, size %d \n",
              chan, pstFaceSelQuePrm->enType, pstFaceSelQuePrm->uiBufCnt, pstFaceSelQuePrm->uiBufSize);
    return SAL_SOK;

}

/**
 * @function    Face_DrvInitDefaultInputParam
 * @brief          初始化人脸识别配置参数
 * @param[in]   chan-通道号
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvInitDefaultInputParam(UINT32 chan)
{
    /* 初始化参数队列 */
    if (SAL_SOK != Face_DrvInitPrmQue(chan))
    {
        FACE_LOGE("init Face Yuv Que Err\n");
        return SAL_FAIL;
    }

    /* 初始化人脸登录业务线队列_1: 选帧使用 */
    if (SAL_SOK != Face_DrvInitVideoLoginSelectQue(chan))
    {
        FACE_LOGE("init video cap select Queue Err\n");
        return SAL_FAIL;
    }

    /* 初始化人脸抓拍业务线队列_1: 选帧使用 */
    if (SAL_SOK != Face_DrvInitVideoCapSelectQue(chan))
    {
        FACE_LOGE("init video cap select Queue Err\n");
        return SAL_FAIL;
    }

    /* 初始化人脸抓拍业务线队列_2: 人脸属性及建模 */
    if (SAL_SOK != Face_DrvInitVideoCapAttrFeatQue(chan))
    {
        FACE_LOGE("init video cap attr feature Err\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Face_DrvEncJpeg
 * @brief         人脸jpg编图
 * @param[in]  chan-通道号
 * @param[in]  pstFrameInfo-帧数据
 * @param[in]  *uiCropInfo-抠图参数
 * @param[out]  jpg
 * @return SAL_SOK
 */
static INT32 Face_DrvEncJpeg(UINT32 chan, SYSTEM_FRAME_INFO *pstSysFrameInfo, CROP_S *pstCropInfo, UINT32 uiJpgIdx)
{
    INT32 s32Ret = SAL_FAIL;

    JPEG_COMMON_ENC_PRM_S jpegEncPrm = {0};

    FACE_DEV_PRM *pstFace_Dev = NULL;

    pstFace_Dev = Face_DrvGetDev(chan);

    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pstSysFrameInfo, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pstFace_Dev, SAL_FAIL);

    FACE_LOGW("Get Face Num Rect--- enable(%d) x(%d) y(%d) hei(%d) wid(%d)\n", \
              pstCropInfo->u32CropEnable, pstCropInfo->u32X, pstCropInfo->u32Y, pstCropInfo->u32W, pstCropInfo->u32H);

    jpegEncPrm.pOutJpeg = pstFace_Dev->stJpegVenc.pPicJpeg[uiJpgIdx];
    sal_memcpy_s(&jpegEncPrm.stSysFrame, sizeof(SYSTEM_FRAME_INFO), pstSysFrameInfo, sizeof(SYSTEM_FRAME_INFO));

    s32Ret = jpeg_drv_cropEnc(pstFace_Dev->stJpegVenc.pstJpegChnInfo,
                              &jpegEncPrm,
                              pstCropInfo);
    if (s32Ret != SAL_SOK)
    {
        FACE_LOGE("Jpg Enc Err!\n");
        return SAL_FAIL;
    }

    /* 填充编图结果的图像大小 */
    pstFace_Dev->stJpegVenc.uiPicJpegSize = jpegEncPrm.outSize;

    return SAL_SOK;
}

/**
 * @function    Face_DrvInputDataToEngine
 * @brief         向引擎推送抓拍帧数据
 * @param[in] chan-通道号
 * @param[in] pstSysFrmInfo-帧数据
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvInputDataToEngine(UINT32 chan, SYSTEM_FRAME_INFO *pstSysFrmInfo)
{
    /* 变量定义 */
    INT32 s32Ret = SAL_SOK;

    UINT32 uiFreeBufId = 0;

    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    ICF_INPUT_DATA_V2 *pstFaceAnaBuf = NULL;
    SAE_FACE_IN_DATA_INPUT_T *pstFaceInDataInfo = NULL;
    SYSTEM_FRAME_INFO *pstSystemFrame = NULL;
    void *pVir = NULL;
    void *pHandle = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    SAL_VideoFrameBuf stVideoFrmBuf = {0};

    /* 入参判断 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pstSysFrmInfo, SAL_FAIL);

    /* 入参本地化 */
    pstSystemFrame = (SYSTEM_FRAME_INFO *)pstSysFrmInfo;

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_PRM(pstFaceHalComm, SAL_FAIL);

    if (SAL_SOK != sys_hal_getVideoFrameInfo(pstSystemFrame, &stVideoFrmBuf))
    {
        FACE_LOGE("get video frame info failed! \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != Face_HalGetAnaFreeBuf(FACE_VIDEO_CAP_MODE, &uiFreeBufId))
    {
        FACE_LOGE("Get Free Buf Failed! Chan %d\n", chan);
        return SAL_FAIL;
    }

    pVir = (void *)stVideoFrmBuf.virAddr[0];
    if (NULL == pVir)
    {
        FACE_LOGE("pVir == NULL!!! chan %d\n", chan);
        return SAL_FAIL;
    }

    FACE_LOGD("get cap free id %d \n", uiFreeBufId);

    pstFaceAnaBufTab = Face_HalGetAnaDataTab(FACE_VIDEO_CAP_MODE);
    pstFaceAnaBuf = &pstFaceAnaBufTab->stFaceBufData[uiFreeBufId];
    pstFaceInDataInfo = &pstFaceAnaBufTab->stFaceData[uiFreeBufId];

    /* 填充帧号 */
    pstFaceAnaBuf->stBlobData[0].nFrameNum = pstFaceAnaBufTab->uiFrameNum;
    pstFaceInDataInfo->data_info[0].frame_num = pstFaceAnaBufTab->uiFrameNum;

    /* 拷贝nv21数据送入引擎 */
    memcpy(pstFaceInDataInfo->data_info[0].yuv_data.y,
           (void *)pVir,
           FACE_CAP_IMG_WIDTH * FACE_CAP_IMG_HEIGHT * 3 / 2);

#if 0
    Face_HalDumpNv21((CHAR *)pstFaceInDataInfo->data_info[0].yuv_data.y,
                     FACE_CAP_IMG_WIDTH * FACE_CAP_IMG_HEIGHT * 3 / 2,
                     "./cap", pstFaceAnaBufTab->uiFrameNum, FACE_CAP_IMG_WIDTH, FACE_CAP_IMG_HEIGHT);
#endif

    /* 模块控制参数 */
    pstFaceInDataInfo->proc_type = SAE_FACE_PROC_TYPE_TRACK_SELECT;
    pstFaceInDataInfo->face_proc_type = SAE_FACE_FACE_PROC_TYPE_MULTI_FACE;  /* 开启多人脸才会进行多人脸抓拍选帧 */
    pstFaceInDataInfo->error_proc_type = SAE_FACE_ERROR_PROC_MODULE; /* SAE_FACE_ERROR_PROC_MODULE; */
    pstFaceInDataInfo->ls_img_enable = 0;
    pstFaceInDataInfo->priv_data = NULL;
    pstFaceInDataInfo->priv_data_size = 0;
    pstFaceInDataInfo->liveness_type = SAE_FACE_LIVENESS_TYPE_DISABLE;
    pstFaceInDataInfo->track_enable = 1;     /* 此业务线，必须track_enable使能，否则就会报错 */
    pstFaceInDataInfo->compare_enable = 1;  /* 此业务线，该功能无效 */

    pHandle = Face_HalGetICFHandle(FACE_VIDEO_CAP_MODE, 0);
    if (NULL == pHandle)
    {
        *(pstFaceAnaBuf->pUseFlag[0]) = SAL_FALSE;

        FACE_LOGE("Get Vcae Handle Failed! Mode %d\n ", FACE_VIDEO_CAP_MODE);
        return SAL_FAIL;
    }

    /* 将yuv数据传入引擎接口,获取抓拍数据 */
    s32Ret = pstFaceHalComm->stIcfFuncP.IcfInputData(pHandle, pstFaceAnaBuf, sizeof(ICF_INPUT_DATA_V2));
    while (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Input data error 0x%x. Mode %d \n", s32Ret, FACE_VIDEO_CAP_MODE);
        s32Ret = pstFaceHalComm->stIcfFuncP.IcfInputData(pHandle, pstFaceAnaBuf, sizeof(ICF_INPUT_DATA_V2));
        usleep(10000);
    }

    FACE_LOGD("++++++ INPUT DATA TO SELECT, frameNum %d, buf id %d \n", pstFaceAnaBufTab->uiFrameNum, uiFreeBufId);

    pstFaceAnaBufTab->uiFrameNum++;   /* 帧序号累加 */
    return SAL_SOK;
}

/**
 * @function    Face_DrvVideoLoginFeatThread
 * @brief         人脸登录业务线基于选帧结果送入建模节点
 * @param[in]  *prm - 人脸通道全局变量
 * @param[out] NULL
 * @return NULl
 */
void *Face_DrvVideoLoginFeatThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 uiFreeBufId = 0;

    DSA_QueHndl *pstSelFullQue = NULL;
    DSA_QueHndl *pstSelEmptQue = NULL;
    FACE_DEV_PRM *pstFace_Dev = NULL;
    FACE_VIDEO_LOGIN_SELECT_QUE_DATA_S *pstVideoLoginSelQueBuf = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    ICF_INPUT_DATA_V2 *pstInputData = NULL;
    SAE_FACE_IN_DATA_INPUT_T *pstFaceInDataInfo = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;
    void *pHandle = NULL;

    SAE_FACE_IN_DATA_INPUT_T *pstTmp = NULL;

    SAL_VideoFrameBuf stSrcVideoFrmBuf = {0};

    /* 入参有效性判断 */
    FACE_DRV_CHECK_PRM(prm, NULL);

    /* 入参本地化 */
    pstFace_Dev = (FACE_DEV_PRM *)prm;

    /* 选帧结果队列 */
    pstSelFullQue = pstFace_Dev->stFaceVideoLoginSelQueInfo.stFaceSelQuePrm.pstFullQue;
    pstSelEmptQue = pstFace_Dev->stFaceVideoLoginSelQueInfo.stFaceSelQuePrm.pstEmptQue;

    pstFaceAnaBufTab = Face_HalGetAnaDataTab(FACE_VIDEO_LOGIN_MODE);
    FACE_DRV_CHECK_RET(NULL == pstFaceAnaBufTab, exit, "pstFaceAnaBufTab == null!");

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_RET(NULL == pstFaceHalComm, exit, "pstFaceHalComm == null!");

    SAL_SetThreadCoreBind(3); /* 人脸抓拍算法处理线程绑核 */
    SAL_SET_THR_NAME();

    while (1)
    {
        /* 获取人脸登录业务线节点一的数据，包括背景图数据和人脸区域 */
        s32Ret = Face_DrvGetQueBuf(pstSelFullQue, (void **)&pstVideoLoginSelQueBuf, SAL_TIMEOUT_FOREVER);
        if (SAL_SOK != s32Ret || NULL == pstVideoLoginSelQueBuf)
        {
            usleep(500 * 1000);
            continue;
        }

        FACE_LOGI("====== login: get from select frame queue! \n");

        /* 送入节点二，获取人脸建模数据 */
        {
            /* 清空上一次的处理结果 */
            memset(&pstFaceAnaBufTab->uFaceProcOut.stVideoLoginProcOut, 0x00, sizeof(FACE_VIDEO_LOGIN_OUT_S));

            if (SAL_SOK != Face_HalGetAnaFreeBuf(FACE_VIDEO_LOGIN_MODE, &uiFreeBufId))
            {
                FACE_LOGE("Get Free Buf Failed! \n");
                goto reuse;
            }

            pstInputData = &pstFaceAnaBufTab->stFaceBufData[uiFreeBufId];
            pstFaceInDataInfo = &pstFaceAnaBufTab->stFaceData[uiFreeBufId];

            pstInputData->stBlobData[0].nFrameNum = pstFaceAnaBufTab->uiFrameNum_priv;

            /* 模块控制参数 */
            pstFaceInDataInfo->proc_type = SAE_FACE_PROC_TYPE_DET_FEATURE;
            pstFaceInDataInfo->face_proc_type = SAE_FACE_FACE_PROC_TYPE_SINGLE_FACE_BIGGEST;  /* 开启多人脸才会进行多人脸抓拍选帧 */
            pstFaceInDataInfo->error_proc_type = SAE_FACE_ERROR_PROC_FACE;//SAE_FACE_ERROR_PROC_MODULE
            pstFaceInDataInfo->ls_img_enable = 0;
            pstFaceInDataInfo->attribute_enable = 0;
            pstFaceInDataInfo->liveness_type = SAE_FACE_LIVENESS_TYPE_DISABLE;
            pstFaceInDataInfo->track_enable = 0;
            pstFaceInDataInfo->compare_enable = 0;
            pstFaceInDataInfo->priv_data = NULL;
            pstFaceInDataInfo->priv_data_size = 0;

            if (SAL_SOK != sys_hal_getVideoFrameInfo(&pstVideoLoginSelQueBuf->stFrame, &stSrcVideoFrmBuf))
			{
				FACE_LOGE("get video frame info failed! \n");
				goto exit;
			}

            /* 拷贝nv21数据送入引擎 */
            memcpy(pstFaceInDataInfo->data_info[0].yuv_data.y,
                   (VOID *)stSrcVideoFrmBuf.virAddr[0],
                   FACE_LOGIN_IMG_WIDTH * FACE_LOGIN_IMG_HEIGHT * 3 / 2);

            /* 获取人脸登录句柄 */
            pHandle = Face_HalGetICFHandle(FACE_VIDEO_LOGIN_MODE, 1);
            FACE_DRV_CHECK_RET(NULL == pHandle, exit, "pHandle == null!");

            s32Ret = pstFaceHalComm->stIcfFuncP.IcfInputData(pHandle, pstInputData, sizeof(ICF_INPUT_DATA_V2));
            if (SAL_SOK != s32Ret)
            {
                *(pstInputData->pUseFlag[0]) = SAL_FALSE;

                FACE_LOGE("login: input feature failed! \n");
                goto reuse;
            }

            FACE_LOGI("======== login: input data for face feature! \n");

            pstTmp = (SAE_FACE_IN_DATA_INPUT_T *)pstInputData->stBlobData[0].pData;
            FACE_LOGI("========= proc_type %d, face_proc_type %d, error_proc_type %d, ls_img_enable %d, attribute_enable %d, liveness_type %d, track_enable %d, compare_enable %d \n",
                      pstTmp->proc_type,
                      pstTmp->face_proc_type,
                      pstTmp->error_proc_type,
                      pstTmp->ls_img_enable,
                      pstTmp->attribute_enable,
                      pstTmp->liveness_type,
                      pstTmp->track_enable,
                      pstTmp->compare_enable);

            /* 帧号累加 */
            pstFaceAnaBufTab->uiFrameNum_priv++;
        }

reuse:
        /*释放帧数据*/
        if (NULL != pstVideoLoginSelQueBuf)
        {
            (VOID)Face_DrvPutQueBuf(pstSelEmptQue, pstVideoLoginSelQueBuf, SAL_TIMEOUT_NONE);
            pstVideoLoginSelQueBuf = NULL;
        }
    }

exit:
    return NULL;
}

/**
 * @function    Face_DrvVideoCapAttrFeatThread
 * @brief         人脸抓拍业务线基于选帧结果获取人脸属性和人脸建模数据
 * @param[in]  *prm - 人脸通道全局变量
 * @param[out] NULL
 * @return NULl
 */
void *Face_DrvVideoCapAttrFeatThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0;
	UINT32 u32Cnt = 0;
    UINT32 uiFreeBufId = 0;

    DSA_QueHndl *pstSelFullQue = NULL;
    DSA_QueHndl *pstSelEmptQue = NULL;
    DSA_QueHndl *pstJencFullQue = NULL;
    DSA_QueHndl *pstJencEmptQue = NULL;
    FACE_FRAME_INFO_ST *pstFaceYuvFrame = NULL;
    FACE_DEV_PRM *pstFace_Dev = NULL;
    FACE_VIDEO_CAP_SELECT_QUE_DATA_S *pstVideoCapSelQueBuf = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    ICF_INPUT_DATA_V2 *pstInputData = NULL;
    SAE_FACE_IN_DATA_INPUT_T *pstFaceInDataInfo = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;
    void *pHandle = NULL;
	FACE_VIDEO_CAPTURE_OUT_S *pstVideoCapOut = NULL;

    SAL_VideoFrameBuf stSrcVideoFrmBuf = {0};
    SAL_VideoFrameBuf stDstVideoFrmBuf = {0};

    /* 入参有效性判断 */
    FACE_DRV_CHECK_PRM(prm, NULL);

    /* 入参本地化 */
    pstFace_Dev = (FACE_DEV_PRM *)prm;

    /* 选帧结果队列 */
    pstSelFullQue = pstFace_Dev->stFaceVideoCapSelQueInfo.stFaceSelQuePrm.pstFullQue;
    pstSelEmptQue = pstFace_Dev->stFaceVideoCapSelQueInfo.stFaceSelQuePrm.pstEmptQue;

    /* 编图队列 */
    pstJencFullQue = pstFace_Dev->stFaceYuvQueInfo.pstFaceYuvQuePrm.pstFullQue;
    pstJencEmptQue = pstFace_Dev->stFaceYuvQueInfo.pstFaceYuvQuePrm.pstEmptQue;

    pstFaceAnaBufTab = Face_HalGetAnaDataTab(FACE_VIDEO_CAP_MODE);
    FACE_DRV_CHECK_RET(NULL == pstFaceAnaBufTab, exit, "pstFaceAnaBufTab == null!");

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_RET(NULL == pstFaceHalComm, exit, "pstFaceHalComm == null!");

    SAL_SetThreadCoreBind(3); /* 人脸抓拍算法处理线程绑核 */
    SAL_SET_THR_NAME();

    while (1)
    {
        /* 获取人脸抓拍业务线节点一的数据，包括背景图数据和人脸区域 */
        s32Ret = Face_DrvGetQueBuf(pstSelFullQue, (void **)&pstVideoCapSelQueBuf, SAL_TIMEOUT_FOREVER);
        if (SAL_SOK != s32Ret || NULL == pstVideoCapSelQueBuf)
        {
            usleep(500 * 1000);
            continue;
        }

        FACE_LOGI("====== cap: get from select frame queue! \n");

        s32Ret = Face_DrvGetQueBuf(pstJencEmptQue, (void **)&pstFaceYuvFrame, SAL_TIMEOUT_NONE);
        while (SAL_SOK != s32Ret || NULL == pstFaceYuvFrame)
        {
            FACE_LOGE("get jenc empty queue data failed! continue \n");
            usleep(100 * 1000);

            s32Ret = Face_DrvGetQueBuf(pstJencEmptQue, (void **)&pstFaceYuvFrame, SAL_TIMEOUT_NONE);
            continue;
        }

        /* 拷贝图像数据和人脸区域信息 */
        {
            s32Ret = sys_hal_getVideoFrameInfo(&pstVideoCapSelQueBuf->stFrame, &stSrcVideoFrmBuf);
			if (SAL_SOK != s32Ret)
			{
				FACE_LOGE("get video frame info, src failed! \n");
				goto reuse;
			}
			
            s32Ret = sys_hal_getVideoFrameInfo(&pstFaceYuvFrame->stFrame, &stDstVideoFrmBuf);
			if (SAL_SOK != s32Ret)
			{
				FACE_LOGE("get video frame info, dst failed! \n");
				goto reuse;
			}
			
            memcpy((VOID *)stDstVideoFrmBuf.virAddr[0],
                   (VOID *)stSrcVideoFrmBuf.virAddr[0],
                   FACE_CAP_IMG_WIDTH * FACE_CAP_IMG_HEIGHT * 3 / 2);

            pstFaceYuvFrame->u32FaceNum = pstVideoCapSelQueBuf->u32FaceNum;
            for (i = 0; i < pstFaceYuvFrame->u32FaceNum && i < FACE_MAX_SELECT_FACE_NUM_IN_ONE_FRAME; i++)
            {
                memcpy(&pstFaceYuvFrame->stFaceRec[i], &pstVideoCapSelQueBuf->stFaceRec[i], sizeof(FACE_RECT));
            }
        }

        /* 送入节点二，获取人脸属性和人脸建模数据 */
        {
            /* 清空上一次的处理结果 */
            memset(&pstFaceAnaBufTab->uFaceProcOut.stVideoCapProcOut, 0x00, sizeof(FACE_VIDEO_CAPTURE_OUT_S));

            if (SAL_SOK != Face_HalGetAnaFreeBuf(FACE_VIDEO_CAP_MODE, &uiFreeBufId))
            {
                FACE_LOGE("Get Free Buf Failed! \n");
                goto reuse;
            }

            pstInputData = &pstFaceAnaBufTab->stFaceBufData[uiFreeBufId];
            pstFaceInDataInfo = &pstFaceAnaBufTab->stFaceData[uiFreeBufId];

            pstFaceInDataInfo->data_info[0].frame_num = pstFaceAnaBufTab->uiFrameNum_priv;

            /* 模块控制参数 */
            pstFaceInDataInfo->proc_type = SAE_FACE_PROC_TYPE_DET_FEATURE;
            pstFaceInDataInfo->face_proc_type = SAE_FACE_FACE_PROC_TYPE_SINGLE_FACE_BIGGEST;  /* 开启多人脸才会进行多人脸抓拍选帧 */
            pstFaceInDataInfo->error_proc_type = SAE_FACE_ERROR_PROC_FACE;//SAE_FACE_ERROR_PROC_MODULE
            pstFaceInDataInfo->ls_img_enable = 0;
            pstFaceInDataInfo->attribute_enable = 0;
            pstFaceInDataInfo->liveness_type = SAE_FACE_LIVENESS_TYPE_DISABLE;//SAE_FACE_LIVENESS_TYPE_VL;
            pstFaceInDataInfo->track_enable = 0;
            pstFaceInDataInfo->compare_enable = 0;  /*图片建模，不进行对比 */
            pstFaceInDataInfo->priv_data = NULL;
            pstFaceInDataInfo->priv_data_size = 0;

            /* 拷贝nv21数据送入引擎 */
            memcpy(pstFaceInDataInfo->data_info[0].yuv_data.y,
                   (VOID *)stDstVideoFrmBuf.virAddr[0],
                   FACE_CAP_IMG_WIDTH * FACE_CAP_IMG_HEIGHT * 3 / 2);

            /* 获取人脸抓拍句柄 */
            pHandle = Face_HalGetICFHandle(FACE_VIDEO_CAP_MODE, 1);
            FACE_DRV_CHECK_RET(NULL == pHandle, exit, "pHandle == null!");

            s32Ret = pstFaceHalComm->stIcfFuncP.IcfInputData(pHandle, pstInputData, sizeof(ICF_INPUT_DATA_V2));
            if (SAL_SOK != s32Ret)
            {
                *(pstInputData->pUseFlag[0]) = SAL_FALSE;

                FACE_LOGE("cap: input feature failed! \n");
                goto reuse;
            }

            FACE_LOGI("======== cap: input data for face feature! \n");

            /* 帧号累加 */
            pstFaceAnaBufTab->uiFrameNum_priv++;
        }

        sem_wait(&pstFaceAnaBufTab->sem);

        FACE_LOGI("cap: get attr and feature result! feature len %d \n",
                  pstFaceAnaBufTab->uFaceProcOut.stVideoCapProcOut.au32FeatDataLen[0]);

		pstVideoCapOut = &pstFaceAnaBufTab->uFaceProcOut.stVideoCapProcOut;

        /* 将获取到的feat和attr拷贝到编图队列buf中 */
        if (FACE_REGISTER_SUCCESS == pstVideoCapOut->enProcSts)
        {
        	for(pstFaceYuvFrame->u32FaceNum = 0, u32Cnt = 0, i = 0;\
				i < pstVideoCapOut->u32FaceNum;\
				i++)
        	{
        		/* 拷贝人脸建模数据 */
	            memcpy((UINT8 *)pstFaceYuvFrame->acFaceFeature[i], 
					   (UINT8 *)pstVideoCapOut->acFeaureData[i], 
					   pstVideoCapOut->au32FeatDataLen[i]);

				/* 拷贝人脸属性数据 (目前无法获取人脸属性数据，copy值为空)*/
	            memcpy(&pstFaceYuvFrame->astFaceAttrubute[i], 
					   &pstVideoCapOut->astFaceAttribute[i], 
					   sizeof(FACE_ATTRIBUTE_DSP_OUT));

				u32Cnt++;
        	}

			pstFaceYuvFrame->u32FaceNum = u32Cnt;

            /* 送入编图队列 */
            Face_DrvPutQueBuf(pstJencFullQue, (void *)pstFaceYuvFrame, SAL_TIMEOUT_NONE);
            pstFaceYuvFrame = NULL;
        }
        else
        {
            Face_DrvPutQueBuf(pstJencEmptQue, (void *)pstFaceYuvFrame, SAL_TIMEOUT_NONE);
            pstFaceYuvFrame = NULL;
        }

        /* 释放选帧帧数据队列缓存 */
        if (NULL != pstVideoCapSelQueBuf)
        {
            (VOID)Face_DrvPutQueBuf(pstSelEmptQue, pstVideoCapSelQueBuf, SAL_TIMEOUT_NONE);
            pstVideoCapSelQueBuf = NULL;
        }

        continue;

reuse:  /* 异常处理分支，需要将队列数据释放 */
        if (NULL != pstVideoCapSelQueBuf)
        {
            (VOID)Face_DrvPutQueBuf(pstSelEmptQue, pstVideoCapSelQueBuf, SAL_TIMEOUT_NONE);
            pstVideoCapSelQueBuf = NULL;
        }

        if (NULL != pstFaceYuvFrame)
        {
            (VOID)Face_DrvPutQueBuf(pstJencEmptQue, pstFaceYuvFrame, SAL_TIMEOUT_NONE);
            pstFaceYuvFrame = NULL;
        }
    }

exit:
    return NULL;
}

/**
 * @function    Face_DrvCapProThread
 * @brief         人脸抓拍算法处理线程
 * @param[in]  *prm - 人脸通道全局变量
 * @param[out] NULL
 * @return NULl
 */
void *Face_DrvCapProThread(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i = 0, k = 0;

    DSA_QueHndl *pstYuvFullQue = NULL;
    DSA_QueHndl *pstYuvEmptQue = NULL;
    FACE_FRAME_INFO_ST *pstFaceYuvFrame = NULL;
    FACE_DEV_PRM *pstFace_Dev = NULL;

    FACE_CAP_DSP_OUT stFaceDspOut = {0}; /* 回调数据 */
    CROP_S stCropInfo = {0};
    STREAM_ELEMENT stStreamEle = {0};

    /* 入参有效性判断 */
    FACE_DRV_CHECK_PRM(prm, NULL);

    /* 入参本地化 */
    pstFace_Dev = (FACE_DEV_PRM *)prm;

    pstYuvFullQue = pstFace_Dev->stFaceYuvQueInfo.pstFaceYuvQuePrm.pstFullQue;
    pstYuvEmptQue = pstFace_Dev->stFaceYuvQueInfo.pstFaceYuvQuePrm.pstEmptQue;

    SAL_SetThreadCoreBind(3); /* 人脸抓拍算法处理线程绑核 */
    SAL_SET_THR_NAME();

    while (1)
    {
        /*阻塞等待,确定通道状态建立并且开启抓拍才执行*/
        if ((SAL_TRUE != pstFace_Dev->uiChnStatus) || (SAL_TRUE != pstFace_Dev->bUpFrmThStart))
        {
            usleep(200 * 1000);
            continue;
        }

        /*等待抓拍结果*/
        s32Ret = Face_DrvGetQueBuf(pstYuvFullQue, (void **)&pstFaceYuvFrame, SAL_TIMEOUT_NONE);
        if (SAL_SOK != s32Ret || NULL == pstFaceYuvFrame)
        {
            usleep(500 * 1000);
            continue;
        }

        if (pstFaceYuvFrame->u32FaceNum <= 0)
        {
            FACE_LOGW("Err Frame!,pstFaceYuvFrame->u32FaceNum:%d\n",pstFaceYuvFrame->u32FaceNum);
            goto reuse;
        }

        /* copy回调数据结果 */
        sal_memset_s(&stStreamEle, sizeof(STREAM_ELEMENT), 0, sizeof(STREAM_ELEMENT));
        sal_memset_s(&stFaceDspOut, sizeof(FACE_CAP_DSP_OUT), 0, sizeof(FACE_CAP_DSP_OUT));

        stStreamEle.type = STREAM_ELEMENT_FACE_CAP;
        stStreamEle.chan = pstFace_Dev->chan;
        SAL_getDateTime(&stStreamEle.absTime);

        /* 获取抓拍数据,回调给应用 */
        stFaceDspOut.ullTimePts = SAL_getCurMs();
        stFaceDspOut.face_num = pstFaceYuvFrame->u32FaceNum;
        for (i = 0; i < stFaceDspOut.face_num && i < MAX_CAP_FACE_NUM; i++)
        {
            /* 属性信息 */
            stFaceDspOut.face_attrubute[i].face_glass = pstFaceYuvFrame->astFaceAttrubute[i].face_glass;
            stFaceDspOut.face_attrubute[i].face_mask = pstFaceYuvFrame->astFaceAttrubute[i].face_mask;
            stFaceDspOut.face_attrubute[i].face_hat	= pstFaceYuvFrame->astFaceAttrubute[i].face_hat;
            stFaceDspOut.face_attrubute[i].face_beard = pstFaceYuvFrame->astFaceAttrubute[i].face_beard;
            stFaceDspOut.face_attrubute[i].face_gender = pstFaceYuvFrame->astFaceAttrubute[i].face_gender;
            stFaceDspOut.face_attrubute[i].face_race = pstFaceYuvFrame->astFaceAttrubute[i].face_race;
            stFaceDspOut.face_attrubute[i].face_nation = pstFaceYuvFrame->astFaceAttrubute[i].face_nation;
            stFaceDspOut.face_attrubute[i].face_experess = pstFaceYuvFrame->astFaceAttrubute[i].face_experess;
            stFaceDspOut.face_attrubute[i].face_age	= pstFaceYuvFrame->astFaceAttrubute[i].face_age;
            stFaceDspOut.face_attrubute[i].face_age_satge = pstFaceYuvFrame->astFaceAttrubute[i].face_age_satge;
            stFaceDspOut.face_attrubute[i].face_hair_style = pstFaceYuvFrame->astFaceAttrubute[i].face_hair_style;
            stFaceDspOut.face_attrubute[i].face_shap = pstFaceYuvFrame->astFaceAttrubute[i].face_shap;

            /* feature 信息 */
            sal_memcpy_s((void *)&stFaceDspOut.face_feature_data[i], FACE_FEATURE_LENGTH, &pstFaceYuvFrame->acFaceFeature[i], FACE_FEATURE_LENGTH);
        }

        /*jpg编图--抠图编图*/
        for (k = 0; k < pstFaceYuvFrame->u32FaceNum && k < MAX_CAP_FACE_NUM; k++)
        {
            stCropInfo.u32CropEnable = SAL_TRUE;
            stCropInfo.u32X = SAL_alignDown((UINT32)(pstFaceYuvFrame->stFaceRec[k].x * FACE_CAP_IMG_WIDTH), 2);
            stCropInfo.u32Y = SAL_alignDown((UINT32)(pstFaceYuvFrame->stFaceRec[k].y * FACE_CAP_IMG_HEIGHT), 2);
            stCropInfo.u32W = SAL_align((UINT32)(pstFaceYuvFrame->stFaceRec[k].width * FACE_CAP_IMG_WIDTH), 4);
            stCropInfo.u32H = SAL_align((UINT32)(pstFaceYuvFrame->stFaceRec[k].height * FACE_CAP_IMG_HEIGHT), 4);

            /*抠图区域保护，避免抠图区域超出图片*/
            if(stCropInfo.u32X + stCropInfo.u32W > FACE_CAP_IMG_WIDTH)
            {
                stCropInfo.u32X = FACE_CAP_IMG_WIDTH - stCropInfo.u32W;
            }
            if(stCropInfo.u32Y + stCropInfo.u32H > FACE_CAP_IMG_HEIGHT)
            {
                stCropInfo.u32Y = FACE_CAP_IMG_HEIGHT - stCropInfo.u32H;
            }

            s32Ret = Face_DrvEncJpeg(0, &pstFaceYuvFrame->stFrame, &stCropInfo, k);
            if (s32Ret != SAL_SOK)
            {
                FACE_LOGE("Face Jpg Enc Err!\n");
                goto reuse;
            }

            stFaceDspOut.face_result[k].cJpegAddr = (UINT8 *)pstFace_Dev->stJpegVenc.pPicJpeg[k];
            stFaceDspOut.face_result[k].uiJpegSize = pstFace_Dev->stJpegVenc.uiPicJpegSize;
        }

        /*编图 --背景图*/
        stCropInfo.u32CropEnable = SAL_TRUE;
        stCropInfo.u32X = 0;
        stCropInfo.u32Y = 0;
        stCropInfo.u32W = FACE_CAP_IMG_WIDTH;
        stCropInfo.u32H = FACE_CAP_IMG_HEIGHT;
        s32Ret = Face_DrvEncJpeg(0, &pstFaceYuvFrame->stFrame, &stCropInfo, 3);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Face Jpg Enc Err!\n");
            goto reuse;
        }

        stFaceDspOut.face_background_result.cJpegAddr = (UINT8 *)pstFace_Dev->stJpegVenc.pPicJpeg[3];
        stFaceDspOut.face_background_result.uiJpegSize = pstFace_Dev->stJpegVenc.uiPicJpegSize;

        FACE_LOGI("Face Cap to app:face_num(%d) -------------new4------------!!!!!!!!!!!\n", stFaceDspOut.face_num);
        SystemPrm_cbFunProc(&stStreamEle, (unsigned char *)&stFaceDspOut, sizeof(FACE_CAP_DSP_OUT));
reuse:
        /*释放帧数据*/
        if (NULL != pstFaceYuvFrame)
        {
            (VOID)Face_DrvPutQueBuf(pstYuvEmptQue, pstFaceYuvFrame, SAL_TIMEOUT_NONE);
            pstFaceYuvFrame = NULL;
        }
    }

    return NULL;
}

/**
 * @function    Face_DrvSendFrameDataProc
 * @brief         发送人脸抓拍帧线程
 * @param[in]  prm - 人脸通道全局变量
 * @param[out] NULL
 * @return NULL
 */
void *Face_DrvSendFrameDataProc(void *prm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 chan = 0;
    UINT32 faceCapVdecChn = 0;
    UINT32 curTime = 0;
    UINT32 lastTime = 0;
    UINT32 uiGapTime = 80;
    UINT32 uiLastStatus = 0;
    UINT32 uiFailCnt = 0;
    UINT32 uiQueueCnt = 0;

    FACE_DEV_PRM *pstFace_Dev = NULL;
    FACE_PROCESS_IN *pstIn = NULL;
    DSA_QueHndl *pstPrmFullQue = NULL;
    DSA_QueHndl *pstPrmEmptQue = NULL;

    /* 入参有效性判断 */
    FACE_DRV_CHECK_PRM(prm, NULL);

    pstFace_Dev = (FACE_DEV_PRM *)prm;

    pstPrmFullQue = &pstFace_Dev->stPrmFullQue;
    pstPrmEmptQue = &pstFace_Dev->stPrmEmptQue;

    if (0x00 == pstFace_Dev->stCapFrameInfo.uiAppData)
    {
        s32Ret = Face_DrvGetFrameMem(FACE_CAP_IMG_WIDTH, FACE_CAP_IMG_HEIGHT, &pstFace_Dev->stCapFrameInfo);
        if (s32Ret != SAL_SOK)
        {
            FACE_LOGE("Get Frame Mem error !!!\n");
            return NULL;
        }
    }

    SAL_SET_THR_NAME();

    while (1)
    {
        /*更新参数输入*/
        uiQueueCnt = DSA_QueGetQueuedCount(pstPrmFullQue);
        if (uiQueueCnt)
        {
            /* 获取 buffer */
            DSA_QueGet(pstPrmFullQue, (void **)&pstIn, SAL_TIMEOUT_FOREVER);
            memcpy(&pstFace_Dev->stIn, pstIn, sizeof(FACE_PROCESS_IN));
            DSA_QuePut(pstPrmEmptQue, (void *)pstIn, SAL_TIMEOUT_NONE);
        }

        /*确定人脸抓拍开启*/
        if (SAL_TRUE != pstFace_Dev->uiChnStatus || SAL_TRUE != pstFace_Dev->bUpFrmThStart)
        {
            usleep(200 * 1000);
            continue;
        }

        faceCapVdecChn = pstFace_Dev->uiFrameFaceCapVdecChn;

        curTime = SAL_getCurMs();

        /*保证获取解码数据时间*/
        if (curTime < lastTime + uiGapTime)
        {
            usleep((uiGapTime + lastTime - curTime) * 1000);
            continue;
        }

        /*获取解码帧数据*/
        if (SAL_SOK != Face_HalGetFrame(faceCapVdecChn, &pstFace_Dev->stCapFrameInfo))
        {
            usleep(80 * 1000);

            if (0 == uiLastStatus)
            {
                uiFailCnt++;
                uiLastStatus = 0;

                /*连续多帧数据未获取到进行错误打印*/
                if (GET_FRAME_DATA_FAIL_CNT == uiFailCnt)
                {
                    uiFailCnt = 0;
                    FACE_LOGE("Vdec %d Get Frame fail for 120 times! Pls Check if IPC is down or the VdecChan is stopped!\n", faceCapVdecChn);
                }
            }
            else
            {
                uiLastStatus = 0;
                FACE_LOGE("Vdec %d Get Frame fail!\n", faceCapVdecChn);
            }

            continue;
        }
        /*获取解码数据成功*/
        else
        {
            uiFailCnt = 0;
            uiLastStatus = 1;
        }

#if 0
        static UINT32 cnt = 0;
        static CHAR path[64] = {0};
        VIDEO_FRAME_INFO_S *pstFrameInfo = NULL;
        SAL_clear(path);
        pstFrameInfo = (VIDEO_FRAME_INFO_S *)pstFace_Dev->stCapFrameInfo.uiAppData;
        sprintf(path, "./face/output/cap/input_%d_test_%dx%d.yuv", cnt, pstFrameInfo->stVFrame.u32Width, pstFrameInfo->stVFrame.u32Height);

        (VOID)Face_HalDebugDumpData((CHAR *)pstFrameInfo->stVFrame.u64VirAddr[0], (CHAR *)path, FACE_CAP_IMG_WIDTH * FACE_CAP_IMG_HEIGHT * 3 / 2);

        cnt++;

#endif
        /* 向引擎推送数据帧 */
        (void)Face_DrvInputDataToEngine(chan, &pstFace_Dev->stCapFrameInfo);

        lastTime = curTime;
    }
}

/**
 * @function    Face_DrvModuleInit
 * @brief         人脸模块初始化
 * @param[in]  chan-通道号
 * @param[in]  *pPrm-通道号
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_DrvModuleInit(UINT32 chan, void *pPrm)
{
    INT32 s32Ret = SAL_FAIL;

    FACE_DEV_PRM *pstFace_Dev = NULL;
    FACE_DSP_CHANNEL_NUM_PARAM *pstFaceChanPrm = NULL;
	FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    SAL_VideoFrameParam stHalPrm = {0};

    /* 入参有效性判断 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);
    FACE_DRV_CHECK_PRM(pPrm, SAL_FAIL);

    pstFaceChanPrm = (FACE_DSP_CHANNEL_NUM_PARAM *)pPrm;

    FACE_LOGI("from app: get login vdecChn %d, cap vdecChn %d \n", pstFaceChanPrm->uiFaceLoginChan, pstFaceChanPrm->uiFaceCapChan);

    pstFace_Dev = Face_DrvGetDev(chan);

    pstFaceHalComm = Face_HalGetComPrm();
    FACE_DRV_CHECK_RET(NULL == pstFaceHalComm, err, "pstFaceHalComm == null!");

    if (SAL_TRUE != pstFaceHalComm->bInit)
    {
        FACE_LOGE("Face Engine is not initialized! return fail!! \n");
        goto err;
    }

    if (SAL_TRUE == pstFace_Dev->uiChnStatus)
    {
        FACE_LOGI("Face Module: chan %d is already Init! return Success!\n", chan);
        return SAL_SOK;
    }

    pstFace_Dev->uiFrameFaceLogVdecChn = pstFaceChanPrm->uiFaceLoginChan;
    pstFace_Dev->uiFrameFaceCapVdecChn = pstFaceChanPrm->uiFaceCapChan;

    /* 人脸抓拍需要创建编码通道 */
    if (SAL_FALSE == pstFace_Dev->stJpegVenc.bDecChnStatus)
    {
        sal_memset_s(&stHalPrm, sizeof(SAL_VideoFrameParam), 0x00, sizeof(SAL_VideoFrameParam));

        stHalPrm.fps = 1;
        stHalPrm.width = FACE_CAP_IMG_WIDTH;
        stHalPrm.height = FACE_CAP_IMG_WIDTH;
        stHalPrm.encodeType = MJPEG;
        stHalPrm.quality = 80;
        stHalPrm.dataFormat = SAL_VIDEO_DATFMT_YUV420SP_VU;

        s32Ret = jpeg_drv_createChn((void **)&pstFace_Dev->stJpegVenc.pstJpegChnInfo, &stHalPrm);
        if (s32Ret != SAL_SOK)
        {
            SVA_LOGE("jpeg_drv_createChn error !!!\n");
        }

        pstFace_Dev->stJpegVenc.bDecChnStatus = SAL_TRUE;

        FACE_LOGI("Face Create Dec Chan Success! bDecChnStatus %d\n", pstFace_Dev->stJpegVenc.bDecChnStatus);
    }

    pstFace_Dev->uiChnStatus = SAL_TRUE;
    g_face_common.uiDevCnt++;

    FACE_LOGI("Face Module: chan %d Init OK! Current Dev Chn %d\n", chan, g_face_common.uiDevCnt);

    return SAL_SOK;
err:
	return SAL_FAIL;
}

/**
 * @function    Face_DrvModuleDeInit
 * @brief         人脸模块去初始化
 * @param[in]  chan: 通道号
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_DrvModuleDeInit(UINT32 chan)
{

    /* 入参有效性判断 */
    FACE_DRV_CHECK_CHAN(chan, SAL_FAIL);

    FACE_DEV_PRM *pstFace_Dev = Face_DrvGetDev(chan);

    if (SAL_FALSE == pstFace_Dev->uiChnStatus)
    {
        FACE_LOGI("Face Module: chan %d is already DeInit! return Success!\n", chan);
        return SAL_SOK;
    }

    pstFace_Dev->uiChnStatus = SAL_FALSE;

    g_face_common.uiDevCnt--;

    FACE_LOGI("Face Module: chan %d DeInit OK! Current Dev Cnt %d\n", chan, g_face_common.uiDevCnt);

    return SAL_SOK;
}

/**
 * @function    Face_DrvDataBaseInit
 * @brief         本地数据库初始化
 * @param[in]  NULL
 * @param[out] NULL
 * @return SAL_SOK
 */
UINT32 Face_DrvDataBaseInit(VOID)
{
    UINT32 i = 0;
    FACE_MODEL_DATA_BASE *pstDataBase = NULL;

    pstDataBase = Face_HalGetDataBase();

    if (NULL == pstDataBase)
    {
        FACE_LOGE("Face DataBase is NULL! Pls Check!\n");
        return SAL_FAIL;
    }

    pstDataBase->uiModelCnt = 0;
    pstDataBase->uiMaxModelCnt = MAX_FACE_NUM;
    for (i = 0; i < pstDataBase->uiMaxModelCnt; i++)
    {
        pstDataBase->pFeatureData[i] = NULL;
        pstDataBase->pFeatureData[i] = SAL_memZalloc(FACE_TEATURE_HEADER_LENGTH + FACE_FEATURE_LENGTH, "FACE", "Face DataBase");

        if (0x00 == (PhysAddr)pstDataBase->pFeatureData[i])
        {
            FACE_LOGE("Face DataBase Malloc MMZ Failed! i %d\n ", i);
            return SAL_FAIL;
        }
    }

    FACE_LOGI("Init Face DataBase Success!\n");

    return SAL_SOK;
}

/**
 * @function    Face_DrvDeInitGlobalData
 * @brief         人脸全局参数去初始化接口
 * @param[in]  chanNum - 通道个数
 * @param[out] NULL
 * @return SAL_SOK
 */


static INT32 Face_DrvDeInitGlobalData(UINT32 chanNum)
{
    FACE_DEV_PRM *pstFace_Dev = NULL;
    INT32 s32Ret = SAL_FAIL;
    UINT32 i = 0;
    UINT32 k = 0;
    FACE_JDEC_BUF_INFO *pstJdecBufInfo = NULL;

    for (i = 0; i < chanNum; i++)
    {
        pstFace_Dev = Face_DrvGetDev(i);
        pstJdecBufInfo = Face_HalGetJdecBuf();

        /*人脸jpg软编YUV缓存释放*/
        s32Ret = mem_hal_vbFree((Ptr)pstFace_Dev->stFaceYuvMem.pVirAddr, "FACE", "face_jdec_tmp_buf", pstFace_Dev->stFaceYuvMem.bufLen, pstFace_Dev->stFaceYuvMem.u64VbBlk, pstFace_Dev->stFaceYuvMem.u32PoolId);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("Free Vb Buf For Jdec Failed! chan %d\n", i);
            return SAL_FAIL;
        }

        /*图片解码缓存数据资源释放*/
        s32Ret = Face_HalFree((VOID *)pstJdecBufInfo->stBufData[0].VirAddr);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("Free Vb Buf For Jdec Failed! chan %d\n", i);
            return SAL_FAIL;
        }

        /*jpg编图内存释放*/
        for (k = 0; k < FACE_MAX_PIC_FRAME_NUM; k++)
        {
            s32Ret = SAL_memfree((Ptr)pstFace_Dev->stJpegVenc.pPicJpeg[k], "FACE", "Face jpg");
            if (SAL_SOK != s32Ret)
            {
                FACE_LOGE("Free Vb Buf For JpegVenc Failed! chan %d\n", i);
                return SAL_FAIL;
            }
        }

        s32Ret = sys_hal_rleaseVideoFrameInfoSt(&pstFace_Dev->stRegisterFrameInfo);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("release Register frame info failed! \n");
            return SAL_FAIL;
        }

        if (0x00 != pstFace_Dev->stLoginFrameInfo.uiAppData)
        {
            s32Ret = Face_DrvPutFrameMem(&pstFace_Dev->stLoginFrameInfo);
            if (SAL_SOK != s32Ret)
            {
                FACE_LOGE("Put LoginFrame Mem Failed! \n");
                return SAL_FAIL;
            }
        }
        if (0x00 != pstFace_Dev->stCapFrameInfo.uiAppData)
        {
            s32Ret = Face_DrvPutFrameMem(&pstFace_Dev->stCapFrameInfo);
            if (SAL_SOK != s32Ret)
            {
                FACE_LOGE("Put CapFrame Mem Failed!\n");
                return SAL_FAIL;
            }
        }
    }
    return SAL_SOK;

}


/**
 * @function    Face_DrvDeInit
 * @brief         人脸模块资源去初始化
 * @param[in]  NULL
 * @param[out] NULL
 * @return SAL_SOK
 */
UINT32 Face_DrvDeInit(VOID)
{
    UINT32 s32Ret = SAL_FAIL;

    s32Ret = Face_HalDeInit();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGI("Face Module: model resource Fail!\n");
        return SAL_FAIL;
    }

    s32Ret = Face_DrvDeInitGlobalData(1);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Global Data DeInit Fail!\n");
        return SAL_FAIL;
    }

    FACE_LOGI("Face Module: model resource deInit OK!\n");
    return SAL_SOK;
}

/**
 * @function    Face_DrvInitGlobalData
 * @brief         人脸全局参数初始化接口
 * @param[in]  chanNum - 通道个数
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvInitGlobalData(UINT32 chanNum)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 j = 0, uiBlkSize = 0, i = 0, k = 0;

    FACE_DEV_PRM *pstFace_Dev = NULL;
    FACE_JDEC_BUF_INFO *pstJdecBufInfo = NULL;
    FACE_ANA_BUF_INFO *pstFaceAnaBufTab = NULL;
    DSPINITPARA *pstInitInfo = NULL;
    SYSTEM_FRAME_INFO *pstSysFrmInfo = NULL;
    FACE_COMMON_PARAM *pstFaceHalComm = NULL;

    ALLOC_VB_INFO_S stVbInfo = {0};

    /*入参判断*/
    if (chanNum > 1)
    {
        FACE_LOGE("Err,chan Err! chan(%d)\n", chanNum);
        return SAL_FAIL;
    }

    pstInitInfo = SystemPrm_getDspInitPara();

    for (i = 0; i < chanNum; i++)
    {
        pstFace_Dev = Face_DrvGetDev(i);
        FACE_DRV_CHECK_PRM(pstFace_Dev, SAL_FAIL);

        /* 人脸jpeg解码通道号，使用应用层下发的解码通道数的最后一个 */
        pstFace_Dev->uiJpegVdecChn = pstInitInfo->decChanCnt - 1;

		FACE_LOGI("JPEG VDEC CHN: %d, init dec chn %d \n", pstFace_Dev->uiJpegVdecChn, pstInitInfo->decChanCnt);

        /* 信号量初始化 */
        for (j = 0; j < FACE_MODE_MAX_NUM; j++)
        {
            pstFaceAnaBufTab = Face_HalGetAnaDataTab(j);
            if (NULL == pstFaceAnaBufTab)
            {
                FACE_LOGE("Get Face Ana Buf Table Failed! chan %d Mode %d\n", i, j);
                continue;
            }

            if (SAL_SOK != sem_init(&pstFaceAnaBufTab->sem, 0, 0))
            {
                FACE_LOGE("sem init failed! chan %d Mode %d\n", i, j);
                continue;
            }
        }

        /*人脸jpg软编YUV缓存数据初始化*/
        pstFace_Dev->stFaceYuvMem.bufLen = MAX_JPEG_WIDTH * MAX_JPEG_HEIGHT * 3 / 2;

        s32Ret = mem_hal_vbAlloc(pstFace_Dev->stFaceYuvMem.bufLen,
                                 "FACE", "face_jdec_tmp_buf", NULL,
                                 SAL_FALSE,
                                 &stVbInfo);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("Mmz Cache Creat Failed!\n");
            continue;
        }

        pstFace_Dev->stFaceYuvMem.u32PoolId = stVbInfo.u32PoolId;
        pstFace_Dev->stFaceYuvMem.u64VbBlk = stVbInfo.u64VbBlk;
        pstFace_Dev->stFaceYuvMem.uiPhyAddr = stVbInfo.u64PhysAddr;
        pstFace_Dev->stFaceYuvMem.pVirAddr = stVbInfo.pVirAddr;

        /*初始化图片解码缓存数据资源*/
        pstJdecBufInfo = Face_HalGetJdecBuf();
        if (NULL == pstJdecBufInfo)
        {
            FACE_LOGE("Get Jdec Buf Failed! chan %d\n", i);
            continue;
        }

        uiBlkSize = MAX_JPEG_WIDTH * MAX_JPEG_HEIGHT * 4;

        s32Ret = mem_hal_vbAlloc(uiBlkSize,
                                 "FACE", "face_jdec_buf", NULL,
                                 SAL_TRUE,
                                 &stVbInfo);
        if (SAL_SOK != s32Ret)
        {
            FACE_LOGE("Alloc Vb Buf For Jdec Failed! chan %d\n", i);
            continue;
        }

        pstJdecBufInfo->uiIdx = 0;
        pstJdecBufInfo->stBufData[0].uiUseFlag = SAL_FALSE;
        pstJdecBufInfo->stBufData[0].uiBufSize = uiBlkSize;
        pstJdecBufInfo->stBufData[0].PhyAddr = stVbInfo.u64PhysAddr;
        pstJdecBufInfo->stBufData[0].VirAddr = (UINT64)stVbInfo.pVirAddr;

        /*jpg编图内存申请*/
        for (k = 0; k < FACE_MAX_PIC_FRAME_NUM; k++)
        {
            pstFace_Dev->stJpegVenc.pPicJpeg[k] = (INT8 *)SAL_memZalloc(FACE_JPEG_SIZE, "FACE", "Face jpg");
            if (NULL == pstFace_Dev->stJpegVenc.pPicJpeg[k])
            {
                FACE_LOGE("error !!!\n");
                return SAL_FAIL;
            }
        }
        /* 初始化帧结构内存 */
        if (0x00 == pstFace_Dev->stRegisterFrameInfo.uiAppData)
        {
            s32Ret = sys_hal_allocVideoFrameInfoSt(&pstFace_Dev->stRegisterFrameInfo);
            if (s32Ret != SAL_SOK)
            {
                FACE_LOGE("Get Frame Mem error !!!\n");
                return SAL_FAIL;
            }
        }
    
        /* 初始化帧数据内存 */
        if (0x00 == pstFace_Dev->stLoginFrameInfo.uiAppData)
        {
            s32Ret = Face_DrvGetFrameMem(FACE_LOGIN_IMG_WIDTH, FACE_LOGIN_IMG_HEIGHT, &pstFace_Dev->stLoginFrameInfo);
            if (s32Ret != SAL_SOK)
            {
                FACE_LOGE("Get Frame Mem error !!!\n");
                return SAL_FAIL;
            }
        }
    
        if (0x00 == pstFace_Dev->stCapFrameInfo.uiAppData)
        {
            s32Ret = Face_DrvGetFrameMem(FACE_CAP_IMG_WIDTH, FACE_CAP_IMG_HEIGHT, &pstFace_Dev->stCapFrameInfo);
            if (s32Ret != SAL_SOK)
            {
                FACE_LOGE("Get Frame Mem error !!!\n");
                return SAL_FAIL;
            }
        }
    }

    /* 初始化解码帧数据临时变量 */
    pstFaceHalComm = Face_HalGetComPrm();
    pstSysFrmInfo = &pstFaceHalComm->stVdecCpyTmpFrameInfo;

    if (0x00 == pstSysFrmInfo->uiAppData)
    {
        /* 初始化锁资源，用于避免登录和抓拍业务竞态 */
        SAL_mutexCreate(SAL_MUTEX_NORMAL, &pstFaceHalComm->pVdecCpyFrmMutex);

        (VOID)sys_hal_allocVideoFrameInfoSt(pstSysFrmInfo);
    }

#ifdef DEBUG_FACE_LOG
    g_fp_res = fopen(RESULT_PATH_FD_CMP, "wt+");
    if (NULL == g_fp_res)
    {
        FACE_LOGE("file for result open failed\n");
        return SAL_FAIL;
    }

#endif

    /* 为本地数据库申请内存 */
    s32Ret = Face_DrvDataBaseInit();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("error\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    Face_DrvThrInit
 * @brief         人脸线程初始化接口
 * @param[in]  NULL
 * @param[out] NULL
 * @return SAL_SOK
 */
static INT32 Face_DrvThrInit(void)
{
    FACE_DEV_PRM *pstFace_Dev = NULL;

    /* 人脸只有一个通道 */
    pstFace_Dev = Face_DrvGetDev(0);
    if (NULL == pstFace_Dev)
    {
        FACE_LOGE("get dev 0 glb prm failed! \n");
        return SAL_FAIL;
    }

    /* 人脸登录基于选帧结果送入后处理通道，实现获取人脸建模数据 */
    SAL_thrCreate(&pstFace_Dev->stFaceLoginFeatThrHandl, Face_DrvVideoLoginFeatThread, SAL_THR_PRI_DEFAULT, 0, (void *)pstFace_Dev);

    /* 获取人脸抓拍解码数据线程 */
    SAL_thrCreate(&pstFace_Dev->stFaceGetFrameHandle, Face_DrvSendFrameDataProc, SAL_THR_PRI_DEFAULT, 0, (void *)pstFace_Dev);

    /* 人脸抓拍基于选帧结果送入后处理通道，实现获取人脸属性和人脸建模数据 */
    SAL_thrCreate(&pstFace_Dev->stFaceCapAttrFeatThrHandl, Face_DrvVideoCapAttrFeatThread, SAL_THR_PRI_DEFAULT, 0, (void *)pstFace_Dev);

    /* 人脸抓拍编图处理线程 */
    SAL_thrCreate(&pstFace_Dev->stFaceCapProcThrHandl, Face_DrvCapProThread, SAL_THR_PRI_DEFAULT, 0, (void *)pstFace_Dev);

    FACE_LOGI("Face ProThr init success!\n");

    return SAL_SOK;
}


/*******************************************************************************
* 函数名  : FACE_DrvEngineInitThread
* 描  述  : 启动人脸引擎相关资源初始化资源
* 输  入  : 无
* 输  出  : 无
* 返回值  : NULL
*******************************************************************************/
void *Face_DrvEngineInitThread(void *prm)
{
    INT32 s32Ret = SAL_FAIL;

    /* 设置线程名称*/
    SAL_SET_THR_NAME();

    /* 初始化算法资源 */
    s32Ret = Face_HalInit();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Face Module: Module Init Fail!\n");
        goto EXIT;
    }

    FACE_LOGI("Face Module: HAL Init OK!\n");

EXIT:
    SAL_thrExit(NULL);
    return NULL;

}

/**
 * @function    Face_DrvStartEngineInitThread
 * @brief      人脸通道引擎资源开始初始化
 * @param[in]  NULL
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_DrvStartEngineInitThread(VOID)
{
    SAL_ThrHndl Handle;

    if (SAL_SOK != SAL_thrCreate(&Handle, Face_DrvEngineInitThread, SAL_THR_PRI_DEFAULT, 2 * 1024 * 1024, NULL))
    {
        SVA_LOGE("Create Engine Init Thread Failed! \n");
        return SAL_FAIL;
    }

    if (SAL_SOK != SAL_thrDetach(&Handle))
    {
        SVA_LOGE("pthread detach failed! \n");
        return SAL_FAIL;
    }
    return SAL_SOK;
}

/**
 * @function    Face_DrvInit
 * @brief         人脸通道相关资源初始化
 * @param[in]  NULL
 * @param[out] NULL
 * @return SAL_SOK
 */
INT32 Face_DrvInit(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    s32Ret = Face_HalInitRtld();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Face Module: init runtime so Fail!\n");
        return SAL_FAIL;
    }

    s32Ret = Face_DrvStartEngineInitThread();
    if (SAL_SOK != s32Ret)
    {
       SVA_LOGE("Start Engine Init Thread Failed! \n");
       return SAL_FAIL;
    }

    /* 通道相关默认参数初始化 */
    s32Ret = Face_DrvInitDefaultInputParam(0);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("chan %d Init Default Param Failed!\n", 0);
        return SAL_FAIL;
    }

    /* 全局参数初始化 */
    s32Ret = Face_DrvInitGlobalData(1);
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Global Data Init Fail!\n");
        return SAL_FAIL;
    }

    /* 线程资源初始化 */
    s32Ret = Face_DrvThrInit();
    if (SAL_SOK != s32Ret)
    {
        FACE_LOGE("Init Thr Err!\n");
        return SAL_FAIL;
    }

    FACE_LOGI("Face Module: Drv Init OK!\n");
    return SAL_SOK;
}

