/***
 * @file   sal_time.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  时间相关功能
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

/* ========================================================================== */
/*                             头文件区                                       */
/* ========================================================================== */

#include "dspcommon.h"


/* ========================================================================== */
/*                           宏和类型定义区                                   */
/* ========================================================================== */


/* ========================================================================== */
/*                          数据结构定义区                                    */
/* ========================================================================== */


/* ========================================================================== */
/*                          函数定义区                                        */
/* ========================================================================== */

/**
 * @function   SAL_getTimeOfJiffies
 * @brief      获取系统tick，返回值换算为毫秒
 * @param[in]  void
 * @param[out] None
 * @return     UINT64 时间值，单位是毫秒
 */
UINT64 SAL_getTimeOfJiffies(void);

/**
 * @function   SAL_getCurMs
 * @brief      获取系统单调递增的时间，单位是毫秒
 * @param[in]  void
 * @param[out] None
 * @return     UINT32 时间值，单位是毫秒
 */
UINT32 SAL_getCurMs(void);

/**
 * @function   SAL_getCurUs
 * @brief      获取系统单调递增的时间，单位是微妙
 * @param[in]  void
 * @param[out] None
 * @return     UINT64 时间值，单位是微秒
 */
UINT64 SAL_getCurUs(void);

/**
 * @function   sal_get_tickcnt
 * @brief      获取系统单调递增的时钟数，以ms计
 * @param[in]  void
 * @param[out] None
 * @return     UINT64 时钟数，单位：毫秒
 */
UINT64 sal_get_tickcnt(void);

/**
 * @function   sal_tm_map_ms_2_timespec
 * @brief      时间映射，用于设置等待时间
 * @param[in]  IN const UINT64 wait_ms  等待时间
 * @param[out] OUT struct timespec *ts  返回时间
 * @param[out] None
 * @return     void
 */
void sal_tm_map_ms_2_timespec(IN const UINT64 wait_ms, OUT struct timespec *ts);

/**
 * @function   sal_msleep_by_nano
 * @brief      基于nanosleep的毫秒级封装，线程安全
 * @param[in]  UINT32 msec  毫秒
 * @param[out] None
 * @return     void
 */
void sal_msleep_by_nano(UINT32 msec);

/**
 * @function   SAL_msleep
 * @brief      毫秒的精确延时，线程安全，不占用CPU时间
 * @param[in]  INT32 msecTimeOut  超时时间，毫秒值
 * @param[out] None
 * @return     INT32 HIK_SOK  : Success
 *                   HIK_FAIL : Fail
 */
INT32 SAL_msleep(INT32 msecTimeOut);

/**
 * @function   SAL_mselect
 * @brief      select 超时，精确延时，超时时使用，线程安全
 * @param[in]  INT32 fd           设置需要超时操作的文件句柄
 * @param[in]  INT32 msecTimeOut  超时时间，毫秒值
 * @param[out] None
 * @return     INT32
 */
INT32 SAL_mselect(INT32 fd, INT32 msecTimeOut);

/**
 * @function   SAL_getTimeOfDayMs
 * @brief      获取当前时间，单位毫秒
 * @param[in]  VOID
 * @param[out] None
 * @return     UINT64 返回时间，单位毫秒
 */
UINT64 SAL_getTimeOfDayMs(VOID);

/**
 * @function   SAL_getTimeOfDay
 * @brief      获取当前时间，返回tv由秒和微妙组成
 * @param[in]  struct timeval *tv  时间表述结构，由秒和微妙组成
 * @param[out] None
 * @return     INT32
 */
INT32 SAL_getTimeOfDay(struct timeval *tv);

/**
 * @function   SAL_getCurrentTime
 * @brief      获取当前时间，以年月日分秒格式输出
 * @param[in]  CHAR *current  返回时间信息字符串
 * @param[out] None
 * @return     INT32
 */
INT32 SAL_getCurrentTime(CHAR *current);

/**
 * @function:   SAL_getDateTime_tz
 * @brief:      获取当前时间(带时区timeZone)
 * @param[in]:  DATE_TIME *pDT
 * @param[out]: None
 * @return:     void
 */
void SAL_getDateTime_tz(DATE_TIME *pDT);

/**
 * @function   SAL_getDateTime
 * @brief      获取当前系统时间，使用DATE_TIME返回
 * @param[in]  DATE_TIME *pDT  返回的时间信息
 * @param[out] None
 * @return     void
 */
void SAL_getDateTime(DATE_TIME *pDT);

/**
 * @function:   SAL_getTimepFromCalender
 * @brief:      日历时间转换成UTC描述
 * @param[in]:  struct tm *tm  
 * @param[out]: None
 * @return:     time_t
 */
time_t SAL_getTimepFromCalender(struct tm *tm);

/**
 * @function:   SAL_setDSTDateTime
 * @brief:      设置夏令时参数
 * @param[in]:  DATE_TIME *pDT
 * @param[out]: None
 * @return:     void
 */
INT32 SAL_setDSTDateTime(OSD_DST_PRAM_S *pDST);

/**
 * @function:   SAL_getDateTime_DST
 * @brief:      获取当前时间(夏令时时间)
 * @param[in]:  DATE_TIME *pDT
 * @param[out]: None
 * @return:     void
 */
void SAL_getDateTime_DST(DATE_TIME *pDT);

