/** @file      ICF_toolkit.h
*   @note      HangZhou Hikvision Digital Technology Co., Ltd. All right reserved
*   @brief     引擎工具头文件
*   @version   0.9.0
*   @author    祁文涛
*   @date      2019/9/25
*   @note      初始版本
*/

#ifndef _ICF_TOOLKIT_H_
#define _ICF_TOOLKIT_H_

#include "ICF_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/// 线程核心亲和性信息，注意小于实际逻辑核数std::thread::hardware_concurrency();
typedef struct
{
    unsigned int    nNum;               /// 核心数
    unsigned int*   pCoreIDs;           /// 核心ID，从0开始
} CORE_AFFINITY_INFO;

/** @fn  long long ICF_GetSystemTime()
 *  @brief  <获取系统时间，单位us>
 *  @param  无
 *  @return 当前时间，单位us
*/
long long ICF_GetSystemTime();

/** @fn  int ICF_SetCoreAffinity 
 *  @brief  <引擎绑核工具接口>
 *  @param  pCoreAffinityInfo   -I    绑核参数
 *  @return ICF错误码
*/
int ICF_SetCoreAffinity(void *pInitHandle, void *pCoreAffinityInfo);

/** @fn  int int ICF_AlgsPerfInfo 
 *  @brief  <查看引擎内部内存使用情况接口，日志模块打印级别TRACE>
 *  @param  无
 *  @return ICF错误码
*/
int ICF_GetMemPoolStatus(void *pInitHandle);

/** @fn     int MemPoolMemUseTypeStatistics()
*   @brief  内存池查看各个类型内存中使用用途
*   @return int
*/
int MemPoolMemUseTypeStatistics(void *pInitHandle);

/** @fn  int int ICF_AlgsPerfInfo 
 *  @brief  <获得算子运行期间耗时统计数据的接口，日志模块打印级别 EFFECT>
 *  @param  无
 *  @return ICF错误码
*/
int ICF_AlgsPerfInfo(void *pInitHandle);

/// 操作节点指针的功能接口
/** @fn  int ICF_Node_GetNodeID(void *pNodeHandle)
 *  @brief  <Node操作函数，从节点句柄获取NodeID>
 *  @param  pNodeHandle         -I    Node句柄
 *  @return 通道号，<=0表示未找到
*/
int ICF_Node_GetNodeID(void *pNodeHandle);

/** @fn  int ICF_Node_AddRouteItem(void *pNodeHandle, void *pPackageHandle, int nNodeID, int nPortID)
 *  @brief  <为一个数据包添加路由项,当前数据包需要发送给多个节点时可以连续调用>
 *  @param  pNodeHandle         -I    节点句柄指针
 *  @param  pPackageHandle      -I    待发送数据包的指针
 *  @param  nNodeID             -I    目标节点ID, <=0 无效
 *  @param  nPortID             -I    目标节点端口号>=0 单输入节点为0 多输入节点需准确输入
 *  @return ICF_SUCCESS:添加成功,others:失败
*/
int ICF_Node_AddRouteItem(void *pNodeHandle, void *pPackageHandle, int nNodeID, int nPortID);

/// 操作数据包指针的功能接口
/** @fn  void* ICF_GetDataPtrFromPkg(void *pPackageHandle, int nDataType)
 *  @brief  <获取Pacakage中指定数据类型对应的数据指针,建议采用最新接口ICF_Package_GetData()>
 *  @param  pPackageHandle      -I    数据包
 *  @param  nDataType           -I    数据类型
 *  @return 返回数据类型对应的数据指针(兼容老接口,返回的不是ICF_DATA_PACK),若返回为NULL表示未找到
*/
void* ICF_GetDataPtrFromPkg(void *pPackageHandle, int nDataType);

/** @fn  int ICF_GetPackageGraphID(void *pPackageHandle)
 *  @brief  <获取Pacakage 所属通道号,建议采用最新接口ICF_Package_GetGraphID()>
 *  @param  pPackageHandle      -I    数据包
 *  @return 通道号，<=0表示未找到
*/
int ICF_GetPackageGraphID(void *pPackageHandle);

/** @fn  int ICF_Package_GetLifeTime
 *  @brief  <ICF Package操作函数, 获取数据包在框架内部停留的时间>
 *  @param  pPackageHandle      -I    数据指针
 *  @return 时间, 单位：us, <0代表获取失败
*/
long long ICF_Package_GetLifeTime(void *pPackageHandle);

/** @fn  int ICF_Package_GetDataNum
 *  @brief  <ICF Package操作函数, 获取数据包中算子输出的数据个数>
 *  @param  pPackageHandle      -I    数据指针
 *  @return -1: error, >=0:该数据包流过的节点的算子输出结果的个数
*/
int ICF_Package_GetDataNum(void *pPackageHandle);

/** @fn  int ICF_Package_GetData(void *pPackageHandle, int nDataType, int nNodeID, void *pDataList[], int *pDataNum)
 *  @brief  <从数据指针中获取某个数据类型的数据>
 *  @param  pPackageHandle      -I    数据包指针
 *  @param  nDataType           -I    需要的数据类型,系统定义加用户自定义ICF_ANA_DATA_TYPE_E
 *  @param  nNodeID             -I    数据产生的节点,如果<=0,则不关心。>0,加入匹配条件中
 *  @param  pDataList           -O    数据指针ICF_DATA_PACK, 按照加入数据包的先后顺序输出
 *  @param  pDataNum            -IO   数据指针个数。接口保证输出数据个数不超过传进来的该值大小
 *  @return ICF_SUCCESS:成功,others:失败
*/
int ICF_Package_GetData(void *pPackageHandle, int nDataType, int nNodeID, void *pDataList[], int *pDataNum);

/** @fn  int ICF_Package_GetDataAll(void *pPackageHandle, void *pDataList[], int *pDataNum)
 *  @brief  <从数据指针中获取全部数据，不包括SourceData>
 *  @param  pPackageHandle      -I    数据包指针
 *  @param  pDataList           -O    数据指针ICF_DATA_PACK,按照加入数据包的先后顺序输出
 *  @param  pDataNum            -IO   数据指针个数。接口保证输出数据个数不超过传进来的该值大小
 *  @return ICF_SUCCESS:成功,others:失败
*/
int ICF_Package_GetDataAll(void *pPackageHandle, void *pDataList[], int *pDataNum);

/** @fn  int ICF_Package_GetGraphID(void *pPackageHandle)
 *  @brief  <获取Pacakage 所属通道号>
 *  @param  pPackageHandle      -I    数据包
 *  @return 通道号，<=0表示未找到
*/
int ICF_Package_GetGraphID(void *pPackageHandle);

/** @fn  int ICF_Package_GetGraphType(void *pPackageHandle)
 *  @brief  <获取Pacakage 所属GraphType>
 *  @param  pPackageHandle   -I  数据包
 *  @return 通道号，<=0表示未找到
*/
int ICF_Package_GetGraphType(void *pPackageHandle);

/** @fn  int ICF_Package_GetState(void *pPackageHandle)
 *  @brief  <获取Pacakage 数据包状态>
 *  @param  pPackageHandle   -I  数据包
 *  @return 数据包状态,ICF错误码
*/
int ICF_Package_GetState(void *pPackageHandle);

/** @fn  int ICF_Tool_CheckBasePtrIsFree(void *pData)
 *  @brief  <判断是否能够释放>
 *  @param  pData               -I    ICF_DATA_PACK或ICF_MEDIA_INFO中的原始的数据指针
 *  @return 0:可以释放 >0:不能释放 <0:一般出现了问题
*/
int ICF_Tool_CheckBasePtrIsFree(void *pInitHandle, void *pData);

/** @fn  int ICF_Tool_UpdataPtrCount(void *pData, int nCount)
 *  @brief  <更改源数据指针在框架内部的计数值>
 *  @param  pInitHandle         -I    ICF初始化句柄
 *  @param  pData               -I    数据指针
 *  @param  nCount              -I    加更新值，可以为负数
 *  @return ICF_SUCCESS:成功,others:失败
*/
int ICF_Tool_UpdataPtrCount(void *pInitHandle, void *pData, int nCount);

#ifdef __cplusplus
}
#endif

#endif /* _ICF_TOOLKIT_H_ */
