
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>

#include "prt_trace.h"
#include "sqlite3.h"
#include "dspttk_sqlite.h"

/* ============================ 宏/枚举 ============================ */
#define SQL_CMD_LEN_MAX		512		// SQL执行语句最大长度
#define KEY_TYPE_LEN_MAX	16		// key类型最大长度


/* ========================= 结构体/联合体 ========================== */
typedef struct
{
	sqlite3 *p_sql;
	UINT32 key_num;
	SQL_KEY_PAIR key_pair[]; // 动态数组
}DB_PARAM_INNER;

typedef struct
{
	UINT32 group_num;
	UINT32 group_idx;
	CHAR **results_seq;
}SELECT_OUTPUT;


/* =================== 函数申明，static && extern =================== */


/* =================== 全局变量，static && extern =================== */

/**
 * @fn      dspttk_db_init
 * @brief   获取数据库的句柄 
 * @warning 仅是获取，该句柄对应的数据库并未初始化 
 * 
 * @return  DB_HANDLE 数据库的句柄
 */
DB_HANDLE dspttk_db_init(void)
{
	DB_PARAM_INNER *p_db_param = NULL;

	p_db_param = (DB_PARAM_INNER *)malloc(sizeof(DB_PARAM_INNER));
	if (NULL == p_db_param)
	{
		PRT_INFO("malloc for DB_PARAM_INNER failed\n");
	}

	return p_db_param;
}


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
SAL_STATUS dspttk_db_open_conn(DB_HANDLE db_handle, CHAR *db_name)
{
	INT32 ret_val = 0;
	CHAR *szerrmsg = NULL;
	DB_PARAM_INNER *p_db_param = (DB_PARAM_INNER *)(db_handle);

	if (NULL == db_handle || NULL == db_name)
	{
		PRT_INFO("the input paramters are invalid, db_handle: %p, db_name: %p\n", \
				db_handle, db_name);
		return SAL_FAIL;
	}

	// 打开指定的数据库文件，如果不存在将创建一个同名的数据库文件
	ret_val = sqlite3_open(db_name, &p_db_param->p_sql);
	if (ret_val != SQLITE_OK)
	{
		PRT_INFO("sqlite3_open failed, ret_val: %d, errmsg: %s\n", ret_val, sqlite3_errmsg(p_db_param->p_sql));
		goto ERR_EIXT;
	}

	// 检查数据库的完整性
	ret_val = sqlite3_exec(p_db_param->p_sql, "PRAGMA integrity_check;", NULL, NULL, &szerrmsg);
	if (ret_val != SQLITE_OK || szerrmsg != NULL)
	{
		PRT_INFO("PRAGMA integrity_check failed: %d, %s\n", ret_val, szerrmsg);
		sqlite3_free(szerrmsg);
		goto ERR_EIXT;
	}

	// 配置数据库磁盘同步模式，PRAGMA synchronous = 0 | OFF | 1 | NORMAL | 2 | FULL
	ret_val = sqlite3_exec(p_db_param->p_sql, "PRAGMA synchronous = FULL;", NULL, NULL, &szerrmsg);
	if (ret_val != SQLITE_OK || szerrmsg != NULL)
	{
		PRT_INFO("PRAGMA synchronous failed: %d, %s\n", ret_val, szerrmsg);
		sqlite3_free(szerrmsg);
		goto ERR_EIXT;
	}

	// 配置数据库缓存大小，默认大小为2000页
	ret_val = sqlite3_exec(p_db_param->p_sql, "PRAGMA cache_size = 4096;", NULL, NULL, &szerrmsg);
	if (ret_val != SQLITE_OK || szerrmsg != NULL)
	{
		PRT_INFO("PRAGMA cache_size failed: %d, %s\n", ret_val, szerrmsg);
		sqlite3_free(szerrmsg);
		goto ERR_EIXT;
	}

	// 配置数据库内存模式，0-default，1-file，2-memory
	ret_val = sqlite3_exec(p_db_param->p_sql, "PRAGMA temp_store = 2;", NULL, NULL, &szerrmsg);
	if (ret_val != SQLITE_OK || szerrmsg != NULL)
	{
		PRT_INFO("PRAGMA temp_store failed: %d, %s\n", ret_val, szerrmsg);
		sqlite3_free(szerrmsg);
		goto ERR_EIXT;
	}

	// 配置数据写日志关
	ret_val = sqlite3_exec(p_db_param->p_sql, "PRAGMA journal_mode = DELETE;", NULL, NULL, &szerrmsg);
	if (ret_val != SQLITE_OK || szerrmsg != NULL)
	{
		PRT_INFO("PRAGMA journal_mode failed: %d, %s\n", ret_val, szerrmsg);
		sqlite3_free(szerrmsg);
		goto ERR_EIXT;
	}

ERR_EIXT:
	if (SQLITE_OK == ret_val)
	{
		return SAL_SOK;
	}
	else
	{
		sqlite3_close(p_db_param->p_sql);
		return SAL_FAIL;
	}
}


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
SAL_STATUS dspttk_db_create_table(DB_HANDLE *db_handle, const CHAR *table_name, SQL_KEY_PAIR key_pair[], const UINT32 key_num)
{
	UINT32 i = 0;
	INT32 ret_val = 0;
	CHAR sql_cmd[SQL_CMD_LEN_MAX] = {0}, *szerrmsg = NULL;
	DB_PARAM_INNER *p_db_param = (DB_PARAM_INNER *)(*db_handle);
	CHAR key_type_str[SQL_TYPE_TOTAL][KEY_TYPE_LEN_MAX] = {"INTEGER", "TEXT", "REAL", "BLOB", "NUMERIC", "DATETIME"};

	if (NULL == db_handle || NULL == table_name || NULL == key_pair || 0 == key_num)
	{
		PRT_INFO("the input paramters are invalid, db_handle: %p, table_name: %p, key_pair: %p, key_num: %u\n", \
				db_handle, table_name, key_pair, key_num);
		return SAL_FAIL;
	}

	#if 0 // check whether the table that to be created is existent
	UINT32 group_num = 1;
	CHAR sql_cond[64] = {0}, tbl_cnt[8] = {0}, *p_tbl_cnt[1] = {tbl_cnt};
	snprintf(sql_cond, sizeof(sql_cond), "TYPE=\"table\" AND NAME=\"%s\"", table_name);
	ret_val = db_select(*db_handle, "sqlite_master", "count(*)", sql_cond, p_tbl_cnt, &group_num);
	if (SAL_SOK == ret_val)
	{
		PRT_INFO("tbl_cnt: %s, group_num: %u\n", tbl_cnt, group_num);
	}
	#endif

	for (i = 0; i < key_num; i++)
	{
		if (key_pair[i].type < SQL_TYPE_INTEGER || key_pair[i].type > SQL_TYPE_DATETIME)
		{
			PRT_INFO("the key[%u] type is invalid: %d, range: [%d, %d]\n", \
					i, key_pair[i].type, SQL_TYPE_INTEGER, SQL_TYPE_DATETIME);
			return SAL_FAIL;
		}
	}

	p_db_param = (DB_PARAM_INNER *)realloc(p_db_param, sizeof(DB_PARAM_INNER) + sizeof(SQL_KEY_PAIR) * key_num);
	if (NULL != p_db_param)
	{
		*db_handle = (DB_HANDLE)p_db_param;
		p_db_param->key_num = key_num;
		memcpy(p_db_param->key_pair, key_pair, sizeof(SQL_KEY_PAIR) * key_num);
	}
	else
	{
		PRT_INFO("realloc for db_handle failed, buffer size: %zu\n", sizeof(DB_PARAM_INNER) + sizeof(SQL_KEY_PAIR) * key_num);
		return SAL_FAIL;
	}

	// 创建表，第一个列默认为PRIMARY KEY
	snprintf(sql_cmd, SQL_CMD_LEN_MAX, "CREATE TABLE IF NOT EXISTS %s(", table_name);
	snprintf(sql_cmd+strlen(sql_cmd), SQL_CMD_LEN_MAX-strlen(sql_cmd), "%s %s PRIMARY KEY, ", key_pair[0].name, key_type_str[key_pair[0].type]);
	for (i = 1; i < key_num; i++)
	{
		if (strlen(sql_cmd) + strlen(key_pair[i].name) + strlen(key_type_str[key_pair[i].type]) + 4 <= SQL_CMD_LEN_MAX)
		{
			sprintf(sql_cmd+strlen(sql_cmd), "%s %s, ", key_pair[i].name, key_type_str[key_pair[i].type]);
		}
		else
		{
			PRT_INFO("the buffer of sql_cmd reaches the limit, sql_cmd len: %lu, key_pair[%d]: %s %s\n", \
					strlen(sql_cmd), i, key_pair[i].name, key_type_str[key_pair[i].type]);
			return SAL_FAIL;
		}
	}
	// 将最后的“, ”换成“)”
	sql_cmd[strlen(sql_cmd)-2] = ')';
	sql_cmd[strlen(sql_cmd)-1] = '\0';

	ret_val = sqlite3_exec(p_db_param->p_sql, sql_cmd, NULL, NULL, &szerrmsg);
	if (ret_val == SQLITE_OK)
	{
		return SAL_SOK;
	}
	else
	{
		PRT_INFO("CREATE TABLE failed: %d, %s, sql_cmd: %s\n", ret_val, szerrmsg, sql_cmd);
		sqlite3_free(szerrmsg);
		return SAL_FAIL;
	}
}


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
SAL_STATUS dspttk_db_parse_value_row(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *val_row, CHAR *val_single[])
{
	INT32 ret_val = 0, nleft = 0;
	UINT32 i = 0, len = 0;
	regex_t regex = {0};
	CHAR reg_pattern_single[] = "\\s*([^,]*)\\s*";
	CHAR *p_reg_pattern = NULL, *p_tmp = NULL;
	regmatch_t *p_match_str = NULL;
	CHAR err_info[128] = {0};
	DB_PARAM_INNER *p_db_param = (DB_PARAM_INNER *)(db_handle);

	if (NULL == db_handle || NULL == table_name || NULL == val_row || NULL == val_single)
	{
		PRT_INFO("the input paramters are invalid, db_handle: %p, table_name: %p, val_row: %p, val_single: %p\n", \
				db_handle, table_name, val_row, val_single);
		return SAL_FAIL;
	}

	p_match_str = (regmatch_t *)malloc(sizeof(regmatch_t) * (p_db_param->key_num+1));
	if (NULL == p_match_str)
	{
		PRT_INFO("malloc for p_match_str failed, buffer size: %zu\n", sizeof(regmatch_t) * p_db_param->key_num);
		return SAL_FAIL;
	}

	nleft = (strlen(reg_pattern_single) + 1) * p_db_param->key_num;
	p_reg_pattern = (CHAR *)malloc(nleft);
	if (NULL != p_reg_pattern)
	{
		p_tmp = p_reg_pattern;
		for (i = 0; i < p_db_param->key_num-1; i++)
		{
			snprintf(p_tmp, nleft, "%s,", reg_pattern_single);
			p_tmp += strlen(reg_pattern_single) + 1;
			nleft -= strlen(reg_pattern_single) + 1;
		}
		snprintf(p_tmp, nleft, "%s", reg_pattern_single);
	}
	else
	{
		PRT_INFO("malloc for p_reg_pattern failed, buffer size: %u\n", nleft);
		free(p_match_str);
		return SAL_FAIL;
	}

	ret_val = regcomp(&regex, p_reg_pattern, REG_EXTENDED);
	if (ret_val != 0)
	{
		regerror(ret_val, &regex, err_info, sizeof(err_info));
		PRT_INFO("regcomp failed, error info: %s, pattern: %s\n", err_info, p_reg_pattern);
		free(p_match_str);
		free(p_reg_pattern);
		return SAL_FAIL;
	}

	ret_val = regexec(&regex, val_row, p_db_param->key_num+1, p_match_str, 0);
	if (0 == ret_val)
	{
		for (i = 1; i <= p_db_param->key_num; i++)
		{
			if (p_match_str[i].rm_eo > p_match_str[i].rm_so)
			{
                len = p_match_str[i].rm_eo - p_match_str[i].rm_so;
				strncpy(val_single[i-1], val_row+p_match_str[i].rm_so, len);
                val_single[i-1][len] = '\0';
				//PRT_INFO("%u: %s\n", i-1, val_single[i-1]);
			}
		}
		ret_val = SAL_SOK;
	}
	else
	{
		if (ret_val != REG_NOMATCH)
		{
			regerror(ret_val, &regex, err_info, sizeof(err_info));
			PRT_INFO("regexec failed, error info: %s, val_row: %s\n", err_info, val_row);
		}
		else
		{
			PRT_INFO("regexec failed, REG_NOMATCH, val_row: %s\n", val_row);
		}
		ret_val = SAL_FAIL;
	}

	regfree(&regex);
	free(p_match_str);
	free(p_reg_pattern);

	return ret_val;
}


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
SAL_STATUS dspttk_db_insert(DB_HANDLE db_handle, const CHAR *table_name, CHAR *value_inst[], const UINT32 group_num)
{
	UINT32 i = 0, j = 0, alloc_buf_size = 0;
	UINT32 value_str_buf_len = 0, name_str_buf_len = 0;
	INT32 ret_val = 0;
	CHAR sql_cmd[SQL_CMD_LEN_MAX] = {0}, *szerrmsg = NULL;
	CHAR *p_buf = NULL, *p_value_str = NULL, **p_value_ptr = NULL;
	CHAR *p_inst_name = NULL, *p_inst_value = NULL;
	DB_PARAM_INNER *p_db_param = (DB_PARAM_INNER *)(db_handle);

	if (NULL == db_handle || NULL == table_name || NULL == value_inst)
	{
		PRT_INFO("the input paramters are invalid, db_handle: %p, table_name: %p, value_inst: %p\n", \
				db_handle, table_name, value_inst);
		return SAL_FAIL;
	}

	value_str_buf_len = SQL_VAL_STR_LEN_MAX * p_db_param->key_num;
	name_str_buf_len = SQL_KEY_NAME_LEN_MAX * p_db_param->key_num;
	alloc_buf_size = sizeof(CHAR *) * p_db_param->key_num + value_str_buf_len * 2 + name_str_buf_len;

	/**
	 * ======================================= p_buf的结构 =======================================
	 *              p_value_ptr             |    p_value_str    |   p_inst_value    |   p_inst_name 
	 * sizeof(CHAR *) * p_db_param->key_num | value_str_buf_len | value_str_buf_len | name_str_buf_len
	 */
	p_buf = (CHAR *)malloc(alloc_buf_size);
	if (NULL != p_buf)
	{
		p_value_ptr = (CHAR **)p_buf;
		p_value_str = p_buf + sizeof(CHAR *) * p_db_param->key_num;
		p_inst_value = p_value_str + value_str_buf_len;
		p_inst_name = p_inst_value + value_str_buf_len;
	}
	else
	{
		PRT_INFO("malloc for p_buf failed, buffer size: %u\n", alloc_buf_size);
		return SAL_FAIL;
	}

	for (i = 0; i < p_db_param->key_num; i++)
	{
		*(p_value_ptr + i) = p_value_str + SQL_VAL_STR_LEN_MAX * i;
	}

	for (i = 0; i < group_num; i++)
	{
		memset(p_value_str, 0, value_str_buf_len);
		ret_val = dspttk_db_parse_value_row(db_handle, table_name, value_inst[i], p_value_ptr);
		if (SAL_SOK != ret_val)
		{
			PRT_INFO("db_parse_value_row failed, value_inst[%u]: %s\n", i, value_inst[i]);
			free(p_buf);
			return SAL_FAIL;
		}

		memset(p_inst_value, 0, value_str_buf_len);
		memset(p_inst_name, 0, name_str_buf_len);
		for (j = 0; j < p_db_param->key_num; j++)
		{
			if (strlen(p_value_ptr[j]) > 0)
			{
				snprintf(p_inst_name+strlen(p_inst_name), name_str_buf_len-strlen(p_inst_name), "%s, ", p_db_param->key_pair[j].name);
				if (p_db_param->key_pair[j].type == SQL_TYPE_INTEGER || p_db_param->key_pair[j].type == SQL_TYPE_REAL)
				{
					snprintf(p_inst_value+strlen(p_inst_value), value_str_buf_len-strlen(p_inst_value), "%s, ", p_value_ptr[j]);
				}
				else
				{
					snprintf(p_inst_value+strlen(p_inst_value), value_str_buf_len-strlen(p_inst_value), "'%s', ", p_value_ptr[j]);
				}
			}
		}

		p_inst_name[strlen(p_inst_name)-2] = '\0'; // 删除最后的“, ”
		p_inst_value[strlen(p_inst_value)-2] = '\0'; // 删除最后的“, ”
		snprintf(sql_cmd, SQL_CMD_LEN_MAX, "INSERT INTO %s (%s) VALUES (%s)", table_name, p_inst_name, p_inst_value);

		ret_val = sqlite3_exec(p_db_param->p_sql, sql_cmd, NULL, NULL, &szerrmsg);
		if (ret_val == SQLITE_OK)
		{
			memset(sql_cmd, 0, SQL_CMD_LEN_MAX);
		}
		else
		{
			PRT_INFO("INSERT failed: %d, %s, sql_cmd: %s\n", ret_val, szerrmsg, sql_cmd);
			sqlite3_free(szerrmsg);
			break;
		}
	}

	free(p_buf);

	if (ret_val == SQLITE_OK)
	{
		return SAL_SOK;
	}
	else
	{
		return SAL_FAIL;
	}
}


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
SAL_STATUS dspttk_db_update(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *value_update)
{
	UINT32 i = 0, alloc_buf_size = 0, value_str_buf_len = 0;
	BOOL b_update = SAL_FALSE;
	INT32 ret_val = 0;
	CHAR sql_cmd[SQL_CMD_LEN_MAX] = {0}, *szerrmsg = NULL;
	CHAR *p_buf = NULL, *p_value_str = NULL, **p_value_ptr = NULL;
	DB_PARAM_INNER *p_db_param = (DB_PARAM_INNER *)(db_handle);

	if (NULL == db_handle || NULL == table_name || NULL == value_update)
	{
		PRT_INFO("the input paramters are invalid, db_handle: %p, table_name: %p, value_update: %p\n", \
				db_handle, table_name, value_update);
		return SAL_FAIL;
	}

	value_str_buf_len = SQL_VAL_STR_LEN_MAX * p_db_param->key_num;
	alloc_buf_size = sizeof(CHAR *) * p_db_param->key_num + value_str_buf_len;

	/**
	 * ==================== p_buf的结构 ====================
	 *              p_value_ptr             |    p_value_str
	 * sizeof(CHAR *) * p_db_param->key_num | value_str_buf_len
	 */
	p_buf = (CHAR *)malloc(alloc_buf_size);
	if (NULL != p_buf)
	{
		p_value_ptr = (CHAR **)p_buf;
		p_value_str = p_buf + sizeof(CHAR *) * p_db_param->key_num;
		memset(p_value_str, 0, value_str_buf_len);
		for (i = 0; i < p_db_param->key_num; i++)
		{
			*(p_value_ptr + i) = p_value_str + SQL_VAL_STR_LEN_MAX * i;
		}
	}
	else
	{
		PRT_INFO("malloc for p_buf failed, buffer size: %u\n", alloc_buf_size);
		return SAL_FAIL;
	}

	ret_val = dspttk_db_parse_value_row(db_handle, table_name, value_update, p_value_ptr);
	if (SAL_SOK == ret_val && strlen(p_value_ptr[0]) > 0)
	{
		snprintf(sql_cmd, SQL_CMD_LEN_MAX, "UPDATE %s SET ", table_name);
		for (i = 1; i < p_db_param->key_num; i++)
		{
			if (strlen(p_value_ptr[i]) > 0)
			{
				b_update = SAL_TRUE;
				if (p_db_param->key_pair[i].type == SQL_TYPE_INTEGER || p_db_param->key_pair[i].type == SQL_TYPE_REAL)
				{
					snprintf(sql_cmd+strlen(sql_cmd), SQL_CMD_LEN_MAX-strlen(sql_cmd), "%s = %s, ", \
							 p_db_param->key_pair[i].name, p_value_ptr[i]);
				}
				else
				{
					snprintf(sql_cmd+strlen(sql_cmd), SQL_CMD_LEN_MAX-strlen(sql_cmd), "%s = '%s', ", \
							 p_db_param->key_pair[i].name, p_value_ptr[i]);
				}
			}
		}

		if (b_update)
		{
			// 去掉最后的“, ”，所以有“-2”的操作
			snprintf(sql_cmd+strlen(sql_cmd)-2, SQL_CMD_LEN_MAX-strlen(sql_cmd), " WHERE %s = %s", \
					 p_db_param->key_pair[0].name, p_value_ptr[0]);

			ret_val = sqlite3_exec(p_db_param->p_sql, sql_cmd, NULL, NULL, &szerrmsg);
			if (ret_val == SQLITE_OK)
			{
				ret_val = SAL_SOK;
			}
			else
			{
				PRT_INFO("UPDATE failed: %d, %s, sql_cmd: %s\n", ret_val, szerrmsg, sql_cmd);
				sqlite3_free(szerrmsg);
				ret_val = SAL_FAIL;
			}
		}
	}
	else
	{
		PRT_INFO("db_parse_value_row failed, value_update: %s\n", value_update);
		ret_val = SAL_FAIL;
	}

	free(p_buf);
	return ret_val;
}


static INT32 select_cb(void *pusr, INT32 col_num, CHAR *col_data[], CHAR *col_name[])
{
	INT32 i = 0;
	CHAR *p_tmp = NULL;
	SELECT_OUTPUT *p_sel_out = (SELECT_OUTPUT *)pusr;

	if (p_sel_out->group_num > 0 && NULL != p_sel_out->results_seq)
	{
		if (p_sel_out->group_idx < p_sel_out->group_num)
		{
			p_tmp = p_sel_out->results_seq[p_sel_out->group_idx];
			for (i = 0; i < col_num; i++)
			{
				if (NULL != col_data[i])
				{
					//PRT_INFO("%-16s ---- %s\n", col_name[i], col_data[i]);
					sprintf(p_tmp, "%s, ", col_data[i]);
					p_tmp += strlen(col_data[i]) + 2;
				}
				else
				{
					sprintf(p_tmp, ", "); // 空内容，逗号保留
					p_tmp += 2;
				}
			}

			*(--p_tmp) = '\0';
			*(--p_tmp) = '\0';
			p_sel_out->group_idx++;

			return 0;
		}
		else
		{
			PRT_INFO("the select output buffer is full, group_num: %u\n", p_sel_out->group_num);
			return -1;
		}
	}
	else
	{
		p_sel_out->group_idx++;
		return 0;
	}
}


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
SAL_STATUS dspttk_db_select(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *column, const CHAR *condition, CHAR *results_seq[], UINT32 *group_num)
{
	INT32 ret_val = 0;
	CHAR sql_cmd[SQL_CMD_LEN_MAX] = {0};
	CHAR *szerrmsg = NULL;
	SELECT_OUTPUT sel_out = {0};
	DB_PARAM_INNER *p_db_param = (DB_PARAM_INNER *)(db_handle);

	if (NULL == db_handle || NULL == table_name || NULL == group_num)
	{
		PRT_INFO("the input paramters are invalid, db_handle: %p, table_name: %p, group_num: %p\n", \
				db_handle, table_name, group_num);
		return SAL_FAIL;
	}

	snprintf(sql_cmd, SQL_CMD_LEN_MAX, "SELECT %s FROM %s", (NULL==column) ? "*" : column, table_name);
	if (NULL != condition)
	{
		snprintf(sql_cmd+strlen(sql_cmd), SQL_CMD_LEN_MAX-strlen(sql_cmd), " %s", condition);
	}

	sel_out.group_num = *group_num;
	sel_out.results_seq = (CHAR **)results_seq;

	ret_val = sqlite3_exec(p_db_param->p_sql, sql_cmd, select_cb, &sel_out, &szerrmsg);
	if (ret_val == SQLITE_OK)
	{
		*group_num = sel_out.group_idx;
		return SAL_SOK;
	}
	else
	{
		PRT_INFO("SELECT failed: %d, %s, sql_cmd: %s\n", ret_val, szerrmsg, sql_cmd);
		sqlite3_free(szerrmsg);
		return SAL_FAIL;
	}
}


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
SAL_STATUS dspttk_db_count(DB_HANDLE db_handle, const CHAR *table_name, UINT32 *count)
{
	INT32 ret_val = 0;
	UINT32 group_num = 1;
	CHAR select_str[SQL_VAL_STR_LEN_MAX] = {0}, *p_select_str = select_str;

	if (NULL == db_handle || NULL == table_name || NULL == count)
	{
		PRT_INFO("the input paramters are invalid, db_handle: %p, table_name: %p, count: %p\n", \
				db_handle, table_name, count);
		return SAL_FAIL;
	}

	ret_val = dspttk_db_select(db_handle, table_name, "COUNT(*)", NULL, &p_select_str, &group_num);
	if (SAL_SOK == ret_val && 1 == group_num)
	{
		*count = strtoul(select_str, NULL, 0);
	}

	return ret_val;
}


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
SAL_STATUS dspttk_db_select_max(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *column, void *max_val)
{
	INT32 ret_val = 0;
	UINT32 i = 0, group_num = 1;
	CHAR select_col[SQL_VAL_STR_LEN_MAX] = {0};
	CHAR select_str[SQL_VAL_STR_LEN_MAX] = {0}, *p_select_str = select_str;
	DB_PARAM_INNER *p_db_param = (DB_PARAM_INNER *)(db_handle);

	if (NULL == db_handle || NULL == table_name || NULL == column || NULL == max_val)
	{
		PRT_INFO("the input paramters are invalid, db_handle: %p, table_name: %p, count: %p, max_val: %p\n", \
				db_handle, table_name, column, max_val);
		return SAL_FAIL;
	}

	for (i = 0; i < p_db_param->key_num; i++)
	{
		if (0 == strcmp(p_db_param->key_pair[i].name, column))
		{
			break;
		}
	}

	if (i < p_db_param->key_num)
	{
		snprintf(select_col, sizeof(select_col), "MAX(%s)", column);
		ret_val = dspttk_db_select(db_handle, table_name, select_col, NULL, &p_select_str, &group_num);
		if (SAL_SOK == ret_val && 1 == group_num)
		{
			if (strlen(select_str) > 0) // 若表中无任何行，返回成功，但select_str中无内容
			{
				switch (p_db_param->key_pair[i].type)
				{
				case SQL_TYPE_INTEGER:
					*(long *)max_val = strtol(select_str, NULL, 0);
					break;
				case SQL_TYPE_REAL:
					*(double *)max_val = strtod(select_str, NULL);
					break;
				default:
					strcpy((CHAR *)max_val, select_str);
					break;
				}
			}
		}
		else
		{
			PRT_INFO("db_select failed, ret_val: %d, group_num: %u\n", ret_val, group_num);
			ret_val = SAL_FAIL;
		}
	}
	else
	{
		PRT_INFO("the column name is invalid: %s\n", column);
		ret_val = SAL_FAIL;
	}

	return ret_val;
}


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
SAL_STATUS dspttk_db_select_min(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *column, void *min_val)
{
	INT32 ret_val = 0;
	UINT32 i = 0, group_num = 1;
	CHAR select_col[SQL_VAL_STR_LEN_MAX] = {0};
	CHAR select_str[SQL_VAL_STR_LEN_MAX] = {0}, *p_select_str = select_str;
	DB_PARAM_INNER *p_db_param = (DB_PARAM_INNER *)(db_handle);

	if (NULL == db_handle || NULL == table_name || NULL == column || NULL == min_val)
	{
		PRT_INFO("the input paramters are invalid, db_handle: %p, table_name: %p, count: %p, min_val: %p\n", \
				db_handle, table_name, column, min_val);
		return SAL_FAIL;
	}

	for (i = 0; i < p_db_param->key_num; i++)
	{
		if (0 == strcmp(p_db_param->key_pair[i].name, column))
		{
			break;
		}
	}

	if (i < p_db_param->key_num)
	{
		snprintf(select_col, sizeof(select_col), "MIN(%s)", column);
		ret_val = dspttk_db_select(db_handle, table_name, select_col, NULL, &p_select_str, &group_num);
		if (SAL_SOK == ret_val && 1 == group_num)
		{
			if (strlen(select_str) > 0) // 若表中无任何行，返回成功，但select_str中无内容
			{
				switch (p_db_param->key_pair[i].type)
				{
				case SQL_TYPE_INTEGER:
					*(long *)min_val = strtol(select_str, NULL, 0);
					break;
				case SQL_TYPE_REAL:
					*(double *)min_val = strtod(select_str, NULL);
					break;
				default:
					strcpy((CHAR *)min_val, select_str);
					break;
				}
			}
		}
		else
		{
			PRT_INFO("db_select failed, ret_val: %d, group_num: %u\n", ret_val, group_num);
			ret_val = SAL_FAIL;
		}
	}
	else
	{
		PRT_INFO("the column name is invalid: %s\n", column);
		ret_val = SAL_FAIL;
	}

	return ret_val;
}


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
SAL_STATUS dspttk_db_delete(DB_HANDLE db_handle, const CHAR *table_name, const CHAR *condition)
{
	INT32 ret_val = 0;
	CHAR sql_cmd[SQL_CMD_LEN_MAX] = {0}, *szerrmsg = NULL;
	DB_PARAM_INNER *p_db_param = (DB_PARAM_INNER *)(db_handle);

	if (NULL == db_handle || NULL == table_name)
	{
		PRT_INFO("the input paramters are invalid, db_handle: %p, table_name: %p\n", \
				db_handle, table_name);
		return SAL_FAIL;
	}

	if (NULL != condition)
	{
		snprintf(sql_cmd, SQL_CMD_LEN_MAX, "DELETE FROM %s %s", table_name, condition);
	}
	else
	{
		snprintf(sql_cmd, SQL_CMD_LEN_MAX, "DELETE FROM %s", table_name);
	}
	ret_val = sqlite3_exec(p_db_param->p_sql, sql_cmd, NULL, NULL, &szerrmsg);
	if (ret_val == SQLITE_OK)
	{
		return SAL_SOK;
	}
	else
	{
		PRT_INFO("DELETE failed: %d, %s, sql_cmd: %s\n", ret_val, szerrmsg, sql_cmd);
		sqlite3_free(szerrmsg);
		return SAL_FAIL;
	}
}


/**
 * @fn      dspttk_db_close_conn
 * @brief   断开与数据库的连接
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * 
 * @return  SAL_STATUS SAL_SOK：断开成功，SAL_FAIL：断开失败
 */
SAL_STATUS dspttk_db_close_conn(DB_HANDLE db_handle)
{
	INT32 ret_val = 0;
	DB_PARAM_INNER *p_db_param = (DB_PARAM_INNER *)(db_handle);

	if (NULL == db_handle)
	{
		PRT_INFO("the db_handle is NULL\n");
		return SAL_FAIL;
	}

	ret_val = sqlite3_close(p_db_param->p_sql);
	if (ret_val == SQLITE_OK)
	{
		return SAL_SOK;
	}
	else
	{
		PRT_INFO("sqlite3_close failed, ret_val: %d, errmsg: %s\n", ret_val, sqlite3_errmsg(p_db_param->p_sql));
		return SAL_FAIL;
	}
}


/**
 * @fn      dspttk_db_close_conn
 * @brief   销毁数据库
 * 
 * @param   [IN] db_handle 数据库句柄，由dspttk_db_create_table()输出
 * 
 * @return  SAL_STATUS SAL_SOK：销毁成功，SAL_FAIL：销毁失败
 */
SAL_STATUS dspttk_db_destroy(DB_HANDLE db_handle)
{
	if (NULL != db_handle)
	{
		free(db_handle);
		db_handle = NULL;
	}

	return SAL_SOK;
}

