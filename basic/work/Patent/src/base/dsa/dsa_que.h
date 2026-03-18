/**
 * @file   dsa_que.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  队列操作头文件
 * @author liwenbin
 * @date   2022/6/2
 * @note
 * @note \n History
   1.日    期: 2022/6/2
     作    者: liwenbin
     修改历史: 创建文件
 */

#ifndef _DSA_QUE_H_
#define _DSA_QUE_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "sal_type.h"
#include <pthread.h>
#include <stdlib.h>

/* 队列句柄*/
typedef struct
{
    UINT32 curRd;
    UINT32 curWr;
    UINT32 len;
    UINT32 count;

    void **queue;         /* 队列指针数组 */

    pthread_mutex_t lock;
    pthread_cond_t condRd;
    pthread_cond_t condWr;

} DSA_QueHndl;

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
INT32 DSA_QueCreate(DSA_QueHndl *phndl, UINT32 u32maxLen);

/*******************************************************************************
* 函数名  : DSA_QueDelete
* 描  述  : 销毁一个队列
* 输  入  : - phndl: 队列结构体
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 DSA_QueDelete(DSA_QueHndl *phndl);

/*******************************************************************************
* 函数名  : DSA_QuePut
* 描  述  : 往对列中放数据
* 输  入  : - phndl   : 队列结构体
*         : - pvalue : 放入队列的数据指针
*         : - u32TimeOut: 等待选项
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 DSA_QuePut(DSA_QueHndl *phndl, void *pvalue, UINT32 u32TimeOut);

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
INT32 DSA_QueGet(DSA_QueHndl *phndl, void **ppvalue, UINT32 u32TimeOut);

/*******************************************************************************
* 函数名  : DSA_QuePeek
* 描  述  : 队列最新数据
* 输  入  : - phndl   : 队列结构体
*         : - ppvalue: 从队列中获取地址的二级指针
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
INT32 DSA_QuePeek(DSA_QueHndl *phndl, void **ppvalue);

/*******************************************************************************
* 函数名  : DSA_QueIsEmpty
* 描  述  : 队列是否为空
* 输  入  : - phndl   : 队列结构体
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失
*******************************************************************************/
INT32 DSA_QueIsEmpty(DSA_QueHndl *phndl);

/*******************************************************************************
* 函数名  : DSA_QueGetQueuedCount
* 描  述  : 队列缓存个数
* 输  入  : - phndl   : 队列结构体
* 输  出  : 无
* 返回值  : SAL_SOK  : 成功
*           SAL_FAIL : 失败
*******************************************************************************/
UINT32 DSA_QueGetQueuedCount(DSA_QueHndl *phndl);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _SAL_QUE_H_ */

