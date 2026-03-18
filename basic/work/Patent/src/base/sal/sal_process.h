/***
 * @file   sal_process.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  进程功能
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef _SAL_PROCESS_H_
#define _SAL_PROCESS_H_

#include <sched.h>
#include "sal.h"

/**
 * @function:   SAL_SetProcessName
 * @brief:      设置名称
 * @param[in]:  CHAR *prcName
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SetProcessName(const CHAR *prcName);

/**
 * @function:   SAL_GetSchedPriorityScope
 * @brief:      获取进程调度策略静态优先级范围
 * @param[in]:  INT32 policy
 * @param[in]:  INT32 *minPriority
 * @param[in]:  INT32 *maxPriority
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_GetSchedPriorityScope(INT32 policy, INT32 *minPriority, INT32 *maxPriority);

/**
 * @function:   SAL_GetScheduler
 * @brief:      获取指定进程的调度策略
 * @param[in]:  pid_t pid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_GetScheduler(pid_t pid);

/**
 * @function   SAL_SetScheduler
 * @brief      设置指定进程的调度策略
 * @param[in]  pid_t pid       
 * @param[in]  INT32 policy    
 * @param[in]  INT32 priority  
 * @param[out] None
 * @return     INT32
 */
INT32 SAL_SetScheduler(pid_t pid, INT32 policy, INT32 priority);

/**
 * @function:   SAL_GetSchedPriority
 * @brief:      获取指定进程的优先级
 * @param[in]:  pid_t pid
 * @param[in]:  INT32 *priority
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_GetSchedPriority(pid_t pid, INT32 *priority);

/**
 * @function:   SAL_SetSchedPriority
 * @brief:      设置指定进程的调度优先级
 * @param[in]:  pid_t pid
 * @param[in]:  INT32 priority
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SetSchedPriority(pid_t pid, INT32 priority);

/**
 * @function:   SAL_SchedSuspend
 * @brief:      进程主动让出处理器,并将自己等候调度队列队尾
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SchedSuspend(void);

/**
 * @function:   SAL_SchedGetRRInterval
 * @brief:      获取指定进程轮转调度的时间片间隔
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SchedGetRRInterval(pid_t pid, struct timespec *tp);

#endif

