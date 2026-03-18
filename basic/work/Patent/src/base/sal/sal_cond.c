/***
 * @file   sal_cond.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  条件变量
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "sal.h"

/**
 * @fn      SAL_CondInit
 * @brief   初始化条件变量
 *
 * @param   cond_s[OUT] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 *
 * @return  SAL_SOK-初始化成功，SAL_FAIL-初始化失败
 */
SAL_STATUS SAL_CondInit(COND_T *cond_s)
{
    INT32 ret_val = 0;
    pthread_condattr_t attr;

    if (NULL == cond_s)
    {
        SAL_LOGE("cond_s is NULL\n");
        return SAL_FAIL;
    }

    ret_val = SAL_mutexTmInit(&cond_s->mid, SAL_MUTEX_NORMAL);
    if (SAL_SOK != ret_val)
    {
        SAL_LOGE("init mutex in COND_T failed\n");
        return SAL_FAIL;
    }

    ret_val = pthread_condattr_init(&attr);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_condattr_init failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    ret_val = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_condattr_setclock failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        pthread_condattr_destroy(&attr);
        return SAL_FAIL;
    }

    #ifdef _POSIX_SHARED_MEMORY_OBJECTS
    ret_val = pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_condattr_setpshared failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        pthread_condattr_destroy(&attr);
        return SAL_FAIL;
    }

    #else
    #error UNSUPPORT 'PTHREAD_PROCESS_SHARED'
    #endif

    ret_val = pthread_cond_init(&cond_s->cid, &attr);
    if (0 != ret_val)
    {
        SAL_LOGE("pthread_cond_init failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
    }
    else
    {
        ret_val = SAL_SOK;
    }

    pthread_condattr_destroy(&attr);

    return ret_val;
}

/**
 * @fn      SAL_CondWait
 * @brief   在一定时间内等待条件变为真
 * @warning SAL_CondWaitt成功返回时，线程需要重新计算条件，因为其他线程可能在运行过程中已经改变条件
 *
 * @param   cond_s[IN] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 * @param   wait_ms[IN] 等待条件变为真的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，其他表示时间
 * @param   function[IN] 函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 *
 * @return  SAL_SOK-等待条件变为真成功，SAL_FAIL-等待条件变成真失败
 */
SAL_STATUS SAL_CondWait(COND_T *cond_s, UINT32 wait_ms, const CHAR *function, const UINT32 line)
{
    INT32 ret_val = 0;
    struct timespec timeout;

    if (NULL == cond_s)
    {
        SAL_LOGE("the 'cond_s' is NULL\n");
        return SAL_FAIL;
    }

    /**
     * 调用pthread_cond_wait()函数后，会原子的执行以下两个动作：
     * ①将调用线程放到等待条件的线程列表上，即进入睡眠；
     * ②对互斥量进行解锁
     * 由于这两个操作时原子操作，这样就关闭了条件检查和线程进入睡眠等待条件改变这两个操作之间的时间通道
     * 当pthread_cond_wait()返回后，互斥量会再次被锁住。
     */
    if (wait_ms == SAL_TIMEOUT_FOREVER)
    {
        ret_val = pthread_cond_wait(&cond_s->cid, &cond_s->mid);
    }
    else
    {
        ret_val = clock_gettime(CLOCK_MONOTONIC, &timeout);
        if (0 != ret_val)
        {
            SAL_LOGE("clock_gettime failed, errno: %d, %s\n", errno, strerror(errno));
            return SAL_FAIL;
        }

        timeout.tv_sec += wait_ms / 1000;
        timeout.tv_nsec += (wait_ms % 1000) * 1000000;
        if (timeout.tv_nsec > 999999999)
        {
            timeout.tv_sec++;
            timeout.tv_nsec -= 1000000000;
        }

        ret_val = pthread_cond_timedwait(&cond_s->cid, &cond_s->mid, &timeout);
    }

    if (0 != ret_val)
    {
        if (ETIMEDOUT == ret_val)
        {
            if (NULL != function)
            {
                SAL_LOGD("pthread_cond_timedwait TIMEOUT @ FUNCTION[%s]-LINE[%u], wait_ms: %u\n", \
                         function, line, wait_ms);
            }
            else
            {
                SAL_LOGD("pthread_cond_timedwait TIMEOUT, wait_ms: %d\n", wait_ms);
            }
        }
        else
        {
            if (NULL != function)
            {
                SAL_LOGE("pthread_cond_wait failed @ FUNCTION[%s]-LINE[%u], wait_ms: %u, errno: %d, %s\n", \
                         function, line, wait_ms, ret_val, strerror(ret_val));
            }
            else
            {
                SAL_LOGE("pthread_cond_wait failed, wait_ms: %u, errno: %d, %s\n", wait_ms, ret_val, strerror(ret_val));
            }
        }

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      SAL_CondSignal
 * @brief   向等待条件的线程发送唤醒信号
 *
 * @param   cond_s[IN] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 * @param   signal_type[IN] 唤醒信号类型：
 *                          SAL_COND_ST_BROADCAST-广播条件状态的改变，以唤醒等待该条件的所有线程
 *                          SAL_COND_ST_SINGLE-只会唤醒等待该条件的某个线程
 *          注：只有在等待者代码编写确切，只有一个等待者需要唤醒，且唤醒哪个线程无所谓，
 *              那么此时为这种情况使用单播，所以其他情况下都必须使用广播发送。
 * @param   function[IN] 函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 *
 * @return  SAL_SOK-发送唤醒信号成功，SAL_FAIL-发送唤醒信号失败
 */
SAL_STATUS SAL_CondSignal(COND_T *cond_s, SAL_COND_SIGNAL_TYPE signal_type, const CHAR *function, const UINT32 line)
{
    INT32 ret_val = 0;

    if (NULL == cond_s)
    {
        SAL_LOGE("the 'cond_s' is NULL\n");
        return SAL_FAIL;
    }

    if (SAL_COND_ST_SINGLE == signal_type)
    {
        ret_val = pthread_cond_signal(&cond_s->cid);
    }
    else
    {
        ret_val = pthread_cond_broadcast(&cond_s->cid);
    }

    if (0 != ret_val)
    {
        if (NULL != function)
        {
            SAL_LOGE("pthread_cond_signal failed @ FUNCTION[%s]-LINE[%u], signal_type: %d, errno: %d, %s\n", \
                     function, line, signal_type, ret_val, strerror(ret_val));
        }
        else
        {
            SAL_LOGE("pthread_cond_signal failed, signal_type: %d, errno: %d, %s\n", signal_type, ret_val, strerror(ret_val));
        }

        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      SAL_CondDestroy
 * @brief   销毁条件变量
 *
 * @param   cond_s[IN] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 *
 * @return  SAL_SOK-销毁成功，SAL_FAIL-销毁失败
 */
SAL_STATUS SAL_CondDestroy(COND_T *cond_s)
{
    INT32 ret_val_cond = 0;
    INT32 ret_val_mutex = 0;

    if (NULL == cond_s)
    {
        SAL_LOGE("the 'cond_s' is NULL\n");
        return SAL_FAIL;
    }

    ret_val_cond = pthread_cond_destroy(&cond_s->cid);
    if (0 != ret_val_cond)
    {
        SAL_LOGE("pthread_cond_destroy failed, errno: %d, %s\n", ret_val_cond, strerror(ret_val_cond));
        ret_val_cond = SAL_FAIL;
    }
    else
    {
        ret_val_cond = SAL_SOK;
    }

    ret_val_mutex = SAL_mutexTmDestroy(&cond_s->mid);
    if (SAL_SOK != ret_val_mutex)
    {
        SAL_LOGE("destroy mutex in COND_T failed\n");
    }

    return (ret_val_mutex | ret_val_cond);
}

