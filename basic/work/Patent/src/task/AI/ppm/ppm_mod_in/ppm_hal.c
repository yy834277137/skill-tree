/**
 * @file:   ppm_hal.c
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  人包关联hal层源文件
 * @author: sunzelin
 * @date    2020/12/28
 * @note:
 * @note \n History:
   1.日    期: 2020/12/28
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "sal.h"
#include "ppm_hal.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define PPM_HAL_CHECK_RET(ret, loop, str) \
    { \
        if (ret) \
        { \
            SAL_ERROR("%s, ret: 0x%x \n", str, ret); \
            goto loop; \
        } \
    }

#define PPM_HAL_CHECK_RET_NO_LOOP(ret, str) \
    { \
        if (ret) \
        { \
            SAL_ERROR("%s, ret: 0x%x \n", str, ret); \
        } \
    }

#define PPM_HAL_CHECK_PTR(ptr, loop, str) \
    { \
        if (!ptr) \
        { \
            SAL_ERROR("%s \n", str); \
            goto loop; \
        } \
    }

#define PPM_HAL_CHECK_SUB_MOD_ID(idx, value) \
    { \
        if (idx > PPM_SUB_MOD_MAX_NUM - 1) \
        { \
            SAL_ERROR("Invalid sub mod idx %d \n", idx); \
            return value; \
        } \
    }

#define DEMO_BPC_CALIB_PARAM_PATH	"./mtme/config/inner_param_dpt_310.txt"
#define PPM_CFG_FILE_NUM			(4)   /* 配置文件数量 */
#define PPM_CFG_PATH_LENGTH			(64)  /* 配置文件路径长度 */
/* 关联匹配图ID */
#define PPM_MATCH_GRAPH_ID          (1)
/* 跨相机标定图ID */
#define PPM_MCC_CALIB_GRAPH_ID      (2)
/* 单目人脸相机标定图ID */
#define PPM_FACE_CALIB_GRAPH_ID     (3)
/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/
/* 关联匹配通道-可见光包裹回调 */
extern VOID Ppm_DrvMatchPackRsltCbFunc(INT32 nNodeID, 
			                           INT32 nCallBackType, 
			                           VOID *pstOutput, 
			                           UINT32 nSize, 
			                           VOID *pUsr, 
			                           INT32 nUserSize);

/* 关联匹配通道-人脸抓拍回调 */
extern VOID Ppm_DrvMatchFaceRsltCbFunc(INT32 nNodeID, 
	                                   INT32 nCallBackType, 
	                                   VOID *pstOutput, 
	                                   UINT32 nSize, 
	                                   VOID *pUsr, 
	                                   INT32 nUserSize);

/* 关联匹配通道-POS处理 */
extern VOID Ppm_DrvMatchPosRsltCbFunc(INT32 nNodeID, 
	                                  INT32 nCallBackType, 
	                                  VOID *pstOutput, 
	                                  UINT32 nSize, 
	                                  VOID *pUsr, 
	                                  INT32 nUserSize);

/* 跨相机标定通道-回调 */
extern VOID Ppm_DrvMccCalibCbFunc(INT32 nNodeID, 
                                  INT32 nCallBackType, 
                                  VOID *pstOutput, 
                                  UINT32 nSize, 
                                  VOID *pUsr, 
                                  INT32 nUserSize);

/* 人脸单目相机标定通道-回调 */
extern VOID Ppm_DrvFaceCalibCbFunc(INT32 nNodeID, 
                                   INT32 nCallBackType, 
                                   VOID *pstOutput, 
                                   UINT32 nSize,
                                   VOID *pUsr, 
                                   INT32 nUserSize);

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/
static PPM_MOD_S g_stPpmModPrm = {0};

/* 默认的匹配矩阵参数 */
static float fCamHeight = 3.10;
static double r_p2p[3][3] = {{0.096204286058416355, -0.99480466684180124, 0.033292794622311495},
                             {0.10734818096007570, 0.043622524181663180, 0.99326403510274308},
                             {-0.98955601326481435, -0.091982336422778588, 0.11098714519102437}};
static double t_p2p[3] = {133.34851737012993, -1434.5156687687139, 1462.9398563447233};

static double tri_matrix_inv[3][3] = {{0.0014641876, 0, -1.404888},
                                      {0, 0.0014618126, -0.78864789},
                                      {0, 0, 1}}; /* 需要更新实测 */
static double face_matrix[3][3] = {{1486.006714, 0.000000, 959.231995},
                                   {0.000000, 1478.289307, 569.886780},
                                   {0.000000, 0.000000, 1.000000}}; /* 需要更新实测 */
static double face_Distmatrix[8] = {-0.084097, -0.010785, 0.000334, -0.000815, 0.000000, 0.000000, 0.000000, 0.000000};

static UINT32 uiInitModeFlag = SAL_FALSE;
static UINT32 uiPackSetSenity = MTME_PACK_MATCH_SENSITIVITY_STEP3;

static CHAR g_Ppmcfg_path[PPM_CFG_FILE_NUM][PPM_CFG_PATH_LENGTH] =
{
    {"./mtme/config/AlgCfgTest.json"},             /* AlgCfgPath */
    {"./mtme/config/TASK.json"},                   /* TaskCfgPath */
    {"./mtme/config/toolkit_cfg.json"},             /* ToolkitCfgPath */
    {"./mtme/config/hisi_param.json"},       /* AppParamCfgPath */
};

/* json配置文件解析一级键值名称 */
static CHAR *g_PpmjSonMainClassKeyTab[PPM_JSON_CLASS_NUM] =
{
    /* 第一级键值名称 */
    "DPT_PARAM",
};

/* json配置文件解析二级键值名称 */
static CHAR *g_PpmjSonSubClassKeyTab[PPM_JSON_SUB_CLASS_NUM] =
{
    /* 第一级包含的子键值名称 */
    "tri_cam_height",
    "tri_cam_incline",
    "tri_cam_pitch",
    /* 第二级，暂时没有，若有需求往后面增加 */

};

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/*******************************************************************************
* 函数名  : Ppm_HalGetIcfParaJson
* 描  述  : 获取相应的配置文件路径
* 输  入  : - cfg_num: 文件num
* 输  出  : 无
* 返回值  : 对应的json文件路径
*******************************************************************************/
CHAR *Ppm_HalGetIcfParaJson(PPM_CFG_JSONFILE_E cfg_num)
{
    /*input check*/
    if (cfg_num > PPM_PARAM_CFG_PATH)
    {
        PPM_LOGE("Input json file num ERROR ! \n");
        return NULL;
    }

    return g_Ppmcfg_path[cfg_num];

}

/*******************************************************************************
* 函数名  : Ppm_HalGetInitFlag
* 描  述  : 获取模式初始化状态标志
* 输  入  : 无
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
UINT32 Ppm_HalGetInitFlag(void)
{
    return uiInitModeFlag;
}

/*******************************************************************************
* 函数名  : PPM_DrvSetProcFlag
* 描  述  : 设置模式切换状态标志
* 输  入  : - UINT32 uiFlag: 初始化标志
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
void Ppm_HalSetProcFlag(UINT32 uiFlag)
{
    if (uiFlag > SAL_TRUE)
    {
        SVA_LOGE("Invalid flag %d \n", uiFlag);
        return;
    }

    uiInitModeFlag = uiFlag;
    return;
}

/*******************************************************************************
* 函数名  : Ppm_DrvSetPackSensity
* 描  述  : 设置可见光包裹的检测灵敏度
* 输  入  : - UINT32 sensity: 检测灵敏度
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
VOID Ppm_DrvSetPackSensity(UINT32 sensity)
{
    if (sensity < SAL_TRUE || sensity > 4)
    {
        SVA_LOGE("Invalid flag %d \n", sensity);
        return;
    }

    uiPackSetSenity = sensity;
}

/**
 * @function:   Ppm_HalGetModPrm
 * @brief:      获取模块全局参数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     PPM_MOD_S *
 */
PPM_MOD_S *Ppm_HalGetModPrm(VOID)
{
    return &g_stPpmModPrm;
}

/**
 * @function:   Ppm_HalGetSubModPrm
 * @brief:      获取子模块全局参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     PPM_SUB_MOD_S *
 */
PPM_SUB_MOD_S *Ppm_HalGetSubModPrm(UINT32 chan)
{
    PPM_HAL_CHECK_SUB_MOD_ID(chan, NULL);

    return &g_stPpmModPrm.stPpmSubMod[chan];
}

/**
 * @function   Ppm_HalCheckSubModSts
 * @brief      查询子模块通道是否已初始化
 * @param[in]  PPM_SUB_MOD_TYPE_E enChnIdx  
 * @param[out] None
 * @return     BOOL
 */
BOOL Ppm_HalCheckSubModSts(PPM_SUB_MOD_TYPE_E enChnIdx)
{
    PPM_HAL_CHECK_SUB_MOD_ID(enChnIdx, SAL_FALSE);
	
	return !!(g_stPpmModPrm.stPpmSubMod[enChnIdx].uiUseFlag);
}

/**
 * @function   Ppm_HalGetSubModChnPrm
 * @brief      获取子模块通道全局参数
 * @param[in]  PPM_SUB_MOD_TYPE_E enChnIdx  
 * @param[out] None
 * @return     VOID *
 */
VOID *Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_TYPE_E enChnIdx)
{
	VOID *pChnPrm = NULL;
	
    PPM_HAL_CHECK_SUB_MOD_ID(enChnIdx, NULL);

	switch(enChnIdx)
	{
		case PPM_SUB_MOD_MATCH:
		{
			pChnPrm = (VOID *)&g_stPpmModPrm.stPpmSubMod[enChnIdx].uSubModChnPrm.stMatchChnPrm;
			break;
		}
		case PPM_SUB_MOD_MCC_CALIB:
		{
			pChnPrm = (VOID *)&g_stPpmModPrm.stPpmSubMod[enChnIdx].uSubModChnPrm.stMccCalibChnPrm;
			break;
		}
		case PPM_SUB_MOD_FACE_CALIB:
		{
			pChnPrm = (VOID *)&g_stPpmModPrm.stPpmSubMod[enChnIdx].uSubModChnPrm.stFaceCalibChnPrm;
			break;
		}	
		default:
		{
			PPM_LOGE("invalid chn prm idx %d \n", enChnIdx);
			break;
		}	
	}
	
    return (VOID *)pChnPrm;
}

/**
 * @function:   Ppm_HalGetFreeIptData
 * @brief:      获取空闲的input节点
 * @param[in]:  UINT32 chan
 * @param[in]:  UINT32 *puiIdx
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalGetFreeIptData(UINT32 chan, UINT32 *puiIdx)
{
    UINT32 i = 0;

    PPM_SUB_MOD_S *pstSubModPrm = NULL;
    PPM_INPUT_DATA_INFO *pstInputDataInfo = NULL;

    PPM_HAL_CHECK_SUB_MOD_ID(chan, SAL_FAIL);
    PPM_HAL_CHECK_PTR(puiIdx, err, "puiIdx == null!");

    pstSubModPrm = Ppm_HalGetSubModPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstSubModPrm, err, "pstSubModPrm == null!");

    for (i = pstSubModPrm->stIptDataPrm.uiIdx; i < pstSubModPrm->stIptDataPrm.uiIdx + pstSubModPrm->stIptDataPrm.uiCnt; i++)
    {
        pstInputDataInfo = &pstSubModPrm->stIptDataPrm.stInputDataInfo[i % pstSubModPrm->stIptDataPrm.uiCnt];
        if (pstInputDataInfo->bUsed)
        {
            continue;
        }

        pstInputDataInfo->bUsed = SAL_TRUE;
        break;
    }

    if (i >= pstSubModPrm->stIptDataPrm.uiIdx + pstSubModPrm->stIptDataPrm.uiCnt)
    {
        SAL_ERROR("get free input data failed! \n");
        return SAL_FAIL;
    }

    pstSubModPrm->stIptDataPrm.uiIdx = (pstSubModPrm->stIptDataPrm.uiIdx + 1) % pstSubModPrm->stIptDataPrm.uiCnt;

    *puiIdx = i % pstSubModPrm->stIptDataPrm.uiCnt;
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_HalMemPoolSystemAllocCallback
 * @brief:      内存申请回调函数
 * @param[in]:  void *pInitHandle
 * @param[in]:  ICF_MEM_INFO *pstMemInfo
 * @param[in]:  ICF_MEM_BUFFER *pstMemBuffer
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalMemPoolSystemAllocCallback(void *pInitHandle,
                                        ICF_MEM_INFO *pstMemInfo,
                                        ICF_MEM_BUFFER *pstMemBuffer)
{
	INT32 s32Ret = SAL_FAIL;
	
    VOID *va = NULL;
	ALLOC_VB_INFO_S stVbInfo = {0};

    PPM_HAL_CHECK_PTR(pstMemInfo, exit, "pstMemInfo == null!");
    PPM_HAL_CHECK_PTR(pstMemBuffer, exit, "pstMemBuffer == null!");

    switch (pstMemInfo->eMemType)
    {
        case ICF_MEM_MALLOC:
        {
            va = SAL_memZalloc(pstMemInfo->nMemSize, "ppm", "engine");
            if (NULL == va)
            {
                FACE_LOGE("malloc failed! \n");
                goto exit;
            }
            pstMemBuffer->pVirMemory = (void *)va;
            pstMemBuffer->pPhyMemory = (VOID *)va;
            break;
        }
        case ICF_RN_MEM_MMZ_CMA_WITH_CACHE:
        {
            SVA_LOGI("RK MEM ALLOC, lBufSize13 = %d\n", (UINT32)pstMemInfo->nMemSize);
            s32Ret = mem_hal_iommuMmzAlloc(pstMemInfo->nMemSize, "ppm", "engine", NULL, SAL_TRUE, &stVbInfo);
            if (SAL_SOK != s32Ret)
            {
                SVA_LOGE("rk cma Malloc err!!\n");
                goto exit;
            }
            PPM_LOGD("cache: stVbInfo.pVirAddr %p, stVbInfo.u64PhysAddr %llu, %p  \n", stVbInfo.pVirAddr, stVbInfo.u64PhysAddr,(void *)stVbInfo.u64PhysAddr);

			pstMemBuffer->pVirMemory = (void *)stVbInfo.pVirAddr;
			pstMemBuffer->pPhyMemory = (void *)stVbInfo.pVirAddr;
            break;
        }
        default:
        {
            SVA_LOGE("invalid mem type %d \n", pstMemInfo->eMemType);
            goto exit;
        }
    }

	s32Ret = SAL_SOK;
exit:
    return s32Ret;
}

/**
 * @function   Ppm_HalMemPoolSystemFreeCallback
 * @brief      引擎内存释放回调函数
 * @param[in]  void *pInitHandle             
 * @param[in]  ICF_MEM_INFO *pMemInfo        
 * @param[in]  ICF_MEM_BUFFER *pstMemBuffer  
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalMemPoolSystemFreeCallback(void *pInitHandle,
                                       ICF_MEM_INFO *pMemInfo,
                                       ICF_MEM_BUFFER *pstMemBuffer)
{
	INT32 s32Ret = SAL_FAIL;
	
    PPM_HAL_CHECK_PTR(pMemInfo, exit, "pMemInfo == null!");
    PPM_HAL_CHECK_PTR(pstMemBuffer, exit, "pstMemBuffer == null!");

    switch (pMemInfo->eMemType)
    {
        case ICF_MEM_MALLOC:
        {
            SAL_memfree(pstMemBuffer->pVirMemory, "ppm", "engine");
            break;
        }
        case ICF_RN_MEM_MMZ_CMA_WITH_CACHE:
		{
            s32Ret = mem_hal_iommuMmzFree(pMemInfo->nMemSize, "ppm", "engine", (UINT64)pstMemBuffer->pPhyMemory, (VOID *)pstMemBuffer->pVirMemory, 0);
			if (s32Ret != SAL_SOK)
			{
				SVA_LOGE("mmz free Err! sts %x\n", s32Ret);
				goto exit;
			}

			break;
		}
        default:
        {
			PPM_LOGE("invalid mem type %d \n", pMemInfo->eMemType);
			goto exit;
        }
    }

	s32Ret = SAL_SOK;

exit:
    return s32Ret;
}

/**
 * @function:   Ppm_HalDebugDumpData
 * @brief:      ppm模块dump数据接口
 * @param[in]:  INT8 *pData
 * @param[in]:  INT8 *pPath
 * @param[in]:  UINT32 uiSize
 * @param[out]: None
 * @return:     void
 */
void Ppm_HalDebugDumpData(CHAR *pData, CHAR *pPath, UINT32 uiSize, UINT32 uiFlag)
{
    INT32 ret = 0;
    CHAR sz[2][16] = {"w+", "a+"};

    FILE *fp = NULL;

    fp = fopen(pPath, sz[uiFlag]);
    if (!fp)
    {
        SAL_ERROR("fopen %s failed! \n", pPath);
        goto exit;
    }

    ret = fwrite(pData, uiSize, 1, fp);
    if (ret < 0)
    {
        SAL_ERROR("fwrite err! \n");
        goto exit;
    }

    fflush(fp);

exit:
    if (NULL != fp)
    {
        fclose(fp);
        fp = NULL;
    }

    return;
}

/**
 * @function:   Ppm_HalFree
 * @brief:      释放一段malloc内存
 * @param[in]:  VOID *p
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ppm_HalFree(VOID *p)
{
    PPM_HAL_CHECK_PTR(p, exit, "no need free!");

    SAL_memfree(p, "PPM", "ppm_malloc");

exit:
    return SAL_SOK;
}

/**
 * @function:   Ppm_HalMalloc
 * @brief:      申请一段malloc内存
 * @param[in]:  VOID **pp
 * @param[in]:  UINT32 uiSize
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ppm_HalMalloc(VOID **pp, UINT32 uiSize)
{
    if (NULL != *pp)
    {
        SAL_INFO("no need malloc! \n");
        goto exit;
    }

    *pp = SAL_memMalloc(uiSize, "PPM", "ppm_malloc");
    PPM_HAL_CHECK_PTR(*pp, err, "malloc failed!");

    sal_memset_s(*pp, uiSize, 0x00, uiSize);

exit:
    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_HalMemFree
 * @brief:      PPM模块内存释放
 * @param[in]:  VOID *pVir
 * @param[out]: None
 * @return:     VOID *
 */
VOID Ppm_HalMemFree(VOID *pVir)
{
#ifdef PPM_ST_USE_MMZ
    (VOID)mem_hal_mmzFree(pVir, "PPM", pcMemName);
#else
    (VOID)Ppm_HalFree(pVir);
#endif

    return;
}

/**
 * @function:   Ppm_HalMemAlloc
 * @brief:      PPM模块内存申请
 * @param[in]:  UINT32 uiSize
 * @param[out]: None
 * @return:     VOID *
 */
VOID *Ppm_HalMemAlloc(UINT32 uiSize, BOOL bCache, CHAR *pcMemName)
{
    VOID *pVir = NULL;

#ifdef PPM_ST_USE_MMZ
    (VOID)mem_hal_mmzAlloc(uiSize, "PPM", pcMemName, NULL, SAL_FALSE, NULL, &pVir))
#else
    (VOID)Ppm_HalMalloc(&pVir, uiSize);
#endif
    return pVir;
}

/**
 * @function:   Ppm_HalSetMatchMatrixPrm
 * @brief:      设置关联匹配矩阵参数
 * @param[in]:  UINT32 chan
 * @param[in]:  PPM_MATCH_MATRIX_INFO_S *pstMatchMatrixInfo
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalSetMatchMatrixPrm(UINT32 chan, PPM_MATCH_MATRIX_INFO_S *pstMatchMatrixInfo)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 i = 0;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;
    PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;

    MTME_FACE_MATCH_MTRIX_T stFaceMatchParam = {0};
	ICF_CONFIG_PARAM stConfigParam = {0};

    PPM_HAL_CHECK_SUB_MOD_ID(chan, SAL_FAIL);
	PPM_HAL_CHECK_RET(SAL_TRUE != Ppm_HalCheckSubModSts(PPM_SUB_MOD_MATCH), exit, "sub mod not init");

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstPpmGlobalPrm, exit, "pstPpmGlobalPrm == null!");

    stFaceMatchParam.f32Cam_height = pstMatchMatrixInfo->fCamHeight;

    for (i = 0; i < pstMatchMatrixInfo->stR_MatrixInfo.uiRowCnt; i++)
    {
        memcpy(stFaceMatchParam.r_p2p[i],
               pstMatchMatrixInfo->stR_MatrixInfo.fMatrix[i],
               pstMatchMatrixInfo->stR_MatrixInfo.uiColCnt * sizeof(double));
    }

    memcpy(stFaceMatchParam.t_p2p,
           pstMatchMatrixInfo->stT_MatrixInfo.fMatrix[0],
           pstMatchMatrixInfo->stT_MatrixInfo.uiColCnt * sizeof(double));

    for (i = 0; i < pstMatchMatrixInfo->stFaceInterMatrixInfo.uiRowCnt; i++)
    {
        memcpy(stFaceMatchParam.face_matrix[i],
               pstMatchMatrixInfo->stFaceInterMatrixInfo.fMatrix[i],
               pstMatchMatrixInfo->stFaceInterMatrixInfo.uiColCnt * sizeof(double));
    }

	/* 新增人脸相机畸变矫正矩阵参数 */
	if (1 != pstMatchMatrixInfo->stFaceDistMatrixInfo.uiRowCnt 
		|| 8 != pstMatchMatrixInfo->stFaceDistMatrixInfo.uiColCnt)
	{
		PPM_LOGE("invalid face distort matrix prm! row %d, col %d \n",
			     pstMatchMatrixInfo->stFaceDistMatrixInfo.uiRowCnt, pstMatchMatrixInfo->stFaceDistMatrixInfo.uiColCnt);
		return SAL_FAIL;
	}
	
    memcpy(&stFaceMatchParam.dist_param[0],
           pstMatchMatrixInfo->stFaceDistMatrixInfo.fMatrix[0],
           pstMatchMatrixInfo->stFaceDistMatrixInfo.uiColCnt * sizeof(double));

    for (i = 0; i < pstMatchMatrixInfo->stTriCamInvMatrixInfo.uiRowCnt; i++)
    {
        memcpy(stFaceMatchParam.tri_matrix_inv[i],
               pstMatchMatrixInfo->stTriCamInvMatrixInfo.fMatrix[i],
               pstMatchMatrixInfo->stTriCamInvMatrixInfo.uiColCnt * sizeof(double));
    }

	/* 关联匹配通道全局参数 */
    pstMatchChnPrm = (PPM_SUB_MOD_MATCH_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstMatchChnPrm, exit, "pstMatchChnPrm == null!");

	stConfigParam.nNodeID	= MTME_MATCH_PACK_NODE_ID;
	stConfigParam.nKey		= MTME_SET_FACE_MATCH_MTRIX_PARAM;
	stConfigParam.pConfigData = &stFaceMatchParam;
	stConfigParam.nConfSize = sizeof(MTME_FACE_MATCH_MTRIX_T);
	
	s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfSetConfig(pstMatchChnPrm->stCreateHandle.pChannelHandle, 
												         &stConfigParam,
												         sizeof(ICF_CONFIG_PARAM));
    PPM_HAL_CHECK_RET(s32Ret, exit, "ICF_Set_config MTME_SET_FACE_MATCH_MTRIX_PARAM failed!");

exit:
    return s32Ret;
}

/**
 * @function:   Ppm_HalSetCapRegion
 * @brief:      设定抓拍AB区域
 * @param[in]:  MTME_CAPTURE_REGION_T *pstCapRegion
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalSetCapRegion(MTME_CAPTURE_REGION_T *pstCapRegion)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;
    PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;
	
    ICF_CONFIG_PARAM stConfigParam = {0};

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstPpmGlobalPrm, exit, "pstPpmGlobalPrm == null!");

	/* 关联匹配通道全局参数 */
    pstMatchChnPrm = Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstMatchChnPrm, exit, "pstMatchChnPrm == null!");

	stConfigParam.nNodeID	= MTME_MATCH_PACK_NODE_ID;
	stConfigParam.nKey		= MTME_SET_CAPTURE_REGION;
	stConfigParam.pConfigData = pstCapRegion;
	stConfigParam.nConfSize = sizeof(MTME_CAPTURE_REGION_T);
	
	s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfSetConfig(pstMatchChnPrm->stCreateHandle.pChannelHandle, 
												         &stConfigParam,
												         sizeof(ICF_CONFIG_PARAM));
    PPM_HAL_CHECK_RET(s32Ret, exit, "ICF_Set_config MTME_SET_CAPTURE_REGION failed!");

exit:
    return s32Ret;
}

/**
 * @function   Ppm_HalSetMatchPackSensity
 * @brief      配置关联通道可见光包裹抓拍灵敏度
 * @param[in]  MTME_PACK_MATCH_SENSITIVITY_E enSensity  
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetMatchPackSensity(MTME_PACK_MATCH_SENSITIVITY_E enSensity)
{
	INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstModPrm = NULL;
	PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;

	ICF_CONFIG_PARAM stConfigParam = {0};

    PPM_LOGI("set pack match sensity %d \n", enSensity);

    pstModPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstModPrm, exit, "pstModPrm == null!");

	/* 关联匹配通道全局参数 */
    pstMatchChnPrm = (PPM_SUB_MOD_MATCH_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstMatchChnPrm, exit, "pstMatchChnPrm == null!");

	stConfigParam.nNodeID	= MTME_MATCH_PACK_NODE_ID;
	stConfigParam.nKey		= MTME_SET_PACK_MATCH_SENSITIVITY;
	stConfigParam.pConfigData = &enSensity;
	stConfigParam.nConfSize = sizeof(INT32);

	s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfSetConfig(pstMatchChnPrm->stCreateHandle.pChannelHandle, 
												   &stConfigParam,
												   sizeof(ICF_CONFIG_PARAM));  
    PPM_HAL_CHECK_RET(s32Ret, exit, "ICF_Set_config MTME_SET_PACK_MATCH_SENSITIVITY failed!");
	
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalSetMatchPackRoi
 * @brief      配置关联通道可见光包裹抓拍roi
 * @param[in]  MTME_RECT_T *pstPackRoiInfo 
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetMatchPackRoi(MTME_RECT_T *pstPackRoiInfo)
{
	INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstModPrm = NULL;
	PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;

	ICF_CONFIG_PARAM stConfigParam = {0};

    PPM_LOGI("set match pack roi [%f, %f] [%f, %f] \n",
		     pstPackRoiInfo->x, pstPackRoiInfo->y, pstPackRoiInfo->width, pstPackRoiInfo->height);

    pstModPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstModPrm, exit, "pstModPrm == null!");

	/* 关联匹配通道全局参数 */
    pstMatchChnPrm = (PPM_SUB_MOD_MATCH_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstMatchChnPrm, exit, "pstMatchChnPrm == null!");

	stConfigParam.nNodeID	= MTME_MATCH_PACK_NODE_ID;
	stConfigParam.nKey		= MTME_SET_PACK_MATCH_ROI;
	stConfigParam.pConfigData = pstPackRoiInfo;
	stConfigParam.nConfSize = sizeof(MTME_RECT_T);

	s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfSetConfig(pstMatchChnPrm->stCreateHandle.pChannelHandle, 
												   &stConfigParam,
												   sizeof(ICF_CONFIG_PARAM));  
    PPM_HAL_CHECK_RET(s32Ret, exit, "ICF_Set_config MTME_SET_PACK_MATCH_ROI failed!");
	
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalSetMatchFaceIouThresh
 * @brief      配置关联通道人脸IOU阈值
 * @param[in]  float fMatchFaceIou
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetMatchFaceIouThresh(float fMatchFaceIou)
{
	INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstModPrm = NULL;
	PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;

	ICF_CONFIG_PARAM stConfigParam = {0};

    PPM_LOGI("set match face iou thresh %f \n", fMatchFaceIou);

    pstModPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstModPrm, exit, "pstModPrm == null!");

	/* 关联匹配通道全局参数 */
    pstMatchChnPrm = (PPM_SUB_MOD_MATCH_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstMatchChnPrm, exit, "pstMatchChnPrm == null!");

	stConfigParam.nNodeID	= MTME_MATCH_FACE_NODE_ID;
	stConfigParam.nKey		= MTME_SET_FACE_MATCH_THR;
	stConfigParam.pConfigData = &fMatchFaceIou;
	stConfigParam.nConfSize = sizeof(float);

	s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfSetConfig(pstMatchChnPrm->stCreateHandle.pChannelHandle, 
												   &stConfigParam,
												   sizeof(ICF_CONFIG_PARAM));  
    PPM_HAL_CHECK_RET(s32Ret, exit, "ICF_Set_config MTME_SET_FACE_MATCH_THR failed!");
	
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalSetMatchFaceRoi
 * @brief      配置关联通道人脸抓拍roi
 * @param[in]  MTME_RECT_T *pstFaceRoiInfo
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetMatchFaceRoi(MTME_RECT_T *pstFaceRoiInfo)
{
	INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstModPrm = NULL;
	PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;

	ICF_CONFIG_PARAM stConfigParam = {0};

    PPM_LOGI("set match face roi [%f, %f] [%f, %f] \n", 
		      pstFaceRoiInfo->x, pstFaceRoiInfo->y, pstFaceRoiInfo->width, pstFaceRoiInfo->height);

    pstModPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstModPrm, exit, "pstModPrm == null!");

	/* 关联匹配通道全局参数 */
    pstMatchChnPrm = (PPM_SUB_MOD_MATCH_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstMatchChnPrm, exit, "pstMatchChnPrm == null!");

	stConfigParam.nNodeID	= MTME_MATCH_FACE_NODE_ID;
	stConfigParam.nKey		= MTME_SET_FACE_MATCH_ROI;
	stConfigParam.pConfigData = pstFaceRoiInfo;
	stConfigParam.nConfSize = sizeof(MTME_RECT_T);

	s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfSetConfig(pstMatchChnPrm->stCreateHandle.pChannelHandle, 
												   &stConfigParam,
												   sizeof(ICF_CONFIG_PARAM));  
    PPM_HAL_CHECK_RET(s32Ret, exit, "ICF_Set_config MTME_SET_FACE_MATCH_ROI failed!");
	
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalSetMatchFaceScoreFilter
 * @brief      配置关联通道人脸评分过滤
 * @param[in]  float fFilterScore
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetMatchFaceScoreFilter(float fFilterScore)
{
	INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstModPrm = NULL;
	PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;

	ICF_CONFIG_PARAM stConfigParam = {0};

    PPM_LOGI("set match face filter score %f \n", fFilterScore);

    pstModPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstModPrm, exit, "pstModPrm == null!");

	/* 关联匹配通道全局参数 */
    pstMatchChnPrm = (PPM_SUB_MOD_MATCH_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstMatchChnPrm, exit, "pstMatchChnPrm == null!");

	stConfigParam.nNodeID	= MTME_DFR_NODE_ID;
	stConfigParam.nKey		= MTME_SET_DFR_SCORE_FILTER;
	stConfigParam.pConfigData = &fFilterScore;
	stConfigParam.nConfSize = sizeof(float);

	s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfSetConfig(pstMatchChnPrm->stCreateHandle.pChannelHandle, 
												   &stConfigParam,
												   sizeof(ICF_CONFIG_PARAM));  
	PPM_HAL_CHECK_RET(s32Ret, exit, "ICF_Set_config MTME_SET_DFR_SCORE_FILTER failed!");
	
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalSetTrackObjConfThrs
 * @brief      配置目标跟踪置信度阈值
 * @param[in]  MTME_OBJ_CONFIDENCE_PARAM_T *pstObjConfPrm
 * @param[out] None
 * @return     INT32
 */
INT32 Ppm_HalSetTrackObjConfThrs(MTME_OBJ_CONFIDENCE_PARAM_T *pstObjConfPrm)
{
	INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstModPrm = NULL;
	PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;

	ICF_CONFIG_PARAM stConfigParam = {0};

    PPM_LOGI("set track obj thresh: enable %d, conf %f \n", 
		      pstObjConfPrm->enable, pstObjConfPrm->filter_threshold);

    pstModPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstModPrm, exit, "pstModPrm == null!");

	/* 关联匹配通道全局参数 */
    pstMatchChnPrm = (PPM_SUB_MOD_MATCH_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstMatchChnPrm, exit, "pstMatchChnPrm == null!");

	stConfigParam.nNodeID	= MTME_DECODE_NODE_ID;
	stConfigParam.nKey		= MTME_SET_OBJ_CONFIDENCE_THRESH;
	stConfigParam.pConfigData = pstObjConfPrm;
	stConfigParam.nConfSize = sizeof(MTME_OBJ_CONFIDENCE_PARAM_T);

	s32Ret = pstModPrm->stIcfFuncP.Ppm_IcfSetConfig(pstMatchChnPrm->stCreateHandle.pChannelHandle, 
												   &stConfigParam,
												   sizeof(ICF_CONFIG_PARAM));  
	PPM_HAL_CHECK_RET(s32Ret, exit, "ICF_Set_config MTME_SET_OBJ_CONFIDENCE_THRESH failed!");
	
exit:
	return s32Ret;
}

/**
 * @function:   Ppm_HalSetMatchChnDefaultCfg
 * @brief:      设置关联匹配引擎通道默认参数
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_HalSetMatchChnDefaultCfg(VOID)
{
    INT32 s32Ret = SAL_FAIL;

	PPM_HAL_CHECK_RET(SAL_TRUE != Ppm_HalCheckSubModSts(PPM_SUB_MOD_MATCH), err, "sub mod not init");

#if 1	/* 配置PACK_MATCH灵敏度 */
	s32Ret = Ppm_HalSetMatchPackSensity(uiPackSetSenity);
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalSetMatchPackSensity failed!");
#endif

#if 1	/* 配置PACK_MATCH */
	MTME_RECT_T stPackRoiInfo = {0};

	stPackRoiInfo.x = 632.0 / DEMO_SRC_WIDTH;
	stPackRoiInfo.y = 111.0 / DEMO_SRC_HEIGHT;
	stPackRoiInfo.width = (115.0) / DEMO_SRC_WIDTH;
	stPackRoiInfo.height = 145.0 / DEMO_SRC_HEIGHT;

	s32Ret = Ppm_HalSetMatchPackRoi(&stPackRoiInfo);
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalSetMatchPackRoi failed!");
#endif

#if 0  /* not support */
    /* 配置XSI_MATCH */
    MTME_XSI_MATCH_PARM_T stXsiPrm = {0};

    stXsiPrm.band_speed = 0.5;   /* m/s */
    stXsiPrm.center_dis = 2.5;   /* m */
    s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfSetConfig(pstSubModPrm->pHandle, MTME_MATCH_XSI_NODE_ID, MTME_SET_XSI_MATCH_PARAM, &stXsiPrm, sizeof(stXsiPrm));
    PPM_HAL_CHECK_RET(s32Ret, err, "ICF_Set_config MTME_SET_XSI_MATCH_PARAM failed!");
#endif

#if 1  /* 配置FACE MATCH阈值 */
    float fIouThrs = 0.20;

	s32Ret = Ppm_HalSetMatchFaceIouThresh(fIouThrs);
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalSetMatchFaceIouThresh failed!");
#endif

#if 1  /* 配置人脸抓拍区域 */
    MTME_RECT_T stFaceRoiInfo = {0};

    stFaceRoiInfo.x = (500.0) / DEMO_FACE_WIDTH;
    stFaceRoiInfo.y = (332.0) / DEMO_FACE_HEIGHT;
    stFaceRoiInfo.width = (936.0) / DEMO_FACE_WIDTH;
    stFaceRoiInfo.height = (693.0) / DEMO_FACE_HEIGHT;

	s32Ret = Ppm_HalSetMatchFaceRoi(&stFaceRoiInfo);
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalSetMatchFaceRoi failed!");
#endif

#if 1  /* 人脸匹配参数 */
    UINT32 i = 0;
    PPM_MATCH_MATRIX_INFO_S stMatchMatrixInfo = {0};

    stMatchMatrixInfo.fCamHeight = fCamHeight;

    /* 人脸相机内参 */
    stMatchMatrixInfo.stFaceInterMatrixInfo.uiRowCnt = 3;
    stMatchMatrixInfo.stFaceInterMatrixInfo.uiColCnt = 3;
    for (i = 0; i < 3; i++)
    {
        memcpy(stMatchMatrixInfo.stFaceInterMatrixInfo.fMatrix[i],
               face_matrix[i], sizeof(double) * 3);
    }

    /* 人脸相机畸变系数 */
    stMatchMatrixInfo.stFaceDistMatrixInfo.uiRowCnt = 1;
    stMatchMatrixInfo.stFaceDistMatrixInfo.uiColCnt = 8;
    memcpy(stMatchMatrixInfo.stFaceDistMatrixInfo.fMatrix[0], face_Distmatrix, sizeof(double) * 8);

    /* 三目相机内参逆矩阵 */
    stMatchMatrixInfo.stTriCamInvMatrixInfo.uiRowCnt = 3;
    stMatchMatrixInfo.stTriCamInvMatrixInfo.uiColCnt = 3;
    for (i = 0; i < 3; i++)
    {
        memcpy(stMatchMatrixInfo.stTriCamInvMatrixInfo.fMatrix[i],
               tri_matrix_inv[i], sizeof(double) * 3);
    }

    /* R矩阵 */
    stMatchMatrixInfo.stR_MatrixInfo.uiRowCnt = 3;
    stMatchMatrixInfo.stR_MatrixInfo.uiColCnt = 3;
    for (i = 0; i < 3; i++)
    {
        memcpy(stMatchMatrixInfo.stR_MatrixInfo.fMatrix[i],
               r_p2p[i], sizeof(double) * 3);
    }

    /* T矩阵 */
    stMatchMatrixInfo.stT_MatrixInfo.uiRowCnt = 1;
    stMatchMatrixInfo.stT_MatrixInfo.uiColCnt = 3;
    memcpy(stMatchMatrixInfo.stT_MatrixInfo.fMatrix[0], t_p2p, sizeof(double) * 3);

    s32Ret = Ppm_HalSetMatchMatrixPrm(PPM_SUB_MOD_MATCH, &stMatchMatrixInfo);
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalSetFaceMatchPrm failed!");
#endif

#if 1 /* DFR_SCORE，not support */
    float fFilterScore = 0.15;

	s32Ret = Ppm_HalSetMatchFaceScoreFilter(fFilterScore);
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalSetMatchFaceScoreFilter failed!");
#endif

#if 0  /* 相机矫正参数 */
    s32Ret = Ppm_HalSetCalibPrm(chan);
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalSetCalibPrm failed!");
#endif

#if 0  /* 目标跟踪规则区域，not support */
    s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetConfig(pstSubModPrm->pHandle, MTME_BPC_NODE_ID, MTME_GET_BPC_MAX_RULE_REGION, &stMaxRegion, sizeof(stMaxRegion));
    PPM_HAL_CHECK_RET(s32Ret, err, "ICF_Get_config MTME_GET_BPC_MAX_RULE_REGION failed!");

    for (i = 0; i < stMaxRegion.vertex_num; i++)
    {
        SAL_INFO("i:%d, x:%.3f, y:%.3f \n", i, stMaxRegion.point[i].x, stMaxRegion.point[i].y);
    }

    s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfSetConfig(pstSubModPrm->pHandle, MTME_BPC_NODE_ID, MTME_SET_BPC_RULE_REGION, &stMaxRegion, sizeof(stMaxRegion));
    PPM_HAL_CHECK_RET(s32Ret, err, "ICF_Set_config MTME_SET_BPC_RULE_REGION failed!");
#endif

#if 1  /* 场景抓拍AB区域参数 */
    MTME_CAPTURE_REGION_T stCapRegionPrm = {0};

    /* Capture A区域 */
    stCapRegionPrm.region_a.vertex_num = 4;
    stCapRegionPrm.region_a.point[0].x = 562.0 / DEMO_SRC_WIDTH;
    stCapRegionPrm.region_a.point[0].y = 291.0 / DEMO_SRC_HEIGHT;   /* 168->50贴合实际跨线区域，避免未计入到B区域不触发抓拍 */
    stCapRegionPrm.region_a.point[1].x = 745.0 / DEMO_SRC_WIDTH;
    stCapRegionPrm.region_a.point[1].y = 291.0 / DEMO_SRC_HEIGHT;
    stCapRegionPrm.region_a.point[2].x = 562.0 / DEMO_SRC_WIDTH;
    stCapRegionPrm.region_a.point[2].y = 550.0 / DEMO_SRC_HEIGHT;
    stCapRegionPrm.region_a.point[3].x = 745.0 / DEMO_SRC_WIDTH;
    stCapRegionPrm.region_a.point[3].y = 550.0 / DEMO_SRC_HEIGHT;

    /* Capture B区域 */
    stCapRegionPrm.region_b.vertex_num = 4;
    stCapRegionPrm.region_b.point[0].x = 383.0 / DEMO_SRC_WIDTH;  /* 530->460 */
    stCapRegionPrm.region_b.point[0].y = 291.0 / DEMO_SRC_HEIGHT;
    stCapRegionPrm.region_b.point[1].x = 562.0 / DEMO_SRC_WIDTH;
    stCapRegionPrm.region_b.point[1].y = 291.0 / DEMO_SRC_HEIGHT;
    stCapRegionPrm.region_b.point[2].x = 383.0 / DEMO_SRC_WIDTH;
    stCapRegionPrm.region_b.point[2].y = 550.0 / DEMO_SRC_HEIGHT;
    stCapRegionPrm.region_b.point[3].x = 562.0 / DEMO_SRC_WIDTH;
    stCapRegionPrm.region_b.point[3].y = 550.0 / DEMO_SRC_HEIGHT;

    s32Ret = Ppm_HalSetCapRegion(&stCapRegionPrm);
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalSetCapRegionPrm failed!");
#endif

    SAL_INFO("Ppm_HalSetDefaultCfg end!\n");
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_HalIcfDeinit
 * @brief:      引擎资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_HalIcfDeinit(VOID)
{
    if (!g_stPpmModPrm.stInitHandle.pInitHandle)
    {
        SAL_ERROR("Ppm Module is Deinitialized! \n");
        return SAL_SOK;
    }

    (VOID)g_stPpmModPrm.stIcfFuncP.Ppm_IcfFinit(&g_stPpmModPrm.stInitHandle, sizeof(ICF_INIT_HANDLE));

	memset(&g_stPpmModPrm.stInitHandle, 0x00, sizeof(ICF_INIT_HANDLE));

    SAL_INFO("ICF_Finit success \n");
    return SAL_SOK;
}

/**
 * @function:   Ppm_HalGetIcfVersion
 * @brief:      获取icf版本号
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_HalGetIcfVersion(VOID)
{
#if 0  /* szl_ppm_todo: 版本号获取，待引擎release版本集成后开放 */
    INT32 s32Ret = SAL_SOK;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;
    ICF_VERSION stIcfVersion = {0};

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_RET(NULL == pstPpmGlobalPrm, err, "global prm is not initialized!");

    /* ICF引擎框架版本号 */
    s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetVersion(&stIcfVersion, sizeof(ICF_VERSION));
    PPM_HAL_CHECK_RET(SAL_SOK != s32Ret, err, "get icf version failed!");

    snprintf(pstPpmGlobalPrm->strIcfVer,
             PPM_MAX_ICF_VERSION_LEN,
             "ICF Version: V_%d.%d.%d build %d-%d-%d",
             stIcfVersion.nMajorVersion,
             stIcfVersion.nSubVersion,
             stIcfVersion.nRevisVersion,
             stIcfVersion.nVersionYear,
             stIcfVersion.nVersionMonth,
             stIcfVersion.nVersionDay);
    SAL_INFO("ICF version: %d.%d.%d, data: %d.%d.%d\n", stIcfVersion.nMajorVersion,
             stIcfVersion.nSubVersion,
             stIcfVersion.nRevisVersion,
             stIcfVersion.nVersionYear,
             stIcfVersion.nVersionMonth,
             stIcfVersion.nVersionDay);

    return SAL_SOK;
err:
    return SAL_FAIL;
#else
	PPM_LOGW("not support! return success! \n");
	return SAL_SOK;
#endif
}

/**
 * @function:   Ppm_HalIcfInit
 * @brief:      引擎资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_HalIcfInit(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;
    ICF_INIT_PARAM stInitParam = {0};

    stInitParam.stConfigInfo.pAlgCfgPath = "./mtme/config/AlgCfgTest.json";
    stInitParam.stConfigInfo.pTaskCfgPath = "./mtme/config/TASK.json";
    stInitParam.stConfigInfo.pToolkitCfgPath = "./mtme/config/ToolkitCfg.json";

    stInitParam.stMemConfig.pMemSytemMallocInter = (void *)Ppm_HalMemPoolSystemAllocCallback;
    stInitParam.stMemConfig.pMemSytemFreeInter = (void *)Ppm_HalMemPoolSystemFreeCallback;

    stInitParam.stConfigInfo.bAlgCfgEncry      = 0;
    stInitParam.stConfigInfo.bTaskCfgEncry     = 0;
    stInitParam.stConfigInfo.bToolkitCfgEncry  = 0;

    stInitParam.pScheduler = IA_GetScheHndl();     /* 传入调度器句柄 */

    s32Ret = Ppm_HalGetIcfVersion();
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalGetIcfVersion failed!");

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstPpmGlobalPrm, err, "pstPpmGlobalPrm == null!");

    s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfInit(&stInitParam, 
		                                            sizeof(ICF_INIT_PARAM),
		                                            &pstPpmGlobalPrm->stInitHandle,
		                                            sizeof(ICF_INIT_HANDLE));
    PPM_HAL_CHECK_RET(s32Ret, err, "ppm icf init failed!");

    PPM_LOGI("ICF_Init success, init handle %p \n", pstPpmGlobalPrm->stInitHandle.pInitHandle);
    return SAL_SOK;

err:
	if (pstPpmGlobalPrm && pstPpmGlobalPrm->stInitHandle.pInitHandle)
	{
    	(VOID)pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfFinit(&pstPpmGlobalPrm->stInitHandle, sizeof(ICF_INIT_HANDLE));
	}
    return SAL_FAIL;
}

/**
 * @function   Ppm_HalRegCbFuncToEngine
 * @brief      向引擎通道注册结果回调函数
 * @param[in]  ICF_CREATE_HANDLE *pstCreateChn  
 * @param[in]  ICF_CALLBACK_PARAM *pstCbPrm     
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalRegCbFuncToEngine(ICF_CREATE_HANDLE *pstCreateChn, ICF_CALLBACK_PARAM *pstCbPrm)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;

    PPM_HAL_CHECK_PTR(pstCreateChn, exit, "pstCreateChn == null!");
    PPM_HAL_CHECK_PTR(pstCbPrm, exit, "pstCbPrm == null!");

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstPpmGlobalPrm, exit, "pstPpmGlobalPrm == null!");

    s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfSetCallback(pstCreateChn->pChannelHandle,
		                                                   pstCbPrm,
		                                                   sizeof(ICF_CALLBACK_PARAM));
    PPM_HAL_CHECK_RET(s32Ret, exit, "ICF_Set_callback failed!");

    SAL_INFO("ICF_Set_callback success \n");

exit:
    return s32Ret;
}

/**
 * @function:   Ppm_HalPrModelMemInfo
 * @brief:      打印模型使用的内存信息
 * @param[in]:  ICF_MODEL_HANDLE *pstModelHandleInfo
 * @param[out]: None
 * @return:     static INT32
 */
static VOID Ppm_HalPrModelMemInfo(ICF_MODEL_HANDLE *pstModelHandleInfo)
{
    UINT32 i;

    for (i = 0; i < ICF_MEM_TYPE_NUM; ++i)
    {
        if (pstModelHandleInfo->modelMemSize.stNonSharedMemInfo[i].nMemSize <= 0)
        {
            continue;
        }

        PPM_LOGI("stmodel_NonSharedMemInfo[%d].nMemSize = %d B %f MB type %d\n", i,
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

        PPM_LOGI("stmodel_SharedMemInfo[%d].nMemSize = %d B %f MB type %d\n", i,
                 (INT32)pstModelHandleInfo->modelMemSize.stSharedMemInfo[i].nMemSize,
                 (float)pstModelHandleInfo->modelMemSize.stSharedMemInfo[i].nMemSize / 1024.0 / 1024.0,
                 pstModelHandleInfo->modelMemSize.stSharedMemInfo[i].eMemType);
    }
}

/**
 * @function   Ppm_HalUnloadModel
 * @brief      卸载模型
 * @param[in]  ICF_MODEL_HANDLE *pstModelHandle  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalUnloadModel(ICF_MODEL_HANDLE *pstModelHandle)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;

    PPM_HAL_CHECK_PTR(pstModelHandle, exit, "pstModelHandle == null!");

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstPpmGlobalPrm, exit, "pstPpmGlobalPrm == null!");

	if (!pstModelHandle->pModelHandle)
	{
		PPM_LOGW("model handle is destroyed! return success! \n");

		s32Ret = SAL_SOK;
		goto exit;
	}
	
	s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfUnloadModel(pstModelHandle, 
		                                                   sizeof(ICF_MODEL_HANDLE));
	if (SAL_SOK != s32Ret)
	{
		PPM_LOGE("ppm icf unload model failed! r: 0x%x \n", s32Ret);
		goto exit;
	}

	memset(pstModelHandle, 0x00, sizeof(ICF_MODEL_HANDLE));
	
	s32Ret = SAL_SOK;
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalLoadModel
 * @brief      加载模型
 * @param[in]  ICF_MODEL_PARAM *pstModelParm     
 * @param[in]  ICF_MODEL_HANDLE *pstModelHandle  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalLoadModel(ICF_MODEL_PARAM *pstModelParm, ICF_MODEL_HANDLE *pstModelHandle)
{
    INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;

    PPM_HAL_CHECK_PTR(pstModelParm, exit, "pstModelParm == null!");
    PPM_HAL_CHECK_PTR(pstModelHandle, exit, "pstModelHandle == null!");

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstPpmGlobalPrm, exit, "pstPpmGlobalPrm == null!");
	
	s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfLoadModel(pstModelParm, 
		                                                 sizeof(ICF_MODEL_PARAM), 
		                                                 pstModelHandle, 
		                                                 sizeof(ICF_MODEL_HANDLE));
	if (SAL_SOK != s32Ret)
	{
		PPM_LOGE("ICF_LoadModel_V2 MAIN error\n");
		goto exit;
	}
	
	/* 打印模型内存 */
	Ppm_HalPrModelMemInfo(pstModelHandle);
	
	s32Ret = SAL_SOK;
exit:
	return s32Ret;
}

/**
 * @function:   Ppm_HalPrEngChnMemInfo
 * @brief:      打印引擎运行使用的内存信息
 * @param[in]:  ICF_MODEL_HANDLE *pstModelHandleInfo
 * @param[out]: None
 * @return:     static INT32
 */
static VOID Ppm_HalPrEngChnMemInfo(ICF_CREATE_HANDLE *pstEngChnHandleInfo)
{
    UINT32 i;

    for (i = 0; i < ICF_MEM_TYPE_NUM; ++i)
    {
        if (pstEngChnHandleInfo->createMemSize.stNonSharedMemInfo[i].nMemSize <= 0)
        {
            continue;
        }

        PPM_LOGI("stalg_NonSharedMemInfo[%d].nMemSize = %d B %f MB type %d\n", i,
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

        PPM_LOGI("stalg_SharedMemInfo[%d].nMemSize = %d B %f MB type %d\n", i,
                 (INT32)pstEngChnHandleInfo->createMemSize.stSharedMemInfo[i].nMemSize,
                 (float)pstEngChnHandleInfo->createMemSize.stSharedMemInfo[i].nMemSize / 1024.0 / 1024.0,
                 pstEngChnHandleInfo->createMemSize.stSharedMemInfo[i].eMemType);
    }
}

/**
 * @function   Ppm_HalDestroyEngHandle
 * @brief      销毁引擎通道句柄
 * @param[in]  ICF_CREATE_HANDLE *pstCreateHandle  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalDestroyEngHandle(ICF_CREATE_HANDLE *pstCreateHandle)
{
	INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;

    PPM_HAL_CHECK_PTR(pstCreateHandle, exit, "pstCreateHandle == null!");

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstPpmGlobalPrm, exit, "pstPpmGlobalPrm == null!");

	if (!pstCreateHandle->pChannelHandle)
	{
		PPM_LOGW("ppm engine channel is destroyed! return success! \n");
		return SAL_SOK;
	}

    s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfDestroy(pstCreateHandle, sizeof(ICF_CREATE_HANDLE));
    if (SAL_SOK != s32Ret)
    {
        PPM_LOGE("ppm icf destroy failed! r: 0x%x \n", s32Ret);
        goto exit;
    }

	memset(pstCreateHandle, 0x00, sizeof(ICF_CREATE_HANDLE));
	
	s32Ret = SAL_SOK;
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalCreateEngHandle
 * @brief      创建引擎通道句柄
 * @param[in]  ICF_MODEL_PARAM *pstModelParm       
 * @param[in]  ICF_MODEL_HANDLE *pstModelHandle    
 * @param[in]  ICF_CREATE_HANDLE *pstCreateHandle  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalCreateEngHandle(IN ICF_MODEL_PARAM *pstModelParm, 
                                    IN ICF_MODEL_HANDLE *pstModelHandle,
                                    OUT ICF_CREATE_HANDLE *pstCreateHandle)
{
	INT32 s32Ret = SAL_FAIL;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;

    ICF_CREATE_PARAM stCreateParam = {0};

    PPM_HAL_CHECK_PTR(pstModelParm, exit, "pstModelParm == null!");
    PPM_HAL_CHECK_PTR(pstModelHandle, exit, "pstModelHandle == null!");
    PPM_HAL_CHECK_PTR(pstCreateHandle, exit, "pstCreateHandle == null!");

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstPpmGlobalPrm, exit, "pstPpmGlobalPrm == null!");

    memcpy(&stCreateParam.modelParam,  pstModelParm,  sizeof(ICF_MODEL_PARAM));
    memcpy(&stCreateParam.modelHandle, pstModelHandle, sizeof(ICF_MODEL_HANDLE));

    s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfCreate(&stCreateParam, 
		                                              sizeof(ICF_CREATE_PARAM),
		                                              pstCreateHandle, 
		                                              sizeof(ICF_CREATE_HANDLE));
    if (SAL_SOK != s32Ret)
    {
        PPM_LOGE("ICF_Create_V2 error\n");
        goto exit;
    }
	
	/* 打印引擎通道内存信息 */
    Ppm_HalPrEngChnMemInfo(pstCreateHandle);
	
	s32Ret = SAL_SOK;
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalInitEngFaceCalibChan
 * @brief      初始化引擎通道-人脸单目相机标定
 * @param[in]  VOID  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalInitEngFaceCalibChan(VOID)
{
	INT32 s32Ret = SAL_FAIL;

	PPM_SUB_MOD_S *pstSubModPrm = NULL;
	PPM_SUB_MOD_FACE_CALIB_PRM_S *pstFaceCalibChnPrm = NULL;

	pstSubModPrm = Ppm_HalGetSubModPrm(PPM_SUB_MOD_FACE_CALIB);
    PPM_HAL_CHECK_PTR(pstSubModPrm, exit, "pstSubModPrm == null!");

	/* 获取关联匹配通道的全局参数 */
	pstFaceCalibChnPrm = (PPM_SUB_MOD_FACE_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_FACE_CALIB);
    PPM_HAL_CHECK_PTR(pstFaceCalibChnPrm, exit, "pstFaceCalibChnPrm == null!");

	/* 加载模型 */
	{			
		pstFaceCalibChnPrm->stModelParam.nGraphID = PPM_FACE_CALIB_GRAPH_ID;
		pstFaceCalibChnPrm->stModelParam.pAppParam = NULL;
		pstFaceCalibChnPrm->stModelParam.nMaxCacheNums = 0;
		pstFaceCalibChnPrm->stModelParam.pInitHandle = g_stPpmModPrm.stInitHandle.pInitHandle;

		s32Ret = Ppm_HalLoadModel(&pstFaceCalibChnPrm->stModelParam, &pstFaceCalibChnPrm->stModelHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalLoadModel failed!");
	}
	
	/* 创建通道句柄 */
	{
		s32Ret = Ppm_HalCreateEngHandle(&pstFaceCalibChnPrm->stModelParam, 
		                                &pstFaceCalibChnPrm->stModelHandle,
		                                &pstFaceCalibChnPrm->stCreateHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalCreateEngHandle failed!");
	}

	/* 注册异步回调处理接口 */
	{
		ICF_CALLBACK_PARAM stCallbackParam = {0};
		
		/* 人脸单目相机标定回调注册 */
		{
			stCallbackParam.nNodeID 	  = MTME_SVCABLIC_POST_NODE_ID;
			stCallbackParam.nCallbackType = ICF_CALLBACK_OUTPUT;
			stCallbackParam.pCallbackFunc = (void *)Ppm_DrvFaceCalibCbFunc;
			stCallbackParam.nUserSize	  = 0;
			stCallbackParam.pUsr		  = NULL;
			
			s32Ret = Ppm_HalRegCbFuncToEngine(&pstFaceCalibChnPrm->stCreateHandle, 
											  &stCallbackParam);
			PPM_HAL_CHECK_RET(s32Ret, exit, "reg face calib cb failed!");
		}
	}

	/* 子模块使能 */
	pstSubModPrm->uiUseFlag = SAL_TRUE;

	PPM_LOGI("init face calib channel end! \n");
	s32Ret = SAL_SOK;
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalDeinitEngFaceCalibChan
 * @brief      去初始化引擎通道-人脸单目相机标定
 * @param[in]  VOID  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalDeinitEngFaceCalibChan(VOID)
{
	INT32 s32Ret = SAL_FAIL;

	PPM_SUB_MOD_FACE_CALIB_PRM_S *pstFaceCalibChnPrm = NULL;

	/* 获取关联匹配通道的全局参数 */
	pstFaceCalibChnPrm = (PPM_SUB_MOD_FACE_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_FACE_CALIB);
    PPM_HAL_CHECK_PTR(pstFaceCalibChnPrm, exit, "pstMccCalibChnPrm == null!");
	
	/* 销毁通道句柄 */
	{
		s32Ret = Ppm_HalDestroyEngHandle(&pstFaceCalibChnPrm->stCreateHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalDestroyEngHandle failed!");
	}

	/* 卸载模型 */
	{			
		s32Ret = Ppm_HalUnloadModel(&pstFaceCalibChnPrm->stModelHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalLoadModel failed!");
	}

	PPM_LOGI("deinit face calib channel end! \n");
	s32Ret = SAL_SOK;
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalInitEngMccCalibChan
 * @brief      初始化引擎通道-跨相机标定
 * @param[in]  VOID 
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalInitEngMccCalibChan(VOID)
{
	INT32 s32Ret = SAL_FAIL;

	PPM_SUB_MOD_S *pstSubModPrm = NULL;
	PPM_SUB_MOD_MCC_CALIB_PRM_S *pstMccCalibChnPrm = NULL;

	pstSubModPrm = Ppm_HalGetSubModPrm(PPM_SUB_MOD_MCC_CALIB);
    PPM_HAL_CHECK_PTR(pstSubModPrm, exit, "pstSubModPrm == null!");

	/* 获取关联匹配通道的全局参数 */
	pstMccCalibChnPrm = (PPM_SUB_MOD_MCC_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MCC_CALIB);
    PPM_HAL_CHECK_PTR(pstMccCalibChnPrm, exit, "pstMccCalibChnPrm == null!");

	/* 加载模型 */
	{			
		pstMccCalibChnPrm->stModelParam.nGraphID = PPM_MCC_CALIB_GRAPH_ID;
		pstMccCalibChnPrm->stModelParam.pAppParam = NULL;
		pstMccCalibChnPrm->stModelParam.nMaxCacheNums = 0;
		pstMccCalibChnPrm->stModelParam.pInitHandle = g_stPpmModPrm.stInitHandle.pInitHandle;

		s32Ret = Ppm_HalLoadModel(&pstMccCalibChnPrm->stModelParam, &pstMccCalibChnPrm->stModelHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalLoadModel failed!");
	}
	
	/* 创建通道句柄 */
	{
		s32Ret = Ppm_HalCreateEngHandle(&pstMccCalibChnPrm->stModelParam, 
		                                &pstMccCalibChnPrm->stModelHandle,
		                                &pstMccCalibChnPrm->stCreateHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalCreateEngHandle failed!");
	}

	/* 注册异步回调处理接口 */
	{
		ICF_CALLBACK_PARAM stCallbackParam = {0};
		
		/* 跨相机标定回调注册 */
		{
			stCallbackParam.nNodeID       = MTME_CABLIC_POST_NODE_ID;
		    stCallbackParam.nCallbackType = ICF_CALLBACK_OUTPUT;
		    stCallbackParam.pCallbackFunc = (void *)Ppm_DrvMccCalibCbFunc;
		    stCallbackParam.nUserSize     = 0;
		    stCallbackParam.pUsr          = NULL;
			
			s32Ret = Ppm_HalRegCbFuncToEngine(&pstMccCalibChnPrm->stCreateHandle, 
				                              &stCallbackParam);
			PPM_HAL_CHECK_RET(s32Ret, exit, "reg mcc calib cb failed!");
		}
	}

	/* 子模块使能 */
	pstSubModPrm->uiUseFlag = SAL_TRUE;

	PPM_LOGI("init mcc calib channel end! \n");
	s32Ret = SAL_SOK;
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalDeinitEngMccCalibChan
 * @brief      去初始化引擎通道-跨相机标定
 * @param[in]  VOID  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalDeinitEngMccCalibChan(VOID)
{
	INT32 s32Ret = SAL_FAIL;

	PPM_SUB_MOD_MCC_CALIB_PRM_S *pstMccCalibChnPrm = NULL;

	/* 获取关联匹配通道的全局参数 */
	pstMccCalibChnPrm = (PPM_SUB_MOD_MCC_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MCC_CALIB);
    PPM_HAL_CHECK_PTR(pstMccCalibChnPrm, exit, "pstMccCalibChnPrm == null!");
	
	/* 销毁通道句柄 */
	{
		s32Ret = Ppm_HalDestroyEngHandle(&pstMccCalibChnPrm->stCreateHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalDestroyEngHandle failed!");
	}

	/* 卸载模型 */
	{			
		s32Ret = Ppm_HalUnloadModel(&pstMccCalibChnPrm->stModelHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalLoadModel failed!");
	}

	PPM_LOGI("deinit mcc channel end! \n");
	s32Ret = SAL_SOK;
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalInitEngMatchChan
 * @brief      初始化引擎通道-匹配
 * @param[in]  VOID  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalInitEngMatchChan(VOID)
{
	INT32 s32Ret = SAL_FAIL;

	PPM_SUB_MOD_S *pstSubModPrm = NULL;
	PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;

	pstSubModPrm = Ppm_HalGetSubModPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstSubModPrm, exit, "pstSubModPrm == null!");

	/* 获取关联匹配通道的全局参数 */
	pstMatchChnPrm = (PPM_SUB_MOD_MATCH_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstMatchChnPrm, exit, "pstMatchChnPrm == null!");

	/* 加载模型 */
	{
		ICF_APP_PARAM_INFO stAppParam[2] = {0};
			
		pstMatchChnPrm->stModelParam.nGraphID = PPM_MATCH_GRAPH_ID;
		pstMatchChnPrm->stModelParam.nGraphType = 0;
		pstMatchChnPrm->stModelParam.pAppParam = &stAppParam[0];
		pstMatchChnPrm->stModelParam.nMaxCacheNums = 0;
		pstMatchChnPrm->stModelParam.pInitHandle = g_stPpmModPrm.stInitHandle.pInitHandle;

		s32Ret = Ppm_HalLoadModel(&pstMatchChnPrm->stModelParam, &pstMatchChnPrm->stModelHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalLoadModel failed!");
	}
	
	/* 创建通道句柄 */
	{
		s32Ret = Ppm_HalCreateEngHandle(&pstMatchChnPrm->stModelParam, 
		                                &pstMatchChnPrm->stModelHandle,
		                                &pstMatchChnPrm->stCreateHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalCreateEngHandle failed!");
	}

	/* 注册异步回调处理接口 */
	{
		ICF_CALLBACK_PARAM stCallbackParam = {0};
		
		/* 可见光包裹回调注册 */
		{
			stCallbackParam.nNodeID       = MTME_MATCH_PACK_NODE_ID;
		    stCallbackParam.nCallbackType = ICF_CALLBACK_OUTPUT;
		    stCallbackParam.pCallbackFunc = (VOID *)Ppm_DrvMatchPackRsltCbFunc;
		    stCallbackParam.nUserSize     = 0;
		    stCallbackParam.pUsr          = NULL;
			
			s32Ret = Ppm_HalRegCbFuncToEngine(&pstMatchChnPrm->stCreateHandle, 
				                              &stCallbackParam);
			PPM_HAL_CHECK_RET(s32Ret, exit, "reg pack cb failed!");
		}

		/* 人脸抓拍回调注册 */
		{
			stCallbackParam.nNodeID		= MTME_MATCH_FACE_NODE_ID;
			stCallbackParam.nCallbackType = ICF_CALLBACK_OUTPUT;
			stCallbackParam.pCallbackFunc = (void *)Ppm_DrvMatchFaceRsltCbFunc;
			stCallbackParam.nUserSize 	= 0;
			stCallbackParam.pUsr			= NULL;
			
			s32Ret = Ppm_HalRegCbFuncToEngine(&pstMatchChnPrm->stCreateHandle, 
											  &stCallbackParam);
			PPM_HAL_CHECK_RET(s32Ret, exit, "reg face cb failed!");
		}

		/* pos信息回调注册 */
		{
			stCallbackParam.nNodeID		= MTME_POST_NODE_ID;
			stCallbackParam.nCallbackType = ICF_CALLBACK_OUTPUT;
			stCallbackParam.pCallbackFunc = (void *)Ppm_DrvMatchPosRsltCbFunc;
			stCallbackParam.nUserSize 	= 0;
			stCallbackParam.pUsr			= NULL;
			
			s32Ret = Ppm_HalRegCbFuncToEngine(&pstMatchChnPrm->stCreateHandle, 
											  &stCallbackParam);
			PPM_HAL_CHECK_RET(s32Ret, exit, "reg pos cb failed!");
		}
	}

	/* 子模块使能 */
	pstSubModPrm->uiUseFlag = SAL_TRUE;

	/* 设置通道默认参数 */
	{
		s32Ret = Ppm_HalSetMatchChnDefaultCfg();
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalSetDefaultCfg failed!");
	}

	PPM_LOGI("init match channel end! \n");
	s32Ret = SAL_SOK;
exit:
	return s32Ret;
}

/**
 * @function   Ppm_HalDeinitEngMatchChan
 * @brief      去初始化引擎通道-匹配
 * @param[in]  VOID  
 * @param[out] None
 * @return     static INT32
 */
static INT32 Ppm_HalDeinitEngMatchChan(VOID)
{
	INT32 s32Ret = SAL_FAIL;

	PPM_SUB_MOD_MATCH_PRM_S *pstMatchChnPrm = NULL;

	/* 获取关联匹配通道的全局参数 */
	pstMatchChnPrm = (PPM_SUB_MOD_MATCH_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MATCH);
    PPM_HAL_CHECK_PTR(pstMatchChnPrm, exit, "pstMatchChnPrm == null!");
	
	/* 销毁通道句柄 */
	{
		s32Ret = Ppm_HalDestroyEngHandle(&pstMatchChnPrm->stCreateHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalDestroyEngHandle failed!");
	}

	/* 卸载模型 */
	{			
		s32Ret = Ppm_HalUnloadModel(&pstMatchChnPrm->stModelHandle);
		PPM_HAL_CHECK_RET(s32Ret, exit, "Ppm_HalLoadModel failed!");
	}

	PPM_LOGI("deinit match channel end! \n");
	s32Ret = SAL_SOK;
exit:
	return s32Ret;
}

/*******************************************************************************
* 函数名  : Ppm_HalWriteJsonFile
* 描  述  : 写Json文件
* 输  入  : pFile : Json路径
*            : pData : 需要写入的内容
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static INT32 Ppm_HalWriteJsonFile(char *pFile, char *pData)
{
    FILE *fp = NULL;
    INT32 ret = SAL_SOK;

    /*check input parameter*/
    if (NULL == pData || NULL == pFile)
    {
        PPM_LOGE("Input Parameter Error!! \n");
        return SAL_FAIL;
    }

    /*open	file*/
    fp = fopen(pFile, "w");
    if (fp == NULL)
    {
        PPM_LOGE("Write Json File ERROR !!filename is %s\n", pFile);
        return SAL_FAIL;
    }

    /*write pData to the fp file*/
    fprintf(fp, "%s", pData);

    if (fp != NULL)
    {
        fclose(fp);
        fp = NULL;
    }

    return ret;
}

/*******************************************************************************
* 函数名  : PPM_HalGetCjsonFile
* 描  述  : 获取现有Json文件字符串
* 输  入  : pFileName : Json路径
*
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
static cJSON *Ppm_HalGetCjsonFile(char *pFileName)
{
    FILE *f = NULL;
    long len = 0;
    char *data = NULL;
    cJSON *json = NULL, *ret = NULL;
    int result = 0;

    /*check the filename*/
    if (NULL == pFileName)
    {
        PPM_LOGE("Cjson file is  NULL! \n");
        return NULL;
    }

    /*open file*/
    f = fopen(pFileName, "rb");
    if (NULL == f)
    {
        PPM_LOGE("fopen file %s fail!\n", pFileName);
        return NULL;
    }

    /*file seek_end*/
    if (0 != fseek(f, 0, SEEK_END))
    {
        PPM_LOGE("fseek failed! \n");
        if (NULL != f)
        {
            fclose(f);
            f = NULL;
        }

        return NULL;
    }

    /*Get file length*/
    len = ftell(f);
    if (len <= 0)
    {
        PPM_LOGE("file data is null! \n");
        ret = NULL;
        goto EXIT;
    }

    /*file seek_set*/
    if (0 != fseek(f, 0, SEEK_SET))
    {
        PPM_LOGE("fseek failed! \n");
        ret = NULL;
        goto EXIT;
    }

    /*malloc*/
    data = (char *)SAL_memMalloc(len + 1, "PPM", "ppm_json");
    if (NULL == data)
    {
        PPM_LOGE("Data Malloc ERROR! \n");
        ret = NULL;
        goto EXIT;
    }

    /*read file to data*/
    result = fread(data, 1, len, f);
    if (result == 0 || feof(f))
    {
        PPM_LOGE("fread err! \n");
        ret = NULL;
        goto EXIT;
    }

    data[len] = '\0';

    /*use cJSON interface to change the data(char *) to json object*/
    json = cJSON_Parse(data);
    if (!json)
    {
        PPM_LOGE("Error before: [%s]\n", cJSON_GetErrorPtr());
        ret = NULL;
        goto EXIT;
    }
    else
    {
        ret = json;
    }

EXIT:
    if (NULL != data)
    {
        SAL_memfree(data, "PPM", "ppm_json");
        data = NULL;
    }

    if (NULL != f)
    {
        fclose(f);
        f = NULL;
    }

    return ret;
}

/**
 * @function:   Sva_HalGetJsonKeyStr
 * @brief:      获取JSON键值名称
 * @param[in]:  SVA_JSON_MOD_PRM_S *pstSvaJsonPrm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_HalGetJsonKeyStr(PPM_JSON_MOD_PRM_S *pstPpmJsonPrm)
{
    CHAR *pcString = NULL;

    PPM_HAL_CHECK_PTR(pstPpmJsonPrm, exit, "pstPpmJsonPrm error ");

    switch (pstPpmJsonPrm->enClassType)
    {
        case PPM_JSON_FIRST_CLASS:
            if (pstPpmJsonPrm->uiClassId > PPM_JSON_CLASS_NUM - 1)
            {
                PPM_LOGE("Invalid Main Class Id %d \n", pstPpmJsonPrm->uiClassId);
                goto exit;
            }

            pcString = g_PpmjSonMainClassKeyTab[pstPpmJsonPrm->uiClassId];
            break;
        case PPM_JSON_SECOND_CLASS:
            if (pstPpmJsonPrm->uiClassId > PPM_JSON_SUB_CLASS_NUM - 1)
            {
                PPM_LOGE("Invalid Sub Class Id %d \n", pstPpmJsonPrm->uiClassId);
                goto exit;
            }

            pcString = g_PpmjSonSubClassKeyTab[pstPpmJsonPrm->uiClassId];
            break;
        default:
            SVA_LOGW("Invalid Main Class Id %d \n", pstPpmJsonPrm->uiClassId);
            break;
    }

    pstPpmJsonPrm->pcVal = pcString;

exit:
    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : PPM_HalModifyCfgFile
* 描  述  : 修改配置文件中特定配置项
* 输  入  : pUpdataJson     需要修改的json内容结构体
*
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 Ppm_HalModifyCfgFile(PPM_UPDATA_CFG_JSON_PARA *pstUpJsonPrm)
{
    cJSON *pstRootPara = NULL;
    cJSON *pstBasicPara = NULL;
    cJSON *pstObjItem = NULL;
    CHAR *pstOutPara = NULL;

    PPM_JSON_MOD_PRM_S stPpmJsonPrm = {0};

    PPM_HAL_CHECK_PTR(pstUpJsonPrm, err, "pstUpJsonPrm error!");

    /*Get cjson filepath*/
    pstRootPara = Ppm_HalGetCjsonFile(Ppm_HalGetIcfParaJson(PPM_PARAM_CFG_PATH));
    PPM_HAL_CHECK_PTR(pstRootPara, err, "pstRootPara error!");

    /*use cJson interface to get the json object data to the outPara(char *),注意:cJSON_Print函数已经释放root内存，后面不需要重复释放*/
    pstOutPara = cJSON_Print(pstRootPara);
    free(pstOutPara);
    pstOutPara = NULL;

    stPpmJsonPrm.enClassType = PPM_JSON_FIRST_CLASS;
    stPpmJsonPrm.uiClassId = PPM_JSON_CLASS_SECURITY_DETECT;
    (VOID)Ppm_HalGetJsonKeyStr(&stPpmJsonPrm);

    pstBasicPara = cJSON_GetObjectItem(pstRootPara, stPpmJsonPrm.pcVal);
    PPM_HAL_CHECK_PTR(stPpmJsonPrm.pcVal, err, "cJSON_GetObjectItem error!");

    stPpmJsonPrm.enClassType = PPM_JSON_SECOND_CLASS;

    switch (pstUpJsonPrm->enChgType)
    {
        case CHANGE_DPT_PARAM:
            stPpmJsonPrm.uiClassId = PPM_JSON_SUB_CLASS_HEIGHT;
            (VOID)Ppm_HalGetJsonKeyStr(&stPpmJsonPrm);
            PPM_HAL_CHECK_PTR(stPpmJsonPrm.pcVal, err, "Ppm_HalGetJsonKeyStr error!");

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stPpmJsonPrm.pcVal);
            PPM_HAL_CHECK_PTR(pstObjItem, err, "cJSON_GetObjectItem error!");

            pstObjItem->valuedouble = pstUpJsonPrm->fCamHeight;
            PPM_LOGI("modify Height %lf\n", pstObjItem->valuedouble);

            stPpmJsonPrm.uiClassId = PPM_JSON_SUB_CLASS_INCLINE;
            (VOID)Ppm_HalGetJsonKeyStr(&stPpmJsonPrm);
            PPM_HAL_CHECK_PTR(stPpmJsonPrm.pcVal, err, "Ppm_HalGetJsonKeyStr error!");

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stPpmJsonPrm.pcVal);
            PPM_HAL_CHECK_PTR(pstObjItem, err, "cJSON_GetObjectItem error!");

            pstObjItem->valuedouble = pstUpJsonPrm->fInclineAngle;
            PPM_LOGI("pitchAngle %lf\n", pstObjItem->valuedouble);

            stPpmJsonPrm.uiClassId = PPM_JSON_SUB_CLASS_PITCH;
            (VOID)Ppm_HalGetJsonKeyStr(&stPpmJsonPrm);
            PPM_HAL_CHECK_PTR(stPpmJsonPrm.pcVal, err, "Ppm_HalGetJsonKeyStr error!");

            pstObjItem = cJSON_GetObjectItem(pstBasicPara, stPpmJsonPrm.pcVal);
            PPM_HAL_CHECK_PTR(pstObjItem, err, "cJSON_GetObjectItem error!");

            pstObjItem->valuedouble = pstUpJsonPrm->fPitchAngle;
            PPM_LOGI("inclineAngle %lf\n", pstObjItem->valuedouble);

            break;

        /*default*/
        default:
            PPM_LOGE("Change Type ERROR!!! \n");
            break;
    }

    /*copy the root data to the out,注意:cJSON_Print函数已经释放root内存，后面不需要重复释放*/
    pstOutPara = cJSON_Print(pstRootPara);

    /*write the json file*/
    if (Ppm_HalWriteJsonFile(Ppm_HalGetIcfParaJson(PPM_PARAM_CFG_PATH), pstOutPara) != SAL_SOK)
    {
        PPM_LOGE("Write Json  Faile !! \n");
        return SAL_FAIL;
    }

    free(pstOutPara);
    pstOutPara = NULL;

    return SAL_SOK;
err:
    free(pstOutPara);
    pstOutPara = NULL;
    return SAL_FAIL;
}

#if 1 /* 场景限制参数，后续根据现场标定结果使用某种方式实现配置 */

/**
 * @function:   Ppm_HalSetCalibPrm
 * @brief:      设置标定参数
 * @param[in]:  UINT32 chan
 * @param[out]: None
 * @return:     INT32
 */
static INT32 ATTRIBUTE_UNUSED Ppm_HalSetCalibPrm(UINT32 chan)
{
#if 0
    INT32 s32Ret = SAL_SOK;

    FILE *fp_camera_param = NULL;
    PPM_MOD_S *pstPpmGlobalPrm = NULL;
    PPM_SUB_MOD_S *pstSubModPrm = NULL;

    VCA_BV_CALIB_PARAM calib_param = {0};

    PPM_HAL_CHECK_SUB_MOD_ID(chan, SAL_FAIL);

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_PTR(pstPpmGlobalPrm, err, "pstPpmGlobalPrm == null!");

    pstSubModPrm = &pstPpmGlobalPrm->stPpmSubMod[chan];
    PPM_HAL_CHECK_PTR(pstSubModPrm, err, "pstSubModPrm == null!");

    if (!pstSubModPrm->uiUseFlag)
    {
        SAL_ERROR("sub mod %d is not used! \n", chan);
        return SAL_FAIL;
    }

    PPM_HAL_CHECK_PTR(pstSubModPrm->pHandle, err, "pHandle == null!");

    fp_camera_param = fopen(DEMO_BPC_CALIB_PARAM_PATH, "rb");
    if (NULL == fp_camera_param)
    {
        SAL_ERROR("open camera config err\n");
        goto err;
    }

    s32Ret = fread(&calib_param, sizeof(calib_param), 1, fp_camera_param);
    if (1 != s32Ret)
    {
        SAL_ERROR("read  camera config err\n");
        goto err;
    }

    fclose(fp_camera_param);
    fp_camera_param = NULL;

    s32Ret = pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfSetConfig(pstSubModPrm->pHandle, MTME_BPC_NODE_ID, MTME_SET_BPC_CALIB_PARAM, &calib_param, sizeof(calib_param));
    PPM_HAL_CHECK_RET(s32Ret, err, "set config MTME_SET_BPC_CALIB_PARAM failed!");

    return SAL_SOK;

err:
    if (NULL != fp_camera_param)
    {
        fclose(fp_camera_param);
        fp_camera_param = NULL;
    }

    return SAL_FAIL;
#else
	PPM_LOGW("errrrrrrrrrrr! not supposed call this function! pls check! \n");
	return SAL_SOK;
#endif
}
#endif

/**
 * @function:   Ppm_HalEngineDeinit
 * @brief:      引擎资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_HalEngineDeinit(VOID)
{
	INT32 s32Ret = SAL_FAIL;

	s32Ret = Ppm_HalDeinitEngMatchChan();
	s32Ret |= Ppm_HalDeinitEngMccCalibChan();
	s32Ret |= Ppm_HalDeinitEngFaceCalibChan();
    s32Ret |= Ppm_HalIcfDeinit();
	PPM_HAL_CHECK_RET(s32Ret, exit, "engine deinit failed!");

	PPM_LOGI("ppm engine deinit end! \n");
exit:
    return s32Ret;
}

/**
 * @function:   Ppm_HalEngineInit
 * @brief:      引擎资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_HalEngineInit(VOID)
{
    INT32 s32Ret = SAL_FAIL;

    if (!g_stPpmModPrm.pEncryptHandle)
    {
		s32Ret = IA_InitEncrypt(&g_stPpmModPrm.pEncryptHandle);
		PPM_HAL_CHECK_RET(s32Ret, err, "encrypt_init failed!");
    }

	/* 初始化硬核调度资源 */
    s32Ret = IA_InitHwCore();
    PPM_HAL_CHECK_RET(s32Ret, err, "IA_InitHwCore failed!");

	/* 初始化icf智能计算框架 */
    s32Ret = Ppm_HalIcfInit();
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalIcfInit failed!");

	/* 引擎子通道初始化，包括关联匹配、跨相机标定、人脸单目相机标定三个通道 */
    {
		/* 初始化关联匹配通道 */
		s32Ret = Ppm_HalInitEngMatchChan();
		PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalInitEngMatchChan failed!");

		/* 初始化跨相机标定通道 */
		s32Ret = Ppm_HalInitEngMccCalibChan();
		PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalInitEngMccCalibChan failed!");

		/* 初始化人脸单目相机标定通道 */
		s32Ret = Ppm_HalInitEngFaceCalibChan();
		PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalInitEngFaceCalibChan failed!");
    }

    return SAL_SOK;

err:
    (VOID)Ppm_HalEngineDeinit();
    return SAL_FAIL;
}

/**
 * @function:   Ppm_HalInitAlgApi
 * @brief:      加载引擎应用层动态库符号(libalg.so)
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_HalInitAlgApi(VOID)
{
    INT32 s32Ret = SAL_SOK;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_RET(pstPpmGlobalPrm == NULL, err, "Ppm_HalGetModPrm failed!");

    if (pstPpmGlobalPrm->pAlgHandle)
    {
        PPM_LOGI("alg func ptr is initialized! return success! \n");
        return SAL_SOK;
    }

    s32Ret = Sal_GetLibHandle("libMtmeAlg.so", &pstPpmGlobalPrm->pAlgHandle);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    /* ICF_Interface.h */
#if 0  /* ywn_ppm_todo: 版本号获取，待引擎release版本集成后开放 */
    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pAlgHandle, "ICF_APP_GetVersion", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfAppGetVersion);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");
#endif /* NEVER */

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_HalInitIcfApi
 * @brief:      加载引擎应用框架层动态库符号(libicf.so)
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Ppm_HalInitIcfApi(VOID)
{
    INT32 s32Ret = SAL_SOK;

    PPM_MOD_S *pstPpmGlobalPrm = NULL;

    pstPpmGlobalPrm = Ppm_HalGetModPrm();
    PPM_HAL_CHECK_RET(pstPpmGlobalPrm == NULL, err, "Ppm_HalGetModPrm failed!");

    if (pstPpmGlobalPrm->pIcfLibHandle)
    {
        PPM_LOGI("icf func ptr is initialized! return success! \n");
        return SAL_SOK;
    }

    s32Ret = Sal_GetLibHandle("libicf.so", &pstPpmGlobalPrm->pIcfLibHandle);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibHandle failed!");

    /* ICF_Interface.h */
    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_Init_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfInit);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_Finit_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfFinit);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_Create_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfCreate);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_Destroy_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfDestroy);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_LoadModel_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfLoadModel);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_UnloadModel_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfUnloadModel);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_SetConfig_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfSetConfig);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_GetConfig_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetConfig);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_SetCallback_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfSetCallback);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_GetVersion_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetVersion);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_InputData_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfInputData);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

#if 0
    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_GetRuntimeLimit", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetRunTimeLimit);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_SetCoreAffinity", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfSetCoreAffinity);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_GetMemPoolStatus", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetMemPoolStatus);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");
#endif

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_Package_GetState_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageState);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    s32Ret = Sal_GetLibSymbol(pstPpmGlobalPrm->pIcfLibHandle, "ICF_GetDataPtrFromPkg_V2", (VOID **)&pstPpmGlobalPrm->stIcfFuncP.Ppm_IcfGetPackageDataPtr);
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Sal_GetLibSymbol failed!");

    return SAL_SOK;
err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_HalInitRtld
 * @brief:      Ppm模块加载动态库符号
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Ppm_HalInitRtld(VOID)
{
    INT32 s32Ret = SAL_SOK;

    /* libicf.so */
    s32Ret = Ppm_HalInitIcfApi();
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Ppm_HalInitIcfApi failed!");

    /* libMtmeAlg.so */
    s32Ret = Ppm_HalInitAlgApi();
    PPM_HAL_CHECK_RET(s32Ret != SAL_SOK, err, "Ppm_HalInitAlgApi failed!");

    SAL_INFO("init run time loader end! \n");
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Ppm_HalDeinitGlobalVar
 * @brief:      去初始化全局变量
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 ATTRIBUTE_UNUSED Ppm_HalDeinitGlobalVar(VOID)
{
	INT32 s32Ret = SAL_FAIL;

	/* 跨相机标定通道 */
	{
		PPM_SUB_MOD_MCC_CALIB_PRM_S *pstMccCalibChnPrm = NULL;

		pstMccCalibChnPrm = (PPM_SUB_MOD_MCC_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MCC_CALIB);
		PPM_HAL_CHECK_PTR(pstMccCalibChnPrm, exit, "pstMccCalibChnPrm == null!");

		s32Ret = sem_destroy(&pstMccCalibChnPrm->stIcfCalibPrm.sem);
		PPM_HAL_CHECK_RET_NO_LOOP(s32Ret, "sem_destroy failed!");
	}

	/* 人脸单目相机标定通道 */
	{
		PPM_SUB_MOD_FACE_CALIB_PRM_S *pstFaceCalibChnPrm = NULL;
		
		pstFaceCalibChnPrm = (PPM_SUB_MOD_FACE_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_FACE_CALIB);
		PPM_HAL_CHECK_PTR(pstFaceCalibChnPrm, exit, "pstFaceCalibChnPrm == null!");

		s32Ret = sem_destroy(&pstFaceCalibChnPrm->stIcfFaceCalibPrm.sem);
		PPM_HAL_CHECK_RET_NO_LOOP(s32Ret, "sem_destroy failed!");
	}

    g_stPpmModPrm.uiSubModCnt = 0;
	s32Ret = SAL_SOK;
	
exit:
    return s32Ret;
}

/**
 * @function:   Ppm_HalInitGlobalVar
 * @brief:      初始化全局变量
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
INT32 Ppm_HalInitGlobalVar(VOID)
{
    INT32 s32Ret = SAL_FAIL;

	/* 跨相机标定通道 */
	{
		PPM_SUB_MOD_MCC_CALIB_PRM_S *pstMccCalibChnPrm = NULL;

		pstMccCalibChnPrm = (PPM_SUB_MOD_MCC_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_MCC_CALIB);
		PPM_HAL_CHECK_PTR(pstMccCalibChnPrm, exit, "pstMccCalibChnPrm == null!");

		s32Ret = sem_init(&pstMccCalibChnPrm->stIcfCalibPrm.sem, 0, 0);
		PPM_HAL_CHECK_RET(s32Ret, exit, "sem_init failed!");
	}

	/* 人脸单目相机标定通道 */
	{
		PPM_SUB_MOD_FACE_CALIB_PRM_S *pstFaceCalibChnPrm = NULL;

		pstFaceCalibChnPrm = (PPM_SUB_MOD_FACE_CALIB_PRM_S *)Ppm_HalGetSubModChnPrm(PPM_SUB_MOD_FACE_CALIB);
		PPM_HAL_CHECK_PTR(pstFaceCalibChnPrm, exit, "pstFaceCalibChnPrm == null!");

		s32Ret = sem_init(&pstFaceCalibChnPrm->stIcfFaceCalibPrm.sem, 0, 0);
		PPM_HAL_CHECK_RET(s32Ret, exit, "sem_init failed!");
	}

    g_stPpmModPrm.uiSubModCnt = PPM_SUB_MOD_MAX_NUM;

    s32Ret = Ppm_HalInitRtld();
    PPM_HAL_CHECK_RET(SAL_SOK != s32Ret, exit, "Ppm_HalInitRtld failed!");

exit:
    return s32Ret;
}

/**
 * @function:   Ppm_HalDeinit
 * @brief:      HAL层资源去初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalDeinit(VOID)
{
    if (!g_stPpmModPrm.uiInitFlag)
    {
        SAL_WARN("ppm hal is deinitialized! return Success! \n");
        return SAL_SOK;
    }

    (VOID)Ppm_HalEngineDeinit();

    /* (VOID)Ppm_HalDeinitGlobalVar(); */

    g_stPpmModPrm.uiInitFlag = SAL_FALSE;
    return SAL_SOK;
}

/**
 * @function:   Ppm_HalInit
 * @brief:      HAL层资源初始化
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Ppm_HalInit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    if (g_stPpmModPrm.uiInitFlag)
    {
        PPM_LOGW("ppm hal is initialized! return Success! \n");
        return SAL_SOK;
    }

    s32Ret = Ppm_HalEngineInit();
    PPM_HAL_CHECK_RET(s32Ret, err, "Ppm_HalEngineInit failed!");

    g_stPpmModPrm.uiInitFlag = SAL_TRUE;
    uiInitModeFlag = SAL_TRUE;
	
    PPM_LOGI("%s end! \n", __func__);
    return SAL_SOK;

err:
    (VOID)Ppm_HalDeinit();
    return SAL_FAIL;
}

