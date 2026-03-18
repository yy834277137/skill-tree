/***
 * @file   sal_sem_posix.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  本文件封装对linux下IPC的信号量的操作(posix标准)
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#include "sal.h"

/**
 * @function:   getTimespec
 * @brief:      超时时间ms转化为timespec格式
 * @param[in]:  INT32 wait_ms
 * @param[in]:  struct timespec *tp
 * @param[out]: None
 * @return:     static inline VOID
 */
static inline VOID getTimespec(INT32 wait_ms, struct timespec *tp)
{
    time_t sec, t;
    INT64L nsec;

    sec = 0;
    if (wait_ms == 0)
    {
        nsec = 0;
    }
    else if (wait_ms < 0)
    {
        nsec = (1000 * 10000) * 1000000LL; /* 特别大的值，直接修改为等10000s*/
    }
    else
    {
        nsec = wait_ms * 1000000LL;
    }

    if (clock_gettime(CLOCK_REALTIME, tp) == -1)
    {
        tp->tv_sec = time(NULL) + 1;
        tp->tv_nsec = 0;
    }
    else
    {
        t = time(NULL) + 1;
        if ((INT32)(tp->tv_sec - t) > 30)
        {
            tp->tv_sec = t;
            tp->tv_nsec = 0;
        }
    }

    nsec += tp->tv_nsec;
    if (nsec >= 1000000000)
    {
        sec = nsec / 1000000000;
        nsec = nsec % 1000000000;
    }

    tp->tv_sec += sec;
    tp->tv_nsec = nsec;

    return;
}

/**
 * @function:   SAL_SemCreate
 * @brief:      创建无名信号量
 * @param[in]:  BOOL bShared
 * @param[in]:  UINT32 uiInitCount
 * @param[out]: None
 * @return:     VOID *
 */
VOID *SAL_SemCreate(BOOL bShared, UINT32 uiInitCount)
{
    VOID *pSem = NULL;

    SAL_UNUSED(bShared);   /* 暂不开启进程间共享信号量，默认进程内线程间共享 */

    pSem = SAL_memMalloc(sizeof(sem_t), "sem_posix", "sem_t");
    if (!pSem)
    {
        return NULL;
    }

    memset(pSem, 0x00, sizeof(sem_t));

    if (sem_init((sem_t *)pSem, 0, uiInitCount) != 0)
    {
        SAL_memfree(pSem, "sem_posix", "sem_t");
        return NULL;
    }

    return pSem;
}

/**
 * @function:   SAL_SemDestroy
 * @brief:      销毁无名信号量
 * @param[in]:  VOID *pSem
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemDestroy(VOID *pSem)
{
    if (NULL == pSem)
    {
        return SAL_FAIL;
    }

    if (sem_destroy(pSem) != 0)
    {
        return SAL_FAIL;
    }

    SAL_memfree(pSem, "sem_posix", "sem_t");

    return SAL_SOK;
}

/**
 * @function:   SAL_SemWait
 * @brief:      信号量阻塞获取
 * @param[in]:  VOID *pSem
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemWait(VOID *pSem)
{
    if (NULL == pSem)
    {
        return SAL_FAIL;
    }

    if (sem_wait(pSem) != 0)
    {
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_SemTimeWait
 * @brief:      信号量非阻塞获取
 * @param[in]:  VOID *pSem
 * @param[in]:  UINT32 uiTimeOutMs
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemTimeWait(VOID *pSem, UINT32 uiTimeOutMs)
{
    struct timespec timeout;

    if (NULL == pSem)
    {
        return SAL_FAIL;
    }

    if (uiTimeOutMs == SEM_WAIT_NONE)
    {
        if (sem_trywait(pSem) != 0)
        {
            return SAL_FAIL;
        }
    }
    else
    {
        getTimespec(uiTimeOutMs, &timeout);
        if (sem_timedwait(pSem, &timeout) != 0)
        {
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_SemPost
 * @brief:      信号量+1
 * @param[in]:  VOID *pSem
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemPost(VOID *pSem)
{
    if (NULL == pSem)
    {
        return SAL_FAIL;
    }

    return (sem_post(pSem) != 0) ? SAL_FAIL : SAL_SOK;
}

/**
 * @function:   SAL_SemGetValue
 * @brief:      获取当前信号量的计数值
 * @param[in]:  VOID *pSem
 * @param[in]:  INT32 *pValue
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemGetValue(VOID *pSem, INT32 *pValue)
{
    if (NULL == pSem || NULL == pValue)
    {
        return SAL_FAIL;
    }

    return (sem_getvalue(pSem, pValue) == 0) ? SAL_SOK : SAL_FAIL;
}

