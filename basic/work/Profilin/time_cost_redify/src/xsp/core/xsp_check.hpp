/**    @file xsp_check.h
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*      @brief 参数检查宏定义工具
*
*      @author wangtianshu
*      @date 2022/11/12
*
*      @note 历史记录
*      @note V0.0.1 实现初版参数检查功能
*/

#ifndef _XSP_CHECK_HPP_
#define _XSP_CHECK_HPP_
#include <stdlib.h>
#include <stdio.h>
#include <cassert>
#include "utils/log.h"

#if defined _MSC_VER
#ifndef WINDOWS
#define WINDOWS
#endif
#elif defined __GNUC__
#ifndef LINUX
#define LINUX
#endif
#endif

// @brief XSP_ASSERT
#ifdef XSP_DEBUG
#define XSP_ASSERT(flag) assert(flag)
#else
#define XSP_ASSERT(flag) ((void)0)
#endif

/** @brief 双参数操作展开
*
* 宏OPERATION(op, val1, val2)展开双参数比较操作
* example：
* @code
*    OPERATION(< , 10， 100); // 编译后为(10 < 100)
*    OPERATION(>=, 10， 100); // 编译后为(10 >= 100)
*/
#define OPERATION(op, val1, val2) (val1 op val2)


/** @brief 检查输入双参数是否符合条件, 不符合打印错误位置并退出当前进程
*
* 宏CHECK_OP_EXIT(op, val1, val2)检查(val1 op val2)的值，若为0不符合条件，打印当前代码
* 位置并结束进程，若为1符合条件，继续向下运行程序；该宏在XSP_DEBUG下起作用，非XSP_DEBUG
* 下不再做检查判断；
*
* example：
* @code
* 	 CHECK_OP_EXIT(< , 10， 100); // 继续运行
*	 CHECK_OP_EXIT(>=, 10， 100); // 打印错误并退出
*/
#define CHECK_OP_EXIT(op, val1, val2) XSP_ASSERT(OPERATION(op, val1, val2))



/** @brief 检查输入双参数是否符合条件, 不符合打印错误位置并退出当前进程
*
* 宏CHECK_OP_RET(op, val1, val2, hr)检查(val1 op val2)的值，若为0不符合条件，
* 打印当前代码位置返回hr，若为1符合条件，继续向下运行程序；该宏在DEBUG和RELEASE
* 宏下都进行检查判断；
*
* example：
* @code
* 	 CHECK_OP_RET(< , 10， 100); // 继续运行
*	 CHECK_OP_RET(>=, 10， 100); // 打印错误并返回
*/
#define CHECK_OP_RET(op, val1, val2, hr)                          \
do {                                                              \
    if (!OPERATION(op, val1, val2))                               \
    {                                                             \
        log_error("Check (" #val1 " " #op " " #val2 ") error.");  \
        return hr;                                                \
    }                                                             \
} while (0)   



/** @brief 检查输入双参数是否符合条件, 不符合打印错误位置并退出当前进程
*
* 宏CHECK_EQ_EXIT(val1, val2)检查(val1==val2)的，若为0不符合条件，打印当前代码
* 位置并结束进程，若为1符合条件，继续向下运行程序，其余宏功能相似；该宏仅在DEBUG
* 下起作用，开启RELEASE宏后不再做检查判断；
*
* example：
* @code
* 	 CHECK_EQ_EXIT(10, 100); // 打印错误并退出
*	 CHECK_LE_EXIT(10, 100); // 继续运行
*/
#define CHECK_EQ_EXIT(val1, val2) CHECK_OP_EXIT(==, val1, val2)
#define CHECK_NE_EXIT(val1, val2) CHECK_OP_EXIT(!=, val1, val2)
#define CHECK_LE_EXIT(val1, val2) CHECK_OP_EXIT(<=, val1, val2)
#define CHECK_LT_EXIT(val1, val2) CHECK_OP_EXIT(< , val1, val2)
#define CHECK_GE_EXIT(val1, val2) CHECK_OP_EXIT(>=, val1, val2)
#define CHECK_GT_EXIT(val1, val2) CHECK_OP_EXIT(> , val1, val2)


/** @brief 检查输入双参数是否符合条件, 不符合打印错误位置并返回错误码
*
* 宏CHECK_EQ_RET(val1, val2, hr)检查(val1==val2)的，若为0不符合条件，打印
* 当前代码位置并返回错误码，若为1符合条件，继续向下运行程序，其余宏功能相似；
* 该宏在DEBUG和RELEASE宏下都做检查判断；
*
* example：
* @code
*	 CHECK_EQ_RET(10, 100, error_code); // 打印错误并返回错误码
*	 CHECK_LE_RET(10, 100, error_code); // 继续运行
*/
#define CHECK_EQ_RET(val1, val2, hr) CHECK_OP_RET(==, val1, val2, hr)
#define CHECK_NE_RET(val1, val2, hr) CHECK_OP_RET(!=, val1, val2, hr)
#define CHECK_LE_RET(val1, val2, hr) CHECK_OP_RET(<=, val1, val2, hr)
#define CHECK_LT_RET(val1, val2, hr) CHECK_OP_RET(< , val1, val2, hr)
#define CHECK_GE_RET(val1, val2, hr) CHECK_OP_RET(>=, val1, val2, hr)
#define CHECK_GT_RET(val1, val2, hr) CHECK_OP_RET(> , val1, val2, hr)


/** @brief 检查条件并返回设定返回值，打印相关信息
*
* 宏XRAY_CHECK(flag, hr, ...)检查flag的值，若为0不符合条件，return返回hr，
* 一般为错误码，并打印相关错误信息，若为1符合条件，继续向下运行程序；
*
* example：
* @code
*	 打印默认错误信息，打印自定义错误信息
*	 XRAY_CHECK(nHeight >= 0， XRAY_LIB_IMAGELENGTH_OVERFLOW, "nHeight : %d", nHeight);
*	 XRAY_CHECK(XRAY_LIB_OK == hr, hr, "FlatDetCali High failed");
*	 打印默认错误信息，不打印自定义错误信息
*	 XRAY_CHECK_RET(XRAY_LIB_OK == hr, hr);
*/
#if defined WINDOWS
#define XSP_CHECK(flag, hr, ...)                                    \
do {                                                                \
    if (!(flag))                                                    \
    {                                                               \
        log_error("Check (" #flag ") error. " ##__VA_ARGS__);       \
        return hr;                                                  \
    }                                                               \
} while (0)     
#elif defined LINUX
#define XSP_CHECK(flag, hr, ...)                                    \
do {                                                                \
    if (!(flag))                                                    \
    {                                                               \
        log_error("Check (" #flag ") error. " __VA_ARGS__);                  \
        return hr;                                                  \
    }                                                               \
} while (0)   
#endif


#endif // _XSP_CHECK_HPP_