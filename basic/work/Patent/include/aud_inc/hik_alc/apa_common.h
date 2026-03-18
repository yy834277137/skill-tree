/***********************************************************************************************************************
* 
* 版权信息：版权所有(c) 2017-2020, 杭州海康威视研究院, 保留所有权利
*
* 文件名称：apa_common.h
* 文件标识：_HIK_APA_COMMON_H_
* 摘    要：海康威视研究院音频处理分析公用头文件

* 当前版本：0.9.0
* 作    者：陈扬坤
* 日    期：2017年03月21日
* 备    注：制定音频处理分析公用头文件            
***********************************************************************************************************************/
#ifndef _HIK_APA_COMMON_H_
#define _HIK_APA_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef unsigned int HRESULT;
#endif /* _HRESULT_DEFINED */

/***********************************************************************************************************************
* 错误码
***********************************************************************************************************************/
#define HIK_APA_LIB_S_OK                     1           // 成功	  
#define HIK_APA_LIB_S_FAIL                   0           // 失败
#define HIK_APA_LIB_E_MEM_OUT                0x81F00001  // 内存不够
#define HIK_APA_LIB_E_PTR_NULL	             0x81F00002  // 传入指针为空  
#define HIK_APA_LIB_KEY_PARAM_ERR            0x81F00003  // 高级参数设置错误
#define HIK_APA_LIB_PARAM_VAL_ERR            0x81F00004  // 参数设置值错误
#define HIK_APA_LIB_E_BITS_PER_SAMP_ERR      0x81F00005  // 每个采样点bit精数错误
#define HIK_APA_LIB_E_CHANNEL_ERR            0x81F00006  // 声道数错误
#define	HIK_APA_LIB_E_SAMPLERATE_ERR         0x81F00007  // 采样率错误
#define HIK_APA_LIB_E_INPUT_SIZE_ERR         0x81F00008  // 输入数据大小错误
#define HIK_APA_LIB_E_MEMTAB_SIZE_ERR        0x81F00009  // 内存表大小错误
#define HIK_APA_LIB_E_AUDIO_UNSUPPORT        0x81F0000a  // 音频输入信息错误
#define HIK_APA_LIB_E_ERRCODE_UNSUPPORT      0x81F0000b  // 音频错误码不支持
#define HIK_APA_LIB_E_UNKNOW                 0x81FFFFFF  // 未知错误

/***********************************************************************************************************************
* 内存管理器APA_MEM_TAB结构体定义
***********************************************************************************************************************/
#define APA_ENUM_END          0x0FFFFF                   // 定义枚举结束值，用于对齐
#define APA_MAX_PARAM_NUM     50                         // 最多关键参数个数
#define APA_MAX_CHN_N         64                         // 最多音频通道数

/*内存对齐属性*/
typedef enum _APA_MEM_ALIGNMENT_
{
    APA_MEM_ALIGN_4BYTE    = 4,
    APA_MEM_ALIGN_8BYTE    = 8,
    APA_MEM_ALIGN_16BYTE   = 16,
    APA_MEM_ALIGN_32BYTE   = 32,
    APA_MEM_ALIGN_64BYTE   = 64,
    APA_MEM_ALIGN_128BYTE  = 128,
    APA_MEM_ALIGN_256BYTE  = 256,
    APA_MEM_ALIGN_END      = APA_ENUM_END
}APA_MEM_ALIGNMENT;

/* 内存属性 */
typedef enum _APA_MEM_ATTRS_
{
    APA_MEM_SCRATCH,                    // 可复用内存，能在多路切换时有条件复用
    APA_MEM_PERSIST,                    // 不可复用内存
    APA_MEM_ATTRS_END = APA_ENUM_END
}APA_MEM_ATTRS;

typedef enum _APA_MEM_PLAT_
{
	APA_MEM_PLAT_CPU,                   // CPU内存 
	APA_MEM_PLAT_GPU,                   // GPU内存
	APA_MEM_PLAT_END = APA_ENUM_END
}APA_MEM_PLAT;

/* 内存分配空间 */
typedef enum _APA_MEM_SPACE_ 
{
    APA_MEM_EXTERNAL_PROG,              // 外部程序存储区           
    APA_MEM_INTERNAL_PROG,              // 内部程序存储区         
    APA_MEM_EXTERNAL_TILERED_DATA,      // 外部Tilered数据存储区  
    APA_MEM_EXTERNAL_CACHED_DATA,       // 外部可Cache存储区      
    APA_MEM_EXTERNAL_UNCACHED_DATA,     // 外部不可Cache存储区    
    APA_MEM_INTERNAL_DATA,              // 内部存储区
    APA_MEM_EXTERNAL_TILERED8 ,         // 外部Tilered数据存储区8bit，Netra/Centaurus特有 
    APA_MEM_EXTERNAL_TILERED16,         // 外部Tilered数据存储区16bit，Netra/Centaurus特有
    APA_MEM_EXTERNAL_TILERED32 ,        // 外部Tilered数据存储区32bit，Netra/Centaurus特有
    APA_MEM_EXTERNAL_TILEREDPAGE,       // 外部Tilered数据存储区page形式，Netra/Centaurus特有
    APA_MEM_EXTERNAL_END = APA_ENUM_END
}APA_MEM_SPACE;


typedef struct _APA_MEM_TAB_
{
	unsigned int        size;           // 以BYTE为单位的内存大小
	APA_MEM_ALIGNMENT   alignment;      // 内存对齐属性, 建议为128
	APA_MEM_SPACE       space;          // 内存分配空间 
	APA_MEM_ATTRS       attrs;          // 内存属性 
	void                *base;          // 分配出的内存指针 
	APA_MEM_PLAT        plat;           // 平台
} APA_MEM_TAB;

/***********************************************************************************************************************
*输入音频帧结构体
***********************************************************************************************************************/

//音频帧头信息
typedef struct _APA_AUDIO_INFO_
{    
    unsigned int   channel_num;         // 声道数
    unsigned int   sample_rate;         // 采样率
    unsigned int   bits_per_sample;     // 每个采样点的bit数，公司现有的音频数据都是16bit
    unsigned int   data_len;            // 每通道数据采样点数，多通道则每个通道数据长度必须一样
    int            reserved[4];         // 保留字节    
}APA_AUDIO_INFO;

/***********************************************************************************************************************
*配置信息中高级参数相关信息结构体定义
***********************************************************************************************************************/

//算法库配置数据类型枚举
typedef enum _APA_SET_CFG_TYPE_
{
	APA_SET_CFG_SINGLE_PARAM      = 0x0001,         // 单个参数
	APA_SET_CFG_PARAM_LIST        = 0x0002,         // 参数表
	APA_SET_CFG_DEFAULT_PARAM     = 0x0003,         // 默认参数
	APA_SET_CFG_RESTART_LIB       = 0x0004,         // 重启算法库	
    APA_SET_CFG_END               = APA_ENUM_END
}APA_SET_CFG_TYPE;

//配置参数类型
typedef enum _APA_GET_CFG_TYPE_
{
	APA_GET_CFG_SINGLE_PARAM      = 0x0001,         // 单个参数
	APA_GET_CFG_PARAM_LIST        = 0x0002,         // 参数表
    APA_GET_CFG_DEFAULT_PARAM     = 0x0003,         // 默认参数
	APA_GET_CFG_VERSION           = 0x0004,         // 版本信息
    APA_GET_CFG_END               = APA_ENUM_END
}APA_GET_CFG_TYPE;


// 算法库参数结构体
typedef struct _APA_KEY_PARAM_
{
    int  index;
    int  value; 
}APA_KEY_PARAM;

// 参数链表
typedef struct _APA_KEY_PARAM_LIST_
{
    unsigned int   param_num;
    APA_KEY_PARAM  param[APA_MAX_PARAM_NUM];
}APA_KEY_PARAM_LIST;


#ifdef __cplusplus
}
#endif 

#endif /* _HIK_APA_BASE_H_ */

