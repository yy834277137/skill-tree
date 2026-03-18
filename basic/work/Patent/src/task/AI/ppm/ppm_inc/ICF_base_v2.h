/** @file ICF_base_v2.h
 *  @note Hikvision Digital Technology Co., Ltd. All Rights Reserved
 *  @brief ICF基础类型、ICF宏定义
 *
 *  @author   曹贺磊
 *  @version  1.0.4
 *  @date     2020/06/08
 *  @note     更改ICF_SOURCE_BLOB结构体，增加部分HVA算子需要的字段，并增加ROI参数结构体定义
 * 
 *  @author   刘锦胜
 *  @version  1.0.3
 *  @date     2020/1/10
 *  @note     加入相关注释，增加易用性
 *
 *  @author   曹贺磊
 *  @version  1.0.2
 *  @date     2020/1/9
 *  @note     ICF_INIT_PARAM结构中加入调度器
 *
 *  @author   祁文涛
 *  @version  1.0.1
 *  @date     2020/1/9
 *  @note     将内存池状态结构体移至外部头文件
 *
 *  @author   曹贺磊
 *  @version  1.0.0
 *  @date     2019/11/14
 * 
 *  @author   郭俞江
 *  @version  0.9.1
 *  @date     2019/6/27
 *
 *  @note 
 */


#ifndef _ICF_BASE_H_
#define _ICF_BASE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "ICF_type_v2.h"

#define ICF_MAX_BLOB_DIM          (16)              /// 输入数据支持的维度
#define ICF_MAX_INPUT_BLOB_NUM    (16)              /// 输入数据支持最大BLOB数--16个
#define ICF_MEM_TYPE_NUM          (6)               /// 内存池支持最大内存类型数量--6个
#define ICF_MAX_SRC_DATA_CNT      (8)               /// 与后端解码应用兼容,请忽略
#define ICF_ANA_DATA_MAX_NUM      (64)              /// PKG分析包包含最多64个中间结果(流水中最多64个节点)
#define ICF_NODE_PORTMAX          (64)              /// 节点最大接入数--64个(每个连接使用一个port)
#define ICF_NODE_OUTPUTMAX        (16)              /// 节点最大后节节点数--16个(多输出节点)
#define ICF_NODE_BATCHMAX         (16)              /// 最大BATCH数--16个(多batch节点，未支持)
#define ICF_NODE_QUEUEMAX         (64)              /// 节点队列最大长度--64个
#define ICF_NODE_SENDMAX          (64)              /// 节点队列最大发送长度--64个
#define ICF_EVENT_NUMMAX          (64)              /// 某节点最多控制的节点数--64个
#define ICF_TYPENAME_LEN          (64)              /// 类型名称--64字符
#define ICF_MEM_TAB_NUM           (32)              /// 内存表数目限制
#define ICF_MAX_CLUSTER_NUM       (16)              /// PCIE互联最大芯片数量
#define ICF_ALGP_MODEL_NUM        (64)              /// 最大模型个数(对应算子配置文件中"model_paths"数组最多只能配置64个模型路径)
#define ICF_SHARE_ALG_NUM         (16)              /// 每个通道中最多存在的共享算子个数

#define NODE_NAME_LEN             (64)              /// Node节点名称--64个字符 如"face_detect"
#define GRAPH_ID_COUNT            (64)              /// 同一配置最多支持16个GRAPH
#define NODE_ID_COUNT             (64)              /// 同一个NODE最多有64个ID(用于后端多路应用,如64路包含64个检测NODE，NODE从n-n+64)
#define GRAPH_TYPE_MAX_NUM        (16)              /// 同一配置最多支持16个GRAPH TYPE
#define GLOBAL_BUF_MAX_NUM        (3)               /// 全局内存类型最大数量--3个
#define SA_NODE_ALG_MAX_NUM       (8)               /// 序列NODE最多包含算子个数--8个
#define NODE_CORE_BIND_NUM        (16)              /// NODE绑定核心模式下,一个NODE最多可以绑定在16个核心上
#define BATCH_NUM_MAX             (64)              /// 最大支持的多Batch数
#define BATCH_OVERTIME_MAX        (2000)            /// 多Batch节点超时上限, 2000 ms

#define ICF_MAX_VERTEX_NUM        (10)              /// 多边形最大顶点数
#define ICF_MAX_BLOB_FLOW_NUM     (4)               /// 单个blob内部最多存在的流数据个数
#define ICF_MAX_ROI_NUM           (4)               /// 全局最大ROI个数


/// 数据类型定义
#ifndef  PACKAGE_HANDLE                             /// 数据包句柄类型
#define  PACKAGE_HANDLE            void
#endif

#ifndef ICF_HANDLE                                  /// 引擎句柄
#define ICF_HANDLE                 void
#endif

// ICF 四字节对齐宏
#if (defined(_WIN32) || defined(_WIN64))
#define ICF_STR_ALIGN_4
#else
#define ICF_STR_ALIGN_4         __attribute__ ((aligned (4)))
#endif


/// 内存信息结构体
typedef struct _ICF_MEM_INFO_V2
{
    int                eMemType;                    /// 内存类型ICF_MEM_TYPE_E
    long long          nMemSize;                    /// 内存大小
    
    long long          nVbMemBlkSize;               /// VB类型内存块大小
    int                nVbMemBlkCnt;                /// VB内存块个数

    int                reserved[5];                 /// 预留字段
}ICF_STR_ALIGN_4 ICF_MEM_INFO_V2;

/// 内存块信息
typedef struct _ICF_MEM_BUFFER_V2
{
    int                eMemType;                    /// 内存类型ICF_MEM_TYPE_E
    long long          nMemSize;                    /// [普通内存]内存大小
    void              *pVirMemory;                  /// [普通内存]可用内存区的虚拟起始指针
    void              *pPhyMemory;                  /// [普通内存]可用内存区的物理地址指针

    long long          nVbMemBlkSize;               /// [VB类型内存]块大小
    int                nVbMemBlkCnt;                /// [VB类型内存]内存块个数
    unsigned int       nVbPoolID;                   /// [VB类型内存]申请的VBPool信息
    void              *pReserved[4];                /// 预留参数字段
    int                nReserved[4];                /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_MEM_BUFFER_V2;

/**
 * 内存配置,目前支持两种内存池模式:
 * 1、使用框架默认申请和释放(pMemSytemMallocInter/pMemSytemFreeInter 都设为空)
 * 2、用户自定义申请和释放内存,用户实现回调接口(pMemSytemMallocInter/pMemSytemFreeInter)
*/
typedef struct _ICF_MEM_INTERFACE_V2
{
    void              *pMemSytemMallocInter;        /// 平台底层内存申请接口，接口定义见ICF_Interface.h中 ICF_MemAllocCB
    void              *pMemSytemFreeInter;          /// 平台底层内存释放接口，接口定义见ICF_Interface.h中 ICF_MemReleaseCB
    int                reserved[4];                 /// 预留字段
}ICF_STR_ALIGN_4 ICF_MEM_INTERFACE_V2;

/// 缓存结构体，用于传入配置信息的buff（也可直接传入文件）
typedef struct _ICF_BUFF_V2
{  
    void               *pBuff;                      /// 缓存指针
    int                 nBuffSize;                  /// 缓存大小
    void               *pReserved[4];               /// 预留参数字段
    int                 nReserved[4];               /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_BUFF_V2;

/**
 * 引擎配置文件路径结构体
 * 目前框架需要四个相互独立的配置json:1、算子配置 2、任务配置 3、工具配置 4、APP配置
 * 目前框架支持两种模式:1、文件模式 2、缓存模式 两种模式分别支持加密/不加密配置
 * 文件模式:需设置 pAlgCfgPath路径与bAlgCfgEncry是否加密等参数
 * 缓存模式:需设置 stAlgCfgBuff缓存与bAlgCfgEncry是否加密等参数
 * 三种配置均支持加密/解码输入,加解码工具可使用版本发布配套工具CfgDESTool(目录在util\CfgDESTool下)
 */
typedef struct _ICF_CONFIG_INFO_V2
{  
    const char        *pAlgCfgPath;                 /// 引擎算子配置文件路径
    const char        *pTaskCfgPath;                /// 引擎Task任务配置文件路径
    const char        *pToolkitCfgPath;             /// 引擎工具配置文件路径
    
    int                bAlgCfgEncry;                /// 引擎算子配置是否加密            (加密1/不加密0)
    int                bTaskCfgEncry;               /// 引擎Task任务配置文件是否加密    (加密1/不加密0)
    int                bToolkitCfgEncry;            /// 引擎工具配置文件是否加密        (加密1/不加密0)

    ICF_BUFF_V2        stAlgCfgBuff;                /// 引擎算子配置缓存信息
    ICF_BUFF_V2        stTaskCfgBuff;               /// 引擎Task任务配置缓存信息
    ICF_BUFF_V2        stToolkitCfgBuff;            /// 引擎工具配置缓存信息
    void              *pReserved[4];                /// 预留参数字段
    int                nReserved[4];                /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_CONFIG_INFO_V2;

/// 引擎初始化结构体 ICF_Init传入参数
typedef struct _ICF_INIT_PARAM_V2
{
    ICF_MEM_INTERFACE_V2  stMemConfig;                 /// 内存接口配置
    ICF_CONFIG_INFO_V2    stConfigInfo;                /// 配置信息
    void                 *pScheduler;                  /// 调度器(只做透传,不做校验)
    void                 *pSnapShotMemBase;            /// 快照工具所用内存起始地址
    unsigned int          nSnapShotMemSize;            /// 快照工具所用内存空间大小
    void                 *pReserved[4];                /// 预留参数字段
    int                   nReserved[4];                /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_INIT_PARAM_V2;

/// ICF初始化输出参数
typedef struct _ICF_INIT_HANDLE_V2
{
    void                 *pInitHandle;                 /// 初始化句柄
    void                 *pReserved[4];                /// 预留参数字段
    int                   nReserved[4];                /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_INIT_HANDLE_V2;

/// HVA创建参数
typedef struct _ICF_HVA_PARAM_INFO_V2
{
    int                 nAlgNums;                                                     // 算子个数
    int                 nAlgID[ICF_ANA_DATA_MAX_NUM];                                 // 算子ID
    void*               pPlatHandle[ICF_ANA_DATA_MAX_NUM];                            // 配置信息句柄
    void*               pPlatParam[ICF_ANA_DATA_MAX_NUM];                             // 平台参数信息

    int                 nModelReplace[ICF_ANA_DATA_MAX_NUM];                          // 是否改变算法模型
    int                 nModelNums[ICF_ANA_DATA_MAX_NUM];                             // 模型数目
    int                 nModelMemType[ICF_ANA_DATA_MAX_NUM][ICF_ALGP_MODEL_NUM];      // 打开模型所用的内存
    const char         *pModelFilePath[ICF_ANA_DATA_MAX_NUM][ICF_ALGP_MODEL_NUM];     // 模型路径
    void               *pReserved[16];                                                // 预留参数字段
    int                 nReserved[16];                                                // 预留参数字段
}ICF_STR_ALIGN_4 ICF_HVA_PARAM_INFO_V2;

// APP 参数信息
typedef struct _ICF_APP_PARAM_INFO_V2
{
    const char            *pAppParamCfgPath;                /// 引擎应用参数配置路径
    int                    bAppParamCfgEncry;               /// 引擎应用参数配置路径是否加密    (加密1/不加密0)
    ICF_BUFF_V2            stAppParamCfgBuff;               /// 引擎应用配置缓存信息
    ICF_BUFF_V2            stAppAlgModelBuff;               /// 用于底层算子读取模型文件的缓存(透传给用户)
    ICF_HVA_PARAM_INFO_V2 *pHvaParamInfo;                   /// HVA算子创建参数; 可为空,按照默认方式赋值
    void                  *pReserved[16];                   /// 预留参数字段
    int                    nReserved[16];                   /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_APP_PARAM_INFO_V2;

/// 内存统计参数
typedef struct _ICF_MEMSIZE_PARAM_V2
{
    ICF_MEM_INFO_V2  stNonSharedMemInfo[ICF_MEM_TYPE_NUM];           // 通道独占内存
    ICF_MEM_INFO_V2  stSharedMemInfo[ICF_MEM_TYPE_NUM];              // 通道共享内存
    ICF_MEM_INFO_V2  stMemScratchSize[ICF_MEM_TYPE_NUM];             // 可复用内存表(暂时不提供，后续完善)
    void            *pReserved[16];                                  // 预留参数字段
    int              nReserved[16];                                  // 预留参数字段
}ICF_STR_ALIGN_4 ICF_MEMSIZE_PARAM_V2;

/// 模型创建参数
typedef struct _ICF_MODEL_PARAM_V2
{
    void                      *pInitHandle;                          // 初始化句柄 ICF_INIT_HANDLE_V2.pInitHandle
    int                        nGraphType;                           // 计算图类型
    int                        nGraphID;                             // 计算图ID
    const char                *pGraphName;                           // 计算图名称
    ICF_APP_PARAM_INFO_V2     *pAppParam;                            // APP传递给算子的参数
    int                        nMaxCacheNums;                        // 外部保证最大送入ICF的帧数
    int                        nPreLoadModelNums;                    // 当前无效,预加载模型句柄数
    int                        nPreLoadModelID[ICF_ALGP_MODEL_NUM];  // 当前无效,预加载模型ID(算子配置文件中的AlgID)
    void                      *pReserved[16];                        // 预留参数字段
    int                        nReserved[16];                        // 预留参数字段
}ICF_STR_ALIGN_4 ICF_MODEL_PARAM_V2;

/// 模型句柄结构体
typedef struct _ICF_MODEL_HANDLE_V2
{
    void                      *pModelHandle;                         // 创建的模型句柄
    ICF_MEMSIZE_PARAM_V2       modelMemSize;                         // 创建模型消耗的内存
    void                      *pReserved[16];
    int                        nReserved[16];
}ICF_STR_ALIGN_4 ICF_MODEL_HANDLE_V2;

/// 计算图创建参数
typedef struct _ICF_CREATE_PARAM_V2
{
    ICF_MODEL_PARAM_V2         modelParam;                           // 模型参数
    ICF_MODEL_HANDLE_V2        modelHandle;                          // 模型句柄
    void                      *pReserved[16];
    int                        nReserved[16];
}ICF_STR_ALIGN_4 ICF_CREATE_PARAM_V2;

/// 计算图创建句柄
typedef struct _ICF_CREATE_HANDLE_V2
{
    void                      *pChannelHandle;                       // 通道句柄
    ICF_MEMSIZE_PARAM_V2       createMemSize;                        // 创建算子和运行时需要消耗的内存大小
    void                      *pReserved[16];
    int                        nReserved[16];
}ICF_STR_ALIGN_4 ICF_CREATE_HANDLE_V2;

/// 回调参数结构体
typedef struct _ICF_CALLBACK_PARAM_V2
{
    int                nNodeID;                     /// 设置回调的节点ID
    int                nCallbackType;               /// 回调类型
    void              *pCallbackFunc;               /// 回调函数
    void              *pUsr;                        /// 送给回调的用户数据
    int                nUserSize;                   /// 用户数据大小
    void              *pReserved[16];
    int                nReserved[16];
}ICF_STR_ALIGN_4 ICF_CALLBACK_PARAM_V2;

/// 节点配置参数结构体
typedef struct _ICF_CONFIG_PARAM_V2
{
    int                nNodeID;                     /// 设置回调的节点ID
    int                nKey;                        /// 配置关键字
    void              *pConfigData;                 /// 配置参数,除ICF定义配置结构体外要求该指针指向的内容在内存上连续
    unsigned int       nConfSize;                   /// 参数内存大小
    ICF_CONFIG_MODE    nShareNodeConfMode;          /// 共享节点专用，控制SetConfig时的工作模式，非共享节点和GetConfig时无效
    void              *pReserved[16];               /// 预留指针字段
    int                nReserved[16];               /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_CONFIG_PARAM_V2;

/// 系统（全局）时间结构体 (兼容后端解码功能,可忽略)
typedef struct  _ICF_SYSTEMTIME_V2
{
    unsigned short     sYear;                       /// 年
    unsigned short     sMonth;                      /// 月
    unsigned short     sDayOfWeek;                  /// 周
    unsigned short     sDay;                        /// 日
    unsigned short     sHour;                       /// 时
    unsigned short     sMinute;                     /// 分
    unsigned short     sSecond;                     /// 秒
    unsigned short     sMilliseconds;               /// 毫秒
}ICF_STR_ALIGN_4 ICF_SYSTEMTIME_V2;

/// 解码原始数据 (兼容后端解码功能,可忽略)
typedef struct _ICF_MEDIA_RAW_DATA_V2
{
    unsigned int       nWidth;                      /// 宽
    unsigned int       nHeight;                     /// 高
    unsigned int       nFrameNum;                   /// 帧号
    unsigned int       nFrameRate;                  /// 帧率
    FRAMETYPE          enFrameType;                 /// 帧类型
    unsigned char*     pFrameData;                  /// 数据缓存
    unsigned int       nFrameLen;                   /// 数据长度
    unsigned int       nFrameTime;                  /// 帧时间
    ICF_SYSTEMTIME_V2  stSysTime;                   /// 系统时间
    void              *pReserved[16];               /// 预留指针字段
    int                nReserved[16];               /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_MEDIA_RAW_DATA_V2;

/// 媒体数据结构体 (兼容后端解码功能,可忽略)
typedef struct _ICF_MEDIA_DATA_V2
{
    ICF_BLOB_FORMAT_E  nType;                       /// 解码后得到的数据类型
    int                nWidth;                      /// 宽度
    int                nHeight;                     /// 高度
    int                nFrameRate;                  /// 帧率
    unsigned int       nTimeStamp;                  /// 时间戳
    ICF_SYSTEMTIME_V2  stSysTime;                   /// 系统时间
    int                nFrameNum;                   /// 帧号
    unsigned int       nPitchY;                     /// 图像y处理跨度
    unsigned int       nPitchUV;                    /// 图像uv处理跨度
    int                nLen;                        /// 数据长度
    unsigned char*     pBuffer;                     /// 缓存
    unsigned char*     pUv;                         /// uv地址, 只有解码带pitch时有用
    void*              pSurFace;                    /// 解码surface，
    void*              pHandle;                     /// handle
    void*              pPrivate;                    /// 私有指针
    void              *pReserved[16];               /// 预留指针字段
    int                nReserved[16];               /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_MEDIA_DATA_V2;

/// (兼容后端解码功能,可忽略)
typedef struct _ICF_DECODE_DATA_V2
{
    int                nRawCnt;                         /// 个数
    ICF_MEDIA_RAW_DATA_V2 stRawData[ICF_MAX_SRC_DATA_CNT]; /// 原始数据，输入的图片数据或解码出来的原始码流
    ICF_MEDIA_DATA_V2  stSrcDecodeData;                 /// 解码数据, 原始宽高
    ICF_MEDIA_DATA_V2  stDecodeData;                    /// 解码数据，降采样解码数据
    void              *pUserData;                       /// 用户定义数据
    int                nSize;                           /// 大小
    void              *pReserved[16];                   /// 预留指针字段
    int                nReserved[16];                   /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_DECODE_DATA_V2;

/// 源数据信息结构体
typedef struct _ICF_SOURCE_BLOB_V2
{
    ICF_BLOB_FORMAT_E         eBlobFormat;                    /// 源数据类型 ICF_BLOB_FORMAT_E
    ICF_BLOB_LAYOUT_FORMAT_E  eLayoutFormat;                  /// 数据排列格式,见 ICF_BLOB_LAYOUT_FORMAT_E
    long long                 nFrameNum;                      /// 帧号
    unsigned int              nFrameRate;                     /// 输入视频码流帧率，非处理帧率
    float                     fProcessFrameRate;              /// 处理帧率
    long long                 nTimeStamp;                     /// 时间戳

    int                       nBlobDataType;                  /// 数据类型
    int                       nSpace;                         /// 内存类型
    void                     *pData;                          /// 源数据指针
    long long                 nDataLength;                    /// 数据长度
    char                      szDataName[ICF_TYPENAME_LEN];   /// 数据名称(HVA模式必填)
    unsigned int              nShapeNum;                      /// 维度个数
    unsigned int              nShape[ICF_MAX_BLOB_DIM];       /// 维度具体值
    unsigned int              nStride[ICF_MAX_BLOB_DIM];      /// 维度步长

    void                     *pReserved[16];                  /// 预留指针字段
    int                       nReserved[16];                  /// 预留参数字段
} ICF_STR_ALIGN_4 ICF_SOURCE_BLOB_V2;

/**
 * 输入数据对应的数据以及附带的数据参数
 * ICF_Input_data接口输入结构体
 */
typedef struct _ICF_INPUT_DATA_V2
{
    int                nBlobNum;                          /// 输入BLOB数量, 最多16个
    ICF_SOURCE_BLOB_V2 stBlobData[ICF_MAX_INPUT_BLOB_NUM];/// 多个输入源数据
    int               *pUseFlag[ICF_MAX_INPUT_BLOB_NUM];  /// 标记框架是否用完该源数据数据
    void              *pUserPtr;                          /// 用户指针,用于透传用户需要信息
    int                nUserDataSize;                     /// pUserPtr指向结构体大小
    void              *pReserved[16];                     /// 预留指针字段
    int                nReserved[16];                     /// 预留参数字段
}ICF_STR_ALIGN_4  ICF_INPUT_DATA_V2;

/// 多媒体资源管理信息，对应类型是ICF_ANA_MEDIA_DATA
typedef struct _ICF_MEDIA_INFO_V2
{
    ICF_INPUT_DATA_V2  stInputInfo;                       /// 用户输入数据
    ICF_DECODE_DATA_V2 stDecodeInfo;                      /// 解码相关数据(兼容后端解码功能,可忽略)
    void              *pReserved[16];                     /// 预留指针字段
    int                nReserved[16];                     /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_MEDIA_INFO_V2;

/// 用于统一包装算子输出数据或源数据
typedef struct ICF_DATA_PACK_V2_
{
    void              *pData;                             /// 数据指针,源数据或算子输出内存
    int                nSize;                             /// 数据内存大小
    int                nNodeID;                           /// 产生该数据的节点ID
    int                nDataType;                         /// 用户自定义,标识pData类型,参考ICF_ANA_DATA_TYPE_E分段
    ICF_MEM_TYPE_E     nSpace;                            /// 内存类型
    void              *pReserved[8];                      /// 预留指针字段
    int                nReserved[8];                      /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_DATA_PACK_V2;

/// 节点标签结构体
typedef struct _ICF_NODE_LABEL_V2
{
    int                nNodeID;                           /// 节点ID, 必须配置
    int                nAlgType;                          /// 算子标签，不支持用于查找节点
    char               szNodeName[NODE_NAME_LEN];         /// 节点名称，不支持用于查找节点
    void              *pReserved[8];                      /// 预留指针字段
    int                nReserved[8];                      /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_NODE_LABEL_V2;

/// 内存池状态结构体
/// 工具接口中,查看内存池状态 MemPoolMemGetStatus_V2 返回信息结构体
typedef struct _ICF_MEMPOOL_STATUS_V2
{
    long long           lTotalSize;                       /// 初始化内存大小
    long long           lTotalUserSize;                   /// 用户可用内存大小
    long long           lRemainSize;                      /// 可用空闲内存大小
    long long           lMaxIndex;                        /// 可用的最大内存块序号
    long long           lRemainMaxSize;                   /// 可用的最大内存块大小
    long long           lMinIndex;                        /// 可用的最小内存块序号
    long long           lRemainMinSize;                   /// 可用的最小内存块大小
    long long           lChunkNum;                        /// 内存块总个数
    long long           lFreeChunkNum;                    /// 空闲内存块个数
    ICF_MEM_TYPE_E      nSpace;                           /// 内存类型
    int                 nReserved[16];                    /// 预留参数字段
} ICF_STR_ALIGN_4 ICF_MEMPOOL_STATUS_V2;

/// 版本信息结构体
typedef struct _ICF_VERSION_V2
{
    unsigned int        nMajorVersion;                    /// 主版本号
    unsigned int        nSubVersion;                      /// 子版本号
    unsigned int        nRevisVersion;                    /// 修订版本号

    unsigned int        nVersionYear;                     /// 版本日期-年
    unsigned int        nVersionMonth;                    /// 版本日期-月
    unsigned int        nVersionDay;                      /// 版本日期-日
    int                 nReserved[16];                    /// 预留参数字段
}ICF_STR_ALIGN_4 ICF_VERSION_V2;

// 日志配置参数结构体
// 建议配置文件配置好对应日志等级，置于打开状态，先GetConfig然后修改需要修改的参数项。
// 如果配置文件中未打开相关等级，GetConfig获取不出相关配置，需要手动配置该结构体的各项，要求不为空的项不能为空。
typedef struct _ICF_LOG_CFG_V2
{
    const char *pLogLevel;         // 日志等级 get/set时不能为空 "ERROR" "INFO" "DEBUG" "WARN" "EFFECT" "TRACE" "STATUS"
    const char *pLogSwitch;        // 日志开关 set时 不能为空 "on" "off",get时需要注意，如果配置文件配有打开开关则获取不到
    const char *pLogTag;           // 日志tag  NULL/"":不过滤,"ICF_LOG ALG":表示tag为ICF_LOG/ALG的会被记录(ICF Tag为ICF_LOG)
    const char *pLogType;          // 日志类型 set时不能为空 "console" "file"
    const char *pLogPath;          // 日志文件路径，set时 pLogType为file时不能为空
    const char *pLogRollBack;      // 日志文件到达最大行数后的覆盖方式，pLogType为file时不能为空
    uint32_t    nLogNotFlush;      // 日志文件是否立即刷新 0:立即刷新（默认），1：不刷新，先写到文件缓冲区
    uint32_t    nLogMaxLength;     // 日志文件最大行数，超出后按照覆盖方式进行覆盖，=0：默认为5亿条, >0:有效，最大不能超过5亿条
    void       *pReserved[8];      // 预留指针字段
    int         nReserved[8];      // 预留参数字段
} ICF_LOG_CFG_V2;

#ifdef __cplusplus
}
#endif 

#endif  /* _ICF_BASE_H_ */
