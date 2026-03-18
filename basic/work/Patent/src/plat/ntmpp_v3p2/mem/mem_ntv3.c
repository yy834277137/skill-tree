/**
 * @file   mem_ntv3.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   NT平台 HDAL的内存申请释放接口
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

typedef struct{
       /*内存物理地址*/
    void *pPhyAddr;
    /*内存的虚拟地址*/
    void *pVirAddr;
    /*内存大小*/
    unsigned int memSize;
    /*poolType NT用*/
    unsigned int poolType;
    /*ddrId NT用*/
    unsigned int ddrId;
    
    BOOL bCached;

    HD_COMMON_MEM_VB_BLK blk;
}MEM_NT_INFO;


/**
 * @function    mem_ntv3Map
 * @brief       物理内存映射
 * @param[in]   pAddr 物理地址
 *              size  大小
 *              bCache 是否使用cache
 * @param[out]  

 * @return
 */
static void *mem_ntv3Map(void *pAddr, UINT size, UINT bCache)
{
    void *pVirAddr = NULL;
    HD_COMMON_MEM_MEM_TYPE memType = HD_COMMON_MEM_MEM_TYPE_MAX;

    memType = bCache ? HD_COMMON_MEM_MEM_TYPE_CACHE : HD_COMMON_MEM_MEM_TYPE_NONCACHE;

    pVirAddr = hd_common_mem_mmap(memType, (ULONG)pAddr, size);
    if (NULL == pVirAddr)
    {
        SAL_WARN("phy_addr = %p, size = 0x%x, hd_common_mem_mmap fail\n", pAddr, size);
        return NULL;
    }

    return pVirAddr;
}

/**
 * @function    mem_ntv3AllocMmz
 * @brief       申请mmz
 * @param[in]   pstMemInfo 申请的内存信息
 *              pool_type 申请的模块名称
 *              ddrId 申请的ddr id
 * @param[out]  

 * @return
 */
static int mem_ntv3AllocMmz(MEM_NT_INFO *pstMemInfo, UINT pool_type, UINT ddrId)
{

    HD_COMMON_MEM_VB_BLK blk = 0;

    if (NULL == pstMemInfo)
    {
       SAL_WARN("pstMemInfo is NULL\n");
       return SAL_FAIL;
    }

    if (pool_type < HD_COMMON_MEM_COMMON_POOL || pool_type > HD_COMMON_MEM_USER_DEFINIED_POOL)
    {
        SAL_WARN("input pooltype err pool_type:%d\n", pool_type);
        return SAL_FAIL;
    }

    blk = hd_common_mem_get_block(pool_type, pstMemInfo->memSize, ddrId);
    if (HD_COMMON_MEM_VB_INVALID_BLK == blk)
    {
        SAL_WARN("hd_common_mem_get_block fail! pool_type = %d, size = %d, ddrId = %d\n",
                pool_type,pstMemInfo->memSize, ddrId);
        return SAL_FAIL;
    }
    SAL_DEBUG("mem_get blk %lx! \n", blk);

    /*记录blk*/
    pstMemInfo->blk = blk;
    pstMemInfo->pPhyAddr = (void *)hd_common_mem_blk2pa(blk);
    if (NULL == pstMemInfo->pPhyAddr)
    {
        SAL_WARN("plt_mem_handleToPhysAddr fail!\n");
        hd_common_mem_release_block(blk);
        return SAL_FAIL;
    }

    if (DDR_ID0 == ddrId || DDR_ID1 == ddrId)
    {
        pstMemInfo->pVirAddr = mem_ntv3Map(pstMemInfo->pPhyAddr, pstMemInfo->memSize, pstMemInfo->bCached);
        if (NULL == pstMemInfo->pVirAddr)
        {
            SAL_WARN("mem_ntv3Map fail!\n");
            hd_common_mem_release_block(blk);
            return SAL_FAIL;
        }
    }
    else
    {
        pstMemInfo->pVirAddr = NULL;
    }

    return SAL_SOK;
}

#if 1
/**
 * @function    mem_ntv3_mmzAlloc
 * @brief       申请mmz
 * @param[in]   u32Size 需要申请的内存大小
 *              pcMmb mmzBlock名字
 *              pcZone vb申请的内存区域
 *              bCache 映射时是否带cache
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址

 * @return
 */
static INT32 mem_ntv3_mmzAllocDDR0(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 u32Size, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr, UINT64 *pVbBlk)
{

    UINT32 poolType = 0;
    UINT32 ddrId = 0;
    INT32  ret = SAL_SOK; 
    MEM_NT_INFO pMemParam = {0};
   
    poolType = HD_COMMON_MEM_CNN_POOL;   //智能专用申请内存块
    ddrId = DDR_ID0;
    
    memset(&pMemParam, 0, sizeof(MEM_NT_INFO));
    pMemParam.memSize = u32Size;
    pMemParam.bCached = bCached;
    pMemParam.poolType = poolType;
    pMemParam.ddrId = ddrId;
    /*此接口对接SYS层，SYS层申请内存写死user_blk，平台层直接调用mem_ntv3AllocMmz传入pool和ddr*/
    ret = mem_ntv3AllocMmz(&pMemParam, poolType, ddrId);
    if (ret)
    {
        ddrId = DDR_ID0;
        pMemParam.ddrId = ddrId;
        ret =mem_ntv3AllocMmz(&pMemParam, poolType, ddrId);// mem_ntv3AllocMmz(&pMemParam, HD_COMMON_MEM_COMMON_POOL, DDR_ID0);
        if (ret)
        {
            SAL_ERROR("MMZ alloc err=%#x size=%d addr=%p %p \n",
                    ret, pMemParam.memSize, pMemParam.pPhyAddr,
                    pMemParam.pVirAddr);
            ret = SAL_FAIL;
        }
        else
        {
            ret = SAL_SOK;
        }
    }
    else
    {
        ret = SAL_SOK;
    }

    *pRetPhysAddr = (UINT64)pMemParam.pPhyAddr;;
    *ppRetVirAddr = pMemParam.pVirAddr;
    *pVbBlk = pMemParam.blk;
    
    return ret;
 
}
#endif

/**
 * @function    mem_ntv3_mmzAlloc
 * @brief       申请mmz
 * @param[in]   u32Size 需要申请的内存大小
 *              pcMmb mmzBlock名字
 *              pcZone vb申请的内存区域
 *              bCache 映射时是否带cache
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址

 * @return
 */
static INT32 mem_ntv3_mmzAlloc(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 u32Size, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr, UINT64 *pVbBlk)
{

    UINT32 poolType = 0;
    UINT32 ddrId = 0;
    INT32  ret = SAL_SOK; 
    MEM_NT_INFO pMemParam = {0};
   
    poolType = HD_COMMON_MEM_USER_BLK;   //vb使用占时都用usr和ddr0中内存
    ddrId = DDR_ID1;
    
    memset(&pMemParam, 0, sizeof(MEM_NT_INFO));
    pMemParam.memSize = u32Size;
    pMemParam.bCached = bCached;
    pMemParam.poolType = poolType;
    pMemParam.ddrId = ddrId;
    /*此接口对接SYS层，SYS层申请内存写死user_blk，平台层直接调用mem_ntv3AllocMmz传入pool和ddr*/
    ret = mem_ntv3AllocMmz(&pMemParam, poolType, ddrId);
    if (ret)
    {
        ddrId = DDR_ID0;
        pMemParam.ddrId = ddrId;
        ret =mem_ntv3AllocMmz(&pMemParam, poolType, ddrId);// mem_ntv3AllocMmz(&pMemParam, HD_COMMON_MEM_COMMON_POOL, DDR_ID0);
        if (ret)
        {
            SAL_ERROR("MMZ alloc err=%#x size=%d addr=%p %p \n",
                    ret, pMemParam.memSize, pMemParam.pPhyAddr,
                    pMemParam.pVirAddr);
            ret = SAL_FAIL;
        }
        else
        {
            ret = SAL_SOK;
        }
    }
    else
    {
        ret = SAL_SOK;
    }

    *pRetPhysAddr = (UINT64)pMemParam.pPhyAddr;;
    *ppRetVirAddr = pMemParam.pVirAddr;
    *pVbBlk = pMemParam.blk;
    
    return ret;
 
}


/**
 * @function    mem_ntv3_mmzFree
 * @brief       释放mmz
 * @param[in]   U64PhysAddr 物理地址
 *              pVirAddr    虚拟地址
 * @param[out]
 * @return
 */
static INT32 mem_ntv3_mmzFree(UINT32 allocSize, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk)
{
    INT32  ret = SAL_SOK; 

    if (NULL == pVirAddr)
    {
        SAL_ERROR("intput virAddr is NULL\n");
        return SAL_FAIL;
    }

    ret = hd_common_mem_munmap(pVirAddr, allocSize);
    if (SAL_SOK != ret)
    {
        SAL_ERROR("plt_common_unmap failure :%#x\n", ret);
        return SAL_FAIL;
    }

    ret = hd_common_mem_release_block(vbBlk);
    if (ret != SAL_SOK)
    {
        SAL_ERROR("plt_mem_releaseVbBlock fail, rer:%#x\n", ret);
        return SAL_FAIL;
    }

    return SAL_FAIL;


}

/**
 * @function    mem_ntv3_vbAlloc
 * @brief       申请vb
 * @param[in]   u32Size 需要申请的内存大小
 *              pcZone vb申请的内存区域
 * @param[out]  pRetPhysAddr 返回的物理地址
 *              ppRetVirAddr 返回的虚拟地址
 *              pPoolId      内存池ID
 *              pVbBlk       vb block
 * @return
 */
static INT32 mem_ntv3_vbAlloc(UINT32 u32Size, CHAR *pcZone, BOOL bCached, UINT64 *retPhysAddr, VOID **retVirtAddr, UINT32 *pPoolId, UINT64 *pVbBlk)
{
    UINT32 poolType = 0;
    UINT32 ddrId = 0;
    INT32  s32Ret = SAL_SOK; 
    MEM_NT_INFO pMemParam = {0};
   
    poolType = HD_COMMON_MEM_USER_BLK;   //vb使用占时都用usr和ddr0中内存
    ddrId = DDR_ID1;

    memset(&pMemParam, 0, sizeof(MEM_NT_INFO));
    pMemParam.memSize = u32Size;
    pMemParam.bCached = bCached;
    pMemParam.poolType = poolType;
    pMemParam.ddrId = ddrId;
    /*此接口对接SYS层，SYS层申请内存写死user_blk，平台层直接调用mem_ntv3AllocMmz传入pool和ddr*/
    s32Ret = mem_ntv3AllocMmz(&pMemParam, poolType, ddrId);
    if (s32Ret)
    {
        ddrId = DDR_ID0;
        pMemParam.ddrId = ddrId;
        s32Ret = mem_ntv3AllocMmz(&pMemParam, poolType, ddrId); //mem_ntv3AllocMmz(&pMemParam, HD_COMMON_MEM_COMMON_POOL, DDR_ID0);
        if (s32Ret)
        {
            SAL_ERROR("MMZ alloc err=%#x size=%d addr=%p %p \n",
                    s32Ret, pMemParam.memSize, pMemParam.pPhyAddr,
                    pMemParam.pVirAddr);
            s32Ret = SAL_FAIL;
        }
        else
        {
            s32Ret = SAL_SOK;
        }
    }
    else
    {
        s32Ret = SAL_SOK;
    }

    *retPhysAddr = (UINT64)pMemParam.pPhyAddr;;
    *retVirtAddr = pMemParam.pVirAddr;
    *pPoolId = 0;
    *pVbBlk = pMemParam.blk;
    
    return s32Ret;
}

/**
 * @function    mem_ntv3_vbFree
 * @brief       释放vb
 * @param[in]   u32Size 需要释放的内存大小
 *              virAddr 需要释放的虚拟地址
 *              poolId      内存池ID
 *              vbBlk       vb block
 * @param[out]  
 * @return
 */
static INT32 mem_ntv3_vbFree(UINT32 u32Size, VOID *virAddr, UINT32 poolId, UINT64 vbBlk)
{
    INT32  s32Ret = SAL_SOK; 
    HD_COMMON_MEM_VB_BLK vblk = (HD_COMMON_MEM_VB_BLK)vbBlk;

    if (NULL == virAddr)
    {
        SAL_ERROR("intput virAddr is NULL\n");
        return SAL_FAIL;
    }

    s32Ret = hd_common_mem_munmap(virAddr, u32Size);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("plt_common_unmap failure :%#x\n", s32Ret);
        return SAL_FAIL;
    }
    SAL_DEBUG("mem release blk %llx! \n", vbBlk);

    s32Ret = hd_common_mem_release_block(vblk);
    if (s32Ret != SAL_SOK)
    {
        SAL_ERROR("plt_mem_releaseVbBlock fail, rer:%#x\n", s32Ret);
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
    g_stMemPlatOps.mmzAlloc = mem_ntv3_mmzAlloc;
	g_stMemPlatOps.mmzAllocDDR0 = mem_ntv3_mmzAllocDDR0;
    g_stMemPlatOps.mmzFree = mem_ntv3_mmzFree;
    g_stMemPlatOps.vbAlloc = mem_ntv3_vbAlloc;
    g_stMemPlatOps.vbFree = mem_ntv3_vbFree;
    g_stMemPlatOps.vbPoolAlloc = NULL;
    g_stMemPlatOps.vbPoolFree = NULL;

    return &g_stMemPlatOps;
}


