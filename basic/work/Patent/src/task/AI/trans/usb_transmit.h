/**
 * @file:   usb_transmit.h
 * @note:   2010-2020, 杭州海康威视数字技术股份有限公司
 * @brief  Usb related header files and store outer structures
 * @author: sunzelin
 * @date    2020/7/11
 * @note:
 * @note \n History:
   1.日    期: 2020/7/11
     作    者: sunzelin
     修改历史: 创建文件
 */
#ifndef _USB_TRANSMIT_H_
#define _USB_TRANSMIT_H_

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
 * @function:   Usb_RecvData
 * @brief:      receive data via usb device
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_RecvData(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Usb_SendData
 * @brief:      send data via usb device
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_SendData(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Usb_CheckDataStatus
 * @brief:      check if data is ready
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     BOOL
 */
BOOL Usb_CheckDataStatus(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Usb_SetDataStatus
 * @brief:      设置数据状态
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_SetDataStatus(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Usb_SetDmaPhyAddr
 * @brief:      设置DMA传输的物理地址(当前使用mmap内存在片间传输)
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     BOOL
 */
INT32 Usb_SetDmaPhyAddr(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Usb_GetDmaPhyAddr
 * @brief:      获取DMA传输的物理地址(当前使用mmap内存在片间传输)
 * @param[in]:  UINT32 uiArgCnt
 * @param[in]:  UINT64 *pArg
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_GetDmaPhyAddr(UINT32 uiArgCnt, UINT64 *pArg);

/**
 * @function:   Usb_DevDeInit
 * @brief:      usb device deinit
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_DevDeInit(VOID);

/**
 * @function:   Usb_DevDeInit
 * @brief:      usb device init
 * @param[in]:  VOID
 * @param[out]: None
 * @return:     INT32
 */
INT32 Usb_DevInit(VOID);

/**
 * @function:   Usb_GetDevHandle
 * @brief:      Get Usb Device Description
 * @param[in]   void
 * @param[out]: None
 * @return      INT32
 */
INT32 Usb_GetDevHandle(VOID);

#endif

