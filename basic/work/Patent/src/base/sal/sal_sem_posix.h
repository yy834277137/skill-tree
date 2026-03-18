/***
 * @file   sal_sem_posix.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  本文件封装对linux下IPC的信号量的操作(posix标准)
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef _SAL_SEM_H_
#define _SAL_SEM_H_
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sem.h>    /*包含信号量定义的头文件*/
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
#define SEM_WAIT_NONE (0)

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/**
 * @function:   SAL_SemCreate
 * @brief:      创建无名信号量
 * @param[in]:  BOOL bShared
 * @param[in]:  UINT32 uiInitCount
 * @param[out]: None
 * @return:     VOID *
 */
VOID *SAL_SemCreate(BOOL bShared, UINT32 uiInitCount);

/**
 * @function:   SAL_SemDestroy
 * @brief:      销毁无名信号量
 * @param[in]:  VOID *pSem
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemDestroy(VOID *pSem);

/**
 * @function:   SAL_SemWait
 * @brief:      信号量阻塞获取
 * @param[in]:  VOID *pSem
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemWait(VOID *pSem);

/**
 * @function:   SAL_SemTimeWait
 * @brief:      信号量非阻塞获取
 * @param[in]:  VOID *pSem
 * @param[in]:  UINT32 uiTimeOutMs
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemTimeWait(VOID *pSem, UINT32 uiTimeOutMs);

/**
 * @function:   SAL_SemPost
 * @brief:      信号量+1
 * @param[in]:  VOID *pSem
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemPost(VOID *pSem);

/**
 * @function:   SAL_SemGetValue
 * @brief:      获取当前信号量的计数值
 * @param[in]:  VOID *pSem
 * @param[in]:  INT32 *pValue
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemGetValue(VOID *pSem, INT32 *pValue);

#endif

