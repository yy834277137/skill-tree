/*
 * CopyRight:   HangZhou Hikvision System Technology Co., Ltd. All  Right Reserved.
 * FileName:    HPR_Types.h
 * Desc:        define CC Types files.
 * Author:      schina
 * Date:        2009-03-02
 * Contact:     zhaoyiji@hikvision.com.cn
 * History:     Created By Zhaoyiji 2009-03-02
 * */

#ifndef __HPR_TYPES_H__
#define __HPR_TYPES_H__

    typedef /*signed*/ char     HPR_INT8;
    typedef unsigned char   HPR_UINT8;
    typedef /*signed*/ short    HPR_INT16;
    typedef unsigned short  HPR_UINT16;
    typedef /*signed*/ int      HPR_INT32;
    typedef unsigned int    HPR_UINT32;
    typedef void*           HPR_VOIDPTR;
    typedef /*signed*/ long     HPR_LONG;
    typedef unsigned long   HPR_ULONG;
	
	//add baoqingti
	typedef unsigned long      HPR_UINT32L;     /* 无符号32位长整形数类型 */   	
	typedef long			   HPR_INT32L;		/* 有符号32位长整形数类型 */ 
	typedef unsigned int	   Bool32;		/* 32位布尔类型 */
	typedef unsigned char	   Bool;		/* 通用布尔类型 */
	typedef size_t             Sizet;       /* size_t类型 */
	typedef void *             Ptr;         /* 指针类型 */

    #define HPR_VOID void

#if defined(_MSC_VER)
    typedef /*signed*/ __int64  HPR_INT64;
    typedef unsigned __int64 HPR_UINT64;
    typedef HANDLE          HPR_HANDLE;

#if (_MSC_VER >= 1310)
    #if defined _WIN32_WCE
        #include <crtdefs.h>
    #else
        #include <stddef.h>
    #endif
    typedef uintptr_t       HPR_UINT;
    typedef intptr_t        HPR_INT;
#endif

    #ifndef socklen_t
        typedef int socklen_t;
    #endif

    typedef int (CALLBACK *HPR_PROC)();

#elif defined(__GNUC__) || defined(__SYMBIAN32__)
    #if defined(__LP64__)
        typedef /*signed*/ long HPR_INT64;
        typedef unsigned long HPR_UINT64;
    #else
        typedef /*signed*/ long long HPR_INT64;
        typedef unsigned long long HPR_UINT64;
    #endif  //defined(__LP64__)
    typedef void*           HPR_HANDLE;

    #include <ctype.h>
    /*typedef uintptr_t HPR_UINT;
    typedef intptr_t HPR_INT;*/

    typedef void*   HPR_PROC;
#endif      //#if defined(_MSC_VER)

#define HPR_SUPPORT_INT64 1

#ifndef  HPR_BOOL
    #define HPR_BOOL HPR_INT32
    #define HPR_TRUE  1
    #define HPR_FALSE 0
#endif
#ifdef MEGAEYES_DSP_VER
    #define HPR_STATUS HPR_INT32
#endif

#define HPR_INVALID_HANDLE NULL


#endif      //#ifndef __HPR_TYPES_H__
