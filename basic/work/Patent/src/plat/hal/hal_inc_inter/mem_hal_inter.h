/**
 * @file   mem_hal_inter.h
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief
 * @author
 * @date
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */
#ifndef _MEM_HAL_INTER_H_

#define _MEM_HAL_INTER_H_

#include "sal.h"

//#include "platform_sdk.h"
#include "platform_hal.h"





typedef struct
{
    INT32 (*mmzAlloc)(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 allocSize, UINT64 *retPhysAddr, VOID **retVirtAddr, UINT64 *pVbBlk);
    INT32 (*mmzAllocDDR0)(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 allocSize, UINT64 *retPhysAddr, VOID **retVirtAddr, UINT64 *pVbBlk);
    INT32 (*cmaMmzAlloc)(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 allocSize, UINT64 *retPhysAddr, VOID **retVirtAddr, UINT64 *pVbBlk);
    INT32 (*iommuMmzAlloc)(BOOL bCached, CHAR *pcMmb, CHAR *pcZone, UINT32 allocSize, UINT64 *retPhysAddr, VOID **retVirtAddr, UINT64 *pVbBlk);
    INT32 (*mmzFree)(UINT32 allocSize, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk);
    INT32 (*cmaMmzFree)(UINT32 allocSize, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk);
    INT32 (*iommuMmzFree)(UINT32 allocSize, UINT64 U64PhysAddr, VOID *pVirAddr, UINT64 vbBlk);
    INT32 (*mmzFlushCache)(UINT64 vbBlk, BOOL bReadOnly);
    INT32 (*vbAlloc)(UINT32 allocSize, CHAR *pcZone, BOOL bCached, UINT64 *retPhysAddr, VOID **retVirtAddr, UINT32 *pPoolId, UINT64 *pVbBlk);
    INT32 (*vbFree)(UINT32 allocSize, VOID *virAddr, UINT32 poolId, UINT64 vbBlk);
    INT32 (*vbPoolAlloc)(VB_INFO_S *pstVbInfo, CHAR *aszZone);
    INT32 (*vbPoolFree)(VB_INFO_S *pstVbInfo);
    INT32 (*vbGetVirAddr)(UINT64 pVbBlk,VOID **retVirtAddr);
    INT32 (*vbGetSize)(UINT64 vbBlk,UINT64 *u64Size);
    INT32 (*vbGetOffset)(UINT64 vbBlk,UINT32 *u32Offset);
    INT32 (*vbMap)(UINT64 srcVbBlk,UINT64 *dstVbBlk);
    INT32 (*vbDelete)(UINT64 vbBlk);
} MEM_PLAT_OPS_S;

/**
 * @function    mem_hal_register
 * @brief       对平台相关的内存申请函数进行注册
 * @param[in]
 * @param[out]
 * @return
 */
const MEM_PLAT_OPS_S *mem_hal_register(void);


#endif /* _MEM_HAL_INTER_H_ */


