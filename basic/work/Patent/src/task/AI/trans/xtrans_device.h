   
#ifndef __XTRANS_DEVICE_H__
#define __XTRANS_DEVICE_H__


#include "xtrans_config.h"

#include "xtrans_port.h"


enum xtrans_errno
{
    XTRANS_SUCCESS = 0,
    XTRANS_NOINIT = 0xa0001000,
    XTRANS_NOMEM = 0xa0001001,
    XTRANS_ERRTYPE = 0xa0001002,
    XTRANS_MGRFAIL = 0xa0001003,
    XTRANS_NOCREATE = 0xa0001004,
    XTRANS_RESBUSY = 0xa0001005,
    XTRANS_NOTFIND = 0xa0001006,
    XTRANS_LOADFAIL = 0xa0001007,
    XTRANS_CONNECTFAIL = 0xa0001008,
    XTRANS_PORTOPENFAIL = 0xa0001009,
    XTRANS_MSGREADFAIL = 0xa000100a,
    XTRANS_MSGSENDFAIL = 0xa000100b,
    XTRANS_PORTCLOSEFAIL = 0xa000100c,
    XTRANS_WRITESTMFAIL = 0xa000100d,
    XTRANS_CREATEFAIL = 0xa000100e,
    XTRANS_UPDATASLAVEFAIL = 0xa000100f,
};


/* xtrans库基础信息 */
typedef struct xtrans_lib_time
{
	HPR_INT32 year;
	HPR_INT32 moth;
	HPR_INT32 day;
	HPR_INT32 hour;
	HPR_INT32 minute;
	HPR_INT32 second;
}xtrans_lib_time_s;

typedef struct xtrans_lib_version
{
	HPR_INT32 mainVersion;
	HPR_INT32 secondVersion;
	HPR_INT32 thirdVersion;
}xtrans_lib_version_s;

#define XTRANS_BUILD_AUTHOR_LEN (16)
#define XTRANS_BUILD_REVISION_LEN (8)

typedef struct xtrans_lib_info
{
	HPR_INT8 author[XTRANS_BUILD_AUTHOR_LEN];
	HPR_INT8 revision[XTRANS_BUILD_REVISION_LEN];
	xtrans_lib_time_s time;
	xtrans_lib_version_s version;
}xtrans_lib_info_s;

typedef struct xtrans_chip_device
{
    void *private;
    
    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 启动从片
     
     \parma [in] pstDev 指向TRANS设备端口设备指针
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*startSlave)(struct xtrans_chip_device *Thiz);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief重启从片

     \parma [in] phHandle 指向TRANS模块设备池
     \parma [in] pstDev 指向TRANS设备端口设备指针
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*resetSlave)(struct xtrans_chip_device *Thiz);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 等待和对端链接成功

     \parma [in] phHandle 指向TRANS模块
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*waitConnect)(struct xtrans_chip_device *Thiz);


    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 申请内存

     \param [in] u32Size 块大小
     \param [out] aiphyAddr 块物理地址
     \param [out] avpVirAddr 块虚拟地址
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*memAlloc)(struct xtrans_chip_device *Thiz, HPR_UINT32 u32Size, intptr_t *aiphyAddr,void **avpVirAddr);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 释放内存
 
     \param [in] u32BlkSize 块大小
     \param [in] aiphyAddr 块物理地址
     \param [in] pvVirAddr 块虚拟地址
 
     \return HPR_OK 成功
     \return HPR_ERROR 失败

    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*memFree)(struct xtrans_chip_device *Thiz,  intptr_t aiphyAddr,void *pvVirAddr);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 接收消息

     \parma [in] Thiz 指向XTRANS模块端口设备
     \parma [in] pstMsg 指向XTRANS设备端口设备需要传输的内存块信息，TCP的方式
     该内存由调用者申请；USB/PCIE方式内存内部会申请物理地址进行映射
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*msgReceive)(struct xtrans_chip_device *Thiz, xtrans_msg_info_s *pstMsg);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 发送消息

     \parma [in] Thiz 指向XTRANS模块端口设备
     \parma [in] pstMsg 指向XTRANS设备端口设备需要接收的内存块信息，该内存由调用者申请
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*msgSend)(struct xtrans_chip_device *Thiz, xtrans_msg_info_s *pstMsg);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 创建传输端口

     \parma [in] Thiz 指向XTRANS模块端口设备
     \parma [in] pstAttr 指向端口创建属性
     \parma [out] ppstPort 指向需要创建的TRANS设备端口设备
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*portCreate)(struct xtrans_chip_device *Thiz, xtrans_port_attr_s *pstAttr, xtrans_port_s **ppstPort);

    /**--------------------------------------------------------------------------------------------------------------------@n
     \brief 销毁传输端口

     \parma [in] Thiz 指向TRANS模块端口设备
     \parma [in] pstPort 指向需要销毁的TRANS设备端口设备
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*portDestroy)(struct xtrans_chip_device *Thiz, xtrans_port_s *pstPort);
	
	/**--------------------------------------------------------------------------------------------------------------------@n
     \brief 升级从片

     \parma [in] Thiz 指向TRANS模块端口设备
     \parma [in] filePath 指向升级数据文件路径
     
     \return HPR_OK 成功
     \return HPR_ERROR 失败
     
    ----------------------------------------------------------------------------------------------------------------------*/
    HPR_INT32 (*updateSlave)(struct xtrans_chip_device *Thiz, void *filePath);

	
	/**--------------------------------------------------------------------------------------------------------------------@n
	 \brief 获取当前设备的通信方式

	 \parma [in] Thiz 指向TRANS模块端口设备	 
	 \return HPR_OK 成功
	 \return HPR_ERROR 失败
	 
	----------------------------------------------------------------------------------------------------------------------*/
	HPR_UINT8 (*getXtransType)(struct xtrans_chip_device *Thiz);

    HPR_UINT8 reserve[32];
}xtrans_chip_device_s;


/* *INDENT-OFF* */
#ifdef __cplusplus
extern "C" {
#endif
/* *INDENT-ON* */

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 初始化TRANS模块
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Init(void);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 去初始化TRANS模块
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_Deinit(void);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 创建TRANS模块设备

 \parma [in] pstConfig 指向TRANS模块设备配置
 \parma [out] ppstDevice 指向TRANS模块设备的指针
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_DeviceCreate(xtrans_config_s *pstConfig, xtrans_chip_device_s **ppstDevice);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 获取TRANS模块设备和端口

 \parma [in] pstDesc 指向TRANS模块设备端口描述
 \parma [out] pstDevice 指向TRANS模块设备的指针
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_DeviceGet(xtrans_comm_desc_s *pstDesc, xtrans_chip_device_s **ppstDevice);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 销毁TRANS模块设备

 \parma [out] pstDevice 指向TRANS模块设备的指针
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_DeviceDestroy(void);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 线程等待命令

 \parma [in] hHnd 命令句柄
 \parma [in] puInCmd 等待的入口命令值
 \parma [in] pu8OutCmd 等待的出口命令值
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_WaitCmd(HPR_UINT8 *pu8InCmd, HPR_UINT8 *pu8OutCmd, HPR_UINT8 timeout);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 唤醒线程等待Trans的命令

 \parma [in] hHnd 命令句柄
 \parma [in] puInCmd 等待的入口命令值
 \parma [in] pu8OutCmd 等待的出口命令值
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_AwakeCmd(HPR_UINT8 *pu8InCmd, HPR_UINT8 *pu8OutCmd);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 获取本地芯片描述

 \parma [out] pstDesc 本地描述
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_GetLocalDesc(xtrans_comm_desc_s *pstDesc);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 获取远端芯片描述

 \parma [out] astDesc 远程描述数组
 \parma [out] ps32Count 远程芯片个数
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_GetRemoteDesc(xtrans_comm_desc_s astDesc[], HPR_INT32 *ps32Count);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 内存申请，带Cache

 \param [in] u32MemSize 内存尺寸
 \param [in] ppvVirAddr 内存物理地址
 \param [in] pvVirAddr 内存虚拟地址

 \return HPR_OK 成功
 \return HPR_ERROR 失败

----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_MemAllocCached(HPR_UINT32 u32MemSize, uintptr_t *puiptrPhyAddr, void **ppvVirAddr);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 内存刷Cache

 \param [in] u32MemSize 内存尺寸
 \param [in] puiptrPhyAddr 内存物理地址
 \param [in] pvVirAddr 内存虚拟地址

 \return HPR_OK 成功
 \return HPR_ERROR 失败

----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_MemFlushCache(HPR_UINT32 u32MemSize, uintptr_t uiptrPhyAddr, void *pvVirAddr);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 内存刷Cache

 \param [in] u32MemSize 内存尺寸
 \param [in] puiptrPhyAddr 内存物理地址
 \param [in] ppvVirAddr 内存虚拟地址

 \return HPR_OK 成功
 \return HPR_ERROR 失败

----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_MemAlloc(HPR_UINT32 u32MemSize, uintptr_t *puiptrPhyAddr, void **ppvVirAddr);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 内存刷Cache

 \param [in] uiptrPhyAddr 内存物理地址
 \param [in] pvVirAddr 内存虚拟地址

 \return HPR_OK 成功
 \return HPR_ERROR 失败

----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_MemFree(uintptr_t uiptrPhyAddr, void *pvVirAddr);

/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 获取动态库信息

 \param [in] xtrans_lib_info_s 库信息

 \return HPR_OK 成功
 \return HPR_ERROR 失败

----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_GetVesion(xtrans_lib_info_s *pInfo);
	

/* *INDENT-OFF* */
#ifdef __cplusplus
}
#endif
/* *INDENT-ON* */



#endif

