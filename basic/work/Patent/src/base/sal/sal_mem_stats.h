/**
 * @file   sal_mem_stats.h
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
#ifndef _SAL_MEM_STATS_H_
#define _SAL_MEM_STATS_H_

#include "sal_type.h"

#define MOD_NUM		256
#define MEM_NUM		128
#define NAME_LENGTH	32

/* 按内存名字进行统计的结构体 */
typedef struct
{
    CHAR szMemName[NAME_LENGTH];
    UINT32 u32IsUsed;
    UINT32 u32MemLen; /* 该内存名对应的申请内存数量 */
} MEM_INFO_S;

/* 单独一个模块的内存统计结构体 */
typedef struct
{
    CHAR szModName[NAME_LENGTH];
    UINT32 u32IsUsed;  /* 1表示在使用，0表示没有使用 */
    MEM_INFO_S stMemInfo[MEM_NUM];
    UINT32 u32ModMemLen; /* 模块总共申请的内存，以及在清除模块的u32IsUed标志位时也会使用 */
} MOD_MEM_S;

/* 内存统计总的结构体 */
typedef struct
{
    MOD_MEM_S stModMemInfo[MOD_NUM];
    pthread_mutex_t mutex;
} MOD_MEM_STATS_S;

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
INT32 SAL_memAllocStats(MOD_MEM_STATS_S *pstModMemStats, CHAR *szModName, CHAR *szMemName, UINT32 u32Size);

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
INT32 SAL_memFreeStats(MOD_MEM_STATS_S *pstModMemStats, CHAR *szModName, CHAR *szMemName, UINT32 u32Size);

/**
 * @function    SAL_memPrintAlloced
 * @brief       对已申请的内存进行打印输出
 * @param[in]   pstModMemStats 内存统计结构体
 * @param[out]
 * @return
 */
void SAL_memPrintAlloced(MOD_MEM_STATS_S *pstModMemStats);


#endif

