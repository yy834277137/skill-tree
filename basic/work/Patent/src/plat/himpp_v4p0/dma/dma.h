/**
 * @file:   dma.h
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  dma拷贝头文件
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
#include "sal.h"

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 函数声明                     */
/*----------------------------------------------*/

/**
 * @function:   Dma_OpenDev
 * @brief:      打开DMA设备
 * @param[in]:  void
 * @param[out]: None
 * @return:     INT32
 */
INT32 Dma_OpenDev(void);

/**
 * @function:   Dma_CloseDev
 * @brief:      关闭DMA设备
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     VOID
 */
VOID Dma_CloseDev(VOID);

/**
 * @function:   Dma_AllocChn
 * @brief:      申请DMA通道
 * @param[in]:  INT32 *piChn
 * @param[out]: None
 * @return:     INT32
 */
INT32 Dma_AllocChn(INT32 *piChn);

/**
 * @function:   Dma_FreeChn
 * @brief:      释放DMA通道
 * @param[in]:  INT32 iChn
 * @param[out]: None
 * @return:     VOID
 */
INT32 Dma_FreeChn(INT32 iChn);

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
INT32 Dma_CopyData(UINT32 uiChnId, UINT64 u64SrcAddr, UINT64 u64DstAddr, UINT64 u64Size);


