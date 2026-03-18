/***********************************************************************************************************************
* 版权信息：版权所有 (c), 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：hkann_amai_error.h
* 摘    要：跨平台推理库公共错误码文件
* 
* 当前版本：0.8.0
* 作    者：刘伟良6
* 日    期：2020-06-16
* 备    注：初始版本
***********************************************************************************************************************/
#ifndef _HKANN_AMAI_ERROR_H_
#define _HKANN_AMAI_ERROR_H_

#include "hkann_lib.h"

#ifdef __cplusplus 
extern "C" { 
#endif

/***********************************************************************************************************************
* 跨平台推理库公共错误码范围[0x8641 2000 ~ 0x8641 3FFF]，共4096 * 2个
***********************************************************************************************************************/
typedef enum _HKANN_AMAI_ERROR_CODE_
{
    HKANN_AMAI_OK                       = 1,                        // 已有惯例，组件库需定义XXXX_OK，值为1

    /*******************************************************************************************************************
    * 接口校验错误码，范围[0x86412000 0x864120FF]，共256个
    ********************************************************************************************************************/
    // 空指针和通用错误(0x86412000 ~ 0x8641200F)
    HKANN_AMAI_ERR_BASE                 = 0x86412000,
    HKANN_AMAI_ERR_COMMON               = 0x86412001,               // 一般错误码
    HKANN_AMAI_ERR_NULL_PTR             = 0x86412002,               // 空指针

    // 检查内存是否正确(0x86412010 ~ 0x8641201F)
    HKANN_AMAI_ERR_MEM_NULL             = 0x86412010,               // 空地址
    HKANN_AMAI_ERR_MEM_ALIGN            = 0x86412011,               // 内存对齐不满足要求
    HKANN_AMAI_ERR_MEM_LACK             = 0x86412012,               // 内存空间不足   
    HKANN_AMAI_ERR_MEM_ATTR             = 0x86412013,               // 内存attitude错误
    HKANN_AMAI_ERR_MEM_SPACE            = 0x86412014,               // 内存space错误
    HKANN_AMAI_ERR_MEM_PLAT             = 0x86412015,               // 内存plat错误

    // 检查输入图像是否正确(0x86412020 ~ 0x8641202F)
    HKANN_AMAI_ERR_IMG_FORMAT           = 0X86412020,               // 图像格式不正确或者不支持
    HKANN_AMAI_ERR_IMG_SIZE             = 0X86412021,               // 图像宽高不正确或者超出范围
    HKANN_AMAI_ERR_IMG_DATA_NULL        = 0X86412022,               // 图像数据存储地址为空（某个分量）

    // 检查参数是否正常(0x86412030 ~ 0x8641203F)
    HKANN_AMAI_ERR_PARAM_SIZE           = 0x86412030,               // 参数尺寸不正确
    HKANN_AMAI_ERR_PARAM_TYPE           = 0x86412031,               // 参数类型不正确
    HKANN_AMAI_ERR_CFG_SIZE             = 0x86412032,               // 设置、获取参数输入、输出结构体大小不正确
    HKANN_AMAI_ERR_CFG_TYPE             = 0x86412033,               // 设置、获取参数类型不正确
    HKANN_AMAI_ERR_PARAM_VALUE          = 0x86412034,               // 参数大小不正确

    // 检测handle是否正常(0x86412040 ~ 0x8641204F)
    HKANN_AMAI_ERR_FAKE_HANDLE          = 0x86412040,               // Handle个数不正确以及空指针
    HKANN_AMAI_ERR_HANDLE_TYPE          = 0x86412041,               // Handle类型不正确

    // BLOB参数错误(0x86412050 ~ 0x8641205F)
    HKANN_AMAI_ERR_IN_BLOB_DIM          = 0x86412050,               // in blob dim不正确 
    HKANN_AMAI_ERR_IN_BLOB_NUM          = 0x86412051,               // in blob 个数不正确 
    HKANN_AMAI_ERR_IN_BLOB_FMT          = 0x86412052,               // in blob fmt不正确 
    HKANN_AMAI_ERR_OUT_BLOB_DIM         = 0x86412053,               // out blob dim不正确 
    HKANN_AMAI_ERR_OUT_BLOB_NUM         = 0x86412054,               // out blob 个数不正确 
    HKANN_AMAI_ERR_BLOB_LAYOUT          = 0x86412055,               // blob layout不正确
    HKANN_AMAI_ERR_BLOB_SIZE            = 0x86412056,               // blob 字节尺寸不正确
    HKANN_AMAI_ERR_BLOB_DTYPE           = 0x86412057,               // blob 数据类型不正确
    HKANN_AMAI_ERR_BLOB_SHAPE           = 0x86412058,               // blob 数据形状不正确

    /*******************************************************************************************************************
    * 模型相关错误码，范围[0x86412100 0X864121FF]，共256个
    *******************************************************************************************************************/
    // (0x86412100 ~ 0X8641211F，共32个)
    HKANN_AMAI_ERR_MODEL_BASE           = 0x86412100,      
    HKANN_AMAI_ERR_MODEL_NET_HEAD       = 0x86412101,               // 网络头模型错误
    HKANN_AMAI_ERR_MODEL_SEG_HEAD       = 0x86412102,               // 网络子段头模型错误
    HKANN_AMAI_ERR_MODEL_OP_NOT_SUPPORT = 0x86412103,               // 不支持的OP类型
    HKANN_AMAI_ERR_MODEL_KERNEL         = 0x86412104,               // kernel参数错误
    HKANN_AMAI_ERR_MODEL_TOPO           = 0x86412105,               // 模型拓扑错误
    HKANN_AMAI_ERR_MODEL_BLOB           = 0x86412106,               // BLOB错误
    HKANN_AMAI_ERR_MODEL_BLOB_SHAPE     = 0x86412107,               // SHAPE错误
    HKANN_AMAI_ERR_TOPO_ID_TYPE         = 0x86412108,               // 拓扑ID类型错误

    // 模型加解密、解析相关错误码(0x86412120 ~ 0x8641212F，共16个)
    HKANN_AMAI_ERR_MODEL_ENCRYPT        = 0x86412120,               // 模型加密错误
    HKANN_AMAI_ERR_MODEL_DECRYPT        = 0x86412121,               // 模型解密错误
    HKANN_AMAI_ERR_MODEL_PARSE          = 0x86412122,               // 模型解析错误

    // OP相关错误码(0x86412130 ~ 0x8641215F，共64个)
    HKANN_AMAI_ERR_MODEL_CONV           = 0x86412130,               // CONV模型参数错误
    HKANN_AMAI_ERR_MODEL_FC             = 0x86412131,               // FC模型参数错误
    HKANN_AMAI_ERR_MODEL_RESHAPE        = 0x86412132,               // FC模型参数错误
    HKANN_AMAI_ERR_MODEL_FEATRESHAPE    = 0x86412133,               // FC模型参数错误
    HKANN_AMAI_ERR_MODEL_YOLOOUT        = 0x86412134,               // YOLO模型参数错误
    HKANN_AMAI_ERR_MODEL_YOLOPROPOSAL   = 0x86412135,               // YOLOPROPOSAL模型参数错误
    HKANN_AMAI_ERR_MODEL_NMS            = 0x86412136,               // NMS模型参数错误
    HKANN_AMAI_ERR_MODEL_SOFTMAX        = 0x86412137,               // SOFTMAX模型参数错误
    HKANN_AMAI_ERR_MODEL_BLOBTRANS      = 0x86412138,               // BLOBTRANS模型参数错误
    HKANN_AMAI_ERR_MODEL_YLOUTFPT       = 0x86412139,               // yolooutfpt模型参数错误
    HKANN_AMAI_ERR_MODEL_KP_TOPDOWN     = 0x8641213a,               // kp_topdown_postproc模型参数错误

    /*******************************************************************************************************************
    * 组件操作错误码，范围[0x86412200 0x864129FF]，共2048个
    *******************************************************************************************************************/
    // 组件操作错误码
    HKANN_AMAI_ERR_LIB_BASE             = 0x86412200,

    // 版本操作错误(0x86412200 ~ 0x8641220F，共16个)
    HKANN_AMAI_ERR_LIB_VERSION          = 0x86412201,               // 组件版本错误
	HKANN_AMAI_ERR_LIB_VERSION_LIMITED  = 0x86412201,               // 组件非加密运行次数达上限错误

    // Cache操作错误(0x86412210 ~ 0x8641221F，共16个)
    HKANN_AMAI_ERR_DRIVER_SYNC_CACHE    = 0x86412210,               // 刷cache错误  

    // 加密操作错误(0x86412220 ~ 0x8641222F，共16个)
    HKANN_AMAI_ERR_CRYPTO_CHECK         = 0x86412220,               // 硬件加密检查错误


    // 调度器操作错误(0x86412230 ~ 0x8641223F，共16个)

    // Set/Get Config操作错误(0x86412240 ~ 0x8641225F，共32个)


    // 系统类通用错误，如IPC(0x86412260 ~ 0x8641227F，共32个)
    HKANN_AMAI_ERR_DRIVER_RELEASE       = 0x86412260,               // driver释放错误  
    HKANN_AMAI_ERR_DRIVER_INIT          = 0x86412261,               // driver初始化错误  
    HKANN_AMAI_ERR_PROC_TYPE            = 0x86412262,               // 处理器类型错误  


    // 对象操作错误(0x86412280 ~ 0x8641229F，共32个)
    HKANN_AMAI_ERR_FACTORY                  = 0x86412280,           // 驱动错误
    HKANN_AMAI_ERR_FACTORY_GET_SYS_FAILED   = 0x86412281,           // 获取系统驱动失败
    HKANN_AMAI_ERR_FACTORY_GET_NPU_FAILED   = 0x86412282,           // 获取NPU驱动失败
    HKANN_AMAI_ERR_FACTORY_GET_SEG_FAILED   = 0x86412283,           // 获取SEG实例失败
    HKANN_AMAI_ERR_FACTORY_GET_OP_FAILED    = 0x86412284,           // 获取SEG实例失败

    // 函数操作错误(0x864122A0 ~ 0x864122BF，共32个)
    HKANN_AMAI_ERR_FUNC_NULL                = 0x864122A0,           // 获取函数失败
    HKANN_AMAI_ERR_FUNC_NULL_DRIVER_SYS     = 0x864122A1,           // 系统驱动函数获取失败
    HKANN_AMAI_ERR_FUNC_NULL_DRIVER_NPU     = 0x864122A2,           // NPU驱动函数获取失败
    HKANN_AMAI_ERR_FUNC_NULL_SEG            = 0x864122A3,           // SEG类函数获取失败
    HKANN_AMAI_ERR_FUNC_NULL_OP             = 0x864122A4,           // OP类函数获取失败

    // 子模块错误(0x864122C0 ~ 0x864122DF，共32个)
    HKANN_AMAI_ERR_DUMP_DATA                = 0x864122C0,           // 导出数据失败

    
    // OP操作错误(0x86412300 ~ 0x864126FF，共16*64个)
    // CONV OP操作错误(0x86412300 ~ 0x8641230F，共16个)
    // HKANN_AMAI_ERR_OP_CONV_INIT             = 0x86412300,        // CONV初始化错误 
    // HKANN_AMAI_ERR_OP_CONV_PROC             = 0x86412301,        // CONV处理错误 

    // 其他OP操作错误

    HKANN_HI_ERR_CODE_END                   = HKANN_ENUM_END
} HKANN_AMAI_ERROR_CODE;

#ifdef __cplusplus 
} 
#endif

#endif  // _HKANN_AMAI_ERROR_H_
