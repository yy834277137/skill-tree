/***
 * @file   sal_cond.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  条件变量头文件
 * @author liwenbin
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef __SAL_COND_H__
#define __SAL_COND_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include "sal_type.h"


typedef enum
{
    SAL_COND_ST_BROADCAST = 0,      /* 广播条件状态的改变，以唤醒等待该条件的所有线程 */
    SAL_COND_ST_SINGLE = 1,         /* 只会唤醒等待该条件的某个线程 */
    SAL_COND_ST_MIN	= INT32_MIN,
    SAL_COND_ST_MAX	= INT32_MAX
} SAL_COND_SIGNAL_TYPE;

typedef struct
{
    pthread_mutex_t mid;    /* 互斥锁ID */
    pthread_cond_t cid;     /* 条件变量ID */
} COND_T;

/**
 * @note condWait使用说明：
 * mutexLock(&cond_s->mid, FOREVER); // 加锁
 * while (等待条件) // 比如等待条件为队列空
 * {
 *     condWait(cond_s, wait_ms, __FUNCTION__, __LINE__);
 * }
 * TODO: 对变量（一般是全局变量）进行操作 // 比如从队列中获取并删除一个元素
 * mutexUnlock(&cond_s->mid); // 解锁
 */

/**
 * @note condSignal使用说明：
 * mutexLock(&cond_s->mid, FOREVER); // 加锁
 * TODO: 对变量（一般是全局变量）进行操作 // 比如向队列中插入一个元素
 * condSignal(cond_s, signal_type, __FUNCTION__, __LINE__);
 * mutexUnlock(&cond_s->mid); // 解锁
 */

/**
 * @fn      SAL_CondInit
 * @brief   初始化条件变量
 *
 * @param   cond_s[OUT] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 *
 * @return  SAL_SOK-初始化成功，SAL_FAIL-初始化失败
 */
SAL_STATUS SAL_CondInit(COND_T *cond_s);

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
SAL_STATUS SAL_CondWait(COND_T *cond_s, UINT32 wait_ms, const CHAR *function, const UINT32 line);

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
SAL_STATUS SAL_CondSignal(COND_T *cond_s, SAL_COND_SIGNAL_TYPE signal_type, const CHAR *function, const UINT32 line);

/**
 * @fn      SAL_CondDestroy
 * @brief   销毁条件变量
 *
 * @param   cond_s[IN] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 *
 * @return  SAL_SOK-销毁成功，SAL_FAIL-销毁失败
 */
SAL_STATUS SAL_CondDestroy(COND_T *cond_s);

#ifdef __cplusplus
}
#endif

#endif /* __SAL_COND_H__ */

