/**
 * @file   dsa_dynamic_list.c
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  动态链表
 * @author dsp
 * @date   2022/6/2
 * @note
 * @note \n History
   1.日    期: 2022/6/2
     作    者: dsp
     修改历史: 创建文件
 */

#include <stdlib.h>
#include "dsa_dynamic_list.h"

/*init 初始化链表*/
DSA_DY_LIST *DSA_DyListInit(cmp_t cmp)
{
    DSA_DY_LIST *pDyList = NULL;

    pDyList = (DSA_DY_LIST *)malloc(sizeof(DSA_DY_LIST));
    if (NULL == pDyList)
    {
        SAL_LOGE("malloc pList pHead failed\n");
        return NULL;
    }

    pDyList->pHead = NULL;
    pDyList->pTail = NULL;
    pDyList->u32Length = 0;
    pDyList->cmp = cmp;

    return pDyList;
}

/*deinit 销毁链表*/
INT32 DSA_DyListDestory(DSA_DY_LIST *pList)
{
    DSA_DY_NODE *pCurNode = NULL;
    DSA_DY_NODE *pTemp = NULL;

    if (pList == NULL)
    {
        SAL_LOGE("HEAD null err\n");
        return SAL_FAIL;
    }

    pCurNode = pList->pHead;
    while (pCurNode != NULL)
    {
        pTemp = pCurNode;
        pCurNode = pCurNode->pNext;

        pTemp->pData = NULL;
        pTemp->pNext = NULL;
        pTemp->pPrev = NULL;
        free(pTemp);
    }

    pList->pHead = NULL;
    pList->pTail = NULL;
    pList->u32Length = 0;

    free(pList);

    return SAL_SOK;
}

/*delete 删除节点*/
INT32 DSA_DyListDeleteNode(DSA_DY_LIST *pList, DSA_DY_NODE *pNode)
{
    DSA_DY_NODE *pCurNode = NULL;

    if (pList == NULL || pNode == NULL)
    {
        SAL_LOGE("null err\n");
        return SAL_FAIL;
    }

    pCurNode = pList->pHead;

    while (pCurNode != NULL && pCurNode != pNode)
    {
        pCurNode = pCurNode->pNext;
    }

    if (pCurNode == NULL)
    {
        SAL_LOGE("null err\n");
        return SAL_FAIL;
    }

    if (pCurNode == pList->pHead)
    {
        pList->pHead = pList->pHead->pNext;
        pList->pHead->pPrev = NULL;
    }
    else if (pCurNode == pList->pTail)
    {
        pList->pTail = pCurNode->pPrev;
        pList->pTail->pNext = NULL;
    }
    else
    {
        pCurNode->pPrev->pNext = pCurNode->pNext;
        pCurNode->pNext->pPrev = pCurNode->pPrev;
    }

    pList->u32Length--;
    if (pList->u32Length <= 0)
    {
        SAL_LOGW("pList has no pData len %d\n", pList->u32Length);
    }

    pCurNode->pNext = NULL;
    pCurNode->pPrev = NULL;
    pCurNode->pData = NULL;

    free(pCurNode);
    pCurNode = NULL;

    return SAL_SOK;
}

/*add pHead pNode*/
INT32 DSA_DyListPushFront(DSA_DY_LIST *pList, void *pData)
{
    DSA_DY_NODE *pNode = NULL;

    if (pList == NULL || pData == NULL)
    {
        SAL_LOGE("null err\n");
        return SAL_FAIL;
    }

    pNode = (DSA_DY_NODE *)malloc(sizeof(DSA_DY_NODE));
    if (pNode == NULL)
    {
        return SAL_FAIL;
    }

    pNode->pData = pData;
    pNode->pPrev = NULL;

    if (pList->u32Length == 0)
    {
        pNode->pNext = NULL;
        pList->pTail = pNode;
    }
    else
    {
        pNode->pNext = pList->pHead;
        pList->pHead->pPrev = pNode;
    }

    pList->pHead = pNode;
    pList->u32Length++;

    return SAL_SOK;
}

/*add pTail pNode*/
INT32 DSA_DyListPushBack(DSA_DY_LIST *pList, void *pData)
{
    DSA_DY_NODE *pNode = NULL;

    if (pList == NULL || pData == NULL)
    {
        SAL_LOGE("null err\n");
        return SAL_FAIL;
    }

    pNode = (DSA_DY_NODE *)malloc(sizeof(DSA_DY_NODE));
    if (pNode == NULL)
    {
        return SAL_FAIL;
    }

    pNode->pData = pData;
    pNode->pNext = NULL;
    pNode->pPrev = NULL;

    if (pList->pHead == NULL)
    {
        pList->pHead = pNode;
        pList->pTail = pNode;
    }
    else
    {
        pNode->pPrev = pList->pTail;
        pList->pTail->pNext = pNode;
        pList->pTail = pNode;
    }

    pList->u32Length++;

    return SAL_SOK;
}

/*insert 插入节点*/
INT32 DSA_DyListInsert(DSA_DY_LIST *pList, DSA_DY_NODE *pPreNode, void *pData)
{
    DSA_DY_NODE *pNode = NULL;
    DSA_DY_NODE *pCurNode = NULL;

    if (pList == NULL || pData == NULL || pPreNode == NULL)
    {
        SAL_LOGE("null err\n");
        return SAL_FAIL;
    }

    pNode = (DSA_DY_NODE *)malloc(sizeof(DSA_DY_NODE));
    if (pNode == NULL)
    {
        return SAL_FAIL;
    }

    pCurNode = pList->pHead;
    while (pCurNode != NULL && pCurNode != pPreNode)
    {
        pCurNode = pCurNode->pNext;
    }

    if (pCurNode == NULL)
    {
        SAL_LOGE("no pNode match\n");
        free(pNode);
        return SAL_FAIL;
    }

    pNode->pData = pData;
    pNode->pPrev = pCurNode;

    if (pCurNode == pList->pTail)
    {
        pList->pTail = pNode;
        pNode->pNext = NULL;
    }
    else
    {
        pCurNode->pNext->pPrev = pNode;
        pNode->pNext = pCurNode->pNext;
    }

    pCurNode->pNext = pNode;

    pList->u32Length++;

    return SAL_SOK;
}

/*findpstNode 获取节点*/
DSA_DY_NODE *DSA_DyListFind(DSA_DY_LIST *pList, void *pData)
{
    DSA_DY_NODE *pTemp = NULL;

    if (pList == NULL || pData == NULL || pList->cmp == NULL)
    {
        SAL_LOGE("null err\n");
        return NULL;
    }

    pTemp = pList->pHead;
    if (pList->cmp != NULL)
    {
        while (pTemp != NULL && 0 != pList->cmp(pTemp, pData))
        {
            pTemp = pTemp->pNext;
        }

        if (pTemp != NULL)
        {
            return pTemp;
        }
    }

    SAL_LOGW("no find match\n");
    return NULL;
}

