/** @file ICF_base.h
 *  @note Hikvision Digital Technology Co., Ltd. All Rights Reserved
 *  @brief ICF基础类型、ICF宏定义
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

#include "ICF_type.h"

#define ICF_MAX_BLOB_DIM          (4)               /// 输入数据支持的维度，最多四维 (n, c, h, w)
#define ICF_MAX_INPUT_BLOB_NUM    (16)              /// 输入数据支持最大BLOB数--16个
#define ICF_MEM_TYPE_NUM          (6)               /// 内存池支持最大内存类型数量--6个
#define ICF_MAX_SRC_DATA_CNT      (8)               /// 与后端解码应用兼容,请忽略
#define ICF_ANA_DATA_MAX_NUM      (32)              /// PKG分析包包含最多32个中间结果(流水中最多32个节点)
#define ICF_NODE_PORTMAX          (64)              /// 节点最大接入数--64个(每个连接使用一个port)
#define ICF_NODE_OUTPUTMAX        (16)              /// 节点最大后节节点数--16个(多输出节点)
#define ICF_NODE_BATCHMAX         (16)              /// 最大BATCH数--16个(多batch节点，未支持)
#define ICF_NODE_QUEUEMAX         (64)              /// 节点队列最大长度--64个
#define ICF_NODE_SENDMAX          (64)              /// 节点队列最大发送长度--64个
#define ICF_EVENT_NUMMAX          (64)              /// 某节点最多控制的节点数--64个

#define NODE_NAME_LEN             (64)              /// Node节点名称--64个字符 如"face_detect"
#define GRAPH_ID_COUNT            (16)              /// 同一配置最多支持16个GRAPH
#define NODE_ID_COUNT             (16)              /// 同一个NODE最多有16个ID(用于后端多路应用,如16路包含16个检测NODE，NODE从n-n+16)
#define GRAPH_TYPE_MAX_NUM        (16)              /// 同一配置最多支持16个GRAPH TYPE
#define GLOBAL_BUF_MAX_NUM        (3)               /// 全局内存类型最大数量--3个
#define SA_NODE_ALG_MAX_NUM       (8)               /// 序列NODE最多包含算子个数--8个
#define NODE_CORE_BIND_NUM        (16)              /// NODE绑定核心模式下,一个NODE最多可以绑定在16个核心上
#define BATCH_NUM_MAX             (64)              /// 最大支持的多Batch数
#define BATCH_OVERTIME_MAX        (2000)            /// 多Batch节点超时上限, 2000 ms

/// 内存信息结构体
typedef struct _ICF_MEM_INFO_
{  
    ICF_MEM_TYPE_E     eMemType;                    /// 内存类型
    long long          nMemSize;                    /// 内存大小
    int                reserved[5];                 /// 预留字段
}ICF_MEM_INFO;

/**
 * 内存配置,目前支持两种内存池模式:
 * 1、使用框架默认申请和释放(pMemSytemMallocInter/pMemSytemFreeInter 都设为空)
 * 2、用户自定义申请和释放内存,用户实现回调接口(pMemSytemMallocInter/pMemSytemFreeInter)
*/
typedef struct _ICF_MEM_CONFIG_
{  
    int                nNum;                        /// 内存池中内存类型数量
    ICF_MEM_INFO       stMemInfo[ICF_MEM_TYPE_NUM]; /// 不同类型内存信息,最大内存池类型数ICF_MEM_TYPE_NUM
    void              *pMemSytemMallocInter;        /// 平台底层内存申请接口，接口定义见ICF_Interface.h中 ICF_MemAllocCB
    void              *pMemSytemFreeInter;          /// 平台底层内存释放接口，接口定义见ICF_Interface.h中 ICF_MemReleaseCB
    int                reserved[4];                 /// 预留字段
}ICF_MEM_CONFIG;

/// 缓存结构体，用于传入配置信息的buff（也可直接传入文件）
typedef struct _ICF_BUFF_
{  
    void               *pBuff;                      /// 缓存指针
    int                 nBuffSize;                  /// 缓存大小
}ICF_BUFF;

/**
 * 引擎配置文件路径结构体
 * 目前框架需要四个相互独立的配置json:1、算子配置 2、任务配置 3、工具配置 4、APP配置
 * 目前框架支持两种模式:1、文件模式 2、缓存模式 两种模式分别支持加密/不加密配置
 * 文件模式:需设置 pAlgCfgPath路径与bAlgCfgEncry是否加密等参数
 * 缓存模式:需设置 stAlgCfgBuff缓存与bAlgCfgEncry是否加密等参数
 * 三种配置均支持加密/解码输入,加解码工具可使用版本发布配套工具CfgDESTool(目录在util\CfgDESTool下)
 */
typedef struct _ICF_CONFIG_INFO_
{  
    char              *pAlgCfgPath;                 /// 引擎算子配置文件路径
    char              *pTaskCfgPath;                /// 引擎Task任务配置文件路径
    char              *pToolkitCfgPath;             /// 引擎工具配置文件路径
    //char              *pAppParamCfgPath;          /// 引擎应用参数配置路径(ICF 1.2.4后转移至create接口)
    
    int                bAlgCfgEncry;                /// 引擎算子配置是否加密            (加密1/不加密0)
    int                bTaskCfgEncry;               /// 引擎Task任务配置文件是否加密    (加密1/不加密0)
    int                bToolkitCfgEncry;            /// 引擎工具配置文件是否加密        (加密1/不加密0)
    //int                bAppParamCfgEncry;         /// 引擎应用参数配置路径是否加密    (加密1/不加密0)(ICF 1.2.4后转移至create接口)

    ICF_BUFF           stAlgCfgBuff;                /// 引擎算子配置缓存信息
    ICF_BUFF           stTaskCfgBuff;               /// 引擎Task任务配置缓存信息
    ICF_BUFF           stToolkitCfgBuff;            /// 引擎工具配置缓存信息
    //ICF_BUFF           stAppParamCfgBuff;           /// 引擎应用配置缓存信息(ICF 1.2.4后转移至create接口)
    
    int                reserved[4];                 /// 预留字段
}ICF_CONFIG_INFO;

/// 引擎初始化结构体 ICF_Init传入参数
typedef struct _ICF_INIT_PARAM_
{
    ICF_MEM_CONFIG     stMemConfig;                 /// 内存信息
    ICF_CONFIG_INFO    stConfigInfo;                /// 配置信息
    void              *pScheduler;                  /// 调度器(只做透传,不做校验)
    int                reserved[4];                 /// 预留字段
}ICF_INIT_PARAM;

/// 引擎运行时参数限制
typedef struct ICF_LIMIT_INPUT_
{
    void              *pInitParam;                  /// [输入]参考ICF_Init的pInitParam
    int                nGraphType;                  /// [输入]参考ICF_Create的nGraphType
    int                nGraphIDNum;                 /// [输入]需要评估的通道数目
    void              *pAppParam[GRAPH_ID_COUNT];   /// [输入]参考ICF_Create的pAppParam
    int                nGraphID[GRAPH_ID_COUNT];    /// [输入]通道ID数组,接口尽可能的输出和该数组中通道
                                                    ///       顺序一致的创建过程需要的内存大小
    int                nMaxCacheNum[GRAPH_ID_COUNT];/// [输入]通道内最大缓存 <=0:无效，该数据可控制统计内存的精确度
}ICF_LIMIT_INPUT;

/// 引擎运行时内存参数限制
typedef struct ICF_MEM_LIMIT_
{
    ICF_MEM_INFO       stMemCreate[ICF_MEM_TYPE_NUM];/// 创建时内存信息,
    ICF_MEM_INFO       stMemProc[ICF_MEM_TYPE_NUM];  /// 运行时内存信息
    ICF_MEM_INFO       stMemTotal[ICF_MEM_TYPE_NUM]; /// 总内存信息，开启内存越界检测时该值可能偏小
}ICF_MEM_LIMIT;

/// 引擎运行时参数限制
typedef struct ICF_LIMIT_OUTPUT_
{
    ICF_MEM_LIMIT      stMemLimit[GRAPH_ID_COUNT];  /// [输出]各通道内存限制,仅创建内存(算子创建)、运行内存(算子申请)有效
    ICF_MEM_LIMIT      stMemTotal;                  /// [输出]总内存限制，创建内存、运行内存、总内存有效，设置ICF_Init参数时可参考。
    int                reserved[6];                 /// 预留字段
}ICF_LIMIT_OUTPUT;

/// 系统（全局）时间结构体 (兼容后端解码功能,可忽略)
typedef struct  _ICF_SYSTEMTIME_
{
    unsigned short     sYear;                       /// 年
    unsigned short     sMonth;                      /// 月
    unsigned short     sDayOfWeek;                  /// 周
    unsigned short     sDay;                        /// 日
    unsigned short     sHour;                       /// 时
    unsigned short     sMinute;                     /// 分
    unsigned short     sSecond;                     /// 秒
    unsigned short     sMilliseconds;               /// 毫秒
} ICF_SYSTEMTIME;

/// 解码原始数据 (兼容后端解码功能,可忽略)
typedef struct _ICF_MEDIA_RAW_DATA_
{
    unsigned int       nWidth;                      /// 宽
    unsigned int       nHeight;                     /// 高
    unsigned int       nFrameNum;                   /// 帧号
    unsigned int       nFrameRate;                  /// 帧率
    FRAMETYPE          enFrameType;                 /// 帧类型
    unsigned char*     pFrameData;                  /// 数据缓存
    unsigned int       nFrameLen;                   /// 数据长度
    unsigned int       nFrameTime;                  /// 帧时间
    ICF_SYSTEMTIME     stSysTime;                   /// 系统时间
}ICF_MEDIA_RAW_DATA;

/// 媒体数据结构体 (兼容后端解码功能,可忽略)
typedef struct _ICF_MEDIA_DATA_
{
    ICF_BLOB_FORMAT_E  nType;                       /// 解码后得到的数据类型
    int                nWidth;                      /// 宽度
    int                nHeight;                     /// 高度
    int                nFrameRate;                  /// 帧率
    unsigned int       nTimeStamp;                  /// 时间戳
    ICF_SYSTEMTIME     stSysTime;                   /// 系统时间
    int                nFrameNum;                   /// 帧号
    unsigned int       nPitchY;                     /// 图像y处理跨度
    unsigned int       nPitchUV;                    /// 图像uv处理跨度
    int                nLen;                        /// 数据长度
    unsigned char*     pBuffer;                     /// 缓存
    unsigned char*     pUv;                         /// uv地址, 只有解码带pitch时有用
    void*              pSurFace;                    /// 解码surface，
    void*              pHandle;                     /// handle
    void*              pPrivate;                    /// 私有指针
}ICF_MEDIA_DATA;

/// (兼容后端解码功能,可忽略)
typedef struct _ICF_DECODE_DATA_
{
    int                nRawCnt;                     /// 个数
    ICF_MEDIA_RAW_DATA stRawData[ICF_MAX_SRC_DATA_CNT];/// 原始数据，输入的图片数据或解码出来的原始码流
    ICF_MEDIA_DATA     stSrcDecodeData;             /// 解码数据, 原始宽高
    ICF_MEDIA_DATA     stDecodeData;                /// 解码数据，降采样解码数据
    void              *pUserData;                   /// 用户定义数据
    int                nSize;                       /// 大小
}ICF_DECODE_DATA;

/// 源数据信息结构体
typedef struct _ICF_SOURCE_BLOB_
{
    ICF_BLOB_FORMAT_E  eBlobFormat;                 /// 源数据类型
    long long          nFrameNum;                   /// 帧号
    long long          nTimeStamp;                  /// 时间戳
    int                nDataType;                   /// 用户自定义,可用于区分同类型数据下不同的源
    int                nSpace;                      /// 内存类型
    void              *pData;                       /// 源数据指针
    int                nShape[ICF_MAX_BLOB_DIM];    /// 数据各维度尺寸[宽、高、通道、batch]
    int                nStride[ICF_MAX_BLOB_DIM];   /// 数据各维度尺寸[宽、高、通道、batch]
    int                reserved[6];                 /// 预留字段
} ICF_SOURCE_BLOB;

/**
 * 输入数据对应的数据以及附带的数据参数
 * ICF_Input_data接口输入结构体
 */
typedef struct _ICF_INPUT_DATA_
{
    int                nBlobNum;                          /// 输入BLOB数量, 最多16个
    ICF_SOURCE_BLOB    stBlobData[ICF_MAX_INPUT_BLOB_NUM];/// 多个输入源数据
    int               *pUseFlag[ICF_MAX_INPUT_BLOB_NUM];  /// 标记框架是否用完该源数据数据
    void              *pUserPtr;                          /// 用户指针,用于透传用户需要信息
    int                nUserDataSize;                     /// pUserPtr指向结构体大小
    int                reserved[5];                       /// 预留字段
} ICF_INPUT_DATA;

/// 多媒体资源管理信息，对应类型是ICF_ANA_MEDIA_DATA
typedef struct _ICF_MEDIA_INFO_
{
    ICF_INPUT_DATA     stInputInfo;                       /// 用户输入数据
    ICF_DECODE_DATA    stDecodeInfo;                      /// 解码相关数据(兼容后端解码功能,可忽略)
}ICF_MEDIA_INFO;

/// 用于统一包装算子输出数据或源数据
typedef struct ICF_ANA_DATA_
{
    void              *pData;                             /// 数据指针,源数据或算子输出内存
    int                nSize;                             /// 数据内存大小
    int                nNodeID;                           /// 产生该数据的节点ID
    int                nDataType;                         /// 用户自定义,标识pData类型,参考ICF_ANA_DATA_TYPE_E分段
    ICF_MEM_TYPE_E     nSpace;                            /// 内存类型
    int                reserved[3];                       /// 预留字段
}ICF_DATA_PACK;

/// 节点标签结构体
typedef struct _ICF_NODE_LABEL_
{
    int                nNodeID;                           /// 节点ID, 必须配置
    int                nAlgType;                          /// 算子标签，不支持用于查找节点
    char               szNodeName[NODE_NAME_LEN];         /// 节点名称，不支持用于查找节点
    int                reserved[6];                       /// 预留字段
}ICF_NODE_LABEL;

/// 系统内存申请接口吐出的内存指针
typedef struct _ICF_MEM_BUFFER_
{
    void               *pVirMemory;                       /// 可用内存区的虚拟起始指针
    void               *pPhyMemory;                       /// 可用内存区的物理地址指针
} ICF_MEM_BUFFER;

/// 内存池状态结构体
/// 工具接口中,查看内存池状态 MemPoolMemGetStatus 返回信息结构体
typedef struct _ICF_MEMPOOL_STATUS_
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
} ICF_MEMPOOL_STATUS;

/// 版本信息结构体
typedef struct _ICF_VERSION_
{
    unsigned int        nMajorVersion;                    /// 主版本号
    unsigned int        nSubVersion;                      /// 子版本号
    unsigned int        nRevisVersion;                    /// 修订版本号

    unsigned int        nVersionYear;                     /// 版本日期-年
    unsigned int        nVersionMonth;                    /// 版本日期-月
    unsigned int        nVersionDay;                      /// 版本日期-日
}ICF_VERSION;

// APP 参数信息
typedef struct _ICF_APP_PARAM_INFO_
{
    char              *pAppParamCfgPath;                  /// 引擎应用参数配置路径
    int                bAppParamCfgEncry;                 /// 引擎应用参数配置路径是否加密    (加密1/不加密0)
    ICF_BUFF           stAppParamCfgBuff;                 /// 引擎应用配置缓存信息
} ICF_APP_PARAM_INFO;

#ifdef __cplusplus
}
#endif 

#endif  /* _ICF_BASE_H_ */
