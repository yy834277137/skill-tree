/**
 * @file:   pcie_transmit.h
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  pcie related header files and store outer structures
 * @author: sunzelin
 * @date    2020/7/11
 * @note:
 * @note \n History:
   1.日    期: 2020/7/11
     作    者: sunzelin
     修改历史: 创建文件
 */
#ifndef _PCIE_TRANSMIT_H_
#define _PCIE_TRANSMIT_H_

/*----------------------------------------------*/
/*                 包含头文件                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                 宏类型定义                   */
/*----------------------------------------------*/

/*----------------------------------------------*/
/*                结构体定义                    */
/*----------------------------------------------*/

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
 * @function:   Pcie_GetDevHandle
 * @brief:      Get Pcie Device Description
 * @param[in]   void
 * @param[out]: None
 * @return      INT32
 */
INT32 Pcie_GetDevHandle(void);

/**
 * @function:   Pcie_SendData
 * @brief:      send data via pcie device
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Pcie_SendData(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Pcie_RecvData
 * @brief:      receive data via pcie device
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Pcie_RecvData(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Pcie_CheckDataStatus
 * @brief:      确认当前芯片间缓存状态是否就绪
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     BOOL
 */
BOOL Pcie_CheckDataStatus(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Pcie_SetDataStatus
 * @brief:      设置数据状态
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Pcie_SetDataStatus(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Pcie_SetDmaPhyAddr
 * @brief:      设置DMA传输的物理地址(当前使用mmap内存在片间传输)
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     BOOL
 */
INT32 Pcie_SetDmaPhyAddr(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Pcie_GetDmaPhyAddr
 * @brief:      获取DMA传输的物理地址(当前使用mmap内存在片间传输)
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     BOOL
 */
INT32 Pcie_GetDmaPhyAddr(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Pcie_DevDeInit
 * @brief:      Pcie device deinit
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Pcie_DevDeInit(VOID);

/**
 * @function:   Pcie_DevDeInit
 * @brief:      Pcie device init
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Pcie_DevInit(VOID);

#endif

