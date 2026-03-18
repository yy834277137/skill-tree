#ifndef __XTRANS_PORT_H__
#define __XTRANS_PORT_H__

#include "xtrans_msg.h"
#include "xtrans_config.h"

typedef enum xtrans_prot_func
{
    XTRANS_PROT_FUNC_UNKNOW = 0,
    XTRANS_PROT_MSG_RECEIVE = (1<<0x1),   /* 消息接收端口 */
    XTRANS_PROT_MSG_SEND = (1<<0x2),      /* 消息发送端口 */
    XTRANS_PROT_STREAM_RECEIVE = (1<<0x3),/* 码流接收端口 */
    XTRANS_PROT_STREAM_SEND = (1<<0x4),   /* 码流发送端口 */

}xtrans_prot_func_e;


typedef struct xtrans_msg_info
{
    HPR_UINT32 u32MsgSize;    /* 传输的消息长度 */
	HPR_UINT32 offset;		/*传输的读取偏移量*/	
    void *pvVirAddr;        /* 传输的消息虚拟地址 */
    intptr_t ipPhyAddr;     /* 传输的消息物理地址 */
    HPR_UINT8 reserve[8];
}xtrans_msg_info_s;


typedef struct xtrans_stm_info
{
    HPR_UINT32 u32StmSize;        /* 传输的码流长度 */
    void *pvSrcVirAddr;         /* 传输的源码流虚拟地址 */
    void *pvDstVirAddr;         /* 传输的目的码流虚拟地址 */
    intptr_t ipSrcPhyAddr;      /* 传输的源码流物理地址 */
    intptr_t ipDstPhyAddr;      /* 传输的目的码流物理地址 */
    HPR_UINT8 reserve[8];
}xtrans_stm_info_s;


typedef struct xtrans_port_stm_rec_attr
{
/*----------------------------接收回调接口↓↓↓------------------------------------------*/	

	//1、码流接收时，如果是DMA拷贝需要申请一块内存，通过此接口
	HPR_INT32 (*phyMalloc)(HPR_UINT32 u32Size, intptr_t *aiphyAddr,void **avpVirAddr);

	/*不使用一定要置NULL*/
	//2、消息处理的回调,dma数据传输就使用消息传递告诉码流地址
	HPR_INT32 (*msgProcess)(const void *pstStmInfo/*XTRANS_BUF_HEAD_S*/,
								xtrans_comm_desc_s stTransFromDesc,/* 来自的传输模块的设备描述 */
								const char *pstInfo);

	//3、数据处理完，调用此接口进行内存释放
	HPR_INT32 (*free)(intptr_t aiphyAddr,void *pvVirAddr);

	
/*----------------------------发送回调接口↓↓↓------------------------------------------*/	

	//码流处理的回调，不使用一定要置NULL
	//HPR_INT32 (*stmProcess)(const void *pstStmInfo/*XTRANS_BUF_HEAD_S*/,const char *buf);
	
	//码流发送完成的回调通知，可以用于释放内存
	HPR_INT32 (*stmSendOver)(const xtrans_stm_info_s *stmInfo);

	HPR_INT32 portIndex;//要小于1000
    HPR_UINT8 reserve[8];
}xtrans_port_stm_rec_attr_s;


typedef struct xtrans_port_attr
{
    HPR_UINT8 u8Func;         /* 端口功能xtrans_prot_func_e掩码 */
    union{
        xtrans_port_stm_rec_attr_s stStmRecAttr;  /* 码流接收端口属性 */
    };
    HPR_UINT8 reserve[8];
}xtrans_port_attr_s;


typedef struct xtrans_port
{
    void *private;

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 接收消息

     \parma [in] Thiz 指向TRANS模块端口设备
     \parma [in] pstBuf 指向TRANS设备端口设备需要传输的内存块信息，该内存由调用者申请
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*msgReceive)(struct xtrans_port *Thiz, xtrans_msg_info_s *pstBuf);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 发送消息

     \parma [in] Thiz 指向TRANS模块端口设备
     \parma [in] pstBuf 指向TRANS设备端口设备需要接收的内存块信息，该内存由调用者申请
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*msgSend)(struct xtrans_port *Thiz, xtrans_msg_info_s *pstBuf);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 接收码流

     \parma [in] Thiz 指向TRANS模块端口设备
     \parma [in] pstBuf 指向TRANS设备端口设备需要接收的内存块信息，该内存由调用者申请
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*streamReceive)(struct xtrans_port *Thiz, xtrans_stm_info_s *pstBuf);
    
    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 发送码流

     \parma [in] Thiz 指向TRANS模块端口设备
     \parma [in] pstBuf 指向TRANS设备端口设备需要接收的内存块信息，该内存由调用者申请
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*streamSend)(struct xtrans_port *Thiz, xtrans_stm_info_s *pstBuf);

	
	HPR_INT32 (*phymemMalloc)(struct xtrans_port *Thiz, HPR_UINT32 u32BlkSize, intptr_t *aiphyAddr,void **avpVirAddr);
	
	HPR_INT32 (*phymemFree)(struct xtrans_port *Thiz,intptr_t aiphyAddr,void *pvVirAddr);

	HPR_INT32 (*getAttr)(struct xtrans_port *Thiz,xtrans_port_attr_s *pattr);

    HPR_UINT8 reserve[64];
}xtrans_port_s;

#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 创建TRANS模块端口设备

 \param [in] enType TRANS模块端口类型
 \param [in] pstDesc 指向TRANS模块描述
 \param [in] pstAttr 指向TRANS模块端口属性
 \param [out] ppstPort 指向TRANS模块端口的指针
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_PortCreate(HPR_UINT8 enType, xtrans_desc_s *pstDesc, xtrans_port_attr_s *pstAttr, xtrans_port_s **ppstPort);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 销毁TRANS模块端口设备

 \param [out] ppstPort 指向TRANS模块端口设备
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_PortDestroy(xtrans_port_s *pstPort);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */



#endif

