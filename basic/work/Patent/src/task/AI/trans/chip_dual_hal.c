/**
 * @file:   chip_dual_hal.c
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  dual-chip transmit source file
 * @author: sunzelin
 * @date    2020/11/17
 * @note:
 * @note \n History:
   1.日    期: 2020/11/17
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "sal.h"

#include "chip_dual_hal.h"
#include "usb_transmit.h"
#include "pcie_transmit.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define TRANS_HAL_CHECK_PTR(ptr, loop, str) \
    { \
        if (!ptr) \
        { \
            SAL_ERROR("%s \n", str); \
            goto loop; \
        } \
    }

#define TRANS_HAL_CHECK_RET(ret, loop, str) \
    { \
        if (ret) \
        { \
            SAL_ERROR("%s, ret: 0x%x \n", str, ret); \
            goto loop; \
        } \
    }

#define TRANS_HAL_REG_MOD_NUM (1)

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef struct _TRANS_HAL_REG_FUNC_
{
    INT32 (*Trans_HalGetDevHandle)(VOID);
    INT32 (*Trans_HalInitDev)(VOID);
    INT32 (*Trans_HalDeinitDev)(VOID);
    INT32 (*Trans_HalSendData)(UINT32 uiArgCnt, UINT64 *pArg);
    INT32 (*Trans_HalRecvData)(UINT32 uiArgCnt, UINT64 *pArg);
    BOOL (*Trans_HalCheckDataStatus)(UINT32 uiArgCnt, UINT64 *pArg);
    INT32 (*Trans_HalSetDataStatus)(UINT32 uiArgCnt, UINT64 *pArg);
    INT32 (*Trans_HalGetDmaPhyAddr)(UINT32 uiArgCnt, UINT64 *pArg);
    INT32 (*Trans_HalSetDmaPhyAddr)(UINT32 uiArgCnt, UINT64 *pArg);
} TRANS_HAL_REG_FUNC_S;

typedef struct _TRANS_HAL_COMMON_
{
    INT32 iDevFd;                                                      /* 设备描述符 */

    /* 钩子函数: 实现获取文件描述符、打开设备、关闭设备的功能 */
    TRANS_HAL_REG_FUNC_S stRegFunc[TRANS_HAL_REG_MOD_NUM];             /* 目前只有一类注册 */

    UINT32 uiReserve[4];                                               /* 保留字段 */
} TRANS_HAL_MOD_S;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/
static TRANS_HAL_MOD_S g_transHalMod =
{
    .iDevFd = -1,
};

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/**
 * @function:   Trans_HalRegUsbFunc
 * @brief:      register function from usb module
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Trans_HalRegUsbFunc(VOID)
{
    g_transHalMod.stRegFunc[0].Trans_HalInitDev = Usb_DevInit;
    g_transHalMod.stRegFunc[0].Trans_HalDeinitDev = Usb_DevDeInit;
    g_transHalMod.stRegFunc[0].Trans_HalGetDevHandle = Usb_GetDevHandle;
    g_transHalMod.stRegFunc[0].Trans_HalSendData = Usb_SendData;
    g_transHalMod.stRegFunc[0].Trans_HalRecvData = Usb_RecvData;
    g_transHalMod.stRegFunc[0].Trans_HalCheckDataStatus = Usb_CheckDataStatus;
    g_transHalMod.stRegFunc[0].Trans_HalGetDmaPhyAddr = Usb_GetDmaPhyAddr;
    g_transHalMod.stRegFunc[0].Trans_HalSetDmaPhyAddr = Usb_SetDmaPhyAddr;
    g_transHalMod.stRegFunc[0].Trans_HalSetDataStatus = Usb_SetDataStatus;

    return;
}

/**
 * @function:   Trans_HalRegPcieFunc
 * @brief:      register function from pcie module
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static VOID
 */
static VOID Trans_HalRegPcieFunc(VOID)
{
    g_transHalMod.stRegFunc[0].Trans_HalInitDev = Pcie_DevInit;             
    g_transHalMod.stRegFunc[0].Trans_HalDeinitDev = Pcie_DevDeInit;
    g_transHalMod.stRegFunc[0].Trans_HalGetDevHandle = Pcie_GetDevHandle;
    g_transHalMod.stRegFunc[0].Trans_HalSendData = Pcie_SendData;
    g_transHalMod.stRegFunc[0].Trans_HalRecvData = Pcie_RecvData;
    g_transHalMod.stRegFunc[0].Trans_HalCheckDataStatus = Pcie_CheckDataStatus;
    g_transHalMod.stRegFunc[0].Trans_HalGetDmaPhyAddr = Pcie_GetDmaPhyAddr;
    g_transHalMod.stRegFunc[0].Trans_HalSetDmaPhyAddr = Pcie_SetDmaPhyAddr;
    g_transHalMod.stRegFunc[0].Trans_HalSetDataStatus = Pcie_SetDataStatus;

    return;
}

/**
 * @function:   Trans_HalSendData
 * @brief:      send data
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Trans_HalSendData(VOID *prm)
{
    TRANS_HAL_DATA_INFO *pstDataInfo = NULL;

    TRANS_HAL_CHECK_PTR(prm, err, "prm == null!");

    pstDataInfo = (TRANS_HAL_DATA_INFO *)prm;

    return g_transHalMod.stRegFunc[0].Trans_HalSendData(pstDataInfo->uiArgCnt, &pstDataInfo->u64Arg[0]);

err:
    return SAL_FAIL;
}

/**
 * @function:   Trans_HalRecvData
 * @brief:      receive data
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Trans_HalRecvData(VOID *prm)
{
    TRANS_HAL_DATA_INFO *pstDataInfo = NULL;

    TRANS_HAL_CHECK_PTR(prm, err, "prm == null!");

    pstDataInfo = (TRANS_HAL_DATA_INFO *)prm;

    return g_transHalMod.stRegFunc[0].Trans_HalRecvData(pstDataInfo->uiArgCnt, &pstDataInfo->u64Arg[0]);;

err:
    return SAL_FAIL;
}

/**
 * @function:   Trans_HalCheckDataStatus
 * @brief:      check data status
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static BOOL
 */
static BOOL Trans_HalCheckDataStatus(VOID *prm)
{
    TRANS_HAL_DATA_INFO *pstDataInfo = NULL;

    TRANS_HAL_CHECK_PTR(prm, err, "prm == null!");

    pstDataInfo = (TRANS_HAL_DATA_INFO *)prm;

    return g_transHalMod.stRegFunc[0].Trans_HalCheckDataStatus(pstDataInfo->uiArgCnt, &pstDataInfo->u64Arg[0]);

err:
    return SAL_FAIL;
}

/**
 * @function:   Trans_HalSetDataStatus
 * @brief:      set data status
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static BOOL
 */
static INT32 Trans_HalSetDataStatus(VOID *prm)
{
    TRANS_HAL_DATA_INFO *pstDataInfo = NULL;

    TRANS_HAL_CHECK_PTR(prm, err, "prm == null!");

    pstDataInfo = (TRANS_HAL_DATA_INFO *)prm;

    return g_transHalMod.stRegFunc[0].Trans_HalSetDataStatus(pstDataInfo->uiArgCnt, &pstDataInfo->u64Arg[0]);

err:
    return SAL_FAIL;
}

/**
 * @function:   Trans_HalSetDmaPhyAddr
 * @brief:      set dma phyaddr
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static UINT64
 */
static INT32 Trans_HalSetDmaPhyAddr(VOID *prm)
{
    TRANS_HAL_DATA_INFO *pstDataInfo = NULL;

    TRANS_HAL_CHECK_PTR(prm, err, "prm == null!");

    pstDataInfo = (TRANS_HAL_DATA_INFO *)prm;

    return g_transHalMod.stRegFunc[0].Trans_HalSetDmaPhyAddr(pstDataInfo->uiArgCnt, &pstDataInfo->u64Arg[0]);

err:
    return SAL_FAIL;
}

/**
 * @function:   Trans_HalGetDmaPhyAddr
 * @brief:      get dma phyaddr
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     static UINT64
 */
static INT32 Trans_HalGetDmaPhyAddr(VOID *prm)
{
    TRANS_HAL_DATA_INFO *pstDataInfo = NULL;

    TRANS_HAL_CHECK_PTR(prm, err, "prm == null!");

    pstDataInfo = (TRANS_HAL_DATA_INFO *)prm;

    return g_transHalMod.stRegFunc[0].Trans_HalGetDmaPhyAddr(pstDataInfo->uiArgCnt, &pstDataInfo->u64Arg[0]);

err:
    return SAL_FAIL;
}

/**
 * @function:   Trans_HalGetDevFd
 * @brief:      get device fd
 * @param[in]:  INT32 *piFd
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Trans_HalGetDevFd(INT32 *piFd)
{
    TRANS_HAL_CHECK_PTR(piFd, err, "piFd == null!");

    if (-1 == g_transHalMod.iDevFd)
    {
        g_transHalMod.iDevFd = g_transHalMod.stRegFunc[0].Trans_HalGetDevHandle();
    }

    *piFd = g_transHalMod.iDevFd;

    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Trans_HalInitDev
 * @brief:      init device
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Trans_HalInitDev(VOID)
{
    return g_transHalMod.stRegFunc[0].Trans_HalInitDev();
}

/**
 * @function:   Trans_HalDeinitDev
 * @brief:      deinit device
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Trans_HalDeinitDev(VOID)
{
    return g_transHalMod.stRegFunc[0].Trans_HalDeinitDev();
}

/**
 * @function:   Trans_HalRegHw
 * @brief:      注册跨芯片传输硬件类型，初始化相关变量
 * @param[in]:  TRANS_HW_TYPE_E enType
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Trans_HalRegHw(VOID *prm)
{
    TRANS_HW_TYPE_E enType = 0;

    TRANS_HAL_CHECK_PTR(prm, err, "prm == null!");

    enType = *(TRANS_HW_TYPE_E *)prm;

    switch (enType)
    {
        case USB_TYPE:
            (VOID)Trans_HalRegUsbFunc();
            break;
        case PCIE_TYPE:
            (VOID)Trans_HalRegPcieFunc();
            break;
        default:
            SAL_WARN("Invalid Hw type %d \n", enType);
            break;
    }

    SAL_INFO("Register dual-chip transmit Hw end! \n");
    return SAL_SOK;

err:
    return SAL_FAIL;
}

/**
 * @function:   Trans_HalUnRegHw
 * @brief:      反注册跨芯片传输硬件相关变量
 * @param[in]:  TRANS_HW_TYPE_E enType
 * @param[out]: None
 * @return:     VOID
 */
static INT32 Trans_HalUnRegHw(VOID)
{
    g_transHalMod.iDevFd = -1;

    g_transHalMod.stRegFunc[0].Trans_HalInitDev = NULL;
    g_transHalMod.stRegFunc[0].Trans_HalDeinitDev = NULL;
    g_transHalMod.stRegFunc[0].Trans_HalGetDevHandle = NULL;

    return SAL_SOK;
}

/**
 * @function:   Trans_HalCmdProc
 * @brief:      抽象层支持的命令字
 * @param[in]:  TRANS_HAL_PROC_CMD_E enCmd
 * @param[in]:  VOID *prm
 * @param[out]: None
 * @return:     INT32
 */
INT32 Trans_HalCmdProc(TRANS_HAL_PROC_CMD_E enCmd, VOID *prm)
{
    INT32 s32Ret = SAL_FAIL;

    switch (enCmd)
    {
        case TRANS_HAL_REGISTER:
            s32Ret = Trans_HalRegHw(prm);
            break;
        case TRANS_HAL_UNREGISTER:
            s32Ret = Trans_HalUnRegHw();
            break;
        case TRANS_HAL_INIT_DEV:
            s32Ret = Trans_HalInitDev();
            break;
        case TRANS_HAL_DEINIT_DEV:
            s32Ret = Trans_HalDeinitDev();
            break;
        case TRANS_HAL_GET_DEV_FD:
            s32Ret = Trans_HalGetDevFd(prm);
            break;
        case TRANS_HAL_SEND_DATA:
            s32Ret = Trans_HalSendData(prm);
            break;
        case TRANS_HAL_RECV_DATA:
            s32Ret = Trans_HalRecvData(prm);
            break;
        case TRANS_HAL_CHECK_DATA_STATUS:
            s32Ret = Trans_HalCheckDataStatus(prm);
            break;
        case TRANS_HAL_SET_DATA_STATUS:
            s32Ret = Trans_HalSetDataStatus(prm);
            break;
        case TRANS_HAL_GET_DMA_PHYADDR:
            s32Ret = Trans_HalGetDmaPhyAddr(prm);
            break;
        case TRANS_HAL_SET_DMA_PHYADDR:
            s32Ret = Trans_HalSetDmaPhyAddr(prm);
            break;
        default:
            SAL_WARN("Invalid cmd type! \n");
            break;
    }

    return s32Ret;
}

