/**
 * @file:   usb_transmit.c
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  usb related source file for master-slave communication
 * @author: sunzelin
 * @date    2020/7/11
 * @note:
 * @note \n History:
   1.日    期: 2020/7/11
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include "sal.h"
/* #include <platform_sdk.h> */

#include "usb_transmit.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define USB_CHECK_PTR(ptr, value) {if (!ptr) {SAL_ERROR("Ptr (The address is empty or Value is 0 )\n"); return (value); }}

#define USB_DEV_NAME "/dev/usb_zero_cdev"

#define USB_ZERO_PHYS_BLOCKED_WRITE _IOR('z', 50, struct usb_zero_param)
#define USB_ZERO_PHYS_BLOCKED_READ	_IOR('z', 51, struct usb_zero_param)
#define USB_ZERO_MMAP_BLOCKED_WRITE _IOR('z', 52, struct usb_zero_param)
#define USB_ZERO_MMAP_BLOCKED_READ	_IOR('z', 53, struct usb_zero_param)
#define USB_ZERO_STATE				_IOR('z', 54, struct usb_zero_param)

/*
 * OP_MAGIC : 魔数字
 * OP_APP   : APP 操作接口使用标志 @usb_zero_param.flags
 * OP_DSP   : DSP 操作接口使用标志 @usb_zero_param.flags
 */
#define     OP_MAGIC (0x6d6a)
#define     OP_X(x) (((OP_MAGIC) << 16) | (x))
#define     OP_APP	OP_X(0x41)
#define     OP_DSP	OP_X(0x44)

#define USB_MMAP_BUF_NUM			(3)                    /* 每种类别的mmap内存申请个数，目前统一使用3 */
#define USB_MMAP_FRM_PRM_TYPE_SIZE	(296)                  /* 传递帧参数的mmap内存类型大小 */
#define USB_MMAP_RESULT_TYPE_SIZE	(20056)                /* 传递检测结果的mmap内存类型大小 */

/*
   |0001 0001|
 |    |--------代表usb驱动传输使用的内存类别，参考USB_ADDR_TYPE_E
 |
 ||-------------代表mmap内存的用途类别，参考USB_MMAP_MEM_TYPE_NUM
 */
#define USB_MEM_TYPE_OFFSET		(0)                        /* 计算使用的内存类型偏移位数，类别参考USB_ADDR_TYPE_E */
#define USB_MEM_TYPE_MASK		(0x3)
#define USB_MMAP_TYPE_OFFSET	(4)                        /* 计算使用的mmap内存类别的偏移位数 */
#define USB_MMAP_TYPE_MASK		(0x1)


/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
struct usb_zero_param
{
    UINT64 addr;            /* !< addr 用于物理地址传输时传入物理地址或mmap映射传输时传入映射序号 */
    UINT64 len;             /* !< len 传输长度 */
    UINT32 flags;           /* !< flags 用于区分 DSP 和 APP 操作 */
    UINT32 state;           /* !< flags 用于usb-zero状态 */
#define USB_ZEOR_EP_ENABLE (1 << 0)
};


typedef enum _USB_MMAP_MEM_TYPE_
{
    USB_MMAP_MEM_FRM_PRM_TYPE = 0,                    /* 用于传输帧参数的mmap内存类型 */
    USB_MMAP_MEM_RESULT_TYPE,                         /* 用于传输检测结果的mmap内存类型 */
    USB_MMAP_MEM_TYPE_NUM,
} USB_MMAP_MEM_TYPE_E;

typedef enum _USB_ADDR_TYPE_
{
    MMAP_TYPE = 0,
    PHY_TYPE,
    USB_ADDR_NUM,
} USB_ADDR_TYPE_E;

typedef enum _USB_TRANS_TYPE_
{
    USB_MMAP_SEND_TYPE = 0,                           /* 数据发送 */
    USB_MMAP_RECV_TYPE,                               /* 数据接收 */
    USB_MMAP_PROC_TYPE_NUM,
} USB_TRANS_TYPE_E;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/
static INT32 g_UsbFd = -1;

static UINT32 uiMmapMemIdx[USB_MMAP_MEM_TYPE_NUM] = {0};
static VOID *pMmapMem[USB_MMAP_MEM_TYPE_NUM][USB_MMAP_BUF_NUM] = {NULL};

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/**
 * @function:   Usb_GetDevHandle
 * @brief:      Get Usb Device Description
 * @param[in]   void
 * @param[out]: None
 * @return      INT32
 */
INT32 Usb_GetDevHandle(VOID)
{
    return g_UsbFd;
}

/**
 * @function:   Usb_GetFreeMmapAddr
 * @brief:      获取空闲mmap内存地址用于传输
 * @param[in]:  UINT64 u64Arg
 * @param[out]: None
 * @return:     static UINT64
 */
static UINT64 Usb_GetFreeMmapAddr(UINT64 u64Arg)
{
    UINT32 uiMmapType = 0;
    UINT64 u64MmapAddr = 0;

    uiMmapType = (u64Arg >> USB_MMAP_TYPE_OFFSET) & USB_MMAP_TYPE_MASK;

    u64MmapAddr = (UINT64)(pMmapMem[uiMmapType][uiMmapMemIdx[uiMmapType]]);
    uiMmapMemIdx[uiMmapType] = (uiMmapMemIdx[uiMmapType] + 1) % USB_MMAP_BUF_NUM;

    return u64MmapAddr;
}

/**
 * @function:   Usb_SendData
 * @brief:      send data via usb device
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_SendData(UINT32 uiArgCnt, UINT64 *pArg)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uiMemType = 0;

    struct usb_zero_param param;

    if (3 != uiArgCnt)
    {
        SAL_ERROR("Invalid arg! arg_cnt %d \n", uiArgCnt);
        return SAL_FAIL;
    }

    uiMemType = ((UINT64)pArg[2] >> USB_MEM_TYPE_OFFSET) & USB_MEM_TYPE_MASK;

    param.len = (UINT64)pArg[0];
    param.flags = OP_DSP;

    switch (uiMemType)
    {
        case MMAP_TYPE:
        {
            param.addr = Usb_GetFreeMmapAddr((UINT64)pArg[2]);
            memcpy((CHAR *)param.addr, (CHAR *)pArg[1], param.len);   /* 将外部数据拷贝到内部mmap内存 */
            s32Ret = ioctl(g_UsbFd, USB_ZERO_MMAP_BLOCKED_WRITE, &param);
            break;
        }
        case PHY_TYPE:
        {
            param.addr = (UINT64)pArg[1];
            s32Ret = ioctl(g_UsbFd, USB_ZERO_PHYS_BLOCKED_WRITE, &param);
            break;
        }
        default:
        {
            SAL_WARN("Invalid type %d \n", uiMemType);
            break;
        }
    }

    return s32Ret;
}

/**
 * @function:   Usb_CheckDataStatus
 * @brief:      check if data is ready
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     BOOL
 */
BOOL Usb_CheckDataStatus(UINT32 uiArgCnt, UINT64 *pArg)
{
    return SAL_TRUE;   /* USB阻塞式传输，故无需检查是否数据状态已经OK，此处与PCIE进行兼容处理 */
}

/**
 * @function:   Usb_SetDataStatus
 * @brief:      设置数据状态
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_SetDataStatus(UINT32 uiArgCnt, UINT64 *pArg)
{
    return SAL_SOK;
}

/**
 * @function:   Usb_SetDmaPhyAddr
 * @brief:      设置DMA传输的物理地址(当前使用mmap内存在片间传输)
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     BOOL
 */
INT32 Usb_SetDmaPhyAddr(UINT32 uiArgCnt, UINT64 *pArg)
{
    return SAL_SOK;   /* 当前USB未使用该接口，为了代码一致性增加 */
}

/**
 * @function:   Usb_GetDmaPhyAddr
 * @brief:      获取DMA传输的物理地址(当前使用mmap内存在片间传输)
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_GetDmaPhyAddr(UINT32 uiArgCnt, UINT64 *pArg)
{
    return SAL_SOK;    /* 当前USB未使用该接口，为了代码一致性增加 */
}

/**
 * @function:   Usb_RecvData
 * @brief:      receive data via usb device
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_RecvData(UINT32 uiArgCnt, UINT64 *pArg)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 uiMemType = 0;

    struct usb_zero_param param = {0};

    if (3 != uiArgCnt)
    {
        SAL_ERROR("Invalid arg! arg_cnt %d \n", uiArgCnt);
        return SAL_FAIL;
    }

    uiMemType = ((UINT64)pArg[2] >> USB_MEM_TYPE_OFFSET) & USB_MEM_TYPE_MASK;

    param.len = (UINT64)pArg[0];
    param.flags = OP_DSP;

    switch (uiMemType)
    {
        case MMAP_TYPE:
            param.addr = Usb_GetFreeMmapAddr((UINT64)pArg[2]);
            s32Ret = ioctl(g_UsbFd, USB_ZERO_MMAP_BLOCKED_READ, &param);
            memcpy((CHAR *)pArg[1], (CHAR *)param.addr, param.len);    /* 将传输数据从mmap内存拷贝到外部内存 */
            break;
        case PHY_TYPE:
            param.addr = pArg[1];
            s32Ret = ioctl(g_UsbFd, USB_ZERO_PHYS_BLOCKED_READ, &param);
            break;
        default:
            SAL_WARN("Invalid type %d \n", uiMemType);
            break;
    }

    return s32Ret;
}

/**
 * @function:   Usb_MmapMemFree
 * @brief:      free mmap mem
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Usb_MmapMemFree(VOID)
{
    UINT32 i = 0;
    UINT32 j = 0;

    UINT32 auiMemSize[USB_MMAP_MEM_TYPE_NUM] = {USB_MMAP_FRM_PRM_TYPE_SIZE, USB_MMAP_RESULT_TYPE_SIZE};

    for (i = 0; i < USB_MMAP_MEM_TYPE_NUM; i++)
    {
        for (j = 0; j < USB_MMAP_BUF_NUM; j++)
        {
            munmap(pMmapMem[i][j], auiMemSize[i]);
            pMmapMem[i][j] = NULL;
        }
    }

    SAL_WARN("mmap mem free End! \n");
    return SAL_SOK;
}

/**
 * @function:   Usb_MmapMemAlloc
 * @brief:      alloc mmap mem
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Usb_MmapMemAlloc(VOID)
{
    UINT32 i = 0;
    UINT32 j = 0;

    UINT32 auiMemSize[USB_MMAP_MEM_TYPE_NUM] = {USB_MMAP_FRM_PRM_TYPE_SIZE, USB_MMAP_RESULT_TYPE_SIZE};

    for (i = 0; i < USB_MMAP_MEM_TYPE_NUM; i++)
    {
        for (j = 0; j < USB_MMAP_BUF_NUM; j++)
        {
            pMmapMem[i][j] = mmap(NULL, auiMemSize[i], PROT_READ | PROT_WRITE, MAP_SHARED, g_UsbFd, 0);
            if (MAP_FAILED == pMmapMem[i][j])
            {
                SAL_ERROR("mmap failed! \n");
            }
        }
    }

    SAL_WARN("mmap mem alloc End! \n");
    return SAL_SOK;
}

/**
 * @function:   Usb_CloseDev
 * @brief:      close usb device
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Usb_CloseDev(VOID)
{
    if (g_UsbFd > 0)
    {
        close(g_UsbFd);
        g_UsbFd = -1;
    }

    SAL_INFO("close usb dev fd %d End! \n", g_UsbFd);
    return SAL_SOK;
}

/**
 * @function:   Usb_DevCheckState
 * @brief:      check device state
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_DevCheckState(VOID)
{
    INT32 s32Ret = SAL_SOK;
    struct usb_zero_param param = {0};

    param.flags = OP_DSP;

    while (1)
    {
        s32Ret = ioctl(g_UsbFd, USB_ZERO_STATE, &param);
        if (s32Ret == 0 && (param.state & USB_ZEOR_EP_ENABLE))
        {
            printf("check usb-zero state OK!!!\n");
            return SAL_SOK;
        }
        else
            printf("usb state error! s32Ret %d, param.state %d\n", s32Ret, param.state);

        sleep(1);
    }

    return SAL_FAIL;
}

/**
 * @function:   Usb_OpenDev
 * @brief:      open usb device
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Usb_OpenDev(VOID)
{
    INT32 s32Ret = SAL_SOK;
    INT32 DevOpenFlag = SAL_TRUE;

    g_UsbFd = open(USB_DEV_NAME, O_RDWR);
    if (g_UsbFd < 0)
    {
        SAL_ERROR("open dev %s failed!\n", USB_DEV_NAME);
        while (DevOpenFlag)
        {
            sleep(1);
            g_UsbFd = open(USB_DEV_NAME, O_RDWR);
            if (g_UsbFd < 0)
            {
                SAL_ERROR("open dev %s failed!\n", USB_DEV_NAME);
            }
            else
            {
                SAL_INFO("open usb dev success!\n");
                DevOpenFlag = 0;
            }
        }
    }

    s32Ret = Usb_DevCheckState();
    if (SAL_SOK == s32Ret)
    {
        printf("check usb-zero state end!!!\n");
    }

    SAL_INFO("open usb dev fd %d End! \n", g_UsbFd);
    return SAL_SOK;
}

/**
 * @function:   Usb_DevDeInit
 * @brief:      usb device deinit
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_DevDeInit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = Usb_CloseDev();
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("close usb device failed! \n");
        return SAL_FAIL;
    }

    s32Ret = Usb_MmapMemFree();
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("free usb mmap mem failed! \n");
        return SAL_FAIL;
    }

    SAL_WARN("deinit usb dev fd %d End! \n", g_UsbFd);
    return SAL_SOK;
}

/**
 * @function:   Usb_DevDeInit
 * @brief:      usb device init
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_DevInit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = Usb_OpenDev();
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("open usb device failed! \n");
        return SAL_FAIL;
    }

    s32Ret = Usb_MmapMemAlloc();
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("alloc usb mmap mem failed! \n");
        return SAL_FAIL;
    }

    SAL_WARN("init usb dev fd %d End! \n", g_UsbFd);
    return SAL_SOK;
}

