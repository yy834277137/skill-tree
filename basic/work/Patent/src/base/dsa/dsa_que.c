/**
 * @file   dsa_que.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  队列操作
 * @author dsp
 * @date   2022/6/2
 * @note
 * @note \n History
   1.日    期: 2022/6/2
     作    者: dsp
     修改历史: 创建文件
 */


#include "dsa_que.h"
#include "sal.h"


/*******************************************************************************
* 函数名  : DSA_QueCreate
* 描  述  : 创建一个队列
            队列本体是一个指针数组，内部可存放maxLen个数的指针数据
* 输  入  : - phndl  : 队列结构体
*         : - u32maxLen: 最大队列长度
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 DSA_QueCreate(DSA_QueHndl *phndl, UINT32 u32maxLen)
{
    pthread_mutexattr_t mutex_attr;
    pthread_condattr_t cond_attr;
    INT32 s32Status = SAL_SOK;

    phndl->curRd = phndl->curWr = 0;
    phndl->count = 0;
    phndl->len = u32maxLen;
    phndl->queue = (void **)SAL_memZalloc(sizeof(void *) * phndl->len, "dsa_que", "que_ele");

    if (NULL == phndl->queue)
    {
        SAL_LOGE("DSA_QueCreate() = %d \n", s32Status);

        return SAL_FAIL;
    }

    s32Status |= pthread_mutexattr_init(&mutex_attr);
    s32Status |= pthread_condattr_init(&cond_attr);

    s32Status |= pthread_mutex_init(&phndl->lock, &mutex_attr);
    s32Status |= pthread_cond_init(&phndl->condRd, &cond_attr);
    s32Status |= pthread_cond_init(&phndl->condWr, &cond_attr);

    if (s32Status != SAL_SOK)
    {
        SAL_LOGE("DSA_QueCreate() = %d \n", s32Status);
    }

    pthread_condattr_destroy(&cond_attr);
    pthread_mutexattr_destroy(&mutex_attr);

    return s32Status;
}

/*******************************************************************************
* 函数名  : DSA_QueDelete
* 描  述  : 销毁一个队列
* 输  入  : - phndl: 队列结构体
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 DSA_QueDelete(DSA_QueHndl *phndl)
{
    if (NULL != phndl->queue)
    {
        SAL_memfree(phndl->queue, "dsa_que", "que_ele");
        phndl->queue = NULL;
    }

    pthread_cond_destroy(&phndl->condRd);
    pthread_cond_destroy(&phndl->condWr);
    pthread_mutex_destroy(&phndl->lock);

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : DSA_QuePut
* 描  述  : 往队列中放数据
* 输  入  : - phndl   : 队列结构体
*         : - pvalue : 放入队列的数据指针
*         : - u32TimeOut: 等待选项
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 DSA_QuePut(DSA_QueHndl *phndl, void *pvalue, UINT32 u32TimeOut)
{
    INT32 s32Status = SAL_FAIL;

    pthread_mutex_lock(&phndl->lock);

    while (1)
    {
        if (phndl->count < phndl->len)
        {
            phndl->queue[phndl->curWr] = pvalue;
            phndl->curWr = (phndl->curWr + 1) % phndl->len;
            phndl->count++;
            s32Status = SAL_SOK;
            pthread_cond_signal(&phndl->condRd);
            break;
        }
        else
        {
            if (SAL_TIMEOUT_NONE == u32TimeOut)
            {
                break;
            }

            s32Status = pthread_cond_wait(&phndl->condWr, &phndl->lock);
        }
    }

    pthread_mutex_unlock(&phndl->lock);

    return s32Status;
}

/*******************************************************************************
* 函数名  : DSA_QueGet
* 描  述  : 从队列中获取一帧数据
* 输  入  : - phndl   : 队列结构体
*         : - ppvalue: 从队列中获取地址的二级指针
*         : - u32TimeOut: 等待选项
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 DSA_QueGet(DSA_QueHndl *phndl, void **ppvalue, UINT32 u32TimeOut)
{
    INT32 s32Status = SAL_FAIL;

    pthread_mutex_lock(&phndl->lock);

    while (1)
    {
        if (phndl->count > 0)
        {

            if (NULL != ppvalue)
            {
                *ppvalue = phndl->queue[phndl->curRd];
            }

            phndl->curRd = (phndl->curRd + 1) % phndl->len;
            phndl->count--;
            s32Status = SAL_SOK;
            pthread_cond_signal(&phndl->condWr);
            break;
        }
        else
        {
            if (SAL_TIMEOUT_NONE == u32TimeOut)
            {
                break;
            }

            s32Status = pthread_cond_wait(&phndl->condRd, &phndl->lock);
        }
    }

    pthread_mutex_unlock(&phndl->lock);

    return s32Status;
}

/*******************************************************************************
* 函数名  : DSA_QueGetQueuedCount
* 描  述  : 队列缓存个数
* 输  入  : - phndl   : 队列结构体
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
UINT32 DSA_QueGetQueuedCount(DSA_QueHndl *phndl)
{
    UINT32 u321QueuedCount = 0;

    pthread_mutex_lock(&phndl->lock);
    u321QueuedCount = phndl->count;
    pthread_mutex_unlock(&phndl->lock);

    return u321QueuedCount;
}

/*******************************************************************************
* 函数名  : DSA_QuePeek
* 描  述  : 队列最新数据
* 输  入  : - phndl   : 队列结构体
*         : - ppvalue: 从队列中获取地址的二级指针
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 DSA_QuePeek(DSA_QueHndl *phndl, void **ppvalue)
{
    INT32 s32Status = SAL_FAIL;

    pthread_mutex_lock(&phndl->lock);
    if (phndl->count > 0)
    {
        if (NULL != ppvalue)
        {
            *ppvalue = phndl->queue[phndl->curRd];
            s32Status = SAL_SOK;
        }
    }

    pthread_mutex_unlock(&phndl->lock);

    return s32Status;
}

/*******************************************************************************
* 函数名  : DSA_QueIsEmpty
* 描  述  : 队列是否为空
* 输  入  : - phndl   : 队列结构体
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 DSA_QueIsEmpty(DSA_QueHndl *phndl)
{
    INT32 s32IsEmpty;

    pthread_mutex_lock(&phndl->lock);

    if (0 == phndl->count)
    {
        s32IsEmpty = SAL_TRUE;
    }
    else
    {
        s32IsEmpty = SAL_FALSE;
    }

    pthread_mutex_unlock(&phndl->lock);

    return s32IsEmpty;
}

