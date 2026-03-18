/**    @file time_keeper.h
*	   @note HangZhou Hikvision System Technology Co., Ltd. All Right Reserved.
*      @brief 用于耗时计算
*
*      @author wangtianshu
*      @date 2022/11/23
*
*      @note
*      @note 历史记录
*      @note V0.0.1 
*/
#ifndef TIME_KEEPER_H_
#define TIME_KEEPER_H_

#if defined _MSC_VER
#ifndef WINDOWS
#define WINDOWS
#endif
#elif defined __GNUC__
#ifndef LINUX
#define LINUX
#endif
#endif

#ifdef  WINDOWS
#include <time.h>
typedef clock_t TIME_STRUCT;
#elif defined (LINUX)
#include <sys/time.h>
typedef struct timeval TIME_STRUCT;
#endif

/**@fn      GetTimeNow
* @brief    获取当前时间
* @param1   time                    [out]     - 输出时间
* @return   void
* @note
*/
void GetTimeNow(TIME_STRUCT* time);


/**@fn      GetTimeSpanUs
* @brief    计算时间差
* @param1   time1                   [in]     - 输入时间1
* @param1   time2                   [in]     - 输入时间2
* @return   时间差(us)
* @note
*/
long GetTimeSpanUs(TIME_STRUCT* time1, TIME_STRUCT* time2);


#endif // TIME_KEEPER_H_
