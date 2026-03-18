

#ifndef _XTRANS_LIST_H_
#define _XTRANS_LIST_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */


#ifdef __cplusplus
extern "C" {
#endif


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

/* 用于初始化链表节点 */
#ifndef _WIN32
#define xtrans_LIST_POISON1  ((void *) 0xAC11CD76)
#define xtrans_LIST_POISON2  ((void *) 0xAC12CD86)
#else
#define xtrans_LIST_POISON1  NULL
#define xtrans_LIST_POISON2  NULL
#endif


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* 链表头 */
typedef struct xtrans_ListHead
{
	struct xtrans_ListHead *next;
	struct xtrans_ListHead *prev;
} xtrans_ListHead;


/* 链表静态定义和初始化 */
#define xtrans_LIST_HEAD(name) \
	xtrans_ListHead name = { &(name), &(name) }


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/* 链表动态初始化 */
static inline void xtrans_listHeadInit(xtrans_ListHead *list)
{
	list->next = list;
	list->prev = list;
}


/* 通用添加成员的接口，需指定队列的前节点和后节点。*/
static inline void __xtrans_listAdd(xtrans_ListHead *newList,
			                     xtrans_ListHead *prev,
			                     xtrans_ListHead *next)
{
	next->prev = newList;
	newList->next = next;
	newList->prev = prev;
	prev->next = newList;
}


/* 添加成员的接口，head的next指向队尾，head的prev指向下一个元素。*/
static inline void xtrans_listAdd(xtrans_ListHead *newList, xtrans_ListHead *head)
{
	__xtrans_listAdd(newList, head, head->next);
}


/* 添加成员的接口，head的next指向下一个元素，head的prev指向队尾。*/
static inline void xtrans_listAddTail(xtrans_ListHead *newList, xtrans_ListHead *head)
{
	__xtrans_listAdd(newList, head->prev, head);
}


/* 从链表中删除成员的通用接口，指定该成员的队头和队尾。*/
static inline void __xtrans_listDel(xtrans_ListHead * prev, xtrans_ListHead * next)
{
	next->prev = prev;
	prev->next = next;
}


/* 从链表中删除成员的通用接口，指定该成员。*/
static inline void xtrans_listDel(xtrans_ListHead *entry)
{
	__xtrans_listDel(entry->prev, entry->next);
	entry->next = (xtrans_ListHead *)xtrans_LIST_POISON1;
	entry->prev = (xtrans_ListHead *)xtrans_LIST_POISON2;
}


/* 判断链表是否为空的接口。*/
static inline int xtrans_listEmpty(const xtrans_ListHead *head)
{
	return head->next == head;
}


/* 通过链表成员获取包裹结构体的首地址。*/ 
#define xtrans_listEntry(ptr, type, member) \
	xtrans_containerOf(ptr, type, member)


/* 遍历链表，过程中获得的节点不可被删除。*/ 
#define xtrans_listForEach(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)

/* 遍历链表，过程中获得的节点可安全删除。tmp为临时节点指针。*/
#define xtrans_listForEachSafe(pos, tmp, head) \
    for (pos = (head)->next, tmp = pos->next; pos != (head); \
         pos = tmp, tmp = pos->next)


#ifdef __cplusplus
    }
#endif
    
    
#endif /* _xtrans_LIST_H_ */

