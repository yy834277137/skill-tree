/**
 * @file    sal_elem_buf.c 
 * @note    Hangzhou Hikvision Digital Technology Co.,Ltd. All Right Reserved. 
 * @brief   FIFO Element Buffer 
 */

/*=================== Doubly Linked List ===================*/

/**
DESCRIPTION
This subroutine library supports the creation and maintenance of a
doubly linked pList.  The user supplies a pList descriptor (type DSA_LIST)
that will contain pointers to the first and last nodes in the pList,
and a u32Count of the number of nodes in the pList.  The nodes in the
pList can be any user-defined structure, but they must reserve space
for two pointers as their first elements.  Both the forward and
backward chains are terminated with a NULL pointer.

The linked-pList library simply manipulates the linked-pList data structures;
no kernel functions are invoked.  In particular, linked lists by themselves
provide no task synchronization or mutual exclusion.  If multiple tasks will
access a single linked pList, that pList must be guarded with some
mutual-exclusion mechanism (e.g., a mutual-exclusion semaphore).

NON-EMPTY DSA_LIST:
.CS
   ---------             --------          --------
   | head--------------->| next----------->| next---------
   |       |             |      |          |      |      |
   |       |       ------- prev |<---------- prev |      |
   |       |       |     |      |          |      |      |
   | tail------    |     | ...  |    ----->| ...  |      |
   |       |  |    v                 |                   v
   |count=2|  |  -----               |                 -----
   ---------  |   ---                |                  ---
              |    -                 |                   -
              |                      |
              ------------------------
.CE

EMPTY DSA_LIST:
.CS
    -----------
        |  head------------------
        |         |             |
        |  tail----------       |
        |         |     |       v
        | count=0 |   -----   -----
        -----------    ---     ---
                        -       -
.CE

*/

#include <stdlib.h>
#include <string.h>
#include "sal_trace.h"
#include "dsa_list.h"

#include "sal.h"

typedef struct
{
    DSA_LIST pList;
    UINT32 u32NodeNum;
    DSA_NODE *pNodeBuf;
    UINT32 u32NodeIdle;
}LIST_INNER;


/**
 * @fn      DSA_LstGetHead
 * @brief   peek the first node in a pList
 * 
 * @param   pList[IN] the pList
 * 
 * @return  a pointer to the first node, or NULL if the pList is empty
 */
DSA_NODE *DSA_LstGetHead(DSA_LIST *pList)
{
    return (pList->head);
}


/**
 * @fn      DSA_LstGetTail
 * @brief   peek the last node in a pList
 * 
 * @param   pList[IN] the pList
 * 
 * @return  a pointer to the last node, or NULL if the pList is empty
 */
DSA_NODE *DSA_LstGetTail(DSA_LIST *pList)
{
    return (pList->tail);
}


/**
 * @fn      DSA_LstIsFull
 * @brief   judge the pList is FULL
 * 
 * @param   pList[IN] the pList
 * 
 * @return  TRUE - full, FALSE - not full
 */
BOOL DSA_LstIsFull(DSA_LIST *pList)
{
    if (pList->count == ((LIST_INNER *)pList)->u32NodeNum)
    {
        return SAL_TRUE;
    }
    else
    {
        return SAL_FALSE;
    }
}

/**
 * @fn      DSA_LstIsEmpty
 * @brief   judge the pList is EMPTY
 * 
 * @param   pList[IN] the pList
 * 
 * @return  TRUE - empty, FALSE - not empty
 */
BOOL DSA_LstIsEmpty(DSA_LIST *pList)
{
    if (pList->count == 0)
    {
        return SAL_TRUE;
    }
    else
    {
        return SAL_FALSE;
    }
}


/**
 * @fn      lstCount
 * @brief   get the number of nodes in a pList
 * 
 * @param   pList[IN] the pList
 * 
 * @return  the number of nodes in the pList
 */
UINT32 DSA_LstGetCount(DSA_LIST *pList)
{
    return (pList->count);
}


/**
 * @fn      DSA_LstGetIdleNode
 * @brief   get an idle node in a pList
 * 
 * @param   pList[IN] the pList
 * 
 * @return  a pointer to an idle node, OR NULL if there's non idle node
 */
DSA_NODE *DSA_LstGetIdleNode(DSA_LIST *pList)
{
    UINT32 i = 0;
    LIST_INNER *list_inner = (LIST_INNER *)pList;
    DSA_NODE *node_cur = NULL;

    if (pList->count == list_inner->u32NodeNum)
    {
        return NULL;
    }

    node_cur = list_inner->pNodeBuf + list_inner->u32NodeIdle;
    for (i = list_inner->u32NodeIdle; i < list_inner->u32NodeNum; i++)
    {
        if (NULL == node_cur->next && NULL == node_cur->prev && node_cur != pList->head)
        {
            list_inner->u32NodeIdle = i;
            return node_cur;
        }
        node_cur++;
    }

    node_cur = list_inner->pNodeBuf;
    for (i = 0; i < list_inner->u32NodeIdle; i++)
    {
        if (NULL == node_cur->next && NULL == node_cur->prev && node_cur != pList->head)
        {
            list_inner->u32NodeIdle = i;
            return node_cur;
        }
        node_cur++;
    }

    return NULL;
}


/**
 * @fn      DSA_LstInst
 * @brief   insert a node in a pList after a specified node
 * 
 * @param   pList[IN] the pList
 * @param   pPrev[IN] The new node is placed following the pList node <pPrev>. 
 *                   If <pPrev> is NULL, the node is inserted at the head of the pList. 
 * @param   node[IN] the new node
 */
void DSA_LstInst(DSA_LIST *pList, DSA_NODE *pPrev, DSA_NODE *node)
{
    DSA_NODE *pNext = NULL;

    if (pPrev == NULL)
    {
        pNext = pList->head;
        pList->head = node;
    }
    else
    {
        pNext = pPrev->next;
        pPrev->next = node;
    }

    if (pNext == NULL)
    {
        pList->tail = node;
    }
    else
    {
        pNext->prev = node;
    }

    node->next = pNext;
    node->prev = pPrev;

    pList->count++;

    return;
}


/**
 * @fn      DSA_LstPush
 * @brief   push a node to the end of a pList
 * 
 * @param   pList[IN] the pList
 * @param   node[IN] the new node
 */
void DSA_LstPush(DSA_LIST *pList, DSA_NODE *node)
{
    DSA_LstInst(pList, pList->tail, node);
}


/**
 * @fn      DSA_LstDelete
 * @brief   delete a specified node from a pList
 * 
 * @param   pList[IN] the pList
 * @param   node[IN] the node which needs to be deleted
 */
void DSA_LstDelete(DSA_LIST *pList, DSA_NODE *node)
{
    BOOL b_first_node = SAL_FALSE, b_last_node = SAL_FALSE;

    if (node->prev == NULL)
    {
        if (pList->head == node) // the node should be the head
        {
            b_first_node = SAL_TRUE;
        }
        else // dummy node
        {
            SAL_LOGE("the node[%p] should be equal to the head[%p] of pList\n", node, pList->head);
            return;
        }
    }
    else
    {
        if (node->prev->next != node) // dummy node
        {
            SAL_LOGE("the node[%p] should be equal to the pPrev->next[%p] of pList\n", node, node->prev->next);
            return;
        }
    }

    if (node->next == NULL)
    {
        if (pList->tail == node) // the node should be the pTail
        {
            b_last_node = SAL_TRUE;
        }
        else // dummy node
        {
            SAL_LOGE("the node[%p] should be equal to the pTail[%p] of pList\n", node, pList->tail);
            return;
        }
    }
    else
    {
        if (node->next->prev != node) // dummy node
        {
            SAL_LOGE("the node[%p] should be equal to the pNext->prev[%p] of pList\n", node, node->next->prev);
            return;
        }
    }

    if (b_first_node)
    {
        pList->head = node->next;
    }
    else
    {
        node->prev->next = node->next;
    }

    if (b_last_node)
    {
        pList->tail = node->prev;
    }
    else
    {
        node->next->prev = node->prev;
    }

    pList->count--;
    node->next = NULL;
    node->prev = NULL;

    return;
}


/**
 * @fn      DSA_LstPop
 * @brief   delete the first node from a pList
 * 
 * @param   pList[IN] the pList
 */
void DSA_LstPop(DSA_LIST *pList)
{
    DSA_NODE *node = pList->head;

    if (node != NULL)
    {
        pList->head = node->next;

        if (node->next == NULL) // there's only one node in the pList
        {
            pList->tail = NULL; // the pList is empty now
        }
        else
        {
            node->next->prev = NULL;
        }

        pList->count--;
        node->next = NULL;
        node->prev = NULL;
    }

    return;
}


/**
 * @fn      lst_search
 * @brief   search a node in a pList and return the index of the node
 * @note    the first node's index is 0 
 *  
 * @param   pList[IN] the pList
 * @param   node[IN] the node needs to be searched
 * 
 * @return  The node index(form 0), or -1 if the node is not found
 */
INT32 DSA_LstSearch(DSA_LIST *pList, DSA_NODE *node)
{
    INT32 index = 0;
    DSA_NODE *node_cur = pList->head;

    while (node_cur != NULL)
    {
        if (node_cur != node)
        {
            index++;
            node_cur = node_cur->next;
        }
        else
        {
            break;
        }
    }

    if (node_cur != NULL)
    {
        return index;
    }
    else
    {
        return -1;
    }
}


/**
 * @fn      DSA_LstLocate
 * @brief   locate the index-th(from 0) node in a pList
 * 
 * @param   pList[IN] the pList
 * @param   index[IN] the node which needs to be located
 * 
 * @return  a pointer to the index-th node, or NULL if non-existent
 */
DSA_NODE *DSA_LstLocate(DSA_LIST *pList, INT32 index)
{
    DSA_NODE *node = NULL;

    if (0 == pList->count)
    {
        SAL_LOGE("the pList is empty\n");
        return NULL;
    }

    /* verify node number is in pList */
    if ((index < 0) || (index >= pList->count))
    {
        SAL_LOGE("the index[%d] is invalid, range: [0, %d]\n", index, pList->count-1);
        return NULL;
    }

    /**
     * if index is less than half way, look forward from beginning; 
     * otherwise look back from end 
     */
    if (index < (pList->count >> 1))
    {
        node = pList->head;
        while (index-- > 0)
        {
            node = node->next;
        }
    }
    else
    {
        node = pList->tail;
        index -= (INT32)pList->count;
        while (++index < 0)
        {
            node = node->prev;
        }
    }

    return node;
}


/**
 * @fn      DSA_LstNstep
 * @brief   find a pList node <n_step> steps away from a specified node
 * 
 * @param   node[IN] the node
 * @param   n_step[IN] If <n_step> is positive, it steps toward the pTail
 *                     If <n_step> is negative, it steps toward the head
 * 
 * @return  a pointer to the node <n_step> steps away, or NULL if the node is out of range
 */
DSA_NODE *DSA_LstNstep(DSA_NODE *node, INT32 n_step)
{
    INT32 i = 0;
    INT32 u32Count = (n_step < 0) ? -n_step : n_step;

    for (i = 0; i < u32Count; i++)
    {
        if (n_step < 0)
        {
            node = node->prev;
        }
        else if (n_step > 0)
        {
            node = node->next;
        }

        if (node == NULL)
        {
            break;
        }
    }

    return node;
}


/**
 * @fn      DSA_LstInit
 * @brief   initializes a statistic pList includes <u32NodeNum> nodes
 * 
 * @param   u32NodeNum[IN] the number of nodes in the pList
 * 
 * @return  a pointer to a new pList, OR NULL if init failed
 */
DSA_LIST *DSA_LstInit(UINT32 u32NodeNum)
{
    UINT32 lst_buf_size = 0;
    LIST_INNER *list_inner = NULL;

    /**
     *    LIST_INNER    NODE_BUF
     *    ¡ý             ¡ý              
     *    ©°©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©Ð©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©´
     *    ©¸©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©Ø©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¤©¼
     *                  ©À©¤©¤ u32NodeNum ©¤©¤©È
     */
    lst_buf_size = sizeof(LIST_INNER) + u32NodeNum * sizeof(DSA_NODE);
    list_inner = (LIST_INNER *)SAL_memMalloc(lst_buf_size, "dsa_list", "node");

    if (NULL != list_inner)
    {
        memset(list_inner, 0, lst_buf_size);
        list_inner->u32NodeNum = u32NodeNum;
        list_inner->pNodeBuf = (DSA_NODE *)((U08 *)list_inner + sizeof(LIST_INNER));
        if (SAL_SOK != SAL_CondInit(&list_inner->pList.sync))
        {
            SAL_LOGE("SAL_CondInit for 'pList.sync' failed\n");
            SAL_memfree(list_inner, "dsa_list", "node");
            list_inner = NULL;
        }
    }
    else
    {
        SAL_LOGE("malloc DSA_LIST buffer failed, buffer size: %u\n", lst_buf_size);
    }

    return (DSA_LIST *)list_inner;
}


/**
 * @fn      DSA_LstDeinit
 * @brief   turn all nodes to NULL, and free the pList
 * 
 * @param   pList[IN] the pList
 */
void DSA_LstDeinit(DSA_LIST *pList)
{
    DSA_NODE *p1 = NULL, *p2 = NULL;

    if (NULL != pList)
    {
        if (pList->count > 0)
        {
            p1 = pList->head;
            while (p1 != NULL)
            {
                p2 = p1->next;
                p1->next = NULL;
                p1->prev = NULL;
                p1 = p2;
            }
            pList->count = 0;
            pList->head = pList->tail = NULL;
        }

        SAL_memfree(pList, "dsa_list", "node");
        pList = NULL;
    }

    return;
}

