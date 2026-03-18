/**
 * @file   ai_hal.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  平台层智能硬核资源
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
#include "sal.h"
#include "dspcommon.h"
#include "platform_hal.h"
#include "platform_sdk.h"

#include "ai_hal_inter.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/
/* 平台相关的内存申请抽象函数结构体 */
static AI_PLAT_OPS_ST *g_pstAiPlatOps = NULL;

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/
/**
 * @function   ai_hal_SetThreadCoreBindExt
 * @brief      绑核函数，支持绑定多颗核
 * @param[in]  UINT32 uiCoreNum    需要绑的核数目
 * @param[in]  UINT32 uiCoreId[8]  具体绑核的核ID
 * @param[out] None
 * @return     INT32
 */
INT32 ai_hal_SetThreadCoreBindExt(UINT32 uiCoreNum, UINT32 uiCoreId[8])
{
    cpu_set_t mask;
    int nCoreIdx = 0;
    
    __CPU_ZERO_S(sizeof(cpu_set_t), &mask);

    for (nCoreIdx = 0; nCoreIdx < uiCoreNum; nCoreIdx++)
    {
        __CPU_SET_S(uiCoreId[nCoreIdx], sizeof(cpu_set_t), &mask);
    }
    
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    {
        SAL_LOGE("set_thread_core_bind. pthread_setaffinity_np faild\n");
        return SAL_FAIL;
    }
    return SAL_SOK;
} 

/**
 * @function   ai_hal_Init
 * @brief      ai平台层初始化抽象接口
 * @param[in]  AI_HAL_INIT_PRM_S *pstInitPrm  
 * @param[in]  VOID **ppSchedHandle           
 * @param[out] None
 * @return     INT32
 */
INT32 ai_hal_Init(AI_HAL_INIT_PRM_S *pstInitPrm, VOID **ppSchedHandle)
{
    INT32 s32Ret = SAL_FAIL;

    g_pstAiPlatOps = ai_hal_register();
    if (NULL == g_pstAiPlatOps)
    {
        SAL_ERROR("Ai Register error !! \n");
        return SAL_FAIL;
    }

    s32Ret = g_pstAiPlatOps->pAiInit(pstInitPrm, ppSchedHandle);
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("Ai Plat Init error %d \n", s32Ret);
        return s32Ret;
    }

    return SAL_SOK;
}

/**
 * @function   ai_hal_Deinit
 * @brief      ai平台层去初始化抽象接口
 * @param[in]  VOID  
 * @param[out] None
 * @return     INT32
 */
INT32 ai_hal_Deinit(VOID)
{
    INT32 s32Ret = SAL_FAIL;

	if (NULL != g_pstAiPlatOps && NULL != g_pstAiPlatOps->pAiDeInit)
	{
	    s32Ret = g_pstAiPlatOps->pAiDeInit();
	    if (s32Ret != SAL_SOK)
	    {
	        SAL_ERROR("Ai Plat DeInit error %d \n", s32Ret);
	        return s32Ret;
	    }
	}
	else
	{
		SAL_LOGW("invalid func ptr! %p \n", g_pstAiPlatOps);
	}

	/* reset func ptr as NULL */
	g_pstAiPlatOps = NULL;

    return SAL_SOK;

}

/**
 * @function   ai_hal_GetSbaeSchedHdl
 * @brief      获取行为分析调度器句柄
 * @param[in]  VOID  
 * @param[out] None
 * @return     VOID*
 */
VOID* ai_hal_GetSbaeSchedHdl(VOID)
{
	if (NULL == g_pstAiPlatOps || NULL == g_pstAiPlatOps->pAiGetSbaeSchedHdl)
	{
	    SAL_LOGI("ptr null! %p, %p \n", g_pstAiPlatOps, g_pstAiPlatOps->pAiGetSbaeSchedHdl);
		return NULL;
	}

	return g_pstAiPlatOps->pAiGetSbaeSchedHdl();
}

