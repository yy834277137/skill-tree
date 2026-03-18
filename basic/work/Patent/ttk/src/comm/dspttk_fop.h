
#ifndef _DSPTTK_FOP_H_
#define _DSPTTK_FOP_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "sal_type.h"


typedef enum
{
    RM_FILE_ONLY,     //仅删除当前目录及目录下的文件
    RM_DIR_ONLY,      //仅删除当前目录下的空文件夹
    RM_ALL,           //删除目录下所有文件包括该目录
} RM_ATTR;

/**
 * @fn      dspttk_writen
 * @brief   向设备fd中写入buf中前count字节数据
 *
 * @param   fd[IN] 设备句柄
 * @param   buf[IN] 需要写入的数据buf
 * @param   count[IN] 需要写入的数据长度
 *
 * @return  >0：实际写入的字节数，其他：写入失败
 */
UINT32 dspttk_writen(int fd, const void *buf, size_t count);

/**
 * @fn      dspttk_readn
 * @brief   等待设备fd有可读数据，若可读数据长度不超过buf_size，则读取所有数据到buf中
 *          若可读数据长度超过buf_size，则读取buf_size个字节到buf中，即填充满整个buf
 *
 * @param   fd[IN] 设备句柄
 * @param   buf[IN] 读取的数据存入该buf中
 * @param   buf_size[IN] buf的大小，单位：字节
 *
 * @return  >0：实际读取的字节数，其他：读取失败
 */
int dspttk_readn(int fd, void *buf, const size_t buf_size);

/**
 * @fn      dspttk_read_file
 * @brief   从文件中读取数据到p_data中
 *
 * @param   p_file_name[IN] 文件名
 * @param   offset[IN] 从文件起始往后偏移多少字节开始读数据
 * @param   p_data[OUT] 数据存储的buf
 * @param   size[IN/OUT] 输入需要读取的数据长度，输出实际读取的数据长度
 *
 * @return  SAL_SOK-读取文件成功，SAL_FAIL-读取失败
 */
SAL_STATUS dspttk_read_file(CHAR *p_file_name, long offset, void *p_data, UINT32 *size);

/**
 * @fn      dspttk_remove_dir_or_file
 * @brief   删除当前文件夹
 *
 * @param   p_file_name[IN] 文件夹名
 * @param   RM_ATTR 删除属性
 * @return  SAL_SOK-写入文件成功，SAL_FAIL-写入失败
 */
SAL_STATUS dspttk_remove_dir_or_file(CHAR * p_dir_name, RM_ATTR rm_attr);

/**
 * @fn      dspttk_write_file
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
 * @return  SAL_SOK-写入文件成功，SAL_FAIL-写入失败
 */
SAL_STATUS dspttk_write_file(CHAR *p_file_name, long offset, int whence, void *p_data, UINT32 size);

/**
 * @fn		dspttk_get_file_suffixes
 * @brief	获取文件的后缀名，后缀名是以.xxx结尾的
 *
 * @param	filename[IN] 文件名，可以是完整路径
 * @param	suffixes[OUT] 后缀名，比如9131.xml的后缀名为xml
 *
 * @return	>0：后缀名在文件名的起始位置，比如9131.xml返回值5，其他：异常
 */
int dspttk_get_file_suffixes(const CHAR *filename, CHAR *suffixes);

/**
 * @fn      dspttk_get_file_size
 * @brief   获取文件大小
 *
 * @param   file_name[IN] 文件名，不在当前路径下的需使用绝对路径
 *
 * @return  On success, the file's size is returned.
 *          On error, -1 is returned.
 */
long dspttk_get_file_size(CHAR *file_name);

/**
 * @fn      dspttk_get_file_num
 * @brief   获取目录下的文件数量
 * 
 * @param   [IN] pPath 目录地址，不在当前路径下的需使用绝对路径
 * 
 * @return  INT32 >=0：文件名数量；<0：获取失败
 */
INT32 dspttk_get_file_num(CHAR *pPath);

/**
 * @fn      dspttk_get_file_name_list
 * @brief   获取目录下普通文件列表
 * @warning 函数内部有malloc申请内存，使用完成后，ppRegFile需调用free()释放
 * 
 * @param   [IN] pPath 目录地址，不在当前路径下的需使用绝对路径
 * @param   [OUT] ppRegFile 存储文件名字符串的指针，输出的文件名带pPath前缀，使用完成后需free
 * 
 * @return  INT32 >=0：文件名数量，=0时无需free ppRegFile；<0：获取失败
 */
INT32 dspttk_get_file_name_list(CHAR *pPath, CHAR **ppRegFile[]);

/**
 * @fn      dspttk_get_dir_name_list
 * @brief   获取目录下的文件夹名，支持逐级目录遍历
 * @note    使用完成后，ppDir需调用free()释放
 *
 * @param   [IN] pPath 目录地址，不在当前路径下的需使用绝对路径
 * @param   [OUT] dirName 存储文件夹字符串的指针，输出的文件夹带pPath前缀
 * @param   [IN] s32Hierarchy 最多遍历的层次数，0表示仅当前目录
 * 
 * @return  INT32 >=0：文件夹数量；<0：获取失败
 */
INT32 dspttk_get_dir_name_list(CHAR *pPath, CHAR **ppDir[], INT32 s32Hierarchy);

#ifdef __cplusplus
}
#endif

#endif

