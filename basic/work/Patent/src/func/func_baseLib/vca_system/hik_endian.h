/******************************************************************************
* 
* 版权信息: Copyright (c) 2009, 杭州海康威视数字技术股份有限公司
* 
* 文件名称: hik_endian.h
* 文件标识: HIKVISION_ENDIAN_H
* 摘    要: 海康威视大小端转换文件
*
* 当前版本: 1.0
* 作    者: 陈军
* 日    期: 2009年2月13日
* 备    注:
*           
*******************************************************************************
*/


#ifndef _HIK_ENDIAN_H_
#define _HIK_ENDIAN_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Conversion routines: */
#ifdef WORDS_BIGENDIAN
#undef IS_LITTLE_ENDIAN
#else
#define IS_LITTLE_ENDIAN
#endif

/* big endian (Motorola) -> host byte order */
#ifdef IS_LITTLE_ENDIAN
#define u32bh(i)        ((((i) & 0xff) << 24) | (((i) & 0xff00) << 8)\
| (((i) & 0xff0000) >> 8) | (((i) & 0xff000000) >> 24))

#define u16bh(i)        ((((i) & 0xff) << 8) | (((i) & 0xff00) >> 8))
#else
#define u32bh(i)        (i)
#define u16bh(i)        (i)
#endif

/* host byte order -> big endian */
#define u32hb(i)        u32bh(i)
#define u16hb(i)        u16bh(i)

/* little endian (Intel) -> host byte order */
#ifdef IS_LITTLE_ENDIAN
#define u64lh(i)        (i)
#define u32lh(i)        (i)
#define u16lh(i)        (i)
#else
#define u64lh(i)        ((i & 0xff) << 56) | ((i & 0xff00) << 40) | ((i & 0xff0000) << 24) \
	                  | ((i & 0xff000000) << 8) | ((i & 0xff00000000) >> 8) \
	                  | ((i & 0xff0000000000) >> 24) | ((i & 0xff000000000000) >> 40) \
                      | ((i & 0xff00000000000000) >> 56)

#define u32lh(i)        ((((i) & 0xff) << 24) | (((i) & 0xff00) << 8) \
                      | (((i) & 0xff0000) >> 8) | (((i) & 0xff000000) >> 24))

#define u16lh(i)        ((((i) & 0xff) << 8) | (((i) & 0xff00) >> 8))
#endif

/* host byte order -> little endian */
#define u64hl(i)        u64lh(i) 
#define u32hl(i)        u32lh(i)
#define u16hl(i)        u16lh(i)


#ifdef __cplusplus
}
#endif 

#endif

