
#include "sal_timer.h"
#include <poll.h>

/* 高性能定时器句柄 */
static INT32 g_timer_hp_fd = -1;


/**
 * @fn      SAL_TimerHpInit
 * @brief   初始化高性能定时器句柄
 * 
 * @return  SAL_STATUS SAL_SOK：初始化成功，SAL_FAIL：初始化失败
 */
SAL_STATUS SAL_TimerHpInit(void)
{
    /* 文件句柄*/
    if (g_timer_hp_fd < 0)
    {
        g_timer_hp_fd = open(TIMER_DEV, O_RDWR);
        if (g_timer_hp_fd < 0)
        {
            SAL_LOGE("oops, open '%s' failed\n", TIMER_DEV);
            return SAL_FAIL;
        }
    }

    SAL_LOGI("open '%s' successfully, fd: %d\n", TIMER_DEV, g_timer_hp_fd);

	return SAL_SOK;
}


/**
 * @fn      SAL_TimerHpCreate
 * @brief   创建指定触发频率的高性能定时器
 * 
 * @param   [IN] hz 每秒触发次数
 * 
 * @return  INT32 >=0：高性能定时器句柄，其他：创建失败
 */
INT32 SAL_TimerHpCreate(UINT32 hz)
{
	INT32 fd = SAL_CLIP(hz, 1, 1000);
	INT32 ret = SAL_FAIL;

	ret = ioctl(g_timer_hp_fd, TIMER_CREATE, &fd); // fd即是输入，也是输出，输入需设置的触发频率，输出创建的定时器fd
	if (0 == ret)
	{
        SAL_LOGI("create high performance timer successfully, fd: %d\n", fd);
        return fd;
	}
    else
    {
		SAL_LOGE("ioctl 'TIMER_CREATE' failed, ret: %d, errno: %d, %s\n", ret, errno, strerror(errno));
		return -1;
    }
}


/**
 * @fn      SAL_TimerHpSetHz
 * @brief   设置高性能定时器的触发频率
 * 
 * @param   [IN] fd SAL_TimerHpCreate()返回的高性能定时器句柄
 * @param   [IN] hz 每秒触发次数
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS SAL_TimerHpSetHz(INT32 fd, UINT32 hz)
{
	INT32 ret = 0;
    INT32 freq = SAL_CLIP(hz, 1, 1000);

	if (fd < 3)
    {
    	SAL_LOGW("WARNING !!! the fd maybe invalid: %d\n", fd);
    }

	ret = ioctl(fd, TIMER_SET_HZ, &freq);
	if (0 == ret)
	{
		SAL_LOGI("set timer hz successfully, fd %d, hz: %u\n", fd, freq);
        return SAL_SOK;
	}
    else
    {
		SAL_LOGE("ioctl 'TIMER_SET_HZ' failed, fd: %d, hz: %u, ret: %d, errno: %d, %s\n", fd, freq, ret, errno, strerror(errno));
		return SAL_FAIL;
    }
}


/**
 * @fn      SAL_TimerHpWait
 * @brief   等待高性能定时器触发
 * 
 * @param   [IN] fd SAL_TimerHpCreate()返回的高性能定时器句柄
 * 
 * @return  SAL_STATUS SAL_SOK：成功，SAL_FAIL：失败
 */
SAL_STATUS SAL_TimerHpWait(INT32 fd)
{
    INT32 ret = SAL_SOK;
	struct pollfd fds = {0}; /* fd当前只查询一个time，因此使用数组1，方便后续添加 */

	if (fd < 3)
    {
    	SAL_LOGW("WARNING !!! the fd maybe invalid: %d\n", fd);
    }

    fds.fd = fd;
	fds.events = POLLIN; // 普通或优先级带数据可读
	ret = poll(&fds, 1, 5000);
    if (ret < 0)
    {
		SAL_LOGE("poll failed, fd: %d, ret: %d, errno: %d, %s\n", fd, ret, errno, strerror(errno));
        return SAL_FAIL;
    }
    else
    {
        return SAL_SOK;
    }
}


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
void *SAL_TimerCreate(UINT32 itUs, UINT32 loopCnt, void (*notify_function)(const INT32), INT32 sigval)
{
    INT32 s32Ret = 0;
    timer_t *pTimerHandle = NULL;
    pthread_attr_t stThreadAttr = {0};
    struct sched_param stSchedParam = {0};
    struct sigevent stSigEv = {0};
    struct itimerspec in = {0}, out = {0};

    if (NULL == notify_function)
    {
        SAL_LOGE("the notify_function is NULL\n");
        return NULL;
    }
    s32Ret = pthread_attr_init(&stThreadAttr);
    if (0 != s32Ret)
    {
        SAL_LOGE("pthread_attr_init failed, ret: %d, errno: %d, %s\n", s32Ret, errno, strerror(errno));
        return NULL;
    }

    // 设置线程调度策略，默认为SCHED_RR，优先级比中值略高
    s32Ret = pthread_attr_setinheritsched(&stThreadAttr, PTHREAD_EXPLICIT_SCHED);
    if (0 != s32Ret)
    {
        SAL_LOGE("pthread_attr_setinheritsched failed, ret: %d, errno: %d, %s\n", s32Ret, errno, strerror(errno));
        goto EXIT;
    }
    s32Ret = pthread_attr_setschedpolicy(&stThreadAttr, SCHED_RR);
    if (0 != s32Ret)
    {
        SAL_LOGE("pthread_attr_setschedpolicy failed, ret: %d, errno: %d, %s\n", s32Ret, errno, strerror(errno));
        goto EXIT;
    }
    stSchedParam.sched_priority = (sched_get_priority_max(SCHED_RR) * 2 + sched_get_priority_min(SCHED_RR)) / 3;
    s32Ret = pthread_attr_setschedparam(&stThreadAttr, &stSchedParam);
    if (0 != s32Ret)
    {
        SAL_LOGE("pthread_attr_setschedparam failed, ret: %d, errno: %d, %s\n", s32Ret, errno, strerror(errno));
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
        SAL_LOGE("malloc for 'pTimerHandle' failed, size: %zu\n", sizeof(timer_t));
        goto EXIT;
    }

    // timer_creater只是创建一个定时器，并未启动，需要将它关联到一个到期时间以及启动始终周期
    s32Ret = timer_create(CLOCK_MONOTONIC, &stSigEv, pTimerHandle);
    if (0 == s32Ret)
    {
        /**
         * it_value用于指定当前的定时器到期时间，当定时器到期，it_value的值会被更新成it_interval 的值。 
         * 如果it_interval的值为0，则该定时器停止。任何时候，只要it_value设置为零，该定时器就会停止。
         */
        in.it_value.tv_sec = itUs / 1000000;
        in.it_value.tv_nsec = itUs % 1000000 * 1000;
        if (loopCnt > 1)
        {
            in.it_interval = in.it_value;
        }
        s32Ret = timer_settime(*pTimerHandle, 0, &in, &out);
        if (0 == s32Ret)
        {
            SAL_LOGI("create timer successfully, pTimerHandle: %p, itUs: %u\n", pTimerHandle, itUs);
        }
        else
        {
            SAL_LOGE("timer_settime failed, itUs: %u, ret: %d, errno: %d, %s\n", itUs, s32Ret, errno, strerror(errno));
            free(pTimerHandle);
            pTimerHandle = NULL;
        }
    }
    else
    {
        SAL_LOGE("timer_create failed, itUs: %u, ret: %d, errno: %d, %s\n", itUs, s32Ret, errno, strerror(errno));
        free(pTimerHandle);
        pTimerHandle = NULL;
    }

EXIT:
    pthread_attr_destroy(&stThreadAttr);
    return pTimerHandle;
}


/**
 * @fn      SAL_TimerSetIt
 * @brief   设置定时器的触发时间间隔
 * 
 * @param   [IN] pTimerHandle SAL_TimerCreate()返回的定时器句柄
 * @param   [IN] itUs 定时器触发间隔时间，单位：us
 * 
 * @return  SAL_STATUS SAL_SOK：设置成功，SAL_FAIL：设置失败
 */
SAL_STATUS SAL_TimerSetIt(void *pTimerHandle, UINT32 itUs)
{
    INT32 s32Ret = 0;
    struct itimerspec in = {0}, out = {0};

    if (NULL == pTimerHandle)
    {
        SAL_LOGE("the 'pTimerHandle' is NULL\n");
		return SAL_FAIL;
    }

    in.it_value.tv_sec = itUs / 1000000;
    in.it_value.tv_nsec = itUs % 1000000 * 1000;
    in.it_interval = in.it_value;
    s32Ret = timer_settime(*(timer_t *)pTimerHandle, TIMER_ABSTIME, &in, &out);
    if (s32Ret == 0)
    {
        SAL_LOGI("timer_settime successfully, pTimerHandle: %p, itUs: %u\n", pTimerHandle, itUs);
        return SAL_SOK;
    }
    else
    {
        SAL_LOGE("timer_settime failed, itUs: %u, ret: %d, errno: %d, %s\n", itUs, s32Ret, errno, strerror(errno));
		return SAL_FAIL;
    }
}


/**
 * @fn      SAL_TimerDestroy
 * @brief   销毁定时器
 * 
 * @param   [IN] pTimerHandle SAL_TimerCreate()返回的定时器句柄
 */
void SAL_TimerDestroy(void *pTimerHandle)
{
    INT32 s32Ret = 0;

    if (NULL != pTimerHandle)
    {
        s32Ret = timer_delete(*(timer_t *)pTimerHandle);
        if (0 == s32Ret)
        {
            SAL_LOGI("timer_delete successfully, pTimerHandle: %p\n", pTimerHandle);
            free(pTimerHandle);
            pTimerHandle = NULL;
        }
        else
        {
            SAL_LOGE("timer_delete failed, pTimerHandle: %p, ret: %d, errno: %d, %s\n", pTimerHandle, s32Ret, errno, strerror(errno));
        }
    }

    return;
}

