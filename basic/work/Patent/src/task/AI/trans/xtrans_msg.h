/*
create baoqingti 
date:2021.7.1

!!!!!该头文件对外头文件不允许经常性的变动，保持其稳定性!!!!!

*/   
#ifndef __XTRANS_MSG_H__
#define __XTRANS_MSG_H__

#include "xtrans_config.h"


typedef enum xtrans_msg_module_id
{
	/*-----APP使用base0区域，模块可以随意定义增加------*/
	XTRANS_MSG_MODULE_BASE0 = 0x00,
	/*add*/




	/*-----DSP使用base1区域，模块可以随意定义增加------*/
	XTRANS_MSG_MODULE_BASE1 = 0xFF,
	/*add*/
	XTRANS_MSG_MODULE_VPSS,/*for test*/
	XTRANS_MSG_MODULE_VENC,/*for test*/
	XTRANS_MSG_MODULE_RSLT,/* 智能结果传输，DMA */

	//可以新增，最大支持45535个模块!!!!
	
	XTRANS_MSG_MODULE_TEST,
    XTRANS_MSG_MODULE_BUT=0xFFFF,
}xtrans_msg_module_id_e;



#define XTRANS_MSG_MAKE_CMD(module, cmd)    (((module&0xffff)<<16)|(cmd&0xffff))
#define XTRANS_MSG_GET_MODULE(cmd)          ((cmd>>16)&0xffff)
#define XTRANS_MSG_GET_CMD(cmd)             (cmd&0xffff)


typedef enum xtrans_msg_flag
{
    XTRANS_MSG_FLAG_NONE = 0,
    XTRANS_MSG_WAIT_ACK = 0x0010,/* 需要等待应答 */
    XTRANS_MSG_AWAKE_ACK = 0x0100,
}xtrans_msg_flag_e;


/*!> 整个结构体为一块完整连续内存，大小等于XTRANS_MSG_COMM_SIZE+sizeof(消息结构体) */
typedef struct xtrans_msg
{
    HPR_HANDLE hHandle;     /* 用于匹配消息，指向消息首地址 */
    HPR_UINT32 u32Cmd;    /* 消息命令字，使用XTRANS_MSG_MAKE_CMD组合 */
	HPR_UINT32 u32Size;   /* 消息长度 */
    HPR_UINT16 u16Flags;  /* 消息标志位，使用xtrans_msg_flag_e，mask，使用|运算 */
    HPR_UINT8 u8Wait;     /* 等待 */
    HPR_UINT8 u8Awake;    /* 唤醒 */
    HPR_INT32 s32Status;  /* 状态，可以作为返回值 */
    xtrans_comm_desc_s stTransFromDesc;/* 来自的传输模块的设备描述 */
    xtrans_comm_desc_s stTransToDesc;/* 去往的传输模块的设备描述 */
    HPR_UINT8 pal1[32];     /* 保留字 */
    void *pvPrm;        /* 发送消息结构体 */
}xtrans_msg_s;

#define XTRANS_MSG_COMM_SIZE (sizeof(xtrans_msg_s)-sizeof(void *))



#endif
