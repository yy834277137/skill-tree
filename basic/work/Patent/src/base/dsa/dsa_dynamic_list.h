/**
 * @file   dsa_dynamic_list.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  动态列表头文件
 * @author liwenbin
 * @date   2022/6/2
 * @note
 * @note \n History
   1.日    期: 2022/6/2
     作    者: liwenbin
     修改历史: 创建文件
 */
#ifndef __DSA_DYNAMIC_LIST_H__
#define __DSA_DYNAMIC_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sal_type.h"
#include "sal_trace.h"


typedef struct dynode
{
    struct dynode *pPrev;   /*子节点前一个*/
    struct dynode *pNext;   /*子节点下一个*/
    void *pData;                /*当前结点数据*/
} DSA_DY_NODE;

typedef struct
{
    DSA_DY_NODE *pHead;     /*链表头*/
    DSA_DY_NODE *pTail;     /*链表尾*/
    UINT32 u32Length;           /*节点个数*/
    INT32 (*cmp)(DSA_DY_NODE *pNode, void *pData);   /*查找data节点 返回0找到节点 返回其他失败*/
} DSA_DY_LIST;

typedef INT32 (*cmp_t)(DSA_DY_NODE *pNode, void *pData);

/*******************************************************************************
* 函数名  : DSA_DyListInit
* 描  述  :  链表初始化
* 输  入  : -
* 返回值   : 成功:SAL_SOK
*       : 失败:SAL_FAIL
*******************************************************************************/
DSA_DY_LIST *DSA_DyListInit(cmp_t cmp);

/*******************************************************************************
* 函数名  : DSA_DyListDestory
* 描  述  :  销毁链表
* 输  入  : - pList 链表
* 返回值   : 成功:SAL_SOK
*       : 失败:SAL_FAIL
*******************************************************************************/
INT32 DSA_DyListDestory(DSA_DY_LIST *pList);

/*******************************************************************************
* 函数名  : DSA_DyListDeleteNode
* 描  述  :  删除节点
* 输  入  : - pList 链表
         -pNode 节点
* 返回值   : 成功:SAL_SOK
*       : 失败:SAL_FAIL
*******************************************************************************/
INT32 DSA_DyListDeleteNode(DSA_DY_LIST *pList, DSA_DY_NODE *pNode);

/*******************************************************************************
* 函数名  : DSA_DyListPushFront
* 描  述  :  头部插入节点
* 输  入  : - pList 链表
         -pData data
* 返回值   : 成功:SAL_SOK
*       : 失败:SAL_FAIL
*******************************************************************************/
INT32 DSA_DyListPushFront(DSA_DY_LIST *pList, void *pData);

/*******************************************************************************
* 函数名  : DSA_DyListPushBack
* 描  述  :  尾部插入节点
* 输  入  : - pList 链表
         -pData data
* 返回值   : 成功:SAL_SOK
*       : 失败:SAL_FAIL
*******************************************************************************/
INT32 DSA_DyListPushBack(DSA_DY_LIST *pList, void *pData);

/*******************************************************************************
* 函数名  : DSA_DyListInsert
* 描  述  :  在节点后插入节点
* 输  入  : - pList 链表
          -pNode 节点
          -pData 要插入的data
* 返回值   : 成功:SAL_SOK
*       : 失败:SAL_FAIL
*******************************************************************************/
INT32 DSA_DyListInsert(DSA_DY_LIST *pList, DSA_DY_NODE *pNode, void *pData);

/*******************************************************************************
* 函数名  : DSA_DyListFind
* 描  述  :  查找节点
* 输  入  : - pList 链表
         -pData 节点data
* 返回值   : 成功:SAL_SOK
*       : 失败:SAL_FAIL
*******************************************************************************/
DSA_DY_NODE *DSA_DyListFind(DSA_DY_LIST *pList, void *pData);

#ifdef __cplusplus
}
#endif

#endif


