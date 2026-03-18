/** @file ICF_Log.h
 *  @note Hikvision Digital Technology Co., Ltd. All Rights Reserved
 *  @brief 日志模块头文件
 * 
 *  @author 曹贺磊
 *  @version 0.9.0
 *  @date 2019/09/10
 *  @note 创建
 */

#ifndef _ICF_LOG_H_
#define _ICF_LOG_H_


#ifdef __cplusplus
extern "C" {
#endif


// 定义日志输出级别
#define ICF_ERROR       (1)     // ERROR级别 用于输出错误信息
#define ICF_WARN        (1<<1)  // WARNING级别 用户输出报警但不是大问题
#define ICF_INFO        (1<<2)  // INFO级别 用于输出用户想要输出的信息
#define ICF_DEBUG       (1<<3)  // DEBUG级别 用于输出DEBUG信息
#define ICF_TRACE       (1<<4)  // TRACE级别 用于追溯执行信息
#define ICF_EFFECT      (1<<5)  // EFFECT级别 暂无使用
#define ICF_STATUS      (1<<6)  // STATUS 状态统计专用

// 用户可设置宏定义
#define LOG_FILE_MAX_LENGTH  (500000000)    // 日志文件大小上限(json设置的日志文件大小不可超过此值)


/***************************************************************************************************
* 功  能: 日志写入顶层函数
* 参  数: level                  -I                   等级索引
*         file                   -I                   发生错误代码对应文件
*         func                   -I                   发生错误代码对应函数名
*         line                   -I                   发生错误代码对应行
*         format                 -I                   可变参数
*         ...                    -I                   可变参数
* 返回值: 状态码
* 备  注：
***************************************************************************************************/
int C_LOG_write(void *pInitHandle, int level, const char *file, const char *func, int line, const char *format, ...);


// 用户调用接口
#define LOG_ERROR(pInitHandle, format, ...)    C_LOG_write(pInitHandle, ICF_ERROR, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define LOG_WARN(pInitHandle, format, ...)     C_LOG_write(pInitHandle, ICF_WARN, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define LOG_INFO(pInitHandle, format, ...)     C_LOG_write(pInitHandle, ICF_INFO, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define LOG_DEBUG(pInitHandle, format, ...)    C_LOG_write(pInitHandle, ICF_DEBUG, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define LOG_TRACE(pInitHandle, format, ...)    C_LOG_write(pInitHandle, ICF_TRACE, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define LOG_EFFECT(pInitHandle, format, ...)   C_LOG_write(pInitHandle, ICF_EFFECT, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define LOG_STATUS(pInitHandle, format, ...)   C_LOG_write(pInitHandle, ICF_STATUS, __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define LOG_ERROR_CONSOLE(format, ...)         printf("[error] %s, func: %s, line: %d : " format "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* _ICF_LOG_H_ */
