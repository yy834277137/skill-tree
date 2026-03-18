/***
 * @file   sal_rwlock.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  读写锁
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef __SAL_RWLOCK_H__
#define __SAL_RWLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>

/**
 * @fn      SAL_RwlockInit
 * @brief   初始化读写锁，并设置写锁优先
 *
 * @param   rwlock_id[OUT] 读写锁ID
 *
 * @return  SAL_SOK-初始化读写锁成功，SAL_FAIL-初始化读写锁失败
 */
SAL_STATUS SAL_RwlockInit(pthread_rwlock_t *rwlock_id);

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
SAL_STATUS SAL_RwlockRdlock(pthread_rwlock_t *rwlock_id, UINT32 wait_ms, const CHAR *function, const UINT32 line);

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
SAL_STATUS SAL_RwlockWrlock(pthread_rwlock_t *rwlock_id, UINT32 wait_ms, const CHAR *function, const UINT32 line);

/**
 * @fn      SAL_RwlockUnlock
 * @brief   对rwlock_id解锁
 *
 * @param   rwlock_id[IN] 读写锁ID
 *
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS SAL_RwlockUnlock(pthread_rwlock_t *rwlock_id);

/**
 * @fn      SAL_RwlockDestroy
 * @brief   销毁读写锁
 *
 * @param   rwlock_id[IN] 读写锁ID
 *
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS SAL_RwlockDestroy(pthread_rwlock_t *rwlock_id);

#ifdef __cplusplus
}
#endif

#endif /* __SAL_RWLOCK_H__ */

