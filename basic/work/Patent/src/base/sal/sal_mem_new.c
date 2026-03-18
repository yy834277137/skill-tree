/**
 * @file   sal_mem_stats.c
 * @note   2010-2020, Hikvision Digital Technology Co., Ltd.
 * @brief   OS内存申请模块，内存申请及释放时会进行统计功能。
 * @author  yeyanzhong
 * @date    2021.5.20
 * @note
 * @note \n History
   1.Date        :
     Author      :
     Modification:
 */

 
/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */
#include <malloc.h>

#include "sal.h"
#include "sal_mem_stats.h"



/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
/* 内存数据段指针到头指针的转换 */
#define SAL_memDataToHead(pData) ((SAL_MemBlockObject *)( ((unsigned long)(pData)) - sizeof(SAL_MemBlockObject)))

/* 内存的魔数, 用于有效性校验 */
#define SAL_MEM_MAGIC 0x12345678UL

/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* 定义申请的内存块对象信息, 为保证效率, 该结构体长度必须为32字节整数倍 */
typedef struct
{
    UINT32 nMgicNum;                        /* 魔数,用于校验内存块有效性。 */
    UINT32 size;                            /* 内存块总的大小, 包括Head和Data */
    void *pBufBase;                         /* 申请到的基础地址, free时使用 */
} __attribute__((aligned(128))) SAL_MemBlockObject;

/* ========================================================================== */
/*                            全局变量定义区                                  */
/* ========================================================================== */

/* 用于OS内存统计的全局结构体 */
static MOD_MEM_STATS_S stOsModMemStats =
{
    .mutex = PTHREAD_MUTEX_INITIALIZER,
};


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */


/*******************************************************************************
* 函数名  : mem_uAllocAlign
* 描  述  : 该函数负责在linux用户态中申请内存
*
* 输  入  : - size:  要申请的内存大小
*           - align: 用户自定义对齐的字节数, 若为0表示不进行自定义对齐
*                    该参数必须为4的整数倍, 否则函数将返回NULL
* 输  出  : 无。
* 返回值  : 非NULL: 申请成功
*           NULL:   申请失败
*******************************************************************************/
static Ptr mem_uAllocAlign(UINT32 size, UINT32 align, CHAR *szModName, CHAR *szMemName)
{
    UINT32 allocSize = 0;
    UINT32 headSize = 0;
    Ptr pBufBase = NULL;
    Ptr pBufData = NULL;
    SAL_MemBlockObject *pBufHead = NULL;

    if(0 == size)
    {
        SAL_LOGE("size must > 0\n");
        return NULL;
    }
    if (!szModName || !szMemName )
    {
        SAL_ERROR("input param is null, szModName %s, szMemName %s \n", szModName, szMemName);
        return NULL;
    }


    /* 检查align 必须为4的倍数 */
    if(0 != (align & 3))
    {
        SAL_LOGE("invalid align:%d\n", align);
        return NULL;
    }

    /* 计算头大小 */
    headSize = sizeof(SAL_MemBlockObject);

    /* 总的要申请的内存大小 */
    allocSize = size + align + headSize;

    /* 申请内存 */
    pBufBase = malloc(allocSize);

    if(NULL == pBufBase)
    {
        SAL_LOGE("alloc failed, size:%d, align:%d\n", size, align);
        return NULL;
    }

    /* 计算数据段的位置，内存的前面存放内存使用的类型，后面才是数据区 */
    pBufData = (void *)((unsigned long)pBufBase + headSize);
    if(0 != align)
    {
        pBufData = (Ptr)SAL_align((unsigned long)pBufData, align);
    }

    /* 获取头的位置 */
    pBufHead = SAL_memDataToHead(pBufData);

    /* 填充头 */
    pBufHead->nMgicNum  = SAL_MEM_MAGIC;
    pBufHead->size      = allocSize;
    pBufHead->pBufBase  = pBufBase;

    SAL_memAllocStats(&stOsModMemStats, szModName, szMemName, allocSize);
    
    /*返回数据段指针*/
    return pBufData;
}

/*******************************************************************************
* 函数名  : SAL_memPageSize
* 描  述  : 获取系统页大大小
* 输  入  : - void:
* 输  出  : 无
* 返回值  : 页大小
*******************************************************************************/
UINT32 SAL_memPageSize(void)
{
    return (UINT32)getpagesize();
}


/*******************************************************************************
* Function      : SAL_memCpy
* Description   : 封装 memcpy 函数，进行四字节对齐式的快速拷贝
* Input         : - dest  : 目标地址
*               : - src   : 源
*               : - nBytes: 长度
* Output        : NONE
* Return        : HIK_SOK  : Success
*                 HIK_FAIL : Fail
*******************************************************************************/
void SAL_memCpy(INT8 *dest, INT8 *src, UINT32 nBytes)
{
    PhysAddr desRes, srcRes;

    desRes = (PhysAddr)dest & 0x3;
    srcRes = (PhysAddr)src & 0x3;
    if (srcRes == 0 && desRes != 0)
    {
        memcpy(dest, src, nBytes);
    }
    else if (srcRes != 0 && desRes == 0)
    {
        memcpy(dest, src, nBytes);
    }
    else if ((srcRes != 0 && desRes != 0) && (srcRes == desRes))
    {
        while (srcRes > 0)
        {
            *dest++ = *src++;
            srcRes--;
        }
    }
    else if ((srcRes != 0 && desRes != 0) && (srcRes != desRes))
    {
        memcpy(dest, src, nBytes);
    }

    if ((((PhysAddr)dest & 0x3) == 0) && (((PhysAddr)src & 0x3) == 0))
    {

        while (nBytes >= 4)
        {
            *((UINT32 *)dest) = *((UINT32 *)src);
            dest += 4;
            src += 4;
            nBytes -= 4;
        }
    }

    while (nBytes > 0)
    {
        *dest++ = *src++;
        nBytes--;
    }
}


/*******************************************************************************
* 函数名  : SAL_memAlloc
* 描  述  : 软件抽象层统一的内存申请函数
* 输  入  : - size: 内存大小
*           - szModName 模块名字，不能为空，必须输入
*           - szMemName 内存名字，不能为空，必须输入
* 输  出  : 无
* 返回值  : HIK_SOK  : 成功
*           HIK_FAIL : 失败
*******************************************************************************/
Ptr SAL_memMalloc(UINT32 size, CHAR *szModName, CHAR *szMemName)
{
    return mem_uAllocAlign(size, 0, szModName, szMemName);
}

/*******************************************************************************
* 函数名  : SAL_memZalloc
* 描  述  : 该函数在mem_uAllocAlign的基础上,增加对内存清0的动作
* 输  入  : - size:  要申请的内存大小
* 输  出  : 无。
* 返回值  : 非NULL:  申请成功
*           NULL:    申请失败
*******************************************************************************/
Ptr SAL_memZalloc(UINT32 size, CHAR *szModName, CHAR *szMemName)
{
    Ptr pPtr = mem_uAllocAlign(size, 0, szModName, szMemName);
    if(pPtr)
    {
        SAL_clearSize(pPtr, size);
    }
    return pPtr;
}

/*******************************************************************************
* 函数名  : SAL_memFree
* 描  述  : 系统抽象层下的内存申请封装函数，负责负责释放内存并统计更新当前系统总计使用的内存
* 输  入  : - pPtr:    要释放的内存地址
* 输  出  : 无
* 返回值  : SAL_SOK:   成功,内存已释放
*           SAL_FAIL: 失败, 内存未释放
*******************************************************************************/
INT32 SAL_memfree(Ptr pPtr, CHAR *szModName, CHAR *szMemName)
{
    SAL_MemBlockObject *pBufHead = NULL;

    if(NULL == pPtr)
    {
        return SAL_FAIL;
    }
	
    if (!szModName || !szMemName )
    {
        SAL_ERROR("input param is null, szModName %s, szMemName %s \n", szModName, szMemName);
        return SAL_FAIL;
    }
    
    pBufHead = SAL_memDataToHead(pPtr);

    /* 校验魔数 */
    if(SAL_MEM_MAGIC != pBufHead->nMgicNum)
    {
        SAL_LOGE("invalid magic:0x%x\n", pBufHead->nMgicNum);
        return SAL_FAIL;
    }

    /* 释放内存块时把魔数置为-1 */
    pBufHead->nMgicNum = (UINT32) - 1;

    SAL_memFreeStats(&stOsModMemStats, szModName, szMemName, pBufHead->size);

    /* 释放内存 */
    if (pBufHead->pBufBase != NULL)
    {
        free(pBufHead->pBufBase);
        pPtr = NULL;
    }

    return SAL_SOK;
}

/*******************************************************************************
* 函数名  : SAL_memAlign
* 描  述  : 系统抽象层下的内存申请封装函数，负责申请内存并统计下来，能够保证申请的内存有字节对齐的效果
* 输  入  : - alignSize: 强制字节对齐的长度
*         : - size     : 申请大小
* 输  出  : 无
* 返回值  : ptr  : 申请成功的内存地址
*           NULL : 申请失败
*******************************************************************************/
void *SAL_memAlign(UINT32 alignSize, UINT32 size, CHAR *szModName, CHAR *szMemName)
{
    UINT32 allocSize = 0;
    UINT32 headSize = 0;
    Ptr pBufBase = NULL;
    Ptr pBufData = NULL;
    SAL_MemBlockObject *pBufHead = NULL;

    if (0 == size)
    {
        SAL_LOGE("size must > 0\n");
        return NULL;
    }

    if (!szModName || !szMemName)
    {
        SAL_ERROR("input param is null, szModName %s, szMemName %s \n", szModName, szMemName);
        return NULL;
    }

    /* 检查align 必须为4的倍数 */
    if (0 != (alignSize & 3))
    {
        SAL_LOGE("invalid align:%d\n", alignSize);
        return NULL;
    }

    /* 计算头大小 */
    headSize = sizeof(SAL_MemBlockObject);
    if (headSize & (alignSize - 1))
    {
        SAL_LOGE("invalid headSize %#x align:%#x\n", headSize, alignSize);
        return NULL;
    }

    /* 总的要申请的内存大小 */
    allocSize = size + alignSize + headSize;
    pBufBase = (Ptr)memalign(alignSize, allocSize);
    if (NULL != pBufBase)
    {
        /* 计算数据段的位置，内存的前面存放内存使用的类型，后面才是数据区 */
        pBufData = (void *)((unsigned long)pBufBase + headSize);

        /* 获取头的位置 */
        pBufHead = SAL_memDataToHead(pBufData);

        /* 填充头 */
        pBufHead->nMgicNum = SAL_MEM_MAGIC;
        pBufHead->size = allocSize;
        pBufHead->pBufBase = pBufBase;

        SAL_memAllocStats(&stOsModMemStats, szModName, szMemName, allocSize);
        /*返回数据段指针*/
        return pBufData;
    }
    else
    {
        SAL_LOGE("malloc failed size=%d, vAddr=%p, \n", size, pBufBase);
        return NULL;
    }
}

/*******************************************************************************
* 函数名  : SAL_memArray2D
* 描  述  : 给二维数组申请内存
* 输  入  : - row: 行
*         : - col: 列
* 输  出  : 无
* 返回值  : 二维数组内存
*******************************************************************************/
UINT32** SAL_memArray2D(UINT32 row, UINT32 col, CHAR *szModName, CHAR *szMemName)
{
    UINT32 size = sizeof(UINT32);
    UINT32 point_size = sizeof(UINT32*);

    /* 先申请内存，其中point_size * row表示存放row个行指针 */
    UINT32 **arr = (UINT32 **) SAL_memMalloc(point_size * row + size * row * col, szModName, szMemName);
    if (arr != NULL)
    {
        memset(arr, 0, point_size * row + size * row * col);
        UINT32 *head = (UINT32*)((PhysAddr)arr + point_size * row);
        while (row--)
            arr[row] = (UINT32*)((PhysAddr)head + row * col * size);
    }
    return (UINT32**)arr;
}

/**
 * @function  SAL_showMem_Alloc
 * @brief      统计ALLOC模块申请内存函数
 * @param[in]   void
 * @param[out]  无
 * @return
 */
void SAL_showMem_Alloc(void)
{
    UINT32 i = 0;
    MOD_MEM_S *pstModMemInfo = NULL;
    UINT32 j = 0;
    UINT64 u64OsMemTotal = 0;

    for (i = 0; i < MOD_NUM; i++)
    {
        pstModMemInfo = &stOsModMemStats.stModMemInfo[i];
        if(pstModMemInfo->u32IsUsed == 1)
        {
            u64OsMemTotal += pstModMemInfo->u32ModMemLen;
        }
    }
        
    SAL_print("\n--------------------------------MEM_OS_Alloc(total:%lld)------------------------------------------\n", u64OsMemTotal);
    for (i = 0; i < MOD_NUM; i++)
    {
        pstModMemInfo = &stOsModMemStats.stModMemInfo[i];
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




