/**
 * @file:   dma.c
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  DMA拷贝源文件
 * @author: sunzelin
 * @date    2020/10/20
 * @note:
 * @note \n History:
   1.日    期: 2020/10/20
     作    者: sunzelin
     修改历史: 创建文件
 */

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/
#include <platform_hal.h>
#include "dma.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/
#define DCP_DEV_NAME		"/dev/dcp_cdev"
#define MAX_DMA_CHAN_NUM	(8)             /* 当前最多支持8个通道, 默认已申请通道0 */

#define DCP_CHANNEL_ALLOC	_IOW('d', 50, UINT32 *)
#define DCP_CHANNEL_FREE	_IOR('d', 51, UINT32)
#define DCP_MEMCPY_START	_IOR('d', 52, DMA_DCP_USER_INFO)

#define P2P (1 << 0)       /* 物理地址 - 物理地址 */
#define M2M (1 << 1)       /* mmap 地址 - mmap 地址 */
#define P2M (1 << 2)       /* 物理地址 - mmap 地址 */
#define M2P (1 << 3)       /* mmap 地址 - 物理地址 */

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/
typedef struct _DMA_DCP_USER_INFO_
{
    UINT32 uiChnId;
    UINT32 uiFlag;
    UINT64 u64Size;
    UINT64 u64SrcAddr;
    UINT64 u64DstAddr;
} DMA_DCP_USER_INFO;

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 全局变量                     */
/*----------------------------------------------*/
static INT32 g_fd = -1;

/*----------------------------------------------*/
/*                 函数定义                     */
/*----------------------------------------------*/

/**
 * @function:   Dma_OpenDev
 * @brief:      打开DMA设备
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
INT32 Dma_OpenDev(void)
{
    g_fd = open(DCP_DEV_NAME, O_RDWR);
    if (g_fd < 0)
    {
        SAL_ERROR("open %s failed! \n", DCP_DEV_NAME);
        return -1;
    }

    SAL_INFO("Open Dev %s succeed! \n", DCP_DEV_NAME);
    return 0;
}

/**
 * @function:   Dma_CloseDev
 * @brief:      关闭DMA设备
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     VOID
 */
VOID Dma_CloseDev(VOID)
{
    if (g_fd >= 0)
    {
        close(g_fd);
    }

    SAL_INFO("Close Dev%s succeed! \n", DCP_DEV_NAME);
    return;
}

/**
 * @function:   Dma_AllocChn
 * @brief:      申请DMA通道
 * @param[in]:  INT32 *piChn
 * @param[out]: None
 * @return:     INT32
 */
INT32 Dma_AllocChn(INT32 *piChn)
{
    INT32 s32Ret = SAL_SOK;
    UINT32 uiChnId = -1;

    uiChnId = -1;
    s32Ret = ioctl(g_fd, DCP_CHANNEL_ALLOC, &uiChnId);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("ioctl DCP_CHANNEL_ALLOC error! : %d\n", s32Ret);
        return SAL_FAIL;
    }

    *piChn = uiChnId;
    SAL_INFO("Alloc Dma Channel: %d End! \n", uiChnId);

    return SAL_SOK;
}

/**
 * @function:   Dma_FreeChn
 * @brief:      释放DMA通道
 * @param[in]:  INT32 iChn
 * @param[out]: None
 * @return:     VOID
 */
INT32 Dma_FreeChn(INT32 iChn)
{
    INT32 s32Ret = SAL_SOK;

    if (iChn > MAX_DMA_CHAN_NUM - 1)
    {
        SAL_ERROR("invalid chan %d \n", iChn);
        return SAL_FAIL;
    }

    s32Ret = ioctl(g_fd, DCP_CHANNEL_FREE, &iChn);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("ioctl DCP_CHANNEL_FREE error! : %d\n", s32Ret);
        return SAL_FAIL;
    }

    SAL_INFO("Free Dma Channel: %d End! \n", iChn);
    return SAL_SOK;
}

/**
 * @function:   Dma_CopyData
 * @brief:      DMA拷贝接口
 * @param[in]:  UINT32 uiChnId
 * @param[in]:  UINT64 u64SrcAddr
 * @param[in]:  UINT64 u64DstAddr
 * @param[in]:  UINT64 u64Size
 * @param[out]: None
 * @return:     INT32
 */
INT32 Dma_CopyData(UINT32 uiChnId, UINT64 u64SrcAddr, UINT64 u64DstAddr, UINT64 u64Size)
{
    INT32 s32Ret = SAL_SOK;

    DMA_DCP_USER_INFO stDmaDcpUserInfo = {0};

    if (g_fd < 0)
    {
        SAL_ERROR("Dma Dev has not opened yet! Pls open first! \n");
        return SAL_FAIL;
    }

    if (uiChnId > MAX_DMA_CHAN_NUM - 1)
    {
        SAL_ERROR("Invalid chanId %d! \n", uiChnId);
        return SAL_FAIL;
    }

    stDmaDcpUserInfo.uiChnId = uiChnId;
    stDmaDcpUserInfo.u64Size = u64Size;
    stDmaDcpUserInfo.u64SrcAddr = u64SrcAddr;
    stDmaDcpUserInfo.u64DstAddr = u64DstAddr;
    stDmaDcpUserInfo.uiFlag = P2P;    /* 当前默认物理地址进行拷贝，后续若有mmap拷贝需求，可以将拷贝类型开放 */

    s32Ret = ioctl(g_fd, DCP_MEMCPY_START, &stDmaDcpUserInfo);
    if (SAL_SOK != s32Ret)
    {
        SAL_ERROR("ioctl DCP_CHANNEL_FREE error! : %d\n", s32Ret);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

