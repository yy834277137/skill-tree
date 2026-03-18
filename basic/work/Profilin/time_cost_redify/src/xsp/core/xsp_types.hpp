#ifndef _XSP_TYPES_HPP_
#define _XSP_TYPES_HPP_

#include "libXRay_def.h"
#include "xsp_check.hpp"
#include "xsp_def.hpp"
#include "xmat.hpp"

/***************************************************************************************************
*                                        XSP通用结构体
***************************************************************************************************/
/**
* @brief 原始高低能数据结构体
*/
struct Xraw
{
	XMat low;  /* XSP_16UC1 */
	XMat high; /* XSP_16UC1 */
};

/**
* @brief 归一化高能、低能、原子序数结构体
*/
struct Calilhz
{
	XMat low;   /* XSP_16UC1 */
	XMat high;  /* XSP_16UC1 */
	XMat zData; /* XSP_8UC1 */
};

/**
* @brief 归一化高能、低能结构体
*/
struct Calilh
{
	XMat low;   /* XSP_16UC1 */
	XMat high;  /* XSP_16UC1 */
};

/**
* @brief 融合图像
*/
struct Merge
{
	XMat data;  /* XSP_16UC1 */
};

#endif // _XSP_TYPES_HPP_
