#ifndef XTRANS_BUSSINESS_DEV_H
#define XTRANS_BUSSINESS_DEV_H

typedef enum X_BUSSINESS_DEV_RET_E
{
    XTRANS_BUSSINESS_DEV_EXIST = 1,
    XTRANS_BUSSINESS_DEV_RET_SUC = 0,

}XTRANS_BUSSINESS_DEV_RET_E;


typedef struct 
{
	HPR_UINT8 u8Func;         /* 端口功能xtrans_prot_func_e掩码 */
	
	/*----------------------------接收回调接口↓↓↓------------------------------------------*/	

	//1、码流接收时，如果是DMA拷贝需要申请一块内存，通过此接口
	HPR_INT32 (*phyMalloc)(HPR_UINT32 u32Size, intptr_t *aiphyAddr,void **avpVirAddr);

	/*不使用一定要置NULL*/
	//2、消息处理的回调,dma数据传输就使用消息传递告诉码流地址
	HPR_INT32 (*msgRecvProcess)(const void *pstStmInfo/*XTRANS_BUF_HEAD_S*/,
								xtrans_comm_desc_s stTransFromDesc,/* 来自的传输模块的设备描述 */
								const char *pstInfo);

	//3、数据处理完，调用此接口进行内存释放
	HPR_INT32 (*free)(intptr_t aiphyAddr,void *pvVirAddr);
	

	/*----------------------------发送回调接口↓↓↓------------------------------------------*/	
	
	//码流发送完成的回调通知;
	//可以用于释放内存，也可以干其他你想干的任何事情!
	HPR_INT32 (*stmSendOver)(const xtrans_stm_info_s *stmInfo);


	
}XTRANS_BUSSINESS_STM_S;

typedef struct 
{
	
	/*----------------------------接收回调接口↓↓↓------------------------------------------*/	

	HPR_INT32 (*recvMsgProcCallback)(xtrans_MsgqMsg *pstMsg);/* 消息处理处理函数，*/

	

	/*----------------------------发送回调接口↓↓↓------------------------------------------*/	



	
}XTRANS_BUSSINESS_MSG_S;



typedef struct XTRANS_BUSSINESS_DEV
{
    void *Private;
	
    HPR_INT32 (* init)(struct XTRANS_BUSSINESS_DEV *Thiz);
    /**--------------------------------------------------------------------------------------------------------------------@n
   	 \brief 去初始化
   	 ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (* deinit)(struct XTRANS_BUSSINESS_DEV *Thiz);

	/**--------------------------------------------------------------------------------------------------------------------@n
    	\brief发送消息
    	----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (* sendMsg)(struct XTRANS_BUSSINESS_DEV *Thiz,
	HPR_UINT16 cmd,//模块内部命令
	xtrans_comm_desc_s to,//接收端
	HPR_UINT16 u16Flags,//命令的flag，用于是否需要应答
	void *msg,//命令内容，
	HPR_INT32 MsgSize,//命令数据大小
	HPR_UINT8 timeout);//是否需要超时

	/**--------------------------------------------------------------------------------------------------------------------@n
    	\brief 发送码流
   	 ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (* sendStream)(struct XTRANS_BUSSINESS_DEV *Thiz,
	xtrans_comm_desc_s to,//接收端
	xtrans_buf_info_s *pbufinfo,//码流数据，DMA方式需要指定物理地址
	xtrans_data_info_s *puDataInfo//码流数据信息，用于描述此次数据
	);
   
	
    HPR_UINT8 reserve[32];
}XTRANS_BUSSINESS_DEV_S;
/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif



/**--------------------------------------------------------------------------------------------------------------------@n
\brief 创建BUSSINESS对象,如只发送消息,可以指定NULL 码流属性,
\param  link属性和码流属性，link属性内部是消息传输连接器，并且包含了模块名称
1、总共最大支持256个业务module的同时传输，具体个数按传输方式而定
2、pcie v1版本(不支持多芯片的版本)目前支持同时8个业务module的码流传输
3、TCP可以支持256个业务module的码流传输
4、USB最多可支持100个业务module的码流传输 
5、PCIE v2版本(支持多芯片的版本)暂时还未实现fixme
\return OK 成功
\return FAIL 失败
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_create_bus_dev(xtrans_msg_module_id_e id,
XTRANS_BUSSINESS_STM_S *pstStmAttr,
XTRANS_BUSSINESS_MSG_S *pstMsgAttr,
struct XTRANS_BUSSINESS_DEV **Thiz
);


/**--------------------------------------------------------------------------------------------------------------------@n
\brief 销毁BUSSINESS对象暂时还未 实现fixme
\param  
\return OK 成功
\return FAIL 失败
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_destory_bus_dev(struct XTRANS_BUSSINESS_DEV *Thiz);




/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */

#endif
