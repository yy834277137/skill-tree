/***
 * @file   sal_mq.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  message queue
 * @author liwenbin
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef _SAL_MQ_H_
#define _SAL_MQ_H_

#include <mqueue.h>

typedef enum
{
    SALMQ_MSG_PRIO_HIGH	= 2,        /* 消息队列高优先级消息，会被优先发送 */
    SALMQ_MSG_PRIO_DEFAULT = 1,     /* 消息队列中优先级消息 */
    SALMQ_MSG_PRIO_LOW = 0,         /* 消息队列低优先级消息，若出现竞争时，会被延迟发送 */
} SALMQ_MSG_PRIO;

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
SAL_STATUS SAL_MqInit(mqd_t *mq_id, const char *name, INT32 maxmsg, INT32 msgsize);

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
SAL_STATUS SAL_MqOpen(mqd_t *mq_id, const char *name, INT32 oflag);

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
SAL_STATUS SAL_MqSend(mqd_t mq_id, const void *msg, size_t len, UINT32 wait_ms, SALMQ_MSG_PRIO msg_prio);

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
ssize_t SAL_MqRecv(mqd_t mq_id, void *msg_buf, size_t buf_len, UINT32 wait_ms, unsigned *msg_prio);

/**
 * @fn      SAL_MqGetMsgCnt
 * @brief   获取当前消息队列内消息数
 *
 * @param   mq_id[IN] 消息队列ID
 *
 * @return  >=0：消息数，<0：获取失败
 */
INT32 SAL_MqGetMsgCnt(mqd_t mq_id);

/**
 * @fn      SAL_MqClose
 * @brief   关闭消息队列
 *
 * @param   mq_id[IN] 消息队列ID
 *
 * @return  SAL_SOK-关闭消息队列成功，SAL_FAIL-关闭消息队列失败
 */
SAL_STATUS SAL_MqClose(mqd_t mq_id);

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
SAL_STATUS SAL_MqDestroy(mqd_t mq_id, const char *name);

#endif

