/**
 * @file   sal_mem_stats.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   内存统计模块，可以对各个模块申请的内存进行统计，也可以对模块内申请的每块内存进行统计，如果需要对每块内存进行统计时，需要传入
 *          不同的内存名字，如果传入的内存名字相同，则会按相同的内存名字把他们统计在一起，这样设计的目的是考虑到结构体申请os内存时，申请
 *          内存的地方比较多，如果不能根据内存名字进行归类统计，则比较零散。
 *          本模块对媒体物理内存、os内存的统计都适用。本模块除了对每个模块使用了多少内存进行统计，也可根据统计的结果进行快速定位哪里有
 *          内存泄漏。
 * @author  yeyanzhong
 * @date    2021.5.20
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "sal_trace.h"


#include "sal_mem_stats.h"

/**
 * @function    SAL_memAllocStats
 * @brief       申请内存时，调用该函数进行内存统计。把新申请的内存统计按模块名及内存名进行统计。
 * @param[in]   pstModMemStats 内存统计结构体，需要单独统计的内存类型需要单独定义该变量。比如媒体内存、OS内存单独定义该变量。
 *              szModName 模块名字，相同的模块名字统计在一起
 *              szMemName 内存名字，相同的内存名字统计在一起，如果要按每块内存进行统计，需要使用者传入不同的内存名字。
 *              u32Size   申请内存的大小
 * @param[out]
 * @return
 */
INT32 SAL_memAllocStats(MOD_MEM_STATS_S *pstModMemStats, CHAR *szModName, CHAR *szMemName, UINT32 u32Size)
{
    UINT32 i = 0, j = 0;
    MOD_MEM_S *stModMemInfo = NULL;

    if (!szModName || !szMemName || !pstModMemStats)
    {
        SAL_ERROR("input param is null, szModName %p, szMemName %p, pstModMemStats %p \n", szModName, szMemName, pstModMemStats);
        return SAL_FAIL;
    }

    stModMemInfo = pstModMemStats->stModMemInfo;
    pthread_mutex_lock(&pstModMemStats->mutex);

    for (i = 0; i < MOD_NUM; i++)
    {
        if ((1 == stModMemInfo[i].u32IsUsed) && (0 == strncmp(stModMemInfo[i].szModName, szModName, sizeof(stModMemInfo[i].szModName))))
        {
            for (j = 0; j < MEM_NUM; j++)
            {
                /* 模块以及该模块内部的内存名都已经不是第一次申请内存了 */
                if ((1 == stModMemInfo[i].stMemInfo[j].u32IsUsed) \
                    && (0 == strncmp(stModMemInfo[i].stMemInfo[j].szMemName, szMemName, sizeof(stModMemInfo[i].stMemInfo[j].szMemName))))
                {
                    stModMemInfo[i].stMemInfo[j].u32MemLen += u32Size;
                    stModMemInfo[i].u32ModMemLen += u32Size;
                    pthread_mutex_unlock(&pstModMemStats->mutex);
                    return SAL_SOK;
                }
            }

            if (MEM_NUM == j)
            {
                for (j = 0; j < MEM_NUM; j++)
                {
                    /* 模块已经申请过内存，但内存名第一次申请内存 */
                    if (0 == stModMemInfo[i].stMemInfo[j].u32IsUsed)
                    {
                        stModMemInfo[i].stMemInfo[j].u32IsUsed = 1;
                        strncpy(stModMemInfo[i].stMemInfo[j].szMemName, szMemName, (NAME_LENGTH - 1));
                        stModMemInfo[i].stMemInfo[j].szMemName[NAME_LENGTH - 1] = '\0';
                        stModMemInfo[i].stMemInfo[j].u32MemLen = u32Size;
                        stModMemInfo[i].u32ModMemLen += u32Size;
                        pthread_mutex_unlock(&pstModMemStats->mutex);
                        return SAL_SOK;
                    }
                }

                SAL_ERROR("there is no space for recording the sub mod info of mem \n");
                pthread_mutex_unlock(&pstModMemStats->mutex);
                return SAL_FAIL;
            }
        }
    }

    if (MOD_NUM == i)
    {
        for (i = 0; i < MOD_NUM; i++)
        {
            /* 模块第一次申请内存 */
            if (0 == stModMemInfo[i].u32IsUsed)
            {
                stModMemInfo[i].u32IsUsed = 1;
                strncpy(stModMemInfo[i].szModName, szModName, (NAME_LENGTH - 1));
                stModMemInfo[i].szModName[NAME_LENGTH - 1] = '\0';
                if (stModMemInfo[i].u32ModMemLen > 0)
                {
                    SAL_ERROR("there is some error, u32ModMemLen %d \n", stModMemInfo[i].u32ModMemLen);
                }

                stModMemInfo[i].u32ModMemLen = u32Size;
                stModMemInfo[i].stMemInfo[0].u32IsUsed = 1;
                strncpy(stModMemInfo[i].stMemInfo[0].szMemName, szMemName, (NAME_LENGTH - 1));
                stModMemInfo[i].stMemInfo[0].szMemName[NAME_LENGTH - 1] = '\0';
                stModMemInfo[i].stMemInfo[0].u32MemLen = u32Size;
                pthread_mutex_unlock(&pstModMemStats->mutex);
                return SAL_SOK;
            }
        }

        SAL_ERROR("there is no space for recording the mod info of mem \n");
        pthread_mutex_unlock(&pstModMemStats->mutex);
        return SAL_FAIL;
    }

    pthread_mutex_unlock(&pstModMemStats->mutex);

    return SAL_FAIL;

}

/**
 * @function    SAL_memFreeStats
 * @brief       释放内存时，从已申请内存的统计数据中减去释放的内存量
 * @param[in]   pstModMemStats 内存统计结构体，需要单独统计的内存类型需要单独定义该变量。比如媒体内存、OS内存单独定义该变量。
 *              szModName 模块名字
 *              szMemName 内存名字
 *              u32Size   释放内存的大小
 * @param[out]
 * @return
 */
INT32 SAL_memFreeStats(MOD_MEM_STATS_S *pstModMemStats, CHAR *szModName, CHAR *szMemName, UINT32 u32Size)
{
    UINT32 i = 0, j = 0;
    MOD_MEM_S *stModMemInfo = NULL;

    if (!szModName || !szMemName || !pstModMemStats)
    {
        SAL_ERROR("input param is null, szModName %p, szMemName %p, pstModMemStats %p \n", szModName, szMemName, pstModMemStats);
        return SAL_FAIL;
    }

    stModMemInfo = pstModMemStats->stModMemInfo;
    pthread_mutex_lock(&pstModMemStats->mutex);

    for (i = 0; i < MOD_NUM; i++)
    {
        if ((1 == stModMemInfo[i].u32IsUsed) && (0 == strncmp(stModMemInfo[i].szModName, szModName, sizeof(stModMemInfo[i].szModName))))
        {
            for (j = 0; j < MEM_NUM; j++)
            {
                if ((1 == stModMemInfo[i].stMemInfo[j].u32IsUsed) 
                    && (0 == strncmp(stModMemInfo[i].stMemInfo[j].szMemName, szMemName, sizeof(stModMemInfo[i].stMemInfo[j].szMemName))))
                {
                    if ((stModMemInfo[i].stMemInfo[j].u32MemLen >= u32Size)
                        && (stModMemInfo[i].u32ModMemLen >= u32Size))
                    {
                        stModMemInfo[i].stMemInfo[j].u32MemLen -= u32Size;
                        stModMemInfo[i].u32ModMemLen -= u32Size;
                        pthread_mutex_unlock(&pstModMemStats->mutex);
                        return SAL_SOK;
                    }
                    else
                    {
                        SAL_ERROR("error for mem decrease statistics, u32Size %d, stMemInfo[j].u32MemLen %d, u32ModMemLen %d \n",
                                  u32Size, stModMemInfo[i].stMemInfo[j].u32MemLen, stModMemInfo[i].u32ModMemLen);
                    }

                    pthread_mutex_unlock(&pstModMemStats->mutex);
                    return SAL_FAIL;
                }
            }
        }
    }

    pthread_mutex_unlock(&pstModMemStats->mutex);
    return SAL_FAIL;
}

/**
 * @function    SAL_memPrintAlloced
 * @brief       对已申请的内存进行打印输出
 * @param[in]   pstModMemStats 内存统计结构体
 * @param[out]
 * @return
 */
void SAL_memPrintAlloced(MOD_MEM_STATS_S *pstModMemStats)
{
    UINT32 i = 0, j = 0;
    UINT32 u32MemTotal = 0;
    MOD_MEM_S *stModMemInfo = NULL;

    stModMemInfo = pstModMemStats->stModMemInfo;

    SAL_INFO("memory statistics start.***********************\n");
    for (i = 0; i < MOD_NUM; i++)
    {
        if (stModMemInfo[i].u32IsUsed)
        {
            SAL_INFO("mod_%d: %s, modMemTotal %d \n", i, stModMemInfo[i].szModName, stModMemInfo[i].u32ModMemLen);
            u32MemTotal += stModMemInfo[i].u32ModMemLen;

            for (j = 0; j < MEM_NUM; j++)
            {
                if (stModMemInfo[i].stMemInfo[j].u32IsUsed)
                {
                    SAL_INFO("    sub_mod_%d: %s, subModMemTotal %d \n", i, stModMemInfo[i].stMemInfo[j].szMemName, stModMemInfo[i].stMemInfo[j].u32MemLen);
                }
            }
        }
    }

    SAL_INFO("memory statistics end, total media memory: %d \n.***********************\n", u32MemTotal);
    return;

}

