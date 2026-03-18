/**    @file log.h
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*      @brief 算法组日志打印cpp工具
*
*      @author wangtianshu
*      @date 2022/11/12
*
*      @note 算法组日志打印cpp工具, 支持6个日志等级，分别为 
*            log_trace log_debug log_info 
*            log_warn log_error log_fatal
*            reference : https://github.com/rxi/log.c
*            
*      @note 历史记录
*      @note V0.0.1 实现初版日志功能
*/
#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "utils/log_client.h"
#if defined _MSC_VER
#ifndef WINDOWS
#define WINDOWS
#endif
#elif defined __GNUC__
#ifndef LINUX
#define LINUX
#endif
#endif

#if defined WINDOWS
#define getfilename(x) ((strrchr((x), '\\') != nullptr) ? (strrchr((x), '\\') + 1) : (x))
#elif defined LINUX
#define getfilename(x) ((strrchr((x), '/') != nullptr) ? (strrchr((x), '/') + 1) : (x))
#else
#define getfilename(x) x
#endif


typedef enum _LOG_LEVEL_
{
	LOG_TRACE, LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR, LOG_FATAL
}LOG_LEVEL;

extern LogLevel g_LogSaveMode;	//BSP日志保存等级
extern LOG_LEVEL g_LogLevel;	//XSP日志打印等级
/** @brief 分级日志打印
*
* 宏log_trace(...)用于替代printf打印调试，打印trace等级日志，同步打印日志时间，
* 打印文件名及代码行数；其余宏功能类似
*
* example：
* @code
*    log_trace("Hello world!"); 
*	 //15:49:30 [ALG_TRACE] test.cpp:10: Hello world!
*	 log_debug("height = %d", 100);
*    //15:49:30 [ALG_DEBUG] test.cpp:11: height = 100
*/
/* 定义终端支持打印字体显示的颜色，格式控制: ESC[ 显示方式m; 前景色m; 背景色m; */
#define NONE	"\033[m"
#define RED		"\033[0;31m"            /* 红色 */
#define GREEN	"\033[0;32m"            /* 绿色 */
#define BROWN	"\033[0;33m"            /* 黄色 */
#define BLUE	"\033[0;34m"            /* 蓝色 */
#define PURPLE	"\033[0;35m"            /* 紫色 */
#define CYAN	"\033[0;36m"            /* 深绿 */
#define GRAY	"\033[1;30m"            /* 灰色，即高亮黑色 */
#define YELLOW	"\033[1;33m"            /* 高亮黄色 */
#define WHITE	"\033[1;37m"            /* 高亮白色 */

#if defined WINDOWS
#define log_trace(...) xlog::log_log(LOG_TRACE, getfilename(__FILE__), __LINE__, __VA_ARGS__)
#define log_debug(...) xlog::log_log(LOG_DEBUG, getfilename(__FILE__), __LINE__, __VA_ARGS__)
#define log_info(...)  xlog::log_log(LOG_INFO,  getfilename(__FILE__), __LINE__, __VA_ARGS__)
#define log_warn(...)  xlog::log_log(LOG_WARN,  getfilename(__FILE__), __LINE__, __VA_ARGS__)
#define log_error(...) xlog::log_log(LOG_ERROR, getfilename(__FILE__), __LINE__, __VA_ARGS__)
#define log_fatal(...) xlog::log_log(LOG_FATAL, getfilename(__FILE__), __LINE__, __VA_ARGS__)   
#elif defined LINUX
#define log_trace(fmt,...) \
    do \
    { \
		if (LOG_TRACE < g_LogLevel) { break; } \
		else \
        { \
			log_msg_write((char*)(LOG_MODULE_APP_NAME), g_LogSaveMode, NONE  "[ALG] TRACE|%s|%d: " fmt "\n", getfilename(__FILE__), __LINE__, ##__VA_ARGS__); \
		} \
	} while(0)
#define log_debug(fmt,...) \
    do \
    { \
		if (LOG_DEBUG < g_LogLevel) { break; } \
		else \
        { \
			log_msg_write((char*)(LOG_MODULE_APP_NAME), g_LogSaveMode, WHITE  "[ALG] DEBUG|%s|%d: " fmt "\033[m\n", getfilename(__FILE__), __LINE__, ##__VA_ARGS__); \
		}\
    } while(0)
#define log_info(fmt,...) \
    do \
    { \
		if (LOG_INFO < g_LogLevel) { break; } \
		else \
        { \
			log_msg_write((char*)(LOG_MODULE_APP_NAME), g_LogSaveMode, GREEN  "[ALG] INFO|%s|%d: " fmt "\033[m\n", getfilename(__FILE__), __LINE__, ##__VA_ARGS__); \
		}\
    } while(0)
#define log_warn(fmt,...) \
    do \
    { \
		if (LOG_WARN < g_LogLevel) { break; } \
		else \
        { \
			log_msg_write((char*)(LOG_MODULE_APP_NAME), g_LogSaveMode, YELLOW  "[ALG] WARN|%s|%d: " fmt "\033[m\n", getfilename(__FILE__), __LINE__, ##__VA_ARGS__); \
		}\
    } while(0)
#define log_error(fmt,...) \
    do \
    { \
		if (LOG_ERROR < g_LogLevel) { break; } \
		else \
        { \
			log_msg_write((char*)(LOG_MODULE_APP_NAME), g_LogSaveMode, RED  "[ALG] ERROR|%s|%d: " fmt "\033[m\n", getfilename(__FILE__), __LINE__, ##__VA_ARGS__); \
		}\
    } while(0)
#define log_fatal(fmt,...) \
    do \
    { \
		if (LOG_FATAL < g_LogLevel) { break; } \
		else \
        { \
			log_msg_write((char*)(LOG_MODULE_APP_NAME), g_LogSaveMode, PURPLE  "[ALG] FATAL|%s|%d: " fmt "\033[m\n", getfilename(__FILE__), __LINE__, ##__VA_ARGS__); \
		}\
    } while(0)
#endif


typedef struct {
	va_list ap;
	const char *fmt;
	const char *check_msg;
	const char *file;
	struct tm *time;
	void *udata;
	int line;
	int level;
} log_Event;


typedef void(*log_LogFn)(log_Event *ev);
typedef void(*log_LockFn)(bool lock, void *udata);

namespace xlog
{

/**@fn      log_set_level
* @brief    设置log等级
* @param1   level                   [in]     - log等级
* @return   void
* @note     默认LOG_TRACE，打印所有等级，
*           打印大于等于level的log
*/
void log_set_level(int level);


/**@fn      log_set_quiet
* @brief    设置log静默，不打印
* @param1   enable                  [in]     - true/开始静默
* @return   void
* @note     静默后，文件打印和回调打印仍会继续
*/
void log_set_quiet(bool enable);


/**@fn      log_add_fp
* @brief    添加打印至文件
* @param1   fp                      [in]     - 文件指针
* @param2   level                   [in]     - 输出到文件的打印等级设置
* @return   int 文件写入失败返回负数
* @note     打印大于等于level的log到文件
*/
int log_add_fp(FILE *fp, int level);


/**@fn      log_add_callback
* @brief    添加打印至回调
* @param1   fn                      [in]     - 打印log的回调函数
* @param2   udata                   [in]     - 回调输出的内存指针
* @param3   level                   [in]     - 输出到文件的打印等级设置
* @return   int
* @note     打印大于等于level的log到回调
*/
int log_add_callback(log_LogFn fn, void *udata, int level);


/**@fn      log_level_string
* @brief    获取设定LOG等级对应的字符串
* @param1   level                   [in]     - log等级
* @return   等级对应的log字符串
* @note
*/
const char* log_level_string(int level);


/**@fn      log_set_lock
* @brief    设置上锁
* @param1   fn                      [in]     - 上锁函数
* @param1   udata                   [in]     - 互斥量
* @return   void
* @note
*/
void log_set_lock(log_LockFn fn, void *udata);


void log_log(int level, const char *file, int line, const char *fmt, ...);

void log_check(int level, const char *file, int line, const char *fmt, ...);

void log_check(int level, const char *file, int line, const char *check_msg, const char *fmt, ...);

} // namespace xlog

#endif // LOG_H_
