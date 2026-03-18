/*******************************************************************************
* debug_time.h
*
* HangZhou Hikvision Digital Technology Co., Ltd. All Right Reserved.
*
* Author : 
* Version: 
*
* Description : 通用耗时统计模块
*        示例 :
*                                             TimePoint0 TimePoint1 TimePoint2 .......
*                  ingexTag attrTag1 attrTag2  strTag0    strTag1    strTag2   .......
*         DItem0       0      640     1920       xxx        xxx        xxx 
*         DItem1       1      640      200       xxx        xxx        xxx
*         DItem2       2      640      32        xxx        xxx        xxx
*         DItem3       3      640      32        xxx        xxx        xxx
*         DItem4       4      640      32        xxx        xxx        xxx
*         DItem5       5      640      32        xxx        xxx        xxx
*         DItem6       6      640      32        xxx        xxx        xxx
*         DItem7       7      640      32        xxx        xxx        xxx
*         DItem8       8      640      32        xxx        xxx        xxx
*         DItem9       9      640      32        xxx        xxx        xxx
* Modification:
*******************************************************************************/
#ifndef _DEBUG_TIME_H_
#define _DEBUG_TIME_H_

/* ========================================================================== */
/*                               Include Files                                */
/* ========================================================================== */
#include "sal.h"

/* ========================================================================== */
/*                             Macros & Typedefs                              */
/* ========================================================================== */
typedef struct debug_time* debug_time;


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
debug_time dtime_create(UINT32 u32MaxItemNum);

/**
 * @function   dtime_destroy
 * @brief      删除一个耗时调试对象
 * @param[IN]  debug_time pDebugTime 耗时调试对象
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_destroy(debug_time pDebugTime);

/**
 * @function   dtime_clear
 * @brief      清空耗时调试对象已记录时间信息
 * @param[IN]  debug_time pDebugTime 耗时调试对象
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_clear(debug_time pDebugTime);

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
SAL_STATUS dtime_update_tag(debug_time pDebugTime, UINT32 u32ItemIdx, UINT64 u64IndexTag, UINT32 u32AttrTag1, UINT32 u32AttrTag2);

/**
 * @function   dtime_clear_item
 * @brief      清空该条目的记录信息
 * @param[IN]  debug_time pDebugTime    耗时调试对象
 * @param[IN]  UINT32     u32ItemIdx    条目索引
 * @param[OUT] NONE
 * @return     INT32 增加的条目索引
 */
INT32 dtime_clear_item(debug_time pDebugTime, UINT32 u32ItemIdx);

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
INT32 dtime_add_item(debug_time pDebugTime, UINT64 u64IndexTag, UINT32 u32AttrTag1, UINT32 u32AttrTag2);

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
SAL_STATUS dtime_add_time_point(debug_time pDebugTime, UINT32 u32ItemIdx, BOOL bAutoSort, CHAR *sStringTag, UINT64 u64Value, BOOL bIfTimePoint);

/**
 * @function   dtime_get_time_point
 * @brief      通过条目索引和时间节点索引获取耗时调试对象存储的对应时间值
 * @param[IN]  debug_time pDebugTime        耗时调试对象
 * @param[IN]  UINT32     u32ItemIdx        添加的条目索引
 * @param[IN]  CHAR       *sStringTag       对应时间节点的字符串标签
 * @param[OUT] NONE
 * @return     UINT64 时间值
 */
UINT64 dtime_get_time_point(debug_time pDebugTime, UINT32 u32ItemIdx, CHAR *sStringTag);

/**
 * @function   dtime_print_time_point
 * @brief      打印当前耗时调试对象存储的时间点值
 * @param[IN]  debug_time pDebugTime        耗时调试对象
 * @param[IN]  UINT8      u32PrintItemNum   打印的最新记录条目个数，不能耗时调试对象支持的最大条目个数，
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_print_time_point(debug_time pDebugTime, UINT32 u32PrintItemNum);

/**
 * @function   dtime_print_duration
 * @brief      打印当前耗时调试对象存储的前后时间节点之间的时间差
 * @param[IN]  debug_time pDebugTime        耗时调试对象
 * @param[IN]  UINT8      u32PrintItemNum   打印的最新记录条目个数，不能耗时调试对象支持的最大条目个数，
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_print_duration(debug_time pDebugTime, UINT32 u32PrintItemNum);

/**
 * @function   dtime_print_interval
 * @brief      打印当前耗时调试对象存储的相邻条目之间各个时间节点的时间间隔
 * @param[IN]  debug_time pDebugTime        耗时调试对象
 * @param[IN]  UINT8      u32PrintItemNum   打印的最新记录条目个数，不能耗时调试对象支持的最大条目个数，
 * @param[OUT] NONE
 * @return     SAL_STATUS 成功-SAL_SOK, 失败-SAL_FAIL
 */
SAL_STATUS dtime_print_interval(debug_time pDebugTime, UINT32 u32PrintItemNum);

#endif