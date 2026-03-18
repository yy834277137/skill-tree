/**
 * @file    sal_type.h 
 * @note    Hangzhou Hikvision Digital Technology Co.,Ltd. All Right Reserved. 
 * @brief   统一定义常见数据类型 
 *  
 * @warning 除调用平台SDK, 算法SDK等第三方SDK时可用其指定的数据类型，其他情况必须使用该文件中定义的数据类型 
 * @note    若有缺失数据类型缺失情况，可发起组内讨论
 */

#ifndef _SAL_TYPES_H_
#define _SAL_TYPES_H_


/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

/**
 * stdint.h主要定义，屏蔽了与32bit和64bit差异： 
 * Exact integral types
 *   Signed: int8_t, int16_t, int32_t, int64_t 
 *   Unsigned: uint8_t, uint16_t, uint32_t, uint64_t 
 * Limits of integral types
 *   Minimum of signed integral types: INT8_MIN, INT16_MIN, INT32_MIN, INT64_MIN
 *   Maximum of signed integral types: INT8_MAX, INT16_MAX, INT32_MAX, INT64_MAX
 *   Maximum of unsigned integral types: UINT8_MAX, UINT16_MAX, UINT32_MAX, UINT64_MAX
 * Types for `void *' pointers: intptr_t, uintptr_t, if 32bit sys, as 32bit pointer, else if 64bit sys, as 64bit pointer
 * Limit of `size_t' type: SIZE_MAX, if 32bit sys, as 4294967295U, else if 64bit sys, as 18446744073709551615UL
 * 64bit system: #if __WORDSIZE == 64
 */

/**
 * stddef.h主要定义： 
 * NULL: ((void *)0) 
 * offsetof(TYPE, MEMBER): 计算结构体中某个成员变量在该结构体中的偏移量 
 * size_t: 提供一种可移植的方法来声明与系统中可寻址的内存区域一致的长度，亦sizeof的返回值，使用%z格式化输出
 * ptrdiff_t：有符号整数类型，两个指针在内存中的距离 
 * wchar_t：为国际化的字符集提供宽字符类型 
 */

/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */
/**
 * 布尔类型 
 * TRUE使用：SAL_TRUE 
 * FALSE使用：SAL_FALSE
 */
#ifndef HIK_TYPE_BOOL
#define HIK_TYPE_BOOL
typedef int                 BOOL;
#endif

#define SAL_UNUSED(x)       (void)x

#define SAL_TRUE            (1)
#define SAL_FALSE           (0)

/**
 * 常用函数接口返回值
 */
typedef int                 SAL_STATUS; /* 函数返回类型 */

#define SAL_SOK             (0)         /* 成功 */
#define SAL_FAIL            (-1)        /* 失败 */
#define SAL_ETIMEOUT        (-2)        /* 等待超时 */
#define SAL_EBUSY           (-3)        /* 忙碌需等待 */
#define SAL_EINTR           (-4)        /* 被中断 */

/* 有符号类型定义 */
/**
 * char和unsigned char在某些编译器上解释是不同的 
 * char在某些编译器中被会当作无符号8位整数处理 
 * char变量一般用于存储字符数据，避免与负值进行算术与逻辑运算 
 * 数值常量用signed char和unsigned char
 * stdint.h有定义int8_t和uint8_t 
 */
#ifndef HIK_TYPE_INT8
#define HIK_TYPE_INT8
typedef char                CHAR;       /* 字符类型 */
typedef signed char         INT8;       /* 有符号8位整形数类型 */
typedef signed char         S08;        /* 有符号8位整形数类型 */
#endif

#ifndef HIK_TYPE_INT16
#define HIK_TYPE_INT16
typedef short               INT16;      /* 有符号16位整形数类型 */
typedef signed short        S16;        /* 有符号16位整形数类型 */
#endif


#ifndef HIK_TYPE_INT32
#define HIK_TYPE_INT32
typedef int                 INT32;      /* 有符号32位整形数类型 */
typedef signed int          S32;        /* 有符号32位整形数类型 */
#endif

#ifndef HIK_TYPE_INT64L
#define HIK_TYPE_INT64L
typedef long long           INT64L;     /* 有符号64位整形数类型 */
#endif

/* 无符号类型定义 */
#ifndef HIK_TYPE_UINT8
#define HIK_TYPE_UINT8
typedef unsigned char       UINT8;      /* 无符号8位整形数类型 */
typedef unsigned char       U08;        /* 无符号8位整形数类型 */
#endif

#ifndef HIK_TYPE_UINT16
#define HIK_TYPE_UINT16
typedef unsigned short      UINT16;     /* 无符号16位整形数类型 */
typedef unsigned short      U16;        /* 无符号16位整形数类型 */
#endif


#ifndef HIK_TYPE_UINT32
#define HIK_TYPE_UINT32
typedef unsigned int        UINT32;     /* 无符号32位整形数类型 */
typedef unsigned int        U32;        /* 无符号32位整形数类型 */
#endif

#ifndef HIK_TYPE_UINTL
#define HIK_TYPE_UINTL
typedef unsigned long       UINTL;      /* 无符号长整形数类型，位数不定，32位系统上为32位，64位系统上为64位，谨慎使用 */
#endif

#ifndef HIK_TYPE_UINT64
#define HIK_TYPE_UINT64
typedef unsigned long long  UINT64;     /* 无符号64位整形数类型 */
#endif

/* 浮点类型定义 */
#ifndef HIK_TYPE_FLOAT32
#define HIK_TYPE_FLOAT32
typedef float               FLOAT32;    /* 32位浮点数类型 */
#endif

#ifndef HIK_TYPE_FLOAT64
#define HIK_TYPE_FLOAT64
typedef double              FLOAT64;    /* 64位浮点数类型 */
#endif

/* 指针类型定义 */
#ifndef HIK_TYPE_PTR
#define HIK_TYPE_PTR
typedef void *              Ptr;         /* 指针类型 */
#endif

/* 句柄类型 */
#ifndef HIK_TYPE_Handle
#define HIK_TYPE_Handle
typedef void *              Handle;      /* 统用句柄类型 */
#endif

#ifndef HIK_TYPE_PUINT8
#define HIK_TYPE_PUINT8
typedef unsigned char       *PUINT8;    /* 无符号8位整形数类型 */
#endif

#ifndef HIK_TYPE_VOID
#define HIK_TYPE_VOID
typedef void                VOID;
#endif

#ifndef NULL
#define NULL 0
#endif

#if __WORDSIZE == 64
typedef unsigned long long  PhysAddr;   /* 64 位系统的内存地址描述 */
#else
typedef unsigned int        PhysAddr;   /* 32 位系统的内存地址描述 */
#endif

#ifdef __cplusplus
}
#endif

#endif                                  /*  _SAL_TYPES_H_  */


