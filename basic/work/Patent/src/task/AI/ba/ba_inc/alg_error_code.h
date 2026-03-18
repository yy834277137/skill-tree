/***************************************************************************************************
* 版权信息：版权所有(c), 杭州海康威视数字技术股份有限公司, 保留所有权利
*
* 文件名称：alg_error_code.h
* 摘    要：算子层错误码
*
* 当前版本：0.8.0
* 作    者：陈韬29
* 日    期：2020-09-21
* 备    注：定义算子层错误码
* 说    明：
***************************************************************************************************/

#ifndef _ALG_ERROR_CODE_H_
#define _ALG_ERROR_CODE_H_


#define SBAE_ALG_SUCCESS                        (0)                        // 算子处理成功

// 开放平台算子错误码
#define SBAE_ALG_BASE                           (0x8a180000)               // 开放平台错误码基准
#define SBAE_ALG_NULL_PTR                       (SBAE_ALG_BASE + 30)       // 空指针
#define SBAE_ALG_FUN_TYPE_ERROR                 (SBAE_ALG_BASE + 31)       // 功能类型错误
#define SBAE_ALG_PARAM_ERROR                    (SBAE_ALG_BASE + 32)       // 输入参数错误
#define SBAE_ALG_MEM_TRAN_TYPE_ERR              (SBAE_ALG_BASE + 33)       // 内存类型转换错误
#define SBAE_ALG_CONFIG_JSON_PARAM_ERROR        (SBAE_ALG_BASE + 34)       // 配置json参数错误
#define SBAE_ALG_MODEL_SIZE_ERR                 (SBAE_ALG_BASE + 35)       // 模型大小错误
#define SBAE_ALG_MODEL_READ_UNMATCH             (SBAE_ALG_BASE + 36)       // 模型读取错误
#define SBAE_ALG_ERR_INPUT_DATA_SIZE            (SBAE_ALG_BASE + 37)       // 输入数据尺寸不合
#define SBAE_ALG_ERR_INPUT_DATA_FORMAT          (SBAE_ALG_BASE + 38)       // 输入数据格式不为NV12
#define SBAE_ALG_SET_CONFIG_UNDO                (SBAE_ALG_BASE + 39)       // 未配置参数
#define SBAE_ALG_JSON_FILE_ERR                  (SBAE_ALG_BASE + 40)       // Json文件错误
#endif

