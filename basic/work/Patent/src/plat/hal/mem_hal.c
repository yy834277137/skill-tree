/**
 * @file   mem_hal.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   平台相关的内存申请接口，主要是包括申请mmz、vb；并且对申请的内存进行统计。
 * @author  yeyanzhong
 * @date    2021.5.17
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <sal.h>
#include <malloc.h>
#include <platform_hal.h>

#include "mem_hal_inter.h"
#include "sal_mem_stats.h"

#line __LINE__ "mem_hal.c"

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
/* 内存数据段指针到头指针的转换 */
#define MEM_HAL_memDataToHead(pData) ((MEM_HAL_MEMBLOCK_HEAD_S *)(((unsigned long)(pData)) - sizeof(MEM_HAL_MEMBLOCK_HEAD_S)))

/* 内存的魔数, 用于有效性校验 */
#define HAL_MEM_MAGIC 0x12345678UL


/* ========================================================================== */
/*                          全局变量定义区                                    */
/* ========================================================================== */

/* 用于mmz内存统计的全局结构体 */
static MOD_MEM_STATS_S stMmzModMemStats =
{
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};

/* 平台相关的内存申请抽象函数结构体 */
static const MEM_PLAT_OPS_S *g_pstMemPlatOps = NULL;

/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function    mem_hal_mmzAlloc
 * @brief       申请mmz内存。并且该接口内部对模块进行内存统计，可以对每块内存单独统计，也可以对模块内某个子功能进行统计(通过传递相同的内存名实现)
 * @param[in]   u32Size 需要申请的内存大小
 *              szModName 模块名字，可以把.c源文件名作为模块名，比如disp_task.c，模块名为disp_task
 *              szMemName 内存名字，目前会把相同的内存名字统计在一起，如果上层需要每块内存单独统计，则传入内存名时名字不能相同；
 *              szZone mmz申请的内存区域
 *              bCache 映射时释放带cache
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址
 * @return
 */
INT32 mem_hal_mmzAllocDDR0(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr)
{
    INT32 s32Ret = 0;
    UINT32 align = 256;
    UINT32 allocSize = 0;
    UINT32 headSize = 0;
    UINT64 vbBlk = 0;
    Ptr pBufBase = NULL;
    Ptr pBufData = NULL;
    PhysAddr physAddrBase;
    MEM_HAL_MEMBLOCK_HEAD_S *pBufHead = NULL;

    if (0 == u32Size)
    {
        MEM_LOGE("u32Size must > 0\n");
        return SAL_FAIL;
    }

    if (!szModName || !szMemName)
    {
        MEM_LOGE("input param is null, szModName %s, szMemName %s \n", szModName, szMemName);
        return SAL_FAIL;
    }

	if (NULL == g_pstMemPlatOps->mmzAllocDDR0)
	{
		return SAL_FAIL;
	}

    /* 计算头大小 */
    headSize = sizeof(MEM_HAL_MEMBLOCK_HEAD_S);

    /* 总的要申请的内存大小 */
    allocSize = SAL_align(u32Size + headSize, align);

    s32Ret = g_pstMemPlatOps->mmzAllocDDR0(bCache, szMemName, szZone, allocSize, &physAddrBase, (VOID **)(&pBufBase), &vbBlk);
    if (0 != s32Ret)
    {
        MEM_LOGE("%d bytes failed\n", allocSize);
        return SAL_FAIL;
    }

    memset(pBufBase, 0x00, allocSize);

    /* 计算数据段的位置，内存的前面存放内存使用的类型，后面才是数据区 */
    pBufData = (void *)((unsigned long)pBufBase + headSize);

    /* 获取头的位置 */
    pBufHead = MEM_HAL_memDataToHead(pBufData);

    /* 填充头 */
    pBufHead->nMgicNum = HAL_MEM_MAGIC;
    pBufHead->poolId = SAL_SOK;
    pBufHead->privateData = vbBlk;
    pBufHead->size = allocSize;
    pBufHead->physAddrBase = physAddrBase;
    pBufHead->virAddrBase = pBufBase;
    pBufHead->physAddr = physAddrBase + headSize;

    *pRetPhysAddr = pBufHead->physAddr;
    *ppRetVirAddr = pBufData;

    SAL_memAllocStats(&stMmzModMemStats, szModName, szMemName, allocSize);

    return SAL_SOK;

}


/**
 * @function    mem_hal_mmzAlloc
 * @brief       申请mmz内存。并且该接口内部对模块进行内存统计，可以对每块内存单独统计，也可以对模块内某个子功能进行统计(通过传递相同的内存名实现)
 * @param[in]   u32Size 需要申请的内存大小
 *              szModName 模块名字，可以把.c源文件名作为模块名，比如disp_task.c，模块名为disp_task
 *              szMemName 内存名字，目前会把相同的内存名字统计在一起，如果上层需要每块内存单独统计，则传入内存名时名字不能相同；
 *              szZone mmz申请的内存区域
 *              bCache 映射时释放带cache
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址
 * @return
 */
INT32 mem_hal_mmzAlloc(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr)
{
    INT32 s32Ret = 0;
    UINT32 align = 256;
    UINT32 allocSize = 0;
    UINT32 headSize = 0;
    UINT64 vbBlk = 0;
    Ptr pBufBase = NULL;
    Ptr pBufData = NULL;
    PhysAddr physAddrBase;
    MEM_HAL_MEMBLOCK_HEAD_S *pBufHead = NULL;

    if (0 == u32Size)
    {
        MEM_LOGE("u32Size must > 0\n");
        return SAL_FAIL;
    }

    if (!szModName || !szMemName)
    {
        MEM_LOGE("input param is null, szModName %s, szMemName %s \n", szModName, szMemName);
        return SAL_FAIL;
    }

    /* 计算头大小 */ 
    /* 如果把头部去掉外部释放内存时需要传入size，因为NT mmzfree接口里需要传入size，可NT接口内部好像并没有使用size这个参数 */
    headSize = sizeof(MEM_HAL_MEMBLOCK_HEAD_S);

    /* 总的要申请的内存大小 */
    allocSize = SAL_align(u32Size + headSize, align);

    s32Ret = g_pstMemPlatOps->mmzAlloc(bCache, szMemName, szZone, allocSize, &physAddrBase, (VOID **)(&pBufBase), &vbBlk);
    if (0 != s32Ret)
    {
        MEM_LOGE("%d bytes failed\n", allocSize);
        return SAL_FAIL;
    }

    memset(pBufBase, 0x00, allocSize);

    /* 计算数据段的位置，内存的前面存放内存使用的类型，后面才是数据区 */
    pBufData = (void *)((unsigned long)pBufBase + headSize);

    /* 获取头的位置 */
    pBufHead = MEM_HAL_memDataToHead(pBufData);

    /* 填充头 */
    pBufHead->nMgicNum = HAL_MEM_MAGIC;
    pBufHead->poolId = SAL_SOK;
    pBufHead->privateData = vbBlk;
    pBufHead->size = allocSize;
    pBufHead->physAddrBase = physAddrBase;
    pBufHead->virAddrBase = pBufBase;
    pBufHead->physAddr = physAddrBase + headSize;

    *pRetPhysAddr = pBufHead->physAddr;
    *ppRetVirAddr = pBufData;

    SAL_memAllocStats(&stMmzModMemStats, szModName, szMemName, allocSize);

    return SAL_SOK;

}

/**
 * @function    mem_hal_cmaMmzAlloc
 * @brief       RK平台申请物理地址连续的mmz内存。并且该接口内部对模块进行内存统计，可以对每块内存单独统计，也可以对模块内某个子功能进行统计(通过传递相同的内存名实现)
 * @param[in]   u32Size 需要申请的内存大小
 *              szModName 模块名字，可以把.c源文件名作为模块名，比如disp_task.c，模块名为disp_task
 *              szMemName 内存名字，目前会把相同的内存名字统计在一起，如果上层需要每块内存单独统计，则传入内存名时名字不能相同；
 *              szZone vb申请的内存区域
 *              bCache 映射时是否带cache
 * @param[out]  pstVbInfo 实际申请到的vb信息
 *              
 * @return
 */
INT32 mem_hal_cmaMmzAlloc(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, ALLOC_VB_INFO_S *pstVbInfo)
{
    INT32 s32Ret = 0;
    UINT32 align = 128;
    UINT32 allocSize = 0;
    UINT64 vbBlk = 0;
    Ptr pBufBase = NULL;
    PhysAddr physAddrBase;

    if (0 == u32Size)
    {
        MEM_LOGE("u32Size must > 0\n");
        return SAL_FAIL;
    }
    if (!szModName || !szMemName || !pstVbInfo)
    {
        MEM_LOGE("input param is null, szModName %s, szMemName %s, pstVbInfo %p\n", szModName, szMemName, pstVbInfo);
        return SAL_FAIL;
    }

    /* 总的要申请的内存大小 */
    allocSize = SAL_align(u32Size, align);

    if (g_pstMemPlatOps->cmaMmzAlloc)
    {
        s32Ret = g_pstMemPlatOps->cmaMmzAlloc(bCache, szMemName, szZone, allocSize, &physAddrBase, (VOID **)(&pBufBase), &vbBlk);
        if (0 != s32Ret)
        {
            MEM_LOGE("%d bytes failed\n", allocSize);
            return SAL_FAIL;
        }

        pstVbInfo->u64PhysAddr = physAddrBase;
        pstVbInfo->pVirAddr = pBufBase;
        pstVbInfo->u64VbBlk = vbBlk;
        pstVbInfo->u32PoolId = SAL_SOK;
        pstVbInfo->u32Size = allocSize;

        SAL_memAllocStats(&stMmzModMemStats, szModName, szMemName, allocSize);
    }
    else
    {
        MEM_LOGW("soc plat not support! \n");
    }

    /*返回数据段指针*/
    return SAL_SOK;
}

/**
 * @function   mem_hal_iommuMmzAlloc
 * @brief      申请iommu的mmz内存，目前主要在RK平台里使用
 * @param[in]  UINT32 u32Size              
 * @param[in]  CHAR *szModName             
 * @param[in]  CHAR *szMemName             
 * @param[in]  CHAR *szZone                
 * @param[in]  BOOL bCache                 
 * @param[in]  ALLOC_VB_INFO_S *pstVbInfo  
 * @param[out] None
 * @return     INT32
 */
INT32 mem_hal_iommuMmzAlloc(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, ALLOC_VB_INFO_S *pstVbInfo)
{
    INT32 s32Ret = 0;
    UINT32 align = 128;
    UINT32 allocSize = 0;
    UINT64 vbBlk = 0;
    Ptr pBufBase = NULL;
    PhysAddr physAddrBase;

    if (0 == u32Size)
    {
        MEM_LOGE("u32Size must > 0\n");
        return SAL_FAIL;
    }
    if (!szModName || !szMemName || !pstVbInfo)
    {
        MEM_LOGE("input param is null, szModName %s, szMemName %s, pstVbInfo %p\n", szModName, szMemName, pstVbInfo);
        return SAL_FAIL;
    }

    /* 总的要申请的内存大小 */
    allocSize = SAL_align(u32Size, align);

    if (g_pstMemPlatOps->iommuMmzAlloc)
    {
        s32Ret = g_pstMemPlatOps->iommuMmzAlloc(bCache, szMemName, szZone, allocSize, &physAddrBase, (VOID **)(&pBufBase), &vbBlk);
        if (0 != s32Ret)
        {
            MEM_LOGE("%d bytes failed\n", allocSize);
            return SAL_FAIL;
        }

        pstVbInfo->u64PhysAddr = physAddrBase;
        pstVbInfo->pVirAddr = pBufBase;
        pstVbInfo->u64VbBlk = vbBlk;
        pstVbInfo->u32PoolId = SAL_SOK;
        pstVbInfo->u32Size = allocSize;

        SAL_memAllocStats(&stMmzModMemStats, szModName, szMemName, allocSize);
    }
    else
    {
        MEM_LOGW("soc plat not support! \n");
    }

    /*返回数据段指针*/
    return SAL_SOK;
}

/**
 * @function    mem_hal_mmzFree
 * @brief       对mmz内存进行释放
 * @param[in]   pPtr 虚拟地址
 *              szModName 模块名字，可以把.c源文件名作为模块名，比如disp_task.c，模块名为disp_task
 *              szMemName 内存名字，目前会把相同的内存名字统计在一起，如果上层需要每块内存单独统计，则传入内存名时名字不能相同；
 * @param[out]
 * @return
 */
INT32 mem_hal_mmzFree(Ptr pPtr, CHAR *szModName, CHAR *szMemName)
{
    INT32 s32Ret = 0;
    UINT32 allocSize = 0;
    MEM_HAL_MEMBLOCK_HEAD_S *pBufHead = NULL;

    if (NULL == pPtr)
    {
        return SAL_FAIL;
    }

    if (!szModName || !szMemName)
    {
        MEM_LOGE("input param is null, szModName %s, szMemName %s \n", szModName, szMemName);
        return SAL_FAIL;
    }

    pBufHead = MEM_HAL_memDataToHead(pPtr);

    /* 校验魔数 */
    if (HAL_MEM_MAGIC != pBufHead->nMgicNum)
    {
        MEM_LOGE("invalid magic:0x%x\n", pBufHead->nMgicNum);
        return SAL_FAIL;
    }

    if (pBufHead->poolId > 0)
    {
        MEM_LOGE("This is memAllocFd %d %lu\n", pBufHead->poolId, (UINTL)pBufHead->privateData);
        return SAL_FAIL;
    }

    /* 释放内存块时把魔数置为-1 */
    pBufHead->nMgicNum = (UINT32) -1;
    allocSize = pBufHead->size;

    /* 释放内存 */
    s32Ret = g_pstMemPlatOps->mmzFree(allocSize, pBufHead->physAddrBase, pBufHead->virAddrBase, pBufHead->privateData);
    if (s32Ret != SAL_SOK)
    {
        MEM_LOGE("hFree failed, size:%d\n", allocSize);
        return SAL_FAIL;
    }

    SAL_memFreeStats(&stMmzModMemStats, szModName, szMemName, allocSize);

    return SAL_SOK;
}

/**
 * @function    mem_hal_cmaMmzFree
 * @brief       对cma物理地址连续的mmz内存进行释放；目前主要在RK平台里使用
 * @param[in]   u32Size 申请时实际申请到的内存大小，底层可能会比上层下发的多一点，主要是由于对齐要求
 *              szModName 模块名字，可以把.c源文件名作为模块名，比如disp_task.c，模块名为disp_task
 *              szMemName 内存名字，目前会把相同的内存名字统计在一起，如果上层需要每块内存单独统计，则传入内存名时名字不能相同；
                U64PhysAddr RK平台释放mmz当前不需要传入内存物理地址
                pVirAddr    RK平台释放mmz当前不需要传入内存虚拟地址
                vbBlk       RK里指的是mb指针
 * @param[out]
 * @return
 */
INT32 mem_hal_cmaMmzFree(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk)
{
    INT32 s32Ret = 0;

    if (!szModName || !szMemName)
    {
        MEM_LOGE("input param is null, szModName %s, szMemName %s \n", szModName, szMemName);
        return SAL_FAIL;
    }


    /* 释放内存 */
    if (g_pstMemPlatOps->cmaMmzFree)
    {
        s32Ret = g_pstMemPlatOps->cmaMmzFree(u32Size, U64PhysAddr, pVirAddr, vbBlk);
        if (s32Ret != SAL_SOK)
        {
            MEM_LOGE("hFree failed, size:%d\n", u32Size);
            return SAL_FAIL;
        }

        SAL_memFreeStats(&stMmzModMemStats, szModName, szMemName, u32Size);
    }
    else
    {
        MEM_LOGW("soc plat not support! \n");
    }

    return SAL_SOK;
}

/**
 * @function   mem_hal_iommuMmzFree
 * @brief      对iommu的mmz内存进行释放；目前主要在RK平台里使用
 * @param[in]  UINT32 u32Size      
 * @param[in]  CHAR *szModName     
 * @param[in]  CHAR *szMemName     
 * @param[in]  UINT64 U64PhysAddr  
 * @param[in]  VOID *pVirAddr      
 * @param[in]  UINT64 vbBlk        
 * @param[out] None
 * @return     INT32
 */
INT32 mem_hal_iommuMmzFree(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk)
{
    INT32 s32Ret = 0;

    if (!szModName || !szMemName)
    {
        MEM_LOGE("input param is null, szModName %s, szMemName %s \n", szModName, szMemName);
        return SAL_FAIL;
    }


    /* 释放内存 */
    if (g_pstMemPlatOps->iommuMmzFree)
    {
        s32Ret = g_pstMemPlatOps->iommuMmzFree(u32Size, U64PhysAddr, pVirAddr, vbBlk);
        if (s32Ret != SAL_SOK)
        {
            MEM_LOGE("hFree failed, size:%d\n", u32Size);
            return SAL_FAIL;
        }

        SAL_memFreeStats(&stMmzModMemStats, szModName, szMemName, u32Size);
    }
    else
    {
        MEM_LOGW("soc plat not support! \n");
    }

    return SAL_SOK;
}

/**
 * @function    mem_hal_mmzFree
 * @brief       对m
 * @param[in]   pPtr 虚拟地址
 *              szModName 模块名字，可以把.c源文件名作为模块名，比如disp_task.c，模块名为disp_task
 *              szMemName 内存名字，目前会把相同的内存名字统计在一起，如果上层需要每块内存单独统计，则传入内存名时名字不能相同；
 * @param[out]
 * @return
 */
INT32 mem_hal_mmzFlushCache(UINT64 vbBlk)
{
    INT32 s32Ret = 0;

    if (NULL != g_pstMemPlatOps->mmzFlushCache)
    {
        s32Ret = g_pstMemPlatOps->mmzFlushCache(vbBlk, SAL_FALSE);
        if (s32Ret != SAL_SOK)
        {
            MEM_LOGE("mmzFlushCache failed \n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function    mem_hal_vbGetVirAddr
 * @brief       通过VB获取虚拟地址
 * @param[in]   vbBlk vb块
 *              ppRetVirAddr 存放获取的虚拟地址
 * @param[out]
 * @return
 */
INT32 mem_hal_vbGetVirAddr(UINT64 vbBlk,VOID **ppRetVirAddr)
{
    INT32 s32Ret = 0;

    if (NULL != g_pstMemPlatOps->vbGetVirAddr)
    {
        s32Ret = g_pstMemPlatOps->vbGetVirAddr(vbBlk, ppRetVirAddr);
        if (s32Ret != SAL_SOK)
        {
            MEM_LOGE("mem_hal_vbGetVirAddr failed \n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}

/**
 * @function    mem_hal_vbGetSize
 * @brief       vb获取size
 * @param[in]   vbBlk vb块
 *              pSize 内存大小
 * @param[out]
 * @return
 */
INT32 mem_hal_vbGetSize(UINT64 vbBlk,UINT64 *pSize)
{
    INT32 s32Ret = 0;
    UINT64 u64Size = 0;

    if (NULL != g_pstMemPlatOps->vbGetSize)
    {
        s32Ret = g_pstMemPlatOps->vbGetSize(vbBlk, &u64Size);
        if (s32Ret != SAL_SOK)
        {
            MEM_LOGE("mem_hal_vbGetSize failed \n");
            return SAL_FAIL;
        }
        *pSize = u64Size;
    }

    return SAL_SOK;
}


/**
 * @function    mem_hal_vbGetOffset
 * @brief       VB获取offset
 * @param[in]   vbBlk vb块
 *              u32offset 偏移值
 * @param[out]
 * @return
 */
INT32 mem_hal_vbGetOffset(UINT64 vbBlk,UINT32 *pOffset)
{
    INT32 s32Ret = 0;
    UINT32 u32offset = 0;

    if (NULL != g_pstMemPlatOps->vbGetOffset)
    {
        s32Ret = g_pstMemPlatOps->vbGetOffset(vbBlk, &u32offset);
        if (s32Ret != SAL_SOK)
        {
            MEM_LOGE("mem_hal_vbGetOffset failed \n");
            return SAL_FAIL;
        }
        *pOffset = u32offset; 
    }
    
    return SAL_SOK;
}

/**
 * @function    mem_hal_vbMap
 * @brief       通过srcVbBlk创建vbBlk，可以访问同一块内存，
 * @param[in]   srcVbBlk 
 *              dstVbBlk 
 * @param[out]
 * @return
 */
INT32 mem_hal_vbMap(UINT64 srcVbBlk,UINT64 *dstVbBlk)
{
    INT32 s32Ret = 0;
    UINT64 vbBlk = 0;
    if (NULL != g_pstMemPlatOps->vbMap)
    {
        s32Ret = g_pstMemPlatOps->vbMap(srcVbBlk, &vbBlk);
        if (s32Ret != SAL_SOK)
        {
            MEM_LOGE("mem_hal_vbGetVirAddr failed \n");
            return SAL_FAIL;
        }
        *dstVbBlk = vbBlk;
    }

    return SAL_SOK;
}

/**
 * @function    mem_hal_vbGetVirAddr
 * @brief       销毁vb
 * @param[in]   vbBlk vb块
 * @param[out]
 * @return
 */
INT32 mem_hal_vbDelete(UINT64 vbBlk)
{
    INT32 s32Ret = 0;

    if (NULL != g_pstMemPlatOps->vbDelete)
    {
        s32Ret = g_pstMemPlatOps->vbDelete(vbBlk);
        if (s32Ret != SAL_SOK)
        {
            MEM_LOGE("mem_hal_vbDelete failed \n");
            return SAL_FAIL;
        }
    }

    return SAL_SOK;
}



/**
 * @function    mem_hal_vbAlloc
 * @brief       申请vb内存。并且该接口内部对模块进行内存统计，可以对每块内存单独统计，也可以对模块内某个子功能进行统计(通过传递相同的内存名实现)
 * @param[in]   u32Size 需要申请的内存大小
 *              szModName 模块名字，可以把.c源文件名作为模块名，比如disp_task.c，模块名为disp_task
 *              szMemName 内存名字，目前会把相同的内存名字统计在一起，如果上层需要每块内存单独统计，则传入内存名时名字不能相同；
 *              szZone vb申请的内存区域
 *              bCache 映射时是否带cache
 * @param[out]  pstVbInfo 实际申请到的vb信息
 *              
 * @return
 */
INT32 mem_hal_vbAlloc(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, ALLOC_VB_INFO_S *pstVbInfo)
{
    INT32 s32Ret = 0;
    UINT32 align = 128;
    UINT32 allocSize = 0;
    UINT32 poolId = 0;
    UINT64 vbBlk = 0;
    Ptr pBufBase = NULL;
    PhysAddr pPhyBase;

    if (0 == u32Size)
    {
        MEM_LOGE("u32Size must > 0\n");
        return SAL_FAIL;
    }
    if (!szModName || !szMemName || !pstVbInfo)
    {
        MEM_LOGE("input param is null, szModName %s, szMemName %s, pstVbInfo %p\n", szModName, szMemName, pstVbInfo);
        return SAL_FAIL;
    }

    /* 总的要申请的内存大小 */
    allocSize = SAL_align(u32Size, align);
    s32Ret = g_pstMemPlatOps->vbAlloc(allocSize, szZone, bCache, &pPhyBase, &pBufBase, &poolId, &vbBlk);
    if (0 != s32Ret)
    {
        MEM_LOGE("%d bytes failed\n", allocSize);
        return SAL_FAIL;
    }

    pstVbInfo->u64PhysAddr = pPhyBase;
    pstVbInfo->pVirAddr = pBufBase;
    pstVbInfo->u64VbBlk = vbBlk;
    pstVbInfo->u32PoolId = poolId;
    pstVbInfo->u32Size = allocSize;

    SAL_memAllocStats(&stMmzModMemStats, szModName, szMemName, allocSize);

    /*返回数据段指针*/
    return SAL_SOK;
}

/**
 * @function    mem_hal_vbFree
 * @brief       释放vb内存, 没有MEM_HAL_MEMBLOCK_HEAD_S这个头部，NT98336研发反馈不能对这个头部做偏移
 * @param[in]   pPtr 虚拟地址
 *              szModName 模块名字，可以把.c源文件名作为模块名，比如disp_task.c，模块名为disp_task
 *              szMemName 内存名字，目前会把相同的内存名字统计在一起，如果上层需要每块内存单独统计，则传入内存名时名字不能相同；
                u32Size 申请时实际申请到的大小
                u64VbBlk vb块编号
                u64PoolId vb对应的pool
 * @param[out]
 * @return
 */
INT32 mem_hal_vbFree(Ptr pPtr, CHAR *szModName, CHAR *szMemName, UINT32 u32Size, UINT64 u64VbBlk, UINT32 u32PoolId)
{
    INT32 s32Ret = 0;

    if (NULL == pPtr)
    {
        MEM_LOGE("is null\n");
        return SAL_FAIL;
    }
    if (!szModName || !szMemName)
    {
        MEM_LOGE("input param is null, szModName %s, szMemName %s, \n", szModName, szMemName);
        return SAL_FAIL;
    }

    s32Ret = g_pstMemPlatOps->vbFree(u32Size, pPtr, u32PoolId, u64VbBlk);
    if (0 != s32Ret)
    {
        MEM_LOGE("%d bytes failed\n", u32Size);
        return SAL_FAIL;
    }

    SAL_memFreeStats(&stMmzModMemStats, szModName, szMemName, u32Size);

    return SAL_SOK;
}

INT32 mem_hal_vbPoolAlloc(VB_INFO_S *pstVbInfo, CHAR *szModName, CHAR *szMemName, CHAR *aszZone)
{
    INT32       s32Ret      = 0;
    
    if(!pstVbInfo|| !szModName || !szMemName || !g_pstMemPlatOps || !(g_pstMemPlatOps->vbPoolAlloc))
    {
        DUP_LOGI("input param is null, pstVbInfo %p, szModName %s, szMemName %s \n", pstVbInfo, szModName, szMemName);
        return SAL_FAIL;
    }

    s32Ret = g_pstMemPlatOps->vbPoolAlloc(pstVbInfo, aszZone);
    if (0 != s32Ret)
    {
        MEM_LOGE("g_pstMemPlatOps->vbPoolAlloc failed\n");
        return SAL_FAIL;
    }
    
    if (SAL_SOK != SAL_memAllocStats(&stMmzModMemStats, szModName, szMemName, pstVbInfo->u32PoolSize))
    {
        s32Ret = g_pstMemPlatOps->vbPoolFree(pstVbInfo);
        if (0 != s32Ret)
        {
            MEM_LOGE("SAL_memAllocStats vbPoolFree failed\n");
        }
        return SAL_FAIL;
    }
    return SAL_SOK;
}

INT32 mem_hal_vbPoolFree(VB_INFO_S *pstVbInfo, CHAR *szModName, CHAR *szMemName)
{
    INT32       s32Ret      = 0;

    if(!pstVbInfo|| !szModName || !szMemName || !g_pstMemPlatOps || !(g_pstMemPlatOps->vbPoolFree))
    {
        MEM_LOGE("input param is null, pstVbInfo %p, szModName %s, szMemName %s \n", pstVbInfo, szModName, szMemName);
        return SAL_FAIL;
    }

    s32Ret = g_pstMemPlatOps->vbPoolFree(pstVbInfo);
    if (0 != s32Ret)
    {
        MEM_LOGE("g_pstMemPlatOps->vbPoolFree failed\n");
        return SAL_FAIL;
    }

   SAL_memFreeStats(&stMmzModMemStats, szModName, szMemName, pstVbInfo->u32PoolSize);
    
    return SAL_SOK;
}

/**
 * @function    mem_hal_init
 * @brief       初始化mem功能函数
 * @param[in]   void
 * @param[out]  无
 * @return
 */
INT32 mem_hal_init(void)
{
    if (NULL == g_pstMemPlatOps)
    {
        g_pstMemPlatOps = mem_hal_register();
    }
    
    return SAL_SOK;
}

/**
 * @function  mem_ShowMMZ_Alloc
 * @brief      统计模块申请内存函数
 * @param[in]   void
 * @param[out]  无
 * @return
 */
void mem_ShowMMZ_Alloc(void)
{
    UINT32 i = 0;
    MOD_MEM_S *pstModMemInfo = NULL;
    UINT32 j = 0;
    UINT64 u64MmzTotal = 0;

    for (i = 0; i < MOD_NUM; i++)
    {
        pstModMemInfo = &stMmzModMemStats.stModMemInfo[i];
        if(pstModMemInfo->u32IsUsed == 1)
        {
            u64MmzTotal += pstModMemInfo->u32ModMemLen;
        }
    }

    SAL_print("\n--------------------------------MMZ_Alloc(total:%lld)--------------------------------------\n", u64MmzTotal);
    for (i = 0; i < MOD_NUM; i++)
    {
        pstModMemInfo = &stMmzModMemStats.stModMemInfo[i];
        if(pstModMemInfo->u32IsUsed == 1)
        {
            SAL_print("%s (%d)\n", pstModMemInfo->szModName, pstModMemInfo->u32ModMemLen);
            for(j = 0; j < MEM_NUM; j++)
            {

                if(pstModMemInfo->stMemInfo[j].u32IsUsed == 1)
                {
                    SAL_print("                |\n");
                    SAL_print("                ----%11s (%d)\n", pstModMemInfo->stMemInfo[j].szMemName, pstModMemInfo->stMemInfo[j].u32MemLen);
                }
            }
        }
    }

    return;
}

