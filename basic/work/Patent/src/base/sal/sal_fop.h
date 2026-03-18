/***
 * @file   sal_fop.h
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  file operation
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#ifndef __SAL_FOP__
#define __SAL_FOP__

#ifdef __cplusplus
extern "C" {
#endif

#include "sal_type.h"
#include "sal_macro.h"


/**
 * @fn      writen
 * @brief   向设备fd中写入buf中前count字节数据
 *
 * @param   fd[IN] 设备句柄
 * @param   buf[IN] 需要写入的数据buf
 * @param   count[IN] 需要写入的数据长度
 *
 * @return  >0：实际写入的字节数，其他：写入失败
 */
INT32 SAL_WriteNBytes(INT32 fd, const void *buf, UINT32 count);

/**
 * @fn      readn
 * @brief   等待设备fd有可读数据，若可读数据长度不超过buf_size，则读取所有数据到buf中
 *          若可读数据长度超过buf_size，则读取buf_size个字节到buf中，即填充满整个buf
 *
 * @param   fd[IN] 设备句柄
 * @param   buf[IN] 读取的数据存入该buf中
 * @param   buf_size[IN] buf的大小，单位：字节
 *
 * @return  >0：实际读取的字节数，其他：读取失败
 */
INT32 SAL_ReadNBytes(INT32 fd, void *buf, const UINT32 buf_size);

/**
 * @fn      readex
 * @brief   等待设备fd有可读数据，尝试读取count个字节数据到buf中，超时则直接返回
 *
 * @param   fd[IN] 设备句柄，取值范围：>=0
 * @param   buf[IN] 读取的数据存入该buf中
 * @param   buf_size[IN] buf的大小，单位：字节
 * @param   timeout_ms[IN] 超时时间，若小于0，则为永久等待
 *
 * @return  >0：实际读取的字节数，其他：读取失败
 */
INT32 SAL_ReadTimeEx(INT32 fd, void *buf, const UINT32 buf_size, long timeout_ms);

/**
 * @function:   SAL_ReadFromFile
 * @brief:      从文件中读取数据到p_data中
 * @param[in]:  CHAR *p_file_name
 * @param[in]:  long offset
 * @param[in]:  VOID *p_data
 * @param[in]:  UINT32 buf_len
 * @param[in]:  UINT32 *size
 * @param[out]: None
 * @return:     SAL_STATUS
 */
SAL_STATUS SAL_ReadFromFile(const CHAR *p_file_name, long offset, void *p_data, UINT32 buf_len, UINT32 *size);

/**
 * @fn      write_file
 * @brief   写入数据p_data到文件中，文件存在则覆盖
 *
 * @param   p_file_name[IN] 文件名
 * @param   offset[IN] 与fseek函数的第二个参数相同
 * @param   whence[IN] 与fseek函数的第三个参数相同
 * @param   p_data[IN] 需要写入的数据buf
 * @param   size[IN] 需要写入的数据长度
 *
 * @note    创建新文件并写入，offset填0，whence填SEEK_SET
 * @note    追加内容到文件末，offset填0，whence填SEEK_END
 *
 * @return  OK-写入文件成功，ERROR-写入失败
 */
SAL_STATUS SAL_WriteToFile(const CHAR *p_file_name, long offset, INT32 whence, void *p_data, U32 size);

/**
 * @fn      get_file_size
 * @brief   获取文件大小
 *
 * @param   file_name[IN] 文件名，不在当前路径下的需使用绝对路径
 *
 * @return  On success, the file's size is returned.
 *          On error, -1 is returned.
 */
long SAL_GetFileSize(IN const CHAR *file_name);

#if 0

/**
 * @fn      get_current_path
 * @brief   获取当前程序所在路径与进程名
 *
 * @param   dir[OUT] 当前程序所在路径，不包括最后的“/”
 * @param   name[OUT] 进程名
 *
 * @return  OK-获取成功，ERROR-获取失败
 */
SAL_STATUS sal_get_current_path(OUT char *dir, OUT char *name);

/**
 * @fn      get_filename
 * @brief   获取文件名
 *
 * @param   filepath[IN] 文件所在路径
 * @param   filename[OUT] 文件名
 */
void sal_get_filename(char *filepath, char *filename);

/**
 * @fn		get_filename_suffixes
 * @brief	获取文件的后缀名，后缀名是以.xxx结尾的
 *
 * @param	filename[IN] 文件名，可以是完整路径
 * @param	suffixes[OUT] 后缀名，比如9131.xml的后缀名为xml
 *
 * @return	>0：后缀名在文件名的起始位置，比如9131.xml返回值5，其他：异常
 */
int sal_get_filename_suffixes(const char *filename, char *suffixes);

/**
 * @fn      get_file_size
 * @brief   获取文件大小
 *
 * @param   file_name[IN] 文件名，不在当前路径下的需使用绝对路径
 *
 * @return  On success, the file's size is returned.
 *          On error, -1 is returned.
 */
long SAL_GetFileSize(IN char *file_name);


/**
 * @fn      delete_one_line_from_file
 * @brief   从文本文件中删除一行
 * @warning 文本文件中每行不能超过256个字符
 *
 * @param   fn[IN] 文件名
 * @param   line_num[IN] 行号，从1开始计数
 */
void sal_delete_one_line_from_file(IN char *fn, IN int line_num);


/**
 * @fn      insert_one_line_to_file
 * @brief   向文本文件中插入一行
 * @warning 文本文件中每行不能超过256个字符
 *
 * @param   fn[IN] 文件名
 * @param   line_num[IN] 行号，即字符串将插入到此行，从1开始计数
 * @param   line_str[IN] 字符串
 */
void sal_insert_one_line_to_file(IN char *fn, IN int line_num, IN char *line_str);


/**
 * @fn      get_line_equal_value_pattern
 * @brief   在文件文件中查找“xxx = yyy”格式的行，输出“yyy”，并返回行号
 * @warning 文本文件中每行不能超过256个字符
 *
 * @param   filepath[IN] 文件名
 * @param   reg_pattern[IN] 查找的正则表达式
 * @param   value_str[OUT] 等号“=”后的值，可以为NULL
 *
 * @return  查找成功：行号，未查找到：0，出现异常：-1
 */
int sal_get_line_equal_value_pattern(IN const char *filepath, IN const char *reg_pattern, OUT char *value_str);

/**
 * @fn      set_file_flags
 * @brief   设置文件的某个属性，比如设置文件阻塞->非阻塞
 *
 * @param   fd[IN] 文件句柄
 * @param   flags[IN] 文件属性
 *
 * @return  OK-设置成功，ERROR-设置失败
 */
SAL_STATUS sal_set_file_flags(int fd, int flags);

/**
 * @fn      sal_nlz
 * @brief   计算32位无符号数前导0的位数，比如0x29前导0的位数为26
 *
 * @param   u32_val[IN] 32位无符号数
 *
 * @return  前导0的位数
 */
U32 sal_nlz(IN U32 u32_val);

#endif

#ifdef __cplusplus
}
#endif

#endif

