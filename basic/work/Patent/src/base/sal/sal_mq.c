/***
 * @file   sal_mq.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  message queue
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#include <errno.h>      /* errno */
#include <string.h>     /* strerror */
#include <time.h>       /* clock_gettime */
#include <sys/ipc.h>
#include "sal_type.h"
#include "sal_macro.h"
#include "sal_mq.h"
#include "sal_trace.h"
#include "sal_time.h"

#line __LINE__ "sal_mq.c"

extern void sal_tm_map_ms_2_timespec(IN const UINT64 wait_ms, OUT struct timespec *ts);

#if 0
struct mq_attr
{
    long int mq_flags;      /* Message queue flags，0或O_NONBLOCK，用来表示是否阻塞 */
    long int mq_maxmsg;     /* Maximum number of messages，0或O_NONBLOCK,用来表示是否阻塞 */
    long int mq_msgsize;    /* Maximum message size，消息队列中每个消息的最大字节数 */
    long int mq_curmsgs;    /* Number of messages currently queued，消息队列中当前的消息数目 */
    long int __reserved[4];
};
#endif

/**
 * @fn      SAL_MqInit
 * @brief   创建一个消息队列，并输出一个可读写的消息队列ID
 * @warning 多线程或多进程间消息队列只需要初始化一次，初始化后其他线程调用SAL_MqOpen即可
 *
 * @param   mq_id[OUT] 消息队列ID
 * @param   name[IN] 消息队列名，只能以一个’/’开头，且名字中不能包含其他的’/’
 * @param   maxmsg[IN] 消息队列的最大消息数
 * @param   msgsize[IN] 消息队列中每个消息的最大字节数
 *
 * @return  SAL_SOK-创建消息队列成功，SAL_FAIL-创建消息队列失败
 */
SAL_STATUS SAL_MqInit(mqd_t *mq_id, const CHAR *name, INT32 maxmsg, INT32 msgsize)
{
    mqd_t ret_val = 0;
    struct mq_attr mqa = {0};

    if (NULL == mq_id)
    {
        SAL_LOGE("the mq_id is NULL\n");
        return SAL_FAIL;
    }

    if (NULL != name)
    {
        if (strlen(name) > 1)
        {
            if (name[0] != '/' && NULL != strchr(name + 1, '/'))
            {
                SAL_LOGE("the name is invalid: %s\n", name);
                return SAL_FAIL;
            }
        }
        else
        {
            SAL_LOGE("the name is too short: %s\n", name);
            return SAL_FAIL;
        }
    }
    else
    {
        SAL_LOGE("the name is NULL\n");
        return SAL_FAIL;
    }

    mqa.mq_maxmsg = maxmsg;
    mqa.mq_msgsize = msgsize;

    // ret_val = mq_open(name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR + S_IWUSR, &mqa);
    ret_val = mq_open(name, O_CREAT | O_RDWR, S_IRUSR + S_IWUSR, &mqa);
    if (ret_val >= 0)
    {
        *mq_id = ret_val;
        return SAL_SOK;
    }
    else
    {
        SAL_LOGE("mq_open failed, name: %s, errno: %d, %s\n", name, errno, strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      SAL_MqOpen
 * @brief   以可读、可写或读写三种方式打开一个已创建的消息队列
 * @warning 打开后必须执行SAL_MqClose关闭，SAL_MqOpen - SAL_MqClose
 *
 * @param   mq_id[OUT] 打开的消息队列ID
 * @param   name[IN] 消息队列名
 * @param   oflag[IN] 打开方式，只支持三种方式O_RDONLY、O_WRONLY和O_RDWR
 *
 * @return  SAL_SOK-打开消息队列成功，SAL_FAIL-创建消息队列失败
 */
SAL_STATUS SAL_MqOpen(mqd_t *mq_id, const char *name, INT32 oflag)
{
    mqd_t ret_val = 0;

    if (NULL == mq_id || NULL == name)
    {
        SAL_LOGE("the mq_id[%p] OR name[%p] is NULL\n", mq_id, name);
        return SAL_FAIL;
    }

    if (oflag != O_RDONLY && oflag != O_WRONLY && oflag != O_RDWR)
    {
        SAL_LOGE("the oflag[%d] is unsupported, just support: O_RDONLY, O_WRONLY and O_RDWR\n", oflag);
        return SAL_FAIL;
    }

    ret_val = mq_open(name, oflag);
    if (ret_val >= 0)
    {
        *mq_id = ret_val;
        return SAL_SOK;
    }
    else
    {
        SAL_LOGE("mq_open failed, name: %s, errno: %d, %s\n", name, errno, strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      SAL_MqSend
 * @brief   向消息队列中增加一条消息
 *          Messages are placed on the queue in decreasing order of priority,
 *          with newer messages of the same priority being placed
 *          after older messages with the same priority.
 * @param   mq_id[IN] 消息队列ID
 * @param   msg[IN] 消息数据
 * @param   len[IN] 消息数据长度，不能超过创建消息队列时的属性msgsize
 * @param   wait_ms[IN] 阻塞时间，单位ms，不等待：SAL_TIMEOUT_NONE，永久等待：SAL_TIMEOUT_FOREVER
 * @param   msg_prio[IN] 优先级，仅支持三种：SALMQ_MSG_PRIO_HIGH、SALMQ_MSG_PRIO_DEFAULT和SALMQ_MSG_PRIO_LOW
 *
 * @return  SAL_SOK-发送消息成功，SAL_FAIL-发送消息失败
 */
SAL_STATUS SAL_MqSend(mqd_t mq_id, const void *msg, size_t len, UINT32 wait_ms, SALMQ_MSG_PRIO msg_prio)
{
    INT32 ret_val = SAL_FAIL;
    struct timespec timeout;
    UINT64 ts = 0, te = 0, tl = (UINT64)wait_ms;

    if (SAL_TIMEOUT_FOREVER == wait_ms)
    {
        /**
         * If the message queue is already full, then, by default, the message
         * to be queued, or until the call is interrupted by a signal handler.
         * If the O_NONBLOCK flag is enabled for the message queue description,
         * then the call instead fails immediately with the error EAGAIN.
         */
        ret_val = mq_send(mq_id, (char *)msg, len, msg_prio);
    }
    else
    {
        /**
         * mq_timedsend() behaves just like mq_send(), except that if the queue
         * is full and the O_NONBLOCK flag is not enabled for the message queue
         * description, then timeout points to a structure which specifies
         */
        do
        {
            tl = tl + ts - te;
            ts = sal_get_tickcnt();
            sal_tm_map_ms_2_timespec(tl, &timeout);
            ret_val = mq_timedsend(mq_id, (char *)msg, len, msg_prio, &timeout);
            if (ret_val < 0 && ETIMEDOUT == errno)
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

    if (ret_val < 0)
    {
        SAL_LOGE("mq_send failed, mq_id: %d, len: %zu, wait_ms: %d, errno: %d, %s\n", \
                 mq_id, len, wait_ms, errno, strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      SAL_MqRecv
 * @brief   从消息队列中读取一条最高优先级（msg_prio为NULL时）及最老的消息到msg_buf，并从消息队列中删除
 *          If the queue is empty, then, by default, mq_receive() blocks until a
 *          message becomes available, or the call is interrupted by a signal handler.
 * @param   mq_id[IN] 消息队列ID
 * @param   msg_buf[OUT] 消息队列buffer
 * @param   buf_len[IN] 消息队列buffer长度，必须超过创建消息队列时的属性msgsize
 * @param   wait_ms[IN] 阻塞时间，单位ms，不等待：SAL_TIMEOUT_NONE，永久等待：SAL_TIMEOUT_FOREVER
 * @param   msg_prio[IN] 指定读取相应优先级下最老的消息，为NULL时则读取最高优先级及最老的消息
 *                       If msg_prio is not NULL, then the buffer to which it points
 *                       is used to return the priority associated with the received message.
 *
 * @return  >0：接收到的消息长度，=0：等待超时，其他：接收失败
 */
ssize_t SAL_MqRecv(mqd_t mq_id, void *msg_buf, size_t buf_len, UINT32 wait_ms, unsigned *msg_prio)
{
    ssize_t ret_val = 0;
    struct timespec timeout;
    UINT64 ts = 0, te = 0, tl = (UINT64)wait_ms;

    if (SAL_TIMEOUT_FOREVER == wait_ms)
    {
        ret_val = mq_receive(mq_id, (char *)msg_buf, buf_len, msg_prio);
    }
    else
    {
        do
        {
            tl = tl + ts - te;
            ts = sal_get_tickcnt();
            sal_tm_map_ms_2_timespec(tl, &timeout);
            ret_val = mq_timedreceive(mq_id, (char *)msg_buf, buf_len, msg_prio, &timeout);
            if (ret_val < 0 && ETIMEDOUT == errno)
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

    if (ret_val < 0)
    {
        if (ETIMEDOUT == errno)
        {
            ret_val = 0;
        }
        else
        {
            SAL_LOGE("mq_receive failed, errno: %d, %s\n", errno, strerror(errno));
        }
    }

    return ret_val;
}

/**
 * @fn      SAL_MqGetMsgCnt
 * @brief   获取当前消息队列内消息数
 *
 * @param   mq_id[IN] 消息队列ID
 *
 * @return  >=0：消息数，<0：获取失败
 */
INT32 SAL_MqGetMsgCnt(mqd_t mq_id)
{
    INT32 ret_val = 0;
    struct mq_attr mqa = {0};

    ret_val = mq_getattr(mq_id, &mqa);
    if (ret_val < 0)
    {
        SAL_LOGE("mq_getattr failed, errno: %d, %s\n", errno, strerror(errno));
        return -1;
    }

    return mqa.mq_curmsgs;
}

/**
 * @fn      SAL_MqClose
 * @brief   关闭消息队列
 *
 * @param   mq_id[IN] 消息队列ID
 *
 * @return  SAL_SOK-关闭消息队列成功，SAL_FAIL-关闭消息队列失败
 */
SAL_STATUS SAL_MqClose(mqd_t mq_id)
{
    INT32 ret_val = 0;

    ret_val = mq_close(mq_id);
    if (ret_val < 0)
    {
        SAL_LOGE("mq_close failed, mq_id: %d, errno: %d, %s\n", mq_id, errno, strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @fn      SAL_MqDestroy
 * @brief   销毁消息队列
 * @warning 该接口内会首先执行SAL_MqClose关闭消息队列，然后再执行销毁
 *
 * @param   mq_id[IN] 消息队列ID
 * @param   name[IN] 消息队列名
 *
 * @return  SAL_SOK-销毁消息队列成功，SAL_FAIL-销毁消息队列失败
 */
SAL_STATUS SAL_MqDestroy(mqd_t mq_id, const char *name)
{
    INT32 ret_val = SAL_FAIL;

    SAL_MqClose(mq_id);

    ret_val = mq_unlink(name);
    if (ret_val < 0)
    {
        SAL_LOGE("mq_unlink failed, name: %s, errno: %d, %s\n", name, errno, strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

