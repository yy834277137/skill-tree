
#ifndef _XTRANS_MSGQ_H_
#define _XTRANS_MSGQ_H_


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

#define xtrans_msgGetCmd(msg)         ( (msg)->cmd )
#define xtrans_msgGetPrm(msg)         ( (msg)->pPrm )
#define xtrans_msgGetAckStatus(msg)   ( (msg)->status )

typedef HPR_HANDLE xtrans_MsgqHandle;


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */

/* 消息 */
typedef struct 
{
	HPR_UINT32 cmd;
	HPR_UINT16 flags; 
	Ptr    pPrm;
	HPR_INT32  status;
} xtrans_MsgqMsg;


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : xtrans_msgqCreate
* 描  述  : 创建消息队列。          
* 输  入  : - nMsgNum: 队列的消息数量。
*         : - phMsgq : 返回的消息队列句柄，后续所有操作基于此句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_msgqCreate(HPR_UINT32 nMsgNum, xtrans_MsgqHandle *phMsgq);


/*******************************************************************************
* 函数名  : xtrans_msgqDelete
* 描  述  : 销毁消息队列。          
* 输  入  : - hMsgq: 消息队列句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_msgqDelete(xtrans_MsgqHandle hMsgq);


/*******************************************************************************
* 函数名  : xtrans_msgqSend
* 描  述  : 发送消息。          
* 输  入  : - hMsgq  : 消息队列句柄。
*           - pMsg   : 消息。
*           - timeout: 超时时间。单位是毫秒。xtrans_TIMEOUT_NONE表示不等待，
*                      xtrans_TIMEOUT_FOREVER表示无限等待。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_msgqSend(xtrans_MsgqHandle hMsgq, 
                   xtrans_MsgqMsg   *pMsg, 
                   HPR_UINT32         timeout);


/*******************************************************************************
* 函数名  : xtrans_msgqSendMsg
* 描  述  : 发送消息。通过单个形式参数传递消息信息。         
* 输  入  : - hMsgqTo  : 接收者的消息队列句柄。
*           - hMsgqFrom: 发送者的消息队列句柄。
*           - cmd      : 命令。
*           - msgFlags : 用户自定义标志。
*           - pPrm     : 消息参数。
*           - ppMsg    : 返回的消息指针，供用户使用。注意是指针的指针。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_msgqSendMsg(xtrans_MsgqHandle hMsgqTo, 
                      xtrans_MsgqHandle hMsgqFrom, 
                      HPR_UINT32         cmd,      
                      HPR_UINT16         msgFlags, 
                      void          *pPrm,  
                      xtrans_MsgqMsg  **ppMsg);


/*******************************************************************************
* 函数名  : xtrans_msgqRecvMsg
* 描  述  : 接收消息。        
* 输  入  : - hMsgq  : 消息队列句柄。
*           - ppMsg  : 消息指针。注意是指针的指针。
*           - timeout: 超时时间。单位是毫秒。xtrans_TIMEOUT_NONE表示不等待，
*                      xtrans_TIMEOUT_FOREVER表示无限等待。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_msgqRecvMsg(xtrans_MsgqHandle hMsgq, 
                      xtrans_MsgqMsg  **ppMsg,
				      HPR_UINT32         timeout);


/*******************************************************************************
* 函数名  : xtrans_msgqCheckMsg
* 描  述  : 检查是否有消息。        
* 输  入  : - hMsgq: 消息队列句柄。
* 输  出  : 无。
* 返回值  : xtrans_TRUE  : 成功。
*           xtrans_FALSE : 失败。
*******************************************************************************/
Bool32 xtrans_msgqCheckMsg(xtrans_MsgqHandle hMsgq);


/*******************************************************************************
* 函数名  : xtrans_msgqSendAck
* 描  述  : 发送回复值。        
* 输  入  : - hMsgq    : 消息队列句柄。
*           - ackRetVal: 回复值。
* 输  出  : 无。
* 返回值  : xtrans_TRUE  : 成功。
*           xtrans_FALSE : 失败。
*******************************************************************************/
HPR_INT32 xtrans_msgqSendAck(xtrans_MsgqMsg *pMsg, HPR_INT32 ackRetVal);


/*******************************************************************************
* 函数名  : xtrans_msgqAllocMsg
* 描  述  : 分配消息内存。        
* 输  入  : - ppMsg: 消息指针的指针。
* 输  出  : 无。
* 返回值  : xtrans_TRUE  : 成功。
*           xtrans_FALSE : 失败。
*******************************************************************************/
HPR_INT32 xtrans_msgqAllocMsg(xtrans_MsgqMsg **ppMsg);


/*******************************************************************************
* 函数名  : xtrans_msgqFreeMsg
* 描  述  : 释放已经分配的消息内存。        
* 输  入  : - pMsg: 消息指针。
* 输  出  : 无。
* 返回值  : xtrans_TRUE  : 成功。
*           xtrans_FALSE : 失败。
*******************************************************************************/
HPR_INT32 xtrans_msgqFreeMsg(xtrans_MsgqMsg *pMsg);


/*******************************************************************************
* 函数名  : xtrans_msgqFreeList
* 描  述  : 释放消息到队列中。        
* 输  入  : - pMsg: 消息指针。
* 输  出  : 无。
* 返回值  : xtrans_TRUE  : 成功。
*           xtrans_FALSE : 失败。
*******************************************************************************/
HPR_INT32 xtrans_msgqFreeList(xtrans_MsgqMsg *pMsg);


#endif /* _xtrans_MSGQ_H_ */

