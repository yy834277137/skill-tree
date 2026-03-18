/***
 * @file   sal_sem_systemV.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  本文件封装对linux下IPC的信号量的操作(systemV标准)
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


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/*
   默认不开放SystemV版本的信号量接口，通过宏定义SAL_SEM_SYSTEM_V来开/关编译
   优先使用Posix标准的信号量接口
 */

#ifdef SAL_SEM_SYSTEM_V

/**
 * @function:   SAL_SemSetValue_SysV
 * @brief:      获取信号量的计数值
 * @param[in]:  int iSemid
 * @param[in]:  int iVal
 * @param[out]: None
 * @return:     int
 */
INT32 SAL_SemSetValue_SysV(INT32 iSemid, INT32 iVal);

/**
 * @function:   SAL_SemGetValue_SysV
 * @brief:      获取信号量的值
 * @param[in]:  INT32 semid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemGetValue_SysV(INT32 semid);

/**
 * @function:   SAL_SemDel_SysV
 * @brief:      删除信号量
 * @param[in]:  INT32 semid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemDel_SysV(INT32 semid);

/**
 * @function:   SAL_SemP_SysV
 * @brief:      信号量P操作：对信号量进行减一操作
 * @param[in]:  INT32 iSemid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemP_SysV(INT32 iSemid);

/**
 * @function:   SAL_SemV_SysV
 * @brief:      信号量V操作：对信号量进行加一操作
 * @param[in]:  INT32 iSemid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemV_SysV(INT32 iSemid);

/**
 * @function:   SAL_SemCreate_SysV
 * @brief:      创建信号量
 * @param[in]:  key_t key
 * @param[in]:  INT32 nsems
 * @param[in]:  INT32 semflg
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemCreate_SysV(key_t key, INT32 nsems, INT32 semflg);

#endif

#endif

