/**************************************************************************************************
* 
* 版权信息：版权所有 (c) 2010-2018, 杭州海康威视软件有限公司, 保留所有权利
*
* 文件名称：vca_types.h
* 文件标识：HIK_VCA_TYPES_H_
* 摘    要：海康威视VCA公共数据结构体声明文件

* 当前版本：1.0.0
* 作    者：车军
* 日    期：2020年12月29日
* 备    注：增加校验打印接口

* 当前版本：0.0.9
* 作    者：蔡巍伟
* 日    期：2012年04月06日
* 备    注：初始版本
**************************************************************************************************/

#ifndef _HIK_VCA_TYPES_H_
#define _HIK_VCA_TYPES_H_

#include "vca_base.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************************************
* 数据类型
**************************************************************************************************/
#define VCA_8U   0          //8位无符号数
#define VCA_8S   1          //8位有符号数
#define VCA_16U  2          //16位无符号数
#define VCA_16S  3          //16位有符号数
#define VCA_32U  4          //32位无符号数
#define VCA_32S  5          //32位有符号数
#define VCA_32F  6          //32位单精度浮点数
#define VCA_64F  7          //64位双精度浮点数
#define VCA_64U  8          //64位无符号数
#define VCA_64S  9          //64位有符号数

	//BASETYPES
#ifndef BASETYPES
#define BASETYPES
	typedef unsigned char    	BYTE; 
	typedef unsigned char*   	PBYTE; 
	typedef unsigned short   	WORD; 
	typedef unsigned short*  	PWORD;
	typedef unsigned int     	DWORD;    
	typedef unsigned int*    	PDWORD;    
	typedef unsigned long long	QWORD;
	typedef unsigned long long* PQWORD;
	//typedef unsigned int		BOOL;
#endif 

#ifndef UINT
//#define UINT unsigned int
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef FALSE
	//! Boolean Type
	typedef enum {
		FALSE,
		TRUE
	} Boolean;
#endif

/**************************************************************************************************
* 常用数学宏定义
**************************************************************************************************/
#define VCA_EPS                  0.0000001f
#define VCA_PI                   3.1415926f 
#define VCA_ABS(a)               (((a) > 0) ? (a) : -(a))
#define VCA_ROUND(a)             (int)((a) + (((a) >= 0) ? 0.5 : -0.5))

//大值，小值
#define VCA_MAX(a, b)            (((a) > (b)) ? (a) : (b))
#define VCA_MIN(a, b)            (((a) < (b)) ? (a) : (b))

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
#define VCA_CHECK_ERROR(state, error_code) \
if (state)                                 \
{                                          \
    return error_code;                     \
}

#define VCA_CHECK_RETURN(state) \
if (state)                      \
{                               \
    return;                     \
}

//循环跳转校验
#define VCA_CHECK_CONTINUE(state)      \
if (state)                             \
{                                      \
    continue;                          \
}        

// 打印文件名，函数名，行号，错误码
#define VCA_CHECK_ERROR_PRINT(state, error_code) \
if (state)                                 \
{                                          \
	printf("[ERROR]file=%s, func=%s, line=%d, err=0x%x\n", __FILE__, __FUNCTION__, __LINE__, error_code);  \
	return error_code;                     \
}

// 打印文件名，函数名，行号，错误码，并且打印附加信息
#define VCA_CHECK_ERROR_PRINT_V2(state, error_code, str) \
if (state)                                 \
{                                          \
	printf("[ERROR]file=%s, func=%s, line=%d, err=0x%x\n", __FILE__, __FUNCTION__, __LINE__, error_code);  \
	printf("[ERROR_INFO]%s\n", str);	   \
	return error_code;                     \
}

// 打印文件名，函数名，行号，错误码，并且不限制打印附加信息类型
#define VCA_CHECK_ERROR_PRINT_V3(state, error_code, fmt, ...) \
if (state)                                 \
{                                          \
	printf("[ERROR]file=%s, func=%s, line=%d, err=0x%x\n", __FILE__, __FUNCTION__, __LINE__, error_code);  \
	printf(fmt, ##__VA_ARGS__);			   \
	return error_code;                     \
}
/**************************************************************************************************
* 内存管理器MEM_TAB结构体定义
**************************************************************************************************/

//对齐
#define VCA_SIZE_ALIGN(size, align) (((size)+((align)-1)) & ((unsigned int)(-(align)))) 
#define VCA_SIZE_ALIGN_8(size)   VCA_SIZE_ALIGN(size,  8)
#define VCA_SIZE_ALIGN_16(size)  VCA_SIZE_ALIGN(size, 16)
#define VCA_SIZE_ALIGN_32(size)  VCA_SIZE_ALIGN(size, 32)
#define VCA_SIZE_ALIGN_64(size)  VCA_SIZE_ALIGN(size, 64)
#define VCA_SIZE_ALIGN_128(size) VCA_SIZE_ALIGN(size, 128)
#ifdef  WIN32
#define VCA_ALIGN_16           __declspec(align(16))
#define VCA_CACHE_LINE_SIZE    64
#else
#define VCA_ALIGN_16
#define VCA_CACHE_LINE_SIZE    128
#endif

/**************************************************************************************************
* 几何元素
**************************************************************************************************/

//像素坐标值->归一化坐标值
#define VCA_NORM_X(x, image_w) ((float)(x) / ((image_w) - 1))
#define VCA_NORM_Y(y, image_h) ((float)(y) / ((image_h) - 1))

//归一化坐标值->像素坐标值
#define VCA_PIX_X(n_x, image_w) ((float)(n_x) * ((image_w) - 1))
#define VCA_PIX_Y(n_y, image_h) ((float)(n_y) * ((image_h) - 1))

//像素坐标点->/归一化坐标点
#define VCA_NORM_PT(norm_pt, pix_pt, image_w, image_h)      \
{	                                                        \
    (norm_pt).x =  VCA_NORM_X((pix_pt).x, image_w);         \
    (norm_pt).y =  VCA_NORM_Y((pix_pt).y, image_h);         \
}

//归一化坐标点->像素坐标点
#define VCA_PIX_PT(norm_pt, pix_pt, image_w, image_h)            \
{                                                                \
    (pix_pt).x =  VCA_PIX_X((norm_pt).x, image_w);               \
	(pix_pt).y =  VCA_PIX_Y((norm_pt).y, image_h);               \
}   

//归一化矩形->像素矩形
#define VCA_PIX_RECT(n_rect, p_rect, image_w, image_h)                 \
{                                                                      \
	(p_rect).x      = VCA_PIX_X((n_rect).x, image_w);                  \
	(p_rect).y      = VCA_PIX_Y((n_rect).y, image_h);                  \
	(p_rect).width  = VCA_PIX_X((n_rect).width, image_w);              \
	(p_rect).height = VCA_PIX_Y((n_rect).height, image_h);             \
}

//像素矩形->归一化矩形
#define VCA_NORM_RECT(p_rect, n_rect, image_w, image_h)                 \
{                                                                       \
	(n_rect).x      = VCA_NORM_X((p_rect).x, image_w);                  \
	(n_rect).y      = VCA_NORM_Y((p_rect).y, image_h);                  \
	(n_rect).width  = VCA_NORM_X((p_rect).width, image_w);              \
	(n_rect).height = VCA_NORM_Y((p_rect).height, image_h);             \
}

//包围盒->矩形（整型）
#define VCA_BOX_2_RECT_I(box,rect_i)                                        \
{                                                                           \
    (rect_i).x = (box).left;                                                \
	(rect_i).y = (box).top;                                                 \
	(rect_i).width  = ((box).right  - (box).left + 1);                      \
	(rect_i).height = ((box).bottom - (box).top  + 1);                      \
}

//矩形（整型）-> 包围盒
#define VCA_RECT_I_2_box(rect_i,box)                                        \
{                                                                           \
	(box).left   = (rect_i).x;                                              \
	(box).top    = (rect_i).y;                                              \
	(box).right  = ((rect_i).x + (rect_i).width  - 1);                      \
	(box).bottom = ((rect_i).y + (rect_i).height - 1);                      \
}

//box中心点
#define VCA_BOX_CX(box)    (((box).left + (box).right) >> 1)
#define VCA_BOX_CY(box)    (((box).top  + (box).bottom) >> 1)

//BOX宽高
#define VCA_BOX_W(box)         ((box).right - (box).left + 1)
#define VCA_BOX_H(box)         ((box).bottom - (box).top + 1)
#define VCA_BOX_RW(box)        (((box).right - (box).left + 1) >> 1)
#define VCA_BOX_RH(box)        (((box).bottom - (box).top + 1) >> 1)



//大小结构体
typedef struct _VCA_SIZE_
{
	WORD width;
	WORD height;
}VCA_SIZE;
//blob结构体定义
typedef struct _VCA_BLOB_
{
	int    ID;           //ID	
	int    cx;           //中心点x坐标
	int    cy;           //中心点y坐标
	int    rw;           //宽度半径
	int    rh;           //高度半径
	int    reserved[3];  //保留字
}VCA_BLOB;

#define MAX_BLOB_NUM             100/*60*/       //最大Blob数量

//blob链表定义
typedef struct _VCA_BLOB_LIST_
{
	int          blob_num;
	VCA_BLOB     blob[MAX_BLOB_NUM];           //list中共可存60个BLOB
}VCA_BLOB_LIST;

#define MAX_TRACK_LEN            500      //最大轨迹长度 

//轨迹
typedef struct _VCA_TRACK_
{
	int          track_len;               //轨迹长度
	int          reserved[3];             //保留字
	VCA_POINT_I  point[MAX_TRACK_LEN];   //轨迹点的FIFO队列
}VCA_TRACK;

/**************************************************************************************************
* 矩阵
**************************************************************************************************/
typedef struct _VCA_MATRIX_
{
	int type;      //矩阵元素的数据类型
	int stride;    //行数据宽度（每行所占字节数）
	int rows;      //行
	int cols;      //列

	//数据指针联合结构体
	union
	{
		unsigned char   *ptr;
		short           *s;
		int             *i;
		float           *fl;
		double          *db;
	}data;
}VCA_MATRIX;




#ifdef __cplusplus
}
#endif 

#endif /* _HIK_VCA_TYPES_H_ */

