
/*******************************************************************************
 * sal_empty_full_que.c
 *
 * HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
 *
 * Author : cuifeng5
 * Version: V1.0.0  2020年05月23日 Create
 *
 * Description : 
 * Modification: 
 *******************************************************************************/

#include "sal.h"
#include "dsa_fixed_size_que.h"

/*******************************************************************************
* 函数名  : SAL_EmptyFullQueCreate
* 描  述  : 队列初始化
* 输  入  : SAL_EMPTY_FULL_QUE_S *pstQue :初始化队列
        : UINT32 u32ElenmentSize :每个元素的大小
        : UINT32 u32ElenmentNum :元素个数
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
INT32 DSA_FixedSizeQueCreate(DSA_FIXED_SIZE_QUE_S *pstQue, UINT32 u32ElenmentSize, UINT32 u32ElenmentNum)
{
    INT32 s32Ret = SAL_SOK;
    UINT8 *pu8Buff = NULL;
    UINT32 i = 0;

    if (NULL == pstQue)
    {
        SAL_LOGE("input que is null\n");
        return SAL_FAIL;
    }
    
    pu8Buff = (UINT8 *)SAL_memZalloc(u32ElenmentSize * u32ElenmentNum, "dsa_fixed_que", "que_ele");
    if (NULL == pu8Buff)
    {
        SAL_LOGE("alloc empty full que buff buff fail\n");
        return SAL_FAIL;
    }
    pstQue->pvQue = (VOID *)pu8Buff;

    s32Ret = DSA_QueCreate(&pstQue->empty, u32ElenmentNum);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("create empty que fail:0x%x\n", s32Ret);
        goto out2;
    }

    s32Ret = DSA_QueCreate(&pstQue->full, u32ElenmentNum);
    if (s32Ret != SAL_SOK)
    {
        SAL_LOGE("create full que fail:0x%x\n", s32Ret);
        goto out1;
    }

    for (i = 0; i < u32ElenmentNum; i++)
    {
        DSA_QuePut(&pstQue->empty, pu8Buff, SAL_TIMEOUT_NONE);
        pu8Buff += u32ElenmentSize;
    }

    return SAL_SOK;

out1:
    DSA_QueDelete(&pstQue->empty);
out2:
    SAL_memfree((Ptr)pu8Buff, "dsa_fixed_que", "que_ele");

    return SAL_FAIL;
}


/*******************************************************************************
* 函数名  : SAL_EmptyFullQueDelete
* 描  述  : 删除队列
* 输  入  : SAL_EMPTY_FULL_QUE_S *pstQue : 队列
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
VOID DSA_FixedSizeQueDelete(DSA_FIXED_SIZE_QUE_S *pstQue)
{
    if (NULL != pstQue->pvQue)
    {
        SAL_memfree(pstQue->pvQue, "dsa_fixed_que", "que_ele");
        pstQue->pvQue = NULL;
    }

    DSA_QueDelete(&pstQue->empty);
    DSA_QueDelete(&pstQue->full);

    return;
}


/*******************************************************************************
* 函数名  : SAL_QueGetEmpty
* 描  述  : 从空队列取数据
* 输  入  : SAL_EMPTY_FULL_QUE_S *pstQue : 队列指针
        : VOID **ppvData : 存放地址的指针
        : UINT32 u32Timeout : 超时时间
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
inline INT32 DSA_FixedSizeQueGetEmpty(DSA_FIXED_SIZE_QUE_S *pstQue, VOID **ppvData, UINT32 u32Timeout)
{
    return DSA_QueGet(&pstQue->empty, ppvData, u32Timeout);
}


/*******************************************************************************
* 函数名  : SAL_QuePutEmpty
* 描  述  : 往空队列存数据
* 输  入  : SAL_EMPTY_FULL_QUE_S *pstQue : 队列指针
        : VOID *pvData : 存放地址
        : UINT32 u32Timeout : 超时时间
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
inline INT32 DSA_FixedSizeQuePutEmpty(DSA_FIXED_SIZE_QUE_S *pstQue, VOID *pvData, UINT32 u32Timeout)
{
    return DSA_QuePut(&pstQue->empty, pvData, u32Timeout);
}


/*******************************************************************************
* 函数名  : SAL_QueGetFull
* 描  述  : 从满队列取数据
* 输  入  : SAL_EMPTY_FULL_QUE_S *pstQue : 队列指针
        : VOID **ppvData : 存放地址的指针
        : UINT32 u32Timeout : 超时时间
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
inline INT32 DSA_FixedSizeQueGetFull(DSA_FIXED_SIZE_QUE_S *pstQue, VOID **ppvData, UINT32 u32Timeout)
{
    return DSA_QueGet(&pstQue->full, ppvData, u32Timeout);
}


/*******************************************************************************
* 函数名  : SAL_QuePutFull
* 描  述  : 往满队列存数据
* 输  入  : SAL_EMPTY_FULL_QUE_S *pstQue : 队列指针
        : VOID *pvData : 存放地址
        : UINT32 u32Timeout : 超时时间
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
inline INT32 DSA_FixedSizeQuePutFull(DSA_FIXED_SIZE_QUE_S *pstQue, VOID *pvData, UINT32 u32Timeout)
{
    return DSA_QuePut(&pstQue->full, pvData, u32Timeout);
}


/*******************************************************************************
* 函数名  : SAL_QuePeekFull
* 描  述  : 读满队列
* 输  入  : SAL_EMPTY_FULL_QUE_S *pstQue : 队列指针
        : VOID **ppvData : 存放地址的指针
* 输  出  : 无
* 返回值  : 0        : 成功
*         -1      : 失败
*******************************************************************************/
inline INT32 DSA_FixedSizeQuePeekFull(DSA_FIXED_SIZE_QUE_S *pstQue, VOID **ppvData)
{
    return DSA_QuePeek(&pstQue->full, ppvData);
}


