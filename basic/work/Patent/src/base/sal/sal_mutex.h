/***
 * @file   sal_mutex.h
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

#ifndef _SAL_MUTEX_H_
#define _SAL_MUTEX_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

/*定义互斥锁的类型*/
typedef enum
{
    SAL_MUTEX_NORMAL,       /* 互斥锁默认属性，不支持递归，不进行死锁检测，且有未定义行为，不推荐使用 */
    SAL_MUTEX_ERRORCHECK,   /* 不支持递归，但重复加锁，重复解锁都会返回错误代码，不会导致死锁，推荐使用 */
    SAL_MUTEX_RECURSIVE,    /* 支持递归，即同一线程可加锁多次，当然也要解锁多次，清楚你在干什么并谨慎使用 */
    SAL_MUTEX_TYPE_MIN = INT32_MIN,
    SAL_MUTEX_TYPE_MAX = INT32_MAX
} SAL_MUTEX_TYPE;


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */
/*定义互斥锁对象*/
typedef struct
{
    pthread_mutex_t lock;                /*用户态互斥锁变量*/
    pthread_cond_t cond;
    pthread_condattr_t cattr;
} SAL_MutexObject;

/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : SAL_mutexCreate
* 描  述  : 该函数负责创建一个互斥锁
* 输  入  : - type   : 互斥锁类型,参见 SAL_MutexType定义
*         : - phMutex: 互斥锁句柄指针,当创建成功时输出互斥锁句柄
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 SAL_mutexCreate(SAL_MUTEX_TYPE type, Handle *phMutex);

/*******************************************************************************
* 函数名  : SAL_mutexLock
* 描  述  : 互斥锁加锁
* 输  入  : - hMutex: 互斥锁句柄,
* 输  出  : 无
* 返回值  : HIK_SOK:   成功
*           HIK_FAIL: 失败
*******************************************************************************/
INT32 SAL_mutexLock(Handle hMutex);

/*******************************************************************************
* 函数名  : SAL_mutexUnlock
* 描  述  : 互斥锁解锁
*           该接口不能在内核态中断上下文调用
* 输  入  : - hMutex: 互斥锁句柄,
* 输  出  : 无
* 返回值  : HIK_SOK:   成功
*           HIK_FAIL: 失败
*******************************************************************************/
INT32 SAL_mutexUnlock(Handle hMutex);

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
INT32 SAL_mutexTimeOutWait(Handle hMutex, INT32 msecTimeOut);

/*******************************************************************************
* 函数名  : SAL_mutexWait
* 描  述  : 阻塞等待接收信号才能退出，外部需要管理好SAL_mutexLock/SAL_mutexUnlock的使用
* 输  入  : - hMutex: 互斥锁句柄
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 SAL_mutexWait(Handle hMutex);

/*******************************************************************************
* 函数名  : SAL_mutexSignal
* 描  述  : 给 SAL_mutexWait 发送退出信号
* 输  入  : - hMutex: 互斥锁句柄
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
INT32 SAL_mutexSignal(Handle hMutex);

/*******************************************************************************
* 函数名  : SAL_mutexDelete
* 描  述  : 该函数负责销毁一个互斥锁
*           该接口不能在内核态中断上下文调用
* 输  入  : - hMutex: 互斥锁句柄
* 输  出  : 无。
* 返回值  : HIK_SOK:   成功
*           HIK_FAIL: 失败
*******************************************************************************/
INT32 SAL_mutexDelete(Handle hMutex);


/**
 * @fn      SAL_mutexTmInit
 * @brief   初始化互斥锁
 *
 * @param   mutex_id[OUT] 互斥锁ID
 * @param   mutex_type[IN] 互斥锁类型，推荐使用SAL_MUTEX_ERRORCHECK与SAL_MUTEX_RECURSIVE
 *
 * @return  SAL_SOK-初始化互斥锁成功，SAL_FAIL-初始化互斥锁失败
 */
SAL_STATUS SAL_mutexTmInit(pthread_mutex_t *mutex_id, SAL_MUTEX_TYPE mutex_type);


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
SAL_STATUS SAL_mutexTmLock(pthread_mutex_t *mutex_id, UINT32 wait_ms, const CHAR *function, const UINT32 line);

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
SAL_STATUS SAL_mutexTmUnlock(pthread_mutex_t *mutex_id, const CHAR *function, const UINT32 line);

/**
 * @fn      mutexDestroy
 * @brief   销毁互斥锁
 *
 * @param   mutex_id[IN] 互斥锁ID
 *
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS SAL_mutexTmDestroy(pthread_mutex_t *mutex_id);

#endif

