/***
 * @file   sal_mutex.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  软件抽象层模块对外提供的互斥锁接口，使用流程如下:
           ==========================
 |
           SAL_mutexCreate
 |
           SAL_mutexLock/SAL_mutexUnlock/SAL_mutexWait/SAL_mutexSignal
 |
           SAL_mutexDelete
           ===========================

           注意:
                SAL_MUTEX_NORMAL 为常见的互斥锁，同一线程不能加锁多次
                SAL_MUTEX_RECURSIVE 为支持嵌套锁的互斥锁，同一线程可以循环加锁多次，小心使用，避免死锁
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <sal.h>
/* #include "platform_hal.h" */

#line __LINE__ "sal_mutex.c"

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
extern void sal_tm_map_ms_2_timespec(IN const UINT64 wait_ms, OUT struct timespec *ts);

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/***
 * @brief  该函数负责创建一个互斥锁
 * @param  [SAL_MUTEX_TYPE] type 互斥锁类型,参见 SAL_MutexType定义
 * @param  [Handle] *phMutex     互斥锁句柄指针,当创建成功时输出互斥锁句柄
 * @return [*] HIK_SOK  : 成功
 *           HIK_FAIL : 失败
 */
INT32 SAL_mutexCreate(SAL_MUTEX_TYPE type, Handle *phMutex)
{
    SAL_MutexObject *pMutexObj = NULL;
    pthread_mutexattr_t mutex_attr;

    if (NULL == phMutex)
    {
        SAL_LOGE("Null phMutex\n");
        return SAL_FAIL;
    }

    /*分配内存*/
    pMutexObj = (SAL_MutexObject *)SAL_memMalloc(sizeof(SAL_MutexObject), "base", "sal_mutex");
    if (NULL == pMutexObj)
    {
        SAL_LOGE("alloc mutex failed\n");
        return SAL_FAIL;
    }

    if (SAL_MUTEX_RECURSIVE == type)
    {
        /* 设置锁支持嵌套，需要在makfile中添加_D_GNU_SOURCE。*/
        pthread_mutexattr_init(&mutex_attr);
        pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&pMutexObj->lock, &mutex_attr);

        pthread_condattr_init(&pMutexObj->cattr);
        pthread_condattr_setclock(&pMutexObj->cattr, CLOCK_MONOTONIC);
        pthread_cond_init(&pMutexObj->cond, &pMutexObj->cattr);

        pthread_mutexattr_destroy(&mutex_attr);
    }
    else
    {
        pthread_condattr_init(&pMutexObj->cattr);
        pthread_condattr_setclock(&pMutexObj->cattr, CLOCK_MONOTONIC);

        pthread_mutex_init(&pMutexObj->lock, NULL);
        pthread_cond_init(&pMutexObj->cond, &pMutexObj->cattr);
    }

    SAL_LOGD("mutex pMutexObj %p\n", pMutexObj);

    *phMutex = (Handle)pMutexObj;
    return SAL_SOK;
}

/***
 * @brief  互斥锁加锁
 * @param  [Handle] hMutex 互斥锁句柄
 * @return [*] HIK_SOK:   成功
 *           HIK_FAIL: 失败
 */
INT32 SAL_mutexLock(Handle hMutex)
{
    SAL_MutexObject *pMutexObj = (SAL_MutexObject *)hMutex;
    INT32 ret;

    /*检查handle有效性*/
    if (NULL == (pMutexObj))
    {
        SAL_LOGE("NULL handle\n");
        return SAL_FAIL;
    }

    ret = pthread_mutex_lock(&pMutexObj->lock);
    if (0 != ret)
    {
        SAL_LOGE("mutex failed pMutexObj %p\n", pMutexObj);
    }

    return (0 == ret) ? SAL_SOK : SAL_FAIL;
}

/***
 * @brief  互斥锁解锁
 *         该接口不能在内核态中断上下文调用
 * @param  [Handle] hMutex 互斥锁句柄
 * @return [*]HIK_SOK:   成功
 *           HIK_FAIL: 失败
 */
INT32 SAL_mutexUnlock(Handle hMutex)
{
    SAL_MutexObject *pMutexObj = (SAL_MutexObject *)hMutex;
    INT32 ret;

    /*检查handle有效性*/
    if (NULL == (pMutexObj))
    {
        SAL_LOGE("NULL handle\n");
        return SAL_FAIL;
    }

    ret = pthread_mutex_unlock(&pMutexObj->lock);
    if (0 != ret)
    {
        SAL_LOGE("mutex failed pMutexObj %p\n", pMutexObj);
    }

    return (0 == ret) ? SAL_SOK : SAL_FAIL;
}

/*******************************************************************************
* 函数名  : SAL_mutexTimeOutWait
* 描  述  : 等待信号解锁和等待超时，需要处理返回值
* 输  入  : - hMutex     : 互斥锁句柄
*         : - msecTimeOut: 毫秒的超时时间
* 输  出  : 无
* 返回值  : SAL_ETIMEOUT : 超时返回
*           SAL_SOK      : 接收到信号正常返回
*           SAL_FAIL     : 异常返回
*******************************************************************************/
INT32 SAL_mutexTimeOutWait(Handle hMutex, INT32 msecTimeOut)
{
    SAL_MutexObject *pMutexObj = (SAL_MutexObject *)hMutex;
    INT32 ret;
    struct timespec timeToWait, timespec1;

    /*检查handle有效性*/
    if (NULL == (pMutexObj))
    {
        SAL_LOGE("NULL handle\n");
        return SAL_FAIL;
    }

    clock_gettime(CLOCK_MONOTONIC, &timespec1);
    timeToWait.tv_nsec = timespec1.tv_nsec + (msecTimeOut % 1000) * 1000L * 1000L;
    timeToWait.tv_sec = timespec1.tv_sec + msecTimeOut / 1000 + timeToWait.tv_nsec / 1000000000L;
    timeToWait.tv_nsec = timeToWait.tv_nsec % 1000000000L;

    ret = pthread_cond_timedwait(&pMutexObj->cond, &pMutexObj->lock, &timeToWait);
    /* 超时返回 */
    if (ret == ETIMEDOUT)
    {
        return SAL_ETIMEOUT;
    }

    /* 接收到信号正常返回 */
    if (ret == 0)
    {
        return SAL_SOK;
    }

    /* 异常返回 */
    return SAL_FAIL;
}

/***
 * @brief  阻塞等待接收信号才能退出，外部需要管理好SAL_mutexLock/SAL_mutexUnlock的使用
 * @param  [Handle] hMutex 互斥锁句柄
 * @return [*]HIK_SOK  : 成功
 *           HIK_FAIL : 失败
 */
INT32 SAL_mutexWait(Handle hMutex)
{
    SAL_MutexObject *pMutexObj = (SAL_MutexObject *)hMutex;
    INT32 ret;

    /*检查handle有效性*/
    if (NULL == (pMutexObj))
    {
        SAL_LOGE("NULL handle\n");
        return SAL_FAIL;
    }

    ret = pthread_cond_wait(&pMutexObj->cond, &pMutexObj->lock);
    return ret;
}

/***
 * @brief  给 SAL_mutexWait 发送退出信号
 * @param  [Handle] hMutex 互斥锁句柄
 * @return [*]HIK_SOK  : 成功
 *           HIK_FAIL : 失败
 */
INT32 SAL_mutexSignal(Handle hMutex)
{
    SAL_MutexObject *pMutexObj = (SAL_MutexObject *)hMutex;
    INT32 ret;

    /*检查handle有效性*/
    if (NULL == (pMutexObj))
    {
        SAL_LOGE("NULL handle\n");
        return SAL_FAIL;
    }

    ret = pthread_cond_signal(&pMutexObj->cond);
    return ret;
}

/***
 * @brief  该函数负责销毁一个互斥锁
 *         该接口不能在内核态中断上下文调用
 * @param  [Handle] hMutex 互斥锁句柄
 * @return [*]HIK_SOK:   成功
 *           HIK_FAIL: 失败
 */
INT32 SAL_mutexDelete(Handle hMutex)
{
    SAL_MutexObject *pMutexObj = (SAL_MutexObject *)hMutex;

    if (NULL == (pMutexObj))
    {
        SAL_LOGE("NULL handle\n");
        return SAL_FAIL;
    }

    pthread_cond_destroy(&pMutexObj->cond);
    pthread_mutex_destroy(&pMutexObj->lock);

    SAL_memfree(pMutexObj, "base", "sal_mutex");
    return SAL_SOK;
}

/**
 * @fn      SAL_mutexTmInit
 * @brief   初始化互斥锁
 *
 * @param   mutex_id[OUT] 互斥锁ID
 * @param   mutex_type[IN] 互斥锁类型，推荐使用SAL_MUTEX_ERRORCHECK与SAL_MUTEX_RECURSIVE
 *
 * @return  SAL_SOK-初始化互斥锁成功，SAL_FAIL-初始化互斥锁失败
 */
SAL_STATUS SAL_mutexTmInit(pthread_mutex_t *mutex_id, SAL_MUTEX_TYPE mutex_type)
{
    INT32 ret_val = 0;
    pthread_mutexattr_t attr;

    if (NULL == mutex_id)
    {
        SAL_LOGE("the 'mutex_id' is NULL\n");
        return SAL_FAIL;
    }

    /**
     * 互斥锁各类型的区别：
     * PTHREAD_MUTEX_NORMAL：不进行死锁检测，同一线程企图对已上锁的mutex进行relock会导致死锁，企图对未上锁的mutex进行unlock结果不可知
     * PTHREAD_MUTEX_ERRORCHECK：同一线程企图对已上锁的mutex进行relock，或企图对未上锁的mutex进行unlock， 均返回错误
     * PTHREAD_MUTEX_RECURSIVE：允许同一线程对同一锁成功获得多次，当然也要解锁多次。其余线程在解锁时重新竞争。
     * PTHREAD_MUTEX_DEFAULT：企图递归获取这个mutex结果是不确定的，企图unlock一个未被锁住的mutex结果也是不确定的
     */
    pthread_mutexattr_init(&attr);
    if (SAL_MUTEX_ERRORCHECK == mutex_type)
    {
        ret_val = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    }
    else if (SAL_MUTEX_RECURSIVE == mutex_type)
    {
        ret_val = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    }
    else /* 其他均设置为默认属性 */
    {
        ret_val = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);
    }

    if (0 != ret_val)
    {
        SAL_LOGE("pthread_mutexattr_settype failed, mutex_type: %d, errno: %d, %s\n", mutex_type, ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
        goto ERR_EXIT;
    }

    #ifdef _POSIX_SHARED_MEMORY_OBJECTS
    ret_val = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_mutexattr_setpshared failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
        goto ERR_EXIT;
    }

    #else
    #error UNSUPPORT 'PTHREAD_PROCESS_SHARED'
    #endif

    ret_val = pthread_mutex_init(mutex_id, &attr);
    if (0 == ret_val)
    {
        ret_val = SAL_SOK;
    }
    else
    {
        SAL_LOGE("pthread_mutex_init failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
    }

ERR_EXIT:
    pthread_mutexattr_destroy(&attr);

    return ret_val;
}

/**
 * @fn      SAL_mutexTmLock
 * @brief   尝试在一定时间内对mutex_id上锁
 *
 * @param   mutex_id[IN] 互斥锁ID
 * @param   wait_ms[IN] 等待上锁的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * @param   function[IN] 函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 *
 * @return  SAL_SOK-上锁成功，SAL_FAIL-上锁失败
 */
SAL_STATUS SAL_mutexTmLock(pthread_mutex_t *mutex_id, UINT32 wait_ms, const CHAR *function, const UINT32 line)
{
    INT32 ret_val = 0;
    struct timespec timeout = {0};
    UINT64 ts = 0, te = 0, tl = (UINT64)wait_ms;

    if (NULL == mutex_id)
    {
        SAL_LOGE("the 'mutex_id' is NULL, func:%s|%u\n", function, line);
        return SAL_FAIL;
    }

    if (wait_ms == SAL_TIMEOUT_NONE)
    {
        ret_val = pthread_mutex_trylock(mutex_id);
    }
    else if (wait_ms == SAL_TIMEOUT_FOREVER)
    {
        ret_val = pthread_mutex_lock(mutex_id);
    }
    else
    {
        do
        {
            tl = tl + ts - te;
            ts = sal_get_tickcnt();
            sal_tm_map_ms_2_timespec(tl, &timeout);
            ret_val = pthread_mutex_timedlock(mutex_id, &timeout);
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

        SAL_LOGE("pthread_mutex_lock failed, wait_ms: %u, errno: %d, %s, func:%s|%u\n", wait_ms, ret_val, strerror(ret_val), function, line);

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      SAL_mutexTmUnlock
 * @brief   对mutex_id解锁
 *
 * @param   mutex_id[IN] 互斥锁ID
 * @param   function[IN] 函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 *
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS SAL_mutexTmUnlock(pthread_mutex_t *mutex_id, const CHAR *function, const UINT32 line)
{
    INT32 ret_val = 0;

    if (NULL == mutex_id)
    {
        SAL_LOGE("the 'mutex_id' is NULL\n");
        return SAL_FAIL;
    }

    ret_val = pthread_mutex_unlock(mutex_id);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_mutex_unlock failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      mutexDestroy
 * @brief   销毁互斥锁
 *
 * @param   mutex_id[IN] 互斥锁ID
 *
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS SAL_mutexTmDestroy(pthread_mutex_t *mutex_id)
{
    int	ret_val = 0;

    if (NULL == mutex_id)
    {
        SAL_LOGE("the 'mutex_id' is NULL\n");
        return SAL_FAIL;
    }

    ret_val = pthread_mutex_destroy(mutex_id);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_mutex_unlock failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

