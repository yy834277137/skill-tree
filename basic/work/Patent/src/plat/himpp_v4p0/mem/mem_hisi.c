/**
 * @file   mem_hisi.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   海思平台 HiMPP v4.0 mmz/vb的内存申请释放接口
 * @author  yeyanzhong
 * @date    2021.5.20
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

/**
 * @function    mem_hisi_mmzAlloc
 * @brief       申请mmz
 * @param[in]   u32Size 需要申请的内存大小
 *              pcMmb mmzBlock名字
 *              pcZone vb申请的内存区域
 *              bCache 映射时是否带cache
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址

 * @return
 */
static INT32 mem_hisi_mmzAlloc(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 u32Size, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr, UINT64 *pVbBlk)
{
    Ptr pVirAddr = NULL;
    PhysAddr physAddr;
    HI_S32 s32Ret;

    if (bCached)
    {
        s32Ret = HI_MPI_SYS_MmzAlloc_Cached((HI_U64 *)(&physAddr), ((HI_VOID **)(&pVirAddr)), pcMmb, pcZone, u32Size);
    }
    else
    {
        s32Ret = HI_MPI_SYS_MmzAlloc((HI_U64 *)(&physAddr), ((HI_VOID **)(&pVirAddr)), pcMmb, pcZone, u32Size);

    }

    if (HI_SUCCESS != s32Ret)
    {
        MEM_LOGE("%d bytes failed\n", u32Size);
        return SAL_FAIL;
    }

    *pRetPhysAddr = physAddr;
    *ppRetVirAddr = pVirAddr;

    return SAL_SOK;

}

/**
 * @function    mem_hisi_mmzFree
 * @brief       释放mmz
 * @param[in]   U64PhysAddr 物理地址
 *              pVirAddr    虚拟地址
 * @param[out]
 * @return
 */
static INT32 mem_hisi_mmzFree(UINT32 allocSize, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk)
{
    HI_S32 s32Ret;

    s32Ret = HI_MPI_SYS_MmzFree(U64PhysAddr, pVirAddr);
    if (HI_SUCCESS != s32Ret)
    {
        MEM_LOGE("HI_MPI_SYS_MmzFree failed\n");
        return SAL_FAIL;
    }

    return SAL_SOK;

}

/**
 * @function    mem_hisi_vbAlloc
 * @brief       申请vb
 * @param[in]   u32Size 需要申请的内存大小
 *              pcZone vb申请的内存区域
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址
 *              pPoolId      内存池ID
 *              pVbBlk       vb block
 * @return
 */
static INT32 mem_hisi_vbAlloc(UINT32 u32Size, CHAR *pcZone, BOOL bCached, UINT64 *retPhysAddr, VOID **retVirtAddr, UINT32 *pPoolId, UINT64 *pVbBlk)
{
    VB_POOL poolId = VB_INVALID_POOLID;
    VB_BLK vbBlk = 0;
    Ptr pBufBase = NULL;
    PhysAddr pPhyBase;
    VB_POOL_CONFIG_S stVbPoolCfg;

    if (0 == u32Size)
    {
        MEM_LOGE("size must > 0\n");
        return SAL_FAIL;
    }

    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));
    stVbPoolCfg.u64BlkSize = u32Size;
    stVbPoolCfg.u32BlkCnt = 1;
    stVbPoolCfg.enRemapMode = (SAL_TRUE == bCached) ? VB_REMAP_MODE_CACHED: VB_REMAP_MODE_NONE;
    if (NULL != pcZone)
    {
        strncpy(stVbPoolCfg.acMmzName, pcZone, (MAX_MMZ_NAME_LEN - 1));
        stVbPoolCfg.acMmzName[MAX_MMZ_NAME_LEN - 1] = '\0';
    }

    /* 申请内存 */
    poolId = HI_MPI_VB_CreatePool(&stVbPoolCfg);
    if (VB_INVALID_POOLID == poolId)
    {
        MEM_LOGE("createPool failed size %d!\n", u32Size);
        return SAL_FAIL;
    }

    vbBlk = HI_MPI_VB_GetBlock(poolId, u32Size, NULL);
    if (VB_INVALID_HANDLE == vbBlk)
    {
        MEM_LOGE("GetBlock failed size %d!\n", u32Size);
        return SAL_FAIL;
    }

    pPhyBase = HI_MPI_VB_Handle2PhysAddr(vbBlk);
    if (0 == pPhyBase)
    {
        MEM_LOGE("Handle2PhysAddr failed size %d!\n", u32Size);
        return SAL_FAIL;
    }

    pBufBase = HI_MPI_SYS_Mmap(pPhyBase, u32Size);
    if (NULL == pBufBase)
    {
        MEM_LOGE("hal mmap failed size %d!\n", u32Size);
        return SAL_FAIL;
    }

    /* SAL_INFO("HAL mallocFd size:%d, phyBase:%p BufBase:%p\n", allocSize, pPhyBase, pBufBase); */

    memset(pBufBase, 0x00, u32Size);

    *retPhysAddr = pPhyBase;
    *retVirtAddr = pBufBase;
    *pPoolId = poolId;
    *pVbBlk = (UINT64)vbBlk;

    return SAL_SOK;
}

/**
 * @function    mem_hisi_vbFree
 * @brief       释放vb
 * @param[in]   u32Size 需要释放的内存大小
 *              virAddr 需要释放的虚拟地址
 *              poolId      内存池ID
 *              vbBlk       vb block
 * @param[out]  
 * @return
 */
static INT32 mem_hisi_vbFree(UINT32 u32Size, VOID *virAddr, UINT32 poolId, UINT64 vbBlk)
{
    INT32 s32Ret = 0;
    VB_BLK vblk=  (VB_BLK)vbBlk;


    if(0 == poolId)
    {
        poolId = HI_MPI_VB_Handle2PoolId(vblk);
    }

    s32Ret = HI_MPI_SYS_Munmap(virAddr, u32Size);
    if (HI_SUCCESS != s32Ret)
    {
        MEM_LOGE("hFreeFd umap %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VB_ReleaseBlock(vblk);
    if (HI_SUCCESS != s32Ret)
    {
        MEM_LOGE("hFreeFd release %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = HI_MPI_VB_DestroyPool(poolId);
    if (HI_SUCCESS != s32Ret)
    {
        MEM_LOGE("hFreeFd destroy %#x,poolId %d\n", s32Ret, poolId);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    mem_hisi_vbPoolAlloc
 * @brief       申请vb内存池，目前主要给vpss使用
 * @param[in]   pstVbInfo 需要申请的vb宽高及个数信息
 *              pcZone vb申请的内存区域
 * @param[out]  pstVbInfo 返回poolID和size
 * @return
 */
static INT32 mem_hisi_vbPoolAlloc(VB_INFO_S *pstVbInfo, CHAR *aszZone)
{
    INT32       s32Ret      = 0;
    UINT32      allocSize   = 0;
    VB_POOL     poolId      = VB_INVALID_POOLID;
	UINT64  u64BlkSize  = 0;

    VB_POOL_CONFIG_S stVbPoolCfg;

	if(NULL == pstVbInfo)
    {
        MEM_LOGE("pstVbInfo error\n");
        return SAL_FAIL;
    }

    u64BlkSize  = (UINT64)((UINT64)pstVbInfo->u32BlkWidth * (UINT64)pstVbInfo->u32BlkHeight * 3 / 2);
    if(0 == u64BlkSize)
    {
        MEM_LOGE("size must > 0\n");
         return SAL_FAIL;
    }
	
    memset(&stVbPoolCfg, 0, sizeof(VB_POOL_CONFIG_S));    
    /* 总的要申请的内存大小 */
    allocSize = u64BlkSize;
    
    stVbPoolCfg.u64BlkSize  = allocSize;
    stVbPoolCfg.u32BlkCnt   = (pstVbInfo->u32BlkCnt == 0) ? 1 : pstVbInfo->u32BlkCnt;
    stVbPoolCfg.enRemapMode = VB_REMAP_MODE_NONE;
    if (aszZone != NULL)
    {
        snprintf(stVbPoolCfg.acMmzName, sizeof(stVbPoolCfg.acMmzName), "%s", aszZone);
    }

    /* 申请内存 */
    poolId = HI_MPI_VB_CreatePool(&stVbPoolCfg);
    if(VB_INVALID_POOLID == poolId)
    {
        MEM_LOGE("createPool failed size %d!\n", allocSize);
        return SAL_FAIL;
    }

    /* mmap a video buffer pool */
    s32Ret = HI_MPI_VB_MmapPool(poolId);
    if (HI_SUCCESS != s32Ret)
    {
        MEM_LOGE("HI_MPI_VB_MmapPool Err, pVbPool:%#x\n", poolId);

        (void)HI_MPI_VB_DestroyPool(poolId);

        return SAL_FAIL;
    }

    pstVbInfo->u32VbPoolID = poolId;
    pstVbInfo->u32PoolSize = allocSize * pstVbInfo->u32BlkCnt;

    return SAL_SOK;
}

/**
 * @function    mem_hisi_vbPoolFree
 * @brief       释放vb池
 * @param[in]   VB_INFO_S 需要释放的poolID
 * @param[out]  
 * @return
 */
static INT32 mem_hisi_vbPoolFree(VB_INFO_S *pstVbInfo)
{
    INT32       s32Ret      = 0;
    UINT32      u32VbPool = (-1U);

	if(NULL == pstVbInfo)
    {
        MEM_LOGE("pstVbInfo error\n");
        return SAL_FAIL;
    }

    u32VbPool = pstVbInfo->u32VbPoolID;
    /* unmmap a video buffer pool*/
    s32Ret = HI_MPI_VB_MunmapPool(u32VbPool);
    if (HI_SUCCESS != s32Ret)
    {
        MEM_LOGE("Err, s32Ret:%#x\n", s32Ret);

        (void)HI_MPI_VB_DestroyPool(u32VbPool);

        return SAL_FAIL;
    }

    /* destroy video buffer pool */
    s32Ret = HI_MPI_VB_DestroyPool(u32VbPool);
    if (HI_SUCCESS != s32Ret)
    {
        MEM_LOGE("Err, s32Ret:%#x\n", s32Ret);

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
    g_stMemPlatOps.mmzAlloc = mem_hisi_mmzAlloc;
    g_stMemPlatOps.mmzFree = mem_hisi_mmzFree;
    g_stMemPlatOps.vbAlloc = mem_hisi_vbAlloc;
    g_stMemPlatOps.vbFree = mem_hisi_vbFree;
    g_stMemPlatOps.vbPoolAlloc = mem_hisi_vbPoolAlloc;
    g_stMemPlatOps.vbPoolFree = mem_hisi_vbPoolFree;

    return &g_stMemPlatOps;
}


