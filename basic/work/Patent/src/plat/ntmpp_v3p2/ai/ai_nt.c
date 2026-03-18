/**
 * @file   ai_nt.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  nt平台层智能硬核资源初始化
 * @author sunzelin
 * @date   2022/8/8
 * @note
 * @note \n History
   1.日    期: 2022/8/8
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include <sal.h>
#include <platform_sdk.h>
#include "ai_hal_inter.h"

#include "hkann_scheduler.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
/* rk3588平台层调度器最多可使用npu个数 */
#define AI_DSP_NT98336_NPU_NUM (3)

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef struct _AI_NT_INNER_INIT_PRM_
{
	/* nt98336平台层支持的调度器参数 */
	HKANN_SCHEDULER_MGR_PARAM_T stSchedInitPrm;
} AI_NT_INNER_INIT_PRM_S;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/
/* 平台相关的AI硬件平台函数结构体 */
static AI_PLAT_OPS_ST g_stAiPlatOps = {0};

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/
/**
 * @function   ai_nt_Outer2Inner
 * @brief      将hal层参数转换为nt平台层内部参数
 * @param[in]  IN const AI_HAL_INIT_PRM_S *pstOuterInitPrm  
 * @param[in]  OUT AI_NT_INNER_INIT_PRM_S *pstInnerInitPrm  
 * @param[out] None
 * @return     static INT32
 */
static INT32 ai_nt_Outer2Inner(IN const AI_HAL_INIT_PRM_S *pstOuterInitPrm,
                               OUT AI_NT_INNER_INIT_PRM_S *pstInnerInitPrm)
{
	UINT32 i, u32Idx, u32AiXspChnNum, u32CoreLeft;

	u32AiXspChnNum = 0;
	if (pstOuterInitPrm->stAiXspInitPrm.bAiXspUsed)
	{
		/* nt平台不考虑AI-XSP通道个数及为其单独分配npu，此处保留，便于后续扩充 */
		//u32AiXspChnNum = pstOuterInitPrm->stAiXspInitPrm.u32EnableChnNum;
	}

	/* 校验剩余的npu资源是否足够 */
	if (u32AiXspChnNum >= AI_DSP_NT98336_NPU_NUM)
	{
		SAL_LOGE("invalid ai-xsp chn num %d >= max %d! \n", u32AiXspChnNum, AI_DSP_NT98336_NPU_NUM);
		return SAL_FAIL;
	}

	u32Idx = 0;
	
	/* npu分配策略: 所有智能公用3个npu资源，由底层cnn调度器实现调度 */
	u32CoreLeft = AI_DSP_NT98336_NPU_NUM - u32AiXspChnNum;
	for (i = 0; i < u32CoreLeft; ++i)
	{
		pstInnerInitPrm->stSchedInitPrm.dev_data[u32Idx].dev_id = NTCNN_DEV_ID_HW0 + u32AiXspChnNum + i;
		pstInnerInitPrm->stSchedInitPrm.dev_data[u32Idx++].per = 1;
	}

	/* 保存初始化的dev个数 */
	pstInnerInitPrm->stSchedInitPrm.dev_num = u32Idx;

	return SAL_SOK;
}

/**
 * @function   ai_nt_InitHwCore
 * @brief      初始化nt98336硬核资源
 * @param[in]  IN AI_NT_INNER_INIT_PRM_S *pstInnerInitPrm  
 * @param[in]  OUT VOID **ppHwScheHandle                   
 * @param[out] None
 * @return     static INT32
 */
static INT32 ai_nt_InitHwCore(IN AI_NT_INNER_INIT_PRM_S *pstInnerInitPrm, 
	                          OUT VOID **ppHwScheHandle)  /* TODO: 增加入参，区分不同产品的NN硬核资源分配 */
{
    /* nt暂不需要外部显示创建调度器 */
    return SAL_SOK;

    /* 规避代码静态检测缺陷 */
#if 0
    INT32 s32Ret = SAL_FAIL;

	/* nt暂不需要外部显示创建调度器 */
	return SAL_SOK;  

	SAL_LOGI("========nt98336, init hw core! \n");
	for (int i = 0; i < pstInnerInitPrm->stSchedInitPrm.dev_num; i++)
	{
		SAL_LOGI("i %d, dev_id %d, per %f \n", 
			      i, pstInnerInitPrm->stSchedInitPrm.dev_data[i].dev_id, pstInnerInitPrm->stSchedInitPrm.dev_data[i].per);
	}

	s32Ret =  HKANN_CreateScheduler(&pstInnerInitPrm->stSchedInitPrm, ppHwScheHandle); // using cuda_handle param
	if (s32Ret < 0)
	{
		SAL_LOGE("HKANN_CreateScheduler failed, return %x!!!\n", s32Ret);
		return SAL_FAIL;
	}

	/* 目前nt最新的基线代码中XSIE_V1.0.2_NT98336_CNN2.4.3_build20220422_release_svn66935.zip，
	   引擎初始化时不需要传入调度器句柄，此处为了兼容及减小流程改动，手动将handle对外输出为NULL */
	*ppHwScheHandle = NULL;
	
	SAL_LOGI("nt Hw core init success!!!!\n");
	return SAL_SOK;
#endif
}

/**
 * @function   ai_nt_Init
 * @brief      cai推理平台初始化接口(NT版本)
 * @param[in]  AI_HAL_INIT_PRM_S *pstInitPrm  
 * @param[in]  VOID **ppSchedHandle           
 * @param[out] None
 * @return     INT32
 */
static INT32 ai_nt_Init(AI_HAL_INIT_PRM_S *pstInitPrm, VOID **ppSchedHandle)
{
    INT32 s32Ret = SAL_SOK;

	AI_NT_INNER_INIT_PRM_S stInnerSchedInitPrm = {0};

	/* checker */
	if (NULL == pstInitPrm || NULL == ppSchedHandle)
	{
		SAL_LOGE("ptr null! %p, %p \n", pstInitPrm, ppSchedHandle);
		return SAL_FAIL;
	}

#if 0
    s32Ret = hd_videoproc_init();
    if (s32Ret != SAL_SOK)
    {
        SAL_WARN("hd_videoproc_init err !Ret %x\n", s32Ret);
    }
#endif

#if 0  /* NT V1.0.4开始更新CNN到2.5.6，将vendor_ai_init收敛到cnn内部库中，故外部集成时不需要再初始化 */
    UINT32 u32Schd = VENDOR_AI_PROC_SCHD_FAIR;

    /*配置可扩展插件及运行时的调度*/
    s32Ret |= vendor_ai_cfg_set(VENDOR_AI_CFG_PLUGIN_ENGINE, vendor_ai_cpu1_get_engine());
    s32Ret |= vendor_ai_cfg_set(VENDOR_AI_CFG_PROC_SCHD, &u32Schd);

    /*AI初始化*/
    s32Ret |= vendor_ai_init();
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("vendor_ai_init failed!\n");
        /*AI 去初始化*/
        vendor_ai_uninit();
        return SAL_FAIL;
    }
#endif

	/* 将外部调度器初始化参数，转换为平台层内部参数 */
	s32Ret = ai_nt_Outer2Inner(pstInitPrm, &stInnerSchedInitPrm);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("ai_rk3588_Outer2Inner failed! \n");
		return SAL_FAIL;
	}

	/* 初始化硬核调度资源 */
	s32Ret = ai_nt_InitHwCore(&stInnerSchedInitPrm, ppSchedHandle);
	if (SAL_SOK != s32Ret)
	{
		SAL_LOGE("ai_rk3588_InitHwCore failed! \n");
		return SAL_FAIL;
	}

    return SAL_SOK;
}

/**
 * @function    ai_nt_Deinit
 * @brief         ai推理平台去初始化接口(NT版本)
 * @param[in]  NULL
 * @param[out] NULL
 * @return  SAL_SOK
 */
static INT32 ai_nt_Deinit()
{
    INT32 s32Ret = SAL_SOK;

#if 0  /* NT V1.0.4开始更新CNN到2.5.6，将vendor_ai_init收敛到cnn内部库中，故外部集成时不需要再初始化 */
    vendor_ai_uninit();
#endif

    s32Ret = hd_videoproc_uninit();
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("hd_videoproc_uninit fail=%d\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    ai_hal_register
 * @brief         平台相关的AI 接口注册
 * @param[in]
 * @param[out]
 * @return
 */
AI_PLAT_OPS_ST *ai_hal_register(void)
{
    g_stAiPlatOps.pAiInit = ai_nt_Init;
    g_stAiPlatOps.pAiDeInit = ai_nt_Deinit;
    g_stAiPlatOps.pAiGetSbaeSchedHdl = NULL;

    return &g_stAiPlatOps;
}

