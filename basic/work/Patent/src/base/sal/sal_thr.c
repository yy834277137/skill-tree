/***
 * @file   sal_thr.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  线程操作
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#include "sal_thr.h"

#include <errno.h>
#include <string.h>

#define FUNC_NAME(x) # x

#define OSA_assert(x)  \
    { \
        if ((x) == 0) { \
            SAL_LOGE(" ASSERT (%s|%s|%d)\r\n", __FILE__, __func__, __LINE__); \
            while (getchar() != 'q')  \
                ; \
        } \
    }

/***
 * @brief  设置线程名字
 * @param  [CHAR] *thrName
 * @return [*]
 */
INT32 SAL_setThrName(const CHAR *thrName)
{
    /* 只能在当前线程中配置 */
    prctl(PR_SET_NAME, thrName);
    return SAL_SOK;
}

/***
 * @brief  创建线程
 * @param  [SAL_ThrHndl] *hndl          返回句柄
 * @param  [SAL_ThrEntryFunc] entryFunc 线程执行的函数
 * @param  [UINT32] pri                 优先级
 * @param  [UINT32] stackSize           堆栈大小
 * @param  [VOID] *prm                  私有信息
 * @return [*]
 */
INT32 SAL_thrCreate(SAL_ThrHndl *hndl, SAL_ThrEntryFunc entryFunc, UINT32 pri, UINT32 stackSize, VOID *prm)
{
    int status = SAL_SOK;
    pthread_attr_t thread_attr;
    struct sched_param schedprm;

    /* initialize thread attributes structure */
    status = pthread_attr_init(&thread_attr);

    if (status != SAL_SOK)
    {
        SAL_LOGE("SAL_thrCreate() - Could not initialize thread attributes\n");
        return status;
    }

    /* 设置堆栈大小*/
    if (0 == stackSize)
    {
        pthread_attr_setstacksize(&thread_attr, SAL_THR_STACK_SIZE_DEFAULT);
    }
    else
    {
        pthread_attr_setstacksize(&thread_attr, stackSize);
    }

    /*
        SCHED_OTHER（正常、非实时）、SCHED_RR（实时、轮转法）和 SCHED_FIFO（实时、先入先出）三种，
        缺省为SCHED_OTHER，后两种调度策略仅对超级用户有效
     */

    /*
        SCHED_FIFO 会导致上下文切换变多，使得系统的 Load average 变高
     */

    status |= pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);

    if (pri > SAL_THR_PRI_MAX)
    {
        pri = SAL_THR_PRI_MAX;
    }
    else if (pri < SAL_THR_PRI_MIN)
    {
        pri = SAL_THR_PRI_MIN;
    }

    /* 需要高优先级的线程，配置为 SCHED_RR */
    if (SAL_THR_PRI_MAX == pri)
    {
        status |= pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
        schedprm.sched_priority = SAL_THR_PRI_MAX;
        status |= pthread_attr_setschedparam(&thread_attr, &schedprm);
        if (status != SAL_SOK)
        {
            SAL_LOGE("SAL_thrCreate() - Could not initialize thread attributes\n");
            goto error_exit;
        }
    }
    else if (pri > SAL_THR_PRI_DEFAULT)
    {
        SAL_LOGD("sched_priority is %d\n", pri);
        status |= pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
        schedprm.sched_priority = pri;
        status |= pthread_attr_setschedparam(&thread_attr, &schedprm);
        if (status != SAL_SOK)
        {
            SAL_LOGE("SAL_thrCreate() - Could not initialize thread attributes\n");
            goto error_exit;
        }
    }
    else
    {

    }

    status = pthread_create(&hndl->hndl, &thread_attr, entryFunc, prm);

    if (status != SAL_SOK)
    {
        SAL_LOGE("SAL_thrCreate() - Could not create thread [%s]\n", strerror(errno));
        OSA_assert(status == SAL_SOK);
    }

error_exit:
    pthread_attr_destroy(&thread_attr);

    return status;
}

/***
 * @brief  解除关联
 * @param  [SAL_ThrHndl] *hndl
 * @return [*]
 */
INT32 SAL_thrDetach(SAL_ThrHndl *hndl)
{
    INT32 status = SAL_SOK;

    status = pthread_detach(hndl->hndl);

    return status;
}

/***
 * @brief  join
 * @param  [SAL_ThrHndl] *hndl
 * @return [*]
 */
INT32 SAL_thrJoin(SAL_ThrHndl *hndl)
{
    INT32 status = SAL_SOK;
    VOID *returnVal = NULL;

    status = pthread_join(hndl->hndl, &returnVal);

    return status;
}

/***
 * @brief  删除线程
 * @param  [SAL_ThrHndl] *hndl
 * @return [*]
 */
INT32 SAL_thrDelete(SAL_ThrHndl *hndl)
{
    INT32 status = SAL_SOK;

    if (&hndl->hndl != NULL)
    {
        status = pthread_cancel(hndl->hndl);
        status = SAL_thrJoin(hndl);
    }

    return status;
}

/***
 * @brief  修改现场优先级
 * @param  [SAL_ThrHndl] *hndl
 * @param  [UINT32] pri
 * @return [*]
 */
INT32 SAL_thrChangePri(SAL_ThrHndl *hndl, UINT32 pri)
{
    INT32 status = SAL_SOK;
    INT32 policy = 0;
    struct sched_param schedprm;

    if (pri > SAL_THR_PRI_MAX)
    {
        pri = SAL_THR_PRI_MAX;
    }
    else if (pri < SAL_THR_PRI_MIN)
    {
        pri = SAL_THR_PRI_MIN;
    }

    /* 优先级低的时候，配置为非实时线程*/
    if (pri == SAL_THR_PRI_MIN)
    {
        schedprm.sched_priority = 0;
        status |= pthread_setschedparam(hndl->hndl, SCHED_OTHER, &schedprm);
    }
    else
    {
        status |= pthread_getschedparam(hndl->hndl, &policy, &schedprm);
        schedprm.sched_priority = pri;
        status |= pthread_setschedparam(hndl->hndl, policy, &schedprm);
    }

    return status;
}

/***
 * @brief  线程CPU绑定
 * @param  [UINT32] uiCoreNum
 * @return [*]
 */
INT32 SAL_SetThreadCoreBind(UINT32 uiCoreNum)
{
    cpu_set_t mask;

    __CPU_ZERO_S(sizeof(cpu_set_t), &mask);
    __CPU_SET_S(uiCoreNum, sizeof(cpu_set_t), &mask);
    if (pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask) < 0)
    {
        SAL_LOGE("set_thread_core_bind. pthread_setaffinity_np faild\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   SAL_SetThreadCoreBindExt
 * @brief      绑核函数，支持绑定多颗核
 * @param[in]  UINT32 uiCoreNum    需要绑的核数目
 * @param[in]  UINT32 uiCoreId[8]  具体绑核的核ID
 * @param[out] None
 * @return     INT32
 */
INT32 SAL_SetThreadCoreBindExt(UINT32 uiCoreNum, UINT32 uiCoreId[8])
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


/***
 * @brief  线程退出
 * @param  [VOID] *returnVal
 * @return [*]
 */
INT32 SAL_thrExit(VOID *returnVal)
{
    pthread_exit(returnVal);
    return SAL_SOK;
}

