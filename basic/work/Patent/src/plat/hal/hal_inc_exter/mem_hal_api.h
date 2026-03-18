/**
 * @file   mem_hal.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   平台相关的内存申请接口，主要是包括申请mmz、vb；并且对申请的内存进行统计。
 * @author  yeyanzhong
 * @date    20210517
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */

#ifndef _MEM_HAL_H_
#define _MEM_HAL_H_


#include "sal.h"

/* 申请vb pool 时的入参及出参 */
typedef struct
{
    UINT32 u32BlkWidth; /* 图像分辨率宽度 */
    UINT32 u32BlkHeight;
    UINT32 u32BlkCnt;
    SAL_VideoDataFormat enSalPixelFmt;
	BOOL   bMapped;	    /* 是否映射虚拟地址，暂时没有使用 */
    UINT32 u32VbPoolID; /* 输出参数 */
    UINT32 u32PoolSize; /* 输出参数 */
} VB_INFO_S;

/* 申请出来的vb的地址及blk等信息，作为输出参数 */
typedef struct
{
    PhysAddr u64PhysAddr;
    Ptr      pVirAddr;
    UINT64   u64VbBlk;
	UINT32   u32PoolId;
    UINT32   u32Size;  /* 实际分配的vb大小，因为实际分配时有对齐的要求，释放时需要传入该大小 */
} ALLOC_VB_INFO_S;

/* 定义申请的内存块对象信息, 为保证效率, 该结构体长度必须为128字节对齐，JPEG编码要求内存地址32字节对齐，xsp GPU算法要求内存地址128对齐 */
typedef struct
{
    UINT32 nMgicNum;                        /* 魔数,用于校验内存块有效性 */
    UINT32 poolId;                          /* 内存池的ID值 */
    PhysAddr physAddr;                      /* 物理地址 */
    PhysAddr privateData;                   /* SDK差异化的私有数据 */
    PhysAddr physAddrBase;                  /* 申请到的基础地址, free时使用 */
    void *virAddrBase;                      /* 申请到的基础地址, free时使用 */
    UINT32 size;                            /* 内存块总的大小, 包括Head和Data */
    UINT32 red[7];
} __attribute__((aligned(128))) MEM_HAL_MEMBLOCK_HEAD_S;


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
INT32 mem_hal_mmzAlloc(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr);

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
 *  注意:  NT 智能私有 ,在 DDR0申请智能内存其他禁用!!!!!
 * @return
 */
INT32 mem_hal_mmzAllocDDR0(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, UINT64 *pRetPhysAddr, VOID **ppRetVirAddr);

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
INT32 mem_hal_cmaMmzAlloc(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, ALLOC_VB_INFO_S *pstVbInfo);

/**
 * @function    mem_hal_mmzFree
 * @brief       对mmz内存进行释放
 * @param[in]   pPtr 虚拟地址
 *              szModName 模块名字，可以把.c源文件名作为模块名，比如disp_task.c，模块名为disp_task
 *              szMemName 内存名字，目前会把相同的内存名字统计在一起，如果上层需要每块内存单独统计，则传入内存名时名字不能相同；
 * @param[out]
 * @return
 */
INT32 mem_hal_mmzFree(Ptr pPtr, CHAR *szModName, CHAR *szMemName);
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
INT32 mem_hal_cmaMmzFree(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk);

INT32 mem_hal_mmzFlushCache(UINT64 vbBlk);

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
INT32 mem_hal_iommuMmzAlloc(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, ALLOC_VB_INFO_S *pstVbInfo);

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
INT32 mem_hal_iommuMmzFree(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk);

/**
 * @function    mem_hal_vbGetVirAddr
 * @brief       通过VB获取虚拟地址
 * @param[in]   vbBlk vb块
 *              ppRetVirAddr 存放获取的虚拟地址
 * @param[out]
 * @return
 */
INT32 mem_hal_vbGetVirAddr(UINT64 vbBlk,VOID **ppRetVirAddr);

/**
 * @function    mem_hal_vbGetVirAddr
 * @brief       vb获取size
 * @param[in]   vbBlk vb块
 *              pSize 内存大小
 * @param[out]
 * @return
 */
INT32 mem_hal_vbGetSize(UINT64 vbBlk,UINT64 *pSize);

/**
 * @function    mem_hal_vbGetVirAddr
 * @brief       VB获取offset
 * @param[in]   vbBlk vb块
 *              u32offset 偏移值
 * @param[out]
 * @return
 */
INT32 mem_hal_vbGetOffset(UINT64 vbBlk,UINT32 *u32offset);

/**
 * @function    mem_hal_vbMap
 * @brief       通过srcVbBlk创建vbBlk，可以访问同一块内存，
 * @param[in]   srcVbBlk 
 *              dstVbBlk 
 * @param[out]
 * @return
 */
INT32 mem_hal_vbMap(UINT64 srcVbBlk,UINT64 *dstVbBlk);

/**
 * @function    mem_hal_vbGetVirAddr
 * @brief       销毁vb
 * @param[in]   vbBlk vb块
 * @param[out]
 * @return
 */
INT32 mem_hal_vbDelete(UINT64 vbBlk);




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
INT32 mem_hal_vbAlloc(UINT32 u32Size, CHAR *szModName, CHAR *szMemName, CHAR *szZone, BOOL bCache, ALLOC_VB_INFO_S *pstVbInfo);

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
INT32 mem_hal_vbFree(Ptr pPtr, CHAR *szModName, CHAR *szMemName, UINT32 u32Size, UINT64 u64VbBlk, UINT32 u32PoolId);


INT32 mem_hal_vbPoolAlloc(VB_INFO_S *pstVbInfo, CHAR *szModName, CHAR *szMemName, CHAR *aszZone);
INT32 mem_hal_vbPoolFree(VB_INFO_S *pstVbInfo, CHAR *szModName, CHAR *szMemName);

/**
 * @function    mem_hal_init
 * @brief       初始化mem功能函数
 * @param[in]   void
 * @param[out]  无
 * @return
 */
INT32 mem_hal_init(void);



#endif

