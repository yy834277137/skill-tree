/******************************************************************************
* 
* 版权信息: Copyright (c) 2009, 杭州海康威视数字技术股份有限公司
* 
* 文件名称: type_def.h
* 文件标识: HIKVISION_TYPE_DEF_H
* 摘    要: 海康威视类型定义文件
*
* 当前版本: 1.0
* 作    者: 陈军
* 日    期: 2009年2月13日
* 备    注:
*           
*******************************************************************************
*/

#ifndef _TYPE_DEF_H_
#define _TYPE_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

// 数据类型定义
typedef unsigned int     DWORD;
#if 0
/* BANDWIDTH_64BIT 宏是系统位宽的开关，可通过Makeife传入，传入方法是-D BANDWIDTH_64BIT 。 */
#ifdef BANDWIDTH_64BIT
typedef unsigned long long  DWORD;   /* 64 位系统的内存地址描述 */
#else
typedef unsigned int        DWORD;   /* 32 位系统的内存地址描述 */
#endif
#endif
typedef unsigned short   WORD;

typedef unsigned char    BYTE;

typedef int              INT32_T;

typedef short            INT16_T;

typedef char             INT8_T;

typedef unsigned int     UINT32_T;

typedef unsigned short   UINT16_T;

typedef unsigned char    UINT8_T;

typedef unsigned char *  PBYTE;

typedef float FLOAT32;


#ifdef __cplusplus
}
#endif 

#endif