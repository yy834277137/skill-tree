/***
 * @file   sal_fop.c
 * @note   2020-2030, Hikvision Digital Technology Co., Ltd.
 * @brief  file operation
 * @author dsp
 * @date   2022-02-24
 * @note
 * @note History:
 */

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include "sal_type.h"
#include "sal_trace.h"
#include "sal_macro.h"

#define LINE_BUF_LEN 256        /* the maximum length of each line in wps.conf */

#line __LINE__ "sal_fop.c"

/**
 * On success, select() return the number of file descriptors contained in the three returned descriptor sets
 * (that is, the total number of bits that are set in readfds, writefds, exceptfds)
 * which may be zero if the timeout expires before anything interesting happens.
 * On error, -1 is returned, and errno is set appropriately;
 * the sets and timeout become undefined, so do not rely on their contents after an error.
 */

/**
 * @fn      SAL_WriteNBytes
 * @brief   向设备fd中写入buf中前count字节数据
 *
 * @param   fd[IN] 设备句柄
 * @param   buf[IN] 需要写入的数据buf
 * @param   count[IN] 需要写入的数据长度
 *
 * @return  >0：实际写入的字节数，其他：写入失败
 */
INT32 SAL_WriteNBytes(INT32 fd, const void *buf, UINT32 count)
{
    UINT32 nleft = count, nwritten = 0;
    UINT8 *ptr = (UINT8 *)buf;

    if ((fd < 0) || (NULL == buf) || (0 == count))
    {
        SAL_LOGE("fd[%d] OR buf[%p] OR count[%u] is invalid\n", fd, buf, count);
        return -1;
    }

    while (nleft > 0)
    {
        nwritten = write(fd, ptr, nleft);
        if (-1 == nwritten)
        {
            if (errno == EINTR)
            {
                SAL_LOGE("EINTR accurred, try to write again\n");
                continue;
            }
            else
            {
                SAL_LOGE("write failed, errno: %d, %s\n", errno, strerror(errno));
                break;
            }
        }

        nleft -= nwritten;
        ptr += nwritten;
    }

    return (count - nleft);
}

/**
 * @fn      SAL_ReadNBytes
 * @brief   等待设备fd有可读数据，若可读数据长度不超过buf_size，则读取所有数据到buf中
 *          若可读数据长度超过buf_size，则读取buf_size个字节到buf中，即填充满整个buf
 *
 * @param   fd[IN] 设备句柄
 * @param   buf[IN] 读取的数据存入该buf中
 * @param   buf_size[IN] buf的大小，单位：字节
 *
 * @return  >0：实际读取的字节数，其他：读取失败
 */
INT32 SAL_ReadNBytes(INT32 fd, void *buf, const UINT32 buf_size)
{
    INT32 nread = 0, try_times = 3;

    #if 0 /* def DEBUG */
    UINT32 i = 0;
    #endif

    if ((fd < 0) || (NULL == buf) || (0 == buf_size))
    {
        SAL_LOGE("fd[%d] OR buf[%p] OR buf_size[%u] is invalid\n", fd, buf, buf_size);
        return -1;
    }

    while (try_times > 0)
    {
        nread = read(fd, buf, buf_size);
        if (0 < nread && nread <= buf_size)
        {
            #if 0 /* def DEBUG */
            SAL_LOGW("read over, nread: %d...\n", nread);
            for (i = 0; i < nread; i++)
            {
                PRT_LLY("%02X ", *((char *)buf + i));
            }

            PRT_LLY("\n");
            #endif
            break;
        }
        else
        {
            if (errno == EINTR)
            {
                SAL_LOGE("EINTR accurred, try[%d] again\n", try_times);
                try_times--;
            }
            else
            {
                SAL_LOGE("read failed, ret_val: %d, buf_size: %u, errno: %d, %s\n", nread, buf_size, errno, strerror(errno));
                nread = 0;
                break;
            }
        }
    }

    return nread;
}

/**
 * @fn      SAL_ReadTimeEx
 * @brief   等待设备fd有可读数据，尝试读取count个字节数据到buf中，超时则直接返回
 *
 * @param   fd[IN] 设备句柄，取值范围：>=0
 * @param   buf[IN] 读取的数据存入该buf中
 * @param   buf_size[IN] buf的大小，单位：字节
 * @param   timeout_ms[IN] 超时时间，若小于0，则为永久等待
 *
 * @return  >0：实际读取的字节数，其他：读取失败
 */
INT32 SAL_ReadTimeEx(INT32 fd, void *buf, const UINT32 buf_size, long timeout_ms)
{
    INT32 nread = -1, try_times = 3;
    INT32 ret_val = 0;
    fd_set rset;
    struct timeval time_left = {0};

    if (fd < 0 || NULL == buf || 0 == buf_size)
    {
        SAL_LOGE("fd[%d] OR buf[%p] OR buf_size[%u] is invalid\n", fd, buf, buf_size);
        return -1;
    }

    FD_ZERO(&rset);
    FD_SET(fd, &rset);

    if (timeout_ms > 0)
    {
        time_left.tv_sec = timeout_ms / 1000;
        time_left.tv_usec = (timeout_ms % 1000) * 1000;
        ret_val = select(fd + 1, &rset, NULL, NULL, &time_left);
    }
    else
    {
        ret_val = select(fd + 1, &rset, NULL, NULL, NULL); /* wait forever */
    }

    if (ret_val > 0)
    {
        while (try_times-- > 0)
        {
            nread = read(fd, buf, buf_size);
            if (nread > 0)
            {
                break;
            }
            else
            {
                if (errno == EINTR)
                {
                    SAL_LOGE("EINTR accurred, try to read again\n");
                }
                else
                {
                    SAL_LOGE("read failed, ret_val: %u, errno: %d, %s\n", nread, errno, strerror(errno));
                    break;
                }
            }
        }
    }
    else if (0 == ret_val)
    {
        SAL_LOGW("selcet timeout...\n");
    }
    else
    {
        /* if return 0, selcet timeout */
        SAL_LOGE("select failed, ret_val: %d, errno: %d, %s\n", ret_val, errno, strerror(errno));
    }

    return nread;
}

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
SAL_STATUS SAL_ReadFromFile(const CHAR *p_file_name, long offset, void *p_data, UINT32 buf_len, UINT32 *size)
{
    INT32 ret_val = SAL_FAIL;
    FILE *fp = NULL;
    long file_len = 0;
    UINT32 read_size = 0;

    if (NULL == p_file_name || NULL == p_data || NULL == size || 0 == buf_len)
    {
        SAL_LOGE("p_file_name[%p] OR p_data[%p] OR size[%p] is NULL OR buf_len[%d] equals zero \n", \
                 p_file_name, p_data, size, buf_len);
        return SAL_FAIL;
    }

    if (access(p_file_name, 0) == 0) /* 不存在fopen会出现出错误 */
    {
        fp = fopen(p_file_name, "r");
        if (NULL != fp)
        {
            if (0 == fseek(fp, 0, SEEK_END))
            {
                file_len = ftell(fp);
                if (offset >= 0)
                {
                    if (0 != fseek(fp, offset, SEEK_SET))
                    {
                        goto ERR_EXIT;
                    }
                }
                else
                {
                    if (0 != fseek(fp, offset, SEEK_END))
                    {
                        goto ERR_EXIT;
                    }
                }

                if (file_len <= 0) /* 获取到的文件当前读写位置为0或出错，直接输出0并返回 */
                {
                    *size = 0;
                    goto ERR_EXIT;
                }
            }
            else
            {
                goto ERR_EXIT;
            }
        }
        else
        {
            SAL_LOGE("open %s failed, errno: %d, %s\n", p_file_name, errno, strerror(errno));
            return SAL_FAIL;
        }
    }
    else
    {
        SAL_LOGE("file %s is non-existent!\n", p_file_name);
        return SAL_FAIL;
    }

    if (*size == 0)
    {
        *size = file_len;
    }

    if (offset >= 0 && *size > file_len - offset)
    {
        read_size = file_len - offset;
    }
    else if (offset < 0 && *size > -offset)
    {
        read_size = -offset;
    }
    else
    {
        read_size = *size;
    }

    ret_val = fread(p_data, sizeof(U08), read_size, fp);
    if (read_size == ret_val)
    {
        *size = read_size;
        ret_val = SAL_SOK;
    }
    else
    {
        SAL_LOGE("fread '%s' failed, size need to read: %u, ret_val: %u, errno: %d, %s\n", \
                 p_file_name, read_size, ret_val, ferror(fp), strerror(ferror(fp)));
        clearerr(fp);
        ret_val = SAL_FAIL;
    }

ERR_EXIT:
    fclose(fp);

    return ret_val;
}

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
 * @return  SAL_SOK-写入文件成功，ERROR-写入失败
 */
SAL_STATUS SAL_WriteToFile(const CHAR *p_file_name, long offset, INT32 whence, void *p_data, U32 size)
{
    INT32 ret_val = 0;
    FILE *fp = NULL;
    CHAR fopen_mode[4] = "w";

    if (NULL == p_file_name || NULL == p_data || 0 == size)
    {
        SAL_LOGE("p_file_name[%p] OR p_data[%p] OR size[%u] is invalid\n", \
                 p_file_name, p_data, size);
        return SAL_FAIL;
    }

    if (SEEK_SET == whence && 0 == offset)
    {
        strcpy(fopen_mode, "w");
    }
    else if (SEEK_END == whence && 0 == offset)
    {
        strcpy(fopen_mode, "a");
    }
    else
    {
        strcpy(fopen_mode, "r+");
        if (access(p_file_name, 0) != 0) /* 不存在fopen会出现出错误 */
        {
            SAL_LOGE("file %s is non-existent!\n", p_file_name);
            return SAL_FAIL;
        }
    }

    fp = fopen(p_file_name, fopen_mode);
    if (NULL == fp)
    {
        SAL_LOGE("open %s failed, mode: %s, errno:%d, %s\n", p_file_name, fopen_mode, errno, strerror(errno));
        return SAL_FAIL;
    }

    if (0 == strcmp(fopen_mode, "r+"))
    {
        ret_val = fseek(fp, offset, whence);
        if (ret_val != 0)
        {
            SAL_LOGE("fseek %s failed, offset: %ld, whence: %d, errno: %d, %s\n", \
                     p_file_name, offset, whence, errno, strerror(errno));
            fclose(fp);
            return SAL_FAIL;
        }
    }

    ret_val = fwrite(p_data, sizeof(U08), size, fp);
    if (size == ret_val) /* 验证输入文件大小是否正确 */
    {
        ret_val = SAL_SOK;
    }
    else
    {
        SAL_LOGE("fwrite '%s' failed, size need to write: %u, ret_val: %u, errno: %d, %s\n", \
                 p_file_name, size, ret_val, errno, strerror(errno));
        ret_val = SAL_FAIL;
    }

    fflush(fp);
    fclose(fp);
    sync();

    return ret_val;
}

/**
 * @fn      get_file_size
 * @brief   获取文件大小
 *
 * @param   file_name[IN] 文件名，不在当前路径下的需使用绝对路径
 *
 * @return  On success, the file's size is returned.
 *          On error, -1 is returned.
 */
long SAL_GetFileSize(const CHAR *file_name)
{
    long file_len = -1;
    struct stat file_stat;
    FILE *fp = NULL;

    if (NULL == file_name)
    {
        SAL_LOGE("file_name is NULL\n");
        return -1;
    }

    if (stat(file_name, &file_stat) == 0)
    {
        file_len = file_stat.st_size;
    }
    else
    {
        fp = fopen(file_name, "r");
        if (NULL != fp)
        {
            fseek(fp, 0, SEEK_END);
            file_len = ftell(fp);
        }
        else
        {
            SAL_LOGE("open %s failed, errno:%d, %s\n", file_name, errno, strerror(errno));
        }

        if (NULL != fp)
        {
            fclose(fp);
        }
    }

    return file_len;
}

