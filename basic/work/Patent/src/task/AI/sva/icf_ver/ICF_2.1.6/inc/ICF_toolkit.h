/** @file      ICF_toolkit.h
*   @note      HangZhou Hikvision Digital Technology Co., Ltd. All right reserved
*   @brief     引擎工具头文件
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

#include "ICF_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/// 线程核心亲和性信息，注意小于实际逻辑核数std::thread::hardware_concurrency();
typedef struct
{
    unsigned int    nNum;               /// 核心数
    unsigned int*   pCoreIDs;           /// 核心ID，从0开始
} ICF_STR_ALIGN_4 CORE_AFFINITY_INFO;

/// 堆栈统计信息设置
typedef struct _STACK_MONITOR_CONFIG_
{
    int          nInitSize;             /// 初始的栈大小,单位：字节
    int          reserved[8];           /// 保留字段
}ICF_STR_ALIGN_4 STACK_MONITOR_CONFIG;

// 全局缓存关键字结构体
typedef struct _ICF_GLOBAL_BUFF_KEY
{
    void       *pGraphHandle;     // 通道句柄
    const char *szKey;            // 缓存块的Key值。
    int         reserve[5];       // 保留字段
}ICF_GLOBAL_BUFF_KEY;

// 全局缓存创建参数
typedef struct _ICF_GLOBAL_BUFF_PARAM
{
    ICF_MEM_TYPE_E nSpace;           // 缓存块内存类型
    int            nSize;            // 缓存块大小
    int            nStrong;          // 是否是强一致缓存块 0:最终一致缓存块，经过一定时间各设备一致(建议用于需要运行期间更新的大数据块)
    // 1:强一致缓存块，提供和单机相同的一致性(建议用于管理等小数据、多个写端或更新频率低的大数据块)
    int            reserve[5];       // 保留字段
}ICF_GLOBAL_BUFF_PARAM;


/***********************************************************************************************************************
 * ICF util 工具接口
***********************************************************************************************************************/
/** @fn  long long ICF_GetSystemTime()
 *  @brief  <获取系统时间，单位us>
 *  @param  无
 *  @return 当前时间，单位us
*/
long long ICF_GetSystemTime();

/** @fn  void *ICF_GetModelInfo
 * @brief  <从ICF的模型句柄中获取某个模型ID的模型句柄,需要注意在ICF_UnLoadModel之后句柄指针失效>
 * @param pModelHandle             [in]      ICF的模型句柄,ICF_MODEL_HANDLE.pModelHandle
 * @param nModelID                 [in]      模型ID,当前版本就是ALG配置文件中的algID
 * @return 对应模型ID的模型句柄
*/
void* ICF_GetModelInfo(void *pModelHandle, int nModelID);

/**
* @brief <查看该通道内所拥有的内存池使用情况，日志模块打印级别TRACE>
* @param pChannelHandle            [in]      创建的通道句柄
* @return ICF错误码
*/
int ICF_GetMemPoolStatus(void *pChannelHandle);

// /** @fn     int MemPoolMemUseTypeStatistics()
// *   @brief  内存池查看各个类型内存中使用用途
// *   @return int
// */
// int MemPoolMemUseTypeStatistics(void *pInitHandle);

/** @fn  int int?GetInitHandleFromMemPool
 *  @brief  <从计算图创建句柄中获取ICF初始化句柄>
 *  @param  无
 *  @return ICF初始化句柄
*/
void *GetInitHandleFromMemPool(void *pMemPool);


/***********************************************************************************************************************
 * 数据包指针操作类接口
***********************************************************************************************************************/
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
 *  @param  pDataPackList       -O    数据指针ICF_DATA_PACK, 按照加入数据包的先后顺序输出
 *  @param  pDataNum            -IO   数据指针个数。接口保证输出数据个数不超过传进来的该值大小
 *  @return ICF_SUCCESS:成功,others:失败
*/
int ICF_Package_GetData(void *pPackageHandle, int nDataType, int nNodeID, void *pDataPackList[], int *pDataNum);

/** @fn  int ICF_Package_GetDataAll(void *pPackageHandle, void *pDataList[], int *pDataNum)
 *  @brief  <从数据指针中获取全部数据，不包括SourceData>
 *  @param  pPackageHandle      -I    数据包指针
 *  @param  pDataPackList       -O    数据指针ICF_DATA_PACK,按照加入数据包的先后顺序输出
 *  @param  pDataNum            -IO   数据指针个数。接口保证输出数据个数不超过传进来的该值大小
 *  @return ICF_SUCCESS:成功,others:失败
*/
int ICF_Package_GetDataAll(void *pPackageHandle, void *pDataPackList[], int *pDataNum);

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

/** @fn  int ICF_Package_SetState(void *pPackageHandle, int nErrorCode)
 *  @brief  <设置Pacakage 数据包状态>
 *  @param  pPackageHandle   -I  数据包
 *  @param  nErrorCode       -I  错误码
 *  @return 数据包状态,ICF错误码
*/
// int ICF_Package_SetState(void *pPackageHandle, int nErrorCode);

/** @fn  ICF_Package_GetSourceMetaData(void *pPackageHandle, int nIndex)
 *  @brief  <获取Pacakage中源数据对应的封装好的MetaData>
 *  @param  pPackageHandle   -I  数据包
 *  @param  nIndex           -I  源数据索引
 *  @param  nMetaType        -I  参考HVA_META_TYPE
 *  @return MetaData指针
*/
void* ICF_Package_GetSourceMetaData(void *pPackageHandle, int nIndex, int nMetaType);


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
int ICF_GServer_CreateDataBlock(void                        *pInitHandle,
                                const ICF_GLOBAL_BUFF_KEY   *stKey,
                                const ICF_GLOBAL_BUFF_PARAM *stParam);

/**
 * @brief 获取Key对应的数据块指针,可读写该指针，
 *        需要与ICF_GServer_ReturnDataWithLock配对使用,算子Process阶段调用
 * @param pInitHandle        [IN]      ICF句柄
 * @param stKey              [IN]      全局缓存Key项
 * @return 数据块指针
 */
void *ICF_GServer_GetDataWithLock(void *pInitHandle, const ICF_GLOBAL_BUFF_KEY *stKey);

/**
 * @brief 更新某个Key对应的数据块到集群中，释放锁,算子Process阶段调用
 * @param pInitHandle        [IN]      ICF句柄
 * @param stKey              [IN]      全局缓存Key项
 * @return 错误码
 */
int ICF_GServer_ReturnDataWithLock(void *pInitHandle, const ICF_GLOBAL_BUFF_KEY *stKey);

/**
 * @brief 获取全局缓存的内存，适用于设备上对该key只读的场景,算子Process阶段调用
 * @param pInitHandle        [IN]      ICF句柄
 * @param stKey              [IN]      全局缓存Key项
 * @return 错误码
 */
const void *ICF_GServer_GetData(void *pInitHandle, const ICF_GLOBAL_BUFF_KEY *stKey);

/**
 * @brief 将数据写入数据块并更新到集群中，与ICF_GServer_ReturnDataWithLock的区别在于不需要提前读取指针。
 *        适用于只需要写的场景,算子Process阶段调用
 * @param pInitHandle        [IN]      ICF句柄
 * @param stKey              [IN]      全局缓存Key项
 * @param pData              [IN]      数据源
 * @param nCopySize          [IN]      需要拷贝的大小
 * @return 错误码
 */
int ICF_GServer_SetData(void *pInitHandle, const ICF_GLOBAL_BUFF_KEY *stKey, const void *pData, int nCopySize);


/***********************************************************************************************************************
 * 数据包缓存功能接口
***********************************************************************************************************************/
/** @fn  void* ICF_CacheData(void *pPackageHandle, ICF_DATA_PACK* stDataPack);
 *  @brief  <数据缓存，调用该接口后，对应数据的引用计数增加，延迟释放>
 *          <ICF_Package_DataCache与ICF_Package_DataRelease接口需配对使用>
 *  @param  pPackageHandle      -I    数据包指针，对外开放时为ICF_ANA_PACKAGE
 *  @param  stDataPack          -I    数据包装结构体
 *  @return void*, 用于将缓存释放。为空时代表失败
*/
#define ICF_CacheData(pPackageHandle, pstDataPack) \
        ICF_CacheDataInter(pPackageHandle, pstDataPack, __FILE__, __FUNCTION__, __LINE__)

/** @fn  int ICF_GetDataPackFromDataCache(void *pDataCache);
 *  @brief  <从缓存数据中获取数据包装结构体>
 *          <只是为了用户使用方便，用户也可以不使用该接口，而是自己维护pDataCache与ICF_DATA_PACK的一一映射>
 *  @param  pDataCache          -I    数据缓存指针，ICF_Package_DataCache接口返回值
 *  @param  pDataPack           -O    数据包装结构体
 *  @return ICF_SUCCESS:成功,others:失败
*/
int ICF_GetDataPackFromDataCache(void *pDataCache, ICF_DATA_PACK *pDataPack);

/** @fn  int ICF_ReleaseDataCache(void *pDataCache);
 *  @brief  <数据缓存释放，调用该接口后，对应数据的引用计数减小，进行数据释放>
 *          <ICF_Package_DataCache与ICF_Package_DataRelease接口需配对使用>
 *  @param  pDataCache          -I    数据缓存指针，ICF_Package_DataCache接口返回值
 *  @return ICF_SUCCESS:成功,others:失败
*/
#define ICF_ReleaseDataCache(pDataCache) \
        ICF_ReleaseDataCacheInter(pDataCache, __FILE__, __FUNCTION__, __LINE__)


/***********************************************************************************************************************
 * 节点操作类接口
***********************************************************************************************************************/
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


/***********************************************************************************************************************
 * HVA相关操作接口
***********************************************************************************************************************/
/** @fn       ICF_HVA_GetDeserialData
 *  @brief    将数据反序列化
 *  @param    pPackageHandle         [in]           数据所在的Package
 *  @param    pInputData             [in]           序列化数据
 *  @param    nInputDataSize         [in]           序列化数据大小
 *  @param    pOutputData            [out]          反序列化数据
 *  @param    nOutputDataSize        [in]           反序列化数据大小
 *  @return   状态码
 *  @note
 */
int ICF_HVA_GetDeserialData(void              *pPackageHandle,
                            void              *pInputData,
                            unsigned int       nInputDataSize,
                            void              *pOutputData,
                            unsigned int       nOutputDataSize);

/** @fn       ICF_HVA_GetSerialData
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
int ICF_HVA_GetSerialData(void              *pPackageHandle,
                          void              *pInputData,
                          unsigned int       nInputDataSize,
                          void              *pOutputData,
                          unsigned int       nOutputDataSize,
                          void              *pHvaMemParam);

/** @fn  int ICF_HVA_MemSpace_transfer
 *  @brief  HVA内存类型转ICF内存类型
 *  @param  dst_space              [out]           转化后的ICF内存类型
 *  @param  src_space              [in]            转化前的HVA内存类型
 *  @return 状态码
*/
int ICF_HVA_MemSpace_transfer(unsigned int *dst_space,
                              unsigned int *src_space);

/** @fn  int ICF_HVA_MemSpace_transfer_inv
 *  @brief  ICF内存类型转HVA内存类型
 *  @param  dst_space              [out]           转化后的ICF内存类型
 *  @param  src_space              [in]            转化前的HVA内存类型
 *  @return 状态码
*/
int ICF_HVA_MemSpace_transfer_inv(unsigned int *dst_space,
                                  unsigned int *src_space);


/***********************************************************************************************************************
 * 程序堆栈信息辅助接口
***********************************************************************************************************************/
/// 崩溃时，生成coredump文件打印堆栈
/** @fn  int ICF_CoreDump_Register
 *  @brief  <崩溃打印堆栈>
 *  @param  pDumpConfig          [in]      配置参数（可以为空）
 *  @return 0:成功，其他:失败
*/
int ICF_CoreDump_Register(void *pDumpConfig);

/// 打开堆栈大小监控，框架会去监控算子Process接口堆栈使用量
/** @fn  int ICF_StackInfo_Toggle
 *  @brief  <堆栈大小监控>
 *  @param  pInitHandle           [in]      ICF初始化句柄
 *  @param  pStackConfig          [in]      配置参数 [可为空] [STACK_MONITOR_CONFIG结构体，配置初始化时的堆栈监控大小, 默认为 7*1024*1024,不可设置过大]
 *  @return 0:成功，其他:失败
*/
int ICF_StackInfo_Toggle(void *pInitHandle, void *pStackConfig);

/// 打印堆栈信息，前提是已调用ICF_StackInfo_Toggle接口打开开关
/** @fn  int ICF_StackInfo_Print
 *  @brief  <堆栈大小监控>
 *  @param  pInitHandle           [in]      ICF初始化句柄
 *  @return 0:成功，其他:失败
*/
int ICF_StackInfo_Print(void *pInitHandle);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/***********************************************************************************************************************
 * 协程配套接口
***********************************************************************************************************************/
/**@fn       ICF_Tool_Routine_Quary
 * @brief    协程ID查询接口
 * @param    无
 * @return   协程ID, 其中0代表无效
 * @note
 */
int ICF_Tool_Routine_Quary(void);

/**@fn       ICF_Tool_Routine_Notify
 * @brief    协程通知调度接口
 * @param    pScheduler          [in]           调度器句柄
 * @param    nRroutineID         [in]           协程ID
 * @return   状态码
 * @note
 */
int ICF_Tool_Routine_Notify(int nRroutineID);

/**@fn       ICF_Tool_Routine_Yield
 * @brief    协程转让控制权
 * @param    无
 * @return   状态码
 * @note
 */
int ICF_Tool_Routine_Yield(void);


/***********************************************************************************************************************
 * 特殊用途接口
***********************************************************************************************************************/
/// 数据包释放接口（仅用于和ICF_SubFunction一起使用，其它人请勿用）
/** @fn  int ICF_package_ReleasePackage
 *  @brief  <释放package>
 *  @param  void
 *  @return 0:成功，其他:失败
 *  @note 由于同步接口的限制较大只能适用单进程顺序连接的计算，特别在分布式下代价较大，
 *        ICF2.1.2之后版本将取消ICF_SubFunction的支持，可采用单线程异步接口代替。
*/
int ICF_package_ReleasePackage(void *pkg);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif /* _ICF_TOOLKIT_H_ */
