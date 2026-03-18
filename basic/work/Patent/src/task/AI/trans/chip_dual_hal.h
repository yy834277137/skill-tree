/**
 * @file:   chip_dual_hal.h
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  dual-chip transmit header file
 * @author: sunzelin
 * @date    2020/11/17
 * @note:
 * @note \n History:
   1.日    期: 2020/11/17
     作    者: sunzelin
     修改历史: 创建文件
 */
#ifndef _CHIP_DUAL_HAL_H_
#define _CHIP_DUAL_HAL_H_

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define TRANS_HAL_MAX_ARG_NUM (16)

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef enum _TRANS_HAL_PROC_CMD_
{
    TRANS_HAL_REGISTER = 0x1,                 /* 实现模块注册，目前仅有USB和PCIE */
    TRANS_HAL_UNREGISTER,                     /* 实现模块反注册 */
    TRANS_HAL_INIT_DEV,                       /* 初始化设备 */
    TRANS_HAL_DEINIT_DEV,                     /* 去初始化设备 */
    TRANS_HAL_GET_DEV_FD,                     /* 获取设备描述符 */
    TRANS_HAL_SEND_DATA,                      /* 传输数据 */
    TRANS_HAL_RECV_DATA,                      /* 接收数据 */
    TRANS_HAL_CHECK_DATA_STATUS,              /* 确认主从传输的数据状态 */
    TRANS_HAL_SET_DATA_STATUS,                /* 设置主从传输的数据状态 */
    TRANS_HAL_GET_DMA_PHYADDR,                /* 获取使用DMA主从传输时的物理地址，通过mmap内存拷贝 */
    TRANS_HAL_SET_DMA_PHYADDR,                /* 设置使用DMA主从传输时的物理地址，通过mmap内存拷贝 */
    TRANS_HAL_PROC_CMD_NUM,
} TRANS_HAL_PROC_CMD_E;

typedef enum _TRANS_HW_TYPE_
{
    USB_TYPE = 0,                             /* USB传输类型 */
    PCIE_TYPE,                                /* PCIE传输类型 */
    TRANS_HW_TYPE_NUM,
} TRANS_HW_TYPE_E;

typedef struct _TRANS_HAL_DATA_INFO_
{
    UINT32 uiArgCnt;                          /* 抽象层参数个数 */
    UINT64 u64Arg[TRANS_HAL_MAX_ARG_NUM];     /* 抽象层参数数组 */
} TRANS_HAL_DATA_INFO;

typedef struct _TRANS_HAL_DATA_INFO_V0_
{
    UINT32 enType;

    UINT32 uiDataSize;
    union
    {
        PhysAddr u64PhyAddr;
        VOID *pVirAddr;
    };

    UINT32 uiReserve[1];
} TRANS_HAL_DATA_INFO_V0;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/**
 * @function:   Trans_HalCmdProc
 * @brief:      抽象层支持的命令字
 * @param[in]:  TRANS_HAL_PROC_CMD_E enCmd
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Trans_HalCmdProc(TRANS_HAL_PROC_CMD_E enCmd, VOID *prm);

#endif

