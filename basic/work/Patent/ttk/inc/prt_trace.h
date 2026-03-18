
#ifndef _PRT_TRACE_H_
#define _PRT_TRACE_H_
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>

#define MODULE_NAME             "TTK"       // 打印在终端上用于显示标识的模块名
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1) : __FILE__)

/* _DEBUG宏是调试开关，可通过Makefile传入，传入方法是-D_DEBUG */
#ifdef _DEBUG
#define PRT_INFO(fmt, args...) \
    do {\
	    struct timespec tsnow = {0};\
        struct tm *pTm = NULL;\
        clock_gettime(CLOCK_MONOTONIC, &tsnow);\
        pTm = localtime((time_t *)&tsnow.tv_sec);\
        printf("[%04d-%02d-%02d %02d:%02d:%02d.%03ld] [" MODULE_NAME "] %s|%s|%d: " fmt,\
               (1900+pTm->tm_year), (1+pTm->tm_mon), pTm->tm_mday, pTm->tm_hour, pTm->tm_min, pTm->tm_sec, tsnow.tv_nsec / 1000000,\
               __FILENAME__, __FUNCTION__, __LINE__, ##args);\
    } while(0)

#else
#define PRT_INFO(fmt, args ... )
#endif

/* just printf */
#define PRT_PURE(fmt, args...)\
    do {\
        printf(fmt, ## args);\
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* _PRT_TRACE_H_ */

