/***
 * @file   sal_shm.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  进程间共享内存
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */
#ifndef _SAL_SHM_H_
#define _SAL_SHM_H_

/* 所需头文件 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SAL_IPC_FLAG_OPEN	(0x0)
#define SAL_IPC_FLAG_CREATE (0x1)

/**
 * @function:   SAL_ShmDetach
 * @brief:      断开与共享内存的链接
 * @param[in]:  void * pVaddr
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_ShmDetach(void *pVaddr);

/**
 * @function:   SAL_ShmMakeKey
 * @brief:      获取进程通讯FIFO文件键值
 * @param[in]:  CHAR *pFilePath
 * @param[in]:  INT32 id
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_ShmMakeKey(CHAR *pFilePath, INT32 id);

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
VOID *SAL_ShmCreate(UINT32 key, UINT32 size, INT32 flags, INT32 *semid);

/**
 * @function:   SAL_ShmDelete
 * @brief:      销毁共享内存标志和删除这片内存
 * @param[in]:  INT32 shmid
 * @param[out]: None
 * @return:     INT32
 */
INT32 SAL_ShmDelete(INT32 shmid);

#endif /* _SAL_SHM_H_ */

