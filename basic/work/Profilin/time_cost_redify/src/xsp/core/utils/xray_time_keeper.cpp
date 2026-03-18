#include "utils/xray_time_keeper.h"

/**@fn      GetTimeNow
* @brief    获取当前时间
* @param1   time                    [out]     - 输出时间
* @return   void
* @note
*/
void GetTimeNow(TIME_STRUCT* time)
{
#ifdef WINDOWS
	*time = clock();
#elif defined (LINUX)
	gettimeofday(time, 0);
#endif
}

/**@fn      GetTimeSpanUs
* @brief    计算时间差
* @param1   time1                   [in]     - 输入时间1
* @param1   time2                   [in]     - 输入时间2
* @return   时间差(us)
* @note
*/
long GetTimeSpanUs(TIME_STRUCT* time1,
	TIME_STRUCT* time2)
{
#ifdef WINDOWS
	return (long)((*time2 - *time1) * 1000);
#elif defined (LINUX)

	return (time2->tv_sec - time1->tv_sec) * 1000000 + (time2->tv_usec - time1->tv_usec);
#endif
}