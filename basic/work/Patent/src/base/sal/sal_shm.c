/***
 * @file   sal_shm.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  进程间共享内存
 * @author dsp
 * @date   2022-03-02
 * @note
 * @note History:
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sal.h>

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function:   SAL_ShmDetach
 * @brief:      断开与共享内存的链接
 * @param[in]:  void * pVaddr
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_ShmDetach(VOID *pVaddr)
{
    if (-1 == shmdt(pVaddr))
    {
        SAL_LOGE("Detach Share Memory Error:%s\n", strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function:   SAL_ShmMakeKey
 * @brief:      获取进程通讯FIFO文件键值
 * @param[in]:  CHAR *pFilePath
 * @param[in]:  INT32 id
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_ShmMakeKey(CHAR *pFilePath, INT32 id)
{
    key_t key;
    FILE *checkFile;

    /* 首次起来如果通信文件不存在，这里自动创建 */
    checkFile = fopen(pFilePath, "w");
    if (checkFile == NULL)
    {
        SAL_LOGE("fopen %s\n", pFilePath);
        return -1;
    }

    fclose(checkFile);
    key = ftok(pFilePath, id);

    return (INT32)key;
}

/**
 * @function:   SAL_ShmCreate
 * @brief:      得到一个共享内存标识符或创建一个共享内存对象,把共享内存区对
                象映射到调用进程的地址空间
 * @param[in]:  UINT32 key
 * @param[in]:  UINT32 size
 * @param[in]:  INT32 flags
 * @param[in]:  INT32 *semid
 * @param[out]: None
 * @return:     VOID *
 */
VOID *SAL_ShmCreate(UINT32 key, UINT32 size, INT32 flags, INT32 *semid)
{
    INT32 shmid = 0;
    VOID *pVaddr = NULL;

    /* 返回 EEXIST 表示 参数key所指的共享内存已存在，且参数中设置了IPC_EXCL */
    if (flags & SAL_IPC_FLAG_CREATE)
    {
        /* IPC_EXCL */
        shmid = shmget(key, size, IPC_CREAT | 0666);
    }
    else
    {
        shmid = shmget(key, size, 0666);
    }

    if (shmid < 0)
    {
        SAL_LOGE("Create shm Error:%s|%d|%d\n", strerror(errno), size, shmid);
        return NULL;
    }

    pVaddr = shmat(shmid, 0, 0);
    if (NULL == pVaddr)
    {
        SAL_LOGE("Get Share Memory Error:%s\n", strerror(errno));
        return NULL;
    }

    *semid = shmid;
    return pVaddr;
}

/**
 * @function:   SAL_ShmDelete
 * @brief:      销毁共享内存标志和删除这片内存
 * @param[in]:  INT32 shmid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_ShmDelete(INT32 shmid)
{
    if (-1 == shmctl(shmid, IPC_RMID, NULL))
    {
        SAL_LOGE("Delete Share Memory Error:%s\n", strerror(errno));
        return SAL_FAIL;
    }

    return SAL_SOK;
}

