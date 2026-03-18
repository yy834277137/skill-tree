/***********************************************************************************************************************
* 版权信息：版权所有 (c), 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：hkann_platform.h
* 摘    要：RN平台跨平台推理库私有对外接口
* 
***********************************************************************************************************************/
#ifndef _HKANN_PLATFORM_H_
#define _HKANN_PLATFORM_H_

#ifdef __cplusplus 
extern "C" { 
#endif

#include "hkann_lib.h"
#include "vca_base.h"
/***********************************************************************************************************************
* 宏定义
***********************************************************************************************************************/
// RN平台版本信息
#define HKANN_PLATFORM_NAME              "RN"
#define HKANN_PLATFORM_TYPE              (VCA_DL_PROC_RN)

#define HKANN_MAJOR_VERSION              (1)                // 主版本号
#define HKANN_SUB_VERSION                (4)                // 副版本号
#define HKANN_REVISION_VERSION           (201)                // 修订版本号

/***********************************************************************************************************************
* 错误码，范围 NN:[0X86401000, 0x86401600) 1536 个
*            CMP:[0X86401600, 0X86401A00) 1024 个 为RN平台CMP使用，此处不使用
*             IA:[0x86401A00, 0x86401FFF) 1536 个 为RN平台IA使用，此处不使用
***********************************************************************************************************************/
typedef enum _HKANN_RN_ERROR_CODE_
{   
    // 256
    HKANN_RN_ERR_BASE                         = 0x86401000,

    // RN芯片&驱动错误码(0x86401100, 0x864011FF) 256
    HKANN_RN_ERR_CHIP_BASE                    = 0x86401100,
    
    // NPU相关错误码(0x86401200, 0x864012FF) 256
    HKANN_RN_ERR_NPU_BASE                     = 0x86401200,
    HKANN_RN_ERR_NPU_INIT_ERROR               = 0x86401201,
    HKANN_RN_ERR_NPU_QUERY_ERROR              = 0x86401202,
    HKANN_RN_ERR_NPU_TENSOR_INIT_ERROR        = 0x86401203,
    HKANN_RN_ERR_NPU_TENSOR_RELEASE_ERROR     = 0x86401204,
    HKANN_RN_ERR_NPU_RUN_ERROR                = 0x86401205,
    HKANN_RN_ERR_NPU_DESTROY_ERROR            = 0x86401206,
    
    // GPU相关错误码(0x86401300, 0x864013FF) 256
    HKANN_RN_ERR_GPU_BASE                     = 0x86401300,

    // 
    HKANN_RN_ERR_CODE_END                     = HKANN_ENUM_END

} HKANN_RN_ERROR_CODE;

//网络优先级枚举
typedef enum _HKANN_RN_NET_PRIORITY_
{
    HKANN_RN_NET_PRIORITY_HIGH                = 0,                    // 网络优先级-高
    HKANN_RN_NET_PRIORITY_MID,                                        // 网络优先级-中
    HKANN_RN_NET_PRIORITY_LOW,                                        // 网络优先级-低
    
    HKANN_RN_NET_INVALID                      = HKANN_ENUM_END
} HKANN_RN_NET_PRIORITY;

// 参数配置/获取命令
typedef enum _HKANN_RN_PARAM_CMD_
{
    // 组件类参数
    HKANN_RN_PARAM_LIB_VERSION                = 0x00000001,       // 获取算法库版本号

    // 网络级参数（暂不支持）
    HKANN_RN_PARAM_NET_PRIORITY               = 0x00000101,       // 配置、获取网络优先级(HKANN_RN_NET_PRIORITY)

    HKANN_RN_PARAM_KEY_INVALID                = HKANN_ENUM_END
} HKANN_RN_PARAM_CMD;

#ifdef __cplusplus 
} 
#endif

#endif  // _HKANN_PLATFORM_H_
