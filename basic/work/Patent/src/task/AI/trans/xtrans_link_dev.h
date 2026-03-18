/******************************************************************************

   Copyright 2020-2030 Hikvision Co.,Ltd

   FileName: xtrans_link_dev.h

   Description连接器

   Author:baoqingti

   Date:2021.6.22

   Modification History: <version> <time> <author> <desc>
   a)
******************************************************************************/
#ifndef __XTRANS_LIKN_DEVICE_H__
#define __XTRANS_LIKN_DEVICE_H__

#include "xtrans_msgq.h"
#include "xtrans_msg.h"


#define XTRANS_LINK_MBX_QUEUE_MAX_SIZE          32

enum xtrans_link_errno
{
    XTRANS_LINK_SUCCESS = 0,
    XTRANS_LINK_NOINIT = 0xa0002000,
    XTRANS_LINK_PARAMILL = 0xa0002001,
    XTRANS_LINK_TRANSGETFAILE = 0xa0002002,
};

typedef struct xtrans_link_init_attr
{
    xtrans_comm_desc_s stTransDesc;
    HPR_UINT8 reserve[12];
}xtrans_link_init_attr_s;

typedef struct xtrans_link_device_attr
{
    xtrans_msg_module_id_e eLinkId;/* link id号 */
    HPR_UINT32 uint32MbxQueueSize;/* 邮箱队列长度 */
    HPR_INT32 (*recvMsgProcCallback)(xtrans_MsgqMsg *pstMsg);/* 消息处理处理函数，注意消息的释放是在回调函数中完成 */
    HPR_UINT8 reserve[8];
}xtrans_link_device_attr_s;

typedef struct xtrans_link_device
{
    void *private;

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 发送消息

     \param [in] Thiz link设备指针
     \param [in] hMsg link消息元素
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*sendMsg)(struct xtrans_link_device *Thiz, xtrans_msg_s *pstMsg);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 清空邮箱中的消息

     \param [in] Thiz link设备指针
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*mbxFlush)(struct xtrans_link_device *Thiz);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 清空邮箱中的消息

     \param [in] Thiz link设备指针
     \param [in] pstMsg 应答或释放的消息
     \param [in] s32AckRetVal 应答返回值
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*ackOrFreeMsg)(struct xtrans_link_device *Thiz, xtrans_MsgqMsg *pstMsg, HPR_INT32 s32AckRetVal);

    HPR_UINT8 reserve[32];
}xtrans_link_device_s;


/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 初始化LINK模块,link连接器可以将消息的发送和接收进行封装

 \param pstAttr [in] 指向初始化属性的指针
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_Init(xtrans_link_init_attr_s *pstAttr);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 去初始化LINK模块
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_Deinit(void);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 销毁TRANS模块申请内存块
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_AllocMem(HPR_UINT32 s32Size, void **ppvMem);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 销毁TRANS模块释放内存块
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_FreeMem(void *pvMem);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 创建LINK模块

 \param pstAttr [in] 创建属性
 \param ppDevice [out] 指向LINK模块的指针的指针
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_DeviceCreate(xtrans_link_device_attr_s *pstAttr, xtrans_link_device_s **ppDevice);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 销毁LINK模块

 \param ppDevice [in] 指向LINK模块的指针
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_DeviceDestroy(xtrans_link_device_s *pstDevcie);




/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 创建TRANS模块Pool
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_PoolCreate(void);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 销毁TRANS模块Pool
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_PoolDestroy(void);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 初始化TRANS模块
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_PoolAdd(xtrans_msg_module_id_e enLinkModuleId, xtrans_link_device_s *pstLink);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 初始化TRANS模块
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_PoolDelete(xtrans_link_device_s *pstLink);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 初始化TRANS模块
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Link_PoolGet(xtrans_msg_module_id_e enLinkModuleId, xtrans_link_device_s **ppstLink);

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */




#endif