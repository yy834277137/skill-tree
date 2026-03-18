
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <sys/prctl.h>
#include <time.h>
#include "dspttk_posix_ipc.h"
#include "prt_trace.h"

static void map_ms_to_timespec(IN const long wait_ms, OUT struct timespec *ts)
{
    int ret_val = 0;
    long ns = 0, ms = wait_ms % 1000;

    if (NULL == ts)
    {
        PRT_INFO("ts is NULL\n");
        return;
    }

    ret_val = clock_gettime(CLOCK_REALTIME, ts);
    if (0 == ret_val)
    {
        ns = ts->tv_nsec + ms * 1000000; // ns MAX: 1,998,000,000
        ts->tv_sec += wait_ms / 1000 + ns / 1000000000;
        ts->tv_nsec = ns % 1000000000;
    }
    else
    {
        PRT_INFO("clock_gettime failed, errno: %d, %s\n", errno, strerror(errno));
        ts->tv_sec = time(NULL) + 1;
        ts->tv_nsec = 0;
    }

    return;
}


static unsigned long get_tick_count(void)
{
    int ret_val = 0;
    struct timespec tsnow = {0};

    ret_val = clock_gettime(CLOCK_MONOTONIC, &tsnow);
    if (0 != ret_val)
    {
        PRT_INFO("clock_gettime failed, errno: %d, %s\n", errno, strerror(errno));
    }

    return (unsigned long)(tsnow.tv_sec) * 1000 + tsnow.tv_nsec / 1000000;
}


/*================== POSIX线程 ==================*/

typedef void *(*FUNCPTR)(void *, void *, void *, void *, void *, void *, void *, void *, void *, void *);
typedef void *(*START_ROUTINE)(void *);

typedef struct
{
    FUNCPTR entry; /* execution entry point address for thread */
    void *arg[10]; /* arguments */
}FUNC_WRAP;


static void *func_entry(void *arg)
{
    FUNC_WRAP *func = (FUNC_WRAP *)arg;

    (*(func->entry))(func->arg[0], func->arg[1], func->arg[2], func->arg[3], func->arg[4], func->arg[5], func->arg[6], func->arg[7], func->arg[8], func->arg[9]);
    free(arg);

    return NULL;
}


/**
 * @fn      dspttk_pthread_set_attr
 * @brief   设置线程属性，默认属性为：绑定轻进程，分离线程，调度属性不继承
 * 
 * @param   attr[IN] 线程属性
 * @param   policy[IN] 线程调度策略
 * @param   priority[IN] 优先级，值越大，优先级越高，取值范围[0, 100]
 * @param   stacksize[IN] 栈空间，至少为16KB，建议大小为1MB，即0x100000
 * 
 * @return  SAL_SOK-设置线程属性成功，SAL_FAIL-设置线程属性失败
 */
static SAL_STATUS dspttk_pthread_set_attr(pthread_attr_t *attr, DSPTTK_PTHREAD_SCHED_POLICY policy, int priority, size_t stacksize)
{
    int ret_val = 0;
    struct sched_param _sched_param;
    int _policy = SCHED_OTHER, priority_max = 0, priority_min = 0;

    ret_val = pthread_attr_init(attr);
    if (ret_val != 0)
    {
        PRT_INFO("pthread_attr_init failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    /**
     * 设置线程绑定状态
     * 轻进程可以理解为内核线程，它位于用户层和系统层之间。 
     * 系统对线程资源的分配、对线程的控制是通过轻进程来实现的，一个轻进程可以控制一个或多个线程。 
     * 默认状况下，启动多少轻进程、哪些轻进程来控制哪些线程是由系统来控制的，这种状况即称为非绑定的。 
     * 而在绑定状况下，则顾名思义，即某个线程固定的"绑"在一个轻进程之上。 
     * 被绑定的线程具有较高的响应速度，这是因为CPU时间片的调度是面向轻进程的， 
     * 绑定的线程可以保证在需要的时候总有一个轻进程可用。 
     * 通过设置被绑定的轻进程的优先级和调度级可以使得绑定的线程满足诸如实时反应之类的要求。 
     * PTHREAD_SCOPE_SYSTEM：绑定的 
     * PTHREAD_SCOPE_PROCESS：非绑定的
     */
    ret_val = pthread_attr_setscope(attr, PTHREAD_SCOPE_SYSTEM);
    if (ret_val == ENOTSUP)
    {
        PRT_INFO("The system does not support the PTHREAD_SCOPE_SYSTEM, using PTHREAD_SCOPE_PROCESS instead\n");
        ret_val = pthread_attr_setscope(attr, PTHREAD_SCOPE_PROCESS);
    }
    if (ret_val != 0)
    {
        PRT_INFO("pthread_attr_setscope failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        pthread_attr_destroy(attr);
        return SAL_FAIL;
    }

    /**
     * 设置线程是否分离
     * 线程的分离状态决定一个线程以什么样的方式来终止自己。在非分离状态下，原有的线程等待创建的线程结束。 
     * 只有当pthread_join函数返回时，创建的线程才算终止，才能释放自己占用的系统资源。 
     * 而分离线程不是这样子的，它没有被其他的线程所等待，自己运行结束了，线程也就终止了，马上释放系统资源
     * 这里要注意的一点是，如果设置一个线程为分离线程，而这个线程运行又非常快， 
     * 它很可能在pthread_create 函数返回之前就终止了，它终止以后就可能将线程号和系统资源移交给其他的线程使用，
     * 这样调用pthread_create的线程就得到了错误的线程号。要避免这种情况可以采取一定的同步措施， 
     * 最简单的方法之一是可以在被创建的线程里调用pthread_cond_timewait函数，让这个线程等待一会儿， 
     * 留出足够的时间让函数pthread_create返回。设置一段等待时间，是在多线程编程里常用的方法。 
     * 但是注意不要使用诸如wait()之类的函数，它们是使整个进程睡眠，并不能解决线程同步的问题。
     * PTHREAD_CREATE_DETACHED：分离线程，不允许调用pthread_join获得另一个线程的退出状态
     * PTHREAD _CREATE_JOINABLE：非分离线程
     */
    ret_val = pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED);
    if (ret_val != 0)
    {
        PRT_INFO("pthread_attr_setdetachstate failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        pthread_attr_destroy(attr);
        return SAL_FAIL;
    }

    /**
     * pthread_attr_setinheritsched 用于确定如何设置线程的调度属性，可以从创建者线程或从一个属性对象来继承调度属性。 
     * inheritsched可以为如下值： 
     * PTHREAD_INHERIT_SCHED：线程调度属性是从创建者线程继承得到，attr的任何调度属性都被忽略。
     * PTHREAD_EXPLICIT_SCHED：线程调度属性需自己重新设置。
     * 如果inheritsched值是PTHREAD_EXPLICIT_SCHED， 
     * 则pthread_attr_setschedpolicy()被用于设置调度策略，而pthread_attr_setschedparam()被用于设置优先级。 
     * nptl创建线程默认继承父线程的调度优先级，需设置inheritsched为 PTHREAD_EXPLICIT_SCHED，自己设置的调度策略和优先级才能生效
     */
    ret_val = pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
    if (ret_val != 0)
    {
        PRT_INFO("pthread_attr_setinheritsched failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        pthread_attr_destroy(attr);
        return SAL_FAIL;
    }

    /**
     * 设置线程的调度策略 
     * SCHED_FIFO：先进先出调度策略，执行线程运行到结束。
     * SCHED_RR：轮询调度策略，按照时间片将每个线程分配到处理器上。
     * SCHED_OTHER：另外的调度策（根据实现定义）。这是任何新创建线程的默认调度策略。 
     */
    if (DSPTTK_PTHREAD_SCHED_RR == policy)
    {
        _policy = SCHED_RR;
    }
    else if (DSPTTK_PTHREAD_SCHED_FIFO == policy)
    {
        _policy = SCHED_FIFO;
    }
    else
    {
        _policy = SCHED_OTHER;
    }
    ret_val = pthread_attr_setschedpolicy(attr, _policy);
    if (ret_val != 0)
    {
        PRT_INFO("pthread_attr_setschedpolicy failed, policy: %d, errno: %d, %s\n", _policy, ret_val, strerror(ret_val));
        pthread_attr_destroy(attr);
        return SAL_FAIL;
    }

    /**
     * 设置线程的优先级 
     * 一般说来，我们总是先取优先级，对取得的值修改后再存放回去。 
     */
    ret_val = pthread_attr_getschedparam(attr, &_sched_param);
    if (ret_val != 0)
    {
        PRT_INFO("pthread_attr_getschedparam failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        pthread_attr_destroy(attr);
        return SAL_FAIL;
    }
    priority_min = sched_get_priority_min(_policy);
    if (-1 == priority_min)
    {
        PRT_INFO("sched_get_priority_min failed, errno: %d, %s\n", errno, strerror(errno));
        pthread_attr_destroy(attr);
        return SAL_FAIL;
    }
    priority_max = sched_get_priority_max(_policy);
    if (-1 == priority_max)
    {
        PRT_INFO("sched_get_priority_max failed, errno: %d, %s\n", errno, strerror(errno));
        pthread_attr_destroy(attr);
        return SAL_FAIL;
    }
    _sched_param.sched_priority = SAL_CLIP(priority, 0, 100) * (priority_max - priority_min) / 100;
    ret_val = pthread_attr_setschedparam(attr, &_sched_param);
    if (ret_val != 0)
    {
        PRT_INFO("pthread_attr_setschedparam failed, sched_priority: %d, errno: %d, %s\n", \
                _sched_param.sched_priority, ret_val, strerror(ret_val));
        pthread_attr_destroy(attr);
        return SAL_FAIL;
    }

    /* when set stack size, we define a minmum value to avoid fail */
    stacksize = SAL_MAX(stacksize, PTHREAD_STACK_MIN);
    ret_val = pthread_attr_setstacksize(attr, stacksize);
    if (ret_val != 0)
    {
        PRT_INFO("pthread_attr_setstacksize failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        pthread_attr_destroy(attr);
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_pthread_spawn
 * @brief   创建线程，线程默认属性为：：绑定轻进程，分离，调度属性不继承 
 * @warning 注：不允许调用pthread_join获得另一个线程的退出状态，因为是设置了进程的分离性 
 * 
 * @param   thread_id[IN] 线程ID号，可为NULL，typedef unsigned long pthread_t 
 * @param   policy[IN] 线程调度策略
 * @param   priority[IN] 优先级，值越大，优先级越高，取值范围[0, 100]
 * @param   stacksize[IN] 栈空间，至少为16KB，建议大小为1MB，即0x100000
 * @param   function[IN] 函数名，线程入口
 * @param   argc[IN] 函数参数个数，取值范围[0, 10]，后面紧跟函数形参
 * 
 * @return  SAL_SOK-创建线程成功，SAL_FAIL-创建线程失败
 */
SAL_STATUS dspttk_pthread_spawn(pthread_t *thread_id, DSPTTK_PTHREAD_SCHED_POLICY policy, int priority, size_t stacksize, void *function, unsigned int argc, ...)
{
    int i = 0, ret_val = 0;
    void *arg[] = {[0 ... 9] = NULL};
    START_ROUTINE start_routine = func_entry;
    pthread_t tid = 0, *ptid = NULL;
    pthread_attr_t attr = {{0}};
    va_list ap;
    FUNC_WRAP *funcptr = NULL;

    if (function == NULL || argc > 10)
    {
        PRT_INFO("function[%p] is NULL OR argc[%d] is more than 10\n", function, argc);
        return SAL_FAIL;
    }

    va_start(ap, argc);
    for (i = 0; i < argc; i++)
    {
        arg[i] = va_arg(ap, void *);
    }
    va_end(ap);

    ret_val = dspttk_pthread_set_attr(&attr, policy, priority, stacksize);
    if (ret_val != SAL_SOK)
    {
        PRT_INFO("dspttk_pthread_set_attr failed, priority: %d, stacksize: %u\n", priority, (unsigned int)stacksize);
        return ret_val;
    }

    if (thread_id != NULL)
    {
        ptid = thread_id;
    }
    else
    {
        ptid = &tid;
    }

    /**
     * If the total optional argumens is 0 or 1, we call the pthread_create directly. 
     * Otherwise, we wrap a start_routine routine.
     */
    if (argc <= 1)
    {
        ret_val = pthread_create(ptid, &attr, (START_ROUTINE)function, arg[0]);
        if (ret_val != 0)
        {
            PRT_INFO("pthread_create failed, errno: %d, %s\n", ret_val, strerror(ret_val));
            ret_val = SAL_FAIL;
        }
    }
    else
    {
        funcptr = (FUNC_WRAP *)malloc(sizeof(FUNC_WRAP));
        if (funcptr == NULL)
        {
            PRT_INFO("malloc for FUNC_WRAP failed\n");
            ret_val = SAL_FAIL;
            goto ERR_EXIT;
        }

        memset(funcptr, 0, sizeof(FUNC_WRAP));
        funcptr->entry = function;
        for (i = 0; i < argc; i++)
        {
            funcptr->arg[i] = arg[i];
        }

        ret_val = pthread_create(ptid, &attr, start_routine, (void *)funcptr);
        if (ret_val != 0)
        {
            free(funcptr);
            PRT_INFO("pthread_create failed, errno: %d, %s\n", ret_val, strerror(ret_val));
            ret_val = SAL_FAIL;
        }
        else
        {
            ret_val = SAL_SOK;
        }
    }

ERR_EXIT:
    pthread_attr_destroy(&attr);

    return ret_val;
}


/**
 * @fn      dspttk_pthread_set_name
 * @brief   设置线程名
 * 
 * @param   thread_name[IN] 线程名，注：加最后一个空字符'\0'，总长不能超过16个字符
 * 
 * @return  SAL_SOK-设置成功，SAL_FAIL-设置失败
 */
SAL_STATUS dspttk_pthread_set_name(char *thread_name)
{
    int ret_val = 0;
    char tn[16] = {0};

    if (NULL != thread_name)
    {
        strncpy(tn, thread_name, 15);
    }
    else
    {
        PRT_INFO("thread_name is NULL\n");
        return SAL_FAIL;
    }

    ret_val = prctl(PR_SET_NAME, (unsigned long)tn);
    if (ret_val != 0)
    {
        PRT_INFO("PR_SET_NAME failed, errno: %d, %s\n", errno, strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_pthread_get_name
 * @brief   获取线程名
 * 
 * @param   thread_name[OUT] 线程名
 * @param   name_len[IN] 线程名字符串长度，不能小于16
 * 
 * @return  SAL_SOK-设置成功，SAL_FAIL-设置失败
 */
SAL_STATUS dspttk_pthread_get_name(char *thread_name, size_t name_len)
{
    int ret_val = 0;

    if (NULL == thread_name)
    {
        PRT_INFO("thread_name[%p] is NULL\n", thread_name);
        return SAL_FAIL;
    }

    if (name_len < 16)
    {
        PRT_INFO("name_len must be not less than 16. Current value is: %u\n", (unsigned int)name_len);
        return SAL_FAIL;
    }

    ret_val = prctl(PR_GET_NAME, (unsigned long)thread_name);
    if (ret_val != 0)
    {
        PRT_INFO("PR_GET_NAME failed, errno: %d, %s\n", errno, strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_pthread_get_selfid
 * @brief   获取线程自身的ID号 
 * @note    该函数执行总是成功，没有失败
 *  
 * @return  线程自身的ID号，pthread_t的类型为unsigned long int
 */
pthread_t dspttk_pthread_get_selfid(void)
{
    return pthread_self();
}


/**
 * @fn      dspttk_pthread_is_alive
 * @brief   线程是否活着
 * 
 * @param   thread_id[IN] 线程ID号
 * 
 * @return  SAL_TRUE-线程活着，FALSE-线程不存在
 */
BOOL dspttk_pthread_is_alive(pthread_t thread_id)
{
    if (thread_id > 0 && pthread_kill(thread_id, 0) == 0)
    {
        return SAL_TRUE;
    }
    else
    {
        return SAL_FALSE;
    }
}


/*======================= POSIX互斥锁 =======================*/

/**
 * @fn      dspttk_mutex_init
 * @brief   初始化互斥锁，默认类型位PTHREAD_MUTEX_ERRORCHECK，即防止最简单的死锁发生
 * @note    如果同一线程企图对已上锁的mutex进行relock，或企图对未上锁的mutex进行unlock，均返回错误 
 *  
 * @param   mutex_id[OUT] 互斥锁ID
 * 
 * @return  SAL_SOK-初始化互斥锁成功，SAL_FAIL-初始化互斥锁失败
 */
SAL_STATUS dspttk_mutex_init(pthread_mutex_t *mutex_id)
{
    int ret_val = 0;
    pthread_mutexattr_t attr;

    if (NULL == mutex_id)
    {
        PRT_INFO("mutex_id is NULL\n");
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
    ret_val = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_mutexattr_settype failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
        goto ERR_EXIT;
    }

    #if 1 //def _POSIX_SHARED_MEMORY_OBJECTS
    ret_val = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_mutexattr_setpshared failed, errno: %d, %s\n", ret_val, strerror(ret_val));
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
        PRT_INFO("pthread_mutex_init failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
    }

ERR_EXIT:
    pthread_mutexattr_destroy(&attr);

    return ret_val;
}


/**
 * @fn      dspttk_mutex_init_ex
 * @brief   初始化互斥锁，并带有互斥锁类型
 *  
 * @param   mutex_id[OUT] 互斥锁ID
 * @param   mutex_type[IN] 互斥锁类型：分为PTHREAD_MUTEX_NORMAL、PTHREAD_MUTEX_ERRORCHECK、 
 *                                      PTHREAD_MUTEX_RECURSIVE和PTHREAD_MUTEX_DEFAULT 
 *  
 * @return  SAL_SOK-初始化互斥锁成功，SAL_FAIL-初始化互斥锁失败
 */
SAL_STATUS dspttk_mutex_init_ex(pthread_mutex_t *mutex_id, int mutex_type)
{
    int ret_val = 0;
    pthread_mutexattr_t attr;

    if (NULL == mutex_id)
    {
        PRT_INFO("mutex_id is NULL\n");
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
    ret_val = pthread_mutexattr_settype(&attr, mutex_type);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_mutexattr_settype failed, mutex_type: %d, errno: %d, %s\n", mutex_type, ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
        goto ERR_EXIT;
    }

    #if 1 //def _POSIX_SHARED_MEMORY_OBJECTS
    ret_val = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_mutexattr_setpshared failed, errno: %d, %s\n", ret_val, strerror(ret_val));
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
        PRT_INFO("pthread_mutex_init failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
    }

ERR_EXIT:
    pthread_mutexattr_destroy(&attr);

    return ret_val;
}


/**
 * @fn      dspttk_mutex_lock
 * @brief   尝试在一定时间内对mutex_id上锁
 * 
 * @param   mutex_id[IN] 互斥锁ID
 * @param   wait_ms[IN] 等待上锁的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 * 
 * @return  SAL_SOK-上锁成功，SAL_FAIL-上锁失败
 */
SAL_STATUS dspttk_mutex_lock(pthread_mutex_t *mutex_id, long wait_ms, const char *function, const unsigned int line)
{
    int ret_val = 0;
    struct timespec timeout = {0};
    unsigned long ts = 0, te = 0, tl = (unsigned long)wait_ms;

    if (NULL == mutex_id)
    {
        PRT_INFO("mutex_id is NULL\n");
        return SAL_FAIL;
    }

    if (wait_ms == SAL_TIMEOUT_NONE)
    {
        ret_val = pthread_mutex_trylock(mutex_id);
    }
    else if (wait_ms < 0) 
    {
        ret_val = pthread_mutex_lock(mutex_id);
    }
    else
    {
        do
        {
            tl = tl + ts - te;
            ts = get_tick_count();
            map_ms_to_timespec(tl, &timeout);
            ret_val = pthread_mutex_timedlock(mutex_id, &timeout);
            if (ETIMEDOUT == ret_val) // 如果时间到后，条件还没有发生，那么会返回ETIMEDOUT错误
            {
                te = get_tick_count();
            }
            else
            {
                break;
            }
        } while (ts + tl > te);
    }

    if (0 != ret_val)
    {
        if (ETIMEDOUT != ret_val)
        {
            if (NULL != function)
            {
                PRT_INFO("pthread_mutex_lock failed @ FUNCTION[%s]-LINE[%u], wait_ms: %ld, errno: %d, %s\n", \
                         function, line, wait_ms, ret_val, strerror(ret_val));
            }
            else
            {
                PRT_INFO("pthread_mutex_lock failed, wait_ms: %ld, errno: %d, %s\n", wait_ms, ret_val, strerror(ret_val));
            }
        }
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_mutex_unlock
 * @brief   对mutex_id解锁
 * 
 * @param   mutex_id[IN] 互斥锁ID
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 * 
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS dspttk_mutex_unlock(pthread_mutex_t *mutex_id, const char *function, const unsigned int line)
{
    int ret_val = 0;

    if (NULL == mutex_id)
    {
        PRT_INFO("mutex_id is NULL\n");
        return SAL_FAIL;
    }

    ret_val = pthread_mutex_unlock(mutex_id);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_mutex_unlock failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_mutex_destroy
 * @brief   销毁互斥锁
 * 
 * @param   mutex_id[IN] 互斥锁ID
 * 
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS dspttk_mutex_destroy(pthread_mutex_t *mutex_id)
{
    int ret_val = 0;

    if (NULL == mutex_id)
    {
        PRT_INFO("mutex_id is NULL\n");
        return SAL_FAIL;
    }

    ret_val = pthread_mutex_destroy(mutex_id);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_mutex_unlock failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*====================== POSIX条件变量 ======================*/

/**
 * @fn      dspttk_cond_init
 * @brief   初始化条件变量
 * 
 * @param   cond_s[OUT] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 * 
 * @return  SAL_SOK-初始化成功，SAL_FAIL-初始化失败
 */
SAL_STATUS dspttk_cond_init(DSPTTK_COND_T *cond_s)
{
    int ret_val = 0;
    pthread_condattr_t attr;

    if (NULL == cond_s)
    {
        PRT_INFO("cond_s is NULL\n");
        return SAL_FAIL;
    }

    ret_val = dspttk_mutex_init_ex(&cond_s->mid, PTHREAD_MUTEX_NORMAL);
    if (SAL_SOK != ret_val)
    {
        PRT_INFO("init mutex in DSPTTK_COND_T failed\n");
        return SAL_FAIL;
    }

    ret_val = pthread_condattr_init(&attr);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_condattr_init failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    ret_val = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_condattr_setclock failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        pthread_condattr_destroy(&attr);
        return SAL_FAIL;
    }

    #if 1 //def _POSIX_SHARED_MEMORY_OBJECTS
    ret_val = pthread_condattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_condattr_setpshared failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        pthread_condattr_destroy(&attr);
        return SAL_FAIL;
    }
    #else
    #error UNSUPPORT 'PTHREAD_PROCESS_SHARED'
    #endif

    ret_val = pthread_cond_init(&cond_s->cid, &attr);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_cond_init failed, errno: %d, %s\n", ret_val, strerror(ret_val));
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
 * @fn      dspttk_cond_wait
 * @brief   在一定时间内等待条件变为真
 * @warning dspttk_cond_wait成功返回时，线程需要重新计算条件，因为其他线程可能在运行过程中已经改变条件
 *
 * @param   cond_s[IN] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 * @param   wait_ms[IN] 等待条件变为真的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，其他表示时间
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 * 
 * @return  SAL_SOK-等待条件变为真成功，SAL_FAIL-等待条件变成真失败
 */
SAL_STATUS dspttk_cond_wait(DSPTTK_COND_T *cond_s, long wait_ms, const char *function, const unsigned int line)
{
    int ret_val = 0;
    struct timespec timeout;

    if (NULL == cond_s)
    {
        PRT_INFO("cond_s is NULL\n");
        return SAL_FAIL;
    }

    /**
     * 调用pthread_cond_wait()函数后，会原子的执行以下两个动作： 
     * ①将调用线程放到等待条件的线程列表上，即进入睡眠； 
     * ②对互斥量进行解锁 
     * 由于这两个操作时原子操作，这样就关闭了条件检查和线程进入睡眠等待条件改变这两个操作之间的时间通道 
     * 当pthread_cond_wait()返回后，互斥量会再次被锁住。 
     */
    if (wait_ms < 0)
    {
        ret_val = pthread_cond_wait(&cond_s->cid, &cond_s->mid);
    }
    else
    {
        ret_val = clock_gettime(CLOCK_MONOTONIC, &timeout);
        if (0 != ret_val)
        {
            PRT_INFO("clock_gettime failed, errno: %d, %s\n", errno, strerror(errno));
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
        if (ETIMEDOUT != ret_val)
        {
            if (NULL != function)
            {
                PRT_INFO("pthread_cond_wait failed @ FUNCTION[%s]-LINE[%u], wait_ms: %ld, errno: %d, %s\n", \
                         function, line, wait_ms, ret_val, strerror(ret_val));
            }
            else
            {
                PRT_INFO("pthread_cond_wait failed, wait_ms: %ld, errno: %d, %s\n", wait_ms, ret_val, strerror(ret_val));
            }
        }
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_cond_signal
 * @brief   向等待条件的线程发送唤醒信号
 * 
 * @param   cond_s[IN] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 * @param   bSignalBoardcast[IN] 唤醒信号是否为广播类型：
 *              TRUR-广播条件状态的改变，以唤醒等待该条件的所有线程（常用）
 *              FASLE-只会唤醒等待该条件的某个线程
 *          注：只有在等待者代码编写确切，只有一个等待者需要唤醒，且唤醒哪个线程无所谓，
 *              那么此时为这种情况使用单播，所以其他情况下都必须使用广播发送。
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0 
 *  
 * @return  SAL_SOK-发送唤醒信号成功，SAL_FAIL-发送唤醒信号失败
 */
SAL_STATUS dspttk_cond_signal(DSPTTK_COND_T *cond_s, BOOL bSignalBoardcast, const char *function, const unsigned int line)
{
    int ret_val = 0;

    if (NULL == cond_s)
    {
        PRT_INFO("cond_s is NULL\n");
        return SAL_FAIL;
    }

    if (bSignalBoardcast)
    {
        ret_val = pthread_cond_broadcast(&cond_s->cid);
    }
    else
    {
        ret_val = pthread_cond_signal(&cond_s->cid);
    }

    if (0 != ret_val)
    {
        if (NULL != function)
        {
            PRT_INFO("pthread_cond_signal failed @ FUNCTION[%s]-LINE[%u], bSignalBoardcast: %d, errno: %d, %s\n", \
                    function, line, bSignalBoardcast, ret_val, strerror(ret_val));
        }
        else
        {
            PRT_INFO("pthread_cond_signal failed, bSignalBoardcast: %d, errno: %d, %s\n", bSignalBoardcast, ret_val, strerror(ret_val));
        }
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_cond_destroy
 * @brief   销毁条件变量
 * 
 * @param   cond_s[IN] 条件变量结构体，包含条件变量ID-cid和互斥锁ID-mid
 * 
 * @return  SAL_SOK-销毁成功，SAL_FAIL-销毁失败
 */
SAL_STATUS dspttk_cond_destroy(DSPTTK_COND_T *cond_s)
{
    int ret_val_cond = 0;
    int ret_val_mutex = 0;

    if (NULL == cond_s)
    {
        PRT_INFO("cond_s is NULL\n");
        return SAL_FAIL;
    }

    ret_val_cond = pthread_cond_destroy(&cond_s->cid);
    if (0 != ret_val_cond)
    {
        PRT_INFO("pthread_cond_destroy failed, errno: %d, %s\n", ret_val_cond, strerror(ret_val_cond));
        ret_val_cond = SAL_FAIL;
    }
    else
    {
        ret_val_cond = SAL_SOK;
    }

    ret_val_mutex = dspttk_mutex_destroy(&cond_s->mid);
    if (SAL_SOK != ret_val_mutex)
    {
        PRT_INFO("destroy mutex in DSPTTK_COND_T failed\n");
    }

    return (ret_val_mutex | ret_val_cond);
}


/*======================= POSIX读写锁 =======================*/

/**
 * @fn      rwlockInit
 * @brief   初始化读写锁，并设置写锁优先
 *  
 * @param   rwlock_id[OUT] 读写锁ID
 * 
 * @return  SAL_SOK-初始化读写锁成功，SAL_FAIL-初始化读写锁失败
 */
SAL_STATUS dspttk_rwlock_init(pthread_rwlock_t *rwlock_id)
{
    int ret_val = 0;
    pthread_rwlockattr_t attr;

    if (NULL == rwlock_id)
    {
        PRT_INFO("rwlock_id is NULL\n");
        return SAL_FAIL;
    }

    pthread_rwlockattr_init(&attr);

    // 设置写锁优先
    ret_val = pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_rwlockattr_setkind_np failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
        goto ERR_EXIT;
    }

    #if 1 //def _POSIX_SHARED_MEMORY_OBJECTS
    ret_val = pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_rwlockattr_setpshared failed, errno: %d, %s\n", ret_val, strerror(ret_val));
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
        PRT_INFO("pthread_rwlock_init failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        ret_val = SAL_FAIL;
    }

ERR_EXIT:
    pthread_rwlockattr_destroy(&attr);

    return ret_val;
}


/**
 * @fn      dspttk_rwlock_rdlock
 * @brief   尝试在一定时间内对rdlock上锁
 * 
 * @param   rwlock_id[IN] 读写锁ID
 * @param   wait_ms[IN] 等待上锁的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * 
 * @return  SAL_SOK-上锁成功，SAL_FAIL-上锁失败
 */
SAL_STATUS dspttk_rwlock_rdlock(pthread_rwlock_t *rwlock_id, long wait_ms)
{
    int ret_val = 0;
    struct timespec timeout = {0};
    unsigned long ts = 0, te = 0, tl = (unsigned long)wait_ms;

    if (NULL == rwlock_id)
    {
        PRT_INFO("rwlock_id is NULL\n");
        return SAL_FAIL;
    }

    if (wait_ms == SAL_TIMEOUT_NONE)
    {
        ret_val = pthread_rwlock_tryrdlock(rwlock_id);
    }
    else if (wait_ms < 0) 
    {
        ret_val = pthread_rwlock_rdlock(rwlock_id);
    }
    else
    {
        do
        {
            tl = tl + ts - te;
            ts = get_tick_count();
            map_ms_to_timespec(tl, &timeout);
            ret_val = pthread_rwlock_timedrdlock(rwlock_id, &timeout);
            if (ETIMEDOUT == ret_val) // 如果时间到后，条件还没有发生，那么会返回ETIMEDOUT错误
            {
                te = get_tick_count();
            }
            else
            {
                break;
            }
        } while (ts + tl > te);
    }

    if (0 != ret_val)
    {
        if (ETIMEDOUT != ret_val)
        {
            PRT_INFO("pthread_rwlock_rdlock failed, wait_ms: %ld, errno: %d, %s\n", wait_ms, ret_val, strerror(ret_val));
        }
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_rwlock_wrlock
 * @brief   尝试在一定时间内对wrlock上锁
 * 
 * @param   rwlock_id[IN] 读写锁ID
 * @param   wait_ms[IN] 等待上锁的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * 
 * @return  SAL_SOK-上锁成功，SAL_FAIL-上锁失败
 */
SAL_STATUS dspttk_rwlock_wrlock(pthread_rwlock_t *rwlock_id, long wait_ms)
{
    int ret_val = 0;
    struct timespec timeout = {0};
    unsigned long ts = 0, te = 0, tl = (unsigned long)wait_ms;

    if (NULL == rwlock_id)
    {
        PRT_INFO("rwlock_id is NULL\n");
        return SAL_FAIL;
    }

    if (wait_ms == SAL_TIMEOUT_NONE)
    {
        ret_val = pthread_rwlock_trywrlock(rwlock_id);
    }
    else if (wait_ms < 0) 
    {
        ret_val = pthread_rwlock_wrlock(rwlock_id);
    }
    else
    {
        do
        {
            tl = tl + ts - te;
            ts = get_tick_count();
            map_ms_to_timespec(tl, &timeout);
            ret_val = pthread_rwlock_timedwrlock(rwlock_id, &timeout);
            if (ETIMEDOUT == ret_val) // 如果时间到后，条件还没有发生，那么会返回ETIMEDOUT错误
            {
                te = get_tick_count();
            }
            else
            {
                break;
            }
        } while (ts + tl > te);
    }

    if (0 != ret_val)
    {
        if (ETIMEDOUT != ret_val)
        {
            PRT_INFO("pthread_rwlock_wrlock failed, wait_ms: %ld, errno: %d, %s\n", wait_ms, ret_val, strerror(ret_val));
        }
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_rwlock_unlock
 * @brief   对rwlock_id解锁
 * 
 * @param   rwlock_id[IN] 读写锁ID
 * 
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS dspttk_rwlock_unlock(pthread_rwlock_t *rwlock_id)
{
    int ret_val = 0;

    if (NULL == rwlock_id)
    {
        PRT_INFO("rwlock_id is NULL\n");
        return SAL_FAIL;
    }

    ret_val = pthread_rwlock_unlock(rwlock_id);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_rwlock_unlock failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/**
 * @fn      dspttk_rwlock_destroy
 * @brief   销毁读写锁
 * 
 * @param   rwlock_id[IN] 读写锁ID
 * 
 * @return  SAL_SOK-解锁成功，SAL_FAIL-解锁失败
 */
SAL_STATUS dspttk_rwlock_destroy(pthread_rwlock_t *rwlock_id)
{
    int ret_val = 0;

    if (NULL == rwlock_id)
    {
        PRT_INFO("rwlock_id is NULL\n");
        return SAL_FAIL;
    }

    ret_val = pthread_rwlock_destroy(rwlock_id);
    if (0 != ret_val)
    {
        PRT_INFO("pthread_rwlock_unlock failed, errno: %d, %s\n", ret_val, strerror(ret_val));
        return SAL_FAIL;
    }

    return SAL_SOK;
}


/*================= POSIX信号量/共享内存 =================*/

/**
 * 注：
 * 信号量的主要目的是提供一种进程间同步的方式。这种同步的进程可以共享也可以不共享内存区。
 * 虽然信号量的意图在于进程间的同步，互斥量和条件变量的意图在于线程间同步，
 * 但信号量也可用于线程间同步，互斥量和条件变量也可通过共享内存区进行进程间同步。
 * 应该根据应用场景、效率和易用性进行具体选择。
 */

/**
 * 有名信号量和无名信号量：
 * POSIX信号量有两种：有名信号量和无名信号量，无名信号量也被称作基于内存的信号量。
 * 有名信号量通过IPC名字进行进程间的同步，而无名信号量如果不是放在进程间的共享内存区中，
 * 是不能用来进行进程间同步的，只能用来进行线程同步。
 */

/**
 * @fn      dspttk_sem_init
 * @brief   创建无名信号量，并为信号量赋初值
 * @warning 同一信号量不能调用两次semInit，若不再使用，需调用semDestroy销毁
 *
 * @param   sem[OUT] 无名信号量，注意是sem_t类型
 * @param   sem_value[IN] 无名信号量初始值
 *
 * @return  SAL_SOK-初始化成功；SAL_FAIL-初始化失败
 */
SAL_STATUS dspttk_sem_init(OUT sem_t *sem, IN unsigned int sem_value)
{
    int ret_val = 0;

    if (NULL == sem)
    {
        PRT_INFO("sem is NULL\n");
        return SAL_FAIL;
    }

    /**
     * int sem_init(sem_t *sem, int pshared, unsigned int value);
     *
     * sem_init() initializes the unnamed semaphore at the address pointed to by 'sem'.
     *
     * The 'value' argument specifies the initial value for the semaphore.
     *
     * The 'pshared' argument indicates whether this semaphore is to
     * be shared between the threads of a process, or between
     * processes. If 'pshared' has the value 0, then the semaphore is
     * shared between the threads of a process. If 'pshared' is
     * nonzero, then the semaphore is shared between processes, and
     * should be located in a region of shared memory.
     */
    /**
     * 注：sem_init()用于无名信号量的初始化。
     * 无名信号量在初始化前一定要在内存中分配一个sem_t信号量类型的对象，
     * 这就是无名信号量又称为基于内存的信号量的原因。
     * 无名信号量不使用任何类似O_CREAT的标志，这表示sem_init()总是会初始化信号量的值，
     * 所以对于特定的一个信号量，我们必须保证只调用sem_init()进行初始化一次，
     * 对于一个已初始化过的信号量调用sem_init()的行为是未定义的
     */
    ret_val = sem_init(sem, 0, sem_value);

    if (0 == ret_val)
    {
        ret_val = SAL_SOK;
    }
    else
    {
        PRT_INFO("sem_init failed, errno: %d, %s\n", errno, strerror(errno));
        ret_val = SAL_FAIL;
    }

    return ret_val;
}


/**
 * @fn      dspttk_sem_destroy
 * @brief   销毁无名信号量
 * 
 * @param   sem[IN] 无名信号量，注意必须已初始化
 * 
 * @return  SAL_SOK-销毁成功；SAL_FAIL-销毁失败
 */
SAL_STATUS dspttk_sem_destroy(IN sem_t *sem)
{
    int ret_val = 0;

    if (NULL == sem)
    {
        PRT_INFO("sem is NULL\n");
        return SAL_FAIL;
    }

    ret_val = sem_destroy(sem);
    if (0 == ret_val)
    {
        ret_val = SAL_SOK;
    }
    else
    {
        ret_val = SAL_FAIL;
        PRT_INFO("sem_destroy failed, errno: %d, %s\n", errno, strerror(errno));
    }

    return ret_val;
}


/**
 * @fn      dspttk_sem_open
 * @brief   打开或创建一个有名信号量
 * 
 * @param   name[IN] 信号量名称，以“/”开头，且名字中不能包含其他的“/”，最长不超过_POSIX_NAME_MAX
 * @param   oflag[IN] 打开方式，参数可为0、O_CREAT、O_EXCL，
 *                    0：打开一个已存在的信号量，
 *                    O_CREAT：如果信号量不存在就创建一个信号量，如果存在则打开被返回，此时mode和value需要指定
 *                    O_CREAT|O_EXCL：如果信号量不存在就创建一个信号量，如果信号量已存在则返回错误
 * @param   mode[IN]  信号量的权限位，和open函数一样，包括：S_IRUSR，S_IWUSR，S_IRGRP，S_IWGRP，S_IROTH，S_IWOTH
 *                    常用的可以用宏定义IPC_RDWR、IPC_RDONLY、IPC_WRONLY
 * @param   value[IN] 创建信号量时，信号量的初始值，取值范围[0, SEM_VALUE_MAX] 
 *  
 * @return  NULL-打开或创建有名信号量失败；其他-打开或创建成功
 */
sem_t *dspttk_sem_open(IN const char *name, IN int oflag, ...)
{
    sem_t *ret_val = NULL;
    mode_t mode = 0;
    unsigned int value = 0;
    va_list args;

    if (NULL != name)
    {
        if (strlen(name) > 1)
        {
            if (name[0] != '/' && NULL != strchr(name+1, '/'))
            {
                PRT_INFO("the name is invalid: %s\n", name);
                return NULL;
            }
        }
        else
        {
            PRT_INFO("the name is too short: %s\n", name);
            return NULL;
        }
    }
    else
    {
        PRT_INFO("the name is NULL\n");
        return NULL;
    }

    if (0 == oflag)
    {
        ret_val = sem_open(name, 0);
    }
    else
    {
        va_start(args, oflag);
        mode = va_arg(args, mode_t);
        value = va_arg(args, unsigned int);
        va_end(args);
        ret_val = sem_open(name, oflag, mode, value);
    }

    if (SEM_FAILED != ret_val)
    {
        return ret_val;
    }
    else
    {
        PRT_INFO("sem_open '%s' failed, errno: %d, %s\n", name, errno, strerror(errno));
        return NULL;
    }
}


/**
 * @fn      dspttk_sem_close
 * @brief   关闭一个有名信号量，但并没有把它从系统中删除
 * @note    从系统中删除有名信号量需调用接口semUnlink
 * 
 * @param   sem[IN] 信号量
 * 
 * @return  SAL_SOK-关闭有名信号量成功；SAL_FAIL-关闭失败
 */
SAL_STATUS dspttk_sem_close(IN sem_t *sem)
{
    int ret_val = 0;

    if (NULL == sem)
    {
        PRT_INFO("the sem is NULL\n");
        return SAL_FAIL;
    }

    /**
     * 当一个进程终止时，内核对其上仍然打开的所有有名信号量自动执行sem_close操作
     * 调用sem_close关闭信号量并没有把它从系统中删除它，POSIX有名信号量是随内核持续的
     * 使当前没有进程打开某个信号量它的值依然保持
     */
    ret_val = sem_close(sem);
    if (0 == ret_val)
    {
        return SAL_SOK;
    }
    else
    {
        PRT_INFO("sem_close failed, errno: %d, %s\n", errno, strerror(errno));
        return SAL_FAIL;
    }
}


/**
 * @fn      dspttk_sem_unlink
 * @brief   从系统中删除有名信号量
 * 
 * @param   name[IN] 有名信号量名称，以“/”开头
 * 
 * @return  SAL_SOK-删除有名信号量成功；SAL_FAIL-删除失败
 */
SAL_STATUS dspttk_sem_unlink(const char *name)
{
    int ret_val = 0;

    if (NULL == name)
    {
        PRT_INFO("the name is NULL\n");
        return SAL_FAIL;
    }

    ret_val = sem_unlink(name);
    if (0 == ret_val)
    {
        return SAL_SOK;
    }
    else
    {
        PRT_INFO("sem_unlink failed, errno: %d, %s\n", errno, strerror(errno));
        return SAL_FAIL;
    }
}


/**
 * @fn      dspttk_sem_give
 * @brief   挂出一个信号量，该操作将信号量的值加1，若有进程阻塞着等待该信号量，则其中一个进程将被唤醒。
 * 
 * @param   sem[IN] 信号量，注意必须已初始化
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 * 
 * @return  SAL_SOK-信号量挂出成功；SAL_FAIL-信号量挂出失败，sem_post调用失败
 */
SAL_STATUS dspttk_sem_give(IN sem_t *sem, IN const char *function, IN const unsigned int line)
{
    int ret_val = 0;

    if (NULL == sem)
    {
        PRT_INFO("sem is NULL\n");
        return SAL_FAIL;
    }

    ret_val = sem_post(sem);
    if (0 == ret_val)
    {
        ret_val = SAL_SOK;
    }
    else
    {
        ret_val = SAL_FAIL;
        if (NULL != function)
        {
            PRT_INFO("sem_post failed @ FUNCTION[%s]-LINE[%u], errno: %d, %s\n", \
                    function, line, errno, strerror(errno));
        }
        else
        {
            PRT_INFO("sem_post failed, errno: %d, %s\n", errno, strerror(errno));
        }
    }

    return ret_val;

}


/**
 * @fn      dspttk_sem_take
 * @brief   等待一个信号量，通过wait_ms控制阻塞时间，若信号量大于0，等待进程将信号量的值减1，并获得共享资源的访问权限
 * 
 * @param   sem[IN] 信号量，注意必须已初始化
 * @param   wait_ms[IN] 等待信号量大于0的阻塞时间，单位：ms，SAL_TIMEOUT_FOREVER表示永久阻塞，SAL_TIMEOUT_NONE表示不阻塞
 * @param   function[IN] 该函数的调用函数名，一般输入为“__FUNCTION__”，仅作错误打印输出，可为NULL
 * @param   line[IN] 该函数的调用行，一般输入为“__LINE__”，仅作错误打印输出，可为0
 * 
 * @return  SAL_SOK-等待信号量成功，并将信号量值减1；SAL_FAIL-在wait_ms时间内未等待到信号量或sem_wait系列函数调用失败
 */
SAL_STATUS dspttk_sem_take(IN sem_t *sem, IN long wait_ms, IN const char *function, IN const unsigned int line)
{
    int ret_val = -1;
    struct timespec timeout = {0};
    unsigned long ts = 0, te = 0, tl = (unsigned long)wait_ms;

    if (NULL == sem)
    {
        PRT_INFO("sem is NULL\n");
        return SAL_FAIL;
    }

    if (wait_ms < 0)
    {
        /**
         * 首先会测试指定信号量的值，如果大于0，就会将它减1并立即返回，
         * 如果等于0，那么调用线程会进入睡眠，等待信号量的值大于0
         */
        ret_val = sem_wait(sem);
    }
    else if (wait_ms == SAL_TIMEOUT_NONE)
    {
        /**
         * 和sem_wait的差别是：
         * 当信号量的值等于0时，调用线程不会阻塞，直接返回，并标识'EAGAIN'错误。
         */
        ret_val = sem_trywait(sem);
    }
    else
    {
        do
        {
            tl = tl + ts - te;
            ts = get_tick_count();
            map_ms_to_timespec(tl, &timeout);
            /**
             * 和sem_wait的差别是：
             * 当信号量的值等于0时，调用线程会限时等待。
             * 当等待时间到后，信号量的值还是0，直接返回，并标识'ETIMEDOUT'错误
             */
            ret_val = sem_timedwait(sem, &timeout);
            if (ret_val < 0 && ETIMEDOUT == errno)
            {
                te = get_tick_count();
            }
            else
            {
                break;
            }
        } while (ts + tl > te);
    }

    if (0 == ret_val)
    {
        ret_val = SAL_SOK;
    }
    else
    {
        if (ETIMEDOUT != errno)
        {
            if (NULL != function)
            {
                PRT_INFO("sem_wait failed @ FUNCTION[%s]-LINE[%u], wait_ms: %ld, errno: %d, %s\n", \
                        function, line, wait_ms, errno, strerror(errno));
            }
            else
            {
                PRT_INFO("sem_wait failed, wait_ms: %ld, errno: %d, %s\n", wait_ms, errno, strerror(errno));
            }
        }
        ret_val = SAL_FAIL;
    }

    return ret_val;
}


/**
 * @fn      dspttk_sem_get_value
 * @brief   获取信号量的值
 * 
 * @param   sem[IN] 信号量，注意必须已初始化
 * 
 * @return  >=0：信号量的值，<0：获取失败
 */
INT32 dspttk_sem_get_value(IN sem_t *sem)
{
    INT32 s32SemValue = -1;

    if ((NULL != sem) && (0 == sem_getvalue(sem, &s32SemValue)))
    {
        return s32SemValue;
    }
    else
    {
        PRT_INFO("sem_getvalue failed, errno: %d, %s\n", errno, strerror(errno));
        return -1;
    }
}


/**
 * @fn      dspttk_shm_open
 * @brief   通过共享内存标识符，打开或创建一个指定大小的共享内存
 * 
 * @param   key[IN] 共享内存标识符，实际为int型
 * @param   mem_size[IN] 共享内存大小，单位：字节
 * @param   flag[IN] 共享内存打开标识：(IPC_RDWR/IPC_RDONLY/IPC_WRONLY) | IPC_CREAT | IPC_EXCL
 * 
 * @return  非NULL：共享内存的指针，NULL：打开或创建失败
 */
void *dspttk_shm_open(key_t key, unsigned int mem_size, int flag)
{
    int shmid = -1;
    void *p_addr = NULL;

    shmid = shmget(key, mem_size, flag);
    if (shmid < 0) // On error, -1 is returned, and errno is set to indicate the error.
    {
        PRT_INFO("shmget failed, key: 0x%x, errno: %d, %s\n", key, errno, strerror(errno));
        return NULL;
    }

    p_addr = shmat(shmid, (void *)0, 0);
    if ((void *)-1 == p_addr)
    {
        PRT_INFO("shmat failed, key: 0x%x, errno: %d, %s\n", key, errno, strerror(errno));
        return NULL;
    }
    else
    {
        return p_addr;
    }
}


/**
 * @fn      dspttk_shm_close
 * @brief   通过共享内存在进程中的虚拟地址，以关闭该共享内存
 * @note    将共享内存关闭并不是删除它，只是使该共享内存对当前进程不再可用
 *
 * @param   shm_addr[IN] 共享内存地址
 * 
 * @return  SAL_SOK-关闭共享内存成功；SAL_FAIL-关闭失败
 */
SAL_STATUS dspttk_shm_close(const void *shm_addr)
{
    int ret_val = 0;

    ret_val = shmdt(shm_addr);

    if (0 == ret_val)
    {
        return SAL_SOK;
    }
    else
    {
        PRT_INFO("shmdt failed, errno: %d, %s\n", errno, strerror(errno));
        return SAL_FAIL;
    }
}


/**
 * @fn      dspttk_shm_destroy
 * @brief   删除共享内存段
 * 
 * @param   key[IN] 共享内存标识符，即shmOpen接口中的第一个参数
 * 
 * @return  SAL_SOK-删除共享内存成功；SAL_FAIL-删除失败
 */
SAL_STATUS dspttk_shm_destroy(const key_t key)
{
    int ret_val = -1;
    int shmid = 0;

    shmid = shmget(key, 0, IPC_RDWR);
    if (shmid < 0)
    {
        PRT_INFO("shmget failed, key: 0x%x, errno: %d, %s\n", key, errno, strerror(errno));
        return SAL_FAIL;
    }

    ret_val = shmctl(shmid, IPC_RMID, NULL);
    if (ret_val < 0)
    {
        PRT_INFO("shmctl IPC_RMID failed, key: 0x%x, errno: %d, %s\n", key, errno, strerror(errno));
        return SAL_FAIL;
    }
    else
    {
        return SAL_SOK;
    }
}


/*=================== Doubly Linked List ===================*/

/**
DESCRIPTION
This subroutine library supports the creation and maintenance of a
doubly linked list.  The user supplies a list descriptor (type DSPTTK_LIST)
that will contain pointers to the first and last nodes in the list,
and a count of the number of nodes in the list.  The nodes in the
list can be any user-defined structure, but they must reserve space
for two pointers as their first elements.  Both the forward and
backward chains are terminated with a NULL pointer.

The linked-list library simply manipulates the linked-list data structures;
no kernel functions are invoked.  In particular, linked lists by themselves
provide no task synchronization or mutual exclusion.  If multiple tasks will
access a single linked list, that list must be guarded with some
mutual-exclusion mechanism (e.g., a mutual-exclusion semaphore).

NON-EMPTY DSPTTK_LIST:
.CS
   ---------             --------          --------
   | head--------------->| next----------->| next---------
   |       |             |      |          |      |      |
   |       |       ------- prev |<---------- prev |      |
   |       |       |     |      |          |      |      |
   | tail------    |     | ...  |    ----->| ...  |      |
   |       |  |    v                 |                   v
   |count=2|  |  -----               |                 -----
   ---------  |   ---                |                  ---
              |    -                 |                   -
              |                      |
              ------------------------
.CE

EMPTY DSPTTK_LIST:
.CS
    -----------
        |  head------------------
        |         |             |
        |  tail----------       |
        |         |     |       v
        | count=0 |   -----   -----
        -----------    ---     ---
                        -       -
.CE

*/

typedef struct
{
    DSPTTK_LIST list;
    uint32_t node_num;
    DSPTTK_NODE *node_buf;
    uint32_t node_idle;
} DSPTTK_LIST_INNER;


/**
 * @fn      dspttk_lst_get_head
 * @brief   peek the first node in a list
 * 
 * @param   list[IN] the list
 * 
 * @return  a pointer to the first node, or NULL if the list is empty
 */
DSPTTK_NODE *dspttk_lst_get_head(DSPTTK_LIST *list)
{
    return (list->head);
}


/**
 * @fn      dspttk_lst_get_tail
 * @brief   peek the last node in a list
 * 
 * @param   list[IN] the list
 * 
 * @return  a pointer to the last node, or NULL if the list is empty
 */
DSPTTK_NODE *dspttk_lst_get_tail(DSPTTK_LIST *list)
{
    return (list->tail);
}


/**
 * @fn      dspttk_lst_is_full
 * @brief   judge the list is FULL
 * 
 * @param   list[IN] the list
 * 
 * @return  SAL_TRUE - full, SAL_FALSE - not full
 */
BOOL dspttk_lst_is_full(DSPTTK_LIST *list)
{
    if (list->count == ((DSPTTK_LIST_INNER *)list)->node_num)
    {
        return SAL_TRUE;
    }
    else
    {
        return SAL_FALSE;
    }
}

/**
 * @fn      dspttk_lst_is_empty
 * @brief   judge the list is EMPTY
 * 
 * @param   list[IN] the list
 * 
 * @return  SAL_TRUE - empty, SAL_FALSE - not empty
 */
BOOL dspttk_lst_is_empty(DSPTTK_LIST *list)
{
    if (list->count == 0)
    {
        return SAL_TRUE;
    }
    else
    {
        return SAL_FALSE;
    }
}


/**
 * @fn      dspttk_lst_get_count
 * @brief   get the number of nodes in a list
 * 
 * @param   list[IN] the list
 * 
 * @return  the number of nodes in the list
 */
uint32_t dspttk_lst_get_count(DSPTTK_LIST *list)
{
    return (list->count);
}


/**
 * @fn      dspttk_lst_get_idle_node
 * @brief   get an idle node in a list
 * 
 * @param   list[IN] the list
 * 
 * @return  a pointer to an idle node, OR NULL if there's non idle node
 */
DSPTTK_NODE *dspttk_lst_get_idle_node(DSPTTK_LIST *list)
{
    uint32_t i = 0;
    DSPTTK_LIST_INNER *list_inner = (DSPTTK_LIST_INNER *)list;
    DSPTTK_NODE *node_cur = NULL;

    if (list->count == list_inner->node_num)
    {
        return NULL;
    }

    node_cur = list_inner->node_buf + list_inner->node_idle;
    for (i = list_inner->node_idle; i < list_inner->node_num; i++)
    {
        if (NULL == node_cur->next && NULL == node_cur->prev && node_cur != list->head)
        {
            list_inner->node_idle = i;
            return node_cur;
        }
        node_cur++;
    }

    node_cur = list_inner->node_buf;
    for (i = 0; i < list_inner->node_idle; i++)
    {
        if (NULL == node_cur->next && NULL == node_cur->prev && node_cur != list->head)
        {
            list_inner->node_idle = i;
            return node_cur;
        }
        node_cur++;
    }

    return NULL;
}


/**
 * @fn      dspttk_lst_inst
 * @brief   insert a node in a list after a specified node
 * 
 * @param   list[IN] the list
 * @param   prev[IN] The new node is placed following the list node <prev>. 
 *                   If <prev> is NULL, the node is inserted at the head of the list. 
 * @param   node[IN] the new node
 */
void dspttk_lst_inst(DSPTTK_LIST *list, DSPTTK_NODE *prev, DSPTTK_NODE *node)
{
    DSPTTK_NODE *next = NULL;

    if (prev == NULL)
    {
        next = list->head;
        list->head = node;
    }
    else
    {
        next = prev->next;
        prev->next = node;
    }

    if (next == NULL)
    {
        list->tail = node;
    }
    else
    {
        next->prev = node;
    }

    node->next = next;
    node->prev = prev;

    list->count++;

    return;
}


/**
 * @fn      dspttk_lst_push
 * @brief   push a node to the end of a list
 * 
 * @param   list[IN] the list
 * @param   node[IN] the new node
 */
void dspttk_lst_push(DSPTTK_LIST *list, DSPTTK_NODE *node)
{
    dspttk_lst_inst(list, list->tail, node);
}


/**
 * @fn      dspttk_lst_delete
 * @brief   delete a specified node from a list
 * 
 * @param   list[IN] the list
 * @param   node[IN] the node which needs to be deleted
 */
void dspttk_lst_delete(DSPTTK_LIST *list, DSPTTK_NODE *node)
{
    BOOL b_first_node = SAL_FALSE, b_last_node = SAL_FALSE;

    if (node->prev == NULL)
    {
        if (list->head == node) // the node should be the head
        {
            b_first_node = SAL_TRUE;
        }
        else // dummy node
        {
            PRT_INFO("the node[%p] should be equal to the head[%p] of list\n", node, list->head);
            return;
        }
    }
    else
    {
        if (node->prev->next != node) // dummy node
        {
            PRT_INFO("the node[%p] should be equal to the prev->next[%p] of list\n", node, node->prev->next);
            return;
        }
    }

    if (node->next == NULL)
    {
        if (list->tail == node) // the node should be the tail
        {
            b_last_node = SAL_TRUE;
        }
        else // dummy node
        {
            PRT_INFO("the node[%p] should be equal to the tail[%p] of list\n", node, list->tail);
            return;
        }
    }
    else
    {
        if (node->next->prev != node) // dummy node
        {
            PRT_INFO("the node[%p] should be equal to the next->prev[%p] of list\n", node, node->next->prev);
            return;
        }
    }

    if (b_first_node)
    {
        list->head = node->next;
    }
    else
    {
        node->prev->next = node->next;
    }

    if (b_last_node)
    {
        list->tail = node->prev;
    }
    else
    {
        node->next->prev = node->prev;
    }

    list->count--;
    node->next = NULL;
    node->prev = NULL;

    return;
}


/**
 * @fn      dspttk_lst_pop
 * @brief   delete the first node from a list
 * 
 * @param   list[IN] the list
 */
void dspttk_lst_pop(DSPTTK_LIST *list)
{
    DSPTTK_NODE *node = list->head;

    if (node != NULL)
    {
        list->head = node->next;

        if (node->next == NULL) // there's only one node in the list
        {
            list->tail = NULL; // the list is empty now
        }
        else
        {
            node->next->prev = NULL;
        }

        list->count--;
        node->next = NULL;
        node->prev = NULL;
    }

    return;
}


/**
 * @fn      dspttk_lst_search
 * @brief   search a node in a list and return the index of the node
 * @note    the first node's index is 0 
 *  
 * @param   list[IN] the list
 * @param   node[IN] the node needs to be searched
 * 
 * @return  The node index(form 0), or -1 if the node is not found
 */
int32_t dspttk_lst_search(DSPTTK_LIST *list, DSPTTK_NODE *node)
{
    int32_t index = 0;
    DSPTTK_NODE *node_cur = list->head;

    while (node_cur != NULL)
    {
        if (node_cur != node)
        {
            index++;
            node_cur = node_cur->next;
        }
        else
        {
            break;
        }
    }

    if (node_cur != NULL)
    {
        return index;
    }
    else
    {
        return -1;
    }
}


/**
 * @fn      dspttk_lst_locate
 * @brief   locate the index-th(from 0) node in a list
 * 
 * @param   list[IN] the list
 * @param   index[IN] the node which needs to be located,indx i as a strat
 * 
 * @return  a pointer to the index-th node, or NULL if non-existent
 */
DSPTTK_NODE *dspttk_lst_locate(DSPTTK_LIST *list, int32_t index)
{
    DSPTTK_NODE *node = NULL;

    if (0 == list->count)
    {
        PRT_INFO("the list is empty\n");
        return NULL;
    }

    /* verify node number is in list */
    if ((index < 0) || (index >= list->count))
    {
        PRT_INFO("the index[%d] is invalid, range: [0, %d]\n", index, list->count-1);
        return NULL;
    }

    /**
     * if index is less than half way, look forward from beginning; 
     * otherwise look back from end 
     */
    if (index < (list->count >> 1))
    {
        node = list->head;
        while (index-- > 0)
        {
            node = node->next;
        }
    }
    else
    {
        node = list->tail;
        index -= (int32_t)list->count;
        while (++index < 0)
        {
            node = node->prev;
        }
    }

    return node;
}


/**
 * @fn      dspttk_lst_nstep
 * @brief   find a list node <n_step> steps away from a specified node
 * 
 * @param   node[IN] the node
 * @param   n_step[IN] If <n_step> is positive, it steps toward the tail
 *                     If <n_step> is negative, it steps toward the head
 * 
 * @return  a pointer to the node <n_step> steps away, or NULL if the node is out of range
 */
DSPTTK_NODE *dspttk_lst_nstep(DSPTTK_NODE *node, int32_t n_step)
{
    DSPTTK_NODE *temp = NULL;
    int32_t i = 0;
    int32_t count = (n_step < 0) ? -n_step : n_step;
    
    if (node == NULL)
    {
        PRT_INFO("cur node null err \n");
        return temp;
    }
    
    temp = node;
    for (i = 0; i < count; i++)
    {
        if (n_step < 0)
        {
            temp = temp->prev;
        }
        else if (n_step > 0)
        {
            temp = temp->next;
        }

        if (temp == NULL)
        {
            break;
        }
    }

    return temp;
}


/**
 * @fn      dspttk_lst_init
 * @brief   initializes a statistic list includes <node_num> nodes
 * 
 * @param   node_num[IN] the number of nodes in the list
 * 
 * @return  a pointer to a new list, OR NULL if init failed
 */
DSPTTK_LIST *dspttk_lst_init(uint32_t node_num)
{
    uint32_t list_buf_size = 0;
    DSPTTK_LIST_INNER *list_inner = NULL;

    /**
     *    DSPTTK_LIST_INNER    NODE_BUF
     *    ↓             ↓              
     *    ┌─────────────┬──────────────┐
     *    └─────────────┴──────────────┘
     *                  ├── node_num ──┤
     */
    list_buf_size = sizeof(DSPTTK_LIST_INNER) + node_num * sizeof(DSPTTK_NODE);
    list_inner = (DSPTTK_LIST_INNER *)malloc(list_buf_size);

    if (NULL != list_inner)
    {
        memset(list_inner, 0, list_buf_size);
        list_inner->node_num = node_num;
        list_inner->node_buf = (DSPTTK_NODE *)((U08 *)list_inner + sizeof(DSPTTK_LIST_INNER));
        if (SAL_SOK != dspttk_cond_init(&list_inner->list.sync))
        {
            PRT_INFO("sal_cond_init for 'list.sync' failed\n");
            free(list_inner);
            list_inner = NULL;
        }
    }
    else
    {
        PRT_INFO("malloc DSPTTK_LIST buffer failed, buffer size: %u\n", list_buf_size);
    }

    return (DSPTTK_LIST *)list_inner;
}


/**
 * @fn      dspttk_lst_deinit
 * @brief   turn all nodes to NULL, and free the list
 * 
 * @param   list[IN] the list
 */
void dspttk_lst_deinit(DSPTTK_LIST *list)
{
    DSPTTK_NODE *p1 = NULL, *p2 = NULL;

    if (NULL != list)
    {
        if (list->count > 0)
        {
            p1 = list->head;
            while (p1 != NULL)
            {
                p2 = p1->next;
                p1->next = NULL;
                p1->prev = NULL;
                p1 = p2;
            }
            list->count = 0;
            list->head = list->tail = NULL;
        }

        free(list);
        list = NULL;
    }

    return;
}