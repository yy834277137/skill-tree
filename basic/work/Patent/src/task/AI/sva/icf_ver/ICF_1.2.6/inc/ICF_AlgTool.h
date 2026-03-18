
/** @file      ICF_AlgTool.h
*   @note      HangZhou Hikvision Digital Technology Co., Ltd. All right reserved
*   @brief     本文件中接口是内部操作接口，仅和封装层算子存在交互。
*   @version   0.9.0
*   @author    祁文涛
*   @date      2019/11/15
*   @note      初始版本
*/

#ifndef _ICF_ALGTOOLKIT_H_
#define _ICF_ALGTOOLKIT_H_

#include <stdio.h>
#include "ICF_Log.h"
#include "ICF_base.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @fn  typedef int(*ICF_GetGlobalBufInfo)(void *pstMemTab)
 *  @brief  <引擎全局内存申请接口>
 *  @param  pstMemTab  [in]  内存表参数
 *  @return 错误码
*/
typedef int(*ICF_GetGlobalBufInfo)(void *pstMemTab);

/** @fn  int ICF_EventDriver 
 *  @brief  <事件驱动器接口>
 *  @param  pHandle        -I  控制对象句柄
 *  @param  pstMsgPackage  -I  要传递事件消息，自定义
 *  @return 错误码
*/
int ICF_EventDriver(void *pHandle, void *pstMsgPackage);

/** @fn  int ICF_CopySourceData 
 *  @brief  <Decode算子不修改分辨率情况下的纯拷贝接口>
 *  @param  pstAlgPacket         -I  PKG数据
 *  @return ICF_SUCCESS:满足
*/
int ICF_CopySourceData(void *pstAlgPacket);

/** @fn  int ICF_ResizeAllocData 
 *  @brief  <Decode算子拷贝内存申请转换接口>
 *  @param  pstAlgPacket         -I  PKG数据
 *  @param  pDestData            -I  缩放目标数据信息
 *  @param  pSourceData          -O  PKG的原始数据,返回原始输入的YUV数据
 *  @return ICF_SUCCESS:满足
*/
int ICF_ResizeAllocData(void *pstAlgPacket, ICF_MEDIA_INFO *pDestData, ICF_MEDIA_INFO *pSourceData);

/// 数据包操作接口
/** @fn  int ICF_Package_CreatePackage
 *  @brief  <创建一个Package,初始化内部计数器>
 *  @param  void
 *  @return NULL:创建失败，其他:成功
*/
void *ICF_Package_CreatePackage(void *pInitHandle);

/** @fn  int ICF_Package_AddData(void *pPackageHandle, void *pData)
 *  @brief  <添加某个数据类型的数据指针到内存中,如果需要支持,可以像框架一样填入NODE_ID>
 *  @param  pPackageHandle      -I    数据包指针
 *  @param  pData               -I    数据指针ICF_DATA_PACK如果是ICF_ANA_MEDIA_DATA,
 *  @param  pData.nDataType     -I    数据类型,如果==ICF_ANA_MEDIA_DATA只有[pData，nSize，nSpace]会被存入stBlobData
 *  @param  pData.nNodeID       -I    数据产生的节点,如果<=0,则不关心。
 *  @return ICF_SUCCESS:成功,others:失败
*/
int ICF_Package_AddData(void *pPackageHandle, void *pData);

/** @fn  int ICF_Package_UpdateCount(void *pPackageHandle, void *pData, int nCount)
 *  @brief  <更新数据指针的引用计数器>
 *  @param  pPackageHandle      -I    数据包指针，对外开放时为ICF_ANA_PACKAGE
 *  @param  pData               -I    数据指针ICF_DATA_PACK
 *  @param  pData.nDataType     -I    需要的数据类型
 *  @param  pData.nNodeID       -I    数据产生的节点,如果<=0,则直接按照nDataType查找
 *  @param  nCount              -I    加更新值
 *  @return ICF_SUCCESS:成功,others:失败
*/
int ICF_Package_UpdateCount(void *pPackageHandle, void *pData, int nCount);

/** @fn  int ICF_Package_MoveData(void *pSrcPackageHandle, void *pDstPackageHandle, void *pSrcData)
 *  @brief  <将一个数据从源数据包移动到目的数据包，共享数据，更新计数器>
 *  @param  pSrcPackageHandle   -I    源数据包指针
 *  @param  pDstPackageHandle   -I    目标数据包指针
 *  @param  pData               -I    需要移动的数据ICF_DATA_PACK
 *  @return ICF_SUCCESS:成功,others:失败
*/
int ICF_Package_MoveData(void *pSrcPackageHandle, void *pDstPackageHandle, void *pSrcData);

/** @fn  int ICF_Package_MoveSource(void *pSrcPackageHandle, void *pDstPackageHandle, int nIndex)
 *  @brief  <将一个数据从源数据包移动到目的数据包，共享数据，更新计数器>
 *  @param  pSrcPackageHandle   -I    源数据包指针
 *  @param  pDstPackageHandle   -I    目标数据包指针
 *  @param  nIndex              -I    需要移动的源ICF_SOURCE_BLOB的下标
 *  @return ICF_SUCCESS:成功,others:失败
*/
int ICF_Package_MoveSource(void *pSrcPackageHandle, void *pDstPackageHandle, int nIndex);

/// 数据快照操作接口
/** @fn  int ICF_ShortCut_GetQueueNum(const void *pShortCutHdl)
 *  @brief  <获取快照中队列个数,调用一次从队列中拍取一次快照>
 *  @param  pShortCutHdl        -I    快照数据指针,ICF_ANA_SYNC_DATA
 *  @return >0 队列个数 <=0错误
*/
int ICF_ShortCut_GetQueueNum(const void *pShortCutHdl);

/** @fn  int ICF_ShortCut_GetQueueLen(const void *pShortCutHdl, int nQueueIndex)
 *  @brief  <获取快照中第nQueueIndex个队列的实际数据个数>
 *  @param  pShortCutHdl        -I    快照数据指针,ICF_ANA_SYNC_DATA
 *  @param  nQueueIndex         -I    队列下标,范围是[0,ICF_ShortCut_GetQueueNum()]
 *  @return >=0 队列中数据个数 <0错误
*/
int ICF_ShortCut_GetQueueLen(const void *pShortCutHdl, int nQueueIndex);

/** @fn  int ICF_ShortCut_GetQueueNodeID(const void *pShortCutHdl, int nQueueIndex)
 *  @brief  <获取快照中第nQueueIndex个队列的输入NodeID,该接口谨慎使用,不一定获取准确>
 *  @param  pShortCutHdl        -I    快照数据指针,ICF_ANA_SYNC_DATA类型
 *  @param  nQueueIndex         -I    队列下标,范围是[0,ICF_ShortCut_GetQueueNum()]
 *  @return 该队列连接的输入NodeID, <=0:无效
*/
int ICF_ShortCut_GetQueueNodeID(const void *pShortCutHdl, int nQueueIndex);

/** @fn  void** ICF_ShortCut_GetQueueData(const void *pShortCutHdl, int nQueueIndex)
 *  @brief  <获取快照中第nQueueIndex个队列的全部数据,个数为ICF_ShortCut_GetQueueLen()>
 *  @param  pShortCutHdl        -I    快照数据指针,ICF_ANA_SYNC_DATA
 *  @param  nQueueIndex         -I    队列下标,范围是[0,ICF_ShortCut_GetQueueNum()]
 *  @return void *pPackageHandle[] 数据包句柄首地址，可通过ICF_Package_xxx接口操作
*/
void** ICF_ShortCut_GetQueueData(const void *pShortCutHdl, int nQueueIndex);

/** @fn  int ICF_ShortCut_SetRemove(const void *pShortCutHdl, int nQueueIndex, int nDataIndex)
 *  @brief  <释放快照中第nQueueIndex个队列的第nDataIndex个数据，依照计数器原则进行释放>
 *  @param  pShortCutHdl        -I    快照数据指针,ICF_ANA_SYNC_DATA
 *  @param  nQueueIndex         -I    队列下标,范围是[0,ICF_ShortCut_GetQueueNum()]
 *  @param  nDataIndex          -I    队列中数据下标,范围是[0,IICF_ShortCut_GetQueueLen()]
 *  @return ICF错误码
*/
int ICF_ShortCut_SetRemove(const void *pShortCutHdl, int nQueueIndex, int nDataIndex);

/** @fn  int ICF_ShortCut_AddSendData(const void *pShortCutHdl, const void *pPackageHdl)
 *  @brief  <将pPackageHdl数据包放到待发送区>
 *  @param  pShortCutHdl        -I    快照数据指针,ICF_ANA_SYNC_DATA
 *  @param  pPackageHdl         -I    需要发送的数据包的指针
 *  @return 当前待发送数据的个数,每调用成功一次会加1
*/
int ICF_ShortCut_AddSendData(const void *pShortCutHdl, const void *pPackageHdl);

/** @fn  int ICF_ShortCut_MoveToSend(const void *pShortCutHdl, int nQueueIndex, int nDataIndex)
 *  @brief  <将快照中第nQueueIndex个队列的nDataIndex数据包移动到待发送区>
 *  @param  pShortCutHdl        -I    快照数据指针,ICF_ANA_SYNC_DATA
 *  @param  nQueueIndex         -I    队列下标,范围是[0,ICF_ShortCut_GetQueueNum()]
 *  @param  nDataIndex          -I    队列中数据下标,范围是[0,ICF_ShortCut_GetQueueLen()]
 *  @return 当前待发送数据的个数,每调用成功一次会加1
*/
int ICF_ShortCut_MoveToSend(const void *pShortCutHdl, int nQueueIndex, int nDataIndex);

/** @fn  int ICF_ShortCut_GetSendData(const void *pShortCutHdl, const void *pPackageHdl)
 *  @brief  <将pPackageHdl数据包放到待发送区>
 *  @param  pShortCutHdl        -I    快照数据指针,ICF_ANA_SYNC_DATA类型
 *  @param  pPackList           -O    输出数据
 *  @param  nNum                -I    最大获取的数据个数
 *  @return 当前待发送数据的个数
*/
int ICF_ShortCut_GetSendData(const void *pShortCutHdl, void *pPackList[], int nNum);

/** @fn  int ICF_Tool_CheckPtrIsValid(void *ptr, int size)
 *  @brief  <校验指针指向的一块内存读写是否有效>
 *  @param  ptr                 -I    数据起始地址
 *  @param  size                -I    想要校验的数据块大小
 *  @return ICF_SUCCESS:有效 其他:无效
*/
int ICF_Tool_CheckPtrIsValid(void *ptr, int size);

#ifdef __cplusplus
}
#endif

#endif /* _ICF_ALGTOOLKIT_H_ */
