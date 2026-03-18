
/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include <stdlib.h>
#include <string.h>
#include "sal_type.h"
#include "sal_macro.h"
#include "prt_trace.h"
#include "dspttk_util.h"
#include <errno.h>
#include <time.h>
#include <sys/prctl.h>

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */

/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */

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
CHAR *dspttk_long_to_string(long lVal, CHAR *pStrBuf, UINT32 u32BufSize)
{
    if (NULL != pStrBuf && u32BufSize > 0)
    {
        snprintf(pStrBuf, u32BufSize, "%ld", lVal);
    }

    return pStrBuf;
}


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
CHAR *dspttk_float_to_string(FLOAT32 fVal, CHAR *pStrBuf, UINT32 u32BufSize)
{
    if (NULL != pStrBuf && u32BufSize > 0)
    {
        if ((FLOAT32)((UINT32)fVal) != fVal) // 小数部分有值
        {
            snprintf(pStrBuf, u32BufSize, "%g", fVal);
        }
        else // 小数部分为0，强制显示1位小数
        {
            snprintf(pStrBuf, u32BufSize, "%.1f", fVal);
        }
    }

    return pStrBuf;
}


/**
 * @fn      dspttk_timer_set_time
 * @brief   设置定时器的触发时间间隔
 *
 * @param   [IN] pTimerHandle dspttk_timer_create()返回的定时器句柄
 * @param   [IN] u32Time 定时器的触发时间间隔，单位：us
 *
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS dspttk_timer_set_time(void *pTimerHandle, UINT32 u32Time)
{
    INT32 s32Ret = 0;
    struct itimerspec in = {0}, out = {0};

    if (NULL == pTimerHandle)
    {
        PRT_INFO("the 'pTimerHandle' is NULL\n");
        return SAL_FAIL;
    }

    in.it_value.tv_sec = u32Time / 1000000;
    in.it_value.tv_nsec = (u32Time % 1000000) * 1000;
    in.it_interval.tv_sec = u32Time / 1000000;
    in.it_interval.tv_nsec = (u32Time % 1000000) * 1000;
    s32Ret = timer_settime(*(timer_t *)pTimerHandle, TIMER_ABSTIME, &in, &out);
    if (s32Ret == 0)
    {
        PRT_INFO("timer_settime successfully, pTimerHandle: %p, u32Time: %u\n", pTimerHandle, u32Time);
        return SAL_SOK;
    }
    else
    {
        PRT_INFO("timer_settime failed, u32Time: %u, ret: %d, errno: %d, %s\n", u32Time, s32Ret, errno, strerror(errno));
        return SAL_FAIL;
    }
}


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
void *dspttk_timer_create(UINT32 u32Time, void (*notify_function)(const INT32), INT32 sigval)
{
    INT32 s32Ret = 0;
    timer_t *pTimerHandle = NULL;
    pthread_attr_t stThreadAttr = {0};
    struct sched_param stSchedParam = {0};
    struct sigevent stSigEv = {0};
    struct itimerspec in = {0}, out = {0};

    if (NULL == notify_function)
    {
        PRT_INFO("the notify_function is NULL\n");
        return NULL;
    }
    s32Ret = pthread_attr_init(&stThreadAttr);
    if (0 != s32Ret)
    {
        PRT_INFO("pthread_attr_init failed, ret: %d, errno: %d, %s\n", s32Ret, errno, strerror(errno));
        return NULL;
    }

    // 设置线程调度策略，默认为SCHED_RR，优先级比中值略高
    s32Ret = pthread_attr_setinheritsched(&stThreadAttr, PTHREAD_EXPLICIT_SCHED);
    if (0 != s32Ret)
    {
        PRT_INFO("pthread_attr_setinheritsched failed, ret: %d, errno: %d, %s\n", s32Ret, errno, strerror(errno));
        goto EXIT;
    }
    s32Ret = pthread_attr_setschedpolicy(&stThreadAttr, SCHED_RR);
    if (0 != s32Ret)
    {
        PRT_INFO("pthread_attr_setschedpolicy failed, ret: %d, errno: %d, %s\n", s32Ret, errno, strerror(errno));
        goto EXIT;
    }
    stSchedParam.sched_priority = (sched_get_priority_max(SCHED_RR) * 2 + sched_get_priority_min(SCHED_RR)) / 3;
    s32Ret = pthread_attr_setschedparam(&stThreadAttr, &stSchedParam);
    if (0 != s32Ret)
    {
        PRT_INFO("pthread_attr_setschedparam failed, ret: %d, errno: %d, %s\n", s32Ret, errno, strerror(errno));
        goto EXIT;
    }

    // 当定时器到期，内核会以sigev_notification_attrbutes为属性创建一个线程，并且让它执行sigev_notify_function，传入sigev_value作为一个参数
    stSigEv.sigev_notify = SIGEV_THREAD;
    stSigEv.sigev_notify_function = (void *)notify_function;
    stSigEv.sigev_value.sival_int = sigval;
    stSigEv.sigev_notify_attributes = &stThreadAttr;

    pTimerHandle = (timer_t *)malloc(sizeof(timer_t));
    if (NULL == pTimerHandle)
    {
        PRT_INFO("malloc for 'pTimerHandle' failed, size: %zu\n", sizeof(timer_t));
        goto EXIT;
    }

    // timer_creater只是创建一个定时器，并未启动，需要将它关联到一个到期时间以及启动始终周期
    s32Ret = timer_create(CLOCK_MONOTONIC, &stSigEv, pTimerHandle);
    if (0 == s32Ret)
    {
        /**
         * it_value用于指定当前的定时器到期时间，当定时器到期，it_value的值会被更新成it_interval 的值。
         * 如果it_interval的值为0，则it_value到期就会回到未启动状态，即该定时器不是一个时间间隔定时器。
         */
        in.it_value.tv_sec = u32Time / 1000000;
        in.it_value.tv_nsec = (u32Time % 1000000) * 1000;
        in.it_interval.tv_sec = u32Time / 1000000;
        in.it_interval.tv_nsec = (u32Time % 1000000) * 1000;
        s32Ret = timer_settime(*pTimerHandle, TIMER_ABSTIME, &in, &out);
        if (0 == s32Ret)
        {
            PRT_INFO("create timer successfully, pTimerHandle: %p, u32Time: %u(%lu ns)\n", pTimerHandle, u32Time, in.it_interval.tv_nsec);
        }
        else
        {
            PRT_INFO("timer_settime failed, u32Time: %u, ret: %d, errno: %d, %s\n", u32Time, s32Ret, errno, strerror(errno));
            free(pTimerHandle);
            pTimerHandle = NULL;
        }
    }
    else
    {
        PRT_INFO("timer_create failed, u32Time: %u, ret: %d, errno: %d, %s\n", u32Time, s32Ret, errno, strerror(errno));
        free(pTimerHandle);
        pTimerHandle = NULL;
    }

EXIT:
    pthread_attr_destroy(&stThreadAttr);
    return pTimerHandle;
}


/**
 * @fn      dspttk_timer_destroy
 * @brief   销毁定时器
 *
 * @param   [IN] pTimerHandle dspttk_timer_creat()返回的定时器句柄
 * @param  [OUT] 无
 */
void dspttk_timer_destroy(void *pTimerHandle)
{
    INT32 s32Ret = 0;

    if (NULL != pTimerHandle)
    {
        s32Ret = timer_delete(*(timer_t *)pTimerHandle);
        if (0 == s32Ret)
        {
            free(pTimerHandle);
            PRT_INFO("timer_delete successfully, pTimerHandle: %p\n", pTimerHandle);
        }
        else
        {
            PRT_INFO("timer_delete failed, pTimerHandle: %p, ret: %d, errno: %d, %s\n", pTimerHandle, s32Ret, errno, strerror(errno));
        }
    }

    return;
}


/**
 * @fn      dspttk_get_clock_ms
 * @brief   获取系统单调递增的时钟数，以ms计
 *
 * @return  UINT64 时钟数，单位：毫秒
 */
UINT64 dspttk_get_clock_ms(void)
{
    int ret_val = 0;
    struct timespec tsnow = {0};

    ret_val = clock_gettime(CLOCK_MONOTONIC, &tsnow);
    if (0 == ret_val)
    {
        return (uint64_t)(tsnow.tv_sec) * 1000 + tsnow.tv_nsec / 1000000;
    }
    else
    {
        PRT_INFO("clock_gettime failed, errno: %d, %s\n", errno, strerror(errno));
        return 0;
    }
}


/**
 * @fn      dspttk_msleep
 * @brief   使当前状态sleep指定时间
 *
 * @param   msecTimeOut[IN] 超时时间，毫秒值
 * @param   none[OUT] 无
 *
 * @return    1 为获取成功 0为获取失败
 */
UINT32 dspttk_msleep(INT32 msecTimeOut)
{
    UINT32            s32Ret;
    INT32            cnt = 200;
    struct timeval   tv;
    INT32            tv_sec = msecTimeOut / 1000;
    INT32            tv_usec = msecTimeOut % 1000;

    do
    {
        tv.tv_sec = tv_sec;
        tv.tv_usec = tv_usec * 1000;

        s32Ret = select(0, NULL, NULL, NULL, &tv);
        if ((s32Ret < 0) && (errno == 4))
        {
            if (cnt <= 0)
            {
                PRT_INFO("select TimeOut err %d,errno %d\n", s32Ret, errno);
                return 0;
            }
            cnt--;
        }
        else
        {
            break;
        }
    }
    while (1);

    return 1;
}


/**
 * @fn      dspttk_get_random
 * @brief   获取[s32Min, s32Max]范围内的随机数
 * 
 * @param   [IN] s32Min 随机数最小值
 * @param   [IN] s32Max 随机数最大值
 * 
 * @return  随机数
 */
INT32 dspttk_get_random(INT32 s32Min, INT32 s32Max)
{
	INT32 s32Nu = 0;
	INT32 s32Seed = (INT32)dspttk_get_clock_ms();

	if (s32Min == s32Max)
	{
		return s32Min;
	}
	else if (s32Min > s32Max)
	{
		s32Nu = s32Min;
		s32Min = s32Max;
		s32Max = s32Nu;
	}

	srand(s32Seed);

	s32Nu = rand() % (s32Max - s32Min + 1) + s32Min;

	return s32Nu;
}


/**
 * @fn      dspttk_array_pointers_alloc
 * @brief   指针数组内存申请，内部前部分为ArrCnt个为指针，每个指针执行后部分的元素
 * 
 * @param   [IN] u32ArrCnt 指针数组元素个数
 * @param   [IN] u32ElemSize 指针数组元素内容长度，单位：字节
 * 
 * @return  VOID** 内存地址
 */
VOID **dspttk_array_pointers_alloc(UINT32 u32ArrCnt, UINT32 u32ElemSize)
{
    VOID **pp = NULL;
    UINT8 *p = NULL;
    UINT32 i = 0;

    if (0 == u32ArrCnt || 0 == u32ElemSize)
    {
        return NULL;
    }

    // 前ArrCnt个（VOID *）存储数据各元素的起始地址，后ArrCnt个ElemSize存储元素内容
    pp = (VOID **)malloc((sizeof(VOID *) + u32ElemSize) * u32ArrCnt);
    if (NULL != pp)
    {
        p = (UINT8 *)pp + sizeof(VOID *) * u32ArrCnt; // 指向数据内容起始
        for (i = 0; i < u32ArrCnt; i++)
        {
            pp[i] = p + u32ElemSize * i;
        }
    }

    return pp;
}

