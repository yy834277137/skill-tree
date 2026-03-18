#ifndef _XSP_DEF_HPP_
#define _XSP_DEF_HPP_
#include <cstdint> // 包含标准库的类型定义

/***************************************************************************************************
*                                         XSP基本数据类型
***************************************************************************************************/
/**
* @brief 基本数据类型的定义
* primitive types
*/

typedef float           float32_t;
typedef double          float64_t;

#if defined(_WIN32) || defined (_WIN64)
typedef signed __int64      int64;
typedef unsigned __int64    uint64;
#else
typedef signed long long    int64;
typedef unsigned long long  uint64;
#endif

/***************************************************************************************************
*                                      XSP自定义数据类型
***************************************************************************************************/
#define XSP_CN_MAX     512  /* 最大通道数 */
#define XSP_CN_SHIFT   3  
#define XSP_DEPTH_MAX  (1 << XSP_CN_SHIFT)

// @brief depth
#define XSP_8U   0 /* 000 */
#define XSP_8S   1 /* 001 */
#define XSP_16U  2 /* 010 */
#define XSP_16S  3 /* 011 */
#define XSP_32U  4 /* 100 */
#define XSP_32S  5 /* 101 */
#define XSP_32F  6 /* 110 */
#define XSP_64F  7 /* 111 */

// @brief 获取depth
#define XSP_MAT_DEPTH_MASK       (XSP_DEPTH_MAX - 1)
#define XSP_MAT_DEPTH(flags)     ((flags) & XSP_MAT_DEPTH_MASK)

// @brief 获取channel
#define XSP_MAT_CN_MASK          ((XSP_CN_MAX - 1) << XSP_CN_SHIFT)
#define XSP_MAT_CN(flags)        ((((flags) & XSP_MAT_CN_MASK) >> XSP_CN_SHIFT) + 1)

// @brief 生成自定义type
#define XSP_MAKETYPE(depth,cn) (XSP_MAT_DEPTH(depth) + (((cn)-1) << XSP_CN_SHIFT))
#define XSP_MAKE_TYPE XSP_MAKETYPE

// @brief 获取自定义type
#define XSP_MAT_TYPE_MASK        (XSP_DEPTH_MAX*XSP_CN_MAX - 1) /* 0x00000FFF */
#define XSP_MAT_TYPE(flags)      ((flags) & XSP_MAT_TYPE_MASK)

/** Size of each channel item,
0x84442211 = 1000 0100 0100 0100 0010 0010 0001 0001 ~ array of sizeof(arr_type_elem) */
#define XSP_ELEM_SIZE1(type) ((0x84442211 >> XSP_MAT_DEPTH(type)*4) & 15)

// @brief 获取自定义类型字节大小
#define XSP_ELEM_SIZE(type) (XSP_MAT_CN(type)*XSP_ELEM_SIZE1(type))

/*
* @brief 自定义数据类型
* @example XSP_8SC3 : | 0 0 0 0 | 0 0 0 1 | 0 0 0 1 |
*                       --------------------- -----
*    001 : XSP_8S 低三位存储depth类型
*    010 : | C1 000 | C2 001 | C3 010 | C4 011 | 高位存储channel
*/
// @{
#define XSP_8UC1 XSP_MAKETYPE(XSP_8U,1)
#define XSP_8UC2 XSP_MAKETYPE(XSP_8U,2)
#define XSP_8UC3 XSP_MAKETYPE(XSP_8U,3)
#define XSP_8UC4 XSP_MAKETYPE(XSP_8U,4)

#define XSP_8SC1 XSP_MAKETYPE(XSP_8S,1)
#define XSP_8SC2 XSP_MAKETYPE(XSP_8S,2)
#define XSP_8SC3 XSP_MAKETYPE(XSP_8S,3)
#define XSP_8SC4 XSP_MAKETYPE(XSP_8S,4)

#define XSP_16UC1 XSP_MAKETYPE(XSP_16U,1)
#define XSP_16UC2 XSP_MAKETYPE(XSP_16U,2)
#define XSP_16UC3 XSP_MAKETYPE(XSP_16U,3)
#define XSP_16UC4 XSP_MAKETYPE(XSP_16U,4)

#define XSP_16SC1 XSP_MAKETYPE(XSP_16S,1)
#define XSP_16SC2 XSP_MAKETYPE(XSP_16S,2)
#define XSP_16SC3 XSP_MAKETYPE(XSP_16S,3)
#define XSP_16SC4 XSP_MAKETYPE(XSP_16S,4)

#define XSP_32UC1 XSP_MAKETYPE(XSP_32U,1)
#define XSP_32UC2 XSP_MAKETYPE(XSP_32U,2)
#define XSP_32UC3 XSP_MAKETYPE(XSP_32U,3)
#define XSP_32UC4 XSP_MAKETYPE(XSP_32U,4)

#define XSP_32SC1 XSP_MAKETYPE(XSP_32S,1)
#define XSP_32SC2 XSP_MAKETYPE(XSP_32S,2)
#define XSP_32SC3 XSP_MAKETYPE(XSP_32S,3)
#define XSP_32SC4 XSP_MAKETYPE(XSP_32S,4)

#define XSP_32FC1 XSP_MAKETYPE(XSP_32F,1)
#define XSP_32FC2 XSP_MAKETYPE(XSP_32F,2)
#define XSP_32FC3 XSP_MAKETYPE(XSP_32F,3)
#define XSP_32FC4 XSP_MAKETYPE(XSP_32F,4)

#define XSP_64FC1 XSP_MAKETYPE(XSP_64F,1)
#define XSP_64FC2 XSP_MAKETYPE(XSP_64F,2)
#define XSP_64FC3 XSP_MAKETYPE(XSP_64F,3)
#define XSP_64FC4 XSP_MAKETYPE(XSP_64F,4)
// @} 自定义数据类型

#endif // _XSP_DEF_HPP_
