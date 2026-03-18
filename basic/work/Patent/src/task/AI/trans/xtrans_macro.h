
#ifndef _XTRANS_MACRO_H_
#define _XTRANS_MACRO_H_


#include"xtrans_sys.h"
#include "HPR_Config.h"
#include "HPR_Types.h"


#define PURPLE        "\033[0;35m"     //紫色
#define BROWN         "\033[0;33m"    //灰色
#define YELLOW        "\033[1;33m"    //黄色
#define GREEN         "\033[0;32m"      /* 绿色 */
#define RED           "\033[0;31m"      /* 红色 */
#define NONE          "\033[m"          /* 无色 */



#define xtrans_assert(x)  \
	    if((x) == 0) 	   \
	    	{ 			   \
	    	\
	    	printf(RED "[%s #%d][@%s][assert]" NONE,__FILE__,__LINE__,__FUNCTION__); \
	    	while(1);     \
	    } 
		
#ifndef xtrans_ReturnValIfFail
#define xtrans_ReturnValIfFail(p,val) if(!(p)) \
	{	\
		XTRANS_ERR(" \n");		\
		return val;			\
	}
#endif	
#define xtrans_assertSuccess(status)	\
    xtrans_assert(HPR_OK == (status))
#define xtrans_assertFail(status)  \
    xtrans_assert(HPR_OK != (status))

#define xtrans_assertNotNull(ptr)  \
    xtrans_assert(NULL != (ptr))
#define xtrans_assertNull(ptr)  \
    xtrans_assert(NULL == (ptr))

#define xtrans_assertSuccessRl(status)  \
    xtrans_assert(HPR_OK == (status))
#define xtrans_assertFailRl(status)  \
    xtrans_assert(HPR_OK != (status))

#define xtrans_assertNotNullRl(ptr)  \
    xtrans_assert(NULL != (ptr))
#define xtrans_assertNullRl(ptr)  \
    xtrans_assert(NULL == (ptr)) 

/* 判断返回值 */
#define xtrans_isSuccess(status)  (HPR_OK == (status))
#define xtrans_isFail(status)     (HPR_OK != (status))
    
#define xtrans_isTrue(status)     ((status))
#define xtrans_isFalse(status)    (!(status))   
    
#define xtrans_isNull(ptr)        (NULL == (ptr))
#define xtrans_isNotNull(ptr)     (NULL != (ptr))

#define xtrans_TIMEOUT_NONE         (0)    /* 不等待，立即返回。*/ 
#define xtrans_TIMEOUT_FOREVER      (~0U)  /* 一直等待直到返回 */


/* 数据存储度量单位 */
#define xtrans_KB (1024)
#define xtrans_MB (xtrans_KB * xtrans_KB)
#define xtrans_GB (xtrans_KB * xtrans_MB)
/* 
 * 设置版本号用的宏。高8位代表Major Version，中8位代表Minor Version，
 * 低16代表Revision。
 */
#define xtrans_versionSet(major, minor, revision)  \
        ((((major) & 0x0ff) << 24U) | (((minor) & 0x0ff) << 16U) \
           | ((revision) & 0x0ffff))


/* 数值比较*/
#define xtrans_max(a, b)    ( (a) > (b) ? (a) : (b) )
#define xtrans_min(a, b)    ( (a) < (b) ? (a) : (b) )


/* 内存操作*/
#define xtrans_clear(ptr)		        memset((ptr), 0, sizeof(*(ptr)))	
#define xtrans_clearSize(ptr, size)        memset((ptr), 0, (size))
#define xtrans_memCmp(dst, src)            memcmp((dst), (src), sizeof(*(src)))
#define xtrans_memCmpSize(dst, src, size)  memcmp((dst), (src), (size))


/* 对齐操作*/
#define xtrans_align(value, align)   ((( (value) + ( (align) - 1 ) ) \
                                     / (align) ) * (align) )
#define xtrans_ceil(value, align)    xtrans_align(value, align) 
#define xtrans_floor(value, align)   (( (value) / (align) ) * (align) )


/* 设置真假 */
#define xtrans_setTrue(val)        ((val) = HPR_TRUE)
#define xtrans_setFalse(val)       ((val) = HPR_FALSE)      


/* 获取数组成员数量 */
#define xtrans_arraySize(array)    (sizeof(array)/sizeof((array)[0]))


/* 获取结构体成员的地址偏移量 */
#ifdef __compiler_offsetof
#define xtrans_offsetof(TYPE,MEMBER) __compiler_offsetof(TYPE,MEMBER)
#else
#define xtrans_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif


/* 通过结构体成员获取结构体首地址 */
#define xtrans_containerOf(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - xtrans_offsetof(type,member) );})


/* 检测value值是否是2的N次方 */
#define xtrans_checkIs2n(value)  ( ((value) == 0) ? HPR_FALSE  \
                               : ((value) & ((value) - 1))  \
                                  ? HPR_FALSE : HPR_TRUE )




#define XTRANS_ERR(fmt, args ...) \
		do \
		{ \
			printf(RED "[%s #%d][@%s][ERROR]" fmt NONE,__FILE__,__LINE__,__FUNCTION__, ## args);\
		} \
		while(0)
			
#define XTRANS_WARN(fmt, args ...) \
					do \
					{ \
						\
						printf(YELLOW "[%s #%d][@%s][tid:%d][WARN]" fmt NONE,__FILE__,__LINE__,__FUNCTION__,(HPR_INT32)syscall(SYS_gettid), ## args);\
					} \
					while(0)

#define XTRANS_INFO(fmt, args ...) \
		do \
		{ \
			\
			printf(GREEN "[%s #%d][@%s][tid:%d][INFO]" fmt NONE,__FILE__,__LINE__,__FUNCTION__,(HPR_INT32)syscall(SYS_gettid), ## args);\
		} \
		while(0)

#if 0
#define XTRANS_DEBUG(fmt, args ...) \
				do \
				{ \
					 \
						printf(PURPLE "[%s #%d][@%s][tid:%d][DEBUG]" fmt NONE,__FILE__,__LINE__,__FUNCTION__,(HPR_INT32)syscall(SYS_gettid), ## args);\
				} \
				while(0)
#else

#define XTRANS_DEBUG(fmt, args ...) 

#endif

#if 0
#define XTRANS_TIME(fmt, args ...) \
				do \
				{ \
					 \
						printf(BROWN "[#%d][@%s][tid:%d]" fmt NONE,__LINE__,__FUNCTION__,(HPR_INT32)syscall(SYS_gettid), ## args);\
				} \
				while(0)
#else

#define XTRANS_TIME(fmt, args ...) 

#endif

#define XTRANS_RETURN_IFFALSE(p) if (!(p))\
					{ \
						XTRANS_ERR("%s check fails, return HPR_ERROR\n", #p); \
						return HPR_ERROR; \
					}

#endif

