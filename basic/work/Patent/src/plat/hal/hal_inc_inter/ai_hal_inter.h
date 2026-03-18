/**
 * @file   ai_hal_inter.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  ai平台层智能硬核资源内部数据结构定义
 * @author sunzelin
 * @date   2022/8/10
 * @note
 * @note \n History
   1.日    期: 2022/8/10
     作    者: sunzelin
     修改历史: 创建文件
 */

#ifndef _AI_HAL_INTER_H_
#define _AI_HAL_INTER_H_

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "sal.h"
#include "platform_sdk.h"
#include "platform_hal.h"

#include "ai_hal_api.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef struct _AI_PLAT_OPS
{
	/* 平台层智能硬核资源初始化接口*/
    INT32 (*pAiInit)(AI_HAL_INIT_PRM_S *pstInitPrm, VOID **ppSchedHandle);
	/* 平台层智能硬核资源去初始化接口*/
	INT32 (*pAiDeInit)(VOID);
	/* 获取行为分析调度句柄，仅在hisi3559a有具体实现 */
	VOID* (*pAiGetSbaeSchedHdl)(VOID);
} AI_PLAT_OPS_ST;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/
/**
 * @function   ai_hal_register
 * @brief      AI 平台相关注册函数
 * @param[in]  void  
 * @param[out] None
 * @return     AI_PLAT_OPS_ST *
 */
AI_PLAT_OPS_ST *ai_hal_register(void);

/**
 * @function   ai_hal_SetThreadCoreBindExt
 * @brief      绑核函数，支持绑定多颗核
 * @param[in]  UINT32 uiCoreNum    需要绑的核数目
 * @param[in]  UINT32 uiCoreId[8]  具体绑核的核ID
 * @param[out] None
 * @return     INT32
 */
INT32 ai_hal_SetThreadCoreBindExt(UINT32 uiCoreNum, UINT32 uiCoreId[8]);

#endif /* _AI_HAL_INTER_H_ */


