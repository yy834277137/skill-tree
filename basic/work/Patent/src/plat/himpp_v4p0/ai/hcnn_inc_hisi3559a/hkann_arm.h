/***********************************************************************************************************************
* 版权信息：版权所有(c), 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：hkann_arm.h
* 摘    要：ARM平台深度学习通用算法库统一接口对外接口头文件, ARM平台私有头文件
*
* 当前版本：0.8.0
* 作    者：严杭琦
* 日    期：2019-02-14
* 备    注：初始版本
*          
***********************************************************************************************************************/
#ifndef _HKANN_ARM_H_
#define _HKANN_ARM_H_

#include "vca_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
#define HKANN_ARM_MAJOR_VERSION                  1                  // ARM_CNN库主版本号
#define HKANN_ARM_SUB_VERSION                    4                  // ARM_CNN库副版本号
#define HKANN_ARM_REVISION_VERSION               0                  // ARM_CNN库修订版本号

#define HKANN_ARM_HANDLE_NUM                    (1)                 // ARM_CNN库创建时需要输入的句柄个数（暂不需要调度句柄）
#define HKANN_ARM_BLOB_DIM                      (5)                 // ARM_CNN库blob维度，0~3表示NCHW，第4维预留
#define HKANN_ARM_AUX_MEM_SIZE                  (3 * 1024 * 1024)   // ARM_CNN库GetMemSize所需辅助内存大小

/***********************************************************************************************************************
* 枚举值
***********************************************************************************************************************/
// 处理线程绑核策略
// 适用平台：安卓、linux、ios(选择NONE选项)
// 注意：ACNN内部将查找CPU信息文件来判断CPU属于大核还是小核，对于无法读取CPU信息的平台，ACNN在HKANN_ARM_AFFINITY_BIG下
// 按照cpun到cpu0的顺序进行亲核心设置，在HKANN_ARM_AFFINITY_LITTLE下按cpu0到cpun的顺序设置亲核性；
// 常见平台小核编号出现在大核之前，比如2个A53（小核）和2个A73（大核）的平台，cpu0到cpu依次为A53 A53 A73 A73，
// 此时，设置亲核性策略后ACNN可正确进行绑定；
// 但对于小核编号出现在大核之后的情况，HKANN_ARM_AFFINITY_BIG下ACNN将绑定小核，HKANN_ARM_AFFINITY_LITTLE为大核；
// 最后，对于大小核不按簇编号的情况，亲核性设置结果无法预测，算法库性能受制于小核性能；
typedef enum _HKANN_ARM_AFFINITY_POLICY_
{
    HKANN_ARM_AFFINITY_BIG                     = 0,                 // 优先绑大核
    HKANN_ARM_AFFINITY_LITTLE                  = 1,                 // 优先绑小核
    HKANN_ARM_AFFINITY_NONE                    = 2,                 // 不做亲核性设置，全凭系统调度
    HKANN_ARM_AFFINITY_WEIGHTED                = 3,                 // 能力加权多核任务划分
    HKANN_ARM_AFFINITY_END                     = VCA_ENUM_END
} HKANN_ARM_AFFINITY_POLICY;

// ARM平台通用接口状态码
// HKANN_ARM 错误码，范围[0x86201000 0x86201FFF]，共4096个, [0x86201000, 0x86201064]预留101个用于映射hka_types.h中的返回码
// 状态取值1表示正常状态/成功状态
typedef enum _HKANN_ARM_STATUS_CODE_
{
	HKANN_ARM_STS_ERR_CPU_ABILITY_OVERFLOW     = 0x8620117e,        // cpu 能力比值设置不正确
	HKANN_ARM_STS_ERR_CPU_ID_OVERFLOW          = 0x8620117d,        // cpu id 设置超过 最大线程id 
    //对外接口头文件相关[0x86201170 - 0x862011ef) 128 个
    HKANN_ARM_STS_ERR_ROI                      = 0x8620117c,        // 配置的ROI不正确，越界或者奇偶不对
    HKANN_ARM_STS_ERR_RESOLUTION               = 0x8620117b,        // 配置的分辨率不支持（超过设置的最大分辨率或奇偶不对）
    HKANN_ARM_STS_ERR_GET_CPU_INFO             = 0x8620117a,        // CPU信息获取错误
    HKANN_ARM_STS_ERR_MODEL_SIZE               = 0x86201179,        // 模型大小错误
    HKANN_ARM_STS_ERR_MODEL_TYPE               = 0x86201178,        // 模型类型错误
    HKANN_ARM_STS_ERR_DL_PROC_TYPE             = 0x86201177,        // VCA DL处理类型错误
    HKANN_ARM_STS_ERR_PROC_TYPE                = 0x86201176,        // HKANN Process接口传入处理类型错误
    HKANN_ARM_STS_ERR_HANDLE_TYPE              = 0x86201175,        // 句柄类型错误
    HKANN_ARM_STS_ERR_HANDLE_NUM               = 0x86201174,        // 句柄数量错误
    HKANN_ARM_STS_ERR_THREAD_NUM               = 0x86201173,        // 线程数错误
    HKANN_ARM_STS_ERR_AFFINITY_POLICY          = 0x86201172,        // 绑核策略错误
    HKANN_ARM_STS_ERR_RECONFIG_FAILED          = 0x86201171,        // 用户变更分辨率或ROI时参数重新配置失败
    HKANN_ARM_STS_ERR_NO_SPACE_FOR_RECONFIG    = 0x86201170,        // 暂时没有空间用来重配参数

    // 模型解析相关[0x862010f0 - 0x8620116f) 128 个
    HKANN_ARM_STS_ERR_MODEL_NO_DECRYPT         = 0x862010ff,        // 模型未解密
    HKANN_ARM_STS_ERR_ACTIVE_TYPE              = 0x862010fe,        // 模型激活类型不支持
    HKANN_ARM_STS_ERR_GET_LAYER_IDX            = 0x862010fd,        // 模型解析时获取暂存层中idnex出错
    HKANN_ARM_STS_ERR_PARAM_ARRAY_NUM          = 0x862010fc,        // 模型参数队列个数错误
    HKANN_ARM_STS_ERR_PARAM_REPLACE_POLICY     = 0x862010fb,        // 模型参数替换策略无效
    HKANN_ARM_STS_ERR_OP_TYPE                  = 0x862010fa,        // 模型解析中发现不支持的层类型
    HKANN_ARM_STS_ERR_LAYER_HEAD               = 0x862010f9,        // 模型层头参数错误
    HKANN_ARM_STS_ERR_PARAMS_SPACE             = 0x862010f8,        // 模型参数空间大小不对
    HKANN_ARM_STS_ERR_PASER_LAYER_NUM          = 0x862010f7,        // 模型解析得到的层数与实际层数不符
    HKANN_ARM_STS_ERR_GET_IN_BLOB              = 0x862010f6,        // 模型拓扑信息中记录的输入BLOB没有找到
    HKANN_ARM_STS_ERR_GET_IN_LAYER             = 0x862010f5,        // 模型拓扑信息中记录的输入层没有找到
    HKANN_ARM_STS_ERR_IN_BLOB_NUMS             = 0x862010f4,        // 层输入的BLOB个数错误
    HKANN_ARM_STS_ERR_CRC_CHECK                = 0x862010f3,        // 模型CRC检验失败
    HKANN_ARM_STS_ERR_LAYER_NUM                = 0x862010f2,        // 模型中层数超出范围
    HKANN_ARM_STS_ERR_NO_PARAM_GROUP           = 0x862010f1,        // 模型中没有指定分辨率的参数组
    HKANN_ARM_STS_ERR_PARAM_GROUP_NUM          = 0x862010f0,        // 模型中不同分辨率参数组的个数超出范围

    // HKANN补充的通用错误码（主要是因为HKA头文件提供的不够用）[0x86201070 - 0x862010ef] 128个
    HKANN_ARM_STS_ERR_DETECT_OBJ_NUM           = 0x86201077,        // 检出框个数超出最大限制
    HKANN_ARM_STS_ERR_BLOB_DIM                 = 0x86201076,        // BLOB的维度信息错误
    HKANN_ARM_STS_ERR_BLOB_STRIDES             = 0x86201075,        // BLOB的stride信息错误
    HKANN_ARM_STS_ERR_BLOB_SHAPES              = 0x86201074,        // BLOB的shape信息错误

    HKANN_ARM_STS_ERR_STORE_FORMAT             = 0x86201073,        // 数据存储类型错误
    HKANN_ARM_STS_ERR_DATA_TYPE                = 0x86201072,        // 数据类型错误
    // 补充HKANN_ARM_MEM_TAB成员变量的错误类型
    HKANN_ARM_STS_ERR_MEM_SPACE                = 0x86201071,        // 内存空间类型错误
    HKANN_ARM_STS_ERR_MEM_ATTR                 = 0x86201070,        // 内存属性错误

    /* 对应 hka_types.h ***********************************************************************************************/
    //cpu指令集支持错误码
    HKANN_ARM_STS_ERR_CPUID                    = 0x8620101d,        //cpu不支持优化代码中的指令集

    //内部模块返回的基本错误类型
    HKANN_ARM_STS_ERR_STEP                     = 0x8620101c,        //数据step不正确（除HKANN_ARM_IMAGE结构体之外）
    HKANN_ARM_STS_ERR_DATA_SIZE                = 0x8620101b,        //数据大小不正确（一维数据len，二维数据的HKANN_ARM_SIZE）
    HKANN_ARM_STS_ERR_BAD_ARG                  = 0x8620101a,        //参数范围不正确

    //算法库加密相关错误码定义
    HKANN_ARM_STS_ERR_EXPIRE                   = 0x86201019,        //算法库使用期限错误
    HKANN_ARM_STS_ERR_ENCRYPT                  = 0x86201018,        //加密错误

    //以下为组件接口函数使用的错误类型
    HKANN_ARM_STS_ERR_CALL_BACK                = 0x86201017,        //回调函数出错
    HKANN_ARM_STS_ERR_OVER_MAX_MEM             = 0x86201016,        //超过HKA限定最大内存
    HKANN_ARM_STS_ERR_NULL_PTR                 = 0x86201015,        //函数参数指针为空（共用）

    //检查HKANN_ARM_KEY_PARAM、HKANN_ARM_KEY_PARAM_LIST成员变量的错误类型
    HKANN_ARM_STS_ERR_PARAM_NUM                = 0x86201014,        //param_num参数不正确
    HKANN_ARM_STS_ERR_PARAM_VALUE              = 0x86201013,        //value参数不正确或者超出范围
    HKANN_ARM_STS_ERR_PARAM_INDEX              = 0x86201012,        //index参数不正确

    //检查cfg_type, cfg_size, prc_type, in_size, out_size, func_type是否正确
    HKANN_ARM_STS_ERR_FUNC_SIZE                = 0x86201011,        //子处理时输入、输出参数大小不正确
    HKANN_ARM_STS_ERR_FUNC_TYPE                = 0x86201010,        //子处理类型不正确
    HKANN_ARM_STS_ERR_PRC_SIZE                 = 0x8620100f,        //处理时输入、输出参数大小不正确
    HKANN_ARM_STS_ERR_PRC_TYPE                 = 0x8620100e,        //处理类型不正确
    HKANN_ARM_STS_ERR_CFG_SIZE                 = 0x8620100d,        //设置、获取参数输入、输出结构体大小不正确
    HKANN_ARM_STS_ERR_CFG_TYPE                 = 0x8620100c,        //设置、获取参数类型不正确

    //检查HKANN_ARM_IMAGE成员变量的错误类型
    HKANN_ARM_STS_ERR_IMG_DATA_NULL            = 0x8620100b,        //图像数据存储地址为空（某个分量）
    HKANN_ARM_STS_ERR_IMG_STEP                 = 0x8620100a,        //图像宽高与step参数不匹配
    HKANN_ARM_STS_ERR_IMG_SIZE                 = 0x86201009,        //图像宽高不正确或者超出范围
    HKANN_ARM_STS_ERR_IMG_FORMAT               = 0x86201008,        //图像格式不正确或者不支持

    //检查HKANN_ARM_MEM_TAB成员变量的错误类型
    HKANN_ARM_STS_ERR_MEM_ADDR_ALIGN           = 0x86201007,        //内存地址不满足对齐要求
    HKANN_ARM_STS_ERR_MEM_SIZE_ALIGN           = 0x86201006,        //内存空间大小不满足对齐要求
    HKANN_ARM_STS_ERR_MEM_LACK                 = 0x86201005,        //内存空间大小不够
    HKANN_ARM_STS_ERR_MEM_ALIGN                = 0x86201004,        //内存对齐不满足要求
    HKANN_ARM_STS_ERR_MEM_NULL                 = 0x86201003,        //内存地址为空

    //检查ability成员变量的错误类型
    HKANN_ARM_STS_ERR_ABILITY_ARG              = 0x86201002, 


    //通用类型
    HKANN_ARM_STS_OK                           = 1,                 //处理正确
    HKANN_ARM_STS_WARNING                      = 0x86201000,        //警告
    HKANN_ARM_STS_ERR                          = 0x86201001,        //不确定类型错误

    HKANN_ARM_STS_END                          = VCA_ENUM_END,
} HKANN_ARM_STATUS_CODE; 

// ARM平台数据值定义在200~300
// 配置参数KEY-PARAM的索引号
typedef enum _HKANN_ARM_PARAM_IDX_
{
    HKANN_ARM_IDX_X86_TOOL_VERSION             = 200,               // x86模型转换工具版本
    HKANN_ARM_IDX_FRCOUT_CONF_THRESHOLD        = 201,               // FRCOUT置信度阈值
    HKANN_ARM_IDX_INVALID                      = VCA_ENUM_END,
} HKANN_ARM_PARAM_IDX;

// 网络配置选项CMD
typedef enum _HKANN_AMR_CMD_
{
    HKANN_AMR_CFG_SINGLE_PARAM                 = 250,               // 配置单个参数
} HKANN_AMR_CMD;


/***********************************************************************************************************************
* 结构体
***********************************************************************************************************************/
// 算法库接口设置、获取单个参数结构体
typedef struct _HAKNN_ARM_KEY_PARAM_
{
    int                             index;                          // 参数索引
    size_t                          value;                          // 参数值
} HAKNN_ARM_KEY_PARAM;

// 网络工作提示信息
typedef struct _HKANN_ARM_HINT_
{
    HKANN_ARM_AFFINITY_POLICY       affinity_policy;                // ACNN工作线程绑核策略
    unsigned char                  *cpu_id_list;                    // 用户指定的cpu id 列表  [ 0 1 2 3 4 ]
    float                          *cpu_ability_ratio_list;         // 系统 cpu 能力比值      [ 1 1 1 1 2]
    unsigned int                    reserved[13];                   // 预留 15-2
} HKANN_ARM_HINT;

// ARM私有参数
typedef struct _HKANN_ARM_PRIVATE_
{
    unsigned int                    param_array_num;                // 网络执行所需参数缓存的队列深度
    unsigned int                    thread_num_max;                 // 智能算法库可使用的最大线程数
    HKANN_ARM_HINT                  hint;                           // 网络工作提示信息
    unsigned int                    reserved[14];
} HKANN_ARM_PRIVATE;


#ifdef __cplusplus
}
#endif

#endif // _HKANN_ARM_H_