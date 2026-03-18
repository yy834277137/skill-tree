/**
 * @file   ai_hal_api.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   AI - 硬件相关--- hal层接口
 * @author zongkai5
 * @date  2022/2/21
 * @note
 * @note \n History
   1.Date        : 2022/2/21
     Author      : zongkai5
     Modification:  用于封装不同平台的AI硬核加载相关接口
 */
#ifndef _AI_HAL_API_H
#define _AI_HAL_API_H

#include "sal.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*****************************************************************************
                                   宏定义
*****************************************************************************/

/*****************************************************************************
                                   结构体定义
*****************************************************************************/
typedef struct _AI_HAL_XSP_INIT_PRM_
{
	/* 是否启用AI-XSP，若是则需要根据单/双路分配特定个数npu */
	BOOL bAiXspUsed;
	UINT32 u32EnableChnNum;
} AI_HAL_XSP_INIT_PRM_S;

typedef struct _AI_HAL_OTHERS_INIT_PRM_
{
	/* 人包关联智能是否使能，当前H5平台需要分配dsp0作为深度图计算 */
	BOOL bPpmUsed;
	/* 当前H5平台行为分析调度器创建有特殊处理，使用单独调度句柄及存在runtime内存申请操作，此处将内存申请接口作为函数指针传入，平台层解耦 */

	
	/* 保留 */
	UINT32 u32Reserved[7];
} AI_HAL_OTHERS_INIT_PRM_S;

typedef struct _AI_HAL_INIT_PRM_
{
	/* AI-XSP初始化参数 */
	AI_HAL_XSP_INIT_PRM_S stAiXspInitPrm;

	/* 其他智能初始化参数 */
	AI_HAL_OTHERS_INIT_PRM_S stOthersInitPrm;
} AI_HAL_INIT_PRM_S;

/*****************************************************************************
                                   函数说明
*****************************************************************************/

/**
 * @function   ai_hal_Init
 * @brief      ai平台层初始化抽象接口
 * @param[in]  AI_HAL_INIT_PRM_S *pstInitPrm  
 * @param[in]  VOID **ppSchedHandle           
 * @param[out] None
 * @return     INT32
 */
INT32 ai_hal_Init(AI_HAL_INIT_PRM_S *pstInitPrm, VOID **ppSchedHandle);

/**
 * @function   ai_hal_Deinit
 * @brief      ai平台层去初始化抽象接口
 * @param[in]  VOID  
 * @param[out] None
 * @return     INT32
 */
INT32 ai_hal_Deinit(VOID);

/**
 * @function   ai_hal_GetSbaeSchedHdl
 * @brief      获取行为分析调度器句柄
 * @param[in]  VOID  
 * @param[out] None
 * @return     VOID*
 */
VOID* ai_hal_GetSbaeSchedHdl(VOID);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif



