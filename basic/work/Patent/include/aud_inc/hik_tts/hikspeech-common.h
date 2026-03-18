/***********************************************************************************************************************
* 
* 版权信息：版权所有(c) 2017-2020, 杭州海康威视研究院, 保留所有权利
*
* 文件名称：hikspeech-common.h
* 文件标识：_HIKSPEECH_COMMON_H_
* 摘    要：海康威视研究院语音识别公用头文件

* 当前版本：1.0.1
* 作    者：齐昕
* 日    期：2019-06-13
* 备    注：1. 修改错误码
            2. 修改HIKSPEECH_MEM_TAB定义

* 当前版本：1.0.0
* 作    者：金超
* 日    期：2019-05-11
* 备    注：初始版本
***********************************************************************************************************************/
#ifndef _HIKSPEECH_COMMON_H_
#define _HIKSPEECH_COMMON_H_

#include <stddef.h>
#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef unsigned int HRESULT;
#endif /* _HRESULT_DEFINED */

/***********************************************************************************************************************
* 错误码
* 0x81F1为识别组字段
* 语音识别组通用错误码字段为0x81F100**
* 服务器端ASR错误码字段为   0x81F101**
* VAD错误码字段为           0x81F102**
* 本地关键词错误码字段为    0x81F103**
* 事件检测错误码字段为      0x81F104**
* 声纹检测错误码字段为      0x81F105**
***********************************************************************************************************************/
#define HIKSPEECH_LIB_S_OK                      1           // 成功
#define HIKSPEECH_LIB_S_FAIL                    0           // 失败

// 内存相关错误码
#define HIKSPEECH_LIB_E_MEM_OUT                 0x81F10001  // 内存不够
#define HIKSPEECH_LIB_E_PTR_NULL                0x81F10002  // 空指针

// 模型相关错误码
#define HIKSPEECH_LIB_E_MODEL_FILE_PATH_ERR     0x81F10010  // 模型路径错误
#define HIKSPEECH_LIB_E_MODEL_TYPE_ERR          0x81F10011  // 模型类型错误
#define HIKSPEECH_LIB_E_MODEL_VERSION_ERR       0x81F10012  // 模型版本号错误
#define HIKSPEECH_LIB_E_MODEL_FORMAT_ERR        0x81F10013  // 模型文件格式错误
#define HIKSPEECH_LIB_E_MODEL_NUM_ERR           0x81F10014  // 模型数量错误
#define HIKSPEECH_LIB_E_MODEL_SIZE_ERR          0x81F10015  // 模型大小错误
#define HIKSPEECH_LIB_E_MODEL_MATCH_ERR         0x81F10016  // 模型不匹配
#define HIKSPEECH_LIB_E_MODEL_HEADER_ERR        0x81F10017  // 模型头错误
#define HIKSPEECH_LIB_E_MODEL_PARAM_ERR         0x81F10018  // 模型参数错误

// 矩阵相关错误码
#define HIKSPEECH_LIB_E_MATRIX_ERR              0x81F10020 // 矩阵操作错误
#define HIKSPEECH_LIB_E_MATRIX_READ_ERR         0x81F10021 // 矩阵读错误     

/***********************************************************************************************************************
* 相关参数宏定义
***********************************************************************************************************************/
#define HIKSPEECH_MAX_STR_LEN                      1024                 // 字符串最大长度 
#define HIKSPEECH_ENUM_END                         0x0FFFFF             // 定义枚举结束值，用于对齐
#define HIKSPEECH_MAX_PARAM_NUM                    50                   // 最多关键参数个数
#define HIKSPEECH_MAX_MODEL_NUM                    16                   // 最大模型个数

/***********************************************************************************************************************
* 语音信息相关宏定义
***********************************************************************************************************************/
#define HIKSPEECH_AUDIO_SAMPLE_RATE_48K             48000
#define HIKSPEECH_AUDIO_SAMPLE_RATE_44K             44100
#define HIKSPEECH_AUDIO_SAMPLE_RATE_32K             32000
#define HIKSPEECH_AUDIO_SAMPLE_RATE_16K             16000
#define HIKSPEECH_AUDIO_SAMPLE_RATE_8K              8000
#define HIKSPEECH_AUDIO_BIT_PER_SAMPLE              16
#define HIKSPEECH_AUDIO_MONO_CHANNEL                1

/*音频压缩编码类型*/
typedef enum _HIKSPEECH_ENCODE_TYPE_
{
    HIKSPEECH_DATA_RAW,
    HIKSPEECH_DATA_OPUS,
    HIKSPEECH_DATA_AMR_NB,
    HIKSPEECH_DATA_ENCODE_END = HIKSPEECH_ENUM_END
}HIKSPEECH_ENCODE_TYPE;

/***********************************************************************************************************************
* 内存管理器SPEECH_MEM_TAB结构体定义
***********************************************************************************************************************/

/* 内存对齐属性 */
typedef enum _HIKSPEECH_MEM_ALIGNMENT_
{
    HIKSPEECH_MEM_ALIGN_4BYTE    = 4,
    HIKSPEECH_MEM_ALIGN_8BYTE    = 8,
    HIKSPEECH_MEM_ALIGN_16BYTE   = 16,
    HIKSPEECH_MEM_ALIGN_32BYTE   = 32,
    HIKSPEECH_MEM_ALIGN_64BYTE   = 64,
    HIKSPEECH_MEM_ALIGN_128BYTE  = 128,
    HIKSPEECH_MEM_ALIGN_256BYTE  = 256,
    HIKSPEECH_MEM_ALIGN_END      = HIKSPEECH_ENUM_END
}HIKSPEECH_MEM_ALIGNMENT;

/* 内存属性 */
typedef enum _HIKSPEECH_MEM_ATTRS_
{
    HIKSPEECH_MEM_SCRATCH,                               // 可复用内存，能在多路切换时有条件复用
    HIKSPEECH_MEM_PERSIST,                               // 不可复用内存
    HIKSPEECH_MEM_ATTRS_END = HIKSPEECH_ENUM_END
}HIKSPEECH_MEM_ATTRS;

typedef enum _HIKSPEECH_MEM_PLAT_
{
    HIKSPEECH_MEM_PLAT_CPU,                              // CPU内存 
    HIKSPEECH_MEM_PLAT_GPU,                              // GPU内存
    HIKSPEECH_MEM_PLAT_END = HIKSPEECH_ENUM_END
}HIKSPEECH_MEM_PLAT;

/* 内存分配空间 */
typedef enum _HIKSPEECH_MEM_SPACE_
{
    HIKSPEECH_MEM_EXTERNAL_PROG,                          // 外部程序存储区
    HIKSPEECH_MEM_INTERNAL_PROG,                          // 内部程序存储区
    HIKSPEECH_MEM_EXTERNAL_TILERED_DATA,                  // 外部Tilered数据存储区
    HIKSPEECH_MEM_EXTERNAL_CACHED_DATA,                   // 外部可Cache存储区
    HIKSPEECH_MEM_EXTERNAL_UNCACHED_DATA,                 // 外部不可Cache存储区
    HIKSPEECH_MEM_INTERNAL_DATA,                          // 内部存储区
    HIKSPEECH_MEM_EXTERNAL_TILERED8,                      // 外部Tilered数据存储区8bit，Netra/Centaurus特有 
    HIKSPEECH_MEM_EXTERNAL_TILERED16,                     // 外部Tilered数据存储区16bit，Netra/Centaurus特有
    HIKSPEECH_MEM_EXTERNAL_TILERED32,                     // 外部Tilered数据存储区32bit，Netra/Centaurus特有
    HIKSPEECH_MEM_EXTERNAL_TILEREDPAGE,                   // 外部Tilered数据存储区page形式，Netra/Centaurus特有
    HIKSPEECH_MEM_EXTERNAL_END = HIKSPEECH_ENUM_END
}HIKSPEECH_MEM_SPACE;

typedef struct _HIKSPEECH_MEM_TAB_
{
    size_t                     size;                     // 以BYTE为单位的内存大小
    HIKSPEECH_MEM_ALIGNMENT   alignment;                 // 内存对齐属性, 建议为128
    HIKSPEECH_MEM_SPACE       space;                     // 内存分配空间 
    HIKSPEECH_MEM_ATTRS       attrs;                     // 内存属性 
    void                       *base;                    // 分配出的内存指针 
    HIKSPEECH_MEM_PLAT        plat;                      // 平台
} HIKSPEECH_MEM_TAB;

//存储空间管理结构体
typedef struct _HIKSPEECH_MEM_BUF_
{
    void     *start;                                     // 缓存起始位置
    void     *end;                                       // 缓存结束位置
    void     *cur_pos;                                   // 缓存空余起始位置
}HIKSPEECH_MEM_BUF;

/***********************************************************************************************************************
* 输入音频帧结构体
***********************************************************************************************************************/
// 音频帧头信息
typedef struct _HIKSPEECH_AUDIO_INFO_
{    
    unsigned int   channel_num;         // 声道数
    unsigned int   sample_rate;         // 采样率
    unsigned int   bits_per_sample;     // 每个采样点的bit数，公司现有的音频数据都是16bit
    unsigned int   data_len;            // 每通道数据采样点数，多通道则每个通道数据长度必须一样
    int            reserved[4];         // 保留字节
}HIKSPEECH_AUDIO_INFO;

/***********************************************************************************************************************
* 配置信息中高级参数相关信息结构体定义
***********************************************************************************************************************/
// 算法库配置数据类型枚举
typedef enum _HIKSPEECH_SET_CFG_TYPE_
{
    HIKSPEECH_SET_CFG_SINGLE_PARAM      = 0x0001,                // 单个参数
    HIKSPEECH_SET_CFG_PARAM_LIST        = 0x0002,                // 参数表
    HIKSPEECH_SET_CFG_DEFAULT_PARAM     = 0x0003,                // 默认参数
    HIKSPEECH_SET_CFG_RESTART_LIB       = 0x0004,                // 重启算法库
    HIKSPEECH_SET_CFG_END               = HIKSPEECH_ENUM_END
}HIKSPEECH_SET_CFG_TYPE;

// 配置参数类型
typedef enum _HIKSPEECH_GET_CFG_TYPE_
{
    HIKSPEECH_GET_CFG_SINGLE_PARAM      = 0x0001,                 // 单个参数
    HIKSPEECH_GET_CFG_PARAM_LIST        = 0x0002,                 // 参数表
    HIKSPEECH_GET_CFG_DEFAULT_PARAM     = 0x0003,                 // 默认参数
    HIKSPEECH_GET_CFG_VERSION           = 0x0004,                 // 版本信息
    HIKSPEECH_GET_CFG_END               = HIKSPEECH_ENUM_END
}HIKSPEECH_GET_CFG_TYPE;

// 算法库参数结构体
typedef struct _HIKSPEECH_KEY_PARAM_
{
    int  index;
    int  value;
}HIKSPEECH_KEY_PARAM;

// 参数链表
typedef struct _HIKSPEECH_KEY_PARAM_LIST_
{
    unsigned int              param_num;
    HIKSPEECH_KEY_PARAM      param[HIKSPEECH_MAX_PARAM_NUM];
}HIKSPEECH_KEY_PARAM_LIST;


#endif /* _HIKSPEECH_BASE_H_ */

