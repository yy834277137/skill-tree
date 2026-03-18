/** @file      MonitorInter.h
*   @note      HangZhou Hikvision Digital Technology Co., Ltd. All right reserved
*   @brief     状态监控模块对外头文件
*   @version   1.1.0
*   @author    祁文涛
*   @date      2019/12/18
*   @note      初始版本
*/

#ifndef _ICF_MONITORINTER_H_
#define _ICF_MONITORINTER_H_

#include "ICF_PosMgr.h"
#include "ICF_toolkit.h"
#include "ICF_type.h"
#include "ICF_base.h"
#include "ICF_Interface.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  THREAD_MAX_NUM         (128)                        // 线程数量
#define  PROCESSOR_MAX_NUM      (32)                         // 核心数量
#define  FLOW_NODE_MAX_NUM      (16)                         // 单个Package最多流过的NODE数量
#define  NODE_ALGO_NUM          (16)                         // 同一个NODE关联的算子最大数量
#define  ALGO_MAX_NUM           (64)                         // 算子最大数量
#define  CONNECT_MAX_NUM        (64)                         // 连接关系最大数量
#define  ALGO_NAME_LEN          (32)                         // 算子名称长度

#define  PROCESSOR_CORE_MAX_NUM (64)                         // 处理器总数限制
#define  INFO_MAX_LENGTH        (256)                        // 信息字符串最大长度
#define  ALGO_MAX_HANDLE_NUM    (16)                         // 同类型算子最大数量
#define  CONFIG_TYPE_MAX_NUM    (16)                         // 最多支持的监控类型数目
#define  CACHE_QUEUE_MAX        (10)                         // 缓存长度限制

// 事件连接关系
typedef struct _EVENT_CONNECT_
{
    int                    nSrcNodeID;                       // 源NodeID,同类型NodeID都是从1开始，依次累加
    int                    nDestNodeIDNum;                   // 目的NODE数量
    int                    nDestNodeID[ICF_EVENT_NUMMAX];    // 目的NodeID
}EVENT_CONNECT;

// Node节点连接关系
typedef struct _NODE_CONNECT_
{
    int                    nGraphID;                         // GraphID
    int                    nSrcNodeID;                       // 源NodeID,同类型NodeID都是从1开始，依次累加
    int                    nDestNodeID;                      // 目的NodeID
    int                    nDestNodePort;                    // 目的NodePort，多类型输入的节点有效
}NODE_CONNECT;

// GRAPH参数
typedef struct _ICF_GRAPH_STATUS_
{
    int                    nGraphType;                       // 任务类型
    int                    nGraphCount;                      // NodeGraph数量
    int                    szGraphIDs[GRAPH_ID_COUNT];       // GraphID集合
    int                    nEventConnectNum;                 // 事件连接数目
    EVENT_CONNECT          EventConnect[CONNECT_MAX_NUM];    // 事件连接关系集合
    int                    nNodeConnectNum;                  // Node连接数目
    NODE_CONNECT           NodeConnect[CONNECT_MAX_NUM];     // Node连接关系集合
}ICF_GRAPH_STATUS;

// Node层给用户的状态参数
typedef struct _ICF_NODE_STATUS_
{
    char                   szNodeName[NODE_NAME_LEN];        // Node名称
    int                    nNodeID;                          // NodeID
    int                    nAlgNum;                          // 包含的算子数量
    int                    nAlgType[NODE_ALGO_NUM];          // Node关联的算子类型
    bool                   bAlgShare;                        // 算子句柄是否共享
    short                  nInputPortNum;                    // 最大输入端口数
    short                  nQueueBehave[ICF_NODE_PORTMAX];   // 队列行为
    short                  nQueueCurSize[ICF_NODE_PORTMAX];  // 队列实时长度
    short                  nQueueAvgSize[ICF_NODE_PORTMAX];  // 队列平均长度
    short                  nQueueMaxSize[ICF_NODE_PORTMAX];  // 队列最大长度
    float                  fInputFrameRate;                  // 节点输入数据的帧率
    float                  fProcessFrameRate;                // 节点处理数据的帧率
    long long              lInputCount;                      // 输入的总帧数
    long long              lDropCount;                       // 丢掉的总帧数
}ICF_NODE_STATUS;

// 单个算子的状态信息
typedef struct _ICF_ALG_STAT_
{
    int                    nAlgType;                         // 算子类型
    char                   cAlgName[ALGO_NAME_LEN];          // 算子名字
    float                  fCreateTime;                      // 算子句柄创建耗时
    float                  fCreateModelTime;                 // 算子模型句柄创建耗时
    float                  fTimeMean;                        // 算子处理平均之间 ms
    float                  fTimeMax;                         // 算子处理最大时间 ms
    float                  fTimeMin;                         // 算子处理最小时间 ms
    float                  fAlgLoad;                         // 算子负载，最大支持链接数上的平均使用比例
    char                   lTimeCollect[INFO_MAX_LENGTH];    // 采集时间
}ICF_ALG_STAT; 

// 多个算子的状态信息
typedef struct _ICF_ALG_STATUS_
{
    int                    nAlgType;                         // 算子类型
    int                    nAlgNum;                          // 算子句柄数量
    ICF_ALG_STAT           stAlgStatus[ALGO_MAX_HANDLE_NUM]; // 算子状态
}ICF_ALG_STATUS; 


// 调度层参数（由于外部C接口，无法使用C++ vector）
typedef struct _ICF_SCHEDULE_STATUS_
{
    int                    nThreadNum;                       // 线程池中线程数量
    float                  fThreadUsage[THREAD_MAX_NUM];     // 线程利用率
    char                   lTimeCollect[INFO_MAX_LENGTH];    // 采集时间
}ICF_SCHEDULE_STATUS;


// 系统CPU处理器负载信息
typedef struct _CORE_LOAD_INFO_
{
    float                  fUsrLoad;       // CPU usr 负载
    float                  fNicLoad;       // CPU nice 负载
    float                  fSysLoad;       // CPU system 负载
    float                  fIdleLoad;      // CPU idle 负载
    float                  fIowaitLoad;    // CPU iowait 负载
    float                  fIrqLoad;       // CPU 硬中断 负载
    float                  fSoftirqLoad;   // CPU 软中断 负载
}CORE_LOAD_INFO;

// 系统CPU处理器负载信息
typedef struct _PROCESSOR_LOAD_INFO_
{
    int                    nCoreNum;                                 // CPU核心数
    CORE_LOAD_INFO         stCoreLoadInfo[PROCESSOR_CORE_MAX_NUM];   // CPU负载信息
}PROCESSOR_LOAD_INFO;

// 系统平台参数
typedef struct _ICF_SYSTEM_STATUS_
{
    PROCESSOR_LOAD_INFO    stCpuLoad;                        // CPU占用信息
    long long              lTimeCollect;                     // 采集时间
}ICF_SYSTEM_STATUS;

// 数据包状态记录
typedef struct _ICF_PACK_STAT_
{
    int                    nProcessCode;                     // 每个节点的算子处理返回码
    int                    nFlowNodeID;                      // 数据流流过的NodeId
    int                    nThreadID;                        // 数据流流过的线程Id
    short                  nInputPortNum;                    // 输入队列数
    short                  nQueueCurSize[ICF_NODE_PORTMAX];  // 队列实时长度
    float                  fTimeStay;                        // 在某个Node中等待的时间
    long long              lTimeArrive;                      // 到达某节点时间
    long long              lTimeLeave;                       // 离开某节点时间
}ICF_PACK_STAT;

// 数据包流动信息
typedef struct _ICF_ANA_PACKAGE_STATUS_
{
    int                    nErrorCode;                       // 错误码，数据包状态
    int                    nFrameNum;                        // 数据包帧号
    int                    nFlowNodeNum;                     // 流过的节点数目
    ICF_PACK_STAT          stPackageStat[FLOW_NODE_MAX_NUM]; // 数据流状态记录数组
}ICF_ANA_PACKAGE_STATUS;

// 引擎内存信息
typedef struct _ICF_MEMORY_STATUS_
{
    int                    nNum;                             // 内存池中内存类型数量
    ICF_MEM_INFO           stMemInfo[ICF_MEM_TYPE_NUM];      // 引擎初始化所需不同类型内存信息
    ICF_MEMPOOL_STATUS     pMemPoolStatus[ICF_MEM_TYPE_NUM]; // 内存池状态信息
    char                   lTimeCollect[INFO_MAX_LENGTH];    // 采集时间
}ICF_MEMORY_STATUS;

/** @fn     int GetGraphStatusInter(int nGraphType, ICF_GRAPH_STATUS *pGraphInfo)
*   @brief  Graph状态查看接口
*   @param  nGraphType        [in]      - GraphType
*   @param  pGraphInfo        [out]     - Graph状态参数
*   @return int
*/
int GetGraphStatusInter(void *pInitHandle, int nGraphType, ICF_GRAPH_STATUS *pGraphInfo); 

/** @fn     int GetNodeDynamicStatusInter(int nNodeId, int nIndex, ICF_NODE_STATUS *pNodeInfo)
*   @brief  Node动态状态查看接口
*   @param  nNodeId           [in]      - Node序号
*   @param  nIndex            [in]      - 缓存序号 (0为当前数据，N-1为向前追溯的第N个数据，N小于缓存队列长度)
*   @param  pNodeInfo         [out]     - Node状态参数
*   @return int
*/
int GetNodeStatusInter(void *pInitHandle, int nNodeId, int nIndex, ICF_NODE_STATUS *pNodeInfo); 


/** @fn     int GetAlgStatusInter(int nAlgType, int nIndex, ICF_ALG_STATUS *pAlgPerfStat)
*   @brief  算子动态状态查看接口
*   @param  nAlgType          [in]      - 算子类型
*   @param  nIndex            [in]      - 缓存序号 (0为当前数据，N-1为向前追溯的第N个数据，N小于缓存队列长度)
*   @param  pAlgPerfStat      [out]     - 算子性能
*   @return int
*/
int GetAlgStatusInter(void *pInitHandle, int nAlgType, int nIndex, ICF_ALG_STATUS *pAlgPerfStat);


/** @fn     int GetMemoryStatusInter(int nIndex, ICF_MEMORY_STATUS *pMemoryStatus)
*   @brief  内存状态查看接口
*   @param  nIndex            [in]      - 缓存序号 (0为当前数据，N-1为向前追溯的第N个数据，N小于缓存队列长度)
*   @param  pMemoryStatus     [out]     - 内存状态信息
*   @return int
*/
int GetMemoryStatusInter(void *pInitHandle, int nIndex, ICF_MEMORY_STATUS *pMemoryStatus);

/** @fn     int GetScheduleStatusInter(int nIndex, ICF_SCHEDULE_STATUS *pScheduleStat)
*   @brief  调度模块状态查看接口
*   @param  nIndex            [in]      - 缓存序号 (0为当前数据，N-1为向前追溯的第N个数据，N小于缓存队列长度)
*   @param  pScheduleStat     [out]     - 调度模块状态信息
*   @return int
*/
int GetScheduleStatusInter(void *pInitHandle, int nIndex, ICF_SCHEDULE_STATUS *pScheduleStat);

/** @fn     int GetSystemStatusInter(int nIndex, ICF_SYSTEM_STATUS *pSystemStat)
*   @brief  系统状态查看接口
*   @param  nIndex            [out]     - 缓存序号 (0为当前数据，N-1为向前追溯的第N个数据，N小于缓存队列长度)
*   @param  pSystemStat       [out]     - 系统状态信息
*   @return int
*/
int GetSystemStatusInter(void *pInitHandle, int nIndex, ICF_SYSTEM_STATUS *pSystemStat);

/** @fn     int GetPackageStatusInter(int nGraphType, int nGraphId, int nIndex, ICF_ANA_PACKAGE_STATUS *pPackageInfo)
*   @brief  PACKAGE状态查看接口
*   @param  nGraphType        [in]      - 任务类型
*   @param  nGraphId          [in]      - 通道Id
*   @param  nIndex            [in]      - 缓存序号 (0为当前数据，N-1为向前追溯的第N个数据，N小于缓存队列长度)
*   @param  pPackageInfo      [out]     - PACKAGE状态信息
*   @return int
*/
int GetPackageStatusInter(void *pInitHandle, int nGraphType, int nGraphId, int nIndex, ICF_ANA_PACKAGE_STATUS *pPackageInfo);

/** @fn     int GetPosStatusInter(POS_BUF_OUT_PTR *posBufPtr)
*   @brief  POS状态查看接口
*   @param  posBufPtr         [out]    - POS状态信息
*   @return int
*/
int GetPosStatusInter(void *pInitHandle, POS_BUF_OUT_PTR *posBufPtr); 

#ifdef __cplusplus
}
#endif

#endif /* _ICF_MONITORINTER_H_ */