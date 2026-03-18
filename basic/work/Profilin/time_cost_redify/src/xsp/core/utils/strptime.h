#pragma once
/**    @file strptime.h
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*      @brief 日期字符串转tm结构体
*
*      @author huangtian8
*      @date 2024/1/30
*
*      @note
*      @note 历史记录
*      @note V0.0.1
*/
#ifndef STR_TIME_
#define STR_TIME_

char *strptime(const char *buf, const char *fmt, struct tm *tm);

#endif // STR_TIME_