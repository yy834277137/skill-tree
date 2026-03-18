/** @file ICF_types.h
 *  @note Hikvision Digital Technology Co., Ltd. All Rights Reserved
 *  @brief 公共配置
 *
 *  @author    曹贺磊
 *  @version   1.0.2
 *  @date      2020/06/16
 *  @note      增加ICF_BLOB_DATA_TYPE_E ICF_BLOB_LAYOUT_FORMAT_E枚举类型；
 *             增加ROI相关枚举；
 *             取消用户config键值最大值限制；
 * 
 *  @author    刘锦胜
 *  @version   1.0.1
 *  @date      2020/01/10
 *  @note      加入相关注释，增加易用性
 *
 *  @author    曹贺磊
 *  @version   1.0.0
 *  @date      2019/11/14
 *  @note      按照规范修改
 *
 *  @author    郭俞江
 *  @version   0.9.1
 *  @date      2017/7/11
 *  @note      创建
 */


#ifndef _ICF_TYPE_H_
#define _ICF_TYPE_H_

#ifndef ICF_CALLBACK
#define ICF_CALLBACK
#else 
#undef ICF_CALLBACK
#define ICF_CALLBACK
#endif

#if (defined(_WIN32) || defined(_WIN64))
#define ICF_EXPORT __declspec(dllexport)
#elif (defined(__linux__) || defined(__ANDROID__))
#define ICF_EXPORT __attribute__((visibility("default")))
#else
#define ICF_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 设置参数枚举, ICF_Set_config调用时配置的系统参数(key)
typedef enum _ICF_KEY_CONFIG_
{
    // 引擎系统使用[0~99]段,需保证与引擎算法配置key不冲突
    ICF_SYS_CFG_CMD_START       = 0,
    ICF_SYS_SET_SYSTEM_DESTROY  = 1,      // int 1:系统销毁通知,算子如有挂起需求需要响应该配置项退出挂起状态
    ICF_SYS_SET_RESET           = 2,      // [用户可控]通道队列重置
    ICF_SYS_SET_NODE_CTRL       = 3,      // [用户可控]启动/暂停某个节点0：启动 1：暂停
    ICF_SYS_MEM_RELEASE_CB      = 5,      // [用户可控]设置释放函数开关,自动调用
    ICF_SYS_PARA_NODE_SPLIT     = 6,      // [用户可控]并行节点设置最大拆包数
    ICF_SYS_BATCHNUM            = 8,      // [用户可控]设置多Batch节点BatchNumber
    ICF_SYS_BATCH_OVERTIME      = 9,      // [用户可控]设置多Batch节点超时等待时间，单位(ms)
	ICF_SYS_SHARE_NODE_CFG_EXEC = 10,     // [用户可控]配置共享节点配置是否进行set/get测试 
                                          // 默认0：set/get时会进行测试，1：不测试，仅Process时执行
    ICF_SYS_NODE_CONFIG         = 11,     // [用户可控]节点内存配置
    ICF_SYS_SYSTEM_LOG          = 12,     // [用户可控]系统日志配置
    ICF_SYS_CFG_CMD_END         = 99,     // 系统键值最大值

    // 算子相关100~ICF_CFG_AMD_END，用户可扩展
    ICF_HVA_CFG_MODULE_INFO     = 100,     // HVA引擎信息, 见HVA_CONFIG_INFO
    ICF_COMP_CFG_MODULE_INFO    = 101,     // HVA计算图中某个节点模块信息, 见HVA_CONFIG_INFO

    ICF_CFG_AMD_END             = 0x7fffffff,// 用户配置键值最大值

}ICF_KEY_CONFIG;

// 共享节点配置项的模式
typedef enum _ICF_CONFIG_MODE_
{
    ICF_CFG_MODE_DEFAULT       = 0,         // Process时执行Set，每个通道执行完会将默认配置复位，和1.2.x、1.3.x行为兼容，
                                            // 有大量默认配置，不同通道对于这些配置要求不一样时可用该模式
    ICF_CFG_MODE_SAME_NOSHARE  = 1,         // Set行为和非共享节点相同，Process期间不会执行Set，每次执行时仅执行一次Set
                                            // 多个通道可共用的配置项或者任何时刻只有一个通道工作时可用该模式
}ICF_CONFIG_MODE;

// 数据包算法处理中间结果,数据类型alg_type, 枚举规划参考《智能计算框架ICF使用规范》
typedef enum _ICF_ANA_DATA_TYPE_E
{
    // 引擎系统使用[0~99]段,用于标记框架产生的数据,需保证与引擎NODE使用alg_type不冲突
    ICF_ANA_GRAPH_DATA          = 0,    // SYSTEM(内部)
    ICF_ANA_MEDIA_DATA          = 1,    // ICF_MEDIA_INFO_V2
    ICF_ANA_BLOB_DATA           = 2,    // ICF_SOURCE_BLOB_V2
    ICF_ANA_SYNC_DATA           = 3,    // ICF_COMBINE_DATA
    ICF_ANA_POST_DATA           = 5,    // post节点
    ICF_ANA_DATA_END            = 99,   // 最大保留数据类型

} ICF_ANA_DATA_TYPE_E;

// 回调类型,ICF_Set_callback调用中参数nCallbackType
typedef enum _ICF_CALLBACK_TYPE_
{
    ICF_CALLBACK_OUTPUT         = 0,    // 输出结果回调
    ICF_CALLBACK_RELEASE_SOURCE = 1,    // 输入源释放回调
    ICF_CALLBACK_RELEASE_ALG    = 2,    // 算子结果释放回调
    ICF_CALLBACK_ABNORMAL_ALARM = 3,    // 异常报警回调
    ICF_CALLBACK_END                    // 回调类型最大值
}ICF_CALLBACK_TYPE;

// 源/输入数据格式 输入结构体 ICF_SOURCE_BLOB_V2 中eBlobFormat成员,表示源数据类型(用户可在此基础上自行加入其它数据类型)
typedef enum _ICF_BLOB_FORMAT_E
{
    ICF_INPUT_FORMAT_VIDEO_PIC     = 1,    // VIDEO_PIC
    ICF_INPUT_FORMAT_VIDEO_HEAD    = 2,    // VIDEO_HEADER
    ICF_INPUT_FORMAT_VIDEO_DATA    = 3,    // VIDEO_DATA
    ICF_INPUT_FORMAT_VIDEO_END     = 4,    // VIDEO_END
    ICF_INPUT_FORMAT_YUV_NV21      = 5,    // YUV_NV21
    ICF_INPUT_FORMAT_YUV_NV12      = 6,    // YUV_NV12
    ICF_INPUT_FORMAT_YUV_I420      = 7,    // YUV_I420
    ICF_INPUT_FORMAT_YUV_YV12      = 8,    // YUV_YV12
    ICF_INPUT_FORMAT_SENSOR        = 9,    // 传感器数据
    ICF_INPUT_FORMAT_AUDIO         = 10,   // 音频数据
    ICF_INPUT_FORMAT_HVA_METADATA  = 11,   // HVA源数据
    
    ICF_INPUT_FORMAT_AMOUNT        = 99    // 最大值限制，用户新增枚举不可大于该值
} ICF_BLOB_FORMAT_E;


// 源/输入数据LayoutFormat, ICF_SOURCE_BLOB中的eLayoutFormat成员
typedef enum _ICF_BLOB_LAYOUT_FORMAT_E_
{
    ICF_BLOB_LAYOUT_FORMAT_NCHW    = 0,                  // nchw 格式
    ICF_BLOB_LAYOUT_FORMAT_NHCW    = 1,                  // nhcw 格式
    ICF_BLOB_LAYOUT_FORMAT_NHWC    = 2,                  // nhwc 格式
    ICF_BLOB_LAYOUT_FORMAT_NCLHW   = 3,                  // nclhw 格式
    ICF_BLOB_LAYOUT_FORMAT_NHCLW   = 4,                  // nhclw 格式
    ICF_BLOB_LAYOUT_FORMAT_END
}ICF_BLOB_LAYOUT_FORMAT_E;


// 视频帧类型, 与后端解码兼容, 请忽略
typedef enum _FRAMETYPE_
{
    ICF_VF_NONE                 = 0,    // 无
    ICF_VF_I                    = 1,    // I帧
    ICF_VF_P                    = 2,    // P帧
    ICF_VF_B                    = 3,    // B帧
    ICF_VF_BP                   = 4,    // BP组
    ICF_VF_BBP                  = 5,    // BBP组
    ICF_VF_HP                   = 6,    // 深P帧
    ICF_VF_LP                   = 7     // 浅P帧
} FRAMETYPE;

// 快照处理状态, 同步节点/流控节点中用户定义的帧行为策略
typedef enum _ICF_PROC_STATE_
{
    ICF_PROC_REMOVE             = 1,    // 丢弃
    ICF_PROC_SAVE               = 2,    // 存回原队列
    ICF_PROC_SEND               = 3,    // 往后发送
    ICF_PROC_UNPROC_SEND        = 4,    // 不处理后发
    ICF_PROC_END                        // 最大值
}ICF_PROC_STATE;

// 快照存取状态，同步节点/流控节点中用户取数据包行为的标记
typedef enum _ICF_PACK_STATE_
{    
    ICF_PACK_OUT               = 0,    // 其它，数据包为空，或被取出过
    ICF_PACK_IN                = 1,    // 数据包不为空且未被取出
    ICF_PACK_END                       // 最大值
}ICF_PACK_STATE;

// 内存类型,当前ICF支持的内存类型,覆盖linux x86/海思/NT/K91平台
typedef enum _ICF_MEM_TYPE_E
{
    ICF_MEM_MALLOC,                      // malloc类型的内存
    ICF_HISI_MEM_MMZ_WITH_CACHE,         // 海思平台MMZ内存带cache
    ICF_HISI_MEM_MMZ_NO_CACHE,           // 海思平台MMZ内存不带cache
    ICF_HISI_MEM_MMZ_WITH_CACHE_PRIORITY,// 海思平台MMZ内存带cache, 优先申请3.2G内存以内
    ICF_HISI_MEM_MMZ_NO_CACHE_PRIORITY,  // 海思平台MMZ内存不带cache, 优先申请3.2G内存以内
    ICF_K91_MEM_FEATURE,                 // K91平台特有内存
    ICF_P4_MEM_FEATURE,                  // P4平台特有内存
    ICF_NT_MEM_FEATURE,                  // NT平台特有内存
    ICF_MLU_MEM_FEATURE,                 // 寒武纪mlu内存
    ICF_T4_MEM_GPU,                      // T4平台GPU
    ICF_H8_MEM_MMZ_WITH_CACHE,           // H8平台MMZ带cache
    ICF_H8_MEM_MMZ_NO_CACHE,             // H8平台MMZ不带cache
    ICF_MEM_ANYTYPE,                     // 任意类型内存，不可用于ICF_init创建内存池，只支持内存池接口分配和释放
    ICF_HISI_MEM_VB_REMAP_NONE,          // 海思VB内存类型
    ICF_HISI_MEM_VB_REMAP_NOCACHE,       // 海思VB内存类型
    ICF_HISI_MEM_VB_REMAP_CACHED,        // 海思VB内存类型
    ICF_H9_MEM_MMZ_WITH_CACHE,           // H9平台MMZ内存带cache
	ICF_H9_MEM_MMZ_NO_CACHE,             // H9平台MMZ内存不带cache
    ICF_RN_MEM_MMZ_CMA_WITH_CACHE,       // RN平台CMA内存带cache，其中CMA内存为物理连续内存
	ICF_RN_MEM_MMZ_CMA_NO_CACHE,         // RN平台CMA内存不带cache
	ICF_RN_MEM_MMZ_IOMMU_WITH_CACHE,     // RN平台IOMMU内存带cache，其中IOMMU内存为物理非连续内存
	ICF_RN_MEM_MMZ_IOMMU_NO_CACHE,       // RN平台IOMMU内存不带cache
    ICF_MEM_END = 32                     // 最大内存类型数目，自定义内存类型不要超过这个数
} ICF_MEM_TYPE_E;

// 异常报警级别
typedef enum _ICF_ABNORMAL_LEVEL_
{
    ICF_ABNORMAL_SYSTEM           = 0,                       // 系统异常（包括内存释放错误）
    ICF_ABNORMAL_ALGPROCESS       = 1,                       // 算子处理返回错误码
    ICF_ABNORMAL_QUEENOVERFLOW    = 2,                       // 队列满
    ICF_ABNORMAL_QUEENCOVER       = 3,                       // 队列覆盖
    ICF_ABNORMAL_MEMORYLACK       = 4                        // 内存不足
}ICF_ABNORMAL_LEVEL;

// ICF框架状态信息类型枚举
typedef enum _ICF_STATUS_DATATYPE
{
    ICF_CHAR       = 0,
    ICF_INT16_T    = 1,
    ICF_INT32_T    = 2,
    ICF_INT64_T    = 3,
    ICF_LONG_LONG  = 4,
    ICF_TYPE_END   = 10

}ICF_STATUS_DATATYPE;


// ICF框架状态信息枚举
typedef enum _ICF_STATUS_KEY
{
    ICF_GRAPH_TYPE             = 0,    // 计算图类型
    ICF_GRAPH_ID               = 1,    // 配置文件中的图ID, 该状态也指当前数据包的通道ID
    ICF_GRAPH_LABEL            = 2,    // 图所属通道的编号
    ICF_GRAPH_TAG              = 3,    // 任务图的来源
    ICF_NODE_ID                = 4,    // 节点ID, 该状态也指目标节点ID、当前方向顶点对应的节点ID、被初始化的节点ID、调用当前算子的节点ID
    ICF_NODE_PORT              = 5,    // 节点端口, 该状态也指目标节点端口
    ICF_PORT_NUM               = 6,    // 端口数量
    ICF_QUEUE_BEHAVE           = 7,    // 队列行为, 该状态也指目标节点端口队列行为、反馈节点的队列行为
    ICF_QUEUE_LENGTH           = 8,    // 队列长度, 该状态也指目标节点端口队列最大长度、反馈输入队列的长度
    ICF_FRAME_NUM              = 9,    // 帧号
    ICF_PKG_STATE              = 10,   // 数据包状态
    ICF_SEND_NUM               = 11,   // 要发送的数据包个数
    ICF_ALG_ID                 = 12,   // 算子ID, 该状态也指被初始化节点的算子ID、算子插件ID
    ICF_PLUGIN_TYPE            = 13,   // 算子插件类型
    ICF_ALG_BATCHES            = 14,   // 拼帧数
    ICF_CONFIG_KEY             = 15,   // 配置关键字
    ICF_CONFIG_SIZE            = 16,   // 配置参数大小
    ICF_FUNC_RET               = 17,   // 算子句柄处理结果或算子单包处理结果
    ICF_PORT_INDEX             = 18,   // 端口下标
    ICF_ALG_INDEX              = 19,   // 算子序号
    ICF_QUIT_INPUT             = 20,   // 节点销毁时调度层通知退出InputData
    ICF_SEND_FLAG              = 21,   // 反馈节点是否向后续节点发送数据
    ICF_GET_VALID_DATA         = 22,   // 端口中是否拿到了有效数据
    ICF_DATA_COUNT             = 23,   // 添加任务计数
    ICF_THREAD_WAIT            = 24,   // 线程是否等待激活, 0表示已激活, 1表示等待激活
    FRAMEWORK_STATUS_END       = 500   // 枚举结束

}ICF_STATUS_KEY;


// ------------------------------计算图层执行点位定义------------------------------
// 计算图的异步输入接口内，节点while循环输入数据。
// 该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；帧号；数据包状态；目标节点ID；目标节点端口；目标节点端口队列行为；目标节点端口队列最大长度。
#define GRAPH0000000        (0x10000000)

// 计算图的同步输入接口内，顶点的节点while循环输入数据。
// 该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；帧号；数据包状态；当前方向顶点对应的节点名称、节点ID。
#define GRAPH0000001        (0x10000001)

// 远程设备上创建子图。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号。
#define GRAPH0000002        (0x10000002)

// 初始化节点。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；被初始化的节点ID、节点名称、算子ID。
#define GRAPH0000003        (0x10000003)

// 销毁子图。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号。
#define GRAPH0000004        (0x10000004)

// 通知节点退出时删除任务。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；节点ID。
#define GRAPH0000005        (0x10000005)

// 配置本地节点。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；节点ID；配置关键字。
#define GRAPH0000006        (0x10000006)

// 配置远程设备节点。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；任务图的来源；节点ID；配置关键字。
#define GRAPH0000007        (0x10000007)

// 全部节点配置。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；节点ID；配置关键字。
#define GRAPH0000008        (0x10000008)

// 非本地设备配置。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；任务图的来源；节点ID；配置关键字。
#define GRAPH0000009        (0x10000009)

// 本地节点获取配置。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；节点ID；配置关键字。
#define GRAPH0000010        (0x10000010)

// 远程节点获取配置。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；任务图的来源；节点ID；配置关键字。
#define GRAPH0000011        (0x10000011)

// 远程设备设置回调。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；任务图的来源；节点ID。
#define GRAPH0000012        (0x10000012)

// 本地计算图数据输出回调。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；帧号；节点ID。
#define GRAPH0000013        (0x10000013)

// 远程计算图数据输出回调。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；任务图的来源；帧号；节点ID。
#define GRAPH0000014        (0x10000014)

// 跨设备分发任务给其他设备。该点位新增或更新的状态包括：计算图类型；配置文件中图ID；图所属通道的编号；帧号；数据包状态；目标节点ID；目标节点端口。
#define GRAPH0000015        (0x10000015)


// ------------------------------节点层执行点位定义------------------------------
// 共享算子SetConfig时，因Set行为和非共享节点相同，所以需要while循环获取算子句柄。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID。
#define NODE0000000         (0x20000000)

// 共享算子SetConfig时，因共享节点不仅在process时执行cfg，所以需要while循环获取算子句柄。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID。
#define NODE0000001         (0x20000001)

// while循环获取算子句柄。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID。
#define NODE0000002         (0x20000002)

// 共享算子SetConfig时，因为共享节点不仅在process时执行cfg，所以将配置信息配置到当前算子后配置还需归还算子。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字。
#define NODE0000003         (0x20000003)

// 将config信息更新至表中。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字；配置参数大小。
#define NODE0000004         (0x20000004)

// 非共享算子SetConfig。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字。
#define NODE0000005         (0x20000005)

// 共享算子GetConfig。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字。
#define NODE0000006         (0x20000006)

// 非共享算子GetConfig。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字。
#define NODE0000007         (0x20000007)

// 算子单包处理(只进行前后处理，无算子信息)时的前处理。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；帧号；拼帧数。
#define NODE0000008         (0x20000008)

// 算子单包处理(只进行前后处理，无算子信息)时的后处理。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；帧号；拼帧数。
#define NODE0000009         (0x20000009)

// 算子单包处理(有算子信息)时开辟算子结果需要的内存。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；节点类型；算子ID；帧号；拼帧数。
#define NODE0000010         (0x20000010)

// 获取算子句柄。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；帧号。
#define NODE0000011         (0x20000011)

// 算子处理。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；帧号；拼帧数。
#define NODE0000012         (0x20000012)

// 设置算子运行时配置的过程中，算子句柄先获取配置，然后设置配置。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字；配置参数大小。
#define NODE0000013         (0x20000013)

// 配置归还算子时获取旧的配置信息。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字。
#define NODE0000014         (0x20000014)

// 配置归还算子时设置旧的配置信息。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字。
#define NODE0000015         (0x20000015)

// 配置归还算子时设置新的配置信息。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字。
#define NODE0000016         (0x20000016)

// 配置归还算子时再次设置旧的配置信息。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字。
#define NODE0000017         (0x20000017)

// 算子单包处理(有算子信息)时的前处理。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；帧号；拼帧数。
#define NODE0000018         (0x20000018)

// 算子单包处理(有算子信息)时的后处理。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；帧号；拼帧数。
#define NODE0000019         (0x20000019)

// 还原算子运行时配置。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；帧号。
#define NODE0000020         (0x20000020)

// 共享算子动态获取算子句柄。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID。
#define NODE0000021         (0x20000021)

// 还原算子配置时算子句柄SetConfig。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；配置关键字。
#define NODE0000022         (0x20000022)

// 多类型输入合并节点输入数据。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；节点端口号。
#define NODE0000023         (0x20000023)

// 往多类型输入合并节点的队列中输入数据。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；节点端口号。
#define NODE0000024         (0x20000024)

// 多类型输入合并节点的输入队列添加数据包。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；节点端口号。
#define NODE0000025         (0x20000025)

// 多类型输入合并节点在外部同步策略下的算子处理(算子句柄非空)。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号。
#define NODE0000026         (0x20000026)

// 多类型输入合并节点在外部同步策略下的算子单包处理(算子句柄为空)。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号。
#define NODE0000027         (0x20000027)

// 多类型输入合并节点在外部同步策略下发送数据。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；算子句柄处理结果或算子单包处理结果；要发送的数据包个数。
#define NODE0000028         (0x20000028)

// 多类型输入合并节点在内部同步策略下，非阻塞获取数据时的while循环。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；端口下标；端口数量。
#define NODE0000029         (0x20000029)

// 多类型输入合并节点输入队列弹出数据包。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；端口下标。
#define NODE0000030         (0x20000030)

// 多类型输入合并节点在内部同步策略下发送数据。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号。
#define NODE0000031         (0x20000031)

// 解码节点在拷贝模式下的单包处理。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号。
#define NODE0000032         (0x20000032)

// 解码节点下发数据给后续节点。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号。
#define NODE0000033         (0x20000033)

// 并行节点的子节点发送数据回并行节点时的while循环。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号。
#define NODE0000034         (0x20000034)

// 反馈节点的反馈行为是ICF_NODEFEED_WAIT时的while循环。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；反馈节点是否向后续节点发送数据标志；反馈输入队列的长度。
#define NODE0000035         (0x20000035)

// 反馈节点输入数据。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；节点销毁时调度层是否通知退出InputData。
#define NODE0000036         (0x20000036)

// 经过反馈节点下游反馈从节点，将输入数据插入反馈链表。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；队列行为。
#define NODE0000037         (0x20000037)

// 未经过反馈节点下游反馈从节点。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；队列行为。
#define NODE0000038         (0x20000038)

// 在反馈节点输入数据接口内向调度器添加任务。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；添加任务计数。
#define NODE0000039         (0x20000039)

// 多输入节点的process内while循环判断端口中是否拿到了有效数据。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；端口中是否拿到了有效数据；端口。
#define NODE0000040         (0x20000040)

// 并行节点将数据包送入子节点时，输入队列已满，while循环重新选举，直到数据包送出。该点位新增或更新的状态包括：计算图类型；计算图ID；算子序号；帧号；数据包状态。
#define NODE0000041         (0x20000041)

// 并行节点在注册拆包函数的处理策略下，数据包压入拆包队列时的while循环。该点位新增或更新的状态包括：计算图类型；计算图ID；帧号；数据包状态。
#define NODE0000042         (0x20000042)

// 普通节点异步数据输入。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；节点销毁时调度层是否通知退出InputData。
#define NODE0000043         (0x20000043)

// 普通节点异步数据输入时，节点队列行为是可覆盖数据输入。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；端口。
#define NODE0000044         (0x20000044)

// 普通节点异步数据输入时，节点队列行为是覆盖后发。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；端口。
#define NODE0000045         (0x20000045)

// 普通节点异步数据输入时，节点队列行为是覆盖用户释放。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；端口。
#define NODE0000046         (0x20000046)

// 普通节点异步数据输入时，节点队列行为是不可覆盖数据输入。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；端口。
#define NODE0000047         (0x20000047)

// 普通节点Process时，若数据包的状态失败，仍需发送。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号。
#define NODE0000048         (0x20000048)

// 普通节点Process时，获取输入和解码数据。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；帧号。
#define NODE0000049         (0x20000049)

// 普通节点Process时，获取算子句柄。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；帧号。
#define NODE0000050         (0x20000050)

// 普通节点Process时，算子单包处理并返还句柄。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；算子ID；帧号。
#define NODE0000051         (0x20000051)

// 普通节点发送数据到下一个节点。该点位新增或更新的状态包括：计算图类型；计算图ID；节点ID；帧号；数据包状态。
#define NODE0000052         (0x20000052)

// 并行节点的子节点送数据结束。该点位新增或更新的状态包括：无。
#define NODE0000053         (0x20000053)


// ------------------------------算子层执行点位定义------------------------------
// 带运行时的算法分析。该点位新增或更新的状态包括：调用当前算子的节点ID；当前数据包的通道ID、计算图类型；算子插件ID；算子插件类型；帧号。
#define ALG0000000          (0x30000000)

// 算子Process加锁的情况下，检查原子变量。该点位新增或更新的状态包括：调用当前算子的节点ID；当前数据包的通道ID、计算图类型；算子插件ID；算子插件类型；帧号。
#define ALG0000001          (0x30000001)

// 封装算子处理。该点位新增或更新的状态包括：调用当前算子的节点ID；当前数据包的通道ID、计算图类型；算子插件ID；算子插件类型；帧号；数据包状态；拼帧数。
#define ALG0000002          (0x30000002)

// HVA算子处理。该点位新增或更新的状态包括：当前数据包的通道ID、计算图类型；算子插件ID；算子插件类型；帧号；数据包状态。
#define ALG0000003          (0x30000003)

// 算法配置。该点位新增或更新的状态包括：当前数据包的通道ID、计算图类型；算子插件ID；算子插件类型；配置关键字。
#define ALG0000004          (0x30000004)

// 算法配置检查原子变量。该点位新增或更新的状态包括：算子插件ID；算子插件类型。
#define ALG0000005          (0x30000005)

// 封装类型的算子插件设置参数。该点位新增或更新的状态包括：算子插件ID；算子插件类型；配置关键字。
#define ALG0000006          (0x30000006)

// HVA类型的算子插件设置参数。该点位新增或更新的状态包括：算子插件ID；算子插件类型；配置关键字。
#define ALG0000007          (0x30000007)

// 算法获取配置。该点位新增或更新的状态包括：算子插件ID；算子插件类型；配置关键字。
#define ALG0000008          (0x30000008)

// 算法获取配置检查原子变量。该点位新增或更新的状态包括：算子插件ID；算子插件类型。
#define ALG0000009          (0x30000009)

// 封装类型的算子插件获取设置参数。该点位新增或更新的状态包括：算子插件ID；算子插件类型；配置关键字。
#define ALG0000010          (0x30000010)

// HVA类型的算子插件获取设置参数。该点位新增或更新的状态包括：算子插件ID；算子插件类型；配置关键字。
#define ALG0000011          (0x30000011)

// 事件处理驱动器内检查原子变量。该点位新增或更新的状态包括：算子插件ID；算子插件类型。
#define ALG0000012          (0x30000012)


// ------------------------------调度层执行点位定义------------------------------
// 自由模式下线程等待激活。该点位新增或更新的状态包括：线程是否等待激活。
#define SCHED0000000        (0x40000000)

// 自由模式下线程函数处理任务。该点位新增或更新的状态包括：线程是否等待激活。
#define SCHED0000001        (0x40000001)

// 自由模式下线程停止。该点位新增或更新的状态包括：线程是否等待激活。
#define SCHED0000002        (0x40000002)

// 绑核模式下线程等待激活。该点位新增或更新的状态包括：线程是否等待激活。
#define SCHED0000003        (0x40000003)

// 绑核模式下线程函数处理任务。该点位新增或更新的状态包括：线程是否等待激活。
#define SCHED0000004        (0x40000004)

// 绑核模式下线程停止。该点位新增或更新的状态包括：线程是否等待激活。
#define SCHED0000005        (0x40000005)


#ifdef __cplusplus
}
#endif

#endif /* _ICF_TYPE_H_ */
