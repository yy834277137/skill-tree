
#ifndef _XTRANS_MBX_H_
#define _XTRANS_MBX_H_


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */

/* 消息的标志，表明消息的属性。*/
typedef enum
{  
    /* 无效标志 */
    XTRANS_MBX_FLAG_NON = 0x0000,
    
    /* 等待消息回复的标志 */
    XTRANS_MBX_WAIT_ACK = 0x0002,
    
    /* 说明消息本身是Malloc的，需要被Free。*/
    XTRANS_MBX_FREE_MSG = 0x0004, 
    
    /* 说明消息的pPrm是Malloc的，需要被Free。*/
    XTRANS_MBX_FREE_PRM = 0x0008, 
} xtrans_MbxMsgFlags;


/* 广播消息时，接收邮箱句柄队列的最大数量。*/
#define XTRANS_MBX_BROADCAST_MAX 32   


typedef void * xtrans_MbxHandle;


/* ========================================================================== */
/*                          函数声明区                                        */
/* ========================================================================== */

/*******************************************************************************
* 函数名  : xtrans_mbxCreate
* 描  述  : 创建邮箱。          
* 输  入  : - nMsgNum: 邮箱的消息数量。
*         : - phMsgq : 返回的邮箱句柄，后续所有操作基于此句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_mbxCreate(HPR_UINT32 nMbxNum, xtrans_MbxHandle *phMbx);


/*******************************************************************************
* 函数名  : xtrans_mbxDelete
* 描  述  : 删除邮箱。          
* 输  入  - hMbx : 邮箱句柄。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_mbxDelete(xtrans_MbxHandle hMbx);


/*******************************************************************************
* 函数名  : xtrans_mbxSendMsg
* 描  述  : 发送消息。通过单个形式参数传递消息信息。         
* 输  入  : - hMsgqTo  : 接收者的邮箱句柄。
*           - hMsgqFrom: 发送者的邮箱句柄。
*           - cmd      : 命令。
*           - flags    : 标志。其定义见xtrans_MbxMsgFlags。
*           - pPrm     : 消息参数。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_mbxSendMsg(xtrans_MbxHandle hMbxTo, 
                     xtrans_MbxHandle hMbxFrom,
				     HPR_UINT32        cmd, 
				     HPR_UINT16        flags,
				     void         *pPrm);


/*******************************************************************************
* 函数名  : xtrans_mbxBroadcastMsg
* 描  述  : 广播消息。         
* 输  入  : - phMbxToList  : 接收者的邮箱句柄组，包含所有接收消息的邮箱句柄。
*                            其最大数量见xtrans_MBX_BROADCAST_MAX定义。
*           - hMsgqFrom    : 发送者的邮箱句柄。
*           - cmd          : 命令。
*           - flags        : 标志。其定义见xtrans_MbxMsgFlags。
*           - pPrm         : 消息参数。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/			     
HPR_INT32 xtrans_mbxBroadcastMsg(xtrans_MbxHandle *phMbxToList, 
				          xtrans_MbxHandle  hMbxFrom, 
				          HPR_UINT32         cmd, 
				          HPR_UINT16         flags, 
				          void          *pPrm);


/*******************************************************************************
* 函数名  : xtrans_mbxRecvMsg
* 描  述  : 接收消息。        
* 输  入  : - hMsgq  : 邮箱句柄。
*           - ppMsg  : 消息指针。注意是指针的指针。
*           - timeout: 超时时间。单位是毫秒。xtrans_TIMEOUT_NONE表示不等待，
*                      xtrans_TIMEOUT_FOREVER表示无限等待。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_mbxRecvMsg(xtrans_MbxHandle hMbx, 
                     xtrans_MsgqMsg **pMsg,
                     HPR_UINT32        timeOut);

/*******************************************************************************
* 函数名  : xtrans_mbxRecvMsg
* 描  述  : 检查并接收消息。有无消息都立即返回，不等待。        
* 输  入  : - hMsgq  : 邮箱句柄。
*           - ppMsg  : 消息指针。注意是指针的指针。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_mbxCheckMsg(xtrans_MbxHandle hMbx, 
                      xtrans_MsgqMsg **pMsg);


/*******************************************************************************
* 函数名  : xtrans_mbxAckOrFreeMsg
* 描  述  : 发送回复并释放消息内存。        
* 输  入  : - hMsgq    : 邮箱句柄。
*           - ackRetVal: 返回值。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_mbxAckOrFreeMsg(xtrans_MsgqMsg *pMsg, HPR_INT32 ackRetVal);


/*******************************************************************************
* 函数名  : xtrans_mbxFlush
* 描  述  : 刷新邮箱。清除所有邮箱中的消息。        
* 输  入  : - hMsgq    : 邮箱句柄。
*           - ackRetVal: 返回值。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_mbxFlush(xtrans_MbxHandle hMbx);


/*******************************************************************************
* 函数名  : xtrans_mbxWaitCmd
* 描  述  : 等待指定的命令值。        
* 输  入  : - hMsgq  : 邮箱句柄。
*           - pMsg   : 包含命令值的消息。
*           - waitCmd: 命令值。
* 输  出  : 无。
* 返回值  : xtrans_SOK  : 成功。
*           xtrans_EFAIL: 失败。
*******************************************************************************/
HPR_INT32 xtrans_mbxWaitCmd(xtrans_MbxHandle hMbx, 
                     xtrans_MsgqMsg **pMsg, 
					 HPR_UINT32        waitCmd);

#endif /* _xtrans_MBX_H_ */


