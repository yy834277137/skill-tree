/**
 * @file   ai_rk3588.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  ai模块，实现arm平台相关智能资源初始化
 * @author sunzelin
 * @date   2022/8/5
 * @note
 * @note \n History
   1.日    期: 2022/8/5
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
#define AI_DSP_RK3588_NPU_NUM (3)
/* rk3588平台层调度器最多可使用arm cpu个数 */
#define AI_DSP_RK3588_CPU_NUM (1)
/* rk3588平台层调度器最多可使用ipu个数 */
#define AI_DSP_RK3588_IPU_NUM (1)
/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef struct _AI_RK3588_INNER_INIT_PRM_
{
    /* rk3588平台层支持的调度器参数 */
    HKANN_SCHEDULER_MGR_PARAM_T stSchedInitPrm;
} AI_RK3588_INNER_INIT_PRM_S;

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
 * @function   ai_rk3588_Outer2Inner
 * @brief      将hal层参数转换为rk平台层内部参数
 * @param[in]  IN const AI_HAL_INIT_PRM_S *pstOuterInitPrm
 * @param[in]  OUT AI_RK3588_INNER_INIT_PRM_S *pstInnerInitPrm
 * @param[out] None
 * @return     static INT32
 */
static INT32 ai_rk3588_Outer2Inner(IN const AI_HAL_INIT_PRM_S *pstOuterInitPrm,
                                   OUT AI_RK3588_INNER_INIT_PRM_S *pstInnerInitPrm)
{
    UINT32 i, u32Idx, u32AiXspChnNum, u32CoreLeft;

    u32AiXspChnNum = 0;
    if (pstOuterInitPrm->stAiXspInitPrm.bAiXspUsed)
    {
        u32AiXspChnNum = pstOuterInitPrm->stAiXspInitPrm.u32EnableChnNum;
    }

    /* 校验剩余的npu资源是否足够 */
    if (u32AiXspChnNum >= AI_DSP_RK3588_NPU_NUM)
    {
        SAL_LOGE("invalid ai-xsp chn num %d >= max %d! \n", u32AiXspChnNum, AI_DSP_RK3588_NPU_NUM);
        return SAL_FAIL;
    }

    u32Idx = 0;

    /* npu分配策略: 除分配给AI-XSP之外的所有的npu资源，当前芯片内所有智能组件由cnn调度器统一调度使用，公用调度器 */
    u32CoreLeft = AI_DSP_RK3588_NPU_NUM - u32AiXspChnNum;
    for (i = 0; i < u32CoreLeft; ++i)
    {
        pstInnerInitPrm->stSchedInitPrm.dev_data[u32Idx].dev_type = HKANN_DEV_NPU0; /* npu设备标识 */
        pstInnerInitPrm->stSchedInitPrm.dev_data[u32Idx++].dev_id = HKANN_DEV_ID0 + u32AiXspChnNum + i;
    }

    /* cpu分配 */
    u32CoreLeft = AI_DSP_RK3588_CPU_NUM;
    for (i = 0; i < u32CoreLeft; ++i)
    {
        pstInnerInitPrm->stSchedInitPrm.dev_data[u32Idx].dev_type = HKANN_DEV_NPU1; /* arm cpu设备标识 */
        pstInnerInitPrm->stSchedInitPrm.dev_data[u32Idx++].dev_id = HKANN_DEV_ID2 + i;
    }

    /* ipu分配 */
    u32CoreLeft = AI_DSP_RK3588_IPU_NUM;
    for (i = 0; i < u32CoreLeft; ++i)
    {
        pstInnerInitPrm->stSchedInitPrm.dev_data[u32Idx].dev_type = HKANN_DEV_NPU4; /* ipu设备标识 */
        pstInnerInitPrm->stSchedInitPrm.dev_data[u32Idx++].dev_id = HKANN_DEV_ID0 + i;
    }

    /* 保存初始化的dev个数 */
    pstInnerInitPrm->stSchedInitPrm.dev_num = u32Idx;

    return SAL_SOK;
}

/**
 * @function   ai_rk3588_InitHwCore
 * @brief      初始化rk3588硬核资源
 * @param[in]  IN AI_RK3588_INNER_INIT_PRM_S *pstInnerInitPrm
 * @param[in]  OUT VOID **ppHwScheHandle
 * @param[out] None
 * @return     static INT32
 */
static INT32 ai_rk3588_InitHwCore(IN AI_RK3588_INNER_INIT_PRM_S *pstInnerInitPrm,
                                  OUT VOID **ppHwScheHandle)  /* TODO: 增加入参，区分不同产品的NN硬核资源分配 */
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uiCoreId[8];

    /* RK平台，绑核，让CNN线程继承绑核属性*/
    uiCoreId[0] = 2;
    uiCoreId[1] = 3;
    ai_hal_SetThreadCoreBindExt(2, uiCoreId);

    SAL_LOGI("========rk3588, init hw core! \n");
    for (int i = 0; i < pstInnerInitPrm->stSchedInitPrm.dev_num; i++)
    {
        SAL_LOGI("i %d, dev_type %d, dev_id %d \n",
                 i, pstInnerInitPrm->stSchedInitPrm.dev_data[i].dev_type, pstInnerInitPrm->stSchedInitPrm.dev_data[i].dev_id);
    }

    s32Ret = HKANN_CreateScheduler(&pstInnerInitPrm->stSchedInitPrm, ppHwScheHandle);
    if (s32Ret < 0)
    {
        SAL_LOGE("HKANN_CreateScheduler failed, return %x!!!\n", s32Ret);
        return SAL_FAIL;
    }

    SAL_LOGI("Rk Hw core init success!!!!\n");
    return SAL_SOK;
}

/**
 * @function   ai_rk3588_Init
 * @brief      初始化接口
 * @param[in]  const AI_HAL_INIT_PRM_S *pstInitPrm
 * @param[in]  VOID **ppSchedHandle
 * @param[out] None
 * @return     INT32
 */
static INT32 ai_rk3588_Init(AI_HAL_INIT_PRM_S *pstInitPrm, VOID **ppSchedHandle)   /* TODO: 增加入参，区分不同产品的NN硬核资源分配 */
{
    INT32 s32Ret = SAL_FAIL;

    AI_RK3588_INNER_INIT_PRM_S stInnerSchedInitPrm = {0};

    /* checker */
    if (NULL == pstInitPrm || NULL == ppSchedHandle)
    {
        SAL_LOGE("ptr null! %p, %p \n", pstInitPrm, ppSchedHandle);
        return SAL_FAIL;
    }

    /* 将外部调度器初始化参数，转换为平台层内部参数 */
    s32Ret = ai_rk3588_Outer2Inner(pstInitPrm, &stInnerSchedInitPrm);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("ai_rk3588_Outer2Inner failed! \n");
        return SAL_FAIL;
    }

    /* 初始化硬核调度资源 */
    s32Ret = ai_rk3588_InitHwCore(&stInnerSchedInitPrm, ppSchedHandle);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("ai_rk3588_InitHwCore failed! \n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   ai_rk3588_Deinit
 * @brief      去初始化接口
 * @param[in]  VOID
 * @param[out] None
 * @return     INT32
 */
static INT32 ai_rk3588_Deinit(VOID)
{
    /* 保留 */
    return SAL_SOK;
}

/**
 * @function   ai_hal_register
 * @brief      平台相关的AI 接口注册
 * @param[in]  void
 * @param[out] None
 * @return     AI_PLAT_OPS_ST *
 */
AI_PLAT_OPS_ST *ai_hal_register(void)
{
    g_stAiPlatOps.pAiInit = ai_rk3588_Init;
    g_stAiPlatOps.pAiDeInit = ai_rk3588_Deinit;
    g_stAiPlatOps.pAiGetSbaeSchedHdl = NULL;

    return &g_stAiPlatOps;
}

