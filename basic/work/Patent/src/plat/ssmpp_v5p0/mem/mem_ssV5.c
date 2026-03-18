/**
 * @file   mem_ssV5.c
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
 * @function    mem_ssV5_mmzAlloc
 * @brief       申请mmz
 * @param[in]   u32Size 需要申请的内存大小
 *              pcMmb mmzBlock名字
 *              pcZone vb申请的内存区域
 *              bCache 映射时是否带cache
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址

 * @return
 */
static INT32 mem_ssV5_mmzAlloc(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 u32Size, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr, UINT64 *pVbBlk)
{
    Ptr pVirAddr = NULL;
    PhysAddr physAddr;
    td_s32 s32Ret;

    if (bCached)
    {
        s32Ret = ss_mpi_sys_mmz_alloc_cached((td_phys_addr_t *)(&physAddr), ((td_void **)(&pVirAddr)), pcMmb, pcZone, u32Size);
    }
    else
    {
        s32Ret = ss_mpi_sys_mmz_alloc((td_phys_addr_t *)(&physAddr), ((td_void **)(&pVirAddr)), pcMmb, pcZone, u32Size);

    }

    if (TD_SUCCESS != s32Ret)
    {
        MEM_LOGE("%d bytes failed\n", u32Size);
        return SAL_FAIL;
    }

    *pRetPhysAddr = physAddr;
    *ppRetVirAddr = pVirAddr;

    return SAL_SOK;

}

/**
 * @function    mem_ssV5_mmzFree
 * @brief       释放mmz
 * @param[in]   U64PhysAddr 物理地址
 *              pVirAddr    虚拟地址
 * @param[out]
 * @return
 */
static INT32 mem_ssV5_mmzFree(UINT32 u32Size, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk)
{
    td_s32 s32Ret;

    s32Ret = ss_mpi_sys_mmz_free(U64PhysAddr, pVirAddr);
    if (TD_SUCCESS != s32Ret)
    {
        MEM_LOGE("ss_mpi_sys_mmz_free failed\n");
        return SAL_FAIL;
    }

    return SAL_SOK;

}

/**
 * @function    mem_ssV5_vbAlloc
 * @brief       申请vb
 * @param[in]   u32Size 需要申请的内存大小
 *              pcZone vb申请的内存区域
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址
 *              pPoolId      内存池ID
 *              pVbBlk       vb block
 * @return
 */
static INT32 mem_ssV5_vbAlloc(UINT32 u32Size, CHAR *pcZone, BOOL bCached, UINT64 *retPhysAddr, VOID **retVirtAddr, UINT32 *pPoolId, UINT64 *pVbBlk)
{
    ot_vb_pool poolId = OT_VB_INVALID_POOL_ID;
    ot_vb_blk vbBlk = 0;
    Ptr pBufBase = NULL;
    PhysAddr pPhyBase;
    ot_vb_pool_cfg stVbPoolCfg;

    if (0 == u32Size)
    {
        MEM_LOGE("size must > 0\n");
        return SAL_FAIL;
    }

    memset(&stVbPoolCfg, 0, sizeof(stVbPoolCfg));
    stVbPoolCfg.blk_size = u32Size;
    stVbPoolCfg.blk_cnt = 1;
    stVbPoolCfg.remap_mode = (SAL_TRUE == bCached) ? OT_VB_REMAP_MODE_CACHED : OT_VB_REMAP_MODE_NONE;
    if (NULL != pcZone)
    {
        strncpy(stVbPoolCfg.mmz_name, pcZone, (OT_MAX_MMZ_NAME_LEN - 1));
        stVbPoolCfg.mmz_name[OT_MAX_MMZ_NAME_LEN - 1] = '\0';
    }

    /* 申请内存 */
    poolId = ss_mpi_vb_create_pool(&stVbPoolCfg);
    if (OT_VB_INVALID_POOL_ID == poolId)
    {
        MEM_LOGE("createPool failed size %d!\n", u32Size);
        return SAL_FAIL;
    }

    vbBlk = ss_mpi_vb_get_blk(poolId, u32Size, NULL);
    if (OT_VB_INVALID_HANDLE == vbBlk)
    {
        MEM_LOGE("GetBlock failed size %d!\n", u32Size);
        return SAL_FAIL;
    }

    pPhyBase = ss_mpi_vb_handle_to_phys_addr(vbBlk);
    if (0 == pPhyBase)
    {
        MEM_LOGE("Handle2PhysAddr failed size %d!\n", u32Size);
        return SAL_FAIL;
    }

    pBufBase = ss_mpi_sys_mmap(pPhyBase, u32Size);
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
 * @function    mem_ssV5_vbFree
 * @brief       释放vb
 * @param[in]   u32Size 需要释放的内存大小
 *              virAddr 需要释放的虚拟地址
 *              poolId      内存池ID
 *              vbBlk       vb block
 * @param[out]  
 * @return
 */
static INT32 mem_ssV5_vbFree(UINT32 u32Size, VOID *virAddr, UINT32 poolId, UINT64 vbBlk)
{
    INT32 s32Ret = 0;
    ot_vb_blk vb_blk = (ot_vb_blk)vbBlk;

    if(0 == poolId)
    {
        poolId = ss_mpi_vb_handle_to_pool_id(vb_blk);
    }

    s32Ret = ss_mpi_sys_munmap(virAddr, u32Size);
    if (TD_SUCCESS != s32Ret)
    {
        MEM_LOGE("hFreeFd umap %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vb_release_blk(vb_blk);
    if (TD_SUCCESS != s32Ret)
    {
        MEM_LOGE("hFreeFd release %#x\n", s32Ret);
        return SAL_FAIL;
    }

    s32Ret = ss_mpi_vb_destroy_pool(poolId);
    if (TD_SUCCESS != s32Ret)
    {
        MEM_LOGE("hFreeFd destroy %#x,poolId %d\n", s32Ret, poolId);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function    mem_ssV5_vbPoolAlloc
 * @brief       申请vb内存池，目前主要给vpss使用
 * @param[in]   pstVbInfo 需要申请的vb宽高及个数信息
 *              pcZone vb申请的内存区域
 * @param[out]  pstVbInfo 返回poolID和size
 * @return
 */
static INT32 mem_ssV5_vbPoolAlloc(VB_INFO_S *pstVbInfo, CHAR *aszZone)
{
    UINT32      allocSize   = 0;
    ot_vb_pool     poolId      = OT_VB_INVALID_POOL_ID;
	UINT64  u64BlkSize  = 0;

    ot_vb_pool_cfg stVbPoolCfg;

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
	
    memset(&stVbPoolCfg, 0, sizeof(stVbPoolCfg));    
    /* 总的要申请的内存大小 */
    allocSize = u64BlkSize;
    
    stVbPoolCfg.blk_size  = allocSize;
    stVbPoolCfg.blk_cnt   = (pstVbInfo->u32BlkCnt == 0) ? 1 : pstVbInfo->u32BlkCnt;
    stVbPoolCfg.remap_mode = OT_VB_REMAP_MODE_NONE;
    if (aszZone != NULL)
    {
        snprintf(stVbPoolCfg.mmz_name, sizeof(stVbPoolCfg.mmz_name), "%s", aszZone);
    }

    /* 申请内存 */
    poolId = ss_mpi_vb_create_pool(&stVbPoolCfg);
    if(OT_VB_INVALID_POOL_ID == poolId)
    {
        MEM_LOGE("createPool failed size %d!\n", allocSize);
        return SAL_FAIL;
    }

#if 0
    /* mmap a video buffer pool, ssMpp v5.0里下面的接口已去掉 */
    s32Ret = OT_MPI_VB_MmapPool(poolId);
    if (TD_SUCCESS != s32Ret)
    {
        MEM_LOGE("OT_MPI_VB_MmapPool Err, pVbPool:%#x\n", poolId);

        (void)ot_mpi_vb_destroy_pool(poolId);

        return SAL_FAIL;
    }
#endif
    pstVbInfo->u32VbPoolID = poolId;
    pstVbInfo->u32PoolSize = allocSize * pstVbInfo->u32BlkCnt;

    return SAL_SOK;
}

/**
 * @function    mem_ssV5_vbPoolFree
 * @brief       释放vb池
 * @param[in]   VB_INFO_S 需要释放的poolID
 * @param[out]  
 * @return
 */
static INT32 mem_ssV5_vbPoolFree(VB_INFO_S *pstVbInfo)
{
    INT32       s32Ret      = 0;
    UINT32      u32VbPool = (-1U);

	if(NULL == pstVbInfo)
    {
        MEM_LOGE("pstVbInfo error\n");
        return SAL_FAIL;
    }

    u32VbPool = pstVbInfo->u32VbPoolID;
#if 0
    /* unmmap a video buffer pool*/
    s32Ret = OT_MPI_VB_MunmapPool(u32VbPool);
    if (TD_SUCCESS != s32Ret)
    {
        MEM_LOGE("Err, s32Ret:%#x\n", s32Ret);

        (void)ot_mpi_vb_destroy_pool(u32VbPool);

        return SAL_FAIL;
    }
#endif
    /* destroy video buffer pool */
    s32Ret = ss_mpi_vb_destroy_pool(u32VbPool);
    if (TD_SUCCESS != s32Ret)
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
    g_stMemPlatOps.mmzAlloc = mem_ssV5_mmzAlloc;
    g_stMemPlatOps.mmzFree = mem_ssV5_mmzFree;
    g_stMemPlatOps.vbAlloc = mem_ssV5_vbAlloc;
    g_stMemPlatOps.vbFree = mem_ssV5_vbFree;
    g_stMemPlatOps.vbPoolAlloc = mem_ssV5_vbPoolAlloc;
    g_stMemPlatOps.vbPoolFree = mem_ssV5_vbPoolFree;

    return &g_stMemPlatOps;
}


