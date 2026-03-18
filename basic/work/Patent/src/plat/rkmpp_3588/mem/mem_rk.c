/**
 * @file   mem_rk.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   RK平台 mmz/vb的内存申请释放接口
 * @author  liuxianying
 * @date    2022.03.09
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#include "sal.h"
#include "platform_sdk.h"
#include "mem_hal_inter.h"

static MEM_PLAT_OPS_S g_stMemPlatOps;
#define INVALID_PHYADDR       -1

/**
 * @function    mem_rk_mmzAlloc
 * @brief       申请mmz
 * @param[in]   u32Size 需要申请的内存大小
 *              pcMmb mmzBlock名字
 *              pcZone vb申请的内存区域
 *              bCache 映射时是否带cache
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址

 * @return
 */
static INT32 mem_rk_mmzAlloc(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 u32Size, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr, UINT64 *pVbBlk)
{
    Ptr pVirAddr = NULL;
    PhysAddr physAddr;
    MB_BLK mbBlk;
    INT32 s32Ret;

    if (bCached)
    {
        s32Ret =RK_MPI_SYS_MmzAlloc_Cached(&mbBlk, RK_NULL, RK_NULL, u32Size);
    }
    else
    {
        s32Ret = RK_MPI_SYS_MmzAlloc(&mbBlk, RK_NULL, RK_NULL, u32Size);
    }
	
    if (SAL_SOK != s32Ret)
    {
        MEM_LOGE("%d bytes failed, ret 0x%x\n", u32Size, s32Ret);
        return SAL_FAIL;
    }
    *pVbBlk = (UINT64)mbBlk;

    pVirAddr = RK_MPI_MB_Handle2VirAddr(mbBlk);
    physAddr = INVALID_PHYADDR;      /*不连续内存无法获取缓存模块物理内存*/

    *ppRetVirAddr = pVirAddr;
    *pRetPhysAddr = physAddr; 
    

    return SAL_SOK;

}

/**
 * @function    mem_rk_cmaMmzAlloc
 * @brief       申请物理地址连续的mmz
 * @param[in]   u32Size 需要申请的内存大小
 *              pcMmb mmzBlock名字
 *              pcZone vb申请的内存区域
 *              bCache 映射时是否带cache
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址

 * @return
 */
static INT32 mem_rk_cmaMmzAlloc(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 u32Size, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr, UINT64 *pVbBlk)
{
    Ptr pVirAddr = NULL;
    RK_U64 physAddr;
    MB_BLK mbBlk;
    INT32 s32Ret;

    if (bCached)
    {
        s32Ret = RK_MPI_MMZ_Alloc(&mbBlk, u32Size, RK_MMZ_ALLOC_TYPE_CMA | RK_MMZ_ALLOC_CACHEABLE);
    }
    else
    {
        s32Ret = RK_MPI_MMZ_Alloc(&mbBlk, u32Size, RK_MMZ_ALLOC_TYPE_CMA | RK_MMZ_ALLOC_UNCACHEABLE);
    }
    if (SAL_SOK != s32Ret)
    {
        MEM_LOGE("%d bytes failed 0x%x\n", u32Size, s32Ret);
        return SAL_FAIL;
    }
    *pVbBlk = (UINT64)mbBlk;

    pVirAddr = RK_MPI_MB_Handle2VirAddr(mbBlk);
    physAddr = RK_MPI_MB_Handle2PhysAddr(mbBlk);      /*不连续内存无法获取缓存模块物理内存*/
    *ppRetVirAddr = pVirAddr;
    *pRetPhysAddr = physAddr; 
    

    return SAL_SOK;

}

/**
 * @function    mem_rk_mmzFree
 * @brief       释放mmz
 * @param[in]   U64PhysAddr 物理地址
 *              pVirAddr    虚拟地址
 * @param[out]
 * @return
 */
static INT32 mem_rk_mmzFree(UINT32 allocSize, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk)
{
    INT32 s32Ret;
    MB_BLK mbBlk = (MB_BLK)vbBlk;

    s32Ret = RK_MPI_SYS_MmzFree(mbBlk);
    if (SAL_SOK != s32Ret)
    {
        MEM_LOGE("RK_MPI_SYS_MmzFree failed\n");
        return SAL_FAIL;
    }

    return SAL_SOK;

}

/**
 * @function    mem_rk_mmzFree
 * @brief       释放mmz
 * @param[in]   U64PhysAddr 物理地址
 *              pVirAddr    虚拟地址
 * @param[out]
 * @return
 */
static INT32 mem_rk_cmaMmzFree(UINT32 allocSize, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk)
{
    INT32 s32Ret;
    MB_BLK mbBlk = (MB_BLK)vbBlk;

	if (vbBlk == 0)
	{
        mbBlk = RK_MPI_MMZ_VirAddr2Handle(pVirAddr);
		if (NULL == mbBlk)
		{
			MEM_LOGE("vir addr 2 handle failed! \n");
			return SAL_FAIL;
		}
	}

    s32Ret = RK_MPI_MMZ_Free(mbBlk);
    if (SAL_SOK != s32Ret)
    {
        MEM_LOGE("RK_MPI_SYS_MmzFree failed\n");
        return SAL_FAIL;
    }

    return SAL_SOK;

}


/**
 * @function    mem_rk_mmzFlushCache
 * @brief       刷cache
 * @param[in]   vbBlk 
 *              bReadOnly    true：使cache里内容无效；false：把cache里内容回写到mem并且使cache无效
 * @param[out]
 * @return
 */
static INT32 mem_rk_mmzFlushCache(UINT64 vbBlk, BOOL bReadOnly)
{
    INT32 s32Ret;
    MB_BLK mbBlk = (MB_BLK)vbBlk;

    s32Ret = RK_MPI_SYS_MmzFlushCache(mbBlk, (RK_BOOL)bReadOnly);
    if (SAL_SOK != s32Ret)
    {
        MEM_LOGE("RK_MPI_SYS_MmzFlushCache failed\n");
        return SAL_FAIL;
    }

    return SAL_SOK;

}

/**
 * @function    mem_rk_vbAlloc
 * @brief       申请vb
 * @param[in]   u32Size 需要申请的内存大小
 *              pcZone vb申请的内存区域
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址
 *              pPoolId      内存池ID
 *              pVbBlk       vb block
 * @return
 */
static INT32 mem_rk_vbAlloc(UINT32 u32Size, CHAR *pcZone, BOOL bCached, UINT64 *retPhysAddr, VOID **retVirtAddr, UINT32 *pPoolId, UINT64 *pVbBlk)
{
    MB_POOL poolId = MB_INVALID_POOLID;
    MB_BLK vbBlk = 0;
    Ptr pBufBase = NULL;
    PhysAddr pPhyBase;
  
    MB_POOL_CONFIG_S stMbPoolCfg;

    if (0 == u32Size)
    {
        MEM_LOGE("size must > 0\n");
        return SAL_FAIL;
    }

    memset(&stMbPoolCfg, 0, sizeof(MB_POOL_CONFIG_S));
    stMbPoolCfg.u64MBSize = u32Size;
    stMbPoolCfg.u32MBCnt = 1;
    /* MB_REMAP_MODE_NONE 这个模块RK底层跟MB_REMAP_MODE_CACHED一致，跟海思平台有区别 */
    stMbPoolCfg.enRemapMode = (SAL_TRUE == bCached) ? MB_REMAP_MODE_CACHED: MB_REMAP_MODE_NOCACHE;

    /* 申请内存 */
    poolId = RK_MPI_MB_CreatePool(&stMbPoolCfg);
    if (MB_INVALID_POOLID == poolId)
    {
        MEM_LOGE("createPool failed size %d!\n", u32Size);
        return SAL_FAIL;
    }

    vbBlk = RK_MPI_MB_GetMB(poolId, u32Size, RK_TRUE);
    if (MB_INVALID_HANDLE == vbBlk)
    {
        MEM_LOGE("GetBlock failed size %d!\n", u32Size);
        return SAL_FAIL;
    }

    pBufBase = RK_MPI_MB_Handle2VirAddr(vbBlk);
    if (NULL == pBufBase)
    {
        MEM_LOGE("hal mmap failed size %d!\n", u32Size);
        return SAL_FAIL;
    }

    /* SAL_INFO("HAL mallocFd size:%d, phyBase:%p BufBase:%p\n", allocSize, pPhyBase, pBufBase); */

    memset(pBufBase, 0x00, u32Size);
    pPhyBase = INVALID_PHYADDR;    /*不连续内存无法获取缓存模块物理内存*/
    
    *retPhysAddr = pPhyBase;
    *retVirtAddr = pBufBase;
    *pPoolId = poolId;
    *pVbBlk = (UINT64)vbBlk;

    return SAL_SOK;
}

/**
 * @function    mem_rk_vbFree
 * @brief       释放vb
 * @param[in]   u32Size 需要释放的内存大小
 *              virAddr 需要释放的虚拟地址
 *              poolId      内存池ID
 *              vbBlk       vb block
 * @param[out]  
 * @return
 */
static INT32 mem_rk_vbFree(UINT32 u32Size, VOID *virAddr, UINT32 poolId, UINT64 vbBlk)
{
    INT32 s32Ret = 0;
    MB_BLK vblk=  (MB_BLK)vbBlk;
    MB_POOL mbPool = (MB_POOL)poolId;

    if(0 == poolId)
    {
        poolId = RK_MPI_MB_Handle2PoolId(vblk);
        if (MB_INVALID_POOLID == mbPool)
        {
            MEM_LOGE("handle 2 poolid failed! ret: 0x%x \n", mbPool);
            return SAL_FAIL;
        }
    }

    s32Ret = RK_MPI_MB_ReleaseMB(vblk);
    if (SAL_SOK != s32Ret)
    {
        MEM_LOGE("hFreeFd release %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = RK_MPI_MB_DestroyPool(poolId);
    if (SAL_SOK != s32Ret)
    {
        MEM_LOGE("hFreeFd destroy %#x,poolId %d\n", s32Ret, poolId);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    mem_rk_vbPoolAlloc
 * @brief       申请vb内存池，目前主要给vpss使用
 * @param[in]   pstVbInfo 需要申请的vb宽高及个数信息
 *              pcZone vb申请的内存区域
 * @param[out]  pstVbInfo 返回poolID和size
 * @return
 */
static INT32 mem_rk_vbPoolAlloc(VB_INFO_S *pstVbInfo, CHAR *aszZone)
{
    
    UINT32      allocSize   = 0;
    MB_POOL     poolId      = MB_INVALID_POOLID;
    UINT64  u64BlkSize  = 0;
    MB_POOL_CONFIG_S stVbPoolCfg;

    if(NULL == pstVbInfo)
    {
        MEM_LOGE("pstVbInfo error\n");
        return SAL_FAIL;
    }

    /* vpss/venc/vdec等不同模块宽高对齐在外部做，内部不好统一 */
    u64BlkSize  = (UINT64)((UINT64)pstVbInfo->u32BlkWidth * (UINT64)pstVbInfo->u32BlkHeight * SAL_getBitsPerPixel(pstVbInfo->enSalPixelFmt)) >> 3;
    if(0 == u64BlkSize)
    {
        MEM_LOGE("size must > 0\n");
         return SAL_FAIL;
    }
    
    memset(&stVbPoolCfg, 0, sizeof(MB_POOL_CONFIG_S));    
    /* 总的要申请的内存大小 */
    allocSize = u64BlkSize;
    
    stVbPoolCfg.u64MBSize  = allocSize;
    stVbPoolCfg.u32MBCnt   = (pstVbInfo->u32BlkCnt == 0) ? 1 : pstVbInfo->u32BlkCnt;
    stVbPoolCfg.enRemapMode = MB_REMAP_MODE_NONE;
  
   
    /* 申请内存 */
    poolId = RK_MPI_MB_CreatePool(&stVbPoolCfg);
    if(MB_INVALID_POOLID == poolId)
    {
        MEM_LOGE("createPool failed size %d!\n", allocSize);
        return SAL_FAIL;
    }

    pstVbInfo->u32VbPoolID = poolId;
    pstVbInfo->u32PoolSize = allocSize * pstVbInfo->u32BlkCnt;

    return SAL_SOK;
}

/**
 * @function    mem_rk_vbPoolFree
 * @brief       释放vb池
 * @param[in]   VB_INFO_S 需要释放的poolID
 * @param[out]  
 * @return
 */
static INT32 mem_rk_vbPoolFree(VB_INFO_S *pstVbInfo)
{
    INT32       s32Ret      = 0;
    MB_POOL      u32VbPool = (-1U);

    if(NULL == pstVbInfo)
    {
        MEM_LOGE("pstVbInfo error\n");
        return SAL_FAIL;
    }

    u32VbPool = pstVbInfo->u32VbPoolID;
    /* destroy video buffer pool */
    s32Ret = RK_MPI_MB_DestroyPool(u32VbPool);
    if (SAL_SOK != s32Ret)
    {
        MEM_LOGE("Err, s32Ret:%#x\n", s32Ret);

        return SAL_FAIL;
    }
    
    return SAL_SOK;
}


/**
 * @function    mem_rk_vbGetVirAddr
 * @brief       获取vbBlk虚拟地址
 * @param[in]   UINT64 vbBlk
                VOID **retVirtAddr返回的虚拟地址
 * @param[out]  
 * @return
 */
static INT32 mem_rk_vbGetVirAddr(UINT64 vbBlk,VOID **retVirtAddr)
{

    Ptr pBufBase = NULL;

    if(0 == vbBlk)
    {
        MEM_LOGE("vbBlk err %llu!\n", vbBlk);
    }
    MB_BLK vblk=  (MB_BLK)vbBlk;
    
    pBufBase = RK_MPI_MB_Handle2VirAddr(vblk);
    if (NULL == pBufBase)
    {
        MEM_LOGE("mem_rk_vbGetVirAddr err %llu!\n", vbBlk);
        return SAL_FAIL;
    }

    *retVirtAddr = pBufBase;
    
    return SAL_SOK;
}

/**
 * @function    mem_rk_vbGetSize
 * @brief       获取vbBlk size
 * @param[in]   UINT64 srcVbBlk
                UINT64 *u64Size
 * @param[out]  
 * @return
 */
static INT32 mem_rk_vbGetSize(UINT64 vbBlk,UINT64 *u64Size)
{
    
    if(0 == vbBlk)
    {
        MEM_LOGE("vbBlk err %llu!\n", vbBlk);
        return SAL_FAIL;
    }
    
    *u64Size = RK_MPI_MB_GetSize((MB_BLK)vbBlk);
    return SAL_SOK;
}


/**
 * @function    mem_rk_vbGetOffset
 * @brief       获取vbBlk偏移值
 * @param[in]   UINT64 srcVbBlk
                UINT32 *u32Offset
 * @param[out]  
 * @return
 */
static INT32 mem_rk_vbGetOffset(UINT64 vbBlk,UINT32 *u32Offset)
{
    
    if(0 == vbBlk)
    {
        MEM_LOGE("vbBlk err %llu!\n", vbBlk);
        return SAL_FAIL;
    }
    
    *u32Offset = RK_MPI_MB_GetOffset((MB_BLK)vbBlk);
    return SAL_SOK;
}


/**
 * @function    mem_rk_vbMap
 * @brief       创建vb，和源vb指向同一块内存
 * @param[in]   UINT64 srcVbBlk
                UINT64 dstVbBlk
 * @param[out]  
 * @return
 */
static INT32 mem_rk_vbMap(UINT64 srcVbBlk,UINT64 *dstVbBlk)
{
    INT32 s32Ret = 0;
    MB_BLK vblk;
    MB_EXT_CONFIG_S extConfig = {0};
    
    extConfig.pu8VirAddr  = (RK_U8 *)RK_MPI_MB_Handle2VirAddr((MB_BLK)srcVbBlk);
    extConfig.u64Size    = RK_MPI_MB_GetSize((MB_BLK)srcVbBlk);
    extConfig.s32Fd = RK_MPI_MB_Handle2Fd((MB_BLK)srcVbBlk);
    
    s32Ret = RK_MPI_SYS_CreateMB(&vblk, &extConfig);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_MPI_SYS_CreateMB err 0x%x! \n", s32Ret);
        return SAL_FAIL;
    }

    *dstVbBlk = (UINT64)vblk;
    
    return SAL_SOK;
}

/**
 * @function    mem_rk_vbDelete
 * @brief       销毁vb
 * @param[in]   UINT64 VbBlk
 * @param[out]  
 * @return
 */
static INT32 mem_rk_vbDelete(UINT64 vbBlk)
{
    INT32 s32Ret = 0;

    s32Ret = RK_MPI_SYS_Free((MB_BLK)vbBlk);
    if (SAL_SOK != s32Ret)
    {
        SAL_LOGE("RK_MPI_SYS_Free MB 0x%x! \n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   mem_rk_iommuMmzAlloc
 * @brief      iommu内存申请
 * @param[in]  BOOL bCached          
 * @param[in]  CHAR *pcMmb           
 * @param[in]  CHAR *pcZone          
 * @param[in]  UINT32 u32Size        
 * @param[in]  UINT64 *pRetPhysAddr  
 * @param[in]  VOID **ppRetVirAddr   
 * @param[in]  UINT64 *pVbBlk        
 * @param[out] None
 * @return     static INT32
 */
static INT32 mem_rk_iommuMmzAlloc(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 u32Size, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr, UINT64 *pVbBlk)
{
    Ptr pVirAddr = NULL;
    RK_U64 physAddr;
    MB_BLK mbBlk;
    INT32 s32Ret;

    if (bCached)
    {
        s32Ret = RK_MPI_MMZ_Alloc(&mbBlk, u32Size, RK_MMZ_ALLOC_TYPE_IOMMU | RK_MMZ_ALLOC_CACHEABLE);
    }
    else
    {
        s32Ret = RK_MPI_MMZ_Alloc(&mbBlk, u32Size, RK_MMZ_ALLOC_TYPE_IOMMU | RK_MMZ_ALLOC_UNCACHEABLE);
    }
    if (SAL_SOK != s32Ret)
    {
        MEM_LOGE("%d bytes failed\n", u32Size);
        return SAL_FAIL;
    }
    *pVbBlk = (UINT64)mbBlk;

    pVirAddr = RK_MPI_MB_Handle2VirAddr(mbBlk);
    physAddr = RK_MPI_MB_Handle2PhysAddr(mbBlk);      /*不连续内存无法获取缓存模块物理内存*/

    *ppRetVirAddr = pVirAddr;
    *pRetPhysAddr = physAddr; 
    
    return SAL_SOK;
}

/**
 * @function   mem_rk_iommuMmzFree
 * @brief      释放iommu内存
 * @param[in]  UINT32 allocSize    
 * @param[in]  UINT64 U64PhysAddr  
 * @param[in]  VOID *pVirAddr      
 * @param[in]  UINT64 vbBlk        
 * @param[out] None
 * @return     static INT32
 */
static INT32 mem_rk_iommuMmzFree(UINT32 allocSize, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk)
{
    INT32 s32Ret;
    MB_BLK mbBlk = (MB_BLK)vbBlk;

	if (vbBlk == 0)
	{
        mbBlk = RK_MPI_MMZ_VirAddr2Handle(pVirAddr);
		if (NULL == mbBlk)
		{
			MEM_LOGE("vir addr 2 handle failed! \n");
			return SAL_FAIL;
		}
	}

    s32Ret = RK_MPI_MMZ_Free(mbBlk);
    if (SAL_SOK != s32Ret)
    {
        MEM_LOGE("RK_MPI_SYS_MmzFree failed\n");
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    mem_hal_register
 * @brief       对平台相关的内存申请函数进行注册
 * @param[in]
 * @param[out]
 * @return
 */
const MEM_PLAT_OPS_S *mem_hal_register(void)
{
    g_stMemPlatOps.mmzAlloc = mem_rk_mmzAlloc;
    g_stMemPlatOps.cmaMmzAlloc = mem_rk_cmaMmzAlloc;
    g_stMemPlatOps.iommuMmzAlloc = mem_rk_iommuMmzAlloc;
    g_stMemPlatOps.mmzFree = mem_rk_mmzFree;
    g_stMemPlatOps.cmaMmzFree = mem_rk_cmaMmzFree;
    g_stMemPlatOps.iommuMmzFree = mem_rk_iommuMmzFree;
    g_stMemPlatOps.mmzFlushCache = mem_rk_mmzFlushCache;
    g_stMemPlatOps.vbAlloc = mem_rk_vbAlloc;
    g_stMemPlatOps.vbFree = mem_rk_vbFree;
    g_stMemPlatOps.vbPoolAlloc = mem_rk_vbPoolAlloc;
    g_stMemPlatOps.vbPoolFree = mem_rk_vbPoolFree;
    g_stMemPlatOps.vbGetVirAddr = mem_rk_vbGetVirAddr;
    g_stMemPlatOps.vbGetSize = mem_rk_vbGetSize;
    g_stMemPlatOps.vbGetOffset = mem_rk_vbGetOffset;
    g_stMemPlatOps.vbMap =  mem_rk_vbMap;
    g_stMemPlatOps.vbDelete = mem_rk_vbDelete;
    return &g_stMemPlatOps;
}


