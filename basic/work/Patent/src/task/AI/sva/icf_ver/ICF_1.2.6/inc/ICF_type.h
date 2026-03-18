/** @file ICF_types.h
 *  @note Hikvision Digital Technology Co., Ltd. All Rights Reserved
 *  @brief 公共配置
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
    ICF_SYS_SET_SYSTEM_EVENT    = 0,      // 系统初始化后自动调用
    ICF_SYS_SET_RESET           = 1,      // 通道队列重置
    ICF_SYS_SET_NODE_CTRL       = 2,      // 启动/暂停某个节点0：启动 1：暂停
    ICF_SYS_SET_SYSTEM_DESTROY  = 3,      // int 1:系统销毁通知,算子如有挂起需求需要响应该配置项退出挂起状态
    ICF_SYS_DATA_CALL_BACK      = 4,      // 设置回调函数开关,自动调用
    ICF_SYS_MEM_RELEASE_CB      = 5,      // 设置释放函数开关,自动调用
    ICF_SYS_PARA_NODE_SPLIT     = 6,      // 并行节点设置最大拆包数
    ICF_SYS_DYNAMIC_ROUTE_CB    = 7,      // 设置动态路由回调开关,自动调用
    ICF_SYS_BATCHNUM            = 8,      // 设置多Batch节点BatchNumber
    ICF_SYS_BATCH_OVERTIME      = 9,      // 设置多Batch节点超时等待时间，单位(ms)
	ICF_SYS_SHARE_NODE_CFG_EXEC = 10,     // 配置共享节点配置是否进行set/get测试 
                                          // 默认0：set/get时会进行测试，1：不测试，仅Process时执行
    ICF_SYS_CFG_CMD_END         = 99,     // 系统键值最大值

    // 用户自定义100~ICF_CFG_AMD_END
    ICF_CFG_AMD_END             = 0x7fffffff,// 用户配置键值最大值
}ICF_KEY_CONFIG;

// 数据包算法处理中间结果,数据类型alg_type, 枚举规划参考《智能计算框架ICF使用规范》
typedef enum _ICF_ANA_DATA_TYPE_E
{
    // 引擎系统使用[0~99]段,用于标记框架产生的数据,需保证与引擎NODE使用alg_type不冲突
    ICF_ANA_GRAPH_DATA          = 0,    // SYSTEM(内部)
    ICF_ANA_MEDIA_DATA          = 1,    // ICF_MEDIA_INFO
    ICF_ANA_SYNC_DATA           = 2,    // ICF_COMBINE_DATA
    ICF_ANA_GBUF_DATA           = 3,    // 全局缓存内存
    ICF_ANA_POST_DATA           = 4,    // post节点
    ICF_ANA_DATA_END            = 99,   // 最大保留数据类型

    // 图像预处理算法 [100, 200)
    ICF_ANA_SYN1_DATA            = 103,  // 同步节点1
    ICF_ANA_SYN2_DATA            = 104,  // 同步节点2
    ICF_ANA_SYN3_DATA            = 105,  // 同步节点3
    ICF_ANA_ROUTE1_DATA          = 106,  // 路由节点1
    ICF_ANA_ROUTE2_DATA          = 107,  // 路由节点2
    ICF_ANA_ROUTE3_DATA          = 108,  // 路由节点3

    // 人脸算法 [200, 300)
    ICF_ANA_FDDET_DATA          = 200,  // 海思FD检测
    ICF_ANA_FDTRK_DATA          = 201,  // 海思FD跟踪
    ICF_ANA_FDQUA_DATA          = 202,  // 海思FD检测

    ICF_ANA_FRLANDMAKE_DATA     = 203,  // NT FR关键点
    ICF_ANA_FRQUALITY_DATA      = 204,  // NT FR评分 评分校验
    ICF_ANA_FRFEATURE_DATA      = 205,  // NT FR特征
    ICF_ANA_FRLIVENESS_DATA     = 206,  // NT FR活体
    ICF_ANA_FRCOMPARE_DATA      = 207,  // NT FR比对
    ICF_ANA_FDDETECT_DATA       = 208,  // NT FD检测
    
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

    // 开放平台算法 [300, 400)

    // HMS算法 [400, 500)

    // 雷球算法 [500, 600)

    // 周界算法 [600, 700)

    // 雷达算法 [700, 800)

    // 多传感算法 [1000, 2000)
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

// 源/输入数据格式 输入结构体 ICF_SOURCE_BLOB 中eBlobFormat成员,表示源数据类型
typedef enum _ICF_BLOB_FORMAT_E
{
    ICF_INPUT_FORMAT_VIDEO_PIC  = 1,    // VIDEO_PIC
    ICF_INPUT_FORMAT_VIDEO_HEAD = 2,    // VIDEO_HEADER
    ICF_INPUT_FORMAT_VIDEO_DATA = 3,    // VIDEO_DATA
    ICF_INPUT_FORMAT_VIDEO_END  = 4,    // VIDEO_END
    ICF_INPUT_FORMAT_YUV_NV21   = 5,    // YUV_NV21
    ICF_INPUT_FORMAT_YUV_NV12   = 6,    // YUV_NV12
    ICF_INPUT_FORMAT_YUV_I420   = 7,    // YUV_I420
    ICF_INPUT_FORMAT_YUV_YV12   = 8,    // YUV_YV12
    ICF_INPUT_FORMAT_SENSOR     = 9,    // 传感器数据
    ICF_INPUT_FORMAT_AUDIO      = 10,   // 音频数据
    ICF_INPUT_FORMAT_AMOUNT
} ICF_BLOB_FORMAT_E;

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
    ICF_MEM_ANYTYPE,                     // 任意类型内存，不可用于ICF_init创建内存池，只支持内存池接口分配和释放
    ICF_MEM_END
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
