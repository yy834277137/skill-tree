/***
 * @file   sal_sem_systemV.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  本文件封装对linux下IPC的信号量的操作(systemV标准)
 *         优先使用Posix标准的信号量接口
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#include "sal.h"

/*
   默认不开放SystemV版本的信号量接口，通过宏定义SAL_SEM_SYSTEM_V来开/关编译
   优先使用Posix标准的信号量接口
 */

#ifdef SAL_SEM_SYSTEM_V
union semun
{
    INT32 val;
    struct semid_ds *buf;
    unsigned short int *array;
    struct seminfo *__buf;
};


/**
 * @function:   SAL_SemSetValue_SysV
 * @brief:      获取信号量的计数值
 * @param[in]:  int iSemid
 * @param[in]:  int iVal
 * @param[out]: None
 * @return:     int
 */
INT32 SAL_SemSetValue_SysV(INT32 iSemid, INT32 iVal)
{
    union semun sem_union;

    sem_union.val = iVal;

    if (-1 == semctl(iSemid, 0, SETVAL, sem_union))
    {
        SAL_LOGE("semctl(SETVAL) error:");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_SemGetValue_SysV
 * @brief:      获取信号量的值
 * @param[in]:  INT32 semid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemGetValue_SysV(INT32 semid)
{
    union semun sem_union;

    memset(&sem_union, 0, sizeof(sem_union));

    if (-1 == semctl(semid, 0, GETVAL, sem_union))
    {
        SAL_LOGE("semctl(GETVAL) error\n");
        return SAL_FAIL;
    }

    return sem_union.val;
}

/**
 * @function:   SAL_SemDel_SysV
 * @brief:      删除信号量
 * @param[in]:  INT32 semid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemDel_SysV(INT32 semid)
{
    if (-1 == semctl(semid, 0, IPC_RMID, 0))
    {
        SAL_LOGE("semctl(IPC_RMID) error\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_SemP_SysV
 * @brief:      信号量P操作：对信号量进行减一操作
 * @param[in]:  INT32 iSemid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemP_SysV(INT32 iSemid)
{
    struct sembuf stSemBuf;

    stSemBuf.sem_num = 0;               /*信号量编号 */
    stSemBuf.sem_op = -1;               /*P操作  */
    stSemBuf.sem_flg = SEM_UNDO;

    if (-1 == semop(iSemid, &stSemBuf, 1))
    {
        SAL_LOGE("semctl(semop P) error:");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_SemV_SysV
 * @brief:      信号量V操作：对信号量进行加一操作
 * @param[in]:  INT32 iSemid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemV_SysV(INT32 iSemid)
{
    struct sembuf stSemBuf;

    stSemBuf.sem_num = 0;               /*信号量编号 */
    stSemBuf.sem_op = 1;                /*V操作   */
    stSemBuf.sem_flg = SEM_UNDO;

    if (-1 == semop(iSemid, &stSemBuf, 1))
    {
        SAL_LOGE("semctl(semop V) error:");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_SemCreate_SysV
 * @brief:      创建信号量
 * @param[in]:  key_t key
 * @param[in]:  INT32 nsems
 * @param[in]:  INT32 semflg
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_SemCreate_SysV(key_t key, INT32 nsems, INT32 semflg)
{
    INT32 iSemid;

    iSemid = semget(key, nsems, semflg);
    if (-1 == iSemid)
    {
        if (EEXIST == errno)
        {
            return SAL_FAIL;
        }

        SAL_LOGE("semget error:");
        return SAL_FAIL;
    }

    return iSemid;
}

#endif

