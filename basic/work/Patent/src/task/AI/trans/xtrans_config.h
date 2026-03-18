/*
* @file  
* @note   HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
* @brief 芯片配置
* @author baoqingti
* @date 2021/7/16
* @note 历史记录:
* @note V4.0         新建   2021/7/16
* @warning 
*/

#ifndef __XTRANS_CONFIG_H__
#define __XTRANS_CONFIG_H__

#include "HPR_Config.h"
#include "HPR_Types.h"

#define XTRANS_DEVICE_MAX_NUM 16  /* TRANS通道最大个数 */
#define XTRANS_PORT_MAX_NUM   8   /* TRANS通道中端口的最大个数 */


typedef struct xtrans_comm_desc
{
   
    HPR_UINT8 u8ChipId;   /* 传输设备芯片描述,0表示主芯片 */
	HPR_INT8 	u8BusId;				//设备总线id，暂只支持pci从片
	HPR_UINT8 	reserve[15];
}xtrans_comm_desc_s;

typedef enum xtrans_type
{
    XTRANS_TYPE_UNKNOW = 0,
    XTRANS_PCIE_V1,      /*PCIe老版本实现 */
    XTRANS_PCIE_V2,      /*PCIe新版本实现 */
    XTRANS_SHM,
    XTRANS_TCP,       /* 网络实现 */
    XTRANS_USB
}xtrans_type_e;



typedef struct xtrans_desc
{
    xtrans_comm_desc_s stDesc;            /* 端口配置 */
	xtrans_comm_desc_s stLocalDesc;            /* 端口配置 */
    //可以增加其他配置
    HPR_UINT8 reserve[32];
}xtrans_desc_s;

typedef struct Xtrans_config_device_info
{
    HPR_UINT8 u8NeedLoad;                 /* 是否要加载 */
    HPR_UINT8 pal[3];
    HPR_UINT8 reserve[8];
}xtrans_config_device_info_s;


typedef struct xtrans_config
{
    HPR_UINT8 enType;                 /* 实现类型，xtrans_type_e */
    HPR_UINT8 pal[3];
    xtrans_desc_s stDesc;     /* 描述 */
    xtrans_config_device_info_s stDeviceInfo;/* 设备配置信息 */
    HPR_UINT8 reserve[128];
}xtrans_config_s;


typedef struct xtrans_group_config
{
    HPR_UINT32 u32Num;                                            /* trans通道个数,表示对端个数 */
    xtrans_config_s astTransCfg[XTRANS_DEVICE_MAX_NUM];/*remote trans通道配置 */
    xtrans_comm_desc_s stLocalDesc;
    HPR_UINT32 reserve[1024];
}xtrans_group_config_s;


#ifdef __cplusplus
extern "C" {
#endif


/**--------------------------------------------------------------------------------------------------------------------@n
 \brief 获取TRANS模块设备群配置

 \parma [out] pstCfg 指向TRANS模块配置指针
 
 \return HPR_OK 成功
 \return HPR_ERROR 失败
 
----------------------------------------------------------------------------------------------------------------------*/
HPR_INT32 xtrans_GroupConfigGet(xtrans_group_config_s *pstCfg);
   
 
void xtrans_SetChipType(int chip_type);

#ifdef __cplusplus
}
#endif



#endif
