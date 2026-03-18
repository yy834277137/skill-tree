/**
 * Image SoLution Hardware Abstraction Layer Interface 
 * 不同硬件平台上的数据类型统一定义 
 */
#ifndef __ISL_HAL_H__
#define __ISL_HAL_H__

#if defined __cplusplus
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cfloat>
#include <climits>
#else
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#endif

// 如果编译器不支持std::float32_t和float64_t，则需要自己定义
#if defined(__STDCPP_FLOAT32_T__)
using float32_t = std::float32_t;
#else
//#error "32-bit float type required"
typedef float float32_t; 
#endif
#if defined(__STDCPP_FLOAT64_T__)
using float64_t = std::float64_t;
#else
//#error "64-bit float type required"
typedef double float64_t;
#endif

#ifndef ALWAYS_INLINE
#  if defined(__GNUC__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#    define ALWAYS_INLINE inline __attribute__((always_inline))
#  elif defined(_MSC_VER)
#    define ALWAYS_INLINE __forceinline
#  else
#    define ALWAYS_INLINE inline
#  endif
#endif

/**
 * F E D C B A 9 8 7 6 5 4 3 2 1 0
 *              | CN  |BTP| Depth |
 */
/// BW：占用位宽（Bit Width）
#define ISL_DEPTH_BW    4   // 深度（Depth）占4bit，Bit0~3，直接表示占用字节数
#define ISL_BTP_BW      2   // 基础数据类型（Basic DataType）占2bit，Bit4~5，Unsigned-0、Signed-1、Float-2
#define ISL_CN_BW       3   // 通道（ChaNnel）占3bit，Bit6~8，直接表示通道数，最多支持4个通道，0个通道仅表示类型

/// 基础数据类型（Basic DataType）
#define ISL_TPU         0   // Unsigned
#define ISL_TPS         1   // Signed
#define ISL_TPF         2   // Float

/// 深度的Mask与获取接口
#define ISL_MAT_DEPTH_MASK      ((1 << ISL_DEPTH_BW) - 1)
#define ISL_MAT_DEPTH(flags)    ((flags) & ISL_MAT_DEPTH_MASK) // 单个通道数据的深度

/// 基础数据类型（Unsigned、Signed、Float）的Mask与获取接口
#define ISL_MAT_BTP_MASK        (((1 << ISL_BTP_BW) - 1) << ISL_DEPTH_BW)
#define ISL_MAT_BTP(flags)      (((flags) & ISL_MAT_BTP_MASK) >> ISL_DEPTH_BW)

/// 完整数据类型（Derived DataType）的Mask与获取接口，基础数据类型+深度，见宏定义ISL_U8、ISL_U16、ISL_F64等
#define ISL_MAT_DTP_MASK        ((1 << (ISL_BTP_BW + ISL_DEPTH_BW)) - 1)
#define ISL_MAT_DTP(flags)      (((flags) & ISL_MAT_DTP_MASK))

/// 通道数的Mask与获取接口
#define ISL_MAT_CN_MASK         (((1 << ISL_CN_BW) - 1) << (ISL_BTP_BW + ISL_DEPTH_BW))
#define ISL_MAT_CN(flags)       (((flags) & ISL_MAT_CN_MASK) >> (ISL_BTP_BW + ISL_DEPTH_BW))

/// 整个Imat类型的Mask与获取接口
#define ISL_MAT_TYPE_MASK       ((1 << (ISL_DEPTH_BW + ISL_BTP_BW + ISL_CN_BW)) - 1)
#define ISL_MAT_ITP(flags)      ((flags) & ISL_MAT_TYPE_MASK)

/// 整个Imat类型的生成接口
#define ISL_MAKETYPE(depth, btp, cn) ((depth) | ((btp) << ISL_DEPTH_BW) | ((cn) << (ISL_DEPTH_BW + ISL_BTP_BW)))

/// 单个元素（像素）占用的字节数
#define ISL_ELEM_SIZE(flags)    (ISL_MAT_DEPTH(flags) * ISL_MAT_CN(flags))

// 常用数学量定义
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


///  下面是Imat类型的定义
#define ISL_U8      ISL_MAKETYPE(1, ISL_TPU, 0)
#define ISL_U8C1    ISL_MAKETYPE(1, ISL_TPU, 1)
#define ISL_U8C2    ISL_MAKETYPE(1, ISL_TPU, 2)
#define ISL_U8C3    ISL_MAKETYPE(1, ISL_TPU, 3)
#define ISL_U8C4    ISL_MAKETYPE(1, ISL_TPU, 4)

#define ISL_S8      ISL_MAKETYPE(1, ISL_TPS, 0)
#define ISL_S8C1    ISL_MAKETYPE(1, ISL_TPS, 1)
#define ISL_S8C2    ISL_MAKETYPE(1, ISL_TPS, 2)
#define ISL_S8C3    ISL_MAKETYPE(1, ISL_TPS, 3)
#define ISL_S8C4    ISL_MAKETYPE(1, ISL_TPS, 4)

#define ISL_U16     ISL_MAKETYPE(2, ISL_TPU, 0)
#define ISL_U16C1   ISL_MAKETYPE(2, ISL_TPU, 1)
#define ISL_U16C2   ISL_MAKETYPE(2, ISL_TPU, 2)
#define ISL_U16C3   ISL_MAKETYPE(2, ISL_TPU, 3)
#define ISL_U16C4   ISL_MAKETYPE(2, ISL_TPU, 4)

#define ISL_S16     ISL_MAKETYPE(2, ISL_TPS, 0)
#define ISL_S16C1   ISL_MAKETYPE(2, ISL_TPS, 1)
#define ISL_S16C2   ISL_MAKETYPE(2, ISL_TPS, 2)
#define ISL_S16C3   ISL_MAKETYPE(2, ISL_TPS, 3)
#define ISL_S16C4   ISL_MAKETYPE(2, ISL_TPS, 4)

#define ISL_U32     ISL_MAKETYPE(4, ISL_TPU, 0)
#define ISL_U32C1   ISL_MAKETYPE(4, ISL_TPU, 1)
#define ISL_U32C2   ISL_MAKETYPE(4, ISL_TPU, 2)

#define ISL_S32     ISL_MAKETYPE(4, ISL_TPS, 0)
#define ISL_S32C1   ISL_MAKETYPE(4, ISL_TPS, 1)
#define ISL_S32C2   ISL_MAKETYPE(4, ISL_TPS, 2)

#define ISL_F32     ISL_MAKETYPE(4, ISL_TPF, 0)
#define ISL_F32C1   ISL_MAKETYPE(4, ISL_TPF, 1)

#define ISL_F64     ISL_MAKETYPE(8, ISL_TPF, 0)
#define ISL_F64C1   ISL_MAKETYPE(8, ISL_TPF, 1)

/// 系统定义的数据类型（int8_t、uint16_t、int32_t、float64_t等）判断，编译时执行
template<typename _T>
struct is_s8 : std::integral_constant<bool, std::is_same<typename std::decay<_T>::type, int8_t>::value> {};
template<typename _T>
inline constexpr bool is_s8_v = is_s8<_T>::value;

template<typename _T>
struct is_u8 : std::integral_constant<bool, std::is_same<typename std::decay<_T>::type, uint8_t>::value> {};
template<typename _T>
inline constexpr bool is_u8_v = is_u8<_T>::value;

template<typename _T>
struct is_s16 : std::integral_constant<bool, std::is_same<typename std::decay<_T>::type, int16_t>::value> {};
template<typename _T>
inline constexpr bool is_s16_v = is_s16<_T>::value;

template<typename _T>
struct is_u16 : std::integral_constant<bool, std::is_same<typename std::decay<_T>::type, uint16_t>::value> {};
template<typename _T>
inline constexpr bool is_u16_v = is_u16<_T>::value;

template<typename _T>
struct is_s32 : std::integral_constant<bool, std::is_same<typename std::decay<_T>::type, int32_t>::value> {};
template<typename _T>
inline constexpr bool is_s32_v = is_s32<_T>::value;

template<typename _T>
struct is_u32 : std::integral_constant<bool, std::is_same<typename std::decay<_T>::type, uint32_t>::value> {};
template<typename _T>
inline constexpr bool is_u32_v = is_u32<_T>::value;

template<typename _T>
struct is_f32 : std::integral_constant<bool, std::is_same<typename std::decay<_T>::type, float32_t>::value> {};
template<typename _T>
inline constexpr bool is_f32_v = is_f32<_T>::value;

template<typename _T>
struct is_f64 : std::integral_constant<bool, std::is_same<typename std::decay<_T>::type, float64_t>::value> {};
template<typename _T>
inline constexpr bool is_f64_v = is_f64<_T>::value;

#endif

