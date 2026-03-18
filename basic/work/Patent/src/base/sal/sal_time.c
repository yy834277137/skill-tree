/***
 * @file   sal_time.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  时间相关功能
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

/* ========================================================================== */
/*                             头文件区                                           */
/* ========================================================================== */
#include <sys/times.h>
#include <sys/time.h>
#include "sal.h"
#include "dspcommon.h"

/* ========================================================================== */
/*                             宏及类型区                                          */
/* ========================================================================== */
/* 夏令时/冬令时 */
#define  SUMMER_TIME	0x11
#define  WINTER_TIME	0x22

/* ========================================================================== */
/*                             结构体定义区                                         */
/* ========================================================================== */
typedef struct
{
    UINT32 u32CurSec;
    UINT32 u32DSTStartSec;
    UINT32 u32DSTEndSec;
    OSD_DST_PRAM_S stDSTPrame;
} TIME_DST_INFO_S;

/* ========================================================================== */
/*                             全局变量区                                          */
/* ========================================================================== */
static UINT8 gu8DayOrderList[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static UINT8 gu8DSTFlag = 0;
static TIME_DST_INFO_S gstDSTInfo;

/* ========================================================================== */
/*                          函数定义区                                             */
/* ========================================================================== */

/**
 * @function   SAL_getTimeOfJiffies
 * @brief      获取系统tick，返回值换算为毫秒
 * @param[in]  void
 * @param[out] None
 * @return     UINT64 时间值，单位是毫秒
 */
UINT64 SAL_getTimeOfJiffies(void)
{
    struct tms tBuf;

    /* 当前tick计数，初始化为一个中间值，处理第一次溢出 */
    static UINT64 curtick = 0x7fffffff;

    /* 先备份一下，避免多线程访问出错 */
    UINT64 tmptick = curtick;

    /* times必须使用非空参数，以兼容更多的平台 */
    UINT64 tick = (UINT32)times(&tBuf);

    if (tick == (UINT32)-1)             /* 溢出，取errno的值 */
    {
        /* glibc在处理溢出时，返回的errno可能不准，引起误溢出 */
        /* tick = (UINT32)-errno; */
        tick = tmptick;                /* 使用上一次合理的值，风险更低 */
    }

    if ((UINT32)tmptick != (UINT32)tick) /* 低32位变化说明tick变化 */
    {
        while (tick < tmptick)          /* 溢出处理 */
        {
            tick += 0xffffffff;
            tick += 1;
        }

        /* 提前处理多线程引起的时间倒流的问题，提高效率 */
        if (curtick < tick)
        {
            curtick = tick;
        }
    }

    return curtick * (1000 / sysconf(_SC_CLK_TCK));
}

/**
 * @function   SAL_getCurMs
 * @brief      获取系统单调递增的时间，单位是毫秒
 * @param[in]  void
 * @param[out] None
 * @return     UINT32 时间值，单位是毫秒
 */
UINT32 SAL_getCurMs(void)
{
    unsigned int uiResult = 0;
    struct timespec ts = {0};

    clock_gettime(CLOCK_MONOTONIC, &ts);
    uiResult = ((ts.tv_sec * 1000) + ts.tv_nsec / 1000000);
    return uiResult;
}

/**
 * @function   SAL_getCurUs
 * @brief      获取系统单调递增的时间，单位是微妙
 * @param[in]  void
 * @param[out] None
 * @return     UINT64 时间值，单位是微秒
 */
UINT64 SAL_getCurUs(void)
{
    INT32 ret = 0;
    struct timespec ts = {0};

    ret = clock_gettime(CLOCK_MONOTONIC, &ts);
    if (ret < 0)
    {
        return 0;
    }

    return ((ts.tv_sec * 1000000) + ts.tv_nsec / 1000);
}

/**
 * @function   sal_get_tickcnt
 * @brief      获取系统单调递增的时钟数，以ms计
 * @param[in]  void
 * @param[out] None
 * @return     UINT64 时钟数，单位：毫秒
 */
UINT64 sal_get_tickcnt(void)
{
    int ret_val = 0;
    struct timespec tsnow = {0};

    ret_val = clock_gettime(CLOCK_MONOTONIC, &tsnow);
    if (0 == ret_val)
    {
        return (uint64_t)(tsnow.tv_sec) * 1000 + tsnow.tv_nsec / 1000000;
    }
    else
    {
        SAL_LOGE("clock_gettime failed, errno: %d, %s\n", errno, strerror(errno));
        return 0;
    }
}

/**
 * @function   sal_tm_map_ms_2_timespec
 * @brief      时间映射，用于设置等待时间
 * @param[in]  IN const UINT64 wait_ms  等待时间
 * @param[out] OUT struct timespec *ts  返回时间
 * @param[out] None
 * @return     void
 */
void sal_tm_map_ms_2_timespec(IN const UINT64 wait_ms, OUT struct timespec *ts)
{
    int ret_val = 0;
    long ns = 0, ms = wait_ms % 1000;

    if (NULL == ts)
    {
        SAL_LOGE("the 'ts' is NULL\n");
        return;
    }

    ret_val = clock_gettime(CLOCK_REALTIME, ts);
    if (0 == ret_val)
    {
        ns = ts->tv_nsec + ms * 1000000; /* ns MAX: 1,998,000,000 */
        ts->tv_sec += wait_ms / 1000 + ns / 1000000000;
        ts->tv_nsec = ns % 1000000000;
    }
    else
    {
        SAL_LOGE("clock_gettime failed, errno: %d, %s\n", errno, strerror(errno));
        ts->tv_sec = time(NULL) + 1;
        ts->tv_nsec = 0;
    }

    return;
}

/**
 * @function   sal_msleep_by_nano
 * @brief      基于nanosleep的毫秒级封装，线程安全
 * @param[in]  UINT32 msec  毫秒
 * @param[out] None
 * @return     void
 */
void sal_msleep_by_nano(UINT32 msec)
{
    struct timespec ts = {
        msec / 1000,
        (msec % 1000) * 1000000
    };

    while ((-1 == nanosleep(&ts, &ts)) && (EINTR == errno))
        ;

    return;
}

/**
 * @function   SAL_msleep
 * @brief      毫秒的精确延时，线程安全，不占用CPU时间
 * @param[in]  INT32 msecTimeOut  超时时间，毫秒值
 * @param[out] None
 * @return     INT32 HIK_SOK  : Success
 *                   HIK_FAIL : Fail
 */
INT32 SAL_msleep(INT32 msecTimeOut)
{
    INT32 s32Ret;
    INT32 cnt = 200;
    struct timeval tv;
    INT32 tv_sec = msecTimeOut / 1000;
    INT32 tv_usec = msecTimeOut % 1000;

    do
    {
        tv.tv_sec = tv_sec;
        tv.tv_usec = tv_usec * 1000;

        s32Ret = select(0, NULL, NULL, NULL, &tv);
        if ((s32Ret < 0) && (errno == 4))
        {
            if (cnt <= 0)
            {
                SAL_LOGE("select TimeOut err %d,errno %d\n", s32Ret, errno);
                return SAL_FAIL;
            }

            cnt--;
        }
        else
        {
            break;
        }
    }
    while (1);

    return SAL_SOK;
}

/**
 * @function   SAL_mselect
 * @brief      select 超时，精确延时，超时时使用，线程安全
 * @param[in]  INT32 fd           设置需要超时操作的文件句柄
 * @param[in]  INT32 msecTimeOut  超时时间，毫秒值
 * @param[out] None
 * @return     INT32
 */
INT32 SAL_mselect(INT32 fd, INT32 msecTimeOut)
{
    INT32 s32Ret;
    fd_set fdSet;
    struct timeval tv;
    INT32 cnt = 200;

    do
    {
        FD_ZERO(&fdSet);
        FD_SET(fd, &fdSet);

        tv.tv_sec = 0;
        tv.tv_usec = msecTimeOut * 1000;
        s32Ret = select(fd + 1, &fdSet, NULL, NULL, &tv);
        if ((s32Ret < 0) && (errno == 4))
        {
            if (cnt <= 0)
            {
                char *mesg = strerror(errno);

                SAL_LOGE("select err Mesg:%s\n", mesg);
            }

            cnt--;
        }
        else
        {
            break;
        }
    }
    while (1);

    return s32Ret;
}

/**
 * @function   SAL_getTimeOfDayMs
 * @brief      获取当前时间，单位毫秒
 * @param[in]  VOID
 * @param[out] None
 * @return     UINT64 返回时间，单位毫秒
 */
UINT64 SAL_getTimeOfDayMs(VOID)
{
    INT32 status;
    UINT64 ullMs = 0;
    struct timeval tv;

    status = gettimeofday(&tv, NULL);
    if (status < 0)
    {
        SAL_LOGE("select TimeOut err %d\n", status);
        return SAL_FAIL;
    }

    ullMs = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return ullMs;
}

/**
 * @function   SAL_getTimeOfDay
 * @brief      获取当前时间，返回tv由秒和微妙组成
 * @param[in]  struct timeval *tv  时间表述结构，由秒和微妙组成
 * @param[out] None
 * @return     INT32
 */
INT32 SAL_getTimeOfDay(struct timeval *tv)
{
    INT32 status;

    status = gettimeofday(tv, NULL);
    if (status < 0)
    {
        SAL_LOGE("select TimeOut err %d\n", status);
        return SAL_FAIL;
    }

    return SAL_SOK;
}

/**
 * @function   SAL_getCurrentTime
 * @brief      获取当前时间，以年月日分秒格式输出
 * @param[in]  CHAR *current  返回时间信息字符串
 * @param[out] None
 * @return     INT32
 */
INT32 SAL_getCurrentTime(CHAR *current)
{
    struct timeval tv;
    struct timespec time;
    struct tm nowTime;

    if (NULL == current)
    {
        SAL_LOGE("current is NULL!!\n");
        return SAL_FAIL;
    }

    gettimeofday(&tv, NULL);

    clock_gettime(CLOCK_REALTIME, &time);  /* 获取相对于1970到现在的秒数 */
    localtime_r(&time.tv_sec, &nowTime);

    sprintf(current, "%02d%02d%02d%02d%02d%02d%03d",
            nowTime.tm_year + 1900 - 2000,
            nowTime.tm_mon + 1,
            nowTime.tm_mday,
            nowTime.tm_hour,
            nowTime.tm_min,
            nowTime.tm_sec,
            (UINT32)(tv.tv_usec / 1000));

    return SAL_SOK;
}

/**
 * @function:   SAL_getDateTime_tz
 * @brief:      获取当前时间(带时区timeZone)
 * @param[in]:  DATE_TIME *pDT
 * @param[out]: None
 * @return:     void
 */
void SAL_getDateTime_tz(DATE_TIME *pDT)
{
    struct tm nowTime = {0};
    time_t tp;

    struct timeval tv;
    gettimeofday(&tv,NULL);

    if (pDT == NULL)
    {
        return;
    }

    /* 得到日历时间，从标准时间点到现在的秒数 */
    tp = time(NULL);

    /* 格式化日历时间 */
    localtime_r(&tp, &nowTime);

    pDT->year = nowTime.tm_year + 1900;
    pDT->month = nowTime.tm_mon + 1;
    pDT->day = nowTime.tm_mday;
    pDT->hour = nowTime.tm_hour;
    pDT->minute = nowTime.tm_min;
    pDT->second = nowTime.tm_sec;
    pDT->dayOfWeek = nowTime.tm_wday;
    pDT->milliSecond  = (UINT32)(tv.tv_usec / 1000);

    return;
}

/**
 * @function   SAL_getDateTime
 * @brief      获取当前系统时间，使用DATE_TIME返回
 * @param[in]  DATE_TIME *pDT  返回的时间信息
 * @param[out] None
 * @return     void
 */
void SAL_getDateTime(DATE_TIME *pDT)
{
    struct tm tmpSysTime;
    struct tm *p = &tmpSysTime;
    time_t tp;

    if (pDT == NULL)
    {
        return;
    }

    /* 得到日历时间，从标准时间点到现在的秒数 */
    tp = time(NULL);

    /* 格式化日历时间 */
    gmtime_r(&tp, p);

    pDT->year = p->tm_year + 1900;
    pDT->month = p->tm_mon + 1;
    pDT->day = p->tm_mday;
    pDT->hour = p->tm_hour;
    pDT->minute = p->tm_min;
    pDT->second = p->tm_sec;
    pDT->dayOfWeek = p->tm_wday;
    pDT->milliSecond = 0;

    return;
}

/**
 * @function:   SAL_getTimepFromCalender
 * @brief:      日历时间转换成UTC描述
 * @param[in]:  struct tm *tm  
 * @param[out]: None
 * @return:     time_t
 */
time_t SAL_getTimepFromCalender(struct tm *tm)
{
    if (tm == NULL)
    {
        SAL_ERROR("tm == null! \n");
        return 0;
    }

    return mktime(tm);
}

/**
 * @function:   SAL_isLeapYear
 * @brief:      计算当前是否是闰年,修改2月份的天数
 * @param[in]:  const UINT32 currSec
 * @param[out]: None
 * @return:     void
 */
static UINT32 SAL_isLeapYear(UINT32 u32Year)
{
    if ((u32Year % 400 == 0) || ((u32Year % 4 == 0) && (u32Year % 100 != 0)))
    {
        gu8DayOrderList[1] = 29;
        return SAL_TRUE;
    }
    else
    {
        gu8DayOrderList[1] = 28;
        return SAL_FALSE;
    }
}

/**
 * @function:   convertTime
 * @brief:      计算从1900年经过了多少秒
 * @param[in]:  const UINT32 currSec
 * @param[out]: None
 * @return:     void
 */
static unsigned long convertTime(unsigned int year, unsigned int mon,
                                 unsigned int day, unsigned int hour,
                                 unsigned int minute, unsigned int sec)
{

    if (0 >= (int) (mon -= 2))    /* 1..12 -> 11,12,1..10 */
    {
        mon += 12;              /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return ((((unsigned long)(year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day)
              + year * 365 - 719499
              ) * 24 + hour /* now have hours */
             ) * 60 + minute /* now have minutes */
            ) * 60 + sec;   /* finally seconds */
}

/**
 * @function:   SAL_RetDstBias
 * @brief:      夏令时的偏移秒数
 * @param[in]:  const UINT32 currSec
 * @param[out]: None
 * @return:     void
 */
static UINT32 SAL_RetDstBias(VOID)
{
    return gstDSTInfo.stDSTPrame.DSTBias * 60;
}

/**
 * @function:   SAL_changDSTTimeToSec
 * @brief:      把夏令时配置时间转换为秒
 * @param[in]:  const UINT32 currSec
 * @param[out]: None
 * @return:     void
 */
static UINT32 SAL_changDSTTimeToSec(UINT32 year, UINT32 month, UINT32 weekNo, UINT32 date, UINT32 hour)
{
    struct timespec tspec;
    struct tm *pSysTime, tmpSysTime;
    /* struct tm gmTime; */
    UINT32 DSTDay;

    pSysTime = &tmpSysTime;
    if (weekNo == 5)
    {
        tspec.tv_sec = convertTime(year, month, gu8DayOrderList[month - 1], 0, 0, 0);

        gmtime_r(&tspec.tv_sec, pSysTime);
        tspec.tv_sec = mktime(pSysTime);
        localtime_r(&tspec.tv_sec, pSysTime);

        if (date > pSysTime->tm_wday)
        {
            DSTDay = date - pSysTime->tm_wday - 7;
        }
        else
        {
            DSTDay = date - pSysTime->tm_wday;
        }

        tspec.tv_sec += (DSTDay * 24 + hour) * 3600;
    }
    else
    {
        tspec.tv_sec = convertTime(year, month, 1, 0, 0, 0);

        gmtime_r(&tspec.tv_sec, pSysTime);
        tspec.tv_sec = mktime(pSysTime);
        localtime_r(&tspec.tv_sec, pSysTime);

        if (date >= pSysTime->tm_wday)
        {
            weekNo -= 1;
        }

        tspec.tv_sec += ((7 * weekNo + (date - pSysTime->tm_wday)) * 24 + hour) * 3600;
    }

    return tspec.tv_sec;
}

/**
 * @function:   SAL_needCheckSummerTime
 * @brief:      是否开始检测有无处于夏令时时间内
 * @param[in]:  const UINT32 currSec
 * @param[out]: None
 * @return:     void
 */
static BOOL SAL_needCheckSummerTime(const UINT32 currSec)
{
    INT32 s32STimDiff = 0;   /* 距离夏令时开始的时间差 */
    INT32 s32ETimDiff = 0;   /* 距离夏令时结束的时间差 */
    time_t tmpCurrSec = (time_t)currSec;
    INT32 s32Diff = 0;

    if ((gstDSTInfo.u32DSTStartSec <= 0) || (gstDSTInfo.u32DSTEndSec <= 0))
    {
        return SAL_TRUE;
    }

    s32Diff = gstDSTInfo.u32CurSec - tmpCurrSec;
    gstDSTInfo.u32CurSec = tmpCurrSec;
    if ((SAL_ABS(s32Diff) > 10))
    {
        return SAL_TRUE;
    }

    s32STimDiff = tmpCurrSec - gstDSTInfo.u32DSTStartSec;
    s32ETimDiff = tmpCurrSec - gstDSTInfo.u32DSTEndSec;

    if ((SAL_ABS(s32STimDiff) <= 60) || (SAL_ABS(s32ETimDiff) <= 60))
    {
        return SAL_TRUE;
    }

    return SAL_FALSE;
}

/**
 * @function:   SAL_checkSummerTime
 * @brief:      检测有无处于夏令时时间内
 * @param[in]:  const UINT32 currSec
 * @param[out]: None
 * @return:     void
 */
static UINT32 SAL_checkSummerTime(const UINT32 currSec)
{
    BOOL bCheckFlg = 0;
    struct tm tmpSysTime;
    time_t startSec = 0, endSec = 0;
    time_t tmpCurrSec = (time_t)currSec; /* 需要查看此处是否可以不用做转换,直接入参就传入time_t结构体 */
    OSD_DST_PRAM_S *pstDSTPrm = &gstDSTInfo.stDSTPrame;

    if (NULL == pstDSTPrm)
    {
        SAL_ERROR("stDSTPrame == null! \n");
        return SAL_FAIL;
    }

    bCheckFlg = SAL_needCheckSummerTime(currSec);


    if (pstDSTPrm->uenableDST && bCheckFlg)
    {

        /* 重新计算夏令时的开始和结束时间 */
        gmtime_r(&tmpCurrSec, &tmpSysTime);
        SAL_isLeapYear(tmpSysTime.tm_year + 1900);

        startSec = SAL_changDSTTimeToSec(tmpSysTime.tm_year + 1900, pstDSTPrm->startpoint.mon, pstDSTPrm->startpoint.weekNo, \
                                         pstDSTPrm->startpoint.date, pstDSTPrm->startpoint.hour);

        endSec = SAL_changDSTTimeToSec(tmpSysTime.tm_year + 1900, pstDSTPrm->endpoint.mon, pstDSTPrm->endpoint.weekNo, \
                                       pstDSTPrm->endpoint.date, pstDSTPrm->endpoint.hour) - SAL_RetDstBias();

        if ((pstDSTPrm->endpoint.mon < pstDSTPrm->startpoint.mon) && (tmpCurrSec >= startSec))
        {
            endSec = SAL_changDSTTimeToSec(tmpSysTime.tm_year + 1900 + 1, pstDSTPrm->endpoint.mon, pstDSTPrm->endpoint.weekNo, \
                                           pstDSTPrm->endpoint.date, pstDSTPrm->endpoint.hour) - SAL_RetDstBias();
        }
        else if ((pstDSTPrm->endpoint.mon < pstDSTPrm->startpoint.mon) && (tmpCurrSec <= endSec))
        {
            startSec = SAL_changDSTTimeToSec(tmpSysTime.tm_year + 1900 - 1, pstDSTPrm->startpoint.mon, pstDSTPrm->startpoint.weekNo, \
                                             pstDSTPrm->startpoint.date, pstDSTPrm->startpoint.hour);
        }

        gstDSTInfo.u32DSTStartSec = startSec;
        gstDSTInfo.u32DSTEndSec = endSec;

        /*判断当前时间是否处于夏令时状态*/
        if ((tmpCurrSec >= startSec) && (tmpCurrSec <= endSec))
        {
            gu8DSTFlag = SUMMER_TIME;
            return 0;
        }
        else
        {
            gu8DSTFlag = WINTER_TIME;
            return 0;
        }
    }
    else
    {
        return 0;
    }
}

/**
 * @function:   SAL_setDSTDateTime
 * @brief:      设置夏令时参数
 * @param[in]:  DATE_TIME *pDT
 * @param[out]: None
 * @return:     void
 */
INT32 SAL_setDSTDateTime(OSD_DST_PRAM_S *pDST)
{
    if (NULL == pDST)
    {
        SAL_ERROR("DST time null\n");
        return SAL_FAIL;
    }

    SAL_clear(&gstDSTInfo);
    SAL_clear(&gu8DSTFlag);
    memcpy(&gstDSTInfo.stDSTPrame, pDST, sizeof(OSD_DST_PRAM_S));


    SAL_LOGI("DST time prame:DSTBias:%d uenableDST:%d startpoint(M%d-W%d-D%d-H%d) endpoint(M%d-W%d-D%d-H%d) \n", \
             gstDSTInfo.stDSTPrame.DSTBias, gstDSTInfo.stDSTPrame.uenableDST, \
             gstDSTInfo.stDSTPrame.startpoint.mon, gstDSTInfo.stDSTPrame.startpoint.weekNo, gstDSTInfo.stDSTPrame.startpoint.date, \
             gstDSTInfo.stDSTPrame.startpoint.hour, \
             gstDSTInfo.stDSTPrame.endpoint.mon, gstDSTInfo.stDSTPrame.endpoint.weekNo, gstDSTInfo.stDSTPrame.endpoint.date,
             gstDSTInfo.stDSTPrame.endpoint.hour);
    return SAL_SOK;
}

/**
 * @function:   SAL_getDateTime_DST
 * @brief:      获取当前时间(夏令时时间)
 * @param[in]:  DATE_TIME *pDT
 * @param[out]: None
 * @return:     void
 */
void SAL_getDateTime_DST(DATE_TIME *pDT)
{
    time_t tp;
    struct tm nowTime = {0};
    struct timeval tv;

    gettimeofday(&tv, NULL);

    if (pDT == NULL)
    {
        return;
    }

    /* 得到日历时间，从标准时间点到现在的秒数 */
    tp = time(NULL);

    if (gstDSTInfo.stDSTPrame.uenableDST)
    {
        SAL_checkSummerTime(tp);

        if (SUMMER_TIME == gu8DSTFlag)
        {
            tp += SAL_RetDstBias();
        }
    }

    localtime_r(&tp, &nowTime);

    pDT->year = nowTime.tm_year + 1900;
    pDT->month = nowTime.tm_mon + 1;
    pDT->day = nowTime.tm_mday;
    pDT->hour = nowTime.tm_hour;
    pDT->minute = nowTime.tm_min;
    pDT->second = nowTime.tm_sec;
    pDT->dayOfWeek = nowTime.tm_wday;
    pDT->milliSecond = (UINT32)(tv.tv_usec / 1000);

    return;
}

