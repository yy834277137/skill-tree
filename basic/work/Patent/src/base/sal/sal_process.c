/***
 * @file   sal_process.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  进程功能
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#include "sal_process.h"

#include <errno.h>
#include <string.h>

/**
 * @brief:      设置名称
 * @param[in]:  CHAR *prcName 名称
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SetProcessName(const CHAR *prcName)
{
    /* 只能在当前进程中配置 */
    prctl(PR_SET_NAME, prcName);
    return SAL_SOK;
}

/**
 * @function:   SAL_GetSchedPriorityScope
 * @brief:      获取进程调度策略静态优先级范围
 * @param[in]:  INT32 policy
 * @param[in]:  INT32 *minPriority
 * @param[in]:  INT32 *maxPriority
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_GetSchedPriorityScope(INT32 policy, INT32 *minPriority, INT32 *maxPriority)
{
    INT32 priority;

    if (NULL == maxPriority || NULL == minPriority)
    {
        SAL_ERROR("Ptr NULL! \n");
        return SAL_FAIL;
    }

    if ((priority = sched_get_priority_min(policy)) == -1)
    {
        return SAL_FAIL;
    }
    else
    {
        *minPriority = priority;
    }

    if ((priority = sched_get_priority_max(policy)) == -1)
    {
        return SAL_FAIL;
    }
    else
    {
        *maxPriority = priority;
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_GetScheduler
 * @brief:      获取指定进程的调度策略
 * @param[in]:  pid_t pid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_GetScheduler(pid_t pid)
{
    INT32 policy;

    if ((policy = sched_getscheduler(pid)) != 0)
    {
        return SAL_FAIL;
    }

    return policy;
}

/**
 * @function   SAL_SetScheduler
 * @brief      设置指定进程的调度策略
 * @param[in]  pid_t pid       
 * @param[in]  INT32 policy    
 * @param[in]  INT32 priority  
 * @param[out] None
 * @return     INT32
 */
INT32 SAL_SetScheduler(pid_t pid, INT32 policy, INT32 priority)
{
    struct sched_param param;

	param.sched_priority = priority;

    if (sched_setscheduler(pid, policy, &param) != 0)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_GetSchedPriority
 * @brief:      获取指定进程的优先级
 * @param[in]:  pid_t pid
 * @param[in]:  INT32 *priority
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_GetSchedPriority(pid_t pid, INT32 *priority)
{
    struct sched_param param;

    if (priority == NULL)
    {
        return SAL_FAIL;
    }

    if (sched_getparam(pid, &param) != 0)
    {
        return SAL_FAIL;
    }

    *priority = param.sched_priority;

    return SAL_SOK;
}

/**
 * @function:   SAL_SetSchedPriority
 * @brief:      设置指定进程的调度优先级
 * @param[in]:  pid_t pid
 * @param[in]:  INT32 priority
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SetSchedPriority(pid_t pid, INT32 priority)
{
    struct sched_param param;

    if (sched_getparam(pid, &param) != 0)
    {
        return SAL_FAIL;
    }

    param.sched_priority = priority;

    if (sched_setparam(pid, &param) != 0)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_SchedSuspend
 * @brief:      进程主动让出处理器,并将自己等候调度队列队尾
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SchedSuspend(void)
{
    if (sched_yield() != 0)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_SchedGetRRInterval
 * @brief:      获取指定进程轮转调度的时间片间隔
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SchedGetRRInterval(pid_t pid, struct timespec *tp)
{
    if (sched_rr_get_interval(pid, (struct timespec *)tp) != 0)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}

