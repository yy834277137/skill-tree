
#ifndef _DSPTTK_SQLITE_H_
#define _DSPTTK_SQLITE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sal_type.h"

/* ============================ 宏/枚举 ============================ */
#define SQL_KEY_NAME_LEN_MAX	32		// key名最大长度
#define SQL_VAL_STR_LEN_MAX		64		// 表中单个值转成字符串后字符串的最大长度

typedef enum
{
	SQL_TYPE_INTEGER,
	SQL_TYPE_TEXT,
	SQL_TYPE_REAL,
	SQL_TYPE_BLOB,
	SQL_TYPE_NUMERIC,
	SQL_TYPE_DATETIME,
	SQL_TYPE_TOTAL,
}SQL_TYPE;

typedef void * DB_HANDLE;


/* ========================= 结构体/联合体 ========================== */
typedef struct
{
	CHAR name[SQL_KEY_NAME_LEN_MAX];
	SQL_TYPE type;
}SQL_KEY_PAIR;


/* =================== 函数申明，static && extern =================== */
/**
 * @fn      dspttk_db_init
 * @brief   获取数据库的句柄 
 * @warning 仅是获取，该句柄对应的数据库并未初始化 
 * 
 * @return  DB_HANDLE 数据库的句柄
 */
DB_HANDLE dspttk_db_init(void);

/**
 * @fn      dspttk_db_open_conn
 * @brief   连接数据库中的表，不存在则创建 
 * @warning 目前仅支持单库单表 
 * 
 * @param   [IN] db_handle 数据库句柄
 * @param   [IN] db_name 数据库表名
 * 
 * @return  SAL_STATUS SAL_SOK：连接成功，SAL_FAIL：连接失败
 */
SAL_STATUS dspttk_db_open_conn(DB_HANDLE db_handle, CHAR *db_name);

/**
 * @fn      dspttk_db_create_table
 * @brief   创建数据库表中的Key，目前仅支持单库单表
 * 
 * @param   [IN/OUT] db_handle 数据库句柄，若数据库表已存在，且Key不一致，则会创建新的数据库句柄
 * @param   [IN] table_name 数据库表名
 * @param   [IN] key_pair 数据库Key名与其类型的组合关系表，第一个为Primary Key
 * @param   [IN] key_num 组合关系表中条目数
 * 
 * @return  SAL_STATUS SAL_SOK：创建成功，SAL_FAIL：创建失败
 */
SAL_STATUS dspttk_db_create_table(DB_HANDLE *db_handle, const CHAR *table_name, SQL_KEY_PAIR key_pair[], const UINT32 key_num);

/**
 * @fn      dspttk_db_parse_value_row
 * @brief   将数据库的插入字符串、更新字符串、查询输出字符串解析成单个字段的字符串
 * 
 * @param   [IN] db_handle 数据库句柄
 * @param   [IN] table_name 数据库表名
 * @param   [IN] val_row 多字段组成的长字符串
 * @param   [OUT] val_single 单个字段的字符串，指针数组形式，对应的内存需外部申请
 * 
 * @return  SAL_STATUS SAL_SOK：解析成功，SAL_FAIL：解析失败
 */
SAL_STATUS dspttk_db_parse_value_row(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *val_row, CHAR *val_single[]);

/**
 * @fn      dspttk_db_insert
 * @brief   插入数据库
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * @param   [IN] table_name 数据库表名
 * @param   [IN] value_inst 按数据库表key的顺序组成的字符串，各字段中间使用逗号分隔，最后一个字段后也需要加逗号
 * @param   [IN] group_num 需要插入的条目数
 * 
 * @return  SAL_STATUS SAL_SOK：插入成功，SAL_FAIL：插入失败
 */
SAL_STATUS dspttk_db_insert(DB_HANDLE db_handle, const CHAR *table_name, CHAR *value_inst[], const UINT32 group_num);

/**
 * @fn      dspttk_db_update
 * @brief   更新数据库
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * @param   [IN] table_name 数据库表名
 * @param   [IN] value_update 按数据库表key的顺序组成的字符串，各字段中间使用逗号分隔，最后一个字段后也需要加逗号
 * 
 * @return  SAL_STATUS SAL_SOK：更新成功，SAL_FAIL：更新失败
 */
SAL_STATUS dspttk_db_update(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *value_update);

/**
 * @fn      dspttk_db_select
 * @brief   数据库查询
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * @param   [IN] table_name 查询的数据库表名，可以是实际的表名，也可以是select输出的中间结果，比如： 
 *                          (SELECT * FROM 表名 ORDER BY ID DESC LIMIT 10) tempTable 
 * @param   [IN] column 需要查询的字段（列）或信息（比如最大最小值等），可为NULL；当为NULL时，查询所有字段，即SELECT *
 * @param   [IN] condition 查询条件，比如WHERE、ORDER BY、LIKE等，可为NULL 
 * @param   [IN/OUT] results_seq 输入为字符串指针数组，输出为查询得到的结果，字符串格式，各字段中间用逗号分隔
 * @param   [IN/OUT] group_num 输入为字符串指针数组个数，输出为查询得到的结果数
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_db_select(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *column, const CHAR *condition, CHAR *results_seq[], UINT32 *group_num);

/**
 * @fn      dspttk_db_count
 * @brief   获取数据库中的记录条数
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * @param   [IN] table_name 查询的数据库表名
 * @param   [OUT] count 数据库中的记录条数
 * 
 * @return  SAL_STATUS SAL_SOK：获取成功，SAL_FAIL：获取失败
 */
SAL_STATUS dspttk_db_count(DB_HANDLE db_handle, const CHAR *table_name, UINT32 *count);

/**
 * @fn      dspttk_db_select_max
 * @brief   查询数据库中的某列数据的最大值
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * @param   [IN] table_name 查询的数据库表名
 * @param   [IN] column 数据库表列名
 * @param   [OUT] max_val 最大值，INTEGER输出为long，REAL输出为double，其他为字符串
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_db_select_max(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *column, void *max_val);

/**
 * @fn      dspttk_db_select_min
 * @brief   查询数据库中的某列数据的最小值
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * @param   [IN] table_name 查询的数据库表名
 * @param   [IN] column 数据库表列名
 * @param   [OUT] max_val 最小值，INTEGER输出为long，REAL输出为double，其他为字符串
 * 
 * @return  SAL_STATUS SAL_SOK：查询成功，SAL_FAIL：查询失败
 */
SAL_STATUS dspttk_db_select_min(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *column, void *min_val);

/**
 * @fn      dspttk_db_delete
 * @brief   删除数据库中的指定记录
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * @param   [IN] table_name 数据库表名
 * @param   [IN] condition 删除条件，比如WHERE、ORDER BY、LIKE等，可为NULL 
 * 
 * @return  SAL_STATUS SAL_SOK：删除成功，SAL_FAIL：删除失败
 */
SAL_STATUS dspttk_db_delete(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *condition);

/**
 * @fn      dspttk_db_close_conn
 * @brief   断开与数据库的连接
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * 
 * @return  SAL_STATUS SAL_SOK：断开成功，SAL_FAIL：断开失败
 */
SAL_STATUS dspttk_db_close_conn(DB_HANDLE db_handle);

/**
 * @fn      dspttk_db_close_conn
 * @brief   销毁数据库
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * 
 * @return  SAL_STATUS SAL_SOK：销毁成功，SAL_FAIL：销毁失败
 */
SAL_STATUS dspttk_db_destroy(DB_HANDLE db_handle);

/* =================== 全局变量，static && extern =================== */


#ifdef __cplusplus
}
#endif

#endif

