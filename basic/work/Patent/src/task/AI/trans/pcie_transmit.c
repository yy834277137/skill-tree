/**
 * @file:   pcie_transmit.c
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  pcie related source file for master-slave communication
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
#include "pci_shm.h"
#include "pci_dma.h"
#include "pcie_transmit.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define PCIE_DMA_DEV_NAME	("/dev/pci-dma")              /* TODO: 未定，暂时预留 */
#define PCIE_SHM_DEV_NAME	("/dev/pci-shm")              /* TODO: 未定，暂时预留 */

#define PCIE_CHECK_MASK				(0x3)                 /* 掩码，目前仅使用最低位，3是为了规避sd coverity */
#define PCIE_DATA_MEM_TYPE_OFFSET	(0)
#define PCIE_MMAP_TYPE_OFFSET		(4)

#define PCIE_MMAP_BUF_CNT	(1)                           /* 当前mmap内存块个数，消息和结果各三个 */
#define PCIE_WAIT_MSG_CNT	(100)

/* 底层相关宏定义(相对固定的参数)，主要为目前BSP分配给DSP的bar空间信息。DSP在拿到的两块内存会进行二次分配 */
#define PCIE_SHARE_MEM_BLOCK_CNT	(2)                   /* 目前分配给DSP的BAR空间只有两块，分别是256KB，参考shm_type_t */
#define PCIE_SHARE_MEM_SIZE			(0x40000)             /* 256KB */
#define PCIE_MMAP_BAR0_SIZE			(PCIE_SHARE_MEM_SIZE) /* BAR0 空间大小 */
#define PCIE_MMAP_BAR1_SIZE			(PCIE_SHARE_MEM_SIZE) /* BAR1 空间大小 */

/* 两块BAR空间重新分配的相关宏定义 */
#define PCIE_BAR0_FRM_PRM_OFFSET	(0)                   /* BAR0使用划分: 主->从的物理地址 + 主->从的帧参数 */
#define PCIE_BAR0_FRM_PRM_OFFSET_1	(128 * 1024)          /* BAR0的第二半用于存放主->从的物理地址 */
#define PCIE_BAR1_RSLT_OFFSET		(0)                   /* BAR1使用划分: 从->主目标结果参数 */

#define PCIE_PHY_ADDR_SIZE	(PCIE_SHARE_MEM_SIZE / 2)
#define PCIE_FRM_PRM_SIZE	(PCIE_SHARE_MEM_SIZE / 2)
#define PCIE_RLST_SIZE		(PCIE_SHARE_MEM_SIZE)

/* 共享内存传输的数据类型大小，Byte */
#define PCIE_MMAP_FRM_PRM_DATA_SIZE		(296)             /* 帧参数大小 */
#define PCIE_MMAP_FRM_PRM_DATA_SIZE_1	(sizeof(UINT64))  /* 物理地址大小 */

#define PCIE_MMAP_RSLT_DATA_SIZE (20056)                  /* 智能结果结构体大小 */

#define HI3559AV100_DEV_ID (0x355919e5)                   /*bsp提供*/
#define RK3588_DEV_ID (0x35881d87) 


#define BUS_ID0 3
#define BUS_ID1 5

/* 外部数据结构，BSP确定的结构体 */

#if 0
#ifndef pci_dma_addr_t
typedef UINT64 pci_dma_addr_t;
#endif

typedef enum
{
    PCI_ADDR_USER,           /* TODO: 这个类型还能用吗? */
    PCI_ADDR_PHYS,
    PCI_ADDR_MAX,
} pci_addr_type_t;

/*
   typedef enum
   {
    PCI_DMA_DIR_READ,
    PCI_DMA_DIR_WRITE,
   } pci_dir_type_t;
 */

/*
   typedef struct pci_dma_arg
   {
    pci_dma_addr_t src;
    pci_dma_addr_t dst;
    UINT64 length;
    pci_addr_type_t type;
    pci_dir_type_t dir;
    uint32_t		id;
    UINT32 reserved[32];
   } pci_dma_arg_t;
 */
#define PCI_DMA_BASE	'D'
#define PCI_DMA_START	_IOWR(PCI_DMA_BASE, 60, pci_dma_arg_t *)
#endif

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
/* 外部数据结构，BSP确定的结构体 */

/*
   typedef enum
   {
    SHM_APP_0 = (0 * 4096),
    SHM_APP_1 = (1 * 4096),
    SHM_DSP_0 = (2 * 4096),
    SHM_DSP_1 = (3 * 4096),
   } shm_type_t;
 */

/* yuv数据传输方式类别 */
typedef enum _PCIE_MEM_TYPE_
{
    PCIE_MMAP_TYPE = 0,                    /* mmap映射内存 */
    PCIE_PHY_TYPE,                         /* 物理地址DMA */
    PCIE_MEM_TYPE_NUM,
} PCIE_MEM_TYPE_E;

/* mmap映射内存使用方式类别 */
typedef enum _PCIE_MMAP_TYPE_
{
    PCIE_MMAP_FRM_PRM_TYPE = 0,           /* 帧参数(主->从) */
    PCIE_MMAP_RSLT_TYPE,                  /* 智能结果数据(从->主) */
    PCIE_MMAP_PHYADDR_TYPE,
	PCIE_MMAP_RSLT_TYPE_1,

    PCIE_MMAP_TYPE_NUM,
} PCIE_MMAP_TYPE_E;

typedef struct _PCIE_MSG_HEAD_
{
    UINT16 bUsed;
    UINT16 uiType;
    UINT32 uiDataLen;
} PCIE_MSG_HEAD_S;

typedef struct _PCIE_MMAP_MEM_PRM_
{
    BOOL bUsed;                            /* 使用标志位 */
    VOID *pVir;                            /* 映射后的虚拟地址 */
} PCIE_MMAP_MEM_PRM_S;

#if 1  /* 统计耗时相关 */
#define PCIE_TIME_DEBUG_MEM_TYPE_NUM	(2)         /* 统计耗时的内存类型 */
#define PCIE_TIME_DEBUG_SUB_TYPE_NUM	(2)         /* 每种内存统计的子类别 */
typedef struct _PCIE_TIME_DEBUG_PRM_
{
    UINT32 u32Idx;                         /* 指向记录耗时的数组索引 */
    UINT32 u32SubIdx;

    CHAR acPrStr[PCIE_TIME_DEBUG_MEM_TYPE_NUM][64];
    UINT32 u32CalCnt[PCIE_TIME_DEBUG_MEM_TYPE_NUM];                      /* 当前统计的次数 */
    UINT32 au32TimeCost[PCIE_TIME_DEBUG_MEM_TYPE_NUM][PCIE_TIME_DEBUG_SUB_TYPE_NUM];               /* 耗时统计数组 */
} PCIE_TIME_DEBUG_PRM;

PCIE_TIME_DEBUG_PRM stSendTimeDbg = {0};
PCIE_TIME_DEBUG_PRM stRecvTimeDbg = {0};

static BOOL bPcieShowPrtInfo = SAL_FALSE;

/**
 * @function    Pcie_SetPrtFlag
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
VOID Pcie_SetPrtFlag(BOOL bFlag)
{
    bPcieShowPrtInfo = bFlag;
    return;
}

/**
 * @function    Pcie_GetPrtFlag
 * @brief
 * @param[in]
 * @param[out]
 * @return
 */
static BOOL Pcie_GetPrtFlag(VOID)
{
    return bPcieShowPrtInfo;
}

/**
 * @function:   Pcie_GetDbgTime
 * @brief:      累加统计耗时
 * @param[in]   UINT32 time_s
 * @param[in]   UINT32 time_e
 * @param[out]: PCIE_TIME_DEBUG_PRM *pstTimeDbg
 * @return      VOID
 */
static VOID Pcie_GetDbgTime(UINT32 time_s, UINT32 time_e, PCIE_TIME_DEBUG_PRM *pstTimeDbg)
{
    pstTimeDbg->au32TimeCost[pstTimeDbg->u32Idx][pstTimeDbg->u32SubIdx] += (time_e - time_s);

    return;
}

/**
 * @function:   Pcie_FillPrStr
 * @brief:      填充打印的字符串
 * @param[in]   CHAR *pcStr
 * @param[out]: PCIE_TIME_DEBUG_PRM *pstTimeDbg
 * @return      VOID
 */
static VOID Pcie_FillPrStr(CHAR *pcStr, PCIE_TIME_DEBUG_PRM *pstTimeDbg)
{
    /* 调试接口入参外部保证，此处不增加校验，下同 */
    memset(pstTimeDbg->acPrStr[pstTimeDbg->u32Idx], 0x00, 64);
    snprintf(pstTimeDbg->acPrStr[pstTimeDbg->u32Idx], 64, "%s", pcStr);

    return;
}

/**
 * @function:   Pcie_AddDbgCnt
 * @brief:      统计次数自加
 * @param[out]: PCIE_TIME_DEBUG_PRM *pstTimeDbg
 * @return      VOID
 */
static VOID Pcie_AddDbgCnt(PCIE_TIME_DEBUG_PRM *pstTimeDbg)
{
    pstTimeDbg->u32CalCnt[pstTimeDbg->u32Idx]++;

    return;
}

/**
 * @function:   Pcie_PrDbgTimeInfo
 * @brief:      打印统计耗时
 * @param[in]   UINT32 u32PrGap
 * @param[out]: PCIE_TIME_DEBUG_PRM *pstTimeDbg
 * @return      VOID
 */
static VOID Pcie_PrDbgTimeInfo(UINT32 u32PrGap, PCIE_TIME_DEBUG_PRM *pstTimeDbg)
{
    if ((SAL_TRUE != Pcie_GetPrtFlag()) || (pstTimeDbg->u32CalCnt[pstTimeDbg->u32Idx] < u32PrGap))
    {
        return;
    }

    SAL_WARN("%s: [%d, %d] \n",
             pstTimeDbg->acPrStr[pstTimeDbg->u32Idx],
             pstTimeDbg->au32TimeCost[pstTimeDbg->u32Idx][0] / pstTimeDbg->u32CalCnt[pstTimeDbg->u32Idx],
             pstTimeDbg->au32TimeCost[pstTimeDbg->u32Idx][1] / pstTimeDbg->u32CalCnt[pstTimeDbg->u32Idx]);

    pstTimeDbg->u32CalCnt[pstTimeDbg->u32Idx] = 0;
    memset(pstTimeDbg->au32TimeCost[pstTimeDbg->u32Idx], 0x00, sizeof(UINT32) * PCIE_TIME_DEBUG_SUB_TYPE_NUM);

    return;
}

#endif

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/
static INT32 g_PcieDmaFd = -1;
static INT32 g_PcieShmFd = -1;
/* pcie设备信息 */
static pci_device_info gstPcieDevInfo = {0};

static VOID *pBarVirAddr[PCIE_SHARE_MEM_BLOCK_CNT] = {NULL};
static UINT32 auiBarVirSize[PCIE_SHARE_MEM_BLOCK_CNT] = {PCIE_MMAP_BAR0_SIZE, PCIE_MMAP_BAR1_SIZE};
/* static shm_type_t enBarMemTypeTab[PCIE_SHARE_MEM_BLOCK_CNT] = {SHM_DSP_0, SHM_DSP_1}; */

/* 消息只用于DMA传输，大小为物理地址大小；结果用于算法结果传输 */
static UINT32 auiMmapMemDataSize[PCIE_MMAP_TYPE_NUM] = {PCIE_MMAP_FRM_PRM_DATA_SIZE, PCIE_MMAP_RSLT_DATA_SIZE, PCIE_MMAP_FRM_PRM_DATA_SIZE_1, PCIE_MMAP_RSLT_DATA_SIZE};

/* PCIE内部使用的内存块，根据BSP分配过来的两块BAR空间进行二次分配 */
static PCIE_MMAP_MEM_PRM_S stMmapMemTab[PCIE_MMAP_TYPE_NUM] = {0};
static UINT32 auiMmapMemOffsetTab[PCIE_MMAP_TYPE_NUM] = {PCIE_BAR0_FRM_PRM_OFFSET, PCIE_BAR1_RSLT_OFFSET, PCIE_BAR0_FRM_PRM_OFFSET_1, PCIE_BAR0_FRM_PRM_OFFSET_1};

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/**
 * @function:   Pcie_GetDevHandle
 * @brief:      Get Pcie Device Description
 * @param[in]   void
 * @param[out]: None
 * @return      INT32
 */
INT32 Pcie_GetDevHandle(void)
{
    return g_PcieDmaFd;
}

/**
 * @function   Pcie_getDevBusId
 * @brief      获取pcie从片类型
 * @param[in]  unsigned int *busId  获取ID
 * @param[in]  unsigned int type    从片类型
 * @param[out] None
 * @return     static INT32
 */
static INT32 Pcie_getDevBusId(unsigned int *busId, unsigned int type)
{
    INT32 s32Ret = SAL_FAIL;
    int i = 0;

    for (i = 0; i < gstPcieDevInfo.num; i++)
    {
        if (RK3588_DEV_ID == gstPcieDevInfo.device_info[i].dev_type
			||HI3559AV100_DEV_ID == gstPcieDevInfo.device_info[i].dev_type)   /*多个从片需要重新修改，当前临时调试只适应单从片*/
        {
            *busId = gstPcieDevInfo.device_info[i].id;
            s32Ret = SAL_SOK;
            break;
        }
    }

    return s32Ret;
}

/**
 * @function:   Pcie_GetMsgHead
 * @brief:      获取消息头
 * @param[in]:  PCIE_MMAP_MEM_PRM_S *pstMmapMem
 * @param[in]:  PCIE_MSG_HEAD_S *pstMsgHead
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Pcie_GetMsgHead(PCIE_MMAP_MEM_PRM_S *pstMmapMem, PCIE_MSG_HEAD_S *pstMsgHead)
{
    UINT32 uiOffset = 0;

    if (!pstMmapMem || !pstMsgHead)
    {
        SAL_ERROR("ptr == null! %p, %p \n", pstMmapMem, pstMsgHead);
        return SAL_FAIL;
    }

    memcpy(pstMsgHead, (CHAR *)pstMmapMem->pVir + uiOffset, sizeof(PCIE_MSG_HEAD_S));
    uiOffset += sizeof(PCIE_MSG_HEAD_S);

    return SAL_SOK;
}

/**
 * @function:   Pcie_GetFreeMmap
 * @brief:      获取空闲mmap内存地址用于传输
 * @param[in]:  PCIE_MMAP_TYPE_E enMmapMemType
 * @param[in]:  PCIE_MMAP_MEM_PRM_S **ppstMmapMem
 * @param[in]:  INT32 iTimeOutCnt
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 ATTRIBUTE_UNUSED Pcie_GetFreeMmap(PCIE_MMAP_TYPE_E enMmapMemType,
                                               PCIE_MMAP_MEM_PRM_S **ppstMmapMem,
                                               BOOL bBufSts,
                                               INT32 iTimeOutCnt,
                                               PCIE_TIME_DEBUG_PRM *pstTimeDbg)
{
    PCIE_MSG_HEAD_S stMsgHead = {0};

    UINT32 time0 = SAL_getCurMs();

    do
    {
        (VOID)Pcie_GetMsgHead(&stMmapMemTab[enMmapMemType], &stMsgHead);

        /* 校验消息头信息是否正确 */
        if (stMsgHead.uiType != enMmapMemType)
        {
            SAL_ERROR("wrong mmap mem type! %d, %d \n", enMmapMemType, stMsgHead.uiType);
            return SAL_FAIL;
        }

        if (bBufSts != stMsgHead.bUsed || stMsgHead.uiDataLen == 0)
        {
            /* 若时间计数=-1，会进入死循环，阻塞处理 */
            if (iTimeOutCnt > 0)
            {
                iTimeOutCnt--;
            }

            usleep(1 * 1000);    /* 若没有获取到，休眠5ms */
            continue;
        }

        break;

        /* TODO: 添加调试信息，用于定位是否死循环 */



    }
    while (iTimeOutCnt != 0);

    if (0 == iTimeOutCnt)
    {
        SAL_ERROR("loop %d times and failed to get free mmap mem! Used %d, DataLen %d\n", iTimeOutCnt, stMsgHead.bUsed, stMsgHead.uiDataLen);
        return SAL_FAIL;
    }

    UINT32 time1 = SAL_getCurMs();

    (VOID)Pcie_GetDbgTime(time0, time1, pstTimeDbg);

    SAL_LOGD("get msg end! cost %d \n", PCIE_WAIT_MSG_CNT - iTimeOutCnt);

    *ppstMmapMem = &stMmapMemTab[enMmapMemType];
    return SAL_SOK;
}

/**
 * @function:   Pcie_FillMsgHead
 * @brief:      填充消息数据头
 * @param[in]:  PCIE_MMAP_TYPE_E enType
 * @param[in]:  PCIE_MSG_HEAD_S *pstMsgHead
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Pcie_FillMsgHead(PCIE_MMAP_TYPE_E enType, UINT16 uFlag, PCIE_MSG_HEAD_S *pstMsgHead)
{
    if (!pstMsgHead)
    {
        SAL_ERROR("pstMsgHead == null! \n");
        return SAL_FAIL;
    }

    pstMsgHead->bUsed = uFlag;
    pstMsgHead->uiType = enType;
    pstMsgHead->uiDataLen = auiMmapMemDataSize[enType];

    return SAL_SOK;
}

/**
 * @function:   Pcie_SetDataStatus
 * @brief:      设置数据状态
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Pcie_SetDataStatus(UINT32 uiArgCnt, UINT64 *pArg)
{
    INT32 s32Ret = SAL_FAIL;

    BOOL bDataStatus = SAL_FALSE;
    PCIE_MMAP_TYPE_E enMmapMemType = PCIE_MMAP_TYPE_NUM;

    PCIE_MSG_HEAD_S stMsgHead = {0};

    if (2 != uiArgCnt)
    {
        SAL_ERROR("invalid arg cnt %d \n", uiArgCnt);
        goto err;
    }

    enMmapMemType = (PCIE_MMAP_TYPE_E)pArg[0];
    bDataStatus = (BOOL)pArg[1];

    s32Ret = Pcie_FillMsgHead(enMmapMemType, bDataStatus, &stMsgHead);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("fill msg head failed! \n");
        goto err;
    }

    memcpy((CHAR *)stMmapMemTab[enMmapMemType].pVir, &stMsgHead, sizeof(PCIE_MSG_HEAD_S));
    return SAL_SOK;

err:
    return s32Ret;
}

/**
 * @function:   Pcie_SetDmaPhyAddr
 * @brief:      设置DMA传输的物理地址(当前使用mmap内存在片间传输)
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     BOOL
 */
INT32 Pcie_SetDmaPhyAddr(UINT32 uiArgCnt, UINT64 *pArg)
{
    PCIE_MMAP_TYPE_E enMmapMemType = PCIE_MMAP_TYPE_NUM;

    if (2 != uiArgCnt)
    {
        SAL_ERROR("%s: invalid arg cnt %d \n", __FUNCTION__, uiArgCnt);
        return SAL_FAIL;
    }

    enMmapMemType = (PCIE_MMAP_TYPE_E)pArg[0];
    memcpy((CHAR *)stMmapMemTab[enMmapMemType].pVir + sizeof(PCIE_MSG_HEAD_S), &pArg[1], sizeof(UINT64));

#if 0
    SAL_WARN("pcie: set dma phy addr %p mmap %d \n", (VOID *)pArg[1], enMmapMemType);
#endif
    return SAL_SOK;
}

/**
 * @function:   Pcie_GetDmaPhyAddr
 * @brief:      获取DMA传输的物理地址(当前使用mmap内存在片间传输)
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     BOOL
 */
INT32 Pcie_GetDmaPhyAddr(UINT32 uiArgCnt, UINT64 *pArg)
{
    UINT64 u64PhyAddr = 0;
    PCIE_MMAP_TYPE_E enMmapMemType = PCIE_MMAP_TYPE_NUM;

    if (2 != uiArgCnt)
    {
        SAL_ERROR("%s: invalid arg cnt %d \n", __FUNCTION__, uiArgCnt);
        return SAL_FAIL;
    }

    enMmapMemType = (PCIE_MMAP_TYPE_E)pArg[0];
    u64PhyAddr = *(UINT64 *)((CHAR *)stMmapMemTab[enMmapMemType].pVir + sizeof(PCIE_MSG_HEAD_S));

    pArg[1] = u64PhyAddr;
    return SAL_SOK;
}

/**
 * @function:   Pcie_CheckDataStatus
 * @brief:      确认当前芯片间缓存状态是否就绪
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     BOOL
 */
BOOL Pcie_CheckDataStatus(UINT32 uiArgCnt, UINT64 *pArg)
{
    BOOL bCheckRslt = SAL_FALSE;
    BOOL bBufSts = SAL_FALSE;
    INT32 iTimeOutCnt = 0;
    PCIE_MMAP_TYPE_E enMmapMemType = PCIE_MMAP_TYPE_NUM;

    PCIE_MSG_HEAD_S stMsgHead = {0};

	if (g_PcieShmFd < 0)
    {
        SAL_ERROR("open dev %s failed!\n", PCIE_SHM_DEV_NAME);
        return SAL_FAIL;
    }

    if (3 != uiArgCnt)
    {
        SAL_ERROR("invalid input arg cnt %d \n", uiArgCnt);

        bCheckRslt = SAL_FALSE;
        goto exit;
    }

    enMmapMemType = (PCIE_MMAP_TYPE_E)pArg[0];
    bBufSts = (BOOL)pArg[1];
    iTimeOutCnt = (INT32)pArg[2];

    do
    {
        (VOID)Pcie_GetMsgHead(&stMmapMemTab[enMmapMemType], &stMsgHead);

        /* 校验消息头信息是否正确 */
        if (stMsgHead.uiType != enMmapMemType)
        {
            SAL_ERROR("wrong mmap mem type! %d, %d \n", enMmapMemType, stMsgHead.uiType);
            goto exit;
        }

        if (bBufSts != stMsgHead.bUsed || stMsgHead.uiDataLen == 0)
        {
            /* 若时间计数=-1，会进入死循环，阻塞处理 */
            if (iTimeOutCnt > 0)
            {
                iTimeOutCnt--;
            }

            usleep(1 * 1000);    /* 若没有获取到，休眠5ms */
            continue;
        }

        bCheckRslt = SAL_TRUE;
        break;

        /* TODO: 添加调试信息，用于定位是否死循环 */



    }
    while (iTimeOutCnt != 0);

    if (0 == iTimeOutCnt)
    {
        bCheckRslt = SAL_FALSE;

        SAL_ERROR("loop %d times and failed to get free mmap mem! Used %d, DataLen %d\n", iTimeOutCnt, stMsgHead.bUsed, stMsgHead.uiDataLen);
        goto exit;
    }

exit:
    return bCheckRslt;
}

/**
 * @function:   Pcie_SendData
 * @brief:      send data via pcie device
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Pcie_SendData(UINT32 uiArgCnt, UINT64 *pArg)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time11 = 0;
    UINT32 time12 = 0;
    UINT32 time2 = 0;

    UINT32 uiOffset = 0;
    UINT32 uiMemType = 0;
    PCIE_MMAP_TYPE_E enMmapMemType = 0;
    UINT64 u64DataLen = 0;
    UINT64 u64Addr = 0;
    UINT64 u64DstAddr = 0;
    UINT32 u32busId = 0;

    pci_dma_arg_t stDmaPrm = {0};

    if (4 != uiArgCnt)
    {
        SAL_ERROR("Invalid arg! arg_cnt %d \n", uiArgCnt);
        return SAL_FAIL;
    }

    if (SAL_SOK != Pcie_getDevBusId(&u32busId, 0))
    {
        u32busId = BUS_ID0;
    }

    u64DataLen = (UINT64)pArg[0];

    u64Addr = (UINT64)pArg[1];
    uiMemType = ((UINT64)pArg[2] >> PCIE_DATA_MEM_TYPE_OFFSET) & PCIE_CHECK_MASK;
    enMmapMemType = ((UINT64)pArg[2] >> PCIE_MMAP_TYPE_OFFSET) & PCIE_CHECK_MASK;
    u64DstAddr = (UINT64)pArg[3];

    time0 = SAL_getCurMs();

    switch (uiMemType)
    {
        case PCIE_MMAP_TYPE:
        {
            uiOffset += sizeof(PCIE_MSG_HEAD_S);

            time11 = SAL_getCurMs();

            sal_memcpy_s((CHAR *)stMmapMemTab[enMmapMemType].pVir + uiOffset, u64DataLen, (CHAR *)u64Addr, u64DataLen);       /* 将外部数据拷贝到内部mmap内存 */

            time1 = SAL_getCurMs();

            s32Ret = SAL_SOK;

            if (SAL_SOK == s32Ret)
            {
                stSendTimeDbg.u32Idx = PCIE_MMAP_TYPE;
                Pcie_FillPrStr("send---mmap", &stSendTimeDbg);

                Pcie_AddDbgCnt(&stSendTimeDbg);

                stSendTimeDbg.u32SubIdx = 0;
                Pcie_GetDbgTime(time0, time11, &stSendTimeDbg);

                stSendTimeDbg.u32SubIdx = 1;
                Pcie_GetDbgTime(time11, time1, &stSendTimeDbg);

                Pcie_PrDbgTimeInfo(60, &stSendTimeDbg);
            }

            break;
        }

        case PCIE_PHY_TYPE:
        {
            time12 = SAL_getCurMs();

            stDmaPrm.src = u64Addr;
            stDmaPrm.dst = u64DstAddr;
            stDmaPrm.length = u64DataLen;
            stDmaPrm.type = PCI_ADDR_PHYS;
            stDmaPrm.dir = PCI_DMA_WRITE;
            stDmaPrm.id = u32busId;
            s32Ret = ioctl(g_PcieDmaFd, PCI_DMA_START, &stDmaPrm);
            if (SAL_SOK != s32Ret)
            {
                SAL_ERROR("reset msg head \n");

            }

            time2 = SAL_getCurMs();

            if (SAL_SOK == s32Ret)
            {
                stSendTimeDbg.u32Idx = PCIE_PHY_TYPE;
                Pcie_FillPrStr("send---phy", &stSendTimeDbg);

                Pcie_AddDbgCnt(&stSendTimeDbg);

                stSendTimeDbg.u32SubIdx = 0;
                Pcie_GetDbgTime(time0, time12, &stSendTimeDbg);

                stSendTimeDbg.u32SubIdx = 1;
                Pcie_GetDbgTime(time12, time2, &stSendTimeDbg);

                Pcie_PrDbgTimeInfo(60, &stSendTimeDbg);
            }

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
 * @function:   Pcie_RecvData
 * @brief:      receive data via pcie device
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Pcie_RecvData(UINT32 uiArgCnt, UINT64 *pArg)
{
    INT32 s32Ret = SAL_FAIL;

    UINT32 time0 = 0;
    UINT32 time1 = 0;
    UINT32 time11 = 0;
    UINT32 time12 = 0;
    UINT32 time2 = 0;

    UINT64 u64DataLen = 0;
    UINT64 u64Addr = 0;
    UINT64 u64DstAddr = 0;
    UINT32 uiMemType = 0;
    UINT32 uiOffset = 0;
    UINT32 u32busId = 0;
    PCIE_MMAP_TYPE_E enMmapMemType = 0;

    pci_dma_arg_t stDmaPrm = {0};

    if (4 != uiArgCnt)
    {
        SAL_ERROR("Invalid arg! arg_cnt %d \n", uiArgCnt);
        return SAL_FAIL;
    }

    if (SAL_SOK != Pcie_getDevBusId(&u32busId, 0))
    {
        u32busId = BUS_ID0;
    }

    u64DataLen = (UINT64)pArg[0];
    u64Addr = (UINT64)pArg[1];
    uiMemType = ((UINT64)pArg[2] >> PCIE_DATA_MEM_TYPE_OFFSET) & PCIE_CHECK_MASK;
    enMmapMemType = ((UINT64)pArg[2] >> PCIE_MMAP_TYPE_OFFSET) & PCIE_CHECK_MASK;
    u64DstAddr = (UINT64)pArg[3];

    SAL_LOGD("111 recv -- mem type %d \n", uiMemType);

    time0 = SAL_getCurMs();
    switch (uiMemType)
    {
        case PCIE_MMAP_TYPE:
        {
            uiOffset += sizeof(PCIE_MSG_HEAD_S);

            time11 = SAL_getCurMs();

            memcpy((CHAR *)u64Addr, (CHAR *)stMmapMemTab[enMmapMemType].pVir + uiOffset, u64DataLen);    /* 将传输数据从mmap内存拷贝到外部内存 */
            while (uiOffset <= u64DataLen)
            {
                memset((CHAR *)stMmapMemTab[enMmapMemType].pVir + uiOffset, 0, 128);
                uiOffset += 128;
            }

            time1 = SAL_getCurMs();

            s32Ret = SAL_SOK;

            if (SAL_SOK == s32Ret)
            {
                stRecvTimeDbg.u32Idx = PCIE_MMAP_TYPE;
                (VOID)Pcie_FillPrStr("recv---mmap", &stRecvTimeDbg);

                Pcie_AddDbgCnt(&stRecvTimeDbg);

                stRecvTimeDbg.u32SubIdx = 0;
                Pcie_GetDbgTime(time0, time11, &stRecvTimeDbg);

                stRecvTimeDbg.u32SubIdx = 1;
                (VOID)Pcie_GetDbgTime(time11, time1, &stRecvTimeDbg);

                Pcie_PrDbgTimeInfo(60, &stRecvTimeDbg);
            }

            break;
        }
        case PCIE_PHY_TYPE:
        {
            time12 = SAL_getCurMs();

            stDmaPrm.src = u64Addr;         /* 获取RC端传送过来的源物理地址 */
            stDmaPrm.dst = u64DstAddr;
            stDmaPrm.length = u64DataLen;
            stDmaPrm.type = PCI_ADDR_PHYS;
            stDmaPrm.dir = PCI_DMA_READ;
            stDmaPrm.id = u32busId;  /* 3559a里从片这个id可以不配 */

            s32Ret = ioctl(g_PcieDmaFd, PCI_DMA_START, &stDmaPrm);
            time2 = SAL_getCurMs();

            if (SAL_SOK == s32Ret)
            {
                stRecvTimeDbg.u32Idx = PCIE_PHY_TYPE;
                (VOID)Pcie_FillPrStr("recv---phy", &stRecvTimeDbg);

                Pcie_AddDbgCnt(&stRecvTimeDbg);

                stRecvTimeDbg.u32SubIdx = 0;
                Pcie_GetDbgTime(time0, time12, &stRecvTimeDbg);

                stRecvTimeDbg.u32SubIdx = 1;
                (VOID)Pcie_GetDbgTime(time12, time2, &stRecvTimeDbg);

                Pcie_PrDbgTimeInfo(60, &stRecvTimeDbg);
            }

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
 * @function:   Pcie_MmapMemSecondFree
 * @brief:      释放二次分配的mmap内存
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Pcie_MmapMemSecondFree(VOID)
{
    UINT32 i = 0;

    for (i = 0; i < PCIE_MMAP_TYPE_NUM; i++)
    {
        stMmapMemTab[i].pVir = NULL;
    }

    return SAL_SOK;
}

/**
 * @function:   Pcie_MmapMemSecondAlloc
 * @brief:      对申请的mmap内存进行二次分配
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Pcie_MmapMemSecondAlloc(VOID)
{
    UINT32 i = 0;
    PCIE_MSG_HEAD_S stMsgHead = {0};

    for (i = 0; i < PCIE_SHARE_MEM_BLOCK_CNT; i++)
    {
        stMmapMemTab[i].bUsed = SAL_FALSE;

        if (stMmapMemTab[i].pVir)
        {
            SAL_WARN("mmap mem is alloc! i %d \n", i);
            continue;
        }

        stMmapMemTab[i].pVir = (CHAR *)pBarVirAddr[i] + auiMmapMemOffsetTab[i];
        SAL_WARN("pcie_dbg: i %d, vir %p \n", i, stMmapMemTab[i].pVir);

        /* 填充PCIE文件头 */
        stMsgHead.bUsed = 0;
        stMsgHead.uiType = i;
        stMsgHead.uiDataLen = auiMmapMemDataSize[i];

        memcpy((CHAR *)stMmapMemTab[i].pVir, &stMsgHead, sizeof(PCIE_MSG_HEAD_S));

        /* dma传输yuv数据时存放物理地址的mmap内存需要单独分配，目前使用Bar1的后128KB */
        if (1 == i)
        {
            stMmapMemTab[PCIE_MMAP_PHYADDR_TYPE].pVir = (CHAR *)pBarVirAddr[i] + auiMmapMemOffsetTab[PCIE_MMAP_PHYADDR_TYPE];
            SAL_WARN("pcie_dbg: i %d, vir %p \n", PCIE_MMAP_PHYADDR_TYPE, stMmapMemTab[PCIE_MMAP_PHYADDR_TYPE].pVir);

            /* 填充PCIE文件头 */
            stMsgHead.bUsed = 0;
            stMsgHead.uiType = PCIE_MMAP_PHYADDR_TYPE;
            stMsgHead.uiDataLen = auiMmapMemDataSize[PCIE_MMAP_PHYADDR_TYPE];

            memcpy((CHAR *)stMmapMemTab[PCIE_MMAP_PHYADDR_TYPE].pVir, &stMsgHead, sizeof(PCIE_MSG_HEAD_S));

#if 1
            stMmapMemTab[PCIE_MMAP_RSLT_TYPE_1].pVir = (CHAR *)pBarVirAddr[0] + auiMmapMemOffsetTab[PCIE_MMAP_RSLT_TYPE_1];
            SAL_WARN("pcie_dbg: i %d, vir %p \n", PCIE_MMAP_RSLT_TYPE_1, stMmapMemTab[PCIE_MMAP_RSLT_TYPE_1].pVir);
        
            /* 填充PCIE文件头 */
            stMsgHead.bUsed = 0;
            stMsgHead.uiType = PCIE_MMAP_RSLT_TYPE_1;
            stMsgHead.uiDataLen = auiMmapMemDataSize[PCIE_MMAP_RSLT_TYPE_1];
        
            memcpy((CHAR *)stMmapMemTab[PCIE_MMAP_RSLT_TYPE_1].pVir, &stMsgHead, sizeof(PCIE_MSG_HEAD_S));
#endif
        }
    }

    return SAL_SOK;
}

/**
 * @function:   Pcie_MmapMemFree
 * @brief:      free mmap mem
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Pcie_MmapMemFree(VOID)
{
    UINT32 i = 0;

    (VOID)Pcie_MmapMemSecondFree();

    for (i = 0; i < PCIE_SHARE_MEM_BLOCK_CNT; i++)
    {
        if (!pBarVirAddr[i])
        {
            SAL_WARN("Bar %d is mmapped! \n", i);
            continue;
        }

        munmap(pBarVirAddr[i], auiBarVirSize[i]);
    }

    SAL_WARN("mmap mem free End! \n");
    return SAL_SOK;
}

/**
 * @function:   Pcie_MmapMemAlloc
 * @brief:      alloc mmap mem
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     static INT32
 */
static INT32 Pcie_MmapMemAlloc(VOID)
{
    UINT32 i = 0;
    UINT32 type = 0;
    UINT32 u32busId = 0;


    if (SAL_SOK != Pcie_getDevBusId(&u32busId, 0))
    {
        u32busId = BUS_ID0;
    }

    SAL_WARN("pcie busId = %u \n", u32busId);

    for (i = 0; i < PCIE_SHARE_MEM_BLOCK_CNT; i++)
    {
        if (pBarVirAddr[i])
        {
            SAL_WARN("Bar %d is mmapped! \n", i);
            continue;
        }

        type = ((u32busId << 4) | dev_type[i + 2]) << PAGE_SHIFT;

        pBarVirAddr[i] = mmap(NULL, PCIE_SHARE_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, g_PcieShmFd, type);
        if (MAP_FAILED == pBarVirAddr[i])
        {
            SAL_ERROR("mmap bar %d failed! \n", i);
            goto err;
        }
    }

    /* 对BAR0 和 BAR1的内存二次分配 */
    (VOID)Pcie_MmapMemSecondAlloc();

    SAL_WARN("mmap mem alloc End! \n");
    return SAL_SOK;

err:
    (VOID)Pcie_MmapMemFree();
    return SAL_FAIL;
}

/**
 * @function:   Pcie_CloseDev
 * @brief:      close pcie device
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Pcie_CloseDev(void)
{
    if (g_PcieDmaFd > 0)
    {
        close(g_PcieDmaFd);
        g_PcieDmaFd = -1;
    }

    if (g_PcieShmFd > 0)
    {
        close(g_PcieShmFd);
        g_PcieShmFd = -1;
    }

    SAL_INFO("close usb dev, dma fd %d, shm fd %d \n", g_PcieDmaFd, g_PcieShmFd);
    return SAL_SOK;
}

/**
 * @function:   Pcie_OpenDev
 * @brief:      open pcie device
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
static INT32 Pcie_OpenDev(void)
{
    INT32 s32Ret = SAL_SOK;

    g_PcieDmaFd = open(PCIE_DMA_DEV_NAME, O_RDWR);
    if (g_PcieDmaFd < 0)
    {
        SAL_ERROR("open dev %s failed!\n", PCIE_DMA_DEV_NAME);
        return SAL_FAIL;
    }

    /*获取系统PCIE设备所有信息保存到到本地变量*/
    memset(&gstPcieDevInfo, 0x0, sizeof(pci_device_info));
    s32Ret = ioctl(g_PcieDmaFd, PCI_GET_ALL_DEVICE, &gstPcieDevInfo);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("ioctl PCI_GET_ALL_DEVICE failed\n");
        /* return SAL_FAIL; */
    }

    g_PcieShmFd = open(PCIE_SHM_DEV_NAME, O_RDWR);
    if (g_PcieShmFd < 0)
    {
        SAL_ERROR("open dev %s failed!\n", PCIE_SHM_DEV_NAME);
        return SAL_FAIL;
    }

    SAL_INFO("open pcie dev, dma fd %d, shm fd %d End! \n", g_PcieDmaFd, g_PcieShmFd);
    return SAL_SOK;
}

/**
 * @function:   Pcie_DevDeInit
 * @brief:      Pcie device deinit
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Pcie_DevDeInit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    (VOID)Pcie_MmapMemFree();

    s32Ret = Pcie_CloseDev();
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("close Pcie device failed! \n");
        return SAL_FAIL;
    }

    SAL_INFO("deinit Pcie dev End! \n");
    return SAL_SOK;
}

/**
 * @function:   Pcie_DevDeInit
 * @brief:      Pcie device init
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Pcie_DevInit(VOID)
{
    INT32 s32Ret = SAL_SOK;

    s32Ret = Pcie_OpenDev();
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("open Pcie device failed! \n");
        goto err;
    }

    s32Ret = Pcie_MmapMemAlloc();
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("Pcie mmap mem alloc failed! \n");
        goto free;
    }

    SAL_INFO("init Pcie dev End! \n");
    return SAL_SOK;

free:
    (VOID)Pcie_MmapMemFree();
err:
    return SAL_FAIL;
}

