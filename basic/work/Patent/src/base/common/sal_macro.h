/**
 * @file   sal_macro.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  宏定义操作
 * @author  
 * @date    
 * @note
 * @note \n History
   1.Date        : 2021.8.19
     Author      : yeyanzhong
     Modification: 把system_prm.h里的操作缓冲区的宏定义统一放到sal_macro.h里
*/

#ifndef _SAL_MACRO_H_
#define _SAL_MACRO_H_

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/**
 * 名词缩写 
 * SAL		System Abstract Layer，中间抽象层
 */


/* 字符串拼接 */
#define CAT(a, b)                       a ## b
#define CAT3(a, b, c)                   a ## b ## c
#define CAT4(a, b, c, d)                a ## b ## c ## d


/**
 * 函数接口输入输出参数标识
 */
#define IN				/* 函数接口输入参数标识 */
#define OUT				/* 函数接口输出参数标识 */
#define IO				/* 函数接口输入输出参数标识，即作为输入参数，又作为输出参数 */

/* 编译器选项 */
#define ATTRIBUTE_UNUSED __attribute__((unused))
#define ATTRIBUTE_WEAK   __attribute__((weak))

/* 超时参数 */
#define SAL_TIMEOUT_NONE                (0)    /* 不等待，立即返回 */ 
#define SAL_TIMEOUT_FOREVER             (~0U)  /* 一直等待直到返回 */

/* 数据存储度量单位 */
#define SAL_KB                          (1024)
#define SAL_MB                          (SAL_KB * SAL_KB)
#define SAL_GB                          (SAL_KB * SAL_MB)

/* 交换操作 */
#define SAL_SWAP(x, y)                  do {x ^= y; y = x^y; x = x^y;} while (0)

/* 数值比较 */
#define SAL_MAX(a, b)                   ((a) > (b) ? (a) : (b))
#define SAL_MIN(a, b)                   ((a) < (b) ? (a) : (b))
#define SAL_CLIP(a, min, max)           (SAL_MIN(SAL_MAX(a, min), (max)))

/* 绝对值*/
#define SAL_ABS(x)                      ((x) >= 0 ? (x) : (-(x)))

/* 减法操作 */
#ifndef SAL_SUB_SAFE // 被减数小于减数则强制置0
#define SAL_SUB_SAFE(a, b)				(((a) > (b)) ? ((a) - (b)) : 0)
#endif
#ifndef SAL_SUB_ABS // 两数相减的绝对值
#define SAL_SUB_ABS(a, b)				(((a) > (b)) ? ((a) - (b)) : ((b) - (a)))
#endif

/* 除法操作 */
#ifndef SAL_DIV_SAFE // 除数为0不做操作
#define SAL_DIV_SAFE(a, b)              (0 == (b) ? (a) : (a) / (b))
#endif

/* 内存操作*/
#define SAL_clear(ptr)                  memset((ptr), 0, sizeof(*(ptr)))
#define SAL_clearSize(ptr, size)        memset((ptr), 0, (size))
#define SAL_memCpySize(dst, src, size)  memcpy((dst), (src), (size))

/* 判断返回值 */
#define SAL_isSuccess(status)           (SAL_SOK == (status))
#define SAL_isFail(status)              (SAL_SOK != (status))
#define SAL_isNull(ptr)                 (NULL == (ptr))

/* 对齐操作*/
#define SAL_align(value, align)         (((value) + (align - 1)) & ~(align - 1))
#define SAL_alignDown(value, align)     ((value) & ~(align - 1))
#define SAL_isAligned(value, align)     (((value) & (align - 1)) == 0)
#define SAL_CEIL(value, align)          ((0 == (value) % (align)) ? ((value) / (align)) : ((value) / (align) + 1))
#define SAL_floor(value, align)         (( (value) / (align) ) * (align) )

/* 获取数组成员数量 */
#define SAL_arraySize(array)            (sizeof(array)/sizeof((array)[0]))

/* 获取结构体成员的地址偏移量 */
#ifdef __compiler_offsetof
#define SAL_offsetof(TYPE,MEMBER)       (__compiler_offsetof(TYPE,MEMBER))
#else
#define SAL_offsetof(TYPE, MEMBER)      ((size_t) &((TYPE *)0)->MEMBER)
#endif


/* 通过结构体成员获取结构体首地址 */
#define SAL_containerOf(ptr, type, member) ({                       \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);       \
        (type *)( (char *)__mptr - SAL_offsetof(type,member) );})


/* 检测value值是否是2的N次方 */
#define SAL_checkIs2n(value)                ( ((value) == 0) ? SAL_FALSE  \
                                                : ((value) & ((value) - 1))  \
                                                ? SAL_FALSE : SAL_TRUE )
                                                

/* ========================================================================== */
/*                                操作缓冲区的宏定义                          */
/* ========================================================================== */

/*
缓冲索引操作，假设索引为长整型
*/
#define FORWARD(_idx_,_len_)        ((_idx_)==(_len_)-1?0:(_idx_)+1)
#define BACKWARD(_idx_,_len_)       ((_idx_)==0?(_len_)-1:(_idx_)-1)
#define GO_FORWARD(_idx_,_len_)     _idx_=FORWARD(_idx_,_len_)
#define GO_BACKWARD(_idx_,_len_)    _idx_=BACKWARD(_idx_,_len_)
/*
缓冲索引操作，假设索引为长整型，长度为2的n次方,用取模操作(MOD)。
*/
#define FORWARD_M(_idx_,_len_)      (((_idx_)+1)&((_len_)-1))
#define BACKWARD_M(_idx_,_len_)     (((_idx_)-1)&((_len_)-1))
#define GO_FORWARD_M(_idx_,_len_)   _idx_=FORWARD_M(_idx_,_len_)
#define GO_BACKWARD_M(_idx_,_len_)  _idx_=BACKWARD_M(_idx_,_len_)
/*
计算缓冲剩余个数，假设索引为长整型
*/
#define DIST(_x_,_y_,_len_) (((_x_)>=(_y_))?((_x_)-(_y_)):((_x_)+(_len_)-(_y_)))



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif

