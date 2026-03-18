
#ifndef __SAL_TIMER_H__
#define __SAL_TIMER_H__

#ifdef __cplusplus
extern  "C" {
#endif

#include "sal.h"
#include <sys/ioctl.h>


#define TIMER_DEV		"/dev/dsp_timer"
#define TIMER_CREATE		_IOW('p', 0x100, INT32)
#define TIMER_SET_HZ		_IOW('p', 0x101, UINT32)

typedef void (*sal_timer_notify_function)(INT32);

/**
 * @fn      SAL_TimerHpInit
 * @brief   初始化高性能定时器句柄
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：初始化失败
 */
SAL_STATUS SAL_TimerHpInit(void);

/**
 * @fn      SAL_TimerHpCreate
 * @brief   创建指定触发频率的高精度定时器
 * 
 * @param   [IN] hz 每秒触发次数
 * 
 * @return  INT32 >=0：高精度定时器句柄，其他：创建失败
 */
INT32 SAL_TimerHpCreate(UINT32 hz);

/**
 * @fn      SAL_TimerHpSetHz
 * @brief   设置高精度定时器的触发频率
 * 
 * @param   [IN] fd SAL_TimerHpCreate()返回的高精度定时器句柄
 * @param   [IN] hz 每秒触发次数
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS SAL_TimerHpSetHz(INT32 fd, UINT32 hz);

/**
 * @fn      SAL_TimerHpWait
 * @brief   等待高精度定时器触发
 * 
 * @param   [IN] fd SAL_TimerHpCreate()返回的高精度定时器句柄
 * 
 * @return  SAL_STATUS SAL_SOK：成功，SAL_FAIL：失败
 */
SAL_STATUS SAL_TimerHpWait(INT32 fd);

/**
 * @fn      SAL_TimerCreate
 * @brief   创建指定触发频率的定时器，当定时器到期，以线程方式启动notify_function
 * 
 * @param   [IN] itUs 定时器触发间隔时间，单位：us
 * @param   [IN] loopCnt 循环次数，0xFFFFFFFF表示无限循环 
 * @param   [IN] notify_function 定时器到期的回调函数
 * @param   [IN] sigval 定时器到期的回调函数输入参数
 *  
 * @return  void* 定时器handle，实际类型为(timer_t *)
 */
void *SAL_TimerCreate(UINT32 itUs, UINT32 loopCnt, void (*notify_function)(const INT32), INT32 sigval);

/**
 * @fn      SAL_TimerSetIt
 * @brief   设置定时器的触发时间间隔
 * 
 * @param   [IN] pTimerHandle SAL_TimerCreate()返回的定时器句柄
 * @param   [IN] itUs 定时器触发间隔时间，单位：us
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS SAL_TimerSetIt(void *pTimerHandle, UINT32 itUs);

/**
 * @fn      SAL_TimerDestroy
 * @brief   销毁定时器
 * 
 * @param   [IN] pTimerHandle SAL_TimerCreate()返回的定时器句柄
 */
void SAL_TimerDestroy(void *pTimerHandle);


#ifdef __cplusplus
}
#endif

#endif

