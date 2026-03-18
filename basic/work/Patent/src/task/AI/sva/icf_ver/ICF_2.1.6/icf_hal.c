/*******************************************************************************
* icf_hal.c
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : sunzelin <sunzelin@hikvision.com>
* Version: V1.0.0  2022年3月18日 Create
*
* Description :
* Modification:
*******************************************************************************/

/* icf V2.1.6 */
#include "ia_common.h"
#include "dspcommon.h"

#include "icf_hal.h"

#define ICF_VER ("V_2_1_6")    /* icf版本 */

static ICF_LIB_SYM_INFO g_stIcfLibInfo = {0};    /* 动态库符号解析信息 */
static ICF_INNER_INFO g_stIcfInnerInfo = {0};    /* icf内部信息，包括句柄和通道信息 */

/**
 * @function:   icf_pr_eng_chn_mem_info
 * @brief:      打印引擎运行使用的内存信息
 * @param[in]:  ICF_MODEL_HANDLE *pstModelHandleInfo
 * @param[out]: None
 * @return:     static INT32
 */
static VOID icf_pr_eng_chn_mem_info(ICF_CREATE_HANDLE *pstEngChnHandleInfo)
{
    UINT32 i;

    for (i = 0; i < ICF_MEM_TYPE_NUM; ++i)
    {
        if (pstEngChnHandleInfo->createMemSize.stNonSharedMemInfo[i].nMemSize <= 0)
        {
            continue;
        }

        SAL_LOGI("stalg_NonSharedMemInfo[%d].nMemSize = %d B %f MB type %d\n", i,
                 (INT32)pstEngChnHandleInfo->createMemSize.stNonSharedMemInfo[i].nMemSize,
                 (float)pstEngChnHandleInfo->createMemSize.stNonSharedMemInfo[i].nMemSize / 1024.0 / 1024.0,
                 pstEngChnHandleInfo->createMemSize.stNonSharedMemInfo[i].eMemType);
    }

    for (i = 0; i < ICF_MEM_TYPE_NUM; ++i)
    {
        if (pstEngChnHandleInfo->createMemSize.stSharedMemInfo[i].nMemSize <= 0)
        {
            continue;
        }

        SAL_LOGI("stalg_SharedMemInfo[%d].nMemSize = %d B %f MB type %d\n", i,
                 (INT32)pstEngChnHandleInfo->createMemSize.stSharedMemInfo[i].nMemSize,
                 (float)pstEngChnHandleInfo->createMemSize.stSharedMemInfo[i].nMemSize / 1024.0 / 1024.0,
                 pstEngChnHandleInfo->createMemSize.stSharedMemInfo[i].eMemType);
    }
}

/**
 * @function:   icf_pr_model_mem_info
 * @brief:      打印模型使用的内存信息
 * @param[in]:  ICF_MODEL_HANDLE *pstModelHandleInfo
 * @param[out]: None
 * @return:     static INT32
 */
static VOID icf_pr_model_mem_info(ICF_MODEL_HANDLE *pstModelHandleInfo)
{
    UINT32 i;

    for (i = 0; i < ICF_MEM_TYPE_NUM; ++i)
    {
        if (pstModelHandleInfo->modelMemSize.stNonSharedMemInfo[i].nMemSize <= 0)
        {
            continue;
        }

        SAL_LOGI("stmodel_NonSharedMemInfo[%d].nMemSize = %d B %f MB type %d\n", i,
                 (INT32)pstModelHandleInfo->modelMemSize.stNonSharedMemInfo[i].nMemSize,
                 (float)pstModelHandleInfo->modelMemSize.stNonSharedMemInfo[i].nMemSize / 1024.0 / 1024.0,
                 pstModelHandleInfo->modelMemSize.stNonSharedMemInfo[i].eMemType);
    }

    for (i = 0; i < ICF_MEM_TYPE_NUM; ++i)
    {
        if (pstModelHandleInfo->modelMemSize.stSharedMemInfo[i].nMemSize <= 0)
        {
            continue;
        }

        SAL_LOGI("stmodel_SharedMemInfo[%d].nMemSize = %d B %f MB type %d\n", i,
                 (INT32)pstModelHandleInfo->modelMemSize.stSharedMemInfo[i].nMemSize,
                 (float)pstModelHandleInfo->modelMemSize.stSharedMemInfo[i].nMemSize / 1024.0 / 1024.0,
                 pstModelHandleInfo->modelMemSize.stSharedMemInfo[i].eMemType);
    }
}

/**
 * @function:   icf_engine_get_free_inner_buf
 * @brief:      获取空闲的通道缓存
 * @param[in]:  VOID *pChnHandle
 * @param[out]: ICF_INNER_CHN_BUF **ppstInnerBuf
 * @return:     INT32
 */
static INT32 icf_engine_get_free_inner_buf(VOID *pChnHandle, ICF_INNER_CHN_BUF **ppstInnerBuf)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;

    ICF_INNER_CHN_INFO *pstInnerChnInfo = NULL;
    ICF_INNER_CHN_BUF *pstInnerChnBuf = NULL;

    ENGINE_HAL_CHECK_RET(NULL == pChnHandle, exit, "pChnHandle == NULL!");
    ENGINE_HAL_CHECK_RET(NULL == ppstInnerBuf, exit, "ppstInputData == NULL!");

    for (i = 0; i < g_stIcfInnerInfo.u32ChnNum; i++)
    {
        if (pChnHandle == g_stIcfInnerInfo.astInnerChnInfo[i].stCreateHandle.pChannelHandle)
        {
            break;
        }
    }

    if (i >= g_stIcfInnerInfo.u32ChnNum)
    {
        SAL_LOGE("inner chn handle not found! chn num %d, icf handle %p \n",
                 g_stIcfInnerInfo.u32ChnNum, g_stIcfInnerInfo.stIcfHandle.pInitHandle);
        goto exit;
    }

    pstInnerChnInfo = &g_stIcfInnerInfo.astInnerChnInfo[i];

    for (i = pstInnerChnInfo->uiR_idx; i < (pstInnerChnInfo->uiR_idx + ICF_MAX_INNER_CHN_BUF_NUM); i++)
    {
        pstInnerChnBuf = &pstInnerChnInfo->astIcfInputDataBuf[i % ICF_MAX_INNER_CHN_BUF_NUM];
        if (*(pstInnerChnBuf->stIcfInputDataBuf.pUseFlag[0]) == 1)
        {
            continue;
        }

        pstInnerChnBuf->bBusy = SAL_TRUE;
        break;
    }

    if (i >= (pstInnerChnInfo->uiR_idx + ICF_MAX_INNER_CHN_BUF_NUM))
    {
        SAL_LOGE("inner buf all busy! \n");
        goto exit;
    }

    /* 读索引+1 */
    pstInnerChnInfo->uiR_idx = (pstInnerChnInfo->uiR_idx + 1) % ICF_MAX_INNER_CHN_BUF_NUM;

    *ppstInnerBuf = pstInnerChnBuf;
    s32Ret = SAL_SOK;

exit:
    return s32Ret;
}

/**
 * @function:	icf_engine_rls_inner_chn_buf
 * @brief:		释放内部通道缓存
 * @param[in]:	ICF_INNER_CHN_BUF *pstInnerChnBuf
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 icf_engine_rls_inner_chn_buf(VOID *pInnerBuf)
{
    ICF_INNER_CHN_BUF *pstInnerChnBuf = NULL;

    /* 指针有效性外部调用方保证，此处无需校验 */
    pstInnerChnBuf = (ICF_INNER_CHN_BUF *)pInnerBuf;

    /* 释放内部的通道缓存 */
    pstInnerChnBuf->bBusy = SAL_FALSE;
    return SAL_SOK;
}

/**
 * @function:	icf_engine_post_proc_inner_chn_data
 * @brief:		内部通道资源后处理(当前仅有释放通道buf)
 * @param[in]:	ENGINE_COMM_ENGINE_PRVT_CTRL_PRM *pstEngPrvtCtrlPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 icf_engine_post_proc_inner_chn_data(ENGINE_COMM_ENGINE_PRVT_CTRL_PRM *pstEngPrvtCtrlPrm)
{
    INT32 s32Ret = SAL_FAIL;

    /* checker */
    ENGINE_COMM_CHECK_CTRL_PRM(pstEngPrvtCtrlPrm->pFuncProcEngPrvtData, \
                            pstEngPrvtCtrlPrm->pEngineInnerChnPrvtData, \
                            exit);

    s32Ret = pstEngPrvtCtrlPrm->pFuncProcEngPrvtData(pstEngPrvtCtrlPrm->pEngineInnerChnPrvtData);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "proc engine prvt data failed!");

exit:
    return s32Ret;
}

/**
 * @function:   icf_engine_deinit_InnerChn
 * @brief:      去初始化引擎业务通道的资源
 * @param[in]:  none
 * @param[out]: None
 * @return:     INT32
 */
static INT32 icf_engine_deinit_InnerChn(VOID)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i, j, k;
    ICF_INNER_CHN_INFO *pstInnerChnInfo = NULL;
    ICF_INNER_CHN_BUF *pstIcfInnerChnBuf = NULL;
    ICF_SOURCE_BLOB *pstIcfBlobInfo = NULL;

    for (i = 0; i < g_stIcfInnerInfo.u32ChnNum; i++)
    {
        pstInnerChnInfo = &g_stIcfInnerInfo.astInnerChnInfo[i];

        for (j = 0; j < ICF_MAX_INNER_CHN_BUF_NUM; j++)
        {
            pstIcfInnerChnBuf = &pstInnerChnInfo->astIcfInputDataBuf[j];

            /* 当前xsie仅使用blob_0, nBlobNum必须为1 */
            for (k = 0; k < pstIcfInnerChnBuf->stIcfInputDataBuf.nBlobNum; k++)
            {
                pstIcfBlobInfo = &pstIcfInnerChnBuf->stIcfInputDataBuf.stBlobData[k];

                if (NULL != pstIcfBlobInfo->pData)
                {
                    s32Ret |= SAL_memfree(pstIcfBlobInfo->pData, "icf", "st");
                    if (SAL_SOK != s32Ret)
                    {
                        SAL_LOGE("free blob buf failed! \n");
                    }
                }

                if (NULL != pstIcfInnerChnBuf->stIcfInputDataBuf.pUseFlag[k])
                {
                    s32Ret |= SAL_memfree(pstIcfInnerChnBuf->stIcfInputDataBuf.pUseFlag[k], "icf", "st");
                    if (SAL_SOK != s32Ret)
                    {
                        SAL_LOGE("free blob useFlag failed! \n");
                    }
                }
            }
        }

        memset(pstInnerChnInfo, 0x00, sizeof(ICF_INNER_CHN_INFO));
    }

    return s32Ret;
}

/**
 * @function:   icf_engine_init_InnerChn
 * @brief:      初始化引擎业务通道的资源
 * @param[in]:  const ENGINE_COMM_INIT_PRM_S *pstInitPrm
 * @param[out]: None
 * @return:     INT32
 */
static INT32 icf_engine_init_InnerChn(const ENGINE_COMM_INIT_PRM_S *pstInitPrm)
{
    INT32 s32Ret = SAL_SOK;

    UINT32 i, j, k;
    ICF_INNER_CHN_INFO *pstInnerChnInfo = NULL;
    ICF_INNER_CHN_BUF *pstIcfInnerChnBuf = NULL;
    ICF_SOURCE_BLOB *pstIcfBlobInfo = NULL;

    if (0 == pstInitPrm->u32IaPrvtDataSize)
    {
        SAL_LOGE("invalid prvt data size %d \n", pstInitPrm->u32IaPrvtDataSize);
        return SAL_FAIL;
    }

    for (i = 0; i < g_stIcfInnerInfo.u32ChnNum; i++)
    {
        pstInnerChnInfo = &g_stIcfInnerInfo.astInnerChnInfo[i];

        for (j = 0; j < ICF_MAX_INNER_CHN_BUF_NUM; j++)
        {
            /* fixme: xsie目前仅使用blob_0，故此处没有对blob进行遍历 */
            pstIcfInnerChnBuf = &pstInnerChnInfo->astIcfInputDataBuf[j];
            pstIcfInnerChnBuf->stIcfInputDataBuf.nBlobNum = pstInnerChnInfo->u32BlobNum;

            for (k = 0; k < pstIcfInnerChnBuf->stIcfInputDataBuf.nBlobNum; k++)
            {
                pstIcfBlobInfo = &pstIcfInnerChnBuf->stIcfInputDataBuf.stBlobData[k];

                if (NULL == pstIcfBlobInfo->pData)
                {
                    pstIcfBlobInfo->pData = SAL_memZalloc(pstInitPrm->u32IaPrvtDataSize, "icf", "st");
                    if (NULL == pstIcfBlobInfo->pData)
                    {
                        SAL_LOGE("zalloc icf blob failed! \n");
                        s32Ret = SAL_FAIL;
                    }
                }

                if (NULL == pstIcfInnerChnBuf->stIcfInputDataBuf.pUseFlag[k])
                {
                    pstIcfInnerChnBuf->stIcfInputDataBuf.pUseFlag[k] = SAL_memZalloc(sizeof(INT32), "icf", "st");
                    if (NULL == pstIcfInnerChnBuf->stIcfInputDataBuf.pUseFlag[k])
                    {
                        SAL_LOGE("zalloc icf blob useFlag failed! k %d \n", k);
                        s32Ret = SAL_FAIL;
                    }
                }
            }

            pstIcfInnerChnBuf->bBusy = SAL_FALSE;   /* 空闲 */
        }
    }

    return s32Ret;
}

/**
 * @function:   icf_engine_destroy
 * @brief:      销毁引擎通道
 * @param[in]:  None
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 icf_engine_destroy(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;
    ICF_INNER_CHN_INFO *pstInnerChnInfo = NULL;

    for (i = 0; i < g_stIcfInnerInfo.u32ChnNum; i++)
    {
        pstInnerChnInfo = &g_stIcfInnerInfo.astInnerChnInfo[i];

        if (NULL == pstInnerChnInfo->stCreateHandle.pChannelHandle)
        {
            SVA_LOGW("Engine Handle %d is Destroyed! chan_num %d \n", i, g_stIcfInnerInfo.u32ChnNum);
            continue;
        }

        s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfDestroy(&pstInnerChnInfo->stCreateHandle, sizeof(ICF_CREATE_HANDLE));
        if (SAL_SOK != s32Ret)
        {
            SVA_LOGE("Icf Destroy Handle Failed! idx %d, ret: 0x%x \n", i, s32Ret);
        }

        pstInnerChnInfo->stCreateHandle.pChannelHandle = NULL;
    }

    return s32Ret;
}

/**
 * @function:   icf_engine_create
 * @brief:      创建引擎通道
 * @param[in]:  const ENGINE_COMM_INIT_PRM_S *pstInitPrm
 * @param[out]: ENGINE_COMM_INIT_OUTPUT_PRM *pstInitOutput
 * @return:     static INT32
 */
static INT32 icf_engine_create(const ENGINE_COMM_INIT_PRM_S *pstInitPrm, ENGINE_COMM_INIT_OUTPUT_PRM *pstInitOutput)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 i = 0;
    UINT32 u32UserData[10] = {0};

    ICF_CREATE_PARAM stCreateParam = {0};
    ICF_CALLBACK_PARAM stCallbackParam = { 0 };
    ICF_INNER_CHN_INFO *pstIcfInnerChnInfo = NULL;

    time0 = SAL_getCurMs();

    if (pstInitPrm->stInitChnCfg.u32ChnNum > ENGINE_COMM_MAX_USER_CHN_NUM)
    {
        SAL_LOGE("invalid init chn num %d \n", pstInitPrm->stInitChnCfg.u32ChnNum);
        goto exit;
    }

    g_stIcfInnerInfo.u32ChnNum = pstInitPrm->stInitChnCfg.u32ChnNum;

    for (i = 0; i < g_stIcfInnerInfo.u32ChnNum; i++)
    {
        pstIcfInnerChnInfo = &g_stIcfInnerInfo.astInnerChnInfo[i];

        if (NULL == pstIcfInnerChnInfo->stCreateHandle.pChannelHandle)
        {
            memcpy(&stCreateParam.modelParam, &g_stIcfInnerInfo.stModelParam, sizeof(ICF_MODEL_PARAM));
            memcpy(&stCreateParam.modelHandle, &g_stIcfInnerInfo.stModelHandle, sizeof(ICF_MODEL_HANDLE));

            /* 创建引擎通道 */
            s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfCreate(&stCreateParam,
                                                          sizeof(ICF_CREATE_PARAM),
                                                          &pstIcfInnerChnInfo->stCreateHandle,
                                                          sizeof(ICF_CREATE_HANDLE));
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("ICF_Create channel %d error, ret: 0x%x\n", i, s32Ret);
                return SAL_FAIL;
            }

            /* 打印通道内存消耗信息 */
            icf_pr_eng_chn_mem_info(&pstIcfInnerChnInfo->stCreateHandle);

            stCallbackParam.nNodeID	= pstInitPrm->stInitChnCfg.astChnAttr[i].u32NodeId;
            stCallbackParam.nCallbackType = pstInitPrm->stInitChnCfg.astChnAttr[i].u32CallBackType;
            stCallbackParam.pCallbackFunc = (void *)pstInitPrm->pRsltCbFunc;
            stCallbackParam.nUserSize = 10;
            stCallbackParam.pUsr = u32UserData;               /* 未使用 */

            /* 注册通道的结果回调函数 */
            s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfSetCallback(pstIcfInnerChnInfo->stCreateHandle.pChannelHandle,
                                                               &stCallbackParam,
                                                               sizeof(ICF_CALLBACK_PARAM));
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("icf set callback %d failed! ret: 0x%x\n", i, s32Ret);
                return SAL_FAIL;
            }

            pstIcfInnerChnInfo->u32HdlIdx = i;   /* 记录引擎通道索引值 */
            pstIcfInnerChnInfo->u32BlobNum = 1;  /* fixme: xsie私有信息，不具备通用性 */
        }
        else
        {
            SVA_LOGW("Engine Handle is Initialized! i %d \n", i);
        }

        pstInitOutput->stInitChnOutput.pChnHandle[i] = pstIcfInnerChnInfo->stCreateHandle.pChannelHandle;
    }

    /* 存放通道创建输出结果 */
    pstInitOutput->stInitChnOutput.u32ChnNum = pstInitPrm->stInitChnCfg.u32ChnNum;

    time1 = SAL_getCurMs();

    SVA_LOGI("icf register End! register %d \n", time1 - time0);
exit:
    return s32Ret;
}

/**
 * @function:   icf_engine_unload_model
 * @brief:      引擎框架卸载模型
 * @param[in]:  None
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 icf_engine_unload_model(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfUnloadModel(&g_stIcfInnerInfo.stModelHandle, sizeof(ICF_MODEL_HANDLE));
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Icf unload model Failed! ret: 0x%x. \n", s32Ret);
        goto exit;
    }

exit:
    return s32Ret;
}

/**
 * @function:   icf_engine_load_model
 * @brief:      引擎框架加载模型
 * @param[in]:  None
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 icf_engine_load_model(const ENGINE_COMM_INIT_PRM_S *pstInitPrm)
{
    INT32 s32Ret = SAL_FAIL;

    /* checker */
    ENGINE_HAL_CHECK_RET(NULL == pstInitPrm, exit, "pstInitPrm == null!");

    g_stIcfInnerInfo.stAppParam[0].pAppParamCfgPath = pstInitPrm->stInitChnCfg.astChnAttr[0].pAppParamCfgPath;

    g_stIcfInnerInfo.stModelParam.nGraphID = 1;  /* szl_todo: 入参传入 */
    g_stIcfInnerInfo.stModelParam.nGraphType = pstInitPrm->stInitChnCfg.astChnAttr[0].u32GraphType; /*1*/       /* fixme: graphtype 1表示视频模式，2表示仅分包 */
    g_stIcfInnerInfo.stModelParam.pAppParam = &g_stIcfInnerInfo.stAppParam[0];
    g_stIcfInnerInfo.stModelParam.nMaxCacheNums = 0;
    g_stIcfInnerInfo.stModelParam.pInitHandle = g_stIcfInnerInfo.stIcfHandle.pInitHandle;

    s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfLoadModel(&g_stIcfInnerInfo.stModelParam,
                                                     sizeof(ICF_MODEL_PARAM),
                                                     &g_stIcfInnerInfo.stModelHandle,
                                                     sizeof(ICF_MODEL_HANDLE));
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("icf load model Failed! ret: 0x%x. \n", s32Ret);

		icf_pr_model_mem_info(&g_stIcfInnerInfo.stModelHandle);
        goto exit;
    }

    icf_pr_model_mem_info(&g_stIcfInnerInfo.stModelHandle);

exit:
    return s32Ret;
}

/**
 * @function:   icf_engine_deinit
 * @brief:      引擎框架去初始化
 * @param[in]:  None
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 icf_engine_deinit(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfFinit(&g_stIcfInnerInfo.stIcfHandle, sizeof(ICF_INIT_HANDLE));
    if (SAL_SOK != s32Ret)
    {
        SVA_LOGE("Icf DeInit Failed! ret: 0x%x. \n", s32Ret);
        goto exit;
    }

exit:
    return s32Ret;
}

/**
 * @function   icf_engine_do_mem_map
 * @brief      对icf框架输入的内存类型映射到业务层可识别的内存类型
 * @param[in]  ICF_MEM_TYPE_E enIcfMemType  
 * @param[in]  IA_MEM_TYPE_E *penIaMemType  
 * @param[out] None
 * @return     static INT32
 */
static INT32 icf_engine_do_mem_map(ICF_MEM_TYPE_E enIcfMemType, IA_MEM_TYPE_E *penIaMemType)
{
	switch(enIcfMemType)
	{
		case ICF_MEM_MALLOC:
		{
			*penIaMemType = IA_MALLOC;
			break;
		}
		case ICF_HISI_MEM_MMZ_WITH_CACHE:
		{
			*penIaMemType = IA_HISI_MMZ_CACHE;
			break;
		}
		case ICF_HISI_MEM_MMZ_NO_CACHE:
		{
			*penIaMemType = IA_HISI_MMZ_NO_CACHE;
			break;
		}
		case ICF_HISI_MEM_MMZ_WITH_CACHE_PRIORITY:
		{
			*penIaMemType = IA_HISI_MMZ_CACHE_PRIORITY;
			break;
		}
		case ICF_HISI_MEM_MMZ_NO_CACHE_PRIORITY:
		{
			*penIaMemType = IA_HISI_MMZ_CACHE_NO_PRIORITY;
			break;
		}
		case ICF_HISI_MEM_VB_REMAP_NONE:
		{
			*penIaMemType = IA_HISI_VB_REMAP_NONE;
			break;
		}
		case ICF_HISI_MEM_VB_REMAP_NOCACHE:
		{
			*penIaMemType = IA_HISI_VB_REMAP_NO_CACHED;
			break;
		}		
		case ICF_HISI_MEM_VB_REMAP_CACHED:
		{
			*penIaMemType = IA_HISI_VB_REMAP_CACHED;
			break;
		}	
		case ICF_NT_MEM_FEATURE:
		{
			*penIaMemType = IA_NT_MEM_FEATURE;
			break;
		}
#if 0
		case ICF_RN_MEM_MMZ_CMA_WITH_CACHE:
		{
			*penIaMemType = IA_RK_CACHE_CMA;
			break;
		}
#endif
		default:
		{
			SAL_LOGE("invalid mem type %d \n", enIcfMemType);
			return SAL_FAIL;
		}
	}

	return SAL_SOK;
}

/**
 * @function   icf_engine_free_mem
 * @brief      engine注册给icf框架的内存释放接口
 * @param[in]  VOID *pInitHandle             
 * @param[in]  ICF_MEM_INFO *pstMemInfo      
 * @param[in]  ICF_MEM_BUFFER *pstMemBuffer  
 * @param[out] None
 * @return     static INT32
 */
static INT32 icf_engine_free_mem(VOID *pInitHandle,
                                 ICF_MEM_INFO *pstMemInfo,
                                 ICF_MEM_BUFFER *pstMemBuffer)
{
	INT32 s32Ret = SAL_FAIL;

	/* 当前智能模块的内存在系统初始化时统一申请，不进行动态释放。故此处直接返回成功 */
	IA_MEM_TYPE_E enMemType = IA_MALLOC;/* TODO: 后续取消使用ia_common.c中的数据结构 */
	IA_MEM_PRM_S stMemBuf = {0};

	s32Ret = icf_engine_do_mem_map(pstMemInfo->eMemType, &enMemType);
	ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "mem map failed!");	

	stMemBuf.VirAddr = (char *)pstMemBuffer->pVirMemory;
	stMemBuf.PhyAddr = (PhysAddr)pstMemBuffer->pPhyMemory;

	if (g_stIcfInnerInfo.pMemFreeFunc)
	{
		s32Ret = g_stIcfInnerInfo.pMemFreeFunc(enMemType, pstMemInfo->nMemSize, &stMemBuf);
		ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "mem free func failed!");
		
		SAL_LOGI("free mem enter! \n");
	}
	
	SAL_LOGI("free mem end! \n");
exit:
	return s32Ret;
}

/**
 * @function   icf_engine_alloc_mem
 * @brief      engine层注册给icf框架的内存申请接口
 * @param[in]  VOID *pInitHandle               
 * @param[in]  const ICF_MEM_INFO *pstMemInfo  
 * @param[in]  ICF_MEM_BUFFER *pstMemBuffer    
 * @param[out] None
 * @return     static INT32
 */
static INT32 icf_engine_alloc_mem(VOID *pInitHandle,
                                  const ICF_MEM_INFO *pstMemInfo,
                                  ICF_MEM_BUFFER *pstMemBuffer)
{
	INT32 s32Ret = SAL_FAIL;

	IA_MEM_TYPE_E enMemType = IA_MALLOC;/* TODO: 后续取消使用ia_common.c中的数据结构 */
	IA_MEM_PRM_S stMemBuf = {0};

	s32Ret = icf_engine_do_mem_map(pstMemInfo->eMemType, &enMemType);
	ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "mem map failed!");	

	if (g_stIcfInnerInfo.pMemAllocFunc)
	{
		s32Ret = g_stIcfInnerInfo.pMemAllocFunc(enMemType, pstMemInfo->nMemSize, &stMemBuf);
		ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "mem alloc func failed!");

		SAL_LOGI("alloc mem enter! \n");
	}

	pstMemBuffer->pVirMemory = (VOID *)stMemBuf.VirAddr;
	pstMemBuffer->pPhyMemory = (VOID *)stMemBuf.PhyAddr;

	SAL_LOGI("alloc mem end! \n");
exit:
	return s32Ret;
}

/**
 * @function:   icf_engine_init
 * @brief:      引擎框架初始化
 * @param[in]:  ENGINE_COMM_INIT_PRM_S *pstInitPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 icf_engine_init(const ENGINE_COMM_INIT_PRM_S *pstInitPrm)
{
    INT32 s32Ret = SAL_FAIL;

    ICF_INIT_PARAM stInitParam = {0};

    /* checker */
    ENGINE_HAL_CHECK_RET(NULL == pstInitPrm, exit, "pstInitPrm == null!");

    stInitParam.stConfigInfo.pAlgCfgPath = (char *)pstInitPrm->stCfgFileInfo.pAlgCfgPath;               /* 每种模式，会对应一种配置文件，引擎已经配置好，可直接使用 */
    stInitParam.stConfigInfo.pTaskCfgPath = (char *)pstInitPrm->stCfgFileInfo.pTaskCfgPath;
    stInitParam.stConfigInfo.pToolkitCfgPath = (char *)pstInitPrm->stCfgFileInfo.pToolkitCfgPath;

    stInitParam.stConfigInfo.bAlgCfgEncry = 0;
    stInitParam.stConfigInfo.bTaskCfgEncry = 0;
    stInitParam.stConfigInfo.bToolkitCfgEncry = 0;
    //stInitParam.stMemConfig.reserved[0] = 1;                 /* 内存越界检测，当前版本不支持该功能 */

	/* 保存智能业务模块传入的内存申请/释放接口函数指针 */
	g_stIcfInnerInfo.pMemAllocFunc = pstInitPrm->stInitMemCfg.pMemAllocFunc;
	g_stIcfInnerInfo.pMemFreeFunc = pstInitPrm->stInitMemCfg.pMemRlsFunc;

    /* 支持用户在外部申请内存管理函数 */
    stInitParam.stMemConfig.pMemSytemMallocInter = (void *)icf_engine_alloc_mem;
    stInitParam.stMemConfig.pMemSytemFreeInter = (void *)icf_engine_free_mem;
    stInitParam.pScheduler = IA_GetScheHndl();

    s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfInit(&stInitParam,
                                                sizeof(ICF_INIT_PARAM),
                                                &g_stIcfInnerInfo.stIcfHandle,
                                                sizeof(ICF_INIT_HANDLE));
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, finit, "icf init failed!");

	goto exit;
	
finit:
    (VOID)g_stIcfLibInfo.stIcfLibApi.IcfFinit(&g_stIcfInnerInfo.stIcfHandle, sizeof(ICF_INIT_HANDLE));

exit:
    return s32Ret;
}

/**
 * @function:   icf_rslv_icf_lib_sym
 * @brief:      加载引擎应用框架层动态库符号(libicf.so)
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 icf_rslv_icf_lib_sym(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    s32Ret = Sal_GetLibHandle("libicf.so", &g_stIcfLibInfo.pIcfLibHandle);

    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibHandle failed!");

    /* ICF_Interface.h */
    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_Init", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfInit);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_Init failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_Finit", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfFinit);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_Finit failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_LoadModel", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfLoadModel);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_Init failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_UnloadModel", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfUnloadModel);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_Finit failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_Create", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfCreate);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_Create failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_Destroy", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfDestroy);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_Destroy failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_SetConfig", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfSetConfig);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_SetConfig failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_GetConfig", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfGetConfig);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_GetConfig failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_SetCallback", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfSetCallback);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_Set_callback failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_GetVersion", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfGetVersion);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_GetVersion failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_InputData", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfInputData);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_InputData failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_Package_GetState", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfGetPackageStatus);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_Package_GetState failed!");

    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pIcfLibHandle, "ICF_GetDataPtrFromPkg", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfGetPackageDataPtr);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_GetDataPtrFromPkg failed!");

exit:
    return s32Ret;
}

/**
 * @function:   icf_rslv_xsie_lib_sym
 * @brief:      加载引擎应用框架层动态库符号(libXsieAlg.so)
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 icf_rslv_xsie_lib_sym(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    s32Ret = Sal_GetLibHandle("libXsieAlg.so", &g_stIcfLibInfo.pXsieLibHandle);

    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibHandle failed!");

    /* ICF_Interface.h */
    s32Ret = Sal_GetLibSymbol(g_stIcfLibInfo.pXsieLibHandle, "ICF_APP_GetVersion", (VOID **)&g_stIcfLibInfo.stIcfLibApi.IcfAppGetVersion);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Sal_GetLibSymbol ICF_APP_GetVersion failed!");

exit:
    return s32Ret;
}

/**
 * @function:   icf_rslv_lib_sym
 * @brief:      加载引擎应用框架层动态库符号
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 icf_rslv_lib_sym(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    /* szl_todo: 通过外部传参解耦 */
    s32Ret = icf_rslv_icf_lib_sym();
    s32Ret |= icf_rslv_xsie_lib_sym();

    SAL_LOGI("szl_dbg: icf lib ver: %s \n", ICF_VER);  /* 打印版本号 */

    return s32Ret;
}

/**
 * @function:   icf_user_img_type_to_icf_type
 * @brief:      用户层图像类型转换为icf类型
 * @param[in]:  ENGINE_COMM_INPUT_DATA_IMG_TYPE enImgType
 * @param[out]: ICF_BLOB_FORMAT_E *penIcfImgType
 * @return:     static INT32
 */
static INT32 icf_user_img_type_to_icf_type(ENGINE_COMM_INPUT_DATA_IMG_TYPE enImgType, ICF_BLOB_FORMAT_E *penIcfImgType)
{
    INT32 s32Ret = SAL_FAIL;

    ICF_BLOB_FORMAT_E enIcfDefImgType = ICF_INPUT_FORMAT_YUV_NV21;

    ENGINE_HAL_CHECK_RET(NULL == penIcfImgType, exit, "penIcfImgType == null!");

    switch (enImgType)
    {
        case YUV_NV21:
        {
            enIcfDefImgType = ICF_INPUT_FORMAT_YUV_NV21;
            break;
        }
        default:
        {
            SAL_LOGW("invalid user img type %d \n", enImgType);
            break;
        }
    }

	s32Ret = SAL_SOK;
    *penIcfImgType = enIcfDefImgType;

exit:
    return s32Ret;
}

/**
 * @function:   icf_user_mem_type_to_icf_type
 * @brief:      用户层内存类型转换为icf类型
 * @param[in]:  ENGINE_COMM_INPUT_DATA_MEM_TYPE enMemType
 * @param[out]: ICF_MEM_TYPE_E *penIcfMemType
 * @return:     static INT32
 */
static INT32 icf_user_mem_type_to_icf_type(ENGINE_COMM_INPUT_DATA_MEM_TYPE enMemType, ICF_MEM_TYPE_E *penIcfMemType)
{
    INT32 s32Ret = SAL_FAIL;

    ICF_MEM_TYPE_E enIcfDefMemType = ICF_HISI_MEM_MMZ_WITH_CACHE;

    ENGINE_HAL_CHECK_RET(NULL == penIcfMemType, exit, "penIcfMemType == null!");

    switch (enMemType)
    {
        case HISI_MMZ_CACHE:
        {
            enIcfDefMemType = ICF_HISI_MEM_MMZ_WITH_CACHE;
            break;
        }
        case NT_FEATURE:
        {
            enIcfDefMemType = ICF_NT_MEM_FEATURE;
            break;
        }
        case RK_MALLOC:
        {
            enIcfDefMemType = ICF_MEM_MALLOC;
            break;
        }

        default:
        {
            SAL_LOGW("invalid user mem type %d \n", enMemType);
            break;
        }
    }

	s32Ret = SAL_SOK;
    *penIcfMemType = enIcfDefMemType;

exit:
    return s32Ret;
}

/**
 * @function:	icf_get_user_info
 * @brief:		获取用户透传信息
 * @param[in]:	VOID *pstRawOutput
 * @param[out]: ENGINE_COMM_OUTPUT_DATA_S *pstOutputRslt
 * @param[out]: ENGINE_COMM_ENGINE_PRVT_CTRL_PRM *pstEngPrvtCtrlPrm
 * @return:     static INT32
 */
static INT32 icf_get_user_info(VOID *pstRawOutput,
                               ENGINE_COMM_OUTPUT_DATA_S *pstOutputRslt,
                               ENGINE_COMM_ENGINE_PRVT_CTRL_PRM **ppstEngPrvtCtrlPrm)
{
    INT32 s32Ret = SAL_FAIL;

    ICF_MEDIA_INFO *pstInputYuvData = NULL;
    ENGINE_COMM_INPUT_DATA_USER_INFO *pstUsrPrvtInfo = NULL;
    ENGINE_COMM_USER_PRVT_CTRL_PRM *pstUsrPrvtCtrlPrm = NULL;
    ENGINE_COMM_ENGINE_PRVT_CTRL_PRM *pstEnginePrvtCtrlPrm = NULL;

    pstInputYuvData = (ICF_MEDIA_INFO *)g_stIcfLibInfo.stIcfLibApi.IcfGetPackageDataPtr(pstRawOutput, ICF_ANA_MEDIA_DATA);
    ENGINE_HAL_CHECK_RET(NULL == pstInputYuvData, exit, "pstInputYuvData == null!");

    /* 校验引擎通用用户结构体信息是否有效 */
    if (NULL == pstInputYuvData->stInputInfo.pUserPtr
        || pstInputYuvData->stInputInfo.nUserDataSize != sizeof(ENGINE_COMM_INPUT_DATA_USER_INFO))
    {
        s32Ret = SAL_FAIL;

        SAL_LOGE("icf comm user info failed! \n");
        goto exit;
    }

    /* 若用户私有参数有效，则进行输出 */
    pstUsrPrvtInfo = (ENGINE_COMM_INPUT_DATA_USER_INFO *)pstInputYuvData->stInputInfo.pUserPtr;
    pstUsrPrvtCtrlPrm = &pstUsrPrvtInfo->stUsrPrvtCtrlPrm;     /* 用户透传数据 */
    pstEnginePrvtCtrlPrm = &pstUsrPrvtInfo->stEnginePrvtInfo;  /* engine通用透传数据，目前仅有内部通道缓存 */

    /* 若外部模块传入用户私有信息，则需要调用控制参数中的回调函数处理，or忽略 */
    if (pstUsrPrvtCtrlPrm->bUsed)
    {
        ENGINE_COMM_GET_NODE_CTRL_PRM_CHECK(ICF_ANA_MEDIA_DATA, \
                                         pstOutputRslt->stOutputCtlPrm.pFuncGetUserInfo, \
                                         pstOutputRslt->stOutputCtlPrm.pUserInfo, \
                                         exit);

        /* 使用输入用户透传参数绑定的回调处理函数进行拷贝到外部 */
        s32Ret = pstOutputRslt->stOutputCtlPrm.pFuncGetUserInfo(pstUsrPrvtInfo->stUsrPrvtCtrlPrm.pUsrData,
                                                                pstOutputRslt->stOutputCtlPrm.pUserInfo);
        ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "get usr data fail!");
    }

    /* 注意，此处engine控制参数为全局参数 */
    *ppstEngPrvtCtrlPrm = pstEnginePrvtCtrlPrm;

    s32Ret = SAL_SOK;

exit:
    return s32Ret;
}

/**
 * @function   icf_do_node_map
 * @brief      实现智能模块的节点向icf支持的算法节点映射
 * @param[in]  ENGINE_COMM_NODE_TYPE_E enEngineNodeType  
 * @param[in]  UINT32 *pu32IcfNodeId                     
 * @param[out] None
 * @return     static INT32
 */
static INT32 icf_do_node_map(ENGINE_COMM_NODE_TYPE_E enEngineNodeType, UINT32 *pu32IcfNodeId)
{
	switch(enEngineNodeType)
	{
		case ENGINE_COMM_XSI_NODE:
		{
			*pu32IcfNodeId = ICF_ANA_XSI_DATA;
			break;
		}
		default:
		{
			SAL_LOGE("invalid node type %d \n", enEngineNodeType);
			return SAL_FAIL;
		}
	}
	return SAL_SOK;
}

/**
 * @function:	icf_get_node_out
 * @brief:		获取节点输出结果
 * @param[in]:	VOID *pstRawOutput
 * @param[out]: ENGINE_COMM_OUTPUT_DATA_S *pstOutputData
 * @return:     static INT32
 */
static INT32 icf_get_node_out(VOID *pstRawOutput, ENGINE_COMM_OUTPUT_DATA_S *pstOutputData)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 u32NodeId = 0;
    VOID *pTmp = NULL;

	s32Ret = icf_do_node_map(pstOutputData->stOutputCtlPrm.enNodeId, &u32NodeId);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "node map failed!");

    pTmp = (VOID *)g_stIcfLibInfo.stIcfLibApi.IcfGetPackageDataPtr(pstRawOutput, u32NodeId);
    ENGINE_HAL_CHECK_RET(NULL == pTmp, exit, "get node rlst failed!");

    /* checker */
    ENGINE_COMM_GET_NODE_CTRL_PRM_CHECK(u32NodeId, \
                                     pstOutputData->stOutputCtlPrm.pFuncGetNodeRslt, \
                                     pstOutputData->stOutputCtlPrm.pAlgRsltInfo, \
                                     exit);

    /* 拷贝节点信息到外部模块 */
    s32Ret = pstOutputData->stOutputCtlPrm.pFuncGetNodeRslt(pTmp, pstOutputData->stOutputCtlPrm.pAlgRsltInfo);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "pFuncGetNodeRslt failed!");

exit:
    return s32Ret;
}

/**
 * @function:	icf_get_pkg_status
 * @brief:		获取xsi输出结果
 * @param[in]:	VOID *pstRawOutput
 * @return:     static INT32
 */
static INT32 icf_get_pkg_status(VOID *pstRawOutput)
{
    INT32 s32Ret = SAL_FAIL;

    s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfGetPackageStatus(pstRawOutput);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "icf get pkg status failed!");

exit:
    return s32Ret;
}

#if 1 /* COMMON_API */

/**
 * @function:	engine_comm_get_alg_result
 * @brief:		获取算法结果
 * @param[in]:	INT32 nNodeID
 * @param[in]:	INT32 nCallBackType
 * @param[in]:	void *pstOutput
 * @param[in]:	UINT32 nSize
 * @param[in]:	void *pUsr
 * @param[in]:	INT32 nUserSize
 * @param[out]: VOID *pData
 * @return:     static INT32
 */
static INT32 engine_comm_get_alg_result(VOID *pstOutput, ENGINE_COMM_OUTPUT_DATA_S *pstOutputRslt)
{
    INT32 s32Ret = SAL_FAIL;

    ENGINE_COMM_ENGINE_PRVT_CTRL_PRM *pstEngPrvtCtrlPrm = NULL;

    /* checker */
    ENGINE_HAL_CHECK_RET(NULL == pstOutput, exit, "pstOutput == null!");
    ENGINE_HAL_CHECK_RET(NULL == pstOutputRslt, exit, "pstOutputRslt == null!");
	
    /* 获取数据包状态 */
    s32Ret = icf_get_pkg_status(pstOutput);
    ENGINE_HAL_CHECK_RET(s32Ret, exit, "icf_get_user_info failed!");

    /* 获取用户透传信息 */
    s32Ret = icf_get_user_info(pstOutput, pstOutputRslt, &pstEngPrvtCtrlPrm);
    ENGINE_HAL_CHECK_RET(s32Ret, rls, "icf_get_user_info failed!");

    /* 获取xsie输出信息 */
    s32Ret = icf_get_node_out(pstOutput, pstOutputRslt);
    ENGINE_HAL_CHECK_RET(s32Ret, rls, "icf_get_xsi_out_info failed!");
	
rls:
	if (pstEngPrvtCtrlPrm)
	{		
    	(VOID)icf_engine_post_proc_inner_chn_data(pstEngPrvtCtrlPrm);
	}
exit:
    return s32Ret;
}

/**
 * @function:   engine_comm_get_version
 * @brief:      获取引擎框架icf版本号
 * @param[in]:  ENGINE_COMM_VERSION_S *pstIcfVer
 * @param[in]:  UINT32 u32Size
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 engine_comm_get_version(ENGINE_COMM_VERSION_S *pstIcfVer, UINT32 u32Size)
{
    INT32 s32Ret = SAL_FAIL;

	ICF_VERSION stIcfVersion = {0};

    ENGINE_HAL_CHECK_RET(NULL == pstIcfVer, exit, "pstIcfVer == null!");
    ENGINE_HAL_CHECK_RET(u32Size != sizeof(ENGINE_COMM_VERSION_S), exit, "size error!");

    s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfGetVersion(&stIcfVersion, sizeof(ICF_VERSION));
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "get icf version failed!");

	pstIcfVer->u32MajorVersion = stIcfVersion.nMajorVersion;
	pstIcfVer->u32SubVersion = stIcfVersion.nSubVersion;
	pstIcfVer->u32RevisVersion = stIcfVersion.nRevisVersion;
	pstIcfVer->u32VersionYear = stIcfVersion.nVersionYear;
	pstIcfVer->u32VersionMonth = stIcfVersion.nVersionMonth;
	pstIcfVer->u32VersionDay = stIcfVersion.nVersionDay;

exit:
    return s32Ret;
}

/**
 * @function:   engine_comm_get_app_version
 * @brief:      获取引擎应用算法版本号
 * @param[in]:  ENGINE_COMM_VERSION_S *pstIcfAppVer
 * @param[in]:  UINT32 u32Size
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 engine_comm_get_app_version(ENGINE_COMM_VERSION_S *pstIcfAppVer, UINT32 u32Size)
{
    INT32 s32Ret = SAL_FAIL;

	ICF_VERSION stIcfAppVersion = {0};

    ENGINE_HAL_CHECK_RET(NULL == pstIcfAppVer, exit, "pstIcfAppVer == null!");
    ENGINE_HAL_CHECK_RET(u32Size != sizeof(ENGINE_COMM_VERSION_S), exit, "size error!");

    s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfAppGetVersion(&stIcfAppVersion, sizeof(ICF_VERSION));
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "get icf app version failed!");

	pstIcfAppVer->u32MajorVersion = stIcfAppVersion.nMajorVersion;
	pstIcfAppVer->u32SubVersion = stIcfAppVersion.nSubVersion;
	pstIcfAppVer->u32RevisVersion = stIcfAppVersion.nRevisVersion;
	pstIcfAppVer->u32VersionYear = stIcfAppVersion.nVersionYear;
	pstIcfAppVer->u32VersionMonth = stIcfAppVersion.nVersionMonth;
	pstIcfAppVer->u32VersionDay = stIcfAppVersion.nVersionDay;
	
exit:
    return s32Ret;
}

/**
 * @function:   engine_comm_set_config
 * @brief:      配置参数
 * @param[in]:  ENGINE_COMM_USER_CONFIG_PRM *pstConfigPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 engine_comm_set_config(ENGINE_COMM_USER_CONFIG_PRM *pstConfigPrm)
{
    INT32 s32Ret = SAL_FAIL;

    ICF_CONFIG_PARAM configParam = {0};

    /* checker */
    ENGINE_HAL_CHECK_RET(NULL == pstConfigPrm, exit, "pstConfigPrm == null!");

    configParam.nNodeID = pstConfigPrm->u32NodeId;
    configParam.nKey = pstConfigPrm->u32CfgKey;
    configParam.pConfigData = pstConfigPrm->pData;
    configParam.nConfSize = pstConfigPrm->u32DataSize;

    s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfSetConfig(pstConfigPrm->pChnHandle,
                                                     &configParam,
                                                     sizeof(ICF_CONFIG_PARAM));
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "set config failed!");

exit:
    return s32Ret;
}

/**
 * @function:   engine_comm_get_config
 * @brief:      获取配置
 * @param[in]:  ENGINE_COMM_USER_CONFIG_PRM *pstConfigPrm
 * @param[out]: ENGINE_COMM_USER_CONFIG_PRM *pstConfigPrm
 * @return:     static INT32
 */
static INT32 engine_comm_get_config(ENGINE_COMM_USER_CONFIG_PRM *pstConfigPrm)
{
    INT32 s32Ret = SAL_FAIL;

    ICF_CONFIG_PARAM configParam = {0};

    /* checker */
    ENGINE_HAL_CHECK_RET(NULL == pstConfigPrm, exit, "pstConfigPrm == null!");

    configParam.nNodeID = pstConfigPrm->u32NodeId;
    configParam.nKey = pstConfigPrm->u32CfgKey;
    configParam.pConfigData = pstConfigPrm->pData;
    configParam.nConfSize = pstConfigPrm->u32DataSize;

    s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfGetConfig(pstConfigPrm->pChnHandle,
                                                     &configParam,
                                                     sizeof(ICF_CONFIG_PARAM));
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "set config failed!");

exit:
    return s32Ret;
}

/**
 * @function:   engine_comm_input_data
 * @brief:      送数据
 * @param[in]:  ENGINE_COMM_INPUT_DATA_INFO *pstIcfCommInputDataInfo
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 engine_comm_input_data(ENGINE_COMM_INPUT_DATA_INFO *pstIcfCommInputDataInfo)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i;
    ICF_MEM_TYPE_E enIcfMemType = 0;
    ICF_BLOB_FORMAT_E enIcfImgType = 0;

    VOID *pChnHandle = NULL;
    ENGINE_COMM_INPUT_DATA_IMG_INFO *pstIcfCommInputDataImg = NULL;

    ICF_INNER_CHN_BUF *pstInnerChnBuf = NULL;
    ICF_INPUT_DATA *pstIcfInputData = NULL;

    /* checker */
    ENGINE_HAL_CHECK_RET(NULL == pstIcfCommInputDataInfo, exit, "pstIcfCommInputDataInfo == null!");

    pChnHandle = pstIcfCommInputDataInfo->pChnHandle;
    ENGINE_HAL_CHECK_RET(NULL == pChnHandle, exit, "pChnHandle == null!");

    /* 获取空闲通道缓存 */
    s32Ret = icf_engine_get_free_inner_buf(pChnHandle, &pstInnerChnBuf);
    ENGINE_HAL_CHECK_RET((SAL_SOK != s32Ret || NULL == pstInnerChnBuf), exit, "get free inner buf failed!");

    pstIcfInputData = &pstInnerChnBuf->stIcfInputDataBuf;

    pstIcfInputData->nBlobNum = pstIcfCommInputDataInfo->stIaImgPrvtCtrlPrm.u32InputDataImgNum;
    if (1 != pstIcfInputData->nBlobNum)
    {
        SAL_LOGE("blob num err %d ! \n", pstIcfInputData->nBlobNum);
    }

    for (i = 0; i < pstIcfInputData->nBlobNum; i++)
    {
        pstIcfCommInputDataImg = &pstIcfCommInputDataInfo->stIaImgPrvtCtrlPrm.stInputDataImg[i];

        s32Ret = icf_user_mem_type_to_icf_type(pstIcfCommInputDataImg->enInputDataMemType, &enIcfMemType);
        ENGINE_HAL_CHECK_RET_NO_LOOP(SAL_SOK != s32Ret, "icf_user_mem_type_to_icf_type failed!");

        s32Ret = icf_user_img_type_to_icf_type(pstIcfCommInputDataImg->enImgType, &enIcfImgType);
        ENGINE_HAL_CHECK_RET_NO_LOOP(SAL_SOK != s32Ret, "icf_user_img_type_to_icf_type failed!");

        /* 业务层格式转换到icf支持的类型 */
        pstIcfInputData->stBlobData[i].eBlobFormat = enIcfImgType;
        pstIcfInputData->stBlobData[i].nSpace = enIcfMemType;
        pstIcfInputData->stBlobData[i].nShape[0] = pstIcfCommInputDataImg->u32W;
        pstIcfInputData->stBlobData[i].nShape[1] = pstIcfCommInputDataImg->u32H;
        pstIcfInputData->stBlobData[i].nFrameNum = pstIcfCommInputDataImg->u64FrameNum;
        pstIcfInputData->stBlobData[i].nTimeStamp = 40 * (pstIcfInputData->stBlobData[i].nFrameNum + 1);

        /* 回调处理函数校验 */
        ENGINE_HAL_CHECK_RET(NULL == pstIcfCommInputDataInfo->stIaImgPrvtCtrlPrm.pFuncProcIaImg, exit, "proc ia img func null!");
        ENGINE_HAL_CHECK_RET(NULL == pstIcfCommInputDataInfo->stIaPrmPrvtCtrlPrm.pFuncProcIaPrm, exit, "proc ia prm func null!");

        /* 拷贝智能模块的图片信息到icf框架结构体中 */
        s32Ret = pstIcfCommInputDataInfo->stIaImgPrvtCtrlPrm.pFuncProcIaImg((VOID *)pstIcfCommInputDataImg,
                                                                            (VOID *)pstIcfInputData->stBlobData[i].pData);
        ENGINE_HAL_CHECK_RET((SAL_SOK != s32Ret), exit, "proc ia image failed!");

        /* szl_todo: 拷贝智能模块的参数信息到icf框架结构体中 */
        s32Ret = pstIcfCommInputDataInfo->stIaPrmPrvtCtrlPrm.pFuncProcIaPrm((VOID *)pstIcfCommInputDataInfo->stIaPrmPrvtCtrlPrm.pIaAlgPrm,
                                                                            (VOID *)pstIcfInputData->stBlobData[i].pData);
        ENGINE_HAL_CHECK_RET((SAL_SOK != s32Ret), exit, "proc ia param failed!");

        *(pstIcfInputData->pUseFlag[i]) = SAL_TRUE;
    }

    /* 填充引擎框架内部使用的全局缓存信息，注意在回调处理时需要释放 */
    pstIcfCommInputDataInfo->stInputUserInfo.stEnginePrvtInfo.pEngineInnerChnPrvtData = (VOID *)pstInnerChnBuf;
    pstIcfCommInputDataInfo->stInputUserInfo.stEnginePrvtInfo.pFuncProcEngPrvtData = (ENGINE_COMM_ENGINE_PROC_PRVT_DATA)icf_engine_rls_inner_chn_buf;

    pstIcfInputData->pUserPtr = (VOID *)&pstIcfCommInputDataInfo->stInputUserInfo;
    pstIcfInputData->nUserDataSize = sizeof(ENGINE_COMM_INPUT_DATA_USER_INFO);

    s32Ret = g_stIcfLibInfo.stIcfLibApi.IcfInputData(pChnHandle, pstIcfInputData, sizeof(ICF_INPUT_DATA));

exit:
    return s32Ret;
}

/**
 * @function:   engine_comm_deinit
 * @brief:      去初始化
 * @param[in]:  None
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 engine_comm_deinit(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    /* icf destroy */
    s32Ret = icf_engine_destroy();
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "icf_engine_destroy failed!");

    /* icf unload model */
    s32Ret = icf_engine_unload_model();
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "icf_engine_unload_model failed!");

    /* icf deinit */
    s32Ret = icf_engine_deinit();
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "icf_engine_deinit failed!");

    /* icf inner channel needed resource */
    s32Ret = icf_engine_deinit_InnerChn();
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "icf_engine_deinit_InnerChn failed!");

exit:
    return s32Ret;
}

/**
 * @function:   engine_comm_init
 * @brief:      初始化
 * @param[in]:  ENGINE_COMM_INIT_PRM_S *pstInitPrm
 * @param[out]: ENGINE_COMM_INIT_OUTPUT_PRM *pstInitOutput
 * @return:     static INT32
 */
static INT32 engine_comm_init(const ENGINE_COMM_INIT_PRM_S *pstInitPrm, ENGINE_COMM_INIT_OUTPUT_PRM *pstInitOutput)
{
    INT32 s32Ret = SAL_FAIL;

    /* checker */
    ENGINE_HAL_CHECK_RET(NULL == pstInitPrm, exit, "pstInitPrm == null!");
    ENGINE_HAL_CHECK_RET(NULL == pstInitOutput, exit, "pstInitOutput == null!");

    /* dlopen解析动态库，并全局保存函数指针 */
    s32Ret = icf_rslv_lib_sym();
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "icf_rslv_lib_sym failed!");

    /* icf init */
    s32Ret = icf_engine_init(pstInitPrm);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "icf_engine_init failed!");

    /* icf load model */
    s32Ret = icf_engine_load_model(pstInitPrm);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "icf_engine_load_model failed!");

    /* icf create */
    s32Ret = icf_engine_create(pstInitPrm, pstInitOutput);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "icf_engine_create failed!");

    /* icf inner channel needed resource */
    s32Ret = icf_engine_init_InnerChn(pstInitPrm);
    ENGINE_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "icf_engine_init_InnerChn failed!");

exit:
    pstInitOutput->s32InitStatus = s32Ret;
    return s32Ret;
}

#endif

static ENGINE_COMM_FUNC_P g_stEngineCommFunc =
{
    .init = engine_comm_init,
    .deinit = engine_comm_deinit,
    .get_icf_version = engine_comm_get_version,
    .get_icf_app_version = engine_comm_get_app_version,
    .set_config = engine_comm_set_config,
    .get_config = engine_comm_get_config,
    .input_data = engine_comm_input_data,
    .get_rslt = engine_comm_get_alg_result,
};

/**
 * @function:   engine_comm_register
 * @brief:      注册通用接口函数
 * @param[in]:  None
 * @param[out]: None
 * @return:     ENGINE_COMM_FUNC_P *
 */
ENGINE_COMM_FUNC_P *engine_comm_register(VOID)
{
    return &g_stEngineCommFunc;
}

