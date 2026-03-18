/**
 * @file   debug_time.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  通用耗时统计模块
 * @author 
 * @date   
 * @note :
 * @note \n History:
 */

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */

#include "debug_time.h"

#line __LINE__ "debug_time.c"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
#define CHECK_PTR_IS_NULL(ptr, errno) \
    do { \
        if (!ptr) \
        { \
            SAL_LOGE("the '%s' is NULL\n", # ptr); \
            return (errno); \
        } \
    } while (0)

#define MAX_TIME_POINT_NUM  (16)
#define MAX_STRING_TAG_LEN  (10)

/* 时间记录条目结构体 */
typedef struct debug_time_list
{
    UINT64 u64IndexTag;         // 时间记录条目的Index标签属性
    UINT32 u32AttrTag1;         // 时间记录条目的通用标签属性
    UINT32 u32AttrTag2;         // 时间记录条目的通用标签属性
    UINT32 u32CurTimePointIdx;
    UINT64 au64TimePoint[MAX_TIME_POINT_NUM];   // 存储的时间点数值
    BOOL aIfTimePoint[MAX_TIME_POINT_NUM];      // 是否为时间点，或者为属性
} DEBUG_TIMEITEM_ST;

/* 耗时调试对象结构体 */
typedef struct debug_time
{
    UINT32 u32DItemIdx;                 // 当前耗时调试对象最新的条目索引值
    UINT32 u32MaxItemNum;               // 耗时调试对象所能记录最大的条目个数
    UINT16 u16MaxTimePointIdx;          // 耗时调试对象最大的时间节点数
    CHAR aStringTag[MAX_TIME_POINT_NUM][MAX_STRING_TAG_LEN];    // 耗时调试对象每个调试节点对应的字符串标签
    DEBUG_TIMEITEM_ST *pstDItemList;    // 耗时调试模块的时间记录条目列表
} DEBUG_TIME_ST;

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */



/* ========================================================================== */
/*                           Function Declarations                            */
/* ========================================================================== */
/**
 * @function   dtime_create
 * @brief      创建一个耗时调试对象
 * @param[IN]  UINT32 u32MaxItemNum 记录的最大条目项数
 * @param[OUT] NONE
 * @return     debug_time 耗时调试对象
 */
debug_time dtime_create(UINT32 u32MaxItemNum)
{
    debug_time pstDbgTime = (debug_time)malloc(sizeof(DEBUG_TIME_ST));

    pstDbgTime->pstDItemList = (DEBUG_TIMEITEM_ST *)malloc(u32MaxItemNum * sizeof(DEBUG_TIMEITEM_ST));
    pstDbgTime->u32DItemIdx = 0;
    pstDbgTime->u32MaxItemNum = u32MaxItemNum;
    pstDbgTime->u16MaxTimePointIdx = 0;
    memset(pstDbgTime->pstDItemList, 0, u32MaxItemNum * sizeof(DEBUG_TIMEITEM_ST));
    memset(pstDbgTime->aStringTag, 0, sizeof(pstDbgTime->aStringTag));

    return pstDbgTime;
}

/**
 * @function   dtime_destroy
 * @brief      删除一个耗时调试对象
 * @param[IN]  debug_time pDebugTime 耗时调试对象
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_destroy(debug_time pDebugTime)
{
    if (NULL != pDebugTime)
    {
        if (NULL != pDebugTime->pstDItemList)
        {
            free(pDebugTime->pstDItemList);
        }
        free(pDebugTime);
        pDebugTime = NULL;
    }

    return SAL_SOK;
}

/**
 * @function   dtime_clear
 * @brief      清空耗时调试对象已记录时间信息
 * @param[IN]  debug_time pDebugTime 耗时调试对象
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_clear(debug_time pDebugTime)
{
    CHECK_PTR_IS_NULL(pDebugTime, SAL_FAIL);

    pDebugTime->u32DItemIdx = 0;
    pDebugTime->u16MaxTimePointIdx = 0;
    memset(pDebugTime->aStringTag, 0, sizeof(pDebugTime->aStringTag));
    memset(pDebugTime->pstDItemList, 0, pDebugTime->u32MaxItemNum * sizeof(DEBUG_TIMEITEM_ST));

    return SAL_SOK;
}

/**
 * @function   dtime_update_tag
 * @brief      更新耗时调试对象的条目标签信息
 * @param[IN]  debug_time pDebugTime  耗时调试对象
 * @param[IN]  UINT32     u32ItemIdx  更新的条目索引
 * @param[IN]  UINT64     u64IndexTag 更新的索引标签值
 * @param[IN]  UINT32     u32AttrTag1 更新的属性标签值
 * @param[IN]  UINT32     u32AttrTag2 更新的属性标签值
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_update_tag(debug_time pDebugTime, UINT32 u32ItemIdx, UINT64 u64IndexTag, UINT32 u32AttrTag1, UINT32 u32AttrTag2)
{
    DEBUG_TIMEITEM_ST *pstDItemList = NULL;

    CHECK_PTR_IS_NULL(pDebugTime, SAL_FAIL);
    if (u32ItemIdx >= pDebugTime->u32MaxItemNum)
    {
        SAL_LOGE("Current item idx[%d] exceeds max item number[%d]\n", u32ItemIdx, pDebugTime->u32MaxItemNum);
        return SAL_FAIL;
    }
    pstDItemList = pDebugTime->pstDItemList + u32ItemIdx;

    pstDItemList->u64IndexTag = u64IndexTag;
    pstDItemList->u32AttrTag1 = u32AttrTag1;
    pstDItemList->u32AttrTag2 = u32AttrTag2;

    return SAL_SOK;
}

/**
 * @function   dtime_clear_item
 * @brief      清空该条目的记录信息
 * @param[IN]  debug_time pDebugTime    耗时调试对象
 * @param[IN]  UINT32     u32ItemIdx    条目索引
 * @param[OUT] NONE
 * @return     INT32 增加的条目索引
 */
INT32 dtime_clear_item(debug_time pDebugTime, UINT32 u32ItemIdx)
{
    INT32 i = 0;
    DEBUG_TIMEITEM_ST *pstDItemList = NULL;

    CHECK_PTR_IS_NULL(pDebugTime, -1);
    if (u32ItemIdx > pDebugTime->u32MaxItemNum)
    {
        SAL_LOGE("Current item idx[%d] exceeds max item number[%d]\n", u32ItemIdx, pDebugTime->u32MaxItemNum);
        return SAL_FAIL;
    }

    pstDItemList = pDebugTime->pstDItemList + u32ItemIdx;
    pstDItemList->u32CurTimePointIdx = 0;
    memset(pstDItemList->au64TimePoint, 0, sizeof(pstDItemList->au64TimePoint));
    for (i = 0; i < MAX_TIME_POINT_NUM; i++)
    {
        pstDItemList->aIfTimePoint[i] = SAL_TRUE;
    }

    return SAL_SOK;
}

/**
 * @function   dtime_add_item
 * @brief      耗时调试对象添加记录条目
 * @param[IN]  debug_time pDebugTime  耗时调试对象
 * @param[IN]  UINT64     u64IndexTag 更新的索引标签值
 * @param[IN]  UINT32     u32AttrTag1 更新的属性标签值
 * @param[IN]  UINT32     u32AttrTag2 更新的属性标签值
 * @param[OUT] NONE
 * @return     INT32 增加的条目索引
 */
INT32 dtime_add_item(debug_time pDebugTime, UINT64 u64IndexTag, UINT32 u32AttrTag1, UINT32 u32AttrTag2)
{
    CHECK_PTR_IS_NULL(pDebugTime, -1);

    pDebugTime->u32DItemIdx = (pDebugTime->u32DItemIdx + pDebugTime->u32MaxItemNum + 1) % pDebugTime->u32MaxItemNum;

    dtime_clear_item(pDebugTime, pDebugTime->u32DItemIdx);
    dtime_update_tag(pDebugTime, pDebugTime->u32DItemIdx, u64IndexTag, u32AttrTag1, u32AttrTag2);

    return pDebugTime->u32DItemIdx;
}

/**
 * @function   dtime_add_time_point
 * @brief      向耗时调试对象的指定条目添加时间节点
 * @param[IN]  debug_time pDebugTime        耗时调试对象
 * @param[IN]  UINT32     u32ItemIdx        添加的条目索引
 * @param[IN]  BOOL       bAutoSort         是否自动排序，SAL_TRUE-根据添加顺序自动排序，SAL_FALSE-从现有节点中根据sStringTag查找对应时间节点
 * @param[IN]  CHAR       *sStringTag       对应时间节点的字符串标签
 * @param[IN]  UINT64     u64Value          使用的预设时间节点值，-1时表示由内部获取当前时间
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_add_time_point(debug_time pDebugTime, UINT32 u32ItemIdx, BOOL bAutoSort, CHAR *sStringTag, UINT64 u64Value, BOOL bIfTimePoint)
{
    UINT32 i= 0, u32TimePointIdx = -1;
    DEBUG_TIMEITEM_ST *pstDItemList = NULL;

    CHECK_PTR_IS_NULL(pDebugTime, SAL_FAIL);
    CHECK_PTR_IS_NULL(sStringTag, SAL_FAIL);

    if (u32ItemIdx >= pDebugTime->u32MaxItemNum)
    {
        SAL_LOGE("Current item idx[%d] exceeds max item number[%d], string tag:%s\n", u32ItemIdx, pDebugTime->u32MaxItemNum, NULL != sStringTag ? sStringTag : "null");
        return SAL_FAIL;
    }
    if (SAL_FALSE == bIfTimePoint && -1 == u64Value)
    {
        SAL_LOGW("pDTime:%p, ItemIdx:%u, must specify the u64Value(-1) value when adding non-timepoint value\n", 
                 pDebugTime, u32ItemIdx);
        return SAL_FAIL;
    }
    
    pstDItemList = pDebugTime->pstDItemList + u32ItemIdx;
    if (pstDItemList->u32CurTimePointIdx + 1 >= MAX_TIME_POINT_NUM)
    {
        SAL_LOGE("Current time point idx[%d] exceeds MAX_TIME_POINT_NUM[%d], string tag:%s\n", pstDItemList->u32CurTimePointIdx, MAX_TIME_POINT_NUM, NULL != sStringTag ? sStringTag : "null");
        return SAL_FAIL;
    }

    if (SAL_TRUE == bAutoSort)
    {
        u32TimePointIdx = pstDItemList->u32CurTimePointIdx;
    }
    else
    {
        if (0 == pDebugTime->u16MaxTimePointIdx)
        {
            SAL_LOGW("pDTime:%p, current dtime has not been auto sorted, can't add specified time point, stringTag:%s\n", pDebugTime, sStringTag);
            return SAL_FAIL;
        }
        for (i = 0; i <= pDebugTime->u16MaxTimePointIdx; i++)
        {
            if (0 == strcmp(pDebugTime->aStringTag[i], sStringTag))
            {
                u32TimePointIdx = i;
            }
        }
        if (-1 == u32TimePointIdx)
        {
            SAL_LOGE("pDTime:%p, can't find specified time point named \"%s\"\n", pDebugTime, sStringTag);
            return SAL_FAIL; 
        }
    }

    if (0 != strcmp(pDebugTime->aStringTag[u32TimePointIdx], sStringTag))
    {
        snprintf(pDebugTime->aStringTag[u32TimePointIdx], MAX_STRING_TAG_LEN, "%s", sStringTag);
    }

    if (SAL_TRUE == bIfTimePoint)
    {
        pstDItemList->aIfTimePoint[u32TimePointIdx] = SAL_TRUE;
        pstDItemList->au64TimePoint[u32TimePointIdx] = (-1 == u64Value) ? sal_get_tickcnt() : u64Value;
    }
    else
    {
        pstDItemList->aIfTimePoint[u32TimePointIdx] = SAL_FALSE;
        pstDItemList->au64TimePoint[u32TimePointIdx] = u64Value;
    }
    
    if (SAL_TRUE == bAutoSort)
    {
        pDebugTime->u16MaxTimePointIdx = SAL_MAX(pDebugTime->u16MaxTimePointIdx, u32TimePointIdx);
        pstDItemList->u32CurTimePointIdx++;
    }

    return SAL_SOK;
}

/**
 * @function   dtime_get_time_point
 * @brief      通过条目索引和时间节点索引获取耗时调试对象存储的对应时间值
 * @param[IN]  debug_time pDebugTime        耗时调试对象
 * @param[IN]  UINT32     u32ItemIdx        添加的条目索引
 * @param[IN]  CHAR       *sStringTag       对应时间节点的字符串标签
 * @param[OUT] NONE
 * @return     UINT64 时间值
 */
UINT64 dtime_get_time_point(debug_time pDebugTime, UINT32 u32ItemIdx, CHAR *sStringTag)
{
    UINT32 i = 0, u32TimePointIdx = -1;
    DEBUG_TIMEITEM_ST *pstDItemList = NULL;

    CHECK_PTR_IS_NULL(pDebugTime, -1);
    CHECK_PTR_IS_NULL(sStringTag, SAL_FAIL);

    if (u32ItemIdx >= pDebugTime->u32MaxItemNum)
    {
        SAL_LOGE("Current item idx[%d] exceeds max item number[%d]\n", u32ItemIdx, pDebugTime->u32MaxItemNum);
        return SAL_FAIL;
    }

    pstDItemList = pDebugTime->pstDItemList + u32ItemIdx;

    for (i = 0; i <= pDebugTime->u16MaxTimePointIdx; i++)
    {
        if (0 == strcmp(pDebugTime->aStringTag[i], sStringTag))
        {
            u32TimePointIdx = i;
        }
    }
    if (-1 == u32TimePointIdx)
    {
        SAL_LOGE("pDTime:%p, can't find specified time point named \"%s\"\n", pDebugTime, sStringTag);
        return SAL_FAIL; 
    }

    return pstDItemList->au64TimePoint[u32TimePointIdx];
}

/**
 * @function   dtime_print_time_point
 * @brief      打印当前耗时调试对象存储的时间点值
 * @param[IN]  debug_time pDebugTime        耗时调试对象
 * @param[IN]  UINT32     u32PrintItemNum   打印的最新记录条目个数，不能耗时调试对象支持的最大条目个数，
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_print_time_point(debug_time pDebugTime, UINT32 u32PrintItemNum)
{
    UINT32 u32ActPrintNum = 0;
    INT32 i = 0, j = 0;
    UINT32 u32StartIdx = 0;
    CHAR cTitle[MAX_STRING_TAG_LEN * (MAX_TIME_POINT_NUM + 5)] = {0};
    CHAR cContent[16 * (MAX_TIME_POINT_NUM + 5)] = {0};
    DEBUG_TIMEITEM_ST *pstDItemList = NULL;

    CHECK_PTR_IS_NULL(pDebugTime, SAL_FAIL);

    u32ActPrintNum = SAL_MIN(u32PrintItemNum, pDebugTime->u32MaxItemNum);
    u32StartIdx = (pDebugTime->u32DItemIdx + 1 + pDebugTime->u32MaxItemNum - u32ActPrintNum) % pDebugTime->u32MaxItemNum;

    /* 打印时间点标题 */
    pstDItemList = pDebugTime->pstDItemList + pDebugTime->u32DItemIdx;
    SAL_LOGW("\nmax time point:%d, start print item idx:%u, cur item idx:%d, print num:%u, max item num:%d\n", pDebugTime->u16MaxTimePointIdx, u32StartIdx, pDebugTime->u32DItemIdx, u32ActPrintNum, pDebugTime->u32MaxItemNum);
    snprintf(cTitle, sizeof(cTitle), "\n     index   arrtTag1   attrTag2");
    for (j = 0; j <= pDebugTime->u16MaxTimePointIdx; j++)
    {
        snprintf(cTitle + strlen(cTitle), sizeof(cTitle) - strlen(cTitle), " %10s", pDebugTime->aStringTag[j]);
    }
    SAL_print("%s\n", cTitle);

    for (i = u32StartIdx; i < (u32StartIdx + u32ActPrintNum); i++)
    {
        pstDItemList = pDebugTime->pstDItemList + (i % pDebugTime->u32MaxItemNum);

        /* 打印时间点数值 */
        memset(cContent, 0x0, sizeof(cContent));
        snprintf(cContent, sizeof(cContent), "%10u %10u %10u", (UINT32)pstDItemList->u64IndexTag, pstDItemList->u32AttrTag1, pstDItemList->u32AttrTag2);
        for (j = 0; j <= pDebugTime->u16MaxTimePointIdx; j++)
        {
            snprintf(cContent + strlen(cContent), sizeof(cContent) - strlen(cContent), " %10u", (UINT32)pstDItemList->au64TimePoint[j]);
        }
        SAL_print("%s\n", cContent);
    }

    SAL_print(">>> print end\n");

    return SAL_SOK;
}

/**
 * @function   dtime_print_duration
 * @brief      打印当前耗时调试对象存储的前后时间节点之间的时间差
 * @param[IN]  debug_time pDebugTime        耗时调试对象
 * @param[IN]  UINT32     u32PrintItemNum   打印的最新记录条目个数，不能耗时调试对象支持的最大条目个数，
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_print_duration(debug_time pDebugTime, UINT32 u32PrintItemNum)
{
    UINT32 u32ActPrintNum = 0;
    INT32 i = 0, j = 0;
    UINT32 u32StartIdx = 0;
    CHAR cTitle[MAX_STRING_TAG_LEN * (MAX_TIME_POINT_NUM + 5)] = {0};
    CHAR cContent[16 * (MAX_TIME_POINT_NUM + 5)] = {0};
    DEBUG_TIMEITEM_ST *pstDItemList = NULL;
    UINT64 au64TimeDuration[MAX_TIME_POINT_NUM] = {0};

    CHECK_PTR_IS_NULL(pDebugTime, SAL_FAIL);

    u32ActPrintNum = SAL_MIN(u32PrintItemNum, pDebugTime->u32MaxItemNum);
    u32StartIdx = (pDebugTime->u32DItemIdx + 1 + pDebugTime->u32MaxItemNum - u32ActPrintNum) % pDebugTime->u32MaxItemNum;

    /* 打印时间点标题 */
    pstDItemList = pDebugTime->pstDItemList + pDebugTime->u32DItemIdx;
    snprintf(cTitle, sizeof(cTitle), "\n     index   arrtTag1   attrTag2");
    SAL_LOGW("\nmax time point:%d, start print item idx:%u, cur item idx:%d, print num:%u, max item num:%d\n", pDebugTime->u16MaxTimePointIdx, u32StartIdx, pDebugTime->u32DItemIdx, u32ActPrintNum, pDebugTime->u32MaxItemNum);
    for (j = 0; j <= pDebugTime->u16MaxTimePointIdx; j++)
    {
        snprintf(cTitle + strlen(cTitle), sizeof(cTitle) - strlen(cTitle), " %10s", pDebugTime->aStringTag[j]);
    }
    SAL_print("%s\n", cTitle);

    /* 打印时间点数值 */
    for (i = u32StartIdx; i < (u32StartIdx + u32ActPrintNum); i++)
    {
        pstDItemList = pDebugTime->pstDItemList + (i % pDebugTime->u32MaxItemNum);

        memset(cContent, 0x0, sizeof(cContent));
        snprintf(cContent, sizeof(cContent), "%10u %10u %10u  ", (UINT32)pstDItemList->u64IndexTag, pstDItemList->u32AttrTag1, pstDItemList->u32AttrTag2);
        for (j = 0; j <= pDebugTime->u16MaxTimePointIdx - 1; j++)
        {
            if (SAL_TRUE == pstDItemList->aIfTimePoint[j] && SAL_TRUE == pstDItemList->aIfTimePoint[j + 1])
            {
                if (0 != pstDItemList->au64TimePoint[j] && 0 != pstDItemList->au64TimePoint[j + 1])
                {
                    au64TimeDuration[j] = SAL_SUB_SAFE(pstDItemList->au64TimePoint[j + 1], pstDItemList->au64TimePoint[j]);
                }
                snprintf(cContent + strlen(cContent), sizeof(cContent) - strlen(cContent), " %10u", (UINT32)au64TimeDuration[j]);
            }
            else
            {
                snprintf(cContent + strlen(cContent), sizeof(cContent) - strlen(cContent), "           ");
                snprintf(cContent + strlen(cContent), sizeof(cContent) - strlen(cContent), " %8u", (UINT32)pstDItemList->au64TimePoint[j + 1]);
            }
            au64TimeDuration[j] = 0;
        }
        SAL_print("%s\n", cContent);
    }
    SAL_print(">>> print end\n");

    return SAL_SOK;
}

/**
 * @function   dtime_print_interval
 * @brief      打印当前耗时调试对象存储的相邻条目之间各个时间节点的时间间隔
 * @param[IN]  debug_time pDebugTime        耗时调试对象
 * @param[IN]  UINT32     u32PrintItemNum   打印的最新记录条目个数，不能耗时调试对象支持的最大条目个数，
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_print_interval(debug_time pDebugTime, UINT32 u32PrintItemNum)
{
    UINT32 u32ActPrintNum = 0;
    INT32 i = 0, j = 0;
    UINT32 u32StartIdx = 0;
    CHAR cTitle[MAX_STRING_TAG_LEN * (MAX_TIME_POINT_NUM + 5)] = {0};
    CHAR cContent[16 * (MAX_TIME_POINT_NUM + 5)] = {0};
    DEBUG_TIMEITEM_ST *pstDItemList = NULL;
    DEBUG_TIMEITEM_ST *pstPrevDItemList = NULL;
    UINT64 au64TimeInterval[MAX_TIME_POINT_NUM] = {0};

    CHECK_PTR_IS_NULL(pDebugTime, SAL_FAIL);

    u32ActPrintNum = SAL_MIN(u32PrintItemNum, pDebugTime->u32MaxItemNum);
    u32StartIdx = (pDebugTime->u32DItemIdx + 1 + pDebugTime->u32MaxItemNum - u32ActPrintNum) % pDebugTime->u32MaxItemNum;

    /* 打印时间点标题 */
    pstDItemList = pDebugTime->pstDItemList + pDebugTime->u32DItemIdx;
    snprintf(cTitle, sizeof(cTitle), "\n     index   arrtTag1   attrTag2");
    SAL_LOGW("\nmax time point:%d, start print item idx:%u, cur item idx:%d, print num:%u, max item num:%d\n", pDebugTime->u16MaxTimePointIdx, u32StartIdx, pDebugTime->u32DItemIdx, u32ActPrintNum, pDebugTime->u32MaxItemNum);
    for (j = 0; j <= pDebugTime->u16MaxTimePointIdx; j++)
    {
        snprintf(cTitle + strlen(cTitle), sizeof(cTitle) - strlen(cTitle), " %10s", pDebugTime->aStringTag[j]);
    }
    SAL_print("%s\n", cTitle);

    /* 打印时间点数值 */
    for (i = u32StartIdx + 1; i < (u32StartIdx + u32ActPrintNum); i++)
    {
        pstPrevDItemList = pDebugTime->pstDItemList + ((i - 1) % pDebugTime->u32MaxItemNum);
        pstDItemList = pDebugTime->pstDItemList + (i % pDebugTime->u32MaxItemNum);

        memset(cContent, 0x0, sizeof(cContent));
        snprintf(cContent, sizeof(cContent), "%10u %10u %10u", (UINT32)pstDItemList->u64IndexTag, pstDItemList->u32AttrTag1, pstDItemList->u32AttrTag2);
        for (j = 0; j <= pDebugTime->u16MaxTimePointIdx; j++)
        {
            if (SAL_TRUE == pstPrevDItemList->aIfTimePoint[j])
            {
                if (0 != pstPrevDItemList->au64TimePoint[j] && 0 != pstDItemList->au64TimePoint[j])
                {
                    au64TimeInterval[j] = SAL_SUB_SAFE(pstDItemList->au64TimePoint[j], pstPrevDItemList->au64TimePoint[j]);
                }
                snprintf(cContent + strlen(cContent), sizeof(cContent) - strlen(cContent), " %10u", (UINT32)au64TimeInterval[j]);
            }
            else
            {
                snprintf(cContent + strlen(cContent), sizeof(cContent) - strlen(cContent), " %10u", (UINT32)pstDItemList->au64TimePoint[j]);
            }
            au64TimeInterval[j] = 0;
        }
        SAL_print("%s\n", cContent);
    }
    SAL_print(">>> print end\n");

    return SAL_SOK;
}
