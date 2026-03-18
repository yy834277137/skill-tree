/** @file      ICF_toolkit_v2.h
*   @note      HangZhou Hikvision Digital Technology Co., Ltd. All right reserved
*   @brief     引擎工具头文件
*  
*   @version   1.3.2
*   @author    Liuzhaozhu
*   @date      2022/03/10
*   @note      Add system snapshot tool interface.
*
*   @version   1.3.0
*   @author    曹贺磊
*   @date      2020/06/08
*   @note      增加HVA算子模式反序列化接口
*
*   @version   0.9.0
*   @author    祁文涛
*   @date      2019/9/25
*   @note      初始版本
*/

#ifndef _ICF_TOOLKIT_H_
#define _ICF_TOOLKIT_H_

#include "ICF_base_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

/// 堆栈统计信息设置
typedef struct _STACK_MONITOR_CONFIG_V2
{
    int          nInitSize;             /// 初始的栈大小,单位：字节
    int          reserved[8];           /// 保留字段
}ICF_STR_ALIGN_4 STACK_MONITOR_CONFIG_V2;
#define STACK_MONITOR_CONFIG STACK_MONITOR_CONFIG_V2

// 全局缓存关键字结构体
typedef struct _ICF_GLOBAL_BUFF_KEY
{
    void       *pGraphHandle;     // 通道句柄
    const char *szKey;            // 缓存块的Key值。
    int         reserve[5];       // 保留字段
}ICF_GLOBAL_BUFF_KEY_V2;
#define ICF_GLOBAL_BUFF_KEY ICF_GLOBAL_BUFF_KEY_V2

// 全局缓存创建参数
typedef struct _ICF_GLOBAL_BUFF_PARAM_V2
{
    ICF_MEM_TYPE_E nSpace;           // 缓存块内存类型
    int            nSize;            // 缓存块大小
    int            nStrong;          // 是否是强一致缓存块 0:最终一致缓存块，经过一定时间各设备一致(建议用于需要运行期间更新的大数据块)
                                     // 1:强一致缓存块，提供和单机相同的一致性(建议用于管理等小数据、多个写端或更新频率低的大数据块)
    int            reserve[5];       // 保留字段
}ICF_GLOBAL_BUFF_PARAM_V2;
#define ICF_GLOBAL_BUFF_PARAM ICF_GLOBAL_BUFF_PARAM_V2

// 系统快照工具整个内存空间的头信息
typedef struct _GROUP_HEADER_V2
{
    unsigned int nBlockNum;      // 内存Block区间的数量，一个Block记录了一个线程的执行点信息
    unsigned int nBlockSize;     // Block的内存大小，单位为字节，所有的Block大小均相同
    unsigned int reserved[4];

}ICF_STR_ALIGN_4 GROUP_HEADER_V2;
#define GROUP_HEADER GROUP_HEADER_V2

// 系统快照工具内存区间块的头信息
typedef struct _BLOCK_HEADER_V2
{
    unsigned int nStatusNum;     // 当前Block记录的有效状态的数量
    int          nExecutePoint;  // 当前Block对应的执行点位，每个执行点位具有唯一性
    long long    nTimeStamp;     // 记录当前Block内容时的时间戳
    unsigned int reserved[4];

}ICF_STR_ALIGN_4 BLOCK_HEADER_V2;
#define BLOCK_HEADER BLOCK_HEADER_V2

/***********************************************************************************************************************
 * ICF util 工具接口
***********************************************************************************************************************/
/** @fn  long long ICF_GetSystemTime_V2()
 *  @brief  <获取系统时间，单位us>
 *  @param  无
 *  @return 当前时间，单位us
*/
#define ICF_GetSystemTime ICF_GetSystemTime_V2
long long ICF_EXPORT ICF_GetSystemTime_V2();

/** @fn  void *ICF_GetModelInfo_V2
 * @brief  <从ICF的模型句柄中获取某个模型ID的模型句柄,需要注意在ICF_UnLoadModel之后句柄指针失效>
 * @param pModelHandle             [in]      ICF的模型句柄,ICF_MODEL_HANDLE_V2.pModelHandle
 * @param nModelID                 [in]      模型ID,当前版本就是ALG配置文件中的algID
 * @return 对应模型ID的模型句柄
*/
#define ICF_GetModelInfo ICF_GetModelInfo_V2
void ICF_EXPORT *ICF_GetModelInfo_V2(void *pModelHandle, int nModelID);

/**
* @brief <查看该通道内所拥有的内存池使用情况，日志模块打印级别TRACE>
* @param pChannelHandle            [in]      创建的通道句柄
* @return ICF错误码
*/
#define ICF_GetMemPoolStatus ICF_GetMemPoolStatus_V2
int ICF_EXPORT ICF_GetMemPoolStatus_V2(void *pChannelHandle);

// /** @fn     int MemPoolMemUseTypeStatistics()
// *   @brief  内存池查看各个类型内存中使用用途
// *   @return int
// */
// int MemPoolMemUseTypeStatistics(void *pInitHandle);

/** @fn  int int?GetInitHandleFromMemPool_V2
 *  @brief  <从计算图创建句柄中获取ICF初始化句柄>
 *  @param  无
 *  @return ICF初始化句柄
*/
#define GetInitHandleFromMemPool GetInitHandleFromMemPool_V2
void ICF_EXPORT *GetInitHandleFromMemPool_V2(void *pMemPool);


/***********************************************************************************************************************
 * 数据包指针操作类接口
***********************************************************************************************************************/
/** @fn  void* ICF_GetDataPtrFromPkg_V2(void *pPackageHandle, int nDataType)
 *  @brief  <获取Pacakage中指定数据类型对应的数据指针,建议采用最新接口ICF_Package_GetData()>
 *  @param  pPackageHandle      -I    数据包
 *  @param  nDataType           -I    数据类型
 *  @return 返回数据类型对应的数据指针(兼容老接口,返回的不是ICF_DATA_PACK),若返回为NULL表示未找到
*/
#define ICF_GetDataPtrFromPkg ICF_GetDataPtrFromPkg_V2
void ICF_EXPORT *ICF_GetDataPtrFromPkg_V2(void *pPackageHandle, int nDataType);

/** @fn  int ICF_GetPackageGraphID_V2(void *pPackageHandle)
 *  @brief  <获取Pacakage 所属通道号,建议采用最新接口ICF_Package_GetGraphID()>
 *  @param  pPackageHandle      -I    数据包
 *  @return 通道号，<=0表示未找到
*/
#define ICF_GetPackageGraphID ICF_GetPackageGraphID_V2
int ICF_EXPORT ICF_GetPackageGraphID_V2(void *pPackageHandle);

/** @fn  int ICF_Package_GetLifeTime_V2
 *  @brief  <ICF Package操作函数, 获取数据包在框架内部停留的时间>
 *  @param  pPackageHandle      -I    数据指针
 *  @return 时间, 单位：us, <0代表获取失败
*/
#define ICF_Package_GetLifeTime ICF_Package_GetLifeTime_V2
long long ICF_EXPORT ICF_Package_GetLifeTime_V2(void *pPackageHandle);

/** @fn  int ICF_Package_GetDataNum_V2
 *  @brief  <ICF Package操作函数, 获取数据包中算子输出的数据个数>
 *  @param  pPackageHandle      -I    数据指针
 *  @return -1: error, >=0:该数据包流过的节点的算子输出结果的个数
*/
#define ICF_Package_GetDataNum ICF_Package_GetDataNum_V2
int ICF_EXPORT ICF_Package_GetDataNum_V2(void *pPackageHandle);

/** @fn  int ICF_Package_GetData_V2(void *pPackageHandle, int nDataType, int nNodeID, void *pDataList[], int *pDataNum)
 *  @brief  <从数据指针中获取某个数据类型的数据>
 *  @param  pPackageHandle      -I    数据包指针
 *  @param  nDataType           -I    需要的数据类型,系统定义加用户自定义ICF_ANA_DATA_TYPE_E
 *  @param  nNodeID             -I    数据产生的节点,如果<=0,则不关心。>0,加入匹配条件中
 *  @param  pDataPackList       -O    数据指针ICF_DATA_PACK, 按照加入数据包的先后顺序输出
 *  @param  pDataNum            -IO   数据指针个数。接口保证输出数据个数不超过传进来的该值大小
 *  @return ICF_SUCCESS:成功,others:失败
*/
#define ICF_Package_GetData ICF_Package_GetData_V2
int ICF_EXPORT ICF_Package_GetData_V2(void *pPackageHandle, int nDataType, int nNodeID, void *pDataPackList[], int *pDataNum);

/** @fn  int ICF_Package_GetDataAll_V2(void *pPackageHandle, void *pDataList[], int *pDataNum)
 *  @brief  <从数据指针中获取全部数据，不包括SourceData>
 *  @param  pPackageHandle      -I    数据包指针
 *  @param  pDataPackList       -O    数据指针ICF_DATA_PACK,按照加入数据包的先后顺序输出
 *  @param  pDataNum            -IO   数据指针个数。接口保证输出数据个数不超过传进来的该值大小
 *  @return ICF_SUCCESS:成功,others:失败
*/
#define ICF_Package_GetDataAll ICF_Package_GetDataAll_V2
int ICF_EXPORT ICF_Package_GetDataAll_V2(void *pPackageHandle, void *pDataPackList[], int *pDataNum);

/** @fn  int ICF_Package_GetGraphID_V2(void *pPackageHandle)
 *  @brief  <获取Pacakage 所属通道号>
 *  @param  pPackageHandle      -I    数据包
 *  @return 通道号，<=0表示未找到
*/
#define ICF_Package_GetGraphID ICF_Package_GetGraphID_V2
int ICF_EXPORT ICF_Package_GetGraphID_V2(void *pPackageHandle);

/** @fn  int ICF_Package_GetGraphType_V2(void *pPackageHandle)
 *  @brief  <获取Pacakage 所属GraphType>
 *  @param  pPackageHandle   -I  数据包
 *  @return 通道号，<=0表示未找到
*/
#define ICF_Package_GetGraphType ICF_Package_GetGraphType_V2
int ICF_EXPORT ICF_Package_GetGraphType_V2(void *pPackageHandle);

/** @fn  int ICF_Package_GetState_V2(void *pPackageHandle)
 *  @brief  <获取Pacakage 数据包状态>
 *  @param  pPackageHandle   -I  数据包
 *  @return 数据包状态,ICF错误码
*/
#define ICF_Package_GetState ICF_Package_GetState_V2
int ICF_EXPORT ICF_Package_GetState_V2(void *pPackageHandle);

/** @fn  int ICF_Package_SetState(void *pPackageHandle, int nErrorCode)
 *  @brief  <设置Pacakage 数据包状态>
 *  @param  pPackageHandle   -I  数据包
 *  @param  nErrorCode       -I  错误码
 *  @return 数据包状态,ICF错误码
*/
// int ICF_Package_SetState(void *pPackageHandle, int nErrorCode);

/** @fn  ICF_Package_GetSourceMetaData_V2(void *pPackageHandle, int nIndex)
 *  @brief  <获取Pacakage中源数据对应的封装好的MetaData>
 *  @param  pPackageHandle   -I  数据包
 *  @param  nIndex           -I  源数据索引
 *  @param  nMetaType        -I  参考HVA_META_TYPE
 *  @return MetaData指针
*/
#define ICF_Package_GetSourceMetaData ICF_Package_GetSourceMetaData_V2
void ICF_EXPORT *ICF_Package_GetSourceMetaData_V2(void *pPackageHandle, int nIndex, int nMetaType);


/***********************************************************************************************************************
 * 单机或分布式缓存操作接口
***********************************************************************************************************************/
/**
 * @brief 创建数据块,算子Create阶段调用
 * @param pInitHandle        [IN]      ICF句柄
 * @param stKey              [IN]      全局缓存Key项
 * @param stParam            [IN]      缓存块配置参数
 * @return 错误码
 */
#define ICF_GServer_CreateDataBlock ICF_GServer_CreateDataBlock_V2
int ICF_EXPORT ICF_GServer_CreateDataBlock_V2(void                        *pInitHandle,
                                              const ICF_GLOBAL_BUFF_KEY_V2   *stKey,
                                              const ICF_GLOBAL_BUFF_PARAM_V2 *stParam);

/**
 * @brief 获取Key对应的数据块指针,可读写该指针，
 *        需要与ICF_GServer_ReturnDataWithLock配对使用,算子Process阶段调用
 * @param pInitHandle        [IN]      ICF句柄
 * @param stKey              [IN]      全局缓存Key项
 * @return 数据块指针
 */
#define ICF_GServer_GetDataWithLock ICF_GServer_GetDataWithLock_V2
void ICF_EXPORT *ICF_GServer_GetDataWithLock_V2(void *pInitHandle, const ICF_GLOBAL_BUFF_KEY_V2 *stKey);

/**
 * @brief 更新某个Key对应的数据块到集群中，释放锁,算子Process阶段调用
 * @param pInitHandle        [IN]      ICF句柄
 * @param stKey              [IN]      全局缓存Key项
 * @return 错误码
 */
#define ICF_GServer_ReturnDataWithLock ICF_GServer_ReturnDataWithLock_V2
int ICF_EXPORT ICF_GServer_ReturnDataWithLock_V2(void *pInitHandle, const ICF_GLOBAL_BUFF_KEY_V2 *stKey);

/**
 * @brief 获取全局缓存的内存，适用于设备上对该key只读的场景,算子Process阶段调用
 * @param pInitHandle        [IN]      ICF句柄
 * @param stKey              [IN]      全局缓存Key项
 * @return 错误码
 */
#define ICF_GServer_GetData ICF_GServer_GetData_V2
const void ICF_EXPORT *ICF_GServer_GetData_V2(void *pInitHandle, const ICF_GLOBAL_BUFF_KEY_V2 *stKey);

/**
 * @brief 将数据写入数据块并更新到集群中，与ICF_GServer_ReturnDataWithLock的区别在于不需要提前读取指针。
 *        适用于只需要写的场景,算子Process阶段调用
 * @param pInitHandle        [IN]      ICF句柄
 * @param stKey              [IN]      全局缓存Key项
 * @param pData              [IN]      数据源
 * @param nCopySize          [IN]      需要拷贝的大小
 * @return 错误码
 */
#define ICF_GServer_SetData ICF_GServer_SetData_V2
int ICF_EXPORT ICF_GServer_SetData_V2(void *pInitHandle, const ICF_GLOBAL_BUFF_KEY_V2 *stKey, const void *pData, int nCopySize);


/***********************************************************************************************************************
 * 数据包缓存功能接口
***********************************************************************************************************************/
/** @fn  void* ICF_CacheData(void *pPackageHandle, ICF_DATA_PACK_V2* stDataPack);
 *  @brief  <数据缓存，调用该接口后，对应数据的引用计数增加，延迟释放>
 *          <ICF_Package_DataCache与ICF_Package_DataRelease接口需配对使用>
 *  @param  pPackageHandle      -I    数据包指针，对外开放时为ICF_ANA_PACKAGE
 *  @param  stDataPack          -I    数据包装结构体
 *  @return void*, 用于将缓存释放。为空时代表失败
*/
#define ICF_CacheData(pPackageHandle, pstDataPack) \
        ICF_CacheDataInter_V2(pPackageHandle, pstDataPack, __FILE__, __FUNCTION__, __LINE__)

/** @fn  int ICF_GetDataPackFromDataCache_V2(void *pDataCache);
 *  @brief  <从缓存数据中获取数据包装结构体>
 *          <只是为了用户使用方便，用户也可以不使用该接口，而是自己维护pDataCache与ICF_DATA_PACK的一一映射>
 *  @param  pDataCache          -I    数据缓存指针，ICF_Package_DataCache接口返回值
 *  @param  pDataPack           -O    数据包装结构体
 *  @return ICF_SUCCESS:成功,others:失败
*/
#define ICF_GetDataPackFromDataCache ICF_GetDataPackFromDataCache_V2
int ICF_GetDataPackFromDataCache_V2(void *pDataCache, ICF_DATA_PACK_V2 *pDataPack);

/** @fn  int ICF_ReleaseDataCache(void *pDataCache);
 *  @brief  <数据缓存释放，调用该接口后，对应数据的引用计数减小，进行数据释放>
 *          <ICF_Package_DataCache与ICF_Package_DataRelease接口需配对使用>
 *  @param  pDataCache          -I    数据缓存指针，ICF_Package_DataCache接口返回值
 *  @return ICF_SUCCESS:成功,others:失败
*/
#define ICF_ReleaseDataCache(pDataCache) \
        ICF_ReleaseDataCacheInter_V2(pDataCache, __FILE__, __FUNCTION__, __LINE__)


/***********************************************************************************************************************
 * 节点操作类接口
***********************************************************************************************************************/
/** @fn  int ICF_Node_GetNodeID_V2(void *pNodeHandle)
 *  @brief  <Node操作函数，从节点句柄获取NodeID>
 *  @param  pNodeHandle         -I    Node句柄
 *  @return 通道号，<=0表示未找到
*/
#define ICF_Node_GetNodeID ICF_Node_GetNodeID_V2
int ICF_EXPORT ICF_Node_GetNodeID_V2(void *pNodeHandle);

/** @fn  int ICF_Node_AddRouteItem_V2(void *pNodeHandle, void *pPackageHandle, int nNodeID, int nPortID)
 *  @brief  <为一个数据包添加路由项,当前数据包需要发送给多个节点时可以连续调用>
 *  @param  pNodeHandle         -I    节点句柄指针
 *  @param  pPackageHandle      -I    待发送数据包的指针
 *  @param  nNodeID             -I    目标节点ID, <=0 无效
 *  @param  nPortID             -I    目标节点端口号>=0 单输入节点为0 多输入节点需准确输入
 *  @return ICF_SUCCESS:添加成功,others:失败
*/
#define ICF_Node_AddRouteItem ICF_Node_AddRouteItem_V2
int ICF_EXPORT ICF_Node_AddRouteItem_V2(void *pNodeHandle, void *pPackageHandle, int nNodeID, int nPortID);


/***********************************************************************************************************************
 * HVA相关操作接口
***********************************************************************************************************************/
/** @fn       ICF_HVA_GetDeserialData_V2
 *  @brief    将数据反序列化
 *  @param    pPackageHandle         [in]           数据所在的Package
 *  @param    pInputData             [in]           序列化数据
 *  @param    nInputDataSize         [in]           序列化数据大小
 *  @param    pOutputData            [out]          反序列化数据
 *  @param    nOutputDataSize        [in]           反序列化数据大小
 *  @return   状态码
 *  @note
 */
#define ICF_HVA_GetDeserialData ICF_HVA_GetDeserialData_V2
int ICF_EXPORT ICF_HVA_GetDeserialData_V2(void              *pPackageHandle,
                                          void              *pInputData,
                                          unsigned int       nInputDataSize,
                                          void              *pOutputData,
                                          unsigned int       nOutputDataSize);

/** @fn       ICF_HVA_GetSerialData_V2
 *  @brief    将数据序列化
 *  @param    pPackageHandle         [in]           数据所在的Package
 *  @param    pInputData             [in]           反序列化数据
 *  @param    nInputDataSize         [in]           反序列化数据大小
 *  @param    pOutputData            [out]          序列化数据
 *  @param    nOutputDataSize        [in]           序列化数据大小
 *  @param    pHvaMemParam           [in]           HVA序列化参数,HVA_MEM_PARAM结构体
 *  @return   状态码
 *  @note
 */
#define ICF_HVA_GetSerialData ICF_HVA_GetSerialData_V2
int ICF_EXPORT ICF_HVA_GetSerialData_V2(void              *pPackageHandle,
                                        void              *pInputData,
                                        unsigned int       nInputDataSize,
                                        void              *pOutputData,
                                        unsigned int       nOutputDataSize,
                                        void              *pHvaMemParam);

/** @fn  int ICF_HVA_MemSpace_transfer_V2
 *  @brief  HVA内存类型转ICF内存类型
 *  @param  dst_space              [out]           转化后的ICF内存类型
 *  @param  src_space              [in]            转化前的HVA内存类型
 *  @return 状态码
*/
#define ICF_HVA_MemSpace_transfer ICF_HVA_MemSpace_transfer_V2
int ICF_EXPORT ICF_HVA_MemSpace_transfer_V2(unsigned int *dst_space,
                                            unsigned int *src_space);

/** @fn  int ICF_HVA_MemSpace_transfer_inv_V2
 *  @brief  ICF内存类型转HVA内存类型
 *  @param  dst_space              [out]           转化后的ICF内存类型
 *  @param  src_space              [in]            转化前的HVA内存类型
 *  @return 状态码
*/
#define ICF_HVA_MemSpace_transfer_inv ICF_HVA_MemSpace_transfer_inv_V2
int ICF_EXPORT ICF_HVA_MemSpace_transfer_inv_V2(unsigned int *dst_space,
                                                unsigned int *src_space);


/***********************************************************************************************************************
 * 程序堆栈信息辅助接口
***********************************************************************************************************************/
/// 崩溃时，生成coredump文件打印堆栈
/** @fn  int ICF_CoreDump_Register_V2
 *  @brief  <崩溃打印堆栈>
 *  @param  pDumpConfig          [in]      配置参数（可以为空）
 *  @return 0:成功，其他:失败
*/
#define ICF_CoreDump_Register ICF_CoreDump_Register_V2
int ICF_EXPORT ICF_CoreDump_Register_V2(void *pDumpConfig);

/// 打开堆栈大小监控，框架会去监控算子Process接口堆栈使用量
/** @fn  int ICF_StackInfo_Toggle_V2
 *  @brief  <堆栈大小监控>
 *  @param  pInitHandle           [in]      ICF初始化句柄
 *  @param  pStackConfig          [in]      配置参数 [可为空] [STACK_MONITOR_CONFIG结构体，配置初始化时的堆栈监控大小, 默认为 7*1024*1024,不可设置过大]
 *  @return 0:成功，其他:失败
*/
#define ICF_StackInfo_Toggle ICF_StackInfo_Toggle_V2
int ICF_EXPORT ICF_StackInfo_Toggle_V2(void *pInitHandle, void *pStackConfig);

/// 打印堆栈信息，前提是已调用ICF_StackInfo_Toggle接口打开开关
/** @fn  int ICF_StackInfo_Print_V2
 *  @brief  <堆栈大小监控>
 *  @param  pInitHandle           [in]      ICF初始化句柄
 *  @return 0:成功，其他:失败
*/
#define ICF_StackInfo_Print ICF_StackInfo_Print_V2
int ICF_EXPORT ICF_StackInfo_Print_V2(void *pInitHandle);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/***********************************************************************************************************************
 * 协程配套接口
***********************************************************************************************************************/
/**@fn       ICF_Tool_Routine_Quary_V2
 * @brief    协程ID查询接口
 * @param    无
 * @return   协程ID, 其中0代表无效
 * @note
 */
#define ICF_Tool_Routine_Quary ICF_Tool_Routine_Quary_V2
int ICF_EXPORT ICF_Tool_Routine_Quary_V2(void);

/**@fn       ICF_Tool_Routine_Notify_V2
 * @brief    协程通知调度接口
 * @param    pScheduler          [in]           调度器句柄
 * @param    nRroutineID         [in]           协程ID
 * @return   状态码
 * @note
 */
#define ICF_Tool_Routine_Notify ICF_Tool_Routine_Notify_V2
int ICF_EXPORT ICF_Tool_Routine_Notify_V2(int nRroutineID);

/**@fn       ICF_Tool_Routine_Yield_V2
 * @brief    协程转让控制权
 * @param    无
 * @return   状态码
 * @note
 */
#define ICF_Tool_Routine_Yield ICF_Tool_Routine_Yield_V2
int ICF_EXPORT ICF_Tool_Routine_Yield_V2(void);


/***********************************************************************************************************************
 * 特殊用途接口
***********************************************************************************************************************/
/// 数据包释放接口（仅用于和ICF_SubFunction一起使用，其它人请勿用）
/** @fn  int ICF_package_ReleasePackage_V2
 *  @brief  <释放package>
 *  @param  void
 *  @return 0:成功，其他:失败
 *  @note 由于同步接口的限制较大只能适用单进程顺序连接的计算，特别在分布式下代价较大，
 *        ICF2.1.2之后版本将取消ICF_SubFunction的支持，可采用单线程异步接口代替。
*/
#define ICF_package_ReleasePackage ICF_package_ReleasePackage_V2
int ICF_EXPORT ICF_package_ReleasePackage_V2(void *pkg);


/***********************************************************************************************************************
 * 系统快照工具接口
***********************************************************************************************************************/
/** @fn     ICF_Snapshot_Point_V2
*   @brief  记录当前执行点的执行点位信息
*   @param  pInitHandle        [in]      - ICF初始化句柄
*   @param  nExecutePoint      [in]      - 执行点位数值
*   @return 执行点句柄
*/
#define ICF_Snapshot_Point ICF_Snapshot_Point_V2
void ICF_EXPORT *ICF_Snapshot_Point_V2(void *pInitHandle, int nExecutePoint);


/** @fn     ICF_Snapshot_Status_V2
*   @brief  记录当前执行点位的状态信息
*   @param  pExecuteHandle        [in]      - 执行点句柄
*   @param  nKey                  [in]      - 状态索引关键字
*   @param  nType                 [in]      - 状态值数据类型
*   @param  nSize                 [in]      - 状态值数据长度，单位为字节
*   @param  pStatus               [in]      - 状态值指针
*   @return 无
*/
#define ICF_Snapshot_Status ICF_Snapshot_Status_V2
void ICF_EXPORT ICF_Snapshot_Status_V2(void *pExecuteHandle, int nKey, int nType, int nSize, void *pStatus);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _ICF_TOOLKIT_H_ */
