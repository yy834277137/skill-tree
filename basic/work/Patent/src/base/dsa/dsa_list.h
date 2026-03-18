/**
 * @file   dsa_list.h
 * @note   2020-2030, 杭州海康威视数字技术股份有限公司
 * @brief  列表操作头文件
 * @author dsp
 * @date   2022/6/2
 * @note
 * @note \n History
   1.日    期: 2022/6/2
     作    者: dsp
     修改历史: 创建文件
 */
#ifndef __SAL_ELEM_BUF_H__
#define __SAL_ELEM_BUF_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "sal_cond.h"
#include "sal_type.h"

/*=================== Doubly Linked List ===================*/
typedef struct node
{
    struct node *next;  /* pointer to the next node in the list */
    struct node *prev;  /* pointer to the previous node in the list */
    void *pAdData;      /* additional data, always be a struct */
} DSA_NODE;

typedef struct
{
    struct node *head;  /* pointer to the head node in the list */
    struct node *tail;  /* pointer to the tail node in the list */
    uint32_t count;     /* number of nodes in the list */
    COND_T sync;        /* the mutex and condition that control the list */
} DSA_LIST;


typedef struct
{
    uint8_t *p1;      /* 单个element对应的buffer地址 */
    uint32_t len1;    /* 单个element对应的buffer数据长度 */
    uint8_t *p2;      /* 若此element有回头写buffer，则对应第二块buffer的地址，即element buffer的首地址，否则为NULL */
    uint32_t len2;    /* 若此element有回头写buffer，则对应第二块buffer的数据长度，否则为0 */
} DSA_SEG_INFO;


/**
 * 注：以下接口，除lst_init外，均需在调用前加互斥锁，并在调用完成后释放。
 * 若连续调用多个接口，则可一起加锁，所有接口调用完成后一起释放。
 */

/**
 * @fn      DSA_LstGetHead
 * @brief   peek the first node in a pList
 *
 * @param   pList[IN] the pList
 *
 * @return  a pointer to the first node, or NULL if the pList is empty
 */
DSA_NODE *DSA_LstGetHead(DSA_LIST *pList);

/**
 * @fn      DSA_LstGetTail
 * @brief   peek the last node in a pList
 *
 * @param   pList[IN] the pList
 *
 * @return  a pointer to the last node, or NULL if the pList is empty
 */
DSA_NODE *DSA_LstGetTail(DSA_LIST *pList);

/**
 * @fn      DSA_LstIsFull
 * @brief   judge the pList is FULL
 *
 * @param   pList[IN] the pList
 *
 * @return  TRUE - full, FALSE - not full
 */
BOOL DSA_LstIsFull(DSA_LIST *pList);

/**
 * @fn      DSA_LstIsEmpty
 * @brief   judge the pList is EMPTY
 *
 * @param   pList[IN] the pList
 *
 * @return  TRUE - empty, FALSE - not empty
 */
BOOL DSA_LstIsEmpty(DSA_LIST *pList);

/**
 * @fn      lstCount
 * @brief   get the number of nodes in a pList
 *
 * @param   pList[IN] the pList
 *
 * @return  the number of nodes in the pList
 */
UINT32 DSA_LstGetCount(DSA_LIST *pList);

/**
 * @fn      DSA_LstGetIdleNode
 * @brief   get an idle node in a pList
 *
 * @param   pList[IN] the pList
 *
 * @return  a pointer to an idle node, OR NULL if there's non idle node
 */
DSA_NODE *DSA_LstGetIdleNode(DSA_LIST *pList);

/**
 * @fn      DSA_LstInst
 * @brief   insert a node in a pList after a specified node
 *
 * @param   pList[IN] the pList
 * @param   pPrev[IN] The new node is placed following the pList node <pPrev>.
 *                   If <pPrev> is NULL, the node is inserted at the pHead of the pList.
 * @param   node[IN] the new node
 */
void DSA_LstInst(DSA_LIST *pList, DSA_NODE *pPrev, DSA_NODE *node);

/**
 * @fn      DSA_LstPush
 * @brief   push a node to the end of a pList
 *
 * @param   pList[IN] the pList
 * @param   node[IN] the new node
 */
void DSA_LstPush(DSA_LIST *pList, DSA_NODE *node);

/**
 * @fn      DSA_LstDelete
 * @brief   delete a specified node from a pList
 *
 * @param   pList[IN] the pList
 * @param   node[IN] the node which needs to be deleted
 */
void DSA_LstDelete(DSA_LIST *pList, DSA_NODE *node);

/**
 * @fn      DSA_LstPop
 * @brief   delete the first node from a pList
 *
 * @param   pList[IN] the pList
 */
void DSA_LstPop(DSA_LIST *pList);

/**
 * @fn      DSA_LstSearch
 * @brief   search a node in a pList and return the index of the node
 * @note    the first node's index is 0
 *
 * @param   pList[IN] the pList
 * @param   node[IN] the node needs to be searched
 *
 * @return  The node index(form 0), or -1 if the node is not found
 */
INT32 DSA_LstSearch(DSA_LIST *pList, DSA_NODE *node);

/**
 * @fn      DSA_LstLocate
 * @brief   locate the index-th(from 0) node in a pList
 *
 * @param   pList[IN] the pList
 * @param   index[IN] the node which needs to be located
 *
 * @return  a pointer to the index-th node, or NULL if non-existent
 */
DSA_NODE *DSA_LstLocate(DSA_LIST *pList, INT32 index);

/**
 * @fn      DSA_LstNstep
 * @brief   find a pList node <n_step> steps away from a specified node
 *
 * @param   node[IN] the node
 * @param   n_step[IN] If <n_step> is positive, it steps toward the pTail
 *                     If <n_step> is negative, it steps toward the pHead
 *
 * @return  a pointer to the node <n_step> steps away, or NULL if the node is out of range
 */
DSA_NODE *DSA_LstNstep(DSA_NODE *node, INT32 n_step);

/**
 * @fn      DSA_LstInit
 * @brief   initializes a statistic pList includes <node_num> nodes
 *
 * @param   node_num[IN] the number of nodes in the pList
 *
 * @return  a pointer to a new pList, OR NULL if init failed
 */
DSA_LIST *DSA_LstInit(UINT32 node_num);

/**
 * @fn      DSA_LstDeinit
 * @brief   turn all nodes to NULL, and free the pList
 *
 * @param   pList[IN] the pList
 */
void DSA_LstDeinit(DSA_LIST *pList);

#ifdef __cplusplus
}
#endif

#endif /* __SAL_ELEM_BUF_H__ */

