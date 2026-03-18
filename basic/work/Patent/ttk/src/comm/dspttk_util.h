
#ifndef _DSPTTK_UTIL_H_
#define _DSPTTK_UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "sal_type.h"
#include <pthread.h>
#include "dspcommon.h"
#include <semaphore.h>
#include <signal.h>


/**
 * @fn      dspttk_long_to_string
 * @brief   long类型转字符串，返回字符串首地址
 * 
 * @param   [IN] lVal long类型值
 * @param   [IN] pStrBuf 字符串Buffer
 * @param   [IN] u32BufSize Buffer大小，单位：字节
 * 
 * @return  CHAR* 转换后的字符串首地址
 */
CHAR *dspttk_long_to_string(long lVal, CHAR *pStrBuf, UINT32 u32BufSize);

/**
 * @fn      dspttk_float_to_string
 * @brief   float类型转字符串，返回字符串首地址
 * 
 * @param   [IN] fVal float类型值
 * @param   [IN] pStrBuf 字符串Buffer
 * @param   [IN] u32BufSize Buffer大小，单位：字节
 * 
 * @return  CHAR* 转换后的字符串首地址
 */
CHAR *dspttk_float_to_string(FLOAT32 fVal, CHAR *pStrBuf, UINT32 u32BufSize);

/**
 * @fn      dspttk_get_clock_ms
 * @brief   获取系统单调递增的时钟数，以ms计
 * 
 * @return  UINT64 时钟数，单位：毫秒
 */
UINT64 dspttk_get_clock_ms(void);

/**
 * @fn      dspttk_msleep
 * @brief   获取系统单调递增的时间。
 *
 * @param   msecTimeOut[IN] 超时时间，微秒值
 * @param   none[OUT] 无
 *
 * @return    SAL_SOK为获取成功 Success SAL_FAIL为获取失败
 */
UINT32 dspttk_msleep(INT32 msecTimeOut);

/**
 * @fn      dspttk_timer_create
 * @brief   创建指定触发频率的定时器，当定时器到期，以线程方式启动notify_function
 *
 * @param   [IN] u32Time 定时器的触发时间间隔，单位：us
 * @param   [IN] notify_function 定时器到期的回调函数
 * @param   [IN] sigval 定时器到期的回调函数输入参数
 *
 * @return  void* 定时器handle，实际类型为(timer_t *)
 */
void *dspttk_timer_create(UINT32 u32Time, void (*notify_function)(const INT32), INT32 sigval);

/**
 * @fn      dspttk_timer_set_time
 * @brief   设置定时器的触发时间间隔
 *
 * @param   [IN] pTimerHandle dspttk_timer_create()返回的定时器句柄
 * @param   [IN] u32Time 定时器的触发时间间隔，单位：us
 *
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_timer_set_time(void *pTimerHandle, UINT32 u32Time);

/**
 * @fn      dspttk_timer_destroy
 * @brief   销毁定时器
 * 
 * @param   [IN] pTimerHandle dspttk_timer_creat()返回的定时器句柄
 */
void dspttk_timer_destroy(void *pTimerHandle);

/**
 * @fn      dspttk_get_random
 * @brief   获取[s32Min, s32Max]范围内的随机数
 * 
 * @param   [IN] s32Min 随机数最小值
 * @param   [IN] s32Max 随机数最大值
 * 
 * @return  随机数
 */
INT32 dspttk_get_random(INT32 s32Min, INT32 s32Max);

/**
 * @fn      dspttk_array_pointers_alloc
 * @brief   指针数组内存申请，内部前部分为ArrCnt个为指针，每个指针执行后部分的元素
 * 
 * @param   [IN] u32ArrCnt 指针数组元素个数
 * @param   [IN] u32ElemSize 指针数组元素内容长度，单位：字节
 * 
 * @return  VOID** 内存地址
 */
VOID **dspttk_array_pointers_alloc(UINT32 u32ArrCnt, UINT32 u32ElemSize);

#ifdef __cplusplus
}
#endif

#endif /* _DSPTTK_UTIL_H_ */

