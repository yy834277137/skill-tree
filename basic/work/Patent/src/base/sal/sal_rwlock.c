
/**
 * @file    sal_rwlock.c
 * @note    Hangzhou Hikvision Digital Technology Co.,Ltd. All Right Reserved.
 * @brief   rwlock api
 */

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "sal.h"

extern void sal_tm_map_ms_2_timespec(IN const UINT64 wait_ms, OUT struct timespec *ts);


/**
 * @fn      SAL_RwlockInit
 * @brief   初始化读写锁，并设置写锁优先
 *
 * @param   rwlock_id[OUT] 读写锁ID
 *
 * @return  SAL_SOK-初始化读写锁成功，SAL_FAIL-初始化读写锁失败
 */
SAL_STATUS SAL_RwlockInit(pthread_rwlock_t *rwlock_id)
{
    int ret_val = 0;
    pthread_rwlockattr_t attr;

    if (NULL == rwlock_id)
    {
        SAL_LOGE("the 'rwlock_id' is NULL\n");
        return SAL_FAIL;
    }

    pthread_rwlockattr_init(&attr);

    /* 设置写锁优先 */
    ret_val = pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_rwlockattr_setkind_np failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
        goto ERR_EXIT;
    }

    #ifdef _POSIX_SHARED_MEMORY_OBJECTS
    ret_val = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_rwlockattr_setpshared failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
        goto ERR_EXIT;
    }

    #else
    #error UNSUPPORT 'PTHREAD_PROCESS_SHARED'
    #endif

    ret_val = pthread_rwlock_init(rwlock_id, &attr);
    if (0 == ret_val)
    {
        ret_val = SAL_SOK;
    }
    else
    {
        SAL_LOGE("pthread_rwlock_init failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
    }

ERR_EXIT:
    pthread_rwlockattr_destroy(&attr);

    return ret_val;
}

/**
 * @fn      SAL_RwlockRdlock
 * @brief   尝试在一定时间内对rdlock上锁
 *
 * @param   rwlock_id[IN] 读写锁ID
 * @param   wait_ms[IN] 等待上锁的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * @param   function[IN] 函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 *
 * @return  SAL_SOK-上锁成功，SAL_FAIL-上锁失败
 */
SAL_STATUS SAL_RwlockRdlock(pthread_rwlock_t *rwlock_id, UINT32 wait_ms, const CHAR *function, const UINT32 line)
{
    int ret_val = 0;
    struct timespec timeout = {0};
    UINT64 ts = 0, te = 0, tl = (UINT64)wait_ms;

    if (NULL == rwlock_id)
    {
        SAL_LOGE("the 'rwlock_id' is NULL\n");
        return SAL_FAIL;
    }

    if (SAL_TIMEOUT_NONE == wait_ms)
    {
        ret_val = pthread_rwlock_tryrdlock(rwlock_id);
    }
    else if (SAL_TIMEOUT_FOREVER == wait_ms)
    {
        ret_val = pthread_rwlock_rdlock(rwlock_id);
    }
    else
    {
        do
        {
            tl = tl + ts - te;
            ts = sal_get_tickcnt();
            sal_tm_map_ms_2_timespec(tl, &timeout);
            ret_val = pthread_rwlock_timedrdlock(rwlock_id, &timeout);
            if (ETIMEDOUT == ret_val) /* 如果时间到后，条件还没有发生，那么会返回ETIMEDOUT错误 */
            {
                te = sal_get_tickcnt();
            }
            else
            {
                break;
            }
        }
        while (ts + tl > te);
    }

    if (0 != ret_val)
    {
        if (ETIMEDOUT != ret_val)
        {
            SAL_LOGE("pthread_rwlock_rdlock failed, wait_ms: %u, errno: %d, %s\n", wait_ms, ret_val, strerror(ret_val));
        }

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      SAL_RwlockWrlock
 * @brief   尝试在一定时间内对wrlock上锁
 *
 * @param   rwlock_id[IN] 读写锁ID
 * @param   wait_ms[IN] 等待上锁的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * @param   function[IN] 函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 *
 * @return  SAL_SOK-上锁成功，SAL_FAIL-上锁失败
 */
SAL_STATUS SAL_RwlockWrlock(pthread_rwlock_t *rwlock_id, UINT32 wait_ms, const CHAR *function, const UINT32 line)
{
    int ret_val = 0;
    struct timespec timeout = {0};
    UINT64 ts = 0, te = 0, tl = (UINT64)wait_ms;

    if (NULL == rwlock_id)
    {
        SAL_LOGE("the 'rwlock_id' is NULL\n");
        return SAL_FAIL;
    }

    if (SAL_TIMEOUT_NONE == wait_ms)
    {
        ret_val = pthread_rwlock_trywrlock(rwlock_id);
    }
    else if (SAL_TIMEOUT_FOREVER == wait_ms)
    {
        ret_val = pthread_rwlock_wrlock(rwlock_id);
    }
    else
    {
        do
        {
            tl = tl + ts - te;
            ts = sal_get_tickcnt();
            sal_tm_map_ms_2_timespec(tl, &timeout);
            ret_val = pthread_rwlock_timedwrlock(rwlock_id, &timeout);
            if (ETIMEDOUT == ret_val) /* 如果时间到后，条件还没有发生，那么会返回ETIMEDOUT错误 */
            {
                te = sal_get_tickcnt();
            }
            else
            {
                break;
            }
        }
        while (ts + tl > te);
    }

    if (0 != ret_val)
    {
        if (ETIMEDOUT != ret_val)
        {
            SAL_LOGE("pthread_rwlock_wrlock failed, wait_ms: %u, errno: %d, %s\n", wait_ms, ret_val, strerror(ret_val));
        }

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      SAL_RwlockUnlock
 * @brief   对rwlock_id解锁
 *
 * @param   rwlock_id[IN] 读写锁ID
 *
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS SAL_RwlockUnlock(pthread_rwlock_t *rwlock_id)
{
    int ret_val = 0;

    if (NULL == rwlock_id)
    {
        SAL_LOGE("the 'rwlock_id' is NULL\n");
        return SAL_FAIL;
    }

    ret_val = pthread_rwlock_unlock(rwlock_id);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_rwlock_unlock failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      SAL_RwlockDestroy
 * @brief   销毁读写锁
 *
 * @param   rwlock_id[IN] 读写锁ID
 *
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS SAL_RwlockDestroy(pthread_rwlock_t *rwlock_id)
{
    int ret_val = 0;

    if (NULL == rwlock_id)
    {
        SAL_LOGE("the 'rwlock_id' is NULL\n");
        return SAL_FAIL;
    }

    ret_val = pthread_rwlock_destroy(rwlock_id);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_rwlock_unlock failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

