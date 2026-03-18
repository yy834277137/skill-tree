/**************************************************************************************************
* 
* 版权信息：版权所有 (c) 2010-2018, 杭州海康威视软件有限公司, 保留所有权利
*
* 文件名称：hiknlu_common.h
* 文件标识：hiknlu_COMMON_H_
* 摘    要：海康威视NLU公共数据结构体声明文件

* 当前版本：0.0.1
* 作    者：沈力行
* 日    期：2018年12月20日
* 备    注：初始版本
**************************************************************************************************/
#ifndef _HIKNLU_COMMON_H_
#define _HIKNLU_COMMON_H_


#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif


/**************************************************************************************************
* 错误码
**************************************************************************************************/
#define HIK_NLU_LIB_S_OK                      1           // 成功	  
#define HIK_NLU_LIB_E_MEM_OUT                 0x81F10001  // 内存不够
#define HIK_NLU_LIB_E_PTR_NULL	              0x81F10002  // 传入指针为空 
#define HIK_NLU_LIB_E_MODEL_FILE_PATH_ERR     0x81F10003  // 模型文件路径错误
#define HIK_NLU_LIB_E_FREAD_FAIL              0x81F10004  // 文件读入结尾发生错误
#define HIK_NLU_LIB_E_OVERSTEP                0x81F10005  // 越界错误
#define HIK_NLU_LIB_E_LACK_PARAMETER          0x81F10006  // 缺失必要参数
#define HIK_NLU_LIB_E_INPUT_SIZE_ERR          0x81F10007  // 输入长度错误
#define HIK_NLU_VECTOR_E_DIM_VALUE_ERR        0x81F10008  // 矩阵库向量值错误
#define HIK_NLU_LIB_UTF_INPUT_ERR             0x81F10009  // UTF-8输入错误
#define HIK_NLU_LIB_E_VALUE_ERR               0x81F1000A  // 输入值错误
#define HIK_NLU_LIB_E_PROCESS                 0x81F1000B  // 流程错误

//配置文件相关错误码
#define HIK_NLU_LIB_E_CONF_FILE_PATH_ERR     0x81F10012  // 配置文件路径错误
#define HIK_NLU_LIB_E_CONF_FILE_ERR          0x81F10013  // 配置文件格式错误
#define HIK_NLU_LIB_E_CONF_FILE_VALUE_ERR    0x81F10014  // 配置文件值错误

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef unsigned int HRESULT;
#endif /* _HRESULT_DEFINED */


/**************************************************************************************************
* 内存
**************************************************************************************************/

#define HIK_NLU_MEM_TAB_NUM         3         // 内存表的个数
#define HIK_NLU_ENUM_END            0xFFFFFF  // 定义枚举结束值，用于对齐
#define HIK_NLU_MAX_PARAM_NUM       50        // 最多关键参数个数

/***********************************************************************************************************************
*内存管理器NLU_MEM_TAB结构体定义
***********************************************************************************************************************/

/*内存对齐属性*/
typedef enum _NLU_MEM_ALIGNMENT_
{
    NLU_MEM_ALIGN_4BYTE    = 4,
    NLU_MEM_ALIGN_8BYTE    = 8,
    NLU_MEM_ALIGN_16BYTE   = 16,
    NLU_MEM_ALIGN_32BYTE   = 32,
    NLU_MEM_ALIGN_64BYTE   = 64,
    NLU_MEM_ALIGN_128BYTE  = 128,
    NLU_MEM_ALIGN_256BYTE  = 256,
    NLU_MEM_ALIGN_END      = HIK_NLU_ENUM_END
}NLU_MEM_ALIGNMENT;

/* 内存属性 */
typedef enum _NLU_MEM_ATTRS_
{
    NLU_MEM_SCRATCH,                 /* 可复用内存，能在多路切换时有条件复用 */ 
    NLU_MEM_PERSIST,                 /* 不可复用内存 */ 
    NLU_MEM_ATTRS_END = HIK_NLU_ENUM_END
}NLU_MEM_ATTRS;


typedef enum _NLU_MEM_PLAT_
{
	NLU_MEM_PLAT_CPU,                 /* CPU内存 */ 
	NLU_MEM_PLAT_GPU,                 /* GPU内存 */ 
	NLU_MEM_PLAT_END = HIK_NLU_ENUM_END
} NLU_MEM_PLAT;

/* 内存分配空间 */
typedef enum _NLU_MEM_SPACE_ 
{
    NLU_MEM_EXTERNAL_PROG,              /* 外部程序存储区          */  
    NLU_MEM_INTERNAL_PROG,              /* 内部程序存储区          */
    NLU_MEM_EXTERNAL_TILERED_DATA,      /* 外部Tilered数据存储区   */
    NLU_MEM_EXTERNAL_CACHED_DATA,       /* 外部可Cache存储区       */
    NLU_MEM_EXTERNAL_UNCACHED_DATA,     /* 外部不可Cache存储区     */
    NLU_MEM_INTERNAL_DATA,              /* 内部存储区              */
    NLU_MEM_EXTERNAL_TILERED8 ,         /* 外部Tilered数据存储区8bit，Netra/Centaurus特有 */
    NLU_MEM_EXTERNAL_TILERED16,         /* 外部Tilered数据存储区16bit，Netra/Centaurus特有 */
    NLU_MEM_EXTERNAL_TILERED32 ,        /* 外部Tilered数据存储区32bit，Netra/Centaurus特有 */
    NLU_MEM_EXTERNAL_TILEREDPAGE,       /* 外部Tilered数据存储区page形式，Netra/Centaurus特有 */
	NLU_MEM_EXTERNAL_DATA_IVE,          // add by wxc @ 20160422
    NLU_MEM_EXTERNAL_END = HIK_NLU_ENUM_END
} NLU_MEM_SPACE;

#if ((!defined _HIK_COMPLEX_MEM_) && (!defined DSP) && (!defined _HIK_DSP_APP_))
typedef struct _NLU_MEM_TAB_
{
    unsigned int        size;       /* 以BYTE为单位的内存大小*/
    NLU_MEM_ALIGNMENT   alignment;  /* 内存对齐属性, 建议为128 */
    void*               base;       /* 分配出的内存指针 */
} NLU_MEM_TAB;
#else
/* 内存分配结构体 */
typedef struct _NLU_MEM_TAB_
{
    unsigned int        size;       /* 以BYTE为单位的内存大小*/
    NLU_MEM_ALIGNMENT   alignment;  /* 内存对齐属性, 建议为128 */
    NLU_MEM_SPACE       space;      /* 内存分配空间 */
    NLU_MEM_ATTRS       attrs;      /* 内存属性 */
    void                *base;       /* 分配出的内存指针 */
} NLU_MEM_TAB;
#endif  /* ((!defined _HIK_COMPLEX_MEM_) && (!defined DSP) && (!defined _HIK_DSP_APP_)) */

typedef struct _NLU_MEM_TAB_V2_
{
	unsigned int        size;       /* 以BYTE为单位的内存大小*/
	NLU_MEM_ALIGNMENT   alignment;  /* 内存对齐属性, 建议为128 */
	NLU_MEM_SPACE       space;      /* 内存分配空间 */
	NLU_MEM_ATTRS       attrs;      /* 内存属性 */
	void                *base;      /* 分配出的内存指针 */
	NLU_MEM_PLAT        plat;       /* 平台 */
} NLU_MEM_TAB_V2;

//存储空间管理结构体
typedef struct _NLU_MEM_BUF_
{
    void     *start;             //缓存起始位置
    void     *end;               //缓存结束位置
    void     *cur_pos;           //缓存空余起始位置
} NLU_MEM_BUF;



// 算法库参数结构体
typedef struct _NLU_KEY_PARAM_
{
    int  index;
    int  value; 
} NLU_KEY_PARAM;

// 参数链表
typedef struct _NLU_KEY_PARAM_LIST_
{
    unsigned int   param_num;
    NLU_KEY_PARAM  param[HIK_NLU_MAX_PARAM_NUM];
} NLU_KEY_PARAM_LIST;


// v0.05
/**************************************************************************************************
* 错误码
**************************************************************************************************/

#define HIKNLU_S_OK                    0x00000001  /* 成功 */

#define HIKNLU_S_FAIL_ERR              0x81f20000  /* 当函数调用失败，且暂时无法归到已定义的错误类型 */
#define HIKNLU_MEM_ALLOC_FAIL_ERR      0x81f20003  /* 分配内存时，若分配的内存为空或大小不满足要求 */
#define HIKNLU_MEM_OVER_ERR            0x81f20004  /* 使用内存前检测到内存空间不够 */
#define HIKNLU_MEM_CHECK_FAIL_ERR      0x81f20005  /* 使用前指定空间设定特定值，使用后检测特定值， */
                                                   /* 若特定值被修改，则返回该错误码 */
#define HIKNLU_PTR_NULL_ERR            0x81f20006  /* 传入参数的指针为空 */
#define HIKNLU_PARAM_ERR               0x81f20007  /* 传入参数值不在规定范围内或为不合理值 */
#define HIKNLU_DOG_CHECK_FAIL_ERR      0x81f20008  /* 未插加密狗或者加密狗损坏，返回该错误码 */
#define HIKNLU_OPEN_FILE_FAIL_ERR      0x81f20009  /* 打开文件失败 */
#define HIKNLU_READ_FAIL_ERR           0x81f2000a  /* 从文件读取数据出错 */
#define HIKNLU_WRITE_FAIL_ERR          0x81f2000b  /* 向文件写数据出错 */
#define HIKNLU_READ_VERIFY_ERR         0x81f2000c  /* 校验读取的数据出错 */
#define HIKNLU_MEM_NOT_ALIGNED_WARNING 0x81f20101  /* 内存地址不是4字节或8字节等字节对齐的 */
#define HIKNLU_NOT_INT_DIVIDED_WARNING 0x81f20102  /* 某一数值不能被另一数值整除，返回该警告码 */

#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef unsigned int HRESULT;
#endif /* _HRESULT_DEFINED */


/**************************************************************************************************
* 内存
**************************************************************************************************/

#define HIKNLU_MEM_TAB_NUM         3         // 内存表的个数
#define HIKNLU_ENUM_END 	  	   0xFFFFFF  // 定义枚举结束值，用于对齐
#define HIKNLU_MAX_PARAM_NUM       50        // 最多关键参数个数

/**********************************************************************************************************************
*内存管理器NLU_MEM_TAB结构体定义
**********************************************************************************************************************/






#if ((!defined _HIK_COMPLEX_MEM_) && (!defined DSP) && (!defined _HIK_DSP_APP_))
typedef struct _HIKNLU_MEM_TAB_
{
    unsigned int           size;       /* 以BYTE为单位的内存大小*/
    NLU_MEM_ALIGNMENT   alignment;  /* 内存对齐属性, 建议为128 */
    void*                  base;       /* 分配出的内存指针 */
} HIKNLU_MEM_TAB;
#else
/* 内存分配结构体 */
typedef struct _HIKNLU_MEM_TAB_
{
    unsigned int          size;       /* 以BYTE为单位的内存大小*/
    NLU_MEM_ALIGNMENT  alignment;  /* 内存对齐属性, 建议为128 */
    NLU_MEM_SPACE         space;      /* 内存分配空间 */
    NLU_MEM_ATTRS         attrs;      /* 内存属性 */
    void                 *base;       /* 分配出的内存指针 */
} HIKNLU_MEM_TAB;
#endif  /* ((!defined _HIK_COMPLEX_MEM_) && (!defined DSP) && (!defined _HIK_DSP_APP_)) */

typedef struct _HIKNLU_MEM_TAB_V2_
{
	unsigned int           size;       /* 以BYTE为单位的内存大小*/
	NLU_MEM_ALIGNMENT   alignment;  /* 内存对齐属性, 建议为128 */
	NLU_MEM_SPACE       space;      /* 内存分配空间 */
	NLU_MEM_ATTRS       attrs;      /* 内存属性 */
	void                  *base;       /* 分配出的内存指针 */
	NLU_MEM_PLAT        plat;       /* 平台 */
} HIKNLU_MEM_TAB_V2;

/* 存储空间管理结构体 */
typedef struct _HIKNLU_MEM_BUF_
{
    void     *start;             //缓存起始位置
    void     *end;               //缓存结束位置
    void     *cur_pos;           //缓存空余起始位置
} HIKNLU_MEM_BUF;



/* 算法库参数结构体 */
typedef struct _HIKNLU_KEY_PARAM_
{
    int  index;
    int  value; 
} HIKNLU_KEY_PARAM;

/* 参数链表 */
typedef struct _HIKNLU_KEY_PARAM_LIST_
{
    unsigned int      param_num;
    HIKNLU_KEY_PARAM  param[HIKNLU_MAX_PARAM_NUM];
} HIKNLU_KEY_PARAM_LIST;


// from hiknlu_types.h
/**************************************************************************************************
* 数据类型
**************************************************************************************************/

/** 添加陈云川部分代码 **/
typedef  uint8_t     U08;      //无符号字节类型
// 不应该使用 S08* 作为字符串的类型。否则可能会产生大量警告。而应该 使用 char* 作为
// 字符串的类型。 int8_t 应该作为有符号的整数使用。不应该将 char 视为整数。因为它是
// 否表示有符号的整数不确定的，不同的编译器可能不同。在标准中，char既可能是有符号的，
// 也可能是无符号的，这是由不同实现决定的。 因此如果使用 char 作为 8 位的整数，可能
// 导致不可移植的问题。
typedef  int8_t      S08;     //有符号字节类型
typedef  uint16_t    U16;     //无符号字类型
typedef  int16_t   	 S16;     //有符号字类型
typedef  uint32_t    U32;     //无符号双字类型
typedef  int32_t     S32;     //有符号双字类型
typedef  uint64_t	 U64;     //无符号8字节类型
typedef  int64_t	 S64;     //有符号8字节类型

/** public **/
#define HIKNLU_8U   0          //8位无符号数
#define HIKNLU_8S   1          //8位有符号数
#define HIKNLU_16U  2          //16位无符号数
#define HIKNLU_16S  3          //16位有符号数
#define HIKNLU_32U  4          //32位无符号数
#define HIKNLU_32S  5          //32位有符号数
#define HIKNLU_32F  6          //32位单精度浮点数
#define HIKNLU_64F  7          //64位双精度浮点数
#define HIKNLU_64U  8          //64位无符号数
#define HIKNLU_64S  9          //64位有符号数

#ifndef NULL
#define NULL 0
#endif

// defined in HIKSPEECH
// #ifndef FALSE
//     //! Boolean Type
//     typedef enum {
//         FALSE,
//         TRUE
//     } Boolean;
// #endif

/**************************************************************************************************
* 常用数学宏定义
**************************************************************************************************/
#define HIKNLU_EPS                          0.0000001f
#define HIKNLU_PI                           3.1415926f 
#define HIKNLU_ABS(a)                       (((a) > 0) ? (a) : -(a))
#define HIKNLU_ROUND(a)                     (int)((a) + (((a) >= 0) ? 0.5 : -0.5))
#define HIKNLU_LINE_LOCATION(s, l, u, b, e) (int)(1.0 * ((s) - (l)) / ((u) - (l)) * ((e) - (b)) + (b))
//大值，小值
#define HIKNLU_MAX(a, b)         (((a) > (b)) ? (a) : (b))
#define HIKNLU_MIN(a, b)         (((a) < (b)) ? (a) : (b))

/**************************************************************************************************
* 函数调用规范
**************************************************************************************************/
#ifdef WIN32
#define restrict
#endif

#ifndef __cplusplus
#ifdef _WIN32
#define inline __inline
#endif
#endif /* __cplusplus */

/**************************************************************************************************
* 校验
**************************************************************************************************/
#define HIKNLU_CHECK_ERROR(state, error_code) \
if (state)                                 \
{                                          \
    return error_code;                     \
}

#define HIKNLU_CHECK_RETURN(state) \
if (state)                      \
{                               \
    return;                     \
}

//循环跳转校验
#define HIKNLU_CHECK_CONTINUE(state)      \
if (state)                             \
{                                      \
    continue;                          \
}

/**************************************************************************************************
* 内存管理器MEM_TAB结构体定义
**************************************************************************************************/

/*
 *  所有大小先转换为 size_t，再进行计算，以保证在64位机器上，即便未来遇到 size > 2^32也能正确工作
 */
#define HIKNLU_SIZE_ALIGN(size, align) ( ((size_t)(size) + ( (size_t)(align) - 1 )) & ( -((size_t)(align)) ) )
#define HIKNLU_SIZE_ALIGN_8(size)   HIKNLU_SIZE_ALIGN(size,  8)
#define HIKNLU_SIZE_ALIGN_16(size)  HIKNLU_SIZE_ALIGN(size, 16)
#define HIKNLU_SIZE_ALIGN_32(size)  HIKNLU_SIZE_ALIGN(size, 32)
#define HIKNLU_SIZE_ALIGN_64(size)  HIKNLU_SIZE_ALIGN(size, 64)
#define HIKNLU_SIZE_ALIGN_128(size) HIKNLU_SIZE_ALIGN(size, 128)
#ifdef  WIN32
#define HIKNLU_ALIGN_16           __declspec(align(16))
#define HIKNLU_CACHE_LINE_SIZE    64
#else
#define HIKNLU_ALIGN_16
#define HIKNLU_CACHE_LINE_SIZE    128
#endif




#ifdef __cplusplus
}
#endif 

#endif /* _HIKNLU_COMMON_H_ */
