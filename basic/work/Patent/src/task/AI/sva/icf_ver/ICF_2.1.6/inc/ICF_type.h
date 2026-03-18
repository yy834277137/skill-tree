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
    ICF_ANA_MEDIA_DATA          = 1,    // ICF_MEDIA_INFO
    ICF_ANA_BLOB_DATA           = 2,    // ICF_SOURCE_BLOB
    ICF_ANA_SYNC_DATA           = 3,    // ICF_COMBINE_DATA
    ICF_ANA_POST_DATA           = 5,    // post节点
    ICF_ANA_DATA_END            = 99,   // 最大保留数据类型

    // 图像预处理算法 [100, 200)
    ICF_ANA_SYN1_DATA            = 103,  // 同步节点1
    ICF_ANA_SYN2_DATA            = 104,  // 同步节点2
    ICF_ANA_SYN3_DATA            = 105,  // 同步节点3
    ICF_ANA_ROUTE1_DATA          = 106,  // 路由节点1
    ICF_ANA_ROUTE2_DATA          = 107,  // 路由节点2
    ICF_ANA_ROUTE3_DATA          = 108,  // 路由节点3

    // 智慧安检相关NODE序号
    ICF_ANA_OBD_DATA_1          = 350,  // OBD检测
    ICF_ANA_OBD_DATA_2          = 351,  // OBD检测
    ICF_ANA_AIOBD_DATA_1        = 352,  // AIOBD检测
    ICF_ANA_AIOBD_DATA_2        = 353,  // AIOBD检测
    ICF_ANA_PD_DATA_1           = 354,  // 包裹检测
    ICF_ANA_PD_DATA_2           = 355,  // 包裹检测
    ICF_ANA_CLS_DATA_1          = 356,  // 违禁品分类
    ICF_ANA_CLS_DATA_2          = 357,  // 违禁品分类
    ICF_ANA_XSI_DATA            = 358,  // XSI报警

} ICF_ANA_DATA_TYPE_E;

// 回调类型,ICF_Set_callback调用中参数nCallbackType
typedef enum _ICF_CALLBACK_TYPE_
{
    ICF_CALLBACK_OUTPUT         = 0,    // 输出结果回调
    ICF_CALLBACK_RELEASE_SOURCE = 1,    // 输入源释放回调
    ICF_CALLBACK_DYNAMIC_ROUTE  = 2,    // 节点输出动态路由回调
    ICF_CALLBACK_ABNORMAL_ALARM = 3,    // 异常报警回调
    ICF_CALLBACK_END                    // 回调类型最大值
}ICF_CALLBACK_TYPE;

// 源/输入数据格式 输入结构体 ICF_SOURCE_BLOB 中eBlobFormat成员,表示源数据类型(用户可在此基础上自行加入其它数据类型)
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

#ifdef __cplusplus
}
#endif

#endif /* _ICF_TYPE_H_ */
