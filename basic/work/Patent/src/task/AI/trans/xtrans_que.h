

#ifndef _XTRANS_QUE_H_
#define _XTRANS_QUE_H_


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

typedef HPR_HANDLE xtrans_QueHandle;


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/*  */
typedef struct
{
    HPR_UINT16 maxElems;    /* 队列成员数量。*/

    HPR_UINT16 flags;       /* 标志，目前未使用。*/

    HPR_UINT32 *pQueue;     /* xtrans_que队列可传入, xtrans_queList无效。*/

    HPR_UINT32 reserved[4]; /* 保留 */
} xtrans_QueCreate;


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : xtrans_queCreate
* 描  述  : 创建队列。
* 输  入  : - pCreate: 创建参数。
*         : - phQue  : 返回的队列句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queCreate(xtrans_QueCreate *pCreate,
                    xtrans_QueHandle *phQue);


/*******************************************************************************
* 函数名  : xtrans_queDelete
* 描  述  : 销毁队列。
* 输  入  : - hQue: 队列句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queDelete(xtrans_QueHandle hQue);


/*******************************************************************************
* 函数名  : xtrans_quePut
* 描  述  : 写队列。
* 输  入  : - hQue   : 队列句柄。
*           - value  : 队列成员。
*           - timeout: 超时时间。单位是毫秒。xtrans_TIMEOUT_NONE表示不等待，
*                      xtrans_TIMEOUT_FOREVER表示无限等待。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_quePut(xtrans_QueHandle hQue,
                 HPR_UINT32        value,
                 HPR_UINT32        timeout);

/*******************************************************************************
* 函数名  : xtrans_quePut
* 描  述  : 读队列。
* 输  入  : - hQue   : 队列句柄。
*           - timeout: 超时时间。单位是毫秒。xtrans_TIMEOUT_NONE表示不等待，
*                      xtrans_TIMEOUT_FOREVER表示无限等待。
* 输  出  : - pVaule : 队列成员指针。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queGet(xtrans_QueHandle hQue,
                 HPR_UINT32       *pVaule,
                 HPR_UINT32        timeout);


/*******************************************************************************
* 函数名  : xtrans_queReset
* 描  述  : 复位队列。仅对xtrans_que类型队列有效。
* 输  入  : - hQue: 队列句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queReset(xtrans_QueHandle hQue);


/*******************************************************************************
* 函数名  : xtrans_queslPut
* 描  述  : 轻量级的队列写。没有互斥保护，只能用于单线程无抢占的场合。
* 输  入  : - hQue   : 队列句柄。
*           - value  : 队列成员。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queslPut(xtrans_QueHandle hQue, HPR_UINT32  value);


/*******************************************************************************
* 函数名  : xtrans_queslGet
* 描  述  : 轻量级的队列读。没有互斥保护，只能用于单线程无抢占的场合。
* 输  入  : - hQue   : 队列句柄。
*           - value  : 队列成员。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queslGet(xtrans_QueHandle hQue, HPR_UINT32 *pValue);


/*******************************************************************************
* 函数名  : xtrans_queListCreate
* 描  述  : 创建链表队列。使用链表管理队列。
* 输  入  : - pCreate: 创建参数。
*         : - phQue  : 返回的队列句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queListCreate(xtrans_QueCreate *pCreate,
                        xtrans_QueHandle *phQue);


/*******************************************************************************
* 函数名  : xtrans_queListDelete
* 描  述  : 销毁链表队列。
* 输  入  : - hQue: 队列句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queListDelete(xtrans_QueHandle hQue);


/*******************************************************************************
* 函数名  : xtrans_queListRemove
* 描  述  : 链表队列写。
* 输  入  : - hQue    : 队列句柄。
*           - pQueElem: 队列成员的链表头指针。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queListRemove(xtrans_QueHandle hQue,
                        xtrans_ListHead *pQueElem);


/*******************************************************************************
* 函数名  : xtrans_queListNext
* 描  述  : 获取链表中的指定节点的后一个节点，队列元素在队列中维持原装，不被删除
* 输  入  : - hQue      : 队列句柄。
*           - pQueElem  : 指定队列中的节点，如果指定为xtrans_NULL，则返回第一个元素
* 输  出  : 无
* 返回值  : xtrans_NULL表示当前指定的元素已经是最后一个
*           非空表示队列元素pQueElem的后一个元素
*******************************************************************************/
xtrans_ListHead* xtrans_queListNext(xtrans_QueHandle  hQue,
                              xtrans_ListHead  *pQueElem);


/*******************************************************************************
* 函数名  : xtrans_queListPrev
* 描  述  : 获取链表中的指定节点的前一个节点，队列元素在队列中维持原装，不被删除
* 输  入  : - hQue      : 队列句柄。
*           - pQueElem  : 指定队列中的节点，如果指定为xtrans_NULL，则返回尾部元素
* 输  出  : 无
* 返回值  : xtrans_NULL表示当前指定的元素已经是第一个
*           非空表示队列元素pQueElem的前一个元素
*******************************************************************************/
xtrans_ListHead* xtrans_queListPrev(xtrans_QueHandle  hQue,
                              xtrans_ListHead  *pQueElem);


/*******************************************************************************
* 函数名  : xtrans_queListPut
* 描  述  : 链表队列写。
* 输  入  : - hQue    : 队列句柄。
*           - pQueElem: 队列成员的链表头指针。
*           - timeout : 超时时间。单位是毫秒。xtrans_TIMEOUT_NONE表示不等待，
*                       xtrans_TIMEOUT_FOREVER表示无限等待。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queListPut(xtrans_QueHandle hQue,
                     xtrans_ListHead *pQueElem,
                     HPR_UINT32        timeout);


/*******************************************************************************
* 函数名  : xtrans_queListGet
* 描  述  : 链表队列读。
* 输  入  : - hQue   : 队列句柄。
*           - timeout: 超时时间。单位是毫秒。xtrans_TIMEOUT_NONE表示不等待，
*                      xtrans_TIMEOUT_FOREVER表示无限等待。
* 输  出  : - pVaule :  队列成员的链表头指针的指针。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_queListGet(xtrans_QueHandle  hQue,
                     xtrans_ListHead **ppQueElem,
                     HPR_UINT32         timeout);


/* 以下接口，链表队列和普通队列都可以使用。*/

/*******************************************************************************
* 函数名  : xtrans_queIsEmpty
* 描  述  : 判断队列是否为空。
* 输  入  : - hQue: 队列句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
Bool32 xtrans_queIsEmpty(xtrans_QueHandle hQue);


/*******************************************************************************
* 函数名  : xtrans_queGetLen
* 描  述  : 获取队列长度。
* 输  入  : - hQue: 队列句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_UINT32 xtrans_queGetLen(xtrans_QueHandle hQue);


/*******************************************************************************
* 函数名  : xtrans_queGetCount
* 描  述  : 获取队列当前已经缓存的成员数量。
* 输  入  : - hQue: 队列句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_UINT32 xtrans_queGetCount(xtrans_QueHandle hQue);


#endif /* _xtrans_QUE_H_ */

